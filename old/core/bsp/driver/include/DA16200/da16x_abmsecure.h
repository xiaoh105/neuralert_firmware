/**
 ****************************************************************************************
 *
 * @file da16x_abmsecure.h
 *
 * @brief ABMSECURE
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

#ifndef __da16x_abmsecure_h__
#define __da16x_abmsecure_h__

#include "hal.h"

/******************************************************************************
 *
 * DA16200 ABMSECURE
 *
 ******************************************************************************/
typedef		struct	_da16x_abmsecure_	da16x_abmsecure_TypeDef;

struct		_da16x_abmsecure_
{
	volatile UINT32	c_abm_req_clr_op;	// 0x00
	volatile UINT32	c_abm_req_clr_irq;	// 0x04
	volatile UINT32	c_abm_req_start;	// 0x08
	volatile UINT32	_reserve0;		// 0x0C

	volatile UINT32	c_abm_op_en;		// 0x10
	volatile UINT32	c_abm_msk_en_irq;	// 0x14
	volatile UINT32	c_am2d_op_auto_off;	// 0x18
	volatile UINT32	_reserve1;		// 0x1C

	volatile UINT32	ABM_MGM_CONFIG;		// 0x20
	volatile UINT32	ABM_MGM_CTRL;		// 0x24
	volatile UINT32	ABM_MGM_S_ADDR;		// 0x28
	volatile UINT32	ABM_MGM_D_ADDR;		// 0x2C

	volatile UINT32	ABM_AO_CONFIG;		// 0x30
	volatile UINT32	ABM_AO_CTRL;		// 0x34
	volatile UINT32	ABM_AO_S_ADDR;		// 0x38
	volatile UINT32	ABM_AO_D_ADDR;		// 0x3C

	volatile UINT32	ABM_ZERO_CONFIG;	// 0x40
	volatile UINT32	ABM_ZERO_CTRL;		// 0x44
	volatile UINT32	ABM_ZERO_S_ADDR;	// 0x48
	volatile UINT32	ABM_ZERO_D_ADDR;	// 0x4C

	volatile UINT32	_reserve2[40];		// 0x50-EC

	volatile UINT32	ABM_OTP_CONFIG;		// 0xF0
	volatile UINT32	ABM_OTP_CTRL;		// 0xF4
	volatile UINT32	ABM_OTP_S_ADDR;		// 0xF8
	volatile UINT32	ABM_OTP_D_ADDR;		// 0xFC

	volatile UINT32	ABM_ZERO_REQ;		// 0x100
	volatile UINT32	ABM_ZERO_REQCTRL;	// 0x104
	volatile UINT32	ABM_ZERO_START_ADDR;	// 0x108
	volatile UINT32	ABM_ZERO_TOT_LENGTH;	// 0x10C
};

#define da16x_abmsecure	((da16x_abmsecure_TypeDef *)ABM_SECURE_BASE)

#endif /* __da16x_abmsecure_h__ */
