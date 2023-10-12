/**
 ****************************************************************************************
 *
 * @file da16x_dbgt.h
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


#ifndef __da16x_dbgt_h__
#define __da16x_dbgt_h__

#include "hal.h"

#undef	SUPPORT_DA16X_JTAGCAPTURE

#define	DBGT_OP_TXTRGEN		(0x04)
#define	DBGT_OP_FRUNCOUNT	(0x02)
#define	DBGT_OP_TXCHAR		(0x01)

#ifdef	SUPPORT_DA16X_JTAGCAPTURE
extern	UINT32	da16x_sysdbg_capture(UINT32 mode);
#else	//SUPPORT_DA16X_JTAGCAPTURE
#define	da16x_sysdbg_capture(...)	0
#endif	//SUPPORT_DA16X_JTAGCAPTURE
extern	void	da16x_sysdbg_dbgt(char *str, int len);
extern	void	da16x_sysdbg_dbgt_trigger(UINT32 value);
extern	void	da16x_sysdbg_dbgt_init(VOID *baddr, VOID *saddr, UINT32 length, UINT32 mode);
extern	UINT32	da16x_sysdbg_dbgt_reinit(UINT32 mode);
extern	UINT32	da16x_sysdbg_dbgt_dump(UINT32 *baddr, UINT32 *curr);
extern	void	da16x_sysdbg_dbgt_backup(void);

#endif /* __da16x_dbgt_h__ */
