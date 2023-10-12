/**
 ****************************************************************************************
 *
 * @file command_sys_os.c
 *
 * @brief Command Parser
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
#include "project_config.h"
#include "sdk_type.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "oal.h"
#include "hal.h"
#include "driver.h"
#include "da16x_system.h"

#include "monitor.h"
#include "command.h"
#include "syscommand.h"
#include "command_sys.h"
#include "tcb.h"
#include "da16x_sys_watchdog.h"

#pragma GCC diagnostic ignored "-Warray-bounds"

#if defined ( WATCH_DOG_ENABLE )
extern void watch_dog_kick_start();
extern void watch_dog_kick_stop();
#endif //( WATCH_DOG_ENABLE )

//-----------------------------------------------------------------------
// Command SYS-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_sys_os_list[] = {
  { "OS",		CMD_MY_NODE,	cmd_sys_os_list,	NULL,	    "OS command "	} , // Head

  { "-------",	CMD_FUNC_NODE,	NULL,	NULL,		"------------------------------"},
  { "heap",	CMD_FUNC_NODE,	NULL,	&cmd_heapinfo_func, "HEAP Info"			},

  { NULL, 		CMD_NULL_NODE,	NULL,	NULL,		NULL }			// Tail
};


//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

void	cmd_taskinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	printTaskInfo();
}

#ifdef XIP_CACHE_BOOT  //rtos
void	cmd_lwip_mem_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void lwip_mem_status();

	lwip_mem_status();
}

void	cmd_lwip_memp_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void lwip_memp_status();

	lwip_memp_status();
}

void	cmd_lwip_tcp_pcb_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void lwip_tcp_pcb_status();

	lwip_tcp_pcb_status();
}

void	cmd_lwip_udp_pcb_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void lwip_udp_pcb_status();

	lwip_udp_pcb_status();
}

void	cmd_lwip_timer_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void lwip_timer_status();

	lwip_timer_status();
}

void	cmd_timer_status(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void prvPrintTimerListsItem();

	prvPrintTimerListsItem();
}
#endif

#ifndef	__REDUCE_CODE_SIZE__
void	cmd_seminfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	semInfo();
}

void	cmd_hisrinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	hisrInfo();
}

void	cmd_queinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	qInfo();
}

void	cmd_eventinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	eventInfo();
}
#endif	/* __REDUCE_CODE_SIZE__ */

void	cmd_poolinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	//memoryPoolInfo();		/* F_F_S */
}

void	cmd_memprof_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

#ifdef	SUPPORT_MEM_PROFILE
	print_mem_profiler( );
#endif	//SUPPORT_MEM_PROFILE
}

#if defined ( WATCH_DOG_ENABLE )
void	cmd_wdt_kick_start_func(int argc, char *argv[])	/* for test */
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	watch_dog_kick_start();
	PRINTF(" watch dog kicking start \r\n");
}

void	cmd_wdt_kick_stop_func(int argc, char *argv[])	/* for test */
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	watch_dog_kick_stop();
	PRINTF(" watch dog kicking stop \r\n");
}
#endif //( WATCH_DOG_ENABLE )

HeapStats_t HeapStatus;
void	cmd_heapinfo_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	extern void heapInfo(void);
	heapInfo();

#if 0	/* F_F_S */
#define DIALOG_MINFO_TYPE	size_t
#define STRUCT_MALLINFO_DECLARED 1
	struct dialog_minfo_type {
		/* non-mmapped space allocated from system */
		DIALOG_MINFO_TYPE arena;

		/* number of free chunks */
		DIALOG_MINFO_TYPE ord_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE sm_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE hb_lks;

		/* space in mmapped regions */
		DIALOG_MINFO_TYPE hb_lkhd;

		/* maximum total allocated space */
		DIALOG_MINFO_TYPE usm_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE fsm_blks;

		/* total allocated space */
		DIALOG_MINFO_TYPE uord_blks;

		/* total free space */
		DIALOG_MINFO_TYPE ford_blks;

		/* releasable (via malloc_trim) space */
		DIALOG_MINFO_TYPE keep_cost;
	};

extern struct dialog_minfo_type __iar_dlmallinfo(void);

	struct dialog_minfo_type m;
	m = __iar_dlmallinfo();
	PRINTF("total alloc space = %u\n", m.uord_blks);
	PRINTF("total free  space = %u\n", m.ford_blks);
	PRINTF("asigned space = %u\n", HEAP_MEM_SIZE());
#endif
}

void cmd_memory_map_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

#ifdef USING_HEAP_5
	extern unsigned int __data_regions_array_start;
	extern unsigned int __bss_regions_array_start;
#ifdef __USING_REDUNDANCY_HEAP__
	extern unsigned int __bss_regions_array_end;
#endif	//__USING_REDUNDANCY_HEAP__
	extern HeapRegion_t xHeapRegions[];

	unsigned int	*p;
	unsigned int 	 i, totalsize;
	unsigned int	 bss_start_addr, bss_end_addr;

	p = &__bss_regions_array_start;
	bss_start_addr	= *p++;
	bss_end_addr	= *p;

	totalsize = 0;
	for(i=0; xHeapRegions[i].pucStartAddress != NULL; i++ ){
		totalsize += xHeapRegions[i].xSizeInBytes;
	}

	Printf("\r\n            << SRAM Memory map >> \r\n");

	Printf(" 0x%08x +-----------------------------+ \n", &__region_SYSRAM0_start__);
	Printf("            |   System memory             | \n");
	Printf("            |   (DMA Area, ISR Vector...) | \n");
#ifdef __USING_REDUNDANCY_HEAP__
	Printf(" 0x%08x +-----------------------------+ \n", xHeapRegions[0].pucStartAddress);
	Printf("            |   HEAP_1 (size:%8d)    | \n", xHeapRegions[0].xSizeInBytes);
#else
	Printf("            |                             | \n");
#endif
	Printf(" 0x%08x +-----------------------------+ \n", &__region_RAMIPD_start__);
	Printf("            |   System memory (IPD)       | \n");  			p = &__data_regions_array_start; p++;
	Printf(" 0x%08x +-----------------------------+ \n", *p);			p++;
	Printf("            |   Text (RAM Regident)       | \n");
	Printf(" 0x%08x +-----------------------------+ \n", *p);
	Printf("            |   DATA (rw)                 | \n");
	Printf(" 0x%08x +-----------------------------+ \n", bss_start_addr);
	Printf("            |                             | \n");
	Printf("            |   BSS (size:%8d)       | \n", bss_end_addr - bss_start_addr);
	Printf("            |                             | \n");
#ifdef __USING_REDUNDANCY_HEAP__
	if (i >= 2) {
	Printf(" 0x%08x +-----------------------------+ \n", xHeapRegions[1].pucStartAddress);
	Printf("            |                             | \n");
	Printf("            |                             | \n");
	Printf("            |   HEAP_2 (size:%8d)    | \n", xHeapRegions[1].xSizeInBytes);
	Printf("            |                             | \n");
	Printf("            |                             | \n");
	Printf(" 0x%08x +-----------------------------+ \n", xHeapRegions[1].pucStartAddress+xHeapRegions[1].xSizeInBytes);
	}
	else {	/* 1 Heap */
	p = &__bss_regions_array_end;
	Printf(" 0x%08x +-----------------------------+ \n", *p);
	}
#else
	Printf(" 0x%08x +-----------------------------+ \n", xHeapRegions[0].pucStartAddress);
	Printf("            |                             | \n");
	Printf("            |                             | \n");
	Printf("            |   HEAP   (size:%8d)    | \n", xHeapRegions[0].xSizeInBytes);
	Printf("            |                             | \n");
	Printf("            |                             | \n");
	Printf(" 0x%08x +-----------------------------+ \n", xHeapRegions[0].pucStartAddress+xHeapRegions[0].xSizeInBytes);
#endif
	Printf("\n");
#endif
}

#ifndef	__REDUCE_CODE_SIZE__
void	cmd_taskctrl_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);

	tskTCB  *thread;

	if(argc < 2) {
		PRINTF("Usage : %s [task name]\n", argv[0]);
		return;
	}

	if ((thread = (tskTCB *)getTaskInfo(argv[1])) != NULL) {

		if(DRIVER_STRCMP(argv[0], "resume") == 0){
			PRINTF(" Resume %s ......\n", argv[1]);
		}else
		if(DRIVER_STRCMP(argv[0], "suspend") == 0){
			PRINTF(" Suspend %s ......\n", argv[1]);
		}else
		if(DRIVER_STRCMP(argv[0], "term") == 0){
			PRINTF(" Terminate %s ......\n", argv[1]);
		}else
		if(DRIVER_STRCMP(argv[0], "reset") == 0){
			PRINTF(" Reset %s ......\n", argv[1]);
		}
	}
}

void	cmd_sleep_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);

	long timestamp ;

	if( argc != 2 ){
		return;
	}

	timestamp = xTaskGetTickCount();

	PRINTF("Before-Sleep (%d)\n", timestamp );
	vTaskDelay(ctoi(argv[1])/10);

	timestamp = xTaskGetTickCount() - timestamp;

	PRINTF("After-Sleep (%d)\n", timestamp );
}

#endif	/* __REDUCE_CODE_SIZE__ */

void cmd_sys_wdog_func(int argc, char *argv[])
{
    da16x_sys_watchdog_cmd(argc, argv);

    return ;
}

/* EOF */
