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
@brief This file contains the definitions and APIs for power-management implementation.

       This is a placeholder for platform-specific power management implementation.
       The module should be updated whether CryptoCell is active or not,
       to notify the external PMU when it might be powered down.

@defgroup cc_pal_pm CryptoCell PAL power-management APIs
@brief Contains PAL power-management APIs.

@{
@ingroup cc_pal

*/

#ifndef _CC_PAL_PM_H
#define _CC_PAL_PM_H


/*
******** Function pointer definitions **********
*/


/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
 @brief This function initiates an atomic counter.
 @return Void.
 */
void CC_PalPowerSaveModeInit(void);

/*!
 @brief This function returns the number of active registered CryptoCell operations.

 @return The value of the atomic counter.
 */
int32_t CC_PalPowerSaveModeStatus(void);

/*!
@brief This function updates the atomic counter on each call to CryptoCell.

On each call to CryptoCell, the counter is increased. At the end of each operation
the counter is decreased.
Once the counter is zero, an external callback is called.

@return \c 0 on success.
@return A non-zero value on failure.
 */
CCError_t CC_PalPowerSaveModeSelect(CCBool isPowerSaveMode /*!< [in] <ul><li>TRUE: CryptoCell is active.</li><li>FALSE: CryptoCell is idle.</li></ul> */ );


#endif
/**
@}
 */

