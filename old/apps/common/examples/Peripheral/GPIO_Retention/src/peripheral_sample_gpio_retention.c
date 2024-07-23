/**
 ****************************************************************************************
 *
 * @file peripheral_sample_gpio.c
 *
 * @brief GPIO sample
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

#if defined (__PERIPHERAL_SAMPLE_GPIO_RETENTION__)
#include "da16x_system.h"
#include "da16200_ioconfig.h"

extern INT32 _GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num);
extern INT32 _GPIO_RETAIN_LOW(UINT32 gpio_port, UINT32 gpio_num);
extern void	cmd_setsleep(int argc, char *argv[]);

/**
 * @brief GPIO retention sample
 *
 * If GPIO pin is set to retention high, it is kept in the high state during the sleep period.
 * If GPIO pin is set to retention low, it is kept in the low state during the sleep period.
 *
 * How to run:
 * 1. Build the DA16200 SDK, download the RTOS image to your DA16200 EVB and reboot.
 * 2. Toggling switch 13 (SW13).
 * 3. Check that the GPIOA [10: 8] and GPIOC [7] remain PIN states with the oscilloscope.
 *
 * DA16200 module board PIN mapping
 * - GPIOA[8]  (GPIO_8)
 * - GPIOA[9]  (GPIO_9)
 * - GPIOA[10] (GPIO_10)
 * - GPIOC[7]  (GPIO_13)
 *
 */
void run_gpio_retention_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    INT32 ret;
    /*
     * 1. Set to GPIOA[11:8], GPIOC[8:6]
     * 2. Need be written to "config_pin_mux" function.
     */
    _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
    _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);

    /* Set GPIOA[9:8] to retention high */
    ret = _GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN8 | GPIO_PIN9);
    if (ret == FALSE) {
        PRINTF("GPIO_RETAIN_HIGH() return false.\n");
    }

    /* Set GPIOA[10] to retention low */
    ret = _GPIO_RETAIN_LOW(GPIO_UNIT_A, GPIO_PIN10);
    if (ret == FALSE) {
        PRINTF("GPIO_RETAIN_LOW() return false.\n");
    }

    /* Set GPIOC[7] to retention high */
    ret = _GPIO_RETAIN_HIGH(GPIO_UNIT_C, GPIO_PIN7);
    if (ret == FALSE) {
        PRINTF("GPIO_RETAIN_HIGH() return false.\n");
    }

    PRINTF("GPIOA[9:8], GPIOC[7] retention high.\n");
    PRINTF("GPIOA[10] retention low.\n");

    PRINTF("sleep 10 seconds.\n");
    vTaskDelay(100);		// 1000ms sleep


    /*
     * Activate Sleep
     */
    char *_argv[4] = {"setsleep", "2", "10", "1"};
    cmd_setsleep(4, _argv);
}

#endif
