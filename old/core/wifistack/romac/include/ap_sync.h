/**
 ****************************************************************************************
 *
 * @file ap_sync.h
 *
 * @brief APSYNC header
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

////////////////////////////////////////////////////////////////////////////////
//
// Deprecated
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __AP_SYNC_H__
#define __AP_SYNC_H__

#include "dpmrtm_patch.h"

#define DPM_AP_SYNC_ATC 2000
#define MAX_RTC_CLK     (0x1000000000LL) // 36 bits

/* Max Contant 15 */
enum AP_SYNC_LOCK {
	AP_SYNC_UNLOCK,
	AP_SYNC_COARSE_LOCK,
	AP_SYNC_LOCK,
	AP_SYNC_TRACKING
};

/* Max 8 Bits */
enum DPM_AP_SYNC {
	AP_SYNC_MODE_DISABLE        = 0x00,
	AP_SYNC_MDOE_ENABLE         = 0x01,
	AP_SYNC_MODE_COARSE_LOCK    = 0x02,
	AP_SYNC_MODE_UNDER_TRACKING = 0x04,
	AP_SYNC_MODE_OVER_TRACKING  = 0x08,
	AP_SYNC_MODE_RESYNC_EN      = 0x10
};

/* Max 4 Bits */
enum DPM_AP_SYNC_TRACKING_STATUS {
	AP_SYNC_TRACKING_NONE           = 0x00,
	AP_SYNC_TRACKING_DETECT_MIN_TSF = 0x01,
	AP_SYNC_TRACKING_UPDATED        = 0x02,
	AP_SYNC_TRACKING_OVER_DONE      = 0x04
};

extern long ap_sync_get_tbtt(struct dpm_param *dpmp);
extern void ap_sync_reset(struct dpm_param *dpmp);
extern long ap_sync_get_tbtt_off(struct dpm_param *dpmp);
extern void ap_sync_measure_patch(struct dpm_param *dpmp);

#endif /* __AP_SYNC_H__ */
