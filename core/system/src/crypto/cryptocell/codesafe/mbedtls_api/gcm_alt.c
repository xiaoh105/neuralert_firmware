/****************************************************************************
 * The confidential and proprietary information contained in this file may   *
 * only be used by a person authorized under and to the extent permitted     *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.   *
 *  (C) COPYRIGHT [2017] ARM Limited or its affiliates.              *
 *      ALL RIGHTS RESERVED                      *
 * This entire notice must be reproduced on all copies of this file      *
 * and copies of this file may only be made by a person if such person is    *
 * permitted to do so under the terms of a subsisting license agreement      *
 * from ARM Limited or its affiliates.                       *
 *****************************************************************************/

/*
 * Definition of GCM:
 * http://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf
 * Recommendation for Block Cipher Modes of Operation:
 * Galois/Counter Mode (GCM) and GMAC
 *
 * The API supports AES-GCM as defined in NIST SP 800-38D.
 *
 */
#if defined(MBEDTLS_CONFIG_FILE)
    #include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_GCM_C) && defined(MBEDTLS_GCM_ALT)

#include "cc_pal_types.h"
#include "cc_pal_mem.h"
#include "cc_pal_abort.h"
#include "cc_common.h"
#include "aesgcm_driver.h"
#include "mbedtls_common.h"
#include "mbedtls/gcm_alt.h"

#define MBEDTLS_ERR_GCM_API_IS_NOT_SUPPORTED        -0x0016  /**< API is NOT supported. */

/*! AES GCM data in maximal size in bytes. */
#define MBEDTLS_AESGCM_DATA_IN_MAX_SIZE_BYTES       0xFFFF // (64KB - 1)
/*! AES GCM IV maximal size in bytes. */
#define MBEDTLS_AESGCM_IV_MAX_SIZE_BYTES            0xFFFF // (64KB - 1) ToDo: check if it should not be smaller due to the concatenation for J0 Calc.
/*! AES GCM AAD maximal size in bytes. */
#define MBEDTLS_AESGCM_AAD_MAX_SIZE_BYTES           0xFFFF // (64KB - 1)

/*! AES GCM 96 bits size IV. */
#define MBEDTLS_AESGCM_IV_96_BITS_SIZE_BYTES        12

/*! AES GCM Tag size: 4 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_4_BYTES             4
/*! AES GCM Tag size: 8 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_8_BYTES             8
/*! AES GCM Tag size: 12 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_12_BYTES            12
/*! AES GCM Tag size: 13 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_13_BYTES            13
/*! AES GCM Tag size: 14 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_14_BYTES            14
/*! AES GCM Tag size: 15 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_15_BYTES            15
/*! AES GCM Tag size: 16 bytes. */
#define MBEDTLS_AESGCM_TAG_SIZE_16_BYTES            16

#ifdef	GCM_FCI_OPTIMIZE
typedef struct {
    mbedtls_cipher_context_t cipher_ctx;  /*!< The cipher context used. */
    uint64_t HL[16];                      /*!< Precalculated HTable low. */
    uint64_t HH[16];                      /*!< Precalculated HTable high. */
    uint64_t len;                         /*!< The total length of the encrypted data. */
    uint64_t add_len;                     /*!< The total length of the additional data. */
    unsigned char base_ectr[16];          /*!< The first ECTR for tag. */
    unsigned char y[16];                  /*!< The Y working value. */
    unsigned char buf[16];                /*!< The buf working value. */
    int mode;                             /*!< The operation to perform:
                                               #MBEDTLS_GCM_ENCRYPT or
                                               #MBEDTLS_GCM_DECRYPT. */
}
fcimbedtls_gcm_context;

static int fcimbedtls_gcm_setkey( fcimbedtls_gcm_context *ctx,
                        mbedtls_cipher_id_t cipher,
                        const unsigned char *key,
                        unsigned int keybits );

static int fcimbedtls_gcm_starts( fcimbedtls_gcm_context *ctx,
                int mode,
                const unsigned char *iv,
                size_t iv_len,
                const unsigned char *add,
                size_t add_len );

static int fcimbedtls_gcm_update( fcimbedtls_gcm_context *ctx,
                size_t length,
                const unsigned char *input,
                unsigned char *output );

static int fcimbedtls_gcm_finish( fcimbedtls_gcm_context *ctx,
                unsigned char *tag,
                size_t tag_len );

#endif //GCM_FCI_OPTIMIZE

/*
 * Initialize a context
 */
void mbedtls_gcm_init(mbedtls_gcm_context *ctx)
{
    if (NULL == ctx) {
        CC_PalAbort("!!!!GCM context is NULL!!!\n");
    }

    if (sizeof(mbedtls_gcm_context) < sizeof(AesGcmContext_t)) {
        CC_PalAbort("!!!!GCM context sizes mismatch!!!\n");
    }

    mbedtls_zeroize_internal(ctx, sizeof(mbedtls_gcm_context));
}

int mbedtls_gcm_setkey(mbedtls_gcm_context *ctx,
               mbedtls_cipher_id_t cipher,
               const unsigned char *key,
               unsigned int keybits)
{
    AesGcmContext_t *aes_gcm_ctx ;

    if (ctx == NULL || key == NULL) {
        CC_PAL_LOG_ERR("Null pointer, ctx or key are NULL\n");
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }
#ifdef	GCM_FCI_OPTIMIZE
	ctx->cipher = cipher;

	if (cipher != MBEDTLS_CIPHER_ID_AES)
	{
		fcimbedtls_gcm_context *gcmctx ;
		gcmctx = (fcimbedtls_gcm_context *)&(ctx->buf[0]);
		return fcimbedtls_gcm_setkey(gcmctx, cipher, key, keybits);
	}

	aes_gcm_ctx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    if (cipher != MBEDTLS_CIPHER_ID_AES) {
        /* No real use case for GCM other then AES*/
        CC_PAL_LOG_ERR("Only AES cipher id is supported\n");
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    aes_gcm_ctx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    switch (keybits) {
        case 128:
            aes_gcm_ctx->keySizeId = KEY_SIZE_128_BIT;
            break;
        case 192:
            aes_gcm_ctx->keySizeId = KEY_SIZE_192_BIT;
            break;
        case 256:
            aes_gcm_ctx->keySizeId = KEY_SIZE_256_BIT;
            break;
        default:
            return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Copy user key to context */
    CC_PalMemCopy(aes_gcm_ctx->keyBuf, key, (keybits/8));

    return(0);
}

/*
 * Free context
 */
void mbedtls_gcm_free(mbedtls_gcm_context *ctx)
{
    mbedtls_zeroize_internal(ctx, sizeof(mbedtls_gcm_context));
}

static int gcm_calc_h(mbedtls_gcm_context *ctx)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif //GCM_FCI_OPTIMIZE

    /* Set process mode to 'CalcH' */
    pAesGcmCtx->processMode = DRV_AESGCM_Process_CalcH;

    /* set data buffers structures */
    rc = SetDataBuffersInfo((uint8_t*)(pAesGcmCtx->tempBuf), CC_AESGCM_GHASH_DIGEST_SIZE_BYTES, &inBuffInfo,
                            (uint8_t*)(pAesGcmCtx->H), AES_128_BIT_KEY_SIZE, &outBuffInfo);
    if (rc != 0) {
         CC_PAL_LOG_ERR("illegal data buffers\n");
         return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    /* Calculate H */
    rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, CC_AES_BLOCK_SIZE_IN_BYTES);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("calculating H failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    return (0);
}

static int gcm_init(mbedtls_gcm_context *ctx,
                    cryptoDirection_t encryptDecryptFlag,
            const uint8_t* pIv,
            size_t ivSize,
            const uint8_t* pAad,
            size_t aadSize,
            const uint8_t* pDataIn,
            size_t dataSize,
            uint8_t* pDataOut,
            const uint8_t* pTag,
            size_t tagSize)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc;

    /* Check the Encrypt / Decrypt flag validity */
    if (CRYPTO_DIRECTION_NUM_OF_ENC_MODES <= encryptDecryptFlag) {
        return  MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check the data in size validity */
    if (MBEDTLS_AESGCM_DATA_IN_MAX_SIZE_BYTES < dataSize) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    if (0 != dataSize) {
        /* Check dataIn pointer */
        if (NULL == pDataIn) {
            return MBEDTLS_ERR_GCM_BAD_INPUT;
        }

        /* Check dataOut pointer */
        if (NULL == pDataOut) {
            return MBEDTLS_ERR_GCM_BAD_INPUT;
        }
    }

    /* Check the IV size validity */
    if ((MBEDTLS_AESGCM_IV_MAX_SIZE_BYTES < ivSize) || (0 == ivSize)) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check iv pointer */
    if (NULL == pIv) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check the AAD size validity */
    if (MBEDTLS_AESGCM_AAD_MAX_SIZE_BYTES < aadSize) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check aad pointer */
    if ((NULL == pAad) && (aadSize != 0)) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check the Tag size validity */
    if ((MBEDTLS_AESGCM_TAG_SIZE_4_BYTES  != tagSize) && (MBEDTLS_AESGCM_TAG_SIZE_8_BYTES  != tagSize) &&
        (MBEDTLS_AESGCM_TAG_SIZE_12_BYTES != tagSize) && (MBEDTLS_AESGCM_TAG_SIZE_13_BYTES != tagSize) &&
        (MBEDTLS_AESGCM_TAG_SIZE_14_BYTES != tagSize) && (MBEDTLS_AESGCM_TAG_SIZE_15_BYTES != tagSize) &&
        (MBEDTLS_AESGCM_TAG_SIZE_16_BYTES != tagSize)) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Check Tag pointer */
    if (NULL == pTag) {
        return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif //GCM_FCI_OPTIMIZE

    /* Set direction of operation: enc./dec. */
    pAesGcmCtx->dir = MBEDTLS_2_DRIVER_DIRECTION(encryptDecryptFlag);
    pAesGcmCtx->dataSize = dataSize;
    pAesGcmCtx->ivSize = ivSize;
    pAesGcmCtx->aadSize = aadSize;
    pAesGcmCtx->tagSize = tagSize;

    /******************************************************/
    /***                Calculate H                     ***/
    /******************************************************/
    rc = gcm_calc_h(ctx);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("calculating H failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    return 0;
}

static int gcm_process_j0(mbedtls_gcm_context *ctx, const uint8_t* pIv)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    if (MBEDTLS_AESGCM_IV_96_BITS_SIZE_BYTES == pAesGcmCtx->ivSize) {
        // Concatenate IV||0(31)||1
        CC_PalMemCopy(pAesGcmCtx->J0, pIv, MBEDTLS_AESGCM_IV_96_BITS_SIZE_BYTES);
        pAesGcmCtx->J0[3] = SWAP_ENDIAN(0x00000001);
    } else {

        /***********************************************/
        /* Calculate GHASH over the first phase buffer */
        /***********************************************/
        /* Set process mode to 'CalcJ0' */
        pAesGcmCtx->processMode = DRV_AESGCM_Process_CalcJ0_FirstPhase;

        /* set data buffers structures */
        rc = SetDataBuffersInfo(pIv, pAesGcmCtx->ivSize, &inBuffInfo,
                                NULL, 0, &outBuffInfo);
        if (rc != 0) {
             CC_PAL_LOG_ERR("illegal data buffers\n");
             return MBEDTLS_ERR_GCM_AUTH_FAILED;
        }

        /* Calculate J0 - First phase */
        rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, pAesGcmCtx->ivSize);
        if (rc != AES_DRV_OK) {
            CC_PAL_LOG_ERR("calculating J0 (phase 1) failed with error code 0x%X\n", rc);
            return MBEDTLS_ERR_GCM_AUTH_FAILED;
        }

        /*********************************************/
        /* Build & Calculate the second phase buffer */
        /*********************************************/
        CC_PalMemSetZero(pAesGcmCtx->tempBuf, sizeof(pAesGcmCtx->tempBuf));
        pAesGcmCtx->tempBuf[3] = (pAesGcmCtx->ivSize << 3) & BITMASK(CC_BITS_IN_32BIT_WORD);
        pAesGcmCtx->tempBuf[3] = SWAP_ENDIAN(pAesGcmCtx->tempBuf[3]);

        /* Set process mode to 'CalcJ0' */
        pAesGcmCtx->processMode = DRV_AESGCM_Process_CalcJ0_SecondPhase;

        /* set data buffers structures */
        rc = SetDataBuffersInfo((uint8_t*)(pAesGcmCtx->tempBuf), CC_AESGCM_GHASH_DIGEST_SIZE_BYTES, &inBuffInfo,
                                NULL, 0, &outBuffInfo);
        if (rc != 0) {
             CC_PAL_LOG_ERR("illegal data buffers\n");
             return MBEDTLS_ERR_GCM_AUTH_FAILED;
        }

        /* Calculate J0 - Second phase  */
        rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, CC_AESGCM_GHASH_DIGEST_SIZE_BYTES);
        if (rc != AES_DRV_OK) {
            CC_PAL_LOG_ERR("calculating J0 (phase 2) failed with error code %d\n", rc);
            return MBEDTLS_ERR_GCM_AUTH_FAILED;
        }
    }

    return 0;
}

static int gcm_process_aad(mbedtls_gcm_context *ctx, const uint8_t* pAad)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc = 0;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    /* Clear Ghash result buffer */
    CC_PalMemSetZero(pAesGcmCtx->ghashResBuf, sizeof(pAesGcmCtx->ghashResBuf));

    if (0 == pAesGcmCtx->aadSize) {
        return rc;
    }

    /* Set process mode to 'Process_A' */
    pAesGcmCtx->processMode = DRV_AESGCM_Process_A;

    /* set data buffers structures */
    rc = SetDataBuffersInfo(pAad, pAesGcmCtx->aadSize, &inBuffInfo,
                            NULL, 0, &outBuffInfo);
    if (rc != 0) {
         CC_PAL_LOG_ERR("illegal data buffers\n");
         return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    /* Calculate GHASH(A) */
    rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, pAesGcmCtx->aadSize);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("processing AAD failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    return 0;
}

static int gcm_process_cipher(mbedtls_gcm_context *ctx, const uint8_t* pTextDataIn, uint8_t* pTextDataOut)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    /* Must NOT perform in this case */
    if (0 == pAesGcmCtx->dataSize) {
        return 0;
    }

    /* Set process mode to 'Process_DataIn' */
    pAesGcmCtx->processMode = DRV_AESGCM_Process_DataIn;

    /* set data buffers structures */
    rc = SetDataBuffersInfo(pTextDataIn, pAesGcmCtx->dataSize, &inBuffInfo,
                            pTextDataOut, pAesGcmCtx->dataSize, &outBuffInfo);
    if (rc != 0) {
         CC_PAL_LOG_ERR("illegal data buffers\n");
         return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, pAesGcmCtx->dataSize);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("processing cipher failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    return 0;
}

static int gcm_process_lenA_lenC(mbedtls_gcm_context *ctx)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    drvError_t rc;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    /* Build buffer */
    pAesGcmCtx->tempBuf[1] = (pAesGcmCtx->aadSize << 3) & BITMASK(CC_BITS_IN_32BIT_WORD);
    pAesGcmCtx->tempBuf[1] = SWAP_ENDIAN(pAesGcmCtx->tempBuf[1]);
    pAesGcmCtx->tempBuf[0] = 0;
    pAesGcmCtx->tempBuf[3] = (pAesGcmCtx->dataSize << 3) & BITMASK(CC_BITS_IN_32BIT_WORD);
    pAesGcmCtx->tempBuf[3] = SWAP_ENDIAN(pAesGcmCtx->tempBuf[3]);
    pAesGcmCtx->tempBuf[2] = 0;

    /* Set process mode to 'Process_LenA_LenC' */
    pAesGcmCtx->processMode = DRV_AESGCM_Process_LenA_LenC;

   /* set data buffers structures */
    rc = SetDataBuffersInfo((uint8_t*)(pAesGcmCtx->tempBuf), CC_AESGCM_GHASH_DIGEST_SIZE_BYTES, &inBuffInfo,
                            NULL, 0, &outBuffInfo);
    if (rc != 0) {
         CC_PAL_LOG_ERR("illegal data buffers\n");
         return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    /* Calculate GHASH(LenA || LenC) */
    rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, CC_AESGCM_GHASH_DIGEST_SIZE_BYTES);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("processing Lengths of AAD and Cipher failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    return 0;
}

static int gcm_finish(mbedtls_gcm_context *ctx, uint8_t* pTag)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    int rc;
    CCBuffInfo_t inBuffInfo;
    CCBuffInfo_t outBuffInfo;

    /* set pointer to user context */
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE

    /* Set process mode to 'Process_GctrFinal' */
    pAesGcmCtx->processMode = DRV_AESGCM_Process_GctrFinal;

    /* set data buffers structures */
    rc = SetDataBuffersInfo((uint8_t*)(pAesGcmCtx->tempBuf), CC_AESGCM_GHASH_DIGEST_SIZE_BYTES, &inBuffInfo,
                            pAesGcmCtx->preTagBuf, CC_AESGCM_GHASH_DIGEST_SIZE_BYTES, &outBuffInfo);
    if (rc != 0) {
         CC_PAL_LOG_ERR("illegal data buffers\n");
         return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    /* Calculate Encrypt and Calc. Tag */
    rc = ProcessAesGcm(pAesGcmCtx, &inBuffInfo, &outBuffInfo, CC_AESGCM_GHASH_DIGEST_SIZE_BYTES);
    if (rc != AES_DRV_OK) {
        CC_PAL_LOG_ERR("Finish operation failed with error code %d\n", rc);
        return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

    if (CRYPTO_DIRECTION_ENCRYPT == pAesGcmCtx->dir) {
        CC_PalMemCopy(pTag, pAesGcmCtx->preTagBuf, pAesGcmCtx->tagSize);
        rc = 0;
    } else {
        if (0 == CC_PalMemCmp(pAesGcmCtx->preTagBuf, pTag, pAesGcmCtx->tagSize)) {
            rc = 0;
        } else {
            rc = MBEDTLS_ERR_GCM_AUTH_FAILED;
           }
    }

    return rc;
}

static int gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
                 int mode,
                 size_t length,
                 const unsigned char *iv,
                 size_t iv_len,
                 const unsigned char *aad,
                 size_t aad_len,
                 const unsigned char *input,
                 unsigned char *output,
                 size_t tag_len,
                 unsigned char *tag)
{
    AesGcmContext_t *pAesGcmCtx = NULL;
    int rc;

    /* check for  user context */
    if (NULL == ctx) {
    return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

    /* Aes-GCM Initialization function */
    rc = gcm_init(ctx, (cryptoDirection_t)mode,
          iv, iv_len,
          aad, aad_len,
          input, length, output,
          tag, tag_len);

    if (0 != rc) {
    goto gcm_crypt_and_tag_END;
    }

    /* Aes-GCM Process J0 function */
    rc = gcm_process_j0(ctx, iv);
    if (0 != rc) {
    goto gcm_crypt_and_tag_END;
    }

    /* Aes-GCM Process AAD function */
    rc = gcm_process_aad(ctx, aad);
    if (0 != rc) {
    goto gcm_crypt_and_tag_END;
    }

    /* Aes-GCM Process Cipher function */
    rc = gcm_process_cipher(ctx, input, output);
    if (0 != rc) {
    goto gcm_crypt_and_tag_END;
    }

    /* Aes-GCM Process LenA||LenC function */
    rc = gcm_process_lenA_lenC(ctx);
    if (0 != rc) {
    goto gcm_crypt_and_tag_END;
    }

    rc = gcm_finish(ctx, tag);


gcm_crypt_and_tag_END:
    /* set pointer to user context and clear the output in case of failure*/
#ifdef	GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)&(ctx->buf[0]);
#else	//GCM_FCI_OPTIMIZE
    pAesGcmCtx = (AesGcmContext_t *)ctx;
#endif	//GCM_FCI_OPTIMIZE
    if ((CRYPTO_DIRECTION_DECRYPT == pAesGcmCtx->dir) && (MBEDTLS_ERR_GCM_AUTH_FAILED == rc)) {
        CC_PalMemSetZero(output, pAesGcmCtx->dataSize);
    }

    /* Clear working context */
    CC_PalMemSetZero(ctx->buf, sizeof(mbedtls_gcm_context));

    return rc;
}

int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context *ctx,
                  int mode,
                  size_t length,
                  const unsigned char *iv,
                  size_t iv_len,
                  const unsigned char *add,
                  size_t add_len,
                  const unsigned char *input,
                  unsigned char *output,
                  size_t tag_len,
                  unsigned char *tag)
{
    int rc;

#ifdef	GCM_FCI_OPTIMIZE
	if( ctx != NULL && ctx->cipher != MBEDTLS_CIPHER_ID_AES )
	{
	    int ret;

	    if( ( ret = mbedtls_gcm_starts( ctx, mode, iv, iv_len, add, add_len ) ) != 0 )
	        return( ret );

	    if( ( ret = mbedtls_gcm_update( ctx, length, input, output ) ) != 0 )
	        return( ret );

	    if( ( ret = mbedtls_gcm_finish( ctx, tag, tag_len ) ) != 0 )
	        return( ret );

	    return( 0 );
	}
#endif	//GCM_FCI_OPTIMIZE

    rc = gcm_crypt_and_tag(ctx, mode, length,
                   iv, iv_len,
                   add, add_len,
                   input, output,
                   tag_len, tag);
    return rc;
}

int mbedtls_gcm_auth_decrypt( mbedtls_gcm_context *ctx,
                  size_t length,
                  const unsigned char *iv,
                  size_t iv_len,
                  const unsigned char *add,
                  size_t add_len,
                  const unsigned char *tag,
                  size_t tag_len,
                  const unsigned char *input,
                  unsigned char *output )
{
    int rc;

#ifdef	GCM_FCI_OPTIMIZE
	if( ctx != NULL && ctx->cipher != MBEDTLS_CIPHER_ID_AES )
	{
	    int ret;
	    unsigned char check_tag[16];
	    size_t i;
	    int diff;

	    if( ( ret = mbedtls_gcm_crypt_and_tag( ctx, MBEDTLS_GCM_DECRYPT, length,
	                                   iv, iv_len, add, add_len,
	                                   input, output, tag_len, check_tag ) ) != 0 )
	    {
	        return( ret );
	    }

	    /* Check tag in "constant-time" */
	    for( diff = 0, i = 0; i < tag_len; i++ )
	        diff |= tag[i] ^ check_tag[i];

	    if( diff != 0 )
	    {
	        mbedtls_zeroize_internal( output, length );
	        return( MBEDTLS_ERR_GCM_AUTH_FAILED );
	    }

	    return( 0 );
	}
#endif	//GCM_FCI_OPTIMIZE

    rc = gcm_crypt_and_tag(ctx, 0, length, // ToDo: MBEDTLS_GCM_DECRYPT (0) Vs. CRYPTO_DIRECTION_DECRYPT (1)
                   iv, iv_len,
                   add, add_len,
                   input, output,
                   tag_len, (unsigned char *)tag);
    return rc;
}

/**************************************************************************************************/
/******                             UN-Supported API's                   **************************/
/**************************************************************************************************/
int mbedtls_gcm_starts(mbedtls_gcm_context *ctx,
                       int mode,
                       const unsigned char *iv,
               size_t iv_len,
               const unsigned char *aad,
               size_t aad_len)
{
#ifdef	GCM_FCI_OPTIMIZE
	if( ctx != NULL ){
		fcimbedtls_gcm_context *gcmctx ;
		gcmctx = (fcimbedtls_gcm_context *)&(ctx->buf[0]);

		return fcimbedtls_gcm_starts(gcmctx, mode, iv, iv_len, aad, aad_len);
	}
	return MBEDTLS_ERR_GCM_BAD_INPUT;
#else	//GCM_FCI_OPTIMIZE
    CC_UNUSED_PARAM(ctx);
    CC_UNUSED_PARAM(mode);
    CC_UNUSED_PARAM(iv);
    CC_UNUSED_PARAM(iv_len);
    CC_UNUSED_PARAM(aad);
    CC_UNUSED_PARAM(aad_len);

    return (MBEDTLS_ERR_GCM_API_IS_NOT_SUPPORTED);
#endif	//GCM_FCI_OPTIMIZE
}

int mbedtls_gcm_update(mbedtls_gcm_context *ctx,
               size_t length,
               const unsigned char *input,
               unsigned char *output)
{
#ifdef	GCM_FCI_OPTIMIZE
	if( ctx != NULL ){
		fcimbedtls_gcm_context *gcmctx ;
		gcmctx = (fcimbedtls_gcm_context *)&(ctx->buf[0]);

		return fcimbedtls_gcm_update(gcmctx, length, input, output);
	}
	return MBEDTLS_ERR_GCM_BAD_INPUT;
#else	//GCM_FCI_OPTIMIZE
    CC_UNUSED_PARAM(ctx);
    CC_UNUSED_PARAM(length);
    CC_UNUSED_PARAM(input);
    CC_UNUSED_PARAM(output);

    return (MBEDTLS_ERR_GCM_API_IS_NOT_SUPPORTED);
#endif	//GCM_FCI_OPTIMIZE
}

int mbedtls_gcm_finish(mbedtls_gcm_context *ctx,
               unsigned char *tag,
               size_t tag_len)
{
#ifdef	GCM_FCI_OPTIMIZE
	if( ctx != NULL ){
		fcimbedtls_gcm_context *gcmctx ;
		gcmctx = (fcimbedtls_gcm_context *)&(ctx->buf[0]);

		return fcimbedtls_gcm_finish(gcmctx, tag, tag_len);
	}
	return MBEDTLS_ERR_GCM_BAD_INPUT;
#else	//GCM_FCI_OPTIMIZE
    CC_UNUSED_PARAM(ctx);
    CC_UNUSED_PARAM(tag);
    CC_UNUSED_PARAM(tag_len);

    return (MBEDTLS_ERR_GCM_API_IS_NOT_SUPPORTED);
#endif	//GCM_FCI_OPTIMIZE
}
/**************************************************************************************************/

#ifdef	GCM_FCI_OPTIMIZE
/******************************************************************************
 *  Reneasa SW model
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

/*
 * Precompute small multiples of H, that is set
 *      HH[i] || HL[i] = H times i,
 * where i is seen as a field element as in [MGV], ie high-order bits
 * correspond to low powers of P. The result is stored in the same way, that
 * is the high-order bit of HH corresponds to P^0 and the low-order bit of HL
 * corresponds to P^127.
 */
static int fcigcm_gen_table( fcimbedtls_gcm_context *ctx )
{
    int ret, i, j;
    uint64_t hi, lo;
    uint64_t vl, vh;
    unsigned char h[16];
    size_t olen = 0;

    memset( h, 0, 16 );
    if( ( ret = mbedtls_cipher_update( &ctx->cipher_ctx, h, 16, h, &olen ) ) != 0 )
        return( ret );

    /* pack h as two 64-bits ints, big-endian */
    GET_UINT32_BE( hi, h,  0  );
    GET_UINT32_BE( lo, h,  4  );
    vh = (uint64_t) hi << 32 | lo;

    GET_UINT32_BE( hi, h,  8  );
    GET_UINT32_BE( lo, h,  12 );
    vl = (uint64_t) hi << 32 | lo;

    /* 8 = 1000 corresponds to 1 in GF(2^128) */
    ctx->HL[8] = vl;
    ctx->HH[8] = vh;

#if defined(MBEDTLS_AESNI_C) && defined(MBEDTLS_HAVE_X86_64)
    /* With CLMUL support, we need only h, not the rest of the table */
    if( mbedtls_aesni_has_support( MBEDTLS_AESNI_CLMUL ) )
        return( 0 );
#endif

    /* 0 corresponds to 0 in GF(2^128) */
    ctx->HH[0] = 0;
    ctx->HL[0] = 0;

    for( i = 4; i > 0; i >>= 1 )
    {
        uint32_t T = ( vl & 1 ) * 0xe1000000U;
        vl  = ( vh << 63 ) | ( vl >> 1 );
        vh  = ( vh >> 1 ) ^ ( (uint64_t) T << 32);

        ctx->HL[i] = vl;
        ctx->HH[i] = vh;
    }

    for( i = 2; i <= 8; i *= 2 )
    {
        uint64_t *HiL = ctx->HL + i, *HiH = ctx->HH + i;
        vh = *HiH;
        vl = *HiL;
        for( j = 1; j < i; j++ )
        {
            HiH[j] = vh ^ ctx->HH[j];
            HiL[j] = vl ^ ctx->HL[j];
        }
    }

    return( 0 );
}

static int fcimbedtls_gcm_setkey( fcimbedtls_gcm_context *ctx,
                        mbedtls_cipher_id_t cipher,
                        const unsigned char *key,
                        unsigned int keybits )
{
    int ret;
    const mbedtls_cipher_info_t *cipher_info;

    cipher_info = mbedtls_cipher_info_from_values( cipher, keybits, MBEDTLS_MODE_ECB );
    if( cipher_info == NULL )
        return( MBEDTLS_ERR_GCM_BAD_INPUT );

    if( cipher_info->block_size != 16 )
        return( MBEDTLS_ERR_GCM_BAD_INPUT );

    mbedtls_cipher_free( &ctx->cipher_ctx );

    if( ( ret = mbedtls_cipher_setup( &ctx->cipher_ctx, cipher_info ) ) != 0 )
        return( ret );

    if( ( ret = mbedtls_cipher_setkey( &ctx->cipher_ctx, key, keybits,
                               MBEDTLS_ENCRYPT ) ) != 0 )
    {
        return( ret );
    }

    if( ( ret = fcigcm_gen_table( ctx ) ) != 0 )
        return( ret );

    return( 0 );
}

/*
 * Shoup's method for multiplication use this table with
 *      last4[x] = x times P^128
 * where x and last4[x] are seen as elements of GF(2^128) as in [MGV]
 */
static const uint64_t fcilast4[16] =
{
    0x0000, 0x1c20, 0x3840, 0x2460,
    0x7080, 0x6ca0, 0x48c0, 0x54e0,
    0xe100, 0xfd20, 0xd940, 0xc560,
    0x9180, 0x8da0, 0xa9c0, 0xb5e0
};

/*
 * Sets output to x times H using the precomputed tables.
 * x and output are seen as elements of GF(2^128) as in [MGV].
 */
static void fcigcm_mult( fcimbedtls_gcm_context *ctx, const unsigned char x[16],
                      unsigned char output[16] )
{
    int i = 0;
    unsigned char lo, hi, rem;
    uint64_t zh, zl;

#if defined(MBEDTLS_AESNI_C) && defined(MBEDTLS_HAVE_X86_64)
    if( mbedtls_aesni_has_support( MBEDTLS_AESNI_CLMUL ) ) {
        unsigned char h[16];

        PUT_UINT32_BE( ctx->HH[8] >> 32, h,  0 );
        PUT_UINT32_BE( ctx->HH[8],       h,  4 );
        PUT_UINT32_BE( ctx->HL[8] >> 32, h,  8 );
        PUT_UINT32_BE( ctx->HL[8],       h, 12 );

        mbedtls_aesni_gcm_mult( output, x, h );
        return;
    }
#endif /* MBEDTLS_AESNI_C && MBEDTLS_HAVE_X86_64 */

    lo = x[15] & 0xf;

    zh = ctx->HH[lo];
    zl = ctx->HL[lo];

    for( i = 15; i >= 0; i-- )
    {
        lo = x[i] & 0xf;
        hi = x[i] >> 4;

        if( i != 15 )
        {
            rem = (unsigned char) zl & 0xf;
            zl = ( zh << 60 ) | ( zl >> 4 );
            zh = ( zh >> 4 );
            zh ^= (uint64_t) fcilast4[rem] << 48;
            zh ^= ctx->HH[lo];
            zl ^= ctx->HL[lo];

        }

        rem = (unsigned char) zl & 0xf;
        zl = ( zh << 60 ) | ( zl >> 4 );
        zh = ( zh >> 4 );
        zh ^= (uint64_t) fcilast4[rem] << 48;
        zh ^= ctx->HH[hi];
        zl ^= ctx->HL[hi];
    }

    PUT_UINT32_BE( zh >> 32, output, 0 );
    PUT_UINT32_BE( zh, output, 4 );
    PUT_UINT32_BE( zl >> 32, output, 8 );
    PUT_UINT32_BE( zl, output, 12 );
}

static int fcimbedtls_gcm_starts( fcimbedtls_gcm_context *ctx,
                int mode,
                const unsigned char *iv,
                size_t iv_len,
                const unsigned char *add,
                size_t add_len )
{
    int ret;
    unsigned char work_buf[16];
    size_t i;
    const unsigned char *p;
    size_t use_len, olen = 0;

    /* IV and AD are limited to 2^64 bits, so 2^61 bytes */
    /* IV is not allowed to be zero length */
    if( iv_len == 0 ||
      ( (uint64_t) iv_len  ) >> 61 != 0 ||
      ( (uint64_t) add_len ) >> 61 != 0 )
    {
        return( MBEDTLS_ERR_GCM_BAD_INPUT );
    }

    memset( ctx->y, 0x00, sizeof(ctx->y) );
    memset( ctx->buf, 0x00, sizeof(ctx->buf) );

    ctx->mode = mode;
    ctx->len = 0;
    ctx->add_len = 0;

    if( iv_len == 12 )
    {
        memcpy( ctx->y, iv, iv_len );
        ctx->y[15] = 1;
    }
    else
    {
        memset( work_buf, 0x00, 16 );
        PUT_UINT32_BE( iv_len * 8, work_buf, 12 );

        p = iv;
        while( iv_len > 0 )
        {
            use_len = ( iv_len < 16 ) ? iv_len : 16;

            for( i = 0; i < use_len; i++ )
                ctx->y[i] ^= p[i];

            fcigcm_mult( ctx, ctx->y, ctx->y );

            iv_len -= use_len;
            p += use_len;
        }

        for( i = 0; i < 16; i++ )
            ctx->y[i] ^= work_buf[i];

        fcigcm_mult( ctx, ctx->y, ctx->y );
    }

    if( ( ret = mbedtls_cipher_update( &ctx->cipher_ctx, ctx->y, 16, ctx->base_ectr,
                             &olen ) ) != 0 )
    {
        return( ret );
    }

    ctx->add_len = add_len;
    p = add;
    while( add_len > 0 )
    {
        use_len = ( add_len < 16 ) ? add_len : 16;

        for( i = 0; i < use_len; i++ )
            ctx->buf[i] ^= p[i];

        fcigcm_mult( ctx, ctx->buf, ctx->buf );

        add_len -= use_len;
        p += use_len;
    }

    return( 0 );
}

static int fcimbedtls_gcm_update( fcimbedtls_gcm_context *ctx,
                size_t length,
                const unsigned char *input,
                unsigned char *output )
{
    int ret;
    unsigned char ectr[16];
    size_t i;
    const unsigned char *p;
    unsigned char *out_p = output;
    size_t use_len, olen = 0;

    if( output > input && (size_t) ( output - input ) < length )
        return( MBEDTLS_ERR_GCM_BAD_INPUT );

    /* Total length is restricted to 2^39 - 256 bits, ie 2^36 - 2^5 bytes
     * Also check for possible overflow */
    if( ctx->len + length < ctx->len ||
        (uint64_t) ctx->len + length > 0xFFFFFFFE0ull )
    {
        return( MBEDTLS_ERR_GCM_BAD_INPUT );
    }

    ctx->len += length;

    p = input;
    while( length > 0 )
    {
        use_len = ( length < 16 ) ? length : 16;

        for( i = 16; i > 12; i-- )
            if( ++ctx->y[i - 1] != 0 )
                break;

        if( ( ret = mbedtls_cipher_update( &ctx->cipher_ctx, ctx->y, 16, ectr,
                                   &olen ) ) != 0 )
        {
            return( ret );
        }

        for( i = 0; i < use_len; i++ )
        {
            if( ctx->mode == MBEDTLS_GCM_DECRYPT )
                ctx->buf[i] ^= p[i];
            out_p[i] = ectr[i] ^ p[i];
            if( ctx->mode == MBEDTLS_GCM_ENCRYPT )
                ctx->buf[i] ^= out_p[i];
        }

        fcigcm_mult( ctx, ctx->buf, ctx->buf );

        length -= use_len;
        p += use_len;
        out_p += use_len;
    }

    return( 0 );
}

static int fcimbedtls_gcm_finish( fcimbedtls_gcm_context *ctx,
                unsigned char *tag,
                size_t tag_len )
{
    unsigned char work_buf[16];
    size_t i;
    uint64_t orig_len = ctx->len * 8;
    uint64_t orig_add_len = ctx->add_len * 8;

    if( tag_len > 16 || tag_len < 4 )
        return( MBEDTLS_ERR_GCM_BAD_INPUT );

    memcpy( tag, ctx->base_ectr, tag_len );

    if( orig_len || orig_add_len )
    {
        memset( work_buf, 0x00, 16 );

        PUT_UINT32_BE( ( orig_add_len >> 32 ), work_buf, 0  );
        PUT_UINT32_BE( ( orig_add_len       ), work_buf, 4  );
        PUT_UINT32_BE( ( orig_len     >> 32 ), work_buf, 8  );
        PUT_UINT32_BE( ( orig_len           ), work_buf, 12 );

        for( i = 0; i < 16; i++ )
            ctx->buf[i] ^= work_buf[i];

        fcigcm_mult( ctx, ctx->buf, ctx->buf );

        for( i = 0; i < tag_len; i++ )
            tag[i] ^= ctx->buf[i];
    }

    return( 0 );
}

#endif	//GCM_FCI_OPTIMIZE
#endif
