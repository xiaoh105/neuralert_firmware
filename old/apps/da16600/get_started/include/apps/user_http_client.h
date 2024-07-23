/**
 ****************************************************************************************
 *
 * @file user_http_client.h
 *
 * @brief HTTP Client feature.
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

#ifndef __USER_HTTP_CLIENT_H__
#define __USER_HTTP_CLIENT_H__

#include "FreeRTOSConfig.h"
#include "da16x_system.h"
#include "da16x_network_common.h"
#include "application.h"
#include "iface_defs.h"
#include "common_def.h"

#include "user_dpm_api.h"

#include "lwip/apps/http_client.h"

/// Debug Log level(Information) for HTTP Client
#undef  ENABLE_HTTPC_DEBUG_INFO

/// Debug Log level(Error) for HTTP Client
#define ENABLE_HTTPC_DEBUG_ERR

/// Debug Log level(Hex Dump) for HTTP Client
#undef  ENABLE_HTTPC_DEBUG_DUMP

/// Name of HTTP Client
#define HTTPC_XTASK_NAME              "HttpClient"

/// Stack size of HTTP Client
#define HTTPC_STACK_SZ                (1024 * 6) / 4 //WORD

/// Max size of HTTP Client's request data
#define HTTPC_MAX_REQ_DATA            HTTPC_REQ_DATA_LEN

/// Default delay for HTTP Client
#define HTTPC_DEF_TIMEOUT             10


#define HTTP_SERVER_PORT              80
#define HTTPS_SERVER_PORT             443
#define HTTPC_MAX_HOSTNAME_LEN        256
#define HTTPC_MAX_PATH_LEN            256
#define HTTPC_MAX_NAME                20
#define HTTPC_MAX_PASSWORD            20

#define HTTPC_MIN_INCOMING_LEN        (1024 * 1)
#define HTTPC_MAX_INCOMING_LEN        (1024 * 20)
#define HTTPC_DEF_INCOMING_LEN        (1024 * 4)
#define HTTPC_MIN_OUTGOING_LEN        (1024 * 1)
#define HTTPC_MAX_OUTGOING_LEN        (1024 * 20)
#define HTTPC_DEF_OUTGOING_LEN        (1024 * 4)

#define HTTPC_MAX_ALPN_CNT            3
#define HTTPC_MAX_ALPN_LEN            24
#define HTTPC_MAX_SNI_LEN             64
#define HTTPC_MAX_STOP_TIMEOUT        (300 * HTTPC_DEF_TIMEOUT)

// NVRAM name of HTTP-CLIENT TLS auth_mode
#define HTTPC_NVRAM_CONFIG_TLS_AUTH       "HTTPC_TLS_AUTH"

// NVRAM name of HTTP-CLIENT TLS alpn
#define HTTPC_NVRAM_CONFIG_TLS_ALPN       "HTTPC_TLS_ALPN"

// NVRAM name of the number of TLS alpn
#define HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM   "HTTPC_TLS_ALPN_NUM"

// NVRAM name of HTTP-CLIENT TLS SNI
#define HTTPC_NVRAM_CONFIG_TLS_SNI        "HTTPC_TLS_SNI"

/// Status of HTTP Client
typedef enum {
    HTTP_CLIENT_STATUS_READY,
    HTTP_CLIENT_STATUS_WAIT,
    HTTP_CLIENT_STATUS_PROGRESS,
} DA16_HTTP_CLIENT_STATUS;

/// Operation code of HTTP Client
typedef enum {
    /// Init value
    HTTP_CLIENT_OPCODE_READY,
    /// Head request
    HTTP_CLIENT_OPCODE_HEAD,
    /// GET request
    HTTP_CLIENT_OPCODE_GET,
    /// PUT request
    HTTP_CLIENT_OPCODE_PUT,
    /// POST request
    HTTP_CLIENT_OPCODE_POST,
    /// DELETE request
    HTTP_CLIENT_OPCODE_DELETE,
    /// User Generated Messages request
    HTTP_CLIENT_OPCODE_MESSAGE,
    /// Display status of HTTP client
    HTTP_CLIENT_OPCODE_STATUS,
    /// Display Help menu
    HTTP_CLIENT_OPCODE_HELP,
    /// Stop HTTP Client
     HTTP_CLIENT_OPCODE_STOP,
} DA16_HTTP_CLIENT_OPCODE;

/// HTTP Request structure
typedef struct http_client_request {
    /// Operation code
    DA16_HTTP_CLIENT_OPCODE        op_code;

    /// Interface
    UINT                        iface;
    /// Port number of HTTP Server
    UINT                        port;
    /// Secure Mode
    UINT                        insecure;

    /// Host Name of HTTP request
    CHAR                        hostname[HTTPC_MAX_HOSTNAME_LEN];
    /// Path of HTTP request
    CHAR                        path[HTTPC_MAX_PATH_LEN];
    /// Data of HTTP request
    CHAR                        data[HTTPC_MAX_REQ_DATA];
    /// User name of HTTP request
    CHAR                        username[HTTPC_MAX_NAME];
    /// Password of HTTP request
    CHAR                        password[HTTPC_MAX_PASSWORD];

    // TLS Configuration
    httpc_secure_connection_t    https_conf;

} DA16_HTTP_CLIENT_REQUEST;

/// Data transfer with AT-CMD interface
typedef struct http_client_receive {
    // Flag
    bool            chunked;
    // Parsed Chunked Size
    int             chunked_len;
    // Chunked payload not yet received
    int             chunked_remain_len;

} DA16_HTTP_CLIENT_RECEIVE;

#if defined (__SUPPORT_ATCMD__)
    /// Data transfer with AT-CMD interface
typedef struct _atcmd_httpc_context {
    // Flag
    bool            insert;
    // Status
    char            status[3];
    // Tx buffer
    char            *buffer;
    // Tx buffer length
    UINT            buffer_len;
} atcmd_httpc_context;
#endif // (__SUPPORT_ATCMD__)

/// HTTP Client configuration
typedef struct http_client_conf {
    /// Status of HTTP Client
    DA16_HTTP_CLIENT_STATUS        status;
    /// HTTP request instance
    DA16_HTTP_CLIENT_REQUEST    request;
    /// HTTP receive instance
    DA16_HTTP_CLIENT_RECEIVE        receive;
#if defined (__SUPPORT_ATCMD__)
    /// Data transfer with AT-CMD interface
    atcmd_httpc_context             atcmd;
#endif // (__SUPPORT_ATCMD__)
} DA16_HTTP_CLIENT_CONF;


/**
 ****************************************************************************************
 * @brief Parse URI to generate HTTP Client's reuqest.
 * @param[in]  uri      URI
 * @param[in]  len      Length of URI
 * @param[in]  request  Pointer of HTTP Client's request instance
 * @return     0(NX_SUCCESS) on success.
 * @note This function is called internally.
 ****************************************************************************************
 */
err_t http_client_parse_uri(unsigned char *uri, size_t len, DA16_HTTP_CLIENT_REQUEST *request);

/**
 ****************************************************************************************
 * @brief Display usage of HTTP Client's CMD.
 ****************************************************************************************
 */
void http_client_display_usage(void);

/**
 ****************************************************************************************
 * @brief Display HTTP requst information.
 * @param[in]  config   Pointer of HTTP Client's configuration instance
 * @param[in]  request  Pointer of HTTP Client's requuest instance
 ****************************************************************************************
 */
void http_client_display_request(DA16_HTTP_CLIENT_CONF *config, DA16_HTTP_CLIENT_REQUEST *request);

/**
 ****************************************************************************************
 * @brief Progress HTTP request by User CMD.
 * @param[in]  argc     Arguments count by User CMD
 * @param[in]  argv[]   Enter a string that combines thecommand and options.
 *        \n - "[http-client command] + [Option1] + [Option2] + [Option...] + [URI]".
 *        \n - ex) http-client -i wlan0 -get https://server.com/main.html
 *        \n - Set the "wlan0" or "wlan1" interface with option [-i]. http-client uses "wlan0".
 *        \n - The option [-get] requests the GET method from http-client to http-server.
 *              In addition, "-head", "-put" and "-post" are supported.
 * @return     0(ERR_OK) on success.
 ****************************************************************************************
 */
err_t run_user_http_client(int argc, char *argv[]);

err_t http_client_set_alpn(int argc, char *argv[]);
err_t http_client_set_sni(int argc, char *argv[]);
err_t http_client_set_tls_auth_mode(int tls_auth_mode);

#endif // (__USER_HTTP_CLIENT_H__)

/* EOF */
