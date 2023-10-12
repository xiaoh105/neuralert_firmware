/**
 ****************************************************************************************
 *
 * @file user_sensor.h
 *
 * @brief Header file for the example for an implementation of user sensor task.
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

#ifndef COMBO_USER_SENSOR_USER_SENSOR_H_
#define COMBO_USER_SENSOR_USER_SENSOR_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "sdk_type.h"

#if defined (__USER_SENSOR__)
#include "stdbool.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/****************************************************************************************/
/* The name of the user sensor task                                                     */
/****************************************************************************************/
#define USER_SENSOR_TAG                    "Sensor"

/****************************************************************************************/
/* The sampling period                                                                  */
/****************************************************************************************/
#define USER_SENSOR_SAMPLING_PERIOD_SEC    (5000)  // 5s

/****************************************************************************************/
/* EVENTS for the user sensor                                                           */
/****************************************************************************************/
#define USER_SENSOR_EVT_ENABLE             ( 1UL << 0)
#define USER_SENSOR_EVT_DISABLE            ( 1UL << 1)
#define USER_SENSOR_EVT_SENSE              ( 1UL << 2)

#if defined(__IOT_SENSOR_SVC_ENABLE__)
extern struct iot_sensor_svc_cb user_sensor_svc_callback;
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the user sensor
 ****************************************************************************************
 */
void user_sensor_init(void);

/**
 ****************************************************************************************
 * @brief Deinitialize the user sensor
 ****************************************************************************************
 */
void user_sensor_deinit(void);

/**
 ****************************************************************************************
 * @brief Task for processing events of the user sensor
 ****************************************************************************************
 */
void user_sensor_task(void *pvParameters);

/**
 ****************************************************************************************
 * @brief Enable or disable the user sensor
 ****************************************************************************************
 */
void user_sensor_enable(bool onoff);

#endif // __USER_SENSOR__

#endif /* COMBO_USER_SENSOR_USER_SENSOR_H_ */

/* EOF */
