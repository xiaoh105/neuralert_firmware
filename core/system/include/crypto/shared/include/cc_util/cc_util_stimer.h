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

#ifndef  _CC_UTIL_STIMER_H
#define  _CC_UTIL_STIMER_H

/*!
@file
@brief This file contains the functions and definitions for the secure timer module.
*/

#ifdef __cplusplus
extern "C"
{
#endif


#include "cc_util_error.h"
#include "cc_pal_types_plat.h"


#define NSEC_SEC 1000000000
#define CONVERT_CLK_TO_NSEC(clks,hz) ((NSEC_SEC/hz)*(clks))
#define STIMER_COUNTER_BYTE_SIZE    8

/******************************************************************************
*                           DEFINITIONS
******************************************************************************/


typedef struct {
    uint32_t lsbLowResTimer;
    uint32_t msbLowResTimer;
}CCUtilCntr_t;

typedef uint64_t    CCUtilTimeStamp_t;


/*!
 * @brief This function records and retrieves the current time stamp read from the Secure Timer.
 *
 * @return CC_OK on success.
 * @return A non-zero value in case of failure.
 *
 */
CCError_t CC_UtilGetTimeStamp(CCUtilTimeStamp_t *pTimeStamp /*!< [out] Time stamp read from the Secure Timer. */);

/*!
 * @brief This function returns the elapsed time, in nano-seconds, between two recorded time stamps. The first time stamp is assumed to
 *    be the stamp of the interval start, so if timeStamp2 is lower than timeStamp1, negative duration is returned.
 *    The translation to nano-seconds is based on the clock frequency definitions described in cc_secure_clk_defs.h.
 *
 * @return  - Duration between two time stamps in nsec.
 *
 */
int64_t CC_UtilCmpTimeStamp(
        CCUtilTimeStamp_t timeStamp1, /*!< [in] Time stamp of the interval start. */
        CCUtilTimeStamp_t timeStamp2  /*!< [in] Time stamp of the interval end. */);


#ifdef __cplusplus
}
#endif

#endif /*_CC_UTIL_STIMER_H*/
