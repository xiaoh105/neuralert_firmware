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

#include "cc_pal_mem.h"
#include "cc_pal_mutex.h"
#include "cal.h"
/************************ Defines ******************************/
#define	TEST_FCI_OPTIMIZE
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
        *pMutexId = CC_PalMemMalloc(sizeof(OAL_SEMAPHORE));
	if (*pMutexId != NULL){

		if( OAL_CREATE_SEMAPHORE( (OAL_SEMAPHORE *)(*pMutexId), "cc.mut", 1, OAL_SUSPEND)
					== OAL_SUCCESS ){
			return CC_SUCCESS;
		}
		CC_PalMemFree(*pMutexId);
		*pMutexId = NULL;
        }
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
	if( *pMutexId != NULL ){
		OAL_DELETE_SEMAPHORE(*pMutexId);
		CC_PalMemFree(*pMutexId);
		*pMutexId = NULL;
	}
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
CCError_t CC_PalMutexLock (CC_PalMutex *pMutexId, uint32_t timeOut)
{
#ifdef	TEST_FCI_OPTIMIZE
	if( *pMutexId == NULL ){
		return CC_SUCCESS;
	}
#endif	//TEST_FCI_OPTIMIZE
	if(!CC_INFINITE)
		timeOut = timeOut / 10; // 10 msec tick
	else
		timeOut = OAL_SUSPEND;

	if( OAL_OBTAIN_SEMAPHORE( (OAL_SEMAPHORE *)(*pMutexId), timeOut ) == OAL_SUCCESS ){
	        return( CC_SUCCESS );
	}
	return CC_FAIL;
}



/**
 * @brief This function purpose is to release the mutex.
 *
 *
 * @param[in] pMutexId - pointer to Mutex handle
 *
 * @return returns 0 on success, otherwise indicates failure
 */
CCError_t CC_PalMutexUnlock (CC_PalMutex *pMutexId)
{
#ifdef	TEST_FCI_OPTIMIZE
	if( *pMutexId == NULL ){
		return CC_SUCCESS;
	}
#endif	//TEST_FCI_OPTIMIZE

	if( OAL_RELEASE_SEMAPHORE( (OAL_SEMAPHORE *)(*pMutexId) ) != OAL_SUCCESS ){
	        return( CC_FAIL );
	}
	return CC_SUCCESS;
}
