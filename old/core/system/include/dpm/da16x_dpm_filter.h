/**
 ****************************************************************************************
 *
 * @file da16x_dpm_filter.h
 *
 * @brief
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


#ifndef _DPM_FILTER_H_
#define _DPM_FILTER_H_

#include "da16x_system.h"
#include "dpmrtm_patch.h"
#include "romac_primitives_patch.h"

/** TIM Wakeup Type definition per application processing **/
#if !defined (DEF_DPM_WAKEUP_TYPE)
#define DEF_DPM_WAKEUP_TYPE
enum DPM_WAKEUP_TYPE {
	DPM_UNKNOWN_WAKEUP     = 0,
	DPM_RTCTIME_WAKEUP     = 1,
	DPM_PACKET_WAKEUP      = 2,
	DPM_USER_WAKEUP        = 3,
	DPM_NOACK_WAKEUP       = 4,
	DPM_DEAUTH_WAKEUP      = 5,
	DPM_TIM_ERR_WAKEUP     = 6,
	DPM_DDPS_BUFP_WAKEUP	= 7
};
#endif // DEF_DPM_WAKEUP_TYPE

static inline void dpm_get_iv(unsigned char *iv, int len)
{
	unsigned char *p = dpm_get_txiv_iv(GET_DPMP());

	int i;

	//memcpy(iv, dpm_get_conf()->txiv.iv, len);
	//for (i = 0; i < len; i++)
	//	iv[i] = dpm_get_conf()->txiv.iv[i];

	for (i = 0; i < len; i++)
		iv[i] = p[i];
}

static inline void dpm_wep_iv2key(unsigned char *wep_key)
{
	unsigned char iv[4];

	dpm_get_iv(&iv[0], 4);

	wep_key[2] = iv[0];
	wep_key[1] = iv[1];
	wep_key[0] = iv[2];
}

static inline void dpm_ccmp_iv2pn(unsigned char *pn)
{
	unsigned char iv[8];

	dpm_get_iv(&iv[0], 8);

	pn[5] = iv[7];
	pn[4] = iv[6];
	pn[3] = iv[5];
	pn[2] = iv[4];
	pn[1] = iv[1];
	pn[0] = iv[0];
}

static inline void dpm_tkip_iv2tsc(unsigned char *tsc_16, unsigned char *tsc_32)
{
	unsigned char iv[8];

	dpm_get_iv(&iv[0], 8);

	tsc_16[1] = iv[0];
	tsc_16[0] = iv[2];
	tsc_32[0] = iv[4];
	tsc_32[1] = iv[5];
	tsc_32[2] = iv[6];
	tsc_32[3] = iv[7];
}

#endif // #ifndef _DPM_FILTER_H_

/* EOF */
