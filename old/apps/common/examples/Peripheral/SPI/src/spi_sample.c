/**
 ****************************************************************************************
 *
 * @file spi_sample.c
 *
 * @brief Sample code for SPI
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

#if defined (__PERIPHERAL_SAMPLE_SPI__)

#include "da16x_system.h"
#include "da16200_ioconfig.h"
#include "spi_flash/spi_flash.h"

#undef SPI_LOOP_BACK_EXAMPLE
#define SPI_MASTER_SERIAL_FLASH_EXAMPLE

#ifdef SPI_LOOP_BACK_EXAMPLE
/* host protocol config */
#define HPC_HEADER_LEN				4
#define HPC_COMMON_ADDR_MODE		1
#define HPC_REF_LEN				0
#define HPC_WRITE_CMD				0x80
#define HPC_READ_CMD				0xc0

/* spi master config */
#define SMC_ADDR					0x50080270  // DA16200 Slave Interface dummy register
#define SMC_DATA_LEN				8
#define SMC_SPI_SPEED				10
#define SMC_SPI_POLARITY			SPI_TYPE_MOTOROLA_O0H0
#define SMC_SPI_CS					SPI_CSEL_0
#define SMC_IO_OPERTATION_TYPE		4

INT32 host_protocol_read(HANDLE spi, UINT32 addr, VOID *rx_data, UINT32 len);
INT32 host_protocol_write(HANDLE spi, UINT32 addr, VOID *data, UINT32 length);

/**
 * @brief SPI Loopback Example
 *
 * This example describes how to communicate SPI master to SPI slave using one DA16200 module board.
 *
 * How to run:
 * 1. Connect pins of DA16200 module board as below.
 * 	- GPIOA[0] (SPI_MISO) <--> GPIOA[9] (E_SPI_DIO1)
 * 	- GPIOA[1] (SPI_MOSI) <--> GPIOA[8] (E_SPI_DIO0)
 * 	- GPIOA[2] (SPI_CSB)  <--> GPIOA[6] (E_SPI_CSB)
 * 	- GPIOA[3] (SPI_CLK)  <--> GPIOA[7] (E_SPI_CLK)
 * 2. Build the DA16200 SDK, download the RTOS image to your DA16200 EVB and reboot.
 *
 * The DA16200 SPI slave must use the Host Protocol. See the "Host Protocol Guide" for more details.
 */
void run_spi_sample(ULONG arg)
{
    INT32 status;
    UINT8 *tx_data = NULL;
    UINT8 *rx_data = NULL;
    UINT32 ioctldata[4];
    HANDLE spi = NULL;
    DA16X_IO_ALL_PINMUX pinmux_all;

    /* backup pinmux configuration */
    _da16x_io_get_all_pinmux(&pinmux_all);

    /* pinmux config for SPI Slave - GPIOA[3:0] */
    _da16x_io_pinmux(PIN_AMUX, AMUX_SPIs);
    _da16x_io_pinmux(PIN_BMUX, BMUX_SPIs);

    /* pinmux config for SPI Host  - GPIOA[9:6] */
    _da16x_io_pinmux(PIN_DMUX, DMUX_SPIm);
    _da16x_io_pinmux(PIN_EMUX, EMUX_SPIm);

    /* create SPI master instance */
    spi = SPI_CREATE(SPI_UNIT_0);
    if (spi == NULL) {
        PRINTF("[%s]failed to create instance\n", __func__);
        return;
    }

    _sys_clock_read( ioctldata, sizeof(UINT32));
    SPI_IOCTL(spi, SPI_SET_CORECLOCK, ioctldata);

    /* set SPI speed */
    ioctldata[0] = SMC_SPI_SPEED * MHz;
    SPI_IOCTL(spi, SPI_SET_SPEED, ioctldata);

    /* set SPI polarity */
    ioctldata[0] = SMC_SPI_POLARITY;
    SPI_IOCTL(spi, SPI_SET_FORMAT, ioctldata);

    /* set SPI DMA config */
    ioctldata[0] = SPI_DMA_MP0_BST(8)
                   | SPI_DMA_MP0_IDLE(1)
                   | SPI_DMA_MP0_HSIZE(XHSIZE_DWORD)
                   | SPI_DMA_MP0_AI(SPI_ADDR_INCR);
    SPI_IOCTL(spi, SPI_SET_DMA_CFG, ioctldata);
    SPI_IOCTL(spi, SPI_SET_DMAMODE, NULL);

    /* set SPI chip select, io operation type */
    ioctldata[0] = SMC_SPI_CS;
    ioctldata[1] = SMC_IO_OPERTATION_TYPE;
    SPI_IOCTL( spi, SPI_SET_WIRE, (VOID *)ioctldata);

    /* set SPI delay index */
    ioctldata[0] = SPI_DELAY_INDEX_LOW;
    SPI_IOCTL(spi, SPI_SET_DELAY_INDEX, ioctldata);

    /* SPI initialization */
    status = SPI_INIT(spi);
    if (status == TRUE) {
        PRINTF("SPI initialization succeeded.\n");
    } else {
        PRINTF("SPI failed to initialize.\n");
        return;
    }

    /* allocate tx_data */
    tx_data = pvPortMalloc(sizeof(UINT8) * SMC_DATA_LEN);
    if (tx_data == NULL) {
        PRINTF("[%s]failed to allocate memory\n", __func__);
        return;
    }

    /* set tx data */
    for (int i = 0; i < SMC_DATA_LEN; i++) {
        tx_data[i] = i;
    }

    /* write data to slave */
    status = host_protocol_write(spi, SMC_ADDR, (VOID *)tx_data, SMC_DATA_LEN);
    if (status < 0) {
        PRINTF("[%s]write failed\n", __func__);
        return;
    }

    vTaskDelay(1);

    /* allocate rx_data */
    rx_data = pvPortMalloc(sizeof(UINT8) * SMC_DATA_LEN);
    if (rx_data == NULL) {
        PRINTF("[%s]failed to allocate memory\n", __func__);
        return;
    }

    /* read data from slave */
    status = host_protocol_read(spi, SMC_ADDR, (VOID *)rx_data, SMC_DATA_LEN);
    if (status < 0) {
        PRINTF("[%s]read failed\n", __func__);
        return;
    }

    /* check if it works properly. */
    if (memcmp(tx_data, rx_data, SMC_DATA_LEN)) {
        PRINTF("Fail\n");
    } else {
        PRINTF("Success\n");
    }

    /* restore pinmux configuration */
    _da16x_io_set_all_pinmux(&pinmux_all);

    vPortFree(tx_data);
    vPortFree(rx_data);
    SPI_CLOSE(spi);

    while (1) {
        vTaskDelay(100);
    }

    return;
}



/* This function is used to read data to SPI slave using Host Protocol. */
INT32 host_protocol_read(HANDLE spi, UINT32 addr, VOID *rx_data, UINT32 len)
{
    INT32 status;
    UINT32	ioctldata[4];
    UINT8 *_buf;

    /* allocate buffer */
    _buf = pvPortMalloc(sizeof(UINT8) * 4);
    if (_buf == NULL) {
        return -1;
    }

    /* generate host interface protocol header */
    _buf[0] = (addr >> 8) & 0xff;
    _buf[1] = (addr >> 0) & 0xff;
    _buf[2] = HPC_READ_CMD
              | (HPC_COMMON_ADDR_MODE << 5)
              | (HPC_REF_LEN << 4) | ((len >> 8) & 0xf);
    _buf[3] = (len) & 0xff;

    /* Bus Lock : CSEL0 */
    ioctldata[0] = TRUE;
    ioctldata[1] = portMAX_DELAY;
    ioctldata[2] = SPI_CSEL_0;
    SPI_IOCTL(spi, SPI_SET_LOCK, (VOID *)ioctldata);

    status = SPI_WRITE_READ(spi, _buf, 4, rx_data, len);

    /* Bus Unlock */
    ioctldata[0] = FALSE;
    ioctldata[1] = portMAX_DELAY;
    ioctldata[2] = SPI_CSEL_0;
    SPI_IOCTL(spi, SPI_SET_LOCK, (VOID *)ioctldata);

    vPortFree(_buf);
    return status;
}

/* This function is used to write data to SPI slave using Host Protocol. */
INT32 host_protocol_write(HANDLE spi, UINT32 addr, VOID *data, UINT32 length)
{
    UINT32 ioctldata[4];
    UINT8 *_buf;
    INT32 status;

    /* allocate buffer */
    _buf = pvPortMalloc(sizeof(UINT8) * (HPC_HEADER_LEN + length));
    if (_buf == NULL) {
        return -1;
    }

    /* generate host interface protocol header */
    _buf[0] = (addr >> 8) & 0xff;
    _buf[1] = (addr >> 0) & 0xff;
    _buf[2] = (HPC_WRITE_CMD & 0xff)
              | (HPC_COMMON_ADDR_MODE << 5)
              | (HPC_REF_LEN << 4) | ((length >> 8) & 0xf);
    _buf[3] = (length) & 0xff;

    /* copy data to buf */
    memcpy(&(_buf[4]), data, length);

    /* Bus Lock : CSEL0 */
    ioctldata[0] = TRUE;
    ioctldata[1] = portMAX_DELAY;
    ioctldata[2] = SPI_CSEL_0;
    SPI_IOCTL(spi, SPI_SET_LOCK, (VOID *)ioctldata);

    status = SPI_WRITE(spi, _buf, (HPC_HEADER_LEN + length));

    /* Bus Unlock */
    ioctldata[0] = FALSE;
    ioctldata[1] = portMAX_DELAY;
    ioctldata[2] = SPI_CSEL_0;
    SPI_IOCTL(spi, SPI_SET_LOCK, (VOID *)ioctldata);

    vPortFree(_buf);
    return status;
}

#endif

#ifdef SPI_MASTER_SERIAL_FLASH_EXAMPLE
#define SPI_MASTER_CLK 	5 	// 5Mhz
#define SPI_MASTER_CS 		0	// CS0
#define SPI_SFLASH_ADDRESS 0x200
#define SPI_FLASH_TXDATA_SIZE 100
#define SPI_FLASH_RXDATA_SIZE 100
#define SPI_FLASH_INPUT_DATA_SIZE 64
#define SPI_FLASH_TEST_COUNT 1000

#define	TEST_DBG_DUMP(tag, buf, siz)	thread_peripheral_dump( # buf , buf , siz)
static void thread_peripheral_dump(const char *title, unsigned char *buf,
                                   size_t len)
{
    size_t i;

    PRINTF("%s (%d, %p)", title, len, buf);
    for (i = 0; i < len; i++) {
        if ((i % 32) == 0) {
            PRINTF("\n\t");
        } else if ((i % 4) == 2) {
            PRINTF("_");
        } else if ((i % 4) == 0) {
            PRINTF(" ");
        }
        PRINTF("%c%c", "0123456789ABCDEF"[buf[i] / 16],
               "0123456789ABCDEF"[buf[i] % 16]);
    }
    PRINTF("\n");
}

void spi_flash_config_pin()
{
    _da16x_io_pinmux(PIN_DMUX, DMUX_SPIm);
    _da16x_io_pinmux(PIN_EMUX, EMUX_SPIm);
}

/**
 * * @brief SPI Master Serial Flash Example
 *
 * This example describes how to communicate SPI master to serial flash using DA16200 EVK board.
 *
 * How to run:
 * 1. Set DA16200 EVK IO voltage to 3.3V
 * 2. Connect pins of DA16200 EVK board as below.
 *     - DA16200 EVK GPIOA[6] (SPI_CSB)  <--> serial flash (CS)
 *     - DA16200 EVK GPIOA[7] (SPI_CLK)  <--> serial flash (SCLK)
 *     - DA16200 EVK GPIOA[8] (SPI_MOSI) <--> serial flash (SI)
 *     - DA16200 EVK GPIOA[9] (SPI_MISO) <--> serial flash (SO)
 *     - DA16200 EVK                 GND <--> GND
 * 3. Build this sample code, download the RTOS image to your DA16200 EVK and reboot.
 */
void run_spi_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    HANDLE spi_flash;
    UINT32 status, rnd, erase_fail_count, write_fail_count;
    UINT8 *rx_data;
    UINT8 *tx_data;

    spi_flash_config_pin();

    tx_data = APP_MALLOC(sizeof(UINT8) * SPI_FLASH_TXDATA_SIZE);
    if (tx_data == NULL) {
        PRINTF("txdata APP_MALLOC Error\n");
    }

    rx_data = APP_MALLOC(sizeof(UINT8) * SPI_FLASH_RXDATA_SIZE);
    if (rx_data == NULL) {
        PRINTF("rxdata APP_MALLOC Error\n");
    }

    spi_flash = spi_flash_open(SPI_MASTER_CLK, SPI_MASTER_CS);

    status = spi_flash_read_id(spi_flash, rx_data);
    PRINTF("status: %d\n", status);
    TEST_DBG_DUMP(0, rx_data, 3);

#if 0
    spi_flash_erase(spi_flash, SPI_SFLASH_ADDRESS);

    DRV_MEMSET(tx_data, 0, SPI_FLASH_TXDATA_SIZE);
    for (UINT32 i = 0; i < SPI_FLASH_INPUT_DATA_SIZE; i++) {
        tx_data[i] = (UINT8) i;
    }
    status = spi_flash_write(spi_flash, SPI_SFLASH_ADDRESS, tx_data,
                             SPI_FLASH_INPUT_DATA_SIZE);

    PRINTF("status: %d\n", status);
    TEST_DBG_DUMP(0, tx_data, SPI_FLASH_INPUT_DATA_SIZE);

    DRV_MEMSET(rx_data, 0, SPI_FLASH_RXDATA_SIZE);
    status = spi_flash_read(spi_flash, SPI_SFLASH_ADDRESS, rx_data,
                            SPI_FLASH_INPUT_DATA_SIZE);
    PRINTF("status: %d\n", status);
    TEST_DBG_DUMP(0, rx_data, SPI_FLASH_INPUT_DATA_SIZE);
#endif

    erase_fail_count = 0;
    write_fail_count = 0;
    for (UINT32 i = 0; i < SPI_FLASH_TEST_COUNT; i++) {
        /*
         * erase verification
         */
        DRV_MEMSET(tx_data, 0xff, SPI_FLASH_TXDATA_SIZE);
        DRV_MEMSET(rx_data, 0, SPI_FLASH_RXDATA_SIZE);
        spi_flash_erase(spi_flash, SPI_SFLASH_ADDRESS);
        spi_flash_read(spi_flash, SPI_SFLASH_ADDRESS, rx_data,
                       SPI_FLASH_INPUT_DATA_SIZE);
        if (DRV_MEMCMP(tx_data, rx_data, SPI_FLASH_INPUT_DATA_SIZE)) {
            erase_fail_count++;
        }

        /*
         * write verification
         */
        rnd = da16x_random();
        DRV_MEMSET(tx_data, (rnd) & 0xff, SPI_FLASH_TXDATA_SIZE);
        DRV_MEMSET(rx_data, 0, SPI_FLASH_RXDATA_SIZE);
        spi_flash_write(spi_flash, SPI_SFLASH_ADDRESS, tx_data,
                        SPI_FLASH_INPUT_DATA_SIZE);
        spi_flash_read(spi_flash, SPI_SFLASH_ADDRESS, rx_data,
                       SPI_FLASH_INPUT_DATA_SIZE);
        if (DRV_MEMCMP(tx_data, rx_data, SPI_FLASH_INPUT_DATA_SIZE)) {
            write_fail_count++;
        }

        PRINTF(".");
    }
    PRINTF("\nerase fail: (%d/%d)\n", erase_fail_count, SPI_FLASH_TEST_COUNT);
    PRINTF("\nwrite fail: (%d/%d)\n", write_fail_count, SPI_FLASH_TEST_COUNT);

    spi_flash_close(spi_flash);

    APP_FREE(tx_data);
    APP_FREE(rx_data);

    while (1) {
        vTaskDelay(100);				// 1000ms sleep
    }

    return;
}
#endif

#endif

/* EOF */
