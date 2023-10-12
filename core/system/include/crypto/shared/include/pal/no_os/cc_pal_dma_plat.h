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

#ifndef _CC_PAL_DMA_PLAT_H
#define _CC_PAL_DMA_PLAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_address_defs.h"

/**
 * @brief   stub function, the function should initialize the DMA mapping of the platform (if needed)
 *
 * @param[in] buffSize - buffer size in Bytes
 * @param[in] physBuffAddr - physical start address of the memory to map
 *
 * @return Virtual start address of contiguous memory
 */
extern uint32_t CC_PalDmaInit(uint32_t  buffSize,    /*!< [in] Buffer size in Bytes. */
                  CCDmaAddr_t  physBuffAddr /*!< [in] Physical start address of the memory to map. */);

/**
 * @brief   free system resources created in PD_PAL_DmaInit()
 *
 *
 * @return void
 */
extern void CC_PalDmaTerminate(void);
#ifdef __cplusplus
}
#endif

#endif


