/**
 ****************************************************************************************
 *
 * @file config_generic_sdk.h
 *
 * @brief Configuration for Generic-SDK
 *
 * Copyright (c) 2016-2021 Dialog Semiconductor. All rights reserved.
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

#ifndef	__CONFIG_GENERIC_SDK_H__
#define	__CONFIG_GENERIC_SDK_H__

///
/// Features for Generic SDK
///

#if defined ( GENERIC_SDK )

	#if defined (__DA16600__)
		#define	__BLE_COMBO_REF__
		#define __FREERTOS__
	#endif //(__DA16600__)

// SDK Name
#undef	SDK_NAME
	#define	SDK_NAME	"GEN"

// SDK Version number
#undef	SDK_MAJOR
#undef	SDK_MINOR
#undef	SDK_REVISION
	#define	SDK_MAJOR		3				// SDK Package Major number
	#define	SDK_MINOR		2				// SDK Package Minor number
	#define	SDK_REVISION	2				// SDK Package Revision number
	#define	SDK_ENG_VER		0				// SDK Engineering release number

///////////////////////////////////////////////////////////////////////
//
// Mendatory features for customer SDK
// !!! Notice !!!
//		Do not remove this features
//
	#define	__FOR_GENERIC_SDK__				// For Generic SDK

///////////////////////////////////////////////////////////////////////

// Factory Reset / WPS Button features
	#undef	__SUPPORT_WPS_BTN__				// WPS button
	#define	__SUPPORT_FACTORY_RESET_BTN__	// Factory reset Button

// Support BOR Circuit
	#define	__SET_BOR_CIRCUIT__				// Brown & Black out interrupt enable

// DPM Sleep Manager
	#define	__SUPPORT_DPM_MANAGER__

// Additional running features
	#undef	__SUPPORT_CONSOLE_PWD__			// Console Security : Password
	#undef	__SUPPORT_BTCOEX__				// Support Wi-Fi & BT co-existence


// Network applications and protocol features
	#define	__SUPPORT_DHCP_SVR__
	#define	__SUPPORT_DHCP_API__
	#define	__SUPPORT_SNTP_CLIENT__			// SNTP Client module
	#undef	__SUPPORT_NSLOOKUP__			// nslookup network utility
	#if defined (__BLE_COMBO_REF__)
		#undef	__SUPPORT_LINK_LOCAL_ADDRESS__		// Link-Local Address module
	#endif	//(__BLE_COMBO_REF__)


// OTA Update - FW grade on HTTP Client Protocol
	#define	__SUPPORT_OTA__					// Support OTA upgrdade


// HTTP Clinet / Server features
	#undef	__SUPPORT_HTTP_SERVER_FOR_ATCMD__	// HTTP Server module
	#undef	__SUPPORT_HTTP_SERVER_FOR_CLI__	// HTTP Server module
	#undef	__SUPPORT_HTTP_CLIENT_FOR_ATCMD__			// HTTP Client module
	#undef	__SUPPORT_HTTP_CLIENT_FOR_CLI__			// HTTP Client module


// Zero-Config
	#undef	__SUPPORT_ZERO_CONFIG__			// Support mDNS, DNS-SD & xmDNS

// IoT Protocols
	#define	__SUPPORT_MQTT__				// Support MQTT


// UART Interface features
	#if defined (__BLE_COMBO_REF__)
		#define	__SUPPORT_UART1__				// Support UART1
	#else
		#undef	__SUPPORT_UART1__				// Support UART1
	#endif //(__BLE_COMBO_REF__)
	#undef	__SUPPORT_UART2__				// Support UART2

// AT-CMD features
	#undef	__SUPPORT_ATCMD__				// Support AT-CMD
		#ifdef __SUPPORT_ATCMD__
			#if defined (__BLE_COMBO_REF__)
				#undef	__ATCMD_IF_UART1__		// AT-CMD over UART1
				#define	__ATCMD_IF_UART2__		// AT-CMD over UART2
			#else
				#define	__ATCMD_IF_UART1__		// AT-CMD over UART1
				#undef	__ATCMD_IF_UART2__		// AT-CMD over UART2
			#endif //(__BLE_COMBO_REF__)

			#undef	__USER_UART_CONFIG__	// Support Customer's UART configuration

			#undef	__ATCMD_IF_SPI__		// AT-CMD over SPI
			#undef	__ATCMD_IF_SDIO__		// AT-CMD over SDIO
			#if defined(__ATCMD_IF_SDIO__) || defined(__ATCMD_IF_SPI__)
				#undef	__SUPPORT_WPS_BTN__ 			// WPS button
				#undef	__SUPPORT_FACTORY_RESET_BTN__	// Factory reset Button
			#endif // defined(__ATCMD_IF_SDIO__) || defined(__ATCMD_IF_SPI__)
		#endif	/* __SUPPORT_ATCMD__ */


// Customer call-back to notify Wi-Fi connection.
	#undef	__SUPPORT_WIFI_CONN_CB__


// Throughput test features : iPerf
	#define	__SUPPORT_IPERF__				// NX iPerf


// User Application for "Hello-World"
	#undef	__SUPPORT_HELLO_WORLD__

// User Application for "Provisioning"
	#ifdef	__BLE_COMBO_REF__
		#undef	__SUPPORT_WIFI_PROVISIONING__	
	#else
		#define	__SUPPORT_WIFI_PROVISIONING__
	#endif	//__BLE_COMBO_REF__

// Other Features ...
	#define	__SUPPORT_BOOT_FW_VER_CHK__     // Version check when boot time
	#define	__SET_WAKEUP_HW_RESOURCE__      // Init HW Wakeup resources

///////////////////////////////////////////////////////////////////////
//
// !!! Notice !!!
//		Have to change cmconfig file before compile.
//
//		~/SDK/boot/SBOOT/cmconfig/da16xtpmconfig.cfg.W25Q32JW(4MB)
//		=>
//		~/SDK/boot/SBOOT/cmconfig/da16xtpmconfig.cfg
//
// SFLASH 4MB memory map
	#define	__FOR_4MB_SFLASH__              // For 4MB SFLASH - Default 4MB


///////////////////////////////////////////////////////////////////////
//
// !!! Notice !!!
//		Don't remove this header-file
//		And do not move the location here ...
//

#include "sys_common_features.h"

///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// BLE Combo Reference ( for DA16600 )
//
// !!! Notice !!!
//		Do not change the location this header-file ...
//
#if defined (__BLE_COMBO_REF__)
	#include "ble_combo_features.h"
#endif	// (__BLE_COMBO_REF__)

#define CFG_USE_RETMEM_WITHOUT_DPM
#define CFG_USE_SYSTEM_CONTROL

// Debugging features
#define	__RUNTIME_CALCULATION__				// Enable log message for Code run-time


#endif	// GENERIC_SDK  -------------------------------------



#endif	// __CONFIG_GENERIC_SDK_H__

/* EOF */
