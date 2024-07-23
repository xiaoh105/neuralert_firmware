/**
 ****************************************************************************************
 *
 * @file atcmd_tls_client.c
 *
 * @brief AT-CMD TLS Client Controller
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

#include "atcmd_tls_client.h"

#if defined (__SUPPORT_ATCMD_TLS__)

#include "atcmd.h"
#include "net_bsd_sockets.h"
#include "atcmd_cert_mng.h"

#undef	ENABLE_ATCMD_TLSC_DBG_INFO
#define	ENABLE_ATCMD_TLSC_DBG_ERR

#define ATCMD_TLSC_DBG	ATCMD_DBG

#if defined (ENABLE_ATCMD_TLSC_DBG_INFO)
#define ATCMD_TLSC_INFO(fmt, ...)   \
	ATCMD_TLSC_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_TLSC_INFO(...)    do {} while (0)
#endif  // (ENABLE_ATCMD_TLSC_DBG_INFO)

#if defined (ENABLE_ATCMD_TLSC_DBG_ERR)
#define ATCMD_TLSC_ERR(fmt, ...)    \
	ATCMD_TLSC_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_TLSC_ERR(...) do {} while (0)
#endif // (ENABLE_ATCMD_TLSC_DBG_ERR)

void *(*atcmd_tlsc_malloc)(size_t n) = pvPortMalloc;
void (*atcmd_tlsc_free)(void *ptr) = vPortFree;

void atcmd_tlsc_set_malloc_free(void *(*malloc_func)(size_t), void (*free_func)(void *))
{
	atcmd_tlsc_malloc = malloc_func;
	atcmd_tlsc_free = free_func;

	return ;
}

int atcmd_tlsc_init_context(atcmd_tlsc_context *ctx)
{
	ATCMD_TLSC_INFO("Start\n");

	memset(ctx, 0x00, sizeof(atcmd_tlsc_context));

	ctx->state = ATCMD_TLSC_STATE_TERMINATED;

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_deinit_context(atcmd_tlsc_context *ctx)
{
	ATCMD_TLSC_INFO("Start\n");

	if (ctx->state != ATCMD_TLSC_STATE_TERMINATED) {
		ATCMD_TLSC_ERR("tls client is not terminated(%d)\r\n", ctx->state);
		return DA_APP_NOT_CLOSED;
	}

	if (ctx->task_handler) {
		ATCMD_TLSC_INFO("To delete tls client task(%s:%ld)\r\n",
						ctx->task_name, strlen((const char *)ctx->task_name));
		vTaskDelete(ctx->task_handler);
	}

	if (ctx->recv_buf) {
		ATCMD_TLSC_INFO("To free tls client's recv buffer\r\n");
		ATCMD_FREE(ctx->recv_buf);
	}

	atcmd_tlsc_init_context(ctx);

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_setup_config(atcmd_tlsc_context *ctx, atcmd_tlsc_config *conf)
{
	if (!ctx || !conf) {
		ATCMD_TLSC_ERR("Invalid context\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (ctx->state != ATCMD_TLSC_STATE_TERMINATED) {
		PRINTF("[%s:%d]TLS Client state is not terminated(%d)\n",
			   __func__, __LINE__, ctx->state);
		return DA_APP_NOT_SUCCESSFUL;
	}

	if (conf->svr_addr == 0 || conf->svr_port == 0) {
		ATCMD_TLSC_ERR("Not setup TLS server info\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (!conf->task_priority) {
		conf->task_priority = ATCMD_TLSC_TASK_PRIORITY;
	}

	if (!conf->task_size) {
		conf->task_size = ATCMD_TLSC_TASK_SIZE;
	}

	if (!conf->recv_buflen) {
		conf->recv_buflen = ATCMD_TLSC_DATA_BUF_SIZE;
	}

	ctx->conf = conf;

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_set_incoming_buflen(atcmd_tlsc_config *conf, unsigned int buflen)
{
	conf->incoming_buflen = buflen;
	return DA_APP_SUCCESS;
}

int atcmd_tlsc_set_outgoing_buflen(atcmd_tlsc_config *conf, unsigned int buflen)
{
	conf->outgoing_buflen = buflen;
	return DA_APP_SUCCESS;
}

int atcmd_tlsc_set_hostname(atcmd_tlsc_config *conf, char *hostname)
{
	if (strlen(hostname) >= ATCMD_TLSC_MAX_HOSTNAME) {
		ATCMD_TLSC_ERR("over hostname length(%ld)\n", ATCMD_TLSC_MAX_HOSTNAME);
		return DA_APP_INVALID_PARAMETERS;
	}

	strcpy(conf->hostname, hostname);

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_run(atcmd_tlsc_context *ctx, int id)
{
	int status = 0;

	if (!ctx || !ctx->conf) {
		ATCMD_TLSC_ERR("Invalid context\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (ctx->state != ATCMD_TLSC_STATE_TERMINATED) {
		ATCMD_TLSC_ERR("TLS client is not terminated(%d)\n", ctx->state);
		return DA_APP_NOT_CLOSED;
	}

	if (ctx->task_handler) {
		ATCMD_TLSC_INFO("To delete previous tls client task\r\n");
		vTaskDelete(ctx->task_handler);
		ctx->task_handler = NULL;
	}

	ctx->cid = id;

	snprintf((char *)(ctx->task_name), sizeof(ctx->task_name), "%s%d",
			 ATCMD_TLSC_PREFIX_TASK_NAME, id);

	snprintf(ctx->socket_name, sizeof(ctx->socket_name), "%s%d",
			 ATCMD_TLSC_PREFIX_SOCKET_NAME, id);

	snprintf(ctx->tls_name, sizeof(ctx->tls_name), "%s%d",
			 ATCMD_TLSC_PREFIX_TLS_NAME, id);

	status = xTaskCreate(atcmd_tlsc_entry_func,
						 (const char *)(ctx->task_name),
						 ctx->conf->task_size,
						 (void *)ctx,
						 ctx->conf->task_priority,
						 &ctx->task_handler);
	if (status != pdPASS) {
		ATCMD_DBG("Failed to create tcp client task(%d)\r\n", status);
		return DA_APP_NOT_CREATED;
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_stop(atcmd_tlsc_context *ctx, unsigned int wait_option)
{
	unsigned int wait_time = 0;

	if (!ctx) {
		ATCMD_TLSC_ERR("Invalid context\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (ctx->state == ATCMD_TLSC_STATE_CONNECTED) {
		ATCMD_TLSC_INFO("Change tls client state(%s) from %d to %d\n",
						ctx->task_name, ctx->state, ATCMD_TLSC_STATE_REQ_TERMINATE);

		ctx->state = ATCMD_TLSC_STATE_REQ_TERMINATE;

		do {
			if (ctx->state == ATCMD_TLSC_STATE_TERMINATED) {
				return DA_APP_SUCCESS;
			}

			ATCMD_TLSC_INFO("sleep_time(%ld), wait_time(%ld), max(%ld)\n",
							ATCMD_TLSC_SLEEP_TIMEOUT, wait_time, wait_option);

			vTaskDelay(ATCMD_TLSC_SLEEP_TIMEOUT);

			wait_time += ATCMD_TLSC_SLEEP_TIMEOUT;
		} while (wait_time < wait_option);
	}

	return ((ctx->state == ATCMD_TLSC_STATE_TERMINATED) ? DA_APP_SUCCESS : DA_APP_NOT_SUCCESSFUL);
}

int atcmd_tlsc_write_data(atcmd_tlsc_context *ctx, unsigned char *data, size_t data_len)
{
	int status = DA_APP_SUCCESS;

	if (ctx->state == ATCMD_TLSC_STATE_CONNECTED) {
		status = atcmd_tlsc_send_data(ctx, data, data_len);
		if (status) {
			ATCMD_TLSC_ERR("Failed to write data(0x%x)\n", -status);
			return status;
		}
	} else {
		status = DA_APP_NOT_CREATED;
	}

	return status;
}

int atcmd_tlsc_init_socket(atcmd_tlsc_context *ctx)
{
	int status = 0;

	ATCMD_TLSC_INFO("Socket Name: %s(%ld)\n", ctx->socket_name, strlen(ctx->socket_name));

	status = mbedtls_net_bsd_init_dpm(&ctx->net_ctx, ctx->socket_name);
	if (status) {
		ATCMD_TLSC_ERR("Failed to init net_ctx(0x%x)\n", -status);
		return DA_APP_NOT_CREATED;
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_connect_socket(atcmd_tlsc_context *ctx, unsigned int wait_option)
{
	int status = DA_APP_SUCCESS;
	const atcmd_tlsc_config *conf = ctx->conf;

	char svr_ip_str[16] = {0x00,};
	char svr_port_str[8] = {0x00,};
	char local_port_str[8] = {0x00,};
	unsigned int local_addr_len = sizeof(struct sockaddr_in);

	ATCMD_TLSC_INFO("Local port(%d), TLS Server(%d.%d.%d.%d:%d)\n",
					conf->local_port,
					(conf->svr_addr >> 24) & 0xff,
					(conf->svr_addr >> 16) & 0xff,
					(conf->svr_addr >>  8) & 0xff,
					(conf->svr_addr      ) & 0xff,
					conf->svr_port);

	//convert server information
	longtoip(conf->svr_addr, svr_ip_str);

	snprintf(svr_port_str, sizeof(svr_port_str), "%d", conf->svr_port);

	if (conf->local_port) {
		//convert local port
		snprintf(local_port_str, sizeof(local_port_str), "%d", conf->local_port);

		//connect to server
		status = mbedtls_net_bsd_connect_with_bind_timeout_dpm(&ctx->net_ctx,
															   svr_ip_str, svr_port_str,
															   MBEDTLS_NET_PROTO_TCP, NULL,
															   local_port_str, wait_option);
		if (status) {
			ATCMD_TLSC_ERR("Failed to connect to server(0x%x)\n", -status);
			return status;
		}
	} else {
		//connect to server
		status = mbedtls_net_bsd_connect_timeout_dpm(&ctx->net_ctx,
													 svr_ip_str, svr_port_str,
													 MBEDTLS_NET_PROTO_TCP, wait_option);
		if (status) {
			ATCMD_TLSC_ERR("Failed to connect to server(0x%x)\n", -status);
			return status;
		}
	}

	status = getsockname(ctx->net_ctx.fd, (struct sockaddr *)&ctx->local_addr, (socklen_t *)&local_addr_len);
	if (status) {
		ATCMD_TLSC_ERR("Failed to get local information(%d)\n", status);
		return DA_APP_NOT_SUCCESSFUL;
	}

	ATCMD_TLSC_INFO("Assigned IP address(fd:%d, %d.%d.%d.%d:%d)\n",
					ctx->net_ctx.fd,
					(ntohl(ctx->local_addr.sin_addr.s_addr) >> 24) & 0xFF,
					(ntohl(ctx->local_addr.sin_addr.s_addr) >> 16) & 0xFF,
					(ntohl(ctx->local_addr.sin_addr.s_addr) >>  8) & 0xFF,
					(ntohl(ctx->local_addr.sin_addr.s_addr)      ) & 0xFF,
					ntohs(ctx->local_addr.sin_port));

	ATCMD_TLSC_INFO("set filter(%d)\n", ntohs(ctx->local_addr.sin_port));

	dpm_tcp_port_filter_set(ntohs(ctx->local_addr.sin_port));

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_disconnect_socket(atcmd_tlsc_context *ctx)
{
	ATCMD_TLSC_INFO("Socket Name:%s(%ld), del filter(%d)\n",
					ctx->socket_name, strlen(ctx->socket_name),
					ntohs(ctx->local_addr.sin_port));

	dpm_tcp_port_filter_delete(ntohs(ctx->local_addr.sin_port));

	mbedtls_net_bsd_free_dpm(&ctx->net_ctx);

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_send_data(atcmd_tlsc_context *ctx, unsigned char *data, size_t data_len)
{
	int status = 0;

	while ((status = mbedtls_ssl_write(ctx->ssl_ctx, data, data_len)) <= 0) {
		if ((status != MBEDTLS_ERR_SSL_WANT_READ) && (status != MBEDTLS_ERR_SSL_WANT_WRITE)) {
			ATCMD_TLSC_ERR("Failed to send data(0x%x)\n", -status);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	atcmd_tlsc_store_ssl(ctx);

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_recv(atcmd_tlsc_context *ctx, unsigned char *out, size_t outlen,
					unsigned int wait_option)
{
	int status = DA_APP_SUCCESS;

	mbedtls_ssl_conf_read_timeout(ctx->ssl_conf, wait_option);

	status = mbedtls_ssl_read(ctx->ssl_ctx, out, outlen);
	if (status < 0) {
		if ((status == MBEDTLS_ERR_SSL_WANT_WRITE) || (status == MBEDTLS_ERR_SSL_TIMEOUT)) {
			status = DA_APP_NO_PACKET;
		} else if (status == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			status = DA_APP_NOT_CONNECTED;
		} else {
			ATCMD_TLSC_ERR("Failed to read ssl packet(0x%x)\n", -status);
			status = DA_APP_NOT_SUCCESSFUL;
		}
	}

	atcmd_tlsc_store_ssl(ctx);

	return status;
}

//related with ssl
int atcmd_tls_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
							   const unsigned char *input, unsigned char *output,
							   size_t output_max_len)
{
	return mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *)ctx, NULL, NULL, mode,
									 olen, input, output, output_max_len);
}

int atcmd_tls_rsa_sign_func(void *ctx,
							int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
							int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
							const unsigned char *hash, unsigned char *sig)
{
	return mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *)ctx, f_rng, p_rng, mode,
								  md_alg, hashlen, hash, sig);
}

size_t atcmd_tls_rsa_key_len_func(void *ctx)
{
	return ((const mbedtls_rsa_context *)ctx)->len;
}

//related with dpm
int atcmd_tlsc_store_ssl(atcmd_tlsc_context *ctx)
{
	int status = 0;

	if (dpm_mode_is_enabled()) {
		//ATCMD_TLSC_INFO("To store tls session(%s)\n", ctx->tls_name);

		status = set_tls_session(ctx->tls_name, ctx->ssl_ctx);
		if (status) {
			ATCMD_TLSC_ERR("Failed to save tls session(0x%x)\n", status);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_restore_ssl(atcmd_tlsc_context *ctx)
{
	int status = 0;

	if (dpm_mode_is_enabled()) {
		ATCMD_TLSC_INFO("To restore tls session(%s)\n", ctx->tls_name);

		status = get_tls_session(ctx->tls_name, ctx->ssl_ctx);
		if (status == ER_NOT_FOUND) {
			ATCMD_TLSC_INFO("Not found(%s:%ld)\n", ctx->tls_name, strlen(ctx->tls_name));
			return DA_APP_SUCCESS;
		} else if (status != 0) {
			ATCMD_TLSC_ERR("Failed to restore tls session(0x%x)\n", status);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_clear_ssl(atcmd_tlsc_context *ctx)
{
	int status = 0;

	if (dpm_mode_is_enabled()) {
		ATCMD_TLSC_INFO("To clear tls session(%s)\n", ctx->tls_name);

		status = clr_tls_session(ctx->tls_name);
		if (status) {
			ATCMD_TLSC_INFO("Failed to restore tls session(0x%x)\n", status);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_init_ssl(atcmd_tlsc_context *ctx)
{
	atcmd_tlsc_config *conf = ctx->conf;

	ATCMD_TLSC_INFO("Init TLS\n");

	da16x_secure_module_init();

	if (!ctx->ssl_ctx) {
		ctx->ssl_ctx = atcmd_tlsc_malloc(sizeof(mbedtls_ssl_context));
		if (!ctx->ssl_ctx) {
			ATCMD_TLSC_ERR("Failed to allocate ssl context\n");
			goto error;
		}

		mbedtls_ssl_init(ctx->ssl_ctx);
	}

	if (!ctx->ssl_conf) {
		ctx->ssl_conf = atcmd_tlsc_malloc(sizeof(mbedtls_ssl_config));
		if (!ctx->ssl_conf) {
			ATCMD_TLSC_ERR("Failed to allocate ssl config\n");
			goto error;
		}

		mbedtls_ssl_config_init(ctx->ssl_conf);
	}

	if (!ctx->ctr_drbg_ctx) {
		ctx->ctr_drbg_ctx = atcmd_tlsc_malloc(sizeof(mbedtls_ctr_drbg_context));
		if (!ctx->ctr_drbg_ctx) {
			ATCMD_TLSC_ERR("Failed to allocate ctr-drbg\n");
			goto error;
		}

		mbedtls_ctr_drbg_init(ctx->ctr_drbg_ctx);
	}

	if (!ctx->entropy_ctx) {
		ctx->entropy_ctx = atcmd_tlsc_malloc(sizeof(mbedtls_entropy_context));
		if (!ctx->entropy_ctx) {
			ATCMD_TLSC_ERR("Failed to allocate entropy\n");
			goto error;
		}

		mbedtls_entropy_init(ctx->entropy_ctx);
	}

	if (strlen(conf->ca_cert_name) > 0) {
		if (!ctx->ca_cert_crt) {
			ctx->ca_cert_crt = atcmd_tlsc_malloc(sizeof(mbedtls_x509_crt));
			if (!ctx->ca_cert_crt) {
				ATCMD_TLSC_ERR("Failed to allocate CA certificate\n");
				goto error;
			}

			mbedtls_x509_crt_init(ctx->ca_cert_crt);
		}
	}

	if (strlen(conf->cert_name) > 0) {
		if (!ctx->cert_crt) {
			ctx->cert_crt = atcmd_tlsc_malloc(sizeof(mbedtls_x509_crt));
			if (!ctx->cert_crt) {
				ATCMD_TLSC_ERR("Failed to allocate certificate\n");
				goto error;
			}

			mbedtls_x509_crt_init(ctx->cert_crt);
		}

		if (!ctx->pkey_ctx) {
			ctx->pkey_ctx = atcmd_tlsc_malloc(sizeof(mbedtls_pk_context));
			if (!ctx->pkey_ctx) {
				ATCMD_TLSC_ERR("Failed to allocate private key\n");
				goto error;
			}

			mbedtls_pk_init(ctx->pkey_ctx);
		}

		if (!ctx->alt_pkey_ctx) {
			ctx->alt_pkey_ctx = atcmd_tlsc_malloc(sizeof(mbedtls_pk_context));
			if (!ctx->alt_pkey_ctx) {
				ATCMD_TLSC_ERR("Failed to allocate private key for alt\n");
				goto error;
			}

			mbedtls_pk_init(ctx->alt_pkey_ctx);
		}
	}

	return DA_APP_SUCCESS;

error:

	atcmd_tlsc_deinit_ssl(ctx);

	return DA_APP_NOT_CREATED;
}

int atcmd_tlsc_setup_ssl(atcmd_tlsc_context *ctx)
{
	int status = DA_APP_SUCCESS;
	atcmd_tlsc_config *conf = ctx->conf;
	const char *pers = "atcmd_tls_client";

	char *ca_cert = NULL;
	size_t ca_cert_len = 0;

	char *cert = NULL;
	size_t cert_len = 0;

	char *privkey = NULL;
	size_t privkey_len = 0;

	int preset = MBEDTLS_SSL_PRESET_DA16X;

	ATCMD_TLSC_INFO("Setup TLS\n");

#if defined(__SUPPORT_TLS_HW_CIPHER_SUITES__)
	preset = MBEDTLS_SSL_PRESET_DA16X_ONLY_HW;
#endif /* __SUPPORT_TLS_HW_CIPHER_SUITES__ */

	status = mbedtls_ssl_config_defaults(ctx->ssl_conf,
										 MBEDTLS_SSL_IS_CLIENT,
										 MBEDTLS_SSL_TRANSPORT_STREAM,
										 preset);
	if (status) {
		ATCMD_TLSC_ERR("Failed to set default ssl config(0x%x)\n", -status);
		return DA_APP_NOT_SUCCESSFUL;
	}

	if (strlen(conf->ca_cert_name) > 0) {
		status = atcmd_cm_get_cert_len(conf->ca_cert_name, ATCMD_CM_CERT_TYPE_CA_CERT, 0,
									   &ca_cert_len);
		if (status || ca_cert_len == 0) {
			ATCMD_TLSC_ERR("Failed to get length of CA cert(0x%x)\n", status);
			if (ca_cert_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_ca_cert;
		}

		ca_cert = atcmd_tlsc_malloc(ca_cert_len);
		if (!ca_cert) {
			ATCMD_TLSC_ERR("Failed to allocate memory for CA cert(%ld)\n", ca_cert_len);
			status = DA_APP_MALLOC_ERROR;
			goto end_of_ca_cert;
		}

		status = atcmd_cm_get_cert(conf->ca_cert_name, ATCMD_CM_CERT_TYPE_CA_CERT, 0,
								   ca_cert, &ca_cert_len);
		if (status || ca_cert_len == 0) {
			ATCMD_TLSC_ERR("Failed to get CA cert(0x%x,%ld)\n", status, ca_cert_len);
			if (ca_cert_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_ca_cert;
		}

		status = mbedtls_x509_crt_parse(ctx->ca_cert_crt, (const unsigned char *)ca_cert, ca_cert_len);
		if (status) {
			ATCMD_TLSC_ERR("Failed to parse CA certificate(0x%x)\n", -status);
			status = DA_APP_NOT_SUCCESSFUL;

#if defined(ENABLE_ATCMD_TLSC_DBG_INFO)
            hex_dump((unsigned char *)ca_cert, ca_cert_len);
#endif /* ENABLE_ATCMD_TLSC_DBG_INFO */

			goto end_of_ca_cert;
		}

		mbedtls_ssl_conf_ca_chain(ctx->ssl_conf, ctx->ca_cert_crt, NULL);

		ATCMD_TLSC_INFO("Imported CA certificate\n");

end_of_ca_cert:

		if (ca_cert) {
			atcmd_tlsc_free(ca_cert);
			ca_cert = NULL;
		}

		if (status) {
			return status;
		}
	}

	if (strlen(conf->cert_name) > 0) {
		status = atcmd_cm_get_cert_len(conf->cert_name,
									   ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_CERT,
									   &cert_len);
		if (status || cert_len == 0) {
			ATCMD_TLSC_ERR("Failed to get length of cert(0x%x)\n", status);
			if (cert_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_cert;
		}

		status = atcmd_cm_get_cert_len(conf->cert_name,
									   ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_KEY,
									   &privkey_len);
		if (status || privkey_len == 0) {
			ATCMD_TLSC_ERR("Failed to get length of private key(0x%x)\n", status);
			if (privkey_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_cert;
		}

		cert = atcmd_tlsc_malloc(cert_len);
		if (!cert) {
			ATCMD_TLSC_ERR("Failed to allocate memory for cert(%ld)\n", cert_len);
			status = DA_APP_MALLOC_ERROR;
			goto end_of_cert;
		}

		privkey = atcmd_tlsc_malloc(privkey_len);
		if (!privkey) {
			ATCMD_TLSC_ERR("Failed to allocate memory for private key(%ld)\n", privkey_len);
			status = DA_APP_MALLOC_ERROR;
			goto end_of_cert;
		}

		status = atcmd_cm_get_cert(conf->cert_name,
								   ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_CERT,
								   cert, &cert_len);
		if (status || cert_len == 0) {
			ATCMD_TLSC_ERR("Failed to get cert(0x%x)\n", status);
			if (cert_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_cert;
		}

		status = atcmd_cm_get_cert(conf->cert_name,
								   ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_KEY,
								   privkey, &privkey_len);
		if (status || privkey_len == 0) {
			ATCMD_TLSC_ERR("Failed to get private key(0x%x)\n", status);
			if (privkey_len == 0) {
				status = DA_APP_NOT_FOUND;
			}
			goto end_of_cert;
		}
		
		//Import certificate
		status = mbedtls_x509_crt_parse(ctx->cert_crt, (const unsigned char *)cert, cert_len);
		if (status) {
			ATCMD_TLSC_ERR("Failed to parse cert(0x%x)\n", -status);
			status = DA_APP_NOT_SUCCESSFUL;

#if defined(ENABLE_ATCMD_TLSC_DBG_INFO)
            hex_dump((unsigned char *)cert, cert_len);
#endif /* ENABLE_ATCMD_TLSC_DBG_INFO */

			goto end_of_cert;
		}

		ATCMD_TLSC_INFO("Imported certificate\n");

		//Import private key
		status = mbedtls_pk_parse_key(ctx->pkey_ctx, (const unsigned char *)privkey, privkey_len, NULL, 0);
		if (status) {
			ATCMD_TLSC_ERR("Failed to parse private key(0x%x)\n", -status);
			status = DA_APP_NOT_SUCCESSFUL;

#if defined(ENABLE_ATCMD_TLSC_DBG_INFO)
            hex_dump((unsigned char *)privkey, privkey_len);
#endif /* ENABLE_ATCMD_TLSC_DBG_INFO */

			goto end_of_cert;
		}

		ATCMD_TLSC_INFO("Imported private key\n");

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
		if (mbedtls_pk_get_type(ctx->pkey_ctx) == MBEDTLS_PK_RSA) {
			status = mbedtls_pk_setup_rsa_alt(ctx->alt_pkey_ctx,
											  (void *)mbedtls_pk_rsa(*ctx->pkey_ctx),
											  atcmd_tls_rsa_decrypt_func,
											  atcmd_tls_rsa_sign_func,
											  atcmd_tls_rsa_key_len_func);
			if (status) {
				ATCMD_TLSC_ERR("Failed to set rsa alt(0x%x)\n", -status);
				status = DA_APP_NOT_SUCCESSFUL;
				goto end_of_cert;
			}

			status = mbedtls_ssl_conf_own_cert(ctx->ssl_conf, ctx->cert_crt,
											   ctx->alt_pkey_ctx);
		} else {
			status = mbedtls_ssl_conf_own_cert(ctx->ssl_conf, ctx->cert_crt, ctx->pkey_ctx);
		}
#else
		status = mbedtls_ssl_conf_own_cert(ctx->ssl_conf, ctx->cert_crt, ctx->pkey_ctx);
#endif /* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */

end_of_cert:

		if (cert) {
			atcmd_tlsc_free(cert);
		}

		if (privkey) {
			atcmd_tlsc_free(privkey);
		}

		if (status) {
			ATCMD_TLSC_ERR("Failed to set cert & private key(0x%x)\n", -status);
			return status;
		}
	}

	status = mbedtls_ctr_drbg_seed(ctx->ctr_drbg_ctx,
								   mbedtls_entropy_func,
								   ctx->entropy_ctx,
								   (const unsigned char *)pers,
								   strlen(pers));
	if (status) {
		ATCMD_TLSC_ERR("Faield to set ctr-drbg seed(0x%x)\n", -status);
		return DA_APP_NOT_SUCCESSFUL;
	}

	mbedtls_ssl_conf_rng(ctx->ssl_conf, mbedtls_ctr_drbg_random, ctx->ctr_drbg_ctx);

	if (conf->auth_mode) {
		mbedtls_ssl_conf_authmode(ctx->ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	} else {
		mbedtls_ssl_conf_authmode(ctx->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
	}

	if (conf->incoming_buflen < ATCMD_TLSC_MIN_INCOMING_LEN) {
		conf->incoming_buflen = ATCMD_TLSC_DEF_INCOMING_LEN;
	}

	if (conf->outgoing_buflen < ATCMD_TLSC_MIN_OUTGOING_LEN) {
		conf->outgoing_buflen = ATCMD_TLSC_DEF_OUTGOING_LEN;
	}

	ATCMD_TLSC_INFO("Incoming(%ld), outgoing(%ld)\n",
					conf->incoming_buflen, conf->outgoing_buflen);

	status = mbedtls_ssl_conf_content_len(ctx->ssl_conf, conf->incoming_buflen,
										  conf->outgoing_buflen);
	if (status) {
		ATCMD_TLSC_ERR("Failed to set content buffer len(0x%x,%ld,%ld)\n",
					   -status, conf->incoming_buflen, conf->outgoing_buflen);
		return DA_APP_NOT_SUCCESSFUL;
	}

	if (strlen(conf->hostname)) {
		status = mbedtls_ssl_set_hostname(ctx->ssl_ctx, (const char *)conf->hostname);
		if (status) {
			ATCMD_TLSC_ERR("Failed to set hostname(0x%x,%s)\n", -status, conf->hostname);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	status = mbedtls_ssl_setup(ctx->ssl_ctx, ctx->ssl_conf);
	if (status) {
		ATCMD_TLSC_ERR("Failed to set ssl config(0x%x)\n", -status);
		return DA_APP_NOT_SUCCESSFUL;
	}

	status = atcmd_tlsc_restore_ssl(ctx);
	if (status) {
		ATCMD_TLSC_ERR(" Failed to restore ssl(0x%x)\n", -status);
		return status;
	}

	mbedtls_ssl_set_bio(ctx->ssl_ctx, (void *)&ctx->net_ctx,
						mbedtls_net_bsd_send_dpm,
						mbedtls_net_bsd_recv_dpm,
						mbedtls_net_bsd_recv_timeout_dpm);

	return DA_APP_SUCCESS;
}


int atcmd_tlsc_deinit_ssl(atcmd_tlsc_context *ctx)
{
	if (ctx->ssl_ctx) {
		mbedtls_ssl_free(ctx->ssl_ctx);
		atcmd_tlsc_free(ctx->ssl_ctx);
	}

	if (ctx->ssl_conf) {
		mbedtls_ssl_config_free(ctx->ssl_conf);
		atcmd_tlsc_free(ctx->ssl_conf);
	}

	if (ctx->ctr_drbg_ctx) {
		mbedtls_ctr_drbg_free(ctx->ctr_drbg_ctx);
		atcmd_tlsc_free(ctx->ctr_drbg_ctx);
	}

	if (ctx->entropy_ctx) {
		mbedtls_entropy_free(ctx->entropy_ctx);
		atcmd_tlsc_free(ctx->entropy_ctx);
	}

	if (ctx->ca_cert_crt) {
		mbedtls_x509_crt_free(ctx->ca_cert_crt);
		atcmd_tlsc_free(ctx->ca_cert_crt);
	}

	if (ctx->cert_crt) {
		mbedtls_x509_crt_free(ctx->cert_crt);
		atcmd_tlsc_free(ctx->cert_crt);
	}

	if (ctx->pkey_ctx) {
		mbedtls_pk_free(ctx->pkey_ctx);
		atcmd_tlsc_free(ctx->pkey_ctx);
	}

	if (ctx->alt_pkey_ctx) {
		mbedtls_pk_free(ctx->alt_pkey_ctx);
		atcmd_tlsc_free(ctx->alt_pkey_ctx);
	}

	ctx->ssl_ctx = NULL;
	ctx->ssl_conf = NULL;
	ctx->ctr_drbg_ctx = NULL;
	ctx->entropy_ctx = NULL;
	ctx->ca_cert_crt = NULL;
	ctx->cert_crt = NULL;
	ctx->pkey_ctx = NULL;
	ctx->alt_pkey_ctx = NULL;

	da16x_secure_module_deinit();

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_shutdown_ssl(atcmd_tlsc_context *ctx)
{
	int status = 0;

	status = mbedtls_ssl_session_reset(ctx->ssl_ctx);
	if (status) {
		ATCMD_TLSC_ERR("Failed to reset session(0x%x)\n", -status);
		return DA_APP_NOT_SUCCESSFUL;
	}

	return DA_APP_SUCCESS;
}

int atcmd_tlsc_do_handshake(atcmd_tlsc_context *ctx, unsigned long wait_option)
{
	int status = DA_APP_SUCCESS;

	if (ctx->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		ATCMD_TLSC_INFO("Already established tls session\n");
		return DA_APP_SUCCESS;
	}

	mbedtls_ssl_conf_read_timeout(ctx->ssl_conf, wait_option);

	while ((status = mbedtls_ssl_handshake(ctx->ssl_ctx)) != 0) {
		if (status == MBEDTLS_ERR_NET_CONN_RESET) {
			PRINTF("[%s] Peer closed the connection(0x%x)\r\n", __func__, -status);
			break;
		}

		if ((status != MBEDTLS_ERR_SSL_WANT_READ)
			&& (status != MBEDTLS_ERR_SSL_WANT_WRITE)) {
			ATCMD_TLSC_ERR("Failed to process tls handshake(0x%x)\n", -status);
			break;
		}
	}

	return status;
}

void atcmd_tlsc_transfer_recv_data(atcmd_tlsc_context *ctx, unsigned char *data,
								   size_t data_len)
{
	ATCMD_TLSC_INFO(ATCMD_TLSC_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d\n", ctx->cid,
					((ctx->conf->svr_addr) >> 24) & 0xFF,
					((ctx->conf->svr_addr) >> 16) & 0xFF,
					((ctx->conf->svr_addr) >>  8) & 0xFF,
					((ctx->conf->svr_addr)      ) & 0xFF,
					ctx->conf->local_port, data_len);

	PRINTF_ATCMD("\r\n" ATCMD_TLSC_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", ctx->cid,
				 ((ctx->conf->svr_addr) >> 24) & 0xFF,
				 ((ctx->conf->svr_addr) >> 16) & 0xFF,
				 ((ctx->conf->svr_addr) >>  8) & 0xFF,
				 ((ctx->conf->svr_addr)      ) & 0xFF,
				 ctx->conf->local_port, data_len);

	PUTS_ATCMD((char *)data, (int)data_len);

	PRINTF_ATCMD("\r\n");

	return ;
}

void atcmd_tlsc_transfer_disconn_data(atcmd_tlsc_context *ctx)
{
	ATCMD_TLSC_INFO(ATCMD_TLSC_DISCONN_RX_PREFIX ":%d,%d.%d.%d.%d,%u\n", ctx->cid,
					((ctx->conf->svr_addr) >> 24) & 0xFF,
					((ctx->conf->svr_addr) >> 16) & 0xFF,
					((ctx->conf->svr_addr) >>  8) & 0xFF,
					((ctx->conf->svr_addr)      ) & 0xFF,
					ctx->conf->local_port);

	PRINTF_ATCMD("\r\n" ATCMD_TLSC_DISCONN_RX_PREFIX ":%d,%d.%d.%d.%d,%u\r\n", ctx->cid,
				 ((ctx->conf->svr_addr) >> 24) & 0xFF,
				 ((ctx->conf->svr_addr) >> 16) & 0xFF,
				 ((ctx->conf->svr_addr) >>  8) & 0xFF,
				 ((ctx->conf->svr_addr)      ) & 0xFF,
				 ctx->conf->local_port);

	return ;
}

void atcmd_tlsc_entry_func(void *pvParameters)
{
	int sys_wdog_id = -1;
	int status = DA_APP_SUCCESS;
	atcmd_tlsc_context *ctx = (atcmd_tlsc_context *)pvParameters;

	size_t recv_len = 0;

	int conn_retry = 0;
	int handshake_retry = 0;

	ATCMD_TLSC_INFO("Start\n");

	ctx->state = ATCMD_TLSC_STATE_DISCONNECTED;

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	if (!ctx || !ctx->conf) {
		ATCMD_TLSC_ERR("Invaild context\n");
		goto terminate;
	}

	ctx->recv_buf = atcmd_tlsc_malloc(ctx->conf->recv_buflen);
	if (!ctx->recv_buf) {
		ATCMD_TLSC_ERR("Failed to allocate recv buffer(%ld)\n",
					   ctx->conf->recv_buflen);
		goto terminate;
	}

	if (dpm_mode_is_enabled()) {
		ATCMD_TLSC_INFO("Register DPM(%s,%ld)\n", ctx->task_name, ctx->conf->svr_port);
		dpm_app_register((char *)(ctx->task_name), ctx->conf->svr_port);
	}

	status = atcmd_tlsc_init_socket(ctx);
	if (status) {
		ATCMD_TLSC_ERR("Failed to init socket(0x%x)\n", -status);
		goto terminate;
	}

	status = atcmd_tlsc_init_ssl(ctx);
	if (status) {
		ATCMD_TLSC_ERR("Failed to init SSL(0x%x)\n", -status);
		goto terminate;
	}

	status = atcmd_tlsc_setup_ssl(ctx);
	if (status) {
		ATCMD_TLSC_ERR("Failed to setup SSL(0x%x)\n", -status);
		goto terminate;
	}

	dpm_app_wakeup_done((char *)(ctx->task_name));

	da16x_sys_watchdog_notify(sys_wdog_id);

	da16x_sys_watchdog_suspend(sys_wdog_id);

connect:

	ATCMD_TLSC_INFO("#%d. Connecting & waiting for %ld\n",
					conn_retry + 1, ATCMD_TLSC_CONN_TIMEOUT);

	status = atcmd_tlsc_connect_socket(ctx, ATCMD_TLSC_CONN_TIMEOUT);
	if (status) {
		ATCMD_TLSC_ERR("Failed to connect to TLS server(0x%x)\n", status);

		if (conn_retry < ATCMD_TLSC_MAX_CONN_CNT) {
			conn_retry++;
			vTaskDelay(ATCMD_TLSC_RECONN_SLEEP_TIMEOUT);

			ATCMD_TLSC_ERR("wait_time(%ld), cur_cnt(%ld), max_retry_cnt(%ld)\n",
						   ATCMD_TLSC_RECONN_SLEEP_TIMEOUT, conn_retry, ATCMD_TLSC_MAX_CONN_CNT);

			goto connect;
		} else {
			goto terminate;
		}
	}

	ATCMD_TLSC_INFO("#%d. TLS Handshake\n", handshake_retry + 1);

	status = atcmd_tlsc_do_handshake(ctx, ATCMD_TLSC_HANDSHAKE_TIMEOUT);
	if (status) {
		ATCMD_TLSC_ERR("Failed to establish TLS session(0x%x)\n", -status);

		atcmd_tlsc_shutdown_ssl(ctx);

		atcmd_tlsc_disconnect_socket(ctx);

		atcmd_tlsc_init_socket(ctx);

		if (handshake_retry < ATCMD_TLSC_MAX_CONN_CNT) {
			handshake_retry++;
			vTaskDelay(ATCMD_TLSC_DEF_TIMEOUT);
			goto connect;
		} else {
			goto terminate;
		}
	}

	atcmd_tlsc_store_ssl(ctx);

	ATCMD_TLSC_INFO("Estalished TLS session\n");

	ctx->state = ATCMD_TLSC_STATE_CONNECTED;

	dpm_app_data_rcv_ready_set((char *)(ctx->task_name));

	da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

	while (ctx->state == ATCMD_TLSC_STATE_CONNECTED) {
		da16x_sys_watchdog_notify(sys_wdog_id);

		da16x_sys_watchdog_suspend(sys_wdog_id);

		status = atcmd_tlsc_recv(ctx, ctx->recv_buf, ctx->conf->recv_buflen,
								 ATCMD_TLSC_RECV_TIMEOUT);

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

		if (status > 0) {
			ATCMD_TLSC_INFO("Clear DPM(%s)\n", ctx->task_name);
			dpm_app_sleep_ready_clear((char *)(ctx->task_name)); // dpm clear to progress msg

			recv_len = status;

			atcmd_tlsc_transfer_recv_data(ctx, ctx->recv_buf, recv_len);
		} else if (status == DA_APP_NOT_CONNECTED) {
			ctx->state = ATCMD_TLSC_STATE_DISCONNECTED;
			break;
		} else if (status == DA_APP_NO_PACKET) {
			;
		} else {
			ATCMD_TLSC_ERR("Failed to recv packet(0x%x)\n", -status);
			ctx->state = ATCMD_TLSC_STATE_DISCONNECTED;
			break;
		}

		if (mbedtls_ssl_get_bytes_avail(ctx->ssl_ctx) == 0) {
			//ATCMD_TLSC_INFO("Set DPM(%s)\n", ctx->task_name);
			dpm_app_sleep_ready_set((char *)(ctx->task_name));
		}
	}

terminate:

	da16x_sys_watchdog_unregister(sys_wdog_id);

	ATCMD_TLSC_INFO("Clear DPM(%s)\n", ctx->task_name);
	dpm_app_sleep_ready_clear((char *)(ctx->task_name));

	atcmd_tlsc_deinit_ssl(ctx);

	atcmd_tlsc_clear_ssl(ctx);

	atcmd_tlsc_disconnect_socket(ctx);

	if (ctx->recv_buf) {
		atcmd_tlsc_free(ctx->recv_buf);
		ctx->recv_buf = NULL;
	}

	ctx->state = ATCMD_TLSC_STATE_TERMINATED;

	if (dpm_mode_is_enabled()) {
		ATCMD_TLSC_INFO("deregister DPM(%s,%ld)\n", ctx->task_name,
						ctx->conf->local_port);
		dpm_app_unregister((char *)(ctx->task_name));
	}

	ATCMD_TLSC_INFO("End\n");

    ctx->task_handler = NULL;

    vTaskDelete(NULL);

	return ;
}
#endif // (__SUPPORT_ATCMD_TLS__)

/* EOF */
