/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2018] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#define CC_PAL_LOG_CUR_COMPONENT CC_LOG_MASK_CC_API

#include "cc_pal_types.h"
#include "cc_pal_mem.h"
#include "mbedtls_cc_srp.h"
#include "cc_hash_defs.h"
#include "hash_driver.h"
#include "mbedtls_cc_srp_error.h"
#include "cc_general_defs.h"
#include "md.h"
#include "cc_bitops.h"

CCHashResultBuf_t zeroBuff = {0};

uint32_t   srpHashFinish(mbedtls_md_context_t *p_hash_ctx,
                         CCHashResultBuf_t    hashResultBuff)
{
    uint32_t rc = CC_OK;

    rc = mbedtls_md_finish(p_hash_ctx, (unsigned char *)hashResultBuff);

    return rc;
}

uint32_t  SRP_kMultiplierCalc(mbedtls_srp_context *pCtx)
{
    CCError_t   rc = 0;
    size_t          hashDigestSize;
    size_t          modSize;
    CCHashResultBuf_t      hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t hash_ctx;

    // verify input
    if (pCtx == NULL) {
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }
    hashDigestSize = pCtx->hashDigestSize;
    modSize = CALC_FULL_BYTES(pCtx->groupParam.modSizeInBits);

    CC_PalMemSetZero(pCtx->kMult, sizeof(mbedtls_srp_digest));
    switch(pCtx->srpVer) {
    case CC_SRP_VER_3:
        pCtx->kMult[hashDigestSize-1] = 0x1;
        break;
    case CC_SRP_VER_6:
        pCtx->kMult[hashDigestSize-1] = 0x3;
        break;
    case CC_SRP_VER_6A:
    case CC_SRP_VER_HK:
        md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
        if (NULL == md_info) {
                rc = CC_SRP_PARAM_INVALID_ERROR;
                goto End;
        }
        mbedtls_md_init(&hash_ctx);
        rc = mbedtls_md_setup(&hash_ctx, md_info, 0); // 0 = HASH, not HMAC
        if (rc != 0) {
                goto End;
        }
        rc = mbedtls_md_starts(&hash_ctx);
        if (rc != 0) {
            goto End;
        }
        rc = mbedtls_md_update(&hash_ctx, pCtx->groupParam.modulus, modSize);
        if (rc != 0) {
            goto End;
        }
        // use ephemPriv as tempopary buffer with paded zeros
        CC_PalMemSetZero(pCtx->ephemPriv, sizeof(mbedtls_srp_modulus));
        pCtx->ephemPriv[modSize-1] =  pCtx->groupParam.gen;
        rc = mbedtls_md_update(&hash_ctx, (uint8_t *)pCtx->ephemPriv, modSize);
        if (rc != 0) {
            goto End;
        }
        rc = srpHashFinish(&hash_ctx, hashResultBuff);
        if (rc != 0) {
            goto End;
        }
        // verify k != 0
        if (CC_PalMemCmp((uint8_t *)zeroBuff, (uint8_t *)hashResultBuff, hashDigestSize) == 0) {
            rc = CC_SRP_RESULT_ERROR;
            goto End;
        }
        // copy the result to the user
        CC_PalMemCopy(pCtx->kMult, (uint8_t *)hashResultBuff, hashDigestSize);
        break;
    default:  // should not reach here
        return CC_SRP_PARAM_INVALID_ERROR;
    }

    End:
    if (NULL != md_info) {
            mbedtls_md_free(&hash_ctx);
    }

    return rc;
}

uint32_t  SRP_uScrambleCalc(
        mbedtls_srp_modulus ephemPubA,
        mbedtls_srp_modulus ephemPubB,
        mbedtls_srp_digest  uScramble,
        mbedtls_srp_context *pCtx)
{
    CCError_t   rc = 0;
    CCHashResultBuf_t      hashResultBuff;
    size_t  modSize;
    uint32_t    bytesToCopy = 0;
    mbedtls_md_context_t hash_ctx;

    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t *md_ctx=NULL;

    // verify input
    if ((ephemPubA == NULL) ||
        (ephemPubB == NULL) ||
        (uScramble == NULL) ||
        (pCtx == NULL)) {
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }

    modSize = CALC_FULL_BYTES(pCtx->groupParam.modSizeInBits);

    CC_PalMemSetZero(uScramble, sizeof(mbedtls_srp_digest));
    switch(pCtx->srpVer) {
    case CC_SRP_VER_3:
        md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
        if (NULL == md_info) {
             rc = CC_SRP_PARAM_INVALID_ERROR;
             goto End;
        }
        rc = mbedtls_md(md_info,
                        (uint8_t *)ephemPubB,
                        modSize,
                        (unsigned char *)hashResultBuff);
        if (rc != 0) {
            goto End;
        }
        // The parameter u is a 32-bit unsigned integer which takes its value
        // from the first 32 bits of the SHA1 hash of B, MSB first.
        bytesToCopy = CC_32BIT_WORD_SIZE;
        break;
    case CC_SRP_VER_6:
    case CC_SRP_VER_6A:
    case CC_SRP_VER_HK:
        md_ctx = &hash_ctx;
        md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
        if (NULL == md_info) {
                rc = CC_SRP_PARAM_INVALID_ERROR;
                goto End;
        }
        mbedtls_md_init(md_ctx);
        rc = mbedtls_md_setup(md_ctx, md_info, 0); // 0 = HASH, not HMAC
        if (rc != 0) {
                goto End;
        }
        rc = mbedtls_md_starts(md_ctx);
        if (rc != 0) {
            goto End;
        }
        rc = mbedtls_md_update(md_ctx, ephemPubA, modSize);
        if (rc != 0) {
            goto End;
        }
        rc = mbedtls_md_update(md_ctx, ephemPubB, modSize);
        if (rc != 0) {
            goto End;
        }

        rc = srpHashFinish(md_ctx, hashResultBuff);
        if (rc != 0) {
            goto End;
        }
        bytesToCopy = pCtx->hashDigestSize;
        break;
    default:  // should not reach here
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }
    // verify u != 0
    if (CC_PalMemCmp((uint8_t *)zeroBuff, (uint8_t *)hashResultBuff, bytesToCopy) == 0) {
        rc = CC_SRP_RESULT_ERROR;
        goto End;
    }
    // copy uScramble
    CC_PalMemCopy(uScramble, (uint8_t *)hashResultBuff, bytesToCopy);

    End:
    if((md_info!=NULL) && (md_ctx!=NULL)) {
            mbedtls_md_free(md_ctx);
    }

    return rc;
}


// credDigest = SHA(U|:|P)
uint32_t  SRP_UserCredDigCalc(uint8_t   *pUserName,
              size_t        userNameSize,
              uint8_t   *pPwd,
              size_t        pwdSize,
              mbedtls_srp_context *pCtx)
{
    CCError_t   rc = 0;
    char  colonChar = ':';
    CCHashResultBuf_t      hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t hash_ctx;

    // verify input
    if ((pUserName == NULL) || (userNameSize == 0) ||
        (pPwd == NULL) || (pwdSize == 0) ||
        (pCtx == NULL)) {
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }

    CC_PalMemSetZero(pCtx->credDigest, sizeof(mbedtls_srp_digest));

    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
    if (NULL == md_info) {
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }
    mbedtls_md_init(&hash_ctx);
    rc = mbedtls_md_setup(&hash_ctx, md_info, 0); // 0 = HASH, not HMAC
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_starts(&hash_ctx);
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, pUserName, userNameSize);
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, (uint8_t *)&colonChar, sizeof(colonChar));
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, pPwd, pwdSize);
    if (rc != 0) {
        goto End;
    }
    rc = srpHashFinish(&hash_ctx, hashResultBuff);
    if (rc != 0) {
        goto End;
    }
    CC_PalMemCopy(pCtx->credDigest, (uint8_t *)hashResultBuff, pCtx->hashDigestSize);

    End:
    if (NULL != md_info) {
            mbedtls_md_free(&hash_ctx);
    }

    return rc;
}

// calc x = SHA(pSalt | pCtx->credDigest)
uint32_t  SRP_xBuffCalc(uint8_t             *pSalt,
            size_t          saltSize,
            mbedtls_srp_digest  xBuff,  // out
            mbedtls_srp_context     *pCtx)
{
    CCError_t   rc = 0;
    CCHashResultBuf_t      hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t hash_ctx;

    // verify input
    if ((pSalt == NULL) ||
        (saltSize < CC_SRP_MIN_SALT_SIZE) || (saltSize > CC_SRP_MAX_SALT_SIZE) ||
        (xBuff == NULL) ||
        (pCtx == NULL)) {
        rc = CC_SRP_PARAM_INVALID_ERROR;
        goto End;
    }
    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
    if (NULL == md_info) {
            rc = CC_SRP_PARAM_INVALID_ERROR;
            goto End;
    }
    mbedtls_md_init(&hash_ctx);
    rc = mbedtls_md_setup(&hash_ctx, md_info, 0); // 0 = HASH, not HMAC
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_starts(&hash_ctx);
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, pSalt, saltSize);
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, pCtx->credDigest, pCtx->hashDigestSize);
    if (rc != 0) {
        goto End;
    }
    rc = srpHashFinish(&hash_ctx, hashResultBuff);
    if (rc != 0) {
        goto End;
    }
    CC_PalMemCopy(xBuff, (uint8_t *)hashResultBuff, pCtx->hashDigestSize);

    End:
    if (NULL != md_info) {
            mbedtls_md_free(&hash_ctx);
    }

    return rc;
}


//SHA(U)
uint32_t  SRP_UserNameDigCalc(uint8_t   *pUserName,
        size_t        userNameSize,
        mbedtls_srp_context     *pCtx)
{
    CCError_t   rc = 0;
    CCHashResultBuf_t      hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;

    // verify input
    if ((pUserName == NULL) || (userNameSize == 0) ||
        (pCtx == NULL)) {
        return CC_SRP_PARAM_INVALID_ERROR;
    }

    if (pCtx->hashDigestSize > sizeof(CCHashResultBuf_t)) {
        return CC_SRP_PARAM_INVALID_ERROR;
    }

    // SHA(U)
    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
    if (NULL == md_info) {
         return CC_SRP_PARAM_INVALID_ERROR;
    }
    rc = mbedtls_md(md_info,
                    pUserName,
                    userNameSize,
                    (unsigned char *)hashResultBuff);
    if (rc != 0) {
        return rc;
    }
    CC_PalMemCopy(pCtx->userNameDigest, (uint8_t *)hashResultBuff, pCtx->hashDigestSize);

    return rc;
}


// SHA((SHA(N) XOR SHA(g))|SHA(U)|s|A|B|K)
uint32_t  SRP_UserProofCalc2(uint8_t        *pSalt,
            size_t                  saltSize,
            mbedtls_srp_modulus         userPubKeyA,
            mbedtls_srp_modulus         hostPubKeyB,
            mbedtls_srp_secret  sharedSecret,
            mbedtls_srp_digest  userProof, // out
            mbedtls_srp_context     *pCtx)
{
    CCError_t   rc = 0;
    CCHashResultBuf_t      hashResultBuff;
    size_t          hashDigestSize;
    size_t          modSize;
    uint32_t    i = 0;
    CCHashResultBuf_t   buff1 = {0};
    CCHashResultBuf_t   buff2 = {0};
    CCHashOperationMode_t   hashMode;
    mbedtls_md_context_t hash_ctx;

    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t *md_ctx=NULL;

    // verify input
    if ((pSalt == NULL) ||
        (saltSize < CC_SRP_MIN_SALT_SIZE) || (saltSize > CC_SRP_MAX_SALT_SIZE) ||
        (userPubKeyA == NULL) ||
        (hostPubKeyB == NULL) ||
        (sharedSecret == NULL) ||
        (userProof == NULL) ||
        (pCtx == NULL)) {
        return CC_SRP_PARAM_INVALID_ERROR;
    }

    hashDigestSize = pCtx->hashDigestSize;
    modSize = CALC_FULL_BYTES(pCtx->groupParam.modSizeInBits);
    hashMode = pCtx->hashMode;

    CC_PalMemSetZero(userProof, sizeof(mbedtls_srp_digest));

    // SHA(N)
    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[hashMode] );
    if (NULL == md_info) {
         rc = CC_SRP_PARAM_INVALID_ERROR;
         goto End;
    }
    rc = mbedtls_md(md_info,
                    pCtx->groupParam.modulus,
                    CALC_FULL_BYTES(pCtx->groupParam.modSizeInBits),
                    (unsigned char *)buff1);
    if (rc != 0) {
        goto End;
    }

    // SHA(g)
    rc = mbedtls_md(md_info,
                    &pCtx->groupParam.gen,
                    sizeof(pCtx->groupParam.gen),
                    (unsigned char *)buff2);
    if (rc != 0) {
        goto End;
    }
    // XOR SHA(N)^=SHA(g)
    for (i = 0; i < CALC_32BIT_WORDS_FROM_BYTES(hashDigestSize); i++) {
        buff1[i] ^= buff2[i];
    }
    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[hashMode] );
    if (NULL == md_info) {
            rc = CC_SRP_PARAM_INVALID_ERROR;
            goto End;
    }
    md_ctx = &hash_ctx;
    mbedtls_md_init(md_ctx);
    rc = mbedtls_md_setup(md_ctx, md_info, 0); // 0 = HASH, not HMAC
    if (rc != 0) {
            goto End;
    }
    rc = mbedtls_md_starts(md_ctx);
    if (rc != 0) {
        goto End;
    }
    // Update(SHA(N)^=SHA(g))
    rc = mbedtls_md_update(md_ctx, (uint8_t *)buff1, hashDigestSize);
    if (rc != 0) {
        goto End;
    }
    // Update(SHA(U))
    rc = mbedtls_md_update(md_ctx, pCtx->userNameDigest, hashDigestSize);
    if (rc != 0) {
        goto End;
    }
    //Update(salt)
    rc = mbedtls_md_update(md_ctx, pSalt, saltSize);
    if (rc != 0) {
        goto End;
    }
    //Update(A)
    rc = mbedtls_md_update(md_ctx, (uint8_t *)userPubKeyA, modSize);
    if (rc != 0) {
        goto End;
    }
    //Update(B)
    rc = mbedtls_md_update(md_ctx, (uint8_t *)hostPubKeyB, modSize);
    if (rc != 0) {
        goto End;
    }
    //Update(K)
    if (pCtx->srpVer == CC_SRP_VER_HK) {
        rc = mbedtls_md_update(md_ctx, (uint8_t *)sharedSecret, hashDigestSize);
    } else {
        rc = mbedtls_md_update(md_ctx, (uint8_t *)sharedSecret, 2*hashDigestSize);
    }
    if (rc != 0) {
        goto End;
    }
    rc = srpHashFinish(md_ctx, hashResultBuff);
    if (rc != 0) {
        goto End;
    }
    CC_PalMemCopy(userProof, (uint8_t *)hashResultBuff, hashDigestSize);

    End:
    if((md_info!=NULL) && (md_ctx!=NULL)) {
            mbedtls_md_free(md_ctx);
    }

    return rc;
}



// SHA(A|M1|K)
uint32_t  SRP_HostProofCalc(mbedtls_srp_modulus  userPubKeyA,
        mbedtls_srp_digest  userProof,
        mbedtls_srp_secret  sharedSecret,
            mbedtls_srp_digest  hostProof,  // out
            mbedtls_srp_context     *pCtx)
{
    CCError_t   rc = 0;
    size_t      hashDigestSize;
    CCHashResultBuf_t      hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;
    mbedtls_md_context_t hash_ctx;

    // verify input
    if ((userPubKeyA == NULL) ||
        (userProof == NULL) ||
        (sharedSecret == NULL) ||
        (hostProof == NULL) ||
        (pCtx == NULL)) {
        return CC_SRP_PARAM_INVALID_ERROR;
    }
    hashDigestSize = pCtx->hashDigestSize;

    CC_PalMemSetZero(hostProof, sizeof(mbedtls_srp_digest));

    md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
    if (NULL == md_info) {
            rc = CC_SRP_PARAM_INVALID_ERROR;
            goto End;
    }
    mbedtls_md_init(&hash_ctx);
    rc = mbedtls_md_setup(&hash_ctx, md_info, 0); // 0 = HASH, not HMAC
    if (rc != 0) {
            goto End;
    }
    rc = mbedtls_md_starts(&hash_ctx);
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, (uint8_t *)userPubKeyA, CALC_FULL_BYTES(pCtx->groupParam.modSizeInBits));
    if (rc != 0) {
        goto End;
    }
    rc = mbedtls_md_update(&hash_ctx, (uint8_t *)userProof, hashDigestSize);
    if (rc != 0) {
        goto End;
    }
    if (pCtx->srpVer == CC_SRP_VER_HK) {
        rc = mbedtls_md_update(&hash_ctx, (uint8_t *)sharedSecret, hashDigestSize);
    } else {
        rc = mbedtls_md_update(&hash_ctx, (uint8_t *)sharedSecret, 2*hashDigestSize);
    }
    if (rc != 0) {
        goto End;
    }
    rc = srpHashFinish(&hash_ctx, hashResultBuff);
    if (rc != 0) {
        goto End;
    }
    CC_PalMemCopy(hostProof, (uint8_t *)hashResultBuff, hashDigestSize);

    End:
    if (NULL != md_info) {
            mbedtls_md_free(&hash_ctx);
    }

    return rc;
}


// Sha_interleave
uint32_t  SRP_SharedSecretCalc(uint8_t      *pInBuff,       /* in buff to hash*/
               size_t       inBuffSize, /* in buffer size */
               mbedtls_srp_secret   sharedSecret,   /* out hash interleave buff - shared secret */
               mbedtls_srp_context  *pCtx)
{
    CCError_t   rc = 0;
    CCHashResultBuf_t   hashResultBuff;
    const mbedtls_md_info_t *md_info=NULL;

    //Verify inputs
    if ((pInBuff == NULL) ||
        (inBuffSize == 0) || (inBuffSize > CC_SRP_MAX_MODULUS) ||
        (sharedSecret == NULL) ||
        (pCtx == NULL)) {
        return CC_SRP_PARAM_INVALID_ERROR;
    }

    switch(pCtx->srpVer) {
    case CC_SRP_VER_HK:
        md_info = mbedtls_md_info_from_string( HashAlgMode2mbedtlsString[pCtx->hashMode] );
        if (NULL == md_info) {
             return CC_SRP_PARAM_INVALID_ERROR;
        }
        rc = mbedtls_md(md_info,
                        pInBuff,
                        inBuffSize,
                        (unsigned char *)hashResultBuff);
        if (rc != 0) {
            return rc;
        }
        CC_PalMemCopy(sharedSecret, (uint8_t *)hashResultBuff, pCtx->hashDigestSize);
        break;
    case CC_SRP_VER_3:
    case CC_SRP_VER_6:
    case CC_SRP_VER_6A:
    default:  // should not reach here
        return CC_SRP_PARAM_INVALID_ERROR;
    }
    return CC_OK;
}


uint32_t SRP_SecureMemCmp( const uint8_t* aTarget, /*!< [in] The target buffer to compare. */
                           const uint8_t* aSource, /*!< [in] The Source buffer to compare to. */
                           size_t      aSize    /*!< [in] Number of bytes to compare. */)
{
    uint32_t i;
    uint32_t stat = 0;

    for( i = 0; i < aSize; i++ ) {
            stat |= (aTarget[i] ^ aSource[i]);
    }

    return stat;
}
