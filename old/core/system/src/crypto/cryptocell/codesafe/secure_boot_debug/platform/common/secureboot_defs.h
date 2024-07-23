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

/*! @file
@brief This file contains type definitions for the Secure Boot.
@defgroup cc_sb_defs CryptoCell Secure Boot definitions
@brief Contains CryptoCell Secure Boot type definitions.
@{
@ingroup cc_sb
*/

#ifndef _SECURE_BOOT_DEFS_H
#define _SECURE_BOOT_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_crypto_boot_defs.h"
#include "cc_sec_defs.h"


/* General definitions */
/***********************/

/*! Input or output structure to the Secure Boot verification API. */
typedef struct{
    uint32_t otpVersion;                /*!< The NV counter saved in OTP. */
    CCSbPubKeyIndexType_t keyIndex;         /*!< The key hash to retrieve:<ul><li>The 128-bit Hbk0.</li><li>The 128-bit Hbk1.</li><li>The 256-bit Hbk.</li></ul> */
    uint32_t activeMinSwVersionVal;         /*!< The value of the SW version for the certificate-chain. */
    CCHashResult_t pubKeyHash;          /*!< <ul><li>In: The hash of the public key (N||Np), to compare to the public key stored in the certificate.</li>
                                             <li> Out: The hash of the public key (N||Np) stored in the certificate, to be used for verification
                                                  of the public key of the next certificate in the chain.</li></ul> */
    uint32_t initDataFlag;              /*!< The initialization indication. Internal flag. */
}CCSbCertInfo_t;



/*! The size of the data of the SW-image certificate. */
#define SW_REC_SIGNED_DATA_SIZE_IN_BYTES            44  // HS(8W) + load_adddr(1) + maxSize(1) + isCodeEncUsed(1)
/*! The size of the additional-data of the SW-image certificate in Bytes. */
#define SW_REC_NONE_SIGNED_DATA_SIZE_IN_BYTES       8   // storage_addr(1) + ActualSize(1)
/*! The size of the additional-data of the SW-image certificate in words. */
#define SW_REC_NONE_SIGNED_DATA_SIZE_IN_WORDS       SW_REC_NONE_SIGNED_DATA_SIZE_IN_BYTES/CC_32BIT_WORD_SIZE

/*! Indication whether or not to load the SW image to memory. */
#define CC_SW_COMP_NO_MEM_LOAD_INDICATION       0xFFFFFFFFUL


#ifdef __cplusplus
}
#endif

#endif

/**
@}
*/

