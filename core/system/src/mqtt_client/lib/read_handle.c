/**
 *****************************************************************************************
 * @file	read_handle.c
 * @brief	Procssing MQTT messages received
 * @todo	File name : read_handle.c -> mqtt_client_read.c
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
#include "messages_mosq.h"
#include "mqtt3_protocol.h"
#include "net_mosq.h"
#include "read_handle.h"
#include "send_mosq.h"
#include "util_mosq.h"
#include "atcmd.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"

extern void mqtt_connect_ind_sent_set(int val);
extern int mqtt_connect_ind_is_sent(void);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

extern mqttParamForRtm mqttParams;
extern void _mosquitto_set_broker_conn_state(UINT8 state);
extern void atcmd_asynchony_event_for_mqttmsgpub(int result, int err_code);
extern void mqtt_client_unsub_status_set(int status);

int _mosquitto_packet_handle(struct mosquitto *mosq)
{
	switch ((mosq->in_packet.command) & 0xF0)
	{
		case PINGREQ:
		{
			return _mosquitto_handle_pingreq(mosq);
		}

		case PINGRESP:
		{
			return _mosquitto_handle_pingresp(mosq);
		}

		case PUBACK:
		{
#if defined (__MQTT_EMUL_CMD__)
			if (mqttParams.q1_tx_discard_puback_rx)
				return MOSQ_ERR_SUCCESS;
#endif // __MQTT_EMUL_CMD__

			return _mosquitto_handle_pubackcomp(mosq, "PUBACK");
		}

		case PUBCOMP:
		{
			return _mosquitto_handle_pubackcomp(mosq, "PUBCOMP");
		}

		case PUBLISH:
		{
			return _mosquitto_handle_publish(mosq);
		}

		case PUBREC:
		{
#if defined (__MQTT_EMUL_CMD__)
			if (mqttParams.q2_tx_discard_pubrec_rx)
				return MOSQ_ERR_SUCCESS;
#endif // __MQTT_EMUL_CMD__

			return _mosquitto_handle_pubrec(mosq);
		}

		case PUBREL:
		{
#if defined (__MQTT_EMUL_CMD__)
			if (mqttParams.q2_rx_discard_pubrel_rx)
				return MOSQ_ERR_SUCCESS;
#endif // __MQTT_EMUL_CMD__

			return _mosquitto_handle_pubrel(mosq);
		}

		case CONNACK:
		{
			return _mosquitto_handle_connack(mosq);
		}

		case SUBACK:
		{
			return _mosquitto_handle_suback(mosq);
		}

		case UNSUBACK:
		{
			return _mosquitto_handle_unsuback(mosq);
		}

		default:
		{
			MQTT_DBG_ERR(RED_COLOR "ERR: Unrecognised cmd %d\n" CLEAR_COLOR,
						 (mosq->in_packet.command) & 0xF0);
			return MOSQ_ERR_PROTOCOL;
		}
	}
}

int _mosquitto_handle_publish(struct mosquitto *mosq)
{
	uint8_t header;
	struct mosquitto_message_all *message;
	int rc = 0;
	uint16_t mid;

	message = _mosquitto_calloc(1, sizeof(struct mosquitto_message_all));

	if (!message)
	{
		return MOSQ_ERR_NOMEM;
	}

	header = mosq->in_packet.command;

	message->dup		= (header & 0x08) >> 3;
	message->msg.qos	= (header & 0x06) >> 1;
	message->msg.retain	= (header & 0x01);

	rc = _mosquitto_read_string(&mosq->in_packet, &message->msg.topic);
	if (rc)
	{
		_mosquitto_message_cleanup(&message);
		return rc;
	}

	if (!strlen(message->msg.topic))
	{
		_mosquitto_message_cleanup(&message);
		return MOSQ_ERR_PROTOCOL;
	}

	if (message->msg.qos > 0)
	{
		rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);

		if (rc)
		{
			_mosquitto_message_cleanup(&message);
			return rc;
		}

		message->msg.mid = (int)mid;
	}

	message->msg.payloadlen = (int)(mosq->in_packet.remaining_length - mosq->in_packet.pos);

	if (message->msg.payloadlen <= MQTT_MSG_MAX_LEN)
	{
		message->msg.payload = (void *)_mosquitto_calloc(message->msg.payloadlen + 1, sizeof(uint8_t));

		if (!message->msg.payload)
		{
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_NOMEM;
		}

		rc = (int)_mosquitto_read_bytes(&mosq->in_packet, message->msg.payload, message->msg.payloadlen);
		if (rc)
		{
			_mosquitto_message_cleanup(&message);
			return rc;
		}
	}
	else
	{
		MQTT_DBG_ERR("Client %s: Payload is too long(%ld bytes))\n" CLEAR_COLOR,
					 mosq->id, message->msg.payloadlen);
#if defined (MQTT_PROC_LONG_SIZE)
		goto process_mqtt;
#else
		_mosquitto_message_cleanup(&message);
		return MOSQ_ERR_PAYLOAD_SIZE;
#endif // (MQTT_PROC_LONG_SIZE)
	}
	
	MQTT_DBG_PRINT("(Rx: Len=%d,Topic=%s,Msg_ID=%d)\n",
				   message->msg.payloadlen, 
				   message->msg.topic, 
				   message->msg.mid);

	MQTT_DBG_TEMP(" [%s] Client %s received PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))\n",
				  __func__, mosq->id, message->dup, message->msg.qos, message->msg.retain,
				  message->msg.mid, message->msg.topic, (long)message->msg.payloadlen);

#if defined (MQTT_PROC_LONG_SIZE)
process_mqtt:
#endif // (MQTT_PROC_LONG_SIZE)
	message->timestamp = mosquitto_time();

	switch (message->msg.qos)
	{
		case 0:
		{
			ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
			if (mosq->on_message)
			{
				mosq->in_callback = true;
				mosq->on_message(message->msg.payload, message->msg.payloadlen, message->msg.topic);
				mosq->in_callback = false;
			}
			xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);

			atcmd_asynchony_event_for_mqttmsgrecv(0, (char *)message->msg.payload, 
				(char *)message->msg.topic, message->msg.payloadlen);
			
			_mosquitto_message_cleanup(&message);			

			mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
			rc = MOSQ_ERR_SUCCESS;
		} break;

		case 1:
		{
			rc = _mosquitto_send_puback(mosq, (uint16_t)(message->msg.mid));
			ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
			if (mosq->on_message)
			{
				mosq->in_callback = true;
				mosq->on_message(message->msg.payload, message->msg.payloadlen, message->msg.topic);
				mosq->in_callback = false;
			}
			xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
			
			atcmd_asynchony_event_for_mqttmsgrecv(0, (char *)message->msg.payload, 
				(char *)message->msg.topic, message->msg.payloadlen);

			_mosquitto_message_cleanup(&message);
			
			mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
		} break;

		case 2:
		{
#if defined (__MQTT_EMUL_CMD__)		
			if (!mqttParams.q2_rx_no_pubrec_tx) {
				rc = _mosquitto_send_pubrec(mosq, (uint16_t)(message->msg.mid));
			} else {
				PRINTF("mq_emul: PUBREC(mid=%d) not sent \n", message->msg.mid);
			}
#else			
			rc = _mosquitto_send_pubrec(mosq, (uint16_t)(message->msg.mid));
#endif // __MQTT_EMUL_CMD__
			ut_mqtt_sem_take_recur(&(mosq->in_message_mutex), portMAX_DELAY);
			message->state = mosq_ms_wait_for_pubrel;
			mosq->q2_proc = 1;
			if (_mosquitto_message_search_mid_in_rx_queue(mosq, (uint16_t)(message->msg.mid)) == MOSQ_ERR_SUCCESS) {
				// an old one exists, remove it and add a new one
				struct mosquitto_message_all *old_msg = NULL;
				
				_mosquitto_message_remove(mosq, (uint16_t)(message->msg.mid), mosq_md_in, &old_msg);
				_mosquitto_message_cleanup(&old_msg);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
				mq_msg_tbl_presvd_del(message, mosq_md_in);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
			}
			
			_mosquitto_message_queue(mosq, message, mosq_md_in);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
			mq_msg_tbl_presvd_add(message, mosq_md_in);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

			xSemaphoreGiveRecursive(mosq->in_message_mutex.mutex_info);
		} break;

		default:
		{
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_PROTOCOL;
		}
	}

	return rc;
}

int _mosquitto_handle_connack(struct mosquitto *mosq)
{
	uint8_t	byte;
	uint8_t	result;
	int		rc;

	MQTT_DBG_TEMP(" [%s] Client %s received CONNACK\n", __func__, mosq->id);

	rc = _mosquitto_read_byte(&mosq->in_packet, &byte);
	if (rc)
	{
		return rc;
	}

	rc = _mosquitto_read_byte(&mosq->in_packet, &result);
	if (rc)
	{
		return rc;
	}

	if (mosq->mqtt_pub_or_sub == CLIENT_SUB)
	{
		mqtt_sub_on_connect(mosq, mosq->userdata, result);
	}

	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);

	if (mosq->on_connect)
	{
		mosq->in_callback = true;
		mosq->on_connect();
		mosq->in_callback = false;
	}

	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);	

	if (!result && mosq->mqtt_pub_or_sub == CLIENT_SUB)
	{
		if (mosq->unsub_topic == 0)
		{
			extern mqttParamForRtm mqttParams;
			MQTT_DBG_PRINT(CYAN_COLOR ">>> MQTT_Sub connection OK (%s)\n" CLEAR_COLOR, mosq->id);
			atcmd_asynchony_event(4, "1");
			mqttParams.sub_connect_retry_count = 0;

			_mosquitto_set_broker_conn_state(TRUE);

			if (dpm_mode_is_enabled())
			{
				mosquitto_pingreq_timer_register(mosq);
                mqtt_client_dpm_port_filter_set();
			}

			mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		} else {
			if (mq_msg_tbl_presvd_is_enabled() && 
				(mosq->inflight_messages > 0 || mosq->in_messages != NULL)) {

				MQTT_DBG_PRINT(CYAN_COLOR ">>> MQTT Client connection OK (%s) \n" CLEAR_COLOR, mosq->id);
				mqtt_connect_ind_sent_set(TRUE);
				
				atcmd_asynchony_event(4, "1");
			}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		}
	}

	switch (result)
	{
		case 0:
		{
			if (mosq->state != mosq_cs_disconnecting)
			{
				mosq->state = mosq_cs_connected;
			}
			return MOSQ_ERR_SUCCESS;
		}

		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		{
			return MOSQ_ERR_CONN_REFUSED;
		}

		default:
		{
			return MOSQ_ERR_PROTOCOL;
		}
	}
}

int _mosquitto_handle_pingreq(struct mosquitto *mosq)
{
	MQTT_DBG_PRINT("Client %s rx PINGREQ\n", mosq->id);

	return _mosquitto_send_pingresp(mosq);
}

int _mosquitto_handle_pingresp(struct mosquitto *mosq)
{
	mosq->ping_t = 0; /* No longer waiting for a PINGRESP. */
	MQTT_DBG_PRINT("Client %s rx PINGRESP\n", mosq->id);

	if (dpm_mode_is_enabled())
	{
		subPingStatus = PING_STS_RECEIVED;
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_pubackcomp(struct mosquitto *mosq, const char *type)
{
	uint16_t mid;
	int rc;

	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if (rc)
	{
		return rc;
	}
	MQTT_DBG_PRINT("[%s] (Rx, Msg_ID=%d)\n", type, mid);
	MQTT_DBG_TEMP(" [%s] Client %s rx %s (Mid: %d)\n", __func__, mosq->id, type, mid);

	if (!_mosquitto_message_delete(mosq, mid, mosq_md_out))
	{
		/* Only inform the client the message has been sent once. */
		ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);

		if (mosq->on_publish)
		{
			mosq->in_callback = true;
			mosq->on_publish(mid);
			mosq->in_callback = false;
		}
		atcmd_asynchony_event_for_mqttmsgpub(1, 0);	
		xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);

	}

	if (mosq->q2_proc != 1) {
		mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_pubrec(struct mosquitto *mosq)
{
	uint16_t mid;
	int rc;

	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if (rc)
	{
		return rc;
	}

	MQTT_DBG_PRINT("[%s] (Rx: Msg_ID=%d)\n", "PUBREC", mid);
	MQTT_DBG_TEMP("[%s] Client %s received PUBREC (Mid: %d)\n", __func__, mosq->id, mid);

	rc = _mosquitto_message_out_update(mosq, mid, mosq_ms_wait_for_pubcomp);

	if (rc == MOSQ_ERR_NOT_FOUND)
	{
		MQTT_DBG_ERR(RED_COLOR "Warn: Rx PUBREC from %s for an unknown pkt identifier %d.\n" CLEAR_COLOR,  mosq->id, mid);
	}
	else if (rc != MOSQ_ERR_SUCCESS)
	{
		return rc;
	}
#if defined (__MQTT_EMUL_CMD__)
	if (!mqttParams.q2_tx_no_pubrel_tx) {
		rc = _mosquitto_send_pubrel(mosq, mid);
	} else {
		PRINTF("mq_emul: PUBREL(mid=%d) not sent \n", mid);
	}
#else
	rc = _mosquitto_send_pubrel(mosq, mid);
#endif // __MQTT_EMUL_CMD__

	if (rc)
	{
		return rc;
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_pubrel(struct mosquitto *mosq)
{
	struct mosquitto_message_all *message = NULL;
	uint16_t	mid;
	int			rc;

	if (mosq->protocol == mosq_p_mqtt311)
	{
		if ((mosq->in_packet.command & 0x0F) != 0x02)
		{
			return MOSQ_ERR_PROTOCOL;
		}
	}

	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if (rc)
	{
		return rc;
	}

	MQTT_DBG_PRINT("[%s] (Rx: Msg_ID=%d)\n", "PUBREL", mid);
	MQTT_DBG_TEMP("[%s] Client %s received PUBREL (Mid: %d)\n", __func__, mosq->id, mid);

#if defined (__MQTT_EMUL_CMD__)
	if (!mqttParams.q2_rx_no_pubcomp_tx) {
		rc = _mosquitto_send_pubcomp(mosq, mid);
	} else {
		PRINTF("mq_emul: PUBCOMP(mid=%d) not sent \n", mid);
	}
#else
	rc = _mosquitto_send_pubcomp(mosq, mid);
#endif // __MQTT_EMUL_CMD__
	if (!_mosquitto_message_remove(mosq, mid, mosq_md_in, &message))
	{
		/* Only pass the message on if we have removed it from the queue - this
		 * prevents multiple callbacks for the same message. */
		 
		ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);

		if (mosq->on_message)
		{
			//mosq->in_callback = true; (by Shingu)
			mosq->on_message(message->msg.payload, message->msg.payloadlen, message->msg.topic);
			mosq->in_callback = false;
		}

		xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
		atcmd_asynchony_event_for_mqttmsgrecv(0, (char *)message->msg.payload, 
									(char *)message->msg.topic, message->msg.payloadlen);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		mq_msg_tbl_presvd_del(message, mosq_md_in);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		
		_mosquitto_message_cleanup(&message);
	}
	
	mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);

	mosq->q2_proc = 0;

	if (rc)
	{
		return rc;
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_suback(struct mosquitto *mosq)
{
	uint16_t	mid;
	uint8_t		qos;
	int			*granted_qos;
	int			qos_count;
	int			i = 0;
	int			rc;

	extern mqttParamForRtm mqttParams;
	extern int subRetryCount;

	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);

	if (rc)
	{
		return rc;
	}

	qos_count	= (int)(mosq->in_packet.remaining_length - mosq->in_packet.pos);
	granted_qos	= (int *)_mosquitto_malloc((size_t)(qos_count * sizeof(int)));

	if (!granted_qos)
	{
		return MOSQ_ERR_NOMEM;
	}

	while (mosq->in_packet.pos < mosq->in_packet.remaining_length)
	{
		rc = _mosquitto_read_byte(&mosq->in_packet, &qos);
		if (rc)
		{
			_mosquitto_free(granted_qos);
			return rc;
		}

		granted_qos[i] = (int)qos;
		i++;
	}

	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	if (mosq->on_subscribe)
	{
		mosq->in_callback = true;
		mosq->on_subscribe();
		mosq->in_callback = false;
	}
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);

	_mosquitto_free(granted_qos);

	if (--mosq->unsub_topic == 0)
	{
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		if (!mqtt_connect_ind_is_sent())
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
			MQTT_DBG_PRINT(CYAN_COLOR ">>> MQTT Client connection OK (%s) \n" CLEAR_COLOR, mosq->id);
		
		atcmd_asynchony_event(4, "1");
		
		subRetryCount = 0;
		mqttParams.sub_connect_retry_count = 0;

		_mosquitto_set_broker_conn_state(TRUE);

        if (dpm_mode_is_enabled())
        {
            mosquitto_pingreq_timer_register(mosq);
            mqtt_client_dpm_port_filter_set();
        }

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		if (mq_msg_tbl_presvd_is_enabled() && 
			(mosq->inflight_messages > 0 || mosq->in_messages != NULL)) {} else
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
		{mqtt_client_dpm_sleep(APP_MQTT_SUB, __func__, __LINE__);}

	}
	
	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_unsuback(struct mosquitto *mosq)
{
	uint16_t mid;
	int rc;

	MQTT_DBG_TEMP(" [%s] Client %s received UNSUBACK\n", __func__, mosq->id);
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if (rc)
	{
		mqtt_client_unsub_status_set(-1); // UNSUBACK recv, but MOSQ_ERR_PROTOCOL
		return rc;
	}

	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	if (mosq->on_unsubscribe)
	{
		mosq->in_callback = true;
		mosq->on_unsubscribe();
		mosq->in_callback = false;
	}
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);

	mqtt_client_unsub_status_set(0); // success
	return MOSQ_ERR_SUCCESS;
}
#endif // (__SUPPORT_MQTT__)
