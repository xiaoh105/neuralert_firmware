/**
 ****************************************************************************************
 *
 * @file da16x_types.h
 *
 * @brief Type defines and macros for DA16XXX
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


#ifndef __DA16X_TYPES_H__
#define __DA16X_TYPES_H__

/******************************************************************************
 *
 *  Type
 *
 ******************************************************************************/

#ifndef	 TX_PORT_H

typedef signed char			CHAR;
typedef unsigned char		UCHAR;
typedef signed short		SHORT;
typedef unsigned short		USHORT;
typedef signed int			INT;
typedef unsigned int		UINT;
typedef signed long			LONG;
typedef unsigned long		ULONG;

typedef	 void				VOID;
#endif	/* TX_PORT_H */

typedef char				INT8;
typedef unsigned char		UINT8;
typedef short				INT16;
typedef unsigned short		UINT16;
typedef int					INT32;
typedef unsigned int		UINT32;
typedef long long			INT64;
typedef unsigned long long	UINT64;

typedef	 void				*HANDLE;

typedef unsigned int		OPTION;
typedef	unsigned long		UNSIGNED;
typedef	long				SIGNED;

typedef unsigned long long	u64;
typedef unsigned int		u32;
typedef unsigned short		u16;
typedef unsigned char		u8;



/******************************************************************************
 *
 *  Macro
 *
 ******************************************************************************/

#ifndef	 NX_PORT_H
#define		TRUE		(1)
#define		FALSE		(0)
#else
#define		TRUE		pdTRUE
#define		FALSE		pdFALSE
#endif

#define		MBYTE		(1024UL*1024UL)
#define		KBYTE		(1024UL)

#define		MHz			(1000UL*1000UL)
#define		KHz			(1000UL)

#ifndef	DA16X_UNUSED_ARG
#define	DA16X_UNUSED_ARG(x)	(void)x
#endif // DA16X_UNUSED_ARG

/******************************************************************************
 *
 *  Callback
 *
 ******************************************************************************/

typedef 	VOID	(*USR_CALLBACK )(VOID *);

typedef		struct {
	VOID	(*func)(VOID *);
	VOID	*param;
} 	ISR_CALLBACK_TYPE;

typedef		struct {
	VOID	(*func)(UINT32, VOID *);
	VOID	*param;
} 	CLOCK_CALLBACK_TYPE;

#endif	/* __DA16X_TYPES_H__ */

/* EOF */
