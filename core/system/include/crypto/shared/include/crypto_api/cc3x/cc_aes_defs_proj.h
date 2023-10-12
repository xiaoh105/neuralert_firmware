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
@brief This file contains definitions that are used for CryptoCell AES APIs.

@addtogroup cc_aes_defs

@{

*/

#ifndef CC_AES_DEFS_PROJ_H
#define CC_AES_DEFS_PROJ_H

#include "cc_pal_types.h"


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/*! The size of the context prototype of the user in words. See ::CCAesUserContext_t.*/
#define CC_AES_USER_CTX_SIZE_IN_WORDS (4+8+8+4)

/*! The maximal size of the AES key in words. */
#define CC_AES_KEY_MAX_SIZE_IN_WORDS 8
/*! The maximal size of the AES key in bytes. */
#define CC_AES_KEY_MAX_SIZE_IN_BYTES (CC_AES_KEY_MAX_SIZE_IN_WORDS * sizeof(uint32_t))


#ifdef __cplusplus
}
#endif

#endif /* #ifndef CC_AES_DEFS_PROJ_H */
/**
@}
 */

