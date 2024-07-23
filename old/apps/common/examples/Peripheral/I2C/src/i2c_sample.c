/**
 ****************************************************************************************
 *
 * @file i2c_sample.c
 *
 * @brief Sample app of I2C
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

#if defined (__I2C_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"

/*
 * Sample Functions for I2C
 */
#include "sys_i2c.h"

#define PRINT_I2C 	DRV_PRINTF

#define	AT_I2C_LENGTH_FOR_WORD_ADDRESS		(2)
#define	AT_I2C_DATA_LENGTH			(32)
#define	AT_I2C_FIRST_WORD_ADDRESS		(0x00)
#define	AT_I2C_SECOND_WORD_ADDRESS		(0x00)
#define	AT_I2C_ERROR_DATA_CHECK			(0x8)

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
    /////////////////////////////////////
    // MPW3 EVB's PIN Configuration
    /////////////////////////////////////

    _da16x_io_pinmux(PIN_AMUX, AMUX_I2Cm);

    return TRUE;
}

void i2c_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    PRINTF("I2C_SAMPLE\n");
    int status;
    HANDLE I2C;

    status = FALSE;

    // Device Address for AT24C512
    UINT32 addr = 0xa0;

    // I2C Working Clock [KHz]
    UINT32 i2c_clock = 400;

    // Buffer for read from EEPROM
    UINT8 i2c_data_read[AT_I2C_DATA_LENGTH];

    UINT8 i2c_data[AT_I2C_DATA_LENGTH + AT_I2C_LENGTH_FOR_WORD_ADDRESS] = {0,};

    /////////////////////////////////////
    // Board Initialization
    /////////////////////////////////////

    Board_initialization();

    /////////////////////////////////////
    // I2C EEPROM Test
    /////////////////////////////////////

    PRINTF("I2C EEPROM Test start ...\r\n");

    DA16X_CLOCK_SCGATE->Off_DAPB_I2CM = 0;
    DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

    // Create Handle for I2C Device
    I2C = DRV_I2C_CREATE(i2c_0);

    // Initialization I2C Device
    DRV_I2C_INIT(I2C);

    // Set Address for Atmel eeprom AT24C512
    DRV_I2C_IOCTL(I2C, I2C_SET_CHIPADDR, &addr);

    // Set I2C Working Clock. Unit = KHz
    DRV_I2C_IOCTL(I2C, I2C_SET_CLOCK, &i2c_clock);

    // GPIO Select for I2C working. GPIO1 = SCL, GPIO0= SDA
    //	_da16x_io_pinmux(PIN_AMUX, AMUX_I2Cm);

    // Data Random Write  to EEPROM
    // Address = 0, Length = 32, Word Address Length = 2
    // [Start] - [Device addr. W] -  [1st word addr.] - [2nd word addr.] - [wdata0] ~ [wdata31] - [Stop]

    i2c_data[0] = AT_I2C_FIRST_WORD_ADDRESS; //Word Address to Write Data. 2 Bytes. refer at24c512 DataSheet
    i2c_data[1] = AT_I2C_SECOND_WORD_ADDRESS; //Word Address to Write Data. 2 Bytes. refer at24c512 DataSheet

    // Fill Ramp Data
    for (int i = 0; i < AT_I2C_DATA_LENGTH; i++) {
        i2c_data[i + AT_I2C_LENGTH_FOR_WORD_ADDRESS] = i;
    }

    status = DRV_I2C_WRITE(I2C, i2c_data,
                           // Handle, buffer, length, stop enable, dummy
                           AT_I2C_DATA_LENGTH + AT_I2C_LENGTH_FOR_WORD_ADDRESS, 1, 0);

    if (status != TRUE) {
        PRINTF("ret : 0x%08x\r\n", status);
    }

    // Delay for AT24C512
    vTaskDelay(2);

    // Data Random Read  from EEPROM
    // Address = 0, Length = 32, Word Address Length = 2
    // [Start] - [Device addr. W] - [1st word addr.] - [2nd word addr.]  - [Start] -[Device addr. R] - [rdata0] ~ [rdata31] - [Stop]

    // Word Address to Write Data. 2 Bytes. refer at24c512 DataSheet
    i2c_data_read[0] = AT_I2C_FIRST_WORD_ADDRESS;

    //Word Address to Write Data. 2 Bytes. refer at24c512 DataSheet
    i2c_data_read[1] = AT_I2C_SECOND_WORD_ADDRESS;

    // Handle, buffer, length, address length, dummy
    status = DRV_I2C_READ(I2C, i2c_data_read, AT_I2C_DATA_LENGTH, AT_I2C_LENGTH_FOR_WORD_ADDRESS, 0);

    if (status != TRUE) {
        PRINTF("ret : 0x%08x\r\n", status);
    }

    // Check Data
    for (int i = 0; i < AT_I2C_DATA_LENGTH; i++) {
        if (i2c_data_read[i] != i2c_data[i + AT_I2C_LENGTH_FOR_WORD_ADDRESS]) {
            PRINTF("	%dth data is different W:0x%02x, R:0x%02x\r\n", i,
                   i2c_data[i + AT_I2C_LENGTH_FOR_WORD_ADDRESS],
                   i2c_data_read[i]);
            status = AT_I2C_ERROR_DATA_CHECK;
        }
    }

    if (status != AT_I2C_ERROR_DATA_CHECK) {
        PRINTF("***** 32 Bytes Data Write and Read Success *****\r\n");
    }

    // Data Current Address  Read from EEPROM
    // Length = 32, Word Address Length = 0
    // [Start] -[Device addr. R] - [rdata0] ~ [rdata31] - [Stop]

    // Handle, buffer, length, address length, dummy
    status = DRV_I2C_READ(I2C, i2c_data_read, 4, 0, 0);

    if (status != TRUE) {
        PRINTF("ret : 0x%08x\r\n", status);
    }

    vTaskDelete(NULL);

    return ;
}
#endif // (__I2C_SAMPLE__)
