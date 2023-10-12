/**
 ****************************************************************************************
 *
 * @file command_net_cmd.c
 *
 * @brief Network related Command table
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


#include "command_cmd.h"
#include "command_net.h"
#include "lwipopts.h"


//-----------------------------------------------------------------------
// Command NET-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_net_list[] = {
  { "NET",				CMD_MY_NODE,	cmd_net_list,	NULL,		"Network "				},	// Head

  { "-------",			CMD_FUNC_NODE,	NULL,	NULL,			"--------------------------------"	},


  { "arp",				CMD_FUNC_NODE,	NULL,	&cmd_arp_table,			"arp [wlan0|wlan1|-a]"											},
  { "arpsend",			CMD_FUNC_NODE,  NULL,   &cmd_arp_send,			"arpsend [wlan0|wlan1] [dst IP Address]"						},
  { "arpresponse",		CMD_FUNC_NODE,  NULL,   &cmd_arp_send,			"arpresponse [wlan0|wlan1] [dst IP Address] [dst MAC Address]"	},
  { "garpsend",			CMD_FUNC_NODE,  NULL,   &cmd_arp_send,			"garpsend [wlan0|wlan1 [option]"								},
  { "dhcp_arp",			CMD_FUNC_NODE,  NULL,   &cmd_arp_send,			"dhcp_arp [wlan0|wlan1] [dst IP_addr]"							},
  { "arping",			CMD_FUNC_NODE,	NULL,	&cmd_arping,			"arping help"													},
  { "       ",			CMD_FUNC_NODE,	NULL,	NULL,			"" },

  { "dhcpd",			CMD_FUNC_NODE,	NULL,	&cmd_network_dhcpd,		"dhcpd help"													},
  { "ifconfig",			CMD_FUNC_NODE,	NULL,	&cmd_network_config,	"ifconfig all (simple)"											},
#if defined ( __USER_DHCP_HOSTNAME__ )
  { "dhcp_hostname",	CMD_FUNC_NODE,  NULL,   &cmd_dhcp_hostname,		"dhcp_hostname [get|set|del] [hostname]"						},
#endif	// __USER_DHCP_HOSTNAME__

#ifdef	__SUPPORT_NSLOOKUP__
#ifdef LWIP_DNS
  { "nslookup",			CMD_FUNC_NODE,	NULL,	&cmd_nslookup,			"nslookup [domain]"												},
#endif	/*LWIP_DNS*/
#endif	/* __SUPPORT_NSLOOKUP__ */
  { "ping",				CMD_FUNC_NODE,	NULL,	&cmd_ping_client,		"ping help"														},

#ifdef	__SUPPORT_SNTP_CLIENT__
  { "sntp",				CMD_FUNC_NODE,	NULL,	&cmd_network_sntp,		"sntp help"														},

#endif	/* __SUPPORT_SNTP_CLIENT__ */

#ifdef __SUPPORT_IPERF__
  { "iperf",			CMD_FUNC_NODE,	NULL,	&cmd_iperf_cli,			"iperf -help"													},
#endif /* __SUPPORT_IPERF__ */

#ifdef __SUPPORT_HTTP_SERVER_FOR_CLI__
  { "http-server",		CMD_FUNC_NODE,	NULL,	&cmd_network_http_svr,	"http-server -I [wlan0|wlan1] [start|stop]"						},
  { "https-server",		CMD_FUNC_NODE,	NULL,	&cmd_network_https_svr,	"https-serer -I [wlan0|wlan1] [start|stop]"						},
#endif /* __SUPPORT_HTTP_SERVER_FOR_CLI__ */

#ifdef __SUPPORT_HTTP_CLIENT_FOR_CLI__
  { "http-client",		CMD_FUNC_NODE,	NULL,	&cmd_network_http_client,"http-client help"												},
#endif /* __SUPPORT_HTTP_CLIENT_FOR_CLI__ */
  { "http-sni",			CMD_FUNC_NODE,	NULL,	&cmd_network_http_client_sni,"http-client SNI"											},
  { "http-alpn",		CMD_FUNC_NODE,	NULL,	&cmd_network_http_client_alpn,"http-client ALPN"										},

#ifdef	__SUPPORT_MDNS_CMDS__	////////////////////////////////////////////
  /*commands to send and receive UDP Data*/
  { "mdns-start",		CMD_FUNC_NODE,	NULL,	&cmd_mdns_start,		"mdns-start [host-name] (host-name should not contain .local)"	},
  { "mdns-stop",		CMD_FUNC_NODE,	NULL,	&cmd_mdns_stop,			"mdns-stop"														},
  { "mdns-lookup",		CMD_FUNC_NODE,	NULL,	&cmd_mdns_lookup,		"mdns-lookup [domains seperated by space]"						},
  { "mdns-change-name",	CMD_FUNC_NODE,	NULL,	&cmd_mdns_change_host,	"mdns-change-name [new host-name] (host-name should not contain .local)"},
#ifdef	__SUPPORT_XMDNS__
  { "xmdns-start", 		CMD_FUNC_NODE,	NULL,	&cmd_xmdns_start,		"xmdns-start [host-name] (host-name should not contain .local)"	},
  { "xmdns-stop", 		CMD_FUNC_NODE,	NULL,	&cmd_xmdns_stop,		"xmdns-stop"													},
  { "xmdns-lookup", 	CMD_FUNC_NODE,	NULL,	&cmd_xmdns_lookup,		"xmdns-lookup [domains seperated by space]"						},
  { "xmdns-change-name",CMD_FUNC_NODE,	NULL,	&cmd_xmdns_change_host,	"xmdns-change-name [new host-name] (host-name should not contain .local)"	},
#endif	/* __SUPPORT_XMDNS__ */

  { "zconf-state",		CMD_FUNC_NODE,	NULL,	&cmd_zeroconf_state,	"zconf-state"													},
  { "zconf-cache",		CMD_FUNC_NODE,	NULL,	&cmd_zeroconf_cache,	"zconf-cache"													},

#ifdef	__SUPPORT_DNS_SD__
  { "dns-sd",			CMD_FUNC_NODE,	NULL,	&cmd_dns_sd,			"dns-sd"														},
#endif	/* __SUPPORT_DNS_SD__ */
  { "mdns-config",		CMD_FUNC_NODE,	NULL,	&cmd_mdns_config,		"mdns-config"													},
#endif	/* __SUPPORT_MDNS_CMDS__ */	////////////////////////////////////////////

#if defined ( CONFIG_DNS2CACHE_TEST )	////////////////////////////////////////////
  { "dns2cache",		CMD_FUNC_NODE,	NULL,	&cmd_dns2cache,			"dns 2nd cache test command"									},
#endif	// CONFIG_DNS2CACHE_TEST

#ifdef	__SUPPORT_MQTT__
  { "mqtt_config",		CMD_FUNC_NODE,  NULL,   &cmd_mqtt_client, 		"MQTT Configuration cmd"										},
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)  
  { "mq_emul",			CMD_FUNC_NODE,  NULL,  	&cmd_mq_msg_tbl_test,	"MQTT emulation command" 										},
  { "mq_tbl",			CMD_FUNC_NODE,  NULL,  	&cmd_mq_msg_tbl_test,	"MQTT preserved msg table admin command" 						},
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
  { "mqtt_client",	CMD_FUNC_NODE,  NULL,   &cmd_mqtt_client, 		"MQTT Client cmd" 													},
#endif	/* __SUPPORT_MQTT__ */

#ifdef  __SUPPORT_OTA__
  { "ota_update",		CMD_FUNC_NODE,	NULL,	&cmd_ota_update,		"ota_update help"												},
#endif /*  __SUPPORT_OTA__ */

#ifdef	__SUPPORT_IPV6__
  { "net6init",			CMD_FUNC_NODE, 	NULL, 	&cmd_ipv6_auto_init,	"IPv6 Initiation"												},
#endif	/* __SUPPORT_IPV6__ */

  { "       ",			CMD_FUNC_NODE,	NULL,	NULL,			"" },
#ifdef __SUPPORT_ASD__
  { "asd",				CMD_FUNC_NODE,	NULL,	&cmd_asd, 				"asd [info|enable|disable|set_ant_p #]"							},
#endif /* __SUPPORT_ASD__ */
  { "cert",				CMD_FUNC_NODE,  NULL,	&cmd_certificate,		"manage Certificate for TLS"									},
  { "cli",				CMD_FUNC_NODE,	NULL,	&cmd_da16x_cli,			"cli help (Same as wpa_cli)"									},
  { "wpa_cli",			CMD_FUNC_NODE,	NULL,	&cmd_da16x_cli,			"wpa_cli help"													},

  { "debug",            CMD_FUNC_NODE,  NULL,   &cmd_debug,             "debug help"                                                    },

  { CMD_MACSPOOFING,	CMD_FUNC_NODE,  NULL,	&cmd_setWLANMac,		"MAC Spoofing for Station"										},

#ifndef	__REDUCE_CODE_SIZE__
  { "chk_rssi",			CMD_FUNC_NODE,	NULL,	&cmd_chk_rssi,	 		"rssi [wlan0|wlan1] [period] [count]"							},
#endif /* !__REDUCE_CODE_SIZE__ */
  { "rssi",				CMD_FUNC_NODE,	NULL,	&cmd_rssi, 				"rssi [wlan0|wlan1]"											},
#ifdef CONFIG_UMAC_MCS_TEST
  { "rate",				CMD_FUNC_NODE,	NULL,	&cmd_umac_rate_test,	"rate [mcs|bg|auto] [0|1]"										},
#endif
  { "getsysmode",		CMD_FUNC_NODE,	NULL,	&cmd_getSysMode,		"Get current Wi-Fi operation mode"								},
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  { "setsysmode",		CMD_FUNC_NODE,	NULL,	&cmd_setSysMode,		"Set System WiFi Mode"											},
  { "set_switch_mode",	CMD_FUNC_NODE,	NULL,	&cmd_set_switch_mode,	"Set Switch sysmode for concurrent-mode"						},
  { "mode_switch",		CMD_FUNC_NODE,	NULL,	&cmd_mode_switch,		"Mode switching for concurrent-mode"							},
#endif // __SUPPORT_WIFI_CONCURRENT__
  { "defap",		    CMD_FUNC_NODE,	NULL,	&cmd_defap,		        "Configure default SoftAP and reboot"							},

  { "       ",			CMD_FUNC_NODE,	NULL,	NULL,			"" },

#ifdef __SUPPORT_SIGMA_TEST__
  { "sigma",			CMD_FUNC_NODE,	NULL,	&cmd_sigma_test,			"sigma [start|stop]"										},
  { "sigma_setprofile",	CMD_FUNC_NODE,	NULL,	&cmd_sigma_set_profile,		"sigma set profile command"									},

  { "ps",				CMD_FUNC_NODE,  NULL,   &cmd_lmac_ps_mode,			"ps [0|1|2]"	 											},
  { "wmm_ps",			CMD_FUNC_NODE,  NULL,   &cmd_lmac_wmm_queue_mode,	"wmm_ps [0|1|2|3|4|5|6]"									},
  { "wmm_ps_mode",		CMD_FUNC_NODE,  NULL,   &cmd_lmac_wmm_queue_mode2,	"wmm_ps_modes [BE] [BK] [VI] [VO]"							},
#endif	/* __SUPPORT_SIGMA_TEST__ */
  { NULL, 	CMD_NULL_NODE,	NULL,	NULL,			NULL 				}	// Tail
};


/* EOF */
