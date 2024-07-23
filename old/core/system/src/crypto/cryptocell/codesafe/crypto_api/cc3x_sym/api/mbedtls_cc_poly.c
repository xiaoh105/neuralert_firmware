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

#define CC_PAL_LOG_CUR_COMPONENT CC_LOG_MASK_CC_API

#include "mbedtls_cc_poly.h"
#include "poly.h"
#include "mbedtls_cc_poly_error.h"


CIMPORT_C CCError_t  mbedtls_poly(
        mbedtls_poly_key         pKey,
                uint8_t                *pDataIn,
                            size_t                  dataInSize,
                            mbedtls_poly_mac        macRes)

{
    CCError_t rc;

    // Verify inputs
    if (pKey == NULL) {
        return CC_POLY_KEY_INVALID_ERROR;
    }
    if ((pDataIn == NULL) || (macRes == NULL) || (dataInSize == 0)) {
        return CC_POLY_DATA_INVALID_ERROR;
    }

    // calculate teh MAC using PKA
    rc = PolyMacCalc(pKey, NULL, 0, pDataIn, dataInSize, macRes);
    if (rc != CC_OK) {
        return CC_POLY_RESOURCES_ERROR;
    }

    return CC_OK;
}

