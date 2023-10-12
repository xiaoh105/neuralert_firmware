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

#ifndef  _CC_UTIL_H
#define  _CC_UTIL_H


/*!
@file
@brief This file contains CryptoCell Util functions and definitions.
*/


#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_util_defs.h"
#include "cc_util_error.h"
#include "cc_pal_types.h"
#include "cc_rnd_common.h"
#include "cc_ecpki_types.h"

/******************************************************************************
*                           DEFINITIONS
******************************************************************************/


/*****************************************/
/* Endorsement key derivation definitions*/
/*****************************************/
typedef enum {
    CC_UTIL_EK_DomainID_secp256k1 = 1,
    CC_UTIL_EK_DomainID_secp256r1 = 2,
    CC_UTIL_EK_DomainID_Max,
    CC_UTIL_EK_DomainID_Last      = 0x7FFFFFFF,
}CCUtilEkDomainID_t;


#define CC_UTIL_EK_BUFF_MAX_LENGTH 32  // 256 bit modulus and order sizes

typedef  uint8_t  CCUtilEkBuf_t[CC_UTIL_EK_BUFF_MAX_LENGTH];

typedef  struct {
    CCUtilEkBuf_t  PrivKey;
} CCUtilEkPrivkey_t;


typedef  struct {
    CCUtilEkBuf_t  PublKeyX;
    CCUtilEkBuf_t  PublKeyY;
} CCUtilEkPubkey_t;

/*! Required for internal FIPS verification for Endorsement key derivation. */
typedef CCEcpkiKgFipsContext_t CCUtilEkFipsContext_t;

/*! Required for  Endorsement key derivation. */

typedef  struct CCUtilEkTempData_t{
        CCEcpkiUserPrivKey_t  privKeyBuf;
        CCEcpkiUserPublKey_t  publKeyBuf;
        CCEcpkiKgTempData_t   ecpkiKgTempData;
} CCUtilEkTempData_t;

/*!
 * @brief This function computes the device unique endorsement key, as an ECC256 key pair, derived from the device root key (KDR).
 *    Prior to using this ECC key pair with CC ECC APIs, translate the domain ID that was used to create it, to a CryptoCell
 *    domain ID:
 *    <ul><li>CC_UTIL_EK_DomainID_secp256r1 - CC_ECPKI_DomainID_secp256r1.</li>
 *    <li>CC_UTIL_EK_DomainID_secp256k1 - CC_ECPKI_DomainID_secp256k1.</li></ul>
 *
 *
 * @return CC_UTIL_OK on success.
 * @return A non-zero value on failure as defined cc_util_error.h.
 *
 */
CCUtilError_t CC_UtilDeriveEndorsementKey(
            CCUtilEkDomainID_t  domainID,   /*!< [in] Selection of domain ID for the key. The following domain IDs are supported:
                                    <ul><li> CC_UTIL_EK_DomainID_secp256r1 (compliant with [TBBR_C].)</li>
                                    <li> CC_UTIL_EK_DomainID_secp256k1. </li></ul>*/
            CCUtilEkPrivkey_t   *pPrivKey_ptr, /*!< [out] Pointer to the derived private key. To use this private key with CC ECC,
                                    use ::CC_EcpkiPrivKeyBuild (CC domainID, pPrivKey_ptr, sizeof(*pPrivKey_ptr),
                                    UserPrivKey_ptr) to convert to CC ECC private key format. */
            CCUtilEkPubkey_t    *pPublKey_ptr,  /*!< [out] Pointer to the derived public key, in [X||Y] format (X and Y being the point
                                    coordinates). To use this public key with CC ECC:
                                    <ul><li> Concatenate a single byte with value 0x04 (indicating uncompressed
                                        format) with pPublKey_ptr in the following order [0x04||X||Y].</li>
                                    <li> Call ::CC_EcpkiPubKeyBuild (CC domainID, [PC || pPublKey_ptr],
                                        1+sizeof(*pPublKey_ptr), UserPublKey_ptr) to convert to CC_ECC public key
                                        format.</li></ul>*/
              CCUtilEkTempData_t     *pTempDataBuf,  /*!< [in] Temporary buffers for internal use. */
              CCRndContext_t     *pRndContext,    /*!< [in/out] Pointer to the RND context buffer used in case FIPS certification if required
                                    (may be NULL for all other cases). */
              CCUtilEkFipsContext_t  *pEkFipsCtx     /*!< [in]  Pointer to temporary buffer used in case FIPS certification if required
                                    (may be NULL for all other cases). */
    );



/*****************************************/
/*   SESSION key settings definitions    */
/*****************************************/

/*!
 * @brief This function builds a random session key (KSESS), and sets it to the session key registers.
 *        It must be used as early as possible during the boot sequence, but only after the RNG is initialized.
 *
 * \note If this API is called more than once, each subsequent call invalidates any prior session-key-based authentication.
 *       These prior authentications have to be authenticated again. \par
 * \note Whenever the device reconfigures memory buffers previously used for secure content, to become accessible from non-secure context,
 *   ::CC_UtilSetSessionKey must be invoked to set a new session key, and thus invalidate any existing secure key packages.
 *
 * @return CC_UTIL_OK on success.
 * @return A non-zero value on failure as defined cc_util_error.h.
 */
CCUtilError_t CC_UtilSetSessionKey(CCRndContext_t *pRndContext /*!< [in,out] Pointer to the RND context buffer. */);


/*!
 * @brief This function disables the device security. The function does the following:
 *    <ol><li> Sets the security disabled register.</li>
 *    <li>Sets the session key register to 0.</li></ol>
 * \note If ARM SDER register is to be set the user must set the device to temporary Security Disabled by calling
 *   this API prior to setting the register.
 *
 * @return CC_UTIL_OK on success.
 * @return A non-zero value on failure as defined cc_util_error.h.
 */
CCUtilError_t CC_UtilSetSecurityDisable(void);

#ifdef __cplusplus
}
#endif

#endif /*_CC_UTIL_H*/
