/**
 ****************************************************************************************
 *
 * @file     dpm_sys.h
 *
 * @brief    Define the TIM system control functions.
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

#ifndef __DPM_SYS_H__
#define __DPM_SYS_H__

/**
 * @file     dpm_sys.h
 * @brief    Define the TIM system control functions.
 * @details  This header file is only used in PTIM.
 * @ingroup  DPMCOMMON
 * @{
 */
#include "dpmrtm.h"

extern int dpm_sys_ready(struct dpm_param *dpmp);
extern int dpm_sys_chk(struct dpm_param *dpmp);
extern int dpm_sys_fullboot(struct dpm_param *dpmp, unsigned int boot_mem,
		unsigned int rtos_off);
extern long long dpm_sys_sleep(struct dpm_param *dpmp, unsigned long ptim_addr,
		long long rtc_us);

/// @}

#endif /* __DPM_SYS_H__ */
