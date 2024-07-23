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
 @defgroup cc_utils CryptoCell utility APIs
 @brief This contains all utility APIs.
 @{
 @ingroup cryptocell_api
 @}
*/

/*!
 @file
 @brief This file contains general definitions of the CryptoCell utility APIs.
 @defgroup cc_utils_defs CryptoCell utility APIs general definitions
 @brief Contains CryptoCell utility APIs general definitions.
 @{
 @ingroup cc_utils
*/

#ifndef  _MBEDTLS_CC_UTIL_DEFS_H
#define  _MBEDTLS_CC_UTIL_DEFS_H



#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_types_plat.h"
#include "mbedtls_cc_util_key_derivation_defs.h"


/******************************************************************************
*                           DEFINITIONS
******************************************************************************/
/*! The size of the AES 128-bit key in Bytes. */
#define CC_UTIL_AES_128BIT_SIZE 16
/*! The size of the AES 192-bit key in Bytes. */
#define CC_UTIL_AES_192BIT_SIZE 24
/*! The size of the AES 256-bit key in Bytes. */
#define CC_UTIL_AES_256BIT_SIZE 32
/*****************************************/
/* CMAC derive key definitions*/
/*****************************************/
/*! The minimal size of the data for the CMAC derivation operation. */
#define CC_UTIL_CMAC_DERV_MIN_DATA_IN_SIZE  CC_UTIL_FIX_DATA_MIN_SIZE_IN_BYTES+2
/*! The maximal size of the data for CMAC derivation operation. */
#define CC_UTIL_CMAC_DERV_MAX_DATA_IN_SIZE  CC_UTIL_MAX_KDF_SIZE_IN_BYTES
/*! The size of the AES CMAC result in Bytes. */
#define CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES   0x10UL
/*! The size of the AES CMAC result in words. */
#define CC_UTIL_AES_CMAC_RESULT_SIZE_IN_WORDS   (CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES/sizeof(uint32_t))

/*! Util error type. */
typedef uint32_t CCUtilError_t;



/*! Key data. */
typedef struct mbedtls_util_keydata {
    uint8_t*  pKey;     /*!< A pointer to the key. */
    size_t    keySize;  /*!< The size of the key in Bytes. */
}mbedtls_util_keydata;

#ifdef __cplusplus
}
#endif

#endif /*_MBEDTLS_CC_UTIL_DEFS_H*/

/**
@}
 */

