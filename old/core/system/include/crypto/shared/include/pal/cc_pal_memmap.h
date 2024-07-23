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
@brief This file contains functions for memory mapping.

       \note None of the described functions validate the input parameters, so that the behavior
       of the APIs in case of an illegal parameter is dependent on the behavior of the operating system.

@defgroup cc_pal_memmap CryptoCell PAL memory mapping APIs
@brief Contains memory mapping functions.

@{
@ingroup cc_pal
*/

#ifndef _CC_PAL_MEMMAP_H
#define _CC_PAL_MEMMAP_H


#ifdef __cplusplus
extern "C"
{
#endif


#include "cc_pal_types.h"
#include "cc_address_defs.h"


/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
  @brief This function returns the base virtual address that maps the base physical address.

  @return \c 0 on success.
  @return A non-zero value in case of failure.
 */
uint32_t CC_PalMemMap(
        CCDmaAddr_t physicalAddress, /*!< [in] The starting physical address of the I/O range to be mapped. */
        uint32_t mapSize,   /*!< [in] The number of Bytes that were mapped. */
        uint32_t **ppVirtBuffAddr /*!< [out] A pointer to the base virtual address to which the physical pages were mapped. */ );


/*!
  @brief This function unmaps a specified address range that was previously mapped by #CC_PalMemMap.

  @return \c 0 on success.
  @return A non-zero value in case of failure.
 */
uint32_t CC_PalMemUnMap(uint32_t *pVirtBuffAddr, /*!< [in] A pointer to the base virtual address to which the physical pages were mapped. */
                    uint32_t mapSize       /*!< [in] The number of Bytes that were mapped. */);

#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

