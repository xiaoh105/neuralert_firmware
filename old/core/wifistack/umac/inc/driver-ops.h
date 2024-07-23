/**
 ****************************************************************************************
 *
 * @file driver-ops.h
 *
 * @brief Header file
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

#ifndef __MACD11_DRIVER_OPS
#define __MACD11_DRIVER_OPS

#include "mac80211.h"
#include "ieee80211_i.h"
#include "errno.h"
#include "umac_tasklet.h"
#include "da16x_system.h"

#ifdef UMAC_DBG_LOG_EN
#include "umac_util.h"
#endif

static inline struct i3ed11_wifisubif *
get_bss_sdata(struct i3ed11_wifisubif *wsubif)
{
	if (wsubif->vif.type == NLD11_IFTYPE_AP_VLAN)
		wsubif = container_of(wsubif->bss, struct i3ed11_wifisubif,
							  u.ap);

	return wsubif;
}

static inline void drv_tx(struct i3ed11_macctrl *macctrl,
						  struct i3ed11_tx_control *control,
						  struct wpktbuff *wpkt)
{
	macctrl->ops->tx(&macctrl->hw, control, wpkt);
}

static inline int drv_start(struct i3ed11_macctrl *macctrl)
{
	int ret;
	might_sleep();
	macctrl->started = true;

	ret = macctrl->ops->start(&macctrl->hw);
	return ret;
}

static inline void drv_stop(struct i3ed11_macctrl *macctrl)
{
	might_sleep();

	macctrl->ops->stop(&macctrl->hw);

	/* sync away all work on the tasklet before clearing started */
	umac_tasklet_disable(TASKLET_EVENT_NORMAL);
	umac_tasklet_enable(TASKLET_EVENT_NORMAL);

	macctrl->started = false;
}

static inline int drv_add_interface(struct i3ed11_macctrl *macctrl,
									struct i3ed11_wifisubif *wsubif)
{
	int ret;

	might_sleep();

	if (wsubif->vif.type == NLD11_IFTYPE_AP_VLAN)
	{
		return -EINVAL;
	}

	ret = macctrl->ops->add_interface(&macctrl->hw, &wsubif->vif);

	if (ret == 0)
	{
		wsubif->flags |= I3ED11_SDATA_IN_DRIVER;
	}

	return ret;
}

static inline void drv_remove_interface(struct i3ed11_macctrl *macctrl,
										struct i3ed11_wifisubif *wsubif)
{
	might_sleep();

	macctrl->ops->remove_interface(&macctrl->hw, &wsubif->vif);
	wsubif->flags &= ~I3ED11_SDATA_IN_DRIVER;
}


static inline int drv_config(struct i3ed11_macctrl *macctrl, u32 changed)
{
	int ret;

#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return 0;
	}

#endif

	might_sleep();
	ret = macctrl->ops->config(&macctrl->hw, changed);
	return ret;
}

static inline void drv_bss_info_changed(struct i3ed11_macctrl *macctrl,
										struct i3ed11_wifisubif *wsubif,
										struct i3ed11_bss_conf *info,
										u32 changed)
{
#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return;
	}

#endif

	might_sleep();

	if ((changed & (DIW_BSS_CHANGED_BEACON | DIW_BSS_CHANGED_BEACON_ENABLED) &&
#ifdef FEATURE_UMAC_MESH
		wsubif->vif.type != NLD11_IFTYPE_MESH_POINT &&
#endif /* FEATURE_UMAC_MESH */
			wsubif->vif.type != NLD11_IFTYPE_AP))
	{
		return;
	}

	if ((wsubif->vif.type == NLD11_IFTYPE_P2P_DEVICE))
	{
		return;
	}

	if (macctrl->ops->bss_info_changed)
	{
		macctrl->ops->bss_info_changed(&macctrl->hw, &wsubif->vif, info, changed);
	}
}

static inline u64 drv_prepare_multicast(struct i3ed11_macctrl *macctrl,
										struct netdev_hw_addr_list *mc_list)
{
	u64 ret = 0;

	ret = macctrl->ops->prepare_multicast(&macctrl->hw, mc_list);
	return ret;
}

static inline void drv_configure_filter(struct i3ed11_macctrl *macctrl,
										unsigned int changed_flags,
										unsigned int *total_flags,
										u64 multicast)
{

#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return;
	}

#endif

	macctrl->ops->configure_filter(&macctrl->hw, changed_flags, total_flags,
								   multicast);
}

static inline int drv_set_tim(struct i3ed11_macctrl *macctrl,
							  struct i3ed11_sta *sta, bool set)
{
	int ret = 0;

	if (macctrl->ops->set_tim)
	{
		ret = macctrl->ops->set_tim(&macctrl->hw, sta, set);
	}

	return ret;
}

static inline int drv_set_key(struct i3ed11_macctrl *macctrl,
							  enum set_key_cmd cmd,
							  struct i3ed11_wifisubif *wsubif,
							  struct i3ed11_sta *sta,
							  struct i3ed11_key_conf *key)
{
	int ret;

	might_sleep();

	wsubif = get_bss_sdata(wsubif);

	ret = macctrl->ops->set_key(&macctrl->hw, cmd, &wsubif->vif, sta, key);
	return ret;
}

static inline int drv_hw_scan(struct i3ed11_macctrl *macctrl,
							  struct i3ed11_wifisubif *wsubif,
							  struct cfgd11_scan_request *req)
{
	int ret;
#ifdef UMAC_DBG_LOG_EN

	if (um_dbg_info.is_on)
	{
		UM_PRTF(UM_SC, "UM->LM:hw_scan\n");
	}

#endif
	might_sleep();
	ret = macctrl->ops->hw_scan(&macctrl->hw, &wsubif->vif, req);
	return ret;
}

static inline void drv_cancel_hw_scan(struct i3ed11_macctrl *macctrl,
									  struct i3ed11_wifisubif *wsubif)
{
	might_sleep();
	//	PRINTF("drv_cancel_hw_scan\n");

	//	check_sdata_in_driver(wsubif);

	//	trace_drv_cancel_hw_scan(macctrl, wsubif);
	macctrl->ops->cancel_hw_scan(&macctrl->hw, &wsubif->vif);
	//	trace_drv_return_void(macctrl);
}

static inline int drv_sta_add(struct i3ed11_macctrl *macctrl,
							  struct i3ed11_wifisubif *wsubif,
							  struct i3ed11_sta *sta)
{
	int ret = 0;

	might_sleep();

	wsubif = get_bss_sdata(wsubif);

	ret = macctrl->ops->sta_add(&macctrl->hw, &wsubif->vif, sta);
 
	return ret;
}

static inline void drv_sta_remove(struct i3ed11_macctrl *macctrl,
								  struct i3ed11_wifisubif *wsubif,
								  struct i3ed11_sta *sta)
{
	might_sleep();

	wsubif = get_bss_sdata(wsubif);

	macctrl->ops->sta_remove(&macctrl->hw, &wsubif->vif, sta);

}

static inline int drv_sta_state(struct i3ed11_macctrl *macctrl,
								struct i3ed11_wifisubif *wsubif,
								struct um_sta_info *sta,
								enum i3ed11_sta_state old_state,
								enum i3ed11_sta_state new_state)
{
	int ret = 0;

	might_sleep();

	wsubif = get_bss_sdata(wsubif);

	if (old_state == I3ED11_STA_AUTH &&
			new_state == I3ED11_STA_ASSOC)
	{
		ret = drv_sta_add(macctrl, wsubif, &sta->sta);

		if (ret == 0)
		{
			sta->uploaded = true;
		}
	}
	else if (old_state == I3ED11_STA_ASSOC &&
			 new_state == I3ED11_STA_AUTH)
	{
		drv_sta_remove(macctrl, wsubif, &sta->sta);
	}

	return ret;
}

static inline int drv_conf_tx(struct i3ed11_macctrl *macctrl,
							  struct i3ed11_wifisubif *wsubif, u16 ac,
							  const struct i3ed11_tx_queue_params *params)
{
	int ret = -EOPNOTSUPP;
#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return 0;
	}

#endif
	might_sleep();
	ret = macctrl->ops->conf_tx(&macctrl->hw, &wsubif->vif, ac, params);

	return ret;
}

static inline int drv_ampdu_action(struct i3ed11_macctrl *macctrl,
								   struct i3ed11_wifisubif *wsubif,
								   enum i3ed11_ampdu_mlme_action action,
								   struct i3ed11_sta *sta, u16 tid,
								   u16 *ssn, u8 buf_sz)
{
	int ret = -EOPNOTSUPP;

	might_sleep();
	wsubif = get_bss_sdata(wsubif);
	ret = macctrl->ops->ampdu_action(&macctrl->hw, &wsubif->vif, action,
									 sta, tid, ssn, buf_sz);
	return ret;
}

static inline void drv_flush(struct i3ed11_macctrl *macctrl,
							 u32 queues, bool drop)
{
	might_sleep();
	macctrl->ops->flush(&macctrl->hw, queues, drop);
}

static inline bool drv_tx_frames_pending(struct i3ed11_macctrl *macctrl)
{
	bool ret = false;
	might_sleep();
	ret = macctrl->ops->tx_frames_pending(&macctrl->hw);
	return ret;
}


static inline void drv_mgd_prepare_tx(struct i3ed11_macctrl *macctrl,
									  struct i3ed11_wifisubif *wsubif)
{
	might_sleep();

	if (macctrl->ops->mgd_prepare_tx)
	{
		macctrl->ops->mgd_prepare_tx(&macctrl->hw, &wsubif->vif);
	}
}

static inline int drv_add_chanctx(struct i3ed11_macctrl *macctrl,
								  struct i3ed11_chanctx *ctx)
{
	int ret = -EOPNOTSUPP;

	if (macctrl->ops->add_chanctx)
	{
		ret = macctrl->ops->add_chanctx(&macctrl->hw, &ctx->conf);
	}

	if (!ret)
	{
		ctx->driver_present = true;
	}

	return ret;
}

static inline void drv_remove_chanctx(struct i3ed11_macctrl *macctrl,
									  struct i3ed11_chanctx *ctx)
{
#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, From the AP Deauth packet was received  */
	if (macctrl->dpmfastfullboot)
	{
		return;
	}

#endif

	if (macctrl->ops->remove_chanctx)
	{
		macctrl->ops->remove_chanctx(&macctrl->hw, &ctx->conf);
	}

	ctx->driver_present = false;
}

static inline void drv_change_chanctx(struct i3ed11_macctrl *macctrl,
									  struct i3ed11_chanctx *ctx,
									  u32 changed)
{
#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return;
	}

#endif

	if (macctrl->ops->change_chanctx)
	{
		macctrl->ops->change_chanctx(&macctrl->hw, &ctx->conf, changed);
	}
}

static inline int drv_assign_vif_chanctx(struct i3ed11_macctrl *macctrl,
		struct i3ed11_wifisubif *wsubif,
		struct i3ed11_chanctx *ctx)
{
	int ret = 0;


	if (macctrl->ops->assign_vif_chanctx)
	{
		ret = macctrl->ops->assign_vif_chanctx(&macctrl->hw, &wsubif->vif, &ctx->conf);
	}

	return ret;
}

static inline void drv_unassign_vif_chanctx(struct i3ed11_macctrl *macctrl,
		struct i3ed11_wifisubif *wsubif,
		struct i3ed11_chanctx *ctx)
{
#ifdef WITH_UMAC_DPM

	/* During DPM Full booting, bss_info already was loaded by LMAC */
	if (macctrl->dpmfastfullboot)
	{
		return;
	}

#endif

	if (macctrl->ops->unassign_vif_chanctx)
	{
		macctrl->ops->unassign_vif_chanctx(&macctrl->hw,
										   &wsubif->vif,
										   &ctx->conf);
	}
}

#ifdef UMAC_ACS_SUPPORT
static inline int drv_get_survey(struct i3ed11_macctrl *macctrl, int idx,
								 struct survey_info *survey)
{
	int ret = -EOPNOTSUPP;

	if (macctrl->ops->get_survey)
	{
		ret = macctrl->ops->get_survey(&macctrl->hw, idx, survey);
	}

	return ret;
}
#endif

#ifdef WITH_UMAC_DPM
static inline int drv_get_rtccurtime(struct i3ed11_macctrl *macctrl, u64 *rtccurtime)
{
	int ret = -EOPNOTSUPP;

	if (macctrl->ops->get_rtccurtime)
	{
		ret = macctrl->ops->get_rtccurtime(&macctrl->hw, rtccurtime);
	}

	return ret;
}

static inline int drv_get_rtctimegap(struct i3ed11_macctrl *macctrl, u64 *rtctime)
{
	int ret = -EOPNOTSUPP;

	if (macctrl->ops->get_rtctimegap)
	{
		ret = macctrl->ops->get_rtctimegap(&macctrl->hw, rtctime);
	}

	return ret;
}

#if 0 /* Not used */
static inline int drv_set_keepalivetime(struct i3ed11_macctrl *macctrl, unsigned int sec,
										void (* callback_func)(void))
{
	int ret = -EOPNOTSUPP;


	if (macctrl->ops->set_keepalivetime)
	{
		ret = macctrl->ops->set_keepalivetime(&macctrl->hw, sec, callback_func);
	}

	/* Timer ID */
	return ret;
}
#endif

static inline int drv_set_app_keepalivetime(struct i3ed11_macctrl *macctrl,
		unsigned char tid, unsigned int msec, void (* callback_func)(unsigned int tid))
{
	int ret = -EOPNOTSUPP;


	if (macctrl->ops->set_app_keepalivetime)
	{
		ret = macctrl->ops->set_app_keepalivetime(&macctrl->hw, tid, msec, callback_func);
	}

	/* Timer ID */
	return ret;
}


static inline void drv_set_macaddr(struct i3ed11_macctrl *macctrl)
{
	if (macctrl->ops->set_macaddress)
	{
		macctrl->ops->set_macaddress(&macctrl->hw);
	}

	return;
}

static inline void drv_pridown(struct i3ed11_macctrl *macctrl, bool retention)
{
	if (macctrl->ops->pwrdown_primary)
	{
		macctrl->ops->pwrdown_primary(&macctrl->hw, retention);
	}

	return;
}

static inline void drv_secdown(struct i3ed11_macctrl *macctrl, unsigned long long usec,
							   bool retention)
{
	if (macctrl->ops->pwrdown_secondary)
	{
		macctrl->ops->pwrdown_secondary(&macctrl->hw, usec, retention);
	}

	return;
}

static inline void drv_dpm_setup(struct i3ed11_macctrl *macctrl,
								 unsigned char ap_sync_cntrl,
								 int ka_period_ms)
{
	if (macctrl->ops->set_dpmconf)
	{
		macctrl->ops->set_dpmconf(&macctrl->hw, ap_sync_cntrl, ka_period_ms);
	}

	return;
}
#endif

static inline int drv_cntrl_mac_hw(struct i3ed11_macctrl *macctrl, unsigned int cntrl,
								   unsigned int val)
{
	int ret = 0;

	if (macctrl->ops->cntrl_mac_hw)
	{
		ret = macctrl->ops->cntrl_mac_hw(&macctrl->hw, cntrl, val);
	}

	return ret;
}

#ifdef FEATURE_UMAC_MESH
static inline unsigned long long drv_get_tsf(struct i3ed11_macctrl *macctrl, struct i3ed11_wifisubif *wsubif)
{
	unsigned long long ret = -1ULL;

#if 0
	if(macctrl->ops->get_tsf)
		ret = macctrl->ops->get_tsf(&macctrl->hw, &wsubif->vif);		
#endif
	return ret;
}
static inline void drv_set_tsf(struct i3ed11_macctrl *macctrl, struct i3ed11_wifisubif *wsubif , unsigned long long tsf)
{
	int ret = 0;

#if 0
	if(macctrl->ops->set_tsf)
		ret = macctrl->ops->set_tsf(&macctrl->hw, &wsubif->vif , tsf);		
#endif
	return;
}
#endif /* FEATURE_UMAC_MESH */

#endif /* __MACD11_DRIVER_OPS */
