/**
 ****************************************************************************************
 *
 * @file ueboot_initialize.c
 *
 * @brief Initialize system Wi-Fi features
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
#include <stdlib.h>

#include "oal.h"
#include "hal.h"
#include "library.h"
#include "da16x_system.h"
#include "da16x_oops.h"

#if		defined(BUILD_OPT_DA16200_ROMALL)
#include "romsdk.h"
#include "romcrypto.h"
#include "romndk.h"
#include "rommac.h"
#elif	defined(BUILD_OPT_DA16200_LIBNDK)
#else	//BUILD_OPT_DA16200_LIBALL
#endif //BUILD_OPT_DA16200_LIBALL

#include "common_def.h"
#include "iface_defs.h"

#include "schd_system.h"
#include "schd_idle.h"
#include "schd_trace.h"
#include "nvedit.h"
#include "environ.h"
#include "sys_cfg.h"

#include "command.h"
#include "common_uart.h"

#include "da16x_image.h"
#include "project_config.h"

#include "cal.h"
#include "crypto_primitives.h"
#include "mbedtls_cc_mng_int.h"

/******************************************************************************
 * Macro
 ******************************************************************************/

#define SUPPORT_SECURE_BOOT

#define	LOAD_FAIL		1
#define	CHK_ABNORMAL	2

#define	IMG_NORMAL0		0
#define	IMG_NORMAL1		1
#define	IMG_ABNORMAL0	2
#define	IMG_ABNORMAL1	3

#undef	SUPPORT_DA16X_RMA_OTP_ERASE	//Non-Secure:undef	Secure:undef	RMA:define
#define	SUPPORT_SECURE_REGION_LOCK	//Non-Secure:define	Secure:undef	RMA:undef
#define	SUPPORT_UEBOOT_NVRAM
#undef	SUPPORT_UEBOOT_PROMPT
#define SUPPORT_OTA
#define	SUPPORT_EMERGENCY_BOOT
#define EMERGENCY_TIMEOUT   (2)
#undef	SUPPORT_RECOVER_IDX

#define SUPPORT_SFLASH_UPDATE_REGS

//Using SUPPORT_EMERGENCY_BOOT defines below
#define ESC   (0x1B)

#ifdef SUPPORT_SFLASH_UPDATE_REGS
#include "sflash.h"
#include "sflash_bus.h"
#include "sflash_device.h"
#include "sflash_jedec.h"
#include "sflash_primitive.h"
#endif //SUPPORT_SFLASH_UPDATE_REGS

#ifdef SUPPORT_OTA

#ifndef INDEPENDENT_FLASH_SIZE
#define IMAGE0						"DA16200_IMAGE_ZERO"
#define IMAGE1						"DA16200_IMAGE_ONE"

#define IMAGE0_RTOS					DA16X_FLASH_RTOS_MODEL0
#define IMAGE1_RTOS					DA16X_FLASH_RTOS_MODEL1

#define IMAGE0_RAMLIB				(IMAGE0_RTOS + 0x180000)
#define IMAGE1_RAMLIB				(IMAGE1_RTOS + 0x180000)

#define SFLASH_BOOT_OFFSET_LEN		0x20
#endif	/* INDEPENDENT_FLASH_SIZE */

//#define SFLASH_BOOT_OFFSET			SFLASH_BOOT_INDEX_BASE

#define WAKEUP_SOURCE				0xF80000
#define RAMLIB_JUMP_ADDR			0xF802D0
#define PTIM_JUMP_ADDR				0xF802D4
#define FULLBOOT_JUMP_ADDR			0xF802D8
#ifdef SUPPORT_RECOVER_IDX
#define BOOT_CHECK_CNT_ADDR			0x50080270		// register for interface testing, HW default value is 0x0
#endif

#define BOOT_RETRY_CNT				10

#undef UE_OTA_DEBUG					/* FOR_DEBUGGING */
#ifdef UE_OTA_DEBUG
#define DBG_PRINTF(...)				Printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)				//
#endif

#ifdef INDEPENDENT_FLASH_SIZE
extern int read_boot_offset(int *address);
extern void write_boot_offset(int offset, int address);
#else /* INDEPENDENT_FLASH_SIZE */
static int ue_read_boot_offset(void);
static void ue_write_boot_offset(int offset);
#endif

/* global variable */
int bootoffset;
int image0_ng_flag;
int image1_ng_flag;
int check_img;
HANDLE gsflash;
UINT32	*bootaddr;
#endif	/* SUPPORT_OTA */

/******************************************************************************
 * Types
 ******************************************************************************/

typedef		struct		_thd_info_	{
	char			*name;
	VOID 			(*entry_function)(ULONG);
	UINT			stksize;
	UINT			priority;
	UINT			preempt;
	ULONG			tslice;
	UINT			autorun;
} THD_INFO_TYPE;

typedef		struct		_run_thd_list_	{
	THD_INFO_TYPE		*info;
	struct	_run_thd_list_	*prev;
} RUN_THD_LIST_TYPE;

/******************************************************************************
 * Functions
 ******************************************************************************/
extern void	init_system_step2(void);
extern void	init_system_step3(void);
extern void	init_system_step4(UINT32 rtosmode);

#ifdef SUPPORT_EMERGENCY_BOOT
extern int uart_read_timeout(VOID *p_data, UINT32 p_dlen, UINT32 timeout, UINT32 *recv_len);
#endif

extern void da16200_trace(int index, int value);

#ifdef	SUPPORT_UEBOOT_PROMPT
int Create_Console_Task(void)
{
// Console_OUT's priority should be higher than system_launcher 
// to prevent uart tx errors.
#define tskCONSOLE_OUT_PRIORITY		((UBaseType_t) 4U)
#define tskCONSOLE_IN_PRIORITY		((UBaseType_t) 3U)
	int stack_size;
	BaseType_t	xRtn;
	TaskHandle_t xHandle = NULL;

	extern void trace_scheduler(void* thread_input);
	extern void thread_console_in(void* thread_input);


	//Printf(">> Create Console Tasks...\r\n", __func__);

	/////////////////////////////////////
	// Console output thread
	/////////////////////////////////////
#ifdef	__ENABLE_PERI_CMD__
	stack_size = 256;		// highest stack : utils for secure boot, sys.sbrom
#else	//__ENABLE_PERI_CMD__
	stack_size = 128*2;	/* 128 * sizeof(int) */
#endif	//__ENABLE_PERI_CMD__

	xRtn = xTaskCreate(trace_scheduler,
							"Console_OUT",
							stack_size,
							(void *)NULL,
							tskCONSOLE_OUT_PRIORITY,
							&xHandle);
	if (xRtn != pdPASS) {
		Printf(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "Console_OUT");
	}

	/////////////////////////////////////
	// Console input thread
	/////////////////////////////////////

#if 1 //def	__ENABLE_PERI_CMD__
	stack_size = 256 * 6;		// highest stack : mbedtls testsuite
#else	//__ENABLE_PERI_CMD__
	stack_size = 128 * 5;		/* 640 * sizeof(int) */
#endif	//__ENABLE_PERI_CMD__

	xRtn = xTaskCreate(thread_console_in,
							"Console_IN",
							stack_size,
							(void *)NULL,
							tskCONSOLE_IN_PRIORITY,
							&xHandle);
	if (xRtn != pdPASS) {
		Printf(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "Console_IN");
	}

	return TRUE ;
}
#endif	/* SUPPORT_UEBOOT_PROMPT */

static const	UINT32 bootaddrlist[] = {
/* Original Model */	BOOT_MEM_SET(BOOT_SFLASH)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL0) ,
/* Original Model */	BOOT_MEM_SET(BOOT_SFLASH)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL1) ,
/* Unsecure Boot  */	BOOT_MEM_SET(BOOT_SFLASH_X)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL0) ,
/* Unsecure Boot  */	BOOT_MEM_SET(BOOT_SFLASH_X)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL1) ,
	0 /* tail */
};

static const	UINT32 fullbootaddrlist[] = {
/* Original Model */	BOOT_MEM_SET(BOOT_SFLASH_C)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL0) ,
/* Original Model */	BOOT_MEM_SET(BOOT_SFLASH_C)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL1) ,
/* Unsecure Boot  */	BOOT_MEM_SET(BOOT_SFLASH_C)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL0) ,
/* Unsecure Boot  */	BOOT_MEM_SET(BOOT_SFLASH_C)|BOOT_OFFSET_SET(DA16X_FLASH_RTOS_MODEL1) ,
	0 /* tail */
};


#ifdef SUPPORT_OTA
#ifndef INDEPENDENT_FLASH_SIZE
static int ue_read_boot_offset(void)
{
	HANDLE sflash;
	UINT32 sfdp[8];
	UINT8	*mem_read_data;
	int offset = 0xff;

	mem_read_data = pvPortMalloc(SFLASH_BOOT_OFFSET_LEN + 4);
	memset(mem_read_data, 0x00, SFLASH_BOOT_OFFSET_LEN + 4);

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);
	if (sflash != NULL) {
		sfdp[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, sfdp);

		sfdp[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, sfdp);

		if (SFLASH_INIT(sflash) == TRUE){
			// to prevent a reinitialization
			if (da16x_sflash_setup_parameter((UINT32*)sfdp) == TRUE){
				SFLASH_IOCTL(sflash, SFLASH_SET_INFO, (UINT32*)sfdp);
				DBG_PRINTF(" UEBoot: sflash init success!! \r\n");
			}
			else{
				DBG_PRINTF(" UEBoot: sflash set param fail!! \r\n");
			}
		}
		else {
			sfdp[0] = 0;
			SFLASH_IOCTL(sflash, SFLASH_GET_INFO, (UINT32*)sfdp);

			if (sfdp[3] == 0x40){ 	//SFDP_TABLE_SIZE
				DBG_PRINTF(" UEBoot: sflash get info sfdp table size!! \r\n");
			}
			else {
				DBG_PRINTF(" UEBoot: sflash get info fail sfdp table size!! \r\n");
			}
		}
		SFLASH_READ(sflash, SFLASH_BOOT_OFFSET, mem_read_data, SFLASH_BOOT_OFFSET_LEN);
		DBG_PRINTF(" UEBoot: boot index %s \r\n", mem_read_data);
		// may need 1RTC clock delay
		SFLASH_CLOSE(sflash);
	}
	else {
		DBG_PRINTF(" UEBoot: sflash NULL \r\n");
	}

	if (strcmp((char const*)mem_read_data, IMAGE0) == 0)
	{
		offset = 0;
	}
	else if (strcmp((char const*)mem_read_data, IMAGE1) == 0)
	{
		offset = 1;
	}

	vPortFree(mem_read_data);

	return offset;
}

static void ue_write_boot_offset(int offset)
{
	UINT8	*mem_read_data;

	UINT32	ioctldata[8];
	HANDLE	sflash;
	UINT32	dest_addr;
	UINT32	dest_len;

	dest_addr = SFLASH_BOOT_OFFSET;
	dest_len = SFLASH_BOOT_OFFSET_LEN;

	// Initialize
	mem_read_data = pvPortMalloc(SFLASH_BOOT_OFFSET_LEN + 4);
	memset(mem_read_data, 0x00, SFLASH_BOOT_OFFSET_LEN + 4);

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);
	if (sflash != NULL) {
		if (SFLASH_INIT(sflash) == TRUE){
		}else{
			ioctldata[0] = 0;
			SFLASH_IOCTL(sflash, SFLASH_GET_INFO, (UINT32*)ioctldata);

			if (ioctldata[3] == 0x40){ //SFDP_TABLE_SIZE
				DBG_PRINTF(" UEBoot: sflash get info sfdp table size!! \r\n");
			}
		}

		ioctldata[0] = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, ioctldata);

		// unlock
		ioctldata[0] = dest_addr;
		ioctldata[1] = dest_len;
		SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

		// write �ϱ� ���� erase
		ioctldata[0] = dest_addr;
		ioctldata[1] = dest_len;
		SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata);

		if (offset == 0)
			strcat((char*)mem_read_data,IMAGE0);
		else
			strcat((char*)mem_read_data,IMAGE1);

		DBG_PRINTF(" [%s] %s \r\n", __func__, mem_read_data);
		dest_len = SFLASH_WRITE(sflash, dest_addr, mem_read_data, dest_len);
		DBG_PRINTF(" sflash write %d \r\n", dest_len);
		SFLASH_CLOSE(sflash);
	}
	vPortFree(mem_read_data);
}
#endif /* INDEPENDENT_FLASH_SIZE */

#ifdef	INDEPENDENT_FLASH_SIZE
int	check_image_and_boot(int offset_condition, int offset_address)
#else
int	check_image_and_boot(int offset_condition)
#endif /* INDEPENDENT_FLASH_SIZE */
{
	UINT32	errorcode;
	int	image_rtos;
#ifdef USING_RAMLIB
	int	image_rlib;
#endif
	extern UINT32	p_rom_check_boot_image(UINT32 loadmode, UINT32 bootbase, UINT32 limitaddr);

#ifdef	INDEPENDENT_FLASH_SIZE
	if ( (offset_condition == IMG_NORMAL0) || (offset_condition == IMG_ABNORMAL0)) {
		image_rtos = offset_address;
	}
	else if ( (offset_condition == IMG_NORMAL1) || (offset_condition == IMG_ABNORMAL1)) {
		image_rtos = offset_address;
	}
	else {
		Printf(RED_COLOR " [%s] Error offset_condition: %d \r\n" CLEAR_COLOR, __func__, offset_condition);
		/* default zero */
		image_rtos = DEFAULT_OFFSET_ADDRESS;
	}
#else
	if ( (offset_condition == IMG_NORMAL0) || (offset_condition == IMG_ABNORMAL0)) {
		rtos_base_addr = SFLASH_RTOS_0_BASE;
		image_rtos = IMAGE0_RTOS;
#ifdef USING_RAMLIB
		image_rlib = IMAGE0_RAMLIB;
#endif
		//Printf(" IMG #0 \r\n");
	}
	else if ( (offset_condition == IMG_NORMAL1) || (offset_condition == IMG_ABNORMAL1)) {
		rtos_base_addr = SFLASH_RTOS_1_BASE;
		image_rtos = IMAGE1_RTOS;
#ifdef USING_RAMLIB
		image_rlib = IMAGE1_RAMLIB;
#endif
		//Printf(" IMG #1 \r\n");
	}
	else {
		Printf(RED_COLOR " [%s] Error offset_condition: %d \r\n" CLEAR_COLOR, __func__, offset_condition);
		/* default zero */
		rtos_base_addr = SFLASH_RTOS_0_BASE;
		image_rtos = IMAGE0_RTOS;
	}
#endif

	if (check_img || (offset_condition == IMG_ABNORMAL0) || (offset_condition == IMG_ABNORMAL1)) {
#ifdef USING_RAMLIB
		gsflash = flash_image_open( (DA16X_IMG_BOOT|(sizeof(UINT32)*8)) , image_rlib, NULL);
		if (gsflash == NULL) {
			Printf(" image open fail #%d \r\n", offset_condition);
			//goto IMG_LOAD_FAIL;
			return LOAD_FAIL;
		}

		/* ramlib */
		errorcode = flash_image_check(gsflash, 0, image_rlib);
		Printf(" ramlib check 0x%x \r\n", errorcode);
		if (errorcode == 0) {
			if ( (offset_condition == IMG_NORMAL0) || (offset_condition == IMG_ABNORMAL0)) {
				image0_ng_flag = 1;

				if (image1_ng_flag == 1) {
					return LOAD_FAIL;
				}
			}
			else if ( (offset_condition == IMG_NORMAL1) || (offset_condition == IMG_ABNORMAL1)) {
				image1_ng_flag = 1;

				if (offset_condition == IMG_ABNORMAL1) {
					if (bootoffset != 0) {
#ifdef INDEPENDENT_FLASH_SIZE
						write_boot_offset(0, DEFAULT_OFFSET_ADDRESS);
#else
						ue_write_boot_offset(0);
#endif
					}
					return LOAD_FAIL;
				}

				if (image0_ng_flag == 1) {
					return LOAD_FAIL;
				}
			}

			return CHK_ABNORMAL;
		}
		flash_image_close(gsflash);
#endif
		gsflash = flash_image_open( (DA16X_IMG_BOOT|(sizeof(UINT32)*8)) , image_rtos, NULL);
		if (gsflash == NULL) {
			Printf(" image open fail #%d \r\n", offset_condition);

			return LOAD_FAIL;
		}
		/* RTOS */
		errorcode = flash_image_check(gsflash, 0, image_rtos);
		//Printf(" RTOS check (gsflash:0x%x image_rtos:0x%x code:0x%x) \r\n", gsflash, image_rtos, errorcode);
		if (errorcode == 0) {
			if ( (offset_condition == IMG_NORMAL0) || (offset_condition == IMG_ABNORMAL0)) {
				image0_ng_flag = 1;

				if (image1_ng_flag == 1) {
					return LOAD_FAIL;
				}
			}
			else if ( (offset_condition == IMG_NORMAL1) || (offset_condition == IMG_ABNORMAL1)) {
				image1_ng_flag = 1;

				if (offset_condition == IMG_ABNORMAL1) {
					if (bootoffset != 0) {
#ifdef INDEPENDENT_FLASH_SIZE
						write_boot_offset(0, DEFAULT_OFFSET_ADDRESS);
						bootoffset = 0;
#else
						ue_write_boot_offset(0);
#endif
					}
					return LOAD_FAIL;
				}

				if (image0_ng_flag == 1) {
					return LOAD_FAIL;
				}
			}
			flash_image_close(gsflash);
			return CHK_ABNORMAL;
		}
		flash_image_close(gsflash);
	}

	//Printf(" [%s] Call #%d p_rom_check_boot_image \r\n", __func__, offset_condition);

#ifdef INDEPENDENT_FLASH_SIZE
	if ( (bootoffset != 0) && (offset_condition == IMG_ABNORMAL0)) {
		write_boot_offset(0, SFLASH_RTOS_0_BASE);
		bootoffset = 0;
	}
	else if ( (bootoffset != 1) && (offset_condition == IMG_ABNORMAL1)) {
		write_boot_offset(1, SFLASH_RTOS_1_BASE);
		bootoffset = 1;
	}
#else
	/* overwrite 0x1000 boot offset data to img 1 */
	if ( (bootoffset != 0) && (offset_condition == IMG_ABNORMAL0)) {
		ue_write_boot_offset(0);
	}
	else if ( (bootoffset != 1) && (offset_condition == IMG_ABNORMAL1)) {
		ue_write_boot_offset(1);
	}
#endif

#ifdef USING_RAMLIB
	*(UINT32*)RAMLIB_JUMP_ADDR = image_rlib;
#endif
#ifdef SUPPORT_RECOVER_IDX
	*(UINT32*)BOOT_CHECK_CNT_ADDR += 1 ;
	if (*(UINT32*)BOOT_CHECK_CNT_ADDR > BOOT_RETRY_CNT) {
		/* Change the boot index to another */
		if ( bootoffset == 0) {
			write_boot_offset(1, SFLASH_RTOS_1_BASE);
		}
		else if ( bootoffset == 1 ) {
			write_boot_offset(0, SFLASH_RTOS_0_BASE);
		}
		*(UINT32*)BOOT_CHECK_CNT_ADDR = 0 ;
		reboot_func(SYS_REBOOT); /* Reset */
	}
#endif

	*(UINT32*)PTIM_JUMP_ADDR = 0;
	//*(UINT32*)FULLBOOT_JUMP_ADDR = bootaddrlist[bootoffset];
	*(UINT32*)FULLBOOT_JUMP_ADDR = bootaddr[bootoffset];
	//errorcode = p_rom_check_boot_image(0x01, bootaddrlist[bootoffset], DA16X_SRAM_END);
	errorcode = p_rom_check_boot_image(0x01, bootaddr[bootoffset], DA16X_SRAM_END);		/* MROM_CODE */

	return LOAD_FAIL;
}


#endif	/* SUPPORT_OTA */


#ifdef SUPPORT_SFLASH_UPDATE_REGS

#define SFLASH_PATCH_INTR_MASK	(0x80000000)
#define	SFLASH_PATCH_MAX_ITER_PROBE	64
#define	SFLASH_PATCH_MAX_PROBE_LOOP	32

#ifdef	SFLASH_SLEEP_PRESET
#define	JEDEC_PRESET_SLEEP(sflash, unit,count)	{			\
	sflash->slptim = SFLASH_UTIME(unit,(count-1));			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, 		\
			SFLASH_BUS_PRESET_USLEEP, &(sflash->slptim) );	\
}

#define	JEDEC_PRESET_SLEEPTIME(sflash, sleeptime){			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, 		\
			SFLASH_BUS_PRESET_USLEEP, &sleeptime );			\
}

#define	JEDEC_SET_SLEEP(sflash, unit,count)
#define	JEDEC_SET_SLEEPTIME(sflash, sleeptime)

#define	JEDEC_GET_SLEEPTIME(sflash)		(sflash->slptim)

#else	//SFLASH_SLEEP_PRESET
#define	JEDEC_PRESET_SLEEP(sflash, unit,count)
#define	JEDEC_PRESET_SLEEPTIME(sflash, sleeptime)

#define	JEDEC_SET_SLEEP(sflash, unit,count)	{				\
	sflash->slptim = SFLASH_UTIME(unit,(count-1));			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, 		\
			SFLASH_BUS_SET_USLEEP, &(sflash->slptim) );		\
}

#define	JEDEC_SET_SLEEPTIME(sflash, sleeptime)	{			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, 		\
			SFLASH_BUS_SET_USLEEP, &sleeptime );			\
}

#define	JEDEC_GET_SLEEPTIME(sflash)		(sflash->slptim)

#endif	//SFLASH_SLEEP_PRESET



static UINT32 sflash_patch_jedec_bus_update(SFLASH_HANDLER_TYPE *sflash
				, UINT32 force)
{
	SFLASH_SFDP_TYPE *sfdp;
	UINT32		 phase;
	UINT32		 adrbyte;

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	switch((sflash->accmode & SFLASH_BUS_MASK))
	{
	case SFLASH_BUS_GRP(SFLASH_BUS_444):
		phase = BUS_CADD(SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_044):
		phase = BUS_CADD(SPIPHASE(0,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_004):
		phase = BUS_CADD(SPIPHASE(0,0,2),SPIPHASE(0,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_144):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_114):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_014):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_222):
		phase = BUS_CADD(SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_022):
		phase = BUS_CADD(SPIPHASE(0,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_002):
		phase = BUS_CADD(SPIPHASE(0,0,1),SPIPHASE(0,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_122):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_112):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_012):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_111):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_011):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_001):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);
		break;
	default:
		phase = force;
		break;
	}

	if( sfdp != NULL )
	{
		switch(sfdp->addbyt)
		{
			case	2: // 4 Byte dddress only
				adrbyte = 4;
				break;
			case	1: // 3 Byte or 4 Byte
				if( (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR)
				{
					adrbyte = 4;
				}
				else
				{
					adrbyte = 3;
				}
				break;
			default: // 3 Byte only or others
				adrbyte = 3;
				break;
		}
	}
	else
	{
		adrbyte = 3;
	}

	phase = DARWMDCADD_ADR_UPDATE(phase,adrbyte);


	return phase;
}

#if 0
static int  sflash_patch_jedec_addr(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT32 addr)
{
	if( (cmd != 0) && (sflash->bus != NULL) )
	{
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_patch_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 1, 0, 0); // CMD-ADDR

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, addr, 0
						, NULL, 0, NULL, 0 );
		return (status);
	}
	return (0);
}
#endif

static int  sflash_patch_jedec_read(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 *data, UINT32 len)
{
	if( (cmd != 0) && (sflash->bus != NULL) )
	{
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_patch_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		return bus->transmit( sflash, (UINT32)cmd, 0, 0
					, NULL, 0, data, len );
	}
	return (0);
}


static int  sflash_patch_jedec_write(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 *data, UINT32 len)
{
	if( (cmd != 0) && (sflash->bus != NULL) )
	{
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_patch_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, 0, 0
						, data, len, NULL, 0 );
		return (status);
	}
	return (0);
}

#if 0
static int  sflash_patch_jedec_atomic(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force)
{
	if( (cmd != 0) && (sflash->bus != NULL) )
	{
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_patch_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 0); // CMD-only

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, 0, 0
						, NULL, 0, NULL, 0 );

		return ((status == 0) ? 1 : 0);
	}
	return (0);
}
#endif

static int  sflash_patch_jedec_atomic_check_iter(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 mask, UINT8 flag, UINT32 maxloop)
{
	if( (cmd != 0) && (sflash->bus != NULL) )
	{
		UINT32	loopcnt;
		SFLASH_BUS_TYPE	*bus;
		UINT32	bkupsleep;
		UINT8	rxdata[4];

		bus = sflash->bus;

		force = sflash_patch_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		bkupsleep = JEDEC_GET_SLEEPTIME(sflash);

		for(loopcnt=0; loopcnt< maxloop; loopcnt++ )
		{

			JEDEC_PRESET_SLEEP(sflash, 3,2); // 1msec

			bus->transmit( sflash, (UINT32)cmd, 0, 0
							, NULL, 0, rxdata, 1 );

			if((rxdata[0] & mask) != flag)
			{
				break;
			}

			JEDEC_SET_SLEEP(sflash, 3,2); // 1msec
		}

		JEDEC_PRESET_SLEEPTIME(sflash, bkupsleep);
		JEDEC_SET_SLEEPTIME(sflash, bkupsleep);

		if(loopcnt >= maxloop)
		{
			return 0;
		}
		return 1;
	}
	return(0);
}


static void	sflash_patch_interrupt_deactive(SFLASH_HANDLER_TYPE *sflash)
{
	if( (sflash->chbkup & SFLASH_PATCH_INTR_MASK) == SFLASH_PATCH_INTR_MASK )
	{
		return;
	}

	sflash->chbkup = 0;

	{
		UINT32 bkupintr;

		CHECK_INTERRUPTS(bkupintr);
		PREVENT_INTERRUPTS(1);

		sflash->chbkup = sflash->chbkup | (bkupintr<<16);
	}
	{
		UINT32 bkupcache;

		bkupcache = da16x_cache_status();

		if( (bkupcache == TRUE) ){
			da16x_cache_disable();
		}

		sflash->chbkup = sflash->chbkup | (bkupcache<<8);
	}
	{
		UINT32 curtype;

		curtype = DA16XFPGA_GET_NORTYPE();

		if( curtype == DA16X_SFLASHTYPE_CACHE ){
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP);
		}

		sflash->chbkup = sflash->chbkup | (curtype<<0);
	}

	return;
}

static void	sflash_patch_interrupt_active(SFLASH_HANDLER_TYPE *sflash)
{
	UINT32 bkupintr, bkupcache, wdogstatus, curtype;

	DA16X_UNUSED_ARG(wdogstatus);

	sflash->slptim = 0; // Reset
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash,
				SFLASH_BUS_PRESET_USLEEP, &(sflash->slptim) );

	if( (sflash->chbkup & SFLASH_PATCH_INTR_MASK) == SFLASH_PATCH_INTR_MASK )
	{
		return;
	}

	curtype  = (sflash->chbkup & 0x0ff);
	bkupcache  = (sflash->chbkup >> 8) & 0x0ff;
	bkupintr   = (sflash->chbkup >> 16) & 0x0ff;
	wdogstatus = (sflash->chbkup >> 24) & 0x07f;

	if( curtype == DA16X_SFLASHTYPE_CACHE )
	{
		DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE);
	}

	if( (bkupcache == TRUE) )
	{
		da16x_cache_enable(FALSE);
	}

	PREVENT_INTERRUPTS(bkupintr);

	return;
}

DATA_ALIGN(4) static char *printflagformat = "CMP %d, SRL %d, QE %d\r\n";
DATA_ALIGN(4) static char *printwrsr2format   = "W25Q32JW.WrSR2(0x31):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr1format   = "W25Q32JW.RdSR1(0x05):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr2format   = "W25Q32JW.RdSR2(0x35):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr3format   = "W25Q32JW.RdSR3(0x15):%02x\r\n";

#define	SFLASH_DBG_JTAG(...)	Printf( __VA_ARGS__ )

static int	ue_W25Q32JW_CMP_update(HANDLE handler)
{
	SFLASH_HANDLER_TYPE	*sflash;

	/* Handler Checking */
	if(handler == NULL)
	{
		return FALSE;
	}

	sflash = (SFLASH_HANDLER_TYPE *)handler;

	if( sflash->jedecid == 0xef601615 )
	{
		SFLASH_DEV_TYPE *sflash_dev;
		UINT32		phase, bkbusmode, busmode;
		UINT8		sdata[4], cdata[4];

		sflash_dev = (SFLASH_DEV_TYPE *)(sflash->device);

		sflash_dev->buslock(sflash, TRUE);

		sflash_patch_interrupt_deactive(sflash);

		switch(SFLASH_BUS_GRP(sflash->accmode))
		{
			case	SFLASH_BUS_444:
			case	SFLASH_BUS_044:
			case	SFLASH_BUS_004:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_444);
				break;
			default:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_111);
				break;
		}

		bkbusmode = sflash->accmode;
		sflash->accmode = busmode;	// change busmode

		sflash_unknown_setspeed(sflash, 0); // very slow

		sflash_unknown_set_wren(sflash);

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		phase = sflash_patch_jedec_bus_update(sflash, phase);

		sflash_patch_jedec_read( 0x05, sflash, phase, cdata, 1);
		//SFLASH_DBG_INFO(printrdsr1format,cdata[0]);

		sflash_patch_jedec_read( 0x15, sflash, phase, cdata, 1);
		//SFLASH_DBG_INFO(printrdsr3format,cdata[0]);

		sflash_patch_jedec_read( 0x35, sflash, phase, cdata, 1);
		//SFLASH_DBG_INFO(printrdsr2format,cdata[0]);

		//TEST point: cdata[0] = cdata[0] | 0x40; /// Do NOT modify it !!!

		if( (cdata[0] & 0x41) != 0 ){
			// 0xEF601615 --> CMP, SRL := 0 & QE := 1
			SFLASH_DBG_JTAG(printflagformat
					, ((cdata[0] & 0x40) != 0)
					, ((cdata[0] & 0x01) != 0)
					, ((cdata[0] & 0x02) != 0)
					);

			sdata[0] = (cdata[0] & (~0x41)) |  (1<<1) ;

			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			sflash_patch_jedec_write( 0x31, sflash, phase, sdata, 1);

			SFLASH_DBG_JTAG(printwrsr2format, sdata[0]);

			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_patch_jedec_atomic_check_iter(0x35, sflash
						, phase, (1<<1), 0x00 , (SFLASH_MAX_LOOP<<1));


			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			phase = sflash_patch_jedec_bus_update(sflash, phase);

			sflash_patch_jedec_read( 0x05, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr1format,cdata[0]);

			sflash_patch_jedec_read( 0x15, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr3format,cdata[0]);

			sflash_patch_jedec_read( 0x35, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr2format,cdata[0]);

			SFLASH_DBG_JTAG(printflagformat
					, ((cdata[0] & 0x40) != 0)
					, ((cdata[0] & 0x01) != 0)
					, ((cdata[0] & 0x02) != 0)
					);
		}


		sflash_unknown_set_wrdi(sflash);

		sflash->accmode = bkbusmode;	// restore busmode

		sflash_patch_interrupt_active(sflash);

		sflash_dev->buslock(sflash, FALSE);

	}

	return TRUE;

}

void	ue_sflash_update_registers(void)
{
	UINT32	ioctldata[8];
	HANDLE	sflash;

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);
	if (sflash != NULL) {
		ioctldata[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, ioctldata);

		ioctldata[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, ioctldata);

		if( SFLASH_INIT(sflash) == TRUE ){
			// to prevent a reinitialization
			da16x_sflash_setup_parameter((UINT32*)ioctldata);

		}

		ue_W25Q32JW_CMP_update(sflash);

		SFLASH_CLOSE(sflash);
	}
}

#endif //SUPPORT_SFLASH_UPDATE_REGS

// When the wake-up source is one of POR, RESET or NO-RTM,
// the fast-boot can not be used.
unsigned char ue_check_fastboot(unsigned long mode)
{
	unsigned char por_boot = (mode & WAKEUP_SOURCE_POR) ? 1 : 0;
	unsigned char rtm_off = (mode & WAKEUP_RESET_WITH_RETENTION) ? 0 : 1;
	unsigned long wakeup_source_mask = WAKEUP_SOURCE_EXT_SIGNAL |
								WAKEUP_SOURCE_WAKEUP_COUNTER |
								WAKEUP_WATCHDOG |
								WAKEUP_SENSOR |
								WAKEUP_PULSE |
								WAKEUP_GPIO;
	unsigned char rst_boot = (mode & wakeup_source_mask) ? 0 : 1;

	if (por_boot | rtm_off | rst_boot)
		return 0;

	return 1;
}

/******************************************************************************
 *  boot_loader()
 *
 *  Purpose : Daemon
 *  Input	:
 *  Output	:
 *  Return	:
 ******************************************************************************/
void boot_loader(void *pvParameters)
{
	DA16X_UNUSED_ARG(pvParameters);
	UINT32	errorcode;
	UINT32	promptmode;

#ifdef INDEPENDENT_FLASH_SIZE
	int		offset_address = 0;
#endif

	da16200_trace(0, 0x11110001);

	init_system_step2();

	{
		static const CRYPTO_PRIMITIVE_TYPE sym_crypto_primitive = {
			trc_que_proc_dump,
			trc_que_proc_text,
			trc_que_proc_vprint,
			trc_que_proc_getchar
			,pvPortMalloc,
			vPortFree
		};

		init_crypto_primitives(&sym_crypto_primitive);

		// UEBoot does not need to call these functions !!
		//BOOT: mbedtls_platform_init(NULL);
		//BOOT: DA16X_Crypto_Init(0);
	}

	init_system_step3();
	//Printf(CYAN_COLOR " [%s] boot_loader Started \r\n" CLEAR_COLOR, __func__);

	//ue_init_system_post();	// Clock Change
	init_system_step4(FALSE);
	//Printf(CYAN_COLOR " [%s] sflash initialized. \r\n" CLEAR_COLOR, __func__);

	//DA16X_CLOCK_SCGATE->Off_CC_OTPW = 0;
	{
		#define CLK_GATING_OTP	0x50006048
		#define MEM_BYTE_WRITE(addr, data)	*((volatile UINT8 *)(addr)) = (data)
		MEM_BYTE_WRITE(CLK_GATING_OTP, 0x00);
	}

#ifdef SUPPORT_DA16X_RMA_OTP_ERASE
	{
		UINT32 lcs;
		volatile UINT32 *otpaddr = (UINT32 *)(DA16X_ACRYPT_BASE|0x02084) ;
		extern UINT32 DA16X_SecureBoot_RMA(void);

		if (mbedtls_mng_lcsGet((uint32_t *)&lcs) == 0){
			switch(lcs){
				case 0x00 : // CM
				case 0x01 : // DM
				if((*otpaddr & 0xC0000000) != 0){
					UINT32 pwrctrl;
					pwrctrl = 0x03;
					RTC_IOCTL(RTC_SET_DC_PWR_CONTROL_REG, &pwrctrl);
				}
				break;
				case 0x07 : // RMA
					DA16X_SecureBoot_RMA();
					break;
			}
		}
	}
#endif //SUPPORT_DA16X_RMA_OTP_ERASE

#ifdef	SUPPORT_SECURE_REGION_LOCK
	{
		UINT32 rval;
		// read spare lock bit
		otp_mem_create();
		otp_mem_lock_read((0x3ffc>>2), &rval);
		DBG_PRINTF(" otp lock status 0x%08x \r\n", rval);

		if ( (rval &0x01) != 0x01)
		{
			// if not lock it
			otp_mem_lock_write(0xfff, (0x01<<0));
			otp_mem_lock_read(0xfff, &rval);
			DBG_PRINTF(" OTP lock register 0x%08x \r\n",rval);
		}
		otp_mem_close();
		// read dummy
		otp_mem_create();
		otp_mem_read(0x700, &rval);
		DBG_PRINTF(" temp read 0x%08x \r\n",rval);
		//BOOT: otp_mem_close();
	}
#endif //SUPPORT_SECURE_REGION_LOCK

#ifdef SUPPORT_SFLASH_UPDATE_REGS
	ue_sflash_update_registers();
#endif //SUPPORT_SFLASH_UPDATE_REGS

#ifdef	SUPPORT_UEBOOT_NVRAM
	{
		UINT32 nviodta[2];
		UINT32 data32;

		DA16X_UNUSED_ARG(data32);

		nviodta[0] = (UINT32)"boot.auto.ueboot";
		nviodta[1] = (UINT32)NULL;

		if (SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, nviodta) == TRUE){
			if((VOID *)(nviodta[1]) != NULL){
				data32 = SYS_NVEDIT_READ( nviodta[1], &promptmode, sizeof(UINT32));
			}
		}
	}
#endif	//SUPPORT_UEBOOT_NVRAM

	promptmode = 0;

	switch(promptmode){
		case	1: DBG_PRINTF(" UEBoot: Boot mode!! \r\n"); promptmode = 0; break;
#ifdef	SUPPORT_UEBOOT_PROMPT
		case	2: DBG_PRINTF(" UEBoot: Prompt mode!! \r\n"); promptmode = 2; break;
#endif	//SUPPORT_UEBOOT_PROMPT
		default :	promptmode = 0; break;
	}

#ifdef	SUPPORT_UEBOOT_RELOAD_SFDP
	da16x_sflash_reset_parameters(); // reset
#endif	//SUPPORT_UEBOOT_RELOAD_SFDP

#ifdef SUPPORT_SECURE_BOOT
	unsigned char fast_boot = ue_check_fastboot(da16x_boot_get_wakeupmode());
#else
	unsigned char fast_boot = da16x_boot_get_wakeupmode() & WAKEUP_SOURCE_POR;
#endif
	if (!fast_boot) {
		/*UINT32 sysclock = 120000000;
		_sys_clock_write(&sysclock, sizeof(UINT32));*/
		bootaddr = (UINT32 *) bootaddrlist;
	}
	else {
		bootaddr = (UINT32 *) fullbootaddrlist;
	}

#ifdef SUPPORT_EMERGENCY_BOOT
	/* if there are 3 ESC character on UART, it go to MROM prompt */
	if (da16x_boot_get_wakeupmode() & WAKEUP_SOURCE_POR) {
		/* it check the UART0 port */
		char buffer = 0, esc = ESC;
		UINT32 recv;
		int ret;

		/* wait EMERGENCY_TIMEOUT */
		ret = uart_read_timeout(&buffer, 1, EMERGENCY_TIMEOUT, &recv);

		/* if meets go to MROM*/
		if (ret) {
			if (buffer == esc) {
				// goto MROM prompt
				vTaskDelay(10);
				reboot_func(SYS_RESET); /* Reset */
				while(1);
			}
			else {
				//todo - handling other key
			}
		}
	}
#endif

#ifdef SUPPORT_OTA
	// check sflash 0x9000 boot index
	DBG_PRINTF(" >> wakeup source %x \r\n", da16x_boot_get_wakeupmode());
	DBG_PRINTF(" >> RTC wakeup source %x \r\n", RTC_GET_WAKEUP_SOURCE());
	DBG_PRINTF(" >> RTC reg wakeup source %x \r\n", *(UINT32*)0x50091028);
#ifdef INDEPENDENT_FLASH_SIZE
	bootoffset = read_boot_offset(&offset_address);
#else
	bootoffset = ue_read_boot_offset();
#endif
	if ( (bootoffset != 0) && (bootoffset != 1))
	{
		DBG_PRINTF(" UEBoot: abnormal boot index \r\n");
#if 0	/* F_F_S */
		goto ABNORMAL_IMG0;
#else

#ifdef INDEPENDENT_FLASH_SIZE
		write_boot_offset(0, DEFAULT_OFFSET_ADDRESS);
		bootoffset = 0;
		offset_address = DEFAULT_OFFSET_ADDRESS;
#else
		ue_write_boot_offset(0);
		bootoffset = 0;
#endif	/* INDEPENDENT_FLASH_SIZE */

#endif
	}
#ifdef INDEPENDENT_FLASH_SIZE
	else if (offset_address == UNKNOWN_OFFSET_ADDRESS) {
		write_boot_offset(0, DEFAULT_OFFSET_ADDRESS);
		bootoffset = 0;
		offset_address = DEFAULT_OFFSET_ADDRESS;
	}
#endif	/* INDEPENDENT_FLASH_SIZE */

	if (((*(UINT32 *)WAKEUP_SOURCE) >> 24) == WAKEUP_COUNTER_WITH_RETENTION) {
		check_img = 0;
	}
	else {
		check_img = 1;
	}

	//Printf(" [%s] checkAndFactoryNvram \r\n", __func__);
	//checkAndFactoryNvram(0);

#ifdef INDEPENDENT_FLASH_SIZE
	errorcode = check_image_and_boot(bootoffset, offset_address);
	if (errorcode == LOAD_FAIL) {		/* load fail */
		Printf(" [%s] Error LOAD_FAIL(bootoffset:%d, offset_address:0x%x) \r\n",
											__func__, bootoffset, offset_address);
		goto IMG_LOAD_FAIL;
	}
	else if (errorcode == CHK_ABNORMAL) {	/* abnormal image */
		Printf(" [%s] Error CHK_ABNORMAL(bootoffset:%d) \r\n", __func__, bootoffset);
		if (bootoffset == 0) {
			errorcode = check_image_and_boot(IMG_ABNORMAL1, DA16X_FLASH_RTOS_MODEL1);
		}
		else {
			errorcode = check_image_and_boot(IMG_ABNORMAL0, DA16X_FLASH_RTOS_MODEL0);
		}	

		Printf(" [%s] Error LOAD_FAIL (bootoffset:%d errorcode:%d) \r\n", __func__, bootoffset, errorcode);

		goto IMG_LOAD_FAIL;
	}
#else
	if (bootoffset == 0) {
		errorcode = check_image_and_boot(IMG_NORMAL0);
		if (errorcode == LOAD_FAIL) {		/* load fail */
			Printf(" [%s] Error \r\n", __func__);
		}
		else if (errorcode == CHK_ABNORMAL) {	/* abnormal image */
			check_image_and_boot(IMG_ABNORMAL1);
		}
		goto IMG_LOAD_FAIL;
	}
	else {
		errorcode = check_image_and_boot(IMG_NORMAL1);
		if (errorcode == LOAD_FAIL) {		/* load fail */
			Printf(" [%s] Error \r\n", __func__);
		}
		else if (errorcode == CHK_ABNORMAL) {	/* abnormal image */
			check_image_and_boot(IMG_ABNORMAL0);
		}
		goto IMG_LOAD_FAIL;
	}
#endif
IMG_LOAD_FAIL:
#endif

#ifdef	SUPPORT_UEBOOT_PROMPT

	//Printf(" [%s] create console thread \r\n", __func__);
	Create_Console_Task();

	{
		WDOG_HANDLER_TYPE *wdog;
		wdog = WDOG_CREATE(WDOG_UNIT_0);
		WDOG_INIT(wdog);
		WDOG_IOCTL(wdog, WDOG_SET_OFF, NULL);
		WDOG_CLOSE(wdog);
		Printf(RED_COLOR " [%s] WDOG off on Debugger mode. \r\n" CLEAR_COLOR, __func__);
	}

	for(;;){
		vTaskDelay(1000);
	}	

#else	//SUPPORT_UEBOOT_PROMPT

	Printf(" [%s] Failed image loading \r\n", __func__);
	reboot_func(SYS_RESET); /* Reset */

#endif	//SUPPORT_UEBOOT_PROMPT
}

#if 0

/*
 * For UART1 initializing /////////////////////////////////////////
 */

HANDLE	uart1 = NULL;

static const uart_dma_primitive_type uart1_dma_primitives = {
	&SYS_DMA_COPY,
	&SYS_DMA_REGISTRY_CALLBACK,
	&SYS_DMA_RELEASE_CHANNEL
};

int _uart1_dma_init(HANDLE handler)
{
	UINT32	dma_control;
	UINT32	fifo_level;
	UINT32	dev_unit;
	UINT32	rw_word;
	UINT32	use_word_access;
	HANDLE	dma_channel_tx;
	HANDLE	dma_channel_rx;

#define GET_RX_DMA_CHANNEL(x)	((x == UART_UNIT_0)? CH_DMA_PERI_UART0_RX : 	\
				(x == UART_UNIT_1)?CH_DMA_PERI_UART1_RX : 10)

#define GET_TX_DMA_CHANNEL(x)	((x == UART_UNIT_0)? CH_DMA_PERI_UART0_TX : 	\
				(x == UART_UNIT_1)? CH_DMA_PERI_UART1_TX : 10)


	UART_IOCTL(handler, UART_GET_FIFO_INT_LEVEL, &fifo_level);
	UART_IOCTL(handler, UART_GET_PORT, &dev_unit);
	UART_IOCTL(handler, UART_GET_RW_WORD, &rw_word);
	UART_IOCTL(handler, UART_GET_WORD_ACCESS, &use_word_access);

#if 0
	DBG_PRINTF(" [%s] fifo_level: %d, dev_unit: %d, rw_word: %d, use_word_access: %d \r\n",
		__func__, fifo_level, dev_unit, rw_word, use_word_access);
#endif	/* 0 */

	/* RX */
	if (use_word_access) {
		dma_control = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
	} else {
		dma_control = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_BYTE);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
	}

	if (((fifo_level) >> 3) > 3) {
		DBG_PRINTF(" [%s] Not support DMA burst fifo length !!! \r\n", __func__);
		return -1;
	}

	if (use_word_access) {
		dma_control |= DMA_CHCTRL_BURSTLENGTH(fifo_level >> 3);
	} else {
		dma_control |= DMA_CHCTRL_BURSTLENGTH((fifo_level >> 3) + 2);
	}

	dma_channel_rx = SYS_DMA_OBTAIN_CHANNEL(GET_RX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
	if (dma_channel_rx == NULL) {
		DBG_PRINTF(" [%s] DMA Rx null \r\n", __func__);
		return -1;
	}

	/* TX */
	if (use_word_access) {
		dma_control = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
	} else {
		dma_control = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_BYTE) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
	}

	dma_channel_tx = SYS_DMA_OBTAIN_CHANNEL(GET_TX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
	if (dma_channel_tx == NULL) {
		DBG_PRINTF(" [%s] DMA Tx null ... \r\n", __func__);
		return -1;
	}

	UART_DMA_INIT(handler, dma_channel_tx, dma_channel_rx, &uart1_dma_primitives);

	return 0;
}

HANDLE _uart1_init(int _baud, int _bits, int _parity, int _stopbit, int _flow_ctrl)
{
	HANDLE	handler;

	UINT32	clock = 40000000 * 2;
	UINT32	parity_en = 0;
	UINT32	fifo_en = 1;
	UINT32	DMA = 0;
	UINT32	swflow = 0;
	UINT32	word_access = 0;
	UINT32	rw_word = 0;

	UINT32	baud = (UINT32)_baud;
	UINT32	bits = (UINT32)_bits;
	UINT32	parity = (UINT32)_parity;
	UINT32	stopbit = (UINT32)_stopbit;
	UINT32	flow_ctrl = (UINT32)_flow_ctrl;

	UINT32	temp;

	handler = UART_CREATE(UART_UNIT_1);

	/* Configure uart1 */
	UART_IOCTL(handler, UART_SET_CLOCK, &clock);
	UART_IOCTL(handler, UART_SET_BAUDRATE, &baud);

	PRINTF("\n>>> UART1 : Clock=%d, BaudRate=%d\n", clock, baud);

	temp = UART_WORD_LENGTH(bits) | UART_FIFO_ENABLE(fifo_en)
		| UART_PARITY_ENABLE(parity_en) | UART_EVEN_PARITY(parity)
		| UART_STOP_BIT(stopbit);
	UART_IOCTL(handler, UART_SET_LINECTRL, &temp);

	temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(flow_ctrl);
	UART_IOCTL(handler, UART_SET_CONTROL, &temp);

	temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) | UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
	UART_IOCTL(handler, UART_SET_FIFO_INT_LEVEL, &temp);

	temp = DMA;
	UART_IOCTL(handler, UART_SET_USE_DMA, &temp);

	/* FIFO Enable */
	temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT
		| UART_INTBIT_TIMEOUT | UART_INTBIT_ERROR
		| UART_INTBIT_FRAME | UART_INTBIT_PARITY
		| UART_INTBIT_BREAK | UART_INTBIT_OVERRUN;

	UART_IOCTL(handler, UART_SET_INT, &temp);

	temp = swflow;
	UART_IOCTL(handler, UART_SET_SW_FLOW_CONTROL, &temp);

	temp = word_access;
	UART_IOCTL(handler, UART_SET_WORD_ACCESS, &temp);

	temp = rw_word;
	UART_IOCTL(handler, UART_SET_RW_WORD, &temp);

	if (_uart1_dma_init(handler) < 0) {
		PRINTF("\n[%s] Failed to initialize UART1 DMA !!!\n", __func__);
		return NULL;
	}

	PRINTF(">>> UART1 : DMA Enabled ...\n\n");

	UART_INIT(handler);

	return handler;
}

void host_uart_init(void)	/* move from command_host.c */
{
	UINT32	baud;
	UINT32	bits;
	UINT32	parity;
	UINT32	stopbit;
	UINT32	flow_ctrl;

	uart1_info_t	*uart1_info = NULL;
	int	rtm_len = 0;
	char	rtm_alloc_fail_flag = pdFALSE;

heap_malloc :

	uart1_info = (uart1_info_t *)pvPortMalloc(sizeof(uart1_info_t));

	memset(uart1_info, 0x00, sizeof(uart1_info_t));

	uart1 = _uart1_init(uart1_info->baud,
						uart1_info->bits,
						uart1_info->parity,
						uart1_info->stopbit,
						uart1_info->flow_ctrl);

}

void host_uart_set_baudrate_control(UINT32 baudrate, UINT32 set)
{
	UINT32	temp;

	UART_IOCTL(uart1, UART_SET_BAUDRATE, &baudrate);

	temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(set);
	UART_IOCTL(uart1, UART_SET_CONTROL, &temp);

	UART_CHANGE_BAUERATE(uart1, baudrate);
}
#endif

/* EOF */
