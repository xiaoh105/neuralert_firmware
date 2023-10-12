/**
 ****************************************************************************************
 *
 * @file asd.h
 *
 * @brief Application Specific Device
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

#ifdef DPM_PORT
#include "user_dpm.h"
#endif	/*DPM_PORT*/
#include "common_def.h"

/* Supplicant -> App Event Buffer */
#define	WIFI_MONITOR_EV_DATA_SIZE       52
typedef struct wifi_monitor_msg_buffer {
	unsigned int	cmd;
	unsigned int	data_len;
	unsigned long	value;
	char		data[WIFI_MONITOR_EV_DATA_SIZE];
} wifi_monitor_msg_buf_t;

#define	WIFI_EVENT_SEND_MSG_TO_APP		0x01
#define	WIFI_EVENT_SEND_MSG_TO_UART		0x02

#define	WIFI_CMD_P2P_READ_AP_STR		0
#define	WIFI_CMD_P2P_READ_PIN			1
#define	WIFI_CMD_P2P_READ_MAIN_STR		2
#define	WIFI_CMD_P2P_READ_GID_STR		3
#define	WIFI_CMD_SEND_DHCP_RECONFIG		4
#define WIFI_CMD_ASD_BCN_NOTI			5       /* Notification */


/*
 * Global external functions
 */
extern void	asd_init(void);
extern int asd_enable(int ant_type);
extern int	asd_disable(void);
extern int  asd_set_ant_port(int port);
extern int	get_asd_enabled(void);



/* EOF */
