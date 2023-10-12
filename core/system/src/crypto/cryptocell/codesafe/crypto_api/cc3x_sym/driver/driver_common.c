/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2017] ARM Limited or its affiliates.      *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#include "driver_defs.h"
#include "cc_pal_buff_attr.h"
#include "cc_pal_abort.h"
#include "cc_error.h"

/******************************************************************************
*       PUBLIC FUNCTIONS
******************************************************************************/


drvError_t SetDataBuffersInfo(const uint8_t *pDataIn, size_t dataInSize, CCBuffInfo_t *pInputBuffInfo,
                              const uint8_t *pDataOut, size_t dataOutSize, CCBuffInfo_t *pOutputBuffInfo)
{
    drvError_t drvRet = CC_OK;
    uint8_t  buffNs = 0;

    drvRet = CC_PalDataBufferAttrGet(pDataIn, dataInSize, INPUT_DATA_BUFFER, &buffNs);
    if (drvRet != CC_OK){
        CC_PAL_LOG_ERR("input buffer memory is illegal\n");
        return CC_FATAL_ERROR;
    }
    pInputBuffInfo->dataBuffAddr = (uint32_t)pDataIn;
    pInputBuffInfo->dataBuffNs = buffNs;

    if (pOutputBuffInfo != NULL) {
        if (pDataOut != NULL) {
            drvRet = CC_PalDataBufferAttrGet(pDataOut, dataOutSize, OUTPUT_DATA_BUFFER, &buffNs);
            if (drvRet != CC_OK){
                CC_PAL_LOG_ERR("output buffer memory is illegal\n");
                return CC_FATAL_ERROR;
            }
        }
        pOutputBuffInfo->dataBuffAddr = (uint32_t)pDataOut;
        pOutputBuffInfo->dataBuffNs = buffNs;
    }

    return drvRet;
}



