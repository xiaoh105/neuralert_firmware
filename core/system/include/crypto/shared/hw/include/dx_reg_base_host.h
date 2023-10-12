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

#ifndef __DX_REG_BASE_HOST_H__
#define __DX_REG_BASE_HOST_H__

#include "FreeRTOSConfig.h"
#include "target.h"

/* Identify platform: ARM MPS2 PLUS */
#define DX_PLAT_MPS2_PLUS 1

#define DX_BASE_CC 		(DA16X_ACRYPT_BASE|0x00000) // Org. 0x50010000
#define DX_BASE_CODE 		(DA16X_ACRYPT_BASE|0x10000) // Org. 0x50030000

#define DX_BASE_ENV_REGS 	(DA16X_ACRYPT_BASE|0x18000) // Org. 0x50028000
#define DX_BASE_ENV_NVM_LOW 	(DA16X_ACRYPT_BASE|0x1A000) // Org. 0x5002A000
#define DX_BASE_ENV_NVM_HI  	(DA16X_ACRYPT_BASE|0x1B000) // Org. 0x5002B000
#define DX_BASE_ENV_PERF_RAM 	(DA16X_ACRYPT_BASE|0x20000) // Org. 0x40009000

#define DX_BASE_HOST_RGF 	0x0UL
#define DX_BASE_CRY_KERNEL     	0x0UL

#define DX_BASE_RNG 		0x0000UL
#endif /*__DX_REG_BASE_HOST_H__*/
