/**
 ****************************************************************************************
 *
 * @file sflash_jedec.h
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


#ifndef __SFLASH_JEDEC_H__
#define __SFLASH_JEDEC_H__

#include "driver.h"
#include "sflash.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------
#define	SUPPORT_STD_JESD216B
#define	SFLASH_JESD216B_LEN	(4*16)
#define	SFLASH_JEDEC_BUFSIZ	(4+16)

typedef		struct	_sflash_jedec_sfdp	SFLASH_SFDP_TYPE;
typedef		struct	_sflash_jedec_cfi	SFLASH_CFI_TYPE;
typedef		UINT8				SFLASH_CMD_TYPE;
typedef		UINT8				SFLASH_CMDLIST_TYPE;
typedef		struct	_sflash_jedec_extra	SFLASH_EXTRA_TYPE;

//---------------------------------------------------------
//	DRIVER :: Definition
//---------------------------------------------------------

struct		_sflash_jedec_sfdp
{
	/* 1st DWORD */
	UINT8	secsiz: 2;	/* Block/Sector Erase Size */
	UINT8	wrgran: 1;	/* Write Granularity */
	UINT8	wenbit: 1;	/* WEN instruction */
	UINT8	wen_op: 1;	/* WEN OPcode */
	UINT8   unuse10: 3;

	UINT8	ers_op;		/* 4KB Erase OPcode */

	UINT8	frd112: 1;	/* 1-1-2 Fast Read */
	UINT8	addbyt: 2;	/* Addr Bytes Num: 0 - 3 , 1 - 3 or 4, 2 - 4 */
	UINT8	dtrmod: 1;	/* Double Transfer Rate */
	UINT8	frd122: 1;	/* 1-2-2 Fast Read */
	UINT8	frd144: 1;	/* 1-4-4 Fast Read */
	UINT8	frd114: 1;	/* 1-1-4 Fast Read */
	UINT8	unuse11: 1;

	UINT8	unuse12;

	/* 2nd DWORD */
	UINT32	density;	/* Flash Memory Density */

	/* 3rd DWORD */
	UINT8	frwt144: 5;	/* 1-4-4 Fast Read Dummy Cycles */
	UINT8	fr144md: 3;	/* 1-4-4 Fast Read Number of Mode Clocks */
	UINT8	fr144op;	/* 1-4-4 Fast Read OPcode */
	UINT8	frwt114: 5;	/* 1-1-4 Fast Read Dummy Cycles */
	UINT8	fr114md: 3;	/* 1-1-4 Fast Read Number of Mode Clocks */
	UINT8	fr114op;	/* 1-1-4 Fast Read OPcode */

	/* 4th DWORD */
	UINT8	frwt112: 5;	/* 1-1-2 Fast Read Dummy Cycles */
	UINT8	fr112md: 3;	/* 1-1-2 Fast Read Number of Mode Clocks */
	UINT8	fr112op;	/* 1-1-2 Fast Read OPcode */
	UINT8	frwt122: 5;	/* 1-2-2 Fast Read Dummy Cycles */
	UINT8	fr122md: 3;	/* 1-2-2 Fast Read Number of Mode Clocks */
	UINT8	fr122op;	/* 1-2-2 Fast Read OPcode */

	/* 5th DWORD */
	UINT8	frd222: 1;	/* 2-2-2 Fast Read Support */
	UINT8	unuse50: 3;
	UINT8	frd444: 1;	/* 4-4-4 Fast Read Support */
	UINT8	unuse51: 3;
	UINT8	unuse52;
	UINT16	unuse53;

	/* 6th DWORD */
	UINT16	unuse6;
	UINT8	frwt222: 5;	/* 2-2-2 Fast Read Dummy Cycles */
	UINT8	fr222md: 3;	/* 2-2-2 Read Number of Mode Clocks */
	UINT8	fr222op;	/* 2-2-2 Fast Read OPcode */

	/* 7th DWORD */
	UINT16	unuse7;
	UINT8	frwt444: 5;	/* 4-4-4 Fast Read Dummy Cycles */
	UINT8	fr444md: 3;	/* 4-4-4 Read Number of Mode Clocks */
	UINT8	fr444op;	/* 4-4-4 Fast Read OPcode */

	/* 8th DWORD */
	UINT8	ers1siz	;	/* Erase Type 1 Size */
	UINT8	ers1op	;	/* Erase Type 1 Instruction */
	UINT8	ers2siz	;	/* Erase Type 2 Size */
	UINT8	ers2op	;	/* Erase Type 2 Instruction */

	/* 9th DWORD */
	UINT8	ers3siz	;	/* Erase Type 3 Size */
	UINT8	ers3op	;	/* Erase Type 3 Instruction */
	UINT8	ers4siz	;	/* Erase Type 4 Size */
	UINT8	ers4op	;	/* Erase Type 4 Instruction */

#ifdef	SUPPORT_STD_JESD216B
	/* 10th DWORD */
	UINT32	erscnt: 4;	/* Multiplier from typ erase time to max erase time */
	UINT32	ers1tim: 7;	/* Erase Type 1 Erase, typ time */
	UINT32	ers2tim: 7;	/* Erase Type 2 Erase, typ time */
	UINT32	ers3tim: 7;	/* Erase Type 3 Erase, typ time */
	UINT32	ers4tim: 7;	/* Erase Type 4 Erase, typ time */

	/* 11th DWORD */
	UINT32	prgcnt: 4;	/* Multiplier from typ b-prog time to max b-prog time */
	UINT32	pagsiz: 4;	/* Page Size */
	UINT32	pprgtim: 6;	/* Page Program Typ Time */
	UINT32	bprgtim: 5;	/* Byte Program Typ Time */
	UINT32	bprgadd: 5;	/* Byte Program Typ Time, additional byte */
	UINT32	cerstim: 7;	/* Chip Erase, Typ Time */
	UINT32	unuse110: 1;

	/* 12th DWORD */
	UINT32	podps:4;	/* Prohibited Operations During Program Suspend */
	UINT32	podes:4;	/* Prohibited Operations During Erase Suspend */
	UINT32	unuse120:1;
	UINT32	prtsi:4;	/* Program Resume to Suspend Interval */
	UINT32	sipml:7;	/* Suspend in-progress program max latency */
	UINT32	ertsi:4;	/* Erase Resume to Suspend Interval */
	UINT32	sieml:7;	/* Suspend in-progress erase max latency */
	UINT32	srsupp:1;	/* Suspend / Resume supported */

	/* 13th DWORD */
	UINT8	prgrop;		/* Program Resume OPcode */
	UINT8	prgsop;		/* Program Suspend OPcode */
	UINT8	rsmeop;		/* Resume OPcode */
	UINT8	spndop;		/* Suspend OPcode */

	/* 14th DOWRD */
	UINT32	unuse140: 2;
	UINT32	busychk: 6;	/* Status Register Polling Device Busy */
	UINT32	xdpddly: 7;	/* Exit Deep Powerdown to next op delay */
	UINT32	xdpdop: 8;	/* Exit Deep Poewrdown OPcode */
	UINT32	edpdop: 8;	/* Enter Deep Poewrdown OPcode */
	UINT32	suppdpd: 1;	/* Deep Powerdown Supported */

	/* 15th DWORD */
	UINT32	dis444: 4;	/* 4-4-4 mode disable sequences */
	UINT32	enb444: 5; 	/* 4-4-4 mode enable sequences */
	UINT32	sup044: 1;	/* 0-4-4 mode supported */
	UINT32	ext044: 6;	/* 0-4-4 mode exit method */
	UINT32	ent044: 4;	/* 0-4-4 mode entry method */
	UINT32	qer: 3;		/* Qead Enable Requirements */
	UINT32  hrdis: 1;	/* HOLD or Reset Disable */
	UINT32	unuse150: 8;

	/* 16th DWORD */
	UINT32	sr1mod: 7;	/* Volatile or Non-Volatile Register */
	UINT32	unuse160: 1;
	UINT32	softrst: 6;	/* Soft Reset and Rescue Sequence support */
	UINT32	ext4adr: 10;	/* Exit 4 Byte Addressing */
	UINT32	ent4adr: 8;	/* Enter 4 Byte Addressing */
#endif	//SUPPORT_STD_JESD216B
};

struct		_sflash_jedec_cfi
{
	UINT8	dev_id[0x10];
	UINT8	query[3];
	UINT8	prim_id[2];
	UINT8	prim_asse[2];
	UINT8	alt_id[2];
	UINT8	alt_addr[2];
	UINT8	sys_info[12];
	UINT8	geo_info[22];
};

#define	SFLASH_GEO_SIZE		0	/* 2^n bytes */
#define	SFLASH_GEO_ADDR		1
#define	SFLASH_GEO_PAGE		3	/* 2^n bytes */
#define	SFLASH_GEO_BOOT		5	/* n of erase block regions  */
#define SFLASH_GEO_ERSSIZ	6	/* size: boot * 4 Bytes */


struct	_sflash_jedec_extra
{
	/* 1st DWORD */
	UINT8	magiccode;	/* 7 DWORDs */

	UINT8	dyblock: 1;	/* dyblock supported */
	UINT8	ppblock: 1;	/* ppblock supported */
	UINT8	lck4addr: 1;	/* Lock address byte : 3 or 4 */
	UINT8   blkprot: 1;	/* Block Protection */
	UINT8	wpsel: 4;	/* WP selection */

	UINT8	ppblock_op;	/* PPBLOCK */
	UINT8	ppbunlock_op;	/* PPBUNLOCK */

	/* 2nd DWORD */
	UINT8	ppbrd_op;	/* PPBRD */
	UINT8	ppblock_cod;	/* PPB LOCK code */
	UINT8	dybwr_op;	/* DYBWRITE */
	UINT8	dybrd_op;	/* DYBREAD */

	/* 3rd DWORD */
	UINT8	dyblock_cod;	/* DYB LOCK code */
	UINT8	dybunlock_cod;	/* DYB UNLOCK code */
	UINT8	bpmask;		/* Block Protection Mask */
	UINT8	bpunlck_cod;

	/* 4th DWORD */
	UINT8	sdr_speed;	// SPI Read
	UINT8	fsr_speed;	// Fast SPI Read
	UINT8	ddr_speed;	// Dual Read
	UINT8	qdr_speed;	// Quad Read

	/* 5th DWORD */
	UINT8	dtr_speed;	// Double Transfer
	UINT8	max_speed;
	UINT16	total_sec;	/* Total Number of Sector Group */

	/* 6th DWORD */
	UINT16	n_top4ksec;	/* Number of Top 4KB Sector Group */
	UINT16	n_32ksec;	/* Number of 32KB Sector Group */

	/* 7th DWORD */
	UINT16	n_64ksec;	/* Number of 64KB Sector Group */
	UINT16	n_bot4ksec;	/* Number of Bottom 4KB Sector Group */
};

#if	defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)

//---------------------------------------------------------
//	DRIVER :: atomic
//---------------------------------------------------------

extern SFLASH_CMD_TYPE *sflash_jedec_get_cmd(UINT32 cmd
				, SFLASH_HANDLER_TYPE *sflash);
//---------------------------------------------------------
//	DRIVER :: Common
//---------------------------------------------------------

extern int sflash_find_sfdp(SFLASH_HANDLER_TYPE *sflash, UINT8 *dump);
extern int sflash_find_cfi(SFLASH_HANDLER_TYPE *sflash, UINT8 *dump);

extern void sflash_print_sfdp(SFLASH_HANDLER_TYPE *sflash);
extern void sflash_print_cfi(SFLASH_HANDLER_TYPE *sflash);
extern int  sflash_find_sfdppage(SFLASH_HANDLER_TYPE *sflash);

//---------------------------------------------------------
//	DRIVER :: Common
//---------------------------------------------------------

extern UINT32 sflash_jedec_bus_update(SFLASH_HANDLER_TYPE *sflash
				, UINT32 force);

extern int  sflash_jedec_read(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT8 *data, UINT32 len);			

extern int  sflash_jedec_atomic(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force);

extern int  sflash_jedec_atomic_check_iter(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT8 mask, UINT8 flag, UINT32 maxloop);

extern int  sflash_jedec_atomic_check(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT8 mask, UINT8 flag);

extern int  sflash_jedec_addr_read(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT32 addr, UINT8 *data, UINT32 len);

extern int  sflash_jedec_addr(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT32 addr);

extern int  sflash_jedec_addr_write(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT32 addr, UINT8 *data, UINT32 len);

extern int  sflash_jedec_write(UINT8 cmd, SFLASH_HANDLER_TYPE *sflash
				, UINT32 force, UINT8 *data, UINT32 len);


extern void sflash_unknown_setspeed(SFLASH_HANDLER_TYPE *sflash, UINT32 mode);

extern void sflash_unknown_set_wren(SFLASH_HANDLER_TYPE *sflash);

extern void sflash_unknown_set_wrdi(SFLASH_HANDLER_TYPE *sflash);

extern	int sflash_unknown_probe(SFLASH_HANDLER_TYPE *sflash
				, VOID *anonymous, UINT8 *dump);

extern	int sflash_unknown_reset(SFLASH_HANDLER_TYPE *sflash, UINT32 mode);

extern  int sflash_unknown_busmode(SFLASH_HANDLER_TYPE *sflash
				, UINT32 busmode);

extern  int sflash_unknown_buslock(SFLASH_HANDLER_TYPE *sflash
				, UINT32 lockmode);


extern int sflash_unknown_read(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, VOID *data, UINT32 len);

extern int sflash_unknown_write(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, VOID *data, UINT32 len);

extern	int sflash_unknown_erase(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 num);


extern	int sflash_unknown_lock(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 num);

extern	int sflash_unknown_unlock(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 num);

extern	int sflash_unknown_verify(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 num);

extern	int sflash_unknown_powerdown(SFLASH_HANDLER_TYPE *sflash
				, UINT32 pwrmode, UINT32 *data);

extern  int sflash_unknown_vendor(SFLASH_HANDLER_TYPE *sflash
				, UINT32 cmd, VOID *data);

extern	int sflash_unknown_suspend(SFLASH_HANDLER_TYPE *sflash
				, UINT32 addr, UINT32 mode, UINT32 secsiz);

#endif /*defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)*/


#endif  /*__SFLASH_JEDEC_H__*/
