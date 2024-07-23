/**
 * \addtogroup HALayer
 * \{
 * \addtogroup PWM
 * \{
 * \brief Pulse Width Moduator control
 */
 
/**
 ****************************************************************************************
 *
 * @file pwm.h
 *
 * @brief PWM Generator
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


#ifndef __pwm_h__
#define __pwm_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
typedef 	struct __pwm_handler__	PWM_HANDLER_TYPE; 	// deferred
typedef 	struct __pwm_regmap__ 	PWM_REG_MAP;		// deferred

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

/**
 * \brief PWM device ID
 */
typedef enum	{
	pwm_0	= 0,/**< PWM Channel 0 */
	pwm_1	= 1,/**< PWM Channel 1 */
	pwm_2	= 2,/**< PWM Channel 2 */
	pwm_3	= 3,/**< PWM Channel 3 */
	PWM_MAX
} PWM_LIST;


//---------------------------------------------------------
//	DRIVER :: Definition
//---------------------------------------------------------

#define 	PWM_CTRL_ENABLE		((0x1)<<0)

/**
 * \brief PWM Start Mode
 */
typedef enum    {
    PWM_DRV_MODE_CYC = 1, /**< Unit of Start Parameter is Cycle */
    PWM_DRV_MODE_US	 =0   /**< Unit of Start Parameter is micor second and percent */
} PWM_MODE;





#define SYS_PWM_BASE  (PWMO_BASE_0)


#define HW_REG_WRITE32_PWM(addr,value)  (addr) = (value)
#define HW_REG_READ32_PWM(addr)   ((addr))
#define HW_REG_AND_WRITE32_PWM(addr,value)  (addr) = ((addr)&(value))
#define HW_REG_OR_WRITE32_PWM(addr,value) (addr) = ((addr)|(value))


/**
 * \brief Register MAP 
 */
struct	__pwm_regmap__
{
	volatile UINT32	pwm_en0; /**< -*/		// 0x00
	volatile UINT32	pwm_en1; /**< -*/		// 0x04
	volatile UINT32	pwm_en2; /**< -*/		// 0x08
	volatile UINT32	pwm_en3; /**< -*/		// 0x0C
	volatile UINT32	pwm_maxcy0; /**< -*/		// 0x10
	volatile UINT32	pwm_maxcy1; /**< -*/		// 0x14
	volatile UINT32	pwm_maxcy2; /**< -*/		// 0x18
	volatile UINT32	pwm_maxcy3; /**< -*/		// 0x1c
	volatile UINT32	pwm_hduty0; /**< -*/		// 0x20
	volatile UINT32	pwm_hduty1; /**< -*/			// 0x24
	volatile UINT32	pwm_hduty2; /**< -*/			// 0x28
	volatile UINT32	pwm_hduty3; /**< -*/			// 0x2c
	volatile UINT32	pwm_mc_offs0; /**< -*/			// 0x30
	volatile UINT32	pwm_mc_offs1; /**< -*/			// 0x34
	volatile UINT32	pwm_mc_offs2; /**< -*/			// 0x38
	volatile UINT32	pwm_mc_offs3; /**< -*/			// 0x3c
	volatile UINT32	pwm_mc_mode; /**< -*/			// 0x40
};


/**
 * \brief PWM Handle Structure 
 */
struct	__pwm_handler__
{
	UINT32				dev_addr;  /**< register address */
	// Device-dependant
	UINT32				dev_unit;  /**< channel */
	UINT32				instance;/**< -*/
	UINT32				clock;/**< -*/

	// Register Map
	PWM_REG_MAP			*regmap;/**< -*/

	VOID				*mutex;/**< -*/
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create PWM Handle
 *
 * This function will create PWM handle
 *
 * \param [in] dev_id Possible values for device id is:\parblock
 *     - ::pwm_0
 *     - ::pwm_1
 *     - ::pwm_2 
 *     - ::pwm_3 
 * \endparblock
 *
 * \return Handle for PWM device ID in case success, NULL otherwise
 *
 */
extern HANDLE	DRV_PWM_CREATE( UINT32 dev_id );

/**
 * \brief Initialize PWM 
 *
 * This function will init PWM 
 *
 * \param [in] handler handle for intialization PWM block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_PWM_INIT (HANDLE handler);

/**
 * \brief Start PWM 
 *
 * This function will start PWM 
 *
 * \param [in] handler handle for start PWM
 * \param [in] period_us period in us or cycles 
 * \param [in] hduty_percent duty high duration in percent or cycles
 * \param [in] mode Possible values for mode is: \parblock
 *	    - ::PWM_DRV_MODE_CYC 	
 *	    - ::PWM_DRV_MODE_US 	
 * \endparblock
 *
 * \return TRUE in case success, FALSE otherwise
 */
extern int DRV_PWM_START(HANDLE handler,  UINT32 period_us, UINT32 hduty_percent, UINT32 mode);

/**
 * \brief Stop PWM 
 *
 * This function will stop PWM 
 *
 * \param [in] handler handle for stop PWM
 *
 * \return TRUE in case success, FALSE otherwise
 */
extern int DRV_PWM_STOP(HANDLE handler,  UINT32 dummy);

/**
 * \brief Close PWM Handle
 *
 * This function will close PWM 
 *
 * \param [in] handler handle for close PWM
 *
 * \return TRUE in case success, FALSE otherwise
 */
extern int	DRV_PWM_CLOSE(HANDLE handler);


#define PWM_DEBUG_ON

#define DRIVER_DBG_NONE		DRV_PRINTF
#define DRIVER_DBG_ERROR	DRV_PRINTF

#ifdef PWM_DEBUG_ON
#define PRINT_PWM 			DRV_PRINTF
#else
#define PRINT_PWM
#endif

#endif /* __pwm_h__ */

/**
 * \}
 * \}
 */

