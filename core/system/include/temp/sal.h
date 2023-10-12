/**
 ****************************************************************************************
 *
 * @file sal.h
 *
 * @brief DA16200 internal Peripherals
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


#ifndef __sal_h__
#define __sal_h__

//--------------------------------------------------------------------
//	Register Map
//--------------------------------------------------------------------

#include "CMSDK_CM4.h"

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------

#include "target.h"
#include "oal.h"

//--------------------------------------------------------------------
//	Retargeted Console & Pool
//--------------------------------------------------------------------

#include "oal_primitives.h"

//--------------------------------------------------------------------
//	SAL
//--------------------------------------------------------------------

#include "clib.h"
#include "console.h"
#include "sys_exception.h"


//--------------------------------------------------------------------
//	Macro
//--------------------------------------------------------------------

#define	SAL_MALLOC(...)			OAL_MALLOC( __VA_ARGS__ )
#define	SAL_FREE(...)			OAL_FREE( __VA_ARGS__ )

#define	SAL_PRINTF(...)			OAL_PRINTF( __VA_ARGS__ )
#define	SAL_VPRINTF(...)		OAL_VPRINTF( __VA_ARGS__ )
#define SAL_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	SAL_GETC()				OAL_GETC( )
#define	SAL_GETC_NOWAIT()		OAL_GETC_NOWAIT( )
#define SAL_PUTC(...)			OAL_PUTC( __VA_ARGS__ )
#define SAL_PUTS(...)			OAL_PUTS( __VA_ARGS__ )

#define SAL_STRLEN(...)			OAL_STRLEN( __VA_ARGS__ )
#define	SAL_STRCPY(...)			OAL_STRCPY( __VA_ARGS__ )
#define SAL_STRCMP(...)			OAL_STRCMP( __VA_ARGS__ )
#define SAL_STRCAT(...)			OAL_STRCAT( __VA_ARGS__ )
#define SAL_STRNCMP(...)		OAL_STRNCMP( __VA_ARGS__ )
#define SAL_STRNCPY(...)		OAL_STRNCPY( __VA_ARGS__ )
#define SAL_STRCHR(...)			OAL_STRCHR( __VA_ARGS__ )

#define SAL_MEMCPY(...)			OAL_MEMCPY( __VA_ARGS__ )
#define SPL_MEMCMP(...)			OAL_MEMCMP( __VA_ARGS__ )
#define SAL_MEMSET(...)			OAL_MEMSET( __VA_ARGS__ )

#define SAL_DBG_NONE(...)		OAL_DBG_NONE( __VA_ARGS__ )
#define SAL_DBG_BASE(...)		OAL_DBG_BASE( __VA_ARGS__ )
#define SAL_DBG_INFO(...)		OAL_DBG_INFO( __VA_ARGS__ )
#define SAL_DBG_WARN(...)		OAL_DBG_WARN( __VA_ARGS__ )
#define SAL_DBG_ERROR(...)		OAL_DBG_ERROR( __VA_ARGS__ )
#define SAL_DBG_DUMP(...)		OAL_DBG_DUMP( __VA_ARGS__ )
#define SAL_DBG_TEXT(...)		OAL_DBG_TEXT( __VA_ARGS__ )


#endif /* __sal_h__ */

/* EOF */
