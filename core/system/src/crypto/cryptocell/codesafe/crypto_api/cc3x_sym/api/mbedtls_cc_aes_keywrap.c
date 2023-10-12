/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorized under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.                 *
*       ALL RIGHTS RESERVED                                                  *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                                        *
*****************************************************************************/

#include "cc_bitops.h"
#include "aes_driver.h"
#include "cc_aes_defs.h"
#include "mbedtls_cc_aes_key_wrap.h"
#include "mbedtls_cc_aes_key_wrap_defs.h"
#include "mbedtls_cc_aes_key_wrap_error.h"
#include "cc_pal_mem.h"
#include "cc_pal_abort.h"

#include "mbedtls/aes.h"

#pragma GCC diagnostic ignored "-Wconversion"

CCError_t AesKeyWrapInputCheck(mbedtls_keywrap_mode_t keyWrapFlag, CCAesEncryptMode_t encryptDecryptFlag,
        CCAesKeyBuffer_t keyBuf, size_t keySize, uint8_t* pDataIn, size_t semiBlocksNum, uint8_t* pDataOut, size_t dataOutSize)
{
    CCError_t rc = CC_OK;
    uint8_t   keyWrapMinNumOfSemiblocksArr[] = CC_AES_KEYWRAP_MIN_NUM_OF_SEMIBLOCKS_ARR;
    size_t    minSemiBlocks;

    /*************************************/
    /***     Input Validity Checks     ***/
    /*************************************/
    /* Check the Encrypt / Decrypt flag validity */
    if (CC_AES_NUM_OF_ENCRYPT_MODES <= encryptDecryptFlag) {
        return  CC_AES_KEYWRAP_INVALID_ENCRYPT_MODE_ERROR;
    }

    /* Check Data in text pointer */
    if (NULL == pDataIn) {
        return CC_AES_KEYWRAP_DATA_IN_POINTER_INVALID_ERROR;
    }

    /* Check Data out pointer */
    if (NULL == pDataOut) {
        return CC_AES_KEYWRAP_DATA_OUT_POINTER_INVALID_ERROR;
    }

    /* Check the KW / KWP flag validity */
    if (CC_AES_KEYWRAP_NUM_OF_MODES <= keyWrapFlag) {
        return  CC_AES_KEYWRAP_INVALID_KEYWRAP_MODE_ERROR;
    }

    /* Check the semiBlocksNum validity */
    if (keyWrapMinNumOfSemiblocksArr[keyWrapFlag] > semiBlocksNum) {
        return CC_AES_KEYWRAP_SEMIBLOCKS_NUM_ILLEGAL;
    }

    if (CC_AES_ENCRYPT == encryptDecryptFlag) {
        if (((semiBlocksNum+1)<< CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) > dataOutSize) {
            return CC_AES_KEYWRAP_DATA_OUT_SIZE_ILLEGAL;
        }
    } else { /* CC_AES_DECRYPT */
        if( (CC_AES_KEYWRAP_KWP_MODE == keyWrapFlag)){
            if ( (((semiBlocksNum-1)<< CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) - CC_AES_KEYWRAP_MAX_PAD_LEN) > dataOutSize){
                return CC_AES_KEYWRAP_DATA_OUT_SIZE_ILLEGAL;
            }
        } else if (((semiBlocksNum-1)<< CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) > dataOutSize){
            return CC_AES_KEYWRAP_DATA_OUT_SIZE_ILLEGAL;
        }
    }

    minSemiBlocks = (CC_AES_ENCRYPT == encryptDecryptFlag) ? semiBlocksNum+1 : semiBlocksNum;
    if (CC_AES_KEYWRAP_DATA_IN_MAX_SIZE_BYTES < (minSemiBlocks << CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT)) {
        return CC_AES_KEYWRAP_SEMIBLOCKS_NUM_ILLEGAL;
    }

    /* Check key pointer */
    if (NULL == keyBuf) {
        return CC_AES_KEYWRAP_INVALID_KEY_POINTER_ERROR;
    }

    if ((AES_128_BIT_KEY_SIZE != keySize) && (AES_192_BIT_KEY_SIZE != keySize) && (AES_256_BIT_KEY_SIZE != keySize)) {
        return CC_AES_KEYWRAP_ILLEGAL_KEY_SIZE_ERROR;
    }

    return rc;
}

static CCError_t AesWrap(uint8_t* keyBuf, size_t keySize, uint8_t* ICV, uint8_t* pPlainText, size_t semiBlocksNum, uint8_t* pCipherText, mbedtls_keywrap_mode_t keyWrapFlag)
{
    CCError_t          rc = CC_OK;
    uint8_t*       srcAddr = NULL;
    mbedtls_aes_context aesContext;
    uint32_t           i, j;
    DWORD_t            t;
    uint32_t           A[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS];
    uint32_t           inBuff[CC_AES_CRYPTO_BLOCK_SIZE_IN_WORDS];
    uint32_t           outBuff[CC_AES_CRYPTO_BLOCK_SIZE_IN_WORDS];

    /************************
     *        AES Init       *
     *************************/
    mbedtls_aes_init(&aesContext);

    /* AES set key */
    rc = mbedtls_aes_setkey_enc(&aesContext, keyBuf, keySize*CC_BITS_IN_BYTE);
    if (CC_OK != rc)
    {
            goto CC_AesKW_KeyWrap_END;
    }

    if ((CC_AES_KEYWRAP_KWP_MODE == keyWrapFlag) && (CC_AES_KEYWRAP_KWP_MIN_NUM_OF_SEMIBLOCKS == semiBlocksNum))
    {
        /* Copy ICV1 to A */
        CC_PalMemCopy(inBuff, ICV, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
        /* Copy the Plain to Cipher starting from the second semiblock */
        CC_PalMemCopy((inBuff + CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS),
                pPlainText,
                CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
        /* Encrypt the buffer */
        rc = mbedtls_aes_crypt_ecb(&aesContext,
                MBEDTLS_AES_ENCRYPT,
                (uint8_t*)inBuff,
                pCipherText);

        if (CC_OK != rc)
        {
            goto CC_AesKW_KeyWrap_END;
        }
    }
    else
    {
        /* 1. Initialize variables */
        /* Copy ICV1 to A */
        CC_PalMemCopy(A, ICV, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

        /* 2. Calculate intermediate values */
        for (j = 0; j <= CC_AES_KEYWRAP_OUTER_LOOP_LIMIT; j++)
        {
            srcAddr = (j == 0) ? pPlainText : (pCipherText + CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

            for (i = CC_AES_KEYWRAP_INNER_LOOP_LIMIT; i <= semiBlocksNum; i++)
            {
                t.val = (semiBlocksNum * j) + i;
                /* Prepare the buffer to encrypt - In 2 steps */
                CC_PalMemCopy(inBuff, A, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                CC_PalMemCopy((inBuff + CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS),
                          (srcAddr + (i-1)*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES),
                          CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

                /* Encrypt the buffer */

                rc = mbedtls_aes_crypt_ecb(&aesContext,
                        MBEDTLS_AES_ENCRYPT,
                        (uint8_t*)inBuff,
                        (uint8_t*)outBuff);

                if (CC_OK != rc)
                    {
                        goto CC_AesKW_KeyWrap_END;
                    }

                /* MSB64 ^ t */
                CC_PalMemCopy(A, outBuff, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                /* Swap and invert !!! */
                A[0] ^= SWAP_ENDIAN(t.twoWords.valHigh);
                A[1] ^= SWAP_ENDIAN(t.twoWords.valLow);

                /* LSB64 */
                CC_PalMemCopy((pCipherText + i*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES),
                        (uint8_t*)(&outBuff[CALC_32BIT_WORDS_FROM_64BIT_DWORD(1)]),
                        CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
            } // for i
        } // for j

        /* 3. Output the results */
        CC_PalMemCopy(pCipherText, A, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
    } //else

CC_AesKW_KeyWrap_END:
    mbedtls_aes_free(&aesContext);

    return rc;
}

CCError_t mbedtls_aes_key_wrap(mbedtls_keywrap_mode_t keyWrapFlag, uint8_t* keyBuf, size_t keySize, uint8_t* pPlainText, size_t plainTextSize, uint8_t* pCipherText, size_t* pCipherTextSize)
{
    CCError_t rc = CC_OK;
    uint8_t   padLen;
    uint8_t   PAD[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES] = {0x00};
    uint8_t   *ICV;
    uint32_t  ICV1[] = CC_AES_KEYWRAP_ICV1;
    uint32_t  ICV2[] = CC_AES_KEYWRAP_ICV2;
    size_t    semiBlocksNum;

    semiBlocksNum = (plainTextSize + (CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES - 1)) >> CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT;

    /* AES Key Wrap Validation function */
    rc = AesKeyWrapInputCheck(keyWrapFlag, CC_AES_ENCRYPT, keyBuf, keySize, pPlainText, semiBlocksNum, pCipherText, *pCipherTextSize);
    if (CC_OK != rc) {
        goto CC_Aes_KeyWrap_END;
    }

    if (CC_AES_KEYWRAP_KW_MODE == keyWrapFlag) {
        /* Set ICV */
        ICV = (uint8_t*)&ICV1;
    } else {
        /* Set ICV */
        ICV2[1] = SWAP_ENDIAN((uint32_t)plainTextSize);
        ICV = (uint8_t*)&ICV2;

        /* Set PAD */
        padLen = (semiBlocksNum << CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) - plainTextSize;
        CC_PalMemCopy((pPlainText + plainTextSize), PAD, padLen);
    }

    /* The W function*/
    rc = AesWrap(keyBuf, keySize, ICV, pPlainText, semiBlocksNum, pCipherText, keyWrapFlag);
    if (CC_OK != rc) {
        goto CC_Aes_KeyWrap_END;
    }

    /* Set cipher text size (output size) */
    *pCipherTextSize = (semiBlocksNum + 1) << CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT;

 CC_Aes_KeyWrap_END:

     return rc;
}


static CCError_t AesUnWrap(uint8_t* keyBuf, size_t keySize, uint8_t* pCipherText, size_t cipherSemiBlocksNum, uint8_t* pPlainText, size_t plainTextSize, uint32_t* MSB64, uint32_t* LSB64, mbedtls_keywrap_mode_t keyWrapFlag)
{
    CCError_t      rc = CC_OK;
    mbedtls_aes_context aesContext;
    uint8_t*       srcAddr = NULL;
    uint8_t*       inSrcAddr = NULL;
    uint8_t*       destAddr = NULL;
    int32_t        j;
    size_t         semiBlocksNum = cipherSemiBlocksNum -1;
    DWORD_t        t;
    uint32_t       A[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS];
    uint32_t       inBuff[CC_AES_CRYPTO_BLOCK_SIZE_IN_WORDS];
    uint32_t       outBuff[CC_AES_CRYPTO_BLOCK_SIZE_IN_WORDS];
    uint8_t        tempBuff[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES];
    uint32_t i, bytesToCopy = 0;

    /************************
    *        AES Init       *
    *************************/
    mbedtls_aes_init(&aesContext);

    /* AES set key */
    rc = mbedtls_aes_setkey_dec(&aesContext, keyBuf, keySize*CC_BITS_IN_BYTE);
    if (CC_OK != rc)
    {
            goto CC_AesKW_KeyUnWrap_END;
    }
    bytesToCopy = (plainTextSize % CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES) ?
                            (plainTextSize % CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES) : CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES;
    /* 1. Initialize variables */
    if ((CC_AES_KEYWRAP_KWP_MODE == keyWrapFlag) && ((CC_AES_KEYWRAP_KWP_MIN_NUM_OF_SEMIBLOCKS+1) == cipherSemiBlocksNum)) {
        /* Decrypt the buffer */
        rc = mbedtls_aes_crypt_ecb(&aesContext,
                        MBEDTLS_AES_DECRYPT,
                        pCipherText,
                        (uint8_t*)outBuff);
        if (CC_OK != rc) {
            goto CC_AesKW_KeyUnWrap_END;
        }

        /* MSB64 */
        CC_PalMemCopy(MSB64, outBuff, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
        /* LSB64 */
        CC_PalMemCopy(LSB64, (uint8_t*)(&outBuff[CALC_32BIT_WORDS_FROM_64BIT_DWORD(1)]), CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

        /* Copy plaintext with the appropriate size*/
        CC_PalMemCopy(pPlainText, LSB64, bytesToCopy );

    } else {
        /* Copy the Cipher first semiblock to A and the last semiblock to tempBuff*/
        CC_PalMemCopy(A, pCipherText, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
        CC_PalMemCopy(tempBuff, (pCipherText + semiBlocksNum*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES), CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

        /* 2. Calculate intermediate values */
        for (j = CC_AES_KEYWRAP_OUTER_LOOP_LIMIT; j >= 0; j--) {

            srcAddr = (j == CC_AES_KEYWRAP_OUTER_LOOP_LIMIT) ? (pCipherText + CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES) : pPlainText;

            for (i = semiBlocksNum; i >= CC_AES_KEYWRAP_INNER_LOOP_LIMIT; i--) {

                /* Set source and destination addresses*/
                if( i == semiBlocksNum ) {
                    inSrcAddr = tempBuff; /* src for LSB intermediate calculation is the tempBuff*/
                    destAddr = tempBuff; /* destination for LSB intermediate calculation is the tempBuff*/
                } else {
                    inSrcAddr = srcAddr + (i-1)*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES;
                    destAddr = (pPlainText + (i-1)*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                }

                t.val = (semiBlocksNum * j) + i;
                /* Prepare the buffer to encrypt - In 2 steps */
                /* Swap and invert !!! */
                A[0] ^= SWAP_ENDIAN(t.twoWords.valHigh);
                A[1] ^= SWAP_ENDIAN(t.twoWords.valLow);
                CC_PalMemCopy(inBuff, A, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                CC_PalMemCopy((inBuff + CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS),
                              inSrcAddr,
                              CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);

                /* Decrypt the buffer */
                rc = mbedtls_aes_crypt_ecb(&aesContext,
                                        MBEDTLS_AES_DECRYPT,
                                        (uint8_t*)inBuff,
                                        (uint8_t*)outBuff);
                if (CC_OK != rc) {
                    goto CC_AesKW_KeyUnWrap_END;
                }
                /* MSB64 */
                CC_PalMemCopy(A, outBuff, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                /* LSB64 */
                CC_PalMemCopy(destAddr,
                              (uint8_t*)(&outBuff[CALC_32BIT_WORDS_FROM_64BIT_DWORD(1)]),
                              CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                if ( (i == semiBlocksNum) && (j == 0) ){
                /* copy the 64 LS decrypted bits for checking the padding length.
                 * This is the least significant semiblock that is outputed from the encryption cipher -
                 * it holds 1-8 byte of plaintext and 0-7 bytes of padding
                 * (plaintext + padding at the LS semiblock is 8 bytes)*/
                    CC_PalMemCopy(LSB64,
                              (&outBuff[CALC_32BIT_WORDS_FROM_64BIT_DWORD(1)]),
                              CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
                }
            }// for i
        }// for j

        CC_PalMemCopy(MSB64, A, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES);
        /* Copy last bytes of plaintext */
        CC_PalMemCopy(pPlainText + (semiBlocksNum-1)*CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES, LSB64, bytesToCopy );
    } // else

CC_AesKW_KeyUnWrap_END:
    mbedtls_aes_free(&aesContext);
    return rc;
}

CCError_t mbedtls_aes_key_unwrap(mbedtls_keywrap_mode_t keyWrapFlag, uint8_t* keyBuf, size_t keySize, uint8_t* pCipherText, size_t cipherTextSize, uint8_t* pPlainText, size_t* pPlainTextSize)
{
    CCError_t rc = CC_OK;
    uint8_t   PAD[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES] = {0x00};
    uint8_t   *ICV;
    uint32_t  ICV1[] = CC_AES_KEYWRAP_ICV1;
    uint32_t  ICV2[] = CC_AES_KEYWRAP_ICV2;
    uint32_t  MSB64[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS];
    uint32_t  LSB64[CC_AES_KEYWRAP_SEMIBLOCK_SIZE_WORDS];
    uint32_t  Plen;
    int32_t   padLen;
    size_t    semiBlocksNum, bytesToSetZero;
    uint32_t  comparisonOffset = 0;

    semiBlocksNum = (cipherTextSize + (CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES - 1)) >> CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT;
    /* AES Key Wrap Validation function */
    rc = AesKeyWrapInputCheck(keyWrapFlag, CC_AES_DECRYPT, keyBuf, keySize, pPlainText, semiBlocksNum, pCipherText, *pPlainTextSize);
    if (CC_OK != rc) {
        goto CC_Aes_KeyUnWrap_END;
    }

    rc = AesUnWrap(keyBuf, keySize, pCipherText, semiBlocksNum, pPlainText, *pPlainTextSize, MSB64, LSB64, keyWrapFlag);
    if (CC_OK != rc) {
        goto CC_Aes_KeyUnWrap_END;
    }

    /* 3. Check result & Output the results */
    if (CC_AES_KEYWRAP_KW_MODE == keyWrapFlag) {
        /* Set ICV & Compare */
        ICV = (uint8_t*)&ICV1;
        if (CC_PalMemCmp(MSB64, ICV, CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES) != 0) {
            rc = CC_AES_KEYWRAP_UNWRAP_COMPARISON_ERROR;
            goto CC_Aes_KeyUnWrap_END;
        }

        /* Set plaintext size*/
        *pPlainTextSize = (semiBlocksNum -1) << CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT;

    } else {
        /* Set ICV & Compare */
        ICV = (uint8_t*)&ICV2;
        if (CC_PalMemCmp(MSB64, ICV, (CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES >> 1)) != 0) {
            rc = CC_AES_KEYWRAP_UNWRAP_COMPARISON_ERROR;
            goto CC_Aes_KeyUnWrap_END;
        }

        /* Check PadLen range */
        Plen = SWAP_ENDIAN(MSB64[1]);
        padLen = ((semiBlocksNum -1) << CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) - Plen;
        if ((padLen < 0) || (padLen > CC_AES_KEYWRAP_MAX_PAD_LEN)) {
            rc = CC_AES_KEYWRAP_UNWRAP_COMPARISON_ERROR;
            goto CC_Aes_KeyUnWrap_END;
        }

        comparisonOffset = CC_AES_KEYWRAP_SEMIBLOCK_SIZE_BYTES - padLen;
        /* Verify PAD is all 0's */
        if (0 != CC_PalMemCmp(((uint8_t*)LSB64 + comparisonOffset), PAD, padLen)) {
            rc = CC_AES_KEYWRAP_UNWRAP_COMPARISON_ERROR;
            goto CC_Aes_KeyUnWrap_END;
        }

        /* If all verifications passed - verify the length given for the output (plaintext) by the user is large enough */
        if (Plen < *pPlainTextSize) {
            rc = CC_AES_KEYWRAP_DATA_OUT_SIZE_ILLEGAL;
            goto CC_Aes_KeyUnWrap_END;
        }

        /* Set plaintext size*/
        *pPlainTextSize = (size_t)Plen;
    }// else

CC_Aes_KeyUnWrap_END:
    if ( CC_OK != rc){
        /* set output to 0's : size to zeroize is the minimum of the user's output buffer size (*pPlainTextSize)
         * and the maximum possible output for the given ciphertext.*/
        bytesToSetZero = (*pPlainTextSize >  (semiBlocksNum<<CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT) ) ?
                (semiBlocksNum<<CC_AES_KEYWRAP_SEMIBLOCK_TO_BYTES_SHFT): *pPlainTextSize;
        CC_PalMemSetZero(pPlainText, bytesToSetZero);
    }
    return rc;
}


