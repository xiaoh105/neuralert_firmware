/**
 ****************************************************************************************
 *
 * @file command_root_cmd.c
 *
 * @brief Console command for system main
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
#include "command_cmd.h"
#include "common_def.h"
#include "user_dpm_api.h"
#include "command_pwm.h"
#include "command_adc.h"
#include "command_i2s.h"
#include "command_i2c.h"
#include "command_host.h"
#include "rf_meas_api.h"

#ifdef	XIP_CACHE_BOOT  
  #define SUPPORT_ALL_COMMANDS
#else	//XIP_CACHE_BOOT
  #undef  SUPPORT_ALL_COMMANDS
#endif	//XIP_CACHE_BOOT

#ifdef	SUPPORT_ALL_COMMANDS
//-----------------------------------------------------------------------
// Command Top-List
//-----------------------------------------------------------------------
const COMMAND_TREE_TYPE	cmd_root_list[] = {
				{ "ROOT",				CMD_MY_NODE,	cmd_root_list,	NULL,				"ROOT command "		},	// Head
{ "-------",			CMD_FUNC_NODE,	NULL,			NULL,				"--------------------------------"	},
{ "heap",				CMD_FUNC_NODE,	NULL,			&cmd_heapinfo_func, "show heap info command"			},
{ "memory_map",			CMD_FUNC_NODE,	NULL,			&cmd_memory_map_func, "show memory map info command"	},
{ "?",					CMD_FUNC_NODE,	NULL,			&cmd_help_func, 	"help command "						},	// Help
{ "help",				CMD_FUNC_NODE,	NULL,			&cmd_help_func, 	"help command "						},	// Help
{ "",					CMD_FUNC_NODE,	NULL,			NULL,				""									},

{ "top",				CMD_ROOT_NODE,	NULL,			NULL,				"ROOT directory "					},	// Root
{ "up", 				CMD_TOP_NODE,	NULL,			NULL,				"upper directory "					},	// Upper
{ ".",					CMD_FUNC_NODE,	NULL,			NULL, 				"Previous command"					},
{ "",					CMD_FUNC_NODE,	NULL,			NULL,				""									},

{ "boot_idx",			CMD_FUNC_NODE,  NULL,			&cmd_set_boot_idx,	"Set boot index "					},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "echo",				CMD_FUNC_NODE,	NULL,			&cmd_echo_func,		"Echo command"						},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "factory",			CMD_FUNC_NODE,	NULL,			&cmd_factory_reset,	"Factory Reset"						},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "hidden",				CMD_FORCE_NODE,	NULL,			&cmd_hidden_func, 	"display on/off"					},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "lwip_mem",			CMD_FUNC_NODE,	NULL,			&cmd_lwip_mem_func,	"mem information"					},
{ "lwip_memp",			CMD_FUNC_NODE,	NULL,			&cmd_lwip_memp_func,"memp information"					},
{ "lwip_tcp_pcb",		CMD_FUNC_NODE,	NULL,			&cmd_lwip_tcp_pcb_func,"TCP PCB information"			},
{ "lwip_udp_pcb",		CMD_FUNC_NODE,	NULL,			&cmd_lwip_udp_pcb_func,"UDP PCB information"			},
{ "lwip_timer",			CMD_FUNC_NODE,	NULL,			&cmd_lwip_timer_func,"Lwip timer information"			},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "passwd",				CMD_FORCE_NODE,	NULL,			&cmd_passwd_func, 	"password on/off"					},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "ping",				CMD_FUNC_NODE,	NULL,			&cmd_ping_client,	"ping help" 						},
{ "ps",					CMD_FUNC_NODE,	NULL,			&cmd_taskinfo_func,	"Thread information"				},
{ "reboot",				CMD_FUNC_NODE,	NULL,			&cmd_reboot_func, 	"reboot command"					},
{ "reset",				CMD_FUNC_NODE,	NULL,			&cmd_reboot_func, 	"reboot command"					},
{ "setup",				CMD_FUNC_NODE,	NULL,			&cmd_setup,			"Easy Setup"						},
{ "time",				CMD_FUNC_NODE,  NULL,			&cmd_time,			"time  [option]"					},
{ "timer",				CMD_FUNC_NODE,	NULL,			&cmd_timer_status,	"timer information"					},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "trace",				CMD_FUNC_NODE,	NULL,			&cmd_logview_func,	"log view command "					},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "txpwr_l",			CMD_FUNC_NODE,	NULL,			&cmd_debug_ch_tx_level,			"txpwr_l [print]|[erase]|[CH1 CH2 CH3 ... CH13 CH14]"	},
#if !defined ( __REDUCE_CODE_SIZE__ )
{ "txpwr_ctl",			CMD_FUNC_NODE,	NULL,			&cmd_txpwr_ctl_same_idx_table,	"txpwr_ctl [set]|[get] [0|1|2|]: Only for FC9060"   },
#endif	// ! __REDUCE_CODE_SIZE__
{ "ver",				CMD_FUNC_NODE,	NULL,			&version_display, 	"version display "					},
 
#if defined ( __SUPPORT_CONSOLE_PWD__ )
{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "chg_pw",				CMD_FUNC_NODE,	NULL,			&cmd_chg_pw,		"Change password"					},
{ "logout",				CMD_FUNC_NODE,	NULL,			&cmd_logout,		"Logout"							},
#endif	// __SUPPORT_CONSOLE_PWD__

{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ CMD_GETWLANMAC,		CMD_FUNC_NODE,	NULL,			&cmd_getWLANMac,	"Show MAC_addr"						},
{ CMD_SETWLANMAC,		CMD_FUNC_NODE,	NULL,			&cmd_setWLANMac,	"Write MAC_addr"					},
{ CMD_SETOTPMAC,		CMD_FUNC_NODE,	NULL,			&cmd_setWLANMac,	"OTP Write MAC_addr"				},

{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "brd",				CMD_FUNC_NODE,	NULL,			&cmd_mem_read,		"byte read  [addr] [length]"		},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "bwr",				CMD_FUNC_NODE,	NULL,			&cmd_mem_write,		"byte write [addr] [data] [length]"	},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "wrd",				CMD_FUNC_NODE,	NULL,			&cmd_mem_wread,		"word read  [addr] [length]"		},
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "wwr",				CMD_FUNC_NODE,	NULL,			&cmd_mem_wwrite,	"word write [addr] [data] [length]"	},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "lrd",				CMD_FUNC_NODE,	NULL,			&cmd_mem_lread,		"long read  [addr] [length]"		},
{ "lwr",				CMD_FUNC_NODE,  NULL,			&cmd_mem_lwrite,    "long write [addr] [data] [length]"	},

#if defined ( __SUPPORT_DPM_CMD__ )
	  /* for DPM */
{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},

{ "- DPM Commands -",	CMD_FUNC_NODE,	NULL,			NULL,				"--------------------------------"	},
{ "dpm",				CMD_FUNC_NODE,	NULL,			&cmd_dpm,						"DPM enable / disable"					},
{ "dpm_keep_conf",		CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_ka_cfg,			"Set DPM Keepalive"						},
{ "dpm_set_mc", 		CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_mc_filter,			"Set DPM Multicast filter"				},
{ "dpm_set_filter", 	CMD_FUNC_NODE,	NULL,			&cmd_dpm_udp_tcp_filter_func,	"Set DPM TCP/UDP Port Filter "			},
{ "dpm_tim_wu_time",	CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_tim_wu,			"Set DPM TIM Wakeup time [sec]"			},
{ "dpm_user_wu_time",	CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_user_wu_time,		"Set DPM User wakeup time"				},
{ "dpm_bcn_timeout",	CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_tim_bcn_timeout,	"Set DPM TIM Beacon Timeout [sec]"		},
{ "dpm_nobcn_step",		CMD_FUNC_NODE,	NULL,			&cmd_set_dpm_tim_nobcn_step,	"Set DPM TIM NO BCN skip step"			},
#endif	// __SUPPORT_DPM_CMD__
{ "setsleep",			CMD_FUNC_NODE,	NULL,			&cmd_setsleep,					"Activate sleep 2"	},
{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "MonRxTaRssi",		CMD_FUNC_NODE,	NULL,			&cmd_rf_meas_MonRxTaRssi,	"Measureing MonRxTaRssi"	},
{ "MonRxTaCfo",			CMD_FUNC_NODE,	NULL,			&cmd_rf_meas_MonRxTaCfo,	"Measureing MonRxTaCfo"		},

{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "- Sub-Commands -",	CMD_FUNC_NODE,	NULL,			NULL,				"--------------------------------"	},

#if defined (__SUPPORT_ATCMD__) && defined ( __SUPPORT_CMD_AT__ )
{ "at",					CMD_SUB_NODE,   cmd_at_list,	NULL,				"AT commands"						},
#endif // defined (__SUPPORT_ATCMD__) && defined ( __SUPPORT_CMD_AT__ )
#if defined ( __SUPPORT_DBG_SYS_CMD__ )
{ "mem",				CMD_SUB_NODE,	cmd_mem_list,	NULL,				"Memory commands"					},
#endif	// __SUPPORT_DBG_SYS_CMD__
{ "lmac",				CMD_SUB_NODE,	cmd_lmac_list,  NULL,				"Lower-MAC commands"				},

{ "net",				CMD_SUB_NODE,	cmd_net_list,	NULL,				"Network commands"					},
{ "nvram",				CMD_SUB_NODE,	cmd_nvram_list,	NULL,				"NVRAM commands"					},
{ "sys",				CMD_SUB_NODE,	cmd_sys_list,	NULL,				"System commands"					},
#if defined ( __ENABLE_UMAC_CMD__ )
{ "umac",               CMD_SUB_NODE,   cmd_umac_list,  NULL,               "Upper-MAC commands"				},
#endif // __ENABLE_UMAC_CMD__
{ "rf",					CMD_SUB_NODE,	cmd_rf_list,	NULL,				"RF commands"						},

{ "user",				CMD_SUB_NODE,	cmd_user_list,	NULL,				"User commands"						},

#ifdef __ENABLE_PERI_CMD_FOR_TEST__
{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "pwm",				CMD_SUB_NODE,	cmd_pwm_list,	NULL,				"PWM commands"						},
{ "adc",                CMD_SUB_NODE,   cmd_adc_list,   NULL,               "ADC commands"						},
{ "i2s",                CMD_SUB_NODE,   cmd_i2s_list,   NULL,               "I2S commands"						},
{ "i2c",                CMD_SUB_NODE,   cmd_i2c_list,   NULL,               "I2C commands"						},
{ "host",               CMD_SUB_NODE,   cmd_host_list,  NULL,               "HostIF commands"					},
#endif // __ENABLE_PERI_CMD_FOR_TEST__
#if defined (__BLE_FEATURE_ENABLED__)
{ "ble",                CMD_SUB_NODE,	cmd_ble_list,	NULL,               "BLE-combo App commands"			},
#endif	/* __BLE_FEATURE_ENABLED__ */
{ "",	CMD_FUNC_NODE,	NULL,		NULL,				""														},
{ "sys_wdog",           CMD_FUNC_NODE,  NULL,           &cmd_sys_wdog_func, "show sys watchdog info command"    },
{ NULL, 	CMD_NULL_NODE,	NULL,	NULL,		NULL }		// Tail
};

#else // SUPPORT_ALL_COMMANDS	===================================

//-----------------------------------------------------------------------
// Command Top-List
//-----------------------------------------------------------------------
const COMMAND_TREE_TYPE	cmd_root_list[] = {
{ "ROOT",				CMD_MY_NODE,	cmd_root_list,	NULL,				"ROOT command "						},	// Head
{ "-------",			CMD_FUNC_NODE,	NULL,			NULL,				"--------------------------------"	},
{ "?",					CMD_FUNC_NODE,	NULL,			&cmd_help_func, 	"help command "						},	// Help
{ "help",				CMD_FUNC_NODE,	NULL,			&cmd_help_func, 	"help command "						},	// Help
{ "",					CMD_FUNC_NODE,	NULL,			NULL,				""									},

{ "top",				CMD_ROOT_NODE,	NULL,			NULL,				"ROOT directory "					},	// Root
{ "up", 				CMD_TOP_NODE,	NULL,			NULL,				"upper directory "					},	// Upper
{ ".",					CMD_FUNC_NODE,	NULL,			NULL, 				"Previous command"					},
{ "",					CMD_FUNC_NODE,	NULL,			NULL,				""									},

{ NULL, 	CMD_NULL_NODE,	NULL,	NULL,		NULL }		// Tail
};

#endif //SUPPORT_ALL_COMMANDS

/* EOF */
