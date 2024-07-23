/**
 ****************************************************************************************
 *
 * @file atcmd_tls_client.h
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

#ifndef __ATCMD_TLS_CLIENT_H__
#define __ATCMD_TLS_CLIENT_H__

#include "sdk_type.h"

#if defined (__SUPPORT_ATCMD_TLS__)

#include "da16x_network_common.h"
#include "da16x_network_main.h"

#include "user_dpm.h"
#include "user_dpm_api.h"
#include "da16x_system.h"
#include "command.h"
#include "common_uart.h"
#include "da16x_dpm_tls.h"
#include "cc3120_hw_eng_initialize.h"
#include "atcmd_cert_mng.h"
#include "app_errno.h"
#include "lwip/sockets.h"

#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"

#define ATCMD_TLSC_PREFIX_TASK_NAME		"atc_tlsc_task"
#define ATCMD_TLSC_PREFIX_SOCKET_NAME	"atc_tlsc_sock"
#define ATCMD_TLSC_PREFIX_TLS_NAME		"atc_tlsc_tls"

#define ATCMD_TLSC_MAX_THREAD_NAME		20
#define ATCMD_TLSC_MAX_QUEUE_NAME		20
#define ATCMD_TLSC_MAX_SOCKET_NAME		20
#define ATCMD_TLSC_MAX_TLS_NAME			20

#define ATCMD_TLSC_TASK_SIZE			(1024)	//word
#define ATCMD_TLSC_TASK_PRIORITY		(tskIDLE_PRIORITY + 8)

#define ATCMD_TLSC_SLEEP_TIMEOUT		10
#define ATCMD_TLSC_HANDSHAKE_TIMEOUT	500
#define ATCMD_TLSC_DEF_TIMEOUT			500
#define ATCMD_TLSC_RECV_TIMEOUT			10
#define ATCMD_TLSC_RECONN_SLEEP_TIMEOUT	100
#define ATCMD_TLSC_CONN_TIMEOUT			100
#define ATCMD_TLSC_DISCONN_TIMEOUT		100
#define ATCMD_TLSC_MAX_CONN_CNT			5
#define ATCMD_TLSC_MAX_HOSTNAME			128

#define ATCMD_TLSC_DATA_BUF_SIZE		(1024 * 2)

#define ATCMD_TLSC_DEF_INCOMING_LEN		(1024 * 4)
#define ATCMD_TLSC_MIN_INCOMING_LEN		(1024)
#define ATCMD_TLSC_MAX_INCOMING_LEN		(1024 * 17)

#define ATCMD_TLSC_DEF_OUTGOING_LEN		(1024 * 4)
#define ATCMD_TLSC_MIN_OUTGOING_LEN		(1024)
#define ATCMD_TLSC_MAX_OUTGOING_LEN		(1024 * 17)

/// Rx TLS Client message Prefix
#define ATCMD_TLSC_DATA_RX_PREFIX		"+TRSSLDTC"

/// TLS Client Disconnection Prefix
#define ATCMD_TLSC_DISCONN_RX_PREFIX	"+TRSSLXTC"

typedef enum _atcmd_tlsc_state {
	ATCMD_TLSC_STATE_TERMINATED     = 0,
	ATCMD_TLSC_STATE_DISCONNECTED   = 1,
	ATCMD_TLSC_STATE_CONNECTED      = 2,
	ATCMD_TLSC_STATE_REQ_TERMINATE  = 3,
} atcmd_tlsc_state;

typedef struct _atcmd_tlsc_config {
	char ca_cert_name[ATCMD_CM_MAX_NAME];
	char cert_name[ATCMD_CM_MAX_NAME];

	unsigned int auth_mode;
	unsigned int incoming_buflen;
	unsigned int outgoing_buflen;

	char hostname[ATCMD_TLSC_MAX_HOSTNAME];

	unsigned int local_port;
	unsigned long svr_addr;
	unsigned int svr_port;
	unsigned int iface;

	size_t recv_buflen;

	unsigned long task_priority;
	size_t task_size;
} atcmd_tlsc_config;

typedef struct _atcmd_tlsc_context {
	atcmd_tlsc_state state;
	int cid;

	TaskHandle_t task_handler;
	unsigned char task_name[ATCMD_TLSC_MAX_THREAD_NAME];

	unsigned int timeout;
	unsigned char *recv_buf;

	mbedtls_net_context_dpm net_ctx;
	char socket_name[ATCMD_TLSC_MAX_SOCKET_NAME];

	struct sockaddr_in local_addr; //assigned local ip

	mbedtls_ssl_context *ssl_ctx;
	mbedtls_ssl_config *ssl_conf;
	mbedtls_ctr_drbg_context *ctr_drbg_ctx;
	mbedtls_entropy_context *entropy_ctx;

	mbedtls_x509_crt *ca_cert_crt;
	mbedtls_x509_crt *cert_crt;
	mbedtls_pk_context *pkey_ctx;
	mbedtls_pk_context *alt_pkey_ctx;

	char tls_name[ATCMD_TLSC_MAX_TLS_NAME];

	atcmd_tlsc_config *conf;
} atcmd_tlsc_context;

// External
void atcmd_tlsc_set_malloc_free(void *(*malloc_func)(size_t), void (*free_func)(void *));
int atcmd_tlsc_init_context(atcmd_tlsc_context *ctx);
int atcmd_tlsc_deinit_context(atcmd_tlsc_context *ctx);
int atcmd_tlsc_setup_config(atcmd_tlsc_context *ctx, atcmd_tlsc_config *conf);
int atcmd_tlsc_set_incoming_buflen(atcmd_tlsc_config *conf, unsigned int buflen);
int atcmd_tlsc_set_outgoing_buflen(atcmd_tlsc_config *conf, unsigned int buflen);
int atcmd_tlsc_set_hostname(atcmd_tlsc_config *conf, char *hostname);
int atcmd_tlsc_run(atcmd_tlsc_context *ctx, int id);
int atcmd_tlsc_stop(atcmd_tlsc_context *ctx, unsigned int wait_option);

int atcmd_tlsc_write_data(atcmd_tlsc_context *ctx, unsigned char *data, size_t data_len);

// Internal
int atcmd_tlsc_init_socket(atcmd_tlsc_context *ctx);
int atcmd_tlsc_connect_socket(atcmd_tlsc_context *ctx, unsigned int wait_option);
int atcmd_tlsc_disconnect_socket(atcmd_tlsc_context *ctx);
int atcmd_tlsc_send_data(atcmd_tlsc_context *ctx, unsigned char *data, size_t data_len);
int atcmd_tlsc_recv(atcmd_tlsc_context *ctx, unsigned char *out, size_t outlen,
					unsigned int wait_option);
int atcmd_tls_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
							   const unsigned char *input, unsigned char *output,
							   size_t output_max_len);
int atcmd_tls_rsa_sign_func(void *ctx,
							int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
							int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
							const unsigned char *hash, unsigned char *sig);
size_t atcmd_tls_rsa_key_len_func(void *ctx);
int atcmd_tlsc_store_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_restore_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_clear_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_init_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_setup_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_deinit_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_shutdown_ssl(atcmd_tlsc_context *ctx);
int atcmd_tlsc_do_handshake(atcmd_tlsc_context *ctx, unsigned long wait_option);
void atcmd_tlsc_transfer_recv_data(atcmd_tlsc_context *ctx, unsigned char *data, size_t data_len);
void atcmd_tlsc_transfer_disconn_data(atcmd_tlsc_context *ctx);
void atcmd_tlsc_entry_func(void *pvParamters);

#endif // (__SUPPORT_ATCMD_TLS__)

#endif // __ATCMD_TLS_CLIENT_H__

/* EOF */
