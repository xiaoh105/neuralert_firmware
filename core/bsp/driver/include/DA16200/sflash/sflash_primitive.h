/**
 ****************************************************************************************
 *
 * @file sflash_primitive.h
 *
 * @brief SFLASH Driver
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


#ifndef __SFLASH_PRIMITIVE_H__
#define __SFLASH_PRIMITIVE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver.h"
#include "sflash.h"


#define	SFLASH_SLEEP_PRESET
#undef	DEBUG_SFLASH_ACCESS
#define	SUPPORT_FLASH_CACHE	// No PowerDown


#define	SFLASH_MAX_SHFT			(10)
#define	SFLASH_MAX_LOOP			(4<<SFLASH_MAX_SHFT)
#define	SFLASH_MAX_LOOPSPEED		(1<<SFLASH_MAX_SHFT)

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef		struct	_sflash_work_table	SFLASH_WORK_TYPE;

//---------------------------------------------------------
//	DRIVER :: Definition
//---------------------------------------------------------

//---------------------------------------------------------
//	Command Set
//---------------------------------------------------------

typedef		enum {
	SFLASH_CMD_REMS = 0,			/* 90 */
	SFLASH_CMD_RDID,			/* 9F */
	SFLASH_CMD_RES,				/* AB */
	SFLASH_CMD_RSFDP,			/* 5A or 9F */
	SFLASH_CMD_RDSR1,			/* 05 */
	SFLASH_CMD_WRR,				/* 01 */
	SFLASH_CMD_WRDI,			/* 04 */
	SFLASH_CMD_WREN,			/* 06 */
	SFLASH_CMD_WRENA,			/* 50 */
	SFLASH_CMD_RSTEN,			/* 66 */
	SFLASH_CMD_RESET,			/* F0 or 99 */
	SFLASH_CMD_MBR,				/* FF */
	SFLASH_CMD_MPM,				/* A3 */
	SFLASH_CMD_DPD,				/* B9 */
	SFLASH_CMD_RDPD,			/* AB */
	SFLASH_CMD_EQIO,			/* 35 or 38 */
	SFLASH_CMD_RSTQIO,			/* F5 or FF */
	SFLASH_CMD_EN4B,			/* B7 */
	SFLASH_CMD_EX4B,			/* E9 */
	SFLASH_CMD_WPSEL,			/* 68 */
	SFLASH_CMD_CLSR,			/* 30 */
	SFLASH_CMD_PP,				/* 02 */
	SFLASH_CMD_QPP,				/* 32 */
	SFLASH_CMD_QPPA,			/* 38 */
	SFLASH_CMD_4PP,				/* 12 */
	SFLASH_CMD_4QPP,			/* 34 */
	SFLASH_CMD_4QPPA,			/* 38 */
	SFLASH_CMD_P4E,				/* 20 */
	SFLASH_CMD_4P4E,			/* 21 */
	SFLASH_CMD_FSRD,			/* 0B */
	SFLASH_CMD_CE,				/* 60 */
	SFLASH_CMD_RDSR2,			/* 07 */
	SFLASH_CMD_RDCR,			/* 35 */
	SFLASH_CMD_BRRD,			/* 16 */
	SFLASH_CMD_BRWR,			/* 17 */
	SFLASH_CMD_DLPRD,			/* 41 */
	SFLASH_CMD_PNVDLR,			/* 43 */
	SFLASH_CMD_WVDLR,			/* 4A */
	SFLASH_CMD_OTPP,			/* 42 */
	SFLASH_CMD_OTPR,			/* 4B */
	SFLASH_CMD_ASPRD,			/* 2B */
	SFLASH_CMD_ASPP,			/* 2F */
	SFLASH_CMD_PLBRD,			/* A7 */
	SFLASH_CMD_PLBWR,			/* A6 */
	SFLASH_CMD_MAX
}	SFLASH_CMD_ENUM_TYPE;

//---------------------------------------------------------
//	Suspend/Resume Command
//---------------------------------------------------------

typedef	enum {
	SUS_GET_PROGRAM = 0,
	SUS_GET_ERASE,
	SUS_RUN_PROGRAM,
	SUS_RUN_ERASE,
	RES_RUN_PROGRAM,
	RES_RUN_ERASE
} SFLASH_SUSCMD_TYPE;

//---------------------------------------------------------
//	Vendor Specific Command
//---------------------------------------------------------

typedef enum	{
	SFLASH_DEV_SET_PPBL,
	SFLASH_DEV_GET_PPBL,

	SFLASH_DEV_VENDOR,

	SFLASH_DEV_MAX
} SFLASH_VENDOR_CMD_LIST;

//---------------------------------------------------------
//	Access Mode
//---------------------------------------------------------
// CON[   31]: concatenation
// DTR[   30]: DTR
// AX [29:28]: AXh Mode
// CMD[   27]: Cmd Bytes
// ADR[   26]: Addr Bytes
// RD [   25]: Read Data
// WR [   24]: Write Data
// MOD[23:21]: Mode Bits
// DMY[20:16]: Dummy Cycles
// CMD[15:12]: (on,dtr,pow) pow=2^N
// ADD[11: 8]: (on,dtr,pow) pow=2^N
// DMY[ 7: 4]: (on,dtr,bus) pow=2^N
// DAT[ 3: 0]: (on,dtr,bus) pow=2^N

#define	DARWMDCADD(con,ax,cmd,adr,rd,wr,mod,dmy,pcmd,padd,pdmy,pdat)	\
		( (((con)&0x01)<<31)					\
		| (((ax)&0x03)<<28)					\
		| ((((cmd)-1)&0x01)<<27)|((((adr)-3)&0x01)<<26)		\
		| (((rd)&0x01)<<25)|(((wr)&0x01)<<24)			\
		| (((mod)&0x07)<<21)|(((dmy)&0x1f)<<16)			\
		| (((pcmd)&0x0f)<<12)|(((padd)&0x0f)<<8)		\
		| (((pdmy)&0x0f)<<4)|(((pdat)&0x0f)<<0)	)

#define	BUS_CADD(pcmd,padd,pdmy,pdat)					\
		( (((pcmd)&0x0f)<<12)|(((padd)&0x0f)<<8)		\
		| (((pdmy)&0x0f)<<4)|(((pdat)&0x0f)<<0)	)

#define SPI_READ		1
#define SPI_WRITE		1
#define SPI_NONE		0
#define SPIPHASE(on,dtr,pow)	((((on)&0x01)<<3)|(((dtr)&0x01)<<2)|((pow)&0x03))

#define DARWMDCADD_ADR_UPDATE(phs,adr)	(((phs)&(~(1<<26)))|((((adr)-3)&0x01)<<26))

#define DARWMDCADD_BUS_UPDATE(phs,bus)	(((phs)&(~(0x0FFFF)))|((bus)&(0x0FFFF)))

#define	BUS_CADD_ON_UPDATE(phs, mask, cmdon,addon,dmyon,daton)		\
		( ((phs) & (~(0x08888&((mask)<<3))))			\
		|(((cmdon)&0x01)<<15)|(((addon)&0x01)<<11)		\
		|(((dmyon)&0x01)<<7)|(((daton)&0x01)<<3)	)

#define DARWMDCADD_BUS(phs)		((phs)&0x0FFFF)
#define DARWMDCADD_NOBUS(phs)		((phs)&(~0x0FFFF))

#define DARWMDCADD_CONCAT		(1<<31)
#define DARWMDCADD_DTR			(1<<30)
#define DARWMDCADD_AXH			(0x03<<28)
#define DARWMDCADD_CMD			(0x01<<27)
#define DARWMDCADD_ADR			(0x01<<26)
#define DARWMDCADD_MODEBIT(x)		(((x)&0x03)<<28)

//---------------------------------------------------------
//	ID-CFI Address Map
//---------------------------------------------------------

#define	ID_CFI_SHORT_LEN	3
#define ID_CFI_MAX_LENGTH	512
#define	ID_CFI_MAX_SECGRP_NUM	5

#define	ID_CFI_MANID		0x00
#define	ID_CFI_DEVID		0x01
#define	ID_CFI_DENSITY		0x02

//---------------------------------------------------------
//	Status Register 1
//---------------------------------------------------------

#define	SR1_SRWD		(1<<7)
#define	SR1_PERR		(1<<6)
#define	SR1_QEN			(1<<6)
#define	SR1_EERR		(1<<5)
#define	SR1_BP2			(1<<4)
#define	SR1_BP1			(1<<3)
#define	SR1_BP0			(1<<2)
#define	SR1_WEL			(1<<1)
#define	SR1_WIP			(1<<0)

//---------------------------------------------------------
//	DYB, PPB Register
//---------------------------------------------------------

#define	DYB_LOCK		0xFF
#define DYB_UNLOCK		0x00

#define	PPB_LOCK		0xFF
#define PPB_UNLOCK		0x00

//---------------------------------------------------------
//	DYB, PPB Register
//---------------------------------------------------------

#define	SFLASH_UTIME(unit,count)	((unit<<16)|((count)&0x0ffff))
#define	SFLASH_CONV_UTIME(unit)		(((unit)&0x0ffff) * (1<<(3*((unit>>16)&0x0ffff))))
#define	SFLASH_UTIME_UNIT(sleeptime)	(((sleeptime)>>16)&0x0ffff)
#define	SFLASH_UTIME_COUNT(sleeptime)	(((sleeptime)>>0)&0x0ffff)
#define	SFLASH_UITME_MSEC(x)		((3<<16)|(((x)<<1)&0x0001f))
#define	SFLASH_UITME_USEC(x)		((0<<16)|(((x)<<1)&0x0001f))
#define	SFLASH_UITME_100US(x)		((2<<16)|(((x)<<1)&0x0001f))

//---------------------------------------------------------
//	DRIVER :: SFLASH Debug
//---------------------------------------------------------
#ifdef	DEBUG_SFLASH_ACCESS
#define	SFLASH_DBG_NONE(...)	//DRV_DBG_NONE( __VA_ARGS__ )
#define	SFLASH_DBG_BASE(...)	//DRV_DBG_BASE( __VA_ARGS__ )
#define	SFLASH_DBG_INFO(...)	DRV_DBG_INFO( __VA_ARGS__ )
#define	SFLASH_DBG_WARN(...)	DRV_DBG_WARN( __VA_ARGS__ )
#define	SFLASH_DBG_ERROR(...)	DRV_DBG_ERROR( __VA_ARGS__ )
#define	SFLASH_DBG_DUMP(...)	DRV_DBG_DUMP(6, __VA_ARGS__ )
#else	//DEBUG_SFLASH_ACCESS
#define	SFLASH_DBG_NONE(...)
#define	SFLASH_DBG_BASE(...)
#define	SFLASH_DBG_INFO(...)
#define	SFLASH_DBG_WARN(...)
#define	SFLASH_DBG_ERROR(...)
#define	SFLASH_DBG_DUMP(...)
#endif 	//DEBUG_SFLASH_ACCESS
#define	SFLASH_DBG_BUS(...)	//DRV_DBG_INFO( __VA_ARGS__ )
//---------------------------------------------------------
//	DRIVER :: SFLASH Info
//---------------------------------------------------------

struct		_sflash_work_table
{
	SFLASH_CMD_TYPE  	cmditem;
	UINT32			num_secgrp;
	SFLASH_SECTOR_LIST	sec_grp[ID_CFI_MAX_SECGRP_NUM];
	UINT8 			id_cfi[ID_CFI_MAX_LENGTH];
};

#if	defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)

//---------------------------------------------------------
//	DRIVER :: Macro
//---------------------------------------------------------

//---------------------------------------------------------
//	DRIVER :: Init Functions
//---------------------------------------------------------

extern void	sflash_local_primitive_init(SFLASH_HANDLER_TYPE *sflash);

#ifdef	SFLASH_SLEEP_PRESET
#else	//SFLASH_SLEEP_PRESET
extern  void sflash_flash_sleep(UINT32 usec);
#endif	//SFLASH_SLEEP_PRESET

#endif /*defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)*/


#endif  /*__SFLASH_PRIMITIVE_H__*/
