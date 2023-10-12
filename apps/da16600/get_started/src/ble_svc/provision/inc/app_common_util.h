/**
 ****************************************************************************************
 *
 * @file app_common_util.h
 *
 * @brief common util for app
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

#include "da16x_system.h"

#if !defined(_APP_COMMON_UTIL_H_)
#define _APP_COMMON_UTIL_H_

/*! type define app mutex*/
//typedef TX_MUTEX app_mutex ;

/*! wrapping system sleep */
#if !defined(sleep)
#define sleep(v)    vTaskDelay(v/10)
#endif

/*! memory malloc API for APP  */
#define app_common_malloc(x) malloc(x)
/*! memory free API for APP  */
#define app_common_free(x) if(x!=NULL){free(x); x=NULL;}
/*! memory calloc API for APP  */
#define app_common_calloc(n,x) calloc(n,x)
/*! memory realloc API for APP  */
#define app_common_realloc(n,x) realloc(n,x)

/*! print function for APP debugging */
#if !defined(APRINTF)
#define APRINTF(...) PRINTF(__VA_ARGS__)
//#define APRINTF(...)
#endif

#if !defined(APRINTF_Y)
#define APRINTF_Y(...)	\
	{\
		PRINTF("\33[1;33m");\
		PRINTF(__VA_ARGS__);\
		PRINTF("\33[0m");	\
	}
#endif

#if !defined(APRINTF_I)
#define APRINTF_I(...)	\
{\
	PRINTF("\33[1;32m");\
	PRINTF(__VA_ARGS__);\
	PRINTF("\33[0m");	\
}
#endif

#if !defined(APRINTF_S)
#define APRINTF_S(...)	\
{\
	PRINTF("\33[1;36m");\
	PRINTF(__VA_ARGS__);\
	PRINTF("\33[0m");	\
}
#endif

#if !defined(APRINTF_E)
#define APRINTF_E(...)	\
{\
	PRINTF("\33[1;31m");\
	PRINTF(__VA_ARGS__);\
	PRINTF("\33[0m");	\
}
#endif

#endif

/* EOF */
