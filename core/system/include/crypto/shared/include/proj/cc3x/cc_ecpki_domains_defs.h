/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates).   *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).              *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                       *
*****************************************************************************/

/*!
@defgroup cc_ecc CryptoCell Elliptic Curve APIs
@brief Contains all CryptoCell Elliptic Curve APIs.

@{
@ingroup cryptocell_api
@}
*/

/*!
@defgroup cc_ecpki CryptoCell ECPKI APIs
@brief Contains all CryptoCell ECPKI APIs.

@{
@ingroup cc_ecc
@}
*/


/*!
@file
@brief This file contains CryptoCell ECPKI domains supported by the project.

@defgroup cc_ecpki_domains_defs CryptoCell ECPKI supported domains
@brief Contains CryptoCell ECPKI domains supported by the project.

@{
@ingroup cc_ecpki

*/

#ifndef _CC_ECPKI_DOMAIN_DEFS_H
#define _CC_ECPKI_DOMAIN_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_ecpki_domain_secp192r1.h"
#include "cc_ecpki_domain_secp224r1.h"
#include "cc_ecpki_domain_secp256r1.h"
#include "cc_ecpki_domain_secp521r1.h"
#include "cc_ecpki_domain_secp192k1.h"
#include "cc_ecpki_domain_secp224k1.h"
#include "cc_ecpki_domain_secp256k1.h"
#include "cc_ecpki_domain_secp384r1.h"

/*! Definition of the domain-retrieval function. */
typedef const CCEcpkiDomain_t * (*getDomainFuncP)(void);


#ifdef __cplusplus
}
#endif

#endif

/**
 @}
 */

