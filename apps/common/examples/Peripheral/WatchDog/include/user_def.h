/**
 ****************************************************************************************
 *
 * @file user_def.h
 *
 * @brief Define for User system definition variables for SDK
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

#ifndef __USER_DEF_H__
#define __USER_DEF_H__

#include "da16x_types.h"
#include "sdk_type.h"

#define INTERRUPT_LOW              0
#define INTERRUPT_HIGH             1

#define BUTTON1_GPIO               7                /* GPIO 7 */
#define BUTTON2_GPIO               6                /* GPIO 6 */

#define FACTORY_BUTTON             BUTTON1_GPIO
#define WPS_BUTTON                 BUTTON2_GPIO

#define CHECK_TIME_FACTORY_BTN     5                /* 5 sec. */
#define CHECK_TIME_WPS_BTN         1                /* 1 sec. */

#if defined ( __SUPPORT_EVK_LED__ )

#if defined (__ATCMD_IF_SPI__)
  #error "Config error: If __ATCMD_IF_SPI__ is defined, assign unused GPIO for LEDs"
#endif // __ATCMD_IF_SPI__

  #define BUTTON1_LED              2                /* GPIO 2 */
  #define BUTTON2_LED              3                /* GPIO 3 */

  #define FACTORY_LED              BUTTON1_LED
  #define WPS_LED                  BUTTON2_LED

  #define BLINK_FAST_TIME          10               /* 100ms */
  #define BLINK_MID_TIME           30               /* 300ms */
  #define BLINK_SLOW_TIME          50               /* 500ms */

  #define LED_OFF                  0
  #define LED_ON                   1
  #define LED_BLINK                2

#endif    // __SUPPORT_EVK_LED__

#endif /* __USER_DEF_H__ */

/* EOF */
