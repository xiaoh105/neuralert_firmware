/**
 ****************************************************************************************
 *
 * @file da16200_system.c
 *
 * @brief System Peripherals
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
#include <stdarg.h>

#include "hal.h"
#include "driver.h"
#include "common_def.h"

#if 0//(dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define traceISR_ENTER()
#  define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

#undef	SUPPORT_BOOTMAM_ROMLIB

#ifndef	SUPPORT_BOOTMAM_ROMLIB

/******************************************************************************
 *
 *  Clear Boot retry count.
 *
 ******************************************************************************/
void	da16x_boot_clear_count(void)
{
#define BOOT_CHECK_CNT_ADDR			0x50080270		// register for interface testing, HW default value is 0x0
	*(UINT32*)BOOT_CHECK_CNT_ADDR = 0;
}

/******************************************************************************
 *
 *  Boot Paramter functions
 *
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void	da16x_boot_set_offset(UINT32 boot, UINT32 imgaddr)
{
	if( ((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_TAG] != BTCFG_LOCK_TAG ){
#if	(BOOT_IDX_RTCM==0)
		if( boot <= BOOT_IDX_DBGTLEN){
#else   //(BOOT_IDX_RTCM==0)
		if( boot >= BOOT_IDX_RTCM && boot <= BOOT_IDX_DBGTLEN){
#endif  //(BOOT_IDX_RTCM==0)
			((UINT32 *)RETMEM_BOOTMODE_BASE)[boot] = imgaddr ;
		}
	}
}
	
ATTRIBUTE_RAM_FUNC
void	da16x_boot_reset_offset(UINT32 boot)
{
	if( ((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_TAG] != BTCFG_LOCK_TAG ){
#if	(BOOT_IDX_RTCM==0)
		if( boot <= BOOT_IDX_DBGTLEN){
#else   //(BOOT_IDX_RTCM==0)
		if( boot >= BOOT_IDX_RTCM && boot <= BOOT_IDX_DBGTLEN){
#endif  //(BOOT_IDX_RTCM==0)
			((UINT32 *)RETMEM_BOOTMODE_BASE)[boot] = 0xFFFFFFFF ;
		}
	}
}
	
ATTRIBUTE_RAM_FUNC
UINT32	da16x_boot_get_offset(UINT32 boot)
{
#if	(BOOT_IDX_RTCM==0)
	if( boot <= BOOT_IDX_DBGTLEN){
#else   //(BOOT_IDX_RTCM==0)
	if( boot >= BOOT_IDX_RTCM && boot <= BOOT_IDX_DBGTLEN){
#endif  //(BOOT_IDX_RTCM==0)
		return ((UINT32 *)RETMEM_BOOTMODE_BASE)[boot];
	}
	return 0xFFFFFFFF ;
}
	
ATTRIBUTE_RAM_FUNC
void	da16x_boot_set_lock(UINT32 mode)
{
	if( mode == TRUE ){
		((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_TAG] = BTCFG_LOCK_TAG;
	}else{
		((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_TAG] = 0x0;
	}
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_boot_get_wakeupmode(void)
{
	// MPW3 function
	return ( (((UINT32 *)RETMEM_MAGIC_BASE)[0] >> 24) & 0x0ff );
}

/******************************************************************************
 *  da16x_get_default_xtal( )
 *
 *  Purpose :
 *  Input   :   None.
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	XTAL_SUPPORTED(...)
ATTRIBUTE_RAM_RODATA
static const	UINT16 fc9000_pll_xtal_table[][2] = {
			 /* 32K */
XTAL_SUPPORTED(	{ 48000, 2930 }, )
		{ 40000, 2441 },
XTAL_SUPPORTED(	{ 38400, 2344 }, )
XTAL_SUPPORTED(	{ 33000, 2014 }, )
XTAL_SUPPORTED(	{ 32000, 1953 }, )
XTAL_SUPPORTED(	{ 30000, 1831 }, )
XTAL_SUPPORTED(	{ 27000, 1648 }, )
XTAL_SUPPORTED(	{ 24576, 1500 }, )
XTAL_SUPPORTED(	{ 24000, 1464 }, )
XTAL_SUPPORTED(	{ 20000, 1221 }, )
XTAL_SUPPORTED(	{ 19200, 1172 }, )
XTAL_SUPPORTED(	{ 16384, 1000 }, )
		{     0,    0 }
};

ATTRIBUTE_RAM_FUNC
UINT32	da16x_pll_xtal_index(UINT32 xtal)
{
	int 	i, idx;

	idx 	= 1; // 40 MHz

	for(i = 0; fc9000_pll_xtal_table[i][0] != 0 ; i++){
		if( (fc9000_pll_xtal_table[i][0] * KHz) == xtal ){
			idx = i;
			break;
		}
	}

	return idx;
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_pll_xtal_clock(UINT32 index)
{
	return (fc9000_pll_xtal_table[index][0] * KHz) ;
}

ATTRIBUTE_RAM_FUNC
void	da16x_get_default_xtal(UINT32 *xtalhz, UINT32 *busclock)
{
	UINT32	*local_retmemboot = (UINT32 *) RETMEM_BOOTMODE_BASE;

	if( ((local_retmemboot[BOOT_IDX_CLKCFG] & BOOTCFG_MARK) == BOOTCFG_MARK) ){
		//fixed: *xtalhz = BOOTCFG_GET_XTAL(local_retmemboot[BOOT_IDX_CLKCFG]);
		//fixed: *xtalhz = da16x_pll_xtal_clock((*xtalhz));
		*xtalhz = DA16X_SYSTEM_XTAL;
		*busclock = BOOTCFG_GET_CLOCK_KHZ(local_retmemboot[BOOT_IDX_CLKCFG]);
		*busclock = (*busclock) * KHz ;
	}
	else {
		*xtalhz = DA16X_SYSTEM_XTAL;
		_sys_clock_read(busclock, sizeof(UINT32));
	}
}

ATTRIBUTE_RAM_FUNC
void	da16x_set_default_xtal(UINT32 rcosc, UINT32 xtal, UINT32 sysclock)
{
	UINT32	*local_retmemboot = (UINT32 *) RETMEM_BOOTMODE_BASE;

	local_retmemboot[BOOT_IDX_CLKCFG] = /*BOOTCFG_MARK*/
		  BOOTCFG_SET_RCOSC(rcosc)
		| BOOTCFG_SET_XTAL( da16x_pll_xtal_index(xtal) )
		| BOOTCFG_SET_CLOCK_KHZ( (sysclock/KHz)  ) ;

}

/******************************************************************************
 *  da16x_cache( )
 *
 *  Purpose :
 *  Input   :   None.
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define CACHE_SET_BRST  	(1<<11)
#define CACHE_SET_WRAP  	(1<<10)
#define CACHE_SET_PLRU  	(1<<9)
#define CACHE_SET_PLRURAND  	(1<<8)
#define CACHE_SET_WAY8  	(1<<7)

#define CACHE_STATISTIC_EN  	(1<<6)
#define CACHE_SET_PREFETCH  	(1<<5)
#define CACHE_SET_MAN_INV  	(1<<4)
#define CACHE_SET_MAN_POW  	(1<<3)
#define CACHE_POW_REQ  		(1<<2)
#define CACHE_INV_REQ  		(1<<1)
#define CACHE_ENABLE  		(1<<0)

#define	CACHE_SET_MASK		((0x01FF)<<7)

#define CACHE_STAT_POWER	(1<<4)
#define CACHE_STAT_INV  	(1<<2)
#define CACHE_STATUS_MASK	(0x03)
#define CACHE_STATUS_DISABLED  	(0)
#define CACHE_STATUS_ENABLING  	(1)
#define CACHE_STATUS_ENABLED  	(2)
#define CACHE_STATUS_DISABLING 	(3)

typedef		struct	_da16x_cache_ctrl_	DA16X_CACHE_TypeDef;

struct		_da16x_cache_ctrl_
{
	volatile UINT32	CCR;		// 0x00, RW
	volatile UINT32	SR;			// 0x04, RO
	volatile UINT32	IRQMASK;	// 0x08
	volatile UINT32	IRQSTAT;	// 0x0C
	volatile UINT32	HWPARAMS;	// 0x10, RO
	volatile UINT32	CSHR;		// 0x14, RW
	volatile UINT32	CSMR;		// 0x18, RW
};

#define DA16X_CACHE_CTRL		((DA16X_CACHE_TypeDef *)DA16X_CCHCTRL_BASE)

ATTRIBUTE_RAM_FUNC
void _lowlevel_da16x_cache_interrupt(void)
{
	traceISR_ENTER();
	{
		register UINT32 irqstat;
		irqstat = DA16X_CACHE_CTRL->IRQSTAT;
		DA16X_CACHE_CTRL->IRQSTAT = irqstat;
		//ASIC_DBG_TRIGGER(0xFC9CAC4E);
	}
	traceISR_EXIT();
}

ATTRIBUTE_RAM_FUNC
static void	da16x_cache_interrupt(void)
{
	INTR_CNTXT_CALL(_lowlevel_da16x_cache_interrupt);
}

ATTRIBUTE_RAM_FUNC
void	da16x_cache_enable(UINT32 intrmode)
{
	UINT32 special;

	if( intrmode == TRUE ){
		_sys_nvic_write(Cache_Ctrl_IRQn, (void *)da16x_cache_interrupt, 0x7);
		DA16X_CACHE_CTRL->IRQMASK = 0x3;
	}

	if( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) == CACHE_STATUS_ENABLED ){
		return;
	}

	special = (DA16X_CACHE_CTRL->CCR) & (CACHE_SET_MASK);

	// 3.4.2. Manual power and auto invalidate mode
	// -- enabling & invalidating
	DA16X_CACHE_CTRL->CCR = CACHE_SET_MAN_POW;
	DA16X_CACHE_CTRL->CCR |= CACHE_POW_REQ;
	while( (DA16X_CACHE_CTRL->SR & CACHE_STAT_POWER) != CACHE_STAT_POWER );

	if( (special&CACHE_SET_WAY8) != 0 ){
		DA16X_CACHE_CTRL->CCR |= CACHE_SET_WAY8;
	}
	DA16X_CACHE_CTRL->CCR |= CACHE_ENABLE;

	while( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_ENABLED );

	//Prefetch : DA16X_CACHE_CTRL->CCR |= (CACHE_SET_PREFETCH|CACHE_STATISTIC_EN);
	DA16X_CACHE_CTRL->CCR |= (CACHE_STATISTIC_EN);
	DA16X_CACHE_CTRL->CCR |= special;
}

ATTRIBUTE_RAM_FUNC
void	da16x_cache_disable(void)
{
	UINT32 special;

	if( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) == CACHE_STATUS_DISABLED ){
		return;
	}

	special = (DA16X_CACHE_CTRL->CCR) & (CACHE_SET_MASK);

	DA16X_CACHE_CTRL->CCR &= ~CACHE_ENABLE;
	DA16X_CACHE_CTRL->IRQMASK = 0x0;
	DA16X_CACHE_CTRL->IRQSTAT = 0x3;
	DA16X_CACHE_CTRL->CSHR = 0;
	DA16X_CACHE_CTRL->CSMR = 0;
	DA16X_CACHE_CTRL->CCR = CACHE_SET_MAN_INV|CACHE_SET_MAN_POW;

	while( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_DISABLED );

	DA16X_CACHE_CTRL->CCR |= special;

	_sys_nvic_write(Cache_Ctrl_IRQn, NULL, 0x7);
}

ATTRIBUTE_RAM_FUNC
void	da16x_cache_invalidate(void)
{
	UINT32 waymode;
	UINT32 special;

	if( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_ENABLED ){
		return;
	}

	special = (DA16X_CACHE_CTRL->CCR) & (CACHE_SET_MASK);

	waymode = ((DA16X_CACHE_CTRL->CCR & CACHE_SET_WAY8) != 0) ? 1 : 0;

	DA16X_CACHE_CTRL->CCR &= ~CACHE_ENABLE;

	DA16X_CACHE_CTRL->CCR |= CACHE_SET_MAN_INV|CACHE_SET_MAN_POW|CACHE_POW_REQ;
	while( (DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_DISABLED );

	DA16X_CACHE_CTRL->CCR |= CACHE_INV_REQ;
	while( (DA16X_CACHE_CTRL->CCR & CACHE_INV_REQ) == CACHE_INV_REQ );

	if( waymode != 0 ){
		DA16X_CACHE_CTRL->CCR |= CACHE_SET_WAY8;
	}
	DA16X_CACHE_CTRL->CCR |= CACHE_ENABLE;
	DA16X_CACHE_CTRL->CCR |= special;
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_cache_get_hitcount(void)
{
	UINT32 count;

	if((DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_ENABLED){
		return 0;
	}

	count = DA16X_CACHE_CTRL->CSHR;
	DA16X_CACHE_CTRL->CSHR = 0;
	return count;
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_cache_get_misscount(void)
{
	UINT32 count;

	if((DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_ENABLED){
		return 0;
	}

	count = DA16X_CACHE_CTRL->CSMR;
	DA16X_CACHE_CTRL->CSMR = 0;
	return count;
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_cache_status(void)
{
	if((DA16X_CACHE_CTRL->SR & CACHE_STATUS_MASK) != CACHE_STATUS_ENABLED){
		return FALSE;
	}
	return TRUE;
}

CLOCK_LIST_TYPE	da16x_cache_pll_info;

ATTRIBUTE_RAM_FUNC
static void da16x_cache_pll_callback(UINT32 clock, void *param)
{
	DA16X_UNUSED_ARG(param);

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		da16x_cache_disable();
		return;
	}

	da16x_cache_enable(FALSE);
}
ATTRIBUTE_RAM_FUNC
void	da16x_cache_clkmgmt(UINT32 code_base)
{
	if( ( (code_base >= DA16X_NOR_CACHE_BASE) && (code_base <= DA16X_NOR_CACHE_END) )
	 || ( (code_base >= DA16X_CACHE_BASE) && (code_base <= DA16X_CACHE_END) ) ){
		da16x_cache_pll_info.callback.func = da16x_cache_pll_callback;
		da16x_cache_pll_info.callback.param = (void *)CLKMGMT_PRI_URGENT;
		da16x_cache_pll_info.next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, &da16x_cache_pll_info);

	}
}

/******************************************************************************
 *
 *  DA16200 only
 *
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
UINT32	da16x_get_cacheoffset(void)
{
	volatile UINT32 *XIP_Offset = (UINT32 *)(DA16X_XFC_BASE|0x0360);
	return (UINT32)(*XIP_Offset);
}

ATTRIBUTE_RAM_FUNC
void	da16x_set_cacheoffset(UINT32 flashoffset)
{
	volatile UINT32 *XIP_Offset = (UINT32 *)(DA16X_XFC_BASE|0x0360);
	*XIP_Offset = flashoffset;
}


#ifdef	BUILD_OPT_DA16200_ASIC

/******************************************************************************
 *
 *  ASIC PLL functions
 *
 ******************************************************************************/

#define	PLL_DIV_VCO_MASK	(PLL1_ARM_VCO_SEL(0x3)|PLL1_ARM_DIV_SEL(0x3f))
#define	PLL_MAX_LCKCHK_TIME	0x00007FFF

#define	PLL_MAX_FREQ		(480*KHz)

#define	SUPPORT_PLL_PROGRAMABLE
#ifdef	SUPPORT_PLL_PROGRAMABLE
#define	MAX_PLLLP55T_DIV	30
#define	MIN_PLLLP55T_DIV	15
#define	PLL_MAX_VCO		3

static UINT32 da16x_pll_getvalue(UINT32 xtalkhz, UINT32 *clock, UINT32 *pll_v, UINT32 *pll_r);
#endif	//SUPPORT_PLL_PROGRAMABLE

/******************************************************************************
 *  da16x_pll_on ( )
 *
 *  Purpose :
 *  Input   :  Hz
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
UINT32 da16x_pll_on(UINT32 crystal)
{
	volatile UINT32 plllocked = TRUE;

	// 5.  set dpll1en & xtalen to activate PLL  ( XTAL_GAIN(3) )
	DA16200_COMMON->XTAL40Mhz
		= (DA16200_COMMON->XTAL40Mhz & ~BB2XTAL_GAINMASK)
			| BB2XTAL_EN
			| BB2XTAL_DPLL1EN	/* turn on */
			| BB2XTAL_GAIN(3) ;	/* After power on, need to set the value from 7 downto 3 : 2018.05.28 */
	// 6.  disable PLL ( power-down )
	DA16200_COMMON->PLL1_ARM &= ~(PLL1_ARM_ENABLE);
	NOP_EXECUTION();

	if( (DA16200_COMMON->PLL1_ARM & PLL1_ARM_LOCK) == PLL1_ARM_LOCK ){
			plllocked = FALSE; // illegal PLL LOCK ???
	}
	// 7.  setup PLL to operate 480MHz
#ifdef SUPPORT_PLL_PROGRAMABLE
	{
		UINT32	clock, pllv, pllr;

		clock = PLL_MAX_FREQ;
		pllv = 0;
		pllr = 0;

		da16x_pll_getvalue((crystal/KHz), &clock, &pllv, &pllr);

		DA16200_COMMON->PLL1_ARM =
			( DA16200_COMMON->PLL1_ARM & (~PLL_DIV_VCO_MASK) )
			| PLL1_ARM_VCO_SEL(pllv)
			| PLL1_ARM_DIV_SEL(pllr) ;
	}
#else	//SUPPORT_PLL_PROGRAMABLE
	DA16200_COMMON->PLL1_ARM =
			( DA16200_COMMON->PLL1_ARM & (~PLL_DIV_VCO_MASK) )
			| PLL1_ARM_VCO_SEL(2)
			| PLL1_ARM_DIV_SEL(24) ;
#endif	//SUPPORT_PLL_PROGRAMABLE

	DA16200_COMMON->PLL1_ARM |= (PLL1_ARM_ENABLE);

	// 8.  wait until PLL is locked

	// 1st step : loop delay for illegal LOCK
	if( plllocked == FALSE ){
		plllocked = PLL_MAX_LCKCHK_TIME; /* x1 times */
		while( plllocked-- > 0 );
	}

	// 2nd step : PLL Lock checking
	plllocked = PLL_MAX_LCKCHK_TIME; /* x1 times */

	while( (DA16200_COMMON->PLL1_ARM & PLL1_ARM_LOCK) != PLL1_ARM_LOCK ){
		if( plllocked-- == 0 ){
			break;
		}
	}

	if( plllocked != 0 ){
		// 9.  disable PLL_CLK_EN_CPU
		DA16200_SYSCLOCK->PLL_CLK_EN_0_CPU = 0;
		// 10. setup DIV_CPU to run CPU at default freq. (120MHz)
		DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU = (((PLL_MAX_FREQ*KHz)/DA16X_SYSTEM_CLOCK) - 1);
		// 11. enable PLL_CLK_EN_CPU
		DA16200_SYSCLOCK->PLL_CLK_EN_0_CPU = 1;
		// 12. set SRC_CLK_SEL_CPU to switch from XTAL to PLL
		DA16200_SYSCLOCK->SRC_CLK_SEL_0_CPU = 1; // PLL
		// 13. wait on XTAL_2_PLL  (CPU Sleep?)
		WAIT_FOR_INTR();

		if( DA16200_SYSCLOCK->SRC_CLK_STA_0 != CLK_STA_PLL ){
			plllocked = 0; // False
		}
	}

	return (( plllocked == 0 ) ? FALSE : TRUE);
}

/******************************************************************************
 *  da16x_pll_off ( )
 *
 *  Purpose :
 *  Input   :  KHz
 *  Output  :  KHz
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void da16x_pll_off(void)
{
	if( DA16200_SYSCLOCK->SRC_CLK_SEL_0_CPU != 0 ){
		// 2.  set SRC_CLK_SEL_CPU to change from PLL to XTAL
		DA16200_SYSCLOCK->SRC_CLK_SEL_0_CPU = 0; // XTAL
		// 3.  wait on PLL_2_XTAL  (CPU Sleep?)
		WAIT_FOR_INTR();
	}
}

/******************************************************************************
 *  da16x_xtal_trim ( )
 *
 *  Purpose :
 *  Input   :  KHz
 *  Output  :  KHz
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void da16x_xtal_trim(UINT32 gain, UINT32 ppm)
{
	DA16200_COMMON->XTAL40Mhz
			= (DA16200_COMMON->XTAL40Mhz & (~(BB2XTAL_GAINMASK|BB2XTAL_CCTRLMASK)))
			| ( BB2XTAL_GAIN(gain) | BB2XTAL_CCTRL(ppm) ) ;
}

/******************************************************************************
 *  da16x_pll_getspeed ( )
 *
 *  Purpose :
 *  Input   :  KHz
 *  Output  :  KHz
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
UINT32 da16x_pll_getspeed(UINT32 xtalkhz)
{
	UINT32 pll_v, pll_r, pll_s;

	pll_v = DA16200_COMMON->PLL1_ARM;
	pll_s = 0; // 1/2 fixed
	pll_r = PLL1_ARM_GET_DIV_SEL(pll_v);
	pll_v = PLL1_ARM_GET_VCO_SEL(pll_v);

	return (((xtalkhz * pll_r) >> pll_v) >> (1<<pll_s)) ;
}

/******************************************************************************
 *  da16x_pll_getvalue ( )
 *
 *  Purpose :
 *  Input   : KHz
 *  Output  : KHz
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
static UINT32 da16x_pll_getvalue(UINT32 xtalkhz, UINT32 *clock, UINT32 *pll_v, UINT32 *pll_r)
{
	volatile UINT32	min, max;
	UINT32	local_v, local_r, local_c;

	min = (MIN_PLLLP55T_DIV * (xtalkhz >> 1)) ;
	max = (MAX_PLLLP55T_DIV * (xtalkhz >> 1)) ;
	//DA16200: *clock = *clock << 1;

	for( local_v = 0; local_v <= PLL_MAX_VCO; local_v++ ){

		local_r = (*clock << local_v) ;
		local_c = (local_r >> local_v) ;

		//PRINTF("match %d < %d < %d\n", min, local_c, max);

		if( local_c >= min && local_c <= max ){

			if( pll_v != NULL ){
				*pll_v = local_v ;
			}
			if( pll_r != NULL ){
				*pll_r = local_r / (xtalkhz >> 1) ;
				*clock = ((((xtalkhz >> 1) * *pll_r) >> local_v) >> 1) ;

				ASIC_DBG_TRIGGER( *clock );
			}

			return TRUE;
		}

		min = min >> 1;
		max = max >> 1;
	}
	return FALSE;
}

#endif	//BUILD_OPT_DA16200_ASIC

/******************************************************************************
 *  da16x_pll_x2check ( )
 *
 *  Purpose :
 *  Input   : KHz
 *  Output  : KHz
 *  Return  :
 ******************************************************************************/

// for changing the system clock safely in cache boot,
// this table should be located in sram.
// 
//   Index 0: not used, 1: x1 only, 2: x2 upto 80MHz, 3: x2 upto 120MHz
//
// 200924: Through previous tests, only the x1 model was confirmed to be stable.

#ifdef	BUILD_OPT_DA16200_ASIC
ATTRIBUTE_RAM_RODATA
static const UINT8 da16x_pll_clk_table[0x20] = {
	0x00,	// reserved
	0x01,	//01 : 240.000 MHz (=480/2),
	0x02,	//02 : 160.000 MHz (=480/3),
	0x03,	//03 : 120.000 MHz (=480/4),
	0x04,	//04 :  96.000 MHz (=480/5),
	0x05,	//05 :  80.000 MHz (=480/6),
	0x06,	//06 :  68.571 MHz (=480/7),
	0x07,	//07 :  60.000 MHz (=480/8),
	0x08,	//08 :  53.333 MHz (=480/9)
	0x09,	//09 :  48.000 MHz (=480/10)
	0x0A,	//0A :  43.636 MHz (=480/11)
	0x0B,	//0B :  40.000 MHz (=480/12)
	0x0C,	//0C :  36.923 MHz (=480/13)
	0x0D,	//0D :  34.286 MHz (=480/14)
	0x0E,	//0E :  32.000 MHz (=480/15)
	0x0F,	//0F :  30.000 MHz (=480/16)
	0x10,	//10 :  28.235 MHz (=480/17) unstable
	0x11,	//11 :  26.667 MHz (=480/18)
	0x12,	//12 :  25.263 MHz (=480/19)
	0x13,	//13 :  24.000 MHz (=480/20)
	0x14,	//14 :  22.857 MHz (=480/21)
	0x15,	//15 :  21.818 MHz (=480/22)
	0x16,	//16 :  20.870 MHz (=480/23)
	0x17,	//17 :  20.000 MHz (=480/24)
	0x18,	//18 :  19.200 MHz (=480/25)
	0x19,	//19 :  18.462 MHz (=480/26)
	0x1A,	//1A :  17.778 MHz (=480/27)
	0x1B,	//1B :  17.143 MHz (=480/28)
	0x1C,	//1C :  16.552 MHz (=480/29)
	0x1D,	//1D :  16.000 MHz (=480/30)
	0x1E,	//1E :  15.484 MHz (=480/31)
	0x1F	//1F :  15.000 MHz (=480/32)
};
#endif	//BUILD_OPT_DA16200_ASIC
 
static UINT8 *da16x_pll_x2check_exp;

ATTRIBUTE_RAM_FUNC
void	da16x_pll_x2check_experimental(UINT8 *exp)
{
	da16x_pll_x2check_exp = exp;
}

ATTRIBUTE_RAM_FUNC
UINT32	da16x_pll_x2check(void)
{
	UINT32 cpuclk ;

	cpuclk = DA16X_CLK_DIV( DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU );

	//Printf(" [%s] DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU:%d cpuclk:%d da16x_pll_x2check_exp:0x%x da16x_pll_x2check_exp[cpuclk]:%d \r\n", __func__, DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU, cpuclk, da16x_pll_x2check_exp, da16x_pll_x2check_exp[cpuclk]);

#ifdef	BUILD_OPT_DA16200_ASIC
	if( da16x_pll_x2check_exp == NULL ){
		return da16x_pll_clk_table[cpuclk]; // x2 or x1
		//TRICK200924: return cpuclk;
	}
	return da16x_pll_x2check_exp[cpuclk];
#else	//BUILD_OPT_DA16200_ASIC
	return cpuclk;	// x1
#endif	//BUILD_OPT_DA16200_ASIC
}

#ifdef	SUPPORT_DA16X_AUTOBAUD
/******************************************************************************
 *
 *  UART Baud Detector
 *
 ******************************************************************************/

#define DA16X_NOTSUPPORT(...)

typedef	 struct {
	volatile UINT8	UART_BRD_OP;
	volatile UINT8	UART_BRD_STATUS;
	volatile UINT8	_reserve[14];
	volatile UINT32	UART_BRD_CNT;
} DA16XUART_BAUD_Type;

#define	DA16X_UARTDET_BASE	(DA16200_SYSPERI_BASE|0x00002200)
#define DA16X_UARTDET		((DA16XUART_BAUD_Type *)DA16X_UARTDET_BASE)
ATTRIBUTE_RAM_RODATA
static const UINT32 uart_baud_rate[] = {
	0      ,
DA16X_NOTSUPPORT(50     ,)
DA16X_NOTSUPPORT(75     ,)
	110    ,
DA16X_NOTSUPPORT(150    ,)
	300    ,
DA16X_NOTSUPPORT(600    ,)
	1200   ,
DA16X_NOTSUPPORT(1800   ,)
	2400   ,
	4800   ,
DA16X_NOTSUPPORT(7200   ,)
	9600   ,
	14400  ,
	19200  ,
	38400  ,
DA16X_NOTSUPPORT(56000  ,)
	57600  ,
DA16X_NOTSUPPORT(76800  ,)
	115200 ,
DA16X_NOTSUPPORT(128000 ,)
	230400 ,
DA16X_NOTSUPPORT(256000 ,)
	460800 ,
	921600 ,
	1843200 ,
	3686400 ,	// tail
};

static UINT32 da16x_auto_uart_clock;

static UINT32 da16x_auto_baud_clock;

static UINT32 da16x_auto_baud_count;

ATTRIBUTE_RAM_FUNC
static void _tx_specific_conbaud_interrupt(void)
{
	INTR_CNTXT_CALL(da16x_baud_detect_core);
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_auto_baud_on(void *isrcallback, UINT32 uartclock, UINT32 detclk)
{
	DA16X_UARTDET->UART_BRD_OP = 0; // clear
	DA16X_UARTDET->UART_BRD_OP = 2; // clear

	da16x_auto_uart_clock = uartclock;
	da16x_auto_baud_clock = detclk;

	if( isrcallback != NULL ){
		_sys_nvic_write(BardRateDetect_IRQn, (void *)isrcallback, 0x7);
	}else{
		_sys_nvic_write(BardRateDetect_IRQn, (void *)_tx_specific_conbaud_interrupt, 0x7);
	}
	DA16X_UARTDET->UART_BRD_OP = 1; // start
	return TRUE;
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_auto_baud_off(void)
{
	UINT32 bkup_autobaud_irq;

	DA16X_UARTDET->UART_BRD_OP = 0; // clear
	DA16X_UARTDET->UART_BRD_OP = 2; // clear

	_sys_nvic_read( BardRateDetect_IRQn, (void *)&bkup_autobaud_irq); // backup
	_sys_nvic_write(BardRateDetect_IRQn, (void *)NULL, 0x7);

	return bkup_autobaud_irq;
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_get_curr_baudidx(UINT32 uarthz)
{
	if( DA16X_UARTDET->UART_BRD_STATUS == 3 ) // status = done
	{
		UINT32 uartbaud;
		da16x_auto_baud_count = DA16X_UARTDET->UART_BRD_CNT;
		uartbaud = (da16x_auto_baud_count>>3);
		uartbaud = uarthz / (uartbaud<<3);
		return da16x_get_baud_index(uartbaud);
	}

	return 0;
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_get_baud_rate(void)
{
	UINT32 index;

	index = ( *((UINT32 *)RETMEM_MAGIC_BASE) & 0x0ff );

	if( (index > 0) && (index < (sizeof(uart_baud_rate)/sizeof(UINT32))) ){
		return uart_baud_rate[index];
	}
	return 0;
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_get_baud_count(void)
{
	return da16x_auto_baud_count;
}

ATTRIBUTE_RAM_FUNC
UINT32 da16x_get_baud_index(UINT32 uartbaud)
{
	UINT32 i, prev;

	prev = 0xFFFFFFFF;
	for( i = 1; uart_baud_rate[i] < 3686400 ; i++ ){
		if( uart_baud_rate[i] == uartbaud ){
			return (i);
		}
		if( uart_baud_rate[i] > uartbaud ){
			if( prev > (uart_baud_rate[i] - uartbaud) ){
				return (i);
			}else{
				return (i-1);
			}
		}
		prev = uartbaud - uart_baud_rate[i];
	}
	return 0;
}

ATTRIBUTE_RAM_FUNC
void da16x_baud_detect_core(void)
{
	UINT32 conbaud, conport;
	traceISR_ENTER();

	conbaud = da16x_get_curr_baudidx(da16x_auto_baud_clock);

	if( conbaud != 0 ){
		da16x_auto_baud_off();
		conport = console_deinit(0);

		// update baud index
		*((UINT32 *)RETMEM_MAGIC_BASE) = ( *((UINT32 *)RETMEM_MAGIC_BASE) & ~0x0ff ) | (conbaud & 0x0ff) ;

		// re-init console
		conbaud = da16x_get_baud_rate();
		console_init(conport, conbaud, da16x_auto_uart_clock);
		console_hidden_mode(TRUE); // console re-okay
	}else{
		// re-activate baud detector
		UINT32 conbaudisr = da16x_auto_baud_off();
		da16x_auto_baud_on((void *)&conbaudisr, da16x_auto_uart_clock, da16x_auto_baud_clock);
	}
	traceISR_EXIT();
}
#endif	//SUPPORT_DA16X_AUTOBAUD

#endif	/*SUPPORT_BOOTMAM_ROMLIB*/
