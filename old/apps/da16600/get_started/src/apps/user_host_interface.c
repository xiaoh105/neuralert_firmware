/**
 ****************************************************************************************
 *
 * @file user_host_interface.c
 *
 * @brief User host interfaces
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

#include "command_host.h"
#include "sdk_type.h"
#include "da16x_system.h"
#include "da16x_types.h"
#include "common_uart.h"
#include "target.h"

/**************************************************************************
 * External Variables
 **************************************************************************/

extern unsigned int    hostif;

/**************************************************************************
 * Internal Functions
 **************************************************************************/

void user_host_interface_init(void);

/**************************************************************************
 * External Functions
 **************************************************************************/

extern void host_spi_slave_init(void);
extern void host_i2c_slave_init(void);
extern void host_callback_handler(UINT32 address);


void user_host_interface_init(void)
{
    if (hostif == HOSTIF_SPI) {
        host_spi_slave_init();
    } else if (hostif == HOSTIF_UART) {

    } else if (hostif == HOSTIF_I2C) {
        host_i2c_slave_init();
    } else if (hostif == HOSTIF_SDIO) {

        SDIO_SLAVE_INIT();

        PRINTF(">>> Initialize SDIO Slave ...\n");

        SDIO_SLAVE_CALLBACK_REGISTER(host_callback_handler);
        DA16X_DIOCFG->EXT_INTB_CTRL =  0xa;

        // drive strength
    }
}

/* EOF */
