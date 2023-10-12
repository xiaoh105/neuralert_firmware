/**
 ****************************************************************************************
 *
 * @file uart_sample.c
 *
 * @brief Sample code for UART1
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

#if defined (__PERIPHERAL_SAMPLE_UART__)
#include "da16x_system.h"

#include "da16200_ioconfig.h"
#include "user_dpm_api.h"

#include "common_uart.h"

#define	USER_DELIMITER_0	'\0'
#define	USER_DELIMITER_1	'\n'
#define	USER_DELIMITER_2	'\r'

#define	USER_UART1_BUF_SZ	1024

#undef	DO_CHECK_RXEMPTY

/* External Variables/Functions */
extern void	hex_dump(UCHAR *data, UINT length);

/* External global variables */


/* Internal Variables/Functions */


/* Local static variables */
static int	sample_uart_idx = UART_UNIT_1;
static int	echo_enable = FALSE;


/*
 * For Customer's configuration for UART devices
 *
 * "user_UART_config_info" data is located in /SDK/customer/src/user_uart.c.
 *
 * This data is temporay for sample application.
 */
static uart_info_t sample_UART_config_info = {
    UART_BAUDRATE_115200,	/* baud */
    UART_DATABITS_8,		/* bits */
    UART_PARITY_NONE,		/* parity */
    UART_STOPBITS_1,		/* stopbit */
    UART_FLOWCTL_OFF		/* flow control */
};


/*
 * Receive data from UART1
 */
static int get_data_from_uart1(char *buf)
{
    int		ch = 0;
    int		i = 0;

    while (1) {
        /* Get on byte from uart1 com port */
        ch = getchar_UART(sample_uart_idx, portMAX_DELAY);

        if (ch < 0) {
            PRINTF("UART error occur\n");
            vTaskDelay(1);
            continue;
        }

        if (echo_enable == TRUE) {
            puts_UART(sample_uart_idx, (char *)&ch, sizeof(char)); // echo
        }

        /* check data length */
        if (i >= (USER_UART1_BUF_SZ - 1)) {
            i = USER_UART1_BUF_SZ - 2;
        }

        if (ch == USER_DELIMITER_1 || ch == USER_DELIMITER_2) {
            buf[i] = USER_DELIMITER_0;
            break;
        } else {
            buf[i++] = ch;
        }
    }
    return (i);
}

static void uart1_sample(void)
{
    int	 	rx_len;
    char	*init_str = "- Start UART1 communicate module ...\r\n";
    char	*rx_buf = NULL;
    char	*tx_buf = "\r\n- Data receiving OK...\r\n";
    int	tx_len;

    /* Print-out test string to console and to UART1 device */
    PRINTF((const char *)init_str);	// For Console
    puts_UART(sample_uart_idx, init_str, strlen((const char *)init_str));	// for UART1

    echo_enable = TRUE;
    rx_buf = pvPortMalloc(USER_UART1_BUF_SZ);

    while (1) {
        memset(rx_buf, 0, USER_UART1_BUF_SZ);

#ifdef DO_CHECK_RXEMPTY
        /* This code is just a example of 'check rx empty' function */
        if (is_UART_rx_empty(sample_uart_idx) == TRUE ) {
            PRINTF("UART_RX empty\n");
            vTaskDelay(10);
            continue;
        }
#endif

        /* Get on byte from uart1 comm port */
        rx_len = get_data_from_uart1(rx_buf);

        if (rx_len > 0) {	// Rx data exist ...
            /* Debug */
            hex_dump((UCHAR *)rx_buf, rx_len);
            PRINTF("\n");

            /* Send response to UART1 (Implement UART parser here) */
            tx_len = strlen((char *)tx_buf);

            /* Send text string to UART1 */
            puts_UART(sample_uart_idx, tx_buf, tx_len);
        }
    }
}
#ifdef UART_ERR_CALLBACK
void	UART_parity_err_callback(HANDLE hdl)
{
    /* The PRINTF function cannot be used under ISR */
    Printf("parity error occur \n");
}

void	UART_overrun_err_callback(HANDLE hdl)
{
    /* The PRINTF function cannot be used under ISR */
    Printf("overrun error occur \n");
}

void 	UART_err_int_callback(HANDLE hdl)
{
    /* The PRINTF function cannot be used under ISR */
    Printf("UART error occur \n");
}

void 	UART_frame_err_callback(HANDLE hdl)
{
    /* The PRINTF function cannot be used under ISR */
    Printf("frame error occur \n");
}

void 	UART_break_err_callback(HANDLE hdl)
{
    /* The PRINTF function cannot be used under ISR */
    Printf("break error occur \n");
}
#endif

void run_uart1_sample(UINT32 arg)
{
    DA16X_UNUSED_ARG(arg);

    int	status;

    /* pinmux config for UART1 - GPIOA[5:4] */
    _da16x_io_pinmux(PIN_CMUX, CMUX_UART1d);

    /*
     *
     ****************************************************************************************
     * @brief  Set UART configuration
     * @param[in] uart_idx          Index value of UART interface (UART_UNIT_1, UART_UNIT_2)
     * @param[in] *uart_conf_info   Buffer pointer of configuration data
     * @param[in] atcmd_flag        AT-CMD flag
     * @return TX_SUCCESS if succeed to process, others else
     ****************************************************************************************
     *
     * int set_user_UART_conf(int uart_idx, uart_info_t *uart_conf_info, char atcmd_flag)
     */
    status = set_user_UART_conf(UART_UNIT_1, &sample_UART_config_info, FALSE);
    if (status != 0) {
        PRINTF("[%S] Error to configure for UART1 !!!\n", __func__);
        return;
    }


    /**
     ****************************************************************************************
     * @brief  Initialize UART device
     * @param[in] uart_idx          Index value of UART interface (UART_UNIT_1, UART_UNIT_2)
     * @return TX_SUCCESS if succeed to process, others else
     ****************************************************************************************
     *
     * int UART_init(int uart_idx);
     */
    status = UART_init(sample_uart_idx);
    if (status != 0) {
        PRINTF("[%S] Error to initialize UART1 with sample_UART_config !!!\n", __func__);
        return;
    }


    /**
     ****************************************************************************************
     * @brief  Register interrupt callback function.
     ****************************************************************************************
     */
#ifdef UART_ERR_CALLBACK
    UART_IOCTL(get_UART_handler(sample_uart_idx), UART_SET_OVERRUN_INT_CALLBACK, (void *)UART_overrun_err_callback);

    /* Below error interrupts can be tested under parity enabled. */
    UART_IOCTL(get_UART_handler(sample_uart_idx), UART_SET_PARITY_INT_CALLBACK, (void *)UART_parity_err_callback);
    UART_IOCTL(get_UART_handler(sample_uart_idx), UART_SET_ERR_INT_CALLBACK, (void *)UART_err_int_callback);
    UART_IOCTL(get_UART_handler(sample_uart_idx), UART_SET_FRAME_INT_CALLBACK, (void *)UART_frame_err_callback);
    UART_IOCTL(get_UART_handler(sample_uart_idx), UART_SET_BREAK_INT_CALLBACK, (void *)UART_break_err_callback);
#endif

    /**
     ****************************************************************************************
     * @brief  Flushing UART_RX queue. After UART_INIT() is called, it try to receive UART_RX data even if UART_READ() does not call.
     *         If the user don't want to use the data before UART_READ(), it need to flush the UART_RX data.
     ****************************************************************************************
     */
#ifdef UART_DATA_FLUSH
    UART_FLUSH(get_UART_handler(sample_uart_idx));
#endif

    /* Start UART monitor */
    uart1_sample();
}

#endif
