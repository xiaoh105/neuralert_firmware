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

#ifndef MBEDTLS_GCM_ALT_H
#define MBEDTLS_GCM_ALT_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_GCM_C)

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/cipher.h"

#define MBEDTLS_GCM_ENCRYPT     1
#define MBEDTLS_GCM_DECRYPT     0

#define MBEDTLS_ERR_GCM_AUTH_FAILED             -0x0012  /**< Authenticated decryption failed. */
#define MBEDTLS_ERR_GCM_BAD_INPUT               -0x0014  /**< Bad input parameters to function. */

/* hide internal implementation of the struct. Allocate enough space for it.*/
#define MBEDTLS_GCM_CONTEXT_SIZE_IN_WORDS   40

#ifdef __cplusplus
extern "C" {
#endif

#undef	GCM_FCI_OPTIMIZE

/**
 * \brief          GCM context structure
 */
typedef struct {
#ifdef	GCM_FCI_OPTIMIZE
    mbedtls_cipher_id_t cipher;
#endif //CCM_FCI_OPTIMIZE
    uint32_t buf[MBEDTLS_GCM_CONTEXT_SIZE_IN_WORDS];
}
mbedtls_gcm_context;

/**
 * \brief           This function initializes the specified GCM context,
 *                  to make references valid, and prepares the context
 *                  for mbedtls_gcm_setkey() or mbedtls_gcm_free().
 *
 *                  The function does not bind the GCM context to a particular
 *                  cipher, nor set the key. For this purpose, use
 *                  mbedtls_gcm_setkey().
 *
 * \param ctx       The GCM context to initialize.
 */
void mbedtls_gcm_init( mbedtls_gcm_context *ctx );

/**
 * \brief           This function associates a GCM context with a
 *                  cipher algorithm and a key.
 *
 * \param ctx       The GCM context to initialize.
 * \param cipher    The 128-bit block cipher to use.
 * \param key       The encryption key.
 * \param keybits   The key size in bits. Valid options are:
 *                  <ul><li>128 bits</li>
 *                  <li>192 bits</li>
 *                  <li>256 bits</li></ul>
 *
 * \return          \c 0 on success, or a cipher specific error code.
 */
int mbedtls_gcm_setkey( mbedtls_gcm_context *ctx,
                       mbedtls_cipher_id_t cipher,
                       const unsigned char *key,
                       unsigned int keybits );

/**
 * \brief           This function performs GCM encryption or decryption of a buffer.
 *
 * \note For encryption, the output buffer can be the same as the input buffer.
 *       For decryption, the output buffer cannot be the same as input buffer.
 *       If the buffers overlap, the output buffer must trail at least 8 Bytes
 *       behind the input buffer.
 *
 * \param ctx       The GCM context to use for encryption or decryption.
 * \param mode      The operation to perform: #MBEDTLS_GCM_ENCRYPT or
 *                  #MBEDTLS_GCM_DECRYPT.
 * \param length    The length of the input data. This must be a multiple of 16 except in the last call before mbedtls_gcm_finish().
 * \param iv        The initialization vector.
 * \param iv_len    The length of the IV.
 * \param add       The buffer holding the additional data.
 * \param add_len   The length of the additional data.
 * \param input     The buffer holding the input data.
 * \param output    The buffer for holding the output data.
 * \param tag_len   The length of the tag to generate.
 * \param tag       The buffer for holding the tag.
 *
 * \return         \c 0 on success.
 */
int mbedtls_gcm_crypt_and_tag( mbedtls_gcm_context *ctx,
                  int mode,
                  size_t length,
                  const unsigned char *iv,
                  size_t iv_len,
                  const unsigned char *add,
                  size_t add_len,
                  const unsigned char *input,
                  unsigned char *output,
                  size_t tag_len,
                  unsigned char *tag );
/**
 * \brief           This function performs a GCM authenticated decryption of a
 *                  buffer.
 *
 * \note For decryption, the output buffer cannot be the same as input buffer.
 *       If the buffers overlap, the output buffer must trail at least 8 Bytes
 *       behind the input buffer.
 *
 * \param ctx       The GCM context.
 * \param length    The length of the input data. This must be a multiple of 16 except in the last call before mbedtls_gcm_finish().
 * \param iv        The initialization vector.
 * \param iv_len    The length of the IV.
 * \param add       The buffer holding the additional data.
 * \param add_len   The length of the additional data.
 * \param tag       The buffer holding the tag.
 * \param tag_len   The length of the tag.
 * \param input     The buffer holding the input data.
 * \param output    The buffer for holding the output data.
 *
 * \return         0 if successful and authenticated, or
 *                 a non-zero error code on failure.
 */
int mbedtls_gcm_auth_decrypt( mbedtls_gcm_context *ctx,
                      size_t length,
                      const unsigned char *iv,
                      size_t iv_len,
                      const unsigned char *add,
                      size_t add_len,
                      const unsigned char *tag,
                      size_t tag_len,
                      const unsigned char *input,
                      unsigned char *output );


/**************************************************************************************************/
/******                             UN-Supported API's                   **************************/
/**************************************************************************************************/
/**
 * \brief           This function starts a GCM encryption or decryption
 *                  operation.
 *
 * \param ctx       The GCM context.
 * \param mode      The operation to perform: #MBEDTLS_GCM_ENCRYPT or
 *                  #MBEDTLS_GCM_DECRYPT.
 * \param iv        The initialization vector.
 * \param iv_len    The length of the IV.
 * \param add       The buffer holding the additional data, or NULL if \p add_len is 0.
 * \param add_len   The length of the additional data. If 0, \p  add is NULL.
 *
 * \return         \c 0 on success.
 */
int mbedtls_gcm_starts( mbedtls_gcm_context *ctx,
                int mode,
                const unsigned char *iv,
                size_t iv_len,
                const unsigned char *add,
                size_t add_len );

/**
 * \brief           This function feeds an input buffer into an ongoing GCM
 *                  encryption or decryption operation.
 *
 *    `             The function expects input to be a multiple of 16
 *                  Bytes. Only the last call before calling
 *                  mbedtls_gcm_finish() can be less than 16 Bytes.
 *
 * \note For decryption, the output buffer cannot be the same as input buffer.
 *       If the buffers overlap, the output buffer must trail at least 8 Bytes
 *       behind the input buffer.
 *
 * \param ctx       The GCM context.
 * \param length    The length of the input data. This must be a multiple of 16 except in the last call before mbedtls_gcm_finish().
 * \param input     The buffer holding the input data.
 * \param output    The buffer for holding the output data.
 *
 * \return         \c 0 on success, or #MBEDTLS_ERR_GCM_BAD_INPUT on failure.
 */
int mbedtls_gcm_update( mbedtls_gcm_context *ctx,
                size_t length,
                const unsigned char *input,
                unsigned char *output );

/**
 * \brief           This function finishes the GCM operation and generates
 *                  the authentication tag.
 *
 *                  It wraps up the GCM stream, and generates the
 *                  tag. The tag can have a maximum length of 16 Bytes.
 *
 * \param ctx       The GCM context.
 * \param tag       The buffer for holding the tag.
 * \param tag_len   The length of the tag to generate. Must be at least four.
 *
 * \return          \c 0 on success, or #MBEDTLS_ERR_GCM_BAD_INPUT on failure.
 */
int mbedtls_gcm_finish( mbedtls_gcm_context *ctx,
                unsigned char *tag,
                size_t tag_len );

/**************************************************************************************************/
/******                     End of UN-Suppored API's **************************/
/**************************************************************************************************/

void mbedtls_gcm_free( mbedtls_gcm_context *ctx );

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_GCM_C */
#endif /* MBEDTLS_GCM_ALT_H */
