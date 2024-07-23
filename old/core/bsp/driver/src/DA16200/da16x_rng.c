/**
 ****************************************************************************************
 *
 * @file da16x_rng.c
 *
 * @brief RANDOM
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

#undef	SUPPORT_PRNG_ROMLIB

#ifndef	SUPPORT_PRNG_ROMLIB
/******************************************************************************
 *  da16x_random ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

typedef		struct	_da16x_prng_	DA16X_PRNG_TypeDef;

struct		_da16x_prng_
{
	volatile unsigned long req;
	volatile unsigned long op;
	volatile unsigned long par;
	volatile unsigned long seed;
	volatile unsigned long calv;
};

#define DA16X_PRNG	((DA16X_PRNG_TypeDef *)DA16X_PRNG_BASE)

UINT32 da16x_random (void)
{
	UINT32  SEED_X;	// ROM HEX

	DA16X_CLKGATE_ON(clkoff_prng_core);

	if( DA16X_PRNG->seed == 0x7FFFFFFF || DA16X_PRNG->calv == 0 ){
		SEED_X = (UINT32)(SysTick->VAL);
		DA16X_PRNG->seed = 0x512e7874 + SEED_X;
		DA16X_PRNG->par = 2;
		DA16X_PRNG->req = 3;
	}else{
		DA16X_PRNG->req = 2;
	}

	SEED_X = (UINT32)DA16X_PRNG->calv;

	DA16X_CLKGATE_OFF(clkoff_prng_core);

	return SEED_X;
}

/******************************************************************************
 *  da16x_random_seed ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void   da16x_random_seed(UINT32 seed)
{
	DA16X_PRNG->seed = seed;
}

#endif /*SUPPORT_PRNG_ROMLIB*/
