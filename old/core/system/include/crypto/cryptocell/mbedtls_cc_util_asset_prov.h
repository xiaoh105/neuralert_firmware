/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited or its (affiliates).                                     *
*****************************************************************************/

/*!
 @file mbedtls_cc_util_asset_prov.h
 @brief This file contains CryptoCell runtime-library ICV and OEM asset-provisioning APIs and definitions.

 @defgroup cc_util_asset_prov CryptoCell runtime-library asset-provisioning APIs
 @brief Contains CryptoCell runtime-library ICV and OEM asset-provisioning APIs and definitions.

 @{
 @ingroup cc_util
*/

#ifndef  _MBEDTLS_CC_UTIL_ASSET_PROV_H
#define  _MBEDTLS_CC_UTIL_ASSET_PROV_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "cc_pal_types_plat.h"

/*! The maximal size of an asset package. */
#define CC_ASSET_PROV_MAX_ASSET_PKG_SIZE  560

/*! The type of key used to pack the asset. */
typedef enum {
       /*! ICV: The Kpicv key was used to pack the asset. */
       ASSET_PROV_KEY_TYPE_KPICV = 1,
       /*! OEM: The Kcp key was used to pack the asset. */
       ASSET_PROV_KEY_TYPE_KCP = 2,
       /*! Reserved. */
       ASSET_PROV_KEY_TYPE_RESERVED    = 0x7FFFFFFF,
} CCAssetProvKeyType_t;


/*!
 @brief This API securely provisions ICV or OEM assets to devices, using CryptoCell.

 It takes an encrypted and authenticated asset package produced by the ICV or OEM asset-packaging offline utility
 (using AES-CCM with key derived from Kpicv or Kcp respectively, and the asset identifier), authenticates and decrypts it.
 The decrypted asset data is returned to the caller.

 \note  The function is valid in all LCSes. However, an error is returned if the requested key is locked.

 @return \c CC_UTIL_OK on success.
 @return A non-zero value on failure as defined in cc_util_error.h.
 */
CCError_t mbedtls_util_asset_pkg_unpack(
            CCAssetProvKeyType_t        keyType,          /*!< [in]  The type of key used to pack the asset.*/
            uint32_t                    assetId,         /*!< [in] A 32-bit index identifying the asset, in big-endian order.  */
            uint32_t                    *pAssetPackage,  /*!< [in]  The encrypted and authenticated asset package. */
            size_t                      assetPackageLen, /*!< [in] The length of the asset package. Must not exceed CC_ASSET_PROV_MAX_ASSET_PKG_SIZE. */
            uint8_t                     *pAssetData,     /*!< [out] The buffer for retrieving the decrypted asset data. */
            size_t                      *pAssetDataLen   /*!< [in, out] <ul><li>In: The size of the available asset-data buffer. Maximal size is 512 bytes.</li>
                                                              <li>Out: A pointer to the actual length of the decrypted asset data.</li></ul> */
            );


#ifdef __cplusplus
}
#endif

#endif /*_MBEDTLS_CC_UTIL_ASSET_PROV_H*/

/**
@}
*/

