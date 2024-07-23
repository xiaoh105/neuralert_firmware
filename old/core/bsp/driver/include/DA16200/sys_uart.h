/**
 ****************************************************************************************
 *
 * @file sys_uart.h
 *
 * @brief CMSDK UART Driver
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


#ifndef __sys_uart_h__
#define __sys_uart_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

#ifdef	SUPPORT_SYSUART
//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#undef	SYS_UART_TX_INTR_SUPPORT
#define	SYS_UART_RX_INTR_SUPPORT

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

void	_sys_uart_create(void);
void	_sys_uart_init( UINT32 baud, UINT32 clock );
void	_sys_uart_close(void);
int	_sys_uart_read(char *buf, int len);
int	_sys_uart_write(char *buf, int len);
int	_sys_uart_rxempty(void);
int	_sys_uart_txempty(void);

#endif /*SUPPORT_SYSUART*/

#endif	/*__sys_uart_h__*/
