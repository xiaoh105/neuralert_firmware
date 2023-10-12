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
#include "dx_reg_base_host.h"
#include "cc_pal_mutex.h"
#include "cc_pal_mem.h"
#include "cc_pal_abort.h"
#include "cc_pal_pm.h"

#define	PAL_FCI_OPTIMIZE

extern CC_PalMutex CCSymCryptoMutex;
extern CC_PalMutex CCAsymCryptoMutex;
extern CC_PalMutex CCRndCryptoMutex;
extern CC_PalMutex *pCCRndCryptoMutex;
extern CC_PalMutex CCGenVecMutex;
extern CC_PalMutex *pCCGenVecMutex;
extern CC_PalMutex CCApbFilteringRegMutex;

#define PAL_WORKSPACE_MEM_BASE_ADDR	    0
#define PAL_WORKSPACE_MEM_SIZE		    0

/**
 * @brief   PAL layer entry point.
 *          The function initializes customer platform sub components,
 *           such as memory mapping used later by CRYS to get physical contiguous memory.
 *
 *
 * @return Virtual start address of contiguous memory
 */
int CC_PalInit(void)
{
    int rc = 0;

    CC_PalLogInit();

    rc = CC_PalDmaInit(PAL_WORKSPACE_MEM_SIZE, PAL_WORKSPACE_MEM_BASE_ADDR);
    if (rc != 0) {
        return 1;
    }

#ifdef CC_IOT
    /* Initialize power management module */
    CC_PalPowerSaveModeInit();
#endif

#ifdef	PAL_FCI_OPTIMIZE
	CCSymCryptoMutex = NULL;
#else	//PAL_FCI_OPTIMIZE
    /* Initialize mutex that protects shared memory and crypto access */
    rc = CC_PalMutexCreate(&CCSymCryptoMutex);
    if (rc != 0) {
        CC_PalAbort("Fail to create SYM mutex\n");
    }
#endif	//PAL_FCI_OPTIMIZE
    /* Initialize mutex that protects shared memory and crypto access */
    rc = CC_PalMutexCreate(&CCAsymCryptoMutex);
    if (rc != 0) {
        CC_PalAbort("Fail to create ASYM mutex\n");
    }
#ifndef CC_IOT
    /* Initialize mutex that protects shared memory and crypto access */
    rc = CC_PalMutexCreate(&CCRndCryptoMutex);
    if (rc != 0) {
        CC_PalAbort("Fail to create RND mutex\n");
    }
    pCCRndCryptoMutex = &CCRndCryptoMutex;
#else
    pCCRndCryptoMutex = &CCAsymCryptoMutex;

    rc = CC_PalMutexCreate(&CCGenVecMutex);
    if (rc != 0) {
        CC_PalAbort("Fail to create GenVec mutex\n");
    }
    pCCGenVecMutex = &CCGenVecMutex;

    /* Initialize mutex that protects APBC access */
    rc = CC_PalMutexCreate(&CCApbFilteringRegMutex);
    if (rc != 0) {
        CC_PalAbort("Fail to create APBC mutex\n");
    }
#endif

    return 0;
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
        CCError_t err = 0;

        CC_PalDmaTerminate();

#ifdef	PAL_FCI_OPTIMIZE
	CCSymCryptoMutex = NULL;
#else	//PAL_FCI_OPTIMIZE
    err = CC_PalMutexDestroy(&CCSymCryptoMutex);
        if (err != 0){
                CC_PAL_LOG_DEBUG("failed to destroy mutex CCSymCryptoMutex\n");
        }
        CC_PalMemSetZero(&CCSymCryptoMutex, sizeof(CC_PalMutex));
#endif	//PAL_FCI_OPTIMIZE
    err = CC_PalMutexDestroy(&CCAsymCryptoMutex);
        if (err != 0){
                CC_PAL_LOG_DEBUG("failed to destroy mutex CCAsymCryptoMutex\n");
        }
        CC_PalMemSetZero(&CCAsymCryptoMutex, sizeof(CC_PalMutex));

#ifndef CC_IOT
    err = CC_PalMutexDestroy(&CCRndCryptoMutex);
        if (err != 0){
                CC_PAL_LOG_DEBUG("failed to destroy mutex CCRndCryptoMutex\n");
        }
        CC_PalMemSetZero(&CCRndCryptoMutex, sizeof(CC_PalMutex));
#else
        err = CC_PalMutexDestroy(&CCGenVecMutex);
        if (err != 0){
                CC_PAL_LOG_DEBUG("failed to destroy mutex CCGenVecMutex\n");
        }
        CC_PalMemSetZero(&CCGenVecMutex, sizeof(CC_PalMutex));

        err = CC_PalMutexDestroy(&CCApbFilteringRegMutex);
    if (err != 0){
        CC_PAL_LOG_DEBUG("failed to destroy mutex CCApbFilteringRegMutex\n");
    }
    CC_PalMemSetZero(&CCApbFilteringRegMutex, sizeof(CC_PalMutex));
#endif

}

