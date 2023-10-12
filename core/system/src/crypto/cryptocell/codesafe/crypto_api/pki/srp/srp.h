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

#define CC_PAL_LOG_CUR_COMPONENT CC_LOG_MASK_CC_API

#ifndef _SRP_H
#define _SRP_H

#include "mbedtls_cc_srp.h"



uint32_t  SrpPwdVerifierCalc(mbedtls_srp_digest xBuff,
        mbedtls_srp_modulus pwdVerifier,
        mbedtls_srp_context *pCtx);

/* calculates S=(((A*v^u)^b)%N */
uint32_t   SrpHostSessionKeyCalc(mbedtls_srp_modulus   userPubKeyA,
        mbedtls_srp_modulus pwdVerifier,
        mbedtls_srp_digest uScramble,
                mbedtls_srp_modulus  sessionKey,
                mbedtls_srp_context  *pCtx);


// Use PKA to calculate S=((B-g^x)^(a+u*x))%N
uint32_t   SrpUserSessionKeyCalc(mbedtls_srp_modulus  hostPubKeyB,
        mbedtls_srp_digest    xBuff,
        mbedtls_srp_digest    uScramble,
                mbedtls_srp_modulus   sessionKey,
                mbedtls_srp_context   *pCtx);

/* calculates B = (k*v+ g^b)%N */
uint32_t  SrpHostPublicKeyCalc(mbedtls_srp_modulus  pwdVerifier,    // in
        mbedtls_srp_modulus hostPubKey,     // out
        mbedtls_srp_context *pCtx);

/* calculates A = (g^a)%N */
uint32_t  SrpUserPublicKeyCalc(mbedtls_srp_modulus  userPubKeyA,    // out
        mbedtls_srp_context *pCtx);

#endif

