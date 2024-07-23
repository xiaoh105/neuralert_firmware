/**
 ****************************************************************************************
 *
 * @file gpio_handle.c
 *
 * @brief GPIO polling thread
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

#include "FreeRTOSConfig.h"
#include "common_def.h"

#include "da16x_system.h"
#include "gpio.h"

#include "da16x_sys_watchdog.h"


/*
 * Local externel APIs
 */

int gpio_number = 0;

int factory_rst_btn = FACTORY_BUTTON;
#if defined ( __SUPPORT_EVK_LED__ )
int factory_rst_led = FACTORY_LED;
#endif    // __SUPPORT_EVK_LED__
int factory_rst_btn_chk_time = CHECK_TIME_FACTORY_BTN;

int wps_btn = WPS_BUTTON;
#if defined ( __SUPPORT_EVK_LED__ )
int wps_led = WPS_LED;
#endif    // __SUPPORT_EVK_LED__
int wps_btn_chk_time = CHECK_TIME_WPS_BTN;


/* gpio_event_task */
#define GPIO_EV_TASK_NAME        "GPIO_ev"
#define GPIO_EV_TASK_STACK_SIZE  (1024 / sizeof(unsigned long))

#define GPIO_EVENT               0x01
#define GPIO_EVENT_WAIT_TICK     (100 / portTICK_PERIOD_MS)

EventGroupHandle_t  evt_grp_sys_gpio = NULL;
TaskHandle_t        gpio_ev_task = NULL;

#if defined ( __SUPPORT_EVK_LED__ )
extern UINT check_factory_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
extern UINT check_factory_button(int btn_gpio_num, int check_time);
#endif    // __SUPPORT_EVK_LED__

extern UINT factory_reset_default(int reboot_flag);
extern void reboot_func(UINT flag);

#ifdef __SUPPORT_WPS_BTN__
#if defined ( __SUPPORT_EVK_LED__ )
extern UINT check_wps_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
extern UINT check_wps_button(int btn_gpio_num, int check_time);
#endif    // __SUPPORT_EVK_LED__

extern UINT wps_setup(char *macaddr);
#endif    /* __SUPPORT_WPS_BTN__ */


#if defined ( __SUPPORT_EVK_LED__ )
void config_factory_reset_button(int reset_gpio, int led_gpio, int seconds)
{
    factory_rst_btn = reset_gpio;
    factory_rst_led = led_gpio;
    factory_rst_btn_chk_time = seconds;
}

void config_wps_button(int wps_gpio, int led_gpio, int seconds)
{
    wps_btn = wps_gpio;
    wps_led = led_gpio;
    wps_btn_chk_time = seconds;
}
#else
void config_factory_reset_button(int reset_gpio, int seconds)
{
    factory_rst_btn = reset_gpio;
    factory_rst_btn_chk_time = seconds;
}

void config_wps_button(int wps_gpio, int seconds)
{
    wps_btn = wps_gpio;
    wps_btn_chk_time = seconds;
}
#endif    // __SUPPORT_EVK_LED__

static void gpio_event_task(void *param)
{
    int sys_wdog_id = -1;

    DA16X_UNUSED_ARG(param);

    EventBits_t gpio_ev_bits;

#if defined ( __SUPPORT_FACTORY_RESET_BTN__ ) || defined ( __SUPPORT_WPS_BTN__ )
    UINT status = 0;
#endif    // __SUPPORT_FACTORY_RESET_BTN__ || __SUPPORT_WPS_BTN__

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        gpio_ev_bits = xEventGroupWaitBits(
                                  evt_grp_sys_gpio,
                                  GPIO_EVENT,
                                  pdTRUE,
                                  pdFALSE,
                                  GPIO_EVENT_WAIT_TICK);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        // Event occured
        if (gpio_ev_bits & GPIO_EVENT) {
            xEventGroupClearBits(evt_grp_sys_gpio, GPIO_EVENT);
        } else {
            continue;
        }

#ifdef __SUPPORT_FACTORY_RESET_BTN__
        if (gpio_number == factory_rst_btn) {    /* GPIO 7 */ /* Factory Reset */
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

  #if defined ( __SUPPORT_EVK_LED__ )
            status = check_factory_button(factory_rst_btn, factory_rst_led, factory_rst_btn_chk_time);
  #else
            status = check_factory_button(factory_rst_btn, factory_rst_btn_chk_time);
  #endif    // __SUPPORT_EVK_LED__

            if (status == 1) {
                factory_reset_default(1);
            } else if (status ==  2) {    /* Reboot */
                reboot_func(2);
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        } else
#endif    /* __SUPPORT_FACTORY_RESET_BTN__ */

#ifdef __SUPPORT_WPS_BTN__
        if (gpio_number == wps_btn) {    /* GPIO 6 */ /* WPS */
            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

  #if defined ( __SUPPORT_EVK_LED__ )
            status = check_wps_button(wps_btn, wps_led, wps_btn_chk_time);
  #else
            status = check_wps_button(wps_btn, wps_btn_chk_time);
  #endif    // __SUPPORT_EVK_LED__

            if (status == 1) {
                wps_setup(NULL);
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
#endif /* __SUPPORT_WPS_BTN__ */

        gpio_number = 0;
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

int gpio_handle_create_event(void)
{
    if (!evt_grp_sys_gpio) {
        // Create sync-up event
        evt_grp_sys_gpio = xEventGroupCreate();
        if (evt_grp_sys_gpio == NULL) {
            PRINTF("\n\n>>> Failed to create GPIO event group !!!\n\n");
            return -1;
        }
    }

    return 0;
}

void start_gpio_thread(void)
{
    BaseType_t    status;

    // Create GPIO event handling task
    status = xTaskCreate(gpio_event_task,
                         GPIO_EV_TASK_NAME,
                         GPIO_EV_TASK_STACK_SIZE,
                         (void *)NULL,
                         OS_TASK_PRIORITY_USER + 1,    // Don't assign as 0 priority
                         &gpio_ev_task);

    if (status != pdPASS) {
        PRINTF(RED_COLOR " [%s] polling_gpio thread creation fail (status:%d) \n" CLEAR_COLOR, __func__, status);
    }
}

/*
 * Checking flag if interrupt occurs or not
 */
void gpio_callback(void *param)
{
    BaseType_t xResult;

    gpio_number = (int)param;

    if (evt_grp_sys_gpio) {
        // Set event to GPIO_event task : callback function for GPIO interrupt
        xResult = xEventGroupSetBitsFromISR(evt_grp_sys_gpio,
                                            GPIO_EVENT,
                                            pdFALSE);    // xHigherPriorityTaskWoken

        if (xResult != pdFAIL) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
             * switch should be requested. The macro used is port specific and will
             * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
             * the documentation page for the port being used. */
            portYIELD_FROM_ISR(pdFALSE);
        }
    }
}

/* EOF */
