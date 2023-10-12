/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/


#ifndef _CC_PAL_MEMALLOC_INT_H
#define _CC_PAL_MEMALLOC_INT_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "cal.h"

/**
* @brief File Description:
*        This file contains wrappers for memory operations APIs.
*/


/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
 * @brief This function purpose is to allocate a memory buffer according to aSize.
 *
 *
 * @return The function returns a pointer to allocated buffer or NULL if allocation failed.
 */
void* CC_PalMemMallocPlat(size_t  aSize /*!< [in] Number of bytes to allocate. */);

/*!
 * @brief This function purpose is to reallocate a memory buffer according to aNewSize.
 *        The content of the old buffer is moved to the new location.
 *
 * @return The function returns a pointer to the newly allocated buffer or NULL if allocation failed.
 */
void* CC_PalMemReallocPlat(  void* aBuffer,     /*!< [in] Pointer to allocated buffer. */
                             size_t  aNewSize   /*!< [in] Number of bytes to reallocate. */);

/*!
 * @brief This function purpose is to free allocated buffer.
 *
 *
 * @return void.
 */
void CC_PalMemFreePlat(void* aBuffer /*!< [in] Pointer to allocated buffer.*/);
#ifdef __cplusplus
}
#endif

#endif


