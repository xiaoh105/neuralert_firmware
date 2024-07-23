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

#ifndef _SB_CC3X_X509_EXT_PARSER_H
#define _SB_CC3X_X509_EXT_PARSER_H

#include "stdint.h"
#include "secureboot_basetypes.h"
#include "bootimagesverifier_def.h"
#include "secdebug_defs.h"

/*!
 * @brief Parse certificate extension segment
 *
 * @param[in/out] ppCert    - pointer to X509 certificate as ASN.1 byte array
 * @param[in] certType      - certificate type
 * @param[in] cntNumOfImg   - number of images for content certificate (should be 0 for all else)
 * @param[out] pOutStr      - extension data structure according to certificate type
 * @param[in] pOutStrSize   - extension data structure max size
 * @param[in] maxCertSize   - max certificate size
 * @param[in] startAddress      - start address of certificate
 * @param[in] endAddress    - end address of certificate (the certificate pointer cannot exceed this address)
 *
 * @return uint32_t         - On success: the value CC_OK is returned,
 *                    On failure: a value from bsv_error.h
 */
CCError_t SB_X509_ParseCertExtensions(uint8_t       **ppCert,
                                      uint32_t      certSize,
                                      CCSbCertHeader_t **ppCertPropHeader,
                                      uint8_t       **ppNp,
                                      uint8_t       **ppCertBody,
                                      uint32_t  *pCertBodySize,
                                      unsigned long   startAddress,
                                      unsigned long   endAddress);

#endif
