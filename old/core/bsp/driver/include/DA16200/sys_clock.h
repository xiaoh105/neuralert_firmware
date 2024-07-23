/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SysClock
 * \{
 * \brief Driver to support a PLL setup and clock divider configuration.
 */

/**
 ****************************************************************************************
 *
 * @file sys_clock.h
 *
 * @brief Clock Management
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


#ifndef __sys_clock_h__
#define __sys_clock_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

//--------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------

#define	CLKMGMT_SRCMSK		0xC0000000
#define	CLKMGMT_FREQMSK		0x3FFFFFFF	/* Max 1.073 GHz */

#define	CLKMGMT_PRI_URGENT	0xFC905547

//--------------------------------------------------------------------
//	Callback Info
//--------------------------------------------------------------------
/**
 * \brief PLL Callback Structure
 * PLL & Clock Callback
 */
typedef	struct	__clock_list__
{
	struct	__clock_list__	*next;	/**< linked list pointer */
	CLOCK_CALLBACK_TYPE	callback ;	/**< Function pointer and paramters */
} CLOCK_LIST_TYPE;


/**
 * \brief IOCTL Commands
 */
typedef enum	{
	SYSCLK_SET_XTAL = 0,	/**< Set XTAL Frequency (Hz) */
	SYSCLK_SET_CALLACK ,	/**< Add a PLL callback structure */
	SYSCLK_RESET_CALLACK,	/**< Delete a PLL callback structure */

	SYSCLK_PRINT_INFO,		/**< Print a list of registered callbacks */
	SYSCLK_CMD_MAX
} SYSCLK_IOCTL_LIST;

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

/**
 * \brief Create SysClock driver
 *
 */
extern void	_sys_clock_create(void);

/**
 * \brief Initialize SysClock driver
 *
 * \param [in] clock 	XTAL Frequency
 *
 */
extern void	_sys_clock_init(UINT32 clock);

/**
 * \brief Close SysClock driver
 *
 */
extern void	_sys_clock_close(void);

/**
 * \brief Call for device specific operations
 *
 * \param [in] 		cmd		IOCTL command
 * \param [inout] 	data	command specific data array
 *
 * \return	TRUE,  			function succeeded
 * \return	FALSE, 			function failed
 *
 */
extern int	_sys_clock_ioctl(int cmd, void *data);

/**
 * \brief Read the system clock freqency
 *
 * \param [out] 	clock	data pointer to store current value of system clock
 * \param [in]		len		size of data (sizeof(UINT32))
 *
 * \return	sizeof(UINT32),  function succeeded
 * \return	FALSE, 			function failed
 *
 */
extern int	_sys_clock_read(UINT32 *clock, int len);

/**
 * \brief Change the system clock frequency
 *
 * \param [in] 		clock	new value of system clock
 * \param [in]		len		size of data (sizeof(UINT32))
 *
 * \return	sizeof(UINT32),  function succeeded
 * \return	FALSE, 			function failed
 *
 */
extern int	_sys_clock_write(UINT32 *clock, int len);

#endif	/*__sys_clock_h__*/
/**
 * \}
 * \}
 */
