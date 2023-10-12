/**
 * \addtogroup HALayer
 * \{
 * \addtogroup CMSDK_WATCHDOG	WatchDog
 * \{
 * \brief Timer used to reset a system that hangs because of a SW/HW fault (reserved for SDK)
 */
 
/**
 ****************************************************************************************
 *
 * @file wdog.h
 *
 * @brief WatchDog Driver
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


#ifndef __WDOG_H__
#define __WDOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef		struct __wdog_device_handler__	WDOG_HANDLER_TYPE;

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
/**
 * \brief WatchDog ID
 */
typedef 	enum	__wdog_unit__ {
	WDOG_UNIT_0 = 0,	/**< CMSDK WatchDog */

	WDOG_UNIT_MAX
} WDOG_UNIT_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief IOCTL Commands
 */
typedef enum	{
	WDOG_GET_DEVREG = 1,	/**< Not used */

	WDOG_SET_ENABLE,		/**< Enable WatchDog (data[0] : mode selection, set to NMI generation if data[0] == TRUE, otherwise set to reset generation) */
	WDOG_SET_DISABLE,		/**< Disable WatchDog */
	WDOG_SET_COUNTER,		/**< Preset the polling duration (data[0] : counter) */	
	WDOG_SET_RESET,			/**< Set to reboot the system usign WatchDog interrupt (enable WatchDog Reset if data[0] == NULL)  */
	WDOG_SET_CALLACK,		/**< Set an interrupt callback for monitoring (data[0] : index '0', data[1], callback function, data[2] : callback parameter) */	
	WDOG_SET_ABORT,			/**< Not used */

	WDOG_GET_STATUS,		/**< Get the current status of WatchDog */	

	WDOG_SET_ON,
	WDOG_SET_OFF,

	WDOG_SET_MAX
} WDOG_DEVICE_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//

#define INT_MAX_WDOG_VEC		1

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------
/**
 * \brief Structure of WatchDog Driver
 */
struct	__wdog_device_handler__
{
	UINT32			dev_addr;	/**< Physical address for identification */

	// Device-dependant
	UINT32			dev_unit;	/**< WatchDog ID */
	UINT32			instance;	/**< Number of times that a driver with the same ID is created */
	UINT32			setvalue;	/**< Polling duration */
	UINT32			force;		/**< forcing variables for watchdog reset */
	UINT32			wdogbreak;	/**< Not used */
	void			*clkmgmt;	/**< PLL callback structure */
	ISR_CALLBACK_TYPE	callback[INT_MAX_WDOG_VEC] ;	/**< function pointer of interrupt callback */
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create WatchDog driver
 *
 * \param [in] dev_type		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 * \note  This function is used for System Monitoring (function system_schedule)
 *
 */
extern HANDLE	WDOG_CREATE( UINT32 dev_type );

/**
 * \brief Initialize WatchDog driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	WDOG_INIT (HANDLE handler);

/**
 * \brief Call for device specific operations
 *
 * \param [in] handler		driver handler
 * \param [in] cmd			IOCTL command
 * \param [inout] data		command specific data array
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	WDOG_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief Read current raw value of VALUE Register
 *
 * \param [in] handler		driver handler
 * \param [in] addr			deprecated parameter
 * \param [out] p_data		data pointer to store return value (VALUE)
 * \param [in] p_dlen		data size (sizeof(UINT32))
 *
 * \return	sizeof(UINT32),	function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	WDOG_READ(HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen);

/**
 * \brief Close WatchDog driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int 	WDOG_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	DRIVER :: Utility
//---------------------------------------------------------


#endif  // __WDOG_H__
/**
 * \}
 * \}
 */
