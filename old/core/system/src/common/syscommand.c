/**
 ****************************************************************************************
 *
 * @file syscommand.c
 *
 * @brief System Commands
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

#include "oal.h"
#include "hal.h"
#include "da16x_system.h"

#include "monitor.h"
#include "command.h"
#include "syscommand.h"
#include "common_def.h"
#include "tcb.h"

#define	DISPALY(x,tag) {	\
			PRINTF( "\nOS.Perf." # x tag ": %10u (%10u)" , x , (x - old_ ## x) );	\
			old_ ## x = x ;								\
		}

char *stringTaskStatus[] = { "Running", "Ready", "Blocked", "Suspend", "Deleted", "Invalid" };

#ifdef  USED_HEAP_BLOCK_STATUS
extern void printUsedHeapBlockInfo();
#endif	/* USED_HEAP_BLOCK_STATUS */

/* print task information */
void printTaskInfo(void)
{
	TaskStatus_t	*pxTaskStatusArray;
	UBaseType_t		uxArraySize, x;
	uint32_t		ulTotalTime, ulStatsAsPercentage;
	TaskStatus_t	*pTaskInfo;
	int				min = 0;
	UBaseType_t		searchNo = 0;
	UBaseType_t		copyCount;
	int				stackSize;
	tskTCB			*p_TCB;
	char			*taskStatus;

	uxArraySize = uxTaskGetNumberOfTasks();

	PRINTF("\n <<< Task information >>> \n");

	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray != NULL) {

		/* Generate the (binary) data. */
		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalTime);

		/* Sort by xTaskNumber */
		pTaskInfo = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
		if (pTaskInfo != NULL) {
			for( copyCount = 0; copyCount < uxArraySize; ) {
				for( UBaseType_t i = 0; i < uxArraySize; i++ ) {
					if (searchNo == pxTaskStatusArray[i].xTaskNumber) {
						memcpy(&pTaskInfo[min], &pxTaskStatusArray[i], sizeof(TaskStatus_t));
						++copyCount;
						++min;
						break;
					}
				}
				++searchNo;
			}
		}
		else {
			PRINTF(RED_COLOR " pTaskInfo Alloc failed\n" CLEAR_COLOR);
			vPortFree(pxTaskStatusArray);
			return;
		}

		PRINTF(" Task count: %d  TotalTime: %d Ticks\n", uxArraySize, ulTotalTime);

		/* Avoid divide by zero errors. */
		if (ulTotalTime > 0UL) {
			PRINTF(" ==============================================================================\n");
			PRINTF(" No TaskName    State   Run-Tm  Run-Per    Prio Stack-B Stack-E  S-Size Stack-L\n");
			PRINTF(" ==============================================================================\n");
			/* Create a human readable table from the binary data. */
			for( x = 0; x < uxArraySize; x++ ) {

				ulStatsAsPercentage = (pTaskInfo[x].ulRunTimeCounter*100) / ulTotalTime;

				p_TCB = pTaskInfo[x].xHandle;
				stackSize = (p_TCB->pxEndOfStack - p_TCB->pxStack) +2;

				switch (pTaskInfo[x].eCurrentState) {
					case eRunning:
						taskStatus = stringTaskStatus[eRunning];
						break;

					case eReady:
						taskStatus = stringTaskStatus[eReady];
						break;

					case eBlocked:
						taskStatus = stringTaskStatus[eBlocked];
						break;

					case eSuspended:
						taskStatus = stringTaskStatus[eSuspended];
						break;

					case eDeleted:
						taskStatus = stringTaskStatus[eDeleted];
						break;

					case eInvalid:		/* Fall through. */
					default:			/* Should not get here, but it is included
										 * to prevent static checking errors. */
						taskStatus = NULL;
						break;
				}
				/* TaskNo TaskName State Run-Tm */
				PRINTF(" %2d %-11s %-7s%7u ",
								pTaskInfo[x].xTaskNumber,
								pTaskInfo[x].pcTaskName,
								taskStatus,
								pTaskInfo[x].ulRunTimeCounter);
				/* Run-Per */
				if (ulStatsAsPercentage > 0UL) {
					PRINTF("%7u%% ", ulStatsAsPercentage);
				}
				else {
					PRINTF("%7s%% ", "<1");
				}
				/* Prio Stack-B Stack-E S-Size Stack-H */
				PRINTF("%7u 0x%x 0x%x %7u %7d\n",
							pTaskInfo[x].uxCurrentPriority,
							pTaskInfo[x].pxStackBase,
							p_TCB->pxEndOfStack,
							stackSize * sizeof(StackType_t),	//Change stack size unit : StackType_t -> byte
							pTaskInfo[x].usStackHighWaterMark * sizeof(StackType_t));	//Change stack size unit : StackType_t -> byte
			}
		}
		else {
			PRINTF(" Error : get alive time\n");
		}

		PRINTF(" ==============================================================================\n");

		vTaskDelay(3);

		vPortFree(pxTaskStatusArray);
		vPortFree(pTaskInfo);
	}
	else {
		PRINTF(RED_COLOR " pxTaskStatusArray Alloc failed\n" CLEAR_COLOR);
	}
}

void *getTaskInfo(char *task)
{
	TaskStatus_t    *pxTaskStatusArray;
	UBaseType_t     uxArraySize, x;
	uint32_t        ulTotalTime;
	TaskStatus_t    *pTaskInfo;
	tskTCB          *p_TCB = NULL;

	uxArraySize = uxTaskGetNumberOfTasks();

	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray != NULL) {

		pTaskInfo = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalTime);

		PRINTF(" Task count: %d  TotalTime: %d Ticks \n", uxArraySize, ulTotalTime);

		/* Avoid divide by zero errors. */
		if (ulTotalTime > 0UL) {
			for( x = 0; x < uxArraySize; x++ ) {

				if (strcmp((char *)task, (char *)pTaskInfo[x].pcTaskName) == 0) {
					PRINTF(" Matched %s  \n", pTaskInfo[x].pcTaskName);
					p_TCB = pTaskInfo[x].xHandle;
				}
			}
		}
		else {
			PRINTF(" Error : get alive time \r\n");
		}

		vPortFree(pxTaskStatusArray);
		vPortFree(pTaskInfo);
	}
	else {
		PRINTF(RED_COLOR " pxTaskStatusArray Alloc failed \r\n" CLEAR_COLOR);
	}

	return p_TCB;
}

void heapInfo(void)
{
#ifdef	USING_HEAP_5
	size_t	size;
	size_t	totalsize;

	PRINTF("\r\n <<< HEAP Status >> \r\n");
	{
		int i;
		extern HeapRegion_t xHeapRegions[];	

		totalsize = 0;
		for(i=0; xHeapRegions[i].pucStartAddress != NULL; i++ ){
#ifdef __USING_REDUNDANCY_HEAP__
			PRINTF("xHeapRegions[%d].pucStartAddress : %p\r\n", i, xHeapRegions[i].pucStartAddress);
			PRINTF("xHeapRegions[%d].xSizeInBytes    : %d\r\n", i, xHeapRegions[i].xSizeInBytes);
#else
			PRINTF("Heap StartAddress  : %p\r\n", xHeapRegions[i].pucStartAddress);
			PRINTF("Heap Size(in Bytes): %d\r\n", xHeapRegions[i].xSizeInBytes);
#endif
			totalsize += xHeapRegions[i].xSizeInBytes;
		}
	}

	PRINTF("\n Total os.HEAP                     : %d\r\n", totalsize);
	size = xPortGetMinimumEverFreeHeapSize();
	PRINTF(" Minimum ever free bytes remaining : %d\r\n", size);
	size = xPortGetFreeHeapSize();
	PRINTF(GREEN_COLOR " Available HEAP space              : %d\r\n" CLEAR_COLOR, size);

#if 0	// FOR_DEBUGGING
	{
		extern unsigned int _Heap_Begin;
		extern unsigned int _Heap_Limit;
		PRINTF("\nsys.Heap: begin 0x%p, end 0x%p, size  %8d\r\n"
					, &_Heap_Begin, &_Heap_Limit
					, ((unsigned int)(&_Heap_Limit) - (unsigned int)(&_Heap_Begin)) );
		PRINTF("          curr  0x%p, usage %8d, avail %8d\r\n"
					, _sbrk(0)
					, (((unsigned int)_sbrk(0))- (unsigned int)(&_Heap_Begin))
					, ((unsigned int)(&_Heap_Limit) - ((unsigned int)_sbrk(0))) );
	}
#endif
	
#ifdef  USED_HEAP_BLOCK_STATUS
	printUsedHeapBlockInfo();
#endif	/* USED_HEAP_BLOCK_STATUS */

#else	/* HEAP_4 */
	HeapStats_t HeapStatus;

	vPortGetHeapStats(&HeapStatus);

	PRINTF("\r\n <<< HEAP Status >> \r\n");
	//PRINTF(" Available heap space:            %d \r\n", HeapStatus.xAvailableHeapSpaceInBytes);
	PRINTF(" Size of largest free block:        %d \r\n", HeapStatus.xSizeOfLargestFreeBlockInBytes);
	PRINTF(" Size of smallest free block:       %d \r\n", HeapStatus.xSizeOfSmallestFreeBlockInBytes);
	PRINTF(" Number of free blocks:             %d \r\n", HeapStatus.xNumberOfFreeBlocks);
	PRINTF(" Minimum ever free bytes remaining: %d \r\n", HeapStatus.xMinimumEverFreeBytesRemaining);
	PRINTF(" Number of successful allocations:  %d \r\n", HeapStatus.xNumberOfSuccessfulAllocations);
	PRINTF(" Number of successful frees:        %d \r\n", HeapStatus.xNumberOfSuccessfulFrees);
	PRINTF(GREEN_COLOR " Total HEAP:               %d \r\n" CLEAR_COLOR, configTOTAL_HEAP_SIZE);
	PRINTF(GREEN_COLOR " Available heap space:     %d \r\n" CLEAR_COLOR, HeapStatus.xAvailableHeapSpaceInBytes);
#endif	/* USING_HEAP_5 */
}

/* PRINTF semaphore information */
void semInfo(void)
{

}

/* PRINTF hisr information */
void hisrInfo(void)
{

}

/* PRINTF Queue informaiton */
void qInfo(void)
{

}

/* PRINTF Event informaiton */
void eventInfo(void)
{

}


void memoryPoolInfo(void)
{
}


void osHistory(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);
}

int isStrn(char* arg,int n)
{
	int i;

	for (i = 0; i < n && *arg != '\0'; arg++)
	{
		if (*arg >= 0x20)
			continue;
		else
			return 0;
	}
	return 1;
}


/* is it ASCII ? */
int isascii (char* arg)
{
	if (*arg >= 0x20)
		return 1;
	else
		return 0;
}

void bfill ( char* ascii, int length, char conts)
{
	int i;

	for (i = 0; i < length; i++)
	{
		*(ascii+i) = conts;
	}
}

int my_isxdigit(int digit)
{
	if ((digit >= 0x30 && digit <= 0x39) || (digit >= 'a' && digit <= 'f')
			|| (digit >= 'A' && digit <= 'F'))
		return 1;
	else
		return 0;
}


void skipSpace(unsigned char** strn)
{
	while (**strn == SP)
		++*strn;
}

int getArg(unsigned char** strn, unsigned long *arg)
{
	*arg= 0;

	skipSpace(strn);
	
	if (**strn == ',')
		++*strn;

	if (**strn == '0' && *(*strn+1) == 'x')
		*strn += 2;

	if (my_isxdigit(**strn))
	{
		unsigned long tempVal;

		tempVal = 0;

		while (**strn != SP && **strn != ',' && **strn != 0x0D)
		{
			if (my_isxdigit(**strn))
			{
				if (**strn >= 0x30 && **strn <= 0x39)
				{
					tempVal = (**strn&0x0f);
				}
				else
				{
					tempVal = (**strn)&0x0f;
					tempVal += 9;
				}

				*arg=(*arg) << (4);
				*arg |= tempVal;
				//conPrint("%x\n",*arg);
				++*strn;
				//i++;
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

/* EOF */
