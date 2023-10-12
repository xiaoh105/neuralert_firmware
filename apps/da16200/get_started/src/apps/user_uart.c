/**
 ****************************************************************************************
 *
 * @file user_uart.c
 *
 * @brief Entry point for User written AT-CMD
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
#include "da16x_types.h"
#include "common_uart.h"

#if    defined ( __SUPPORT_UART1__ ) || defined ( __SUPPORT_UART2__ )

/*
 * For Customer's configuration for UART devices
 */
uart_info_t user_UART1_config_info = {
    UART_BAUDRATE_115200,    /* baud */
    UART_DATABITS_8,         /* bits */
    UART_PARITY_NONE,        /* parity */
    UART_STOPBITS_1,         /* stopbit */        
    UART_FLOWCTL_OFF         /* flow control */
};

uart_info_t user_UART2_config_info = {
    UART_BAUDRATE_115200,    /* baud */
    UART_DATABITS_8,         /* bits */
    UART_PARITY_NONE,        /* parity */
    UART_STOPBITS_1,         /* stopbit */        
    UART_FLOWCTL_OFF         /* flow control */
};

#endif    // __SUPPORT_UART1__ ||  __SUPPORT_UART2__

/* EOF */
