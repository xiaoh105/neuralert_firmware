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

#include "cc_pal_types.h"
#include "cc_pal_mem.h"
#include "mbedtls_cc_chacha.h"
#include "mbedtls_cc_poly.h"
#include "poly.h"
#include "mbedtls_cc_chacha_poly_error.h"


CIMPORT_C CCError_t  mbedtls_chacha_poly(
        mbedtls_chacha_nonce         pNonce,
                            mbedtls_chacha_key            pKey,
                            mbedtls_chacha_encrypt_mode_t    encryptDecryptFlag,
                uint8_t          *pAddData,
                size_t           addDataSize,
                uint8_t                     *pDataIn,
                            size_t                       dataInSize,
                            uint8_t                     *pDataOut,
                mbedtls_poly_mac         macRes)

{
    CCError_t rc;
    uint8_t chachaInState[CC_CHACHA_BLOCK_SIZE_IN_BYTES] = {0};
    uint8_t chachaOutState[CC_CHACHA_BLOCK_SIZE_IN_BYTES] = {0};
    mbedtls_poly_key polyKey = {0};
    mbedtls_poly_mac polyMac = {0};
    uint8_t *pCipherData = NULL;


    // Verify inputs
    if ((pAddData == NULL) || (addDataSize == 0)) {
        return CC_CHACHA_POLY_ADATA_INVALID_ERROR;
    }
    if ((pDataIn == NULL) || (pDataOut == NULL) || (dataInSize == 0)) {
        return CC_CHACHA_POLY_DATA_INVALID_ERROR;
    }
    if ((macRes == NULL) || (pNonce == NULL) || (pKey == NULL))
        return CC_CHACHA_POLY_DATA_INVALID_ERROR;


    if (encryptDecryptFlag == CC_CHACHA_Encrypt) {
        pCipherData = pDataOut;
    } else if (encryptDecryptFlag == CC_CHACHA_Decrypt) {
        pCipherData = pDataIn;
    } else {
        return CC_CHACHA_POLY_ENC_MODE_INVALID_ERROR;
    }

    // 1. Generate poly key
    // Calling mbedtls_chacha with data=0 is like performing the chacha block function without the encryption
    rc = mbedtls_chacha(pNonce, CC_CHACHA_Nonce96BitSize, pKey, 0, CC_CHACHA_Encrypt, chachaInState, sizeof(chachaInState), chachaOutState);
    if (rc != CC_OK) {
        rc = CC_CHACHA_POLY_GEN_KEY_ERROR;
        goto end_with_error;
    }
    // poly key defined as the first 32 bytes of chacha output.
    CC_PalMemCopy(polyKey, chachaOutState, sizeof(polyKey));

    // 2. Encryption pDataIn
    if (encryptDecryptFlag == CC_CHACHA_Encrypt) {
        rc = mbedtls_chacha(pNonce, CC_CHACHA_Nonce96BitSize, pKey, 1, encryptDecryptFlag, (uint8_t *)pDataIn,  dataInSize, (uint8_t *)pDataOut);
        if (rc != CC_OK) {
            rc = CC_CHACHA_POLY_ENCRYPTION_ERROR;
            goto end_with_error;
        }
    }

    // 3. Authentication
    rc = PolyMacCalc(polyKey, pAddData, addDataSize, pCipherData, dataInSize, polyMac);
    if (rc != CC_OK) {
        rc = CC_CHACHA_POLY_AUTH_ERROR;
        goto end_with_error;
    }
    // 3.1. If encrypt, Calculate mac
    if (encryptDecryptFlag == CC_CHACHA_Encrypt) {
        CC_PalMemCopy(macRes, polyMac, sizeof(polyMac));
        return CC_OK;
    }

    // 3.2. If decrypt, first Verify the expected macRes with polyMac, than decrypt the msg
    if (CC_PalMemCmp(macRes, polyMac, sizeof(polyMac)) != 0) {
        rc = CC_CHACHA_POLY_MAC_ERROR;
        goto end_with_error;
    }
    rc = mbedtls_chacha(pNonce, CC_CHACHA_Nonce96BitSize, pKey, 1, encryptDecryptFlag, (uint8_t *)pDataIn,  dataInSize, (uint8_t *)pDataOut);
    if (rc != CC_OK) {
        rc = CC_CHACHA_POLY_ENCRYPTION_ERROR;
        goto end_with_error;
    }
    return CC_OK;

end_with_error:
    if (pDataOut != NULL) {
        CC_PalMemSetZero(pDataOut, dataInSize);
    }
    return rc;

}

