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


/*! @file
@brief This file contains basic platform-dependent type definitions.
*/
#ifndef _CC_PAL_TYPES_PLAT_H
#define _CC_PAL_TYPES_PLAT_H
/* Host specific types for standard (ISO-C99) compilant platforms */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uintptr_t       CCVirtAddr_t;
typedef uint32_t            CCBool_t;
typedef uint32_t            CCStatus;

#define CCError_t           CCStatus
#define CC_INFINITE         0xFFFFFFFF

#define CEXPORT_C
#define CIMPORT_C

#endif /*_CC_PAL_TYPES_PLAT_H*/
