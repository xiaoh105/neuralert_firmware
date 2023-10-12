/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorized under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT 2017 ARM Limited or its affiliates.        *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef  _AES_DRIVER_EXT_DMA_H
#define  _AES_DRIVER_EXT_DMA_H

#include "driver_defs.h"
#include "aes_driver.h"





drvError_t finalizeAesExtDma(aesMode_t mode, uint32_t *pIv);
drvError_t terminateAesExtDma(void);


drvError_t AesExtDmaSetIv(aesMode_t mode, uint32_t *pIv);


drvError_t AesExtDmaSetKey(uint32_t *keyBuf, keySizeId_t keySizeId);

drvError_t AesExtDmaInit(cryptoDirection_t encryptDecryptFlag,
        aesMode_t operationMode,
        size_t data_size,
        keySizeId_t keySizeId);



#endif
