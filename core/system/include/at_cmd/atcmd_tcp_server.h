/**
 ****************************************************************************************
 *
 * @file atcmd_tcp_server.h
 *
 * @brief AT-CMD TCP Server Socket Controller
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

#ifndef __ATCMD_TCP_SERVER_H__
#define __ATCMD_TCP_SERVER_H__

#include "sdk_type.h"

#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )

#define ATCMD_TCPS_MAX_SESS             (ATCMD_NW_TR_MAX_SESSION_CNT_DPM - 1)

#define ATCMD_TCPS_MAX_TASK_NAME        configMAX_TASK_NAME_LEN
#define ATCMD_TCPS_MAX_SOCK_NAME        configMAX_TASK_NAME_LEN
#define ATCMD_TCPS_CLI_TASK_NAME        "atcts_tc"
#define ATCMD_TCPS_BACKLOG              4
#define ATCMD_TCPS_RECV_TIMEOUT         100     //ms
#define ATCMD_TCPS_SEND_TIMEOUT         5000    //ms
#define ATCMD_TCPS_MIN_PORT             1
#define ATCMD_TCPS_MAX_PORT             0xFFFF
#define ATCMD_TCPS_INIT_SOCKET_FD       -1

#define ATCMD_TCPS_TASK_NAME            "atcts_t"
#define ATCMD_TCPS_SOCK_NAME            "atcts_s"

#define ATCMD_TCPS_TASK_PRIORITY        (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPS_TASK_SIZE            (1024 * 2)

#define ATCMD_TCPS_CLI_TASK_PRIORITY    (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPS_CLI_TASK_SIZE        (512 * 2)
#define ATCMD_TCPS_RECV_HDR_SIZE        50
#define ATCMD_TCPS_RECV_PAYLOAD_SIZE    2048
#define ATCMD_TCPS_RECV_BUF_SIZE        (ATCMD_TCPS_RECV_HDR_SIZE + ATCMD_TCPS_RECV_PAYLOAD_SIZE)

#define	ATCMD_TCPS_EVT_ANY              0xFF
#define	ATCMD_TCPS_EVT_ACCEPT           0x01
#define	ATCMD_TCPS_EVT_CLOSED           0x02

#else /////////////////////////////////

#define ATCMD_TCPS_MAX_SESS             (DPM_SOCK_MAX_TCP_SESS - 1)
#define ATCMD_TCPS_MAX_TASK_NAME        20
#define ATCMD_TCPS_CLI_TASK_NAME        "atcmd_tcps_"
#define ATCMD_TCPS_BACKLOG              4
#define ATCMD_TCPS_RECV_TIMEOUT         100 //msec
#define ATCMD_TCPS_SEND_TIMEOUT         5000	//ms
#define ATCMD_TCPS_MIN_PORT             1
#define ATCMD_TCPS_MAX_PORT             0xFFFF
#define ATCMD_TCPS_INIT_SOCKET_FD       -1

#define ATCMD_TCPS_TASK_NAME            "ATCMD_TCPS_T"
#define ATCMD_TCPS_SOCK_NAME            "atcmd_tcps_sock"

#define ATCMD_TCPS_TASK_PRIORITY        (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPS_TASK_SIZE            (1024 * 1)

#define ATCMD_TCPS_CLI_TASK_PRIORITY    (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPS_CLI_TASK_SIZE        (512 * 2)
#define ATCMD_TCPS_RECV_HDR_SIZE        50
#define ATCMD_TCPS_RECV_PAYLOAD_SIZE    ATCMD_DATA_BUF_SIZE
#define ATCMD_TCPS_RECV_BUF_SIZE        (ATCMD_TCPS_RECV_HDR_SIZE + ATCMD_TCPS_RECV_PAYLOAD_SIZE)

// TCP Server session name for DPM
#define	ATCMD_TCPS_DPM_NAME             "atcmd_tcp_svr"

#endif // __SUPPORT_ATCMD_MULTI_SESSION__

typedef enum _atcmd_tcps_cli_state {
    ATCMD_TCPS_CLI_STATE_TERMINATED    = 0,
    ATCMD_TCPS_CLI_STATE_READY         = 1,
    ATCMD_TCPS_CLI_STATE_CONNECTED     = 2,
    ATCMD_TCPS_CLI_STATE_DISCONNECTED  = 3,
    ATCMD_TCPS_CLI_STATE_REQ_TERMINATE = 4,
} atcmd_tcps_cli_state;

typedef enum _atcmd_tcps_state {
    ATCMD_TCPS_STATE_TERMINATED        = 0,
    ATCMD_TCPS_STATE_READY             = 1,
    ATCMD_TCPS_STATE_ACCEPT            = 2,
    ATCMD_TCPS_STATE_REQ_TERMINATE     = 3,
} atcmd_tcps_state;

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
typedef struct _atcmd_tcps_cli_context {
    int cid;

    TaskHandle_t task_handler;
    char task_name[ATCMD_TCPS_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;
    EventGroupHandle_t event;

    atcmd_tcps_cli_state state;

    // Recv buffer
    unsigned char *buffer;
    size_t buffer_len;

    int socket;
    struct sockaddr_in addr;

    void *svr_ptr;
    struct _atcmd_tcps_cli_context *next;
} atcmd_tcps_cli_context;

// It's to recover session
typedef struct _atcmd_tcps_sess_info {
    int local_port;
    int max_allow_client;
    struct sockaddr_in cli_addr[ATCMD_TCPS_MAX_SESS];
} atcmd_tcps_sess_info;

typedef struct _atcmd_tcps_config {
    int cid;

    char task_name[ATCMD_TCPS_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;

    char sock_name[ATCMD_TCPS_MAX_SOCK_NAME];

    size_t rx_buflen;

    struct sockaddr_in local_addr;

    atcmd_tcps_sess_info *sess_info;
} atcmd_tcps_config;

typedef struct _atcmd_tcps_context {
    TaskHandle_t task_handler;
    atcmd_tcps_state state;
    EventGroupHandle_t event;

    //listen socket
    int socket;

    const atcmd_tcps_config *conf;

    //client
    int cli_cnt;
    atcmd_tcps_cli_context *cli_ctx;
    SemaphoreHandle_t mutex;
} atcmd_tcps_context;

#else

typedef struct _atcmd_tcps_cli_context {
    TaskHandle_t task_handler;
    unsigned char task_name[ATCMD_TCPS_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;

    atcmd_tcps_cli_state state;

    //recv buffer
    unsigned char *buffer;
    size_t buffer_len;

    int socket;
    struct sockaddr_in addr;

    void *svr_ptr;
    struct _atcmd_tcps_cli_context *next;
} atcmd_tcps_cli_context;

typedef struct _atcmd_tcps_config {
    unsigned char task_name[ATCMD_TCPS_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;

    size_t rx_buflen;

    int max_allow_client;
    struct sockaddr_in local_addr;
} atcmd_tcps_config;

typedef struct _atcmd_tcps_context {
    TaskHandle_t task_handler;
    atcmd_tcps_state state;

    //listen socket
    int socket;

    const atcmd_tcps_config *conf;

    //client
    int cli_cnt;
    atcmd_tcps_cli_context *cli_ctx;
    SemaphoreHandle_t mutex;
} atcmd_tcps_context;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcps_init_context(atcmd_tcps_context *ctx);
int atcmd_tcps_deinit_context(atcmd_tcps_context *ctx);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcps_init_config(int cid, atcmd_tcps_config *conf, atcmd_tcps_sess_info *sess_info);
#else
int atcmd_tcps_init_config(int cid, atcmd_tcps_config *conf);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcps_deinit_config(atcmd_tcps_config *conf);
int atcmd_tcps_set_local_addr(atcmd_tcps_config *conf, char *ip, int port);
int atcmd_tcps_set_max_allowed_client(atcmd_tcps_config *conf, int max_allowed_client);
int atcmd_tcps_set_config(atcmd_tcps_context *ctx, atcmd_tcps_config *conf);
int atcmd_tcps_start(atcmd_tcps_context *ctx);
int atcmd_tcps_stop(atcmd_tcps_context *ctx);
int atcmd_tcps_stop_cli(atcmd_tcps_context *ctx, const char *ip, const int port);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcps_wait_for_ready(atcmd_tcps_context *ctx);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcps_tx(atcmd_tcps_context *ctx, char *data, unsigned int data_len, char *ip, unsigned int port);

#endif // __SUPPORT_ATCMD__

#endif // __ATCMD_TCP_SERVER_H__

/* EOF */
