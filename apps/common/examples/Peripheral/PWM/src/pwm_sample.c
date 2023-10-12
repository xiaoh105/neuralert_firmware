/**
 ****************************************************************************************
 *
 * @file pwm_sample.c
 *
 * @brief Sample app of PWM
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

#if defined (__PWM_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"

/*
 * Sample Functions for PWM
 */
#include "pwm.h"
#include "gpio.h"

void Board_Init()
{
    // GPIO[9] - INTR_OUT,  GPIO[8] - PWM_OUT
    _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
    _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
}

void pwm_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    PRINTF("PWM_SAMPLE\n");

    HANDLE pwm[4];
    HANDLE	gpio = NULL;
    unsigned int period, duty_percent, cycle, duty_cycle;

    Board_Init();
    DA16X_CLOCK_SCGATE->Off_CAPB_PWM = 0;


    gpio = GPIO_CREATE(GPIO_UNIT_A);
    GPIO_INIT(gpio);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_PWM_OUT0, GPIO_ALT_FUNC_GPIO0);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_PWM_OUT1, GPIO_ALT_FUNC_GPIO1);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_PWM_OUT2, GPIO_ALT_FUNC_GPIO2);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_PWM_OUT3, GPIO_ALT_FUNC_GPIO3);

    pwm[0] = DRV_PWM_CREATE(pwm_0);
    pwm[1] = DRV_PWM_CREATE(pwm_1);
    pwm[2] = DRV_PWM_CREATE(pwm_2);
    pwm[3] = DRV_PWM_CREATE(pwm_3);

    DRV_PWM_INIT(pwm[0]);
    DRV_PWM_INIT(pwm[1]);
    DRV_PWM_INIT(pwm[2]);
    DRV_PWM_INIT(pwm[3]);

    period = 10; // 10us
    duty_percent = 30; //30%, duration high 3us per 10us
    DRV_PWM_START(pwm[0], period, duty_percent, PWM_DRV_MODE_US); //PWM Start

    period = 20; // 20us
    duty_percent = 40; //40%, duration high 8us per 10us
    DRV_PWM_START(pwm[1], period, duty_percent, PWM_DRV_MODE_US); //PWM Start

    period = 40; // 40us
    duty_percent = 50; //50%, duration high 20us per 10us
    DRV_PWM_START(pwm[2], period, duty_percent, PWM_DRV_MODE_US); //PWM Start

    period = 80; // 80us
    duty_percent = 80; //80%, duration high 64us per 10us
    DRV_PWM_START(pwm[3], period, duty_percent, PWM_DRV_MODE_US); //PWM Start

    /* 10Sec delay */
    vTaskDelay(1000);

    // 2400 cycles(=30us @ 80MHz), cycle = value + 1
    cycle = 2400 - 1;
    // 1680 cycles(=21us@80MHz, 70% Duty High), duty_cycle = value + 1
    duty_cycle = 1680 - 1;
    DRV_PWM_START(pwm[0], cycle, duty_cycle, PWM_DRV_MODE_CYC); //PWM Start

    // 2400 cycles(=30us @ 80MHz), cycle = value + 1
    cycle = 2400 - 1;
    // 1680 cycles(=21us@80MHz, 70% Duty High), 70% Duty High), duty_cycle = value + 1
    duty_cycle = 1680 - 1;
    DRV_PWM_START(pwm[1], cycle, duty_cycle, PWM_DRV_MODE_CYC); //PWM Start

    // 2400 cycles(=30us @ 80MHz), cycle = value + 1
    cycle = 2400 - 1;
    // 1680 cycles(=21us@80MHz, 70% Duty High), 70% Duty High), duty_cycle = value + 1
    duty_cycle = 1680 - 1;
    DRV_PWM_START(pwm[2], cycle, duty_cycle, PWM_DRV_MODE_CYC); //PWM Start

    // 2400 cycles(=30us @ 80MHz), cycle = value + 1
    cycle = 2400 - 1;
    // 1680 cycles(=21us@80MHz, 70% Duty High), 70% Duty High), duty_cycle = value + 1
    duty_cycle = 1680 - 1;
    DRV_PWM_START(pwm[3], cycle, duty_cycle, PWM_DRV_MODE_CYC); //PWM Start

    /* 10Sec delay */
    vTaskDelay(1000);

    DRV_PWM_STOP(pwm[0], 0);
    DRV_PWM_STOP(pwm[1], 0);
    DRV_PWM_STOP(pwm[2], 0);
    DRV_PWM_STOP(pwm[3], 0);

    vTaskDelete(NULL);

    return ;
}
#endif // (__PWM_SAMPLE__)
