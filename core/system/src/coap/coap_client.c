/**
 ****************************************************************************************
 *
 * @file coap_client.c
 *
 * @brief CoAP Client functionality
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
#include "coap_client.h"
#include "da16x_dns_client.h"

#define	COAPC_PRINTF			PRINTF

#if defined (ENABLE_COAPC_DEBUG_INFO)
#define	COAPC_DEBUG_INFO(fmt, ...)	\
	COAPC_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPC_DEBUG_INFO(...)		do {} while (0)
#endif	// (ENABLED_COAPC_DEBUG_INFO)

#if defined (ENABLE_COAPC_DEBUG_ERR)
#define	COAPC_DEBUG_ERR(fmt, ...)	\
	COAPC_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPC_DEBUG_ERR(...)		do {} while (0)
#endif	// (ENABLED_COAPC_DEBUG_ERR) 

#if defined (ENABLE_COAPC_DEBUG_TEMP)
#define	COAPC_DEBUG_TEMP(fmt, ...)	\
	COAPC_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	COAPC_DEBUG_TEMP(...)		do {} while (0)
#endif	// (ENABLED_COAPC_DEBUG_TEMP)

//Definition
#define COAP_CLIENT_UNIT_OF_TIME			10

#define	COAP_CLIENT_MAX_HOST_LENGTH			32
#define	COAP_CLIENT_MAX_IP_CACHE			10
#define	COAP_CLIENT_MAX_IP_CACHE_LIST		2

#define	COAP_CLIENT_MAX_OBS_URI_LENGTH		128

//coap client Naming
#define	COAP_CLIENT_IP_CACHE_NAME			"coapc_ip_cache"

#define	COAP_CLIENT_REQ_SOCK(prefix, name)				\
	sprintf(name, "%s_%s", prefix, "rsock")

#define	COAP_CLIENT_OBS_THREAD_NAME(prefix, name, no)	\
	sprintf(name, "%s_%s%d", prefix, "othd", no)

#define	COAP_CLIENT_OBS_SOCK_NAME(prefix, name, no)		\
	sprintf(name, "%s_%s%d", prefix, "osock", no)

#define	COAP_CLIENT_DPM_CONF_NAME(prefix, name)			\
	sprintf(name, "%s_%s", prefix, "conf")

#define	COAP_CLIENT_OBS_TIMER_NAME(idx, name)			\
	sprintf(name, "obs_tm%d", idx)

typedef struct _coap_client_ip_cache_t {
	unsigned int idx;

	struct coap_client_ip_cache_item {
		unsigned int cur_pos;
		unsigned int cnt;
		char host_name[COAP_CLIENT_MAX_HOST_LENGTH];
		unsigned long ip_addr[COAP_CLIENT_MAX_IP_CACHE];
	} items[COAP_CLIENT_MAX_IP_CACHE_LIST];
} coap_client_ip_cache_t;

typedef struct _coap_client_dpm_obs_req_info_t {
	unsigned char token[COAP_CLIENT_TOKEN_LENGTH];
	unsigned short msgId;

	unsigned char uri[COAP_CLIENT_MAX_OBS_URI_LENGTH];
	size_t urilen;

	unsigned char proxy_uri[COAP_CLIENT_MAX_OBS_URI_LENGTH];
	size_t proxy_urilen;

	unsigned long notify_callback;
	unsigned long close_callback;
} coap_client_dpm_obs_req_info_t;

typedef struct _coap_client_dpm_obs_conf_t {
	unsigned int idx;			//obs_list's index.
	unsigned int state;			//0: Not registered. 1: registered.

	unsigned int local_port;	//local port

	coap_client_dpm_obs_req_info_t req_info;	//request info
} coap_client_dpm_obs_conf_t;

typedef struct _coap_client_dpm_conf_t {
	unsigned char token[COAP_CLIENT_TOKEN_LENGTH];
	unsigned short msgId;

	coap_client_dpm_obs_conf_t obs_list[COAP_CLIENT_MAX_OBSERVE_LIST];
} coap_client_dpm_conf_t;

typedef struct _coap_client_opt_t {
	uint8_t num;
	coap_rw_buffer_t value;
	struct _coap_client_opt_t *next;
} coap_client_opt_t;

//Extern function
extern int chk_dpm_rcv_ready(char *mod_name);
extern void da16x_secure_module_deinit(void);
extern void da16x_secure_module_init(void);

//Internal function.
int coap_client_send_request(coap_client_t *client_ptr, unsigned int port,
							 coap_method_t method,
							 unsigned char *payload, size_t payload_len);
int coap_client_recv_resp(coap_client_conf_t *config_ptr,
						  coap_client_conn_t *conn_ptr,
						  coap_client_req_t *request_ptr,
						  coap_rw_packet_t *resp_ptr,
						  unsigned int retransmit);
int coap_client_send_ack(coap_client_conn_t *conn,
						 const unsigned char *token,
						 const unsigned char *msgId,
						 coap_rw_buffer_t *payload);
int coap_client_send_rst(coap_client_conn_t *conn, const unsigned short msgId);
coap_client_opt_t *coap_client_make_option(uint8_t num, char *value,
		size_t value_len);
void coap_client_release_option(coap_client_opt_t *option);
int coap_client_make_request(coap_client_req_t *request,
							 unsigned char *out, unsigned int *out_len);
int coap_client_recover_observe(coap_client_t *client_ptr);
int coap_client_release_observe(coap_client_obs_t *obs_ptr);
void coap_client_observe_processor(void *params);
coap_client_dpm_conf_t *coap_client_dpm_get_config(char *name);
int coap_client_dpm_clear_config(char *name);
int coap_client_dpm_set_msgId(char *name, unsigned short msgId);
int coap_client_dpm_get_msgId(char *name, unsigned short *msgId);
int coap_client_dpm_set_token(char *name, unsigned char *token);
int coap_client_dpm_get_token(char *name, unsigned char *token);
coap_client_dpm_obs_conf_t *coap_client_dpm_get_obs_config(char *name);
coap_client_dpm_obs_conf_t *coap_client_dpm_get_empty_obs_config(char *name);
int coap_client_dpm_clear_obs_config(char *name, int idx);
int coap_client_dpm_set_obs_msgId(char *name, int idx, unsigned short msgId);
int coap_client_register_observe_timer(coap_client_obs_t *observe_ptr);
int coap_client_unregister_observe_timer(coap_client_obs_t *observe_ptr);
int coap_client_get_ip_info(char *uri, size_t uri_len, unsigned long *ip_addr,
							unsigned int *port);
int coap_client_update_ip_info(char *uri, size_t uri_len,
							   unsigned long *ip_addr, unsigned int *port);
int coap_client_load_ip_cache();
int coap_client_release_ip_cache();
int coap_client_get_ip_addr(char *host, size_t host_len,
							unsigned long *ip_addr);
int coap_client_parse_uri(unsigned char *uri, size_t uri_len,
						  coap_rw_buffer_t *scheme,
						  coap_rw_buffer_t *host,
						  coap_rw_buffer_t *path,
						  coap_rw_buffer_t *query,
						  unsigned long *ip_addr,
						  unsigned int *port,
						  unsigned int *secure_conn);
void coap_client_clear_request(coap_client_req_t *request_ptr);
int coap_client_init_connection(coap_client_conn_t *conn_ptr,
								char *name_ptr, unsigned int port,
								unsigned long peer_ip_addr, unsigned int peer_port,
								unsigned int secure_conn);
int coap_client_deinit_connection(coap_client_conn_t *conn_ptr);
void coap_client_init_msgId(unsigned short *msgId);
void coap_client_increase_msgId(unsigned short *msgId);
void coap_client_init_token(unsigned char *token);
void coap_client_inc_token(unsigned char *token);
int coaps_client_init(coap_client_conn_t *conn_ptr,
					  coap_client_conf_t *config_ptr);
int coaps_client_deinit(coap_client_conn_t *conn_ptr);
int coaps_client_do_handshake(coap_client_conn_t *conn_ptr,
							  unsigned int secure_conn_max_retransmit);
void coaps_client_send_close_notify(coap_client_conn_t *conn_ptr);
int coaps_client_save_ssl(coap_client_conn_t *conn_ptr);
int coaps_client_restore_ssl(coap_client_conn_t *conn_ptr);
int coaps_client_clear_ssl(coap_client_conn_t *conn_ptr);
void coap_client_dtls_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms);
int coap_client_dtls_timer_get_status(void *ctx);
int coap_client_send(coap_client_conn_t *conn, unsigned char *data, size_t len);
int coap_client_recv(coap_client_conn_t *conn,
					 unsigned char *buf, size_t buflen,
					 unsigned int wait_option);

//Debug function.
static void coap_client_secure_debug(void *ctx, int level, const char *file,
									 int line, const char *str);
#if defined (ENABLE_COAPC_DEBUG_INFO)
static void coap_client_print_conn(coap_client_conn_t *conn_ptr);
static void coap_client_print_req(coap_client_req_t *request_ptr);
#endif	// (ENABLE_COAPC_DEBUG_INFO)

//Global variable
coap_client_ip_cache_t *g_coap_client_ip_cache = NULL;

//Function body
int coap_client_init(coap_client_t *client_ptr, char *name_ptr)
{
	if (client_ptr->state != READY) {
		COAPC_DEBUG_ERR("Already init coap client\r\n");
		return DA_APP_ALREADY_ENABLED;
	}

	if (!name_ptr && strlen((char *)name_ptr) > COAP_CLIENT_MAX_SVCS_NAME) {
		COAPC_DEBUG_ERR("name should be less than %d characters(%s).\r\n",
						COAP_CLIENT_MAX_SVCS_NAME, name_ptr);
		return DA_APP_INVALID_PARAMETERS;
	}

	memset(client_ptr, 0x00, sizeof(coap_client_t));

	client_ptr->name_ptr = name_ptr;
	COAP_CLIENT_DPM_CONF_NAME((char *)client_ptr->name_ptr,
							  (char *)client_ptr->dpm_conf_name);

	client_ptr->state = SUSPEND;

	//default configruation
	client_ptr->config.ack_timeout = COAP_CLIENT_ACK_TIMEOUT;
	client_ptr->config.max_retransmit = COAP_CLIENT_MAX_RETRANSMIT;
	client_ptr->config.block1_szx = COAP_CLIENT_BLOCK_SIZE;
	client_ptr->config.block2_szx = COAP_CLIENT_BLOCK_SIZE;

	client_ptr->config.msg_type = COAP_MSG_TYPE_CON;
	client_ptr->config.authmode = pdTRUE;
	client_ptr->config.fragment = 0;
	client_ptr->config.handshake_min_timeout = COAPS_CLIENT_HANDSHAKE_MIN_TIMEOUT;
	client_ptr->config.handshake_max_timeout = COAPS_CLIENT_HANDSHAKE_MAX_TIMEOUT;
	client_ptr->config.secure_conn_max_retransmit =
		COAPS_CLIENT_CONNECTION_MAX_RETRANSMIT;

	client_ptr->observe_count = 0;
	client_ptr->observe_list = NULL;

	//default ip cache
	coap_client_load_ip_cache();

	//init cc312
	da16x_secure_module_init();

	//init token & msgId
	if (!dpm_mode_is_enabled()) {
		coap_client_init_token(client_ptr->token);
		coap_client_init_msgId(&client_ptr->msgId);
	} else {
		if (coap_client_dpm_get_token(client_ptr->dpm_conf_name, client_ptr->token)) {
			coap_client_init_token(client_ptr->token);
			coap_client_dpm_set_token(client_ptr->dpm_conf_name, client_ptr->token);
		}

		if (coap_client_dpm_get_msgId(client_ptr->dpm_conf_name, &client_ptr->msgId)) {
			coap_client_init_msgId(&client_ptr->msgId);
			coap_client_dpm_set_msgId(client_ptr->dpm_conf_name, client_ptr->msgId);
		}

		coap_client_recover_observe(client_ptr);
	}

	return DA_APP_SUCCESS;
}

int coap_client_deinit(coap_client_t *client_ptr)
{
	if (client_ptr->state != SUSPEND) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");

		return DA_APP_IN_PROGRESS;
	}

	coap_client_release_ip_cache();

	coap_client_clear_observe(client_ptr);

	if (client_ptr->conn_ptr) {
		if (client_ptr->conn_ptr->secure_conn) {
			coaps_client_deinit(client_ptr->conn_ptr);
		}

		coap_client_deinit_connection(client_ptr->conn_ptr);
		coap_free(client_ptr->conn_ptr);
	}

	if (client_ptr->config.uri) {
		coap_free(client_ptr->config.uri);
	}

	if (client_ptr->config.proxy_uri) {
		coap_free(client_ptr->config.proxy_uri);
	}

	coap_client_dpm_clear_config(client_ptr->dpm_conf_name);

	memset(client_ptr, 0x00, sizeof(coap_client_t));

	//deinit cc312
	da16x_secure_module_deinit();

	return DA_APP_SUCCESS;
}

void coap_client_use_conf(coap_client_t *client_ptr)
{
	client_ptr->config.msg_type = COAP_MSG_TYPE_CON;
}

void coap_client_use_non_conf(coap_client_t *client_ptr)
{
	client_ptr->config.msg_type = COAP_MSG_TYPE_NONCON;
}

void coap_client_set_ack_timeout(coap_client_t *client_ptr,
								 unsigned int ack_timeout)
{
	client_ptr->config.ack_timeout = ack_timeout;
}

void coap_client_set_max_retransmit(coap_client_t *client_ptr,
									unsigned int max_retransmit)
{
	client_ptr->config.max_retransmit = max_retransmit;
}

void coap_client_set_block1_szx(coap_client_t *client_ptr, unsigned int szx)
{
	if (0 < szx && szx < 7) {
		client_ptr->config.block1_szx = szx;
	}
}

void coap_client_set_block2_szx(coap_client_t *client_ptr, unsigned int szx)
{
	if (0 < szx && szx < 7) {
		client_ptr->config.block2_szx = szx;
	}
}

int coap_client_set_uri(coap_client_t *client_ptr, unsigned char *uri,
						size_t urilen)
{
	coap_client_conf_t *config_ptr = &client_ptr->config;

	if (uri == NULL || urilen == 0) {
		return DA_APP_INVALID_PARAMETERS;
	}

	//Release previous uri information
	if (config_ptr->uri) {
		coap_free(config_ptr->uri);
		config_ptr->uri = NULL;
	}

	config_ptr->uri_len = 0;

	config_ptr->uri = coap_calloc(1, urilen);
	if (!config_ptr->uri) {
		COAPC_DEBUG_ERR("failed to allocate memory for uri\r\n");
		return DA_APP_NOT_CREATED;
	}

	memcpy(config_ptr->uri, uri, urilen);
	config_ptr->uri_len = urilen;

	return DA_APP_SUCCESS;
}

int coap_client_set_proxy_uri(coap_client_t *client_ptr, unsigned char *uri,
							  size_t urilen)
{
	coap_client_conf_t *config_ptr = &client_ptr->config;

	//Release previous uri information
	if (config_ptr->proxy_uri) {
		coap_free(config_ptr->proxy_uri);
		config_ptr->proxy_uri = NULL;
	}

	config_ptr->proxy_uri_len = 0;

	if (uri == NULL || urilen == 0) {
		COAPC_DEBUG_INFO("clear proxy-uri\r\n");
		return DA_APP_SUCCESS;
	}

	config_ptr->proxy_uri = coap_calloc(1, urilen);
	if (!config_ptr->proxy_uri) {
		COAPC_DEBUG_ERR("failed to allocate memory for proxy-uri\r\n");
		return DA_APP_NOT_CREATED;
	}

	memcpy(config_ptr->proxy_uri, uri, urilen);
	config_ptr->proxy_uri_len = urilen;

	return DA_APP_SUCCESS;
}

int coaps_client_set_ca_cert(coap_client_t *client_ptr,
							 const unsigned char *ca_cert, size_t len)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

	client_ptr->config.ca_cert = ca_cert;
	client_ptr->config.ca_cert_len = len;

	return DA_APP_SUCCESS;
}

int coaps_client_set_private_key(coap_client_t *client_ptr,
								 const unsigned char *private_key, size_t len)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

	client_ptr->config.pkey = private_key;
	client_ptr->config.pkey_len = len;

	return DA_APP_SUCCESS;
}

int coaps_client_set_cert(coap_client_t *client_ptr,
						  const unsigned char *cert, size_t len)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

	client_ptr->config.cert = cert;
	client_ptr->config.cert_len = len;

	return DA_APP_SUCCESS;
}

int coaps_client_set_psk(coap_client_t *client_ptr,
						 const unsigned char *identity, size_t identity_len,
						 const unsigned char *key, size_t key_len)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

#if defined (MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	client_ptr->config.psk_identity = identity;
	client_ptr->config.psk_identity_len = identity_len;

	client_ptr->config.psk_key = key;
	client_ptr->config.psk_key_len = key_len;

	return DA_APP_SUCCESS;
#else
	LWIP_UNUSED_ARG(identity);
	LWIP_UNUSED_ARG(identity_len);
	LWIP_UNUSED_ARG(key);
	LWIP_UNUSED_ARG(key_len);

	return DA_APP_NOT_SUPPORTED;
#endif	// (MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
}

int coaps_client_set_authmode(coap_client_t *client_ptr, unsigned int mode)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

	client_ptr->config.authmode = mode;

	return DA_APP_SUCCESS;
}

int coaps_client_set_max_fragment(coap_client_t *client_ptr,
								  unsigned char fragment)
{
	if (client_ptr->state == IN_PROGRESS || client_ptr->conn_ptr) {
		COAPC_DEBUG_INFO("coap client is in progress.\r\n");
		return DA_APP_IN_PROGRESS;
	}

	client_ptr->config.fragment = fragment;

	return DA_APP_SUCCESS;
}

void coaps_client_set_handshake_timeout(coap_client_t *client_ptr,
										unsigned int min, unsigned int max)
{
	client_ptr->config.handshake_min_timeout = min;
	client_ptr->config.handshake_max_timeout = max;
}

void coaps_client_set_max_secure_conn_retransmit(coap_client_t *client_ptr,
		unsigned int max_retransmit)
{
	client_ptr->config.secure_conn_max_retransmit = max_retransmit;
}

int coap_client_request_get(coap_client_t *client_ptr)
{
	return coap_client_request_get_with_port(client_ptr, 0);
}

int coap_client_request_get_with_port(coap_client_t *client_ptr, unsigned int port)
{
	return coap_client_send_request(client_ptr, port, COAP_METHOD_GET, NULL, 0);
}

int coap_client_request_put(coap_client_t *client_ptr,
							unsigned char *payload, unsigned int payload_len)
{
	return coap_client_request_put_with_port(client_ptr, 0, payload, payload_len);
}

int coap_client_request_put_with_port(coap_client_t *client_ptr,
									  unsigned int port,
									  unsigned char *payload, unsigned int payload_len)
{
	return coap_client_send_request(client_ptr, port,
									COAP_METHOD_PUT, payload, payload_len);
}

int coap_client_request_post(coap_client_t *client_ptr,
							 unsigned char *payload, unsigned int payload_len)
{
	return coap_client_request_post_with_port(client_ptr, 0, payload, payload_len);
}

int coap_client_request_post_with_port(coap_client_t *client_ptr,
									   unsigned int port,
									   unsigned char *payload, unsigned int payload_len)
{
	return coap_client_send_request(client_ptr, port,
									COAP_METHOD_POST, payload, payload_len);
}

int coap_client_request_delete(coap_client_t *client_ptr)
{
	return coap_client_request_delete_with_port(client_ptr, 0);
}

int coap_client_request_delete_with_port(coap_client_t *client_ptr,
		unsigned int port)
{
	return coap_client_send_request(client_ptr, port, COAP_METHOD_DELETE, NULL, 0);
}

int coap_client_ping(coap_client_t *client_ptr)
{
	return coap_client_ping_with_port(client_ptr, 0);
}

int coap_client_ping_with_port(coap_client_t *client_ptr, unsigned int port)
{
	int ret = DA_APP_SUCCESS;
	coap_rw_packet_t resp_packet;

	memset(&resp_packet, 0x00, sizeof(coap_rw_packet_t));

	ret = coap_client_send_request(client_ptr, port, COAP_METHOD_NONE, NULL, 0);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to progress PING request(%d)\r\n", ret);
		return ret;
	}

	ret = coap_client_recv_response(client_ptr, &resp_packet);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to receive response(%d)\r\n", ret);
	}

	coap_clear_rw_packet(&resp_packet);

	return ret;
}

int coap_client_set_observe_notify(coap_client_t *client_ptr,
								   int (*observe_notify)(void *client_ptr,
														 coap_rw_packet_t *resp_ptr),
								   void (*observe_close_notify)(char *timer_name))
{
	return coap_client_set_observe_notify_with_port(client_ptr, 0,
													observe_notify, observe_close_notify);
}

int coap_client_set_observe_notify_with_port(coap_client_t *client_ptr,
											 unsigned int port,
											 int (*observe_notify)(void *client_ptr,
																   coap_rw_packet_t *resp_ptr),
											 void (*observe_close_notify)(char *timer_name))
{
	int ret = DA_APP_SUCCESS;
	char socket_name[COAP_MAX_NAME_LEN] = {0x00,};

	unsigned long peer_ip_addr;
	unsigned int secure_conn;

	unsigned char pck_buf[COAP_CLIENT_SEND_BUF_SIZE] = {0x00,};
	size_t pck_len = 0;

	coap_client_conf_t *config_ptr = &client_ptr->config;
	coap_client_obs_t *observe_ptr = NULL;

	coap_rw_packet_t response;

	//check validation
	if (client_ptr->state != SUSPEND) {
		COAPC_DEBUG_ERR("coap client is not ready to registe obs\r\n");
		return DA_APP_CANNOT_START;
	}

	if (!config_ptr->uri || config_ptr->uri_len == 0) {
		COAPC_DEBUG_ERR("uri is not setup\r\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (client_ptr->observe_count >= COAP_CLIENT_MAX_OBSERVE_LIST) {
		COAPC_DEBUG_ERR("max observe(%d)\r\n", client_ptr->observe_count);
		return DA_APP_NOT_CREATED;
	}

	memset(&response, 0x00, sizeof(coap_rw_packet_t));

	//create obs structure
	observe_ptr = coap_calloc(1, sizeof(coap_client_obs_t));
	if (observe_ptr == NULL) {
		COAPC_DEBUG_ERR("failed to create observe\r\n");
		client_ptr->state = SUSPEND;
		return DA_APP_MALLOC_ERROR;
	}

	ret = coap_client_parse_uri((unsigned char *)config_ptr->uri,
								config_ptr->uri_len,
								&observe_ptr->request.scheme,
								&observe_ptr->request.host,
								&observe_ptr->request.path,
								&observe_ptr->request.query,
								&peer_ip_addr,
								&observe_ptr->request.port,
								&secure_conn);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to set uri(0x%x)\r\n", -ret);
		coap_client_clear_request(&observe_ptr->request);
		coap_free(observe_ptr);
		client_ptr->state = SUSPEND;
		return ret;
	}

	observe_ptr->id = (int)(client_ptr->observe_count);

	COAP_CLIENT_OBS_TIMER_NAME(observe_ptr->id, observe_ptr->close_timer_name);

	//set connection
	COAP_CLIENT_OBS_SOCK_NAME(client_ptr->name_ptr, socket_name, observe_ptr->id);

	ret = coap_client_init_connection(&observe_ptr->conn,
									  socket_name,
									  port,
									  peer_ip_addr,
									  observe_ptr->request.port,
									  secure_conn);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to init connection(0x%x)\r\n", ret);
		coap_client_clear_request(&observe_ptr->request);
		coap_free(observe_ptr);
		client_ptr->state = SUSPEND;
		return ret;
	}

	//establish dtls session
	if (secure_conn) {
		ret = coaps_client_init(&observe_ptr->conn, config_ptr);
		if (ret != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("failed to init secure mode(0x%x)\r\n", ret);
			coap_client_deinit_connection(&observe_ptr->conn);
			coap_client_clear_request(&observe_ptr->request);
			coap_free(observe_ptr);
			client_ptr->state = SUSPEND;
			return ret;
		}

		ret = coaps_client_do_handshake(&observe_ptr->conn,
										config_ptr->secure_conn_max_retransmit);
		if (ret != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("failed to progress dtls handshake(0x%x)\r\n", -ret);
			coaps_client_deinit(&observe_ptr->conn);
			coap_client_deinit_connection(&observe_ptr->conn);
			coap_client_clear_request(&observe_ptr->request);
			coap_free(observe_ptr);
			client_ptr->state = SUSPEND;
			return ret;
		}
	}

	//set observe config
	observe_ptr->status = REQ_REGISTER;
	observe_ptr->client_ptr = (void *)client_ptr;
	observe_ptr->notify = observe_notify;
	observe_ptr->close_notify = observe_close_notify;
	observe_ptr->next = NULL;

	coap_client_init_token(observe_ptr->token);
	coap_client_increase_msgId(&observe_ptr->msgId);

	COAP_CLIENT_OBS_THREAD_NAME(client_ptr->name_ptr,
								observe_ptr->thread_name, observe_ptr->id);

	observe_ptr->dpm_idx = -1;

	//set request
	observe_ptr->request.method = COAP_METHOD_GET;
	observe_ptr->request.type = config_ptr->msg_type;

	observe_ptr->request.observe_reg = 1;

	observe_ptr->request.payload.p = NULL;
	observe_ptr->request.payload.len = 0;

	observe_ptr->request.block1_num = 0;
	observe_ptr->request.block1_more = 0;
	observe_ptr->request.block1_szx = (uint8_t)config_ptr->block1_szx;

	observe_ptr->request.block2_num = 0;
	observe_ptr->request.block2_more = 0;
	observe_ptr->request.block2_szx = (uint8_t)config_ptr->block2_szx;

	if (config_ptr->proxy_uri_len) {
		coap_set_rw_buffer(&observe_ptr->request.proxy_uri,
						   (const char *)config_ptr->proxy_uri,
						   config_ptr->proxy_uri_len);
	}

	observe_ptr->request.token_ptr = observe_ptr->token;
	observe_ptr->request.msgId_ptr = &observe_ptr->msgId;

#if defined (ENABLE_COAPC_DEBUG_INFO)
	coap_client_print_conn(&observe_ptr->conn);
	coap_client_print_req(&observe_ptr->request);
#endif	// (ENABLE_COAPC_DEBUG_INFO)

	ret = coap_client_make_request(&observe_ptr->request, pck_buf, &pck_len);
	if ((ret != DA_APP_SUCCESS) || (pck_len == 0)) {
		COAPC_DEBUG_ERR("failed to generate request(0x%x, len:%ld)\r\n", -ret, pck_len);
		goto disconn;
	}

	ret = coap_client_send(&observe_ptr->conn, pck_buf, pck_len);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to send request(0x%x)\r\n", -ret);
		goto disconn;
	}

	ret = coap_client_recv_resp(config_ptr,
								&observe_ptr->conn,
								&observe_ptr->request,
								&response,
								pdTRUE);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to receive response(0x%x)\r\n", -ret);
		goto disconn;
	}

	if ((response.header.code != COAP_RESP_CODE_CREATED)
			&& (response.header.code != COAP_RESP_CODE_VALID)
			&& (response.header.code != COAP_RESP_CODE_CONTINUE)
			&& (response.header.code != COAP_RESP_CODE_CHANGED)
			&& (response.header.code != COAP_RESP_CODE_CONTENT)) {
		COAPC_DEBUG_ERR("Received Error code(0x%x)\n", response.header.code);
		ret = DA_APP_NOT_CREATED;
		goto disconn;
	}

	//Save observe config for dpm
	if (dpm_mode_is_enabled()) {
		//Register dpm
		dpm_app_register(observe_ptr->thread_name, observe_ptr->conn.local_port);

		//set udp filter for dpm wakeup
		dpm_udp_port_filter_set((unsigned short)(observe_ptr->conn.local_port));

		//save dtls session
		if (observe_ptr->conn.secure_conn) {
			coaps_client_save_ssl(&observe_ptr->conn);
		}

		//save observe info
		coap_client_dpm_obs_conf_t *obs_dpm_conf = NULL;

		obs_dpm_conf = coap_client_dpm_get_empty_obs_config(client_ptr->dpm_conf_name);

		if (obs_dpm_conf) {
			//state
			obs_dpm_conf->state = 1;

			//connection
			obs_dpm_conf->local_port = observe_ptr->conn.local_port;

			//request
			memcpy(obs_dpm_conf->req_info.token,
				   observe_ptr->token,
				   COAP_CLIENT_TOKEN_LENGTH);
			obs_dpm_conf->req_info.msgId = observe_ptr->msgId;
			memcpy(obs_dpm_conf->req_info.uri,
				   config_ptr->uri,
				   config_ptr->uri_len);
			obs_dpm_conf->req_info.urilen = config_ptr->uri_len;

			if (config_ptr->proxy_uri_len) {
				memcpy(obs_dpm_conf->req_info.proxy_uri,
					   config_ptr->proxy_uri,
					   config_ptr->proxy_uri_len);
				obs_dpm_conf->req_info.proxy_urilen = config_ptr->proxy_uri_len;
			} else {
				memset(obs_dpm_conf->req_info.proxy_uri,
					   0x00,
					   COAP_CLIENT_MAX_OBS_URI_LENGTH);
				obs_dpm_conf->req_info.proxy_urilen = 0;
			}

			obs_dpm_conf->req_info.notify_callback = (unsigned long)observe_ptr->notify;
			obs_dpm_conf->req_info.close_callback = (unsigned long)observe_ptr->close_notify;

			observe_ptr->dpm_idx = (int)(obs_dpm_conf->idx);
		} else {
			COAPC_DEBUG_ERR("failed to get CoAP observe configuration for DPM\r\n");
		}
	}

	//parse max age
	for (int opt_idx = 0 ; opt_idx < response.numopts ; opt_idx++) {
		coap_rw_option_t *opt_ptr = &(response.opts[opt_idx]);
		int max_age = 0;

		switch (opt_ptr->num) {
			case COAP_OPTION_MAX_AGE:
				for (unsigned int idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
					max_age = (max_age << 8) | opt_ptr->buf.p[idx];
				}

				COAPC_DEBUG_INFO("Max-age(%d)\r\n", max_age);

				observe_ptr->max_age = (unsigned int)max_age;

				//update close notify timer
				ret = coap_client_register_observe_timer(observe_ptr);
				if (ret != DA_APP_SUCCESS) {
					COAPC_DEBUG_ERR("failed to register observe close timer(0x%x)\r\n", -ret);
				}
				break;
			default:
				break;
		}
	}

	if (observe_ptr->notify) {
		observe_ptr->notify(client_ptr, &response);
	}

	observe_ptr->status = REGISTERED;

	ret = xTaskCreate(coap_client_observe_processor,
					  observe_ptr->thread_name,
					  COAP_CLIENT_OBS_STACK_SIZE,
					  (void *)observe_ptr,
					  OS_TASK_PRIORITY_USER,
					  &observe_ptr->task_handler);
	if (ret != pdPASS) {
		COAPC_DEBUG_ERR("failed to create observe thread(%d)\r\n", ret);
		ret = DA_APP_NOT_CREATED;
		goto disconn;
	}

	//update observe list
	if (client_ptr->observe_list) {
		coap_client_obs_t *last = NULL;

		for (last = client_ptr->observe_list ; last->next != NULL ; last = last->next);

		last->next = observe_ptr;
	} else {
		client_ptr->observe_list = observe_ptr;
	}

	client_ptr->observe_count++;

	//waiting for recv ready flag.
	if (dpm_mode_is_enabled()) {
		while (!chk_dpm_rcv_ready(observe_ptr->thread_name)) {
			vTaskDelay(10);
		}
	}

	client_ptr->state = SUSPEND;

	return DA_APP_SUCCESS;

disconn:

	if (observe_ptr) {
		coap_client_unregister_observe_timer(observe_ptr);

		if (dpm_mode_is_enabled()) {
			coap_client_dpm_clear_obs_config(client_ptr->dpm_conf_name,
											 observe_ptr->dpm_idx);
			dpm_app_unregister((char *)observe_ptr->thread_name);
		}

		if (observe_ptr->conn.secure_conn) {
			coaps_client_deinit(&observe_ptr->conn);
		}

		coap_client_deinit_connection(&observe_ptr->conn);
		coap_client_clear_request(&observe_ptr->request);
		coap_free(observe_ptr);
		client_ptr->state = SUSPEND;
	}

	client_ptr->state = SUSPEND;

	return ret;
}

void coap_client_clear_observe(coap_client_t *coap_client)
{
	coap_client_obs_t *del_obs = NULL;

	while (coap_client->observe_count) {
		del_obs = coap_client->observe_list;
		coap_client_release_observe(del_obs);
		coap_client->observe_list = coap_client->observe_list->next;
		coap_free(del_obs);

		coap_client->observe_count--;
	}

	coap_client->observe_list = NULL;
	coap_client->observe_count = 0;

	return ;
}

int coap_client_recv_response(coap_client_t *client_ptr, coap_rw_packet_t *resp_ptr)
{
	int ret = DA_APP_SUCCESS;

	ret = coap_client_recv_resp(&client_ptr->config, client_ptr->conn_ptr,
								client_ptr->request_ptr, resp_ptr, pdTRUE);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("Failed to receive response(0x%x)\r\n", -ret);

		if (client_ptr->conn_ptr) {
			if (client_ptr->conn_ptr->secure_conn) {
				coaps_client_deinit(client_ptr->conn_ptr);
			}

			coap_client_deinit_connection(client_ptr->conn_ptr);
			coap_free(client_ptr->conn_ptr);
			client_ptr->conn_ptr = NULL;
		}
	} else {
		if (client_ptr->conn_ptr->secure_conn) {
			coaps_client_save_ssl(client_ptr->conn_ptr);
		}
	}

	if (client_ptr->request_ptr) {
		coap_client_clear_request(client_ptr->request_ptr);
		coap_free(client_ptr->request_ptr);
		client_ptr->request_ptr = NULL;
	}

	if (dpm_mode_is_enabled()) {
		coap_client_dpm_set_token(client_ptr->dpm_conf_name, client_ptr->token);
		coap_client_dpm_set_msgId(client_ptr->dpm_conf_name, client_ptr->msgId);
	}

	return ret;
}

//internal
int coap_client_send_request(coap_client_t *client_ptr, unsigned int port,
							 coap_method_t method,
							 unsigned char *payload, size_t payload_len)
{
	int ret = DA_APP_SUCCESS;
	char socket_name[COAP_MAX_NAME_LEN] = {0x00,};

	unsigned long peer_ip_addr;
	unsigned int peer_port;
	unsigned int secure_conn;

	coap_client_conf_t *config_ptr = &client_ptr->config;

	unsigned char pck_buf[COAP_CLIENT_SEND_BUF_SIZE] = {0x00,};
	size_t pck_len = 0;

	if ((client_ptr->state != SUSPEND) || (client_ptr->request_ptr)) {
		COAPC_DEBUG_ERR("coap client is not ready to request method(%d,%d)\r\n",
						client_ptr->state, method);
		return DA_APP_IN_PROGRESS;
	}

	if (!config_ptr->uri || config_ptr->uri_len == 0) {
		COAPC_DEBUG_ERR("uri is not setup\r\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	client_ptr->request_ptr = coap_calloc(1, sizeof(coap_client_req_t));
	if (client_ptr->request_ptr == NULL) {
		COAPC_DEBUG_ERR("failed to allocate memory for coap request\r\n");
		return DA_APP_MALLOC_ERROR;
	}

	client_ptr->state = IN_PROGRESS;

	ret = coap_client_parse_uri((unsigned char *)config_ptr->uri,
								config_ptr->uri_len,
								&client_ptr->request_ptr->scheme,
								&client_ptr->request_ptr->host,
								&client_ptr->request_ptr->path,
								&client_ptr->request_ptr->query,
								&peer_ip_addr,
								&client_ptr->request_ptr->port,
								&secure_conn);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to parse uri(%d)\r\n", ret);
		goto clean_up;
	}

	peer_port = client_ptr->request_ptr->port;

	//Set connection
	if (client_ptr->conn_ptr) {
		if ((client_ptr->conn_ptr->peer_ip_addr == peer_ip_addr)
			&& (client_ptr->conn_ptr->peer_port == peer_port)
			&& (client_ptr->conn_ptr->secure_conn == secure_conn)
			&& (client_ptr->conn_ptr->local_port == port)) {
			COAPC_DEBUG_INFO("Found previous connection\r\n");
		} else {
			COAPC_DEBUG_INFO("Release previous connection\r\n");

			//Release previous connection
			if (client_ptr->conn_ptr->secure_conn) {
				coaps_client_deinit(client_ptr->conn_ptr);
			}

			coap_client_deinit_connection(client_ptr->conn_ptr);
			coap_free(client_ptr->conn_ptr);
			client_ptr->conn_ptr = NULL;
		}
	}

	if (!client_ptr->conn_ptr) {
		client_ptr->conn_ptr = coap_calloc(1, sizeof(coap_client_conn_t));
		if (client_ptr->conn_ptr == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for coap conn\r\n");
			ret = DA_APP_NOT_CREATED;
			goto clean_up;
		}

		COAP_CLIENT_REQ_SOCK(client_ptr->name_ptr, socket_name);

		ret = coap_client_init_connection(client_ptr->conn_ptr,
										  socket_name, port,
										  peer_ip_addr,
										  peer_port,
										  secure_conn);
		if (ret != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("failed to init connection(%d)\r\n", ret);
			goto disconn;
		}

		//dtls handshake
		if (secure_conn) {
			ret = coaps_client_init(client_ptr->conn_ptr, config_ptr);
			if (ret != DA_APP_SUCCESS) {
				COAPC_DEBUG_ERR("failed to init secure mode(%d)\r\n", ret);
				goto disconn;
			}

			ret = coaps_client_do_handshake(client_ptr->conn_ptr,
											config_ptr->secure_conn_max_retransmit);
			if (ret != DA_APP_SUCCESS) {
				COAPC_DEBUG_ERR("failed to progress dtls handshake(0x%x)\r\n", -ret);
				goto disconn;
			}
		}
	}

	//set request
	client_ptr->request_ptr->method = method;
	client_ptr->request_ptr->type = config_ptr->msg_type;

	client_ptr->request_ptr->observe_reg = 0;

	client_ptr->request_ptr->payload.p = payload;
	client_ptr->request_ptr->payload.len = payload_len;

	client_ptr->request_ptr->block1_num = 0;

	if (payload_len) {
		client_ptr->request_ptr->block1_more = 1;
	} else {
		client_ptr->request_ptr->block1_more = 0;
	}

	client_ptr->request_ptr->block1_szx = (uint8_t)config_ptr->block1_szx;

	client_ptr->request_ptr->block2_num = 0;
	client_ptr->request_ptr->block2_more = 0;
	client_ptr->request_ptr->block2_szx = (uint8_t)config_ptr->block2_szx;

	if (config_ptr->proxy_uri_len) {
		coap_set_rw_buffer(&client_ptr->request_ptr->proxy_uri,
						   (const char *)config_ptr->proxy_uri,
						   config_ptr->proxy_uri_len);
	}

	client_ptr->request_ptr->token_ptr = client_ptr->token;
	client_ptr->request_ptr->msgId_ptr = &client_ptr->msgId;

	coap_client_increase_msgId(&client_ptr->msgId);
	coap_client_inc_token(client_ptr->token);

#ifdef	ENABLE_COAPC_DEBUG_INFO
	coap_client_print_conn(client_ptr->conn_ptr);
	coap_client_print_req(client_ptr->request_ptr);
#endif	/* ENABLE_COAPC_DEBUG_INFO */

	ret = coap_client_make_request(client_ptr->request_ptr, pck_buf, &pck_len);
	if ((ret != DA_APP_SUCCESS) || (pck_len == 0)) {
		COAPC_DEBUG_ERR("failed to generate request (0x%x,len:%ld)\r\n", -ret, pck_len);
		goto disconn;
	}

	ret = coap_client_send(client_ptr->conn_ptr, pck_buf, pck_len);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to send request(%d)\r\n", ret);
		goto disconn;
	}

disconn:

	if (ret == DA_APP_SUCCESS) {
		if (secure_conn) {
			coaps_client_save_ssl(client_ptr->conn_ptr);
		}
	} else {
		if (secure_conn) {
			coaps_client_deinit(client_ptr->conn_ptr);
		}

		coap_client_deinit_connection(client_ptr->conn_ptr);
		coap_free(client_ptr->conn_ptr);
		client_ptr->conn_ptr = NULL;
	}

	if (dpm_mode_is_enabled()) {
		coap_client_dpm_set_token(client_ptr->dpm_conf_name, client_ptr->token);
		coap_client_dpm_set_msgId(client_ptr->dpm_conf_name, client_ptr->msgId);
	}

clean_up:

	if (ret && client_ptr->request_ptr) {
		coap_client_clear_request(client_ptr->request_ptr);
		coap_free(client_ptr->request_ptr);
		client_ptr->request_ptr = NULL;
	}

	client_ptr->state = SUSPEND;

	return ret;
}

int coap_client_recv_resp(coap_client_conf_t *config_ptr,
						  coap_client_conn_t *conn_ptr,
						  coap_client_req_t *request_ptr,
						  coap_rw_packet_t *resp_ptr,
						  unsigned int retransmit)
{
	int ret = DA_APP_SUCCESS;

	unsigned char pck_buf[COAP_CLIENT_SEND_BUF_SIZE] = {0x00,};
	unsigned int pck_len = 0;

	unsigned int loop_endlessly = pdTRUE;

	unsigned char *recv_buf = NULL;
	size_t recv_buflen = 0;
	size_t recv_len = 0;

	unsigned int retry_cnt = 0;
	unsigned long wait_option = config_ptr->ack_timeout * COAP_CLIENT_UNIT_OF_TIME;

	unsigned int got_ack = pdFALSE;

	unsigned int max_age = 0;
	unsigned int obs_num = 0;

	unsigned int opt_idx = 0;

	//parsed response info
	coap_packet_t parsed_packet;

	uint32_t block1_num = 0;
	uint8_t block1_more = 0;
	uint8_t block1_szx = 0;

	uint32_t block2_num = 0;
	uint8_t block2_more = 0;
	uint8_t block2_szx = 0;

	unsigned short prev_msgId = 0;
	unsigned short recv_msgId = 0;

	coap_rw_buffer_t payload;

	unsigned int recv_proxy_request = 0;

	unsigned int is_obs_option = pdFALSE;
	unsigned int is_duplicated = pdFALSE;

	unsigned int saved_resp = pdFALSE;

	if (!request_ptr || !conn_ptr || !config_ptr || !resp_ptr) {
		return DA_APP_INVALID_PARAMETERS;
	}

	memset(&parsed_packet, 0x00, sizeof(coap_packet_t));
	memset(&payload, 0x00, sizeof(coap_rw_buffer_t));

	if (config_ptr->block1_szx > config_ptr->block2_szx) {
		recv_buflen = (size_t)pow_long(2, (int)(config_ptr->block1_szx + 4));
	} else {
		recv_buflen = (size_t)pow_long(2, (int)(config_ptr->block2_szx + 4));
	}

	recv_buf = coap_calloc(recv_buflen, sizeof(unsigned char));
	if (recv_buf == NULL) {
		COAPC_DEBUG_ERR("Failed to allocate recv buffer(%ld)\r\n", recv_buflen);
		return DA_APP_MALLOC_ERROR;
	}

	COAPC_DEBUG_INFO("Recv buffer size(%ld)\r\n", recv_buflen);

	while (loop_endlessly) {
		recv_len = 0;

		memset(recv_buf, 0x00, recv_buflen);

		if (request_ptr->type == COAP_MSG_TYPE_NONCON) {
			got_ack = pdTRUE;
		} else {
			got_ack = pdFALSE;
		}

		//init parsed response info
		memset(&parsed_packet, 0x00, sizeof(coap_packet_t));

		block1_num = 0;
		block1_more = 0;
		block1_szx = 0;

		block2_num = 0;
		block2_more = 0;
		block2_szx = 0;

		is_obs_option = pdFALSE;
		is_duplicated = pdFALSE;

		//recv packet
		ret = coap_client_recv(conn_ptr, recv_buf, recv_buflen, wait_option);
		if (ret > 0) {
			COAPC_DEBUG_INFO("received packet(len:%ld)\r\n", ret);

			recv_len = (size_t)ret;
			ret = 0;

			//parse packet
			if (!coap_parse_raw_data(&parsed_packet, recv_buf, recv_len)) {
				saved_resp = pdTRUE;

#if _ENABLE_UNUSED_ /* not init retry count & timeout. */
				//init retry cnt & wait time
				wait_option = config_ptr->ack_timeout * COAP_CLIENT_UNIT_OF_TIME;
				retry_cnt = 0;
#endif

				COAPC_DEBUG_INFO("Copied response packet\r\n");
				coap_copy_rw_packet(resp_ptr, &parsed_packet);

#if defined (ENABLE_COAPC_DEBUG_TEMP)
				coap_print_header(&parsed_packet.header);
				coap_print_response(&parsed_packet, pdTRUE);
#endif	// (ENALBE_COAPC_DEBUG_TEMP)

				//Check header
				if (parsed_packet.header.type == COAP_MSG_TYPE_RESET) {
					COAPC_DEBUG_INFO("Received reset\r\n");
					break;
				} else if (parsed_packet.header.type == COAP_MSG_TYPE_ACK) {
					COAPC_DEBUG_INFO("Received ACK\r\n");
					got_ack = pdTRUE;
				} else if (parsed_packet.header.type == COAP_MSG_TYPE_CON) {
					//send empty ack
					ret = coap_client_send_ack(conn_ptr,
											   NULL,
											   parsed_packet.header.msg_id,
											   NULL);
					if (ret) {
						COAPC_DEBUG_ERR("failed to send ack(%d)\r\n", ret);
					}

					if (request_ptr->proxy_uri.len) {
						recv_proxy_request = 1;
					}
				}

				//check header response code
				switch (parsed_packet.header.code) {
					case COAP_RESP_CODE_CREATED:
					case COAP_RESP_CODE_DELETED:
					case COAP_RESP_CODE_VALID:
					case COAP_RESP_CODE_CONTINUE:
					case COAP_RESP_CODE_CHANGED:
					case COAP_RESP_CODE_CONTENT:
						break;

					case COAP_RESP_CODE_BAD_REQUEST:
					case COAP_RESP_CODE_UNAUTHORIZED:
					case COAP_RESP_CODE_BAD_OPTION:
					case COAP_RESP_CODE_FORBIDDEN:
					case COAP_RESP_CODE_NOT_FOUND:
					case COAP_RESP_CODE_METHOD_NOT_ALLOWED:
					case COAP_RESP_CODE_NOT_ACCEPTABLE:
					case COAP_RESP_CODE_PRECONDITION_FAILED:
					case COAP_RESP_CODE_REQUEST_ENTITY_TOO_LARGE:
					case COAP_RESP_CODE_UNSUPPORTED_CONTENT_FORMAT:
					case COAP_RESP_CODE_INTERNAL_SERVER_ERROR:
					case COAP_RESP_CODE_NOT_IMPLEMENTED:
					case COAP_RESP_CODE_BAD_GATEWAY:
					case COAP_RESP_CODE_SERVICE_UNAVAILABLE:
					case COAP_RESP_CODE_GATEWAY_TIMEOUT:
					case COAP_RESP_CODE_PROXYING_NOT_SUPPORTED:
						COAPC_DEBUG_INFO("Received Error code(0x%x)\r\n",
										 parsed_packet.header.code);
						goto end_of_func;
					default:
						break;
				}

				//check duplicated message ID
				recv_msgId = parsed_packet.header.msg_id[0] << 8;
				recv_msgId += parsed_packet.header.msg_id[1];

				if (prev_msgId == recv_msgId) {
					COAPC_DEBUG_INFO("Duplicated msgID(%d)\r\n", recv_msgId);
					continue;
				}

				prev_msgId = recv_msgId;

				//Read option
				for (opt_idx = 0 ; opt_idx < parsed_packet.numopts ; opt_idx++) {
					coap_option_t *opt_ptr = &(parsed_packet.opts[opt_idx]);
					unsigned int idx = 0;

					switch (opt_ptr->num) {
						case COAP_OPTION_OBSERVE:
							for (idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
								obs_num = (obs_num << 8) | opt_ptr->buf.p[idx];
							}

							is_obs_option = pdTRUE;
							COAPC_DEBUG_INFO("Observe(num:%d,len:%d)\r\n",
											 obs_num, opt_ptr->buf.len);
							break;
						case COAP_OPTION_BLOCK_1:
							coap_get_block_info(opt_ptr, &block1_num,
												&block1_more, &block1_szx);

							COAPC_DEBUG_INFO("Block#1(%d:%d:%d)\r\n",
											 block1_num, block1_more, block1_szx);

							if (request_ptr->block1_num > block1_num) {
								COAPC_DEBUG_INFO("Duplicated block#1\r\n");
								is_duplicated = pdTRUE;
							}

							request_ptr->block1_num = block1_num + 1;
							request_ptr->block1_szx = block1_szx;
							//more flag will be updated during make request.
							break;
						case COAP_OPTION_BLOCK_2:
							coap_get_block_info(opt_ptr, &block2_num,
												&block2_more, &block2_szx);

							COAPC_DEBUG_INFO("Block#2(%d:%d:%d)\r\n",
											 block2_num, block2_more, block2_szx);

							if ((request_ptr->block2_more)
								&& (request_ptr->block2_num > block2_num)) {
								COAPC_DEBUG_INFO("Duplicated block#2\r\n");
								is_duplicated = pdTRUE;
							}

							request_ptr->block2_num = block2_num + 1;
							request_ptr->block2_more = block2_more;
							request_ptr->block2_szx = block2_szx;
							break;
						case COAP_OPTION_MAX_AGE:
							for (idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
								max_age = (max_age << 8) | opt_ptr->buf.p[idx];
							}

							COAPC_DEBUG_INFO("Max-age(%d)\r\n", max_age);
							break;
						default:
							break;
					}
				}

				if (is_duplicated) {
					continue;
				}

				if (parsed_packet.payload.len > 0) {
					coap_set_rw_buffer(&payload,
									   (const char *)parsed_packet.payload.p,
									   parsed_packet.payload.len);
				}

				if (request_ptr->block1_more || block2_more) {
					if (is_obs_option) {
						request_ptr->method = COAP_METHOD_GET;
						request_ptr->observe_reg = 0;
					} else {
						coap_client_inc_token(request_ptr->token_ptr);
					}

					coap_client_increase_msgId(request_ptr->msgId_ptr);

					memset(pck_buf, 0x00, sizeof(pck_buf));
					pck_len = 0;

					//create the req packet
					ret = coap_client_make_request(request_ptr, pck_buf, &pck_len);
					if (ret || pck_len == 0) {
						COAPC_DEBUG_ERR("failed to generate request(ret:%d,len:%ld)\r\n",
										ret, pck_len);
						goto end_of_func;
					}

					COAPC_DEBUG_INFO("Send coap request for block-wise\r\n");

					//send request
					ret = coap_client_send(conn_ptr, pck_buf, pck_len);
					if (ret) {
						COAPC_DEBUG_ERR("failed to send request(0x%x)\r\n", -ret);
						goto end_of_func;
					}

					COAPC_DEBUG_INFO("Request next packet for block-wise\r\n");

					if (request_ptr->proxy_uri.len) {
						recv_proxy_request = 0;
					}

					continue;
				}

				//Check proxy request
				if (request_ptr->proxy_uri.len) {
					if (recv_proxy_request && !got_ack) {
						if (request_ptr->block1_more && block2_more) {
							COAPC_DEBUG_INFO("Not completed block-wise of proxy\r\n");
							continue;
						} else {
							COAPC_DEBUG_INFO("Completed block-wise of proxy\r\n");
							break;
						}
					} else {
						COAPC_DEBUG_INFO("Not received proxy request\r\n");
						continue;
					}
				}

				//received ack
				if (got_ack) {
					COAPC_DEBUG_INFO("Received Ack and done\r\n");
					break;
				}

				if (is_obs_option) {
					COAPC_DEBUG_INFO("Received Observe option\r\n");
					break;
				}
			}
		} else if (ret == DA_APP_NO_PACKET) {
			if ((request_ptr->type == COAP_MSG_TYPE_NONCON)
				&& (request_ptr->method != COAP_METHOD_GET)) {
				COAPC_DEBUG_TEMP("Requested non-confirmable msg\r\n");
				break;
			}

			if (retransmit) {
				//send request - retry
				if (retry_cnt < config_ptr->max_retransmit) {
					memset(pck_buf, 0x00, sizeof(pck_buf));
					pck_len = 0;

					ret = coap_client_make_request(request_ptr, pck_buf, &pck_len);
					if (ret || pck_len == 0) {
						COAPC_DEBUG_ERR("failed to generate request(ret:%d,len:%ld)\r\n",
										ret, pck_len);
						goto end_of_func;
					}

					COAPC_DEBUG_INFO("Retransmit CoAP request\r\n");

					ret = coap_client_send(conn_ptr, pck_buf, pck_len);
					if (ret) {
						COAPC_DEBUG_ERR("failed to send request(0x%x)\r\n", -ret);
						goto end_of_func;
					}

					if (request_ptr->proxy_uri.len) {
						recv_proxy_request = 0;
					}

					retry_cnt++;

					/* FIXME: Need to check rfc. 7252 - 4.8.2
					 * Time Values Derived from Transmission Parameters
					 */
					wait_option = (config_ptr->ack_timeout * (unsigned long)pow_long(2, (int)retry_cnt))
								  * COAP_CLIENT_UNIT_OF_TIME;

					COAPC_DEBUG_ERR("no ack. retry#%d - next wait time(%ld msec)\r\n",
									retry_cnt, wait_option);
				} else {
					COAPC_DEBUG_ERR("failed to get coap ack\r\n");
					ret = DA_APP_NO_PACKET;
					goto end_of_func;
				}
			} else {
				break;
			}
		} else {
			COAPC_DEBUG_ERR("failed to recevie packet(0x%x)\r\n", -ret);
			break;
		}
	}

end_of_func:

	//Copy response
	if (saved_resp) {
		coap_copy_rw_packet(resp_ptr, &parsed_packet);
	}

	if (resp_ptr->payload.len) {
		coap_free_rw_buffer(&resp_ptr->payload);
	}

	memcpy(&resp_ptr->payload, &payload, sizeof(coap_rw_buffer_t));

	if (recv_buf) {
		coap_free(recv_buf);
	}

	return ret;
}

int coap_client_send_ack(coap_client_conn_t *conn,
						 const unsigned char *token,
						 const unsigned char *msgId,
						 coap_rw_buffer_t *payload)
{
	coap_packet_t ack_pkt;
	unsigned char *ack_buf = NULL;
	size_t ack_buf_len = 0, ack_len = 0;
	int ret = 0;

	unsigned char content_type[2];

	/*section for ACK to separate response*/
	ack_pkt.header.version = 0x01;
	ack_pkt.header.type = COAP_MSG_TYPE_ACK;
	memcpy(ack_pkt.header.msg_id, msgId, COAP_CLIENT_MSGID_LENGTH);
	ack_pkt.copy_mode = 0;

	if (token == NULL) {
		ack_pkt.header.token_len = 0;
		ack_pkt.token.len = 0;
	} else {
		ack_pkt.header.token_len = COAP_CLIENT_TOKEN_LENGTH;
		ack_pkt.token.p = token;
		ack_pkt.token.len = COAP_CLIENT_TOKEN_LENGTH;
	}

	//set payload
	if (payload == NULL || payload->len == 0) {
		ack_pkt.header.code = 0;
		ack_pkt.numopts = 0;
		ack_pkt.payload.len = 0;
	} else {
		ack_pkt.header.code = COAP_RESP_CODE_CONTENT;
		//ack_pkt.header.code = COAP_RESP_CODE_VALID;
		//ack_pkt.header.code = COAP_RESP_CODE_CHANGED;

		content_type[0] = (COAP_CONTENT_TYPE_TEXT_PLAIN & 0xFF00) >> 8;
		content_type[1] = (COAP_CONTENT_TYPE_TEXT_PLAIN & 0x00FF);

		ack_pkt.numopts = 1;
		ack_pkt.opts[0].num = COAP_OPTION_CONTENT_FORMAT;
		ack_pkt.opts[0].buf.p = content_type;
		ack_pkt.opts[0].buf.len = 2;

		ack_pkt.payload.len = payload->len;
		ack_pkt.payload.p = payload->p;
	}

	ack_buf_len = ack_len = 50 + ack_pkt.payload.len;

	ack_buf = coap_calloc(ack_buf_len, sizeof(unsigned char));
	if (!ack_buf) {
		COAPC_DEBUG_ERR("failed to allocate memory for ack message\r\n");
		return DA_APP_MALLOC_ERROR;
	}

	ret = coap_generate_packet(ack_buf, &ack_len, &ack_pkt);
	if (ret == COAP_ERR_SUCCESS) {
		ret = coap_client_send(conn, ack_buf, ack_len);
		if (ret) {
			COAPC_DEBUG_ERR("failed to send ack message(0x%x)\r\n", -ret);
		}
	} else {
		COAPC_DEBUG_ERR("failed to generate ack message(0x%x)\r\n", -ret);
		ret = DA_APP_NOT_SUCCESSFUL;
	}

	if (ack_buf) {
		coap_free(ack_buf);
	}

	return ret;
}

int coap_client_send_rst(coap_client_conn_t *conn, const unsigned short msgId)
{
	coap_packet_t rst_pkt;
	unsigned char *rst_buf = NULL;
	size_t rst_buf_len = 0, rst_len = 0;
	int ret = 0;

	memset(&rst_pkt, 0x00, sizeof(coap_packet_t));

	rst_pkt.header.version = 0x01;
	rst_pkt.header.type = COAP_MSG_TYPE_RESET;
	rst_pkt.header.msg_id[0] = (uint8_t)(msgId >> 8);
	rst_pkt.header.msg_id[1] = (uint8_t)(msgId & 0xff);

	rst_buf_len = rst_len = 50;

	rst_buf = coap_calloc(rst_buf_len, sizeof(unsigned char));
	if (!rst_buf) {
		COAPC_DEBUG_ERR("failed to allocate memory for RST message\r\n");
		return DA_APP_MALLOC_ERROR;
	}

	ret = coap_generate_packet(rst_buf, &rst_len, &rst_pkt);
	if (ret == COAP_ERR_SUCCESS) {
		ret = coap_client_send(conn, rst_buf, rst_len);
		if (ret) {
			COAPC_DEBUG_ERR("failed to send RST message(0x%x)\r\n", -ret);
		}
	} else {
		COAPC_DEBUG_ERR("failed to construct RST message(%d)\r\n", ret);
		ret = DA_APP_NOT_SUCCESSFUL;
	}

	if (rst_buf) {
		coap_free(rst_buf);
	}

	return ret;
}

coap_client_opt_t *coap_client_make_option(uint8_t num, char *value,
		size_t value_len)
{
	coap_client_opt_t *option = NULL;

	option = coap_calloc(1, sizeof(coap_client_opt_t));
	if (!option) {
		COAPC_DEBUG_ERR("Failed to allocate memory for option\r\n");
		return NULL;
	}

	memset(option, 0x00, sizeof(coap_client_opt_t));

	option->num = num;

	if (value) {
		coap_set_rw_buffer(&option->value, (const char *)value, value_len);
	}

	option->next = NULL;

	return option;
}

void coap_client_release_option(coap_client_opt_t *option)
{
	if (option) {
		option->num = 0;
		coap_free_rw_buffer(&option->value);
		option->next = NULL;

		coap_free(option);
	}

	return ;
}

int coap_client_make_request(coap_client_req_t *request,
							 unsigned char *out, unsigned int *out_len)
{
	int ret = DA_APP_SUCCESS;

	coap_client_opt_t *opt_root = NULL;
	coap_client_opt_t *opt_last = NULL;
	coap_client_opt_t *opt_cur = NULL;

	uint32_t last_option_number = 0;
	uint32_t option_delta = 0;
	uint8_t option_delta_nibble = 0;

	uint32_t option_length = 0;
	uint8_t option_length_nibble = 0;

	int idx = 0;

	uint8_t bit_buf[4] = {0x00,};
	int bit_buf_idx = 0;

	uint8_t byte_buf[32] = {0x00,};
	unsigned int is_empty_token = pdTRUE;

	unsigned char *pos = out;

	const uint8_t *payload_ptr = request->payload.p;
	unsigned int payload_block_size = request->payload.len;

#if defined (coap_client_add_option)
#undef	coap_client_add_option
#else
#define	coap_client_add_option(root, last, new)	\
	if (root) {				\
		last->next = new;		\
		last = new;			\
	} else {				\
		root = last = new;		\
	}
#endif // (coap_client_add_option)

	if (request->token_ptr != NULL) {
		for (idx = 0 ; idx < COAP_CLIENT_TOKEN_LENGTH ; idx++) {
			if (request->token_ptr[idx]) {
				is_empty_token = pdFALSE;
				break;
			}
		}
	}

	//header
	coap_set_in_bit_array(byte_buf, 0, 2, 1);

	if (request->type == COAP_MSG_TYPE_CON) {
		coap_set_in_bit_array(byte_buf, 2, 2, 0);
	} else {
		coap_set_in_bit_array(byte_buf, 2, 2, 1);
	}

	if (is_empty_token) {
		coap_set_in_bit_array(byte_buf, 4, 4, 0);
	} else {
		coap_set_in_bit_array(byte_buf, 4, 4, COAP_CLIENT_TOKEN_LENGTH);
	}

	*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);
	*pos++ = request->method;
	*pos++ = (unsigned char)(*request->msgId_ptr >> 8);
	*pos++ = (unsigned char)(*request->msgId_ptr & 0xff);

	if (!is_empty_token) {
		memcpy(pos, request->token_ptr, COAP_CLIENT_TOKEN_LENGTH);
		pos += COAP_CLIENT_TOKEN_LENGTH;
	}

	//construct uri-host option
	if (!is_in_valid_ip_class((char *)request->host.p)) {
		opt_cur = coap_client_make_option(COAP_OPTION_URI_HOST,
										  (char *)request->host.p,
										  strlen((char *)request->host.p));

		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create uri-host option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//construct observe option
	if ((request->method == COAP_METHOD_GET) && (request->observe_reg)) {
		char value = 0x00;

		if (request->observe_reg == 2) {
			value = 0x01;
		}

		opt_cur = coap_client_make_option(COAP_OPTION_OBSERVE, &value, sizeof(value));
		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create observe option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//construct uri-port option
	if ((request->port != COAP_PORT)
		&& (request->port != COAPS_PORT)
		&& (request->port != 0)) {

		if (request->port <= 0xFF) {
			coap_set_in_bit_array(byte_buf, 0, 8, (int)request->port);
			bit_buf_idx += 8;
		} else if (request->port <= 0xFFFF) {
			coap_set_in_bit_array(byte_buf, 0, 16, (int)request->port);
			bit_buf_idx += 16;
		} else {
			COAPC_DEBUG_ERR("Invalid port number(%ld)\r\n", request->port);
			ret = DA_APP_INVALID_PARAMETERS;
			goto end_of_func;
		}

		for (idx = 0 ; idx < bit_buf_idx / 8 ; idx++) {
			bit_buf[idx] = (uint8_t)coap_get_byte_from_bits(byte_buf + (idx * 8));
		}

		opt_cur = coap_client_make_option(COAP_OPTION_URI_PORT,
										  (char *)bit_buf, (size_t)(bit_buf_idx / 8));
		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create uri-port option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//construct uri-path option
	if (request->path.len) {
		char *path_buf = NULL;
		char *path = NULL;

		path_buf = coap_calloc(1, request->path.len + 1);
		if (path_buf == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory to parse uri-path\r\n");
			ret = DA_APP_MALLOC_ERROR;
			goto end_of_func;
		}

		memset(path_buf, 0x00, request->path.len + 1);
		memcpy(path_buf, request->path.p, request->path.len);

		path = strtok(path_buf, "/");

		while (path != NULL) {
			opt_cur = coap_client_make_option(COAP_OPTION_URI_PATH,
											  path, strlen(path));
			if (!opt_cur) {
				COAPC_DEBUG_ERR("Failed to create uri-path option\r\n");
				ret = DA_APP_NOT_CREATED;
				goto end_of_func;
			}

			coap_client_add_option(opt_root, opt_last, opt_cur);

			path = strtok(NULL, "/");
		}

		if (path_buf) {
			coap_free(path_buf);
		}
	}

	//construct contents-format
	if (!is_empty_token) {
		opt_cur = coap_client_make_option(COAP_OPTION_CONTENT_FORMAT, NULL, 0);
		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create content format option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//construct uri-query option
	if (request->query.len) {
		char *query_buf = NULL;
		char *query = NULL;

		query_buf = coap_calloc(1, request->query.len + 1);
		if (query_buf == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory to parse uri-query\r\n");
			ret = DA_APP_MALLOC_ERROR;
			goto end_of_func;
		}

		memset(query_buf, 0x00, request->query.len + 1);
		memcpy(query_buf, request->query.p, request->query.len);

		query = strtok(query_buf, "&");

		while (query != NULL) {
			opt_cur = coap_client_make_option(COAP_OPTION_URI_QUERY,
											  query, strlen(query));
			if (!opt_cur) {
				COAPC_DEBUG_ERR("Failed to create uri-query option\r\n");
				ret = DA_APP_NOT_CREATED;
				goto end_of_func;
			}

			coap_client_add_option(opt_root, opt_last, opt_cur);

			query = strtok(NULL, "&");
		}

		if (query_buf) {
			coap_free(query_buf);
		}
	}

	//construct block#2 option
	//block2_size = (unsigned int)pow_long(2, request->block2_szx + 4);
	if (request->method == COAP_METHOD_GET || request->block2_more) {
		if (request->block2_num <= 0xF) {
			coap_set_in_bit_array(byte_buf, 0, 4, (int)request->block2_num);
			bit_buf_idx += 4;
		} else if (request->block2_num <= 0xFFF) {
			coap_set_in_bit_array(byte_buf, 0, 12, (int)request->block2_num);
			bit_buf_idx += 12;
		} else if (request->block2_num <= 0xFFFFF) {
			coap_set_in_bit_array(byte_buf, 0, 20, (int)request->block2_num);
			bit_buf_idx += 20;
		} else {
			COAPC_DEBUG_ERR("Invalid Block#2 Num(0x%x)\r\n", request->block2_num);
			ret = DA_APP_INVALID_PARAMETERS;
			goto end_of_func;
		}

		//More bit
		coap_set_in_bit_array(byte_buf, bit_buf_idx, 1, 0);
		bit_buf_idx += 1;

		//Block size
		coap_set_in_bit_array(byte_buf, bit_buf_idx, 3, request->block2_szx);
		bit_buf_idx += 3;

		//convert to bit from byte
		for (idx = 0 ; idx < bit_buf_idx / 8 ; idx++) {
			bit_buf[idx] = (uint8_t)coap_get_byte_from_bits(byte_buf + (idx * 8));
		}

		opt_cur = coap_client_make_option(COAP_OPTION_BLOCK_2,
										  (char *)bit_buf, (size_t)(bit_buf_idx / 8));

		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create block2 option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//construct block#1 option
	if (request->block1_more) {
		unsigned int block1_size = (unsigned int)pow_long(2, request->block1_szx + 4);

		if (payload_block_size > block1_size) {
			payload_block_size = block1_size;

			//Block number
			if (request->block1_num <= 0xF) {
				coap_set_in_bit_array(byte_buf, 0, 4, (int)request->block1_num);
				bit_buf_idx += 4;
			} else if (request->block1_num <= 0xFFF) {
				coap_set_in_bit_array(byte_buf, 0, 12, (int)request->block1_num);
				bit_buf_idx += 12;
			} else if (request->block1_num <= 0xFFFFF) {
				coap_set_in_bit_array(byte_buf, 0, 20, (int)request->block1_num);
				bit_buf_idx += 20;
			} else {
				COAPC_DEBUG_ERR("Invalid Block#1 Num(0x%x)\r\n", request->block1_num);
				ret = DA_APP_INVALID_PARAMETERS;
				goto end_of_func;
			}

			//More bit
			if (((request->block1_num + 1) * payload_block_size) >= request->payload.len) {
				coap_set_in_bit_array(byte_buf, bit_buf_idx, 1, 0);
				payload_block_size =
					request->payload.len - ((request->block1_num) * block1_size);
				request->block1_more = 0;
			} else {
				coap_set_in_bit_array(byte_buf, bit_buf_idx, 1, 1);
				payload_block_size = block1_size;
				request->block1_more = 1;
			}

			bit_buf_idx += 1;

			payload_ptr = request->payload.p + (request->block1_num * block1_size);

			//Block size
			coap_set_in_bit_array(byte_buf, bit_buf_idx, 3, request->block1_szx);
			bit_buf_idx += 3;

			//convert to bit from byte
			for (idx = 0 ; idx < bit_buf_idx / 8 ; idx++) {
				bit_buf[idx] = (uint8_t)coap_get_byte_from_bits(byte_buf + (idx * 8));
			}

			opt_cur = coap_client_make_option(COAP_OPTION_BLOCK_1,
											  (char *)bit_buf, (size_t)(bit_buf_idx / 8));

			if (!opt_cur) {
				COAPC_DEBUG_ERR("Failed to create block1 option\r\n");
				ret = DA_APP_NOT_CREATED;
				goto end_of_func;
			}

			coap_client_add_option(opt_root, opt_last, opt_cur);

			//construct size#1 option
			if (request->block1_num == 0) {
				if (request->payload.len <= 0xFF) {
					bit_buf_idx = 8;
				} else if (request->payload.len <= 0xFFFF) {
					bit_buf_idx = 16;
				} else if (request->payload.len <= 0xFFFFFF) {
					bit_buf_idx = 24;
				} else {
					bit_buf_idx = 32;
				}

				coap_set_in_bit_array(byte_buf, 0, bit_buf_idx, (int)request->payload.len);

				//convert to bit from byte
				for (idx = 0 ; idx < bit_buf_idx / 8 ; idx++) {
					bit_buf[idx] = (uint8_t)coap_get_byte_from_bits(byte_buf + (idx * 8));
				}

				opt_cur = coap_client_make_option(COAP_OPTION_SIZE_1,
												  (char *)bit_buf, (size_t)bit_buf_idx / 8);

				if (!opt_cur) {
					COAPC_DEBUG_ERR("Failed to create size1 option\r\n");
					ret = DA_APP_NOT_CREATED;
					goto end_of_func;
				}

				coap_client_add_option(opt_root, opt_last, opt_cur);
			}
		} else {
			request->block1_more = 0;
		}
	} else {
		payload_ptr = NULL;
		payload_block_size = 0;
	}

	//construct proxy-uri option
	if (request->proxy_uri.len) {
		opt_cur = coap_client_make_option(COAP_OPTION_PROXY_URI,
										  (char *)request->proxy_uri.p,
										  strlen((char *)request->proxy_uri.p));
		if (!opt_cur) {
			COAPC_DEBUG_ERR("Failed to create proxy-uri option\r\n");
			ret = DA_APP_NOT_CREATED;
			goto end_of_func;
		}

		coap_client_add_option(opt_root, opt_last, opt_cur);
	}

	//Adding options
	for (opt_cur = opt_root ; opt_cur != NULL ; opt_cur = opt_cur->next) {
		// write 4-bit option delta
		option_delta = opt_cur->num - last_option_number;
		coap_get_option_nibble(option_delta, &option_delta_nibble);
		coap_set_in_bit_array(byte_buf, 0, 4, option_delta_nibble);

		// write 4-bit option length
		option_length = opt_cur->value.len;
		coap_get_option_nibble(option_length, &option_length_nibble);
		coap_set_in_bit_array(byte_buf, 4, 4, option_length_nibble);

		*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);

		// write extended option delta field (0 - 2 bytes)
		if (option_delta_nibble == 13) {
			coap_set_in_bit_array(byte_buf, 0, 8, (int)(option_delta - 13));
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);
		} else if (option_delta_nibble == 14) {
			coap_set_in_bit_array(byte_buf, 0, 16, (int)(option_delta - 269));
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf + 8);
		}

		// write extended option length field (0 - 2 bytes)
		if (option_length_nibble == 13) {
			coap_set_in_bit_array(byte_buf, 0, 8, (int)(option_length - 13));
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);
		} else if (option_length_nibble == 14) {
			coap_set_in_bit_array(byte_buf, 0, 16, (int)(option_length - 269));
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf);
			*pos++ = (unsigned char)coap_get_byte_from_bits(byte_buf + 8);
		}

		// write option value
		if (option_length) {
			memcpy(pos, opt_cur->value.p, option_length);
			pos += option_length;
		}

		last_option_number = opt_cur->num;
	}

	//Add payload
	if (payload_ptr) {
		*pos++ = 0xFF;	//options marker
		memcpy(pos, payload_ptr, payload_block_size);
		pos += payload_block_size;
	}

end_of_func:

	opt_cur = opt_root;

	while (opt_cur) {
		opt_last = opt_cur->next;
		coap_client_release_option(opt_cur);
		opt_cur = opt_last;
	}

	opt_root = opt_last = opt_cur = NULL;

	if (ret == DA_APP_SUCCESS) {
		*out_len = (unsigned int)(pos - out);
	} else {
		*out_len = 0;
	}

#if defined (coap_client_add_option)
#undef	coap_client_add_option
#endif // (coap_client_add_option)

	return ret;
}

int coap_client_recover_observe(coap_client_t *client_ptr)
{
	int ret = DA_APP_SUCCESS;
	char socket_name[COAP_MAX_NAME_LEN] = {0x00,};

	unsigned long peer_ip_addr;
	unsigned int secure_conn;

	coap_client_conf_t *config_ptr = &client_ptr->config;
	coap_client_obs_t *observe_ptr = NULL;

	coap_client_dpm_obs_conf_t *obs_dpm_conf = NULL;

	if (!dpm_mode_is_enabled()) {
		COAPC_DEBUG_INFO("Not supported\r\n");
		return DA_APP_SUCCESS;
	}

	obs_dpm_conf = coap_client_dpm_get_obs_config(client_ptr->dpm_conf_name);
	if (!obs_dpm_conf) {
		COAPC_DEBUG_ERR("failed to load observe config for DPM\r\n");
		return DA_APP_NOT_SUCCESSFUL;
	}

	for (int idx = 0 ; idx < COAP_CLIENT_MAX_OBSERVE_LIST ; idx++, obs_dpm_conf++) {
		//init local variables
		memset(socket_name, 0x00, sizeof(socket_name));
		peer_ip_addr = 0;
		secure_conn = 0;

		observe_ptr = NULL;

		//check validation
		if (obs_dpm_conf->state == 0) {
			COAPC_DEBUG_INFO("%d. CoAP observe is not registered(state:%d)\r\n",
							 obs_dpm_conf->idx, obs_dpm_conf->state);
			continue;
		}

		observe_ptr = coap_calloc(1, sizeof(coap_client_obs_t));
		if (!observe_ptr) {
			COAPC_DEBUG_ERR("%d. failed to creaate observe\r\n", obs_dpm_conf->idx);
			continue;
		}

		ret = coap_client_parse_uri(obs_dpm_conf->req_info.uri,
									obs_dpm_conf->req_info.urilen,
									&observe_ptr->request.scheme,
									&observe_ptr->request.host,
									&observe_ptr->request.path,
									&observe_ptr->request.query,
									&peer_ip_addr,
									&observe_ptr->request.port,
									&secure_conn);
		if (ret != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("%d. failed to parse uri(0x%x)\r\n", obs_dpm_conf->idx, -ret);
			coap_client_clear_request(&observe_ptr->request);
			coap_free(observe_ptr);
			continue;
		}

		observe_ptr->id = (int)client_ptr->observe_count;

		COAP_CLIENT_OBS_TIMER_NAME(observe_ptr->id,
								   (char *)observe_ptr->close_timer_name);

		//recover connection
		COAP_CLIENT_OBS_SOCK_NAME((char *)client_ptr->name_ptr,
								  (char *)socket_name, observe_ptr->id);

		ret = coap_client_init_connection(&observe_ptr->conn,
										  (char *)socket_name,
										  obs_dpm_conf->local_port,
										  peer_ip_addr,
										  observe_ptr->request.port,
										  secure_conn);
		if (ret != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("%d. failed to recover connection(0x%x)\r\n",
							obs_dpm_conf->idx, -ret);
			coap_client_clear_request(&observe_ptr->request);
			coap_free(observe_ptr);
			continue;
		}

		//recover dtls session
		if (secure_conn) {
			ret = coaps_client_init(&observe_ptr->conn, config_ptr);
			if (ret != DA_APP_SUCCESS) {
				COAPC_DEBUG_ERR("%d. failed to recover dtls session(0x%x)\r\n",
								obs_dpm_conf->idx, -ret);
				coap_client_deinit_connection(&observe_ptr->conn);
				coap_client_clear_request(&observe_ptr->request);
				coap_free(observe_ptr);
				continue;
			}
		}

		//recover observe config
		observe_ptr->status = REGISTERED;
		observe_ptr->client_ptr = (void *)client_ptr;
		observe_ptr->notify =
			(int (*)(void *, coap_rw_packet_t *))obs_dpm_conf->req_info.notify_callback;
		observe_ptr->close_notify =
			(void (*)(char *))obs_dpm_conf->req_info.close_callback;
		observe_ptr->next = NULL;

		memcpy(observe_ptr->token, obs_dpm_conf->req_info.token,
			   COAP_CLIENT_TOKEN_LENGTH);
		observe_ptr->msgId = obs_dpm_conf->req_info.msgId;

		COAP_CLIENT_OBS_THREAD_NAME(client_ptr->name_ptr,
									observe_ptr->thread_name,
									observe_ptr->id);

		//register dpm
		dpm_app_register(observe_ptr->thread_name, obs_dpm_conf->local_port);

		//Set udp filter
		dpm_udp_port_filter_set((unsigned short)(obs_dpm_conf->local_port));

		COAPC_DEBUG_INFO("id(%d), thread name(%s), local port(%d)\r\n",
						 observe_ptr->id, observe_ptr->thread_name,
						 obs_dpm_conf->local_port);

		//set request
		observe_ptr->request.method = COAP_METHOD_GET;
		observe_ptr->request.type = config_ptr->msg_type;

		observe_ptr->request.observe_reg = 1;

		observe_ptr->request.payload.p = NULL;
		observe_ptr->request.payload.len = 0;

		observe_ptr->request.block1_num = 0;
		observe_ptr->request.block1_more = 0;
		observe_ptr->request.block1_szx = (uint8_t)config_ptr->block1_szx;

		observe_ptr->request.block2_num = 0;
		observe_ptr->request.block2_more = 0;
		observe_ptr->request.block2_szx = (uint8_t)config_ptr->block2_szx;

		if (obs_dpm_conf->req_info.proxy_urilen) {
			coap_set_rw_buffer(&observe_ptr->request.proxy_uri,
							   (const char *)obs_dpm_conf->req_info.proxy_uri,
							   obs_dpm_conf->req_info.proxy_urilen);
		}

		observe_ptr->request.token_ptr = observe_ptr->token;
		observe_ptr->request.msgId_ptr = &observe_ptr->msgId;

		ret = xTaskCreate(coap_client_observe_processor,
						  observe_ptr->thread_name,
						  COAP_CLIENT_OBS_STACK_SIZE,
						  (void *)observe_ptr,
						  OS_TASK_PRIORITY_USER,
						  &observe_ptr->task_handler);
		if (ret != pdPASS) {
			COAPC_DEBUG_ERR("%d. failed to create observe thread(%d)\r\n",
							obs_dpm_conf->idx, ret);

			ret = DA_APP_NOT_CREATED;

			if (observe_ptr->conn.secure_conn) {
				coaps_client_deinit(&observe_ptr->conn);
			}

			coap_client_deinit_connection(&observe_ptr->conn);
			coap_client_clear_request(&observe_ptr->request);
			coap_free(observe_ptr);
			continue;
		}

		//update observe list
		if (client_ptr->observe_list) {
			coap_client_obs_t *last = NULL;

			//find tail of linked-list
			for (last = client_ptr->observe_list ; last->next != NULL ; last = last->next);

			last->next = observe_ptr;
		} else {
			client_ptr->observe_list = observe_ptr;
		}

		client_ptr->observe_count++;

		//waiting for recv ready flag.
		while (!chk_dpm_rcv_ready((char *)observe_ptr->thread_name)) {
			vTaskDelay(10);
		}

		dpm_app_wakeup_done((char *)observe_ptr->thread_name);

		COAPC_DEBUG_INFO("#%d. Recovered CoAP Observe\r\n", obs_dpm_conf->idx);
	}

	client_ptr->state = SUSPEND;

	return ret;
}

int coap_client_release_observe(coap_client_obs_t *obs_ptr)
{
	const int wait_option = 10;

	if (obs_ptr->task_handler) {
		obs_ptr->status = REQ_TERMINATE;

		do {
			vTaskDelay(wait_option);
		} while (obs_ptr->status != TERMINATED);

		vTaskDelete(obs_ptr->task_handler);
		obs_ptr->task_handler = NULL;
	}

	return DA_APP_SUCCESS;
}

void coap_client_observe_processor(void *params)
{
    int sys_wdog_id = -1;
	int ret = DA_APP_SUCCESS;

	coap_client_obs_t *observe_ptr = (coap_client_obs_t *)params;

	coap_client_t *client_ptr = (coap_client_t *)observe_ptr->client_ptr;
	coap_client_conf_t *config_ptr = &client_ptr->config;
	coap_client_req_t *request_ptr = &observe_ptr->request;
	coap_client_conn_t *conn_ptr = &observe_ptr->conn;

	unsigned int loop_endlessly = pdTRUE;

	coap_rw_packet_t response;

	int opt_idx = 0;
	int max_age = 0;

	unsigned short prev_msgId = 0;
	unsigned short recv_msgId = 0;

	memset(&response, 0x00, sizeof(coap_rw_packet_t));

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	COAPC_DEBUG_INFO("Start of %s\r\n", __func__);

#if defined (ENABLE_COAPC_DEBUG_INFO)
	coap_client_print_conn(conn_ptr);
	coap_client_print_req(request_ptr);
#endif	// (ENABLE_COAPC_DEBUG_INFO )

	if (dpm_mode_is_enabled()) {
		if (chk_dpm_reg_state((char *)observe_ptr->thread_name) == DPM_NOT_REGISTERED) {
			COAPC_DEBUG_INFO("to register observe dpm(%s,%d)\r\n",
							 observe_ptr->thread_name, conn_ptr->local_port);

			dpm_app_register((char *)observe_ptr->thread_name, conn_ptr->local_port);
		}

		dpm_app_wakeup_done((char *)observe_ptr->thread_name);

		dpm_app_data_rcv_ready_set((char *)observe_ptr->thread_name);
	}

	while (loop_endlessly) {
		da16x_sys_watchdog_notify(sys_wdog_id);

		coap_clear_rw_packet(&response);

		max_age = 0;

		if (observe_ptr->status == REQ_TERMINATE) {
			COAPC_DEBUG_INFO("Required to terminate observe. Send RST msg.\r\n");
			coap_client_send_rst(conn_ptr, *request_ptr->msgId_ptr);
			break;
		}

		da16x_sys_watchdog_suspend(sys_wdog_id);

		ret = coap_client_recv_resp(config_ptr, conn_ptr, request_ptr,
									&response, pdFALSE);

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

		if (dpm_mode_is_enabled()) {
			dpm_app_sleep_ready_clear((char *)observe_ptr->thread_name);
		}

		if (ret == DA_APP_SUCCESS) {
			if (dpm_mode_is_enabled()) {
				coap_client_dpm_set_obs_msgId(client_ptr->dpm_conf_name,
											  observe_ptr->dpm_idx,
											  observe_ptr->msgId);
			}

			//check duplicated message ID
			recv_msgId = response.header.msg_id[0] << 8;
			recv_msgId += response.header.msg_id[1];

			if (prev_msgId != recv_msgId) {
				prev_msgId = recv_msgId;

				//parse options
				for (opt_idx = 0 ; opt_idx < response.numopts ; opt_idx++) {
					coap_rw_option_t *opt_ptr = &(response.opts[opt_idx]);

					switch (opt_ptr->num) {
						case COAP_OPTION_MAX_AGE:
							for (unsigned int idx = 0 ; idx < opt_ptr->buf.len ; idx++) {
								max_age = (max_age << 8) | opt_ptr->buf.p[idx];
							}

							COAPC_DEBUG_INFO("Max-age(%d)\r\n", max_age);

							observe_ptr->max_age = (unsigned int)max_age;

							ret = coap_client_register_observe_timer(observe_ptr);
							if (ret != DA_APP_SUCCESS) {
								COAPC_DEBUG_ERR("failed to register observe "
												"close timer(0x%x)\r\n", -ret);
							}
							break;
						default:
							break;
					}
				}

				if (response.header.type == COAP_MSG_TYPE_RESET) {
					break;
				}

				//call receive notify callback
				if (observe_ptr->notify) {
					observe_ptr->notify(client_ptr, &response);
				} else {
					COAPC_DEBUG_INFO("Not register observe notification callback\r\n");
				}
			} else {
				COAPC_DEBUG_INFO("Duplicated mID(%d)\r\n", recv_msgId);
			}
		} else if (ret == DA_APP_NO_PACKET) {
			;
		} else {
			COAPC_DEBUG_ERR("failed to receive response(0x%x)\r\n", -ret);
			break;
		}

		if (conn_ptr->secure_conn) {
			ret = coaps_client_save_ssl(conn_ptr);
			if (ret != DA_APP_SUCCESS) {
				COAPC_DEBUG_ERR("failed to save dtls session(0x%x)\r\n", -ret);
			}
		}

		if (dpm_mode_is_enabled()) {
			dpm_app_sleep_ready_set(observe_ptr->thread_name);
		}
	}

	coap_clear_rw_packet(&response);

	ret = coap_client_unregister_observe_timer(observe_ptr);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to unregister observe close timer(0x%x)\r\n", -ret);
	}

	if (dpm_mode_is_enabled()) {
		coap_client_dpm_clear_obs_config(client_ptr->dpm_conf_name, observe_ptr->dpm_idx);
		dpm_app_unregister(observe_ptr->thread_name);
	}

	if (conn_ptr->secure_conn) {
		coaps_client_deinit(conn_ptr);
	}

	coap_client_deinit_connection(conn_ptr);
	coap_client_clear_request(request_ptr);

	if (observe_ptr->close_notify) {
		observe_ptr->close_notify((char *)observe_ptr->close_timer_name);
	}

	observe_ptr->status = TERMINATED;

	da16x_sys_watchdog_unregister(sys_wdog_id);

	COAPC_DEBUG_INFO("End of %s\r\n", __func__);

    observe_ptr->task_handler = NULL;
    vTaskDelete(NULL);

	return ;
}

//dpm function
coap_client_dpm_conf_t *coap_client_dpm_get_config(char *name)
{
	int ret = DA_APP_SUCCESS;
	const unsigned long wait_option = 100;

	coap_client_dpm_conf_t *data = NULL;
	unsigned int len = 0;

	int obs_idx = 0;

	if (!dpm_mode_is_enabled()) {
		COAPC_DEBUG_INFO("Not supported\r\n");
		return NULL;
	}

	len = dpm_user_mem_get((char *)name, (unsigned char **)&data);

	if (len == 0) {
		ret = (int)dpm_user_mem_alloc((char *)name,
								 (void **)&data,
								 sizeof(coap_client_dpm_conf_t),
								 wait_option);
		if (ret) {
			COAPC_DEBUG_ERR("failed to allocate rtm memory for dpm config\r\n");
			return NULL;
		}

		memset(data, 0x00, sizeof(coap_client_dpm_conf_t));

		//set observe idx
		for (obs_idx = 0 ; obs_idx < COAP_CLIENT_MAX_OBSERVE_LIST ; obs_idx++) {
			data->obs_list[obs_idx].idx = (unsigned int)obs_idx;
		}
	} else if (len != sizeof(coap_client_dpm_conf_t)) {
		COAPC_DEBUG_ERR("invalid size(%ld)\r\n", len);
		return NULL;
	}

	return data;
}

int coap_client_dpm_clear_config(char *name)
{
	int ret = DA_APP_SUCCESS;

	if (!dpm_mode_is_enabled()) {
		COAPC_DEBUG_INFO("Not supported\r\n");
		return DA_APP_SUCCESS;
	}

	ret = (int)dpm_user_mem_free((char *)name);
	if (ret) {
		COAPC_DEBUG_ERR("failed to release rtm of coap client's config(0x%x)\r\n", ret);
		return DA_APP_NOT_SUCCESSFUL;
	}

	return DA_APP_SUCCESS;
}

int coap_client_dpm_set_msgId(char *name, unsigned short msgId)
{
	coap_client_dpm_conf_t *data = NULL;

	data = coap_client_dpm_get_config(name);
	if (data) {
		data->msgId = msgId;

		COAPC_DEBUG_TEMP("to set msgId(%d)\r\n", data->msgId);

		return DA_APP_SUCCESS;
	}

	return DA_APP_NOT_SUCCESSFUL;
}

int coap_client_dpm_get_msgId(char *name, unsigned short *msgId)
{
	coap_client_dpm_conf_t *data = NULL;

	data = coap_client_dpm_get_config(name);
	if (data) {
		*msgId = data->msgId;

		COAPC_DEBUG_TEMP("to get msgId(%d)\r\n", *msgId);

		return DA_APP_SUCCESS;
	}

	return DA_APP_NOT_SUCCESSFUL;
}

int coap_client_dpm_set_token(char *name, unsigned char *token)
{
	coap_client_dpm_conf_t *data = NULL;

	data = coap_client_dpm_get_config(name);
	if (data) {
		memcpy(data->token, token, COAP_CLIENT_TOKEN_LENGTH);

		COAPC_DEBUG_TEMP("to set token"
						 "(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\r\n",
						 token[0], token[1], token[2], token[3],
						 token[4], token[5], token[6], token[7]);

		return DA_APP_SUCCESS;
	}

	return DA_APP_NOT_SUCCESSFUL;
}

int coap_client_dpm_get_token(char *name, unsigned char *token)
{
	int idx = 0;
	coap_client_dpm_conf_t *data = NULL;

	data = coap_client_dpm_get_config(name);
	if (data) {
		memcpy(token, data->token, COAP_CLIENT_TOKEN_LENGTH);

		for (idx = 0 ; idx < COAP_CLIENT_TOKEN_LENGTH ; idx++) {
			if (token[idx] != 0x00) {
				break;
			}
		}

		if (idx >= COAP_CLIENT_TOKEN_LENGTH) {
			COAPC_DEBUG_INFO("token is empty\r\n");
			return DA_APP_INVALID_PARAMETERS;
		}

		COAPC_DEBUG_TEMP("to get token"
						 "(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\r\n",
						 token[0], token[1], token[2], token[3],
						 token[4], token[5], token[6], token[7]);

		return DA_APP_SUCCESS;
	}

	return DA_APP_NOT_SUCCESSFUL;
}

coap_client_dpm_obs_conf_t *coap_client_dpm_get_obs_config(char *name)
{
	coap_client_dpm_conf_t *dpm_conf = NULL;

	dpm_conf = coap_client_dpm_get_config(name);
	if (!dpm_conf) {
		return NULL;
	}

	return &(dpm_conf->obs_list[0]);
}

coap_client_dpm_obs_conf_t *coap_client_dpm_get_empty_obs_config(char *name)
{
	int idx = 0;
	coap_client_dpm_obs_conf_t *dpm_obs_conf = NULL;

	dpm_obs_conf = coap_client_dpm_get_obs_config(name);
	if (!dpm_obs_conf) {
		return NULL;
	}

	for (idx = 0 ; idx < COAP_CLIENT_MAX_OBSERVE_LIST ; idx++) {
		if (dpm_obs_conf->state == 0) {
			return dpm_obs_conf;
		}

		dpm_obs_conf++;
	}

	return NULL;
}

int coap_client_dpm_clear_obs_config(char *name, int idx)
{
	coap_client_dpm_obs_conf_t *dpm_obs_conf = NULL;

	if (idx < 0) {
		COAPC_DEBUG_ERR("invalid obs config index for dpm(%s,%d)\r\n",
						name, idx);
		return DA_APP_INVALID_PARAMETERS;
	}

	dpm_obs_conf = coap_client_dpm_get_obs_config(name);
	if (!dpm_obs_conf) {
		COAPC_DEBUG_ERR("Not found obs config index for dpm(%s,%d)\r\n",
						name, idx);
		return DA_APP_NOT_FOUND;
	}

	memset(&(dpm_obs_conf[idx]), 0x00, sizeof(coap_client_dpm_conf_t));

	return DA_APP_SUCCESS;
}

int coap_client_dpm_set_obs_msgId(char *name, int idx, unsigned short msgId)
{
	coap_client_dpm_obs_conf_t *dpm_obs_conf = NULL;

	if (idx < 0) {
		COAPC_DEBUG_ERR("invalid obs config index for dpm(%s,%d)\r\n",
						name, idx);
		return DA_APP_INVALID_PARAMETERS;
	}

	dpm_obs_conf = coap_client_dpm_get_obs_config(name);
	if (!dpm_obs_conf) {
		COAPC_DEBUG_ERR("Not found obs config index for dpm(%s,%d)\r\n",
						name, idx);
		return DA_APP_NOT_FOUND;
	}

	dpm_obs_conf[idx].req_info.msgId = msgId;

	COAPC_DEBUG_TEMP("to set obs message Id(%ld)\r\n", msgId);

	return DA_APP_SUCCESS;
}

static void coap_client_observe_close_timer_callback(TimerHandle_t xTimer)
{
	coap_client_obs_t *observe_ptr = NULL;

	observe_ptr = (coap_client_obs_t *)pvTimerGetTimerID(xTimer);

	if (dpm_mode_is_enabled()
		|| (observe_ptr == NULL)
		|| (observe_ptr->close_notify == NULL)) {
		return ;
	}

	observe_ptr->close_notify((char *)observe_ptr->close_timer_name);

	return ;
}

int coap_client_register_observe_timer(coap_client_obs_t *observe_ptr)
{
	int ret = DA_APP_SUCCESS;
	const int add_interval = COAP_CLIENT_OBSERVE_ADD_INTERVAL;

	if (observe_ptr->max_age <= 0) {
		COAPC_DEBUG_ERR("Not required to register timer(obs max-age%d)\r\n",
						observe_ptr->max_age);
		return DA_APP_SUCCESS;
	}

	if (dpm_mode_is_enabled()) {
		ret = coap_client_unregister_observe_timer(observe_ptr);
		if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_NOT_FOUND)) {
			COAPC_DEBUG_ERR("failed to unregister observe timer(0x%x)\r\n", -ret);
		}

		ret = dpm_timer_create(observe_ptr->thread_name,
									observe_ptr->close_timer_name,
									observe_ptr->close_notify,
									((observe_ptr->max_age + (unsigned int)add_interval) * 1000),
									0); //msec
		if (ret < 0) {
			COAPC_DEBUG_ERR("failed to create dpm timer(%d)\r\n", ret);
			return DA_APP_NOT_CREATED;
		}

		COAPC_DEBUG_INFO("Registered observe close timer"
						 "tid:%d, id:%d, dpm_idx:%d, thread_name(%s), "
						 "close_timer_name(%s), interval(%d+%d)\r\n",
						 ret, observe_ptr->id, observe_ptr->dpm_idx,
						 observe_ptr->thread_name, observe_ptr->close_timer_name,
						 observe_ptr->max_age, add_interval);
	} else {
		coap_client_unregister_observe_timer(observe_ptr);

		observe_ptr->close_timer = xTimerCreate(observe_ptr->close_timer_name,
												pdMS_TO_TICKS((observe_ptr->max_age + (unsigned int)add_interval) * 1000),
												pdTRUE,
												observe_ptr,
												coap_client_observe_close_timer_callback);
		if (observe_ptr->close_timer == NULL) {
			COAPC_DEBUG_ERR("failed to create close timer of observe\r\n");
			return DA_APP_NOT_CREATED;
		}

		COAPC_DEBUG_INFO("Registered observe timer"
						 "(id:%d, dpm_idx:%d, max_age:%d, additional interval:%d, ret:%d)\r\n",
						 observe_ptr->id, observe_ptr->dpm_idx,
						 observe_ptr->max_age, add_interval, ret);
	}

	return DA_APP_SUCCESS;
}

int coap_client_unregister_observe_timer(coap_client_obs_t *observe_ptr)
{
	int ret = 0;

	if (dpm_mode_is_enabled()) {
		ret = dpm_user_timer_delete(observe_ptr->thread_name,
									observe_ptr->close_timer_name);
		if (ret == -2) {
			COAPC_DEBUG_INFO("Not found observe close timer(%s/%s)\r\n",
							 observe_ptr->thread_name,
							 observe_ptr->close_timer_name);
			return DA_APP_NOT_FOUND;
		} else if (ret < 0) {
			COAPC_DEBUG_ERR("failed to delete observe timer(%d/%s/%s)\r\n", ret,
							observe_ptr->thread_name,
							observe_ptr->close_timer_name);
			return DA_APP_NOT_SUCCESSFUL;
		}

		ret = DA_APP_SUCCESS;
	} else if (observe_ptr->close_timer) {
		ret = xTimerDelete(observe_ptr->close_timer, 0);
		if (ret != pdPASS) {
			COAPC_DEBUG_ERR("failed to delete close timer of observe(%d)\r\n", ret);
			return DA_APP_NOT_SUCCESSFUL;
		}
	}

	observe_ptr->close_timer = NULL;

	return DA_APP_SUCCESS;
}

int coap_client_get_ip_info(char *uri, size_t uri_len, unsigned long *ip_addr,
							unsigned int *port)
{
	int ret = 0;

	if (!ip_addr || !port) {
		COAPC_DEBUG_ERR("invalid parameters\r\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	ret = coap_client_parse_uri((unsigned char *)uri, uri_len,
								NULL,
								NULL,
								NULL,
								NULL,
								ip_addr,
								port,
								NULL);
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to get ip address(0x%x)\r\n", -ret);
	}

	return ret;
}

int coap_client_update_ip_info(char *uri, size_t uri_len,
							   unsigned long *ip_addr, unsigned int *port)
{
	int ret = 0;
	coap_rw_buffer_t host = {0x00,};

	if (!ip_addr || !port) {
		COAPC_DEBUG_ERR("invalid parameters\r\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (g_coap_client_ip_cache) {
		for (unsigned int idx = 0 ; idx < g_coap_client_ip_cache->idx ; idx++) {
			if (memcmp(host.p, g_coap_client_ip_cache->items[idx].host_name,
					   host.len) == 0) {
				COAPC_DEBUG_TEMP("increase current pos for cached ip address\r\n");
				g_coap_client_ip_cache->items[idx].cur_pos += 1;
				break;
			}
		}
	}

	ret = coap_client_parse_uri((unsigned char *)uri, uri_len,
								NULL,		//scheme
								&host,		//host
								NULL,		//path
								NULL,		//query
								ip_addr,	//ip_addr
								port,		//port
								NULL);		//secure flag
	if (ret != DA_APP_SUCCESS) {
		COAPC_DEBUG_ERR("failed to parse uri(0x%x)\r\n", -ret);
		goto end;
	}

end:

	coap_free_rw_buffer(&host);

	return ret;
}

int coap_client_load_ip_cache()
{
	int ret = 0;
	const unsigned long wait_option = 100;

	unsigned int len = 0;
	coap_client_ip_cache_t *data = NULL;

	if (!g_coap_client_ip_cache) {
		if (dpm_mode_is_enabled()) {
			len = dpm_user_mem_get(COAP_CLIENT_IP_CACHE_NAME,
								   (unsigned char **)&data);
			if (len == 0) {
				ret = (int)dpm_user_mem_alloc(COAP_CLIENT_IP_CACHE_NAME,
										 (void **)&data,
										 sizeof(coap_client_ip_cache_t),
										 wait_option);
				if (ret) {
					COAPC_DEBUG_ERR("failed to allocate rtm memory "
									"for ip cache(0x%02x)\r\n", ret);
					return DA_APP_MALLOC_ERROR;
				}

				memset(data, 0x00, sizeof(coap_client_ip_cache_t));
			} else if (len != sizeof(coap_client_ip_cache_t)) {
				COAPC_DEBUG_ERR("invalid size(%ld)\r\n", len);
				return DA_APP_NOT_SUCCESSFUL;
			}

			g_coap_client_ip_cache = data;
		} else {
			g_coap_client_ip_cache = coap_calloc(1, sizeof(coap_client_ip_cache_t));
			if (!g_coap_client_ip_cache) {
				COAPC_DEBUG_ERR("failed to allocate memory for ip cache\r\n");
				return DA_APP_MALLOC_ERROR;
			}
		}
	}

	return DA_APP_SUCCESS;
}

int coap_client_release_ip_cache()
{
	int ret = 0;

	if (dpm_mode_is_enabled()) {
		ret = (int)dpm_user_mem_free(COAP_CLIENT_IP_CACHE_NAME);
		if (ret) {
			COAPC_DEBUG_ERR("failed to release rtm memory(%s:0x%02x)\r\n",
							COAP_CLIENT_IP_CACHE_NAME, ret);
			return DA_APP_NOT_SUCCESSFUL;
		}
	} else {
		coap_free(g_coap_client_ip_cache);
	}

	g_coap_client_ip_cache = NULL;

	return DA_APP_SUCCESS;
}

int coap_client_get_ip_addr(char *host, size_t host_len, unsigned long *ip_addr)
{
	int ret = DA_APP_SUCCESS;
	unsigned long dns_query_wait_option = 500;

	unsigned long ip_buf[COAP_CLIENT_MAX_IP_CACHE] = {0x00,};
	unsigned int ip_cnt = 0;

	unsigned int idx = 0;


	*ip_addr = 0;

	if (!host || !host_len) {
		COAPC_DEBUG_ERR("invalid parameters\r\n");
		return DA_APP_INVALID_PARAMETERS;
	}

	if (is_in_valid_ip_class((char *)host)) {
		*ip_addr = (unsigned long)iptolong(host);
	} else {
		if (g_coap_client_ip_cache) {
			for (idx = 0 ; idx < g_coap_client_ip_cache->idx ; idx++) {
				if (memcmp(host, g_coap_client_ip_cache->items[idx].host_name, host_len) == 0) {
					if (g_coap_client_ip_cache->items[idx].cur_pos >=
						g_coap_client_ip_cache->items[idx].cnt) {
						COAPC_DEBUG_TEMP("Need to update ip address\r\n");
						break;
					} else {
						*ip_addr =
							g_coap_client_ip_cache->items[idx].ip_addr[g_coap_client_ip_cache->items[idx].cur_pos];

						COAPC_DEBUG_TEMP("Found ip address in ip cache list\r\n"
										 "\t>>> host name:%s(len:%ld)\r\n"
										 "\t>>> ip address:%d.%d.%d.%d\r\n"
										 "\t>>> cur pos:%d\r\n"
										 "\t>>> cnt:%d\r\n",
										 host, host_len,
										 ((*ip_addr >> 24) & 0x0ff),
										 ((*ip_addr >> 16) & 0x0ff),
										 ((*ip_addr >> 8 ) & 0x0ff),
										 ((*ip_addr      ) & 0x0ff),
										 g_coap_client_ip_cache->items[idx].cur_pos,
										 g_coap_client_ip_cache->items[idx].cnt);

						return DA_APP_SUCCESS;
					}
				}
			}
		}

		//send dns query
		ret = (int)dns_ALL_Query((unsigned char *)host,
							(unsigned char *)&ip_buf[0], (COAP_CLIENT_MAX_IP_CACHE * 4),
							&ip_cnt, dns_query_wait_option);
		if (ret) {
			COAPC_DEBUG_ERR("failed to get ip address(dns query error(0x%x)\r\n", ret);
			return DA_APP_NOT_SUCCESSFUL;
		}

#if defined (ENABLE_COAPC_DEBUG_INFO)
		COAPC_DEBUG_INFO("CoAP Server IP Address\r\n");

		for (idx = 0 ; idx < ip_cnt ; idx++) {
			COAPC_DEBUG_INFO("\t%d. %d.%d.%d.%d\r\n", idx + 1,
							 (ntohl(ip_buf[idx]) >> 24) & 0x0ff,
							 (ntohl(ip_buf[idx]) >> 16) & 0x0ff,
							 (ntohl(ip_buf[idx]) >> 8) & 0x0ff,
							 (ntohl(ip_buf[idx])) & 0x0ff);
		}
#endif // (ENABLE_COAPC_DEBUG_INFO)

		//save ip address to ip cache
		if (g_coap_client_ip_cache) {
			for (idx = 0 ; idx < g_coap_client_ip_cache->idx ; idx++) {
				if (memcmp(host, g_coap_client_ip_cache->items[idx].host_name, host_len) == 0) {
					break;
				}
			}

			if (idx == g_coap_client_ip_cache->idx) {
				if (g_coap_client_ip_cache->idx < COAP_CLIENT_MAX_IP_CACHE_LIST) {
					idx = g_coap_client_ip_cache->idx;
					g_coap_client_ip_cache->idx++;
				} else {
					idx = g_coap_client_ip_cache->idx - 1;
				}
			}

			//update server ip cache
			g_coap_client_ip_cache->items[idx].cnt = ip_cnt;
			g_coap_client_ip_cache->items[idx].cur_pos = 0;

			memset(g_coap_client_ip_cache->items[idx].host_name,
				   0x00, sizeof(g_coap_client_ip_cache->items[idx].host_name));

			memset(g_coap_client_ip_cache->items[idx].ip_addr,
				   0x00, sizeof(g_coap_client_ip_cache->items[idx].ip_addr));

			memcpy(g_coap_client_ip_cache->items[idx].host_name, host, host_len);

			for (unsigned int ip_idx = 0 ; ip_idx < ip_cnt ; ip_idx++) {
				g_coap_client_ip_cache->items[idx].ip_addr[ip_idx] = ntohl(ip_buf[ip_idx]);
			}

			//return server ip address
			*ip_addr = g_coap_client_ip_cache->items[idx].ip_addr[0];

#if defined (ENABLE_COAPC_DEBUG_TEMP)
			COAPC_DEBUG_TEMP("Cached ip address\r\n");
			COAPC_PRINTF("\tnumber of cached host:%d\r\n", g_coap_client_ip_cache->idx);

			for (idx = 0 ; idx < g_coap_client_ip_cache->idx ; idx++) {
				COAPC_PRINTF("\t%d. host:%s\r\n"
							 "\t   cur_pos:%d\r\n"
							 "\t   number of ip address:%d\r\n",
							 idx + 1,
							 g_coap_client_ip_cache->items[idx].host_name,
							 g_coap_client_ip_cache->items[idx].cur_pos,
							 g_coap_client_ip_cache->items[idx].cnt);

				for (unsigned int ip_idx = 0 ; ip_idx < g_coap_client_ip_cache->items[idx].cnt ;
					 ip_idx++) {
					COAPC_PRINTF("\t   %d.%d.%d.%d\r\n",
								 (g_coap_client_ip_cache->items[idx].ip_addr[ip_idx] >> 24) & 0x0ff,
								 (g_coap_client_ip_cache->items[idx].ip_addr[ip_idx] >> 16) & 0x0ff,
								 (g_coap_client_ip_cache->items[idx].ip_addr[ip_idx] >> 8) & 0x0ff,
								 (g_coap_client_ip_cache->items[idx].ip_addr[ip_idx]) & 0x0ff);
				}
			}
#endif // (ENABLE_COAPC_DEBUG_TEMP)
		} else {
			*ip_addr = ntohl(ip_buf[0]);
		}
	}

	return DA_APP_SUCCESS;
}

/*
 * Parse coap-URI.
 * coap-URI = "coap:" "//" host [ ":" port ] path-abempty [ "?" query]
 * coaps-URI = "coaps:" "//" host [ ":" port ] path-abempty [ "?" query]
 */
int coap_client_parse_uri(unsigned char *uri, size_t uri_len,
						  coap_rw_buffer_t *scheme,
						  coap_rw_buffer_t *host,
						  coap_rw_buffer_t *path,
						  coap_rw_buffer_t *query,
						  unsigned long *ip_addr,
						  unsigned int *port,
						  unsigned int *secure_conn)
{
	unsigned char *p = NULL;
	unsigned char *q = NULL;
	size_t len = uri_len;

	/* search for scheme */
	p = uri;

	if (*p == '/') {
		q = p;
		goto path;
	}

	q = (unsigned char *)"coap";

	while (len && *q && coap_client_tolower(*p) == *q) {
		++p;
		++q;
		--len;
	}

	/* if q does not point to the string end marker '\0',
	   the schema identifier iw wrong.
	 */
	if (*q) {
		goto error;
	}

	/* There might be an additional 's',
	   indicating the secure version.
	 */
	if (len && (coap_client_tolower(*p) == 's')) {
		++p;
		--len;

		if (port) {
			*port = COAPS_PORT;
		}

		if (secure_conn) {
			*secure_conn = pdTRUE;
		}

		if (scheme) {
			if (coap_set_rw_buffer(scheme, "coaps", 5) < 0) {
				goto error;
			}
		}
	} else {
		if (port) {
			*port = COAP_PORT;
		}

		if (secure_conn) {
			*secure_conn = pdFALSE;
		}

		if (scheme) {
			if (coap_set_rw_buffer(scheme, "coap", 5) < 0) {
				goto error;
			}
		}
	}

	q = (unsigned char *)"://";

	while (len && *q && coap_client_tolower(*p) == *q) {
		++p;
		++q;
		--len;
	}

	if (*q) {
		goto error;
	}

	/* p points to beginning of Uri-Host */
	q = p;

	if (len && *p == '[') {	/* IPv6 address reference */
		++p;

		while (len && *q != ']') {
			++q;
			--len;
		}

		if (!len || *q != ']' || p == q) {
			goto error;
		}

		if (host) {
			if (coap_set_rw_buffer(host, (const char *)p, (size_t)(q - p)) < 0) {
				goto error;
			}
		}

		++q;
		--len;
	} else { /* IPv4 address or FQDN */
		while (len && *q != ':' && *q != '/' && *q != '?') {
			*q = (unsigned char)coap_client_tolower(*q);
			++q;
			--len;
		}

		if (p == q) {
			goto error;
		}

		if (host) {
			if (coap_set_rw_buffer(host, (const char *)p, (size_t)(q - p)) < 0) {
				goto error;
			}
		}
	}

	/* resolve ip address */
	if (host && ip_addr) {
		if (coap_client_get_ip_addr((char *)host->p, host->len,
									ip_addr) != DA_APP_SUCCESS) {
			COAPC_DEBUG_ERR("failed to get ip address\r\n");
			goto error;
		}
	}

	/* check for Uri-Port */
	if (len && *q == ':') {
		p = ++q;
		--len;

		while (len && isdigit(*q)) {
			++q;
			--len;
		}

		if (p < q) { /* explicit port number given */
			unsigned int parsed_port = 0;

			while (p < q) {
				parsed_port = parsed_port * 10 + (*p++ - '0');
			}

			if (port) {
				*port = parsed_port;
			}
		}
	}

	/* at this point, p must point to an absolute path */
path:

	if (!len) {
		goto end;
	}

	if (*q == '/') {
		p = ++q;
		--len;

		while (len && *q != '?') {
			++q;
			--len;
		}

		if (p < q) {
			if (path) {
				if (coap_set_rw_buffer(path, (const char *)p, (size_t)(q - p)) < 0) {
					goto error;
				}
			}

			p = q;
		}
	}

	/* Uri_Query */
	if (len && *p == '?') {
		++p;
		--len;

		if (query) {
			if (coap_set_rw_buffer(query, (const char *)p, len) < 0) {
				goto error;
			}
		}

		len = 0;
	}

end:
	return len ? DA_APP_INVALID_PARAMETERS : DA_APP_SUCCESS;
error:
	return DA_APP_INVALID_PARAMETERS;
}

void coap_client_clear_request(coap_client_req_t *request_ptr)
{
	request_ptr->type = COAP_MSG_TYPE_CON;
	request_ptr->method = COAP_METHOD_NONE;
	coap_free_rw_buffer(&request_ptr->scheme);
	coap_free_rw_buffer(&request_ptr->host);
	coap_free_rw_buffer(&request_ptr->path);
	coap_free_rw_buffer(&request_ptr->query);
	request_ptr->port = COAP_PORT;

	coap_free_rw_buffer(&request_ptr->proxy_uri);
	//coap_free_rw_buffer(&request_ptr->proxy_host);
	//coap_free_rw_buffer(&request_ptr->proxy_path);
	//coap_free_rw_buffer(&request_ptr->proxy_query);
	//request_ptr->proxy_port = 0;

	request_ptr->observe_reg = 0;

	request_ptr->payload.p = NULL;
	request_ptr->payload.len = 0;

	request_ptr->block1_num = 0;
	request_ptr->block1_more = 0;
	request_ptr->block1_szx = COAP_CLIENT_BLOCK_SIZE;

	request_ptr->block2_num = 0;
	request_ptr->block2_more = 0;
	request_ptr->block2_szx = COAP_CLIENT_BLOCK_SIZE;

	return ;
}

int coap_client_init_connection(coap_client_conn_t *conn_ptr,
								char *name_ptr, unsigned int port,
								unsigned long peer_ip_addr, unsigned int peer_port,
								unsigned int secure_conn)
{
	int ret = DA_APP_SUCCESS;

	char str_local_port[32] = {0x00,};
	char str_peer_ip_addr[32] = {0x00,};
	char str_peer_port[32] = {0x00,};

	strcpy((char *)conn_ptr->socket_name, name_ptr);
	conn_ptr->peer_ip_addr = peer_ip_addr;
	conn_ptr->peer_port = peer_port;
	conn_ptr->secure_conn = secure_conn;
	conn_ptr->local_port = port;

	sprintf(str_local_port, "%d", conn_ptr->local_port);
	sprintf(str_peer_port, "%d", conn_ptr->peer_port);
	sprintf(str_peer_ip_addr, "%ld.%ld.%ld.%ld",
			(conn_ptr->peer_ip_addr >> 24) & 0x0ff,
			(conn_ptr->peer_ip_addr >> 16) & 0x0ff,
			(conn_ptr->peer_ip_addr >>  8) & 0x0ff,
			conn_ptr->peer_ip_addr & 0x0ff);

	COAPC_DEBUG_INFO("Peer IP information(%s:%s), local port(%s)\r\n",
					 str_peer_ip_addr, str_peer_port, str_local_port);

	mbedtls_net_init(&conn_ptr->sock_fd);

	if (port) {
		ret = mbedtls_net_bsd_connect_with_bind(&conn_ptr->sock_fd,
				str_peer_ip_addr, str_peer_port,
				MBEDTLS_NET_PROTO_UDP,
				NULL, str_local_port);
	} else {
		ret = mbedtls_net_connect(&conn_ptr->sock_fd,
				str_peer_ip_addr,
				str_peer_port,
				MBEDTLS_NET_PROTO_UDP);
	}
	if (ret) {
		COAPC_DEBUG_ERR("failed to create socket(0x%x)\r\n", -ret);
		return ret;
	}

	return ret;
}

int coap_client_deinit_connection(coap_client_conn_t *conn_ptr)
{
	mbedtls_net_free(&conn_ptr->sock_fd);

	return DA_APP_SUCCESS;
}

void coap_client_init_msgId(unsigned short *msgId)
{
	*msgId = 0;
}

void coap_client_increase_msgId(unsigned short *msgId)
{
	(*msgId)++;
}

void coap_client_init_token(unsigned char *token)
{
	for (int index = 0 ; index < COAP_CLIENT_TOKEN_LENGTH ; index++) {
		token[index] = (unsigned char)(rand() % 256);
	}

	return ;
}

void coap_client_inc_token(unsigned char *token)
{
	int index = COAP_CLIENT_TOKEN_LENGTH - 1;

	for (index = COAP_CLIENT_TOKEN_LENGTH - 1 ; index >= 0 ; index--) {
		if (token[index] == 255) {
			token[index] = 0x00;
		} else {
			break;
		}
	}

	if (index >= 0) {
		token[index] += 1;
	} else {
		coap_client_init_token(token);
	}

	return ;
}

//internal function for dtls
int coaps_client_init(coap_client_conn_t *conn_ptr,
					  coap_client_conf_t *config_ptr)
{
	int ret = DA_APP_MALLOC_ERROR;
	const char *pers = "coaps_client";
	int preset = MBEDTLS_SSL_PRESET_DA16X;

#if defined(__SUPPORT_TLS_HW_CIPHER_SUITES__)
	preset = MBEDTLS_SSL_PRESET_DA16X_ONLY_HW;
#endif /* __SUPPORT_TLS_HW_CIPHER_SUITES__ */

	if (!conn_ptr->ssl_ctx) {
		conn_ptr->ssl_ctx = coap_calloc(1, sizeof(mbedtls_ssl_context));
		if (conn_ptr->ssl_ctx == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for ssl_ctx\r\n");
			goto error;
		}

		mbedtls_ssl_init(conn_ptr->ssl_ctx);
	}

	if (!conn_ptr->ssl_conf) {
		conn_ptr->ssl_conf = coap_calloc(1, sizeof(mbedtls_ssl_config));
		if (conn_ptr->ssl_conf == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for ssl_conf\r\n");
			goto error;
		}

		mbedtls_ssl_config_init(conn_ptr->ssl_conf);
	}

	if (!conn_ptr->ctr_drbg) {
		conn_ptr->ctr_drbg = coap_calloc(1, sizeof(mbedtls_ctr_drbg_context));
		if (conn_ptr->ctr_drbg == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for ctr_drbg\r\n");
			goto error;
		}

		mbedtls_ctr_drbg_init(conn_ptr->ctr_drbg);
	}

	if (!conn_ptr->entropy) {
		conn_ptr->entropy = coap_calloc(1, sizeof(mbedtls_entropy_context));
		if (conn_ptr->entropy == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for entropy\r\n");
			goto error;
		}

		mbedtls_entropy_init(conn_ptr->entropy);
	}

	if (!config_ptr->ca_cert && config_ptr->ca_cert_len > 0) {
		conn_ptr->ca_cert = coap_calloc(1, sizeof(mbedtls_x509_crt));
		if (conn_ptr->ca_cert == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for ca cert\r\n");
			goto error;
		}

		mbedtls_x509_crt_init(conn_ptr->ca_cert);
	}

	if (!(config_ptr->cert) && (config_ptr->cert_len > 0)
		&& !(config_ptr->pkey) && (config_ptr->pkey_len > 0)) {
		conn_ptr->cert = coap_calloc(1, sizeof(mbedtls_x509_crt));
		if (conn_ptr->cert == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for cert\r\n");
			goto error;
		}

		mbedtls_x509_crt_init(conn_ptr->cert);

		conn_ptr->pkey = coap_calloc(1, sizeof(mbedtls_pk_context));
		if (conn_ptr->pkey == NULL) {
			COAPC_DEBUG_ERR("failed to allocate memory for priv key\r\n");
			goto error;
		}

		mbedtls_pk_init(conn_ptr->pkey);
	}

#if defined (MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(COAPS_CLIENT_DEBUG_LEVEL);
#endif // (MBEDTLS_DEBUG_C)

	ret = mbedtls_ssl_config_defaults(conn_ptr->ssl_conf,
									  MBEDTLS_SSL_IS_CLIENT,
									  MBEDTLS_SSL_TRANSPORT_DATAGRAM,
									  preset);
	if (ret != 0) {
		COAPC_DEBUG_ERR("failed to set default ssl config(0x%x)\r\n", -ret);
		goto error;
	}

	if (config_ptr->ca_cert && (config_ptr->ca_cert_len > 0)) {
		ret = mbedtls_x509_crt_parse(conn_ptr->ca_cert,
									 config_ptr->ca_cert,
									 config_ptr->ca_cert_len);
		if (ret != 0) {
			COAPC_DEBUG_ERR("failed to parse ca cert(0x%x)\r\n", -ret);
			goto error;
		}

		mbedtls_ssl_conf_ca_chain(conn_ptr->ssl_conf, conn_ptr->ca_cert, NULL);
	}

	if (config_ptr->cert && (config_ptr->cert_len > 0)
		&& config_ptr->pkey && (config_ptr->pkey_len > 0)) {
		ret = mbedtls_x509_crt_parse(conn_ptr->cert,
									 config_ptr->cert,
									 config_ptr->cert_len);
		if (ret != 0) {
			COAPC_DEBUG_ERR("failed to parse cert(0x%x)\r\n", -ret);
			goto error;
		}

		ret = mbedtls_pk_parse_key(conn_ptr->pkey,
								   config_ptr->pkey,
								   config_ptr->pkey_len,
								   NULL, 0);
		if (ret != 0) {
			COAPC_DEBUG_ERR("failed to parse private key(0x%x)\r\n", -ret);
			goto error;
		}

		ret = mbedtls_ssl_conf_own_cert(conn_ptr->ssl_conf,
										conn_ptr->cert,
										conn_ptr->pkey);
		if (ret != 0) {
			COAPC_DEBUG_ERR("failed to set cert & private key(0x%x)\r\n", -ret);
			goto error;
		}
	}

#if defined (MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	if ((config_ptr->psk_key && (config_ptr->psk_key_len > 0))
		&& (config_ptr->psk_identity && (config_ptr->psk_identity_len > 0))) {
		ret = mbedtls_ssl_conf_psk(conn_ptr->ssl_conf,
								   config_ptr->psk_key,
								   config_ptr->psk_key_len,
								   config_ptr->psk_identity,
								   config_ptr->psk_identity_len);
		if (ret != 0) {
			COAPC_DEBUG_ERR("failed to set psk(0x%x)\r\n", -ret);
			goto error;
		}
	}
#endif	// (MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)

	ret = mbedtls_ctr_drbg_seed(conn_ptr->ctr_drbg,
								mbedtls_entropy_func,
								conn_ptr->entropy,
								(const unsigned char *)pers,
								strlen((char *)pers));
	if (ret != 0) {
		COAPC_DEBUG_ERR("failed to set ctr drbg seed(0x%x)\r\n", -ret);
		goto error;
	}

	mbedtls_ssl_conf_rng(conn_ptr->ssl_conf,
						 mbedtls_ctr_drbg_random,
						 conn_ptr->ctr_drbg);

	mbedtls_ssl_conf_dbg(conn_ptr->ssl_conf, coap_client_secure_debug, NULL);

	mbedtls_ssl_conf_dtls_anti_replay(conn_ptr->ssl_conf,
									  MBEDTLS_SSL_ANTI_REPLAY_ENABLED);

	mbedtls_ssl_conf_authmode(conn_ptr->ssl_conf, (int)(config_ptr->authmode));

	mbedtls_ssl_conf_max_frag_len(conn_ptr->ssl_conf, (unsigned char)config_ptr->fragment);

	mbedtls_ssl_conf_read_timeout(conn_ptr->ssl_conf,
								  COAP_CLIENT_ACK_TIMEOUT * 100);

	mbedtls_ssl_conf_handshake_timeout(conn_ptr->ssl_conf,
									   config_ptr->handshake_min_timeout * 100,
									   config_ptr->handshake_max_timeout * 100);

	//TODO: set cipher suites for coaps
	//mbedtls_ssl_conf_ciphersuites(secure_conf->ssl_conf, cipher_suites);

	ret = mbedtls_ssl_setup(conn_ptr->ssl_ctx, conn_ptr->ssl_conf);
	if (ret != 0) {
		COAPC_DEBUG_ERR("failed to set ssl config(0x%x)\r\n", -ret);
		goto error;
	}

	mbedtls_ssl_set_timer_cb(conn_ptr->ssl_ctx,
							 &conn_ptr->timer,
							 coap_client_dtls_timer_start,
							 coap_client_dtls_timer_get_status);

	mbedtls_ssl_set_bio(conn_ptr->ssl_ctx, &conn_ptr->sock_fd,
						mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

	ret = coaps_client_restore_ssl(conn_ptr);
	if (ret) {
		COAPC_DEBUG_ERR("failed to restore dtls session(0x%x)\r\n", -ret);
		goto error;
	}

	return DA_APP_SUCCESS;

error:

	coaps_client_deinit(conn_ptr);

	return ret;

}

int coaps_client_deinit(coap_client_conn_t *conn_ptr)
{
	if (dpm_mode_is_enabled()) {
		coaps_client_clear_ssl(conn_ptr);
	}

	if (conn_ptr->ssl_ctx) {
		mbedtls_ssl_free(conn_ptr->ssl_ctx);
		coap_free(conn_ptr->ssl_ctx);
	}

	if (conn_ptr->ssl_conf) {
		mbedtls_ssl_config_free(conn_ptr->ssl_conf);
		coap_free(conn_ptr->ssl_conf);
	}

	if (conn_ptr->ctr_drbg) {
		mbedtls_ctr_drbg_free(conn_ptr->ctr_drbg);
		coap_free(conn_ptr->ctr_drbg);
	}

	if (conn_ptr->entropy) {
		mbedtls_entropy_free(conn_ptr->entropy);
		coap_free(conn_ptr->entropy);
	}

	if (conn_ptr->ca_cert) {
		mbedtls_x509_crt_free(conn_ptr->ca_cert);
		coap_free(conn_ptr->ca_cert);
	}

	if (conn_ptr->cert) {
		mbedtls_x509_crt_free(conn_ptr->cert);
		coap_free(conn_ptr->cert);
	}

	if (conn_ptr->pkey) {
		mbedtls_pk_free(conn_ptr->pkey);
		coap_free(conn_ptr->pkey);
	}

	conn_ptr->ssl_ctx = NULL;
	conn_ptr->ssl_conf = NULL;
	conn_ptr->ctr_drbg = NULL;
	conn_ptr->entropy = NULL;
	conn_ptr->ca_cert = NULL;
	conn_ptr->cert = NULL;
	conn_ptr->pkey = NULL;

	return DA_APP_SUCCESS;
}

int coaps_client_do_handshake(coap_client_conn_t *conn_ptr,
							  unsigned int secure_conn_max_retransmit)
{
	int ret = DA_APP_SUCCESS;
	unsigned int retry_cnt = 0;

	if (conn_ptr->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
		COAPC_DEBUG_INFO("Already established\r\n");
		return DA_APP_SUCCESS;
	}

	for (retry_cnt = 0 ; retry_cnt < secure_conn_max_retransmit ; retry_cnt++) {
		while ((ret = mbedtls_ssl_handshake(conn_ptr->ssl_ctx)) != 0) {
			if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
				COAPC_DEBUG_ERR("Peer closed the connection(0x%x)\r\n", -ret);
				break;
			}

			if ((ret != MBEDTLS_ERR_SSL_WANT_READ)
				&& (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
				COAPC_DEBUG_ERR("failed to progress dtls handshake(0x%x)\r\n", -ret);
				break;
			}
		}

		ret = (int)mbedtls_ssl_get_verify_result(conn_ptr->ssl_ctx);
		if (ret) {
			COAPC_DEBUG_ERR("failed to verify dtls session(0x%x)\r\n", ret);
			ret = DA_APP_NOT_SUCCESSFUL;
			break;
		}

		if (ret == 0) {
			//handshake is over successfully
			COAPC_DEBUG_INFO("Handshake is over successfully\r\n");
			break;
		} else {
			COAPC_DEBUG_INFO("#%d. Retry Handshake\r\n", retry_cnt + 1);
			mbedtls_ssl_session_reset(conn_ptr->ssl_ctx);
		}
	}

	if (ret == 0) {
		ret = coaps_client_save_ssl(conn_ptr);
		if (ret) {
			COAPC_DEBUG_ERR("failed to save dtls session(0x%x)\r\n", -ret);
		}
	}

	return ret;
}

void coaps_client_send_close_notify(coap_client_conn_t *conn_ptr)
{
	mbedtls_ssl_close_notify(conn_ptr->ssl_ctx);
}

int coaps_client_save_ssl(coap_client_conn_t *conn_ptr)
{
	int ret = 0;

	if (!dpm_mode_is_enabled()) {
		return DA_APP_SUCCESS;
	}

	COAPC_DEBUG_INFO("To save DTLS session(%s)\r\n", conn_ptr->socket_name);

	ret = set_tls_session((const char *)conn_ptr->socket_name, conn_ptr->ssl_ctx);
	if (ret) {
		COAPC_DEBUG_ERR("failed to save dtls session(0x%x)\r\n", -ret);
		return DA_APP_NOT_SUCCESSFUL;
	}

	return DA_APP_SUCCESS;
}

int coaps_client_restore_ssl(coap_client_conn_t *conn_ptr)
{
	int ret = 0;

	if (!dpm_mode_is_enabled()) {
		return DA_APP_SUCCESS;
	}

	COAPC_DEBUG_INFO("To restore DTLS session(%s)\r\n", conn_ptr->socket_name);

	ret = get_tls_session((const char *)conn_ptr->socket_name, conn_ptr->ssl_ctx);
	if (ret == ER_NOT_FOUND) {
		COAPC_DEBUG_INFO("Not found(%s)\r\n", conn_ptr->socket_name);
		return DA_APP_SUCCESS;
	} else if (ret != 0) {
		COAPC_DEBUG_ERR("failed to restore dtls session(0x%x)\r\n", -ret);
		return DA_APP_NOT_SUCCESSFUL;
	}

	return DA_APP_SUCCESS;
}

int coaps_client_clear_ssl(coap_client_conn_t *conn_ptr)
{
	int ret = 0;

	if (!dpm_mode_is_enabled()) {
		return DA_APP_SUCCESS;
	}

	COAPC_DEBUG_INFO("To clear DTLS session(%s)\r\n", conn_ptr->socket_name);

	ret = clr_tls_session((const char *)conn_ptr->socket_name);
	if (ret) {
		COAPC_DEBUG_ERR("failed to restore dtls session(0x%x)\r\n", -ret);
		return DA_APP_NOT_SUCCESSFUL;
	}

	return DA_APP_SUCCESS;
}

static void coap_client_secure_debug(void *ctx, int level, const char *file,
									 int line,
									 const char *str)
{
	const char *p, *basename;

	LWIP_UNUSED_ARG(ctx);

	for (p = basename = file ; *p != '\0' ; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	COAPC_PRINTF("%s:%04d: |%d| %s\r\n", basename, line, level, str);
}

void coap_client_dtls_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
	coap_timer_t *timer_ptr = (coap_timer_t *)ctx;

	if (!timer_ptr) {
		COAPC_DEBUG_ERR("[%s] Invaild paramter\n", __func__);
		return ;
	}

	timer_ptr->int_ms = int_ms;
	timer_ptr->fin_ms = fin_ms;

	if (fin_ms != 0) {
		timer_ptr->snapshot = xTaskGetTickCount();
	}

	return ;
}

int coap_client_dtls_timer_get_status(void *ctx)
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

	COAPC_DEBUG_TEMP("fin_ms(%d), int_ms(%d), snapshot(%d), "
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

int coap_client_send(coap_client_conn_t *conn, unsigned char *data, size_t len)
{
	int ret = 0;

	if (conn->secure_conn) {
		do {
			ret = mbedtls_ssl_write(conn->ssl_ctx, data, len);
		} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	} else {
		ret = mbedtls_net_send(&conn->sock_fd, data, len);
	}

	if (ret < 0) {
		COAPC_DEBUG_ERR("Failed to send data(0x%x)\r\n", -ret);
		return ret;
	}

	return DA_APP_SUCCESS;
}

int coap_client_recv(coap_client_conn_t *conn,
					 unsigned char *buf, size_t buflen,
					 unsigned int wait_option)
{
	int ret = 0;

	if (conn->secure_conn) {
		mbedtls_ssl_conf_read_timeout(conn->ssl_conf, wait_option);

		ret = mbedtls_ssl_read(conn->ssl_ctx, buf, buflen);
	} else {
		ret = mbedtls_net_recv_timeout(&conn->sock_fd, buf, buflen, wait_option);
	}

	if (ret < 0) {
		switch (ret) {
			case MBEDTLS_ERR_SSL_TIMEOUT:
				COAPC_DEBUG_INFO("Timeout\r\n");
				ret = DA_APP_NO_PACKET;
				break;
			case MBEDTLS_ERR_SSL_WANT_READ:
			case MBEDTLS_ERR_SSL_WANT_WRITE:
				COAPC_DEBUG_INFO("Need more data(0x%x)\r\n", -ret);
				ret = DA_APP_NO_PACKET;
				break;
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				COAPC_DEBUG_INFO("Connection was closed gracefully\r\n");
				break;
			case MBEDTLS_ERR_NET_CONN_RESET:
				COAPC_DEBUG_INFO("Connection was reset by peer\r\n");
				break;
			default:
				COAPC_DEBUG_INFO("Failed to recv data(0x%x)\r\n", -ret);
				break;
		}
	}

	return ret;
}

#if defined (ENABLE_COAPC_DEBUG_INFO)
static void coap_client_print_conn(coap_client_conn_t *conn_ptr)
{
	COAPC_PRINTF("\t%-20s\t:\t%d.%d.%d.%d(%d)\r\n",
				 "CoAP Server",
				 (conn_ptr->peer_ip_addr >> 24) & 0x0ff,
				 (conn_ptr->peer_ip_addr >> 16) & 0x0ff,
				 (conn_ptr->peer_ip_addr >> 8) & 0x0ff,
				 (conn_ptr->peer_ip_addr     ) & 0x0ff,
				 conn_ptr->peer_port);

	COAPC_PRINTF("\t%-20s\t:\t%s\r\n", "Secure conn",
				 conn_ptr->secure_conn ? "Yes" : "No");

	return ;
}

static void coap_client_print_req(coap_client_req_t *request_ptr)
{
	unsigned int index = 0;

	COAPC_PRINTF("\t%-20s\t:\t", "Request Type");

	switch (request_ptr->type) {
		case COAP_MSG_TYPE_CON:
			COAPC_PRINTF("CONFIRMABLE\r\n");
			break;
		case COAP_MSG_TYPE_NONCON:
			COAPC_PRINTF("NON-CONFIRMABLE\r\n");
			break;
		case COAP_MSG_TYPE_ACK:
			COAPC_PRINTF("ACK\r\n");
			break;
		case COAP_MSG_TYPE_RESET:
			COAPC_PRINTF("RESET\r\n");
			break;
		default:
			COAPC_PRINTF("Unknown type(%d)\r\n", request_ptr->type);
			break;
	}

	COAPC_PRINTF("\t%-20s\t:\t", "Method");

	switch (request_ptr->method) {
		case COAP_METHOD_NONE:
			COAPC_PRINTF("PING\r\n");
			break;
		case COAP_METHOD_GET:
			if (request_ptr->observe_reg == 1) {
				COAPC_PRINTF("Register Observe - GET\r\n");
			} else if (request_ptr->observe_reg == 2) {
				COAPC_PRINTF("Deregister Observe - GET\r\n");
			} else {
				COAPC_PRINTF("GET\r\n");
			}
			break;
		case COAP_METHOD_POST:
			COAPC_PRINTF("POST\r\n");
			break;
		case COAP_METHOD_PUT:
			COAPC_PRINTF("PUT\r\n");
			break;
		case COAP_METHOD_DELETE:
			COAPC_PRINTF("DELETE\r\n");
			break;
		default:
			COAPC_PRINTF("Unknown method(%d)\r\n", request_ptr->method);
			break;
	}

	if (request_ptr->scheme.len) {
		COAPC_PRINTF("\t%-20s\t:\t%s(len:%ld)\r\n",
					 "Scheme", request_ptr->scheme.p, request_ptr->scheme.len);
	}

	if (request_ptr->host.len) {
		COAPC_PRINTF("\t%-20s\t:\t", "Host");

		for (index = 0 ; index < request_ptr->host.len ; index++) {
			if (request_ptr->host.p[index] == 0x0A) {
				COAPC_PRINTF("\r\n");
			} else if (request_ptr->host.p[index] == 0x09) {
				COAPC_PRINTF("\t");
			} else if ((request_ptr->host.p[index] > 0x1F)
					   && (request_ptr->host.p[index] < 0x7F)) {
				COAPC_PRINTF("%c", request_ptr->host.p[index]);
			} else {
				COAPC_PRINTF(".");
			}
		}

		COAPC_PRINTF("(len:%ld)\r\n", request_ptr->host.len);
	}

	if (request_ptr->path.len) {
		COAPC_PRINTF("\t%-20s\t:\t", "Path");

		for (index = 0 ; index < request_ptr->path.len ; index++) {
			if (request_ptr->path.p[index] == 0x0A) {
				COAPC_PRINTF("\r\n");
			} else if (request_ptr->path.p[index] == 0x09) {
				COAPC_PRINTF("\t");
			} else if ((request_ptr->path.p[index] > 0x1F)
					   && (request_ptr->path.p[index] < 0x7F)) {
				COAPC_PRINTF("%c", request_ptr->path.p[index]);
			} else {
				COAPC_PRINTF(".");
			}
		}

		COAPC_PRINTF("(len:%ld)\r\n", request_ptr->path.len);
	}

	if (request_ptr->query.len) {
		COAPC_PRINTF("\t%-20s\t:\t", "Query");

		for (index = 0 ; index < request_ptr->query.len ; index++) {
			if (request_ptr->query.p[index] == 0x0A) {
				COAPC_PRINTF("\r\n");
			} else if (request_ptr->query.p[index] == 0x09) {
				COAPC_PRINTF("\t");
			} else if ((request_ptr->query.p[index] > 0x1F)
					   && (request_ptr->query.p[index] < 0x7F)) {
				COAPC_PRINTF("%c", request_ptr->query.p[index]);
			} else {
				COAPC_PRINTF(".");
			}
		}

		COAPC_PRINTF("(len:%ld)\r\n", request_ptr->query.len);
	}

	if (request_ptr->proxy_uri.len) {
		COAPC_PRINTF("\t%-20s\t:\t", "Proxy-URI");

		for (index = 0 ; index < request_ptr->proxy_uri.len ; index++) {
			if (request_ptr->proxy_uri.p[index] == 0x0A) {
				COAPC_PRINTF("\r\n");
			} else if (request_ptr->proxy_uri.p[index] == 0x09) {
				COAPC_PRINTF("\t");
			} else if ((request_ptr->proxy_uri.p[index] > 0x1F)
					   && (request_ptr->proxy_uri.p[index] < 0x7F)) {
				COAPC_PRINTF("%c", request_ptr->proxy_uri.p[index]);
			} else {
				COAPC_PRINTF(".");
			}
		}

		COAPC_PRINTF("(len:%ld)\r\n", request_ptr->proxy_uri.len);
	}

	if (request_ptr->payload.len) {
		COAPC_PRINTF("\t%-20s\t:\t(len:%ld)\r\n", "Payload", request_ptr->payload.len);

		for (index = 0 ; index < request_ptr->payload.len ; index++) {
			if (request_ptr->payload.p[index] == 0x0A) {
				COAPC_PRINTF("\r\n");
			} else if (request_ptr->payload.p[index] == 0x09) {
				COAPC_PRINTF("\t");
			} else if ((request_ptr->payload.p[index] > 0x1F)
					   && (request_ptr->payload.p[index] < 0x7F)) {
				COAPC_PRINTF("%c", request_ptr->payload.p[index]);
			} else {
				COAPC_PRINTF(".", request_ptr->payload.p[index]);
			}
		}

		COAPC_PRINTF("\r\n");
	}

	COAPC_PRINTF("\t%-20s\t:\t%d/%d/%d\r\n", "BLOCK#1",
				 request_ptr->block1_num,
				 request_ptr->block1_more,
				 request_ptr->block1_szx);
	COAPC_PRINTF("\t%-20s\t:\t%d/%d/%d\r\n", "BLOCK#2",
				 request_ptr->block2_num,
				 request_ptr->block2_more,
				 request_ptr->block2_szx);

	COAPC_PRINTF("\t%-20s\t:\t%d\r\n", "MSG ID", *request_ptr->msgId_ptr);

	COAPC_PRINTF("\t%-20s\t:\t", "TOKEN");

	for (index = 0 ; index < COAP_CLIENT_TOKEN_LENGTH ; index++) {
		COAPC_PRINTF("0x%02x ", request_ptr->token_ptr[index]);
	}

	COAPC_PRINTF("\r\n");

	return ;
}
#endif	// (ENABLE_COAPC_DEBUG_INFO)

/* EOF */
