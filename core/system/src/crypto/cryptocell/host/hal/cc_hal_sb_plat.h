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

/*!
@file
@brief This file contains definitions that are used for the Boot Services HAL layer.
*/
#ifndef _SECURE_BOOT_STAGE_DEFS_H
#ifndef _CC_HAL_SB_PLAT_H
#define _CC_HAL_SB_PLAT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cal.h"

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*! Reads a 32-bit value from a CryptoCell-312 memory-mapped register. */
#define SB_HAL_READ_REGISTER(addr,val) 	\
		((val) = (*((volatile uint32_t*)(DA16X_ACRYPT_BASE|(addr)))))


/*!
 * Writes a 32-bit value to a CryptoCell-312 memory-mapped register.
 * \note This macro must be modified to make the operation synchronous, that is, the write operation must complete and
 * the new value must be written to the register before the macro returns. The mechanisms required to achieve this
 * are architecture-dependent, for example the memory barrier in ARM architecture.
 */
#define SB_HAL_WRITE_REGISTER(addr,val) 	\
		((*((volatile uint32_t*)(DA16X_ACRYPT_BASE|(addr)))) = (unsigned long)(val))

#ifdef __cplusplus
}
#endif

#endif
#endif	/*_SECURE_BOOT_STAGE_DEFS_H*/
