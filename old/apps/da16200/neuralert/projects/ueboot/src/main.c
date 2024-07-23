/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

#include "diag/Trace.h"
#include "da16200_arch.h"
#include "da16x_types.h"
#include "oal.h"
#include "sdk_type.h"
#include "common_def.h"
#include "project_config.h"

// ----------------------------------------------------------------------------
//
// Print a greeting message on the trace device and enter a loop
// to count seconds.
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//
// ----------------------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#define mainCREATOR_TASK_PRIORITY			( tskIDLE_PRIORITY + 3UL )

extern void boot_loader( void *pvParameters );
extern void Printf(const char *fmt,...);
extern void da16200_trace(int index, int value);

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file. See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

#ifdef	USING_HEAP_5
extern unsigned int __region_RAM0_start__;
extern unsigned int __region_RAM0_end__;
extern unsigned int __region_RAMIPD_start__;
extern unsigned int __region_RAMIPD_end__;

#define	RAM0_START_ADDRESS	&__region_RAM0_start__
#define	RAM0_SIZE			((unsigned int)(&__region_RAM0_end__) - (unsigned int)(&__region_RAM0_start__))
#define	RAM1_START_ADDRESS	&__region_RAMIPD_start__
#define	RAM1_SIZE			((unsigned int)(&__region_RAMIPD_end__) - (unsigned int)(&__region_RAMIPD_start__))

#define SUPPORT_RUNTIME_HEAP_MANAGE
#ifdef SUPPORT_RUNTIME_HEAP_MANAGE
// Variable size.
// It automatically calculates the heap size every booting time.
extern unsigned int _Heap_Limit;
extern caddr_t _sbrk(int incr);

#define HEAP_AVAIL_SIZE		((unsigned int)(&_Heap_Limit) - ((unsigned int)_sbrk(0)) - sizeof(unsigned int))
#define OTHER_APPS_MARGIN	(12*1024) // (8*1024) // reserved for built-in libraries (libc, ...)
#else  //SUPPORT_RUNTIME_HEAP_MANAGE
// Code fixed size.
// It is useful as it can detect the heap overflow during the compile time.
#define RAM2_HEAP_SIZE	(configTOTAL_HEAP_SIZE - 0xB200 - 0x800)	/* configTOTAL_HEAP_SIZE - RAM0, RAM1 size */
uint8_t	ucHeap[RAM2_HEAP_SIZE]; 
#endif	//SUPPORT_RUNTIME_HEAP_MANAGE

HeapRegion_t xHeapRegions[3];


#endif	/* USING_HEAP_5 */

int
main (int argc, char* argv[])
{

	// Normally at this stage most of the microcontroller subsystems, including
	// the clock, were initialised by the CMSIS SystemInit() function invoked
	// from the startup file, before calling main().
	// (see system/src/cortexm/_initialize_hardware.c)
	// If further initialisations are required, customise __initialize_hardware()

	da16200_trace(0, 0x11110003);

#ifdef USING_HEAP_5
	{
	HeapRegion_t	*p_xHeapRegions = xHeapRegions;
	
	p_xHeapRegions = xHeapRegions;

	/* region 0 : RAM0 (+ RAMIPD) */
	p_xHeapRegions->pucStartAddress = (uint8_t *)RAM0_START_ADDRESS;
	p_xHeapRegions->xSizeInBytes = (size_t)RAM0_SIZE ;

	/* region 1 : ucHeap */
#ifdef SUPPORT_RUNTIME_HEAP_MANAGE
	++p_xHeapRegions;
	if( configTOTAL_HEAP_SIZE < (HEAP_AVAIL_SIZE - OTHER_APPS_MARGIN)){
		p_xHeapRegions->pucStartAddress = (uint8_t *)_sbrk(configTOTAL_HEAP_SIZE);
		p_xHeapRegions->xSizeInBytes = (size_t)(configTOTAL_HEAP_SIZE);
	}else{
		volatile size_t allocsize;
		allocsize = (HEAP_AVAIL_SIZE - OTHER_APPS_MARGIN);
		p_xHeapRegions->pucStartAddress = (uint8_t *)_sbrk((int)allocsize);
		p_xHeapRegions->xSizeInBytes = (allocsize);
	}
#else  //SUPPORT_RUNTIME_HEAP_MANAGE
	++p_xHeapRegions;
	p_xHeapRegions->pucStartAddress = ucHeap;
	p_xHeapRegions->xSizeInBytes = RAM2_HEAP_SIZE;
#endif  //SUPPORT_RUNTIME_HEAP_MANAGE

	/* End mark */
	++p_xHeapRegions;
	p_xHeapRegions->pucStartAddress = NULL;
	p_xHeapRegions->xSizeInBytes = 0;

	vPortDefineHeapRegions(xHeapRegions);
	}
#endif	/* USING_HEAP_5 */

	xTaskCreate(boot_loader,
				"boot_loader",
				256*3,    // for SecureBoot
				(void *)NULL,
				OS_TASK_PRIORITY_USER,
				NULL);

	/* Start the scheduler itself. */
	vTaskStartScheduler();

	da16200_trace(0, 0x1111000F);

	// Infinite loop, never return.
	while (1) { 
		vTaskDelay(1000);
	}
}

/* Catch asserts so the file and line number of the assert can be viewed. */
void vMainAssertCalled( const char *pcFileName, uint32_t ulLineNumber )
{
volatile BaseType_t xSetToNonZeroToStepOutOfLoop = 0;

	//Printf(">> ASSERT %s %d !!! \r\n", pcFileName, ulLineNumber);
	Printf(">> ASSERT %s %d !!! \r\n", pcFileName+50, ulLineNumber);		/* TEMPORARY */

	taskENTER_CRITICAL();
	while( xSetToNonZeroToStepOutOfLoop == 0 ) {
		/* Use the variables to prevent compiler warnings and in an attempt to
		 ensure they can be viewed in the debugger. If the variables get
		 optimised away then set copy their values to file scope or globals then
		 view the variables they are copied to. */
		( void ) pcFileName;
		( void ) ulLineNumber;
	}
}

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created. It is also called by various parts of the
	demo application. If heap_1.c, heap_2.c or heap_4.c is being used, then the
	size of the	heap available to pvPortMalloc() is defined by
	configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
	API function can be used to query the size of free heap space that remains
	(although it does not provide information on how the remaining heap might be
	fragmented). See http://www.freertos.org/a00111.html for more
	information. */
//	vAssertCalled( __LINE__, __func__ );
	vMainAssertCalled(__func__, __LINE__);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
	task. It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()). If application tasks make use of the
	vTaskDelete() API function to delete themselves then it is also important
	that vApplicationIdleHook() is permitted to return to its calling function,
	because it is the responsibility of the idle task to clean up memory
	allocated by the kernel to any task that has since deleted itself. */

	/* Uncomment the following code to allow the trace to be stopped with any
	key press. The code is commented out by default as the kbhit() function
	interferes with the run time behaviour. */
	/*
		if( _kbhit() != pdFALSE )
		{
			if( xTraceRunning == pdTRUE )
			{
				vTraceStop();
				prvSaveTraceFile();
				xTraceRunning = pdFALSE;
			}
		}
	*/

}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook
	function is called if a stack overflow is detected.*/

//	vAssertCalled( __LINE__, __func__ );
	vMainAssertCalled(__func__, __LINE__);
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */

}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
	/* This function will be called once only, when the daemon task starts to
	execute	(sometimes called the timer task). This is useful if the
	application includes initialisation code that would benefit from executing
	after the scheduler has been started. */
}

/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;

/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
use a callback function to optionally provide the memory required by the idle
and timer tasks. This is the stack that will be used by the timer task. It is
declared here, as a global, so it can be checked by a test that is implemented
in a different file. */
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
