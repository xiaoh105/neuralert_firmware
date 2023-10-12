/**
 ****************************************************************************************
 *
 * @file crypto_primitives.h
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


#ifndef __crypto_primitives_h__
#define __crypto_primitives_h__

#include "cal.h"

#ifdef	SUPPORT_MEM_PROFILER
#define	SUPPORT_CRYPTOMEM_PROFILE
#else	//SUPPORT_MEM_PROFILER
#undef	SUPPORT_CRYPTOMEM_PROFILE
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
	char	(*retarget_getchar)(UINT32 mode );
#ifdef	SUPPORT_HALMEM_PROFILE
	void 	*(*profile_malloc)(char *name, int line, size_t size);
	void 	(*profile_free)(char *name, int line, void *f);
#else	//SUPPORT_HALMEM_PROFILE
	void 	*(*raw_malloc)(size_t size);
	void 	(*raw_free)(void *f);
#endif	//SUPPORT_HALMEM_PROFILE
} CRYPTO_PRIMITIVE_TYPE;

extern void	init_crypto_primitives(const CRYPTO_PRIMITIVE_TYPE *primitive);

//--------------------------------------------------------------------
//	Memory
//--------------------------------------------------------------------

#ifdef	SUPPORT_CRYPTOMEM_PROFILE

extern void *profile_embcrypto_malloc(char *name, int line, size_t size);
extern void profile_embcrypto_free(char *name, int line, void *f);

#define	CRYPTO_MALLOC(...)		profile_embcrypto_malloc( __func__, __LINE__, __VA_ARGS__ )
#define	CRYPTO_FREE(...)		profile_embcrypto_free(  __func__, __LINE__, __VA_ARGS__ )
#else	//SUPPORT_CRYPTOMEM_PROFILE

extern void *embcrypto_malloc(size_t size);
extern void embcrypto_free(void *f);

#define	CRYPTO_MALLOC(...)		embcrypto_malloc( __VA_ARGS__ )
#define	CRYPTO_FREE(...)		embcrypto_free( __VA_ARGS__ )
#endif	//SUPPORT_CRYPTOMEM_PROFILE

//--------------------------------------------------------------------
//	Console
//--------------------------------------------------------------------

extern void	embcrypto_dump(UINT16 tag, void *srcdata, UINT16 len);
extern void	embcrypto_text(UINT16 tag, void *srcdata, UINT16 len);
extern void	embcrypto_vprint(UINT16 tag, const char *format, va_list arg);
extern void	embcrypto_print(UINT16 tag, const char *fmt,...);
extern int	embcrypto_getchar(UINT32 mode );
extern void 	embcrypto_memset(void * dest, int value, size_t num);

#endif /* __crypto_primitives_h__ */

/* EOF */
