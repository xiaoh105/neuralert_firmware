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


#ifndef LLF_RND_ERROR_H
#define LLF_RND_ERROR_H


#include "cc_error.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/
/** RND module on the LLF layer base address -  0x00F10C00     */

#define LLF_RND_HW_VERSION_NOT_CORRECT_ERROR        (LLF_RND_MODULE_ERROR_BASE + 0x0UL)
#define LLF_RND_CPRNG_TEST_FAIL_ERROR           (LLF_RND_MODULE_ERROR_BASE + 0x1UL)
#define LLF_RND_CRNGT_TEST_FAIL_ERROR           (LLF_RND_MODULE_ERROR_BASE + 0x2UL)
#define LLF_RND_STATE_PTR_INVALID_ERROR         (LLF_RND_MODULE_ERROR_BASE + 0x3UL)
#define LLF_RND_AES_256_NOT_SUPPORTED_ERROR             (LLF_RND_MODULE_ERROR_BASE + 0x4UL)

#define LLF_RND_TRNG_TIME_LIMIT_EXCEEDED_ERROR          (LLF_RND_MODULE_ERROR_BASE + 0x20UL)
#define LLF_RND_TRNG_ENTR_ESTIM_SIZE_EXCEED_ERROR       (LLF_RND_MODULE_ERROR_BASE + 0x21UL)

#define LLF_RND_TRNG_PREVIOUS_PARAMS_NOT_MATCH_ERROR    (LLF_RND_MODULE_ERROR_BASE + 0x30UL)
#define LLF_RND_TRNG_REQUIRED_ROSCS_NOT_ALLOWED_ERROR   (LLF_RND_MODULE_ERROR_BASE + 0x31UL)
#define LLF_RND_TRNG_GENERATION_NOT_COMPLETED_ERROR     (LLF_RND_MODULE_ERROR_BASE + 0x32UL)
#define LLF_RND_TRNG_LOW_ENTROPY_ERROR              (LLF_RND_MODULE_ERROR_BASE + 0x33UL)
#define LLF_RND_TRNG_NULL_ENTROPY_ERROR             (LLF_RND_MODULE_ERROR_BASE + 0x34UL)
#define LLF_RND_TRNG_ILLEGAL_PTR_ERROR              (LLF_RND_MODULE_ERROR_BASE + 0x35UL)
#define LLF_RND_TRNG_REPETITION_COUNTER_ERROR           (LLF_RND_MODULE_ERROR_BASE + 0x36UL)
#define LLF_RND_TRNG_ADAPTION_PROPORTION_ERROR          (LLF_RND_MODULE_ERROR_BASE + 0x37UL)

#ifdef __cplusplus
}
#endif

#endif

