/**
 ****************************************************************************************
 *
 * @file sflash_device.c
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

#define	CODE_SEC_SFLASH
#define	RO_SEC_SFLASH

#if	defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)

/******************************************************************************
 *   sflash_device_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_SFLASH_LOCKINFO
static UINT8	sflash_common_sectors[1024];
#else
#define		sflash_common_sectors	NULL
#endif	//SUPPORT_SFLASH_LOCKINFO

ATTRIBUTE_RAM_FUNC
void	sflash_local_device_init(SFLASH_HANDLER_TYPE *sflash)
{
	DA16X_UNUSED_ARG(sflash);
}

/******************************************************************************
 *
 ******************************************************************************/
#ifdef	SUPPORT_SFLASH_ROMLIB
	// ROM Library linked

#else	//SUPPORT_SFLASH_ROMLIB

#ifdef	SUPPORT_SFLASH_UNKNOWN
/******************************************************************************
 *   UNKNOWN1
 ******************************************************************************/
ATTRIBUTE_RAM_RODATA
const char	sflash_local_anonymous_anonym[] = "ANONYM";

ATTRIBUTE_RAM_RODATA
const SFLASH_DEV_TYPE	sflash_local_anonymous
= {
	sflash_local_anonymous_anonym,				// *name
	0x00,					// mfr_id
	0x00,					// dev_id ( memory type, dev_id of REMS : 0x15 )
	0x00,					// density

	32*MBYTE,				// size
	256,					// page

	0, 					// num_secgrp
	NULL, 					// *sec_grp
	(8192),					// num_sector
	sflash_common_sectors,			// *protect

	NULL,					// *sfdp
	NULL,					// *cmdlist

	&sflash_unknown_probe,			// (*probe)
	&sflash_unknown_reset,			// (*reset)
	&sflash_unknown_busmode,		// (*busmode)
	&sflash_unknown_lock,			// (*lock)
	&sflash_unknown_unlock,			// (*unlock)
	&sflash_unknown_verify,			// (*verify)
	&sflash_unknown_erase,			// (*erase)
	&sflash_unknown_read,			// (*read)
	&sflash_unknown_write,			// (*write)
	&sflash_unknown_vendor,			// (*vendor)
	&sflash_unknown_powerdown,		// (*pwrdown)
	&sflash_unknown_buslock,		// (*buslock)
	&sflash_unknown_suspend			// (*suspend)
};

#endif	//SUPPORT_SFLASH_UNKNOWN

#endif	//SUPPORT_SFLASH_ROMLIB

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#endif /*defined(SUPPORT_SFLASH_DEVICE) && !defined(SUPPORT_SFLASH_ROMLIB)*/
