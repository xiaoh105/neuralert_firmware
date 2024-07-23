/**
 * \addtogroup CRYLayer	Crypto Abstraction Layer
 * \{
 * \brief Cryptography Drviers
 */

/**
 ****************************************************************************************
 *
 * @file cal.h
 *
 * @brief DA16200 crypto
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


#ifndef __cal_h__
#define __cal_h__

//--------------------------------------------------------------------
//	Dependency
//--------------------------------------------------------------------

#include "oal.h"
#include "sal.h"
#include "hal.h"

#include "crypto_primitives.h"

//--------------------------------------------------------------------
// Peripheral options
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Peripheral Headers
//--------------------------------------------------------------------

#include "da16x_crypto.h"
#include "da16x_secureboot.h"

/******************************************************************************
 *
 *  crypto Macro
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

#define	CRYPTO_PRINTF(...)		DBGT_Printf( __VA_ARGS__ )
#define	CRYPTO_VPRINTF(...)
#define CRYPTO_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	CRYPTO_GETC()			0x03
#define	CRYPTO_GETC_NOWAIT()		0x03
#define CRYPTO_PUTC(ch)
#define CRYPTO_PUTS(s)
#define CRYPTO_STRLEN(...)		strlen( __VA_ARGS__ )
#define	CRYPTO_STRCPY(...)		strcpy( __VA_ARGS__ )
#define CRYPTO_STRCMP(...)		strcmp( __VA_ARGS__ )
#define CRYPTO_STRCAT(...)		strcat( __VA_ARGS__ )
#define CRYPTO_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define CRYPTO_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define CRYPTO_STRCHR(...)		strchr( __VA_ARGS__ )

#define CRYPTO_MEMCPY(...)		memcpy( __VA_ARGS__ )
#define CRYPTO_MEMCMP(...)		memcmp( __VA_ARGS__ )
#define CRYPTO_MEMSET(...)		embcrypto_memset( __VA_ARGS__ )

#define CRYPTO_DBG_NONE(...)
#define CRYPTO_DBG_BASE(...)
#define CRYPTO_DBG_INFO(...)
#define CRYPTO_DBG_WARN(...)
#define CRYPTO_DBG_ERROR(...)
#define CRYPTO_DBG_DUMP(...)
#define CRYPTO_DBG_TEXT(...)

#else	//BUILD_OPT_VSIM
/******************************************************************************
 *
 *  ASIC & FPGA Model
 *
 ******************************************************************************/

//--------------------------------------------------------------------
// Retargeted primitives
//--------------------------------------------------------------------

#define	CRYPTO_PRINTF(...)		embcrypto_print(0, __VA_ARGS__ )
#define	CRYPTO_VPRINTF(...)		embcrypto_vprint(0, __VA_ARGS__ )
#define CRYPTO_SPRINTF(...)		da16x_sprintf( __VA_ARGS__ )
#define	CRYPTO_GETC()			embcrypto_getchar(OAL_SUSPEND)
#define	CRYPTO_GETC_NOWAIT()		embcrypto_getchar(OAL_NO_SUSPEND)
#define CRYPTO_PUTC(ch)			embcrypto_print(0, "%c", ch )
#define CRYPTO_PUTS(s)			embcrypto_print(0, s )

#define CRYPTO_STRLEN(...)		strlen( __VA_ARGS__ )
#define	CRYPTO_STRCPY(...)		strcpy( __VA_ARGS__ )
#define CRYPTO_STRCMP(...)		strcmp( __VA_ARGS__ )
#define CRYPTO_STRCAT(...)		strcat( __VA_ARGS__ )
#define CRYPTO_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define CRYPTO_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define CRYPTO_STRCHR(...)		strchr( __VA_ARGS__ )

#define CRYPTO_MEMCPY(...)		memcpy( __VA_ARGS__ )
#define CRYPTO_MEMCMP(...)		memcmp( __VA_ARGS__ )
#define CRYPTO_MEMSET(...)		embcrypto_memset( __VA_ARGS__ )

#define CRYPTO_DBG_NONE(...)		// CodeOpt.:: embcrypto_print(5, __VA_ARGS__ )
#define CRYPTO_DBG_BASE(...)		// CodeOpt.:: embcrypto_print(4, __VA_ARGS__ )
#define CRYPTO_DBG_INFO(...)		// CodeOpt.:: embcrypto_print(3, __VA_ARGS__ )
#define CRYPTO_DBG_WARN(...)		embcrypto_print(2, __VA_ARGS__ )
#define CRYPTO_DBG_ERROR(...)		embcrypto_print(1, __VA_ARGS__ )
#define CRYPTO_DBG_DUMP(tag, ...)	// CodeOpt.:: embcrypto_dump(tag, __VA_ARGS__ )
#define CRYPTO_DBG_TEXT(tag, ...)	// CodeOpt.:: embcrypto_text(tag, __VA_ARGS__ )

#endif	//BUILD_OPT_VSIM

#endif /* __cal_h__ */
/**
 * \}
 */

/* EOF */
