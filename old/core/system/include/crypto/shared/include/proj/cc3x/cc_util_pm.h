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

#ifndef _CC_PM_DEFS_H_
#define _CC_PM_DEFS_H_

/*!
@file
@defgroup cc_pm_defs CryptoCell power management macroes
@brief This file contains power management definitions.
@{
@ingroup cryptocell_api

*/

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_pm.h"

/*! Get ARM Cerberus status. Return number of active registered CC operations */
#define CC_STATUS_GET   CC_PalPowerSaveModeStatus()

/*! Notify ARM Cerberus is active. */
#define CC_IS_WAKE  CC_PalPowerSaveModeSelect(CC_FALSE)

/*! Notify ARM Cerberus is idle. */
#define CC_IS_IDLE  CC_PalPowerSaveModeSelect(CC_TRUE)


#ifdef __cplusplus
}
#endif
/**
@}
 */
#endif /*_CC_PM_DEFS_H_*/
