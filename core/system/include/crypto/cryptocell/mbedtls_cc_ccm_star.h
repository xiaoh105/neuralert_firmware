/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2017-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
@file

@brief This file contains the CryptoCell AES-CCM star APIs, their enums and definitions.

This API supports AES-CCM*, as defined in <em>IEEE 802.15.4: IEEE Standard for Local and
metropolitan area networks— Part 15.4: Low-Rate Wireless Personal Area Networks (LR-WPANs)</em>,
with the instantiations defined in section B.3.2, and the nonce defined in section 7.3.2.

@defgroup cc_aesccm_star CryptoCell AES-CCM star APIs
@brief Contains the CryptoCell AES-CCM star APIs.
@{
@ingroup cc_aes

*/

#ifndef _MBEDTLS_AES_CCM_STAR_H
#define _MBEDTLS_AES_CCM_STAR_H


#include "cc_pal_types.h"
#include "cc_error.h"

#include "mbedtls/ccm.h"
#include "mbedtls_ccm_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/************************ Typedefs  ****************************/

/************************ Enums ********************************/

/************************ Structs  ******************************/

/************************ context Structs  ******************************/

/**
 * \brief           This function initializes the CCM star context.
 *
 *                  It makes the context ready for mbedtls_ccm_star_setkey() or
 *                  mbedtls_ccm_star_free().
 *
 * \param ctx       The CCM star context to initialize.
 */
void mbedtls_ccm_star_init( mbedtls_ccm_context *ctx );

/**
 * \brief           This function initializes the CCM star context set in the
 *                  \p ctx parameter and sets the encryption or decryption key.
 *
 * \param ctx       The CCM star context to initialize.
 * \param cipher    The 128-bit block cipher to use.
 * \param key       The encryption or decryption key.
 * \param keybits   The size of the key in bits. This must be acceptable by the cipher.
 *
 * \return          \c 0 on success, or a cipher-specific error code on failure.
 */
int mbedtls_ccm_star_setkey( mbedtls_ccm_context *ctx,
                             mbedtls_cipher_id_t cipher,
                             const unsigned char *key,
                             unsigned int keybits );

/**
 * \brief           This function releases and clears the specified CCM star context
 *                  and underlying cipher sub-context.
 *
 * \param ctx       The CCM star context to free.
 */
void mbedtls_ccm_star_free( mbedtls_ccm_context *ctx );
/**
 * \brief           This function encrypts a buffer using CCM star.
 *
 * \param ctx       The CCM star context to use for encryption.
 * \param length    The length of the input data in Bytes.
 * \param iv        Initialization vector (nonce).
 * \param iv_len    The length of the IV in Bytes. Must be 13 Bytes.
 * \param add       The additional data field.
 * \param add_len   The length of additional data in Bytes.
 *                  Must be less than 2^16 - 2^8.
 * \param input     The buffer holding the input data.
 * \param output    The buffer holding the output data.
 *                  Must be at least \p length Bytes wide.
 * \param tag       The buffer holding the tag.
 * \param tag_len   The length of the tag to generate in Bytes:
 *                  4, 6, 8, 10, 14 or 16.
 *
 * \note            The tag is written to a separate buffer. To concatenate
 *                  the \p tag with the \p output, as done in <em>RFC-3610:
 *                  Counter with CBC-MAC (CCM)</em>, use
 *                  \p tag = \p output + \p length, and make sure that the
 *                  output buffer is at least \p length + \p tag_len wide.
 *
 * \return          \c 0 on success.
 */
int mbedtls_ccm_star_encrypt_and_tag( mbedtls_ccm_context *ctx,
                                      size_t length,
                                      const unsigned char *iv,
                                      size_t iv_len,
                                      const unsigned char *add,
                                      size_t add_len,
                                      const unsigned char *input,
                                      unsigned char *output,
                                      unsigned char *tag,
                                      size_t tag_len );

/**
 * \brief           This function performs a CCM star authenticated decryption
 *                  of a buffer.
 *
 * \param ctx       The CCM star context to use for decryption.
 * \param length    The length of the input data in Bytes.
 * \param iv        Initialization vector.
 * \param iv_len    The length of the IV in Bytes.
 * \param add       The additional data field.
 * \param add_len   The length of additional data in Bytes.
 * \param input     The buffer holding the input data.
 * \param output    The buffer holding the output data.
 * \param tag       The buffer holding the tag.
 * \param tag_len   The length of the tag in Bytes.
 *
 * \return          \c 0 if successful and authenticated, or
 *                  #MBEDTLS_ERR_CCM_AUTH_FAILED if the tag does not match.
 */
int mbedtls_ccm_star_auth_decrypt( mbedtls_ccm_context *ctx,
                                   size_t length,
                                   const unsigned char *iv,
                                   size_t iv_len,
                                   const unsigned char *add,
                                   size_t add_len,
                                   const unsigned char *input,
                                   unsigned char *output,
                                   const unsigned char *tag,
                                   size_t tag_len );

/**
 * \brief               This function receives the MAC source address, the frame counter, and the MAC size,
 *                      and returns the required nonce for AES-CCM*, as defined in <em>IEEE 802.15.4: IEEE
 *                      Standard for Local and metropolitan area networks— Part 15.4: Low-Rate Wireless
 *                      Personal Area Networks (LR-WPANs)</em>.
 *
 * \note                This API should be called before mbedtls_ccm_star_init(),
 *                      and the generated nonce should be provided to this function.
 *
 * \param src_addr      The MAC address in EUI-64 format.
 * \param frame_counter The MAC frame counter.
 * \param size_of_t     The size of the AES-CCM* MAC tag in Bytes: 4, 6, 8, 10, 14 or 16.
 * \param nonce_buf     The required nonce for AES-CCM*.
 *
 * \return              \c CC_OK on success.
 *                      A non-zero value on failure, as defined cc_aesccm_error.h.
 */
int mbedtls_ccm_star_nonce_generate( unsigned char * src_addr,
                                     uint32_t frame_counter,
                                     uint8_t size_of_t,
                                     unsigned char * nonce_buf
);

 #ifdef __cplusplus
}
#endif

#endif /* _MBEDTLS_AES_CCM_STAR_H */

/**
@}
*/

