/**
 ****************************************************************************************
 *
 * @file user_sensor.c
 *
 * @brief Example for an implementation of user sensor task.
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdbool.h>
#include <stdlib.h>

#include "user_sensor.h"

#include "FreeRTOSConfig.h"
#include "common_def.h"
#include "limits.h"
#include "timers.h"

#if defined (__USER_SENSOR__)
#if defined (__IOT_SENSOR_SVC_ENABLE__)
#include "../include/iot_sensor_user_custom_profile.h"
#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static TaskHandle_t user_sensor_task_handle = NULL;
static TimerHandle_t user_sensor_timer = NULL;

#if defined(__IOT_SENSOR_SVC_ENABLE__)
static void user_sensor_svc_enabled(bool enabled);
static void user_sensor_notify_enabled(bool enabled);

struct iot_sensor_svc_cb user_sensor_svc_callback = {
    .enabled = user_sensor_svc_enabled,
    .notify_enabled = user_sensor_notify_enabled,
};
#endif // __IOT_SENSOR_SVC_ENABLE__


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if defined(__IOT_SENSOR_SVC_ENABLE__)
static void user_sensor_svc_enabled(bool enabled)
{
    user_sensor_enable(enabled);
}

static void user_sensor_notify_enabled(bool enabled)
{
    PRINTF("User sensor notify %s!\r\n", enabled ? "enabled" : "disabled");
}
#endif // __IOT_SENSOR_SVC_ENABLE__

/**
 ****************************************************************************************
 * @brief Get value from sensor
 ****************************************************************************************
 */
static uint8_t user_sensor_get_value(void)
{
    return (uint8_t)(20 + (rand() % 10));
}

/**
 ****************************************************************************************
 * @brief The callback for the user sensor timer
 ****************************************************************************************
 */
static void user_sensor_timer_cb(TimerHandle_t xTimer)
{
    DA16X_UNUSED_ARG(xTimer);

    if (user_sensor_task_handle) {
        xTaskNotifyIndexed(user_sensor_task_handle, 0, USER_SENSOR_EVT_SENSE, eSetBits);
    }
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Task for processing events of the user sensor
 ****************************************************************************************
 */
void user_sensor_task(void *pvParameters)
{
    DA16X_UNUSED_ARG(pvParameters);

    uint32_t ulNotifiedValue;

    user_sensor_task_handle = xTaskGetCurrentTaskHandle();

// @formatter:off
    user_sensor_timer = xTimerCreate("Sensor tmr",
                                     USER_SENSOR_SAMPLING_PERIOD_SEC / 10,
                                     pdTRUE,
                                     (void*)0,
                                     user_sensor_timer_cb);
    configASSERT(user_sensor_timer);
// @formatter:on

// @formatter:off
    for (;;) {
        /* Block indefinitely (without a timeout, so no need to check the function's
           return value) to wait for a notification.
           Bits in this RTOS task's notification value are set by the notifying
           tasks and interrupts to indicate which events have occurred. */
        xTaskNotifyWaitIndexed(0,                 /* Wait for 0th notification. */
                               0x00,              /* Don't clear any notification bits on entry. */
                               ULONG_MAX,         /* Reset the notification value to 0 on exit. */
                               &ulNotifiedValue,  /* Notified value pass out in ulNotifiedValue. */
                               portMAX_DELAY);    /* Block indefinitely. */

        /* Process any events that have been latched in the notified value. */
        if ((ulNotifiedValue & USER_SENSOR_EVT_ENABLE) != 0) {
            if (user_sensor_timer) {
                xTimerStart(user_sensor_timer, portMAX_DELAY);
                PRINTF("User sensor enabled!\r\n");
            }
        }

        if ((ulNotifiedValue & USER_SENSOR_EVT_DISABLE) != 0) {
            if (user_sensor_timer) {
                xTimerStop(user_sensor_timer, portMAX_DELAY);
                PRINTF("User sensor disabled!\r\n");
            }
        }

        if ((ulNotifiedValue & USER_SENSOR_EVT_SENSE) != 0) {
#if defined(__IOT_SENSOR_SVC_ENABLE__)
            uint8_t value = user_sensor_get_value();

            iot_sensor_svc_set_temp(value);
#endif
        }
    }
// @formatter:on

    if (user_sensor_timer) {
        xTimerDelete(user_sensor_timer, portMAX_DELAY);
        user_sensor_timer = NULL;
    }

    vTaskDelete(NULL);
}

/**
 ****************************************************************************************
 * @brief Initialize the user sensor
 ****************************************************************************************
 */
void user_sensor_init(void)
{
    BaseType_t xReturned;

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
                            user_sensor_task,          /* Function that implements the task. */
                            USER_SENSOR_TAG,           /* Text name for the task. */
                            128,                       /* Stack size in words, not bytes. */
                            (void*)UNDEF_PORT,         /* Parameter passed into the task. */
                            tskIDLE_PRIORITY + 1,      /* Priority at which the task is created. */
                            &user_sensor_task_handle); /* Used to pass out the created task's handle. */
    configASSERT(xReturned == pdPASS);
}

/**
 ****************************************************************************************
 * @brief Deinitialize the user sensor
 ****************************************************************************************
 */
void user_sensor_deinit(void)
{
    if (user_sensor_timer) {
        xTimerDelete(user_sensor_timer, portMAX_DELAY);
        user_sensor_timer = NULL;
    }

    if (user_sensor_task_handle) {
        vTaskDelete(user_sensor_task_handle);
        user_sensor_task_handle = NULL;
    }
}

/**
 ****************************************************************************************
 * @brief Enable or disable the user sensor
 ****************************************************************************************
 */
void user_sensor_enable(bool onoff)
{
    if (user_sensor_task_handle) {
        xTaskNotifyIndexed(user_sensor_task_handle, 0,
                           (onoff) ? USER_SENSOR_EVT_ENABLE : USER_SENSOR_EVT_DISABLE, eSetBits);
    }
}

#endif // __USER_SENSOR__

/* EOF */
