/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2017-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
@file

@brief This file contains the common definitions of the CryptoCell AES-CCM star APIs.

@defgroup cc_aesccm_star_common Common definitions of the CryptoCell AES-CCM star APIs
@brief Contains the CryptoCell AES-CCM star APIs.

@{
@ingroup cc_aesccm_star

*/

#ifndef _MBEDTLS_CCM_COMMON_H
#define _MBEDTLS_CCM_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! The size of the AES CCM star nonce in Bytes. */
#define MBEDTLS_AESCCM_STAR_NONCE_SIZE_BYTES               13
/*! The size of source address of the AES CCM star in Bytes. */
#define MBEDTLS_AESCCM_STAR_SOURCE_ADDRESS_SIZE_BYTES      8

/*! AES CCM mode: CCM. */
#define MBEDTLS_AESCCM_MODE_CCM             0
/*! AES CCM mode: CCM star. */
#define MBEDTLS_AESCCM_MODE_STAR            1

#ifdef __cplusplus
}
#endif

#endif /* _MBEDTLS_CCM_COMMON_H */

/**
@}
*/

