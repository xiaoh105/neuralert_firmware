/**
 *****************************************************************************************
 * @file	net_mosq.c
 * @brief	MQTT implementation with network stack
 * @todo	File name : net_mosq.c -> mqtt_client_net.c
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
#include "read_handle.h"
#include "mqtt3_protocol.h"
#include "net_mosq.h"
#include "util_mosq.h"
#include "mqtt_client.h"
#include "da16x_network_common.h"
#include "lwip/sockets.h"

#pragma GCC diagnostic ignored "-Wsign-conversion"

extern struct mosquitto *mosq_sub;
extern mqttParamForRtm	mqttParams;
extern int new_local_port_allocated;
extern void ut_sock_reset (int* sock);
extern void ut_sock_opt_get_timeout(mosq_sock_t sock, ULONG* timeval);
extern int ut_sock_opt_set_timeout (mosq_sock_t sock, ULONG timeval, UINT8 for_recv);
extern void ut_sock_convert2tv(struct timeval* tv, ULONG ultime);
extern int da16x_dpm_sock_is_established(char *name);

static void _mosquitto_secure_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	DA16X_UNUSED_ARG(ctx);

	const char *p, *basename;

	/* Extract basename from file */
	for ( p = basename = file; *p != '\0'; p++ )
		if ( *p == '/' || *p == '\\' )
		{
			basename = p + 1;
		}

	MQTT_DBG_PRINT("%s:%04d: |%d| %s\n", basename, line, level, str );
}

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
static int _mosquitto_rsa_decrypt_func( void *ctx, int mode, size_t *olen, const unsigned char *input,
									   unsigned char *output, size_t output_max_len )
{
	return mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *)ctx, NULL, NULL, mode, olen,
									 input, output, output_max_len);
}

static int _mosquitto_rsa_sign_func( void *ctx, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
									 int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
									 const unsigned char *hash, unsigned char *sig )
{
	return mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *)ctx, f_rng, p_rng, mode, md_alg, hashlen, hash, sig);
}

static size_t _mosquitto_rsa_key_len_func( void *ctx )
{
	return ( ((const mbedtls_rsa_context *) ctx)->len );
}
#endif // (MBEDTLS_RSA_C) && (MBEDTLS_PK_RSA_ALT_SUPPORT)

void _mosquitto_packet_cleanup(struct _mosquitto_packet *packet)
{
	if (!packet)
	{
		return;
	}

	packet->command				= 0;
	packet->remaining_count		= 0;
	packet->remaining_mult		= 1;
	packet->packet_length		= 0;
	packet->remaining_length	= 0;

	if (packet->payload)
	{
		_mosquitto_free(packet->payload);
	}

	packet->payload				= NULL;
	packet->to_process			= 0;
	packet->pos					= 0;
}

int _mosquitto_packet_queue(struct mosquitto *mosq, struct _mosquitto_packet *packet)
{
	packet->pos = 0;
	packet->to_process = packet->packet_length;

	packet->next = NULL;
	ut_mqtt_sem_take_recur(&(mosq->out_packet_mutex), portMAX_DELAY);

	if (mosq->out_packet)
	{
		mosq->out_packet_last->next = packet;
	}
	else
	{
		mosq->out_packet = packet;
	}

	mosq->out_packet_last = packet;

	xSemaphoreGiveRecursive(mosq->out_packet_mutex.mutex_info);

	if (mosq->in_callback == false && mosq->threaded == mosq_ts_none)
	{
		return _mosquitto_packet_write(mosq);
	}
	else
	{
		return MOSQ_ERR_SUCCESS;
	}
}

int _mosquitto_socket_close(struct mosquitto *mosq)
{
	int rc = 0;

	if (!mosq) return 0;

	_mosquitto_tls_deinit(mosq);

	if((int)mosq->sock >= 0)
	{
		rc = close(mosq->sock);
		mosq->sock = INVALID_SOCKET;
	}
	return rc;
}

int _mosquitto_try_connect(struct mosquitto *mosq, const char *host, uint16_t port)
{
	int status = ERR_OK;
	struct sockaddr_in broker_addr, local_addr;
	int my_port;
	fd_set writablefds;
	struct timeval local_timeout;
	int sockfd_flags_before;
	extern int subRetryCount;
	int sockopt_reuse = 1;

	memset(&local_addr, 0x00, sizeof(struct sockaddr_in));
	memset(&broker_addr, 0x00, sizeof(struct sockaddr_in));

	int pub_or_sub = mosq->mqtt_pub_or_sub;

	if (pub_or_sub == CLIENT_SUB)
	{
		my_port = mqttParams.my_sub_port_no;
		strcpy(mosq->socket_name, SUB_SOCK_NAME);
	}
	else
	{
		MQTT_DBG_ERR(RED_COLOR "Unknown mode (%s)\n" CLEAR_COLOR, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB");
		return MOSQ_ERR_UNKNOWN;
	}

	if (mosq->sock == INVALID_SOCKET)
	{
		// no valid socket exists ... proceed to socket creation / restoration 
	}
	else
	{
		// valid socket exists ... 
		MQTT_DBG_ERR(RED_COLOR "%s Socket already exist\n" CLEAR_COLOR,
					 (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB");
		return MOSQ_ERR_INVAL;
	}

	// create a socket
	if (dpm_mode_is_enabled() == TRUE)
		mosq->sock = socket_dpm(mosq->socket_name, PF_INET, SOCK_STREAM, 0);
	else
		mosq->sock = socket(PF_INET, SOCK_STREAM, 0);
	
	if (mosq->sock < 0)
	{
		MQTT_DBG_ERR("[%s] Failed to create socket\n", __func__);
		mosq->sock = INVALID_SOCKET;
		return MOSQ_ERR_UNKNOWN;
	}

	if (dpm_mode_is_wakeup() == TRUE                              && 
		da16x_dpm_sock_is_established(mosq->socket_name) == TRUE  &&
		subRetryCount == 0                                        && 
		mqttParams.sub_connect_retry_count == 0 )
	{
		mosq->state = mosq_cs_connected_in_dpm; // meaning broker connection is intact in dpm mode
		MQTT_DBG_INFO("[%s] %s: Restored sess (my_port:%d)\n",
				  __func__, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB", my_port);
	
		return MOSQ_ERR_SUCCESS;
	}

	status = setsockopt(mosq->sock, SOL_SOCKET, SO_REUSEADDR, &sockopt_reuse, sizeof(sockopt_reuse));
	if (status != 0) {
		MQTT_DBG_ERR("[%s] setsockopt fails. \n", __func__);
		ut_sock_reset(&(mosq->sock));
		return MOSQ_ERR_NO_CONN;
	}
		
	// bind to my_port
	MQTT_DBG_INFO("[%s] %s Binding (my_port:%d)\n",
				  __func__, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB", my_port);


	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = (in_port_t)htons((u16_t)my_port);
	
	status = bind(mosq->sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (status == -1)
	{
		MQTT_DBG_ERR("[%s] Failed to bind socket\n", __func__);
		ut_sock_reset(&(mosq->sock));
		return MOSQ_ERR_NO_CONN;
	}

	MQTT_DBG_INFO("[%s] %s Connecting (ip:%s port:%d)\n",
				  __func__, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB", host, port);

	// connect
	broker_addr.sin_family = AF_INET;
	broker_addr.sin_addr.s_addr = inet_addr((char *)host);
	broker_addr.sin_port = htons(port);
	
	sockfd_flags_before = fcntl(mosq->sock, F_GETFL, 0);		
	fcntl(mosq->sock, F_SETFL, sockfd_flags_before | O_NONBLOCK);

	status = connect(mosq->sock, (struct sockaddr *)&broker_addr, 
					sizeof(struct sockaddr_in));

	if (status == 0) {
        MQTT_DBG_INFO("[%s] %s Connected !\n", __func__, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB");
		fcntl(mosq->sock, F_SETFL, sockfd_flags_before);
	} else if ( status == -1 && errno == EINPROGRESS ) {
		MQTT_DBG_INFO("[%s] connection in progress, ret=%d, errno=%d\r\n", __func__, status, errno);

	    FD_ZERO(&writablefds);
	    FD_SET(mosq->sock, &writablefds);
		ut_sock_convert2tv(&local_timeout, 250); // 2.5 second time-out

		status = select(mosq->sock + 1, NULL, &writablefds, NULL, &local_timeout);

		if (status != 1) {
			if (status == -1) {
				MQTT_DBG_INFO("[%s] select failed !\r\n", __func__);
			} else if (status == 0) {
				MQTT_DBG_INFO("[%s] select timeout !\r\n", __func__);
			}
			goto MQTT_TCP_CONN_FAIL;
		}

		// Socket selected for write ...

        int sock_error;
        socklen_t len = sizeof(sock_error);

		status = getsockopt(mosq->sock, SOL_SOCKET, SO_ERROR, &sock_error, &len);

		if (status < 0) {
			MQTT_DBG_ERR("[%s] Error getsockopt \r\n", __func__);
			goto MQTT_TCP_CONN_FAIL;
		}

		if (sock_error == 0) {
			MQTT_DBG_INFO("[%s] %s Connected !\n", __func__, (pub_or_sub == CLIENT_SUB) ? "SUB" : "PUB");
			fcntl(mosq->sock, F_SETFL, sockfd_flags_before);
		}
		else
		{
			MQTT_DBG_ERR("[%s] Error in non-blocking connection (sock_error=%d:%d)\r\n", 
						__func__, sock_error, errno);
			goto MQTT_TCP_CONN_FAIL;
		}

	} else {
		// Non EINPROGRESS error!
		MQTT_DBG_ERR("[%s] Non EINPROGRESS error!\r\n", __func__);
		goto MQTT_TCP_CONN_FAIL;
	}

	return MOSQ_ERR_SUCCESS;

MQTT_TCP_CONN_FAIL:
	MQTT_DBG_ERR(RED_COLOR "[%s] Connecting FAIL (status=%d, errno=%d) \n" CLEAR_COLOR, 
				__func__, status, errno);
	ut_sock_reset(&(mosq->sock));
	return MOSQ_ERR_CONN_REFUSED;
}

static int mosquitto__socket_connect_tls_verify(struct mosquitto *mosq)
{
	int ret = MOSQ_ERR_SUCCESS;
	int verify_result = 0;
	int is_empty_ca_cert = mosq->ca_cert_crt ? 0 : 1;

	//check result of verification of the certificate
	verify_result = (int)mbedtls_ssl_get_verify_result(mosq->ssl_ctx);

	if (verify_result)
	{
		if ((mosq->tls_time_check)
				&& ((verify_result & MBEDTLS_X509_BADCERT_EXPIRED)
					|| (verify_result & MBEDTLS_X509_BADCERT_FUTURE)))
		{
			MQTT_DBG_ERR(" [%s]Failed to verify certificate validity\n", __func__);

			ret = MOSQ_ERR_TLS_HANDSHAKE_TIME_EXPIRED;
		}

		if (!is_empty_ca_cert &&
				((verify_result & MBEDTLS_X509_BADCERT_OTHER)
				 || (verify_result & MBEDTLS_X509_BADCERT_CN_MISMATCH)
				 || (verify_result & MBEDTLS_X509_BADCERT_KEY_USAGE)
				 || (verify_result & MBEDTLS_X509_BADCERT_EXT_KEY_USAGE)
				 || (verify_result & MBEDTLS_X509_BADCERT_NS_CERT_TYPE)
				 || (verify_result & MBEDTLS_X509_BADCERT_BAD_MD)
				 || (verify_result & MBEDTLS_X509_BADCERT_BAD_PK)
				 || (verify_result & MBEDTLS_X509_BADCERT_BAD_KEY)
				 || (verify_result & MBEDTLS_X509_BADCERT_REVOKED)
				 || (verify_result & MBEDTLS_X509_BADCERT_NOT_TRUSTED)))
		{
			MQTT_DBG_ERR(" [%s] Failed to verify certificate(0x%x)\n", __func__, verify_result);

			ret = MOSQ_ERR_TLS_HANDSHAKE_CERT;
		}

#if defined	(MQTT_DEBUG_INFO)
		MQTT_DBG_ERR("[%s] Failed to verify TLS handshake(0x%x)\n", __func__, verify_result);

		if (verify_result & MBEDTLS_X509_BADCERT_EXPIRED)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_EXPIRED\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_EXPIRED);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_FUTURE)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_FUTURE\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_FUTURE);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_OTHER)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_OTHER\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_OTHER);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_CN_MISMATCH)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_CN_MISMATCH\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_CN_MISMATCH);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_KEY_USAGE)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_KEY_USAGE\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_KEY_USAGE);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_EXT_KEY_USAGE)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_EXT_KEY_USAGE\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_EXT_KEY_USAGE);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_NS_CERT_TYPE)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_NS_CERT_TYPE\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_NS_CERT_TYPE);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_BAD_MD)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_BAD_MD\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_BAD_MD);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_BAD_PK)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_BAD_PK\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_BAD_PK);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_BAD_KEY)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_BAD_KEY\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_BAD_KEY);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_REVOKED)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_REVOKED\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_REVOKED);
		}

		if (verify_result & MBEDTLS_X509_BADCERT_NOT_TRUSTED)
		{
			MQTT_DBG_ERR(" * MBEDTLS_X509_BADCERT_NOT_TRUSTED\n");
			verify_result &= ~(MBEDTLS_X509_BADCERT_NOT_TRUSTED);
		}

		if (verify_result)
		{
			MQTT_DBG_ERR(" * Remaining result(0x%0x)\n", verify_result);
		}
#endif // (MQTT_DEBUG_INFO)
	}

	return ret;

}

int mosquitto__socket_connect_tls(struct mosquitto *mosq)
{
	int			ret = 0;
	int			retry = 1;
	const int	max_retry = 4;

	if (mosq->tls_insecure)
	{
		if (mosq->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			MQTT_DBG_INFO("[%s] Already established tls-sess\n", __func__);
			return MOSQ_ERR_SUCCESS;
		}

		mosq->socket_timeout_required = TRUE;

		// tls handshake timeout
		mosq->socket_timeout = 500;	// 5 sec

		MQTT_DBG_INFO("tls handshake started\n", __func__);
		
		while ((ret = mbedtls_ssl_handshake(mosq->ssl_ctx)) != 0)
		{
			if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
			{
				extern UINT	runContinueSub;
				extern UINT	force_mqtt_stop;

				if ((mosq->mqtt_pub_or_sub == CLIENT_SUB && runContinueSub == false) || 
					(force_mqtt_stop == true))
				{
					MQTT_DBG_ERR("[%s] Stop tls handshake - %d,%ld\n",
								 __func__, mosq->mqtt_pub_or_sub, mosq->socket_timeout);

					return MOSQ_ERR_TLS_HANDSHAKE;
				}

				if (++retry < max_retry)
				{
					MQTT_DBG_ERR("[%s] Wait for TLS Handshake msg for (%ld) seconds : (%ld) seconds passed \n",
								 __func__, 
								 (mosq->socket_timeout*(max_retry-1))/100, 
								 (mosq->socket_timeout*(retry-1))/100);
					continue;
				}
			}

			if ((ret != MBEDTLS_ERR_SSL_WANT_READ)
					&& (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
			{
				MQTT_DBG_ERR("[%s] Failed to establish tls-sess(0x%x)\n",
							 __func__, -ret);
				return MOSQ_ERR_TLS_HANDSHAKE;
			}
		}

		// Init timeout
		mosq->socket_timeout = 100; //100 msec

		ret = mosquitto__socket_connect_tls_verify(mosq);
		if (ret)
		{
			MQTT_DBG_ERR("[%s] Failed to verify tls-sess(%d)\n",
						 __func__, ret);

			mosq->socket_timeout_required = FALSE;

			//Init first timeout
			mosq->socket_timeout = 0;
			
			return ret;
		}
		
		mosq->socket_timeout_required = FALSE;

		//Init first timeout
		mosq->socket_timeout = 0;

	}

	if (dpm_mode_is_enabled())
	{
		UINT status = _mosquitto__socket_save_tls(mosq);
		if (status)
		{
			MQTT_DBG_ERR("[%s] Failed to save tls-sess(0x%02x)\n", __func__, status);
		}
		else
		{
			mosq->rtm_saved = 1;
		}
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_socket_connect_step3(struct mosquitto *mosq)
{
	int ret = 0;

	if (mosq->tls_insecure)
	{
		ret = _mosquitto_tls_init(mosq);
		if (ret)
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to init TLS(%d)\n" CLEAR_COLOR,
						 __func__, ret);
			return ret;
		}

		ret = _mosquitto_tls_set(mosq);
		if (ret)
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to set TLS(%d)\n"  CLEAR_COLOR,
						 __func__, ret);
			_mosquitto_tls_deinit(mosq);
			return ret;
		}

		ret = mosquitto__socket_connect_tls(mosq);
		if (ret)
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to connect tls-sess(%d)\n" CLEAR_COLOR,
						 __func__, ret);

			_mosquitto_tls_deinit(mosq);

			return ret;
		}
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_socket_connect(struct mosquitto *mosq, const char *host, uint16_t port)
{
	int rc;

	if (!mosq || !host || !port)
	{
		return MOSQ_ERR_INVAL;
	}

	rc = _mosquitto_try_connect(mosq, host, port);

	if (rc > 0)
	{
		return rc;
	}

	rc = _mosquitto_socket_connect_step3(mosq);

	return rc;
}

int _mosquitto_read_byte(struct _mosquitto_packet *packet, uint8_t *byte)
{
	if (packet->pos + 1 > packet->remaining_length)
	{
		return MOSQ_ERR_PROTOCOL;
	}

	*byte = packet->payload[packet->pos];
	packet->pos++;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_byte(struct _mosquitto_packet *packet, uint8_t byte)
{
	packet->payload[packet->pos] = byte;
	packet->pos++;
}

int _mosquitto_read_bytes(struct _mosquitto_packet *packet, void *bytes, uint32_t count)
{
	if (packet->pos + count > packet->remaining_length)
	{
		return MOSQ_ERR_PROTOCOL;
	}

	memcpy(bytes, &(packet->payload[packet->pos]), count);
	packet->pos += count;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_bytes(struct _mosquitto_packet *packet, const void *bytes, uint32_t count)
{
	memcpy(&(packet->payload[packet->pos]), bytes, count);
	packet->pos += count;
}

int _mosquitto_read_string(struct _mosquitto_packet *packet, char **str)
{
	uint16_t len;
	int rc;

	rc = _mosquitto_read_uint16(packet, &len);
	if (rc)
	{
		return rc;
	}

	if (packet->pos + len > packet->remaining_length)
	{
		return MOSQ_ERR_PROTOCOL;
	}

	*str = (char *)_mosquitto_malloc(len + 1);
	if (*str)
	{
		memcpy(*str, &(packet->payload[packet->pos]), len);
		(*str)[len] = '\0';
		packet->pos += len;
	}
	else
	{
		return MOSQ_ERR_NOMEM;
	}

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_string(struct _mosquitto_packet *packet, const char *str, uint16_t length)
{
	_mosquitto_write_uint16(packet, length);
	_mosquitto_write_bytes(packet, str, length);
}

int _mosquitto_read_uint16(struct _mosquitto_packet *packet, uint16_t *word)
{
	uint8_t msb, lsb;

	if (packet->pos + 2 > packet->remaining_length)
	{
		return MOSQ_ERR_PROTOCOL;
	}

	msb = packet->payload[packet->pos];
	packet->pos++;
	lsb = packet->payload[packet->pos];
	packet->pos++;

	*word = (uint16_t)((msb << 8) + lsb);

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_uint16(struct _mosquitto_packet *packet, uint16_t word)
{
	_mosquitto_write_byte(packet, MOSQ_MSB(word));
	_mosquitto_write_byte(packet, MOSQ_LSB(word));
}

int _mosquitto_net_read(struct mosquitto *mosq, void *buf, size_t count)
{
	int ret = 0;

	if (mosq->tls_insecure)
	{
		mosq->socket_timeout_required = FALSE;
		ret = mbedtls_ssl_read(mosq->ssl_ctx, buf, count);

		if (ret <= 0)
		{
			if ((ret == MBEDTLS_ERR_SSL_WANT_WRITE)
					|| (ret == MBEDTLS_ERR_SSL_TIMEOUT))
			{
				ret = 0;
			}
			else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
			{
				;
			}
			else
			{
				MQTT_DBG_ERR("Failed to read ssl-pkt(0x%x)\n", -ret);
			}
		}

		if (dpm_mode_is_enabled() && mosq->rtm_saved && ret >= 0)
		{
			UINT dpm_status = _mosquitto__socket_save_tls(mosq);

			if (dpm_status)
			{
				MQTT_DBG_ERR("Failed to save tls-sess(0x%02x)\n", dpm_status);

			}
		}

		return ret;
	}
	else
	{
		return read(mosq->sock, buf, count);
	}
}

int _mosquitto_net_write(struct mosquitto *mosq, void *buf, size_t count)
{
	int 	ret = (int)count;

	if (mosq->tls_insecure)
	{
		while ((ret = mbedtls_ssl_write(mosq->ssl_ctx, buf, count)) <= 0)
		{
			if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
			{
				MQTT_DBG_ERR("[%s] Failed to write ssl-pkt(0x%x)\n", __func__, -ret);
				break;
			}
		}

		if (ret < 0)
		{
			ret = -1;
		}

		if (dpm_mode_is_enabled() && mosq->rtm_saved && ret >= 0)
		{
			UINT dpm_status = _mosquitto__socket_save_tls(mosq);

			if (dpm_status)
			{
				MQTT_DBG_ERR("[%s] Failed to save tls-sess(0x%02x)\n",
						__func__, dpm_status);
			}
		}
		
		return ret;
	}
	else
	{
		return write(mosq->sock, buf, count);
	}

}

void _mosquitto_set_broker_conn_state(UINT8 state)
{
#if defined	( __MQTT_CONN_STATUS__ )
	mqttParams.broker_conn_done = (char)state;
#else
	DA16X_UNUSED_ARG(state);
#endif	// __MQTT_CONN_STATUS__
}

int _mosquitto_packet_read(struct mosquitto *mosq)
{
	uint8_t byte;
	int read_length;
	int rc = 0;

	if (!mosq)
	{
		return MOSQ_ERR_INVAL;
	}

	if(mosq->sock == INVALID_SOCKET) 
	{
		_mosquitto_set_broker_conn_state(FALSE);
		return MOSQ_ERR_NO_CONN;
	}

	if (mosq->state == mosq_cs_connect_pending)
	{
		return MOSQ_ERR_SUCCESS;
	}

	// read 'command' from header
	if (!mosq->in_packet.command)
	{
		// if data recv is a new mqtt packet, read 'command' (1 bytes) from header
		read_length = _mosquitto_net_read(mosq, &byte, 1);

		if (read_length == 1)
		{
			mosq->in_packet.command = byte;
		}
		else
		{
			if (read_length == 0)
			{
				_mosquitto_set_broker_conn_state(FALSE);
				return MOSQ_ERR_CONN_LOST;   /* EOF */
			}

			//  ... meaning read_length == -xxxx (error), checking errno
			switch(errno)
			{
				/*
					EAGAIN or EWOULDBLOCK is not assumed as 
					the socket is a blocking socket
				*/
				case ECONNRESET:
				case ENOTCONN:
					_mosquitto_set_broker_conn_state(FALSE);
					return MOSQ_ERR_CONN_LOST;
				default:
					// TCP reset is caught
					_mosquitto_set_broker_conn_state(FALSE);
					MQTT_DBG_INFO("[%s:%d] errno=%d \r\n", __func__, __LINE__, errno);
					return MOSQ_ERR_ERRNO;
			}
		}
	}

	// read 'remaining_length' from header
	if (mosq->in_packet.remaining_count <= 0)
	{
		do
		{
			read_length = _mosquitto_net_read(mosq, &byte, 1);

			if (read_length == 1)
			{
				mosq->in_packet.remaining_count--;

				if (mosq->in_packet.remaining_count < -4)
				{
					_mosquitto_set_broker_conn_state(FALSE);
					return MOSQ_ERR_PROTOCOL;
				}

				mosq->in_packet.remaining_length	+= (byte & 127) * mosq->in_packet.remaining_mult;
				mosq->in_packet.remaining_mult		*= 128;
			}
			else
			{
				if (read_length == 0)
				{
					_mosquitto_set_broker_conn_state(FALSE);
					return MOSQ_ERR_CONN_LOST;    /* EOF */
				}

				switch(errno)
				{
					/*
						EAGAIN or EWOULDBLOCK is not assumed 
						as the socket is a blocking socket
					*/
					case ECONNRESET:
					case ENOTCONN:
						_mosquitto_set_broker_conn_state(FALSE);
						return MOSQ_ERR_CONN_LOST;
					default:
						_mosquitto_set_broker_conn_state(FALSE);
						MQTT_DBG_INFO("[%s:%d] errno=%d \r\n", __func__, __LINE__, errno);
						return MOSQ_ERR_ERRNO;
				}
			}
		}
		while ((byte & 128) != 0);

		/* We have finished reading remaining_length, so make remaining_count
		 * positive. */
		mosq->in_packet.remaining_count = (int8_t)(mosq->in_packet.remaining_count * -1);

		if (mosq->in_packet.remaining_length > 0)
		{
			mosq->in_packet.payload = _mosquitto_malloc(mosq->in_packet.remaining_length * sizeof(uint8_t));
			if (!mosq->in_packet.payload)
			{
				return MOSQ_ERR_NOMEM;
			}

			mosq->in_packet.to_process = mosq->in_packet.remaining_length;
		}
	}

	// read 'payload'
	while (mosq->in_packet.to_process > 0)
	{
		read_length = _mosquitto_net_read(mosq, 
			&(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
		if (read_length > 0)
		{
			mosq->in_packet.to_process	= (uint32_t)((int)mosq->in_packet.to_process - read_length);
			mosq->in_packet.pos			= (uint32_t)((int)mosq->in_packet.pos + read_length);
		}
		else
		{
			// there's more data to read (.to-process > 0), but read_length is 0 / -(??)
			// this may be an error condition or not. Check errno for taking the next action
			
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if(mosq->in_packet.to_process > 1000)
				{
					ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
					
					mosq->last_msg_in = mosquitto_time();
					xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);
				}
				return MOSQ_ERR_SUCCESS;
			}
			else
			{			
				switch(errno)
				{
					case ECONNRESET:
					case ENOTCONN:
						_mosquitto_set_broker_conn_state(FALSE);
						return MOSQ_ERR_CONN_LOST;
					default:
						_mosquitto_set_broker_conn_state(FALSE);
						MQTT_DBG_INFO("[%s:%d] errno=%d \r\n", __func__, __LINE__, errno);
						return MOSQ_ERR_ERRNO;
				}
			}
		}
	}

	/* All data for this packet is read. */
	mosq->in_packet.pos = 0;
	rc = _mosquitto_packet_handle(mosq);

	/* Free data and reset values */
	_mosquitto_packet_cleanup(&mosq->in_packet);

	ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
	mosq->last_msg_in = mosquitto_time();
	xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);

	return rc;
}

int _mosquitto_packet_write(struct mosquitto *mosq)
{
	int write_length;
	struct _mosquitto_packet *packet;

	if (!mosq)
	{
		return MOSQ_ERR_INVAL;
	}

	ut_mqtt_sem_take_recur(&(mosq->current_out_packet_mutex), portMAX_DELAY);
	ut_mqtt_sem_take_recur(&(mosq->out_packet_mutex), portMAX_DELAY);

	if (mosq->out_packet && !mosq->current_out_packet)
	{
		mosq->current_out_packet	= mosq->out_packet;
		mosq->out_packet			= mosq->out_packet->next;

		if (!mosq->out_packet)
		{
			mosq->out_packet_last = NULL;
		}
	}

	xSemaphoreGiveRecursive(mosq->out_packet_mutex.mutex_info);

	// currently no place found to set mosq_cs_connect_pending
	if (mosq->state == mosq_cs_connect_pending)
	{
		xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);
		return MOSQ_ERR_SUCCESS;
	}

	while (mosq->current_out_packet)
	{
		packet = mosq->current_out_packet;

		while (packet->to_process > 0)
		{
			write_length = _mosquitto_net_write(mosq, &(packet->payload[packet->pos]), packet->to_process);

			if (write_length > 0)
			{
				packet->to_process	= (uint32_t)((int)packet->to_process - write_length);
				packet->pos	= (uint32_t)((int)packet->pos + write_length);
			}
			else if (write_length == -1)
			{
				xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);
				return MOSQ_ERR_NO_CONN;
			}
			else
			{
				xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);
				return MOSQ_ERR_SUCCESS;
			}
		}

		if (((packet->command) & 0xF6) == PUBLISH)
		{
			ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);

			if (mosq->on_publish)
			{
				/* This is a QoS=0 message */
				mosq->in_callback = true;
				mosq->on_publish(packet->mid);
				mosq->in_callback = false;
			}

			xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
		}
		else if (((packet->command) & 0xF0) == DISCONNECT)
		{
			ut_mqtt_sem_take_recur(&(mosq->out_packet_mutex), portMAX_DELAY);
			mosq->current_out_packet = mosq->out_packet;

			if (mosq->out_packet)
			{
				mosq->out_packet = mosq->out_packet->next;

				if (!mosq->out_packet)
				{
					mosq->out_packet_last = NULL;
				}
			}

			xSemaphoreGiveRecursive(mosq->out_packet_mutex.mutex_info);

			_mosquitto_packet_cleanup(packet);
			_mosquitto_free(packet);

			ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);
			
			mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
			
			xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);

			ut_mqtt_sem_take_recur(&(mosq->callback_mutex), portMAX_DELAY);

			if (mosq->on_disconnect)
			{
				mosq->in_callback = true;
				mosq->on_disconnect();
				mosq->in_callback = false;
			}

			xSemaphoreGiveRecursive(mosq->callback_mutex.mutex_info);
			xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);
			return MOSQ_ERR_SUCCESS;
		}

		ut_mqtt_sem_take_recur(&(mosq->out_packet_mutex), portMAX_DELAY);
		mosq->current_out_packet = mosq->out_packet;

		if (mosq->out_packet)
		{
			mosq->out_packet = mosq->out_packet->next;

			if (!mosq->out_packet)
			{
				mosq->out_packet_last = NULL;
			}
		}

		xSemaphoreGiveRecursive(mosq->out_packet_mutex.mutex_info);

		if (mosq->mqtt_pub_or_sub == CLIENT_SUB && mosq_sub)
		{
			_mosquitto_packet_cleanup(packet);
			_mosquitto_free(packet);
		}
		else
		{
			xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);
			return MOSQ_ERR_NOMEM;
		}

		ut_mqtt_sem_take_recur(&(mosq->msgtime_mutex), portMAX_DELAY);

		mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
		xSemaphoreGiveRecursive(mosq->msgtime_mutex.mutex_info);
	}

	xSemaphoreGiveRecursive(mosq->current_out_packet_mutex.mutex_info);

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_recv_func(void *ctx, unsigned char *buf, size_t len);
int _mosquitto_send_func(void *ctx, const unsigned char *buf, size_t len);

int _mosquitto_tls_init(struct mosquitto *mosq)
{
	if (mosq->tls_insecure)
	{
		mosq->ssl_ctx = _mosquitto_calloc(1, sizeof(mbedtls_ssl_context));

		if (mosq->ssl_ctx)
		{
			mbedtls_ssl_init(mosq->ssl_ctx);
		}
		else
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc ssl-ctx\n" CLEAR_COLOR, __func__);
			goto error;
		}

		mosq->ssl_conf = _mosquitto_calloc(1, sizeof(mbedtls_ssl_config));

		if (mosq->ssl_conf)
		{
			mbedtls_ssl_config_init(mosq->ssl_conf);
		}
		else
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc ssl-conf\n" CLEAR_COLOR, __func__);
			goto error;
		}

		mosq->ctr_drbg_ctx = _mosquitto_calloc(1, sizeof(mbedtls_ctr_drbg_context));

		if (mosq->ctr_drbg_ctx)
		{
			mbedtls_ctr_drbg_init(mosq->ctr_drbg_ctx);
		}
		else
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc ctr-drgb\n" CLEAR_COLOR, __func__);
			goto error;
		}

		mosq->entropy_ctx = _mosquitto_calloc(1, sizeof(mbedtls_entropy_context));

		if (mosq->entropy_ctx)
		{
			mbedtls_entropy_init(mosq->entropy_ctx);
		}
		else
		{
			MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc entropy\n" CLEAR_COLOR, __func__);
			goto error;
		}

		if (mosq->tls_ca_cert)
		{
			mosq->ca_cert_crt = _mosquitto_calloc(1, sizeof(mbedtls_x509_crt));

			if (!mosq->ca_cert_crt)
			{
				MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc ca-cert\n" CLEAR_COLOR, __func__);
				goto error;

			}
			else
			{
				mbedtls_x509_crt_init(mosq->ca_cert_crt);
			}
		}

		if (mosq->tls_cert)
		{
			mosq->cert_crt = _mosquitto_calloc(1, sizeof(mbedtls_x509_crt));

			if (!mosq->cert_crt)
			{
				MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc cert\n" CLEAR_COLOR, __func__);
				goto error;
			}
			else
			{
				mbedtls_x509_crt_init(mosq->cert_crt);
			}
		}

		if (mosq->tls_private_key)
		{
			mosq->pkey_ctx = _mosquitto_calloc(1, sizeof(mbedtls_pk_context));

			if (!mosq->pkey_ctx)
			{
				MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc pkey_ctx\n" CLEAR_COLOR, __func__);
				goto error;
			}
			else
			{
				mbedtls_pk_init(mosq->pkey_ctx);
			}

			mosq->pkey_alt_ctx = _mosquitto_calloc(1, sizeof(mbedtls_pk_context));

			if (!mosq->pkey_alt_ctx)
			{
				MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc pkey_alt_ctx\n" CLEAR_COLOR, __func__);
				goto error;
			}
			else
			{
				mbedtls_pk_init(mosq->pkey_alt_ctx);
			}
		}

		if (mosq->tls_dh_param)
		{
			mosq->dhm_ctx = _mosquitto_calloc(1, sizeof(mbedtls_dhm_context));

			if (!mosq->dhm_ctx)
			{
				MQTT_DBG_ERR(RED_COLOR "[%s] Failed to alloc dhm\n" CLEAR_COLOR, __func__);
				goto error;
			}
			else
			{
				mbedtls_dhm_init(mosq->dhm_ctx);
			}
		}

	}

	return MOSQ_ERR_SUCCESS;

error:
	_mosquitto_tls_deinit(mosq);

	return MOSQ_ERR_TLS;
}

int _mosquitto_tls_deinit(struct mosquitto *mosq)
{
	if (mosq->ssl_ctx)
	{
		mbedtls_ssl_free(mosq->ssl_ctx);
		_mosquitto_free(mosq->ssl_ctx);
		mosq->ssl_ctx = NULL;
	}

	if (mosq->ssl_conf)
	{
		mbedtls_ssl_config_free(mosq->ssl_conf);
		_mosquitto_free(mosq->ssl_conf);
		mosq->ssl_conf = NULL;
	}

	if (mosq->ctr_drbg_ctx)
	{
		mbedtls_ctr_drbg_free(mosq->ctr_drbg_ctx);
		_mosquitto_free(mosq->ctr_drbg_ctx);
		mosq->ctr_drbg_ctx = NULL;
	}

	if (mosq->entropy_ctx)
	{
		mbedtls_entropy_free(mosq->entropy_ctx);
		_mosquitto_free(mosq->entropy_ctx);
		mosq->entropy_ctx = NULL;
	}

	if (mosq->ca_cert_crt)
	{
		mbedtls_x509_crt_free(mosq->ca_cert_crt);
		_mosquitto_free(mosq->ca_cert_crt);
		mosq->ca_cert_crt = NULL;
	}

	if (mosq->cert_crt)
	{
		mbedtls_x509_crt_free(mosq->cert_crt);
		_mosquitto_free(mosq->cert_crt);
		mosq->cert_crt = NULL;
	}

	if (mosq->pkey_ctx)
	{
		mbedtls_pk_free(mosq->pkey_ctx);
		_mosquitto_free(mosq->pkey_ctx);
		mosq->pkey_ctx = NULL;
	}

	if (mosq->pkey_alt_ctx)
	{
		mbedtls_pk_free(mosq->pkey_alt_ctx);
		_mosquitto_free(mosq->pkey_alt_ctx);
		mosq->pkey_alt_ctx = NULL;
	}

	if (mosq->dhm_ctx)
	{
		mbedtls_dhm_free(mosq->dhm_ctx);
		_mosquitto_free(mosq->dhm_ctx);
		mosq->dhm_ctx= NULL;
	}

	if (dpm_mode_is_enabled())
	{
		_mosquitto__socket_clear_tls(mosq);
	}

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_tls_set(struct mosquitto *mosq)
{
	const char *pers = "mqtt";
	int ret = 0;
	UINT status = ERR_OK;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	unsigned int rsa_min_bitlen = mbedtls_x509_crt_profile_default.rsa_min_bitlen;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
	int preset = MBEDTLS_SSL_PRESET_DA16X;

#if defined(__SUPPORT_TLS_HW_CIPHER_SUITES__)
	preset = MBEDTLS_SSL_PRESET_DA16X_ONLY_HW;
#endif /* __SUPPORT_TLS_HW_CIPHER_SUITES__ */

#if	defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(4);
#endif

	ret = mbedtls_ssl_config_defaults(mosq->ssl_conf, MBEDTLS_SSL_IS_CLIENT,
									  MBEDTLS_SSL_TRANSPORT_STREAM, preset);

	if (ret != 0)
	{
		MQTT_DBG_ERR("[%s] Failed to set default ssl-conf(%d)\n", __func__, ret);
		return MOSQ_ERR_TLS;
	}

	if (mosq->ca_cert_crt && mosq->tls_ca_cert)
	{
		ret = mbedtls_x509_crt_parse(mosq->ca_cert_crt, (const unsigned char *)mosq->tls_ca_cert, mosq->tls_ca_cert_len);

		if (ret != 0)
		{
			MQTT_DBG_ERR("[%s] Failed to parse ca-cert(0x%x)\n", __func__, -ret);
			return MOSQ_ERR_TLS_INVALID_CERT;
		}

		mbedtls_ssl_conf_ca_chain(mosq->ssl_conf, mosq->ca_cert_crt, NULL);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
		for (mbedtls_x509_crt *cur = mosq->ca_cert_crt ; cur != NULL ; cur = cur->next)
		{
			if (mbedtls_pk_get_type(&cur->pk) == MBEDTLS_PK_RSA
					|| mbedtls_pk_get_type(&cur->pk) == MBEDTLS_PK_RSASSA_PSS)
			{
				if (mbedtls_pk_get_bitlen(&cur->pk) < rsa_min_bitlen)
				{
					rsa_min_bitlen = mbedtls_pk_get_bitlen(&cur->pk);
				}
			}
		}
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
	}

	if (dpm_mode_is_wakeup() == pdFALSE || 
	    (dpm_mode_is_wakeup() == pdTRUE && mosq->state != mosq_cs_connected_in_dpm)) {
	    
		if ((mosq->cert_crt && mosq->tls_cert) && (mosq->pkey_ctx && mosq->tls_private_key))
		{
			ret = mbedtls_x509_crt_parse(mosq->cert_crt, (const unsigned char *)mosq->tls_cert, mosq->tls_cert_len);

			if (ret != 0)
			{
				MQTT_DBG_ERR("[%s] Failed to parse cert(0x%x)\n", __func__, -ret);
				return MOSQ_ERR_TLS_INVALID_CERT;
			}

			ret = mbedtls_pk_parse_key(mosq->pkey_ctx,
									   (const unsigned char *)mosq->tls_private_key,
									   mosq->tls_private_key_len, NULL, 0);

			if (ret != 0)
			{
				MQTT_DBG_ERR("[%s] Failed to parse private-key(0x%x)\n",
							 __func__, -ret);
				return MOSQ_ERR_TLS_INVALID_CERT;
			}

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
			if (mbedtls_pk_get_type(mosq->pkey_ctx) == MBEDTLS_PK_RSA)
			{
				ret = mbedtls_pk_setup_rsa_alt(mosq->pkey_alt_ctx,
											   (void *)mbedtls_pk_rsa(*mosq->pkey_ctx),
											   _mosquitto_rsa_decrypt_func,
											   _mosquitto_rsa_sign_func,
											   _mosquitto_rsa_key_len_func);
				if ( ret != 0 )
				{
					MQTT_DBG_ERR("[%s] Failed to set RSA-alt(0x%x)\n", __func__, -ret);
					return MOSQ_ERR_TLS;
				}

				ret = mbedtls_ssl_conf_own_cert(mosq->ssl_conf,	mosq->cert_crt,	mosq->pkey_alt_ctx);
			} else {
				ret = mbedtls_ssl_conf_own_cert(mosq->ssl_conf, mosq->cert_crt,	mosq->pkey_ctx);
			}

#else
			ret = mbedtls_ssl_conf_own_cert(mosq->ssl_conf, mosq->cert_crt,	mosq->pkey_ctx);
#endif /* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */

			if (ret != 0)
			{
				MQTT_DBG_ERR("[%s] Failed to set own cert & private-key(0x%x)\n", __func__, -ret);
				return MOSQ_ERR_TLS;
			}

#if defined(MBEDTLS_X509_CRT_PARSE_C)

			for (mbedtls_x509_crt *cur = mosq->cert_crt ; cur != NULL ; cur = cur->next)
			{
				if (mbedtls_pk_get_type(&cur->pk) == MBEDTLS_PK_RSA	|| mbedtls_pk_get_type(&cur->pk) == MBEDTLS_PK_RSASSA_PSS)
				{
					if (mbedtls_pk_get_bitlen(&cur->pk) < rsa_min_bitlen)
					{
						rsa_min_bitlen = mbedtls_pk_get_bitlen(&cur->pk);
					}
				}
			}

#endif	/* MBEDTLS_X509_CRT_PARSE_C */
		}

        if (mosq->dhm_ctx && mosq->tls_dh_param)
        {
            ret = mbedtls_dhm_parse_dhm(mosq->dhm_ctx, (const unsigned char *)mosq->tls_dh_param, mosq->tls_dh_param_len);
            if (ret != 0)
            {
                MQTT_DBG_ERR("[%s] Failed to parse DH param(0x%x)\n", __func__, -ret);
                return MOSQ_ERR_TLS_INVALID_CERT;
            }

            ret = mbedtls_ssl_conf_dh_param_ctx(mosq->ssl_conf, mosq->dhm_ctx);
            if (ret != 0)
            {
                MQTT_DBG_ERR(" [%s] Failed to set DH param(0x%x)\n", __func__, -ret);
                return MOSQ_ERR_TLS;
            }
        }
	}

	ret = mbedtls_ctr_drbg_seed(mosq->ctr_drbg_ctx,	mbedtls_entropy_func,
								mosq->entropy_ctx, (const unsigned char *)pers, strlen(pers));

	if (ret != 0)
	{
		MQTT_DBG_ERR("[%s] Faield to set ctr drbg seed(0x%x)\n", __func__, -ret);
		return MOSQ_ERR_TLS;
	}

	mbedtls_ssl_conf_rng(mosq->ssl_conf, mbedtls_ctr_drbg_random, mosq->ctr_drbg_ctx);

	mbedtls_ssl_conf_dbg(mosq->ssl_conf, _mosquitto_secure_debug, NULL);

	mbedtls_ssl_conf_authmode(mosq->ssl_conf, mosq->tls_auth_mode);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (mbedtls_x509_crt_profile_default.rsa_min_bitlen != rsa_min_bitlen)
	{
		memcpy(&mosq->crt_profile, &mbedtls_x509_crt_profile_default, sizeof(mbedtls_x509_crt_profile));

		mosq->crt_profile.rsa_min_bitlen = rsa_min_bitlen;

		mbedtls_ssl_conf_cert_profile(mosq->ssl_conf, &mosq->crt_profile);
	}
#endif // (MBEDTLS_X509_CRT_PARSE_C)

	ret = mbedtls_ssl_conf_content_len(mosq->ssl_conf, mosq->tls_incoming_size, mosq->tls_outgoing_size);
	if (ret != 0)
	{
		MQTT_DBG_ERR(" [%s] Failed to set content buf-len(0x%x)\n", __func__, -ret);
		return MOSQ_ERR_TLS;
	}

#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
	if (mosq->tls_alpn_count && mosq->tls_alpn[0])
	{
		ret = mbedtls_ssl_conf_alpn_protocols(mosq->ssl_conf, (const char **)mosq->tls_alpn);
		if (ret)
		{
			MQTT_DBG_ERR(" [%s] Failed to set ALPN(0x%x)\n", __func__, -ret);
			return MOSQ_ERR_TLS;
		}
	}

	if (mosq->tls_sni)
	{
		ret = mbedtls_ssl_set_hostname(mosq->ssl_ctx, (const char *)mosq->tls_sni);
		if (ret)
		{
			MQTT_DBG_ERR(" [%s] Failed to set host name(0x%x)\n", __func__, -ret);
			return MOSQ_ERR_TLS;
		}
	}

	if (mosq->tls_csuits)
	{
		mbedtls_ssl_conf_ciphersuites(mosq->ssl_conf,
						    (const int *)mosq->tls_csuits);
	}
#endif // __MQTT_TLS_OPTIONAL_CONFIG__

	ret = mbedtls_ssl_setup(mosq->ssl_ctx, mosq->ssl_conf);
	if (ret != 0)
	{
		MQTT_DBG_ERR(" [%s] Failed to set ssl-conf(0x%x)\n", __func__, -ret);
		return MOSQ_ERR_TLS;
	}

	if (dpm_mode_is_enabled())
	{
		status = _mosquitto__socket_restore_tls(mosq);

		if (status)
		{
			MQTT_DBG_ERR("[%s] Failed to restore ssl(0x%02x)\n", __func__);
			return MOSQ_ERR_TLS;
		}
		else
		{
			mosq->rtm_saved = 1;
		}
	}

	mbedtls_ssl_set_bio(mosq->ssl_ctx, (void *)mosq,
						_mosquitto_send_func, _mosquitto_recv_func, NULL);

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_send_func(void *ctx, const unsigned char *buf, size_t len)
{
	int ret = 0;
	struct mosquitto *mosq = (struct mosquitto *)ctx;

	if (mosq == NULL || mosq->sock < 0)
	{
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;
	}

    if (chk_dpm_pdown_start() > WAIT_DPM_SLEEP) {
        MQTT_DBG_PRINT("[%s] DPM Power-Down started! \n", __func__);
        return ret;
    }
    
	ret = (int) write( mosq->sock, buf, len);

	if( ret < 0 )
	{
		//if( net_would_block( ctx ) != 0 )
			//return( MBEDTLS_ERR_SSL_WANT_WRITE );

		if( errno == EPIPE || errno == ECONNRESET )
			return( MBEDTLS_ERR_NET_CONN_RESET );

		if( errno == EINTR )
			return( MBEDTLS_ERR_SSL_WANT_WRITE );

		return( MBEDTLS_ERR_NET_SEND_FAILED );
	}

	return( ret );
	
}

UINT _mosquitto_send_packet(struct mosquitto *mosq, char* buf, size_t buf_len, ULONG wait_option)
{
	int status = ERR_OK;
	size_t len= 0;
	ULONG prev_timeout = 0; // in sec

	if (wait_option != portMAX_DELAY)
	{
		ut_sock_opt_get_timeout(mosq->sock, &prev_timeout);
		ut_sock_opt_set_timeout(mosq->sock, wait_option, FALSE);
	}
	
	len = send(mosq->sock, buf, buf_len, 0);
	if (len <= 0)
	{
		MQTT_DBG_ERR("[%s] Failed to send data\n", __func__);
		return MOSQ_ERR_UNKNOWN;;
	}
	
	if (wait_option != portMAX_DELAY)
	{
		ut_sock_opt_set_timeout(mosq->sock, prev_timeout, FALSE);
	}

	if (status)
	{
		MQTT_DBG_ERR("[%s] Failed to tx pkt (0x%02x)\n", __func__, status);
	}

	return (UINT)status;
}

int _mosquitto_recv_func(void *ctx, unsigned char *buf, size_t len)
{
	struct mosquitto *mosq = (struct mosquitto *)ctx;

	int ret;

	struct timeval local_timeout;
	fd_set readfds;
	int fdcount;
	int maxfd = 0;


	if (mosq == NULL || mosq->sock < 0)
	{
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;
	}

	if (mosq->socket_timeout_required == TRUE)
	{

		if(mosq->sock >= FD_SETSIZE)
		{
			MQTT_DBG_ERR(" [%s:%d] ERR: (mosq->sock >= FD_SETSIZE) \n", __func__, __LINE__);
			return MBEDTLS_ERR_NET_INVALID_CONTEXT;
		}

		FD_ZERO(&readfds);
		maxfd = mosq->sock;
		FD_SET(mosq->sock, &readfds);
		ut_sock_convert2tv((struct timeval *)(&local_timeout), (ULONG)mosq->socket_timeout);
		fdcount = select(maxfd+1, &readfds, NULL, NULL, &local_timeout);
		if(fdcount == -1)
		{
			if(errno == EINTR)
			{
				return( MBEDTLS_ERR_SSL_WANT_READ );
			}
			else
			{
				MQTT_DBG_ERR(" [%s:%d] ERR: errno=%d \r\n", __func__, __LINE__, errno);
				return( MBEDTLS_ERR_NET_RECV_FAILED );
			}		
		}
		else
		{
			if (fdcount == 0)
			{
				// select( ) timeout
				
				if (mosq->socket_timeout == 500) { // tls handshake timeout
					return( MBEDTLS_ERR_SSL_TIMEOUT );
				} else {
					return( MBEDTLS_ERR_SSL_WANT_READ );
				}
			}
			
			// there's incoming data in tcp layer
			
			if(mosq->sock != INVALID_SOCKET)
			{
				if(FD_ISSET(mosq->sock, &readfds))
				{
					// when !successful, clear event_callback()
					FD_ZERO(&readfds);
					FD_CLR(mosq->sock, &readfds);
					ut_sock_convert2tv((struct timeval *)(&local_timeout), (ULONG)1);
					select(maxfd+1, &readfds, NULL, NULL, &local_timeout);

					ret = (int) read( mosq->sock, buf, len );
					if( ret < 0 )
					{
						if( errno == EPIPE || errno == ECONNRESET )
							return( MBEDTLS_ERR_NET_CONN_RESET );
					
						if( errno == EINTR )
							return( MBEDTLS_ERR_SSL_WANT_READ );
					
						return( MBEDTLS_ERR_NET_RECV_FAILED );
					}
					
					return ret; 

				}
				else
				{
					// (fdcount > 0) + (socket != invalid) + readfds (x)
					MQTT_DBG_ERR(" [%s:%d] (fdcount > 0) + (socket != invalid) + readfds (x)  \n", 
									__func__, __LINE__);
					return( MBEDTLS_ERR_NET_RECV_FAILED );
				}
			}
			else
			{
				// INVALID_SOCKET
				MQTT_DBG_ERR(" [%s:%d] ERR: INVALID_SOCKET \n", __func__, __LINE__);
				return( MBEDTLS_ERR_NET_CONNECT_FAILED );
			}
		}

	}
	else
	{
		ret = (int) read( mosq->sock, buf, len );
		if( ret < 0 )
		{
			//if( net_would_block( ctx ) != 0 )
				//return( MBEDTLS_ERR_SSL_WANT_READ );
	
			if( errno == EPIPE || errno == ECONNRESET )
				return( MBEDTLS_ERR_NET_CONN_RESET );
	
			if( errno == EINTR )
				return( MBEDTLS_ERR_SSL_WANT_READ );
	
			return( MBEDTLS_ERR_NET_RECV_FAILED );
		}
	
		return ret;	
	}

}

UINT _mosquitto__socket_restore_tls(struct mosquitto *mosq)
{
	int ret = 0;

	const char *postfix = "_tls";
	char name[20] = {0x00,}; //REG_NAME_DPM_MAX_LEN

	if ((strlen(mosq->socket_name) == 0) || (strlen(mosq->socket_name) + strlen(postfix) > 20))
	{
		MQTT_DBG_ERR("[%s] Check socket name length (< %d)\n", __func__, 20);
		return 1; // NOT_CREATED
	}

	if (new_local_port_allocated == TRUE) {
		MQTT_DBG_INFO("[%s] new local port generated, skip restoring tls session \n", __func__);
		new_local_port_allocated = FALSE;
		return 0; // SUCCESS
	}
	sprintf(name, "%s_%s", mosq->socket_name, postfix);

	ret = get_tls_session(name, mosq->ssl_ctx);

	if (ret == ER_NOT_FOUND)
	{
		MQTT_DBG_INFO("[%s] Not found (%s)\n", __func__, name);
		return 0; // SUCCESS
	}
	else if (ret != 0)
	{
		MQTT_DBG_ERR("Failed to restore TLS sess(0x%x)\n", __func__, -ret);
		return 2;// NOT_SUCCESSFUL
	}

	return 0; // SUCCESS
}

UINT _mosquitto__socket_save_tls(struct mosquitto *mosq)
{
	int ret = 0;

	const char *postfix = "_tls";
	char name[20] = {0x00,}; //REG_NAME_DPM_MAX_LEN

	if ((strlen(mosq->socket_name) == 0) || (strlen(mosq->socket_name) + strlen(postfix) > 20))
	{
		MQTT_DBG_ERR("[%s] Check socket name length (< %d)\n", __func__, 20);
		return 1; // NOT_CREATED
	}

	sprintf(name, "%s_%s", mosq->socket_name, postfix);

	ret = set_tls_session(name, mosq->ssl_ctx);
	if (ret)
	{
		MQTT_DBG_ERR("[%s] Failed to save tls sess(0x%x)\n", __func__, -ret);
		return 2;// NOT_SUCCESSFUL
	}

	return 0; // NX_SUCCESS;
}

UINT _mosquitto__socket_clear_tls(struct mosquitto *mosq)
{
	int ret = 0;

	const char *postfix = "_tls";
	char name[20] = {0x00,}; //REG_NAME_DPM_MAX_LEN

	if (!mosq->rtm_saved)
	{
		return 1; // NOT_CREATED;
	}

	if ((strlen(mosq->socket_name) == 0) || (strlen(mosq->socket_name) + strlen(postfix) > 20))
	{
		MQTT_DBG_ERR("[%s] Check socket name length (< %d)\n", __func__, 20);
		return 1; // NOT_CREATED;
	}

	sprintf(name, "%s_%s", mosq->socket_name, postfix);

	ret = clr_tls_session(name);

	if (ret)
	{
		MQTT_DBG_TEMP("[%s] Failed to restore tls-sess(0x%x)\n", __func__, -ret);
		return 2;// NOT_SUCCESSFUL
	}

	return 0; // SUCCESS;
}

#endif // (__SUPPORT_MQTT__)

/* EOF */
