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

#include "cc_pal_barrier.h"

/* taken from arch/arm/include/asm/barrier.h */



#if defined(__arm64__)
/* This is memmory barrier for ARM64*/

#define dsb(opt)        asm volatile("dsb " #opt : : : "memory")

#elif defined(__arm__)
/* This is memmory barrier for ARM*/

#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")

#else
#error This is a place holder for platform specific memory barrier implementation
#define dsb()
#endif
/* This is a plac holder for L2 cache sync function*/
#define CC_PAL_L2_CACHE_SYNC() do { } while (0)

#if defined(__arm64__)
#define mb()            dsb(sy)
#define rmb()           dsb(ld)
#define wmb()           dsb(st)
#else
#define mb()            do { dsb(); CC_PAL_L2_CACHE_SYNC(); } while (0)
#define rmb()           dsb()
#define wmb()           mb()
#endif



void CC_PalWmb(void)
{
    wmb();
}

void CC_PalRmb(void)
{
    rmb();
}




