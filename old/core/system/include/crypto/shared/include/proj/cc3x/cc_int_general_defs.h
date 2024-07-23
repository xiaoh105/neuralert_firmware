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

#ifndef _CC_INT_GENERAL_DEFS_H
#define _CC_INT_GENERAL_DEFS_H

/*!
@file
@brief This file contains internal general definitions of the CryptoCell runtime SW APIs.
@defgroup cc_general_defs CryptoCell general definitions
@{
@ingroup cryptocell_api

*/

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Macros ******************************/

/* check if fatal error bit is set to ON */
#define CC_IS_FATAL_ERR_ON(rc)\
do {\
        uint32_t regVal = 0;\
    regVal = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, HOST_AO_LOCK_BITS));\
            rc = CC_REG_FLD_GET(0, HOST_AO_LOCK_BITS, HOST_FATAL_ERR, regVal);\
            rc = (rc == 1)?CC_TRUE:CC_FALSE;\
}while(0)


#ifdef __cplusplus
}
#endif
/**
@}
 */
#endif



