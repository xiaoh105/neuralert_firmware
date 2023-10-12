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
@brief This file contains all the CryptoCell Management APIs, their enums and definitions.

The following terms, used throughout this module, are defined in
<em>Arm Architecture Reference Manual Armv8</em>:
<ul><li>Privileged and unprivileged modes.</li>
<li>Secure and Non-secure modes.</li></ul>

@defgroup cc_management CryptoCell management APIs
@brief Contains CryptoCell Management APIs.

@{
@ingroup cryptocell_api
*/

#ifndef _MBEDTLS_CC_MNG_H
#define _MBEDTLS_CC_MNG_H

/* *********************** Includes ***************************** */
#include "cc_pal_types_plat.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* *********************** Defines ***************************** */
/* LCS. */
/*! Chip manufacturer (CM LCS). */
#define CC_MNG_LCS_CM       0x0
/*! Device manufacturer (DM LCS). */
#define CC_MNG_LCS_DM       0x1
/*! Security enabled (Secure LCS).*/
#define CC_MNG_LCS_SEC_ENABLED  0x5
/*! RMA (RMA LCS). */
#define CC_MNG_LCS_RMA      0x7

/* *********************** Macros ***************************** */


/* *********************** Enums ***************************** */
/*! RMA statuses. */
typedef enum  {
    CC_MNG_NON_RMA          = 0,         /*! Non-RMA: bit [30] = 0, bit [31] = 0. */
    CC_MNG_PENDING_RMA  = 1,         /*! Pending RMA: bit [30] = 1, bit [31] = 0. */
    CC_MNG_ILLEGAL_STATE    = 2,         /*! Illegal state: bit [30] = 0, bit [31] = 1. */
    CC_MNG_RMA          = 3,         /*! RMA: bit [30] = 1, bit [31] = 1. */
}mbedtls_mng_rmastatus;

/*! AES HW key types. */
typedef enum  {
    CC_MNG_HUK_KEY         = 0,         /*! Device root key (HUK). */
    CC_MNG_RTL_KEY         = 1,         /*! Platform key (Krtl). */
    CC_MNG_PROV_KEY        = 2,         /*! ICV provisioning key (Kcp). */
    CC_MNG_CE_KEY          = 3,         /*! OEM code-encryption key (Kce). */
    CC_MNG_ICV_PROV_KEY    = 4,         /*! OEM provisioning key (Kpicv). */
    CC_MNG_ICV_CE_KEY      = 5,         /*! ICV code-encryption key (Kceicv). */
    CC_MNG_TOTAL_HW_KEYS   = 6,         /*! Total number of HW Keys. */
    CC_MNG_END_OF_KEY_TYPE = 0x7FFFFFFF /*! Reserved. */
}mbedtls_mng_keytype;

/************************ Typedefs  ****************************/

/*! Input to the mbedtls_mng_apbc_config_set() function. */
typedef union mbedtls_mng_apbcconfig{
    uint8_t apbcConfigVal; /*!< APB-C configuration values. */
    struct {
        uint8_t isApbcSecAccessMode :1 /*!< APB-C Secure or Non-secure access mode. */;
        uint8_t isApbcSecModeLock   :1 /*!< APB-C Secure or Non-secure access mode lock. */;
        uint8_t isApbcPrivAccessMode:1 /*!< APB-C privileged or unprivileged access mode. */;
        uint8_t isApbcPrivModeLock  :1 /*!< APB-C privileged or unprivileged access mode lock. */;
        uint8_t isApbcInstAccessMode:1 /*!< APB-C instruction access. */;
        uint8_t isApbcInstModeLock  :1 /*!< APB-C instruction access lock. */;
        uint8_t rfu                 :2 /*!< Reserved. */;
    }apbcbits; /*!< APB-C bits. */
}mbedtls_mng_apbcconfig;

/* ****************************************** Public Functions **************************************** */
/*
Management APIs enable to set, get or obtain device status by reading or writing the
appropriate registers or the OTP.
*/
/* ********************************************************************************************** */
/*!
@brief This function reads the OTP word of the OEM flags, and returns the OEM RMA flag status: TRUE or FALSE.

The function returns the value only in DM LCS or Secure LCS.
It validates the device RoT configuration, and returns the value only if both HBK0 and HBK1 are supported.
Otherwise, it returns FALSE regardless to the OTP status.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_pending_rma_status_get(uint32_t *rmaStatus    /*!< [out] The RMA status. */);

/*!
@brief This function verifies and returns the CryptoCell HW version.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_hw_version_get(
                 uint32_t *partNumber,    /*!< [out] The part number. */
                 uint32_t *revision       /*!< [out] The HW revision. */
);

/*!
@brief This function sets CryptoCell to Secure mode.

It is done only while CryptoCell is idle.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_cc_sec_mode_set(
                 CCBool_t isSecAccessMode,           /*!< [in] <ul><li>True: Set CryptoCell to Secure mode.</li>
                                                               <li>False: Set CryptoCell to non-Secure mode.</li></ul> */
                 CCBool_t isSecModeLock              /*!< [in] <ul><li>True: Lock CryptoCell to current mode.</li>
                                                               <li>False: Do not lock CryptoCell to current mode.
                                                               Allows calling this function again.</li></ul> */
);

/*!
@brief This function sets CryptoCell to Privilege mode.

It is done only while CryptoCell is idle.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_cc_priv_mode_set(
                  CCBool_t isPrivAccessMode,       /*!< [in] <ul><li>True: Set CryptoCell to privileged mode.</li>
                                                               <li>False: Set CryptoCell to unprivileged mode.</li></ul> */
                  CCBool_t isPrivModeLock          /*!< [in] <ul><li>True: Lock CryptoCell to current mode.</li>
                                                               <li>False: Do not lock CryptoCell to current mode.
                                                               Allows calling this function again.</li></ul>. */
);

/*!
@brief This function sets the shadow register of one of the
HW Keys when the device is in CM LCS or DM LCS.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_debug_key_set(mbedtls_mng_keytype keyType,    /*!< [in] The HW-key type:<ul><li>HUK</li><li>Kcp</li><li>KCE</li><li>KPICV</li><li>KCEICV</li></ul> */
                   uint32_t *pHwKey,                          /*!< [in] A pointer to the HW-key buffer. */
                   size_t keySize                             /*!< [in] The size of the HW key in Bytes. */

);

/*!
@brief This function retrieves the general configuration from the OTP. See <em>Arm TrustZone CryptoCell-312 Software Integrator's Manual</em>.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_gen_config_get(uint32_t *pOtpWord /*!< [out] The OTP configuration word. */);

/*!
@brief This function locks the usage of either Kcp, Kce, or both during runtime, in either Secure LCS or RMA LCS.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_oem_key_lock(
               CCBool_t kcpLock,          /*!< [in] The flag for locking Kcp usage. */
               CCBool_t kceLock           /*!< [in] The flag for locking Kce usage. */
);

/*!
@brief This function sets the CryptoCell APB-C into one of the following modes:
<ul><li>Secured access mode.</li><li>Privileged access mode.</li><li>Instruction access.</li></ul>.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_apbc_config_set(mbedtls_mng_apbcconfig apbcConfig /*!< APB-C mode.*/ );

/*!
@brief This function requests usage of or releases the APB-C.

@note This function must be called before and after each use of APB-C.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
 */
int mbedtls_mng_apbc_access(CCBool_t isApbcAccessUsed  /*!< [in] <ul><li>TRUE: Request usage of APB-C</li><li>FALSE: Free APB-C.</li></ul> */);

/*!
@brief This function is called once the external PMU decides to power-down CryptoCell.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
*/
int mbedtls_mng_suspend(
            uint8_t *pBackupBuffer,   /*!< [in] A pointer to a buffer that can be used for backup. */
            size_t backupSize         /*!< [in] The size of the backup buffer. Must be at least \c CC_MNG_MIN_BACKUP_SIZE_IN_BYTES. */
);

/*!
@brief This function is called once the external PMU decides to power-up CryptoCell.

@return CC_OK on success.
@return A non-zero value from mbedtls_cc_mng_error.h on failure.
*/
int mbedtls_mng_resume(
               uint8_t *pBackupBuffer,    /*!< [in] A pointer to a buffer that can be used for backup. */
               size_t backupSize          /*!< [in] The size of the backup buffer. Must be at least \c CC_MNG_MIN_BACKUP_SIZE_IN_BYTES. */
);
#ifdef __cplusplus
}
#endif

#endif // _MBEDTLS_CC_MNG_H
/**
@}
 */

