/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef  _CC_UTIL_RPMB_H
#define  _CC_UTIL_RPMB_H

/*!
@file
@brief This file contains the functions and definitions for the RPMB.
*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_util_defs.h"
#include "cc_util_error.h"

/******************************************************************************
*                           DEFINITIONS
******************************************************************************/

/*******************************************/
/*   RPMB shared secret key definitions    */
/*******************************************/

#define CC_UTIL_RPMB_DATA_FRAME_SIZE_IN_BYTES        284
#define CC_UTIL_MIN_RPMB_DATA_BUFFERS            1
#define CC_UTIL_MAX_RPMB_DATA_BUFFERS            65535
#define CC_UTIL_HMAC_SHA256_DIGEST_SIZE_IN_WORDS     8

typedef uint8_t     CCUtilRpmbKey_t[CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES*2];
typedef uint8_t     CCUtilRpmbDataBuffer_t[CC_UTIL_RPMB_DATA_FRAME_SIZE_IN_BYTES];
typedef uint32_t    CCUtilHmacResult_t[CC_UTIL_HMAC_SHA256_DIGEST_SIZE_IN_WORDS];


/**
 * @brief This function derives a 256-bit RPMB key by performing AES CMAC on fixed data, using KDR. Because the derivation is
 *    performed based on fixed data, the key does not need to be saved, and can be derived again consistently.
 *
 *
 * @return CC_UTIL_OK on success.
 * @return A non-zero value on failure as defined in cc_util_error.h.
 */
CCUtilError_t CC_UtilDeriveRPMBKey(CCUtilRpmbKey_t pRpmbKey /*!< [out] Pointer to 32byte output, to be used as RPMB key. */);


/**
 * @brief This function computes HMAC SHA-256 authentication code of a sequence of 284 Byte RPMB frames
 *   (as defined in [JESD84]), using the RPMB key (which is derived internally using ::CC_UtilDeriveRPMBKey).
 *
 *
 * @return CC_UTIL_OK on success.
 * @return A non-zero value on failure as defined in cc_util_error.h.
 */
CCUtilError_t CC_UtilSignRPMBFrames(
            unsigned long *pListOfDataFrames, /*!< [in] Pointer to a list of 284 Byte frame addresses. The entire frame list is signed.*/
            size_t listSize,          /*!< [in] The number of 284 Byte frames in the list, up to 65,535. */
            CCUtilHmacResult_t pHmacResult  /*!< [out] Pointer to the output data (HMAC result). */);

#ifdef __cplusplus
}
#endif

#endif /*_CC_UTIL_RPMB_H*/
