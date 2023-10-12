/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SysTick
 * \{
 * \brief 24-bit system timer that RTOS Kernel can used to switch contexts
 */

/**
 ****************************************************************************************
 *
 * @file sys_tick.h
 *
 * @brief SysTick Driver
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


#ifndef __sys_tick_h__
#define __sys_tick_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

/**
 * \brief IOCTL Commands
 */
typedef enum	{
	TICK_CMD_GETUSEC = 1,	/**< Get current raw value of register VAL  */
	TICK_CMD_GETSCALE,		/**< Get current configuration (data[0] : tick duration (msec), data[1] : clock scaler, data[2] : register LOAD) */
	TICK_CMD_SETSCALE,		/**< Set systick configuration (data[0] : tick duration (msec)) */
	TICK_CMD_MAX
} SYSTICK_IOCTL_LIST;

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

/**
 * \brief Create SysTick driver
 *
 * \param [in] msec			system clock frequency
 *
 * \note  This function is reserved for Thread context management in RTOS.
 *
 */
extern void	_sys_tick_create( UINT msec );

/**
 * \brief Initialize SysTick driver
 *
 * \param [in] msec			system clock frequency
 * \param [in] systick		function pointer to the interrupt callback
 *
 * \note  This function is reserved for Thread context management in RTOS.
 *
 */
extern void	_sys_tick_init( UINT msec , void *systick);

/**
 * \brief Close SysTick driver
 *
 * \note  This function is reserved for Thread context management in RTOS.
 *
 */
extern void	_sys_tick_close(void);

/**
 * \brief Call for device specific operations
 *
 * \param [in] cmd			IOCTL command
 * \param [inout] data		command specific data array
 *
 * \return	TRUE,  			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	_sys_tick_ioctl(int cmd, void *data);

#endif	/*__sys_tick_h__*/
/**
 * \}
 * \}
 */
