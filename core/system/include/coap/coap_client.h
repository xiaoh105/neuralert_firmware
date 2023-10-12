/**
 ****************************************************************************************
 *
 * @file coap_client.h
 *
 * @brief CoAP Client functionality.
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
#ifndef	__COAP_CLIENT_H__
#define	__COAP_CLIENT_H__

#include "sdk_type.h"
#include "coap_common.h"
#include "coap_utils.h"

/// Debug Log level for DTLS
#define	COAPS_CLIENT_DEBUG_LEVEL				4

/// Debug Log level(Information) for CoAP Client
#undef	ENABLE_COAPC_DEBUG_INFO

/// Debug Log level(Error) for CoAP Client
#define	ENABLE_COAPC_DEBUG_ERR

/// Debug Log level(Temporary) for CoAP Client
#undef	ENABLE_COAPC_DEBUG_TEMP

/// Stack size of CoAP client observe
#define	COAP_CLIENT_OBS_STACK_SIZE				(1024)

/// CoAP token length
#define	COAP_CLIENT_TOKEN_LENGTH				8

/// CoAP message-ID length
#define	COAP_CLIENT_MSGID_LENGTH				2

/// CoAP service name length
#define	COAP_CLIENT_MAX_SVCS_NAME				8

/// CoAP observe max count
#define	COAP_CLIENT_MAX_OBSERVE_LIST			1

/// CoAP client default send buffer size
#define COAP_CLIENT_SEND_BUF_SIZE				(1024 + 32)

/// CoAP ack timeout (unit of time: 100 tick)
#define	COAP_CLIENT_ACK_TIMEOUT					10

/// CoAP ack random factor to retransmit
#define	COAP_CLIENT_ACK_RANDOM_FACTOR			1

/// CoAP max retransmition count
#define	COAP_CLIENT_MAX_RETRANSMIT				4

/// CoAPs initial timeout for handshake(unit of time: 100 tick)
#define	COAPS_CLIENT_HANDSHAKE_MIN_TIMEOUT		10

/// CoAPs maximum timeout for handshake(unit of time: 100 tick)
#define	COAPS_CLIENT_HANDSHAKE_MAX_TIMEOUT		80

/// CoAPs maximum retry count for handshake
#define	COAPS_CLIENT_CONNECTION_MAX_RETRANSMIT	2

/// CoAP default block size(2^^(szx + 4))
#define	COAP_CLIENT_BLOCK_SIZE					6

/// CoAP observe default additional interval(RFC7641. 3.3.3.1 Freshness)
#define	COAP_CLIENT_OBSERVE_ADD_INTERVAL		15

/// CoAP max name length in DPM
#if defined (DPM_TIMER_NAME_MAX_LEN)
#define	COAP_CLIENT_MAX_TIMER_NAME				DPM_TIMER_NAME_MAX_LEN
#else
#define	COAP_CLIENT_MAX_TIMER_NAME				8
#endif	// (DPM_TIMER_NAME_MAX_LEN)

/// CoAP client state
typedef enum
{
	READY			= 0,
	SUSPEND			= 1,
	IN_PROGRESS		= 2
} coap_client_state_t;

/// CoAP observe state
typedef enum
{
	/// To request CoAP observe registration
	REQ_REGISTER	= 0,
	/// Registered CoAP observe
	REGISTERED		= 1,
	/// To terminate CoAP observe
	REQ_TERMINATE	= 2,
	/// Terminated CoAP observe
	TERMINATED		= 3
} coap_client_obs_state_t;

/// CoAP connection information
typedef struct _coap_client_conn_t
{
	/// Peer IP address
	unsigned long				peer_ip_addr;
	/// Peer port number
	unsigned int				peer_port;
	/// Secure connection
	unsigned int				secure_conn;
	/// Local port number
	unsigned int				local_port;

	/// Socket name
	char						socket_name[COAP_MAX_NAME_LEN];
	/// Socket pointer
	mbedtls_net_context			sock_fd;

	/// CoAPs timer
	coap_timer_t				timer;

	/// SSL context for CoAPs
	mbedtls_ssl_context			*ssl_ctx;
	/// SSL config for CoAPs
	mbedtls_ssl_config			*ssl_conf;
	/// SSL CTR-DRBG for CoAPs
	mbedtls_ctr_drbg_context	*ctr_drbg;
	/// SSL entropy for CoAPs
	mbedtls_entropy_context		*entropy;

	/// CA certificate container for CoAPs
	mbedtls_x509_crt			*ca_cert;
	/// Certificate container for CoAPs
	mbedtls_x509_crt			*cert;
	/// Private key context for CoAPs
	mbedtls_pk_context			*pkey;
} coap_client_conn_t;

/// CoAP request information
typedef struct _coap_client_req_t
{
	/// Request type
	coap_msgtype_t				type;
	/// Method type
	coap_method_t				method;
	/// Scheme
	coap_rw_buffer_t			scheme;
	/// Host
	coap_rw_buffer_t			host;
	/// Path
	coap_rw_buffer_t			path;
	/// Query
	coap_rw_buffer_t			query;
	/// Port number
	unsigned int				port;
	/// Proxy-URI
	coap_rw_buffer_t			proxy_uri;
	/// Payload
	coap_buffer_t				payload;

	/// CoAP observe flag
	unsigned int				observe_reg;	/* 0: None. 1: Reg. 2: Unreg */

	/// CoAP Block#1 option - number
	unsigned int				block1_num;
	/// CoAP Block#1 option - more flag
	uint8_t						block1_more;
	/// CoAP Block#1 option - szx
	uint8_t						block1_szx;

	/// CoAP Block#2 option - number
	unsigned int				block2_num;
	/// CoAP Block#2 option - more flag
	uint8_t						block2_more;
	/// CoAP Block#2 option - szx
	uint8_t						block2_szx;

	/// Token pointer
	unsigned char				*token_ptr;
	/// Message-ID pointer
	unsigned short				*msgId_ptr;
} coap_client_req_t;

/// CoAP observe information
typedef struct _coap_client_obs_t
{
	/// CoAP client instance pointer
	void						*client_ptr;

	/// CoAP observe thread name
	char						thread_name[COAP_MAX_NAME_LEN];
	/// CoAP observe thread pointer
	TaskHandle_t				task_handler;

	/// CoAP request pointer
	coap_client_req_t	request;
	/// CoAP connection pointer
	coap_client_conn_t	conn;

	/// CoAP observe close timer name
	char						close_timer_name[COAP_CLIENT_MAX_TIMER_NAME];
	/// CoAP observe close timer in Non-DPM mode
	TimerHandle_t				close_timer;
	/// CoAP observe max-age
	unsigned int				max_age;

	/// CoAP observe token
	unsigned char				token[COAP_CLIENT_TOKEN_LENGTH];
	/// CoAP observe message-ID
	unsigned short				msgId;

	/// CoAP observe receive notify callback
	int (*notify)(void *client_ptr, coap_rw_packet_t *resp_ptr);
	/// CoAP observe close notify callback
	void (*close_notify)(char *timer_name);

	/// CoAP observe state
	coap_client_obs_state_t	status;

	/// CoAP observe id
	int							id;
	/// CoAP observe index in DPM mode
	int							dpm_idx;

	/// CoAP observe pointer for next
	struct _coap_client_obs_t	*next;
} coap_client_obs_t;

/// CoAP client configuration
typedef struct _coap_client_conf_t
{
	/// Interface
	unsigned int				iface;
	/// Message type for CoAP request
	coap_msgtype_t				msg_type;

	/// CoAP Ack timeout
	unsigned int				ack_timeout;
	/// CoAP max retransmission count
	unsigned int				max_retransmit;

	/// CoAP Block#1 option - szx
	unsigned int				block1_szx;
	/// CoAP Block#2 option - szx
	unsigned int				block2_szx;

	/// CA certificate pointer for CoAPs
	const unsigned char			*ca_cert;
	/// CA certificate length for CoAPs
	size_t						ca_cert_len;
	/// Certificate pointer for CoAPs
	const unsigned char			*cert;
	/// Certificate length for CoAPs
	size_t						cert_len;
	/// Private key pointer for CoAPs
	const unsigned char			*pkey;
	/// Private key length for CoAPs
	size_t						pkey_len;
	/// Pre-shared key pointer for CoAPs
	const unsigned char			*psk_key;
	/// Pre-shared key length for CoAPs
	size_t						psk_key_len;
	/// Pre-shared key identity pointer for CoAPs
	const unsigned char			*psk_identity;
	/// Pre-shared key identity length for CoAPs
	size_t						psk_identity_len;

	/// Auth mode for DTLS Handshake
	unsigned int				authmode;
	/// Fragment for CoAPs
	unsigned int				fragment;

	/// CoAPs initial timeout for handshake
	unsigned int				handshake_min_timeout;
	/// CoAPs maximum timeout for handshake
	unsigned int				handshake_max_timeout;
	/// CoAPs maximum retry count for handshake
	unsigned int				secure_conn_max_retransmit;

	/// CoAP URI
	unsigned char				*uri;
	/// CoAP URI length
	size_t						uri_len;

	/// CoAP Proxy-URI
	unsigned char				*proxy_uri;
	/// CoAP Proxy-URI length
	size_t						proxy_uri_len;
} coap_client_conf_t;

/// CoAP Client instance
typedef struct _coap_client_t
{
	/// CoAP Client state
	coap_client_state_t			state;

	/// CoAP Client name pointer
	char						*name_ptr;
	/// CoAP Client name in DPM mode
	char						dpm_conf_name[COAP_MAX_NAME_LEN];

	/// CoAP Client configuration
	coap_client_conf_t			config;

	/// CoAP observe count
	unsigned int				observe_count;
	/// CoAP observe instance 
	coap_client_obs_t			*observe_list;

	/// CoAP Client token
	unsigned char				token[COAP_CLIENT_TOKEN_LENGTH];
	/// CoAP Client message-ID
	unsigned short				msgId;

	/// CoAP connection pointer
	coap_client_conn_t			*conn_ptr;
	/// CoAP request pointer
	coap_client_req_t			*request_ptr;
} coap_client_t;

/**
 ****************************************************************************************
 * @brief Initialize CoAP Client.
 * @param[in] client_ptr    CoAP Client instance pointer
 * @param[in] name_ptr      Name of CoAP Client
 * @param[in] ip_ptr        IP instance pointer
 * @param[in] pool_ptr      Pool to allocate packet from
 * @return 0(DA_APP_SUCCESS) on success
 * @note Length of CoAP Client Name has to be less than COAP_MAX_NAME_LEN - 9.
 ****************************************************************************************
 */

int coap_client_init(coap_client_t *client_ptr, char *name_ptr);

/**
 ****************************************************************************************
 * @brief Deinitialize CoAP Client.
 * @param[in] client_ptr    CoAP Cient instance pointer
 * @return                  0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_deinit(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Set confirmable message type for CoAP Request.
 * @param[in] client_ptr    CoAP Client instance pointer
 ****************************************************************************************
 */
void coap_client_use_conf(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Set non-confirmable message type for CoAP Request.
 * @param[in] client_ptr    CoAP Client instance pointer
 ****************************************************************************************
 */
void coap_client_use_non_conf(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Set CoAP Ack timeout. Default is 1 sec.
 * @param[in] client_ptr    CoAP Client instance pointer
 * @param[in] ack_timeout   Ack timeout(Unit of time: 100 msec)
 ****************************************************************************************
 */
void coap_client_set_ack_timeout(coap_client_t *client_ptr, unsigned int ack_timeout);

/**
 ****************************************************************************************
 * @brief Set CoAP max restranmission count. Default is 4.
 * @param[in] client_ptr    CoAP Client instance pointer
 * @param[in] max_retransmit
 *                          Max retransmission count
 ****************************************************************************************
 */
void coap_client_set_max_retransmit(coap_client_t *client_ptr, unsigned int max_retransmit);

/**
 ****************************************************************************************
 * @brief Set size of CoAP block-wise for Block#1 option. Default is 6(1024 bytes).
 * @param[in] client_ptr    CoAP Client instance pointer
 * @param[in] szx           Size of CoAP block-wise for Block#1
 ****************************************************************************************
 */
void coap_client_set_block1_szx(coap_client_t *client_ptr, unsigned int szx);

/**
 ****************************************************************************************
 * @brief Set size of CoAP block-wise for Block#2 option.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] szx          Size of CoAP block-wise for Block#2
 ****************************************************************************************
 */
void coap_client_set_block2_szx(coap_client_t *client_ptr, unsigned int szx);
 
/**
 ****************************************************************************************
 * @brief Set CoAP URI.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] uri          URI of CoAP request
 * @param[in] urilen       Length of URI
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_set_uri(coap_client_t *client_ptr, unsigned char *uri, size_t urilen);

/**
 ****************************************************************************************
 * @brief Set CoAP Proxy-URI.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] uri          Proxy-URI of CoAP request
 * @param[in] urilen       Length of Proxy-URI
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_set_proxy_uri(coap_client_t *client_ptr, unsigned char *uri, size_t urilen);

/**
 ****************************************************************************************
 * @brief Set CA certificate for DTLS.
 *        It is applicable for TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8
 *        and TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] ca_cert      CA certificate pointer
 * @param[in] len          Length of CA certificate
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_ca_cert(coap_client_t *client_ptr,
							 const unsigned char *ca_cert, size_t len);

/**
 ****************************************************************************************
 * @brief Set private key for DTLS.
 *        It is applicable for TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8
 *        and TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] private_key  Private key pointer
 * @param[in] len          Length of private key
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_private_key(coap_client_t *client_ptr,
								 const unsigned char *private_key, size_t len);

/**
 ****************************************************************************************
 * @brief Set client certificate for DTLS.
 *        It is applicable for TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8
 *        and TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] cert         Client certificate pointer
 * @param[in] len          Length of Client certificate
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_cert(coap_client_t *client_ptr,
						  const unsigned char *cert, size_t len);

/**
 ****************************************************************************************
 * @brief Set a key value for a given identity for DTLS
 *        It is applicable for TLS_PSK_WITH_AES_128_CCM_8
 *        and TLS_PSK_WITH_AES_128_CBC_SHA256.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] identity     Identity associated with the key
 * @param[in] identity_len Length of Identity
 * @param[in] key          Key used to authenticate the identity
 * @param[in] key_len      Length of key
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_psk(coap_client_t *client_ptr,
						 const unsigned char *identity, size_t identity_len,
						 const unsigned char *key, size_t key_len);

/**
 ****************************************************************************************
 * @brief Set to check DTLS server's certificate validity.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] mode         Auth mode
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_authmode(coap_client_t *client_ptr, unsigned int mode);

/**
 ****************************************************************************************
 * @brief Set fragment size of DTLS.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] fragment     Size of DTLS fragment
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coaps_client_set_max_fragment(coap_client_t *client_ptr, unsigned char fragment);

/**
 ****************************************************************************************
 * @brief Set DTLS handshake timeout. Default is 1 sec.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] min          Initial timeout value
 * @param[in] max          Maximum timeout value
 ****************************************************************************************
 */
void coaps_client_set_handshake_timeout(coap_client_t *client_ptr, unsigned int min, unsigned int max);

/**
 ****************************************************************************************
 * @brief Set max reconnection count of DTLS connection. Default is 2.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] max_retransmit
 *                         Max reconnection count of DTLS connection
 ****************************************************************************************
 */
void coaps_client_set_max_secure_conn_retransmit(coap_client_t *client_ptr,
												 unsigned int max_retransmit);

/**
 ****************************************************************************************
 * @brief Send GET request.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_get(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Send GET request with specific local port.
 * @param[in] client_ptr   CoAP Client instance pointer
 * @param[in] port         Local port number
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_get_with_port(coap_client_t *client_ptr, unsigned int port);

/**
 ****************************************************************************************
 * @brief Send PUT request.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] payload     Payload pointer
 * @param[in] payload_len Length of payload
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_put(coap_client_t *client_ptr,
							unsigned char *payload, unsigned int payload_len);

/**
 ****************************************************************************************
 * @brief Send PUT request with specific local port.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] payload     Payload pointer
 * @param[in] payload_len Length of payload
 * @param[in] port        Local port number
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_put_with_port(coap_client_t *client_ptr, unsigned int port,
									  unsigned char *payload, unsigned int payload_len);

/**
 ****************************************************************************************
 * @brief Send POST request.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] payload     Payload pointer
 * @param[in] payload_len Length of payload
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_post(coap_client_t *client_ptr,
							 unsigned char *payload, unsigned int payload_len);

/**
 ****************************************************************************************
 * @brief Send POST request with specific local port.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] payload     Payload pointer
 * @param[in] payload_len Length of payload
 * @param[in] port        Local port number
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_post_with_port(coap_client_t *client_ptr, unsigned int port,
									   unsigned char *payload, unsigned int payload_len);

/**
 ****************************************************************************************
 * @brief Send DELETE request.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_delete(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Send DELETE request with specific local port.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] port        Local port number
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_request_delete_with_port(coap_client_t *client_ptr, unsigned int port);

/**
 ****************************************************************************************
 * @brief Send PING request.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_ping(coap_client_t *client_ptr);

/**
 ****************************************************************************************
 * @brief Send PING request with specific local port.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] port        Local port number
 * @return 0(DA_APP_SUCCESS) on success
 * @note URI has to be setup before sending.
 ****************************************************************************************
 */
int coap_client_ping_with_port(coap_client_t *client_ptr, unsigned int port);

/**
 ****************************************************************************************
 * @brief Receive CoAP response.
 * @param[in] client_ptr  CoAP Client instance pointer
 * @param[in] resp_ptr    CoAP Response
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_recv_response(coap_client_t *client_ptr, coap_rw_packet_t *resp_ptr);

/**
 ****************************************************************************************
 * @brief Register CoAP observe.
 *        The callback function, observe_notify,
 *        will be called when CoAP observe notification is received.
 * @param[in] client_ptr  CoAP Client instance potiner
 * @param[in] observe_notify
 *                        Callback function for CoAP observe notification
 * @param[in] observe_close_notify
 *                        Callback function for CoAP observe closing
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_set_observe_notify(coap_client_t *client_ptr,
								   int (*observe_notify)(void *client_ptr,
														 coap_rw_packet_t *resp_ptr),
								   void (*observe_close_notify)(char *timer_name));

/**
 ****************************************************************************************
 * @brief Register CoAP observe with specific local port.
 *        The callback function, observe_notify,
 *        will be called when CoAP observe notification is received.
 * @param[in] client_ptr  CoAP Client instance potiner
 * @param[in] port        Local port number
 * @param[in] observe_notify
 *                        Callback function for CoAP observe notification
 * @param[in] observe_close_notify
 *                        Callback function for CoAP observe closing
 * @return 0(DA_APP_SUCCESS) on success
 ****************************************************************************************
 */
int coap_client_set_observe_notify_with_port(coap_client_t *client_ptr, unsigned int port,
											 int (*observe_notify)(void *client_ptr,
																   coap_rw_packet_t *resp_ptr),
											 void (*observe_close_notify)(char *timer_name));

/**
 ****************************************************************************************
 * @brief Deregister CoAP observe relation.
 * @param[in] coap_client CoAP Client instance pointer
 ****************************************************************************************
 */
void coap_client_clear_observe(coap_client_t *coap_client);

#endif	// (__COAP_CLIENT_H__)

/* EOF */
