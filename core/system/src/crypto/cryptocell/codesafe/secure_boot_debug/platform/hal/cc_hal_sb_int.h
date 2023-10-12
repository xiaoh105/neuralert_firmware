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


#ifndef _CC_HAL_SB_INT_H
#define _CC_HAL_SB_INT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_hal_sb_plat.h"

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
 * @brief This function initiates a descriptor flow of crypto operation.
 *
 * @param[in] hwBaseAddress 	- CryptoCell base address
 *
 * @return none
 */
void SB_HalInit(unsigned long hwBaseAddress);


#ifdef __cplusplus
}
#endif

#endif
