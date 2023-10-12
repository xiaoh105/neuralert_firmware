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
@brief This file contains the error definitions of the CryptoCell POLY APIs.

@defgroup cc_poly_errors Specific errors of the CryptoCell POLY APIs
@brief Contains the CryptoCell POLY-API error definitions.
@{
@ingroup cc_poly
*/

#ifndef _MBEDTLS_CC_POLY_ERROR_H
#define _MBEDTLS_CC_POLY_ERROR_H


#include "cc_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/* The base address of errors for the CryptoCell POLY module - 0x00F02500 */
/*! Invalid key. */
#define CC_POLY_KEY_INVALID_ERROR               (CC_POLY_MODULE_ERROR_BASE + 0x01UL)
/*! Invalid input data. */
#define CC_POLY_DATA_INVALID_ERROR              (CC_POLY_MODULE_ERROR_BASE + 0x02UL)
/*! Illegal input data size. */
#define CC_POLY_DATA_SIZE_INVALID_ERROR         (CC_POLY_MODULE_ERROR_BASE + 0x03UL)
/*! MAC calculation error. */
#define CC_POLY_RESOURCES_ERROR                 (CC_POLY_MODULE_ERROR_BASE + 0x04UL)

/************************ Enums ********************************/

/************************ Typedefs  ****************************/

/************************ Structs  *****************************/

/************************ Public Variables *********************/

/************************ Public Functions *********************/

#ifdef __cplusplus
}
#endif

#endif //_MBEDTLS_CC_POLY_ERROR_H


/**
@}
  */

