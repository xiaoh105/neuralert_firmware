/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
 @file
 @brief This file contains all of the CryptoCell ChaCha-POLY APIs, their enums and definitions.

 @defgroup cc_chacha_poly CryptoCell ChaCha-POLY APIs
 @brief Contains CryptoCell ChaCha-POLY APIs.

 @{
 @ingroup cryptocell_api
*/

#ifndef _MBEDTLS_CC_CHACHA_POLY_H
#define _MBEDTLS_CC_CHACHA_POLY_H

#include "cc_pal_types.h"
#include "cc_error.h"
#include "mbedtls_cc_chacha.h"
#include "mbedtls_cc_poly.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*!
@brief This function performs the ChaCha-POLY encryption and authentication operation.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_poly_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha_poly(
        mbedtls_chacha_nonce             pNonce,             /*!< [in] A pointer to a buffer containing the nonce value. */
        mbedtls_chacha_key               pKey,               /*!< [in] A pointer to the key buffer of the user. */
        mbedtls_chacha_encrypt_mode_t    encryptDecryptFlag, /*!< [in] A flag specifying which operation the ChaCha-POLY module should perform: encrypt or decrypt. */
        uint8_t                          *pAddData,          /*!< [in] A pointer to the additional data input buffer to the POLY module.
                                                                       This pointer does not need to be aligned. Must not be null. */
        size_t                           addDataSize,        /*!< [in] The size of the input data. Must not be zero. */
        uint8_t                          *pDataIn,           /*!< [in] A pointer to the input-data buffer to the ChaCha engine.
                                                                       This pointer does not need to be aligned. Must not be null. */
        size_t                           dataInSize,         /*!< [in] The size of the input data. Must not be zero. */
        uint8_t                          *pDataOut,          /*!< [out] A pointer to the output-data buffer from the ChaCha engine.
                                                                        This pointer does not need to be aligned. Must not be null. */
        mbedtls_poly_mac                 macRes              /*!< [in/out] A pointer to the MAC result buffer.*/
);



#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MBEDTLS_CC_CHACHA_POLY_H */

/**
@}
 */

