/**
 ****************************************************************************************
 *
 * @file da16x_btm.h
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

#ifndef	__da16x_btm_h__
#define	__da16x_btm_h__


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

#define	SUPPORT_BTM_NO_VENEER

#define	BTM_CHM_CMD		0
#define	BTM_CHM_WTM		1
#define	BTM_CHM_SLP		2
#define	BTM_CHM_IDL		3

extern void	da16x_btm_control(UINT32 mode);
#ifdef	SUPPORT_BTM_NO_VENEER
#define	da16x_btm_get_timestamp()	*((volatile UINT32 *)(DA16X_BTMCON_BASE|0x010))
#else	//SUPPORT_BTM_NO_VENEER
extern UINT32	da16x_btm_get_timestamp(void);
#endif	//SUPPORT_BTM_NO_VENEER
extern void	da16x_btm_ch_setup(UINT32 ch, UINT32 mode);
extern void	da16x_btm_ch_select(UINT32 ch, UINT32 mode);
extern void	da16x_btm_get_statistics(UINT32 mode, UINT32 *data);

#endif	/*__da16x_btm_h__*/
