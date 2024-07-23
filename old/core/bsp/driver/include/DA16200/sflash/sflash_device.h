/**
 ****************************************************************************************
 *
 * @file sflash_device.h
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


#ifndef __SFLASH_DEVICE_H__
#define __SFLASH_DEVICE_H__

#include "driver.h"
#include "sflash.h"

//---------------------------------------------------------
// Deferred Types
//---------------------------------------------------------

typedef		struct	_sflash_device_	SFLASH_DEV_TYPE;
typedef		struct _sflash_sector_	SFLASH_SECTOR_LIST;

//---------------------------------------------------------
//	DRIVER :: SFLASH Info
//---------------------------------------------------------

struct		_sflash_sector_
{
	UINT32		offset;
	UINT32		blksiz;
	UINT32		blknum;
};

struct		_sflash_device_
{
	const char			*name;
	UINT8			mfr_id;
	UINT8			dev_id;
	UINT8			density;

	UINT32			size;
	UINT32			page;

	UINT32			num_secgrp;
	SFLASH_SECTOR_LIST	*sec_grp;
	UINT32			num_sector;
	UINT8			*protect;

	VOID			*sfdp;
	VOID			*cmdlist;

	int	(*probe)   (SFLASH_HANDLER_TYPE *sflash, VOID *device, UINT8 *dump);
	int	(*reset)   (SFLASH_HANDLER_TYPE *sflash, UINT32 mode);
	int	(*busmode) (SFLASH_HANDLER_TYPE *sflash, UINT32 mode);
	int	(*lock)    (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 num);
	int	(*unlock)  (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 num);
	int	(*verify)  (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 num);
	int	(*erase)   (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 num);
	int	(*read)    (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, VOID *data, UINT32 len);
	int	(*write)   (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, VOID *data, UINT32 len);
	int	(*vendor)  (SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data);
	int	(*pwrdown) (SFLASH_HANDLER_TYPE *sflash, UINT32 mode, UINT32 *data);
	int	(*buslock) (SFLASH_HANDLER_TYPE *sflash, UINT32 mode);
	int	(*suspend) (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 mode, UINT32 secsiz);
};

//---------------------------------------------------------
//	DRIVER :: SFLASH Device Initializaiton
//---------------------------------------------------------

extern void	sflash_local_device_init(SFLASH_HANDLER_TYPE	*sflash);

//---------------------------------------------------------
//	DRIVER :: SFLASH Device Declaration
//---------------------------------------------------------
#if	defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)

#ifdef	SUPPORT_SFLASH_UNKNOWN
extern const SFLASH_DEV_TYPE	sflash_local_anonymous;
#endif	//SUPPORT_SFLASH_UNKNOWN

#endif  /*defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)*/

#endif  /*__SFLASH_DEVICE_H__*/
