/**
 ****************************************************************************************
 *
 * @file sflash_bus.h
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


#ifndef __SFLASH_BUS_H__
#define __SFLASH_BUS_H__

#include "driver.h"
#include "sflash.h"


#define	SUPPORT_SFLASH_BUS

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef		struct	_sflash_bus_	SFLASH_BUS_TYPE;

//
//	IOCTL Commands
//

typedef enum	{
	SFLASH_BUS_SET_DEBUG = 0,

	SFLASH_BUS_SET_SPEED	,
	SFLASH_BUS_SET_FORMAT	,
	SFLASH_BUS_SET_DATA_WIDTH,

	SFLASH_BUS_SET_POLLMODE,
	SFLASH_BUS_SET_INTRMODE,
	SFLASH_BUS_SET_DMAMODE,
	SFLASH_BUS_SET_XIPMODE,

	SFLASH_BUS_SET_BUSCONTROL,

	SFLASH_BUS_GET_MAXRDSIZE,
	SFLASH_BUS_SET_MAXRDSIZE,

	SFLASH_BUS_SET_USLEEP,
	SFLASH_BUS_PRESET_USLEEP,

	SFLASH_BUS_SET_DELAYSEL,

	SFLASH_BUS_SET_MAX
} SFLASH_BUS_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: SFLASH Info
//---------------------------------------------------------

struct		_sflash_bus_
{
	UINT32	devid;
	HANDLE	(*create)   (VOID *hndlr);
	int	(*init)     (VOID *hndlr);
	int	(*ioctl)    (VOID *hndlr, UINT32 cmd , VOID *data);
	int	(*transmit) (VOID *hndlr
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );
	int	(*lock)     (VOID *hndlr, UINT32 mode);
	int	(*close)    (VOID *hndlr);
};

//---------------------------------------------------------
//	DRIVER :: SFLASH BUS Initializaiton
//---------------------------------------------------------

extern void	sflash_bus_init(SFLASH_HANDLER_TYPE *sflash);

#ifdef	SUPPORT_SFLASH_BUS_SPI
extern const SFLASH_BUS_TYPE	sflash_bus_spi;
#endif //SUPPORT_SFLASH_BUS_SPI
#ifdef	SUPPORT_SFLASH_BUS_QSPI
extern const SFLASH_BUS_TYPE	sflash_bus_qspi;
#endif //SUPPORT_SFLASH_BUS_QSPI
#ifdef	SUPPORT_SFLASH_BUS_XFC
extern const SFLASH_BUS_TYPE	sflash_bus_xfc;
#endif //SUPPORT_SFLASH_BUS_XFC

#endif  /*__SFLASH_BUS_H__*/
