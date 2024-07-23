/**
 ****************************************************************************************
 *
 * @file da16x_network_common.h
 *
 * @brief Common defines, structures and variable declaration
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


#ifndef	__DA16X_NETWORK_COMMON_H__
#define	__DA16X_NETWORK_COMMON_H__

#include "sdk_type.h"

#ifdef IP_STACK_PORT
#include "nx_api.h"
#endif	/*IP_STACK_PORT*/

#include "da16x_system.h"
#include "common_def.h"


/* Application Names ... */
#define	NET_IFCONFIG	"ifconfig"
#define	NET_SNTP	"sntp_client"

#ifdef	ETH_ALEN
#define	DA16X_ETH_ALEN	ETH_ALEN
#else
#define	DA16X_ETH_ALEN	6
#endif

//extern functions
extern long iptolong(char *ip);
extern int get_run_mode(void);

#if defined ( __SUPPORT_DHCPC_IP_TO_STATIC_IP__ )
extern int da16x_get_temp_staticip_mode(int iface_flag);
extern int do_autoarp_check(void);
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

//Common APIs
void da16x_network_common_set_discovery_ip_state(UINT state);
UINT da16x_network_common_get_discovery_ip_state();
void da16x_network_common_set_gateway_ip_addr(ULONG ip_addr);
ULONG da16x_network_common_get_gateway_ip_addr();

int check_net_init(int iface);
int check_net_ip_status(int iface);
UINT chk_ipaddress(UCHAR iface);
UINT chk_network_ready(UCHAR iface);
void da16x_net_check(int iface_flag, int simple);

int get_netmode(int iface);
UINT set_netmode(UCHAR iface, UCHAR mode, UCHAR save);
int getSysMode(void);
int setSysMode(int mode);

void iface_updown(UINT iface, UINT flag);
void reconfig_net(int state);

#ifdef	__TICKTIME__
time_t tick_time(time_t *p);
#endif	/* __TICKTIME__ */

long get_time_zone(void);
unsigned long set_time_zone(long timezone);

/* For Customer's Soft-AP configuration */
#define	MAX_SSID_LEN		32
#define	MAX_PASSKEY_LEN		64
#define	MAX_IP_ADDR_LEN		16

#define	AP_OPEN_MODE		0
#define	AP_SECURITY_MODE	1

#define	IPADDR_DEFAULT		0
#define	IPADDR_CUSTOMER		1

#define	DHCPD_DEFAULT		0
#define	DHCPD_CUSTOMER		1


typedef struct _softap_config
{
	int		customer_cfg_flag;		// MODE_ENABLE, MODE_DISABLE

	char	ssid_name[MAX_SSID_LEN+1];
	char	psk[MAX_PASSKEY_LEN+1];
	char	auth_type;				// AP_OPEN_MODE, AP_SECURITY_MODE
	char	country_code[4];

	int		customer_ip_address;	// IPADDR_DEFAULT, IPADDR_CUSTOMER
	char	ip_addr[MAX_IP_ADDR_LEN];
	char	subnet_mask[MAX_IP_ADDR_LEN];
	char	default_gw[MAX_IP_ADDR_LEN];
	char	dns_ip_addr[MAX_IP_ADDR_LEN];

	int		customer_dhcpd_flag;	// DHCPD_DEFAULT, DHCPD_CUSTOMER
	int		dhcpd_lease_time;
	char	dhcpd_start_ip[MAX_IP_ADDR_LEN];
	char	dhcpd_end_ip[MAX_IP_ADDR_LEN];
	char	dhcpd_dns_ip_addr[MAX_IP_ADDR_LEN];
} softap_config_t;

enum {
	NO_INIT,
	WLAN0_INIT,
	WLAN1_INIT,
	CONCURRENT_INIT
};
#endif	/* __DA16X_NETWORK_COMMON_H__ */

/* EOF */
