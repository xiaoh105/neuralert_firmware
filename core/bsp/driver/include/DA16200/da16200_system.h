/**
 ****************************************************************************************
 *
 * @file da16200_system.h
 *
 * @brief DA16200 System Controller
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


#ifndef	__da16200_system_h__
#define	__da16200_system_h__


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"
#include "common_def.h"

#undef	SUPPORT_DA16X_AUTOBAUD

/******************************************************************************
 * Boot Parameters
 ******************************************************************************/

#define	BTCFG_LOCK_TAG	0xFC9100A1

#define	BOOT_MEM_GET(x)		(((x)>>28)&0x0F)
#define	BOOT_MEM_SET(x)		(((x)&0x0F)<<28)
#define	BOOT_OFFSET_GET(x)	((x)&0x0FFFFFFF)
#define	BOOT_OFFSET_SET(x)	((x)&0x0FFFFFFF)
#define	BOOT_MEMADR_GET(x)	((x)&0x001FFFFF)

#define	BOOT_IDX_RTCM		0
#define	BOOT_IDX_SENM		1
#define	BOOT_IDX_POR		2
#define BOOT_IDX_TAG		3
#define	BOOT_IDX_RLIBLA		4
#define	BOOT_IDX_RLIBAP		5
#define	BOOT_IDX_RLIBSZ		6
#define	BOOT_IDX_OOPS		7
#define	BOOT_IDX_CLKCFG		8
#define	BOOT_IDX_PWRCFG		9
#define	BOOT_IDX_PINCFG		10  /* FIXME: no longer used. */

#define	BOOT_IDX_DBGTBAS	60
#define	BOOT_IDX_DBGTCUR	61
#define	BOOT_IDX_DBGTLEN	62

#define	CONVERT_BOOT_OFFSET(mem,off)	(BOOT_MEM_SET((UINT32)(mem))|BOOT_OFFSET_SET((UINT32)(off)))

extern  void 	da16x_boot_clear_count(void);
extern	void	da16x_boot_set_offset(UINT32 boot, UINT32 imgaddr);
extern	void	da16x_boot_reset_offset(UINT32 boot);
extern	UINT32	da16x_boot_get_offset(UINT32 boot);
extern	void	da16x_boot_set_lock(UINT32 mode);
extern	UINT32	da16x_boot_get_wakeupmode(void);

/******************************************************************************
 * Clock Parameters
 ******************************************************************************/

#define	BOOTCFG_MARK			0x80000000
#define	BOOTCFG_GET_RCOSC(x)		(((x)>>30)&0x01)
#define	BOOTCFG_SET_RCOSC(x)		(((x)&0x01)<<30)
#define	BOOTCFG_GET_XTAL(x)		(((x)>>24)&0x03f)
#define	BOOTCFG_SET_XTAL(x)		(((x)&0x03f)<<24)
#define	BOOTCFG_GET_CLOCK_KHZ(x)	((x)&0x0fffff) /* Max 1GHz */
#define	BOOTCFG_SET_CLOCK_KHZ(x)	((x)&0x0fffff) /* Max 1GHz */

extern	UINT32	da16x_pll_xtal_index(UINT32 xtal);
extern	UINT32	da16x_pll_xtal_clock(UINT32 index);
extern	void	da16x_get_default_xtal(UINT32 *xtalhz, UINT32 *busclock);
extern	void	da16x_set_default_xtal(UINT32 rcosc, UINT32 xtal, UINT32 sysclock);

/******************************************************************************
 * Cache Control
 ******************************************************************************/

extern void	da16x_cache_enable(UINT32 intrmode);
extern void	da16x_cache_disable(void);
extern void	da16x_cache_invalidate(void);

extern UINT32	da16x_cache_get_hitcount(void);
extern UINT32	da16x_cache_get_misscount(void);
extern UINT32	da16x_cache_status(void);

extern void	da16x_cache_clkmgmt(UINT32 code_base);

extern UINT32	da16x_get_cacheoffset(void);
extern void	da16x_set_cacheoffset(UINT32 flashoffset);

#ifdef	BUILD_OPT_DA16200_ASIC

/******************************************************************************
 *
 *  ASIC PLL functions
 *
 ******************************************************************************/

extern UINT32 da16x_pll_on(UINT32 crystal);
extern void da16x_pll_off(void);
extern void da16x_xtal_trim(UINT32 gain, UINT32 ppm);
extern UINT32 da16x_pll_getspeed(UINT32 xtalkhz);

#else	//BUILD_OPT_DA16200_ASIC

/******************************************************************************
 *
 *  FPGA PLL functions
 *
 ******************************************************************************/

#define	da16x_pll_on(...)		TRUE
#define	da16x_pll_off(...)
#define	da16x_xtal_trim(...)
#define	da16x_pll_getspeed(...)		((DA16200FPGA_SYSCON->FPGA_PLL_FREQ & 0x0ff)*KHz)

#endif	//BUILD_OPT_DA16200_ASIC

extern	void	da16x_pll_x2check_experimental(UINT8 *exp);
extern	UINT32	da16x_pll_x2check(void);

/******************************************************************************
 *
 *  UART Baud Detector
 *
 ******************************************************************************/
#ifdef	SUPPORT_DA16X_AUTOBAUD
extern UINT32 da16x_auto_baud_on(void *isrcallback, UINT32 uartclock, UINT32 detclk);
extern UINT32 da16x_auto_baud_off(void);
extern UINT32 da16x_get_curr_baudidx(UINT32 uarthz);
extern UINT32 da16x_get_baud_rate(void);
extern UINT32 da16x_get_baud_count(void);
extern UINT32 da16x_get_baud_index(UINT32 uartbaud);
extern void da16x_baud_detect_core(void);
#else	//SUPPORT_DA16X_AUTOBAUD
#define	da16x_auto_baud_on(...)
#define	da16x_auto_baud_off(...)
#define	da16x_get_curr_baudidx(...)	0
#define	da16x_get_baud_rate(...)		0
#define	da16x_get_baud_count(...)	0
#define	da16x_get_baud_index(...)	0
#define	da16x_baud_detect_core(...)
#endif	//SUPPORT_DA16X_AUTOBAUD

#endif	/*__da16200_system_h__*/

/* EOF */
