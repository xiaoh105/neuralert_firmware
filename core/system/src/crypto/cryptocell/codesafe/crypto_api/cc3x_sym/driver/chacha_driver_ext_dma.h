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
#ifndef  _CHACHA_DRIVER_EXT_DMA_H
#define  _CHACHA_DRIVER_EXT_DMA_H

#include "driver_defs.h"




drvError_t FinishChachaExtDma(hashMode_t mode, uint32_t * digest);
drvError_t InitChachaExtDma(uint32_t *nonceBuf, chachaNonceSize_t nonceSizeFlag, uint32_t *keyBuf, uint32_t initialCounter);
drvError_t terminateChachaExtDma(void);




#endif // #_CHACHA_DRIVER_EXT_DMA_H
