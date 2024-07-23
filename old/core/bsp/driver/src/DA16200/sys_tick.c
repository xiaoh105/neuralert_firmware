/**
 ****************************************************************************************
 *
 * @file sys_tick.c
 *
 * @brief SysTick Driver
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
#include "sys_tick.h"
#include "driver.h"

/******************************************************************************
 *
 *  Local Definitions & Declaration
 *
 ******************************************************************************/

#define	CLOCK_SCALE	1000
#define TICK_SCALE	10


static CLOCK_LIST_TYPE	sys_tick_pll_info;
static UINT32 sysclock;
static UINT32 tickscale;

static void _sys_tick_pll_callback(UINT32 clock, void *param);

/******************************************************************************
 *  _sys_tick_create ( )
 *
 *  Purpose :   
 *  Input   :  clock 
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void _sys_tick_create( UINT clock )
{
	tickscale = TICK_SCALE;
	sysclock = clock / CLOCK_SCALE;
}

/******************************************************************************
 *  _sys_tick_init ( )
 *
 *  Purpose :   
 *  Input   :  msec 
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC 
void _sys_tick_init( UINT clock, void *systick)
{
	sysclock = clock / CLOCK_SCALE;

	SysTick->LOAD  = (tickscale * sysclock) - 1;   
	//NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
	SysTick->VAL   = 0;
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
					SysTick_CTRL_TICKINT_Msk   |
					SysTick_CTRL_ENABLE_Msk;    

	sys_tick_pll_info.callback.func = _sys_tick_pll_callback;
	sys_tick_pll_info.callback.param = &sysclock;
	sys_tick_pll_info.next = NULL;

	_sys_clock_ioctl( SYSCLK_SET_CALLACK, &sys_tick_pll_info);

	if( systick != NULL ){
		_sys_nvic_write(SysTick_IRQn, (void *)systick , 0);
	}
}

/******************************************************************************
 *  _sys_tick_close ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void _sys_tick_close(void)
{
  	SysTick->CTRL  = 0; 	
	_sys_clock_ioctl( SYSCLK_RESET_CALLACK, &sys_tick_pll_info);
	_sys_nvic_write(SysTick_IRQn, NULL, 0);
}

/******************************************************************************
 *  _sys_tick_ioctl ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int _sys_tick_ioctl(int cmd, void *data)
{
	switch(cmd){
		case TICK_CMD_GETUSEC:
			if( data != NULL ){
				*(UINT32 *)data = SysTick->VAL ;
			}
			break;
		case TICK_CMD_GETSCALE:
			if( data != NULL ){
				((UINT32 *)data)[0] = tickscale;
				((UINT32 *)data)[1] = CLOCK_SCALE;
				((UINT32 *)data)[2] = SysTick->LOAD;
			}
			break;
		case TICK_CMD_SETSCALE:
			_sys_tick_close();
			tickscale = *(UINT32 *)data;
			_sys_tick_init( (sysclock * CLOCK_SCALE), NULL);
			break;
	}
	return TRUE;
}


/******************************************************************************
 *  _sys_tick_pll_callback ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
static void _sys_tick_pll_callback(UINT32 clock, void *param)
{
	UINT32 bkup;

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	if( (clock/CLOCK_SCALE) != *((UINT32 *)param) ){
	  	bkup = SysTick->CTRL;  
		SysTick->CTRL = 0;
		
		sysclock = clock/CLOCK_SCALE;

		//SysTick_Config( (TICK_SCALE * sysclock) ); 
		/* set reload register */
		SysTick->LOAD  = (tickscale * sysclock) - 1;  
		/* set Priority for Systick Interrupt */
		//NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
		SysTick->VAL   = 0;  		

		SysTick->CTRL = bkup;
	}
}
