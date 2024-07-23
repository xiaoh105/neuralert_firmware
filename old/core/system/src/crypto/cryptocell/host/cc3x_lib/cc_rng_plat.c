/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2018] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

/************* Include Files ****************/
#include "dx_rng.h"
#include "cc_pal_mem.h"
#include "cc_pal_mutex.h"
#include "cc_rng_plat.h"
#include "cc_pal_abort.h"
#include "dx_crys_kernel.h"
#include "cc_common_math.h"
#include "cc_rnd_local.h"
#include "cc_rnd_error.h"
#include "llf_rnd.h"
#include "llf_rnd_error.h"
#include "bypass_driver.h"
#include "cc_rng_params.h"
#include "cc_address_defs.h"
#include "cc_util_pm.h"
#include "llf_rnd_trng.h"

#define	TRNG_FCI_CUSTOMIZE
#ifdef	TRNG_FCI_CUSTOMIZE
#include "da16200_map.h"
#endif	//TRNG_FCI_CUSTOMIZE

extern CC_PalMutex CCSymCryptoMutex;

/****************************************************************************************/
/**
 *
 * @brief The function retrievess the TRNG parameters, provided by the User trough NVM,
 *        and sets them into structures given by pointers pRndContext and pTrngParams.
 *
 * @author reuvenl (6/26/2012)
 *
 * @param[out] pRndState - The pointer to structure, containing PRNG data and
 *                            parameters.
 * @param[out] pTrngParams - The pointer to structure, containing parameters
 *                            of HW TRNG.
 *
 * @return CCError_t - no return value
 */
CCError_t RNG_PLAT_SetUserRngParameters(
        CCRndParams_t  *pTrngParams)
{
        CCError_t  error = CC_OK;
        size_t  paramsSize = sizeof(CC_PalTrngParams_t);

        /* FUNCTION LOGIC */
        error = CC_PalTrngParamGet(&pTrngParams->userParams, &paramsSize);
        if (error != CC_OK) {
            return error;
        }
        // Verify PAL and run-time lib compiled with the same CC_CONFIG_TRNG_MODE
        if (paramsSize != sizeof(CC_PalTrngParams_t)) {
            error = CC_RND_MODE_MISMATCH_ERROR;
            goto func_error;
        }

#ifdef	TRNG_FCI_CUSTOMIZE
	{
		ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC31201));
		ASIC_DBG_TRIGGER(pTrngParams->userParams.SubSamplingRatio1);
		ASIC_DBG_TRIGGER(pTrngParams->userParams.SubSamplingRatio2);
		ASIC_DBG_TRIGGER(pTrngParams->userParams.SubSamplingRatio3);
		ASIC_DBG_TRIGGER(pTrngParams->userParams.SubSamplingRatio4);
	}
#endif	//TRNG_FCI_CUSTOMIZE

        /* Set TRNG parameters         */
        /*-----------------------------*/
        pTrngParams->TrngMode = CC_RND_FE;

        /* Allowed ROSCs lengths b'0-3. If bit value 1 - appropriate ROSC is allowed. */
        pTrngParams->RoscsAllowed = (((pTrngParams->userParams.SubSamplingRatio1 > 0) ? 0x1 : 0x0) |
                ((pTrngParams->userParams.SubSamplingRatio2 > 0) ? 0x2 : 0x0) |
                ((pTrngParams->userParams.SubSamplingRatio3 > 0) ? 0x4 : 0x0) |
                ((pTrngParams->userParams.SubSamplingRatio4 > 0) ? 0x8 : 0x0));
        pTrngParams->SubSamplingRatio = 0;
        if (pTrngParams->RoscsAllowed == 0) {
            error = CC_RND_STATE_VALIDATION_TAG_ERROR;
            goto func_error;
        }

        return CC_OK;
func_error:
        CC_PalMemSetZero(pTrngParams, sizeof(CC_PalTrngParams_t));
        return error;



} /* End of RNG_PLAT_SetUserRngParameters */


