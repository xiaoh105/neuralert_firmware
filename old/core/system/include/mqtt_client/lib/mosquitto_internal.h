/**
 *****************************************************************************************
 * @file		mosquitto_internal.h
 * @brief	Header file - MQTT module internal defined entities
 * @todo		File name : mosquitto_internal.h -> mqtt_client_internal.h
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

#ifndef	__MOSQUITTO_INTERNAL_H__
#define	__MOSQUITTO_INTERNAL_H__

#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "clib.h"

#include "mosquitto.h"

#include "mbedtls/config.h"

#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/rsa.h"

#include "cc3120_hw_eng_initialize.h"

#include "da16x_dpm_tls.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#define CLIENT_PUB 1
#define CLIENT_SUB 2

typedef int mosq_sock_t;

typedef struct mqtt_mutex_control_block
{
	/* This mutex's control block  */
	SemaphoreHandle_t mutex_info;

	/* This mutex's attributes */
	INT           type;
	/* Is this Mutex object is in use?  */
	INT           in_use;

} mqtt_mutex_t;

enum mosquitto_msg_direction
{
	mosq_md_in = 0,
	mosq_md_out = 1
};

enum mosquitto_msg_state
{
	mosq_ms_invalid					= 0,
	mosq_ms_publish_qos0			= 1,
	mosq_ms_publish_qos1			= 2,
	mosq_ms_wait_for_puback			= 3,
	mosq_ms_publish_qos2			= 4,
	mosq_ms_wait_for_pubrec			= 5,
	mosq_ms_resend_pubrel			= 6,
	mosq_ms_wait_for_pubrel			= 7,
	mosq_ms_resend_pubcomp			= 8,
	mosq_ms_wait_for_pubcomp		= 9,
	mosq_ms_send_pubrec				= 10,
	mosq_ms_queued					= 11
};

enum mosquitto_client_state
{
	mosq_cs_new						= 0,
	mosq_cs_connected				= 1,
	mosq_cs_disconnecting			= 2,
	mosq_cs_connect_async			= 3,
	mosq_cs_connect_pending			= 4,
	mosq_cs_connect_srv				= 5,
	mosq_cs_disconnect_cb			= 6,
	mosq_cs_disconnected			= 7,
	mosq_cs_socks5_new				= 8,
	mosq_cs_socks5_start			= 9,
	mosq_cs_socks5_request			= 10,
	mosq_cs_socks5_reply			= 11,
	mosq_cs_socks5_auth_ok			= 12,
	mosq_cs_socks5_userpass_reply	= 13,
	mosq_cs_socks5_send_userpass	= 14,
	mosq_cs_expiring				= 15,
	mosq_cs_connected_in_dpm		= 16,
};

enum _mosquitto_protocol
{
	mosq_p_invalid					= 0,
	mosq_p_mqtt31					= 1,
	mosq_p_mqtt311					= 2,
	mosq_p_mqtts					= 3,
};

enum mosquitto__threaded_state
{
	mosq_ts_none					= 0,
	mosq_ts_self					= 1,
	mosq_ts_external				= 2	,
};

enum _mosquitto_transport
{
	mosq_t_invalid					= 0,
	mosq_t_tcp						= 1,
	mosq_t_ws						= 2,
	mosq_t_sctp						= 3
};

struct _mosquitto_packet
{
	uint8_t	*payload;
	struct _mosquitto_packet *next;
	uint32_t remaining_mult;
	uint32_t remaining_length;
	uint32_t packet_length;
	uint32_t to_process;
	uint32_t pos;
	uint16_t mid;
	uint8_t command;
	int8_t remaining_count;
};

struct mosquitto_message_all
{
	struct mosquitto_message_all *next;
	da16x_time_t timestamp;
	enum mosquitto_msg_state state;
	bool dup;
	struct mosquitto_message msg;
};

struct mosquitto
{
	enum _mosquitto_protocol protocol;
	char *address;
	char *id;
	char *username;
	char *password;
	int tid;
	uint16_t keepalive;
	uint16_t last_mid;
	enum mosquitto_client_state state;
	da16x_time_t last_msg_in;
	da16x_time_t next_msg_out;
	da16x_time_t ping_t;
	struct _mosquitto_packet in_packet;
	struct _mosquitto_packet *current_out_packet;
	struct _mosquitto_packet *out_packet;
	struct mosquitto_message *will;

	bool tls_insecure;
	mbedtls_ssl_context      *ssl_ctx;
	mbedtls_ssl_config       *ssl_conf;
	mbedtls_ctr_drbg_context *ctr_drbg_ctx;
	mbedtls_entropy_context  *entropy_ctx;

	mbedtls_x509_crt         *ca_cert_crt;
	mbedtls_x509_crt         *cert_crt;
	mbedtls_pk_context       *pkey_ctx;
	mbedtls_pk_context       *pkey_alt_ctx;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_profile  crt_profile;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
    mbedtls_dhm_context      *dhm_ctx;

	char                     *tls_ca_cert;
	size_t                    tls_ca_cert_len;
	char                     *tls_cert;
	size_t                    tls_cert_len;
	char                     *tls_private_key;
	size_t                    tls_private_key_len;
	char                     *tls_dh_param;
	size_t                    tls_dh_param_len;
	int	                      tls_time_check;
	int                       tls_auth_mode;
	int                       tls_incoming_size;
	int                       tls_outgoing_size;
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	char                    **tls_alpn;
	int                       tls_alpn_count;
	char                     *tls_sni;
	int						 *tls_csuits;
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
	char                      socket_name[20]; //REG_NAME_DPM_MAX_LEN

	ULONG                     socket_timeout;
	
	mosq_sock_t 			  sock;
	bool					  socket_timeout_required;

	bool want_write;
	bool want_connect;

	mqtt_mutex_t callback_mutex;
	mqtt_mutex_t log_callback_mutex;
	mqtt_mutex_t msgtime_mutex;
	mqtt_mutex_t out_packet_mutex;
	mqtt_mutex_t current_out_packet_mutex;
	mqtt_mutex_t state_mutex;
	mqtt_mutex_t in_message_mutex;
	mqtt_mutex_t out_message_mutex;
	mqtt_mutex_t mid_mutex;
	mqtt_mutex_t send_msg_mutex;
	ULONG thread_id;

	bool clean_session;

	void *userdata;
	bool in_callback;
	unsigned int message_retry;
	da16x_time_t last_retry_check;
	struct mosquitto_message_all *in_messages;
	struct mosquitto_message_all *in_messages_last;
	struct mosquitto_message_all *out_messages;
	struct mosquitto_message_all *out_messages_last;
	void (*on_connect)(void);
	void (*on_disconnect)(void);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
	void (*on_disconnect2)(void);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
	void (*on_publish)(int mid);
	void (*on_message)(const char *buf, int len, const char *topic);
	void (*on_subscribe)(void);
	void (*on_unsubscribe)(void);
	void (*on_log)(const char *str);
	char *host;
	int port;
	int in_queue_len;
	int out_queue_len;
	char *bind_address;
	unsigned int reconnect_delay;
	unsigned int reconnect_delay_max;
	bool reconnect_exponential_backoff;
	char threaded;
	struct _mosquitto_packet *out_packet_last;
	int inflight_messages;
	int max_inflight_messages;
	int q2_proc;
	int unsub_topic;
	int mqtt_pub_or_sub;
	int rtm_saved;
};

typedef struct _mqtt_fd
{
	INT                  count;
	ULONG                desc;
} mqtt_fd;

#define STREMPTY(str) (str[0] == '\0')

#endif	/* __MOSQUITTO_INTERNAL_H__ */
