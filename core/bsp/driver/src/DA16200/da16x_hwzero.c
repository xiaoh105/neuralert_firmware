/**
 ****************************************************************************************
 *
 * @file da16x_hwzero.c
 *
 * @brief HWZero
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

//#ifndef  USING_MROM_FUNCTION


#undef	TEST_HWZERO_BUS_ISSUE

#ifdef	TEST_HWZERO_BUS_ISSUE
#define HWZERO_FUNCTION_ATTR	DATA_ALIGN(16)
#define	HWZERO_FUNCTION_ALIGN	{		\
		NOP_EXECUTION();				\
		NOP_EXECUTION();				\
		NOP_EXECUTION();				\
		NOP_EXECUTION();				\
	}
#define	HWZERO_NOINIT_ATTR		ATTRIBUTE_NOINIT_DATA
#else	//TEST_HWZERO_BUS_ISSUE
#define HWZERO_FUNCTION_ATTR	ATTRIBUTE_RAM_FUNC
#define	HWZERO_FUNCTION_ALIGN
#define	HWZERO_NOINIT_ATTR		ATTRIBUTE_NOINIT_DATA
#endif	//TEST_HWZERO_BUS_ISSUE

//---------------------------------------------------------
// ZERO
//---------------------------------------------------------

typedef		struct	_da16x_zeroing_	DA16X_ZEROING_TypeDef;

struct		_da16x_zeroing_		// 0x5004_0100
{
	volatile UINT32	REQ;			// RW, 0x0000
	volatile UINT32	OP_EN;			// RW, 0x0004
	volatile UINT32	ADDR;			// RW, 0x0008
	volatile UINT32 LENGTH;			// RW, 0x000C
	volatile UINT32 SEED;			// RO, 0x0010
	volatile UINT32 OP_STA;			// RO, 0x0014
	volatile UINT32 partition[6];		// RO, 0x0018
	volatile UINT32 ACT_UNIT;		// RO, 0x0030
};

#define DA16X_ZEROING		((DA16X_ZEROING_TypeDef *)DA16X_ZEROING_BASE)

#define	ZERO_REQ_STOP		0x0004
#define	ZERO_REQ_START		0x0002
#define	ZERO_REQ_CLEAR		0x0001

 // removed :: #define	ZERO_OP_PHASE1		(1<<9)
#define	ZERO_OP_PHASE0		(1<<8)
#define ZERO_OP_SLEEP		(1<<2)
#define ZERO_OP_IRQEN		(1<<1)
#define ZERO_OP_EN		(1<<0)

#define	ZERO_STA_DONE		(1UL<<31)

static void 	da16x_hwzero_interrupt(void);
static UINT32	da16x_hwzero_lock(UINT32 mode);

HWZERO_NOINIT_ATTR
static UINT32	da16x_hwzero_busy;

/******************************************************************************
 *  func()
 *
 *  Purpose :
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

void	da16x_zeroing_init(void)
{
	da16x_hwzero_busy = FALSE;
	_sys_nvic_write(HW_Zeroing_IRQn, (void *)da16x_hwzero_interrupt, 0x7);	
}

void	da16x_zeroing_deinit(void)
{
	_sys_nvic_write(HW_Zeroing_IRQn, (void *)NULL, 0x7);	
}

HWZERO_FUNCTION_ATTR
void	da16x_zeroing_start(UINT32 addr, UINT32 len, UINT32 seed)
{
	while( da16x_hwzero_lock(TRUE) == FALSE );
		
	DA16X_CLKGATE_ON(clkoff_hw_zero);

	// CPU Sleep Mode
	DA16X_ZEROING->OP_EN	= ZERO_OP_PHASE0 | /*ZERO_OP_SLEEP |*/ ZERO_OP_IRQEN | ZERO_OP_EN;

	DA16X_ZEROING->REQ	= ZERO_REQ_CLEAR /*| ZERO_REQ_STOP*/;

	DA16X_ZEROING->ADDR	= (addr >> 2) ;
	DA16X_ZEROING->LENGTH	= ((len + 3) >> 2) - 1;
	DA16X_ZEROING->SEED	= seed ;

	DA16X_ZEROING->REQ = ZERO_REQ_START ;
}

HWZERO_FUNCTION_ATTR
void	da16x_zeroing_stop(void)
{
	while((DA16X_ZEROING->OP_STA & ZERO_STA_DONE) != ZERO_STA_DONE ){
		DA16X_ZEROING->REQ = ZERO_REQ_START ;
	}
	
	DA16X_ZEROING->REQ	= ZERO_REQ_CLEAR /*| ZERO_REQ_STOP*/;
	DA16X_ZEROING->OP_EN	= 0;

	DA16X_CLKGATE_OFF(clkoff_hw_zero);
	da16x_hwzero_lock(FALSE);
}

ATTRIBUTE_RAM_FUNC
static void da16x_hwzero_interrupt(void)
{
	// TODO 
	da16x_hwzero_busy = FALSE;
}

static UINT32  da16x_hwzero_lock(UINT32 mode)
{
	UINT32 intrbkup, status;
	
	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);	

	if ( da16x_hwzero_busy == TRUE ) {
		if ( mode == TRUE ) {
			status = FALSE;
		}else{
			da16x_hwzero_busy = FALSE;
			status = TRUE;
		}
	}
	else {
		if ( mode == TRUE ) {
			da16x_hwzero_busy = TRUE;
			status = TRUE;
		}
		else {
			status = TRUE;
		}
	}

	PREVENT_INTERRUPTS(intrbkup);	

	return status;
}

/******************************************************************************
 *  da16x_memset32 ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

HWZERO_FUNCTION_ATTR void da16x_memset32(UINT32 *data, UINT32 seed, UINT32 length)
{
	HWZERO_FUNCTION_ALIGN

	while( da16x_hwzero_lock(TRUE) == FALSE );
	
	DA16X_CLKGATE_ON(clkoff_hw_zero);

	// CPU Sleep Mode
	DA16X_ZEROING->OP_EN	= ZERO_OP_PHASE0 | /*ZERO_OP_SLEEP |*/ ZERO_OP_IRQEN | ZERO_OP_EN;
	
	DA16X_ZEROING->REQ	= ZERO_REQ_CLEAR /*| ZERO_REQ_STOP*/;
	
	DA16X_ZEROING->ADDR	= ((UINT32)data >> 2) ;
	DA16X_ZEROING->LENGTH	= ((length +3) >> 2) - 1;
	DA16X_ZEROING->SEED	= seed ;

	// CPU Sleep Mode
	do {
		DA16X_ZEROING->REQ = ZERO_REQ_START ;

		WAIT_FOR_INTR();
		
	} while ((DA16X_ZEROING->OP_STA & ZERO_STA_DONE) != ZERO_STA_DONE );
	
	DA16X_ZEROING->REQ	= ZERO_REQ_CLEAR /*| ZERO_REQ_STOP*/;
	DA16X_ZEROING->OP_EN	= 0;

	DA16X_CLKGATE_OFF(clkoff_hw_zero);
	da16x_hwzero_lock(FALSE);	
}

//#endif	/* USING_MROM_FUNCTION */
