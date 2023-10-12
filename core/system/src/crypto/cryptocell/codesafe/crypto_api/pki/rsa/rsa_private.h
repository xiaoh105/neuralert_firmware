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


#ifndef LLF_RSA_PRIVATE_H
#define LLF_RSA_PRIVATE_H

/*
 * All the includes that are needed for code using this module to
 * compile correctly should be #included here.
 */

#include "cc_error.h"
#include "cc_rsa_types.h"


#ifdef __cplusplus
extern "C"
{
#endif


typedef union {
    struct {
/* the Barrett mod N tag  NP for N-modulus - used in the modular multiplication and
  exponentiation, calculated in CC_RsaPrivKeyBuild function */
        uint32_t NP[CC_PKA_BARRETT_MOD_TAG_BUFF_SIZE_IN_WORDS];

    }NonCrt;

    struct {
/* the Barrett mod P tag  PP for P-factor - used in the modular multiplication and
  exponentiation, calculated in CC_RsaPrivKeyBuild function */
        uint32_t PP[CC_PKA_BARRETT_MOD_TAG_BUFF_SIZE_IN_WORDS];

/* the Barrett mod Q tag  QP for Q-factor - used in the modular multiplication and
  exponentiation, calculated in CC_RsaPubKeyBuild function */
        uint32_t QP[CC_PKA_BARRETT_MOD_TAG_BUFF_SIZE_IN_WORDS];

    }Crt;

}RsaPrivKeyDb_t;

CCError_t RsaInitPrivKeyDb(CCRsaPrivKey_t *pPrivKey);

CCError_t RsaExecPrivKeyExp(CCRsaPrivKey_t    *pPrivKey,
            CCRsaPrimeData_t *pPrivData);

#ifdef __cplusplus
}
#endif

#endif
