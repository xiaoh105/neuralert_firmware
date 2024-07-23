/**
 ****************************************************************************************
 *
 * @file schd_idle.c
 *
 * @brief Idle Management
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

#include "oal.h"
#include "hal.h"
#include "da16x_system.h"

/******************************************************************************
 * Macro
 ******************************************************************************/


/******************************************************************************
 * Declaration
 ******************************************************************************/

static UINT32	da16x_sleep_mode;

/******************************************************************************
 *  init_idle_management( )
 *
 *  Purpose :   preprocessing routine
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void init_idle_management(void)
{
	da16x_sleep_mode = FALSE;

}

/******************************************************************************
 *  da16x_idle_get_info( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_idle_get_info(UINT32 mode)
{
	DA16X_UNUSED_ARG(mode);

	return da16x_sleep_mode;
}

/******************************************************************************
 *  da16x_idle_set_info( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	da16x_idle_set_info(UINT32 mode)
{
	da16x_sleep_mode = mode;
	return TRUE;
}


/* EOF */
