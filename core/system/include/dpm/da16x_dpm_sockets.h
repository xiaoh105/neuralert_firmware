/**
 ****************************************************************************************
 *
 * @file da16x_dpm_sockets.h
 *
 * @brief TCP socket inforamtion for DPM
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

#ifndef DA16X_DPM_SOCKETS_H
#define DA16X_DPM_SOCKETS_H

#include "FreeRTOS.h"
#include "lwip/opt.h"
#include "lwip/tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPM_SOCK_MAX_TCP_SESS           8
#define DPM_SOCK_MAX_TCP_WAIT_CNT       1400

typedef struct _dpm_tcp_sess_info {
	unsigned int used;
	unsigned int wait_cnt;
	struct tcp_pcb pcb;
} dpm_tcp_sess_info;

void da16x_dpm_socket_init(void);
void da16x_dpm_socket_clear_all_tcp_sess_info(void);

void *da16x_dpm_sock_get_lwip_sock_names(void);

struct tcp_pcb *da16x_dpm_sock_create_tcp_pcb(char *name);
struct tcp_pcb *da16x_dpm_sock_get_tcp_pcb(char *name);
int da16x_dpm_sock_delete_tcp_pcb(char *name);

int da16x_dpm_sock_is_available_set_sleep(void);
int da16x_dpm_sock_is_available_connected_tcp_sess(void);
int da16x_dpm_sock_is_established(char *name);
int da16x_dpm_sock_is_remaining_rx_data(int fd);
#ifdef __cplusplus
}
#endif

#endif // DA16X_DPM_SOCKETS_H

/* EOF */
