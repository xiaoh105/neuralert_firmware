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


/************* Include Files ****************/

#include "cc_pal_mem.h"
#include "cc_hash.h"
#include "cc_hash_error.h"

#include "hash_driver.h"
#include "driver_defs.h"
#include "sw_llfcd_hash.h"
#include "sw_hash_common.h"

#pragma GCC diagnostic ignored "-Wconversion"

/************************ Defines ******************************/


/************************ Enums ******************************/


/************************ Typedefs ******************************/


/************************ Global Data ******************************/

const uint32_t CC_HASH_SHA512_INIT_VECTOR[] =
    {0xf3bcc908UL,0x6a09e667UL,0x84caa73bUL,0xbb67ae85UL,
     0xfe94f82bUL,0x3c6ef372UL,0x5f1d36f1UL,0xa54ff53aUL,
     0xade682d1UL,0x510e527fUL,0x2b3e6c1fUL,0x9b05688cUL,
     0xfb41bd6bUL,0x1f83d9abUL,0x137e2179UL,0x5be0cd19UL};

/************* Private function prototype ****************/


/************************ Public Functions ******************************/

drvError_t InitSwHash512(void  *pCtx)
{
    HashContext_t  *hashCtx;

        /* verify user context pointer */
        if ( pCtx == NULL ) {
                return HASH_DRV_INVALID_USER_CONTEXT_POINTER_ERROR;
        }
    hashCtx = (HashContext_t  *)pCtx;

        /* verify hash valid mode */
        if (hashCtx->mode != HASH_SHA512){
                return HASH_DRV_ILLEGAL_OPERATION_MODE_ERROR;
    }

    CC_PalMemCopy(hashCtx->digest, CC_HASH_SHA512_INIT_VECTOR, CC_HASH_SHA512_DIGEST_SIZE_IN_BYTES);

    hashCtx->valid_tag = HASH_CONTEXT_VALIDATION_TAG;

    return HASH_DRV_OK;
}


drvError_t ProcessSwHash512(void  *pCtx, CCBuffInfo_t *pInputBuffInfo, uint32_t dataInSize )
{

    /* LOCAL DECLERATIONS */
    HashContext_t  *hashCtx;
    uint8_t    *dataIn_ptr;

    /* the number of blocks to update */
    uint32_t numOfBlocksToUpdate, dataSizeToUpdate;
    /* defining a local temp input buffer to handle unaligned data */
    uint32_t TempInputBlockData_Uint32[CC_HASH_SHA512_BLOCK_SIZE_IN_WORDS];

    /* a byte pointer to point on the previous update data stored in the context */

    /* loop variable */
    uint32_t i;

    /* check input parameters */
    if (pInputBuffInfo == NULL) {
         return HASH_DRV_INVALID_USER_DATA_BUFF_POINTER_ERROR;
    }

    /* verify user context pointer */
    if ( pCtx == NULL ) {
         return HASH_DRV_INVALID_USER_CONTEXT_POINTER_ERROR;
    }
    hashCtx = (HashContext_t  *)pCtx;

    if (hashCtx->valid_tag != HASH_CONTEXT_VALIDATION_TAG) {
        return HASH_DRV_USER_CONTEXT_CORRUPTED_ERROR;
    }

    dataIn_ptr = (uint8_t *)pInputBuffInfo->dataBuffAddr;

    /* the number of blocks to update */
    numOfBlocksToUpdate = dataInSize / hashCtx->blockSizeInBytes;
    dataSizeToUpdate = numOfBlocksToUpdate * hashCtx->blockSizeInBytes * 8;

    /* set the operation function pointer accordingly to the selected mode */
    /* update counter */
    HASH_COMMON_IncMsbUnsignedCounter(hashCtx->totalDataSizeProcessed , /* the counter */
                      dataSizeToUpdate,          /* the aditional data */
                      LLF_HASH_SHA2_COUNTER_SIZE_ON_END_OF_PADDING_IN_WORDS);  /* size in words */


    /* If Input data is non aligned perform loop of HASH block operations with copying
       of non aligned Input data to aligned temporary buffer */

    if (IS_ALIGNED(dataIn_ptr,4)) {
        /* loading the next blocks */
        for (i = 0 ; i < numOfBlocksToUpdate ; i++) {
            /* loading the next block dataIn casted to void to avoid compilation warnings,
               if not aligned to word will not be in this if case */
            LLF_HASH_SHA512_SingleBlockUpdate( hashCtx, (uint32_t*)((void*)dataIn_ptr));
            dataIn_ptr += hashCtx->blockSizeInBytes;
        }
    } else {
    /* If Input data is aligned perform loop of HASH block operations directly on
       Input data without its copying to aligned temporary buffer */
        for (i = 0 ; i < numOfBlocksToUpdate ; i++) {

            CC_PalMemCopy(TempInputBlockData_Uint32 , dataIn_ptr ,
                       hashCtx->blockSizeInBytes );

            LLF_HASH_SHA512_SingleBlockUpdate( hashCtx, TempInputBlockData_Uint32);
            dataIn_ptr += hashCtx->blockSizeInBytes;
        }
    }

    return HASH_DRV_OK;
}


drvError_t  FinishSwHash512(void  *pCtx)
{

    HashContext_t  *hashCtx;

    /* defining a local temp input buffer to handle unaligned data */
    uint32_t TempInputBlockData_Uint32[CC_HASH_SHA512_BLOCK_SIZE_IN_WORDS];
    uint8_t *TempInputBlockData_Uint8_ptr;

    /* The padding vector */
    uint8_t PaddingVec[CC_HASH_SHA512_BLOCK_SIZE_IN_BYTES*2];

    /* The padding vector size */
    uint32_t PaddingDataSize;

    /* The position on the padding vector */
    uint16_t PaddingPosIdx;

    /* number of bytes at the end of the padding */
    uint16_t PaddingSizeEndBufferBytes = 0;

    /* the 32 lsb value of the total data size counter */
    uint32_t TotalDataSizeUpdated_32lsb = 0;

    /* counter */
    uint8_t  i;

    /* set the 8 bit pointer to the temp buffer */
    TempInputBlockData_Uint8_ptr = (uint8_t*)TempInputBlockData_Uint32;

        /* verify user context pointer */
        if ( pCtx == NULL ) {
                return HASH_DRV_INVALID_USER_CONTEXT_POINTER_ERROR;
        }
    hashCtx = (HashContext_t  *)pCtx;

    if (hashCtx->valid_tag != HASH_CONTEXT_VALIDATION_TAG) {
        return HASH_DRV_USER_CONTEXT_CORRUPTED_ERROR;
    }

    HASH_COMMON_IncMsbUnsignedCounter(hashCtx->totalDataSizeProcessed,
                      hashCtx->prevDataInSize * 8,
                      LLF_HASH_SHA2_COUNTER_SIZE_ON_END_OF_PADDING_IN_WORDS);  /* size in words */

    TotalDataSizeUpdated_32lsb = CC_COMMON_REVERSE32(hashCtx->totalDataSizeProcessed[3]);
    PaddingSizeEndBufferBytes = LLF_HASH_SHA2_COUNTER_SIZE_ON_END_OF_PADDING_IN_BYTES;



    /* prepare padding */

    /* clear the padding vector */
    CC_PalMemSet(PaddingVec, 0, sizeof(PaddingVec));

    /* calculating the number of padding bytes on the last block - using the 32 lsbits of the size
    ( since it is presented in big endian we are swapping the bytes of the counter ) */
    PaddingDataSize = (hashCtx->blockSizeInBytes -
               (( TotalDataSizeUpdated_32lsb / 8) % hashCtx->blockSizeInBytes));

    /* if there is not enouth bytes for padding add an additional block to the padding vector size */
    if (PaddingDataSize <= PaddingSizeEndBufferBytes) {
        PaddingDataSize += hashCtx->blockSizeInBytes;
    }

    /* set the padding values */
    PaddingVec[0] = LLF_HASH_FIRST_PADDING_BYTE;

#ifdef BIG__ENDIAN
    hashCtx->totalDataSizeProcessed[0] = CC_COMMON_REVERSE32(hashCtx->totalDataSizeProcessed[0]);
    hashCtx->totalDataSizeProcessed[1] = CC_COMMON_REVERSE32(hashCtx->totalDataSizeProcessed[1]);
    hashCtx->totalDataSizeProcessed[2] = CC_COMMON_REVERSE32(hashCtx->totalDataSizeProcessed[2]);
    hashCtx->totalDataSizeProcessed[3] = CC_COMMON_REVERSE32(hashCtx->totalDataSizeProcessed[3]);
#endif

    /* load the value to the end of the padding vector */
    CC_PalMemCopy( &PaddingVec[PaddingDataSize - PaddingSizeEndBufferBytes],
            hashCtx->totalDataSizeProcessed,
            PaddingSizeEndBufferBytes );

    /* initialize the position to 0 */
    PaddingPosIdx = 0;

    /* ................ executing the last block + padding ............... */

    /* load to the temp buffer the data left to load from the previous block stored in the context */
    CC_PalMemCopy( TempInputBlockData_Uint32 ,
            hashCtx->prevDataIn,
            sizeof(TempInputBlockData_Uint32) );

    /* from the position the data is finished load the bytes */
    CC_PalMemCopy( &TempInputBlockData_Uint8_ptr[hashCtx->prevDataInSize],
            PaddingVec,
            hashCtx->blockSizeInBytes -
            hashCtx->prevDataInSize );

    /* update the padding position */
    PaddingPosIdx = hashCtx->blockSizeInBytes - hashCtx->prevDataInSize;

    /* loading the next block */
    LLF_HASH_SHA512_SingleBlockUpdate( hashCtx, TempInputBlockData_Uint32);

    /* ........ loading the second padding block if required ............ */
    if (PaddingPosIdx < PaddingDataSize) {
        /* copy the second buffer to the temp block buffer */
        CC_PalMemCopy( TempInputBlockData_Uint32,
                &PaddingVec[PaddingPosIdx],
                hashCtx->blockSizeInBytes );

        /* loading the next block */
        LLF_HASH_SHA512_SingleBlockUpdate( hashCtx, TempInputBlockData_Uint32);

    }/* end of second block loading case */

#ifndef BIG__ENDIAN
    /* preparing the final result (big- or little-endian accroding to the operation mode */
    for (i = 0; i < CC_HASH_SHA512_DIGEST_SIZE_IN_WORDS; i++) {
        hashCtx->digest[i] = CC_COMMON_REVERSE32(hashCtx->digest[i]);
        {
            uint32_t tempUint;
            if(i%2)
                {
                tempUint = hashCtx->digest[i - 1];
                hashCtx->digest[i - 1] = hashCtx->digest[i];
                hashCtx->digest[i] = tempUint;
            }
        }
    }

#endif

    return HASH_DRV_OK;
}






