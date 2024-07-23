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

#ifndef  _CC_UTIL_KEY_DERIVATION_H
#define  _CC_UTIL_KEY_DERIVATION_H

/*!
@file
@brief This file defines the API that supports Key derivation function as specified
       in NIST Special Publication 800-108: Recommendation for Key Derivation Using Pseudorandom Functions
       in section "KDF in Counter Mode".
@defgroup cc_utils_key_derivation CryptoCell utility key derivation APIs
@{
@ingroup cc_utils

*/

#ifdef __cplusplus
extern "C"
{
#endif


#include "cc_util_defs.h"
#include "cc_util_key_derivation_defs.h"
#include "cc_aes_defs.h"
#include "cc_hash_defs.h"

/******************************************************************************
*                           DEFINITIONS
******************************************************************************/

/*! Input key derivation type. */
typedef enum  {
    /*! User's key.*/
    CC_UTIL_USER_KEY = 0,
    /*! Root key (Kdr).*/
    CC_UTIL_ROOT_KEY = 1,
    /*! Total .*/
    CC_UTIL_TOTAL_KEYS = 2,
    /*! Reserved.*/
    CC_UTIL_END_OF_KEY_TYPE = 0x7FFFFFFF
}CCUtilKeyType_t;

/*! Pseudo random function type for key derivation. */
typedef enum {
    /*! CMAC.*/
    CC_UTIL_PRF_CMAC = 0,
    /*! HMAC.*/
    CC_UTIL_PRF_HMAC = 1,
    /*! Total .*/
    CC_UTIL_TOTAL_PRFS = 2,
    /*! Reserved.*/
    CC_UTIL_END_OF_PRF_TYPE = 0x7FFFFFFF
}CCUtilPrfType_t;


/*!
@brief  The key derivation function is as specified in the "KDF in Counter Mode" section of
    NIST Special Publication 800-108: Recommendation for Key Derivation Using Pseudorandom Functions.
    The derivation is based on length l, label L, context C and derivation key Ki.
        AES-CMAC is used as the pseudorandom function (PRF).
\note   The user must well define the label and context for each use-case, when using this API.

@return CC_UTIL_OK on success.
@return A non-zero value from cc_util_error.h on failure.
*/

/*  A key derivation functions can iterates n times until l bits of keying material are generated.
        For each of the iteration of the PRF, i=1 to n, do:
        result(0) = 0;
        K(i) = PRF (Ki, [i] || Label || 0x00 || Context || length);
        results(i) = result(i-1) || K(i);

        concisely, result(i) = K(i) || k(i-1) || .... || k(0)*/
CCUtilError_t CC_UtilKeyDerivation(
    CCUtilKeyType_t             keyType,       /*!< [in] The key type that is used as an input to a key derivation function.
                                               Can be one of: CC_UTIL_USER_KEY or CC_UTIL_ROOT_KEY. */
    CCKeyData_t                 *pUserKey,     /*!< [in] A pointer to the user's key buffer (in case of CC_UTIL_USER_KEY). */
    CCUtilPrfType_t             prfType,       /*! [in] The PRF type that is used as an input to a key derivation function.
                                               Can be one of: CC_UTIL_PRF_CMAC or CC_UTIL_PRF_HMAC. */
    CCHashOperationMode_t       hashMode,      /*!< [in]  One of the supported HASH modes, as defined in CCHashOperationMode_t. */
    const uint8_t               *pLabel,       /*!< [in] A string that identifies the purpose for the derived keying material.*/
    size_t                      labelSize,     /*!< [in] The label size should be in range of 1 to 64 bytes length. */
    const uint8_t               *pContextData, /*!< [in] A binary string containing the information related to the derived keying material. */
    size_t                      contextSize,   /*!< [in] The context size should be in range of 1 to 64 bytes length. */
    uint8_t                     *pDerivedKey,  /*!< [out] Keying material output (MUST be atleast the size of derivedKeySize). */
    size_t                      derivedKeySize /*!< [in] Size of the derived keying material in bytes (limited to 4080 bytes). */
    );


/*!
@brief  The key derivation function is as specified in the "KDF in Counter Mode" section of
    NIST Special Publication 800-108: Recommendation for Key Derivation Using Pseudorandom Functions.
    The derivation is based on length l, label L, context C and derivation key Ki.
    In this MACRO, AES-CMAC is used as the pseudorandom function (PRF).

@return CC_UTIL_OK on success.
@return A non-zero value from cc_util_error.h on failure.
*/
#define CC_UtilKeyDerivationCMAC(keyType, pUserKey, pLabel, labelSize, pContextData, contextSize, pDerivedKey, derivedKeySize) \
        CC_UtilKeyDerivation(keyType, pUserKey, CC_UTIL_PRF_CMAC, CC_HASH_OperationModeLast, pLabel, labelSize, pContextData, contextSize, pDerivedKey, derivedKeySize)


/*!
@brief  The key derivation function is as specified in the "KDF in Counter Mode" section of
    NIST Special Publication 800-108: Recommendation for Key Derivation Using Pseudorandom Functions.
    The derivation is based on length l, label L, context C and derivation key Ki.
    In this MACRO, HMAC is used as the pseudorandom function (PRF).

@return CC_UTIL_OK on success.
@return A non-zero value from cc_util_error.h on failure.
*/
#define CC_UtilKeyDerivationHMAC(keyType, pUserKey, hashMode, pLabel, labelSize, pContextData, contextSize, pDerivedKey, derivedKeySize) \
        CC_UtilKeyDerivation(keyType, pUserKey, CC_UTIL_PRF_HMAC, hashMode, pLabel, labelSize, pContextData, contextSize, pDerivedKey, derivedKeySize)


#ifdef __cplusplus
}
#endif
/**
@}
 */
#endif /*_CC_UTIL_KEY_DERIVATION_H*/
