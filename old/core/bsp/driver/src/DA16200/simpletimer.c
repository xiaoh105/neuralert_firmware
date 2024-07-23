/**
 ****************************************************************************************
 *
 * @file simpletimer.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "sys_nvic.h"
#include "common_def.h"

//---------------------------------------------------------
//
//---------------------------------------------------------

#undef	SUPPORT_STIMER_CLOCKGATING

//---------------------------------------------------------
//
//---------------------------------------------------------

#define	SSTIMER_PRINTF(...)	DRV_PRINTF( __VA_ARGS__ )

//---------------------------------------------------------
//
//---------------------------------------------------------

#ifdef	SUPPORT_STIMER_CLOCKGATING
#define	STIMER_CLOCKGATE_ACTIVATE(stimer)			{		\
		DA16X_CLKGATE_ON(clkoff_timer[stimer->dev_unit]);		\
	}

#define	STIMER_CLOCKGATE_DEACTIVATE(stimer)			{		\
		DA16X_CLKGATE_OFF(clkoff_timer[stimer->dev_unit]);		\
	}

#else	//SUPPORT_STIMER_CLOCKGATING
#define	STIMER_CLOCKGATE_ACTIVATE(stimer)
#define	STIMER_CLOCKGATE_DEACTIVATE(stimer)
#endif	//SUPPORT_STIMER_CLOCKGATING

//---------------------------------------------------------
//	DRIVER :: Declaration
//---------------------------------------------------------

static int	stimer_isr_create(HANDLE handler);
static int	stimer_isr_init(HANDLE handler);
static int	stimer_isr_close(HANDLE handler);
static int	stimer_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler);
static void 	_sys_stimer_pll_callback(UINT32 clock, void *param);

void		_simpletimer_interrupt(UINT32 idx);
static void	_lowlevel_simpletimer0_interrupt(void);
static void	_lowlevel_simpletimer1_interrupt(void);


//---------------------------------------------------------
//	DRIVER :: internal variables
//---------------------------------------------------------

static int			_stimer_instance[STIMER_UNIT_MAX] ;
static STIMER_HANDLER_TYPE	*_stimer_handler[STIMER_UNIT_MAX] ;

/******************************************************************************
 *  STIMER_CREATE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	STIMER_CREATE( UINT32 dev_type )
{
	STIMER_HANDLER_TYPE	*timer;

	// Allocate
	if( dev_type >= STIMER_UNIT_MAX ){
		DRV_DBG_ERROR("STIMER, ilegal unit number");
		return NULL;
	}

	if( _stimer_handler[dev_type] == NULL ){
		timer = (STIMER_HANDLER_TYPE *)pvPortMalloc(sizeof(STIMER_HANDLER_TYPE));
		if( timer == NULL ){
			return NULL;
		}
		DRV_MEMSET(timer, 0, sizeof(STIMER_HANDLER_TYPE));
	}else{
		timer = _stimer_handler[dev_type];
	}

	// Address Mapping
	switch((STIMER_UNIT_TYPE)dev_type){
		case	STIMER_UNIT_0:
			timer->dev_unit  = dev_type ;
			timer->dev_addr  = (UINT32)CMSDK_TIMER0 ;
			timer->instance  = (UINT32)(_stimer_instance[dev_type]) ;
			_stimer_instance[dev_type] ++;
			break;

		case	STIMER_UNIT_1:
			timer->dev_unit  = dev_type ;
			timer->dev_addr  = (UINT32)CMSDK_TIMER1 ;
			timer->instance  = (UINT32)(_stimer_instance[dev_type]) ;
			_stimer_instance[dev_type] ++;
			break;

		default:
			break;
	}

	//DRV_DBG_INFO("(%p) TIMER create, base %p", timer, (UINT32 *)timer->dev_addr );

	if(_stimer_instance[timer->dev_unit] == 1){
		_stimer_handler[dev_type] = timer;

		//timer->clkmgmt = (void *)HAL_MALLOC(sizeof(CLOCK_LIST_TYPE));
		timer->clkmgmt = (void *)pvPortMalloc(sizeof(CLOCK_LIST_TYPE));
		// registry default ISR
		STIMER_CLOCKGATE_ACTIVATE(timer);
		stimer_isr_create(timer);
	}else{
		timer->clkmgmt = _stimer_handler[dev_type]->clkmgmt ;
	}

	return (HANDLE) timer;
}

/******************************************************************************
 *  STIMER_INIT ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	STIMER_INIT (HANDLE handler)
{
	STIMER_HANDLER_TYPE	*timer;

	int ret = FALSE;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	timer = (STIMER_HANDLER_TYPE *)handler;

	// Init Condition Check
	if( _stimer_instance[timer->dev_unit] == 1 ){
		// Create HISR
		ret = stimer_isr_init(timer);
		// Sys Clock
		((CLOCK_LIST_TYPE *)(timer->clkmgmt))->callback.func = _sys_stimer_pll_callback;
		((CLOCK_LIST_TYPE *)(timer->clkmgmt))->callback.param = handler;
		((CLOCK_LIST_TYPE *)(timer->clkmgmt))->next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, ((CLOCK_LIST_TYPE *)(timer->clkmgmt)));
	}
	return ret;
}

/******************************************************************************
 *  STIMER_IOCTL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	STIMER_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	timer = (STIMER_HANDLER_TYPE *)handler;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);

	switch(cmd){
		case	STIMER_GET_TICK:
			*((UINT32 *)data)   = timer->tickcount;
			break;

		case	STIMER_SET_MODE:
			timerreg->CTRL 	= *((UINT32 *)data);
			break;

		case	STIMER_GET_MODE:
			*((UINT32 *)data)   = timerreg->CTRL;
			break;

		case	STIMER_SET_LOAD:
			if( data  == NULL || ((UINT32 *)data)[1] == 0 ){
				/* if data[1] == 0 , divied-by-zero error */
				return FALSE;
			}

			timer->setmode = STIMER_SET_LOAD;
			timer->clock = ((UINT32 *)data)[0];
			timer->divider = ((UINT32 *)data)[1];

			timerreg->RELOAD = (timer->clock)/(timer->divider);
			timerreg->VALUE = (timer->clock)/(timer->divider);
			break;

		case	STIMER_SET_UTIME:
			if( data  == NULL || ((UINT32 *)data)[1] == 0 ){
				/* if data[1] == 0 , divied-by-zero error */
				return FALSE;
			}

			timer->setmode = STIMER_SET_UTIME;
			timer->clock = ( ((UINT32 *)data)[0] / 1000000UL );
			timer->divider = ((UINT32 *)data)[1];

			timerreg->RELOAD = (timer->clock)*(timer->divider);
			timerreg->VALUE = (timer->clock)*(timer->divider);
			break;

		case	STIMER_GET_UTIME:
			/* return remain value register*/
			if (!timer->clock)
				return FALSE;
			*((UINT32 *)data)   = timerreg->VALUE / timer->clock;
			break;

		case	STIMER_SET_ACTIVE:
			timerreg->CTRL |= CMSDK_TIMER_CTRL_EN_Msk ;
			break;

		case	STIMER_SET_DEACTIVE:
			timerreg->CTRL &= ~CMSDK_TIMER_CTRL_EN_Msk ;
			break;

		case	STIMER_SET_CALLACK:
			return stimer_callback_registry( (HANDLE)timer
							, ((UINT32 *)data)[0]
							, ((UINT32 *)data)[1]
							, ((UINT32 *)data)[2]  );

		default:
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *  STIMER_READ ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int STIMER_READ (HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen)
{
	DA16X_UNUSED_ARG(addr);
	DA16X_UNUSED_ARG(p_dlen);

	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	timer = (STIMER_HANDLER_TYPE *)handler ;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);

	*p_data = timerreg->VALUE ;

	return sizeof(UINT32);
}

/******************************************************************************
 *  STIMER_CLOSE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	STIMER_CLOSE(HANDLE handler)
{
	STIMER_HANDLER_TYPE	*timer;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	timer = (STIMER_HANDLER_TYPE *)handler;

	if ( _stimer_instance[timer->dev_unit] > 0) {
		_stimer_instance[timer->dev_unit] -- ;

		if(_stimer_instance[timer->dev_unit] == 0) {
			// Sys Clock
			_sys_clock_ioctl( SYSCLK_RESET_CALLACK
					, ((CLOCK_LIST_TYPE *)(timer->clkmgmt)));

			vPortFree( timer->clkmgmt );
			timer->clkmgmt = NULL;

			// Close ISR
			stimer_isr_close(timer);

			STIMER_CLOCKGATE_DEACTIVATE(timer);

			vPortFree(timer);
			_stimer_handler[timer->dev_unit] = NULL;
		}
	}

	return TRUE;
}

/******************************************************************************
 *  stimer_isr_create ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	stimer_isr_create(HANDLE handler)
{
	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;

	if(handler == NULL){
		return FALSE;
	}

	timer = (STIMER_HANDLER_TYPE *) handler ;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);


	// Disable Timer
	timerreg->CTRL &= ~(CMSDK_TIMER_CTRL_EN_Msk|CMSDK_TIMER_CTRL_IRQEN_Msk);
	timerreg->INTCLEAR = CMSDK_TIMER_INTCLEAR_Msk;

	return TRUE;

}

/******************************************************************************
 *  stimer_isr_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	stimer_isr_init(HANDLE handler)
{
	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;

	if(handler == NULL){
		return FALSE;
	}

	timer = (STIMER_HANDLER_TYPE *) handler ;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);

	// Registry LISR
	switch(timer->dev_unit){
		case	STIMER_UNIT_0:
			_sys_nvic_write(TIMER0_IRQn, (void *)_lowlevel_simpletimer0_interrupt, 0x7);
			break;
		case	STIMER_UNIT_1:
			_sys_nvic_write(TIMER1_IRQn, (void *)_lowlevel_simpletimer1_interrupt, 0x7);
			break;

	}

	// Enable Timer
	timerreg->CTRL |= CMSDK_TIMER_CTRL_EN_Msk;

	return TRUE;
}

/******************************************************************************
 *  stimer_isr_close ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	stimer_isr_close(HANDLE handler)
{
	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;

	if(handler == NULL){
		return FALSE;
	}

	timer = (STIMER_HANDLER_TYPE *) handler ;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);

	// Disable All Interrupts
	timerreg->CTRL &= ~CMSDK_TIMER_CTRL_EN_Msk;
	timerreg->INTCLEAR = CMSDK_TIMER_INTCLEAR_Msk;

	// UN-Registry LISR
	switch(timer->dev_unit){
		case	STIMER_UNIT_0:
			_sys_nvic_write(TIMER0_IRQn, NULL, 0x7);
			break;
		case	STIMER_UNIT_1:
			_sys_nvic_write(TIMER1_IRQn, NULL, 0x7);
			break;
	}

	return TRUE;

}

/******************************************************************************
 *  stimer_callback_registry ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	stimer_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler)
{
	STIMER_HANDLER_TYPE	*timer;

	if(handler == NULL){
		return FALSE;
	}

	timer = (STIMER_HANDLER_TYPE *) handler ;

	if( index < INT_MAX_STIMER_VEC ){
		timer->callback[index].func  = (void (*)(void *)) callback;
		timer->callback[index].param = (HANDLE)userhandler	;
		return TRUE;
	}

	return FALSE;
}

/******************************************************************************
 *  _simpletimer_interrupt ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void _simpletimer_interrupt(UINT32 idx)
{
	CMSDK_TIMER_TypeDef *timerreg;
#if 0
	if( OAL_GET_KERNEL_STATE( ) != TRUE ){
		return;
	}
#endif
	if( _stimer_handler[idx] != NULL ){

		timerreg = (CMSDK_TIMER_TypeDef *)(_stimer_handler[idx]->dev_addr);

		if((timerreg->INTSTATUS & CMSDK_TIMER_INTSTATUS_Msk) != 0 ){
			timerreg->INTCLEAR = CMSDK_TIMER_INTCLEAR_Msk ;

			_stimer_handler[idx]->tickcount ++ ;
			if( _stimer_handler[idx]->callback[0].func != NULL ){
				_stimer_handler[idx]->callback[0].func( _stimer_handler[idx]->callback[0].param );
			}
		}

	}

}

static  void	_lowlevel_simpletimer0_interrupt(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL_PARA(_simpletimer_interrupt, STIMER_UNIT_0);
	traceISR_EXIT();
}

static  void	_lowlevel_simpletimer1_interrupt(void)
{
	INTR_CNTXT_CALL_PARA(_simpletimer_interrupt, STIMER_UNIT_1);
}

/******************************************************************************
 *  _sys_stimer_pll_callback ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void _sys_stimer_pll_callback(UINT32 clock, void *param)
{
	STIMER_HANDLER_TYPE	*timer;
	CMSDK_TIMER_TypeDef *timerreg;
	UINT32 bkup, keep_time;

	if( param == NULL || clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	timer = (STIMER_HANDLER_TYPE *)param;
	timerreg = (CMSDK_TIMER_TypeDef *)(timer->dev_addr);

	bkup = timerreg->CTRL;
	timerreg->CTRL = 0;

	/* change a resolution of timer */
	if( timer->setmode == STIMER_SET_UTIME ){
		keep_time = timerreg->VALUE / timer->clock;
		timer->clock = ( clock / 1000000UL );

		if (keep_time > 0)
		{
			/*timer->setmode = STIMER_SET_UTIME;*/
			timer->divider = keep_time;

			timerreg->RELOAD = (timer->clock)*(timer->divider);
			timerreg->VALUE = (timer->clock)*(timer->divider);
		}
	}else{
		UINT32 newreload, oldvalue, oldreload;

		timer->clock = clock;

		newreload = (timer->clock)/(timer->divider);
		oldvalue  = timerreg->VALUE;
		oldreload  = timerreg->RELOAD;

		timerreg->VALUE = (oldvalue * newreload) / oldreload ;
		timerreg->RELOAD = newreload ;
	}

	timerreg->CTRL = bkup;
}

