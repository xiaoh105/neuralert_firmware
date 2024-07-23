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
@brief This file contains general definitions for CryptoCell APIs.
@defgroup cc_general_defs CryptoCell general definitions
@brief Contains general CryptoCell definitions.

@{
@ingroup cryptocell_api

*/

#ifndef _CC_ADDRESS_DEFS_H
#define _CC_ADDRESS_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/* Address types within CryptoCell. */
/*! The SRAM address type. */
typedef uint32_t CCSramAddr_t;
/*! The DMA address type. */
typedef uint32_t CCDmaAddr_t;

#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

