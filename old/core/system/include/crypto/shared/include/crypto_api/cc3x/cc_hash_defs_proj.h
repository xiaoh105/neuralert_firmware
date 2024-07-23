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
@brief This file contains the project-specific definitions of hash APIs.

@defgroup cc_hash CryptoCell hash APIs
@brief Contains all CryptoCell hash APIs and definitions.

@{
@ingroup cryptocell_api
@}
*/

#ifndef _CC_HASH_DEFS_PROJ_H
#define _CC_HASH_DEFS_PROJ_H

/*!

@defgroup cc_hash_defs_proj CryptoCell hash-API project-specific definitions
@brief Contains the project-specific hash-API definitions.

@{
@ingroup cc_hash
*/
#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/*! The size of the context prototype of the user in words. See ::CCHashUserContext_t. */
#define CC_HASH_USER_CTX_SIZE_IN_WORDS 60


#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

