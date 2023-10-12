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


/************* Include Files *************************************************/
#include "cc_pal_types.h"
#include "cc_pal_mutex.h"
#include "cc_pal_interrupt_ctrl_plat.h"
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "cc_regs.h"
#include "dx_host.h"
#include "cc_hal.h"

/************* Include Files *************************************************/
#include "cc_hal_plat.h"
#include "dx_reg_base_host.h"
#include "cal.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

/************************ Defines ********************************************/

/************************ Enums **********************************************/

/************************ Typedefs *******************************************/
typedef	unsigned char	uint8_t;
typedef	unsigned long	uint32_t;

/************************ Global Data ****************************************/

/************************ Private Functions **********************************/

/************************ Public Functions ***********************************/
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
			const char *name, uint8_t nameLength, void *args)
{
	if( _sys_nvic_write(irq, (void *)funcPtr, 0x7) != TRUE){
		return CC_FAIL;
	}

	return CC_SUCCESS;
}

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
void CC_PalFreeIrq(uint32_t irq)
{
	_sys_nvic_write(irq, NULL, 0x7);
}

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
CCError_t CC_PalEnableIrq(uint32_t irq)
{
	//NVIC_ClearPendingIRQ(irq);
	NVIC_EnableIRQ((IRQn_Type)irq);

	return CC_SUCCESS;
}

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
CCError_t CC_PalDisableIrq(uint32_t irq)
{
	NVIC_DisableIRQ((IRQn_Type)irq);

	return CC_SUCCESS;
}

/**
 * @brief
 *
 * @param[in]
 *
 * @param[out]
 *
 * @return - CC_SUCCESS for success, CC_FAIL for failure.
 */
CCError_t CC_PalInitIrq(void)
{
    return CC_SUCCESS;
}

/**
 * @brief This function removes the interrupt handler for
 * cryptocell interrupts.
 *
 */
void CC_PalFinishIrq(void)
{
}


/*!
 * Busy wait upon Interrupt Request Register (IRR) signals.
 * This function notifys for any ARM CryptoCell interrupt, it is the caller responsiblity
 * to verify and prompt the expected case interupt source.
 *
 * @param[in] data  - input data for future use
 * \return  CCError_t   - CC_OK upon success
 */
CCError_t CC_PalWaitInterrupt( uint32_t data){
    uint32_t irr = 0;
    CCError_t error = CC_OK;

    /* busy wait upon IRR signal */
    do {
        irr = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, HOST_IRR));
        /* check APB bus error from HOST */
        if( CC_REG_FLD_GET(0, HOST_IRR, AHB_ERR_INT, irr) == CC_TRUE){
            error = CC_FAIL;
            /*set data for clearing bus error*/
            CC_REG_FLD_SET(HOST_RGF, HOST_ICR, AXI_ERR_CLEAR, data , 1);
            break;
        }
    } while (!(irr & data));

    /* clear interrupt */
    CC_HAL_WRITE_REGISTER(CC_REG_OFFSET(HOST_RGF, HOST_ICR), data); // IRR and ICR bit map is the same use data to clear interrupt in ICR

    return error;
}
