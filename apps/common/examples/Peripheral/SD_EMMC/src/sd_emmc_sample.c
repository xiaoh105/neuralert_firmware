/**
 ****************************************************************************************
 *
 * @file sd_emmc_sample.c
 *
 * @brief Sample code for SD / EMMC
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

#if defined (__PERIPHERAL_SAMPLE_SD_EMMC__)

#include "da16x_system.h"
#include "da16200_ioconfig.h"
#include "driver.h"

#define MAX_EMMC_BLOCK_COUNT	10 		// 0x14		//128 0x80
#define EMMC_BLOCK_LEN			512

#define EMMC_CLK_DIV_VAL		0x0b	// output clock = bus clock / emmc_clk_div_val

EMMC_HANDLER_TYPE *_emmc = 0;
int err, card_insert = 0;
static char *write_buf = NULL, *read_buf = NULL;
static UINT32 addr, block_len, loop = 0, failcount = 0;
//UINT32 wtemp;

static int get_width(int digi)
{
    int x = 10, width = 1;

    while ((digi / x) != 0) {
        x *= 10;
        width++;
    }

    return width;
}

void emmc_init()
{
    DA16X_CLOCK_SCGATE->Off_SysC_HIF = 0;
    DA16X_SYSCLOCK->CLK_DIV_EMMC = EMMC_CLK_DIV_VAL;
    DA16X_SYSCLOCK->CLK_EN_SDeMMC = 0x01;	// clock enable

    /*
     * SDIO Master
     */
    // GPIO[9] - mSDeMMCIO_D0,  GPIO[8] - mSDeMMCIO_D1
    _da16x_io_pinmux(PIN_EMUX, EMUX_SDm);
    // GPIO[5] - mSDeMMCIO_CLK,  GPIO[4] - mSDeMMCIO_CMD
    _da16x_io_pinmux(PIN_CMUX, CMUX_SDm);
    // GPIO[7] - mSDeMMCIO_D2,  GPIO[6] - mSDeMMCIO_D3
    _da16x_io_pinmux(PIN_DMUX, DMUX_SDm);


    if (write_buf == NULL) {
        write_buf = pvPortMalloc(EMMC_BLOCK_LEN * MAX_EMMC_BLOCK_COUNT);

        if (write_buf == NULL) {
            Printf("write buffer malloc fail\n");
        }
    }

    if (read_buf == NULL) {
        read_buf = pvPortMalloc(EMMC_BLOCK_LEN * MAX_EMMC_BLOCK_COUNT);

        if (read_buf == NULL) {
            Printf("read buffer malloc fail\n");
        }

        memset((void *) read_buf, 0xcc, EMMC_BLOCK_LEN * MAX_EMMC_BLOCK_COUNT);
    }

    if (_emmc == NULL) {
        _emmc = EMMC_CREATE();
    }

    err = EMMC_INIT(_emmc);

    if (err) {
        Printf("emmc_init fail\n");
    } else {
        card_insert = 1;

        if (_emmc->card_type) {
            Printf("%s is inserted\n",
                   ((_emmc->card_type == SD_CARD) ? "SD card" :
                    (_emmc->card_type == SD_CARD_1_1) ? "SD_CARD 1.1" :
                    (_emmc->card_type == SDIO_CARD) ?
                    "SDIO_CARD" : "MMC card"));
        }

        if (_emmc->card_type == SDIO_CARD && _emmc->sdio_num_info > 0) {
            unsigned int i = 0;

            for (i = 0; i < _emmc->sdio_num_info; i++) {
                PRINTF("%s\n", _emmc->psdio_info[i]);
            }
        }

        // set sd clock
        DA16X_SYSCLOCK->CLK_DIV_EMMC = 3;
    }

}

void emmc_verify()
{
    addr = 210;
    block_len = MAX_EMMC_BLOCK_COUNT;
    loop = 100;
    failcount = 0;
    {
        unsigned int i, j;
        int k;

        for (i = 0; i < loop; i++) {
            for (j = 0; j < block_len * EMMC_BLOCK_LEN; j++) {
                write_buf[j] = (char) rand();
                //write_buf[j] = (char)j;
            }

            err = EMMC_WRITE(_emmc, addr, (write_buf), block_len);

            if (err) {
                Printf("write error  %x\n", err);
                failcount++;
                continue;
            }

            err = EMMC_READ(_emmc, addr, (void *) read_buf, block_len);

            if (err) {
                failcount++;
                Printf("E");
            }

            if (DRIVER_MEMCMP(write_buf, read_buf, 512 * block_len) == 0) {
                Printf("%d", i);

                for (k = 0; k < get_width(i); k++) {
                    Printf("\b");
                }
            } else {
                failcount++;
                PRINTF("X");
            }
        }
    }
    Printf("fail / total %d / %d\n", failcount, loop);
}

void run_sd_emmc_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    Printf("emmc sample start\r\n");
    emmc_init();
    emmc_verify();

    while (1) {
        vTaskDelay(10);
    }
}
#endif
