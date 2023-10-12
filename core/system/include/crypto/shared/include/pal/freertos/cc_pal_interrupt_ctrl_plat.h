/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2017] ARM Limited or its affiliates.      *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef _CC_PAL_INTERRUPTCTRL_PLAT_H
#define _CC_PAL_INTERRUPTCTRL_PLAT_H

#include "FreeRTOS.h"
//#include "InterruptCtrl.h"

#include "cc_pal_types_plat.h"
typedef	 void *IrqHandlerPtr;
/**
 * @brief This function sets one of the handler function pointers that are
 * in handlerFuncPtrArr, according to given index.
 *
 * @param[in]
 * handlerIndx - Irq index.
 * funcPtr - Address of the new handler function.
 *
 * @param[out]
 *
 * @return - CC_SUCCESS for success, CC_FAIL for failure.
 */
CCError_t CC_PalRequestIrq(uint32_t irq, IrqHandlerPtr funcPtr,
            const char *name, uint8_t nameLength, void *args);

/**
 * @brief This function removes an interrupt handler.
 *
 * @param[in]
 * irq - Irq index.
 *
 * @param[out]
 *
 * @return
 */
void CC_PalFreeIrq(uint32_t irq);

/**
 * @brief This function enables an IRQ according to given index.
 *
 * @param[in]
 * irq - Irq index.
 *
 * @param[out]
 *
 * @return - CC_SUCCESS for success, CC_FAIL for failure.
 */
CCError_t CC_PalEnableIrq(uint32_t irq);

/**
 * @brief This function disables an IRQ according to given index.
 *
 * @param[in]
 * irq - Irq index.
 *
 * @param[out]
 *
 * @return - CC_SUCCESS for success, CC_FAIL for failure.
 */
CCError_t CC_PalDisableIrq(uint32_t irq);

/**
 * @brief This function removes the interrupt handler for
 * cryptocell interrupts.
 *
 */
void CC_PalFinishIrq(void);

/* @brief
*
* @param[in]
*
* @param[out]
*
* @return - CC_SUCCESS for success, CC_FAIL for failure.
*/
CCError_t CC_PalInitIrq(void);

/*!
 * Busy wait upon Interrupt Request Register (IRR) signals.
 * This function notifys for any ARM CryptoCell interrupt, it is the caller responsiblity
 * to verify and prompt the expected case interupt source.
 *
 * @param[in] data  - input data for future use
 * \return  CCError_t   - CC_OK upon success
 */
CCError_t CC_PalWaitInterrupt( uint32_t data);


#endif /* _CC_PAL_INTERRUPTCTRL_H */

