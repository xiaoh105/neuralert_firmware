/**
 ****************************************************************************************
 *
 * @file sys_exception.c
 *
 * @brief System exception
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

#include "sdk_type.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys_exception.h"
#include "da16200_map.h"
#include "driver.h"
#include "tcb.h"
#include "common_def.h"

/*
******************************************************************************
*	VARIABLES
******************************************************************************
*/

#undef	SUPPORT_OOPS_DUMP

#ifndef	__da16x_oops_h__

#define	OOPS_MARK			0x4F4F5053 /* OOPS */
#define OOPS_LEN_MASK		0x00000FFF
#define OOPS_MOD_SHFT		12
#define	OOPS_STACK_TAG		((UINT32 *)(DA16X_SRAM_BASE|0x00E00))
#define	OOPS_STACK_CTXT		((UINT32 *)(DA16X_SRAM_BASE|0x00E10))
#define	OOPS_STACK_PICS		19
#define	OOPS_STACK_SICS		(7+6)

typedef 	struct _oops_taginfo_	OOPS_TAG_TYPE;

struct	_oops_taginfo_ {
	UINT32	tag;
	UINT16	mark;		// Application Mark
	UINT16	length;		// Dump Length
	UINT32	rtc[2];		// RTC Time Stamp
};

#endif	//__sys_oops_h__

#define EXCEPT_VSIMPRINT(...)	Printf( __VA_ARGS__ )
#define EXCEPT_PRINT(...)		Printf( __VA_ARGS__ )

#ifdef	SUPPORT_ISR_DEBUG
static  void	*sys_exception_current_isr;
static  int	sys_nested_isr_count;
#endif //SUPPORT_ISR_DEBUG

void print_fault_handler(unsigned int *mspfile, unsigned int *pspfile, unsigned int mode);
#if 0
void print_stack_checker(OAL_THREAD_TYPE *thread);
#endif

#if defined(__CHECK_CONTINUOUS_FAULT__) && defined(XIP_CACHE_BOOT)
int	autoRebootStopFlag = 0;

extern void increase_fault_count(void);
extern unsigned char get_fault_count(void);
extern unsigned int get_fault_pc(void);
extern void set_fault_count(char cnt);
extern void set_fault_pc(unsigned int pc);

int check_fault_PC(unsigned int faultPC)
{
	increase_fault_count();

	if ((get_fault_count() >= 5) && (get_fault_pc() == faultPC))
	{
		EXCEPT_PRINT(CYAN_COLOR "\n Stop auto--reboot (Fault_count:%d Fault_PC:0x%x) \n" CLEAR_COLOR,
																get_fault_count(), get_fault_pc());
		set_fault_count(0); 
		set_fault_pc(0);

		return pdTRUE;			// stop auto-reboot
	}

	if (get_fault_pc() != faultPC)
	{
		set_fault_count(1); 	// set first
		set_fault_pc(faultPC);
	}

	EXCEPT_PRINT(RED_COLOR "\n Fault_count:%d Fault_PC:0x%x \n" CLEAR_COLOR, get_fault_count(), get_fault_pc());
	
	return pdFALSE;				// auto-reboot
}
#endif	// __CHECK_CONTINUOUS_FAULT__

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void system_exception_init(void)
{
	_sys_nvic_write( HardFault_IRQn		, (void *)sys_hrd_exception_handler , 0);
	_sys_nvic_write( MemoryManagement_IRQn	, (void *)sys_mpu_exception_handler , 0);
	_sys_nvic_write( BusFault_IRQn		, (void *)sys_bus_exception_handler , 0);
	_sys_nvic_write( UsageFault_IRQn	, (void *)sys_usg_exception_handler , 0);

	//tx_thread_stack_error_notify( &print_stack_checker );		/* F_F_S */
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_ISR_DEBUG

void	push_current_interrupt_routine(void *ptr)
{
	UINT32 currsp = 0;
	TASK_DBG_TRACE((UINT32)ptr);
	CHECK_STACKPOINTER(currsp);
	TASK_DBG_TRACE((UINT32)currsp);
	sys_exception_current_isr = ptr;
	sys_nested_isr_count ++;
}

void	pop_current_interrupt_routine(void)
{
	TASK_DBG_TRACE((UINT32)0xFC495352); // ISR exits
	sys_exception_current_isr = NULL;
	sys_nested_isr_count --;
}

#endif	//SUPPORT_ISR_DEBUG

void	push_current_task_routine(void *entrypoint)
{
	TASK_DBG_TRACE((UINT32)entrypoint);
}

/******************************************************************************
 *  sys_exception_handler( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void sys_hrd_exception_handler(void)
{

	__asm(
	"   tst    lr, #4              \n"
	"   ite    eq                  \n"
	"   mrseq  r0, msp             \n"
	"   mrsne  r0, psp             \n"
	"   push   {r4-r11,lr}         \n"
	"   mov	   r1, sp              \n"
	"   mov	   r2, #0              \n"
	"   bl     print_fault_handler \n"
	);
}

void sys_mpu_exception_handler(void)
{
	__asm(
	"   tst    lr, #4              \n"
	"   ite    eq                  \n"
	"   mrseq  r0, msp             \n"
	"   mrsne  r0, psp             \n"
	"   push   {r4-r11,lr}         \n"
	"   mov	   r1, sp              \n"
	"   mov	   r2, #1              \n"
	"   bl     print_fault_handler \n"
	);
}

void sys_bus_exception_handler(void)
{
	__asm(
	"   tst    lr, #4              \n"
	"   ite    eq                  \n"
	"   mrseq  r0, msp             \n"
	"   mrsne  r0, psp             \n"
	"   push   {r4-r11,lr}         \n"
	"   mov	   r1, sp              \n"
	"   mov	   r2, #2              \n"
	"   bl     print_fault_handler \n"
	);
}

void sys_usg_exception_handler(void)
{
	__asm(
	"   tst    lr, #4              \n"
	"   ite    eq                  \n"
	"   mrseq  r0, msp             \n"
	"   mrsne  r0, psp             \n"
	"   push   {r4-r11,lr}         \n"
	"   mov	   r1, sp              \n"
	"   mov	   r2, #3              \n"
	"   bl     print_fault_handler \n"
	);
}

/******************************************************************************
 *  print_fault_handler( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifdef	PARAM_FOR_ISR_DEBUG
int	param1_for_isr_debug = 0;
int	param2_for_isr_debug = 0;
save_param_for_isr_debug(int param1, int param2)
{
	param1_for_isr_debug = param1;
	param2_for_isr_debug = param2;
}
#endif

void print_watchdog_handler(unsigned int *mspfile, unsigned int *pspfile, unsigned int mode)
{
	DA16X_UNUSED_ARG(mode);

#ifndef	 SUPPORT_OOPS_DUMP
	const char *reg_msg_format = "\t%s :%08x%s";
	const char *regdelimiter[]= {
		"R0 ", "R1 ",
		"R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ",
		"R8 ", "R9 ", "R10", "R11", "R12", "SP ",
		"LR ", "PC ", "PSR", "EXC"
	};
	const char *faultdelimiter[]= {
		"SHCSR", "CFSR ", "HFSR ", "DFSR ", "MMFAR", "BFAR ", "AFSR "
	};
#endif	//SUPPORT_OOPS_DUMP

	UINT32 i, dumpaddr;
	OOPS_TAG_TYPE *oopstag;
	tskTCB		*pTCB;

	oopstag = (OOPS_TAG_TYPE *)OOPS_STACK_TAG;

	oopstag->tag  	 = OOPS_MARK; /*OOPS*/
	oopstag->length  = sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS);

	for(i = 0; i < 4; i++){ // R0-R3
		OOPS_STACK_CTXT[i] = mspfile[i];
	}
	for(i = 4; i < 12; i++){ // R4-R11
		OOPS_STACK_CTXT[i] = pspfile[i-4];
	}

	OOPS_STACK_CTXT[12] = mspfile[4]; // R12
	OOPS_STACK_CTXT[13] = (UINT32)(&(mspfile[8])); // SP
	OOPS_STACK_CTXT[14] = mspfile[5]; // LR
	OOPS_STACK_CTXT[15] = mspfile[6]; // PC
	OOPS_STACK_CTXT[16] = mspfile[7]; // PSR
	OOPS_STACK_CTXT[17] = pspfile[8]; // EXC_RETURN

	EXCEPT_PRINT(CYAN_COLOR "\r\n\r\n [Watch Dog Timer Expired]\r\n" CLEAR_COLOR);

	dumpaddr = OOPS_STACK_CTXT[13];

	for(i=0; i<(48); i++) {
		if ( dumpaddr <= DA16X_SRAM_END ){
			OOPS_STACK_CTXT[OOPS_STACK_PICS+i] = *((UINT32 *)(dumpaddr));
			dumpaddr = dumpaddr + sizeof(UINT32);
		} else {
			OOPS_STACK_CTXT[OOPS_STACK_PICS+i] = 0;
			break;
		}
	}

	OOPS_STACK_CTXT[18] = i;

	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+0] = SCB->SHCSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+1] = SCB->CFSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+2] = SCB->HFSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+3] = SCB->DFSR;

	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+4] = SCB->MMFAR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+5] = SCB->BFAR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+6] = SCB->AFSR;

	// reserved area : OOPS_STACK_SICS
	if ( OOPS_STACK_SICS > 7 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+7] =
			  ((da16x_boot_get_wakeupmode() & 0xFF) << 24)
			| ((SYSGET_SFLASH_INFO()  & 0xFF) << 16)
			| ((SYSGET_RESET_STATUS() & 0xFF) <<  8)
			| ((0 & 0xFF) <<  8) // reserved
			;
	}
	if ( OOPS_STACK_SICS > 8 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+8] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 9 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+9] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 10 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+10] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 11 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+11] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 12 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+12] = SYSGET_SWVERSION(); // SW Version
	}

	pTCB = xTaskGetCurrentTaskHandle();

	if ( pTCB != NULL ) {
		oopstag->length	= sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+32);

		if (pTCB->pcTaskName[0]) {
			DRV_MEMCPY( &(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0]), pTCB->pcTaskName, 16);
		}
		else{
			DRV_MEMSET( &(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0]), 0, 16);
		}

		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+4] = (UINT32)(pTCB->ulRunTimeCounter);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] = (UINT32)(pTCB->pxTopOfStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+6] = (UINT32)(pTCB->pxStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] = (UINT32)(pTCB->pxEndOfStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+8] = (UINT32)(pTCB->pxTopOfStack);

		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+9] = (UINT32)(pTCB->uxBasePriority);

		dumpaddr = (UINT32)(pTCB->pxTopOfStack);
		for (i=0; i<(32); i++) {
			if ( dumpaddr <= DA16X_SRAM_END ){
				OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+i] = *((UINT32 *)(dumpaddr));
				dumpaddr = dumpaddr + sizeof(UINT32);
			}else{
				OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+i] = 0;
				break;
			}
		}

		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10] = i;
	}
	else {
		oopstag->length	= sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS+2);
#ifdef	SUPPORT_ISR_DEBUG
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0] = (UINT32)sys_nested_isr_count;
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+1] = (UINT32)sys_exception_current_isr;
#else	//SUPPORT_ISR_DEBUG
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0] = 0;
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+1] = 0;
#endif	//SUPPORT_ISR_DEBUG
	}

#ifndef	 SUPPORT_OOPS_DUMP
	EXCEPT_PRINT("\r\nRegister-Dump\r\n");
	for(i=0; i < 18; i++){
		if (i == 15) {
			Printf(CYAN_COLOR);
		}
		EXCEPT_PRINT(reg_msg_format
				, regdelimiter[i], OOPS_STACK_CTXT[i]
				, ((i%4==3)? "\r\n": ","));
		if (i == 15) {
			Printf(CLEAR_COLOR);
		}
	}

	EXCEPT_PRINT("\r\nStatus Register\r\n");
	for(i=0; i < 7; i++){
		EXCEPT_PRINT(reg_msg_format
				, faultdelimiter[i], OOPS_STACK_CTXT[OOPS_STACK_PICS+48+i]
				, ((i%3==2)? "\r\n": ","));
	}
	EXCEPT_PRINT("\r\nSysInfo\r\n");
	for( i= 7; i < OOPS_STACK_SICS; i++ ){
		EXCEPT_PRINT("\tSICS[%d] = %08x\r\n"
				, (i-7), OOPS_STACK_CTXT[OOPS_STACK_PICS+48+i]);
	}

#if 0
	EXCEPT_PRINT("\r\nStack-Dump (%d)", OOPS_STACK_CTXT[18]);
	dumpaddr = ((UINT32)&(OOPS_STACK_CTXT[OOPS_STACK_PICS]));

	for(i=0; i<OOPS_STACK_CTXT[18]; i++) {
		if ( dumpaddr <= DA16X_SRAM_END ){
			if ((i % 8) == 0){
				EXCEPT_PRINT("\r\n[0x%08x] : "
					, (OOPS_STACK_CTXT[13] + (i*sizeof(UINT32)))
					);
			}
			EXCEPT_PRINT("%08lX ", *((UINT32 *)dumpaddr) );

			dumpaddr = dumpaddr + sizeof(UINT32);
		}else{
			break;
		}
	}
#endif

	EXCEPT_PRINT("\r\n\r\nCurrent Thread\r\n");
	if (pTCB != NULL) {
		EXCEPT_PRINT(CYAN_COLOR "\t Thread: %s\r\n" CLEAR_COLOR, (char *)(&(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0])) );
		EXCEPT_PRINT("\t stack ptr : %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] );
		EXCEPT_PRINT("\t stack base: %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+6] );
		EXCEPT_PRINT("\t stack end : %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] );
		EXCEPT_PRINT("\t stack high: %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+8] );
		EXCEPT_PRINT("\t max usage : %p\r\n",
			(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] - OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + 1)
			);
		EXCEPT_PRINT("\t suspend   : %08x\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+9] );

		EXCEPT_PRINT("\r\nThread Stack (%d)", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]);

		dumpaddr = ((UINT32)&(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11]));

		for (i=0; i<OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]; i++) {
			if ( dumpaddr <= DA16X_SRAM_END ) {
				if ((i % 8) == 0){
					EXCEPT_PRINT("\r\n[0x%08x] : "
						, (OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + (i*sizeof(UINT32)))
						);
				}
				EXCEPT_PRINT("%08lX ", *((UINT32 *)dumpaddr) );

				dumpaddr = dumpaddr + sizeof(UINT32);
			}
			else {
				break;
			}
		}
	}

#ifdef	SUPPORT_ISR_DEBUG
	if (sys_nested_isr_count > 0) {
		EXCEPT_PRINT("\r\n\r\nLast ISR - nested count:%d\r\n", sys_nested_isr_count);
		if ( sys_exception_current_isr != NULL ){
			EXCEPT_PRINT(RED_COLOR "\t Called function address from ISR: %p\r\n" CLEAR_COLOR, sys_exception_current_isr);
#ifdef	PARAM_FOR_ISR_DEBUG
			EXCEPT_PRINT(RED_COLOR "\t (Param1:0x%x param2:0x%x) \r\n" CLEAR_COLOR, param1_for_isr_debug, param2_for_isr_debug);
#endif
		}
	}
#endif	//SUPPORT_ISR_DEBUG

	EXCEPT_PRINT("\r\n\r\n");
#endif	//SUPPORT_OOPS_DUMP

	return;
}

void print_fault_handler(unsigned int *mspfile, unsigned int *pspfile, unsigned int mode)
{
	const char *exception_title[]={ "Hard", "MPU", "BUS", "Usage", "Stack", "Wdog" };
#ifndef	 SUPPORT_OOPS_DUMP
	const char *reg_msg_format = "\t%s :%08x%s";
	const char *regdelimiter[]= {
		"R0 ", "R1 ",
		"R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ",
		"R8 ", "R9 ", "R10", "R11", "R12", "SP ",
		"LR ", "PC ", "PSR", "EXC"
	};
	const char *faultdelimiter[]= {
		"SHCSR", "CFSR ", "HFSR ", "DFSR ", "MMFAR", "BFAR ", "AFSR "
	};
#endif	//SUPPORT_OOPS_DUMP

	UINT32 i, dumpaddr;
	OOPS_TAG_TYPE *oopstag;
	tskTCB		*pTCB;

	oopstag = (OOPS_TAG_TYPE *)OOPS_STACK_TAG;

	oopstag->tag  	 = OOPS_MARK; /*OOPS*/
	oopstag->length  = sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS);

	for(i = 0; i < 4; i++){ // R0-R3
		OOPS_STACK_CTXT[i] = mspfile[i];
	}
	for(i = 4; i < 12; i++){ // R4-R11
		OOPS_STACK_CTXT[i] = pspfile[i-4];
	}

	OOPS_STACK_CTXT[12] = mspfile[4]; // R12
	OOPS_STACK_CTXT[13] = (UINT32)(&(mspfile[8])); // SP
	OOPS_STACK_CTXT[14] = mspfile[5]; // LR
	OOPS_STACK_CTXT[15] = mspfile[6]; // PC
#if defined(__CHECK_CONTINUOUS_FAULT__) && defined(XIP_CACHE_BOOT)
	autoRebootStopFlag = check_fault_PC(mspfile[6]);
#endif	// __CHECK_CONTINUOUS_FAULT__ 
	OOPS_STACK_CTXT[16] = mspfile[7]; // PSR
	OOPS_STACK_CTXT[17] = pspfile[8]; // EXC_RETURN

	EXCEPT_PRINT(RED_COLOR "\r\n [%s Fault Exception]\r\n" CLEAR_COLOR, exception_title[mode]);

	dumpaddr = OOPS_STACK_CTXT[13];

	for(i=0; i<(48); i++) {
		if ( dumpaddr <= DA16X_SRAM_END ){
			OOPS_STACK_CTXT[OOPS_STACK_PICS+i] = *((UINT32 *)(dumpaddr));
			dumpaddr = dumpaddr + sizeof(UINT32);
		}else{
			OOPS_STACK_CTXT[OOPS_STACK_PICS+i] = 0;
			break;
		}
	}

	OOPS_STACK_CTXT[18] = i;

	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+0] = SCB->SHCSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+1] = SCB->CFSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+2] = SCB->HFSR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+3] = SCB->DFSR;

	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+4] = SCB->MMFAR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+5] = SCB->BFAR;
	OOPS_STACK_CTXT[OOPS_STACK_PICS+48+6] = SCB->AFSR;

	// reserved area : OOPS_STACK_SICS
	if ( OOPS_STACK_SICS > 7 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+7] =
			  ((da16x_boot_get_wakeupmode() & 0xFF) << 24)
			| ((SYSGET_SFLASH_INFO()  & 0xFF) << 16)
			| ((SYSGET_RESET_STATUS() & 0xFF) <<  8)
			| ((0 & 0xFF) <<  8) // reserved
			;
	}
	if ( OOPS_STACK_SICS > 8 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+8] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 9 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+9] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 10 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+10] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 11 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+11] = 0x00; // reserved
	}
	if ( OOPS_STACK_SICS > 12 ){
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+12] = SYSGET_SWVERSION(); // SW Version
	}

	pTCB = xTaskGetCurrentTaskHandle();

	if ( pTCB != NULL ) {
		oopstag->length	= sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+32);

		if (pTCB->pcTaskName[0]) {
			DRV_MEMCPY( &(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0]), pTCB->pcTaskName, 16);
		}
		else{
			DRV_MEMSET( &(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0]), 0, 16);
		}

		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+4] = (UINT32)(pTCB->ulRunTimeCounter);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] = (UINT32)(pTCB->pxTopOfStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+6] = (UINT32)(pTCB->pxStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] = (UINT32)(pTCB->pxEndOfStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+8] = (UINT32)(pTCB->pxTopOfStack);
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+9] = (UINT32)(pTCB->uxBasePriority);

		//dumpaddr = (UINT32)(thread->tx_thread_stack_ptr);
		dumpaddr = (UINT32)(pTCB->pxTopOfStack);
		for(i=0; i<(32); i++) {
			if ( dumpaddr <= DA16X_SRAM_END ){
				OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+i] = *((UINT32 *)(dumpaddr));
				dumpaddr = dumpaddr + sizeof(UINT32);
			}else{
				OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11+i] = 0;
				break;
			}
		}

		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10] = i;
	}
	else
	{
		oopstag->length	= sizeof(UINT32)*(OOPS_STACK_PICS+48+OOPS_STACK_SICS+2);
#ifdef	SUPPORT_ISR_DEBUG
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0] = (UINT32)sys_nested_isr_count;
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+1] = (UINT32)sys_exception_current_isr;
#else	//SUPPORT_ISR_DEBUG
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0] = 0;
		OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+1] = 0;
#endif	//SUPPORT_ISR_DEBUG
	}

#ifndef	 SUPPORT_OOPS_DUMP

	EXCEPT_PRINT("\r\nRegister-Dump\r\n");
	for(i=0; i < 18; i++){
		EXCEPT_PRINT(reg_msg_format
				, regdelimiter[i], OOPS_STACK_CTXT[i]
				, ((i%4==3)? "\r\n": ","));
	}

	EXCEPT_PRINT("\r\nFault Status\r\n");
	for(i=0; i < 7; i++){
		EXCEPT_PRINT(reg_msg_format
				, faultdelimiter[i], OOPS_STACK_CTXT[OOPS_STACK_PICS+48+i]
				, ((i%3==2)? "\r\n": ","));
	}
	EXCEPT_PRINT("\r\nSysInfo\r\n");
	for( i= 7; i < OOPS_STACK_SICS; i++ ){
		EXCEPT_PRINT("\tSICS[%d] = %08x\r\n"
				, (i-7), OOPS_STACK_CTXT[OOPS_STACK_PICS+48+i]);
	}

	EXCEPT_PRINT("\r\nStack-Dump (%d)", OOPS_STACK_CTXT[18]);
	dumpaddr = ((UINT32)&(OOPS_STACK_CTXT[OOPS_STACK_PICS]));

	for(i=0; i<OOPS_STACK_CTXT[18]; i++) {
		if ( dumpaddr <= DA16X_SRAM_END ){
			if ((i % 8) == 0){
				EXCEPT_PRINT("\r\n[0x%08x] : "
					, (OOPS_STACK_CTXT[13] + (i*sizeof(UINT32)))
					);
			}
			EXCEPT_PRINT("%08lX ", *((UINT32 *)dumpaddr) );

			dumpaddr = dumpaddr + sizeof(UINT32);
		}else{
			break;
		}
	}

	EXCEPT_PRINT("\r\n\r\nCurrent Thread\r\n");
	if (pTCB != NULL) {
		EXCEPT_PRINT(CYAN_COLOR "\t Thread: %s\r\n" CLEAR_COLOR, (char *)(&(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+0])) );
		EXCEPT_PRINT("\t stack ptr : %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] );
		EXCEPT_PRINT("\t stack base: %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+6] );
		EXCEPT_PRINT("\t stack end : %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] );
		EXCEPT_PRINT("\t stack high: %p\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+8] );
		EXCEPT_PRINT("\t max usage : %p\r\n",
				(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+7] - OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + 1)
			);
		EXCEPT_PRINT("\t suspend   : %08x\r\n", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+9] );

		EXCEPT_PRINT("\r\nThread Stack (%d)", OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]);

		dumpaddr = ((UINT32)&(OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+11]));

		for (i=0; i<OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+10]; i++) {
			if ( dumpaddr <= DA16X_SRAM_END ) {
				if ((i % 8) == 0){
					EXCEPT_PRINT("\r\n[0x%08x] : "
						, (OOPS_STACK_CTXT[OOPS_STACK_PICS+48+OOPS_STACK_SICS+5] + (i*sizeof(UINT32)))
						);
				}
				EXCEPT_PRINT("%08lX ", *((UINT32 *)dumpaddr) );

				dumpaddr = dumpaddr + sizeof(UINT32);
			}
			else {
				break;
			}
		}
	}

#ifdef	SUPPORT_ISR_DEBUG
	if (sys_nested_isr_count > 0) {
		EXCEPT_PRINT("\r\n\r\nLast ISR - nested count:%d\r\n", sys_nested_isr_count);
		if ( sys_exception_current_isr != NULL ){
			EXCEPT_PRINT(RED_COLOR "\t Called function address from ISR: %p\r\n" CLEAR_COLOR, sys_exception_current_isr);
#ifdef	PARAM_FOR_ISR_DEBUG
			EXCEPT_PRINT(RED_COLOR "\t (Param1:0x%x param2:0x%x) \r\n" CLEAR_COLOR, param1_for_isr_debug, param2_for_isr_debug);
#endif
		}
	}
#endif	//SUPPORT_ISR_DEBUG

	EXCEPT_PRINT("\r\n\r\n");
#endif	//SUPPORT_OOPS_DUMP

	for (i = 0; i < 0x10000; i++) {
		asm volatile (  "nop       \n");
	}

	oopstag->length = (mode<<OOPS_MOD_SHFT) | oopstag->length;

#ifdef	__AUTO_REBOOT_EXCEPTION__
#if defined(__CHECK_CONTINUOUS_FAULT__) && defined(XIP_CACHE_BOOT)
	if (autoRebootStopFlag)
	{
		SYSSET_SWVERSION(0xFFFFC000);		/* only reset */
	}
	else
#endif	// __CHECK_CONTINUOUS_FAULT__
		SYSSET_SWVERSION(0x00000000);		/* reboot */
#else
	//SYSSET_SWVERSION(0xFFFFC0F0);		/* request message dump to MROM */
	SYSSET_SWVERSION(0xFFFFC000);		/* only reset */
#endif

	_sys_tick_close( );
	_sys_nvic_close( );


	da16x_sysdbg_dbgt_backup();

	NVIC_SystemReset();
}

#if	0
/******************************************************************************
 *  print_fault_handler( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void print_stack_checker(OAL_THREAD_TYPE *thread)
{
	if ( thread != NULL ){
		EXCEPT_VSIMPRINT("Stk.Over: %s, %x, %x\n"
			, thread->tx_thread_name
			, thread->tx_thread_stack_highest_ptr
			, thread->tx_thread_stack_start );
	}

	__asm(
		"   tst    lr, #4              \n"
		"   ite    eq                  \n"
		"   mrseq  r0, msp             \n"
		"   mrsne  r0, psp             \n"
		"   push   {r4-r11,lr}         \n"
		"   mov	   r1, sp              \n"
		"   mov	   r2, #4              \n"
		"   bl     print_fault_handler \n"
	);
}
#endif

/* EOF */
