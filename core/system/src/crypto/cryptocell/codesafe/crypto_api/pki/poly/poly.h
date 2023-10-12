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


#ifndef POLY_H
#define POLY_H

/*
 * All the includes that are needed for code using this module to
 * compile correctly should be #included here.
 */

#include "cc_error.h"
#include "mbedtls_cc_poly.h"


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief Generates the POLY mac according to RFC 7539 section 2.5.1
 *
 * @return  CC_OK On success, otherwise indicates failure
 */
CCError_t PolyMacCalc(mbedtls_poly_key      key,        /*!< [in] Poniter to 256 bits of KEY. */
            uint8_t     *pAddData,  /*!< [in] Optional - pointer to additional data if any */
            size_t          addDataSize,    /*!< [in] The size of the additional data */
            uint8_t     *pDataIn,   /*!< [in] Pointer to data buffer to calculate MAC on */
            size_t          dataInSize, /*!< [in] The size of the additional data */
            mbedtls_poly_mac        macRes);        /*!< [out] The calculated MAC */

#ifdef __cplusplus
}
#endif

#endif  //POLY_H
