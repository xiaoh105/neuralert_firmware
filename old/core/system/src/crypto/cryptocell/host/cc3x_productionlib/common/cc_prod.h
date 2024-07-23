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
@brief This file contains all of the enums and definitions that are used for the
ICV and OEM production libraries.

@defgroup prod CryptoCell production-library APIs
@brief Contains CryptoCell production-library APIs.
@{
@ingroup cryptocell_api
*/

#ifndef _PROD_H
#define _PROD_H

/*!
@defgroup prod_mem CryptoCell production-library definitions
@{
@ingroup prod
*/
/************************ Defines ******************************/

/*! The definition of the number of Bytes in a word. */
#define CC_PROD_32BIT_WORD_SIZE    sizeof(uint32_t)
/*! The size of the plain-asset in Bytes. */
#define PROD_ASSET_SIZE     16
/*! The size of the asset-package in Bytes. */
#define PROD_ASSET_PKG_SIZE     64
/*! The size of the asset-package in words. */
#define PROD_ASSET_PKG_WORD_SIZE    (PROD_ASSET_PKG_SIZE/CC_PROD_32BIT_WORD_SIZE)
/*! The number of words of the DCU LOCK. */
#define PROD_DCU_LOCK_WORD_SIZE     4

/************************ Enums ********************************/

/*! The type of the provided asset. */
typedef enum {
    /*! The asset is not provided. */
        ASSET_NO_KEY = 0,
    /*! The asset is provided as plain, not in a package. */
        ASSET_PLAIN_KEY = 1,
    /*! The asset is provided as a package. */
        ASSET_PKG_KEY = 2,
    /*! Reserved. */
        ASSET_TYPE_RESERVED     = 0x7FFFFFFF,
} CCAssetType_t;

/************************ Typedefs  ****************************/

/*! Defines the buffer of the plain asset. A 16-Byte array. */
typedef uint8_t CCPlainAsset_t[PROD_ASSET_SIZE];
/*! Defines the buffer of the asset-package. A 64-Byte array. */
typedef uint32_t CCAssetPkg_t[PROD_ASSET_PKG_WORD_SIZE];


/************************ Structs  ******************************/

/*! @brief The asset buffer.

  If the asset is provided as plain asset, the \p plainAsset field is used.
  Otherwise, the \p pkgAsset field is used.
  */
typedef union {
    /*! Plain asset buffer. */
        CCPlainAsset_t     plainAsset;
    /*! Asset-package buffer. */
        CCAssetPkg_t       pkgAsset;
} CCAssetBuff_t;

/**
@}
*/

#endif  //_PROD_H
/**
@}
*/

