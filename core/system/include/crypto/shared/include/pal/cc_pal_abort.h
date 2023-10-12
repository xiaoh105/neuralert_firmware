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
@brief This file includes all PAL APIs.

@defgroup cc_pal CryptoCell PAL APIs
@brief Groups all PAL APIs and definitions.

@{
@ingroup cryptocell_api
*/

#ifndef _CC_PAL_ABORT_H
#define _CC_PAL_ABORT_H


#include "cc_pal_abort_plat.h"


/*!
@defgroup cc_pal_abort CryptoCell PAL abort operations
@{
@ingroup cc_pal
*/

/*! \brief This function performs the "Abort" operation.

    Must be implemented according to platform and OS.
*/
void CC_PalAbort(const char * exp);

/**
@}
*/

#endif

/**
@}
 */

