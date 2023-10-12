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
@brief This file contains functions used for the Boot Services' HAL layer.
*/

#ifndef _CC_HAL_SB_H
#define _CC_HAL_SB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_hal_sb_plat.h"

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/


/*!
@brief This function is used to wait for the Interrupt Request Register (IRR) signal, according to given bits. The existing implementation performs a
"busy wait" on the IRR, and must be adapted to the partner's system.
@return a non-zero value from secureboot_error.h on failure.
*/
CCError_t SB_HalWaitInterrupt(unsigned long hwBaseAddress /*!< [in] CryptoCell HW registers' base address. */,
			      uint32_t data /*!< [in] The interrupt bits to wait for. */);

/*!
@brief This function is used to mask specific interrupt bits.
@return void
*/
void SB_HalMaskInterrupt(unsigned long hwBaseAddress /*!< [in] CryptoCell HW registers' base address. */,
			  uint32_t data /*!< [in] the bits to mask. */);

/*!
@brief This function is used to clear bits in the Interrupt Clear Register (ICR).
@return void
*/
void SB_HalClearInterruptBit(unsigned long hwBaseAddress /*!< [in] CryptoCell base address. */,
			      uint32_t data /*!< [in] The bits to clear. */);


#ifdef __cplusplus
}
#endif

#endif
