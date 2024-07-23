/**
 ****************************************************************************************
 *
 * @file user_gpio_handle.c
 *
 * @brief Config GPIO interrupt
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

#include "system_start.h"
#include "da16x_system.h"
#include "gpio.h"


int set_gpio_interrupt(void)
{
    UINT32 ioctldata[3] = { 0, };
    HANDLE hgpio = NULL;

#ifdef __SUPPORT_WPS_BTN__
    UINT16 io = (UINT16)((1 << wps_btn) | (1 << factory_rst_btn));
#else
    UINT16 io = (1 << factory_rst_btn);
#endif /* __SUPPORT_WPS_BTN__ */

    hgpio = GPIO_CREATE(GPIO_UNIT_A);

    if (hgpio == NULL) {
        PRINTF("[%s] GPIO handle error !!!\n", __func__);
        return FALSE;
    }

    GPIO_INIT(hgpio);
    GPIO_IOCTL(hgpio, GPIO_SET_INPUT, &io);

    GPIO_IOCTL(hgpio, GPIO_GET_INTR_MODE, &ioctldata[0]);

    ioctldata[0] |= (UINT32)io;         // 1 edge, 0 level
    ioctldata[1] &= ~(UINT32)io;        // 1 high, 0 low
    GPIO_IOCTL(hgpio, GPIO_SET_INTR_MODE, &ioctldata[0]);

#ifdef __SUPPORT_WPS_BTN__
    ioctldata[0] = (UINT32)(0x01 << wps_btn);
    ioctldata[1] = (UINT32)gpio_callback;
    ioctldata[2] = (UINT32)wps_btn;
    GPIO_IOCTL(hgpio, GPIO_SET_CALLACK, ioctldata);
#endif /* __SUPPORT_WPS_BTN__ */

    ioctldata[0] = (UINT32)(0x01 << factory_rst_btn);
    ioctldata[1] = (UINT32)gpio_callback;
    ioctldata[2] = (UINT32)factory_rst_btn;
    GPIO_IOCTL(hgpio, GPIO_SET_CALLACK, ioctldata);

    GPIO_IOCTL(hgpio, GPIO_SET_INTR_ENABLE, &io);

    return TRUE;
}

/* EOF */
