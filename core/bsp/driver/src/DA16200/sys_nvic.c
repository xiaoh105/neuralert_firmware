/**
 ****************************************************************************************
 *
 * @file sys_nvic.c
 *
 * @brief NVIC Driver
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "sys_nvic.h"
#include "driver.h"
#include "common_def.h"
#include "project_config.h"

/******************************************************************************
 *
 *  Local Definitions & Declaration
 *
 ******************************************************************************/

#define	NVIC_INTRSRC_MAX	(16+NVICMax_IRQn)
#define NVIC_SYSTICK_OFF	(16)

/******************************************************************************
 *  _sys_nvic_create ( )
 *
 *  Purpose :   creates console driver
 *  Input   :   baud
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_NOINIT_DATA
static UINT32	*backup_nvic_offset;

/* RAM Vector Table. this table must be aligned with 0x200. */
#define		NVIC_VECSIZE	(NVIC_INTRSRC_MAX)

ATTRIBUTE_NOINIT_DATA
static UINT32	*new_nvic_offset;

ATTRIBUTE_RAM_FUNC
void _sys_nvic_create(void)
{
	backup_nvic_offset = (UINT32 *)(SCB->VTOR);
}

/******************************************************************************
 *  _sys_nvic_init ( )
 *
 *  Purpose :   initializes nvic controller
 *  Input   :   baud
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void _sys_nvic_init(unsigned int *vec_src)
{
	int i;

	/* IRQ is already disabled */
	// uDMA Done Intr & HostSlave Intr Issue
	for( i = 0; i < 8; i++ )
	{
		NVIC->ICER[i] = 0xFFFFFFFFUL; // Disable All Interrupts
		NVIC->ICPR[i] = 0xFFFFFFFFUL; // Clear Pending Interrupts
	}

	new_nvic_offset = ((UINT32 *) (&__region_SYSRAM1_start__));
	DRV_MEMSET(new_nvic_offset, 0, NVIC_VECSIZE*sizeof(UINT32));
	for( i=0 ; i<(NVIC_SYSTICK_OFF) ;i++ ){
		new_nvic_offset[i] = vec_src[i];
	}

	/* Setup Vector Table Offset Register */
	SCB->VTOR = ((uint32_t) new_nvic_offset) & SCB_VTOR_TBLOFF_Msk;

	/* Setup Configuration Control Register   */
	SCB->CCR = 0x00000000UL ;

	for( i=0 ; i<12 ;i++ ){
		SCB->SHP[i] = 0x00;
	}
	/* Setup System Handlers 4-7 Priority Registers   */
	/* Rsrv, UsgF, BusF, MemM  : 0x0000.0000          */
	/* Setup System Handlers 8-11 Priority Registers  */
	/* SVCl, Rsrv, Rsrv, Rsrv  : 0xFF00.0000          */
	SCB->SHP[7] = 0xFF ;
	/* Setup System Handlers 12-15 Priority Registers */
	/* SysT, PnSV, Rsrv, DbgM  : 0x40FF.0000          */
	SCB->SHP[10] = 0xFF ;
	SCB->SHP[11] = 0x40 ;

	/* Enable the cycle count register.  */
	DWT->CTRL = DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk;
}

/******************************************************************************
 *  _sys_nvic_close ( )
 *
 *  Purpose :   closes nvic controller
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
void _sys_nvic_close(void)
{
	int i;

	/* DisableIRQ */
	DISABLE_INTERRUPTS();
	/* Restore old Vector table */
	SCB->VTOR = (uint32_t) (backup_nvic_offset) ;

	for( i = 0; i < 8; i++ )
	{
		NVIC->ICER[i] = 0xFFFFFFFFUL; // Disable All Interrupts
	}
}

/******************************************************************************
 *  _sys_nvic_read ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int	_sys_nvic_read(int index, void *handleroffset)
{
	int vecoff = index + NVIC_SYSTICK_OFF;

	if( vecoff < NVIC_INTRSRC_MAX){
		*((UINT32 *)handleroffset) = new_nvic_offset[vecoff] ;
		return TRUE;
	}
	return FALSE;
}

/******************************************************************************
 *  _sys_nvic_write ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
int	_sys_nvic_write(int index, void *handleroffset, uint32_t priority)
{
	int vecoff = index + NVIC_SYSTICK_OFF;

	if( vecoff < NVIC_INTRSRC_MAX){		//NVIC_INTRSRC_MAX    (16+NVICMax_IRQn)
		/* DisableIRQ */
		if(index >= 0){
			NVIC_DisableIRQ((IRQn_Type)index);
		}
		new_nvic_offset[vecoff] = (UINT32)handleroffset;

		if (index >= 0) {	/* case of USER ISR */
			if (priority < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) {
//RO Issue:		Printf(RED_COLOR " USER ISR must be lower than %d \r\n" CLEAR_COLOR, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);	/* The lower the number, the higher the priority */
				priority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;
			}
			NVIC_SetPriority(index, priority);
		}

		/* EnableIRQ */
		if(index >= 0 && handleroffset != NULL){
			NVIC_ClearPendingIRQ((IRQn_Type)index);
			NVIC_EnableIRQ((IRQn_Type)index);
		}
		return TRUE;
	}
	return FALSE;
}
