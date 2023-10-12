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
#include <stdio.h>
#include <stdarg.h>
#include "cc_pal_types.h"
#include "cc_pal_log.h"
#include "cal.h"

int CC_PAL_logLevel = CC_PAL_MAX_LOG_LEVEL;
uint32_t CC_PAL_logMask = 0xFFFFFFFF;
#ifdef ARM_DSM
#else
#if _CC_PAL_MAX_LOG_LEVEL > CC_PAL_LOG_LEVEL_NULL
void CC_PalLogInit(void)
{
}

void CC_PalLogLevelSet(int setLevel)
{
    CC_PAL_logLevel = setLevel;
}

void CC_PalLogMaskSet(uint32_t setMask)
{
    CC_PAL_logMask = setMask;
}

void CC_PalLog(int level, const char * format, ...)
{
    va_list args;
    CC_UNUSED_PARAM(level);
    va_start( args, format );

   //vprintf( format, args);
    CRYPTO_VPRINTF(format, args);

    va_end(args);
}
#else // _CC_PAL_MAX_LOG_LEVEL > CC_PAL_LOG_LEVEL_NULL
#endif
#endif

