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
#include "cc_pal_init.h"
#include "cc_pal_dma_plat.h"
#include "cc_pal_log.h"
#include "cc_pal_mutex.h"
#include "cc_pal_pm.h"

extern CC_PalMutex CCSymCryptoMutex;

/**
 * @brief   PAL layer entry point.
 *          The function initializes customer platform sub components,
 *           such as memory mapping used later by CRYS to get physical contiguous memory.
 *
 *
 * @return Returns a non-zero value in case of failure
 */
int CC_PalInit(void)
{  // IG - need to use palInit of cc_linux for all PALs
        uint32_t rc = CC_OK;

    CC_PalLogInit();

        rc =    CC_PalMutexCreate(&CCSymCryptoMutex);
        if (rc != CC_OK) {
                return rc;
        }

        rc =    CC_PalDmaInit(0, 0);
        if (rc != CC_OK) {
                return rc;
        }

#ifdef CC_IOT
    /* Initialize power management module */
    CC_PalPowerSaveModeInit();
#endif

    return rc;
}


/**
 * @brief   PAL layer entry point.
 *          The function initializes customer platform sub components,
 *           such as memory mapping used later by CRYS to get physical contiguous memory.
 *
 *
 * @return None
 */
void CC_PalTerminate(void)
{
         CC_PalMutexDestroy(&CCSymCryptoMutex);
         CC_PalDmaTerminate();
}

