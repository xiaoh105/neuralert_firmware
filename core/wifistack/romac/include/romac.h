/**
 ****************************************************************************************
 *
 * @file romac.h
 *
 * @brief DA16200 romac
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



#ifndef __ROMAC_h__
#define __ROMAC_h__

/**
 * @file     romac.h
 * @brief    Implementing PTIM & RomMAC
 * @details  This header file should only be included for implementation.
 * @addtogroup ROMAC
 * @{
 */

//--------------------------------------------------------------------
//	Dependency
//--------------------------------------------------------------------

#ifndef TIM_SIMULATION
#include "oal.h"
#include "sal.h"
#endif

#include "hal.h"
#include <stdio.h>
#include "timp.h"

//--------------------------------------------------------------------
// Peripheral options
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Peripheral Headers
//--------------------------------------------------------------------

/******************************************************************************
 *
 *  romac Macro
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

#define	ROMAC_PRINTF(...)		DBGT_Printf( __VA_ARGS__ )
#define	ROMAC_VPRINTF(...)
#define ROMAC_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	ROMAC_GETC()			0x03
#define	ROMAC_GETC_NOWAIT()		0x03
#define ROMAC_PUTC(ch)
#define ROMAC_PUTS(s)
#define ROMAC_STRLEN(...)		strlen( __VA_ARGS__ )
#define	ROMAC_STRCPY(...)		strcpy( __VA_ARGS__ )
#define ROMAC_STRCMP(...)		strcmp( __VA_ARGS__ )
#define ROMAC_STRCAT(...)		strcat( __VA_ARGS__ )
#define ROMAC_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define ROMAC_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define ROMAC_STRCHR(...)		strchr( __VA_ARGS__ )

#define ROMAC_MEMCPY(...)		memcpy( __VA_ARGS__ )
#define ROMAC_MEMCMP(...)		memcmp( __VA_ARGS__ )
#define ROMAC_MEMSET(...)		embromac_memset( __VA_ARGS__ )

#define ROMAC_DBG_NONE(...)
#define ROMAC_DBG_BASE(...)
#define ROMAC_DBG_INFO(...)
#define ROMAC_DBG_WARN(...)
#define ROMAC_DBG_ERROR(...)
#define ROMAC_DBG_DUMP(...)
#define ROMAC_DBG_TEXT(...)

#else	//BUILD_OPT_VSIM
/******************************************************************************
 *
 *  ASIC & FPGA Model
 *
 ******************************************************************************/

//--------------------------------------------------------------------
// Retargeted primitives
//--------------------------------------------------------------------

#define	ROMAC_PRINTF(...)		embromac_print(0, __VA_ARGS__ )
#define	ROMAC_VPRINTF(...)		embromac_vprint(0, __VA_ARGS__ )
#define ROMAC_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	ROMAC_GETC()			embromac_getchar(OAL_SUSPEND)
#define	ROMAC_GETC_NOWAIT()		embromac_getchar(OAL_NO_SUSPEND)
#define ROMAC_PUTC(ch)			embromac_print(0, "%c", ch )
#define ROMAC_PUTS(s)			embromac_print(0, s )

#define ROMAC_STRLEN(...)		strlen( __VA_ARGS__ )
#define	ROMAC_STRCPY(...)		strcpy( __VA_ARGS__ )
#define ROMAC_STRCMP(...)		strcmp( __VA_ARGS__ )
#define ROMAC_STRCAT(...)		strcat( __VA_ARGS__ )
#define ROMAC_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define ROMAC_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define ROMAC_STRCHR(...)		strchr( __VA_ARGS__ )

#define ROMAC_MEMCPY(...)		memcpy( __VA_ARGS__ )
#define ROMAC_MEMCMP(...)		memcmp( __VA_ARGS__ )
#define ROMAC_MEMSET(...)		embromac_memset( __VA_ARGS__ )

#define ROMAC_DBG_NONE(...)		embromac_print(5, __VA_ARGS__ )
#define ROMAC_DBG_BASE(...)		embromac_print(4, __VA_ARGS__ )
#define ROMAC_DBG_INFO(...)		embromac_print(3, __VA_ARGS__ )
#define ROMAC_DBG_WARN(...)		embromac_print(2, __VA_ARGS__ )
#define ROMAC_DBG_ERROR(...)		embromac_print(1, __VA_ARGS__ )
#define ROMAC_DBG_DUMP(tag, ...)	embromac_dump(tag, __VA_ARGS__ )
#define ROMAC_DBG_TEXT(tag, ...)	embromac_text(tag, __VA_ARGS__ )

#endif	//BUILD_OPT_VSIM

/// @} end of group ROMAC

#endif /* __ROMAC_h__ */
