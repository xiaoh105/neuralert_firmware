/**
 ****************************************************************************************
 *
 * @file da16x_sys_watchdog.h
 *
 * @brief Watchdog service
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

#ifndef __DA16X_SYS_WATCHDOG_H__
#define __DA16X_SYS_WATCHDOG_H__

#include "sdk_type.h"
#include "common_def.h"
#include "da16x_system.h"
#include "da16x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SUPPORT_SYS_WATCHDOG__) && defined(WATCH_DOG_ENABLE)
    #error "WATCH_DOG_ENABLE must be disabled."
#endif // WATCH_DOG_ENABLE

#define DA16X_SYS_WDOG_RESCALE(x)               ((x)/100)   // 10 msec

#define DA16X_SYS_WDOG_MIN_RESCALE_TIME         (10)        // 20 * 10 msec
#define DA16X_SYS_WDOG_MAX_RESCALE_TIME         (3000)      // 3000 * 10 msec
#define DA16X_SYS_WDOG_DEF_RESCALE_TIME         (500)       // 500 * 10 msec

void da16x_sys_watchdog_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Display current information of da16x_sys_watchdog module over CLI.
 ****************************************************************************************
 */
void da16x_sys_watchdog_display_info(void);

/**
 ****************************************************************************************
 * @brief Initialize da16x_sys_watchdog module.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_init(void);

/**
 ****************************************************************************************
 * @brief Register current task in da16x_sys_watchdog module.
 * @param[in] notify_trigger true if task notify should be triggered periodically.
 *
 * \note notify_trigger is not implemeted yet.
 *
 * @return identifier on success, -1 on failure.
 ****************************************************************************************
 */
int da16x_sys_watchdog_register(unsigned int notify_trigger);

/**
 ****************************************************************************************
 * @brief Unregister task from da16x_sys_watchdog module.
 * @param[in] id identifier.
 *
 * @return 0 on success
 ****************************************************************************************
 */
int da16x_sys_watchdog_unregister(int id);

/**
 ****************************************************************************************
 * @brief Inform da16x_sys_watchdog module about the wdog id the IDLE task.
 * @param[in] id identifier.
 ****************************************************************************************
 */
void da16x_sys_watchdog_configure_idle_id(int id);

/**
 ****************************************************************************************
 * @brief Suspend task monitoring in da16x_sys_watchdog module.
 * @param[in] id identifier.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_suspend(int id);

/**
 ****************************************************************************************
 * @brief Resume task monitoring in da16x_sys_watchdog module.
 * @param[in] id identifier.
 *
 * \note This function does not notify the watchdog service for this task. It is possible that
 *       monitor resuming occurs too close to the time that the watchdog expires, before the task
 *       has a chance to explicitly send a notification. This can lead to an unwanted reset.
 *       Therefore, either call da16x_sys_watchdog_notify()
 *       before calling da16x_sys_watchdog_resume(), or use da16x_sys_watchdog_notify_and_resume() instead.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_resume(int id);

/**
 ****************************************************************************************
 * @brief Notify da16x_sys_watchdog module for task.
 *        Registered task shall use this periodically to notify sys_watchdog module that it's alive.
 *        This should be done frequently enough to fit into hw_watchdog interval.
 * @param[in] id identifier
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_notify(int id);

/**
 ****************************************************************************************
 * @brief Notify da16x_sys_watchdog module for task with handle \p id and resume its monitoring.
 *        This function combines the funtionality of da16x_sys_watchdog_notify()
 *        and da16x_sys_watchdog_resume().
 * @param[in] id identifier.
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_notify_and_resume(int id);

/**
 ****************************************************************************************
 * @brief Set watchdog latency for task.
 *        This allows task to miss given number of notifications to da16x_sys_watchdog
 *        without triggering platform reset.
 *        Once set, it's allowed that task does not notify sys_watchdog for \p latency consecutive
 *        hw_watchdog intervals which can be used to allow for parts of code which are known
 *        to block for long period of time (i.e. computation).
 *        This value is set once and does not reload automatically,
 *        thus it shall be set every time increased latency is required.

 * @param[in] id identifier.
 * @param[in] latency latency.
 *
 * @return 
 ****************************************************************************************
 */
int da16x_sys_watchdog_set_latency(int id, unsigned char latency);

/**
 ****************************************************************************************
 * @brief Set watchdog rescale time.
 * @param rescale_time rescale time (unit of time: 10 msec)
 *
 * @return 0 on success.
 ****************************************************************************************
 */
int da16x_sys_watchdog_set_rescale_time(unsigned int rescale_time);

/**
 ****************************************************************************************
 * @brief Get watchdog rescale time.
 *
 * @return rescale time(unit of time: 10 msec)
 ****************************************************************************************
 */
unsigned int da16x_sys_watchdog_get_rescale_time(void);

/**
 ****************************************************************************************
 * @brief Enable watchdog.
 *
 * \note da16x_sys_watchdog must be enabled.
 ****************************************************************************************
 */
void da16x_sys_watchdog_enable();

/**
 ****************************************************************************************
 * @brief Disable watchdog.
 ****************************************************************************************
 */
void da16x_sys_watchdog_disable();

/**
 ****************************************************************************************
 * @brief Get watchdog status.
 * @param[out] status pdTRUE is watchdog is enabled.
 ****************************************************************************************
 */
void da16x_sys_watchdog_get_status(unsigned int *status);

#ifdef __cplusplus
}
#endif

#endif // __DA16X_SYS_WATCHDOG__H__
