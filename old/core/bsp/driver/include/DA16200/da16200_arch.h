/**
 ****************************************************************************************
 *
 * @file da16200_arch.h
 *
 * @brief DA16200 SW specific
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

#include	"FreeRTOSConfig.h"

#ifndef __da16200_arch_h__
#define __da16200_arch_h__

#define	SUPPORT_ISR_DEBUG
#undef	SUPPORT_MEM_PROFILER

/******************************************************************************
 *  Predefined Symbols
 ******************************************************************************/

#define BUILDTOOL_ID()			__ICCARM__
#define	BUILDTOOL_VER()			__VER__

/******************************************************************************
 *  Pragma
 ******************************************************************************/

#ifdef __GNUC__

#define	DATA_ALIGN(x)		__attribute__((aligned(x)))

#define ATTRIBUTE_RAM_FUNC __attribute__ ((section(".ram_text")))
#define ATTRIBUTE_RAM_RODATA __attribute__ ((section(".ram_rodata")))
#define ATTRIBUTE_NOINIT_DATA __attribute__ ((section(".noinit")))

#else  //__GNUC__
// IAR compiler
#define	_QUOTEME(a)		#a
#define	DATA_ALIGN(x)		_Pragma(_QUOTEME(data_alignment=x))

// IAR does not use these attributes.
#define ATTRIBUTE_RAM_FUNC 
#define ATTRIBUTE_RAM_RODATA
#define ATTRIBUTE_NOINIT_DATA

#endif //__GNUC__

/******************************************************************************
 *  Exported ICF Symbols
 ******************************************************************************/

extern unsigned int  __region_ROM_start__;
extern unsigned int  __region_ROM_end__;
extern unsigned int  __region_RAM_start__;
extern unsigned int  __region_RAM_end__;
extern unsigned int  __region_SYSRAM0_start__;
extern unsigned int  __region_SYSRAM0_end__;
extern unsigned int  __region_SYSRAM1_start__;
extern unsigned int  __region_SYSRAM1_end__;
extern unsigned int  __region_FREE_start__;
extern unsigned int  __region_FREE_end__;
extern unsigned int  __region_STACK_start__;
extern unsigned int  __region_STACK_end__;

extern unsigned int  __region_RAM0_start__;
extern unsigned int  __region_RAM0_end__;

extern unsigned int  __region_RAMIPD_start__;	/* TIM & RTOS only */
extern unsigned int  __region_RAMIPD_end__;	/* TIM & RTOS only */

extern unsigned int CSTACK$$Base;
extern unsigned int CSTACK$$Limit;
extern unsigned int FREE_MEM$$Base;
extern unsigned int FREE_MEM$$Limit;
extern unsigned int Region$$Table$$Base;
extern unsigned int Region$$Table$$Limit;
extern unsigned int HEAP$$Base;
extern unsigned int HEAP$$Limit;

extern void   _tx_thread_context_save(void);
extern void   _tx_thread_context_restore(void);

/******************************************************************************
 *  ROM/RAM Base Address
 ******************************************************************************/

/* name chg by_sjlee*/
#define	ROM_MEM_BASE()						\
	((UINT32)(&__region_ROM_start__))
#define	ROM_MEM_SIZE() 						\
	(((UINT32)(&__region_ROM_end__) 		\
	- (UINT32)(&__region_ROM_start__))+1)

#define	RAM_MEM_BASE()						\
	((UINT32)(&__region_RAM_start__))
#define	RAM_MEM_SIZE() 						\
	(((UINT32)(&__region_RAM_end__) 		\
	- (UINT32)(&__region_RAM_start__))+1)

#define	SYSRAM_MEM_BASE()					\
	((UINT32)(&__region_SYSRAM0_start__))
#define	SYSRAM_MEM_SIZE() 					\
	(((UINT32)(&__region_SYSRAM1_end__) 		\
	- (UINT32)(&__region_SYSRAM0_start__))+1)

/******************************************************************************
 *  Free Memory Section
 ******************************************************************************/

#define	FREE_MEM_BASE() 					\
	((UINT32)(&__region_FREE_start__))
#define	FREE_MEM_END() 						\
	((UINT32)(&__region_FREE_end__))
#define	FREE_MEM_SIZE() 					\
	(((UINT32)(&__region_FREE_end__) 		\
	- (UINT32)(&__region_FREE_start__))+1)

#define	STACK_MEM_BASE() 					\
	((UINT32)(&__region_STACK_start__))
#define	STACK_MEM_END() 					\
	((UINT32)(&__region_STACK_end__))
#define	STACK_MEM_SIZE() 					\
	(((UINT32)(&__region_STACK_end__) 		\
	- (UINT32)(&__region_STACK_start__))+1)

#define	HEAP_MEM_SIZE() 					\
	(((UINT32)(&HEAP$$Limit) 		        	\
	- (UINT32)(&HEAP$$Base)))

/******************************************************************************
 *  Special Memory Section : TIM & RTOS only
 ******************************************************************************/

#define	RAMIPD_MEM_BASE() 					\
	((UINT32)(&__region_RAMIPD_start__))
#define	RAMIPD_MEM_END() 					\
	((UINT32)(&__region_RAMIPD_end__))
#define	RAMIPD_MEM_SIZE() 					\
	(((UINT32)(&__region_RAMIPD_end__) 		\
	- (UINT32)(&__region_RAMIPD_start__))+1)

/******************************************************************************
 *  Macro Functions
 ******************************************************************************/

extern void push_current_interrupt_routine(void *isr);
extern void pop_current_interrupt_routine(void);

#define CHECK_STACKPOINTER(x)	asm volatile ( "mov %0, SP" : : "r" (x)  )

#define DISABLE_INTERRUPTS()	asm volatile ( "cpsid i" )
#define ENABLE_INTERRUPTS()	asm volatile ( "cpsie i" )

#define CHECK_INTERRUPTS(x)	asm volatile ( "mrs %0, PRIMASK" : "=r" (x) : )
#define PREVENT_INTERRUPTS(x)	asm volatile ( "msr PRIMASK, %0" : : "r" (x)  )


#define	INTR_CNTXT_SAVE()

#ifdef	SUPPORT_ISR_DEBUG

#define INTR_CNTXT_CALL(x)	{					\
	                push_current_interrupt_routine((void *)x );	\
			asm volatile ("BL " # x );			\
	                pop_current_interrupt_routine( );		\
			asm volatile ("LDR R0, [SP]");			\
			asm volatile ("STR R0, [SP, #8]");		\
		}

#define INTR_CNTXT_CALL_PARA(x,p)	{				\
	                push_current_interrupt_routine((void *)x );	\
			asm volatile ("MOV R0, %0" : : "i"(p));		\
			asm volatile ("BL " # x );			\
	                pop_current_interrupt_routine( );		\
			asm volatile ("LDR R0, [SP]");			\
			asm volatile ("STR R0, [SP, #8]");		\
		}

#else	//SUPPORT_ISR_DEBUG

#define INTR_CNTXT_CALL(x)			x()
#define INTR_CNTXT_CALL_PARA(x,p)	x(p)

#endif	//SUPPORT_ISR_DEBUG

#define	INTR_CNTXT_RESTORE()

#define ASM_BRANCH(x)		{					\
			asm volatile ( "mov pc, %0\n"			\
					"nop       \n"			\
					"nop       \n"			\
					"nop       \n"			\
				: "=r" (x) : );				\
		}

#define NOP_EXECUTION()		{					\
			asm volatile (	"nop       \n");		\
		}

#define	WAIT_FOR_INTR()		{					\
			asm volatile ( "dsb       \n");			\
			asm volatile ( "wfi       \n");			\
			asm volatile ( "isb       \n");			\
		}

#endif	/*__da16200_arch_h__*/

/* EOF */
