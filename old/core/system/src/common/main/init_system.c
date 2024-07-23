/**
 ****************************************************************************************
 *
 * @file init_system.c
 *
 * @brief Application Initializer
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
#include <stdlib.h>

#include "sdk_type.h"

#include "hal.h"
#include "driver.h"

#include "da16x_system.h"
#include "schd_system.h"
#include "schd_idle.h"
#include "schd_trace.h"
#include "sys_cfg.h"
#include "ExceptionHandlers.h"
#include "driver.h"
#include "common_def.h"
#include "da16x_oops.h"
#include "sys_image.h"
#include "da16x_image.h"
#include "da16x_secureboot.h"
#include "project_config.h"

#undef	SUPPORT_TEST_DBGT_BOOT
#undef	SUPPORT_TEST_SECUREBOOT

#if defined(__DISABLE_CONSOLE__)
	#define CONSOLE_VISIBLE FALSE
#else
	#define CONSOLE_VISIBLE TRUE
#endif // __DISABLE_CONSOLE__

typedef void (* const pHandler)(void);

extern __attribute__ ((section(".isr_vector"))) pHandler __isr_vectors[];
__attribute__ ((used)) pHandler * used_vectors = __isr_vectors;
#ifdef WATCH_DOG_ENABLE
extern void setup_wdog();
#endif // WATCH_DOG_ENABLE


/******************************************************************************
 *  init_system_step0pre( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	init_system_step0pre(void)
{
	/* NVIC */
	_sys_nvic_create( );
	
	_sys_nvic_init((unsigned int *)__isr_vectors);

	da16x_zeroing_init();
}

/******************************************************************************
 *  init_system_step0( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	init_system_step0(void)
{
	/* NVIC */
	FDMA_SYS_INIT(FALSE);			// non-os model

	da16x_rtc_init_mirror(TRUE);
}

/******************************************************************************
 *  init_system_step1( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	init_system_step1(void)
{
	da16x_btm_control(TRUE);
}

/******************************************************************************
 *  init_system_step2( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void	init_system_step2(void)
{
	//extern void _sys_tick_interrupt(void);
	extern void SysTick_Handler (void);

	/* System Clock */
	_sys_clock_create();
	_sys_clock_init( DA16X_SYSTEM_XTAL );


	/* SysTick */
	_sys_tick_create( DA16X_BOOT_CLOCK );	// 120MHz
	_sys_tick_init( DA16X_BOOT_CLOCK, (void *)SysTick_Handler );		/* xPortSysTickHandler */

	/* DMA */
	_sys_dma_init();

	/* HWZero */
	da16x_zeroing_init();

	/* SOFT-IRQ */
	softirq_create();
	softirq_init();

	/* Crypto */

	DA16X_Crypto_ClkMgmt(TRUE);


	/* Exception */
	system_exception_init();
}

/******************************************************************************
 *  init_system_step3( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	init_system_step3(void)
{
#ifdef WATCH_DOG_ENABLE
	setup_wdog();
#else
    /* WatchDog */
    {
        HANDLE wdog;
        wdog = WDOG_CREATE(WDOG_UNIT_0);
        WDOG_INIT(wdog);
        WDOG_IOCTL(wdog, WDOG_SET_DISABLE, NULL);
        WDOG_CLOSE(wdog);
    }
#endif // WATCH_DOG_ENABLE

	/* FDMA */
	FDMA_SYS_DEINIT();
	FDMA_SYS_INIT(TRUE);	// os-model

	/* Console */
	#define DCU_EN0_OFFSET    (DA16X_ACRYPT_BASE|0x1E00UL)
	if( (*((volatile UINT32 *)DCU_EN0_OFFSET)&0xC0000000L) != 0 )
	{
		UINT32 conbaud;

		conbaud = da16x_get_baud_rate();
		if( conbaud != 0 ){
			console_init(DA16X_CONSOLE_PORT, conbaud, DA16X_UART_CLOCK);
			/* Set console display on/off */
			console_hidden_mode(CONSOLE_VISIBLE);
			da16x_auto_baud_on(NULL, DA16X_UART_CLOCK, (DA16X_UART_CLOCK)); //TEST
		} else {
			console_init(DA16X_CONSOLE_PORT, DA16X_BAUDRATE, DA16X_UART_CLOCK);
			da16x_auto_baud_on(NULL, DA16X_UART_CLOCK, (DA16X_UART_CLOCK));
			/* Set console display on/off */
			console_hidden_mode(CONSOLE_VISIBLE);
		}
	}

#ifdef	BUILD_OPT_VSIM
	{	// DBGT: Experimental
		char	*rosdk_date;
		char	*rosdk_time;

#ifdef	SUPPORT_TEST_DBGT_BOOT
		VOID	*dbgtbuff;

		dbgtbuff = (VOID *)(DA16200_RETMEM_BASE|(DA16200_RETMEMS_SIZE-0x800));
		DRV_MEMSET(dbgtbuff, 0, 0x800);

		da16x_sysdbg_dbgt_init(dbgtbuff, dbgtbuff, 0x800
			, (DBGT_OP_FRUNCOUNT|DBGT_OP_TXTRGEN|DBGT_OP_TXCHAR));

		da16x_sysdbg_dbgt_reinit(0); // stop

		da16x_sysdbg_dbgt_reinit((DBGT_OP_FRUNCOUNT|/*DBGT_OP_TXTRGEN|*/DBGT_OP_TXCHAR)); // restart
#endif	//SUPPORT_TEST_DBGT_BOOT

#ifdef	BUILD_OPT_DA16200_FPGA
		Printf("UF\r\n");
		DBGT_Printf("DBGT-ON (FPGA)\n"); // legal access
#else	//BUILD_OPT_DA16200_FPGA
		Printf("UA\r\n");
		DBGT_Printf("DBGT-ON (ASIC)\n"); // legal access
#endif	//BUILD_OPT_DA16200_FPGA

#ifdef	BUILD_OPT_DA16200_ROMALL
		ver_rtos_romsdk_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoSDK: %s %s\n", rosdk_date, rosdk_time);
		ver_rtos_romcrypto_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoCRY: %s %s\n", rosdk_date, rosdk_time);
		ver_rtos_romndk_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoNDK: %s %s\n", rosdk_date, rosdk_time);
#endif	//BUILD_OPT_DA16200_ROMALL
#ifdef	BUILD_OPT_DA16200_LIBNDK
		ver_rtos_romsdk_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoSDK: %s %s\n", rosdk_date, rosdk_time);
		ver_rtos_romcrypto_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoCRY: %s %s\n", rosdk_date, rosdk_time);
		ver_rtos_romndk_buildtime(&rosdk_date, &rosdk_time);
		VSIM_PRINTF("RoNDK: %s %s\n", rosdk_date, rosdk_time);
#endif	//BUILD_OPT_DA16200_LIBNDK

		VSIM_PRINTF("VSIM: %8d %6d\n", REV_BUILDDATE, REV_BUILDTIME );
		VSIM_PRINTF("CODE: %08X, size %08X\n", ROM_MEM_BASE() , ROM_MEM_SIZE() );
		VSIM_PRINTF("FREE: %08X, size %08X\n", FREE_MEM_BASE(), FREE_MEM_SIZE());
	}

	if( SYSGET_BOOTMODE() == BOOT_SRAM ){
		//VSIM_PRINTF("RTOS SRAM based Boot : ATE (%02x)\n", da16x_boot_get_wakeupmode());
		SYSSET_BKUP_SINFO();
		VSIM_PRINTF("RTOS SRAM based Boot : ATE (VSIM)\n");
	}else{
		//VSIM_PRINTF("RTOS POR Boot (%02x)\n", da16x_boot_get_wakeupmode());
		VSIM_PRINTF("RTOS POR Boot (VSIM)\n");
	}
#endif	//BUILD_OPT_VSIM
}

/******************************************************************************
 *  init_system_step4( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	init_system_step4(UINT32 rtosmode)
{
	UINT32	sysclock;

#ifdef	BUILD_OPT_DA16200_ROMALL
	/* NVRAM : skip */
#else	//BUILD_OPT_DA16200_ROMALL
#ifdef	BUILD_OPT_VSIM
	if( SYSGET_BOOTMODE() == BOOT_SRAM ){
		VSIM_PRINTF("NVRAM: sflash access skip \r\n");
	}
	else
#endif	//BUILD_OPT_VSIM
	{
		/* NVRAM */
		system_config_create(NVCFG_INIT_SECSIZE);
		system_config_init(FALSE); 				// manual mode
	}
#endif	//BUILD_OPT_DA16200_ROMALL

#ifdef	XIP_CACHE_BOOT
  #ifdef	BUILD_OPT_DA16200_CACHEXIP
	da16x_cache_clkmgmt(ROM_MEM_BASE());
  #endif	//BUILD_OPT_DA16200_CACHEXIP
#endif	/* XIP_CACHE_BOOT */

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0xA1));

	/* Clock */
	if( SYS_NVCONFIG_FIND("boot.clk.bus", &sysclock) == 0 ){
#ifdef	PRINT_CLOCK 
		Printf(" [%s] Not found clk.bus  \r\n", __func__);
#endif
		_sys_clock_read( &sysclock, sizeof(UINT32));
	}
	else if(sysclock != 0)
	{
		UINT32  curclock;
		
#ifdef	PRINT_CLOCK 
		Printf(" [%s] Found clk.bus %d \r\n", __func__, sysclock);
#endif
		_sys_clock_read( &curclock, sizeof(UINT32));

#ifdef  SUPPORT_NVRAM_CLOCK_SPEED
		/* Update system clock */
#ifdef	PRINT_CLOCK 
		Printf(CYAN_COLOR " [%s] write sysclock (sys:%d curr:%d) \r\n" CLEAR_COLOR, __func__, sysclock, curclock);
#endif
		if( sysclock != curclock ){
			_sys_clock_write( &sysclock, sizeof(UINT32));
		}
		else {
			_sys_clock_write( &sysclock, 0xFFFFFFFF);
		}
#else   //SUPPORT_NVRAM_CLOCK_SPEED

		sysclock = curclock;
		_sys_clock_write( &sysclock, 0xFFFFFFFF);

#endif  //SUPPORT_NVRAM_CLOCK_SPEED
	}

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0xA2));

	{
		HANDLE flash;
		flash = SFLASH_CREATE(SFLASH_UNIT_0);

		if(flash != NULL ){
			SFLASH_PATCH(flash, rtosmode);
			//Printf(">>>SFLASH_PATCH\r\n");
		}
	}
}

/******************************************************************************
 *  init_system_post( )
 *
 *  Purpose : this function must be run after loading RaLIB !!
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	init_system_post(void)
{
#ifdef	SUPPORT_USLEEP
	{
		HANDLE  handler;
		UINT32 ioctldata;

		handler = USLEEP_CREATE(USLEEP_0);
		USLEEP_INIT(handler);
		//TICK-Resolution:
#ifdef	BUILD_OPT_VSIM
		ioctldata = 200 /* usec, min 100 */;
#else	//BUILD_OPT_VSIM
		ioctldata = 500 /* usec, min 100 */;
#endif	//BUILD_OPT_VSIM
		USLEEP_IOCTL(USLEEP_HANDLER(), USLEEP_SET_RESOLUTION, &ioctldata);
	}
#endif //SUPPORT_USLEEP
}

#ifdef RAM_2ND_BOOT
#include "buildtime.h"

/******************************************************************************
 *
 ******************************************************************************/

#define SUPPORT_WDOG_PROTECT
#undef  SUPPORT_ROM_USLEEP
#undef	SUPPORT_ROM_UDMA
#undef	SUPPORT_TEST_DBGT_BOOT
#undef	SUPPORT_NEW_VSIMBOOT		// New VSIM Boot Offset

#define	SYSDBG_DEBUG_MASK	0x08	// TCLK = high

#ifdef	BUILD_OPT_DA16200_FPGA
#define	SW_BIN_BUILD_INFO	(0xC0000000|((REV_BUILDFULLTIME/100)%100000000))
#else	//BUILD_OPT_DA16200_FPGA
#define	SW_BIN_BUILD_INFO	(0x80000000|((REV_BUILDFULLTIME/100)%100000000))
#endif	//BUILD_OPT_DA16200_FPGA

/******************************************************************************
 *  rom_check_ramlib_image( )
 *
 *  Purpose :
 *  Input   :	loadmode = 0, 1 : rtos, 2 : ramlib, 3 : rtos + ramlib
 *  Output  :
 *  Return  :  never returns if Image exists.
 ******************************************************************************/

#define	DA16X_NOR_BASE_ADDRESS	(DA16X_NOR_BASE)

UINT32 rom_check_ramlib_image(UINT32 skip)
{
	UINT32	rlib_la, rlib_ap, rlib_sz;
	HANDLE	handler;
	UINT32	*local_retmemboot = (UINT32 *) RETMEM_BOOTMODE_BASE;
	UINT32 loadbase = local_retmemboot[BOOT_IDX_RLIBLA];

	rlib_sz = 0;

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0x600));
	ASIC_DBG_TRIGGER(BOOT_MEM_GET(loadbase));

	switch(BOOT_MEM_GET(loadbase)){
#ifdef	BUILD_OPT_DA16200_FPGA
		case BOOT_NOR: /* NOR */
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x610));
			rlib_sz = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_RUN)
				, (DA16X_NOR_BASE_ADDRESS|BOOT_OFFSET_GET(loadbase))
				, &rlib_la, &rlib_ap );

			if( rlib_sz > 0 ){
				local_retmemboot[BOOT_IDX_RLIBAP] = rlib_la;
				local_retmemboot[BOOT_IDX_RLIBSZ] = rlib_sz;
			}
			break;
#endif //BUILD_OPT_DA16200_FPGA
		case BOOT_RETMEM: /* Retained Memory */
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x620));
			// local_retmemboot[BOOT_IDX_RLIBLA]
			// local_retmemboot[BOOT_IDX_RLIBAP]
			handler = retm_image_open(0
					, (DA16X_RETMEM_BASE|BOOT_MEMADR_GET(loadbase))
					, NULL);


			rlib_sz = retm_image_load(handler, &rlib_la);

			if( rlib_sz > 0 ){
				local_retmemboot[BOOT_IDX_RLIBAP] = rlib_la;
				local_retmemboot[BOOT_IDX_RLIBSZ] = rlib_sz;
			}

			if( skip == FALSE ){
				retm_image_close(handler);
			}

			break;
		case BOOT_EFLASH: /* SFLASH */
			da16x_sflash_set_bussel(SFLASH_CSEL_1);
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x630));
			rlib_sz = DA16X_SecureBoot(BOOT_OFFSET_GET(loadbase), NULL, NULL);
			if( rlib_sz > 0 ){
				local_retmemboot[BOOT_IDX_RLIBAP] = 0;
				local_retmemboot[BOOT_IDX_RLIBSZ] = 0;
			}
			break;
		case BOOT_SFLASH: /* SFLASH */
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x630));
			rlib_sz = DA16X_SecureBoot(BOOT_OFFSET_GET(loadbase), NULL, NULL);
			if( rlib_sz > 0 ){
				local_retmemboot[BOOT_IDX_RLIBAP] = 0;
				local_retmemboot[BOOT_IDX_RLIBSZ] = 0;
			}
			break;
		default:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x640));

			handler = flash_image_open((sizeof(UINT32)*8), BOOT_OFFSET_GET(loadbase), NULL);

			rlib_sz = flash_image_check(handler, 0, BOOT_OFFSET_GET(loadbase));

			if( rlib_sz > 0 ){
				rlib_sz = flash_image_load(handler
					, BOOT_OFFSET_GET(loadbase)
					, &rlib_la, &rlib_ap
					);
			}

			if( rlib_sz > 0 ){
				local_retmemboot[BOOT_IDX_RLIBAP] = rlib_la;
				local_retmemboot[BOOT_IDX_RLIBSZ] = rlib_sz;
			}

			if( skip == FALSE ){
				flash_image_close(handler);
			}

			break;
	}

	return rlib_sz;
}

/******************************************************************************
 *  p_rom_stop_system( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void    p_rom_stop_system(void)
{
#ifdef  SUPPORT_ROM_USLEEP
	USLEEP_CLOSE(USLEEP_HANDLER());
#endif  //SUPPORT_ROM_USLEEP
	console_deinit(0);

	_sys_tick_close( );

	_sys_nvic_close( );
}

/******************************************************************************
 *  p_rom_check_boot_image( )
 *
 *  Purpose :
 *  Input   :	loadmode = 0, 1 : rtos, 2 : ramlib, 3 : rtos + ramlib
 *  Output  :
 *  Return  :  never returns if Image exists.
 ******************************************************************************/

UINT32	p_rom_check_boot_image(UINT32 loadmode, UINT32 bootbase, UINT32 limitaddr)
{
	HANDLE  handler;
	UINT32	load_addr, jmp_addr, status, error, jumpnow;
	UINT32	*local_retmemboot = (UINT32 *) RETMEM_BOOTMODE_BASE;

	load_addr = 0;
	jmp_addr  = 0;
	error = 0x02;	// Image error
	status = 0;
	jumpnow = FALSE;

	//Printf(" [%s] loadmode:%d bootbase:0x%x limitaddr:0x%x BOOT_MEM_GET:0x%x\r\n", __func__, loadmode, bootbase, limitaddr, BOOT_MEM_GET(bootbase));

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0x700));
	ASIC_DBG_TRIGGER(BOOT_MEM_GET(bootbase));

	// Secure-to-Unsecure =========================
	switch(BOOT_MEM_GET(bootbase)){
	case	BOOT_SFLASH:	// Secure
		if( DA16X_SecureBoot_GetLock() == FALSE ){ //Unsecure Boot, 0x2C is '0'
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x701));
			bootbase = BOOT_MEM_SET(BOOT_SFLASH_X)|BOOT_OFFSET_SET(bootbase);
		}
		break;
	case	BOOT_SFLASH_X: // Unsecure w/ CRC
		break;
	case	BOOT_SFLASH_C:	// Unsecure w/o CRC
		break;
	case	BOOT_EFLASH:	// Secure
		if( DA16X_SecureBoot_GetLock() == FALSE ){ //Unsecure Boot, 0x2C is '0'
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x702));
			bootbase = BOOT_MEM_SET(BOOT_EFLASH_X)|BOOT_OFFSET_SET(bootbase);
		}
		break;
	case	BOOT_EFLASH_X:	// Unsecure w/ CRC
		break;
	case	BOOT_EFLASH_C:	// Unsecure w/o CRC
		break;
	default:
		break;
	}

#ifdef	SUPPORT_TEST_SECUREBOOT
	bootbase = BOOT_MEM_SET(BOOT_SFLASH)|BOOT_OFFSET_SET(bootbase);
#endif	//SUPPORT_TEST_SECUREBOOT

	// SFLASH-to-EFLASH ==========================
	//if( SYSGET_SFLASH_INFO() == SFLASH_NONSTACK )
	if( da16x_sflash_get_bussel() == SFLASH_CSEL_1 ){
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0x703));

		switch(BOOT_MEM_GET(bootbase)){
		case	BOOT_SFLASH:
		case	BOOT_SFLASH_X:
		case	BOOT_SFLASH_C:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x704));
			bootbase = BOOT_MEM_SET(BOOT_MEM_GET(bootbase)+BOOT_EFLASH-BOOT_SFLASH)|BOOT_OFFSET_SET(bootbase);
			break;
		default:
			break;
		}
	}else{
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0x705));

		switch(BOOT_MEM_GET(bootbase)){
		case	BOOT_EFLASH:
		case	BOOT_EFLASH_X:
		case	BOOT_EFLASH_C:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x706));
			bootbase = BOOT_MEM_SET(BOOT_MEM_GET(bootbase)+BOOT_SFLASH-BOOT_EFLASH)|BOOT_OFFSET_SET(bootbase);
			break;
		default:
			break;
		}
	}

	// ATE mode ==================================
	switch(BOOT_MEM_GET(bootbase)){
	case	BOOT_ROM:
	case	BOOT_SRAM:
	case	BOOT_RETMEM:
			/* DPM Case */
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x709));
			break;
	default:
		switch(SYSGET_BOOTMODE()){ /* Unsecure boot only !! */
		case BOOT_SFLASH:
		case BOOT_SFLASH_X:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x707));
			bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(bootbase);
			break;
		case BOOT_SFLASH_C:
#ifdef	SUPPORT_NEW_VSIMBOOT
			if( DA16X_SecureBoot_GetLock() == FALSE ){ //Unsecure Boot, 0x2C is '0'
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x70A));
				bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(SYSGET_BOOTADDR());
			}
			else
#endif	//SUPPORT_NEW_VSIMBOOT
			{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x70B));
				bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(bootbase);
			}
			break;
		case BOOT_EFLASH:
		case BOOT_EFLASH_X:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x708));
			bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(bootbase);
			break;
		case BOOT_EFLASH_C:
#ifdef	SUPPORT_NEW_VSIMBOOT
			if( DA16X_SecureBoot_GetLock() == FALSE ){ //Unsecure Boot, 0x2C is '0'
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x70C));
				bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(SYSGET_BOOTADDR());
			}
			else
#endif	//SUPPORT_NEW_VSIMBOOT
			{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x70D));
				bootbase = BOOT_MEM_SET(SYSGET_BOOTMODE())|BOOT_OFFSET_SET(bootbase);
			}
			break;
		default:
			break;
		}
	}

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0x777));
	ASIC_DBG_TRIGGER(BOOT_MEM_GET(bootbase));

	switch(BOOT_MEM_GET(bootbase)){
#ifdef	BUILD_OPT_DA16200_FPGA
		case BOOT_NOR: /* NOR */
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x300));

			//=================================================
			// Check Flash Image
			//=================================================
			if((loadmode & 0x1) != 0){
				status = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_HEADER)
					, (DA16X_NOR_BASE_ADDRESS|BOOT_OFFSET_GET(bootbase))
					, &load_addr, &jmp_addr );
			}else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x301));
				status = 0;
				error  = 0;	// skip
				load_addr = 0;
			}
			//=================================================
			// Check Image Size
			//=================================================
			if( (load_addr < limitaddr) && ((load_addr+status) >= limitaddr) ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x302));
				// Image size exceeds a loadable limit !!
				status = 0;
				error = 1;
			}
			//=================================================
			// Load Flash Image
			//=================================================
			if( (status > 0) || (loadmode == 0) ){ // RomPatch
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x310));

				status = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_RUN)
					, (DA16X_NOR_BASE_ADDRESS|BOOT_OFFSET_GET(bootbase))
					, &load_addr, &jmp_addr );
			}

			//=================================================
			// Load RamLib
			//=================================================
			if( ((status == 0) && (error == 0)) || (status > 0) )
			if( ((loadmode & 0x2) != 0) && local_retmemboot[BOOT_IDX_RLIBLA] != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x315));
				status = rom_check_ramlib_image(FALSE);
			}

			if( status > 0 && jmp_addr != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x320));

				// TODO: stop_system(4);

				jumpnow = TRUE;
			}
			break;
#endif	//BUILD_OPT_DA16200_FPGA
		case BOOT_RETMEM: /* Retained Memory */

			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x500));

			if((loadmode & 0x1) != 0){
				handler = retm_image_open(0
						, (DA16X_RETMEM_BASE|BOOT_MEMADR_GET(bootbase))
						, NULL);


				status = retm_image_load(handler, &jmp_addr);

				if( status > 0 ){
					error = 0;
				}else{
					status = 0;
					error = 1;
					jmp_addr = 0;
				}

				retm_image_close(handler);
			}else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x501));
				status = 0;
				error  = 0;	// skip
				load_addr = 0;
			}

			//=================================================
			// Load RamLib
			//=================================================
			if( ((status == 0) && (error == 0)) || (status > 0) )
			if( ((loadmode & 0x2) != 0) && local_retmemboot[BOOT_IDX_RLIBLA] != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x515));
				status = rom_check_ramlib_image(FALSE);
			}

			if( status > 0 && jmp_addr != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x520));

				// TODO: stop_system(4);

				jumpnow = TRUE;
			}
			break;
		case BOOT_EFLASH: /* SFLASH */
			da16x_sflash_set_bussel(SFLASH_CSEL_1);

			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x200));

			da16x_cache_disable();
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);

			da16x_set_cacheoffset(BOOT_OFFSET_GET(bootbase)); // Cache Offset

			if((loadmode & 0x1) != 0){
				status = DA16X_SecureBoot(BOOT_OFFSET_GET(bootbase), &jmp_addr, NULL);
				if( status == 0 ){
					error  = 0x3;	// Error
				}else{
					error  = 0;	// OK
				}

			}else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x201));
				status = 0;
				error  = 0;	// skip
				jmp_addr = 0;
			}

			//=================================================
			// Load Flash RamLib
			//=================================================
			if( ((status == 0) && (error == 0)) || (status > 0) )
			if( ((loadmode & 0x2) != 0) && local_retmemboot[BOOT_IDX_RLIBLA] != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x215));
				status = rom_check_ramlib_image(TRUE); // skip
			}

			if( status > 0 && jmp_addr != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x220));

				// TODO: stop_system(4);

				jumpnow = TRUE;
			}

			da16x_cache_disable();
			break;

		case BOOT_SFLASH: /* SFLASH */

			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x200));

			da16x_cache_disable();
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);

			da16x_set_cacheoffset(BOOT_OFFSET_GET(bootbase)); // Cache Offset

			if((loadmode & 0x1) != 0){
				status = DA16X_SecureBoot(BOOT_OFFSET_GET(bootbase), &jmp_addr, NULL);
				if( status == 0 ){
					error  = 0x3;	// Error
				}else{
					error  = 0;	// OK
				}

			}else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x201));
				status = 0;
				error  = 0;	// skip
				jmp_addr = 0;
			}

			//=================================================
			// Load Flash RamLib
			//=================================================
			if( ((status == 0) && (error == 0)) || (status > 0) )
			if( ((loadmode & 0x2) != 0) && local_retmemboot[BOOT_IDX_RLIBLA] != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x215));
				status = rom_check_ramlib_image(TRUE); // skip
			}

			if( status > 0 && jmp_addr != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x220));

				// TODO: stop_system(4);

				jumpnow = TRUE;
			}

			da16x_cache_disable();
			break;

		default:
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0x100));

			da16x_set_cacheoffset(BOOT_OFFSET_GET(bootbase)); // Cache Offset

			switch(BOOT_MEM_GET(bootbase)){ /* Unsecure boot only !! */
			case BOOT_SFLASH_X:
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C0)); // w/ CRC
				handler = flash_image_open((DA16X_IMG_BOOT|(sizeof(UINT32)*8)), BOOT_OFFSET_GET(bootbase), NULL);
				break;
			case BOOT_SFLASH_C:
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C1));	// w/o CRC
				handler = flash_image_open((DA16X_IMG_BOOT|DA16X_IMG_SKIP|(sizeof(UINT32)*8)), BOOT_OFFSET_GET(bootbase), NULL);
				break;
			case BOOT_EFLASH_X:
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C2));	// w/ CRC
				da16x_sflash_set_bussel(SFLASH_CSEL_1);
				handler = flash_image_open((DA16X_IMG_BOOT|(sizeof(UINT32)*8)), BOOT_OFFSET_GET(bootbase), NULL);
				break;
			case BOOT_EFLASH_C:
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C3));	// w/o CRC
				da16x_sflash_set_bussel(SFLASH_CSEL_1);
				handler = flash_image_open((DA16X_IMG_BOOT|DA16X_IMG_SKIP|(sizeof(UINT32)*8)), BOOT_OFFSET_GET(bootbase), NULL);
				break;
			default:
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x1C4));	// w/ CRC
				da16x_sflash_set_bussel(SFLASH_CSEL_1); // default external sflash
				handler = flash_image_open((DA16X_IMG_BOOT|(sizeof(UINT32)*8)), BOOT_OFFSET_GET(bootbase), NULL);
				break;
			}

			//=================================================
			// Check Flash Image
			//=================================================
			if((loadmode & 0x1) != 0){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x101));

				status = flash_image_check(handler
						, 0
						, BOOT_OFFSET_GET(bootbase)
					);
				status = flash_image_extract(handler
						, BOOT_OFFSET_GET(bootbase)
						, &load_addr, &jmp_addr
					);
			}else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x102));
				status = 0;
				error  = 0;	// skip
				load_addr = 0;
			}
			//=================================================
			// Check Image Size
			//=================================================
			if( (load_addr < limitaddr) && (load_addr+status) >= limitaddr ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x103));
				// Image size exceeds a loadable limit !!
				status = 0;
				error = 1;
			}
			//=================================================
			// Load Flash Image
			//=================================================
			if( status > 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x110));

				// TODO: Secure Debug, Check
				DA16X_SecureDebug(handler, BOOT_OFFSET_GET(bootbase));

				status = flash_image_load(handler
						, BOOT_OFFSET_GET(bootbase)
						, &load_addr, &jmp_addr
					);
			}
			else{
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x113));

				// TODO: Secure Debug, NULL
				DA16X_SecureDebug(NULL, 0);
			}

			//=================================================
			// Load Flash RamLib
			//=================================================
			if( ((status == 0) && (error == 0)) || (status > 0) )
			if( ((loadmode & 0x2) != 0) && local_retmemboot[BOOT_IDX_RLIBLA] != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x115));
				status = rom_check_ramlib_image(TRUE); // skip
			}

			flash_image_close(handler);

			if( status > 0 && jmp_addr != 0 ){
				ASIC_DBG_TRIGGER(MODE_INI_STEP(0x120));

				// TODO: stop_system(4);

				jumpnow = TRUE;
			}
			break;
	}

	if( (jumpnow == TRUE) && (loadmode != 0) ){
		ASIC_DBG_TRIGGER(MODE_DPM_STEP(0xF50400));
		ASIC_DBG_TRIGGER(jmp_addr);

		p_rom_stop_system();

		//=================================================
		// XIP/Cache
		//=================================================
#ifdef	BUILD_OPT_DA16200_FPGA
		if( (jmp_addr >= DA16X_NOR_XIP_BASE) && (jmp_addr <= DA16X_NOR_XIP_END) ){
			ASIC_DBG_TRIGGER(DA16X_NORTYPE_XIP);
			DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // XIP Boot
		}else
		if( (jmp_addr >= DA16X_NOR_CACHE_BASE) && (jmp_addr <= DA16X_NOR_CACHE_END) ){
			ASIC_DBG_TRIGGER(DA16X_NORTYPE_CACHE);
			DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);
		}else
#endif	//BUILD_OPT_DA16200_FPGA
//da16200removed:		if( (jmp_addr >= DA16X_XIP_BASE) && (jmp_addr <= DA16X_XIP_END) ){
//da16200removed:			ASIC_DBG_TRIGGER(DA16X_SFLASHTYPE_XIP);
//da16200removed:			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP); // XIP Boot
//da16200removed:		}else
		if( (jmp_addr >= DA16X_CACHE_BASE) && (jmp_addr <= DA16X_CACHE_END) ){
			ASIC_DBG_TRIGGER(DA16X_SFLASHTYPE_CACHE);
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE); // CACHE Boot
			da16x_cache_enable(FALSE);
		}else{
			ASIC_DBG_TRIGGER(0);
#ifdef	BUILD_OPT_DA16200_FPGA
			DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // FLASH Boot
#else	//BUILD_OPT_DA16200_FPGA
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP); // FLASH Boot
#endif	//BUILD_OPT_DA16200_FPGA
		}
		//=================================================
		// Activate WatchDog
		//=================================================
#ifdef	SUPPORT_WDOG_PROTECT
#ifdef WATCH_DOG_ENABLE
		{
			HANDLE wdog;
			wdog = WDOG_CREATE(WDOG_UNIT_0);
			WDOG_INIT(wdog);
			WDOG_IOCTL(wdog, WDOG_SET_DISABLE, NULL );
			WDOG_IOCTL(wdog, WDOG_SET_RESET, NULL );
		if (!(CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)){
			WDOG_IOCTL(wdog, WDOG_SET_ENABLE, NULL );
		}
			//WDOG_CLOSE(wdog);
			SYSSET_SWVERSION((SYS_FALT_BOOT_MASK|0x00E0)); // Protection
		}
#endif	/* WATCH_DOG_ENABLE */
#endif	//SUPPORT_WDOG_PROTECT

		//=================================================
		// Branch
		//=================================================
		{ /* NOTICE !! do not modify this clause !! */
			volatile UINT32  dst_addr;
			dst_addr = jmp_addr;
			/* get Reset handler address     */
			dst_addr = *((UINT32 *)(jmp_addr|0x04));
			/* Jump into App                 */
			dst_addr = (dst_addr|0x01);

			ASM_BRANCH(dst_addr);	/* Jump into App */
		}
	}

	return error;
}

#endif	/* RAM_2ND_BOOT */

/* EOF */
