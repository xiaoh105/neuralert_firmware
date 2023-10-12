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

 #ifndef PKA_EC_MONT_GLOB_REGS_DEF_H
 #define PKA_EC_MONT_GLOB_REGS_DEF_H

 /*! Note: Don't change registers ID-s ! */

 /*! Define global PKA registers ID-s used in EC Montgomry operations */
 /* global regs. */
 #define EC_MONT_REG_N     PKA_REG_N  /* EC mod. */
 #define EC_MONT_REG_NP    PKA_REG_NP  /* EC Barr.tag */
 #define EC_MONT_REG_T     2
 #define EC_MONT_REG_T1    3
 #define EC_MONT_REG_T2    4
 #define EC_MONT_REG_N4    5  /* 4*mod */
 #define EC_MONT_REG_A24   6  /* ec parameter (A+2)/4 */
 /*! scalarmult in/out and local regs. */
 #define EC_MONT_REG_RES   7  /* result point */
 #define EC_MONT_REG_X1    8  /* inputt point */
 #define EC_MONT_REG_X2    9
 #define EC_MONT_REG_Z2   10
 #define EC_MONT_REG_X3   11
 #define EC_MONT_REG_Z3   12

 #define EC_MONT_PKA_REGS_USED 13 /* beside 2 PKA temp regs. */
#endif
