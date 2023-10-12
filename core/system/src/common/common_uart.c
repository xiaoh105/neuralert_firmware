/**
 ****************************************************************************************
 *
 * @file common_uart.c
 *
 * @brief Host interface drviver module for UART
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

#include "da16x_system.h"
#include "common_uart.h"
#include "console.h"

/*************************************************
 * Default UART configuration for DA16200
 ************************************************/
uart_info_t UART_config_default = {
    UART_BAUDRATE_115200,    /* baud */
    UART_DATABITS_8,         /* bits */
    UART_PARITY_NONE,        /* parity */
    UART_STOPBITS_1,         /* stopbit */
    UART_FLOWCTL_OFF         /* flow control */
};


#if defined (__SUPPORT_UART1__) || defined (__SUPPORT_UART2__) || defined (__SUPPORT_ATCMD__)

#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"


/**************************************************************************
 * External functions
 **************************************************************************/
extern void PRINTF_UART1(const char *fmt,...);
extern void PUTS_UART1(char *data, int data_len);
extern int  getchar_UART(int uart_idx, unsigned long mode);
extern int chk_dpm_wakeup(void);


/**************************************************************************
 * External global variables
 **************************************************************************/


/**************************************************************************
 * Global Variables
 **************************************************************************/
HANDLE    uart1 = NULL;
HANDLE    uart2 = NULL;

int    _uart1_echo = 0;
int    _uart2_echo = 0;

uart_info_t g_UART1_config_info = { 0, };
uart_info_t g_UART2_config_info = { 0, };


/*
 * For UART1 initializing /////////////////////////////////////////
 */

static const uart_dma_primitive_type uart_dma_primitives = {
    &SYS_DMA_COPY,
    &SYS_DMA_REGISTRY_CALLBACK,
    &SYS_DMA_RELEASE_CHANNEL
};

static int _uart_dma_init(HANDLE handler)
{
    UINT32    dma_control;
    UINT32    fifo_level;
    UINT32    dev_unit;
    UINT32    rw_word;
    UINT32    use_word_access;
    HANDLE    dma_channel_tx;
    HANDLE    dma_channel_rx;

#define GET_RX_DMA_CHANNEL(x)    ( \
                                 (x == UART_UNIT_0) ? CH_DMA_PERI_UART0_RX :        \
                                 (x == UART_UNIT_1) ? CH_DMA_PERI_UART1_RX :        \
                                 (x == UART_UNIT_2) ? CH_DMA_PERI_UART2_RX : 10     \
                                )

#define GET_TX_DMA_CHANNEL(x)    ( \
                                 (x == UART_UNIT_0) ? CH_DMA_PERI_UART0_TX :        \
                                 (x == UART_UNIT_1) ? CH_DMA_PERI_UART1_TX :        \
                                 (x == UART_UNIT_2) ? CH_DMA_PERI_UART2_TX : 10     \
                                )

    UART_IOCTL(handler, UART_GET_FIFO_INT_LEVEL, &fifo_level);
    UART_IOCTL(handler, UART_GET_PORT, &dev_unit);
    UART_IOCTL(handler, UART_GET_RW_WORD, &rw_word);
    UART_IOCTL(handler, UART_GET_WORD_ACCESS, &use_word_access);

    /* RX */
    if (use_word_access) {
        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
    } else {
        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_BYTE);
        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
    }

    if (((fifo_level) >> 3 ) > 3) {
        PRINTF("[%s] Not support DMA burst fifo len !!!\n", __func__);
        return -1;
    }

    if (use_word_access) {
        dma_control |= DMA_CHCTRL_BURSTLENGTH(fifo_level >> 3);
    } else {
        dma_control |= DMA_CHCTRL_BURSTLENGTH((fifo_level >> 3) + 2);
    }

    dma_channel_rx = SYS_DMA_OBTAIN_CHANNEL(GET_RX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
    if (dma_channel_rx == NULL) {
        PRINTF("[%s] DMA Rx null\n", __func__);
        return -1;
    }

    /* TX */
    if (use_word_access) {
        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
    } else {
        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_BYTE) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
    }

    dma_channel_tx = SYS_DMA_OBTAIN_CHANNEL(GET_TX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
    if (dma_channel_tx == NULL) {
        PRINTF("[%s] DMA Tx null ...\n", __func__);
        return -1;
    }

    UART_DMA_INIT(handler, dma_channel_tx, dma_channel_rx, &uart_dma_primitives);

    return 0;
}

HANDLE _uart1_init(int _baud, int _bits, int _parity, int _stopbit, int _flow_ctrl)
{
    HANDLE    handler;

    UINT32    clock = 40000000 * 2;
    UINT32    parity_en = 0;
    UINT32    fifo_en = 1;
    UINT32    DMA = 0;
    UINT32    swflow = 0;
    UINT32    word_access = 0;
    UINT32    rw_word = 0;

    UINT32    baud = (UINT32)_baud;
    UINT32    bits = (UINT32)_bits;
    UINT32    parity = (UINT32)_parity;
    UINT32    stopbit = (UINT32)_stopbit;
    UINT32    flow_ctrl = (UINT32)_flow_ctrl;

    UINT32    temp;

    handler = UART_CREATE(UART_UNIT_1);

    /* Configure uart1 */
    UART_IOCTL(handler, UART_SET_CLOCK, &clock);
    UART_IOCTL(handler, UART_SET_BAUDRATE, &baud);

    if (!chk_dpm_wakeup()) {
        PRINTF("\n>>> UART1 : Clock=%d, BaudRate=%d\n", clock, baud);
    }

    temp =   UART_WORD_LENGTH(bits)
           | UART_FIFO_ENABLE(fifo_en)
           | UART_PARITY_ENABLE(parity_en)
           | UART_EVEN_PARITY(parity)
           | UART_STOP_BIT(stopbit);
    UART_IOCTL(handler, UART_SET_LINECTRL, &temp);

    temp =   UART_RECEIVE_ENABLE(1)
           | UART_TRANSMIT_ENABLE(1)
           | UART_HWFLOW_ENABLE(flow_ctrl);
    UART_IOCTL(handler, UART_SET_CONTROL, &temp);

    temp =   UART_RX_INT_LEVEL(UART_ONEEIGHTH)
           | UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
    UART_IOCTL(handler, UART_SET_FIFO_INT_LEVEL, &temp);

    temp = DMA;
    UART_IOCTL(handler, UART_SET_USE_DMA, &temp);

    /* FIFO Enable */
    temp =   UART_INTBIT_RECEIVE
           | UART_INTBIT_TRANSMIT
           | UART_INTBIT_TIMEOUT
           | UART_INTBIT_ERROR
           | UART_INTBIT_FRAME
           | UART_INTBIT_PARITY
           | UART_INTBIT_BREAK
           | UART_INTBIT_OVERRUN;
    UART_IOCTL(handler, UART_SET_INT, &temp);

    temp = swflow;
    UART_IOCTL(handler, UART_SET_SW_FLOW_CONTROL, &temp);

    temp = word_access;
    UART_IOCTL(handler, UART_SET_WORD_ACCESS, &temp);

    temp = rw_word;
    UART_IOCTL(handler, UART_SET_RW_WORD, &temp);

    if (_uart_dma_init(handler) < 0) {
        PRINTF("[%s] Failed to initialize UART1 DMA !!!\n", __func__);
        return NULL;
    }
    if (!chk_dpm_wakeup()) {
        PRINTF(">>> UART1 : DMA Enabled ...\n");
    }

    UART_INIT(handler);

    return handler;
}

void host_uart_init(void)    /* move from command_host.c */
{
    UINT32    baud;
    UINT32    bits;
    UINT32    parity;
    UINT32    stopbit;
    UINT32    flow_ctrl;

    uart_info_t    *uart1_info = NULL;
    int    rtm_len = 0;
    char    rtm_alloc_fail_flag = pdFALSE;

    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        UINT    status;

        rtm_len = dpm_user_mem_get(UART_CONF_NAME, (UCHAR **)&uart1_info);
        if (rtm_len == 0) {
            status = dpm_user_mem_alloc(UART_CONF_NAME, (VOID **)&uart1_info, sizeof(uart_info_t), 100);
            if (status != 0 /* SUCCESS */) {
                PRINTF("[%s] Failed to allocate RTM area !!!\n", __func__);
                rtm_alloc_fail_flag = pdTRUE;

                goto heap_malloc;
            }

            memset(uart1_info, 0x00, sizeof(uart_info_t));

        } else if (rtm_len != sizeof(uart_info_t)) {
            PRINTF("[%s] Invalid RTM alloc size (%d)\n", __func__, rtm_len);
            rtm_alloc_fail_flag = pdTRUE;

            goto heap_malloc;
        }
    } else {
heap_malloc :
        uart1_info = (uart_info_t *)pvPortMalloc(sizeof(uart_info_t));

        memset(uart1_info, 0x00, sizeof(uart_info_t));

        if (rtm_alloc_fail_flag == pdTRUE) {
            goto with_heap_mem;
        }
    }

    if (dpm_mode_is_wakeup() == DPM_WAKEUP) {
        baud        = uart1_info->baud;
        bits        = uart1_info->bits;
        parity      = uart1_info->parity;
        stopbit     = uart1_info->stopbit;
        flow_ctrl   = uart1_info->flow_ctrl;
    } else {
with_heap_mem :
        /* Restore saved values in NVRAM */
        if (read_nvram_int((const char *)NVR_KEY_UART1_BAUDRATE, (int *)&baud) != 0) {
            baud = DEFAULT_UART_BAUD;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_BITS, (int *)&bits) != 0) {
            bits = UART_BITPERCHAR;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_PARITY, (int *)&parity) != 0) {
            parity = UART_PARITY;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_STOPBIT, (int *)&stopbit) != 0) {
            stopbit = UART_STOPBIT;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_FLOWCTRL, (int *)&flow_ctrl) != 0) {
            flow_ctrl = UART_FLOW_CTRL;
        }

        uart1_info->baud      = baud;
        uart1_info->bits      = bits;
        uart1_info->parity    = parity;
        uart1_info->stopbit   = stopbit;
        uart1_info->flow_ctrl = flow_ctrl;
    }

    uart1 = _uart1_init(uart1_info->baud,
                        uart1_info->bits,
                        uart1_info->parity,
                        uart1_info->stopbit,
                        uart1_info->flow_ctrl);

}

uart_info_t  uart1_info;
uart_info_t  *uart1_info_rtm = NULL;
void host_uart_init_pre_usr_rtm(void)
{
    int    baud;
    int    bits;
    int    parity;
    int    stopbit;
    int    flow_ctrl;

    if (dpm_mode_is_wakeup() == TRUE) {
        int rtm_len = 0;

        // load uart1_info from rtm
        rtm_len = dpm_user_mem_get(UART_CONF_NAME, (UCHAR **)&uart1_info_rtm);
        if (rtm_len == 0) {
            uart1_info_rtm = NULL;
            goto UART_READ_FROM_NVRAM;
        } else if (rtm_len != sizeof(uart_info_t)) {
            PRINTF("[%s] Invalid RTM alloc size (%d)\n", __func__, rtm_len);
            uart1_info_rtm = NULL;
            goto UART_READ_FROM_NVRAM;
        }

        // rtm loading success
        memcpy(&uart1_info, uart1_info_rtm, sizeof(uart_info_t));
    } else {
UART_READ_FROM_NVRAM:
        // dpm (x)
        // dpm (o) && wakeup (x)

        /* Read in values from NVRAM */
        if (read_nvram_int((const char *)NVR_KEY_UART1_BAUDRATE, &baud) != 0) {
            uart1_info.baud = DEFAULT_UART_BAUD;
        } else {
            uart1_info.baud = (unsigned int)baud;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_BITS, &bits) != 0) {
            uart1_info.bits = UART_BITPERCHAR;
        } else {
            uart1_info.bits = (char)bits;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_PARITY, &parity) != 0) {
            uart1_info.parity = UART_PARITY;
        } else {
            uart1_info.parity = (char)parity;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_STOPBIT, &stopbit) != 0) {
            uart1_info.stopbit = UART_STOPBIT;
        } else {
            uart1_info.stopbit = (char)stopbit;
        }

        if (read_nvram_int((const char *)NVR_KEY_UART1_FLOWCTRL, &flow_ctrl) != 0) {
            uart1_info.flow_ctrl = UART_FLOW_CTRL;            
        } else {
            uart1_info.flow_ctrl = (char)flow_ctrl;
        }
    }

    uart1 = _uart1_init(uart1_info.baud,
                        uart1_info.bits,
                        uart1_info.parity,
                        uart1_info.stopbit,
                        uart1_info.flow_ctrl);

}

void host_uart_init_post_usr_rtm(void)
{
    UINT    status;

    if (dpm_mode_is_enabled() == TRUE && uart1_info_rtm == NULL) {
        // allocate user rtm
        status = dpm_user_mem_alloc(UART_CONF_NAME, (VOID **)&uart1_info_rtm, sizeof(uart_info_t), 100);

        if (status != 0 /* SUCCESS */) {
            PRINTF("[%s] Failed to allocate RTM area !!!\n", __func__);
            return;
        }

        // copy uart1_info to uart1_info_rtm
        memcpy(uart1_info_rtm, &uart1_info, sizeof(uart_info_t));
    }
}

void host_uart_set_baudrate_control(UINT32 baudrate, UINT32 set)
{
    UINT32  temp;

    UART_IOCTL(uart1, UART_SET_BAUDRATE, &baudrate);

    temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(set);
    UART_IOCTL(uart1, UART_SET_CONTROL, &temp);

    UART_CHANGE_BAUERATE(uart1, baudrate);
}

/////////////////////////////////////////////////////////////////////
static int user_uart_is_idx_valid(int uart_idx)
{
    int result = pdTRUE;

    if (uart_idx < 1 || uart_idx > 2) {
        PRINTF("[%s] ERR: uart idx : out of bound \n", __func__);
        return pdFALSE;
    }

    if (   (atcmd_on_uart1_flag == pdTRUE && uart_idx == 1)
        || (atcmd_on_uart2_flag == pdTRUE && uart_idx == 2)) {
        PRINTF("[%s] ERR: same uart as AT-CMD \n", __func__);
        return pdFALSE;
    }

    return result;
}

int is_UART_rx_empty(int uart_idx)
{
    HANDLE    __uart = NULL;
    int rx_empty;

    if (user_uart_is_idx_valid(uart_idx) == pdFALSE) {
        return FALSE;
    }

    switch (uart_idx) {
    case UART_UNIT_1 :
        __uart = uart1;
        break;

    case UART_UNIT_2 :
        __uart = uart2;
        break;
    }

    UART_IOCTL(__uart, UART_CHECK_RXEMPTY, &rx_empty);

    return rx_empty;
}

int getchar_UART(int uart_idx, unsigned long mode)
{
    int        ch, ret;
    int        temp ;
    HANDLE    __uart = NULL;
    int        uart_echo = 0;

    switch (uart_idx) {
    case UART_UNIT_1 :
        __uart = uart1;
        uart_echo = _uart1_echo;
        break;

    case UART_UNIT_2 :
        __uart = uart2;
        uart_echo = _uart2_echo;
        break;
    }

    if (mode == portNO_DELAY) {
        temp = FALSE;
        ch = '\0';

        UART_IOCTL(__uart, UART_SET_RX_SUSPEND, &temp);
        ret = UART_READ(__uart, &ch, sizeof(char));

        temp = TRUE;
        UART_IOCTL(__uart, UART_SET_RX_SUSPEND, &temp);

        if (ret > 0) {
            return (int)ch;
        } else {
            return (int)-1;
        }
    } else {
        ch = '\0';

        ret = UART_READ(__uart, &ch, sizeof(char));
        if (ret > 0) {
            if (uart_echo == 1) {
                UART_WRITE(__uart, &ch, sizeof(char));
            }

            return (int)ch;
        } else {
            return (int)-1;
        }
    }
}

void PRINTF_UART(int uart_idx, const char *fmt,...)
{
    int    len;
    va_list ap;
    char    *uart_print_buf;
    HANDLE __uart = NULL;

    switch (uart_idx) {
    case 1:
        __uart = uart1;
        break;
    case 2:
        __uart = uart2;
        break;
    }

    uart_print_buf = pvPortMalloc(UART_PRINT_BUF_SIZE);
    if (uart_print_buf == NULL) {
        PRINTF("- [%s] Failed to alloc print buf\n", __func__);
        return;
    }

    va_start(ap, fmt);
    len = da16x_vsnprintf(uart_print_buf, UART_PRINT_BUF_SIZE, 0, fmt, ap);
    va_end(ap);

    UART_WRITE(__uart, uart_print_buf, len);

    vPortFree(uart_print_buf);
}


void puts_UART(int uart_idx, char *data, int data_len)
{
    HANDLE __uart = NULL;

    switch (uart_idx) {
    case 1:
        __uart = uart1;
        break;
    case 2:
        __uart = uart2;
        break;
    }

    UART_WRITE(__uart, data, data_len);
}


/*...........................................................*/

int getchar_uart1(UINT32 mode)
{
    int text;

    text = getchar_UART(UART_UNIT_1, mode);

    return (int)text;
}

void PRINTF_UART1(const char *fmt,...)
{
    int    len;
    va_list ap;
    char    *uart1_print_buf;

    uart1_print_buf = pvPortMalloc(UART_PRINT_BUF_SIZE);
    if (uart1_print_buf == NULL) {
        PRINTF("- [%s] Failed to alloc print buf\n", __func__);
        return;
    }

    va_start(ap, fmt);
    len = da16x_vsnprintf(uart1_print_buf, UART_PRINT_BUF_SIZE, 0, fmt, ap);
    va_end(ap);

    UART_WRITE(uart1, uart1_print_buf, len);

    vPortFree(uart1_print_buf);
}

void PUTS_UART1(char *data, int data_len)
{
    puts_UART(UART_UNIT_1, data, data_len);
}

/////////////////////////////////////////////////////////////////////

int set_user_UART_conf(int uart_idx, uart_info_t *uart_conf_info, char atcmd_flag)
{
    uart_info_t    *local_UART_conf;
    int result = 0;

    local_UART_conf = uart_conf_info;

    if (atcmd_flag == TRUE) {
        if (atcmd_on_uart1_flag == pdTRUE) {
            memcpy(&g_UART1_config_info, local_UART_conf, sizeof(uart_info_t));
        } else if (atcmd_on_uart2_flag == pdTRUE) {
            memcpy(&g_UART2_config_info, local_UART_conf, sizeof(uart_info_t));
        } else {
            PRINTF("[%s] ERR: Undefined UART interface !!!\n", __func__);
            return 1; /* NOT_DONE */
        }
    } else {
        // configure user uart ...

        if (user_uart_is_idx_valid(uart_idx) == pdFALSE) {
            return 1; /* NOT_DONE */
        }

        switch (uart_idx) {
        case 1:
            memcpy(&g_UART1_config_info, local_UART_conf, sizeof(uart_info_t));
            break;
        case 2:
            memcpy(&g_UART2_config_info, local_UART_conf, sizeof(uart_info_t));
            break;
        }
    }

    return result;
}

int UART_init(int uart_idx)
{
    HANDLE    handler = NULL;

    unsigned long    clock = 40000000 * 2;
    char    parity_en = FALSE;
    char    fifo_en = TRUE;
    char    DMA = FALSE;
    char    swflow = FALSE;
    char    word_access = FALSE;
    char    rw_word = FALSE;

    UINT32 baud, bits, parity, stopbit, flow_ctrl;
    uart_info_t    *uart_config = NULL;

    if (user_uart_is_idx_valid(uart_idx) == pdFALSE) {
        return 1;/* NOT_DONE */
    }

    if (uart_idx == UART_UNIT_1) {
        uart_config = &g_UART1_config_info;
    } else if (uart_idx == UART_UNIT_2) {
        uart_config = &g_UART2_config_info;
    }

    baud = (unsigned int)(uart_config->baud);
    bits = (char)(uart_config->bits);
    switch(uart_config->parity) {
    case UART_PARITY_ODD:
        parity_en = 1;
        parity = 0;
        break;

    case UART_PARITY_EVEN:
        parity_en = 1;
        parity = 1;
        break;

    case UART_PARITY_NONE:
    default:
        parity_en = 0;
        parity = 0;
        break;
    }
    stopbit = (char)(uart_config->stopbit);
    flow_ctrl = (char)(uart_config->flow_ctrl);

    unsigned long    temp;

    switch (uart_idx) {
    case UART_UNIT_1 :
        handler = UART_CREATE(UART_UNIT_1);
        break;

    case UART_UNIT_2 :
        DA16X_CLOCK_SCGATE->Off_CAPB_UART2 = 0;
        handler = UART_CREATE(UART_UNIT_2);
        break;
    }

    /* Configure uart1 */
    UART_IOCTL(handler, UART_SET_CLOCK, &clock);
    UART_IOCTL(handler, UART_SET_BAUDRATE, &baud);

    if (dpm_mode_is_wakeup() == NORMAL_BOOT) {
        PRINTF("\n>>> UART%d : Clock=%d, BaudRate=%d\n", uart_idx, clock, baud);
    }

    temp =   UART_WORD_LENGTH(bits)
           | UART_FIFO_ENABLE(fifo_en)
           | UART_PARITY_ENABLE(parity_en)
           | UART_EVEN_PARITY(parity)
           | UART_STOP_BIT(stopbit);
    UART_IOCTL(handler, UART_SET_LINECTRL, &temp);

    temp =   UART_RECEIVE_ENABLE(1)
           | UART_TRANSMIT_ENABLE(1)
           | UART_HWFLOW_ENABLE(flow_ctrl);
    UART_IOCTL(handler, UART_SET_CONTROL, &temp);

    temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) | UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
    UART_IOCTL(handler, UART_SET_FIFO_INT_LEVEL, &temp);

    temp = DMA;
    UART_IOCTL(handler, UART_SET_USE_DMA, &temp);

    /* FIFO Enable */
    temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT
           | UART_INTBIT_TIMEOUT | UART_INTBIT_ERROR
           | UART_INTBIT_FRAME | UART_INTBIT_PARITY
           | UART_INTBIT_BREAK | UART_INTBIT_OVERRUN;

    UART_IOCTL(handler, UART_SET_INT, &temp);

    temp = swflow;
    UART_IOCTL(handler, UART_SET_SW_FLOW_CONTROL, &temp);

    temp = word_access;
    UART_IOCTL(handler, UART_SET_WORD_ACCESS, &temp);

    temp = rw_word;
    UART_IOCTL(handler, UART_SET_RW_WORD, &temp);

    if (_uart_dma_init(handler) < 0) {
        PRINTF("[%s] Failed to initialize UART DMA !!!\n", __func__);
        return 1; /* NOT_DONE */
    }

    if (dpm_mode_is_wakeup() == NORMAL_BOOT) {
        PRINTF(">>> UART%d : DMA Enabled ...\n", uart_idx);
    }

    if (UART_INIT(handler) == TRUE) {
        switch (uart_idx) {
        case 1:
            uart1 = handler;
            break;

        case 2:
            uart2 = handler;
            break;
        }
        return 0; /* SUCCESS */
    }

    return 1; /* NOT_DONE */
}

int UART_deinit(int uart_idx)
{
    HANDLE __uart = NULL;

    if (user_uart_is_idx_valid(uart_idx) == pdFALSE) {
        return 1;/* NOT_DONE */
    }

    switch (uart_idx) {
    case 1:
        __uart = uart1;
        break;

    case 2:
        __uart = uart2;
        break;
    }

    return UART_CLOSE(__uart);
}

HANDLE get_UART_handler(int uart_idx)
{
    HANDLE __uart = NULL;

    if (user_uart_is_idx_valid(uart_idx) == pdFALSE) {
        return NULL;
    }

    switch (uart_idx) {
    case 1:
        __uart = uart1;
        break;

    case 2:
        __uart = uart2;
        break;
    }

    return __uart;
}
#endif // __SUPPORT_UART1__ || __SUPPORT_UART2__ || __SUPPORT_ATCMD__

/* EOF */
