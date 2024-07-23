/**
 ****************************************************************************************
 *
 * @file dpmpower.h
 *
 * @brief DPM power-up header
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

#ifndef __DPMPOWER_H__

extern void dpm_power_up_step0(unsigned long powerup_mode, unsigned long reserved);
extern void dpm_power_up_step1(unsigned long prep_bit, unsigned long calen);
extern void dpm_power_up_step1_patch(unsigned long prep_bit, unsigned long calen);
extern void dpm_power_up_step2(unsigned long prep_bit, unsigned long calen);
extern void dpm_power_up_step3(unsigned long prep_bit, unsigned long calen,
		unsigned long cal_addr);
extern void dpm_power_up_step3_patch(unsigned long prep_bit, unsigned long calen,
		unsigned long cal_addr);
extern void dpm_power_up_step4(unsigned long prep_bit);
extern void dpm_power_up_step4_patch(unsigned long prep_bit);
extern void dpm_prepare_for_power_down(unsigned long prep_bit);
extern void dpm_power_xip_on(long wait_clk);
extern void dpm_power_xip_off(void);
extern void dpm_power_fadc_cal(unsigned long prep_bit, unsigned long cal_addr);

#endif
