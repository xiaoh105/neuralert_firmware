/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/


#include "cc_pal_types.h"
#include "cc_pal_log.h"

/* Implementation that should never be optimized out by the compiler */

void mbedtls_zeroize_internal( void *v, size_t n )
{
    volatile unsigned char *p = NULL;
    if( NULL == v )
    {
        CC_PAL_LOG_ERR( "input is NULL\n" );
        return;
    }
    p = (unsigned char*)v; while( n-- ) *p++ = 0;
}

