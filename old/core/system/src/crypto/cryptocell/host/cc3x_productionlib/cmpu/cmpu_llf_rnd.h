/***************************************************************************
* The confidential and proprietary information contained in this file may
* only be used by a person authorised under and to the extent permitted
* by a subsisting licensing agreement from ARM Limited or its affiliates.
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.
*       ALL RIGHTS RESERVED
* This entire notice must be reproduced on all copies of this file
* and copies of this file may only be made by a person if such person is
* permitted to do so under the terms of a subsisting license agreement
* from ARM Limited or its affiliates.
*****************************************************************************/


#ifndef _CMPU_LLF_RND_DEFS_H
#define _CMPU_LLF_RND_DEFS_H

/************* Include Files ****************/
#include "prod_hw_defs.h"
#include "cc_otp_defs.h"

#define CC_PROD_RND_Fast                                        0
#define CC_PROD_REQUIRED_ENTROPY_BITS                           256

uint32_t CC_PROD_LLF_RND_GetTrngSource(uint32_t           **ppSourceOut,
                       uint32_t           *pSourceOutSize,
                                       uint32_t *pRndWorkBuff);

uint32_t CC_PROD_LLF_RND_VerifyGeneration(uint8_t *pBuff);

#endif //_CMPU_LLF_RND_DEFS_H
