/**
 ****************************************************************************************
 *
 * @file da16x_ta2sync.h
 *
 * @brief TA2SYNC Controller
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


#ifndef	__da16x_ta2sync_h__
#define	__da16x_ta2sync_h__


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

#define TA_ACC_K	9

#define	TA_MODE_ACQ		(1<<23)
#define	TA_MODE_TRK		(1<<22)
#define	TA_MODE_DONE		(1<<22)

typedef		struct	_da16x_ta2sync_	DA16X_TA2SYNC_TypeDef;

struct		_da16x_ta2sync_
{
	volatile unsigned long start;          /* 0x500a0000 */
	volatile unsigned long sleep_period;   /* 0x500a0004 */
	volatile unsigned long k;              /* 0x500a0008 */
	volatile unsigned long ctrl0;          /* 0x500a000c */
	volatile unsigned long ctrl1;          /* 0x500a0010 */
	volatile unsigned long ne1_rtm;        /* 0x500a0014 */
	volatile unsigned long ncm;            /* 0x500a0018 */
	volatile unsigned long nr;             /* 0x500a001c */
	volatile unsigned long n0;             /* 0x500a0020 */
	volatile unsigned long r;              /* 0x500a0024 */
	volatile unsigned long ne;             /* 0x500a0028 */
	volatile unsigned long ne1;            /* 0x500a003c */
	volatile unsigned long delta_ne;       /* 0x500a0030 */
	volatile unsigned long monitor0;       /* 0x500a0034 */
	volatile unsigned long monitor1;       /* 0x500a0038 */
	volatile unsigned long set_glich;      /* 0x500a003c */
	volatile unsigned long stop;           /* 0x500a0040 */
	volatile unsigned long mode;           /* 0x500a0044 */
	volatile unsigned long kperiod;        /* 0x500a0048 */
};

extern void	da16x_litetasync_start(UINT32 xtal, UINT32 scale, UINT32 test);
extern UINT32	da16x_litetasync_stop(UINT32 itertime);

extern UINT32 	ta2_acquisition(UINT32 xtal, VOID *backup);
extern INT32 	ta_sync_get_ta2(void);

#endif	/*__da16x_ta2sync_h__*/
