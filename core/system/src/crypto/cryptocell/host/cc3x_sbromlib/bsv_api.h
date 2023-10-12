/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
* 	(C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.	     *
*	    ALL RIGHTS RESERVED						     *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.					     *
*****************************************************************************/


#ifndef _BSV_API_H
#define _BSV_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! @file
@brief This file contains the Boot Services APIs and definitions.
*/

#include "cc_pal_types.h"
#include "cc_sec_defs.h"

#ifndef CC_BSV_CHIP_MANUFACTURE_LCS
/* Life cycle state definitions. */
#define CC_BSV_CHIP_MANUFACTURE_LCS		0x0 /*!< Defines the CM lifecycle state value. */
#define CC_BSV_DEVICE_MANUFACTURE_LCS		0x1 /*!< Defines the DM lifecycle state value. */
#define CC_BSV_SECURE_LCS			0x5 /*!< Defines the Secure lifecycle state value. */
#define CC_BSV_RMA_LCS				0x7 /*!< Defines the RMA lifecycle state value. */
#endif //CC_BSV_CHIP_MANUFACTURE_LCS

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
@brief This function must be the first Arm CryptoCell 3xx SBROM library API called.
It verifies the HW product and version numbers, and initializes the HW.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvInit(
	unsigned long hwBaseAddress 	/*!< [in] CryptoCell HW registers' base address. */
	);

/*!
 * @brief This function retrieves the security lifecycle state from the NVM manager.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvLcsGet(
	unsigned long hwBaseAddress,	/*!< [in] CryptoCell HW registers' base address. */
	uint32_t *pLcs			/*!< [out] Value of the current security lifecycle state. */
	);

/*!
@brief This function retrieves the HW security lifecycle state and performs validity checks.
If LCS is RMA, performs additional initializations (sets the OTP secret keys to fixed value).
\note	Invalid LCS results in an error returned, upon which the partner's code must completely disable the device.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvLcsGetAndInit(
	unsigned long hwBaseAddress,	/*!< [in] CryptoCell HW registers' base address. */
	uint32_t *pLcs 			/*!< [out] Returned lifecycle state. */
	);

/*!
@brief This function is called in RMA LCS to erase one or more of the private keys.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvOTPPrivateKeysErase(
	unsigned long hwBaseAddress, 	/*!< [in] CryptoCell HW registers' base address. */
	CCBool_t isHukErase,		/*!< [in] Huk secret key:
						<ul><li> CC_TRUE: Huk secret is erased.</li>
						<li> CC_FALSE: Huk secret remains unchanged.</li></ul>  */
	CCBool_t isKpicvErase,		/*!< [in] Kpicv secret key:
						<ul><li> CC_TRUE: Kpicv secret is erased.</li>
						<li> CC_FALSE: Kpicv secret remains unchanged.</li></ul>  */
	CCBool_t isKceicvErase,		/*!< [in] Kceicv secret key:
						<ul><li> CC_TRUE: Kceicv secret is erased.</li>
						<li> CC_FALSE: Kceicv secret remains unchanged.</li></ul>  */
	CCBool_t isKcpErase,		/*!< [in] Kcp secret key:
						<ul><li> CC_TRUE: Kcp secret is erased.</li>
						<li> CC_FALSE: Kcp secret remains unchanged.</li></ul>  */
	CCBool_t isKceErase,		/*!< [in] Kce secret key:
						<ul><li> CC_TRUE: Kce secret is erased.</li>
						<li> CC_FALSE: Kce secret remains unchanged.</li></ul>  */
	uint32_t *pStatus		/*!< [out] Returned status word: */
	);

/*!
@brief This function sets the "fatal error" flag in the NVM manager, to disable the use of any HW Keys or security services.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvFatalErrorSet(
	unsigned long hwBaseAddress 	/*!< [in] CryptoCell HW registers' base address. */
	);

/*!
@brief This function permanently sets the RMA lifecycle state per OEM/ICV.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvRMAModeEnable(
	unsigned long hwBaseAddress 	/*!< [in] CryptoCell HW registers' base address. */
	);

/*!
@brief This function derives the device's unique SOC_ID as hashed (Hbk || AES_CMAC (HUK)).
\note	SOC_ID is required for the creation of debug certificates.
        Therefore, the OEM or ICV must provide a method for a developer to discover the SOC_ID
        of a target device without having to first enable debugging.
        One suggested implementation is to have the device ROM code compute the SOC_ID and
	place it in a specific location in the flash memory, where it can be accessed by the developer.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvSocIDCompute(unsigned long hwBaseAddress, 	/*!< [in] CryptoCell HW registers' base address. */
			     CCHashResult_t hashResult	/*!< [out] The derived SOC ID. */
	);


/*!
@brief This function must be called when the user needs to lock one of the ICV keys from further usage.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvICVKeyLock(
	unsigned long hwBaseAddress,	/*!< [in] CryptoCell HW registers' base address. */
	CCBool_t isICVProvisioningKeyLock,	/*!< [in] The ICV provisioning key mode:
						<ul><li> CC_TRUE: Kpicv is locked for further usage.</li>
						<li> CC_FALSE: Kpicv is not locked.</li></ul>  */
	CCBool_t isICVCodeEncKeyLock		/*!< [in] The ICV code encryption key mode:
						<ul><li> CC_TRUE: Kceicv is locked for further usage.</li>
						<li> CC_FALSE: Kceicv is not locked.</li></ul>  */
	);

/*!
@brief This function is called by the ICV code, to disable the OEM code from changing the ICV RMA bit flag.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvICVRMAFlagBitLock(
	unsigned long hwBaseAddress	/*!< [in] CryptoCell HW registers' base address. */
	);


/*!
@brief This API enables the core_clk gating mechanism, which is disabled during power-up.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvCoreClkGatingEnable(
	unsigned long hwBaseAddress	/*!< [in] CryptoCell HW registers' base address. */
	);


/*!
@brief This function controls the APB secure filter,
allowing only secure transactions to access CryptoCell-312 registers.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvSecModeSet(
	unsigned long hwBaseAddress,	/*!< [in] CryptoCell HW registers' base address. */
	CCBool_t isSecAccessMode,	/*!< [in] The APB secure filter mode:
						<ul><li> CC_TRUE: only secure accesses are served.</li>
						<li> CC_FALSE: both secure and non-secure accesses are served.</li></ul>  */
	CCBool_t isSecModeLock		/*!< [in] The APB security lock mode:
						<ul><li> CC_TRUE: secure filter mode is locked for further changes.</li>
						<li> CC_FALSE: secure filter mode is not locked.</li></ul>  */
	);

/*!
@brief This function activates the APB privilege filter,
allowing only secure transactions to access CryptoCell-312 registers.

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
*/
CCError_t CC_BsvPrivModeSet(
	unsigned long hwBaseAddress,	/*!< [in] CryptoCell HW registers' base address. */
	CCBool_t isPrivAccessMode,	/*!< [in] The APB privileged mode:
						<ul><li> CC_TRUE: only privileged accesses are served.</li>
						<li> CC_FALSE: both privileged and non-privileged accesses are served.</li></ul>  */
	CCBool_t isPrivModeLock		/*!< [in] The privileged lock mode:
						<ul><li> CC_TRUE: privileged mode is locked for further changes.</li>
						<li> CC_FALSE: privileged mode is not locked.</li></ul>  */
	);


/*!
@brief This function unpacks the ICV asset packet and returns the asset data

@return CC_OK on success.
@return A non-zero value from bsv_error.h on failure.
 */
CCError_t CC_BsvIcvAssetProvisioningOpen(unsigned long    hwBaseAddress, 	/*!< [in] CryptoCell HW registers' base address. */
				     uint32_t         assetId,	/*!< [in] an asset identifier. */
				     uint32_t         *pAssetPkgBuff,	/*!< [in] an asset package word-array formatted to unpack. */
				     size_t           assetPackageLen,	/*!< [in] an asset package's exact length in bytes. Must be multiple of 16 Bytes. */
				     uint8_t          *pOutAssetData,	/*!< [out] the decrypted contents of asset data. */
				     size_t           *pAssetDataLen	/*!< [in/out] <ul><li>As input: the size of the allocated asset data buffer. Maximal size is 512 Bytes.</li>
						        		 <li>As output: the actual size of the decrypted asset data buffer. Maximal size is 512 Bytes.</li></ul> */
	);

#ifdef __cplusplus
}
#endif

#endif



