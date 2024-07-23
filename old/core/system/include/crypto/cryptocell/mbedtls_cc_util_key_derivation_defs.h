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
 @brief This file contains the definitions for the key-derivation API.
 @defgroup cc_utils_key_defs CryptoCell utils general key definitions
 @brief Contains KDF API definitions.
 @{
 @ingroup cc_utils
*/

#ifndef  _CC_UTIL_KEY_DERIVATION_DEFS_H
#define  _CC_UTIL_KEY_DERIVATION_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
*                           DEFINITIONS
******************************************************************************/

/*! The maximal length of the label in Bytes. */
#define CC_UTIL_MAX_LABEL_LENGTH_IN_BYTES   64
/*! The maximal length of the context in Bytes. */
#define CC_UTIL_MAX_CONTEXT_LENGTH_IN_BYTES     64
/*! The minimal size of the fixed data in Bytes. */
#define CC_UTIL_FIX_DATA_MIN_SIZE_IN_BYTES  3 /*!< \internal counter, 0x00, lengt(-0xff) */
/*! The maximal size of the fixed data in Bytes. */
#define CC_UTIL_FIX_DATA_MAX_SIZE_IN_BYTES  4 /*!< \internal counter, 0x00, lengt(0x100-0xff0) */
/*! The maximal size of the derived-key material in Bytes. */
#define CC_UTIL_MAX_KDF_SIZE_IN_BYTES (CC_UTIL_MAX_LABEL_LENGTH_IN_BYTES+CC_UTIL_MAX_CONTEXT_LENGTH_IN_BYTES+CC_UTIL_FIX_DATA_MAX_SIZE_IN_BYTES)
/*! The maximal size of the derived-key in Bytes. */
#define CC_UTIL_MAX_DERIVED_KEY_SIZE_IN_BYTES 4080

#ifdef __cplusplus
}
#endif

#endif /*_CC_UTIL_KEY_DERIVATION_DEFS_H*/

/**
@}
 */

