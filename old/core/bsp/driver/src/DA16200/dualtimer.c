/**
 ****************************************************************************************
 *
 * @file dualtimer.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "sys_nvic.h"

//---------------------------------------------------------
// 
//---------------------------------------------------------

#undef	SUPPORT_DTIMER_CLOCKGATING

//---------------------------------------------------------
// 
//---------------------------------------------------------

#define	DTIMER_PRINTF(...)	DRV_PRINTF( __VA_ARGS__ )

//---------------------------------------------------------
// 
//---------------------------------------------------------

#ifdef	SUPPORT_DTIMER_CLOCKGATING
#define	DTIMER_CLOCKGATE_ACTIVATE(stimer)			{	\
		 DA16X_CLKGATE_ON(clkoff_dualtimer);			\
	}

#define	DTIMER_CLOCKGATE_DEACTIVATE(stimer)			{	\
		 DA16X_CLKGATE_OFF(clkoff_dualtimer);			\
	}

#else	//SUPPORT_DTIMER_CLOCKGATING
#define	DTIMER_CLOCKGATE_ACTIVATE(stimer)
#define	DTIMER_CLOCKGATE_DEACTIVATE(stimer)
#endif	//SUPPORT_DTIMER_CLOCKGATING

//---------------------------------------------------------
//	DRIVER :: Declaration
//---------------------------------------------------------

static int	dtimer_isr_create(HANDLE handler);
static int	dtimer_isr_init(HANDLE handler);
static int	dtimer_isr_close(HANDLE handler);
static int	dtimer_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler);
static void 	_sys_dtimer_pll_callback(UINT32 clock, void *param);

void		_dtimer_interrupt(void);
static void	_lowlevel_dtimer_interrupt(void);

//---------------------------------------------------------
//	DRIVER :: internal variables  
//---------------------------------------------------------

static int			_dtimer_instance[DTIMER_UNIT_MAX] ;
static DTIMER_HANDLER_TYPE	*_dtimer_handler[DTIMER_UNIT_MAX] ;

/******************************************************************************
 *  DTIMER_CREATE ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
HANDLE	DTIMER_CREATE( UINT32 dev_type )
{
	DTIMER_HANDLER_TYPE	*dtimer;

	// Allocate
	if( dev_type >= DTIMER_UNIT_MAX ){
		DRV_DBG_ERROR("DTIMER, ilegal unit number");
		return NULL;
	}

	if( _dtimer_handler[dev_type] == NULL ){
		dtimer = (DTIMER_HANDLER_TYPE *)pvPortMalloc(sizeof(DTIMER_HANDLER_TYPE));
		if( dtimer == NULL ){
			return NULL;
		}
		DRV_MEMSET(dtimer, 0, sizeof(DTIMER_HANDLER_TYPE));
	}else{
		dtimer = _dtimer_handler[dev_type];
	}
	
	// Address Mapping
	switch((DTIMER_UNIT_TYPE)dev_type){
		case	DTIMER_UNIT_00:
			dtimer->dev_unit  = dev_type ;
			dtimer->dev_addr  = (UINT32)CMSDK_DUALTIMER1 ;
			dtimer->instance  = (UINT32)(_dtimer_instance[dev_type]) ;
			_dtimer_instance[dev_type] ++;
			break;

		case	DTIMER_UNIT_01:
			dtimer->dev_unit = dev_type ;
			dtimer->dev_addr = (UINT32)CMSDK_DUALTIMER2 ;
			dtimer->instance = (UINT32)(_dtimer_instance[dev_type]) ;
			_dtimer_instance[dev_type] ++;
			break;

		default:
			break;
	}

	//DRV_DBG_INFO("(%p) DTIMER create, base %p", dtimer, (UINT32 *)dtimer->dev_addr );

	if(_dtimer_instance[dtimer->dev_unit] == 1){
		_dtimer_handler[dev_type] = dtimer;
		dtimer->clkmgmt = (void *)pvPortMalloc(sizeof(CLOCK_LIST_TYPE));
		// registry default ISR
		DTIMER_CLOCKGATE_ACTIVATE(dtimer);
		dtimer_isr_create(dtimer);
	}else{
		dtimer->clkmgmt = _dtimer_handler[dev_type]->clkmgmt ;
	}

	return (HANDLE) dtimer;
}

/******************************************************************************
 *  DTIMER_INIT ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
int 	DTIMER_INIT (HANDLE handler)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	
	int ret = FALSE;
	
	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	dtimer = (DTIMER_HANDLER_TYPE *)handler;

	// Init Condition Check
	if( _dtimer_instance[dtimer->dev_unit] == 1 ){
		// Create HISR
		ret = dtimer_isr_init(dtimer);
		// Sys Clock
		((CLOCK_LIST_TYPE *)(dtimer->clkmgmt))->callback.func = _sys_dtimer_pll_callback;
		((CLOCK_LIST_TYPE *)(dtimer->clkmgmt))->callback.param = handler;
		((CLOCK_LIST_TYPE *)(dtimer->clkmgmt))->next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, ((CLOCK_LIST_TYPE *)(dtimer->clkmgmt)));
	}
	return ret;
}

/******************************************************************************
 *  DTIMER_IOCTL ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

int	DTIMER_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;
	
	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	dtimer = (DTIMER_HANDLER_TYPE *)handler;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);

	switch(cmd){
		case	DTIMER_GET_TICK:
			*((UINT32 *)data)   = dtimer->tickcount;
			break;

		case	DTIMER_SET_MODE:
			dtimerreg->TimerControl = *((UINT32 *)data) ;
			break;

		case	DTIMER_GET_MODE:
			*((UINT32 *)data)   = dtimerreg->TimerControl;
			break;

		case	DTIMER_SET_LOAD:
			if( data  == NULL || ((UINT32 *)data)[1] == 0 ){
				/* if data[1] == 0 , divied-by-zero error */
				return FALSE;
			}

			dtimer->clock = ((UINT32 *)data)[0];
			dtimer->divider = ((UINT32 *)data)[1];
				
			dtimerreg->TimerLoad = (dtimer->clock)/(dtimer->divider);
			if( dtimerreg->TimerLoad !=  *((UINT32 *)data) )
			{
				return FALSE;
			}
			break;

		case	DTIMER_SET_UTIME:
			if( data  == NULL || ((UINT32 *)data)[1] == 0 ){
				/* if data[1] == 0 , divied-by-zero error */
				return FALSE;
			}

			dtimer->clock = ( ((UINT32 *)data)[0] / 1000000UL );
			dtimer->divider = ((UINT32 *)data)[1];
				
			dtimerreg->TimerLoad = (dtimer->clock)/(dtimer->divider);
			break;

		case	DTIMER_SET_ACTIVE:
			dtimerreg->TimerControl |= CMSDK_DUALTIMER_CTRL_EN_Msk ;
			break;

		case	DTIMER_SET_DEACTIVE:
			dtimerreg->TimerControl &= ~CMSDK_DUALTIMER_CTRL_EN_Msk ;
			break;
	
		// Interrupt Register Access
		case	DTIMER_SET_CALLACK:
			return dtimer_callback_registry( (HANDLE)dtimer
							, ((UINT32 *)data)[0]
							, ((UINT32 *)data)[1]
							, ((UINT32 *)data)[2]  );

		default:
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *  DTIMER_READ ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
int	DTIMER_READ (HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen)
{
	DA16X_UNUSED_ARG(addr);
	DA16X_UNUSED_ARG(p_dlen);

	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	dtimer = (DTIMER_HANDLER_TYPE *)handler ;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);
	
	*p_data = dtimerreg->TimerValue ;

	return sizeof(UINT32);
}

/******************************************************************************
 *  DTIMER_CLOSE ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

int 	DTIMER_CLOSE(HANDLE handler)
{
	DTIMER_HANDLER_TYPE	*dtimer;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	dtimer = (DTIMER_HANDLER_TYPE *)handler;
	
	if ( _dtimer_instance[dtimer->dev_unit] > 0) {
		_dtimer_instance[dtimer->dev_unit] -- ;

		if(_dtimer_instance[dtimer->dev_unit] == 0) {
			// Sys Clock
			_sys_clock_ioctl( SYSCLK_RESET_CALLACK
					, ((CLOCK_LIST_TYPE *)(dtimer->clkmgmt)));

			vPortFree( dtimer->clkmgmt );
			dtimer->clkmgmt = NULL;

			{ 
				DTIMER_HANDLER_TYPE *other;
				if( dtimer->dev_unit == 0 ){
					other = _dtimer_handler[1];
				}else{
					other = _dtimer_handler[0];
				}
				if( other == NULL ){
					// Close ISR
					dtimer_isr_close(dtimer);
				}
			}

			if( _dtimer_instance[DTIMER_UNIT_00] == 0 
			   && _dtimer_instance[DTIMER_UNIT_01] == 0 ){
				DTIMER_CLOCKGATE_DEACTIVATE(dtimer);
			}
			
			vPortFree(dtimer);
			_dtimer_handler[dtimer->dev_unit] = NULL;			
		}
	}

	return TRUE;
}

/******************************************************************************
 *  dtimer_isr_create ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static int	dtimer_isr_create(HANDLE handler)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;
	
	if(handler == NULL){
		return FALSE;
	}
	
	dtimer = (DTIMER_HANDLER_TYPE *) handler ;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);
	

	// Disable All Interrupts
	dtimerreg->TimerControl &= ~(CMSDK_DUALTIMER_CTRL_EN_Msk|CMSDK_DUALTIMER_CTRL_INTEN_Msk);
	dtimerreg->TimerIntClr = CMSDK_DUALTIMER_INTCLR_Msk;
	
	return TRUE;

}

/******************************************************************************
 *  dtimer_isr_init ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static int	dtimer_isr_init(HANDLE handler)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;
	
	if(handler == NULL){
		return FALSE;
	}
	
	dtimer = (DTIMER_HANDLER_TYPE *) handler ;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);

	// Registry LISR
	switch(dtimer->dev_unit){
		case	DTIMER_UNIT_00:
			_sys_nvic_write( DUALTIMER_IRQn, (void *)_lowlevel_dtimer_interrupt , 0x7);
			break;
				
		case	DTIMER_UNIT_01:
			_sys_nvic_write( DUALTIMER_IRQn, (void *)_lowlevel_dtimer_interrupt , 0x7);
			break;

	}

	// Enable Clock
	dtimerreg->TimerControl |= CMSDK_DUALTIMER_CTRL_EN_Msk;

	return TRUE;
}

/******************************************************************************
 *  dtimer_isr_close ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static int	dtimer_isr_close(HANDLE handler)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;
	
	if(handler == NULL){
		return FALSE;
	}
	
	dtimer = (DTIMER_HANDLER_TYPE *) handler ;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);	

	// Disable All Interrupts
	dtimerreg->TimerControl &= ~CMSDK_DUALTIMER_CTRL_EN_Msk;
	dtimerreg->TimerIntClr = CMSDK_DUALTIMER_INTCLR_Msk;
	
	// UN-Registry LISR 
	switch(dtimer->dev_unit){
		case	DTIMER_UNIT_00:
			_sys_nvic_write( DUALTIMER_IRQn, NULL , 0x7);
			break;
		case	DTIMER_UNIT_01:
			_sys_nvic_write( DUALTIMER_IRQn, NULL , 0x7);
			break;
	}

	return TRUE;

}

/******************************************************************************
 *  dtimer_callback_registry ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static int	 dtimer_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	
	if(handler == NULL){
		return FALSE;
	}
	
	dtimer = (DTIMER_HANDLER_TYPE *) handler ;

	if(index < INT_MAX_DTIMER_VEC){
		dtimer->callback[index].func  = (void (*)(void *)) callback;
		dtimer->callback[index].param = (HANDLE)userhandler	;
		return TRUE;
	}
	
	return FALSE;
}

/******************************************************************************
 *  _dtimer_interrupt ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

void _dtimer_interrupt(void)
{
	UINT32		i;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;

	for( i=0; i< DTIMER_UNIT_MAX; i++){
		if( _dtimer_handler[i] != NULL ){		
			dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(_dtimer_handler[i]->dev_addr);	
			if( dtimerreg != NULL 
			 && (dtimerreg->TimerMIS & CMSDK_DUALTIMER_MASKINTSTAT_Msk) != 0 ){
				dtimerreg->TimerIntClr = CMSDK_DUALTIMER_INTCLR_Msk ;

				_dtimer_handler[i]->tickcount ++;
				if( _dtimer_handler[i]->callback[0].func != NULL ){
					_dtimer_handler[i]->callback[0].func( _dtimer_handler[i]->callback[0].param );
				}			
			}
		}
	}
}

static  void	_lowlevel_dtimer_interrupt(void)
{
	traceISR_ENTER();
	_dtimer_interrupt();
	traceISR_EXIT();
}

/******************************************************************************
 *  _sys_dtimer_pll_callback ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static void _sys_dtimer_pll_callback(UINT32 clock, void *param)
{
	DTIMER_HANDLER_TYPE	*dtimer;
	CMSDK_DUALTIMER_SINGLE_TypeDef *dtimerreg;
	UINT32		bkup;
	
	if( param == NULL || clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}
	
	dtimer = (DTIMER_HANDLER_TYPE *)param;
	dtimerreg = (CMSDK_DUALTIMER_SINGLE_TypeDef *)(dtimer->dev_addr);

	dtimer->clock = clock;

	bkup = dtimerreg->TimerControl;
	dtimerreg->TimerControl = 0;	

	/* change a resolution of timer */
	dtimerreg->TimerLoad = (dtimer->clock)/(dtimer->divider);	

	dtimerreg->TimerControl = bkup;
}

