/**
 * \addtogroup HALayer
 * \{
 * \addtogroup CMSDK_DUALTIMER	DualTimer
 * \{
 * \brief Two 32/16-bit down count timers with free-running, perioic and nonperiodic(one-shot) modes.
 */
 
/**
 ****************************************************************************************
 *
 * @file dualtimer.h
 *
 * @brief DualTimer Driver
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

#ifndef __DUAL_TIMER_H__
#define __DUAL_TIMER_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef 	struct __dtimer_device_handler__	DTIMER_HANDLER_TYPE;

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
/**
 * \brief DTIMER ID
 */
typedef 	enum	__dtimer_unit__ {
	DTIMER_UNIT_00 = 0,		/**< one part of DualTimer */
	DTIMER_UNIT_01,			/**< other part of DualTimer */
	DTIMER_UNIT_MAX
} DTIMER_UNIT_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief IOCTL Commands
 */
typedef enum	{
	DTIMER_GET_DEVREG = 1,	/**< Not used */

	DTIMER_GET_TICK,		/**< Get Number of Tick interrupts (data[0] : return number) */
	DTIMER_SET_MODE,		/**< Set Control Register (data[0] : value) */
	DTIMER_GET_MODE,		/**< Get current value of Control Register (data[0] : return value) */
	DTIMER_SET_LOAD,		/**< Set timer duration (data[0] : target frequency, data[1] : divider) */	
	DTIMER_SET_UTIME,		/**< Set timer duration (data[0] : target frequency, data[1] : microseconds) */
	DTIMER_SET_ACTIVE,		/**< Active the timer */
	DTIMER_SET_DEACTIVE,	/**< Deactive the timer */

	DTIMER_SET_CALLACK,		/**< Set interrupt callback : (data[0] : index '0', data[1], callback function, data[2] : callback parameter) */

	DTIMER_SET_MAX
} DTIMER_DEVICE_IOCTL_LIST;


 
/**
 * \brief Control Register - ENABLE
 */
#define	DTIMER_DEV_ENABLED			CMSDK_DUALTIMER_CTRL_EN_Msk

/**
 * \brief Control Register - disable periodic mode (must be used with WRAPPING) \n
 * FREERUN is that a counter operates continuously and wraps around to its
 * maximum value each time that it reaches zero. 
 */
#define DTIMER_DEV_FREERUN_MODE		0x00000000
/**
 * \brief Control Register - enable periodic mode \n
 * PERIODIC is that a counter operates continuously by reloading from the Load
 * Register each time that the counter reaches zero.
 */
#define DTIMER_DEV_PERIODIC_MODE 	CMSDK_DUALTIMER_CTRL_MODE_Msk
/**
 * \brief Control Register - disable interrupt
 */
#define DTIMER_DEV_INTR_DISABLE		0x00000000
/**
 * \brief Control Register - enable interrupt
 */
#define DTIMER_DEV_INTR_ENABLE 		CMSDK_DUALTIMER_CTRL_INTEN_Msk
/**
 * \brief Control Register - prescaler divider of 1
 */
#define DTIMER_DEV_PRESCALE_1		0x00000000
/**
 * \brief Control Register - prescaler divider of 16
 */
#define DTIMER_DEV_PRESCALE_16		((1)<<CMSDK_DUALTIMER_CTRL_PRESCALE_Pos)
/**
 * \brief Control Register - prescaler divider of 256
 */
#define DTIMER_DEV_PRESCALE_256		((2)<<CMSDK_DUALTIMER_CTRL_PRESCALE_Pos)
/**
 * \brief Control Register - 16 bit count mode
 */
#define DTIMER_DEV_16BIT_SIZE		0x00000000
/**
 * \brief Control Register - 32 bit count mode
 */
#define DTIMER_DEV_32BIT_SIZE		CMSDK_DUALTIMER_CTRL_SIZE_Msk
/**
 * \brief Control Register - disable one-shot mode 
 */
#define DTIMER_DEV_WRAPPING			0x00000000
/**
 * \brief Control Register - enable one-shot mode \n
 * ONESHOT is that a counter is loaded with a new value by writing to the Load
 * Register and decrements to zero and then halts until it is reprogrammed.
 */
#define DTIMER_DEV_ONESHOT			CMSDK_DUALTIMER_CTRL_ONESHOOT_Msk

#define	DTIMER_DEV_IOCTL_MASK		0x0000004F

//---------------------------------------------------------
//	DRIVER :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//

#define INT_MAX_DTIMER_VEC 	1


//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------
//
//	Driver Structure
//

struct	__dtimer_device_handler__
{
	UINT32			dev_addr;	/**< Physical address for identification */

	// Device-dependant
	UINT32			dev_unit;	/**< DTIMER ID */
	UINT32			instance;	/**< Number of times that a driver with the same ID is created */
	UINT32			clock;		/**< Current value of clock count */
	UINT32			divider;	/**< Current value of divider */
	UINT32			tickcount;	/**< Number of tick interrupts */
	void			*clkmgmt;	/**< PLL callback structure */

	ISR_CALLBACK_TYPE	callback[INT_MAX_DTIMER_VEC] ;
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create DTIMER driver
 *
 * \param [in] dev_type		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 */
extern HANDLE	DTIMER_CREATE( UINT32 dev_type );

/**
 * \brief Initialize DTIMER driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	DTIMER_INIT(HANDLE handler);

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
extern int	DTIMER_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief Read current raw value of TimerValue Register
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
extern int	DTIMER_READ(HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen);

/**
 * \brief Close DTIMER driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	DTIMER_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------


#endif  /*__DUAL_TIMER_H__*/
/**
 * \}
 * \}
 */
