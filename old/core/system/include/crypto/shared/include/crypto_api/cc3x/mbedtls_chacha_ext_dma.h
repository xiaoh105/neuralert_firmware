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
 @brief This file contains all the CryptoCell ChaCha external DMA APIs, their enums and definitions.

 @defgroup mbedtls_chacha_ext_dma CryptoCell ChaCha external DMA APIs
 @brief Contains CryptoCell ChaCha external DMA APIs.

 @{
 @ingroup mbedtls_ext_dma
*/

#ifndef _MBEDTLS_CHACHA_EXT_DMA_H
#define _MBEDTLS_CHACHA_EXT_DMA_H

#include "cc_pal_types.h"
#include "mbedtls_cc_chacha.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*!
* @brief This function initializes the external DMA control. It configures the ChaCha mode, the initial hash value, and other configurations in the ChaCha engine.
* @return \c 0 on success.
* @return A non-zero value from mbedtls_ext_dma_error.h on failure.
*/
int mbedtls_ext_dma_chacha_init(uint8_t *  pNonce,                                  /*!< [in] The nonce buffer.  */
                                mbedtls_chacha_nonce_size_t         nonceSizeFlag,  /*!< [in] The nonce size flag.  */
                                uint8_t *  pKey,                                    /*!< [in] The key buffer.  */
                                uint32_t    keySizeBytes,                           /*!< [in] The size of the key buffer. Must be 32 Bytes.  */
                                uint32_t    initialCounter,                         /*!< [in] Intial counter value.  */
                                mbedtls_chacha_encrypt_mode_t  EncryptDecryptFlag   /*!< [in] The ChaCha operation:<ul><li>Encrypt</li><li>Decrypt</li></ul>. */
                                );


/*!
 * @brief This function frees used resources.
 * @return \c CC_OK on success.
 * @return A non-zero value from mbedtls_ext_dma_error.h on failure.
 */
int mbedtls_chacha_ext_dma_finish(void);



#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MBEDTLS_CHACHA_EXT_DMA_H */

/**
@}
 */

