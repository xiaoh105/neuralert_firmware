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


#ifndef CC_ECPKI_DOMAIN_SECP384R1_H
#define CC_ECPKI_DOMAIN_SECP384R1_H

/*
 * All the includes that are needed for code using this module to
 * compile correctly should be #included here.
 */
#include "cc_pal_types.h"
#include "cc_ecpki_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 @brief    the function returns the domain pointer
 @return   return domain pointer

*/
const CCEcpkiDomain_t *CC_EcpkiGetSecp384r1DomainP(void);

#ifdef __cplusplus
}
#endif

#endif

