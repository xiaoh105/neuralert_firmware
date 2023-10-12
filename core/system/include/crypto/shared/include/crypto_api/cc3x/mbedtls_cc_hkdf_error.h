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
@brief This file contains the error definitions of the CryptoCell HKDF APIs.
@defgroup cc_hkdf_error Specific errors of the HMAC key-derivation APIs
@brief Contains the CryptoCell HKDF-API error definitions.
@{
@ingroup cc_hkdf
*/

#ifndef _MBEDTLS_CC_HKDF_ERROR_H
#define _MBEDTLS_CC_HKDF_ERROR_H

#include "cc_error.h"


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines *******************************/

/* The base address for the CryptoCell HKDF module errors - 0x00F01100. */
/*! Invalid argument. */
#define CC_HKDF_INVALID_ARGUMENT_POINTER_ERROR      (CC_HKDF_MODULE_ERROR_BASE + 0x0UL)
/*! Invalid argument size. */
#define CC_HKDF_INVALID_ARGUMENT_SIZE_ERROR         (CC_HKDF_MODULE_ERROR_BASE + 0x1UL)
/*! Illegal hash mode. */
#define CC_HKDF_INVALID_ARGUMENT_HASH_MODE_ERROR        (CC_HKDF_MODULE_ERROR_BASE + 0x3UL)
/*! HKDF not supported. */
#define CC_HKDF_IS_NOT_SUPPORTED                              (CC_HKDF_MODULE_ERROR_BASE + 0xFFUL)

/************************ Enums *********************************/

/************************ Typedefs  *****************************/

/************************ Structs  ******************************/

/************************ Public Variables **********************/

/************************ Public Functions **********************/




#ifdef __cplusplus
}
#endif

#endif //_MBEDTLS_CC_HKDF_ERROR_H
/**
@}
 */

