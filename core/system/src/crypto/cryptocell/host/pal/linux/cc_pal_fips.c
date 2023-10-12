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

#include "cc_pal_types.h"
#include "cc_pal_fips.h"
#include "cc_pal_mem.h"

CCFipsStateData_t   gStateData = { CC_FIPS_STATE_CRYPTO_APPROVED, CC_TEE_FIPS_ERROR_OK, CC_FIPS_TRACE_NONE };


CCError_t CC_PalFipsWaitForReeStatus(void)
{
    FipsSetReeStatus(CC_TEE_FIPS_REE_STATUS_OK);
    return CC_OK;
}

CCError_t CC_PalFipsStopWaitingRee(void)
{
    return CC_OK;
}

CCError_t CC_PalFipsGetState(CCFipsState_t *pFipsState)
{
    *pFipsState = gStateData.state;

    return CC_OK;
}


CCError_t CC_PalFipsGetError(CCFipsError_t *pFipsError)
{
    *pFipsError = gStateData.error;

    return CC_OK;
}


CCError_t CC_PalFipsGetTrace(CCFipsTrace_t *pFipsTrace)
{
    *pFipsTrace = gStateData.trace;

    return CC_OK;
}

CCError_t CC_PalFipsSetState(CCFipsState_t fipsState)
{
    gStateData.state = fipsState;

    return CC_OK;
}

CCError_t CC_PalFipsSetError(CCFipsError_t fipsError)
{
    gStateData.error = fipsError;

    return CC_OK;
}

CCError_t CC_PalFipsSetTrace(CCFipsTrace_t fipsTrace)
{
    gStateData.trace = (gStateData.trace | fipsTrace);

    return CC_OK;
}

