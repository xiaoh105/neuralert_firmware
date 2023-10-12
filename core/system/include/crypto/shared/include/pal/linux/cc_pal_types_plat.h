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

#ifndef _CC_PAL_TYPES_PLAT_H
#define _CC_PAL_TYPES_PLAT_H

/*! @file
@brief This file contains basic platform-dependent type definitions.
*/

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

/* Define macros for host to SeP endianess conversion (for host wrappers) */
#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(x) bswap_16(x)
#define le16_to_cpu(x) bswap_16(x)
#define cpu_to_le32(x) bswap_32(x)
#define le32_to_cpu(x) bswap_32(x)
#else /*__LITTLE_ENDIAN*/
#define cpu_to_le16(x) x
#define le16_to_cpu(x) x
#define cpu_to_le32(x) x
#define le32_to_cpu(x) x
#endif /*__BYTE_ORDER*/

#endif /*_CC_PAL_TYPES_PLAT_H*/
