/**
 ****************************************************************************************
 *
 * @file da16x_hwcrc32.c
 *
 * @brief HWCrc32
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

//---------------------------------------------------------
// CRC
//---------------------------------------------------------

#define	DA16X_CRCCON_BASE	(DA16200_HWCRC32_BASE)

typedef		struct	_da16x_crccon_	DA16X_CRCCON_TypeDef;

struct		_da16x_crccon_			// 0x5004_0200
{
	volatile UINT32	REQ;			// RW, 0x0000
	volatile UINT32	OP_EN;			// RW, 0x0004
	volatile UINT32	TYPCTRL;		// RW, 0x0008
	volatile UINT32 SEED;			// RW, 0x000C
	volatile UINT32 ADDR_MIN;		// RW, 0x0010
	volatile UINT32 ADDR_MAX;		// RW, 0x0014
	volatile UINT32 PSEUDO;			// RW, 0x0018
	volatile UINT32 _reserved;
	volatile UINT32 CAL_VAL;		// RO, 0x0020
	volatile UINT32 CAL_STA;		// RO, 0x0024
	volatile UINT32 CAL_VAL_REV;		// RO, 0x0028
};

#define DA16X_CRCCON		((DA16X_CRCCON_TypeDef *)DA16X_CRCCON_BASE)

#define	CRC_REQ_STOP		0x0004
#define	CRC_REQ_START		0x0002
#define	CRC_REQ_CLEAR		0x0001

#define	CRC_CAL_IDLE		(0<<24)
#define	CRC_CAL_BUSY		(1<<24)
#define	CRC_CAL_STOP		(2<<24)

#define	CRC_CAL_NUM_MSK		(0x0fffff)


#define	CRC_FDMA_SRC_TYPE	0
#define	CRC_FDMA_SRC_PORT	0x17
#define	CRC_FDMA_COPY_SIZE	(0x100000)

static void fdma_accelerator_intr_callback(void *param);

/******************************************************************************
 *
 * CRC32 functions
 *
 ******************************************************************************/

void	da16x_crc32_start(UINT32 mode, UINT32 startaddr, UINT32 endaddr,  UINT32 seed)
{
	DA16X_CLKGATE_ON(clkoff_crc32_cal);
	// Active
	DA16X_CRCCON->OP_EN   = 1;
	DA16X_CRCCON->TYPCTRL = mode;
	// Range
	DA16X_CRCCON->ADDR_MIN = (UINT32)startaddr;
	DA16X_CRCCON->ADDR_MAX = (UINT32)endaddr;
	// Start
	DA16X_CRCCON->SEED    = seed;
	DA16X_CRCCON->REQ     = CRC_REQ_CLEAR|CRC_REQ_START;
}

void	da16x_crc32_restart(void)
{
	DA16X_CLKGATE_ON(clkoff_crc32_cal);
	// Active
	DA16X_CRCCON->OP_EN   = 1;
	// Restart
	DA16X_CRCCON->REQ     = CRC_REQ_START;
}

UINT32	da16x_crc32_stop(void)
{
	UINT32	crc32value;

	// Stop
	DA16X_CRCCON->REQ     = CRC_REQ_STOP;
	crc32value = DA16X_CRCCON->CAL_VAL;
	// Inactive
	DA16X_CRCCON->OP_EN   = 0;

	DA16X_CLKGATE_OFF(clkoff_crc32_cal);

	return crc32value;
}

UINT32	da16x_crc32_pause(void)
{
	UINT32	crc32temp;

	// Stop
	DA16X_CRCCON->REQ     = CRC_REQ_STOP;
	crc32temp = DA16X_CRCCON->CAL_VAL_REV;
	// Inactive
	DA16X_CRCCON->OP_EN   = 0;

	DA16X_CLKGATE_OFF(clkoff_crc32_cal);

	return crc32temp;
}

void	da16x_crc32_pseudo8(UINT8 *value, UINT32 length)
{
	while( length > 0 ){
		DA16X_CRCCON->PSEUDO     = *value++;
		length --;
	}
}

void	da16x_crc32_pseudo16(UINT16 *value, UINT32 length)
{
	length = length >> 1;
	while( length > 0 ){
		DA16X_CRCCON->PSEUDO     = *value++;
		length --;
	}
}

void	da16x_crc32_pseudo32(UINT32 *value, UINT32 length)
{
	length = length >> 2;
	while( length > 0 ){
		DA16X_CRCCON->PSEUDO     = *value++;
		length --;
	}
}

/******************************************************************************
 *  da16x_crc32 ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_hwcrc32(UINT32 dwidth, UINT8 *data, UINT32 length, UINT32 seed)
{
	return da16x_hwcopycrc32( dwidth, (UINT8 *)0xFFFFFFFF, data, length, seed ); // DST is NULL !!
}

UINT32	da16x_hwcopycrc32(UINT32 dwidth, UINT8 *dst, UINT8 *src, UINT32 length, UINT32 seed)
{
	HANDLE	channel;
	UINT32	acclen, transmode, transiz;
	UINT32	tmpwriteregion[1];
	UINT32	writeregion;
	UINT32	readregion;
	UINT32	fdma_control, fdma_config, fdma_chan_id, event;
	UINT32	crc32value;

	switch((dwidth & CRC_CTRL_DWMASK)){
		case	4:	acclen = FDMA_CHAN_HSIZE_4B;
				transmode = (CRC_DAT_TYPE(0)|CRC_PAR_TYPE(2)|CRC_ACC_TYPE(CRC_FDMA_SRC_TYPE)|CRC_PATH_SEL(CRC_FDMA_SRC_PORT));
				break;
		case	2:	acclen = FDMA_CHAN_HSIZE_2B;
				transmode = (CRC_DAT_TYPE(0)|CRC_PAR_TYPE(1)|CRC_ACC_TYPE(CRC_FDMA_SRC_TYPE)|CRC_PATH_SEL(CRC_FDMA_SRC_PORT));
				break;
		default:	acclen = FDMA_CHAN_HSIZE_1B;
				transmode = (CRC_DAT_TYPE(0)|CRC_PAR_TYPE(0)|CRC_ACC_TYPE(CRC_FDMA_SRC_TYPE)|CRC_PATH_SEL(CRC_FDMA_SRC_PORT));
				break;
	}

	if( dst == (UINT8 *)0xFFFFFFFF ){ // if DST is NULL,
		writeregion = CONVERT_C2SYS_MEM((UINT32)&(tmpwriteregion[0])) ;
	}else
	if( (DA16X_SRAM_BASE >= ((UINT32)dst)) && (DA16X_SRAM_END <= ((UINT32)dst)) ){
		writeregion = CONVERT_C2SYS_MEM((UINT32)dst);
	}else{
		writeregion = (UINT32)dst;
	}
	if( (DA16X_SRAM_BASE >= ((UINT32)src)) && (DA16X_SRAM_END <= ((UINT32)src)) ){
		readregion = CONVERT_C2SYS_MEM((UINT32)src);
	}else{
		readregion = (UINT32)src;
	}

	da16x_crc32_start( transmode, (UINT32)readregion, ((UINT32)readregion + length), seed );
	//da16x_crc32_start( transmode, (UINT32)0, ((UINT32)~0), seed );

	if( length >= 128 )
	{
		UINT32 bkupnorype;

		// if DST is NULL,
		transmode = ( dst == (UINT8 *)0xFFFFFFFF ) ? 0 : FDMA_CHAN_AINCR ;

		// dma_setup
		fdma_control = FDMA_CHAN_CONTROL(0,1);
		fdma_config = FDMA_CHAN_CONFIG(
				transmode, FDMA_CHAN_AHB1, acclen
				, FDMA_CHAN_AINCR, FDMA_CHAN_AHB0, acclen
				);
		fdma_config |= FDMA_CHAN_BURST(10) ;
		fdma_chan_id = 0xFDC00000 + length;

		channel = FDMA_OBTAIN_CHANNEL(FDMA_SYS_GET_HANDLER(), fdma_chan_id, fdma_control, fdma_config, 1);

		FDMA_LOCK_CHANNEL(channel);

	////////////////////////////////////////////////////////
	if((dwidth & CRC_CTRL_CACHE)){
		bkupnorype = DA16XFPGA_GET_NORTYPE();
		if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
			da16x_cache_disable();
		}
		DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // FLASH access
	}
	////////////////////////////////////////////////////////


		for( acclen = 0, transiz = CRC_FDMA_COPY_SIZE ; acclen  < length;  )
		{
			transiz = ( ( length - acclen ) > CRC_FDMA_COPY_SIZE )
				? CRC_FDMA_COPY_SIZE
				: ( length - acclen ) ;

			event = FALSE;

			FDMA_SCATTER_CHANNEL( channel
				, (void *)writeregion
				, (void *)readregion
				, transiz
				, (UINT32)&fdma_accelerator_intr_callback, (UINT32)(&event));


			FDMA_START_CHANNEL(channel);

			readregion = ((UINT32)readregion+transiz);

			if( dst != (UINT8 *)0xFFFFFFFF ){ // if DST is not NULL,
				writeregion = ((UINT32)writeregion+transiz);
			}

			acclen  += transiz ;

			while( event == FALSE){
				WAIT_FOR_INTR();
			}

		}

	////////////////////////////////////////////////////////
	if((dwidth & CRC_CTRL_CACHE)){
		DA16XFPGA_SET_NORTYPE(bkupnorype);	// FLASH access
		if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
			da16x_cache_enable(FALSE);
		}
	}
	////////////////////////////////////////////////////////

		FDMA_UNLOCK_CHANNEL(channel);

		FDMA_RELEASE_CHANNEL( FDMA_SYS_GET_HANDLER(), channel );
	}
	else
	{
		if( dst != (UINT8 *)0xFFFFFFFF ){ // if DST is not NULL,
			DRV_MEMCPY(dst, src, length );
		}

		switch((dwidth & CRC_CTRL_DWMASK)){
			case	4:
				da16x_crc32_pseudo32((UINT32 *)src, length);
				break;
			case	2:
				da16x_crc32_pseudo16((UINT16 *)src, length);
				break;
			default:
				da16x_crc32_pseudo8(src, length);
				break;
		}

	}

	crc32value = da16x_crc32_stop();

	return crc32value;
}

/******************************************************************************
 *  fdma_accelerator_intr_callback ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void fdma_accelerator_intr_callback(void *param)
{
	UINT32	*event;

	if( param == NULL){
		return ;
	}

	event = (UINT32 *) param;
	*event = TRUE;
}

