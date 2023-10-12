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

#ifndef _CC_PAL_MUTEX_PLAT_H
#define _CC_PAL_MUTEX_PLAT_H



#ifdef __cplusplus
extern "C"
{
#endif
#include <pthread.h>

/**
* @brief File Description:
*        This file contains functions for resource management (semaphor operations).
*        The functions implementations are generally just wrappers to different operating system calls.
*        None of the described functions will check the input parameters so the behavior
*        of the APIs in illegal parameters case is dependent on the operating system behavior.
*
*/

typedef pthread_mutex_t CC_PalMutex;



/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/


#ifdef __cplusplus
}
#endif

#endif


