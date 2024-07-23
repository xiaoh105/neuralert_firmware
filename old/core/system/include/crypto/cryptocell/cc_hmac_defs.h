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

#ifndef _CC_HMAC_DEFS_H
#define _CC_HMAC_DEFS_H

#ifndef USE_MBEDTLS_CRYPTOCELL

#ifdef __cplusplus
extern "C"
{
#endif

/*!
@file
@brief This file contains HMAC definitions.
@defgroup cc_hmac_defs CryptoCell Hmac definitions
@{
@ingroup cc_hmac 

*/ 


/************************ Defines ******************************/
/*! The size of user's context prototype (see ::CC_HmacUserContext_t) in words. */
#define CC_HMAC_USER_CTX_SIZE_IN_WORDS 94


#ifdef __cplusplus
}
#endif

#endif //USE_MBEDTLS_CRYPTOCELL
/** 
@}
 */
#endif
