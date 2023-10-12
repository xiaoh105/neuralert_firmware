/**
 ****************************************************************************************
 *
 * @file command_cmd.h
 *
 * @brief Header file for command table
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

#include "command.h"
#include "command_net.h"
#include "rtc.h"


//-----------------------------------------------------------------------
// Global external functions
//-----------------------------------------------------------------------

extern void cmd_setup(int argc, char *argv[]);
extern void cmd_time(int argc, char *argv[]);
extern void cmd_factory_reset(int argc, char *argv[]);
extern void cmd_setWLANMac(int argc, char *argv[]);
extern void cmd_getWLANMac(int argc, char *argv[]);
extern void cmd_ping_client(int argc, char *argv[]);
extern void	cmd_time(int argc, char *argv[]);
extern void	cmd_arp_table(int argc, char *argv[]);
extern void	cmd_arp_send(int argc, char *argv[]);
extern void	cmd_network_config(int argc, char *argv[]);
extern void	cmd_da16x_cli(int argc, char *argv[]);
extern void	cmd_network_dhcpd(int argc, char *argv[]);
extern void	cmd_arping(int argc, char *argv[]);
extern void	cmd_certificate(int argc, char *argv[]);
extern void	cmd_debug(int argc, char *argv[]);
extern void	cmd_ping_client(int argc, char *argv[]);
extern void cmd_set_boot_idx(int argc, char *argv[]);
extern void	cmd_setsleep(int argc, char *argv[]);

extern void cmd_getSysMode(int argc, char *argv[]);

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
extern void cmd_setSysMode(int argc, char *argv[]);
#endif // __SUPPORT_WIFI_CONCURRENT__

#if defined ( __SUPPORT_DBG_SYS_CMD__ )
extern void cmd_comment_func(int argc, char *argv[]);
#endif	// __SUPPORT_DBG_SYS_CMD__

#if defined ( __SUPPORT_NSLOOKUP__ )
extern void	cmd_nslookup(int argc, char *argv[]);
#endif	// __SUPPORT_NSLOOKUP__

#if defined ( __SUPPORT_SNTP_CLIENT__ )
extern void	cmd_network_sntp(int argc, char *argv[]);
#endif	// __SUPPORT_SNTP_CLIENT__

#if defined ( __SUPPORT_BCT__ )
extern void	cmd_bonjour_conformance_test(int argc, char *argv[]);
#endif	// __SUPPORT_BCT__

extern void cmd_iperf_cli(int argc, char *argv[]);

#if defined ( __SUPPORT_MQTT__ )
extern void	cmd_mqtt_client(int argc, char *argv[]);
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
extern void	cmd_mq_msg_tbl_test(int argc, char *argv[]);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#endif	// __SUPPORT_MQTT__

#if defined ( __SUPPORT_IPV6__ )
extern void	cmd_ipv6_auto_init(int argc, char *argv[]);
#endif	// __SUPPORT_IPV6__

#if defined ( __SUPPORT_FTP_CLIENT__ )
extern void	cmd_network_ftpc(int argc, char *argv[]);
#endif // __SUPPORT_FTP_CLIENT__

#if defined ( __SUPPORT_SIGMA_TEST__ )
extern void	cmd_sigma_test(int argc, char *argv[]);
extern void	cmd_lmac_ps_mode(int argc, char *argv[]);
extern void	cmd_lmac_wmm_queue_mode(int argc, char *argv[]);
extern void	cmd_lmac_wmm_queue_mode2(int argc, char *argv[]);
#endif	// __SUPPORT_SIGMA_TEST__

#if defined ( __SUPPORT_ASD__ )
extern void	cmd_asd(int argc, char *argv[]);
#endif // __SUPPORT_ASD__

extern void cmd_debug_ch_tx_level(int argc, char *argv[]);
extern void cmd_txpwr_ctl_same_idx_table(int argc, char *argv[]);
extern void	cmd_rssi(int argc, char *argv[]);
extern void	cmd_chk_rssi(int argc, char *argv[]);

#if defined ( __SUPPORT_MDNS_CMDS__	)
extern void	cmd_mdns_start(int argc, char *argv[]);
extern void	cmd_mdns_stop(int argc, char *argv[]);
extern void	cmd_mdns_change_host(int argc, char *argv[]);
extern void	cmd_mdns_lookup(int argc, char *argv[]);
extern void	cmd_mdns_config(int argc, char *argv[]);
extern void	cmd_zeroconf_state(int argc, char *argv[]);
extern void	cmd_zeroconf_cache(int argc, char *argv[]);
#if defined ( __SUPPORT_XMDNS__ )
extern void	cmd_xmdns_start(int argc, char *argv[]);
extern void	cmd_xmdns_stop(int argc, char *argv[]);
extern void	cmd_xmdns_change_host(int argc, char *argv[]);
extern void	cmd_xmdns_lookup(int argc, char *argv[]);
#endif	// __SUPPORT_XMDNS__
#if defined ( __SUPPORT_DNS_SD__ )
extern void	cmd_dns_sd(int argc, char *argv[]);
#endif	// __SUPPORT_DNS_SD__
#endif	// __SUPPORT_MDNS_CMDS__

#if defined ( __USER_DHCP_HOSTNAME__ )
extern void cmd_dhcp_hostname(int argc, char *argv[]);
#endif	// __USER_DHCP_HOSTNAME__

#if defined ( CONFIG_DNS2CACHE_TEST )
extern void cmd_dns2cache(int argc, char *argv[]);
#endif //CONFIG_DNS2CACHE_TEST

#if defined ( CONFIG_UMAC_MCS_TEST )
extern void	cmd_umac_rate_test(int argc, char *argv[]);
#endif	// CONFIG_UMAC_MCS_TEST


//-----------------------------------------------------------------------
// Global external variables
//-----------------------------------------------------------------------
extern const COMMAND_TREE_TYPE cmd_lmac_list[];

#if defined ( __ENABLE_UMAC_CMD__ )
extern const COMMAND_TREE_TYPE cmd_umac_list[];
#endif	// __ENABLE_UMAC_CMD__

extern const COMMAND_TREE_TYPE cmd_rf_list[];
extern const COMMAND_TREE_TYPE cmd_user_list[];

#if defined (__BLE_FEATURE_ENABLED__)
extern const COMMAND_TREE_TYPE	cmd_ble_list[];
#endif	/* __BLE_FEATURE_ENABLED__ */

//-----------------------------------------------------------------------
// Global functions
//-----------------------------------------------------------------------

/* EOF */
