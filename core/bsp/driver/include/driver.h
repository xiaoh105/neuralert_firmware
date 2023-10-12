/**
 * \addtogroup DALayer		Device Abstraction Layer
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file driver.h
 *
 * @brief DA16200 external devices
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

#ifndef __driver_h__
#define __driver_h__

//--------------------------------------------------------------------
//	Register Map
//--------------------------------------------------------------------
#include "CMSDK_CM4.h"

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------

#include "target.h"
#include "hal.h"

//--------------------------------------------------------------------
//	Retargeted Console & Pool
//--------------------------------------------------------------------

#include "hal_primitives.h"

//--------------------------------------------------------------------
// Peripheral options
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Peripheral Headers
//--------------------------------------------------------------------

//
//	Devices
//
#include "sflash.h"
#include "console.h" //for da16x_sprintf()

//
//	Macro
//

#define		DRIVER_STRLEN(...)	strlen( __VA_ARGS__ )
#define		DRIVER_STRCPY(...)	strcpy( __VA_ARGS__ )
#define		DRIVER_STRCMP(...)	strcmp( __VA_ARGS__ )
#define		DRIVER_STRCAT(...)	strcat( __VA_ARGS__ )
#define		DRIVER_STRNCMP(...)	strncmp( __VA_ARGS__ )
#define		DRIVER_STRNCPY(...)	strncpy( __VA_ARGS__ )
#define		DRIVER_STRCHR(...)	strchr( __VA_ARGS__ )
#define		DRIVER_SPRINTF(...)	da16x_sprintf( __VA_ARGS__ )

#define		DRIVER_MEMCPY(...)	memcpy( __VA_ARGS__ )
#define		DRIVER_MEMCMP(...)	memcmp( __VA_ARGS__ )
#define		DRIVER_MEMSET(...)	memset( __VA_ARGS__ )
#define		DRIVER_MEMMOVE(...)	memmove( __VA_ARGS__ )

#ifdef	SUPPORT_HALMEM_PROFILE
#define		DRIVER_MALLOC(...)	profile_embhal_malloc( __func__, __LINE__, __VA_ARGS__ )
#define		DRIVER_FREE(...)	profile_embhal_free( __func__, __LINE__, __VA_ARGS__ )
#else	//SUPPORT_HALMEM_PROFILE
//#define		DRIVER_MALLOC		embhal_malloc
//#define		DRIVER_FREE		embhal_free
#define		DRIVER_MALLOC		pvPortMalloc
#define		DRIVER_FREE			vPortFree
#endif	//SUPPORT_HALMEM_PROFILE

#endif /* __driver_h__ */
