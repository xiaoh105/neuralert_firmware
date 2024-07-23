/**
 ****************************************************************************************
 *
 * @file da16x_btm.c
 *
 * @brief BTM Controller
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

//---------------------------------------------------------
// BTM
//---------------------------------------------------------

typedef		struct	_da16x_btmcon_	DA16X_BTMCON_TypeDef;

struct		_da16x_btmcon_			// 0x5004_0000
{
	volatile UINT32	BTM_REQ;		// RW, 0x0000
	volatile UINT32	BTM_CH;			// RW, 0x0004
	volatile UINT32	BTM_CH_SEL[2];		// RW, 0x0008

	volatile UINT32 BTM_C[8];		// RO, 0x0010

	volatile UINT32	BTM_MODE;		// RW, 0x0030
	volatile UINT32	BTM_EXPIRE;		// RW, 0x0034
	volatile UINT32 BTM_IRQ_STA;		// RO, 0x0038
};

#define	BTM_REQ_STOP	0x0004
#define	BTM_REQ_START	0x0002
#define	BTM_REQ_CLEAR	0x0001
#define	BTM_REQ_MCWCLR	0x0100

#define	BTM_EXPIRE_ON	0x0001
#define	BTM_EXPIRE_IRQ	0x0002

/******************************************************************************
 *  da16x_btm_control
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define DA16X_BTMCON	((DA16X_BTMCON_TypeDef *)DA16X_BTMCON_BASE)

void	da16x_btm_control(UINT32 mode)
{
	if (mode == TRUE ) {
		DA16X_BTMCON->BTM_REQ = BTM_REQ_START | BTM_REQ_CLEAR ;
	}
	else {
		DA16X_BTMCON->BTM_REQ = BTM_REQ_STOP ;
	}
}

#ifdef	SUPPORT_BTM_NO_VENEER
// Skip
#else	//SUPPORT_BTM_NO_VENEER
UINT32	da16x_btm_get_timestamp(void)
{
	return DA16X_BTMCON->BTM_C[0];
}
#endif	//SUPPORT_BTM_NO_VENEER

void	da16x_btm_ch_setup(UINT32 ch, UINT32 mode)
{
	if(ch == 5){
		DA16X_BTMCON->BTM_CH &= ~((0x3 << (5+8)) | (0x1 << 5));
		DA16X_BTMCON->BTM_CH |= ((mode & 0x3) << (5+8));
	}else
	if( ch != 7 ){
		DA16X_BTMCON->BTM_CH &= ~((0x1 << (ch+8)) | (0x1 << ch));
		DA16X_BTMCON->BTM_CH |= ((mode & 0x1) << (ch+8));
	}

	if( mode != BTM_CHM_IDL ) {
		DA16X_BTMCON->BTM_CH |= (0x1 << ch);
	}
}

void	da16x_btm_ch_select(UINT32 ch, UINT32 mode)
{
	switch(ch){
	case	0:	DA16X_BTMCON->BTM_CH_SEL[0] &= ~(0x1f << 0);
			DA16X_BTMCON->BTM_CH_SEL[0] |= ((mode & 0x1f) << 0);
			break;
	case	1:	DA16X_BTMCON->BTM_CH_SEL[0] &= ~(0x1f << 8);
			DA16X_BTMCON->BTM_CH_SEL[0] |= ((mode & 0x1f) << 8);
			break;
	case	2:	DA16X_BTMCON->BTM_CH_SEL[0] &= ~(0x1f << 16);
			DA16X_BTMCON->BTM_CH_SEL[0] |= ((mode & 0x1f) << 16);
			break;
	case	3:	DA16X_BTMCON->BTM_CH_SEL[0] &= ~(0x1f << 24);
			DA16X_BTMCON->BTM_CH_SEL[0] |= ((mode & 0x1f) << 24);
			break;
	case	4:	DA16X_BTMCON->BTM_CH_SEL[1] &= ~(0x1f << 0);
			DA16X_BTMCON->BTM_CH_SEL[1] |= ((mode & 0x1f) << 0);
			break;
	case	5:	DA16X_BTMCON->BTM_CH_SEL[1] &= ~(0x1f << 8);
			DA16X_BTMCON->BTM_CH_SEL[1] |= ((mode & 0x1f) << 8);
			break;
	case	6:	DA16X_BTMCON->BTM_CH_SEL[1] &= ~(0x1f << 16);
			DA16X_BTMCON->BTM_CH_SEL[1] |= ((mode & 0x1f) << 16);
			DA16X_BTMCON->BTM_REQ = BTM_REQ_MCWCLR ;
			break;
	}
}

void	da16x_btm_get_statistics(UINT32 mode, UINT32 *data)
{
	UINT32	flag;

	if(mode == TRUE ){
		DA16X_BTMCON->BTM_REQ = BTM_REQ_STOP ;
	}

	data[0] = DA16X_BTMCON->BTM_C[0];
	data[1] = DA16X_BTMCON->BTM_C[1];
	data[2] = DA16X_BTMCON->BTM_C[2];
	data[3] = DA16X_BTMCON->BTM_C[3];
	data[4] = DA16X_BTMCON->BTM_C[4];
	data[5] = DA16X_BTMCON->BTM_C[5];
	data[6] = DA16X_BTMCON->BTM_C[6];
	data[7] = DA16X_BTMCON->BTM_C[7];

	data[8] = DA16X_BTMCON->BTM_CH_SEL[0] ;
	data[9] = DA16X_BTMCON->BTM_CH_SEL[1] ;

	flag = DA16X_BTMCON->BTM_CH ;
	if(flag & 0x01){
		data[8] |= (0x80 | ((flag & 0x100) >> 3));
	}
	if(flag & 0x02){
		data[8] |= ((0x80 | ((flag & 0x200) >> 4)) << 8);
	}
	if(flag & 0x04){
		data[8] |= ((0x80 | ((flag & 0x400) >> 5)) << 16);
	}
	if(flag & 0x08){
		data[8] |= ((0x80 | ((flag & 0x800) >> 6)) << 24);
	}
	if(flag & 0x10){
		data[9] |= ((0x80 | ((flag & 0x1000) >> 7)) << 0);
	}
	if(flag & 0x20){
		data[9] |= ((0x80 | ((flag & 0x6000) >> 8)) << 8);
	}
	if(flag & 0x40){
		data[9] |= (0x80 << 16);
	}

	if(mode == TRUE ){
		DA16X_BTMCON->BTM_REQ = BTM_REQ_START | BTM_REQ_CLEAR ;
	}
}
