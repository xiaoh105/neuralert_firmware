/**
 ****************************************************************************************
 *
 * @file mesh_mpm.c
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

#include "includes.h"

#ifdef CONFIG_MESH

#include "supp_common.h"
#include "supp_eloop.h"
#include "common/ieee802_11_defs.h"
#include "common/hw_features_common.h"
#include "ap/hostapd.h"
#include "ap/sta_info.h"
#include "ap/ieee802_11.h"
#include "ap/wpa_auth.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "mesh_mpm.h"
#include "mesh_rsn.h"
//#include "notify.h"

struct mesh_peer_mgmt_ie {
	const u8 *proto_id; /* Mesh Peering Protocol Identifier (2 octets) */
	const u8 *llid; /* Local Link ID (2 octets) */
	const u8 *plid; /* Peer Link ID (conditional, 2 octets) */
	const u8 *reason; /* Reason Code (conditional, 2 octets) */
	const u8 *chosen_pmk; /* Chosen PMK (optional, 16 octets) */
};

static void plink_timer(void *eloop_ctx, void *user_data);


enum plink_event {
	PLINK_UNDEFINED,
	OPN_ACPT,
	OPN_RJCT,
	CNF_ACPT,
	CNF_RJCT,
	CLS_ACPT,
	REQ_RJCT
};

static const char * const mplstate[] = {
	[0] = "UNINITIALIZED",
	[PLINK_IDLE] = "IDLE",
	[PLINK_OPN_SNT] = "OPN_SNT",
	[PLINK_OPN_RCVD] = "OPN_RCVD",
	[PLINK_CNF_RCVD] = "CNF_RCVD",
	[PLINK_ESTAB] = "ESTAB",
	[PLINK_HOLDING] = "HOLDING",
	[PLINK_BLOCKED] = "BLOCKED"
};

static const char * const mplevent[] = {
	[PLINK_UNDEFINED] = "UNDEFINED",
	[OPN_ACPT] = "OPN_ACPT",
	[OPN_RJCT] = "OPN_RJCT",
	[CNF_ACPT] = "CNF_ACPT",
	[CNF_RJCT] = "CNF_RJCT",
	[CLS_ACPT] = "CLS_ACPT",
	[REQ_RJCT] = "REQ_RJCT",
};


static int mesh_mpm_parse_peer_mgmt(struct wpa_supplicant *wpa_s,
				    u8 action_field,
				    const u8 *ie, size_t len,
				    struct mesh_peer_mgmt_ie *mpm_ie)
{
	os_memset(mpm_ie, 0, sizeof(*mpm_ie));

	/* Remove optional Chosen PMK field at end */
	if (len >= SAE_PMKID_LEN) {
		mpm_ie->chosen_pmk = ie + len - SAE_PMKID_LEN;
		len -= SAE_PMKID_LEN;
	}

	if ((action_field == PLINK_OPEN && len != 4) ||
	    (action_field == PLINK_CONFIRM && len != 6) ||
	    (action_field == PLINK_CLOSE && len != 6 && len != 8)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "MPM: Invalid peer mgmt ie");
		return -1;
	}

	/* required fields */
	if (len < 4)
		return -1;
	mpm_ie->proto_id = ie;
	mpm_ie->llid = ie + 2;
	ie += 4;
	len -= 4;

	/* close reason is always present at end for close */
	if (action_field == PLINK_CLOSE) {
		if (len < 2)
			return -1;
		mpm_ie->reason = ie + len - 2;
		len -= 2;
	}

	/* Peer Link ID, present for confirm, and possibly close */
	if (len >= 2)
		mpm_ie->plid = ie;

	return 0;
}


static int plink_free_count(struct hostapd_data *hapd)
{
	if (hapd->max_plinks > hapd->num_plinks)
		return hapd->max_plinks - hapd->num_plinks;
	return 0;
}


static u16 copy_supp_rates(struct wpa_supplicant *wpa_s,
			   struct sta_info *sta,
			   struct ieee802_11_elems *elems)
{
	if (!elems->supp_rates) {
		wpa_msg(wpa_s, MSG_ERROR, "no supported rates from " MACSTR,
			MAC2STR(sta->addr));
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (elems->supp_rates_len + elems->ext_supp_rates_len >
	    sizeof(sta->supported_rates)) {
		wpa_msg(wpa_s, MSG_ERROR,
			"Invalid supported rates element length " MACSTR
			" %d+%d", MAC2STR(sta->addr), elems->supp_rates_len,
			elems->ext_supp_rates_len);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	sta->supported_rates_len = merge_byte_arrays(
		sta->supported_rates, sizeof(sta->supported_rates),
		elems->supp_rates, elems->supp_rates_len,
		elems->ext_supp_rates, elems->ext_supp_rates_len);

	return WLAN_STATUS_SUCCESS;
}


/* return true if elems from a neighbor match this MBSS */
static Boolean matches_local(struct wpa_supplicant *wpa_s,
			     struct ieee802_11_elems *elems)
{
	struct mesh_conf *mconf = wpa_s->ifmsh->mconf;

	if (elems->mesh_config_len < 5)
		return FALSE;

	return (mconf->meshid_len == elems->mesh_id_len &&
		os_memcmp(mconf->meshid, elems->mesh_id,
			  elems->mesh_id_len) == 0 &&
		mconf->mesh_pp_id == elems->mesh_config[0] &&
		mconf->mesh_pm_id == elems->mesh_config[1] &&
		mconf->mesh_cc_id == elems->mesh_config[2] &&
		mconf->mesh_sp_id == elems->mesh_config[3] &&
		mconf->mesh_auth_id == elems->mesh_config[4]);
}


/* check if local link id is already used with another peer */
static Boolean llid_in_use(struct wpa_supplicant *wpa_s, u16 llid)
{
	struct sta_info *sta;
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];

	for (sta = hapd->sta_list; sta; sta = sta->next) {
		if (sta->my_lid == llid)
			return TRUE;
	}

	return FALSE;
}


/* generate an llid for a link and set to initial state */
static void mesh_mpm_init_link(struct wpa_supplicant *wpa_s,
			       struct sta_info *sta)
{
	u16 llid;

	do {
		if (os_get_random((u8 *) &llid, sizeof(llid)) < 0)
			continue;
	} while (!llid || llid_in_use(wpa_s, llid));

	sta->my_lid = llid;
	sta->peer_lid = 0;
	sta->peer_aid = 0;

	/*
	 * We do not use wpa_mesh_set_plink_state() here because there is no
	 * entry in kernel yet.
	 */
	sta->plink_state = PLINK_IDLE;
	sta->plink_intial_time = xTaskGetTickCount()/*tx_time_get()*/;
}


static void mesh_mpm_send_plink_action(struct wpa_supplicant *wpa_s,
				       struct sta_info *sta,
				       enum plink_action_field type,
				       u16 close_reason)
{
	struct wpabuf *buf;
	struct hostapd_iface *ifmsh = wpa_s->ifmsh;
	struct hostapd_data *bss = ifmsh->bss[0];
	struct mesh_conf *conf = ifmsh->mconf;
	u8 supp_rates[2 + 2 + 32];
	u8 *pos, *cat;
	u8 ie_len, add_plid = 0;
	int ret;
	int ampe = conf->security & MESH_CONF_SEC_AMPE;
	size_t buf_len;

	if (!sta)
		return;

	buf_len = 2 +      /* capability info */
		  2 +      /* AID */
		  2 + 8 +  /* supported rates */
		  2 + (32 - 8) +
		  2 + 32 + /* mesh ID */
		  2 + 7 +  /* mesh config */
		  2 + 23 + /* peering management */
		  2 + 96 + /* AMPE */
		  2 + 16;  /* MIC */
#ifdef CONFIG_IEEE80211N
	if (type != PLINK_CLOSE && wpa_s->mesh_ht_enabled) {
		buf_len += 2 + 26 + /* HT capabilities */
			   2 + 22;  /* HT operation */
	}
#endif /* CONFIG_IEEE80211N */
#ifdef CONFIG_IEEE80211AC
	if (type != PLINK_CLOSE && wpa_s->mesh_vht_enabled) {
		buf_len += 2 + 12 + /* VHT Capabilities */
			   2 + 5;  /* VHT Operation */
	}
#endif /* CONFIG_IEEE80211AC */
	if (type != PLINK_CLOSE)
		buf_len += conf->rsn_ie_len; /* RSN IE */

	buf = wpabuf_alloc(buf_len);
	if (!buf)
		return;

	cat = wpabuf_mhead_u8(buf);
	wpabuf_put_u8(buf, WLAN_ACTION_SELF_PROTECTED);
	wpabuf_put_u8(buf, type);

	if (type != PLINK_CLOSE) {
		u8 info;

		/* capability info */
		wpabuf_put_le16(buf, ampe ? IEEE80211_CAP_PRIVACY : 0);

		/* aid */
		if (type == PLINK_CONFIRM)
			wpabuf_put_le16(buf, sta->aid);

		/* IE: supp + ext. supp rates */
		pos = hostapd_eid_supp_rates(bss, supp_rates);
		pos = hostapd_eid_ext_supp_rates(bss, pos);
		wpabuf_put_data(buf, supp_rates, pos - supp_rates);

		/* IE: RSN IE */
		wpabuf_put_data(buf, conf->rsn_ie, conf->rsn_ie_len);

		/* IE: Mesh ID */
		wpabuf_put_u8(buf, WLAN_EID_MESH_ID);
		wpabuf_put_u8(buf, conf->meshid_len);
		wpabuf_put_data(buf, conf->meshid, conf->meshid_len);

		/* IE: mesh conf */
		wpabuf_put_u8(buf, WLAN_EID_MESH_CONFIG);
		wpabuf_put_u8(buf, 7);
		wpabuf_put_u8(buf, conf->mesh_pp_id);
		wpabuf_put_u8(buf, conf->mesh_pm_id);
		wpabuf_put_u8(buf, conf->mesh_cc_id);
		wpabuf_put_u8(buf, conf->mesh_sp_id);
		wpabuf_put_u8(buf, conf->mesh_auth_id);
#if 1 /* Changed by Dialog */
		info = (bss->num_plinks >= bss->max_plinks ? bss->max_plinks - 1 : bss->num_plinks) << 1;
#else
		info = (bss->num_plinks > 63 ? 63 : bss->num_plinks) << 1;
#endif /* Changed by Dialog */
		/* TODO: Add Connected to Mesh Gate/AS subfields */
		wpabuf_put_u8(buf, info);
		/* always forwarding & accepting plinks for now */
		wpabuf_put_u8(buf, MESH_CAP_ACCEPT_ADDITIONAL_PEER |
			      MESH_CAP_FORWARDING);
	} else {	/* Peer closing frame */
		/* IE: Mesh ID */
		wpabuf_put_u8(buf, WLAN_EID_MESH_ID);
		wpabuf_put_u8(buf, conf->meshid_len);
		wpabuf_put_data(buf, conf->meshid, conf->meshid_len);
	}

	/* IE: Mesh Peering Management element */
	ie_len = 4;
	if (ampe)
		ie_len += PMKID_LEN;
	switch (type) {
	case PLINK_OPEN:
		break;
	case PLINK_CONFIRM:
		ie_len += 2;
		add_plid = 1;
		break;
	case PLINK_CLOSE:
		ie_len += 2;
		add_plid = 1;
		ie_len += 2; /* reason code */
		break;
	}

	wpabuf_put_u8(buf, WLAN_EID_PEER_MGMT);
	wpabuf_put_u8(buf, ie_len);
	/* peering protocol */
	if (ampe)
		wpabuf_put_le16(buf, 1);
	else
		wpabuf_put_le16(buf, 0);
	wpabuf_put_le16(buf, sta->my_lid);
	if (add_plid)
		wpabuf_put_le16(buf, sta->peer_lid);
	if (type == PLINK_CLOSE)
		wpabuf_put_le16(buf, close_reason);
	if (ampe) {
		if (sta->sae == NULL) {
			wpa_msg(wpa_s, MSG_INFO, "Mesh MPM: no SAE session");
			goto fail;
		}
		mesh_rsn_get_pmkid(wpa_s->mesh_rsn, sta,
				   wpabuf_put(buf, PMKID_LEN));
	}

#ifdef CONFIG_IEEE80211N
	if (type != PLINK_CLOSE && wpa_s->mesh_ht_enabled) {
		u8 ht_capa_oper[2 + 26 + 2 + 22];

		pos = hostapd_eid_ht_capabilities(bss, ht_capa_oper);
		pos = hostapd_eid_ht_operation(bss, pos);
		wpabuf_put_data(buf, ht_capa_oper, pos - ht_capa_oper);
	}
#endif /* CONFIG_IEEE80211N */
#ifdef CONFIG_IEEE80211AC
	if (type != PLINK_CLOSE && wpa_s->mesh_vht_enabled) {
		u8 vht_capa_oper[2 + 12 + 2 + 5];

		pos = hostapd_eid_vht_capabilities(bss, vht_capa_oper, 0);
		pos = hostapd_eid_vht_operation(bss, pos);
		wpabuf_put_data(buf, vht_capa_oper, pos - vht_capa_oper);
	}
#endif /* CONFIG_IEEE80211AC */

	if (ampe && mesh_rsn_protect_frame(wpa_s->mesh_rsn, sta, cat, buf)) {
		wpa_msg(wpa_s, MSG_INFO,
			"Mesh MPM: failed to add AMPE and MIC IE");
		goto fail;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Mesh MPM: Sending peering frame type %d to "
		MACSTR " (my_lid=0x%x peer_lid=0x%x)",
		type, MAC2STR(sta->addr), sta->my_lid, sta->peer_lid);
	ret = wpa_drv_send_action(wpa_s, wpa_s->assoc_freq, 0,
				  sta->addr, wpa_s->own_addr, wpa_s->own_addr,
				  wpabuf_head(buf), wpabuf_len(buf), 0);
	if (ret < 0)
		wpa_msg(wpa_s, MSG_INFO,
			"Mesh MPM: failed to send peering frame");

fail:
	wpabuf_free(buf);
}


/* configure peering state in ours and driver's station entry */
void wpa_mesh_set_plink_state(struct wpa_supplicant *wpa_s,
			      struct sta_info *sta,
			      enum mesh_plink_state state)
{
	struct hostapd_sta_add_params params;
	int ret;

	wpa_dbg(wpa_s, MSG_DEBUG, "MPM set " MACSTR " from %s into %s",
		MAC2STR(sta->addr), mplstate[sta->plink_state],
		mplstate[state]);
	sta->plink_state = state;

	os_memset(&params, 0, sizeof(params));
	params.addr = sta->addr;
	params.plink_state = state;
	params.peer_aid = sta->peer_aid;
	params.set = 1;

	ret = wpa_drv_sta_add(wpa_s, &params);
	if (ret) {
		wpa_msg(wpa_s, MSG_ERROR, "Driver failed to set " MACSTR
			": %d", MAC2STR(sta->addr), ret);
	}
}


static void mesh_mpm_fsm_restart(struct wpa_supplicant *wpa_s,
				 struct sta_info *sta)
{
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];

#if 1	/* by Bright : Merge 2.7 */
	da16x_eloop_cancel_timeout(plink_timer, wpa_s, sta);
#else
	da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */

	ap_free_sta(hapd, sta);
}


static void plink_timer(void *eloop_ctx, void *user_data)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct sta_info *sta = user_data;
	u16 reason = 0;
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];

	switch (sta->plink_state) {
	case PLINK_OPN_RCVD:
	case PLINK_OPN_SNT:
		/* retry timer */
		if (sta->mpm_retries < conf->dot11MeshMaxRetries) {
#if 1	/* by Bright : Merge 2.7 */
			da16x_eloop_register_timeout(
				conf->dot11MeshRetryTimeout / 1000,
				(conf->dot11MeshRetryTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
#else
			da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */
			mesh_mpm_send_plink_action(wpa_s, sta, PLINK_OPEN, 0);
			sta->mpm_retries++;
			break;
		}
		reason = WLAN_REASON_MESH_MAX_RETRIES;
		/* fall through */

	case PLINK_CNF_RCVD:
		/* confirm timer */
		if (!reason)
			reason = WLAN_REASON_MESH_CONFIRM_TIMEOUT;
		wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
		da16x_eloop_register_timeout(conf->dot11MeshHoldingTimeout / 1000,
			(conf->dot11MeshHoldingTimeout % 1000) * 1000,
			plink_timer, wpa_s, sta);
		mesh_mpm_send_plink_action(wpa_s, sta, PLINK_CLOSE, reason);
		break;
	case PLINK_HOLDING:
		/* holding timer */

		if (sta->mesh_sae_pmksa_caching) {
			wpa_printf_dbg(MSG_DEBUG, "MPM: Peer " MACSTR
				   " looks like it does not support mesh SAE PMKSA caching, so remove the cached entry for it",
				   MAC2STR(sta->addr));
			wpa_auth_pmksa_remove(hapd->wpa_auth, sta->addr);
		}
		mesh_mpm_fsm_restart(wpa_s, sta);
		break;
	default:
		break;
	}
}


/* initiate peering with station */
static void
mesh_mpm_plink_open(struct wpa_supplicant *wpa_s, struct sta_info *sta,
		    enum mesh_plink_state next_state)
{
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;

#ifdef CONFIG_MESH /* by Bright : Merge 2.7 */
	da16x_eloop_register_timeout(conf->dot11MeshRetryTimeout / 1000,
			       (conf->dot11MeshRetryTimeout % 1000) * 1000,
			       plink_timer, wpa_s, sta);
#else
	da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */
	mesh_mpm_send_plink_action(wpa_s, sta, PLINK_OPEN, 0);
	wpa_mesh_set_plink_state(wpa_s, sta, next_state);
}


static int mesh_mpm_plink_close(struct hostapd_data *hapd, struct sta_info *sta,
				void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	int reason = WLAN_REASON_MESH_PEERING_CANCELLED;

	if (sta) {
		wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
		mesh_mpm_send_plink_action(wpa_s, sta, PLINK_CLOSE, reason);
		wpa_printf_dbg(MSG_DEBUG, "MPM closing plink sta=" MACSTR,
			   MAC2STR(sta->addr));
#ifdef CONFIG_MESH	/* by Bright : Merge 2.7 */
		da16x_eloop_cancel_timeout(plink_timer, wpa_s, sta);
#else
		da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */
		return 0;
	}

	return 1;
}


int mesh_mpm_close_peer(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	struct hostapd_data *hapd;
	struct sta_info *sta;

	if (!wpa_s->ifmsh) {
		wpa_msg(wpa_s, MSG_INFO, "Mesh is not prepared yet");
		return -1;
	}

	hapd = wpa_s->ifmsh->bss[0];
	sta = ap_get_sta(hapd, addr);
	if (!sta) {
		wpa_msg(wpa_s, MSG_INFO, "No such mesh peer");
		return -1;
	}

	return mesh_mpm_plink_close(hapd, sta, wpa_s) == 0 ? 0 : -1;
}


static void peer_add_timer(void *eloop_ctx, void *user_data)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];

	os_memset(hapd->mesh_required_peer, 0, ETH_ALEN);
}


int mesh_mpm_connect_peer(struct wpa_supplicant *wpa_s, const u8 *addr,
			  int duration)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	struct hostapd_data *hapd;
	struct sta_info *sta;
	struct mesh_conf *conf;

	if (!wpa_s->ifmsh) {
		wpa_msg(wpa_s, MSG_INFO, "Mesh is not prepared yet");
		return -1;
	}

	if (!ssid || !ssid->no_auto_peer) {
		wpa_msg(wpa_s, MSG_INFO,
			"This command is available only with no_auto_peer mesh network");
		return -1;
	}

	hapd = wpa_s->ifmsh->bss[0];
	conf = wpa_s->ifmsh->mconf;

	sta = ap_get_sta(hapd, addr);
	if (!sta) {
		wpa_msg(wpa_s, MSG_INFO, "No such mesh peer");
		return -1;
	}

	if ((PLINK_OPN_SNT <= sta->plink_state &&
	    sta->plink_state <= PLINK_ESTAB) ||
	    (sta->sae && sta->sae->state > SAE_NOTHING)) {
		wpa_msg(wpa_s, MSG_INFO,
			"Specified peer is connecting/connected");
		return -1;
	}

	if (conf->security == MESH_CONF_SEC_NONE) {
		mesh_mpm_plink_open(wpa_s, sta, PLINK_OPN_SNT);
	} else {
		mesh_rsn_auth_sae_sta(wpa_s, sta);
		os_memcpy(hapd->mesh_required_peer, addr, ETH_ALEN);
		da16x_eloop_register_timeout(duration == -1 ? 10 : duration, 0,
				       peer_add_timer, wpa_s, NULL);
	}

	return 0;
}


void mesh_mpm_deinit(struct wpa_supplicant *wpa_s, struct hostapd_iface *ifmsh)
{
	struct hostapd_data *hapd = ifmsh->bss[0];

	/* notify peers we're leaving */
	ap_for_each_sta(hapd, mesh_mpm_plink_close, wpa_s);

	hapd->num_plinks = 0;
	hostapd_free_stas(hapd);
#ifdef CONFIG_MESH	/* by Bright : Merge 2.7 */
	da16x_eloop_cancel_timeout(peer_add_timer, wpa_s, NULL);
#else
	da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */
}

void mesh_mpm_sta_remove(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];
	struct sta_info *sta;

	sta = ap_get_sta(hapd, addr);
	if (!sta) {
		wpa_dbg(wpa_s, MSG_DEBUG, "no such mesh peer");
		return;
	}
	
	mesh_mpm_plink_close(hapd, sta, wpa_s);


	if (sta->flags & WLAN_STA_AUTH) {
		mlme_deauthenticate_indication(
			hapd, sta, WLAN_REASON_UNSPECIFIED);
#if 0
		if (hapd->num_plinks > 0) { 
			hapd->num_plinks--;
		}
#endif /* 0 */
	}

	da16x_notice_prt("[%s] Removing station " MACSTR "\n",
		   __func__, MAC2STR(addr));

	ap_free_sta(hapd, sta);
	da16x_eloop_cancel_timeout(peer_add_timer, wpa_s, NULL);
}


/* for mesh_rsn to indicate this peer has completed authentication, and we're
 * ready to start AMPE */
void mesh_mpm_auth_peer(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	struct hostapd_data *data = wpa_s->ifmsh->bss[0];
	struct hostapd_sta_add_params params;
	struct sta_info *sta;
	int ret;

	sta = ap_get_sta(data, addr);
	if (!sta) {
		wpa_dbg(wpa_s, MSG_DEBUG, "no such mesh peer");
		return;
	}

	/* TODO: Should do nothing if this STA is already authenticated, but
	 * the AP code already sets this flag. */
	sta->flags |= WLAN_STA_AUTH;

	mesh_rsn_init_ampe_sta(wpa_s, sta);

	os_memset(&params, 0, sizeof(params));
	params.addr = sta->addr;
	params.flags = WPA_STA_AUTHENTICATED | WPA_STA_AUTHORIZED;
	params.set = 1;

	wpa_dbg(wpa_s, MSG_DEBUG, "MPM authenticating " MACSTR,
		MAC2STR(sta->addr));
	ret = wpa_drv_sta_add(wpa_s, &params);
	if (ret) {
		wpa_msg(wpa_s, MSG_ERROR,
			"Driver failed to set " MACSTR ": %d",
			MAC2STR(sta->addr), ret);
	}

	if (!sta->my_lid)
		mesh_mpm_init_link(wpa_s, sta);

	mesh_mpm_plink_open(wpa_s, sta, PLINK_OPN_SNT);
}

/*
 * Initialize a sta_info structure for a peer and upload it into the driver
 * in preparation for beginning authentication or peering. This is done when a
 * Beacon (secure or open mesh) or a peering open frame (for open mesh) is
 * received from the peer for the first time.
 */
 
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
extern int mesh_linked_ie_addr_find(u8 *addr, const u8 *ie);
#endif

static struct sta_info * mesh_mpm_add_peer(struct wpa_supplicant *wpa_s,
					   const u8 *addr,
					   struct ieee802_11_elems *elems)
{
	struct hostapd_sta_add_params params;
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	struct hostapd_data *data = wpa_s->ifmsh->bss[0];
	struct sta_info *sta;
#ifdef CONFIG_IEEE80211N
	struct _ieee80211_ht_operation *oper;
#endif /* CONFIG_IEEE80211N */
	int ret;

	if (elems->mesh_config_len >= 7 &&
	    !(elems->mesh_config[6] & MESH_CAP_ACCEPT_ADDITIONAL_PEER)) {
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
#if 0
		PRINTF(" [%s](%d) ",__func__ ,	elems->mesh_star_link_len);
		for(int j =0; j< elems->mesh_star_link_len; j++)
		PRINTF("0x%x ",elems->mesh_star_link[j]);
		PRINTF("\n");	
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED
		if(((elems->mesh_star_link_len >0) &&
			(mesh_linked_ie_addr_find( &wpa_s->own_addr[0], elems->mesh_star_link) == TRUE)) ||
			(elems->mesh_star_link_len ==0))
		{
			//PRINTF("[%s]mesh_star_link_len(%d)\n",__func__, elems->mesh_star_link_len);
		} else {
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED

		wpa_dbg(wpa_s, MSG_DEBUG,
			"mesh: Ignore a crowded peer " MACSTR,
			MAC2STR(addr));
		return NULL;
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
		}
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED
	}

	sta = ap_get_sta(data, addr);
	if (!sta) {
		sta = ap_sta_add(data, addr);
		if (!sta)
			return NULL;
	}

	/* Set WMM by default since Mesh STAs are QoS STAs */
	sta->flags |= WLAN_STA_WMM;

	/* initialize sta */
	if (copy_supp_rates(wpa_s, sta, elems)) {
		ap_free_sta(data, sta);
		return NULL;
	}

	if (!sta->my_lid)
		mesh_mpm_init_link(wpa_s, sta);

#ifdef CONFIG_IEEE80211N
	copy_sta_ht_capab(data, sta, elems->ht_capabilities);

	oper = (struct _ieee80211_ht_operation *) elems->ht_operation;
	if (oper &&
	    !(oper->ht_param & HT_INFO_HT_PARAM_STA_CHNL_WIDTH) &&
	    sta->ht_capabilities) {
		wpa_dbg(wpa_s, MSG_DEBUG, MACSTR
			" does not support 40 MHz bandwidth",
			MAC2STR(sta->addr));
		set_disable_ht40(sta->ht_capabilities, 1);
	}

	update_ht_state(data, sta);
#endif /* CONFIG_IEEE80211N */

#ifdef CONFIG_IEEE80211AC
	copy_sta_vht_capab(data, sta, elems->vht_capabilities);
	set_sta_vht_opmode(data, sta, elems->vht_opmode_notif);
#endif /* CONFIG_IEEE80211AC */

	if (hostapd_get_aid(data, sta) < 0) {
		wpa_msg(wpa_s, MSG_ERROR, "No AIDs available");
		ap_free_sta(data, sta);
		return NULL;
	}

	/* insert into driver */
	os_memset(&params, 0, sizeof(params));
	params.supp_rates = sta->supported_rates;
	params.supp_rates_len = sta->supported_rates_len;
	params.addr = addr;
	params.plink_state = sta->plink_state;
	params.aid = sta->aid;
	params.peer_aid = sta->peer_aid;
	params.listen_interval = 100;
	params.ht_capabilities = sta->ht_capabilities;
#ifdef CONFIG_AP_VHT
	params.vht_capabilities = sta->vht_capabilities;
#endif /* CONFIG_AP_VHT */
	params.flags |= WPA_STA_WMM;
	params.flags_mask |= WPA_STA_AUTHENTICATED;
	if (conf->security == MESH_CONF_SEC_NONE) {
		params.flags |= WPA_STA_AUTHORIZED;
		params.flags |= WPA_STA_AUTHENTICATED;
	} else {
		sta->flags |= WLAN_STA_MFP;
		params.flags |= WPA_STA_MFP;
	}

	ret = wpa_drv_sta_add(wpa_s, &params);
	if (ret) {
		wpa_msg(wpa_s, MSG_ERROR,
			"Driver failed to insert " MACSTR ": %d",
			MAC2STR(addr), ret);
		ap_free_sta(data, sta);
		return NULL;
	}

	return sta;
}


void wpa_mesh_new_mesh_peer(struct wpa_supplicant *wpa_s, const u8 *addr,
			    struct ieee802_11_elems *elems)
{
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	struct hostapd_data *data = wpa_s->ifmsh->bss[0];
	struct sta_info *sta;
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	sta = mesh_mpm_add_peer(wpa_s, addr, elems);
	if (!sta)
		return;

	if (ssid && ssid->no_auto_peer &&
	    (is_zero_ether_addr(data->mesh_required_peer) ||
	     os_memcmp(data->mesh_required_peer, addr, ETH_ALEN) != 0)) {
		wpa_msg(wpa_s, MSG_INFO, "will not initiate new peer link with "
			MACSTR " because of no_auto_peer", MAC2STR(addr));
		if (data->mesh_pending_auth) {
			struct os_reltime age;
			const struct _ieee80211_mgmt *mgmt;
			struct hostapd_frame_info fi;

			mgmt = wpabuf_head(data->mesh_pending_auth);
			os_reltime_age(&data->mesh_pending_auth_time, &age);
			if (age.sec < 2 &&
			    os_memcmp(mgmt->sa, addr, ETH_ALEN) == 0) {
				wpa_printf_dbg(MSG_DEBUG,
					   "mesh: Process pending Authentication frame from %u.%06u seconds ago",
					   (unsigned int) age.sec,
					   (unsigned int) age.usec);
				os_memset(&fi, 0, sizeof(fi));
				ieee802_11_mgmt(
					data,
					wpabuf_head(data->mesh_pending_auth),
					wpabuf_len(data->mesh_pending_auth),
					&fi);
			}
			wpabuf_free(data->mesh_pending_auth);
			data->mesh_pending_auth = NULL;
		}
		return;
	}

	if (conf->security == MESH_CONF_SEC_NONE) {
		if (sta->plink_state < PLINK_OPN_SNT ||
		    sta->plink_state > PLINK_ESTAB)
			mesh_mpm_plink_open(wpa_s, sta, PLINK_OPN_SNT);
	} else {
		mesh_rsn_auth_sae_sta(wpa_s, sta);
	}
}


void mesh_mpm_mgmt_rx(struct wpa_supplicant *wpa_s, struct rx_mgmt *rx_mgmt)
{
	struct hostapd_frame_info fi;

	os_memset(&fi, 0, sizeof(fi));
	fi.datarate = rx_mgmt->datarate;
	fi.ssi_signal = rx_mgmt->ssi_signal;
	ieee802_11_mgmt(wpa_s->ifmsh->bss[0], rx_mgmt->frame,
			rx_mgmt->frame_len, &fi);
}


static int/*void*/ mesh_mpm_plink_estab(struct wpa_supplicant *wpa_s,
				 struct sta_info *sta)
{
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	u8 seq[6] = {0,};
        int  ret=0;

	wpa_msg(wpa_s, MSG_INFO, "mesh plink with " MACSTR " established",
		MAC2STR(sta->addr));

	if (conf->security & MESH_CONF_SEC_AMPE) {
		wpa_hexdump_key(MSG_DEBUG, "mesh: MTK", sta->mtk, sta->mtk_len);
		ret = wpa_drv_set_key(wpa_s, wpa_cipher_to_alg(conf->pairwise_cipher),
				sta->addr, 0, 0, seq, sizeof(seq),
				sta->mtk, sta->mtk_len);
		if(ret)
			return ret;

		wpa_hexdump_key(MSG_DEBUG, "mesh: RX MGTK Key RSC",
				sta->mgtk_rsc, sizeof(sta->mgtk_rsc));
		wpa_hexdump_key(MSG_DEBUG, "mesh: RX MGTK",
				sta->mgtk, sta->mgtk_len);
		ret = wpa_drv_set_key(wpa_s, wpa_cipher_to_alg(conf->group_cipher),
				sta->addr, sta->mgtk_key_id, 0,
				sta->mgtk_rsc, sizeof(sta->mgtk_rsc),
				sta->mgtk, sta->mgtk_len);
		if(ret)
			return ret;

		if (sta->igtk_len) {
			wpa_hexdump_key(MSG_DEBUG, "mesh: RX IGTK Key RSC",
					sta->igtk_rsc, sizeof(sta->igtk_rsc));
			wpa_hexdump_key(MSG_DEBUG, "mesh: RX IGTK",
					sta->igtk, sta->igtk_len);
			ret = wpa_drv_set_key(
				wpa_s,
				wpa_cipher_to_alg(conf->mgmt_group_cipher),
				sta->addr, sta->igtk_key_id, 0,
				sta->igtk_rsc, sizeof(sta->igtk_rsc),
				sta->igtk, sta->igtk_len);
			if(ret)
				return ret;
		}
	}

	wpa_mesh_set_plink_state(wpa_s, sta, PLINK_ESTAB);
	wpa_msg(wpa_s, MSG_USER_DEBUG, "[%s] num_plinks=%d : num_plinks++\n", __func__, hapd->num_plinks);
	hapd->num_plinks++;

	sta->flags |= WLAN_STA_ASSOC;
	sta->mesh_sae_pmksa_caching = 0;

#ifdef CONFIG_MESH	/* by Bright : Merge 2.7 */
	da16x_eloop_cancel_timeout(peer_add_timer, wpa_s, NULL);
	peer_add_timer(wpa_s, NULL);
	da16x_eloop_cancel_timeout(plink_timer, wpa_s, sta);

	if (os_strcmp(wpa_s->ifname, MESH_POINT_DEVICE_NAME) != 0
		&& get_run_mode() == STA_MESH_PORTAL_MODE)
	{
		da16x_eloop_register_timeout(0, 0, wpa_supplicant_reconfig_net, wpa_s, NULL); /* 0 ms */
	}
#else
	peer_add_timer(wpa_s, NULL);
	da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */

	/* Send ctrl event */
	wpa_msg(wpa_s, MSG_INFO, MESH_PEER_CONNECTED MACSTR,
		MAC2STR(sta->addr));
#if 0 //courmagu
	/* Send D-Bus event */
	wpas_notify_mesh_peer_connected(wpa_s, sta->addr);
#endif
}


static int/*void*/ mesh_mpm_fsm(struct wpa_supplicant *wpa_s, struct sta_info *sta,
			 enum plink_event event, u16 reason)
{
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];
	struct mesh_conf *conf = wpa_s->ifmsh->mconf;
	int ret=0;

	wpa_msg(wpa_s, MSG_DEBUG, "MPM " MACSTR " state %s event %s",
		MAC2STR(sta->addr), mplstate[sta->plink_state],
		mplevent[event]);

	switch (sta->plink_state) {
	case PLINK_IDLE:
		switch (event) {
		case CLS_ACPT:
			mesh_mpm_fsm_restart(wpa_s, sta);
			break;
		case OPN_ACPT:
			mesh_mpm_plink_open(wpa_s, sta, PLINK_OPN_RCVD);
			mesh_mpm_send_plink_action(wpa_s, sta, PLINK_CONFIRM,
						   0);
			break;
		case REQ_RJCT:
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		default:
			break;
		}
		break;
	case PLINK_OPN_SNT:
		switch (event) {
		case OPN_RJCT:
		case CNF_RJCT:
			if (!reason)
				reason = WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION;
			/* fall-through */
		case CLS_ACPT:
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
			if (!reason)
				reason = WLAN_REASON_MESH_CLOSE_RCVD;
			da16x_eloop_register_timeout(
				conf->dot11MeshHoldingTimeout / 1000,
				(conf->dot11MeshHoldingTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		case OPN_ACPT:
			/* retry timer is left untouched */
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_OPN_RCVD);
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CONFIRM, 0);
			break;
		case CNF_ACPT:
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_CNF_RCVD);
#ifdef CONFIG_MESH	/* by Bright : Merge 2.7 */
			da16x_eloop_cancel_timeout(plink_timer, wpa_s, sta);
#else
			da16x_iface_prt("[%s] Have to cancel timer function ...\n", __func__);
#endif	/* 0 */
			da16x_eloop_register_timeout(
				conf->dot11MeshConfirmTimeout / 1000,
				(conf->dot11MeshConfirmTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
			break;
		default:
			break;
		}
		break;
	case PLINK_OPN_RCVD:
		switch (event) {
		case OPN_RJCT:
		case CNF_RJCT:
			if (!reason)
				reason = WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION;
			/* fall-through */
		case CLS_ACPT:
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
			if (!reason)
				reason = WLAN_REASON_MESH_CLOSE_RCVD;
			da16x_eloop_register_timeout(
				conf->dot11MeshHoldingTimeout / 1000,
				(conf->dot11MeshHoldingTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
			sta->mpm_close_reason = reason;
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		case OPN_ACPT:
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CONFIRM, 0);
			break;
		case CNF_ACPT:
			if (conf->security & MESH_CONF_SEC_AMPE)
				mesh_rsn_derive_mtk(wpa_s, sta);
			ret = mesh_mpm_plink_estab(wpa_s, sta);
			if(ret)
				return ret;
			break;
		default:
			break;
		}
		break;
	case PLINK_CNF_RCVD:
		switch (event) {
		case OPN_RJCT:
		case CNF_RJCT:
			if (!reason)
				reason = WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION;
			/* fall-through */
		case CLS_ACPT:
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
			if (!reason)
				reason = WLAN_REASON_MESH_CLOSE_RCVD;
			da16x_eloop_register_timeout(
				conf->dot11MeshHoldingTimeout / 1000,
				(conf->dot11MeshHoldingTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
			sta->mpm_close_reason = reason;
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		case OPN_ACPT:
			if (conf->security & MESH_CONF_SEC_AMPE)
				mesh_rsn_derive_mtk(wpa_s, sta);
			ret = mesh_mpm_plink_estab(wpa_s, sta);
			if(ret)
				return ret;
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CONFIRM, 0);
			break;
		default:
			break;
		}
		break;
	case PLINK_ESTAB:
		switch (event) {
		case OPN_RJCT:
		case CNF_RJCT:
		case CLS_ACPT:
			wpa_mesh_set_plink_state(wpa_s, sta, PLINK_HOLDING);
			if (!reason)
				reason = WLAN_REASON_MESH_CLOSE_RCVD;

			da16x_eloop_register_timeout(
				conf->dot11MeshHoldingTimeout / 1000,
				(conf->dot11MeshHoldingTimeout % 1000) * 1000,
				plink_timer, wpa_s, sta);
			sta->mpm_close_reason = reason;

			wpa_msg(wpa_s, MSG_INFO, "mesh plink with " MACSTR
				" closed with reason %d",
				MAC2STR(sta->addr), reason);

			wpa_msg(wpa_s, MSG_INFO, MESH_PEER_DISCONNECTED MACSTR,
				MAC2STR(sta->addr));

			/* Send D-Bus event */
#if 0 //courmagu			
			wpas_notify_mesh_peer_disconnected(wpa_s, sta->addr,
							   reason);
#endif
			wpa_msg(wpa_s, MSG_USER_DEBUG, "[%s] PLINK_ESTAB event=%d num_plinks=%d : num_plinks--", __func__, event, hapd->num_plinks);
			if (hapd->num_plinks > 0) {
				hapd->num_plinks--;
			}

			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		case OPN_ACPT:
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CONFIRM, 0);
			break;
		default:
			break;
		}
		break;
	case PLINK_HOLDING:
		switch (event) {
		case CLS_ACPT:
			wpa_msg(wpa_s, MSG_USER_DEBUG, "[%s] PLINK_HOLDING event=CLS_ACPT num_plinks=%d", __func__, wpa_s->ifmsh->bss[0]->num_plinks);
			mesh_mpm_fsm_restart(wpa_s, sta);
			break;
		case OPN_ACPT:
		case CNF_ACPT:
		case OPN_RJCT:
		case CNF_RJCT:
			reason = sta->mpm_close_reason;
			mesh_mpm_send_plink_action(wpa_s, sta,
						   PLINK_CLOSE, reason);
			break;
		default:
			break;
		}
		break;
	default:
		wpa_msg(wpa_s, MSG_DEBUG,
			"Unsupported MPM event %s for state %s",
			mplevent[event], mplstate[sta->plink_state]);
		break;
	}

	return ret;
}


void mesh_mpm_action_rx(struct wpa_supplicant *wpa_s,
			const struct _ieee80211_mgmt *mgmt, size_t len)
{
	u8 action_field;
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];
	struct mesh_conf *mconf = wpa_s->ifmsh->mconf;
	struct sta_info *sta;
	u16 plid = 0, llid = 0, aid = 0;
	enum plink_event event;
	struct ieee802_11_elems elems;
	struct mesh_peer_mgmt_ie peer_mgmt_ie;
	const u8 *ies;
	size_t ie_len;
	int ret;
	u16 reason = 0;

	if (mgmt->u.action.category != WLAN_ACTION_SELF_PROTECTED)
		return;

	action_field = mgmt->u.action.u.slf_prot_action.action;
	if (action_field != PLINK_OPEN &&
	    action_field != PLINK_CONFIRM &&
	    action_field != PLINK_CLOSE)
		return;

	ies = mgmt->u.action.u.slf_prot_action.variable;
	ie_len = (const u8 *) mgmt + len -
		mgmt->u.action.u.slf_prot_action.variable;

	/* at least expect mesh id and peering mgmt */
	if (ie_len < 2 + 2) {
		wpa_printf_dbg(MSG_DEBUG,
			   "MPM: Ignore too short action frame %u ie_len %u",
			   action_field, (unsigned int) ie_len);
		return;
	}
	wpa_printf_dbg(MSG_DEBUG, "MPM: Received PLINK action %u", action_field);

	if (action_field == PLINK_OPEN || action_field == PLINK_CONFIRM) {
		wpa_printf_dbg(MSG_DEBUG, "MPM: Capability 0x%x",
			   WPA_GET_LE16(ies));
		ies += 2;	/* capability */
		ie_len -= 2;
	}
	if (action_field == PLINK_CONFIRM) {
		aid = WPA_GET_LE16(ies);
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s AID 0x%x", mplstate[action_field], aid);
		ies += 2;	/* aid */
		ie_len -= 2;
	}

	/* check for mesh peering, mesh id and mesh config IEs */
	if (ieee802_11_parse_elements(ies, ie_len, &elems, 0) == ParseFailed) {
		wpa_printf(MSG_DEBUG, "MPM: %s Failed to parse PLINK IEs", mplstate[action_field]);
		return;
	}
	if (!elems.peer_mgmt) {
		wpa_printf_dbg(MSG_DEBUG,
			   "MPM: %s No Mesh Peering Management element", mplstate[action_field]);
		return;
	}
	if (action_field != PLINK_CLOSE) {
		if (!elems.mesh_id || !elems.mesh_config) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s No Mesh ID or Mesh Configuration element", mplstate[action_field]);
			return;
		}

		if (!matches_local(wpa_s, &elems)) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s Mesh ID or Mesh Configuration element do not match local MBSS", mplstate[action_field]);
			return;
		}
	}

	ret = mesh_mpm_parse_peer_mgmt(wpa_s, action_field,
				       elems.peer_mgmt,
				       elems.peer_mgmt_len,
				       &peer_mgmt_ie);
	if (ret) {
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s Mesh parsing rejected frame", mplstate[action_field]);
		return;
	}

	/* the sender's llid is our plid and vice-versa */
	plid = WPA_GET_LE16(peer_mgmt_ie.llid);
	if (peer_mgmt_ie.plid)
		llid = WPA_GET_LE16(peer_mgmt_ie.plid);
	wpa_printf_dbg(MSG_DEBUG, "MPM: %s plid=0x%x llid=0x%x", mplstate[action_field], plid, llid);

	if (action_field == PLINK_CLOSE)
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s close reason=%u", mplstate[action_field],
			   WPA_GET_LE16(peer_mgmt_ie.reason));

	sta = ap_get_sta(hapd, mgmt->sa);

#if 0 /* TEST Code Added by Dialog */
	if (sta == NULL && (hapd->num_sta == wpa_s->ifmsh->conf->bss[0]->max_num_sta)) { /* ���� �õ� �ܸ� ������ ���� ���?*/
		wpa_msg(wpa_s, MSG_DEBUG, "[%s] Discard Action_Rx "MACSTR " num_plinks=[%d/%d] num_sta=[%d/%d]",
			__func__,
			MAC2STR(mgmt->sa),
			hapd->num_plinks,
			hapd->max_plinks,
			hapd->num_sta,
			wpa_s->ifmsh->conf->bss[0]->max_num_sta);
		//wpa_printf_dbg(MSG_DEBUG, "MPM: %s No STA entry for peer", mplstate[action_field]);
		wpa_printf(MSG_DEBUG,
			   "MPM: %s STA num over quota(%d/%d)",
			   mplstate[action_field],
			   hapd->num_sta, wpa_s->ifmsh->conf->bss[0]->max_num_sta);
		return;

	} else {
		wpa_msg(wpa_s, MSG_DEBUG, "[%s] " MACSTR "num_plinks=[%d/%d] num_sta=[%d/%d]",
			__func__,
			MAC2STR(mgmt->sa),
			hapd->num_plinks,
			hapd->max_plinks,
			hapd->num_sta,
			wpa_s->ifmsh->conf->bss[0]->max_num_sta);
	/*
	 * If this is an open frame from an unknown STA, and this is an
	 * open mesh, then go ahead and add the peer before proceeding.
	 */
	if (!sta && action_field == PLINK_OPEN &&
	    (!(mconf->security & MESH_CONF_SEC_AMPE) ||
	     wpa_auth_pmksa_get(hapd->wpa_auth, mgmt->sa, NULL)))
		sta = mesh_mpm_add_peer(wpa_s, mgmt->sa, &elems);
	}
#else
	/*
	 * If this is an open frame from an unknown STA, and this is an
	 * open mesh, then go ahead and add the peer before proceeding.
	 */
	if (!sta && action_field == PLINK_OPEN &&
	    (!(mconf->security & MESH_CONF_SEC_AMPE) ||
	     wpa_auth_pmksa_get(hapd->wpa_auth, mgmt->sa, NULL)))
		sta = mesh_mpm_add_peer(wpa_s, mgmt->sa, &elems);
#endif  /* TEST Code Added by Dialog */

	if (!sta) {
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s No STA entry for peer", mplstate[action_field]);
		return;
	}

#ifdef CONFIG_SAE
	/* peer is in sae_accepted? */
	if (sta->sae && sta->sae->state != SAE_ACCEPTED) {
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s SAE not yet accepted for peer", mplstate[action_field]);
		return;
	}
#endif /* CONFIG_SAE */

	if (!sta->my_lid)
		mesh_mpm_init_link(wpa_s, sta);

	if (mconf->security & MESH_CONF_SEC_AMPE) {
		int res;

		res = mesh_rsn_process_ampe(wpa_s, sta, &elems,
					    &mgmt->u.action.category,
					    peer_mgmt_ie.chosen_pmk,
					    ies, ie_len);
		if (res) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s RSN process rejected frame (res=%d)", mplstate[action_field],
				   res);
			if (action_field == PLINK_OPEN && res == -2) {
				/* AES-SIV decryption failed */
			ret = mesh_mpm_fsm(wpa_s, sta, OPN_RJCT,
					     WLAN_REASON_MESH_INVALID_GTK);
			}
			return;
		}
	}

	if (sta->plink_state == PLINK_BLOCKED) {
		wpa_printf_dbg(MSG_DEBUG, "MPM: %s PLINK_BLOCKED", mplstate[action_field]);
		return;
	}

	/* Now we will figure out the appropriate event... */
	switch (action_field) {
	case PLINK_OPEN:
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
		if(plink_free_count(hapd) == 0)
		{
			int count = 0;

			count = mesh_hapd_sta_estab_count(hapd);

			if(hapd->max_plinks != count)
				hapd->num_plinks --;
		}
		
		if (plink_free_count(hapd) == 0) {
#else
  #if 0
    #if 0
		if (plink_free_count(hapd) == 0) {
    #else
      #if 0 
		if ((plink_free_count(hapd) == 0)&&(sta->plink_state != PLINK_ESTAB)) {
      #endif	
		if (((plink_free_count(hapd) == 0)&&(sta->plink_state != PLINK_ESTAB)) 
			||((plink_free_count(hapd) == 0) 
				&&(mesh_hapd_sta_count(hapd) != hapd->num_plinks))){
    #endif /* 0 */
  #endif
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED
			event = REQ_RJCT;
			reason = WLAN_REASON_MESH_MAX_PEERS;
			wpa_printf(MSG_INFO,
				   "MPM: %s Peer link num over quota(%d/%d)",
				   mplstate[action_field],
				   hapd->num_plinks, hapd->max_plinks);
		} else if (sta->peer_lid && sta->peer_lid != plid) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s peer_lid mismatch: 0x%x != 0x%x",
				   mplstate[action_field],
				   sta->peer_lid, plid);
			return; /* no FSM event */
		} else {
			sta->peer_lid = plid;
			event = OPN_ACPT;
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s Ok(OPN_ACPT) peer_lid=0x%x", mplstate[action_field],
				   sta->peer_lid);
		}
		break;
	case PLINK_CONFIRM:
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
		if(plink_free_count(hapd) == 0)
		{
			int count = 0;

			count = mesh_hapd_sta_estab_count(hapd);

			if(hapd->max_plinks != count)
				hapd->num_plinks --;
		}
		
		if (plink_free_count(hapd) == 0) {
#else
#if 0
  #if 0		
		if (plink_free_count(hapd) == 0) {
  #else
    #if 0
		if ((plink_free_count(hapd) == 0)&&(sta->plink_state != PLINK_ESTAB)) {
    #else
		if (((plink_free_count(hapd) == 0)&&(sta->plink_state != PLINK_ESTAB)) 
			||((plink_free_count(hapd) == 0) 
				&&(mesh_hapd_sta_count(hapd) != hapd->num_plinks))){
    #endif
  #endif
#endif
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED
			event = REQ_RJCT;
			reason = WLAN_REASON_MESH_MAX_PEERS;
			wpa_printf(MSG_INFO,
				   "MPM: %s->%s Peer link num over quota(%d/%d)",
				   mplstate[sta->plink_state], mplstate[action_field],
				   hapd->num_plinks, hapd->max_plinks);
		} else if (sta->my_lid != llid ||
			   (sta->peer_lid && sta->peer_lid != plid)) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s->%s lid mismatch: my_lid: 0x%x != 0x%x or peer_lid: 0x%x != 0x%x",
				   mplstate[sta->plink_state], mplstate[action_field],
				   sta->my_lid, llid, sta->peer_lid, plid);
			return; /* no FSM event */
		} else {
			if (!sta->peer_lid)
				sta->peer_lid = plid;
			sta->peer_aid = aid;
			event = CNF_ACPT;
		}
		break;
	case PLINK_CLOSE:
		if (sta->plink_state == PLINK_ESTAB) {
			/* Do not check for llid or plid. This does not
			 * follow the standard but since multiple plinks
			 * per cand are not supported, it is necessary in
			 * order to avoid a livelock when MP A sees an
			 * establish peer link to MP B but MP B does not
			 * see it. This can be caused by a timeout in
			 * B's peer link establishment or B being
			 * restarted.
			 */
			event = CLS_ACPT;
			wpa_printf_dbg(MSG_DEBUG,
				"MPM: %s->%s event=CLS_ACPT plid=0x%x", mplstate[sta->plink_state], mplstate[action_field], plid);
		} else if (sta->peer_lid != plid) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s->%s peer_lid mismatch: 0x%x != 0x%x",
				   mplstate[sta->plink_state], mplstate[action_field],
				   sta->peer_lid, plid);
			return; /* no FSM event */
		} else if (peer_mgmt_ie.plid && sta->my_lid != llid) {
			wpa_printf_dbg(MSG_DEBUG,
				   "MPM: %s->%s my_lid mismatch: 0x%x != 0x%x",
				   mplstate[sta->plink_state], mplstate[action_field],
				   sta->my_lid, llid);
			return; /* no FSM event */
		} else {
			event = CLS_ACPT;
			wpa_printf_dbg(MSG_DEBUG,
				"MPM: %s->%s event=CLS_ACPT plid=0x%x", mplstate[sta->plink_state], mplstate[action_field], plid);
		}
		break;
	default:
		/*
		 * This cannot be hit due to the action_field check above, but
		 * compilers may not be able to figure that out and can warn
		 * about uninitialized event below.
		 */
		return;
	}
	ret = mesh_mpm_fsm(wpa_s, sta, event, reason);
}


/* called by ap_free_sta */
void mesh_mpm_free_sta(struct hostapd_data *hapd, struct sta_info *sta)
{
	if (sta->plink_state == PLINK_ESTAB) {
		if (hapd->num_plinks > 0) {
			wpa_printf_dbg(MSG_DEBUG, "[%s] "MACSTR" %s num_plinks=%d : num_plinks--", __func__, MAC2STR(sta->addr), mplstate[sta->plink_state], hapd->num_plinks);
			hapd->num_plinks--;
		}
	} else {
		wpa_printf_dbg(MSG_DEBUG, "[%s] "MACSTR" %s num_plinks=%d", __func__, MAC2STR(sta->addr), mplstate[sta->plink_state], hapd->num_plinks);
	}

	da16x_eloop_cancel_timeout(plink_timer, ELOOP_ALL_CTX, sta);
	da16x_eloop_cancel_timeout(mesh_auth_timer, ELOOP_ALL_CTX, sta);
}

#if 1 /* Add by Dialog */
#define MESH_MPM_STATE_REJECT_TIMER 500 /* 5000 ms */

int mesh_hapd_sta_count(struct hostapd_data *hapd)
{
	int count =0;
	struct sta_info *sta = NULL;
	
	for (int  i = 0; i < STA_HASH_SIZE; i++) { 
		sta = hapd->sta_hash[i];
		while (sta != NULL) {
			count ++;
			sta = sta->hnext;
		}	
	}

	return count;
}

#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
int mesh_hapd_sta_estab_count(struct hostapd_data *hapd)
{
	int count =0;
	struct sta_info *sta = NULL;
	
	for (int  i = 0; i < STA_HASH_SIZE; i++) { 
		sta = hapd->sta_hash[i];
		while (sta != NULL) {
			if(sta->plink_state == PLINK_ESTAB)
				count ++;
			sta = sta->hnext;
		}	
	}

	return count;
}
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED

static struct sta_info *mesh_sta_state_other(struct hostapd_data *hapd)
{
	struct sta_info *sta = NULL;
	
	for (int  i = 0; i < STA_HASH_SIZE; i++) { 
		sta = hapd->sta_hash[i];
		
		while (sta != NULL) {
			if (sta->plink_state == PLINK_HOLDING || sta->plink_state == PLINK_BLOCKED) {
				return sta;
			}

			if ((sta->plink_state == PLINK_IDLE)
				&& (( xTaskGetTickCount()/*tx_time_get()*/ - sta->plink_intial_time) > MESH_MPM_STATE_REJECT_TIMER)) {
				wpa_printf_dbg(MSG_DEBUG, "[%s] Elapsed time: %d ms", __func__, 
					(xTaskGetTickCount()/*tx_time_get()*/ - sta->plink_intial_time) * 10);
				return sta;
			}	
			
			sta = sta->hnext;
		}
	}
	
	return NULL;
}

void mesh_mpm_sta_sate_remove(struct wpa_supplicant *wpa_s)
{
	
	struct hostapd_data *hapd = wpa_s->ifmsh->bss[0];  
	struct sta_info *sta = NULL;
	
	sta = mesh_sta_state_other(hapd);

	if (sta != NULL) {
		mesh_mpm_sta_remove(wpa_s,  (const u8 *)sta->addr);
	}
}
#endif  /* Add by Dialog */

#endif	/* CONFIG_MESH */
