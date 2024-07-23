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

#ifndef  _CC_PAL_FIPS_H
#define  _CC_PAL_FIPS_H

/*!
@file
@brief This file contains definitions that are used by the FIPS related APIs. The implementation of these functions
need to be replaced according to the Platform and TEE_OS.
*/

#include "cc_pal_types_plat.h"
#if (!defined CC_SW) && (!defined CC_IOT)
//#include "cc_fips.h"
#endif
#include "cc_fips_defs.h"

#ifdef	CC_SUPPORT_FIPS
/**
 * @brief This function purpose is to get the FIPS state.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsGetState(CCFipsState_t *pFipsState);


/**
 * @brief This function purpose is to get the FIPS Error.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsGetError(CCFipsError_t *pFipsError);


/**
 * @brief This function purpose is to get the FIPS trace.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsGetTrace(CCFipsTrace_t *pFipsTrace);


/**
 * @brief This function purpose is to set the FIPS state.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsSetState(CCFipsState_t fipsState);


/**
 * @brief This function purpose is to set the FIPS error.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsSetError(CCFipsError_t fipsError);


/**
 * @brief This function purpose is to set the FIPS trace.
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsSetTrace(CCFipsTrace_t fipsTrace);


/**
 * @brief This function purpose is to wait for FIPS interrupt.
 *      After GPR0 (==FIPS) interrupt is detected, clear the interrupt in ICR,
 *      and call CC_FipsIrqHandle
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsWaitForReeStatus(void);

/**
 * @brief This function purpose is to stop waiting for REE FIPS interrupt.
 *      since TEE lib is terminating
 *
 *
 * @return Zero on success.
 * @return A non-zero value on failure.
 */
CCError_t CC_PalFipsStopWaitingRee(void);

#endif 	//CC_SUPPORT_FIPS

#endif  // _CC_PAL_FIPS_H
