/**
 ****************************************************************************************
 *
 * @file user_interface.c
 *
 * @brief User interfaces
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

#if defined (__SUPPORT_ATCMD__)

#include "command_host.h"
#include "da16x_system.h"
#include "da16x_types.h"
#include "common_uart.h"
#include "atcmd.h"
#include "user_dpm.h"
#include "user_dpm_api.h"


#define ATCMD_UART_CONF_NAME            "ATCMD_UART_CONF"
#define NVR_KEY_ATCMD_UART_BAUDRATE     "ATCMD_BAUDRATE"
#define NVR_KEY_ATCMD_UART_BITS         "ATCMD_BITS"
#define NVR_KEY_ATCMD_UART_PARITY       "ATCMD_PARITY"
#define NVR_KEY_ATCMD_UART_STOPBIT      "ATCMD_STOPBIT"
#define NVR_KEY_ATCMD_UART_FLOWCTRL     "ATCMD_FLOWCTRL"

#if defined ( __USER_UART_CONFIG__ )
/*
 * Customer configuration for AT-CMD UART
 */
uart_info_t ATCMD_UART_config_info = 
{
    UART_BAUDRATE_115200,    /* baud */
    UART_DATABITS_8,         /* bits */
    UART_PARITY_NONE,        /* parity */
    UART_STOPBITS_1,         /* stopbit */ 
#if defined (__UART1_FLOW_CTRL_ON__)
    UART_FLOWCTL_ON         /* flow control */
#else
    UART_FLOWCTL_OFF         /* flow control */
#endif    
};
#endif // __USER_UART_CONFIG__

/**************************************************************************
 * Internal Functions
 **************************************************************************/
void user_interface_init(void);

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static void atcmd_UART_init(void);
static HANDLE uart_init(int index, int _baud, int _bits, int _parity, int _stopbit, int _flow_ctrl);
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

/**************************************************************************
 * External Variables
 **************************************************************************/
#if  defined ( __ATCMD_IF_UART2__ )
extern HANDLE    uart2;
#elif defined ( __ATCMD_IF_UART1__ ) 
extern HANDLE    uart1;
#endif

/**************************************************************************
 * External Functions
 **************************************************************************/
#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
extern int _uart_dma_init(HANDLE handler);
#else
extern void   host_init(void);
#endif // __ATCMD_IF_UART1__ ||  __ATCMD_IF_UART2__ 

#if defined ( __USER_UART_CONFIG__ )
extern uart_info_t ATCMD_UART_config_info;
#endif // __USER_UART_CONFIG__

void user_interface_init(void)
{
#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
    atcmd_UART_init();
#elif defined ( __ATCMD_IF_SPI__ )
    extern unsigned int    hostif;

    hostif = HOSTIF_SPI;
    host_init();
#elif defined ( __ATCMD_IF_SDIO__ )
    extern unsigned int    hostif;

    hostif = HOSTIF_SDIO;
    host_init();
#endif // __ATCMD_IF_UART1__ ||__ATCMD_IF_UART2__
}

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static HANDLE uart_init(int index, int _baud, int _bits, int _parity, int _stopbit, int _flow_ctrl)
{
    HANDLE    handler;

    UINT32    clock = 40000000 * 2;
    UINT32    parity_en = 0;
    UINT32    fifo_en = 1;
    UINT32    DMA = 1;
    UINT32    swflow = 0;
    UINT32    word_access = 0;
    UINT32    rw_word = 0;

    UINT32    baud = (UINT32)_baud;
    UINT32    bits = (UINT32)_bits;
    UINT32    parity = (UINT32)_parity;
    UINT32    stopbit = (UINT32)_stopbit;
    UINT32    flow_ctrl = (UINT32)_flow_ctrl;

    UINT32    temp;

    if (index == UART_UNIT_1) {
        handler = UART_CREATE(UART_UNIT_1);
    } else if (index == UART_UNIT_2) {
        DA16X_CLOCK_SCGATE->Off_CAPB_UART2 = 0;
        handler = UART_CREATE(UART_UNIT_2);
    } else {
        PRINTF("[%s] Wrong AT-CMD UART index ..\n", __func__);
        return NULL;
    }

    /* Configure uart */
    UART_IOCTL(handler, UART_SET_CLOCK, &clock);
    UART_IOCTL(handler, UART_SET_BAUDRATE, &baud);

    if (!dpm_mode_is_wakeup()) {
        PRINTF("\n>>> %s : Clock=%d, BaudRate=%d\n", (index == UART_UNIT_2 ? "UART2" : "UART1"), clock, baud);
    }

    temp =   UART_WORD_LENGTH(bits) | UART_FIFO_ENABLE(fifo_en) | UART_PARITY_ENABLE(parity_en)
           | UART_EVEN_PARITY(parity) | UART_STOP_BIT(stopbit);
    UART_IOCTL(handler, UART_SET_LINECTRL, &temp);

    temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(flow_ctrl);
    UART_IOCTL(handler, UART_SET_CONTROL, &temp);

    temp = UART_RX_INT_LEVEL(UART_HALF) | UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
    UART_IOCTL(handler, UART_SET_FIFO_INT_LEVEL, &temp);

    temp = DMA;
    UART_IOCTL(handler, UART_SET_USE_DMA, &temp);

    /* FIFO Enable */
    temp =   UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT | UART_INTBIT_TIMEOUT | UART_INTBIT_ERROR
           | UART_INTBIT_FRAME | UART_INTBIT_PARITY | UART_INTBIT_BREAK | UART_INTBIT_OVERRUN;

    UART_IOCTL(handler, UART_SET_INT, &temp);

    temp = swflow;
    UART_IOCTL(handler, UART_SET_SW_FLOW_CONTROL, &temp);

    temp = word_access;
    UART_IOCTL(handler, UART_SET_WORD_ACCESS, &temp);

    temp = rw_word;
    UART_IOCTL(handler, UART_SET_RW_WORD, &temp);

    if (_uart_dma_init(handler) < 0) {
        PRINTF("[%s] Failed to initialize UART DMA !!!\n", __func__);
        return NULL;
    }

    if (!dpm_mode_is_wakeup()) {
        if (DMA) {
            PRINTF(">>> %s : DMA Enabled ...\n", (index == UART_UNIT_2 ? "UART2" : "UART1"));
        } else {
            PRINTF(">>> %s : DMA Disabled ...\n", (index == UART_UNIT_2 ? "UART2" : "UART1"));
        }
    }

    UART_INIT(handler);

    return handler;
}

static void atcmd_UART_init(void)
{
#if defined ( __ATCMD_IF_UART2__ )
    int index = UART_UNIT_2;
#else
    int index = UART_UNIT_1;    // Default UART interface for AT-CMD
#endif //__ATCMD_IF_UART2__
    int    status;

#if defined ( __USER_UART_CONFIG__ )
    /* Initialize AT-CMD UART configuration */
    status = set_user_UART_conf(index, &ATCMD_UART_config_info, pdTRUE);
#else
    long    baud;
    int     bits;
    int     parity;
    int     stopbit;
    int     flow_ctrl;

    uart_info_t    *local_uart_info = NULL;
    int     rtm_len = 0;
    char    rtm_alloc_fail_flag = pdFALSE;

    if (dpm_mode_is_enabled() == DPM_ENABLED) {

        rtm_len = dpm_user_mem_get(ATCMD_UART_CONF_NAME, (UCHAR **)&local_uart_info);
        if (rtm_len == 0) {
            status = dpm_user_mem_alloc(ATCMD_UART_CONF_NAME,
                                            (void **)&local_uart_info,
                                            sizeof(uart_info_t),
                                            100);
            if (status != 0 /* SUCCESS */) {
                PRINTF("[%s] Failed to allocate RTM area !!!\n", __func__);
                rtm_alloc_fail_flag = pdTRUE;

                goto heap_malloc;
            }

            memset(local_uart_info, 0x00, sizeof(uart_info_t));

        } else if (rtm_len != sizeof(uart_info_t)) {
            PRINTF("[%s] Invalid RTM alloc size (%d)\n", __func__, rtm_len);
            rtm_alloc_fail_flag = pdTRUE;

            goto heap_malloc;
        }
    } else {
heap_malloc :
        local_uart_info = (uart_info_t *)pvPortMalloc(sizeof(uart_info_t));

        memset(local_uart_info, 0x00, sizeof(uart_info_t));

        if (rtm_alloc_fail_flag == pdTRUE) {
            goto with_heap_mem;
        }
    }

    if (dpm_mode_is_wakeup() == DPM_WAKEUP) {
        baud      = local_uart_info->baud;
        bits      = local_uart_info->bits;
        parity    = local_uart_info->parity;
        stopbit   = local_uart_info->stopbit;
        flow_ctrl = local_uart_info->flow_ctrl;
    } else {
with_heap_mem :
        /* Restore saved values in NVRAM */
        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_BAUDRATE, (int *)&baud) != 0) {
            baud = DEFAULT_UART_BAUD;
        }

        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_BITS, (int *)&bits) != 0) {
            bits = UART_BITPERCHAR;
        }

        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_PARITY, (int *)&parity) != 0) {
            parity = UART_PARITY;
        }

        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_STOPBIT, (int *)&stopbit) != 0) {
            stopbit = UART_STOPBIT;
        }

#if defined (__UART1_FLOW_CTRL_ON__)
        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_FLOWCTRL, (int *)&flow_ctrl) != 0) {
            flow_ctrl = UART_FLOW_CTRL;
        }
#else
        flow_ctrl = UART_FLOWCTL_OFF;
#endif        

        local_uart_info->baud      = (unsigned int)baud;
        local_uart_info->bits      = (char)bits;
        local_uart_info->parity    = (char)parity;
        local_uart_info->stopbit   = (char)stopbit;
        local_uart_info->flow_ctrl = (char)flow_ctrl;
    }

    /* Initialize AT-CMD UART configuration */
    status = set_user_UART_conf(index, local_uart_info, pdTRUE);
#endif // __USER_UART_CONFIG__
    if (status != 0) {
        PRINTF("[%s] Error to configure for AT-CMD UART !!!\n", __func__);
        return;
    }

#if defined ( __ATCMD_IF_UART2__ )
    uart2 = uart_init(index,
                      g_UART2_config_info.baud,
                      g_UART2_config_info.bits,
                      g_UART2_config_info.parity,
                      g_UART2_config_info.stopbit,
                      g_UART2_config_info.flow_ctrl);
#else    // Default UART1
    uart1 = uart_init(index,
                      g_UART1_config_info.baud,
                      g_UART1_config_info.bits,
                      g_UART1_config_info.parity,
                      g_UART1_config_info.stopbit,
                      g_UART1_config_info.flow_ctrl);
#endif //__ATCMD_IF_UART2__
}
#endif // defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )

#endif // __SUPPORT_ATCMD__

/* EOF */
