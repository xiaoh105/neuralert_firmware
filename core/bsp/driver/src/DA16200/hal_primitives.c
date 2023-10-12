/**
 ****************************************************************************************
 *
 * @file hal_primitives.c
 *
 * @brief HAL specific
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "hal.h"

//---------------------------------------------------------
//
//---------------------------------------------------------

void	init_hal_primitives(const HAL_PRIMITIVE_TYPE *primitive)
{
	DA16X_UNUSED_ARG(primitive);
}
void	embhal_dump(UINT16 tag, void *srcdata, UINT16 len)
{
	DA16X_UNUSED_ARG(tag);
	DA16X_UNUSED_ARG(srcdata);
	DA16X_UNUSED_ARG(len);
}

void	embhal_text(UINT16 tag, void *srcdata, UINT16 len)
{
	DA16X_UNUSED_ARG(tag);
	DA16X_UNUSED_ARG(srcdata);
	DA16X_UNUSED_ARG(len);
}
void	embhal_vprint(UINT16 tag, const char *format, va_list arg)
{
	DA16X_UNUSED_ARG(tag);
	DA16X_UNUSED_ARG(format);
	DA16X_UNUSED_ARG(arg);
}

void	embhal_print(UINT16 tag, const char *fmt,...)
{
	DA16X_UNUSED_ARG(tag);
	DA16X_UNUSED_ARG(fmt);
}

int	embhal_getchar(UINT32 mode )
{
	DA16X_UNUSED_ARG(mode);
	return 0;
}

#ifdef	SUPPORT_HALMEM_PROFILE
void *profile_embhal_malloc(char *name, int line, size_t size)
{
	return NULL;
}
void profile_embhal_free(char *name, int line, void *f)
{
}
#else	//SUPPORT_HALMEM_PROFILE
void *embhal_malloc(size_t size)
{
	DA16X_UNUSED_ARG(size);
	return NULL;
}
void embhal_free(void *f)
{
	DA16X_UNUSED_ARG(f);
}
#endif	//SUPPORT_HALMEM_PROFILE
