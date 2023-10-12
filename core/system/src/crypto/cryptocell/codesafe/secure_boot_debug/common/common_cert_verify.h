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


#ifndef _COMMON_CERT_VERIFY_H
#define _COMMON_CERT_VERIFY_H

#include "common_cert_parser.h"

/**
   @brief This function is used for basic verification of all secure boot/debug certificates.
   it verifies type, size, public key and signature.
   The function returns pointers to certificate proprietary header, and body.
   The function:
   1. calls CCCertFieldsParse() - according to certificate type(x509 or not),
                copy public key, Np and signature to workspace,
                and returns pointers to certificate proprietary header, and body.
   2. Calls CCCertValidateHeader(), and verify cert type (as expected) and size (according to type).
   3. If expected public key hash is NULL, call CC_BsvPubKeyHashGet() with HBK type defined in certificate to get OTP HBK
   4. Calls verifyCertPubKeyAndSign() To verify public key and certificate RSA signature.
 */
CCError_t CCCommonCertVerify(unsigned long   hwBaseAddress,
                             BufferInfo32_t  *pCertInfo,
                             CertFieldsInfo_t  *pCertFields,  // in/out
                             CCSbCertInfo_t  *pSbCertInfo,   //in/out
                             BufferInfo32_t  *pWorkspaceInfo,
                             BufferInfo32_t  *pX509HeaderInfo);


/**
   @brief This function verifies key certificate specific fields.
 */
uint32_t CCCommonKeyCertVerify(unsigned long   hwBaseAddress,
                               uint32_t certFlags,
                               uint8_t  *pCertMain,
                               CCSbCertInfo_t *pCertPkgInfo);

/**
   @brief This function   verifies content certificate specific fields
        Verifies certificate flags, NV counter according to HBK type
        Call CCCertValidateSWComps()
        Call CCSbSetNvCounter()
 */
uint32_t CCCommonContentCertVerify(CCSbFlashReadFunc flashReadFunc,
                                   void *userContext,
                                   unsigned long hwBaseAddress,
                                   CCAddr_t certStoreAddress,
                                   CCSbCertInfo_t *certPkgInfo,
                                   uint32_t certFlags,
                                   uint8_t *pCertMain,
                                   BufferInfo32_t  *pWorkspaceInfo);


#endif /* _COMMON_CERT_VERIFY_H */


