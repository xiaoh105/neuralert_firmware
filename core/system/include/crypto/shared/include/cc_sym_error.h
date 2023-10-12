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

#ifndef __CC_ERROR_H__
#define __CC_ERROR_H__

#ifdef __KERNEL__
#include <linux/types.h>
#define INT32_MAX 0x7FFFFFFFL
#else
#include <stdint.h>
#endif


typedef enum CCSymRetCode {
    CC_RET_OK = 0, /* No error */
    CC_RET_UNSUPP_ALG, /* Unsupported algorithm */
    CC_RET_UNSUPP_ALG_MODE, /* Unsupported algorithm mode */
    CC_RET_UNSUPP_OPERATION, /* Unsupported operation */
    CC_RET_UNSUPP_HWKEY, /* Unsupported hw key */
    CC_RET_INV_HWKEY, /* invalid hw key */
    CC_RET_INVARG, /* Invalid parameter */
    CC_RET_INVARG_KEY_SIZE, /* Invalid key size */
    CC_RET_INVARG_CTX_IDX, /* Invalid context index */
    CC_RET_INVARG_CTX, /* Bad or corrupted context */
    CC_RET_INVARG_BAD_ADDR, /* Bad address */
    CC_RET_INVARG_INCONSIST_DMA_TYPE, /* DIN is inconsist with DOUT DMA type */
    CC_RET_PERM, /* Operation not permitted */
    CC_RET_NOEXEC, /* Execution format error */
    CC_RET_BUSY, /* Resource busy */
    CC_RET_NOMEM, /* Out of memory */
    CC_RET_OSFAULT, /* Internal TEE_OS error */
    CCSYMCRYPTO_RET_RESERVE32 = INT32_MAX /* assure this enum is 32b */
}CCSymRetCode_t;


#endif /*__CC_ERROR_H__*/
