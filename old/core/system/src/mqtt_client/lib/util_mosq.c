/**
 *****************************************************************************************
 * @file	util_mosq.c
 * @brief	MQTT Utilities (will, time, keepalive, validation check, ...)
 * @todo	File name : util_mosq.c -> mqtt_client_util.c
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
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "mqtt3_protocol.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

int _mosquitto_packet_alloc(struct _mosquitto_packet *packet)
{
	uint8_t remaining_bytes[5], byte;
	uint32_t remaining_length;
	int i;

	remaining_length = packet->remaining_length;
	packet->payload = NULL;
	packet->remaining_count = 0;

	do
	{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;

		/* If there are more digits to encode, set the top bit of this digit */
		if (remaining_length > 0)
		{
			byte = byte | 0x80;
		}

		remaining_bytes[packet->remaining_count] = byte;
		packet->remaining_count++;
	} while (remaining_length > 0 && packet->remaining_count < 5);

	if (packet->remaining_count == 5)
	{
		return MOSQ_ERR_PAYLOAD_SIZE;
	}

	packet->packet_length	= packet->remaining_length + 1 + packet->remaining_count;
	packet->payload			= _mosquitto_malloc(sizeof(uint8_t) * packet->packet_length);

	if (!packet->payload)
	{
		return MOSQ_ERR_NOMEM;
	}

	packet->payload[0] = packet->command;

	for (i = 0; i < packet->remaining_count; i++)
	{
		packet->payload[i + 1] = remaining_bytes[i];
	}

	packet->pos = 1 + packet->remaining_count;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_check_keepalive(struct mosquitto *mosq)
{
	da16x_time_t next_msg_out;
	da16x_time_t last_msg_in;
	da16x_time_t now = mosquitto_time();

	ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
	next_msg_out	= mosq->next_msg_out;
	last_msg_in		= mosq->last_msg_in;
	xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);

	if (mosq->keepalive && (now >= next_msg_out || now - last_msg_in >= mosq->keepalive))
	{
		if (mosq->state == mosq_cs_connected && mosq->ping_t == 0)
		{
			_mosquitto_send_pingreq(mosq);
			/* Reset last msg times to give the server time to send a pingresp */
			ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
			mosq->last_msg_in	= now;
			mosq->next_msg_out	= now + mosq->keepalive;
			xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);
		}
		else
		{
			_mosquitto_socket_close(mosq);
			ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
			

			if (mosq->on_disconnect)
			{
				mosq->in_callback = true;
				mosq->on_disconnect();
				mosq->in_callback = false;
			}

			xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
		}
	}
}

uint16_t _mosquitto_mid_generate(struct mosquitto *mosq)
{
	uint16_t mid;

	if (dpm_mode_is_enabled())
	{
		extern mqttParamForRtm mqttParams;
		ut_mqtt_sem_take_recur(&(mosq->mid_mutex), portMAX_DELAY);
		mosq->last_mid = ++mqttParams.pub_msg_id;

		if (mosq->last_mid == 0)
		{
			mqttParams.pub_msg_id = ++mosq->last_mid;
		}

		mid = mosq->last_mid;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		mq_msg_tbl_presvd_update_last_mid(mid);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		xSemaphoreGiveRecursive(mosq->mid_mutex.mutex_info);
	}
	else
	{
		ut_mqtt_sem_take_recur(&(mosq->mid_mutex), portMAX_DELAY);
		mosq->last_mid++;

		if (mosq->last_mid == 0)
		{
			mosq->last_mid++;
		}

		mid = mosq->last_mid;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		mq_msg_tbl_presvd_update_last_mid(mid);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		xSemaphoreGiveRecursive(mosq->mid_mutex.mutex_info);
	}

	return mid;
}

/* Check that a topic used for publishing is valid.
 * Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_pub_topic_check(const char *str)
{
	int len = 0;

	while (str && str[0])
	{
		if (str[0] == '+' || str[0] == '#')
		{
			return MOSQ_ERR_INVAL;
		}

		len++;
		str = &str[1];
	}

	if (len > 65535)
	{
		return MOSQ_ERR_INVAL;
	}

	return MOSQ_ERR_SUCCESS;
}

/* Check that a topic used for subscriptions is valid.
 * Search for + or # in a topic, check they aren't in invalid positions such as
 * foo/#/bar, foo/+bar or foo/bar#.
 * Return MOSQ_ERR_INVAL if invalid position found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_sub_topic_check(const char *str)
{
	char c = '\0';
	int len = 0;

	while (str && str[0])
	{
		if (str[0] == '+')
		{
			if ((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/'))
			{
				return MOSQ_ERR_INVAL;
			}
		}
		else if (str[0] == '#')
		{
			if ((c != '\0' && c != '/')  || str[1] != '\0')
			{
				return MOSQ_ERR_INVAL;
			}
		}

		len++;
		c = str[0];
		str = &str[1];
	}

	if (len > 65535)
	{
		return MOSQ_ERR_INVAL;
	}

	return MOSQ_ERR_SUCCESS;
}

/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	int slen, tlen;
	int spos, tpos;
	bool multilevel_wildcard = false;

	if (!sub || !topic || !result)
	{
		return MOSQ_ERR_INVAL;
	}

	slen = strlen(sub);
	tlen = strlen(topic);

	if (!slen || !tlen)
	{
		*result = false;
		return MOSQ_ERR_INVAL;
	}

	if (slen && tlen)
	{
		if ((sub[0] == '$' && topic[0] != '$') || (topic[0] == '$' && sub[0] != '$'))
		{
			*result = false;
			return MOSQ_ERR_SUCCESS;
		}
	}

	spos = 0;
	tpos = 0;

	while (spos < slen && tpos <= tlen)
	{
		if (sub[spos] == topic[tpos])
		{
			if (tpos == tlen - 1)
			{
				/* Check for e.g. foo matching foo/# */
				if (spos == slen - 3
						&& sub[spos + 1] == '/'
						&& sub[spos + 2] == '#')
				{
					*result = true;
					multilevel_wildcard = true;
					return MOSQ_ERR_SUCCESS;
				}
			}

			spos++;
			tpos++;

			if (spos == slen && tpos == tlen)
			{
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}
			else if (tpos == tlen && spos == slen - 1 && sub[spos] == '+')
			{
				if (spos > 0 && sub[spos - 1] != '/')
				{
					*result = false;
					return MOSQ_ERR_INVAL;
				}

				spos++;
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}
		}
		else
		{
			if (sub[spos] == '+')
			{
				/* Check for bad "+foo" or "a/+foo" subscription */
				if (spos > 0 && sub[spos - 1] != '/')
				{
					*result = false;
					return MOSQ_ERR_INVAL;
				}

				/* Check for bad "foo+" or "foo+/a" subscription */
				if (spos < slen - 1 && sub[spos + 1] != '/')
				{
					*result = false;
					return MOSQ_ERR_INVAL;
				}

				spos++;

				while (tpos < tlen && topic[tpos] != '/')
				{
					tpos++;
				}

				if (tpos == tlen && spos == slen)
				{
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}
			else if (sub[spos] == '#')
			{
				if (spos > 0 && sub[spos - 1] != '/')
				{
					*result = false;
					return MOSQ_ERR_INVAL;
				}

				multilevel_wildcard = true;

				if (spos + 1 != slen)
				{
					*result = false;
					return MOSQ_ERR_INVAL;
				}
				else
				{
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}
			else
			{
				*result = false;
				return MOSQ_ERR_SUCCESS;
			}
		}
	}

	if (multilevel_wildcard == false && (tpos < tlen || spos < slen))
	{
		*result = false;
	}

	return MOSQ_ERR_SUCCESS;
}

#if defined (REAL_WITH_TLS_PSK)
int _mosquitto_hex2bin(const char *hex, unsigned char *bin, int bin_max_len)
{
	BIGNUM *bn = NULL;
	int len;

	if (BN_hex2bn(&bn, hex) == 0)
	{
		if (bn)
		{
			BN_free(bn);
		}

		return 0;
	}

	if (BN_num_bytes(bn) > bin_max_len)
	{
		BN_free(bn);
		return 0;
	}

	len = BN_bn2bin(bn, bin);
	BN_free(bn);
	return len;
}
#endif // (REAL_WITH_TLS_PSK)

da16x_time_t mosquitto_time(void)
{
#if defined (__TIME64__)
	__time64_t uptime = 0;
	__uptime(&uptime);
	return uptime;
#else
	return __uptime();
#endif // (__TIME64__)
}

int _mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen,
						const void *payload, int qos, bool retain)
{
	int rc = MOSQ_ERR_SUCCESS;

	if (!mosq || !topic)
	{
		return MOSQ_ERR_INVAL;
	}

	if (payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD)
	{
		return MOSQ_ERR_PAYLOAD_SIZE;
	}

	if (payloadlen > 0 && !payload)
	{
		return MOSQ_ERR_INVAL;
	}

	if (mosquitto_pub_topic_check(topic))
	{
		return MOSQ_ERR_INVAL;
	}

	if (mosq->will)
	{
		if (mosq->will->topic)
		{
			_mosquitto_free(mosq->will->topic);
			mosq->will->topic = NULL;
		}

		if (mosq->will->payload)
		{
			_mosquitto_free(mosq->will->payload);
			mosq->will->payload = NULL;
		}

		_mosquitto_free(mosq->will);
		mosq->will = NULL;
	}

	mosq->will = _mosquitto_calloc(1, sizeof(struct mosquitto_message));

	if (!mosq->will)
	{
		return MOSQ_ERR_NOMEM;
	}

	mosq->will->topic = _mosquitto_strdup(topic);

	if (!mosq->will->topic)
	{
		rc = MOSQ_ERR_NOMEM;
		goto cleanup;
	}

	mosq->will->payloadlen = payloadlen;

	if (mosq->will->payloadlen > 0)
	{
		if (!payload)
		{
			rc = MOSQ_ERR_INVAL;
			goto cleanup;
		}

		mosq->will->payload = _mosquitto_malloc(sizeof(char) * mosq->will->payloadlen);

		if (!mosq->will->payload)
		{
			rc = MOSQ_ERR_NOMEM;
			goto cleanup;
		}

		memcpy(mosq->will->payload, payload, payloadlen);
	}

	mosq->will->qos = qos;
	mosq->will->retain = retain;

	return MOSQ_ERR_SUCCESS;

cleanup:
	if (mosq->will)
	{
		if (mosq->will->topic)
		{
			_mosquitto_free(mosq->will->topic);
		}

		if (mosq->will->payload)
		{
			_mosquitto_free(mosq->will->payload);
		}
	}

	_mosquitto_free(mosq->will);
	mosq->will = NULL;

	return rc;
}

int _mosquitto_will_clear(struct mosquitto *mosq)
{
	if (!mosq->will)
	{
		return MOSQ_ERR_SUCCESS;
	}

	if (mosq->will->topic)
	{
		_mosquitto_free(mosq->will->topic);
		mosq->will->topic = NULL;
	}

	if (mosq->will->payload)
	{
		_mosquitto_free(mosq->will->payload);
		mosq->will->payload = NULL;
	}

	_mosquitto_free(mosq->will);
	mosq->will = NULL;

	return MOSQ_ERR_SUCCESS;
}
#endif // (__SUPPORT_MQTT__)
