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
@brief This file contains functions for resource management (mutex operations).

       These functions are generally implemented as wrappers to different operating-system calls.

       \note None of the described functions validate the input parameters, so that the behavior
       of the APIs in case of an illegal parameter is dependent on the behavior of the operating system.

@defgroup cc_pal_mutex CryptoCell PAL mutex APIs
@brief Contains resource management functions.

@{
@ingroup cc_pal
*/

#ifndef _CC_PAL_MUTEX_H
#define _CC_PAL_MUTEX_H

#include "cc_pal_mutex_plat.h"
#include "cc_pal_types_plat.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
  @brief This function creates a mutex.


  @return \c 0 on success.
  @return A non-zero value on failure.
 */
CCError_t CC_PalMutexCreate(CC_PalMutex *pMutexId /*!< [out] A pointer to the handle of the created mutex. */);


/*!
  @brief This function destroys a mutex.


  @return \c 0 on success.
  @return A non-zero value on failure.
 */
CCError_t CC_PalMutexDestroy(CC_PalMutex *pMutexId /*!< [in] A pointer to handle of the mutex to destroy. */);


/*!
  @brief This function waits for a mutex with \p aTimeOut. \p aTimeOut is
         specified in milliseconds. A value of \p aTimeOut=CC_INFINITE means that the function will not return.

  @return \c 0 on success.
  @return A non-zero value on failure.
 */
CCError_t CC_PalMutexLock(CC_PalMutex *pMutexId, /*!< [in] A pointer to handle of the mutex. */
                uint32_t aTimeOut   /*!< [in] The timeout in mSec, or CC_INFINITE. */);


/*!
  @brief This function releases the mutex.

  @return \c 0 on success.
  @return A non-zero value on failure.
 */
CCError_t CC_PalMutexUnlock(CC_PalMutex *pMutexId/*!< [in] A pointer to the handle of the mutex. */);





#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

