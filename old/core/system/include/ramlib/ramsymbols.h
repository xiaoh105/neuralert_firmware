/**
 ****************************************************************************************
 *
 * @file ramsymbols.h
 *
 * @brief DA16200 RAM Library
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


#ifndef	__ramsymbols_h__
#define	__ramsymbols_h__

#include "hal.h"
#include "da16x_timer.h"
//---------------------------------------------------------
// 	RAM-LIB Definition
//---------------------------------------------------------
#define NEW_AGC_CFG

//---------------------------------------------------------
// 	RAM-LIB Definition
//---------------------------------------------------------

#define	DA16X_RAMSYM_BASE	(DA16X_SRAM_BASE|0x00003000)
#define	DA16X_RAMSYM_MAGIC_TAG	0xFC96B000
#define	DA16X_RAMSYM_MAGIC_MASK	0xFFFFF000

#define	DA16X_RAMSYM_MAGIC	(DA16X_RAMSYM_MAGIC_TAG|0x001)

typedef 	struct __da16x_ramsym_tab__	RAMLIB_SYMBOL_TYPE;

//---------------------------------------------------------
//	RAM-LIB :: Symbol Table
//---------------------------------------------------------

struct	__da16x_ramsym_tab__
{
	UINT32	magic;							/* Export */
	UINT32	build;							/* Export */
	INT8    *logo;							/* Export */
	UINT32	(*checkramlib)(UINT32);					/* Export */
	INT32	(*init_ramlib)(void);					/* Export */

	UINT32	(*htoi)(char *);					/* Import */
	UINT32	(*atoi)(char *);					/* Import */
	INT32	(*printf)(char *,...);					/* Import */
	VOID	(*spinlock)(HANDLE);					/* Import */
	VOID	(*spinunlock)(HANDLE);					/* Import */
	VOID	(*sleep)(UINT32);					/* Import */
	UINT32	(*random)(void);					/* Import */
	UINT32	(*memcopy)(UINT32, VOID *, VOID *, UINT32 );		/* Import */

	// PHY Driver
	VOID	(*phy_init)(void *);					/* Export */
	VOID	(*phy_set_channel)(uint8_t , uint8_t
			, uint16_t , uint16_t
			, uint16_t , uint8_t);				/* Export */
	VOID	(*phy_get_channel)(VOID* , uint8_t);			/* Export */

	VOID	*phy_commands;						/* Export */
	INT32	(*phy_set_conf)(VOID *data, UINT32 size);
	INT32	(*phy_get_conf)(VOID *data, UINT32 size);
	// RTC Manager
	HANDLE (*MCNT_CREATE)(void);					/* IMPORT */
	INT32	(*MCNT_START)(HANDLE, UINT64, ISR_CALLBACK_TYPE*); 	/* IMPORT */
	INT32	(*MCNT_STOP)(HANDLE, UINT64 *remain);			/* IMPORT */
	VOID	(*MCNT_CLOSE)(HANDLE);					/* IMPORT */

	INT32 (*RTC_IOCTL)(UINT32 cmd , VOID *data );			/* IMPORT */
	VOID (*RTC_INTO_POWER_DOWN)(UINT32 flag,
				unsigned long long period);		/* IMPORT */
	WAKEUP_SOURCE (*RTC_GET_WAKEUP_SOURCE)(VOID);			/* IMPORT */
	VOID (*RTC_READY_POWER_DOWN)(int clear);			/* IMPORT */

	DA16X_TIMER_ID (*DA16X_GET_EMPTY_ID)(VOID);			/* Export */
	DA16X_TIMER_ID (*DA16X_SET_TIMER)(DA16X_TIMER_ID id,
		unsigned long long time, DA16X_TIMER_PARAM param);	/* Export */
	DA16X_TIMER_ID (*DA16X_SET_WAKEUP_TIMER)
		(DA16X_TIMER_ID id, unsigned long long time,
		 DA16X_TIMER_PARAM param);				/* Export */
	DA16X_TIMER_ID (*DA16X_KILL_TIMER)(DA16X_TIMER_ID id); 		/* Export */
	WAKEUP_SOURCE (*DA16X_TIMER_SCHEDULER)(VOID);			/* Export */

	DA16X_TIMER_ID (*DA16X_SET_PERIOD)(DA16X_TIMER_ID id,
		unsigned long long time, DA16X_TIMER_PARAM param);	/* Export */
	VOID (*DA16XSLEEP_EN)(void);					/* Export */

	UINT32 (*rfcal_irq_init)(void);					/* IMPORT */
};


#endif	/*__ramsymbols_h__*/

/* EOF */
