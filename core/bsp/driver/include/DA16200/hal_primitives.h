/**
 ****************************************************************************************
 *
 * @file hal_primitives.h
 *
 * @brief HAL specific defines
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



#ifndef __hal_primitives_h__
#define __hal_primitives_h__

#include "hal.h"

#ifdef	SUPPORT_MEM_PROFILER
#define	SUPPORT_HALMEM_PROFILE
#else	//SUPPORT_MEM_PROFILER
#undef	SUPPORT_HALMEM_PROFILE
#endif	//SUPPORT_MEM_PROFILER

/******************************************************************************
 *
 *  Retargeted functions
 *
 ******************************************************************************/
 
typedef		struct	{
	void	(*retarget_dump)(UINT16 tag, void *srcdata, UINT16 len);
	void	(*retarget_text)(UINT16 tag, void *srcdata, UINT16 len);
	void	(*retarget_vprint)(UINT16 tag, const char *format, va_list arg);
	int	(*retarget_getchar)(UINT32 mode );
#ifdef	SUPPORT_HALMEM_PROFILE
	void 	*(*profile_malloc)(char *name, int line, size_t size);
	void 	(*profile_free)(char *name, int line, void *f);
#else	//SUPPORT_HALMEM_PROFILE
	void 	*(*raw_malloc)(size_t size);
	void 	(*raw_free)(void *f);
#endif	//SUPPORT_HALMEM_PROFILE	
} HAL_PRIMITIVE_TYPE;

extern void	init_hal_primitives(const HAL_PRIMITIVE_TYPE *primitive);
 
//--------------------------------------------------------------------
//	Memory
//--------------------------------------------------------------------

#ifdef	SUPPORT_HALMEM_PROFILE

extern void *profile_embhal_malloc(char *name, int line, size_t size);
extern void profile_embhal_free(char *name, int line, void *f);

#define	HAL_MALLOC(...)		profile_embhal_malloc( __func__, __LINE__, __VA_ARGS__ )
#define	HAL_FREE(...)		profile_embhal_free(  __func__, __LINE__, __VA_ARGS__ )
#else	//SUPPORT_HALMEM_PROFILE

extern void *embhal_malloc(size_t size);
extern void embhal_free(void *f);

#define	HAL_MALLOC(...)		pvPortMalloc( __VA_ARGS__ )
#define	HAL_FREE(...)		vPortFree( __VA_ARGS__ )
#endif	//SUPPORT_HALMEM_PROFILE

//--------------------------------------------------------------------
//	Console
//--------------------------------------------------------------------

extern void	embhal_dump(UINT16 tag, void *srcdata, UINT16 len);
extern void	embhal_text(UINT16 tag, void *srcdata, UINT16 len);
extern void	embhal_vprint(UINT16 tag, const char *format, va_list arg);
extern void	embhal_print(UINT16 tag, const char *fmt,...);
extern int	embhal_getchar(UINT32 mode );

#endif /* __hal_primitives_h__ */
