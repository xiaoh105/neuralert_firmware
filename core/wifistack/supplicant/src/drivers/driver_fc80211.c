/**
 ****************************************************************************************
 *
 * @file driver_fc80211.c
 *
 * @brief Driver interaction with DA16X fc80211
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


#include "sdk_type.h"	// After app_features.h

#include "includes.h"

#include "fc80211_copy.h"
#include "common/ieee802_11_defs.h"

#include "supp_driver.h"
#include "supp_eloop.h"
#include "supp_config.h"
#include "wpa_supplicant_i.h"
#include "supp_scan.h"
#include "da16x_system.h"

#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wunused-variable"

extern unsigned char	passive_scan_feature_flag;

extern int get_run_mode(void);
extern int chk_dpm_mode(void);
extern int chk_dpm_wakeup(void);

#ifdef CONFIG_SIMULATED_MIC_FAILURE
extern union wpa_event_data	mic_event;
#endif

struct fc80211_global {
	struct	dl_list		interfaces;
	int	if_add_ifindex;
	ULONG	if_add_wdevid;
	int	if_add_wdevid_set;
	int	fc80211_id;
};

struct fc80211_softmac_data {
	struct dl_list	list;
	struct dl_list	bsss;
	struct dl_list	drvs;

	int softmac_idx;
};

void fc80211_global_deinit(void *priv);

struct wpa_driver_fc80211_data {
	struct fc80211_global *global;
	struct dl_list list;
	struct dl_list softmac_list;
	char	phyname[16];
	void	*ctx;
	int	ifindex;
	int	if_removed;
	int	if_disabled;
	int	ignore_if_down_event;
	struct wpa_driver_capa capa;
	UCHAR *extended_capa, *extended_capa_mask;
	unsigned int extended_capa_len;
	int	has_capability;

	int	operstate;

	int	scan_complete_events;
	enum scan_states {
		NO_SCAN, SCAN_REQUESTED, SCAN_STARTED, SCAN_COMPLETED,
		SCAN_ABORTED
	} scan_state;

	UCHAR	auth_bssid[ETH_ALEN];
	UCHAR	auth_attempt_bssid[ETH_ALEN];
	UCHAR	bssid[ETH_ALEN];
	UCHAR	prev_bssid[ETH_ALEN];
	int	associated;
	UCHAR	ssid[32];
	size_t	ssid_len;
	enum fc80211_iftype	nlmode;
	enum fc80211_iftype	ap_scan_as_station;
	unsigned int	assoc_freq;

	unsigned int disabled_11b_rates:1;
	unsigned int pending_remain_on_chan:1;
	unsigned int in_interface_list:1;
	unsigned int device_ap_sme:1;
	unsigned int poll_command_supported:1;
	unsigned int data_tx_status:1;
	unsigned int scan_for_auth:1;
	unsigned int retry_auth:1;
	unsigned int use_monitor:1;
	unsigned int ignore_next_local_disconnect:1;
	unsigned int ignore_next_local_deauth:1;
	unsigned int allow_p2p_device:1;
	unsigned int hostapd:1;
	unsigned int start_mode_ap:1;
	unsigned int start_iface_up:1;
	unsigned int test_use_roc_tx:1;
	unsigned int ignore_deauth_event:1;
	unsigned int dfs_vendor_cmd_avail:1;

	ULONG	remain_on_chan_cookie;
	ULONG	send_action_cookie;

	unsigned int	last_mgmt_freq;

	struct wpa_driver_scan_filter	*filter_ssids;
	size_t num_filter_ssids;

	struct i802_bss *first_bss;

	int	default_if_indices[16];
	int	*if_indices;
	int	num_if_indices;

	/* From failed authentication command */
	int	auth_freq;
	UCHAR	auth_bssid_[ETH_ALEN];
	UCHAR	auth_ssid[32];
	size_t	auth_ssid_len;
	int	auth_alg;
	UCHAR	*auth_ie;
	size_t	auth_ie_len;
	UCHAR	auth_wep_key[4][16];
	size_t	auth_wep_key_len[4];
	int	auth_wep_tx_keyidx;
	int	auth_local_state_change;
	int	auth_p2p;

	int wnm_intval; //CONFIG_WNM_SLEEP_MODE

#ifdef	CONFIG_SUPP27_DRV_80211
	UCHAR	scan_ssid[32];
	size_t	scan_ssid_len;
	UCHAR	scan_bssid[ETH_ALEN];
#endif	/* CONFIG_SUPP27_DRV_80211 */
};

/* By Bright : extern define for removed libnl functions */
/* Do not move to another position */
#include "driver_fc80211.h"

struct fc80211_global *global;

void wpa_driver_fc80211_deinit(struct i802_bss *bss);
void wpa_driver_fc80211_scan_timeout(int timeout, void *drv, void *ctx);
int wpa_driver_fc80211_set_mode(struct i802_bss *bss,
			enum fc80211_iftype nlmode);
int wpa_driver_fc80211_finish_drv_init(
			struct wpa_driver_fc80211_data *drv,
			const UCHAR *set_addr, int first);
int wpa_driver_fc80211_mlme(struct wpa_driver_fc80211_data *drv,
			const UCHAR *addr, int cmd, USHORT reason_code,
			int local_state_change);
int fc80211_send_frame_cmd(struct i802_bss *bss,
			unsigned int freq, unsigned int wait,
			const UCHAR *buf, size_t buf_len, ULONG *cookie,
			int no_cck, int no_ack, int offchanok);
int fc80211_register_frame(struct i802_bss *bss,
			USHORT type, const UCHAR *match, size_t match_len);
int wpa_driver_fc80211_probe_req_report(struct i802_bss *bss,
		       int report);
void add_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx);
void del_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx);
int have_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx);
int wpa_driver_fc80211_if_remove(struct i802_bss *bss,
			enum wpa_driver_if_type type, const char *ifname);

int fc80211_set_chan(struct i802_bss *bss,
			       struct hostapd_freq_params *freq, int set_chan);
int fc80211_disable_11b_rates(struct wpa_driver_fc80211_data *drv,
				     int ifindex, int disabled);

int i802_set_iface_flags(struct i802_bss *bss, int up);

int is_ap_interface(enum fc80211_iftype nlmode)
{
	return nlmode == FC80211_IFTYPE_AP || nlmode == FC80211_IFTYPE_P2P_GO;
}

int is_sta_interface(enum fc80211_iftype nlmode)
{
	return nlmode == FC80211_IFTYPE_STATION ||
		nlmode == FC80211_IFTYPE_P2P_CLIENT;
}

#ifdef CONFIG_MESH
static int is_mesh_interface(enum fc80211_iftype nlmode)
{
	return nlmode == FC80211_IFTYPE_MESH_POINT;
}
#endif /* CONFIG_MESH */

#ifdef	CONFIG_P2P
int is_p2p_net_interface(enum fc80211_iftype nlmode)
{
	return nlmode == FC80211_IFTYPE_P2P_CLIENT ||
		nlmode == FC80211_IFTYPE_P2P_GO;
}
#endif	/* CONFIG_P2P */

void fc80211_mark_disconnected(struct wpa_driver_fc80211_data *drv)
{
	if (drv->associated)
		os_memcpy(drv->prev_bssid, drv->bssid, ETH_ALEN);
	drv->associated = 0;
	os_memset(drv->bssid, 0, ETH_ALEN);
}

struct fc80211_bss_info_buf {
	struct wpa_driver_fc80211_data *drv;
	struct wpa_scan_results *res;
	unsigned int assoc_freq;
	unsigned int ibss_freq;
	UCHAR assoc_bssid[ETH_ALEN];
};

struct softmac_idx_data {
	int softmac_idx;
	enum fc80211_iftype nlmode;
	UCHAR *macaddr;
};

int fc80211_get_softmac_index(struct i802_bss *bss)
{
	int ret = -1;

	/* by Bright : FC80211_CMD_GET_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_get_interface_softmac_index(bss->wdev_id, bss->ifindex);

	return ret;
}

enum fc80211_iftype fc80211_get_ifmode(struct i802_bss *bss)
{
	int	if_mode;

	TX_FUNC_START("");

	/* by Bright : FC80211_CMD_GET_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	if_mode = fc80211_get_interface_mode(bss->wdev_id, bss->ifindex);

	if (if_mode < 0) {
		da16x_drv_prt("[%s] if_mode=%d error\n", __func__, if_mode);
		return FC80211_IFTYPE_UNSPECIFIED;
	}

	da16x_drv_prt("[%s] ifmode=%d\n", __func__, if_mode);

	TX_FUNC_END("");

	return (enum fc80211_iftype)if_mode;
}

int fc80211_get_macaddr(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	UCHAR	*mac_addr;

	TX_FUNC_START("");

	/* by Bright : FC80211_CMD_GET_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	mac_addr = fc80211_get_interface_macaddr(bss->wdev_id, drv->ifindex);

	if (mac_addr == NULL)
		os_memset(bss->addr, 0, ETH_ALEN);
	else
		os_memcpy(bss->addr, mac_addr, ETH_ALEN);

	da16x_drv_prt("[%s] My MacAddr=[" MACSTR "]\n",
			__func__,
			bss->addr[0], bss->addr[1], bss->addr[2],
			bss->addr[3], bss->addr[4], bss->addr[5]);

	TX_FUNC_END("");

	return 0;
}

struct fc80211_softmac_data *fc80211_get_softmac_data_ap(struct i802_bss *bss)
{
	static DEFINE_DL_LIST(fc80211_softmacs);
	struct fc80211_softmac_data *w;
	int softmac_idx, found = 0;
	struct i802_bss *tmp_bss;

	if (bss->softmac_data != NULL) {
		da16x_drv_prt("[%s] softmac_data is exist\n", __func__);
		return bss->softmac_data;
	}

	softmac_idx = fc80211_get_softmac_index(bss);

	dl_list_for_each(w, &fc80211_softmacs, struct fc80211_softmac_data, list) {
		if (w->softmac_idx == softmac_idx) {
			goto add;
		}
	}

	/* alloc new one */
	w = os_zalloc(sizeof(*w));
	if (w == NULL)
		return NULL;

	w->softmac_idx = softmac_idx;

	dl_list_init(&w->bsss);
	dl_list_init(&w->drvs);

	dl_list_add(&fc80211_softmacs, &w->list);

add:
	/* drv entry for this bss already there? */
	dl_list_for_each(tmp_bss, &w->bsss, struct i802_bss, softmac_list) {
		if (tmp_bss->drv == bss->drv) {
			found = 1;
			break;
		}
	}
	/* if not add it */
	if (!found)
		dl_list_add(&w->drvs, &bss->drv->softmac_list);

	dl_list_add(&w->bsss, &bss->softmac_list);
	bss->softmac_data = w;

	return w;
}

void fc80211_put_softmac_data_ap(struct i802_bss *bss)
{
	struct fc80211_softmac_data *w = bss->softmac_data;
	struct i802_bss *tmp_bss;
	int found = 0;

	TX_FUNC_START("");

	if (w == NULL) {
		da16x_drv_prt("[%s] softmac_data is NULL\n", __func__);
		goto err_rtn;
	}

	bss->softmac_data = NULL;
	dl_list_del(&bss->softmac_list);

	/* still any for this drv present? */
	dl_list_for_each(tmp_bss, &w->bsss, struct i802_bss, softmac_list) {
		if (tmp_bss->drv == bss->drv) {
			found = 1;
			break;
		}
	}
	/* if not remove it */
	if (!found)
		dl_list_del(&bss->drv->softmac_list);

	if (!dl_list_empty(&w->bsss)) {
		da16x_drv_prt("[%s] Error #2\n", __func__);
		goto err_rtn;
	}

	dl_list_del(&w->list);
	os_free(w);

err_rtn :
	TX_FUNC_END("");
}

int wpa_driver_fc80211_get_bssid(void *priv, UCHAR *bssid)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	if (!drv->associated)
		return -1;
	os_memcpy(bssid, drv->bssid, ETH_ALEN);

	TX_FUNC_END("");

	return 0;
}

int wpa_driver_fc80211_get_ssid(void *priv, UCHAR *ssid)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	if (!drv->associated)
		return -1;
	os_memcpy(ssid, drv->ssid, drv->ssid_len);

	TX_FUNC_END("");

	return drv->ssid_len;
}

void mlme_event_auth(struct wpa_driver_fc80211_data *drv,
			    const UCHAR *frame, size_t len)
{
	const struct _ieee80211_mgmt *mgmt;
	union wpa_event_data event;

	RX_FUNC_START("");

	mgmt = (const struct _ieee80211_mgmt *) frame;
	if (len < 24 + sizeof(mgmt->u.auth)) {
		da16x_drv_prt("<%s> Too short associate event frame\n", __func__);
		return;
	}

	os_memcpy(drv->auth_bssid, mgmt->sa, ETH_ALEN);
	os_memset(drv->auth_attempt_bssid, 0, ETH_ALEN);
	os_memset(&event, 0, sizeof(event));
	os_memcpy(event.auth.peer, mgmt->sa, ETH_ALEN);

	event.auth.auth_type = le_to_host16(mgmt->u.auth.auth_alg);
	event.auth.auth_transaction =
		le_to_host16(mgmt->u.auth.auth_transaction);
	event.auth.status_code = le_to_host16(mgmt->u.auth.status_code);
	if (len > 24 + sizeof(mgmt->u.auth)) {
		event.auth.ies = mgmt->u.auth.variable;
		event.auth.ies_len = len - 24 - sizeof(mgmt->u.auth);
	}

	wpa_supplicant_event(drv->ctx, EVENT_AUTH, &event);

	RX_FUNC_END("");
}

unsigned int fc80211_get_assoc_freq(struct wpa_driver_fc80211_data *drv)
{
	unsigned int freq = 0;
	struct fc80211_bss_info_buf buf;
	struct i802_bss *bss = drv->first_bss;

	TX_FUNC_START("");

	os_memset(&buf, 0, sizeof(buf));

	buf.drv = drv;

	/* by Bright : FC80211_CMD_GET_SCAN
	 *	Direct function call on UMAC layer
	 */
#if 0	 
	freq = fc80211_get_scan(bss->wdev_id, drv->ifindex, (void *)&buf);
#else
	fc80211_get_scan(bss->wdev_id, drv->ifindex, (void *)&buf);
	freq = buf.assoc_freq;
#endif
	if (freq)
		drv->assoc_freq = freq;

	da16x_drv_prt("[%s] Operating frequency for the "
		   "associated BSS from scan results: %u MHz\n",
						__func__, freq);

	TX_FUNC_END("");

	return drv->assoc_freq;
}

void mlme_event_assoc(struct wpa_driver_fc80211_data *drv,
			    const UCHAR *frame, size_t len)
{
	const struct _ieee80211_mgmt *mgmt;
	union wpa_event_data event;
	USHORT status;

	RX_FUNC_START("");

	mgmt = (const struct _ieee80211_mgmt *) frame;
	if (len < 24 + sizeof(mgmt->u.assoc_resp)) {
		da16x_drv_prt("[%s] Too short association event frame\n", __func__);
		return;
	}

	status = le_to_host16(mgmt->u.assoc_resp.status_code);
	if (status != WLAN_STATUS_SUCCESS) {
		os_memset(&event, 0, sizeof(event));
		event.assoc_reject.bssid = mgmt->bssid;
		if (len > 24 + sizeof(mgmt->u.assoc_resp)) {
			event.assoc_reject.resp_ies =
				(UCHAR *) mgmt->u.assoc_resp.variable;
			event.assoc_reject.resp_ies_len =
				len - 24 - sizeof(mgmt->u.assoc_resp);
		}
		event.assoc_reject.status_code = status;

		wpa_supplicant_event(drv->ctx, EVENT_ASSOC_REJECT, &event);
		return;
	}

	drv->associated = 1;
	os_memcpy(drv->bssid, mgmt->sa, ETH_ALEN);
	os_memcpy(drv->prev_bssid, mgmt->sa, ETH_ALEN);

	os_memset(&event, 0, sizeof(event));
	if (len > 24 + sizeof(mgmt->u.assoc_resp)) {
		event.assoc_info.resp_ies = (UCHAR *) mgmt->u.assoc_resp.variable;
		event.assoc_info.resp_ies_len =
			len - 24 - sizeof(mgmt->u.assoc_resp);
	}

	event.assoc_info.freq = drv->assoc_freq;

	wpa_supplicant_event(drv->ctx, EVENT_ASSOC, &event);

	RX_FUNC_END("");
}

void mlme_timeout_event(struct wpa_driver_fc80211_data *drv,
				da16x_drv_msg_buf_t *drv_msg_buf)
{
	union wpa_event_data event;
	enum wpa_event_type ev;

	da16x_drv_prt("[%s] MLME event %d; timeout with " MACSTR "\n",
			__func__,
			drv_msg_buf->cmd,
			MAC2STR((UCHAR *) drv_msg_buf->data->attr.addr));

	if (drv_msg_buf->cmd == FC80211_CMD_AUTHENTICATE)
		ev = EVENT_AUTH_TIMED_OUT;
	else if (drv_msg_buf->cmd == FC80211_CMD_ASSOCIATE)
		ev = EVENT_ASSOC_TIMED_OUT;
	else
		return;

	os_memset(&event, 0, sizeof(event));
	wpa_supplicant_event(drv->ctx, ev, &event);
}

void mlme_event_mgmt(struct i802_bss *bss,
			    da16x_drv_msg_buf_t *drv_msg_buf,
			    const UCHAR *frame, size_t len)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	union wpa_event_data event;
#ifdef	ENABLE_DRV_DBG
	const struct _ieee80211_mgmt *mgmt;
	USHORT fc, stype;
#endif	/* ENABLE_DRV_DBG */
	int ssi_signal;
#ifdef	ENABLE_DRV_DBG
	int rx_freq;
#endif	/* ENABLE_DRV_DBG */

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_START("");
	}
#ifdef	ENABLE_DRV_DBG
	mgmt = (const struct _ieee80211_mgmt *) frame;
#endif	/* ENABLE_DRV_DBG */
	if (len < 24) {
		da16x_drv_prt("[%s] Too short mgmt frame\n", __func__);
		return;
	}

#ifdef	ENABLE_DRV_DBG
	stype = 0;
	rx_freq = 0;
#endif	/* ENABLE_DRV_DBG */
	ssi_signal = 0;

#ifdef	ENABLE_DRV_DBG
	fc = le_to_host16(mgmt->frame_control);
	stype = WLAN_FC_GET_STYPE(fc);
#endif	/* ENABLE_DRV_DBG */

	ssi_signal = (int)drv_msg_buf->data->attr.sig_dbm;
	os_memset(&event, 0, sizeof(event));


	event.rx_mgmt.freq = (int)drv_msg_buf->data->attr.freq;
	drv->last_mgmt_freq = event.rx_mgmt.freq;
#ifdef	ENABLE_DRV_DBG
	rx_freq = drv->last_mgmt_freq;
#endif	/* ENABLE_DRV_DBG */

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {

		da16x_drv_prt("[%s] RX frame freq=%d ssi_signal=%d stype=%u len=%u\n",
				__func__,
			   rx_freq, ssi_signal, stype, (unsigned int) len);
	}


	event.rx_mgmt.frame = frame;
	event.rx_mgmt.frame_len = len;
	event.rx_mgmt.ssi_signal = ssi_signal;
	event.rx_mgmt.drv_priv = bss;

	wpa_supplicant_event(drv->ctx, EVENT_RX_MGMT, &event);

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_END("");
	}
}

void mlme_event_mgmt_tx_status(struct wpa_driver_fc80211_data *drv,
					da16x_drv_msg_buf_t *drv_msg_buf,
					const UCHAR *frame,size_t len)
{
	union wpa_event_data event;
	const struct _ieee80211_hdr *hdr;
	USHORT fc;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] Frame TX status event\n", __func__);

	if (!is_ap_interface(drv->nlmode)) {
		ULONG cookie_val;

		if (drv_msg_buf->data->attr.cookie == 0)
			return;

		cookie_val = drv_msg_buf->data->attr.cookie;
		da16x_drv_prt("[%s] Action TX status: cookie=0%llx%s (ack=%d)\n",
				__func__,
				(long long unsigned int) cookie_val,
				cookie_val == drv->send_action_cookie ?
					" (match)" : " (unknown - Discard)",
					drv_msg_buf->data->attr.ack != 0);
		if (cookie_val != drv->send_action_cookie) {
			return;
		}
	}

	hdr = (const struct _ieee80211_hdr *) frame;
	fc = le_to_host16(hdr->frame_control);

	os_memset(&event, 0, sizeof(event));
	event.tx_status.type = WLAN_FC_GET_TYPE(fc);
	event.tx_status.stype = WLAN_FC_GET_STYPE(fc);
	event.tx_status.dst = hdr->addr1;
	event.tx_status.data = frame;
	event.tx_status.data_len = len;
	event.tx_status.ack = drv_msg_buf->data->attr.ack != 0;

	wpa_supplicant_event(drv->ctx, EVENT_TX_STATUS, &event);

	TX_FUNC_END("");
}

void mlme_event_deauth_disassoc(struct wpa_driver_fc80211_data *drv,
				       enum wpa_event_type type,
				       const UCHAR *frame, size_t len)
{
	const struct _ieee80211_mgmt *mgmt;
	union wpa_event_data event;
	const UCHAR *bssid = NULL;
	USHORT reason_code = 0;

	if (type == EVENT_DEAUTH) {
		da16x_drv_prt("[%s] Deauthenticate event\n", __func__);
	} else {
		da16x_drv_prt("[%s] Disassociate event\n", __func__);
	}
	mgmt = (const struct _ieee80211_mgmt *) frame;
	if (len >= 24) {
		bssid = mgmt->bssid;

		if ((drv->capa.flags & WPA_DRIVER_FLAGS_SME) &&
		    !drv->associated &&
		    os_memcmp(bssid, drv->auth_bssid, ETH_ALEN) != 0 &&
		    os_memcmp(bssid, drv->auth_attempt_bssid, ETH_ALEN) != 0 &&
		    os_memcmp(bssid, drv->prev_bssid, ETH_ALEN) == 0) {

			/*
			 * Avoid issues with some roaming cases where
			 * disconnection event for the old AP may show up after
			 * we have started connection with the new AP.
			 */
			da16x_drv_prt("[%s] Ignore deauth/disassoc event from "
					"old AP " MACSTR
					" when already authenticating with "
					MACSTR "\n",
				__func__, MAC2STR(bssid),
				   MAC2STR(drv->auth_attempt_bssid));
			return;
		}

		if (drv->associated != 0 &&
		    os_memcmp(bssid, drv->bssid, ETH_ALEN) != 0 &&
		    os_memcmp(bssid, drv->auth_bssid, ETH_ALEN) != 0) {
			/*
			 * We have presumably received this deauth as a
			 * response to a clear_state_mismatch() outgoing
			 * deauth.  Don't let it take us offline!
			 */
			da16x_drv_prt("[%s] Deauth received from Unknown BSSID "
					MACSTR " -- ignoring\n",
				   __func__, MAC2STR(bssid));
			return;
		}
	}

	fc80211_mark_disconnected(drv);
	os_memset(&event, 0, sizeof(event));

	/* Note: Same offset for Reason Code in both frame subtypes */
	if (len >= 24 + sizeof(mgmt->u.deauth))
		reason_code = le_to_host16(mgmt->u.deauth.reason_code);

	if (type == EVENT_DISASSOC) {
		event.disassoc_info.locally_generated =
			!os_memcmp(mgmt->sa, drv->first_bss->addr, ETH_ALEN);
		event.disassoc_info.addr = bssid;
		event.disassoc_info.reason_code = reason_code;
		if (frame + len > mgmt->u.disassoc.variable) {
			event.disassoc_info.ie = mgmt->u.disassoc.variable;
			event.disassoc_info.ie_len = frame + len -
				mgmt->u.disassoc.variable;
		}
	} else {
		if (drv->ignore_deauth_event) {
			da16x_drv_prt("[%s] Ignore deauth event due to "
					"previous forced deauth-during-auth\n",
					__func__);
			drv->ignore_deauth_event = 0;
			return;
		}
		event.deauth_info.locally_generated =
			!os_memcmp(mgmt->sa, drv->first_bss->addr, ETH_ALEN);
		if (drv->ignore_next_local_deauth) {
			drv->ignore_next_local_deauth = 0;
			if (event.deauth_info.locally_generated) {
				da16x_drv_prt("[%s] Ignore deauth event "
					"triggered due to own deauth request\n",
					__func__);
				return;
			}
			da16x_drv_prt("[%s] Was expecting local deauth "
				"but got another disconnect event first\n",
					__func__);
		}
		event.deauth_info.addr = bssid;
		event.deauth_info.reason_code = reason_code;
		if (frame + len > mgmt->u.deauth.variable) {
			event.deauth_info.ie = mgmt->u.deauth.variable;
			event.deauth_info.ie_len = frame + len -
				mgmt->u.deauth.variable;
		}
	}

	wpa_supplicant_event(drv->ctx, type, &event);
}

#ifdef CONFIG_IEEE80211W
static void mlme_event_unprot_disconnect(struct wpa_driver_fc80211_data *drv,
				       enum wpa_event_type type,
				       const UCHAR *frame, size_t len)
{
	const struct _ieee80211_mgmt *mgmt;
	union wpa_event_data event;
	u16 reason_code = 0;

	if (type == EVENT_UNPROT_DEAUTH) {
		da16x_drv_prt("[%s] : Deauthenticate event\n", __func__);
	} else {
		da16x_drv_prt("[%s] : Disassociate event\n", __func__);
	}

	if (len < 24)
		return;

	mgmt = (const struct _ieee80211_mgmt *) frame;

	os_memset(&event, 0, sizeof(event));
	/* Note: Same offset for Reason Code in both frame subtypes */
	if (len >= 24 + sizeof(mgmt->u.deauth))
		reason_code = le_to_host16(mgmt->u.deauth.reason_code);

	if (type == EVENT_UNPROT_DISASSOC) {
		event.unprot_disassoc.sa = mgmt->sa;
		event.unprot_disassoc.da = mgmt->da;
		event.unprot_disassoc.reason_code = reason_code;
	} else {
		event.unprot_deauth.sa = mgmt->sa;
		event.unprot_deauth.da = mgmt->da;
		event.unprot_deauth.reason_code = reason_code;
	}

	wpa_supplicant_event(drv->ctx, type, &event);
}
#endif
UINT get_fc80211_protocol_features(struct wpa_driver_fc80211_data *drv)
{
	UINT feat = 0;
	int ret = -1;

	/* by Bright : FC80211_CMD_GET_PROTOCOL_FEATURES
	 *	Direct function call on UMAC layer
	 */
#if 0 //[[ trinity_0150331_BEGIN -- tmp
	ret = fc80211_get_protocol_features(drv->ifindex, &feat);
#endif //]] trinity_0150331_END -- tmp
	if (ret == 0)
		return feat;

	return 0;
}

struct softmac_info_data {
	struct wpa_driver_fc80211_data *drv;
	struct wpa_driver_capa *capa;

	unsigned int num_multichan_concurrent;

	unsigned int error:1;
	unsigned int device_ap_sme:1;
	unsigned int poll_command_supported:1;
	unsigned int data_tx_status:1;
	unsigned int monitor_supported:1;
	unsigned int auth_supported:1;
	unsigned int connect_supported:1;
	unsigned int p2p_go_supported:1;
	unsigned int p2p_client_supported:1;
	unsigned int p2p_concurrent:1;
	unsigned int channel_switch_supported:1;
	unsigned int set_qos_map_supported:1;
};

int wpa_driver_fc80211_get_info(struct wpa_driver_fc80211_data *drv,
				       struct softmac_info_data *info)
{
	struct i802_bss *bss = drv->first_bss;
	int	ret;

	TX_FUNC_START("");

	os_memset(info, 0, sizeof(struct softmac_info_data));

	info->capa = &drv->capa;
	info->drv = drv;

	/* by Bright : FC80211_CMD_GET_SOFTMAC
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_get_softmac(bss->wdev_id, drv->ifindex, (void *)info);
	if (ret < 0) {
		da16x_drv_prt("[%s] #1 Error\n", __func__);

		return -1;
	}

	if (info->auth_supported)
		drv->capa.flags |= WPA_DRIVER_FLAGS_SME;
	else if (!info->connect_supported) {
		da16x_drv_prt("[%s] Driver does not support "
			   "auth/assoc commands\n");
		info->error = 1;
	}

	if (info->p2p_go_supported && info->p2p_client_supported)
		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CAPABLE;
	if (info->p2p_concurrent) {
		da16x_drv_prt("[%s] Use separate P2P group "
			   "interface (driver advertised support)\n",
					__func__);

		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CONCURRENT;
		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P;
	}
	if (info->num_multichan_concurrent > 1) {
		da16x_drv_prt("[%s] Enable multi-channel "
			   "concurrent (driver advertised support)\n",
					__func__);

		drv->capa.num_multichan_concurrent =
			info->num_multichan_concurrent;
	}

	/* default to 5000 since early versions of macd11 don't set it */
	if (!drv->capa.max_remain_on_chan)
		drv->capa.max_remain_on_chan = 5000;

	if (info->channel_switch_supported)
		drv->capa.flags |= WPA_DRIVER_FLAGS_AP_CSA;

	TX_FUNC_END("");

	return 0;
}

int wpa_driver_fc80211_capa(struct wpa_driver_fc80211_data *drv)
{
	struct softmac_info_data info;

	TX_FUNC_START("");

	if (wpa_driver_fc80211_get_info(drv, &info)) {
		da16x_drv_prt("[%s] Error #1\n", __func__);
		return -1;
	}

	if (info.error) {
		da16x_drv_prt("[%s] Error #2\n", __func__);

		return -1;
	}

	drv->has_capability = 1;
	drv->capa.key_mgmt = WPA_DRIVER_CAPA_KEY_MGMT_WPA |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
		WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
	drv->capa.auth = WPA_DRIVER_AUTH_OPEN |
		WPA_DRIVER_AUTH_SHARED;

	drv->capa.flags |= WPA_DRIVER_FLAGS_SANE_ERROR_CODES;
	drv->capa.flags |= WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE;
	drv->capa.flags |= WPA_DRIVER_FLAGS_EAPOL_TX_STATUS;

	/*
	 * As all cfgd11 drivers must support cases where the AP interface is
	 * removed without the knowledge of wpa_supplicant/hostapd, e.g., in
	 * case that the user space daemon has crashed, they must be able to
	 * cleanup all stations and key entries in the AP tear down flow. Thus,
	 * this flag can/should always be set for cfgd11 drivers.
	 */
	drv->capa.flags |= WPA_DRIVER_FLAGS_AP_TEARDOWN_SUPPORT;

	if (!info.device_ap_sme) {
		drv->capa.flags |= WPA_DRIVER_FLAGS_DEAUTH_TX_STATUS;

		/*
		 * No AP SME is currently assumed to also indicate no AP MLME
		 * in the driver/firmware.
		 */
		drv->capa.flags |= WPA_DRIVER_FLAGS_AP_MLME;
	}

	drv->device_ap_sme = info.device_ap_sme;
	drv->poll_command_supported = info.poll_command_supported;
	drv->data_tx_status = info.data_tx_status;
	if (info.set_qos_map_supported)
		drv->capa.flags |= WPA_DRIVER_FLAGS_QOS_MAPPING;

	drv->use_monitor = 0;

	if (!info.data_tx_status)
		drv->capa.flags &= ~WPA_DRIVER_FLAGS_EAPOL_TX_STATUS;

	TX_FUNC_END("");

	return 0;
}

void * wpa_driver_fc80211_drv_init(void *ctx, const char *ifname,
					  void *global_priv, int hostapd,
					  const UCHAR *set_addr)
{
	struct wpa_driver_fc80211_data *drv;
	struct i802_bss *bss;
	int errnum;

	TX_FUNC_START("");

	if (global_priv == NULL)
		return NULL;

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;
	drv->global = global_priv;
	drv->ctx = ctx;
	drv->hostapd = !!hostapd;
	drv->num_if_indices = sizeof(drv->default_if_indices) / sizeof(int);
	drv->if_indices = drv->default_if_indices;

	drv->first_bss = os_zalloc(sizeof(*drv->first_bss));
	if (!drv->first_bss) {
		os_free(drv);
		return NULL;
	}
	bss = drv->first_bss;
	bss->drv = drv;
	bss->ctx = ctx;

	os_strlcpy(bss->ifname, ifname, sizeof(bss->ifname));

	drv->ap_scan_as_station = FC80211_IFTYPE_UNSPECIFIED;
	drv->start_iface_up = 1;

	if ((errnum = wpa_driver_fc80211_finish_drv_init(drv, set_addr, 1)) < 0) {
		da16x_err_prt("[%s] finish_drv_init failed. Error = %d\n", __func__, errnum);
		goto failed;
	}

	if (drv->global) {
		dl_list_add(&drv->global->interfaces, &drv->list);
		drv->in_interface_list = 1;
	}

	TX_FUNC_END("");
	return bss;

failed:
	wpa_driver_fc80211_deinit(bss);
	return NULL;
}

/**
 * wpa_driver_fc80211_init - Initialize fc80211 driver interface
 * @ctx: context to be used when calling wpa_supplicant functions,
 * e.g., wpa_supplicant_event()
 * @ifname: interface name, e.g., wlan0
 * @global_priv: private driver global data from global_init()
 * Returns: Pointer to private data, %NULL on failure
 */
void * wpa_driver_fc80211_init(void *ctx, const char *ifname,
				      void *global_priv)
{
	return wpa_driver_fc80211_drv_init(ctx, ifname, global_priv, 0, NULL);
}

int fc80211_register_frame(struct i802_bss *bss,
			  USHORT type, const UCHAR *match, size_t match_len)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;
	char buf[30];

	TX_FUNC_START("");

	buf[0] = '\0';
	wpa_snprintf_hex(buf, sizeof(buf), match, match_len);

	da16x_drv_prt("[%s] Register frame type=0x%x match=%s\n", __func__, type, buf);

	/* by Bright : FC80211_CMD_GET_SOFTMAC
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_register_action(bss->wdev_id_set, drv->ifindex, type,
				match, match_len);
	if (ret) {
		if (ret == -EALREADY) {
			da16x_p2p_prt("[%s] Probe Request is already registered\n",
				     __func__);

			ret = 0;
			goto put_cmd_fail;
		}

		da16x_drv_prt("[%s] Register frame command "
				"failed (type=%u): ret=%d (%s)\n"
				, __func__, type, ret, strerror(-ret));

		da16x_drv_prt("[%s] ERROR #1\n", __func__);
		goto put_cmd_fail;
	}

	ret = 0;

put_cmd_fail :
	TX_FUNC_END("");

	return ret;
}

int fc80211_register_action_frame(struct i802_bss *bss,
					 const UCHAR *match, size_t match_len)
{
	USHORT type = (WLAN_FC_TYPE_MGMT << 2) | (WLAN_FC_STYPE_ACTION << 4);

	return fc80211_register_frame(bss, type, match, match_len);
}

int fc80211_mgmt_subscribe_non_ap(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = 0;

	TX_FUNC_START("");

	if (drv->nlmode == FC80211_IFTYPE_ADHOC) {
		USHORT type = (WLAN_FC_TYPE_MGMT << 2) | (WLAN_FC_STYPE_AUTH << 4);

		/* register for any AUTH message */
		fc80211_register_frame(bss, type, NULL, 0);
	}

#ifdef CONFIG_INTERWORKING
	/* QoS Map Configure */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x01\x04", 2) < 0)
		ret = -1;
#endif /* CONFIG_INTERWORKING */
#if defined(CONFIG_P2P) || defined(CONFIG_INTERWORKING)
#ifdef	CONFIG_GAS
	/* GAS Initial Request */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x04\x0a", 2) < 0)
		ret = -1;
	/* GAS Initial Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x04\x0b", 2) < 0)
		ret = -1;
	/* GAS Comeback Request */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x04\x0c", 2) < 0)
		ret = -1;
	/* GAS Comeback Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x04\x0d", 2) < 0)
		ret = -1;
	/* Protected GAS Initial Request */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x09\x0a", 2) < 0)
		ret = -1;
	/* Protected GAS Initial Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x09\x0b", 2) < 0)
		ret = -1;
	/* Protected GAS Comeback Request */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x09\x0c", 2) < 0)
		ret = -1;
	/* Protected GAS Comeback Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x09\x0d", 2) < 0)
		ret = -1;
#endif	/* CONFOG_GAS */
#endif	/* CONFIG_P2P || CONFIG_INTERWORKING */

#ifdef CONFIG_P2P
	/* Run this register_action for just p2p operations */
	if (   get_run_mode() == P2P_ONLY_MODE
#ifdef	CONFIG_CONCURRENT
	    || get_run_mode() == STA_P2P_CONCURRENT_MODE
#endif	/* CONFIG_CONCURRENT */
	    || get_run_mode() == P2P_GO_FIXED_MODE) {
		/* P2P Public Action */
		if (fc80211_register_action_frame(bss,
					(UCHAR *) "\x04\x09\x50\x6f\x9a\x09",
					6) < 0) {
			ret = -1;
		}
		/* P2P Action */
		if (fc80211_register_action_frame(bss,
					(UCHAR *) "\x7f\x50\x6f\x9a\x09",
					5) < 0) {
			ret = -1;
		}
	}
#endif /* CONFIG_P2P */

#ifdef CONFIG_IEEE80211W
	/* SA Query Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x08\x01", 2) < 0)
		ret = -1;
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_WNM_BSS_TRANS_MGMT
	/* WNM - BSS Transition Management Request */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x0a\x07", 2) < 0)
		ret = -1;
#endif /* CONFIG_WNM_BSS_TRANS_MGMT */
#ifdef CONFIG_WNM_SLEEP_MODE
	/* WNM-Sleep Mode Response */
	if (fc80211_register_action_frame(bss, (UCHAR *) "\x0a\x11", 2) < 0)
		ret = -1;
#endif /* CONFIG_WNM_SLEEP_MODE */

	TX_FUNC_END("");

	return ret;
}

#ifdef CONFIG_MESH
static int fc80211_mgmt_subscribe_mesh(struct i802_bss *bss)
{
	int ret = 0;
	
	TX_FUNC_START("");

#if 0
	if (nld11_alloc_mgmt_handle(bss))
		return -1;
#endif
	wpa_printf_dbg(MSG_DEBUG,
		   "nld11: Subscribe to mgmt frames with mesh handle");

	/* Auth frames for mesh SAE */
#if 1	
	{
		USHORT type = (WLAN_FC_TYPE_MGMT << 2) | (WLAN_FC_STYPE_AUTH << 4);

		/* register for any AUTH message */
		fc80211_register_frame(bss, type, NULL, 0);
	}
#else	
	if (fc80211_register_frame(bss, bss->nl_mgmt,
				   (WLAN_FC_TYPE_MGMT << 2) |
				   (WLAN_FC_STYPE_AUTH << 4),
				   NULL, 0) < 0)
		ret = -1;
#endif
	/* Mesh peering open */
	if (fc80211_register_action_frame(bss, (u8 *) "\x0f\x01", 2) < 0)
		ret = -1;
	/* Mesh peering confirm */
	if (fc80211_register_action_frame(bss, (u8 *) "\x0f\x02", 2) < 0)
		ret = -1;
	/* Mesh peering close */
	if (fc80211_register_action_frame(bss, (u8 *) "\x0f\x03", 2) < 0)
		ret = -1;
#if 0
	nld11_mgmt_handle_register_eloop(bss);
#endif
	TX_FUNC_END("");

	return ret;
}
#endif

int fc80211_register_spurious_class3(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;

	/* by Bright : FC80211_CMD_UNEXPECTED_FRAME
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_unexpected_frame(drv->ifindex);
	if (ret) {
		da16x_drv_prt("[%s] Register spurious class3 failed: ret=%d(%s)\n",
			   __func__, ret, strerror(-ret));

		goto put_cmd_fail;
	}
	ret = 0;

put_cmd_fail :
	return ret;
}

#ifdef	CONFIG_AP
int fc80211_mgmt_subscribe_ap(struct i802_bss *bss)
{
	static const int stypes[] = {
		WLAN_FC_STYPE_AUTH,
		WLAN_FC_STYPE_ASSOC_REQ,
		WLAN_FC_STYPE_REASSOC_REQ,
		WLAN_FC_STYPE_DISASSOC,
		WLAN_FC_STYPE_DEAUTH,
		WLAN_FC_STYPE_ACTION,
		WLAN_FC_STYPE_PROBE_REQ,
/* Beacon doesn't work as macd11 doesn't currently allow
 * it, but it wouldn't really be the right thing anyway as
 * it isn't per interface ... maybe just dump the scan
 * results periodically for OLBC?
 */
		/* WLAN_FC_STYPE_BEACON, */
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(stypes); i++) {
		if (fc80211_register_frame(bss,
					   (WLAN_FC_TYPE_MGMT << 2) |
					   (stypes[i] << 4),
					   NULL, 0) < 0) {
			goto out_err;
		}
	}

	if (fc80211_register_spurious_class3(bss))
		goto out_err;

	if (fc80211_get_softmac_data_ap(bss) == NULL)
		goto out_err;

	return 0;

out_err:
	return -1;
}

int fc80211_mgmt_subscribe_ap_dev_sme(struct i802_bss *bss)
{
	if (fc80211_register_frame(bss,
				   (WLAN_FC_TYPE_MGMT << 2) |
				   (WLAN_FC_STYPE_ACTION << 4),
				   NULL, 0) < 0)
		goto out_err;

	return 0;

out_err:
	return -1;
}
#endif	/* CONFIG_AP */

void fc80211_mgmt_unsubscribe(struct i802_bss *bss, const char *reason)
{
	TX_FUNC_START("");

	da16x_drv_prt("[%s] reason=<%s>\n", __func__, reason);

	fc80211_put_softmac_data_ap(bss);

	TX_FUNC_END("");
}

#ifdef	CONFIG_P2P
void fc80211_del_p2pdev(struct i802_bss *bss)
{
#ifdef	ENABLE_P2P_DBG
	int ret = -1;

	/* by Bright : FC80211_CMD_DEL_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_del_interface(bss->wdev_id, -1);

	da16x_p2p_prt("[%s] Delete P2P Device %s (0x%llx): %d\n",
			__func__,
		   bss->ifname, (long long unsigned int) bss->wdev_id, ret);
#else
	/* by Bright : FC80211_CMD_DEL_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	da16x_p2p_prt("[%s] Delete P2P Device %s (0x%llx)\n",
			__func__,
		   bss->ifname, (long long unsigned int) bss->wdev_id);
#endif	/* ENABLE_P2P_DBG */
}

int fc80211_set_p2pdev(struct i802_bss *bss, int start)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;

	if (start)
		ret = fc80211_start_p2p_dev(bss->wdev_id, drv->ifindex);
	else
		ret = fc80211_stop_p2p_dev(bss->wdev_id, drv->ifindex);

	da16x_p2p_prt("[%s] %s P2P Device %s (0x%llx): %s\n",
		__func__,
		   start ? "Start" : "Stop",
		   bss->ifname, (long long unsigned int) bss->wdev_id,
		   strerror(-ret));

	return ret;
}
#endif	/* CONFIG_P2P */

int i802_set_iface_flags(struct i802_bss *bss, int up)
{
	enum fc80211_iftype nlmode;

	nlmode = fc80211_get_ifmode(bss);

	if (nlmode != FC80211_IFTYPE_P2P_DEVICE) {
da16x_drv_prt("[%s] What do I do for set_iface\n", __func__);
		return 0;
	}

#ifdef	CONFIG_P2P
	/* P2P Device has start/stop which is equivalent */
	return fc80211_set_p2pdev(bss, up);
#else
	return 0;
#endif	/* 0 */
}

int
wpa_driver_fc80211_finish_drv_init(struct wpa_driver_fc80211_data *drv,
				   const UCHAR *set_addr, int first)
{
	struct i802_bss *bss = drv->first_bss;
	enum fc80211_iftype iface_type;

	TX_FUNC_START("");

	if (os_strcmp(bss->ifname, SOFTAP_DEVICE_NAME) == 0) {
		drv->ifindex = 1;
		drv->hostapd = 1;
	} else if (os_strcmp(bss->ifname, P2P_DEVICE_NAME) == 0) {
		drv->ifindex = 1;
	} else if (os_strcmp(bss->ifname, MESH_POINT_DEVICE_NAME) == 0) {
		drv->ifindex = 1;
	} else {
		drv->ifindex = 0;
	}

	da16x_drv_prt("[%s] ifidx=%d (%s)\n" , __func__, drv->ifindex, bss->ifname);

	bss->ifindex = drv->ifindex;
	bss->wdev_id = drv->global->if_add_wdevid;
	bss->wdev_id_set = drv->global->if_add_wdevid_set;

	bss->if_dynamic = drv->ifindex == drv->global->if_add_ifindex;
	bss->if_dynamic = bss->if_dynamic || drv->global->if_add_wdevid_set;
	drv->global->if_add_wdevid_set = 0;

	if (wpa_driver_fc80211_capa(drv) < 0) {
		da16x_drv_prt("[%s] ERROR return ...\n", __func__);
		return -1;
	}

	da16x_drv_prt("[%s] interface=[%s], phy=[%s]\n",
		   __func__, bss->ifname, drv->phyname);

	if (first && fc80211_get_ifmode(bss) == FC80211_IFTYPE_AP)
		drv->start_mode_ap = 1;

	if (drv->hostapd)
		iface_type = FC80211_IFTYPE_AP;
	else if (bss->if_dynamic)
		iface_type = fc80211_get_ifmode(bss);
	else
		iface_type = FC80211_IFTYPE_STATION;

	da16x_drv_prt("[%s] iface_type=%d\n", __func__, iface_type);

	if (wpa_driver_fc80211_set_mode(bss, iface_type) < 0) {
		da16x_drv_prt("[%s] Could not configure "
				"driver mode\n", __func__);
		return -2;
	}

	/* by Bright :
	 *	Have to get mac_addr from UMAC all kind of device type
	 */
	fc80211_get_macaddr(bss);

	TX_FUNC_END("");

	return 0;
}

int wpa_driver_fc80211_del_beacon(struct wpa_driver_fc80211_data *drv)
{
	int ret = -1;

	TX_FUNC_START("");

	ret = fc80211_del_beacon(drv->ifindex);
	return ret;
}

/**
 * wpa_driver_fc80211_deinit - Deinitialize fc80211 driver interface
 * @bss: Pointer to private fc80211 data from wpa_driver_fc80211_init()
 *
 * Shut down driver interface and processing of driver events. Free
 * private data buffer if one was allocated in wpa_driver_fc80211_init().
 */
void wpa_driver_fc80211_deinit(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;

	bss->in_deinit = 1;

	da16x_drv_prt("[%s] : What do I do for Bridge opertion ?\n", __func__);

	if (is_ap_interface(drv->nlmode))
		wpa_driver_fc80211_del_beacon(drv);

	if (drv->if_indices != drv->default_if_indices)
		os_free(drv->if_indices);

	if (drv->disabled_11b_rates)
		fc80211_disable_11b_rates(drv, drv->ifindex, 0);

	if (!drv->start_iface_up)
		(void) i802_set_iface_flags(bss, 0);

	if (drv->nlmode != FC80211_IFTYPE_P2P_DEVICE) {
		if (!drv->hostapd || !drv->start_mode_ap)
			wpa_driver_fc80211_set_mode(bss,
						    FC80211_IFTYPE_STATION);
		fc80211_mgmt_unsubscribe(bss, "deinit");
	}
#ifdef	CONFIG_P2P
	else {
		fc80211_mgmt_unsubscribe(bss, "deinit");
		fc80211_del_p2pdev(bss);
	}
#endif	/* CONFIG_P2P */

	os_free(drv->filter_ssids);

	os_free(drv->auth_ie);

	if (drv->in_interface_list) {
		dl_list_del(&drv->list);
	}

	os_free(drv->extended_capa);
	os_free(drv->extended_capa_mask);
	os_free(drv->first_bss);
	os_free(drv);
}

/**
 * wpa_driver_fc80211_scan_timeout - Scan timeout to report scan completion
 * @eloop_ctx: Driver private data
 * @timeout_ctx: ctx argument given to wpa_driver_fc80211_init()
 *
 * This function can be used as registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
void wpa_driver_fc80211_scan_timeout(int timeout,
					void *da16x_drv, void *timeout_ctx)
{
	struct wpa_driver_fc80211_data *drv = da16x_drv;
	UINT	status = ERR_OK;

	extern int da16x_scan_results_timer(void *timeout_ctx, int timeout);

	if (drv->ap_scan_as_station != FC80211_IFTYPE_UNSPECIFIED) {
		wpa_driver_fc80211_set_mode(drv->first_bss,
					    drv->ap_scan_as_station);
		drv->ap_scan_as_station = FC80211_IFTYPE_UNSPECIFIED;
	}

	da16x_drv_prt("[%s] da16x_scan_results_timer create call\n",
								__func__);

#ifndef ENABLE_SCAN_ON_AP_MODE
	if(drv->nlmode != FC80211_IFTYPE_AP)
#endif /* ! ENABLE_SCAN_ON_AP_MODE */
		status = da16x_scan_results_timer((void *)timeout_ctx, timeout);

	if (status != ERR_OK) {
		da16x_drv_prt("[%s] da16x_scan_results_timer create fail\n",
				__func__);
	}
}

int wpa_driver_fc80211_abort_scan(struct i802_bss *bss,
				   struct wpa_driver_scan_params *params)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;

	TX_FUNC_START("");

	ret = fc80211_abort_scan(bss->wdev_id, drv->ifindex);
	if (ret) {
		da16x_drv_prt("[%s] Scan abort failed: ret=%d\n",
					__func__, ret);
	}

	TX_FUNC_END("");

	return ret;
}

/**
 * wpa_driver_fc80211_scan - Request the driver to initiate scan
 * @bss: Pointer to private driver data from wpa_driver_fc80211_init()
 * @params: Scan parameters
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_fc80211_scan(struct i802_bss *bss,
				   struct wpa_driver_scan_params *params)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1, timeout;

	TX_FUNC_START("");

	drv->scan_for_auth = 0;

	/* by Bright : FC80211_CMD_TRIGGER_SCAN
	 *	Direct function call on UMAC layer
	 */
	os_free(drv->filter_ssids);
        drv->filter_ssids = params->filter_ssids;
        params->filter_ssids = NULL;
        drv->num_filter_ssids = params->num_filter_ssids;

	ret = fc80211_trigger_scan(bss->wdev_id, drv->ifindex, params);

	if (ret) {
		da16x_err_prt("[%s] Scan trigger failed: ret=%d\n", __func__, ret);

        if(ret == -EBUSY)
            fc80211_abort_scan(bss->wdev_id, drv->ifindex);
        
		if (drv->hostapd && is_ap_interface(drv->nlmode)) {
			enum fc80211_iftype old_mode = drv->nlmode;

			/*
			 * macd11 does not allow scan requests in AP mode, so
			 * try to do this in station mode.
			 */
			if (wpa_driver_fc80211_set_mode(
				    bss, FC80211_IFTYPE_STATION))
				goto put_cmd_fail;

			if (wpa_driver_fc80211_scan(bss, params)) {
				wpa_driver_fc80211_set_mode(bss, drv->nlmode);
				goto put_cmd_fail;
			}

			/* Restore AP mode when processing scan results */
			drv->ap_scan_as_station = old_mode;
			ret = 0;
		} else
			goto put_cmd_fail;
	}

	drv->scan_state = SCAN_REQUESTED;

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	timeout = 10;
	if (drv->scan_complete_events) {
		/*
		 * The driver seems to deliver events to notify when scan is
		 * complete, so use longer timeout to avoid race conditions
		 * with scanning and following association request.
		 */
		timeout = 30;
	}
#if 0
	da16x_drv_prt("[%s] Scan requested (ret=%d) - scan timeout %d sec\n",
				__func__, ret, timeout);
#endif
	if ( passive_scan_feature_flag == pdTRUE ){
		if(params->passive_scan_flag){
			if(params->passive_duration > 1000000)
				timeout = (params->passive_duration / 1000000) * 13 + 10;
		}
		//PRINTF("timeout = %d\n", timeout);
	}
	wpa_driver_fc80211_scan_timeout(timeout, drv, drv->ctx);

put_cmd_fail :
	TX_FUNC_END("");

	return ret;
}

struct wpa_scan_results *
fc80211_get_scan_results(struct wpa_driver_fc80211_data *drv)
{
	struct wpa_scan_results *res=NULL;
	struct fc80211_bss_info_buf buf;
	struct i802_bss *bss = drv->first_bss;

	TX_FUNC_START("");

	os_memset(&buf, 0, sizeof(buf));

	res = os_zalloc(sizeof(*res));
	if (res == NULL)
		return NULL;

	buf.drv = drv;
	buf.res = res;

	/* by Bright : FC80211_CMD_GET_SCAN
	 *	Direct function call on UMAC layer
	 */
	fc80211_get_scan(bss->wdev_id, drv->ifindex, (void *)&buf);
	da16x_drv_prt("[%s] : Rx results = (%lu BSSes)\n",
				__func__, (unsigned long) res->num);

	da16x_drv_prt("<%s> FINISH #1\n", __func__);

	return res;
}

/**
 * wpa_driver_fc80211_get_scan_results - Fetch the latest scan results
 * @priv: Pointer to private wext data from wpa_driver_fc80211_init()
 * Returns: Scan results on success, -1 on failure
 */
struct wpa_scan_results *
wpa_driver_fc80211_get_scan_results(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct wpa_scan_results *res=NULL;

	RX_FUNC_START("");

	res = fc80211_get_scan_results(drv);

	RX_FUNC_END("");

	return res;
}

int wpa_driver_fc80211_free_umac_scan_results(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	RX_FUNC_START("");

	da16x_drv_prt("[%s] \n", __func__);

	fc80211_free_umac_scan_results(bss->wdev_id, drv->ifindex);

	RX_FUNC_END("");

	return 0;
}


void fc80211_dump_scan(struct wpa_driver_fc80211_data *drv)
{
	struct wpa_scan_results *res;
	size_t i;

	res = fc80211_get_scan_results(drv);
	if (res == NULL) {
		da16x_drv_prt("[%s] Failed to get scan results\n", __func__);
		return;
	}

	da16x_drv_prt("[%s] Scan result dump\n", __func__);

	for (i = 0; i < res->num; i++) {
#ifdef	ENABLE_DRV_DBG
		struct wpa_scan_res *r = res->res[i];
#endif	/* ENABLE_DRV_DBG */

		da16x_drv_prt("\t%d/%d " MACSTR "%s%s\n",
			   (int) i, (int) res->num, MAC2STR(r->bssid),
			   r->flags & WPA_SCAN_AUTHENTICATED ? " [auth]" : "",
			   r->flags & WPA_SCAN_ASSOCIATED ? " [assoc]" : "");
	}

	wpa_scan_results_free(res);
}

int wpa_driver_fc80211_set_key(const char *ifname, struct i802_bss *bss,
				      int alg, const UCHAR *addr,
				      int key_idx, int set_tx,
				      const UCHAR *seq, size_t seq_len,
				      const UCHAR *key, size_t key_len)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -ENODATA;

	TX_FUNC_START("");

	/* Ignore for P2P Device */
	if (drv->nlmode == FC80211_IFTYPE_P2P_DEVICE)
		return 0;

	da16x_drv_prt("[%s] ifname=(%s) alg=%d addr=" MACSTR " key_idx=%d "
		   "set_tx=%d seq_len=%lu key_len=%lu\n",
		   __func__, ifname, alg, MAC2STR(addr), key_idx, set_tx,
		   (unsigned long) seq_len, (unsigned long) key_len);

	if (alg == WPA_ALG_NONE) {
		/* by Bright : FC80211_CMD_DEL_KEY
	 	 *	Direct function call on UMAC layer
	 	 */
		if (chk_dpm_wakeup() == 0) {
			ret = fc80211_del_key(drv->ifindex, addr, seq, seq_len, key_idx, set_tx);
		}
	} else {
		/*if (addr == NULL)
			addr = drv->bssid;*/

		/* by Bright : FC80211_CMD_NEW_KEY
	 	 *	Direct function call on UMAC layer
	 	 */
		ret = fc80211_new_key(drv->ifindex, addr,
				      key, key_len, seq, seq_len,
				      key_idx, set_tx, alg);
	}

	if (ret == -ENOMEM) {
		da16x_err_prt("[%s] set_key failed; %s\n", __func__, strerror(-ret));
		return ret;
	}

	if ((ret == -ENOENT || ret == -ENOLINK) && alg == WPA_ALG_NONE)
		ret = 0;

	if (ret)
		da16x_drv_prt("[%s] set_key failed; %s\n", __func__, strerror(-ret));

	/*
	 * If we failed or don't need to set the default TX key (below),
	 * we're done here.
	 */
#if 0	/* by Bright : Code sync with Supp 2.4 */
	if (ret || !set_tx || alg == WPA_ALG_NONE){
		if(ret) // if set_tx is disabled or wpa_alg has none, don't log.
			PRINTF("[%s] Error (line %d) ret = %d\n",__func__,__LINE__ , ret);
		return ret;
	}
#else
	if (ret || !set_tx || alg == WPA_ALG_NONE)
		return ret;
#endif	/* 0 */

	if (is_ap_interface(drv->nlmode) && addr &&
	    !is_broadcast_ether_addr(addr)) {
		da16x_drv_prt("[%s] END #1\n", __func__);
		return ret;
	}

	/* by Bright : FC80211_CMD_SET_KEY
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_set_key(drv->ifindex, addr, alg, key_idx);
	if (ret == -ENOENT)
		ret = 0;

	if (ret) {
		da16x_err_prt("[%s] set_key default failed: err=%d)\n",__func__, ret);
	}

	TX_FUNC_END("");

	return ret;
}

int fc80211_set_conn_keys(struct wpa_driver_associate_params *params,
				int *privacy)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (!params->wep_key[i])
			continue;
		*privacy = 1;
		break;
	}
	if (params->wps == WPS_MODE_PRIVACY)
		*privacy = 1;
	if (params->pairwise_suite &&
	    params->pairwise_suite != WPA_CIPHER_NONE)
		*privacy = 1;

	if (!(*privacy))
		return 0;

	return 0;
}

int wpa_driver_fc80211_mlme(struct wpa_driver_fc80211_data *drv,
			   const UCHAR *addr, int cmd, USHORT reason_code,
			   int local_state_change)
{
	int ret = -1;

	/* by Bright : FC80211_CMD_DEAUTHENTICATE
	 *	Direct function call on UMAC layer
         * trinity.song : add switch for command
	 */
	switch(cmd) {
	case FC80211_CMD_DEAUTHENTICATE:
	ret = fc80211_deauthenticate(drv->ifindex,
					addr,
					reason_code,
					local_state_change);
		break;
	default:
		da16x_drv_prt("[%s] Unknown MLME command\n", __func__, cmd);
	}

	if (ret) {
		da16x_drv_prt("[%s] MLME cmd failed: reason=%u ret=%d (%s)\n",
			__func__, reason_code, ret, strerror(-ret));
		goto put_cmd_fail;
	}

	ret = 0;

put_cmd_fail :
	return ret;
}

int wpa_driver_fc80211_disconnect(struct wpa_driver_fc80211_data *drv,
					 int reason_code)
{
	int ret = -1;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] reason_code=%d\n", __func__, reason_code);

	fc80211_mark_disconnected(drv);

	/* Disconnect command doesn't need BSSID - it uses cached value */

	/*
	 * For locally generated disconnect, supplicant already generates a
	 * DEAUTH event, so ignore the event from FC80211.
	 */
	drv->ignore_next_local_disconnect = ret == 0;

	TX_FUNC_END("");

	return ret;
}

int wpa_driver_fc80211_deauthenticate(struct i802_bss *bss,
					     const UCHAR *addr, int reason_code)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	if (!(drv->capa.flags & WPA_DRIVER_FLAGS_SME))
		return wpa_driver_fc80211_disconnect(drv, reason_code);

	da16x_drv_prt("[%s] addr=" MACSTR ", reason_code=%d\n",
		   	__func__, MAC2STR(addr), reason_code);

	fc80211_mark_disconnected(drv);
	ret = wpa_driver_fc80211_mlme(drv, addr, FC80211_CMD_DEAUTHENTICATE,
				      reason_code, 0);
	/*
	 * For locally generated deauthenticate, supplicant already generates a
	 * DEAUTH event, so ignore the event from FC80211.
	 */
	drv->ignore_next_local_deauth = ret == 0;

	return ret;
}

void fc80211_copy_auth_params(struct wpa_driver_fc80211_data *drv,
				     struct wpa_driver_auth_params *params)
{
	int i;

	drv->auth_freq = params->freq;
	drv->auth_alg = params->auth_alg;
	drv->auth_wep_tx_keyidx = params->wep_tx_keyidx;
	drv->auth_local_state_change = params->local_state_change;
	drv->auth_p2p = params->p2p;

	if (params->bssid)
		os_memcpy(drv->auth_bssid_, params->bssid, ETH_ALEN);
	else
		os_memset(drv->auth_bssid_, 0, ETH_ALEN);

	if (params->ssid) {
		os_memcpy(drv->auth_ssid, params->ssid, params->ssid_len);
		drv->auth_ssid_len = params->ssid_len;
	} else
		drv->auth_ssid_len = 0;


	os_free(drv->auth_ie);
	drv->auth_ie = NULL;
	drv->auth_ie_len = 0;
	if (params->ie) {
		drv->auth_ie = os_malloc(params->ie_len);
		if (drv->auth_ie) {
			os_memcpy(drv->auth_ie, params->ie, params->ie_len);
			drv->auth_ie_len = params->ie_len;
		}
	}

	for (i = 0; i < 4; i++) {
		if (params->wep_key[i] && params->wep_key_len[i] &&
		    params->wep_key_len[i] <= 16) {
			os_memcpy(drv->auth_wep_key[i], params->wep_key[i],
				  params->wep_key_len[i]);
			drv->auth_wep_key_len[i] = params->wep_key_len[i];
		} else
			drv->auth_wep_key_len[i] = 0;
	}
}


#if 1
#include "fc80211_copy.h"

const char * wpa_auth_type_txt(enum fc80211_auth_type type)
{
	switch (type) {
		case FC80211_AUTHTYPE_OPEN_SYSTEM:
			return "OPEN_SYSTEM";
		case FC80211_AUTHTYPE_SHARED_KEY:
			return "SHARED_KEY";
		case FC80211_AUTHTYPE_FT:
			return "FT";
		case FC80211_AUTHTYPE_NETWORK_EAP:
			return "EAP";
		case FC80211_AUTHTYPE_SAE:
			return "SAE";
		default:
			return "UNKNOWN";
	}
}
#endif /* 1 */

int wpa_driver_fc80211_auth(
	struct i802_bss *bss, struct wpa_driver_auth_params *params)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;
	enum fc80211_auth_type type;
	enum fc80211_iftype nlmode;
	int count = 0;
	int is_retry;

	TX_FUNC_START("");

	is_retry = drv->retry_auth;
	drv->retry_auth = 0;
	drv->ignore_deauth_event = 0;

	fc80211_mark_disconnected(drv);
	os_memset(drv->auth_bssid, 0, ETH_ALEN);
	if (params->bssid)
		os_memcpy(drv->auth_attempt_bssid, params->bssid, ETH_ALEN);
	else
		os_memset(drv->auth_attempt_bssid, 0, ETH_ALEN);

	/* FIX: IBSS mode */
	nlmode = params->p2p ?
		FC80211_IFTYPE_P2P_CLIENT : FC80211_IFTYPE_STATION;

	if (drv->nlmode != nlmode &&
	    wpa_driver_fc80211_set_mode(bss, nlmode) < 0)
		return -1;

retry:
	da16x_drv_prt("[%s] Authenticate (ifindex=%d)\n", __func__, drv->ifindex);

	if (params->bssid) {
		da16x_drv_prt("  * bssid : " MACSTR "\n",
			   MAC2STR(params->bssid));
	}
	if (params->freq) {
		da16x_drv_prt("  * freq : %d\n", params->freq);
	}
	if (params->ssid) {
		da16x_dump("  * SSID", params->ssid, params->ssid_len);
	}

	da16x_dump("  * IEs", params->ie, params->ie_len);

	if (params->auth_alg & WPA_AUTH_ALG_OPEN)
		type = FC80211_AUTHTYPE_OPEN_SYSTEM;
	else if (params->auth_alg & WPA_AUTH_ALG_SHARED)
		type = FC80211_AUTHTYPE_SHARED_KEY;
	else if (params->auth_alg & WPA_AUTH_ALG_LEAP)
		type = FC80211_AUTHTYPE_NETWORK_EAP;
	else if (params->auth_alg & WPA_AUTH_ALG_FT)
		type = FC80211_AUTHTYPE_FT;
	else if (params->auth_alg & WPA_AUTH_ALG_SAE)
		type = FC80211_AUTHTYPE_SAE;
	else
		goto put_cmd_fail;


	da16x_drv_prt("  * Auth Type = %s (%d)\n", wpa_auth_type_txt(type), type);

	if (params->auth_data) {
		wpa_hexdump(MSG_DEBUG, "  * auth_data", params->auth_data,
			    params->auth_data_len);
#if 0 // supplicant 2.7
		if (nla_put(msg, NLD11_ATTR_SAE_DATA, params->auth_data_len,
			    params->auth_data))
			goto fail;
#endif /* 0 */
	}

	if (params->local_state_change) {
		da16x_drv_prt("  * Local state change only\n");
	}

	if (type == FC80211_AUTHTYPE_SHARED_KEY) {
		fc80211_new_key(drv->ifindex, NULL,
				params->wep_key[params->wep_tx_keyidx],
		    		params->wep_key_len[params->wep_tx_keyidx],
		    		NULL, 0, params->wep_tx_keyidx, 1, WPA_ALG_WEP);
	}

	/* by Bright : FC80211_CMD_AUTHENTICATE
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_authenticate(drv->ifindex, params, type);

	if (ret) {
		da16x_debug_prt("[%s] MLME command failed (auth): ret=%d (%s)\n",
					__func__, ret, strerror(-ret));
		count++;
		if (ret == -EALREADY && count == 1 && params->bssid &&
		    !params->local_state_change) {
			/*
			 * macd11 does not currently accept new
			 * authentication if we are already authenticated. As a
			 * workaround, force deauthentication and try again.
			 */
			da16x_debug_prt("[%s] Retry authentication after "
					"forced deauthentication\n", __func__);

			drv->ignore_deauth_event = 1;
			wpa_driver_fc80211_deauthenticate(
				bss, params->bssid,
				WLAN_REASON_PREV_AUTH_NOT_VALID);
			goto retry;
		}

		if (ret == -ENOENT && params->freq && !is_retry) {
			/*
			 * cfgd11 has likely expired the BSS entry even
			 * though it was previously available in our internal
			 * BSS table. To recover quickly, start a single
			 * channel scan on the specified channel.
			 */
			struct wpa_driver_scan_params scan;
			int freqs[2];

			os_memset(&scan, 0, sizeof(scan));
			scan.num_ssids = 1;
			if (params->ssid) {
				scan.ssids[0].ssid = params->ssid;
				scan.ssids[0].ssid_len = params->ssid_len;
			}
			freqs[0] = params->freq;
			freqs[1] = 0;
			scan.freqs = freqs;
			da16x_drv_prt("[%s] Trigger single channel scan to refresh "
				"cfgd11 BSS entry\n", __func__);

			ret = wpa_driver_fc80211_scan(bss, &scan);
			if (ret == 0) {
				fc80211_copy_auth_params(drv, params);
				drv->scan_for_auth = 1;
			}
		} else if (is_retry) {
			/*
			 * Need to indicate this with an event since the return
			 * value from the retry is not delivered to core code.
			 */
			union wpa_event_data event;

			da16x_drv_prt("[%s] Auth retry failed\n", __func__);

			os_memset(&event, 0, sizeof(event));
			os_memcpy(event.timeout_event.addr, drv->auth_bssid_,
				  ETH_ALEN);
			wpa_supplicant_event(drv->ctx, EVENT_AUTH_TIMED_OUT,
					     &event);
		}

		goto put_cmd_fail;
	}
#ifdef FIXED_ISSUES_LOCAL_DEAUTH //20151104 by Jin
	drv->ignore_next_local_deauth = 0;
#endif /* FIXED_ISSUES_LOCAL_DEAUTH */
	ret = 0;

	da16x_drv_prt("[%s] Authentication request send successfully\n", __func__);

put_cmd_fail :
	TX_FUNC_END("");

	return ret;
}

struct phy_info_arg {
	USHORT *num_modes;
	struct hostapd_hw_modes *modes;
	int last_mode, last_chan_idx;
};

struct hostapd_hw_modes *
wpa_driver_fc80211_postprocess_modes(struct hostapd_hw_modes *modes,
				     USHORT *num_modes)
{
	USHORT m;
	struct hostapd_hw_modes *mode11g = NULL, *nmodes, *mode;
	int i, mode11g_idx = -1;

	/* heuristic to set up modes */
	for (m = 0; m < *num_modes; m++) {
		if (!modes[m].num_channels)
			continue;
		if (modes[m].channels[0].freq < 4000) {
			modes[m].mode = HOSTAPD_MODE_IEEE80211B;
			for (i = 0; i < modes[m].num_rates; i++) {
				if (modes[m].rates[i] > 200) {
					modes[m].mode = HOSTAPD_MODE_IEEE80211G;
					break;
				}
			}
		} else if (modes[m].channels[0].freq > 50000)
			modes[m].mode = HOSTAPD_MODE_IEEE80211AD;
		else
			modes[m].mode = HOSTAPD_MODE_IEEE80211A;
	}

	/* If only 802.11g mode is included, use it to construct matching
	 * 802.11b mode data. */

	for (m = 0; m < *num_modes; m++) {
		if (modes[m].mode == HOSTAPD_MODE_IEEE80211B)
			return modes; /* 802.11b already included */
		if (modes[m].mode == HOSTAPD_MODE_IEEE80211G)
			mode11g_idx = m;
	}

	if (mode11g_idx < 0)
		return modes; /* 2.4 GHz band not supported at all */

	nmodes = os_realloc_array(modes, *num_modes + 1, sizeof(*nmodes));
	if (nmodes == NULL)
		return modes; /* Could not add 802.11b mode */

	mode = &nmodes[*num_modes];
	os_memset(mode, 0, sizeof(*mode));
	(*num_modes)++;
	modes = nmodes;

	mode->mode = HOSTAPD_MODE_IEEE80211B;

	mode11g = &modes[mode11g_idx];
	mode->num_channels = mode11g->num_channels;
	mode->channels = os_malloc(mode11g->num_channels *
				   sizeof(struct hostapd_channel_data));
	if (mode->channels == NULL) {
		(*num_modes)--;
		return modes; /* Could not add 802.11b mode */
	}
	os_memcpy(mode->channels, mode11g->channels,
		  mode11g->num_channels * sizeof(struct hostapd_channel_data));

	mode->num_rates = 0;
	mode->rates = os_malloc(4 * sizeof(int));
	if (mode->rates == NULL) {
		os_free(mode->channels);
		(*num_modes)--;
		return modes; /* Could not add 802.11b mode */
	}

	for (i = 0; i < mode11g->num_rates; i++) {
		if (mode11g->rates[i] != 10 && mode11g->rates[i] != 20 &&
		    mode11g->rates[i] != 55 && mode11g->rates[i] != 110)
			continue;
		mode->rates[mode->num_rates] = mode11g->rates[i];
		mode->num_rates++;
		if (mode->num_rates == 4)
			break;
	}

	if (mode->num_rates == 0) {
		os_free(mode->channels);
		os_free(mode->rates);
		(*num_modes)--;
		return modes; /* No 802.11b rates */
	}

	da16x_drv_prt("[%s] Added 802.11b mode based on 802.11g information\n",
					__func__);

	return modes;
}

struct hostapd_hw_modes *
wpa_driver_fc80211_get_hw_feature_data(void *priv, USHORT *num_modes, USHORT *flags)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct phy_info_arg result = {
		.num_modes = num_modes,
		.modes = NULL,
		.last_mode = -1,
	};
	int ret;

	*num_modes = 0;
	*flags = 0;

	get_fc80211_protocol_features(drv);

	/* by Bright : FC80211_CMD_GET_SOFTMAC
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_get_softmac(bss->wdev_id, drv->ifindex, &result);
	if (ret == 0) {
		struct hostapd_hw_modes *hw_mode;

#if 0 //[[ trinity_0150331_BEGIN -- tmp
		fc80211_set_regulatory_flags(drv, &result);
#endif //]] trinity_0150331_END -- tmp

		hw_mode = wpa_driver_fc80211_postprocess_modes(result.modes,
							    num_modes);
		return hw_mode;
	}

	return NULL;
}

int wpa_driver_fc80211_send_frame(struct i802_bss *bss,
					 const void *data, size_t len,
					 int encrypt, int noack,
					 unsigned int freq, int no_cck,
					 int offchanok, unsigned int wait_time)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	ULONG cookie;
	int res;

	if (freq == 0 && drv->nlmode == FC80211_IFTYPE_ADHOC) {
		freq = fc80211_get_assoc_freq(drv);
		da16x_drv_prt("[%s] send_frame - Use assoc_freq=%u for IBSS\n",
			   			__func__, freq);
	}
	if (freq == 0) {
		da16x_drv_prt("[%s] send_frame - Use bss->freq=%u\n",
			   			__func__, bss->freq);
		freq = bss->freq;
	}

	if (get_run_mode() == 0) {
		da16x_drv_prt("[%s] send_frame -> send_frame_cmd\n", __func__);
	}

	res = fc80211_send_frame_cmd(bss, freq, wait_time, data, len,
				     &cookie, no_cck, noack, offchanok);
	if (res == 0 && !noack) {
		const struct _ieee80211_mgmt *mgmt;
		USHORT fc;

		mgmt = (const struct _ieee80211_mgmt *) data;
		fc = le_to_host16(mgmt->frame_control);
		if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
		    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_ACTION) {
			da16x_drv_prt("[%s] Update send_action_cookie from 0x%llx "
					"to 0x%llx\n",
				   __func__, (long long unsigned int)
				   drv->send_action_cookie,
				   (long long unsigned int) cookie);

			drv->send_action_cookie = cookie;
		}
	}

	return res;
}

int wpa_driver_fc80211_send_mlme(struct i802_bss *bss, const UCHAR *data,
					size_t data_len, int noack,
					unsigned int freq, int no_cck,
					int offchanok,
					unsigned int wait_time)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct _ieee80211_mgmt *mgmt;
	int encrypt = 1;
	USHORT fc;

	mgmt = (struct _ieee80211_mgmt *) data;
	fc = le_to_host16(mgmt->frame_control);

	if (get_run_mode() == 0) {
		da16x_drv_prt("[%s] send_mlme - noack=%d freq=%u no_cck=%d offchanok=%d "
				"wait_time=%u fc=0x%x nlmode=%d\n",
				__func__,
				noack, freq, no_cck, offchanok,
				wait_time, fc, drv->nlmode);
	}

	if ((is_sta_interface(drv->nlmode) ||
	     drv->nlmode == FC80211_IFTYPE_P2P_DEVICE) &&
	    WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
	    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_PROBE_RESP) {
		/*
		 * The use of last_mgmt_freq is a bit of a hack,
		 * but it works due to the single-threaded nature
		 * of wpa_supplicant.
		 */
		if (freq == 0) {
			da16x_drv_prt("[%s] Use last_mgmt_freq=%d\n",
						__func__, drv->last_mgmt_freq);
			freq = drv->last_mgmt_freq;
		}
		return fc80211_send_frame_cmd(bss, freq, wait_time,
					      data, data_len, NULL, 1, noack,
					      1);
	}

	if (drv->device_ap_sme && is_ap_interface(drv->nlmode)) {
		if (freq == 0) {
			da16x_drv_prt("[%s] Use bss->freq=%d\n", __func__, bss->freq);
			freq = bss->freq;
		}
		return fc80211_send_frame_cmd(bss, freq,
					      wait_time,
					      data, data_len,
					      &drv->send_action_cookie,
					      no_cck, noack, offchanok);
	}

	if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
	    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_AUTH) {
		/*
		 * Only one of the authentication frame types is encrypted.
		 * In order for static WEP encryption to work properly (i.e.,
		 * to not encrypt the frame), we need to tell macd11 about
		 * the frames that must not be encrypted.
		 */
		USHORT auth_alg = le_to_host16(mgmt->u.auth.auth_alg);
		USHORT auth_trans = le_to_host16(mgmt->u.auth.auth_transaction);
		if (auth_alg != WLAN_AUTH_SHARED_KEY || auth_trans != 3)
			encrypt = 0;
	}

	//da16x_drv_prt("[%s] send_mlme -> send_frame\n", __func__);

	return wpa_driver_fc80211_send_frame(bss, data, data_len, encrypt,
					     noack, freq, no_cck, offchanok,
					     wait_time);
}

#define NLD11_MAX_SUPP_RATES_SUPP			32
int fc80211_bss_set(struct i802_bss *bss, int cts, int preamble,
			   int slot, int ht_opmode, int ap_isolate,
			   int *basic_rates)
{
	int ret = -1;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	u8 rates[NLD11_MAX_SUPP_RATES_SUPP];
	u8 rates_len = 0;

	/* by Bright : FC80211_CMD_SET_BSS
	 *	Direct function call on UMAC layer
	 */

	if (basic_rates) {
		int i;
		for (i = 0; i < NLD11_MAX_SUPP_RATES_SUPP && basic_rates[i] >= 0; i++)
			rates[rates_len++] = basic_rates[i] / 5;

	}

	ret = fc80211_set_bss(drv->ifindex, cts, preamble, slot, ht_opmode,
			ap_isolate, rates,rates_len);

	return ret;
}

int wpa_driver_fc80211_set_acl(void *priv,
				      struct hostapd_acl_params *params)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;

	TX_FUNC_START("");

	if (!(drv->capa.max_acl_mac_addrs))
		return -ENOTSUP;

	if (params->num_mac_acl > drv->capa.max_acl_mac_addrs)
		return -ENOTSUP;

	da16x_drv_prt("[%s] Set %s ACL (num_mac_acl=%u)\n",
		__func__,
		params->acl_policy ? "Accept" : "Deny", params->num_mac_acl);


	if (ret) {
		da16x_drv_prt("[%s] Failed to set MAC ACL: %d (%s)\n",
			   __func__, ret, strerror(-ret));
	}

	TX_FUNC_END("");

	return ret;
}

#ifdef CONFIG_AP_ISOLATION
int wpa_driver_fc80211_set_ap_isolate(void *priv,
			      int isolate)
{
	if (isolate < 0)
		return -1;

	if (isolate)
		fc80211_ctrl_bridge(FALSE);
	else
		fc80211_ctrl_bridge(TRUE);
        return 0;
}
#endif /* CONFIG_AP_ISOLATION */

int wpa_driver_fc80211_set_ap(void *priv,
			      struct wpa_driver_ap_params *params)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;
	int beacon_set;
#ifdef	ENABLE_DRV_DBG
	int ifindex;
#endif	/* ENABLE_DRV_DBG */
	int num_suites;
	UINT suites[10];
#ifdef CONFIG_MESH
	struct wpa_driver_mesh_bss_params mesh_params;
#endif /* CONFIG_MESH */

	TX_FUNC_START("");

	da16x_drv_prt("[%s] Set as 0 temporaly (%s)...\n", __func__, bss->ifname);

#ifdef	ENABLE_DRV_DBG
	ifindex = 0;
#endif	/* ENABLE_DRV_DBG */
#ifdef ENABLE_DRV_DBG
	if(os_strncmp(bss->ifname, "wlan1", 5) == 0) {
		ifindex = 1;
	} else {
		ifindex = 0;
	}
#endif

	beacon_set = bss->beacon_set;

	da16x_drv_prt("  * Set beacon (beacon_set=%d)\n", beacon_set);

	da16x_dump("  * Beacon head", params->head, params->head_len);
	da16x_dump("  * Beacon tail", params->tail, params->tail_len);

#ifdef	ENABLE_DRV_DBG
	da16x_drv_prt("  * ifindex : %d\n", ifindex);
#endif	/* ENABLE_DRV_DBG */
	da16x_drv_prt("  * beacon_int : %d\n", params->beacon_int);
	da16x_drv_prt("  * dtim_period : %d\n", params->dtim_period);
#ifdef CONFIG_AP_ISOLATION
	da16x_drv_prt("  * isolate : %d\n", params->isolate);
#endif /* CONFIG_AP_ISOLATION */

	da16x_ascii_dump("  * ssid", params->ssid, params->ssid_len);

	if (params->proberesp && params->proberesp_len) {
		da16x_dump("proberesp (offload)",
				params->proberesp, params->proberesp_len);
	}

	switch (params->hide_ssid) {
	case NO_SSID_HIDING:
		da16x_drv_prt(" - hidden SSID not in use\n");
		break;
	case HIDDEN_SSID_ZERO_LEN:
		da16x_drv_prt(" - hidden SSID zero len\n");
		break;
	case HIDDEN_SSID_ZERO_CONTENTS:
		da16x_drv_prt(" - hidden SSID zero contents\n");
		break;
	}

	da16x_drv_prt("  * privacy = %d\n", params->privacy);
	da16x_drv_prt("  * auth_algs = 0x%x\n", params->auth_algs);
	da16x_drv_prt("  * wpa_version = 0x%x\n", params->wpa_version);
	da16x_drv_prt("  * key_mgmt_suites = 0x%x\n", params->key_mgmt_suites);

	num_suites = 0;
	memset(suites, 0, sizeof(UINT) * 10);

	if (params->key_mgmt_suites & WPA_KEY_MGMT_IEEE8021X)
		suites[num_suites++] = WLAN_AKM_SUITE_8021X;
	if (params->key_mgmt_suites & WPA_KEY_MGMT_PSK)
		suites[num_suites++] = WLAN_AKM_SUITE_PSK;

	da16x_drv_prt("  * pairwise_ciphers = 0x%x\n", params->pairwise_ciphers);
	da16x_drv_prt("  * group_cipher = 0x%x\n", params->group_cipher);

	if (params->beacon_ies) {
		da16x_buf_dump("  * beacon_ies", params->beacon_ies);
	}
	if (params->proberesp_ies) {
		da16x_buf_dump("  * proberesp_ies", params->proberesp_ies);
	}
	if (params->assocresp_ies) {
		da16x_buf_dump("  * assocresp_ies", params->assocresp_ies);
	}

	if (drv->capa.flags & WPA_DRIVER_FLAGS_INACTIVITY_TIMER) {
		da16x_drv_prt("  * ap_max_inactivity = %d\n", params->ap_max_inactivity);
	}

	if (beacon_set) {
		/* by Bright : FC80211_CMD_SET_BEACON
		 *	Direct function call on UMAC layer
		 */
#ifdef CONFIG_OWE_TRANS
		if (drv->ifindex == 0) {
		} else
#endif // CONFIG_OWE_TRANS
		ret = fc80211_set_beacon(drv->ifindex, params);
	} else {
		/* by Bright : FC80211_CMD_NEW_BEACON
		 *	Direct function call on UMAC layer
		 */
		ret = fc80211_new_beacon(drv->ifindex, params);
#ifdef CONFIG_OWE_TRANS
		if (drv->ifindex == 0) {
			fc80211_set_beacon_dual(1,params);
		}	
#endif // CONFIG_OWE_TRANS
	}

	if (ret) {
		da16x_debug_prt("[%s] Beacon set failed: %d\n",
			   	__func__, ret);
	} else {
		bss->beacon_set = 1;
#ifdef CONFIG_AP_ISOLATION
		fc80211_bss_set(bss, params->cts_protect,
				params->preamble, params->short_slot_time,
				params->ht_opmode, params->isolate,
				params->basic_rates);
#else
		fc80211_bss_set(bss, params->cts_protect,
				params->preamble, params->short_slot_time,
				params->ht_opmode, ISOLATE_DEFAULT,
				params->basic_rates);
#endif /* CONFIG_AP_ISOLATION */

		if (beacon_set && params->freq &&
		    params->freq->bandwidth != bss->bandwidth) {
			da16x_drv_prt("[%s] Update BSS %s bandwidth: %d -> %d\n",
					__func__,
					bss->ifname, bss->bandwidth,
					params->freq->bandwidth);
			ret = fc80211_set_chan(bss, params->freq, 1);
			if (ret) {
				da16x_err_prt("[%s] Frequency set failed: %d\n",
					__func__, ret);
			} else {
				da16x_drv_prt("[%s] Frequency set succeeded for "
					"ht2040 coex\n", __func__);

				bss->bandwidth = params->freq->bandwidth;
			}
		} else if (!beacon_set) {
			/*
			 * cfgd11 updates the driver on frequence change in AP
			 * mode only at the point when beaconing is started, so
			 * set the initial value here.
			 */
			bss->bandwidth = params->freq->bandwidth;
		}
	}

#ifdef CONFIG_MESH
		if (is_mesh_interface(drv->nlmode) && params->ht_opmode != -1) {
			os_memset(&mesh_params, 0, sizeof(mesh_params));
			mesh_params.flags |= WPA_DRIVER_MESH_CONF_FLAG_HT_OP_MODE;
			mesh_params.ht_opmode = params->ht_opmode;
			ret = fc80211_update_mesh_config(drv->ifindex, &mesh_params);
			
			if (ret < 0)
				return ret;
		}
#endif /* CONFIG_MESH */


	TX_FUNC_END("");

	return ret;
}

int fc80211_set_chan(struct i802_bss *bss,
			       struct hostapd_freq_params *freq, int set_chan)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] Set freq %d (ht_enabled=%d, vht_enabled=%d, bandwidth=%d MHz, cf1=%d MHz, cf2=%d MHz)\n",
			__func__,
		   freq->freq, freq->ht_enabled, freq->vht_enabled,
		   freq->bandwidth, freq->cent_freq1, freq->cent_freq2);

	/* by Bright : FC80211_CMD_SET_CHANNEL
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_set_channel(drv->ifindex, freq, set_chan);
	if (ret == 0) {
		bss->freq = freq->freq;
		return 0;
	}

	da16x_err_prt("[%s] Failed to set channel (freq=%d): %d\n",
				__func__, freq->freq, ret);

	TX_FUNC_END("");
	return -1;
}

UINT sta_flags_fc80211(int flags)
{
	UINT f = 0;

	if (flags & WPA_STA_AUTHORIZED)
		f |= BIT(FC80211_STA_FLAG_AUTHORIZED);
	if (flags & WPA_STA_WMM)
		f |= BIT(FC80211_STA_FLAG_WME);
	if (flags & WPA_STA_SHORT_PREAMBLE)
		f |= BIT(FC80211_STA_FLAG_SHORT_PREAMBLE);
	if (flags & WPA_STA_MFP)
		f |= BIT(FC80211_STA_FLAG_MFP);
	if (flags & WPA_STA_TDLS_PEER)
		f |= BIT(FC80211_STA_FLAG_TDLS_PEER);

	return f;
}

#ifdef  CONFIG_P2P_POWER_SAVE /* by Shingu 20180208 (P2P PS opt.) */
int prev_p2p_ps_status = -1;
int wpa_driver_fc80211_p2p_ps_from_ps(void *priv, int p2p_ps_status)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = 0;
	int val;

	if (p2p_ps_status == prev_p2p_ps_status)
		return 0;

	if (get_run_mode() == STA_P2P_CONCURRENT_MODE) {
		da16x_drv_prt("[%s] P2P-PS unabailable on Concurrent mode\n");
		return -1;
	}

	if (wpa_config_read_nvram_int("p2p_ps", &val) != -1 && val == 0) {
		if (prev_p2p_ps_status == -1) {
			prev_p2p_ps_status = 0;
			return fc80211_p2p_go_ps_on_off(drv->ifindex, 0);
		} else {
			return 0;
		}
	}

	prev_p2p_ps_status = p2p_ps_status;
	ret = fc80211_p2p_go_ps_on_off(drv->ifindex, p2p_ps_status);

	return ret;
}
#endif	/* CONFIG_P2P_POWER_SAVE */

int wpa_driver_fc80211_sta_add(void *priv,
				      struct hostapd_sta_add_params *params)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct fc80211_sta_flag_update upd;
	int ret = -ENOBUFS;

	TX_FUNC_START("");

	if ((params->flags & WPA_STA_TDLS_PEER) &&
	    !(drv->capa.flags & WPA_DRIVER_FLAGS_TDLS_SUPPORT))
		return -EOPNOTSUPP;

	da16x_drv_prt("[%s] %s STA " MACSTR "\n",
			__func__,
			params->set ? "Set" : "Add", MAC2STR(params->addr));

	da16x_dump("  * supported rates",
			params->supp_rates, params->supp_rates_len);

	if (!params->set) {
		if (params->aid) {
			da16x_drv_prt("  * aid=%u\n", params->aid);
		} else {
			/*
			 * cfgd11 validates that AID is non-zero, so we have
			 * to make this a non-zero value for the TDLS case where
			 * a dummy STA entry is used for now.
			 */
			da16x_drv_prt("  * aid=1 (TDLS workaround)\n");
		}
		da16x_drv_prt("  * listen_interval=%u\n", params->listen_interval);
	} else if (params->aid && (params->flags & WPA_STA_TDLS_PEER)) {
		da16x_drv_prt("  * peer_aid=%u\n", params->aid);
	}
	if (params->ht_capabilities) {
		da16x_dump("  * ht_capabilities",
			(UCHAR *) params->ht_capabilities,
			sizeof(*params->ht_capabilities));
	}

#ifdef CONFIG_AP_VHT
	if (params->vht_capabilities) {
		da16x_dump("  * vht_capabilities",
				(UCHAR *) params->vht_capabilities,
					sizeof(*params->vht_capabilities));
	}

	if (params->vht_opmode_enabled) {
		da16x_drv_prt("  * opmode=%u\n", params->vht_opmode);
	}
#endif /* CONFIG_AP_VHT */

	da16x_drv_prt("  * capability=0x%x\n", params->capability);

	if (params->ext_capab) {
		da16x_dump("  * ext_capab",
			params->ext_capab, params->ext_capab_len);
	}

	if (params->supp_channels) {
		da16x_dump("  * supported channels",
			params->supp_channels, params->supp_channels_len);
	}

	if (params->supp_oper_classes) {
		da16x_dump("  * supported operating classes",
				params->supp_oper_classes,
				params->supp_oper_classes_len);
	}

	os_memset(&upd, 0, sizeof(upd));
	upd.mask = sta_flags_fc80211(params->flags);
	upd.set = upd.mask;
	da16x_drv_prt("  * flags set=0x%x mask=0x%x\n", upd.set, upd.mask);

	if (params->flags & WPA_STA_WMM) {
		da16x_drv_prt("  * qosinfo=0x%x\n", params->qosinfo);
	}

	if (params->set) {
		/* by Bright : FC80211_CMD_SET_STATION
		 *	Direct function call on UMAC layer
		 */
#ifdef CONFIG_MESH
		ret = fc80211_set_station(drv->ifindex, params, &upd);
#else
		ret = fc80211_set_station(drv->ifindex, params->addr, &upd);
#endif
	} else {
		/* by Bright : FC80211_CMD_SET_STATION
		 *	Direct function call on UMAC layer
		 */
		ret = fc80211_new_station(drv->ifindex, params, &upd);
	}

	if (ret)
		da16x_drv_prt("[%s] FC80211_CMD_%s_STATION result: %d (%s)\n",
				__func__,
				params->set ? "SET" : "NEW", ret,
				strerror(-ret));
	if (ret == -EEXIST)
		ret = 0;

	TX_FUNC_END("");

	return ret;
}

int wpa_driver_fc80211_sta_remove(struct i802_bss *bss, const UCHAR *addr)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	TX_FUNC_START("");

	/* by Bright : FC80211_CMD_DEL_STATION
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_del_station(drv->ifindex, addr, -1);
	da16x_drv_prt("[%s] sta_remove -> DEL_STATION %s " MACSTR " --> %d (%s)\n",
		   __func__, bss->ifname, MAC2STR(addr), ret, strerror(-ret));
	if (ret == -ENOENT)
		return 0;

	TX_FUNC_END();

	return ret;
}

void fc80211_remove_iface(struct wpa_driver_fc80211_data *drv,
				 int ifidx)
{
	struct wpa_driver_fc80211_data *drv2;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] Remove interface ifindex=%d\n", __func__, ifidx);

	/* stop listening for EAPOL on this interface */
	dl_list_for_each(drv2, &drv->global->interfaces,
			 struct wpa_driver_fc80211_data, list)
			del_ifidx(drv2, ifidx);

	/* by Bright : FC80211_CMD_DEL_STATION
	 *	Direct function call on UMAC layer
	 */
	if (chk_dpm_wakeup() == pdFALSE)
		fc80211_del_interface(0, drv->ifindex);

	TX_FUNC_END("");

	return;
}

int fc80211_create_iface_once(struct wpa_driver_fc80211_data *drv,
				     const char *ifname,
				     enum fc80211_iftype iftype,
				     const UCHAR *addr, int wds,
				     int (*handler)(void *),
				     void *arg)
{
	struct i802_bss *bss = (struct i802_bss *)drv->first_bss;
	int ifidx = 1;
	int ret = -ENOBUFS;

	da16x_drv_prt("[%s] Create interface iftype %d\n", __func__, iftype);

	/* by Bright : FC80211_CMD_NEW_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0 ||
	    os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0
#ifdef CONFIG_MESH	    
	    ||  os_strcmp(ifname, MESH_POINT_DEVICE_NAME) == 0
#endif	    
	    ) {
		ret = fc80211_new_interface(bss->wdev_id, ifidx, "wlan1", iftype);
	} else {
		if (chk_dpm_wakeup() == pdTRUE) {
			ret = 0;
		} else {
#ifdef CONFIG_OWE_TRANS
			ifidx = 0;
#endif // CONFIG_OWE_TRANS
			ret = fc80211_new_interface(bss->wdev_id, ifidx, "wlan0", iftype);
		}
	}

	if (ret < 0) {
		da16x_drv_prt("[%s] Failed to create interface %s: %d (%s)\n",
			   __func__, ifname, ret, strerror(-ret));
		return ret;
	}

	if (iftype == FC80211_IFTYPE_P2P_DEVICE)
		return 0;

	da16x_drv_prt("[%s] New interface %s created: ifindex=%d\n",
		   				__func__, ifname, ifidx);


	/*
	 * Some virtual interfaces need to process EAPOL packets and events on
	 * the parent interface. This is used mainly with hostapd.
	 */
	if (drv->hostapd ||
	    iftype == FC80211_IFTYPE_AP_VLAN ||
	    iftype == FC80211_IFTYPE_WDS ||
	    iftype == FC80211_IFTYPE_MONITOR) {
		/* start listening for EAPOL on this interface */
		add_ifidx(drv, ifidx);
	}

	da16x_drv_prt("[%s] What do I do for this operation ?\n", __func__);

	return ifidx;
}

int fc80211_create_iface(struct wpa_driver_fc80211_data *drv,
				const char *ifname,
				enum fc80211_iftype iftype,
				const UCHAR *addr,
				int wds,
				int (*handler)(void *),
				void *arg,
				int use_existing)
{
	int ret;

	ret = fc80211_create_iface_once(drv, ifname, iftype, addr, wds,
					handler, arg);

#ifdef	CONFIG_P2P
	/* by Bright :
	 *	P2P don't support ieee802.11b because of performance
	 */
	if (ret >= 0 && is_p2p_net_interface(iftype))
		fc80211_disable_11b_rates(drv, ret, 1);
#endif	/* CONFIG_P2P */

	return ret;
}

#ifdef	CONFIG_AP
int fc80211_setup_ap(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;

	/*
	 * Disable Probe Request reporting unless we need it in this way for
	 * devices that include the AP SME, in the other case (unless using
	 * monitor iface) we'll get it through the nl_mgmt socket instead.
	 */
	if (!drv->device_ap_sme) {
		wpa_driver_fc80211_probe_req_report(bss, 0);
	}

	if (!drv->device_ap_sme && !drv->use_monitor) {
		if (fc80211_mgmt_subscribe_ap(bss))
			return -1;
	}

	if (drv->device_ap_sme && !drv->use_monitor) {
		if (fc80211_mgmt_subscribe_ap_dev_sme(bss))
			return -1;
	}

	if (drv->device_ap_sme &&
	    wpa_driver_fc80211_probe_req_report(bss, 1) < 0) {
		da16x_drv_prt("[%s] Failed to enable Probe Request frame "
			"reporting in AP mode\n", __func__);
		/* Try to survive without this */
	}

	return 0;
}

void fc80211_teardown_ap(struct i802_bss *bss)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;

	da16x_drv_prt("[%s] Teardown AP(%s) - device_ap_sme=%d use_monitor=%d\n",
		   __func__, bss->ifname, drv->device_ap_sme, drv->use_monitor);

	if (drv->device_ap_sme) {
		wpa_driver_fc80211_probe_req_report(bss, 0);
		if (!drv->use_monitor)
			fc80211_mgmt_unsubscribe(bss, "AP teardown (dev SME)");
	} else {
		fc80211_mgmt_unsubscribe(bss, "AP teardown");
	}

	bss->beacon_set = 0;
}
#endif	/* CONFIG_AP */

#ifdef	IEEE8021X_EAPOL
int fc80211_send_eapol_data(struct i802_bss *bss,
				   const UCHAR *addr, const UCHAR *data,
				   size_t data_len)
{
	extern int i3ed11_l2data_from_sp(int ifindex,
					unsigned char *data,
					unsigned int length,
					unsigned char hasMacHeader,
					const char *dst,
					int is_hs_ack);

	int ret = 0;
#ifdef	CONFIG_AP
	da16x_ap_prt("[%s] bss->ifindex = %d \n", __func__, bss->ifindex);
#endif	/* CONFIG_AP */

	/* direct function call to UMAC */
	ret = i3ed11_l2data_from_sp(bss->ifindex,
				(unsigned char *)data,
				(unsigned int)data_len,
				0,	// FALSE
				(const char *)addr,
				0);

	return ret;
}

#if 1	/* by Bright : to separate from UMAC dependancy */
UCHAR	supp_rfc1042_hdr[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
#else
extern UCHAR rfc1042_header[6];		/* defined in umac/src/tx.c */
#endif	/* 1 */

int wpa_driver_fc80211_hapd_send_eapol(
	void *priv, const UCHAR *addr, const UCHAR *data,
	size_t data_len, int encrypt, const UCHAR *own_addr, UINT flags)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct _ieee80211_hdr *hdr;
	size_t len;
	UCHAR *pos;
	int res;
	int qos = flags & WPA_STA_WMM;

	TX_FUNC_START("");

	if (drv->device_ap_sme || !drv->use_monitor)
		return fc80211_send_eapol_data(bss, addr, data, data_len);

#if 1	/* by Bright : to separate from UMAC dependancy */
	len = sizeof(*hdr) + (qos ? 2 : 0) + sizeof(supp_rfc1042_hdr) + 2 + data_len;
#else
	len = sizeof(*hdr) + (qos ? 2 : 0) + sizeof(rfc1042_header) + 2 + data_len;
#endif	/* 1 */
	hdr = os_zalloc(len);
	if (hdr == NULL) {
		da16x_drv_prt("[%s] Failed to allocate EAPOL buffer(len=%lu)\n",
			   __func__, (unsigned long) len);
		return -1;
	}

	hdr->frame_control =
		IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
	hdr->frame_control |= host_to_le16(WLAN_FC_FROMDS);
	if (encrypt)
		hdr->frame_control |= host_to_le16(WLAN_FC_ISWEP);
	if (qos) {
		hdr->frame_control |=
			host_to_le16(WLAN_FC_STYPE_QOS_DATA << 4);
	}

	memcpy(hdr->IEEE80211_DA_FROMDS, addr, ETH_ALEN);
	memcpy(hdr->IEEE80211_BSSID_FROMDS, own_addr, ETH_ALEN);
	memcpy(hdr->IEEE80211_SA_FROMDS, own_addr, ETH_ALEN);
	pos = (UCHAR *) (hdr + 1);

	if (qos) {
		/* Set highest priority in QoS header */
		pos[0] = 7;
		pos[1] = 0;
		pos += 2;
	}

#if 1	/* by Bright : to separate from UMAC dependancy */
	memcpy(pos, supp_rfc1042_hdr, sizeof(supp_rfc1042_hdr));
	pos += sizeof(supp_rfc1042_hdr);
#else
	memcpy(pos, rfc1042_header, sizeof(rfc1042_header));
	pos += sizeof(rfc1042_header);
#endif	/* 1 */
	WPA_PUT_BE16(pos, ETH_P_PAE);
	pos += 2;
	memcpy(pos, data, data_len);

	res = wpa_driver_fc80211_send_frame(bss,
					(UCHAR *)hdr,
					len,
					encrypt,
					0, 0, 0, 0, 0);
	if (res < 0) {
		da16x_drv_prt("[%s] i802_send_eapol - packet len: %lu - "
			   "failed: %d\n",
			   __func__, (unsigned long) len, res);
	}
	os_free(hdr);

	return res;
}
#endif	/* IEEE8021X_EAPOL */

int wpa_driver_fc80211_sta_set_flags(void *priv, const UCHAR *addr,
					    unsigned int total_flags,
					    unsigned int flags_or, unsigned int flags_and)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct fc80211_sta_flag_update upd;

	TX_FUNC_START("");

	os_memset(&upd, 0, sizeof(upd));
	upd.mask = sta_flags_fc80211(flags_or | ~flags_and);
	upd.set = sta_flags_fc80211(flags_or);

	/* by Bright : FC80211_CMD_SET_STATION
	 *	Direct function call on UMAC layer
	 */
#ifdef CONFIG_MESH
	struct hostapd_sta_add_params params;
	params.addr = addr;
	return fc80211_set_station(drv->ifindex, &params, &upd);	
#else
	return fc80211_set_station(drv->ifindex, addr, &upd);
#endif
}

#ifdef	CONFIG_AP
int wpa_driver_fc80211_ap(struct wpa_driver_fc80211_data *drv,
				 struct wpa_driver_associate_params *params)
{
	enum fc80211_iftype nlmode;
#ifndef	CONFIG_ACS
	enum fc80211_iftype old_mode = drv->nlmode;
	struct hostapd_freq_params freq = {
		.freq = params->freq.freq,
	};
#endif	/* CONFIG_ACS */

	if (params->p2p) {
		da16x_drv_prt("[%s] Setup AP operations for P2P group (GO)\n",
				__func__);
		nlmode = FC80211_IFTYPE_P2P_GO;
	} else
		nlmode = FC80211_IFTYPE_AP;

	if (wpa_driver_fc80211_set_mode(drv->first_bss, nlmode)) {
		da16x_drv_prt("[%s] Error wpa_driver_fc80211_set_mode\n", __func__);
		return -1;
	}

#ifdef CONFIG_ACS
	da16x_drv_prt("[%s] ACS params->freq.freq : %d\n",__func__, params->freq.freq);
#else
	if (fc80211_set_chan(drv->first_bss, &freq.freq, 0)) {
		if (old_mode != nlmode)
			wpa_driver_fc80211_set_mode(drv->first_bss, old_mode);
		return -1;
	}
#endif /* CONFIG_ACS */
	return 0;
}

int fc80211_ap_pwrsave(void *priv, int ps_state, int timeout)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	fc80211_set_power_save(drv->ifindex, ps_state, timeout);
	return 0;
}
int fc80211_addba_reject(void *priv, u8 addba_reject)
{
	TX_FUNC_START("");
	fc80211_ctrl_ampdu_rx(addba_reject);
	return 0;
}

#endif	/* CONFIG_AP */

int wpa_driver_fc80211_try_connect(
	struct wpa_driver_fc80211_data *drv,
	struct wpa_driver_associate_params *params)
{
	int ret = -1;
	int algs;
	int privacy = 0;

	da16x_drv_prt("[%s] Connect (ifindex=%d)\n", drv->ifindex);

	algs = 0;
	if (params->auth_alg & WPA_AUTH_ALG_OPEN)
		algs++;
	if (params->auth_alg & WPA_AUTH_ALG_SHARED)
		algs++;
	if (params->auth_alg & WPA_AUTH_ALG_LEAP)
		algs++;
	if (algs > 1) {
		da16x_drv_prt("  * Leave out Auth Type for automatic selection\n");
		goto skip_auth_type;
	}

skip_auth_type:
/* by Bright :
 *	What do I do to get privacy from this function call.
 *	But have to remove "msg" argument
 */
	ret = fc80211_set_conn_keys(params, &privacy);
	if (ret)
		goto put_cmd_fail;

	ret = 0;
	da16x_drv_prt("[%s] Connect request send successfully\n", __func__);

put_cmd_fail:
	return ret;
}

int wpa_driver_fc80211_connect(
	struct wpa_driver_fc80211_data *drv,
	struct wpa_driver_associate_params *params)
{
	int ret = wpa_driver_fc80211_try_connect(drv, params);
	if (ret == -EALREADY) {
		/*
		 * cfgd11 does not currently accept new connections if
		 * we are already connected. As a workaround, force
		 * disconnection and try again.
		 */
		da16x_drv_prt("[%s] Explicitly disconnecting before reassociation "
			   "attempt\n", __func__);

		if (wpa_driver_fc80211_disconnect(
			    drv, WLAN_REASON_PREV_AUTH_NOT_VALID))
			return -1;
		ret = wpa_driver_fc80211_try_connect(drv, params);
	}
	return ret;
}


int wpa_driver_fc80211_associate(
	void *priv, struct wpa_driver_associate_params *params)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	TX_FUNC_START("");

#ifdef	CONFIG_AP
	if (params->mode == IEEE80211_MODE_AP)
		return wpa_driver_fc80211_ap(drv, params);
#endif	/* CONFIG_AP */

	if (!(drv->capa.flags & WPA_DRIVER_FLAGS_SME)) {
		enum fc80211_iftype nlmode = params->p2p ?
			FC80211_IFTYPE_P2P_CLIENT : FC80211_IFTYPE_STATION;

		if (wpa_driver_fc80211_set_mode(priv, nlmode) < 0)
			return -1;
		return wpa_driver_fc80211_connect(drv, params);
	}

	fc80211_mark_disconnected(drv);

	if (params->freq.freq)
		drv->assoc_freq = (unsigned int)params->freq.freq;
	else
		drv->assoc_freq = 0;

	if (params->ssid) {
		os_memcpy(drv->ssid, params->ssid, params->ssid_len);
		drv->ssid_len = params->ssid_len;
	}

	/* by Bright : FC80211_CMD_ASSOCIATE
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_associate(drv->ifindex, params);
	if (ret) {
		da16x_err_prt("[%s] MLME command failed (assoc): ret=%d\n",
			__func__, ret);

		fc80211_dump_scan(drv);

		goto put_cmd_fail;
	}

	ret = 0;

	da16x_drv_prt("[%s] Association request send successfully\n", __func__);

put_cmd_fail :
	TX_FUNC_END("");

	return ret;
}

int fc80211_set_mode(struct i802_bss *bss,
			    int ifindex, enum fc80211_iftype mode)
{
	int ret = -1;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] ifindex=%d iftype=%d\n", __func__, ifindex, mode);

	/* by Bright : FC80211_CMD_SET_INTERFACE
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_set_interface(bss->wdev_id, ifindex, mode);

	TX_FUNC_END("");

	return ret;
}


int wpa_driver_fc80211_set_mode(struct i802_bss *bss,
				       enum fc80211_iftype fc_mode)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;
	int i;
	int was_ap = is_ap_interface(drv->nlmode);
	int res;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] drv->nlmode = %d\n", __func__, drv->nlmode);
	if(drv->nlmode == fc_mode) {
		da16x_drv_prt("[%s] Already in a requested mode\n", __func__);
		return 0;
	}

	if(chk_dpm_wakeup() == pdTRUE) {
		res = fc80211_set_mode(bss, drv->ifindex, fc_mode);

		drv->nlmode = fc_mode;
		ret = 0;
		goto done;
	} else {
		res = fc80211_set_mode(bss, drv->ifindex, fc_mode);
		if (res && fc_mode == fc80211_get_ifmode(bss))
			res = 0;

		if (res == 0) {
			drv->nlmode = fc_mode;
			ret = 0;
			goto done;
		}
	}

	if (res == -ENODEV) {
		ret = -1;
		da16x_drv_prt("[%s] Error #1\n", __func__);
		goto done;
	}

	if (fc_mode == drv->nlmode) {
		da16x_drv_prt("[%s] Interface already in "
			   "requested mode - ignore error\n", __func__);
		ret = 0;
		goto done; /* Already in the requested mode */
	}

	/* macd11 doesn't allow mode changes while the device is up, so
	 * take the device down, try to set the mode again, and bring the
	 * device back up.
	 */
	da16x_drv_prt("[%s] Try mode change after setting interface down\n", __func__);

	for (i = 0; i < 10; i++) {
		res = i802_set_iface_flags(bss, 0);
		if (res == -EACCES || res == -ENODEV)
			break;
		if (res == 0) {
			/* Try to set the mode again while the interface is
			 * down */
			ret = fc80211_set_mode(bss, drv->ifindex, fc_mode);
			if (ret == -EACCES)
				break;
			res = i802_set_iface_flags(bss, 1);
			if (res && !ret)
				ret = -1;
			else if (ret != -EBUSY)
				break;
		} else {
			da16x_drv_prt("[%s] Failed to set "
					"interface down\n", __func__);
		}
		os_sleep(0, 100000);
	}

	if (!ret) {
		da16x_drv_prt("[%s] Mode change succeeded while "
			   "interface is down\n", __func__);
		drv->nlmode = fc_mode;
		drv->ignore_if_down_event = 1;
	}

done:
	if (ret) {
		da16x_drv_prt("[%s] Interface mode change to "
				"%d from %d failed\n",
				__func__, fc_mode, drv->nlmode);
		return ret;
	}

#ifdef	CONFIG_P2P
	if (is_p2p_net_interface(fc_mode))
		fc80211_disable_11b_rates(drv, drv->ifindex, 1);
	else
#endif	/* CONFIG_P2P */
	if (drv->disabled_11b_rates)
		fc80211_disable_11b_rates(drv, drv->ifindex, 0);

#ifdef	CONFIG_AP
	if (is_ap_interface(fc_mode)) {
		fc80211_mgmt_unsubscribe(bss, "start AP");
		/* Setup additional AP mode functionality if needed */
		if (fc80211_setup_ap(bss))
			return -1;
	} else if (was_ap) {
		/* Remove additional AP mode functionality */
		fc80211_teardown_ap(bss);
	} else
#endif	/* CONFIG_AP */
	{
		fc80211_mgmt_unsubscribe(bss, "mode change");
	}

#ifdef CONFIG_MESH
	if (is_mesh_interface(fc_mode) &&
		fc80211_mgmt_subscribe_mesh(bss))
		return -1;
#endif

	if (!bss->in_deinit && !is_ap_interface(fc_mode) &&
	    fc80211_mgmt_subscribe_non_ap(bss) < 0)
		da16x_drv_prt("[%s] Failed to register Action "
			   "frame processing - ignore for now\n", __func__);

	TX_FUNC_END("");

	return 0;
}

int wpa_driver_fc80211_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int dfs_capability = 0;
	int ret = 0;

	TX_FUNC_START("");

	if (!drv->has_capability)
		return -1;

	os_memcpy(capa, &drv->capa, sizeof(*capa));
	if (drv->extended_capa && drv->extended_capa_mask) {
		capa->extended_capa = drv->extended_capa;
		capa->extended_capa_mask = drv->extended_capa_mask;
		capa->extended_capa_len = drv->extended_capa_len;
	}

	if ((capa->flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
	    !drv->allow_p2p_device) {
		da16x_drv_prt("[%s] Do not indicate P2P_DEVICE support "
			"(p2p_device=1 driver param not specified)\n",
				__func__);

		capa->flags &= ~WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE;
	}

	if (drv->dfs_vendor_cmd_avail == 1) {
		if (!ret) {
			if (dfs_capability)
				capa->flags |= WPA_DRIVER_FLAGS_DFS_OFFLOAD;
		}
	}

	TX_FUNC_END("");

	return ret;
}

int wpa_driver_fc80211_set_supp_port(void *priv, int authorized)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct fc80211_sta_flag_update upd;
	int ret = -ENOBUFS;

	TX_FUNC_START("");

	if (!drv->associated && is_zero_ether_addr(drv->bssid) && !authorized) {
		da16x_drv_prt("[%s] Skip set_supp_port(unauthorized) "
				"while not associated\n", __func__);
		return 0;
	}

	da16x_drv_prt("[%s] Set supplicant port %sauthorized for "
		MACSTR "\n",
		__func__, authorized ? "" : "un", MAC2STR(drv->bssid));

	os_memset(&upd, 0, sizeof(upd));
	upd.mask = BIT(FC80211_STA_FLAG_AUTHORIZED);
	if (authorized)
		upd.set = BIT(FC80211_STA_FLAG_AUTHORIZED);

	/* by Bright : FC80211_CMD_SET_STATION
	 *	Direct function call on UMAC layer
	 */
#ifdef CONFIG_MESH
	struct hostapd_sta_add_params params;
	params.addr = (const u8*)&drv->bssid;
	ret = fc80211_set_station(drv->ifindex, &params, &upd);		
#else
	ret = fc80211_set_station(drv->ifindex, drv->bssid, &upd);
#endif
	if (!ret) {
		TX_FUNC_END("");
		return 0;
	}

	da16x_drv_prt("[%s] Failed to set STA flag: %d (%s)\n",
			__func__, ret, strerror(-ret));

	TX_FUNC_END("");

	return ret;
}

/* Set kernel driver on given frequency (MHz) */
int i802_set_freq(void *priv, struct hostapd_freq_params *freq)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return fc80211_set_chan(bss, freq, 0);
}

int get_key_handler(void *arg)
{
	da16x_drv_prt("[%s] What do I do for this ???\n", __func__);

	return 1;
}

int i802_get_seqnum(const char *iface, void *priv, const UCHAR *addr,
			   int idx, UCHAR *seq)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	memset(seq, 0, 6);

	/* by Bright : FC80211_CMD_GET_KEY
	 *	Direct function call on UMAC layer
	 */
	return fc80211_get_key(drv->ifindex, seq,(u8)idx,addr);
}
int i802_set_rts(void *priv, int rts)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -ENOBUFS;
	UINT val;

	TX_FUNC_START("");

	if (rts > 2347) //20160613_trinity ">=" -> ">"
		val = (UINT) -1;
	else
		val = rts;

	/* by Bright : FC80211_CMD_SET_SOFTMAC
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_set_softmac_rts(drv->ifindex, val);
	if (!ret)
		return 0;

	da16x_drv_prt("[%s] Failed to set RTS threshold %d: %d (%s)\n",
			__func__, rts, ret, strerror(-ret));

	return ret;
}

int i802_get_rts(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int res;

	TX_FUNC_START("");

	res = fc80211_get_softmac_rts(drv->ifindex);
	if(res == -ENODATA)
		da16x_drv_prt("[%s] Failed to get rts value: %d (%s)\n",
			__func__,  res, strerror(-res));

	return res;
}

#ifdef __FRAG_ENABLE__
int i802_set_frag(void *priv, int frag)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -ENOBUFS;
	UINT val;

	TX_FUNC_START("");

	if (frag >= 2346)
		val = (UINT) -1;
	else
		val = frag;

	/* by Bright : FC80211_CMD_SET_SOFTMAC
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_set_softmac_frag(drv->ifindex, val);
	if (!ret)
		return 0;

	da16x_drv_prt("[%s] Failed to set fragmentation threshold %d: %d (%s)\n",
			__func__, frag, ret, strerror(-ret));

	return ret;
}
#endif
int i802_set_retry(void *priv, u8 retry, u8 retry_long)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -ENOBUFS;

	TX_FUNC_START("");

	ret = fc80211_set_softmac_retry(drv->ifindex, retry, retry_long);
	if (ret == -ENODATA)
		return 0;

	da16x_drv_prt("[%s] Failed to set retry value(retry = %d): %d (%s)\n",
			__func__, retry, ret, strerror(-ret));

	return ret;
}

int i802_get_retry(void *priv, u8 retry_long)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int res;

	TX_FUNC_START("");

	res = fc80211_get_softmac_retry(drv->ifindex, retry_long);
	if(res == -ENODATA)
		da16x_drv_prt("[%s] Failed to get retry value(retry_long = %d): %d (%s)\n",
			__func__, retry_long, res, strerror(-res));

	return res;
}

int i802_flush(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int res;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] flush -> DEL_STATION %s (all)\n", __func__, bss->ifname);

	/* by Bright : FC80211_CMD_DEL_STATION
	 *	Direct function call on UMAC layer
	 */
	res = fc80211_del_station(drv->ifindex, NULL, -1);
	if (res) {
		da16x_drv_prt("[%s] Station flush failed: ret=%d (%s)\n",
					__func__, res, strerror(-res));
	}

	return res;
}

#ifdef CONFIG_AP_POWER
int i802_set_ap_power(struct i802_bss *bss, int type, int power, int *get_power)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;

	return fc80211_set_ap_power(drv->ifindex, type, power, get_power);
}
#endif /* CONFIG_AP_POWER */

int i802_read_sta_data(struct i802_bss *bss,
		       struct hostap_sta_driver_data *data, const UCHAR *addr)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;

	os_memset(data, 0, sizeof(*data));

	/* by Bright : FC80211_CMD_GET_STATION
	 *	Direct function call on UMAC layer
	 */
	return fc80211_get_station(drv->ifindex, addr, 1, data);
}

int i802_set_tx_queue_params(void *priv, int queue, int aifs,
				    int cwmin, int cwmax, int burst_time)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	/* by Bright : FC80211_CMD_DEL_STATION
	 *	Direct function call on UMAC layer
	 */
	if (fc80211_set_softmac(drv->ifindex, queue, aifs, cwmin,
			cwmax, burst_time) == 0)
		return 0;

	return -1;
}

#ifdef CONFIG_AP_VLAN
int i802_set_sta_vlan(struct i802_bss *bss, const UCHAR *addr,
			     const char *ifname, int vlan_id)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -ENOBUFS;

	da16x_drv_prt("[%s] %s: set_sta_vlan(" MACSTR ", ifname=%s, vlan_id=%d)\n",
		   __func__, bss->ifname, MAC2STR(addr), ifname, vlan_id);

	/* by Bright : FC80211_CMD_SET_STATION
	 *	Direct function call on UMAC layer
	 */
#ifdef CONFIG_MESH
	struct hostapd_sta_add_params params;
	params.addr = addr;
	ret = fc80211_set_station(drv->ifindex, &params, NULL);		
#else
	ret = fc80211_set_station(drv->ifindex, addr, NULL);
#endif
	if (ret < 0) {
		da16x_drv_prt("[%s] FC80211_ATTR_STA_VLAN (addr="
			   MACSTR " ifname=%s vlan_id=%d) failed: %d (%s)\n",
				__func__,
				MAC2STR(addr), ifname, vlan_id, ret,
				strerror(-ret));
	}
	return ret;
}
#endif

int i802_sta_clear_stats(void *priv, const UCHAR *addr)
{
	da16x_drv_prt("[%s] What do I do for this ???\n", __func__);

	return 0;
}

int i802_sta_deauth(void *priv, const UCHAR *own_addr, const UCHAR *addr,
			   int reason)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct _ieee80211_mgmt mgmt;

	TX_FUNC_START("");

	if (drv->device_ap_sme)
		return wpa_driver_fc80211_sta_remove(bss, addr);

	memset(&mgmt, 0, sizeof(mgmt));
	mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					  WLAN_FC_STYPE_DEAUTH);
	memcpy(mgmt.da, addr, ETH_ALEN);
	memcpy(mgmt.sa, own_addr, ETH_ALEN);
	memcpy(mgmt.bssid, own_addr, ETH_ALEN);
	mgmt.u.deauth.reason_code = host_to_le16(reason);
	return wpa_driver_fc80211_send_mlme(bss, (UCHAR *) &mgmt,
					    IEEE80211_HDRLEN +
					    sizeof(mgmt.u.deauth), 0, 0, 0, 0,
					    0);
}

int i802_sta_disassoc(void *priv, const UCHAR *own_addr, const UCHAR *addr,
			     int reason)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct _ieee80211_mgmt mgmt;

	TX_FUNC_START("");

	if (drv->device_ap_sme)
		return wpa_driver_fc80211_sta_remove(bss, addr);

	memset(&mgmt, 0, sizeof(mgmt));
	mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					  WLAN_FC_STYPE_DISASSOC);
	memcpy(mgmt.da, addr, ETH_ALEN);
	memcpy(mgmt.sa, own_addr, ETH_ALEN);
	memcpy(mgmt.bssid, own_addr, ETH_ALEN);
	mgmt.u.disassoc.reason_code = host_to_le16(reason);
	return wpa_driver_fc80211_send_mlme(bss, (UCHAR *) &mgmt,
					    IEEE80211_HDRLEN +
					    sizeof(mgmt.u.disassoc), 0, 0, 0, 0,
					    0);
}

#if 1	/* by Shingu 20160622 (P2P GO Inactivity) */
#define	MAX_WAITING_FOR_DEAUTH_SEND_TIMEOUT	3	/* Sec. */
int i802_chk_deauth_send_done(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int count = 0;

	while (1) {
		if (fc80211_chk_deauth_send_done(drv->ifindex))
			break;
		else
			count++;

		if (count >= (MAX_WAITING_FOR_DEAUTH_SEND_TIMEOUT + 1) * 2) {
			da16x_drv_prt("[%s] Send Deauthentication (Diassoc) "
				     "Fail!\n", __func__);
			return -1;
		}

		if (count % 2 == 0)
			os_sleep(1, 0);
	}

	return 0;
}
#endif	/* 1 */

#ifdef  CONFIG_AP /* by Shingu 20161010 (Keep-alive) */
int i802_chk_keep_alive(void *priv, const UCHAR *addr)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int res;

	res = fc80211_sta_null_send(drv->ifindex, (u8 *)addr);

	if (res == 1)
		return 0;
	else
		return -1;
}
#endif  /* CONFIG_AP */


void dump_ifidx(struct wpa_driver_fc80211_data *drv)
{
	char buf[200], *pos, *end;
	int i, res;

	pos = buf;
	end = pos + sizeof(buf);

	for (i = 0; i < drv->num_if_indices; i++) {
		if (!drv->if_indices[i])
			continue;
		res = os_snprintf(pos, end - pos, " %d", drv->if_indices[i]);
		if (res < 0 || res >= end - pos)
			break;
		pos += res;
	}
	*pos = '\0';

	da16x_drv_prt("[%s] if_indices[%d]:%s\n", __func__, drv->num_if_indices, buf);
}

void add_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx)
{
	int i;
	int *old;

	da16x_drv_prt("[%s] Add own interface ifindex %d\n", __func__, ifidx);

	if (have_ifidx(drv, ifidx)) {
		da16x_drv_prt("[%s] ifindex %d already in the list\n",
			   __func__, ifidx);
		return;
	}
	for (i = 0; i < drv->num_if_indices; i++) {
		if (drv->if_indices[i] == 0) {
			drv->if_indices[i] = ifidx;
			dump_ifidx(drv);
			return;
		}
	}

	if (drv->if_indices != drv->default_if_indices)
		old = drv->if_indices;
	else
		old = NULL;

	drv->if_indices = os_realloc_array(old, drv->num_if_indices + 1,
					   sizeof(int));
	if (!drv->if_indices) {
		if (!old)
			drv->if_indices = drv->default_if_indices;
		else
			drv->if_indices = old;

		da16x_drv_prt("[%s] Failed to reallocate memory for interfaces\n"
			"\tIgnoring EAPOL on interface %d",
						__func__, ifidx);
		return;
	} else if (!old) {
		os_memcpy(drv->if_indices, drv->default_if_indices,
			  sizeof(drv->default_if_indices));
	}

	drv->if_indices[drv->num_if_indices] = ifidx;
	drv->num_if_indices++;
	dump_ifidx(drv);
}

void del_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx)
{
	int i;

	for (i = 0; i < drv->num_if_indices; i++) {
		if (drv->if_indices[i] == ifidx) {
			drv->if_indices[i] = 0;
			break;
		}
	}
	dump_ifidx(drv);
}

int have_ifidx(struct wpa_driver_fc80211_data *drv, int ifidx)
{
	int i;

	for (i = 0; i < drv->num_if_indices; i++)
		if (drv->if_indices[i] == ifidx)
			return 1;

	return 0;
}

#ifdef	CONFIG_AP_WDS
int i802_set_wds_sta(void *priv, const UCHAR *addr, int aid, int val,
			    const char *bridge_ifname, char *ifname_wds)
{
da16x_drv_prt("[%s] What do I do for this ?\n", __func__);
	return 0;
}
#endif	/* CONFIG_AP_WDS */

int i802_check_bridge(struct wpa_driver_fc80211_data *drv,
			     struct i802_bss *bss,
			     const char *brname, const char *ifname)
{
da16x_drv_prt("[%s] What do I do for br_add() ???\n", __func__);

	return 0;
}

void *i802_init(struct hostapd_data *hapd,
		       struct wpa_init_params *params)
{
	da16x_drv_prt("[%s] What do I do for this ???\n", __func__);
	return NULL;
}

void i802_deinit(void *priv)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	wpa_driver_fc80211_deinit(bss);
}

enum fc80211_iftype wpa_driver_fc80211_if_type(
	enum wpa_driver_if_type type)
{
	switch (type) {
	case WPA_IF_STATION:
		return FC80211_IFTYPE_STATION;
	case WPA_IF_P2P_CLIENT:
	case WPA_IF_P2P_GROUP:
		return FC80211_IFTYPE_P2P_CLIENT;
	case WPA_IF_AP_VLAN:
		return FC80211_IFTYPE_AP_VLAN;
	case WPA_IF_AP_BSS:
		return FC80211_IFTYPE_AP;
	case WPA_IF_P2P_GO:
		return FC80211_IFTYPE_P2P_GO;
	case WPA_IF_P2P_DEVICE:
		return FC80211_IFTYPE_P2P_DEVICE;
	case WPA_IF_MESH:
		return FC80211_IFTYPE_MESH_POINT;		
	}

	return FC80211_IFTYPE_UNSPECIFIED;
}

#ifdef CONFIG_P2P

int fc80211_addr_in_use(struct fc80211_global *global, const UCHAR *addr)
{
	struct wpa_driver_fc80211_data *drv;

	dl_list_for_each(drv, &global->interfaces,
			 struct wpa_driver_fc80211_data, list) {
		if (os_memcmp(addr, drv->first_bss->addr, ETH_ALEN) == 0) {
			return 1;
		}
	}

	return 0;
}

int fc80211_p2p_interface_addr(struct wpa_driver_fc80211_data *drv,
				      UCHAR *new_addr)
{
	unsigned int idx;

	if (!drv->global)
		return -1;

	os_memcpy(new_addr, drv->first_bss->addr, ETH_ALEN);
	for (idx = 0; idx < 64; idx++) {
		new_addr[0] = drv->first_bss->addr[0] | 0x02;
		new_addr[0] ^= idx << 2;
		if (!fc80211_addr_in_use(drv->global, new_addr))
			break;
	}
	if (idx == 64)
		return -1;

	da16x_drv_prt("[%s] Assigned new P2P Interface Address " MACSTR "\n",
				__func__, MAC2STR(new_addr));

	return 0;
}
#endif /* CONFIG_P2P */

struct wdev_info {
	ULONG wdev_id;
	int wdev_id_set;
	UCHAR macaddr[ETH_ALEN];
};

int fc80211_wdev_handler(void *arg)
{
	da16x_drv_prt("[%s] Empty Function", __func__);

	return 1;
}

int wpa_driver_fc80211_if_add(void *priv, enum wpa_driver_if_type type,
				     const char *ifname, const UCHAR *addr,
				     void *bss_ctx, void **drv_priv,
				     char *force_ifname, UCHAR *if_addr,
				     const char *bridge, int use_existing)
{
	enum fc80211_iftype iface_dev_type;
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ifidx;
#ifdef	CONFIG_AP
#if 0	/* by Bright */
	int ap_iface_added = 1;
#endif /* 0 */
#endif	/* CONFIG_AP */

	TX_FUNC_START("");

	if (addr)
		os_memcpy(if_addr, addr, ETH_ALEN);

	iface_dev_type = wpa_driver_fc80211_if_type(type);

	if (iface_dev_type == FC80211_IFTYPE_P2P_DEVICE) {
		struct wdev_info p2pdev_info;

		os_memset(&p2pdev_info, 0, sizeof(p2pdev_info));
		ifidx = fc80211_create_iface(drv, ifname, iface_dev_type, addr,
					     0, fc80211_wdev_handler,
					     &p2pdev_info, use_existing);

		if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0) {
			p2pdev_info.wdev_id_set = 1;
			p2pdev_info.wdev_id = 1;
		}

		if (!p2pdev_info.wdev_id_set || ifidx != 0) {
			da16x_drv_prt("[%s] Failed to create a P2P Device "
				"interface %s\n", __func__, ifname);
			return -1;
		}

		drv->global->if_add_wdevid = p2pdev_info.wdev_id;
		drv->global->if_add_wdevid_set = p2pdev_info.wdev_id_set;
		if (!is_zero_ether_addr(p2pdev_info.macaddr))
			os_memcpy(if_addr, p2pdev_info.macaddr, ETH_ALEN);
		da16x_drv_prt("[%s] New P2P Device interface %s (0x%llx) created]\n",
			   __func__, ifname,
			   (long long unsigned int) p2pdev_info.wdev_id);
	} else {
		ifidx = fc80211_create_iface(drv, ifname, iface_dev_type, addr,
					     0, NULL, NULL, use_existing);
#if 0	/* by Bright */
		if (use_existing && ifidx == -ENFILE) {
#ifdef	CONFIG_AP
			ap_iface_added = 0;
#endif	/* CONFIG_AP */
#if 0 //[[ trinity_0150309_BEGIN -- tmp
			da16x_drv_prt("[%s] Set as 0 temporaly\n", __func__);
			ifidx = 0;
#endif //]] trinity_0150309_END -- tmp
		} else
#endif /* 0 */

		if (ifidx < 0) {
			return -1;
		}
	}

	if (!addr) {
		if (drv->nlmode == FC80211_IFTYPE_P2P_DEVICE)
			os_memcpy(if_addr, bss->addr, ETH_ALEN);
		else {
			da16x_drv_prt("[%s] How do I get ifhwaddr ???\n", __func__);
		}
	}


//#ifdef	CONFIG_AP
#if 0 //trinity_0150317
	if (type == WPA_IF_AP_BSS) {
		struct i802_bss *new_bss = os_zalloc(sizeof(*new_bss));
		if (new_bss == NULL) {
			if (ap_iface_added)
				fc80211_remove_iface(drv, ifidx);
			return -1;
		}

		if (bridge &&
		    i802_check_bridge(drv, new_bss, bridge, ifname) < 0) {
			da16x_drv_prt("[%s] Failed to add the new "
				   "interface %s to a bridge %s\n",
				   __func__, ifname, bridge);
			if (ap_iface_added)
				fc80211_remove_iface(drv, ifidx);
			os_free(new_bss);
			return -1;
		}

		da16x_drv_prt("[%s] What do I do for set_iface_flags() ???\n",
						__func__);

		os_strlcpy(new_bss->ifname, ifname, IFNAMSIZ);
		os_memcpy(new_bss->addr, if_addr, ETH_ALEN);
		new_bss->ifindex = ifidx;
		new_bss->drv = drv;
		new_bss->next = drv->first_bss->next;
		new_bss->freq = drv->first_bss->freq;
		new_bss->ctx = bss_ctx;
		new_bss->added_if = ap_iface_added;
		drv->first_bss->next = new_bss;
		if (drv_priv)
			*drv_priv = new_bss;

#if 0 //[[ trinity_0150305_BEGIN -- tmp , CONFIG_AP_TEST
		/* Subscribe management frames for this WPA_IF_AP_BSS */
		if (fc80211_setup_ap(new_bss))
			return -1;
#endif //]] trinity_0150305_END -- tmp
	}
#endif	/* CONFIG_AP */

	if (drv->global)
		drv->global->if_add_ifindex = ifidx;

	if (ifidx > 0)
		add_ifidx(drv, ifidx);

	return 0;
}

int wpa_driver_fc80211_if_remove(struct i802_bss *bss,
					enum wpa_driver_if_type type,
					const char *ifname)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ifindex = 0;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] (type=%d ifname=%s) ifindex=%d added_if=%d\n",
		   __func__, type, ifname, ifindex, bss->added_if);

	if (ifindex > 0 && (bss->added_if || bss->ifindex != ifindex))
		fc80211_remove_iface(drv, ifindex);
	else if (ifindex > 0 && !bss->added_if) {
		struct wpa_driver_fc80211_data *drv2;

		dl_list_for_each(drv2, &drv->global->interfaces,
				 struct wpa_driver_fc80211_data, list)
			del_ifidx(drv2, ifindex);
	//[[ trinity_0150420_BEGIN -- tmp
	} else {
		fc80211_remove_iface(drv, ifindex);
	//]] trinity_0150420_END -- tmp
	}

	if (type != WPA_IF_AP_BSS)
		return 0;

	if (bss != drv->first_bss) {
		struct i802_bss *tbss;

		da16x_drv_prt("[%s] Not the first BSS - remove it\n", __func__);
		for (tbss = drv->first_bss; tbss; tbss = tbss->next) {
			if (tbss->next == bss) {
				tbss->next = bss->next;
#ifdef	CONFIG_AP
				/* Unsubscribe management frames */
				fc80211_teardown_ap(bss);
#endif	/* CONFIG_AP */
				if (!bss->added_if)
					i802_set_iface_flags(bss, 0);
				os_free(bss);
				bss = NULL;
				break;
			}
		}
		if (bss)
			da16x_drv_prt("[%s] could not find BSS %p in the list\n",
						__func__, bss);
	} else {
		da16x_drv_prt("[%s] First BSS - reassign context\n", __func__);

#ifdef	CONFIG_AP
		fc80211_teardown_ap(bss);
#endif	/* CONFIG_AP */

		if (!bss->added_if && !drv->first_bss->next)
			wpa_driver_fc80211_del_beacon(drv);

		if (!bss->added_if)
			i802_set_iface_flags(bss, 0);
		if (drv->first_bss->next) {
			drv->first_bss = drv->first_bss->next;
			drv->ctx = drv->first_bss->ctx;
			os_free(bss);
		} else {
			da16x_drv_prt("[%s] No second BSS to reassign context to\n",
							__func__);
		}
	}

	return 0;
}

int fc80211_send_frame_cmd(struct i802_bss *bss,
				  unsigned int freq, unsigned int wait,
				  const UCHAR *buf, size_t buf_len,
				  ULONG *cookie_out, int no_cck, int no_ack,
				  int offchanok)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	ULONG cookie;
	int ret = -1;

	if (get_run_mode() == 0) {
		da16x_dump("CMD_FRAME", buf, buf_len);
	}

	/* by Bright : FC80211_CMD_FRAME
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_tx_mgmt(bss->wdev_id, drv->ifindex, freq, wait,
				buf, buf_len, offchanok, no_cck, no_ack,
				&cookie);

	if (ret < 0) {
		da16x_drv_prt("[%s] Frame command failed: ret=%d "
				"(%s) (freq=%u wait=%u)\n",
				__func__,
				ret, strerror(-ret), freq, wait);
		goto put_cmd_fail;
	}

	if (cookie_out)
		*cookie_out = no_ack ? (ULONG) -1 : cookie;

	ret = 0;

put_cmd_fail :
	return ret;
}

int wpa_driver_fc80211_send_action(struct i802_bss *bss,
					  unsigned int freq,
					  unsigned int wait_time,
					  const UCHAR *dst, const UCHAR *src,
					  const UCHAR *bssid,
					  const UCHAR *data, size_t data_len,
					  int no_cck)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -1;
	UCHAR *buf;
	struct _ieee80211_hdr *hdr;

	da16x_drv_prt("[%s] Send Action frame (ifindex=%d, freq=%u MHz wait=%d "
			"ms no_cck=%d)\n",
			__func__, drv->ifindex, freq, wait_time, no_cck);

	buf = os_zalloc(24 + data_len);
	if (buf == NULL)
		return ret;
	os_memcpy(buf + 24, data, data_len);
	hdr = (struct _ieee80211_hdr *) buf;
	hdr->frame_control =
		IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_ACTION);
	os_memcpy(hdr->addr1, dst, ETH_ALEN);
	os_memcpy(hdr->addr2, src, ETH_ALEN);
	os_memcpy(hdr->addr3, bssid, ETH_ALEN);

	if (is_ap_interface(drv->nlmode) &&
	    (!(drv->capa.flags & WPA_DRIVER_FLAGS_OFFCHANNEL_TX) ||
	     (int) freq == bss->freq || drv->device_ap_sme ||
	     !drv->use_monitor))
		ret = wpa_driver_fc80211_send_mlme(bss, buf, 24 + data_len,
						   0, freq, no_cck, 1,
						   wait_time);
	else
		ret = fc80211_send_frame_cmd(bss, freq, wait_time, buf,
					     24 + data_len,
					     &drv->send_action_cookie,
					     no_cck, 0, 1);

	os_free(buf);

	return ret;
}

void wpa_driver_fc80211_send_action_cancel_wait(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	TX_FUNC_START("");

	/* by Bright : FC80211_CMD_FRAME_WAIT_CANCEL
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_frame_wait_cancel(drv->ifindex);
	if (ret)
		da16x_drv_prt("[%s] wait cancel failed: ret=%d (%s)\n",
					__func__, ret, strerror(-ret));
}

int wpa_driver_fc80211_remain_on_channel(void *priv, unsigned int freq,
						unsigned int duration)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	ULONG cookie;

	TX_FUNC_START("");

	cookie = 0;

	/* by Bright : FC80211_CMD_REMAIN_ON_CHANNEL
	 *	Direct function call on UMAC layer
	 */
	cookie = fc80211_remain_on_channel(drv->ifindex, freq, duration);
	if (cookie > 0) {
		da16x_drv_prt("[%s] Remain-on-channel cookie "
			   "0x%llx for freq=%u MHz duration=%u\n",
			__func__,
			(long long unsigned int) cookie, freq, duration);

		drv->remain_on_chan_cookie = cookie;
		drv->pending_remain_on_chan = 1;
		return 0;
	}

	da16x_err_prt("[%s] Failed to request remain-on-channel "
		   "(freq=%d duration=%u)\n", __func__, freq, duration);

	return -1;
}

int wpa_driver_fc80211_cancel_remain_on_channel(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;

	TX_FUNC_START("");

	if (!drv->pending_remain_on_chan) {
		da16x_drv_prt("[%s] No pending remain-on-channel to cancel\n",
					__func__);
		return -1;
	}

	da16x_drv_prt("[%s] Cancel remain-on-channel with cookie 0x%llx\n",
			__func__,
			(long long unsigned int) drv->remain_on_chan_cookie);

	/* by Bright : FC80211_CMD_CANCEL_REMAIN_ON_CHANNEL
	 *	Direct function call on UMAC layer
	 */
	ret = fc80211_cancel_remain_on_channel(drv->ifindex,
					drv->remain_on_chan_cookie);
	if (ret == 0)
		return 0;

	da16x_drv_prt("[%s] Failed to cancel remain-on-channel: %d (%s)\n",
				__func__, ret, strerror(-ret));
	return -1;
}

int wpa_driver_fc80211_probe_req_report(struct i802_bss *bss, int report)
{
	if (!report) {
		da16x_drv_prt("[%s] What do I do for this #1 ???\n", __func__);
		return 0;
	}


	if (fc80211_register_frame(bss,
				   (WLAN_FC_TYPE_MGMT << 2) |
				   (WLAN_FC_STYPE_PROBE_REQ << 4),
				   NULL, 0) < 0) {
		goto out_err;
	}

		da16x_drv_prt("[%s] What do I do for this #2 ???\n", __func__);

	return 0;

out_err:
	return -1;
}

int fc80211_disable_11b_rates(struct wpa_driver_fc80211_data *drv,
				     int ifindex, int disabled)
{
	int ret;

	ret = fc80211_set_tx_bitrate_mask(ifindex, disabled);
	if (ret) {
		da16x_drv_prt("[%s] Set TX rates failed: ret=%d (%s)\n",
					__func__, ret, strerror(-ret));
	} else {
		drv->disabled_11b_rates = disabled;
	}

	return ret;
}

int wpa_driver_fc80211_deinit_ap(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	if (!is_ap_interface(drv->nlmode))
		return -1;
	wpa_driver_fc80211_del_beacon(drv);
	bss->beacon_set = 0;

#ifdef	CONFIG_P2P
#ifdef	CONFIG_P2P_POWER_SAVE /* by Shingu 20180208 (P2P PS opt.) */
	prev_p2p_ps_status = -1;
#endif	/* CONFIG_P2P_POWER_SAVE */
	/*
	 * If the P2P GO interface was dynamically added, then it is
	 * possible that the interface change to station is not possible.
	 */
	if (drv->nlmode == FC80211_IFTYPE_P2P_GO && bss->if_dynamic)
		return wpa_driver_fc80211_set_mode(priv, FC80211_IFTYPE_P2P_DEVICE);
	else
#endif	/* CONFIG_P2P */
		return wpa_driver_fc80211_set_mode(priv, FC80211_IFTYPE_STATION);
}

#ifdef	CONFIG_P2P
int wpa_driver_fc80211_deinit_p2p_cli(void *priv)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	/*
	 * If the P2P Client interface was dynamically added, then it is
	 * possible that the interface change to station is not possible.
	 */
	if (bss->if_dynamic)
		return wpa_driver_fc80211_set_mode(priv, FC80211_IFTYPE_P2P_DEVICE);
	else
		return wpa_driver_fc80211_set_mode(priv, FC80211_IFTYPE_STATION);
}
#endif	/* CONFIG_P2P */

void wpa_driver_fc80211_resume(void *priv)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	if (i802_set_iface_flags(bss, 1))
		da16x_drv_prt("[%s] Failed to set interface up on resume event\n",
					__func__);
}

#ifdef CONFIG_IEEE80211R
int fc80211_send_ft_action(void *priv, UCHAR action, const UCHAR *target_ap,
				  const UCHAR *ies, size_t ies_len)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret;
	UCHAR *data, *pos;
	size_t data_len;
	const UCHAR *own_addr = bss->addr;

	TX_FUNC_START("");

	if (action != 1) {
		da16x_drv_prt("[%s] Unsupported send_ft_action action %d\n",
					__func__, action);
		return -1;
	}

	/*
	 * Action frame payload:
	 * Category[1] = 6 (Fast BSS Transition)
	 * Action[1] = 1 (Fast BSS Transition Request)
	 * STA Address
	 * Target AP Address
	 * FT IEs
	 */

	data_len = 2 + 2 * ETH_ALEN + ies_len;
	data = os_malloc(data_len);
	if (data == NULL)
		return -1;
	pos = data;
	*pos++ = 0x06; /* FT Action category */
	*pos++ = action;
	os_memcpy(pos, own_addr, ETH_ALEN);
	pos += ETH_ALEN;
	os_memcpy(pos, target_ap, ETH_ALEN);
	pos += ETH_ALEN;
	os_memcpy(pos, ies, ies_len);

	ret = wpa_driver_fc80211_send_action(bss, drv->assoc_freq, 0,
					     drv->bssid, own_addr, drv->bssid,
					     data, data_len, 0);
	os_free(data);

	return ret;
}
#endif /* CONFIG_IEEE80211R */

int wpa_driver_fc80211_shared_freq(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct wpa_driver_fc80211_data *driver;
	int freq = 0;

	TX_FUNC_START("");

	/*
	 * If the same PHY is in connected state with some other interface,
	 * then retrieve the assoc freq.
	 */
	da16x_drv_prt("[%s] Get shared freq for PHY %s\n", drv->phyname);

	dl_list_for_each(driver, &drv->global->interfaces,
			 struct wpa_driver_fc80211_data, list) {
		if (drv == driver ||
		    os_strcmp(drv->phyname, driver->phyname) != 0 ||
		    !driver->associated)
			continue;

		da16x_drv_prt("[%s] Found a match for PHY %s - %s "
			   MACSTR "\n",
			__func__,
			   driver->phyname, driver->first_bss->ifname,
			   MAC2STR(driver->first_bss->addr));

		if (is_ap_interface(driver->nlmode))
			freq = driver->first_bss->freq;
		else
			freq = fc80211_get_assoc_freq(driver);

		da16x_drv_prt("[%s] Shared freq for PHY %s: %d\n",
			   __func__, drv->phyname, freq);
	}

	if (!freq)
		da16x_drv_prt("[%s] No shared interface for "
			   "PHY (%s) in associated state\n",
					__func__, drv->phyname);

	return freq;
}

int fc80211_send_frame(void *priv, const UCHAR *data, size_t data_len,
			      int encrypt)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return wpa_driver_fc80211_send_frame(bss, data, data_len, encrypt, 0,
					     0, 0, 0, 0);
}

int fc80211_set_param(void *priv, const char *param)
{
	TX_FUNC_START("");

	if (param == NULL) {
		da16x_drv_prt("[%s] driver param is NULL\n", __func__);
		return 0;
	}

#ifdef CONFIG_P2P
	if (os_strstr(param, "use_p2p_group_interface=1")) {
		struct i802_bss *bss = priv;
		struct wpa_driver_fc80211_data *drv = bss->drv;

		da16x_drv_prt("[%s] Use separate P2P group interface\n", __func__);

		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CONCURRENT;
		drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P;
	}

	if (os_strstr(param, "p2p_device=1")) {
		struct i802_bss *bss = priv;
		struct wpa_driver_fc80211_data *drv = bss->drv;
		drv->allow_p2p_device = 1;
	}
#endif /* CONFIG_P2P */

	if (os_strstr(param, "force_connect_cmd=1")) {
		struct i802_bss *bss = priv;
		struct wpa_driver_fc80211_data *drv = bss->drv;
		drv->capa.flags &= ~WPA_DRIVER_FLAGS_SME;
	}

	if (os_strstr(param, "no_offchannel_tx=1")) {
		struct i802_bss *bss = priv;
		struct wpa_driver_fc80211_data *drv = bss->drv;
		drv->capa.flags &= ~WPA_DRIVER_FLAGS_OFFCHANNEL_TX;
		drv->test_use_roc_tx = 1;
	}

	TX_FUNC_END("");

	return 0;
}

void * fc80211_global_init(void)
{
	TX_FUNC_START("");

	global = os_zalloc(sizeof(struct fc80211_global));
	if (global == NULL) {
		da16x_drv_prt("[%s] FINISH : Error\n", __func__);
		return NULL;
	}

	dl_list_init(&global->interfaces);

	global->if_add_ifindex = -1;

	TX_FUNC_END("");

	return global;
}

void fc80211_global_deinit(void *priv)
{
	struct fc80211_global *global = priv;

	TX_FUNC_START("");

	if (global == NULL)
		return;

	if (!dl_list_empty(&global->interfaces)) {
		da16x_drv_prt("[%s] %u interface(s) remain at "
			   "fc80211_global_deinit\n",
			   __func__, dl_list_len(&global->interfaces));
	}

	os_free(global);

	TX_FUNC_END("");
}

const char * fc80211_get_radio_name(void *priv)
{
	struct i802_bss *bss = priv;

	da16x_drv_prt("[%s] phyname=[%s]\n", __func__, bss->drv->phyname);

	return bss->drv->phyname;
}

#ifdef	CONFIG_P2P_POWER_SAVE
int fc80211_set_p2p_powersave(void *priv, int is_noa, u8 count, int duration,
			      int interval, int start, int ctwindow)
{
#if 0	/* by Shingu 20161012 (Not used yet) */
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	if (is_noa) {	/* Notice of Absense */
		da16x_drv_prt("[%s] count=%d duration=%d interval=%d start=%d\n",
				__func__, count, duration, interval, start);

		return fc80211_set_noa(drv->ifindex, count, duration,
				       interval, start);

	} else {	/* Opportunistic Power Save */
		da16x_drv_prt("[%s] ctwindow=%d\n", __func__, ctwindow);

		return fc80211_set_opp_ps(drv->ifindex, ctwindow);
	}
#else
	return 0;
#endif	/* 0 */
}
#endif	/* CONFIG_P2P_POWER_SAVE */

int driver_fc80211_set_key(const char *ifname, void *priv,
				  enum wpa_alg alg, const UCHAR *addr,
				  int key_idx, int set_tx,
				  const UCHAR *seq, size_t seq_len,
				  const UCHAR *key, size_t key_len)
{
	struct i802_bss *bss = priv;
	int ret;

	TX_FUNC_START("");

	ret = wpa_driver_fc80211_set_key(ifname, bss, (int)alg, addr, key_idx,
					  set_tx, seq, seq_len, key, key_len);

	TX_FUNC_END("");

	return ret;
}

int driver_fc80211_abort_scan(void *priv, struct wpa_driver_scan_params *params)
{
	struct i802_bss *bss = priv;

	return wpa_driver_fc80211_abort_scan(bss, params);
}

int driver_fc80211_scan2(void *priv, struct wpa_driver_scan_params *params)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return wpa_driver_fc80211_scan(bss, params);
}

int driver_fc80211_deauthenticate(void *priv, const UCHAR *addr, int reason)
{
	struct i802_bss *bss = priv;

	return wpa_driver_fc80211_deauthenticate(bss, addr, reason);
}

int driver_fc80211_authenticate(void *priv,
				       struct wpa_driver_auth_params *params)
{
	struct i802_bss *bss = priv;

	return wpa_driver_fc80211_auth(bss, params);
}

void driver_fc80211_deinit(void *priv)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	wpa_driver_fc80211_deinit(bss);
}

int driver_fc80211_if_remove(void *priv, enum wpa_driver_if_type type,
				    const char *ifname)
{
	struct i802_bss *bss = priv;

	return wpa_driver_fc80211_if_remove(bss, type, ifname);
}

#if 0
int driver_fc80211_send_mlme(void *priv, const UCHAR *data,
				    size_t data_len, int noack)
#else				    
int driver_fc80211_send_mlme(void *priv, const UCHAR *data, size_t data_len,
				 int noack, unsigned int freq, const u16 *csa_offs,
				 size_t csa_offs_len)
#endif				    
{
	struct i802_bss *bss = priv;

	if (get_run_mode() == 0) {
		TX_FUNC_START("");
	}

#ifdef	CONFIG_P2P
	if (get_run_mode() == P2P_ONLY_MODE
#ifdef	CONFIG_CONCURRENT
		|| get_run_mode() == STA_P2P_CONCURRENT_MODE
#endif	/* CONFIG_CONCURRENT */
		|| get_run_mode() == P2P_GO_FIXED_MODE) {
	return wpa_driver_fc80211_send_mlme(bss, data, data_len, noack,
				0, 0, 0, 0);//DEFAULT_P2P_LISTEN_TIME);
	} else {
	return wpa_driver_fc80211_send_mlme(bss, data, data_len, noack,
				0, 0, 0, 0);
	}
#else	/* CONFIG_P2P */
	return wpa_driver_fc80211_send_mlme(bss, data, data_len, noack, freq, 0, 0, 0);
#endif	/* CONFIG_P2P */

}

int driver_fc80211_sta_remove(void *priv, const UCHAR *addr)
{
	struct i802_bss *bss = priv;

	return wpa_driver_fc80211_sta_remove(bss, addr);
}

#ifdef CONFIG_AP_VLAN
int driver_fc80211_set_sta_vlan(void *priv, const UCHAR *addr,
				       const char *ifname, int vlan_id)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return i802_set_sta_vlan(bss, addr, ifname, vlan_id);
}
#endif

#ifdef CONFIG_STA_POWER_SAVE
int fc80211_sta_pwrsave(void *priv, int pwrsave_mode)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	fc80211_set_sta_power_save(drv->ifindex, pwrsave_mode);
	return 0;
}
#endif /* CONFIG_STA_POWER_SAVE */

#ifdef CONFIG_AP_NONE_STA
int driver_fc80211_ap_none_station(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	TX_FUNC_START("");

	return fc80211_none_station(drv->ifindex);
}
#endif /* CONFIG_AP_NONE_STA */

#ifdef CONFIG_AP_POWER
int driver_fc80211_set_ap_power(void *priv, int type, int power, int *get_power)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return i802_set_ap_power(bss, type, power, get_power);
}
#endif /* CONFIG_AP_POWER */

int driver_fc80211_read_sta_data(void *priv,
					struct hostap_sta_driver_data *data,
					const UCHAR *addr)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return i802_read_sta_data(bss, data, addr);
}

int driver_fc80211_send_action(void *priv, unsigned int freq,
				      unsigned int wait_time,
				      const UCHAR *dst, const UCHAR *src,
				      const UCHAR *bssid,
				      const UCHAR *data, size_t data_len,
				      int no_cck)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return wpa_driver_fc80211_send_action(bss, freq, wait_time, dst, src,
					      bssid, data, data_len, no_cck);
}

int driver_fc80211_probe_req_report(void *priv, int report)
{
	struct i802_bss *bss = priv;

	TX_FUNC_START("");

	return wpa_driver_fc80211_probe_req_report(bss, report);
}

const UCHAR *wpa_driver_fc80211_get_macaddr(void *priv)
{
	struct i802_bss *bss = priv;
#ifdef	ENABLE_DRV_DBG
	struct wpa_driver_fc80211_data *drv = bss->drv;
#endif	/* ENABLE_DRV_DBG */

	TX_FUNC_START("");

#ifdef	ENABLE_DRV_DBG
	da16x_drv_prt("[%s] mode=%d, mac_addr=[" MACSTR "]\n",
			__func__, drv->nlmode,
			bss->addr[0], bss->addr[1], bss->addr[2],
			bss->addr[3], bss->addr[4], bss->addr[5]);
#endif	/* ENABLE_DRV_DBG */

#if 0	/* by Bright */
	if (drv->nlmode != FC80211_IFTYPE_P2P_DEVICE) {
		da16x_drv_prt("[%s] FINISH : #1\n", __func__);
		return NULL;
	}
#endif	/* by Bright */

	TX_FUNC_END("");

	return bss->addr;
}

#ifdef CONFIG_WNM_SLEEP_MODE
int fc80211_wnm_operation(void *priv,   enum wnm_oper oper, const u8 *peer, u8 *buf, u16 *buf_len)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	int ret = -EPERM;

	TX_FUNC_START("");

	da16x_drv_prt("[%s] WNM Operation Request oper = %d\n",
		__func__, oper);

	ret = fc80211_set_wnm_sleep_mode(oper, drv->ifindex, drv->wnm_intval);
	if (ret) {
		da16x_drv_prt("[%s] Sleep mode operation %d failed err=%d (%s)\n",
			   __func__, oper, ret, strerror(-ret));
	}

	return ret;
}
#endif /* CONFIG_WNM_SLEEP_MODE */

#ifdef CONFIG_ACS
static void add_survey(u32 ifidx, struct supp_survey_info get_survey,
		       struct dl_list *survey_list)
{
	struct freq_survey *survey;
	survey = os_zalloc(sizeof(struct freq_survey));
	if  (!survey)
		return;

	survey->ifidx = ifidx;
	survey->freq = get_survey.channel->cent_freq;
	survey->filled = 0;

	survey->nf = get_survey.noise;
	survey->filled |= SURVEY_HAS_NF;

	survey->channel_time = get_survey.channel_time;
	survey->filled |= SURVEY_HAS_CHAN_TIME;

	survey->channel_time_busy = get_survey.channel_time_busy;
	survey->filled |= SURVEY_HAS_CHAN_TIME_BUSY;

	da16x_drv_prt("[%s] nld11: Freq survey dump event (freq=%d MHz noise=%d "\
					"channel_time=%ld busy_time=%ld tx_time=%ld rx_time=%ld filled=%04x)\n",
			__func__,
		   survey->freq,
		   survey->nf,
		   (unsigned long int) survey->channel_time,
		   (unsigned long int) survey->channel_time_busy,
		   (unsigned long int) survey->channel_time_tx,
		   (unsigned long int) survey->channel_time_rx,
		   survey->filled);

	dl_list_add_tail(survey_list, &survey->list);
}

static void clean_survey_results(struct survey_results *survey_results)
{
	struct freq_survey *survey, *tmp;

	if (dl_list_empty(&survey_results->survey_list))
		return;

	dl_list_for_each_safe(survey, tmp, &survey_results->survey_list,
			      struct freq_survey, list) {
		dl_list_del(&survey->list);
		os_free(survey);
	}
}

int fc80211_get_dump_survey(void *priv, unsigned int channel)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	struct supp_survey_info get_survey;

	int err = -ENOBUFS;
	int idx, idx_num;

	TX_FUNC_START("");

	union wpa_event_data data;
	struct survey_results *survey_results;

	os_memset(&data, 0, sizeof(data));
	survey_results = &data.survey_results;

	dl_list_init(&survey_results->survey_list);

	if (channel == 0)
		idx_num = 13;
	else
		idx_num = channel - 1;

	for (idx = 0; idx <= idx_num; idx++) {
		err = fc80211_dump_survey(drv->ifindex, idx, &get_survey);
		add_survey(drv->ifindex, get_survey, &survey_results->survey_list);
	}

	wpa_supplicant_event(drv->ctx, EVENT_SURVEY, &data);

	clean_survey_results(survey_results);

	return err;
}
#endif /* CONFIG_ACS */

void send_scan_event(struct wpa_driver_fc80211_data *drv,
                        int aborted, da16x_drv_msg_buf_t *drv_msg_buf)
{
	union wpa_event_data event;
	struct scan_info *info;
	u8 *data_ptr = (u8 *)drv_msg_buf->data->data;
	int i;
#define MAX_REPORT_FREQS	24
	u16 freqs[MAX_REPORT_FREQS];

	TX_FUNC_START("");

	da16x_drv_prt("[%s] scan_for_auth=%d\n",
					__func__, drv->scan_for_auth);

	if (drv->scan_for_auth) {
		drv->scan_for_auth = 0;

		da16x_drv_prt("<%s> Scan results for missing "
			"cfgd11 BSS entry\n", __func__);

		return;
	}

	os_memset(&event, 0, sizeof(event));
	info = &event.scan_info;
	info->aborted = aborted;

	if (drv_msg_buf->data->attr.n_ssids > 0) {
		da16x_drv_prt("<%s> n_ssids=%d\n",
				__func__, drv_msg_buf->data->attr.n_ssids);

		for (i = 0; i < drv_msg_buf->data->attr.n_ssids; i++) {
			struct wpa_driver_scan_ssid *s =
					&info->ssids[info->num_ssids];
			s->ssid_len = *data_ptr++;
			s->ssid = data_ptr;
			data_ptr += s->ssid_len;
			da16x_drv_prt("<%s> Scan probed for SSID '%s'\n",
				__func__, wpa_ssid_txt(s->ssid, s->ssid_len));
		}
	}

	if (drv_msg_buf->data->attr.n_channels > 0) {
		char msg[200], *pos, *end;
		int res;

		pos = msg;
		end = pos + sizeof(msg);
		*pos = '\0';

		for (i = 0; i < drv_msg_buf->data->attr.n_channels; i++) {
			memcpy((void *)&freqs[i], (u16 *)data_ptr,  sizeof(u16));
			res = os_snprintf(pos, end - pos, " %d", freqs[i]);
			if (res > 0 && end - pos > res)
				pos += res;
			if (i == MAX_REPORT_FREQS - 1)
				break;

			data_ptr += 2;
		}

		info->freqs = (const int *)freqs;
		info->num_freqs = i;
		da16x_drv_prt("<%s> Scan frequencies :%s\n", __func__, msg);
	}

	wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS, &event);

	TX_FUNC_END("");
}

void mlme_event(struct wpa_driver_fc80211_data *drv,
                    struct i802_bss *bss, da16x_drv_msg_buf_t *drv_msg_buf)
{
	const u8 *data;
	size_t len;
	int timed_out = drv_msg_buf->data->attr.timed_out;

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_START("");
	}

	if (timed_out && drv_msg_buf->data->attr.addr) {
		mlme_timeout_event(drv,drv_msg_buf);
		return;
	}

	data = (const u8 *)(drv_msg_buf->data->data);
	len = drv_msg_buf->data->data_len;

	if (len == 0) {
		da16x_drv_prt("<%s> MLME event cmd=%d without frame data\n",
			__func__, drv_msg_buf->cmd);
		return;
	}

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		da16x_dump("MLME event frame", data, len);
	}

	if (len < 4 + 2 * ETH_ALEN) {
		da16x_drv_prt("<%s> MLME event cmd=%d on %s("
			MACSTR ") - too short\n",
				__func__, drv_msg_buf->cmd,
				bss->ifname, MAC2STR(bss->addr));
		return;
	}

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		da16x_drv_prt("<%s> MLME event cmd=%d on %s(" MACSTR ") A1="
						MACSTR " A2=" MACSTR "\n",
				__func__,
				drv_msg_buf->cmd,
				bss->ifname, MAC2STR(bss->addr),
				MAC2STR(data + 4),
				MAC2STR(data + 4 + ETH_ALEN));
	}

	if (drv_msg_buf->cmd != FC80211_CMD_FRAME_TX_STATUS
	    && !(data[4] & 0x01)
	    && os_memcmp(bss->addr, data + 4, ETH_ALEN) != 0
	    && os_memcmp(bss->addr, data + 4 + ETH_ALEN, ETH_ALEN) != 0) {
		da16x_drv_prt("<%s>: %s: Ignore MLME frame event for "
			 "foreign address\n",
			__func__, bss->ifname);
		da16x_drv_prt("[%02x:%02x:%02x:%02x:%02x:%02x] "
				"<%02x:%02x:%02x:%02x:%02x:%02x]>\n",
			bss->addr[0], bss->addr[1], bss->addr[2],
			bss->addr[3], bss->addr[4], bss->addr[5],
			data[4], data[5], data[6],
			data[7], data[8], data[9]);
		return;
	}

	switch (drv_msg_buf->cmd) {
	case FC80211_CMD_AUTHENTICATE:		// 37
		mlme_event_auth(drv, data, len);
		break;
	case FC80211_CMD_ASSOCIATE:		// 38
		mlme_event_assoc(drv, data, len);
		break;
	case FC80211_CMD_DEAUTHENTICATE:	// 39
		mlme_event_deauth_disassoc(drv, EVENT_DEAUTH, data, len);
		break;
	case FC80211_CMD_DISASSOCIATE:		// 40
		mlme_event_deauth_disassoc(drv, EVENT_DISASSOC, data, len);
		break;
	case FC80211_CMD_FRAME:
		mlme_event_mgmt(bss, drv_msg_buf, data, len);
		break;
	case FC80211_CMD_FRAME_TX_STATUS:
		mlme_event_mgmt_tx_status(drv, drv_msg_buf, data, len);
		break;
#ifdef CONFIG_IEEE80211W
	case FC80211_CMD_UNPROT_DEAUTHENTICATE:
		mlme_event_unprot_disconnect(drv, EVENT_UNPROT_DEAUTH,
					     data, len);
		break;
	case FC80211_CMD_UNPROT_DISASSOCIATE:
		mlme_event_unprot_disconnect(drv, EVENT_UNPROT_DISASSOC,
					     data, len);
		break;
#endif
	default:
		break;
	}

#if 1
	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) 
#endif /* 1 */

	RX_FUNC_END("");
}

#ifdef	CONFIG_MIC_FAILURE_RSP
#ifdef CONFIG_SIMULATED_MIC_FAILURE
static void wpa_supplicant_send_mic_failure_event(void *eloop_ctx, void *sock_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	union wpa_event_data	*event = sock_ctx;
	wpa_supplicant_event(wpa_s, EVENT_MICHAEL_MIC_FAILURE, &mic_event);
}
#endif /* CONFIG_SIMULATED_MIC_FAILURE */

static void mlme_event_michael_mic_failure(
			struct wpa_driver_fc80211_data *drv,
			struct i802_bss *bss,
			da16x_drv_msg_buf_t *drv_msg_buf)
{
    union wpa_event_data	event;
	da16x_drv_event_attr_t	*attr;

	RX_FUNC_START("");

	da16x_drv_prt("<%s> MLME event Michael MIC failure...\n", __func__);

	attr = (da16x_drv_event_attr_t *)(&(drv_msg_buf->data->attr));

	da16x_dump("MICHAEL_MIC_FAILURE event frame",
			(const u8 *)attr, sizeof(da16x_drv_event_attr_t));

#ifdef CONFIG_SIMULATED_MIC_FAILURE
	memset(&mic_event, 0, sizeof(mic_event));
#else
	memset(&event, 0, sizeof(event));
#endif /* CONFIG_SIMULATED_MIC_FAILURE */

	/* attr src mac_addr */
	if (attr->addr[0] != 0x0) {
		da16x_dump("MICHAEL_MIC_FAILURE : Source MAC address",
				attr->addr, ETH_ALEN);

#ifdef CONFIG_SIMULATED_MIC_FAILURE
		mic_event.michael_mic_failure.src  = os_zalloc(sizeof(u8 *) * ETH_ALEN);
		os_memcpy((void *)mic_event.michael_mic_failure.src ,(void *) attr->addr, ETH_ALEN);
#else
		event.michael_mic_failure.src = attr->addr;
#endif /* CONFIG_SIMULATED_MIC_FAILURE */
	}

	/* attr key_type */
	if (attr->n_channels) {
		enum fc80211_key_type key_type =
				(enum fc80211_key_type)attr->n_channels;

		da16x_drv_prt("<%s> Key Type %d\n", __func__, key_type);

		if (key_type == FC80211_KEYTYPE_PAIRWISE)
#ifdef CONFIG_SIMULATED_MIC_FAILURE
			mic_event.michael_mic_failure.unicast = 1;
#else
			event.michael_mic_failure.unicast = 1;
#endif /* CONFIG_SIMULATED_MIC_FAILURE */
	} else {
#ifdef CONFIG_SIMULATED_MIC_FAILURE
		mic_event.michael_mic_failure.unicast = 1;
#else
		event.michael_mic_failure.unicast = 1;
#endif /* CONFIG_SIMULATED_MIC_FAILURE */
	}

#ifdef CONFIG_SIMULATED_MIC_FAILURE
	da16x_eloop_register_timeout(30, 0,
				wpa_supplicant_send_mic_failure_event,
				drv->ctx,
				&mic_event);
#else
	wpa_supplicant_event(drv->ctx, EVENT_MICHAEL_MIC_FAILURE, &event);
#endif /* CONFIG_SIMULATED_MIC_FAILURE */
	RX_FUNC_END("");
}
#endif	/* CONFIG_MIC_FAILURE_RSP */

int driver_fc80211_get_rtctimegap(void *priv, u64 *rtctimegap)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	extern int fc80211_get_rtctimegap(int ifindex, u64 *rtctimegap);

	TX_FUNC_START("");

	return fc80211_get_rtctimegap(drv->ifindex, rtctimegap);
}

void fc80211_set_ampdu_mode(int mode, int val)
{
	fc80211_set_ampdu_flag(mode, val);
}

int fc80211_get_ampdu_mode(int mode)
{
	return fc80211_get_ampdu_flag(mode);
}

#ifdef CONFIG_SIMPLE_ROAMING
int fc80211_set_threshold(int min_thold, int max_thold)
{
	return fc80211_enable_rssi_report(0, min_thold, max_thold);
}

void fc80211_set_roaming_mode(int mode)
{
	fc80211_set_roaming_flag(mode);
}
#define GREEN_COLOR		"\33[1;32m"
#define CLEAR_COLOR		"\33[0m"
static void simple_roam_trigger(void *eloop_ctx, int current_rssi)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

/* DPM mode not support ROAM*/
	if (chk_dpm_mode() == pdTRUE) {
		wpa_s->conf->roam_state = ROAM_DISABLE;
		return;
	}

	if (wpa_s->wpa_state != WPA_COMPLETED)
		return;

	if (wpa_s->conf->roam_state == ROAM_ENABLE) {
		fc80211_set_roaming_mode(0);
		wpa_s->conf->roam_state = ROAM_SCANNING;

		PRINTF(GREEN_COLOR"\n>>> [Roaming] Start - Current signal level "
				"is lower then the threshold.\n" CLEAR_COLOR);

		wpa_supplicant_req_scan(wpa_s, 1, 0);
	}
}
#endif /* CONFIG_SIMPLE_ROAMING */


#if 1 /* Debug */
const char * fc80211_cmd_to_string(enum fc80211_commands cmd)
{
#define C2S(n) case FC80211_CMD_ ## n: return #n
	switch (cmd) {
		/* don't change the order or add anything between); this is ABI! */
		C2S(UNSPEC);
	
		C2S(GET_SOFTMAC); 	/* can dump */
		C2S(SET_SOFTMAC);
		C2S(NEW_SOFTMAC);
		C2S(DEL_SOFTMAC);
	
		C2S(GET_INTERFACE);	/* can dump */
		C2S(SET_INTERFACE);
		C2S(NEW_INTERFACE);
		C2S(DEL_INTERFACE);
	
		C2S(GET_KEY);
		C2S(SET_KEY);		// 10
		C2S(NEW_KEY);
		C2S(DEL_KEY);
	
		C2S(GET_BEACON);
		C2S(SET_BEACON);
		C2S(START_AP);
#if 0
		C2S(NEW_BEACON); // = C2S(START_AP);
#endif /* 0 */
		C2S(STOP_AP);
#if 0
		C2S(DEL_BEACON); // = C2S(STOP_AP);
#endif /* 0 */
	
		C2S(GET_STATION);
		C2S(SET_STATION);	//
		C2S(NEW_STATION);
		C2S(DEL_STATION);	// 20
	
		C2S(GET_MPATH);
		C2S(SET_MPATH);
		C2S(NEW_MPATH);
		C2S(DEL_MPATH);
	
		C2S(SET_BSS);
	
	/*** Unused Commad in DA16X ***/
		C2S(SET_REG);
		C2S(REQ_SET_REG);
	
		C2S(GET_MESH_CONFIG);
		C2S(SET_MESH_CONFIG);
	/******************************/
	
#if 0
		C2S(SET_MGMT_EXTRA_IE); /* 30 : reserved; not used */);
#endif /* 0 */
	
	/*** Unused Commad in DA16X ***/
		C2S(GET_REG);
	/******************************/
	
		C2S(GET_SCAN);
		C2S(TRIGGER_SCAN);
		C2S(NEW_SCAN_RESULTS);
		C2S(SCAN_ABORTED);	// 35
	
	/*** Unused Commad in DA16X ***/
		C2S(REG_CHANGE);
	/******************************/
	
		C2S(AUTHENTICATE);
		C2S(ASSOCIATE);
		C2S(DEAUTHENTICATE);
		C2S(DISASSOCIATE);	// 40
	
		C2S(MICHAEL_MIC_FAILURE);
	
	/*** Unused Commad in DA16X ***/
		C2S(REG_BEACON_HINT);
	/******************************/
	
		C2S(JOIN_IBSS);
		C2S(LEAVE_IBSS);
	
		C2S(TESTMODE);		// 45
	
		C2S(CONNECT);
		C2S(ROAM);
		C2S(DISCONNECT);
	
		C2S(SET_SOFTMAC_NETNS);
	
		C2S(GET_SURVEY);		// 50
		C2S(NEW_SURVEY_RESULTS);
	
	/*** Unused Commad in DA16X ***/
		C2S(SET_PMKSA);
		C2S(DEL_PMKSA);
		C2S(FLUSH_PMKSA);
	/******************************/
	
		C2S(REMAIN_ON_CHANNEL); // 55
		C2S(CANCEL_REMAIN_ON_CHANNEL);
	
		C2S(SET_TX_BITRATE_MASK);
	
		C2S(REGISTER_FRAME);
#if 0
		C2S(REGISTER_ACTION); //	= C2S(REGISTER_FRAME);
#endif /* 0 */
		C2S(FRAME);
#if 0
		C2S(ACTION); // 	= C2S(FRAME);
#endif /* 0 */
		C2S(FRAME_TX_STATUS);
#if 0
		C2S(ACTION_TX_STATUS); //	= C2S(FRAME_TX_STATUS); // 60
#endif /* 0 */
	
		C2S(SET_POWER_SAVE);
		C2S(GET_POWER_SAVE);
	
		C2S(SET_CQM);
		C2S(NOTIFY_CQM);
	
		C2S(SET_CHANNEL);	// 65
		C2S(SET_WDS_PEER);
	
		C2S(FRAME_WAIT_CANCEL);
	
		C2S(JOIN_MESH);
		C2S(LEAVE_MESH);
	
		C2S(UNPROT_DEAUTHENTICATE);	// 70
		C2S(UNPROT_DISASSOCIATE);
	
		C2S(NEW_PEER_CANDIDATE);
	
	/*** Unused Commad in DA16X ***/
		C2S(GET_WOWLAN);
		C2S(SET_WOWLAN);
	
		C2S(START_SCHED_SCAN);		// 75
		C2S(STOP_SCHED_SCAN);
		C2S(SCHED_SCAN_RESULTS);
		C2S(SCHED_SCAN_STOPPED);
	/******************************/
	
		C2S(SET_REKEY_OFFLOAD);
	
	/*** Unused Commad in DA16X ***/
		C2S(PMKSA_CANDIDATE);		// 80
	
		C2S(TDLS_OPER);
		C2S(TDLS_MGMT);
	/******************************/
	
		C2S(UNEXPECTED_FRAME);
		C2S(PROBE_CLIENT);
		C2S(REGISTER_BEACONS);		// 85
		C2S(UNEXPECTED_4ADDR_FRAME);
		C2S(SET_NOACK_MAP);
		C2S(CH_SWITCH_NOTIFY);
		C2S(START_P2P_DEVICE);
		C2S(STOP_P2P_DEVICE);		// 90
		C2S(CONN_FAILED);
		C2S(SET_MCAST_RATE);
		C2S(SET_MAC_ACL);
		C2S(RADAR_DETECT);
		C2S(GET_PROTOCOL_FEATURES);	// 95
		C2S(UPDATE_FT_IES);
		C2S(FT_EVENT);
		C2S(CRIT_PROTOCOL_START);
		C2S(CRIT_PROTOCOL_STOP);
		C2S(GET_COALESCE);		// 100
		C2S(SET_COALESCE);
		C2S(CHANNEL_SWITCH);
		C2S(VENDOR);
		C2S(SET_QOS_MAP);

	}

	return "UNKNOWN";
#undef C2S
}
#endif /* debug */

#ifdef CONFIG_MESH
struct mesh_peer_s {
	u8 peer[6];
	const u8 *ies;
	size_t ie_len;
} mesh_peer_s;

static void fc80211_new_peer_candidate(
			struct wpa_driver_fc80211_data *drv,
			da16x_drv_msg_buf_t *drv_msg_buf)
{
	const u8 *addr;
	union wpa_event_data data;
	struct mesh_peer_s temp;

	memcpy(&temp, &drv_msg_buf->data->data, sizeof(struct mesh_peer_s));

	os_memset(&data, 0, sizeof(data));
	data.mesh_peer.peer = (const u8 *)&temp.peer;
	data.mesh_peer.ies = temp.ies;
	data.mesh_peer.ie_len = temp.ie_len;

	wpa_supplicant_event(drv->ctx, EVENT_NEW_PEER_CANDIDATE, &data); 
	os_free((void *)temp.ies);
}
#ifdef FEATURE_MESH_FREERTOS
#else
struct cfgd11_chan_def {
	struct i3ed11_channel *chan;
	enum fc80211_chan_width width;
	u32 cent_freq1;
	u32 cent_freq2;
};
#endif
static void fc80211_mlme_event_ch_switch(
			struct wpa_driver_fc80211_data *drv,
			da16x_drv_msg_buf_t *drv_msg_buf)
{
	const u8 *addr;
	union wpa_event_data data;
	struct cfgd11_chan_def temp;
	int ht_enabled = 1;
	int chan_offset = 0;

	memcpy(&temp, &drv_msg_buf->data->data, sizeof(struct cfgd11_chan_def));

	if(temp.width){
		enum fc80211_channel_type ch_type = temp.width;
		wpa_printf_dbg(MSG_DEBUG, "nld11: Channel type: %d", ch_type);
		
		switch (ch_type) {
			case FC80211_CHAN_NO_HT:
				ht_enabled = 0;
				break;
			case FC80211_CHAN_HT20:
				break;
			case FC80211_CHAN_HT40PLUS:
				chan_offset = 1;
				break;
			case FC80211_CHAN_HT40MINUS:
				chan_offset = -1;
				break;
		}
	} 
		
	os_memset(&data, 0, sizeof(data));
	data.ch_switch.freq = temp.cent_freq1;
	data.ch_switch.ht_enabled = ht_enabled;
	data.ch_switch.ch_offset = 0;
	data.ch_switch.ch_width = temp.width;
	data.ch_switch.cf1 = temp.cent_freq1;
	data.ch_switch.cf2 =  temp.cent_freq2;

//	bss->freq = data.ch_switch.freq;
	drv->assoc_freq = data.ch_switch.freq;

	wpa_supplicant_event(drv->ctx, EVENT_CH_SWITCH, &data); 	
}
#endif

extern void delete_scan_result_timer(void);
extern void delete_scan_rslt_timer(void);
extern void supp_scan_result_event(void *eloop_ctx, void *timeout_ctx);

void process_drv_event(struct i802_bss *bss, da16x_drv_msg_buf_t *drv_msg_buf)
{
	struct wpa_driver_fc80211_data *drv = bss->drv;
#ifdef	ENABLE_DRV_DBG
	int	cnt;
#endif	/* ENABLE_DRV_DBG */

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_START("");
	}

	if (drv->ap_scan_as_station != FC80211_IFTYPE_UNSPECIFIED &&
	    (drv_msg_buf->cmd == FC80211_CMD_NEW_SCAN_RESULTS ||
	     drv_msg_buf->cmd == FC80211_CMD_SCAN_ABORTED)) {
		wpa_driver_fc80211_set_mode(drv->first_bss,
					    drv->ap_scan_as_station);
		drv->ap_scan_as_station = FC80211_IFTYPE_UNSPECIFIED;
	}

#if 1 // debug
#ifdef CHECK_SUPPLICANT_ERR
#include "common_utils.h"
#endif /*CHECK_SUPPLICANT_ERR*/

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		da16x_ev_prt(ANSI_COLOR_LIGHT_BLUE "[%s] CMD %s (%d) received\n" ANSI_COLOR_DEFULT, __func__,
			 fc80211_cmd_to_string(drv_msg_buf->cmd), drv_msg_buf->cmd);	
	}
#else
	da16x_drv_prt("<%s> cmd=%d\n", __func__, drv_msg_buf->cmd);

#endif /* 1 */

	switch(drv_msg_buf->cmd) {
		case FC80211_CMD_TRIGGER_SCAN :
			da16x_drv_prt("<%s> FC80211_CMD_TRIGGER_SCAN: ctx=0x%x\n",
						__func__, drv->ctx);

			drv->scan_state = SCAN_STARTED;
			if (drv->scan_for_auth) {
				/*
				 * Cannot indicate EVENT_SCAN_STARTED here
				 *	since we skip EVENT_SCAN_RESULTS
				 *	in scan_for_auth case and the
				 *	upper layer implementation could
				 *	get confused about scanning state.
				 */
				da16x_drv_prt("[%s] Do not indicate scan-start "
					"event due to internal "
					"scan_for_auth\n", __func__);
				break;
			}
			wpa_supplicant_event(drv->ctx, EVENT_SCAN_STARTED, NULL);
			break;

		case FC80211_CMD_SCAN_ABORTED:
			da16x_drv_prt("<%s> FC80211_CMD_NEW_SCAN_ABORTED\n", __func__); /* fall through */

		case FC80211_CMD_NEW_SCAN_RESULTS :
			da16x_drv_prt("<%s> FC80211_CMD_NEW_SCAN_RESULTS\n", __func__);

			drv->scan_state = SCAN_COMPLETED;
			drv->scan_complete_events = 1;

			/* by Bright :
			 *	delete scan result timer if exist */
#ifdef	ENABLE_DRV_DBG
			cnt = da16x_eloop_cancel_timeout(supp_scan_result_event, drv->ctx, NULL);
			da16x_drv_prt("<%s> FC80211_CMD_NEW_SCAN_RESULTS : cnt=%d\n", __func__, cnt);
#else
			da16x_eloop_cancel_timeout(supp_scan_result_event, drv->ctx, NULL);
#endif	/* ENABLE_DRV_DBG */

			send_scan_event(drv, 0, drv_msg_buf);

			break;

		case FC80211_CMD_AUTHENTICATE :            /* fall through */
		case FC80211_CMD_ASSOCIATE :               /* fall through */
		case FC80211_CMD_DEAUTHENTICATE :          /* fall through */
		case FC80211_CMD_DISASSOCIATE :            /* fall through */
		case FC80211_CMD_FRAME :                   /* fall through */
		case FC80211_CMD_FRAME_TX_STATUS :         /* fall through */
		case FC80211_CMD_UNPROT_DEAUTHENTICATE :   /* fall through */
		case FC80211_CMD_UNPROT_DISASSOCIATE :
			mlme_event(drv, bss, drv_msg_buf);
			break;

#ifdef	CONFIG_MIC_FAILURE_RSP
		case FC80211_CMD_MICHAEL_MIC_FAILURE :
			mlme_event_michael_mic_failure(drv, bss, drv_msg_buf);
			break;
#endif	/* CONFIG_MIC_FAILURE_RSP */

		case FC80211_CMD_GET_BEACON:
			da16x_drv_prt("<%s> FC80211_CMD_GET_BEACON\n", __func__);
#ifdef CONFIG_SIMPLE_ROAMING
			simple_roam_trigger(drv->ctx, 0);
#endif /* CONFIG_SIMPLE_ROAMING */
			break;

#ifdef CONFIG_MESH
		case FC80211_CMD_NEW_PEER_CANDIDATE:
			da16x_drv_prt("<%s> FC80211_CMD_NEW_PEER_CANDIDATE\n", __func__);
			fc80211_new_peer_candidate(drv, drv_msg_buf);
			break;

		case FC80211_CMD_CH_SWITCH_NOTIFY:
			da16x_drv_prt("<%s> FC80211_CMD_CH_SWITCH_NOTIFY\n", __func__);
			fc80211_mlme_event_ch_switch(drv, drv_msg_buf);
			break;
#endif /* CONFIG_MESH */

		default:
			da16x_drv_prt("[%s] Ignored unknown event (cmd=%d)\n",
					__func__, drv_msg_buf->cmd);
			break;
	}

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_END("");
	}
}

void driver_fc80211_process_global_ev(da16x_drv_msg_buf_t *drv_msg_buf)
{
	struct wpa_driver_fc80211_data *drv=NULL, *tmp=NULL;
	struct i802_bss *bss=NULL;
	extern SemaphoreHandle_t da16x_sp_drv_ev_blk_pool_lock;

	if(drv_msg_buf == NULL)
		return;

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_START("");
	}

#ifdef	CONFIG_CONCURRENT
	switch (get_run_mode()) {
#ifdef	CONFIG_P2P
		case STA_P2P_CONCURRENT_MODE :
			dl_list_for_each_safe(drv, tmp, &global->interfaces,
			      struct wpa_driver_fc80211_data, list) {
				for (bss = drv->first_bss; bss; bss = bss->next) {
					if(bss->ifindex == drv_msg_buf->data->attr.softmac_idx)
						process_drv_event(bss, drv_msg_buf);
				}
			}
			break;
#endif	/* CONFIG_P2P */

		default:
			dl_list_for_each_safe(drv, tmp, &global->interfaces,
				      struct wpa_driver_fc80211_data, list) {
			for (bss = drv->first_bss; bss; bss = bss->next) {
				if(bss->ifindex == drv_msg_buf->data->attr.softmac_idx)
					process_drv_event(bss, drv_msg_buf);
				}
			}
			break;
	}
#else
	dl_list_for_each_safe(drv, tmp, &global->interfaces,
			      struct wpa_driver_fc80211_data, list) {
		for (bss = drv->first_bss; bss; bss = bss->next) {
			if(bss->ifindex == drv_msg_buf->data->attr.softmac_idx)
				process_drv_event(bss, drv_msg_buf);
		}
	}
#endif	/* CONFIG_CONCURRENT */

	if(drv_msg_buf->data != NULL)
	os_free(drv_msg_buf->data);

	if (get_run_mode() == 0 || drv_msg_buf->cmd != FC80211_CMD_FRAME) {
		RX_FUNC_END("");
	}
}


#ifdef CONFIG_MESH
int wpa_driver_fc80211_init_mesh(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	return fc80211_init_mesh(drv->ifindex);
}

int wpa_driver_fc80211_join_mesh(void *priv, struct wpa_driver_mesh_join_params *params)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	return fc80211_join_mesh(drv->ifindex , params);
}

int wpa_driver_fc80211_leave_mesh(void *priv)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	return fc80211_leave_mesh(drv->ifindex);
}
#endif /* CONFIG_MESH */


#ifdef CONFIG_MESH_PORTAL
int fc80211_chan_ch_mesh(void *priv , int new_channel)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;
	
	return fc80211_mesh_switch_channel(drv->ifindex , new_channel);
}
#endif

#ifdef SUPPORT_SELECT_NETWORK_FILTER
extern struct wpa_driver_scan_filter *
wpa_supplicant_build_filter_ssids(struct wpa_config *conf, size_t *num_ssid, int set);
 int wpa_driver_scan_select_ssid(struct wpa_supplicant *wpa_s, int set)
{
	struct i802_bss *bss = wpa_s->drv_priv;
	struct wpa_driver_fc80211_data *drv = bss->drv;

	RX_FUNC_START("");

	if (set == true) {
		drv->filter_ssids = wpa_supplicant_build_filter_ssids(
						wpa_s->conf, &drv->num_filter_ssids,set);
	
	} else {
		if (drv->filter_ssids != NULL)
			os_free(drv->filter_ssids);

		drv->filter_ssids = NULL;
		drv->num_filter_ssids = 0;
	}	
		

	RX_FUNC_END("");

	return 0;
}
#endif /* SUPPORT_SELECT_NETWORK_FILTER */


const struct wpa_driver_ops wpa_driver_fc80211_ops = {
	.name = "fc80211",
	.desc = "FCI fc80211",

	.get_bssid = wpa_driver_fc80211_get_bssid,
	.get_ssid = wpa_driver_fc80211_get_ssid,
	.set_key = driver_fc80211_set_key,
	.abort_scan = driver_fc80211_abort_scan,
	.scan2 = driver_fc80211_scan2,
	.get_scan_results2 = wpa_driver_fc80211_get_scan_results,
	.free_umac_scan_results = wpa_driver_fc80211_free_umac_scan_results,
	.deauthenticate = driver_fc80211_deauthenticate,
	.authenticate = driver_fc80211_authenticate,
	.associate = wpa_driver_fc80211_associate,
	.global_init = fc80211_global_init,
	.global_deinit = fc80211_global_deinit,
	.init2 = wpa_driver_fc80211_init,
	.deinit = driver_fc80211_deinit,
	.get_capa = wpa_driver_fc80211_get_capa,
	.set_supp_port = wpa_driver_fc80211_set_supp_port,
	.set_ap = wpa_driver_fc80211_set_ap,
	.set_acl = wpa_driver_fc80211_set_acl,
	.if_add = wpa_driver_fc80211_if_add,
	.if_remove = driver_fc80211_if_remove,
	.send_mlme = driver_fc80211_send_mlme,
	.get_hw_feature_data = wpa_driver_fc80211_get_hw_feature_data,
#ifdef  CONFIG_P2P_POWER_SAVE /* by Shingu 20170524 (P2P_PS) */
	.p2p_ps_from_sp = wpa_driver_fc80211_p2p_ps_from_ps,
#endif	/* CONFIG_P2P_POWER_SAVE */
	.sta_add = wpa_driver_fc80211_sta_add,
	.sta_remove = driver_fc80211_sta_remove,
#ifdef CONFIG_STA_POWER_SAVE
	.sta_power_save = fc80211_sta_pwrsave,
#endif /* CONFIG_STA_POWER_SAVE */
#ifdef	CONFIG_AP
	.hapd_send_eapol = wpa_driver_fc80211_hapd_send_eapol,
#endif	/* CONFIG_AP */
	.sta_set_flags = wpa_driver_fc80211_sta_set_flags,
#ifdef	CONFIG_AP
	.hapd_init = i802_init,
	.hapd_deinit = i802_deinit,
#endif	/* CONFIG_AP */
#ifdef	CONFIG_AP_WDS
	.set_wds_sta = i802_set_wds_sta,
#endif	/* CONFIG_AP_WDS */
#ifdef	CONFIG_AP
	.get_seqnum = i802_get_seqnum,
	.flush = i802_flush,
#ifdef CONFIG_AP_NONE_STA
	.none_station = driver_fc80211_ap_none_station,
#endif /* CONFIG_AP_NONE_STA */
#ifdef CONFIG_AP_POWER
	.set_ap_power = driver_fc80211_set_ap_power,
#endif /* CONFIG_AP_POWER */
#ifdef CONFIG_AP_ISOLATION
	.set_ap_isolate = wpa_driver_fc80211_set_ap_isolate,
#endif /* CONFIG_AP_ISOLATION */
	.ap_pwrsave = fc80211_ap_pwrsave,
	.addba_reject = fc80211_addba_reject,
#endif	/* CONFIG_AP */
	.sta_clear_stats = i802_sta_clear_stats,
	.set_rts = i802_set_rts,
#ifdef __FRAG_ENABLE__
	.set_frag = i802_set_frag,
#endif /* __FRAG_ENABLE__ */
	.set_retry = i802_set_retry,
	.get_retry = i802_get_retry,
	.get_rts = i802_get_rts,
	.set_tx_queue_params = i802_set_tx_queue_params,
#ifdef CONFIG_AP_VLAN
	.set_sta_vlan = driver_fc80211_set_sta_vlan,
#endif /* CONFIG_AP_VLAN */
	.sta_deauth = i802_sta_deauth,
	.sta_disassoc = i802_sta_disassoc,
#if 1	/* by Shingu 20160622 (P2P GO Inactivity) */
	.chk_deauth_send_done = i802_chk_deauth_send_done,
#endif	/* 1 */
#ifdef	CONFIG_AP
	.read_sta_data = driver_fc80211_read_sta_data,
#endif  /* CONFIG_AP */
	.set_freq = i802_set_freq,
	.send_action = driver_fc80211_send_action,
	.send_action_cancel_wait = wpa_driver_fc80211_send_action_cancel_wait,
	.remain_on_ch = wpa_driver_fc80211_remain_on_channel,
	.cancel_remain_on_ch = wpa_driver_fc80211_cancel_remain_on_channel,
	.probe_req_report = driver_fc80211_probe_req_report,
	.deinit_ap = wpa_driver_fc80211_deinit_ap,
#ifdef	CONFIG_P2P
	.deinit_p2p_cli = wpa_driver_fc80211_deinit_p2p_cli,
#endif	/* CONFIG_P2P */
	.resume = wpa_driver_fc80211_resume,
#ifdef CONFIG_IEEE80211R
	.send_ft_action = fc80211_send_ft_action,
#endif /* CONFIG_IEEE80211R */
	.send_frame = fc80211_send_frame,
	.shared_freq = wpa_driver_fc80211_shared_freq,
	.set_param = fc80211_set_param,
	.get_radio_name = fc80211_get_radio_name,
#ifdef	CONFIG_P2P_POWER_SAVE
	.set_p2p_powersave = fc80211_set_p2p_powersave,
#endif	/* CONFIG_P2P_POWER_SAVE */
	.get_mac_addr = wpa_driver_fc80211_get_macaddr,
#ifdef CONFIG_AP_CHANNEL_SWITCH
	.switch_channel = fc80211_switch_channel,
#endif
#ifdef CONFIG_WNM_SLEEP_MODE
	.wnm_oper = fc80211_wnm_operation,
#endif /* CONFIG_WNM_SLEEP_MODE */
#ifdef	CONFIG_ACS
	.get_survey = fc80211_get_dump_survey,
#endif	/* CONFIG_ACS */
#ifdef  CONFIG_AP /* by Shingu 20161010 (Keep-alive) */
        .chk_keep_alive = i802_chk_keep_alive,
#endif  /* CONFIG_AP */

#ifdef CONFIG_MESH
	.init_mesh = wpa_driver_fc80211_init_mesh,
	.join_mesh = wpa_driver_fc80211_join_mesh,
	.leave_mesh = wpa_driver_fc80211_leave_mesh,
#endif

#ifdef CONFIG_MESH_PORTAL
	.chan_ch_mesh = fc80211_chan_ch_mesh,
#endif

	.get_rtctime_gap = driver_fc80211_get_rtctimegap,
};

struct wpa_driver_ops *wpa_drivers =
			(struct wpa_driver_ops *)&wpa_driver_fc80211_ops;

/* EOF */
