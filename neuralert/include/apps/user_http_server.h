/**
 ****************************************************************************************
 *
 * @file user_http_server.h
 *
 * @brief HTTP Server feature.
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

#ifndef __USER_HTTP_SERVER_H__
#define __USER_HTTP_SERVER_H__

#include "sdk_type.h"

#include "FreeRTOSConfig.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp_tcp.h"
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/debug.h"

/// Status of HTTP Client
typedef enum {
    HTTP_SERVER_STATUS_READY,
    HTTP_SERVER_STATUS_RUNNING,
    HTTP_SERVER_STATUS_STOPPING,
} DA16X_HTTP_SERVER_STATUS;

/// Operation code of HTTP Client
typedef enum {
    /// Init value
    HTTP_SERVER_OPCODE_READY,
    /// Start HTTP server
    HTTP_SERVER_OPCODE_START,
    /// Stop HTTP server
    HTTP_SERVER_OPCODE_STOP,
    /// Display status of HTTP server
    HTTP_SERVER_OPCODE_STATUS,
    /// Display help menu for User CMD
    HTTP_SERVER_OPCODE_HELP,
    /// Input CA certificate of HTTPs server
    HTTP_SERVER_OPCODE_CA_CERT,
    /// Input Private key of HTTPs server
    HTTP_SERVER_OPCODE_PRIVATE_KEY,
    /// Input certificate of HTTPs server
    HTTP_SERVER_OPCODE_CERT,
    /// INput DH params of HTTPs server
    HTTP_SERVER_OPCODE_DH_PARAMETER,
    /// Release certificate information of HTTPs server
    HTTP_SERVER_OPCODE_RELEASE,
} DA16_HTTP_SERVER_OPCODE;

typedef struct http_server_cmd {
    /// Operation code
    DA16_HTTP_SERVER_OPCODE    op_code;
    /// Interface
    int iface;
    /// Port number
    int port;
    /// Secure mode
    int is_https_server;
} DA16X_HTTP_SERVER_CMD;


err_t run_user_http_server(char *argv[], int argc);
err_t run_user_https_server(char *argv[], int argc);

#if defined ( __HTTP_SVR_AUTO_START__ )
void auto_run_http_svr(void *pvParameters);
#endif // __HTTP_SVR_AUTO_START__

#if defined ( __HTTPS_SVR_AUTO_START__ )
void auto_run_https_svr(void *pvParameters);
#endif // __HTTPS_SVR_AUTO_START__

#endif // (__USER_HTTP_SERVER_H__)

/* EOF */
