/**
 ****************************************************************************************
 *
 * @file atcmd_tcp_client.h
 *
 * @brief AT-CMD TCP Client Socket Controller
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

#ifndef __ATCMD_TCP_CLIENT_H__
#define __ATCMD_TCP_CLIENT_H__

#include "sdk_type.h"

#if defined ( __SUPPORT_ATCMD__ )
#include "atcmd.h"

typedef enum _atcmd_tcpc_state {
    ATCMD_TCPC_STATE_TERMINATED    = 0,
    ATCMD_TCPC_STATE_DISCONNECTED  = 1,
    ATCMD_TCPC_STATE_CONNECTED     = 2,
    ATCMD_TCPC_STATE_REQ_TERMINATE = 3,
} atcmd_tcpc_state;

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )

#define ATCMD_TCPC_MAX_TASK_NAME        configMAX_TASK_NAME_LEN
#define ATCMD_TCPC_MAX_SOCK_NAME        configMAX_TASK_NAME_LEN
#define ATCMD_TCPC_RECV_TIMEOUT         100     //ms
#define ATCMD_TCPC_SEND_TIMEOUT         5000    //ms
#define ATCMD_TCPC_RECONN_COUNT         3

#define ATCMD_TCPC_DEF_PORT             30000
#define ATCMD_TCPC_MIN_PORT             0
#define ATCMD_TCPC_MAX_PORT             0xFFFF

#define ATCMD_TCPC_TASK_NAME            "atctc_t"
#define ATCMD_TCPC_SOCK_NAME            "atctc_s"
#define ATCMD_TCPC_TASK_PRIORITY        (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPC_TASK_SIZE            (1024 * 2)
#define ATCMD_TCPC_RECV_HDR_SIZE        50
#define ATCMD_TCPC_RECV_PAYLOAD_SIZE    2048
#define ATCMD_TCPC_RECV_BUF_SIZE        (ATCMD_TCPC_RECV_HDR_SIZE + ATCMD_TCPC_RECV_PAYLOAD_SIZE)

#define ATCMD_TCPC_EVT_ANY              0xFF
#define ATCMD_TCPC_EVT_CONNECTION       0x01
#define ATCMD_TCPC_EVT_CLOSED           0x02

typedef struct _atcmd_tcpc_sess_info {
    int  local_port;
    int  peer_port;
    char peer_ipaddr[ATCMD_NVR_NW_TR_PEER_IPADDR_LEN];
} atcmd_tcpc_sess_info;

typedef struct _atcmd_tcpc_config {
    int  cid;
    char task_name[ATCMD_TCPC_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;

    char sock_name[ATCMD_TCPC_MAX_SOCK_NAME];

    size_t rx_buflen;

    struct sockaddr_in local_addr;
    struct sockaddr_in svr_addr;

    atcmd_tcpc_sess_info *sess_info;
} atcmd_tcpc_config;

typedef struct _atcmd_tcpc_context {
    TaskHandle_t task_handler;
    atcmd_tcpc_state state;
    EventGroupHandle_t event;

    // Receive buffer
    unsigned char *buffer;
    size_t buffer_len;

    // Socket
    int socket;

    const atcmd_tcpc_config *conf;
} atcmd_tcpc_context;

#else	//////////////////////////////////////////

#define ATCMD_TCPC_MAX_TASK_NAME        20
#define ATCMD_TCPC_RECV_TIMEOUT         100     //ms
#define ATCMD_TCPC_SEND_TIMEOUT         5000    //ms

#define ATCMD_TCPC_DEF_PORT             30000
#define ATCMD_TCPC_MIN_PORT             0
#define ATCMD_TCPC_MAX_PORT             0xFFFF

#define ATCMD_TCPC_TASK_NAME            "ATCMD_TCPC_T"
#define ATCMD_TCPC_TASK_PRIORITY        (OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_TCPC_TASK_SIZE            (1024 * 1)
#define ATCMD_TCPC_RECV_HDR_SIZE        50
#define ATCMD_TCPC_RECV_PAYLOAD_SIZE    ATCMD_DATA_BUF_SIZE
#define ATCMD_TCPC_RECV_BUF_SIZE        (ATCMD_TCPC_RECV_HDR_SIZE + ATCMD_TCPC_RECV_PAYLOAD_SIZE)

// TCP Client session name for DPM
#define	ATCMD_TCPC_DPM_NAME             "atcmd_tcp_client"

typedef struct _atcmd_tcpc_config {
    unsigned char task_name[ATCMD_TCPC_MAX_TASK_NAME];
    unsigned long task_priority;
    size_t task_size;

    size_t rx_buflen;

    struct sockaddr_in local_addr;
    struct sockaddr_in svr_addr;
} atcmd_tcpc_config;

typedef struct _atcmd_tcpc_context {
    TaskHandle_t task_handler;
    atcmd_tcpc_state state;

    //recv buffer
    unsigned char *buffer;
    size_t buffer_len;

    //socket
    int socket;

    const atcmd_tcpc_config *conf;
} atcmd_tcpc_context;

#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcpc_init_context(atcmd_tcpc_context *ctx);
int atcmd_tcpc_deinit_context(atcmd_tcpc_context *ctx);

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
int atcmd_tcpc_init_config(const int cid, atcmd_tcpc_config *conf, atcmd_tcpc_sess_info *sess_info);
#else
int atcmd_tcpc_init_config(const int cid, atcmd_tcpc_config *conf);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcpc_deinit_config(atcmd_tcpc_config *conf);
int atcmd_tcpc_set_local_addr(atcmd_tcpc_config *conf, char *ip, int port);
int atcmd_tcpc_set_svr_addr(atcmd_tcpc_config *conf, char *ip, int port);
int atcmd_tcpc_set_config(atcmd_tcpc_context *ctx, atcmd_tcpc_config *conf);

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
int atcmd_tcpc_wait_for_ready(atcmd_tcpc_context *ctx);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcpc_start(atcmd_tcpc_context *ctx);
int atcmd_tcpc_stop(atcmd_tcpc_context *ctx);
int atcmd_tcpc_tx(atcmd_tcpc_context *ctx, char *data, unsigned int data_len);
int atcmd_tcpc_tx_with_peer_info(atcmd_tcpc_context *ctx, char *ip, int port, char *data, unsigned int data_len);


/*
 * Global extern functions
 */
extern unsigned short get_random_value_ushort_range(int lower, int upper);

#endif // __SUPPORT_ATCMD__

#endif // __ATCMD_TCP_CLIENT_H__

/* EOF */
