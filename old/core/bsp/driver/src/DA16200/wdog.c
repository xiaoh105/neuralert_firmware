/**
 ****************************************************************************************
 *
 * @file wdog.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "sys_nvic.h"
#include "driver.h"
#include "common_def.h"
#if defined(__SUPPORT_SYS_WATCHDOG__)
#include "da16x_sys_watchdog.h"
#endif // __SUPPORT_SYS_WATCHDOG__

//---------------------------------------------------------
//
//---------------------------------------------------------

#undef	SUPPORT_WDOG_CLOCKGATING

//---------------------------------------------------------
//
//---------------------------------------------------------

#define	WDOG_PRINTF(...)	//DRV_PRINTF( __VA_ARGS__ )

#define	WDOG_LOCK_VALUE		0x1ACCE551

//---------------------------------------------------------
//
//---------------------------------------------------------

#ifdef	SUPPORT_WDOG_CLOCKGATING
#define	WDOG_CLOCKGATE_ACTIVATE(wdog)			{		\
		DA16X_CLKGATE_ON(clkoff_watchdog);			\
	}

#define	WDOG_CLOCKGATE_DEACTIVATE(wdog)			{		\
		DA16X_CLKGATE_OFF(clkoff_watchdog);			\
	}

#else	//SUPPORT_WDOG_CLOCKGATING
#define	WDOG_CLOCKGATE_ACTIVATE(wdog)
#define	WDOG_CLOCKGATE_DEACTIVATE(wdog)
#endif	//SUPPORT_WDOG_CLOCKGATING

//---------------------------------------------------------
//	DRIVER :: Declaration
//---------------------------------------------------------

static int	wdog_isr_create(HANDLE handler);
static int	wdog_isr_init(HANDLE handler);
static int	wdog_isr_close(HANDLE handler);
static int	wdog_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler);

static void 	_wdog_interrupt(void);
#if 0	/* Change task kicking */
static void 	_wdog_pll_callback(UINT32 clock, void *param);
#endif /*0*/

//---------------------------------------------------------
//	DRIVER :: internal variables
//---------------------------------------------------------

static int			_wdog_instance[1] ;
static WDOG_HANDLER_TYPE	*_wdog_handler[1] ;
#define	WDOG_INSTANCE		0	/*wdog->dev_unit*/

WDOG_HANDLER_TYPE	wdog_handler;
CLOCK_LIST_TYPE		clock_list;

/******************************************************************************
 *  WDOG_CREATE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	WDOG_CREATE( UINT32 dev_type )
{
	WDOG_HANDLER_TYPE	*wdog;

	// Allocate
	if( _wdog_handler[dev_type] == NULL ){
		wdog = &wdog_handler;
		DRV_MEMSET(wdog, 0, sizeof(WDOG_HANDLER_TYPE));
	}else{
		wdog = _wdog_handler[dev_type];
	}

	// Clear

	// Address Mapping
	switch((WDOG_UNIT_TYPE)dev_type){
		case	WDOG_UNIT_0:
			wdog->dev_unit	 = WDOG_INSTANCE ;
			wdog->dev_addr	 = (UINT32)CMSDK_WATCHDOG ;
			wdog->instance	 = (UINT32)(_wdog_instance[WDOG_INSTANCE]) ;
			_wdog_instance[WDOG_INSTANCE] ++;
			break;

		default:
			DRV_DBG_ERROR("ilegal unit number");
			return NULL;
	}

	if(wdog->instance == 0){
		_wdog_handler[dev_type] = wdog;
		//wdog->setvalue = (DA16X_SYSTEM_XTAL) ;
		wdog->setvalue = (DA16X_SYSTEM_XTAL*50) ;		/* F_F_S */
		wdog->force = FALSE ;
		wdog->wdogbreak = FALSE;

		wdog->clkmgmt = &clock_list;

		WDOG_CLOCKGATE_ACTIVATE(wdog);
		// registry default ISR
		wdog_isr_create(wdog);
	}

	return (HANDLE) wdog;
}

/******************************************************************************
 *  WDOG_INIT ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	WDOG_INIT (HANDLE handler)
{
	WDOG_HANDLER_TYPE	*wdog;

	int ret = FALSE;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	wdog = (WDOG_HANDLER_TYPE *)handler;

	// Init Condition Check
	if( wdog->instance == 0 ){
		// Create HISR
		ret = wdog_isr_init(wdog);

#if 0	/* Change task kicking */
		// Registry SysClock Callback
		((CLOCK_LIST_TYPE *)(wdog->clkmgmt))->callback.func = _wdog_pll_callback;
		((CLOCK_LIST_TYPE *)(wdog->clkmgmt))->callback.param = handler;
		((CLOCK_LIST_TYPE *)(wdog->clkmgmt))->next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, ((CLOCK_LIST_TYPE *)(wdog->clkmgmt)));
#endif
	}
	return ret;
}

/******************************************************************************
 *  WDOG_IOCTL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
int wdog_off = 0;
int WDOG_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	WDOG_HANDLER_TYPE	*wdog;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	if(wdog_off) {

		if(cmd == WDOG_SET_ON) {
			cmd =  WDOG_SET_ENABLE;
			wdog_off = 0;
		} else
			return TRUE;
	}
	wdog = (WDOG_HANDLER_TYPE *)handler;

	switch(cmd) {
		case	WDOG_SET_ENABLE:
			/* Unlocking */
			CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;

			if(data == NULL || ((UINT32 *)data)[0] == FALSE ) {
				/* Set to reset generation */
				CMSDK_WATCHDOG->LOAD = wdog->setvalue ;
				CMSDK_WATCHDOG->CTRL = 	 CMSDK_Watchdog_CTRL_RESEN_Msk
							| CMSDK_Watchdog_CTRL_INTEN_Msk ;
			}
			else {
				/* Set to NMI generation */
				CMSDK_WATCHDOG->LOAD = wdog->setvalue ;
				CMSDK_WATCHDOG->CTRL = CMSDK_Watchdog_CTRL_INTEN_Msk ;
			}
			/* Locking */
			CMSDK_WATCHDOG->LOCK = 0;
			break;

		case	WDOG_SET_DISABLE:
			/* Unlocking */
			CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;
			/* Disable All functions */
			CMSDK_WATCHDOG->CTRL = 0;
			/* Locking */
			CMSDK_WATCHDOG->LOCK = 0;
			break;

		case	WDOG_SET_COUNTER:
			wdog->setvalue     = *((UINT32 *)data);
			break;

		case	WDOG_SET_RESET:
			if ( data == NULL ) { // if Abnormal Setting
				wdog->force = TRUE;
			}
			else {
				wdog->force = FALSE;
			}
			break;

		case	WDOG_SET_ABORT:
			if( data == NULL ) { // if Abnormal Setting
				wdog->wdogbreak = FALSE;
			}else{
				wdog->wdogbreak = TRUE;
			}
			break;

		case	WDOG_GET_STATUS:
			if(data != NULL ){
				if ( (CMSDK_WATCHDOG->CTRL == 0) ) {
					((UINT32 *)data)[0] = FALSE;
				}
				else {
					((UINT32 *)data)[0] = TRUE;
				}
			}
			break;

		case	WDOG_SET_CALLACK:
			wdog_callback_registry( (HANDLE)wdog, ((UINT32 *)data)[0], ((UINT32 *)data)[1], ((UINT32 *)data)[2] );
			break;
		case WDOG_SET_OFF:
			/* Unlocking */
			CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;
			/* Disable All functions */
			CMSDK_WATCHDOG->CTRL = 0;
			/* Locking */
			CMSDK_WATCHDOG->LOCK = 0;
			wdog_off = 1;

			break;

	default:
			return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *  WDOG_READ ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int WDOG_READ (HANDLE handler, UINT32 addr, UINT32 *p_data, UINT32 p_dlen)
{
	DA16X_UNUSED_ARG(addr);
	DA16X_UNUSED_ARG(p_dlen);

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	*((UINT32 *)p_data) = CMSDK_WATCHDOG->VALUE ;

	return sizeof(UINT32);
}

/******************************************************************************
 *  WDOG_CLOSE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	WDOG_CLOSE(HANDLE handler)
{
	WDOG_HANDLER_TYPE	*wdog;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	wdog = (WDOG_HANDLER_TYPE *)handler;

	if ( _wdog_instance[WDOG_INSTANCE] > 0) {
		_wdog_instance[WDOG_INSTANCE] -- ;

		if(_wdog_instance[WDOG_INSTANCE] == 0) {
			wdog_isr_close(wdog);

			WDOG_CLOCKGATE_DEACTIVATE(wdog);

			// Unregistry SysClock Callback
			_sys_clock_ioctl( SYSCLK_RESET_CALLACK
					, ((CLOCK_LIST_TYPE *)(wdog->clkmgmt)));

			_wdog_handler[WDOG_INSTANCE] = NULL;
		}
	}

	return TRUE;
}

/******************************************************************************
 *  wdog_callback_registry ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	 wdog_callback_registry(HANDLE handler, UINT32 index, UINT32 callback, UINT32 userhandler)
{
	WDOG_HANDLER_TYPE	*wdog;

	if(handler == NULL){
		return FALSE;
	}

	wdog = (WDOG_HANDLER_TYPE *) handler ;

	if( index == 0 ){
		wdog->callback[index].func  = (void (*)(void *)) callback;
		wdog->callback[index].param = (HANDLE)userhandler	;
		return TRUE;
	}

	return FALSE;
}

/******************************************************************************
 *  wdog_isr_create ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	wdog_isr_create(HANDLE handler)
{
	WDOG_HANDLER_TYPE	*wdog;

	if(handler == NULL){
		return FALSE;
	}

	wdog = (WDOG_HANDLER_TYPE *) handler ;

	/* Unlocking */
	CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;
	/* Disable All functions */
	CMSDK_WATCHDOG->CTRL = 0;
	/* Locking */
	CMSDK_WATCHDOG->LOCK = 0;

	wdog->callback[0].func   = NULL;
	wdog->callback[0].param  = NULL;

	return TRUE;
}

/******************************************************************************
 *  wdog_isr_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	wdog_isr_init(HANDLE handler)
{
	WDOG_HANDLER_TYPE	*wdog;

	if(handler == NULL){
		return FALSE;
	}

	wdog = (WDOG_HANDLER_TYPE *) handler ;

	_sys_nvic_write(NonMaskableInt_IRQn, (void *)_wdog_interrupt, 0);

	/* Unlocking */
	CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;

	CMSDK_WATCHDOG->LOAD = wdog->setvalue ;
	CMSDK_WATCHDOG->CTRL = 	 CMSDK_Watchdog_CTRL_RESEN_Msk | CMSDK_Watchdog_CTRL_INTEN_Msk ;


	/* Locking */
	CMSDK_WATCHDOG->LOCK = 0;

	return TRUE;
}

/******************************************************************************
 *  wdog_isr_close ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	wdog_isr_close(HANDLE handler)
{
	DA16X_UNUSED_ARG(handler);

	/* Unlocking */
	CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;
	/* Disable All functions */
	CMSDK_WATCHDOG->CTRL = 0;
	/* Locking */
	CMSDK_WATCHDOG->LOCK = 0;

	_sys_nvic_write(NonMaskableInt_IRQn, NULL, 0);
        return TRUE;
}

/******************************************************************************
 *  _wdog_interrupt ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifndef	__da16x_oops_h__

#define	OOPS_MARK		0x4F4F5053 /*OOPS*/
#define OOPS_LEN_MASK		0x00000FFF
#define OOPS_MOD_SHFT		12
#define	OOPS_STACK_TAG		((UINT32 *)(DA16X_SRAM_BASE|0x00E00))
#define	OOPS_STACK_CTXT		((UINT32 *)(DA16X_SRAM_BASE|0x00E10))

typedef 	struct _oops_taginfo_	OOPS_TAG_TYPE;

struct	_oops_taginfo_ {
	UINT32	tag;
	UINT16	mark;		// Application Mark
	UINT16	length;		// Dump Length
	UINT32	rtc[2];		// RTC Time Stamp
};

#endif	//__sys_oops_h__

void _da16x_watchdog_isr(void)
{
	if(_wdog_handler[0] != NULL ){

		if(_wdog_handler[0]->force == TRUE ){
			OOPS_TAG_TYPE *oopstag;

			// We can't distinguish between Wdog Reset and Hard Fault.
			//SYSSET_SWVERSION(0xFFFFC0EF);		// WDOG Break
			SYSSET_SWVERSION(0xFFFFC000);       /* only reset */

			oopstag = (OOPS_TAG_TYPE *)OOPS_STACK_TAG;
			oopstag->tag  	 = OOPS_MARK; /*OOPS*/
			oopstag->length = (5<<OOPS_MOD_SHFT) ;

#if defined(__SUPPORT_SYS_WATCHDOG__)
            da16x_sys_watchdog_display_info();

            __asm(
                "   tst    lr, #4              \n"
                "   ite    eq                  \n"
                "   mrseq  r0, msp             \n"
                "   mrsne  r0, psp             \n"
                "   push   {r4-r11,lr}         \n"
                "   mov    r1, sp              \n"
                "   mov    r2, #5              \n"
                "   bl     print_watchdog_handler \n"
            );
#endif // __SUPPORT_SYS_WATCHDOG__

			while(1) { WDOG_PRINTF("[Wdog]"); }
		}
		else {
			/* Unlocking */
			CMSDK_WATCHDOG->LOCK = WDOG_LOCK_VALUE;
			/* Clear watchdog interrupt request */
			CMSDK_WATCHDOG->INTCLR = CMSDK_Watchdog_INTCLR_Msk;
			/* Locking */
			CMSDK_WATCHDOG->LOCK = 0;
		}

		Printf(" [Wdog] callback.func 0x%x \n", _wdog_handler[0]->callback[0].func);

		if( _wdog_handler[0]->callback[0].func != NULL ){
			_wdog_handler[0]->callback[0].func( _wdog_handler[0]->callback[0].param );
		}
	}
}

void    wdog_debug_fn()
{
    WDOG_HANDLER_TYPE *wdog;

	Printf(RED_COLOR " [%s] Occur...... " CLEAR_COLOR, __func__);
    wdog = WDOG_CREATE(WDOG_UNIT_0);
	WDOG_IOCTL(wdog, WDOG_SET_OFF, NULL );
	Printf(" WATCHDOG off \n");
}

#undef	FOR_WATCHDOG_DEBUG		// FOR_DEBUGGING
static void _wdog_interrupt(void)
{
#ifdef FOR_WATCHDOG_DEBUG
    traceISR_ENTER();
    INTR_CNTXT_CALL(wdog_debug_fn);
    traceISR_EXIT();
#else
#if defined(WATCH_DOG_ENABLE)
	__asm(
		"   tst    lr, #4              \n"
		"   ite    eq                  \n"
		"   mrseq  r0, msp             \n"
		"   mrsne  r0, psp             \n"
		"   push   {r4-r11,lr}         \n"
		"   mov    r1, sp              \n"
		"   mov    r2, #5              \n"
		"   bl     print_watchdog_handler \n"
	);
#endif // WATCH_DOG_ENABLE
	_da16x_watchdog_isr();
#endif
}

/******************************************************************************
 *  _wdog_pll_callback ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#if 0	/* Change task kicking */
static void _wdog_pll_callback(UINT32 clock, void *param)
{
#ifdef WATCH_DOG_ENABLE
	WDOG_HANDLER_TYPE	*wdog;
	UINT32	ioctldata;

	if( param == NULL || clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	wdog = (WDOG_HANDLER_TYPE *) param ;

	WDOG_IOCTL(wdog, WDOG_GET_STATUS, &ioctldata );
	if(ioctldata == TRUE){
		WDOG_IOCTL(wdog, WDOG_SET_DISABLE, NULL );
		WDOG_IOCTL(wdog, WDOG_SET_COUNTER, &clock ); // update
		WDOG_IOCTL(wdog, WDOG_SET_ENABLE, NULL );
	}
#endif	/* WATCH_DOG_ENABLE */
}
#endif /*0*/

void wdt_on()
{
	WDOG_HANDLER_TYPE *wdog;

	wdog = WDOG_CREATE(WDOG_UNIT_0);
	WDOG_INIT(wdog);

	WDOG_IOCTL(wdog, WDOG_SET_ON, NULL );
	PRINTF("WATCHDOG on\n");

	//WDOG_CLOSE(wdog);
}

void wdt_off()
{
	WDOG_HANDLER_TYPE *wdog;

	wdog = WDOG_CREATE(WDOG_UNIT_0);
	WDOG_INIT(wdog);

	WDOG_IOCTL(wdog, WDOG_SET_OFF, NULL );
	PRINTF("WATCHDOG off\n");

	WDOG_CLOSE(wdog);
}

int rescaleTime = 0;
void set_wdt_rescale_time(int rescale_time)
{
	rescaleTime = rescale_time;
	PRINTF("Set WATCHDOG time: %d sec \n", rescaleTime);
}
