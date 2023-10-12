/**
 *****************************************************************************************
 * @file	send_mosq.c
 * @brief	Generating MQTT messages to send
 * @todo	File name : send_mosq.c -> mqtt_client_send.c
 *****************************************************************************************
 */

/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.

Copyright (c) 2019-2022 Modified by Renesas Electronics.
*/

#include "sdk_type.h"

#if defined (__SUPPORT_MQTT__)

#include "mqtt_client.h"
#include "mosquitto_internal.h"
#include "mqtt3_protocol.h"
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"


int _mosquitto_send_pingreq(struct mosquitto *mosq)
{
	int rc;

	MQTT_DBG_PRINT("Client %s tx PINGREQ\n", mosq->id);
	rc = _mosquitto_send_simple_command(mosq, PINGREQ);

	if (rc == MOSQ_ERR_SUCCESS)
	{
		mosq->ping_t = mosquitto_time();
	}

	return rc;
}

int _mosquitto_send_pingresp(struct mosquitto *mosq)
{
	if (mosq)
	{
		MQTT_DBG_PRINT("Client %s tx PINGRESP\n", mosq->id);
	}

	return _mosquitto_send_simple_command(mosq, PINGRESP);
}

int _mosquitto_send_puback(struct mosquitto *mosq, uint16_t mid)
{
	int status;

	if (mosq)
	{
		MQTT_DBG_PRINT("[%s] (Tx: Msg_ID=%d)\n", "PUBACK", mid);
		MQTT_DBG_TEMP(" [%s] Client %s sending PUBACK (Mid: %d)\n", __func__, mosq->id, mid);
	}

	status = _mosquitto_send_command_with_mid(mosq, PUBACK, mid, false);
	return status;
}

int _mosquitto_send_pubcomp(struct mosquitto *mosq, uint16_t mid)
{
	int status;

	if (mosq)
	{
		MQTT_DBG_PRINT("[%s] (Tx: Msg_ID=%d)\n", "PUBCOMP", mid);
		MQTT_DBG_TEMP(" [%s] Client %s sending PUBCOMP (Mid: %d)\n", __func__, mosq->id, mid);
	}

	status = _mosquitto_send_command_with_mid(mosq, PUBCOMP, mid, false);
	return status;
}

int _mosquitto_send_publish(struct mosquitto *mosq, uint16_t mid, const char *topic,
							uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup)
{
	MQTT_DBG_TEMP(" [%s] Client %s sending PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))\n",
				  __func__, mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);
	mosq->in_callback = false;
	return _mosquitto_send_real_publish(mosq, mid, topic, payloadlen, payload, qos, retain,	dup);
}

int _mosquitto_send_pubrec(struct mosquitto *mosq, uint16_t mid)
{
	if (mosq)
	{
		MQTT_DBG_PRINT("[%s] (Tx: Msg_ID=%d)\n", "PUBREC", mid);
		MQTT_DBG_TEMP("[%s] Client %s sending PUBREC (Mid: %d)\n", __func__, mosq->id, mid);
	}

	return _mosquitto_send_command_with_mid(mosq, PUBREC, mid, false);
}

int _mosquitto_send_pubrel(struct mosquitto *mosq, uint16_t mid)
{
	if (mosq)
	{
		MQTT_DBG_PRINT("[%s] (Tx: Msg_ID=%d)\n", "PUBREL", mid);
		MQTT_DBG_TEMP("[%s] Client %s sending PUBREL (Mid: %d)\n", __func__, mosq->id, mid);
	}

	return _mosquitto_send_command_with_mid(mosq, PUBREL | 2, mid, false);
}

/* For PUBACK, PUBCOMP, PUBREC, and PUBREL */
int _mosquitto_send_command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup)
{
	struct _mosquitto_packet *packet = NULL;
	int rc;

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));

	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	packet->command = command;

	if (dup)
	{
		packet->command |= 8;
	}

	packet->remaining_length = 2;
	rc = _mosquitto_packet_alloc(packet);
	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	packet->payload[packet->pos + 0] = MOSQ_MSB(mid);
	packet->payload[packet->pos + 1] = MOSQ_LSB(mid);

	return _mosquitto_packet_queue(mosq, packet);
}

/* For DISCONNECT, PINGREQ and PINGRESP */
int _mosquitto_send_simple_command(struct mosquitto *mosq, uint8_t command)
{
	struct _mosquitto_packet *packet = NULL;
	int rc;

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));

	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	packet->command = command;
	packet->remaining_length = 0;

	rc = _mosquitto_packet_alloc(packet);

	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	return _mosquitto_packet_queue(mosq, packet);
}

int _mosquitto_send_real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic,
								 uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup)
{
	struct _mosquitto_packet *packet = NULL;
	int packetlen;
	int rc;

	packetlen = 2 + strlen(topic) + payloadlen;

	if (qos > 0)
	{
		packetlen += 2;    /* For message id */
	}

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));

	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	packet->mid					= mid;
	packet->command				= PUBLISH | ((dup & 0x1) << 3) | (qos << 1) | retain;
	packet->remaining_length	= packetlen;
	
	rc = _mosquitto_packet_alloc(packet);
	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header (topic string) */
	_mosquitto_write_string(packet, topic, strlen(topic));

	if (qos > 0)
	{
		_mosquitto_write_uint16(packet, mid);
	}

	/* Payload */
	if (payloadlen)
	{
		_mosquitto_write_bytes(packet, payload, payloadlen);
	}

	return _mosquitto_packet_queue(mosq, packet);
}

int _mosquitto_send_connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session)
{
	struct _mosquitto_packet	*packet = NULL;
	int							payloadlen;
	uint8_t						will = 0;
	uint8_t						byte;
	int							rc;
	uint8_t						version;
	char						*clientid, *username, *password;
	int							headerlen;

	if (dpm_mode_is_wakeup() && mosq->state == mosq_cs_connected_in_dpm)
	{
		mosq->state = mosq_cs_connected;
		mosq->unsub_topic = 0;
		return MOSQ_ERR_SUCCESS;
	}

	clientid = mosq->id;
	username = mosq->username;
	password = mosq->password;

	if (mosq->protocol == mosq_p_mqtt31)
	{
		version = MQTT_PROTOCOL_V31;
		headerlen = 12;
	}
	else if (mosq->protocol == mosq_p_mqtt311)
	{
		version = MQTT_PROTOCOL_V311;
		headerlen = 10;
	}
	else
	{
		return MOSQ_ERR_INVAL;
	}

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));

	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	payloadlen = 2 + strlen(clientid);

	if (mosq->will)
	{
		will = 1;

		payloadlen += 2 + strlen(mosq->will->topic) + 2 + mosq->will->payloadlen;
	}

	if (username)
	{
		payloadlen += 2 + strlen(username);

		if (password)
		{
			payloadlen += 2 + strlen(password);
		}
	}

	packet->command				= CONNECT;
	packet->remaining_length	= headerlen + payloadlen;

	rc = _mosquitto_packet_alloc(packet);
	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	if (version == MQTT_PROTOCOL_V31)
	{
		_mosquitto_write_string(packet, PROTOCOL_NAME_v31, strlen(PROTOCOL_NAME_v31));
	}
	else if (version == MQTT_PROTOCOL_V311)
	{
		_mosquitto_write_string(packet, PROTOCOL_NAME_v311, strlen(PROTOCOL_NAME_v311));
	}

	_mosquitto_write_byte(packet, version);
	byte = (clean_session & 0x1) << 1;

	if (will)
	{
		byte = byte | ((mosq->will->retain & 0x1) << 5) | ((mosq->will->qos & 0x3) << 3) | ((will & 0x1) << 2);
	}

	if (username)
	{
		byte = byte | 0x1 << 7;

		if (mosq->password)
		{
			byte = byte | 0x1 << 6;
		}
	}

	_mosquitto_write_byte(packet, byte);
	_mosquitto_write_uint16(packet, keepalive);

	/* Payload */
	_mosquitto_write_string(packet, clientid, strlen(clientid));

	if (will)
	{
		_mosquitto_write_string(packet, mosq->will->topic, strlen(mosq->will->topic));
		_mosquitto_write_string(packet, (const char *)mosq->will->payload, mosq->will->payloadlen);
	}

	if (username)
	{
		_mosquitto_write_string(packet, username, strlen(username));

		if (password)
		{
			_mosquitto_write_string(packet, password, strlen(password));
		}
	}

	mosq->keepalive = keepalive;
	MQTT_DBG_TEMP(" [%s] Client %s sending CONNECT\n", __func__, clientid);

	return _mosquitto_packet_queue(mosq, packet);
}

int _mosquitto_send_disconnect(struct mosquitto *mosq)
{
	MQTT_DBG_TEMP(" [%s] Client %s sending DISCONNECT\n", __func__, mosq->id);
	return _mosquitto_send_simple_command(mosq, DISCONNECT);
}

int _mosquitto_send_subscribe(struct mosquitto *mosq, int *mid, const char *topic, uint8_t topic_qos)
{
	struct _mosquitto_packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));

	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	packetlen = 2 + 2 + strlen(topic) + 1;

	packet->command				= SUBSCRIBE | (1 << 1);
	packet->remaining_length	= packetlen;

	rc = _mosquitto_packet_alloc(packet);
	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = _mosquitto_mid_generate(mosq);

	if (mid)
	{
		*mid = (int)local_mid;
	}

	_mosquitto_write_uint16(packet, local_mid);

	/* Payload */
	_mosquitto_write_string(packet, topic, strlen(topic));
	_mosquitto_write_byte(packet, topic_qos);

	mosq->in_callback = false;

	MQTT_DBG_TEMP(" [%s] Client %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d)\n",
				  __func__, mosq->id, local_mid, topic, topic_qos);

	return _mosquitto_packet_queue(mosq, packet);
}


int _mosquitto_send_unsubscribe(struct mosquitto *mosq, int *mid, const char *topic)
{
	struct _mosquitto_packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if (!packet)
	{
		return MOSQ_ERR_NOMEM;
	}

	packetlen = 2 + 2 + strlen(topic);

	packet->command				= UNSUBSCRIBE | (1 << 1);
	packet->remaining_length	= packetlen;

	rc = _mosquitto_packet_alloc(packet);
	if (rc)
	{
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = _mosquitto_mid_generate(mosq);

	if (mid)
	{
		*mid = (int)local_mid;
	}

	_mosquitto_write_uint16(packet, local_mid);

	/* Payload */
	_mosquitto_write_string(packet, topic, strlen(topic));

	MQTT_DBG_TEMP(" [%s] Client %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)\n",
				  __func__, mosq->id, local_mid, topic);

	return _mosquitto_packet_queue(mosq, packet);
}
#endif // (__SUPPORT_MQTT__)
