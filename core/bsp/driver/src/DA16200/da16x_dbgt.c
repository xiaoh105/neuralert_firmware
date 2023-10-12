/**
 ****************************************************************************************
 *
 * @file da16x_dbgt.c
 *
 * @brief DBGT
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

#undef	SUPPORT_DBGT_ROMLIB

#ifndef	SUPPORT_DBGT_ROMLIB

//---------------------------------------------------------
//	DRIVER :: internal declaration
//---------------------------------------------------------

typedef		struct	_da16200_sysdbg_capture_	DA16X_SYSDBG_TypeDef;

struct		_da16200_sysdbg_capture_
{
	volatile	UINT32  _reserved0[4];

	volatile	UINT32	DBGT_CHAR_DATA;		// 0x0010
	volatile	UINT32	DBGT_TRIG_DATA;
	volatile	UINT32  DBGT_TASK_DATA;
	volatile	UINT32  _reserved1[1];

	volatile	UINT32	DBGT_REQ_CLR;		// 0x0020
	volatile	UINT32	DBGT_OP_EN;
	volatile	UINT32  _reserved2[2];

	volatile	UINT32	DBGT_BASE_ADDR;		// 0x0030
	volatile	UINT32	DBGT_START_ADDR;
	volatile	UINT32	DBGT_TOT_LENGTH;
	volatile	UINT32  _reserved3[1];

	volatile	UINT32	DBGT_CURR_ADDR;		// 0x0040
	volatile	UINT32	DBGT_FIFO_STA;
};

#define DA16X_SYSDEBUG		((DA16X_SYSDBG_TypeDef *)DA16X_SYSDBG_BASE)

#define	JTAG_REQ_CLR		(1<<0)
#define	JTAG_REQ_CAPTURE	(1<<1)

#define	JTAG_PIN_RST		(1<<0)
#define	JTAG_PIN_TDI		(1<<1)
#define	JTAG_PIN_TMS		(1<<2)
#define	JTAG_PIN_TCK		(1<<3)
#define	JTAG_PIN_TDO		(1<<4)

#define	DBGT_CHAR_STX		(0x02)
#define	DBGT_CHAR_ETX		(0x03)

#define	DBGT_REQ_CLEAR		(0x01)

#define	DBGT_FIFO_IDLE		(0)
#define	DBGT_FIFO_WAIT		(1)
#define	DBGT_FIFO_WRRFRC	(2)
#define	DBGT_FIFO_TXDATA	(3)

/******************************************************************************
 *  da16x_sysdbg_capture ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifdef	SUPPORT_DA16X_JTAGCAPTURE
UINT32	da16x_sysdbg_capture(UINT32 mode)
{
	if(mode == TRUE){
		DA16200_SYSBOOT->JTAG_REQ = JTAG_REQ_CAPTURE | JTAG_REQ_CLR ;
		NOP_EXECUTION();
		NOP_EXECUTION();
		NOP_EXECUTION();
		NOP_EXECUTION();
	}

	return DA16200_SYSBOOT->JTAG_PIN_STA;
}
#endif	//SUPPORT_DA16X_JTAGCAPTURE

/******************************************************************************
 *  da16x_sysdbg_dbgt ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	da16x_sysdbg_dbgt(char *str, int len)
{
	UINT32	*str32, *estr32, adata, adata1;
	int offset;

	if( str == NULL ){
		return;
	}

	switch( ((UINT32)str) & 0x03 ){
		case	0x01:  adata = *((UINT32 *)(str - 1)); adata = (adata & 0xffffff00) | (DBGT_CHAR_STX<<0);
			offset = 3;
			break;
		case	0x02:  adata = *((UINT32 *)(str - 2)); adata = (adata & 0xffff0000) | (DBGT_CHAR_ETX<<0)|(DBGT_CHAR_STX<<8);
			offset = 2;
			break;
		case	0x03:  adata = *((UINT32 *)(str - 3)); adata = (adata & 0xff000000) | (DBGT_CHAR_ETX<<0)|(DBGT_CHAR_ETX<<8)|(DBGT_CHAR_STX<<16);
			offset = 1;
			break;
		default	: adata = (DBGT_CHAR_ETX<<0)|(DBGT_CHAR_ETX<<8)|(DBGT_CHAR_ETX<<16)|(DBGT_CHAR_STX<<24);
			offset = 0;
			break;
	}

	str32 = (UINT32 *)((UINT32)str+(UINT32)offset);
	len = len - offset;

	estr32 = (UINT32 *)((UINT32)str32 + (UINT32)(len & (~0x03)));
	len = len & 0x03;

	switch(len){
		case	0x01:  adata1 = *estr32; adata1 = adata1 & 0x000000ff; adata1 |= (DBGT_CHAR_ETX<<24)|(DBGT_CHAR_ETX<<16)|(DBGT_CHAR_ETX<<8);
			break;
		case	0x02:  adata1 = *estr32; adata1 = adata1 & 0x0000ffff; adata1 |= (DBGT_CHAR_ETX<<24)|(DBGT_CHAR_ETX<<16);
			break;
		case	0x03:  adata1 = *estr32; adata1 = adata1 & 0x00ffffff; adata1 |= (DBGT_CHAR_ETX<<24);
			break;
		default	: adata1 = (DBGT_CHAR_ETX<<24)|(DBGT_CHAR_ETX<<16)|(DBGT_CHAR_ETX<<8)|(DBGT_CHAR_ETX<<0);
			break;
	}

	if( (DA16X_SYSDEBUG->DBGT_OP_EN & DBGT_OP_TXCHAR) == DBGT_OP_TXCHAR ){
		// Trace mode
		while( DA16X_SYSDEBUG->DBGT_FIFO_STA != DBGT_FIFO_IDLE );
		DA16X_SYSDEBUG->DBGT_CHAR_DATA = adata;

		while(str32 != estr32){
			while( DA16X_SYSDEBUG->DBGT_FIFO_STA != DBGT_FIFO_IDLE );
			DA16X_SYSDEBUG->DBGT_CHAR_DATA = *str32;
			str32++;
		}

		while( DA16X_SYSDEBUG->DBGT_FIFO_STA != DBGT_FIFO_IDLE );
		DA16X_SYSDEBUG->DBGT_CHAR_DATA = adata1;
	}
	else
	{
		// VSIM mode
		DA16X_SYSDEBUG->DBGT_CHAR_DATA = adata;

		while(str32 != estr32){
			DA16X_SYSDEBUG->DBGT_CHAR_DATA = *str32;
			str32++;
		}

		DA16X_SYSDEBUG->DBGT_CHAR_DATA = adata1;
	}
}

void	da16x_sysdbg_dbgt_trigger(UINT32 value)
{
	if( (DA16X_SYSDEBUG->DBGT_OP_EN & DBGT_OP_TXCHAR) == DBGT_OP_TXCHAR ){
		while( DA16X_SYSDEBUG->DBGT_FIFO_STA != DBGT_FIFO_IDLE );
		DA16X_SYSDEBUG->DBGT_TRIG_DATA = value;
	}else{
		DA16X_SYSDEBUG->DBGT_TRIG_DATA = value;
	}
}

void	da16x_sysdbg_dbgt_init(VOID *baddr, VOID *saddr, UINT32 length, UINT32 mode)
{
	if( baddr == NULL || saddr == NULL ){
		return;
	}

	DA16X_SYSDEBUG->DBGT_OP_EN = mode;

	DA16X_SYSDEBUG->DBGT_BASE_ADDR = (UINT32)baddr;
	DA16X_SYSDEBUG->DBGT_START_ADDR = (UINT32)saddr;
	DA16X_SYSDEBUG->DBGT_TOT_LENGTH = (length & 0x000fffff); // Under 1MB

	DA16X_SYSDEBUG->DBGT_REQ_CLR = DBGT_REQ_CLEAR;
}

UINT32	da16x_sysdbg_dbgt_reinit(UINT32 mode)
{
	if( mode == 0 ){
		// Stop
		DA16X_SYSDEBUG->DBGT_REQ_CLR = DBGT_REQ_CLEAR;
		DA16X_SYSDEBUG->DBGT_OP_EN = 0;
	}else{
		// Toggle : Pause or Restart
		if( DA16X_SYSDEBUG->DBGT_OP_EN == 0 ){
			DA16X_SYSDEBUG->DBGT_OP_EN = mode;
		}else{
			DA16X_SYSDEBUG->DBGT_OP_EN = 0;
		}
	}

	return (DA16X_SYSDEBUG->DBGT_OP_EN);
}

UINT32	da16x_sysdbg_dbgt_dump(UINT32 *baddr, UINT32 *curr)
{
	UINT32 retval;
	*baddr = (UINT32)(DA16X_SYSDEBUG->DBGT_BASE_ADDR);
	*curr = (UINT32)(DA16X_SYSDEBUG->DBGT_CURR_ADDR);
	retval = (DA16X_SYSDEBUG->DBGT_OP_EN) << 24;
	retval = retval | (DA16X_SYSDEBUG->DBGT_TOT_LENGTH);

	return retval;
}

void	da16x_sysdbg_dbgt_backup(void)
{	// DBGT backup
	UINT32 baddr, saddr, tlen;

	if( (tlen = da16x_sysdbg_dbgt_dump(&baddr, &saddr)) != 0 ){

		da16x_boot_set_lock(FALSE);

		if( (baddr > DA16200_RETMEM_BASE) && (baddr < DA16200_RETMEM_END) ){
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTBAS] = baddr ;
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTCUR] = saddr ;
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTLEN] = tlen ; //  Mode & Length
		}else{
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTBAS] = 0 ;
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTCUR] = 0 ;
			((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_DBGTLEN] = 0 ; //  Mode & Length
		}

		da16x_boot_set_lock(TRUE);
	}
}

#endif	/*SUPPORT_DBGT_ROMLIB*/
