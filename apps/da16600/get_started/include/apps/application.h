/**
 ****************************************************************************************
 *
 * @file application.h
 *
 * @brief Define for System Running Model
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


#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "sdk_type.h"
#include "da16x_types.h"

#include <stdio.h>


/*****************************************************************************
 *    DEVICE
 *****************************************************************************/


/******************************************************************************
 *    NET
 ******************************************************************************/


/******************************************************************************
 * Application threads
 ******************************************************************************/



/* For System and applications */
#define RUN_STA_MODE         0x1
#define RUN_AP_MODE          0x2

#if defined ( __SUPPORT_P2P__ )
#define RUN_P2P_MODE         0x4
#else
#define RUN_P2P_MODE         0x0
#endif // __SUPPORT_P2P__

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#define RUN_STA_SOFTAP_MODE  0x8
#else
#define RUN_STA_SOFTAP_MODE  0x0
#endif // __SUPPORT_WIFI_CONCURRENT__

#define RUN_ALL_MODE_WO_P2P  (RUN_STA_MODE | RUN_AP_MODE | RUN_STA_SOFTAP_MODE)
#define RUN_ALL_MODE_W_P2P   (RUN_STA_MODE | RUN_AP_MODE | RUN_P2P_MODE | RUN_STA_SOFTAP_MODE)

#define RUN_BASIC_MODE       (RUN_STA_MODE | RUN_AP_MODE)
#define RUN_ALL_MODE         RUN_ALL_MODE_WO_P2P

typedef    struct    _app_task_info {
    /// Thread Name
    char    *name;

    /// Funtion Entry_point
    void     (*entry_func)(void *);

    /// Thread Stack Size
    USHORT    stksize;

    /// Thread Priority
    USHORT    priority;

    /// Flag to check network initializing
    UCHAR    net_chk_flag;

    /// Usage flag for DPM running
    UCHAR    dpm_flag;

    /// Port number for network communitation
    USHORT    port_no;

    /// Running mode of DA16xxx
    int    run_sys_mode;
} app_task_info_t;

#include "tcb.h"
typedef    struct    _user_run_task_list_ {
    TaskHandle_t       task_handler;
    app_task_info_t    *info;
    struct _user_run_task_list_    *prev;
} run_app_task_info_t;


/*
 * Application Names ...
 */

/* sys_app_lists[] */
#define APP_INIT_SECURE_MODULE      "secure_module"
#define APP_EXT_WU_MON              "mon_ext_wakeup"
#define APP_ATCMD                   "at_cmd"
#define APP_UART1_MON               "uart1_mon"
#define APP_MQTT_SUB                "mqtt_sub"
#define APP_MQTT_PUB                "mqtt_pub"
#define APP_HTTP_CLIENT             "auto_Http_c"
#define APP_HTTP_SVR                "auto_http_s"
#define APP_HTTPS_SVR               "auto_https_s"
#define APP_ZEROCONF                "zeroconf"
#define APP_AP_TIMER                "SoftAP_timer"


/* customer_app_tbl[] */
#define HELLO_WORLD_1               "helloWorld_1"
#define HELLO_WORLD_2               "helloWorld_2"

#define WIFI_CONN                   "wifi_conn"
#define WIFI_CONN_FAIL              "wifi_conn_fail"
#define WIFI_DISCONN                "wifi_disconn"

#define CUSTOMER_PROVISION          "custom_provision"

#define APP_POLL_STATE              "poll_state"
#define APP_MONITOR                 "monitor_svc"

/* For OTA Update sample */
#define APP_OTA_UPDATE              "ota_update"
#define OTA_UPDATE_MQTT_PORT        1884

#define FAST_SLEEPMODE_MOD          "sleepmode12"

#if defined (__BLE_COMBO_REF__)
#define APP_GTL_INIT                "gtl_init"
#define APP_GTL_MAIN                "gtl_main"
#define APP_GTL_BLE_USR_CMD         "ble_usr_cmd"
#define APP_COMBO_UDP_CLI           "combo_udp_cli"
#endif    /* __BLE_COMBO_REF__ */

#ifdef __SUPPORT_PASSIVE_SCAN__
#define APP_PASSIVE_SCAN            "passive_scan"
#endif /* __SUPPORT_PASSIVE_SCAN__ */


#if defined ( __SUPPORT_DPM_MANAGER__ )
#define TIMER1_ID                    5    /* TIMER_ID 9.10 error */
#define TIMER2_ID                    6
#define TIMER3_ID                    7
#define TIMER4_ID                    8
#endif    /* __SUPPORT_DPM_MANAGER__ */

/*
 * Define RTC Timer ID for customer
 */

#if defined ( __SUPPORT_MQTT__ )
#define MQTT_SUB_CHECK_TIMER_ID      11
#define MQTT_PUB_CHECK_TIMER_ID      12

#define MQTT_SUB_PERIOD_TIMER_ID     14
#define MQTT_PUB_PERIOD_TIMER_ID     15
#endif    /* __SUPPORT_MQTT__ */

#define APP_EC_DOORBELL              "ec_rtsp_svr"
#define EC_RTSP_SVR_PORT             9988


/*
 * Defines for customer's applications
 */

#define TCP_SV_PORT                  5000
#define TCP_SESS_WIN_SZ              4096
#define TCP_PAYLOAD_SZ               640
#define PKT_POOL_CNT                 12
#define TCP_POOL_SIZE                (TCP_PAYLOAD_SZ * PKT_POOL_CNT)

#define TCP_RX_SZ                    4096
#define TCP_LISTEN_MAX_PEND          5



/******************************************************************************
 * External global functions
 ******************************************************************************/

extern int  get_run_mode(void);
extern void start_atcmd(void);
extern int  save_tmp_nvram(void);

#if defined ( __SUPPORT_DPM_EXT_WU_MON__ )
extern void dpm_ext_wu_mon(void *pvParameters);
#endif // __SUPPORT_DPM_EXT_WU_MON__

#if defined ( __ENABLE_AUTO_START_ZEROCONF__ )
extern void zeroconf_auto_start_reg_services(void *params);
#endif    // __ENABLE_AUTO_START_ZEROCONF__

#if defined ( __SUPPORT_ZERO_CONFIG__ )
extern void zeroconf_stop_all_service();
extern void zeroconf_restart_all_service();
#endif // __SUPPORT_ZERO_CONFIG__

#if defined ( __SUPPORT_APMODE_SHUTDOWN_TIMER__ )
static void ap_mode_shutdown_tm(void *pvParameters);
#endif // __SUPPORT_APMODE_SHUTDOWN_TIMER__

#if defined ( __SUPPORT_SIGMA_TEST__ )
extern void sigma_host_init(void);
#endif    // __SUPPORT_SIGMA_TEST__

#if defined ( __SUPPORT_FATFS__ )
extern void init_fs_partition(void);
#endif    // __SUPPORT_FATFS__

#if defined ( __HTTP_SVR_AUTO_START__ )
extern void auto_run_http_svr(void *pvParameters);
#endif // __HTTP_SVR_AUTO_START__

#if defined ( __HTTPS_SVR_AUTO_START__ )
extern void auto_run_https_svr(void *pvParameters);
#endif // __HTTPS_SVR_AUTO_START__

#if defined ( __RUNTIME_CALCULATION__ )
extern void printf_with_run_time(char *string);
#endif    // __RUNTIME_CALCULATION__

#if defined (__ENABLE_SAMPLE_APP__)
extern void register_sample_cb(void);
extern void sample_preconfig(void);
#endif    // __ENABLE_SAMPLE_APP__

#if defined (__BLE_COMBO_REF__)
extern void combo_ble_sw_reset(void);
#endif // __BLE_COMBO_REF__

#endif /* __APPLICATION_H__ */

/* EOF */
