/**
 * \addtogroup DALayer
 * \{
 * \addtogroup SFLASH
 * \{
 * \brief Serial FLASH driver
 */

/**
 ****************************************************************************************
 *
 * @file sflash.h
 *
 * @brief Defines and macros for SFLASH driver
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

#ifndef __SFLASH_H__
#define __SFLASH_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver.h"

#define	 SUPPORT_SFLASH_DEVICE
#undef	 SUPPORT_SFLASH_LOCKINFO
#undef	 SUPPORT_SFLASH_ROMLIB

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef 	struct __sflash_device_handler__	SFLASH_HANDLER_TYPE;
typedef 	struct _jedec_				SFLASH_JEDEC_TYPE;

//---------------------------------------------------------
// Global Definition : Command
//---------------------------------------------------------

typedef 	enum	__sflash_unit__ {
	SFLASH_UNIT_0 = 0,
	SFLASH_UNIT_MAX
} SFLASH_UNIT_TYPE;

//
//	IOCTL Commands
//

typedef enum	{
	SFLASH_GET_DEVREG = 0x20,
	SFLASH_GET_SIZE,
	SFLASH_GET_INFO,

	SFLASH_SET_RESET,
	SFLASH_SET_LOCK,
	SFLASH_SET_UNLOCK,
	SFLASH_CMD_VERIFY,
	SFLASH_CMD_ERASE,

	SFLASH_SET_PPBLOCK,
	SFLASH_GET_PPBLOCK,

	SFLASH_CMD_POWERDOWN,
	SFLASH_CMD_WAKEUP,

	SFLASH_BUS_POLLMODE,
	SFLASH_BUS_INTRMODE,
	SFLASH_BUS_DMAMODE,

	SFLASH_BUS_CONTROL,
	SFLASH_BUS_INTERFACE,
	SFLASH_BUS_MAXSPEED,
	SFLASH_BUS_FORMAT,
	SFLASH_BUS_DWIDTH,

	SFLASH_SET_VENDOR,
	SFLASH_TEST_CONTROL,
	SFLASH_SET_INFO,
	SFLASH_GET_MODE,

	SFLASH_SET_MAXBURST,

	SFLASH_CMD_CHIPERASE,
	SFLASH_SET_XIPMODE,
	SFLASH_SET_BOOTMODE,
	SFLASH_SET_BUSSEL,
	SFLASH_GET_BUSSEL,

	SFLASH_SET_UNSUSPEND,

	SFLASH_SET_MAX
} SFLASH_DRIVER_IOCTL_LIST;


#define	SFLASH_FORMAT_O0H0	XFC_TYPE_MOTOROLA_O0H0
#define	SFLASH_FORMAT_O0H1	XFC_TYPE_MOTOROLA_O0H1
#define	SFLASH_FORMAT_O1H0	XFC_TYPE_MOTOROLA_O1H0
#define	SFLASH_FORMAT_O1H1	XFC_TYPE_MOTOROLA_O1H1

#define	SFLASH_BUS_CAD(c,a,d)	(((c)<<12)|((a)<<8)|((a)<<4)|((d)<<0))
#define	SFLASH_BUS_111		SFLASH_BUS_CAD(1,1,1)
#define	SFLASH_BUS_112		SFLASH_BUS_CAD(1,1,2)
#define	SFLASH_BUS_122		SFLASH_BUS_CAD(1,2,2)
#define	SFLASH_BUS_222		SFLASH_BUS_CAD(2,2,2)
#define	SFLASH_BUS_114		SFLASH_BUS_CAD(1,1,4)
#define	SFLASH_BUS_144		SFLASH_BUS_CAD(1,4,4)
#define	SFLASH_BUS_444		SFLASH_BUS_CAD(4,4,4)
#define	SFLASH_BUS_011		SFLASH_BUS_CAD(0,1,1)
#define	SFLASH_BUS_001		SFLASH_BUS_CAD(0,0,1)
#define	SFLASH_BUS_022		SFLASH_BUS_CAD(0,2,2)
#define	SFLASH_BUS_012		SFLASH_BUS_CAD(0,1,2)
#define	SFLASH_BUS_002		SFLASH_BUS_CAD(0,0,2)
#define	SFLASH_BUS_044		SFLASH_BUS_CAD(0,4,4)
#define	SFLASH_BUS_014		SFLASH_BUS_CAD(0,1,4)
#define	SFLASH_BUS_004		SFLASH_BUS_CAD(0,0,4)

#define	SFLASH_BUS_MASK		(0x0FFFF)
#define	SFLASH_BUS_GRP(x)	(SFLASH_BUS_MASK&(x))
#define	SFLASH_BUS_444_GRP	SFLASH_BUS_GRP(SFLASH_BUS_444)
#define	SFLASH_BUS_144_GRP	SFLASH_BUS_GRP(SFLASH_BUS_144)
#define	SFLASH_BUS_114_GRP	SFLASH_BUS_GRP(SFLASH_BUS_114)
#define	SFLASH_BUS_222_GRP	SFLASH_BUS_GRP(SFLASH_BUS_222)
#define	SFLASH_BUS_122_GRP	SFLASH_BUS_GRP(SFLASH_BUS_122)
#define	SFLASH_BUS_112_GRP	SFLASH_BUS_GRP(SFLASH_BUS_112)
#define	SFLASH_BUS_111_GRP	SFLASH_BUS_GRP(SFLASH_BUS_111)
#define	SFLASH_BUS_NOGRP(x)	((~SFLASH_BUS_MASK)&(x))

#define	SFLASH_BUS_WRENB	0x01000000

#define	SFLASH_BUS_4BADDR	0x90000000
#define	SFLASH_BUS_3BADDR	0x80000000

#define	SFLASH_SUPPORT_QPP(x)	((((x) & 0x0FF0FF00) != 0) ? TRUE : FALSE)
#define	SFLASH_SUPPORT_PP114(x)	((((x) & 0x0F00F000) != 0) ? TRUE : FALSE)
#define	SFLASH_SUPPORT_PP144(x)	((((x) & 0x00F00F00) != 0) ? TRUE : FALSE)

typedef enum	{
	SFLASH_CSEL_0	= 0,
	SFLASH_CSEL_1,
	SFLASH_CSEL_2,
	SFLASH_CSEL_3,

	SFLASH_CSEL_MAX
} SFLASH_CSEL_LIST;

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------


struct				_jedec_
{
	UINT32			len;
	VOID			*sfdp;
	VOID			*cfi;
	VOID			*extra;
	VOID			*cmdlist;
	VOID			*delay;
};

struct __sflash_device_handler__
{
	UINT32				dev_addr;  // Unique Address

	// Device-dependant
	UINT32				dev_unit;
	UINT32				instance;

	VOID				*device;
	VOID				*bus;
	HANDLE				*bushdlr;

	UINT32				jedecid;
	UINT32				max_cfi;
	SFLASH_JEDEC_TYPE		jedec;
	UINT32				accmode;
	UINT32				maxspeed;
	UINT32				dwidth;
	UINT32				chbkup;
	UINT32				xipmode;
	UINT32				bussel;
	UINT32				slptim;
	UINT32				susmode;
	UINT32				psustime;
	UINT32				esustime;

	UINT8				*txbuffer;
	UINT8				*rxbuffer;
	VOID				*work;
};


//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------
#ifdef SUPPORT_SFLASH_DEVICE

extern HANDLE	SFLASH_CREATE( UINT32 dev_type );
extern int	SFLASH_INIT (HANDLE handler);
extern int	SFLASH_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );
extern int	SFLASH_READ (HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);
extern int	SFLASH_WRITE(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);
extern int	SFLASH_CLOSE(HANDLE handler);

extern int 	SFLASH_PATCH (HANDLE handler, UINT32 rtosmode);
#endif /*SUPPORT_SFLASH_DEVICE*/


//---------------------------------------------------------
//	DRIVER :: SFLASH Bus List
//---------------------------------------------------------

#undef	SUPPORT_SFLASH_BUS_SPI
#undef	SUPPORT_SFLASH_BUS_QSPI
#define	SUPPORT_SFLASH_BUS_XFC

//---------------------------------------------------------
//	DRIVER :: SFLASH Device List
//---------------------------------------------------------

#define	SUPPORT_SFLASH_UNKNOWN

#endif  /*__SFLASH_H__*/
