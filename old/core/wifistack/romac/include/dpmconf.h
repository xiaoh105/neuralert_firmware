/**
 ****************************************************************************************
 *
 * @file     dpmconf.h
 *
 * @brief    DPM configuration
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

#ifndef __DPMCONF_H__
#define __DPMCONF_H__

/**
 * @file     dpmconf.h
 * @brief    DPM configuration
 * @details  This file defines the DPM configuration.
 *           This header file is used in RTOS & PTIM.
 * @defgroup DPM_CONF
 * @ingroup  ROMAC_COM
 * @{
 */

/* Simulation */
/*#define TIM_SIMULATION*/ /* TIM_SIMULATION is defined in vcxproj of Visual Studio. */
#ifdef TIM_SIMULATION
#define DPM_RTC_CLK_SPEED_EN
#define DPM_RTC_CLK_SPEED 0
#endif

#undef  DPM_APTRK_CCA_MEASURE /* for SLR */
#define TIM_XTAL_CHANGE_EN /* TIM changes OSC into Xtal 32K */
#undef DPM_RTM_AUTO_CLEAR /* PTIM TEST FEATURE */

#define NX_MDM_VER              20

//#define PTIM_SYS_CLK		80
//#define PTIM_MAC_CLK		40

/**
 * DEBUG FEATURE
 */
#undef DPM_TEST_ALWAYS_HI_CURRENT_CONTROL
#undef DPM_TEST_PBR_EVT_OFF
#undef DPM_TEST_FADC_V_CONTROL

/**
 * ##### DA16200-20190113-LOW_CURRENT_MODE
 * 	DPM_TEST_ALWAYS_HI_CURRENT_CONTROL
 *	DPM_TEST_PBR_EVT_OFF
 *	DPM_TEST_FADC_V_CONTROL
 **/

/// @} end of group DPM_CONF

#endif
