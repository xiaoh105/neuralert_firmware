/**
 * \addtogroup HALayer
 * \{
 * \addtogroup CMSDK_TIMER	SimpleTimer
 * \{
 * \brief 32-bit down count timer with perioic mode. (reserved for SDK)
 */
 
/**
 ****************************************************************************************
 *
 * @file simpletimer.h
 *
 * @brief SimpleTimer Driver
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


#ifndef __STIMER_H__
#define __STIMER_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef 	struct __stimer_device_handler__	 STIMER_HANDLER_TYPE;

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
/**
 * \brief STIMER ID
 */
typedef 	enum	__stimer_unit__ {
	STIMER_UNIT_0 = 0,	/**< Reserved for RTC Manager */
	STIMER_UNIT_1,		/**< Reserved for USLEEP */
	STIMER_UNIT_MAX		
} STIMER_UNIT_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief IOCTL Commands
 */
typedef enum	{
	STIMER_GET_DEVREG = 1,		/**< Not used */

	STIMER_GET_TICK,			/**< Get Number of Tick interrupts (data[0] : return number) */
	STIMER_SET_MODE,			/**< Set Control Register (data[0] : value) */
	STIMER_GET_MODE,			/**< Get current value of Control Register (data[0] : return value) */
	STIMER_SET_LOAD,			/**< Set timer duration (data[0] : target frequency, data[1] : divider) */
	STIMER_SET_UTIME,			/**< Set timer duration (data[0] : target frequency, data[1] : microseconds) */
	STIMER_GET_UTIME,			/**< Get current value of Counter Register (VALUE) (data[0] : return value) */
	STIMER_SET_ACTIVE,			/**< Active the timer */
	STIMER_SET_DEACTIVE,		/**< Deactive the timer */

	STIMER_SET_CALLACK,			/**< Set interrupt callback (data[0] : index '0', data[1], callback function, data[2] : callback parameter) */

	STIMER_SET_MAX
} STIMER_DEVICE_IOCTL_LIST;

//
//	Control Register
//

/**
 * \brief Control Register - ENABLE
 */
#define	STIMER_DEV_ENABLED		CMSDK_TIMER_CTRL_EN_Msk

/**
 * \brief Control Register - disable interrupt
 */
#define STIMER_DEV_INTR_DISABLE		0x00000000
/**
 * \brief Control Register - enable interrupt
 */
#define STIMER_DEV_INTR_ENABLE 		CMSDK_TIMER_CTRL_IRQEN_Msk
/**
 * \brief Control Register - internal clock source, supported
 */
#define STIMER_DEV_INTCLK_MODE		0x00000000
/**
 * \brief Control Register - external clock source, not supported
 */
#define STIMER_DEV_EXTCLK_MODE		CMSDK_TIMER_CTRL_SELEXTCLK_Msk
/**
 * \brief Control Register - enable internal source
 */
#define STIMER_DEV_INTEN_MODE		0x00000000
/**
 * \brief Control Register - enable external source
 */
#define STIMER_DEV_EXTEN_MODE		CMSDK_TIMER_CTRL_SELEXTEN_Msk

#define	STIMER_DEV_IOCTL_MASK		0x0000000E

//---------------------------------------------------------
//	DRIVER :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//

#define INT_MAX_STIMER_VEC 	1


//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------
/**
 * \brief Structure of STIMER Driver
 */
struct	__stimer_device_handler__
{
	UINT32			dev_addr;  	/**< Physical address for identification */

	// Device-dependant
	UINT32			dev_unit;	/**< STIMER ID */
	UINT32			instance;	/**< Number of times that a driver with the same ID is created */
	UINT32			clock;		/**< Current value of clock count */
	UINT32			divider;	/**< Current value of divider */
	UINT32			tickcount;	/**< Number of tick interrupts */
	UINT32			setmode;	/**< Current operation mode */
	void			*clkmgmt;	/**< PLL callback structure */

	ISR_CALLBACK_TYPE	callback[INT_MAX_STIMER_VEC] ;	/**< function pointer of interrupt callback */
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create STIMER driver
 *
 * \param [in] dev_type		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 * \note  This driver is reserved for system operation.
 *        It's recommended to use DUALTIMER.
 *
 */
extern HANDLE	STIMER_CREATE( UINT32 dev_type );

/**
 * \brief Initialize STIMER driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	STIMER_INIT(HANDLE handler);

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
extern int	STIMER_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

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
extern int	STIMER_READ(HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen);

/**
 * \brief Close STIMER driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	STIMER_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------


#endif  /*__STIMER_H__*/
/**
 * \}
 * \}
 */
