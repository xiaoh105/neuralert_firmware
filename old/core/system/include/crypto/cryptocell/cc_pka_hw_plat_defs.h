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
@brief This file contains the platform-dependent definitions of the CryptoCell PKA APIs.
@defgroup cc_pka_hw_plat_defs CryptoCell PKA-API platform-dependant types and definitions
@brief Contains the platform-dependent definitions of the CryptoCell PKA APIs.

@{
@ingroup cc_pka

*/

#ifndef _CC_PKA_HW_PLAT_DEFS_H
#define _CC_PKA_HW_PLAT_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_types.h"

/*! The size of the PKA engine word. */
#define CC_PKA_WORD_SIZE_IN_BITS            64
/*! The maximal supported size of modulus in bits. */
#define CC_SRP_MAX_MODULUS_SIZE_IN_BITS       3072
/*! The maximal supported size of modulus in RSA in bits. */
#define CC_RSA_MAX_VALID_KEY_SIZE_VALUE_IN_BITS         4096
/*! The maximal supported size of key-generation in RSA in bits. */
#define CC_RSA_MAX_KEY_GENERATION_HW_SIZE_BITS         3072
/*! The maximal supported size of modulus in RSA in words. */
#define CC_RSA_MAX_VALID_KEY_SIZE_VALUE_IN_WORDS        CC_RSA_MAX_VALID_KEY_SIZE_VALUE_IN_BITS / CC_BITS_IN_32BIT_WORD

/*! The size of the Secure Boot or Secure Debug certificate RSA public modulus key in bits. */
#define SB_CERT_RSA_KEY_SIZE_IN_BITS     3072UL
/*! The size of the Secure Boot or Secure Debug certificate RSA public modulus key in Bytes. */
#define SB_CERT_RSA_KEY_SIZE_IN_BYTES    (SB_CERT_RSA_KEY_SIZE_IN_BITS/CC_BITS_IN_BYTE)
/*! The size of the Secure Boot or Secure Debug certificate RSA public modulus key in words. */
#define SB_CERT_RSA_KEY_SIZE_IN_WORDS    (SB_CERT_RSA_KEY_SIZE_IN_BITS/CC_BITS_IN_32BIT_WORD)

/*! The maximal count of extra bits in PKA operations. */
#define PKA_EXTRA_BITS  8
/*! The number of memory registers in PKA operations. */
#define PKA_MAX_COUNT_OF_PHYS_MEM_REGS  32


#ifdef __cplusplus
}
#endif

#endif //_CC_PKA_HW_PLAT_DEFS_H

/**
@}
 */

