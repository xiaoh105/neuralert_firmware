/**
 ****************************************************************************************
 *
 * @file command_sys.h
 *
 * @brief Header file for Console Commands to handle and to test
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

#ifndef	__COMMAND_SYS_H__
#define	__COMMAND_SYS_H__

#include "command.h"

//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

extern const COMMAND_TREE_TYPE	cmd_sys_list[] ;
extern const COMMAND_TREE_TYPE	cmd_sys_os_list[] ;
extern const COMMAND_TREE_TYPE	cmd_sys_hal_list[] ;

//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

extern void	cmd_taskinfo_func(int argc, char *argv[]);
#ifdef XIP_CACHE_BOOT  //rtos
extern void	cmd_lwip_mem_func(int argc, char *argv[]);
extern void	cmd_lwip_memp_func(int argc, char *argv[]);
extern void	cmd_lwip_tcp_pcb_func(int argc, char *argv[]);
extern void	cmd_lwip_udp_pcb_func(int argc, char *argv[]);
extern void	cmd_lwip_timer_func(int argc, char *argv[]);
extern void	cmd_timer_status(int argc, char *argv[]);
#endif
extern void	cmd_seminfo_func(int argc, char *argv[]);
extern void	cmd_hisrinfo_func(int argc, char *argv[]);
extern void	cmd_queinfo_func(int argc, char *argv[]);
extern void	cmd_eventinfo_func(int argc, char *argv[]);
extern void	cmd_poolinfo_func(int argc, char *argv[]);
extern void	cmd_sleep_func(int argc, char *argv[]);

extern void	cmd_memprof_func(int argc, char *argv[]);
extern void	cmd_heapinfo_func(int argc, char *argv[]);
extern void	cmd_memory_map_func(int argc, char *argv[]);
extern void	cmd_taskctrl_func(int argc, char *argv[]);

extern void cmd_dmips_func(int argc, char *argv[]);
extern void cmd_coremark_func(int argc, char *argv[]);
extern void	cmd_cpuload_func(int argc, char *argv[]);
extern void cmd_sysinfo_func(int argc, char *argv[]);
extern void cmd_sysbtm_func(int argc, char *argv[]);
extern void	cmd_oops_func(int argc, char *argv[]);
extern void	cmd_bmconfig_func(int argc, char *argv[]);
extern void	cmd_cache_func(int argc, char *argv[]);
extern void	cmd_wdog_func(int argc, char *argv[]);
extern void	cmd_sysidle_func(int argc, char *argv[]);

extern void	cmd_image_store_cmd(int argc, char *argv[]);
extern void	cmd_image_load_cmd(int argc, char *argv[]);
extern void	cmd_image_boot_cmd(int argc, char *argv[]);

extern void	cmd_clock_cmd(int argc, char *argv[]);
extern void cmd_rs485_image_store_cmd(int argc, char *argv[]);

extern void cmd_sys_wdog_func(int argc, char *argv[]);

#endif /* __COMMAND_SYS_H__ */

/* EOF */
