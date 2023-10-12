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

#ifndef SECUREBOOT_GENERAL_HWDEFS_H
#define SECUREBOOT_GENERAL_HWDEFS_H

#include "FreeRTOSConfig.h"
#include "target.h"

#include "cc_regs.h"
#include "dx_host.h"
#include "dx_crys_kernel.h"

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/
#define SB_REG_ADDR(base, reg_name) 	(DA16X_ACRYPT_BASE | CC_REG_OFFSET(CRY_KERNEL, reg_name))
#define SB_REG_ADDR_UNIT(base, reg_name, unit) 	(DA16X_ACRYPT_BASE | CC_REG_OFFSET(unit, reg_name))



#ifdef __cplusplus
}
#endif

#endif

