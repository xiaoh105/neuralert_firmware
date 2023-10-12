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
@brief This file contains the errors definitions of the CryptoCell ChaCha-POLY APIs.
@defgroup cc_chacha_poly_error Specific errors of the CryptoCell ChaCha-POLY APIs
@brief Contains the CryptoCell ChaCha-POLY-API errors definitions.
@{
@ingroup cc_chacha_poly

*/


#ifndef _MBEDTLS_CC_CHACHA_POLY_ERROR_H
#define _MBEDTLS_CC_CHACHA_POLY_ERROR_H


#include "cc_error.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/* The base address of errors for the ChaCha-POLY module - 0x00F02400. */
/*! Invalid additional data. */
#define CC_CHACHA_POLY_ADATA_INVALID_ERROR                      (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x01UL)
/*! Invalid input data. */
#define CC_CHACHA_POLY_DATA_INVALID_ERROR                   (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x02UL)
/*! Illegal encryption mode. */
#define CC_CHACHA_POLY_ENC_MODE_INVALID_ERROR               (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x03UL)
/*! Illegal data size. */
#define CC_CHACHA_POLY_DATA_SIZE_INVALID_ERROR              (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x04UL)
/*! Key-generation error. */
#define CC_CHACHA_POLY_GEN_KEY_ERROR                    (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x05UL)
/*! ChaCha key-generation error. */
#define CC_CHACHA_POLY_ENCRYPTION_ERROR                 (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x06UL)
/*! Authentication error. */
#define CC_CHACHA_POLY_AUTH_ERROR                   (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x07UL)
/*! MAC comparison error. */
#define CC_CHACHA_POLY_MAC_ERROR                                (CC_CHACHA_POLY_MODULE_ERROR_BASE + 0x08UL)

/************************ Enums ********************************/

/************************ Typedefs  ****************************/

/************************ Structs  *****************************/

/************************ Public Variables *********************/

/************************ Public Functions *********************/

#ifdef __cplusplus
}
#endif

#endif //_MBEDTLS_CC_CHACHA_POLY_ERROR_H

/**
@}
 */

