/**
 ****************************************************************************************
 *
 * @file da16200_map.h
 *
 * @brief DA16200 Map
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

#ifndef __da16200_map_h__
#define __da16200_map_h__

#include "sdk_type.h"

/******************************************************************************
 *
 *  Type & CPU Core
 *
 ******************************************************************************/
#include "da16x_types.h"
#include "da16200_arch.h"

/******************************************************************************
 *
 *  Address Space
 *
 ******************************************************************************/

#include "da16200_addr.h"

/******************************************************************************
 *
 *  Register Map
 *
 ******************************************************************************/

#include "da16200_regs.h"
#include "common_def.h"

/* Defined a divider which is used for system clock calculation.
 * System clock would be 30Mhz ~ 160MHz.
 */
enum _sys_clock_divider {
	SYSCLK_DIV_160MHZ = 3,
	SYSCLK_DIV_120MHZ,
	SYSCLK_DIV_96MHZ,
	SYSCLK_DIV_80MHZ,
	SYSCLK_DIV_68MHZ,
	SYSCLK_DIV_60MHZ,
	SYSCLK_DIV_53MHZ,
	SYSCLK_DIV_48MHZ,
	SYSCLK_DIV_43MHZ,
	SYSCLK_DIV_40MHZ,
	SYSCLK_DIV_36MHZ,
	SYSCLK_DIV_34MHZ,
	SYSCLK_DIV_32MHZ,
	SYSCLK_DIV_30MHZ,
	SYSCLK_MAX,
};

/******************************************************************************
 *
 *  Legacy Macro support
 *
 ******************************************************************************/

#define DA16X_SYSTEM_XTAL			40000000UL

#ifdef	BUILD_OPT_DA16200_ASIC
  /* Defined PLL clock as constant value that is from da16x_pll_get_speed(). */
  #define DA16X_PLL_CLOCK			480000000UL
  #define DA16X_SYS_CLK_DIV			SYSCLK_DIV_120MHZ
  #define DA16X_SYSTEM_CLOCK		(DA16X_PLL_CLOCK / DA16X_SYS_CLK_DIV)
#else	//BUILD_OPT_DA16200_ASIC
  #define DA16X_SYSTEM_CLOCK		50000000UL
#endif	//BUILD_OPT_DA16200_ASIC

#define DA16X_SYSTEM_RC32K			32768UL
#define DA16X_SYSTEM_X32K			32768UL

/******************************************************************************
 *
 * Legacy Memory Map support
 *
 ******************************************************************************/
// C-Region
#define	DA16X_ROM_BASE			DA16200_MSKROM_BASE
#define	DA16X_ROM_SIZE			DA16200_MSKROM_SIZE
#define	DA16X_ROM_END			DA16200_MSKROM_END

#define	DA16X_SRAM_BASE			DA16200_SRAM_BASE
#define	DA16X_SRAM_SIZE			DA16200_SRAM_SIZE
#define	DA16X_SRAM_END			DA16200_SRAM_END

#define DA16X_SRAM_PORT1		(DA16200_SRAM_BASE|DA16200_SRAM_PORT1)
#define DA16X_SRAM_PORT2		(DA16200_SRAM_BASE|DA16200_SRAM_PORT2)
#define DA16X_SRAM_PORT3		(DA16200_SRAM_BASE|DA16200_SRAM_PORT3)
#define DA16X_SRAM_PORT4		(DA16200_SRAM_BASE|DA16200_SRAM_PORT4)

#define	DA16X_RETMEM_BASE		DA16200_RETMEM_BASE
#define	DA16X_RETMEM_SIZE		DA16200_RETMEM_SIZE
#define	DA16X_RETMEM_END		DA16200_RETMEM_END

#define	DA16X_CACHE_BASE		DA16200_CACHE_SFLASH_BASE
#define	DA16X_CACHE_SIZE		DA16200_CACHE_SFLASH_SIZE
#define	DA16X_CACHE_END			DA16200_CACHE_SFLASH_END

#define	DA16X_NOR_CACHE_BASE	DA16200_CACHE_NOR_BASE
#define	DA16X_NOR_CACHE_SIZE	DA16200_CACHE_NOR_SIZE
#define	DA16X_NOR_CACHE_END		DA16200_CACHE_NOR_END

#define	DA16X_NOR_XIP_BASE		DA16200_XIP_NOR_BASE
#define	DA16X_NOR_XIP_SIZE		DA16200_XIP_NOR_SIZE
#define	DA16X_NOR_XIP_END		DA16200_XIP_NOR_END

// Sys-Region
#define	DA16X_ROMS_BASE			DA16200_MSKROMS_BASE
#define	DA16X_ROMS_SIZE			DA16200_MSKROMS_SIZE
#define	DA16X_ROMS_END			DA16200_MSKROMS_END

#define	DA16X_SRAMS_BASE		DA16200_SRAMS_BASE
#define	DA16X_SRAMS_SIZE		DA16200_SRAMS_SIZE
#define	DA16X_SRAMS_END			DA16200_SRAMS_END

#define DA16X_SRAMS_PORT1		(DA16200_SRAMS_BASE|DA16200_SRAMS_PORT1)
#define DA16X_SRAMS_PORT2		(DA16200_SRAMS_BASE|DA16200_SRAMS_PORT2)
#define DA16X_SRAMS_PORT3		(DA16200_SRAMS_BASE|DA16200_SRAMS_PORT3)
#define DA16X_SRAMS_PORT4		(DA16200_SRAMS_BASE|DA16200_SRAMS_PORT4)

#define	DA16X_RETMEMS_BASE		DA16200_RETMEMS_BASE
#define	DA16X_RETMEMS_SIZE		DA16200_RETMEMS_SIZE
#define	DA16X_RETMEMS_END		DA16200_RETMEMS_END


#define	DA16X_ZBTSRAM_BASE		DA16200_FPGA_ZBT_BASE
#define	DA16X_ZBTSRAM_SIZE		DA16200_FPGA_ZBT_SIZE
#define	DA16X_ZBTSRAM_END		DA16200_FPGA_ZBT_END

#define	DA16X_SDRAM_BASE		DA16200_FPGA_SDRAM_BASE
#define	DA16X_SDRAM_SIZE		DA16200_FPGA_SDRAM_SIZE
#define	DA16X_SDRAM_END			DA16200_FPGA_SDRAM_END

#define	DA16X_SMSC9220_BASE		DA16200_FPGA_ETH_BASE

#define DA16X_NOR_BASE			DA16200_XIP_NOR_BASE

//////////////////////////////////// SFLASH_MAP_CONFIG start /////////////////////////////////////////////////////
#define SFLASH_2ND_BOOTLOADER_BASE	0x00000000	// 2ndary Bootloader
#define SFLASH_BOOT_INDEX_BASE		0x00022000	// Boot index area	// SFLASH_MAP


/*
 * <DA16200/DA16600 4MB SFALSH Map : Standard >
 *
 * 0x0000_0000 2nd Bootloader			0x22000		(   136 KB )
 * 0x0002_2000 Boot Index				0x1000		(     4 KB )
 *
 * 0x0002_3000 RTOS #0					0x1BF000	( 1,788 KB )
 * 0x001E_2000 RTOS #1		 			0x1BF000	( 1,788 KB )
 * 0x001E_2000 Reserved #0				0x1000		(     4 KB )
 *
 * 0x003A_2000 Debug/RMA Certificate	0x1000		(     4 KB )
 * 0x003A_3000 TLS Certificate Key #1	0x4000		(    16 KB )
 * 0x003A_7000 TLS Certificate Key #2	0x4000		(    16 KB )
 * 0x003A_B000 NVRAM #0					0x1000		(     4 KB )
 * 0x003A_C000 NVRAM #1					0x1000		(     4 KB )
 *
 */

/* RTOS */
#define SFLASH_RTOS_0_BASE				0x00023000	// RTOS	#0
#define SFLASH_RTOS_1_BASE				0x001E2000	// RTOS	#1

#define SFLASH_RESERVED_0				0x003A1000	// 4KB
#define SFLASH_RMA_CERTIFICATE			0x003A2000	// Debug/RMA Certificate

/* TLS Certificate Key #0 */
#define SFLASH_ROOT_CA_ADDR1			0x003A3000
#define SFLASH_CERTIFICATE_ADDR1		(SFLASH_ROOT_CA_ADDR1 + 0x1000)
#define SFLASH_PRIVATE_KEY_ADDR1		(SFLASH_ROOT_CA_ADDR1 + 0x2000)
#define SFLASH_DH_PARAMETER1			(SFLASH_ROOT_CA_ADDR1 + 0x3000)

/* TLS Certificate Key #1 */
#define SFLASH_ROOT_CA_ADDR2			0x003A7000
#define SFLASH_CERTIFICATE_ADDR2		(SFLASH_ROOT_CA_ADDR2 + 0x1000)
#define SFLASH_PRIVATE_KEY_ADDR2		(SFLASH_ROOT_CA_ADDR2 + 0x2000)
#define SFLASH_DH_PARAMETER2			(SFLASH_ROOT_CA_ADDR2 + 0x3000)

/* NVRAM Area */
#define SFLASH_NVRAM_ADDR				0x003AB000	// NVRAM #0
#define SFLASH_NVRAM_BACKUP				0x003AC000	// NVRAM #1

#if defined (__BLE_COMBO_REF__)	// #######################################################

/*
 * 0x003A_D000 BLE Area           0x10000 ~ 0x15000 ( 64 KB MIN ~ 84 KB MAX) // 0x003AD000
 *
 *       BLE Firmware Size Max    0x10000 ~ 0x14000 ( 64 KB MIN ~ 80 KB MAX)
 *		 BLE Security DB          0x00000 ~ 0x01000	( 00 KB MIN ~ 04 KB MAX)
 *
 * BLE Area size can be 64 KB upto 84 KB 
 *                             depending on __BLE_IMG_SIZE__ and __MULTI_BONDING_SUPPORT__
 *			   
 *
 * 0x003B_D000 User Area #1       0x30000		( 192 KB )                   // 0x003BD000
 * or
 * 0x003C_2000 User Area #1       0x2B000		( 172 KB )                   // 0x003C2000
 */

/* DA14531 BLE Area Component Size */

#define SFLASH_14531_BLE_FW_MAX_SIZE	   0x00014000

#if (__BLE_IMG_SIZE__ >  SFLASH_14531_BLE_FW_MAX_SIZE)
#error "Config error: __BLE_IMG_SIZE__ is too big !!!"
#endif

#ifdef __MULTI_BONDING_SUPPORT__
#define SFLASH_14531_BLE_SEC_DB_SIZE	   0x00001000
#else
#define SFLASH_14531_BLE_SEC_DB_SIZE	   0x00
#endif

#define SFLASH_14531_BLE_AREA_START		  (SFLASH_NVRAM_BACKUP + SFLASH_NVRAM_BACKUP - SFLASH_NVRAM_ADDR)
#define SFLASH_14531_BLE_AREA_SIZE 		  (__BLE_IMG_SIZE__ + SFLASH_14531_BLE_SEC_DB_SIZE)
#define SFLASH_14531_BLE_AREA_END		  (SFLASH_14531_BLE_AREA_START + SFLASH_14531_BLE_AREA_SIZE)

/* DA14531 BLE Firmware Download start */
#define SFLASH_BLE_FW_BASE				  (SFLASH_14531_BLE_AREA_START)

/* DA14531 BLE Security DB Area start */
#define SFLASH_USER_AREA_BLE_SECURITY_DB  (SFLASH_BLE_FW_BASE + __BLE_IMG_SIZE__)

/* SFLASH User Area */
#define SFLASH_USER_AREA_1_START		  (SFLASH_14531_BLE_AREA_START + SFLASH_14531_BLE_AREA_SIZE)
#define SFLASH_USER_AREA_1_END			  0x003ECFFF

#else	// __DA16200__  ##################################################################

/*
 * 0x003A_D000 User Area #0				0x40000		(   256 KB )
 */

/* SFLASH User Area */
#define SFLASH_USER_AREA_1_START		0x003AD000
#define SFLASH_USER_AREA_1_END			0x003ECFFF


#endif	// __DA16200__	##################################################################

/*
 * <DA16200 4MB SFALSH Map : Standard>
 *
 * 0x003E_D000 TLS Certitivate Key #3		0x1000		(    16 KB )
 * 0x003F_1000 TLS Certitivate Key #4		0x1000		(    16 KB )
 * 0x003F_5000 NVRAM FOOTPRINT				0x1000		(     4 KB )
 * 0x003F_6000 AT-CMD TLS Certi (#1~#10)	0xA000		(    40 KB )
 * 0x0040_0000 END_ADDR
 */

/* TLS Certificate WPA Enterprise */
#define SFLASH_ENTERPRISE_ROOT_CA		0x003ED000
#define SFLASH_ENTERPRISE_CERTIFICATE	(SFLASH_ENTERPRISE_ROOT_CA + 0x1000)
#define SFLASH_ENTERPRISE_PRIVATE_KEY	(SFLASH_ENTERPRISE_ROOT_CA + 0x2000)
#define SFLASH_ENTERPRISE_DH_PARAMETER	(SFLASH_ENTERPRISE_ROOT_CA + 0x3000)

/* TLS Certificate Extension */
#define SFLASH_RESERVED_ROOT_CA			0x003F1000
#define SFLASH_RESERVED_CERTIFICATE		(SFLASH_RESERVED_ROOT_CA + 0x1000)
#define SFLASH_RESERVED_PRIVATE_KEY		(SFLASH_RESERVED_ROOT_CA + 0x2000)
#define SFLASH_RESERVED_DH_PARAMETER	(SFLASH_RESERVED_ROOT_CA + 0x3000)

/* NVRAM FOOTPRINT */
#define SFLASH_NVRAM_FOOTPRINT			0x003F5000	// 4KB

/* TLS Certificate for AT-CMD extension */
#define SFLASH_ATCMD_TLS_CERT_EXTENSION	0x003F6000
#define SFLASH_ATCMD_TLS_CERT_01		SFLASH_ATCMD_TLS_CERT_EXTENSION
#define SFLASH_ATCMD_TLS_CERT_02		(SFLASH_ATCMD_TLS_CERT_01 + 0x1000)	// 0x3F7000
#define SFLASH_ATCMD_TLS_CERT_03		(SFLASH_ATCMD_TLS_CERT_02 + 0x1000)	// 0x3F8000
#define SFLASH_ATCMD_TLS_CERT_04		(SFLASH_ATCMD_TLS_CERT_03 + 0x1000)	// 0x3F9000
#define SFLASH_ATCMD_TLS_CERT_05		(SFLASH_ATCMD_TLS_CERT_04 + 0x1000)	// 0x3FA000
#define SFLASH_ATCMD_TLS_CERT_06		(SFLASH_ATCMD_TLS_CERT_05 + 0x1000)	// 0x3FB000
#define SFLASH_ATCMD_TLS_CERT_07		(SFLASH_ATCMD_TLS_CERT_06 + 0x1000)	// 0x3FC000
#define SFLASH_ATCMD_TLS_CERT_08		(SFLASH_ATCMD_TLS_CERT_07 + 0x1000)	// 0x3FD000
#define SFLASH_ATCMD_TLS_CERT_09		(SFLASH_ATCMD_TLS_CERT_08 + 0x1000)	// 0x3FE000
#define SFLASH_ATCMD_TLS_CERT_10		(SFLASH_ATCMD_TLS_CERT_09 + 0x1000)	// 0x3FF000


/**********************************************************/

#define SFLASH_USER_AREA_0_START		SFLASH_USER_AREA_1_START
#define SFLASH_CM_0_BASE				SFLASH_USER_AREA_1_START
#define SFLASH_CM_1_BASE				SFLASH_USER_AREA_1_START

#define SFLASH_NVRAM_BACKUP_SZ			(SFLASH_NVRAM_BACKUP - SFLASH_NVRAM_ADDR)

/* User alias */
#define SFLASH_USER_AREA_START			SFLASH_USER_AREA_1_START
#define SFLASH_USER_AREA_END			SFLASH_USER_AREA_1_END

/* Alloc size */
#define SFLASH_ALLOC_SIZE_RTOS			(SFLASH_RTOS_1_BASE - SFLASH_RTOS_0_BASE)
#define SFLASH_ALLOC_SIZE_USER			(SFLASH_USER_AREA_1_END - SFLASH_USER_AREA_1_START)

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// SFLASH_MAP_CONFIG end ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

#define SFLASH_VER_SIZE 		0x00000020
#define SFLASH_VER_OFFSET 		0x00000010
#define	SF_SECTOR_SZ			0x1000	// 4KB

/******************************************************************************
 *
 * Legacy System Map support
 *
 *     The postfix number means an instance number.
 ******************************************************************************/

//--------------------------------------------------------------------
// DA16200 specific SYSCON
//--------------------------------------------------------------------

#define VER_BASE				(DA16200_BOOTCON_BASE)

//--------------------------------------------------------------------
// DMA
//--------------------------------------------------------------------

#define UDMA_BASE_0 			DA16200_UDMA_BASE
#define KDMA_BASE_0 			DA16200_KDMA_BASE

#define FDMA_BASE_0 			DA16200_FDMA_BASE

//--------------------------------------------------------------------
// TIMERs, WatchDog
//--------------------------------------------------------------------

#define	STIMER_BASE_0			DA16200_STIMER0_BASE
#define	STIMER_BASE_1			DA16200_STIMER1_BASE
#define	DTIMER_BASE_0			DA16200_DTIMER_BASE
#define	WDOG_BASE_0				DA16200_WDOG_BASE

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

#define	DA16X_TKIP_BASE			DA16200_TKIP_BASE
#define	DA16X_SYSDBG_BASE		DA16200_SYSDBG_BASE
#define	DA16X_ZEROING_BASE		DA16200_ZEROING_BASE
#define	DA16X_BTMCON_BASE		DA16200_BTMCON_BASE
#define	DA16X_PRNG_BASE			DA16200_PRNG_BASE

#define	DA16X_TA2SYNC_BASE		DA16200_TA2SYNC_BASE
#define DA16X_XFC_BASE			DA16200_XFC_BASE
#define DA16X_SPI_BASE			DA16200_SPI_BASE

#define	DA16X_HSU_BASE			DA16200_HSU_BASE
#define	DA16X_CCHCTRL_BASE		DA16200_CCHCTRL_BASE

/******************************************************************************
 *
 * Legacy Peripheral Map support
 *
 *     The postfix number means an instance number.
 ******************************************************************************/

//--------------------------------------------------------------------
// XSPI
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// UART
//--------------------------------------------------------------------

#define UART_BASE_0				DA16200_UART0_BASE
#define UART_BASE_1				DA16200_UART1_BASE
#define UART_BASE_2				DA16200_UART2_BASE

//--------------------------------------------------------------------
// I2S, I2C, PWM
//--------------------------------------------------------------------

#define I2S_BASE_0				DA16200_I2S_BASE
#define I2C_BASE_0				DA16200_I2C_BASE
#define	PWMO_BASE_0				DA16200_PWMO_BASE
#define	PWMI_BASE_0				DA16200_PWMI_BASE

//--------------------------------------------------------------------
// EMMC, SDIO
//--------------------------------------------------------------------

#define EMMC_BASE_0				DA16200_EMMC_BASE
#define SDIO_SLAVE_BASE_0		DA16200_SDIO_SLAVE_BASE

//--------------------------------------------------------------------
// RTC
//--------------------------------------------------------------------

#define RTC_BASE_0				DA16200_RTC_BASE
#define	RTCIF_BASE_0			DA16200_RTC_IF_BASE

//--------------------------------------------------------------------
// GPIO
//--------------------------------------------------------------------

#define	GPIO_BASE_0				DA16200_GPIO0_BASE
#define	GPIO_BASE_1				DA16200_GPIO1_BASE

//--------------------------------------------------------------------
// Secure
//--------------------------------------------------------------------
#define	DA16X_ACRYPT_BASE		DA16200_ACRYPT_BASE
#define	SYS_SECURE_BASE			DA16200_SYS_SECURE_BASE
#define	ABM_SECURE_BASE			DA16200_ABM_SECURE_BASE

/******************************************************************************
 *
 * Legacy Peripheral Map support
 *
 *     The postfix number means an instance number.
 ******************************************************************************/

//--------------------------------------------------------------------
// MAC
//--------------------------------------------------------------------

#define	DA16X_MAC_BASE			(DA16200_WLAN_MAC_BASE | 0x000000)

//--------------------------------------------------------------------
// MODEM
//--------------------------------------------------------------------

#define	DA16X_MODEM_BASE		(DA16200_WLAN_BBM_BASE | 0x000000)

//--------------------------------------------------------------------
// LA
//--------------------------------------------------------------------

#define	DA16X_LA_CFG_BASE		(DA16200_WLAN_LA_BASE | 0x100000)
#define	DA16X_LA_DUMP_BASE		(DA16200_WLAN_LA_BASE | 0x000000)

//--------------------------------------------------------------------
// TUNER
//--------------------------------------------------------------------

#define	DA16X_FCI_RF_BASE		(DA16200_WLAN_TUN_BASE | 0x00000)
#define	DA16X_WRAPPER_BASE		(DA16200_WLAN_TUN_BASE | 0x40000)

#endif	/*__da16200_map_h__*/

/* EOF */
