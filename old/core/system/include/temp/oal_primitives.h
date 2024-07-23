/**
 ****************************************************************************************
 *
 * @file oal_primitives.h
 *
 * @brief DA16200 OAL specific
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


#ifndef __oal_primitives_h__
#define __oal_primitives_h__

#include "oal.h"

#ifdef	SUPPORT_MEM_PROFILER
#define	SUPPORT_OALMEM_PROFILE
#else //SUPPORT_MEM_PROFILER
#undef	SUPPORT_OALMEM_PROFILE
#endif //SUPPORT_MEM_PROFILER

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
#ifdef	SUPPORT_OALMEM_PROFILE
	void 	*(*profile_malloc)(char *name, int line, size_t size);
	void 	(*profile_free)(char *name, int line, void *f);
#else	//SUPPORT_OALMEM_PROFILE
	void 	*(*raw_malloc)(size_t size);
	void 	(*raw_free)(void *f);
#endif	//SUPPORT_OALMEM_PROFILE
} OAL_PRIMITIVE_TYPE;

extern void	init_oal_primitives(const OAL_PRIMITIVE_TYPE *primitive);

//--------------------------------------------------------------------
//	Memory
//--------------------------------------------------------------------

#ifdef	SUPPORT_OALMEM_PROFILE

extern void *profile_emboal_malloc(char *name, int line, size_t size);
extern void profile_emboal_free(char *name, int line, void *f);

#define	OAL_MALLOC(...)		profile_emboal_malloc( __func__, __LINE__, __VA_ARGS__ )
#define	OAL_FREE(...)		profile_emboal_free( __func__, __LINE__, __VA_ARGS__ )
#else	//SUPPORT_OALMEM_PROFILE

extern void *emboal_malloc(size_t size);
extern void emboal_free(void *f);

#define	OAL_MALLOC(...)		emboal_malloc( __VA_ARGS__ )
#define	OAL_FREE(...)		emboal_free( __VA_ARGS__ )
#endif	//SUPPORT_OALMEM_PROFILE

//--------------------------------------------------------------------
//	Console
//--------------------------------------------------------------------

extern void	emboal_dump(UINT16 tag, void *srcdata, UINT16 len);
extern void	emboal_text(UINT16 tag, void *srcdata, UINT16 len);
extern void	emboal_vprint(UINT16 tag, const char *format, va_list arg);
extern void	emboal_print(UINT16 tag, const char *fmt,...);
extern int	emboal_getchar(UINT32 mode );

#define	OAL_PRINTF(...)			emboal_print(0, __VA_ARGS__ )
#define	OAL_VPRINTF(...)		emboal_vprint(0, __VA_ARGS__ )

#define	OAL_GETC()			emboal_getchar(OAL_SUSPEND)
#define	OAL_GETC_NOWAIT()		emboal_getchar(OAL_NO_SUSPEND)
#define OAL_PUTC(ch)			emboal_print(0, "%c", ch )
#define OAL_PUTS(s)			emboal_print(0, s )

#define OAL_STRLEN(...)			strlen( __VA_ARGS__ )
#define	OAL_STRCPY(...)			strcpy( __VA_ARGS__ )
#define OAL_STRCMP(...)			strcmp( __VA_ARGS__ )
#define OAL_STRCAT(...)			strcat( __VA_ARGS__ )
#define OAL_STRNCMP(...)		strncmp( __VA_ARGS__ )
#define OAL_STRNCPY(...)		strncpy( __VA_ARGS__ )
#define OAL_STRCHR(...)			strchr( __VA_ARGS__ )

#define OAL_MEMCPY(...)			memcpy( __VA_ARGS__ )
#define OAL_MEMCMP(...)			memcmp( __VA_ARGS__ )
#define OAL_MEMSET(...)			memset( __VA_ARGS__ )

#define OAL_DBG_NONE(...)		emboal_print(5, __VA_ARGS__ )
#define OAL_DBG_BASE(...)		emboal_print(4, __VA_ARGS__ )
#define OAL_DBG_INFO(...)		emboal_print(3, __VA_ARGS__ )
#define OAL_DBG_WARN(...)		emboal_print(2, __VA_ARGS__ )
#define OAL_DBG_ERROR(...)		emboal_print(1, __VA_ARGS__ )
#define OAL_DBG_DUMP(tag, ...)		emboal_dump(tag, __VA_ARGS__ )
#define OAL_DBG_TEXT(tag, ...)		emboal_text(tag, __VA_ARGS__ )


#endif /* oal_primitives */

/* EOF */
