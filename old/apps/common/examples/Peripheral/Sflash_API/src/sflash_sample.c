/**
 ****************************************************************************************
 *
 * @file sflash_sample.c
 *
 * @brief Sample app of SFLASH device
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

#if defined (__SFLASH_API_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"

extern UINT user_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size);
extern UINT user_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size);
extern void hex_dump(UCHAR *data, UINT length);

/******************************************************************************
 *  test_sflash_read(void)
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void test_sflash_read(void)
{
    UCHAR	*rd_buf = NULL;
    UINT	rd_addr;
    UINT	status;

#define	SFLASH_RD_TEST_ADDR	SFLASH_USER_AREA_START
#define	TEST_RD_SIZE		(1 * 1024)

    rd_buf = (UCHAR *)pvPortMalloc(TEST_RD_SIZE);

    if (rd_buf == NULL) {
        PRINTF("[%s] malloc fail ...\n", __func__);
        return;
    }

    memset(rd_buf, 0, TEST_RD_SIZE);

    rd_addr = SFLASH_RD_TEST_ADDR;
    status = user_sflash_read(rd_addr, (VOID *)rd_buf, TEST_RD_SIZE);

    if (status == TRUE) {
        hex_dump(rd_buf, 128);
    }

    vPortFree(rd_buf);
}

/******************************************************************************
 *  test_sflash_write(void)
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void test_sflash_write(void)
{
    UCHAR	*wr_buf = NULL;
    UINT	wr_addr;

#define	SFLASH_WR_TEST_ADDR	SFLASH_USER_AREA_START
#define	TEST_WR_SIZE		SF_SECTOR_SZ

    wr_buf = (UCHAR *)pvPortMalloc(TEST_WR_SIZE);

    if (wr_buf == NULL) {
        PRINTF("[%s] malloc fail ...\n", __func__);
        return;
    }

    memset(wr_buf, 0, TEST_WR_SIZE);

    for (int i = 0; i < TEST_WR_SIZE; i++) {
        wr_buf[i] = 0x41;	// A
    }

    wr_addr = SFLASH_WR_TEST_ADDR;

    PRINTF("=== SFLASH Write Data ======================\n");

    user_sflash_write(wr_addr, wr_buf, TEST_WR_SIZE);
}

/*
 * Sample Functions for TEMPLATE
 */
void SFLASH_API_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    /* DO SOMETHING */
    PRINTF("SFLASH_API_SAMPLE\n");

    test_sflash_write();

    vTaskDelay(10);	// Dealy 100 msec

    test_sflash_read();

    vTaskDelete(NULL);

    return ;
}
#endif // (__SFLASH_API_SAMPLE__)
