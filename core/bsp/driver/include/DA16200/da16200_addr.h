/**
 ****************************************************************************************
 *
 * @file da16200_addr.h
 *
 * @brief DA16200 Address Map
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


#ifndef __da16200_addr_h__
#define __da16200_addr_h__

/******************************************************************************
 *
 *  Target Model	: FPGA or ASIC
 *
 ******************************************************************************/

#define	MEM_END_ADDR(base)				((base ## _BASE + base ## _SIZE)-1)

#define	MEM_MODEL_SPACE_SIZE(base,tgt)	(base ## _SIZE_ ## tgt)
#define	MEM_MODEL_END_ADDR(base,tgt)	((base ## _BASE + base ## _SIZE_ ## tgt)-1)

/******************************************************************************
 *
 *  System Top Address
 *
 ******************************************************************************/
// C-Region
#define	DA16200_ROM_BASE_TOP		0x00000000	/* Interanl ROM : C-Region */
#define	DA16200_FAC_BASE_TOP		0x00000000	/* FLASH & Cache: C-Region */
#define	DA16200_RAM_BASE_TOP		0x00000000	/* Internal RAM : C-Region */

// Sys-Region
#define	DA16200_ROMS_BASE_TOP		0x20000000	/* Interanl ROM : Sys-Region */
#define	DA16200_FACS_BASE_TOP		0x20000000	/* FLASH & Cache: Sys-Region */
#define	DA16200_RAMS_BASE_TOP		0x20000000	/* Internal RAM : Sys-Region */

#define	DA16200_APB_BASE_TOP		0x40000000	/* APB Peripherals        */
#define	DA16200_SYS_BASE_TOP		0x50000000	/* System Peripherals     */

#define	DA16200_PHY_BASE_TOP		0x60000000	/* WLAN PHY & MAC         */

#define	DA16200_FPGABASE_TOP		0x70000000	/* FPGA Only              */
#define	DA16200_SCS_BASE_TOP		0xE0000000	/* System Control Space   */
#define	DA16200_CST_BASE_TOP		0xE0000000	/* reserved for CoreSight */

#define CONVERT_C2SYS_MEM(x)		(x)
#define CONVERT_SYS2C_MEM(x)		(x)

/******************************************************************************
 *
 *  Internal Non-volatile MEM Base
 *
 ******************************************************************************/
// C-Region
#define	DA16200_MSKROM_BASE			(DA16200_ROM_BASE_TOP|0x00000000)
#define	DA16200_MSKROM_SIZE			(0x00060000)
#define	DA16200_MSKROM_END			MEM_END_ADDR(DA16200_MSKROM)

#define	DA16200_RETMEM_BASE			(DA16200_ROM_BASE_TOP|0x00F80000)
#define	DA16200_RETMEM_SIZE			0x0000C000
#define	DA16200_RETMEM_END			MEM_END_ADDR(DA16200_RETMEM)

// Sys-Region
#define	DA16200_MSKROMS_BASE		(DA16200_ROMS_BASE_TOP|0x00000000)
#define	DA16200_MSKROMS_SIZE		(0x00060000)
#define	DA16200_MSKROMS_END			MEM_END_ADDR(DA16200_MSKROMS)

#define	DA16200_RETMEMS_BASE		(DA16200_ROMS_BASE_TOP|0x00F80000)
#define	DA16200_RETMEMS_SIZE		(0x0000C000)
#define	DA16200_RETMEMS_END			MEM_END_ADDR(DA16200_RETMEMS)

/******************************************************************************
 *
 *  Internal Volatile MEM Base
 *
 ******************************************************************************/
// C-Region
#define	DA16200_CACHE_SFLASH_BASE	(DA16200_FAC_BASE_TOP|0x00100000)
#define	DA16200_CACHE_SFLASH_SIZE	(0x00200000)
#define	DA16200_CACHE_SFLASH_END	MEM_END_ADDR(DA16200_CACHE_SFLASH)

#define	DA16200_CACHE_NOR_BASE		(0x18000000)
#define	DA16200_CACHE_NOR_SIZE		(0x01000000)
#define	DA16200_CACHE_NOR_END		MEM_END_ADDR(DA16200_CACHE_NOR)

#define	DA16200_XIP_NOR_BASE		(0x19000000)
#define	DA16200_XIP_NOR_SIZE		(0x01000000)
#define	DA16200_XIP_NOR_END			MEM_END_ADDR(DA16200_XIP_NOR)

#define	DA16200_SRAM_BASE			(DA16200_RAM_BASE_TOP|0x00080000)
#define	DA16200_SRAM_SIZE_ASIC		(0x00080000)
#define	DA16200_SRAM_SIZE_FPGA		(0x00080000)

#ifdef	BUILD_OPT_DA16200_FPGA
#define	DA16200_SRAM_SIZE			MEM_MODEL_SPACE_SIZE(DA16200_SRAM,FPGA)
#define	DA16200_SRAM_END			MEM_MODEL_END_ADDR(DA16200_SRAM,FPGA)
#else	//BUILD_OPT_DA16200_FPGA
#define	DA16200_SRAM_SIZE			MEM_MODEL_SPACE_SIZE(DA16200_SRAM,ASIC)
#define	DA16200_SRAM_END			MEM_MODEL_END_ADDR(DA16200_SRAM,ASIC)
#endif	//BUILD_OPT_DA16200_FPGA

#define	DA16200_SRAM_PORT1			(0x01000000)
#define	DA16200_SRAM_PORT2			(0x02000000)
#define	DA16200_SRAM_PORT3			(0x03000000)
#define	DA16200_SRAM_PORT4			(0x04000000)

// Sys-Region

#define	DA16200_SRAMS_BASE			(DA16200_RAMS_BASE_TOP|0x00080000)
#define	DA16200_SRAMS_SIZE_ASIC		(0x00080000)
#define	DA16200_SRAMS_SIZE_FPGA		(0x00080000)

#ifdef	BUILD_OPT_DA16200_FPGA
#define	DA16200_SRAMS_SIZE			MEM_MODEL_SPACE_SIZE(DA16200_SRAMS,FPGA)
#define	DA16200_SRAMS_END			MEM_MODEL_END_ADDR(DA16200_SRAMS,FPGA)
#else	//BUILD_OPT_DA16200_FPGA
#define	DA16200_SRAMS_SIZE			MEM_MODEL_SPACE_SIZE(DA16200_SRAMS,ASIC)
#define	DA16200_SRAMS_END			MEM_MODEL_END_ADDR(DA16200_SRAMS,ASIC)
#endif	//BUILD_OPT_DA16200_FPGA

#define	DA16200_SRAMS_PORT1			(0x01000000)
#define	DA16200_SRAMS_PORT2			(0x02000000)
#define	DA16200_SRAMS_PORT3			(0x03000000)
#define	DA16200_SRAMS_PORT4			(0x04000000)

#define	DA16200_SRAM16S_BASE		(DA16200_RAMS_BASE_TOP|0x00480000)
#define	DA16200_SRAM16S_SIZE_ASIC	(DA16200_SRAMS_SIZE_ASIC)
#define	DA16200_SRAM16S_SIZE_FPGA	(DA16200_SRAMS_SIZE_FPGA)

#define	DA16200_SRAM32S_BASE		(DA16200_RAMS_BASE_TOP|0x00880000)
#define	DA16200_SRAM32S_SIZE_ASIC	(DA16200_SRAMS_SIZE_ASIC)
#define	DA16200_SRAM32S_SIZE_FPGA	(DA16200_SRAMS_SIZE_FPGA)

/******************************************************************************
 *
 *  WLAN PHY & MAC
 *
 ******************************************************************************/

#define	DA16200_WLAN_MAC_BASE		(DA16200_PHY_BASE_TOP|0x00B00000)
#define	DA16200_WLAN_MAC_SIZE		(0x00100000)
#define	DA16200_WLAN_MAC_END		MEM_END_ADDR(DA16200_WLAN_MAC)

#define	DA16200_WLAN_BBM_BASE		(DA16200_PHY_BASE_TOP|0x00C00000)
#define	DA16200_WLAN_BBM_SIZE		(0x00010000)
#define	DA16200_WLAN_BBM_END		MEM_END_ADDR(DA16200_WLAN_BBM)

#define	DA16200_WLAN_TUN_BASE		(DA16200_PHY_BASE_TOP|0x00F00000)
#define	DA16200_WLAN_TUN_SIZE		(0x00050000)
#define	DA16200_WLAN_TUN_END		MEM_END_ADDR(DA16200_WLAN_TUN)

#define	DA16200_WLAN_LA_BASE		(DA16200_PHY_BASE_TOP|0x01000000)
#define	DA16200_WLAN_LA_SIZE		(0x00200000)
#define	DA16200_WLAN_LA_END			MEM_END_ADDR(DA16200_WLAN_TUN)

/******************************************************************************
 *
 *  APB Peripherals
 *
 ******************************************************************************/

#define	DA16200_STIMER0_BASE		(DA16200_APB_BASE_TOP|0x00000000)
#define	DA16200_STIMER1_BASE		(DA16200_APB_BASE_TOP|0x00001000)
#define	DA16200_DTIMER_BASE			(DA16200_APB_BASE_TOP|0x00002000)
#define	DA16200_WDOG_BASE			(DA16200_APB_BASE_TOP|0x00008000)
#define	DA16200_PWMO_BASE			(DA16200_APB_BASE_TOP|0x0000A000)
#define	DA16200_PWMI_BASE			(DA16200_APB_BASE_TOP|0x0000B000)

#define	DA16200_KDMA_BASE			(DA16200_APB_BASE_TOP|0x0000E000)
#define	DA16200_UDMA_BASE			(DA16200_APB_BASE_TOP|0x0000F000)

#define	DA16200_GPIO0_BASE			(DA16200_APB_BASE_TOP|0x00010000)
#define	DA16200_GPIO1_BASE			(DA16200_APB_BASE_TOP|0x00011000)
#define	DA16200_UART0_BASE			(DA16200_APB_BASE_TOP|0x00012000)

#define	DA16200_UART1_BASE			(DA16200_APB_BASE_TOP|0x00007000)
#define	DA16200_UART2_BASE			(DA16200_APB_BASE_TOP|0x00009000)

#define	DA16200_I2S_BASE			(DA16200_APB_BASE_TOP|0x00014000)
#define	DA16200_I2C_BASE			(DA16200_APB_BASE_TOP|0x00015000)
#define	DA16200_ADC0_BASE			(DA16200_APB_BASE_TOP|0x00016000)

#define	DA16200_ACRYPT_BASE			(DA16200_APB_BASE_TOP|0x00100000)

/******************************************************************************
 *
 *  System Peripherals
 *
 ******************************************************************************/

#define	DA16200_SYSPERI_BASE		(DA16200_SYS_BASE_TOP|0x00000000)

#define	DA16200_SDIO_SLAVE_BASE		(DA16200_SYS_BASE_TOP|0x00010000)
#define	DA16200_EMMC_BASE			(DA16200_SYS_BASE_TOP|0x00030000)
#define	DA16200_FAST_BASE			(DA16200_SYS_BASE_TOP|0x00040000)
#define	DA16200_CRYPTO_BASE			(DA16200_SYS_BASE_TOP|0x00050000)
#define	DA16200_FDMA_BASE			(DA16200_SYS_BASE_TOP|0x00060000)
#define	DA16200_SYSTST_BASE			(DA16200_SYS_BASE_TOP|0x00070000)
#define	DA16200_SLAVE_BASE			(DA16200_SYS_BASE_TOP|0x00080000)
#define	DA16200_RTC_IF_BASE			(DA16200_SYS_BASE_TOP|0x00090000)
#define	DA16200_RTC_BASE			(DA16200_SYS_BASE_TOP|0x00091000)
#define	DA16200_TA2SYNC_BASE		(DA16200_SYS_BASE_TOP|0x000A0000)
#define	DA16200_XFC_BASE			(DA16200_SYS_BASE_TOP|0x000B0000)
#define	DA16200_SPI_BASE			(DA16200_SYS_BASE_TOP|0x000B1000)
#define	DA16200_HSU_BASE			(DA16200_SYS_BASE_TOP|0x000C0000)
#define	DA16200_CCHCTRL_BASE		(DA16200_SYS_BASE_TOP|0x000D0000)
#define	DA16200_PSK_SHA1_BASE		(DA16200_SYS_BASE_TOP|0x000E0000)

#define	DA16200_TKIP_BASE			(DA16200_FAST_BASE|0x0800)
#define	DA16200_SYSDBG_BASE			(DA16200_FAST_BASE|0x0700)
#define	DA16200_BTMCON_BASE			(DA16200_FAST_BASE|0x0000)
#define	DA16200_ZEROING_BASE		(DA16200_FAST_BASE|0x0100)
#define	DA16200_HWCRC32_BASE		(DA16200_FAST_BASE|0x0200)
#define	DA16200_TCS32_BASE			(DA16200_FAST_BASE|0x0300)
#define	DA16200_PRNG_BASE			(DA16200_FAST_BASE|0x0400)

#define	DA16200_SYS_SECURE_BASE		(DA16200_SYS_BASE_TOP|0x00004000)
#define	DA16200_ABM_SECURE_BASE		(DA16200_SYS_BASE_TOP|0x00005000)

/******************************************************************************
 *
 *  External Memory (FPGA Only)
 *
 ******************************************************************************/

#define	DA16200_FPGA_ZBT_BASE		(DA16200_FPGABASE_TOP|0x01000000)
#define	DA16200_FPGA_ZBT_SIZE		(0x01000000)
#define	DA16200_FPGA_ZBT_END		MEM_END_ADDR(DA16200_FPGA_ZBT)

#define	DA16200_FPGA_ETH_BASE		(DA16200_FPGABASE_TOP|0x02000000)

#define	DA16200_FPGA_SDRAM_BASE		(DA16200_FPGABASE_TOP|0x04000000)
#define	DA16200_FPGA_SDRAM_SIZE		(0x04000000)
#define	DA16200_FPGA_SDRAM_END		MEM_END_ADDR(DA16200_FPGA_SDRAM)

#define	DA16200_FPGA_PERI_BASE		(DA16200_FPGABASE_TOP|0x0F000000)

#endif	/*__da16200_addr_h__*/

/* EOF */
