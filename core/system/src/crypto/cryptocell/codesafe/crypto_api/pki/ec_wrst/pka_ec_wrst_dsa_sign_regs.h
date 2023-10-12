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
#ifndef PKA_ECDSA_SIGN_REGS_DEF_H
#define PKA_ECDSA_SIGN_REGS_DEF_H

  /* PkaScalarMultSca regs */
  #define EC_SIGN_REG_X2  12
  #define EC_SIGN_REG_Y2  13
  #define EC_SIGN_REG_Z2  14
  #define EC_SIGN_REG_T2  15
  #define EC_SIGN_REG_X4  16
  #define EC_SIGN_REG_Y4  17
  #define EC_SIGN_REG_Z4  18
  #define EC_SIGN_REG_T4  19
  #define EC_SIGN_REG_XS  20
  #define EC_SIGN_REG_YS  21
  #define EC_SIGN_REG_ZS  22
  #define EC_SIGN_REG_TS  23
  #define EC_SIGN_REG_ZP  24
  #define EC_SIGN_REG_TP  25
  #define EC_SIGN_REG_ZR  26
  /* k, p[in/out] */
  #define EC_SIGN_REG_ORD 26 /*=EC_SIGN_REG_ZR, used for EC order*/
  #define EC_SIGN_REG_RK  27 /*scalar*/
  #define EC_SIGN_REG_XP  28 /*in/out*/
  #define EC_SIGN_REG_YP  29 /*in/out*/
#endif
