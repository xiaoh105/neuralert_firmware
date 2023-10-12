/**
 *****************************************************************************************
 * @file	mosquitto.c
 * @brief	MQTT module task
 * @todo	File name : mosquitto.c -> mqtt_client_task.c
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
#include "messages_mosq.h"
#include "memory_mosq.h"
#include "mqtt3_protocol.h"
#include "net_mosq.h"
#include "read_handle.h"
#include "send_mosq.h"
#include "util_mosq.h"
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
#include "mqtt_msg_tbl_presvd.h"
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#include "lwip/sockets.h"
#include "user_dpm_api.h"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

extern	UINT	runContinueSub;
extern	mqttParamForRtm mqttParams;

void		_mosquitto_destroy(struct mosquitto *mosq);
static int	_mosquitto_reconnect(struct mosquitto *mosq, bool blocking);
static int	_mosquitto_connect_init(struct mosquitto *mosq, const char *host, int port,
									int keepalive, const char *bind_address);

extern void ut_sock_convert2tv(struct timeval* tv, ULONG ultime);
extern UINT8 ut_is_task_running(TaskStatus_t* task_status);
extern int da16x_network_main_check_net_link_status(int iface);

#if defined (MBEDTLS_DEBUG_C)
#define	MOSQ_SUB_STACK_SIZE	(512*3)
#else
#define	MOSQ_SUB_STACK_SIZE	1024
#endif // MBEDTLS_DEBUG_C

TaskHandle_t	mosquittoPingSubThread = NULL;

char	mqtt_conn_state(void)
{
#if defined ( __MQTT_CONN_STATUS__ )
	return	mqttParams.broker_conn_done;
#else
	return	0;
#endif	// __MQTT_CONN_STATUS__
}

struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *userdata)
{
	struct mosquitto *mosq = NULL;
	int rc;

	if (clean_session == false && id == NULL) {
		return NULL;
	}

	mosq = (struct mosquitto *)_mosquitto_calloc(1, sizeof(struct mosquitto));
	if (mosq) {
		rc = mosquitto_reinitialise(mosq, id, clean_session, userdata);
		if (rc) {
			mosquitto_destroy(mosq);
			return NULL;
		}
	}

	return mosq;
}

int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_session, void *userdata)
{
	int i;

	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (clean_session == false && id == NULL) {
		return MOSQ_ERR_INVAL;
	}

	memset(mosq, 0, sizeof(struct mosquitto));

	if (userdata) {
		mosq->userdata = userdata;
	} else {
		mosq->userdata = mosq;
	}

	mosq->protocol			= mosq_p_mqtt31;
	mosq->keepalive			= 60;
	mosq->message_retry		= 20;
	mosq->last_retry_check	= 0;
	mosq->clean_session		= clean_session;

	if (id) {
		if (STREMPTY(id)) {
			return MOSQ_ERR_INVAL;
		}

		mosq->id = _mosquitto_strdup(id);
	} else {
		mosq->id = (char *)_mosquitto_calloc(24, sizeof(char));
		if (!mosq->id) {
			return MOSQ_ERR_NOMEM;
		}

		mosq->id[0] = 'm';
		mosq->id[1] = 'o';
		mosq->id[2] = 's';
		mosq->id[3] = 'q';
		mosq->id[4] = '/';

		for (i = 5; i < 23; i++) {
			mosq->id[i] = (char)((rand() % 73) + 48);
		}
	}

	_mosquitto_packet_cleanup(&mosq->in_packet);

	mosq->out_packet					= NULL;
	mosq->current_out_packet			= NULL;
	mosq->last_msg_in					= mosquitto_time();
	mosq->next_msg_out					= mosquitto_time() + mosq->keepalive;
	mosq->ping_t						= 0;
	mosq->last_mid						= 0;
	mosq->state							= mosq_cs_new;
	mosq->in_messages					= NULL;
	mosq->in_messages_last				= NULL;
	mosq->out_messages					= NULL;
	mosq->out_messages_last				= NULL;
	mosq->max_inflight_messages			= 20;
	mosq->will							= NULL;
	mosq->on_connect					= NULL;
	mosq->on_disconnect					= NULL;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	mosq->on_disconnect2				= NULL;
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	mosq->on_publish					= NULL;
	mosq->on_message					= NULL;
	mosq->on_subscribe					= NULL;
	mosq->on_unsubscribe				= NULL;
	mosq->host							= NULL;
	mosq->port							= 1883;
	mosq->in_callback					= false;
	mosq->in_queue_len					= 0;
	mosq->out_queue_len					= 0;
	mosq->reconnect_delay				= 1;
	mosq->reconnect_delay_max			= 1;
	mosq->reconnect_exponential_backoff	= false;
	mosq->threaded						= mosq_ts_none;

	mosq->ssl_ctx						= NULL;
	mosq->ssl_conf						= NULL;
	mosq->ctr_drbg_ctx					= NULL;
	mosq->entropy_ctx					= NULL;
	mosq->ca_cert_crt					= NULL;
	mosq->cert_crt						= NULL;
	mosq->pkey_ctx						= NULL;
	mosq->pkey_alt_ctx					= NULL;

	mosq->tls_ca_cert					= NULL;
	mosq->tls_ca_cert_len				= 0;
	mosq->tls_cert						= NULL;
	mosq->tls_cert_len					= 0;
	mosq->tls_private_key				= NULL;
	mosq->tls_private_key_len			= 0;
	mosq->tls_dh_param	    			= NULL;
	mosq->tls_dh_param_len	    		= 0;
	mosq->tls_time_check				= mqttParams.tls_time_check_option;
	mosq->tls_auth_mode					= mqttParams.tls_authmode;
	mosq->tls_incoming_size				= mqttParams.tls_incoming_size;
	mosq->tls_outgoing_size				= mqttParams.tls_outgoing_size;

	mosq->socket_timeout				= 0;
	mosq->sock							= INVALID_SOCKET;

	ut_mqtt_sem_create(&(mosq->callback_mutex));
	ut_mqtt_sem_create(&(mosq->log_callback_mutex));
	ut_mqtt_sem_create(&(mosq->msgtime_mutex));
	ut_mqtt_sem_create(&(mosq->out_packet_mutex));
	ut_mqtt_sem_create(&(mosq->current_out_packet_mutex));
	ut_mqtt_sem_create(&(mosq->state_mutex));
	ut_mqtt_sem_create(&(mosq->in_message_mutex));
	ut_mqtt_sem_create(&(mosq->out_message_mutex));
	ut_mqtt_sem_create(&(mosq->mid_mutex));
	ut_mqtt_sem_create(&(mosq->send_msg_mutex));

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen,
					   const void *payload, int qos, bool retain)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	return _mosquitto_will_set(mosq, topic, payloadlen, payload, qos, retain);
}

int mosquitto_will_clear(struct mosquitto *mosq)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	return _mosquitto_will_clear(mosq);
}

int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username, const char *password)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (mosq->username) {
		_mosquitto_free(mosq->username);
		mosq->username = NULL;
	}

	if (mosq->password) {
		_mosquitto_free(mosq->password);
		mosq->password = NULL;
	}

	if (username) {
		mosq->username = _mosquitto_strdup(username);

		if (!mosq->username) {
			return MOSQ_ERR_NOMEM;
		}

		if (password) {
			mosq->password = _mosquitto_strdup(password);

			if (!mosq->password) {
				_mosquitto_free(mosq->username);
				mosq->username = NULL;
				return MOSQ_ERR_NOMEM;
			}
		}
	}

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_destroy(struct mosquitto *mosq)
{
	struct _mosquitto_packet *packet;

	if (!mosq) {
		return;
	}

	_mosquitto_socket_close(mosq);
	_mosquitto_message_cleanup_all(mosq);
	_mosquitto_will_clear(mosq);

	if (mosq->address) {
		_mosquitto_free(mosq->address);
		mosq->address = NULL;
	}

	if (mosq->id) {
		_mosquitto_free(mosq->id);
		mosq->id = NULL;
	}

	if (mosq->username) {
		_mosquitto_free(mosq->username);
		mosq->username = NULL;
	}

	if (mosq->password) {
		_mosquitto_free(mosq->password);
		mosq->password = NULL;
	}

	if (mosq->host) {
		_mosquitto_free(mosq->host);
		mosq->host = NULL;
	}

	if (mosq->bind_address) {
		_mosquitto_free(mosq->bind_address);
		mosq->bind_address = NULL;
	}

	/* Out packet cleanup */
	if (mosq->out_packet && !mosq->current_out_packet) {
		mosq->current_out_packet	= mosq->out_packet;
		mosq->out_packet			= mosq->out_packet->next;
	}

	while (mosq->current_out_packet) {
		packet = mosq->current_out_packet;

		/* Free data and reset values */
		mosq->current_out_packet = mosq->out_packet;

		if (mosq->out_packet) {
			mosq->out_packet = mosq->out_packet->next;
		}

		_mosquitto_packet_cleanup(packet);
		_mosquitto_free(packet);
	}

	_mosquitto_packet_cleanup(&mosq->in_packet);

	ut_mqtt_sem_delete(&(mosq->callback_mutex));
	ut_mqtt_sem_delete(&(mosq->log_callback_mutex));
	ut_mqtt_sem_delete(&(mosq->msgtime_mutex));
	ut_mqtt_sem_delete(&(mosq->out_packet_mutex));
	ut_mqtt_sem_delete(&(mosq->current_out_packet_mutex));
	ut_mqtt_sem_delete(&(mosq->state_mutex));
	ut_mqtt_sem_delete(&(mosq->in_message_mutex));
	ut_mqtt_sem_delete(&(mosq->out_message_mutex));
	ut_mqtt_sem_delete(&(mosq->mid_mutex));
	ut_mqtt_sem_delete(&(mosq->send_msg_mutex));
}

void mosquitto_destroy(struct mosquitto *mosq)
{
	if (!mosq) {
		return;
	}

	_mosquitto_destroy(mosq);
	_mosquitto_free(mosq);
}

static int _mosquitto_connect_init(struct mosquitto *mosq, const char *host, int port,
								   int keepalive, const char *bind_address)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (!host || port <= 0) {
		return MOSQ_ERR_INVAL;
	}

	if (mosq->host) {
		_mosquitto_free(mosq->host);
	}

	mosq->host = _mosquitto_strdup(host);

	if (!mosq->host) {
		return MOSQ_ERR_NOMEM;
	}

	mosq->port = port;

	if (mosq->bind_address) {
		_mosquitto_free(mosq->bind_address);
	}

	if (bind_address) {
		mosq->bind_address = _mosquitto_strdup(bind_address);

		if (!mosq->bind_address) {
			return MOSQ_ERR_NOMEM;
		}
	}

	mosq->keepalive = keepalive;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
	return mosquitto_connect_bind(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	int rc;

	rc = _mosquitto_connect_init(mosq, host, port, keepalive, bind_address);

	if (rc) {
		return rc;
	}

	ut_mqtt_sem_take_recur(&(mosq->state_mutex), portMAX_DELAY);
	mosq->state = mosq_cs_new;
	xSemaphoreGiveRecursive(mosq->state_mutex.mutex_info);

	return _mosquitto_reconnect(mosq, true);
}

int mosquitto_connect_async(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
	return mosquitto_connect_bind_async(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host, int port,
								 int keepalive, const char *bind_address)
{
	int rc = _mosquitto_connect_init(mosq, host, port, keepalive, bind_address);

	if (rc) {
		return rc;
	}

	ut_mqtt_sem_take_recur(&(mosq->state_mutex), portMAX_DELAY);	
	mosq->state = mosq_cs_connect_async;
	xSemaphoreGiveRecursive(mosq->state_mutex.mutex_info);

	return _mosquitto_reconnect(mosq, false);
}

int mosquitto_reconnect_async(struct mosquitto *mosq)
{
	return _mosquitto_reconnect(mosq, false);
}

int mosquitto_reconnect(struct mosquitto *mosq)
{
	return _mosquitto_reconnect(mosq, true);
}

static int _mosquitto_reconnect(struct mosquitto *mosq, bool blocking)
{
	DA16X_UNUSED_ARG(blocking);

	int rc;
	struct _mosquitto_packet *packet;

	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (!mosq->host || mosq->port <= 0) {
		return MOSQ_ERR_INVAL;
	}
	
	ut_mqtt_sem_take_recur(&(mosq->state_mutex), portMAX_DELAY);
	mosq->state = mosq_cs_new;
	xSemaphoreGiveRecursive(mosq->state_mutex.mutex_info);

	ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
	mosq->last_msg_in = mosquitto_time();
	mosq->next_msg_out = mosq->last_msg_in + mosq->keepalive;
	xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);

	mosq->ping_t = 0;

	_mosquitto_packet_cleanup(&mosq->in_packet);

	ut_mqtt_sem_take_recur(&(mosq->current_out_packet_mutex), portMAX_DELAY);
	ut_mqtt_sem_take_recur(&(mosq->out_packet_mutex), portMAX_DELAY);

	if (mosq->out_packet && !mosq->current_out_packet) {
		mosq->current_out_packet	= mosq->out_packet;
		mosq->out_packet			= mosq->out_packet->next;
	}

	while (mosq->current_out_packet) {
		packet						= mosq->current_out_packet;
		mosq->current_out_packet	= mosq->out_packet;

		if (mosq->out_packet) {
			mosq->out_packet = mosq->out_packet->next;
		}

		_mosquitto_packet_cleanup(packet);
		_mosquitto_free(packet);
	}

	xSemaphoreGiveRecursive(mosq->out_packet_mutex.mutex_info);
	xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);

	_mosquitto_messages_reconnect_reset(mosq);

	rc = _mosquitto_socket_connect(mosq, mosq->host, mosq->port);
	if (rc > 0) {
		return rc;
	}

	return _mosquitto_send_connect(mosq, mosq->keepalive, mosq->clean_session);
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
	enum mosquitto_client_state prev_state;

	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

    prev_state = mosq->state;

	ut_mqtt_sem_take_recur(&(mosq->state_mutex), portMAX_DELAY);
	mosq->state = mosq_cs_disconnecting;
	xSemaphoreGiveRecursive(mosq->state_mutex.mutex_info);

	if (mosq->sock == INVALID_SOCKET) {
		MQTT_DBG_INFO(RED_COLOR " [%s:%d] Socket is invalid\n" CLEAR_COLOR,
					 __func__, __LINE__);
		return MOSQ_ERR_NO_CONN;
	}

	// check socket disconnection status : __TBI_MQTT_NET__

	if (prev_state != mosq_cs_connected) {
		MQTT_DBG_INFO(RED_COLOR " [%s:%d] Not required to send MQTT disconnect(prev:%d, cur:%d)\n" CLEAR_COLOR,
					  __func__, __LINE__, prev_state, mosq->state);
		return MOSQ_ERR_NO_CONN;
	}

	return _mosquitto_send_disconnect(mosq);
}

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
					  int payloadlen, const void *payload, int qos, bool retain)
{
	struct mosquitto_message_all *message;
	uint16_t local_mid;
	int queue_status;

	if (!mosq || !topic || qos < 0 || qos > 2) {
		return MOSQ_ERR_INVAL;
	}

	if (STREMPTY(topic)) {
		return MOSQ_ERR_INVAL;
	}

	if (payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD) {
		return MOSQ_ERR_PAYLOAD_SIZE;
	}

	if (mosquitto_pub_topic_check(topic) != MOSQ_ERR_SUCCESS) {
		return MOSQ_ERR_INVAL;
	}

	local_mid = _mosquitto_mid_generate(mosq);

	if (mid) {
		*mid = local_mid;
	}

	if (qos == 0) {
		return _mosquitto_send_publish(mosq, local_mid, topic, payloadlen, payload, qos, retain, false);
	} else {
		message = _mosquitto_calloc(1, sizeof(struct mosquitto_message_all));

		if (!message) {
			return MOSQ_ERR_NOMEM;
		}

		message->next		= NULL;
		message->timestamp	= mosquitto_time();
		message->msg.mid	= local_mid;
		message->msg.topic	= _mosquitto_strdup(topic);

		if (!message->msg.topic) {
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_NOMEM;
		}

		if (payloadlen) {
			message->msg.payloadlen	= payloadlen;
			message->msg.payload	= _mosquitto_malloc(payloadlen * sizeof(uint8_t));

			if (!message->msg.payload) {
				_mosquitto_message_cleanup(&message);
				return MOSQ_ERR_NOMEM;
			}

			memcpy(message->msg.payload, payload, payloadlen * sizeof(uint8_t));
		} else {
			message->msg.payloadlen	= 0;
			message->msg.payload	= NULL;
		}

		message->msg.qos	= qos;
		message->msg.retain	= retain;
		message->dup		= false;

		ut_mqtt_sem_take_recur(&(mosq->out_message_mutex), portMAX_DELAY);
		queue_status = _mosquitto_message_queue(mosq, message, mosq_md_out);

		if (queue_status == 0) {
			if (qos == 1) {
				message->state = mosq_ms_wait_for_puback;
			} else if (qos == 2) {
				message->state = mosq_ms_wait_for_pubrec;
			}
					
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
			mq_msg_tbl_presvd_add(message, mosq_md_out);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

			xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
			return _mosquitto_send_publish(mosq, message->msg.mid, message->msg.topic, message->msg.payloadlen, message->msg.payload,
										   message->msg.qos, message->msg.retain, message->dup);
		} else {
			message->state = mosq_ms_invalid;
			xSemaphoreGiveRecursive(mosq->out_message_mutex.mutex_info);
			return MOSQ_ERR_SUCCESS;
		}
	}
}

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (mosquitto_sub_topic_check(sub)) {
		return MOSQ_ERR_INVAL;
	}

	return _mosquitto_send_subscribe(mosq, mid, sub, qos);
}

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	if (mosquitto_sub_topic_check(sub)) {
		return MOSQ_ERR_INVAL;
	}

	return _mosquitto_send_unsubscribe(mosq, mid, sub);
}

int mosquitto_tls_insecure_set(struct mosquitto *mosq, int insecure)
{
	mosq->tls_insecure = insecure;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_ca_cert_set(struct mosquitto *mosq, char *ca_cert, int ca_cert_len)
{
	if (ca_cert && ca_cert_len) {
		mosq->tls_ca_cert		= ca_cert;
		mosq->tls_ca_cert_len	= ca_cert_len;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_cert_set(struct mosquitto *mosq, char *cert, int cert_len)
{
	if (cert && cert_len) {
		mosq->tls_cert = cert;
		mosq->tls_cert_len = cert_len;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_private_key_set(struct mosquitto *mosq, char *private_key, int private_key_len)
{
	if (private_key && private_key_len) {
		mosq->tls_private_key = private_key;
		mosq->tls_private_key_len = private_key_len;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_dh_param_set(struct mosquitto *mosq, char *dh_param, int dh_param_len)
{
	if (dh_param && dh_param_len) {
		mosq->tls_dh_param = dh_param;
		mosq->tls_dh_param_len = dh_param_len;
	}

	return MOSQ_ERR_SUCCESS;
}

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
int mosquitto_tls_alpn_set(struct mosquitto *mosq, char **alpn, int alpn_count)
{
	if (alpn && alpn_count) {
		mosq->tls_alpn = alpn;
		mosq->tls_alpn_count = alpn_count;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_sni_set(struct mosquitto *mosq, char *sni)
{
	if (sni) {
		mosq->tls_sni = sni;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_tls_cipher_suits_set(struct mosquitto *mosq, int *cipher_suits)
{
	if (cipher_suits) {
		mosq->tls_csuits = cipher_suits;
	}

	return MOSQ_ERR_SUCCESS;
}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__

int mosquitto_tls_set(struct mosquitto *mosq, const char *cafile, const char *capath,
					  const char *certfile, const char *keyfile,
					  int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	DA16X_UNUSED_ARG(mosq);
	DA16X_UNUSED_ARG(cafile);
	DA16X_UNUSED_ARG(capath);
	DA16X_UNUSED_ARG(certfile);
	DA16X_UNUSED_ARG(keyfile);
	DA16X_UNUSED_ARG(pw_callback);

	return MOSQ_ERR_NOT_SUPPORTED;
}


int mosquitto_tls_opts_set(struct mosquitto *mosq, int cert_reqs, const char *tls_version, const char *ciphers)
{
	DA16X_UNUSED_ARG(mosq);
	DA16X_UNUSED_ARG(cert_reqs);
	DA16X_UNUSED_ARG(tls_version);
	DA16X_UNUSED_ARG(ciphers);

	return MOSQ_ERR_NOT_SUPPORTED;
}


int mosquitto_tls_psk_set(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers)
{
#if defined (REAL_WITH_TLS_PSK)
	if (!mosq || !psk || !identity) {
		return MOSQ_ERR_INVAL;
	}

	/* Check for hex only digits */
	if (strspn(psk, "0123456789abcdefABCDEF") < strlen(psk)) {
		return MOSQ_ERR_INVAL;
	}

	mosq->tls_psk = _mosquitto_strdup(psk);

	if (!mosq->tls_psk) {
		return MOSQ_ERR_NOMEM;
	}

	mosq->tls_psk_identity = _mosquitto_strdup(identity);

	if (!mosq->tls_psk_identity) {
		_mosquitto_free(mosq->tls_psk);
		return MOSQ_ERR_NOMEM;
	}

	if (ciphers) {
		mosq->tls_ciphers = _mosquitto_strdup(ciphers);

		if (!mosq->tls_ciphers) {
			return MOSQ_ERR_NOMEM;
		}
	} else {
		mosq->tls_ciphers = NULL;
	}

	return MOSQ_ERR_SUCCESS;
#else
	DA16X_UNUSED_ARG(mosq);
	DA16X_UNUSED_ARG(psk);
	DA16X_UNUSED_ARG(identity);
	DA16X_UNUSED_ARG(ciphers);

	return MOSQ_ERR_NOT_SUPPORTED;
#endif // (REAL_WITH_TLS_PSK)
}

int mqtt_tid = 0;

int mosquitto_loop_read(struct mosquitto *mosq, int max_packets)
{
	int rc;
	int i;

	if (max_packets < 1) {
        return MOSQ_ERR_INVAL;
    }

	if (max_packets < 1) {
        max_packets = 1;
    }
	
	for (i = 0; i < max_packets; i++) {
		rc = _mosquitto_packet_read(mosq);
	}

	return rc;
}

#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
int hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__

int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
	struct timeval local_timeout;
	fd_set readfds;
	int fdcount;
	int rc;
	int maxfd = 0;

	/* ===== 0. check abnormal status ===== */
	
	if (!mosq || max_packets < 1) {
		MQTT_DBG_ERR(" [%s:%d] ERR: (!mosq || max_packets < 1) \n", __func__, __LINE__);
		return MOSQ_ERR_INVAL;
	}

	if (mosq->sock >= FD_SETSIZE) {
		MQTT_DBG_ERR(" [%s:%d] ERR: (mosq->sock >= FD_SETSIZE) \n", __func__, __LINE__);
		return MOSQ_ERR_INVAL;
	}
	
	if (mosq->state == mosq_cs_disconnecting || da16x_network_main_check_net_link_status(WLAN0_IFACE) == pdFALSE) {
		if (dpm_mode_is_enabled() == TRUE) {
			mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);
		}
		MQTT_DBG_ERR(" [%s:%d] ERR: MOSQ_ERR_CONN_LOST \n", __func__, __LINE__);
		return MOSQ_ERR_CONN_LOST;
	} else if (mosq->state == mosq_cs_disconnect_cb) {
		if (dpm_mode_is_enabled() == TRUE) {
			mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);
		}
		MQTT_DBG_ERR(" [%s:%d] ERR: MOSQ_ERR_DISCONNECT \n", __func__, __LINE__);
		return MOSQ_ERR_DISCONNECT;
	}

	/* ===== 1. check keep alive, msg retry if needed ===== */
	mosquitto_loop_misc(mosq);

	/* ===== 2. mqtt msg tx if any ==== */
	
	// In non-DPM mode, mqtt_client sends a PUBLISH
	if (!dpm_mode_is_enabled() && mosq->state == mosq_cs_connected) {
		ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
		my_publish(mosq, NULL, 0);
		xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
	}

	/* ===== 3. mqtt msg rx if any ==== */
	
	// start init for select( )
	
	FD_ZERO(&readfds);
	
	if (mosq->sock != INVALID_SOCKET) {
		maxfd = mosq->sock;
		FD_SET(mosq->sock, &readfds);	
	} else {
		MQTT_DBG_ERR(" [%s:%d] ERR: INVALID_SOCKET \n", __func__, __LINE__);
		return MOSQ_ERR_NO_CONN;
	}

	if (timeout < 0) {    // timeout==-1
		timeout = 10; // 100ms select time-out
	}

	
	ut_sock_convert2tv(&local_timeout, (ULONG)timeout);	

	fdcount = select(maxfd+1, &readfds, NULL, NULL, &local_timeout);
	
	if (fdcount <= -1) {
		// select( ) error ...
		
		if (errno == EINTR) {
			MQTT_DBG_INFO("[%s:%d] errno=EINTR\n",__func__, __LINE__);
#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
			hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__
			return MOSQ_ERR_SUCCESS;
		} else {
			MQTT_DBG_ERR(" [%s:%d] ERR: errno=%d \n", __func__, __LINE__, errno);
#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
			hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__

			return MOSQ_ERR_ERRNO;
		}
	} else {
		// times out (no rx packet) or valid packet read ...
	
		if (fdcount == 0) {
			// select( ) timeout

#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
			// time-out without data rx
			
			/*
				condition : dpm wakeup &&
							tls enabled &&
							tls packet decryption in progress
				flag set: hold_sleep_due_to_tls_decrypt_in_progress
			*/
	
			if (dpm_mode_is_wakeup() && mqttParams.insecure == pdTRUE) {
					if (mbedtls_ssl_is_decrypting(mosq->ssl_ctx)) {
						hold_sleep_due_to_tls_decrypt_in_progress = pdTRUE;
					} else {
						hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
					}
			} else {
				hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
			}
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__
			
			return MOSQ_ERR_SUCCESS;
		}
		
		// fdcount > 0 : there's incoming data from stack

		if (mosq->sock != INVALID_SOCKET) {
			if (FD_ISSET(mosq->sock, &readfds)) {
				if (dpm_mode_is_enabled()) {
					mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);							
				}

				rc = mosquitto_loop_read(mosq, max_packets);
			} else {
				// (fdcount > 0) + (socket != invalid) + no event in readfds
				MQTT_DBG_ERR(" [%s:%d] ERR: FD_ISSET(f) \n", __func__, __LINE__);
				rc = MOSQ_ERR_ERRNO;
			}
		} else {
			MQTT_DBG_ERR(" [%s:%d] ERR: INVALID_SOCKET \n", __func__, __LINE__);
			rc = MOSQ_ERR_CONN_LOST;
		}
	}

#if defined (__MQTT_DPM_WAIT_TLS_FULL_DECRYPT__)
	hold_sleep_due_to_tls_decrypt_in_progress = pdFALSE;
#endif // __MQTT_DPM_WAIT_TLS_FULL_DECRYPT__

	return rc;
}

int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets)
{
	int rc;

	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	do {
		rc = mosquitto_loop(mosq, timeout, max_packets);

		if (mosq->mqtt_pub_or_sub == CLIENT_SUB) {
			if (runContinueSub != true) {
				rc = MOSQ_ERR_STOP;
				MQTT_DBG_INFO(" [%s:%d] Stop loop SUB \n", __func__, __LINE__);
				return rc;
			}
		}
	} while (rc == MOSQ_ERR_SUCCESS);

	return rc;
}

int mosquitto_loop_misc(struct mosquitto *mosq)
{
	da16x_time_t now;

	if (!mosq) {
		return MOSQ_ERR_INVAL;
	}

	_mosquitto_check_keepalive(mosq);
	now = mosquitto_time();

	if (mosq->last_retry_check + 1 < now) {
		_mosquitto_message_retry_check(mosq);
		mosq->last_retry_check = now;
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
		mq_msg_tbl_presvd_update_last_retry_chk(mosq->last_retry_check);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	}

	if (mosq->ping_t && now - mosq->ping_t >= mosq->keepalive) {
		_mosquitto_socket_close(mosq);
		
		ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
		if (mosq->on_disconnect) {
			mosq->in_callback = true;
			mosq->on_disconnect();
			mosq->in_callback = false;
		}		
		xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);

		return MOSQ_ERR_CONN_LOST;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_opts_set(struct mosquitto *mosq, enum mosq_opt_t option, void *value)
{
	int ival;

	if (!mosq || !value) {
		return MOSQ_ERR_INVAL;
	}

	switch (option)
	{
	case MOSQ_OPT_PROTOCOL_VERSION:
		ival = *((int *)value);

		if (ival == MQTT_PROTOCOL_V31) {
			mosq->protocol = mosq_p_mqtt31;
		} else if (ival == MQTT_PROTOCOL_V311) {
			mosq->protocol = mosq_p_mqtt311;
		} else {
			return MOSQ_ERR_INVAL;
		}

		break;

	default:
		return MOSQ_ERR_INVAL;
	}

	return MOSQ_ERR_SUCCESS;
}

void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(void))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_connect = on_connect;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

void mosquitto_disconnect_callback_set(struct mosquitto *mosq, void (*on_disconnect)(void))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_disconnect = on_disconnect;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
void mosquitto_disconnect2_callback_set(struct mosquitto *mosq, void (*on_disconnect2)(void))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_disconnect2 = on_disconnect2;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

void mosquitto_publish_callback_set(struct mosquitto *mosq, void (*on_publish)(int))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_publish = on_publish;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

void mosquitto_message_callback_set(struct mosquitto *mosq, void (*on_message)(const char *buf, int len, const char *topic))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_message = on_message;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

void mosquitto_subscribe_callback_set(struct mosquitto *mosq, void (*on_subscribe)(void))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_subscribe = on_subscribe;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq, void (*on_unsubscribe)(void))
{
	ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);
	mosq->on_unsubscribe = on_unsubscribe;
	xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
}

void mosquitto_log_callback_set(struct mosquitto *mosq, void (*on_log)(const char *))
{
	ut_mqtt_sem_take_recur(&(mosq->log_callback_mutex), portMAX_DELAY);
	mosq->on_log = on_log;
	xSemaphoreGiveRecursive(mosq->log_callback_mutex.mutex_info);
}

void mosquitto_user_data_set(struct mosquitto *mosq, void *userdata)
{
	if (mosq) {
		mosq->userdata = userdata;
	}
}

const char *mosquitto_strerror(int mosq_errno)
{
	switch (mosq_errno) {
	case MOSQ_ERR_CONN_PENDING:
		return "Connection pending.";

	case MOSQ_ERR_SUCCESS:
		return "No error.";

	case MOSQ_ERR_NOMEM:
		return "Out of memory.";

	case MOSQ_ERR_PROTOCOL:
		return "A network protocol error occurred when communicating with the broker.";

	case MOSQ_ERR_INVAL:
		return "Invalid function arguments provided.";

	case MOSQ_ERR_NO_CONN:
		return "The client is not currently connected.";

	case MOSQ_ERR_CONN_REFUSED:
		return "The connection was refused.";

	case MOSQ_ERR_NOT_FOUND:
		return "Message not found (internal error).";

	case MOSQ_ERR_CONN_LOST:
		return "The connection was lost.";

	case MOSQ_ERR_TLS:
		return "A TLS error occurred.";

	case MOSQ_ERR_PAYLOAD_SIZE:
		return "Payload too large.";

	case MOSQ_ERR_NOT_SUPPORTED:
		return "This feature is not supported.";

	case MOSQ_ERR_AUTH:
		return "Authorisation failed.";

	case MOSQ_ERR_ACL_DENIED:
		return "Access denied by ACL.";

	case MOSQ_ERR_UNKNOWN:
		return "Unknown error.";

	case MOSQ_ERR_ERRNO:
		return "Unknown error.";

	case MOSQ_ERR_EAI:
		return "Lookup error.";

	case MOSQ_ERR_PROXY:
		return "Proxy error.";

	case MOSQ_ERR_TLS_INVALID_CERT:
		return "Invalid certificate (internal error).";

	case MOSQ_ERR_TLS_HANDSHAKE:
		return "TLS Handshake failed.";

	case MOSQ_ERR_TLS_HANDSHAKE_CERT:
		return "TLS Handshake failed (Certificate).";

	case MOSQ_ERR_TLS_HANDSHAKE_TIME_EXPIRED:
		return "TLS Handshake failed (Time expired).";

	default:
		return "Unknown error.";
	}
}

const char *mosquitto_connack_string(int connack_code)
{
	switch (connack_code) {
	case 0:
		return "Connection Accepted.";

	case 1:
		return "Connection Refused: unacceptable protocol version.";

	case 2:
		return "Connection Refused: identifier rejected.";

	case 3:
		return "Connection Refused: broker unavailable.";

	case 4:
		return "Connection Refused: bad user name or password.";

	case 5:
		return "Connection Refused: not authorised.";

	default:
		return "Connection Refused: unknown reason.";
	}
}

int mosquitto_sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
	int len;
	int hier_count = 1;
	int start, stop;
	int hier;
	int tlen;
	int i, j;

	if (!subtopic || !topics || !count) {
		return MOSQ_ERR_INVAL;
	}

	len = strlen(subtopic);

	for (i = 0; i < len; i++) {
		if (subtopic[i] == '/') {
			if (i > len - 1) {
				/* Separator at end of line */
			} else {
				hier_count++;
			}
		}
	}

	(*topics) = _mosquitto_calloc(hier_count, sizeof(char *));

	if (!(*topics)) {
		return MOSQ_ERR_NOMEM;
	}

	start = 0;
	stop = 0;
	hier = 0;

	for (i = 0; i < len + 1; i++) {
		if (subtopic[i] == '/' || subtopic[i] == '\0') {
			stop = i;

			if (start != stop) {
				tlen = stop - start + 1;
				(*topics)[hier] = _mosquitto_calloc(tlen, sizeof(char));

				if (!(*topics)[hier]) {
					for (i = 0; i < hier_count; i++) {
						if ((*topics)[hier]) {
							_mosquitto_free((*topics)[hier]);
						}
					}

					_mosquitto_free(*topics);
					return MOSQ_ERR_NOMEM;
				}

				for (j = start; j < stop; j++) {
					(*topics)[hier][j - start] = subtopic[j];
				}
			}

			start = i + 1;
			hier++;
		}
	}

	*count = hier_count;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_sub_topic_tokens_free(char ***topics, int count)
{
	int i;

	if (!topics || !(*topics) || count < 1) {
		return MOSQ_ERR_INVAL;
	}

	for (i = 0; i < count; i++) {
		if ((*topics)[i]) {
			_mosquitto_free((*topics)[i]);
		}
	}

	_mosquitto_free(*topics);

	return MOSQ_ERR_SUCCESS;
}

UINT mosquitto_pingreq_timer_register(struct mosquitto *mosq)
{
	int status = ERR_OK;

	if (mosq->keepalive <= 0) {
		MQTT_DBG_INFO(" [%s] Not register timer (keepalive:%d)\n", __func__, mosq->keepalive);
		status = ER_NOT_SUCCESSFUL;
	} else {
		if (mosq->mqtt_pub_or_sub == CLIENT_SUB) {
			MQTT_DBG_INFO(GREEN_COLOR " [%s] Register keepalivetime(id=%d, itv=%d).\n" CLEAR_COLOR,
						  __func__, MQTT_SUB_PERIOD_TIMER_ID, mosq->keepalive);
			status = rtc_register_timer(mosq->keepalive * 1000, APP_MQTT_SUB, MQTT_SUB_PERIOD_TIMER_ID, 1, mosquitto_sub_pingreq_timer_cb);
		} else {
			MQTT_DBG_ERR(RED_COLOR "Failed to register keepalivetime(pub_or_sub=%d).\n" CLEAR_COLOR,
						 mosq->mqtt_pub_or_sub);
		}
	}

	if (status < 0) {
		MQTT_DBG_ERR(RED_COLOR "Failed to register timer\n" CLEAR_COLOR);
		status = ER_NOT_SUCCESSFUL;
	}

	return status;
}

UINT mosquitto_pingreq_timer_unregister(int tid)
{
	if (dpm_mode_is_enabled()) {
		rtc_cancel_timer(tid);
	}

	return ERR_OK;
}

static int is_supp_completed(void)
{
	if (chk_supp_connected() == pdFAIL) {
		MQTT_DBG_ERR(RED_COLOR "Occurred mqtt timer. Return by not COMPLETED\n" CLEAR_COLOR);
		return 0;
	} else {
		return 1;
	}
}

void mosquitto_pingreq_timer_func(struct mosquitto *mosq)
{
	extern UINT	subPingStatus;
	int ret = 0;

	if (mosq && mosq->state == mosq_cs_connected) {
		if (mosq->mqtt_pub_or_sub == CLIENT_SUB) {
			subPingStatus = PING_STS_SENT;
		}

        mqtt_client_dpm_clear(APP_MQTT_SUB, __func__, __LINE__);
		ret = _mosquitto_send_pingreq(mosq);
		if (ret) {
			MQTT_DBG_ERR(RED_COLOR "Failed to send ping req(%d)\n" CLEAR_COLOR, ret);
		}
	}
}

void mosquitto_pingreq_timer_sub_thread(void* thread_input)
{
	DA16X_UNUSED_ARG(thread_input);

	int sys_wdog_id = -1;

	extern struct mosquitto *mosq_sub;

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	da16x_sys_watchdog_notify(sys_wdog_id);

	mosquitto_pingreq_timer_func(mosq_sub);

	da16x_sys_watchdog_unregister(sys_wdog_id);

	vTaskDelete(NULL);
}

void create_mosquitto_pingreq_sub_thread(void)
{
	TaskStatus_t xTaskDetails;

	if (mosquittoPingSubThread != NULL) {
		vTaskGetInfo(mosquittoPingSubThread,
					&xTaskDetails,
					pdFALSE,
					eInvalid);

		if (ut_is_task_running(&xTaskDetails)) {
			return;
		}
	}

	xTaskCreate(mosquitto_pingreq_timer_sub_thread,
				"mosqPingSubThd",
				(MOSQ_SUB_STACK_SIZE/4),
				NULL,
				(OS_TASK_PRIORITY_USER + 6),
				&mosquittoPingSubThread);
}

void mosquitto_sub_pingreq_timer_cb(char *timer_name)
{
	DA16X_UNUSED_ARG(timer_name);

	extern struct mosquitto *mosq_sub;

	if (dpm_mode_is_enabled()) {
		if (!is_supp_completed()) {
			return;
		}

		create_mosquitto_pingreq_sub_thread();
	}

	return ;
}

#else

char	mqtt_conn_state(void)
{
	return 0;
}

#endif // (__SUPPORT_MQTT__)

/* EOF */
