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


#define CC_PAL_LOG_CUR_COMPONENT CC_LOG_MASK_CCLIB

#include "cc_pal_types.h"
#include "cc_pal_log.h"
#include "cc_pal_mem.h"
#include "cc_pal_abort.h"
#include "cc_lib.h"
#include "cc_hal.h"
#include "cc_pal_init.h"
#include "cc_pal_mutex.h"
#include "cc_pal_perf.h"
#include "cc_regs.h"
#include "dx_crys_kernel.h"
#include "dx_rng.h"
#include "dx_reg_common.h"
#include "llf_rnd_trng.h"
#include "cc_rng_plat.h"
#include "dx_id_registers.h"
#include "cc_util_pm.h"
#include "dx_nvm.h"
#include "ctr_drbg.h"
#include "entropy.h"
#include "threading.h"
#include "mbedtls_cc_mng_int.h"
#include "mbedtls_cc_mng.h"
#include "cc_rnd_common.h"
#include "cc_int_general_defs.h"

CC_PalMutex CCSymCryptoMutex;
CC_PalMutex CCAsymCryptoMutex;
CC_PalMutex *pCCRndCryptoMutex;
CC_PalMutex CCGenVecMutex;
CC_PalMutex *pCCGenVecMutex;
CC_PalMutex CCApbFilteringRegMutex;

