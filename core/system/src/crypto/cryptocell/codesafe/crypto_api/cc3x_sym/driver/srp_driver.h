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

#include "mbedtls_cc_srp.h"


uint32_t  SRP_kMultiplierCalc(mbedtls_srp_context *pSrpCtx);

uint32_t  SRP_uScrambleCalc(mbedtls_srp_modulus ephemPubA,
        mbedtls_srp_modulus ephemPubB,
        mbedtls_srp_digest  uScramble,
        mbedtls_srp_context *pCtx);

// credDigest = SHA(U|:|P)
uint32_t  SRP_UserCredDigCalc(uint8_t   *pUserName,
              size_t        userNameSize,
              uint8_t   *pPwd,
              size_t        pwdSize,
              mbedtls_srp_context *pCtx);


// calc x = SHA(pSalt | pCtx->credDigest)
uint32_t  SRP_xBuffCalc(uint8_t             *pSalt,
            size_t          saltSize,
            mbedtls_srp_digest  xBuff,  // out
            mbedtls_srp_context     *pCtx);

//SHA(U)
uint32_t  SRP_UserNameDigCalc(uint8_t   *pUserName,
        size_t        userNameSize,
        mbedtls_srp_context     *pCtx);

// SHA(SHA(N)^=SHA(g) | SHA(U)|s|A|B|K)
uint32_t  SRP_UserProofCalc2(uint8_t            *pSalt,
            size_t                  saltSize,
            mbedtls_srp_modulus         userPubKeyA,
            mbedtls_srp_modulus         hostPubKeyB,
            mbedtls_srp_secret  sharedSecret,
            mbedtls_srp_digest  userProof,
            mbedtls_srp_context     *pCtx);

// SHA(A|M1|K)
uint32_t  SRP_HostProofCalc(mbedtls_srp_modulus  userPubKeyA,
        mbedtls_srp_digest  userProof,
        mbedtls_srp_secret  sharedSecret,
            mbedtls_srp_digest  hostProof,  // out
            mbedtls_srp_context     *pCtx);


// Sha_interleave
uint32_t  SRP_SharedSecretCalc(uint8_t      *pInBuff,       /* in buff to hash*/
               size_t       inBuffSize, /* in buffer size */
               mbedtls_srp_secret   sharedSecret,
               mbedtls_srp_context  *pCtx);


// Secure mem compare comes to prevent timing attacks on pwd comparison.
uint32_t SRP_SecureMemCmp( const uint8_t* aTarget, /*!< [in] The target buffer to compare. */
                           const uint8_t* aSource, /*!< [in] The Source buffer to compare to. */
                           size_t      aSize    /*!< [in] Number of bytes to compare. */);
