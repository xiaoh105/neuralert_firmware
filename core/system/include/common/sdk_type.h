/**
 ****************************************************************************************
 *
 * @file sdk_type.h
 *
 * @brief Define for System Running Model
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

#ifndef	__SYS_RUN_TYPE_H__
#define	__SYS_RUN_TYPE_H__


// FreeRTOS feature
#include "FreeRTOS.h"
#include "project_config.h"


/// Global Define : Chipset Model
#if defined (__DA16600__)
#define CHIPSET_NAME	"DA16600"
#else
#define CHIPSET_NAME	"DA16200"
#endif	//(__DA16600__)


//////////////////////////////////////////////////////////////////


/* Default : Generic SDK mode */
#define	GENERIC_SDK


//////////////////////////////////////////////////////////////////

#if defined ( GENERIC_SDK )
	#include "config_generic_sdk.h"         // Customer's Generic SDK
#else
	#Config Error ...
#endif	/* ... customer_features ... */


#endif	// __SYS_RUN_TYPE_H__

/* EOF */
