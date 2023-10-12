/**
 * \addtogroup CRYLayer
 * \{
 * \addtogroup CRYPTO_SBOOT	Secure Boot
 * \{
 * \brief SecureBoot APIs
 */

/**
 ****************************************************************************************
 *
 * @file da16x_secureboot.h
 *
 * @brief DA16200 secureboot
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#ifndef __da16x_secureboot_h__
#define __da16x_secureboot_h__

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------

#include "cal.h"

#define	TEST_DEBUG

/******************************************************************************
 *  DA16X Secure Boot & Debug
 ******************************************************************************/
/**
 * \addtogroup CRYPTO_SBOOT_CORE	Secure Boot
 * \{
 * \brief SecureBoot & SecureDebug APIs
 */ 

/**
 * \brief Set Debug Mode
 *
 * \param [in] mode			debug mode
 *
 */
extern void	DA16X_Debug_SecureBoot(UINT32 mode);

/**
 * \brief Get Debug Mode
 *
 * \return	current debug mode
 *
 */
extern UINT32	DA16X_Debug_SecureBoot_Mode(void);

/**
 * \brief Run SecureBoot
 *
 * \param [in] taddress		flash memory offset of the image
 * \param [out] jaddress	entry point address of the bootable image \n
 *							if jaddress == NULL, then CPU will branch into the APP automatically.
 * \param [in] stopproc		callback to safely finish a boot sequence before branching APP
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	DA16X_SecureBoot(UINT32 taddress, UINT32 *jaddress, USR_CALLBACK stopproc);

/**
 * \brief Run SecureDebug for DCU Protection
 *
 * \param [in] fhandler		SFLASH handler
 * \param [in] faddress		flash memory offset of the image
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 * \note 	This function runs internally during POR boot. \n
 *		 	So if you run this function in your app, it won't work.
 *
 */
extern UINT32	DA16X_SecureDebug(HANDLE fhandler, UINT32 faddress);

extern void	DA16X_TestOtp(UINT32 mode);

/**
 * \brief Run SecureDebug for DCU Protection
 *
 * \return	In Secure-Lcs, SocID is returned, otherwise curruent Lcs is returned.
 *
 */
extern UINT32	DA16X_SecureSocID(void);

extern UINT32	DA16X_SecureBoot_Fatal(UINT32 *rcRmaFlag);

/**
 * \}
 */

/******************************************************************************
 *  DA16X CMPU & DMPU
 ******************************************************************************/
/**
 * \addtogroup CRYPTO_SBOOT_PROD	Secure Production
 * \{
 * \brief CMPU & DMPU APIs
 */ 

/**
 * \brief Register CMPU to write HUKey & CMKeys in OTP.
 *
 * \param [in] taddress		data pointer of CMPU
 * \param [out] rflag		TRNG test feature. \n 
 * 							Set '0' to avoid the collision of HUKey between products.
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	DA16X_SecureBoot_CMPU(UINT8 *pCmpuData, UINT32 rflag);

/**
 * \brief Register DMPU to write DMKeys in OTP.
 *
 * \param [in] taddress		data pointer of DMPU
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	DA16X_SecureBoot_DMPU(UINT8 *pDmpuData);

extern void	DA16X_SecureBoot_ColdReset(void);

/**
 * \brief Get the OTP flag used to check whether to run a SecureBoot.
 *
 * \return	TRUE,			SecureBoot locked (enabled)
 * \return	FALSE, 			SecureBoot unlocked (disabled)
 *
 */
extern UINT32	DA16X_SecureBoot_GetLock(void);

/**
 * \brief Set the OTP flag to lock the SecureBoot.
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	DA16X_SecureBoot_SetLock(void);

/**
 * \brief Lock/Unlock the OTP Protection
 *
 * \param [in] mode			lock value (1 : unlock for writing data, 0: lock)
 *
 */
extern void	DA16X_SecureBoot_OTPLock(UINT32 mode);

/**
 * \brief Check if current state is a Secure-Lcs.
 *
 * \param [in] taddress		data pointer of DMPU
 *
 * \return	TRUE,			Secure-Lcs
 * \return	FALSE, 			Not Secure-Lcs
 *
 */
extern UINT32	DA16X_SecureBoot_SecureLCS(void);

extern UINT32	DA16X_SecureBoot_GetLock2(void);
extern UINT32	DA16X_SecureBoot_SetLock2(void);

/**
 * \}
 */

/******************************************************************************
 *  DA16X Secure Asset
 ******************************************************************************/
/**
 * \addtogroup CRYPTO_SBOOT_ASSET	Secure Asset
 * \{
 * \brief Secure Asset APIs
 */ 

/**
 * \brief Decrypt SecureAsset
 *
 * \param [in] Owner			key type (1: CMKey - Kpicv, 2: DMKey - Kcp)
 * \param [in] AssetID			unique id (part of nonce)
 * \param [in] InAssetData		encrypted asset data
 * \param [in] AssetSize		size of InAssetData
 * \param [in] OutAssetData		decrypted asset data
 *
 * \return	Non-Zero,		size of OutAssetData
 * \return	Zero, 			function failed 
 *
 */
extern UINT32	DA16X_Secure_Asset(UINT32 Owner, UINT32 AssetID
                , UINT32 *InAssetData, UINT32 AssetSize, UINT8 *OutAssetData);

/**
 * \}
 */

/******************************************************************************
 *  DA16X Secure Runtime Asset
 ******************************************************************************/
/**
 * \addtogroup CRYPTO_SBOOT_RUNTIME	Runtime Asset
 * \{
 * \brief Secure Runtime Asset APIs
 */ 

/**
 * \brief KEY Type list
 *
 */
typedef enum  {
    ASSET_USER_KEY = 0,		/**< User Key */
    ASSET_ROOT_KEY = 1,		/**< HUK Key */
    ASSET_KCP_KEY = 2,		/**< DMKey - Kcp, asset(product) encryption key */
    ASSET_KCE_KEY = 3,		/**< DMKey - Kcp, code encryption key  */
    ASSET_KPICV_KEY = 4,	/**< CMKey - Kpicv, asset(product) encryption key */	
    ASSET_KCEICV_KEY = 5,	/**< CMKey - Kceicv, code encryption key  */
} AssetKeyType_t;

/**
 * \brief User Key Format
 *
 */
typedef struct {
    UINT8	*pKey;			/**< key data */
    size_t	keySize;		/**< key size */
} AssetUserKeyData_t;

/**
 * \brief Encrypted RunTime Asset Info
 *
 */
typedef struct {
        uint32_t  token;		/**< ID for package provisioning */
        uint32_t  version;		/**< version info */
        uint32_t  assetSize;	/**< size of asset */
} AssetInfoData_t;

/**
 * \brief ID code for RunTime Asset
 *
 */
#define CC_RUNASSET_PROV_TOKEN     0x416E7572UL
/**
 * \brief Version code for RunTime Asset
 *
 */
#define CC_RUNASSET_PROV_VERSION   0x10000UL

/**
 * \brief Encrypt RuntimeAsset
 *
 * \param [in] KeyType			key type (\sa AssetKeyType_t)
 * \param [in] noncetype		method to generate a nonce (0xFFFFFFFF: PRNG, otherwise: TRNG)
 * \param [in] KeyData			User Key data
 * \param [in] AssetID			unique id of RuntimeAsset
 * \param [out] OutAssetData	empty output buffer for storing the encrypted RuntimeAsset
 *
 * \return	Non-Zero,			real size of OutAssetData
 * \return	Zero, 				function failed 
 *
 */
extern INT32 DA16X_Secure_Asset_RuntimePack(AssetKeyType_t KeyType, UINT32 noncetype
		, AssetUserKeyData_t *KeyData, UINT32 AssetID, char *title
		, UINT8 *InAssetData, UINT32 AssetSize, UINT8 *OutAssetPkgData);

/**
 * \brief Decrypt RuntimeAsset
 *
 * \param [in] KeyType			key type (\sa AssetKeyType_t)
 * \param [in] KeyData			User Key data
 * \param [in] AssetID			unique id of RuntimeAsset
 * \param [in] 	InAssetPkgData	encrypted RuntimeAsset
 * \param [in] 	AssetPkgSize	size of the encrypted RuntimeAsset
 * \param [out] OutAssetData	empty output buffer for storing the decrypted RuntimeAsset
 *
 * \return	Non-Zero,		size of OutAssetData
 * \return	Zero, 			function failed 
 *
 */
extern INT32 DA16X_Secure_Asset_RuntimeUnpack(AssetKeyType_t KeyType
		, AssetUserKeyData_t *KeyData, UINT32 AssetID
		, UINT8 *InAssetPkgData, UINT32 AssetPkgSize, UINT8 *OutAssetData);

/**
 * \}
 */

#endif /* __da16x_secureboot_h__ */
/**
 * \}
 * \}
 */

/* EOF */
