/**
 ****************************************************************************************
 *
 * @file da16x_dpm_abnormal_chk.c
 *
 * @brief APIs and threads related to DPM abnormal state.
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

#define __CHK_NETWORK_TRAFFIC__

#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"
#include "common_def.h"
#include "iface_defs.h"

#include "driver.h"
#include <stdio.h>
#include "application.h"
#include "nvedit.h"
#include "environ.h"
#include "common/defs.h"

#include "os_con.h"
#include "command_net.h"
#include "da16x_time.h"
#include "rtc.h"
#include "da16x_dpm_filter.h"
#include "customer_dpm_features.h"
#include "romac_primitives_patch.h"
#include "dpmrtm_patch.h"

#include "limits.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#undef  DBG_PRINT_INFO
#define DBG_PRINT_ERR
#define DBG_PRINT_TEMP

#ifdef	DBG_PRINT_INFO
#include "lwip/dhcp.h"
#endif //DBG_PRINT_INFO

#pragma GCC diagnostic ignored "-Wcast-function-type"

#define PERIOD_STS_CHK_TIME         100    // 1sec reschedule_ticks
#define DPM_STS_CHK_NAME            "dpm_sts_chk"
#define WIFI_CONN_WAIT_NAME         "DPM_AB_WIFI_CONN_WAIT"
#define DHCP_RSP_WAIT_NAME          "DPM_AB_DHCP_RSP_WAIT"
#define ARP_RSP_WAIT_NAME           "DPM_AB_ARP_RSP_WAIT"
#define UNKNOWN_DPM_FAIL_WAIT_NAME  "DPM_AB_DPM_FAIL_WAIT"
#define WIFI_CONN_RETRY_CNT_NAME    "DPM_AB_WIFI_CONN_RETRY_CNT"

#define MAX_NET_INIT_TIME           60
#define MAX_INIT_WIFI_CONN_TIME     30
#define MAX_DHCP_RENEW_TIME         30
#define MAX_ARP_WAIT_TIME           5
#define MAX_POWER_DOWN_TIME         30
#define MAX_USER_APP_WAIT_TIME      30

#define DPM_ABNORM_ACT_1            1    // WIFI Connect fail
#define DPM_ABNORM_ACT_2            2    // DHCP Renew fail
#define DPM_ABNORM_ACT_3            3    // ARP response fail
#define DPM_ABNORM_ACT_4            4    // DPM Power down fail (30 Sec)
#define DPM_ABNORM_ACT_5            5    // No data for 5 seconds and DPM is ready
#define DPM_ABNORM_ACT_6            6    // DPM application fail



/* Global extern functions */
extern UCHAR *get_dpm_set_task_names(void);
extern int get_netmode(int iface);
extern int get_dpm_mode(void);
extern int check_net_init(int iface);
extern void show_dpm_set_task_names(void);
extern int is_dpm_sleep_ready(void);
extern int dpm_get_wakeup_source(void);
extern int rtc_cancel_timer(int timer_id);
extern int write_nvram_int(const char *name, int val);
extern int read_nvram_int(const char *name, int *_val);
extern bool chk_retention_mem_exist(void);
extern void (*dpmDaemonSleepFunction)();

#ifdef __DA16X_DPM_MON_CLIENT__
extern void start_dpm_monitor_client(int dpm_wu_type);
extern int sendStatusToMonitorServer(UINT status_code,
                                     UINT para0,
                                     UINT para1,
                                     UINT para2,
                                     UINT para3,
                                     UINT para4,
                                     char *paraStr);
#endif    /* __DA16X_DPM_MON_CLIENT__ */

#ifdef __DPM_FINAL_ABNORMAL__
extern int rtc_register_timer(unsigned int msec,
                              char *name,
                              int timer_id,
                              int peri,
                              void (* callback_func)(char *timer_name));
#endif    //__DPM_FINAL_ABNORMAL__

#ifdef BCWU_SLEEP_DOWN_MODE
extern void fc80211_fc9k_sec_pwr_down(unsigned long long usec , bool retention);
#endif    //BCWU_SLEEP_DOWN_MODE

extern void fc80211_da16x_pri_pwr_down(bool retention);
extern void fc80211_da16x_sec_pwr_down(unsigned long long usec, unsigned char retention);
extern unsigned long da16x_get_wakeup_source();
extern UINT get_current_arp_req_status(void);
extern u8 wpa_supp_wps_in_use(void);
extern unsigned int da16x_network_main_check_ip_addr(unsigned char iface);

/* Global external variables */
#ifdef __DA16X_DPM_MON_CLIENT__
extern ULONG    MON_totalRunCount;
extern ULONG    MON_totalRunTick;
extern ULONG    MON_currRunTick;
#endif    //__DA16X_DPM_MON_CLIENT__

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
/* DDPS Related variable and primitive */
extern unsigned char dpm_dynamic_period_setting_flag;
extern char dpm_get_tim_ddps_bufp_chk();
extern void set_dpm_tim_ddps_macflag(char flag);
#endif    /* __SUPPORT_DPM_DYNAMIC_PERIOD_SET__ */

extern void PRINTF_ATCMD(const char *fmt, ...);

extern unsigned char dpm_mng_save_rtm_flag;
extern unsigned char atcmd_dpm_abnorm_msg_flag;
extern unsigned char ble_combo_ref_flag;
extern unsigned char cmd_combo_is_sleep2_triggered;
extern unsigned long long cmd_combo_sleep2_dur;
extern unsigned char dpm_abnormal_disable_flag;

/* Global Local variables */
const unsigned long long dpm_monitor_retry_interval[DPM_MON_RETRY_CNT] = {
    ULLONG_MAX,
    60,
    60,
    60,
    60 * 30,
    60 * 30,
    60 * 30,
    60 * 60,
    60 * 60,
    60 * 60        // 0xDEADBEAF to stop wakeup
};

unsigned long long *dpm_abnorm_user_wakeup_interval = (unsigned long long *)NULL;
const int dpm_monitor_retry_repeat_type = 0;

TimerHandle_t   dpm_sts_chk_tm = NULL;
dpm_monitor_info_t *dpm_monitor_info_ptr = NULL;


#if defined ( __CHK_NETWORK_TRAFFIC__ )
extern unsigned int dpm_get_net_traffic_rx_cnt(void);
extern unsigned int dpm_get_net_traffic_tx_cnt(void);
static ULONG old_dpm_tx_pck_cnt, old_dpm_rx_pck_cnt;
static int idle_time = 0;
#endif    // __CHK_NETWORK_TRAFFIC__

/* Local static variables */
static int dpm_connection_fail_cnt    = 0;
static int dpm_dhcp_no_response_cnt   = 0;
static int dpm_arp_no_response_cnt    = 0;
static int dpm_power_down_fail_cnt    = 0;
static int dpm_unknown_fail_cnt       = 0;
static int dpm_monitor_stopped        = 0;
static int dpm_abnorm_chk_hold_flag   = 0;
static int dpm_abnorm_sleep1_flag     = 0;
static int dpm_wifi_conn_retry_cnt    = 0;

void force_dpm_abnormal_sleep_by_wifi_conn_fail(void)
{
    dpm_connection_fail_cnt = MAX_INIT_WIFI_CONN_TIME-1;
}

void force_dpm_abnormal_sleep1(void)
{
    dpm_abnorm_sleep1_flag = 1;
}

dpm_monitor_info_t *get_dpm_monitor_info_ptr(void)
{
    if (chk_retention_mem_exist() == pdFALSE) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_DPM_MONITOR_PTR;
}

void dpm_monitor_stop(void)
{
    dpm_monitor_stopped = 1;
    PRINTF("[DPM_Mon] STOP\n");
}

void dpm_monitor_restart(void)
{
    dpm_monitor_stopped = 0;
    PRINTF("[DPM_Mon] Restart\n");
}

void dpm_monitor_cleanup(void)
{
    dpm_power_down_fail_cnt = 0;
    dpm_connection_fail_cnt = 0;
    dpm_dhcp_no_response_cnt = 0;
    dpm_arp_no_response_cnt = 0;
    dpm_unknown_fail_cnt = 0;
#if defined ( __CHK_NETWORK_TRAFFIC__ )
    idle_time = 0;
#endif    // __CHK_NETWORK_TRAFFIC__
}

void dpm_show_dpm_monitor_info(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    if (dpm_monitor_info_ptr->wifi_conn_wait_time > 0) {
        PRINTF("DPM_Mon will enter SLEEP Mode-2 after %d secs\n",
                dpm_monitor_info_ptr->wifi_conn_wait_time);
    } else {
        PRINTF("DPM_Mon will enter SLEEP Mode-2 after\n"
                    "5 secs due to ARP no response\n"
                    "60 seds due to DHCP no response\n"
                    "30 secs due to any other reasons.\n");
    }
}

void dpm_show_dpm_monitor_count(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    int total_cnt = 0;

    if (chk_retention_mem_exist () == pdFALSE) {
        return;
    }

    PRINTF("dpm_monitor_info_ptr = 0x%x\n", dpm_monitor_info_ptr);

    PRINTF("\n");
    PRINTF("=========================================================\n");
    PRINTF("Wakeup Type\t TIM Status\t Counts from last power on \n", "Name");
    PRINTF("=========================================================\n");

    total_cnt = dpm_monitor_info_ptr->tim_count[6] + dpm_monitor_info_ptr->tim_count[7] +
            dpm_monitor_info_ptr->tim_count[9] + dpm_monitor_info_ptr->tim_count[11];

    PRINTF("0\t\tDPM_FROM_KEEP_ALIVE\t%d\n", dpm_monitor_info_ptr->tim_count[6]);
    PRINTF("\t\tDPM_NO\t\t\t%d\n", dpm_monitor_info_ptr->tim_count[7]);
    PRINTF("\t\tDPM_AP_RESET\t\t%d\n", dpm_monitor_info_ptr->tim_count[9]);
    PRINTF("(total:%d)\tDPM_DETECTED_STA\t%d\n", total_cnt, dpm_monitor_info_ptr->tim_count[11]);
    PRINTF("---------------------------------------------------------\n");

    PRINTF("1\t\tDPM_FROM_FAST\t\t%d\n", dpm_monitor_info_ptr->tim_count[4]);
    PRINTF("(total:%d)\t\t\t\t\n", dpm_monitor_info_ptr->tim_count[4]);
    PRINTF("---------------------------------------------------------\n");

    total_cnt = dpm_monitor_info_ptr->tim_count[12] + dpm_monitor_info_ptr->tim_count[13];

    PRINTF("2\t\tDPM_USER0\t\t%d\n", dpm_monitor_info_ptr->tim_count[12]);
    PRINTF("(total:%d)\tDPM_USER1\t\t%d\n", total_cnt, dpm_monitor_info_ptr->tim_count[13]);
    PRINTF("---------------------------------------------------------\n");

    total_cnt = dpm_monitor_info_ptr->tim_count[3] + dpm_monitor_info_ptr->tim_count[5] + dpm_monitor_info_ptr->tim_count[10];

    PRINTF("3\t\tDPM_NO_BCN\t\t%d\n", dpm_monitor_info_ptr->tim_count[3]);
    PRINTF("\t\tDPM_KEEP_ALIVE_NO_ACK\t%d\n", dpm_monitor_info_ptr->tim_count[5]);
    PRINTF("(total:%d)\tDPM_DEAUTH\t\t%d\n", total_cnt, dpm_monitor_info_ptr->tim_count[10]);
    PRINTF("---------------------------------------------------------\n");

    total_cnt = dpm_monitor_info_ptr->tim_count[0] + dpm_monitor_info_ptr->tim_count[8];

    PRINTF("4\t\tDPM_UC\t\t\t%d\n", dpm_monitor_info_ptr->tim_count[0]);
    PRINTF("(total:%d)\tDPM_UC_MORE\t\t%d\n", total_cnt, dpm_monitor_info_ptr->tim_count[8]);
    PRINTF("---------------------------------------------------------\n");

    total_cnt = dpm_monitor_info_ptr->tim_count[1] + dpm_monitor_info_ptr->tim_count[2];

    PRINTF("5\t\tDPM_BC_MC\t\t%d\n", dpm_monitor_info_ptr->tim_count[1]);
    PRINTF("(total:%d)\tDPM_BCN_CHANGED\t\t%d\n", total_cnt, dpm_monitor_info_ptr->tim_count[2]);
    PRINTF("=========================================================\n");

    PRINTF("Error Code\t\t\t\t\t\t\n");
    PRINTF("=========================================================\n");
    PRINTF("101(Assoc/Auth Failed)\t\t\t%d\n", dpm_monitor_info_ptr->error_count[1]);
    PRINTF("---------------------------------------------------------\n");
    PRINTF("102(NO Response from DHCP Server)\t%d\n", dpm_monitor_info_ptr->error_count[2]);
    PRINTF("---------------------------------------------------------\n");
    PRINTF("103(No ARP Response)\t\t\t%d\n", dpm_monitor_info_ptr->error_count[3]);
    PRINTF("---------------------------------------------------------\n");
    PRINTF("104(DPM Power Down Failed)\t\t%d\n", dpm_monitor_info_ptr->error_count[4]);
    PRINTF("---------------------------------------------------------\n");
    PRINTF("105(Unknown Error)\t\t\t%d\n", dpm_monitor_info_ptr->error_count[5]);
    PRINTF("---------------------------------------------------------\n");
    PRINTF("106(Application Failed to set DPM)\t%d\n", dpm_monitor_info_ptr->error_count[6]);
    PRINTF("=========================================================\n");
    PRINTF("Last Act\t\t\t\t%d\n", dpm_monitor_info_ptr->last_abnormal_type);
    PRINTF("Last Count\t\t\t\t%d\n", dpm_monitor_info_ptr->last_abnormal_count);

}

void dpm_save_dpm_tim_status(int tim_status)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

#ifdef __DA16X_DPM_MON_CLIENT__
    int tim_count = 0;
#endif    // __DA16X_DPM_MON_CLIENT__

    switch (tim_status) {
    case DPM_UC:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[0]++;
        break;

    case DPM_BC_MC:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[1]++;
        break;

    case DPM_BCN_CHANGED:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[2]++;
        break;

    case DPM_NO_BCN:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[3]++;
        break;

    case DPM_FROM_FAST:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[4]++;
        break;

    case DPM_KEEP_ALIVE_NO_ACK:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[5]++;
        break;

    case DPM_FROM_KEEP_ALIVE:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[6]++;
        break;

    case DPM_NO:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[7]++;
        break;

    case DPM_UC_MORE:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[8]++;
        break;

    case DPM_AP_RESET:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[9]++;
        break;
    case DPM_DEAUTH:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[10]++;
        break;

    case DPM_DETECTED_STA:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[11]++;
        break;

    case DPM_USER0:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[12]++;
        break;

    case DPM_USER1:
#ifdef __DA16X_DPM_MON_CLIENT__
        tim_count =
#endif    // __DA16X_DPM_MON_CLIENT__
        dpm_monitor_info_ptr->tim_count[13]++;
        break;

    default:
        break;
    }

#ifdef __DA16X_DPM_MON_CLIENT__
#ifdef DBG_PRINT_INFO
    PRINTF(">>>>>> [%s] Send to server(%d,%d,%d,%d,%d) \n",
            __func__,
            tim_status,
            tim_count+1,
            MON_totalRunCount,
            MON_currRunTick,
            MON_totalRunTick);
#endif
    sendStatusToMonitorServer(STATUS_WAKEUP, tim_status, tim_count+1, MON_totalRunCount, MON_currRunTick, MON_totalRunTick, "testStatus");
#endif    /* __DA16X_DPM_MON_CLIENT__ */
}

/* error code
 *        101 : Connection Fail
 *        102 : DHCP Fail
 *        103 : ARP Fail
 *        104 : Power Down Fail
 *        105 : Unknown Fail
 */
void dpm_save_dpm_error_code(int act_type)
{
    int count = dpm_monitor_info_ptr->error_count[act_type];
#ifdef __DA16X_DPM_MON_CLIENT__
    UINT error_code = FUNC_ERROR + act_type;
    int tim_status = da16x_get_wakeup_source();
    char *paraString;
#endif    // __DA16X_DPM_MON_CLIENT__

    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    dpm_monitor_info_ptr->error_count[act_type] = count + 1;

#ifdef __DA16X_DPM_MON_CLIENT__
    if (act_type == DPM_ABNORM_ACT_6) {
        paraString = (char *)get_dpm_set_task_names();
    } else {
        paraString = "testError";
    }

#ifdef DBG_PRINT_INFO
    PRINTF(">>>>>> [%s] Send to server(%d,%d,%d) \n",
            __func__,
            error_code,
            tim_status,
            count + 1);
#endif
    sendStatusToMonitorServer(error_code, tim_status, count + 1, 0, 0, 0, paraString);
#endif    /* __DA16X_DPM_MON_CLIENT__ */
}

int dpm_get_wifi_conn_retry_cnt(void)
{
    if (dpm_monitor_info_ptr != NULL) {
        return dpm_monitor_info_ptr->wifi_conn_retry_cnt;
    } else {
        return 0;
    }
}

int dpm_decr_wifi_conn_retry_cnt(void)
{
    return --dpm_wifi_conn_retry_cnt;
}

int set_dpm_abnormal_wait_time(int time, int mode)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    if (mode == DPM_ABNORM_DPM_WIFI_RETRY_CNT) {
        if (time < 0 || time > 6) {
            PRINTF("\nError : out of range [0 .. 6 times]\n\n");
            return -1;
        }
    } else {
        /* Range 10 secs ~ 60 secs */
        if (time < 10 || time > 60) {
            PRINTF("\nError : Wrong setting time [10 .. 60 secs]\n\n");
            return -1;
        }
    }

    /* Permit NVRAM write when first POR boot ...
     * Else case,, it means on DPM operation and block NVRAM Write operation */
    if (chk_dpm_mode() && (chk_dpm_wakeup() == 0)) {
        switch (mode) {
        case DPM_ABNORM_WIFI_CONN_WAIT    :
            write_nvram_int(WIFI_CONN_WAIT_NAME, time);
            break;

        case DPM_ABNORM_DHCP_RSP_WAIT    :
            write_nvram_int(DHCP_RSP_WAIT_NAME, time);
            break;

        case DPM_ABNORM_ARP_RSP_WAIT    :
            write_nvram_int(ARP_RSP_WAIT_NAME, time);
            break;

        case DPM_ABNORM_DPM_FAIL_WAIT    :
            write_nvram_int(UNKNOWN_DPM_FAIL_WAIT_NAME, time);
            break;

        case DPM_ABNORM_DPM_WIFI_RETRY_CNT    :
            write_nvram_int(WIFI_CONN_RETRY_CNT_NAME, time);
            break;
        }
    } else if (!chk_dpm_mode() && mode == DPM_ABNORM_DPM_WIFI_RETRY_CNT) {
        write_nvram_int(WIFI_CONN_RETRY_CNT_NAME, time);
    }

    switch (mode) {
    case DPM_ABNORM_WIFI_CONN_WAIT :
        dpm_monitor_info_ptr->wifi_conn_wait_time = time;
        PRINTF("- Wi-Fi connection waiting time = %d sec\n", time);
        break;

    case DPM_ABNORM_DHCP_RSP_WAIT :
        dpm_monitor_info_ptr->dhcp_rsp_wait_time = time;
        PRINTF("- DHCP response waiting time = %d sec\n", time);
        break;

    case DPM_ABNORM_ARP_RSP_WAIT :
        dpm_monitor_info_ptr->arp_rsp_wait_time = time;
        PRINTF("- ARP response waiting time = %d sec\n", time);
        break;

    case DPM_ABNORM_DPM_FAIL_WAIT :
        dpm_monitor_info_ptr->unknown_dpm_fail_wait_time = time;
        PRINTF("- DPM fail waiting time = %d sec\n", time);
        break;

    case DPM_ABNORM_DPM_WIFI_RETRY_CNT :
        dpm_wifi_conn_retry_cnt = time;

        if (dpm_monitor_info_ptr != NULL) {
            dpm_monitor_info_ptr->wifi_conn_retry_cnt = time;
        }

        if (dpm_wifi_conn_retry_cnt == 0) {
            PRINTF("- DPM wifi conn retry count = default action \n");
        } else {
            PRINTF("- DPM wifi conn retry count = %d time(s)\n", dpm_wifi_conn_retry_cnt);
        }
        break;

    default :
        break;
    }

    return 0;
}

UCHAR get_last_abnormal_act(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    return dpm_monitor_info_ptr->last_abnormal_type;
}

#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
/* static */ UCHAR get_last_abnormal_cnt(void)
#else
static UCHAR get_last_abnormal_cnt(void)
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    return dpm_monitor_info_ptr->last_abnormal_count;
}

#ifdef __DPM_FINAL_ABNORMAL__
int get_final_abnormal_rtc_tid(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    return dpm_monitor_info_ptr->final_abnormal_tid;
}

static void set_final_abnormal_rtc_tid(int tid)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    dpm_monitor_info_ptr->final_abnormal_tid = (UCHAR) tid;
}
#endif /* __DPM_FINAL_ABNORMAL__ */

static void save_abnormal_act(UCHAR act, UCHAR cnt)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    dpm_monitor_info_ptr->last_abnormal_type = act;
    dpm_monitor_info_ptr->last_abnormal_count = cnt;

#ifdef DBG_PRINT_INFO
    PRINTF(RED_COL "[%s] act = %d cnt = %d\n" CLR_COL, __func__, act, cnt);
#endif
}


void save_dpm_sleep_type(UCHAR type)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    dpm_monitor_info_ptr->last_sleep_type = type;
}

UCHAR get_last_dpm_sleep_type(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    return dpm_monitor_info_ptr->last_sleep_type;
}

#ifdef __DPM_FINAL_ABNORMAL__

#define FINAL_ABNORMAL_TIME    (3600*24) * 1000 /* mec */

static UINT get_final_abnormal_time(void)
{
    return FINAL_ABNORMAL_TIME;
}

static void final_abnormal_act(char *timer_name)
{
    /* Final action */
    PRINTF("The final action is not defined.\n");
}

#endif    /* __DPM_FINAL_ABNORMAL__ */

/*
 * In this function, most of the print/PRINTF/lowPrintf does not run normally
 * because the DA16200 enter to Sleep Mode 2 before finishing print operation.
 *
 * If wants to shown print log message on console,
 * fc80211_fc9k_sec_pwr_down() should be blocked.
 * When a print statement is entered, the output is reduced due to processing delay.
 * vTaskDelay() does not work because it is a function called by the timer.
 * static void dpm_abnormal_next_action(int act_type)
 */
static void dpm_abnormal_next_action(int act_type)
{
    UCHAR last_cnt = 0;
    UCHAR current_cnt = 0;

    unsigned long long uTimeValue;

    /*
     * To printout New-Line character
     * Don't printout a long message.
     * The power will be down in short time
     * ( It is not suffician to pritnout for long message. )
     */
    PRINTF("\n");

    last_cnt = get_last_abnormal_cnt(); /* Total count of consecutive Abrnomal actions */

    if (   dpm_get_wakeup_source() == WAKEUP_EXT_SIG_WITH_RETENTION
        && rtc_timer_info(TID_ABNORMAL) > MAX_INIT_WIFI_CONN_TIME) {
        /* Fix Abnormal event count by External Wakeup button event */
        current_cnt = last_cnt;
    } else {
        current_cnt = last_cnt + 1;
    }

    if (current_cnt == DPM_MON_RETRY_CNT) {
        if (dpm_monitor_retry_repeat_type == 0) {      // repeat last one
            current_cnt--;
        } else if (dpm_monitor_retry_repeat_type == 1) { // go back to first one
            current_cnt = 0;
        }
    }

    save_abnormal_act(act_type, current_cnt);
    dpm_save_dpm_error_code(act_type);

    if (act_type == DPM_ABNORM_ACT_6) { //User application error
#ifdef DBG_PRINT_INFO
        PRINTF(YEL_COL "!!! Unable to enter DPM Sleep Mode !!! Please check following threads.\n" CLR_COL);
#endif
        show_dpm_set_task_names();
    } else {

        /* All DPM Module is set, so power-down will be started.
         * During Aging , no wakeup issue happed */

        /* Need to hold the DPM daemon operation to handle abnormal sequences */
        hold_dpm_sleepd();

        save_dpm_sleep_type(1);
#ifdef DBG_PRINT_INFO
        PRINTF("Abnormal DPM operation.");
#endif /* DBG_PRINT_INFO */

        if (   dpm_get_wakeup_source() == WAKEUP_EXT_SIG_WITH_RETENTION
            && rtc_timer_info(TID_ABNORMAL) > MAX_INIT_WIFI_CONN_TIME) { /* Correct wakeup count when Wakeup-Button */
            uTimeValue = (rtc_timer_info(TID_ABNORMAL) - MAX_INIT_WIFI_CONN_TIME) * 1000000ULL;
        } else {
            
            if (cmd_combo_is_sleep2_triggered == pdTRUE) {
                uTimeValue = cmd_combo_sleep2_dur * 1000000ULL;
            } else {
                if (dpm_abnorm_user_wakeup_interval == NULL) {
                    /* Use default wakeup interval */
                    uTimeValue = dpm_monitor_retry_interval[current_cnt] * 1000000ULL;

                    /* Abnormal Setting Time should be checked one more */
                    if (uTimeValue < 1000000ULL || uTimeValue > 60 * 60 *1000000ULL) {
                        PRINTF("Abnormal Time value (cnt %d) is wrong, so set as last abnormal time (60 min)\n", current_cnt);
                        uTimeValue = (unsigned long long)(60 * 60 * 1000000ULL);
                    }
                } else {
                    uTimeValue = dpm_abnorm_user_wakeup_interval[current_cnt] * 1000000ULL;

                    /* rtc uses 32khz clock and 36 bits wide register, 
                            so a value until 0x1FFFFF sec (== 24 days) is allowed */ 
                    if (uTimeValue < 1000000ULL || uTimeValue > 0x1FFFFF * 1000000ULL) {
                        // overflow ...
                        if (uTimeValue / 1000000ULL != 0xDEADBEAF) { // skip for sleep1 condition
                            PRINTF("Abnormal Time value (cnt %d) is wrong, so set as (60 min) \n", current_cnt);
                            uTimeValue = (unsigned long long)(60ULL * 60ULL * 1000000ULL);
                        }
                    }
                }
            }
        }

        if (   current_cnt == 1
            && chk_supp_connected() == pdFAIL
            && dpm_get_wakeup_source() != WAKEUP_EXT_SIG_WITH_RETENTION) {
#ifdef __DPM_FINAL_ABNORMAL__
            int tid = 0;

            tid = rtc_register_timer(get_final_abnormal_time(), "", 0, 0, final_abnormal_act);

            if (tid > 0) {
                set_final_abnormal_rtc_tid(tid);
            } else {
#ifdef DBG_PRINT_ERROR
                PRINTF("[RTC] Timer registration failed (final_abnormal_rtc_tid). \n");
#endif /* DBG_PRINT_ERROR */
            }
#endif /* __DPM_FINAL_ABNORMAL__ */

#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
            if (rtc_timer_info(TID_DHCP_CLIENT) > 0) {
                rtc_cancel_timer(TID_DHCP_CLIENT); // Delete the DHCP Client RTC Timer
            }
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */
        }

#ifdef DBG_PRINT_ERROR
        PRINTF(" ## %d \n", act_type);
#endif

        if (dpm_mng_save_rtm_flag) {
            if (dpmDaemonSleepFunction) {
#ifdef DBG_PRINT_INFO
                PRINTF(" Call register function !!!\n", __func__);
#endif
                dpmDaemonSleepFunction();
            }
        }

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
        /* POR / RESET / REBOOT , For DPM TIM WAKEUP TIME is set automatically by TIM BUFP */ 
        if (dpm_dynamic_period_setting_flag) {
            if ((dpm_get_wakeup_source() == WAKEUP_SOURCE_POR) || (dpm_get_wakeup_source() == WAKEUP_RESET)) {
                if (dpm_get_tim_ddps_bufp_chk()) {
                    set_dpm_tim_ddps_macflag(1);
                }
            }
        }    
#endif
        /* level 1 : Delete only RTM data
         * level 2 : Delete User RTC Timer and RTM data
         */
#if !defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
        dpm_user_timer_delete_all(2);
#endif    // ! __SUPPORT_DPM_ABN_RTC_TIMER_WU__

        extern void combo_make_sleep_rdy(void);
        if (ble_combo_ref_flag == pdTRUE) {
            combo_make_sleep_rdy();
        }

        /* Power down */
        if (dpm_abnorm_sleep1_flag == TRUE) {
            fc80211_da16x_pri_pwr_down(TRUE);
        } else {
            if (uTimeValue / 1000000ULL == 0xDEADBEAF) { // sleep1 power down
                fc80211_da16x_pri_pwr_down(TRUE);
            } else { // sleep2 pwr down
                fc80211_da16x_sec_pwr_down(uTimeValue, TRUE);
            }
        }
    }
}

/* dpm_wu_type 0 : normal power on reboot
 *    1 : RTC wakeup
 *    2 : user wakeup
 *    3 : no beacon / no ack / deauth
 *    4 : uc wakeup
 *    5 : beacon changed / bc_mc
 *    other: unknown
 */
static void dpm_sts_chk_tm_fn(ULONG dpm_wu_type)
{
	DA16X_UNUSED_ARG(dpm_wu_type);

    int    act_type;
#ifdef __CHK_NETWORK_TRAFFIC__
    int dpm_traffic_exist = FALSE;
#endif
    int wait_time;
#ifdef __PRE_NOTIFY_ABNORMAL__
    int notify_time = 0;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    extern void atcmd_wf_jap_print_with_cause(int cause);

    if (get_dpm_mode() == 0) {
        return;
    }

    if (dpm_abnorm_chk_hold_flag) {
        return;
    }

    if (chk_dpm_sleepd_hold_state() || dpm_monitor_stopped) {
        dpm_monitor_cleanup();
        return;
    }

#ifdef DBG_PRINT_INFO
    PRINTF("[%s] Called Timer Function : dpm_wu_type=%d\n", __func__, dpm_wu_type);
#endif
    /* Check the Wi-Fi connection state ... */
    if (chk_supp_connected() == 0) {
        goto chk_wifi_conn_fail;
    } else {
        char    *block_dpm_mod_name;

        /* In case of Wi-Fi connection okay,,, */
        block_dpm_mod_name = (char *)get_dpm_set_task_names();

        /* If the Disconnect Loss module and DPM KEY module is failed, go abnormal power down*/
        if (   (block_dpm_mod_name != NULL)
            && (strcmp("DPM_KEY", block_dpm_mod_name) == 0 || strcmp("Disconnect_loss", block_dpm_mod_name) == 0)) {
            goto chk_wifi_conn_fail;
        }
    }

    /* Check DHCPC renew operation */
    if ((get_netmode(WLAN0_IFACE) == DHCPCLIENT) && (da16x_network_main_check_ip_addr(WLAN0_IFACE) == pdFALSE)) {
#ifdef DBG_PRINT_INFO
    	struct netif *netif = netif_get_by_index(WLAN0_IFACE+2);

    	if(netif != NULL) {
            PRINTF("--- [%s] wu_type=%d, dhcpc_state_changes=%d\n",
                    __func__,
                    dpm_wu_type,
    				dhcp_get_state(netif));
    	}
#endif
        goto chk_dhcpc_state;
    }

    /* Check if ARP Request is sent and no response received yet. */
    if (get_current_arp_req_status()) {
#ifdef DBG_PRINT_INFO
        PRINTF("--- [%s] wu_type=%d, arp request sent. \n", __func__, dpm_wu_type);
#endif
        goto chk_arp_state;
    }

    /* Check DPM Power down fail state */
    if (chk_dpm_pdown_start() == DONE_DPM_SLEEP) {
#ifdef DBG_PRINT_INFO
        PRINTF("--- [%s] Power Down Failed \n", __func__);
#endif
        goto chk_dpm_power_down_fail_state;
    }

    /* Unknown power down fail */
#if defined ( __CHK_NETWORK_TRAFFIC__ )
    if (   (old_dpm_tx_pck_cnt != dpm_get_net_traffic_tx_cnt())
        || (old_dpm_rx_pck_cnt != dpm_get_net_traffic_rx_cnt())) {
        dpm_traffic_exist = TRUE;
    }

    /* Save current DPM Data packet Traffic */
    old_dpm_tx_pck_cnt = dpm_get_net_traffic_tx_cnt();
    old_dpm_rx_pck_cnt = dpm_get_net_traffic_rx_cnt();

    //no traffic for idle_time
    if (dpm_traffic_exist == TRUE) {
        idle_time = 0;
    } else {
        idle_time++;
#ifdef DBG_PRINT_INFO
        //PRINTF("idle_time: %d\n", idle_time);
#endif
    }

#ifndef TEST_DPM_ABNORM_ERROR_105
    if (   idle_time > 30
        && !is_dpm_sleep_ready()) { //No data for 30 seconds and some application has not set DPM bit.
        goto user_dpm_fail_state;
    } else if (idle_time > 5 && is_dpm_sleep_ready()) { //No data for 5 seconds and DPM is ready
        goto chk_unknown_dpm_fail_state;
    }
#else
    if (idle_time > 5) { //No data for 5 seconds and DPM is ready
        goto chk_unknown_dpm_fail_state;
    }
#endif
#endif    // __CHK_NETWORK_TRAFFIC__

    goto no_action;


chk_wifi_conn_fail :
    if (wpa_supp_wps_in_use()) {
        goto no_action;
    }

    if (dpm_monitor_info_ptr->wifi_conn_wait_time > 0) {
        wait_time = dpm_monitor_info_ptr->wifi_conn_wait_time;
    } else {
        wait_time = MAX_INIT_WIFI_CONN_TIME;
    }

#ifdef __PRE_NOTIFY_ABNORMAL__
    notify_time = (wait_time * 3) / 4;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    if (dpm_connection_fail_cnt++ < wait_time) {
        if (dpm_connection_fail_cnt == wait_time) {
            if ( atcmd_dpm_abnorm_msg_flag == pdTRUE ) {
                if (get_supp_conn_state() >= WPA_SCANNING) {
                    atcmd_wf_jap_print_with_cause(0);
                }
                PRINTF_ATCMD("\r\n+DPM_ABNORM_SLEEP\r\n");
            }
            if (dpm_abnorm_sleep1_flag == pdTRUE) {
                PRINTF(RED_COL ">> Abnormal DPM(%d) Sleep1 after 1 second. \r\n" CLR_COL, DPM_ABNORM_ACT_1);
            } else {
                if (!cmd_combo_is_sleep2_triggered) {
                    PRINTF(RED_COL ">> Abnormal DPM(%d) operation after 1 second.\r\n" CLR_COL, DPM_ABNORM_ACT_1);
                }
            }
        }

#ifdef __PRE_NOTIFY_ABNORMAL__
        if (dpm_connection_fail_cnt == notify_time) {
            PRINTF(CYN_COL ">> Notify Abnormal Status(condition:%d, remain:%d) \r\n" CLR_COL, DPM_ABNORM_ACT_1, (wait_time-notify_time));
            notifyAbnormalBeforeDeepSleep(DPM_ABNORM_ACT_1, wait_time-notify_time);
        }
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

        /* just return because it's periodic timer function. */
        return;
    } else {
        /* Need to check "ssid->auth_failures" what to do something. */
        // Check ... the operation ... wpas_auth_failed() ...

        act_type = DPM_ABNORM_ACT_1;    // WIFI Connect fail
        goto next_action_for_dpm_abnormal;
    }


chk_dhcpc_state :
    /* dhcp client renew */
#if 0    // Need to implement additionally if needs
    if (dpm_monitor_info_ptr->dhcp_rsp_wait_time > 0)
        wait_time = dpm_monitor_info_ptr->dhcp_rsp_wait_time;
    else
#endif    // 0
        wait_time = MAX_DHCP_RENEW_TIME;

#ifdef __PRE_NOTIFY_ABNORMAL__
    notify_time = (wait_time * 3) / 4;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    if (dpm_dhcp_no_response_cnt++ < wait_time) {
        /* just return because it's periodic timer function. */
        if (dpm_dhcp_no_response_cnt == wait_time) {
            PRINTF(RED_COL ">> Abnormal DPM(%d) operation after 1 second.\r\n" CLR_COL, DPM_ABNORM_ACT_2);
        }

#ifdef __PRE_NOTIFY_ABNORMAL__
        if (dpm_dhcp_no_response_cnt == notify_time) {
            PRINTF(CYN_COL ">> Notify Abnormal Status(condition:%d, remain:%d) \r\n" CLR_COL, DPM_ABNORM_ACT_2, (wait_time-notify_time));
            notifyAbnormalBeforeDeepSleep(DPM_ABNORM_ACT_2, wait_time-notify_time);
        }
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

        return;
    } else {
        act_type = DPM_ABNORM_ACT_2;    // DHCP Renew fail
        goto next_action_for_dpm_abnormal;
    }


chk_arp_state:
#if 0    // Need to implement additionally if needs
    if (dpm_monitor_info_ptr->arp_rsp_wait_time > 0)
        wait_time = dpm_monitor_info_ptr->arp_rsp_wait_time;
    else
#endif    // 0
        wait_time = MAX_ARP_WAIT_TIME;

#ifdef __PRE_NOTIFY_ABNORMAL__
    notify_time = (wait_time * 3) / 4;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    if (dpm_arp_no_response_cnt++ < wait_time) {
        /* just return because it's periodic timer function. */
        if (dpm_arp_no_response_cnt == wait_time) {
            if ( atcmd_dpm_abnorm_msg_flag == pdTRUE ) {
                if (get_supp_conn_state() >= WPA_SCANNING) {
                    atcmd_wf_jap_print_with_cause(3);
                }
                PRINTF_ATCMD("\r\n+DPM_ABNORM_SLEEP\r\n");
            }
            PRINTF(RED_COL ">> Abnormal DPM(%d) operation after 1 second.\r\n" CLR_COL, DPM_ABNORM_ACT_3);
        }

#ifdef __PRE_NOTIFY_ABNORMAL__
        if (dpm_arp_no_response_cnt == notify_time) {
            PRINTF(CYN_COL ">> Notify Abnormal Status(condition:%d, remain:%d) \r\n" CLR_COL, DPM_ABNORM_ACT_3, (wait_time-notify_time));
            notifyAbnormalBeforeDeepSleep(DPM_ABNORM_ACT_3, wait_time-notify_time);
        }
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

        return;
    } else {
        act_type = DPM_ABNORM_ACT_3;        // ARP response fail
        goto next_action_for_dpm_abnormal;
    }

chk_dpm_power_down_fail_state :
    // Need to implement additionally
    // if needs to change waiting time for DPM Power-down sequence
    wait_time = MAX_POWER_DOWN_TIME;

#ifdef __PRE_NOTIFY_ABNORMAL__
    notify_time = (wait_time * 3) / 4;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    if (dpm_power_down_fail_cnt++ < wait_time) {
        /* just return because it's periodic timer function. */
        if (dpm_power_down_fail_cnt == wait_time) {
            PRINTF(RED_COL ">> Abnormal DPM(%d) operation after 1 second.\r\n" CLR_COL, DPM_ABNORM_ACT_4);
        }

#ifdef __PRE_NOTIFY_ABNORMAL__
        if (dpm_power_down_fail_cnt == notify_time) {
            PRINTF(CYN_COL ">> Notify Abnormal Status(condition:%d, remain:%d) \r\n" CLR_COL, DPM_ABNORM_ACT_4, (wait_time-notify_time));
            notifyAbnormalBeforeDeepSleep(DPM_ABNORM_ACT_4, wait_time-notify_time);
        }
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

        return;
    } else {
        act_type = DPM_ABNORM_ACT_4;    // DPM Power down fail (30 Sec)
        goto next_action_for_dpm_abnormal;
    }


user_dpm_fail_state :
    act_type = DPM_ABNORM_ACT_6;    // DPM application fail
    goto next_action_for_dpm_abnormal;


chk_unknown_dpm_fail_state :
#if 0    // Need to implement additionally if needs
    if (dpm_monitor_info_ptr->unknown_dpm_fail_wait_time > 0)
        wait_time = dpm_monitor_info_ptr->unknown_dpm_fail_wait_time;
    else
#endif    // 0
        wait_time = MAX_POWER_DOWN_TIME;

#ifdef __PRE_NOTIFY_ABNORMAL__
    notify_time = (wait_time * 3) / 4;
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

    if (dpm_unknown_fail_cnt++ < wait_time) {
        /* just return because it's periodic timer function. */
        if (dpm_unknown_fail_cnt == wait_time) {
            PRINTF(RED_COL ">> Abnormal DPM(%d) operation after 1 second.\r\n" CLR_COL, DPM_ABNORM_ACT_5);
        }

#ifdef __PRE_NOTIFY_ABNORMAL__
        if (dpm_unknown_fail_cnt == notify_time) {
            PRINTF(CYN_COL ">> Notify Abnormal Status(condition:%d, remain:%d) \r\n" CLR_COL, DPM_ABNORM_ACT_5, (wait_time-notify_time));
            notifyAbnormalBeforeDeepSleep(DPM_ABNORM_ACT_5, wait_time-notify_time);
        }
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

        return;
    } else {
        act_type = DPM_ABNORM_ACT_5;    // No data for 5 seconds and DPM is ready
        goto next_action_for_dpm_abnormal;
    }

next_action_for_dpm_abnormal :
    dpm_monitor_cleanup();
    dpm_abnormal_next_action(act_type);

no_action :
    return;
}

#ifdef __PRE_NOTIFY_ABNORMAL__
void setDeepSleepWaitTime(int time)
{
    PRINTF(CYN_COL ">> Set delay_time to Abnormal %d Sec \r\n" CLR_COL, time);

    dpm_monitor_info_ptr->wifi_conn_wait_time = time;

    dpm_power_down_fail_cnt = 0;
    dpm_connection_fail_cnt = 0;
    dpm_dhcp_no_response_cnt = 0;
    dpm_arp_no_response_cnt = 0;
    dpm_unknown_fail_cnt = 0;
    idle_time = 0;
}
#endif  /* __PRE_NOTIFY_ABNORMAL__ */

void start_dpm_sts_chk_timer(int dpm_wu_type)
{
    DA16X_UNUSED_ARG(dpm_wu_type);

    int sys_wdog_id = da16x_sys_wdog_id_get_dpm_sleep_daemon();
    int wifi_conn_wait_time = 0;
    int dhcp_rsp_wait_time = 0;
    int arp_rsp_wait_time = 0;
    int unknown_dpm_fail_wait_time = 0;
    int wifi_conn_retry_cnt = 0;
    int dpm_abnormal_stop_flag = 0;

    /* RTM_DPM_MONITOR_PTR */
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    /* Check the network running state
     * Wait during defined checking time : 10 sec */
#define WLAN0_IFACE    0
    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    while (check_net_init(WLAN0_IFACE) != pdPASS) {
        vTaskDelay(10);        // 100 msec
    }

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    if (get_last_dpm_sleep_type() == 0) {
#ifdef DBG_PRINT_INFO
        PRINTF(CYN_COL "Reset Last abnormal count.\n" CLR_COL);
#endif
        save_abnormal_act(0, 0);
    }

#ifdef __DA16X_DPM_MON_CLIENT__
    start_dpm_monitor_client(dpm_wu_type);
#endif    // __DA16X_DPM_MON_CLIENT__

#ifdef DBG_PRINT_INFO
    PRINTF(CYN_COL "--- [%s] dpm_wu_type=%d, tim status=%d\n" CLR_COL,
                    __func__, dpm_wu_type, da16x_get_wakeup_source());
#endif

    dpm_save_dpm_tim_status(da16x_get_wakeup_source());

    /* Permit NVRAM write when first POR boot ...
     * Else case,, it means on DPM operation and block NVRAM Write operation */
    if (chk_dpm_mode() && (chk_dpm_wakeup() == 0)) {
        /* restore saved wifi_conn_wait_time from NVRAM */
        read_nvram_int(WIFI_CONN_WAIT_NAME, &wifi_conn_wait_time);
        if (wifi_conn_wait_time > 0) {
            dpm_monitor_info_ptr->wifi_conn_wait_time = wifi_conn_wait_time;
        }

        /* restore dpm abnormal wifi conn retry count from NVRAM */
        read_nvram_int(WIFI_CONN_RETRY_CNT_NAME, &wifi_conn_retry_cnt);
        if (wifi_conn_retry_cnt > 0) {
            dpm_wifi_conn_retry_cnt = dpm_monitor_info_ptr->wifi_conn_retry_cnt = wifi_conn_retry_cnt;
        } else {
            dpm_wifi_conn_retry_cnt = dpm_monitor_info_ptr->wifi_conn_retry_cnt = 0;
        }

        /* restore saved dhcp_rsp_wait_time from NVRAM */
        read_nvram_int(DHCP_RSP_WAIT_NAME, &dhcp_rsp_wait_time);
        if (dhcp_rsp_wait_time > 0) {
            dpm_monitor_info_ptr->dhcp_rsp_wait_time = dhcp_rsp_wait_time;
        }

        /* restore saved arp_rsp_wait_time from NVRAM */
        read_nvram_int(ARP_RSP_WAIT_NAME, &arp_rsp_wait_time);
        if (arp_rsp_wait_time > 0) {
            dpm_monitor_info_ptr->arp_rsp_wait_time = arp_rsp_wait_time;
        }

        /* restore saved unknown_dpm_fail_wait_time from NVRAM */
        read_nvram_int(UNKNOWN_DPM_FAIL_WAIT_NAME, &unknown_dpm_fail_wait_time);
        if (unknown_dpm_fail_wait_time > 0) {
            dpm_monitor_info_ptr->unknown_dpm_fail_wait_time = unknown_dpm_fail_wait_time;
        }
    } else if (chk_dpm_mode() && (chk_dpm_wakeup() == 1)) {
        dpm_wifi_conn_retry_cnt = dpm_monitor_info_ptr->wifi_conn_retry_cnt;
    }

    /* Create timer to check the dpm running abnormal status. */
    dpm_sts_chk_tm = xTimerCreate(DPM_STS_CHK_NAME,
                                PERIOD_STS_CHK_TIME,
                                pdTRUE,
                                (void*) 0,
                                (TimerCallbackFunction_t) dpm_sts_chk_tm_fn);

    /* Start DPM-abnormal status check timer */
    xTimerStart(dpm_sts_chk_tm, PERIOD_STS_CHK_TIME);

    /* Check if exist DPM-abnormal stop flag in NVRAM */
    if (dpm_abnormal_disable_flag == pdFALSE) {
        if (read_nvram_int(NVR_KEY_DPM_ABNORM_STOP, &dpm_abnormal_stop_flag) == 0) {
            if (dpm_abnormal_stop_flag == 1) {
                xTimerStop(dpm_sts_chk_tm, 10);
            }
        }
    }
}

void dpm_abnormal_chk_hold(void)
{
    dpm_abnorm_chk_hold_flag = 1;
#ifdef DBG_PRINT_INFO
    PRINTF(CYN_COL "--- [%s] Holding DPM Abnormal check \n" CLR_COL, __func__);
#endif
}

void dpm_abnormal_chk_resume(void)
{
    dpm_abnorm_chk_hold_flag = 0;
#ifdef DBG_PRINT_INFO
    PRINTF(CYN_COL "--- [%s] Resume DPM Abnormal check \n" CLR_COL, __func__);
#endif
}

char config_dpm_abnormal_timer(char flag)
{
    if (dpm_abnormal_disable_flag == pdTRUE) {
        return pdFAIL;
    }

    /* Timer does not created yet. */
    if (dpm_sts_chk_tm == NULL) {
        return pdFAIL;
    }

    if (flag == pdTRUE) {
        /* Start DPM abnormal check timer */
        xTimerStart(dpm_sts_chk_tm, PERIOD_STS_CHK_TIME);
    } else {
        /* Stop DPM abnormal check timer */
        xTimerStop(dpm_sts_chk_tm, 10);
    }

    return pdTRUE;
}

#ifdef __CHECK_BADCONDITION_WAKEUP__

#undef  BCWU_AUTO_MODE
#undef  BCWU_SLEEP_DOWN_MODE

#define CHECK_BADCONDITION_COUNT        1        /* 1 times */
#define CHECK_BADCONDITION_MARGIN       (10 * 1000)    /* 10 Sec */

#define NVR_KEY_BCWU_RX_ON_TIME         "rx_on_time"
#define NVR_KEY_BCWU_BCN_LOSS_CNT       "bcn_loss_count"

#define TIMP_PERIOD_INF    0xffffffff

const unsigned long long dpm_bcwu_retry_interval[10] = {-1, 60*30, 60*30, 60*30, 60*60, 60*60, 60*60, 60*60, 60*60, 60*60};

extern struct timp_data_tag *timp_get_interface_current(enum TIMP_INTERFACE interface);
extern struct timp_data_tag *timp_get_condition(enum TIMP_INTERFACE interface);
extern void timp_clear();

unsigned long   BCWU_CheckPeriod;
unsigned long   BCWU_RxOnTime;
unsigned short  BCWU_BcnLossCount;

UINT isBadConditionWakeup(void)
{
#ifdef BCWU_AUTO_MODE
    int tim_status = fc9k_get_wakeup_source();

    if ((tim_status == DPM_WEAK_SIGNAL_0) || (tim_status == DPM_WEAK_SIGNAL_1)) {
#ifdef DBG_PRINT_INFO
        PRINTF(YELLOW_COLOR " [%s] Bad Condition Wakeup 0x%0x \n" CLEAR_COLOR, __func__, tim_status);
#endif
        return 1;
    } else {
#ifdef DBG_PRINT_INFO
        PRINTF(YELLOW_COLOR " [%s] Not bad Condition Wakeup 0x%0x \n" CLEAR_COLOR, __func__, tim_status);
#endif
        return 0;
    }

#else    /* BCWU_AUTO_MODE */

    struct    timp_data_tag *tagPtrCond = timp_get_condition(TIMP_INTERFACE_0);
    struct    timp_data_tag *tagPtrCurr = timp_get_interface_current(TIMP_INTERFACE_0);
    enum    TIMP_UPDATED_STATUS status = timp_check_cond(TIMP_INTERFACE_0, tagPtrCond, tagPtrCurr);

#ifdef DBG_PRINT_TEMP
    {
        unsigned long ontimeCondition = tagPtrCond->rx_on_time_tu;
        unsigned long ontimeCurrent = tagPtrCurr->rx_on_time_tu;
        unsigned short bcnTimeOutCntCondition = tagPtrCond->bcn_timeout_count;
        unsigned short bcnTimeOutCntCurrent = tagPtrCurr->bcn_timeout_count;

        PRINTF(YELLOW_COLOR " [%s] Condition tag=%x(sts=%d) rx_on_time(%d,%d) bcn_timeout_count(%d,%d) \n" CLEAR_COLOR,
               __func__, tagPtrCurr, status,
               ontimeCondition, ontimeCurrent,
               bcnTimeOutCntCondition, bcnTimeOutCntCurrent);
    }
#endif
    timp_clear();

    if (status) {
        return 1;    /* bad condition wakeup */
    } else {
        return 0;    /* normal */
    }

#endif    /* BCWU_AUTO_MODE */

}

struct timp_data_tag timp_data = {
    0,        /* unsigned long  tim_count */
    0,        /* unsigned long  rx_on_time_tu */
    0,        /* unsigned long  bcn_rx_count */
    0,        /* unsigned long  bcn_timeout_count */
    0,        /* unsigned short connection_loss_count */
    0,        /* unsigned short deauth_frame_count */
};

void initBadConditionWakeup(unsigned long period, int setup_mode)
{
    struct timp_data_tag *data_ptr = &timp_data;

    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    dpm_monitor_info_ptr->firstBadConditionRTC = 0;
    dpm_monitor_info_ptr->badConditionCount = 0;
    dpm_monitor_info_ptr->badConditionStep = 0;

    BCWU_CheckPeriod = period;

    if (read_nvram_int((const char *)NVR_KEY_BCWU_RX_ON_TIME, (int *)&BCWU_RxOnTime)) {
        BCWU_RxOnTime = (BCWU_CheckPeriod * 60 / 12);    /* 300 uA */
    }

    if (read_nvram_int((const char *)NVR_KEY_BCWU_BCN_LOSS_CNT, (int *)&BCWU_BcnLossCount)) {
        BCWU_BcnLossCount = (BCWU_CheckPeriod * 3) / 10;
    }

#ifdef BCWU_AUTO_MODE
    timp_set_period(TIMP_INTERFACE_0, BCWU_CheckPeriod*1000);
#else    /* BCWU_AUTO_MODE */
    timp_set_period(TIMP_INTERFACE_0, TIMP_PERIOD_INF);
#endif    /* BCWU_AUTO_MODE */

    if (setup_mode) {
        long a, b;
        unsigned long tim_period = dpm_sche_get_interval(GET_DPMP(), dpm_get_env_tim_period_ty(GET_DPMP()));

        a = FC9K_DPM_TIM_TBTT_DELAY_OFF * 3;
        b = (BCWU_CheckPeriod * 1000) / (tim_period * dpm_get_env_bcn_int(GET_DPMP()) * 1.024);
        b *= FC9K_DPM_AP_SYNC_READY_TIME;

        BCWU_RxOnTime += US2TU(a) + b;
    }

    data_ptr->rx_on_time_tu = BCWU_RxOnTime;
    data_ptr->bcn_timeout_count = BCWU_BcnLossCount;

    timp_set_condition(TIMP_INTERFACE_0, &timp_data);

    timp_clear();

#ifdef DBG_PRINT_TEMP
    PRINTF(YELLOW_COLOR " [%s] Term=%d, Period=%d sec (Threshold:rx_on_time=%d, bcn_loss_cnt=%d) \n"
                        CLEAR_COLOR,
                        __func__,
                        TIMP_INTERFACE_0,
                        BCWU_CheckPeriod,
                        BCWU_RxOnTime,
                        BCWU_BcnLossCount);
#endif
}

void badConditionLog(dpm_monitor_info_t *dpm_monitor_info_ptr, UINT currentRTC)
{
#ifdef DBG_PRINT_TEMP
    PRINTF(YELLOW_COLOR
        " [%s] Bad Condition Wakeup ==> Step %d, Counts:%d Times, interval:%d sec (first=%d,current=%d)\n"
        CLEAR_COLOR,
            __func__,
            dpm_monitor_info_ptr->badConditionStep,
            dpm_monitor_info_ptr->badConditionCount,
            (currentRTC - dpm_monitor_info_ptr->firstBadConditionRTC)/1000,
            dpm_monitor_info_ptr->firstBadConditionRTC/1000,
            currentRTC/1000);
#endif
}

UINT checkBadConditionWakeup(void)
{
    UINT    currentRTC = get_fci_dpm_curtime()/1000;
    UINT    rt;

    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    /* first case in new step */
    if (dpm_monitor_info_ptr->badConditionCount == 0) {
        dpm_monitor_info_ptr->firstBadConditionRTC = currentRTC;    /* unit : ms */
    }
    ++dpm_monitor_info_ptr->badConditionCount;

    if ( (currentRTC - dpm_monitor_info_ptr->firstBadConditionRTC) >
        ((BCWU_CheckPeriod*1000)*(CHECK_BADCONDITION_COUNT-1)) + CHECK_BADCONDITION_MARGIN )  {
        /* Occurred after 1 minute ==> initialize count & step */
        dpm_monitor_info_ptr->badConditionStep = 1;            /* 1st Step */
        dpm_monitor_info_ptr->badConditionCount = 1;            /* 1st count */
        dpm_monitor_info_ptr->firstBadConditionRTC = currentRTC;    /* unit : ms */

        badConditionLog(dpm_monitor_info_ptr, currentRTC);
        rt = 0;
    } else {
        if (dpm_monitor_info_ptr->badConditionCount >= CHECK_BADCONDITION_COUNT) {

            /* Continuously 3 times occurrences within 1 minute ==> going sleep mode */
            badConditionLog(dpm_monitor_info_ptr, currentRTC);

            if (dpm_monitor_info_ptr->badConditionStep < 9) {
                dpm_monitor_info_ptr->badConditionStep++;
            }
            dpm_monitor_info_ptr->badConditionCount = 0;

            rt = dpm_monitor_info_ptr->badConditionStep;
        } else {
            /* Occur in 1 minute */
            badConditionLog(dpm_monitor_info_ptr, currentRTC);
            rt = 0;
        }
    }

    return rt;
}

#endif    /*__CHECK_BADCONDITION_WAKEUP__    */

#if defined(__CHECK_CONTINUOUS_FAULT__) && defined(XIP_CACHE_BOOT)
unsigned int get_fault_pc(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();

    if (dpm_monitor_info_ptr) {
        return dpm_monitor_info_ptr->fault_PC;
    } else {
        return 0;
    }
}

void set_fault_pc(unsigned int pc)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    if (dpm_monitor_info_ptr) {
        dpm_monitor_info_ptr->fault_PC = pc;
    }
}

void increase_fault_count(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    if (dpm_monitor_info_ptr) {
        ++dpm_monitor_info_ptr->fault_CNT;
    }
}

void clr_fault_count(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    if (dpm_monitor_info_ptr) {
        dpm_monitor_info_ptr->fault_CNT = 0;
    }
}

unsigned char get_fault_count(void)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    if (dpm_monitor_info_ptr) {
        return dpm_monitor_info_ptr->fault_CNT;
    } else {
        return 0;
    }
}

void set_fault_count(char cnt)
{
    dpm_monitor_info_ptr = get_dpm_monitor_info_ptr();
    if (dpm_monitor_info_ptr) {
        dpm_monitor_info_ptr->fault_CNT = cnt;
    }
}

int size_of_dpm_monitor_info_t()
{
    return(sizeof(dpm_monitor_info_t));
}
#endif    // __CHECK_CONTINUOUS_FAULT__

/* EOF */
