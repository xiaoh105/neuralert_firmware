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
@brief This file contains the error definitions of the CryptoCell management APIs.
@defgroup cc_management_error Specific errors of the CryptoCell Management APIs
@brief Contains the CryptoCell management-API error definitions.

@{
@ingroup cc_management

*/

#ifndef _MBEDTLS_CC_MNG_ERROR_H
#define _MBEDTLS_CC_MNG_ERROR_H

#include "cc_error.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/* CryptoCell Management module errors. CC_MNG_MODULE_ERROR_BASE = 0x00F02900 */

/*! Illegal input parameter. */
#define CC_MNG_ILLEGAL_INPUT_PARAM_ERR      (CC_MNG_MODULE_ERROR_BASE + 0x00UL)
/*! Illegal operation. */
#define CC_MNG_ILLEGAL_OPERATION_ERR        (CC_MNG_MODULE_ERROR_BASE + 0x01UL)
/*! Illegal Peripheral ID. */
#define CC_MNG_ILLEGAL_PIDR_ERR             (CC_MNG_MODULE_ERROR_BASE + 0x02UL)
/*! Illegal Component ID. */
#define CC_MNG_ILLEGAL_CIDR_ERR             (CC_MNG_MODULE_ERROR_BASE + 0x03UL)
/*! APB Secure is locked. */
#define CC_MNG_APB_SECURE_IS_LOCKED_ERR         (CC_MNG_MODULE_ERROR_BASE + 0x04UL)
/*! APB Privilege is locked. */
#define CC_MNG_APB_PRIVILEGE_IS_LOCKED_ERR  (CC_MNG_MODULE_ERROR_BASE + 0x05UL)
/*! APBC Secure is locked. */
#define CC_MNG_APBC_SECURE_IS_LOCKED_ERR    (CC_MNG_MODULE_ERROR_BASE + 0x06UL)
/*! APBC Privilege is locked. */
#define CC_MNG_APBC_PRIVILEGE_IS_LOCKED_ERR (CC_MNG_MODULE_ERROR_BASE + 0x07UL)
/*! APBC Instruction is locked. */
#define CC_MNG_APBC_INSTRUCTION_IS_LOCKED_ERR   (CC_MNG_MODULE_ERROR_BASE + 0x08UL)
/*! Invalid Key type. */
#define CC_MNG_INVALID_KEY_TYPE_ERROR       (CC_MNG_MODULE_ERROR_BASE + 0x09UL)
/*! Illegal size of HUK. */
#define CC_MNG_ILLEGAL_HUK_SIZE_ERR     (CC_MNG_MODULE_ERROR_BASE + 0x0AUL)
/*! Illegal size for any HW key other than HUK. */
#define CC_MNG_ILLEGAL_HW_KEY_SIZE_ERR      (CC_MNG_MODULE_ERROR_BASE + 0x0BUL)
/*! HW key is locked. */
#define CC_MNG_HW_KEY_IS_LOCKED_ERR         (CC_MNG_MODULE_ERROR_BASE + 0x0CUL)
/*! Kcp is locked. */
#define CC_MNG_KCP_IS_LOCKED_ERR            (CC_MNG_MODULE_ERROR_BASE + 0x0DUL)
/*! Kce is locked. */
#define CC_MNG_KCE_IS_LOCKED_ERR        (CC_MNG_MODULE_ERROR_BASE + 0x0EUL)
/*! RMA Illegal state. */
#define CC_MNG_RMA_ILLEGAL_STATE_ERR        (CC_MNG_MODULE_ERROR_BASE + 0x0FUL)
/*! Error returned from AO write operation. */
#define CC_MNG_AO_WRITE_FAILED_ERR      (CC_MNG_MODULE_ERROR_BASE + 0x10UL)
/*! APBC access failure. */
#define CC_MNG_APBC_ACCESS_FAILED_ERR       (CC_MNG_MODULE_ERROR_BASE + 0x11UL)
/*! PM SUSPEND/RESUME failure. */
#define CC_MNG_PM_SUSPEND_RESUME_FAILED_ERR (CC_MNG_MODULE_ERROR_BASE + 0x12UL)
/*! SW version failure. */
#define CC_MNG_ILLEGAL_SW_VERSION_ERR       (CC_MNG_MODULE_ERROR_BASE + 0x13UL)
/*! Hash Public Key NA. */
#define CC_MNG_HASH_NOT_PROGRAMMED_ERR      (CC_MNG_MODULE_ERROR_BASE + 0x14UL)
/*! Illegal hash boot key zero count in the OTP error. */
#define CC_MNG_HBK_ZERO_COUNT_ERR           (CC_MNG_MODULE_ERROR_BASE + 0x15UL)

/************************ Enums ********************************/

/************************ Typedefs  ****************************/

/************************ Structs  *****************************/

/************************ Public Variables *********************/

/************************ Public Functions *********************/

#ifdef __cplusplus
}
#endif

#endif // _MBEDTLS_CC_MNG_ERROR_H

/**
@}
 */

