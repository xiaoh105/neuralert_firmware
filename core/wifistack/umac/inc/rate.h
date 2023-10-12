/**
 ****************************************************************************************
 *
 * @file rate.h
 *
 * @brief Header file for WiFi MAC Protocol Rate Processing
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

#ifndef I3ED11_RATE_H
#define I3ED11_RATE_H

#include "net_con.h"
#include "mac80211.h"
#include "ieee80211_i.h"
#include "um_sta_info.h"
#include "driver-ops.h"

#ifdef FEATURE_DPM_RATE_PRIVSTA_MUTEX
extern SemaphoreHandle_t dpm_sta_rate_mtx;
#endif

struct diw_rc_ref
{
	struct i3ed11_macctrl *macctrl;
	struct diw_rc_ops *ops;
	void *priv;
};

void diw_rc_get_rate(struct i3ed11_wifisubif *wsubif,
					 struct um_sta_info *sta,
					 struct i3ed11_tx_rate_control *txrc);

static inline void diw_rc_tx_status(struct i3ed11_macctrl *macctrl,
									struct i3ed11_supported_band *sband,
									struct um_sta_info *sta,
									struct wpktbuff *wpkt)
{
	struct diw_rc_ref *ref = macctrl->rate_ctrl;
	struct i3ed11_sta *ista = &sta->sta;
	void *priv_sta;	// = sta->rate_ctrl_priv;

	if (!ref || !test_sta_flag(sta, DIW_STA_RATE_CONTROL))
	{
		return;
	}

#ifdef	FEATURE_DPM_RATE_PRIVSTA_MUTEX
	{
		xSemaphoreTake(dpm_sta_rate_mtx, portMAX_DELAY);
		priv_sta = sta->rate_ctrl_priv;
		ref->ops->tx_status(ref->priv, sband, ista, priv_sta, wpkt);
		xSemaphoreGive(dpm_sta_rate_mtx);
	}
#else
	ref->ops->tx_status(ref->priv, sband, ista, priv_sta, wpkt);
#endif
}

static inline void diw_rc_rate_init(struct um_sta_info *sta)
{
	struct i3ed11_macctrl *macctrl = sta->wsubif->macctrl;
	struct diw_rc_ref *ref = sta->rate_ctrl;
	struct i3ed11_sta *ista = &sta->sta;
	void *priv_sta;	// = sta->rate_ctrl_priv;
	struct i3ed11_supported_band *sband;
	struct i3ed11_chanctx_conf *chanctx_conf;

	if (!ref)
	{
		return;
	}

	chanctx_conf = sta->wsubif->vif.chanctx_conf;

	if ((!chanctx_conf))
	{
		return;
	}

	sband = macctrl->hw.softmac->bands[chanctx_conf->def.chan->band];

#ifdef	FEATURE_DPM_RATE_PRIVSTA_MUTEX
	{
		xSemaphoreTake(dpm_sta_rate_mtx, portMAX_DELAY);
		priv_sta = sta->rate_ctrl_priv;
		ref->ops->rate_init(ref->priv, sband, &chanctx_conf->def, ista, priv_sta);
		xSemaphoreGive(dpm_sta_rate_mtx);
	}
#else
	ref->ops->rate_init(ref->priv, sband, &chanctx_conf->def, ista, priv_sta);
#endif
	set_sta_flag(sta, DIW_STA_RATE_CONTROL);
}

static inline void diw_rc_rate_update(struct i3ed11_macctrl *macctrl,
									  struct i3ed11_supported_band *sband,
									  struct um_sta_info *sta, u32 changed)
{
	struct diw_rc_ref *ref = macctrl->rate_ctrl;
	struct i3ed11_sta *ista = &sta->sta;
	void *priv_sta;	// = sta->rate_ctrl_priv;
	struct i3ed11_chanctx_conf *chanctx_conf;

	if (ref && ref->ops->rate_update)
	{

		chanctx_conf = sta->wsubif->vif.chanctx_conf;

		if ((!chanctx_conf))
		{
			return;
		}

#ifdef	FEATURE_DPM_RATE_PRIVSTA_MUTEX
		{
			xSemaphoreTake(dpm_sta_rate_mtx, portMAX_DELAY);
			priv_sta = sta->rate_ctrl_priv;
			ref->ops->rate_update(ref->priv, sband, &chanctx_conf->def,
								  ista, priv_sta, changed);
			xSemaphoreGive(dpm_sta_rate_mtx);
		}
#else
		ref->ops->rate_update(ref->priv, sband, &chanctx_conf->def,
							  ista, priv_sta, changed);
#endif
	}
}

static inline void *diw_rc_alloc_sta(struct diw_rc_ref *ref,
									 struct i3ed11_sta *sta,
									 gfp_t gfp)
{
	return ref->ops->alloc_sta(ref->priv, sta, gfp);
}

static inline void diw_rc_free_sta(struct um_sta_info *sta)
{
	struct diw_rc_ref *ref = sta->rate_ctrl;
	struct i3ed11_sta *ista = &sta->sta;
	void *priv_sta;	// = sta->rate_ctrl_priv;

#ifdef	FEATURE_DPM_RATE_PRIVSTA_MUTEX
	{
		xSemaphoreTake(dpm_sta_rate_mtx, portMAX_DELAY);
		priv_sta = sta->rate_ctrl_priv;
		ref->ops->free_sta(ref->priv, ista, priv_sta);
		xSemaphoreGive(dpm_sta_rate_mtx);
	}
#else
	ref->ops->free_sta(ref->priv, ista, priv_sta);
#endif
}

/* Get a reference to the rate control algorithm. If `name' is NULL, get the first available algorithm. */
int i3ed11_init_rc_alg(struct i3ed11_macctrl *macctrl, const char *name);
void diw_rc_deinitialize(struct i3ed11_macctrl *macctrl);

#endif /* I3ED11_RATE_H */
