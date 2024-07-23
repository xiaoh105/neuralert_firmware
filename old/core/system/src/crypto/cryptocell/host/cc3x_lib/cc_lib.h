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
@brief This file contains all of the CryptoCell library basic APIs, their enums and definitions.
@defgroup cc_lib CryptoCell library basic APIs
@brief Contains CryptoCell library basic APIs.

@{
@ingroup cryptocell_api

*/

#ifndef __CC_LIB_H__
#define __CC_LIB_H__

#include "cc_pal_types.h"
#include "cc_rnd_common.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*! Definitions for error returns from ::CC_LibInit or ::CC_LibFini functions. */
typedef enum {
        CC_LIB_RET_OK = 0,          /*!< Success definition.*/
        CC_LIB_RET_EINVAL_CTX_PTR,  /*!< Illegal context pointer.*/
        CC_LIB_RET_EINVAL_WORK_BUF_PTR, /*!< Illegal work-buffer pointer.*/
        CC_LIB_RET_HAL,         /*!< Error returned from the HAL layer.*/
        CC_LIB_RET_PAL,         /*!< Error returned from the PAL layer.*/
        CC_LIB_RET_RND_INST_ERR,    /*!< RND instantiation failed.*/
        CC_LIB_RET_EINVAL_PIDR,     /*!< Invalid peripheral ID. */
        CC_LIB_RET_EINVAL_CIDR,     /*!< Invalid component ID. */
        CC_LIB_AO_WRITE_FAILED_ERR, /*!< Error returned from AO write operation. */
        CC_LIB_RESERVE32B = 0x7FFFFFFFL /*!< Reserved.*/
} CClibRetCode_t;


/*! Internal definition for the product register. */
#define DX_VERSION_PRODUCT_BIT_SHIFT    0x18UL
/*! Internal definition for the product register size. */
#define DX_VERSION_PRODUCT_BIT_SIZE     0x8UL



/*!
@brief This function performs global initialization of the CryptoCell runtime library.

It must be called once per CryptoCell cold-boot cycle.
Among other initializations, this function initializes the RND (TRNG seeding) context. An initialized RND context is required for calling RND APIs, as well as asymmetric-cryptography key-generation and signatures.
The primary context returned by this function can be used as a single global context for all RND needs. Alternatively, other contexts may be initialized and used with a more limited scope, for specific applications or specific threads.

\note The Mutexes, if used, are initialized by this API. Therefore, unlike the other APIs in the library, this API is not thread-safe.

@return \c CC_LIB_RET_OK on success.
@return A non-zero value on failure.
*/
CClibRetCode_t CC_LibInit(CCRndContext_t *rndContext_ptr, /*!< [in/out] A pointer to the RND context buffer allocated by the user. The context is used to maintain the RND state as well as pointers to a function used for random vector generation.
                                    This context must be saved and provided as parameter to any API that uses the RND module.*/
                       CCRndWorkBuff_t  *rndWorkBuff_ptr /*!< [in] Scratchpad for the work of the RND module. */);

/*!
@brief This function finalizes library operations.

It frees the associated resources (mutexes) and call HAL and PAL terminate functions.
The function also cleans the RND context.
@return \c CC_LIB_RET_OK on success.
@return A non-zero value on failure.
*/
CClibRetCode_t CC_LibFini(CCRndContext_t *rndContext_ptr /*!< [in/out] A pointer to the RND context buffer that was initialized in #CC_LibInit.*/);

#ifdef __cplusplus
}
#endif

#endif /*__CC_LIB_H__*/

/**
@}
 */

