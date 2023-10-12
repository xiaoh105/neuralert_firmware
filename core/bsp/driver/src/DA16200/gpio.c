/**
 ****************************************************************************************
 *
 * @file gpio.c
 *
 * @brief GPIO Driver
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "sys_nvic.h"

#if 0//(dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#define traceISR_ENTER()
#define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

//---------------------------------------------------------
// Macro
//---------------------------------------------------------

#define		GPIO_ISR_DECLARATION(port, idx)				\
	static void _lowlevel_gpio_intr_ ## port ## _ ## idx (void)

#define		GPIO_ISR_REGISTRATION(port, idx)				\
	_sys_nvic_write( (PORTA_0_IRQn+idx), 				\
	(void *)_lowlevel_gpio_intr_ ## port ## _ ## idx, 0x7)

#define		GPIO_ISR_UNREGISTRATION(port, idx)			\
	_sys_nvic_write( (PORTA_0_IRQn+idx), (void *)NULL, 0x7)


#define		GPIO_ISR_DEFINITION(port, idx)				\
									\
static  void	_lowlevel_gpio_intr_ ## port ## _ ## idx(void)	\
{									\
	traceISR_ENTER();                 \
	_gpiocore_interrupt_ ## port(idx);  \
	traceISR_EXIT();                 \
}

#define CLEAR_BITS(reg, area, loc)		((reg) &= ~((area) << loc))
#define SET_BIT(reg, loc)	((reg) |= ((0x1) << (loc)))
#define SET_BITS(reg, val, loc)		((reg) |= ((val) << (loc)))
#define GET_BIT(reg, loc)	((reg) & ((0x1) << (loc)))
#define EXTRACT_BITS(reg, area, loc) (((reg) >> (loc)) & (area))

//#define GPIO_TRACE_PRINT(...)	DRV_PRINTF(__VA_ARGS__)
#define GPIO_TRACE_PRINT(...)


//---------------------------------------------------------
//
//---------------------------------------------------------

static int	gpio_isr_create(HANDLE handler);
static int	gpio_isr_init(HANDLE handler);
static int	gpio_isr_close(HANDLE handler);
static int	gpio_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler);
static INT32 gpio_set_high(INT32 gpio_port, INT32 gpio_num);

void 		_gpiocore_interrupt_a (UINT32 index);

GPIO_ISR_DECLARATION(a, 0);
GPIO_ISR_DECLARATION(a, 1);
GPIO_ISR_DECLARATION(a, 2);
GPIO_ISR_DECLARATION(a, 3);
GPIO_ISR_DECLARATION(a, 4);
GPIO_ISR_DECLARATION(a, 5);
GPIO_ISR_DECLARATION(a, 6);
GPIO_ISR_DECLARATION(a, 7);
GPIO_ISR_DECLARATION(a, 8);
GPIO_ISR_DECLARATION(a, 9);
GPIO_ISR_DECLARATION(a, 10);
GPIO_ISR_DECLARATION(a, 11);
GPIO_ISR_DECLARATION(a, 12);
GPIO_ISR_DECLARATION(a, 13);
GPIO_ISR_DECLARATION(a, 14);
GPIO_ISR_DECLARATION(a, 15);


void 		_gpio_b_interrupt(void);
void 		_gpio_c_interrupt(void);
static  void	_tx_specific_gpio_b_interrupt(void);
static  void	_tx_specific_gpio_c_interrupt(void);
static void disable_gpio_pe_ps(UINT32 dev_id, UINT32 pin_num);
static INT32 gpio_set_high(INT32 gpio_port, INT32 gpio_num);

//---------------------------------------------------------
//
//---------------------------------------------------------

static int			_gpio_instance[GPIO_UNIT_MAX];
static GPIO_HANDLER_TYPE	*_gpio_handler[GPIO_UNIT_MAX];

/******************************************************************************
 *  GPIO_CREATE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	GPIO_CREATE( UINT32 dev_id )
{
	GPIO_HANDLER_TYPE	*gpio;

	if( dev_id >= GPIO_UNIT_MAX ){
		DRV_DBG_ERROR("GPIO, ilegal unit, %d", dev_id);
		return NULL;
	}

	// Allocate
	if( _gpio_handler[dev_id] == NULL ){
		gpio = (GPIO_HANDLER_TYPE *)pvPortMalloc(sizeof(GPIO_HANDLER_TYPE));
		if( gpio == NULL ){
			return NULL;
		}
		DRV_MEMSET(gpio, 0, sizeof(GPIO_HANDLER_TYPE));
	}else{
		gpio = _gpio_handler[dev_id];
	}

	// Address Mapping
	switch((GPIO_UNIT_TYPE)dev_id){
		case	GPIO_UNIT_A:
			gpio->dev_addr = (UINT32)CMSDK_GPIO_A;
			gpio->instance = (UINT32)(_gpio_instance[GPIO_UNIT_A]);
			_gpio_instance[GPIO_UNIT_A] ++;
			break;
		case	GPIO_UNIT_B:
			gpio->dev_addr = (UINT32)CMSDK_GPIO_B;
			gpio->instance = (UINT32)(_gpio_instance[GPIO_UNIT_B]);
			_gpio_instance[GPIO_UNIT_B] ++;
			break;
		case	GPIO_UNIT_C:
			gpio->dev_addr = (UINT32)CMSDK_GPIO_C;
			gpio->instance = (UINT32)(_gpio_instance[GPIO_UNIT_C]);
			_gpio_instance[GPIO_UNIT_C] ++;
			break;

		default:
			break;
	}

	gpio->dev_unit = dev_id ;

//	DRV_DBG_BASE("(%p) GPIO Driver create, base %p", gpio, (UINT32 *)(gpio->dev_addr) );

	if(gpio->instance == 0){
		_gpio_handler[dev_id] = gpio;
		// registry default ISR
		 gpio_isr_create(gpio);
	}

	return (HANDLE) gpio;
}

/******************************************************************************
 *  GPIO_INIT ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	GPIO_INIT (HANDLE handler)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;
	int ret = FALSE;

	if(handler == NULL){
		return ret;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	// Normal Mode
	if(gpio->instance == 0){
		gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

		gpioreg->INTENCLR = CMSDK_GPIO_INTENCLR_Msk ;
		gpioreg->INTCLEAR = CMSDK_GPIO_INTCLEAR_Msk ;

		// Create HISR
		ret = gpio_isr_init(gpio);
	}
	return ret;
}

/******************************************************************************
 *  GPIO_IOCTL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	GPIO_IOCTL(HANDLE handler, UINT32 cmd , VOID *data)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	switch(cmd){
		case GPIO_GET_DEVREG:
			data = (VOID *)gpioreg;
            break;
		case GPIO_SET_OUTPUT:
			gpioreg->OUTENABLESET = *((UINT32 *)data) ;
			disable_gpio_pe_ps(gpio->dev_unit, *((UINT32 *)data));
			break;
		case GPIO_SET_INPUT:
			gpioreg->OUTENABLECLR = *((UINT32 *)data) ;
			disable_gpio_pe_ps(gpio->dev_unit, *((UINT32 *)data));
			break;
		case GPIO_GET_DIRECTION:
			*((UINT32 *)data) = gpioreg->OUTENABLESET ;
			break;

		case GPIO_SET_INTR_MODE:
			// ((UINT32 *)data)[0], MODE
			// ((UINT32 *)data)[1], PIN
			gpioreg->INTTYPESET = ((UINT32 *)data)[0];
			gpioreg->INTTYPECLR = ~(((UINT32 *)data)[0]);

			gpioreg->INTPOLSET = ((UINT32 *)data)[1] ;
			gpioreg->INTPOLCLR = ~(((UINT32 *)data)[1]) ;
			break;
		case GPIO_GET_INTR_MODE:
			if( data != NULL ){
				((UINT32 *)data)[0] = gpioreg->INTTYPESET;
				((UINT32 *)data)[1] = gpioreg->INTPOLSET;
			}
			break;

		case GPIO_SET_INTR_ENABLE:
			gpioreg->INTENSET = *((UINT32 *)data) ;
			break;
		case GPIO_SET_INTR_DISABLE:
			gpioreg->INTENCLR = *((UINT32 *)data) ;
			break;
		case GPIO_GET_INTR_ENABLE:
			*((UINT32 *)data) = gpioreg->INTENSET ;
			break;

		case GPIO_GET_INTR_STATUS:
			*((UINT32 *)data) = gpioreg->INTSTATUS ;
			break;
		case GPIO_SET_INTR_CLEAR:
			gpioreg->INTCLEAR = *((UINT32 *)data) ;
			break;


		case GPIO_SET_MODE_ALT:
			gpioreg->ALTFUNCSET = *((UINT32 *)data) ;
			break;
		case GPIO_SET_MODE_NOALT:
			gpioreg->ALTFUNCCLR = *((UINT32 *)data) ;
			break;
		case GPIO_GET_MODE_ALT:
			*((UINT32 *)data) = gpioreg->ALTFUNCSET ;
			break;

		case GPIO_SET_CALLACK:
			return gpio_callback_registry( gpio
						, ((UINT32 *)data)[0]
						, ((UINT32 *)data)[1]
						, ((UINT32 *)data)[2] );

		default:
			break;
	}
	return TRUE;
}

/******************************************************************************
 *  GPIO_WRITE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	GPIO_WRITE(HANDLE handler, UINT32 addr, UINT16 *pdata, UINT32 dlen)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	DA16X_UNUSED_ARG(dlen);

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	gpioreg->LB_MASKED[addr & 0x0FF] = *pdata;
	gpioreg->UB_MASKED[(addr>>8)&0x0FF] = *pdata;

	return TRUE;
}

/******************************************************************************
 *  GPIO_READ ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	GPIO_READ(HANDLE handler, UINT32 addr, UINT16 *pdata, UINT32 dlen)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	DA16X_UNUSED_ARG(dlen);

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	*pdata = gpioreg->LB_MASKED[addr & 0x0FF];
	*pdata |= gpioreg->UB_MASKED[(addr>>8)&0x0FF];

	return TRUE;
}

/******************************************************************************
 *  GPIO_CLOSE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	GPIO_CLOSE(HANDLE handler)
{
	GPIO_HANDLER_TYPE	*gpio;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;

	if(_gpio_instance[gpio->dev_unit] > 0){
		_gpio_instance[gpio->dev_unit] --;

		if(_gpio_instance[gpio->dev_unit] == 0) {
			// Close ISR
			_gpio_handler[gpio->dev_unit] = NULL;
			gpio_isr_close(gpio);
			vPortFree(gpio);
		}
	}

	return TRUE;
}

HANDLE GPIO_GET_INSTANCE(UINT32 dev_type)
{
	if( dev_type >= GPIO_UNIT_MAX ){
		DRV_DBG_ERROR("GPIO, ilegal unit, %d", dev_type);
		return NULL;
	}

	return _gpio_handler[dev_type];
}


INT32 GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num)
{
	UINT32 ioctl;
	if (gpio_port >= GPIO_UNIT_MAX)
		return FALSE;

	gpio_set_high(gpio_port, gpio_num);

	/*
	 * 1. save GPIO Port to RETM
	 * 2. save GPIO Pin Number to RETM
	 */
	SET_BIT(RETMEM_GPIO_RETENTION_INFO->ret_info1, gpio_port);
	if (gpio_port == GPIO_UNIT_A) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, gpio_num, 16);
	} else if (gpio_port == GPIO_UNIT_B) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, gpio_num, 0);
	} else if (gpio_port == GPIO_UNIT_C) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, gpio_num, 16);
	}

	/* call RTC control */
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &ioctl);
	ioctl |= IO_RETENTION_ENABLE((IO_RETENTION_DIO1 | IO_RETENTION_SEN_VDD));
	RTC_IOCTL(RTC_SET_RETENTION_CONTROL_REG, &ioctl);

	return TRUE;
}

INT32 GPIO_RETAIN_HIGH_RECOVERY()
{
	UINT32 port_info, val = 0;
	UINT32 pin_info[GPIO_UNIT_MAX];

	/* call RTC control */
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &val);
	val &= ~(IO_RETENTION_ENABLE((IO_RETENTION_DIO1 | IO_RETENTION_SEN_VDD)));
	RTC_IOCTL(RTC_SET_RETENTION_CONTROL_REG, &val);

	/* restore pin mux info */
	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO1, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio1;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO1, val, 0);
	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO2, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio2;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO2, val, 0);

	GPIO_TRACE_PRINT("restore pin mux info\n");
	GPIO_TRACE_PRINT("\tDA16200_DIOCFG->FSEL_GPIO1: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO1);
	GPIO_TRACE_PRINT("\tDA16200_DIOCFG->FSEL_GPIO2: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO2);

	/* extract gpio retention info */
	port_info = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0x7, 0);
	pin_info[GPIO_UNIT_A] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffff, 16);
	pin_info[GPIO_UNIT_B] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 0);
	pin_info[GPIO_UNIT_C] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 16);

	GPIO_TRACE_PRINT("extract gpio retention info\n");
	GPIO_TRACE_PRINT("\tgpio port: 0x%08x\n", port_info);
	GPIO_TRACE_PRINT("\tgpio_a pin: 0x%08x\n", pin_info[0]);
	GPIO_TRACE_PRINT("\tgpio_b pin: 0x%08x\n", pin_info[1]);
	GPIO_TRACE_PRINT("\tgpio_c pin: 0x%08x\n", pin_info[2]);

	/* create gpio instance */
	for (INT32 i = 0; i < GPIO_UNIT_MAX; i++)
	{
		if (GET_BIT(port_info, i))
			gpio_set_high(i, (INT32)(pin_info[i]));
	}

	/* clear gpio retention info */
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffffffff, 0);
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffffffff, 0);

	GPIO_TRACE_PRINT("clear gpio retention info\n");
	GPIO_TRACE_PRINT("\tret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);

	return TRUE;
}


/******************************************************************************
 *  gpio_isr_create ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	gpio_isr_create(HANDLE handler)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	// Disable All Interrupts
	gpioreg->INTENCLR = 0xFFFF;
	gpioreg->INTCLEAR = 0xFFFF;

	return TRUE;

}

/******************************************************************************
 *  gpio_isr_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	gpio_isr_init(HANDLE handler)
{
	GPIO_HANDLER_TYPE	*gpio;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;

	// Registry LISR
	switch(gpio->dev_unit){
		case	GPIO_UNIT_A:
			GPIO_ISR_REGISTRATION( a, 0);
			GPIO_ISR_REGISTRATION( a, 1);
			GPIO_ISR_REGISTRATION( a, 2);
			GPIO_ISR_REGISTRATION( a, 3);
			GPIO_ISR_REGISTRATION( a, 4);
			GPIO_ISR_REGISTRATION( a, 5);
			GPIO_ISR_REGISTRATION( a, 6);
			GPIO_ISR_REGISTRATION( a, 7);
			GPIO_ISR_REGISTRATION( a, 8);
			GPIO_ISR_REGISTRATION( a, 9);
			GPIO_ISR_REGISTRATION( a, 10);
			GPIO_ISR_REGISTRATION( a, 11);
			GPIO_ISR_REGISTRATION( a, 12);
			GPIO_ISR_REGISTRATION( a, 13);
			GPIO_ISR_REGISTRATION( a, 14);
			GPIO_ISR_REGISTRATION( a, 15);
			break;
		case	GPIO_UNIT_B:
			_sys_nvic_write(PORTB_ALL_IRQn, (void *)_tx_specific_gpio_b_interrupt, 0x7);
			break;

		case	GPIO_UNIT_C:
			_sys_nvic_write(PORTC_ALL_IRQn, (void *)_tx_specific_gpio_c_interrupt, 0x7);
			break;
	}

	return TRUE;
}

/******************************************************************************
 *  gpio_isr_close ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	gpio_isr_close(HANDLE handler)
{
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	// Disable All Interrupts
	gpioreg->INTENCLR = 0xFFFF;
	gpioreg->INTCLEAR = 0xFFFF;

	// UN-Registry LISR
	switch(gpio->dev_unit){
		case	GPIO_UNIT_A:
			GPIO_ISR_UNREGISTRATION( a, 0);
			GPIO_ISR_UNREGISTRATION( a, 1);
			GPIO_ISR_UNREGISTRATION( a, 2);
			GPIO_ISR_UNREGISTRATION( a, 3);
			GPIO_ISR_UNREGISTRATION( a, 4);
			GPIO_ISR_UNREGISTRATION( a, 5);
			GPIO_ISR_UNREGISTRATION( a, 6);
			GPIO_ISR_UNREGISTRATION( a, 7);
			GPIO_ISR_UNREGISTRATION( a, 8);
			GPIO_ISR_UNREGISTRATION( a, 9);
			GPIO_ISR_UNREGISTRATION( a, 10);
			GPIO_ISR_UNREGISTRATION( a, 11);
			GPIO_ISR_UNREGISTRATION( a, 12);
			GPIO_ISR_UNREGISTRATION( a, 13);
			GPIO_ISR_UNREGISTRATION( a, 14);
			GPIO_ISR_UNREGISTRATION( a, 15);
			break;
		case	GPIO_UNIT_B:
			_sys_nvic_write(PORTB_ALL_IRQn, NULL, 0x7);
			break;

		case	GPIO_UNIT_C:
			_sys_nvic_write(PORTC_ALL_IRQn, NULL, 0x7);
			break;
	}

	return TRUE;

}

/******************************************************************************
 *  gpio_callback_registry ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	 gpio_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler)
{
	GPIO_HANDLER_TYPE	*gpio;
	UINT32			idx;

	if(handler == NULL){
		return FALSE;
	}
	gpio = (GPIO_HANDLER_TYPE *)handler ;

	for( idx = 0; idx < GPIO_INTR_MAX; idx++ ){

		if( (index & (1<<idx)) != 0 ){
			gpio->callback[idx].func  = (void (*)(void *)) callback;
			gpio->callback[idx].param = (HANDLE)userhandler	;
		}
	}

	return TRUE;
}

/******************************************************************************
 *  _gpiocore_interrupt_a ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void _gpiocore_interrupt_a (UINT32 index)
{
	CMSDK_GPIO_TypeDef	*gpioreg;

	if( _gpio_handler[0] != NULL ){
		gpioreg = (CMSDK_GPIO_TypeDef *)(_gpio_handler[0]->dev_addr);
		gpioreg->INTCLEAR = (1<<index);

		if( _gpio_handler[0]->callback[index].func != NULL ){
			_gpio_handler[0]->callback[index].func( _gpio_handler[0]->callback[index].param );
		}
	}
}

GPIO_ISR_DEFINITION(a, 0)
GPIO_ISR_DEFINITION(a, 1)
GPIO_ISR_DEFINITION(a, 2)
GPIO_ISR_DEFINITION(a, 3)
GPIO_ISR_DEFINITION(a, 4)
GPIO_ISR_DEFINITION(a, 5)
GPIO_ISR_DEFINITION(a, 6)
GPIO_ISR_DEFINITION(a, 7)
GPIO_ISR_DEFINITION(a, 8)
GPIO_ISR_DEFINITION(a, 9)
GPIO_ISR_DEFINITION(a, 10)
GPIO_ISR_DEFINITION(a, 11)
GPIO_ISR_DEFINITION(a, 12)
GPIO_ISR_DEFINITION(a, 13)
GPIO_ISR_DEFINITION(a, 14)
GPIO_ISR_DEFINITION(a, 15)

/******************************************************************************
 *  _gpio_b_interrupt ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void _gpio_b_interrupt (void)
{
	UINT32		i , intstatus ;
	CMSDK_GPIO_TypeDef	*gpioreg;


	if( _gpio_handler[1] != NULL ){
		gpioreg = (CMSDK_GPIO_TypeDef *)(_gpio_handler[1]->dev_addr);
		intstatus = gpioreg->INTSTATUS ;
		gpioreg->INTCLEAR = intstatus;

		for( i=0; i< GPIOB_INTR_NUM; i++){
			if( (intstatus & (0x01<<i)) != 0 ){
				if( _gpio_handler[1]->callback[i].func != NULL ){
					_gpio_handler[1]->callback[i].func( _gpio_handler[1]->callback[i].param );
				}
			}
		}
	}
}

static  void	_tx_specific_gpio_b_interrupt(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(_gpio_b_interrupt);
	traceISR_EXIT();
}

/******************************************************************************
 *  _gpio_c_interrupt ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void _gpio_c_interrupt (void)
{
	UINT32		i , intstatus ;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if( _gpio_handler[2] != NULL ){
		gpioreg = (CMSDK_GPIO_TypeDef *)(_gpio_handler[2]->dev_addr);
		intstatus = gpioreg->INTSTATUS ;
		gpioreg->INTCLEAR = intstatus;

		for( i=0; i< GPIOC_INTR_NUM; i++){
			if( (intstatus & (0x01<<i)) != 0 ){
				if( _gpio_handler[2]->callback[i].func != NULL ){
					_gpio_handler[2]->callback[i].func( _gpio_handler[2]->callback[i].param );
				}
			}
		}
	}
}

static  void	_tx_specific_gpio_c_interrupt(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(_gpio_c_interrupt);
	traceISR_EXIT();
}


/******************************************************************************
 *  disable_gpio_pe_ps ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
static void disable_gpio_pe_ps(UINT32 dev_id, UINT32 pin_num)
{
	GPIO_TRACE_PRINT("dev_id: %d, pin_num: 0x%04x\n", dev_id, pin_num);

	if (dev_id == GPIO_UNIT_A) {
		DA16200_DIOCFG->GPIOA_PE_PS &= ~(pin_num<<16);
	} else if (dev_id == GPIO_UNIT_B) {
		DA16200_DIOCFG->GPIOB_PE_PS &= ~(pin_num<<16);
	} else if (dev_id == GPIO_UNIT_C) {
		DA16200_DIOCFG->GPIOC_PE_PS &= ~(pin_num<<16);
	}
}

/******************************************************************************
 *  gpio_set_high ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
static INT32 gpio_set_high(INT32 gpio_port, INT32 gpio_num)
{
	HANDLE gpio;
	UINT32 ioctldata;
	UINT16 data;

	gpio = GPIO_CREATE((UINT32)gpio_port);
	if (gpio == NULL)
	{
		return FALSE;
	}

	GPIO_INIT(gpio);

	ioctldata = (UINT32)gpio_num;
	data = (UINT16)gpio_num;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &ioctldata);
	GPIO_WRITE(gpio, (UINT32)gpio_num, &data, sizeof(UINT16));

	GPIO_TRACE_PRINT("GPIO%d Created, 0x%08x Pin Output High.\n", gpio_port, gpio_num);

	return TRUE;
}


/******************************************************************************
 *  GPIO_SET_ALT_FUNC ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
INT32 GPIO_SET_ALT_FUNC(HANDLE handler, GPIO_ALT_FUNC_TYPE alt_func,
		GPIO_ALT_GPIO_NUM_TYPE gpio_num) {

	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if(handler == NULL) {
		return -1;
	}

	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	switch (alt_func) {
	case GPIO_ALT_FUNC_PWM_OUT0:
		CLEAR_BITS(gpioreg->PWM_OUT_SEL, 0xf, 0);
		SET_BITS(gpioreg->PWM_OUT_SEL, gpio_num, 0);
		break;
	case GPIO_ALT_FUNC_PWM_OUT1:
		CLEAR_BITS(gpioreg->PWM_OUT_SEL, 0xf, 4);
		SET_BITS(gpioreg->PWM_OUT_SEL, gpio_num, 4);
		break;
	case GPIO_ALT_FUNC_PWM_OUT2:
		CLEAR_BITS(gpioreg->PWM_OUT_SEL, 0xf, 8);
		SET_BITS(gpioreg->PWM_OUT_SEL, gpio_num, 8);
		break;
	case GPIO_ALT_FUNC_PWM_OUT3:
		CLEAR_BITS(gpioreg->PWM_OUT_SEL, 0xf, 12);
		SET_BITS(gpioreg->PWM_OUT_SEL, gpio_num, 12);
		break;
	case GPIO_ALT_FUNC_EXT_INTR:
		CLEAR_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 0);
		SET_BITS(gpioreg->MSPI_CS_OUT_SEL, gpio_num, 0);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB1:
		CLEAR_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 4);
		SET_BITS(gpioreg->MSPI_CS_OUT_SEL, gpio_num, 4);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB2:
		CLEAR_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 8);
		SET_BITS(gpioreg->MSPI_CS_OUT_SEL, gpio_num, 8);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB3:
		CLEAR_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 12);
		SET_BITS(gpioreg->MSPI_CS_OUT_SEL, gpio_num, 12);
		break;
	case GPIO_ALT_FUNC_RF_SW1:
		CLEAR_BITS(gpioreg->RF_SW_OUT_SEL, 0xf, 0);
		SET_BITS(gpioreg->RF_SW_OUT_SEL, gpio_num, 0);
		break;
	case GPIO_ALT_FUNC_RF_SW2:
		CLEAR_BITS(gpioreg->RF_SW_OUT_SEL, 0xf, 4);
		SET_BITS(gpioreg->RF_SW_OUT_SEL, gpio_num, 4);
		break;
	case GPIO_ALT_FUNC_UART0_TXDOE:
		CLEAR_BITS(gpioreg->UART_OUT_SEL, 0xf, 0);
		SET_BITS(gpioreg->UART_OUT_SEL, gpio_num, 0);
		break;
	case GPIO_ALT_FUNC_UART1_TXDOE:
		CLEAR_BITS(gpioreg->UART_OUT_SEL, 0xf, 4);
		SET_BITS(gpioreg->UART_OUT_SEL, gpio_num, 4);
		break;
	case GPIO_ALT_FUNC_UART2_TXDOE:
		CLEAR_BITS(gpioreg->UART_OUT_SEL, 0xf, 8);
		SET_BITS(gpioreg->UART_OUT_SEL, gpio_num, 8);
		break;
	default:
		return -2;
	}

	SET_BITS(gpioreg->ALT_FUNC_OUT_EN, 0x1, alt_func);
	return 0;
}



/******************************************************************************
 *  GPIO_GET_ALT_FUNC ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
INT32 GPIO_GET_ALT_FUNC(HANDLE handler, GPIO_ALT_FUNC_TYPE alt_func, UINT32 * regVal) {

	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef	*gpioreg;

	if(handler == NULL) {
		return -1;
	}

	gpio = (GPIO_HANDLER_TYPE *)handler ;
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);

	switch (alt_func) {
	case GPIO_ALT_FUNC_PWM_OUT0:
		*regVal = EXTRACT_BITS(gpioreg->PWM_OUT_SEL, 0xf, 0);
		break;
	case GPIO_ALT_FUNC_PWM_OUT1:
		*regVal = EXTRACT_BITS(gpioreg->PWM_OUT_SEL, 0xf, 4);
		break;
	case GPIO_ALT_FUNC_PWM_OUT2:
		*regVal = EXTRACT_BITS(gpioreg->PWM_OUT_SEL, 0xf, 8);
		break;
	case GPIO_ALT_FUNC_PWM_OUT3:
		*regVal = EXTRACT_BITS(gpioreg->PWM_OUT_SEL, 0xf, 12);
		break;
	case GPIO_ALT_FUNC_EXT_INTR:
		*regVal = EXTRACT_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 0);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB1:
		*regVal = EXTRACT_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 4);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB2:
		*regVal = EXTRACT_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 8);
		break;
	case GPIO_ALT_FUNC_MSPI_CSB3:
		*regVal = EXTRACT_BITS(gpioreg->MSPI_CS_OUT_SEL, 0xf, 12);
		break;
	case GPIO_ALT_FUNC_RF_SW1:
		*regVal = EXTRACT_BITS(gpioreg->RF_SW_OUT_SEL, 0xf, 0);
		break;
	case GPIO_ALT_FUNC_RF_SW2:
		*regVal = EXTRACT_BITS(gpioreg->RF_SW_OUT_SEL, 0xf, 4);
		break;
	case GPIO_ALT_FUNC_UART0_TXDOE:
		*regVal = EXTRACT_BITS(gpioreg->UART_OUT_SEL, 0xf, 0);
		break;
	case GPIO_ALT_FUNC_UART1_TXDOE:
		*regVal = EXTRACT_BITS(gpioreg->UART_OUT_SEL, 0xf, 4);
		break;
	case GPIO_ALT_FUNC_UART2_TXDOE:
		*regVal = EXTRACT_BITS(gpioreg->UART_OUT_SEL, 0xf, 8);
		break;
	default:
		return -2;
	}

	return 0;
}
