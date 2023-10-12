/**
 ****************************************************************************************
 *
 * @file sleep2_monitor.c
 *
 * @brief util functions
 *
 * Copyright (c) 2012-2022 Renesas Electronics. All rights reserved.
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

#if defined (__BLE_COMBO_REF__)

#include "application.h"
#include "lwip/err.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"

#include "da16_gtl_features.h"
#include "sleep2_monitor.h"
#include "da16x_sys_watchdog.h"

#if defined (__APP_SLEEP2_MONITOR__)

/// variables
static uint8_t      sleep2_app_count = SLEEP2_APP_NUM_DEF;
static TaskHandle_t sleep2_mon_thd = NULL;
static sleep2_app_t sleep2_apps_list[SLEEP2_APP_NUM_DEF];
static SemaphoreHandle_t     sleep2_apps_list_lock;
static uint8_t      sleep2_mon_is_running = FALSE;
static void (*sleep2_app_rdy_fn) (void) = NULL;

static int8_t sleep2_monitor_can_be_run(char const *fn_name)
{
    DA16X_UNUSED_ARG(fn_name);

    if (dpm_mode_is_enabled() == FALSE) {
        DBG_TMP("[%s] Not run due to non-DPM mode \n", fn_name);
        return FALSE;
    }

    return TRUE;
}

int8_t sleep2_monitor_is_running(void)
{
    return sleep2_mon_is_running;
}

void sleep2_monitor_set_sleep2_rdy(void)
{
    if (sleep2_app_rdy_fn) {
        sleep2_app_rdy_fn();
    }
}

void sleep2_monitor_config(uint8_t sleep2_app_num, void (*app_todo_before_sleep2_fn)(void))
{
    if (sleep2_monitor_can_be_run(__func__) == FALSE) {
        return;
    }
    
    sleep2_app_count = sleep2_app_num;
    sleep2_app_rdy_fn = app_todo_before_sleep2_fn;
}

static void sleep2_monitor_main(void * arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
    uint8_t i;
    uint8_t num_app_sleep2_set;
    unsigned long long usec;
    unsigned int sec = SEC_TO_SLEEP;
    usec = sec * 1000000;

    sleep2_mon_is_running = TRUE;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        num_app_sleep2_set = 0;
        for (i = 0; i < sleep2_app_count; i++) {
            if (sleep2_apps_list[i].is_occupied == TRUE && sleep2_apps_list[i].state == SLEEP2_SET) {
                num_app_sleep2_set++;
            } else {
                break;
            }
        }

        if (num_app_sleep2_set == sleep2_app_count) {
            sleep2_monitor_set_sleep2_rdy();
            dpm_sleep_start_mode_2(usec, TRUE);
        }

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        vTaskDelay(10);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);
}

void sleep2_monitor_start(void)
{
    UINT    status = ERR_OK;
    
    if (sleep2_monitor_can_be_run(__func__) == FALSE) return;

    sleep2_apps_list_lock = xSemaphoreCreateMutex();
    if (sleep2_apps_list_lock == NULL) {
        DBG_ERR("\n\n[%s] Failed to create sleep2_apps_list_lock \n\n", __func__);
        return;
    }

    if (sleep2_mon_thd) {
        vTaskDelete(sleep2_mon_thd);
        sleep2_mon_thd = NULL;
    }
    status = xTaskCreate(sleep2_monitor_main,
                         SLEEP2_MON_NAME,
                         SLEEP2_MON_STK_SIZE,
                         (void *)NULL,
                         tskIDLE_PRIORITY + 1,
                         &sleep2_mon_thd);
    
    if (status != pdPASS) {
        DBG_ERR("[%s] ERROR(0x%0x)\n", __func__, status);

        if (sleep2_mon_thd) {
            MEMFREE(sleep2_mon_thd);
            sleep2_mon_thd = NULL;
        }
    }
    
}

static int8_t sleep2_monitor_get_empty_slot()
{
    int8_t i;

    for (i = 0; i < sleep2_app_count; i++) {
                    
        if (sleep2_apps_list[i].is_occupied == FALSE) {
            return i; // array index
        }
    }

    return -1; // meaning no empty slot    
}

static int8_t sleep2_monitor_find_with_name(char* app_name)
{
    int8_t i;

    for (i = 0; i < sleep2_app_count; i++) {
        if ((strcmp(sleep2_apps_list[i].app_name, app_name) == 0) && sleep2_apps_list[i].is_occupied == TRUE) {
            return i; // array index
        }
    }

    return -1; // meaning not found
}

int8_t sleep2_monitor_regi(char* app_name, char const *fn_name, int line)
{
    DA16X_UNUSED_ARG(fn_name);
    DA16X_UNUSED_ARG(line);

    int8_t empty_slot;
    if (sleep2_monitor_can_be_run(__func__) == FALSE) return -1;

    xSemaphoreTake(sleep2_apps_list_lock, portMAX_DELAY);

    empty_slot = sleep2_monitor_get_empty_slot();
    if (empty_slot == -1) {
        DBG_ERR(RED_COLOR"[%s] no empty slot for app_name=%s\n"CLEAR_COLOR, __func__, app_name);
        xSemaphoreGive(sleep2_apps_list_lock);
        return -1;
    }
    
    strcpy(sleep2_apps_list[empty_slot].app_name, app_name);
    sleep2_apps_list[empty_slot].is_occupied = TRUE;
    sleep2_apps_list[empty_slot].state = SLEEP2_CLR;

    xSemaphoreGive(sleep2_apps_list_lock);

    DBG_TMP(CYAN_COLOR"[sleep2_mng] app=%s : registered (%s, %d)\n"CLEAR_COLOR, 
            sleep2_apps_list[empty_slot].app_name, fn_name, line);
    
    return ERR_OK;
}


uint8_t sleep2_monitor_is_registered(char* app_name)
{
    if (sleep2_monitor_find_with_name(app_name) == -1) {
        return FALSE;
    } else {
        return TRUE;
    }
}

void sleep2_monitor_set_state(char* app_name, uint8_t state_to_set, char const *fn_name, int line)
{
    DA16X_UNUSED_ARG(fn_name);
    DA16X_UNUSED_ARG(line);

    int8_t idx;

    if (sleep2_monitor_can_be_run(__func__) == FALSE) {
        return;
    }

    xSemaphoreTake(sleep2_apps_list_lock, portMAX_DELAY);
    
    idx = sleep2_monitor_find_with_name(app_name);
    if (idx == -1) {
        DBG_ERR(RED_COLOR"[%s] app_name=%s not found \n"CLEAR_COLOR, __func__, app_name);
        xSemaphoreGive(sleep2_apps_list_lock);
        return;
    }

    // set state
    sleep2_apps_list[idx].state = state_to_set;

    xSemaphoreGive(sleep2_apps_list_lock);

    DBG_TMP(CYAN_COLOR"[sleep2_mng] app=%s : sleep2=%s (%s, %d)\n"CLEAR_COLOR, 
            sleep2_apps_list[idx].app_name, 
            (state_to_set ? "set" : "clr"), fn_name, line);
}

int8_t sleep2_monitor_get_state(char* app_name)
{
    
    int8_t idx = sleep2_monitor_find_with_name(app_name);

    if (idx == -1) {
        DBG_ERR(RED_COLOR"[%s] app_name=%s not found \n"CLEAR_COLOR, __func__, app_name);
        return -1;
    }

    return (sleep2_apps_list[idx].state);
}

void sleep2_monitor_get_status_all(void)
{
    int8_t i;

    if (sleep2_monitor_can_be_run(__func__) == FALSE) {
        return;
    }

    PRINTF("\n[idx] / name \n");
    for (i = 0; i < sleep2_app_count; i++) {
        PRINTF("[%d]   / %s\n", i, sleep2_apps_list[i].app_name);
    }
    PRINTF("\n\n");
    
    PRINTF("      index: ");
    for (i = 0; i < sleep2_app_count; i++) {
        PRINTF("%3d ", i);    
    }
    PRINTF("\n");
    
    PRINTF("      state: ");
    for (i = 0; i < sleep2_app_count; i++) {
        PRINTF("%3d ", sleep2_apps_list[i].state);    
    }
    PRINTF("\n");
    
    PRINTF("is_occupied: ");
    for (i = 0; i < sleep2_app_count; i++) {
        PRINTF("%3d ", sleep2_apps_list[i].is_occupied);    
    }
    PRINTF("\n\n");
}

int8_t is_wakeup_by_dpm_abnormal_chk(void)
{
    extern UCHAR get_last_abnormal_act(void);

    if (dpm_mode_is_wakeup() == FALSE && get_last_abnormal_act() > 0) {
        return TRUE;
    }

    return FALSE;
}

#endif // __APP_SLEEP2_MONITOR__

#endif /* __BLE_COMBO_REF__ */

/* EOF */
