/**
 ****************************************************************************************
 *
 * @file patch.h
 *
 * @brief ROM patch functions
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

#ifndef __patch_h__
#define __patch_h__

typedef void (*RTC_BOR_CALLBACK )(void);
extern INT32 __GPIO_RETAIN_HIGH_RECOVERY();

extern void RTC_ENABLE_BROWN_BLACK(RTC_BOR_CALLBACK brown_callback, RTC_BOR_CALLBACK black_callback);
extern void RTC_DISABLE_BROWN_BLACK(void);



#define SUPPORT_GPIO_RETENTION_LOW
//#undef SUPPORT_GPIO_RETENTION_LOW

#ifdef SUPPORT_GPIO_RETENTION_LOW

/**
 ****************************************************************************************
 * @brief This function keeps the GPIO pin high during sleep.
 * @param[in] gpio_port the GPIO port name
 * @param[in] gpio_num the GPIO pin name
 * @return True/False
 * 
 ****************************************************************************************
 */
INT32 _GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num);

/**
 ****************************************************************************************
 * @brief This function keeps the GPIO pin low during sleep.
 * @param[in] gpio_port the GPIO port name
 * @param[in] gpio_num the GPIO pin name
 * @return True/False
 * 
 ****************************************************************************************
 */
INT32 _GPIO_RETAIN_LOW(UINT32 gpio_port, UINT32 gpio_num);
#endif

#endif /* __patch_h__ */
