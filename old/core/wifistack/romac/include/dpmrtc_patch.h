/**
 ****************************************************************************************
 *
 * @file dpmrtc_patch.h
 *
 * @brief DPM RTC patch header
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

#ifndef __DPMRTC_PATCH_H__
#define __DPMRTC_PATCH_H__

#include "timp_patch.h"

#ifdef TIM_SIMULATION
/* Overflow Test Configuration: 4 secs */
#define RTC_COUNT_BITS      (17LL)
#define RTC_COUNT_MASK      (0x000000000001ffffLL)
#define RTC_COUNT_OVFL      (0x0000000000020000LL)
#define RTC_COUNT_OVFL_MASK (0xfffffffffffe0000LL)
#else
/* DA16X Counter: 36 Bits */
#define RTC_COUNT_BITS      (36LL)
#define RTC_COUNT_MASK      (0x0000000fffffffffLL)
#define RTC_COUNT_OVFL      (0x0000001000000000LL)
#define RTC_COUNT_OVFL_MASK (0xfffffff000000000LL)
#endif

extern long long dpm_get_clk(struct dpm_param *dpmp);
extern long long dpm_get_wakeup_clk(struct dpm_param *dpmp);
extern void dpm_wait_clk(long clk);
extern void dpm_wait_clk_patch(long clk);

#endif
