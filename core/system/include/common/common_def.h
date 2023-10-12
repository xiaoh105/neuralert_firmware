/**
 ****************************************************************************************
 *
 * @file common_def.h
 *
 * @brief Define for common variables for SDK
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

#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#include "da16x_types.h"
#include "sdk_type.h"

#include "user_def.h"

#define NVR_MAGIC_CODE                      0x7E7EBEAF
#define NVR_KEY_MAGIC_CODE                  "INIT_MAGIC"

#define NVR_KEY_SETUP_MODE                  "SETUP_MODE"

#define NVR_KEY_ENVIRON_DEVICE              "ENVIRON_DEVICE"
#define NVR_KEY_SYSMODE                     ENV_SYS_MODE

#define NVR_KEY_PROFILE_0                   "N0_Profile"
#define NVR_KEY_PROFILE_1                   "N1_Profile"

#define NVR_KEY_SSID_0                      "N0_ssid"
#define NVR_KEY_SSID_1                      "N1_ssid"
#define NVR_KEY_MODE_0                      "N0_mode"
#define NVR_KEY_MODE_1                      "N1_mode"
#define NVR_KEY_NETMODE_0                   "0:NETMODE"
#define NVR_KEY_NETMODE_1                   "1:NETMODE"
#define NVR_KEY_COUNTRY_CODE                "country_code"
#define NVR_KEY_IPADDR_0                    "0:IPADDR"
#define NVR_KEY_IPADDR_1                    "1:IPADDR"
#define NVR_KEY_NETMASK_0                   "0:NETMASK"
#define NVR_KEY_NETMASK_1                   "1:NETMASK"
#define NVR_KEY_GATEWAY_0                   "0:GATEWAY"
#define NVR_KEY_GATEWAY_1                   "1:GATEWAY"
#define NVR_KEY_DNSSVR_0                    "0:DNSSVR"
#define NVR_KEY_DNSSVR_0_2ND                "0:DNSSVR2"
#define NVR_KEY_DNSSVR_1                    "1:DNSSVR"
#define NVR_KEY_DHCPD                       "USEDHCPD"
#define NVR_KEY_DHCPD_IPCNT                 "DHCPD_IPCNT"

#define NVR_KEY_DHCP_S_IP                    "DHCPD_S_IP"
#define NVR_KEY_DHCP_E_IP                    "DHCPD_E_IP"
#define NVR_KEY_DHCP_TIME                    "DHCPD_TIME"
#define NVR_KEY_DHCP_DNS_IP                  "DHCPD_DNS"
#define NVR_KEY_DHCP_RESP_DELAY              "DHCPD_RESP_DELAY"

#define NVR_KEY_DPM_MODE                     "dpm_mode"
#define NVR_KEY_DPM_DYNAMIC_PERIOD_SET       "DPM_DDPS"
#define NVR_KEY_DPM_KEEPALIVE_TIME           "DPM_KEEPALIVE"
#define NVR_KEY_DPM_USER_WAKEUP_TIME         "DPM_USER_WU_TIME"
#define NVR_KEY_DPM_WAKEUP_CONDITION         "DPM_WAKEUP_COND"
#define NVR_KEY_DPM_TIM_WAKEUP_TIME          "DPM_TIM_WU_TIME"
#define NVR_KEY_DPM_AB_WF_CONN_RETRY         "DPM_AB_WIFI_CONN_RETRY_CNT"
#define NVR_KEY_DPM_ABNORM_STOP              "dpm_abnorm_stop"

/*
 * For DPM abnormal functions ...
 */
#define DPM_ABNORM_WIFI_CONN_WAIT            0
#define DPM_ABNORM_DHCP_RSP_WAIT             1
#define DPM_ABNORM_ARP_RSP_WAIT              2
#define DPM_ABNORM_DPM_FAIL_WAIT             3
#define DPM_ABNORM_DPM_WIFI_RETRY_CNT        4


#define NVR_KEY_DPM_DYNAMIC_PERIOD_SET       "DPM_DDPS"

#define NVR_UART1_MONITOR_DISABLE            "UART1_MONITOR_DISABLE"

#define NVR_KEY_DL_CLI_PERIOD                "dl_cli_period"
#define NVR_KEY_DL_SVC_SVR_IP                "dl_svc_svr_ip"

#define NVR_KEY_IPERF_C                      "USEIPERF"

#define NVR_KEY_CoAP                         "USECoAP"
#define NVR_KEY_HTTP_S                       "USEHTTPServer"
#define NVR_KEY_ICMP_S                       "USEICMPServer"
#define NVR_KEY_ICMP_C                       "USEICMPClient"
#define NVR_KEY_ACS                          "USEACS"
#define NVR_KEY_SNTP_C                       "SNTP_RUN_FLAG"
#define NVR_KEY_DHCP_C                       "USEDHCPClient"
#define NVR_KEY_CHANNEL                      "N1_frequency"
#define NVR_KEY_PROACTKEY_C 				 "N1_proActKeyCaching"

#define NVR_KEY_SNTP_SERVER_DOMAIN           "SNTP_SVR"
#define NVR_KEY_SNTP_SERVER_DOMAIN_1         "SNTP_SVR_1"
#define NVR_KEY_SNTP_SERVER_DOMAIN_2         "SNTP_SVR_2"
#define NVR_KEY_SNTP_SYNC_PERIOD             "SNTP_PERIOD"

#define NVR_KEY_TIMEZONE                     "TZONE"
#define NVR_KEY_OTA_UPDATE                   "OTA_Update"
#define NVR_KEY_OTA_TIMEOUT                  "OTA_op_timeout"
#define NVR_KEY_OTA_UP_SVR_DOMAIN            "OTA_Svr_Domain"
#define NVR_KEY_OTA_UP_SVR_IP                "OTA_Svr_IP"
#define NVR_KEY_OTA_PTIM                     "OTA_ptim"
#define NVR_KEY_OTA_RAMLIB                   "OTA_rlib"
#define NVR_KEY_OTA_RTOS                     "OTA_rtos"
#define NVR_KEY_OTA_INDOOR                   "OTA_indoor"
#define NVR_KEY_OTA_OUTDOOR                  "OTA_outdoor"

#define NVR_KEY_OTA_UPDATE_PERIOD            "OTA_Period"
#define NVR_KEY_WLANMODE_0                   "N0_wifi_mode"
#define NVR_KEY_WLANMODE_1                   "N1_wifi_mode"

#define NVR_KEY_AUTH_TYPE_0                  "N0_key_mgmt"
#define NVR_KEY_ENC_TYPE_0                   "N0_pairwise"

#define NVR_KEY_WEPTYPE_0                    "N0_wep_type"        /* need del */
#define NVR_KEY_WEPINDEX_0                   "N0_wep_tx_keyidx"    /* chg */
#define NVR_KEY_WEPKEYTYPE_0                 "N0_wepkey_type"    /* need del */
#define NVR_KEY_WEPKEY64_0                   "N0_wep_key"        /* need del */
#define NVR_KEY_WEPKEY128_0                  "N0_wep_key"        /* need del */
#define NVR_KEY_WEPKEY_0                     "N0_wep_key"        /* chg */
#define NVR_KEY_WEPKEY0_0                    "N0_wep_key0"        /* chg */
#define NVR_KEY_WEPKEY1_0                    "N0_wep_key1"        /* chg */
#define NVR_KEY_WEPKEY2_0                    "N0_wep_key2"        /* chg */
#define NVR_KEY_WEPKEY3_0                    "N0_wep_key3"        /* chg */

#define NVR_KEY_ENCKEY_0                     "N0_psk"
// for __SUPPORT_FAST_CONN_SLEEP_12__
#define NVR_KEY_PSK_RAW_0                    "N0_PSK_RAW_KEY"

#define NVR_KEY_PROTO_0                      "N0_proto"
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
#define NVR_KEY_SAE_PASS_0                   "N0_sae_password"
#define NVR_KEY_PMF_0                        "N0_ieee80211w"
#endif // __SUPPORT_WPA3_PERSONAL_CORE__

#define NVR_KEY_AUTH_TYPE_1                  "N1_key_mgmt"
#define NVR_KEY_ENC_TYPE_1                   "N1_pairwise"

#define NVR_KEY_WEPTYPE_1                    "N1_wep_type"        /* need del */
#define NVR_KEY_WEPINDEX_1                   "N1_wep_tx_keyidx"    /* chg */
#define NVR_KEY_WEPKEYTYPE_1                 "N1_wepkey_type"    /* need del */
#define NVR_KEY_WEPKEY64_1                   "N1_wep_key"        /* need del */
#define NVR_KEY_WEPKEY128_1                  "N1_wep_key"        /* need del */
#define NVR_KEY_WEPKEY_1                     "N1_wep_key"        /* chg */
#define NVR_KEY_WEPKEY0_1                    "N1_wep_key0"        /* chg */
#define NVR_KEY_WEPKEY1_1                    "N1_wep_key1"        /* chg */
#define NVR_KEY_WEPKEY2_1                    "N1_wep_key2"        /* chg */
#define NVR_KEY_WEPKEY3_1                    "N1_wep_key3"        /* chg */

#define NVR_KEY_ENCKEY_1                     "N1_psk"
#define NVR_KEY_PROTO_1                      "N1_proto"

#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
#define NVR_KEY_SAE_PASS_1                    "N1_sae_password"
#define NVR_KEY_PMF_1                         "N1_ieee80211w"
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
#define NVR_KEY_GROUP_0                       "N0_group"
#define NVR_KEY_GROUP_1                       "N1_group"

#define NVR_KEY_P2P_GO                        "P2P_GO"
#define NVR_KEY_P2P_DEV_NAME                  "dev_name"
#define NVR_KEY_P2P_LISTEN_CH                 "p2p_listen_channel"
#define NVR_KEY_P2P_GO_INTENT                 "p2p_go_intent"
#define NVR_KEY_P2P_GO_OP_CH                  "p2p_oper_channel"

// for __SUPPORT_FAST_CONN_SLEEP_12__
#define NVR_KEY_FST_CONNECT                   "FST_CONNECT"        /* Station Only */
// for __SUPPORT_ASSOC_CHANNEL__
#define NVR_KEY_ASSOC_CH                      "ASSOC_CH"            /* Station Only */

#define NVR_KEY_VOL_UID                       "VOL_UID"

#define NVR_KEY_DBG_TXPWR                     "DBG_TXPWR_L"
#define NVR_KEY_PWR_CTRL                      "pwr_ctrl_grade"
#define NVR_KEY_PWR_CTRL_SAME_IDX_TBL         "SAME_PWR_IDX_VAL"    /* same index value table */

#define CLI_DELIMIT_TAB                       "\t"

#define ENABLE_DHCP_CLIENT                    DHCPCLIENT
#define ENABLE_STATIC_IP                      STATIC_IP

#define CHANNEL_1_FREQ                        2412
#define CHANNEL_2_FREQ                        2417
#define CHANNEL_3_FREQ                        2422
#define CHANNEL_4_FREQ                        2427
#define CHANNEL_5_FREQ                        2432
#define CHANNEL_6_FREQ                        2437
#define CHANNEL_7_FREQ                        2442
#define CHANNEL_8_FREQ                        2447
#define CHANNEL_9_FREQ                        2452
#define CHANNEL_10_FREQ                       2457
#define CHANNEL_11_FREQ                       2462
#define CHANNEL_12_FREQ                       2467
#define CHANNEL_13_FREQ                       2472
#define CHANNEL_14_FREQ                       2484

#define MODE_AUTH_OPEN_STR                    "NONE"
#define MODE_AUTH_WEP_STR                     "????"
#define MODE_AUTH_WPA_PSK_STR                 "WPA-PSK"
#define MODE_AUTH_WPA2_PSK_STR                "?????"
#define MODE_AUTH_WPA_AUTO_PSK_STR            "??????"

#define proto_WPA                            "WPA"
#define proto_RSN                            "RSN"
#define proto_WPA2                           "WPA2"
#define proto_OSEN                           "OSEN"

#define key_mgmt_WPA_PSK                     "WPA-PSK"
#define key_mgmt_WPA_EAP                     "WPA-EAP"
#define key_mgmt_IEEE8021X                   "IEEE8021X"
#define key_mgmt_NONE                        "????"
#define key_mgmt_WPA_NONE                    "WPA-NONE"
#define key_mgmt_FT_PSK                      "FT-PSK"
#define key_mgmt_FT_EAP                      "FT-EAP"
#define key_mgmt_WPA_PSK_SHA256              "WPA-PSK-SHA256"
#define key_mgmt_WPA_EAP_SHA256              "WPA-EAP-SHA256"
#define key_mgmt_WPS                         "WPS"
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
#define key_mgmt_WPA3_OWE                    "OWE"
#define key_mgmt_WPA3_SAE                    "SAE"
#define key_mgmt_WPA_PSK_WPA3_SAE            "WPA-PSK SAE"
#endif // __SUPPORT_WPA3_PERSONAL_CORE__

#define pairwise_CCMP                        "CCMP"
#define pairwise_TKIP                        "TKIP"
#define group_TKIP_CCMP                      "TKIP WEP104 WEP40"

#define MODE_11BGN                           0
#define MODE_11GN                            1
#define MODE_11BG                            2
#define MODE_11N                             3
#define MODE_11G                             4
#define MODE_11B                             5

#define MODE_AUTH_OPEN                       0
#define MODE_AUTH_WEP                        1
#define MODE_AUTH_WPA_PSK                    2
#define MODE_AUTH_WPA2_PSK                   3
#define MODE_AUTH_WPA_AUTO_PSK               4

#define MODE_WEP_64                          0
#define MODE_WEP_128                         1

#define MODE_WEP_TYPE_ASCII                  0
#define MODE_WEP_TYPE_HEXA                   1

#define MODE_WEP_INDEX0                      0
#define MODE_WEP_INDEX1                      1
#define MODE_WEP_INDEX2                      2
#define MODE_WEP_INDEX3                      3

#define MODE_ENC_TKIP                        0
#define MODE_ENC_AES                         1
#define MODE_ENC_AUTO                        2

#define MODE_ENABLE                          1
#define MODE_DISABLE                         0

#define SNTPCLINET_ENABLE                    1
#define SNTPCLINET_DISABLE                   0

#define VIRTUAL_DEV0                         0
#define VIRTUAL_DEV1                         1
#define VIRTUAL_DEV2                         2
#define VIRTUAL_DEV3                         3

enum sysmode {
    SYSMODE_STA_ONLY    = 0,                 /* 0 */
    SYSMODE_AP_ONLY,                         /* 1 */
#if defined ( __SUPPORT_P2P__ )
    SYSMODE_P2P_ONLY,                        /* 2 */
    SYSMODE_P2PGO,                           /* 3 */
#endif	// __SUPPORT_P2P__
    SYSMODE_STA_N_AP,                        /* 4 */
#if defined ( __SUPPORT_P2P__ )
    SYSMODE_STA_N_P2P,                       /* 5 */
#endif	// __SUPPORT_P2P__

#if defined ( __SUPPORT_MESH__ )
    SYSMODE_MESH_POINT,                      /* 6 */
    SYSMODE_STA_N_MESH_PORTAL,               /* 7 */
#endif	// __SUPPORT_MESH__
    SYSMODE_TEST        = 9
};

/****************************/
/*** Default value/string ***/
/****************************/
#define DFLT_SYSMODE                        SYSMODE_AP_ONLY
#define DFLT_WLANMODE                       MODE_11BGN
#define DFLT_MODE_AUTH                      MODE_AUTH_OPEN
#define DFLT_MODE_AUTH_STR                  MODE_AUTH_OPEN_STR
#define DFLT_MODE_WEP                       MODE_WEP_128
#define DFLT_MODE_WEP_TYPE                  MODE_WEP_TYPE_ASCII
#define DFLT_MODE_WEP_INDEX                 MODE_WEP_INDEX3
#define DFLT_MODE_ENC                       MODE_ENC_AUTO

#define DFLT_NETMODE_0                      DEFAULT_NETMODE_WLAN0
#define DFLT_NETMODE_1                      DEFAULT_NETMODE_WLAN1
#define DFLT_DHCP_S_LEASE_IP_CNT            DEFAULT_DHCP_LEASE_COUNT
#define DFLT_DHCP_S_LEASE_IP_CNT_MAX        MAX_DHCP_LEASE_COUNT
#define DFLT_DHCP_S_LEASE_TIME              DEFAULT_DHCP_SERVER_LEASE_TIME
#define DFLT_DHCP_S_LEASE_TIME_MAX          MAX_DHCP_SERVER_LEASE_TIME
#define DFLT_DHCP_S_LEASE_TIME_MIN          MIN_DHCP_SERVER_LEASE_TIME
#define DFLT_HTTP_SERVER_PORT               DEFAULT_HTTP_SERVER_PORT
#define DFLT_AP_IP                          DEFAULT_IPADDR_WLAN1
#define DFLT_AP_SUBNET                      DEFAULT_SUBNET_WLAN1
#define DFLT_AP_GW                          DEFAULT_GATEWAY_WLAN1
#define DFLT_AP_DNS                         DEFAULT_DNS_WLAN1
#define DFLT_STA_IP                         DEFAULT_IPADDR_WLAN0
#define DFLT_STA_SUBNET                     DEFAULT_SUBNET_WLAN0
#define DFLT_STA_GW                         DEFAULT_GATEWAY_WLAN0
#define DFLT_STA_DNS                        DEFAULT_DNS_WLAN0

#define DFLT_AP_SSID                        "\"DA16200\""
#define DFLT_STA_SSID                       "\"DA16200\""

#define DFLT_P2P_DEV_NAME                   "DA16200"
#define DFLT_P2P_LISTEN_CH                  1
#define DFLT_P2P_GO_INTENT                  3
#define DFLT_P2P_GO_OP_CH                   1
#define DFLT_P2P_GO                         MODE_DISABLE

#define DFLT_DHCP_S_S_IP                    "10.0.0.2"
#define DFLT_DHCP_S_E_IP                    "10.0.0.11"
#define DFLT_DHCP_S_DNS_IP                  "8.8.8.8"

#define DFLT_AP_COUNTRY_CODE                "US"
#define DFLT_STA_COUNTRY_CODE               "US"
#define DFLT_AP_ACS                         MODE_DISABLE
#define DFLT_AP_DHCP_S                      MODE_ENABLE
#define DFLT_DHCP_C                         MODE_ENABLE
#define DFLT_AP_CHANNEL                     CHANNEL_1_FREQ

#define DFLT_HTTP_S                         MODE_DISABLE
#define DFLT_CoAP                           MODE_DISABLE
#define DFLT_ICMP_S                         MODE_DISABLE
#define DFLT_ICMP_C                         MODE_ENABLE

#define DFLT_SNTP_C                         MODE_DISABLE
#define DFLT_SNTP_SERVER_DOMAIN             "pool.ntp.org"
#define DFLT_SNTP_SERVER_DOMAIN_1           "1.pool.ntp.org"
#define DFLT_SNTP_SERVER_DOMAIN_2           "2.pool.ntp.org"
#define DFLT_SNTP_SYNC_PERIOD               (3600*36)

#define DFLT_OTA_UPDATE                     MODE_ENABLE
#define DFLT_OTA_UP_SVR_DOMAIN              "ota.diasemi.com"
#define DFLT_OTA_UP_SVR_IP                  "10.20.30.40"    /* TEMPORARY */
#define DFLT_OTA_UPDATE_RUN_MODE            0                /* Manual / Auto = 1 */
#define DFLT_OTA_UPDATE_PERIOD_MIN          10                /* minute */
#define DFLT_OTA_UPDATE_PERIOD_MAX          44640            /* (31 days) minute */
#define DFLT_OTA_UPDATE_PERIOD              DFLT_OTA_UPDATE_PERIOD_MAX //1440 /* (1 day) )minute */


#define DFLT_DPM                            MODE_DISABLE

/* DPM WAKEUP TYPE */
#define DPM_WAKEUP_TYPE_ALL                 0
#define DPM_WAKEUP_TYPE_UNICAST             1
#define DPM_WAKEUP_TYPE_BROADCAST           2
#define DPM_WAKEUP_TYPE_NONE                3
#define DFLT_DPM_WAKEUP_TYPE                DPM_WAKEUP_TYPE_ALL

#define DFLT_DPM_USER_WAKEUP_TIME           0    /* msec. */
#define DFLT_DPM_TEST_APP                   0    /* Disable */

#ifdef __DPM_TIM_WAKEUP_TIME_MSEC__ /* TIM wakeup count => TIM wakeup time */
  #if defined(__SUPPORT_DPM_DTIM_DFLT__)
    #define  USER_DPM_TIM_WAKEUP_COUNT      1000    /* ms */
  #else
    #define USER_DPM_TIM_WAKEUP_COUNT       102    /* ms */
  #endif    /* __SUPPORT_DPM_DTIM_DFLT__ */

#else /* __DPM_TIM_WAKEUP_TIME_MSEC__ */

  #if defined(__SUPPORT_DPM_DTIM_DFLT__)
    #define USER_DPM_TIM_WAKEUP_COUNT       10    /* DTIM */
  #else
    #define USER_DPM_TIM_WAKEUP_COUNT       1    /* DTIM */
  #endif    /* __SUPPORT_DPM_DTIM_DFLT__ */
#endif /* __DPM_TIM_WAKEUP_TIME_MSEC__ */

#ifdef USER_DPM_TIM_WAKEUP_COUNT
  #define DFLT_DPM_TIM_WAKEUP_COUNT         USER_DPM_TIM_WAKEUP_COUNT
#else
  #define DFLT_DPM_TIM_WAKEUP_COUNT         10    /* default tim wakeup */
#endif

#define DPM_WAKEUP_CONDITION_ALL            0
#define DPM_WAKEUP_CONDITION_UNICAST        1
#define DPM_WAKEUP_CONDITION_BROADCAST      2
#define DPM_WAKEUP_CONDITION_NONE           3
#define DPM_WAKEUP_CONDITION_BCMC_FILTER    4    /* BC & MC wakeup disable */

#define DFLT_DPM_WAKEUP_CONDITION           DPM_WAKEUP_CONDITION_ALL

/* Dynamic Period Set */
#define DFLT_DPM_DYNAMIC_PERIOD_SET         0

/* Keepalive Time */
#define DFLT_DPM_KEEPALIVE_TIME             (30*1000)    /* ms */
#define MIN_DPM_KEEPALIVE_TIME              0            /* ms */
#define MAX_DPM_KEEPALIVE_TIME              (600*1000)    /* ms, 10 minutes */

/* User Wakeup Time */
#define MIN_DPM_USER_WAKEUP_TIME            0
#define MAX_DPM_USER_WAKEUP_TIME            (86400 * 1000)    /* msec / 24h */


/* DPM TIM Wakeup Time */
#ifdef __DPM_TIM_WAKEUP_TIME_MSEC__ /* TIM wakeup count => TIM wakeup time */
#define MIN_DPM_TIM_WAKEUP_COUNT            102        /* ms */
#define MAX_DPM_TIM_WAKEUP_COUNT            (int)(65535*102.4)    /* 255 DTIM Interval */
#else
#define MIN_DPM_TIM_WAKEUP_COUNT            1        /* 1= 102.4 ms */
#define MAX_DPM_TIM_WAKEUP_COUNT            6000    /* 255 DTIM Interval */
#define MAX_DPM_TIM_WAKEUP_COUNT_DDPS       30
#endif /* __DPM_TIM_WAKEUP_TIME_MSEC__ */

#define DFLT_DL_CLI_PERIOD                  1800
#define DFLT_DL_SVC_SVR_IP                  "10.20.30.40"    /* TEMPORARY IP */

#define DFLT_IPERF_C                        MODE_DISABLE

#define DFLT_DNS_SD                         MODE_ENABLE
#define DFLT_DNS_SD_SVC_NAME                "DA16200"
#define DFLT_DNS_SD_SVC_TYPE                "_service._tcp"
#define DFLT_DNS_SD_SVC_PORT                9110
#define DFLT_DNS_SD_SVC_DESCRIPT            "DA16X Device"
#define DFLT_MDNS                           MODE_ENABLE
#define DFLT_MDNS_NAME                      "DA16200"
#define DFLT_XMDNS                          MODE_ENABLE
#define DFLT_XMDNS_NAME                     "DA16200"

#define DFLT_NONE                           0

#define HEXA_ON                             1
#define HEXA_OFF                            0

/* ANSI CODE */
#define ESCCODE                             "\33"

#define BLACK_COLOR                         "\33[1;30m"
#define RED_COLOR                           "\33[1;31m"
#define GREEN_COLOR                         "\33[1;32m"
#define YELLOW_COLOR                        "\33[1;33m"
#define BLUE_COLOR                          "\33[1;34m"
#define MAGENTA_COLOR                       "\33[1;35m"
#define CYAN_COLOR                          "\33[1;36m"
#define WHITE_COLOR                         "\33[1;37m"
#define CLEAR_COLOR                         "\33[0m"

/* Set Attributes */
#define ANSI_BOLD                           "\33[1m"
#define ANSI_UNDERLINE                      "\33[4m"
#define ANSI_BLINK                          "\33[5m"
#define ANSI_REVERSE                        "\33[7m"

#define ANSI_R_BOLD                         "\33[21m"
#define ANSI_R_UNDERLINE                    "\33[24m"
#define ANSI_R_BLINK                        "\33[25m"
#define ANSI_R_REVERSE                      "\33[27m"

/* Rset Attributes */
#define ANSI_NORMAL                         "\33[0m"
#define ANSI_RESET_BOLD                     "\33[21m"
#define ANSI_RESET_UNDERLINE                "\33[24m"
#define ANSI_RESET_BLINK                    "\33[25m"
#define ANSI_RESET_REVERSE                  "\33[27m"

/* Control */
#define ANSI_CLEAR                          "\33[2J"
#define ANSI_CURON                          "\33[?25h"
#define ANSI_CUROFF                         "\33[?25l"
#define ANSI_BELL                           "\7"
#define ANSI_ERASE                          "\33[0J"
#define ANSI_LEFT                           "\33[1D"
#define ANSI_RIGHT                          "\33[1C"

/* Foreground Color (text) */
#define ANSI_COLOR_BLACK                    "\33[30m"
#define ANSI_COLOR_RED                      "\33[31m"
#define ANSI_COLOR_GREEN                    "\33[32m"
#define ANSI_COLOR_YELLOW                   "\33[33m"
#define ANSI_COLOR_BLUE                     "\33[34m"
#define ANSI_COLOR_MAGENTA                  "\33[35m"
#define ANSI_COLOR_CYAN                     "\33[36m"
#define ANSI_COLOR_WHITE                    "\33[37m"
#define ANSI_COLOR_LIGHT_RED                "\33[1;31m"
#define ANSI_COLOR_LIGHT_GREEN              "\33[1;32m"
#define ANSI_COLOR_LIGHT_YELLOW             "\33[1;33m"
#define ANSI_COLOR_LIGHT_BLUE               "\33[1;34m"
#define ANSI_COLOR_LIGHT_MAGENTA            "\33[1;35m"
#define ANSI_COLOR_LIGHT_CYAN               "\33[1;36m"
#define ANSI_COLOR_LIGHT_WHITE              "\33[1;37m"
#define ANSI_COLOR_DEFULT                   "\33[0;39m"

/* Background Color */
#define ANSI_BCOLOR_BLACK                   "\33[40m"
#define ANSI_BCOLOR_RED                     "\33[41m"
#define ANSI_BCOLOR_GREEN                   "\33[42m"
#define ANSI_BCOLOR_YELLOW                  "\33[43m"
#define ANSI_BCOLOR_BLUE                    "\33[44m"
#define ANSI_BCOLOR_MAGENTA                 "\33[45m"
#define ANSI_BCOLOR_CYAN                    "\33[46m"
#define ANSI_BCOLOR_WHITE                   "\33[47m"
#define ANSI_BCOLOR_LIGHT_RED               "\33[1;41m"
#define ANSI_BCOLOR_LIGHT_GREEN             "\33[1;42m"
#define ANSI_BCOLOR_LIGHT_YELLOW            "\33[1;43m"
#define ANSI_BCOLOR_LIGHT_BLUE              "\33[1;44m"
#define ANSI_BCOLOR_LIGHT_MAGENTA           "\33[1;45m"
#define ANSI_BCOLOR_LIGHT_CYAN              "\33[1;46m"
#define ANSI_BCOLOR_LIGHT_WHITE             "\33[1:47m"
#define ANSI_BCOLOR_DEFULT                  "\33[0;49m"


#define VT_CLEAR                PRINTF("\33[2J")
#define VT_CURPOS(X,Y)          PRINTF("\33[%d;%dH", Y, X)
#define VT_NORMAL               PRINTF("\33[0m")
#define VT_BOLD                 PRINTF("\33[1m")
#define VT_BLINK                PRINTF("\33[5m")
#define VT_REVERSE              PRINTF("\33[7m")
#define VT_CURON                PRINTF("\33[?25h")
#define VT_CUROFF               PRINTF("\33[?25l")
#define VT_BELL                 PRINTF("\007")
#define VT_ERASE                PRINTF("\33[0J")
#define VT_LEFT                 PRINTF("\33[1D")
#define VT_RIGHT                PRINTF("\33[1C")
#define VT_LINECLEAR(X)         VT_CURPOS(1, X); PRINTF("\33[2K")
#define VT_COLORBLACK           PRINTF("\33[30m")
#define VT_COLORRED             PRINTF("\33[31m")
#define VT_COLORGREEN           PRINTF("\33[32m")
#define VT_COLORYELLOW          PRINTF("\33[33m")
#define VT_COLORBLUE            PRINTF("\33[34m")
#define VT_COLORMAGENTA         PRINTF("\33[35m")
#define VT_COLORCYAN            PRINTF("\33[36m")
#define VT_COLORWHITE           PRINTF("\33[37m")
#define VT_COLORDEFULT          PRINTF("\33[39m")
#define VT_BCOLORBLACK          PRINTF("\33[40m")
#define VT_BCOLORRED            PRINTF("\33[41m")
#define VT_BCOLORGREEN          PRINTF("\33[42m")
#define VT_BCOLORYELLOW         PRINTF("\33[43m")
#define VT_BCOLORBLUE           PRINTF("\33[44m")
#define VT_BCOLORMAGENTA        PRINTF("\33[45m")
#define VT_BCOLORCYAN           PRINTF("\33[46m")
#define VT_BCOLORWHITE          PRINTF("\33[47m")
#define VT_BDEFULT              PRINTF("\33[49m")
#define VT_COLOROFF             VT_BOLD;VT_BCOLORBLACK;VT_COLORWHITE

#define vt_CLEAR                PRINTF("\33[2J")
#define vt_CURPOS(X,Y)          PRINTF("\33[%d;%dH", Y, X)
#define vt_NORMAL               PRINTF("\33[0m")
#define vt_BOLD                 PRINTF("\33[1m")
#define vt_BLINK                PRINTF("\33[5m")
#define vt_REVERSE              PRINTF("\33[7m")
#define vt_CURON                PRINTF("\33[?25h")
#define vt_CUROFF               PRINTF("\33[?25l")
#define vt_BELL                 PRINTF("\007")
#define vt_ERASE                PRINTF("\33[0J")
#define vt_LEFT                 PRINTF("\33[1D")
#define vt_RIGHT                PRINTF("\33[1C")
#define vt_LINECLEAR(X)         vt_CURPOS(1, X); PRINTF("\33[2K")
#define vt_COLORBLACK           PRINTF("\33[30m")
#define vt_COLORRED             PRINTF("\33[31m")
#define vt_COLORGREEN           PRINTF("\33[32m")
#define vt_COLORYELLOW          PRINTF("\33[33m")
#define vt_COLORBLUE            PRINTF("\33[34m")
#define vt_COLORMAGENTA         PRINTF("\33[35m")
#define vt_COLORCYAN            PRINTF("\33[36m")
#define vt_COLORWHITE           PRINTF("\33[37m")
#define vt_COLORDEFULT          PRINTF("\33[39m")
#define vt_BCOLORBLACK          PRINTF("\33[40m")
#define vt_BCOLORRED            PRINTF("\33[41m")
#define vt_BCOLORGREEN          PRINTF("\33[42m")
#define vt_BCOLORYELLOW         PRINTF("\33[43m")
#define vt_BCOLORBLUE           PRINTF("\33[44m")
#define vt_BCOLORMAGENTA        PRINTF("\33[45m")
#define vt_BCOLORCYAN           PRINTF("\33[46m")
#define vt_BCOLORWHITE          PRINTF("\33[47m")
#define vt_BDEFULT              PRINTF("\33[49m")
#define vt_COLOROFF             vt_BCOLORBLACK;vt_COLORWHITE


//ISDIGIT onderzoekt of een karakter numeric is
#define ISDIGIT(c)              ( (c < '0' || c > '9') ? 0 : 1)

#define UNDEF_PORT              0
#define HTTP_SVR_PORT           80
#define OTA_UPDATE_PORT         8048
#define TCP_TRX_PORT            8049
#define UDP_TRX_PORT            8047
#define TCP_TRX_SRC_PORT        8050
#define UDP_TRX_SRC_PORT        8051
#define TCP_TX_SRC_PORT         8877
#define TCP_TX_PORT             8878
#define TCP_RX_PORT             8879
#define DL_SERVICE_PORT         8046
#define MON_SERVICE_PORT        8047
#define MULTICAST_PORT          5353
#define RTSP_PORT               8554

#define CM_UDP_PORT             6060
#define DEMO_SERVICE_UDP_PORT   6123

#define UART0_BAUDRATE          115200
#define UART1_BAUDRATE          115200

#define ENV_ASD_ENABLED         "ENV_ASD_ENABLED"

#define NVR_KEY_AP_LIVING_TIME  "ap_living_time"
#define DFLT_AP_LIVING_TIME     60000            /* 1 Min */

#define NVR_KEY_UART1_BAUDRATE  "UART1_BAUDRATE"
#define NVR_KEY_UART1_BITS      "UART1_BITS"
#define NVR_KEY_UART1_PARITY    "UART1_PARITY"
#define NVR_KEY_UART1_STOPBIT   "UART1_STOPBIT"
#define NVR_KEY_UART1_FLOWCTRL  "UART1_FLOWCTRL"

#define DPM_MON_RETRY_CNT       10

#if defined ( __USER_DHCP_HOSTNAME__ )
// For DHCP Client hostname
#define NVR_DHCPC_HOSTNAME      "DHCPC_HOSTNAME"
#define DHCP_HOSTNAME_MAX_LEN   32
#endif    // __USER_DHCP_HOSTNAME__


/// For Country Code
struct country_ch_power_level {
    char*   country;
    UCHAR   ch[14];
};

extern void trc_que_proc_print(UINT16 tag, const char *fmt, ...);
extern ULONG cc_power_table(UINT index);
extern UINT cc_power_table_size(void);
extern void PRINTF(const char *fmt,...);

#define NUM_SUPPORTS_COUNTRY            cc_power_table_size()


/* Easy Setup */
#define REPLY_FAIL                      "FAIL"

#define SCAN_BSSID_IDX                  0
#define SCAN_FREQ_IDX                   1
#define SCAN_SIGNAL_IDX                 2
#define SCAN_FLGA_IDX                   3
#define SCAN_SSID_IDX                   4

#define HIDDEN_SSID_DETECTION_CHAR      '\t'
#define CLI_SCAN_RSP_BUF_SIZE           3584
#define SCAN_RESULT_BUF_SIZE            4096
#define SSID_BASE64_LEN_MAX             48
#define DEFAULT_BSS_MAX_COUNT           40


#define FREERTOS_MAJOR_VERSION          tskKERNEL_VERSION_MAJOR
#define FREERTOS_MINOR_VERSION          tskKERNEL_VERSION_MINOR
#define FREERTOS_SUB_VERSION            tskKERNEL_VERSION_BUILD

/* API input parameters and general constants. */

#define _AND                            ((UINT) 2)
#define _AND_CLEAR                      ((UINT) 3)
#define _OR                             ((UINT) 0)
#define _OR_CLEAR                       ((UINT) 1)

#define _1_ULONG                        ((UINT) 1)
#define _2_ULONG                        ((UINT) 2)
#define _4_ULONG                        ((UINT) 4)
#define _8_ULONG                        ((UINT) 8)
#define _16_ULONG                       ((UINT) 16)

#define _NO_TIME_SLICE                  ((ULONG) 0)
#define _AUTO_START                     ((UINT) 1)
#define _DONT_START                     ((UINT) 0)
#define _AUTO_ACTIVATE                  ((UINT) 1)

#define _INHERIT                        ((UINT) 1)
#define _NO_INHERIT                     ((UINT) 0)


/* Thread execution state values. */
#define TASK_READY                      ((UINT) 0)
#define TASK_COMPLETED                  ((UINT) 1)
#define TASK_TERMINATED                 ((UINT) 2)

#define LOWER_16_MASK                   ((ULONG)0x0000FFFF)
#define SHIFT_BY_16                     16

#define PRINTF(...)                     trc_que_proc_print(0, __VA_ARGS__ )
#define VPRINTF(...)                    trc_que_proc_vprint(0, __VA_ARGS__ )
#define GETC()                          trc_que_proc_getchar(portMAX_DELAY)
#define GETC_NOWAIT()                   trc_que_proc_getchar(portNO_DELAY)
#define PUTC(ch)                        trc_que_proc_print(0, "%c", ch )
#define PUTS(s)                         trc_que_proc_print(0, s )

#define BUILD_OPT_DA16200_ASIC
#define BUILD_OPT_DA16200_LIBNDK
#define BUILD_OPT_DA16200_CACHEXIP
#define BUILD_OPT_DA16200_SLR

#ifdef __FOR_FCCSP_SDK__
  #define BUILD_OPT_RF9050_FCCSP
#else
  #undef BUILD_OPT_RF9050_FCCSP
#endif    // __FOR_FCCSP_SDK__

#ifdef __FCCSP_LOW_POWER__
  #define BUILD_OPT_RF9050_LP
#else
  #undef BUILD_OPT_RF9050_LP
#endif    // __FCCSP_LOW_POWER__

#ifdef    XIP_CACHE_BOOT
  #define BUILD_OPT_DA16200_MAC
#else
  #undef  BUILD_OPT_DA16200_MAC
#endif    // XIP_CACHE_BOOT

#undef  SFLASH_DEBUGGING            /* FOR_DEBUGGING */
#undef  SFLASH_NVEDIT_DEBUGGING     /* FOR_DEBUGGING */
#undef  SFLASH_ENV_DEBUGGING        /* FOR_DEBUGGING */
#undef  SFLASH_SYS_CFG_DEBUGGING    /* FOR_DEBUGGING */
#undef  SFLASH_XFC_DEBUGGING        /* FOR_DEBUGGING */

#define USING_HEAP_5


// For Wi-Fi Connection call-back event
#define WIFI_CONN_NOTI_WAIT_TICK	(100 / portTICK_PERIOD_MS)

#define WIFI_CONN_SUCC_STA           0x1
#define WIFI_CONN_FAIL_STA           0x2
#define WIFI_DISCONN_STA             0x4
#define WIFI_CONN_SUCC_SOFTAP        0x10
#define WIFI_CONN_FAIL_SOFTAP        0x20
#define WIFI_DISCONN_SOFTAP          0x40

#define WIFI_CONN_SUCC_STA_4_BLE     0x100
#define WIFI_CONN_FAIL_STA_4_BLE     0x200

// common_utils.c function...
/*
ex)
print_separate_bar("=", 10, 2);

"==========\n\n"
*/
void print_separate_bar(UCHAR text, UCHAR loop_count, UCHAR CR_loop_count);

//static unsigned int _parseDecimal ( const char** pchCursor );

//static unsigned int _parseHex ( const char** pchCursor );

//Parse a textual IPv4 or IPv6 address, optionally with port, into a binary
//array (for the address, in network order), and an optionally provided port.
//Also, indicate which of those forms (4 or 6) was parsed. Return true on
//success. ppszText must be a nul-terminated ASCII string. It will be
//updated to point to the character which terminated parsing (so you can carry
//on with other things. abyAddr must be 16 bytes. You can provide NULL for
//abyAddr, nPort, bIsIPv6, if you are not interested in any of those
//informations. If we request port, but there is no port part, then nPort will
//be set to 0. There may be no whitespace leading or internal (though this may
//be used to terminate a successful parse.
//Note: the binary address and integer port are in network order.
int ParseIPv4OrIPv6 ( const char** ppszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 );

//simple version if we want don't care about knowing how much we ate
int ParseIPv4OrIPv6_2 ( const char* pszText, unsigned char* abyAddr, int* pnPort, int* pbIsIPv6 );
int ParseIPv6ToLong ( const char* pszText, ULONG* ipv6addr, int* pnPort);


void pntdumpbin ( unsigned char* pbyBin, int nLen, int simple);
void bin2ipstr ( unsigned char* pbyBin, int nLen, int simple, unsigned char* ip_str);

void ipv6long2int(ULONG *ipv6_long, UINT *ipv6_int);
void ipv6long2str(ULONG *ipv6_long, UCHAR* ipv6_str);
void ipv6uchar2Long(UCHAR *ipv6_uchar, ULONG *ipv6_long);
void ipv6Long2uchar(ULONG *ipv6_long, UCHAR *ipv6_uchar);

int  read_nvram_int(const char *name, int *_val);
char *read_nvram_string(const char *name);

int  write_nvram_int(const char *name, int val);
int  write_nvram_string(const char *name, const char *val);
int  delete_nvram_env(const char *name);
int  clear_nvram_env(void);

int  write_tmp_nvram_int(const char *name, int val);
int  write_tmp_nvram_string(const char *name, const char *val);
int  delete_tmp_nvram_env(const char *name);
int  clear_tmp_nvram_env(void);

int  save_tmp_nvram(void);

int  da16x_cli_reply(char *cmdline, char *delimit, char *cli_reply);
UINT factory_reset(int reboot_flag);
long pow_long(long x, int order);
int  calcbits(long mask);
void longtoip(long ip, char *ipbuf);
long iptolong(char *ip);
int  getipdigit(long ipaddress, int digit);
int  isvalidip(char *theip);
int  is_in_valid_ip_class(char *theip);
int  isvalidmask(char *theip);
long subnetRangeLastIP(long ip, long subnet);
long subnetRangeFirstIP(long ip, long subnet);
long subnetBCIP(long ip, long subnet);
int  isvalidIPsubnetRange(long ip, long subnetip, long subnet);
int  isvalidIPrange(long ip, long firstIP, long lastIP);
void ulltoa(unsigned long long value, char *buf, int radix);
char *lltoa(long long value, char *buf, int radix);
int  roundToInt(double x);
int  coilToInt(double x);
int  compare_macaddr(char* macaddr1, char* macaddr2);
int  gen_ssid(char* prefix, int iface, int quotation, char* ssid, int size);
int  recovery_NVRAM(void);
int  copy_nvram_flash(UINT dest, UINT src, UINT len);

#undef  ISR_SEPERATE
#if defined(__SUPPORT_SYS_WATCHDOG__)
#undef  WATCH_DOG_ENABLE
#else
#define WATCH_DOG_ENABLE
#endif // __SUPPORT_SYS_WATCHDOG__
#undef  USING_RAMLIB

#endif /* __COMMON_DEF_H__ */

/* EOF */
