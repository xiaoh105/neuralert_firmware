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
#include "FreeRTOS.h"
#include "task.h"

int32_t g_pmCntr;

void CC_PalPowerSaveModeInit(void)
{
    g_pmCntr = 0;
    return;
}

int32_t CC_PalPowerSaveModeStatus(void)
{
    return g_pmCntr;
}

CCError_t CC_PalPowerSaveModeSelect(CCBool isPowerSaveMode)
{
    CCError_t rc = CC_OK;

    switch (isPowerSaveMode){
    case CC_FALSE:
        taskENTER_CRITICAL();
         g_pmCntr++;
        taskEXIT_CRITICAL();
        break;
    case CC_TRUE:
        taskENTER_CRITICAL();
        g_pmCntr--;
        taskEXIT_CRITICAL();
        break;
    default:
        return CC_FAIL;
    }

    if(g_pmCntr == 0){
        /* once the counter is zero,
         * an external callback shall be called to notify the PMU that ARM Cerberus might be powered down. */
    }

    if(g_pmCntr < 0 ){
        /* illegal state - exit with error */
        return CC_FAIL;
    }

    return rc;
}
