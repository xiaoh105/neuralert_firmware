/**
 *******************************************************************************
 *
 * @file ble_combo_features.h
 *
 * @brief Define for common system features
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


#ifndef	__BLE_COMBO_FEATURES__
#define __BLE_COMBO_FEATURES__

#ifdef __BLE_COMBO_REF__

/* Additional features for DA16600 (Wi-Fi + BLE) Module */

#define __SET_WAKEUP_HW_RESOURCE__			// Init HW Wakeup resources

/* Factory-Reset button */
#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )
	#if defined(__ATCMD_IF_SPI__)
		#undef __SUPPORT_FACTORY_RESET_BTN__
	#else
	#ifndef __SUPPORT_WIFI_PROVISIONING__
		#undef	__SUPPORT_FACTORY_RST_APMODE__		// Factory reset AP-Mode
		#define __SUPPORT_FACTORY_RST_STAMODE__ 	// Factory reset STA-Mode
	#endif // !__SUPPORT_WIFI_PROVISIONING__
	#endif
#endif /* __SUPPORT_FACTORY_RESET_BTN__ */

/* WPS button */
#if defined(__SUPPORT_WPS_BTN__) && defined(__ATCMD_IF_SPI__)
	#undef __SUPPORT_WPS_BTN__
#endif

 /*
 * #define __BLE_IMG_SIZE__	0x10000 // 64K (2 IMGs), can be up to 80K
 *	 
 * The PRODUCT_HEADER_OFFSET(now 0xFF20 = __BLE_IMG_SIZE__ - 0xE0) must be same as the 
 * 	header_position in create_img.bat at:
 *	(.\projects\target_apps\ble_examples\prox_reporter_sensor_ext_coex\Keil_5) or
 *	(.\projects\target_apps\ble_examples\prox_monitor_aux_ext_coex\Keil_5)
 *	when build the DA14531 SDK
 */
#define __BLE_IMG_SIZE__	0x10000

/* Specific features for Wi-Fi/BLE (DA16600) Combo Module*/

#define __GTL_DBG_PRINT__					// Enable debug print
#undef	__GTL_PKT_DUMP__					// Enable GTL raw data print
#define __USE_APP_MALLOC__					// Use APP_MALLOC for Q mgmt

#define __GTL_IFC_UART__					// BLE interface - UART
#define __GTL_UART_115200__

#undef	__SUPPORT_BTCOEX_1PIN__
#if defined(__ATCMD_IF_SPI__)
	#define __SUPPORT_BTCOEX_1PIN__ 		// BT CoEX with 1Wire for AT CMD over SPI
#endif

#define __ENABLE_RTC_WAKE_UP2_INT__

#define __ENABLE_BLE_WKUP_BEFORE_SEND_MSG__ //wake up toggling before send ble msg

#define __DA14531_BOOT_FROM_UART__			// BLE FW Uart Transfer during boot
 
#if defined (__DA14531_BOOT_FROM_UART__)
/*
 *	BLE FW image is built using the tool in [BLE_SDK_ROOT]\utilities\mkimage
 *
 *	BLE FW image in multi-part format (production):
 *			ble_multi_part_header_t + // multi-part header
 *			ble_img_hdr_t			+ // bank1: fw_1 header
 *			ble fw1 bin 			+ //		fw_1 bin
 *			ble_img_hdr_t			+ // bank2: fw_2 header
 *			ble fw2 bin 			+ //		fw_2 bin
 *			ble_product_header_t	  // product header: bank1/bank2 offset
 *
 *	BLE FW in single format (stored in OTA Server):
 *			ble_multi_part_header_t +
 *			ble_img_hdr_t			+
 *			ble fw bin (new)			
 */
#ifdef __SUPPORT_OTA__
#define __BLE_FW_VER_CHK_IN_OTA__	// Support BLE FW Version check in OTA / Boot
#endif
#endif /* __DA14531_BOOT_FROM_UART__ */

/* ================================================
 * BLE: Peripheral Role(Slave)
 * WiFi Service Sample.
 * ================================================
 */
#if defined ( __BLE_PERI_WIFI_SVC__ )

	/* This is to enable Wi-Fi SVC Security */
	#undef	__WIFI_SVC_SECURITY__
	#if defined (__WIFI_SVC_SECURITY__)
		#define __MULTI_BONDING_SUPPORT__
	#endif

	/* Wi-Fi Provisioning GATT Service */
	#define __WFSVC_ENABLE__
	#if defined(__WFSVC_ENABLE__)
		#define __SUPPORT_BLE_PROVISIONING__
	#endif
	/* IOT Sensor example */
	#define __USER_SENSOR__
	#if defined(__USER_SENSOR__)
		#define __IOT_SENSOR_SVC_ENABLE__
	#endif

	/* BLE Connection Paramter Update Request by Peripheral */
	#define __CONN_PARAM_UPDATE__

	/*
	 *	For Example 3: Gas Leak Detection Sensor
	 *	- In the BLE firmware, it should enable: 
	 *		(extended) sleep mode
	 *		IOT_SENSOR_LOW_POWER_MODE
	 *		WIFI_WAKE_UP_VIA_UART_CTS
	 */
	#define __LOW_POWER_IOT_SENSOR__
	#if defined (__LOW_POWER_IOT_SENSOR__)
		#define __WAKE_UP_BY_BLE__
	#endif

	#define __ENABLE_DPM_FOR_GTL_BLE_APP__

	#define __APP_SLEEP2_MONITOR__

	/* INTERNAL ONLY,  DO NOT  change the defines below */
	#undef	__TMP_DSPS_TEST__
	#if defined ( __TMP_DSPS_TEST__ )
		// rebuild system and iperf lib
		#define __SUPPORT_IPERF__
	#endif	// __TMP_DSPS_TEST__
		
	// test command ble.wifi_force_excep
	#undef __EXCEPTION_RST_EMUL__

	/*
	 *	For this test, the following feature flags should be enabled on DA14531
	 *	__FORCE_RESET_ON_EXCEPTION__
	 *	__TEST_FORCE_EXCEPTION_ON_BLE__
	 */
	#undef	__TEST_FORCE_EXCEPTION_ON_BLE__
	#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
		#define __GTL_UART_115200__
	#endif

	#undef __TEST_FORCE_SLEEP2_WITH_TIMER__ 

	#define __OTA_TEST_CMD__

#endif	/* __BLE_PERI_WIFI_SVC__ */

/* ================================================
 * BLE: Peripheral Role(Slave).
 * BLE chip peripheral samples.
 * ================================================
 */
#if defined ( __BLE_PERI_WIFI_SVC_PERIPHERAL__ )

	// To enable Wi-Fi SVC Security
	#undef	__WIFI_SVC_SECURITY__
	#if defined (__WIFI_SVC_SECURITY__)
		#define __MULTI_BONDING_SUPPORT__
	#endif

	/* Wi-Fi Provisioning GATT Service */
	#define __WFSVC_ENABLE__
	#if defined(__WFSVC_ENABLE__)
		#define __SUPPORT_BLE_PROVISIONING__
	#endif
	/* BLE Connection Paramter Update Request by Peripheral */
	#define __CONN_PARAM_UPDATE__

	#define __WAKE_UP_BY_BLE__

	/* enable DPM for GTL BLE application */
	#define __ENABLE_DPM_FOR_GTL_BLE_APP__

	#undef __BLE_PEER_INACTIVITY_CHK_TIMER__

	/* INTERNAL ONLY,  DO NOT  change the defines below */
	#undef	__TMP_DSPS_TEST__
	#if defined ( __TMP_DSPS_TEST__ )
		// rebuild system and iperf lib
		#define __SUPPORT_IPERF__
	#endif	// __TMP_DSPS_TEST__
		
	/* test command ble.wifi_force_excep */
	#undef __EXCEPTION_RST_EMUL__

	/*
	 *	For this test, the following feature flags should be enabled on DA14531
	 *	__FORCE_RESET_ON_EXCEPTION__
	 *	__TEST_FORCE_EXCEPTION_ON_BLE__
	 */
	#undef	__TEST_FORCE_EXCEPTION_ON_BLE__
	#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
		#define __GTL_UART_115200__
	#endif

	#undef __TEST_FORCE_SLEEP2_WITH_TIMER__ 	

	#undef __FOR_DPM_SLEEP_CURRENT_TEST__

	#define __OTA_TEST_CMD__

	#define __SUPPORT_DA14531_GPIO_CONTROL__
#endif	/* __BLE_PERI_WIFI_SVC_PERIPHERAL__ */

/* ================================================
 * BLE: Peripheral Role(Slave).
 * TCP DPM sample.
 * ================================================
 */
#if defined ( __BLE_PERI_WIFI_SVC_TCP_DPM__ )

    #define __SUPPORT_DPM_MANAGER__
    #if defined (__SUPPORT_DPM_MANAGER__)
        #define __LIGHT_DPM_MANAGER__          // Support 1 session & only client.
        #define __DPM_MNG_SAVE_RTM__ 
    #endif // __SUPPORT_DPM_MANAGER__

	// To enable Wi-Fi SVC Security
	#undef	__WIFI_SVC_SECURITY__
	#if defined (__WIFI_SVC_SECURITY__)
		#define __MULTI_BONDING_SUPPORT__
	#endif

	/* Wi-Fi Provisioning GATT Service */
	#define __WFSVC_ENABLE__
	#if defined(__WFSVC_ENABLE__)
		#define __SUPPORT_BLE_PROVISIONING__
	#endif
	/* BLE Connection Paramter Update Request by Peripheral */
	#define __CONN_PARAM_UPDATE__

	#define __WAKE_UP_BY_BLE__

	/* Enable DPM for GTL BLE application */
	#define __ENABLE_DPM_FOR_GTL_BLE_APP__

	/* INTERNAL ONLY,  DO NOT  change the defines below */
	#undef	__TMP_DSPS_TEST__
	#if defined ( __TMP_DSPS_TEST__ )
		// rebuild system and iperf lib
		#define __SUPPORT_IPERF__
	#endif	// __TMP_DSPS_TEST__
	
	/* test command ble.wifi_force_excep */
	#undef __EXCEPTION_RST_EMUL__

	/*
	 *	For this test, the following feature flags should be enabled on DA14531
	 *	__FORCE_RESET_ON_EXCEPTION__
	 *	__TEST_FORCE_EXCEPTION_ON_BLE__
	 */
	#undef	__TEST_FORCE_EXCEPTION_ON_BLE__
	#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
		#define __GTL_UART_115200__
	#endif

	#undef __TEST_FORCE_SLEEP2_WITH_TIMER__ 	

	#undef __FOR_DPM_SLEEP_CURRENT_TEST__

	#define __OTA_TEST_CMD__

#endif	/* __BLE_PERI_WIFI_SVC_TCP_DPM__ */

/* ================================================
 * BLE: Central Role(Master).
 * Sensor Gateway sample.
 * ================================================
 */
#if defined ( __BLE_CENT_SENSOR_GW__ )

	// Wi-Fi Provisioning GATT Service
	#define __WFSVC_ENABLE__
	#if defined(__WFSVC_ENABLE__)
		#define __SUPPORT_BLE_PROVISIONING__
	#endif
	// test command ble.wifi_force_excep
	#undef __EXCEPTION_RST_EMUL__

#endif	// __BLE_CENT_SENSOR_GW__

/* ===============================================*/

#if defined (__BLE_PERI_WIFI_SVC__) || \
	defined (__BLE_CENT_SENSOR_GW__) || \
	defined (__BLE_PERI_WIFI_SVC_TCP_DPM__) || \
	defined (__BLE_PERI_WIFI_SVC_PERIPHERAL__)
	#define __BLE_FEATURE_ENABLED__
#endif

#if defined (__SUPPORT_ATCMD__) && defined (__SUPPORT_BLE_PROVISIONING__)
    	#define  __PROVISION_ATCMD__                    // provisioining at cmd
#endif	//__SUPPORT_ATCMD__ || __SUPPORT_WIFI_PROVISIONING__

#endif	// __BLE_COMBO_REF__

#endif	// __BLE_COMBO_FEATURES__

/* EOF */
