/**
 ****************************************************************************************
 *
 * @file spi_slave_sample.h
 *
 * @brief Definition for SPI Slave sample app.
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

#ifdef __PERIPHERAL_SAMPLE_SPI_SLAVE__

//----------------------------------------------------------
//    Definitions
//----------------------------------------------------------

#define HOST_ILLEGAL_CMD            (0xffffffff)

#define HOST_MEM_WRITE_REQ          (0x80)
#define HOST_MEM_WRITE_RES          (0x81)
#define HOST_MEM_READ_REQ           (0x82)
#define HOST_MEM_READ_RES           (0x83)
#define HOST_MEM_WRITE              (0x84)
#define HOST_MEM_WRITE_DONE         (0x85)
#define HOST_MEM_READ               (0x86)
#define HOST_MEM_READ_DONE          (0x87)


#define  HOST_DMS_SESSION_OPEN      (0xE1)
#define  HOST_DMS_SESSION_OPEN_RES  (0xE2)
#define  FCI_DMS_SESSION_CLOSE      (0xE3)
#define  FCI_DMS_VIDEO_REQ          (0xB0)
#define  HOST_DMS_VIDEO_RES         (0xB1)
#define  FCI_DMS_AUDIO_REQ          (0xA0)
#define  HOST_DMS_AUDIO_RES         (0xA1)
#define  FCI_DMS_RVS_AUDIO_RES      (0xC0)
#define  FCI_DMS_MCU_IMAGE          (0xD0)
#define  FCI_DMS_SOC_IMAGE          (0xD1)
#define  FCI_DMS_SNAP_REQ           (0xF0)
#define  HOST_DMS_SNAP_RES          (0xF1)

#define COMMAND_INT_ENABLE          (0x1<<15)
#define AT_CMD_INT_ENABLE           (0x1<<14)
#define PROCESS_END_INT_ENABLE      (0x1<<13)
#define PROTOCOL_MODE               (0x1<<12)
#define SPI_SLAVE_RESET             (0x1<<11)
#define SPI_SLAVE_MD_MISO           (0x1<<10)
#define SPI_SLAVE_SPEED             (0x1<<9)
#define SPI_SLAVE_ENDIAN_MD         (0x1<<8)
#define SPI_SLAVE_CPOL              (0x1<<7)
#define SPI_SLAVE_CPHASE            (0x1<<6)

#define SPI_SLAVE_CHIP_ID           (4)
#define SPI_SLAVE_D_BUS_WIDTH       (2)
#define SPI_SLAVE_A_BUS_WIDTH       (0)

#define SPI_READ                    (0x40)
#define SPI_WRITE                   (0x00)
#define SPI_AINC                    (0x80)
#define SPI_COMMON_ADDR_EN          (0x20)
#define SPI_REF_LEN_EN              (0x10)

#define SPI_SLAVE_HDR8              (0x1000)

#define SLAVE_INT_STATUS_CMD        (0x1 << 15)
#define SLAVE_INT_STATUS_ATCMD      (0x1 << 14)
#define SLAVE_INT_STATUS_END        (0x1 << 13)

#define SLAVE_CMD_SIZE              (256)

#define EXT_INTR_CONF               (0x50001020) //
#define EXT_INTR_SET                (0x50001024) //


#define PRINT_HOST    PRINTF


typedef struct _st_host_request {
    u16    host_write_length;
    u8    host_cmd;
    u8    dummy;
} st_host_request;

typedef struct _st_host_response {
    u32    buf_address;
    u16    host_length;
    u8    resp;
    u8    dummy;
} st_host_response;

enum {
    STATE_WAIT_G_WR_REQ = 0,
    STATE_WAIT_G_WR_RES,
    STATE_WAIT_G_WR_SET_LENGTH,
    STATE_WAIT_G_WR_DATA,
    STATE_WAIT_G_RD_REQ,
    STATE_WAIT_G_RD_RES,
    STATE_WAIT_G_RD_SET_LENGTH,
    STATE_WAIT_G_RD_DATA
};

enum {
    WAIT_CMD = 0,
    WAIT_WRITE_DATA,
    WAIT_READ_DATA
};

enum {
    HOSTIF_SPI = 0,
    HOSTIF_UART,
    HOSTIF_I2C,
    HOSTIF_SDIO

};

#define HOST_IF_LOCK_ENABLE

#ifdef HOST_IF_LOCK_ENABLE

#define HOST_OBTAIN_SEMAPHORE         OAL_OBTAIN_SEMAPHORE
#define HOST_RELEASE_SEMAPHORE        OAL_RELEASE_SEMAPHORE

#else
#define HOST_OBTAIN_SEMAPHORE(x,y)
#define HOST_RELEASE_SEMAPHORE(x)
#endif

//----------------------------------------------------------
//Prototype
//
//----------------------------------------------------------
//----------------------------------------------------------

void sample_thread_spi(void *param);


#endif /* __PERIPHERAL_SAMPLE_SPI_SLAVE__ */

/* EOF */
