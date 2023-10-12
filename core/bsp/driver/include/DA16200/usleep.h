/**
 * \addtogroup HALayer
 * \{
 * \addtogroup USLEEP
 * \{
 * \brief Driver to support a micro sleep less than RTOS sleep (systick).
 */

/**
 ****************************************************************************************
 *
 * @file usleep.h
 *
 * @brief Usleep Driver
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


#ifndef __usleep_h__
#define __usleep_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef 	struct __usleep_handler__	USLEEP_HANDLER_TYPE; 	// deferred
typedef 	struct __usleep_info__		USLEEP_INFO_TYPE; 	// deferred

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief USLEEP ID
 */
typedef enum	{
	USLEEP_0	= 0,
	USLEEP_MAX
} USLEEP_LIST;

/**
 * \brief IOCTL Commands
 */
typedef enum	{
	USLEEP_SET_DEBUG = 20,		/**< Not used */

	USLEEP_SET_RUNMODE,			/**< Set the running mode (data[0] : TRUE - periodic mode, FALSE - oneshot mode) */
	USLEEP_SET_ITEMNUM,			/**< Not used */
	USLEEP_SET_RESOLUTION,		/**< Set the USLEEP duration (data[0] : usec) */
	USLEEP_GET_INFO,			/**< Print an USLEEP list */
	USLEEP_GET_TICK,			/**< Get current tick value */

	USLEEP_SET_MAX
} USLEEP_IOCTL_LIST;

//---------------------------------------------------------
//
//---------------------------------------------------------

#define		USLEEP_MAX_ITEM	8


//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------

struct __usleep_info__
{
	UINT32			seq;
	UINT32			ticks;
	char			evtname[8];
	EventGroupHandle_t event;
	USR_CALLBACK		callback;
	VOID			*param;
	USLEEP_INFO_TYPE	*next;
};

/**
 * \brief Structure of USLEEP Driver
 */
struct	__usleep_handler__
{
	// Device-dependant
	UINT32			dev_unit;		/**< USLEEP ID */
	UINT32			instance;		/**< Number of times that a driver with the same ID is created */	
	UINT32			timer_id;		/**< STIMER ID, STIMER_UNIT_1 */
	HANDLE			timer;			/**< handler of STIMER driver */

	UINT32			resolution;		/**< usleep resolution */
	UINT32			ticks;			/**< tick count */
	UINT32			genseq;			/**< internal sequence number */			
	UINT32			runmode;		/**< current running mode */

	UINT32			blknum;			/**< maximum number of entries */
	USLEEP_INFO_TYPE	*block;		/**< linked list of entries */
	UINT32			usecnt;			/**< internal field of driver */

	SemaphoreHandle_t		mutex;
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Get current handler of USLEEP
 *
 * \return	HANDLE			pointer to the driver instance
 *
 */
extern HANDLE	USLEEP_HANDLER(void);

/**
 * \brief Create USLEEP driver
 *
 * \param [in] dev_id		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 */
extern HANDLE	USLEEP_CREATE( UINT32 dev_id );

/**
 * \brief Initialize USLEEP driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	USLEEP_INIT (HANDLE handler);

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
extern int	USLEEP_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief Read current value of STIMER in USLEEP
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
extern int	USLEEP_READ (HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);

/**
 * \brief Close USLEEP driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	USLEEP_CLOSE(HANDLE handler);

/**
 * \brief Suspend current thread until the tick expires.
 *
 * \param [in] handler		driver handler
 * \param [in] ticks		expired time (usec)
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	USLEEP_SUSPEND(HANDLE handler, UINT32 ticks);

/**
 * \brief Set a callback function that is called when the tick expires.
 *
 * \param [in] handler		driver handler
 * \param [in] ticks		expired time (usec)
 * \param [in] expire_func	function pointer of the callback
 * \param [in] param		parameters of callback
 *
 * \return	Non-Zero,		function succeeded. sequence number of callback
 * \return	NULL, 			function failed 
 *
 */
extern HANDLE	USLEEP_REGISTRY_TIMER(HANDLE handler, UINT32 ticks, USR_CALLBACK expire_func, VOID *param);

/**
 * \brief Remove a callback function from the USLEEP list
 *
 * \param [in] handler		driver handler
 * \param [in] timer		sequence number of the callback
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	USLEEP_UNREGISTRY_TIMER(HANDLE handler, HANDLE timer);


//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------


#endif /* __usleep_h__ */
/**
 * \}
 * \}
 */
