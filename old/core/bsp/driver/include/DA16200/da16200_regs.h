/**
 ****************************************************************************************
 *
 * @file da16200_regs.h
 *
 * @brief DA16200 Register Map
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


#ifndef __da16200_regs_h__
#define __da16200_regs_h__

#include "da16200_addr.h"
#include "common_def.h"

/******************************************************************************
 *
 *  DA16200 Registers
 *
 ******************************************************************************/
#define	DA16200_SYSBOOT_BASE		(DA16200_SYSPERI_BASE|0x00000000)
#define	DA16200_COMMON_BASE		(DA16200_SYSPERI_BASE|0x00001000)
#define	DA16200_DIPCFG_BASE		(DA16200_SYSPERI_BASE|0x00001100)
#define	DA16200_DIOCFG_BASE		(DA16200_SYSPERI_BASE|0x00001200)
#define	DA16200_RFMDMC_BASE		(DA16200_SYSPERI_BASE|0x00001300)
#define	DA16200_AIPCFG_BASE		(DA16200_SYSPERI_BASE|0x00001400)
#define	DA16200_DAC10C_BASE		(DA16200_SYSPERI_BASE|0x00001500)
#define	DA16200_FDAC14_BASE		(DA16200_SYSPERI_BASE|0x00001600)

#define	DA16200_BOOTCON_BASE		(DA16200_SYSPERI_BASE|0x00002000)
#define	DA16200_SYSCLOCK_BASE		(DA16200_SYSPERI_BASE|0x00003000)
#define	DA16200_SCLKGATE_BASE		(DA16200_SYSPERI_BASE|0x00006000)
#define	DA16200_SYSDBGTLITE_BASE		(DA16200_FAST_BASE   |0x00000710)

#define	DA16200_FPGASYSCON_BASE		(DA16200_FPGA_PERI_BASE|0x00000000)

typedef		struct	_da16200_sysboot_	DA16200_SYSBOOT_TypeDef;

typedef		struct	_da16200_common_		DA16200_COMMON_TypeDef;
typedef		struct	_da16200_dipcfg_		DA16200_DIPCFG_TypeDef;
typedef		struct	_da16200_diocfg_		DA16200_DIOCFG_TypeDef;
typedef		struct	_da16200_rfmdmc_		DA16200_RFMDMC_TypeDef;
typedef		struct	_da16200_aipcfg_		DA16200_AIPCFG_TypeDef;
typedef		struct	_da16200_dac10c_		DA16200_DAC10C_TypeDef;
typedef		struct	_da16200_fdac14_		DA16200_FDAC14_TypeDef;

typedef		struct	_da16200_bootcon_	DA16200_BOOTCON_TypeDef;
typedef		struct	_da16200_sysclock_	DA16200_SYSCLOCK_TypeDef;
typedef		struct	_da16200_sclkgate_	DA16200_SCLKGATE_TypeDef;
typedef		struct	_da16200_litedbgt_	DA16200_LITESYSDBGT_TypeDef;
typedef		struct  _da16200_psk_sha1_	DA16200_PSK_SHA1_TypeDef;
typedef		struct	_da16200_slavectrl_	DA16200_SLAVECTRL_TypeDef;

typedef		struct	_da16200fpga_syscon_	DA16200FPGA_SYSCON_TypeDef;

/******************************************************************************
 *
 *  DA16200 SYSBOOT
 *
 ******************************************************************************/

struct		_da16200_sysboot_
{
	volatile UINT32	BOOT_MODE;
	volatile UINT32	BOOT_BASE_ADDR; // for VSIM
	volatile UINT32 _reserved0[62];

	volatile UINT32	JTAG_REQ;
	volatile UINT32	JTAG_PIN_STA;
};

#define DA16200_SYSBOOT		((DA16200_SYSBOOT_TypeDef *)DA16200_SYSBOOT_BASE)

// BOOT_MODE
#define	SYSGET_BOOTMODE()	((DA16200_SYSBOOT->BOOT_MODE)&0x00F)
#define	SYSSET_BOOTMODE(x)	((DA16200_SYSBOOT->BOOT_MODE) = ((x)&0x0F))

#define	BOOT_ROM		0x00
#define	BOOT_SRAM		0x01
#define	BOOT_RETMEM		0x02
#define	BOOT_OTP		0x03
#define	BOOT_SFLASH		0x04
#define	BOOT_SFLASH_X		0x05
#define	BOOT_SFLASH_C		0x06
#define	BOOT_EFLASH		0x07
#define	BOOT_EFLASH_X		0x08
#define	BOOT_EFLASH_C		0x09
#define	BOOT_NOR_X		0x0B
#define	BOOT_NOR_C		0x0C
#define	BOOT_NOR		0x0D
#define	BOOT_ZBTSRAM		0x0E
#define	BOOT_SDRAM		0x0F

// BOOT_BASE_ADDR
#define	SYSGET_BOOTADDR()	(DA16200_SYSBOOT->BOOT_BASE_ADDR)

/******************************************************************************
 *
 *  DA16200 COMMON
 *
 ******************************************************************************/

struct		_da16200_common_
{
	volatile UINT32	PLL1_ARM;
	volatile UINT32	XTAL40Mhz;
};

#define DA16200_COMMON		((DA16200_COMMON_TypeDef *)DA16200_COMMON_BASE)


#define	PLL1_ARM_LOCK		(1<<13)
#define	PLL1_ARM_LF_CTRL(x)	(((x)&0x3)<<11)
#define	PLL1_ARM_LCYCLE(x)	(((x)&0x3)<< 9)
#define	PLL1_ARM_GET_LCYCLE(x)	(((x)>> 9)&0x3)

#define	PLL1_ARM_ENABLE		(1<<8)
#define	PLL1_ARM_VCO_SEL(x)	(((x)&0x3)<< 6)
#define	PLL1_ARM_GET_VCO_SEL(x)	(((x)>> 6)&0x3)
#define	PLL1_ARM_DIV_SEL(x)	(((x)&0x3f)<< 0)
#define	PLL1_ARM_GET_DIV_SEL(x)	(((x)>> 0)&0x3f)


#define	BB2XTAL_EN		(1<<0) // Read Only
#define	BB2XTAL_RFEN		(1<<1)
#define	BB2XTAL_DPLL1EN		(1<<2)
#define	BB2XTAL_DPLL2EN		(1<<3)
#define	BB2XTAL_ADCCLKEN	(1<<4)
#define	BB2XTAL_ENMASK		(0x1e)

#define	BB2XTAL_GAINMASK	((0x7)<<5)
#define	BB2XTAL_GAIN(x)		(((x)&0x7)<<5)
#define	BB2XTAL_CCTRLMASK	((0x7F)<<8)
#define	BB2XTAL_CCTRL(x)	(((x)&0x7F)<<8)
#define	BB2XTAL_GET_CCTRL(x)	(((x)>>8)&0x7F)

#define	BB2XTAL_READY		(1<<16)	// Read Only

/******************************************************************************
 *
 *  DA16200 DIPCFG
 *
 ******************************************************************************/

struct		_da16200_dipcfg_
{
	// ----------------------------------
	// Memory Driver Control
	// ----------------------------------
	volatile UINT32	LS_RME_RM;
};

#define DA16200_DIPCFG		((DA16200_DIPCFG_TypeDef *)DA16200_DIPCFG_BASE)

/******************************************************************************
 *
 *  DA16200 DIOCFG
 *
 ******************************************************************************/

struct		_da16200_diocfg_
{
	// ----------------------------------
	// IO MUX & PAD Control
	// ----------------------------------

	volatile UINT32	EXT_INTB_CTRL;		// 0x000
	volatile UINT32	EXT_INTB_SET;		// 0x004
	volatile UINT32	FSEL_GPIO1;		// 0x008
	volatile UINT32	FSEL_GPIO2;		// 0x00C
	volatile UINT32	Test_Control;		// 0x010

	volatile UINT32	Flash_PAD;		// 0x014
	volatile UINT32	UART_PAD;		// 0x018
	volatile UINT32	JTAG_PAD;		// 0x01C
	volatile UINT32	GPIOA_DS;		// 0x020
	volatile UINT32	GPIOA_SR;		// 0x024
	volatile UINT32	GPIOA_PE_PS;		// 0x028
	volatile UINT32	GPIOA_IE_IS;		// 0x02C
	volatile UINT32	EXT_FLASH_PAD;	// 0x030
	volatile UINT32	GPIOB_DS;		// 0x034
	volatile UINT32	GPIOB_SR;		// 0x038
	volatile UINT32	GPIOB_PE_PS;	// 0x03C
	volatile UINT32	GPIOB_IE_IS;	// 0x040
	volatile UINT32	GPIOC_DS;		// 0x044
	volatile UINT32	GPIOC_SR;		// 0x048
	volatile UINT32	GPIOC_PE_PS;	// 0x04C
	volatile UINT32	GPIOC_IE_IS;	// 0x050
};

#define DA16200_DIOCFG		((DA16200_DIOCFG_TypeDef *)DA16200_DIOCFG_BASE)

/******************************************************************************
 *
 *  DA16200 RFMDMC
 *
 ******************************************************************************/

struct		_da16200_rfmdmc_
{
	// ----------------------------------
	// Modem & RF Test Control
	// ----------------------------------
	volatile UINT32	PowerManage;		// 0x000
	volatile UINT32	mon_RF_test;		// 0x004
};

#define DA16200_RFMDMC		((DA16200_RFMDMC_TypeDef *)DA16200_RFMDMC_BASE)

/******************************************************************************
 *
 *  DA16200 AIPCFG
 *
 ******************************************************************************/

struct		_da16200_aipcfg_
{
	//----------------------------------------
	// Analog IP Control
	//----------------------------------------
	volatile UINT32	IP1_band_gap;		// 0x000
	volatile UINT32	IP2_band_gap;		// 0x004
	volatile UINT32	IP4_por;		// 0x008
	volatile UINT32	IP4_1DCDC_DIG;		// 0x00C
	volatile UINT32	IP4_1DCDC_FEM;		// 0x010
	volatile UINT32	LDO_ctrl;		// 0x014
};

#define DA16200_AIPCFG		((DA16200_AIPCFG_TypeDef *)DA16200_AIPCFG_BASE)

/******************************************************************************
 *
 *  DA16200 DAC10C
 *
 ******************************************************************************/

struct		_da16200_dac10c_
{
	//----------------------------------------
	// DAC Control
	//----------------------------------------
	volatile UINT32	DAC_10bit_cfg;
};

#define DA16200_DAC10C		((DA16200_DAC10C_TypeDef *)DA16200_DAC10C_BASE)


/******************************************************************************
 *
 *  DA16200 FDAC14
 *
 ******************************************************************************/

struct		_da16200_fdac14_
{
	//----------------------------------------
	// F.ADC Control
	//----------------------------------------
	volatile UINT32	FADC_14bit_cfg0;	// 0X000
	volatile UINT32	FADC_14bit_cfg1;	// 0X004
	volatile UINT32	FADC_CAL1;		// 0X008
	volatile UINT32	FADC_CAL2;		// 0X00C
	volatile UINT32	cal_done_i;		// 0X010
	volatile UINT32	cal_done_q;		// 0X014

	volatile UINT32	_reserve0[2];

	volatile UINT32	cal_weight_i[10];	// 0X020
	volatile UINT32	cal_weight_q[10];	// 0X048

	volatile UINT32	dout_delta;		// 0X070
	volatile UINT32	dout_error;		// 0X074
};

#define DA16200_FDAC14		((DA16200_FDAC14_TypeDef *)DA16200_FDAC14_BASE)

/******************************************************************************
 *
 *  DA16200 BOOTCON
 *
 ******************************************************************************/

struct		_da16200_bootcon_
{
	volatile UINT32	CHIP_ID;		// 0x00, RO

	volatile UINT8	_reserved0;
	volatile UINT8	BOOT_RESET_INFO;
	volatile UINT8	BOOT_TYPE;
	volatile UINT8	_reserved1;

	volatile UINT8	StackFlash_info;
	volatile UINT8	_reserved2[7];

	volatile UINT32	CPU_RESET_MAN;		// 0x10, RW
	volatile UINT32	CPU_RESET_AUTO;		// 0x14, W
	volatile UINT32	PHY_RESET_MAN;		// 0x18
	volatile UINT32	_reserve3;		// 0x1C
	volatile UINT32	SW_Bin_Info;		// 0x20, RW
	volatile UINT32	IRQ_Test_SW;		// 0x24, W
	volatile UINT32	_reserve4;		// 0x28
	volatile UINT32	_reserve5;		// 0x2C
	volatile UINT8	MP_Remap[12];		// 0x30-0x3B, RW
	volatile UINT32	_reserve6;		// 0x3C
	volatile UINT32	SP_5_Remap_C;		// 0x40, RW
	volatile UINT32 _reserve7[15];		// 0x44-7C
	volatile UINT32 mon_sel_8040;		// 0x80
	volatile UINT32 _reserve8[3];		// 0x84-8C
	volatile UINT32 test_feature_peri[3];	// 0x90-98
	volatile UINT32 test_result_peri;	// 0x9C
	volatile UINT32 test_feature_lmac[3];	// 0xA0-A8
	volatile UINT32 test_result_lmac;	// 0xAC

	volatile UINT32 _reserve9[20];		// 0xB0-FC

	volatile UINT32	ch_en_dma;		// 0x100
	volatile UINT32	sel_uDMA_kDMA;		// 0x104
	volatile UINT32	ch_en_breq;		// 0x108
	volatile UINT32	ch_en_sreq;		// 0x10C
};


#define DA16200_BOOTCON		((DA16200_BOOTCON_TypeDef *)DA16200_BOOTCON_BASE)

// CHIP_ID
#define	SYSGET_CHIPNAME()	(((DA16200_BOOTCON->CHIP_ID)>>8) & 0x00FFFFFF)	// 0x00FC.9050
#define	SYSGET_HWVERSION()	((DA16200_BOOTCON->CHIP_ID)&0x0FF)
#define	SYSGET_CHIP_TYP()	(((DA16200_BOOTCON->CHIP_ID)>>7) & 0x01)		// 0: ASIC, 1: FPGA
#define	SYSGET_CHIP_VER()	(((DA16200_BOOTCON->CHIP_ID)>>4) & 0x07)
#define	SYSGET_CHIP_REV()	(((DA16200_BOOTCON->CHIP_ID)>>0) & 0x0F)

#define	SYSGET_RESET_STATUS()	((DA16200_BOOTCON->BOOT_RESET_INFO) & 0x07)

#define	RESET_POR		(0x0)
#define	RESET_RTCTIMER		(0x1)
#define	RESET_RTCMANUAL		(0x2)
#define	RESET_RTCWDOG		(0x3)
#define	RESET_HWWDOG		(0x4)
#define	RESET_SWWDOG		(0x5)
#define	RESET_SWRESET		(0x6)
#define	RESET_MANUAL		(0x7)

#define	SYSGET_VSIMRUN_TYPE()	((DA16200_BOOTCON->BOOT_TYPE) & 0x01)

#define	BOOT_RX_MODE		(1)
#define	BOOT_TX_MODE		(0)

// SFLASH Info
#define	SYSGET_SFLASH_INFO() 	((DA16200_BOOTCON->StackFlash_info) & (0x03<<4))
#define	SFLASH_NONSTACK		(0x03<<4)
#define	SFLASH_STACK_0		(0x01<<4)
#define	SFLASH_STACK_1		(0x02<<4)
#define	SYSSET_ON_SINFO() 	(DA16200_BOOTCON->StackFlash_info = (0x03<<6))
#define	SYSSET_BKUP_SINFO() 	(DA16200_BOOTCON->StackFlash_info = ((DA16200_BOOTCON->StackFlash_info & 0x03)<<4))

// Reset
#define	SYSSET_M_RESET()	((DA16200_BOOTCON->CPU_RESET_MAN) = 0x01)
#define	SYSSET_A_RESET()	((DA16200_BOOTCON->CPU_RESET_AUTO) = 0x01)

#define	SYSGET_SWVERSION()	(DA16200_BOOTCON->SW_Bin_Info)
#define	SYSSET_SWVERSION(x)	((DA16200_BOOTCON->SW_Bin_Info)=(UINT32)(x))

// IRQ Test
#define SYSSET_SW_IRQ(x)	((DA16200_BOOTCON->SW_Bin_Info)=((x)&0x7F))


/******************************************************************************
 *
 *  DA16200 Sys Clock
 *
 ******************************************************************************/

struct		_da16200_sysclock_
{
	volatile UINT8	PLL_CLK_DIV_0_CPU;	// 0x000
	volatile UINT8	PLL_CLK_DIV_1_XFC;	// 0x001
	volatile UINT8	PLL_CLK_DIV_2_UART;	// 0x002
	volatile UINT8	PLL_CLK_DIV_3_OTP;	// 0x003
	volatile UINT8	PLL_CLK_DIV_4_PHY;	// 0x004
	volatile UINT8	PLL_CLK_DIV_5_I2S;	// 0x005
	volatile UINT8	PLL_CLK_DIV_6_AUXA;	// 0x006
	volatile UINT8	PLL_CLK_DIV_7_CC312;	// 0x007

	volatile UINT8	_reserve0[8];		// 0x008-0x00F

	volatile UINT8	PLL_CLK_EN_0_CPU;	// 0x010
	volatile UINT8	PLL_CLK_EN_1_XFC;	// 0x011
	volatile UINT8	PLL_CLK_EN_2_UART;	// 0x012
	volatile UINT8	PLL_CLK_EN_3_OTP;	// 0x013
	volatile UINT8	PLL_CLK_EN_4_PHY;	// 0x014
	volatile UINT8	PLL_CLK_EN_5_I2S;	// 0x015
	volatile UINT8	PLL_CLK_EN_6_AUXA;	// 0x016
	volatile UINT8	PLL_CLK_EN_7_CC312;	// 0x017

	volatile UINT8	_reserve1[24];		// 0x018-0x02F

	volatile UINT8	SRC_CLK_SEL_0_CPU;	// 0x030
	volatile UINT8	SRC_CLK_SEL_1_XFC;	// 0x031
	volatile UINT8	SRC_CLK_SEL_2_UART;	// 0x032
	volatile UINT8	SRC_CLK_SEL_3_OTP;	// 0x033
	volatile UINT8	_reserve2;		// 0x034
	volatile UINT8	SRC_CLK_SEL_5_I2S;	// 0x035
	volatile UINT8	SRC_CLK_SEL_6_AUXA;	// 0x036
	volatile UINT8	SRC_CLK_SEL_7_CC312;	// 0x037

	volatile UINT8	_reserve3[56];		// 0x038-0x06F

	volatile UINT8	SRC_CLK_STA_0;		// 0x070
	volatile UINT8	_reserve4[3];		// 0x071-0x03
	volatile UINT8	IRQ_CLK_STA_0;		// 0x074
	volatile UINT8	_reserve5[16];		// 0x075-0x084
	volatile UINT8	CLK_DIV_I2S;		// 0x085
	volatile UINT8	_reserve5_1[10];		// 0x086-0x08F

	volatile UINT8	CLK_EN_CPU;		// 0x090
	volatile UINT8	CLK_EN_XFC;		// 0x091
	volatile UINT8	CLK_EN_UART;		// 0x092
	volatile UINT8	CLK_EN_OTP;		// 0x093
	volatile UINT8	_reserve6;		// 0x094
	volatile UINT8	CLK_EN_I2S;		// 0x095
	volatile UINT8	CLK_EN_AUXA;		// 0x096
	volatile UINT8	CLK_EN_CC312;		// 0x097

	volatile UINT8	_reserve7[8];		// 0x098-0x09F

	volatile UINT8	CLK_DIV_PHYBUS;		// 0x0A0
	volatile UINT8	CLK_DIV_EMMC;		// 0x0A1

	volatile UINT8	_reserve8[2];		// 0x0A2-0x0A3

	volatile USHORT	CLK_DIV_AUXA;		// 0x0A4,0xA5

	volatile UINT8	_reserve8_1[10];	// 0x0A6-0x0AF

	volatile UINT8	CLK_EN_PHYBUS;		// 0x0B0
	volatile UINT8	CLK_EN_SDeMMC;		// 0x0B1

	volatile UINT8	_reserve9[30];		// 0x0B2-0x0EF

	volatile UINT8	DIFF_CPU_XFC;		// 0x0F0

	volatile UINT8	_reserve10[15];		// 0x0F1-0x0FF

	volatile UINT32	c_pipe_mst2ints;	// 0x100
	volatile UINT32	c_pipe_ints2mst;	// 0x104
	volatile UINT32	c_pipe_mst2mskr;	// 0x108
	volatile UINT32	c_pipe_mskr2mst;	// 0x10C
	volatile UINT32	c_pipe_mst2retm;	// 0x110
	volatile UINT32	c_pipe_retm2mst;	// 0x114

	volatile UINT32	_reserve11[2];		// 0x118-0x11F

	volatile UINT32	c_pipe_mst2cfgs;	// 0x120
};

#define DA16200_SYSCLOCK		((DA16200_SYSCLOCK_TypeDef *)DA16200_SYSCLOCK_BASE)

#ifdef	BUILD_OPT_DA16200_ASIC
#define	DA16X_CLK_DIV(x)		((x)&0x1f)
#else	//BUILD_OPT_DA16200_ASIC
#define	DA16X_CLK_DIV(x)		((x)&0x00)
#endif	//BUILD_OPT_DA16200_ASIC

#define	CLK_STA_XTAL		0x00
#define	CLK_STA_XTAL2PLL	0x01
#define	CLK_STA_PLL		0x02
#define	CLK_STA_PLL2XTAL	0x03

/******************************************************************************
 *
 *  DA16200 Clock Gate
 *
 ******************************************************************************/

struct		_da16200_sclkgate_
{
	volatile UINT8	Off_CPU_Core;
	volatile UINT8	Off_CPU_Sys;
	volatile UINT8	_reserved2;
	volatile UINT8	_reserved3;
	volatile UINT8	_reserved4;
	volatile UINT8	_reserved5;
	volatile UINT8	_reserved6;
	volatile UINT8	_reserved7;
	volatile UINT8	Off_PMU_Exmp;
	volatile UINT8	Off_PMU_ClkC;
	volatile UINT8	Off_PMU_JTAG;
	volatile UINT8	_reservedB;
	volatile UINT8	Off_MCU_ClkC;
	volatile UINT8	_reservedD;
	volatile UINT8	_reservedE;
	volatile UINT8	_reservedF;
	volatile UINT8	Off_BM_Core;
	volatile UINT8	Off_BM_BTM;
	volatile UINT8	Off_BM_CRC;
	volatile UINT8	Off_BM_TCS;
	volatile UINT8	_reserved14;
	volatile UINT8	_reserved15;
	volatile UINT8	_reserved16;
	volatile UINT8	_reserved17;
	volatile UINT8	_reserved18;
	volatile UINT8	_reserved19;
	volatile UINT8	_reserved1A;
	volatile UINT8	_reserved1B;
	volatile UINT8	_reserved1C;
	volatile UINT8	_reserved1D;
	volatile UINT8	_reserved1E;
	volatile UINT8	_reserved1F;
	volatile UINT8	Off_MC_IntS;
	volatile UINT8	Off_MC_MskR;
	volatile UINT8	Off_MC_RetM;
	volatile UINT8	Off_MC_AOM;
	volatile UINT8	_reserved24;
	volatile UINT8	_reserved25;
	volatile UINT8	_reserved26;
	volatile UINT8	_reserved27;
	volatile UINT8	Off_MC_HSM;
	volatile UINT8	Off_MC_HWZero;
	volatile UINT8	_reserved2A;
	volatile UINT8	_reserved2B;
	volatile UINT8	Off_MC_PROTC;
	volatile UINT8	Off_MC_PROTC_MskR;
	volatile UINT8	Off_MC_PROTC_RetM;
	volatile UINT8	_reserved2F;
	volatile UINT8	Off_MU_IntS;
	volatile UINT8	Off_MU_MskR;
	volatile UINT8	Off_MU_RetM;
	volatile UINT8	Off_MU_AOM;
	volatile UINT8	_reserved34;
	volatile UINT8	_reserved35;
	volatile UINT8	_reserved36;
	volatile UINT8	_reserved37;
	volatile UINT8	_reserved38;
	volatile UINT8	_reserved39;
	volatile UINT8	_reserved3A;
	volatile UINT8	_reserved3B;
	volatile UINT8	_reserved3C;
	volatile UINT8	_reserved3D;
	volatile UINT8	_reserved3E;
	volatile UINT8	_reserved3F;
	volatile UINT8	Off_CC_Cfgs;
	volatile UINT8	Off_CC_Core;
	volatile UINT8	Off_CC_AOM;
	volatile UINT8	Off_CC_HSM;
	volatile UINT8	Off_CC_APBS;
	volatile UINT8	Off_CC_APBC;
	volatile UINT8	Off_CC_OTPB;
	volatile UINT8	Off_CC_M2X1;
	volatile UINT8	Off_CC_OTPW;
	volatile UINT8	Off_CC_AHB_UP;
	volatile UINT8	Off_CC_AHB_DN;
	volatile UINT8	_reserved4B;
	volatile UINT8	_reserved4C;
	volatile UINT8	_reserved4D;
	volatile UINT8	_reserved4E;
	volatile UINT8	_reserved4F;
	volatile UINT8	Off_XIP_AHB_Cache;
	volatile UINT8	Off_XIP_APB_Cache;
	volatile UINT8	Off_XIP_HSM;
	volatile UINT8	Off_XIP_APBS;
	volatile UINT8	Off_XIP_NOR;
	volatile UINT8	_reserved55;
	volatile UINT8	_reserved56;
	volatile UINT8	_reserved57;
	volatile UINT8	Off_XFC_Cfgs;
	volatile UINT8	Off_XFC_Core;
	volatile UINT8	_reserved5A;
	volatile UINT8	_reserved5B;
	volatile UINT8	_reserved5C;
	volatile UINT8	_reserved5D;
	volatile UINT8	_reserved5E;
	volatile UINT8	_reserved5F;
	volatile UINT8	Off_SSI_OI2C;
	volatile UINT8	Off_SSI_MI2C;
	volatile UINT8	Off_SSI_SPI;
	volatile UINT8	Off_SSI_RMF;
	volatile UINT8	Off_SSI_SDIO;
	volatile UINT8	Off_SSI_I2CB;
	volatile UINT8	Off_SSI_SPIB;
	volatile UINT8	Off_SSI_M3X1;
	volatile UINT8	Off_SSI_PROTC;
	volatile UINT8	_reserved69;
	volatile UINT8	_reserved6A;
	volatile UINT8	_reserved6B;
	volatile UINT8	_reserved6C;
	volatile UINT8	_reserved6D;
	volatile UINT8	_reserved6E;
	volatile UINT8	_reserved6F;
	volatile UINT8	Off_CAPB_HSM;
	volatile UINT8	Off_CAPB_APBS;
	volatile UINT8	Off_CAPB_DualT;
	volatile UINT8	Off_CAPB_Timer0;
	volatile UINT8	Off_CAPB_Timer1;
	volatile UINT8	Off_CAPB_WDT;
	volatile UINT8	Off_CAPB_PWM;
	volatile UINT8	Off_CAPB_Test;
	volatile UINT8	Off_CAPB_UART1;
	volatile UINT8	Off_CAPB_UART2;
	volatile UINT8	_reserved7A;
	volatile UINT8	_reserved7B;
	volatile UINT8	_reserved7C;
	volatile UINT8	_reserved7D;
	volatile UINT8	_reserved7E;
	volatile UINT8	_reserved7F;
	volatile UINT8	Off_DAPB_HSM;
	volatile UINT8	Off_DAPB_APBS;
	volatile UINT8	Off_DAPB_AuxA;
	volatile UINT8	Off_DAPB_I2CM;
	volatile UINT8	Off_DAPB_I2S;
	volatile UINT8	Off_DAPB_UART0;
	volatile UINT8	Off_DAPB_GPIO0;
	volatile UINT8	Off_DAPB_CMSDK;
	volatile UINT8	Off_DAPB_GPIO1;
	volatile UINT8	Off_DAPB_GPIO2;
	volatile UINT8	_reserved8A;
	volatile UINT8	_reserved8B;
	volatile UINT8	_reserved8C;
	volatile UINT8	_reserved8D;
	volatile UINT8	_reserved8E;
	volatile UINT8	_reserved8F;
	volatile UINT8	Off_SysC_HSM_CFG;
	volatile UINT8	Off_SysC_Common;
	volatile UINT8	Off_SysC_CPU;
	volatile UINT8	Off_SysC_ClkRst;
	volatile UINT8	Off_SysC_ProtC;
	volatile UINT8	Off_SysC_AM2D;
	volatile UINT8	Off_SysC_FHW;
	volatile UINT8	Off_SysC_Debug;
	volatile UINT8	Off_SysC_Slave;
	volatile UINT8	Off_SysC_fDMA_CFG;
	volatile UINT8	Off_SysC_Gating;
	volatile UINT8	_reserved9B;
	volatile UINT8	_reserved9C;
	volatile UINT8	_reserved9D;
	volatile UINT8	_reserved9E;
	volatile UINT8	_reserved9F;
	volatile UINT8	Off_SysC_uDMA;
	volatile UINT8	Off_SysC_AHB_kDMA;
	volatile UINT8	Off_SysC_APB_kDMA;
	volatile UINT8	Off_SysC_fDMA_Core;
	volatile UINT8	Off_SysC_RWHSU;
	volatile UINT8	Off_SysC_PRNG;
	volatile UINT8	Off_SysC_TA2Sync;
	volatile UINT8	Off_SysC_RTCInt;
	volatile UINT8	Off_SysC_TKIP;
	volatile UINT8	Off_SysC_HSM;
	volatile UINT8	Off_SysC_TDES;
	volatile UINT8	Off_SysC_Strace;
	volatile UINT8	Off_SysC_HIF;
	volatile UINT8	Off_SysC_M3X1;
	volatile UINT8	_reservedAE;
	volatile UINT8	_reservedAF;
	volatile UINT8	Off_SysB_PSKSHA1;
	volatile UINT8	Off_SysB_HSMS7;
	volatile UINT8	Off_SysB_UARTBR;
	volatile UINT8	Off_SysB_DAC;
	volatile UINT8	_reservedB4;
	volatile UINT8	_reservedB5;
	volatile UINT8	_reservedB6;
	volatile UINT8	_reservedB7;
	volatile UINT8	_reservedB8;
	volatile UINT8	_reservedB9;
	volatile UINT8	_reservedBA;
	volatile UINT8	_reservedBB;
	volatile UINT8	_reservedBC;
	volatile UINT8	_reservedBD;
	volatile UINT8	_reservedBE;
	volatile UINT8	_reservedBF;
	volatile UINT8	Off_RF_Int_Wrap;
};

#define DA16X_CLOCK_SCGATE	((DA16200_SCLKGATE_TypeDef *)(DA16200_SCLKGATE_BASE|0x000))
#define DA16X_CLOCK_DCGATE	((DA16200_SCLKGATE_TypeDef *)(DA16200_SCLKGATE_BASE|0x100))

#define	DA16X_CLKGATE_ON(x)
#define	DA16X_CLKGATE_OFF(x)

#define	DA16X_SCGATE_ON(x)	{ DA16X_CLOCK_SCGATE->x = 0x01; }
#define	DA16X_SCGATE_OFF(x)	{ DA16X_CLOCK_SCGATE->x = 0x00; }
#define	FC9k_SCGATE_CHECK(x)	(DA16X_CLOCK_SCGATE->x)
#define	DA16X_DCGATE_ON(x)	{ DA16X_CLOCK_DCGATE->x = 0x01; }
#define	DA16X_DCGATE_OFF(x)	{ DA16X_CLOCK_DCGATE->x = 0x00; }
#define	FC9k_DCGATE_CHECK(x)	(DA16X_CLOCK_DCGATE->x)

/******************************************************************************
 *
 *  DA16200 SYSDBGT Lite
 *
 ******************************************************************************/

struct		_da16200_litedbgt_
{
	volatile	UINT32	DBGT_CHAR_DATA;		// 0x0010
	volatile	UINT32	DBGT_TRIG_DATA;
	volatile	UINT32	DBGT_TASK_DATA;
};

#define DA16X_SYSDBGT_DATA	((DA16200_LITESYSDBGT_TypeDef *)DA16200_SYSDBGTLITE_BASE)

// SLR Version
#define	FPGA_DBG_TIGGER(x) 	DA16X_SYSDBGT_DATA->DBGT_TRIG_DATA = (x)
#define	ASIC_DBG_TRIGGER(x) 	DA16X_SYSDBGT_DATA->DBGT_TRIG_DATA = (x)
#define	TASK_DBG_TRACE(x) 	DA16X_SYSDBGT_DATA->DBGT_TASK_DATA = (x)

#define	MODE_DBG_RUN		0x00000000
#define	MODE_DBG_FIN		0x00000001
#define	MODE_DBG_LREAD		0x00000002
#define	MODE_DBG_LWRITE		0x00000004
#define	MODE_HARD_FAULT		0xF0000000
#define	MODE_INI_STEP(x)	(0x10000000|((x<<4)&0x0FFFFFF0))
#define	MODE_OAL_STEP(x)	(0x20000000|((x<<4)&0x0FFFFFF0))
#define	MODE_HAL_STEP(x)	(0x30000000|((x<<4)&0x0FFFFFF0))
#define	MODE_SAL_STEP(x)	(0x40000000|((x<<4)&0x0FFFFFF0))
#define	MODE_DAL_STEP(x)	(0x50000000|((x<<4)&0x0FFFFFF0))
#define	MODE_NAL_STEP(x)	(0x60000000|((x<<4)&0x0FFFFFF0))
#define	MODE_LMAC_STEP(x)	(0x80000000|((x<<4)&0x0FFFFFF0))
#define	MODE_UMAC_STEP(x)	(0x90000000|((x<<4)&0x0FFFFFF0))
#define	MODE_APP_STEP(x)	(0xA0000000|((x<<4)&0x0FFFFFF0))
#define	MODE_CRY_STEP(x)	(0xC0000000|((x<<4)&0x0FFFFFF0))
#define	MODE_DPM_STEP(x)	(0xD0000000|((x<<4)&0x0FFFFFF0))

/******************************************************************************
 *
 *  DA16200 PSK_SHA1
 *
 ******************************************************************************/

struct		_da16200_psk_sha1_
{
	volatile	UINT32	Enable;				/* 00 */

	union {
		volatile	UINT32	IHV_1_0;		/* 04 */
		volatile	UINT32	cal_weight_i_0;
	};
	union {
		volatile	UINT32	IHV_1_1;		/* 08 */
		volatile	UINT32	cal_weight_i_1;
	};
	union {
		volatile	UINT32	IHV_1_2;		/* 0C */
		volatile	UINT32	cal_weight_i_2;
	};
	union {
		volatile	UINT32	IHV_1_3;		/* 10 */
		volatile	UINT32	cal_weight_i_3;
	};
	union {
		volatile	UINT32	IHV_1_4;		/* 14 */
		volatile	UINT32	cal_weight_i_4;
	};
	union {
		volatile	UINT32	IHV_2_0;		/* 18 */
		volatile	UINT32	cal_weight_i_5;
	};
	union {
		volatile	UINT32	IHV_2_1;		/* 1C */
		volatile	UINT32	cal_weight_i_6;
	};
	union {
		volatile	UINT32	IHV_2_2;		/* 20 */
		volatile	UINT32	cal_weight_i_7;
	};
	union {
		volatile	UINT32	IHV_2_3;		/* 24 */
		volatile	UINT32	cal_weight_i_8;
	};
	union {
		volatile	UINT32	IHV_2_4;		/* 28 */
		volatile	UINT32	cal_weight_i_9;
	};
	union {
		volatile	UINT32	Data_0;			/* 2C */
		volatile	UINT32	cal_weight_q_0;
	};
	union {
		volatile	UINT32	Data_1;			/* 30 */
		volatile	UINT32	cal_weight_q_1;
	};
	union {
		volatile	UINT32	Data_2;			/* 34 */
		volatile	UINT32	cal_weight_q_2;
	};
	union {
		volatile	UINT32	Data_3;			/* 38 */
		volatile	UINT32	cal_weight_q_3;
	};
	union {
		volatile	UINT32	Data_4;			/* 3C */
		volatile	UINT32	cal_weight_q_4;
	};
	union {
		volatile	UINT32	Digest_0;		/* 40 */
		volatile	UINT32	cal_weight_q_5;
	};
	union {
		volatile	UINT32	Digest_1;		/* 44 */
		volatile	UINT32	cal_weight_q_6;
	};
	union {
		volatile	UINT32	Digest_2;		/* 48 */
		volatile	UINT32	cal_weight_q_7;
	};
	union {
		volatile	UINT32	Digest_3;		/* 4C */
		volatile	UINT32	cal_weight_q_8;
	};
	union {
		volatile	UINT32	Digest_4;		/* 50 */
		volatile	UINT32	cal_weight_q_9;
	};
};

#define DA16X_PSK_SHA1	((DA16200_PSK_SHA1_TypeDef *)DA16200_PSK_SHA1_BASE)

/******************************************************************************
 *
 *  DA16200 Slave Controller
 *
 ******************************************************************************/

struct	_da16200_slavectrl_
{
	volatile	UINT32	RMF_REQ_CLR;	//0x50080000
	volatile	UINT32	RMF_EN;	//0x50080004
	volatile	UINT32	RMF_RD_GARBAGE;	//0x50080008
	volatile	UINT32	RMF_IRQ_EN;	//0x5008000c
	volatile	UINT32	RMF_CMD_ADDR;	//0x50080010
	volatile	UINT32	RMF_CMD_LENGTH;	//0x50080014
	volatile	UINT32	RMF_OP_STATUS;	//0x50080018
	volatile	UINT32	RMF_IRQ_STATUS;	//0x5008001c
	volatile	UINT32	RMF_RDB_WA;	//0x50080020
	volatile	UINT32	RMF_RDB_RA;	//0x50080024
	volatile	UINT32	reserved0;	//0x50080028
	volatile	UINT32	reserved1;	//0x5008002c
	volatile	UINT32	I2C_Slave_Sel;	//0x50080030
	volatile	UINT32	I2C_GR_level;	//0x50080034
	volatile	UINT32	Martin_I2C_STATUS;	//0x50080038
	volatile	UINT32	RS485_Mode_Sel;	//0x5008003c
	volatile	UINT32	UART2_SELECT;	//0x50080040

	volatile	UINT32	reserved2[47]; //0x50080044 ~ 0x500800fc

	volatile	UINT32	sel_debug;	//0x50080100
	volatile	UINT32	sel_trigger;	//0x50080104

	volatile	UINT32	reserved3[77]; //0x50080108 ~ 0x50080238

	volatile	UINT32	SPI_Intr_Status;	//0x5008023c
	volatile	UINT32	SPI_Slave_CR;	//0x50080240
	volatile	UINT32	I2C_Slave_CR;	//0x50080244
	volatile	UINT32	Length;	//0x50080248
	volatile	UINT32	I2C_Buffer_ADDR;	//0x5008024c
	volatile	UINT32	Base_Addr;	//0x50080250
	volatile	UINT32	Cmd_Addr;	//0x50080254
	volatile	UINT32	Resp_Addr1;	//0x50080258
	volatile	UINT32	Resp_Addr2;	//0x5008025c
	volatile	UINT32	AT_CMD_Base;	//0x50080260
	volatile	UINT32	AT_CMD_Ref;	//0x50080264
	volatile	UINT32	SPI_Timer;	//0x50080268

	volatile	UINT32	reserved4; //0x5008026c

	volatile	UINT32	RW_Dummy[4]; //0x50080270 ~ 0x5008027c

	volatile	UINT32	reserved5[16]; //0x50080280 ~ 0x500802bc

	volatile	UINT32	c_clk_gate;	//0x500802c0
};

#define DA16X_SLAVECTRL	((DA16200_SLAVECTRL_TypeDef *)DA16200_SLAVE_BASE)

/******************************************************************************
 *
 *  DA16200 FPGACON
 *
 ******************************************************************************/

#ifdef	BUILD_OPT_DA16200_FPGA

struct		_da16200fpga_syscon_		// 0x7F00_0000
{
	volatile UINT32	VERSION;		// RO, 0x0000
	volatile UINT32	FPGA_PLL_FREQ;		// RW, 0x0004
	volatile UINT32 _reserved0;
	volatile UINT32	_reserved1;

	volatile UINT32	zbt_tx_clk_ctrl;	// RW, 0x0010
	volatile UINT32	zbt_rx_clk_ctrl;	// RW, 0x0014
	volatile UINT32	zbt_rx_lat_ctrl;	// RW, 0x0018
	volatile UINT32 _reserved2;

	volatile UINT32	sdr_tx_clk_ctrl;	// RW, 0x0020
	volatile UINT32	sdr_rx_clk_ctrl;	// RW, 0x0024
	volatile UINT32 _reserved3[2];

	volatile UINT32 emi_nor_type;		// WO, 0x0030
	volatile UINT32 emi_tp;			// RW, 0x0034
	volatile UINT32 emi_tp_wr;		// RW, 0x0038
	volatile UINT32 emi_tp_rd;		// RW, 0x003C

	volatile UINT32	eMMC_tx_clk_ctrl;	// RW, 0x0040
	volatile UINT32 _reserved5[15];

	volatile UINT32 define_rm_fpga[4];	// RO, 0x0080
	volatile UINT32 define_en_fpga[4];	// RO, 0x0090
	volatile UINT32 define_en_dcg[4];	// RO, 0x00A0
	volatile UINT32 _reserved6[12];

	volatile UINT32 CPU2SRAM_PATH;		// RW, 0x00F0
};


#define DA16200FPGA_SYSCON	((DA16200FPGA_SYSCON_TypeDef *)DA16200_FPGASYSCON_BASE)

#define	DA16X_NORTYPE_XIP		3
#define	DA16X_NORTYPE_CACHE		1
#define	DA16X_SFLASHTYPE_XIP		5	//da16200removed: 7
#define	DA16X_SFLASHTYPE_CACHE		5
#define	DA16XFPGA_GET_NORTYPE()		(DA16200FPGA_SYSCON->emi_nor_type)
#define	DA16XFPGA_SET_NORTYPE(x)		(DA16200FPGA_SYSCON->emi_nor_type = x)

#define	SYSGET_FPGAVERSION()		((DA16200FPGA_SYSCON->VERSION)

#else	//BUILD_OPT_DA16200_FPGA

#define DA16200FPGA_SYSCON

#define	DA16X_NORTYPE_XIP		3
#define	DA16X_NORTYPE_CACHE		1
#define	DA16X_SFLASHTYPE_XIP		5	//da16200removed: 7
#define	DA16X_SFLASHTYPE_CACHE		5
#define	DA16XFPGA_GET_NORTYPE()		(5)
#define	DA16XFPGA_SET_NORTYPE(x)

#define	SYSGET_FPGAVERSION()		(0)

#endif	//BUILD_OPT_DA16200_FPGA

#endif	/*__da16200_regs_h__*/

/* EOF */
