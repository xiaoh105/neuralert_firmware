/**
 ****************************************************************************************
 *
 * @file sys_clock.c
 *
 * @brief Clock Management
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
#include "driver.h"
#include "common_def.h"

/******************************************************************************
 *
 *  Local Definitions & Declaration
 *
 ******************************************************************************/

#undef	SUPPORT_CLKCHG_SLEEP

static	CLOCK_LIST_TYPE *sys_clock_list;
static	UINT32 sys_xtal_clock;	/* scale of 1/KHz */

/******************************************************************************
 *  _sys_clock_create ( )
 *
 *  Purpose :
 *  Input   :  clock
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void	_sys_clock_create(void)
{
	sys_clock_list = NULL;
}

/******************************************************************************
 *  da16x_sysclk_p2x_isr ( )
 *
 *  Purpose :
 *  Input   :  msec
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void da16x_sysclk_p2x_isr(void)
{
	ASIC_DBG_TRIGGER(0xC1050131);
}

ATTRIBUTE_RAM_FUNC
static  void	da16x_sysclk_p2x_interrupt(void)
{
	INTR_CNTXT_CALL(da16x_sysclk_p2x_isr);
}

ATTRIBUTE_RAM_FUNC
void da16x_sysclk_x2p_isr(void)
{
	ASIC_DBG_TRIGGER(0xC1050232);
}

ATTRIBUTE_RAM_FUNC
static  void	da16x_sysclk_x2p_interrupt(void)
{
	INTR_CNTXT_CALL(da16x_sysclk_x2p_isr);
}

ATTRIBUTE_RAM_FUNC
void	_sys_clock_init(UINT32 clock)
{
	_sys_nvic_write(PLL_2_Xtal_IRQn, (void *)da16x_sysclk_p2x_interrupt, 0x7);
	_sys_nvic_write(Xtal_2_PLL_IRQn, (void *)da16x_sysclk_x2p_interrupt, 0x7);

	_sys_clock_ioctl(SYSCLK_SET_XTAL, &clock);
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
void	_sys_clock_close(void)
{
	_sys_nvic_write(PLL_2_Xtal_IRQn, (void *)NULL, 0x7);
	_sys_nvic_write(Xtal_2_PLL_IRQn, (void *)NULL, 0x7);
}

/******************************************************************************
 *  _sys_clock_ioctl ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int	_sys_clock_ioctl(int cmd, void *data)
{
	if( data == NULL ){
		return FALSE;
	}

	switch(cmd){
	case	SYSCLK_SET_XTAL:
		sys_xtal_clock = ((UINT32 *)data)[0];
		sys_xtal_clock = sys_xtal_clock / KHz ;
		break;

	case	SYSCLK_SET_CALLACK:
		if ( data == NULL ) {
			break;
		}

		if ( ((CLOCK_LIST_TYPE *)data)->next == NULL ) {
			((CLOCK_LIST_TYPE *)data)->next = sys_clock_list;
			sys_clock_list = ((CLOCK_LIST_TYPE *)data);
		}
		else {
			CLOCK_LIST_TYPE *clkitm = ((CLOCK_LIST_TYPE *)data)->next;
			while(clkitm->next != NULL){
				clkitm = clkitm->next;
			}
			clkitm->next = sys_clock_list;
			sys_clock_list = ((CLOCK_LIST_TYPE *)data);
		}
		break;

	case	SYSCLK_RESET_CALLACK:
		if( sys_clock_list == ((CLOCK_LIST_TYPE *)data)){
			sys_clock_list = ((CLOCK_LIST_TYPE *)data)->next;
		}else{
			CLOCK_LIST_TYPE *clkitm = sys_clock_list;
			while( clkitm->next != NULL ){
				if( clkitm->next == ((CLOCK_LIST_TYPE *)data) ){
					clkitm->next = ((CLOCK_LIST_TYPE *)data)->next;
					break;
				}else{
					if( clkitm == clkitm->next ){
						// abnormal case
						break;
					}else{
						clkitm = clkitm->next;
					}
				}
			}
		}
		break;

	case	SYSCLK_PRINT_INFO:
		{
			CLOCK_LIST_TYPE *clkitm = sys_clock_list;
			while( clkitm != NULL ){
				if( clkitm->callback.func != NULL ){
					DRV_PRINTF("callback-%p (%p)\n"
						, clkitm
						, clkitm->callback.param );
				}
				clkitm = clkitm->next;
			}
		}
		break;
	}

	return TRUE;
}

/******************************************************************************
 *  _sys_clock_read ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int	_sys_clock_read(UINT32 *clock, int len)
{
	DA16X_UNUSED_ARG(len);

#ifdef  BUILD_OPT_DA16200_ASIC

	if( clock != NULL ){
		*clock = DA16X_SYSTEM_CLOCK;
	}

	*clock = da16x_pll_getspeed(sys_xtal_clock);
	*clock = ( *clock ) * KHz ; // PLL Clock

	// CPU's Clock Divider
	*clock = (*clock) / (DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU + 1);

	return sizeof(UINT32);
#else	//BUILD_OPT_DA16200_ASIC

	if( clock != NULL ){
		*clock = DA16200FPGA_SYSCON->FPGA_PLL_FREQ ;
		*clock = (( *clock ) & 0x0FF) * MHz;
	}

	return sizeof(UINT32);
#endif  //BUILD_OPT_DA16200_ASIC
}

/******************************************************************************
 *  _sys_clock_write ( )
 *
 *  Purpose :
 *  Input   :  0xeeee := down to XTAL, 0xffff := callback
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int	_sys_clock_write(UINT32 *clock, int len)
{
	UINT32	local_c;
	CLOCK_LIST_TYPE *clkitm ;

	if( len == 0xeeee && ((*clock)&CLKMGMT_FREQMSK) == 0 ){
		/* MPW2 Issue Protection : down to XTAL */
		*clock = sys_xtal_clock * KHz ;
	}

	/* before CPU throttling */
	local_c = 0 ;

	/* Pass-1 : Non-Urgent Task */
	clkitm = sys_clock_list;
	while( clkitm != NULL ){
		// Non-Urgent Task
		if( (clkitm->callback.func != NULL)
		    && ((UINT32)(clkitm->callback.param) != CLKMGMT_PRI_URGENT) ){
			clkitm->callback.func( local_c,
				clkitm->callback.param );
		}
		clkitm = clkitm->next;
	}

	/* Pass-2 : Urgent Task     */
	clkitm = sys_clock_list;
	while( clkitm != NULL ){
		// Urgent Task
		if( (clkitm->callback.func != NULL)
		    && ((UINT32)(clkitm->callback.param) == CLKMGMT_PRI_URGENT) ){
			clkitm->callback.func( local_c,
				clkitm->callback.param );
		}
		clkitm = clkitm->next;
	}

	if( *clock == 0 ){
		/* Clock Down */
		return sizeof(UINT32);
	}

	local_c = ((*clock)&CLKMGMT_SRCMSK) | ((*clock)&CLKMGMT_FREQMSK) ;

	if ( (unsigned int) len != 0xFFFFFFFF ) {
#ifdef  BUILD_OPT_DA16200_ASIC
		UINT32 pll_c, div_c ;

		pll_c = da16x_pll_getspeed(sys_xtal_clock) ;
		div_c = (pll_c*KHz) / local_c ;
		div_c = ((local_c * div_c) > pll_c) ? (div_c - 1) : (div_c - 2) ;
		div_c = DA16X_CLK_DIV(div_c);

#ifdef	SUPPORT_CLKCHG_SLEEP
		// 2.  set SRC_CLK_SEL_CPU to change from PLL to XTAL
		DA16200_SYSCLOCK->SRC_CLK_SEL_0_CPU = 0; // XTAL
		// 3.  wait on PLL_2_XTAL  (CPU Sleep?)
		WAIT_FOR_INTR();
		// 4.  disable PLL_CLK_EN_CPU
		DA16200_SYSCLOCK->PLL_CLK_EN_0_CPU = 0;
#endif	//SUPPORT_CLKCHG_SLEEP
		// 5.  setup DIV_CPU to run CPU at target freq.
		DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU = (volatile UINT8)div_c;
#ifdef	SUPPORT_CLKCHG_SLEEP
		// 6.  enable PLL_CLK_EN_CPU
		DA16200_SYSCLOCK->PLL_CLK_EN_0_CPU = 1;
		// 7.  set SRC_CLK_SEL_CPU to switch from XTAL to PLL
		DA16200_SYSCLOCK->SRC_CLK_SEL_0_CPU = 1; // PLL
		// 8.  wait on XTAL_2_PLL  (CPU Sleep?)
		WAIT_FOR_INTR();
#endif	//SUPPORT_CLKCHG_SLEEP

#else	//BUILD_OPT_DA16200_ASIC
		DA16200FPGA_SYSCON->FPGA_PLL_FREQ = (*clock/MHz);
		WAIT_FOR_INTR();
#endif	//BUILD_OPT_DA16200_ASIC
	}

	/* change setting values of drivers */
#ifdef  BUILD_OPT_DA16200_ASIC
	local_c = da16x_pll_getspeed(sys_xtal_clock);
	local_c = (local_c * KHz) / (DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU + 1);
#else	//BUILD_OPT_DA16200_ASIC
	local_c = DA16200FPGA_SYSCON->FPGA_PLL_FREQ;
	local_c = local_c * MHz ;
#endif	//BUILD_OPT_DA16200_ASIC

	local_c = local_c&CLKMGMT_FREQMSK;
	*clock = ((*clock)&CLKMGMT_SRCMSK) | local_c ;

	/* Step-1 : Urgent Task     */
	clkitm = sys_clock_list;
	while( clkitm != NULL ){
		// Urgent Task
		if( (clkitm->callback.func != NULL)
		    && ((UINT32)(clkitm->callback.param) == CLKMGMT_PRI_URGENT) ){
			clkitm->callback.func( local_c,
				clkitm->callback.param );
		}
		clkitm = clkitm->next;
	}

	/* Step-2 : Non-Urgent & Non-Cached Task */
	clkitm = sys_clock_list;
	while( clkitm != NULL ){
		// Non-Urgent Task
		if( (clkitm->callback.func != NULL)
		    && ((UINT32)(clkitm->callback.param) != CLKMGMT_PRI_URGENT)
		    && ((UINT32)(clkitm->callback.func) <= DA16X_SRAM_END) ){
			clkitm->callback.func( local_c,
				clkitm->callback.param );
		}
		clkitm = clkitm->next;
	}

	/* Step-3 : Non-Urgent & Cached Task */
	clkitm = sys_clock_list;
	while( clkitm != NULL ){
		// Non-Urgent Task
		if( (clkitm->callback.func != NULL)
		    && ((UINT32)(clkitm->callback.param) != CLKMGMT_PRI_URGENT)
		    && ((UINT32)(clkitm->callback.func) > DA16X_SRAM_END) ){
			clkitm->callback.func( local_c,
				clkitm->callback.param );
		}
		clkitm = clkitm->next;
	}

	return sizeof(UINT32);
}
