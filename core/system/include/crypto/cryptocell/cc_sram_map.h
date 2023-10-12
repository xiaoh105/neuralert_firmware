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
@brief This file contains internal SRAM mapping definitions.

@defgroup cc_sram_map CryptoCell SRAM mapping APIs
@brief Contains internal SRAM mapping APIs.

@{
@ingroup cryptocell_api
*/

#ifndef _CC_SRAM_MAP_H_
#define _CC_SRAM_MAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*! The base address of the PKA in the PKA SRAM. */
#define CC_SRAM_PKA_BASE_ADDRESS                                0x0
/*! The size of the PKA SRAM in KB. */
#define CC_PKA_SRAM_SIZE_IN_KBYTES                6

/*! The SRAM address of the RND. */
#define CC_SRAM_RND_HW_DMA_ADDRESS                              0x0
/*! Addresses 0K-2KB in SRAM. Reserved for RND operations. */
#define CC_SRAM_RND_MAX_SIZE                                    0x800
/*! The maximal size of SRAM. */
#define CC_SRAM_MAX_SIZE                                        0x1000

#ifdef __cplusplus
}
#endif

#endif /*_CC_SRAM_MAP_H_*/

/**
@}
 */

