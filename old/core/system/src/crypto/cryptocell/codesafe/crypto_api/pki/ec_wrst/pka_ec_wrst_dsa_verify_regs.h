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

#ifndef PKA_ECDSA_VERIFY_REGS_DEF_H
#define PKA_ECDSA_VERIFY_REGS_DEF_H

/* Definition of ECDSA Verify specific registers */
#define EC_VERIFY_REG_R     PKA_REG_N
#define EC_VERIFY_REG_NR    PKA_REG_NP
#define EC_VERIFY_REG_F     2
#define EC_VERIFY_REG_D     3
#define EC_VERIFY_REG_H     4
#define EC_VERIFY_REG_TMP   5
#define EC_VERIFY_REG_XPQ 14
#define EC_VERIFY_REG_YPQ 15
#define EC_VERIFY_REG_ZR  16
#define EC_VERIFY_REG_TR  17
#define EC_VERIFY_REG_H1   18
#define EC_VERIFY_REG_H2   19
#define EC_VERIFY_REG_P_GX  20
#define EC_VERIFY_REG_P_GY  21
#define EC_VERIFY_REG_P_WX  22
#define EC_VERIFY_REG_P_WY  23
#define EC_VERIFY_REG_P_RX  24
#define EC_VERIFY_REG_P_RY  25
#define EC_VERIFY_REG_TMP_N  26
#define EC_VERIFY_REG_TMP_NP 27
#define EC_VERIFY_REG_C    28
#endif
