/* File: startup_ARMCM4.S
 * Purpose: startup file for Cortex-M4 devices. Should use with
 *   GCC for ARM Embedded Processors
 * Version: V2.0
 * Date: 16 August 2013
 *
 * Copyright (c) 2011 - 2013 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/
	.syntax	unified

	.extern __Main_Stack_Limit

	.section .isr_vector,"a",%progbits
	.align	4
	.global	__isr_vectors
	.global __Vectors

__Vectors:
__isr_vectors: 	/* add ASSEMBLY_START */
    .long   __Main_Stack_Limit - 4		/* Top of Stack at top of RAM*/
	.long	Reset_Handler				/* Reset Handler */
	.long	NMI_Handler					/* NMI Handler */
	.long	HardFault_Handler			/* Hard Fault Handler */
	.long	MemManage_Handler			/* MPU Fault Handler */
	.long	BusFault_Handler			/* Bus Fault Handler */
	.long	UsageFault_Handler			/* Usage Fault Handler */
	.long	0							/* Reserved */
	.long	0							/* Reserved */
	.long	0							/* Reserved */
	.long	0							/* Reserved */
	.long	SVC_Handler					/* SVCall Handler */
	.long	DebugMon_Handler			/* Debug Monitor Handler */
	.long	0							/* Reserved */
	.long	PendSV_Handler				/* PendSV Handler */
	.long	SysTick_Handler				/* SysTick Handler */
	.long	0							/* Int 0	*/
	.long	0							/* Int 1	*/
	.long	0							/* Int 2	*/
	.long	0							/* Int 3	*/

	.section .after_vectors,"a",%progbits
	/* .text */
	.thumb
	.thumb_func
	.align	4
	.globl _start
	.extern main
	.globl	Reset_Handler
	.type	Reset_Handler, %function
_start:
Reset_Handler:
	ldr sp, =__Main_Stack_Limit
	sub sp, sp, #4

#ifndef __NO_SYSTEM_INIT
/*	bl	SystemInit */
#endif

	bl	__start
	/* bl	main */

	.pool
	.size	Reset_Handler, . - Reset_Handler

	.align	1
	.thumb_func
	.weak	Default_Handler
	.type	Default_Handler, %function
Default_Handler:
	b	.
	.size	Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
	.macro	def_irq_handler	handler_name
	.weak	\handler_name
	.set	\handler_name, Default_Handler
	.endm

	.end
