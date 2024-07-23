/*
 * File Name : supp_def.h
 *     defines for Dialog WPA Supplicant
 *
 * Copyright (c) 2014-2018, Dialog, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 */

#ifndef __SUPP_DEF_H__
#define __SUPP_DEF_H__

#include "sdk_type.h"        // For Wi-Fi configuration features

/* Start - Package Version **************************************************/
#define VER_NUM                    "2.7"
#define RELEASE_VER_STR         " - 2022_03"
#define DEVELOPE_VER_STR        " - Develop"
#define RELEASE_VER          // Enable release version
/* End - Package Version ****************************************************/



/* Start - Supported Wi-Fi Mode (default: define all) ***********************/
#define CONFIG_STA
#define CONFIG_AP
#define CONFIG_WPS           // Wi-Fi Protected Setup (WPS) (Main)

#ifdef __LIGHT_SUPPLICANT__

  #undef  CONFIG_P2P
  #undef  CONFIG_CONCURRENT

#else /* __LIGHT_SUPPLICANT__ */

  #ifdef __SUPPORT_WIFI_CONCURRENT_CORE__
    #define CONFIG_CONCURRENT
  #else
    #undef  CONFIG_CONCURRENT
  #endif /* __SUPPORT_WIFI_CONCURRENT_CORE__ */

  #ifdef CONFIG_CONCURRENT
    #define CONFIG_AP

    #ifdef __SUPPORT_P2P__
        #define CONFIG_P2P
    #else
        #undef  CONFIG_P2P
    #endif /* __SUPPORT_P2P__ */
  #endif /* CONFIG_CONCURRENT */

  #ifdef __SUPPORT_MESH__
    #ifdef __SUPPORT_MESH_PORTAL__
      #define CONFIG_MESH_PORTAL
    #else
      #undef  CONFIG_MESH_PORTAL
    #endif /* __SUPPORT_MESH_PORTAL__ */

    #define CONFIG_MESH
    #undef  CONFIG_P2P
    #undef  CONFIG_WPS
  #else
    #undef  CONFIG_MESH
  #endif /* __SUPPORT_MESH__ */
#endif    /* __LIGHT_SUPPLICANT__ */

/* End - Supported Wi-Fi Mode ***********************************************/

/****************************************************************************/
/* Start - Common features for STA, AP, P2P *********************************/
/****************************************************************************/

/* Start - Memory Optimize - - - - - - - - - - - - - - - - - - - - - - - -  */
#define CONFIG_IMMEDIATE_SCAN
#define CONFIG_DISALLOW_CONCURRENT_SCAN

/* Reduce memory for storage for scan result */
#define CONFIG_SCAN_RESULT_OPTIMIZE
#define UPDATE_REQUIRED_SSID_ACTIVATED_IN_SCAN_RESULTS
#define PROBE_REQ_WITH_SSID_FOR_ASSOC

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
  #define CONFIG_SCAN_REPLY_OPTIMIZE
  #define CONFIG_TOGGLE_SCAN_SORT_TYPE
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */


/* Remove ap_scan related codes (ap_scan = 1 by default) */
#define FEATURE_USE_DEFAULT_AP_SCAN

/* End - Memory Optimize - - - - - - - - - - - - - - - - - - - - - - - - -  */

#define FEATURE_SCAN_FREQ_ORDER_TOGGLE  // toggle scan freq order by each scan

/* Start - messages for printf, dump, dbg - - - - - - - - - - - - - - - - - */
#define CONFIG_LOG_MASK                 // for en/disable debug msg for each module

#ifndef CONFIG_LOG_MASK
  #define ENABLE_NOTICE_DBG             // da16x notice debug print
  #define ENABLE_ERROR_DBG
  #define ENABLE_WARN_DBG
  #define ENABLE_FATAL_DBG
  #define ENABLE_DEBUG_DBG
#endif /* CONFIG_LOG_MASK */

#undef  ENABLE_WPA_STATE_DBG

/* For "info" log masking */
#undef  ENABLE_SCAN_DBG                 // da16x scan debug msg print
#undef  ENABLE_ASSOC_DBG                // da16x assoc/auth debug msg print
#undef  ENABLE_EVENT_DBG                // for event debug print
#undef  ENABLE_P2P_DBG                  // for p2p sequence debug msg print


#undef  DA16X_ASSERT_DEBUG

#undef  TX_FUNC_INDICATE_DBG            // for tx function start/end
#undef  RX_FUNC_INDICATE_DBG            // for rx function start/end

#undef  ENABLE_WPA_DBG                  // for wpa debug msg print
#undef  ENABLE_WPS_DBG                  // for wps debug msg print
#undef  ENABLE_STATE_CHG_DBG            // for state change display
#undef  ENABLE_SM_ENTRY_DBG             // for state machine entry debug
#undef  ENABLE_ELOOP_DBG                // for eloop debug msg print
#undef  ENABLE_DRV_DBG                  // for drv debug msg print
#undef  ENABLE_EAPOL_DBG                // for eapol debug msg print
#undef  ENABLE_IFACE_DBG                // for interface debug msg print
#undef  ENABLE_EAP_DBG                  // for wps/eap debug msg print
#undef  ENABLE_AP_DBG                   // for ap sequence debug msg print
#undef  ENABLE_AP_MGMT_DBG              // for ap sequence debug msg print
#undef  ENABLE_AP_WMM_DBG               // for ap wmm debug msg print
#undef  ENABLE_NVRAM_DBG                // for nvram debugging
#undef  ENABLE_CLI_DBG                  // da16x cli debug msg print
#undef  ENABLE_DPM_DBG                  // for DPM debugging
#undef  ENABLE_80211N_DBG               // for WiFi 802.11n debugging
#undef  ENABLE_WNM_DBG                  // for WNM debugging
#undef  ENABLE_CRYPTO_DBG

#undef  ENABLE_WPA_DUMP_DBG             // for wpa debug msg dump
#undef  ENABLE_BUF_DUMP_DBG             // for dump buffer

/* End - messages for printf, dump, dbg - - - - - - - - - - - - - - - - - - */

/* Start - WPS - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
#ifdef CONFIG_WPS
  #define CONFIG_WPS_PIN                // WPS Pin code
  #define CONFIG_WPS_REGISTRAR
  #undef  CONFIG_WPS_AP
  #undef  CONFIG_WPS_STRICT
#endif    /* CONFIG_WPS */

/* End - WPS - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/* Start - Configuration for EAPOL, WPA/WPA2 Authentication - - - - - - - - */
#define CONFIG_EAPOL
#define IEEE8021X_EAPOL
#undef  OPTIMIZE_PBKDF2                 // For optimized_pbkdfs_sha1, use h/w engine
#define CONFIG_PMKSA                    // Pairwise Master Key Security Association
/* End - Configuration for EAPOL, WPA/WPA2 Authentication - - - - - - - - - */

/* Start - EAP - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
#define CONFIG_EAP_METHOD
#define CONFIG_EAP_PEER                 // RFC 4137 EAP Peer State Machine
#define EAP_WSC
/* End - EAP - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/* Start - MIC failure - - - - - - - - - - - - - - - - - - - - - - - - - -  */
#define CONFIG_MIC_FAILURE_RSP          // Support MIC Faulure response
#define CONFIG_DELAYED_MIC_ERROR_REPORT // Support MIC Faulure response
#undef  CONFIG_SIMULATED_MIC_FAILURE    // MIC failure sim. after connection
/* End - MIC failure - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/* Start - WNM Power Save - - - - - - - - - - - - - - - - - - - - - - - - - */
#define CONFIG_WNM
#define CONFIG_WNM_BSS_MAX_IDLE_PERIOD
#undef  CONFIG_WNM_BSS_MAX_IDLE_PERIOD_TEST

#ifdef CONFIG_WNM_BSS_MAX_IDLE_PERIOD_TEST
  #define ENABLE_WNM_DBG
#endif    /* CONFIG_WNM_BSS_MAX_IDLE_PERIOD_TEST */

#undef  CONFIG_WNM_ACTIONS
#undef  CONFIG_WNM_SLEEP_MODE
#undef  CONFIG_WNM_TFS
#undef  CONFIG_WNM_BSS_TRANS_MGMT
#undef  CONFIG_WNM_SSID_LIST
#undef  CONFIG_WNM_NOTIFICATION
/* End  - WNM Power Save - - - - - - - - - - - - - - - - - - - - - - - - -  */

/* Start - WPA3 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#if defined __SUPPORT_WPA3_PERSONAL_CORE__
  #define CONFIG_SAE

  #define CONFIG_OWE
  #define CONFIG_OWE_AP
  #define CONFIG_OWE_BEACON
  #undef  TEST_OWE_GROUP_FIXED

  #undef  CONFIG_DPP

  #ifdef __SUPPORT_OWE_TRANS__        // For MESH
    #define CONFIG_OWE_TRANS
  #endif // __SUPPORT_OWE_TRANS__
#else
  #undef  CONFIG_OWE
  #undef  CONFIG_OWE_AP
  #undef  CONFIG_OWE_BEACON
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
/* End - WPA3 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define SUPPLICANT_PLAIN_TEXT_SSID

#define WPA2_PATCH_201701

#undef  CONFIG_5G_SUPPORT
#define SUPPORT_SELECT_NETWORK_FILTER
#ifdef CONFIG_5G_SUPPORT
  #ifndef SUPPORT_SELECT_NETWORK_FILTER
    #define SUPPORT_SELECT_NETWORK_FILTER
  #endif
#endif

/****************************************************************************/
/* End - Common features for STA, AP, P2P ***********************************/
/****************************************************************************/

/* Start - STA Features *****************************************************/
#ifdef CONFIG_STA

#define CONFIG_ALLOW_ANY_OPEN_AP        // Allow connection to any open mode AP
#define CONFIG_SIMPLE_ROAMING
#define FIXED_ISSUES_LOCAL_DEAUTH       // ignore_next_local_deauth clear
#define CONFIG_STA_POWER_SAVE
#define CONFIG_DPM_OPT_WIFI_MODE

#define DEF_SAVE_DPM_WIFI_MODE          // save/restore wifi connection mode in dpm reconnection

#ifdef __SUPPORT_WPA_ENTERPRISE_CORE__
#define CONFIG_ENTERPRISE               // Need CONFIG_TLS
  #ifdef __SUPPORT_WPA3_ENTERPRISE_192B_CORE__
    // WPA3-Enterprise
    #define CONFIG_SUITEB
    #define CONFIG_SUITEB192
    #define CONFIG_SHA384
  #endif /* __SUPPORT_WPA3_ENTERPRISE_192B_CORE__ */
#else
  #undef  CONFIG_ENTERPRISE             // Need CONFIG_TLS
  // WPA3-Enterprise
  #undef  CONFIG_SUITEB
  #undef  CONFIG_SUITEB192
  #undef  CONFIG_SHA384
#endif    /* __SUPPORT_WPA_ENTERPRISE_CORE__ */

/* Start - EAP Methods for WPA/WPA2-Enterprise - - - - - - - - - - - - - -  */
#ifdef CONFIG_ENTERPRISE
  #undef  EAP_MD5
  #define EAP_TLS                       // Mandatory in da16x
  #define EAP_PEAP                      // Mandatory in da16x
  #define EAP_TTLS                      // Mandatory in da16x
  #define EAP_FAST                      // Mandatory in da16x
  #define EAP_MSCHAPv2                  // Mandatory in da16x
  #define EAP_GTC                       // Mandatory in da16x
#endif    /* CONFIG_ENTERPRISE */
/* End - EAP Methods for WPA/WPA2-Enterprise - - - - - - - - - - - - - - -  */

/* Start - Auto Scan features - - - - - - - - - - - - - - - - - - - - - - - */
#undef  CONFIG_AUTOSCAN
#undef  CONFIG_AUTOSCAN_PERIODIC
#undef  CONFIG_AUTOSCAN_EXPONENTIAL
/* End - Auto Scan features - - - - - - - - - - - - - - - - - - - - - - - - */

#define CONFIG_RECONNECT_OPTIMIZE       // fast reconnect

/* Fast Connection in Sleep mode 1,2 */
/* IN SLEEP MODE 1 & FAST CONNECTION */
#define FAST_CONN_ASSOC_CH

#ifdef __SUPPORT_BSSID_SCAN__           // Scan with BSSID
  #define CONFIG_SCAN_WITH_BSSID
#else
  #undef  CONFIG_SCAN_WITH_BSSID
#endif

#ifdef __SUPPORT_DIR_SSID_SCAN__        // Scan with SSID
  #define CONFIG_SCAN_WITH_DIR_SSID
#else
  #undef  CONFIG_SCAN_WITH_DIR_SSID
#endif

#define CONFIG_FAST_CONNECTION 
#define CONFIG_STA_COUNTRY_CODE         // Set country code, modify scan parameter
#undef  CONFIG_STA_BSSID_FILTER

#define CONFIG_RECEIVE_DHCP_EVENT

#undef  AUTO_WEPKEY_INDEX               // Automatic selection of wep key index

#ifdef CONFIG_IMMEDIATE_SCAN
#undef  CONFIG_FAST_RECONNECT
#undef  CONFIG_FAST_RECONNECT_V2
#endif /* CONFIG_IMMEDIATE_SCAN */

#define CONFIG_AUTH_MAX_FAILURES       // Max number of auth
#endif /* CONFIG_STA */

/* End - STA Features *******************************************************/

/* Start - AP Features ******************************************************/
#ifdef CONFIG_AP

#define NEED_AP_MLME
#define CONFIG_AP_HW_FEATURE
#define CONFIG_8021X                    // for ieee 802.1x authentication
#define CONFIG_NO_HOSTAPD_LOGGER

#define CONFIG_AP_WMM                   // for Soft-AP WMM

#define CONFIG_EAP_SERVER

#ifdef CONFIG_WPS
  #define EAP_SERVER_WSC                // for WPS
  #define EAP_SERVER_IDENTITY           // for WPS
  #define CONFIG_AP_PLAIN_TEXT_SEC      // for WPS
#endif    /* CONFIG_WPS */

#define CONFIG_AP_MANAGE_CLIENT         // cli commands for managing client

#ifdef __SCAN_ON_AP_MODE__
  #define ENABLE_SCAN_ON_AP_MODE        // Enable scan in AP mode
#else
  #undef  ENABLE_SCAN_ON_AP_MODE        // Disable scan in AP mode
#endif    /* __SCAN_ON_AP_MODE__ */

#define CONFIG_AP_ISOLATION             // Isolation
#define CONFIG_ACL                      // ACL (Access Control List)
#define CONFIG_AP_NONE_STA              // Check - No Station left
#define CONFIG_AP_POWER                 // Set Power
#define CONFIG_IEEE80211N               // Enable IEEE 802.11n Support for AP
#define CONFIG_AP_HT                    // AP High Throughput
#define CONFIG_HT_OVERRIDES             // 802.11n enable/disable
#define CONFIG_AP_WIFI_MODE             // Wi-Fi mode set : bgn/bg/n/g/b only
#define CONFIG_ACS                      // Enable Automatic Channel Selection
#define CONFIG_AP_PARAMETERS
#define CONFIG_AP_REASSOC_OPTIMIZE      // Prevent memory free/realloc on reassociation

#undef  CONFIG_AP_TEST_SKIP_TX_STATUS
#undef  CONFIG_IEEE80211D               // ieee 802.11d geographical regulations
#undef  CONFIG_AP_WDS                   // for AP WDS
#undef  CONFIG_AP_DFS                   // for Dynamic Channel Selection : 5GHz
#undef  CONFIG_AP_SECURITY_WEP          // for AP security : WEP
#undef  CONFIG_AP_VLAN                  // for AP VLAN function
#define CONFIG_NO_VLAN

#undef  CONFIG_FULL_DYNAMIC_VLAN
#undef  CONFIG_RADIUS                   // for RADIUS
#define CONFIG_NO_RADIUS                // for RADIUS
#undef  RADIUS_SERVER
#undef  CONFIG_IEEE80211H               // ieee 802.11h DFS
#undef  CONFIG_IEEE80211F               // ieee 802.11f IAPP
#undef  CONFIG_IEEE80211R               // ieee 802.11r Fast Secure Roaming
#undef  CONFIG_IEEE80211R_AP            // ieee 802.11r Fast Secure Roaming


#endif    /* CONFIG_AP */
/* End - AP Features ********************************************************/


/* Start - P2P (Wi-Fi Direct) ***********************************************/
#ifdef CONFIG_P2P

#define CONFIG_P2P_POWER_SAVE           // p2p Power Save (OpPS & NoA)
#undef  CONFIG_P2P_OPTION               // p2p option func. (invite, SD)
#undef  ENABLE_EXTRA_CONF
#undef  CONFIG_GAS                      // Generic advertisement service (GAS) query
#undef  CONFIG_BRIDGE_IFACE
#undef  CONFIG_WIFI_DISPLAY
#undef  CONFIG_ACL_P2P                  // P2P GO ACL (Access Control List)

#endif    /* CONFIG_P2P */
/* End - P2P (Wi-Fi Direct) *************************************************/


/* Start - Not supported, might be used in future ***************************/

#undef  CONFIG_MACSEC                   //  MACsec secure session
#undef  CONFIG_NO_PBKDF2

#undef  CONFIG_IEEE80211AC              // IEEE802.11AC
#undef  CONFIG_IEEE80211R               // Fast BSS Transition;FT (FT-PSK)
#undef  CONFIG_INTERWORKING
#undef  INTERWORKING_3GPP
#undef  CONFIG_ROAMING_PARTNER
#undef  CONFIG_BGSCAN                   // background scan and roaming interface
#undef  CONFIG_BGSCAN_LEARN             // bg scan and roaming module: learn
#undef  CONFIG_EXCLUDE_SSID
#undef  CONFIG_PRIO_GROUP               // To disable priority list
#undef  CONFIG_PRE_AUTH                 // Preauth. for IEEE 802.11r
#undef  CONFIG_NOTIFY
#undef  CONFIG_RRM
#define CONFIG_NO_CONFIG_BLOBS
#undef  CONFIG_BAND_5GHZ
#undef  CONFIG_BAND
#undef  CONFIG_AP_SUPPORT_5GHZ
#undef  CONFIG_IEEE80211AC_WMM
#undef  CONFIG_WMM_ACTIONS
#define CONFIG_LAST_SEQ_CTRL


#if defined(CONFIG_STA) || defined(CONFIG_AP)
  #define CONFIG_SCAN_UMAC_HEAP_ALLOC
#endif // defined(CONFIG_STA) || defined(CONFIG_AP)

#if defined(CONFIG_P2P)
  #undef  CONFIG_SCAN_UMAC_HEAP_ALLOC
#endif // defined(CONFIG_P2P)

#ifdef CONFIG_SCAN_UMAC_HEAP_ALLOC
  #define CONFIG_REUSED_UMAC_BSS_LIST
#else
  #undef  CONFIG_REUSED_UMAC_BSS_LIST
#endif

#ifdef __SUPPORT_IEEE80211W__
  #define CONFIG_IEEE80211W             // Protected Management Frame
  #undef  CONFIG_IEEE80211W_SIGMA       // Only define for sigma test
#endif

/* Diabled SAE FFC */
#ifdef CONFIG_SAE
  #define __DISABLE_SAE_FFC__
#endif // CONFIG_SAE

#define CONFIG_MONITOR_THREAD_EVENT_CHANGE

/* Start - Code Optimize *****************************************************/

#undef  CONFIG_FST                      // Don't enable this feature
#undef  CONFIG_FILS                     // Don't enable this feature
#undef  CONFIG_EXT_PASSWORD             // Don't enable this feature
#undef  CONFIG_IBSS_RSN                 // Don't enable this feature
#undef  CONFIG_ERP                      // Don't enable this feature

#undef  CONFIG_RIC_ELEMENT
#undef  CONFIG_LCI
#undef  CONFIG_SCHED_SCAN
#undef  CONFIG_SCAN_WORK
#undef  CONFIG_SCAN_OFFLOAD
#undef  CONFIG_SCAN_FILTER_RSSI
#undef  CONFIG_SRP
#undef  CONFIG_IBSS
#undef  CONFIG_BSS_DMG
#undef  CONFIG_VENDOR_ELEM
#undef  CONFIG_AP_VENDOR_ELEM
#undef  CONFIG_WOWLAN
#undef  CONFIG_NEIGHBOR_AP_DB
#undef  CONFIG_RANDOM_ADDR
#undef  CONFIG_M2U_BC_DEAUTH
#undef  CONFIG_ASSOC_CB
#undef  CONFIG_RADIO_WORK
#undef  CONFIG_IGNOR_OLD_SCAN
#undef  CONFIG_OPENSSL_MOD
#undef  CONFIG_SUPP27_EAPOL
#undef  CONFIG_SUPP27_IFACE
#undef  CONFIG_AP_BSS_LOAD_UPDATE
#undef  CONFIG_CHANNEL_UTILIZATION
#undef  CONFIG_MAC_RAND_SCAN
#undef  CONFIG_CTRL_PNO
#undef  CONFIG_DISALLOW_BSSID
#undef  CONFIG_DISALLOW_SSID

#undef  CONFIG_SUPP27_PROBE_REQ
#if defined ( CONFIG_SUPP27_PROBE_REQ )
  #undef  CONFIG_NO_AUTH_IF_SEEN_ON
  #undef  CONFIG_NO_PROBE_RESP_IF_SEEN_ON
  #undef  CONFIG_NO_PROBE_RESP_IF_MAX_STA
#endif // CONFIG_SUPP27_PROBE_REQ

#undef  CONFIG_SUPP27_KEY_MGMT
#undef  CONFIG_SUPP27_RADIUS
#undef  CONFIG_SUPP27_CIPHER
#undef  CONFIG_SUPP27_STA_SM
#undef  CONFIG_SUPP27_AUTH
#undef  CONFIG_SUPP27_MIC_LEN
#undef  CONFIG_SUPP27_CONFIG_NVRAM
#undef  CONFIG_SUPP27_CONFIG
#undef  CONFIG_SUPP27_SCAN
#undef  CONFIG_SUPP27_AP_DRV_CB
#undef  CONFIG_SUPP27_AP_NOTIF_ASSOC
#undef  CONFIG_SUPP27_ROAM_CONSORTIUM
#undef  CONFIG_SUPP27_EVENTS
#undef  CONFIG_SUPP27_STA_INFO
#undef  CONFIG_STA_EXT_CAPAB
#undef  CONFIG_SHA384_WPS

#if defined ( CONFIG_ENTERPRISE )
  #define CONFIG_SUPP27_BIN_CLR_FREE
#else
  #undef  CONFIG_SUPP27_BIN_CLR_FREE
#endif  // CONFIG_ENTERPRISE

#undef  CONFIG_SUPP27_STR_CLR_FREE
#undef  CONFIG_SUPP27_DRV_80211
#undef  CONFIG_SUPP27_BEACON
#undef  CONFIG_SUPP27_WPA_DRV_SMPS_MODE
#undef  CONFIG_SUPP27_STA_SEEN
#undef  CONFIG_SUPP27_STA_TRACK
#undef  CONFIG_SUPP27_DFS_DOMAIN

#undef  CONFIG_SUPP27_WPS_NFC
#undef  CONFIG_SUPP27_WPS_2ND_DEV
#undef  CONFIG_SUPP27_WPS_DUALBAND

#undef  CONFIG_DBG_LOG_MSG                // supp_debug.h

#undef  __SUPP_27_SUPPORT__

#undef  UNUSED_CODE

/* End - Code Optimize *******************************************************/

/* End - Not supported, might be used in future *****************************/

#endif /*__SUPP_DEF_H__*/
/* EOF */
