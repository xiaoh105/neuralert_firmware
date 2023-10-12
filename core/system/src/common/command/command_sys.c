/**
 ****************************************************************************************
 *
 * @file command_sys.c
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

#include "sdk_type.h"

#include <stdio.h>
#include <string.h>

#include "oal.h"
#include "hal.h"
#include "driver.h"
#include "da16x_system.h"
#include "sys_image.h"

#include "monitor.h"
#include "command.h"
#include "environ.h"
#include "command_sys.h"
#include "da16x_oops.h" 
#include "common_def.h"

#include "da16x_secureboot.h"

extern void da16x_environ_lock(UINT32 flag);
#if defined ( WATCH_DOG_ENABLE )
extern void watch_dog_kick_start();
extern void watch_dog_kick_stop();
#endif //( WATCH_DOG_ENABLE )

//#define	da16x_environ_lock
//#define	da16x_idle_set_info
//#define	da16x_idle_get_info

extern void system_event_monitor(UINT32 mode);
extern void system_event_busmon(UINT32 mode, UINT32 para);
extern void system_event_cpumon(UINT32 mode, UINT32 para);

//#define	da16x_oops_init
//#define	da16x_oops_view
//#define	da16x_oops_clear
#define tx_time_get()		0		/* F_F_S */

#ifdef	XIP_CACHE_BOOT
	#undef	SUPPORT_SYSTEM_INFO
	#undef	SUPPORT_DMIPS_INFO
	#undef	SUPPORT_COREMARK_INFO
	#undef	SUPPORT_CPULOAD_TEST
	#define	SUPPORT_SBROM_TEST
	#define	SUPPORT_SPROD_TEST
	#define	__SUPPORT_HAL_CMD__
#else	//XIP_CACHE_BOOT
	#undef	SUPPORT_SYSTEM_INFO
	#undef	SUPPORT_DMIPS_INFO
	#undef	SUPPORT_COREMARK_INFO
	#undef	SUPPORT_CPULOAD_TEST
	#define	SUPPORT_SBROM_TEST	// support SBROM command
	#define	SUPPORT_SPROD_TEST	// support Production command
#endif	//XIP_CACHE_BOOT

#undef	SUPPORT_SYS_DEBUG_TEST
#undef	SUPPORT_DOWNLOAD_TEST

//-----------------------------------------------------------------------
// Internal functions
//-----------------------------------------------------------------------

#ifdef	SUPPORT_SBROM_TEST
extern void cmd_sbrom_func(int argc, char *argv[]);
#endif	//SUPPORT_SBROM_TEST
#ifdef	SUPPORT_SPROD_TEST
extern void cmd_sprod_func(int argc, char *argv[]);
#endif	//SUPPORT_SPROD_TEST

#define	OAL_MSLEEP(wtime)		{							\
			portTickType xFlashRate, xLastFlashTime;		\
			xFlashRate = wtime/portTICK_RATE_MS;			\
			xLastFlashTime = xTaskGetTickCount();			\
			vTaskDelayUntil( &xLastFlashTime, xFlashRate );	\
		}

//-----------------------------------------------------------------------
// Command SYS-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_sys_list[] = {
  { "SYS",			CMD_MY_NODE,	cmd_sys_list,			NULL, "System command "				} ,		// Head

#ifdef	XIP_CACHE_BOOT
  { "os",			CMD_SUB_NODE,	cmd_sys_os_list,		NULL,	"OS command"				},
#ifdef __SUPPORT_HAL_CMD__
  { "hal",			CMD_SUB_NODE,	cmd_sys_hal_list,		NULL,	"HAL command"				},
#endif
#ifdef __ENABLE_PERI_CMD__
  { "spi_master",	CMD_SUB_NODE,	cmd_spi_master_list,	NULL,	"spi master command"		},
#endif
#endif	//XIP_CACHE_BOOT

  { "-------",		CMD_FUNC_NODE,	NULL,	NULL,			"--------------------------------"	},
#ifdef	SUPPORT_SYSTEM_INFO
  { "info",			CMD_FUNC_NODE,	NULL,	&cmd_sysinfo_func,		"SYS Info"					},
#endif	//SUPPORT_SYSTEM_INFO
#ifdef	SUPPORT_DMIPS_INFO 
  { "dmips",		CMD_FUNC_NODE,	NULL,	&cmd_dmips_func,		"DMIPS"						},
#endif	//SUPPORT_DMIPS_INFO
#ifdef	SUPPORT_COREMARK_INFO
  { "coremark",		CMD_FUNC_NODE,	NULL,	&cmd_coremark_func,		"CoreMark"					},
#endif	//SUPPORT_COREMARK_INFO
#ifdef	SUPPORT_CPULOAD_TEST
  { "cpuload",		CMD_FUNC_NODE,	NULL,	&cmd_cpuload_func,		"CPU Load Test"				},
#endif	//SUPPORT_CPULOAD_TEST
#ifdef	SUPPORT_SYS_DEBUG_TEST
  { "idle",			CMD_FUNC_NODE,	NULL,	&cmd_sysidle_func,		"SYS IDLE info"				},
  { "btm",			CMD_FUNC_NODE,	NULL,	&cmd_sysbtm_func,		"BTM Info"					},
  { "bmcfg",		CMD_FUNC_NODE,	NULL,	&cmd_bmconfig_func,		"BootMode Config"			},
  { "oops",			CMD_FUNC_NODE,	NULL,	&cmd_oops_func,			"OOPS"						},
  { "cache",		CMD_FUNC_NODE,	NULL,	&cmd_cache_func,		"Cache"						},
#endif	// SUPPORT_SYS_DEBUG_TEST
#if defined ( WATCH_DOG_ENABLE )
  { "wdog",			CMD_FUNC_NODE,	NULL,	&cmd_wdog_func,			"WATCHDOG"					},
#endif	// WATCH_DOG_ENABLE
#ifdef	SUPPORT_DOWNLOAD_TEST
  { "loady",		CMD_FUNC_NODE,	NULL,	&cmd_image_store_cmd,	"Download to SFLASH"		},
  { "floady",		CMD_FUNC_NODE,	NULL,	&cmd_image_store_cmd,	"Fast Download to SFLASH"	},
  { "ymodem",		CMD_FUNC_NODE,	NULL,	&cmd_image_load_cmd,	"Download to RAM"			},
  { "boot",			CMD_FUNC_NODE,	NULL,	&cmd_image_boot_cmd,	"Boot Image"				},
#endif	//SUPPORT_DOWNLOAD_TEST

#ifdef	SUPPORT_SBROM_TEST
  { "-------",		CMD_FUNC_NODE,	NULL,	NULL,			"--------------------------------"	},
  { "sbrom",		CMD_FUNC_NODE,	NULL,	&cmd_sbrom_func,		"SecureBoot"				},
  { "socid",		CMD_FUNC_NODE,	NULL,	&cmd_sbrom_func,		"SecureBoot"				},
  { "sprod",		CMD_FUNC_NODE,	NULL,	&cmd_sprod_func,		"SecureProduct"				},
  { "srng",			CMD_FUNC_NODE,	NULL,	&cmd_sbrom_func,		"SecureBoot"				},
#endif	//SUPPORT_SPROD_TEST

	{ NULL, 		CMD_NULL_NODE,	NULL,	NULL,			NULL }				// Tail
};


#ifdef	SUPPORT_DMIPS_INFO
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

extern void calc_dmips (unsigned int loopcount);

void cmd_dmips_func(int argc, char *argv[])
{
	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_HAL(0));

	cmd_cache_func(0, NULL);
	calc_dmips( ctoi(argv[1]) );
	cmd_cache_func(0, NULL);

	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_APP(7));
}

#endif	//SUPPORT_DMIPS_INFO

#ifdef	SUPPORT_COREMARK_INFO
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

extern void coremark_main(int argc, char *argv[]);

void cmd_coremark_func(int argc, char *argv[])
{
	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_HAL(0));

	cmd_cache_func(0, NULL);
	coremark_main( argc-1, &(argv[1]) );
	cmd_cache_func(0, NULL);

	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_APP(7));
}

#endif	//SUPPORT_COREMARK_INFO

#ifdef	SUPPORT_CPULOAD_TEST
//-----------------------------------------------------------------------------
//
//	Simple CPU Load Test
//
//-----------------------------------------------------------------------------

void cmd_cpuload_func(int argc, char *argv[])
{
	UINT32 sysclock, cpuload, runtime;
	UINT32 curtime, starttime, itercount;
	UINT32 flag_noprint;

	if (argc != 4 && argc != 5) {
		return;
	}

	sysclock = ctoi(argv[1]);
	cpuload = ctoi(argv[2]);
	runtime = ctoi(argv[3]);

	if (argc == 5 && DRIVER_STRCMP("noprint", argv[4]) == 0) {
		flag_noprint = TRUE;
	}
	else {
		flag_noprint = FALSE;
	}

	cpuload = cpuload / 10;
	cpuload = (cpuload > 10) ? 10 : cpuload;

	//_sys_clock_write(&sysclock, sizeof(UINT32));
	_sys_clock_write(&sysclock, 0xFFFFFFFF);	/* F_F_S */
	_sys_clock_read(&sysclock, sizeof(UINT32));

	PRINTF(" >>> 111 CPU Load Test: Clock %9d Hz, Load = %4d %%, RunTime %8d sec\n", sysclock, cpuload, runtime);

	// idle set on
	da16x_idle_set_info(TRUE);

	// btm monitor on
	if (flag_noprint == FALSE) {
		da16x_btm_ch_select(0, 0x10); /* CPU : CM */
		da16x_btm_ch_setup( 0, BTM_CHM_CMD);
		da16x_btm_ch_select(1, 0x10); /* CPU : WM */
		da16x_btm_ch_setup( 1, BTM_CHM_WTM);
		da16x_btm_ch_select(2, 0x11); /* MAC : CM */
		da16x_btm_ch_setup( 2, BTM_CHM_CMD);
		da16x_btm_ch_select(3, 0x11); /* MAC : WM */
		da16x_btm_ch_setup( 3, BTM_CHM_WTM);
		da16x_btm_ch_select(4, 0x19); /* ISP : CM */
		da16x_btm_ch_setup( 4, BTM_CHM_CMD);
		da16x_btm_ch_select(5, 0x10); /* CPU : SM */
		da16x_btm_ch_setup( 5, BTM_CHM_SLP);
		system_event_cpumon(FALSE, 0);
	}

	//starttime = OAL_RETRIEVE_CLOCK();
	starttime = xTaskGetTickCount();

	if (flag_noprint == FALSE) {
		PRINTF("CPU Load Test : Clock %9d Hz, Load = %4d %%, RunTime %8d sec\n"
				"\t start time = %d ticks\n"
				, sysclock, (cpuload*10), runtime, starttime);
	}

	runtime = runtime * 100;
	itercount = (sysclock - (sysclock/15)) / 1200;

	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_HAL(0));

	if (flag_noprint == FALSE) {
		da16x_btm_control(TRUE);
		system_event_monitor( TRUE );
	}

	while(1) {
		volatile UINT32 loopcnt, itercnt;
		volatile UINT32 dump;

		loopcnt = cpuload;

		while(loopcnt-- > 0){
			itercnt = itercount;
			while(itercnt-- > 0){
				dump = SYSGET_CHIPNAME();
			}
		}

		loopcnt = 10 - cpuload;
		while(loopcnt-- > 0){
			SYSUSLEEP(10000);
			//OAL_MSLEEP(10);
		}

		//curtime = OAL_RETRIEVE_CLOCK();
		curtime = xTaskGetTickCount();
		if( (curtime - starttime) >= runtime ){
			break;
		}
	}

	if( flag_noprint == FALSE ){
		system_event_monitor( FALSE );
		da16x_btm_control(FALSE);
	}

	//OAL_CHANGE_PRIORITY(OAL_CURRENT_THREAD_POINTER(), OAL_PRI_APP(7));
}

#endif	//SUPPORT_CPULOAD_TEST

#ifdef	SUPPORT_SYSTEM_INFO
//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

#define	PRINT_BASINFO(base)		PRINTF("%s\t: %p\r\n", #base, base )
#define	PRINT_REGINFO(base,reg)		print_reg_info(0, #reg, (UINT32 *)&(base->reg) )
#define	PRINT_REGINFO32(base,reg)	print_reg_info(1, #reg, (UINT32 *)&(base->reg) )

#define PRINT_PERIINFO(base)	print_peri_info(#base, (UINT32)base)

static void print_reg_info(int mode, char *name, UINT32 *regaddr)
{
	if( mode == 0 ){
		PRINTF("\t%s\t (%p): %02x\r\n", name, regaddr, *regaddr );
	}else{
		PRINTF("\t%s\t (%p): %08x\r\n", name, regaddr, *regaddr );
	}
}

static void print_peri_info(char *name, UINT32 base)
{
	int i;
	volatile UINT32	*regaddr;
	const char pidindex[] = {'4', '5', '6', '7', '0', '1', '2', '3'};

	PRINTF("%s INFO-base  : %p\r\n", name, (void *)(((UINT32)base)|0x0FD0) );

	regaddr = (volatile UINT32 *)(((UINT32)base)|0x0FD0);
	for( i=0; i < 8; i++ ){
		PRINTF("\tPID%c : %02x", pidindex[i], regaddr[i] );
		if(i%4==3){
			PRINTF("\r\n");
		}else{
			PRINTF(",");
		}
	}

	regaddr = (volatile UINT32 *)(((UINT32)base)|0x0FF0);
	for( i=0; i < 4; i++ ){
		PRINTF("\tCID%c : %02x", pidindex[i+4], regaddr[i] );
		if(i%4==3){
			PRINTF("\r\n");
		}else{
			PRINTF(",");
		}
	}
}


void cmd_sysinfo_func(int argc, char *argv[])
{
#if		defined(BUILD_OPT_VSIM)
	#define TARGET_RUN_MODEL	"VSIM"
#else	//BUILD_OPT_VSIM
	#define TARGET_RUN_MODEL	"EVB "
#endif	//BUILD_OPT_VSIM

#if		defined(BUILD_OPT_DA16200_FPGA)
	#define TARGET_CHIP_MODEL	"FPGA"
#else	//BUILD_OPT_DA16200_FPGA
	#define TARGET_CHIP_MODEL	"ASIC"
#endif	//BUILD_OPT_DA16200_FPGA

#if		defined(BUILD_OPT_DA16200_ROMALL)
	PRINTF(" ROM-ALL Model (%s, %s)\n", TARGET_RUN_MODEL, TARGET_CHIP_MODEL);
#elif	defined(BUILD_OPT_DA16200_LIBNDK)
	PRINTF(" LIB-NDK Model (%s, %s)\n", TARGET_RUN_MODEL, TARGET_CHIP_MODEL);
#elif	defined(BUILD_OPT_DA16200_LIBALL)
	PRINTF(" LIB-ALL Model (%s, %s)\n", TARGET_RUN_MODEL, TARGET_CHIP_MODEL);
#else	//BUILD_OPT_DA16200_LIBALL
#warn	"Buil option Error"
#endif	//BUILD_OPT_DA16200_LIBALL
	{
		const char *reset_status[]
			= {"POR ", "RTCT", "RTCM", "HDOG", "SDOG", "SRST", "MANU" };

		PRINTF("Reset: %s\nVRUN: %s\n"
			, reset_status[SYSGET_RESET_STATUS()]
			, ((SYSGET_VSIMRUN_TYPE() == BOOT_RX_MODE) ? "RX" : "TX")
			);
	}
#ifdef	BUILD_OPT_DA16200_FPGA
	{
	UINT32	i, j, c;

	const	char *define_rm_fpga_list[]
	= {
		"[ 0] PHY",
		"[ 1] PHY_160MHz",
		"[ 2] CFR",
		"[ 3] DPD",
		"[ 4] TA2_SYNC",
		"[ 5] TS",
		"[ 6] MMC",
		"[ 7] PWM",
		"[ 8] GPIO",
		"[ 9] IP Cores",
		"[10] CC-312",
		"[11] OTP (default)",
		"[12] RF Int Module",
		"[13] Retention Mem",
		"[14] HW BTM",
		"[15] HW CRC-32",
		"[16] HW TCS",
		"[17] HW Zeroing",
		"[18] SYS Test CFG",
		"[19] AuxADC",
		"[20] SPI_I2C_SDIO",
		"[21] I2S",
		"[22] RTC",
		"[23] JTAG Pins",
		"[24] XFC",
		"[25] ZBT",
		"[26] Clock Gating",
		"[27] MPMC",
		"[28] Fast HW CFG",
		"[29] KEVINBAN RWIP Debug Port",
		"[30] KEVINBAN AFTERM3",
		"[31] FDMA",
		"[32] AGC MULTI-MASTER",
		"[33] Delta Delay QSPI",
		"[34] Host Chacha Engine",
		"[35] Host Ghash Engine",
		"[36] CDC Generated CLK",
		"[37] HIF CLK Mux",
		NULL
	};

	const	char *define_en_fpga_list[]
	= {
		"[ 0] Internal SRAM Size Full (1.5MB)",
		"[ 1] Internal SRAM Size 1.0MB (default)",
		"[ 2] Internal SRAM Size 768KB (0.75MB)",
		"[ 3] RW_EMBEDDED_LA",
		"[ 4] MAC_LP_CLK_SWITCH",
		"[ 5] FPGA_DPM_EMULATION",
		"[ 6] FPGA_DPM_EMULATION_RST",
		"[ 7] USED MaskROM 768K_ROM",
		"[ 8] USED MaskROM 192K",
		"[ 9] USED MaskROM 256K",
		"[10] USED MaskROM 384K",
		"[11] USED MaskROM 768K",
		"[12] USED MaskROM 960K",
		"[13] CACHE 256_8WAY",
		"[14] CACHE 256_4WAY",
		"[15] CACHE 256_2WAY",
		"[16] CACHE 128_8WAY",
		"[17] CACHE 128_4WAY",
		"[18] CACHE 128_2WAY",
		"[19] MSKR WE OFF",
		"[20] FPGA_ASIC_EQ RTC AG",
		"[21] FCI MODEM VITERBI",
		"[22] BOOT MODE SRAM",
		"[23] ARM CODEMUX",
		"[24] MAC 1ST Enable",
		"[25] MARTIN UART",
		"[26] FPGA PLL 50MHz",
		"[27] MAC Core 20MHz Fix",
		"[28] MAC Core 40MHz Fix",
		"[29] XTAL CPU Period 18",
		"[30] CPU PIPELINE",
		"[31] CPU CLK_X2",
		"[32] Peripheral CLK_X2",
		"[33] RTC Gated CLK",
		"[34] Debug Port",
		"[35] Debug Port Bus Matrix",
		"[36] Debug Port CLK",
		"[37] Debug Port Bus SYN",
		"[38] Debug GPIO",
		"[39] PHY Debug Port",
		"[40] Test MAC CLK",
		"[41] Test MAC CLK 30MHz",
		"[42] Test PHY Reset",
		"[43] DX FPGA",
		"[44] Reset Auto Test",
		"[45] Set External RFBB",
		NULL
	};
		PRINTF("Removed HW Blocks:\n");
		for( i=0; define_rm_fpga_list[i] != NULL ; i++ ){
			j = i / 32;
			c = i % 32;
			if( (DA16200FPGA_SYSCON->define_rm_fpga[j] & (1<<c)) != 0 ){
				PRINTF("\t%s\n", define_rm_fpga_list[i]);
			}
		}

		PRINTF("Enabled HW Features:\n");
		for( i=0; define_en_fpga_list[i] != NULL ; i++ ){
			j = i / 32;
			c = i % 32;
			if( (DA16200FPGA_SYSCON->define_en_fpga[j] & (1<<c)) != 0 ){
				PRINTF("\t%s\n", define_en_fpga_list[i]);
			}
		}
	}
#endif	//BUILD_OPT_DA16200_FPGA

	PRINTF("JTAG:%02x\n", da16x_sysdbg_capture(TRUE));

	{
		char const *clklist[] = {
			"---.---", "240.000", "160.000", "120.000", " 96.000",
			" 80.000", " 68.571", " 60.000", " 53.333", " 48.000",
			" 43.636", " 40.000", " 36.923", " 34.286", " 32.000",
			" 30.000", " 28.235", " 26.667", " 25.263", " 24.000",
			" 22.857", " 21.818", " 20.870", " 20.000", " 19.200",
			" 18.462", " 17.778", " 17.143", " 16.552", " 16.000",
			" 15.484", " 15.000"
		};
#define PRINTCLOCK(x)	PRINTF("PLL_CLK_DIV_%s\t: %s MHz (%02x), PLL_CLK_EN_%s\t: %02x\n" \
			, #x					\
			, clklist[ (DA16X_SYSCLOCK->PLL_CLK_DIV_ ## x) ]\
			, (DA16X_SYSCLOCK->PLL_CLK_DIV_ ## x) 	\
			, #x					\
			, (DA16X_SYSCLOCK->PLL_CLK_EN_ ## x) 	\
			);

		PRINTF("X2Check: %x\n", da16x_pll_x2check());
		PRINTCLOCK(0_CPU);
		PRINTCLOCK(1_XFC);
		PRINTCLOCK(2_UART);
		PRINTCLOCK(4_PHY);
		PRINTCLOCK(5_I2S);
		PRINTCLOCK(6_AUXA);
		PRINTCLOCK(7_CC312);
	}
}

#endif	//SUPPORT_SYSTEM_INFO

//----------------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------------

void cmd_sysidle_func(int argc, char *argv[])
{
	UINT32	mode;

	if( argc == 3 && DRIVER_STRCMP(argv[1], "set") == 0 ){
		mode = ctoi(argv[2]);
		da16x_idle_set_info( mode );
	}else{
		mode = da16x_idle_get_info( TRUE );
		PRINTF("Sleep Mode : %s\n", ((mode == TRUE)? "ON" : "OFF"));
	}
}

//-----------------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------------

void cmd_sysbtm_func(int argc, char *argv[])
{
	if( argc == 2 && DRIVER_STRCMP(argv[1], "on") == 0 ){
		da16x_btm_control(TRUE);
		system_event_monitor(TRUE);
	}else
	if( argc == 2 && DRIVER_STRCMP(argv[1], "off") == 0 ){
		system_event_monitor(FALSE);
		da16x_btm_control(FALSE);
	}else
	if( argc == 3 && DRIVER_STRCMP(argv[1], "snap") == 0 ){
		if( DRIVER_STRCMP(argv[2], "on") == 0 ){
			system_event_busmon(TRUE, (UINT32)NULL);
		}else{
			system_event_busmon(FALSE, (UINT32)NULL);
		}
	}else
	if( argc == 4 && DRIVER_STRCMP(argv[1], "set") == 0 ){
		UINT32 ch, mode;

		ch = htoi(argv[2]);
		switch(*argv[3]){
			case	'w':	mode = BTM_CHM_WTM ; break;
			case	'c':	mode = BTM_CHM_CMD ; break;
			case	's':	mode = BTM_CHM_SLP ; break;
			default:	mode = BTM_CHM_IDL ; break;
		}
		da16x_btm_ch_setup(ch, mode);
	}else
	if( argc == 4 && DRIVER_STRCMP(argv[1], "sel") == 0 ){
		UINT32 ch, mode;

		ch = htoi(argv[2]);
		mode = htoi(argv[3]);
		da16x_btm_ch_select(ch, mode);
	}else
	if( argc == 3 && DRIVER_STRCMP(argv[1], "usage") == 0 ){

		UINT32 para = ctoi(argv[2]);

		da16x_btm_ch_select(0, 0x10); /* CPU : CM */
		da16x_btm_ch_setup( 0, BTM_CHM_CMD);
		da16x_btm_ch_select(1, 0x10); /* CPU : WM */
		da16x_btm_ch_setup( 1, BTM_CHM_WTM);
		da16x_btm_ch_select(2, 0x11); /* MAC : CM */
		da16x_btm_ch_setup( 2, BTM_CHM_CMD);
		da16x_btm_ch_select(3, 0x11); /* MAC : WM */
		da16x_btm_ch_setup( 3, BTM_CHM_WTM);
		//da16x_btm_ch_select(4, 0x19); /* ISP : CM */
		//da16x_btm_ch_setup( 4, BTM_CHM_CMD);
		da16x_btm_ch_select(4, 0x00); /* SRAM.P0 : CM */
		da16x_btm_ch_setup( 4, BTM_CHM_CMD);
		da16x_btm_ch_select(5, 0x10); /* CPU : SM */
		da16x_btm_ch_setup( 5, BTM_CHM_SLP);

		if( para == 0 ){
			system_event_cpumon(FALSE, para);
		}else{
			system_event_cpumon(TRUE, para);
		}
	}
}

/******************************************************************************
 *  cmd_oops_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void cmd_oops_func(int argc, char *argv[])
{
	UINT32	oopsinfo;
	OOPS_CONTROL_TYPE *poopsctrl;

	oopsinfo = da16x_boot_get_offset(BOOT_IDX_OOPS);
	poopsctrl = (OOPS_CONTROL_TYPE *)&oopsinfo;

	if( argc == 1 ){
		PRINTF("Active = %d\n"	, poopsctrl->active );
		PRINTF("boot = %d\n"	, poopsctrl->boot );
		PRINTF("stop = %d\n"	, poopsctrl->stop );
		PRINTF("dump = %d\n"	, poopsctrl->dump );
		PRINTF("save = %d\n"	, poopsctrl->save );

		PRINTF("MEM  = %s\n"	, ((poopsctrl->memtype == 1) ? "nor" : "sflash") );
		PRINTF("maxnum = %d\n"	, ((1<<(poopsctrl->secsize)) * (1<<(poopsctrl->maxnum))) );
		PRINTF("secsize = %d\n"	, (1<<(poopsctrl->secsize)) );
		PRINTF("offset = %d\n"	, ((1<<(poopsctrl->secsize)) * (poopsctrl->offset)) );

		da16x_environ_lock(TRUE);
		da16x_oops_view( poopsctrl );
		da16x_environ_lock(FALSE);

		{
			UINT32 saddr, curr, len;

			len = da16x_sysdbg_dbgt_dump( &saddr, &curr );

			PRINTF("DBGT s %08x / l %08x / c %08x / m %x \n"
				, saddr, (len & 0x000fffff) , curr, ((len >> 24) & 0x0ff) );
		}
	}else
	if( argc == 2 && DRV_STRCMP("clear", argv[1]) == 0 ){
		da16x_environ_lock(TRUE);
		da16x_oops_clear( poopsctrl );
		da16x_environ_lock(FALSE);
	}else
	if( argc == 2 && DRV_STRCMP("on", argv[1]) == 0 ){
		OOPS_CONTROL_TYPE oopsctrl;
#ifdef	BUILD_OPT_DA16200_ASIC
		oopsctrl.active  = 1;
		oopsctrl.boot 	 = 1;
		oopsctrl.stop 	 = 1;
		oopsctrl.dump 	 = 1;
		oopsctrl.save    = 1;
		oopsctrl.maxnum  = 2;	// 2^2 = 4 Sectors
		oopsctrl.secsize = 12;  // 2^12 = 4096 Bytes
		oopsctrl.offset  = 896; // 3.5MB
		oopsctrl.memtype = 0;	// sflash
#else	//BUILD_OPT_DA16200_ASIC
		oopsctrl.active  = 1;
		oopsctrl.boot 	 = 1;
		oopsctrl.stop 	 = 1;
		oopsctrl.dump 	 = 1;
		oopsctrl.save    = 1;
		oopsctrl.maxnum  = 1;	// 2^1 = 2 Sectors
		oopsctrl.secsize = 16;  // 2^16 = 64 KB
		oopsctrl.offset  = 64;  // 4MB
		oopsctrl.memtype = 1;	// nor
#endif	//BUILD_OPT_DA16200_ASIC
		da16x_environ_lock(TRUE);
		da16x_boot_set_lock(FALSE);
		da16x_oops_init( poopsctrl, &oopsctrl );
		da16x_boot_set_offset(BOOT_IDX_OOPS, *((UINT32 *)poopsctrl));
		da16x_boot_set_lock(TRUE);
		da16x_environ_lock(FALSE);
	}else
	if( argc == 9 ){
		OOPS_CONTROL_TYPE oopsctrl;

		oopsctrl.active  = ctoi(argv[1])&0x1;
		oopsctrl.boot 	 = ctoi(argv[2])&0x1;
		oopsctrl.stop 	 = ctoi(argv[3])&0x1;
		oopsctrl.dump 	 = 1;
		oopsctrl.save    = ctoi(argv[4])&0x1;
		oopsctrl.maxnum  = ctoi(argv[5])&0xf; // 2^2 = 4 Sectors
		oopsctrl.secsize = ctoi(argv[6])&0x7f; // 2^12 = 4096 Bytes
		oopsctrl.offset  = ctoi(argv[7])&0x7fff;
		oopsctrl.memtype = ctoi(argv[8])&0x1; // 1 : nor , 0 : sflash

		// 1 1 1 1  1 /*2sectors*/ 16 /*64KB*/ 64 /*4MB*/ 1 /* nor */
		// 1 1 1 1  2 /*4sectors*/ 12 /*4KB*/ 512 /*2MB*/ 0 /* sflash */

		da16x_environ_lock(TRUE);
		da16x_boot_set_lock(FALSE);
		da16x_oops_init( poopsctrl, &oopsctrl );
		da16x_boot_set_offset(BOOT_IDX_OOPS, *((UINT32 *)poopsctrl));
		da16x_boot_set_lock(TRUE);
		da16x_environ_lock(FALSE);
	}
}

/******************************************************************************
 *  cmd_bmconfig_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32	bmcfg_get_bootmode(char *mem)
{
	UINT32 bmode, i;
	const char *bootmode[] = { "RTC", "GPIO", "POR", "TAG", "RLIB", NULL };

	bmode = BOOT_IDX_TAG;
	for(i=0; bootmode[i] != NULL; i++ ){
		if( DRV_STRCMP(bootmode[i], mem) == 0 ){
			bmode = i;
			break;
		}
	}

	return bmode;
}

void cmd_bmconfig_func(int argc, char *argv[])
{
	UINT32	bmode, bootcfg, bmlock, i;
	const char *bootmem[16] = {
		"ROM", "SRAM", "RTM", "SFLH", "OTP", NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, "NOR", "ZBT", "XXX" };

	if( argc == 3 && DRV_STRCMP("lock", argv[1]) == 0 ){
		bmode = ctoi(argv[2]);
		da16x_boot_set_lock(bmode);
	}
	else
	if( argc == 3 && DRV_STRCMP("get", argv[1]) == 0 ){
		bmode = bmcfg_get_bootmode( argv[2] );

		if( bmode == BOOT_IDX_TAG ){
			PRINTF("unknown boot mode : %s\n", argv[2]);
			return ;
		}

		bootcfg = da16x_boot_get_offset(bmode);
		bmlock = da16x_boot_get_offset(BOOT_IDX_TAG);

		PRINTF("BootMode get - %s : %s %08x/ tag %x\n"
			, argv[2]
			, bootmem[BOOT_MEM_GET(bootcfg)]
			, BOOT_OFFSET_GET(bootcfg)
			, bmlock
			);
	}
	else
	if( argc == 5 && DRV_STRCMP("set", argv[1]) == 0 ){
		bmode = bmcfg_get_bootmode( argv[2] );

		if( bmode == BOOT_IDX_TAG ){
			PRINTF("unknown boot mode : %s\n", argv[2]);
			return ;
		}

		bootcfg = 0;
		for(i=0; i<16; i++ ){
			if( bootmem[i] != NULL && DRV_STRCMP(bootmem[i], argv[3]) == 0 ){
				bootcfg = BOOT_MEM_SET(i);
				break;
			}
		}

		if( i == 16 ){
			PRINTF("unknown memory : (%s)\n", argv[3]);
			return ;
		}

		bootcfg |= BOOT_OFFSET_SET( htoi(argv[4]) );

		PRINTF("BootMode set - %s : %s %08x\n"
			, argv[2]
			, bootmem[BOOT_MEM_GET(bootcfg)]
			, BOOT_OFFSET_GET(bootcfg)
			);

		da16x_boot_set_offset(bmode, bootcfg);
	}
	else
	{
		const char *bmhelp = "BootMode config\n"
			"\t lock [0|1] \n"
			"\t get [bmode] \n"
			"\t set [bmode] [mem] [offset]\n"
			"\t bmode := RTC|GPIO|POR|RLIB\n"
			"\t mem   := ROM|SRAM|RTM|SFLH|OTP|NOR|ZBT|XXX\n" ;

		PRINTF(bmhelp);
	}
}

/******************************************************************************
 *  cmd_cache_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_cache_func(int argc, char *argv[])
{

	if (argc == 1) {
		if (da16x_cache_status() == TRUE) {
			UINT64	hitcount, misscount;

			hitcount = (UINT64)da16x_cache_get_hitcount();
			misscount = (UINT64)da16x_cache_get_misscount();

			if( hitcount == 0x0FFFFFFFFUL ){
				PRINTF("CACHE: hit count is saturated !!\n");
			}

			PRINTF("CACHE:hit %lld, miss %lld = miss ratio %lld ppm\n"
				, hitcount, misscount
				, ((misscount * 1000000UL) / (hitcount + misscount)) );
		}
		else {
			PRINTF("CACHE:inactive\n");
		}
	}
	else if(argc > 1) {
		if(!strcmp(argv[1], "on")) {
			da16x_cache_enable(FALSE);
			PRINTF("CACHE ON\n");
		}
		else if(!strcmp(argv[1], "off")) {
			da16x_cache_disable();
			PRINTF("CACHE OFF\n");
		}
	}
}

#if defined ( WATCH_DOG_ENABLE )
void	cmd_wdog_func(int argc, char *argv[])
{
	extern int wdog_off;
	WDOG_HANDLER_TYPE *wdog;

	wdog = WDOG_CREATE(WDOG_UNIT_0);
	WDOG_INIT(wdog);
	if(argc == 1) {
		int status;
		WDOG_IOCTL(wdog, WDOG_GET_STATUS, &status);
		PRINTF("WATCDOG status: %s\n", status?"on":"off");

		PRINTF("    wdog feature <<Enabled>>\n\n");
		PRINTF("    Usage : wdog [option]\n");
		PRINTF("     option\n");
		PRINTF("\t [on | off]               : wdog on or off \n");
		PRINTF("\t [kick_start | kick_stop] : wdog kicking start or stop \n");
		PRINTF("\t [status]                 : View wdog status \n\n");
	}
	else if (argc > 1) {
		if(!strcmp(argv[1], "on")) {
			WDOG_IOCTL(wdog, WDOG_SET_ON, NULL );
			PRINTF("WATCHDOG on\n");
		}
		else if(!strcmp(argv[1], "off")) {
			WDOG_IOCTL(wdog, WDOG_SET_OFF, NULL );
			PRINTF("WATCHDOG off\n");
		}
		else if(!strcmp(argv[1], "kick_start")) {
			watch_dog_kick_start();
			PRINTF("WATCHDOG kick_start\n");
		}
		else if(!strcmp(argv[1], "kick_stop")) {
			watch_dog_kick_stop();
			PRINTF("WATCHDOG kick_stop\n");
		}
		else if(!strcmp(argv[1], "status")) {
			PRINTF("WATCHDOG status: %s \n", ((wdog_off==1) ? "off" : "on"));
			PRINTF("WATCHDOG timeout value: %d \n", wdog->setvalue);
		}
	}
	WDOG_CLOSE(wdog);
}
#endif	// WATCH_DOG_ENABLE

#ifdef	SUPPORT_DOWNLOAD_TEST
/******************************************************************************
 *  cmd_image_store_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	IMAGE_BUFF_SIZE		1024
#define	IMAGE_BUFF_NUM		5

void	cmd_image_store_cmd(int argc, char *argv[])
{
	UINT8	*imghdr_offset;
	UINT32	mode;
	UINT32	store_addr, sec_size;
	UINT32	sflash;

#ifdef	BUILD_OPT_DA16200_FPGA
	if(DRIVER_STRCMP(argv[0], "loadxn") == 0){
		mode = FALSE;	// X-Modem
		sflash = FALSE;
	}else
	if(DRIVER_STRCMP(argv[0], "loadyn") == 0){
		mode = TRUE;	// Y-Modem
		sflash = FALSE;
	}else
	if(DRIVER_STRCMP(argv[0], "floadyn") == 0){
		mode = TRUE|0x40;	//Fast Y-Modem
		sflash = FALSE;
	}else
#endif	//BUILD_OPT_DA16200_FPGA
	if(DRIVER_STRCMP(argv[0], "loady") == 0){
		mode = TRUE;	// Y-Modem
		sflash = TRUE;
	}else
	if(DRIVER_STRCMP(argv[0], "floady") == 0){
		mode = TRUE|0x40;	//Fast Y-Modem
		sflash = TRUE;
	}else
	{
		mode = TRUE;	// Y-Modem
		sflash = FALSE;
	}

	if( argc >= 2 ){
		if(DRIVER_STRCMP(argv[1], "boot") == 0){
			store_addr = 0 ;
		}else{
			store_addr = htoi(argv[1]);
		}

		imghdr_offset = (UINT8 *)APP_MALLOC(IMAGE_BUFF_SIZE*IMAGE_BUFF_NUM) ;

		if( argc >= 3 ){
			sec_size = htoi(argv[2]);
		}else{
			if( sflash == FALSE ){
				sec_size = 128*1024 ;
			}else{
#ifdef	BUILD_OPT_DA16200_FPGA
				sec_size = 64*1024 ;
#else	//BUILD_OPT_DA16200_FPGA
				sec_size = 4*1024 ;
#endif	//BUILD_OPT_DA16200_FPGA
			}
		}

		if( (argc >= 4) && (DRIVER_STRCMP(argv[3], "bin") == 0)){
			mode = mode | 0x80; // binary format
		}else
		if( (argc >= 4) && (DRIVER_STRCMP(argv[3], "img") == 0)){
			// old IMG format
		}else
		{
			mode = mode | 0x100; // SBROM format
		}

		//=================================================
		// XIP/Cache
		//=================================================
#ifdef	BUILD_OPT_DA16200_FPGA
		if( (ROM_MEM_BASE() >= DA16X_NOR_XIP_BASE) && (ROM_MEM_BASE() <= DA16X_NOR_XIP_END) ){
			mode = mode | 0x80000000 ;
		}else
		if( (ROM_MEM_BASE() >= DA16X_NOR_CACHE_BASE) && (ROM_MEM_BASE() <= DA16X_NOR_CACHE_END) ){
			mode = mode | 0x80000000 ;
		}else
#endif	//BUILD_OPT_DA16200_FPGA
//da16200removed:		if( (ROM_MEM_BASE() >= DA16X_XIP_BASE) && (ROM_MEM_BASE() <= DA16X_XIP_END) ){
//da16200removed:			mode = mode | 0x80000000 ;
//da16200removed:		}else
		if( (ROM_MEM_BASE() >= DA16X_CACHE_BASE) && (ROM_MEM_BASE() <= DA16X_CACHE_END) ){
			mode = mode | 0x80000000 ;
		}

		PRINTF("Load Addr: %p\n", imghdr_offset);
		PRINTF("To cancel a session, press Ctrl+X");

		trc_que_proc_conmode( TRACE_MODE_ON );
#if	defined(SUPPORT_NOR_DEVICE)
		if( sflash == FALSE ){
			xymodem_nor_store(mode, (UINT32)imghdr_offset
					, IMAGE_BUFF_SIZE, IMAGE_BUFF_NUM
					, sec_size, store_addr );
		}
		else{
			xymodem_sflash_store(mode, (UINT32)imghdr_offset
					, IMAGE_BUFF_SIZE, IMAGE_BUFF_NUM
					, sec_size, store_addr
					, (VOID *)&da16x_environ_lock
					);
		}
#else	//defined(SUPPORT_NOR_DEVICE)
		xymodem_sflash_store(mode, (UINT32)imghdr_offset
				, IMAGE_BUFF_SIZE, IMAGE_BUFF_NUM
				, sec_size, store_addr
				, (VOID *)&da16x_environ_lock
				);
#endif	//defined(SUPPORT_NOR_DEVICE)
		trc_que_proc_conmode( TRACE_MODE_ON|TRACE_MODE_ECHO );

		APP_FREE(imghdr_offset);
	}

}


/******************************************************************************
 *  cmd_rs485_image_store_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#define	RS485_IMAGE_BUFF_NUM		8
void cmd_rs485_image_store_cmd(int argc, char *argv[]) {
	UINT8 *imghdr_offset;
	UINT32 mode;
	UINT32 store_addr, sec_size;
	UINT8 uid;

	if (argc != 3) {
		PRINTF("rs485 [offset] [id]\n");
		return;
	}

	if(DRIVER_STRCMP(argv[1], "boot") == 0){
		store_addr = 0 ;
	}else{
		store_addr = htoi(argv[1]);
	}

	uid = htoi(argv[2]);
	mode = TRUE;

	imghdr_offset = (UINT8 *) APP_MALLOC(
			IMAGE_BUFF_SIZE * RS485_IMAGE_BUFF_NUM);
#ifdef	BUILD_OPT_DA16200_FPGA
	sec_size = 64 * 1024;
#else	//BUILD_OPT_DA16200_FPGA
	sec_size = 4*1024;
#endif	//BUILD_OPT_DA16200_FPGA

	mode = mode | 0x100; // SBROM format

	//=================================================
	// XIP/Cache
	//=================================================
#ifdef	BUILD_OPT_DA16200_FPGA
	if ((ROM_MEM_BASE() >= DA16X_NOR_XIP_BASE)
			&& (ROM_MEM_BASE() <= DA16X_NOR_XIP_END)) {
		mode = mode | 0x80000000;
	} else if ((ROM_MEM_BASE() >= DA16X_NOR_CACHE_BASE)
			&& (ROM_MEM_BASE() <= DA16X_NOR_CACHE_END)) {
		mode = mode | 0x80000000;
	} else
#endif	//BUILD_OPT_DA16200_FPGA
//da16200removed:	if ((ROM_MEM_BASE() >= DA16X_XIP_BASE) && (ROM_MEM_BASE() <= DA16X_XIP_END)) {
//da16200removed:		mode = mode | 0x80000000;
//da16200removed:	} else
	if ((ROM_MEM_BASE() >= DA16X_CACHE_BASE)
			&& (ROM_MEM_BASE() <= DA16X_CACHE_END)) {
		mode = mode | 0x80000000;
	}

	PRINTF("Load Addr: %p\n", imghdr_offset);
	rs485_sflash_store(mode, (UINT32) imghdr_offset, IMAGE_BUFF_SIZE,
			RS485_IMAGE_BUFF_NUM, sec_size, store_addr,
			(VOID *) &da16x_environ_lock, uid);
	APP_FREE(imghdr_offset);
}


/******************************************************************************
 *  cmd_image_load_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_image_load_cmd(int argc, char *argv[])
{
	UINT32 mode ;
	UINT32	img_offset, img_siz;


	if(DRIVER_STRCMP(argv[0], "xmodem") == 0){
		mode = FALSE;	// X-Modem
	}else{
		mode = TRUE;	// Y-Modem
	}

	if( argc == 2 && DRIVER_STRCMP(argv[1],"sfdp") == 0 ){
		img_offset = RETMEM_SFLASH_BASE;
		img_siz = (RETMEM_LMAC_BASE - RETMEM_SFLASH_BASE + 1);
	}else
	if( argc >= 3 ){
		img_offset = htoi(argv[1]);
		img_siz = htoi(argv[2]);
	}else{
		return;
	}

	PRINTF("Load Addr: %x, Size: %d \n", img_offset, img_siz );
	PRINTF("To cancel a session, press Ctrl+X");

	xymodem_ram_store(mode, img_offset, img_siz );
}

/******************************************************************************
 *  cmd_image_boot_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_image_boot_cmd(int argc, char *argv[])
{
	HANDLE	handler;
	UINT32	sflash, status, skip, bussel;
	volatile UINT32	store_addr, load_addr, jmp_addr;

#ifdef	BUILD_OPT_DA16200_FPGA
	if(DRIVER_STRCMP(argv[0], "bootn") == 0){
		sflash = FALSE;
		skip = 0;
	}else
#endif	//BUILD_OPT_DA16200_FPGA
	if(DRIVER_STRCMP(argv[0], "boots") == 0){
		sflash = TRUE;
		skip = 0; //Secure Test
	}else
	{	// "bootu" or "boot"?
		sflash = TRUE;
		skip = 1; // Default Unsecure
	}

#ifdef	BUILD_OPT_DA16200_ROMALL
	if( argc > 1 ){
		store_addr = htoi(argv[1]);
	}else{
		store_addr = 0;
	}

	if( argc > 2 ){
		bussel = htoi(argv[2]);
	}else{
		bussel = 0;
	}

	if( (skip == 1) && (argc > 3) ){
		skip = htoi(argv[2]);
	}

	// TODO: Protect Unsecure Boot mode !! 2019.02.08
	// this command can be only executed at MROM prompt mode.
	// so, this command will be run after completing the previous step,
	// rom_boot_check_normalboot( ) in MROM Boot process.
	// therefore, nothing is required at this stage to protect DCU-LOCK and OTP keys.

#ifdef	BUILD_OPT_DA16200_FPGA
	if( (sflash == FALSE) && ((status = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_HEADER), (DA16X_NOR_BASE|store_addr), (UINT32 *)&load_addr, (UINT32 *)&jmp_addr)) > 0) )
	{
		store_addr |= DA16X_NOR_BASE;

		if( (load_addr < DA16X_SRAM_END) && ((load_addr+status) >= DA16X_SRAM_END) ){
			// Image size exceeds a loadable limit !!
			PRINTF("Image size exceeds a loadable space (%d @ %x)\n", status, load_addr );
		}
		else
		{
			PRINTF("Load Addr: %x, JMP Addr:%x\n", load_addr, jmp_addr );
			if( load_addr > DA16X_SRAM_END ){
				load_addr = 0;
			}
			if( nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_RUN), store_addr, (UINT32 *)&load_addr, (UINT32 *)&jmp_addr) > 0 )
			{ /* NOTICE !! do not modify this clause !! */
				volatile UINT32	dst_addr;

				if( (jmp_addr >= DA16X_NOR_XIP_BASE) && (jmp_addr <= DA16X_NOR_XIP_END) ){
					DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // XIP Boot
				}else
				if( (jmp_addr >= DA16X_NOR_CACHE_BASE) && (jmp_addr <= DA16X_NOR_CACHE_END) ){
					DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_CACHE); // CACHE Boot
					da16x_cache_enable(FALSE);
				}else{
					DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // FLASH Boot
				}

				/* stop Scheduler */
				_sys_tick_close( );

				/* get Reset handler address */
				dst_addr = *((UINT32 *)(jmp_addr|0x04));
				/* Jump into App */
				dst_addr = (dst_addr|0x01);
				ASM_BRANCH(dst_addr);
			}
		}
	}
	else
#endif	//BUILD_OPT_DA16200_FPGA
	if( skip == 0 ){

		if( argc > 2 ){
			da16x_sflash_set_bussel(bussel);
		}

		da16x_cache_disable();
		DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
		da16x_cache_enable(FALSE);

		// SFLASH Test
		da16x_sflash_set_image( 0, 0, 0 ); // Clear

		DA16X_Debug_SecureBoot(1);
		{
			UINT32 tjmpaddr = 0;
			DA16X_SecureBoot( BOOT_OFFSET_GET(store_addr), &tjmpaddr, NULL);
		}
	}
	else
	{
		UINT32 sbootmode;

		if( argc > 2 ){
			da16x_sflash_set_bussel(bussel);
		}

		// SFLASH Test
		da16x_sflash_set_image( 0, 0, 0 ); // Clear

		if( skip > 1 ){
			sbootmode = (DA16X_IMG_BOOT|DA16X_IMG_SKIP|(sizeof(UINT32)*8));
		}else{
			sbootmode = (DA16X_IMG_BOOT|(sizeof(UINT32)*8));
		}

		handler = flash_image_open( sbootmode, BOOT_OFFSET_GET(store_addr), NULL );

		if( flash_image_check(handler
			, 0
			, BOOT_OFFSET_GET(store_addr) ) > 0 )
		{
			flash_image_extract(handler
				, BOOT_OFFSET_GET(store_addr)
				, (UINT32 *)&load_addr, (UINT32 *)&jmp_addr
			);

			if( flash_image_load(handler
				, store_addr
				, (UINT32 *)&load_addr, (UINT32 *)&jmp_addr) > 0 ){
				volatile UINT32	dst_addr;

				PRINTF("Load Addr: %x, JMP Addr:%x\n", load_addr, jmp_addr );
				flash_image_close(handler);

//da16200removed:				if( (jmp_addr >= DA16X_XIP_BASE) && (jmp_addr <= DA16X_XIP_END) ){
//da16200removed:					DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP); // XIP Boot
//da16200removed:				}else
				if( (jmp_addr >= DA16X_CACHE_BASE) && (jmp_addr <= DA16X_CACHE_END) ){
					DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
					da16x_cache_enable(FALSE);
				}else{
					DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP); // FLASH Boot
				}

				/* stop Scheduler */
				_sys_tick_close( );

				/* get Reset handler address */
				dst_addr = *((UINT32 *)(jmp_addr|0x04));
				/* Jump into App */
				dst_addr = (dst_addr|0x01);
				ASM_BRANCH(dst_addr);
			}
		}

		flash_image_close(handler);
	}
#else	//BUILD_OPT_DA16200_ROMALL
	PRINTF("boot command is supported only for ROM-ALL Model\n");
#endif	//BUILD_OPT_DA16200_ROMALL
}

#endif	/*SUPPORT_DOWNLOAD_TEST*/

#if defined(SUPPORT_SBROM_TEST)
/******************************************************************************
 *  cmd_sbrom_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#if	!defined(BUILD_OPT_DA16200_ROMALL)
static UINT32 sbrom_sflash_read(UINT32 address, UINT8 *mem_read_data, UINT32 data_length)
{
#ifdef	SUPPORT_SFLASH_DEVICE
	UINT32	ioctldata[7];
	HANDLE	sflash;
	UINT32	tbusmode, status;

	status = FALSE;
	da16x_environ_lock(TRUE);

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);

	if(sflash != NULL){

		ioctldata[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, ioctldata);

		if( SFLASH_INIT(sflash) == TRUE ){
			// to prevent a reinitialization
			if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
				SFLASH_IOCTL(sflash, SFLASH_SET_INFO, ioctldata);
			}
		}

		SFLASH_IOCTL(sflash, SFLASH_CMD_WAKEUP, ioctldata);
		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			OAL_MSLEEP( ioctldata[0] );
		}

		tbusmode = SFLASH_BUS_3BADDR | SFLASH_BUS_144;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

		SFLASH_READ(sflash, address, mem_read_data, data_length );

		SFLASH_CLOSE(sflash);

		status = TRUE;
	}

	da16x_environ_lock(FALSE);

	return status;
#else	//SUPPORT_SFLASH_DEVICE
	return FALSE;
#endif	//SUPPORT_SFLASH_DEVICE
}

static UINT32 sbrom_sflash_erase_write(UINT32 address, UINT8 *mem_write_data, UINT32 data_length, UINT32 wrmode)
{
#ifdef	SUPPORT_SFLASH_DEVICE
	UINT32	ioctldata[7];
	HANDLE	sflash;
	UINT32	tbusmode, busmode, status;

	status = FALSE;
	da16x_environ_lock(TRUE);

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);

	if(sflash != NULL){

		ioctldata[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, ioctldata);

		if( SFLASH_INIT(sflash) == TRUE ){
			// to prevent a reinitialization
			if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
				SFLASH_IOCTL(sflash, SFLASH_SET_INFO, ioctldata);
			}
		}

		SFLASH_IOCTL(sflash, SFLASH_CMD_WAKEUP, ioctldata);
		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			OAL_MSLEEP( ioctldata[0] );
		}

		tbusmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &tbusmode);

		ioctldata[0] = address;
		ioctldata[1] = data_length;
		SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata );

		ioctldata[0] = address;
		ioctldata[1] = data_length;
		SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata );

		if( wrmode == TRUE ){
			SFLASH_WRITE(sflash, address, mem_write_data, data_length );
		}

		busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_144;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, &busmode);

		SFLASH_CLOSE(sflash);

		status = TRUE;
	}

	da16x_environ_lock(FALSE);

	return status;
#else	//SUPPORT_SFLASH_DEVICE
	return FALSE;
#endif	//SUPPORT_SFLASH_DEVICE
}

#if	!defined(BUILD_OPT_DA16200_ROMALL)
#ifndef __REDUCE_CODE_SIZE__
static UINT32 sbrom_sflash_write(UINT32 address, UINT8 *mem_write_data, UINT32 data_length)
{
	if( mem_write_data == NULL || data_length == 0 ){
		return FALSE;
	}
	return sbrom_sflash_erase_write(address, mem_write_data, data_length, TRUE);
}
#endif // __REDUCE_CODE_SIZE__
#endif //defined(BUILD_OPT_DA16200_ROMALL)

static UINT32 sbrom_sflash_erase(UINT32 address, UINT32 data_length)
{
	if( data_length == 0 ){
		return FALSE;
	}
	return sbrom_sflash_erase_write(address, NULL, data_length, FALSE);
}


#endif	//!defined(BUILD_OPT_FC9100_ROMALL)
#endif //defined(SUPPORT_SBROM_TEST)

#ifdef SUPPORT_SBROM_TEST

#undef	SUPPORT_SBROM_CHECK

#ifdef	SUPPORT_SBROM_CHECK
typedef		struct	{
	char	*regname;
	UINT32	regaddr;
	UINT32	regcheck[1];
} SBROM_DEBUG_TYPE;

static const SBROM_DEBUG_TYPE sbrom_dbgitem_list[] = {	// no-check
 	{ "AO.DCU_EN0    " , (DA16X_ACRYPT_BASE|0x1e00), {0xA5A5A5A5UL} },
	{ "AO.DCU_EN1    " , (DA16X_ACRYPT_BASE|0x1e04), {0xA5A5A5A5UL} },
	{ "AO.DCU_EN2    " , (DA16X_ACRYPT_BASE|0x1e08), {0xA5A5A5A5UL} },
	{ "AO.DCU_EN3    " , (DA16X_ACRYPT_BASE|0x1e0c), {0xA5A5A5A5UL} },
	{ "AO.DCU_LOCK0  " , (DA16X_ACRYPT_BASE|0x1e10), {0xA5A5A5A5UL} },
	{ "AO.DCU_LOCK1  " , (DA16X_ACRYPT_BASE|0x1e14), {0xA5A5A5A5UL} },
	{ "AO.DCU_LOCK2  " , (DA16X_ACRYPT_BASE|0x1e18), {0xA5A5A5A5UL} },
	{ "AO.DCU_LOCK3  " , (DA16X_ACRYPT_BASE|0x1e1c), {0xA5A5A5A5UL} },
	{ "AO.DCU_MSK0   " , (DA16X_ACRYPT_BASE|0x1e20), {0xA5A5A5A5UL} },
	{ "AO.DCU_MSK1   " , (DA16X_ACRYPT_BASE|0x1e24), {0xA5A5A5A5UL} },
	{ "AO.DCU_MSK2   " , (DA16X_ACRYPT_BASE|0x1e28), {0xA5A5A5A5UL} },
	{ "AO.DCU_MSK3   " , (DA16X_ACRYPT_BASE|0x1e2c), {0xA5A5A5A5UL} },
	{ "AO.SEC_DBG_RST" , (DA16X_ACRYPT_BASE|0x1e30), {0xA5A5A5A5UL} },
	{ "AO.LOCK_BITS  " , (DA16X_ACRYPT_BASE|0x1e34), {0xA5A5A5A5UL} },
	{ "AO.APB_FILTER " , (DA16X_ACRYPT_BASE|0x1e38), {0xA5A5A5A5UL} },
	{ "AO.GPPC       " , (DA16X_ACRYPT_BASE|0x1e3c), {0xA5A5A5A5UL} },
	{ "NVM.FUSE_PROG " , (DA16X_ACRYPT_BASE|0x1f04), {0xA5A5A5A5UL} },
	{ "NVM.DBG_STAT  " , (DA16X_ACRYPT_BASE|0x1f08), {0xA5A5A5A5UL} },
	{ "NVM.LCS_VALID " , (DA16X_ACRYPT_BASE|0x1f0c), {0xA5A5A5A5UL} },
	{ "NVM.IS_IDLE   " , (DA16X_ACRYPT_BASE|0x1f10), {0xA5A5A5A5UL} },
	{ "NVM.LCS_REG   " , (DA16X_ACRYPT_BASE|0x1f14), {0xA5A5A5A5UL} },
	{ "NVM.OTP_AWIDTH" , (DA16X_ACRYPT_BASE|0x1f2c), {0xA5A5A5A5UL} },
	{ NULL   	, (0), 0x0 }
};

static void sbrom_otp_check(UINT32 baseaddr)
{
	UINT32 idx;
	volatile UINT32 *otpaddr = (UINT32 *)(baseaddr) ;

	idx = 0 ;
	PRINTF("\nOTP Secure Area ===============");
	PRINTF("\nHUK      [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x08; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nKpicv    [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x0C; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nKceicv   [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x10; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nICV-prog [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x11; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nHBK0     [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x15; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nHBK1     [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x19; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nKcp      [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x1D; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nKce      [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x21; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nOEM-prog [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x22; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }

	PRINTF("\nICV-NVC  [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x24; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nOEM-NVC  [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x27; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\nGPPC     [%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x28; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }

	PRINTF("\nOTP-DLock[%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x2C; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }

	PRINTF("\nOTP-SLock[%p]: ", &(otpaddr[idx]));
	for( ; idx < 0x2D; idx++ ){ PRINTF("%08x ", otpaddr[idx]); }
	PRINTF("\n");

}

static UINT32 sbrom_dbgitem_check(UINT32 idx)
{
	UINT32 i, status, regval, regstat;

	status = TRUE;
	for( i = 0; sbrom_dbgitem_list[i].regname != NULL; i++ ){
		volatile UINT32 *regaddr ;

		regaddr = (volatile UINT32 *)(sbrom_dbgitem_list[i].regaddr);
		regval = *regaddr;

		regstat = TRUE;
		if( sbrom_dbgitem_list[i].regcheck[idx] != 0xA5A5A5A5UL ){
			if( regval != sbrom_dbgitem_list[i].regcheck[idx] ){
				regstat = FALSE;
			}
		}

		if( regstat == FALSE ){
			PRINTF("%s [%p]: %08x [exp %08x] err\n"
				, sbrom_dbgitem_list[i].regname
				, regaddr
				, regval
				, sbrom_dbgitem_list[i].regcheck[idx]
			);
		}else{
			PRINTF("%s [%p]: %08x \n"
				, sbrom_dbgitem_list[i].regname
				, regaddr
				, regval
			);
		}

		status = ( regstat == FALSE )? FALSE : status ;
	}

	// OTP Normal Path
	sbrom_otp_check((DA16X_ACRYPT_BASE|0x02000));
	// OTP Debug Path
	sbrom_otp_check((DA16X_ACRYPT_BASE|0x22000));

	return status;
}
#endif	//SUPPORT_SBROM_CHECK

void cmd_sbrom_func(int argc, char *argv[])
{
	UINT32  dst_addr;

	/* OTP dummy read */
	otp_mem_create();
	otp_mem_read(0x700, &dst_addr);
	otp_mem_close();

	if( DRIVER_STRCMP("socid", argv[0]) == 0 ){
		DA16X_SecureSocID();
#ifdef	SUPPORT_SBROM_CHECK
		sbrom_dbgitem_check(0);
#endif	//SUPPORT_SBROM_CHECK
		return;
	}

	if( DRIVER_STRCMP("srng", argv[0]) == 0 ){
		int i, errcnt;
		unsigned int a, b, c;
		UINT32 oldstamp, curstamp, peakstamp;
		
		unsigned int rsize;
		unsigned int *rbox;

		extern int CC_TST_TRNG(    unsigned long regBaseAddress,
                    unsigned int TRNGMode,   // Fast:0, BSI AIS-31:1 , NIST SP 800-90B:2
                    unsigned int roscLength, // 0 ~ 3
                    unsigned int sampleCount, // 
                    unsigned int buffSize,    // N*sizeof(unsigned int), min N=13
                    unsigned int *dataBuff_ptr);


		a = ctoi(argv[1]);
		b = ctoi(argv[2]);
		c = ctoi(argv[3]);
		rsize = 32*sizeof(unsigned int); // min 13*sizeof(unsigned int)

		PRINTF("TRNGMode(%d), roscLength(%d), sampleCount(%d)\n", a,b,c);

		rbox  = (unsigned int *)CRYPTO_MALLOC(rsize);
		errcnt = 0;
		peakstamp = 0;
		
		for(i =0; i < 100; i++ ){
			oldstamp = da16x_rtc_get_mirror32(FALSE);
			
			if( CC_TST_TRNG(DA16X_ACRYPT_BASE, a, b, c, rsize, rbox) == 0 ){
				
				curstamp = da16x_rtc_get_mirror32(FALSE);
				PRINTF("TRNG ==========[%d, err %d]\n", (curstamp-oldstamp), errcnt);
				//cmd_dump_print(0, (UINT8 *)rbox, rsize, 0);
				PRINTF("%08x\n", rbox[8]);
				if( peakstamp < (curstamp-oldstamp)){
					peakstamp = (curstamp-oldstamp);
				}
			}else{

				errcnt++;
				PRINTF("TRNG error - %d\n", errcnt);

			}
		}

		PRINTF("Peak:%d\n", peakstamp);

		CRYPTO_FREE(rbox);
		return;
	}	

#ifdef	BUILD_OPT_DA16200_ASIC
	// following commands will write the OTP contents during the execution.
	if( DRIVER_STRCMP("cm", argv[1]) == 0 ){
		return;
	}
	if( DRIVER_STRCMP("cmpu", argv[1]) == 0 ){
		return;
	}
	if( DRIVER_STRCMP("dmpu", argv[1]) == 0 ){
		return;
	}
	if( DRIVER_STRCMP("icv.asset", argv[1]) == 0 ){
		return;
	}
	if( DRIVER_STRCMP("oem.asset", argv[1]) == 0 ){
		return;
	}
	if( DRIVER_STRCMP("reset", argv[1]) == 0 ){
		return;
	}
#endif //BUILD_OPT_DA16200_ASIC

	if( (argc == 3) || (argc == 4) ){
		UINT32 flag_dbg;

		#define	DA16X_FLASH_RALIB_OFFSET	(*(UINT32*)0x00F802D0)
		if(DRIVER_STRCMP("redirect", argv[2]) == 0 ){
			dst_addr = DA16X_FLASH_RALIB_OFFSET ; // redirect
			PRINTF("redirection : %08x\n", dst_addr);
		}else{
			DA16X_FLASH_RALIB_OFFSET = 0; // Clear
			
			dst_addr = htoi(argv[2]) ;
		}

		if( argc == 4 ){
			flag_dbg = htoi(argv[3]);
		}else{
			flag_dbg = 0; // false
		}

		if( DRIVER_STRCMP("eflash", argv[1]) == 0 ){
			dst_addr = (BOOT_MEM_SET(BOOT_EFLASH)|BOOT_OFFSET_SET(dst_addr));
			da16x_sflash_set_bussel(SFLASH_CSEL_1);
#if	!defined(BUILD_OPT_DA16200_CACHEXIP)
			da16x_cache_disable();
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);
#endif	//!defined(BUILD_OPT_DA16200_CACHEXIP)
		}
#ifdef	BUILD_OPT_DA16200_FPGA
		else
		if( DRIVER_STRCMP("nor", argv[1]) == 0 ){
			dst_addr = (UINT32)(BOOT_MEM_SET(BOOT_NOR)|BOOT_OFFSET_SET(dst_addr));
#if	!defined(BUILD_OPT_DA16200_CACHEXIP)
			da16x_cache_disable();
			DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);
#endif	//!defined(BUILD_OPT_DA16200_CACHEXIP)
		}
#endif	//BUILD_OPT_DA16200_FPGA
		else
		{
			dst_addr = (UINT32)(BOOT_MEM_SET(BOOT_SFLASH)|BOOT_OFFSET_SET(dst_addr));
#if	!defined(BUILD_OPT_DA16200_CACHEXIP)
			da16x_cache_disable();
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);
#endif	//!defined(BUILD_OPT_DA16200_CACHEXIP)
		}

		DA16X_Debug_SecureBoot(flag_dbg);
		{
			UINT32 tjmpaddr = 0;
			DA16X_SecureBoot(dst_addr, &tjmpaddr, NULL);
		}

	}else
	if( argc == 2 ){
#ifdef	BUILD_OPT_DA16200_FPGA
		if( DRIVER_STRCMP("cm", argv[1]) == 0 ){
			DA16X_TestOtp(0);
		}else
#endif	//BUILD_OPT_DA16200_FPGA
		if( DRIVER_STRCMP("fatal", argv[1]) == 0 ){
			UINT32	status;
			UINT32	rcRmaFlag = 0;

			status = DA16X_SecureBoot_Fatal(&rcRmaFlag);

			PRINTF("FATAL: st = %08x, rma = %08x\n", status, rcRmaFlag);
		}else
#if	!defined(BUILD_OPT_DA16200_ROMALL)
		if( DRIVER_STRNCMP("cmpu", argv[1], 4) == 0 ){
			UINT32	status;
			UINT8	*cmpu_hex_list;
			UINT8	*dump_cmpu_hex_list = NULL;

			dump_cmpu_hex_list = APP_MALLOC(180);

			if( DRIVER_STRLEN(argv[1]) >= 6 ){ // cmpu:hex-addr
				UINT32 address ;

				address = htoi((char *)(argv[1] + 5));
				status = sbrom_sflash_read( address, dump_cmpu_hex_list, 180);

			}else{
				const unsigned char const_cmpu_hex_list[] = {
				0x01, 0x00, 0x00, 0x00, 		//uniqueDataType
				0xac, 0xd4, 0x97, 0x30, 0xb3, 0xe4, 0x16, 0x4d, 0x35, 0x64, 0x67, 0x38, 0x03, 0x8f, 0x35, 0x98,
						//uniqueBuff
				0x02, 0x00, 0x00, 0x00, 		//kpicvDataType
				0x64, 0x6f, 0x72, 0x50, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x31, 0x76, 0x65, 0x52,
				0x32, 0x76, 0x65, 0x52, 0xdc, 0x4f, 0x47, 0xb4, 0x86, 0xaa, 0xa9, 0x0a, 0x97, 0x2e, 0xad, 0x4a,
				0xaa, 0xa7, 0x6f, 0xf3, 0x64, 0x7e, 0x08, 0x48, 0xed, 0x0c, 0xea, 0xed, 0x83, 0xcd, 0x2a, 0x5b,
				0x86, 0x5e, 0x04, 0xd2, 0xcb, 0x6f, 0xa6, 0xcb, 0x57, 0xf5, 0xd8, 0x21, 0x5a, 0x33, 0x14, 0xca,
						//kpicv
				0x02, 0x00, 0x00, 0x00, 		//kceicvDataType
				0x64, 0x6f, 0x72, 0x50, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x31, 0x76, 0x65, 0x52,
				0x32, 0x76, 0x65, 0x52, 0x6c, 0x01, 0x38, 0xdf, 0x6f, 0xa9, 0xc1, 0x43, 0xfa, 0xeb, 0xbb, 0x02,
				0x36, 0x36, 0x08, 0x7f, 0xb8, 0x4f, 0xf3, 0x8a, 0xf9, 0x15, 0x62, 0x5a, 0xa8, 0x90, 0x18, 0x99,
				0xde, 0x51, 0x35, 0xea, 0x39, 0xda, 0x88, 0x61, 0xcb, 0xcc, 0x5a, 0x4e, 0x43, 0x07, 0x22, 0x7c,
						//kceicv
				0x05, 0x00, 0x00, 0x00, 		//icvMinVersion
				0x00, 0x00, 0x00, 0x00, 		//icvConfigWord
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
						//icvDcuDefaultLock
				};
				status = TRUE;
				DRV_MEMCPY(dump_cmpu_hex_list, const_cmpu_hex_list, 180);
			}

			if( status == TRUE ){
				cmpu_hex_list = (UINT8 *)dump_cmpu_hex_list;
			}else{
				cmpu_hex_list = NULL;
			}

			status = DA16X_SecureBoot_CMPU((UINT8 *)cmpu_hex_list, 0);
			PRINTF("Secure.CMPU: %x\n", status);

			if( dump_cmpu_hex_list != NULL ){
				APP_FREE(dump_cmpu_hex_list);
			}

		}else
		if( DRIVER_STRNCMP("dmpu", argv[1], 4) == 0 ){
			UINT32	status;
			UINT8	*dmpu_hex_list;
			UINT8	*dump_dmpu_hex_list = NULL;

			dump_dmpu_hex_list = APP_MALLOC(192);

			if( DRIVER_STRLEN(argv[1]) >= 6 ){ // dmpu:hex-addr
				UINT32 address ;

				address = htoi((char *)(argv[1] + 5));
				status = sbrom_sflash_read( address, dump_dmpu_hex_list, 192);

			}else{
				const unsigned char const_dmpu_hex_list[] = {
				0x01, 0x00, 0x00, 0x00, 		//uniqueDataType
				0xff, 0x08, 0x4d, 0xc8, 0x5f, 0xb2, 0xfd, 0x1d, 0xc1, 0x54, 0x7a, 0x68, 0xfb, 0x71, 0x14, 0x4a,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						//uniqueBuff
				0x02, 0x00, 0x00, 0x00, 		//kcpDataType
				0x64, 0x6f, 0x72, 0x50, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x31, 0x76, 0x65, 0x52,
				0x32, 0x76, 0x65, 0x52, 0xb5, 0x44, 0x48, 0xc7, 0x48, 0x5e, 0xc0, 0x44, 0x9e, 0x49, 0x84, 0x67,
				0x30, 0x93, 0xee, 0x67, 0x15, 0xfd, 0xf6, 0xd7, 0x3f, 0x7d, 0xd7, 0xd5, 0x09, 0xcb, 0x9c, 0x98,
				0x78, 0xe7, 0x35, 0xc6, 0x9f, 0xc5, 0x31, 0xe6, 0xd5, 0xf5, 0x2a, 0xc2, 0x25, 0x96, 0x28, 0xda,
						//kcp
				0x02, 0x00, 0x00, 0x00, 		//kceDataType
				0x64, 0x6f, 0x72, 0x50, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x31, 0x76, 0x65, 0x52,
				0x32, 0x76, 0x65, 0x52, 0xda, 0x49, 0x81, 0x51, 0xfe, 0xdc, 0xfe, 0x68, 0x6a, 0x90, 0x05, 0x5b,
				0x98, 0xe7, 0x1b, 0x4a, 0x78, 0xb3, 0xd6, 0x00, 0x37, 0xaf, 0xb5, 0xe3, 0x45, 0x2c, 0xb4, 0xc6,
				0xc3, 0x43, 0x5d, 0xa0, 0x52, 0x59, 0x4f, 0x8b, 0xb5, 0x98, 0x4d, 0xab, 0x7c, 0x3b, 0x2a, 0x06,
						//kce
				0x05, 0x00, 0x00, 0x00, 		//oemMinVersion
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
						//oemDcuDefaultLock
				};
				status = TRUE;
				DRV_MEMCPY(dump_dmpu_hex_list, const_dmpu_hex_list, 192);
			}

			if( status == TRUE ){
				dmpu_hex_list = (UINT8 *)dump_dmpu_hex_list;
			}else{
				dmpu_hex_list = NULL;
			}

			status = DA16X_SecureBoot_DMPU((UINT8 *)dmpu_hex_list);
			PRINTF("Secure.DMPU: %x\n", status);

			if( dump_dmpu_hex_list != NULL ){
				APP_FREE(dump_dmpu_hex_list);
			}

		}else
		if( DRIVER_STRNCMP("icv.asset", argv[1], 9) == 0 ){
			UINT32	status;
			UINT32	assetsiz, encassetsiz;
			UINT8	*asset;
			UINT8	*dump_encasset_hex = NULL;

			dump_encasset_hex = APP_MALLOC((48+512)); // tag + enc.asset

			if( DRIVER_STRLEN(argv[1]) >= 11 ){ // dmpu:hex-addr
				UINT32 address ;

				address = htoi((char *)(argv[1] + 10));
				encassetsiz = (48+512); // tag + enc.asset
				status = sbrom_sflash_read( address, dump_encasset_hex, encassetsiz);
			}else{
				const unsigned char icv_encasset_hex_list[] = {
				0x74, 0x65, 0x73, 0x41, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x43, 0x6d, 0x07, 0xf2, 0x8a, 0xfa, 0x41, 0xf5, 0x22, 0xa9, 0x7c, 0xcf,
				0x74, 0x07, 0x95, 0xd8, 0x50, 0x61, 0xd2, 0x81, 0x72, 0x4f, 0x4f, 0x6c, 0xc9, 0xe3, 0xd1, 0x3f,
				0x5d, 0x2d, 0x6e, 0x97, 0x19, 0x93, 0xbc, 0x60, 0xed, 0x0e, 0xe3, 0x2a, 0x42, 0xf3, 0x5d, 0x32,
				0xef, 0x0c, 0xf0, 0xe7, 0xb6, 0x02, 0x0e, 0x4f, 0x78, 0x91, 0xca, 0x7c, 0x36, 0xdd, 0xce, 0xb2,
				0xcb, 0x21, 0x18, 0xf2, 0x7e, 0x13, 0x03, 0x06, 0x52, 0x88, 0x1f, 0x32, 0x97, 0x0a, 0x79, 0xa4,
				0x3f, 0x88, 0x0e, 0x38, 0x8a, 0xd8, 0x61, 0x31, 0xf0, 0x6d, 0x18, 0xb8, 0xc3, 0xf0, 0x59, 0xb7,
				0x0f, 0x14, 0xb3, 0x83, 0x12, 0x4b, 0xba, 0xac, 0x90, 0xe4, 0x70, 0x60, 0x89, 0x3b, 0x32, 0x5a,
				0x86, 0xc6, 0x95, 0xae, 0x9a, 0x71, 0xba, 0xa6, 0xb4, 0xea, 0x6c, 0x98, 0x25, 0xe1, 0x68, 0x05,
				0x28, 0x23, 0xe5, 0x66, 0xcd, 0x66, 0x7d, 0x2c, 0xa4, 0x4b, 0x03, 0x93, 0x88, 0x98, 0x42, 0x57,
				0x52, 0xc7, 0x00, 0x38, 0xd5, 0x5e, 0x2f, 0x12, 0x07, 0xc4, 0x69, 0x12, 0xb4, 0x16, 0xe4, 0xe7,
				0x22, 0x5e, 0xeb, 0xba, 0x96, 0xc4, 0x3c, 0x65, 0xa6, 0x3b, 0xa8, 0x49, 0xa1, 0xf3, 0x08, 0xa0,
				0xdc, 0xaf, 0xd6, 0x3f, 0x8c, 0xb3, 0xee, 0x4b, 0x85, 0x4f, 0xa3, 0x87, 0x62, 0x4e, 0x45, 0x8d,
				0x83, 0x9a, 0x5e, 0x1a, 0x30, 0x76, 0xe2, 0xc4, 0x9c, 0xe7, 0x34, 0x55, 0x8f, 0x77, 0x6c, 0x8b,
				0xf5, 0x89, 0xb8, 0xa7, 0x6c, 0x7d, 0x9a, 0x17, 0x0a, 0xff, 0xc6, 0x83, 0x58, 0x13, 0x1f, 0xac,
				0x0f, 0x3e, 0x15, 0x8c, 0x3b, 0x4d, 0x68, 0x53, 0x3b, 0x09, 0x1d, 0x31, 0xd9, 0x03, 0xd9, 0x9c,
				0x44, 0x1c, 0xc1, 0x26, 0xe4, 0x3a, 0xed, 0xc7, 0xfc, 0x96, 0x71, 0xee, 0xe8, 0x41, 0xd9, 0x4f,
				0xf1, 0x29, 0x7b, 0x4a, 0x00, 0x94, 0x54, 0x81, 0xa6, 0x13, 0x1d, 0xe9, 0x54, 0x21, 0x7d, 0x6f,
				0xbe, 0xf9, 0x7d, 0x95, 0x58, 0x95, 0x18, 0x5b, 0xbe, 0x5c, 0x1f, 0x3d, 0xf2, 0x2e, 0x08, 0x8f,
				};

				status = TRUE;
				encassetsiz = sizeof(icv_encasset_hex_list); // tag + enc.asset
				DRV_MEMCPY(dump_encasset_hex, icv_encasset_hex_list, encassetsiz);
			}

			if( status == TRUE ){
				asset = CRYPTO_MALLOC(512);

				assetsiz = DA16X_Secure_Asset(1
						, 0x00112233
						, (UINT32 *)dump_encasset_hex
						, encassetsiz
						, asset);

				if( assetsiz > 0 ){
					CRYPTO_DBG_DUMP(0, asset, assetsiz);
				}

				CRYPTO_FREE(asset);
			}

			APP_FREE(dump_encasset_hex);

		}else
		if( DRIVER_STRNCMP("oem.asset", argv[1], 9) == 0 ){
			UINT32	status;
			UINT32	assetsiz, encassetsiz;
			UINT8	*asset;
			UINT8	*dump_encasset_hex = NULL;

			dump_encasset_hex = APP_MALLOC((48+512)); // tag + enc.asset

			if( DRIVER_STRLEN(argv[1]) >= 11 ){ // dmpu:hex-addr
				UINT32 address ;

				address = htoi((char *)(argv[1] + 10));
				encassetsiz = (48+512); // tag + enc.asset
				status = sbrom_sflash_read( address, dump_encasset_hex, encassetsiz);
			}else{
				const unsigned char oem_encasset_hex_list[] = {
				0x74, 0x65, 0x73, 0x41, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x2c, 0xcd, 0x0b, 0xb8, 0xfb, 0x07, 0x1f, 0x4b, 0x90, 0x32, 0xef, 0xdb,
				0x83, 0x05, 0xab, 0xcb, 0x65, 0x5f, 0xfe, 0x9a, 0xae, 0x89, 0x1c, 0xa0, 0x06, 0x3f, 0xe5, 0x39,
				0x0c, 0xb9, 0xf1, 0x8d, 0xa9, 0x98, 0x40, 0x00, 0xec, 0xf2, 0x2a, 0xc7, 0xff, 0x4e, 0x60, 0xec,
				0x25, 0x8e, 0xd8, 0x22, 0x9f, 0x05, 0x40, 0xe6, 0x41, 0x61, 0x11, 0x87, 0xec, 0x37, 0x96, 0x2a,
				0xc8, 0xd2, 0x13, 0x07, 0xf9, 0xae, 0x32, 0x91, 0x6d, 0x3b, 0xc0, 0x21, 0x2d, 0x7b, 0x38, 0xad,
				0x5e, 0xaf, 0x0e, 0x44, 0x05, 0x2a, 0x32, 0x19, 0x8d, 0xac, 0x59, 0xfb, 0x41, 0xf7, 0x42, 0x4d,
				0x82, 0x5c, 0xd4, 0x91, 0xf7, 0xb1, 0x0a, 0x87, 0xb0, 0x37, 0xe6, 0x9b, 0xc2, 0x5d, 0xef, 0xcd,
				0x5e, 0x2e, 0x10, 0xbe, 0x95, 0xee, 0x26, 0xdb, 0xf5, 0xa5, 0x4f, 0x07, 0xa1, 0xfb, 0xb5, 0x33,
				0x45, 0x9e, 0xf9, 0xbc, 0x3f, 0x3d, 0xf2, 0x35, 0x66, 0xbd, 0x0d, 0xf1, 0x0f, 0x0e, 0xce, 0x1e,
				0xc4, 0x23, 0x22, 0xec, 0xf0, 0xc3, 0xec, 0xa9, 0x69, 0x55, 0x06, 0x8c, 0xc5, 0x3b, 0x0c, 0x9a,
				0xf3, 0xdc, 0xa4, 0xcb, 0x79, 0x3d, 0x0e, 0xa4, 0xd8, 0xc9, 0x1a, 0x37, 0x5c, 0x56, 0xfe, 0x17,
				0xdf, 0xc6, 0xcc, 0xe5, 0x36, 0x07, 0xa9, 0xc6, 0x83, 0x2e, 0x20, 0xd7, 0x72, 0x99, 0xe6, 0xb8,
				0xa8, 0x40, 0xba, 0x44, 0x7c, 0x6d, 0xc9, 0xd3, 0xad, 0x0a, 0xae, 0x48, 0x2e, 0x2d, 0x26, 0x7a,
				0xcf, 0x7d, 0x2d, 0x05, 0x2b, 0xfa, 0x7c, 0xd4, 0x40, 0x64, 0xa4, 0x87, 0xdc, 0x1b, 0xd4, 0x27,
				0xf3, 0x65, 0x5b, 0x2b, 0x9d, 0xb6, 0xcf, 0xdd, 0xd0, 0x74, 0x0e, 0x20, 0x26, 0xa2, 0xd8, 0xb6,
				0xaa, 0x88, 0xd8, 0x63, 0x76, 0xb0, 0x1f, 0xb8, 0x49, 0xb2, 0xe4, 0x40, 0x9d, 0x90, 0xdf, 0x80,
				0x10, 0xd0, 0x98, 0x6e, 0x8c, 0x81, 0xa7, 0x2f, 0xfb, 0x17, 0x39, 0x80, 0xc0, 0x66, 0x91, 0xde,
				0x96, 0xc5, 0x56, 0x4f, 0x71, 0x0d, 0xe6, 0x2f, 0x9f, 0xf7, 0x79, 0xd9, 0x44, 0x4f, 0xd0, 0xa8,
				};

				status = TRUE;
				encassetsiz = sizeof(oem_encasset_hex_list); // tag + enc.asset
				DRV_MEMCPY(dump_encasset_hex, oem_encasset_hex_list, encassetsiz);
			}

			if( status == TRUE ){
				asset = CRYPTO_MALLOC(512);

				assetsiz = DA16X_Secure_Asset(2
						, 0x00112233
						, (UINT32 *)dump_encasset_hex
						, encassetsiz
						, asset);

				if( assetsiz > 0 ){
					CRYPTO_DBG_DUMP(0, asset, assetsiz);
				}

				CRYPTO_FREE(asset);
			}

			APP_FREE(dump_encasset_hex);

		}else
		if( DRIVER_STRCMP("test", argv[1]) == 0 ){

			struct AssetSuite {
				char		*KeyName;
				AssetKeyType_t	KeyType;
				size_t		KeyLen;
			} AssetSuiteList[8] = {
				{ "USR-128", ASSET_USER_KEY  , (128/8) },
				{ "USR-192", ASSET_USER_KEY  , (192/8) },
				{ "USR-256", ASSET_USER_KEY  , (256/8) },
				{ "HUK"	   , ASSET_ROOT_KEY  , 0  },
				{ "Kcp"	   , ASSET_KCP_KEY   , 0  },
				{ "Kce"	   , ASSET_KCE_KEY   , 0  },
				{ "Kpicv"  , ASSET_KPICV_KEY , 0  },
				{ "Kceicv" , ASSET_KCEICV_KEY, 0  }
			};

			AssetUserKeyData_t UserKeyData;
			UINT32 i, keytype, assetid;
			INT32 iastlen, pkglen, oastlen;
			UINT8 *inasset, *assetpkg, *outasset, *userkey;

			#define	TEST_ASSET_SIZE	1024

			inasset = (UINT8 *)CRYPTO_MALLOC(TEST_ASSET_SIZE); // x16
			assetpkg = (UINT8 *)CRYPTO_MALLOC(TEST_ASSET_SIZE+48); // assetsize + 48
			outasset = (UINT8 *)CRYPTO_MALLOC(TEST_ASSET_SIZE); // x16
			userkey = (UINT8 *)CRYPTO_MALLOC(32); // AES 128, 192, 256 bit key
			iastlen = TEST_ASSET_SIZE;

			for( keytype = 0; keytype < 8; keytype ++ ){
				PRINTF("\n***** KeyType:%s *****\n", AssetSuiteList[keytype].KeyName);

				UserKeyData.pKey = userkey;
				UserKeyData.keySize = AssetSuiteList[keytype].KeyLen;

				assetid = da16x_random();

				for( i = 0; i < UserKeyData.keySize; i++ ){
					UserKeyData.pKey[i] = (UINT8)da16x_random();
				}

				for( i = 0; i < 128; i++ ){
					inasset[(32*keytype)+i] = (UINT8)i;
				}

				PRINTF("DA16X_Secure_Asset_RuntimePack\n");

				pkglen = DA16X_Secure_Asset_RuntimePack(AssetSuiteList[keytype].KeyType
							, /*0x60600C00*/ 0
							, &UserKeyData, assetid, "RunTest"
							, inasset, (UINT32)iastlen, assetpkg );

				if( pkglen > 0 ){
					PRINTF("assetpkg(%d):\n", pkglen);
					CRYPTO_DBG_DUMP(0, assetpkg, pkglen);

					oastlen = DA16X_Secure_Asset_RuntimeUnpack(AssetSuiteList[keytype].KeyType
								, &UserKeyData, assetid, assetpkg, (UINT32)pkglen, outasset );

					PRINTF("DA16X_Secure_Asset_RuntimeUnpack\n");

					if( oastlen < 0 ){
						PRINTF("DA16X_Secure_Asset_RuntimeUnpack: Error (%x)\n", oastlen);
					}else
					if( DRIVER_MEMCMP(inasset, outasset, (unsigned int)oastlen) == 0 ){
						PRINTF("DA16X_Secure_Asset_RuntimeUnpack: OK\n");
					}else{
						PRINTF("DA16X_Secure_Asset_RuntimeUnpack: mismatch !!\n");
						PRINTF("inasset(%d):\n", iastlen);
						CRYPTO_DBG_DUMP(0, inasset, iastlen);
						PRINTF("outasset(%d):\n", oastlen);
						CRYPTO_DBG_DUMP(0, outasset, oastlen);
					}
				}
				else
				{
					PRINTF("DA16X_Secure_Asset_RuntimePack: Error (%x)\n", pkglen);
				}
			}

			CRYPTO_FREE(userkey);
			CRYPTO_FREE(inasset);
			CRYPTO_FREE(assetpkg);
			CRYPTO_FREE(outasset);
		}else
#endif	//!defined(BUILD_OPT_DA16200_ROMALL)
		if( DRIVER_STRCMP("reset", argv[1]) == 0 ){
			DA16X_SecureBoot_ColdReset();
			PRINTF("ColdReset\n");
		}else
		if( DRIVER_STRCMP("lock", argv[1]) == 0 ){
			DA16X_SecureBoot_SetLock();
			PRINTF("SLock:%d\n", DA16X_SecureBoot_GetLock());
		}else
#if	!defined(BUILD_OPT_DA16200_ROMALL)
#ifndef __REDUCE_CODE_SIZE__
		if( DRIVER_STRCMP("trng", argv[1]) == 0 ){
			int ssize;
			extern void DA16X_Trng_Test(int ssize, int mode);

			ssize = 128;

			DA16X_Trng_Test(ssize, FALSE);
		}else
		if( DRIVER_STRCMP("trngfull", argv[1]) == 0 ){
			int ssize;
			extern void DA16X_Trng_Test(int ssize, int mode);

			ssize = 128;

			DA16X_Trng_Test(ssize, TRUE);
		}else
#endif	//!defined(BUILD_OPT_DA16200_ROMALL)
		if( DRIVER_STRCMP("huk", argv[1]) == 0 ){
			UINT8 HUKBuffer[16];
			UINT32 HUKSize;
			extern UINT32 DA16X_CalcHuk_Test(UINT8 *HUKBuffer, UINT32 *HUKsize);

			if( DA16X_CalcHuk_Test(HUKBuffer, &HUKSize) != 0 ){
				PRINTF("DA16X_CalcHuk_Test : error\n");
			}else{
				CRYPTO_DBG_DUMP(0, HUKBuffer, HUKSize);
			}

		}else
#endif // __REDUCE_CODE_SIZE__
		{
			PRINTF("SLock :%d\n", DA16X_SecureBoot_GetLock());
		}
	}
#if	!defined(BUILD_OPT_DA16200_ROMALL)
#ifndef __REDUCE_CODE_SIZE__
	else
	if( DRIVER_STRCMP("enc", argv[1]) == 0 && argc == 5){
		UINT32	status;
		UINT32	assetid, assetoff;
		INT32	assetsiz, pkgsiz;
		UINT8	*assetbuf, *pkgbuf;

		assetid = htoi(argv[2]);
		assetoff = htoi(argv[3]);
		assetsiz = htoi(argv[4]); // plaintext size

		assetsiz = (((assetsiz + 15) >> 4)<< 4); // 16B aligned
		PRINTF(" Aligned Asset Size:%d\n", pkgsiz);

		assetbuf = APP_MALLOC(assetsiz);
		pkgbuf = APP_MALLOC(assetsiz + 48);

		if( assetbuf == NULL ){
			return;
		}
		if( pkgbuf == NULL ){
			APP_FREE(assetbuf);
			return;
		}

		pkgsiz = 0;
		status = sbrom_sflash_read( assetoff, assetbuf, assetsiz);

		if( status > 0 ){
			pkgsiz = DA16X_Secure_Asset_RuntimePack(ASSET_ROOT_KEY
						, /*0x60600C00*/ 0
						, NULL, assetid, "RunPack"
						, assetbuf, assetsiz, pkgbuf );
		}
		if( pkgsiz > 0 ){
			PRINTF("PKG Size:%d\n", pkgsiz);
			sbrom_sflash_write(assetoff, pkgbuf, pkgsiz);
		}

		APP_FREE(pkgbuf);
		APP_FREE(assetbuf);

	}else
	if( DRIVER_STRCMP("dec", argv[1]) == 0 && argc == 5 ){
		UINT32	status;
		AssetInfoData_t AssetInfoData;
		UINT32 assetid, assetoff, flagwrite;
		INT32	assetsiz, pkgsiz;
		UINT8	*assetbuf, *pkgbuf;

		assetid = htoi(argv[2]);
		assetoff = htoi(argv[3]);

		flagwrite = htoi(argv[4]); // flash write test
		status = sbrom_sflash_read(assetoff, (UINT8 *)(&AssetInfoData), sizeof(AssetInfoData_t));
		if( status == 0 ){
			PRINTF("SFLASH Read Error:%x\n", assetoff);
			return;
		}
		if( (AssetInfoData.token == CC_RUNASSET_PROV_TOKEN) && (AssetInfoData.version == CC_RUNASSET_PROV_VERSION) ){
			assetsiz = AssetInfoData.assetSize;
			PRINTF("Stored PKG Size:%d\n", assetsiz);
			pkgsiz = assetsiz + 48;
		}else{
			PRINTF("Illegal Asset Package:%X.%X\n", AssetInfoData.token, AssetInfoData.version );
			return;
		}

		assetbuf = APP_MALLOC(assetsiz);
		pkgbuf = APP_MALLOC(pkgsiz);

		if( assetbuf == NULL ){
			return;
		}
		if( pkgbuf == NULL ){
			APP_FREE(assetbuf);
			return;
		}

		assetsiz = 0;
		status = sbrom_sflash_read( assetoff, pkgbuf, pkgsiz);

		if( status > 0 ){
			assetsiz = DA16X_Secure_Asset_RuntimeUnpack(ASSET_ROOT_KEY
						, NULL, assetid, pkgbuf, pkgsiz, assetbuf );
		}

		if( assetsiz > 0 ){
			PRINTF("ASSET:%d\n", assetsiz);
			CRYPTO_DBG_DUMP(0, assetbuf, assetsiz);

			if( flagwrite == 1 ){
				sbrom_sflash_write(assetoff, assetbuf, assetsiz);
			}
		}else{
			PRINTF("ASSET:decryption error (%x)\n", assetsiz);
		}

		APP_FREE(pkgbuf);
		APP_FREE(assetbuf);
 	}
#endif // __REDUCE_CODE_SIZE__
#endif //defined(BUILD_OPT_DA16200_ROMALL)

}

#endif	//SUPPORT_SBROM_TEST

#ifdef	SUPPORT_SPROD_TEST
/******************************************************************************
 *  cmd_sprod_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	SPROD_CMPU_FLASH_OFFSET	0x01F2000
#define	SPROD_DMPU_FLASH_OFFSET	0x01F3000
#define	SPROD_XMPU_ERASE_OPTION	TRUE

#define	SPROD_OTP_SLOCK_OPTION	TRUE  // FALSE: Unsecure/Insecure, TRUE : Secure

#define CLK_GATING_OTP				0x50006048
#define MEM_BYTE_WRITE(addr, data)	*((volatile UINT8 *)(addr)) = (data)



#define	SPROD_POR_RESET_ACTION()		{				\
	UINT32 pwrctrl;										\
	OAL_MSLEEP(10);										\
	pwrctrl = 0x03;										\
	RTC_IOCTL(RTC_SET_DC_PWR_CONTROL_REG, &pwrctrl);	\
}

void cmd_sprod_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	UINT32 lcs;
	extern int mbedtls_mng_lcsGet(uint32_t *pLcs);

	DA16X_SYSCLOCK->PLL_CLK_EN_4_PHY = 1;
	DA16X_SYSCLOCK->CLK_EN_PHYBUS = 1;

	DA16X_SecureBoot_OTPLock(1); // unlock 
	// reload otp lock status
	MEM_BYTE_WRITE(CLK_GATING_OTP, 0x00);
	DA16X_SecureBoot_OTPLock(1); // unlock

	/* OTP dummy read */
	otp_mem_create();
	otp_mem_read(0x700, &lcs);
	otp_mem_close();

	if( mbedtls_mng_lcsGet((uint32_t *)&lcs) == 0 ){
		switch(lcs){
		case 0x00 : // CM
			if( SPROD_CMPU_FLASH_OFFSET != 0 ){
				UINT32	status;
				UINT32	address;
				UINT8	*cmpu_hex_list;
				UINT8	*dump_cmpu_hex_list = NULL;

				dump_cmpu_hex_list = APP_MALLOC(180);

				address = SPROD_CMPU_FLASH_OFFSET;
				status = sbrom_sflash_read( address, dump_cmpu_hex_list, 180);

				if( status == TRUE ){
					cmpu_hex_list = (UINT8 *)dump_cmpu_hex_list;
				}else{
					cmpu_hex_list = NULL;
				}

				status = DA16X_SecureBoot_CMPU((UINT8 *)cmpu_hex_list, 0);
				PRINTF("Product.CMPU: %x\n", status);

				if( dump_cmpu_hex_list != NULL ){
					APP_FREE(dump_cmpu_hex_list);
				}

				if( status == 0 ){
					// CMPU OK
					SPROD_POR_RESET_ACTION();
				}else{
					PRINTF("Product.CMPU: Error\n");
				}
			}
			else
			{
				PRINTF("Product.State: Insecure Boot Scenario (Lcs-CM)\n");
			}
			break;
		case 0x01 : // DM
			if( SPROD_DMPU_FLASH_OFFSET != 0 ){
				UINT32	status;
				UINT32	address;
				UINT8	*dmpu_hex_list;
				UINT8	*dump_dmpu_hex_list = NULL;

				dump_dmpu_hex_list = APP_MALLOC(192);

				address = SPROD_DMPU_FLASH_OFFSET;
				status = sbrom_sflash_read( address, dump_dmpu_hex_list, 192);

				if( status == TRUE ){
					dmpu_hex_list = (UINT8 *)dump_dmpu_hex_list;
				}else{
					dmpu_hex_list = NULL;
				}

				status = DA16X_SecureBoot_DMPU((UINT8 *)dmpu_hex_list);
				PRINTF("Product.DMPU: %x\n", status);

				if( dump_dmpu_hex_list != NULL ){
					APP_FREE(dump_dmpu_hex_list);
				}

				if( status == 0 ){
					// DMPU OK
					SPROD_POR_RESET_ACTION();
				}else{
					PRINTF("Product.DMPU: Error\n");
				}
			}
			else
			{
				PRINTF("Product.State: Insecure Boot Scenario (Lcs-DM)\n");
			}
			break;
		case 0x05 : // SECURE
			{
				UINT32	status;
				UINT32	address;
				UINT32	idx;

				DA16X_SecureSocID();

				UINT8	*dmpu_hex_list;
				dmpu_hex_list = APP_MALLOC(192);

				address = SPROD_CMPU_FLASH_OFFSET;
				status = sbrom_sflash_read( address, dmpu_hex_list, 180);

				if( status == TRUE ){
					for( idx = 0; idx < 180; idx++ ){
						if( dmpu_hex_list[idx] != 0xFF ){
							status = FALSE;
							break;
						}
					}
					if( status == FALSE ){
						if( SPROD_XMPU_ERASE_OPTION == TRUE ){
							status = sbrom_sflash_erase( address, 180);
							PRINTF("Product.CMPU: Erased\n");
						}else{
							status = TRUE;
							PRINTF("Product.CMPU: Not Erased\n");
						}
					}else{
						PRINTF("Product.CMPU: Already Erased.\n");
					}
				}

				address = SPROD_DMPU_FLASH_OFFSET;
				status = sbrom_sflash_read( address, dmpu_hex_list, 192);

				if( status == TRUE ){
					for( idx = 0; idx < 192; idx++ ){
						if( dmpu_hex_list[idx] != 0xFF ){
							status = FALSE;
							break;
						}
					}
					if( status == FALSE ){
						if( SPROD_XMPU_ERASE_OPTION == TRUE ){
							status = sbrom_sflash_erase( address, 192);
							PRINTF("Product.DMPU: Erased\n");
						}else{
							PRINTF("Product.DMPU: Not Erased\n");
							status = TRUE;
						}
					}else{
						PRINTF("Product.DMPU: Already Erased.\n");
					}
				}

				if(SPROD_OTP_SLOCK_OPTION == TRUE){
					DA16X_SecureBoot_SetLock();
				}
				PRINTF("Product.SLock: %d\n", DA16X_SecureBoot_GetLock());
				if( DA16X_SecureBoot_GetLock() == 1 ){
					PRINTF("Product.State: Secure Boot Scenario - Good\n");
				}else{
					PRINTF("Product.State: Unsecure Boot Scenario - Good\n");
				}
			}
			break;
		case 0x07 : // RMA
			PRINTF("Product.LCS: RMA\n");
			break;
		}
	}
	else
	{
		PRINTF("Product.LCS: unknown lcs = %x\n", lcs);
	}
}

#endif	//SUPPORT_SPROD_TEST

/* EOF */
