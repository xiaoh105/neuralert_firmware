/**
 ****************************************************************************************
 *
 * @file da16x_network_main.h
 *
 * @brief Define for Network initialize and handle
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


#ifndef	__DA16X_NETWORK_MAIN_H__
#define	__DA16X_NETWORK_MAIN_H__

#include "sdk_type.h"

#include "da16x_system.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_arp.h"

#define	DA16X_PKT_SIZE		640

#if defined ( __SMALL_SYSTEM__ )
#define	DA16X_MAX_PACKETS	54 /* 64 -->54	   7040byte reduce( 704 * 10) */
#else
#define	DA16X_MAX_PACKETS	64
#endif /*...*/

#define IP_PACKET_POOL_SIZE	(DA16X_PKT_SIZE + sizeof(NX_PACKET)) * DA16X_MAX_PACKETS

/* 1K is enouph for IP layer stack and thread stack */
#define DA16X_IP_STACK		((1 * KBYTE) + 384 )	// (1 * KBYTE)
#define DA16X_ARP_STACK		(512) /* (1*KBYTE) */
#define DA16X_BSD_SOCK_STACK	(2 * KBYTE)

//
// Extern functions
//
extern int get_dpm_user_wu_time_from_nvram(void);
extern int wifi_netif_status(int intf);
unsigned int netInit(void);
int get_netmode(int iface);
unsigned int set_netmode(UCHAR iface, UCHAR mode, UCHAR save);
int getSysMode(void);
int setSysMode(int mode);

#ifdef	__TICKTIME__
time_t tick_time(time_t *p);
#endif	/* __TICKTIME__ */
long get_time_zone(void);
unsigned long set_time_zone(long timezone);


long da16x_network_main_get_timezone(void);
unsigned int da16x_network_main_set_timezone(long timezone);
int da16x_network_main_get_netmode(int iface);
unsigned int da16x_network_main_set_netmode(unsigned char iface, unsigned char mode, unsigned char save);
void da16x_network_main_reconfig_net(int state);
int da16x_network_main_get_sysmode(void);
int da16x_network_main_set_sysmode(int mode);
int da16x_get_fast_connection_mode(void);
int da16x_set_fast_connection_mode(int mode);
void da16x_network_main_change_iface_updown(unsigned int iface, unsigned int flag);
void da16x_network_main_check_network(int iface_flag, int simple);
int da16x_network_main_check_net_init(int iface);
unsigned int da16x_network_main_check_ip_addr(unsigned char iface);
unsigned char da16x_network_main_check_dhcp_state(unsigned char iface);
unsigned int da16x_network_main_check_network_ready(unsigned char iface);
int da16x_network_main_check_net_ip_status(int iface);
int da16x_network_main_check_net_link_status(int iface);

#endif	/* __DA16X_NETWORK_MAIN_H__ */

/* EOF */
