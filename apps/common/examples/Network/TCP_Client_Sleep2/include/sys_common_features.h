/**
 ****************************************************************************************
 *
 * @file sys_common_features.h
 *
 * @brief Define for common system features
 *
 * Copyright (c) 2016-2020 Dialog Semiconductor. All rights reserved.
 *
 * This software ("Software") is owned by Dialog Semiconductor.
 *
 * By using this Software you agree that Dialog Semiconductor retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Dialog Semiconductor is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Dialog Semiconductor products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * DIALOG SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#ifndef	__SYS_COMMON_FEATURES_H__
#define	__SYS_COMMON_FEATURES_H__

#include "project_config.h"

// Wi-Fi configuration features
#include "wifi_common_features.h"

// Console Prompt
#define	PROMPT		"/"CHIPSET_NAME


///////////////////////////////////////////////////////////////////////////
//
// !!! Notice !!!
// HW dependency features
// 	- Do not change these features in SDK
// 		( It is possible to change in Full-Source package )
//
///////////////////////////////////////////////////////////////////////////

// fcCSP type Low-Power RF
#undef	__FOR_FCCSP_SDK__					// For fcCSP LP RF
	#if defined ( __BLE_COMBO_REF__ )
		#undef	__FOR_FCCSP_SDK__
	#endif	//  __BLE_COMBO_REF__

	#if defined ( __FOR_FCCSP_SDK__ )
		#define	__FCCSP_LOW_POWER__

		#if defined ( __FCCSP_LOW_POWER__ )
			#undef	SDK_NAME
			#define	SDK_NAME	"CSP-LP"	// CSP Type Low-Power mode
		#else
			#undef	SDK_NAME
			#define	SDK_NAME	"CSP-NP"	// CSP Type Normal-Power mode
		#endif	// __FCCSP_LOW_POWER__
	#endif	// __FOR_FCCSP_SDK__

// System feature
#if !defined (__SUPPORT_MESH_PORTAL__) && !defined (__SUPPORT_MESH_POINT_ONLY__)
	#define	__TINY_MEM_MODEL__				// STA / Soft-AP	//// #undef for P2P & Concurrent Mode
#endif // !__SUPPORT_MESH_PORTAL__ && !__SUPPORT_MESH_POINT_ONLY__

#define	__ENABLE_UART1_CLOCK__				// Enable UART1 interrupt
#define	__ENABLE_GPIO_CLOCK__				// Enable GPIO interrupt

#define	__CMD_BOOT_IDX__					// Support boot_idx command

#define	__AUTO_REBOOT_WDT__					// Reboot by wdt
#define	__AUTO_REBOOT_EXCEPTION__			// Reboot by exception
#define	__SET_WAKEUP_HW_RESOURCE__			// Init HW Wakeup resources
#define	__USING_REDUNDANCY_HEAP__			// add redundancy heap
#define	__CHECK_CONTINUOUS_FAULT__			// Response to continued fault

// DPM features
#define	__TIM_LOAD_FROM_IMAGE__				// Mandatory feature for V3.0

#define	__SUPPORT_DPM_CMD__					// Support DPM command
#undef	__REMOVE_DPM_SETUP__				// Unsupport DPM setup
#undef	__OPTIMIZE_DPM_SLP_TIME__			// Reduce RPM Wakeup -> Sleep

#undef	__DPM_TIM_WAKEUP_TIME_MSEC__		// DTPM Count : TIM ==> msec.
#undef	__SUPPORT_USER_TIM_UC_WAKEUP_TIME__	// DPM TIM UC Max Timeout
#define	__SUPPORT_DPM_DTIM_DFLT__			// Default TIM count value

#define	__DPM_AUTO_ARP_ENABLE__				// DPM Auto ARP Enable in TIM, it should be enabled
#define	__SUPPORT_DPM_DHCP_CLIENT__

#undef	__DA16X_DPM_MON_CLIENT__			// for Debugging
#undef	__SUPPORT_USER_DPM_RCV_READY_TO__	// DPM recevie ready timeout for user application

// DPM Abnormal mode
#undef	__CHK_DPM_ABNORM_STS__				// DPM Abnormal Monitor commands
#undef	__DPM_FINAL_ABNORMAL__
#undef	__CHECK_BADCONDITION_WAKEUP__		// with timp
#define	__USER_DPM_ABNORM_WU_INTERVAL__
#undef	__WIFI_CONN_RETRY_STOP_AT_WK_CONN_FAIL__	// wifi re-connect stops when wifi conn fail by wrong key
#undef __SUPPORT_DPM_ABN_RTC_TIMER_WU__		// Support RTC Timer running in DPM Abnormal mode

#define	__SUPPORT_DPM_TIM_TCP_ACK__			// DPM TCP Ack for TIM

#define	__SUPPORT_DPM_DYNAMIC_PERIOD_SET__  // Tim wakeup is set automatically by DPM TIM Buffering Check

// Network features
#undef	__SUPPORT_IPV6__					// ipv6 feature


// Wi-Fi Certification features
#undef	__SUPPORT_SIGMA_TEST__				// SIGMA Throughput Test

#define	__REDUCE_CODE_SIZE__				// Unsupport some commands
#undef	__SUPPORT_DBG_SYS_CMD__				// Unused debugging system commands


// Debugging features
#undef	__SUPPORT_CMD_AT__					// Support RF Test AT-CMD

#undef __DNS_2ND_CACHE_SUPPORT__			// DNS 2nd cache. Note: if this feature is enabled, 
											// do not use 4KB from SFLASH_USER_AREA_START for
											// other purpose

#if defined ( __DNS_2ND_CACHE_SUPPORT__ )  // // Debugging for dns2cache test command
	#undef CONFIG_DNS2CACHE_TEST
#endif

//
// !!! Caution !!!
// Built-in Secure NVRAM feature
// Do not change this feature to UNDEF
#define	__SUPPORT_SECURE_NVRAM__		0	// 0: Non-secure, 1: HUK, 2: KCP, 3: KPICV

//////////////////////////////////////////////////////////////////////////////////

///
/// Sub-features for each feature
///


// System running mode

#if defined ( XIP_CACHE_BOOT )		// RTOS
#if defined ( __TINY_MEM_MODEL__ )	// STA, Soft-AP
	#define	__LIGHT_SUPPLICANT__

	#undef	__ENABLE_CMD_OPTION_4__
	#undef	__ENABLE_LMAC_CMD__
	#undef	__ENABLE_LMAC_TX_CMD__
	#undef	__ENABLE_RF_CMD__
	#undef	__ENABLE_PERI_CMD__
	#define	__REDUCE_RW_SIZE_2__
	#define	__REDUCE_RW_SIZE_3__
	#define	__REDUCE_RW_SIZE_4__
#else			// STA, Soft-AP, Wi-Fi Direct, Concurrent Mode
	#undef	__ENABLE_CMD_OPTION_4__
	#undef	__ENABLE_LMAC_CMD__
	#undef	__ENABLE_LMAC_TX_CMD__
	#undef	__ENABLE_RF_CMD__
	#undef	__ENABLE_PERI_CMD__
	// Disable DPM Manager because of memory issue
	#undef	__SUPPORT_DPM_MANAGER__
	#undef	__REDUCE_RW_SIZE_2__
	#undef	__REDUCE_RW_SIZE_3__
	#define	__REDUCE_RW_SIZE_4__
#endif // __TINY_MEM_MODEL__
#endif	// XIP_CACHE_BOOT


// Factory-Reset button
#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )
	#define	__SUPPORT_FACTORY_RST_APMODE__		// Factory reset AP-Mode
	#undef	__SUPPORT_FACTORY_RST_STAMODE__		// Factory reset STA-Mode
#endif // __SUPPORT_FACTORY_RESET_BTN__

// DPM commands on Console
#if defined ( __REMOVE_DPM_SETUP__ )
	#undef	__SUPPORT_DPM_CMD__
#endif // __REMOVE_DPM_SETUP__

// DPM Manager feature
#if defined ( __SUPPORT_DPM_MANAGER__ )
	#if defined (__BLE_COMBO_REF__)
		#undef	__LIGHT_DPM_MANAGER__				// Light DPM Manager : support 2 session & only client
		//#define	__DPM_MNG_SAVE_RTM__				// When DPM-sleep, dpm manager saves data in retention memory 
	#else
		#define	__LIGHT_DPM_MANAGER__				// Light DPM Manager : support 2 session & only client
		#define	__DPM_MNG_SAVE_RTM__				// When DPM-sleep, dpm manager saves data in retention memory 
	#endif	// (__BLE_COMBO_REF__)
#endif // __SUPPORT_DPM_MANAGER__

// Zero-Configuration ...
#if defined ( __SUPPORT_ZERO_CONFIG__ )
	#define	__ENABLE_AUTO_START_ZEROCONF__

	#undef	__SUPPORT_MDNS_CMDS__
	#undef	__SUPPORT_XMDNS__
	#undef	__SUPPORT_DNS_SD__
#endif // __SUPPORT_ZERO_CONFIG__

// HTTP Server features
	#undef	__SUPPORT_HTTP_SERVER_FOR_ATCMD__
	#undef	__SUPPORT_HTTP_SERVER_FOR_CLI__
	#define	__SUPPORT_HTTP_CLIENT_FOR_ATCMD__
	#define	__SUPPORT_HTTP_CLIENT_FOR_CLI__

#if defined ( __SUPPORT_HTTP_SERVER_FOR_ATCMD__ ) || defined (__SUPPORT_HTTP_SERVER_FOR_CLI__)
	#undef	__HTTP_SVR_AUTO_START__
	#undef	__HTTPS_SVR_AUTO_START__

	#undef	__WEB_UI__
#endif // __SUPPORT_HTTP_SERVER__ || __SUPPORT_HTTP_SERVER_FOR_CLI__

// OTA Update features
#if defined ( __SUPPORT_OTA__ )
	#define	__OTA_HTTP_CLIENT__					// HTTP Protocol
	#if defined ( __OTA_HTTP_CLIENT__ )
		#undef __OTA_MEM_USE_RW__				// Use RW area for buffer
	#endif  // __OTA_HTTP_CLIENT__

	#undef	__OTA_UPDATE_MCU_FW__
	#if defined ( __OTA_UPDATE_MCU_FW__ )
		#define __OTA_UPDATE_MCU_FW_UART1__
		#undef __OTA_UPDATE_MCU_FW_UART2__
	#endif //__OTA_UPDATE_MCU_FW__
#endif // __SUPPORT_OTA__

// DHCP Client features on DPM mode
#if defined ( __SUPPORT_DPM_DHCP_CLIENT__ )
	#define	__SUPPORT_DHCP_RETRANS_TIMEOUT_4SEC__	// retry timeout 4sec
	#define	__SUPPORT_DHCP_CLIENT_RENEW_5_OF_6__	// DHCP Lease Time 5/6
	#define	__RECONNECT_NON_ACTION_DHCP_CLIENT__
#endif	// __SUPPORT_DPM_DHCP_CLIENT__

// Wi-Fi Certification features
#if defined ( __SUPPORT_SIGMA_TEST__ )
	#define	__SUPPORT_SIGMA_THROUGHPUT__
	#define	__SUPPORT_SIGMA_UAPSD__

	#undef	__SUPPORT_WPS_BTN__

	#define	__SUPPORT_UART1__
	#undef	__SUPPORT_ATCMD__

	#define	__SUPPORT_IEEE80211W__				// IEEE 802.11W(PMF)
	#undef	__SUPPORT_WPA_ENTERPRISE__			// WPA Enterprise
#endif // __SUPPORT_SIGMA_TEST__

#if defined ( __SUPPORT_ATCMD__ )
	#undef	__SUPPORT_ATCMD_TLS__				// TLS session
	#undef	__SUPPORT_DPM_MANAGER__				// DPM Manager
	#undef	__DPM_TEST_WITHOUT_MCU__			// For DPM Test on EV Board

	#define __TRIGGER_DPM_MCU_WAKEUP__			// MCU Wakeup-triggering when DPM UC wakeup
	#define __SUPPORT_DPM_EXT_WU_MON__			// DPM monitor for External Wakeup
	#undef	__SUPPORT_USER_CMD_HELP__			// User AT-CMD Help
	#define	__SUPPORT_RSSI_CMD__				// AT+WFRSSI
	#undef	__DISABLE_ESC_DATA_ECHO__			// Echo ESC data input ( Data Tx Mode )
	#undef	__DPM_WAKEUP_NOTICE_ADDITIONAL__	// Report DPM Wakeup RTC/ETC
	#define __SUPPORT_CMD_SDK_VER__ 			// Support AT+SDKVER

	#undef	__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

	#define __SUPPORT_NOTIFY_RTC_WAKEUP__
	#ifdef	__SUPPORT_NOTIFY_RTC_WAKEUP__
		#define __SET_WAKEUP_HW_RESOURCE__
	#endif	// __SUPPORT_NOTIFY_RTC_WAKEUP__

	#ifdef	__SUPPORT_DPM_EXT_WU_MON__
		#undef	__SUPPORT_UC_WU_MON__			// DPM monitor for UC Wakeup
	#endif	// __SUPPORT_DPM_EXT_WU_MON__

	#undef	__SUPPORT_HELLO_WORLD__				// User Application for "Hello-World"

	#ifdef __SUPPORT_WIFI_PROVISIONING__
    	#define  __PROVISION_ATCMD__                    // provisioining at cmd
	#endif	//__SUPPORT_WIFI_PROVISIONING__
	
	#undef	__SUPPORT_HTTP_SERVER_FOR_ATCMD__
	#undef	__SUPPORT_HTTP_CLIENT_FOR_ATCMD__

	#undef	__SUPPORT_LMAC_RF_CMD__				// lmac/rf feature commands
	#undef	__SUPPORT_PERI_CMD__				// peri. feature commands

	#if defined ( __DISABLE_ESC_DATA_ECHO__ )
		#define	__ENABLE_ESC_DATA_DUMP__		// Tx Data dump on UART1
	#endif // __DISABLE_ESC_DATA_ECHO__

	#if defined ( __SUPPORT_LMAC_RF_CMD__ )
		#define	__ENABLE_LMAC_CMD__
		#define	__ENABLE_LMAC_TX_CMD__
		#define	__ENABLE_RF_CMD__
	#endif // __SUPPORT_LMAC_RF_CMD__

	#undef __WF_CONN_RETRY_CNT_ABN_DPM__ // atcmd to specify the number of wifi reconn trials at dpm abnormal
	#undef	DEBUG_ATCMD			// debug

#endif	// __SUPPORT_ATCMD__

// Throughput test features : iPerf
#if defined ( __SUPPORT_IPERF__ )
	#undef	__IPERF_BANDWIDTH__					// debug only
	#undef	__SUPPORT_APP_POOL_IPERF__			// APP_POOL

	#undef	__IPERF_PRINT_MIB__					// Print iPerf MIB Info.
	#undef	__LIB_IPERF_PRINT_MIB__				// Print iPerf MIB Info.
#endif // __SUPPORT_IPERF__

#if defined (__WIFI_CONN_RETRY_STOP_AT_WK_CONN_FAIL__)
	#undef	__DPM_ABN_SLEEP1_BY_WK_CONN_FAIL__			// optional: dpm abnormal sleep1 when wifi conn fail by wrong key
#endif // __WIFI_CONN_RETRY_STOP_AT_WK_CONN_FAIL__

// MQTT Client features
#if defined ( __SUPPORT_MQTT__ )
	#undef	__USE_HEAP_FOR_TLS__				// not used in FreeRTOS

	#define	__MQTT_SELF_RECONNECT__
	#define	__MQTT_CONN_STATUS__	

	#undef __MQTT_TLS_OPTIONAL_CONFIG__
	#undef __TEST_SET_SLP_DELAY__ // temporary, delay before set_sleep is invoked
#endif // __SUPPORT_MQTT__

#if defined ( __SUPPORT_BSD_SOCK__ )
	#define	__SUPPORT_DPM_FOR_BSD_TCP__			// DPM handling for TCP
#endif	// __SUPPORT_BSD_SOCK__

#if	defined (__TIM_LOAD_FROM_IMAGE__)
	#undef	__USING_REDUNDANCY_HEAP__			// Conflict with RLIB memory
	#undef	__PRINT_TIM_LOAD_FROM_IMAGE__
#endif  /* __TIM_LOAD_FROM_IMAGE__ */

#endif	// __SYS_COMMON_FEATURES_H__

/* EOF */
