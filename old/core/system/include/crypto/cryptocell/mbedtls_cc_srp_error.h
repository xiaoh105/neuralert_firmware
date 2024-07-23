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
@brief This file contains the error definitions of the CryptoCell SRP APIs.
@defgroup cc_srp_errors Specific errors of the CryptoCell SRP APIs
@brief Contains the CryptoCell SRP-API error definitions.
@{
@ingroup cc_srp
*/


#ifndef _MBEDTLS_CC_SRP_ERROR_H
#define _MBEDTLS_CC_SRP_ERROR_H


#include "cc_error.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/* The base address errors of the CryptoCell SRP module - 0x00F02600 */
/*! Illegal parameter. */
#define CC_SRP_PARAM_INVALID_ERROR               (CC_SRP_MODULE_ERROR_BASE + 0x01UL)
/*! Illegal modulus size . */
#define CC_SRP_MOD_SIZE_INVALID_ERROR            (CC_SRP_MODULE_ERROR_BASE + 0x02UL)
/*! Illegal state (uninitialized) . */
#define CC_SRP_STATE_UNINITIALIZED_ERROR         (CC_SRP_MODULE_ERROR_BASE + 0x03UL)
/*! Result validation error. */
#define CC_SRP_RESULT_ERROR                  (CC_SRP_MODULE_ERROR_BASE + 0x04UL)
/*! Invalid parameter. */
#define CC_SRP_PARAM_ERROR                   (CC_SRP_MODULE_ERROR_BASE + 0x05UL)
/*! Internal PKI error. */
#define CC_SRP_INTERNAL_ERROR                    (CC_SRP_MODULE_ERROR_BASE + 0x06UL)

/************************ Enums ********************************/

/************************ Typedefs  ****************************/

/************************ Structs  *****************************/

/************************ Public Variables *********************/

/************************ Public Functions *********************/

#ifdef __cplusplus
}
#endif

#endif //_MBEDTLS_CC_SRP_ERROR_H

/**
@}
 */

