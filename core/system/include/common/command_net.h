/**
 ****************************************************************************************
 *
 * @file command_net.h
 *
 * @brief Header file for Network commands
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


#ifndef	__COMMAND_NET_H__
#define	__COMMAND_NET_H__

#include "sdk_type.h"

#include "command.h"


extern void	cmd_debug(int argc, char *argv[]);

#ifdef	__SUPPORT_NSLOOKUP__
extern void	cmd_nslookup(int argc, char *argv[]);
#endif	/* __SUPPORT_NSLOOKUP__ */
#ifdef	__SUPPORT_SNTP_CLIENT__
extern void	cmd_network_sntp(int argc, char *argv[]);
#endif	/* __SUPPORT_SNTP_CLIENT__ */
#ifdef	XIP_CACHE_BOOT
extern void	cmd_time(int argc, char *argv[]);
#endif	// XIP_CACHE_BOOT

extern void	cmd_arp_send(int argc, char *argv[]);

// mDNS CMD
extern void	cmd_mdns_start(int argc, char *argv[]);
extern void	cmd_mdns_stop(int argc, char *argv[]);
extern void	cmd_mdns_change_host(int argc, char *argv[]);
extern void	cmd_mdns_lookup(int argc, char *argv[]);
extern void	cmd_mdns_config(int argc, char *argv[]);
extern void	cmd_zeroconf_state(int argc, char *argv[]);
extern void	cmd_zeroconf_cache(int argc, char *argv[]);
// xmDNS CMD
extern void	cmd_xmdns_start(int argc, char *argv[]);
extern void	cmd_xmdns_stop(int argc, char *argv[]);
extern void	cmd_xmdns_change_host(int argc, char *argv[]);
extern void	cmd_xmdns_lookup(int argc, char *argv[]);
// DNS-SD CMD
extern void	cmd_dns_sd(int argc, char *argv[]);
extern void	cmd_mqtt_client(int argc, char *argv[]);

#ifdef __SUPPORT_SIGMA_TEST__
extern void	cmd_sigma_test(int argc, char *argv[]);
#endif	/* __SUPPORT_SIGMA_TEST__ */

#ifdef	__SUPPORT_IPV6__
extern void	cmd_ipv6_auto_init(int argc, char *argv[]);
#endif	/* __SUPPORT_IPV6__ */

extern void	umac_set_debug( u8 d_on, u16 d_msk, u8 flag);

extern void	cmd_network_ftpc(int argc, char *argv[]);
extern void	cmd_power_down_config(int argc, char *argv[]);


//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

extern const COMMAND_TREE_TYPE	cmd_net_list[] ;
extern int netinit_flag;

//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

extern void cmd_network_config(int argc, char *argv[]);
extern void cmd_netIfaceInit(int argc, char *argv[]);
extern void cmd_ping_client(int argc, char *argv[]);
extern void cmd_nslookup(int argc, char *argv[]);
extern void cmd_network_http_svr(int argc, char *argv[]);
extern void cmd_network_https_svr(int argc, char *argv[]);

#if defined (__SUPPORT_HTTP_CLIENT_FOR_ATCMD__) || defined (__SUPPORT_HTTP_CLIENT_FOR_CLI__)
extern void cmd_network_http_client(int argc, char *argv[]);
#endif /* __SUPPORT_HTTP_CLIENT_FOR_ATCMD__ || __SUPPORT_HTTP_CLIENT_FOR_CLI__ */
extern void cmd_network_http_client_sni(int argc, char *argv[]);
extern void cmd_network_http_client_alpn(int argc, char *argv[]);

#ifdef  __SUPPORT_OTA__
extern void cmd_ota_update(int argc, char *argv[]);
#endif /* __SUPPORT_OTA__ */

extern void cmd_network_dhcpd(int argc, char *argv[]);
extern unsigned int dhcp_server_lease_table(unsigned int);

extern void cmd_dpm(int argc, char *argv[]);
extern void cmd_show_rtm_for_dpm(int argc, char *argv[]);

extern void cmd_show_dpm_sleep_info(int argc, char *argv[]);
extern void cmd_enable_show_dpm_info(int argc, char *argv[]);
extern void cmd_dpm_sleepd(int argc, char *argv[]);

extern void cmd_set_dpm_user_wu_time(int argc, char *argv[]);
extern void cmd_set_dpm_ka_cfg(int argc, char *argv[]);
extern void cmd_set_dpm_tim_wu(int argc, char *argv[]);
extern void cmd_set_dpm_mc_filter(int argc, char *argv[]);
extern void cmd_dpm_udp_tcp_filter_func(int argc, char *argv[]);
extern void cmd_set_dpm_tim_bcn_timeout(int argc, char *argv[]);
extern void cmd_set_dpm_tim_nobcn_step(int argc, char *argv[]);

#ifdef __SUPPORT_CONSOLE_PWD__
extern void cmd_chg_pw(int argc, char *argv[]);
extern void cmd_logout(int argc, char *argv[]);
#endif  /* __SUPPORT_CONSOLE_PWD__ */

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
extern void cmd_set_switch_mode(int argc, char *argv[]);
extern void cmd_mode_switch(int argc, char *argv[]);
extern int factory_reset_btn_onetouch(void);
#endif // __SUPPORT_WIFI_CONCURRENT__

extern unsigned int arp_table(unsigned int iface);
extern unsigned int get_ip_info(int iface_flag, int info_flag, char* result_str);
#ifdef __SUPPORT_SIGMA_TEST__
extern void cmd_sigma_set_profile(int argc, char *argv[]);
extern void cmd_lmac_ps_mode(int argc, char *argv[]);
extern void cmd_lmac_wmm_queue_mode(int argc, char *argv[]);
extern void cmd_lmac_wmm_queue_mode2(int argc, char *argv[]);
#endif	/* __SUPPORT_SIGMA_TEST__ */
extern void cmd_factory_reset(int argc, char *argv[]);

extern int getMACAddrStr(unsigned int separate, char *macstr);
extern int getMacAddrMswLsw(unsigned int iface, ULONG *macmsw, ULONG *maclsw);
#if defined ( __SUPPORT_EVK_LED__ )
extern unsigned int check_factory_button(int btn_gpio_num, int led_gpio_num, int check_time);
extern unsigned int check_wps_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
extern unsigned int check_factory_button(int btn_gpio_num, int check_time);
extern unsigned int check_wps_button(int btn_gpio_num, int check_time);
#endif	// __SUPPORT_EVK_LED__

#ifdef __SUPPORT_NAT__
extern void nat_table_display(void);
#endif /* __SUPPORT_NAT__ */

#ifdef __SUPPORT_MESH__
extern void fc80211_mesh_path_table_display(UCHAR indent);
#endif /* __SUPPORT_MESH__ */

#ifdef __SUPPORT_NAT__
void cmd_nat_table_display(int argc, char *argv[]);
#endif /* __SUPPORT_NAT__ */

#ifdef __SUPPORT_MESH__
void cmd_mesh_path_table_display(int argc, char *argv[]);
#endif /* __SUPPORT_MESH__ */

extern void cmd_defap(int argc, char *argv[]);

/* CMD LIST */
#define	CMD_GETWLANMAC	"getwlanmac"
#define	CMD_SETWLANMAC	"setwlanmac"
#define CMD_SETOTPMAC	"setotpmac"
#define	CMD_MACSPOOFING	"macspoofing"

#endif /*__COMMAND_NET_H__*/

/* EOF */
