/**
 ****************************************************************************************
 *
 * @file atcmd_udp_session.h
 *
 * @brief AT-CMD UDP Socket Controller
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

#ifndef __ATCMD_UDP_SESSION_H__
#define __ATCMD_UDP_SESSION_H__

#include "sdk_type.h"

#if defined	(__SUPPORT_ATCMD__)

#include "da16x_network_common.h"

#include "da16x_system.h"
#include "command.h"
#include "common_uart.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"
#include "task.h"
#include "atcmd.h"
#include "user_dpm.h"
#include "user_dpm_api.h"

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
#define ATCMD_UDPS_MAX_TASK_NAME		configMAX_TASK_NAME_LEN
#define ATCMD_UDPS_MAX_SOCK_NAME		configMAX_TASK_NAME_LEN
#define ATCMD_UDPS_RECV_TIMEOUT			100 //ms
#define ATCMD_UDPS_MIN_PORT				0
#define ATCMD_UDPS_MAX_PORT				0xFFFF

#define ATCMD_UDPS_TASK_NAME			"atcus_t"
#define ATCMD_UDPS_SOCK_NAME			"atcus_s"
#define ATCMD_UDPS_TASK_PRIORITY		(OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_UDPS_TASK_SIZE			(1024 * 1)
#define ATCMD_UDPS_RECV_HDR_SIZE		50
#define ATCMD_UDPS_RECV_PAYLOAD_SIZE	2048		
#define ATCMD_UDPS_RECV_BUF_SIZE		(ATCMD_UDPS_RECV_HDR_SIZE + ATCMD_UDPS_RECV_PAYLOAD_SIZE)

#define	ATCMD_UDPS_EVT_ANY				0xFF
#define	ATCMD_UDPS_EVT_ACTIVE			0x01
#define	ATCMD_UDPS_EVT_CLOSED			0x02
#else
#define ATCMD_UDPS_MAX_TASK_NAME		20
#define ATCMD_UDPS_RECV_TIMEOUT			100 //ms
#define ATCMD_UDPS_MIN_PORT				0
#define ATCMD_UDPS_MAX_PORT				0xFFFF

#define ATCMD_UDPS_TASK_NAME			"ATCMD_UDP_T"
#define ATCMD_UDPS_TASK_PRIORITY		(OS_TASK_PRIORITY_USER + 7) //8
#define ATCMD_UDPS_TASK_SIZE			(1024 * 1)
#define ATCMD_UDPS_RECV_HDR_SIZE		50
#define ATCMD_UDPS_RECV_PAYLOAD_SIZE	ATCMD_DATA_BUF_SIZE
#define ATCMD_UDPS_RECV_BUF_SIZE		(ATCMD_UDPS_RECV_HDR_SIZE + ATCMD_UDPS_RECV_PAYLOAD_SIZE)

// UDP session name for DPM
#define	ATCMD_UDPS_DPM_NAME			"atcmd_udp_session"
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

typedef enum _atcmd_udps_state {
	ATCMD_UDPS_STATE_TERMINATED = 0,

	ATCMD_UDPS_STATE_READY = 1,

	ATCMD_UDPS_STATE_ACTIVE = 2,

	ATCMD_UDPS_STATE_REQ_TERMINATE = 3,
} atcmd_udps_state;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
typedef struct _atcmd_udps_sess_info {
	int local_port;
} atcmd_udps_sess_info;

typedef struct _atcmd_udps_config {
	int cid;
	char task_name[ATCMD_UDPS_MAX_TASK_NAME];
	unsigned long task_priority;
	size_t task_size;

	char sock_name[ATCMD_UDPS_MAX_SOCK_NAME];

	size_t rx_buflen;

	struct sockaddr_in local_addr;
	struct sockaddr_in peer_addr;

	atcmd_udps_sess_info *sess_info;
} atcmd_udps_config;

typedef struct _atcmd_udps_context {
	TaskHandle_t task_handler;
	atcmd_udps_state state;
    EventGroupHandle_t event;

	//recv buffer
	unsigned char *buffer;
	size_t buffer_len;

	//socket
	int socket;

	const atcmd_udps_config *conf;
} atcmd_udps_context;
#else
typedef struct _atcmd_udps_config {
	unsigned char task_name[ATCMD_UDPS_MAX_TASK_NAME];
	unsigned long task_priority;
	size_t task_size;

	size_t rx_buflen;

	struct sockaddr_in local_addr;
	struct sockaddr_in peer_addr;
} atcmd_udps_config;

typedef struct _atcmd_udps_context {
	TaskHandle_t task_handler;
	atcmd_udps_state state;

	//recv buffer
	unsigned char *buffer;
	size_t buffer_len;

	//socket
	int socket;

	const atcmd_udps_config *conf;
} atcmd_udps_context;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_udps_init_context(atcmd_udps_context *ctx);

int atcmd_udps_deinit_context(atcmd_udps_context *ctx);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_udps_init_config(const int cid, atcmd_udps_config *conf, atcmd_udps_sess_info *sess_info);
#else
int atcmd_udps_init_config(const int cid, atcmd_udps_config *conf);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_udps_deinit_config(atcmd_udps_config *conf);

int atcmd_udps_set_local_addr(atcmd_udps_config *conf, char *ip, int port);

int atcmd_udps_set_peer_addr(atcmd_udps_config *conf, char *ip, int port);

int atcmd_udps_set_config(atcmd_udps_context *ctx, atcmd_udps_config *conf);

#if defined(__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_udps_wait_for_ready(atcmd_udps_context *ctx);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_udps_start(atcmd_udps_context *ctx);

int atcmd_udps_stop(atcmd_udps_context *ctx);

int atcmd_udps_tx(atcmd_udps_context *ctx, char *data, unsigned int data_len, char *ip, unsigned int port);

#endif // __SUPPORT_ATCMD__

#endif // __ATCMD_UDP_SESSION_H__

/* EOF */
