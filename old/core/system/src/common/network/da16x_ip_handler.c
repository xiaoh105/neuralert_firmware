/**
 ****************************************************************************************
 *
 * @file da16x_ip_handler.c
 *
 * @brief Define for System Running Model
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

#include "sdk_type.h"

#include "sys_feature.h"

#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#include "da16x_ip_handler.h"
#include "da16x_dns_client.h"
#include "da16x_dhcp_client.h"
#include "da16x_dpm.h"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

//extern functions
#if defined ( __SUPPORT_MULTI_IP_IF__ )
extern int get_netinit_state(void);
#endif	// __SUPPORT_MULTI_IP_IF__

extern void dpm_accept_arp_broadcast(unsigned long accept_addr, unsigned long subnet_addr);
extern void sys_app_ip_change_notify(void);
extern void dhcp_not_release_and_stop(struct netif *netif);

int ip_change(UINT iface, char  *ipaddress, char *netmask, char *gateway, UCHAR save)
{
	struct netif *netif = netif_get_by_index(iface + 2);
	char 	tmp_str[10];

	if (netif == NULL || check_net_init(iface) != pdPASS) {
		PRINTF("[%s] Failed to get network interface (%d)\n", __func__, iface);
		return pdFAIL;
	}

	/* check ip, subnet, gateway */
	if (   ip4_addr_netmask_valid(ipaddr_addr(netmask)) == pdFALSE
		|| ipaddr_addr(ipaddress) == IPADDR_NONE
		|| (ipaddr_addr(gateway) == IPADDR_NONE  && strcmp(gateway, "0.0.0.0") != 0)
		|| (isvalidIPsubnetRange(iptolong(gateway), iptolong(ipaddress),
								 iptolong(netmask)) == pdFALSE && strcmp(gateway, "0.0.0.0") != 0))
	{
		return pdFAIL;
	}

	/* Write nvram */
	if (save) {
		memset(tmp_str, 0, 10);
		snprintf(tmp_str, sizeof(tmp_str), "%d:%s",
				 iface == ETH0_IFACE ? 0 : iface, NETMODE);

		write_tmp_nvram_int(tmp_str, STATIC_IP); /* Static IP */

		memset(tmp_str, 0, 10);
		snprintf(tmp_str, sizeof(tmp_str), "%d:%s",
				 iface == ETH0_IFACE ? 0 : iface, STATIC_IP_ADDRESS);

		write_tmp_nvram_string(tmp_str, ipaddress);

		memset(tmp_str, 0, 10);
		snprintf(tmp_str, sizeof(tmp_str), "%d:%s",
				 iface == ETH0_IFACE ? 0 : iface, STATIC_IP_NETMASK);

		write_tmp_nvram_string(tmp_str, netmask);

		memset(tmp_str, 0, 10);
		snprintf(tmp_str, sizeof(tmp_str), "%d:%s",
				 iface == ETH0_IFACE ? 0 : iface, STATIC_IP_GATEWAY);

		write_tmp_nvram_string(tmp_str, gateway);

		save_tmp_nvram();
	}

	/* Check the network initialization */
	if (check_net_init(iface) == pdPASS) {
		/* DHCP Client : STOP */
#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
		extern unsigned char fast_connection_sleep_flag;

		if ((get_netmode(iface) == DHCPCLIENT) && (fast_connection_sleep_flag == pdTRUE)) {
			set_netmode(iface, STATIC_IP, 0);
			dhcp_not_release_and_stop(netif);
		} else
#endif /* __SUPPORT_DHCPC_IP_TO_STATIC_IP__ */
		{
			dhcp_release(netif);
			set_netmode(iface, STATIC_IP, save);
		}

		if (chk_dpm_mode() == 1) {
			user_dpm_supp_ip_info_t *dpm_ip_info = NULL;
			user_dpm_supp_net_info_t *dpm_netinfo = NULL;

			unreg_dpm_sleep(NET_IFCONFIG);
			dpm_netinfo = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();

			if (dpm_netinfo) {
				dpm_netinfo->net_mode = (char)get_netmode(WLAN0_IFACE);
			}

			dpm_ip_info = (user_dpm_supp_ip_info_t *)get_supp_ip_info_ptr();

			if (dpm_ip_info) {
				dpm_ip_info->dpm_dhcp_xid = 0;
				dpm_ip_info->dpm_ip_addr = iptolong(ipaddress);
				dpm_ip_info->dpm_netmask = iptolong(netmask);
				dpm_ip_info->dpm_gateway = iptolong(gateway);
				dpm_ip_info->dpm_dhcp_server_ip = 0;

				dpm_accept_arp_broadcast(dpm_ip_info->dpm_ip_addr, dpm_ip_info->dpm_netmask);
			}
		}

#if defined __SUPPORT_MULTI_IP_IF__
		if (   get_run_mode() < SYSMODE_STA_N_AP
#if defined __SUPPORT_MESH__
			|| get_run_mode() == SYSMODE_MESH_POINT
#endif // __SUPPORT_MESH__
		    )
#endif // __SUPPORT_MULTI_IP_IF__
		{

			ip4_addr_t addr;

			ip4addr_aton(ipaddress, &addr);
			netif_set_ipaddr(netif, &addr);

			ip4addr_aton(netmask, &addr);
			netif_set_netmask(netif, &addr);

			if (strcmp(gateway, "0.0.0.0") == 0) {
				netif_set_gw(netif, IPADDR_ANY);
			} else {
				ip4addr_aton(gateway, &addr);
				if (ip4_addr_get_u32(&addr) != IPADDR_ANY) {
					netif_set_gw(netif, &addr);
				}
			}
		}
#if defined __SUPPORT_MULTI_IP_IF__
		else {
			netif_set_ipaddr(netif, ipaddr_addr(ipaddress));
		}
#endif // __SUPPORT_MULTI_IP_IF__

		sys_app_ip_change_notify();
	}

	return pdPASS;

}

UINT get_ip_info(int iface_flag, int info_flag, char *result_str)
{
	struct netif *netif = netif_get_by_index(iface_flag + 2);

	switch (info_flag) {
		case GET_MACADDR:
			snprintf(result_str, NET_INFO_STR_LEN,
					 "%02X:%02X:%02X:%02X:%02X:%02X",
						netif->hwaddr[0],
						netif->hwaddr[1],
						netif->hwaddr[2],
						netif->hwaddr[3],
						netif->hwaddr[4],
						netif->hwaddr[5]);
			break;

		case GET_IPADDR:
			snprintf(result_str, NET_INFO_STR_LEN, "%s", ipaddr_ntoa(&netif->ip_addr));
			break;

		case GET_SUBNET:
			snprintf(result_str, NET_INFO_STR_LEN, "%s", ipaddr_ntoa(&netif->netmask));
			break;

		case GET_GATEWAY:
			snprintf(result_str, NET_INFO_STR_LEN, "%s", ipaddr_ntoa(&netif->gw));
			break;

		case GET_MTU:
			snprintf(result_str, NET_INFO_STR_LEN, "%d", netif->mtu);
			break;

		case GET_DNS:
			/* DNS Server primary */
			snprintf(result_str, NET_INFO_STR_LEN, "%s", ipaddr_ntoa((ip_addr_t *)dns_getserver(0)));
			break;

		case GET_DNS_2ND:
			/* DNS Server 2ndary */
			snprintf(result_str, NET_INFO_STR_LEN, "%s", ipaddr_ntoa((ip_addr_t *)dns_getserver(1)));
			break;

		case GET_IPV6:
#ifdef	__SUPPORT_IPV6__
			PRINTF("=== [%s] Need to write FreeRTOS code for GET_IPV6 ...\n", __func__);
#endif	/* __SUPPORT_IPV6__ */

			break;
	}

	return pdPASS;
}


/* EOF */
