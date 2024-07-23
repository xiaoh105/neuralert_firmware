/**
 ****************************************************************************************
 *
 * @file coap_server.c
 *
 * @brief CoAP Server functionality
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */
#include "coap_server.h"

#define	COAPS_PRINTF			PRINTF

#if defined (ENABLE_COAPS_DEBUG_INFO)
#define	COAPS_DEBUG_INFO(fmt, ...)	\
	COAPS_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPS_DEBUG_INFO(...)		do {} while (0)
#endif // (ENABLED_COAPS_DEBUG_INFO)

#if defined (ENABLE_COAPS_DEBUG_ERR)
#define	COAPS_DEBUG_ERR(fmt, ...)	\
	COAPS_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPS_DEBUG_ERR(...)		do {} while (0)
#endif // (ENABLED_COAPS_DEBUG_ERR)

#if defined (ENABLE_COAPS_DEBUG_TEMP)
#define	COAPS_DEBUG_TEMP(fmt, ...)	\
	COAPS_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPS_DEBUG_TEMP(...)		do {} while (0)
#endif // (ENABLED_COAPS_DEBUG_TEMP)

//CoAP server naiming
#define	COAP_SERVER_SOCK_NAME(prefix, name)			\
	sprintf(name, "%s_%s", prefix, "sock")

#define COAP_SERVER_SOCK_EVENT_NAME(prefix, name)	\
	sprintf(name, "%s_%s", prefix, "evt")

extern void da16x_secure_module_deinit(void);
extern void da16x_secure_module_init(void);

//internal function
int coap_server_init_config(coap_server_t *coap_server_ptr, char *name_ptr,
							size_t stack_size,
							const coap_endpoint_t *endpoint_ptr,
							const coap_server_secure_conf_t *secure_conf_ptr);
int coap_server_init_socket(coap_server_t *coap_server_ptr);
int coap_server_deinit_socket(coap_server_t *coap_server_ptr);
int coap_server_init_task(coap_server_t *coap_server_ptr);
int coap_server_deinit_task(coap_server_t *coap_server_ptr);
int coap_server_init_secure(coap_server_t *coap_server_ptr);
int coap_server_deinit_secure(coap_server_t *coap_server_ptr);

int coap_server_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
								 const unsigned char *input, unsigned char *output,
								 size_t output_max_len);
int coap_server_rsa_sign_func(void *ctx,
							  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
							  int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
							  const unsigned char *hash, unsigned char *sig);
size_t coap_server_rsa_key_len_func(void *ctx);
void coap_server_start_peer_timer(void *ctx, uint32_t int_ms, uint32_t fin_ms);
int coap_server_get_peer_timer_state(void *ctx);
int coap_server_setup_secure(coap_server_t *coap_server_ptr);
int coap_server_shutdown_peer_ssl(coap_server_peer_info_t *peer_ptr);
int coap_server_send_close_notify(coap_server_peer_info_t *peer_ptr);

int coap_server_recv(coap_server_t *coap_server_ptr,
					 coap_server_peer_info_t *peer_ptr,
					 unsigned long wait_option);
void coap_server_processor(void *params);
int coap_server_clear_peer_list(coap_server_t *coap_server_ptr);

void coap_server_print_peer_list(coap_server_t *coap_server_ptr);
coap_server_peer_info_t *coap_server_find_peer_in_ip_addr(
	coap_server_t *coap_server_ptr,
	struct sockaddr_in *addr);
coap_server_peer_info_t *coap_server_find_peer_in_remainder(
	coap_server_t *coap_server_ptr);
int coap_server_add_peer(coap_server_t *coap_server_ptr,
						 coap_server_peer_info_t *peer_ptr);
int coap_server_delete_peer_in_ip_addr(coap_server_t *coap_server_ptr,
									   struct sockaddr_in *addr);
int coap_server_delete_peer_in_timeout(coap_server_t *coap_server_ptr);
int coap_server_update_peer_timeout(coap_server_t *coap_server_ptr,
									unsigned long timeout);
coap_server_peer_info_t *coap_server_create_peer(coap_server_t *coap_server_ptr,
		struct sockaddr_in *ip_addr);
int coap_server_delete_peer(coap_server_peer_info_t **peer_ptr);
int coap_server_init_peer_secure(coap_server_t *coap_server_ptr,
								 coap_server_peer_info_t *peer_ptr);
int coap_server_deinit_peer_secure(coap_server_peer_info_t *peer_ptr);

int coap_server_handle_request(coap_server_t *coap_server_ptr,
							   coap_rw_buffer_t *scratch,
							   const coap_packet_t *in,
							   coap_packet_t *out,
							   uint8_t *is_separate);
int coap_server_send_ack(coap_server_t *coap_server_ptr,
						 coap_server_peer_info_t *peer_ptr,
						 uint8_t msgId[2]);

int coap_server_start(coap_server_t *coap_server_ptr, char *name_ptr,
					  size_t stack_size,
					  const coap_endpoint_t *endpoint_ptr)
{
	if (dpm_mode_is_enabled()) {
		COAPS_DEBUG_ERR("Not supported in DPM mode\r\n");
		return DA_APP_NOT_SUPPORTED;
	}

	return coap_server_init_config(coap_server_ptr, name_ptr, stack_size,
								   endpoint_ptr, NULL);
}

int coaps_server_start(coap_server_t *coap_server_ptr, char *name_ptr,
					   size_t stack_size,
					   const coap_endpoint_t *endpoint_ptr,
					   const coap_server_secure_conf_t *secure_conf_ptr)
{
	return coap_server_init_config(coap_server_ptr, name_ptr, stack_size,
								   endpoint_ptr, secure_conf_ptr);
}

int coap_server_stop(coap_server_t *coap_server_ptr)
{
	int ret = DA_APP_SUCCESS;

	const int max_retry_count = 5;
	int retry_count = 0;


	if (!coap_server_ptr) {
		return DA_APP_INVALID_PARAMETERS;
	}

	if (coap_server_ptr->state != COAP_SERVER_STATUS_RUNNING) {
		return DA_APP_SUCCESS;
	}

	while (coap_server_ptr->state != COAP_SERVER_STATUS_RUNNING) {
		vTaskDelay(COAP_SERVER_DEF_TIMEOUT);

		retry_count++;

		if (retry_count > max_retry_count) {
			break;
		}
	}

	ret = coap_server_deinit_socket(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to deinit socket(%d)\r\n", ret);
	}

	ret = coap_server_deinit_task(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to deinit task(%d)\r\n", ret);
	}

	ret = coap_server_clear_peer_list(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to clear peer list(%d)\r\n", ret);
	}

	ret = coap_server_deinit_secure(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to deinit secure config(%d)\r\n", ret);
	}

	return ret;
}

int coap_server_init_config(coap_server_t *coap_server_ptr, char *name_ptr,
							size_t stack_size,
							const coap_endpoint_t *endpoint_ptr,
							const coap_server_secure_conf_t *secure_conf_ptr)
{
	int ret = 0;

	if (!coap_server_ptr) {
		return DA_APP_INVALID_PARAMETERS;
	}

	if (coap_server_ptr->state == COAP_SERVER_STATUS_RUNNING) {
		return DA_APP_ALREADY_ENABLED;
	}

	memset(coap_server_ptr, 0x00, sizeof(coap_server_t));

	coap_server_ptr->state = COAP_SERVER_STATUS_READY;

	if (secure_conf_ptr == NULL) {
		coap_server_ptr->port = COAP_PORT;
		coap_server_ptr->secure_mode = pdFALSE;
		coap_server_ptr->secure_conf_ptr = NULL;
	} else {
		coap_server_ptr->port = COAPS_PORT;
		coap_server_ptr->secure_mode = pdTRUE;
		coap_server_ptr->secure_conf_ptr = secure_conf_ptr;

		if (secure_conf_ptr->authmode) {
			coap_server_ptr->authmode = secure_conf_ptr->authmode;
		} else {
			coap_server_ptr->authmode = MBEDTLS_SSL_VERIFY_NONE;
		}

		if (secure_conf_ptr->fragment) {
			coap_server_ptr->fragment = secure_conf_ptr->fragment;
		} else {
			coap_server_ptr->fragment = 0;
		}

		if (secure_conf_ptr->handshake_min_timeout) {
			coap_server_ptr->handshake_min_timeout = secure_conf_ptr->handshake_min_timeout;
		} else {
			coap_server_ptr->handshake_min_timeout = COAP_SERVER_HANDSHAKE_MIN_TIMEOUT * 10;
		}

		if (secure_conf_ptr->handshake_max_timeout) {
			coap_server_ptr->handshake_max_timeout = secure_conf_ptr->handshake_max_timeout;
		} else {
			coap_server_ptr->handshake_max_timeout = COAP_SERVER_HANDSHAKE_MAX_TIMEOUT * 10;
		}

		if (secure_conf_ptr->read_timeout) {
			coap_server_ptr->read_timeout = secure_conf_ptr->read_timeout;
		} else {
			coap_server_ptr->read_timeout = COAP_SERVER_DEF_TIMEOUT * 10;
		}

		ret = coap_server_init_secure(coap_server_ptr);
		if (ret) {
			COAPS_DEBUG_ERR("Failed to init secure mode(%d)\r\n", ret);
			return ret;
		}

		ret = coap_server_setup_secure(coap_server_ptr);
		if (ret) {
			COAPS_DEBUG_ERR("Failed to setup secure mode(0x%x)\r\n", -ret);

			coap_server_deinit_secure(coap_server_ptr);

			return ret;
		}
	}

	coap_server_ptr->name = name_ptr;
	coap_server_ptr->stack_size = stack_size;

	coap_server_ptr->max_peer_count = COAP_SERVER_MAX_PEER_COUNT;
	coap_server_ptr->max_peer_activity_timeout =
		(COAP_SERVER_MAX_PEER_ACTIVITY_TIMEOUT * portTICK_PERIOD_MS * 1000);

	coap_server_ptr->peer_list = NULL;

	COAP_SERVER_SOCK_NAME(coap_server_ptr->name,
						  (char *)coap_server_ptr->socket_name);

	ret = coap_server_init_socket(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to init coap server socket(0x%x)\r\n", -ret);

		if (coap_server_ptr->secure_mode) {
			coap_server_deinit_secure(coap_server_ptr);
		}

		return ret;
	}

	ret = coap_server_init_task(coap_server_ptr);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to init coap server thread(%d)\r\n", ret);

		coap_server_deinit_socket(coap_server_ptr);

		if (coap_server_ptr->secure_mode) {
			coap_server_deinit_secure(coap_server_ptr);
		}

		return ret;
	}

	coap_server_ptr->endpoint_ptr = endpoint_ptr;

	return ret;
}

int coap_server_init_socket(coap_server_t *coap_server_ptr)
{
	int ret = DA_APP_SUCCESS;

	char str_local_port[32] = {0x00,};

	sprintf(str_local_port, "%d", coap_server_ptr->port);

	mbedtls_net_init(&coap_server_ptr->listen_ctx);

	ret = mbedtls_net_bind(&coap_server_ptr->listen_ctx,
						   NULL,
						   str_local_port,
						   MBEDTLS_NET_PROTO_UDP);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to bind coap server socket(0x%x)\r\n", -ret);
		return ret;
	}

	return ret;
}

int coap_server_deinit_socket(coap_server_t *coap_server_ptr)
{
	mbedtls_net_free(&coap_server_ptr->listen_ctx);

	return DA_APP_SUCCESS;
}

int coap_server_init_task(coap_server_t *coap_server_ptr)
{
	int ret = 0;

	ret = xTaskCreate(coap_server_processor,
					  coap_server_ptr->name,
					  (configSTACK_DEPTH_TYPE)(coap_server_ptr->stack_size),
					  (void *)coap_server_ptr,
					  OS_TASK_PRIORITY_USER,
					  &coap_server_ptr->task_handler);
	if (ret != pdPASS) {
		COAPS_DEBUG_ERR("Failed to create coap sever task(%d)\r\n", ret);
		return DA_APP_NOT_CREATED;
	}

	return DA_APP_SUCCESS;
}

int coap_server_deinit_task(coap_server_t *coap_server_ptr)
{
	if (coap_server_ptr->task_handler) {
		vTaskDelete(coap_server_ptr->task_handler);
		coap_server_ptr->task_handler = NULL;
	}

	return DA_APP_SUCCESS;
}

int coap_server_init_secure(coap_server_t *coap_server_ptr)
{
	const coap_server_secure_conf_t *secure_conf_ptr =
		coap_server_ptr->secure_conf_ptr;

	da16x_secure_module_init();

	if (!coap_server_ptr->ssl_conf) {
		coap_server_ptr->ssl_conf = coap_calloc(1, sizeof(mbedtls_ssl_config));
		if (!coap_server_ptr->ssl_conf) {
			COAPS_DEBUG_ERR("Failed to allocate ssl config\r\n");
			goto error;
		}

		mbedtls_ssl_config_init(coap_server_ptr->ssl_conf);
	}

	if (!coap_server_ptr->ctr_drbg_ctx) {
		coap_server_ptr->ctr_drbg_ctx = coap_calloc(1,
										sizeof(mbedtls_ctr_drbg_context));
		if (!coap_server_ptr->ctr_drbg_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate ctr-drbg\r\n");
			goto error;
		}

		mbedtls_ctr_drbg_init(coap_server_ptr->ctr_drbg_ctx);
	}

	if (!coap_server_ptr->entropy_ctx) {
		coap_server_ptr->entropy_ctx = coap_calloc(1, sizeof(mbedtls_entropy_context));
		if (!coap_server_ptr->entropy_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate entropy\r\n");
			goto error;
		}

		mbedtls_entropy_init(coap_server_ptr->entropy_ctx);
	}

	if (!coap_server_ptr->ca_cert_crt && (secure_conf_ptr->ca_cert_len > 0)) {
		coap_server_ptr->ca_cert_crt = coap_calloc(1, sizeof(mbedtls_x509_crt));
		if (!coap_server_ptr->ca_cert_crt) {
			COAPS_DEBUG_ERR("Failed to allocate ca cert\r\n");
			goto error;
		}

		mbedtls_x509_crt_init(coap_server_ptr->ca_cert_crt);
	}

	if (!coap_server_ptr->cert_crt && (secure_conf_ptr->cert_len > 0)) {
		coap_server_ptr->cert_crt = coap_calloc(1, sizeof(mbedtls_x509_crt));
		if (!coap_server_ptr->cert_crt) {
			COAPS_DEBUG_ERR("Failed to allocate cert\r\n");
			goto error;
		}

		mbedtls_x509_crt_init(coap_server_ptr->cert_crt);
	}

	if (!coap_server_ptr->pkey_ctx && (secure_conf_ptr->pkey_len > 0)) {
		coap_server_ptr->pkey_ctx = coap_calloc(1, sizeof(mbedtls_pk_context));
		if (!coap_server_ptr->pkey_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate pkey\r\n");
			goto error;
		}

		mbedtls_pk_init(coap_server_ptr->pkey_ctx);
	}

	if (!coap_server_ptr->pkey_alt_ctx && (secure_conf_ptr->pkey_len > 0)) {
		coap_server_ptr->pkey_alt_ctx = coap_calloc(1, sizeof(mbedtls_pk_context));
		if (!coap_server_ptr->pkey_alt_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate pkey_alt\r\n");
			goto error;
		}

		mbedtls_pk_init(coap_server_ptr->pkey_alt_ctx);
	}

	if (!coap_server_ptr->cookie_ctx) {
		coap_server_ptr->cookie_ctx = coap_calloc(1, sizeof(mbedtls_ssl_cookie_ctx));
		if (!coap_server_ptr->cookie_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate cookie context\r\n");
			goto error;
		}

		mbedtls_ssl_cookie_init(coap_server_ptr->cookie_ctx);
	}

	return DA_APP_SUCCESS;

error:

	coap_server_deinit_secure(coap_server_ptr);

	return DA_APP_NOT_CREATED;
}

int coap_server_deinit_secure(coap_server_t *coap_server_ptr)
{
	if (coap_server_ptr->ssl_conf) {
		mbedtls_ssl_config_free(coap_server_ptr->ssl_conf);
		coap_free(coap_server_ptr->ssl_conf);
	}

	if (coap_server_ptr->ctr_drbg_ctx) {
		mbedtls_ctr_drbg_free(coap_server_ptr->ctr_drbg_ctx);
		coap_free(coap_server_ptr->ctr_drbg_ctx);
	}

	if (coap_server_ptr->entropy_ctx) {
		mbedtls_entropy_free(coap_server_ptr->entropy_ctx);
		coap_free(coap_server_ptr->entropy_ctx);
	}

	if (coap_server_ptr->ca_cert_crt) {
		mbedtls_x509_crt_free(coap_server_ptr->ca_cert_crt);
		coap_free(coap_server_ptr->ca_cert_crt);
	}

	if (coap_server_ptr->cert_crt) {
		mbedtls_x509_crt_free(coap_server_ptr->cert_crt);
		coap_free(coap_server_ptr->cert_crt);
	}

	if (coap_server_ptr->pkey_ctx) {
		mbedtls_pk_free(coap_server_ptr->pkey_ctx);
		coap_free(coap_server_ptr->pkey_ctx);
	}

	if (coap_server_ptr->pkey_alt_ctx) {
		mbedtls_pk_free(coap_server_ptr->pkey_alt_ctx);
		coap_free(coap_server_ptr->pkey_alt_ctx);
	}

	if (coap_server_ptr->cookie_ctx) {
		mbedtls_ssl_cookie_free(coap_server_ptr->cookie_ctx);
		coap_free(coap_server_ptr->cookie_ctx);
	}

	coap_server_ptr->ssl_conf = NULL;
	coap_server_ptr->ctr_drbg_ctx = NULL;
	coap_server_ptr->entropy_ctx = NULL;
	coap_server_ptr->ca_cert_crt = NULL;
	coap_server_ptr->cert_crt = NULL;
	coap_server_ptr->pkey_ctx = NULL;
	coap_server_ptr->pkey_alt_ctx = NULL;
	coap_server_ptr->cookie_ctx = NULL;

	da16x_secure_module_deinit();

	return DA_APP_SUCCESS;
}

int coap_server_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
								 const unsigned char *input, unsigned char *output,
								 size_t output_max_len)
{
	return mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *) ctx, NULL, NULL, mode,
									 olen,
									 input, output, output_max_len);
}

int coap_server_rsa_sign_func(void *ctx,
							  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
							  int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
							  const unsigned char *hash, unsigned char *sig)
{
	return mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *)ctx, f_rng, p_rng, mode,
								  md_alg, hashlen, hash, sig);
}

size_t coap_server_rsa_key_len_func(void *ctx)
{
	return ((const mbedtls_rsa_context *)ctx)->len;
}

void coap_server_start_peer_timer(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
	coap_timer_t *timer_ptr = (coap_timer_t *)ctx;

	if (!timer_ptr) {
		COAPS_DEBUG_ERR("[%s] Invaild paramter\n", __func__);
		return ;
	}

	timer_ptr->int_ms = int_ms;
	timer_ptr->fin_ms = fin_ms;

	if (fin_ms != 0) {
		timer_ptr->snapshot = xTaskGetTickCount();
	}

	return ;
}

#define CANCELLED       -1
#define NO_EXPIRY       0
#define INT_EXPIRY      1
#define FIN_EXPIRY      2

int coap_server_get_peer_timer_state(void *ctx)
{
	coap_timer_t *timer_ptr = (coap_timer_t *)ctx;
	TickType_t elapsed = 0;

	if (!timer_ptr) {
		return CANCELLED;
	}

	if(timer_ptr->fin_ms == 0) {
		return CANCELLED;
	}

	elapsed = xTaskGetTickCount() - timer_ptr->snapshot;

	COAPS_DEBUG_TEMP("fin_ms(%d), int_ms(%d), snapshot(%d), "
					 "elapsed(%d), xTaskGetTickCount(%d)\n",
					 timer_ptr->fin_ms, timer_ptr->int_ms, timer_ptr->snapshot,
					 elapsed, xTaskGetTickCount());

	if (elapsed >= pdMS_TO_TICKS(timer_ptr->fin_ms)) {
		return FIN_EXPIRY;
	}

	if(elapsed >= pdMS_TO_TICKS(timer_ptr->int_ms)) {
		return INT_EXPIRY;
	}

	return NO_EXPIRY;
}

int coap_server_setup_secure(coap_server_t *coap_server_ptr)
{
	const coap_server_secure_conf_t *secure_conf_ptr =
		coap_server_ptr->secure_conf_ptr;
	const char *pers = "coaps_server";
	int ret = 0;
	int preset = MBEDTLS_SSL_PRESET_DA16X;

#if defined(__SUPPORT_TLS_HW_CIPHER_SUITES__)
	preset = MBEDTLS_SSL_PRESET_DA16X_ONLY_HW;
#endif /* __SUPPORT_TLS_HW_CIPHER_SUITES__ */

	ret = mbedtls_ssl_config_defaults(coap_server_ptr->ssl_conf,
									  MBEDTLS_SSL_IS_SERVER,
									  MBEDTLS_SSL_TRANSPORT_DATAGRAM,
									  preset);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to set default ssl config(0x%x)\r\n", -ret);
		return ret;
	}

	if (secure_conf_ptr->ca_cert_len > 0) {
		ret = mbedtls_x509_crt_parse(coap_server_ptr->ca_cert_crt,
									 secure_conf_ptr->ca_cert,
									 secure_conf_ptr->ca_cert_len);
		if (ret) {
			COAPS_DEBUG_ERR("Failed to parse cert(0x%x)\r\n", -ret);
			return ret;
		}
	}

	if (secure_conf_ptr->cert_len > 0) {
		ret = mbedtls_x509_crt_parse(coap_server_ptr->cert_crt,
									 secure_conf_ptr->cert,
									 secure_conf_ptr->cert_len);
		if (ret) {
			COAPS_DEBUG_ERR("Failed to parse cert(0x%x)\r\n", -ret);
			return ret;
		}
	}

	if (secure_conf_ptr->pkey_len > 0) {
		ret = mbedtls_pk_parse_key(coap_server_ptr->pkey_ctx,
								   secure_conf_ptr->pkey,
								   secure_conf_ptr->pkey_len,
								   NULL, 0);
		if (ret) {
			COAPS_DEBUG_ERR("Failed to parse private key(0x%x)\r\n", -ret);
			return ret;
		}

		if (mbedtls_pk_get_type(coap_server_ptr->pkey_ctx) == MBEDTLS_PK_RSA) {
			ret = mbedtls_pk_setup_rsa_alt(coap_server_ptr->pkey_alt_ctx,
										   (void *)mbedtls_pk_rsa(*coap_server_ptr->pkey_ctx),
										   coap_server_rsa_decrypt_func,
										   coap_server_rsa_sign_func,
										   coap_server_rsa_key_len_func);
			if (ret) {
				COAPS_DEBUG_ERR("Failed to set rsa alt(0x%x)\r\n", -ret);
				return ret;
			}

			ret = mbedtls_ssl_conf_own_cert(coap_server_ptr->ssl_conf,
											coap_server_ptr->cert_crt,
											coap_server_ptr->pkey_alt_ctx);
			if (ret) {
				COAPS_DEBUG_ERR("Failed to set cert & private key(0x%x)\r\n", -ret);
				return ret;
			}
		} else {
			ret = mbedtls_ssl_conf_own_cert(coap_server_ptr->ssl_conf,
											coap_server_ptr->cert_crt,
											coap_server_ptr->pkey_ctx);
			if (ret) {
				COAPS_DEBUG_ERR("Failed to set cert & private key(0x%x)\r\n", -ret);
				return ret;
			}
		}
	}

	ret = mbedtls_ctr_drbg_seed(coap_server_ptr->ctr_drbg_ctx,
								mbedtls_entropy_func,
								coap_server_ptr->entropy_ctx,
								(const unsigned char *)pers,
								strlen(pers));
	if (ret) {
		COAPS_DEBUG_ERR("Faield to set ctr drbg seed(0x%x)\r\n", -ret);
		return ret;
	}

	mbedtls_ssl_conf_rng(coap_server_ptr->ssl_conf,
						 mbedtls_ctr_drbg_random,
						 coap_server_ptr->ctr_drbg_ctx);

	ret = mbedtls_ssl_cookie_setup(coap_server_ptr->cookie_ctx,
								   mbedtls_ctr_drbg_random,
								   coap_server_ptr->ctr_drbg_ctx);

	if (ret) {
		COAPS_DEBUG_ERR("Failed to set cookie context(0x%x)\r\n", -ret);
		return ret;
	}

	mbedtls_ssl_conf_dtls_cookies(coap_server_ptr->ssl_conf,
								  mbedtls_ssl_cookie_write,
								  mbedtls_ssl_cookie_check,
								  coap_server_ptr->cookie_ctx);

	mbedtls_ssl_conf_authmode(coap_server_ptr->ssl_conf, coap_server_ptr->authmode);
	mbedtls_ssl_conf_max_frag_len(coap_server_ptr->ssl_conf,
								  (unsigned char)(coap_server_ptr->fragment));

	//mbedtls_ssl_conf_dbg(coap_server_ptr->ssl_conf, dtls_server_sample_ssl_debug, NULL);
	mbedtls_ssl_conf_dtls_anti_replay(coap_server_ptr->ssl_conf,
									  MBEDTLS_SSL_ANTI_REPLAY_ENABLED);
	mbedtls_ssl_conf_read_timeout(coap_server_ptr->ssl_conf,
								  COAP_SERVER_DEF_TIMEOUT * 10);
	mbedtls_ssl_conf_handshake_timeout(coap_server_ptr->ssl_conf,
									   (uint32_t)(coap_server_ptr->handshake_min_timeout * 10),
									   (uint32_t)(coap_server_ptr->handshake_max_timeout * 10));

	return DA_APP_SUCCESS;
}

int coap_server_shutdown_peer_ssl(coap_server_peer_info_t *peer_ptr)
{
	int ret = 0;

	ret = mbedtls_ssl_session_reset(peer_ptr->ssl_ctx);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to reset session(0x%x)\r\n", -ret);
		return ret;
	}

	return DA_APP_SUCCESS;
}

int coap_server_send_close_notify(coap_server_peer_info_t *peer_ptr)
{
	int ret = 0;

	COAPS_DEBUG_TEMP("Send Close notify\r\n");

	ret = mbedtls_ssl_close_notify(peer_ptr->ssl_ctx);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to send ssl close notify(0x%x)\r\n", -ret);
		return ret;
	}

	return DA_APP_SUCCESS;
}

int coap_server_recv(coap_server_t *coap_server_ptr,
					 coap_server_peer_info_t *peer_ptr,
					 unsigned long wait_option)
{
	DA16X_UNUSED_ARG(coap_server_ptr);

	int ret = 0;

	unsigned char *buf = NULL;
	size_t buflen = (1024 * 2);

	buf = coap_calloc(buflen, sizeof(unsigned char));
	if (!buf) {
		COAPS_DEBUG_ERR("Failed to allocate memory to receive packet(%d)\r\n", buflen);
		return DA_APP_MALLOC_ERROR;
	}

	if (peer_ptr->secure_mode) {
		ret = mbedtls_ssl_read(peer_ptr->ssl_ctx, buf, buflen);
	} else {
		ret = mbedtls_net_recv_timeout(&peer_ptr->sock_ctx, buf, buflen, wait_option);
	}

	if (ret > 0) {
		if (peer_ptr->buflen) {
			COAPS_DEBUG_TEMP("To allcate additional buf to save received packet(%d+%d)\r\n",
							 peer_ptr->buflen, ret);

			unsigned char *new_buf = coap_calloc(peer_ptr->buflen + (size_t)ret, sizeof(char));
			if (!new_buf) {
				COAPS_DEBUG_ERR("Failed to allocate memory for receive buf(%ld)\r\n",
								peer_ptr->buflen + (size_t)ret);

				ret = DA_APP_MALLOC_ERROR;
				goto error;
			}

			memcpy(new_buf, peer_ptr->buf, peer_ptr->buflen);
			memcpy(new_buf + peer_ptr->buflen, buf, buflen);

			coap_free(peer_ptr->buf);

			peer_ptr->buf = new_buf;
			peer_ptr->buf += ret;
		} else {
			COAPS_DEBUG_TEMP("To allcate buf to save received packet(%d)\r\n", ret);

			peer_ptr->buf = coap_calloc((size_t)ret, sizeof(char));
			if (!peer_ptr->buf) {
				COAPS_DEBUG_ERR("Failed to allocate memory for receive buf(%ld)\r\n", ret);

				ret = DA_APP_MALLOC_ERROR;
				goto error;
			}

			memcpy(peer_ptr->buf, buf, (size_t)ret);
			peer_ptr->buflen = (size_t)ret;
		}

		coap_free(buf);

		COAPS_DEBUG_INFO("ip address(%d.%d.%d.%d:%d)\r\n"
						 "buflen(%ld)\r\n"
						 "secure_mode(%ld)\r\n"
						 "offset(%ld)\r\n"
						 "activity_timeout(%ld)\r\n",
						 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 24) & 0x0ff,
						 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 16) & 0x0ff,
						 (ntohl(peer_ptr->addr.sin_addr.s_addr) >>  8) & 0x0ff,
						 (ntohl(peer_ptr->addr.sin_addr.s_addr)      ) & 0x0ff,
						 ntohs(peer_ptr->addr.sin_port),
						 peer_ptr->buflen,
						 peer_ptr->secure_mode,
						 peer_ptr->offset,
						 peer_ptr->activity_timeout);

		return ret;
	} else {
		switch (ret) {
			case MBEDTLS_ERR_SSL_TIMEOUT:
				COAPS_DEBUG_INFO("Timeout\r\n");
				ret = DA_APP_NO_PACKET;
				break;
			case MBEDTLS_ERR_SSL_WANT_READ:
			case MBEDTLS_ERR_SSL_WANT_WRITE:
				COAPS_DEBUG_INFO("Need more data(0x%x)\r\n", -ret);
				ret = DA_APP_NO_PACKET;
				break;
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				COAPS_DEBUG_INFO("Connection was closed gracefully\r\n");
				break;
			case MBEDTLS_ERR_NET_CONN_RESET:
				COAPS_DEBUG_INFO("Connection was reset by peer\r\n");
				break;
			default:
				COAPS_DEBUG_INFO("Failed to recv data(0x%x)\r\n", -ret);
				break;
		}
	}

error:

	if (buf) {
		coap_free(buf);
		buf = NULL;
	}

	return ret;
}

int coap_server_send(coap_server_t *coap_server_ptr,
					 coap_server_peer_info_t *peer_ptr,
					 unsigned char *data, size_t len)
{
	DA16X_UNUSED_ARG(coap_server_ptr);

	int ret = 0;

	if (peer_ptr->secure_mode) {
		while ((ret = mbedtls_ssl_write(peer_ptr->ssl_ctx, data, len)) <= 0) {
			if ((ret != MBEDTLS_ERR_SSL_WANT_READ)
				&& (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
				return ret;
			}
		}
	} else {
		ret = mbedtls_net_send(&peer_ptr->sock_ctx, data, len);
	}

	if (ret <= 0) {
		COAPS_DEBUG_ERR("Failed to send packet(%d)\r\n", ret);
		return ret;
	}

	return DA_APP_SUCCESS;
}

//main processor
void coap_server_processor(void *params)
{
	coap_server_t *coap_server_ptr = (coap_server_t *)params;

	int ret = 0;
	unsigned int loop_endlessly = pdTRUE;
	unsigned long wait_option = COAP_SERVER_DEF_TIMEOUT;
	struct timeval timeout = {0, COAP_SERVER_DEF_TIMEOUT * 1000};

	unsigned char *send_buf = NULL;
	size_t send_buf_len = COAP_SERVER_SEND_BUF_SIZE;

	coap_packet_t coap_request;
	coap_packet_t coap_response;

	unsigned int idx = 0;

	//option value
	int opt_idx = 0;

	unsigned int obs_num = 0;
	//TODO: Not allowed observe request
	//unsigned int is_obs_option = pdFALSE;

	unsigned int max_age = 0;

	uint32_t block1_num = 0;
	uint8_t block1_more = 0;
	uint8_t block1_szx = 0;

	uint32_t block2_num = 0;
	uint8_t block2_more = 0;
	uint8_t block2_szx = 0;
	unsigned int is_block2_option = pdFALSE;

	if (!coap_server_ptr) {
		COAPS_DEBUG_ERR("Invalid pointer\r\n");
		goto end_of_task;
	}

	send_buf = coap_calloc(send_buf_len, sizeof(unsigned char));
	if (!send_buf) {
		COAPS_DEBUG_ERR("Failed to allocate memory for send buffer\r\n");
		goto end_of_task;
	}

	memset(&coap_request, 0x00, sizeof(coap_packet_t));
	memset(&coap_response, 0x00, sizeof(coap_packet_t));

	coap_server_ptr->state = COAP_SERVER_STATUS_RUNNING;

	COAPS_DEBUG_TEMP("Start CoAP Server Thread(%p)\r\n", coap_server_ptr);

	while (loop_endlessly) {
		coap_server_peer_info_t *peer_info = NULL;
		struct sockaddr_in peer_addr;
		socklen_t peer_addrlen = sizeof(peer_addr);

		char tmp_buf[1] = {0x00};
		char peer_ip_str[16] = {0x00,};
		size_t peer_ip_len = 0;

		COAPS_DEBUG_INFO("Waiting for CoAP request to check client ip info\r\n");

		setsockopt(coap_server_ptr->listen_ctx.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
				   sizeof(timeout));

		ret = recvfrom(coap_server_ptr->listen_ctx.fd, tmp_buf, sizeof(tmp_buf),
					   MSG_PEEK,
					   (struct sockaddr *)&peer_addr, &peer_addrlen);
		if (ret <= 0) {
			COAPS_DEBUG_INFO("No packet on listen socket(%d)\r\n", ret);

			//Check client socket
			for (peer_info = coap_server_ptr->peer_list
							 ; peer_info != NULL
				 ; peer_info = peer_info->next) {
				COAPS_DEBUG_INFO("Check client socket\r\n");

				setsockopt(peer_info->sock_ctx.fd,
						   SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

				ret = recvfrom(peer_info->sock_ctx.fd, tmp_buf, sizeof(tmp_buf), MSG_PEEK,
							   (struct sockaddr *)&peer_addr, &peer_addrlen);
				if (ret > 0) {
					break;
				}
			}

			//TODO: Check remainder

			if (peer_info) {
				COAPS_DEBUG_INFO("Received packet on client(%d.%d.%d.%d:%d)\r\n",
								 (ntohl(peer_info->addr.sin_addr.s_addr) >> 24) & 0x0ff,
								 (ntohl(peer_info->addr.sin_addr.s_addr) >> 16) & 0x0ff,
								 (ntohl(peer_info->addr.sin_addr.s_addr) >>  8) & 0x0ff,
								 (ntohl(peer_info->addr.sin_addr.s_addr)      ) & 0x0ff,
								 ntohs(peer_info->addr.sin_port));
			} else {
				COAPS_DEBUG_INFO("Not received packet\r\n");
				goto next;
			}
		}

		peer_info = coap_server_find_peer_in_ip_addr(coap_server_ptr, &peer_addr);
		if (!peer_info) {
			COAPS_DEBUG_INFO("New Peer\r\n");

			peer_info = coap_server_create_peer(coap_server_ptr, &peer_addr);
			if (!peer_info) {
				COAPS_DEBUG_ERR("Failed to allocate memory for new peer\r\n");
				goto cleanup;
			}

coaps_handshake_reset:

			mbedtls_net_free(&peer_info->sock_ctx);

			mbedtls_net_init(&peer_info->sock_ctx);

			if (coap_server_ptr->secure_mode) {
				mbedtls_ssl_session_reset(peer_info->ssl_ctx);
			}

			ret = mbedtls_net_accept(&coap_server_ptr->listen_ctx,
									 &peer_info->sock_ctx,
									 peer_ip_str,
									 sizeof(peer_ip_str),
									 &peer_ip_len);
			if (ret) {
				COAPS_DEBUG_ERR("Failed to accept new peer(0x%x)\r\n", -ret);
				coap_server_delete_peer(&peer_info);
				goto cleanup;
			}

			COAPS_DEBUG_INFO("peer_addr(%d.%d.%d.%d:%d)\r\n",
							 (ntohl(peer_info->addr.sin_addr.s_addr) >> 24) & 0x0ff,
							 (ntohl(peer_info->addr.sin_addr.s_addr) >> 16) & 0x0ff,
							 (ntohl(peer_info->addr.sin_addr.s_addr) >>  8) & 0x0ff,
							 (ntohl(peer_info->addr.sin_addr.s_addr)      ) & 0x0ff,
							 ntohs(peer_info->addr.sin_port));

			//CoAPs
			if (coap_server_ptr->secure_mode) {
				//set client ip address
				ret = mbedtls_ssl_set_client_transport_id(peer_info->ssl_ctx,
						(const unsigned char *)peer_ip_str,
						peer_ip_len);
				if (ret) {
					COAPS_DEBUG_ERR("Failed to set client transport id(0x%x)\r\n", -ret);
					coap_server_delete_peer(&peer_info);
					goto cleanup;
				}

				mbedtls_ssl_set_bio(peer_info->ssl_ctx,
									&peer_info->sock_ctx,
									mbedtls_net_send,
									mbedtls_net_recv,
									mbedtls_net_recv_timeout);

				COAPS_DEBUG_INFO("Performing the DTLS handshake\r\n");

				while ((ret = mbedtls_ssl_handshake(peer_info->ssl_ctx)) != 0) {
					if ((ret == MBEDTLS_ERR_SSL_WANT_READ)
						|| (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
						continue;
					}

					if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
						COAPS_DEBUG_INFO("hello verification requested\r\n");
						ret = 0;
						goto coaps_handshake_reset;
					} else {
						COAPS_DEBUG_INFO("Failed to do handshake(0x%x)\r\n", __func__, -ret);
						coap_server_delete_peer(&peer_info);
						goto cleanup;
					}
				}
			}

			//Add peer to peer_list
			ret = coap_server_add_peer(coap_server_ptr, peer_info);
			if (ret) {
				coap_server_delete_peer(&peer_info);
				goto cleanup;
			}
		}

		COAPS_DEBUG_INFO("Waiting for CoAP request\r\n");

		ret = coap_server_recv(coap_server_ptr, peer_info, wait_option);
		if (ret > 0) {
			//parse coap request
			ret = coap_parse_raw_data(&coap_request,
									  peer_info->buf + peer_info->offset,
									  peer_info->buflen - peer_info->offset);
			if (ret == 0) {
				if (coap_request.header.type == COAP_MSG_TYPE_ACK) {
					COAPS_DEBUG_INFO("Received CoAP Ack\r\n");
					goto next;
				}

				uint8_t is_separate = 0;

				//TODO: for what?!
				uint8_t scratch_raw[2] = {0x00,};
				coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};

				//handle the request packet
				coap_server_handle_request(coap_server_ptr,
										   &scratch_buf,
										   &coap_request,
										   &coap_response,
										   &is_separate);

				for (opt_idx = 0 ; opt_idx < coap_request.numopts ; opt_idx++) {
					coap_option_t *opt_ptr = &(coap_request.opts[opt_idx]);

					switch (opt_ptr->num) {
						case COAP_OPTION_OBSERVE:
							for (idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
								obs_num = (obs_num << 8) | opt_ptr->buf.p[idx];
							}

							//is_obs_option = pdTRUE;

							COAPS_DEBUG_INFO("Observe(num:%d,len:%d)\r\n",
											 obs_num, opt_ptr->buf.len);
							break;
						case COAP_OPTION_BLOCK_1:
							coap_get_block_info(opt_ptr, &block1_num,
												&block1_more, &block1_szx);

							COAPS_DEBUG_INFO("Block#1(%d:%d:%d)\r\n",
											 block1_num, block1_more, block1_szx);
							break;
						case COAP_OPTION_BLOCK_2:
							coap_get_block_info(opt_ptr,
												&block2_num,
												&block2_more,
												&block2_szx);

							is_block2_option = pdTRUE;

							COAPS_DEBUG_INFO("Block#2(%d:%d:%d)\r\n",
											 block2_num, block2_more, block2_szx);
							break;
						case COAP_OPTION_MAX_AGE:
							for (idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
								max_age = (max_age << 8) | opt_ptr->buf.p[idx];
							}

							COAPS_DEBUG_INFO("Max-age(%d)\r\n", max_age);
							break;
						default:
							break;
					}
				}

				if (is_block2_option) {
					coap_set_bytes_splitted_by_block(&coap_response,
													 (int)block2_num,
													 block2_szx,
													 &block2_more);

					uint8_t option_buf[5] = {0x00, };
					int opt_len = 0;
					int block2_opt_len = coap_get_block_opt_len((int)block2_num);
					opt_len = coap_set_block_payload((int)block2_num,
													 block2_szx,
													 block2_opt_len,
													 option_buf,
													 opt_len,
													 block2_more);

					coap_response.opts[coap_response.numopts].num = COAP_OPTION_BLOCK_2;
					coap_response.opts[coap_response.numopts].buf.p = option_buf;
					coap_response.opts[coap_response.numopts].buf.len = (size_t)block2_opt_len;
					coap_response.numopts++;
				}

				if (is_separate == 1 && coap_request.header.type == COAP_MSG_TYPE_CON) {
					coap_server_send_ack(coap_server_ptr,
										 peer_info,
										 coap_request.header.msg_id);

					/*section for payload to separate response*/
				}

				//generate packet
				ret = coap_generate_packet(send_buf, &send_buf_len, &coap_response);
				if (ret) {
					COAPS_DEBUG_ERR("Failed to bulid packet(%d)\r\n", ret);
					goto next;
				}

				//send packet
				ret = coap_server_send(coap_server_ptr, peer_info, send_buf, send_buf_len);
				if (ret) {
					COAPS_DEBUG_ERR("Failed to send packet(0x%x)\r\n", -ret);
				}
			} else {
				COAPS_DEBUG_ERR("Failed to parse CoAP request(%d)\r\n", ret);

				//release coap client
				peer_addr = peer_info->addr;
				coap_server_delete_peer_in_ip_addr(coap_server_ptr, &peer_addr);
				peer_info = NULL;
			}

			if (peer_info) {
				COAPS_DEBUG_INFO("Release peer's buffer\r\n");
				if (peer_info->buf) {
					coap_free(peer_info->buf);
				}

				peer_info->buf = NULL;
				peer_info->buflen = 0;
				peer_info->offset = 0;
			}
		} else if (ret == DA_APP_NO_PACKET) {
			goto next;
		} else {
			//release coap client
			peer_addr = peer_info->addr;
			coap_server_delete_peer_in_ip_addr(coap_server_ptr, &peer_addr);
			peer_info = NULL;
		}

next:

		coap_server_update_peer_timeout(coap_server_ptr, wait_option);

		coap_server_delete_peer_in_timeout(coap_server_ptr);

		coap_server_print_peer_list(coap_server_ptr);

#if 0 /* TODO: is it required? */
		coap_server_clear_peer_list(coap_server_ptr);
#endif

cleanup:

		if (coap_request.copy_mode) {
			coap_free_rw_buffer(&coap_request.payload_raw);
		}

		memset(&coap_request, 0x00, sizeof(coap_packet_t));

		if (coap_response.copy_mode) {
			coap_free_rw_buffer(&coap_response.payload_raw);
		}

		memset(&coap_response, 0x00, sizeof(coap_packet_t));

		obs_num = 0;
		//is_obs_option = pdFALSE;

		max_age = 0;

		block1_num = 0;
		block1_more = 0;
		block1_szx = 0;

		block2_num = 0;
		block2_more = 0;
		block2_szx = 0;
		is_block2_option = pdFALSE;

		send_buf_len = COAP_SERVER_SEND_BUF_SIZE;
		memset(send_buf, 0x00, send_buf_len);

		//terminate:
		if (ret == COAP_SERVER_REQ_TERMINATE) {
			break;
		}
	}

end_of_task:

	COAPS_DEBUG_INFO("Terminate CoAP Server\r\n");

	coap_server_ptr->state = COAP_SERVER_STATUS_STOPPING;

	if (send_buf) {
		coap_free(send_buf);
	}

	while (1) {
		vTaskDelay(100);
	}

	return ;
}

//manage peer information
int coap_server_clear_peer_list(coap_server_t *coap_server_ptr)
{
	coap_server_peer_info_t *del_ptr = NULL;

	while (coap_server_ptr->peer_list) {
		del_ptr = coap_server_ptr->peer_list;
		coap_server_ptr->peer_list = coap_server_ptr->peer_list->next;

		coap_server_delete_peer(&del_ptr);
	}

	coap_server_ptr->peer_count = 0;
	coap_server_ptr->peer_list = NULL;

	return DA_APP_SUCCESS;
}

void coap_server_print_peer_list(coap_server_t *coap_server_ptr)
{
	coap_server_peer_info_t *cur_ptr = coap_server_ptr->peer_list;

	while (cur_ptr) {
		COAPS_DEBUG_INFO("Peer info\r\n"
						 "\tIP Address: %d.%d.%d.%d:%d\r\n"
						 "\tActivity timeout: %ld(max:%ld)\r\n"
						 "\tRemainder buflen: %ld\r\n",
						 (ntohl(cur_ptr->addr.sin_addr.s_addr) >> 24) & 0x0ff,
						 (ntohl(cur_ptr->addr.sin_addr.s_addr) >> 16) & 0x0ff,
						 (ntohl(cur_ptr->addr.sin_addr.s_addr) >>  8) & 0x0ff,
						 (ntohl(cur_ptr->addr.sin_addr.s_addr)      ) & 0x0ff,
						 ntohs(cur_ptr->addr.sin_port),
						 cur_ptr->activity_timeout,
						 coap_server_ptr->max_peer_activity_timeout,
						 cur_ptr->buflen);

		cur_ptr = cur_ptr->next;
	}

	return ;
}

coap_server_peer_info_t *coap_server_find_peer_in_ip_addr(
	coap_server_t *coap_server_ptr,
	struct sockaddr_in *addr)
{
	coap_server_peer_info_t *peer_ptr = coap_server_ptr->peer_list;
	while (peer_ptr) {
		if (memcmp(&peer_ptr->addr, addr, sizeof(*addr)) == 0) {
			COAPS_DEBUG_INFO("Found peer(%d.%d.%d.%d:%d)\r\n",
							 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 24) & 0x0ff,
							 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 16) & 0x0ff,
							 (ntohl(peer_ptr->addr.sin_addr.s_addr) >>  8) & 0x0ff,
							 (ntohl(peer_ptr->addr.sin_addr.s_addr)      ) & 0x0ff,
							 ntohs(peer_ptr->addr.sin_port));

			return peer_ptr;
		}

		peer_ptr = peer_ptr->next;
	}

	return NULL;
}

coap_server_peer_info_t *coap_server_find_peer_in_remainder(
	coap_server_t *coap_server_ptr)
{
	coap_server_peer_info_t *cur_ptr = coap_server_ptr->peer_list;

	while (cur_ptr) {
		if (cur_ptr->buf) {
			COAPS_DEBUG_TEMP("Found remainder\r\n");
			return cur_ptr;
		}

		cur_ptr = cur_ptr->next;
	}

	return NULL;
}

int coap_server_add_peer(coap_server_t *coap_server_ptr,
						 coap_server_peer_info_t *peer_ptr)
{
	coap_server_peer_info_t *last_ptr = NULL;

	if (coap_server_ptr->max_peer_count < (coap_server_ptr->peer_count + 1)) {
		COAPS_DEBUG_ERR("Not allowed additional peer connection(max peer:%ld)\r\n",
						coap_server_ptr->max_peer_count);
		return DA_APP_NOT_SUCCESSFUL;
	}

	coap_server_ptr->peer_count++;

	COAPS_DEBUG_TEMP("To add peer info\r\n"
					 "\tIP Address: %d.%d.%d.%d:%d\r\n"
					 "\tCurrent connected peer count: %d\r\n",
					 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 24) & 0x0ff,
					 (ntohl(peer_ptr->addr.sin_addr.s_addr) >> 16) & 0x0ff,
					 (ntohl(peer_ptr->addr.sin_addr.s_addr) >>  8) & 0x0ff,
					 (ntohl(peer_ptr->addr.sin_addr.s_addr)      ) & 0x0ff,
					 ntohs(peer_ptr->addr.sin_port),
					 coap_server_ptr->peer_count);

	if (!coap_server_ptr->peer_list) {
		coap_server_ptr->peer_list = peer_ptr;
		return DA_APP_SUCCESS;
	}

	//find last pointer
	for (last_ptr = coap_server_ptr->peer_list ; last_ptr->next != NULL ;
		 last_ptr = last_ptr->next);

	//add peer info
	last_ptr->next = peer_ptr;

	return DA_APP_SUCCESS;
}

int coap_server_delete_peer_in_ip_addr(coap_server_t *coap_server_ptr,
									   struct sockaddr_in *addr)
{
	coap_server_peer_info_t *cur_ptr = coap_server_ptr->peer_list;
	coap_server_peer_info_t *prev_ptr = NULL;
	coap_server_peer_info_t *del_ptr = NULL;

	//find peer info
	while (cur_ptr) {
		if (memcmp(&cur_ptr->addr, addr, sizeof(*addr)) == 0) {
			//delete peer info
			if (cur_ptr == coap_server_ptr->peer_list) {
				coap_server_ptr->peer_list = cur_ptr->next;
			} else {
				prev_ptr->next = cur_ptr->next;
			}

			del_ptr = cur_ptr;
			cur_ptr = cur_ptr->next;
			del_ptr->next = NULL;

			coap_server_delete_peer(&del_ptr);

			coap_server_ptr->peer_count--;
		} else {
			prev_ptr = cur_ptr;
			cur_ptr = cur_ptr->next;
		}
	}

	return DA_APP_SUCCESS;
}

int coap_server_delete_peer_in_timeout(coap_server_t *coap_server_ptr)
{
	coap_server_peer_info_t *cur_ptr = coap_server_ptr->peer_list;
	coap_server_peer_info_t *prev_ptr = NULL;
	coap_server_peer_info_t *del_ptr = NULL;

	//find peer info
	while (cur_ptr) {
		if (cur_ptr->activity_timeout > coap_server_ptr->max_peer_activity_timeout) {
			//delete peer info
			if (cur_ptr == coap_server_ptr->peer_list) {
				coap_server_ptr->peer_list = cur_ptr->next;
			} else {
				prev_ptr->next = cur_ptr->next;
			}

			del_ptr = cur_ptr;
			cur_ptr = cur_ptr->next;

			coap_server_delete_peer(&del_ptr);

			coap_server_ptr->peer_count--;
		} else {
			prev_ptr = cur_ptr;
			cur_ptr = cur_ptr->next;
		}
	}

	return DA_APP_SUCCESS;
}

int coap_server_update_peer_timeout(coap_server_t *coap_server_ptr,
									unsigned long timeout)
{
	coap_server_peer_info_t *cur_ptr = coap_server_ptr->peer_list;

	for (cur_ptr = coap_server_ptr->peer_list ; cur_ptr != NULL ;
		 cur_ptr = cur_ptr->next) {
		cur_ptr->activity_timeout += timeout;
	}

	return DA_APP_SUCCESS;
}

coap_server_peer_info_t *coap_server_create_peer(coap_server_t *coap_server_ptr,
		struct sockaddr_in *ip_addr)
{
	int ret = DA_APP_SUCCESS;
	coap_server_peer_info_t *peer_ptr = NULL;

	peer_ptr = coap_calloc(1, sizeof(coap_server_peer_info_t));
	if (!peer_ptr) {
		COAPS_DEBUG_ERR("Failed to allocate memory to create peer info\r\n");
		return NULL;
	}

	mbedtls_net_init(&peer_ptr->sock_ctx);

	memcpy(&peer_ptr->addr, ip_addr, sizeof(struct sockaddr_in));
	peer_ptr->next = NULL;

	peer_ptr->activity_timeout = 0;

	peer_ptr->coap_server_ptr = (void *)coap_server_ptr;

	if (coap_server_ptr->secure_mode) {
		ret = coap_server_init_peer_secure(coap_server_ptr, peer_ptr);
		if (ret == DA_APP_SUCCESS) {
			COAPS_DEBUG_ERR("Failed to init peer secure mode(0x%x)\r\n", -ret);
			coap_free(peer_ptr);
			return NULL;
		}

		peer_ptr->secure_mode = coap_server_ptr->secure_mode;
	}

	return peer_ptr;
}

int coap_server_delete_peer(coap_server_peer_info_t **peer_ptr)
{
	coap_server_peer_info_t *del_ptr = *peer_ptr;

	if (del_ptr == NULL) {
		return DA_APP_INVALID_PARAMETERS;
	}

	COAPS_DEBUG_TEMP("To delete peer info\r\n"
					 "\tIP Address: %d.%d.%d.%d:%d\r\n",
					 (ntohl(del_ptr->addr.sin_addr.s_addr) >> 24) & 0x0ff,
					 (ntohl(del_ptr->addr.sin_addr.s_addr) >> 16) & 0x0ff,
					 (ntohl(del_ptr->addr.sin_addr.s_addr) >>  8) & 0x0ff,
					 (ntohl(del_ptr->addr.sin_addr.s_addr)      ) & 0x0ff,
					 ntohs(del_ptr->addr.sin_port));


	if (del_ptr->secure_mode && del_ptr->ssl_ctx) {
		coap_server_send_close_notify(del_ptr);
	}

	if (del_ptr->buf) {
		coap_free(del_ptr->buf);
	}

	coap_server_deinit_peer_secure(del_ptr);

	mbedtls_net_free(&del_ptr->sock_ctx);

	coap_free(del_ptr);

	*peer_ptr = NULL;

	return DA_APP_SUCCESS;
}

int coap_server_init_peer_secure(coap_server_t *coap_server_ptr,
								 coap_server_peer_info_t *peer_ptr)
{
	int ret = DA_APP_MALLOC_ERROR;

	if (!peer_ptr->ssl_ctx) {
		peer_ptr->ssl_ctx = coap_calloc(1, sizeof(mbedtls_ssl_context));
		if (!peer_ptr->ssl_ctx) {
			COAPS_DEBUG_ERR("Failed to allocate ssl context\r\n");
			goto error;
		}

		mbedtls_ssl_init(peer_ptr->ssl_ctx);
	}

	ret = mbedtls_ssl_setup(peer_ptr->ssl_ctx, coap_server_ptr->ssl_conf);
	if (ret) {
		COAPS_DEBUG_ERR("Failed to setup ssl(0x%x)\r\n", -ret);
		goto error;
	}

	mbedtls_ssl_set_timer_cb(peer_ptr->ssl_ctx,
							 (void *)&peer_ptr->timer,
							 coap_server_start_peer_timer,
							 coap_server_get_peer_timer_state);

	mbedtls_ssl_set_bio(peer_ptr->ssl_ctx,
						&peer_ptr->sock_ctx,
						mbedtls_net_send,
						mbedtls_net_recv,
						mbedtls_net_recv_timeout);

	return DA_APP_SUCCESS;

error:

	coap_server_deinit_peer_secure(peer_ptr);

	return ret;

}

int coap_server_deinit_peer_secure(coap_server_peer_info_t *peer_ptr)
{
	if (peer_ptr->ssl_ctx) {
		mbedtls_ssl_free(peer_ptr->ssl_ctx);
		coap_free(peer_ptr->ssl_ctx);
		peer_ptr->ssl_ctx = NULL;
	}

	return DA_APP_SUCCESS;
}

// If this looked in the table at the path before the method then
// it could more easily return 405 errors
int coap_server_handle_request(coap_server_t *coap_server_ptr,
							   coap_rw_buffer_t *scratch,
							   const coap_packet_t *in,
							   coap_packet_t *out,
							   uint8_t *is_separate)
{
	const coap_option_t *opt;
	uint8_t count;
	unsigned int idx;
	const coap_endpoint_t *endpoint = coap_server_ptr->endpoint_ptr;
	int match = 0;

	while (endpoint->handler) {
		if ((opt = coap_search_options(in, COAP_OPTION_URI_PATH, &count)) != NULL) {
			if (count != endpoint->path->count) {
				goto next;
			}

			for (idx = 0 ; idx < count ; idx++) {
				if (opt[idx].buf.len != strlen(endpoint->path->elems[idx])) {
					goto next;
				}

				if (memcmp(endpoint->path->elems[idx],
						   opt[idx].buf.p, opt[idx].buf.len) != 0) {
					goto next;
				}
			}

			// match!
			if (endpoint->method != in->header.code) {
				COAPS_DEBUG_TEMP("Not matched method(%d:%d)\r\n",
								 endpoint->method, in->header.code);
				match = 1;
			} else {
				*is_separate = endpoint->is_separate;
				return endpoint->handler(scratch, in, out,
										 in->header.msg_id[0], in->header.msg_id[1]);
			}
		}

next:
		endpoint++;
	}

	if (match) {
		coap_generate_response(scratch,
							   out,
							   NULL,
							   0,
							   in->header.msg_id[0],
							   in->header.msg_id[1],
							   &in->token,
							   COAP_RESP_CODE_NOT_IMPLEMENTED,
							   COAP_CONTENT_TYPE_NONE,
							   in->header);
	} else {
		coap_generate_response(scratch,
							   out,
							   NULL,
							   0,
							   in->header.msg_id[0],
							   in->header.msg_id[1],
							   &in->token,
							   COAP_RESP_CODE_NOT_FOUND,
							   COAP_CONTENT_TYPE_NONE,
							   in->header);
	}

	return 0;
}

int coap_server_send_ack(coap_server_t *coap_server_ptr,
						 coap_server_peer_info_t *peer_ptr,
						 uint8_t msgId[2])
{
	coap_packet_t ack_pkt;
	unsigned char *ack_buf = NULL;
	size_t ack_buf_len = 0;
	size_t ack_len = 0;
	int ret = 0;

	memset(&ack_pkt, 0x00, sizeof(coap_packet_t));

	/*section for ACK to separate response*/
	ack_pkt.header.version = 0x01;
	ack_pkt.header.type = COAP_MSG_TYPE_ACK;
	ack_pkt.header.msg_id[0] = msgId[0];
	ack_pkt.header.msg_id[1] = msgId[1];
	ack_pkt.header.token_len = 0;
	ack_pkt.token.len = 0;

	ack_buf_len = ack_len = 50 + ack_pkt.payload.len;

	ack_buf = coap_calloc(ack_buf_len, sizeof(unsigned char));
	if (!ack_buf) {
		COAPS_DEBUG_ERR("failed to allocate memory for ack message\r\n");
		return DA_APP_MALLOC_ERROR;
	}

	ret = coap_generate_packet(ack_buf, &ack_len, &ack_pkt);
	if (ret != COAP_ERR_SUCCESS) {
		COAPS_DEBUG_ERR("Failed to build CoAP ACK(%d)\r\n", ret);
		ret = DA_APP_NOT_SUCCESSFUL;
		goto end;
	}

	ret = coap_server_send(coap_server_ptr, peer_ptr, ack_buf, ack_len);
	if (ret != DA_APP_SUCCESS) {
		COAPS_DEBUG_ERR("Failed to send CoAP ACK(0x%x)\r\n", -ret);
	}

end:

	if (ack_buf) {
		coap_free(ack_buf);
	}

	return ret;
}

/* EOF */
