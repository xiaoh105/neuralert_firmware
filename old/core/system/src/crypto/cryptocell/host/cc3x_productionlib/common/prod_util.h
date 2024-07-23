/***************************************************************************
* The confidential and proprietary information contained in this file may
* only be used by a person authorised under and to the extent permitted
* by a subsisting licensing agreement from ARM Limited or its affiliates.
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.
*       ALL RIGHTS RESERVED
* This entire notice must be reproduced on all copies of this file
* and copies of this file may only be made by a person if such person is
* permitted to do so under the terms of a subsisting license agreement
* from ARM Limited or its affiliates.
*****************************************************************************/

#ifndef _PROD_UTIL_H
#define _PROD_UTLI_H

#include <stdint.h>
#include "cc_production_asset.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PROD_MIN(a , b ) ( ( (a) < (b) ) ? (a) : (b) )

uint32_t CC_PROD_PkgVerify(CCProdAssetPkg_t *pPkgAsset,
                            const uint8_t      *pAssetId, uint32_t assetIdSize,
                          const uint8_t     *pLabel, uint32_t labelSize,
                          uint8_t     *pContext, uint32_t contextSize,
                                                    CCPlainAsset_t pPlainAsset,
                               unsigned long workspaceAddr,
                            uint32_t     workspaceSize);

uint32_t  CC_PROD_BitListFromNum(uint32_t *pWordBuff,
                                        uint32_t wordBuffSize,
                                        uint32_t numVal);

uint32_t  CC_PROD_GetZeroCount(uint32_t *pBuff,
                               uint32_t buffWordSize,
                               uint32_t  *pZeroCount);


uint32_t  CCProd_Init(void);

void  CCPROD_Fini(void);


#ifdef __cplusplus
}
#endif
#endif  //_PROD_UTIL_H

