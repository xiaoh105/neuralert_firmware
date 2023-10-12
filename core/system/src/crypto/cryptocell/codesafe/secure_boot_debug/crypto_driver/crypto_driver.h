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



#ifndef _CRYPTO_DRIVER_H
#define _CRYPTO_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif


#include "secureboot_basetypes.h"
#include "bsv_crypto_api.h"


/*!
 * @brief This function gives the functionality of integrated hash
 *
 * @param[in] hwBaseAddress     - CryptoCell base address
 * @param[out] hashResult   - the HASH result.
 *
 */
#define SBROM_CryptoHash(hwBaseAddress, inputDataAddr, dataSize, hashBuff)       \
    CC_BsvSHA256(hwBaseAddress, (uint8_t *)inputDataAddr, (size_t)dataSize, hashBuff);



#ifdef __cplusplus
}
#endif

#endif



