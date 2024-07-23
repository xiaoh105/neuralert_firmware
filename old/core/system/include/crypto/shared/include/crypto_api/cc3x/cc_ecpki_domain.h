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


#ifndef CC_ECPKI_DOMAIN_H
#define CC_ECPKI_DOMAIN_H


/*!
@file
@brief This file defines the ecpki build domain API.
@defgroup cc_ecpki_domain CryptoCell ECC domain APIs
@{
@ingroup cryptocell_ecpki

*/


#include "cc_error.h"
#include "cc_ecpki_types.h"

#ifdef __cplusplus
extern "C"
{
#endif




/**********************************************************************************
 *                CC_EcpkiGetEcDomain function                *
 **********************************************************************************/

/*!
 * @brief  The function returns a pointer to an ECDSA saved domain (one of the supported domains).
 *
 * @return Domain pointer on success.
 * @return NULL on failure.
 */

const CCEcpkiDomain_t *CC_EcpkiGetEcDomain(CCEcpkiDomainID_t domainId /*!< [in] Index of one of the domain Id (must be one of the supported domains). */);

#ifdef __cplusplus
}
#endif

#endif

