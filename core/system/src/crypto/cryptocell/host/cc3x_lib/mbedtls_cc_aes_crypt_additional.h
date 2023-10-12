/*****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from Arm Limited (or its affiliates).  *
*   (C) COPYRIGHT [2018] Arm Limited (or its affiliates).                    *
*       ALL RIGHTS RESERVED                                                  *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from Arm Limited (or its affiliates).                                      *
******************************************************************************/
/*!
@defgroup cc_aes CryptoCell AES APIs
@brief Contains all CryptoCell AES APIs.

@{
@ingroup cryptocell_api

@}
*/

/*!
@file
@brief This file contains all CryptoCell AES APIs that are currently not supported by Mbed TLS.

@defgroup cc_aes_crypt_add CryptoCell-specific AES APIs

@{
@ingroup cc_aes

@brief Contains all CryptoCell AES APIs currently not supported by MbedTls.

*/

#ifndef  _MBEDTLS_CC_AES_CRYPT_ADDITIONAL_H
#define  _MBEDTLS_CC_AES_CRYPT_ADDITIONAL_H

#include "mbedtls/aes.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*!
  @brief   This function encrypts or decrypts AES-OFB buffer.

  @note Due to the nature of OFB, you must use the same key
  schedule for both encryption and decryption, so that a context is
  initialized with mbedtls_aes_setkey_enc() for both
  #MBEDTLS_AES_ENCRYPT and #MBEDTLS_AES_DECRYPT.

  @param ctx           The AES context.
  @param length        The length of the data.
  @param nc_off        Not supported. Set to 0.
  @param nonce_counter The 128-bit nonce and counter.
  @param stream_block  Not supported.
  @param input         The input data stream.
  @param output        The output data stream.

  @return         0 on success.
 */
int mbedtls_aes_crypt_ofb( mbedtls_aes_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output );

#ifdef __cplusplus
}
#endif

#endif /*_MBEDTLS_CC_AES_CRYPT_ADDITIONAL_H*/

/**
@}
 */

