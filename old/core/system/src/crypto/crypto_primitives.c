/**
 ****************************************************************************************
 *
 * @file crypto_primitives.c
 *
 * @brief Crypto specific
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

#include "cal.h"


//---------------------------------------------------------
//
//---------------------------------------------------------

static  CRYPTO_PRIMITIVE_TYPE *crypto_primitive ;

/******************************************************************************
 *  init_crypto_primitives()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void	init_crypto_primitives(const CRYPTO_PRIMITIVE_TYPE *primitive)
{
	crypto_primitive = (CRYPTO_PRIMITIVE_TYPE *)primitive;
}

/******************************************************************************
 *  hal trace
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	embcrypto_dump(UINT16 tag, void *srcdata, UINT16 len)
{
	if(crypto_primitive != NULL && crypto_primitive->retarget_dump != NULL ){
		crypto_primitive->retarget_dump(tag, srcdata, len);
	}
}

void	embcrypto_text(UINT16 tag, void *srcdata, UINT16 len)
{
	if(crypto_primitive != NULL && crypto_primitive->retarget_text != NULL ){
		crypto_primitive->retarget_text(tag, srcdata, len);
	}
}
void	embcrypto_vprint(UINT16 tag, const char *format, va_list arg)
{
	if( crypto_primitive != NULL && crypto_primitive->retarget_vprint != NULL ){
		crypto_primitive->retarget_vprint(tag, format, arg);
	}
}

void	embcrypto_print(UINT16 tag, const char *fmt,...)
{
	if( crypto_primitive != NULL && crypto_primitive->retarget_vprint != NULL ){
		va_list ap;
		va_start(ap,fmt);

		crypto_primitive->retarget_vprint(tag, fmt, ap);

		va_end(ap);
	}
}

int	embcrypto_getchar(UINT32 mode )
{
	if( crypto_primitive != NULL && crypto_primitive->retarget_getchar != NULL ){
  		return crypto_primitive->retarget_getchar(mode);
	}
	return 0;
}

/******************************************************************************
 *  hal alloc
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_CRYPTOMEM_PROFILE

void *profile_embcrypto_malloc(char *name, int line, size_t size)
{
	void *allocbuf;
	if( crypto_primitive == NULL || crypto_primitive->profile_malloc == NULL  ){
		return NULL;
	}

	allocbuf = crypto_primitive->profile_malloc(name, line, size);
	if( allocbuf != NULL ){
		// TODO:
		if( (size >= 16) /*&& ((size & 0x03) == 0)*/ ){
			da16x_memset32(allocbuf, 0, (size) );
		}else{
			memset(allocbuf, 0, size);
		}
	}
	return allocbuf;
}

void profile_embcrypto_free(char *name, int line, void *f)
{
	if( crypto_primitive == NULL || crypto_primitive->profile_free == NULL  ){
		return;
	}

	crypto_primitive->profile_free(name, line, f);
}

#else	//SUPPORT_CRYPTOMEM_PROFILE

void *embcrypto_malloc(size_t size)
{
	void *allocbuf;
	if( crypto_primitive == NULL || crypto_primitive->raw_malloc == NULL  ){
		return NULL;
	}

	allocbuf = crypto_primitive->raw_malloc(size);
#if 1
	if( allocbuf != NULL ){
		// TODO:
		if( (size >= 256) /*&& ((size & 0x03) == 0)*/ ){
			da16x_memset32(allocbuf, 0, (size) );
		}else{
			memset(allocbuf, 0, size);
		}
	}
#else /* When USD DCACHE Freemem */
 			memset(allocbuf, 0, size);
#endif

	return allocbuf;
}

void embcrypto_free(void *f)
{
	if( crypto_primitive == NULL || crypto_primitive->raw_free == NULL  ){
		return;
	}
	crypto_primitive->raw_free(f);
}

#endif	//SUPPORT_CRYPTOMEM_PROFILE

void embcrypto_memset(void * dest, int value, size_t num)
{
	// TODO:
	memset(dest, value, num);
}

/* EOF */
