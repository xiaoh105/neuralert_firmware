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

#if defined (__PERIPHERAL_SAMPLE_GPIO__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"

#define GPIO_INT_TYPE_EDGE			1
#define GPIO_INT_TYPE_LEVEL			0
#define GPIO_INT_POL_HIGH			1
#define GPIO_INT_POL_LOW			0

#undef PAD_PULL_CONTROL					/* example of PAD pull up/down control */
#undef GPIOC6_BOTH_EDGE_INTERRUPT		/* example of both edge interrupt control */
#undef RTC_GPO_CONTROL					/* example of RTC_GPO control */
/******************************************************************************
 *  Board_initialization( )
 *
 *  Purpose : Board Initialization
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int Board_initialization(void)
{
    /* AMUX to GPIOA[1:0] */
    _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);

    /* BMUX to GPIOA[3:2] */
    _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);

    /* BMUX to GPIOA[5:4] */
    _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);

    _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
    _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);


    return TRUE;
}

/*
 * Sample Functions for GPIO
 */
static int set_gpio_interrupt(HANDLE handler, UINT8 pin_num, UINT8 int_type, UINT8 int_pol, void *callback_func)
{
    UINT16 pin, int_en_status;
    UINT32 ioctldata[3];
    int ret;

    if (15 < pin_num ) {
        return FALSE;
    }

    if (handler == NULL) {
        return FALSE;
    }

    pin = 0x01 << pin_num;
    ret = GPIO_IOCTL(handler, GPIO_SET_INPUT, &pin);

    ret = GPIO_IOCTL(handler, GPIO_GET_INTR_MODE, &ioctldata[0]);
    /* interrupt type 1: edge, 0: level*/
    ioctldata[0] &= ~(1 << pin_num);
    ioctldata[0] |= (int_type << pin_num);
    /* interrupt pol 1: high active, 0: low active */
    ioctldata[1] &= ~(1 << pin_num);
    ioctldata[1] |= (int_pol << pin_num);
    ret = GPIO_IOCTL(handler, GPIO_SET_INTR_MODE, &ioctldata[0]);

    /* register callback function */
    ioctldata[0] = pin; /* interrupt pin */
    ioctldata[1] = (UINT32) callback_func; /* callback function */
    ioctldata[2] = (UINT32) pin_num; /* param data */
    ret = GPIO_IOCTL(handler, GPIO_SET_CALLACK, ioctldata);

    ret = GPIO_IOCTL(handler, GPIO_GET_INTR_ENABLE, &int_en_status);
    int_en_status |= pin;
    ret = GPIO_IOCTL(handler, GPIO_SET_INTR_ENABLE, &int_en_status);

    return ret;
}

static int clear_gpio_interrupt(HANDLE handler, UINT8 pin_num)
{
    UINT16 pin;

    if (15 < pin_num ) {
        return FALSE;
    }

    if (handler == NULL) {
        return FALSE;
    }

    pin = 0x01 << pin_num;
    GPIO_IOCTL(handler, GPIO_SET_INTR_DISABLE, &pin);

    return TRUE;
}

static void gpio_callback(void *param)
{
    UINT32 data = (UINT32) param;
    Printf("GPIO interrupt- Pin %x \r\n", data);
}

#ifdef GPIOC6_BOTH_EDGE_INTERRUPT
HANDLE gpioc;
static void gpioc_callback(void *param)
{
    UINT32 pin;
    UINT16 read_data;

    pin = 1 << (UINT32)param;
    GPIO_READ(gpioc, pin, &read_data, sizeof(UINT16));
    set_gpio_interrupt(gpioc, (UINT32)param, GPIO_INT_TYPE_EDGE, !(pin & read_data), (void *)gpioc_callback );

#ifdef GPIO_INTERRUPT_DEBUG
    Printf("GPIO interrupt- Pin %x 0x%04x\r\n", (UINT32)param, read_data);
#endif
}
#endif

void peripheral_sample_gpio(void *param)
{
    DA16X_UNUSED_ARG(param);

    HANDLE gpio;
    UINT32 pin, toggle = 0, loop_cnt = 0;;
    UINT16 read_data, write_data;

    /*
     * board init
     */
    Board_initialization();

    /*
     * GPIOA port init
     */
    gpio = GPIO_CREATE(GPIO_UNIT_A);
    GPIO_INIT(gpio);

    /*
     * GPIOA[0],GPIOA[4] output high low toggle
     */
    pin = GPIO_PIN0 | GPIO_PIN4;
    GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &pin);

    /*
     * GPIOA[1] input
     */
    pin = GPIO_PIN1;
    GPIO_IOCTL(gpio, GPIO_SET_INPUT, &pin);

#if PAD_PULL_CONTROL
    /*
     * GPIOA[1] input pull control it can make gpio pad pull up or pull down or HIZ
     */
    _da16x_gpio_set_pull(GPIO_UNIT_A, GPIO_PIN1, PULL_UP);
    /* or */
    _da16x_gpio_set_pull(GPIO_UNIT_A, GPIO_PIN1, PULL_DOWN);
    /* or */
    _da16x_gpio_set_pull(GPIO_UNIT_A, GPIO_PIN1, HIGH_Z);
#endif

    /*
     * GPIOA[2] interrupt active low , Edge trigger
     */
    set_gpio_interrupt(gpio, 2, GPIO_INT_TYPE_EDGE, GPIO_INT_POL_LOW, (void *)gpio_callback );

    /*
    * GPIOA[3] interrupt active high , Edge trigger
    */
    set_gpio_interrupt(gpio, 3, GPIO_INT_TYPE_EDGE, GPIO_INT_POL_HIGH, (void *)gpio_callback );

#ifdef GPIOC6_BOTH_EDGE_INTERRUPT
    /*
     * GPIOC port init
     */
    gpioc = GPIO_CREATE(GPIO_UNIT_C);
    GPIO_INIT(gpioc);
    /*
     * GPIOC[6] interrupt Both edge
     */
    pin = GPIO_PIN6;
    GPIO_IOCTL(gpioc, GPIO_SET_INPUT, &pin);

    GPIO_READ(gpioc, GPIO_PIN6, &read_data, sizeof(UINT16));

    set_gpio_interrupt(gpioc, 6, GPIO_INT_TYPE_EDGE, !(GPIO_PIN6 & read_data), (void *)gpioc_callback );
#endif
#ifdef RTC_GPO_CONTROL
    RTC_GPO_OUT_INIT(1);			// 0: auto, 1: manual
#endif
    while (1) {
        if (toggle) {
            /* GPIOA[0],GPIOA[4] to high */
            write_data = GPIO_PIN0 | GPIO_PIN4;
            GPIO_WRITE(gpio, GPIO_PIN0 | GPIO_PIN4, &write_data, sizeof(UINT16));
#ifdef RTC_GPO_CONTROL
            RTC_GPO_OUT_CONTROL(toggle);
#endif
            toggle = 0;
        } else {
            /* GPIOA[0],GPIOA[4] to low*/
            write_data = 0;
            GPIO_WRITE(gpio, GPIO_PIN0 | GPIO_PIN4, &write_data, sizeof(UINT16));
#ifdef RTC_GPO_CONTROL
            RTC_GPO_OUT_CONTROL(toggle);
#endif
            toggle = 1;
        }

        GPIO_READ(gpio, GPIO_PIN1, &read_data, sizeof(UINT16));

        PRINTF("GPIOA[0] output %s, GPIOA[4] output %s, GPIOA[1] input %s\n",
               (write_data & GPIO_PIN0) ? "high" : "low",
               (write_data & GPIO_PIN4) ? "high" : "low",
               (read_data & GPIO_PIN1) ? "high" : "low");

        vTaskDelay(100);				// 1000ms sleep

        if (++loop_cnt == 30) {
            PRINTF("Clear GPIO interrupt \n");
            /* Disable GPIO2,3 interrupt */
            clear_gpio_interrupt(gpio, 2);
            clear_gpio_interrupt(gpio, 3);
        }
    }
    GPIO_CLOSE(gpio);
}
#endif // (__TEMPLATE_SAMPLE__)
