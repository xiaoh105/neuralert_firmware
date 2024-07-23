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


#ifndef _SECDEBUG_API_H
#define _SECDEBUG_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! @file
@brief This file contains the secure debug API definition.
*/

#include "cc_pal_types_plat.h"

/*! SOC-id size. */
#define CC_BSV_SEC_DEBUG_SOC_ID_SIZE            0x20

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
@brief This API enables/disables debug (through the DCU registers), according to the
permissions given in the debug certificate, or predefined values.
For more information, see ARM TrustZone CryptoCell-312 Software Integrator's Manual.


@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvSecureDebugSet(
    unsigned long   hwBaseAddress,  /*!< [in] CryptoCell HW registers' base address. */
    uint32_t   *pDebugCertPkg,  /*!< [in] Pointer to the Secure Debug certificate package. NULL is a valid value. */
    uint32_t   certPkgSize,     /*!< [in] Byte size of the certificate package. */
    uint32_t   *pEnableRmaMode, /*!< [out] RMA entry flag. Non-zero indicates RMA LCS entry is required. */
    uint32_t   *pWorkspace,     /*!< [in] Pointer buffer used internally */
    uint32_t   workspaceSize    /*!< [in] Size of the buffer used internally, minimal size is CC_SB_MIN_DBG_WORKSPACE_SIZE_IN_BYTES. */
    );


/*!
@brief CC_DeviceCompleteDisable
For more information, see ARM TrustZone CryptoCell-312 Software Integrator's Manual.


@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/

CCError_t CC_DeviceCompleteDisable(unsigned long   hwBaseAddress);

#ifdef __cplusplus
}
#endif

#endif



