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
 @brief This file contains all of the definitions and structures used for the Secure Boot and Secure Debug.

 @defgroup cc_sb_gen_defs CryptoCell Secure Boot and Secure Debug definitions and structures
 @brief Contains CryptoCell Secure Boot and Secure Debug definitions and structures.
 @{
 @ingroup cc_sb
*/

#ifndef _SECURE_BOOT_GEN_DEFS_H
#define _SECURE_BOOT_GEN_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif


#include "secureboot_basetypes.h"
#include "cc_sec_defs.h"
#include "cc_pal_sb_plat.h"

/* General definitions */
/***********************/

/*! The maximal size of the additional-data of the Secure Boot in Bytes. */
#define CC_SB_MAX_SIZE_ADDITIONAL_DATA_BYTES    128

/*! Definition of public key hash array. */
typedef uint32_t CCSbCertPubKeyHash_t[HASH_RESULT_SIZE_IN_WORDS];
/*! Definition of SOC ID array. */
typedef uint32_t CCSbCertSocId_t[HASH_RESULT_SIZE_IN_WORDS];


/********* Function pointer definitions ***********/

/*! @brief Typedef of the Flash read function pointer, to be implemented by the partner.

Used for reading the certificates and SW modules from flash memory.

\note It is your responsibility to verify that this function does not copy data from restricted memory regions.

@param [in] flashAddress The address for reading from flash memory.
@param [out] memDst A pointer to the RAM destination address to write the data to.
@param [in] sizeToRead The size to read in Bytes.
@param [in] context For partner use.
*/

typedef uint32_t (*CCSbFlashReadFunc) (
                     CCAddr_t flashAddress,
                     uint8_t *memDst,
                     uint32_t sizeToRead,
                     void* context
                     );


/*! Typedef of the Flash write function pointer, to be implemented by the partner.

Used for writing authenticated and decrypted SW modules to flash memory.

@param [in] flashAddress The address for writing to flash memory.
@param [out] memSrc A pointer to the RAM source to read the data from.
@param [in] sizeToWrite The size to write in Bytes.
@param [in] context For partner use.
*/
typedef uint32_t (*CCBsvFlashWriteFunc) (
                    CCAddr_t flashAddress,
                    uint8_t *memSrc,
                    uint32_t sizeToWrite,
                    void* context
                    );

/********* End of Function pointer definitions ***********/


#ifdef __cplusplus
}
#endif

#endif

/**
@}
*/

