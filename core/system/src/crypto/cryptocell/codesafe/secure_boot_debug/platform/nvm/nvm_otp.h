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

#ifndef _NVM_OTP_H
#define _NVM_OTP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_crypto_boot_defs.h"

/*------------------------------------
    DEFINES
-------------------------------------*/

/**
 * @brief The NVM_GetSwVersion function is a NVM interface function -
 *        The function retrieves the SW version from the SRAM/NVM.
 *    In case of OTP, we support up to 16 anti-rollback counters (taken from the certificate)
 *
 * @param[in] hwBaseAddress -  CryptoCell base address
 *
 * @param[in] counterId -  relevant only for OTP (valid values: 1,2)
 *
 * @param[out] swVersion   -  the minimum SW version
 *
 * @return CCError_t - On success the value CC_OK is returned, and on failure   -a value from NVM_error.h
 */
CCError_t NVM_GetSwVersion(unsigned long hwBaseAddress, CCSbPubKeyIndexType_t keyIndex, uint32_t* swVersion);


/**
 * @brief The run time secure boot should not support SW version update.
 *
 * @param[in] hwBaseAddress -  CryptoCell base address
 *
 * @param[in] counterId -  relevant only for OTP (valid values: 1,2)
 *
 * @param[out] swVersion   -  the minimum SW version
 *
 * @return CCError_t - always return CC_OK
 */

CCError_t NVM_SetSwVersion(unsigned long hwBaseAddress, CCSbPubKeyIndexType_t keyIndex, uint32_t swVersion);

#ifdef __cplusplus
}
#endif

#endif


