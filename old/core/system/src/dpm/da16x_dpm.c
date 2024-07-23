/**
 ****************************************************************************************
 *
 * @file da16x_dpm.c
 *
 * @brief APIs and threads related to DPM.
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

#if defined ( XIP_CACHE_BOOT )        // RTOS

#include "da16x_types.h"
#include "defs.h"
#include "lwip/err.h"

#include "da16x_dpm.h"
#include "da16x_dpm_filter.h"
#include "da16x_dpm_rtm_mem.h"
#include "da16x_timer.h"
#include "da16x_dpm_sockets.h"

#include "dpmrtm_patch.h"
#include "dpmty_patch.h"
#include "romac_primitives_patch.h"

#include "dpm_aptrk_patch.h"
#include "common_def.h"

#include "nvedit.h"
#include "environ.h"
#include "common_def.h"
#include "user_dpm_api.h"
#include "util_api.h"

#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#pragma GCC diagnostic ignored "-Wcast-function-type"

/* Local type defines ... */


/* Definition values */

#define DPM_DAEMON_TRIGGER_TASK_NAME    "dpm_trigger"
#define DPM_TRIGGER_POLLING_TIME        3             // 30 msec

#define DPM_SLEEP_DAEMON_NAME           "dpm_sleep_d"
#define DPM_SLEEPD_PRIORITY             OS_TASK_PRIORITY_USER
#define DPM_SLEEPD_STACK_SZ             256
#define DPM_DAEMON_SLEEP_DELAY_CNT      2

#define DPM_MUTEX_WAIT                  100

#define DPM_DAEMON_TRIGGER              ( 1 << 0 )
#define DPM_SLEEP_SET                   ( 1 << 1 )
#define DPM_STOP_FLAG                   ( 1 << 2 )

#define MAX_CHECK_CNT_PS_CMD_FAIL       10          // 1000 msec
#define TIMER_MAX_MSEC                  4233600000  //49 day

#define DPM_TIMER_TASK_NAME             "dpm_tm_task"
#define DPM_TIMER_TASK_PRIORITY         OS_TASK_PRIORITY_USER
#define DPM_TIMER_STK_SZ                1024
#define RTC_ABNORMAL_MAX_CNT            3


/* External variables */
extern unsigned char    runtime_cal_flag;
extern unsigned char    dpm_dbg_cmd_flag;
extern unsigned char    dpm_slp_time_reduce_flag;
extern unsigned char    dpm_wu_tm_msec_flag;
extern unsigned char    dpm_abnormal_disable_flag;
extern unsigned char    ble_combo_ref_flag;
extern unsigned char    dpm_mng_save_rtm_flag;
extern TimerHandle_t    dpm_sts_chk_tm;

/* External global functions */
extern int  get_run_mode(void);
extern int  get_boot_mode(void);
extern bool get_wifi_driver_can_rcv(int);
extern void unset_wifi_driver_not_rcv(int intf);
extern void set_wifi_driver_not_rcv(int intf);
extern void start_dpm_sts_chk_timer(int dpm_wu_type);
extern void printf_with_run_time(char * string);

extern bool is_dpm_supplicant_done(void);
extern int  fc80211_set_da16x_power_off(int ifindex);
extern void fc80211_connection_loss(void);
extern void set_dpm_wakeup_condition(unsigned int type);
extern int  fc80211_get_empty_rid(void);
extern int  fc80211_set_app_keepalivetime(unsigned char tid, unsigned int msec,    \
                                  void (* callback_func)(unsigned int tid));
#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
extern unsigned char get_last_abnormal_cnt(void);
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */

extern void umac_dpm_rtm_info_clear(void);
extern void set_dpm_tim_wakeup_time_rtm(unsigned short sec);


/* Global variables */
bool dpm_daemon_cmd_hold_flag    = pdFALSE;

dpm_supp_key_info_t  *dpm_supp_key_info = NULL;
dpm_supp_conn_info_t *dpm_supp_conn_info = NULL;


/* Local static functions */
static void dpm_trigger_timer_fn(unsigned long arg);

/* Local static variables */
static EventGroupHandle_t   da16x_dpm_ev_group = NULL;
static SemaphoreHandle_t    da16x_dpm_reg_req_flag_mutex = NULL;
static SemaphoreHandle_t    dpm_timer_mutex = NULL;

static TimerHandle_t        dpm_trigger_timer = NULL;
static TaskHandle_t         dpm_sleep_daemon_handler = NULL;

static QueueHandle_t        dpm_timer_queue = NULL;
static TaskHandle_t         dpm_timer_proc_handler = NULL;

/* Local initial variables */
static int  dpm_wpa_supp_state      = WPA_DISCONNECTED;
static int  dpm_pdown_state         = WAIT_DPM_SLEEP;

static bool dpm_daemon_start_flag   = pdFALSE;
static bool dpm_mac_err_flag        = pdFALSE;
static bool dpm_mac_err_max_try_reached_flag = pdFALSE;
static bool dpm_daemon_hold_flag    = pdFALSE;
static bool dpm_rtc_to_chk_flag     = pdFALSE;
static bool user_rtm_pool_init_done = pdFALSE;

static unsigned long  reg_dpm_req_flag   = 0;
static unsigned long  dpm_sleep_set_flag = 0;

static dpm_list_table _da16x_dpm_sleep_list[MAX_DPM_REQ_CNT] = { 0, };

#if defined ( WORKING_FOR_DPM_TCP )
static dpm_tcp_ka_info_t        *dpm_tcp_ka_info            = NULL;
#endif /* WORKING_FOR_DPM_TCP */

static char dpm_timer_abnormal_flag[TIMER_ERR] = { 0, };


/* Local static functions */
#if defined ( WORKING_FOR_DPM_TCP )
static void clr_all_dpm_tcp_ka_info(void);
#endif /* WORKING_FOR_DPM_TCP */
static char    *str_dpm_wakeup_condition(void);


/* Global API functions */
int  dpm_read_nvram_int(const char *name, int *_val);
int  dpm_write_nvram_int(const char *name, int val, int def);
int  dpm_write_tmp_nvram_int(const char *name, int val, int def);

void set_dpm_supp_ptr(void);
int  get_dpm_keepalive_from_nvram(void);
int  get_dpm_wakeup_condition_from_nvram(void);
int  get_dpm_TIM_wakeup_time_from_nvram(void);
int  get_dpm_user_wu_time_from_nvram(void);
int  set_dpm_keepalive_to_nvram(int time);
int  set_dpm_user_wu_time_to_nvram(int sec);

bool get_dpm_pwrdown_fail_max_trial_reached(void);
void set_dpm_pwrdown_fail_max_trial_reached(bool value);

enum dpm_enum_dbg_level {
    MSG_ERROR = 1,
    MSG_WARNING,
    MSG_INFO,
    MSG_DEBUG,
    MSG_EXCESSIVE,
    MSG_MSGDUMP
};

/// Debug Level ////////////////////////////////////////
unsigned int get_dpm_dbg_level(void)
{
    if (dpm_dbg_cmd_flag == pdTRUE) {
        return RTM_FLAG_PTR->dpm_dbg_level;
    } else {
        return pdFALSE;
    }
}

void set_dpm_dbg_level(unsigned int level)
{
    if (dpm_dbg_cmd_flag == pdTRUE) {
        RTM_FLAG_PTR->dpm_dbg_level = level;
    }
}


/// Common APIs ////////////////////////////////////////

bool  get_dpm_pwrdown_fail_max_trial_reached(void)
{
    return dpm_mac_err_max_try_reached_flag;
}

void  set_dpm_pwrdown_fail_max_trial_reached(bool value)
{
    dpm_mac_err_max_try_reached_flag = value;
}

static void unsupport_func_in_dpm(void)
{
    /* dpm is not support - Simple roaming */
    dpm_write_nvram_int("STA_roam", 0, 0);
}

void set_default_dpm_mode(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        dpm_write_nvram_int("dpm_mode", 1, 0);
        return;
    }

    if (RTM_FLAG_PTR->dpm_mode == 1) {
        return;
    }

    /* Stop functions while dpm mode running */
    unsupport_func_in_dpm();

    dpm_write_nvram_int("dpm_mode", 1, 0);

    set_dpm_keepalive_to_nvram(DFLT_DPM_KEEPALIVE_TIME);
    set_dpm_user_wu_time_to_nvram(DFLT_DPM_USER_WAKEUP_TIME);

    if (dpm_wu_tm_msec_flag == pdTRUE) {
        set_dpm_TIM_wakeup_time_to_nvram((int)coilToInt(DFLT_DPM_TIM_WAKEUP_COUNT / 102.4) );
    } else {
        set_dpm_TIM_wakeup_time_to_nvram(DFLT_DPM_TIM_WAKEUP_COUNT);
    }

    vTaskDelay(10);

    RTM_FLAG_PTR->dpm_mode = 1;
}

bool chk_retention_mem_exist(void)
{
    int    chk_status;

    chk_status = chk_rtm_exist();

    return ((chk_status == 1) ? pdPASS : pdFAIL);
}

void enable_dpm_mode(void)
{
    if (chk_retention_mem_exist() == pdFALSE) {
        /* Unsupport RTM */
        dpm_write_nvram_int("dpm_mode", 1, 0);
        return;
    }

    if (RTM_FLAG_PTR->dpm_mode == 1) {
        return;
    }

    /* Stop functions while dpm mode running */
    unsupport_func_in_dpm();

    dpm_write_nvram_int("dpm_mode", 1, 0);

    vTaskDelay(10);

    RTM_FLAG_PTR->dpm_mode = 1;
}

void disable_dpm_mode(void)
{
    if (chk_retention_mem_exist() == pdFALSE) {
        /* Unsupport RTM */
        dpm_write_nvram_int("dpm_mode", 0, 0);
        return;
    }

    if (RTM_FLAG_PTR->dpm_mode == 0) {
        return;
    }

    if (dpm_trigger_timer != NULL) {
        xTimerStop(dpm_trigger_timer, 0);        // Stop DPM trigger timer
        xTimerDelete(dpm_trigger_timer, 0);        // Delete DPM trigger timer
        dpm_trigger_timer = NULL;
    }
    
    /* Delete DPM daemon task */
    if (dpm_sleep_daemon_handler != NULL) {
        da16x_sys_watchdog_unregister(da16x_sys_wdog_id_get_dpm_sleep_daemon());

        da16x_sys_wdog_id_set_dpm_sleep_daemon(DA16X_SYS_WDOG_ID_DEF_ID);

        vTaskDelete(dpm_sleep_daemon_handler);

        dpm_sleep_daemon_handler = NULL;
    }

    vTaskDelay(2);
    dpm_write_nvram_int("dpm_mode", 0, 0);
    vTaskDelay(1);

    dpm_daemon_start_flag = pdFALSE;
    dpm_daemon_hold_flag = pdFALSE;

    RTM_FLAG_PTR->dpm_mode = 0;
}

void stop_dpm_mode(void)
{
    if (chk_dpm_mode() == pdFALSE) {
        return;
    }

    if (RTM_FLAG_PTR->dpm_mode == 0) {
        return;
    }

    dpm_daemon_start_flag = pdFALSE;         // Stop DPM Sleep Daemon
    dpm_daemon_hold_flag = pdFALSE;          // Stop DPM Sleep Daemon

    xTimerStop(dpm_trigger_timer, 0);        // Stop DPM trigger timer
    xTimerDelete(dpm_trigger_timer, 0);      // Delete DPM trigger timer

    /* Delete DPM daemon task */
    if (dpm_sleep_daemon_handler != NULL) {
        da16x_sys_watchdog_unregister(da16x_sys_wdog_id_get_dpm_sleep_daemon());

        da16x_sys_wdog_id_set_dpm_sleep_daemon(DA16X_SYS_WDOG_ID_DEF_ID);

        vTaskDelete(dpm_sleep_daemon_handler);

        dpm_sleep_daemon_handler = NULL;
    }

    RTM_FLAG_PTR->dpm_mode = 0;

    vTaskDelay(2);
}

int get_dpm_mode(void)
{
    int dpm_mode = 0;

    if (chk_retention_mem_exist() == pdFALSE) {
        /* Unsupport RTM */
        return 0;
    }

    if (RTM_FLAG_PTR->dpm_mode != 1) {
        if (dpm_read_nvram_int("dpm_mode", &dpm_mode) == -1) {
            RTM_FLAG_PTR->dpm_mode = 0;
        } else {
            RTM_FLAG_PTR->dpm_mode = dpm_mode;
        }
    }

    return RTM_FLAG_PTR->dpm_mode;
}

int chk_dpm_mode(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return pdFALSE ;
    }

    if (RTM_FLAG_PTR->dpm_mode) {
        return pdTRUE;
    }

    return pdFALSE;
}

void enable_dpm_wakeup(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    /* da16x will be wake up by dpm mode */
    RTM_FLAG_PTR->dpm_wakeup = 1;
}

void disable_dpm_wakeup(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    /* da16x will be normal boot */
    RTM_FLAG_PTR->dpm_wakeup = 0;
}

int chk_dpm_wakeup(void)
{
    if (chk_retention_mem_exist() == pdFAIL || get_run_mode() != SYSMODE_STA_ONLY) {
        /* Unsupport RTM */
        return pdFALSE;
    }

    if ((get_run_mode() == SYSMODE_STA_ONLY) && RTM_FLAG_PTR->dpm_mode && RTM_FLAG_PTR->dpm_wakeup) {
        return pdTRUE;
    }

    return pdFALSE;
}

void show_dpm_sleep_info(void)
{
    dpm_list_table    *da16x_dpm_list;
    int    i, cnt = 0;
    unsigned long tim_period;

    if (chk_retention_mem_exist() == pdFAIL) {
        return;
    }

    tim_period = dpm_sche_get_interval(GET_DPMP(), dpm_get_env_tim_period_ty(GET_DPMP()));

    PRINTF("\n");
    PRINTF("[%s]\n", chk_dpm_sleepd_hold_state() ? "HOLD" : "RESUME");
    PRINTF("===============================================\n");
    PRINTF("  DPM_SLEEP state - mode(%d), wakeup(%d), debug(%d)\n",
                RTM_FLAG_PTR->dpm_mode,
                RTM_FLAG_PTR->dpm_wakeup,
                RTM_FLAG_PTR->dpm_dbg_level);
    PRINTF("===============================================\n");
    PRINTF(" - Wakeup condition      = %s\n", str_dpm_wakeup_condition());
    PRINTF(" - TIM wakeup time       = %d ( * 100 msec )\n", tim_period);
    PRINTF(" - User wakeup time      = %d msec\n", get_dpm_user_wu_time_from_nvram());
    PRINTF(" - KeepAlive time        = %d ms\n",
            RTM_FLAG_PTR->dpm_keepalive_time_msec);

    PRINTF("\n");
    PRINTF(" - Bit                   : 11111111111111110000000000000000\n");
    PRINTF("                           FEDCBA9876543210FEDCBA9876543210\n");


    PRINTF(" - Registered bit        = ");

    for (i = (MAX_DPM_REQ_CNT - 1); i >= 0; i--) {
        PRINTF("%d", (reg_dpm_req_flag >> i) & 1);
    }

    PRINTF("\n");
    PRINTF(" - Sleep requested bit   = ");

    for (i = (MAX_DPM_REQ_CNT - 1); i >= 0; i--) {
        PRINTF("%d", (dpm_sleep_set_flag >> i) & 1);
    }

    PRINTF("\n\n-------------------------------------------\n");
    PRINTF(" Bit\t %-22s Port\n", "Name");
    PRINTF("-------------------------------------------\n");

    for (i = 0; i < MAX_DPM_REQ_CNT; i++) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];

        if (da16x_dpm_list->mod_name[0] == 0)
            continue;

        cnt++;
        PRINTF(" %02X  %-22s %d\n", da16x_dpm_list->bit_index,
            da16x_dpm_list->mod_name, da16x_dpm_list->port_number);
    }
    PRINTF("===============================================\n\n");
}

int is_dpm_sleep_ready(void)
{
    if (reg_dpm_req_flag == dpm_sleep_set_flag) {
        return 1;
    } else {
        return 0;
    }
}

void show_dpm_set_task_names(void)
{
    dpm_list_table    *da16x_dpm_list;
    int    i, reg_bit, sleep_bit;

    if (chk_retention_mem_exist() == pdFAIL) {
        return;
    }

    for (i = MAX_DPM_REQ_CNT; i >= 0; i--) {
        reg_bit = (reg_dpm_req_flag >> i) & 1;
        sleep_bit = (dpm_sleep_set_flag >> i) & 1;

        if (reg_bit != sleep_bit) {
            da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
            PRINTF("[%s] Unsleep module name=<%s>\n", __func__, da16x_dpm_list->mod_name);
        }
    }

}

unsigned char *get_dpm_set_task_names(void)
{
    dpm_list_table  *da16x_dpm_list;
    int i, reg_bit, sleep_bit;

    if (chk_retention_mem_exist() == pdFAIL) {
        return (unsigned char *)NULL;
    }

    for (i = MAX_DPM_REQ_CNT; i >= 0; i--) {
        reg_bit = (reg_dpm_req_flag >> i) & 1;
        sleep_bit = (dpm_sleep_set_flag >> i) & 1;

        if (reg_bit != sleep_bit) {
            da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
            return((unsigned char *)da16x_dpm_list->mod_name);
        }
    }

    return (unsigned char *)NULL;
}


void set_dpm_all_flags(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        return;
    }

    dpm_sleep_set_flag = reg_dpm_req_flag;
}

/*
 * API for registering to use DPM_SLEEP
 */
int chk_dpm_reg_state(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int    i;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];

        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            return DPM_REG_DUP_NAME;
        }

        i++;
    }

    return DPM_NOT_REGISTERED;
}

int chk_dpm_rcv_ready(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int    i;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            return (da16x_dpm_list->rcv_ready);
        }
        i++;
    }

    return 0;
}

int chk_dpm_set_state(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int    chk_bit;
    int    i;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            chk_bit = (1 << i);
            if (dpm_sleep_set_flag & chk_bit)
                return 1;
        }
        i++;
    }

    return 0;
}

char *chk_dpm_reg_port(unsigned int port_number)
{
    dpm_list_table  *da16x_dpm_list;
    int     chk_bit;
    int     i;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (da16x_dpm_list->port_number == port_number) {
            chk_bit = (1 << i);

            if (reg_dpm_req_flag & chk_bit)
                return da16x_dpm_list->mod_name;
        }
        i++;
    }

    return (char *)NULL;
}

int chk_dpm_pdown_start(void)
{
    return dpm_pdown_state;
}

int reg_dpm_sleep(char *mod_name, unsigned int port_number)
{
    dpm_list_table    *da16x_dpm_list = NULL;
    int    i, ret = 0;

    if (chk_dpm_mode() == pdFALSE)
        return DPM_REG_OK;

    ret = DPM_REG_OK;

    i = 0;

    /* Get Regist flag */
    if (xSemaphoreTake(da16x_dpm_reg_req_flag_mutex, (TickType_t)DPM_MUTEX_WAIT) != pdTRUE) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR)
            PRINTF(RED_COL "[%s] mutex get error (%s) !!!\n" CLR_COL, __func__, mod_name);

        return DPM_REG_ERR;
    }

    if (chk_dpm_reg_state(mod_name) == DPM_REG_DUP_NAME) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR)
            PRINTF("[%s] Duplicat name (%s) !!!\n", __func__, mod_name);

        ret = DPM_REG_DUP_NAME;
        goto finish;
    }


    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];

        if (dpm_strlen(da16x_dpm_list->mod_name) == 0) {
            break;
        }

        i++;
    }
    if (i >= MAX_DPM_REQ_CNT) {
        PRINTF("[%s] No resource !!!\n", __func__);
        ret = DPM_REG_ERR;

        goto finish;
    }

    /* Register dpm_sleep mode to flag */
    reg_dpm_req_flag |= (1 << i);

    /* Clear(init) dpm_sleep mode to flag */
    dpm_sleep_set_flag &= ~(1 << i);

    /* Set regist information */
    dpm_strcpy(da16x_dpm_list->mod_name, mod_name);
    da16x_dpm_list->port_number = port_number;
    da16x_dpm_list->bit_index = i;

finish:
    if (xSemaphoreGive(da16x_dpm_reg_req_flag_mutex) != pdTRUE) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR) {
            PRINTF("[%s] mutex put error (%s) !!!\n", __func__, mod_name);
        }

        ret = DPM_REG_ERR;
    }

    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_EXCESSIVE) {
        PRINTF("[%s] <%s> bit=%d, ret=%d\n", __func__, da16x_dpm_list->mod_name, i, ret);
    }

    return ret;
}

void unreg_dpm_sleep(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int    i;

    if (chk_dpm_mode() == pdFALSE) {
        return;
    }

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            break;
        }
        i++;
    }
    if (i >= MAX_DPM_REQ_CNT) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR)
            PRINTF("[%s] Unregistered (%s) !!!\n", __func__, mod_name);
        goto finish;
    }

    /* Get Regist flag */
    if (xSemaphoreTake(da16x_dpm_reg_req_flag_mutex, (TickType_t)DPM_MUTEX_WAIT) != pdTRUE) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR)
            PRINTF(RED_COL "[%s] mutex get error (%s) !!!\n" CLR_COL, __func__, mod_name);

        return;
    }

    /* Unregister dpm_sleep mode to flag */
    reg_dpm_req_flag &= ~(1 << i);

    /* Clear dpm_sleep mode to flag */
    dpm_sleep_set_flag &= ~(1 << i);

    if (xSemaphoreGive(da16x_dpm_reg_req_flag_mutex)  != pdTRUE) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR) {
            PRINTF("[%s] mutex put error (%s) !!!\n", __func__, mod_name);
        }
    }

    /* Set regist information */
    memset((void *)da16x_dpm_list->mod_name, 0, REG_NAME_DPM_MAX_LEN);
    da16x_dpm_list->bit_index = 0;

finish:
    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_DEBUG) {
        PRINTF("[%s] <%s> bit=%d\n", __func__, da16x_dpm_list->mod_name, i);
    }

    return;
}

int set_dpm_rcv_ready(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int    i = 0;

    if (chk_dpm_mode() == pdFALSE) {
        return DPM_SET_OK;
    }

    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            if (da16x_dpm_list->port_number == 0) {
                da16x_dpm_list->rcv_ready = 0;
            } else {
                da16x_dpm_list->rcv_ready = 1;
            }

            break;
        }

        i++;
        if (i > MAX_DPM_REQ_CNT) {
            return DPM_SET_ERR;
        }
    }

    return DPM_SET_OK;
}

int set_dpm_rcv_ready_by_port(unsigned int port)
{
    dpm_list_table    *da16x_dpm_list;
    int    i = 0;

    if (chk_dpm_mode() == pdFALSE) {
        return DPM_SET_OK;
    }

    for (i = 0 ; i < MAX_DPM_REQ_CNT ; i++) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (da16x_dpm_list->port_number == port) {
            da16x_dpm_list->rcv_ready = 1;
            return DPM_SET_OK;
        }
    }

    return DPM_SET_ERR;
}

unsigned int get_bit_dpm_sleep(char *mod_name)
{
    dpm_list_table    *da16x_dpm_list;
    int bit_idx = MAX_DPM_REQ_CNT + 1;
    int i;

    if (chk_dpm_mode() == pdFALSE) {
        return (unsigned int)DPM_SET_ERR;
    }

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            bit_idx = i;

            break;
        }
        i++;
    }

    if (i >= MAX_DPM_REQ_CNT) {
        return (unsigned int)DPM_SET_ERR;
    }

    return (unsigned int)((dpm_sleep_set_flag & (1 << bit_idx)) >> bit_idx);

}

int set_dpm_sleep(char *mod_name)
{
    EventBits_t    dpm_ev_bits = (EventBits_t) NULL;
    unsigned long  prv_dpm_sleep_set_flag = 0;
    dpm_list_table *da16x_dpm_list;
    int    bit_idx = MAX_DPM_REQ_CNT + 1;
    int    i, ret = DPM_SET_OK;
    bool     except_case = 0;

    if (chk_dpm_mode() == pdFALSE)
        return DPM_SET_OK;

    /* Already start ... */
    /** If DPM Sleep Command is done, do nothing **/
    while (chk_dpm_pdown_start() != WAIT_DPM_SLEEP) {
        /** RX Packet processing for LMAC RX should not be stopped. **/
        /** Lets do except DPM_UMAC and DPM_UMAC_RX **/
        if (   dpm_strcmp("DPM_UMAC" , mod_name) == 0
            || dpm_strcmp("DPM_UMAC_RX" , mod_name) == 0
            || dpm_strcmp("Disconnect_loss" , mod_name) == 0) {
            /* Cse of TRX DPM Module, should be break loop */
            except_case = 1;
            break;
        }

        vTaskDelay(1);
        continue;
    }

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            bit_idx = i;
            da16x_dpm_list->bit_index = bit_idx;

            /* Already set ... */
              if ((dpm_sleep_set_flag & (1 << bit_idx)) == 1)
                return DPM_SET_OK;

            break;
        }
        i++;
    }

    if (i >= MAX_DPM_REQ_CNT) {
        ret = DPM_NOT_REGISTERED;
        goto finish;
    }

    prv_dpm_sleep_set_flag = dpm_sleep_set_flag;

    /* Set dpm_sleep mode to flag */
    dpm_sleep_set_flag |= (1 << bit_idx);

    /** In case TRX DPM Module & RUN dpm sleep status, do not set the event ,
     *    just set the module bit and return
     */
    if (except_case && chk_dpm_pdown_start() == RUN_DPM_SLEEP)
        return    DPM_SET_OK;

    dpm_ev_bits = xEventGroupSetBits(da16x_dpm_ev_group, DPM_SLEEP_SET);
    if ((dpm_ev_bits & DPM_SLEEP_SET) != DPM_SLEEP_SET) {
        PRINTF("[%s] event flag set error !!! (0x%08x)\n", __func__, dpm_ev_bits);
    }

finish:
    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_EXCESSIVE) {
        if (prv_dpm_sleep_set_flag != dpm_sleep_set_flag) {
            PRINTF("[%s] <%s> bit_idx=%d, ret = %d\n", __func__, mod_name, bit_idx, ret);
        }
    }

    return ret;

}

int clr_dpm_sleep(char *mod_name)
{
    unsigned long    prv_dpm_sleep_set_flag = 0;
    dpm_list_table    *da16x_dpm_list;
    int    bit_idx = MAX_DPM_REQ_CNT + 1;
    int    i, ret;

    if (chk_dpm_mode() == pdFALSE) {
        return DPM_SET_OK;
    }

    /** If DPM Sleep Command is done, do nothing **/
    while (chk_dpm_pdown_start() != WAIT_DPM_SLEEP) {
        /** RX Packet processing for LMAC RX should not be stopped. **/
        /** Lets do except DPM_UMAC and DPM_UMAC_RX **/
        if (   dpm_strcmp("DPM_UMAC" , mod_name) == 0
            || dpm_strcmp("DPM_UMAC_RX" , mod_name) == 0
            || dpm_strcmp("Disconnect_loss" , mod_name) == 0) {
            break;
        }

        vTaskDelay(1);
        continue;
    }

    ret = DPM_SET_OK;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            bit_idx = da16x_dpm_list->bit_index;
            break;
        }
        i++;
    }
    if (i >= MAX_DPM_REQ_CNT) {
        ret = DPM_SET_ERR;
        goto finish;
    }

    prv_dpm_sleep_set_flag = dpm_sleep_set_flag;

    /* Clear dpm_sleep mode to flag */
    dpm_sleep_set_flag &= ~(1 << bit_idx);

finish:
    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_EXCESSIVE) {
        if (prv_dpm_sleep_set_flag != dpm_sleep_set_flag)
            PRINTF("[%s] <%s> bit_idx=%d, ret = %d\n", __func__, mod_name, bit_idx, ret);
    }

    return ret;

}

int chk_dpm_sleep_daemon(void)
{
    return RTM_FLAG_PTR->dpm_sleepd_stop;
}

int start_dpm_sleep_mode_2(unsigned long long usec, unsigned char retention)
{
    extern void fc80211_da16x_pri_pwr_down(unsigned char retention);
    extern void fc80211_da16x_sec_pwr_down(unsigned long long usec, unsigned char retention);

    const unsigned long long max_usec = 0x1FFFFF * 1000000ULL;

    if (((retention != 0) && (retention != 1)) || (usec > max_usec)) {
        //Invalid parameter
        return -1;
    }

    if (usec > 0) {
        fc80211_da16x_sec_pwr_down(usec, retention);

    } else {
        fc80211_da16x_pri_pwr_down(retention);
    }

    return 0;
}

/* Abnormal DPM Parameter clearing */
void clear_abnormal_DPM_param(void)
{
    /* If last sleep type is abnormal , do clearing */
    RTM_DPM_MONITOR_PTR->last_sleep_type = 0;
    RTM_DPM_MONITOR_PTR->last_abnormal_type = 0;
    RTM_DPM_MONITOR_PTR->last_abnormal_count = 0;
#ifdef __DPM_FINAL_ABNORMAL__
    RTM_DPM_MONITOR_PTR->final_abnormal_tid = 0;
#endif /* __DPM_FINAL_ABNORMAL__ */

    return;
}

void    (*dpmDaemonSleepFunction)() = NULL;
int dpm_daemon_regist_sleep_cb(void (*regConfigFunction)())
{
    dpmDaemonSleepFunction = regConfigFunction;
    return 0;
}

static void dpm_sleep_daemon(void *arg)
{
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;
    EventBits_t    da16x_dpm_daemon_ev;
    unsigned long  check_ev_flag;
    unsigned int   rtc_exp_status;
    int  sleep_delay_cnt = DPM_DAEMON_SLEEP_DELAY_CNT;    // for reducing data rx loss
    int  pdown_result;
    int  dpm_sleep_try_chk_cnt = 0;
    int  dpm_wu_type = (int)arg;

#if defined(__DA16X_DPM_MON_CLIENT__)
    unsigned long    run_tick, curr_tick;
#endif /* __DA16X_DPM_MON_CLIENT__ */

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_dpm_sleep_daemon(sys_wdog_id);
    }

#ifdef __SUPPORT_DPM_SLEEP_DELAY__
    /* Start DPM Sleep Daemon */
    dpm_sleepd_delay_time = get_dpm_sleepd_delay();
#endif    /* __SUPPORT_DPM_SLEEP_DELAY__ */

    /* Create timer to check DPM abnormal status. */
    if (dpm_abnormal_disable_flag == pdFALSE) {    // In case of disable DPM-abnormal timer..
        start_dpm_sts_chk_timer(dpm_wu_type);
    }

    /* move to dpm_full_wakeup_wlaninit to here */

    /* dpm_wu_type 0 : normal power on reboot
     *             3 : no beacon / no ack / deauth
     *             4 : uc wakeup
     *             5 : beacon changed / bc_mc
     */
    if (chk_dpm_wakeup() == pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);
        /*
         * In case of DPM Wakeup & tim status is UC, BC, BC Changed,
         * we should start the RX packet of MAC of TIM
         */
        if (dpm_wu_type == DPM_PACKET_WAKEUP || dpm_wu_type == DPM_DEAUTH_WAKEUP) {
            /*
             * For Others rx packet processing,
             * move to wlaninit(), only need packet processing delay
             */
            if (dpm_slp_time_reduce_flag == pdFALSE) {
                /* For UC wakeup, need enough loading time of network stack. */
                vTaskDelay(10);
            }

        } else if (dpm_wu_type == DPM_UNKNOWN_WAKEUP) {
            /*
             * In case of Abnormal wakeup with UNNKOWN_TYPE,
             * some ISSI EVB should to have time delay to start DPM daemon.
             */
            vTaskDelay(10);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    } else {
        /* Clear TCP network information,,, first POR state */
#if defined ( WORKING_FOR_DPM_TCP )
        clr_all_dpm_tcp_ka_info();
#endif    // WORKING_FOR_DPM_TCP

        da16x_dpm_socket_clear_all_tcp_sess_info();

        dpm_clear_mdns_info();
    }

    /* for user RTM area ... */
    dpm_user_rtm_pool_create();

#if defined ( __SUPPORT_ZERO_CONFIG__ )
    set_multicast_for_zeroconfig();
#endif    // __SUPPORT_ZERO_CONFIG__

    /* In DA16200, dpm sleep daemon is started before supplicant connection is done */
    /* So for blocking this, wait until connection is done */
    if (chk_dpm_wakeup() == pdTRUE) {
        struct dpm_param *dpmp = GET_DPMP();

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        while (pdTRUE) {
            if (is_dpm_supplicant_done()) {
                break;
            }

            vTaskDelay(2);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        /* DPM Hold by TIM Feature for debugging */
        if (CHK_DBG_FEATURE(DPM_DBG_WAIT_FOR_DPMRESUME))
            dpm_daemon_hold_flag = pdTRUE;
    }

    /* Loop to check dpm_sleep status */
    while (dpm_daemon_start_flag == pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        /* Hold By Both Command and something Processing */
        if (dpm_daemon_hold_flag == pdTRUE || dpm_daemon_cmd_hold_flag == pdTRUE) {
            da16x_sys_watchdog_suspend(sys_wdog_id);

            vTaskDelay(2);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
            continue;
        }

        if (get_wifi_driver_can_rcv(0) == pdTRUE) {
            unset_wifi_driver_not_rcv(0);
        }

        /* Check events */
        check_ev_flag = DPM_SLEEP_SET | DPM_DAEMON_TRIGGER;

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

		// 10 msec
        da16x_dpm_daemon_ev = xEventGroupWaitBits(da16x_dpm_ev_group, check_ev_flag, pdFALSE, pdFALSE, 1);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        /* None operation */
        if ((da16x_dpm_daemon_ev & check_ev_flag) == 0) {
            continue;
        }

        /* Check DPM hold by command */
        if (dpm_daemon_hold_flag == pdTRUE) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            vTaskDelay(1);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
            continue;
        }

        if (da16x_dpm_daemon_ev & DPM_DAEMON_TRIGGER) {
            da16x_dpm_daemon_ev &= ~DPM_DAEMON_TRIGGER;
        } else if (da16x_dpm_daemon_ev & DPM_SLEEP_SET) {
            /* Need delay to sync with user_application
             * data tx/rx processing before power-down */
            sleep_delay_cnt = DPM_DAEMON_SLEEP_DELAY_CNT;
            da16x_dpm_daemon_ev &= ~DPM_SLEEP_SET;
        } else {
            da16x_dpm_daemon_ev = 0;
            continue;
        }

        /* Check RTC timeout function running state */
        if (dpm_rtc_to_chk_flag == pdTRUE) {
            continue;
        }

        /* Enter DPM daemon task critical section. */
        //taskENTER_CRITICAL();

        if (RTM_FLAG_PTR->dpm_mode && (--sleep_delay_cnt <= 0)) {
#ifdef __SUPPORT_DPM_SLEEP_DELAY__
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            /* Just for test : Delay power-down ... */
            while (dpm_sleepd_delay_time > 0) {
                dpm_sleepd_delay_time--;
                vTaskDelay(100);
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            set_dpm_sleepd_delay(0);    /* TEMPORARY */
#endif    /* __SUPPORT_DPM_SLEEP_DELAY__ */

            /* chk_dpm_sleep_possibility() */
            if ((reg_dpm_req_flag != 0) && (reg_dpm_req_flag == dpm_sleep_set_flag)) {

#if defined(__DA16X_DPM_MON_CLIENT__)
                run_tick = run_tick_from_start(&curr_tick);
                saveStatusToMonitorServer(run_tick);
                start_tick = 0;
#endif    /* __DA16X_DPM_MON_CLIENT__ */

                /*
                 *  Wait until tcp sess (if any) are finished with ongoing transaction 
                 *  e.g. checking packet pool and transmission state <-- v2.4's code can be 
                 *  replaced with this code
                 */
                da16x_sys_watchdog_notify(sys_wdog_id);

                da16x_sys_watchdog_suspend(sys_wdog_id);

                if (!da16x_dpm_sock_is_available_set_sleep()) {
                    vTaskDelay(2);

                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                    continue;
                }

                /* If XTAL is not set after booting, let's do waiting */
                if (RTC_GET_CUR_XTAL() != EXTERNAL_XTAL) {
                    // PRINTF("[%s:%d]Cur Xtal is not set\r\n", __func__, __LINE__);
                    vTaskDelay(1);
                    continue;
                }

                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                
                if (dpm_mng_save_rtm_flag) {
                    if (dpmDaemonSleepFunction) {
#ifdef ENABLE_DPM_DBG_LOG
                        PRINTF(" Call register function !!!\n", __func__);
#endif
                        dpmDaemonSleepFunction();
                    }
                }

                /* Set Data-Rx blocking */
                set_wifi_driver_not_rcv(0);

                /* Try to do Power-Down */
                PRINTF(YEL_COL ">>> Start DPM Power-Down !!! \n" CLR_COL);

                if (runtime_cal_flag == pdTRUE) {
                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    printf_with_run_time("DPMD : Start Power-OFF...\n");
                    vTaskDelay(1);

                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                }

                extern void combo_make_sleep_rdy(void);
                if (ble_combo_ref_flag == pdTRUE) {
                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    combo_make_sleep_rdy();

                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                }

                /* Start DA16x power down for DPM sleep */
                dpm_pdown_state = RUN_DPM_SLEEP;
                pdown_result = fc80211_set_da16x_power_off(0);

                /* Save DPM sleep type in RTM */
                switch (pdown_result) {
                case -1 :
                case -2 :
                    dpm_pdown_state = WAIT_DPM_SLEEP;
                    break;

                /* If RX Packet is processing in IP Stack,
                  *  let's do sleep during a moment */
                case -3 :
                    dpm_pdown_state = WAIT_DPM_SLEEP;
                    break;

                /* STA state is DISCONNECT.
                  * So try to connect processing
                  * by supplicant */
                case -4 :
                    dpm_pdown_state = WAIT_DPM_SLEEP;

                    /* Disconnection status , Check it... */
                    clr_dpm_sleep(REG_NAME_DPM_SUPPLICANT);

                    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_DEBUG)
                        PRINTF(GRN_COL "### Hold DPM Power-Down, wait for re-connection.\n" CLR_COL);

                    break;

                case 0 :    // DPM Power-Down OKay...

                    /* RTC pause */
                    rtc_exp_status = clear_rtc_exp_status(pdTRUE);
                    if (rtc_exp_status) {
                        PRINTF("\nrtc_exp(0x%x)\r\n" , rtc_exp_status);
                    }

                    /* Normal case, but returned
                     * because PS Command has delayed
                     * in MAC cmd
                     */

                    dpm_pdown_state = DONE_DPM_SLEEP;
                    PRINTF(BLU_COL ">>> OK Power-Down !!!\n" CLR_COL);

                    da16x_sys_watchdog_notify(sys_wdog_id);

                    da16x_sys_watchdog_suspend(sys_wdog_id);

                    /* In case of OK, power will be down now ...  */
                     for (;;) {
                         vTaskDelay(10);
                     };

                     da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

                     break;

                default :
                    /* MAC may be abnormal working status
                     * (CMD Timedout, EBUSY,ENOMEM...),
                     */
                    /* We have to check for some time
                     * if need reboot for recovery */
                    PRINTF(RED_COL "*** Fail DPM power down : PS-CMD fail (%d)(%d) !!!\n" CLR_COL,
                           pdown_result, dpm_sleep_try_chk_cnt);

                    /* MAC working has something wrong, rebooting for system recovery   */
                    if (dpm_sleep_try_chk_cnt++ >= MAX_CHECK_CNT_PS_CMD_FAIL) {
                        PRINTF(RED_COL ">>> PS-CMD MAX Try Failed , Disconnect !!!\n" CLR_COL);

                        /* MAC working has something wrong,
                         * PS Command for DPM was failed for continuously,
                         * For workaround, let's do reconnect
                         */
                        fc80211_connection_loss();

                        /* Exit DPM daemon task critical section. */
                        //taskEXIT_CRITICAL();

                        dpm_sleep_try_chk_cnt = 0;
                        dpm_mac_err_flag = pdTRUE;
                        dpm_mac_err_max_try_reached_flag = pdTRUE;

                        continue;
                    }

                    dpm_mac_err_flag = pdTRUE;

                    break;
                } /* end of switch (pdown) */
            }

            sleep_delay_cnt = DPM_DAEMON_SLEEP_DELAY_CNT;
        } /* end of if (chk_dpm_mode() && (--sleep_delay_cnt <= 0)) */
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

void init_dpm_sleep_daemon(void)
{
    // Don't start if DPM mode is disabled.
    if (chk_dpm_mode() == pdFALSE)
        return;

    // Create mendatory resources
    if (dpm_daemon_start_flag == pdFALSE) {
        // Create event group for DPM operation
        da16x_dpm_ev_group = xEventGroupCreate();
        if (da16x_dpm_ev_group == NULL) {
            PRINTF("[%s] Failed to create event group !!!\n", __func__);
            return;
        }

        /* Create mutex for DPM register */
        da16x_dpm_reg_req_flag_mutex = xSemaphoreCreateMutex();
        if (da16x_dpm_reg_req_flag_mutex == NULL) {
            PRINTF("[%s] Failed to create DPM register mutex !!!\n", __func__);
            return;
        }

        // Mark create status
        dpm_daemon_start_flag = pdTRUE;
    }
}

void start_dpm_sleep_daemon(int dpm_wu_type)
{
    int result;

    if (chk_dpm_mode() == pdFALSE) {
        return;
    }

    // Create mendatory resources
    init_dpm_sleep_daemon();

    /* Disable DPM Sleep daemon */
    RTM_FLAG_PTR->dpm_sleepd_stop = 0;

    /* Create timer to trigger the dpm_sleep_daemon periodically. */
    dpm_trigger_timer = xTimerCreate(DPM_DAEMON_TRIGGER_TASK_NAME,
                                DPM_TRIGGER_POLLING_TIME,
                                pdTRUE,
                                (void *)NULL,
                                (TimerCallbackFunction_t) dpm_trigger_timer_fn);
    if (dpm_trigger_timer == NULL) {
        if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_ERROR) {
            PRINTF("[%s] Failed to create DPM trigger timer !!!\n", __func__);
        }
    }

    /* For Fast DPM Trigger Starting */    
    xTimerStart(dpm_trigger_timer, 1);    // Starting After 10m sec

    // Create DPM daemon task
    result = xTaskCreate(dpm_sleep_daemon,
                            DPM_SLEEP_DAEMON_NAME,
                            DPM_SLEEPD_STACK_SZ,
                            (void *)dpm_wu_type,
                            DPM_SLEEPD_PRIORITY,
                            (TaskHandle_t *)&dpm_sleep_daemon_handler);
    if (result != pdPASS) {
        PRINTF("[%s] Failed to create DPM daemon task !!!\n", __func__);
    }
}

#if defined ( __SUPPORT_DPM_SLEEP_DELAY__ )
int get_dpm_sleepd_delay(void)
{
    return RTM_FLAG_PTR->dpm_mgr_sleep_delay;
}

void set_dpm_sleepd_delay(int sec)
{
    dpm_sleepd_delay_time = sec;
    RTM_FLAG_PTR->dpm_mgr_sleep_delay = sec;
}
#endif    // __SUPPORT_DPM_SLEEP_DELAY__

static bool chk_dpm_init_done(char *mod_name)
{
    dpm_list_table  *da16x_dpm_list;
    int     i;

    i = 0;
    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];

        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            return (da16x_dpm_list->init_done);
        }
        i++;
    }

    return pdFALSE;

}

int set_dpm_init_done(char *mod_name)
{
    dpm_list_table  *da16x_dpm_list;
    int     i = 0;

    if (chk_dpm_mode() == pdFALSE) {
        return DPM_SET_OK;
    }

    while (i < MAX_DPM_REQ_CNT) {
        da16x_dpm_list = (dpm_list_table *)&_da16x_dpm_sleep_list[i];
        if (dpm_strcmp(da16x_dpm_list->mod_name, mod_name) == 0) {
            da16x_dpm_list->init_done = 1;
            break;
        }

        i++;
        if (i > MAX_DPM_REQ_CNT) {
            return DPM_SET_ERR;
        }
    }

    return DPM_SET_OK;

}

bool confirm_dpm_init_done(char *mod_name)
{
    int retryCount = 0;

    if (mod_name == NULL) {
        return 1;
    }

    if (chk_dpm_mode() == pdFALSE) {
        return 1;
    }

    if (strlen(mod_name) == 0) {
        return 1;
    }

confirm:
    if (chk_dpm_init_done(mod_name)) {
        return 1;
    } else {
        if ( ++retryCount < 50 ) {
            vTaskDelay(2);
            goto confirm;
        } else {
            ;
        }
    }

    return ERR_OK;
}

void show_rtm_map(void)
{
    PRINTF("\n\n"
           "[NAME]                 [RTM Addr] [RTM Offset] [Struct Name]       [Struct Size]\n");

    print_separate_bar('=', 80, 1);

    PRINTF("%-25s0x%x%6d(0x%03x)\n",
            "UMAC START ADDR", RETMEM_APP_UMAC_OFFSET,
            (RETMEM_APP_SUPP_OFFSET-RETMEM_APP_UMAC_OFFSET),
            (RETMEM_APP_SUPP_OFFSET-RETMEM_APP_UMAC_OFFSET));
    print_separate_bar('-', 80, 1);
    PRINTF("%-25s0x%03x\n", "APP START ADDR", RETMEM_APP_SUPP_OFFSET);
    print_separate_bar('-', 80, 1);
    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_FLAG_BASE", RTM_FLAG_BASE,
            (RTM_SUPP_NET_INFO_BASE-RTM_FLAG_BASE), (RTM_SUPP_NET_INFO_BASE-RTM_FLAG_BASE),
            "dpm_flag_in_rtm_t", sizeof(dpm_flag_in_rtm_t), sizeof(dpm_flag_in_rtm_t));

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_SUPP_NET_INFO_BASE", RTM_SUPP_NET_INFO_BASE,
            (RTM_SUPP_IP_INFO_BASE-RTM_SUPP_NET_INFO_BASE),
            (RTM_SUPP_IP_INFO_BASE-RTM_SUPP_NET_INFO_BASE),
            "dpm_supp_net_info_t", sizeof(dpm_supp_net_info_t), sizeof(dpm_supp_net_info_t));

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_SUPP_IP_INFO_BASE", RTM_SUPP_IP_INFO_BASE,
            (RTM_SUPP_CONN_INFO_BASE-RTM_SUPP_IP_INFO_BASE),
            (RTM_SUPP_CONN_INFO_BASE-RTM_SUPP_IP_INFO_BASE),
            "dpm_supp_ip_info_t", sizeof(dpm_supp_ip_info_t), sizeof(dpm_supp_ip_info_t));

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_SUPP_CONN_INFO_BASE", RTM_SUPP_CONN_INFO_BASE,
            (RTM_SUPP_KEY_INFO_BASE-RTM_SUPP_CONN_INFO_BASE),
            (RTM_SUPP_KEY_INFO_BASE-RTM_SUPP_CONN_INFO_BASE),
            "dpm_supp_conn_info_t", sizeof(dpm_supp_conn_info_t), sizeof(dpm_supp_conn_info_t));

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_SUPP_KEY_INFO_BASE", RTM_SUPP_KEY_INFO_BASE,
            (RTM_ARP_BASE - (RTM_SUPP_KEY_INFO_BASE + SPACE_ALLOC_SZ)),
            (RTM_ARP_BASE - (RTM_SUPP_KEY_INFO_BASE + SPACE_ALLOC_SZ)),
            "dpm_supp_key_info_t", sizeof(dpm_supp_key_info_t), sizeof(dpm_supp_key_info_t));

    PRINTF("%-25s0x%x%6d(0x%03x) (Free Space)\n",
            "RTM_SPACE_BASE", RTM_SPACE_BASE,
            (RTM_ARP_BASE - RTM_SPACE_BASE), (RTM_ARP_BASE-RTM_SPACE_BASE));


    PRINTF("%-25s0x%x%6d(0x%03x)\n",
            "RTM_ARP_BASE", RTM_ARP_BASE,
            RTM_MDNS_BASE-RTM_ARP_BASE, RTM_MDNS_BASE-RTM_ARP_BASE);

    PRINTF("%-25s0x%x%6d(0x%03x)\n",
            "RTM_MDNS_BASE", RTM_MDNS_BASE,
            RTM_DNS_BASE-RTM_MDNS_BASE, RTM_DNS_BASE-RTM_MDNS_BASE);

    PRINTF("%-25s0x%x%6d(0x%03x)\n",
            "RTM_DNS_BASE", RTM_DNS_BASE,
            RTM_OTP_BASE-RTM_DNS_BASE, RTM_OTP_BASE-RTM_DNS_BASE);

    PRINTF("%-25s0x%x%6d(0x%03x)\n",
            "RTM_OTP_BASE", RTM_OTP_BASE,
            RTM_RTC_TIMER_BASE-RTM_OTP_BASE,
            RTM_RTC_TIMER_BASE-RTM_OTP_BASE);

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_RTC_TIMER_BASE", RTM_RTC_TIMER_BASE,
            RTM_DPM_MONITOR_BASE-RTM_RTC_TIMER_BASE,
            RTM_DPM_MONITOR_BASE-RTM_RTC_TIMER_BASE,
            "dpm_timer_list_t",  sizeof(dpm_timer_list_t), sizeof(dpm_timer_list_t));

    PRINTF("%-25s0x%x%6d(0x%03x) %-22s%4d(0x%03x)\n",
            "RTM_DPM_MONITOR_BASE", RTM_DPM_MONITOR_BASE,
            MONITOR_ALLOC_SZ,
            MONITOR_ALLOC_SZ,
            "dpm_monitor_info_t",  sizeof(dpm_monitor_info_t), sizeof(dpm_monitor_info_t));

    print_separate_bar('-', 80, 1);
    PRINTF("%-25s0x%x\n", "RTM_RLIB_BASE", RTM_RLIB_BASE);
    PRINTF("%-25s0x%x\n", "RTM_TIM_BASE", RTM_TIM_BASE);

    print_separate_bar('-', 80, 1);

    PRINTF("%-25s0x%x%5d(0x%04x)\n",
            "RTM_TCP_BASE", RTM_TCP_BASE,
            RTM_TCP_KA_BASE-RTM_TCP_BASE, RTM_TCP_KA_BASE-RTM_TCP_BASE);

    PRINTF("%-25s0x%x%5d(0x%04x)\n",
            "RTM_TCP_KA_BASE", RTM_TCP_KA_BASE,
            RTM_USER_DATA_BASE-RTM_TCP_KA_BASE,
            RTM_USER_DATA_BASE-RTM_TCP_KA_BASE);

    print_separate_bar('-', 80, 1);
    PRINTF("%-25s0x%x%5d(0x%04x)\n",
            "RTM_USER_POOL_BASE", RTM_USER_POOL_BASE,
            RTM_USER_DATA_BASE-RTM_USER_POOL_BASE,
            RTM_USER_DATA_BASE-RTM_USER_POOL_BASE);

    PRINTF("%-25s0x%x%5d(0x%04x)\n",
            "RTM_USER_DATA_BASE", RTM_USER_DATA_BASE,
            RETMEM_APP_END_OFFSET-RTM_USER_DATA_BASE,
            RETMEM_APP_END_OFFSET-RTM_USER_DATA_BASE);

    PRINTF("%-25s0x%x\n",
            "END ADDR", RETMEM_APP_END_OFFSET+1);

    print_separate_bar('=', 80, 1);
}

void init_dpm_environment(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    set_dpm_supp_ptr();    // for Supplicant

    da16x_dpm_socket_init(); // for TCP session

    dpm_user_rtm_pool_create();    //for user RTM region

    /* RTC Timer Mutex */
    if (dpm_timer_mutex == NULL) {
        dpm_timer_mutex = xSemaphoreCreateMutex();
    }
    
    if (dpm_timer_mutex == NULL) {
        PRINTF("\n[DPM_TM] dpm_timer_mutex create error !!!\n");
    }

    if (dpm_dbg_cmd_flag == pdTRUE) {
        /* Show rtm memory map */
        show_rtm_map(); 
    }

    if (chk_dpm_wakeup() == pdFALSE) {
        unsigned int dtim_period;

        /* DPM MAC Connection Information clear */
        umac_dpm_rtm_info_clear();

        /* Clear RTM - Normal booting */
        clr_dpm_conn_info();

        /* Load  keepalive */
        set_dpm_keepalive_config(get_dpm_keepalive_from_nvram());

        /* Load time wakeup type */
        set_dpm_wakeup_condition(get_dpm_wakeup_condition_from_nvram());

        dtim_period = get_dpm_TIM_wakeup_time_from_nvram();
        /* Load tim wakeup time */
        set_dpm_tim_wakeup_dur(dtim_period, 0);

        /* Load tim wakeup time into umac infor */
        set_dpm_tim_wakeup_time_rtm(dtim_period);

#if defined ( WORKING_FOR_DPM_TCP )
        /* clear tcp network info */
        clr_all_dpm_tcp_ka_info();
#endif    // WORKING_FOR_DPM_TCP

        da16x_dpm_socket_clear_all_tcp_sess_info();

        /* RTC timeout flag clear */
        RTM_FLAG_PTR->dpm_rtc_timeout_flag = 0;
        RTM_FLAG_PTR->dpm_rtc_timeout_tid = 0;

        if (dpm_dbg_cmd_flag == pdTRUE) {
            /* Set default debug level */
            set_dpm_dbg_level(MSG_ERROR);
        }
    }
}


//////
unsigned int current_usec_count(void)
{
    unsigned int    cur_usec;
    unsigned long long rtc_cur_time = 0;

    extern int fc80211_get_rtccurtime(int, unsigned long long *);

    fc80211_get_rtccurtime(0, &rtc_cur_time);
    cur_usec = (unsigned int)(rtc_cur_time / 10000);

    return cur_usec;
}

void set_dpm_supp_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    /* Supplicant Basic WiFi infomation */
    dpm_supp_conn_info = RTM_SUPP_CONN_INFO_PTR;

    /* Supplicant Key infomation */
    dpm_supp_key_info = RTM_SUPP_KEY_INFO_PTR;
}

unsigned char *get_da16x_dpm_dns_cache(void)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return NULL;
    }

    return RTM_DNS_PTR;
}

unsigned char *get_da16x_dpm_mdns_ptr(void)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return NULL;
    }

    return RTM_MDNS_PTR;
}

unsigned char *get_da16x_dpm_arp_ptr(void)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return NULL;
    }

    return RTM_ARP_PTR;
}

int chk_dpm_sleepd_hold_state(void)
{
    return dpm_daemon_hold_flag;
}

void hold_dpm_sleepd(void)
{
    dpm_daemon_hold_flag = 1;

    /* Clear Data-Rx blocking */
    unset_wifi_driver_not_rcv(0);
}

void resume_dpm_sleepd(void)
{
    dpm_daemon_hold_flag = 0;
}

void *get_supp_net_info_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_SUPP_NET_INFO_PTR;
}

void *get_supp_ip_info_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_SUPP_IP_INFO_PTR;
}

void *get_supp_conn_info_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_SUPP_CONN_INFO_PTR;
}

void *get_supp_key_info_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_SUPP_KEY_INFO_PTR;
}

int get_supp_conn_state(void)
{
    if (chk_retention_mem_exist() == pdFAIL || chk_dpm_mode() == pdFALSE) {
        /* Unsupport RTM */
        return dpm_wpa_supp_state;
    }

    return RTM_FLAG_PTR->dpm_supp_state;
}

const char *dpm_restore_country_code(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return RTM_SUPP_NET_INFO_PTR->country;
}

void dpm_save_country_code(const char *country)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    memset((void *)RTM_SUPP_NET_INFO_PTR->country, 0, 4);
    memcpy((void *)RTM_SUPP_NET_INFO_PTR->country, country, 3);
}

int chk_supp_connected(void)
{
    if (chk_retention_mem_exist() == pdFAIL || chk_dpm_mode() == pdFALSE) {
        /* Unsupport RTM */
        if (dpm_wpa_supp_state == WPA_COMPLETED) {
            return pdPASS;
        } else {
            return pdFAIL;
        }
    }

    if (RTM_FLAG_PTR->dpm_supp_state == WPA_COMPLETED) {
        return pdPASS;
    }

    return pdFAIL;
}

void save_supp_conn_state(int supp_state)
{
    dpm_wpa_supp_state = supp_state;

    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    RTM_FLAG_PTR->dpm_supp_state = supp_state;
}

void clr_dpm_conn_info(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    /* Supplicant Basic WiFi infomation */
    memset((void *)RTM_SUPP_CONN_INFO_PTR, 0, CONN_INFO_ALLOC_SZ);

    /* Supplicant Key infomation */
    memset((void *)RTM_SUPP_KEY_INFO_PTR, 0, KEY_INFO_ALLOC_SZ);

#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
    if (get_last_abnormal_cnt() > 0 || chk_dpm_wakeup() == pdFALSE) {
        /* Net mode - DHCPC : 1 */
        if (RTM_SUPP_NET_INFO_PTR->net_mode == 1) {
            /* IP Address */
            memset((void *)RTM_SUPP_IP_INFO_PTR, 0, NET_IP_ALLOC_SZ);
        }
    }
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */

    /* ARP Table */
    memset((void *)RTM_ARP_PTR, 0, ARP_ALLOC_SZ);

    clr_dpm_sleep(REG_NAME_DPM_SUPPLICANT);
}

void reset_dpm_info(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return;
    }

    hold_dpm_sleepd();
    disable_dpm_mode();
    clr_dpm_conn_info();

    // DNS
    // mdns
    // Http
}

void set_dpm_wakeup_condition(unsigned int type)
{
	DA16X_UNUSED_ARG(type);
}

void dpm_set_dtim_period(unsigned long dtim_period)
{
    unsigned long bcn_int = dpm_get_env_bcn_int(GET_DPMP());
    long long sleep_time = dtim_period * 102400;

    if (bcn_int == 0) {
        bcn_int = 100;
    }

    dtim_period = sleep_time / (bcn_int * 1024);
    dpm_set_env_force_period(GET_DPMP(), dtim_period);
}

/**
 * TIM wakeup time setting
 */
void set_dpm_tim_wakeup_dur(unsigned int dtim_period, int saveflag)
{
    if (dpm_dbg_cmd_flag == pdTRUE && get_dpm_dbg_level() >= MSG_EXCESSIVE) {
        PRINTF("- DPM Tim Wakeup time : %d * 100 msec\n", dtim_period);
    }

    /* In case of DTIM Perod is 0, is not Force DTIM mode, it follow the AP dtim period */
    if (dtim_period == 0) {
        dpm_set_dtim_period(dtim_period); //parameter is dtim period
    } else { /* Force DTIM Period */
        /**  DTPM Period range : 1 ~ 255
         ** In case of DTIM Period is 1, let's do not set
         */
        if (dtim_period < MIN_DPM_TIM_WAKEUP || dtim_period > MAX_DPM_TIM_WAKEUP) {
            PRINTF("DPM TIM wake up range: 1 ~ 6000 \n");
            return;
        }

        dpm_set_dtim_period(dtim_period); //parameter is dtim period
    }

    if (saveflag) {
        PRINTF("- DPM Tim Wakeup time : %d * 100 msec\n", dtim_period);
        /* If AP connection is done, you should reboot */
        set_dpm_TIM_wakeup_time_to_nvram(dtim_period);
        PRINTF("- AP Connection is already done. System reboot to apply...\n");
    }
}

/** set the tim bcn timeout
 */
void set_dpm_bcn_wait_timeout(unsigned int time)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return;
    }

    //dpm_set_tim_bcn_timeout(time*1000);
    dpm_set_env_tim_bcn_timeout_tu(GET_DPMP(), US2TU(time*1000));
    return;
}

/** set the tim bcn no bcn check step
 */
void set_dpm_nobcn_check_step(int step)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return;
    }

    //dpm_set_dtim_bcn_step(step);
    dpm_set_env_tim_chk_step(GET_DPMP(), step);
    return;
}


/**
 * IP address is saved to retention memroy For ARP Filtering.
 * &
 * Subnet Broadcast address is saved to retention memroy For GARP packet generating.
 */
void dpm_accept_arp_broadcast(unsigned long accept_addr, unsigned long subnet_addr)
{
    ULONG     tmp_ip_addr;
    UINT32     sub_bcast;

    tmp_ip_addr = accept_addr;
    dpm_set_env_ip(GET_DPMP(), tmp_ip_addr);

    /* Subnet broadcast address Calculation */
    sub_bcast = accept_addr | (subnet_addr ^ 0xffffffff);
    dpm_set_env_ap_ip(GET_DPMP(), sub_bcast);

    return;
}

void dpm_accept_tim_arp_resp(unsigned long gw_ip, unsigned char *gw_mac)
{
    ULONG     tmp_ip;

    tmp_ip = gw_ip;

    /* For ARP Processing in TIM, set the GW MAC address */
    dpm_set_env_arp_target_ip(GET_DPMP(), tmp_ip);
    dpm_set_env_arp_target_mac(GET_DPMP(), (unsigned char *) gw_mac);

    /* For ARP request processing in TIM, set the GW MAC address */
    dpm_set_env_arpreq_target_ip(GET_DPMP(), tmp_ip);
    dpm_set_env_arpreq_target_mac(GET_DPMP(), (unsigned char *) gw_mac);

    /* if udp hole punch is used, we should set the gw mac address */
    dpm_set_env_udph_target_mac(GET_DPMP() , (unsigned char *) gw_mac);

    return;
}

/** Multicast mac address(last 3 bytes) is passing
 * mc_filter_cnt is 0 ~ 7
 */
#define dpm_swab32(x) ((UINT32)(                \
    (((UINT32)(x) & (UINT32)0x000000ffUL) << 24) |        \
    (((UINT32)(x) & (UINT32)0x0000ff00UL) <<  8) |        \
    (((UINT32)(x) & (UINT32)0x00ff0000UL) >>  8) |        \
    (((UINT32)(x) & (UINT32)0xff000000UL) >> 24)))

void set_dpm_mc_filter( unsigned long before_mc_addr)
{
    unsigned char mc_filter_cnt;
    unsigned long mc_oui_addr, mc_addr , after_mc_addr;
    struct dpm_param *dpmp = GET_DPMP();
    unsigned char index = 0;

    after_mc_addr = dpm_swab32(before_mc_addr);
    
    for (index = 0; index < DPM_ACCEPT_MC_CNT ; index++) {
        mc_addr = dpm_get_env_mc_ip(GET_DPMP(), index);
        if (mc_addr == 0) {
            break;
        }

        if (mc_addr == after_mc_addr) {
#ifdef ENABLE_DPM_DBG_LOG
            PRINTF("[DPM] %s Reg MC IP(%08x) is already registered\n", __func__, before_mc_addr);
#endif    /* ENABLE_DPM_DBG_LOG */
            return;
        }
    }
    
    if (index >= DPM_ACCEPT_MC_CNT) {
#ifdef ENABLE_DPM_DBG_LOG
        PRINTF("[DPM] %s Reg MC IP(%d:%08x) Over\n", __func__, index , before_mc_addr);
#endif    /* ENABLE_DPM_DBG_LOG */
        return;
    }
        
    mc_filter_cnt = index;
    mc_addr = after_mc_addr;

    PRINTF("[DPM] Set Multicast Address Filter(%dth %08x)\n", mc_filter_cnt , mc_addr);

    if (!(dpm_get_env_rx_filter(dpmp) & (DPM_F_MATCHED_MC_IP)))
        dpm_set_env_rx_filter(dpmp, dpm_get_env_rx_filter(dpmp) | DPM_F_MATCHED_MC_IP);

    /* multicast ip filter set */
    dpm_set_env_mc_ip(dpmp , mc_filter_cnt , mc_addr);

    /* multicast mac filter set */
    mc_oui_addr = dpm_swab32(before_mc_addr & 0x00ffffff);
    dpm_set_env_bcmc_accept_oui_n(dpmp , mc_filter_cnt , mc_oui_addr);

    return;
}

/** UDP Port filter Primitive
 * udp filter max cnt < 7
 */
void set_dpm_udp_port_filter(unsigned short d_port)
{
    unsigned char no , index = 0;
    unsigned short    reg_port = 0;

    for (index = 0; index < DPM_MAX_UDP_FILTER ; index++) {
        reg_port = dpm_get_env_udp_dport(GET_DPMP(), index);
        if (reg_port == 0) break;
        if (d_port == reg_port) {
#ifdef ENABLE_DPM_DBG_LOG
            PRINTF("[DPM] %s Reg UDP Port(%x) is already registered\n", __func__, d_port);
#endif    /* ENABLE_DPM_DBG_LOG */
            return;
        }
    }

    no = index;

    if (no >= DPM_MAX_UDP_FILTER) {
#ifdef ENABLE_DPM_DBG_LOG
        PRINTF("[DPM] %s Reg UDP Port number(%d) Over\n", __func__, no);
#endif    /* ENABLE_DPM_DBG_LOG */
        return;
    }

#ifdef ENABLE_DPM_DBG_LOG
    PRINTF("\n[%s] New UDP filter[%d] d_port=%d\n", __func__, no, d_port);
#endif    /* ENABLE_DPM_DBG_LOG */
    dpm_set_env_udp_dport(GET_DPMP(), no, d_port);
    return;
}

void dpm_print_regi_tcp_port(void)
{
    unsigned char index = 0;

    for (index = 0; index < DPM_MAX_TCP_FILTER ; index++) {
        PRINTF("tcp_port[%d] = %d\n", index, dpm_get_env_tcp_dport(GET_DPMP(), index));
    }
}

/** TCP Port filter Primitive
 * tcp filter max cnt < 7
 */
void set_dpm_tcp_port_filter(unsigned short d_port)
{
    unsigned char no , index = 0;
    unsigned short    reg_port = 0;

    for (index = 0; index < DPM_MAX_TCP_FILTER ; index++) {
        reg_port = dpm_get_env_tcp_dport(GET_DPMP(), index);
        if (reg_port == 0) break;
        if (d_port == reg_port) {
#ifdef ENABLE_DPM_DBG_LOG
            PRINTF("[DPM] %s Reg TCP Port(%d) is already registered\n", __func__, d_port);
#endif    /* ENABLE_DPM_DBG_LOG */
            return;
        }
    }

    no = index;

    if (no >= DPM_MAX_TCP_FILTER) {
#ifdef ENABLE_DPM_DBG_LOG
        PRINTF("[DPM] %s Reg TCP Port number(%d) Over\n", __func__, no);
#endif    /* ENABLE_DPM_DBG_LOG */
        return;
    }

#ifdef ENABLE_DPM_DBG_LOG
    PRINTF("\n[%s] New TCP filter[%d] d_port=%d\n", __func__, no, d_port);
#endif    /* ENABLE_DPM_DBG_LOG */
    dpm_set_env_tcp_dport(GET_DPMP() , no, d_port);

    return;
}

void get_dpm_tcp_port_filter(void)
{
    unsigned char index = 0;
    unsigned short reg_port = 0;
    
    for (index = 0; index < DPM_MAX_TCP_FILTER ; index++) {
        reg_port = dpm_get_env_tcp_dport(GET_DPMP(), index);
        if (reg_port == 0) {
            break;
        }

        PRINTF(" [DPM] %s %dth Reg TCP Port(%d)\n", __func__, index, reg_port);
    }

    return;
}

void del_dpm_udp_port_filter(unsigned short d_port)
{
    unsigned char index = 0;
    unsigned short    reg_port = 0;

    for (index = 0; index < DPM_MAX_UDP_FILTER ; index++) {
        reg_port = dpm_get_env_udp_dport(GET_DPMP(), index);
        if (d_port == reg_port) {
            dpm_set_env_udp_dport(GET_DPMP(), index, 0);
            return;
        }
    }
}

void del_dpm_tcp_port_filter(unsigned short d_port)
{
    unsigned char index = 0;
    unsigned short    reg_port = 0;

    for (index = 0; index < DPM_MAX_TCP_FILTER ; index++) {
        reg_port = dpm_get_env_tcp_dport(GET_DPMP(), index);
        if (d_port == reg_port) {
#ifdef  ENABLE_DPM_DBG_LOG
            PRINTF("[DPM] %s Reg TCP Port(%d) is deleted !\n", __func__, d_port);
#endif  /* ENABLE_DPM_DBG_LOG */
            dpm_set_env_tcp_dport(GET_DPMP(), index, 0);
            return;
        }
    }
}

/** UDP Port filter Control Primitive */
void dpm_udpf_cntrl(unsigned char en_flag)
{
    struct dpm_param *dpmp = GET_DPMP();

    /* UDP Filter enable, unconditional drop except register port */
    if (en_flag) {
        if (!(dpm_get_env_rx_filter(dpmp) & (DPM_F_DROP_UDP_SERVICE)))
            dpm_set_env_rx_filter(dpmp, dpm_get_env_rx_filter(dpmp) | DPM_F_DROP_UDP_SERVICE);
    }
    /* UDP Filter disable, unconditional udp packet accept and wakeup */
    else {
        if (dpm_get_env_rx_filter(dpmp) & (DPM_F_DROP_UDP_SERVICE))
            dpm_set_env_rx_filter(dpmp, (dpm_get_env_rx_filter(dpmp) & ~(DPM_F_DROP_UDP_SERVICE)));
    }
}

/** TCP Port filter Control Primitive
 */
void dpm_tcpf_cntrl(unsigned char en_flag)
{
    struct dpm_param *dpmp = GET_DPMP();

    /* TCP Filter enable, unconditional drop except register port */
    if (en_flag) {
        if (!(dpm_get_env_rx_filter(dpmp) & (DPM_F_DROP_TCP_SERVICE))) {
            dpm_set_env_rx_filter(dpmp, dpm_get_env_rx_filter(dpmp) | DPM_F_DROP_TCP_SERVICE);
        }
    }
    /* UDP Filter disable, unconditional udp packet accept and wakeup */
    else {
        if (dpm_get_env_rx_filter(dpmp) & (DPM_F_DROP_TCP_SERVICE)) {
            dpm_set_env_rx_filter(dpmp, dpm_get_env_rx_filter(dpmp) & ~(DPM_F_DROP_TCP_SERVICE));
        }
    }
}

#ifdef __SUPPORT_DPM_TIM_TCP_ACK__
void dpm_set_tcpka_target_mac(UINT8 *dest_mac)
{
    struct dpm_param *dpmp = GET_DPMP();

    dpm_set_env_tcpka_target_mac(dpmp,(unsigned char *)dest_mac);
}


void dpm_set_tcpack_tx_func(int flag)
{
    struct dpm_param *dpmp = GET_DPMP();

    if (flag) {
        dpm_set_env_tcpka_en(dpmp);
    } else {
        dpm_reset_env_tcpka_en(dpmp);
    }
}

void set_dpm_tim_tcp_chkport_enable()
{
    struct dpm_param *dpmp = GET_DPMP();

    // Run ChkPort when the application calls dpm_setup
    dpm_set_env_tcpka_chkport_en(dpmp);
}
#else
void dpm_set_tcpka_target_mac(UINT8 *dest_mac)
{
    return;
}

void dpm_set_tcpack_tx_func(int flag)
{
    return;
}

void set_dpm_tim_tcp_chkport_enable()
{
    return;
}
#endif    // __SUPPORT_DPM_TIM_TCP_ACK__

/** UDP Hole punch Primitive */
void dpm_udp_hole_punch(int period /* keep period times */,
        unsigned long dest_ip, unsigned short src_port,
        unsigned short dest_port)
{
    struct dpm_param *dpmp = GET_DPMP();
    unsigned short udph_sport = src_port;
    unsigned short udph_dport = dest_port;
    unsigned long udph_dip = dest_ip;

    dpm_set_env_udph_sport(dpmp, udph_sport);
    dpm_set_env_udph_dport(dpmp, udph_dport);
    dpm_set_env_udph_dip(dpmp, udph_dip);

    // If period is non-zero then UDPH is enabled
    if (period) {
        dpm_set_env_udph_en(dpmp);
    } else {
        dpm_reset_env_udph_en(dpmp);
    }

    // UDPH period multiples KA period by period.
    // It is calculated from dpm_sche_update.
    dpm_set_env_udph_period_ka(dpmp, period);

    return;
}

/**
 * Before DPM sleep, if DPM setup is called, we have to reconfigure the UDP Hole punch config
 */
void dpm_udp_hole_punch_reconfig(void)
{
    return;
}

void dpm_delete_udp_hole_punch(void)
{
    return;
}

void dpm_reset_env_udph_enable(void)
{
    dpm_reset_env_udph_en(GET_DPMP());
}


/* DDPS related Setting */
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
#define    DDPS_TIM_WAKEUP_DEFAULT    30

/* TIM Wakeup change as 3sec callback function */
void (*ddps_tim_wakeup_chg_3sec_cb)(void) = NULL;

/* TIM Wakeup change as 1sec callback function */
void (*ddps_tim_wakeup_chg_1sec_cb)(void) = NULL;

void ddps_wakeup_chg_noti_cb_reg(void (*user_1sec_cb)(void) , void (*user_3sec_cb)(void))
{
    ddps_tim_wakeup_chg_1sec_cb = user_1sec_cb;
    ddps_tim_wakeup_chg_3sec_cb = user_3sec_cb;
}

/** 
 * POR / RESET / REBOOT , For DPM TIM WAKEUP TIME is set automatically by TIM BUFP 
 * So, After TIM BUFP, if TIM Wakeup has to be changed, RTOS Wakeup is enabled
 */ 
void dpm_tim_bufp_chk_en(void)
{
    struct dpm_param *dpmp = GET_DPMP();

    /* Current Tim wakeup interval is Default , Try DDPS(BUFP) */
    PRINTF("[DDPS] TIM DDPS-BUFP Enabled\n");
        
    /* After TIM BUFP Working, RTOS Wakeup is enable */
    dpm_set_env_bufp_done_en(dpmp);
        
    /* DPM TIM BUFP Enable */
    dpm_set_env_bufp_en(dpmp);
}    

void set_dpm_DynamicPeriodSet_config(int flag)
{
    dpm_write_nvram_int(NVR_KEY_DPM_DYNAMIC_PERIOD_SET, flag, DFLT_DPM_DYNAMIC_PERIOD_SET);
}

int get_dpm_DynamicPeriodSet_config(void)
{
    int value;

    dpm_read_nvram_int(NVR_KEY_DPM_DYNAMIC_PERIOD_SET, &value);

    if (value == -1) {
        return DFLT_DPM_DYNAMIC_PERIOD_SET;
    }

    return value;
}

char dpm_get_tim_ddps_bufp_chk(void)
{
    struct dpm_param *dpmp = GET_DPMP();
    
    if ( get_dpm_DynamicPeriodSet_config()) {
        /* If current is default TIM Wakeup, ddps bufp is enabled */
        /* DDPS Default is 30 */
        return dpm_get_env_bufp_en(dpmp);
    }
    return 0;
}

void dpm_set_tim_bufp_chk_param(void)
{
    struct dpm_param *dpmp = GET_DPMP();
    int  probe_status , arp_status;
    unsigned int tim_waktup_int;
    unsigned short get_bufp_chk_period, tst_bufp_chk_period , bufp_chk_period;
    unsigned char bufp_period_ty , bufp_probe_cnt;
    long bufp_period;

    probe_status = arp_status = 0;

    /* DDPS Setting is not valid */
    if (!get_dpm_DynamicPeriodSet_config()) {
        return;
    }
        
    if (dpm_get_st_bufp_status(dpmp) == DPM_BUFP_ERROR) {
        PRINTF("[DDPS] TIM BUFP working is error and done\n");
        set_dpm_DynamicPeriodSet_config(0);
        return;
    }

    // DDPS Check Period Probe status
    probe_status = dpm_get_st_bufp_probe_status(dpmp);

    // DDPS Check Period ARP packet rx status
    arp_status = dpm_get_st_bufp_arp_status(dpmp);

    // DDPS BUFP Ceck Period
    bufp_period_ty = dpm_get_env_bufp_period_ty(dpmp);
    bufp_period = DPMSCHE_GET_P2I(dpmp, bufp_period_ty, 1);
    bufp_chk_period = dpm_get_env_bufp_chk_period(dpmp);        

    get_bufp_chk_period = bufp_chk_period * bufp_period;

    // DDPS BUFP Max count
    tst_bufp_chk_period = dpm_get_st_bufp_max_cnt(dpmp);

    // DDPS BUFP Prbe count (include retry)
    bufp_probe_cnt = dpm_get_st_bufp_probe_cnt(dpmp);
    
    PRINTF("[DDPS] Period %d, Max Cnt  %d, Probe %d(cnt %d), Arp Rep %d\n", get_bufp_chk_period, tst_bufp_chk_period , probe_status,bufp_probe_cnt, arp_status);

    if (bufp_probe_cnt > 1) {
        // BUFP probe success
        // Get the Current Tim wakeup interval 
        tim_waktup_int = get_dpm_TIM_wakeup_time_from_nvram();

        /* if tim wakeup interval is not 3 sec , set as 30 (3sec) */
        if ( tim_waktup_int != DDPS_TIM_WAKEUP_DEFAULT) {
            PRINTF("[DDPS] TIM Wakeup Time is changed as 30(3 sec)\n" );        
            set_dpm_TIM_wakeup_time_to_nvram(DDPS_TIM_WAKEUP_DEFAULT);

            vTaskDelay(1);    

            /* if callback is Registered, called by customer features */
            if (ddps_tim_wakeup_chg_3sec_cb != NULL) {
                ddps_tim_wakeup_chg_3sec_cb();
            }
        }
    } else {
        // BUFP probe failure
        tim_waktup_int = get_dpm_TIM_wakeup_time_from_nvram();
        if ( tim_waktup_int != 10) {
            PRINTF("[DDPS] TIM Wakeup Time is changed as default 10(1 sec)\n");        
            set_dpm_TIM_wakeup_time_to_nvram(10);

            vTaskDelay(1);    
            /* if callback is Registered, called by customer features */
            if (ddps_tim_wakeup_chg_1sec_cb != NULL) {
                ddps_tim_wakeup_chg_1sec_cb();
            }
        }
    }

    /* set as DDPS Working is done. 0 means done */
    set_dpm_DynamicPeriodSet_config(0);
}

#else
void dpm_tim_bufp_chk_en()
{
    return;
}    

void dpm_set_tim_bufp_chk_param()
{
    return;
}

char dpm_get_tim_ddps_bufp_chk()
{
    return 0;
}

#endif

/* auto arp enable primitive */
extern unsigned char dpm_cmd_autoarp_en;
static unsigned char dpm_cmd_autoarp_period = 1;
void dpm_arp_en(int period, int mode)
{
    dpm_cmd_autoarp_en = mode;
    dpm_cmd_autoarp_period = period;
}

/* auto arp enable primitive */
void dpm_arp_en_timsch(int mode)
{
    struct dpm_param *dpmp = GET_DPMP();

    if (mode) {
        dpm_set_env_arp_en(dpmp);
    } else {
        dpm_reset_env_arp_en(dpmp);
    }

    dpm_set_env_arp_period_ka(dpmp, dpm_cmd_autoarp_period);
}

/* ARP Req Primitive to default gw */
void dpm_arp_req_en(int period, int mode)
{
    struct dpm_param *dpmp = GET_DPMP();

    if (mode) {
        dpm_set_env_bufp_en(dpmp);
    } else {
        dpm_reset_env_bufp_en(dpmp);
    }

    dpm_set_env_arpreq_period_ka(dpmp, period);
}

/* ARP Resp timeout set Primitive */
void dpm_set_arp_resp(int bc_timeout) /* usec */
{
    struct dpm_param *dpmp = GET_DPMP();

    dpm_set_env_arpresp_timeout_tu(dpmp , bc_timeout);
}


/* DPM Parameter setting for TIM Working */
void dpm_set_ap_params(unsigned short bcn_int, unsigned char dtim_period)
{
    /* beacon interval set */
    dpm_set_env_bcn_int(GET_DPMP(), bcn_int);

    /* dtim period set */
    if (dtim_period != 0) {
        dpm_set_env_dtim_period(GET_DPMP(), dtim_period);
    }

    return;
}

/* Current DTIM Period of DPM Prameters */
unsigned char dpm_get_ap_params_dtim_period(void)
{
    return dpm_get_env_dtim_period(GET_DPMP());
}

/** AP Sync Enable/Disable **/
static unsigned char main_ap_sync_cntrl = 1; //AP Sync enable as default
void dpm_set_tim_ap_sync_cntrl(char ap_sync_flag)
{
    /* TIM AP Sync Enable */
    if (ap_sync_flag) {
        dpm_set_aptrk_en(  GET_DPMP(),
                           APTRK_MODE_COARSE_LOCK
                         | APTRK_MODE_FINE_LOCK
                         | APTRK_MODE_UNDER_TRACKING
                         | APTRK_MODE_OVER_TRACKING
                         | APTRK_MODE_SLOPE_MEASURE);

        main_ap_sync_cntrl = 0;
    } else  {    /* TIM AP Sync Diable */
        dpm_set_aptrk_en(GET_DPMP(), APTRK_MODE_DISABLE);
        main_ap_sync_cntrl = 1;
    }
}

unsigned char dpm_get_tim_ap_sync_cntrl(void)
{
    // If AP Sync is disabled as Command, return 0
    if (!main_ap_sync_cntrl) {
        return 0;
    } else {
        return 1;
    }
}

void set_dpm_tim_data_seqnum(unsigned short seq_number)
{
	DA16X_UNUSED_ARG(seq_number);

#if 0
    UINT32  conf_addr = DPM_TIM_DATA_SEQ_NUM;

    seq_number -= 0x10;        // TIM use it after Plus.
    *((volatile USHORT *)(conf_addr)) = (seq_number >> 4) & 0x0fff;
#endif
}

USHORT get_dpm_tim_data_seqnum(void)
{
#if 0
    UINT32    conf_addr = DPM_TIM_DATA_SEQ_NUM;
    UINT16    tmp_seqnum , seqnum;

    tmp_seqnum = *(volatile USHORT *)(conf_addr);

    seqnum = (tmp_seqnum & 0xffff) << 4;
    //PRINTF("[UMAC Seq Num] ret_seqnum 0x%04x seqnum 0x%04x\n", ret_seqnum , seqnum);
    seqnum += 0x10;        // Main use it after plus
    return seqnum;
#else
    return 0;
#endif
}

void set_dpm_keepalive_config(int duration)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return;
    }

    RTM_FLAG_PTR->dpm_keepalive_time_msec = duration;

    return;
}

int get_dpm_keepalive_config(void)
{
    if (!chk_rtm_exist()) {
        /* Unsupport RTM */
        return -1;
    }

    return RTM_FLAG_PTR->dpm_keepalive_time_msec;
}



/// NVRAM ////////////////////////////////////////

int dpm_read_nvram_int(const char *name, int *_val)
{
    int   val;
    char  *valstr;

    valstr = da16x_getenv(NVEDIT_SFLASH, "app", (char *)name);

    if (valstr == NULL) {
        *_val = -1;
        return -1;
    }

    val = atoi(valstr);
    *_val = val;

    return 0;
}

int dpm_write_tmp_nvram_int(const char *name, int val, int def)
{
	DA16X_UNUSED_ARG(def);

    char valstr[11];
    char *valstr_env = NULL;

    valstr_env = da16x_getenv(NVEDIT_SFLASH, "app", (char *)name);

    if (valstr_env != NULL && atoi(valstr_env) == val) {
        return 0;
    }

    memset((void *)valstr, 0, 11);
    sprintf(valstr, "%d", val);

    /* At this case,,, dpm_sleep operation was started already */
    if (chk_dpm_pdown_start()) {
        PRINTF(RED_COL "NVRAM_WR: In progressing DPM sleep !!!" CLR_COL);
        return 0;
    }

    if (da16x_setenv_temp(NVEDIT_SFLASH, "app", (char *)name, valstr) == 0) {
        PRINTF("[%s] Failed to set %s=%s: error %d\n", __func__, name, valstr, val);
        return -1;
    }

    return 0;
}

int dpm_write_nvram_int(const char *name, int val, int def)
{
	DA16X_UNUSED_ARG(def);

    char valstr[11];
    char *valstr_env = NULL;

    valstr_env = da16x_getenv(NVEDIT_SFLASH, "app", (char *)name);

    if (valstr_env != NULL && atoi(valstr_env) == val) {
        return 0;
    }

    memset((void *)valstr, 0, 11);
    sprintf(valstr, "%d", val);

    /* At this case,,, dpm_sleep operation was started already */
    if (chk_dpm_pdown_start()) {
        PRINTF(RED_COL "NVRAM_WR: In progressing DPM sleep !!!" CLR_COL);
        return 0;
    }

    if (da16x_setenv(NVEDIT_SFLASH, "app", (char *)name, valstr) == 0) {
        PRINTF("[%s] Failed to set %s=%s: error %d\n", __func__, name, valstr, val);
        return -1;
    }

    return 0;
}

int get_dpm_mode_from_nvram(void)
{
    int dpm_mode = 0;

    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return 0;
    }

    if (dpm_read_nvram_int("dpm_mode", &dpm_mode) == -1) {
        RTM_FLAG_PTR->dpm_mode = 0;
    } else {
        RTM_FLAG_PTR->dpm_mode = dpm_mode;
    }

    return RTM_FLAG_PTR->dpm_mode;
}

/* Keppalive time */
int get_dpm_keepalive_from_nvram(void)
{
    int value;

    dpm_read_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, &value);
    if (value == -1) {
        return DFLT_DPM_KEEPALIVE_TIME;
    }

    return value;
}

int set_dpm_keepalive_to_nvram(int time)
{
    int status = 0;

    status = dpm_write_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, time, DFLT_DPM_KEEPALIVE_TIME);
    if (status == -1) {
        return 1;
    }

    return 0;
}

/* User Wakeup Time */
int get_dpm_user_wu_time_from_nvram(void)
{
    int value;

    dpm_read_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, &value);
    if (value == -1) {
        return DFLT_DPM_USER_WAKEUP_TIME;
    }

    return value;
}

int set_dpm_user_wu_time_to_nvram(int msec)
{
    int status = 0;

    status = dpm_write_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, msec, DFLT_DPM_USER_WAKEUP_TIME);
    if (status == -1) {
        return 1;
    }
    return 0;
}

/* Wakeup Type */
int get_dpm_wakeup_condition_from_nvram(void)
{
    int value;

    dpm_read_nvram_int(NVR_KEY_DPM_WAKEUP_CONDITION, &value);
    if (value == -1) {
        return DFLT_DPM_WAKEUP_CONDITION;
    }

    return value;
}

/* Wakeup Time */
int get_dpm_TIM_wakeup_time_from_nvram(void)
{
    int value;

    dpm_read_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, &value);
    if (value == -1) {
        if (dpm_wu_tm_msec_flag == pdTRUE) {
            return (DFLT_DPM_TIM_WAKEUP_COUNT * 102.4);
        } else {
            return DFLT_DPM_TIM_WAKEUP_COUNT;
        }
    }

    return value;
}

int set_dpm_TIM_wakeup_time_to_nvram(int sec)
{
    int status = 0;

    if (dpm_wu_tm_msec_flag == pdTRUE) {
        status = dpm_write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, sec, (int)coilToInt(DFLT_DPM_TIM_WAKEUP_COUNT / 102.4));
    } else {
        status = dpm_write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, sec, DFLT_DPM_TIM_WAKEUP_COUNT);
    }

    if (status == -1) {
        return 1;
    }

    /* let's do save tim wakeup time to retention memory */
    set_dpm_tim_wakeup_time_rtm(sec);

    return 0;
}


/// DPM TIMER ////////////////////////////////////////////////////
static void dpm_trigger_timer_fn(unsigned long arg)
{
	DA16X_UNUSED_ARG(arg);

    EventBits_t    dpm_ev_bits;

    // Trigger when DPM daemon is running ...
    if (dpm_daemon_start_flag == pdTRUE) {
        /* Just return if abnormal MAC case occurs... */
        if (dpm_mac_err_flag == pdTRUE) {
            dpm_pdown_state = WAIT_DPM_SLEEP;
            dpm_mac_err_flag = pdFALSE;
            return;
        }

        /** If DPM Sleep Command is running, do not trigger **/
        if (chk_dpm_pdown_start() != WAIT_DPM_SLEEP) {
            return;
        }

        if (da16x_dpm_ev_group != NULL) {
            dpm_ev_bits = xEventGroupSetBits(da16x_dpm_ev_group, DPM_DAEMON_TRIGGER);
            if ((dpm_ev_bits & DPM_DAEMON_TRIGGER) != DPM_DAEMON_TRIGGER) {
                PRINTF("[%s] event flag set error !!!\n", __func__);
            }
        }
    }
}

static void rtc_timeout_cb(unsigned int param)
{
    RTM_FLAG_PTR->dpm_rtc_timeout_tid |= (1 << param);

    Printf("\nrtc_timeout (tid:%d)\n", param);
}

void *get_rtc_timer_ops_ptr(void)
{
    if (chk_retention_mem_exist() == pdFAIL) {
        /* Unsupport RTM */
        return NULL;
    }

    return (void *)RTM_RTC_TIMER_PTR;
}

static unsigned int get_rtc_timeout_tid(void)
{
    return RTM_FLAG_PTR->dpm_rtc_timeout_tid;
}

static unsigned int clr_rtc_timeout_tid(int tid)
{
    RTM_FLAG_PTR->dpm_rtc_timeout_tid &= ~(1 << tid);

    return pdTRUE;
}

int rtc_cancel_timer(int timer_id)
{
    int ret;

    if (chk_dpm_mode() != pdTRUE || timer_id == 2) {
        return -1;
    }

    /* Get mutex */
    if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
        PRINTF("[CANCEL] mutex_get error\n");
    }

    ret = fc80211_set_app_keepalivetime(timer_id, 0, NULL);

    if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
        PRINTF("[CANCEL] mutex_put error\n");
    }

#ifdef DPM_TIMER_DEBUG
    PRINTF("[%s] tid=%d ret=%d\n", __func__, timer_id, ret);
#endif /* DPM_TIMER_DEBUG */

    return ret;
}

int rtc_register_timer(unsigned int msec, char *name, int timer_id, int peri, void (* callback_func)(char *timer_name))
{
    dpm_timer_info_t *dpm_timer = NULL;
    int available_tid = TIMER_ERR;
    int registered_tid = TIMER_ERR;
    int task_name_len = 0;

    if (chk_dpm_mode() != pdTRUE || timer_id == 1) {
        return -1;
    }

#ifdef DPM_TIMER_DEBUG
    PRINTF("[%s] try tid=%d msec=%d\n\n", __func__, timer_id, msec);
#endif // DPM_TIMER_DEBUG

    task_name_len = strlen(name);
    //if ((REG_NAME_DPM_MAX_LEN < task_name_len) || (task_name_len <= 0)) {
    if ((REG_NAME_DPM_MAX_LEN < task_name_len) || (task_name_len < 0)) {
        PRINTF("[REG] Check the timer name_len(%d) (max len=%d)\n", task_name_len, REG_NAME_DPM_MAX_LEN);
        return -1;
    }

    if (msec > TIMER_MAX_MSEC) {
        /* size of intiger */
        PRINTF("[REG] Time value overflow. (Max:%ld msec)\n", TIMER_MAX_MSEC);
        return -1;
    }

    /* Get mutex */
    if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
        PRINTF("[REG] mutex_get error\n");
    }

    if (timer_id >= 2) {
        available_tid = timer_id;
    } else {
        available_tid = fc80211_get_empty_rid();
    }

    dpm_timer = (dpm_timer_info_t *)get_rtc_timer_ops_ptr();

    /* Initialize */
    dpm_timer[available_tid - 2].timeout_callback = NULL;
    dpm_timer[available_tid - 2].msec = 0;
    memset((void *)dpm_timer[available_tid - 2].task_name, 0x00, REG_NAME_DPM_MAX_LEN);
    memset((void *)dpm_timer[available_tid - 2].timer_name, 0x00, DPM_TIMER_NAME_MAX_LEN);

    /* Register task name */
    strncpy(dpm_timer[available_tid - 2].task_name, name, task_name_len);

    /* Register callback function */
    dpm_timer[available_tid - 2].timeout_callback = callback_func;

    /* Register reschedule time */
    if (peri > 0) {
        dpm_timer[available_tid - 2].msec = msec;
    }

    /* Register to RTC */
    registered_tid = fc80211_set_app_keepalivetime(available_tid, msec, rtc_timeout_cb);

    if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
        PRINTF("[REG] mutex_put error !!!\n");
    }

    if ((registered_tid <= TIMER_0) || (registered_tid >= TIMER_ERR)) {
        PRINTF("[REG] Registration failed (name=<%s>, id=%d).\n", name, registered_tid);
        return -1;
    }

#ifdef DPM_TIMER_DEBUG
    PRINTF("[%s] Register tid=%d, name=%s(len=%d), sec=%d, fuc=0x%x\n\n",__func__,
            registered_tid,
            dpm_timer[available_tid - 2].task_name,
            strlen(dpm_timer[available_tid - 2].task_name),
            dpm_timer[available_tid - 2].msec,
            dpm_timer[available_tid - 2].timeout_callback);
#endif /* DPM_TIMER_DEBUG */

    return registered_tid;
}

static int dpm_timer_find_timer_id(char *task_name, char *timer_name)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int task_name_len = 0;
    int timer_name_len = 0;
    int timer_idx = TIMER_0;

    if (chk_retention_mem_exist() == pdFAIL) {
        return -1;
    }

    if (   (task_name == NULL)
        || (timer_name == NULL)
        || (strlen(task_name) <= 0)
        || (strlen(timer_name) <= 0)) {
        PRINTF("[FIND] Check timer_name !!!\n");
        return -1;
    }

    task_name_len = strlen(task_name);
    timer_name_len = strlen(timer_name);
    if (   (REG_NAME_DPM_MAX_LEN < task_name_len)
        || (task_name_len <= 0)
        || (DPM_TIMER_NAME_MAX_LEN < timer_name_len)
        || (timer_name_len <= 0)) {
        PRINTF("[FIND] Check timer_name (max len=%d)\n", DPM_TIMER_NAME_MAX_LEN);
        return -1;
    }

    dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();

    while (timer_idx < TIMER_ERR) {
        if (   (strncmp(dpm_timer[timer_idx - 2].task_name, task_name, sizeof(dpm_timer[timer_idx - 2].task_name)) == 0)
            && (strncmp(dpm_timer[timer_idx - 2].timer_name, timer_name, sizeof(dpm_timer[timer_idx - 2].timer_name)) == 0)) {

            return timer_idx;
        }
        timer_idx++;
    }

    // Not found ... It's okay to create RTC timer with this name.
    return -99;
}

void dpm_user_timer_list_print(void)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int timer_id = TIMER_5;
    dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();

    PRINTF("DPM Timer\n");
    PRINTF("---------------------------------------------------------\n");

    while (timer_id < TIMER_ERR) {
        if ((dpm_timer[timer_id - 2].task_name != NULL) && (strlen(dpm_timer[timer_id - 2].task_name) > 0)) {

            PRINTF("TID : %d  \tName : %s / %s  \tRemain : %d\n",
                    timer_id,
                    dpm_timer[timer_id - 2].task_name,
                    dpm_timer[timer_id - 2].timer_name,
                    rtc_timer_info(timer_id));
        }

        timer_id++;
    }
    PRINTF("---------------------------------------------------------\n");

}

int dpm_user_timer_get_remaining_msec(char *task_name, char *timer_name)
{
    int timer_id = TIMER_ERR;

    timer_id = dpm_timer_find_timer_id(task_name, timer_name);
    if ((timer_id >= TIMER_0) && (timer_id < TIMER_ERR)) {
        return rtc_timer_info(timer_id);
    }

    PRINTF("[DPM_TM] Failed to read remaining time. (%s/%s)\n", task_name, timer_name);

    return -1;
}

#define    TIMER_DELETE_ERROR    0x11
int dpm_user_timer_delete_all(unsigned int level)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int tid = TIMER_5;
    int delete_tid = TIMER_ERR;
    int    ret = pdTRUE;

    if (chk_dpm_mode() != pdTRUE) {
        return pdFAIL;
    }

    if (dpm_timer_mutex == NULL) {
        dpm_timer_mutex = xSemaphoreCreateMutex();

        if (dpm_timer_mutex == NULL) {
                PRINTF("\n[DPM_TM] dpm_timer_mutex create error !!!\n");
                return pdFAIL;
        }
    }

    if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
        PRINTF("[DPM_TM] mutex_get error !!!\n");
    }

    dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();
    if (dpm_timer == NULL) {
        return pdFAIL;
    }

    PRINTF("[DPM_TM] Delete tid : %d ~ %d (lvl.%d). \r\n", TIMER_5, TIMER_ERR-1 , level);

    for (tid = TIMER_5; tid < TIMER_ERR; tid ++) {

        /* Timer ID 9/10 Use limitation */
        if (tid == TIMER_9 || tid == TIMER_10) {
            continue;
        }

        /* level == 1 : Delete only RTM data */
        /* level == 2 : Delete RTC and RTM */
        if (level > 1) {
            /* Delete RTC */
            delete_tid = fc80211_set_app_keepalivetime(tid, 0, NULL);
            if (delete_tid != tid) {
                PRINTF("[DPM_TM] Timer deletion failed. (%d/%d)\n", delete_tid, tid);
                ret = TIMER_DELETE_ERROR;
                continue;
            }
        }

        /* Delete RTM saved data */
        dpm_timer[tid - 2].timeout_callback = NULL;
        dpm_timer[tid - 2].msec = 0;
        memset((void *)dpm_timer[tid - 2].task_name, 0x00, REG_NAME_DPM_MAX_LEN);
        memset((void *)dpm_timer[tid - 2].timer_name, 0x00, DPM_TIMER_NAME_MAX_LEN);
    }

    if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
        PRINTF("[DPM_TM] mutex_put error !!!\n");
    }

    return ret;
}

int dpm_user_timer_delete(char *task_name, char *timer_name)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int exist_tid = TIMER_ERR;
    int delete_tid = TIMER_ERR;

    if (chk_dpm_mode() != pdTRUE) {
        return -1;
    }

    exist_tid = dpm_timer_find_timer_id(task_name, timer_name);
    if ((exist_tid >= TIMER_0) && (exist_tid < TIMER_ERR)) {
        dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();
        if (dpm_timer == NULL) {
            goto fail;
        }

        /* Delete RTM saved data */
        dpm_timer[exist_tid - 2].timeout_callback = NULL;
        dpm_timer[exist_tid - 2].msec = 0;
        memset((void *)dpm_timer[exist_tid - 2].task_name, 0x00, REG_NAME_DPM_MAX_LEN);
        memset((void *)dpm_timer[exist_tid - 2].timer_name, 0x00, DPM_TIMER_NAME_MAX_LEN);

        if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_get error\n");
            goto fail;
        }

        /* Delete RTC */
        delete_tid = fc80211_set_app_keepalivetime(exist_tid, 0, NULL);

        if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_put error\n");
        }

        if (delete_tid != exist_tid) {
            PRINTF("[DPM_TM] Timer deletion failed. (%d/%d)\n", delete_tid, exist_tid);
            return -1;
        }

        return delete_tid; /* Success */

    } else if (exist_tid == -2) {
        PRINTF("[DPM_TM] Timer not found. (%s/%s)\n", task_name, timer_name);

        return -1;
    }
fail:
    PRINTF("[DPM_TM] Timer deletion failed. (%s/%s)\n", task_name, timer_name);
    return -1;
}

int dpm_user_timer_change(char *task_name, char *timer_name, unsigned int msec)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int exist_tid = TIMER_ERR;
    int regi_tid = TIMER_ERR;

    if (chk_dpm_mode() != pdTRUE) {
        return -1;
    }

    if (msec > TIMER_MAX_MSEC) {
        /* size of intiger */
        PRINTF("[RTC] Timer is out of range. (max: %ld msec)\n", TIMER_MAX_MSEC);
        return -1;
    }

    exist_tid = dpm_timer_find_timer_id(task_name, timer_name);
    if ((exist_tid >= TIMER_0) && (exist_tid < TIMER_ERR)) {
        dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();
        dpm_timer[exist_tid - 2].msec = msec;

        if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_get error !!!\n");
        }

        /* Re-register to RTC */
        regi_tid = fc80211_set_app_keepalivetime(exist_tid, msec, rtc_timeout_cb);

        if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_put error !!!\n");
        }

        return regi_tid;
    }

    PRINTF("[DPM_TM] Failed to change Timer.(%s/%s)\n", task_name, timer_name);

    return -1;
}

int dpm_user_timer_create(char *task_name,
                char *timer_name,
                void (* callback_func)(char *timer_name),
                unsigned int msec,
                unsigned int reschedule_msec)
{
    dpm_timer_info_t *dpm_timer = NULL;
    int regi_tid = TIMER_ERR;
    int exist_tid = TIMER_ERR;

    if (chk_dpm_mode() != pdTRUE) {
        return -1;        // DPM_MODE_NOT_ENABLED
    }

    if (msec > TIMER_MAX_MSEC) {
        /* size of intiger */
        PRINTF("[RTC] Timer is out of range. (max: 2147438647 msec)\n");
        return -2;        // DPM_TIMER_SEC_OVERFLOW
    }

    exist_tid = dpm_timer_find_timer_id(task_name, timer_name);
    if ((exist_tid >= TIMER_0) && (exist_tid < TIMER_ERR)) {
        PRINTF("[DPM_TM] Exists: task_name=%s, timer_name=%s, exist_tid=%d\n",
                        task_name,
                        timer_name,
                        exist_tid);
        return -3;        // DPM_TIMER_ALREADY_EXIST
    } else if (exist_tid == -1) {
        PRINTF("[DPM_TM] Timer name error. (%s/%s)\n", task_name, timer_name);
        return -4;        // DPM_TIMER_NAME_ERROR
    }

    if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
        PRINTF("[DPM_TM] mutex_get error !!!\n");
    }

    dpm_timer = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();
    if (dpm_timer == NULL) {
        PRINTF("[DPM_TM] Unsupport RTM.\n");
        regi_tid = -6;        // DPM_UNSUPPORTED_RTM
        goto finish;
    }

    /* Register to RTC */
    regi_tid = fc80211_set_app_keepalivetime(0, msec, rtc_timeout_cb);
    if ((regi_tid == 0) || (regi_tid >= TIMER_ERR)) {
        PRINTF("[DPM_TM] Timer registration failed. (err_tid=%d)\n", regi_tid);
        regi_tid = -7;        // DPM_TIMER_REGISTER_FAIL
        goto finish;
    }

    /* Initialize */
    dpm_timer[regi_tid - 2].timeout_callback = NULL;
    dpm_timer[regi_tid - 2].msec = 0;
    memset((void *)dpm_timer[regi_tid - 2].task_name, 0x00, REG_NAME_DPM_MAX_LEN);
    memset((void *)dpm_timer[regi_tid - 2].timer_name, 0x00, DPM_TIMER_NAME_MAX_LEN);

    /* Register thread name */
    strncpy(dpm_timer[regi_tid - 2].task_name, task_name, sizeof(dpm_timer[regi_tid - 2].task_name));

    /* Register timer name */
    strncpy(dpm_timer[regi_tid - 2].timer_name, timer_name, sizeof(dpm_timer[regi_tid - 2].timer_name));

    /* Register callback function */
    if (callback_func != NULL) {
        dpm_timer[regi_tid - 2].timeout_callback = callback_func;
    }

    /* Register reschedule time */
    if (reschedule_msec > 0) {
        dpm_timer[regi_tid - 2].msec = reschedule_msec;
    }

finish:
    if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
        PRINTF("[DPM_TM] mutex_put error !!!\n");
    }

    return regi_tid;
}

int dpm_timer_process(dpm_timer_info_t *dpm_timer, unsigned int timeout_id)
{
    int registered_tm_id = TIMER_ERR;

    if (dpm_timer == NULL) {
        PRINTF("[%s] Null pointer Error!!\r\n", __func__);
        return pdFAIL;
    }

    /* Do not sleep while this function working */
    dpm_rtc_to_chk_flag = pdTRUE;

    if (((int)timeout_id < TIMER_0) || ((int)timeout_id >= TIMER_ERR)) {
        /* No RTC timeout event. */
        dpm_rtc_to_chk_flag = pdFALSE;

        return pdPASS;
    }

#ifdef DPM_TIMER_DEBUG
    PRINTF("\n[%s] Timeout (id=%d, name=<%s>, remain_time_sec=%d cb:%p) \r\n\n",
              __func__,
              timeout_id,
              dpm_timer[timeout_id - 2].task_name,
              dpm_timer[timeout_id - 2].msec,
              dpm_timer[timeout_id - 2].timeout_callback);
#endif /* DPM_TIMER_DEBUG */

    if (dpm_timer[timeout_id - 2].msec > 0) {
        if (xSemaphoreTake(dpm_timer_mutex, (TickType_t)portMAX_DELAY) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_get error !!!\n");
        }

        /* Re-register to RTC */
        registered_tm_id = fc80211_set_app_keepalivetime(timeout_id, dpm_timer[timeout_id - 2].msec, rtc_timeout_cb);

        if (xSemaphoreGive(dpm_timer_mutex) != pdTRUE) {
            PRINTF("[DPM_TM] mutex_put error !!!\n");
        }

        if (registered_tm_id == TIMER_ERR) {
            PRINTF("[%s] TID=%d, Re-register failed\n", __func__, timeout_id);
        } else if (registered_tm_id != (int)timeout_id) {
            PRINTF("[%s] Re-register TID has been changed (%d->%d)\n", __func__, timeout_id, registered_tm_id);
        } else {
#ifdef DPM_TIMER_DEBUG
            PRINTF("[%s] Re-regi success (%d, %s, sec=%d)\r\n",
                    __func__,
                    registered_tm_id,
                    dpm_timer[timeout_id - 2].task_name,
                    dpm_timer[timeout_id - 2].msec);
#endif /* DPM_TIMER_DEBUG */
        }
    }

    if (dpm_timer[timeout_id - 2].timeout_callback != NULL) {
        if ((dpm_timer[timeout_id - 2].task_name == NULL) || (strlen(dpm_timer[timeout_id - 2].task_name) <= 0)) {

            /* Call user callback */
            dpm_timer[timeout_id - 2].timeout_callback(dpm_timer[timeout_id - 2].timer_name);

#ifndef USE_SET_DPM_INIT_DONE
        } else if (dpm_timer_cb_ready(dpm_timer[timeout_id - 2].task_name) == 0) {
            dpm_timer[timeout_id - 2].timeout_callback(dpm_timer[timeout_id - 2].timer_name);
#endif /* USE_SET_DPM_INIT_DONE */
        } else if (confirm_dpm_init_done(dpm_timer[timeout_id - 2].task_name) != 0) {
            /* Call user callback */
            dpm_timer[timeout_id - 2].timeout_callback(dpm_timer[timeout_id - 2].timer_name);
        } else {
            /* Wait "rtc_abnormal_cnt" times until user app is created. */
            dpm_timer_abnormal_flag[timeout_id] = dpm_timer_abnormal_flag[timeout_id] + 1;

            if (dpm_timer_abnormal_flag[timeout_id] > RTC_ABNORMAL_MAX_CNT) {
                PRINTF("[%s] '%s' is not ready. Callback can't be called. (%s/%d) \r\n",
                        __func__,
                        dpm_timer[timeout_id - 2].task_name,
                        dpm_timer[timeout_id - 2].timer_name,
                        timeout_id);

                dpm_timer_abnormal_flag[timeout_id] = 0;
            } else {
                /* Restore a TID that has not yet been called set_dpm_init_done function. */
                RTM_FLAG_PTR->dpm_rtc_timeout_tid |= (1 << timeout_id);
#ifdef DPM_TIMER_DEBUG
                PRINTF("[%s] Callback abnormal TID=%d, Cnt=%d \r\n",
                        __func__,
                        timeout_id,
                        dpm_timer_abnormal_flag[timeout_id]);
#endif /* DPM_TIMER_DEBUG */
            }
        }
    }

    if (dpm_timer[timeout_id - 2].msec <= 0) {
#ifdef DPM_TIMER_DEBUG
        PRINTF("[%s] Clear RTM (name=%s, tid=%d)\n",
                __func__, dpm_timer[timeout_id - 2].task_name, timeout_id);
#endif /* DPM_TIMER_DEBUG */

        /* Delete RTM saved data */
        dpm_timer[timeout_id - 2].timeout_callback = NULL;
        dpm_timer[timeout_id - 2].msec = 0;
        memset((void *)dpm_timer[timeout_id - 2].task_name, 0x00, REG_NAME_DPM_MAX_LEN);
        memset((void *)dpm_timer[timeout_id - 2].timer_name, 0x00, DPM_TIMER_NAME_MAX_LEN);
    }

    /* function working done */
    dpm_rtc_to_chk_flag = pdFALSE;

    return pdPASS;
}

void dpm_timer_task(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
    dpm_timer_info_t *dpm_timer_ptr = NULL;
    int    retry_reg_cnt = 5;
    unsigned int     timeout_id, tid;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

retry_dpm_reg:

    da16x_sys_watchdog_notify(sys_wdog_id);

    if (reg_dpm_sleep(REG_NAME_DPM_TIMER_PROC, 0) == DPM_REG_ERR) {
        if (retry_reg_cnt >= 5){
            PRINTF("[%s] Failed to register for %s\n", __func__, REG_NAME_DPM_TIMER_PROC);
            da16x_sys_watchdog_unregister(sys_wdog_id);
            return;
        }

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        retry_reg_cnt++;
        vTaskDelay(10);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        goto retry_dpm_reg;
    }

    clr_dpm_sleep(REG_NAME_DPM_TIMER_PROC);

    dpm_timer_ptr = (dpm_timer_info_t*)get_rtc_timer_ops_ptr();
    if (dpm_timer_ptr == NULL) {
        Printf("[DPM_TM] Null pointer.\r\n");
        goto finish;
    }

    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        /* Do nothing anymore duing DPM sleep operation ... */
        if (chk_dpm_pdown_start() == WAIT_DPM_SLEEP) {

            timeout_id = get_rtc_timeout_tid();
            if (timeout_id != 0) {
                clr_dpm_sleep(REG_NAME_DPM_TIMER_PROC);
            } else {
                set_dpm_sleep(REG_NAME_DPM_TIMER_PROC);

                da16x_sys_watchdog_notify(sys_wdog_id);

                da16x_sys_watchdog_suspend(sys_wdog_id);

                vTaskDelay(1);

                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

                continue;
            }

            /* SEND */
            for (tid = 0; tid <= 15; tid++) {
                if (timeout_id & (1 << tid)) {
                    if (xQueueSend(dpm_timer_queue, (void *)&tid, 0) != pdPASS) {
                        Printf("[DPM_TM] Queue send Error !!!\r\n");
                    } else {
                        clr_rtc_timeout_tid(tid);
                    }
                }
            }

rcv_queue:
            da16x_sys_watchdog_notify(sys_wdog_id);

            /* RECEIVE */
            if (xQueueReceive(dpm_timer_queue, &(tid), 0) == pdPASS) {
                if (dpm_timer_process(dpm_timer_ptr, tid) == pdFAIL) {
                    goto finish;
                }

                if (dpm_timer_abnormal_flag[tid] != 0) {
#ifdef DPM_TIMER_DEBUG
                    PRINTF("[%s] Recall abnormal tid = %d(cnt=%d)\r\n",
                            __func__, tid, dpm_timer_abnormal_flag[tid]);
#endif /* DPM_TIMER_DEBUG */
                    xQueueReset(dpm_timer_queue);
                    continue;
                }
                goto rcv_queue;
            }
        } else {
            set_dpm_sleep(REG_NAME_DPM_TIMER_PROC);

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            vTaskDelay(10);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
    }

finish:
    Printf("[%s] Terminated!!\r\n", __func__);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    set_dpm_sleep(REG_NAME_DPM_TIMER_PROC);
    unreg_dpm_sleep(REG_NAME_DPM_TIMER_PROC);

    vQueueDelete(dpm_timer_queue);        // Delete Queue
    vSemaphoreDelete(dpm_timer_mutex);    // Delete SemaMutex

    return;
}

unsigned int dpm_timer_task_create(void)
{
    BaseType_t    xReturned;

    if (chk_dpm_mode() != pdTRUE) {
        return pdFAIL;
    }

    /* Queue*/
    dpm_timer_queue = xQueueCreate(32, sizeof(unsigned long));
    if (dpm_timer_queue == NULL) {
        Printf("\n[DPM_TM] queue create error(0x%02x)\r\n");
        return pdFAIL;
    }

    /* Task */
    xReturned = xTaskCreate(dpm_timer_task,
                            DPM_TIMER_TASK_NAME,
                            DPM_TIMER_STK_SZ,
                            (void *)NULL,
                            DPM_TIMER_TASK_PRIORITY,        // Fixed the loss of non-periodic timer
                            &dpm_timer_proc_handler);
    if (xReturned != pdPASS) {
        PRINTF("\n[DPM_TM] Failed to create dpm_timer_proc task (0x%x)!!!\n", xReturned);
        return pdFAIL;
    }

    return pdTRUE;
}

int rtc_timer_info(int tid)
{
    if (chk_dpm_mode() != pdTRUE) {
        return -1;
    }

    DA16X_TIMER_ID first_tid = TIMER_ERR;
    DA16X_TIMER_ID next_tid = TIMER_ERR;
    unsigned long first_msec = 0;
    unsigned long before_msec = 0;
    unsigned long next_msec = 0;
    int i = 0;

    DA16X_TIMER *gtimer = (DA16X_TIMER *)(DA16X_RETMEM_BASE + 0x100);
    unsigned int *CURIDX = (unsigned int *)((DA16X_RETMEM_BASE + 0x100)
                        + (sizeof(DA16X_TIMER) * TIMER_ERR)
                        + sizeof(unsigned int *)
                        + sizeof(unsigned int *));

    #define curidx        (*CURIDX)

    if (curidx != TIMER_ERR) {
        first_tid = (DA16X_TIMER_ID)curidx;
        first_msec = (unsigned int)(gtimer[first_tid].time / 1000000);


        if (first_tid == tid) {
            return first_msec;
        }

        i = 0;
        next_tid = first_tid;

        before_msec = first_msec;
        while (i < TIMER_ERR) {
            next_tid = (DA16X_TIMER_ID)gtimer[next_tid].type.content[DA16X_IDX_NEXT];
            next_msec = (unsigned int)(gtimer[next_tid].time / 1000000) + before_msec;
            if (next_tid == tid) {
                return next_msec;
            }

            if (next_tid >= TIMER_ERR) {
                break;
            }

            i++;
            before_msec = next_msec;
        }
    }

    return 0;
}

void rtc_timer_list_info(void)
{
    DA16X_TIMER_ID tid = TIMER_0;
    unsigned long msec = 0;

    if (chk_dpm_mode() != pdTRUE) {
        PRINTF("Timer does not exist.\n");
        return;
    }

    /* Print timeout list */
    PRINTF("\nDPM Timer List (Non-real-time information)\n");
    print_separate_bar('-', 45, 1);
    PRINTF("ID       Expire Time  Descrition\n");
    print_separate_bar('=', 45, 1);

    while ((int)tid < TIMER_ERR) {
        msec = rtc_timer_info((int)tid);

        PRINTF("%02d       %11d  ", (int)tid, msec);

        switch ((int)tid) {
        case 2:
            PRINTF("<User Wake Up>");
            break;

        case 3:
            PRINTF("<DHCP Client>");
            break;

        case 4:
            if (dpm_abnormal_disable_flag == pdFALSE) {
                PRINTF("<Abnormal>");
            }
            break;

        default:
#ifdef __DPM_FINAL_ABNORMAL__
            if (next_tid[i] == get_final_abnormal_rtc_tid() && get_final_abnormal_rtc_tid() > 0) {
                PRINTF("<Final Abnormal>");
            }
#endif /* __DPM_FINAL_ABNORMAL__ */
            break;
        }

        PRINTF("\n");
        tid ++;
    }

    print_separate_bar('-', 45, 2);
}

void disable_all_dpm_timer(void)
{
    if (dpm_trigger_timer != NULL) {
        xTimerStop(dpm_trigger_timer, 0);
    }

    if (dpm_sts_chk_tm != NULL) {
        xTimerStop(dpm_sts_chk_tm, 0);
    }
}

static char *str_dpm_wakeup_condition(void)
{
    return (char *)NULL;
}


/// TIME ////////////////////////////////////////////////////
//
void set_systimeoffset_to_rtm(unsigned long long offset)
{
    RTM_FLAG_PTR->systime_offset = offset;

    return;
}

unsigned long long get_systimeoffset_from_rtm(void)
{
    return (unsigned long long)RTM_FLAG_PTR->systime_offset;
}

void set_rtc_oldtime_to_rtm(unsigned long long time)
{
    RTM_FLAG_PTR->rtc_oldtime = time;

    return;
}

unsigned long long get_rtc_oldtime_from_rtm(void)
{
    return (unsigned long long)(RTM_FLAG_PTR->rtc_oldtime);
}

void set_timezone_to_rtm(long timezone)
{
    RTM_FLAG_PTR->dpm_timezone = timezone;

    return;
}

long get_timezone_from_rtm(void)
{
    return RTM_FLAG_PTR->dpm_timezone;
}

void set_sntp_use_to_rtm(UINT use)
{
    RTM_FLAG_PTR->dpm_sntp_use = (UCHAR)use;

    return;
}

UINT get_sntp_use_from_rtm(void)
{
    return (UINT)RTM_FLAG_PTR->dpm_sntp_use;
}


void set_sntp_period_to_rtm(long period)
{
    RTM_FLAG_PTR->dpm_sntp_period = period;

    return;
}

ULONG get_sntp_period_from_rtm(void)
{
    return RTM_FLAG_PTR->dpm_sntp_period;
}

void set_sntp_timeout_to_rtm(long timeout)
{
    RTM_FLAG_PTR->dpm_sntp_timeout = timeout;

    return;
}

ULONG get_sntp_timeout_from_rtm(void)
{
    return RTM_FLAG_PTR->dpm_sntp_timeout;
}
///////////////////////////////////////////////////////////////////////////////
/////  For USER Retention Memory API  /////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

dpm_user_rtm_pool *user_rtm_pool = (dpm_user_rtm_pool *)RTM_USER_POOL_PTR;

int dpm_user_rtm_pool_init_chk(void)
{
    return user_rtm_pool_init_done;
}

void dpm_user_rtm_pool_clear(void)
{
    memset((void *)user_rtm_pool, 0x00, sizeof(dpm_user_rtm_pool));

    return;
}

void dpm_user_rtm_clear(void)
{
    memset((void *)RTM_USER_DATA_PTR, 0x00, USER_DATA_ALLOC_SZ);

    return;
}

unsigned int dpm_user_rtm_pool_create(void)
{
    if (!user_rtm_pool_init_done) {
        if (chk_dpm_wakeup() == pdFALSE && !get_boot_mode()) {
            //to clear user rtm pool
            dpm_user_rtm_pool_clear();

            //to clear user rtm data
            dpm_user_rtm_clear();

            da16x_dpm_rtm_mem_init(RTM_USER_DATA_PTR, USER_DATA_ALLOC_SZ);

            user_rtm_pool->free_ptr = da16x_dpm_rtm_mem_get_free_ptr();
            user_rtm_pool->first_user_addr = NULL;
        } else {
            da16x_dpm_rtm_mem_recovery(RTM_USER_DATA_PTR, USER_DATA_ALLOC_SZ, user_rtm_pool->free_ptr);
        }

        // Set flag to mark User RTM Initialize done ...
        user_rtm_pool_init_done = pdTRUE;
    }

    return pdTRUE;
}

unsigned int dpm_user_rtm_pool_delete(void)
{
    // to clear user rtm pool
    dpm_user_rtm_pool_clear();

    // to clear user rtm data
    dpm_user_rtm_clear();

    return pdTRUE;
}

void dpm_user_rtm_add(dpm_user_rtm *data)
{
    dpm_user_rtm *cur = NULL;

    if (user_rtm_pool->first_user_addr != NULL) {
        //goto end of linked-list
        for (cur = user_rtm_pool->first_user_addr; cur->next_user_addr != NULL ; cur = cur ->next_user_addr) {
        }

        cur->next_user_addr = data;
    } else {
        user_rtm_pool->first_user_addr = data;
    }

    return;
}

dpm_user_rtm *dpm_user_rtm_remove(char *name)
{
    dpm_user_rtm    *cur = NULL;
    dpm_user_rtm    *prev = NULL;

    if (user_rtm_pool->first_user_addr != NULL) {
        cur = user_rtm_pool->first_user_addr;

        if (dpm_strcmp(cur->name, name) == 0) {
            user_rtm_pool->first_user_addr = cur->next_user_addr;
            return cur;
        } else {
            do {
                prev= cur;
                cur= cur->next_user_addr;

                if (cur != NULL && dpm_strcmp(cur->name, name) == 0) {
                    prev->next_user_addr = cur->next_user_addr;
                    return cur;
                }
            } while (cur != NULL && cur->next_user_addr != NULL);
        }
    }

    return NULL;
}

dpm_user_rtm *dpm_user_rtm_search(char *name)
{
    dpm_user_rtm *cur = NULL;

    if (user_rtm_pool->first_user_addr != NULL) {
        for (cur = user_rtm_pool->first_user_addr; cur != NULL; cur = cur->next_user_addr) {
            if (dpm_strcmp(cur->name, name) == 0) {
                return cur;
            }
        }
    }

    return NULL;
}

unsigned int dpm_user_rtm_allocate(char *name,
                                   void **memory_ptr,
                                   unsigned long memory_size,
                                   unsigned long wait_option)
{
    if (chk_dpm_mode() == pdFALSE) {
        return ER_INVALID_PARAMETERS;
    }

    return user_rtm_pool_allocate(name, memory_ptr, memory_size, wait_option);
}

unsigned int dpm_user_rtm_release(char *name)
{
    if (chk_dpm_mode() == pdFALSE) {
        return ER_INVALID_PARAMETERS;
    }

    return user_rtm_release(name);
}

unsigned int dpm_user_rtm_get(char *name, unsigned char **data)
{
    if (chk_dpm_mode() == pdFALSE) {
        return 0;
    }

    return user_rtm_get(name, data);
}

unsigned int dpm_user_rtm_pool_info_get(void)
{
    da16x_dpm_rtm_mem_status();

    return 0;
}

void dpm_user_rtm_print(void)
{
    dpm_user_rtm *cur = NULL;

    /* Show total pool information */
    dpm_user_rtm_pool_info_get();

    /* show each allocated information */
    if (user_rtm_pool->first_user_addr != NULL) {
        PRINTF(" --- Detail information --------------------------------------\n");
        for (cur = user_rtm_pool->first_user_addr; cur != NULL; cur = cur->next_user_addr) {
            PRINTF("[%s]\taddr=0x%p, alloc_size=%ld, data_size:%ld\n",
                    cur->name,
                    cur,
                    cur->size + sizeof(dpm_user_rtm),
                    cur->size);
            PRINTF(" -------------------------------------------------------------\n");
        }
    } else {
        PRINTF("\n - Whole available to allocate ( No allocated ) ...\n");
    }

    PRINTF("\n");
}

///////////////////////////////////////////////////////////////////////////////
/////  For TCP Session on DPM  ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#if defined ( WORKING_FOR_DPM_TCP )
/*
 *****************************************************************************
 * Function : dpm_clr_tcp_ka_info
 *
 * - arguments
 *       sess_idx : Unique TCP session name for DPM
 *       rtc_timer_id : User RTC timer id for timeout on DPM
 *
 * - return : None
 *
 * - Discription :
 *       Clear TCP session information on RTM
 *
 *****************************************************************************
 */
void set_dpm_tcp_ka_info(char *tcp_sess_name, unsigned int rtc_timer_id)
{
}

int get_dpm_tcp_ka_info(char *tcp_sess_name)
{
    return 0;
}

void dpm_clr_tcp_ka_info(char *tcp_sess_name, unsigned int rtc_timer_id)
{
}

int dpm_chk_tcp_socket_transmit_state(void)
{
    return pdTRUE;
}

int dpm_chk_tcp_socket_receive_queue_state(void)
{
    return pdTRUE;
}
#endif    // WORKING_FOR_DPM_TCP

///////////////////////////////////////////////////////////////////////////////
///  End of APIs - DPM TCP Session handling  //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



/// For MDNS ///////////////////////////////////////////////

void dpm_set_mdns_info(const char *hostname, size_t len)
{
    dpm_mdns_info_t *dpm_info = (dpm_mdns_info_t *)RTM_MDNS_PTR;

    if (len > 0) {
        memset(dpm_info, 0x00, sizeof(dpm_mdns_info_t));
        memcpy(dpm_info->hostname, hostname, len);
        dpm_info->len = len;
    }

    return;
}

int dpm_get_mdns_info(char *hostname)
{
    dpm_mdns_info_t *dpm_info = (dpm_mdns_info_t *)RTM_MDNS_PTR;

    if (dpm_info == NULL || dpm_info->len == 0) {
        return -1;
    }

    memcpy(hostname, dpm_info->hostname, dpm_info->len);

    return dpm_info->len;
}

void dpm_clear_mdns_info(void)
{
    dpm_mdns_info_t *dpm_info = (dpm_mdns_info_t *)RTM_MDNS_PTR;

    memset(dpm_info, 0x00, sizeof(dpm_mdns_info_t));
}

void set_multicast_for_zeroconfig(void)
{
#if defined ( __SUPPORT_ZERO_CONFIG__ )
    unsigned int is_enabled = pdFALSE;
    unsigned long mdns_addr = iptolong("224.0.0.251");

    if (chk_dpm_wakeup() == pdFALSE) {
        extern int zeroconf_get_mdns_reg_from_nvram(unsigned int *tmpValue);

        zeroconf_get_mdns_reg_from_nvram(&is_enabled);

        if (is_enabled == pdTRUE) {
            set_dpm_mc_filter(mdns_addr);
        }
    }
#endif    // __SUPPORT_ZERO_CONFIG__
}

void dpm_set_otp_xtal40_offset(unsigned long otp_xtal40_offset)
{
    dpm_set_phy_otp_xtal40_offset(GET_DPMP(), otp_xtal40_offset);
}

/// For APIs ////////////////////////////////////////////////

#endif // XIP_CACHE_BOOT                // RTOS


/* EOF */
