/**
 ****************************************************************************************
 *
 * @file ramsymbols.c
 *
 * @brief Symbol Table for RAM Library
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

#ifdef	BUILD_OPT_DA16X_RAMLIB


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "ramlib.h"
#include "../../version/genfiles/buildtime.h"
#include "ramsymbols.h"
#include "da16x_timer.h"

#include "da16x_system.h"
#include "romac_primitives_patch.h"

/******************************************************************************
 *
 *  Macros
 *
 ******************************************************************************/

#define MACRO_STR_(x) #x
#define MACRO_STR(x) MACRO_STR_(x)

#define	RAM_SYMBOL_LOGO "DA16X-" MACRO_STR(REV_BUILDDATE) ":" MACRO_STR(REV_BUILDTIME) ":SVN" MACRO_STR(SVN_REVISION_NUMBER)

extern void da16x_phy_init(void *cfg);
extern void da16x_phy_set_channel(uint8_t band, uint8_t type, uint16_t prim20_freq,
                     uint16_t center1_freq, uint16_t center2_freq, uint8_t index);
extern void da16x_phy_get_channel(VOID* param, uint8_t index);
extern int32_t da16x_phy_set_conf(void *param, uint32_t len);
extern void *da16x_phy_commands;
extern uint32_t agc_table_da16x[512];

/******************************************************************************
 *
 *  Static Functions
 *
 ******************************************************************************/

static	UINT32	ramlib_symbols_check(UINT32 code);
static	int	ramlib_symbols_init(void);

#if 0
static  UINT8	ramlib_beacon[3288]	RW_SEC_UNINIT;
#else
//static  UINT8	ramlib_beacon[4076]	RW_SEC_UNINIT;
#endif

/******************************************************************************
 *
 *  Symbol Table
 *
 ******************************************************************************/

const	RAMLIB_SYMBOL_TYPE	da16x_ramlib_symbols 	RW_SEC_LIB
= {
	DA16X_RAMSYM_MAGIC, 	//UINT32	magic;
	REV_BUILDDATE_HEX, 	//UINT32	build;
	RAM_SYMBOL_LOGO,      	//const char	*logo;
	&ramlib_symbols_check,	//UINT32	(*checkramlib)(UINT32);
	&ramlib_symbols_init,	//int		(*init_ramlib)(void);

	NULL,			//UINT32	(*htoi)(char *);
	NULL,			//UINT32	(*atoi)(char *);
	NULL,			//void	(*printf)(char *,...);
	NULL,			//void	(*spinlock)(HANDLE);
	NULL,			//void	(*spinunlock)(HANDLE);
	NULL,			//void	(*sleep)(UINT32);
	NULL,			//UINT32 (*random)(UINT32);
	NULL,			//UINT32 (*memcopy)(UINT32, VOID *, VOID *, UINT32 );

	NULL,			//void	(*phy_init)(VOID *);
	NULL,			//void	(*phy_set_channel)(...);
	NULL,			//void	(*phy_get_channel)(...);
#ifdef FC9000_PHY_EXT_CMD
	NULL,			//VOID	*phy_commands;
#else
	NULL,
#endif
	NULL,			//	int	(*phy_set_conf)(void *data, int size);
	NULL,			//	int	(*phy_get_conf)(void *data, int size);
	//(VOID *)agc_table_da16x,	//VOID	*agctable;
	//(VOID *)ramlib_beacon,	//VOID	*beacon;

	/* RTC manager */
	NULL,			//int (*_sys_clock_read)(UINT32 *clock, int len);
	NULL,			//int	(*STIMER_IOCTL)(HANDLE handler,
				// UINT32 cmd , VOID *data );
	NULL, 			//HANDLE	(*STIMER_CREATE)( UINT32 dev_type );
	NULL,			//int	(*STIMER_INIT)(HANDLE handler);

	NULL,			//int (*RTC_IOCTL)(UINT32 cmd , VOID *data );
	NULL,			//void (*RTC_INTO_POWER_DOWN)(UINT32 flag,
				//unsigned long long period );/* IMPORT */
	NULL,			//WAKEUP_SOURCE (*RTC_GET_WAKEUP_SOURCE)(void);		/* IMPORT */
	NULL,			//void (*RTC_READY_POWER_DOWN)( void );			/* IMPORT */

	NULL,			/*&DA16X_GET_EMPTY_ID*/
	NULL,			/*&DA16X_SET_TIMER*/
	NULL,			/*&DA16X_SET_WAKEUP_TIMER*/
	NULL,			/*&DA16X_KILL_TIMER*/
	NULL,			/*&DA16X_TIMER_SCHEDULER*/
	NULL,			/*&DA16X_SET_PERIOD*/
	NULL,			/*&DA16X_SLEEP_EN*/
	NULL,			//void (*rfcal_irq_init)( void );
};

// RAMLIB_SYMBOL_TYPE	*ramsymbols	/*RW_SEC_UNINIT*/;

/******************************************************************************
 *  ramlib_symbols_check ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static	UINT32	ramlib_symbols_check(UINT32 code)
{
#if 1
	if( da16x_ramlib_symbols.build == code ){
		ramsymbols = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;
		return TRUE;
	}else{
		ramsymbols = NULL;
	}
#endif
	return FALSE;
}

/******************************************************************************
 *  ramlib_symbols_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	PHY_PRINTF(...)		(ramsymbols->printf)( __VA_ARGS__ )
static	int	ramlib_symbols_init(void)
{
	struct dpm_param *dpmp = GET_DPMP();
	MK_TRG_RA(DPM_TRG_START);

	init_ramlib();
	ramlib_symbols_check(REV_BUILDDATE_HEX); // Re-init Issue of ramsymbols

    //ramsymbols->phy_init = &da16200_phy_init;
    //ramsymbols->phy_set_channel = &da16200_phy_set_channel;
    //ramsymbols->phy_get_channel = &da16200_phy_get_channel;

	return TRUE;
}

#endif	/*BUILD_OPT_DA16X_RAMLIB*/

/* EOF */
