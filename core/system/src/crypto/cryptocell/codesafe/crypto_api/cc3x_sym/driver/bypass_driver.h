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

#ifndef  _BYPASS_DRIVER_H
#define  _BYPASS_DRIVER_H

#include "driver_defs.h"

/******************************************************************************
*               FUNCTION PROTOTYPES
******************************************************************************/
/****************************************************************************************************/
/**
 * @brief This function is used to perform the BYPASS operation in one integrated process.
 *
 *
 * @param[in] pInputBuffInfo A structure which represents the data input buffer.
 * @param[in] inputDataAddrType - the memory address input type: SRAM_ADDR or DLLI_ADDR.
 * @param[in] pOutputBuffInfo A structure which represents the data output buffer.
 * @param[in] outputDataAddrType - the memory address input type: SRAM_ADDR or DLLI_ADDR.
 * @param[in] blockSize - number of bytes to copy.
 *
 * @return drvError_t - On success BYPASS_DRV_OK is returned, on failure a value defined in driver_defs.h
 *
 */
drvError_t ProcessBypass(CCBuffInfo_t *pInputBuffInf, dataAddrType_t inputDataAddrType,
                         CCBuffInfo_t *pOutputBuffInfo, dataAddrType_t outputDataAddrType,
                         uint32_t blockSize);

#endif /* _BYPASS_DRIVER_H */

