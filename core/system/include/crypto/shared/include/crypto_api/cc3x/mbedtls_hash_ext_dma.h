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
 @brief This file contains all the CryptoCell hash external DMA APIs, their enums and definitions.
 @defgroup mbedtls_hash_ext_dma CryptoCell hash external DMA APIs
 @brief Contains CryptoCell hash external DMA APIs.

 @{
 @ingroup mbedtls_ext_dma
*/

#ifndef _MBEDTLS_HASH_EXT_DMA_H
#define _MBEDTLS_HASH_EXT_DMA_H

#include "cc_pal_types.h"
#include "cc_hash_defs.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*!
@brief This function initializes the External DMA Control.

It configures the hash mode, hash initial value and other configurations.

@return \c CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
*/
int mbedtls_hash_ext_dma_init(
    CCHashOperationMode_t  operationMode   /*!< [in] The hash mode. Supported modes are:
                                                      <ul><li>SHA1</li><li>SHA224</li><li>SHA256</li></ul> */
    );



/*!
 * @brief This function returns the digest after the hash operation, and frees used resources.
 * @return \c CC_OK on success.
 * @return A non-zero value from cc_hash_error.h on failure.
 */
int mbedtls_hash_ext_dma_finish(
    CCHashOperationMode_t  operationMode,   /*!< [in] The hash mode. Supported modes are:
                                                      <ul><li>SHA1</li><li>SHA224</li><li>SHA256</li></ul> */
    uint32_t digestBufferSize,              /*!< [in] The size of the hash digest in Bytes. */
    uint32_t *digestBuffer                  /*!< [out] The output digest buffer. */
    );



#ifdef __cplusplus
}
#endif

#endif /* #ifndef MBEDTLS_HASH_EXT_DMA_H_ */

/**
@}
 */

