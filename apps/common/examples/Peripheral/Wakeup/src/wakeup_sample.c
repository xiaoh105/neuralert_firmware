/**
 ****************************************************************************************
 *
 * @file wakeup_sample.c
 *
 * @brief Sample code for wakeup
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

#if defined (__PERIPHERAL_SAMPLE_WAKEUP__)
#include "rtc.h"
#include "oal.h"
#include "da16x_system.h"
/* wakeup pin 3, 4 only available in 8x8 */
void rtc_wakeup_sample_ext_cb(void)
{
    unsigned long long time_old;
    UINT32 ioctl = 0;

    time_old = RTC_GET_COUNTER();
    /* Waits until the RTC register is updated*/
    while (RTC_GET_COUNTER() < (time_old + 10));
    RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &ioctl);

    if (ioctl & RTC_WAKEUP_STATUS) {
        Printf(">>> rtc1 wakeup interrupt ...\r\n" );
    }
    if (ioctl & RTC_WAKEUP2_STATUS) {
        Printf(">>> rtc2 wakeup interrupt ...\r\n" );
    }
    if (ioctl & RTC_WAKEUP3_STATUS) {
        Printf(">>> rtc3 wakeup interrupt ...\r\n" );
    }
    if (ioctl & RTC_WAKEUP4_STATUS) {
        Printf(">>> rtc4 wakeup interrupt ...\r\n" );
    }

    /* clear wakeup source */
    time_old = RTC_GET_COUNTER();
    RTC_CLEAR_EXT_SIGNAL();
    while (RTC_GET_COUNTER() < (time_old + 1));

    time_old = RTC_GET_COUNTER();
    ioctl = 0;
    RTC_IOCTL(RTC_SET_WAKEUP_SOURCE_REG, &ioctl);
    while (RTC_GET_COUNTER() < (time_old + 1));
}

static void rtc_wakeup_sample_ext_intr(void)
{
    INTR_CNTXT_SAVE();
    INTR_CNTXT_CALL(rtc_wakeup_sample_ext_cb);
    INTR_CNTXT_RESTORE();
}

void run_rtc_wakeup_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    WAKEUP_SOURCE wakeup_source;
    wakeup_source = (WAKEUP_SOURCE)da16x_boot_get_wakeupmode();
    PRINTF("RTC Wakeup Sample\n");

    /* POR (power on reset) do setting wakeup pins and GPIOC8 */
    if (wakeup_source == WAKEUP_SOURCE_POR) {
        /* enable external wakeup pins */
        UINT32 intr_src, ioctldata;

        // wakeup pin 1
        RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &intr_src);
        intr_src |= WAKEUP_INTERRUPT_ENABLE(1) | WAKEUP_POLARITY(1);
        RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &intr_src);

        // wakeup pin2
        RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &intr_src);
        intr_src |= RTC_WAKEUP2_EN(1);
        RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &intr_src);

        RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONFIG_REG, &intr_src);
        intr_src |= RTC_WAKEUP2_SEL(1); // active low
        // intr_src &= ~RTC_WAKEUP2_SEL(1); // active high
        RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONFIG_REG, &intr_src);

        // GPIOC8 wakeup setting
        RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONFIG_REG, &ioctldata);
        ioctldata &= ~(0x03ff0000);
        ioctldata |= GPIOA11_OR_GPIOC8(1);           // GPIOC8
        ioctldata |= GPIOA11_OR_GPIOC8_EDGE_SEL(1);  // 0; active high 1; active low
        RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONFIG_REG, &ioctldata);

        RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &ioctldata);
        ioctldata |= GPIOA11_OR_GPIOC8_INT_EN(1);    // GPIOC8 enable
        RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &ioctldata);
        PRINTF("Power On Reset\n");
    } else if ( (wakeup_source == WAKEUP_SOURCE_EXT_SIGNAL)
                || (wakeup_source == WAKEUP_EXT_SIG_WITH_RETENTION) ) {
        /* Check which wakeup pin occur */
        UINT32 wakeup_pin;
        wakeup_pin = RTC_GET_EXT_WAKEUP_SOURCE();
        if (wakeup_pin & 0x01) {
            PRINTF("wakeup by wakeup #1 pin\n");
        } else if (wakeup_pin & 0x02) {
            PRINTF("wakeup by wakeup #2 pin\n");
        }

        /* clear wakeup source */
        RTC_CLEAR_EXT_SIGNAL();
        RTC_READY_POWER_DOWN(1);
    } else if ( (wakeup_source == WAKEUP_SENSOR)
                || (wakeup_source == WAKEUP_SENSOR_WITH_RETENTION) ) {
        /* Check which wakeup source occur */
        UINT32 source, gpio_source, i;
        source = RTC_GET_AUX_WAKEUP_SOURCE();
        PRINTF("aux wakeup source %x\n", source);
        if ((source & WAKEUP_GPIO) == WAKEUP_GPIO) {  // GPIO wakeup
            gpio_source = RTC_GET_GPIO_SOURCE();
            PRINTF("aux gpio wakeup %x\n", gpio_source);
            RTC_CLEAR_EXT_SIGNAL();
            RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            gpio_source |= (0x3ff);
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            //check zero
            for (i = 0; i < 10; i++) {
                RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
                if ((gpio_source & 0x3ff) == 0x00) {
                    break;
                } else {
                    PRINTF("gpio wakeup clear %x\n", gpio_source);
                }
            }
            gpio_source &= ~(0x3ff);
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            for (i = 0; i < 10; i++) {
                RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
                if ((gpio_source & 0x3ff) == 0x00) {
                    break;
                } else {
                    PRINTF("gpio wakeup clear %x\n", gpio_source);
                }
            }
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
        }
        RTC_CLEAR_EXT_SIGNAL();
    }

    /* goto sleep  */
    PRINTF("wait 10s\n");
    PRINTF("it can test wakeup pin interrupt within 10s\n");
    _sys_nvic_write(RTC_ExtWkInt_IRQn, (void *)rtc_wakeup_sample_ext_intr, 0x07);
    vTaskDelay(1 * 1000);
    /* power down and wait wakeup signals */
    Printf("enter sleep mode2\r\n");
    vTaskDelay(10);
    RTC_DC12_OFF();
}
#endif
