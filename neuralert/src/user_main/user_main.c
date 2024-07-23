/**
 ****************************************************************************************
 *
 * @file user_main.c
 *
 * @brief MAIN starting entry point
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

#include "driver.h"
#include "da16x_system.h"
#include "common_def.h"
#include "sys_specific.h"
#include "user_main.h"

#ifdef __SUPPORT_NAT__
#include "common_config.h"
#include "common_def.h"
#endif /* __SUPPORT_NAT__ */


/*
 *********************************************************************************
 * @brief    Configure system Pin-Mux
 * @return   None
 *********************************************************************************
 */
int config_pin_mux(void)
{
    /* DA16200 default pin-configuration */
    _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
    _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
    _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
    _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);        // For GPIO 6,7
    _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
    _da16x_io_pinmux(PIN_HMUX, HMUX_JTAG);

    /* PIN remapping for UART, SPI and SDIO */
#if defined (__ATCMD_IF_UART1__) || defined (__SUPPORT_UART1__)
  #if !defined (__UART1_FLOW_CTRL_ON__)
    _da16x_io_pinmux(PIN_CMUX, CMUX_UART1d);
  #else
    //UART1(CTS-GPIO[5],RTS-GPIO[4])
    _da16x_io_pinmux(PIN_CMUX, CMUX_UART1c);
    //UART1(RXD-GPIO[3],TXD-GPIO[2])
    _da16x_io_pinmux(PIN_BMUX, BMUX_UART1d);
  #endif
#elif defined(__ATCMD_IF_SPI__)
    _da16x_io_pinmux(PIN_BMUX, BMUX_SPIs);
    _da16x_io_pinmux(PIN_EMUX, EMUX_SPIs);
#elif defined(__ATCMD_IF_SDIO__)
    _da16x_io_pinmux(PIN_CMUX, CMUX_SDs);
    _da16x_io_pinmux(PIN_DMUX, DMUX_SDs);
    _da16x_io_pinmux(PIN_EMUX, EMUX_SDs);
#endif /* PIN remapping for UART, SPI and SDIO */

#if defined (__ATCMD_IF_UART2__) || defined (__SUPPORT_UART2__)
    _da16x_io_pinmux(PIN_UMUX, UMUX_UART2GPIO);   // UART2 for AT commands or user uart2
#endif

#if defined(__ATCMD_IF_SPI__)|| defined (__ATCMD_IF_SDIO__)
    // Set GPIOC6 as Interrupt pin
    static HANDLE gpio;

    gpio = GPIO_CREATE(GPIO_UNIT_C);
    GPIO_INIT(gpio);

    PRINTF("Initialize spi/sdio interrupt: %x\n", GPIO_ALT_FUNC_GPIO6);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_EXT_INTR, (GPIO_ALT_GPIO_NUM_TYPE)(GPIO_ALT_FUNC_GPIO6));
    GPIO_CLOSE(gpio);
#endif // __ATCMD_IF_SPI__ || __ATCMD_IF_SDIO__

    /* Need to configure by customer */
#ifdef __SUPPORT_WPS_BTN__
    SAVE_PULLUP_PINS_INFO(GPIO_UNIT_A, GPIO_PIN6);
#endif
#ifdef __SUPPORT_FACTORY_RESET_BTN__
    SAVE_PULLUP_PINS_INFO(GPIO_UNIT_A, GPIO_PIN7);
#endif

    return TRUE;
}


/**
 ******************************************************************************
 * @brief      System entry point
 * @input[in]  init_state - initalize result of pTIME and RamLib
 * @return     None
 ******************************************************************************
 */
int user_main(char init_state)
{
    int	status = 0;

    /*
     * 1. Restore saved GPIO PINs
     * 2. RTC PAD connection
     */
    __GPIO_RETAIN_HIGH_RECOVERY();

#if defined ( __REMOVE_32KHZ_CRYSTAL__ )
    /* Initialize Alternative RTC counter */
    ALT_RTC_ENABLE();
#endif // __REMOVE_32KHZ_CRYSTAL__

    /* Entry point for customer main */
    if (init_state == pdTRUE) {
        system_start();
    } else {
        PRINTF("\nFailed to initialize the RamLib or pTIM !!!\n");
    }

    return status;
}

/* EOF */
