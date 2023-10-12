/**
 ****************************************************************************************
 *
 * @file dpm_aptrk_patch.h
 *
 * @brief APTRK patch header
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

#ifndef __DPM_APTRK_PATCH_H__
#define __DPM_APTRK_PATCH_H__

#include "dpmrtm_patch.h"

#define CHK_APTRK_NO_FEATURE(n)       (!(dpm_get_aptrk_en(dpmp) & (n)))
#define CHK_APTRK_FEATURE(n)          (dpm_get_aptrk_en(dpmp) & (n))
#define MK_APTRK_FEATURE(n)           (dpm_set_aptrk_en(dpmp, dpm_get_aptrk_en(dpmp) | n))

/* Max Contant 15 */
enum DPM_APTRK_LOCK {
	APTRK_UNLOCK,
	APTRK_COARSE_LOCK,
	APTRK_FINE_LOCK,
	APTRK_TRACKING,
	APTRK_SLOPE_MEASURE,
	APTRK_SLOPE_DONE
};

/* Max 8 Bits */
enum DPM_APTRK_MODE {
	APTRK_MODE_DISABLE        = 0x00,
	APTRK_MODE_COARSE_LOCK    = 0x01,
	APTRK_MODE_FINE_LOCK      = 0x02,
	APTRK_MODE_UNDER_TRACKING = 0x04,
	APTRK_MODE_OVER_TRACKING  = 0x08,
	APTRK_MODE_RESYNC_EN      = 0x10,
	APTRK_MODE_DETECT_CCA_EN  = 0x20,
	APTRK_MODE_SLOPE_MEASURE  = 0x40
};

/* Max 4 Bits */
enum DPM_APTRK_STATUS {
	APTRK_NONE           = 0x00,
	APTRK_DETECT_MIN_TSF = 0x01,
	APTRK_UPDATED        = 0x02,
	APTRK_OVER_DONE      = 0x04
};

extern void dpm_aptrk_init(struct dpm_param *dpmpode);
extern void dpm_aptrk_reset(struct dpm_param *dpmp);
extern void dpm_aptrk_measure(struct dpm_param *dpmp,
			long long timestamp, long long bcn_rtc,
			long cca, long tsflo);
extern long dpm_aptrk_get_cca(struct dpm_param *dpmp);
extern long dpm_aptrk_get_tbtt(struct dpm_param *dpmp);
extern long dpm_aptrk_get_timeout_tu(struct dpm_param *dpmp);

#endif /* __DPM_APTRK_PATCH_H__ */
