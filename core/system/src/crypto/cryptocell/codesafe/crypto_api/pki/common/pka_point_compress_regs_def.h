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
#ifndef PKA_POINT_COMPRESS_REGS_DEF_H
#define PKA_POINT_COMPRESS_REGS_DEF_H

/*stack*/
#define PKA_REG_X     2
#define PKA_REG_Y     3
#define PKA_REG_EC_A  4
#define PKA_REG_EC_B  5

/*Square root*/
/*in*/
#define PKA_REG_Y1    PKA_REG_Y    //zQ
#define PKA_REG_Y2    PKA_REG_EC_A //zN
/*stack*/
#define PKA_REG_T     6   //zT
#define PKA_REG_Z     7   //zZ
#define PKA_REG_EX    8   //zEx
#define PKA_REG_YT    9   //zYt

/* Jacobi symbol */
/*in*/
#define PKA_REG_A    10   //za
#define PKA_REG_B    11   //zb
/*stack*/
#define PKA_REG_C    12   //zc

#endif
