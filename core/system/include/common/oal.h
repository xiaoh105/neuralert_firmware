/**
 * \addtogroup OALayer
 * \{
 * \brief	OS Primitive wrappers
 */

/**
 ****************************************************************************************
 *
 * @file oal.h
 *
 * @brief Header files for OS Abstract Layer
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


#ifndef	__oal_h__
#define __oal_h__

#include	<stdio.h>
#include	<stdlib.h>
#include 	<stdarg.h>

#include "FreeRTOS.h"
#include "event_groups.h"

//--------------------------------------------------------------------
//	OS
//--------------------------------------------------------------------
#include 	"common_def.h"
#include	"FreeRTOS.h"
#include 	"queue.h"
#include 	"timers.h"
#include 	"event_groups.h"
#include 	"semphr.h"

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------
#include "da16x_types.h"

//--------------------------------------------------------------------
//	Retargeted Console & Pool
//--------------------------------------------------------------------
//#include 	"oal_primitives.h"


/******************************************************************************
 *
 * Options
 *
 ******************************************************************************/
#endif /*__oal_h__*/
/**
 * \}
 */

/* EOF */
