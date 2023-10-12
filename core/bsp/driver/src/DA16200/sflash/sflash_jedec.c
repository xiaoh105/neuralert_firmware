/**
 ****************************************************************************************
 *
 * @file sflash_jedec.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oal.h"
#include "hal.h"
#include "driver.h"

#include "sflash.h"
#include "sflash/sflash_bus.h"
#include "sflash/sflash_device.h"
#include "sflash/sflash_jedec.h"
#include "sflash/sflash_primitive.h"

#include "da16x_compile_opt.h"

#ifdef BUILD_OPT_VSIM
#define	SUPPORT_JERRY_SIM_MODEL
#define	JERRY_SFLASH_IS25LQ032B
#undef	JERRY_SFLASH_MX25L25635F
#undef	SUPPORT_SFLASH_INFO_PRINT
#else	//BUILD_OPT_VSIM
#undef	SUPPORT_JERRY_SIM_MODEL
#undef	JERRY_SFLASH_IS25LQ032B
#undef	JERRY_SFLASH_MX25L25635F
#undef	SUPPORT_SFLASH_INFO_PRINT
#endif	//BUILD_OPT_VSIM

#define	SUPPORT_QEBIT_ISSUE

#define	SUPPORT_SLR_MANAGEMENT
#define	CODE_SEC_SFLASH
#define	RO_SEC_SFLASH

#define	SFLASH_JEDEC_PRINT(...)		DRV_PRINTF( __VA_ARGS__ )

#if	defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)

#define	SFLASH_CMD_RDID_LEN		3
#define SFLASH_CMD_REMS_LEN             2

/******************************************************************************

 ******************************************************************************/

#ifdef	SFLASH_SLEEP_PRESET
#define	JEDEC_PRESET_SLEEP(sflash, unit,count)	{			\
	sflash->slptim = SFLASH_UTIME(unit,(count-1));			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, SFLASH_BUS_PRESET_USLEEP, &(sflash->slptim) );\
}

#define	JEDEC_PRESET_SLEEPTIME(sflash, sleeptime){			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, SFLASH_BUS_PRESET_USLEEP, &sleeptime );\
}

#define	JEDEC_SET_SLEEP(sflash, unit,count)
#define	JEDEC_SET_SLEEPTIME(sflash, sleeptime)

#define	JEDEC_GET_SLEEPTIME(sflash)		(sflash->slptim)

#else	//SFLASH_SLEEP_PRESET
#define	JEDEC_PRESET_SLEEP(sflash, unit,count)
#define	JEDEC_PRESET_SLEEPTIME(sflash, sleeptime)

#define	JEDEC_SET_SLEEP(sflash, unit,count)	{			\
	sflash->slptim = SFLASH_UTIME(unit,(count-1));			\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, SFLASH_BUS_SET_USLEEP, &(sflash->slptim) );\
}

#define	JEDEC_SET_SLEEPTIME(sflash, sleeptime)	{				\
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, SFLASH_BUS_SET_USLEEP, &sleeptime );\
}

#define	JEDEC_GET_SLEEPTIME(sflash)		(sflash->slptim)

#endif	//SFLASH_SLEEP_PRESET

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFDP table of JEDEC
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_RODATA
const SFLASH_CMD_TYPE	sflash_jedec_cmdlist[]
= {
/* SFLASH_CMD_REMS */	 	0x90 ,
/* SFLASH_CMD_RDID */	 	0x9F ,
/* SFLASH_CMD_RES */		0xAB ,
/* SFLASH_CMD_RSFDP */	 	0x5A ,
/* SFLASH_CMD_RDSR1 */	 	0x05 ,
/* SFLASH_CMD_WRR */	 	0x01 ,
/* SFLASH_CMD_WRDI */	 	0x04 ,
/* SFLASH_CMD_WREN */	 	0x06 ,
/* SFLASH_CMD_WRENA */	 	0x50 ,

/* SFLASH_CMD_RSTEN */	 	0x66 ,
/* SFLASH_CMD_RESET */	 	0x99 ,
/* SFLASH_CMD_MBR */	 	0x00 ,
/* SFLASH_CMD_MPM */	 	0xA3 ,
/* SFLASH_CMD_DPD */	 	0xB9 ,
/* SFLASH_CMD_RDPD */	 	0xAB ,
/* SFLASH_CMD_EQIO */	 	0x35 ,
/* SFLASH_CMD_RSTQIO */	 	0xF5 ,
/* SFLASH_CMD_EN4B */	 	0xB7 ,
/* SFLASH_CMD_EX4B */	 	0xE9 ,
/* SFLASH_CMD_WPSEL */	 	0x68 ,
/* SFLASH_CMD_CLSR */	 	0x30 ,

/* SFLASH_CMD_PP */			0x02 ,
/* SFLASH_CMD_QPP */		0x00 ,
/* SFLASH_CMD_QPPA */		0x00 ,
/* SFLASH_CMD_4PP */		0x12 ,
/* SFLASH_CMD_4QPP */		0x00 ,
/* SFLASH_CMD_4QPPA */	 	0x00 ,
/* SFLASH_CMD_P4E */		0x20 ,
/* SFLASH_CMD_4P4E */		0x21 ,

/* SFLASH_CMD_FSRD */	 	0x00 ,
/* SFLASH_CMD_CE */	 		0x60 ,

/* SFLASH_CMD_RDSR2 */	 	0x00 ,
/* SFLASH_CMD_RDCR */	 	0x15 ,
/* SFLASH_CMD_BRRD */	 	0xC8 ,
/* SFLASH_CMD_BRWR */	 	0xC5 ,
/* SFLASH_CMD_DLPRD */	 	0x00 ,
/* SFLASH_CMD_PNVDLR */	 	0x00 ,
/* SFLASH_CMD_WVDLR */	 	0x00 ,

/* SFLASH_CMD_OTPP */	 	0x00 ,
/* SFLASH_CMD_OTPR */	 	0x00 ,

/* SFLASH_CMD_ASPRD */	 	0x2B ,
/* SFLASH_CMD_ASPP */	 	0x2F ,
/* SFLASH_CMD_PLBRD */	 	0xA7 ,
/* SFLASH_CMD_PLBWR */	 	0xA6

};

#ifdef	SUPPORT_SFLASH_INFO_PRINT
ATTRIBUTE_RAM_RODATA
const char	*sflash_jedec_cmdlist_str[]
= {
"REMS",
"RDID",
"RES",
"RSFDP",
"RDSR1",
"WRR",
"WRDI",
"WREN",
"WRENA",

"RSTEN",
"RESET",
"MBR",
"MPM",
"DPD",
"RDPD",
"EQIO",
"RSTQIO",
"EN4B",
"EX4B",
"WPSEL",
"CLSR",

"PP",
"QPP",
"QPPA",
"4PP",
"4QPP",
"4QPPA",
"P4E",
"4P4E",

"FSRD",
"CE",

"RDSR2",
"RDCR",
"BRRD",
"BRWR",
"DLPRD",
"PNVDLR",
"WVDLR",

"OTPP",
"OTPR",

"ASPRD",
"ASPP",
"PLBRD",
"PLBWR",

NULL ,
};
#endif	//SUPPORT_SFLASH_INFO_PRINT

#define	SFLASH_CMD_JEDEC_MAX	(SFLASH_CMD_PLBWR+1)

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
SFLASH_CMD_TYPE *sflash_jedec_get_cmd(UINT32 cmd
				, SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
	SFLASH_WORK_TYPE *work;

	if( cmd > SFLASH_CMD_JEDEC_MAX ){
		cmd = SFLASH_CMD_JEDEC_MAX ; // not supported
	}

	work = (SFLASH_WORK_TYPE *)(sflash->work);

	if( sflash->jedec.cmdlist != NULL /* && sflash->work != NULL */ ){
		work->cmditem = ((SFLASH_CMDLIST_TYPE *)(sflash->jedec.cmdlist))[cmd];
	}
	else {
		work->cmditem = sflash_jedec_cmdlist[cmd];
	}

	return &(work->cmditem);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
UINT32 sflash_jedec_bus_update(SFLASH_HANDLER_TYPE *sflash
				, UINT32 force) CODE_SEC_SFLASH
{
	SFLASH_SFDP_TYPE *sfdp;
	UINT32		 phase;
	UINT32		 adrbyte;

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	switch((sflash->accmode & SFLASH_BUS_MASK)){
	case SFLASH_BUS_GRP(SFLASH_BUS_444):
		phase = BUS_CADD(SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_044):
		phase = BUS_CADD(SPIPHASE(0,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_004):
		phase = BUS_CADD(SPIPHASE(0,0,2),SPIPHASE(0,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_144):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,2),SPIPHASE(1,0,2),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_114):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_014):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,2));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_222):
		phase = BUS_CADD(SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_022):
		phase = BUS_CADD(SPIPHASE(0,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_002):
		phase = BUS_CADD(SPIPHASE(0,0,1),SPIPHASE(0,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_122):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,1),SPIPHASE(1,0,1),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_112):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_012):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,1));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;

	case SFLASH_BUS_GRP(SFLASH_BUS_111):
		phase = BUS_CADD(SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_011):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	case SFLASH_BUS_GRP(SFLASH_BUS_001):
		phase = BUS_CADD(SPIPHASE(0,0,0),SPIPHASE(0,0,0),SPIPHASE(1,0,0),SPIPHASE(1,0,0));
		phase = DARWMDCADD_BUS_UPDATE(force, phase);	//@suppress("Suggested parenthesis around expression")
		break;
	default:
		phase = force;
		break;
	}

	if( sfdp != NULL ){
		switch(sfdp->addbyt){
			case	2: // 4 Byte dddress only
				adrbyte = 4;
				break;
			case	1: // 3 Byte or 4 Byte
				if( (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR){
					adrbyte = 4;
				}else{
					adrbyte = 3;
				}
				break;
			default: // 3 Byte only or others
				adrbyte = 3;
				break;
		}
	}else{
		adrbyte = 3;
	}

	phase = DARWMDCADD_ADR_UPDATE(phase,adrbyte);

	SFLASH_DBG_BASE("%s: adr %d phas %x forc %x"
				, __func__, adrbyte, phase, force );

	return phase;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_read(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 *data, UINT32 len) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		return bus->transmit( sflash, (UINT32)cmd, 0, 0
					, NULL, 0, data, len );
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_atomic(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force)
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 0); // CMD-only	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, 0, 0
						, NULL, 0, NULL, 0 );

		return ((status == 0) ? 1 : 0);
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_atomic_check_iter(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 mask, UINT8 flag, UINT32 maxloop) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		UINT32	loopcnt;
		SFLASH_BUS_TYPE	*bus;
		UINT32	bkupsleep;
		UINT8	rxdata[4];

		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		bkupsleep = JEDEC_GET_SLEEPTIME(sflash);

		for(loopcnt=0; loopcnt< maxloop; loopcnt++ ) {

			JEDEC_PRESET_SLEEP(sflash, 3,2); // 1msec

			bus->transmit( sflash, (UINT32)cmd, 0, 0
							, NULL, 0, rxdata, 1 );

			SFLASH_DBG_INFO("SR:%02x\n", rxdata[0]);

			if((rxdata[0] & mask) != flag){
				break;
			}

			JEDEC_SET_SLEEP(sflash, 3,2); // 1msec
		}

		JEDEC_PRESET_SLEEPTIME(sflash, bkupsleep);
		JEDEC_SET_SLEEPTIME(sflash, bkupsleep);

		if(loopcnt >= maxloop){
			return 0;
		}
		return 1;
	}
	return(0);
}

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_atomic_check(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 mask, UINT8 flag) CODE_SEC_SFLASH
{
	return sflash_jedec_atomic_check_iter(
			cmd, sflash, force, mask, flag, SFLASH_MAX_LOOP
		);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_addr_read(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
		, UINT32 force, UINT32 addr, UINT8 *data, UINT32 len) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 1, 0, 1); // CMD-ADDR-DATA	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, addr, 0
						, NULL, 0, data, len );
		return (status);
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_addr(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT32 addr) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 1, 0, 0); // CMD-ADDR	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, addr, 0
						, NULL, 0, NULL, 0 );
		return (status);
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_addr_write(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT32 addr, UINT8 *data, UINT32 len) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 1, 0, 1); // CMD-ADDR-DATA	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, addr, 0
						, data, len, NULL, 0 );
		return (status);
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   jedec atomic functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int  sflash_jedec_write(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
			, UINT32 force, UINT8 *data, UINT32 len) CODE_SEC_SFLASH
{
	if( (cmd != 0) && (sflash->bus != NULL) ){
		int	status;
		SFLASH_BUS_TYPE	*bus;
		bus = sflash->bus;

		force = sflash_jedec_bus_update(sflash, force);
		force = BUS_CADD_ON_UPDATE(force, 0x1111, 1, 0, 0, 1); // CMD-DATA	//@suppress("Suggested parenthesis around expression")

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &force );

		status = bus->transmit( sflash, (UINT32)cmd, 0, 0
						, data, len, NULL, 0 );
		return (status);
	}
	return (0);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFDP table of JEDEC
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	JEDEC_TABLE_ID	0
ATTRIBUTE_RAM_RODATA
static const int ms_major_rev = 1;
ATTRIBUTE_RAM_RODATA
static const int ms_minor_rev = 0;

ATTRIBUTE_RAM_FUNC
static int update_sfdp_current(UINT32 *major, UINT32 *minor, UINT32 *ptp, UINT8 *data) CODE_SEC_SFLASH
{
	*major = data[1];
	*minor = data[2];
	*ptp = (data[6] << 16) | (data[5] << 8) | (data[4] << 0) ;
	return ((int)(data[3]) << 2) ;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFDP table of JEDEC
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_RODATA
static const  char sflash_find_sfdp_token[] = "SFDP";

ATTRIBUTE_RAM_FUNC
int sflash_find_sfdp(SFLASH_HANDLER_TYPE *sflash, UINT8 *dump) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE *cmditem;
	UINT32 addr, nph, phase;
	UINT32	curr_major_rev = 0;
	UINT32	curr_minor_rev = 0;
	UINT32	curr_length  = 0;
	UINT32	curr_ptp  = 0;
	UINT32	table_found = FALSE;

#ifdef	SUPPORT_JERRY_SIM_MODEL

#ifdef	JERRY_SFLASH_IS25LQ032B
ATTRIBUTE_RAM_RODATA
	const	UINT8	sfdp_is25lq032b[96+16] = {
	0x53, 0x46, 0x44, 0x50, 0x00, 0x01, 0x01, 0xff, 0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0xff,
	0xc2, 0x00, 0x01, 0x04, 0x60, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe5, 0x20, 0xf1, 0xff, 0xff, 0xff, 0xff, 0x01, 0x44, 0xeb, 0x08, 0x6b, 0x08, 0x3b, 0x80, 0xbb,
	0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0x0c, 0x20, 0x0f, 0x52,
	0x10, 0xd8, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x36, 0x00, 0x27, 0x04, 0xa2, 0xd5, 0xdc, 0x00, 0x00, 0xa0, 0xff, 0x80, 0x00, 0x00, 0x00,
	};

	DRIVER_MEMCPY(dump, (void *)sfdp_is25lq032b, (96+16));
	sflash->jedec.sfdp = &(dump[48]);
	sflash->jedec.len = 36;
	return (96+16);
#elif	JERRY_SFLASH_MX25L25635F
ATTRIBUTE_RAM_RODATA
	const	UINT8	sfdp_mx25l25635f[96+16] = {
	0x53, 0x46, 0x44, 0x50, 0x00, 0x01, 0x01, 0xff, 0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0xff,
	0xc2, 0x00, 0x01, 0x04, 0x60, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe5, 0x20, 0xf3, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x44, 0xeb, 0x08, 0x6b, 0x08, 0x3b, 0x04, 0xbb,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x44, 0xeb, 0x0c, 0x20, 0x0f, 0x52,
	0x10, 0xd8, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x36, 0x00, 0x27, 0x9d, 0xf9, 0xc0, 0x64, 0x85, 0xcb, 0xaf, 0xff, 0xff, 0x3f, 0x00, 0x00,
	};

	DRIVER_MEMCPY(dump, (void *)sfdp_mx25l25635f, (96+16));
	sflash->jedec.sfdp = &(dump[48]);
	sflash->jedec.len = 36;
	return (96+16);
#else	//SUPPORT_SFLASH_MX25L25635E
ATTRIBUTE_RAM_RODATA
	const	UINT8	sfdp_mx25l25635e[96+16] = {
	0x53, 0x46, 0x44, 0x50, 0x00, 0x01, 0x01, 0xff, 0x00, 0x00, 0x01, 0x09, 0x30, 0x00, 0x00, 0xff,
	0xc2, 0x00, 0x01, 0x04, 0x60, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe5, 0x20, 0xf3, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x44, 0xeb, 0x08, 0x6b, 0x08, 0x3b, 0x04, 0xbb,
	0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0c, 0x20, 0x52, 0x10, 0x00,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x36, 0x00, 0x27, 0xf7, 0x4f, 0xff, 0xff, 0xd9, 0xc8, 0xaf, 0xff, 0xff, 0x3f, 0x00, 0x00,
	};

	DRIVER_MEMCPY(dump, (void *)sfdp_mx25l25635e, (96+16));
	sflash->jedec.sfdp = &(dump[48]);
	sflash->jedec.len = 36;
	return (96+16);
#endif //SUPPORT_SFLASH_MX25L25635F
#endif //SUPPORT_JERRY_SIM_MODEL

	addr = 0;

	cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RSFDP, sflash);
	phase = DARWMDCADD(0, 0,1,3, 1,0, 0,8, 0,0,0,0);
	sflash_jedec_addr_read((*cmditem), sflash
				, phase, addr, &(dump[addr]), 8);
	addr = addr + 8 ;

	if( DRIVER_MEMCMP(dump, sflash_find_sfdp_token, 4) != 0 ){
		return 0;
	}

	nph = dump[6];
	nph = nph + 1;

	while( nph > 0 ){

		sflash_jedec_addr_read((*cmditem), sflash
				, phase, addr, &(dump[addr]), 8);

		if( ( dump[addr] == JEDEC_TABLE_ID )
		   || ( dump[addr] != JEDEC_TABLE_ID ) /* amic violates the jedec standard */
		){
			if( dump[addr+2] > curr_major_rev
			    && dump[addr+2] <= ms_major_rev )
			{
				curr_length = update_sfdp_current(
						  &curr_major_rev
						, &curr_minor_rev
						, &curr_ptp
						, &(dump[addr]) );

				sflash_jedec_addr_read((*cmditem)
						, sflash, phase
						, curr_ptp
						, &(dump[curr_ptp]), curr_length );

				if( table_found == FALSE ){
					sflash->jedec.sfdp = &(dump[curr_ptp]);
					sflash->jedec.len = curr_length;
				}

				table_found = TRUE;
			}
			else if( dump[addr+2] == curr_major_rev
				 && dump[addr+2] <= ms_major_rev
				 && dump[addr+1] >  curr_minor_rev
				 && dump[addr+1] <= ms_minor_rev
				 )
			{
				curr_length = update_sfdp_current(
						  &curr_major_rev
						, &curr_minor_rev
						, &curr_ptp
						, &(dump[addr]) );

				sflash_jedec_addr_read((*cmditem)
						, sflash, phase
						, curr_ptp
						, &(dump[curr_ptp]), curr_length );

				if( table_found == FALSE ){
					sflash->jedec.sfdp = &(dump[curr_ptp]);
					sflash->jedec.len = curr_length;
				}

				table_found = TRUE;
			}
		}
		nph = nph - 1;
		addr = addr + 8 ;
	}

	if( table_found == TRUE  ){
		addr = curr_ptp;
	}

	return (addr+curr_length);
}


/******************************************************************************
 *   ( )
 *
 *  Purpose :   CFI table of JEDEC
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	JEDEC_QRY_ADDR		0x10

ATTRIBUTE_RAM_RODATA
static const  char sflash_find_cfi_token[] = "QRY";	

ATTRIBUTE_RAM_FUNC
int sflash_find_cfi(SFLASH_HANDLER_TYPE *sflash, UINT8 *dump) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE *cmditem;
	UINT32	phase;

	cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDID, sflash);
	phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
	sflash_jedec_read((*cmditem), sflash
				, phase, dump, 0x31);

	if( DRIVER_MEMCMP( &(dump[JEDEC_QRY_ADDR]), sflash_find_cfi_token, 3) != 0 ){
		return 0;
	}

	sflash->jedec.cfi = &(dump[0]);
	sflash->jedec.len = 0x31;

	return 0x31;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFDP INFO
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_SFLASH_INFO_PRINT
ATTRIBUTE_RAM_RODATA
static const UINT8 sfdp_wen_opcode[] = { 0x50, 0x06 };
ATTRIBUTE_RAM_RODATA
static const char *sfdp_addrbyte_mode[] = {
	"3 byte only", "3 or 4 bytes", "4 byte only", "unused"
};
#endif	//SUPPORT_SFLASH_INFO_PRINT

ATTRIBUTE_RAM_RODATA
const char frd112[] = "1-1-2 Fast Read";
ATTRIBUTE_RAM_RODATA
const char frd122[] = "1-2-2 Fast Read";
ATTRIBUTE_RAM_RODATA
const char frd222[] = "2-2-2 Fast Read";
ATTRIBUTE_RAM_RODATA
const char frd114[] = "1-1-4 Fast Read";
ATTRIBUTE_RAM_RODATA
const char frd144[] = "1-4-4 Fast Read";
ATTRIBUTE_RAM_RODATA
const char frd444[] = "4-4-4 Fast Read";
ATTRIBUTE_RAM_FUNC
void sflash_print_sfdp(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
#ifdef	SUPPORT_SFLASH_INFO_PRINT
	// JEDEC, JESD216B
	SFLASH_SFDP_TYPE *sfdp;
	UINT32	sfdp_len;

	if( sflash->jedec.sfdp == NULL ){
		return;
	}

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
	sfdp_len = sflash->jedec.len;

	SFLASH_JEDEC_PRINT("SFDP (%d)\n" , sfdp_len );

	SFLASH_JEDEC_PRINT("\tSector Erase Size: %d\n", sfdp->secsiz );
	SFLASH_JEDEC_PRINT("\tWrite Granularity: %d\n", sfdp->wrgran );
	SFLASH_JEDEC_PRINT("\tVolatile status bit: %d\n", sfdp->wenbit );
	if( sfdp->wenbit != 0 ){
		SFLASH_JEDEC_PRINT("\tWEN OPcode: %02x\n", sfdp_wen_opcode[sfdp->wen_op] );
	}
	SFLASH_JEDEC_PRINT("\t4KB Erase OPcode: %02x\n", sfdp->ers_op );
	SFLASH_JEDEC_PRINT("\tAddress Bytes: %s\n", sfdp_addrbyte_mode[sfdp->addbyt] );
	SFLASH_JEDEC_PRINT("\tDTR support: %d\n", sfdp->dtrmod );
	SFLASH_JEDEC_PRINT("\tMemory Density: %08x\n", sfdp->density );

	if( sfdp->frd112 != 0 ){
		//const char *frd112 = "1-1-2 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd112, sfdp->frwt112);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd112, sfdp->fr112md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd112, sfdp->fr112op);
	}
	if( sfdp->frd122 != 0 ){
		//const char *frd122 = "1-2-2 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd122, sfdp->frwt122);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd122, sfdp->fr122md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd122, sfdp->fr122op);
	}
	if( sfdp->frd222 != 0 ){
		//const char *frd222 = "2-2-2 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd222, sfdp->frwt222);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd222, sfdp->fr222md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd222, sfdp->fr222op);
	}

	if( sfdp->frd114 != 0 ){
		//const char *frd114 = "1-1-4 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd114, sfdp->frwt114);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd114, sfdp->fr114md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd114, sfdp->fr114op);
	}
	if( sfdp->frd144 != 0 ){
		//const char *frd144 = "1-4-4 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd144, sfdp->frwt144);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd144, sfdp->fr144md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd144, sfdp->fr144op);
	}
	if( sfdp->frd444 != 0 ){
		//const char *frd444 = "4-4-4 Fast Read";
		SFLASH_JEDEC_PRINT("\t%s, Dummy cycles : %d\n", frd444, sfdp->frwt444);
		SFLASH_JEDEC_PRINT("\t%s, Mode  cycles : %d\n", frd444, sfdp->fr444md);
		SFLASH_JEDEC_PRINT("\t%s, OP code : %02x\n", frd444, sfdp->fr444op);
	}


	if( sfdp->ers1siz != 0 ){
		SFLASH_JEDEC_PRINT("\tErase 1 Size   : %d\n", (1<<(sfdp->ers1siz)));
		SFLASH_JEDEC_PRINT("\tErase 1 OP code: %02x\n", sfdp->ers1op);
	}
	if( sfdp->ers2siz != 0 ){
		SFLASH_JEDEC_PRINT("\tErase 2 Size   : %d\n", (1<<(sfdp->ers2siz)));
		SFLASH_JEDEC_PRINT("\tErase 2 OP code: %02x\n", sfdp->ers2op);
	}
	if( sfdp->ers3siz != 0 ){
		SFLASH_JEDEC_PRINT("\tErase 3 Size   : %d\n", (1<<(sfdp->ers3siz)));
		SFLASH_JEDEC_PRINT("\tErase 3 OP code: %02x\n", sfdp->ers3op);
	}
	if( sfdp->ers4siz != 0 ){
		SFLASH_JEDEC_PRINT("\tErase 4 Size   : %d\n", (1<<(sfdp->ers4siz)));
		SFLASH_JEDEC_PRINT("\tErase 4 OP code: %02x\n", sfdp->ers4op);
	}

#ifdef	SUPPORT_STD_JESD216B

	if( sfdp_len == SFLASH_JESD216B_LEN ){	// JESD216B
		SFLASH_JEDEC_PRINT("\tPage size: %02x\n", sfdp->pagsiz);

		if( sfdp->frd444 != 0 ){
			SFLASH_JEDEC_PRINT("\t4-4-4 disable: %02x\n", sfdp->dis444);
			SFLASH_JEDEC_PRINT("\t4-4-4 enable : %02x\n", sfdp->enb444);
		}
		if( sfdp->sup044 != 0 ){
			SFLASH_JEDEC_PRINT("\t0-4-4 exit  : %02x\n", sfdp->ext044);
			SFLASH_JEDEC_PRINT("\t0-4-4 entry : %02x\n", sfdp->ent044);
		}

		SFLASH_JEDEC_PRINT("\tQER: %02x\n", sfdp->qer);

		SFLASH_JEDEC_PRINT("\tExit 4 Byte Addr : %02x\n", sfdp->ext4adr);
		SFLASH_JEDEC_PRINT("\tEnter 4 Byte Addr: %02x\n", sfdp->ent4adr);

		if( sfdp->suppdpd != 0 ){
			SFLASH_JEDEC_PRINT("\tDPDcode: %02x\n", sfdp->edpdop);
			SFLASH_JEDEC_PRINT("\tRDPcode: %02x\n", sfdp->xdpdop);
			SFLASH_JEDEC_PRINT("\tDPdelay: %02x\n", sfdp->xdpddly);
		}
	}

#endif	//SUPPORT_STD_JESD216B

	if( sflash->jedec.extra != NULL ){
		SFLASH_EXTRA_TYPE *extra;

		extra = (SFLASH_EXTRA_TYPE *)(sflash->jedec.extra);

		SFLASH_JEDEC_PRINT("EXT.magic = %d\n", extra->magiccode );

		if( extra->dyblock == 1 ){
			SFLASH_JEDEC_PRINT("EXT.DYBWR = %02x\n", extra->dybwr_op );
			SFLASH_JEDEC_PRINT("EXT.DYBRD = %02x\n", extra->dybrd_op );
			SFLASH_JEDEC_PRINT("EXT.LOCK = %02x\n", extra->dyblock_cod );
			SFLASH_JEDEC_PRINT("EXT.UNLOCK = %02x\n", extra->dybunlock_cod );
		}

		if( extra->ppblock == 1 ){
			SFLASH_JEDEC_PRINT("EXT.PPBLCK = %02x\n", extra->ppblock_op );
			SFLASH_JEDEC_PRINT("EXT.PPBULCK = %02x\n", extra->ppbunlock_op );
			SFLASH_JEDEC_PRINT("EXT.PPBRD = %02x\n", extra->ppbrd_op );
			SFLASH_JEDEC_PRINT("EXT.PPBLOCK = %02x\n", extra->ppblock_cod );
		}

		if( extra->ppblock == 1 || extra->dyblock == 1 ){
			if( extra->lck4addr == 1 ){
				SFLASH_JEDEC_PRINT("EXT.Lock Address = 4 byte only\n");
			}
		}

		if( extra->blkprot == 1 ){
			SFLASH_JEDEC_PRINT("EXT.BPmask = %02x\n", extra->bpmask);
			SFLASH_JEDEC_PRINT("EXT.BPUnlock = %02x\n", extra->bpunlck_cod);
		}

		SFLASH_JEDEC_PRINT("EXT.SDR = %d\n", extra->sdr_speed );
		SFLASH_JEDEC_PRINT("EXT.FSR = %d\n", extra->fsr_speed );
		SFLASH_JEDEC_PRINT("EXT.DDR = %d\n", extra->ddr_speed );
		SFLASH_JEDEC_PRINT("EXT.QDR = %d\n", extra->qdr_speed );
		SFLASH_JEDEC_PRINT("EXT.DTR = %d\n", extra->dtr_speed );
		SFLASH_JEDEC_PRINT("EXT.MAX = %d\n", extra->max_speed );

		SFLASH_JEDEC_PRINT("EXT.SEC.Total = %d\n", extra->total_sec );
		SFLASH_JEDEC_PRINT("EXT.SEC.Top4K = %d\n", extra->n_top4ksec );
		SFLASH_JEDEC_PRINT("EXT.SEC.Bottom4K = %d\n", extra->n_bot4ksec );
		SFLASH_JEDEC_PRINT("EXT.SEC.32K = %d\n", extra->n_32ksec );
		SFLASH_JEDEC_PRINT("EXT.SEC.64K = %d\n", extra->n_64ksec );
		SFLASH_JEDEC_PRINT("EXT.SEC.128K = %d\n",
						(extra->total_sec
							- extra->n_top4ksec
							- extra->n_bot4ksec
							- extra->n_32ksec
							- extra->n_64ksec )
					);
	}

	if( sflash->jedec.cmdlist != NULL ){
		int i;
		SFLASH_CMD_TYPE *cmdlist;
		cmdlist = (SFLASH_CMD_TYPE *)sflash->jedec.cmdlist;
		for( i = 0;  i < SFLASH_CMD_JEDEC_MAX; i++ ){
			if( sflash_jedec_cmdlist_str[i] != NULL ){
				SFLASH_JEDEC_PRINT("%6s : %02x\n"
					, sflash_jedec_cmdlist_str[i]
					, cmdlist[i]
					);
			}
		}
	}
#else
	DA16X_UNUSED_ARG(sflash);
#endif	//SUPPORT_SFLASH_INFO_PRINT
}

ATTRIBUTE_RAM_FUNC
int sflash_find_sfdppage(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
	SFLASH_SFDP_TYPE *sfdp;
	UINT32	sfdp_len;

#ifdef	SUPPORT_STD_JESD216B
	if( sflash->jedec.sfdp != NULL ){

		sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
		sfdp_len = sflash->jedec.len;

		if( sfdp_len >= 44 ){ /* 11 DWORD */
			return (1 << (sfdp->pagsiz));
		}
	}
#endif	//SUPPORT_STD_JESD216B
	return 0;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   CFI INFO
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void sflash_print_cfi(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
#ifdef	SUPPORT_SFLASH_INFO_PRINT
	// JEDEC, JEP137B
	SFLASH_CFI_TYPE *cfi;
	UINT32	data;

	cfi = (SFLASH_CFI_TYPE *)sflash->jedec.cfi;
	SFLASH_JEDEC_PRINT("CFI\n" );

	data = 1 << (cfi->geo_info[SFLASH_GEO_SIZE]);
	SFLASH_JEDEC_PRINT("\tMemory Density: %d\n", data );

	data = ((cfi->geo_info[SFLASH_GEO_ADDR+1])<<8)
		| (cfi->geo_info[SFLASH_GEO_ADDR]) ;
	SFLASH_JEDEC_PRINT("\tInterface Descript: %x\n", data );

	data = ((cfi->geo_info[SFLASH_GEO_PAGE+1])<<8)
		| (cfi->geo_info[SFLASH_GEO_PAGE]) ;
	data = 1 << data ;
	SFLASH_JEDEC_PRINT("\tPage Size: %d\n", data );

	data = cfi->geo_info[SFLASH_GEO_BOOT];
	SFLASH_JEDEC_PRINT("\tErase Block Regions: %d\n", data );
#else
	DA16X_UNUSED_ARG(sflash);
#endif	//SUPPORT_SFLASH_INFO_PRINT
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static  void sflash_enter_adr4byte(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 newbusmode) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(newbusmode);

	SFLASH_CMD_TYPE *cmditem;
	UINT32 phase;

	SFLASH_DBG_BASE("%s (%x)",  __func__, sfdp->ent4adr);

	if( (sfdp->ent4adr & 0x10) != 0x00 ){ // numonyx
		UINT8	data[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0xB5, sflash, phase, data, 2);

		if( (data[0] & (0x01)) != 0 ){
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			data[0] = data[0] & (~(0x01)); // NVCR
			sflash_jedec_write( 0xB1, sflash, phase, data, 2);
		}
	}
	if( (sfdp->ent4adr & 0x01) != 0x00 ){
		// Instruction
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_EN4B, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
	}
	if( (sfdp->ent4adr & 0x02) != 0x00 ){
		// Instruction, numonyx
		sflash_unknown_set_wren(sflash);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_EN4B, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
	}
	if( (sfdp->ent4adr & 0x04) != 0x00 ){
	}
	if( (sfdp->ent4adr & 0x08) != 0x00 ){
		UINT8	regdata[4];
		// Bank address : spansion
#if 0
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRWR, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		regdata[0] = 0x80; // EXTADD
		sflash_jedec_write( (*cmditem), sflash, phase, regdata, 1);
#else
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRRD, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( (*cmditem), sflash, phase, regdata, 1);

		if( (regdata[0] & (0x80)) == 0x00 ){
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRWR, sflash);
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			regdata[0] |= (0x80); // EXTADD
			sflash_jedec_write( (*cmditem), sflash, phase, regdata, 1);
		}
#endif
	}
	if( (sfdp->ent4adr & 0x20) != 0x00 ){
	}
}

ATTRIBUTE_RAM_FUNC
static  void sflash_exit_adr4byte(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 newbusmode) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(newbusmode);

	SFLASH_CMD_TYPE *cmditem;
	UINT32 phase;

	SFLASH_DBG_BASE("%s (%x)",  __func__, sfdp->ext4adr);

	if( (sfdp->ext4adr & 0x01) != 0x00 ){
		// Instruction
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_EX4B, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
	}
	if( (sfdp->ext4adr & 0x02) != 0x00 ){
		// Instruction, numonyx
		sflash_unknown_set_wren(sflash);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_EX4B, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
	}
	if( (sfdp->ext4adr & 0x04) != 0x00 ){
	}
	if( (sfdp->ext4adr & 0x08) != 0x00 ){
		UINT8	regdata[4];
		// Bank address : spansion
#if 0
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRWR, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		regdata[0] = 0x00; // EXTADD
		sflash_jedec_write( (*cmditem), sflash, phase, regdata, 1);
#else
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRRD, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( (*cmditem), sflash, phase, regdata, 1);

		if( (regdata[0] & (0x80)) == 0x80 ){
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_BRWR, sflash);
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			regdata[0] &= (~0x80); // EXTADD
			sflash_jedec_write( (*cmditem), sflash, phase, regdata, 1);
		}
#endif
	}
	if( (sfdp->ext4adr & 0x10) != 0x00 ){ // numonyx
		UINT8	data[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0xB5, sflash, phase, data, 2);

		if( (data[0] & (0x01)) == 0 ){
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			data[0] = data[0] | (0x01); // NVCR
			sflash_jedec_write( 0xB1, sflash, phase, data, 2);
		}
	}
	if( (sfdp->ext4adr & 0x20) != 0x00 ){
	}
	if( (sfdp->ext4adr & 0x40) != 0x00 ){
		/* jedec standard */
		JEDEC_PRESET_SLEEP(sflash, 1,1); // 8usec
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RSTEN, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
		JEDEC_SET_SLEEP(sflash, 1,1); // 8usec

		JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RESET, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( (*cmditem), sflash, phase);
		JEDEC_SET_SLEEP(sflash, 1,5); // 48usec
	}
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static  void sflash_enter_qpi(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 addr4byte ) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(addr4byte);

	UINT32 phase;

	SFLASH_DBG_BASE("%s (%x)",  __func__, sfdp->enb444);

	if(sfdp->enb444 & 0x10){	// read-modify-write (VECR) :: numonyx
		UINT8	data[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0x65, sflash, phase, data, 1);

		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		data[0] = data[0] & (~((1<<7)|(1<<4)|(1<<3))); // VECR :: QIC, DIS, QIO
		sflash_jedec_write( 0x61, sflash, phase, data, 1);
	}

	if(sfdp->enb444 & 0x01){	// set QE per QER, then issue 0x38
	}
	if(sfdp->enb444 & 0x02){	// 0x38
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic(0x38, sflash, phase);
	}
	if(sfdp->enb444 & 0x04){	// 0x35
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic(0x35, sflash, phase);
	}
	if(sfdp->enb444 & 0x08){	// read-modify-write
	}

}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static  void sflash_exit_qpi(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 addr4byte ) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(addr4byte);

	UINT32 phase;

	SFLASH_DBG_BASE("%s (%x, %x)",  __func__, sfdp->dis444, sfdp->enb444);

	if(sfdp->enb444 & 0x10){	// read-modify-write (VECR) :: numonyx
		UINT8		data[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0x65, sflash, phase, data, 1);

		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		data[0] = data[0] | ((1<<7)|(1<<3)); // VECR :: QIC, QIO
		sflash_jedec_write( 0x61, sflash, phase, data, 1);
	}

	if(sfdp->dis444 & 0x01){	// 0xFF
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic(0xFF, sflash, phase);
	}
	if(sfdp->dis444 & 0x02){	// 0xF5
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic(0xF5, sflash, phase);
	}
	if(sfdp->dis444 & 0x04){	// read-modify-write
	}
	if(sfdp->dis444 & 0x08){	// soft reset 66/99
		JEDEC_PRESET_SLEEP(sflash, 1,1); // 8usec
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( 0x66, sflash, phase);
		JEDEC_PRESET_SLEEP(sflash, 1,1); // 8usec

		JEDEC_PRESET_SLEEP(sflash, 1,4); // 32usec
		sflash_jedec_atomic( 0x99, sflash, phase);
		JEDEC_PRESET_SLEEP(sflash, 1,4); // 32usec
	}

}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static  void sflash_enter_qspi(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 addr4byte ) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(addr4byte);

	SFLASH_CMD_TYPE *cmditem;
	UINT32		phase;
	UINT8		data[4];

	SFLASH_DBG_BASE("%s (%x)",  __func__, sfdp->qer);

	switch(sfdp->qer){
	case	0x00: /* instruction */
			break;
	case	0x01: /* QE is bit 1 of status register 2. read 05, write 01 with 2 bytes, writing 1 byte clear SR2 */
			break;
	case	0x02: /* QE is bit 6 of status register 1, read 05, write 01 :: efeon, macronix */
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( (*cmditem), sflash, phase, data, 1);

			if( (data[0] & (SR1_QEN)) == 0 ){
				data[0] = data[0] |  SR1_QEN ;
				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRR, sflash);
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( (*cmditem), sflash, phase, data, 1);

				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);
			}
			break;
	case	0x03: /* QE is bit 7 of status register 2. read 3f, write 3e :: adestro */
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( 0x3f, sflash, phase, data, 1);

			if( (data[0] & (1<<7)) == 0 ){
				data[0] = data[0] |  (1<<7) ;
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( 0x3e, sflash, phase, data, 1);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x3f, sflash
							, phase, (1<<7), 0x00 );
			}
			break;
	case	0x04: /* QE is bit 1 of status register 2. read 05, write 01 with 2 bytes, writing 1 byte doesn't modify SR2 */
			break;
	case	0x05: /* QE is bit 1 of status register 2. read 35, write 01 with 2 bytes :: spansion */
			{	// 32 bit alignment issue
				UINT8	sprxdata[4];

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_read( 0x05, sflash, phase, &(sprxdata[0]), 1);
				data[0] = sprxdata[0] ;
				sflash_jedec_read( 0x35, sflash, phase, &(sprxdata[0]), 1);
				data[1] = sprxdata[0] ;
			}

			if( (data[1] & (1<<1)) == 0 ){
				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRR, sflash);
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				data[0] = data[0] & 0x3F; // LC
				data[1] = data[1] | (1<<1); // QUAD
				sflash_jedec_write( (*cmditem), sflash, phase, data, 2);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x35, sflash
							, phase, (1<<1), 0x00 );
			}
			break;
	case	0x06: /* QE is bit 1 of status register 2. read 35, write 31 with 1 bytes ::  winbond : non-jesd216B */
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( 0x35, sflash, phase, data, 1);

			if( (data[0] & (1<<1)) == 0 ){
				data[0] = data[0] |  (1<<1) ;
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( 0x31, sflash, phase, data, 1);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x35, sflash
							, phase, (1<<1), 0x00 );
			}
			break;
	case	0x07: /* reserved */
			break;
	}

	if(sfdp->enb444 & 0x10){	// read-modify-write (VECR) :: numonyx
		UINT8	data1[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0x65, sflash, phase, data1, 1);

		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		data1[0] = (data1[0] | (1<<7)) & (~((1<<4)|(1<<3))); // VECR :: QIC, DIS, QIO
		sflash_jedec_write( 0x61, sflash, phase, data1, 1);
	}
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static  void sflash_exit_qspi(SFLASH_HANDLER_TYPE *sflash
				, SFLASH_SFDP_TYPE *sfdp
				, UINT32 addr4byte ) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(addr4byte);

	SFLASH_CMD_TYPE *cmditem;
	UINT32		phase;
	UINT8		data[4];

	SFLASH_DBG_BASE("%s (%x,%x)",  __func__, sfdp->qer, sfdp->hrdis);

#ifdef	SUPPORT_QEBIT_ISSUE
	if(sfdp->hrdis == 1 ){
		return;
	}
#endif	//SUPPORT_QEBIT_ISSUE

	switch(sfdp->qer){
	case	0x00: /* instruction */
			break;
	case	0x01: /* QE is bit 1 of status register 2. read 05, write 01 with 2 bytes, writing 1 byte clear SR2 */
			break;
	case	0x02: /* QE is bit 6 of status register 1, read 05, write 01 :: efeon, macronix */
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( (*cmditem), sflash, phase, data, 1);

			if( (data[0] & (SR1_QEN)) != 0 ){
				data[0] = data[0] & ~SR1_QEN ;
				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRR, sflash);
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( (*cmditem), sflash, phase, data, 1);

				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check( (*cmditem), sflash
							, phase, SR1_WIP, SR1_WIP);
			}
			break;
	case	0x03: /* QE is bit 7 of status register 2. read 3f, write 3e :: adestro */
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( 0x3f, sflash, phase, data, 1);

			if( (data[0] & (1<<7)) != 0 ){
				data[0] = data[0] &  (~(1<<7)) ;
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( 0x3e, sflash, phase, data, 1);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x3f, sflash
							, phase, (1<<7), (1<<7) );
			}
			break;
	case	0x04: /* QE is bit 1 of status register 2. read 05, write 01 with 2 bytes, writing 1 byte doesn't modify SR2 */
			break;
	case	0x05: /* QE is bit 1 of status register 2. read 35, write 01 with 2 bytes :: spansion */
			{	// 32 bit alignment issue
				UINT8	sprxdata[4];

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_read( 0x05, sflash, phase, &(sprxdata[0]), 1);
				data[0] = sprxdata[0] ;
				sflash_jedec_read( 0x35, sflash, phase, &(sprxdata[0]), 1);
				data[1] = sprxdata[0] ;
			}

			if( (data[1] & (1<<1)) != 0 ){
				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRR, sflash);
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				data[0] = data[0] & 0x3F; // LC
				data[1] = data[1] & (~(1<<1)); // QUAD
				sflash_jedec_write( (*cmditem), sflash, phase, data, 2);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x35, sflash
							, phase, (1<<1), (1<<1) );
			}
			break;
	case	0x06: /* QE is bit 1 of status register 2. read 35, write 31 with 1 bytes ::  winbond : non-jesd216B */
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_read( 0x35, sflash, phase, data, 1);

			if( (data[0] & (1<<1)) != 0 ){
				data[0] = data[0] &  (~(1<<1)) ;
				phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
				sflash_jedec_write( 0x31, sflash, phase, data, 1);

				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic_check(0x35, sflash
							, phase, (1<<1), (1<<1) );
			}
			break;
	case	0x07: /* reserved */
			break;
	}

	if(sfdp->enb444 & 0x10){	// read-modify-write (VECR) :: numonyx
		UINT8	data1[4];

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( 0x65, sflash, phase, data1, 1);

		phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
		data1[0] = data1[0] | ((1<<7)|(1<<3)); // VECR :: QIC, QIO
		sflash_jedec_write( 0x61, sflash, phase, data1, 1);
	}
}

/******************************************************************************
 *  sflash_unknown_busmode ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static UINT32 sflash_unknown_addrchange(SFLASH_HANDLER_TYPE *sflash, UINT32 newbusmode)
{
	SFLASH_SFDP_TYPE *sfdp;
	UINT32 addr4byte;

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
	addr4byte = (newbusmode & SFLASH_BUS_4BADDR);

	if((sflash->accmode & SFLASH_BUS_4BADDR) != addr4byte){
		switch(addr4byte){
		case 	SFLASH_BUS_4BADDR:
			sflash_enter_adr4byte(sflash, sfdp, SFLASH_BUS_GRP(sflash->accmode));
			break;
		default:
			sflash_exit_adr4byte(sflash, sfdp, SFLASH_BUS_GRP(sflash->accmode));
		break;
		}
	}

	return (SFLASH_BUS_GRP(sflash->accmode)|(newbusmode & SFLASH_BUS_4BADDR));
}

ATTRIBUTE_RAM_FUNC
int	sflash_unknown_busmode(SFLASH_HANDLER_TYPE *sflash, UINT32 busmode)  CODE_SEC_SFLASH
{
	UINT32	newbusmode, addr4byte, flag_addr;
	SFLASH_SFDP_TYPE *sfdp;

	if( sflash->jedec.sfdp == NULL ){
		return SFLASH_BUS_NOGRP(busmode)|SFLASH_BUS_GRP(sflash->accmode);
	}

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	SFLASH_DBG_NONE("%s: check - old %x, new %x"
			,  __func__
			, sflash->accmode, busmode );

	newbusmode = SFLASH_BUS_GRP(busmode) ;
	flag_addr = 0;

	switch(sfdp->addbyt){
		case	1: // 3 Byte or 4 Byte, default 3
			addr4byte  = SFLASH_BUS_4BADDR & (SFLASH_BUS_3BADDR | busmode) ;
			break;
		case	2: // 4 Byte address only
			addr4byte  = SFLASH_BUS_4BADDR ;
			break;
		default: // 3 Byte only
			addr4byte = SFLASH_BUS_3BADDR;
			break;
	}

sflash_unknown_busmode_loop:
	switch(SFLASH_BUS_GRP(newbusmode)){
		case SFLASH_BUS_444:
			if(sfdp->frd444 != 0) {
				newbusmode = SFLASH_BUS_444;
				break;
			}
			newbusmode = SFLASH_BUS_144;
			goto sflash_unknown_busmode_loop;
		case SFLASH_BUS_144:
			if(sfdp->frd144 != 0) {
				newbusmode = SFLASH_BUS_144;
				break;
			}
			newbusmode = SFLASH_BUS_114;
			goto sflash_unknown_busmode_loop;
		case SFLASH_BUS_114:
			if(sfdp->frd114 != 0) {
				newbusmode = SFLASH_BUS_114;
				break;
			}
#ifdef	SUPPORT_SLR_MANAGEMENT
			newbusmode = SFLASH_BUS_222;
			goto sflash_unknown_busmode_loop;
#else	//SUPPORT_SLR_MANAGEMENT
			else{
				newbusmode = SFLASH_BUS_111;
			}
			break;
#endif	//SUPPORT_SLR_MANAGEMENT

		case SFLASH_BUS_222:
			if(sfdp->frd222 != 0) {
				newbusmode = SFLASH_BUS_222;
				break;
			}
			newbusmode = SFLASH_BUS_122;
			goto sflash_unknown_busmode_loop;
		case SFLASH_BUS_122:
			if(sfdp->frd122 != 0) {
				newbusmode = SFLASH_BUS_122;
				break;
			}
			newbusmode = SFLASH_BUS_112;
			goto sflash_unknown_busmode_loop;
		case SFLASH_BUS_112:
			newbusmode = (sfdp->frd112 == 0)
				? SFLASH_BUS_111 : SFLASH_BUS_112;
			break;
		default:
			break;
	}

	newbusmode = newbusmode | (addr4byte & SFLASH_BUS_4BADDR) ;

	if( (sflash->accmode & (SFLASH_BUS_4BADDR|SFLASH_BUS_MASK))
		== (newbusmode & (SFLASH_BUS_4BADDR|SFLASH_BUS_MASK)) ){
		SFLASH_DBG_BASE("%s: busmode not changed - %x"
				,  __func__
				, sflash->accmode );

		return newbusmode;
	}

	sflash_unknown_setspeed(sflash, 1); /* slow */

	SFLASH_DBG_INFO("%s: busmode - old %x, new %x (%x)"
			,  __func__
			, sflash->accmode
			, newbusmode, addr4byte );

	sflash_unknown_set_wren(sflash);

	switch(SFLASH_BUS_GRP(newbusmode)){
	case	SFLASH_BUS_444:
	case	SFLASH_BUS_044:
	case	SFLASH_BUS_004:
		switch(SFLASH_BUS_GRP(sflash->accmode)){
		case SFLASH_BUS_444_GRP:
			break;
		case SFLASH_BUS_114_GRP:
		case SFLASH_BUS_144_GRP:
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash_exit_qspi( sflash, sfdp, addr4byte );
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			// NOTICE: You MUST set MXIC in this order.
			//         EN4B > EQIO
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			flag_addr = 1;
			sflash_enter_qpi( sflash, sfdp, addr4byte );
			break;
		default:
			// NOTICE: You MUST set MXIC in this order.
			//         EN4B > EQIO
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			flag_addr = 1;
			sflash_enter_qpi( sflash, sfdp, addr4byte );
			break;
		}
		break;
	case	SFLASH_BUS_144:
	case	SFLASH_BUS_114:
	case	SFLASH_BUS_014:
		switch(SFLASH_BUS_GRP(sflash->accmode)){
		case SFLASH_BUS_114_GRP:
		case SFLASH_BUS_144_GRP:
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			flag_addr = 1;
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			break;
		case SFLASH_BUS_444_GRP:
			sflash_exit_qpi( sflash, sfdp, addr4byte );
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			flag_addr = 1;
			sflash_enter_qspi( sflash, sfdp, addr4byte );
			break;
		default:
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			flag_addr = 1;
			sflash_enter_qspi( sflash, sfdp, addr4byte );
			break;
		}
		break;
	default:
		switch(SFLASH_BUS_GRP(sflash->accmode)){
		case SFLASH_BUS_114_GRP:
		case SFLASH_BUS_144_GRP:
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash_exit_qspi( sflash, sfdp, addr4byte );
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			break;
		case SFLASH_BUS_444_GRP:
			sflash_exit_qpi( sflash, sfdp, addr4byte );
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			break;
		default:
			sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|SFLASH_BUS_111;
			sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
			sflash_unknown_set_wrdi(sflash); // Dummy Action :: RSTQPI Issue
			sflash_unknown_set_wren(sflash); // Dummy Action :: RSTQPI Issue
			flag_addr = 1;
			break;
		}
		break;
	}

	sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(newbusmode);

	if( flag_addr == 0 ){
		sflash->accmode = sflash_unknown_addrchange(sflash, newbusmode);
	}

	sflash_unknown_set_wrdi(sflash);

	return (sflash->accmode);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_buslock(SFLASH_HANDLER_TYPE *sflash, UINT32 lockmode)  CODE_SEC_SFLASH
{
	SFLASH_BUS_TYPE		*bus;

	bus = (SFLASH_BUS_TYPE *)sflash->bus;

	SFLASH_DBG_NONE("%s: lock = %d", __func__, lockmode);

	bus->lock( sflash, lockmode );

	return TRUE;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void sflash_unknown_setspeed(SFLASH_HANDLER_TYPE *sflash, UINT32 mode) CODE_SEC_SFLASH
{
	SFLASH_BUS_TYPE		*bus;
	SFLASH_EXTRA_TYPE 	*extra;
	volatile UINT8 		newspeed ;
	VOID			*delay;

	bus = (SFLASH_BUS_TYPE *)sflash->bus;
	extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;

	if( sflash->jedec.extra == NULL ) {
		// Unknown device
		switch(mode){
		case	0:
		case	1:	newspeed = 15 /*MHz*/; // 20 - very slow & slow ;; 2018.07.16 MPW ISSUE under 15MHz
				break;
		default:	newspeed = 15 /*MHz*/; // 28 - fast, Under 28 MHz (Old SFLASH support) ;; 2018.07.16 MPW ISSUE under 15MHz
				break;
		}

		delay = NULL;
	}else{
		switch(mode) {
		case	0:	/* very slow */
				newspeed = 15 /*MHz*/; // 20 - very slow ;; 2018.07.16 MPW ISSUE under 15MHz
				break;
		case	1:	/* slow */
				newspeed = extra->sdr_speed ;
				break;
		default:	/* parametric : fast */
				switch(SFLASH_BUS_GRP(sflash->accmode)){
					case SFLASH_BUS_444:
					case SFLASH_BUS_044:
					case SFLASH_BUS_004:
					case SFLASH_BUS_144:
					case SFLASH_BUS_114:
					case SFLASH_BUS_014:
						newspeed = extra->qdr_speed ;
						break;
					case SFLASH_BUS_222:
					case SFLASH_BUS_022:
					case SFLASH_BUS_002:
					case SFLASH_BUS_122:
					case SFLASH_BUS_112:
					case SFLASH_BUS_012:
						newspeed = extra->ddr_speed ;
						break;
					default: /*  SFLASH_BUS_111 */
						newspeed = extra->fsr_speed;
						break;
				}
				break;
		}

		if( newspeed > extra->max_speed ){
			newspeed = extra->max_speed; /* max */
		}

		delay = (VOID *)(sflash->jedec.delay);
	}

	bus->ioctl( sflash, SFLASH_BUS_SET_SPEED, (VOID *)(&newspeed) );
	bus->ioctl( sflash, SFLASH_BUS_SET_DELAYSEL, (VOID *)(delay) );
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void sflash_unknown_set_wren(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE		*cmditem;
	SFLASH_SFDP_TYPE	*sfdp;
	UINT32			phase;

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	if( sfdp == NULL || sfdp->wenbit == 0 ){
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WREN, sflash);
	}else{
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRENA, sflash);
	}
	phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
	sflash_jedec_atomic( (*cmditem), sflash, phase);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void sflash_unknown_set_wrdi(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE		*cmditem;
	UINT32			phase;

	cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRDI, sflash);
	phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
	sflash_jedec_atomic( (*cmditem), sflash, phase);
}

/******************************************************************************
 *  sflash_unknown_probe ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_probe(SFLASH_HANDLER_TYPE *sflash, VOID *anonymous, UINT8 *dump) CODE_SEC_SFLASH
{
	SFLASH_DEV_TYPE		*device;
	SFLASH_CMD_TYPE 	*cmditem;
	UINT8			*rxdata, rxdump[8];
	int			status, max_cfi;
	UINT32	i;
	UINT32			phase;


	device = (SFLASH_DEV_TYPE *)anonymous;

	SFLASH_DBG_BASE("%s: S(%p), H(%p), D(%p), A(%x)"
				, __func__
				, sflash, sflash->bushdlr, device
				, sflash->accmode);

	sflash_unknown_setspeed(sflash, 0); /* very slow */

	max_cfi = 0;
	if( sflash->max_cfi == 0 ){
		int	j;

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDID, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		i = sflash_jedec_read((*cmditem), sflash
				, phase
				, dump, SFLASH_CMD_RDID_LEN);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_REMS, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		max_cfi = sflash_jedec_addr_read((*cmditem), sflash
				, phase
				, 0, &(rxdump[0]), SFLASH_CMD_REMS_LEN);

		rxdata  = dump;
		for(j = 0; j < max_cfi; j++){
			rxdata[i+j] = rxdump[j];
		}
		max_cfi = i + max_cfi;
	}else{
		if( (( max_cfi = sflash_find_sfdp( sflash, dump ))== 0)
		  && (( max_cfi = sflash_find_cfi( sflash, dump ))== 0) ){

			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDID, sflash);
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			max_cfi = sflash_jedec_read((*cmditem), sflash
					, phase
					, dump, SFLASH_CMD_RDID_LEN);
		}
		rxdata = dump;

		if( sflash->jedec.sfdp == NULL ){
			SFLASH_DBG_BASE("SFLASH info - pseudo sfdp");
			sflash->jedec.sfdp = device->sfdp;
			sflash->jedec.len  = sizeof(SFLASH_SFDP_TYPE);
		}
	}

	SFLASH_DBG_NONE("SFLASH info - man %x, dev %x, density %x"
			, rxdata[0], rxdata[1], rxdata[2] );

	status = 0;

	if( sflash->max_cfi == 0 ){
		return ID_CFI_MAX_LENGTH;
	}

	if( device->mfr_id == 0 && device->dev_id == 0 && device->density == 0 ){
		status ++ ;
	}

	if( status != 0 ){
		SFLASH_DBG_WARN("SFLASH mismatched,"
			     "man %x, dev %x, density %x"
			     , rxdata[0]
			     , rxdata[1]
			     , rxdata[2]
			     );
		return 0;
	}else{
		SFLASH_DBG_BASE("SFLASH : %s", device->name);
	}

	// Tune up
	device->sec_grp[0].offset = 0;
	for( i=1 ; i < device->num_secgrp ; i++ ){
		device->sec_grp[i].offset
		    = device->sec_grp[i-1].offset
		    + ( device->sec_grp[i-1].blksiz * device->sec_grp[i-1].blknum ) ;
	}

#ifdef	SUPPORT_SFLASH_LOCKINFO
	if( dump == NULL ){
		for( i=0 ; i< device->num_sector ; i++ ){
			device->protect[i]	= FALSE ;
		}
	}else{
		UINT32	num, addr;
		UINT32  num_acc = 0;

		for(i =0; i < device->num_secgrp; i++){
			for(num = 0; num < device->sec_grp[i].blknum; num++){
				addr = device->sec_grp[i].offset
					+ (num * device->sec_grp[i].blksiz) ;

				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_DYBWR, sflash);
				phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_addr_read((*cmditem), sflash
						, phase
						, addr, rxdump, 1);

				device->protect[num_acc]
					= (rxdump[0] == DYB_UNLOCK) ? FALSE : TRUE;
				num_acc ++;
			}
		}
	}
#endif	//SUPPORT_SFLASH_LOCKINFO

	return max_cfi;
}

/******************************************************************************
 *  sflash_unknown_reset ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_reset(SFLASH_HANDLER_TYPE *sflash, UINT32 mode) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE 	*cmditem;
	UINT32			phase, loopcnt, busmode;
	SFLASH_SFDP_TYPE 	*sfdp;

	SFLASH_DBG_BASE("%s - %d", __func__ , mode);

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	sflash_unknown_setspeed(sflash, 0); /* very slow */

	// Mode 0 : Wakeup only - 111 Wakeup
	// Mode 1 : Fast Reset  - 111 Wakeup & Reset
	// Mode 2 : Slow Reset  - 444 Wakeup & Reset, 111 Wakeup & Reset

	loopcnt = ( mode == 2 ) ? 2 : 1 ;
	busmode = SFLASH_BUS_444;

	while(loopcnt > 0) {
		if( loopcnt == 1 ){
			busmode = SFLASH_BUS_111;
		}

		sflash->accmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(busmode);

		// WakeUp
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDPD, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);

		JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
		if( sfdp != NULL && sfdp->suppdpd != 0 ){
			sflash_jedec_atomic( sfdp->xdpdop, sflash, phase);
		}else{
			sflash_jedec_atomic( (*cmditem), sflash, phase);
		}
		JEDEC_SET_SLEEP(sflash, 1,5); // 48usec

		if( mode == 0 ){
			break;
		}

		// Reset
		if( sfdp == NULL ){
			/* non-jedec : spansion */
			JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
			sflash_jedec_atomic( 0xf0, sflash, phase);
			JEDEC_SET_SLEEP(sflash, 1,5); // 48usec

			JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
			sflash_jedec_atomic( 0xff, sflash, phase);
			JEDEC_SET_SLEEP(sflash, 1,5); // 48usec
		}

		/* jedec standard */
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RSTEN, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);

		JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
		sflash_jedec_atomic( (*cmditem), sflash, phase);
		JEDEC_SET_SLEEP(sflash, 1,5); // 48usec

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RESET, sflash);
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);

		JEDEC_PRESET_SLEEP(sflash, 1,5); // 48usec
		sflash_jedec_atomic( (*cmditem), sflash, phase);
		JEDEC_SET_SLEEP(sflash, 1,5); // 48usec
		// VSIM timing violation (FPGA margin)

		loopcnt--;
	}

	return TRUE;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static UINT32 sflash_get_read_opcode(SFLASH_HANDLER_TYPE *sflash
			, SFLASH_SFDP_TYPE *sfdp
			, SFLASH_EXTRA_TYPE *extra
			, UINT8 *opcode
			, UINT32 *axcode) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(extra);

	UINT8	dummy = 0, mode = 0, modebit;
	UINT32	phase;

	*opcode = 0x00;
	axcode[0] = 0x00;

	if(sfdp == NULL ){
		sflash->accmode = SFLASH_BUS_NOGRP(sflash->accmode)|SFLASH_BUS_111;
	}

	modebit = 0;

	switch(SFLASH_BUS_GRP(sflash->accmode)){
		case SFLASH_BUS_444:
		case SFLASH_BUS_044:
		case SFLASH_BUS_004:
			if( sfdp->frd444 == 1 ){
				*opcode = sfdp->fr444op;
				dummy   = sfdp->frwt444;
				mode    = sfdp->fr444md;
				modebit = mode << 2;

				if( sfdp != NULL && sfdp->sup044 == 1 ){
					if( ((sfdp->ent044) & 0x01) != 0 ){
						axcode[0] = 0xA5;
					}else if( ((sfdp->ent044) & 0x04) != 0 ){
						axcode[0] = 0xA0;
					}else{
						axcode[0] = 0x00; // disable
					}
				}
			}
			break;
		case SFLASH_BUS_144:
			if( sfdp->frd144 == 1 ){
				*opcode = sfdp->fr144op;
				dummy   = sfdp->frwt144;
				mode    = sfdp->fr144md;
				modebit = mode << 2;

				if( sfdp != NULL && sfdp->sup044 == 1 ){
					if( ((sfdp->ent044) & 0x01) != 0 ){
						axcode[0] = 0xA5;
					}else if( ((sfdp->ent044) & 0x04) != 0 ){
						axcode[0] = 0xA0;
					}else{
						axcode[0] = 0x00; // disable
					}
				}
			}
			break;
		case SFLASH_BUS_114:
		case SFLASH_BUS_014:
			if( sfdp->frd114 == 1 ){
				*opcode = sfdp->fr114op;
				dummy   = sfdp->frwt114;
				mode    = sfdp->fr114md;
				modebit = mode << 2;
			}
			break;
		case SFLASH_BUS_222:
		case SFLASH_BUS_022:
		case SFLASH_BUS_002:
			if( sfdp->frd222 == 1 ){
				*opcode = sfdp->fr222op;
				dummy   = sfdp->frwt222;
				mode    = sfdp->fr222md;
				modebit = mode << 1;
			}
			break;
		case SFLASH_BUS_122:
			if( sfdp->frd122 == 1 ){
				*opcode = sfdp->fr122op;
				dummy   = sfdp->frwt122;
				mode    = sfdp->fr122md;
				modebit = mode << 1;
			}
			break;
		case SFLASH_BUS_112:
		case SFLASH_BUS_012:
			if( sfdp->frd112 == 1 ){
				*opcode = sfdp->fr112op;
				dummy   = sfdp->frwt112;
				mode    = sfdp->fr112md;
				modebit = mode << 1;
			}
			break;
		default: /*  SFLASH_BUS_111 */
			{
				SFLASH_CMD_TYPE *cmditem = sflash_jedec_get_cmd(SFLASH_CMD_FSRD, sflash);

				if( *cmditem == 0x00 ){
					*opcode = 0x03; // READ
					dummy   = 0;
					mode    = 0;
				}else{
					*opcode = *cmditem; // Fast READ
					dummy   = 8;
					mode    = 0;
				}
			}
			break;
	}

	phase = DARWMDCADD(0,0, 1,3, 1,0, modebit,(dummy+mode), 0, 0, 0, 0);
	phase = sflash_jedec_bus_update(sflash, phase);
	phase = BUS_CADD_ON_UPDATE(phase, 0x1111, 1, 1, ((mode==0)?0:1), 1); // CMD-ADDR-DUMMY-DATA	//@suppress("Suggested parenthesis around expression")

	SFLASH_DBG_INFO("%s (op %x, phase %x, dmmy %d, mod %d, mbit %d)"
			, __func__, *opcode, phase, dummy, mode, modebit );

	axcode[1] = *opcode;

	return phase;
}

/******************************************************************************
 *  sflash_unknown_read ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_read(SFLASH_HANDLER_TYPE *sflash, UINT32 addr, VOID *data, UINT32 len) CODE_SEC_SFLASH
{
	SFLASH_DEV_TYPE		*device;
	SFLASH_BUS_TYPE		*bus;
	SFLASH_SFDP_TYPE 	*sfdp;
	SFLASH_EXTRA_TYPE	*extra;
	UINT32			phase, maxrdsiz, axmode[2];
	int			status;
	UINT8			opcode;
	UINT32			axcode;

	device = (SFLASH_DEV_TYPE *)sflash->device;
	bus = (SFLASH_BUS_TYPE *)sflash->bus;

	SFLASH_DBG_NONE("%s (%x,%p)", __func__, addr, data );
	SFLASH_DBG_BASE("%s - start BUS %x", __func__, sflash->accmode);

	sflash_unknown_setspeed(sflash, 2); /* fast */

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
	extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;

	// WARN : unused field, unuse150 in JESD215B
	if( sfdp != NULL && (sfdp->unuse150 >0 && sfdp->unuse150 < 255) ){
		maxrdsiz = (sfdp->unuse150) * KBYTE ;
		bus->ioctl( sflash, SFLASH_BUS_SET_MAXRDSIZE, &maxrdsiz );
	}
	bus->ioctl( sflash, SFLASH_BUS_GET_MAXRDSIZE, &maxrdsiz );

	phase = sflash_get_read_opcode(sflash, sfdp, extra, &opcode, axmode);

	axcode = axmode[0]; // AXh

	// Learn XIP mode
	switch(sflash->xipmode){
	case	1: 	axmode[0] = 0x00; 			break;	// Normal-Read
	case 	2:						break;	// XIP-Read
	default	 : 	axmode[0] = 0x00; axmode[1] = 0x00;	break; // None
	}

	SFLASH_DBG_INFO("%s - RD (%x, AX:%x), %02x", __func__, sflash->accmode, opcode, axcode);

	status = TRUE;


	{
		UINT32  bstlen, rmdlen;
		UINT8	*p_data8;

		/* page index alignment for sequential read op. */
		if( (addr % device->page) != 0 ){
			bstlen = device->page - (addr % device->page);
		}else{
			bstlen = maxrdsiz;
		}

		rmdlen = len;
		p_data8 = (UINT8 *)data;

		// TODO: if( p_data8 == NULL) then run CRC Check mode

		bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &phase );

		bus->ioctl( sflash, SFLASH_BUS_SET_XIPMODE, axmode );

		do{
			if( bstlen > rmdlen ){
				bstlen = rmdlen;
			}

			rmdlen -= bstlen ;

			if( rmdlen ==  0 ){
				// Disable AXh
				axcode = 0x00;
			}

			SFLASH_DBG_NONE("%s - read %x/%x, len %d, bst %d, rmd %d"
					, __func__, opcode, axcode, len, bstlen, rmdlen );

			status = bus->transmit( sflash, (UINT32)opcode
						, addr, axcode
						, NULL, 0
						, p_data8, bstlen );

			addr   += bstlen ;
			if( p_data8 != NULL ){
				p_data8 = &(p_data8[bstlen]);
			}
			bstlen = maxrdsiz ;

			// Enable AXh
			if( axcode != 0x00 ){
				UINT32 axphase;

				axphase = BUS_CADD_ON_UPDATE(phase, 0x1101, 0, 1, 0, 1); // ADDR-DUMMY-DATA	//@suppress("Suggested parenthesis around expression")

				bus->ioctl(sflash, SFLASH_BUS_SET_BUSCONTROL, &axphase );
			}

		}while(rmdlen > 0);
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static UINT32 sflash_get_write_opcode(SFLASH_HANDLER_TYPE *sflash
			, SFLASH_SFDP_TYPE *sfdp, UINT8 *opcode, UINT32 *pagetime ) CODE_SEC_SFLASH
{
	UINT32 phase, busmode, scaler, laccmode;
	SFLASH_CMD_TYPE *wropcode;

	phase =0;

	laccmode = SFLASH_BUS_GRP(sflash->accmode);

sflash_get_write_opcode_loop:
	switch(laccmode){
		case SFLASH_BUS_444:
		case SFLASH_BUS_044:
		case SFLASH_BUS_004:
			if( sfdp->addbyt != 0 // 4 Byte
				&& (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR){
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_4PP, sflash);
			}else{
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_PP, sflash);
			}
			if( *wropcode != 0x00 ){
				busmode = (SFLASH_BUS_444);
				break;
			}
			laccmode = SFLASH_BUS_144;
			goto sflash_get_write_opcode_loop;
		case SFLASH_BUS_144:
			if( sfdp->addbyt != 0 // 4 Byte
				&& (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR){
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_4QPPA, sflash);
			}else{
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_QPPA, sflash);
			}
			if( *wropcode != 0x00 ){
				busmode = (SFLASH_BUS_144);
				break;
			}
			laccmode = SFLASH_BUS_114;
			goto sflash_get_write_opcode_loop;
		case SFLASH_BUS_114:
		case SFLASH_BUS_014:
			if( sfdp->addbyt != 0 // 4 Byte
				&& (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR){
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_4QPP, sflash);
			}else{
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_QPP, sflash);
			}
			if( *wropcode != 0x00 ){
				busmode = (SFLASH_BUS_114);
				break;
			}
			laccmode = SFLASH_BUS_111;
			goto sflash_get_write_opcode_loop;
		default: /*  SFLASH_BUS_111 */
			if( sfdp->addbyt != 0 // 4 Byte
				&& (sflash->accmode & SFLASH_BUS_4BADDR) == SFLASH_BUS_4BADDR){
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_4PP, sflash);
			}else{
				wropcode = sflash_jedec_get_cmd(SFLASH_CMD_PP, sflash);
			}
			busmode = (SFLASH_BUS_111);
			break;
	}

#ifdef	SUPPORT_JERRY_SIM_MODEL
	*pagetime = 2;	// disable
#else	//SUPPORT_JERRY_SIM_MODEL

	*pagetime = ((sfdp->pprgtim & 0x1F) + 1); // usec
	scaler = (2 * (sfdp->prgcnt + 1));

	if( scaler < 8 ){
		scaler = 1;
	}else if( scaler < 64 ){
		scaler = 2;
	}else if( scaler < 512 ){
		scaler = 3;
	}else{
		scaler = 4;
	}

	if( (sfdp->pprgtim & 0x20) == 0x20 ){
		*pagetime = SFLASH_UTIME((2+scaler), *pagetime); // x64
	}else{
		*pagetime = SFLASH_UTIME((1+scaler), *pagetime); // x8
	}

#endif	//SUPPORT_JERRY_SIM_MODEL

	phase = DARWMDCADD(0,0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0);

	sflash->accmode = SFLASH_BUS_NOGRP(sflash->accmode)|SFLASH_BUS_GRP(busmode);
	phase = sflash_jedec_bus_update(sflash, phase);
	phase = BUS_CADD_ON_UPDATE(phase, 0x1111, 1, 1, 0, 1); // CMD-ADDR-DATA	//@suppress("Suggested parenthesis around expression")

	*opcode = *wropcode;

	SFLASH_DBG_INFO("%s (op %x, phase %x, pagetime %x)"
			, __func__, *opcode, phase, *pagetime);

	return phase;
}

/******************************************************************************
 *  sflash_unknown_write ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_write(SFLASH_HANDLER_TYPE *sflash, UINT32 addr, VOID *data, UINT32 len) CODE_SEC_SFLASH
{
	UINT8		opcode;
	SFLASH_CMD_TYPE	*cmditem;
	SFLASH_SFDP_TYPE *sfdp;
	UINT32		phase, busmode, bkupbus, pagetime;
	int		status ;

	SFLASH_DBG_BASE("%s (%x)", __func__, addr );

	bkupbus = sflash->accmode;

	sflash_unknown_setspeed(sflash, 1); /* slow */

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
	// PP bus mode
	//sflash_unknown_setspeed(sflash, 2); /* fast */
	phase = sflash_get_write_opcode(sflash, sfdp, &opcode, &pagetime);

	// Change BUS

	SFLASH_DBG_INFO("%s - WREN", __func__);

	sflash_unknown_set_wren(sflash);

	SFLASH_DBG_INFO("%s - PP (%x), %02x", __func__, sflash->accmode, opcode);
	SFLASH_DBG_BASE("%s: phase %x (%x)", __func__, phase);

	if( sflash->psustime == 0 ){
		UINT32 trim;

		trim = SFLASH_UTIME_UNIT(pagetime) ;
		trim = trim >> 1;
		pagetime = SFLASH_UTIME(trim, SFLASH_UTIME_COUNT(pagetime));

		JEDEC_PRESET_SLEEPTIME(sflash, pagetime);
		status = sflash_jedec_addr_write( opcode, sflash, phase, addr, data, len );
		JEDEC_SET_SLEEPTIME(sflash, pagetime);
	}else{
		JEDEC_PRESET_SLEEP(sflash, 1, 2);
		status = sflash_jedec_addr_write( opcode, sflash, phase, addr, data, len );
		JEDEC_SET_SLEEP(sflash, 1, 2);
	}

	// rdsr bus mode
	//sflash_unknown_setspeed(sflash, 2); /* fast */

	SFLASH_DBG_INFO("%s - RDSR1", __func__);

	// SFLASH_CMD_RDSR1
	cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);

	switch(SFLASH_BUS_GRP(sflash->accmode)){
		case	SFLASH_BUS_444:
		case	SFLASH_BUS_044:
		case	SFLASH_BUS_004:
			busmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(SFLASH_BUS_444);
			break;
		default:
			busmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(SFLASH_BUS_111);
			break;
	}

	sflash->accmode = busmode;

	if( sflash->psustime == 0 ){
		status = sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);

		SFLASH_DBG_INFO("%s - WRDI", __func__);
		// SFLASH_CMD_WRDI
		sflash_unknown_set_wrdi(sflash);

		// SFLASH_CMD_CLSR
		if(status == FALSE){
			SFLASH_DBG_INFO("%s - CLSR", __func__);
			// SFLASH_CMD_CLSR
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_CLSR, sflash);
			phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
			sflash_jedec_atomic((*cmditem), sflash, phase);
		}
	}
	else
	{
		status = sflash_jedec_atomic_check_iter( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP, 1);
	}

	// Change BUS
	SFLASH_DBG_BASE("%s - change BUS %x (%x)"
			, __func__, sflash->accmode, bkupbus);

	sflash->accmode = bkupbus;

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static UINT32 sflash_get_erase_opcode(SFLASH_HANDLER_TYPE *sflash
			, SFLASH_SFDP_TYPE *sfdp
			, UINT32 secsiz
			, UINT8 *opcode, UINT32 *erasemsec) CODE_SEC_SFLASH
{
	UINT8	adrbyte;
	UINT32	phase, erastime;


	switch(sfdp->addbyt){
		case	2: // 4 Byte address only
			adrbyte = 4;
			break;
		case	1: // 3 Byte or 4 Byte
			if( (sflash->accmode & SFLASH_BUS_4BADDR)
				== SFLASH_BUS_4BADDR ){
				adrbyte = 4;
			}else{
				adrbyte = 3;
			}
			break;
		default: // 3 Byte only
			adrbyte = 3;
			break;
	}

	phase = DARWMDCADD(0,0, 1, adrbyte, 0, 1, 0, 0, 0, 0, 0, 0);
	phase = sflash_jedec_bus_update( sflash, phase);

	if( secsiz == 0xFFFFFFFF ){
		// CE
		SFLASH_CMD_TYPE	*cmditem;

		phase = BUS_CADD_ON_UPDATE(phase, 0x1111, 1, 0, 0, 0); // CMD	//@suppress("Suggested parenthesis around expression")
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_CE, sflash);
		*opcode = (UINT8)(*cmditem);
		erastime = sfdp->cerstim;
	}else
	if( secsiz == (UINT32)(1<<(sfdp->ers1siz)) ){
		*opcode = sfdp->ers1op;
		erastime = sfdp->ers1tim;
	}else
	if( secsiz == (UINT32)(1<<(sfdp->ers2siz)) ){
		*opcode = sfdp->ers2op;
		erastime = sfdp->ers2tim;
	}else
	if( secsiz == (UINT32)(1<<(sfdp->ers3siz)) ){
		*opcode = sfdp->ers3op;
		erastime = sfdp->ers3tim;
	}else
	if( secsiz == (UINT32)(1<<(sfdp->ers4siz)) ){
		*opcode = sfdp->ers4op;
		erastime = sfdp->ers4tim;
	}else{
		*opcode = sfdp->ers_op;
		erastime = 0x01; /* 1 msec */
	}

#ifdef	SUPPORT_JERRY_SIM_MODEL
	*erasemsec = 1;	// disable
#else	//SUPPORT_JERRY_SIM_MODEL

	if( (2*(sfdp->erscnt + 1)) < 8 ){
		*erasemsec = 0;
	}else if( (2*(sfdp->erscnt + 1)) < 64 ){
		*erasemsec = 1;
	}else if( (2*(sfdp->erscnt + 1)) < 512 ){
		*erasemsec = 2;
	}else{
		*erasemsec = 3;
	}

	if( secsiz == 0xFFFFFFFF ){
		switch((erastime >> 5) & 0x03){
		case	0:	*erasemsec = 5 + *erasemsec; break; //  16 ms, (REG 32)
		case	1:	*erasemsec = 6 + *erasemsec; break; // 256 ms, (REG 262)
		case	2:	*erasemsec = 7 + *erasemsec; break; //   4  s, (REG 2)
		default:	*erasemsec = 8 + *erasemsec; break; //  64  s, (REG 16)
		}
		*erasemsec = (*erasemsec > 7) ? 7 : *erasemsec; // XFC limit
		*erasemsec = SFLASH_UTIME((*erasemsec), (erastime & 0x1F));
	}else{
		switch((erastime >> 5) & 0x03){
		case	0:	*erasemsec = 4 + *erasemsec; break; //   1 ms, (REG 4)
		case	1:	*erasemsec = 5 + *erasemsec; break; //  16 ms, (REG 32)
		case	2:	*erasemsec = 6 + *erasemsec; break; // 128 ms, (REG 262)
		default:	*erasemsec = 7 + *erasemsec; break; //   1  s, (REG 2)
		}
		*erasemsec = (*erasemsec > 7) ? 7 : *erasemsec; // XFC limit
		*erasemsec = SFLASH_UTIME((*erasemsec), (erastime & 0x1F));
	}
#endif	//SUPPORT_JERRY_SIM_MODEL

	SFLASH_DBG_INFO("%s (op %x, phase %x, tim %x)", __func__, *opcode, phase, *erasemsec );

	return phase;
}

/******************************************************************************
 *  sflash_unknown_erase ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_erase(SFLASH_HANDLER_TYPE *sflash
			, UINT32 addr, UINT32 num) CODE_SEC_SFLASH
{
	SFLASH_CMD_TYPE	*cmditem;
	SFLASH_SFDP_TYPE *sfdp;
	UINT8		opcode;
	UINT32		phase, bkupbus, erasetime;
	int		status, itercnt ;

	
	SFLASH_DBG_INFO("%s (%x)", __func__, addr );

	bkupbus = sflash->accmode;

	switch(SFLASH_BUS_GRP(sflash->accmode))
	{
		case	SFLASH_BUS_144:
		case	SFLASH_BUS_114:
		case	SFLASH_BUS_014:
		case	SFLASH_BUS_122:
		case	SFLASH_BUS_112:
		case	SFLASH_BUS_012:
		case	SFLASH_BUS_111:
			sflash->accmode = SFLASH_BUS_NOGRP(sflash->accmode)|SFLASH_BUS_111;

			SFLASH_DBG_INFO("%s - update BUS %x (%x)"
					, __func__, sflash->accmode, bkupbus);
			break;
		default:
			break;
	}

	sflash_unknown_setspeed(sflash, 1); /* slow */

	SFLASH_DBG_INFO("%s - WREN", __func__);

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	sflash_unknown_set_wren(sflash);

	SFLASH_DBG_INFO("%s - P4E", __func__);

	phase = sflash_get_erase_opcode(sflash, sfdp, num, &opcode, &erasetime);

	if( sflash->esustime == 0 )
	{
		UINT32 trim;

		trim = SFLASH_UTIME_UNIT(erasetime) ;
		trim = trim >> 2; // fast erase @ normal mode
		trim = (trim > 0)? trim : 1;
		erasetime = SFLASH_UTIME(trim, SFLASH_UTIME_COUNT(erasetime));

		JEDEC_PRESET_SLEEPTIME(sflash, erasetime);
		if( num == 0xFFFFFFFF )
		{
			sflash_jedec_atomic( opcode, sflash, phase);
		}
		else
		{
			sflash_jedec_addr( opcode, sflash, phase, addr );
		}
		JEDEC_SET_SLEEPTIME(sflash, erasetime);
	}
	else
	{
		JEDEC_PRESET_SLEEP(sflash, 0, 2);
		if( num == 0xFFFFFFFF )
		{
			sflash_jedec_atomic( opcode, sflash, phase);
		}
		else
		{
			sflash_jedec_addr( opcode, sflash, phase, addr );
		}
		JEDEC_SET_SLEEP(sflash, 0, 2);
	}

	SFLASH_DBG_INFO("%s - RDSR1", __func__);
	// SFLASH_CMD_RDSR1
	cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
	phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);

	if( sflash->esustime == 0 )
	{
		if( num == 0xFFFFFFFF ){
			itercnt = 0xFFFF;
		}else{
			itercnt = 1;
		}

		do{
			status = sflash_jedec_atomic_check_iter((*cmditem), sflash, 
								phase, SR1_WIP, SR1_WIP, (SFLASH_MAX_LOOP<<1));
			itercnt--;
		}while((status == FALSE) && (itercnt > 0));	

		SFLASH_DBG_INFO("%s - nosus", __func__);
		// SFLASH_CMD_WRDI
		sflash_unknown_set_wrdi(sflash);

		// SFLASH_CMD_CLSR
		if(status == FALSE)
		{
			// SFLASH_CMD_CLSR
			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_CLSR, sflash);
			if( (*cmditem) != 0x00 )
			{
				SFLASH_DBG_INFO("%s - CLSR", __func__);
				phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic((*cmditem), sflash, phase);
			}
		}
	}
	else
	{
		SFLASH_DBG_INFO("%s - suspend", __func__);
		
		status = sflash_jedec_atomic_check_iter((*cmditem), sflash,
								phase, SR1_WIP, SR1_WIP, 1);
	}

	SFLASH_DBG_INFO("%s - restore BUS %x (%x)"
			, __func__, sflash->accmode, bkupbus);
	sflash->accmode = bkupbus;

	return status;
}

/******************************************************************************
 *  sflash_unknown_lock ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_lock(SFLASH_HANDLER_TYPE *sflash , UINT32 addr, UINT32 num)
{
	DA16X_UNUSED_ARG(num);

	SFLASH_EXTRA_TYPE *extra;
	SFLASH_CMD_TYPE	*cmditem;
	UINT32	bkupmode = 0;
	int	status;

	extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;

	if( extra == NULL
		|| (extra->dyblock == 0 && extra->ppblock == 0 && extra->blkprot == 0 ) ) {
		return FALSE;
	}

	status = FALSE;

	SFLASH_DBG_BASE("%s", __func__ );

	if( extra->lck4addr == TRUE ){
		bkupmode = sflash->accmode;
		sflash->accmode = sflash->accmode | SFLASH_BUS_4BADDR;
	}

	sflash_unknown_setspeed(sflash, 1); /* slow */

	sflash_unknown_set_wren(sflash);

	if( extra->dyblock == 1 ){
		UINT32			phase;
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 0,1, 0,0, 0,0,0,0);
		sflash_jedec_addr_write( extra->dybwr_op, sflash, phase, addr, &(extra->dyblock_cod), 1);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);

		phase = DARWMDCADD(0, 0,1,4, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_addr_read( extra->dybrd_op, sflash, phase, addr, rxdata, 1);

		if( rxdata[0] == extra->dyblock_cod ){
			status = TRUE;
		}
	}
	else if( extra->ppblock == 1 ){
		UINT32			phase;
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 0,1, 0,0, 0,0,0,0);
		sflash_jedec_addr( extra->ppblock_op, sflash, phase, addr);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( (*cmditem), sflash, phase, rxdata, 1);

		status = ((rxdata[0] & SR1_WEL) == 0) ? TRUE : FALSE;
	}

	sflash_unknown_set_wrdi(sflash);

	if( extra->lck4addr == TRUE ){
		sflash->accmode = bkupmode;
	}

	return status;

}

/******************************************************************************
 *  sflash_unknown_unlock ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_unlock(SFLASH_HANDLER_TYPE *sflash , UINT32 addr, UINT32 num)
{
	DA16X_UNUSED_ARG(num);

	SFLASH_EXTRA_TYPE *extra;
	SFLASH_CMD_TYPE	  *cmditem;
	UINT32	bkupmode = 0;
	int	status, ulckiter;
	UINT32	phase;

	extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;

	if( extra == NULL
		|| (extra->dyblock == 0 && extra->ppblock == 0 && extra->blkprot == 0 ) ) {
		return FALSE;
	}

	status = FALSE;

	SFLASH_DBG_BASE("%s", __func__ );

	if( extra->lck4addr == TRUE ){
		bkupmode = sflash->accmode;
		sflash->accmode = sflash->accmode | SFLASH_BUS_4BADDR;
	}

	sflash_unknown_setspeed(sflash, 1); /* slow */

	sflash_unknown_set_wren(sflash);

	if( extra->blkprot == 1 ){
		UINT8	txdata[4];

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( (*cmditem), sflash, phase, txdata, 1);

		if( (txdata[0] & extra->bpmask) != 0 ){
			//==================================
			// method to unprotect BP
			//==================================
			txdata[0] = txdata[0] & (~(extra->bpmask));
			txdata[0] = txdata[0] | (extra->bpunlck_cod & extra->bpmask);

			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WRR, sflash);
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			sflash_jedec_write( (*cmditem), sflash, phase, txdata, 1);

			cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			status = sflash_jedec_atomic_check( (*cmditem), sflash
							, phase, SR1_WIP, SR1_WIP);

			sflash_unknown_set_wren(sflash);
		}
	}

	switch(extra->wpsel){
		case	1: /* macronyx */
			for( ulckiter = SFLASH_MAX_LOOP; ulckiter > 0; ulckiter--)
			{
				UINT8	rxdata[4];

				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_ASPRD, sflash);
				phase = DARWMDCADD(0, 0,1,4, 1,0, 0,0, 0,0,0,0);
				sflash_jedec_read( (*cmditem), sflash, phase, rxdata, 1);

				SFLASH_DBG_INFO("SER:%08x", rxdata[0]);

				if( (rxdata[0] & 0x80) == 0x80 ){
					status = TRUE;
					break;
				}

				cmditem = sflash_jedec_get_cmd(SFLASH_CMD_WPSEL, sflash);
				phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
				sflash_jedec_atomic( (*cmditem), sflash, phase);

			}

			sflash_unknown_set_wren(sflash);
			break;
		default:
			break;
	}

	if( extra->dyblock == 1 ){
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 0,1, 0,0, 0,0,0,0);
		rxdata[0] = extra->dybunlock_cod;
		sflash_jedec_addr_write( extra->dybwr_op, sflash, phase, addr, rxdata, 1);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);

		phase = DARWMDCADD(0, 0,1,4, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_addr_read( extra->dybrd_op, sflash, phase, addr, rxdata, 1);

		if( rxdata[0] == extra->dybunlock_cod ){
			status = TRUE;
		}
	}
	else if( extra->ppblock == 1 ){
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 0,1, 0,0, 0,0,0,0);
		sflash_jedec_addr( extra->ppbunlock_op, sflash, phase, addr);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic_check( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP);

		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);
		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_read( (*cmditem), sflash, phase, rxdata, 1);

		status = ((rxdata[0] & SR1_WEL) == 0) ? TRUE : FALSE;
	}

	sflash_unknown_set_wrdi(sflash);

	if( extra->lck4addr == TRUE ){
		sflash->accmode = bkupmode;
	}

	return status;
}

/******************************************************************************
 *  sflash_unknown_verify ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_verify(SFLASH_HANDLER_TYPE *sflash , UINT32 addr, UINT32 num)
{
	DA16X_UNUSED_ARG(num);

	SFLASH_EXTRA_TYPE *extra;
	UINT32	bkupmode = 0;
	int	status;

	if( sflash->jedec.extra == NULL ) {
		return FALSE;
	}

	extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;
	status = FALSE;

	SFLASH_DBG_INFO("%s", __func__ );

	if( extra->lck4addr == TRUE ){
		bkupmode = sflash->accmode;
		sflash->accmode = sflash->accmode | SFLASH_BUS_4BADDR;
	}

	sflash_unknown_setspeed(sflash, 2); /* fast */

	if( extra->dyblock == 1 ){
		UINT32			phase;
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_addr_read( extra->dybrd_op, sflash, phase, addr, rxdata, 1);

		if( rxdata[0] == extra->dyblock_cod ){
			status = TRUE;
		}
	}
	else if( extra->ppblock == 1 ){
		UINT32			phase;
		UINT8			rxdata[4];

		phase = DARWMDCADD(0, 0,1,4, 1,0, 0,0, 0,0,0,0);
		sflash_jedec_addr_read(extra->ppbrd_op, sflash, phase, addr, rxdata, 1);

		status = (rxdata[0] == extra->ppblock_cod) ? TRUE : FALSE;
	}

	if( extra->lck4addr == TRUE ){
		sflash->accmode = bkupmode;
	}

	return status;
}

/******************************************************************************
 *  sflash_unknown_powerdown ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_powerdown(SFLASH_HANDLER_TYPE *sflash, UINT32 pwrmode, UINT32 *data)  CODE_SEC_SFLASH
{
	SFLASH_SFDP_TYPE  *sfdp;
	UINT32	pdtime, phase;

	SFLASH_DBG_NONE("%s: pwrmode = %d", __func__, pwrmode);

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	if( sfdp == NULL || sfdp->suppdpd == 0 ){
		*data = 0;
		return FALSE;
	}

#ifdef	SUPPORT_FLASH_CACHE
	if(pwrmode == TRUE ){
		// Skip
		if( data != NULL ){
			*data = 0;
		}
		return TRUE;
	}
#endif	//SUPPORT_FLASH_CACHE

	sflash_unknown_setspeed(sflash, 1); /* normal */

	pdtime = ((sfdp->xdpddly&0x1f) + 1);
	switch((sfdp->xdpddly&0x60)){
	case	0x20:	pdtime = SFLASH_UTIME(3, (2*pdtime)); break;
	case	0x40:	pdtime = SFLASH_UTIME(4, (2*pdtime)); break;
	case	0x60:	pdtime = SFLASH_UTIME(5, (2*pdtime)); break;
	default:	pdtime = SFLASH_UTIME(2, (2*pdtime)); break;
	}

	if( pdtime < 1000 ){
		JEDEC_PRESET_SLEEPTIME(sflash, pdtime);
	}

	if(pwrmode == TRUE ){
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( sfdp->edpdop, sflash, phase);
	}else{
		phase = DARWMDCADD(0, 0,1,3, 0,0, 0,0, 0,0,0,0);
		sflash_jedec_atomic( sfdp->xdpdop, sflash, phase);
	}

	if( pdtime < 1000 ){
		JEDEC_PRESET_SLEEPTIME(sflash, pdtime);
		pdtime = 0;
	}else{
		pdtime = SFLASH_CONV_UTIME(pdtime) / 1000; //msec
	}

	if( data != NULL ){
		*data = pdtime;
	}

	return TRUE;
}

/******************************************************************************
 *  sflash_unknown_vendor ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_vendor(SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data)
{
	SFLASH_BUS_TYPE		*bus;
	int			status;

	bus = (SFLASH_BUS_TYPE *)sflash->bus;

	SFLASH_DBG_NONE("%s (%d)", __func__ , cmd);
	status = TRUE;

	switch(cmd){
	case SFLASH_DEV_VENDOR:
		{
			UINT8  *rxdata, *txdata;
			UINT32 txlen, rxlen, phase;

			txdata = (UINT8 *)(  ((UINT32 *)data)[0] );
			txlen  = (  ((UINT32 *)data)[1] );
			rxdata = (UINT8 *)(  ((UINT32 *)data)[2] );
			rxlen  = (  ((UINT32 *)data)[3] );

			if( rxlen == 0 ){
				phase = DARWMDCADD(0,0, 1, 3, 0,1, 0, 0, 0, 0, 0, 0);
			}else{
				phase = DARWMDCADD(0,0, 1, 3, 1,0, 0, 0, 0, 0, 0, 0);
			}

			phase = sflash_jedec_bus_update( sflash, phase);

			sflash_unknown_setspeed(sflash, 1); /* slow */

			bus->ioctl( sflash, SFLASH_BUS_SET_BUSCONTROL, &(phase) );

			// Legacy Mode
			bus->transmit( sflash, 0, 0, 0, txdata, txlen, rxdata, rxlen );

		}
		break;
	default:
		status = FALSE;
		break;
	}

	return status;
}

/******************************************************************************
 *  sflash_unknown_suspend ( )
 *
 *  Purpose :   SFLASH Unknown Device
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_unknown_suspend(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 mode, UINT32 secsiz)
{
	DA16X_UNUSED_ARG(addr);

	SFLASH_SFDP_TYPE  *sfdp;
	UINT8	opcode;
	UINT32	runtime, restime, phase;
	int 	status;

	SFLASH_DBG_NONE("%s: cmd = %d", __func__, mode);

	sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;

	if( sfdp == NULL || sfdp->srsupp == 1 ){
		return FALSE;
	}

	status = TRUE;

	switch((SFLASH_SUSCMD_TYPE)mode){
	case SUS_GET_PROGRAM:
		sflash_get_write_opcode(sflash, sfdp, &opcode, &runtime);
		runtime = SFLASH_CONV_UTIME(runtime);
		restime = sfdp->prtsi + 1;
		restime	= SFLASH_CONV_UTIME(SFLASH_UTIME(2, restime)); // x64
		restime = (restime == 0)? 1 : restime;
		return ((int)((runtime/restime) + 1));

	case SUS_GET_ERASE:
		sflash_get_erase_opcode(sflash, sfdp, secsiz, &opcode, &runtime);
		runtime = SFLASH_CONV_UTIME(runtime);
		restime = sfdp->ertsi + 1;
		restime	= SFLASH_CONV_UTIME(SFLASH_UTIME(2, restime)); // x64
		restime = (restime == 0)? 1 : restime;
		return ((int)((runtime/restime) + 1));

	case SUS_RUN_PROGRAM:
		opcode = sfdp->prgsop;
		if( (opcode == 0x00) || (opcode == 0xFF) ){
			opcode = sfdp->spndop;
		}

		runtime = sfdp->sipml;
		switch((runtime & 0x60)){
		case	0x40:
			runtime = (runtime & 0x1f) + 1 + 8;
			runtime	= SFLASH_UTIME(1, runtime); // x8
			break;
		case	0x60:
			runtime = (runtime & 0x1f) + 1 + 1;
			runtime	= SFLASH_UTIME(2, runtime); // x64
			break;
		default:
			runtime = (runtime & 0x1f) + 1 + 64;
			runtime	= SFLASH_UTIME(0, runtime); // x1
			break;
		}

		if( (secsiz != 0) && (SFLASH_CONV_UTIME(secsiz) > SFLASH_CONV_UTIME(runtime)) ){
			runtime = SFLASH_UTIME(SFLASH_UTIME_UNIT(secsiz)
					,SFLASH_UTIME_COUNT(secsiz) );
		}
		break;
	case SUS_RUN_ERASE:
		opcode = sfdp->spndop;

		runtime = sfdp->sieml;
		switch((runtime & 0x60)){
		case	0x40:
			runtime = (runtime & 0x1f) + 1 + 8;
			runtime	= SFLASH_UTIME(1, runtime); // x8
			break;
		case	0x60:
			runtime = (runtime & 0x1f) + 1 + 1;
			runtime	= SFLASH_UTIME(2, runtime); // x64
			break;
		default:
			runtime = (runtime & 0x1f) + 1 + 64;
			runtime	= SFLASH_UTIME(0, runtime); // x1
			break;
		}

		if( (secsiz != 0) && (SFLASH_CONV_UTIME(secsiz) > SFLASH_CONV_UTIME(runtime)) ){
			runtime = SFLASH_UTIME(SFLASH_UTIME_UNIT(secsiz)
					,SFLASH_UTIME_COUNT(secsiz) );
		}
		break;
	case RES_RUN_PROGRAM:
		opcode = sfdp->prgrop;
		if( (opcode == 0x00) || (opcode == 0xFF) ){
			opcode = sfdp->rsmeop;
		}

		runtime = sfdp->prtsi + 1 + 2;
		runtime	= SFLASH_UTIME(2, runtime); // x64

		if( (secsiz != 0) && (SFLASH_CONV_UTIME(secsiz) > SFLASH_CONV_UTIME(runtime)) ){
			runtime = SFLASH_UTIME(SFLASH_UTIME_UNIT(secsiz)
					,SFLASH_UTIME_COUNT(secsiz) );
		}
		break;
	case RES_RUN_ERASE:
		opcode = sfdp->rsmeop;

		// TEST Model
		if( sfdp->unuse120 == 1 ){
			runtime = sfdp->ertsi ;
			runtime	= SFLASH_UTIME(2, runtime); // x64
		}else{
			runtime = sfdp->ertsi ;
			runtime	= SFLASH_UTIME(3, runtime); // x512	
		}		

		if( (secsiz != 0) && (SFLASH_CONV_UTIME(secsiz) > SFLASH_CONV_UTIME(runtime)) ){
			runtime = SFLASH_UTIME(SFLASH_UTIME_UNIT(secsiz)
					,SFLASH_UTIME_COUNT(secsiz) );
		}
		break;
	default:
		opcode = 0;
		break;
	}

	if( (opcode != 0x00) && (opcode != 0xFF) ){
		SFLASH_CMD_TYPE *cmditem;
		UINT32 bkbusmode, busmode, itercnt;

		if( sfdp->unuse120 == 1 ){	
			itercnt = 1;	// 1 msec
		}else{
			itercnt = 2;	// 2 msec
		}

		phase = DARWMDCADD(0,0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0);
		phase = sflash_jedec_bus_update(sflash, phase);

		JEDEC_PRESET_SLEEPTIME(sflash, runtime);
		sflash_jedec_atomic( opcode, sflash, phase);
		JEDEC_SET_SLEEPTIME(sflash, runtime);

		// SFLASH_CMD_RDSR1
		cmditem = sflash_jedec_get_cmd(SFLASH_CMD_RDSR1, sflash);

		switch(SFLASH_BUS_GRP(sflash->accmode)){
			case	SFLASH_BUS_444:
			case	SFLASH_BUS_044:
			case	SFLASH_BUS_004:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(SFLASH_BUS_444);
				break;
			default:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)|DARWMDCADD_BUS(SFLASH_BUS_111);
				break;
		}

		bkbusmode = sflash->accmode;
		sflash->accmode = busmode;
		status = sflash_jedec_atomic_check_iter( (*cmditem), sflash
						, phase, SR1_WIP, SR1_WIP, itercnt);
		sflash->accmode = bkbusmode;

		switch((SFLASH_SUSCMD_TYPE)mode){
		case SUS_RUN_PROGRAM:
		case SUS_RUN_ERASE:
			//sflash_unknown_set_wrdi(sflash);
			break;
		case RES_RUN_PROGRAM:
		case RES_RUN_ERASE:
			break;
		default:
			break;
		}
	}

	return status;
}

#endif /*defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)*/
