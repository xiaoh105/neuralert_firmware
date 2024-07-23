/**
 ****************************************************************************************
 *
 * @file da16x_sys_watchdog.c
 *
 * @brief 
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

#include "da16x_sys_watchdog.h"
#include "wdog.h"
#include "util_api.h"
#include "task.h"
#include "tcb.h"

#undef  ENABLE_DA16X_SYS_WATCHDOG_DBG_DEBUG
#undef  ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO
#undef  ENABLE_DA16X_SYS_WATCHDOG_DBG_ERR

#define DA16X_SYS_WDOG_DBG  Printf

#if defined (ENABLE_DA16X_SYS_WATCHDOG_DBG_DEBUG)
#define DA16X_SYS_WDOG_DEBUG(fmt, ...)   \
    do { \
        DA16X_SYS_WDOG_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);   \
    } while (0)
#else
#define DA16X_SYS_WDOG_DEBUG(...)    do {} while (0)
#endif  // ENABLE_DA16X_SYS_WATCHDOG_DBG_DEBUG

#if defined (ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO)
#define DA16X_SYS_WDOG_INFO(fmt, ...)   \
    do { \
        DA16X_SYS_WDOG_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);   \
    } while (0)
#else
#define DA16X_SYS_WDOG_INFO(...)    do {} while (0)
#endif  // ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO

#if defined (ENABLE_DA16X_SYS_WATCHDOG_DBG_ERR)
#define DA16X_SYS_WDOG_ERR(fmt, ...)    \
    do { \
        DA16X_SYS_WDOG_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);   \
    } while (0)
#else
#define DA16X_SYS_WDOG_ERR(...) do {} while (0)
#endif // ENABLE_DA16X_SYS_WATCHDOG_DBG_ERR

#define DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT    (2)
#define DA16X_SYS_WDOG_MAX_TASKS_CNT            (64) // Must be the same with da16x_sys_wdog_max_tasks_cnt

typedef unsigned int DA16X_SYS_WDOG_TASK_BLK;

typedef	struct {
    QueueHandle_t   mutex;
    HANDLE          watchdog;
    TaskHandle_t    currtx;
} da16x_sys_wdog_fsm_type_t;

#if defined(__SUPPORT_SYS_WATCHDOG__)
static int da16x_sys_wdog_active = pdTRUE;
#else
static int da16x_sys_wdog_active = pdFALSE;
#endif // __SUPPORT_SYS_WATCHDOG__

const int da16x_sys_wdog_task_blk_size = sizeof(DA16X_SYS_WDOG_TASK_BLK);
const int da16x_sys_wdog_max_tasks_cnt = (DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT * sizeof(DA16X_SYS_WDOG_TASK_BLK) * 8);
DA16X_SYS_WDOG_TASK_BLK da16x_sys_wdog_tasks_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};
DA16X_SYS_WDOG_TASK_BLK da16x_sys_wdog_tasks_monitored_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};
DA16X_SYS_WDOG_TASK_BLK da16x_sys_wdog_tasks_notified_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};
int da16x_sys_wdog_idle_task_id = -1;
int da16x_sys_wdog_max_task_id = -1;

unsigned char da16x_sys_wdog_tasks_latency[DA16X_SYS_WDOG_MAX_TASKS_CNT] = {0x00,};
TaskHandle_t da16x_sys_wdog_task_handle[DA16X_SYS_WDOG_MAX_TASKS_CNT] = {0x00,};

static da16x_sys_wdog_fsm_type_t *da16x_sys_wdog_fsm = NULL;
static unsigned int da16x_sys_wdog_rescale_time = DA16X_SYS_WDOG_DEF_RESCALE_TIME;

const char *da16x_sys_watchdog_cmd_info = "info";
const char *da16x_sys_watchdog_cmd_rescale_time = "rescale_time";
const char *da16x_sys_watchdog_cmd_enable = "enable";
const char *da16x_sys_watchdog_cmd_disable = "disable";

static void da16x_sys_watchdog_set_active(int active)
{
#if defined(__SUPPORT_SYS_WATCHDOG__)
    if (active) {
        da16x_sys_wdog_active = pdTRUE;
    } else {
        da16x_sys_wdog_active = pdFALSE;
    }
#else
    DA16X_UNUSED_ARG(active);
#endif // __SUPPORT_SYS_WATCHDOG__
}

static int da16x_sys_watchdog_is_active()
{
    return da16x_sys_wdog_active;
}

static int da16x_sys_watchdog_is_debug_mode()
{
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        DA16X_SYS_WDOG_INFO("Debug mode\n");
        return pdTRUE;
    }

    return pdFALSE;
}

static void *da16x_sys_watchdog_internal_calloc(size_t n, size_t size)
{
    void *buf = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf) {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

static void da16x_sys_watchdog_internal_free(void *f)
{
    if (f == NULL) {
        return;
    }

    vPortFree(f);
}

void *(*da16x_sys_wdog_calloc)(size_t n, size_t size) = da16x_sys_watchdog_internal_calloc;
void (*da16x_sys_wdog_free)(void *ptr) = da16x_sys_watchdog_internal_free;

static void da16x_sys_watchdog_display_mask_task(DA16X_SYS_WDOG_TASK_BLK tasks[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT])
{
    int idx = 0;
    unsigned int bit_idx = 0;
    DA16X_SYS_WDOG_TASK_BLK task_mask = 0;

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        for (bit_idx = 0 ; bit_idx < (da16x_sys_wdog_task_blk_size * 8) ; bit_idx++) {
            DA16X_SYS_WDOG_DBG("%d", bit_idx > 9 ? bit_idx % 10 : bit_idx);
        }
        DA16X_SYS_WDOG_DBG("|");
    }
    DA16X_SYS_WDOG_DBG("\n");

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        task_mask = tasks[idx];

        for (bit_idx = 0 ; bit_idx < (da16x_sys_wdog_task_blk_size * 8) ; bit_idx++) {
            if (!(task_mask & (1 << bit_idx))) {
                DA16X_SYS_WDOG_DBG("0");
            } else {
                DA16X_SYS_WDOG_DBG("1");
            }
        }
        DA16X_SYS_WDOG_DBG("|");
    }

    DA16X_SYS_WDOG_DBG("\n\n");

    return ;
}

static int da16x_sys_watchdog_is_init()
{
    if (da16x_sys_wdog_fsm) {
        return pdTRUE;
    }

    return pdFALSE;
}

static int da16x_sys_watchdog_vaildate_id(int id)
{
    if ((id < 0) || (id >= da16x_sys_wdog_max_tasks_cnt)) {
        return pdFALSE;
    }

    return pdTRUE;
}

static int da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm_type_t *wdog_fsm)
{
    int ret = pdFALSE;
    const TickType_t timeout = portMAX_DELAY;

    if (wdog_fsm && wdog_fsm->watchdog) {
        ret = xSemaphoreTake(wdog_fsm->mutex, timeout);
        if (ret != pdTRUE) {
            DA16X_SYS_WDOG_ERR("Failed to take mutex(%d)\n", ret);
        }

        return ((ret == pdTRUE) ? 0 : -1);
    }

    DA16X_SYS_WDOG_ERR("Not initialized mutex\n");

    return -1;
}

static int da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm_type_t *wdog_fsm)
{
    int ret = pdFALSE;

    if (wdog_fsm && wdog_fsm->watchdog) {
        ret = xSemaphoreGive(wdog_fsm->mutex);
        if (ret != pdTRUE) {
            DA16X_SYS_WDOG_ERR("Failed to give mutex(%d)\n", ret);
        }

        return ((ret == pdTRUE) ? 0 : -1);
    }

    DA16X_SYS_WDOG_ERR("Not initialized mutex\n");

    return -1;
}

static int da16x_sys_watchdog_get_empty_id()
{
    int idx = 0;
    int bit_idx = 0;
    unsigned int id = 0;
    DA16X_SYS_WDOG_TASK_BLK task_mask = 0;

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        task_mask = da16x_sys_wdog_tasks_mask[idx];

        for (bit_idx = 0 ; bit_idx < (da16x_sys_wdog_task_blk_size * 8) ; bit_idx++) {
            if (!(task_mask & (1 << bit_idx))) {
                break;
            }
        }

        if (bit_idx < (da16x_sys_wdog_task_blk_size * 8)) {
            DA16X_SYS_WDOG_INFO("Found empty id(idx:%d, bit_idx:%d)\n", idx, bit_idx);
            break;
        }
    }

    //full
    if (idx >= DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT) {
        return -1;
    }

    id = bit_idx + (idx * (da16x_sys_wdog_task_blk_size * 8));

    return (int)id;
}

static void da16x_sys_watchdog_get_idx(int id, int *oidx, int *obit_idx)
{
    int idx = 0;
    int bit_idx = 0;

    if (id >= (da16x_sys_wdog_task_blk_size * 8)) {
        idx = id / (da16x_sys_wdog_task_blk_size * 8);
        bit_idx = id - (idx * (da16x_sys_wdog_task_blk_size * 8));
    } else {
        idx = 0;
        bit_idx = id;
    }

    *oidx = idx;
    *obit_idx = bit_idx;

    DA16X_SYS_WDOG_DEBUG("id(%d), idx(%d), bit_idx(%d)\n", id, idx, bit_idx);

    return ;
}

static int da16x_sys_watchdog_mask_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_mask[idx] |= (1 << bit_idx);

    return 0;
}

static int da16x_sys_watchdog_unmask_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_mask[idx] &= ~(1 << bit_idx);

    return 0;
}

static int da16x_sys_watchdog_mask_monitored_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_monitored_mask[idx] |= (1 << bit_idx);

    return 0;
}

static int da16x_sys_watchdog_unmask_monitored_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_monitored_mask[idx] &= ~(1 << bit_idx);

    return 0;
}

static int da16x_sys_watchdog_mask_notified_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_notified_mask[idx] |= (1 << bit_idx);

    return 0;
}

/* static */ int da16x_sys_watchdog_unmask_notified_task(int id)
{
    int idx = 0;
    int bit_idx = 0;

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    da16x_sys_wdog_tasks_notified_mask[idx] &= ~(1 << bit_idx);

    return 0;
}

static void da16x_sys_watchdog_active(unsigned int rescale_time)
{
    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;
	UINT32  cursysclock = 0;

    DA16X_SYS_WDOG_DEBUG("rescale_time(%d)\n", rescale_time);

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return ;
    }

    if (da16x_sys_watchdog_is_debug_mode()) {
        DA16X_SYS_WDOG_DEBUG("Not supported in debug mode\n");
        return ;
    }

    wdog_fsm = da16x_sys_wdog_fsm;

	_sys_clock_read(&cursysclock, sizeof(UINT32));
	cursysclock = DA16X_SYS_WDOG_RESCALE(cursysclock);

	if (rescale_time) {
		cursysclock = cursysclock * rescale_time; 
	}

	WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_DISABLE, NULL);
	WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_COUNTER, &cursysclock); // update
	WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_ENABLE, NULL);

    return ;
}

void da16x_sys_watchdog_enable()
{
    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;

    DA16X_SYS_WDOG_INFO("\n");

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return ;
    }

    if (da16x_sys_watchdog_is_debug_mode()) {
        DA16X_SYS_WDOG_DEBUG("Not supported in debug mode\n");
        return ;
    }

    wdog_fsm = da16x_sys_wdog_fsm;

    //WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_ENABLE, NULL);
    WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_ON, NULL);

    da16x_sys_watchdog_set_active(pdTRUE);

    return ;
}

void da16x_sys_watchdog_disable()
{
    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;

    DA16X_SYS_WDOG_INFO("\n");

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return ;
    }

    if (da16x_sys_watchdog_is_debug_mode()) {
        DA16X_SYS_WDOG_INFO("Not supported in debug mode\n");
        return ;
    }

    wdog_fsm = da16x_sys_wdog_fsm;

    //WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_DISABLE, NULL);
    WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_OFF, NULL);

    da16x_sys_watchdog_set_active(pdFALSE);

    return ;
}

void da16x_sys_watchdog_get_status(unsigned int *status)
{
    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;

    DA16X_SYS_WDOG_INFO("\n");

    *status = pdFALSE;

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return ;
    }

    wdog_fsm = da16x_sys_wdog_fsm;

    WDOG_IOCTL(wdog_fsm->watchdog, WDOG_GET_STATUS, status);

    return ;
}

static void da16x_sys_watchdog_reset_watchdog(void)
{
    memset(da16x_sys_wdog_tasks_notified_mask, 0x00, sizeof(da16x_sys_wdog_tasks_notified_mask));

    da16x_sys_watchdog_active(da16x_sys_wdog_rescale_time);
}

static void da16x_sys_watchdog_callback(void *param)
{
    int id = 0;
    int idx = 0;
    int bit_idx = 0;
    int reset_wdog_cnt = 0;
    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;

    DA16X_SYS_WDOG_TASK_BLK latency_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};
    DA16X_SYS_WDOG_TASK_BLK tmp_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};

    if (!param) {
        DA16X_SYS_WDOG_ERR("Invalid parameter\n");
        return ;
    }

    wdog_fsm = (da16x_sys_wdog_fsm_type_t *)param;

    DA16X_SYS_WDOG_INFO("Watchdog time-out\n");

#if defined(ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO)
    da16x_sys_watchdog_display_info();
#endif // ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO

    memcpy(tmp_mask, da16x_sys_wdog_tasks_monitored_mask, sizeof(tmp_mask));

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        for (bit_idx = 0 ; bit_idx < (da16x_sys_wdog_task_blk_size * 8) ; bit_idx++) {
            id = bit_idx + (idx * (da16x_sys_wdog_task_blk_size * 8));

            if (da16x_sys_wdog_tasks_latency[id] == 0) {
                continue;
            }

            da16x_sys_wdog_tasks_latency[id]--;

            latency_mask[idx] |= (1 << bit_idx);
        }
    }

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        tmp_mask[idx] &= ~latency_mask[idx];

        if ((da16x_sys_wdog_tasks_notified_mask[idx] & tmp_mask[idx]) == tmp_mask[idx]) {
            reset_wdog_cnt++;
        }
    }

    DA16X_SYS_WDOG_INFO("reset_wdog_cnt(%d)\n", reset_wdog_cnt);

    if (reset_wdog_cnt == DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT) {
        goto reset_wdog;
    }

    //da16x_sys_watchdog_display_info();

    // Enable
	WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_RESET, NULL);

    //Immediately stop. WDOG_SET_RESET must be called before calling the function.
    extern void _da16x_watchdog_isr(void);
    _da16x_watchdog_isr();

    return ;

reset_wdog:

    da16x_sys_watchdog_reset_watchdog();

    return ;
}

void da16x_sys_watchdog_cmd(int argc, char *argv[])
{
    if (argc < 1) {
        DA16X_SYS_WDOG_DBG("Invaild parameter\n");
        goto help;
    }

    if (argc == 2) {
        if (strcmp(da16x_sys_watchdog_cmd_info, argv[1]) == 0) {
            da16x_sys_watchdog_display_info();
        } else if (strcmp(da16x_sys_watchdog_cmd_disable, argv[1]) == 0) {
            da16x_sys_watchdog_disable();
        } else if (strcmp(da16x_sys_watchdog_cmd_enable, argv[1]) == 0) {
            da16x_sys_watchdog_enable();
#if defined(DA16X_SYS_WDOG_TEST_CMD)
        } else if (strcmp("test", argv[1]) == 0) {
            int sys_wdog_id = -1;

            sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

            DA16X_SYS_WDOG_DBG("Registered ID for test: %d\n", sys_wdog_id);
#endif // DA16X_SYS_WDOG_TEST_CMD
        } else {
            goto help;
        }
    } else if (argc == 3 && strcmp(da16x_sys_watchdog_cmd_rescale_time, argv[1]) == 0) {
        int ret = 0;
        unsigned int rescale_time = 0;

        ret = get_int_val_from_str(argv[2], (int *)&rescale_time, POL_1);
        if (ret) {
            DA16X_SYS_WDOG_DBG("Failed to parse rescale_time(%s)\n", argv[2]);
            goto help;
        }

        ret = da16x_sys_watchdog_set_rescale_time(rescale_time);
        if (ret) {
            DA16X_SYS_WDOG_DBG("Failed to set rescale_time(%d,%ld)\n", ret, rescale_time);
            goto help;
        }

        DA16X_SYS_WDOG_DBG("Set rescale_time(%ld)\n", da16x_sys_watchdog_get_rescale_time());
    } else {
        goto help;
    }

    return ;

help:

    DA16X_SYS_WDOG_DBG("Uasge: sys_wdog [option]\n");
    DA16X_SYS_WDOG_DBG("sys_wdog %s\n", da16x_sys_watchdog_cmd_info);
    DA16X_SYS_WDOG_DBG("sys_wdog %s\n", da16x_sys_watchdog_cmd_enable);
    DA16X_SYS_WDOG_DBG("sys_wdog %s\n", da16x_sys_watchdog_cmd_disable);
    DA16X_SYS_WDOG_DBG("sys_wdog %s time(%d ~ %d) - unit of time: 10 msec\n",
            da16x_sys_watchdog_cmd_rescale_time,
            DA16X_SYS_WDOG_MIN_RESCALE_TIME,
            DA16X_SYS_WDOG_MAX_RESCALE_TIME);

    return ;
}

void da16x_sys_watchdog_display_info(void)
{
    unsigned int status = pdFALSE;

    da16x_sys_watchdog_get_status(&status);

    DA16X_SYS_WDOG_DBG("Status: %s\n", (status ? "Enabled" : "Disabled"));
    DA16X_SYS_WDOG_DBG("Task mask\n");
    da16x_sys_watchdog_display_mask_task(da16x_sys_wdog_tasks_mask);
    DA16X_SYS_WDOG_DBG("Monitored mask\n");
    da16x_sys_watchdog_display_mask_task(da16x_sys_wdog_tasks_monitored_mask);
    DA16X_SYS_WDOG_DBG("Notified mask\n");
    da16x_sys_watchdog_display_mask_task(da16x_sys_wdog_tasks_notified_mask);
    DA16X_SYS_WDOG_DBG("IDLE task: %d\n", da16x_sys_wdog_idle_task_id);
    DA16X_SYS_WDOG_DBG("Max task id: %d\n", da16x_sys_wdog_max_task_id);
    DA16X_SYS_WDOG_DBG("Rescale time: %ld(unit of time: 10 msec)\n", da16x_sys_wdog_rescale_time);

    DA16X_SYS_WDOG_DBG("Task information\n");
    for (int idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_CNT ; idx++) {
        if (da16x_sys_wdog_task_handle[idx]) {
            DA16X_SYS_WDOG_DBG("ID: %d, latency:%d, Task handle: 0x%p, Name: %s\n",
                idx, da16x_sys_wdog_tasks_latency[idx],
                da16x_sys_wdog_task_handle[idx],
                da16x_sys_wdog_task_handle[idx]->pcTaskName);
        }
    }
}

int da16x_sys_watchdog_init(void)
{
#if defined(__AUTO_REBOOT_WDT__)
    //extern void wdt_disable_auto_goto_por();
    extern void wdt_enable_auto_goto_por();
#endif // __AUTO_REBOOT_WDT__

    da16x_sys_wdog_fsm_type_t *wdog_fsm = NULL;
    UINT32 ioctldata[3] = {0x00,};
    UINT32 abort_ioctldata = 0;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    if (da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Already allocated da16x_sys_wdog_fsm\n");
        return -1;
    }

    wdog_fsm = (da16x_sys_wdog_fsm_type_t *)da16x_sys_wdog_calloc(1, sizeof(da16x_sys_wdog_fsm_type_t));
    if (!wdog_fsm) {
        DA16X_SYS_WDOG_ERR("Failed to allocate wdog_fsm\n");
        return -1;
    }

    // WDOG Creation
    wdog_fsm->watchdog = WDOG_CREATE(WDOG_UNIT_0) ;
    if (!wdog_fsm->watchdog) {
        DA16X_SYS_WDOG_ERR("Error - WDOG_CREATE()\n");
        goto err;
    }

    // WDOG Initialization
    WDOG_INIT(wdog_fsm->watchdog);

    wdog_fsm->currtx = NULL;

    vSemaphoreCreateBinary(wdog_fsm->mutex);
    if (!wdog_fsm) {
        DA16X_SYS_WDOG_ERR("Failed to create semaphore\n");
        goto err;
    }

    if (da16x_sys_watchdog_is_debug_mode()) {
        WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_DISABLE, NULL);
        WDOG_CLOSE(wdog_fsm->watchdog);
    } else {
        //Set ABORT
        abort_ioctldata = pdTRUE;
        WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_ABORT, &abort_ioctldata);

        // WDOG Callback
        ioctldata[0] = 0;
        ioctldata[1] = (UINT32)&da16x_sys_watchdog_callback;
        ioctldata[2] = (UINT32)wdog_fsm;
        WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_CALLACK, ioctldata);
    }

#if defined(__AUTO_REBOOT_WDT__)
    //wdt_disable_auto_goto_por();    // if watchdog occur goto reset
    wdt_enable_auto_goto_por();     // if watchdog occur goto POR reset, auto reboot
#endif // __AUTO_REBOOT_WDT__

    da16x_sys_wdog_fsm = wdog_fsm;

    // Init
    memset(da16x_sys_wdog_tasks_mask, 0x00, sizeof(da16x_sys_wdog_tasks_mask));
    memset(da16x_sys_wdog_tasks_monitored_mask, 0x00, sizeof(da16x_sys_wdog_tasks_monitored_mask));
    memset(da16x_sys_wdog_tasks_notified_mask, 0x00, sizeof(da16x_sys_wdog_tasks_notified_mask));
    da16x_sys_wdog_idle_task_id = -1;
    da16x_sys_wdog_max_task_id = -1;
    memset(da16x_sys_wdog_tasks_latency, 0x00, sizeof(da16x_sys_wdog_tasks_latency));

    da16x_sys_watchdog_active(da16x_sys_wdog_rescale_time);

    return 0;

err:

    if (wdog_fsm) {
        if (wdog_fsm->watchdog) {
            // WDOG Uninitialization
            WDOG_CLOSE(wdog_fsm->watchdog);
            wdog_fsm->watchdog = NULL;
        }

        if (wdog_fsm->mutex) {
            vSemaphoreDelete(wdog_fsm->mutex);
            wdog_fsm->mutex = NULL;
        }

        da16x_sys_wdog_free(wdog_fsm);
        wdog_fsm = NULL;
    }

    da16x_sys_wdog_fsm = NULL;

    return -1;
}

int da16x_sys_watchdog_register(unsigned int notify_trigger)
{
    int ret = -1;
    int id = 0;

    //TODO: Not supported TMO function yet.
    DA16X_UNUSED_ARG(notify_trigger);

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    DA16X_SYS_WDOG_INFO("notify_trigger(%d)\n", notify_trigger);

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");

        ret = da16x_sys_watchdog_init();
        if (ret) {
            DA16X_SYS_WDOG_ERR("Failed to init sys watchdog(%d)\n", ret);
            return -1;
        }
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    id = da16x_sys_watchdog_get_empty_id();
    if (id < 0) {
        DA16X_SYS_WDOG_ERR("Not found empty id(%d)\n", id);
        goto end;
    }

    DA16X_SYS_WDOG_INFO("ID(%d)\n", id);

    da16x_sys_watchdog_mask_task(id);

    da16x_sys_watchdog_mask_monitored_task(id);

    da16x_sys_wdog_task_handle[id] = xTaskGetCurrentTaskHandle();

    if (id > da16x_sys_wdog_max_task_id) {
        da16x_sys_wdog_max_task_id = id;
    }

    ret = id;

#if defined(ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO)
    da16x_sys_watchdog_display_info();
#endif // ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO

end:
    
    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    return ret;
}

int da16x_sys_watchdog_unregister(int id)
{
    int ret = -1;

    int idx = 0;
    int bit_idx = 0;
    DA16X_SYS_WDOG_TASK_BLK tmp_mask[DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT] = {0x00,};

    DA16X_SYS_WDOG_INFO("ID(%d)\n", id);

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n", id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_watchdog_unmask_task(id);

    da16x_sys_watchdog_unmask_monitored_task(id);

    da16x_sys_wdog_tasks_latency[id] = 0;

    da16x_sys_wdog_task_handle[id] = NULL;

    memcpy(tmp_mask, da16x_sys_wdog_tasks_mask, sizeof(tmp_mask));

    //TODO: optimization
    for (idx = (DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT - 1) ; idx >= 0 ; idx--) {
        for (bit_idx = ((da16x_sys_wdog_task_blk_size * 8) - 1) ; bit_idx >= 0 ; bit_idx--) {
            if (tmp_mask[idx] & (1 << bit_idx)) {
                break;
            }
        }

        if (bit_idx >= 0) {
            break;
        }
    }

    if ((idx >= 0) && (bit_idx >= 0)) {
        da16x_sys_wdog_max_task_id = bit_idx + (idx * (da16x_sys_wdog_task_blk_size * 8));
    }

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

#if defined(ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO)
    da16x_sys_watchdog_display_info();
#endif // ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO

    return 0;
}

void da16x_sys_watchdog_configure_idle_id(int id)
{
    if (!da16x_sys_watchdog_is_active()) {
        return ;
    }

   da16x_sys_wdog_idle_task_id = id;

   //da16x_sys_wdog_task_handle[id] = xTaskGetIdleTaskHandle();
}

int da16x_sys_watchdog_suspend(int id)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }
 
    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n", id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_watchdog_unmask_monitored_task(id);

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    return 0;
}

static void da16x_sys_watchdog_resume_monitoring(int id)
{
    int idx = 0;

    da16x_sys_watchdog_mask_monitored_task(id);

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        da16x_sys_wdog_tasks_monitored_mask[idx] &= da16x_sys_wdog_tasks_mask[idx];
    }
}

int da16x_sys_watchdog_resume(int id)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n", id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_watchdog_resume_monitoring(id);

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    return 0;
}

static int da16x_sys_watchdog_notify_about_task(int id)
{
    int idx = 0;
    int bit_idx = 0;
    int reset_wdog_cnt = 0;

    DA16X_SYS_WDOG_TASK_BLK tasks_mask = 0;
    DA16X_SYS_WDOG_TASK_BLK monitored_mask = 0;
    DA16X_SYS_WDOG_TASK_BLK notified_mask = 0;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    da16x_sys_watchdog_get_idx(id, &idx, &bit_idx);

    tasks_mask = da16x_sys_wdog_tasks_mask[idx];
    monitored_mask = da16x_sys_wdog_tasks_mask[idx];

    if (tasks_mask & (1 << bit_idx)) {
        da16x_sys_watchdog_mask_notified_task(id);

        da16x_sys_wdog_tasks_latency[id] = 0;
    }

    for (idx = 0 ; idx < DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT ; idx++) {
        notified_mask = da16x_sys_wdog_tasks_notified_mask[idx];
        monitored_mask = da16x_sys_wdog_tasks_monitored_mask[idx];

        if ((notified_mask & monitored_mask) == monitored_mask) {
            reset_wdog_cnt++;
        }
    }

    if (reset_wdog_cnt == DA16X_SYS_WDOG_MAX_TASKS_ARRAY_COUNT) {
        da16x_sys_watchdog_reset_watchdog();
    }

    return -1;
}

static void da16x_sys_watchdog_notify_idle(int id)
{
    if (!da16x_sys_watchdog_is_active()) {
        return ;
    }

    if ((id != da16x_sys_wdog_idle_task_id) && (da16x_sys_wdog_idle_task_id != -1)) {
        da16x_sys_watchdog_notify(da16x_sys_wdog_idle_task_id);
    }
}

int da16x_sys_watchdog_notify(int id)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    DA16X_SYS_WDOG_DEBUG("ID(%d)\n", id);

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n", id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_watchdog_notify_about_task(id);

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    da16x_sys_watchdog_notify_idle(id);

    return 0;
}

int da16x_sys_watchdog_notify_and_resume(int id)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    DA16X_SYS_WDOG_DEBUG("ID(%d)\n", id);

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n",id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_watchdog_resume_monitoring(id);

    da16x_sys_watchdog_notify_about_task(id);

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    da16x_sys_watchdog_notify_idle(id);

    return 0;
}

int da16x_sys_watchdog_set_latency(int id, unsigned char latency)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    DA16X_SYS_WDOG_DEBUG("ID(%d)\n", id);

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    if (!da16x_sys_watchdog_vaildate_id(id)) {
        DA16X_SYS_WDOG_ERR("Invalid ID(%d)\n", id);
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    da16x_sys_wdog_tasks_latency[id] = latency;

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    return 0;
}

int da16x_sys_watchdog_set_rescale_time(unsigned int rescale_time)
{
    int ret = -1;

    if (!da16x_sys_watchdog_is_active()) {
        return -1;
    }

    if ((rescale_time < DA16X_SYS_WDOG_MIN_RESCALE_TIME)
        || (rescale_time > DA16X_SYS_WDOG_MAX_RESCALE_TIME)) {
        DA16X_SYS_WDOG_ERR("Invalid recale_time(%ld < resclae_time(%ld) < %ld)\n",
                DA16X_SYS_WDOG_MIN_RESCALE_TIME,
                rescale_time, 
                DA16X_SYS_WDOG_MAX_RESCALE_TIME);
        return -1;
    }

    if (!da16x_sys_watchdog_is_init()) {
        DA16X_SYS_WDOG_ERR("Not initialized da16x_sys_wdog_fsm\n");
        return -1;
    }

    ret = da16x_sys_watchdog_get_mutex(da16x_sys_wdog_fsm);
    if (ret) {
        DA16X_SYS_WDOG_ERR("Failed to get mutex(%d)\n", ret);
        return -1;
    }

    DA16X_SYS_WDOG_INFO("Set rescale time to %ld from %ld\n",
            rescale_time, da16x_sys_wdog_rescale_time);

    da16x_sys_wdog_rescale_time = rescale_time;

    da16x_sys_watchdog_put_mutex(da16x_sys_wdog_fsm);

    return 0;
}

unsigned int da16x_sys_watchdog_get_rescale_time(void)
{
    if (!da16x_sys_watchdog_is_active()) {
        return 0;
    }

    return da16x_sys_wdog_rescale_time;
}
