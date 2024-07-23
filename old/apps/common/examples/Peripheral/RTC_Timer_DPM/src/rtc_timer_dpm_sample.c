/**
 ****************************************************************************************
 *
 * @file rtc_timer_dpm_sample.c
 *
 * @brief Sample code for RTC Timer on DPM mode
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

#include "sample_defs.h"

#if defined	(__RTC_TIMER_DPM_SAMPLE__)

#include "da16x_system.h"

#if defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
#include "da16x_dpm_regs.h"
#endif	// __SUPPORT_DPM_ABN_RTC_TIMER_WU__

#include "user_dpm.h"
#include "user_dpm_api.h"
#include "sample_defs.h"


/*
 * Defines for sample
 */
#undef	SAMPLE_FOR_DPM_SLEEP_2				// Sleep Mode 2 by external wake-up signal or RTC timer
#define	SAMPLE_FOR_DPM_SLEEP_3				// Sleep Mode 3


#define	MICROSEC_FOR_ONE_SEC		1000000
#define	RTC_TIMER_WAKEUP_ONCE		5	// seconds
#define	RTC_TIMER_WAKEUP_PERIOD		10	// seconds


#if defined ( SAMPLE_FOR_DPM_SLEEP_2 )
void rtc_timer_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    unsigned long long	wakeup_time;

    /* Just work in case of RTC timer wakeup */
    if ((dpm_mode_is_wakeup() == DPM_WAKEUP) && (dpm_mode_get_wakeup_source() != WAKEUP_COUNTER_WITH_RETENTION)) {
        dpm_app_sleep_ready_set(SAMPLE_RTC_TIMER);
        return;
    }

    /* TRUE : Maintain RTM area for DPM operation */
    wakeup_time = MICROSEC_FOR_ONE_SEC * RTC_TIMER_WAKEUP_ONCE;
    dpm_sleep_start_mode_2(wakeup_time, TRUE);
}

#elif defined ( SAMPLE_FOR_DPM_SLEEP_3 )
static void rtc_timer_dpm_once_cb(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    /*
     * Caution : Don't spend too much time
     *		inside this calback function which is called by RTC-timer.
     */
    dpm_app_sleep_ready_clear(SAMPLE_RTC_TIMER);

    PRINTF("\nWakeup due to One-Shot RTC timer !!!\n");
    vTaskDelay(10);

    dpm_app_sleep_ready_set(SAMPLE_RTC_TIMER);
}

static void rtc_timer_dpm_periodic_cb(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    /*
     * Caution : Don't spend too much time
     *		inside this calback function which is called by RTC-timer.
     */
    dpm_app_sleep_ready_clear(SAMPLE_RTC_TIMER);

    PRINTF("\nWakeup due to Periodic RTC timer!!!\n");
    vTaskDelay(10);

    dpm_app_sleep_ready_set(SAMPLE_RTC_TIMER);
}

void rtc_timer_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    ULONG	status;

    if (dpm_mode_is_wakeup() == NORMAL_BOOT) {
#if defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
        /*
         * Check DPM Abnormal state
         */
        if ((dpm_mode_get_wakeup_source() == WAKEUP_COUNTER_WITH_RETENTION) && 
            (dpm_mode_is_abnormal_wakeup() == pdTRUE)) {
            PRINTF("=== [%s] Abnormal mode ...\n", __func__);
            goto dpm_wakeup_done;
        }
#endif	// __SUPPORT_DPM_ABN_RTC_TIMER_WU__


        /*
         * Create a timer only once during normal boot.
         */

        // Clear DPM sleep flag for "SAMPLE_RTC_TIMER" task.
        dpm_app_sleep_ready_clear(SAMPLE_RTC_TIMER);

        /* One-Shot timer */
        status = dpm_timer_create(SAMPLE_RTC_TIMER,
                                  "timer1",
                                  rtc_timer_dpm_once_cb,
                                  RTC_TIMER_WAKEUP_ONCE * 1000,
                                  0); //msec
        switch ((int)status) {
        case DPM_MODE_NOT_ENABLED	:
        case DPM_TIMER_SEC_OVERFLOW	:
        case DPM_TIMER_ALREADY_EXIST	:
        case DPM_TIMER_NAME_ERROR	:
        case DPM_UNSUPPORTED_RTM	:
        case DPM_TIMER_REGISTER_FAIL	:
        case DPM_TIMER_MAX_ERR :
            PRINTF(">>> Start test DPM sleep mode 3 : Fail to create One-Shot timer (err=%d)\n", (int)status);
            // Delay to display above message on console ...
            vTaskDelay(2);

            break;
        }

        /* Periodic timer */
        status = dpm_timer_create(SAMPLE_RTC_TIMER,
                                  "timer2",
                                  rtc_timer_dpm_periodic_cb,
                                  RTC_TIMER_WAKEUP_PERIOD * 1000,
                                  RTC_TIMER_WAKEUP_PERIOD * 1000); //msec
        switch ((int)status) {
        case DPM_MODE_NOT_ENABLED	:
        case DPM_TIMER_SEC_OVERFLOW	:
        case DPM_TIMER_ALREADY_EXIST	:
        case DPM_TIMER_NAME_ERROR	:
        case DPM_UNSUPPORTED_RTM	:
        case DPM_TIMER_REGISTER_FAIL	:
        case DPM_TIMER_MAX_ERR :
            PRINTF(">>> Start test DPM sleep mode 3 : Fail to create Periodic timer (err=%d)\n", (int)status);
            // Delay to display above message on console ...
            vTaskDelay(2);

            break;
        }

        // Set DPM sleep flag for "SAMPLE_RTC_TIMER" task.
        dpm_app_sleep_ready_set(SAMPLE_RTC_TIMER);
    } else {
#if defined ( __SUPPORT_DPM_ABN_RTC_TIMER_WU__ )
dpm_wakeup_done :
#endif	// __SUPPORT_DPM_ABN_RTC_TIMER_WU__

        /* Notice initialize done to DPM module */
        dpm_app_wakeup_done(SAMPLE_RTC_TIMER);
    }

    while (1) {
        /* Nothing to do... */
        vTaskDelay(100);
    }
}
#endif	/* ... */

#endif	/* __RTC_TIMER_DPM_SAMPLE__ */

/* EOF */
