/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/


/*!
@file
@brief This file contains general definitions of the CryptoCell runtime SW APIs.
@defgroup cc_general_defs CryptoCell general definitions
@brief Contains general definitions of the CryptoCell runtime SW APIs.

@{
@ingroup cryptocell_api

*/
#ifndef _CC_GENERAL_DEFS_H
#define _CC_GENERAL_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_hash_defs.h"

/************************ Defines ******************************/
/*! Hash parameters for HMAC operation. */
typedef struct {
    uint16_t hashResultSize;         /*!< The size of the HMAC hash result. */
    CCHashOperationMode_t hashMode;  /*!< The hash operation mode. */
}HmacHash_t;

/*! The maximal size of the hash string. */
#define CC_HASH_NAME_MAX_SIZE  10

extern const HmacHash_t HmacHashInfo_t[CC_HASH_NumOfModes]; /*!< Hash parameters for HMAC operation. */
extern const uint8_t HmacSupportedHashModes_t[CC_HASH_NumOfModes]; /*!< Supported hash modes. */
extern const char HashAlgMode2mbedtlsString[CC_HASH_NumOfModes][CC_HASH_NAME_MAX_SIZE]; /*!< Hash string names. */


/* general definitions */
/*-------------------------*/
/*! Maximal size of AES HUK in Bytes. */
#define CC_AES_KDR_MAX_SIZE_BYTES   32
/*! Maximal size of AES HUK in words. */
#define CC_AES_KDR_MAX_SIZE_WORDS   (CC_AES_KDR_MAX_SIZE_BYTES/sizeof(uint32_t))


/* Life-cycle states. */
/*! Chip Manufacturer (CM) LCS definition. */
#define CC_LCS_CHIP_MANUFACTURE_LCS     0x0 /*!< The CM LCS value. */
/*! Secure LCS definition. */
#define CC_LCS_SECURE_LCS               0x5 /*!< The Secure LCS value. */

#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

