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
@brief This file contains the PAL layer entry point.

It includes the definitions and APIs for PAL initialization and termination.

@defgroup cc_pal_init CryptoCell PAL entry or exit point APIs
@brief Contains PAL initialization and termination APIs.

@{
@ingroup cc_pal
*/
#ifndef _CC_PAL_INIT_H
#define _CC_PAL_INIT_H

#include "cc_pal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief This function performs all initializations that may be required by your PAL implementation, specifically by the DMA-able buffer
 * scheme.
 *
 * The existing implementation allocates a contiguous memory pool that is later used by the CryptoCell implementation.
 * If no initializations are needed in your environment, the function can be minimized to return OK.
 * It is called by ::CC_LibInit.
 *
 * @return A non-zero value in case of failure.
 */
int CC_PalInit(void);



/**
 * @brief This function terminates the PAL implementation and frees the resources that were allocated by ::CC_PalInit.
 *
 * @return Void.
 */
void CC_PalTerminate(void);



#ifdef __cplusplus
}
#endif

#endif
/**
@}
 */

