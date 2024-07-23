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

/*!
 @file
 @brief This file contains the definitions and APIs for memory-barrier implementation.

       This is a placeholder for platform-specific memory barrier implementation.
       The secure core driver should include a memory barrier, before and after the last word of the descriptor,
       to allow correct order between the words and different descriptors.
 @defgroup cc_pal_barrier CryptoCell PAL memory Barrier APIs
 @brief Contains memory-barrier implementation definitions and APIs.

 @{
 @ingroup cc_pal
*/

#ifndef _CC_PAL_BARRIER_H
#define _CC_PAL_BARRIER_H


/*!
 * This macro is puts the memory barrier after the write operation.
 *
 * @return None
 */

void CC_PalWmb(void);

/*!
 * This macro is puts the memory barrier before the read operation.
 *
 * @return None
 */
void CC_PalRmb(void);

#endif

/**
@}
 */

