/**
 ****************************************************************************************
 *
 * @file sys_feature.h
 *
 * @brief Definition for System features
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

#ifndef    __SYS_FEATURE_H__
#define __SYS_FEATURE_H__

#include "da16x_types.h"
#include "wifi_common_features.h"

#define ENVIRON_DEVICE                 NVEDIT_SFLASH

#define COUNTRY_CODE_DEFAULT           "KR"
#define CHANNEL_AUTO                   0
#define CHANNEL_DEFAULT                CHANNEL_AUTO
#define REPLY_FAIL                    "FAIL"

/* WiFi Direct */
#define WIFI_DIR_LISTEN_CH_DFLT        CHANNEL_AUTO
#define WIFI_DIR_OPERATION_CH_DFLT     CHANNEL_AUTO
#define WIFI_DIR_GO_INTENT_DFLT        3


enum E_SYSMODE {
    E_SYSMODE_STA = 1,
    E_SYSMODE_AP,

#if defined ( __SUPPORT_P2P__ )
    E_SYSMODE_P2P,
    E_SYSMODE_P2P_GO,
#endif    // __SUPPORT_P2P__
    E_SYSMODE_STA_AP,
#if defined ( __SUPPORT_P2P__ )
    E_SYSMODE_STA_P2P,
#endif    // __SUPPORT_P2P__

#if defined ( __SUPPORT_MESH__ )
    E_SYSMODE_MESH_PONIT,
    E_SYSMODE_STA_MESH_PORTAL
#else
    E_SYSMODE_MAX
#endif    // __SUPPORT_MESH__
};

enum E_AUTH_MODE {
    E_AUTH_MODE_NONE = 1,
    E_AUTH_MODE_WEP,
    E_AUTH_MODE_WPA,
    E_AUTH_MODE_RSN,
    E_AUTH_MODE_WPA_RSN,
#ifdef __SUPPORT_WPA3_PERSONAL__
  #ifdef __SUPPORT_WPA3_SAE__
    E_AUTH_MODE_SAE,
    E_AUTH_MODE_RSN_SAE,
  #endif /* __SUPPORT_WPA3_SAE__ */
  #ifdef __SUPPORT_WPA3_OWE__
    E_AUTH_MODE_OWE,
  #endif /* __SUPPORT_WPA3_OWE__ */
#endif // __SUPPORT_WPA3_PERSONAL__

#ifdef __SUPPORT_WPA_ENTERPRISE__
    E_AUTH_MODE_ENT,
  #ifdef __SUPPORT_WPA3_ENTERPRISE__
    E_AUTH_MODE_WPA2_WPA3_ENT,
    E_AUTH_MODE_WPA3_ENT,
    #ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
    E_AUTH_MODE_WPA3_ENT_192B,
    #endif // __SUPPORT_WPA3_ENTERPRISE_192B__
  #endif // __SUPPORT_WPA3_ENTERPRISE__
#endif // __SUPPORT_WPA_ENTERPRISE__
    E_AUTH_MODE_MAX
};

#ifdef __SUPPORT_WPA_ENTERPRISE__
enum E_EAP_AUTH_MODE {
    E_EAP_AUTH_MODE_NONE = 0,
    E_EAP_AUTH_MODE_PEAP_TTLS_FAST,
    E_EAP_AUTH_MODE_PEAP,
    E_EAP_AUTH_MODE_TTLS,
    E_EAP_AUTH_MODE_FAST,
    E_EAP_AUTH_MODE_TLS,
    E_EAP_AUTH_MODE_MAX
};

enum E_EAP_PHASE2_MODE {
    E_EAP_PHASE2_MODE_NONE,
    E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC,
    E_EAP_PHASE2_MODE_MSCHAPV2,
    E_EAP_PHASE2_MODE_GTC,
    E_EAP_PHASE2_MODE_MAX
};
#endif // __SUPPORT_WPA_ENTERPRISE__

enum E_ENCRYP_MODE {
    E_ENCRYP_MODE_TKIP = 1,
    E_ENCRYP_MODE_CCMP,
    E_ENCRYP_MODE_TKIP_CCMP
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
    ,
    E_ENCRYP_MODE_GCMP_256
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
};

enum E_WEP_KEY_TYPE {
    WEP_KEY_TYPE_ASCII = 1,
    WEP_KEY_TYPE_HEXA
};

enum E_WEP_KEY_IDX {
    E_WEP_KEY_IDX_1 = 1,
    E_WEP_KEY_IDX_2,
    E_WEP_KEY_IDX_3,
    E_WEP_KEY_IDX_4,
};

enum E_WEP_KEY_BIT {
    WEP_KEY_64BIT = 1,
    WEP_KEY_128BIT
};

enum E_PMF {
    E_PMF_NONE,        // Diasble
    E_PMF_OPTIONAL,    // Capable
    E_PMF_MANDATORY    // Required
};


/* NET MODE */
enum E_NETMODE {
    E_NETMODE_DYNAMIC_IP = 1,
    E_NETMODE_STATIC_IP
};

/* PSK_KEY_TYPE */
enum E_PSK_KEY_TYPE {
    E_PSK_KEY_ASCII = 1,
    E_PSK_KEY_HEXA
};

enum E_WIFI_MODE {
    E_WIFI_MODE_BGN = 1,
    E_WIFI_MODE_GN,
    E_WIFI_MODE_BG,
    E_WIFI_MODE_N,
    E_WIFI_MODE_G,
    E_WIFI_MODE_B,
};

/* SNTP Client */
enum E_SNTP_CLIENT {
    E_SNTP_CLIENT_STOP = 1,
    E_SNTP_CLIENT_START
};

enum E_CFG_ENABLE{
    E_DISABLE,
    E_ENABLE
};


/* For Console Password ************************************/

#define NVR_KEY_PASSWORD            "PASSWORD"
#define NVR_KEY_PASSWORD_SVC        "PASSWORD_SVC"

#if defined ( __CONSOLE_CONTROL__ )
#define NVR_KEY_DLC_CONSOLE_CTRL    "console_control"
#endif    /* __CONSOLE_CONTROL__ */

/*** !!! Notice !!! ******/
/* Customer can change default Password ... */
#define DEFAULT_PASSWORD            "da16200"
#define PW_TIMEOUT                  12000       /* tick : 2 min */
#define DFLT_PASSWORD_SVC           1


/***********************************************************/


/* External global functions */
extern void da16x_environ_lock(UINT32 flag);
extern void reboot_func(UINT flag);
extern int  chk_dpm_wakeup(void);
extern unsigned int get_firmware_version(UINT fw_type, UCHAR *get_ver);
extern int  console_hidden_check(void);



/* External global variables */
extern unsigned char    zeroconf_support_flag;
extern unsigned char    atcmd_support_flag;
extern unsigned char    ota_support_flag;
extern unsigned char    dhcpd_support_flag;
extern unsigned char    sntp_support_flag;
extern unsigned char    nslookup_support_flag;
extern unsigned char    ipv6_support_flag;
extern unsigned char    factory_rst_btn_flag;
extern unsigned char    wps_btn_flag_flag;
extern unsigned char    ext_wu_sleep_flag;
extern unsigned char    ble_combo_ref_flag;
extern unsigned char    version_chk_flag;


#endif    /* __SYS_FEATURE_H__ */

/* EOF */
