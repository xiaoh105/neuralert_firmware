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


/************* Include Files ****************/
#include "cc_pal_types.h"
#include "cc_pal_mutex.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

/************************ Defines ******************************/

#ifndef NULL
#define NULL ((void *)0)
#endif

/************************ Enums ******************************/

/************************ Typedefs ******************************/

/************************ Global Data ******************************/

/************************ Private Functions ******************************/

/************************ Public Functions ******************************/


/**
 * @brief This function purpose is to create a mutex.
 *
 *
 * @param[out] pMutexId - Pointer to created mutex handle
 *
 * @return returns 0 on success, otherwise indicates failure
 */
CCError_t CC_PalMutexCreate(CC_PalMutex *pMutexId)
{
    *pMutexId = xSemaphoreCreateMutex(); // function returns SemaphoreHandle_t
    if (*pMutexId != NULL)
        return CC_SUCCESS;
    return CC_FAIL;
}


/**
 * @brief This function purpose is to destroy a mutex
 *
 *
 * @param[in] pMutexId - pointer to Mutex handle
 *
 * @return returns 0 on success, otherwise indicates failure
 */
CCError_t CC_PalMutexDestroy(CC_PalMutex *pMutexId)
{
    vSemaphoreDelete(*pMutexId);
    return CC_SUCCESS;
}

/**
 * @brief This function purpose is to Wait for Mutex with aTimeOut. aTimeOut is
 *        specified in milliseconds. (CC_INFINITE is blocking)
 *
 *
 * @param[in] pMutexId - pointer to Mutex handle
 * @param[in] timeOut - timeout in mSec, or CC_INFINITE
 *
 * @return returns 0 on success, otherwise indicates failure
 */
CCError_t CC_PalMutexLock(CC_PalMutex *pMutexId, uint32_t timeOut)
{
    if(!CC_INFINITE)
        timeOut = timeOut / portTICK_PERIOD_MS;
    else
        timeOut = portMAX_DELAY;

    return (xSemaphoreTake(*pMutexId, timeOut) ? CC_SUCCESS : CC_FAIL);
}

/**
 * @brief This function purpose is to release the mutex.
 *
 *
 * @param[in] pMutexId - pointer to Mutex handle
 *
 * @return returns 0 on success, otherwise indicates failure
 */
CCError_t CC_PalMutexUnlock(CC_PalMutex *pMutexId)
{
    return (xSemaphoreGive(*pMutexId) ? CC_SUCCESS : CC_FAIL);
}
