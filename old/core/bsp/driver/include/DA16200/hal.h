/**
 * \addtogroup HALayer		Hardware Abstraction Layer
 * \{
 */
 
/**
 ****************************************************************************************
 *
 * @file hal.h
 *
 * @brief Header file for Hardware Abstract Layer
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



#ifndef __hal_h__
#define __hal_h__

//--------------------------------------------------------------------
//	Register Map
//--------------------------------------------------------------------

#undef	__OM
#include "CMSDK_CM4.h"

#include "core_cm4.h"

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------
#include "da16200_map.h"
#include "da16200_retmem.h"
#include "oal.h"

//--------------------------------------------------------------------
//	Retargeted Console & Pool
//--------------------------------------------------------------------

#include "hal_primitives.h"

//--------------------------------------------------------------------
// Peripheral options
//--------------------------------------------------------------------

#define	SUPPORT_USLEEP

// enable kDMA
#undef	SUPPORT_UDMA
#define	SUPPORT_KDMA

#undef  SUPPORT_SYSUART
#define SUPPORT_PL011UART


//--------------------------------------------------------------------
// Peripheral Headers
//--------------------------------------------------------------------

#include "da16200_system.h"
#include "da16200_ioconfig.h"

#include "sys_nvic.h"
#include "sys_clock.h"
#include "sys_tick.h"
#include "wdog.h"
#include "simpletimer.h"
#include "dualtimer.h"
#include "usleep.h"
#include "gpio.h"
#include "sys_dma.h"	// PL230 UDMA : test only
#include "sys_kdma.h"	// PL230 UDMA : test only
#include "sys_uart.h"	// CMSDK UART : test only
#include "uart.h"	// PL011 UART : test only
#include "rtc.h"	// DA16200 RTC : test only
#include "fdma.h"
#include "sys_i2c.h"

#include "da16x_btm.h"
#include "da16x_dbgt.h"
#include "da16x_hwzero.h"
#include "da16x_hwcrc32.h"
#include "da16x_rng.h"
#include "softirq.h"
#include "da16x_rtcifbcfm.h"
#include "da16x_syssecure.h"
#include "da16x_abmsecure.h"
#include "da16x_otp.h"

#include "pwm.h"

#include "emmc.h"
#include "sdio_slave.h"
#include "xfc.h"
#include "spi.h"

/******************************************************************************
 *
 *  HAL Macro
 *
 ******************************************************************************/

#define	da16xfunc_ntohl(x)	*((UINT32 *)(DA16200_SRAM32S_BASE|((UINT32)&(x))))

#ifdef	BUILD_OPT_VSIM
/******************************************************************************
 *
 *  Simulation Model
 *
 ******************************************************************************/


//--------------------------------------------------------------------
// Retargeted primitives
//--------------------------------------------------------------------

#ifdef	SUPPORT_USLEEP
#define	SYSUSLEEP(x)			USLEEP_SUSPEND(USLEEP_HANDLER(), (x))
#else 	//SUPPORT_USLEEP
#define	SYSUSLEEP(x)			OAL_MSLEEP((x)/1000)
#endif 	//SUPPORT_USLEEP

#define	DRV_PRINTF(...)			DBGT_Printf( __VA_ARGS__ )
#define	DRV_VPRINTF(...)
#define DRV_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	DRV_GETC( )				0x03 //((_sys_uart_rxempty() == FALSE)? getchar():0x00)
#define	DRV_GETC_NOWAIT()		0x03 //((_sys_uart_rxempty() == FALSE)? getchar():0x00)
#define DRV_PUTC(c)
#define DRV_PUTS(s)

#define DRV_STRLEN(...)			strlen( __VA_ARGS__ )
#define	DRV_STRCPY(...)			strcpy( __VA_ARGS__ )
#define DRV_STRCMP(...)			strcmp( __VA_ARGS__ )
#define DRV_STRCAT(...)			strcat( __VA_ARGS__ )
#define DRV_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define DRV_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define DRV_STRCHR(...)			strchr( __VA_ARGS__ )

#define DRV_MEMCPY(...)			memcpy( __VA_ARGS__ )
#define DRV_MEMCMP(...)			memcmp( __VA_ARGS__ )
#define DRV_MEMSET(...)			memset( __VA_ARGS__ )

#define DRV_DBG_NONE(...)
#define DRV_DBG_BASE(...)
#define DRV_DBG_INFO(...)
#define DRV_DBG_WARN(...)
#define DRV_DBG_ERROR(...)
#define DRV_DBG_DUMP(...)
#define DRV_DBG_TEXT(...)

#else	//BUILD_OPT_VSIM
/******************************************************************************
 *
 *  ASIC & FPGA Model
 *
 ******************************************************************************/

//--------------------------------------------------------------------
// Retargeted primitives
//--------------------------------------------------------------------
extern int da16x_sprintf(char *buf, const char *fmt, ...);

#ifdef	SUPPORT_USLEEP
#define	SYSUSLEEP(x)				USLEEP_SUSPEND(USLEEP_HANDLER(), (x))
#else 	//SUPPORT_USLEEP
#define	SYSUSLEEP(x)				OAL_MSLEEP((x)/1000)
#endif 	//SUPPORT_USLEEP

#define	DRV_PRINTF(...)				embhal_print(0, __VA_ARGS__ )
#define	DRV_VPRINTF(...)			embhal_vprint(0, __VA_ARGS__ )
#define DRV_SPRINTF(...)			da16x_sprintf( __VA_ARGS__ )
#define	DRV_GETC()					embhal_getchar(OAL_SUSPEND)
#define	DRV_GETC_NOWAIT()			embhal_getchar(OAL_NO_SUSPEND)
#define DRV_PUTC(ch)				embhal_print(0, "%c", ch )
#define DRV_PUTS(s)					embhal_print(0, s )

#define DRV_STRLEN(...)				strlen( __VA_ARGS__ )
#define	DRV_STRCPY(...)				strcpy( __VA_ARGS__ )
#define DRV_STRCMP(...)				strcmp( __VA_ARGS__ )
#define DRV_STRCAT(...)				strcat( __VA_ARGS__ )
#define DRV_STRNCMP(...)			strncmp( __VA_ARGS__ )
#define DRV_STRNCPY(...)			strncpy( __VA_ARGS__ )
#define DRV_STRCHR(...)				strchr( __VA_ARGS__ )

#define DRV_MEMCPY(...)				memcpy( __VA_ARGS__ )
#define DRV_MEMCMP(...)				memcmp( __VA_ARGS__ )
#define DRV_MEMSET(...)				memset( __VA_ARGS__ )

#define DRV_DBG_NONE(...)			embhal_print(5, __VA_ARGS__ )
#define DRV_DBG_BASE(...)			embhal_print(4, __VA_ARGS__ )
#define DRV_DBG_INFO(...)			embhal_print(3, __VA_ARGS__ )
#define DRV_DBG_WARN(...)			embhal_print(2, __VA_ARGS__ )
#define DRV_DBG_ERROR(...)			embhal_print(1, __VA_ARGS__ )
#define DRV_DBG_DUMP(tag, ...)		embhal_dump(tag, __VA_ARGS__ )
#define DRV_DBG_TEXT(tag, ...)		embhal_text(tag, __VA_ARGS__ )

#endif	//BUILD_OPT_VSIM


//! Test if in interrupt mode
inline int isInterrupt()
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0 ;
}

#include "da16x_compile_opt.h"

#endif /* __hal_h__ */

/**
 * \}
 */
