/**
 * \addtogroup HALayer
 * \{
 * \addtogroup GPIO
 * \{
 * \brief GPIO driver
 */

/**
 ****************************************************************************************
 *
 * @file gpio.h
 *
 * @brief GPIO driver
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


#ifndef __GPIO_H__
#define __GPIO_H__


//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef 	struct __gpio_device_handler__	GPIO_HANDLER_TYPE;

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef 	enum	__gpio_unit__ {
	GPIO_UNIT_A = 0,
	GPIO_UNIT_B,
	GPIO_UNIT_C,
	GPIO_UNIT_MAX
} GPIO_UNIT_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters 
//---------------------------------------------------------
//
//	IOCTL Commands
//

typedef enum	{
	GPIO_GET_DEVREG = 1,

	GPIO_SET_OUTPUT,
	GPIO_SET_INPUT,
	GPIO_GET_DIRECTION,

	GPIO_SET_INTR_MODE,
	GPIO_GET_INTR_MODE,
	GPIO_SET_INTR_ENABLE,
	GPIO_SET_INTR_DISABLE,
	GPIO_GET_INTR_ENABLE,
	GPIO_GET_INTR_STATUS,
	GPIO_SET_INTR_CLEAR,
	
	GPIO_SET_MODE_ALT,
	GPIO_SET_MODE_NOALT,
	GPIO_GET_MODE_ALT,

	GPIO_SET_CALLACK,
	
	GPIO_SET_MAX	
} GPIO_DEVICE_IOCTL_LIST;

//
//	Interrupt Bit Setting
//

#define	INTR_TYP_MASK		0x0F		
#define	INTR_POL_MASK		0xF0		

#define	INTR_LOW_LEVEL		0x00
#define	INTR_HIGH_LEVEL		0x10
#define	INTR_RISE_EDGE		0x11
#define	INTR_FALL_EDGE		0x01

//
//	Pin Bit Mask
//

#define	GPIO_PIN0				0x0001
#define GPIO_PIN1				0x0002
#define GPIO_PIN2				0x0004
#define GPIO_PIN3				0x0008
#define GPIO_PIN4				0x0010
#define GPIO_PIN5				0x0020
#define GPIO_PIN6				0x0040
#define GPIO_PIN7				0x0080
#define GPIO_PIN8				0x0100
#define GPIO_PIN9				0x0200
#define GPIO_PIN10				0x0400
#define GPIO_PIN11				0x0800
#define GPIO_PIN12				0x1000
#define GPIO_PIN13				0x2000
#define GPIO_PIN14				0x4000
#define GPIO_PIN15				0x8000

//---------------------------------------------------------
//	DRIVER :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//


#define INT_MAX_GPIO_VEC		GPIO_INTIDX_MAX

typedef enum  {
	GPIO_INTR0 = 0,	
	GPIO_INTR1,	
	GPIO_INTR2,	
	GPIO_INTR3,	
	GPIO_INTR4,	
	GPIO_INTR5,	
	GPIO_INTR6,	
	GPIO_INTR7,	
	GPIO_INTR8,	
	GPIO_INTR9,	
	GPIO_INTR10,	
	GPIO_INTR11,	
	GPIO_INTR12,	
	GPIO_INTR13,	
	GPIO_INTR14,	
	GPIO_INTR15,	
	
	GPIO_INTR_MAX
}	GPIO_INTIDX_TYPE;

#define GPIOA_INTR_NUM	GPIO_INTR_MAX
#define GPIOB_INTR_NUM	12
#define GPIOC_INTR_NUM	15

//---------------------------------------------------------
//	DRIVER :: Structure 
//---------------------------------------------------------

struct	__gpio_device_handler__
{
	UINT32			dev_addr;  // Unique Address

	// Device-dependant
	UINT32			dev_unit;
	UINT32			instance;
	ISR_CALLBACK_TYPE	callback[GPIO_INTR_MAX] ;
};

//---------------------------------------------------------
//	DRIVER :: Alt Function
//---------------------------------------------------------

typedef enum {
	GPIO_ALT_FUNC_GPIO0  = 0x0,
	GPIO_ALT_FUNC_GPIO1  = 0x1,
	GPIO_ALT_FUNC_GPIO2  = 0x2,
	GPIO_ALT_FUNC_GPIO3  = 0x3,
	GPIO_ALT_FUNC_GPIO4  = 0x4,
	GPIO_ALT_FUNC_GPIO5  = 0x5,
	GPIO_ALT_FUNC_GPIO6  = 0x6,
	GPIO_ALT_FUNC_GPIO7  = 0x7,
	GPIO_ALT_FUNC_GPIO8  = 0x8,
	GPIO_ALT_FUNC_GPIO9  = 0x9,
	GPIO_ALT_FUNC_GPIO10 = 0xA,
	GPIO_ALT_FUNC_GPIO11 = 0xB,
	GPIO_ALT_FUNC_GPIO12 = 0xC,
	GPIO_ALT_FUNC_GPIO13 = 0xD,
	GPIO_ALT_FUNC_GPIO14 = 0xE,
	GPIO_ALT_FUNC_GPIO15 = 0xF
} GPIO_ALT_GPIO_NUM_TYPE;

typedef enum {
	GPIO_ALT_FUNC_PWM_OUT0 = 0,
	GPIO_ALT_FUNC_PWM_OUT1,
	GPIO_ALT_FUNC_PWM_OUT2,
	GPIO_ALT_FUNC_PWM_OUT3,
	GPIO_ALT_FUNC_EXT_INTR,
	GPIO_ALT_FUNC_MSPI_CSB1,
	GPIO_ALT_FUNC_MSPI_CSB2,
	GPIO_ALT_FUNC_MSPI_CSB3,
	GPIO_ALT_FUNC_RF_SW1,
	GPIO_ALT_FUNC_RF_SW2,
	GPIO_ALT_FUNC_UART0_TXDOE,
	GPIO_ALT_FUNC_UART1_TXDOE,
	GPIO_ALT_FUNC_UART2_TXDOE
} GPIO_ALT_FUNC_TYPE;

//---------------------------------------------------------
//	DRIVER :: APP-Interface 
//---------------------------------------------------------

/**
 ****************************************************************************************
 * @brief This function create GPIO handle. The DA16200 can only set GPIO_UNIT_A and GPIO_UNIT_C.
 * @param[in] dev_type Device index.
 * @return If succeeded return handle for the device, if failed return NULL.
 ****************************************************************************************
 */
extern HANDLE	GPIO_CREATE( UINT32 dev_type );

/**
 ****************************************************************************************
 * @brief This function initializes the GPIO handler.
 * @param[in] handler Device handle
 * @return TRUE if success, or FALSE if fails
 ****************************************************************************************
 */
extern int 	GPIO_INIT (HANDLE handler);

/**
 ****************************************************************************************
 * @brief The necessary configuration of GPIO can be set with this function
 * @param[in] handler Device handle
 * @param[in] cmd refer to the below information
 *   - GPIO_GET_DEVREG = 1,
 *   - GPIO_SET_OUTPUT, // set gpio as an output
 *   - GPIO_SET_INPUT, // set gpio as an input
 *   - GPIO_GET_DIRECTION, // get gpio direction
 *   - GPIO_SET_INTR_MODE, // set gpio interrupt mode [edge/level]
 *   - GPIO_GET_INTR_MODE, // get gpio interrupt mode
 *   - GPIO_SET_INTR_ENABLE, // enable gpio interrupt
 *   - GPIO_SET_INTR_DISABLE, // disable gpio interrupt
 *   - GPIO_GET_INTR_ENABLE, // get gpio interrupt enable status
 *   - GPIO_GET_INTR_STATUS, // get gpio interrupt pending status
 *   - GPIO_SET_INTR_CLEAR, // clear gpio interrupt status
 *   - GPIO_SET_CALLACK, // set a callback function for gpio interrupt
 * @param[in] data Data pointer
 * @return TRUE if success, or FALSE if fails
 ****************************************************************************************
 */
extern int 	GPIO_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 ****************************************************************************************
 * @brief GPIO get input level.
 * @param[in] handler Device handle
 * @param[in] addr gpio index
 * @param[in] pdata Data buffer pointer
 * @param[in] dlen Data buffer length
 * @return TRUE if success, or FALSE if fails
 ****************************************************************************************
 */
extern int 	GPIO_READ (HANDLE handler, UINT32 addr, UINT16 *pdata, UINT32 dlen);

/**
 ****************************************************************************************
 * @brief GPIO set output level.
 * @param[in] handler Device handle
 * @param[in] addr gpio index
 * @param[in] pdata Data buffer pointer
 * @param[in] dlen Data buffer length
 * @return TRUE if success, or FALSE if fails
 ****************************************************************************************
 */
extern int	GPIO_WRITE(HANDLE handler, UINT32 addr, UINT16 *pdata, UINT32 dlen);

/**
 ****************************************************************************************
 * @brief This function close GPIO handle.
 * @param[in] handler Device handle
 * @return TRUE if success, or FALSE if fails
 ****************************************************************************************
 */
extern int 	GPIO_CLOSE(HANDLE handler);


/**
 ****************************************************************************************
 * @brief Get GPIO handle.
 * @param[in] dev_type Device index.
 * @return If succeeded return handle for the device, if failed return NULL.
 ****************************************************************************************
 */
extern HANDLE  GPIO_GET_INSTANCE(UINT32 dev_type);

/**
 ****************************************************************************************
 * @brief Get GPIO alternate function setting value.
 * @param[in] handler Device handle
 * @param[in] alt_func gpio index
 * @param[in] regVal Data buffer pointer
 * @return 0 if successful, -1 if handler is null, or -2 if alt_func is unknown
 ****************************************************************************************
 */
INT32 GPIO_GET_ALT_FUNC(HANDLE handler, GPIO_ALT_FUNC_TYPE alt_func, UINT32 * regVal);

/**
 ****************************************************************************************
 * @brief Set GPIO alternate function setting value.
 * @param[in] handler Device handle
 * @param[in] alt_func gpio index
 * @param[in] gpio_num GPIO number
 * @return 0 if successful, -1 if handler is null, or -2 if alt_func is unknown
 ****************************************************************************************
 */
INT32 GPIO_SET_ALT_FUNC(HANDLE handler, GPIO_ALT_FUNC_TYPE alt_func,
		GPIO_ALT_GPIO_NUM_TYPE gpio_num);


INT32 GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num);
INT32 GPIO_RETAIN_HIGH_RECOVERY();

#endif  /*__GPIO_H__*/

/* EOF */
