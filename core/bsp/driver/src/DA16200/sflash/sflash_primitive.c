/**
 ****************************************************************************************
 *
 * @file sflash_primitive.c
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
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void	sflash_local_primitive_init(SFLASH_HANDLER_TYPE *sflash) CODE_SEC_SFLASH
{
	DA16X_UNUSED_ARG(sflash);
}

#ifdef	SFLASH_SLEEP_PRESET
#else	//SFLASH_SLEEP_PRESET
ATTRIBUTE_RAM_FUNC
void sflash_flash_sleep(UINT32 usec)	CODE_SEC_SFLASH
{
	UINT32 sysclock;
	volatile unsigned int k=0;

	_sys_clock_read( &sysclock, sizeof(UINT32) );
	k = sysclock >> 24 ;  // about 1 usec
	k = (k == 0) ? 1 : k;
	k = k * (usec+1) ; // VSIM timing violation (FPGA margin)

	while(k > 0){
		k-- ;
	}
}
#endif	//SFLASH_SLEEP_PRESET

#endif /*defined(SUPPORT_SFLASH_DEVICE) && defined(SUPPORT_SFLASH_ROMLIB)*/
