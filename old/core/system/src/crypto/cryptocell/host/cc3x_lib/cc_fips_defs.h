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

#ifndef  _CC_FIPS_DEFS_H
#define  _CC_FIPS_DEFS_H

//empty macro since FIPS not supported
#define CHECK_AND_RETURN_ERR_UPON_FIPS_ERROR()
#define CHECK_AND_RETURN_UPON_FIPS_ERROR()
#define CHECK_AND_RETURN_UPON_FIPS_STATE()
#define CHECK_FIPS_SUPPORTED(supported) {supported = false;}
#define FIPS_RSA_VALIDATE(rndContext_ptr,pCcUserPrivKey,pCcUserPubKey,pFipsCtx)  (CC_OK)
#define FIPS_ECC_VALIDATE(pRndContext, pUserPrivKey, pUserPublKey, pFipsCtx)  (CC_OK)
#define CC_FIPS_SET_RND_CONT_ERR()

#endif  // _CC_FIPS_DEFS_H

