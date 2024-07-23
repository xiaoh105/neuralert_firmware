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

#ifndef MBEDTLS_CMAC_ALT_H
#define MBEDTLS_CMAC_ALT_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif



#include <stddef.h>
#include <stdint.h>


/* hide internal implementation of the struct. Allocate enough space for it.*/
#define MBEDTLS_CMAC_CONTEXT_SIZE_IN_WORDS          33


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          CMAC cipher context structure
 */
struct mbedtls_cmac_context_t{
    /*! Internal buffer */
    uint32_t buf[MBEDTLS_CMAC_CONTEXT_SIZE_IN_WORDS];
};


/**************************************************************************************************/


/**************************************************************************************************/

/**
 * \brief               This function sets the CMAC key, and prepares to authenticate
 *                      the input data.
 *                      Must be called with an initialized cipher context.
 *
 * \param ctx           The cipher context used for the CMAC operation, initialized
 *                      as one of the following types:<ul>
 *                      <li>MBEDTLS_CIPHER_AES_128_ECB</li>
 *                      <li>MBEDTLS_CIPHER_AES_192_ECB</li>
 *                      <li>MBEDTLS_CIPHER_AES_256_ECB</li></ul>
 * \param key           The CMAC key.
 * \param keybits       The length of the CMAC key in bits.
 *                      Must be supported by the cipher.
 *
 * \return              \c 0 on success, or a cipher-specific error code on failure.
 */
int mbedtls_cipher_cmac_starts( mbedtls_cipher_context_t *ctx,
                                const unsigned char *key, size_t keybits );

/**
 * \brief               This function feeds an input buffer into an ongoing CMAC
 *                      computation.
 *
 *                      It is called between mbedtls_cipher_cmac_starts() or
 *                      mbedtls_cipher_cmac_reset(), and mbedtls_cipher_cmac_finish().
 *                      Can be called repeatedly.
 *
 * \param ctx           The cipher context used for the CMAC operation.
 * \param input         The buffer holding the input data.
 * \param ilen          The length of the input data.
 *
 * \returns             \c 0 on success, or a non-zero error code on failure.
 */
int mbedtls_cipher_cmac_update( mbedtls_cipher_context_t *ctx,
                                const unsigned char *input, size_t ilen );

/**
 * \brief               This function finishes the CMAC operation, and writes
 *                      the result to the output buffer.
 *
 *                      It is called after mbedtls_cipher_cmac_update().
 *                      It can be followed by mbedtls_cipher_cmac_reset() and
 *                      mbedtls_cipher_cmac_update(), or mbedtls_cipher_free().
 *
 * \param ctx           The cipher context used for the CMAC operation.
 * \param output        The output buffer for the CMAC checksum result.
 *
 * \returns             \c 0 on success, or a non-zero error code on failure.
 */
int mbedtls_cipher_cmac_finish( mbedtls_cipher_context_t *ctx,
                                unsigned char *output );

/**
 * \brief               This function prepares the authentication of another
 *                      message with the same key as the previous CMAC
 *                      operation.
 *
 *                      It is called after mbedtls_cipher_cmac_finish()
 *                      and before mbedtls_cipher_cmac_update().
 *
 * \param ctx           The cipher context used for the CMAC operation.
 *
 * \returns             \c 0 on success, or a non-zero error code on failure.
 */
int mbedtls_cipher_cmac_reset( mbedtls_cipher_context_t *ctx );

/**
 * \brief               This function calculates the full generic CMAC
 *                      on the input buffer with the provided key.
 *
 *                      The function allocates the context, performs the
 *                      calculation, and frees the context.
 *
 *                      The CMAC result is calculated as
 *                      output = generic CMAC(cmac key, input buffer).
 *
 *
 * \param cipher_info   The cipher information.
 * \param key           The CMAC key.
 * \param keylen        The length of the CMAC key in bits.
 * \param input         The buffer holding the input data.
 * \param ilen          The length of the input data.
 * \param output        The buffer for the generic CMAC result.
 *
 * \returns             \c 0 on success, or a non-zero error code on failure.
 */
int mbedtls_cipher_cmac( const mbedtls_cipher_info_t *cipher_info,
                         const unsigned char *key, size_t keylen,
                         const unsigned char *input, size_t ilen,
                         unsigned char *output );

#if defined(MBEDTLS_AES_C)
/**
 * \brief           This function implements the AES-CMAC-PRF-128 pseudorandom
 *                  function, as defined in
 *                  <em>RFC-4615: The Advanced Encryption Standard-Cipher-based
 *                  Message Authentication Code-Pseudo-Random Function-128
 *                  (AES-CMAC-PRF-128) Algorithm for the Internet Key
 *                  Exchange Protocol (IKE).</em>
 *
 * \param key       The key to use.
 * \param key_len   The key length in Bytes.
 * \param input     The buffer holding the input data.
 * \param in_len    The length of the input data in Bytes.
 * \param output    The buffer holding the generated 16 Bytes of
 *                  pseudorandom output.
 *
 * \return          \c 0 on success.
 */
int mbedtls_aes_cmac_prf_128( const unsigned char *key, size_t key_len,
                              const unsigned char *input, size_t in_len,
                              unsigned char output[16] );
#endif /* MBEDTLS_AES_C */


#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CMAC_ALT_H */
