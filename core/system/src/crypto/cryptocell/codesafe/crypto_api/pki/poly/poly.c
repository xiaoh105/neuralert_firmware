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

#include "cc_pal_types.h"
#include "cc_pal_mem.h"
#include "pka.h"
#include "pka_error.h"
#include "pka_hw_defs.h"
#include "mbedtls_cc_poly.h"
#include "mbedtls_cc_poly_error.h"
#include "cc_common.h"

/*! The POLY block size in bytes. */
#define CC_POLY_BLOCK_SIZE_IN_WORDS     4
#define CC_POLY_BLOCK_SIZE_IN_BYTES     (CC_POLY_BLOCK_SIZE_IN_WORDS*CC_32BIT_WORD_SIZE)

extern const int8_t regTemps[PKA_MAX_COUNT_OF_PHYS_MEM_REGS];

// only 5 PKA registers are needed for poly opeartion
#define  PRIME_REG  regTemps[0]
#define  NP_REG     regTemps[1]
#define  ACC_REG    regTemps[2]
#define  KEY_R_REG  regTemps[3]
#define  KEY_S_REG  regTemps[4]
#define  DATA_REG   regTemps[5]
#define  ONE_REG    regTemps[6]
#define POLY_PKA_REGS_NUM   (7+2)   // adding 2 for temp registers

#define POLY_PRIME_SIZE_IN_BITS  130
#define POLY_PRIME_SIZE_IN_WORDS  CALC_FULL_32BIT_WORDS(POLY_PRIME_SIZE_IN_BITS)

// Mask for Key "r" buffer: clearing 4 high bits of indexes 3, 7, 11, 15; clearing low 2 bits of indexes 4,8,12
static const uint8_t g_PolyMaskKeyR[CC_POLY_KEY_SIZE_IN_BYTES/2] =
    {0xff, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0x0f};

// POLY PRIME
static const uint32_t g_PolyPrime[POLY_PRIME_SIZE_IN_WORDS] = {0xfffffffb, 0xffffffff  ,0xffffffff, 0xffffffff, 0x3};

// POLY 1<<128
static const uint32_t g_PolyBit129[] = {0x00000000, 0x00000000  ,0x00000000, 0x00000000, 0x1};

// POLY 1<<128
//const uint32_t g_PolyNp[] = {0x00000003, 0x00000003, 0x00068B20, 0x00000001, 0x00000000};
static const uint32_t g_PolyNp[] = {0x00000000, 0x00000000, 0x00000080, 0x00000000, 0x00000000};


/***********    RsaPrimeTestCall   function      **********************/
/**
 * @brief performs internal opeartions on PKA buffer to calculate the POLY accumulator.
 *  Acc = ((Acc+block)*r) % p.
 *
 * @return  CC_OK On success, otherwise indicates failure
 */
static uint32_t PolyAccCalc(uint8_t *pSrcbuff,     /*!< [in] The pointer to the src buff. */
                            size_t   buffSize,      /*!< [in] The size in bytes of pSrcbuff. */
                bool     padBytes)      /*!< [in] Flag indicating padding 0's to short buffer */
{
    uint32_t shiftBits;

        if ((pSrcbuff == NULL) ||
        (buffSize == 0) ||
        (buffSize > CC_POLY_BLOCK_SIZE_IN_BYTES)) {
        return CC_POLY_DATA_INVALID_ERROR;
    }

    // copy buffer to DATA_REG
    if (buffSize < CC_POLY_BLOCK_SIZE_IN_BYTES) {
        PKA_CLEAR(LEN_ID_MAX_BITS, DATA_REG);
    }
    PkaCopyByteBuffIntoPkaReg(DATA_REG, LEN_ID_MAX_BITS, pSrcbuff, buffSize); // DATA_REG = srcbuff
    if (padBytes == true) {
        buffSize = CC_POLY_BLOCK_SIZE_IN_BYTES;
    }

    // calculate ONE_REG
    if (buffSize == CC_POLY_BLOCK_SIZE_IN_BYTES) {
        PkaCopyDataIntoPkaReg(ONE_REG, LEN_ID_MAX_BITS, g_PolyBit129, CALC_32BIT_WORDS_FROM_BYTES(sizeof(g_PolyBit129)));
    } else {
        PKA_CLEAR(LEN_ID_MAX_BITS, ONE_REG);
        PKA_OR_IM(LEN_ID_MAX_BITS, ONE_REG, ONE_REG, 1);
        while (buffSize > 0) {
            shiftBits = (CC_BITS_IN_BYTE * CC_MIN(buffSize, CC_32BIT_WORD_SIZE));
            PKA_SHL_FILL0(LEN_ID_N_BITS, ONE_REG, ONE_REG, (shiftBits-1));
            buffSize -= (shiftBits/CC_BITS_IN_BYTE);
        }
    }

    // DATA_REG += ONE_REG
    PKA_MOD_ADD(LEN_ID_N_BITS, DATA_REG, DATA_REG, ONE_REG);

    // ACC_REG += DATA_REG
    PKA_MOD_ADD(LEN_ID_N_BITS, ACC_REG, ACC_REG, DATA_REG);

    // ACC_REG = (ACC_REG * KEY_R_REG) % PRIME_REG
    PKA_CLEAR(LEN_ID_MAX_BITS, PKA_REG_T0);
    PKA_CLEAR(LEN_ID_MAX_BITS, PKA_REG_T1);
    PKA_MOD_MUL(LEN_ID_N_BITS, ACC_REG, ACC_REG, KEY_R_REG);

    return CC_OK;

}



/***********    PolyMacCalc   function      **********************/
/**
 * @brief Generates the POLY mac according to RFC 7539 section 2.5.1
 *
 * @return  CC_OK On success, otherwise indicates failure
 */
CCError_t PolyMacCalc(mbedtls_poly_key      key,        /*!< [in] Poniter to 256 bits of KEY. */
            uint8_t     *pAddData,  /*!< [in] Optional - pointer to additional data if any */
            size_t          addDataSize,    /*!< [in] The size in bytes of the additional data */
            uint8_t     *pDataIn,   /*!< [in] Pointer to data buffer to calculate MAC on */
            size_t          dataInSize, /*!< [in] The size in bytes of the additional data */
            mbedtls_poly_mac        macRes)     /*!< [out] The calculated MAC */

{
    uint32_t    rc = CC_OK;
    uint32_t    pkaRegsNum = POLY_PKA_REGS_NUM;
    uint32_t    *pKeyR = NULL;
    uint32_t    *pKeyS = NULL;
    uint8_t lastBlock[CC_POLY_BLOCK_SIZE_IN_BYTES] = {0};
    uint32_t    i = 0;
    uint32_t        processedBytes = 0;
    uint32_t        blockBytes = 0;
    uint64_t    buffSize;
    bool        padBytes = false;

    // verify inputs
    if ((key == NULL) ||
        (pDataIn == NULL) ||
        (dataInSize == 0) ||
        (macRes == NULL) ||
        ((pAddData == NULL) && (addDataSize != 0)) ||
        ((pAddData != NULL) && (addDataSize == 0))) {
        return CC_POLY_DATA_INVALID_ERROR;
    }

    pKeyR = &key[0];  // first half is "r"
    pKeyS = &key[CC_POLY_KEY_SIZE_IN_WORDS/2];  // second half is "s"

    // clamp "r"
    for (i = 0; i < CC_POLY_KEY_SIZE_IN_BYTES/2; i++) {
        ((uint8_t *)pKeyR)[i] = ((uint8_t *)pKeyR)[i] & g_PolyMaskKeyR[i];
    }


    rc = PkaInitAndMutexLock(POLY_PRIME_SIZE_IN_BITS, &pkaRegsNum);
    if (rc != CC_OK) {
            return rc;
    }
    // clear ACC_REG registers
    PKA_CLEAR(LEN_ID_MAX_BITS, ACC_REG );

    // set values in PKA register for the MAC operation:
    // 1. set the prime number to ((1<<130) -5)
    PkaCopyDataIntoPkaReg(PRIME_REG, LEN_ID_MAX_BITS, g_PolyPrime, CALC_32BIT_WORDS_FROM_BYTES(sizeof(g_PolyPrime)));

    // 2. calculate NP for modulus operation
    PkaCopyDataIntoPkaReg(NP_REG, LEN_ID_MAX_BITS, g_PolyNp, CALC_32BIT_WORDS_FROM_BYTES(sizeof(g_PolyNp)));

    // 3. Copy pKeyR to PKA register #2
    PkaCopyDataIntoPkaReg(KEY_R_REG, LEN_ID_MAX_BITS, pKeyR, CC_POLY_KEY_SIZE_IN_WORDS/2);
    // 4. Copy pKeyS to PKA register #3
    PkaCopyDataIntoPkaReg(KEY_S_REG, LEN_ID_MAX_BITS, pKeyS, CC_POLY_KEY_SIZE_IN_WORDS/2);

    // 5. calculate the accumulator
    // first process the additional Data
    processedBytes = 0;
    if (addDataSize > 0) {
        padBytes = true;  // meaning this is chacha-poly AEAD and we need to pad buffers
        while (processedBytes < addDataSize) {
            blockBytes = CC_MIN((addDataSize-processedBytes),CC_POLY_BLOCK_SIZE_IN_BYTES);
            rc = PolyAccCalc(&pAddData[processedBytes], blockBytes, padBytes);
            if (rc != 0) {
                goto end_func;
            }
            processedBytes += blockBytes;
        }
    }
    // Process Data-in
    processedBytes = 0;
    while (processedBytes < dataInSize) {
        blockBytes = CC_MIN((dataInSize-processedBytes),CC_POLY_BLOCK_SIZE_IN_BYTES);
        rc = PolyAccCalc(&pDataIn[processedBytes], blockBytes, padBytes);
        if (rc != 0) {
            goto end_func;
        }
        processedBytes += blockBytes;
    }

    // process the sizes if additional data and data-in
    if (addDataSize > 0) {
        // Fill lastBlock, with addDataSize 8 bytes LE || dataInSize 8 bytes LE
        buffSize = addDataSize;
        CC_PalMemCopy((uint8_t *)lastBlock, (uint8_t *)&buffSize, sizeof(buffSize));
        buffSize = dataInSize;
        CC_PalMemCopy((uint8_t *)&lastBlock[CC_POLY_BLOCK_SIZE_IN_BYTES/2], (uint8_t *)&buffSize, sizeof(buffSize));
        rc = PolyAccCalc(lastBlock, CC_POLY_BLOCK_SIZE_IN_BYTES, false);
        if (rc != 0) {
            goto end_func;
        }
    }

    //6. acc = acc+pkeyS
    PKA_ADD(LEN_ID_N_BITS, ACC_REG, ACC_REG, KEY_S_REG);

    //7. copy acc into macRes
    PkaCopyDataFromPkaReg(macRes, CC_POLY_MAC_SIZE_IN_WORDS, ACC_REG);

end_func:
    PkaFinishAndMutexUnlock(pkaRegsNum);
    return rc;

}

