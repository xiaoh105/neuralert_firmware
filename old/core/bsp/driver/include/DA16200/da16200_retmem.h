/**
 ****************************************************************************************
 *
 * @file da16200_retmem.h
 *
 * @brief DA16200 System Controller
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


#ifndef	__da16200_retmem_h__
#define	__da16200_retmem_h__


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"


/******************************************************************************
 *
 *  Structure
 *
 ******************************************************************************/
typedef struct __gpio_retention_info__ GPIO_RETENTION_INFO_TypeDef;

/*
 * ret_info1[15:0]  - GPIO Port Info
 * 	- ret_info1[0]  - GPIOA configured
 * 	- ret_info1[1]  - GPIOB configured
 * 	- ret_info1[2]  - GPIOC configured
 *
 * ret_info2[30:16] - GPIOA Pin Number
 * ret_info2[15:0]  - GPIOB Pin Number
 * ret_info2[30:16] - GPIOC Pin Number
 */
struct __gpio_retention_info__
{
	volatile UINT32 ret_info1;
	volatile UINT32 ret_info2;
};

typedef struct __pinmux_info__ PINMUX_INFO_TypeDef;
struct __pinmux_info__
{
	volatile UINT32 fsel_gpio1;
	volatile UINT32 fsel_gpio2;
};


/******************************************************************************
 *
 *  Offset List in Retained Memory
 *
 ******************************************************************************/

#define	RETMEM_MAGIC_BASE		(DA16200_RETMEM_BASE|0x0000)
//			MPW1 Overwite Issue, MPW2 CRC tag
#define	RETMEM_BOOTMODE_BASE		(DA16200_RETMEM_BASE|0x0004)
//			UINT32 rom_retmemboot[0] : MaskROM/FLASH based RTC WakeUp Model (w/  RETMEM) - M.TIM / U.TIM / U.RTC
//			UINT32 rom_retmemboot[1] : MaskROM/FLASH based Sensor WakeUp Model(w/ RETM)  - U.GPIO
//			UINT32 rom_retmemboot[2] : FLASH based Normal Boot
//
//			UINT32 rom_retmemboot[3] : BootAddress - TAG
//			UINT32 rom_retmemboot[4] : RAMLibrary Load Address (FLASH or RETMEM)
//			UINT32 rom_retmemboot[5] : RAMLibrary Access Point
//			UINT32 rom_retmemboot[6] : RAMLibrary Load Size
//
//			UINT32 rom_retmemboot[7] : OOPS Manager  - on/off | boot | mode | max | address | ...
//			UINT32 rom_retmemboot[8] : Power Control - XTAL | PLL1_LDO (arm) | PLL2_LDO (mac) | DIO_LDO | DCDC0_ANA | RF_LDO | ...
//			UINT32 rom_retmemboot[9] : Clock Control - User Clock Mode
//			UINT32 rom_retmemboot[10]: Pin   Control - User PIN Map
//
//			UINT32 rom_retmemboot[11]: SFLASH Control - Parameters
//			UINT32 rom_retmemboot[12]: SFLASH Control - Computed CRC (Master Image)
//			UINT32 rom_retmemboot[13]: SFLASH Control - Image Size (Master Image)
//			UINT32 rom_retmemboot[14]: SFLASH Control - MAX Speed | Bus Mode | Image Offset
//
#define	RETMEM_SFLASH_BASE		(DA16200_RETMEM_BASE|0x0040)
//			UINT32	magic		: Magic Code
//			UINT32	devid		: Device ID
//			UINT16	offset		: SFDP offset
//			UINT16	length		: SFDP length
//			UINT8	sfdp[64]	: SFDP Table
//			UINT8	extra[28]	: Extra Table
//			UINT8	cmdlst[44]	: Command List
//			UINT8	delay[16]	: Delay Table
#define	RETMEM_RTCM_BASE		(DA16200_RETMEM_BASE|0x0100)

#define RETMEM_GPIO_RETENTION_INFO_OFFSET	0x1d8
#define RETMEM_GPIO_RETENTION_INFO_BASE	(RETMEM_RTCM_BASE|RETMEM_GPIO_RETENTION_INFO_OFFSET)
#define RETMEM_GPIO_RETENTION_INFO		((GPIO_RETENTION_INFO_TypeDef *)RETMEM_GPIO_RETENTION_INFO_BASE)

#define RETMEM_PINMUX_INFO_OFFSET	0x1f0
#define RETMEM_PINMUX_INFO_BASE		(RETMEM_RTCM_BASE|RETMEM_PINMUX_INFO_OFFSET)
#define RETMEM_PINMUX_INFO			((PINMUX_INFO_TypeDef *) RETMEM_PINMUX_INFO_BASE)

#define	RETMEM_LMAC_BASE			(DA16200_RETMEM_BASE | 0x0300)



#endif	/*__da16200_retmem_h__*/

/* EOF */
