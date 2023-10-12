/**
 ****************************************************************************************
 *
 * @file coap_server.h
 *
 * @brief CoAP Server functionality.
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
#ifndef	__COAP_SERVER_H__
#define	__COAP_SERVER_H__

#include "sdk_type.h"
#include "coap_common.h"
#include "coap_utils.h"

/// Debug Log level(Information) for CoAP Server
#undef	ENABLE_COAPS_DEBUG_INFO

/// Debug Log level(Error) for CoAP Server
#define	ENABLE_COAPS_DEBUG_ERR

/// Debug Log level(Temporary) for CoAP Server
#undef	ENABLE_COAPS_DEBUG_TEMP

/// CoAP server default send buffer size
#define COAP_SERVER_SEND_BUF_SIZE				(1024 + 32)

/// CoAP maxium peer connection count
#define	COAP_SERVER_MAX_PEER_COUNT				2

/// CoAP maxium peer acitivity timeout(unit of time: sec)
#define	COAP_SERVER_MAX_PEER_ACTIVITY_TIMEOUT	30

/// CoAP server default delay(unit of time: 10 tick)
#define	COAP_SERVER_DEF_TIMEOUT					100

/// CoAPs initial timeout for handshake(unit of time: 10 tick)
#define	COAP_SERVER_HANDSHAKE_MIN_TIMEOUT		100

/// CoAPs maximum timeout for handshake(unit of time: 10 tick)
#define	COAP_SERVER_HANDSHAKE_MAX_TIMEOUT		800

//Special error code for coap server.
/// Error code to terminate Coap Server
#define	COAP_SERVER_REQ_TERMINATE				DA_APP_RESERVED_CODE0

/// CoAP server state
typedef enum
{
	COAP_SERVER_STATUS_READY,
	COAP_SERVER_STATUS_RUNNING,
	COAP_SERVER_STATUS_STOPPING,
} coap_server_state_t;

/// CoAPs server configuration
typedef struct _coap_server_secure_conf_t
{
	/// CA certificate pointer for CoAPs
	const unsigned char *ca_cert;
	/// CA certificate length for CoAPs
	size_t ca_cert_len;
	/// Certificate pointer for CoAPs
	const unsigned char *cert;
	/// Certificate length for CoAPs
	size_t cert_len;
	/// Private key pointer for CoAPs
	const unsigned char *pkey;
	/// Private key length for CoAPs
	size_t pkey_len;

	/// Auth mode for DTLS Handshake
	int authmode;
	/// Fragment for CoAPs
	int fragment;
	/// CoAPs initial timeout for handshake
	int handshake_min_timeout;
	/// CoAPs maximum timeout for handshake
	int handshake_max_timeout;
	/// CoAPs default delay
	int read_timeout;
} coap_server_secure_conf_t;

/// Peer information
typedef struct _coap_server_peer_info_t
{
	/// CoAP server instance pointer
	void *coap_server_ptr;

	/// CoAP client socket
	mbedtls_net_context sock_ctx;

	/// Peer IP address
	struct sockaddr_in addr;
	/// Secure mode
	int secure_mode;

	/// CoAP received buffer
	unsigned char *buf;
	size_t buflen;
	unsigned int offset;

	/// Peer activity timeout
	unsigned long activity_timeout;

	/// SSL context for CoAPs
	mbedtls_ssl_context *ssl_ctx;

	/// CoAPs timer
	coap_timer_t timer;

	/// Next peer pointer
	struct _coap_server_peer_info_t *next;
} coap_server_peer_info_t;

/// CoAP Server instance
typedef struct _coap_server_t
{
	/// CoAP Server state
	coap_server_state_t state;

	/// CoAP Server port number
	int port;
	/// CoAP(s) Server mode
	int secure_mode;

	/// CoAPs Server configuration pointer
	const coap_server_secure_conf_t *secure_conf_ptr;

	/// CoAP Server thread name
	char *name;
	/// CoAP Server task handler 
	TaskHandle_t task_handler;
	/// CoAP Server task stack size
	size_t stack_size;

	/// CoAP Server socket
	mbedtls_net_context listen_ctx;
	/// CoAP Server socket name
	char socket_name[COAP_MAX_NAME_LEN];

	/// Maxium peer count
	unsigned int max_peer_count;
	/// Maxium activity timeout of connected peer
	unsigned long max_peer_activity_timeout;

	/// Connected peer count
	unsigned int peer_count;
	/// Connected peer pointer
	coap_server_peer_info_t *peer_list;

	/// Pointer of CoAP Server end-point
	const coap_endpoint_t *endpoint_ptr;

	//common secure config
	/// SSL config for CoAPs
	mbedtls_ssl_config *ssl_conf;
	/// SSL CTR-DRBG for CoAPs
	mbedtls_ctr_drbg_context *ctr_drbg_ctx;
	/// SSL entropy for CoAPs
	mbedtls_entropy_context *entropy_ctx;

	/// CA certificate container for CoAPs
	mbedtls_x509_crt *ca_cert_crt;
	/// Certificate container for CoAPs
	mbedtls_x509_crt *cert_crt;
	/// Private key context for CoAPs
	mbedtls_pk_context *pkey_ctx;
	mbedtls_pk_context *pkey_alt_ctx;

	/// DTLS Cookie
	mbedtls_ssl_cookie_ctx *cookie_ctx;

	/// Auth mode for DTLS Handshake
	int authmode;
	/// Fragment for CoAPs
	int fragment;
	/// CoAPs initial timeout for handshake
	int handshake_min_timeout;
	/// CoAPs maximum timeout for handshake
	int handshake_max_timeout;
	/// CoAPs default delay
	int read_timeout;
} coap_server_t;

/**
 ****************************************************************************************
 * @brief Start CoAP Server
 * @param[in] coap_server_ptr    CoAP Server instance pointer
 * @param[in] name_ptr           Name of CoAP Server
 * @param[in] ip_ptr             IP instance pointer
 * @param[in] pool_ptr           Pool pointer to allocate packet from
 * @param[in] stack_ptr          Stack pointer
 * @param[in] stack_size         Stack size
 * @param[in] endpoint_ptr       Endpoint pointer
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_server_start(coap_server_t *coap_server_ptr, char *name_ptr, size_t stack_size,
					  const coap_endpoint_t *endpoint_ptr);

/**
 ****************************************************************************************
 * @brief Start CoAPs Server
 * @param[in] coap_server_ptr    CoAP Server instance pointer
 * @param[in] name_ptr           Name of CoAPs Server
 * @param[in] ip_ptr             IP instance pointer
 * @param[in] pool_ptr           Pool pointer to allocate packet from
 * @param[in] stack_ptr          Stack pointer
 * @param[in] stack_size         Stack size
 * @param[in] endpoint_ptr       Endpoinnt pointer
 * @param[in] secure_conf_ptr    Pointer of secure configuration
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_server_start(coap_server_t *coap_server_ptr, char *name_ptr, size_t stack_size,
					   const coap_endpoint_t *endpoint_ptr,
					   const coap_server_secure_conf_t *secure_conf_ptr);

/**
 ****************************************************************************************
 * @brief Stop CoAP(s) Server
 * @param[in] coap_server_ptr    CoAP Server instance potiner
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_server_stop(coap_server_t *coap_server_ptr);

#endif	// (__COAP_SERVER_H__)

/* EOF */
