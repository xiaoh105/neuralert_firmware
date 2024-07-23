/**
 ****************************************************************************************
 *
 * @file sdio_sample.c
 *
 * @brief sdio slave sample
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

#if defined (__PERIPHERAL_SAMPLE_SDIO__)

#include "driver.h"
#include "da16x_system.h"
#include "da16200_ioconfig.h"

void run_sdio_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    Printf("sdio_slave start\r\n");
    /*
     * SDIO Slave
     */
    // GPIO[9] - SDIO_D0,  GPIO[8] - SDIO_D1
    _da16x_io_pinmux(PIN_EMUX, EMUX_SDs);
    // GPIO[5] - SDIO_CLK,  GPIO[4] - SDIO_CMD
    _da16x_io_pinmux(PIN_CMUX, CMUX_SDs);
    // GPIO[7] - SDIO_D2,  GPIO[6] - SDIO_D3
    _da16x_io_pinmux(PIN_DMUX, DMUX_SDs);

    // clock enable sdio_slave
    DA16X_CLOCK_SCGATE->Off_SSI_M3X1 = 0;
    DA16X_CLOCK_SCGATE->Off_SSI_SDIO = 0;

    SDIO_SLAVE_INIT();
    /* now the sdio host can access the DA16200 */
    Printf("now the sdio host can access the DA16200\r\n");
    while (1) {
        vTaskDelay(100);
    }

    return;
}
#endif
