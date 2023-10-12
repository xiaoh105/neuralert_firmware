/*
 * WPA Supplicant
 * Copyright (c) 2003-2018, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file implements functions for registering and unregistering
 * %wpa_supplicant interfaces. In addition, this file contains number of
 * functions for managing network connections.
 */

#include "includes.h"
#ifdef CONFIG_MATCH_IFACE
#include <net/if.h>
#include <fnmatch.h>
#endif /* CONFIG_MATCH_IFACE */

#include "supp_common.h"
#include "crypto/random.h"
#include "crypto/sha1.h"
#ifdef	IEEE8021X_EAPOL
#include "eapol_supp/eapol_supp_sm.h"
#include "eap_peer/eap.h"
#include "eap_peer/eap_proxy.h"
#include "eap_server/eap_methods.h"
#endif	/* IEEE8021X_EAPOL */
#include "rsn_supp/wpa.h"
#include "supp_eloop.h"
#include "supp_config.h"
#ifdef CONFIG_INTERWORKING
#include "utils/ext_password.h"
#endif /* CONFIG_INTERWORKING */
#include "l2_packet/l2_packet.h"

#include "wpa_common.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
//#include "ctrl_iface.h"
//#include "pcsc_funcs.h"
//#include "common/version.h"

#ifdef	CONFIG_PRE_AUTH
#include "rsn_supp/preauth.h"
#endif	/* CONFIG_PRE_AUTH */
#ifdef	CONFIG_PMKSA
#include "rsn_supp/pmksa_cache.h"
#endif	/* CONFIG_PMKSA */
#include "wpa_ctrl.h"
#include "common/ieee802_11_defs.h"
//#include "common/hw_features_common.h"
#ifdef CONFIG_GAS
#include "common/gas_server.h"
#endif /* CONFIG_GAS */
#include "p2p/supp_p2p.h"
#ifdef CONFIG_FST
#include "fst/fst.h"
#endif /* CONFIG_FST */
#include "blacklist.h"
#include "wpas_glue.h"
#include "wps_supplicant.h"
#ifdef CONFIG_IBSS_RSN
#include "ibss_rsn.h"
#endif /* CONFIG_IBSS_RSN */
#include "supp_sme.h"

#ifdef CONFIG_GAS
#include "gas_query.h"
#endif /* CONFIG_GAS */

#include "ap.h"

#include "environ.h"

#ifdef	CONFIG_P2P
#include "p2p_supplicant.h"
#endif	/* CONFIG_P2P */
#ifdef	CONFIG_WIFI_DISPLAY
#include "wifi_display.h"
#endif	/* CONFIG_WIFI_DISPLAY */
//#include "notify.h"

#ifdef	CONFIG_BGSCAN
#include "bgscan.h"
#endif	/* CONFIG_BGSCAN */

#include "autoscan.h"
#include "bss.h"
#include "supp_scan.h"
#include "offchannel.h"
#ifdef CONFIG_HS20
#include "hs20_supplicant.h"
#endif /* CONFIG_HS20 */

#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"

#include "wnm_sta.h"
//#include "wpas_kay.h"
#ifdef CONFIG_MESH
#include "supp_mesh.h"
#endif /* CONFIG_MESH */
#ifdef CONFIG_DPP
#include "dpp_supplicant.h"
#endif /* CONFIG_DPP */
#ifdef CONFIG_MESH
#include "ap/ap_config.h"
#include "ap/hostapd.h"
#endif /* CONFIG_MESH */

static int wpa_supp_dpm_backup_wep_key(struct wpa_supplicant *wpa_s);
static int wpa_supp_dpm_backup_conn_info(struct wpa_supplicant *wpa_s);
void	wpa_supp_dpm_clear_conn_info(struct wpa_supplicant *wpa_s);

#ifdef AUTO_WEPKEY_INDEX
extern int wpa_supplicant_ctrl_iface_save_config(struct wpa_supplicant *wpa_s);
#endif /* AUTO_WEPKEY_INDEX */

extern int get_run_mode(void);

#ifdef CONFIG_FAST_RECONNECT_V2
extern u8 da16x_is_fast_reconnect;
extern u8 fast_reconnect_tries;
#endif /* CONFIG_FAST_RECONNECT_V2 */

extern int wpa_supplicant_abort_scan(struct wpa_supplicant *wpa_s,
				struct wpa_driver_scan_params *params);
extern struct hostapd_hw_modes *fc80211_get_hw_feature_data
	(int wifi_mode, u16 *num_modes, u16 *flags, u16 set_greenfield, int min_channel, int num_channels, char *country);

extern int country_channel_range (char* country, int *startCH, int *endCH);

extern dpm_supp_key_info_t	*dpm_supp_key_info;
extern dpm_supp_conn_info_t	*dpm_supp_conn_info;

extern void (*wifi_conn_notify_cb)(void);
extern void (*wifi_conn_fail_notify_cb)(short reason_code);
extern void (*wifi_disconn_notify_cb)(short reason_code);

extern int wifi_netif_status(int intf);

#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
extern void supp_rehold_bss(void *temp);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

extern void wpa_supplicant_cancel_scan(struct wpa_supplicant *wpa_s);
extern void set_disable_ht40(struct ieee80211_ht_capabilities *htcaps, int disabled);

// for get_cur_connected_bssid() API
extern u8  cur_connected_bssid[ETH_ALEN];

#if 0
const char *const wpa_supplicant_version =
"wpa_supplicant v" VERSION_STR "\n"
"Copyright (c) 2003-2017, Jouni Malinen <j@w1.fi> and contributors";

const char *const wpa_supplicant_license =
"This software may be distributed under the terms of the BSD license.\n"
"See README for more details.\n"
#ifdef EAP_TLS_OPENSSL
"\nThis product includes software developed by the OpenSSL Project\n"
"for use in the OpenSSL Toolkit (http://www.openssl.org/)\n"
#endif /* EAP_TLS_OPENSSL */
#endif /* 0 */
;

#if 1
int wpa_debug_level = MSG_INFO;
int wpa_debug_show_keys = 1;
int wpa_debug_timestamp = 0;
#endif /* 1 */

extern unsigned char fast_connection_sleep_flag;

#ifdef	UNUSED_CODE
#ifndef CONFIG_NO_STDOUT_DEBUG
/* Long text divided into parts in order to fit in C89 strings size limits. */
const char *const wpa_supplicant_full_license1 =
"";
const char *const wpa_supplicant_full_license2 =
"This software may be distributed under the terms of the BSD license.\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n";
const char *const wpa_supplicant_full_license3 =
"1. Redistributions of source code must retain the above copyright\n"
"   notice, this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"   notice, this list of conditions and the following disclaimer in the\n"
"   documentation and/or other materials provided with the distribution.\n"
"\n";
const char *const wpa_supplicant_full_license4 =
"3. Neither the name(s) of the above-listed copyright holder(s) nor the\n"
"   names of its contributors may be used to endorse or promote products\n"
"   derived from this software without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
"A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n";
const char *const wpa_supplicant_full_license5 =
"OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
"SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n"
"LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n"
"DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n"
"THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
"\n";

#endif /* CONFIG_NO_STDOUT_DEBUG */
#endif	/* UNUSED_CODE */


#ifdef __SUPP_27_SUPPORT__
static void wpa_bss_tmp_disallow_timeout(void *eloop_ctx, void *timeout_ctx);
#endif /* __SUPP_27_SUPPORT__ */
#if defined(CONFIG_FILS) && defined(IEEE8021X_EAPOL)
static void wpas_update_fils_connect_params(struct wpa_supplicant *wpa_s);
#endif /* CONFIG_FILS && IEEE8021X_EAPOL */

int wpa_supp_ifname_to_ifindex(const char *ifname);


/* Configure default/group WEP keys for static WEP */
int wpa_set_wep_keys(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	int i, set = 0;

	for (i = 0; i < NUM_WEP_KEYS; i++) {
		if (ssid->wep_key_len[i] == 0)
			continue;

		set = 1;
		wpa_drv_set_key(wpa_s, WPA_ALG_WEP, NULL,
				i, i == ssid->wep_tx_keyidx, NULL, 0,
				ssid->wep_key[i], ssid->wep_key_len[i]);
		wpa_supp_dpm_backup_wep_key(wpa_s);
	}

	return set;
}


int wpa_supplicant_set_wpa_none_key(struct wpa_supplicant *wpa_s,
				    struct wpa_ssid *ssid)
{
	u8 key[32];
	size_t keylen;
	enum wpa_alg alg;
	u8 seq[6] = { 0 };
	int ret;

	/* IBSS/WPA-None uses only one key (Group) for both receiving and
	 * sending unicast and multicast packets. */

	if (ssid->mode != WPAS_MODE_IBSS) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Invalid mode %d (not "
			"IBSS/ad-hoc) for WPA-None", ssid->mode);
#else
		da16x_info_prt("[%s] WPA: Invalid mode %d (not "
			"IBSS/ad-hoc) for WPA-None\n", __func__, ssid->mode);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	if (!ssid->psk_set) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: No PSK configured for "
			"WPA-None");
#else
		da16x_info_prt("[%s] WPA: No PSK configured for WPA-None\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	switch (wpa_s->group_cipher) {
	case WPA_CIPHER_CCMP:
		os_memcpy(key, ssid->psk, 16);
		keylen = 16;
		alg = WPA_ALG_CCMP;
		break;
	case WPA_CIPHER_GCMP:
		os_memcpy(key, ssid->psk, 16);
		keylen = 16;
		alg = WPA_ALG_GCMP;
		break;
	case WPA_CIPHER_TKIP:
		/* WPA-None uses the same Michael MIC key for both TX and RX */
		os_memcpy(key, ssid->psk, 16 + 8);
		os_memcpy(key + 16 + 8, ssid->psk + 16, 8);
		keylen = 32;
		alg = WPA_ALG_TKIP;
		break;
	default:
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Invalid group cipher %d for "
			"WPA-None", wpa_s->group_cipher);
#else
		da16x_info_prt("[%s] WPA: Invalid group cipher %d for WPA-None\n",
				__func__, wpa_s->group_cipher);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	/* TODO: should actually remember the previously used seq#, both for TX
	 * and RX from each STA.. */

	ret = wpa_drv_set_key(wpa_s, alg, NULL, 0, 1, seq, 6, key, keylen);
	os_memset(key, 0, sizeof(key));
	return ret;
}


static void wpa_supplicant_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	const u8 *bssid = wpa_s->bssid;
	if (!is_zero_ether_addr(wpa_s->pending_bssid) &&
	    (wpa_s->wpa_state == WPA_AUTHENTICATING ||
	     wpa_s->wpa_state == WPA_ASSOCIATING))
		bssid = wpa_s->pending_bssid;
#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_msg(wpa_s, MSG_INFO, "Auth with " MACSTR " timed out.",
		MAC2STR(bssid));
#else
	da16x_info_prt("[%s] Auth with " MACSTR " timed out.\n",
		__func__, MAC2STR(bssid));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
	wpa_blacklist_add(wpa_s, bssid);
	wpa_sm_notify_disassoc(wpa_s->wpa);
	wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
	wpa_s->reassociate = 1;

	/*
	 * If we timed out, the AP or the local radio may be busy.
	 * So, wait a second until scanning again.
	 */
	wpa_supplicant_req_scan(wpa_s, 1, 0);
}


/**
 * wpa_supplicant_req_auth_timeout - Schedule a timeout for authentication
 * @wpa_s: Pointer to wpa_supplicant data
 * @sec: Number of seconds after which to time out authentication
 * @usec: Number of microseconds after which to time out authentication
 *
 * This function is used to schedule a timeout for the current authentication
 * attempt.
 */
void wpa_supplicant_req_auth_timeout(struct wpa_supplicant *wpa_s,
				     int sec, int usec)
{
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (wpa_s->conf->ap_scan == 0 &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_WIRED))
		return;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	wpa_dbg(wpa_s, MSG_DEBUG, "Setting authentication timeout: %d sec "
		"%d usec", sec, usec);
	da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);
#if defined ( __SUPP_27_SUPPORT__ )
	wpa_s->last_auth_timeout_sec = sec;
#endif // __SUPP_27_SUPPORT__
	da16x_eloop_register_timeout(sec, usec, wpa_supplicant_timeout, wpa_s, NULL);
}


#if defined ( __SUPP_27_SUPPORT__ )
/*
 * wpas_auth_timeout_restart - Restart and change timeout for authentication
 * @wpa_s: Pointer to wpa_supplicant data
 * @sec_diff: difference in seconds applied to original timeout value
 */
void wpas_auth_timeout_restart(struct wpa_supplicant *wpa_s, int sec_diff)
{
	int new_sec = wpa_s->last_auth_timeout_sec + sec_diff;

	if (da16x_eloop_is_timeout_registered(wpa_supplicant_timeout, wpa_s, NULL)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Authentication timeout restart: %d sec", new_sec);
#else
		da16x_debug_prt("[%s] Authentication timeout restart: %d sec\n", __func__, new_sec);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);
		da16x_eloop_register_timeout(new_sec, 0, wpa_supplicant_timeout,
				       wpa_s, NULL);
	}
}
#endif // __SUPP_27_SUPPORT__


/**
 * wpa_supplicant_cancel_auth_timeout - Cancel authentication timeout
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to cancel authentication timeout scheduled with
 * wpa_supplicant_req_auth_timeout() and it is called when authentication has
 * been completed.
 */
void wpa_supplicant_cancel_auth_timeout(struct wpa_supplicant *wpa_s)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "Cancelling authentication timeout");
	da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);
	wpa_blacklist_del(wpa_s, wpa_s->bssid);
}


/**
 * wpa_supplicant_initiate_eapol - Configure EAPOL state machine
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to configure EAPOL state machine based on the selected
 * authentication mode.
 */
void wpa_supplicant_initiate_eapol(struct wpa_supplicant *wpa_s)
{
#ifdef IEEE8021X_EAPOL
	struct eapol_config eapol_conf;
	struct wpa_ssid *ssid = wpa_s->current_ssid;

#ifdef CONFIG_IBSS_RSN
	if (ssid->mode == WPAS_MODE_IBSS &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_NONE &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_WPA_NONE) {
		/*
		 * RSN IBSS authentication is per-STA and we can disable the
		 * per-BSSID EAPOL authentication.
		 */
		eapol_sm_notify_portControl(wpa_s->eapol, ForceAuthorized);
		eapol_sm_notify_eap_success(wpa_s->eapol, TRUE);
		eapol_sm_notify_eap_fail(wpa_s->eapol, FALSE);
		return;
	}
#endif /* CONFIG_IBSS_RSN */

	eapol_sm_notify_eap_success(wpa_s->eapol, FALSE);
	eapol_sm_notify_eap_fail(wpa_s->eapol, FALSE);

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE)
		eapol_sm_notify_portControl(wpa_s->eapol, SUPP_ForceAuthorized);
	else
		eapol_sm_notify_portControl(wpa_s->eapol, SUPP_Auto);

	os_memset(&eapol_conf, 0, sizeof(eapol_conf));
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		eapol_conf.accept_802_1x_keys = 1;
		eapol_conf.required_keys = 0;
		if (ssid->eapol_flags & EAPOL_FLAG_REQUIRE_KEY_UNICAST) {
			eapol_conf.required_keys |= EAPOL_REQUIRE_KEY_UNICAST;
		}
		if (ssid->eapol_flags & EAPOL_FLAG_REQUIRE_KEY_BROADCAST) {
			eapol_conf.required_keys |=
				EAPOL_REQUIRE_KEY_BROADCAST;
		}

		if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_WIRED)
			eapol_conf.required_keys = 0;
	}
#ifdef	CONFIG_FAST_REAUTH
	eapol_conf.fast_reauth = wpa_s->conf->fast_reauth;
#endif	/* CONFIG_FAST_REAUTH */
	eapol_conf.workaround = ssid->eap_workaround;
	eapol_conf.eap_disabled =
		!wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) &&
		wpa_s->key_mgmt != WPA_KEY_MGMT_IEEE8021X_NO_WPA &&
		wpa_s->key_mgmt != WPA_KEY_MGMT_WPS;
#ifdef	CONFIG_EXT_SIM
	eapol_conf.external_sim = wpa_s->conf->external_sim;
#endif	/* CONFIG_EXT_SIM */

#ifdef CONFIG_WPS
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPS) {
		eapol_conf.wps |= EAPOL_LOCAL_WPS_IN_USE;
		if (wpa_s->current_bss) {
			struct wpabuf *ie;
			ie = wpa_bss_get_vendor_ie_multi(wpa_s->current_bss,
							 WPS_IE_VENDOR_TYPE);
			if (ie) {
				if (wps_is_20(ie))
					eapol_conf.wps |=
						EAPOL_PEER_IS_WPS20_AP;
				wpabuf_free(ie);
			}
		}
	}
#endif /* CONFIG_WPS */

	eapol_sm_notify_config(wpa_s->eapol, &ssid->eap, &eapol_conf);

#ifdef CONFIG_MACSEC
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE && ssid->mka_psk_set)
		ieee802_1x_create_preshared_mka(wpa_s, ssid);
	else
		ieee802_1x_alloc_kay_sm(wpa_s, ssid);
#endif /* CONFIG_MACSEC */
#endif /* IEEE8021X_EAPOL */
}


/**
 * wpa_supplicant_set_non_wpa_policy - Set WPA parameters to non-WPA mode
 * @wpa_s: Pointer to wpa_supplicant data
 * @ssid: Configuration data for the network
 *
 * This function is used to configure WPA state machine and related parameters
 * to a mode where WPA is not enabled. This is called as part of the
 * authentication configuration when the selected network does not use WPA.
 */
void wpa_supplicant_set_non_wpa_policy(struct wpa_supplicant *wpa_s,
				       struct wpa_ssid *ssid)
{
	int i;

	if (ssid->key_mgmt & WPA_KEY_MGMT_WPS)
		wpa_s->key_mgmt = WPA_KEY_MGMT_WPS;
	else if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA)
		wpa_s->key_mgmt = WPA_KEY_MGMT_IEEE8021X_NO_WPA;
	else
		wpa_s->key_mgmt = WPA_KEY_MGMT_NONE;
	wpa_sm_set_ap_wpa_ie(wpa_s->wpa, NULL, 0);
	wpa_sm_set_ap_rsn_ie(wpa_s->wpa, NULL, 0);
	wpa_sm_set_assoc_wpa_ie(wpa_s->wpa, NULL, 0);
	wpa_s->pairwise_cipher = WPA_CIPHER_NONE;
	wpa_s->group_cipher = WPA_CIPHER_NONE;
	wpa_s->mgmt_group_cipher = 0;

	for (i = 0; i < NUM_WEP_KEYS; i++) {
		if (ssid->wep_key_len[i] > 5) {
			wpa_s->pairwise_cipher = WPA_CIPHER_WEP104;
			wpa_s->group_cipher = WPA_CIPHER_WEP104;
			break;
		} else if (ssid->wep_key_len[i] > 0) {
			wpa_s->pairwise_cipher = WPA_CIPHER_WEP40;
			wpa_s->group_cipher = WPA_CIPHER_WEP40;
			break;
		}
	}

	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_RSN_ENABLED, 0);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_KEY_MGMT, wpa_s->key_mgmt);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_PAIRWISE,
			 wpa_s->pairwise_cipher);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_GROUP, wpa_s->group_cipher);
#ifdef CONFIG_IEEE80211W
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_MGMT_GROUP,
			 wpa_s->mgmt_group_cipher);
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_PMKSA
	pmksa_cache_clear_current(wpa_s->wpa);
#endif /* CONFIG_PMKSA */
}


void free_hw_features(struct wpa_supplicant *wpa_s)
{
	int i;
	if (wpa_s->hw.modes == NULL)
		return;

	for (i = 0; i < wpa_s->hw.num_modes; i++) {
		os_free(wpa_s->hw.modes[i].channels);
		os_free(wpa_s->hw.modes[i].rates);
	}

	os_free(wpa_s->hw.modes);
	wpa_s->hw.modes = NULL;
}


#if defined ( __SUPP_27_SUPPORT__ )
static void free_bss_tmp_disallowed(struct wpa_supplicant *wpa_s)
{
	struct wpa_bss_tmp_disallowed *bss, *prev;

	dl_list_for_each_safe(bss, prev, &wpa_s->bss_tmp_disallowed,
			      struct wpa_bss_tmp_disallowed, list) {
		da16x_eloop_cancel_timeout(wpa_bss_tmp_disallow_timeout, wpa_s, bss);
		dl_list_del(&bss->list);
		os_free(bss);
	}
}
#endif // __SUPP_27_SUPPORT__


#ifdef CONFIG_FILS
void wpas_flush_fils_hlp_req(struct wpa_supplicant *wpa_s)
{
	struct fils_hlp_req *req;

	while ((req = dl_list_first(&wpa_s->fils_hlp_req, struct fils_hlp_req,
				    list)) != NULL) {
		dl_list_del(&req->list);
		wpabuf_free(req->pkt);
		os_free(req);
	}
}
#endif /* CONFIG_FILS */

#ifdef CONFIG_IEEE80211W
extern void da16x_sme_deinit(struct wpa_supplicant *wpa_s);
#endif /* CONFIG_IEEE80211W */ 

static void wpa_supplicant_cleanup(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_VENDOR_ELEM
	int i;
#endif /* CONFIG_VENDOR_ELEM */

#ifdef	CONFIG_BGSCAN
	bgscan_deinit(wpa_s);
#endif	/* CONFIG_BGSCAN */

	autoscan_deinit(wpa_s);
#ifdef PCSC_FUNCS
	scard_deinit(wpa_s->scard);
	wpa_s->scard = NULL;
	wpa_sm_set_scard_ctx(wpa_s->wpa, NULL);
	eapol_sm_register_scard_ctx(wpa_s->eapol, NULL);
#endif /* PCSC_FUNCS */

	l2_packet_deinit(wpa_s->l2);

	wpa_s->l2 = NULL;
#ifdef	CONFIG_BRIDGE_IFACE
	if (wpa_s->l2_br) {
		l2_packet_deinit(wpa_s->l2_br);
		wpa_s->l2_br = NULL;
	}
#endif	/* CONFIG_BRIDGE_IFACE */

#ifdef CONFIG_TESTING_OPTIONS
	l2_packet_deinit(wpa_s->l2_test);
	wpa_s->l2_test = NULL;
	os_free(wpa_s->get_pref_freq_list_override);
	wpa_s->get_pref_freq_list_override = NULL;
	wpabuf_free(wpa_s->last_assoc_req_wpa_ie);
	wpa_s->last_assoc_req_wpa_ie = NULL;
#endif /* CONFIG_TESTING_OPTIONS */

#if 0	/* by Bright : Merge 2.7 */
	if (wpa_s->conf != NULL) {
		struct wpa_ssid *ssid;
		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next)
			wpas_notify_network_removed(wpa_s, ssid);
	}
#endif	/* 0 */

#if defined ( __SUPP_27_SUPPORT__ )
	os_free(wpa_s->confname);
	wpa_s->confname = NULL;

	os_free(wpa_s->confanother);
	wpa_s->confanother = NULL;
#endif // __SUPP_27_SUPPORT__

#ifdef	IEEE8021X_EAPOL
	wpa_sm_set_eapol(wpa_s->wpa, NULL);
	eapol_sm_deinit(wpa_s->eapol);
	wpa_s->eapol = NULL;

#ifdef	CONFIG_PRE_AUTH
	rsn_preauth_deinit(wpa_s->wpa);
#endif	/* CONFIG_PRE_AUTH */
#endif	/* IEEE8021X_EAPOL */

#ifdef CONFIG_TDLS
	wpa_tdls_deinit(wpa_s->wpa);
#endif /* CONFIG_TDLS */

#if 0	/* by Bright : Merge 2.7 */
	wmm_ac_clear_saved_tspecs(wpa_s);
#endif	/* 0 */

#ifdef	CONFIG_PRE_AUTH
	pmksa_candidate_free(wpa_s->wpa);
#endif	/* CONFIG_PRE_AUTH */
	wpa_sm_deinit(wpa_s->wpa);
	wpa_s->wpa = NULL;
	wpa_blacklist_clear(wpa_s);

	wpa_bss_deinit(wpa_s);

#if defined ( CONFIG_SCHED_SCAN )
	wpa_supplicant_cancel_delayed_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN
	wpa_supplicant_cancel_scan(wpa_s);
	wpa_supplicant_cancel_auth_timeout(wpa_s);
	da16x_eloop_cancel_timeout(wpa_supplicant_stop_countermeasures, wpa_s, NULL);
#ifdef CONFIG_DELAYED_MIC_ERROR_REPORT
	da16x_eloop_cancel_timeout(wpa_supplicant_delayed_mic_error_report,
			     wpa_s, NULL);
#endif /* CONFIG_DELAYED_MIC_ERROR_REPORT */

	da16x_eloop_cancel_timeout(wpas_network_reenabled, wpa_s, NULL);

	wpas_wps_deinit(wpa_s);

	wpabuf_free(wpa_s->pending_eapol_rx);
	wpa_s->pending_eapol_rx = NULL;

#ifdef CONFIG_IBSS_RSN
	ibss_rsn_deinit(wpa_s->ibss_rsn);
	wpa_s->ibss_rsn = NULL;
#endif /* CONFIG_IBSS_RSN */

#ifdef CONFIG_IEEE80211W
	da16x_sme_deinit(wpa_s);
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_AP
	wpa_supplicant_ap_deinit(wpa_s);
#endif /* CONFIG_AP */

#ifdef CONFIG_P2P
	wpas_p2p_deinit(wpa_s);
#endif /* CONFIG_P2P */

#ifdef CONFIG_OFFCHANNEL
	offchannel_deinit(wpa_s);
#endif /* CONFIG_OFFCHANNEL */

#if defined ( CONFIG_SCHED_SCAN )
	wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN

	os_free(wpa_s->next_scan_freqs);
	wpa_s->next_scan_freqs = NULL;

	os_free(wpa_s->manual_scan_freqs);
	wpa_s->manual_scan_freqs = NULL;
	os_free(wpa_s->select_network_scan_freqs);
	wpa_s->select_network_scan_freqs = NULL;

	os_free(wpa_s->manual_sched_scan_freqs);
	wpa_s->manual_sched_scan_freqs = NULL;

#ifdef	CONFIG_MAC_RAND_SCAN	
	wpas_mac_addr_rand_scan_clear(wpa_s, MAC_ADDR_RAND_ALL);
#endif	/* CONFIG_MAC_RAND_SCAN */

	/*
	 * Need to remove any pending gas-query radio work before the
	 * gas_query_deinit() call because gas_query::work has not yet been set
	 * for works that have not been started. gas_query_free() will be unable
	 * to cancel such pending radio works and once the pending gas-query
	 * radio work eventually gets removed, the deinit notification call to
	 * gas_query_start_cb() would result in dereferencing freed memory.
	 */
#if defined ( CONFIG_RADIO_WORK )
	if (wpa_s->radio)
		radio_remove_works(wpa_s, "gas-query", 0);
#endif // CONFIG_RADIO_WORK
#ifdef CONFIG_GAS
	gas_query_deinit(wpa_s->gas);
	wpa_s->gas = NULL;
	gas_server_deinit(wpa_s->gas_server);
	wpa_s->gas_server = NULL;
#endif /* CONFIG_GAS */

	free_hw_features(wpa_s);

#ifdef CONFIG_MACSEC
	ieee802_1x_dealloc_kay_sm(wpa_s);
#endif /* CONFIG_MACSEC */

#ifdef CONFIG_STA_BSSID_FILTER
	os_free(wpa_s->bssid_filter);
	wpa_s->bssid_filter = NULL;
#endif /* CONFIG_STA_BSSID_FILTER */

	os_free(wpa_s->disallow_aps_bssid);
	wpa_s->disallow_aps_bssid = NULL;
	os_free(wpa_s->disallow_aps_ssid);
	wpa_s->disallow_aps_ssid = NULL;

#ifdef CONFIG_WNM_BSS_MAX_IDLE_PERIOD
	wnm_bss_keep_alive_deinit(wpa_s);
#endif /* CONFIG_WNM_BSS_MAX_IDLE_PERIOD */

#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_BSS_TRANS_MGMT
	wnm_deallocate_memory(wpa_s);
#endif /* CONFIG_WNM_BSS_TRANS_MGMT */
#endif /* CONFIG_WNM */

#ifdef CONFIG_EXT_PASSWORD
	ext_password_deinit(wpa_s->ext_pw);
	wpa_s->ext_pw = NULL;
#endif /* CONFIG_EXT_PASSWORD */

#ifdef CONFIG_GAS
	wpabuf_free(wpa_s->last_gas_resp);
	wpa_s->last_gas_resp = NULL;
	wpabuf_free(wpa_s->prev_gas_resp);
	wpa_s->prev_gas_resp = NULL;
#endif /* CONFIG_GAS */

	os_free(wpa_s->last_scan_res);
	wpa_s->last_scan_res = NULL;

#ifdef CONFIG_HS20
	if (wpa_s->drv_priv)
		wpa_drv_configure_frame_filters(wpa_s, 0);
	hs20_deinit(wpa_s);
#endif /* CONFIG_HS20 */

#if defined ( CONFIG_VENDOR_ELEM )
	for (i = 0; i < NUM_VENDOR_ELEM_FRAMES; i++) {
		wpabuf_free(wpa_s->vendor_elem[i]);
		wpa_s->vendor_elem[i] = NULL;
	}
#endif	// CONFIG_VENDOR_ELEM

#if 0	/* by Bright : Merge 2.7 */
	wmm_ac_notify_disassoc(wpa_s);
#endif	/* 0 */

#ifdef CONFIG_SCHED_SCAN
	wpa_s->sched_scan_plans_num = 0;
	os_free(wpa_s->sched_scan_plans);
	wpa_s->sched_scan_plans = NULL;
#endif /* CONFIG_SCHED_SCAN */

#ifdef CONFIG_MBO
	wpa_s->non_pref_chan_num = 0;
	os_free(wpa_s->non_pref_chan);
	wpa_s->non_pref_chan = NULL;
#endif /* CONFIG_MBO */

#if defined ( __SUPP_27_SUPPORT__ )
	free_bss_tmp_disallowed(wpa_s);
#endif // __SUPP_27_SUPPORT__

#ifdef	CONFIG_LCI
	wpabuf_free(wpa_s->lci);
	wpa_s->lci = NULL;
#endif	/* CONFIG_LCI */
#if 0	/* by Bright : Merge 2.7 */
	wpas_clear_beacon_rep_data(wpa_s);
#endif	/* 0 */

#ifdef CONFIG_PMKSA_CACHE_EXTERNAL
#ifdef CONFIG_MESH
	{
		struct external_pmksa_cache *entry;

		while ((entry = dl_list_last(&wpa_s->mesh_external_pmksa_cache,
					     struct external_pmksa_cache,
					     list)) != NULL) {
			dl_list_del(&entry->list);
			os_free(entry->pmksa_cache);
			os_free(entry);
		}
	}
#endif /* CONFIG_MESH */
#endif /* CONFIG_PMKSA_CACHE_EXTERNAL */

#ifdef CONFIG_FILS
	wpas_flush_fils_hlp_req(wpa_s);
#endif /* CONFIG_FILS */

#if defined ( CONFIG_RIC_ELEMENT )
	wpabuf_free(wpa_s->ric_ies);
	wpa_s->ric_ies = NULL;
#endif // CONFIG_RIC_ELEMENT

#ifdef CONFIG_DPP
	wpas_dpp_deinit(wpa_s);
#endif /* CONFIG_DPP */
}


/**
 * wpa_clear_keys - Clear keys configured for the driver
 * @wpa_s: Pointer to wpa_supplicant data
 * @addr: Previously used BSSID or %NULL if not available
 *
 * This function clears the encryption keys that has been previously configured
 * for the driver.
 */
void wpa_clear_keys(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	int i, max;

	TX_FUNC_START("        ");

#ifdef CONFIG_IEEE80211W
	max = 6;
#else /* CONFIG_IEEE80211W */
	max = 4;
#endif /* CONFIG_IEEE80211W */

	/* MLME-DELETEKEYS.request */
	for (i = 0; i < max; i++) {
		if (wpa_s->keys_cleared & BIT(i))
			continue;
		wpa_drv_set_key(wpa_s, WPA_ALG_NONE, NULL, i, 0, NULL, 0,
				NULL, 0);
	}
	if (!(wpa_s->keys_cleared & BIT(0)) && addr &&
	    !is_zero_ether_addr(addr)) {
		wpa_drv_set_key(wpa_s, WPA_ALG_NONE, addr, 0, 0, NULL, 0, NULL,
				0);
		/* MLME-SETPROTECTION.request(None) */
		wpa_drv_mlme_setprotection(
			wpa_s, addr,
			MLME_SETPROTECTION_PROTECT_TYPE_NONE,
			MLME_SETPROTECTION_KEY_TYPE_PAIRWISE);
	}
	wpa_s->keys_cleared = (u32) -1;

	TX_FUNC_END("        ");
}


/**
 * wpa_supplicant_state_txt - Get the connection state name as a text string
 * @state: State (wpa_state; WPA_*)
 * Returns: The state name as a printable text string
 */
const char * wpa_supplicant_state_txt(enum wpa_states state)
{
	switch (state) {
		case WPA_DISCONNECTED:
			return "DISCONNECTED";
		case WPA_INACTIVE:
			return "INACTIVE";
		case WPA_INTERFACE_DISABLED:
			return "INTERFACE_DISABLED";
		case WPA_SCANNING:
			return "SCANNING";
		case WPA_AUTHENTICATING:
			return "AUTHENTICATING";
		case WPA_ASSOCIATING:
			return "ASSOCIATING";
		case WPA_ASSOCIATED:
			return "ASSOCIATED";
		case WPA_4WAY_HANDSHAKE:
			return "4WAY_HANDSHAKE";
		case WPA_GROUP_HANDSHAKE:
			return "GROUP_HANDSHAKE";
		case WPA_COMPLETED:
			return "COMPLETED";
		default:
			return "UNKNOWN";
	}
}


#ifdef CONFIG_BGSCAN

static void wpa_supplicant_start_bgscan(struct wpa_supplicant *wpa_s)
{
	const char *name;

	if (wpa_s->current_ssid && wpa_s->current_ssid->bgscan)
		name = wpa_s->current_ssid->bgscan;
	else
		name = wpa_s->conf->bgscan;
	if (name == NULL || name[0] == '\0')
		return;
	if (wpas_driver_bss_selection(wpa_s))
		return;
	if (wpa_s->current_ssid == wpa_s->bgscan_ssid)
		return;
#ifdef CONFIG_P2P
	if (wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE)
		return;
#endif /* CONFIG_P2P */

	bgscan_deinit(wpa_s);
	if (wpa_s->current_ssid) {
		if (bgscan_init(wpa_s, wpa_s->current_ssid, name)) {
			wpa_dbg(wpa_s, MSG_DEBUG, "Failed to initialize "
				"bgscan");
			/*
			 * Live without bgscan; it is only used as a roaming
			 * optimization, so the initial connection is not
			 * affected.
			 */
		} else {
			struct wpa_scan_results *scan_res;
			wpa_s->bgscan_ssid = wpa_s->current_ssid;
			scan_res = wpa_supplicant_get_scan_results(wpa_s, NULL,
								   0);
			if (scan_res) {
				bgscan_notify_scan(wpa_s, scan_res);
				wpa_scan_results_free(scan_res);
			}
		}
	} else
		wpa_s->bgscan_ssid = NULL;
}


static void wpa_supplicant_stop_bgscan(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->bgscan_ssid != NULL) {
		bgscan_deinit(wpa_s);
		wpa_s->bgscan_ssid = NULL;
	}
}

#endif /* CONFIG_BGSCAN */

#ifdef CONFIG_AUTOSCAN
static void wpa_supplicant_start_autoscan(struct wpa_supplicant *wpa_s)
{
	if (autoscan_init(wpa_s, 0))
		wpa_dbg(wpa_s, MSG_DEBUG, "Failed to initialize autoscan");
}


static void wpa_supplicant_stop_autoscan(struct wpa_supplicant *wpa_s)
{
	autoscan_deinit(wpa_s);
}


void wpa_supplicant_reinit_autoscan(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->wpa_state == WPA_DISCONNECTED ||
	    wpa_s->wpa_state == WPA_SCANNING) {
		autoscan_deinit(wpa_s);
		wpa_supplicant_start_autoscan(wpa_s);
	}
}
#endif /* CONFIG_AUTOSCAN */

extern QueueHandle_t wifi_monitor_event_q;
#ifndef CONFIG_MONITOR_THREAD_EVENT_CHANGE
extern EventGroupHandle_t wifi_monitor_event_group;
#endif // CONFIG_MONITOR_THREAD_EVENT_CHANGE

void wpa_supplicant_reconfig_net(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	int status;
	
	wifi_monitor_msg_buf_t *wifi_monitor_msg = NULL;

	if (os_strcmp(wpa_s->ifname, SOFTAP_DEVICE_NAME) == 0) {
		da16x_notice_prt("[%s] Not Support(%s) !!!\n", __func__, wpa_s->ifname);
		return;
	}

	wifi_monitor_msg = os_malloc(sizeof(wifi_monitor_msg_buf_t));

	if (wifi_monitor_msg == NULL) {
		da16x_err_prt("[%s] Malloc fail(%s) !!!\n",
					__func__, wpa_s->ifname);	
		return;
	}

#ifdef	CONFIG_P2P
	if (os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
		/* p2p */
		da16x_assoc_prt(">>> [%s] P2P_NET_GO\n", __func__);
		if (os_memcmp(wpa_s->own_addr, wpa_s->bssid, ETH_ALEN) == 0) {
			da16x_assoc_prt(">>> [%s] P2P_NET_GO\n", __func__);
			os_memcpy(wifi_monitor_msg->data, "0", 1);
		} else {
			da16x_assoc_prt(">>> [%s] P2P_NET_CLIENT\n", __func__);
			os_memcpy(wifi_monitor_msg->data, "1", 1);
		}
	} else
#endif	/* CONFIG_P2P */
	if (os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) {
		/* Station */
		da16x_assoc_prt(">>> [%s] INF_NET_STATION\n", __func__);
		os_memcpy(wifi_monitor_msg->data, "3", 1);
	}
#ifdef CONFIG_MESH
	else if (os_strcmp(wpa_s->ifname, MESH_POINT_DEVICE_NAME) == 0
		     && get_run_mode() == MESH_POINT_MODE) {
		/* Station */
		da16x_assoc_prt(">>> [%s] MESH_POINT\n", __func__);
		os_memcpy(wifi_monitor_msg->data, "4", 1);
	}
#endif /* CONFIG_MESH */
	else
	{
		da16x_notice_prt("[%s] Unknown mode(%s) !!!\n", __func__, wpa_s->ifname);
		vPortFree(wifi_monitor_msg);
		return;
	}

	wifi_monitor_msg->cmd = WIFI_CMD_SEND_DHCP_RECONFIG;

	/* Send  Message */
#ifdef CONFIG_MONITOR_THREAD_EVENT_CHANGE
	status = xQueueSend(wifi_monitor_event_q, &wifi_monitor_msg, portNO_DELAY);
#else
	status = xQueueSend(wifi_monitor_event_q, wifi_monitor_msg, portNO_DELAY);
#endif // CONFIG_MONITOR_THREAD_EVENT_CHANGE

	if (status != pdTRUE) {
		da16x_err_prt("[%s] queue send error(%s) !!! (%d)\n",
					__func__, wpa_s->ifname, status);
		vPortFree(wifi_monitor_msg);
		return;
	}

#ifndef CONFIG_MONITOR_THREAD_EVENT_CHANGE
	status = xEventGroupSetBits(wifi_monitor_event_group, WIFI_EVENT_SEND_MSG_TO_APP);

	if ((status & WIFI_EVENT_SEND_MSG_TO_APP) == pdFALSE) {
		da16x_err_prt("[%s] event set error(%s) !!! (%d)\n", __func__, wpa_s->ifname,  status);
		vPortFree(wifi_monitor_msg);
		return;
	}

	vPortFree(wifi_monitor_msg);
#endif // CONFIG_MONITOR_THREAD_EVENT_CHANGE
}

/**
 * wpa_supplicant_set_state - Set current connection state
 * @wpa_s: Pointer to wpa_supplicant data
 * @state: The new connection state
 *
 * This function is called whenever the connection state changes, e.g.,
 * association is completed for WPA/WPA2 4-Way Handshake is started.
 */
extern void fc80211_dpm_info_save(u8 *bssid , unsigned int assoc_freq);

#ifdef	CONFIG_RECONNECT_OPTIMIZE
extern u8 fast_reconnect_scan;
#endif

void wpa_supplicant_set_state(struct wpa_supplicant *wpa_s, enum wpa_states state)
{
	enum wpa_states old_state = wpa_s->wpa_state;

	wpa_dbg(wpa_s, MSG_DEBUG, "State: %s (%d) -> %s (%d)",
		wpa_supplicant_state_txt(wpa_s->wpa_state), wpa_s->wpa_state,
		wpa_supplicant_state_txt(state), state);

	if (state == WPA_INTERFACE_DISABLED) {
		/* Assure normal scan when interface is restored */
		wpa_s->normal_scans = 0;
	}

	if (state == WPA_COMPLETED) {
		wpas_connect_work_done(wpa_s);
		/* Reinitialize normal_scan counter */
		wpa_s->normal_scans = 0;
#ifdef	CONFIG_RECONNECT_OPTIMIZE /* by Shingu 20170717 (Scan Optimization) */
		wpa_s->reassoc_try = 0;
		fast_reconnect_scan = 0;
#endif	/* CONFIG_RECONNECT_OPTIMIZE */
	}

#if defined ( __SUPP_27_SUPPORT__ )
	if (state != WPA_SCANNING)
		wpa_supplicant_notify_scanning(wpa_s, 0);
#endif // __SUPP_27_SUPPORT__

	if (state == WPA_COMPLETED && wpa_s->new_connection) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
#if 1 // FC9000 Only
		char	do_connect_prt = FALSE;

#ifdef CONFIG_RECONNECT_OPTIMIZE
		/** In the DPM wakeup & Reconnect , it happened that "assoc freq is zero" */
		if(chk_dpm_wakeup() && wpa_s->assoc_freq == 0) {
			wpa_s->assoc_freq = wpa_s->reassoc_freq;
			//da16x_notice_prt(GRN_COL "\nAssoc freq %d for Dpm info saving\n", wpa_s->assoc_freq);
		}
#endif
		/* For umac channel setting */
		fc80211_dpm_info_save(wpa_s->bssid ,wpa_s->assoc_freq);

#ifdef __SUPPORT_CON_EASY__
		/* set the Connection try status as Connection(3) */
		ce_set_conn_try_result_status(3);
#endif	/* __SUPPORT_CON_EASY__ */

		if (chk_dpm_mode() == pdTRUE) {
			if (is_zero_ether_addr(dpm_supp_conn_info->bssid)) 
				do_connect_prt = TRUE;
		} else {
			do_connect_prt = TRUE;
		}

		if (do_connect_prt) {
			if (!(wpa_s->conf->ssid->key_mgmt == WPA_KEY_MGMT_NONE
					&& !(wpa_s->conf->ssid->auth_alg == WPA_AUTH_ALG_OPEN))) /* Not WEP Auth Open */
			{
				wpa_s->disconnect_reason = 0;
			}

			if (wpa_s->current_ssid->mode == WPAS_MODE_AP) {
				da16x_notice_prt(GRN_COL "\nSoft-AP is Ready ("
					MACSTR ")" CLR_COL "\n", MAC2STR(wpa_s->bssid));

				ap_ctrl_iface_sta_disassociate(wpa_s, "FF:FF:FF:FF:FF:FF reason=3");
				//ap_ctrl_iface_sta_deauthenticate(wpa_s, "FF:FF:FF:FF:FF:FF reason=3");
			} else 	if (wpa_s->current_ssid->mode == WPAS_MODE_P2P_GO) {
				da16x_notice_prt(GRN_COL "\nWiFi Direct GO Fixed is Ready ("
					MACSTR ")" CLR_COL "\n", MAC2STR(wpa_s->bssid));

				ap_ctrl_iface_sta_disassociate(wpa_s, "FF:FF:FF:FF:FF:FF reason=3");
				//ap_ctrl_iface_sta_deauthenticate(wpa_s, "FF:FF:FF:FF:FF:FF reason=3");
			} else	if (wpa_s->current_ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION) {
				da16x_notice_prt(GRN_COL "\nConnection COMPLETE(WiFI Direct Go Mode)"
					CLR_COL "\n");
#ifdef CONFIG_MESH
			} else	if (wpa_s->current_ssid->mode == WPAS_MODE_MESH) {
				da16x_notice_prt(GRN_COL "\nMesh Point is Ready" CLR_COL "\n");
#endif /* CONFIG_MESH */
			} else { /* Station or WiFI Direct P2P GC */
#ifdef __BOOT_CONN_TIME_PRINT__
				log_boot_conn_with_run_time("WiFi connection");
#endif			
				da16x_notice_prt(GRN_COL "\nConnection COMPLETE to "
					MACSTR CLR_COL "\n", MAC2STR(wpa_s->bssid));

#ifdef FAST_CONN_ASSOC_CH
				extern int i3ed11_freq_to_ch(int freq);
				if ((wpa_s->conf->scan_cur_freq != 0 || fast_connection_sleep_flag == pdTRUE)
					&& wpa_s->conf->scan_cur_freq != i3ed11_freq_to_ch(wpa_s->assoc_freq))
				{
					da16x_notice_prt("Fast_Connection ASSOC_CH=%d --> %d\n", 
							wpa_s->conf->scan_cur_freq, i3ed11_freq_to_ch(wpa_s->assoc_freq));

					write_nvram_int(NVR_KEY_ASSOC_CH, i3ed11_freq_to_ch(wpa_s->assoc_freq));

				}
#endif // FAST_CONN_ASSOC_CH

#ifdef AUTO_WEPKEY_INDEX
				if (wpa_s->conf->ssid->key_mgmt == WPA_KEY_MGMT_NONE
					&& wpa_s->conf->ssid->wep_key_len[0]) {
						wpa_supp_ctrl_iface_save_config(wpa_s);
						//PRINTF("wepkeyindex=%d\n", wpa_s->conf->ssid->wep_tx_keyidx);
				}
#endif /* AUTO_WEPKEY_INDEX */

				/* Notify Wi-Fi connection status on user callback function */
				if (wifi_conn_notify_cb != NULL) {
					wifi_conn_notify_cb();
				}

				/* Notify Wi-Fi connection status on AT-CMD UART1 */
				atcmd_asynchony_event(0, "1");

#ifdef CONFIG_MESH_PORTAL
				/** In Mesh Portal Mode, Station is associated and channel is different from MP,
				 *	Do change the Mesh Point Channel as Station association channel.
				 */
				if (get_run_mode() == STA_MESH_PORTAL_MODE) {
					extern int fc80211_mesh_switch_channel(int , int);
					
					da16x_notice_prt("[%s] MESH Frequency Change --> assoc: %d , cur_bss: %d cur_ssid: %d \n",
								__func__ , wpa_s->assoc_freq, wpa_s->current_bss->freq, wpa_s->current_ssid->frequency );

					if(!wpa_s->assoc_freq)
					{
						wpa_s->assoc_freq = wpa_s->current_bss->freq;
					}
					fc80211_mesh_switch_channel(1, wpa_s->assoc_freq);
				}
#endif /* CONFIG_MESH_PORTAL */

			}
		}
#else
		int fils_hlp_sent = 0;

#ifdef CONFIG_SME
		if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
		    wpa_auth_alg_fils(wpa_s->sme.auth_alg))
			fils_hlp_sent = 1;
#endif /* CONFIG_SME */
		if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
		    wpa_auth_alg_fils(wpa_s->auth_alg))
			fils_hlp_sent = 1;

#if defined(CONFIG_CTRL_IFACE) || !defined(CONFIG_NO_STDOUT_DEBUG)
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_CONNECTED "- Connection to "
			MACSTR " completed [id=%d id_str=%s%s]",
			MAC2STR(wpa_s->bssid),
			ssid ? ssid->id : -1,
			ssid && ssid->id_str ? ssid->id_str : "",
			fils_hlp_sent ? " FILS_HLP_SENT" : "");
#else
		da16x_info_prt("[%s] " WPA_EVENT_CONNECTED "- Connection to "
			MACSTR " completed [id=%d id_str=%s%s]\n",
			__func__,
			MAC2STR(wpa_s->bssid),
			ssid ? ssid->id : -1,
			ssid && ssid->id_str ? ssid->id_str : "",
			fils_hlp_sent ? " FILS_HLP_SENT" : "");
#endif	/* __CONFIG_DBG_LOG_MSG__ */
#endif /* CONFIG_CTRL_IFACE || !CONFIG_NO_STDOUT_DEBUG */
#endif /* FC9000 Only */
		wpas_clear_temp_disabled(wpa_s, ssid, 1);
		wpa_blacklist_clear(wpa_s);
		wpa_s->extra_blacklist_count = 0;
		wpa_s->new_connection = 0;
		wpa_drv_set_operstate(wpa_s, 1);
#ifdef CONFIG_SIMPLE_ROAMING
		if (wpa_s->conf->roam_state >= 1) {
			wpa_blacklist_clear(wpa_s);
			wpa_s->blacklist_cleared++;
		}
#endif /* CONFIG_SIMPLE_ROAMING */

#ifndef IEEE8021X_EAPOL
		wpa_drv_set_supp_port(wpa_s, 1);
#endif /* IEEE8021X_EAPOL */

#ifdef	CONFIG_WPS
		wpa_s->after_wps = 0;
		wpa_s->known_wps_freq = 0;
#endif	/* CONFIG_WPS */
#ifdef CONFIG_P2P
		wpas_p2p_completed(wpa_s);
#endif /* CONFIG_P2P */

#if 1 /* Dialog Only */
		/* by Bright :
		 *	For DPM : just need to reconfig when state is changed...
		 */
		if (state != old_state) {
			if (chk_dpm_mode() == pdTRUE) {
				if (is_zero_ether_addr(dpm_supp_conn_info->bssid))
				{
					da16x_eloop_register_timeout(0, 0, wpa_supplicant_reconfig_net, wpa_s, NULL); /* 0 ms */
				}
			} else if (os_strcmp(wpa_s->ifname, SOFTAP_DEVICE_NAME) != 0) {
#ifdef CONFIG_MESH_PORTAL
				if (!(get_run_mode() == STA_MESH_PORTAL_MODE && os_strcmp(wpa_s->ifname, MESH_POINT_DEVICE_NAME) == 0)
					|| (get_run_mode() != STA_MESH_PORTAL_MODE))
#endif /* CONFIG_MESH_PORTAL */
				{
					da16x_eloop_register_timeout(0, 0, wpa_supplicant_reconfig_net, wpa_s, NULL); /* 0 ms */
				}
			}
		}
#else
		sme_sched_obss_scan(wpa_s, 1);

#if defined(CONFIG_FILS) && defined(IEEE8021X_EAPOL)
		if (!fils_hlp_sent && ssid && ssid->eap.erp)
			wpas_update_fils_connect_params(wpa_s);
#endif /* CONFIG_FILS && IEEE8021X_EAPOL */
#endif /* Dialog Only */ 
	} else if (state == WPA_DISCONNECTED || state == WPA_ASSOCIATING ||
		   state == WPA_ASSOCIATED) {
		wpa_s->new_connection = 1;
		wpa_drv_set_operstate(wpa_s, 0);
#ifndef IEEE8021X_EAPOL
		wpa_drv_set_supp_port(wpa_s, 0);
#endif /* IEEE8021X_EAPOL */
#if 0	/* by Bright : Merge 2.7 */
		sme_sched_obss_scan(wpa_s, 0);
#endif	/* 0 */
	}
	wpa_s->wpa_state = state;

#ifdef CONFIG_BGSCAN
	if (state == WPA_COMPLETED)
		wpa_supplicant_start_bgscan(wpa_s);
	else if (state < WPA_ASSOCIATED)
		wpa_supplicant_stop_bgscan(wpa_s);
#endif /* CONFIG_BGSCAN */

#ifdef CONFIG_AUTOSCAN
	if (state == WPA_AUTHENTICATING)
		wpa_supplicant_stop_autoscan(wpa_s);
#endif /* CONFIG_AUTOSCAN */

#if defined(CONFIG_AP) && !defined(ENABLE_SCAN_ON_AP_MODE)
	if (!wpa_s->ap_iface) {
#endif	/* CONFIG_AP && !ENABLE_SCAN_ON_AP_MODE */
#ifdef CONFIG_AUTOSCAN
		if (state == WPA_DISCONNECTED || state == WPA_INACTIVE)
			wpa_supplicant_start_autoscan(wpa_s);
#endif /* CONFIG_AUTOSCAN */
#if defined(CONFIG_AP) && !defined(ENABLE_SCAN_ON_AP_MODE)
	}
#endif	/* CONFIG_AP && !ENABLE_SCAN_ON_AP_MODE */

#if 0	/* by Bright : Merge 2.7 */
	if (old_state >= WPA_ASSOCIATED && wpa_s->wpa_state < WPA_ASSOCIATED)
		wmm_ac_notify_disassoc(wpa_s);
#endif	/* 0 */

	if (wpa_s->wpa_state != old_state) {
#if 0 // debug
				PRINTF("[%s] set_state: old (%d) != new (%d) \n", __func__,
					old_state, wpa_s->wpa_state);
#endif 
#if 0	/* by Bright : Merge 2.7 */
		wpas_notify_state_changed(wpa_s, wpa_s->wpa_state, old_state);
#endif	/* 0 */

#ifdef	CONFIG_P2P_UNUSED_CMD
		/*
		 * Notify the P2P Device interface about a state change in one
		 * of the interfaces.
		 */
		wpas_p2p_indicate_state_change(wpa_s);
#endif	/* CONFIG_P2P_UNUSED_CMD */

		if (wpa_s->wpa_state == WPA_COMPLETED ||
		    old_state == WPA_COMPLETED) {
			if (wpa_s->wpa_state == WPA_COMPLETED) {
				wpa_supp_dpm_backup_conn_info(wpa_s);
			}
#if 0	/* by Bright : Merge 2.7 */
			wpas_notify_auth_changed(wpa_s);
#endif	/* 0 */
		}
	}
	save_supp_conn_state(wpa_s->wpa_state);
}


#ifdef __SUPP_27_SUPPORT__
void wpa_supplicant_terminate_proc(struct wpa_global *global)
{
	int pending = 0;

#ifdef CONFIG_WPS
	struct wpa_supplicant *wpa_s = global->ifaces;
	while (wpa_s) {
		struct wpa_supplicant *next = wpa_s->next;
		if (wpas_wps_terminate_pending(wpa_s) == 1)
			pending = 1;
#ifdef CONFIG_P2P
		if (wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE ||
		    (wpa_s->current_ssid && wpa_s->current_ssid->p2p_group))
			wpas_p2p_disconnect(wpa_s);
#endif /* CONFIG_P2P */
		wpa_s = next;
	}
#endif /* CONFIG_WPS */

	if (pending)
		return;
#if 0
	da16x_eloop_terminate();
#endif /* 0 */
}


static void wpa_supplicant_terminate(int sig, void *signal_ctx)
{
	struct wpa_global *global = signal_ctx;
	wpa_supplicant_terminate_proc(global);
}
#endif /* __SUPP_27_SUPPORT__ */


void wpa_supplicant_clear_status(struct wpa_supplicant *wpa_s)
{
#ifdef	CONFIG_P2P
	enum wpa_states old_state = wpa_s->wpa_state;
#endif	/* CONFIG_P2P */

	wpa_s->pairwise_cipher = 0;
	wpa_s->group_cipher = 0;
	wpa_s->mgmt_group_cipher = 0;
	wpa_s->key_mgmt = 0;
	if (wpa_s->wpa_state != WPA_INTERFACE_DISABLED && chk_dpm_wakeup() == 0)
		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);

#ifdef CONFIG_P2P
        if (wpa_s->wpa_state != old_state) {
		if (wpa_s->wpa_state == WPA_COMPLETED)
			wpas_p2p_notif_connected(wpa_s);
		else if (old_state >= WPA_ASSOCIATED && wpa_s->wpa_state < WPA_ASSOCIATED)
			wpas_p2p_notif_disconnected(wpa_s);
	}
#endif /* CONFIG_P2P */

}


/**
 * wpa_supp_reload_config- Reload configuration data
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 on success or -1 if configuration parsing failed
 *
 * This function can be used to request that the configuration data is reloaded
 * (e.g., after configuration file change). This function is reloading
 * configuration only for one interface, so this may need to be called multiple
 * times if %wpa_supplicant is controlling multiple interfaces and all
 * interfaces need reconfiguration.
 */
int wpa_supp_reload_config(struct wpa_supplicant *wpa_s)
{
	struct wpa_config *conf = NULL;
	// int reconf_ctrl;
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	int old_ap_scan;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	TX_FUNC_START("");

#if 1	/* by Shingu 20160928 (WPA_CLI Optimize) */
	if (os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) != 0)
		return -1;
#endif	/* 1 */

	if (chk_dpm_wakeup() == pdFALSE) {
#if 0	/* by Shingu 20160901 (Concurrent) */
		conf = wpa_config_read(NULL);
#else
#if defined ( __SUPP_27_SUPPORT__ )
		conf = wpa_config_read(wpa_s->confname, wpa_s->ifname);
#else
		conf = wpa_config_read(wpa_s->ifname);
#endif // __SUPP_27_SUPPORT__
#endif	/* 0 */
		if (conf == NULL) {
			da16x_err_prt("[%s] Failed to parse the config "
				"NVRAM - exiting\n", __func__);
			return -1;
		}
	}

#ifdef	IEEE8021X_EAPOL
	eapol_sm_invalidate_cached_session(wpa_s->eapol);
#endif	/* IEEE8021X_EAPOL */
	if (wpa_s->current_ssid) {
		if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
			wpa_s->own_disconnect_req = 1;
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);
	}

#ifdef	IEEE8021X_EAPOL
	/*
	 * TODO: should notify EAPOL SM about changes in opensc_engine_path,
	 * pkcs11_engine_path, pkcs11_module_path, openssl_ciphers.
	 */
	if (wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt) ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_OWE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_DPP) {
		/*
		 * Clear forced success to clear EAP state for next
		 * authentication.
		 */
		eapol_sm_notify_eap_success(wpa_s->eapol, FALSE);
	}
	eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
#endif	/* IEEE8021X_EAPOL */

	wpa_sm_set_config(wpa_s->wpa, NULL);
#ifdef	CONFIG_PMKSA
	wpa_sm_pmksa_cache_flush(wpa_s->wpa, NULL);
#endif	/* CONFIG_PMKSA */
#ifdef	CONFIG_FAST_REAUTH
	wpa_sm_set_fast_reauth(wpa_s->wpa, wpa_s->conf->fast_reauth);
#endif	/* CONFIG_FAST_REAUTH */
#ifdef	CONFIG_PRE_AUTH
	rsn_preauth_deinit(wpa_s->wpa);
#endif	/* CONFIG_PRE_AUTH */

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	old_ap_scan = wpa_s->conf->ap_scan;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	wpa_config_free(wpa_s->conf);
	wpa_s->conf = conf;
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (old_ap_scan != wpa_s->conf->ap_scan)
		wpas_notify_ap_scan_changed(wpa_s);
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

#if 0 /* FC9000 */
	if (reconf_ctrl)
		wpa_s->ctrl_iface = wpa_supplicant_ctrl_iface_init(wpa_s);
#endif /* FC9000 */

	wpa_supplicant_update_config(wpa_s);

	if (chk_dpm_wakeup() == pdFALSE) {
		wpa_config_free(wpa_s->conf);
		wpa_s->conf = conf;
	}
	wpa_supplicant_clear_status(wpa_s);

	if (chk_dpm_wakeup() == pdFALSE) {
		if (wpa_supplicant_enabled_networks(wpa_s)) {
			wpa_s->reassociate = 1;
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
			wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		}
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Reconfiguration completed");
	TX_FUNC_END("");

	return 0;
}


/* static */ void wpa_supplicant_reconfig(int sig, void *signal_ctx)
{
	struct wpa_global *global = signal_ctx;
	struct wpa_supplicant *wpa_s;
	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Signal %d received - reconfiguring",
			sig);
		if (wpa_supp_reload_config(wpa_s) < 0) {
			wpa_supplicant_terminate_proc(global);
		}
	}

#if 0
	if (wpa_debug_reopen_file() < 0) {
		/* Ignore errors since we cannot really do much to fix this */
		da16x_notice_prt("Could not reopen debug log file");
	}
#endif /* 0 */
}


static int wpa_supplicant_suites_from_ai(struct wpa_supplicant *wpa_s,
					 struct wpa_ssid *ssid,
					 struct wpa_ie_data *ie)
{
	int ret = wpa_sm_parse_own_wpa_ie(wpa_s->wpa, ie);
	if (ret) {
		if (ret == -2) {
			da16x_err_prt("[%s] Failed to parse WPA IE from assoc info\n", __func__);
		}
		return -1;
	}

	da16x_assoc_prt("[%s] Using WPA IE from AssocReq to set cipher suites\n", __func__);
	if (!(ie->group_cipher & ssid->group_cipher)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Driver used disabled group "
			"cipher 0x%x (mask 0x%x) - reject",
			ie->group_cipher, ssid->group_cipher);
#else
		 da16x_err_prt("[%s] WPA: Driver used disabled group "
            "cipher 0x%x (mask 0x%x) - reject\n",
            __func__,
            ie->group_cipher, ssid->group_cipher);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
	if (!(ie->pairwise_cipher & ssid->pairwise_cipher)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Driver used disabled pairwise "
			"cipher 0x%x (mask 0x%x) - reject",
			ie->pairwise_cipher, ssid->pairwise_cipher);
#else
        da16x_err_prt("[%s] WPA: Driver used disabled pairwise "
            "cipher 0x%x (mask 0x%x) - reject\n",
            __func__,
            ie->pairwise_cipher, ssid->pairwise_cipher);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
	if (!(ie->key_mgmt & ssid->key_mgmt)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Driver used disabled key "
			"management 0x%x (mask 0x%x) - reject",
			ie->key_mgmt, ssid->key_mgmt);
#else
        da16x_err_prt("[%s] WPA: Driver used disabled key "
            "management 0x%x (mask 0x%x) - reject\n",
            __func__, ie->key_mgmt, ssid->key_mgmt);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

#ifdef CONFIG_IEEE80211W
	if (!(ie->capabilities & WPA_CAPABILITY_MFPC) &&
	    wpas_get_ssid_pmf(wpa_s, ssid) == MGMT_FRAME_PROTECTION_REQUIRED) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Driver assoc with an AP "
			"that does not support mgmt frame protection - "
			"reject");
#else
        da16x_info_prt("[%s] WPA: Driver assoc with an AP "
            "that does not support mgmt frame protection - "
            "reject\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
#endif /* CONFIG_IEEE80211W */

	return 0;
}


/**
 * wpa_supplicant_set_suites - Set authentication and encryption parameters
 * @wpa_s: Pointer to wpa_supplicant data
 * @bss: Scan results for the selected BSS, or %NULL if not available
 * @ssid: Configuration data for the selected network
 * @wpa_ie: Buffer for the WPA/RSN IE
 * @wpa_ie_len: Maximum wpa_ie buffer size on input. This is changed to be the
 * used buffer length in case the functions returns success.
 * Returns: 0 on success or -1 on failure
 *
 * This function is used to configure authentication and encryption parameters
 * based on the network configuration and scan result for the selected BSS (if
 * available).
 */
int wpa_supplicant_set_suites(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss, struct wpa_ssid *ssid,
			      u8 *wpa_ie, size_t *wpa_ie_len)
{
	struct wpa_ie_data ie;
	int sel, proto;
	const u8 *bss_wpa, *bss_rsn;
#if defined CONFIG_OWE || defined CONFIG_HS20
	const u8 *bss_osen = NULL;
#endif /* CONFIG_OWE || CONFIG_HS20 */

	TX_FUNC_START("");

	if (bss) {
		bss_wpa = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
		bss_rsn = wpa_bss_get_ie(bss, WLAN_EID_RSN);
#ifdef CONFIG_HS20
		bss_osen = wpa_bss_get_vendor_ie(bss, OSEN_IE_VENDOR_TYPE);
#endif /* CONFIG_HS20 */
	} else {
#ifdef CONFIG_HS20
		bss_wpa = bss_rsn = bss_osen = NULL;
#else
		bss_wpa = bss_rsn = NULL;
#endif /* CONFIG_HS20 */
	}

	if (bss_rsn && (ssid->proto & WPA_PROTO_RSN) &&
	    wpa_parse_wpa_ie(bss_rsn, 2 + bss_rsn[1], &ie) == 0 &&
	    (ie.group_cipher & ssid->group_cipher) &&
	    (ie.pairwise_cipher & ssid->pairwise_cipher) &&
	    (ie.key_mgmt & ssid->key_mgmt)) {
		da16x_wpa_prt("[%s] RSN: using IEEE 802.11i/D9.0\n", __func__);
		proto = WPA_PROTO_RSN;
	} else if (bss_wpa && (ssid->proto & WPA_PROTO_WPA) &&
		   wpa_parse_wpa_ie(bss_wpa, 2 + bss_wpa[1], &ie) == 0 &&
		   (ie.group_cipher & ssid->group_cipher) &&
		   (ie.pairwise_cipher & ssid->pairwise_cipher) &&
		   (ie.key_mgmt & ssid->key_mgmt)) {
		da16x_wpa_prt("[%s] WPA: using IEEE 802.11i/D3.0\n", __func__);
		proto = WPA_PROTO_WPA;
#ifdef CONFIG_HS20
	} else if (bss_osen && (ssid->proto & WPA_PROTO_OSEN)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "HS 2.0: using OSEN");
		/* TODO: parse OSEN element */
		os_memset(&ie, 0, sizeof(ie));
		ie.group_cipher = WPA_CIPHER_CCMP;
		ie.pairwise_cipher = WPA_CIPHER_CCMP;
		ie.key_mgmt = WPA_KEY_MGMT_OSEN;
		proto = WPA_PROTO_OSEN;
	} else if (bss_rsn && (ssid->proto & WPA_PROTO_OSEN) &&
	    wpa_parse_wpa_ie(bss_rsn, 2 + bss_rsn[1], &ie) == 0 &&
	    (ie.group_cipher & ssid->group_cipher) &&
	    (ie.pairwise_cipher & ssid->pairwise_cipher) &&
	    (ie.key_mgmt & ssid->key_mgmt)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "RSN: using OSEN (within RSN)");
		proto = WPA_PROTO_RSN;
#endif /* CONFIG_HS20 */
	} else if (bss) {
		da16x_wpa_prt("[%s] WPA: Failed to select WPA/RSN\n", __func__);

#if 0	/* by Bright : Merge 2.7 */
		wpa_dbg(wpa_s, MSG_DEBUG,
			"WPA: ssid proto=0x%x pairwise_cipher=0x%x group_cipher=0x%x key_mgmt=0x%x",
			ssid->proto, ssid->pairwise_cipher, ssid->group_cipher,
			ssid->key_mgmt);
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: BSS " MACSTR " ssid='%s'%s%s%s",
			MAC2STR(bss->bssid),
			wpa_ssid_txt(bss->ssid, bss->ssid_len),
			bss_wpa ? " WPA" : "",
			bss_rsn ? " RSN" : "",
			bss_osen ? " OSEN" : "");
		if (bss_rsn) {
			wpa_hexdump(MSG_DEBUG, "RSN", bss_rsn, 2 + bss_rsn[1]);
			if (wpa_parse_wpa_ie(bss_rsn, 2 + bss_rsn[1], &ie)) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"Could not parse RSN element");
			} else {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"RSN: pairwise_cipher=0x%x group_cipher=0x%x key_mgmt=0x%x",
					ie.pairwise_cipher, ie.group_cipher,
					ie.key_mgmt);
			}
		}
		if (bss_wpa) {
			wpa_hexdump(MSG_DEBUG, "WPA", bss_wpa, 2 + bss_wpa[1]);
			if (wpa_parse_wpa_ie(bss_wpa, 2 + bss_wpa[1], &ie)) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"Could not parse WPA element");
			} else {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"WPA: pairwise_cipher=0x%x group_cipher=0x%x key_mgmt=0x%x",
					ie.pairwise_cipher, ie.group_cipher,
					ie.key_mgmt);
			}
		}
#endif	/* 0 */
		return -1;
	} else {
		if (ssid->proto & WPA_PROTO_OSEN)
			proto = WPA_PROTO_OSEN;
		else if (ssid->proto & WPA_PROTO_RSN)
			proto = WPA_PROTO_RSN;
		else
			proto = WPA_PROTO_WPA;

		if (wpa_supplicant_suites_from_ai(wpa_s, ssid, &ie) < 0) {
			os_memset(&ie, 0, sizeof(ie));
			ie.group_cipher = ssid->group_cipher;
			ie.pairwise_cipher = ssid->pairwise_cipher;
			ie.key_mgmt = ssid->key_mgmt;
#ifdef CONFIG_IEEE80211W
			ie.mgmt_group_cipher = 0;
			if (ssid->ieee80211w != NO_MGMT_FRAME_PROTECTION) {
				if (ssid->group_mgmt_cipher &
				    WPA_CIPHER_BIP_GMAC_256)
					ie.mgmt_group_cipher =
						WPA_CIPHER_BIP_GMAC_256;
				else if (ssid->group_mgmt_cipher &
					 WPA_CIPHER_BIP_CMAC_256)
					ie.mgmt_group_cipher =
						WPA_CIPHER_BIP_CMAC_256;
				else if (ssid->group_mgmt_cipher &
					 WPA_CIPHER_BIP_GMAC_128)
					ie.mgmt_group_cipher =
						WPA_CIPHER_BIP_GMAC_128;
				else
					ie.mgmt_group_cipher =
						WPA_CIPHER_AES_128_CMAC;
			}
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_OWE
			if ((ssid->key_mgmt & WPA_KEY_MGMT_OWE) &&
			    !ssid->owe_only &&
			    !bss_wpa && !bss_rsn && !bss_osen) {
				wpa_supplicant_set_non_wpa_policy(wpa_s, ssid);
				wpa_s->wpa_proto = 0;
				*wpa_ie_len = 0;
				return 0;
			}
#endif /* CONFIG_OWE */
			da16x_wpa_prt("[%s] WPA: Set cipher suites "
					"based on configuration\n", __func__);
		} else
			proto = ie.proto;
	}

	da16x_wpa_prt("[%s] WPA: Selected cipher suites: group %d "
		"pairwise %d key_mgmt %d proto %d\n",
		__func__,
		ie.group_cipher, ie.pairwise_cipher, ie.key_mgmt, proto);

#ifdef CONFIG_IEEE80211W
	if (ssid->ieee80211w) {
		da16x_wpa_prt("[%s] WPA: Selected mgmt group cipher %d\n",
				__func__, ie.mgmt_group_cipher);
	}
#endif /* CONFIG_IEEE80211W */

	wpa_s->wpa_proto = proto;
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_PROTO, proto);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_RSN_ENABLED,
			 !!(ssid->proto & (WPA_PROTO_RSN | WPA_PROTO_OSEN)));

	if (bss || !wpa_s->ap_ies_from_associnfo) {
		if (wpa_sm_set_ap_wpa_ie(wpa_s->wpa, bss_wpa,
					 bss_wpa ? 2 + bss_wpa[1] : 0) ||
		    wpa_sm_set_ap_rsn_ie(wpa_s->wpa, bss_rsn,
					 bss_rsn ? 2 + bss_rsn[1] : 0))
			return -1;
	}

	sel = ie.group_cipher & ssid->group_cipher;
	wpa_s->group_cipher = wpa_pick_group_cipher(sel);
	if (wpa_s->group_cipher < 0) {
		da16x_wpa_prt("[%s] Failed to select group cipher\n", __func__);
		return -1;
	}
	da16x_wpa_prt("[%s] using GTK %s\n",
			__func__, wpa_cipher_txt(wpa_s->group_cipher));

	sel = ie.pairwise_cipher & ssid->pairwise_cipher;
	wpa_s->pairwise_cipher = wpa_pick_pairwise_cipher(sel, 1);
	if (wpa_s->pairwise_cipher < 0) {
		da16x_wpa_prt("[%s] Failed to select pairwise cipher\n", __func__);
		return -1;
	}
	da16x_wpa_prt("[%s] using PTK %s\n",
                        __func__, wpa_cipher_txt(wpa_s->pairwise_cipher));

	sel = ie.key_mgmt & ssid->key_mgmt;
#ifdef CONFIG_SAE
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SAE))
		sel &= ~(WPA_KEY_MGMT_SAE | WPA_KEY_MGMT_FT_SAE);
#endif /* CONFIG_SAE */

	if (0) {
#ifdef CONFIG_SUITEB192
	} else if (sel & WPA_KEY_MGMT_IEEE8021X_SUITE_B_192) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_IEEE8021X_SUITE_B_192;
		da16x_wpa_prt("[%s] using KEY_MGMT 802.1X with Suite B (192-bit)\n", __func__);
#endif /* CONFIG_SUITEB192 */
#ifdef CONFIG_SUITEB
	} else if (sel & WPA_KEY_MGMT_IEEE8021X_SUITE_B) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_IEEE8021X_SUITE_B;
		da16x_wpa_prt("[%s] using KEY_MGMT 802.1X with Suite B\n", __func__);
#endif /* CONFIG_SUITEB */
#ifdef CONFIG_FILS
#ifdef CONFIG_IEEE80211R
	} else if (sel & WPA_KEY_MGMT_FT_FILS_SHA384) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_FILS_SHA384;
		da16x_wpa_prt("[%s] using KEY_MGMT FT-FILS-SHA384\n", __func__);
	} else if (sel & WPA_KEY_MGMT_FT_FILS_SHA256) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_FILS_SHA256;
		da16x_wpa_prt("[%s] using KEY_MGMT FT-FILS-SHA256\n", __func__);
#endif /* CONFIG_IEEE80211R */
	} else if (sel & WPA_KEY_MGMT_FILS_SHA384) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FILS_SHA384;
		da16x_wpa_prt("[%s] using KEY_MGMT FILS-SHA384\n", __func__);
	} else if (sel & WPA_KEY_MGMT_FILS_SHA256) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FILS_SHA256;
		da16x_wpa_prt("[%s] using KEY_MGMT FILS-SHA256\n", __func__);
#endif /* CONFIG_FILS */
#ifdef CONFIG_IEEE80211R
#ifdef CONFIG_SHA384
	} else if (sel & WPA_KEY_MGMT_FT_IEEE8021X_SHA384) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_IEEE8021X_SHA384;
		da16x_wpa_prt("[%s] using KEY_MGMT FT/802.1X-SHA384\n", __func__);
		if (pmksa_cache_get_current(wpa_s->wpa)) {
			/* PMKSA caching with FT is not fully functional, so
			 * disable the case for now. */
			da16x_wpa_prt("[%s] Disable PMKSA caching for FT/802.1X connection\n", __func__);
			pmksa_cache_clear_current(wpa_s->wpa);
		}
#endif /* CONFIG_SHA384 */
	} else if (sel & WPA_KEY_MGMT_FT_IEEE8021X) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_IEEE8021X;
		da16x_wpa_prt("[%s] using KEY_MGMT FT/802.1X\n", __func__);
		if (pmksa_cache_get_current(wpa_s->wpa)) {
			/* PMKSA caching with FT is not fully functional, so
			 * disable the case for now. */
			da16x_wpa_prt("[%s] Disable PMKSA caching for FT/802.1X connection\n", __func__);
			pmksa_cache_clear_current(wpa_s->wpa);
		}
	} else if (sel & WPA_KEY_MGMT_FT_PSK) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_PSK;
		da16x_wpa_prt("[%s] using KEY_MGMT FT/PSK\n", __func__);
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_SAE
	} else if (sel & WPA_KEY_MGMT_SAE) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_SAE;
		da16x_wpa_prt("[%s] RSN: using KEY_MGMT SAE\n", __func__);
	} else if (sel & WPA_KEY_MGMT_FT_SAE) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_FT_SAE;
		da16x_wpa_prt("[%s] RSN: using KEY_MGMT FT/SAE\n", __func__);
#endif /* CONFIG_SAE */
#ifdef CONFIG_IEEE80211W
	} else if (sel & WPA_KEY_MGMT_IEEE8021X_SHA256) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_IEEE8021X_SHA256;
		da16x_wpa_prt("[%s] using KEY_MGMT 802.1X with SHA256\n", __func__);
	} else if (sel & WPA_KEY_MGMT_PSK_SHA256) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
		da16x_wpa_prt("[%s] using KEY_MGMT PSK with SHA256\n", __func__);
#endif /* CONFIG_IEEE80211W */
	} else if (sel & WPA_KEY_MGMT_IEEE8021X) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_IEEE8021X;
		da16x_wpa_prt("[%s] using KEY_MGMT 802.1X\n", __func__);
	} else if (sel & WPA_KEY_MGMT_PSK) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_PSK;
		da16x_wpa_prt("[%s] using KEY_MGMT WPA-PSK\n", __func__);
	} else if (sel & WPA_KEY_MGMT_WPA_NONE) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_WPA_NONE;
		da16x_wpa_prt("[%s] using KEY_MGMT WPA-NONE\n", __func__);
#ifdef CONFIG_HS20
	} else if (sel & WPA_KEY_MGMT_OSEN) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_OSEN;
		da16x_wpa_prt("[%s] HS 2.0: using KEY_MGMT OSEN\n", __func__);
#endif /* CONFIG_HS20 */
#ifdef CONFIG_OWE
	} else if (sel & WPA_KEY_MGMT_OWE) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_OWE;
		da16x_wpa_prt("[%s] RSN: using KEY_MGMT OWE\n", __func__);
#endif /* CONFIG_OWE */
#ifdef CONFIG_DPP
	} else if (sel & WPA_KEY_MGMT_DPP) {
		wpa_s->key_mgmt = WPA_KEY_MGMT_DPP;
		da16x_wpa_prt("[%s] RSN: using KEY_MGMT DPP\n", __func__);
#endif /* CONFIG_DPP */
	} else {
		da16x_wpa_prt("[%s] Failed to select authenticated key management type\n", __func__);
		return -1;
	}

	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_KEY_MGMT, wpa_s->key_mgmt);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_PAIRWISE,
			 wpa_s->pairwise_cipher);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_GROUP, wpa_s->group_cipher);

#ifdef CONFIG_IEEE80211W
	sel = ie.mgmt_group_cipher;
	if (ssid->group_mgmt_cipher)
		sel &= ssid->group_mgmt_cipher;
	if (wpas_get_ssid_pmf(wpa_s, ssid) == NO_MGMT_FRAME_PROTECTION ||
	    !(ie.capabilities & WPA_CAPABILITY_MFPC))
		sel = 0;
	if (sel & WPA_CIPHER_AES_128_CMAC) {
		wpa_s->mgmt_group_cipher = WPA_CIPHER_AES_128_CMAC;
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: using MGMT group cipher "
			"AES-128-CMAC");
	} else if (sel & WPA_CIPHER_BIP_GMAC_128) {
		wpa_s->mgmt_group_cipher = WPA_CIPHER_BIP_GMAC_128;
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: using MGMT group cipher "
			"BIP-GMAC-128");
	} else if (sel & WPA_CIPHER_BIP_GMAC_256) {
		wpa_s->mgmt_group_cipher = WPA_CIPHER_BIP_GMAC_256;
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: using MGMT group cipher "
			"BIP-GMAC-256");
	} else if (sel & WPA_CIPHER_BIP_CMAC_256) {
		wpa_s->mgmt_group_cipher = WPA_CIPHER_BIP_CMAC_256;
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: using MGMT group cipher "
			"BIP-CMAC-256");
	} else {
		wpa_s->mgmt_group_cipher = 0;
		wpa_dbg(wpa_s, MSG_DEBUG, "WPA: not using MGMT group cipher");
	}
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_KEY_MGMT, wpa_s->key_mgmt);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_MGMT_GROUP,
			 wpa_s->mgmt_group_cipher);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_MFP,
			 wpas_get_ssid_pmf(wpa_s, ssid));
#endif /* CONFIG_IEEE80211W */

	if (wpa_sm_set_assoc_wpa_ie_default(wpa_s->wpa, wpa_ie, wpa_ie_len)) {
		da16x_wpa_prt("[%s] WPA: Failed to generate WPA IE\n", __func__);
		return -1;
	}

	if (wpa_key_mgmt_wpa_psk(ssid->key_mgmt)) {
		int psk_set = 0;
#ifdef CONFIG_SAE
		int sae_only;

		sae_only = (ssid->key_mgmt & (WPA_KEY_MGMT_PSK |
					      WPA_KEY_MGMT_FT_PSK |
					      WPA_KEY_MGMT_PSK_SHA256)) == 0;

#endif /* CONFIG_SAE */

		if (ssid->psk_set
#ifdef CONFIG_SAE
		&& !sae_only
#endif /* CONFIG_SAE */
		) {
			wpa_hexdump_key(MSG_MSGDUMP, "PSK (set in config)",
					ssid->psk, PMK_LEN);
			wpa_sm_set_pmk(wpa_s->wpa, ssid->psk, PMK_LEN, NULL,
				       NULL);
			psk_set = 1;
		}

#ifdef CONFIG_SAE
		if (wpa_key_mgmt_sae(ssid->key_mgmt) &&
		    (ssid->sae_password || ssid->passphrase))
			psk_set = 1;
#endif /* CONFIG_SAE */

#ifndef CONFIG_NO_PBKDF2
		if (bss && ssid->bssid_set && ssid->ssid_len == 0 &&
		    ssid->passphrase
#ifdef CONFIG_SAE
		    && !sae_only
#endif /* CONFIG_SAE */
		    )
		{
			u8 psk[PMK_LEN];
#ifdef OPTIMIZE_PBKDF2
			optimized_pbkdf2_sha1(ssid->passphrase, bss->ssid, bss->ssid_len,
				    4096, psk, PMK_LEN);
#else
		        pbkdf2_sha1(ssid->passphrase, bss->ssid, bss->ssid_len,
				    4096, psk, PMK_LEN);
#endif /* OPTIMIZE_PBKDF2 */
		        wpa_hexdump_key(MSG_MSGDUMP, "PSK (from passphrase)",
					psk, PMK_LEN);
			wpa_sm_set_pmk(wpa_s->wpa, psk, PMK_LEN, NULL, NULL);
			psk_set = 1;
			os_memset(psk, 0, sizeof(psk));
		}
#endif /* CONFIG_NO_PBKDF2 */
#ifdef CONFIG_EXT_PASSWORD
		if (ssid->ext_psk && !sae_only) {
			struct wpabuf *pw = ext_password_get(wpa_s->ext_pw,
							     ssid->ext_psk);
			char pw_str[64 + 1];
			u8 psk[PMK_LEN];

			if (pw == NULL) {
#ifdef	__CONFIG_DBG_LOG_MSG__
				wpa_msg(wpa_s, MSG_INFO, "EXT PW: No PSK "
					"found from external storage");
#else
                da16x_err_prt("[%s] EXT PW: No PSK found from "
                    "external storage\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
				return -1;
			}

			if (wpabuf_len(pw) < 8 || wpabuf_len(pw) > 64) {
#ifdef	__CONFIG_DBG_LOG_MSG__
				wpa_msg(wpa_s, MSG_INFO, "EXT PW: Unexpected "
					"PSK length %d in external storage",
					(int) wpabuf_len(pw));
#else
                da16x_wpa_prt("[%s] EXT PW: Unexpected PSK "
                    "length %d in external storage\n",
                    __func__,
                    (int) wpabuf_len(pw));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
				ext_password_free(pw);
				return -1;
			}

			os_memcpy(pw_str, wpabuf_head(pw), wpabuf_len(pw));
			pw_str[wpabuf_len(pw)] = '\0';

#ifndef CONFIG_NO_PBKDF2
			if (wpabuf_len(pw) >= 8 && wpabuf_len(pw) < 64 && bss)
			{
#ifdef OPTIMIZE_PBKDF2
				optimized_pbkdf2_sha1(pw_str, bss->ssid, bss->ssid_len,
					    4096, psk, PMK_LEN);
#else
				pbkdf2_sha1(pw_str, bss->ssid, bss->ssid_len,
					    4096, psk, PMK_LEN);
#endif /* OPTIMIZE_PBKDF2 */
				os_memset(pw_str, 0, sizeof(pw_str));
#ifdef	__CONFIG_DBG_LOG_MSG__
				wpa_hexdump_key(MSG_MSGDUMP, "PSK (from "
						"external passphrase)",
						psk, PMK_LEN);
#else
                da16x_dump("PSK (from external passphrase)", psk, PMK_LEN);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
				wpa_sm_set_pmk(wpa_s->wpa, psk, PMK_LEN, NULL,
					       NULL);
				psk_set = 1;
				os_memset(psk, 0, sizeof(psk));
			} else
#endif /* CONFIG_NO_PBKDF2 */
			if (wpabuf_len(pw) == 2 * PMK_LEN) {
				if (hexstr2bin(pw_str, psk, PMK_LEN) < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
					wpa_msg(wpa_s, MSG_INFO, "EXT PW: "
						"Invalid PSK hex string");
#else
                    da16x_wpa_prt("[%s] EXT PW: Invalid PSK "
                        "hex string\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
					os_memset(pw_str, 0, sizeof(pw_str));
					ext_password_free(pw);
					return -1;
				}
#ifdef	__CONFIG_DBG_LOG_MSG__
				wpa_hexdump_key(MSG_MSGDUMP,
						"PSK (from external PSK)",
						psk, PMK_LEN);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
				wpa_sm_set_pmk(wpa_s->wpa, psk, PMK_LEN, NULL,
					       NULL);
				psk_set = 1;
				os_memset(psk, 0, sizeof(psk));
			} else {
#ifdef	__CONFIG_DBG_LOG_MSG__
				wpa_msg(wpa_s, MSG_INFO, "EXT PW: No suitable "
					"PSK available");
#else
                da16x_wpa_prt("[%s] EXT PW: No suitable "
                    "PSK available\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
				os_memset(pw_str, 0, sizeof(pw_str));
				ext_password_free(pw);
				return -1;
			}

			os_memset(pw_str, 0, sizeof(pw_str));
			ext_password_free(pw);
		}
#endif /* CONFIG_EXT_PASSWORD */

		if (!psk_set) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_INFO,
				"No PSK available for association");
#else
			da16x_wpa_prt("[%s] No PSK available for association\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			wpas_auth_failed(wpa_s, "NO_PSK_AVAILABLE");
			return -1;
		}
#ifdef CONFIG_OWE
	} else if (wpa_s->key_mgmt == WPA_KEY_MGMT_OWE) {
		/* OWE Diffie-Hellman exchange in (Re)Association
		 * Request/Response frames set the PMK, so do not override it
		 * here. */
#endif /* CONFIG_OWE */
	} else
		wpa_sm_set_pmk_from_pmksa(wpa_s->wpa);

	TX_FUNC_END("");

	return 0;
}


static void wpas_ext_capab_byte(struct wpa_supplicant *wpa_s, u8 *pos, int idx)
{
	*pos = 0x00;

	switch (idx) {
	case 0: /* Bits 0-7 */
		break;
	case 1: /* Bits 8-15 */
		break;
	case 2: /* Bits 16-23 */
#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_SLEEP_MODE
		*pos |= 0x02; /* Bit 17 - WNM-Sleep Mode */
#endif /* CONFIG_WNM_SLEEP_MODE */
#ifdef CONFIG_WNM_BSS_TRANS_MGMT
		*pos |= 0x08; /* Bit 19 - BSS Transition */
#endif /* CONFIG_WNM_BSS_TRANS_MGMT */
#endif /* CONFIG_WNM */
		break;
	case 3: /* Bits 24-31 */
#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_SSID_LIST
		*pos |= 0x02; /* Bit 25 - SSID List */
#endif /* CONFIG_WNM_SSID_LIST */
#endif /* CONFIG_WNM */
#ifdef CONFIG_INTERWORKING
		if (wpa_s->conf->interworking)
			*pos |= 0x80; /* Bit 31 - Interworking */
#endif /* CONFIG_INTERWORKING */
		break;
	case 4: /* Bits 32-39 */
#ifdef CONFIG_INTERWORKING
		if (wpa_s->drv_flags / WPA_DRIVER_FLAGS_QOS_MAPPING)
			*pos |= 0x01; /* Bit 32 - QoS Map */
#endif /* CONFIG_INTERWORKING */
		break;
	case 5: /* Bits 40-47 */
#ifdef CONFIG_HS20
		if (wpa_s->conf->hs20)
			*pos |= 0x40; /* Bit 46 - WNM-Notification */
#endif /* CONFIG_HS20 */
#ifdef CONFIG_MBO
		*pos |= 0x40; /* Bit 46 - WNM-Notification */
#endif /* CONFIG_MBO */
		break;
	case 6: /* Bits 48-55 */
		break;
	case 7: /* Bits 56-63 */
		break;
	case 8: /* Bits 64-71 */
		if (wpa_s->conf->ftm_responder)
			*pos |= 0x40; /* Bit 70 - FTM responder */
		if (wpa_s->conf->ftm_initiator)
			*pos |= 0x80; /* Bit 71 - FTM initiator */
		break;
	case 9: /* Bits 72-79 */
#ifdef CONFIG_FILS
		if (!wpa_s->disable_fils)
			*pos |= 0x01;
#endif /* CONFIG_FILS */
		break;
	}
}


int wpas_build_ext_capab(struct wpa_supplicant *wpa_s, u8 *buf, size_t buflen)
{
	u8 *pos = buf;
	u8 len = 10, i;

	if (len < wpa_s->extended_capa_len)
		len = wpa_s->extended_capa_len;
	if (buflen < (size_t) len + 2) {
		da16x_notice_prt("Not enough room for building ext_capa_element");
		return -1;
	}

	*pos++ = WLAN_EID_EXT_CAPAB;
	*pos++ = len;
	for (i = 0; i < len; i++, pos++) {
		wpas_ext_capab_byte(wpa_s, pos, i);

		if (i < wpa_s->extended_capa_len) {
			*pos &= ~wpa_s->extended_capa_mask[i];
			*pos |= wpa_s->extended_capa[i];
		}
	}

	while (len > 0 && buf[1 + len] == 0) {
		len--;
		buf[1] = len;
	}
	if (len == 0)
		return 0;

	return 2 + len;
}


static int wpas_valid_bss(struct wpa_supplicant *wpa_s,
			  struct wpa_bss *test_bss)
{
	struct wpa_bss *bss;

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		if (bss == test_bss)
		{
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
			supp_rehold_bss(bss->internal_bss);
#endif				
			return 1;
	}
	}

	return 0;
}


static int wpas_valid_ssid(struct wpa_supplicant *wpa_s,
			   struct wpa_ssid *test_ssid)
{
	struct wpa_ssid *ssid;

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (ssid == test_ssid)
			return 1;
	}

	return 0;
}


int wpas_valid_bss_ssid(struct wpa_supplicant *wpa_s, struct wpa_bss *test_bss,
			struct wpa_ssid *test_ssid)
{
	if (test_bss && !wpas_valid_bss(wpa_s, test_bss))
		return 0;

	return test_ssid == NULL || wpas_valid_ssid(wpa_s, test_ssid);
}


void wpas_connect_work_free(struct wpa_connect_work *cwork)
{
#if 0
	if (cwork == NULL)
		return;
	os_free(cwork);
#endif /* 0 */
}


void wpas_connect_work_done(struct wpa_supplicant *wpa_s)
{
#if 0
	struct wpa_connect_work *cwork;
	struct wpa_radio_work *work = wpa_s->connect_work;

	if (!work)
		return;

	wpa_s->connect_work = NULL;
	cwork = work->ctx;
	work->ctx = NULL;
	wpas_connect_work_free(cwork);
	radio_work_done(work);
#endif /* 0 */
}


#ifdef CONFIG_RANDOM_ADDR
int wpas_update_random_addr(struct wpa_supplicant *wpa_s, int style)
{
#if 0
	struct os_reltime now;
	u8 addr[ETH_ALEN];

	os_get_reltime(&now);
	if (wpa_s->last_mac_addr_style == style &&
	    wpa_s->last_mac_addr_change.sec != 0 &&
	    !os_reltime_expired(&now, &wpa_s->last_mac_addr_change,
				wpa_s->conf->rand_addr_lifetime)) {
		wpa_msg(wpa_s, MSG_DEBUG,
			"Previously selected random MAC address has not yet expired");
		return 0;
	}

	switch (style) {
	case 1:
		if (random_mac_addr(addr) < 0)
			return -1;
		break;
	case 2:
		os_memcpy(addr, wpa_s->perm_addr, ETH_ALEN);
		if (random_mac_addr_keep_oui(addr) < 0)
			return -1;
		break;
	default:
		return -1;
	}

	if (wpa_drv_set_mac_addr(wpa_s, addr) < 0) {
		wpa_msg(wpa_s, MSG_INFO,
			"Failed to set random MAC address");
		return -1;
	}

	os_get_reltime(&wpa_s->last_mac_addr_change);
	wpa_s->mac_addr_changed = 1;
	wpa_s->last_mac_addr_style = style;

	if (wpa_supp_update_mac_addr(wpa_s) < 0) {
		wpa_msg(wpa_s, MSG_INFO,
			"Could not update MAC address information");
		return -1;
	}

	wpa_msg(wpa_s, MSG_DEBUG, "Using random MAC address " MACSTR,
		MAC2STR(addr));
#endif /* 0 */

	return 0;
}


int wpas_update_random_addr_disassoc(struct wpa_supplicant *wpa_s)
{
#if 1
	return NULL;
#else
	if (wpa_s->wpa_state >= WPA_AUTHENTICATING ||
	    !wpa_s->conf->preassoc_mac_addr)
		return 0;

	return wpas_update_random_addr(wpa_s, wpa_s->conf->preassoc_mac_addr);
#endif /* 1 */
}
#endif /* CONFIG_RANDOM_ADDR */

#if defined ( CONFIG_ASSOC_CB )
static void wpas_start_assoc_cb(struct wpa_radio_work *work, int deinit);
#endif // CONFIG_ASSOC_CB


/**
 * wpa_supplicant_associate - Request association
 * @wpa_s: Pointer to wpa_supplicant data
 * @bss: Scan results for the selected BSS, or %NULL if not available
 * @ssid: Configuration data for the selected network
 *
 * This function is used to request %wpa_supplicant to associate with a BSS.
 */
void wpa_supplicant_associate(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss, struct wpa_ssid *ssid)
{
#ifdef CONFIG_ASSOC_CB
	struct wpa_connect_work *cwork;
#endif /* CONFIG_ASSOC_CB */
#ifdef CONFIG_RANDOM_ADDR
	int rand_style;
#endif /* CONFIG_RANDOM_ADDR */

	extern void da16x_sme_authenticate(struct wpa_supplicant *wpa_s,
					  struct wpa_bss *bss,
					  struct wpa_ssid *ssid);

	TX_FUNC_START("  ");

	wpa_s->own_disconnect_req = 0;

	/*
	 * If we are starting a new connection, any previously pending EAPOL
	 * RX cannot be valid anymore.
	 */
	wpabuf_free(wpa_s->pending_eapol_rx);
	wpa_s->pending_eapol_rx = NULL;

#ifdef CONFIG_RANDOM_ADDR
	if (ssid->mac_addr == -1)
		rand_style = wpa_s->conf->mac_addr;
	else
		rand_style = ssid->mac_addr;
#endif /* CONFIG_RANDOM_ADDR */

#if 0	/* by Bright : Merge 2.7 */
	wmm_ac_clear_saved_tspecs(wpa_s);
#endif	/* 0 */
	wpa_s->reassoc_same_bss = 0;
	wpa_s->reassoc_same_ess = 0;
#ifdef CONFIG_TESTING_OPTIONS
	wpa_s->testing_resend_assoc = 0;
#endif /* CONFIG_TESTING_OPTIONS */

	if (wpa_s->last_ssid == ssid) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Re-association to the same ESS");
		wpa_s->reassoc_same_ess = 1;
		if (wpa_s->current_bss && wpa_s->current_bss == bss) {
#if 0	/* by Bright : Merge 2.7 */
			wmm_ac_save_tspecs(wpa_s);
#endif	/* 0 */
			wpa_s->reassoc_same_bss = 1;
		}
	}

#ifdef CONFIG_RANDOM_ADDR
	if (rand_style > 0 && !wpa_s->reassoc_same_ess) {
		if (wpas_update_random_addr(wpa_s, rand_style) < 0)
			return;
#ifdef	CONFIG_PMKSA
		wpa_sm_pmksa_cache_flush(wpa_s->wpa, ssid);
#endif	/* CONFIG_PMKSA */
	} else if (rand_style == 0 && wpa_s->mac_addr_changed) {
		if (wpa_drv_set_mac_addr(wpa_s, NULL) < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_INFO,
				"Could not restore permanent MAC address");
#else
			da16x_info_prt("[%s] Could not restore permanent MAC_addr\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return;
		}
		wpa_s->mac_addr_changed = 0;
		if (wpa_supp_update_mac_addr(wpa_s) < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_INFO,
				"Could not update MAC address information");
#else
			da16x_info_prt("[%s] Could not update MAC_addr information\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return;
		}
		wpa_dbg(wpa_s, MSG_DEBUG, "Using permanent MAC_addr");
	}
#endif /* CONFIG_RANDOM_ADDR */
	wpa_s->last_ssid = ssid;

#ifdef CONFIG_IBSS_RSN
	ibss_rsn_deinit(wpa_s->ibss_rsn);
	wpa_s->ibss_rsn = NULL;
#else /* CONFIG_IBSS_RSN */
	if (ssid->mode == WPAS_MODE_IBSS &&
	    !(ssid->key_mgmt & (WPA_KEY_MGMT_NONE | WPA_KEY_MGMT_WPA_NONE))) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO,
			"IBSS RSN not supported in the build");
#else
		da16x_info_prt("[%s] IBSS RSN not supported in the build\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return;
	}
#endif /* CONFIG_IBSS_RSN */

	if (ssid->mode == WPAS_MODE_AP
#ifdef	CONFIG_P2P
		|| ssid->mode == WPAS_MODE_P2P_GO
		|| ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION
#endif	/* CONFIG_P2P */
		) {
#ifdef CONFIG_AP
		if (ssid->mode == WPAS_MODE_AP) {
			da16x_assoc_prt("  [%s] AP mode already started\n",
						__func__);
			return;
		}

		if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_AP)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_INFO, "Driver does not support AP "
				"mode");
#else
			da16x_assoc_prt("[%s] Driver does not support AP mode\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return;
		}
		if (wpa_supplicant_create_ap(wpa_s, ssid) < 0) {
			wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
#ifdef CONFIG_P2P
			if (ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION)
				wpas_p2p_ap_setup_failed(wpa_s);
#endif	/* CONFIG_P2P */
			return;
		}
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = bss;
#else /* CONFIG_AP */
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "AP mode support not included in "
			"the build");
#else
		da16x_err_prt("[%s] AP mode support not included in the build\n", __func_)-);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
#endif /* CONFIG_AP */
		return;
	}

#ifdef CONFIG_MESH
	if (ssid->mode == WPAS_MODE_MESH) {
		if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_MESH)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_INFO,
				"Driver does not support mesh mode");
#else
			da16x_info_prt("[%s] Driver does not support mesh mode\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return;
		}

		if (bss)
			ssid->frequency = bss->freq;

		if (wpa_supplicant_join_mesh(wpa_s, ssid) < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_ERROR, "Could not join mesh");
#else
			da16x_err_prt("[%s] Could not join mesh\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return;
		}
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = bss;
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, MESH_GROUP_STARTED "ssid=\"%s\" id=%d",
			wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
			ssid->id);
#else
		da16x_info_prt("[%s] " MESH_GROUP_STARTED "ssid=\"%s\" id=%d\n",
			__func__,
			wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
			ssid->id);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		//wpas_notify_mesh_group_started(wpa_s, ssid);
		return;
	}
#endif /* CONFIG_MESH */

#ifdef	CONFIG_CONCURRENT /* by Shingu 20160901 (Concurrent) */
	if (get_run_mode() == STA_SOFT_AP_CONCURRENT_MODE) {
#ifdef	CONFIG_AP
		struct wpa_supplicant *ap_wpa_s = wpa_s->global->ifaces;
		char cmd[32];

		if (bss->freq != ap_wpa_s->current_ssid->frequency) {
			sprintf(cmd, "%d", bss->freq);
			ap_ctrl_iface_ap(ap_wpa_s, cmd, NULL, NULL);
		}
#endif	/* CONFIG_AP */
	}
#ifdef	CONFIG_P2P
	else if (get_run_mode() == STA_P2P_CONCURRENT_MODE) {
		struct wpa_supplicant *p2p_wpa_s = wpa_s->global->ifaces;
		int chan = bss->freq == 2484 ? 14 : (bss->freq - 2407) / 5;
		int changed = 0;

		if ((p2p_wpa_s->ap_iface &&	/* P2P GO */
		     (bss->freq != p2p_wpa_s->current_ssid->frequency)) ||
		    (!p2p_wpa_s->ap_iface &&	/* P2P GC */
		     bss->freq != p2p_wpa_s->current_bss->freq)) {
			  wpas_p2p_disconnect(wpa_s->global->ifaces);
			  changed = 1;
		}

		if (chan != 1 && chan != 6 && chan != 11) {
			if (p2p_wpa_s->global->p2p) {
				da16x_notice_prt("STA channel(%d) is not social "
						"channel(1,6,11). Now P2P "
						"interface is terminated.\n",
						chan);
				wpas_p2p_deinit_global(p2p_wpa_s->global);
			}
		} else if (changed || !p2p_wpa_s->global->p2p) {
			if (p2p_wpa_s->conf->p2p_oper_channel != chan ||
			    p2p_wpa_s->conf->p2p_listen_channel != chan) {
				da16x_notice_prt("Setting P2P channels to %d\n",
						chan);
				p2p_wpa_s->conf->p2p_oper_channel = chan;
				p2p_wpa_s->conf->p2p_listen_channel = chan;
			}

			if (!p2p_wpa_s->global->p2p) {
				da16x_notice_prt("Now P2P interface is "
						"reactivated.\n");
				wpas_p2p_init(p2p_wpa_s->global, p2p_wpa_s);
			}

			p2p_set_oper_channel(p2p_wpa_s->global->p2p, 81, chan, 1);
			p2p_set_listen_channel(p2p_wpa_s->global->p2p, 81, chan);
		}
	}
#endif	/* CONFIG_P2P */
#endif	/* CONFIG_CONCURRENT */

	/*
	 * Set WPA state machine configuration to match the selected network now
	 * so that the information is available before wpas_start_assoc_cb()
	 * gets called. This is needed at least for RSN pre-authentication where
	 * candidate APs are added to a list based on scan result processing
	 * before completion of the first association.
	 */
	wpa_supplicant_rsn_supp_set_config(wpa_s, ssid);

#ifdef CONFIG_DPP
	if (wpas_dpp_check_connect(wpa_s, ssid, bss) != 0)
		return;
#endif /* CONFIG_DPP */

#ifdef CONFIG_TDLS
	if (bss)
		wpa_tdls_ap_ies(wpa_s->wpa, (const u8 *) (bss + 1),
				bss->ie_len);
#endif /* CONFIG_TDLS */

	if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
	    ssid->mode == WPAS_MODE_INFRA) {
		da16x_sme_authenticate(wpa_s, bss, ssid);
		return;
	}

#ifdef CONFIG_ASSOC_CB
	if (wpa_s->connect_work) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_dbg(wpa_s, MSG_DEBUG, "Reject wpa_supplicant_associate() call since connect_work exist");
#else
		da16x_wpa_prt("[%s] Reject wpa_supplicant_associate() call since connect_work exist\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return;
	}
#endif /* CONFIG_ASSOC_CB */

#if defined ( __SUPP_27_SUPPORT__ )
	if (radio_work_pending(wpa_s, "connect")) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_dbg(wpa_s, MSG_DEBUG, "Reject wpa_supplicant_associate() call since pending work exist");
#else
		da16x_wpa_prt("[%s] Reject wpa_supplicant_associate() call since pending work exist\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return;
	}
#endif // __SUPP_27_SUPPORT__

#ifdef CONFIG_SME
	if (ssid->mode == WPAS_MODE_IBSS || ssid->mode == WPAS_MODE_MESH) {
		/* Clear possibly set auth_alg, if any, from last attempt. */
		wpa_s->sme.auth_alg = WPA_AUTH_ALG_OPEN;
	}
#endif /* CONFIG_SME */

#if defined ( CONFIG_ASSOC_CB )
	wpas_abort_ongoing_scan(wpa_s);

	cwork = os_zalloc(sizeof(*cwork));
	if (cwork == NULL)
		return;

	cwork->bss = bss;
	cwork->ssid = ssid;

	if (radio_add_work(wpa_s, bss ? bss->freq : 0, "connect", 1,
			   wpas_start_assoc_cb, cwork) < 0) {
		os_free(cwork);
	}
#endif // CONFIG_ASSOC_CB
}


#ifdef CONFIG_MESH
static int bss_is_ibss(struct wpa_bss *bss)
{
	return (bss->caps & (IEEE80211_CAP_ESS | IEEE80211_CAP_IBSS)) ==
		IEEE80211_CAP_IBSS;
}
#endif /* CONFIG_MESH */


#ifdef CONFIG_IEEE80211AC
static int drv_supports_vht(struct wpa_supplicant *wpa_s,
			    const struct wpa_ssid *ssid)
{
	enum hostapd_hw_mode hw_mode;
	struct hostapd_hw_modes *mode = NULL;
	u8 channel;
	int i;

	hw_mode = ieee80211_freq_to_chan(ssid->frequency, &channel);
	if (hw_mode == NUM_HOSTAPD_MODES)
		return 0;
	for (i = 0; wpa_s->hw.modes && i < wpa_s->hw.num_modes; i++) {
		if (wpa_s->hw.modes[i].mode == hw_mode) {
			mode = &wpa_s->hw.modes[i];
			break;
		}
	}

	if (!mode)
		return 0;

	return mode->vht_capab != 0;
}
#endif /* CONFIG_IEEE80211AC */


#ifdef CONFIG_MESH
void ibss_mesh_setup_freq(struct wpa_supplicant *wpa_s,
			  const struct wpa_ssid *ssid,
			  struct hostapd_freq_params *freq)
{
	enum hostapd_hw_mode hw_mode;
	struct hostapd_hw_modes *mode = NULL;
	int ht40plus[] = { 36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 157,
			   184, 192 };
	int vht80[] = { 36, 52, 100, 116, 132, 149 };
	struct hostapd_channel_data *pri_chan = NULL, *sec_chan = NULL;
	u8 channel;
	int i, chan_idx, ht40 = -1, res, obss_scan = 1;
	unsigned int j, k;
	struct hostapd_freq_params vht_freq;
	int chwidth, seg0, seg1;
	u32 vht_caps = 0;

	freq->freq = ssid->frequency;

	for (j = 0; j < wpa_s->last_scan_res_used; j++) {
		struct wpa_bss *bss = wpa_s->last_scan_res[j];

		if (ssid->mode != WPAS_MODE_IBSS)
			break;

		/* Don't adjust control freq in case of fixed_freq */
		if (ssid->fixed_freq)
			break;

		if (!bss_is_ibss(bss))
			continue;

		if (ssid->ssid_len == bss->ssid_len &&
		    os_memcmp(ssid->ssid, bss->ssid, bss->ssid_len) == 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_printf_dbg(MSG_DEBUG,
				   "IBSS already found in scan results, adjust control freq: %d",
				   bss->freq);
#else
			da16x_debug_prt("[%s] IBSS already found in scan results, adjust control freq: %d\n",
					__func__, bss->freq);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			freq->freq = bss->freq;
			obss_scan = 0;
			break;
		}
	}

	/* For IBSS check HT_IBSS flag */
	if (ssid->mode == WPAS_MODE_IBSS &&
	    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_HT_IBSS))
		return;

	if (wpa_s->group_cipher == WPA_CIPHER_WEP40 ||
	    wpa_s->group_cipher == WPA_CIPHER_WEP104 ||
	    wpa_s->pairwise_cipher == WPA_CIPHER_TKIP) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_printf_dbg(MSG_DEBUG,
			   "IBSS: WEP/TKIP detected, do not try to enable HT");
#else
		da16x_debug_prt("[%s] IBSS: WEP/TKIP detected, do not try to enable HT\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return;
	}

	hw_mode = ieee80211_freq_to_chan(freq->freq, &channel);
	for (i = 0; wpa_s->hw.modes && i < wpa_s->hw.num_modes; i++) {
		if (wpa_s->hw.modes[i].mode == hw_mode) {
			mode = &wpa_s->hw.modes[i];
			break;
		}
	}

	if (!mode)
		return;

#ifdef CONFIG_HT_OVERRIDES
	if (ssid->disable_ht) {
		freq->ht_enabled = 0;
		return;
	}
#endif /* CONFIG_HT_OVERRIDES */

	freq->ht_enabled = ht_supported(mode);
	if (!freq->ht_enabled)
		return;

	/* Setup higher BW only for 5 GHz */
	if (mode->mode != HOSTAPD_MODE_IEEE80211A)
		return;

	for (chan_idx = 0; chan_idx < mode->num_channels; chan_idx++) {
		pri_chan = &mode->channels[chan_idx];
		if (pri_chan->chan == channel)
			break;
		pri_chan = NULL;
	}
	if (!pri_chan)
		return;

	/* Check primary channel flags */
	if (pri_chan->flag & (HOSTAPD_CHAN_DISABLED | HOSTAPD_CHAN_NO_IR))
		return;

#ifdef CONFIG_HT_OVERRIDES
	if (ssid->disable_ht40)
		return;
#endif /* CONFIG_HT_OVERRIDES */

	/* Check/setup HT40+/HT40- */
	for (j = 0; j < ARRAY_SIZE(ht40plus); j++) {
		if (ht40plus[j] == channel) {
			ht40 = 1;
			break;
		}
	}

	/* Find secondary channel */
	for (i = 0; i < mode->num_channels; i++) {
		sec_chan = &mode->channels[i];
		if (sec_chan->chan == channel + ht40 * 4)
			break;
		sec_chan = NULL;
	}
	if (!sec_chan)
		return;

	/* Check secondary channel flags */
	if (sec_chan->flag & (HOSTAPD_CHAN_DISABLED | HOSTAPD_CHAN_NO_IR))
		return;

	freq->channel = pri_chan->chan;

#ifdef CONFIG_P2P_UNUSED_CMD
	if (ht40 == -1) {
		if (!(pri_chan->flag & HOSTAPD_CHAN_HT40MINUS))
			return;
	} else {
		if (!(pri_chan->flag & HOSTAPD_CHAN_HT40PLUS))
			return;
	}
#endif /* CONFIG_P2P_UNUSED_CMD */
	freq->sec_channel_offset = ht40;

	if (obss_scan) {
		struct wpa_scan_results *scan_res;

		scan_res = wpa_supplicant_get_scan_results(wpa_s, NULL, 0);
		if (scan_res == NULL) {
			/* Back to HT20 */
			freq->sec_channel_offset = 0;
			return;
		}

		res = check_40mhz_5g(mode, scan_res, pri_chan->chan,
				     sec_chan->chan);
		switch (res) {
		case 0:
			/* Back to HT20 */
			freq->sec_channel_offset = 0;
			break;
		case 1:
			/* Configuration allowed */
			break;
		case 2:
			/* Switch pri/sec channels */
			freq->freq = hw_get_freq(mode, sec_chan->chan);
			freq->sec_channel_offset = -freq->sec_channel_offset;
			freq->channel = sec_chan->chan;
			break;
		default:
			freq->sec_channel_offset = 0;
			break;
		}

		wpa_scan_results_free(scan_res);
	}

#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_printf_dbg(MSG_DEBUG,
		   "IBSS/mesh: setup freq channel %d, sec_channel_offset %d",
		   freq->channel, freq->sec_channel_offset);
#else
	da16x_debug_prt("[%s] IBSS/mesh: setup freq channel %d, sec_channel_offset %d\n",
		   __func__, freq->channel, freq->sec_channel_offset);
#endif	/* __CONFIG_DBG_LOG_MSG__ */

#ifdef CONFIG_IEEE80211AC
	if (!drv_supports_vht(wpa_s, ssid))
#endif /* CONFIG_IEEE80211AC */
		return;

	/* For IBSS check VHT_IBSS flag */
	if (ssid->mode == WPAS_MODE_IBSS &&
	    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_VHT_IBSS))
		return;

	vht_freq = *freq;

#ifdef CONFIG_VHT_OVERRIDES
	if (ssid->disable_vht) {
		freq->vht_enabled = 0;
		return;
	}
#endif /* CONFIG_VHT_OVERRIDES */

#ifdef CONFIG_IEEE80211AC
	vht_freq.vht_enabled = vht_supported(mode);
	if (!vht_freq.vht_enabled)
		return;

	/* setup cent_freq1, bandwidth */
	for (j = 0; j < ARRAY_SIZE(vht80); j++) {
		if (freq->channel >= vht80[j] &&
		    freq->channel < vht80[j] + 16)
			break;
	}

	if (j == ARRAY_SIZE(vht80))
		return;

	for (i = vht80[j]; i < vht80[j] + 16; i += 4) {
		struct hostapd_channel_data *chan;

		chan = hw_get_channel_chan(mode, i, NULL);
		if (!chan)
			return;

		/* Back to HT configuration if channel not usable */
		if (chan->flag & (HOSTAPD_CHAN_DISABLED | HOSTAPD_CHAN_NO_IR))
			return;
	}

	chwidth = VHT_CHANWIDTH_80MHZ;
	seg0 = vht80[j] + 6;
	seg1 = 0;

	if (ssid->max_oper_chwidth == VHT_CHANWIDTH_80P80MHZ) {
		/* setup cent_freq2, bandwidth */
		for (k = 0; k < ARRAY_SIZE(vht80); k++) {
			/* Only accept 80 MHz segments separated by a gap */
			if (j == k || abs(vht80[j] - vht80[k]) == 16)
				continue;
			for (i = vht80[k]; i < vht80[k] + 16; i += 4) {
				struct hostapd_channel_data *chan;

				chan = hw_get_channel_chan(mode, i, NULL);
				if (!chan)
					continue;

				if (chan->flag & (HOSTAPD_CHAN_DISABLED |
						  HOSTAPD_CHAN_NO_IR |
						  HOSTAPD_CHAN_RADAR))
					continue;

				/* Found a suitable second segment for 80+80 */
				chwidth = VHT_CHANWIDTH_80P80MHZ;
				vht_caps |=
					VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ;
				seg1 = vht80[k] + 6;
			}

			if (chwidth == VHT_CHANWIDTH_80P80MHZ)
				break;
		}
	} else if (ssid->max_oper_chwidth == VHT_CHANWIDTH_160MHZ) {
		if (freq->freq == 5180) {
			chwidth = VHT_CHANWIDTH_160MHZ;
			vht_caps |= VHT_CAP_SUPP_CHAN_WIDTH_160MHZ;
			seg0 = 50;
		} else if (freq->freq == 5520) {
			chwidth = VHT_CHANWIDTH_160MHZ;
			vht_caps |= VHT_CAP_SUPP_CHAN_WIDTH_160MHZ;
			seg0 = 114;
		}
	}

	if (hostapd_set_freq_params(&vht_freq, mode->mode, freq->freq,
				    freq->channel, freq->ht_enabled,
				    vht_freq.vht_enabled,
				    freq->sec_channel_offset,
				    chwidth, seg0, seg1, vht_caps) != 0)
		return;

	*freq = vht_freq;

#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_printf_dbg(MSG_DEBUG, "IBSS: VHT setup freq cf1 %d, cf2 %d, bw %d",
		   freq->cent_freq1, freq->cent_freq2, freq->bandwidth);
#else
	da16x_debug_prt("[%s] IBSS: VHT setup freq cf1 %d, cf2 %d, bw %d\n",
			__func__,
		   freq->cent_freq1, freq->cent_freq2, freq->bandwidth);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
#endif /* CONFIG_IEEE80211AC */
}
#endif /* CONFIG_MESH */


#ifdef CONFIG_FILS
static size_t wpas_add_fils_hlp_req(struct wpa_supplicant *wpa_s, u8 *ie_buf,
				    size_t ie_buf_len)
{
	struct fils_hlp_req *req;
	size_t rem_len, hdr_len, hlp_len, len, ie_len = 0;
	const u8 *pos;
	u8 *buf = ie_buf;

	dl_list_for_each(req, &wpa_s->fils_hlp_req, struct fils_hlp_req,
			 list) {
		rem_len = ie_buf_len - ie_len;
		pos = wpabuf_head(req->pkt);
		hdr_len = 1 + 2 * ETH_ALEN + 6;
		hlp_len = wpabuf_len(req->pkt);

		if (rem_len < 2 + hdr_len + hlp_len) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_printf(MSG_ERROR,
				   "FILS: Cannot fit HLP - rem_len=%lu to_fill=%lu",
				   (unsigned long) rem_len,
				   (unsigned long) (2 + hdr_len + hlp_len));
#else
			da16x_err_prt("[%s] FILS: Cannot fit HLP - rem_len=%lu to_fill=%lu\n",
					__func__,
				   (unsigned long) rem_len,
				   (unsigned long) (2 + hdr_len + hlp_len));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			break;
		}

		len = (hdr_len + hlp_len) > 255 ? 255 : hdr_len + hlp_len;
		/* Element ID */
		*buf++ = WLAN_EID_EXTENSION;
		/* Length */
		*buf++ = len;
		/* Element ID Extension */
		*buf++ = WLAN_EID_EXT_FILS_HLP_CONTAINER;
		/* Destination MAC address */
		os_memcpy(buf, req->dst, ETH_ALEN);
		buf += ETH_ALEN;
		/* Source MAC address */
		os_memcpy(buf, wpa_s->own_addr, ETH_ALEN);
		buf += ETH_ALEN;
		/* LLC/SNAP Header */
		os_memcpy(buf, "\xaa\xaa\x03\x00\x00\x00", 6);
		buf += 6;
		/* HLP Packet */
		os_memcpy(buf, pos, len - hdr_len);
		buf += len - hdr_len;
		pos += len - hdr_len;

		hlp_len -= len - hdr_len;
		ie_len += 2 + len;
		rem_len -= 2 + len;

		while (hlp_len) {
			len = (hlp_len > 255) ? 255 : hlp_len;
			if (rem_len < 2 + len)
				break;
			*buf++ = WLAN_EID_FRAGMENT;
			*buf++ = len;
			os_memcpy(buf, pos, len);
			buf += len;
			pos += len;

			hlp_len -= len;
			ie_len += 2 + len;
			rem_len -= 2 + len;
		}
	}

	return ie_len;
}


int wpa_is_fils_supported(struct wpa_supplicant *wpa_s)
{
	return (((wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
		 (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SUPPORT_FILS)) ||
		(!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
		 (wpa_s->drv_flags & WPA_DRIVER_FLAGS_FILS_SK_OFFLOAD)));
}


int wpa_is_fils_sk_pfs_supported(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_FILS_SK_PFS
	return (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
		(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SUPPORT_FILS);
#else /* CONFIG_FILS_SK_PFS */
	return 0;
#endif /* CONFIG_FILS_SK_PFS */
}

#endif /* CONFIG_FILS */


#ifdef CONFIG_ASSOC_CB
static u8 * wpas_populate_assoc_ies(
	struct wpa_supplicant *wpa_s,
	struct wpa_bss *bss, struct wpa_ssid *ssid,
	struct wpa_driver_associate_params *params,
	enum wpa_drv_update_connect_params_mask *mask)
{
	u8 *wpa_ie;
	size_t max_wpa_ie_len = 500;
	size_t wpa_ie_len;
	int algs = WPA_AUTH_ALG_OPEN;
#ifdef CONFIG_FILS
	const u8 *realm, *username, *rrk;
	size_t realm_len, username_len, rrk_len;
	u16 next_seq_num;
	struct fils_hlp_req *req;

	dl_list_for_each(req, &wpa_s->fils_hlp_req, struct fils_hlp_req,
			 list) {
		max_wpa_ie_len += 3 + 2 * ETH_ALEN + 6 + wpabuf_len(req->pkt) +
				  2 + 2 * wpabuf_len(req->pkt) / 255;
	}
#endif /* CONFIG_FILS */

	wpa_ie = os_malloc(max_wpa_ie_len);
	if (!wpa_ie) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_printf(MSG_ERROR,
			   "Failed to allocate connect IE buffer for %lu bytes",
			   (unsigned long) max_wpa_ie_len);
#else
		da16x_err_prt("[%s] Failed to allocate connect IE buffer for %lu bytes\n",
				__func__,
			   (unsigned long) max_wpa_ie_len);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return NULL;
	}

	if (bss && (wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE) ||
		    wpa_bss_get_ie(bss, WLAN_EID_RSN)) &&
	    wpa_key_mgmt_wpa(ssid->key_mgmt)) {
		int try_opportunistic;
		const u8 *cache_id = NULL;

		try_opportunistic = (ssid->proactive_key_caching < 0 ?
				     wpa_s->conf->okc :
				     ssid->proactive_key_caching) &&
			(ssid->proto & WPA_PROTO_RSN);
#ifdef CONFIG_FILS
		if (wpa_key_mgmt_fils(ssid->key_mgmt))
			cache_id = wpa_bss_get_fils_cache_id(bss);
#endif /* CONFIG_FILS */
#ifdef	CONFIG_PMKSA
		if (pmksa_cache_set_current(wpa_s->wpa, NULL, bss->bssid,
					    ssid, try_opportunistic,
					    cache_id, 0) == 0)
			eapol_sm_notify_pmkid_attempt(wpa_s->eapol);
#endif	/* CONFIG_PMKSA */
		wpa_ie_len = max_wpa_ie_len;
		if (wpa_supplicant_set_suites(wpa_s, bss, ssid,
					      wpa_ie, &wpa_ie_len)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_WARNING, "WPA: Failed to set WPA "
				"key management and encryption suites");
#else
			da16x_warn_prt("[%s] Failed to set WPA key mgmt and "
                "encryption suites\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			os_free(wpa_ie);
			return NULL;
		}
	} else if ((ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) && bss &&
		   wpa_key_mgmt_wpa_ieee8021x(ssid->key_mgmt)) {
		/*
		 * Both WPA and non-WPA IEEE 802.1X enabled in configuration -
		 * use non-WPA since the scan results did not indicate that the
		 * AP is using WPA or WPA2.
		 */
		wpa_supplicant_set_non_wpa_policy(wpa_s, ssid);
		wpa_ie_len = 0;
		wpa_s->wpa_proto = 0;
	} else if (wpa_key_mgmt_wpa_any(ssid->key_mgmt)) {
		wpa_ie_len = max_wpa_ie_len;
		if (wpa_supplicant_set_suites(wpa_s, NULL, ssid,
					      wpa_ie, &wpa_ie_len)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_dbg(wpa_s, MSG_WARNING, "WPA: Failed to set WPA "
				"key management and encryption suites (no "
				"scan results)");
#else
			da16x_debug_prt("[%s] Failed to set WPA "
				"key management and encryption suites (no "
				"scan results)\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			os_free(wpa_ie);
			return NULL;
		}
#ifdef CONFIG_WPS
	} else if (ssid->key_mgmt & WPA_KEY_MGMT_WPS) {
		struct wpabuf *wps_ie;
		wps_ie = wps_build_assoc_req_ie(wpas_wps_get_req_type(ssid));
		if (wps_ie && wpabuf_len(wps_ie) <= max_wpa_ie_len) {
			wpa_ie_len = wpabuf_len(wps_ie);
			os_memcpy(wpa_ie, wpabuf_head(wps_ie), wpa_ie_len);
		} else
			wpa_ie_len = 0;
		wpabuf_free(wps_ie);
		wpa_supplicant_set_non_wpa_policy(wpa_s, ssid);
		if (!bss || (bss->caps & IEEE80211_CAP_PRIVACY))
			params->wps = WPS_MODE_PRIVACY;
		else
			params->wps = WPS_MODE_OPEN;
		wpa_s->wpa_proto = 0;
#endif /* CONFIG_WPS */
	} else {
		wpa_supplicant_set_non_wpa_policy(wpa_s, ssid);
		wpa_ie_len = 0;
		wpa_s->wpa_proto = 0;
	}

#ifdef IEEE8021X_EAPOL
	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		if (ssid->leap) {
			if (ssid->non_leap == 0)
				algs = WPA_AUTH_ALG_LEAP;
			else
				algs |= WPA_AUTH_ALG_LEAP;
		}
	}

#ifdef CONFIG_FILS
	/* Clear FILS association */
	wpa_sm_set_reset_fils_completed(wpa_s->wpa, 0);

	if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_FILS_SK_OFFLOAD) &&
	    ssid->eap.erp && wpa_key_mgmt_fils(wpa_s->key_mgmt) &&
	    eapol_sm_get_erp_info(wpa_s->eapol, &ssid->eap, &username,
				  &username_len, &realm, &realm_len,
				  &next_seq_num, &rrk, &rrk_len) == 0) {
		algs = WPA_AUTH_ALG_FILS;
		params->fils_erp_username = username;
		params->fils_erp_username_len = username_len;
		params->fils_erp_realm = realm;
		params->fils_erp_realm_len = realm_len;
		params->fils_erp_next_seq_num = next_seq_num;
		params->fils_erp_rrk = rrk;
		params->fils_erp_rrk_len = rrk_len;

		if (mask)
			*mask |= WPA_DRV_UPDATE_FILS_ERP_INFO;
	}
#endif /* CONFIG_FILS */
#endif /* IEEE8021X_EAPOL */
#ifdef CONFIG_SAE
	if (wpa_s->key_mgmt & (WPA_KEY_MGMT_SAE | WPA_KEY_MGMT_FT_SAE))
		algs = WPA_AUTH_ALG_SAE;
#endif /* CONFIG_SAE */

	wpa_dbg(wpa_s, MSG_DEBUG, "Automatic auth_alg selection: 0x%x", algs);
	if (ssid->auth_alg) {
		algs = ssid->auth_alg;
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Overriding auth_alg selection: 0x%x", algs);
	}

#ifdef CONFIG_P2P
	if (wpa_s->global->p2p) {
		u8 *pos;
		size_t len;
		int res;
		pos = wpa_ie + wpa_ie_len;
		len = max_wpa_ie_len - wpa_ie_len;
		res = wpas_p2p_assoc_req_ie(wpa_s, bss, pos, len,
					    ssid->p2p_group);
		if (res >= 0)
			wpa_ie_len += res;
	}

#ifdef	CONFIG_P2P_UNUSED_CMD
	//wpa_s->cross_connect_disallowed = 0;
	if (bss) {
		struct wpabuf *p2p;
		p2p = wpa_bss_get_vendor_ie_multi(bss, P2P_IE_VENDOR_TYPE);
		if (p2p) {
			wpa_s->cross_connect_disallowed =
				p2p_get_cross_connect_disallowed(p2p);
			wpabuf_free(p2p);
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: WLAN AP %s cross "
				"connection",
				wpa_s->cross_connect_disallowed ?
				"disallows" : "allows");
		}
	}
#endif	/* CONFIG_P2P_UNUSED_CMD */

	os_memset(wpa_s->p2p_ip_addr_info, 0, sizeof(wpa_s->p2p_ip_addr_info));
#endif /* CONFIG_P2P */

	if (bss) {
		wpa_ie_len += wpas_supp_op_class_ie(wpa_s, bss->freq,
						    wpa_ie + wpa_ie_len,
						    max_wpa_ie_len -
						    wpa_ie_len);
	}

	/*
	 * Workaround: Add Extended Capabilities element only if the AP
	 * included this element in Beacon/Probe Response frames. Some older
	 * APs seem to have interoperability issues if this element is
	 * included, so while the standard may require us to include the
	 * element in all cases, it is justifiable to skip it to avoid
	 * interoperability issues.
	 */
#ifdef CONFIG_P2P
	if (ssid->p2p_group)
		wpa_drv_get_ext_capa(wpa_s, WPA_IF_P2P_CLIENT);
	else
#endif	/* CONFIG_P2P */
		wpa_drv_get_ext_capa(wpa_s, WPA_IF_STATION);

	if (!bss || wpa_bss_get_ie(bss, WLAN_EID_EXT_CAPAB)) {
		u8 ext_capab[18];
		int ext_capab_len;
		ext_capab_len = wpas_build_ext_capab(wpa_s, ext_capab,
						     sizeof(ext_capab));
		if (ext_capab_len > 0 &&
		    wpa_ie_len + ext_capab_len <= max_wpa_ie_len) {
			u8 *pos = wpa_ie;
			if (wpa_ie_len > 0 && pos[0] == WLAN_EID_RSN)
				pos += 2 + pos[1];
			os_memmove(pos + ext_capab_len, pos,
				   wpa_ie_len - (pos - wpa_ie));
			wpa_ie_len += ext_capab_len;
			os_memcpy(pos, ext_capab, ext_capab_len);
		}
	}

#ifdef CONFIG_HS20
	if (is_hs20_network(wpa_s, ssid, bss)) {
		struct wpabuf *hs20;

		hs20 = wpabuf_alloc(20 + MAX_ROAMING_CONS_OI_LEN);
		if (hs20) {
			int pps_mo_id = hs20_get_pps_mo_id(wpa_s, ssid);
			size_t len;

			wpas_hs20_add_indication(hs20, pps_mo_id);
			wpas_hs20_add_roam_cons_sel(hs20, ssid);
			len = max_wpa_ie_len - wpa_ie_len;
			if (wpabuf_len(hs20) <= len) {
				os_memcpy(wpa_ie + wpa_ie_len,
					  wpabuf_head(hs20), wpabuf_len(hs20));
				wpa_ie_len += wpabuf_len(hs20);
			}
			wpabuf_free(hs20);

			hs20_configure_frame_filters(wpa_s);
		}
	}
#endif /* CONFIG_HS20 */

#if defined ( CONFIG_VENDOR_ELEM )
	if (wpa_s->vendor_elem[VENDOR_ELEM_ASSOC_REQ]) {
		struct wpabuf *buf = wpa_s->vendor_elem[VENDOR_ELEM_ASSOC_REQ];
		size_t len;

		len = max_wpa_ie_len - wpa_ie_len;
		if (wpabuf_len(buf) <= len) {
			os_memcpy(wpa_ie + wpa_ie_len,
				  wpabuf_head(buf), wpabuf_len(buf));
			wpa_ie_len += wpabuf_len(buf);
		}
	}
#endif	// CONFIG_VENDOR_ELEM

#ifdef CONFIG_FST
	if (wpa_s->fst_ies) {
		int fst_ies_len = wpabuf_len(wpa_s->fst_ies);

		if (wpa_ie_len + fst_ies_len <= max_wpa_ie_len) {
			os_memcpy(wpa_ie + wpa_ie_len,
				  wpabuf_head(wpa_s->fst_ies), fst_ies_len);
			wpa_ie_len += fst_ies_len;
		}
	}
#endif /* CONFIG_FST */

#ifdef CONFIG_MBO
	if (bss && wpa_bss_get_vendor_ie(bss, MBO_IE_VENDOR_TYPE)) {
		int len;

		len = wpas_mbo_ie(wpa_s, wpa_ie + wpa_ie_len,
				  max_wpa_ie_len - wpa_ie_len);
		if (len >= 0)
			wpa_ie_len += len;
	}
#endif /* CONFIG_MBO */

#ifdef CONFIG_FILS
	if (algs == WPA_AUTH_ALG_FILS) {
		size_t len;

		len = wpas_add_fils_hlp_req(wpa_s, wpa_ie + wpa_ie_len,
					    max_wpa_ie_len - wpa_ie_len);
		wpa_ie_len += len;
	}
#endif /* CONFIG_FILS */

#ifdef CONFIG_OWE
#ifdef CONFIG_TESTING_OPTIONS
	if (get_ie_ext(wpa_ie, wpa_ie_len, WLAN_EID_EXT_OWE_DH_PARAM)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_printf(MSG_INFO, "TESTING: Override OWE DH element");
#else
		da16x_info_prt("[%s] TESTING: Override OWE DH element\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
	} else
#endif /* CONFIG_TESTING_OPTIONS */
	if (algs == WPA_AUTH_ALG_OPEN &&
	    ssid->key_mgmt == WPA_KEY_MGMT_OWE) {
		struct wpabuf *owe_ie;
		u16 group;

		if (ssid->owe_group) {
			group = ssid->owe_group;
		} else {
			if (wpa_s->last_owe_group == 19)
				group = 20;
			else if (wpa_s->last_owe_group == 20)
				group = 21;
			else
				group = OWE_DH_GROUP;
		}
		wpa_s->last_owe_group = group;
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_printf_dbg(MSG_DEBUG, "OWE: Try to use group %u", group);
#else
		da16x_debug_prt("[%s] OWE: Try to use group %u\n", __func__, group);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		owe_ie = owe_build_assoc_req(wpa_s->wpa, group);
		if (owe_ie &&
		    wpabuf_len(owe_ie) <= max_wpa_ie_len - wpa_ie_len) {
			os_memcpy(wpa_ie + wpa_ie_len,
				  wpabuf_head(owe_ie), wpabuf_len(owe_ie));
			wpa_ie_len += wpabuf_len(owe_ie);
		}	
			wpabuf_free(owe_ie);
		}
#endif /* CONFIG_OWE */

#ifdef CONFIG_IEEE80211R
	/*
	 * Add MDIE under these conditions: the network profile allows FT,
	 * the AP supports FT, and the mobility domain ID matches.
	 */
	if (bss && wpa_key_mgmt_ft(wpa_sm_get_key_mgmt(wpa_s->wpa))) {
		const u8 *mdie = wpa_bss_get_ie(bss, WLAN_EID_MOBILITY_DOMAIN);

		if (mdie && mdie[1] >= MOBILITY_DOMAIN_ID_LEN) {
			size_t len = 0;
			const u8 *md = mdie + 2;
			const u8 *wpa_md = wpa_sm_get_ft_md(wpa_s->wpa);

			if (os_memcmp(md, wpa_md,
				      MOBILITY_DOMAIN_ID_LEN) == 0) {
				/* Add mobility domain IE */
				len = wpa_ft_add_mdie(
					wpa_s->wpa, wpa_ie + wpa_ie_len,
					max_wpa_ie_len - wpa_ie_len, mdie);
				wpa_ie_len += len;
			}
#ifdef CONFIG_SME
			if (len > 0 && wpa_s->sme.ft_used &&
			    wpa_sm_has_ptk(wpa_s->wpa)) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"SME: Trying to use FT over-the-air");
				algs |= WPA_AUTH_ALG_FT;
			}
#endif /* CONFIG_SME */
		}
	}
#endif /* CONFIG_IEEE80211R */

	params->wpa_ie = wpa_ie;
	params->wpa_ie_len = wpa_ie_len;
	params->auth_alg = algs;
	if (mask)
		*mask |= WPA_DRV_UPDATE_ASSOC_IES | WPA_DRV_UPDATE_AUTH_TYPE;

	return wpa_ie;
}
#endif /* CONFIG_ASSOC_CB */


#if defined(CONFIG_FILS) && defined(IEEE8021X_EAPOL)
static void wpas_update_fils_connect_params(struct wpa_supplicant *wpa_s)
{
	struct wpa_driver_associate_params params;
	enum wpa_drv_update_connect_params_mask mask = 0;
	u8 *wpa_ie;

	if (wpa_s->auth_alg != WPA_AUTH_ALG_OPEN)
		return; /* nothing to do */

	os_memset(&params, 0, sizeof(params));
	wpa_ie = wpas_populate_assoc_ies(wpa_s, wpa_s->current_bss,
					 wpa_s->current_ssid, &params, &mask);
	if (!wpa_ie)
		return;

	if (params.auth_alg != WPA_AUTH_ALG_FILS) {
		os_free(wpa_ie);
		return;
	}

	wpa_s->auth_alg = params.auth_alg;
	wpa_drv_update_connect_params(wpa_s, &params, mask);
	os_free(wpa_ie);
}
#endif /* CONFIG_FILS && IEEE8021X_EAPOL */


#if defined ( CONFIG_ASSOC_CB )
static void wpas_start_assoc_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_connect_work *cwork = work->ctx;
	struct wpa_bss *bss = cwork->bss;
	struct wpa_ssid *ssid = cwork->ssid;
	struct wpa_supplicant *wpa_s = work->wpa_s;
	u8 *wpa_ie;
	int use_crypt, ret, i, bssid_changed;
	unsigned int cipher_pairwise, cipher_group, cipher_group_mgmt;
	struct wpa_driver_associate_params params;
	int wep_keys_set = 0;
	int assoc_failed = 0;
	struct wpa_ssid *old_ssid;
	u8 prev_bssid[ETH_ALEN];
#ifdef CONFIG_HT_OVERRIDES
	struct ieee80211_ht_capabilities htcaps;
	struct ieee80211_ht_capabilities htcaps_mask;
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_VHT_OVERRIDES
       struct ieee80211_vht_capabilities vhtcaps;
       struct ieee80211_vht_capabilities vhtcaps_mask;
#endif /* CONFIG_VHT_OVERRIDES */

	if (deinit) {
		if (work->started) {
			wpa_s->connect_work = NULL;

			/* cancel possible auth. timeout */
			da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s,
					     NULL);
		}
		wpas_connect_work_free(cwork);
		return;
	}

	wpa_s->connect_work = work;

	if (cwork->bss_removed || !wpas_valid_bss_ssid(wpa_s, bss, ssid) ||
	    wpas_network_disabled(wpa_s, ssid)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "BSS/SSID entry for association not valid anymore - drop connection attempt");
		wpas_connect_work_done(wpa_s);
		return;
	}

	os_memcpy(prev_bssid, wpa_s->bssid, ETH_ALEN);
	os_memset(&params, 0, sizeof(params));
	wpa_s->reassociate = 0;
	wpa_s->eap_expected_failure = 0;
	if (bss &&
	    (!wpas_driver_bss_selection(wpa_s) || wpas_wps_searching(wpa_s))) {
#ifdef CONFIG_IEEE80211R
		const u8 *ie, *md = NULL;
#endif /* CONFIG_IEEE80211R */
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "Trying to associate with " MACSTR
			" (SSID='%s' freq=%d MHz)", MAC2STR(bss->bssid),
			wpa_ssid_txt(bss->ssid, bss->ssid_len), bss->freq);
#else
		da16x_notice_prt("[%s] Trying to associate with " MACSTR
            " (SSID='%s' freq=%d MHz)\n",
            __func__, MAC2STR(bss->bssid),
            wpa_ssid_txt(bss->ssid, bss->ssid_len), bss->freq);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		bssid_changed = !is_zero_ether_addr(wpa_s->bssid);
		os_memset(wpa_s->bssid, 0, ETH_ALEN);
		os_memcpy(wpa_s->pending_bssid, bss->bssid, ETH_ALEN);
#if 0	/* by Bright : Merge 2.7 */
		if (bssid_changed)
			wpas_notify_bssid_changed(wpa_s);
#endif	/* 0 */
#ifdef CONFIG_IEEE80211R
		ie = wpa_bss_get_ie(bss, WLAN_EID_MOBILITY_DOMAIN);
		if (ie && ie[1] >= MOBILITY_DOMAIN_ID_LEN)
			md = ie + 2;
		wpa_sm_set_ft_params(wpa_s->wpa, ie, ie ? 2 + ie[1] : 0);
		if (md) {
			/* Prepare for the next transition */
			wpa_ft_prepare_auth_request(wpa_s->wpa, ie);
		}
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_WPS
	} else if ((ssid->ssid == NULL || ssid->ssid_len == 0) &&
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		   wpa_s->conf->ap_scan == 2 &&
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
		   (ssid->key_mgmt & WPA_KEY_MGMT_WPS)) {
		/* Use ap_scan==1 style network selection to find the network
		 */
		wpas_connect_work_done(wpa_s);
		wpa_s->scan_req = MANUAL_SCAN_REQ;
		wpa_s->reassociate = 1;
		wpa_supplicant_req_scan(wpa_s, 0, 0);
		return;
#endif /* CONFIG_WPS */
	} else {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "Trying to associate with SSID '%s'",
			wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
#else
		da16x_notice_prt("[%s] Trying to associate with SSID '%s'\n",
			__func__,
			wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		if (bss)
			os_memcpy(wpa_s->pending_bssid, bss->bssid, ETH_ALEN);
		else
			os_memset(wpa_s->pending_bssid, 0, ETH_ALEN);
	}
#if defined ( CONFIG_SCHED_SCAN )
	if (!wpa_s->pno)
		wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN

	wpa_supplicant_cancel_scan(wpa_s);

	/* Starting new association, so clear the possibly used WPA IE from the
	 * previous association. */
	wpa_sm_set_assoc_wpa_ie(wpa_s->wpa, NULL, 0);

	wpa_ie = wpas_populate_assoc_ies(wpa_s, bss, ssid, &params, NULL);
	if (!wpa_ie) {
		wpas_connect_work_done(wpa_s);
		return;
	}

	wpa_clear_keys(wpa_s, bss ? bss->bssid : NULL);
	use_crypt = 1;
	cipher_pairwise = wpa_s->pairwise_cipher;
	cipher_group = wpa_s->group_cipher;
	cipher_group_mgmt = wpa_s->mgmt_group_cipher;
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE)
			use_crypt = 0;
		if (wpa_set_wep_keys(wpa_s, ssid)) {
			use_crypt = 1;
			wep_keys_set = 1;
		}
	}
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPS)
		use_crypt = 0;

#ifdef IEEE8021X_EAPOL
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		if ((ssid->eapol_flags &
		     (EAPOL_FLAG_REQUIRE_KEY_UNICAST |
		      EAPOL_FLAG_REQUIRE_KEY_BROADCAST)) == 0 &&
		    !wep_keys_set) {
			use_crypt = 0;
		} else {
			/* Assume that dynamic WEP-104 keys will be used and
			 * set cipher suites in order for drivers to expect
			 * encryption. */
			cipher_pairwise = cipher_group = WPA_CIPHER_WEP104;
		}
	}
#endif /* IEEE8021X_EAPOL */

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE) {
		/* Set the key before (and later after) association */
		wpa_supplicant_set_wpa_none_key(wpa_s, ssid);
	}

	wpa_supplicant_set_state(wpa_s, WPA_ASSOCIATING);
	if (bss) {
		params.ssid = bss->ssid;
		params.ssid_len = bss->ssid_len;
		if (!wpas_driver_bss_selection(wpa_s) || ssid->bssid_set ||
		    wpa_s->key_mgmt == WPA_KEY_MGMT_WPS) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_printf_dbg(MSG_DEBUG, "Limit connection to BSSID "
				   MACSTR " freq=%u MHz based on scan results "
				   "(bssid_set=%d wps=%d)",
				   MAC2STR(bss->bssid), bss->freq,
				   ssid->bssid_set,
				   wpa_s->key_mgmt == WPA_KEY_MGMT_WPS);
#else
			da16x_debug_prt("[%s] Limit connection to BSSID "
				   MACSTR " freq=%u MHz based on scan results "
				   "(bssid_set=%d wps=%d)\n",
					__func__,
				   MAC2STR(bss->bssid), bss->freq,
				   ssid->bssid_set,
				   wpa_s->key_mgmt == WPA_KEY_MGMT_WPS);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			params.bssid = bss->bssid;
			params.freq.freq = bss->freq;
		}
		params.bssid_hint = bss->bssid;
		params.freq_hint = bss->freq;
		params.pbss = bss_is_pbss(bss);
	} else {
#ifdef	CONFIG_P2P_UNUSED_CMD
		if (ssid->bssid_hint_set)
			params.bssid_hint = ssid->bssid_hint;
#endif	/* CONFIG_P2P_UNUSED_CMD */
		params.ssid = ssid->ssid;
		params.ssid_len = ssid->ssid_len;
		params.pbss = (ssid->pbss != 2) ? ssid->pbss : 0;
	}

	if (ssid->mode == WPAS_MODE_IBSS && ssid->bssid_set
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		&& wpa_s->conf->ap_scan == 2
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	    ) {
		params.bssid = ssid->bssid;
		params.fixed_bssid = 1;
	}

#ifdef CONFIG_MESH
	/* Initial frequency for IBSS/mesh */
	if ((ssid->mode == WPAS_MODE_IBSS
#ifdef CONFIG_MESH
		|| ssid->mode == WPAS_MODE_MESH
#endif /* CONFIG_MESH */
		) &&
	    ssid->frequency > 0 && params.freq.freq == 0)
		ibss_mesh_setup_freq(wpa_s, ssid, &params.freq);
#endif /* CONFIG_MESH */

	if (ssid->mode == WPAS_MODE_IBSS) {
		params.fixed_freq = ssid->fixed_freq;
		if (ssid->beacon_int)
			params.beacon_int = ssid->beacon_int;
		else
			params.beacon_int = wpa_s->conf->beacon_int;
	}

	params.pairwise_suite = cipher_pairwise;
	params.group_suite = cipher_group;
	params.mgmt_group_suite = cipher_group_mgmt;
	params.key_mgmt_suite = wpa_s->key_mgmt;
	params.wpa_proto = wpa_s->wpa_proto;
	wpa_s->auth_alg = params.auth_alg;
	params.mode = ssid->mode;
#ifdef CONFIG_BGSCAN
	params.bg_scan_period = ssid->bg_scan_period;
#endif /* CONFIG_BGSCAN */
	for (i = 0; i < NUM_WEP_KEYS; i++) {
		if (ssid->wep_key_len[i])
			params.wep_key[i] = ssid->wep_key[i];
		params.wep_key_len[i] = ssid->wep_key_len[i];
	}
	params.wep_tx_keyidx = ssid->wep_tx_keyidx;

	if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE) &&
	    (params.key_mgmt_suite == WPA_KEY_MGMT_PSK ||
	     params.key_mgmt_suite == WPA_KEY_MGMT_FT_PSK)) {
		params.passphrase = ssid->passphrase;
		if (ssid->psk_set)
			params.psk = ssid->psk;
	}

	if (wpa_s->conf->key_mgmt_offload) {
		if (params.key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X ||
		    params.key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SHA256 ||
		    params.key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SUITE_B ||
		    params.key_mgmt_suite == WPA_KEY_MGMT_IEEE8021X_SUITE_B_192)
			params.req_key_mgmt_offload =
				ssid->proactive_key_caching < 0 ?
				wpa_s->conf->okc : ssid->proactive_key_caching;
		else
			params.req_key_mgmt_offload = 1;

		if ((params.key_mgmt_suite == WPA_KEY_MGMT_PSK ||
		     params.key_mgmt_suite == WPA_KEY_MGMT_PSK_SHA256 ||
		     params.key_mgmt_suite == WPA_KEY_MGMT_FT_PSK) &&
		    ssid->psk_set)
			params.psk = ssid->psk;
	}

	params.drop_unencrypted = use_crypt;

#ifdef CONFIG_IEEE80211W
	params.mgmt_frame_protection = wpas_get_ssid_pmf(wpa_s, ssid);
	if (params.mgmt_frame_protection != NO_MGMT_FRAME_PROTECTION && bss) {
		const u8 *rsn = wpa_bss_get_ie(bss, WLAN_EID_RSN);
		struct wpa_ie_data ie;
		if (rsn && wpa_parse_wpa_ie(rsn, 2 + rsn[1], &ie) == 0 &&
		    ie.capabilities &
		    (WPA_CAPABILITY_MFPC | WPA_CAPABILITY_MFPR)) {
			wpa_dbg(wpa_s, MSG_DEBUG, "WPA: Selected AP supports "
				"MFP: require MFP");
			params.mgmt_frame_protection =
				MGMT_FRAME_PROTECTION_REQUIRED;
#ifdef CONFIG_OWE
		} else if (!rsn && (ssid->key_mgmt & WPA_KEY_MGMT_OWE) &&
			   !ssid->owe_only) {
			params.mgmt_frame_protection = NO_MGMT_FRAME_PROTECTION;
#endif /* CONFIG_OWE */			
		}
	}
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_P2P
	params.p2p = ssid->p2p_group;
#endif	/* CONFIG_P2P */

	if (wpa_s->parent->set_sta_uapsd)
		params.uapsd = wpa_s->parent->sta_uapsd;
	else
		params.uapsd = -1;

#ifdef CONFIG_HT_OVERRIDES
	os_memset(&htcaps, 0, sizeof(htcaps));
	os_memset(&htcaps_mask, 0, sizeof(htcaps_mask));
	params.htcaps = (u8 *) &htcaps;
	params.htcaps_mask = (u8 *) &htcaps_mask;
	wpa_supplicant_apply_ht_overrides(wpa_s, ssid, &params);
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_VHT_OVERRIDES
	os_memset(&vhtcaps, 0, sizeof(vhtcaps));
	os_memset(&vhtcaps_mask, 0, sizeof(vhtcaps_mask));
	params.vhtcaps = &vhtcaps;
	params.vhtcaps_mask = &vhtcaps_mask;
	wpa_supplicant_apply_vht_overrides(wpa_s, ssid, &params);
#endif /* CONFIG_VHT_OVERRIDES */

#ifdef CONFIG_P2P_UNUSED_CMD
	/*
	 * If multi-channel concurrency is not supported, check for any
	 * frequency conflict. In case of any frequency conflict, remove the
	 * least prioritized connection.
	 */
	if (wpa_s->num_multichan_concurrent < 2) {
		int freq, num;
		num = get_shared_radio_freqs(wpa_s, &freq, 1);
		if (num > 0 && freq > 0 && freq != params.freq.freq) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_printf_dbg(MSG_DEBUG,
				   "Assoc conflicting freq found (%d != %d)",
				   freq, params.freq.freq);
#else
			da16x_debug_prt("[%s] Assoc conflicting freq found (%d != %d)\n",
				   freq, params.freq.freq);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			if (wpas_p2p_handle_frequency_conflicts(
				    wpa_s, params.freq.freq, ssid) < 0) {
				wpas_connect_work_done(wpa_s);
				os_free(wpa_ie);
				return;
			}
		}
	}
#endif /* CONFIG_P2P_UNUSED_CMD */

	if (wpa_s->reassoc_same_ess && !is_zero_ether_addr(prev_bssid) &&
	    wpa_s->current_ssid)
		params.prev_bssid = prev_bssid;

	ret = wpa_drv_associate(wpa_s, &params);
	os_free(wpa_ie);
	if (ret < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "Association request to the driver "
			"failed");
#else
		da16x_info_prt("[%s] Assoc request to the driver failed\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SANE_ERROR_CODES) {
			/*
			 * The driver is known to mean what is saying, so we
			 * can stop right here; the association will not
			 * succeed.
			 */
			wpas_connection_failed(wpa_s, wpa_s->pending_bssid);
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE // trinity_0160905, Issue #185
			wpa_supplicant_mark_disassoc(wpa_s);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
			wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
			os_memset(wpa_s->pending_bssid, 0, ETH_ALEN);
			return;
		}
		/* try to continue anyway; new association will be tried again
		 * after timeout */
		assoc_failed = 1;
	}

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE) {
		/* Set the key after the association just in case association
		 * cleared the previously configured key. */
		wpa_supplicant_set_wpa_none_key(wpa_s, ssid);
		/* No need to timeout authentication since there is no key
		 * management. */
		wpa_supplicant_cancel_auth_timeout(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_COMPLETED);
#ifdef CONFIG_IBSS_RSN
	} else if (ssid->mode == WPAS_MODE_IBSS &&
		   wpa_s->key_mgmt != WPA_KEY_MGMT_NONE &&
		   wpa_s->key_mgmt != WPA_KEY_MGMT_WPA_NONE) {
		/*
		 * RSN IBSS authentication is per-STA and we can disable the
		 * per-BSSID authentication.
		 */
		wpa_supplicant_cancel_auth_timeout(wpa_s);
#endif /* CONFIG_IBSS_RSN */
	} else {
		/* Timeout for IEEE 802.11 authentication and association */
		int timeout = 60;

		if (assoc_failed) {
			/* give IBSS a bit more time */
			timeout = ssid->mode == WPAS_MODE_IBSS ? 10 : 5;
		}
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		else if (wpa_s->conf->ap_scan == 1) {
			/* give IBSS a bit more time */
			timeout = ssid->mode == WPAS_MODE_IBSS ? 20 : 10;
		}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
		wpa_supplicant_req_auth_timeout(wpa_s, timeout, 0);
	}

	if (wep_keys_set &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC)) {
		/* Set static WEP keys again */
		wpa_set_wep_keys(wpa_s, ssid);
	}

	if (wpa_s->current_ssid && wpa_s->current_ssid != ssid) {
		/*
		 * Do not allow EAP session resumption between different
		 * network configurations.
		 */
		eapol_sm_invalidate_cached_session(wpa_s->eapol);
	}
	old_ssid = wpa_s->current_ssid;
	wpa_s->current_ssid = ssid;

	if (!wpas_driver_bss_selection(wpa_s) || ssid->bssid_set) {
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);
#endif	/* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = bss;
#ifdef CONFIG_HS20
		hs20_configure_frame_filters(wpa_s);
#endif /* CONFIG_HS20 */
	}

	wpa_supplicant_rsn_supp_set_config(wpa_s, wpa_s->current_ssid);
	wpa_supplicant_initiate_eapol(wpa_s);
#if 0	/* by Bright : Merge 2.7 */
	if (old_ssid != wpa_s->current_ssid)
		wpas_notify_network_changed(wpa_s);
#endif	/* 0 */
}
#endif // CONFIG_ASSOC_CB


static void wpa_supplicant_clear_connection(struct wpa_supplicant *wpa_s,
					    const u8 *addr)
{
#if 0
	struct wpa_ssid *old_ssid;
#endif /* 0 */

#ifdef	CONFIG_RECONNECT_OPTIMIZE /* by Shingu 20170717 (Scan Optimization) */
	wpa_s->reassoc_freq = 0;
#endif	/* CONFIG_RECONNECT_OPTIMIZE */

	wpas_connect_work_done(wpa_s);
	wpa_clear_keys(wpa_s, addr);
#if 0
	old_ssid = wpa_s->current_ssid;
#endif /* 0 */
	wpa_supplicant_mark_disassoc(wpa_s);
	wpa_sm_set_config(wpa_s->wpa, NULL);
	eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
#if 0	/* by Bright : Merge 2.7 */
	if (old_ssid != wpa_s->current_ssid)
		wpas_notify_network_changed(wpa_s);
#endif	/* 0 */
	da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);

	clr_dpm_conn_info();
}


/**
 * wpa_supplicant_deauthenticate - Deauthenticate the current connection
 * @wpa_s: Pointer to wpa_supplicant data
 * @reason_code: IEEE 802.11 reason code for the deauthenticate frame
 *
 * This function is used to request %wpa_supplicant to deauthenticate from the
 * current AP.
 */
void wpa_supplicant_deauthenticate(struct wpa_supplicant *wpa_s, int reason_code)
{
	u8 *addr = NULL;
	union wpa_event_data event;
	int zero_addr = 0;

	wpa_dbg(wpa_s, MSG_DEBUG, "Request to deauthenticate - bssid=" MACSTR
		" pending_bssid=" MACSTR " reason=%d state=%s",
		MAC2STR(wpa_s->bssid), MAC2STR(wpa_s->pending_bssid),
		reason_code, wpa_supplicant_state_txt(wpa_s->wpa_state));

	if (!is_zero_ether_addr(wpa_s->pending_bssid) &&
	    (wpa_s->wpa_state == WPA_AUTHENTICATING ||
	     wpa_s->wpa_state == WPA_ASSOCIATING))
		addr = wpa_s->pending_bssid;
	else if (!is_zero_ether_addr(wpa_s->bssid))
		addr = wpa_s->bssid;
	else if (wpa_s->wpa_state == WPA_ASSOCIATING) {
		/*
		 * When using driver-based BSS selection, we may not know the
		 * BSSID with which we are currently trying to associate. We
		 * need to notify the driver of this disconnection even in such
		 * a case, so use the all zeros address here.
		 */
		addr = wpa_s->bssid;
		zero_addr = 1;
	}

#ifdef CONFIG_TDLS
	wpa_tdls_teardown_peers(wpa_s->wpa);
#endif /* CONFIG_TDLS */

#if 0 //def CONFIG_MESH // Move to wpa_supp_ctrl_iface_remove_network() and wpa_supplicant_flush() by Dialog
	if (wpa_s->ifmsh) {
		struct mesh_conf *mconf;

		mconf = wpa_s->ifmsh->mconf;
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, MESH_GROUP_REMOVED "%s",
			wpa_s->ifname);
#else
		da16x_info_prt("[%s] "MESH_GROUP_REMOVED "%s",
			__func__, wpa_s->ifname);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		//wpas_notify_mesh_group_removed(wpa_s, mconf->meshid,
		//			       mconf->meshid_len, reason_code);
		wpa_supplicant_leave_mesh(wpa_s);
	}
#endif /* CONFIG_MESH */

	if (addr) {
		wpa_drv_deauthenticate(wpa_s, addr, reason_code);
		os_memset(&event, 0, sizeof(event));
		event.deauth_info.reason_code = (u16) reason_code;
		event.deauth_info.locally_generated = 1;
		wpa_supplicant_event(wpa_s, EVENT_DEAUTH, &event);
		if (zero_addr)
			addr = NULL;
	}

	wpa_supplicant_clear_connection(wpa_s, addr);
}

static void wpa_supplicant_enable_one_network(struct wpa_supplicant *wpa_s,
					      struct wpa_ssid *ssid)
{
	if (!ssid || !ssid->disabled || ssid->disabled == 2)
		return;

	ssid->disabled = 0;
#ifdef CONFIG_OWE
	ssid->owe_transition_bss_select_count = 0;
#endif
	wpas_clear_temp_disabled(wpa_s, ssid, 1);
#if 0	/* by Bright : Merge 2.7 */
	wpas_notify_network_enabled_changed(wpa_s, ssid);
#endif	/* 0 */

	/*
	 * Try to reassociate since there is no current configuration and a new
	 * network was made available.
	 */
	if (!wpa_s->current_ssid && !wpa_s->disconnected)
		wpa_s->reassociate = 1;
}


/**
 * wpa_supplicant_add_network - Add a new network
 * @wpa_s: wpa_supplicant structure for a network interface
 * Returns: The new network configuration or %NULL if operation failed
 *
 * This function performs the following operations:
 * 1. Adds a new network.
 * 2. Send network addition notification.
 * 3. Marks the network disabled.
 * 4. Set network default parameters.
 */
struct wpa_ssid * wpa_supplicant_add_network(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;

#if 1 /* FC9000 Only */
	ssid = wpa_config_add_network(wpa_s->conf, wpa_s->ifname,
				      FIXED_NETWORK_ID_NONE);
#else
	ssid = wpa_config_add_network(wpa_s->conf);
#endif /* FC9000 Only */
	if (!ssid)
		return NULL;
#if 0	/* by Bright : Merge 2.7 */
	wpas_notify_network_added(wpa_s, ssid);
#endif	/* 0 */
	ssid->disabled = 1;
	wpa_config_set_network_defaults(ssid);

	return ssid;
}


/**
 * wpa_supplicant_remove_network - Remove a configured network based on id
 * @wpa_s: wpa_supplicant structure for a network interface
 * @id: Unique network id to search for
 * Returns: 0 on success, or -1 if the network was not found, -2 if the network
 * could not be removed
 *
 * This function performs the following operations:
 * 1. Removes the network.
 * 2. Send network removal notification.
 * 3. Update internal state machines.
 * 4. Stop any running sched scans.
 */
int wpa_supplicant_remove_network(struct wpa_supplicant *wpa_s, int id)
{
	struct wpa_ssid *ssid;
#ifdef CONFIG_SCHED_SCAN
	int was_disabled;
#endif /* CONFIG_SCHED_SCAN */

	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (!ssid)
		return -1;
#if 0	/* by Bright : Merge 2.7 */
	wpas_notify_network_removed(wpa_s, ssid);
#endif	/* 0 */

	if (wpa_s->last_ssid == ssid)
		wpa_s->last_ssid = NULL;

	if (ssid == wpa_s->current_ssid || !wpa_s->current_ssid) {
#ifdef CONFIG_SME
		wpa_s->sme.prev_bssid_set = 0;
#endif /* CONFIG_SME */
		/*
		 * Invalidate the EAP session cache if the current or
		 * previously used network is removed.
		 */
		eapol_sm_invalidate_cached_session(wpa_s->eapol);
	}

	if (ssid == wpa_s->current_ssid) {
		wpa_sm_set_config(wpa_s->wpa, NULL);
		eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);

		if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
			wpa_s->own_disconnect_req = 1;
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);
	}

#ifdef CONFIG_SCHED_SCAN
	was_disabled = ssid->disabled;
#endif /* CONFIG_SCHED_SCAN */

	if (wpa_config_remove_network(wpa_s->conf, id) < 0)
		return -2;

#if defined ( CONFIG_SCHED_SCAN )
	if (!was_disabled && wpa_s->sched_scanning) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_printf_dbg(MSG_DEBUG,
			   "Stop ongoing sched_scan to remove network from filters");
#else
		da16x_debug_prt("[%s] Stop ongoing sched_scan to remove network from filters\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		wpa_supplicant_cancel_sched_scan(wpa_s);
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	}
#endif	// CONFIG_SCAN_WORK

	return 0;
}


/**
 * wpa_supplicant_enable_network - Mark a configured network as enabled
 * @wpa_s: wpa_supplicant structure for a network interface
 * @ssid: wpa_ssid structure for a configured network or %NULL
 *
 * Enables the specified network or all networks if no network specified.
 */
void wpa_supplicant_enable_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid)
{
	if (ssid == NULL) {
		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next)
			wpa_supplicant_enable_one_network(wpa_s, ssid);
	} else
		wpa_supplicant_enable_one_network(wpa_s, ssid);

	if (wpa_s->reassociate && !wpa_s->disconnected &&
	    (!wpa_s->current_ssid ||
	     wpa_s->wpa_state == WPA_DISCONNECTED ||
	     wpa_s->wpa_state == WPA_SCANNING)) {
#if defined ( CONFIG_SCHED_SCAN )
		if (wpa_s->sched_scanning) {
			wpa_printf_dbg(MSG_DEBUG, "Stop ongoing sched_scan to add "
				   "new network to scan filters");
			wpa_supplicant_cancel_sched_scan(wpa_s);
		}
#endif // CONFIG_SCHED_SCAN

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
		if (wpa_supplicant_fast_associate(wpa_s) != 1)
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
		{
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
			wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
			wpa_s->scan_req = NORMAL_SCAN_REQ;
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		}
	}
}


/**
 * wpa_supplicant_disable_network - Mark a configured network as disabled
 * @wpa_s: wpa_supplicant structure for a network interface
 * @ssid: wpa_ssid structure for a configured network or %NULL
 *
 * Disables the specified network or all networks if no network specified.
 */
void wpa_supplicant_disable_network(struct wpa_supplicant *wpa_s,
				    struct wpa_ssid *ssid)
{
	struct wpa_ssid *other_ssid;
	int was_disabled;

	if (ssid == NULL) {
#if defined ( CONFIG_SCHED_SCAN )
		if (wpa_s->sched_scanning)
			wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN

		for (other_ssid = wpa_s->conf->ssid; other_ssid;
		     other_ssid = other_ssid->next) {
			was_disabled = other_ssid->disabled;
			if (was_disabled == 2)
				continue; /* do not change persistent P2P group
					   * data */

			other_ssid->disabled = 1;

#if 0	/* by Bright : Merge 2.7 */
			if (was_disabled != other_ssid->disabled)
				wpas_notify_network_enabled_changed( wpa_s, other_ssid);
#endif	/* 0 */
		}
		if (wpa_s->current_ssid) {
			if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
				wpa_s->own_disconnect_req = 1;
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		}
	} else if (ssid->disabled != 2) {
		if (ssid == wpa_s->current_ssid) {
			if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
				wpa_s->own_disconnect_req = 1;
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		}

		was_disabled = ssid->disabled;

		ssid->disabled = 1;

#if defined ( CONFIG_SCHED_SCAN )
		if (was_disabled != ssid->disabled) {
#if 0	/* by Bright : Merge 2.7 */
			wpas_notify_network_enabled_changed(wpa_s, ssid);
#endif	/* 0 */
			if (wpa_s->sched_scanning) {
				da16x_notice_prt("Stop ongoing sched_scan "
					   "to remove network from filters");
				wpa_supplicant_cancel_sched_scan(wpa_s);
				wpa_supplicant_req_scan(wpa_s, 0, 0);
			}
		}
#endif // CONFIG_SCHED_SCAN
	}
}


/**
 * wpa_supplicant_select_network - Attempt association with a network
 * @wpa_s: wpa_supplicant structure for a network interface
 * @ssid: wpa_ssid structure for a configured network or %NULL for any network
 */
void wpa_supplicant_select_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid)
{

	struct wpa_ssid *other_ssid;
	int disconnected = 0;

#ifdef CONFIG_FAST_CONNECTION
	wpa_s->num_retries = 0;
#endif /* CONFIG_FAST_CONNECTION */

	if (ssid && ssid != wpa_s->current_ssid && wpa_s->current_ssid) {
#if 0	/* by Bright : Merge 2.7 */
		if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
			wpa_s->own_disconnect_req = 1;
#endif	/* 0 */
		wpa_supplicant_deauthenticate(
			wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		disconnected = 1;
	}

	if (ssid)
		wpas_clear_temp_disabled(wpa_s, ssid, 1);

	/*
	 * Mark all other networks disabled or mark all networks enabled if no
	 * network specified.
	 */
	for (other_ssid = wpa_s->conf->ssid; other_ssid;
	     other_ssid = other_ssid->next) {
		int was_disabled = other_ssid->disabled;

		if (was_disabled == 2)
			continue; /* do not change persistent P2P group data */

		other_ssid->disabled = ssid ? (ssid->id != other_ssid->id) : 0;
		if (was_disabled && !other_ssid->disabled)
			wpas_clear_temp_disabled(wpa_s, other_ssid, 0);

#ifdef CONFIG_NOTIFY
		if (was_disabled != other_ssid->disabled)
			wpas_notify_network_enabled_changed(wpa_s, other_ssid);
#endif /* CONFIG_NOTIFY */
	}

	if (ssid && ssid == wpa_s->current_ssid && wpa_s->current_ssid &&
	    wpa_s->wpa_state >= WPA_AUTHENTICATING) {
		/* We are already associated with the selected network */
		da16x_notice_prt("Already associated with the selected network - do nothing");
		return;
	}

	if (ssid) {
		wpa_s->current_ssid = ssid;

#if 0	/* by Bright : Merge 2.7 */
		eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
#endif	/* 0 */
		wpa_s->connect_without_scan =
#ifdef CONFIG_MESH
			(ssid->mode == WPAS_MODE_MESH) ? ssid : NULL;
#else
			NULL;
#endif /* CONFIG_MESH */

#if 0	/* by Bright : Merge 2.7 */
		/*
		 * Don't optimize next scan freqs since a new ESS has been
		 * selected.
		 */
		os_free(wpa_s->next_scan_freqs);
#endif	/* 0 */
		wpa_s->next_scan_freqs = NULL;
	} else {
		wpa_s->connect_without_scan = NULL;
	}

	wpa_s->disconnected = 0;
	wpa_s->reassociate = 1;

#ifdef CONFIG_OWE
	wpa_s->last_owe_group = 0;

	if(ssid)
		ssid->owe_transition_bss_select_count = 0;
#endif /* CONFIG_OWE */

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	if (
#ifdef	CONFIG_MESH
		wpa_s->connect_without_scan ||
#endif	/* CONFIG_MESH */
 		wpa_supplicant_fast_associate(wpa_s) != 1
	    )
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
	{
		wpa_s->scan_req = NORMAL_SCAN_REQ;
#if 0	/* by Bright : Merge 2.7 */
		wpas_scan_reset_sched_scan(wpa_s);
#endif	/* 0 */
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
		wpa_supplicant_req_scan(wpa_s, 0, disconnected ? 100000 : 0);
	}

#ifdef CONFIG_NOTIFY
	if (ssid)
		wpas_notify_network_selected(wpa_s, ssid);
#endif /* CONFIG_NOTIFY */
}


#ifdef CONFIG_OPENSSL_MOD
/**
 * wpas_set_pkcs11_engine_and_module_path - Set PKCS #11 engine and module path
 * @wpa_s: wpa_supplicant structure for a network interface
 * @pkcs11_engine_path: PKCS #11 engine path or NULL
 * @pkcs11_module_path: PKCS #11 module path or NULL
 * Returns: 0 on success; -1 on failure
 *
 * Sets the PKCS #11 engine and module path. Both have to be NULL or a valid
 * path. If resetting the EAPOL state machine with the new PKCS #11 engine and
 * module path fails the paths will be reset to the default value (NULL).
 */
int wpas_set_pkcs11_engine_and_module_path(struct wpa_supplicant *wpa_s,
					   const char *pkcs11_engine_path,
					   const char *pkcs11_module_path)
{
	char *pkcs11_engine_path_copy = NULL;
	char *pkcs11_module_path_copy = NULL;

	if (pkcs11_engine_path != NULL) {
		pkcs11_engine_path_copy = os_strdup(pkcs11_engine_path);
		if (pkcs11_engine_path_copy == NULL)
			return -1;
	}
	if (pkcs11_module_path != NULL) {
		pkcs11_module_path_copy = os_strdup(pkcs11_module_path);
		if (pkcs11_module_path_copy == NULL) {
			os_free(pkcs11_engine_path_copy);
			return -1;
		}
	}

	os_free(wpa_s->conf->pkcs11_engine_path);
	os_free(wpa_s->conf->pkcs11_module_path);
	wpa_s->conf->pkcs11_engine_path = pkcs11_engine_path_copy;
	wpa_s->conf->pkcs11_module_path = pkcs11_module_path_copy;

	wpa_sm_set_eapol(wpa_s->wpa, NULL);
	eapol_sm_deinit(wpa_s->eapol);
	wpa_s->eapol = NULL;
	if (wpa_supplicant_init_eapol(wpa_s)) {
		/* Error -> Reset paths to the default value (NULL) once. */
		if (pkcs11_engine_path != NULL && pkcs11_module_path != NULL)
			wpas_set_pkcs11_engine_and_module_path(wpa_s, NULL,
							       NULL);

		return -1;
	}
	wpa_sm_set_eapol(wpa_s->wpa, wpa_s->eapol);

	return 0;
}
#endif /* CONFIG_OPENSSL_MOD */


/**
 * wpa_supplicant_set_ap_scan - Set AP scan mode for interface
 * @wpa_s: wpa_supplicant structure for a network interface
 * @ap_scan: AP scan mode
 * Returns: 0 if succeed or -1 if ap_scan has an invalid value
 *
 */
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
int wpa_supplicant_set_ap_scan(struct wpa_supplicant *wpa_s, int ap_scan)
{

#if 0	/* by Bright */
	int old_ap_scan;
#endif	/* 0 */

	if (ap_scan < 0 || ap_scan > 2)
		return -1;

	if (ap_scan == 2 && os_strcmp(wpa_s->driver->name, "nld11") == 0) {
		da16x_notice_prt("Note: nld11 driver interface is not designed to be used with ap_scan=2; this can result in connection failures");
	}

#ifdef ANDROID
	if (ap_scan == 2 && ap_scan != wpa_s->conf->ap_scan &&
	    wpa_s->wpa_state >= WPA_ASSOCIATING &&
	    wpa_s->wpa_state < WPA_COMPLETED) {
		da16x_err_prt("ap_scan = %d (%d) rejected while "
			   "associating", wpa_s->conf->ap_scan, ap_scan);
		return 0;
	}
#endif /* ANDROID */

#if 0	/* by Bright */
	old_ap_scan = wpa_s->conf->ap_scan;
#endif	/* 0 */
	wpa_s->conf->ap_scan = ap_scan;

#ifdef CONFIG_NOTIFY
	if (old_ap_scan != wpa_s->conf->ap_scan)
		wpas_notify_ap_scan_changed(wpa_s);
#endif /* CONFIG_NOTIFY */

#ifdef  CONFIG_CONCURRENT /* by Shingu 20160905 (Concurrent) */
	if (   get_run_mode() == STA_SOFT_AP_CONCURRENT_MODE
#ifdef CONFIG_P2P
		|| get_run_mode() == STA_P2P_CONCURRENT_MODE
#endif /* CONFIG_P2P */
	   ) {
		struct wpa_supplicant *wlan1_wpa_s = wpa_s->global->ifaces;
		wlan1_wpa_s->conf->ap_scan = ap_scan;
	}
#endif  /* CONFIG_CONCURRENT */

	return 0;
}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */


#ifndef CONFIG_IMMEDIATE_SCAN
/**
 * wpa_supplicant_set_bss_expiration_age - Set BSS entry expiration age
 * @wpa_s: wpa_supplicant structure for a network interface
 * @expire_age: Expiration age in seconds
 * Returns: 0 if succeed or -1 if expire_age has an invalid value
 *
 */
int wpa_supplicant_set_bss_expiration_age(struct wpa_supplicant *wpa_s,
					  unsigned int bss_expire_age)
{
	if (bss_expire_age < 10) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Invalid bss expiration age %u",
			bss_expire_age);
#else
		da16x_err_prt("[%s] Invalid bss expiration age %u\n", __func__, bss_expire_age);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_dbg(wpa_s, MSG_DEBUG, "Setting bss expiration age: %d sec",
		bss_expire_age);
#else
	da16x_scan_prt("[%s] Setting bss expiration age: %d sec\n",
                    __func__, bss_expire_age);

#endif	/* __CONFIG_DBG_LOG_MSG__ */
	wpa_s->conf->bss_expiration_age = bss_expire_age;

	return 0;
}


/**
 * wpa_supplicant_set_bss_expiration_count - Set BSS entry expiration scan count
 * @wpa_s: wpa_supplicant structure for a network interface
 * @expire_count: number of scans after which an unseen BSS is reclaimed
 * Returns: 0 if succeed or -1 if expire_count has an invalid value
 *
 */
int wpa_supplicant_set_bss_expiration_count(struct wpa_supplicant *wpa_s,
					    unsigned int bss_expire_count)
{
	if (bss_expire_count < 1) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Invalid bss expiration count %u",
			bss_expire_count);
#else
		da16x_err_prt("[%s] Invalid bss expiration count %u\n", __func__, bss_expire_count);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_dbg(wpa_s, MSG_DEBUG, "Setting bss expiration scan count: %u",
		bss_expire_count);
#else
	da16x_scan_prt("[%s] Setting bss expiration scan count: %u\n",
        __func__, bss_expire_count);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
	wpa_s->conf->bss_expiration_scan_count = bss_expire_count;

	return 0;
}
#endif /* CONFIG_IMMEDIATE_SCAN */

#if 0	/* by Bright : Merge 2.7 */
/**
 * wpa_supplicant_set_scan_interval - Set scan interval
 * @wpa_s: wpa_supplicant structure for a network interface
 * @scan_interval: scan interval in seconds
 * Returns: 0 if succeed or -1 if scan_interval has an invalid value
 *
 */
int wpa_supplicant_set_scan_interval(struct wpa_supplicant *wpa_s,
				     int scan_interval)
{
	if (scan_interval < 0) {
		wpa_msg(wpa_s, MSG_ERROR, "Invalid scan interval %d",
			scan_interval);
		return -1;
	}
	wpa_msg(wpa_s, MSG_DEBUG, "Setting scan interval: %d sec",
		scan_interval);
	wpa_supp_update_scan_int(wpa_s, scan_interval);

	return 0;
}
#endif	/* 0 */

/**
 * wpa_supplicant_set_debug_params - Set global debug params
 * @global: wpa_global structure
 * @debug_level: debug level
 * @debug_timestamp: determines if show timestamp in debug data
 * @debug_show_keys: determines if show keys in debug data
 * Returns: 0 if succeed or -1 if debug_level has wrong value
 */
int wpa_supplicant_set_debug_params(struct wpa_global *global, int debug_level,
				    int debug_timestamp, int debug_show_keys)
{

#if 0
	int old_level, old_timestamp, old_show_keys;
#endif /* 0 */

	/* check for allowed debuglevels */
	if (debug_level != MSG_EXCESSIVE &&
	    debug_level != MSG_MSGDUMP &&
	    debug_level != MSG_DEBUG &&
	    debug_level != MSG_INFO &&
	    debug_level != MSG_WARNING &&
	    debug_level != MSG_ERROR)
		return -1;

#if 0
	old_level = wpa_debug_level;
	old_timestamp = wpa_debug_timestamp;
	old_show_keys = wpa_debug_show_keys;
#endif /* 0 */

	wpa_debug_level = debug_level;
	wpa_debug_timestamp = debug_timestamp ? 1 : 0;
	wpa_debug_show_keys = debug_show_keys ? 1 : 0;

#if 0	/* by Bright : Merge 2.7 */
	if (wpa_debug_level != old_level)
		wpas_notify_debug_level_changed(global);
	if (wpa_debug_timestamp != old_timestamp)
		wpas_notify_debug_timestamp_changed(global);
	if (wpa_debug_show_keys != old_show_keys)
		wpas_notify_debug_show_keys_changed(global);
#endif	/* 0 */

	return 0;
}


#ifdef CONFIG_OWE
static int owe_trans_ssid_match(struct wpa_supplicant *wpa_s, const u8 *bssid,
				const u8 *entry_ssid, size_t entry_ssid_len)
{
	const u8 *owe, *pos, *end;
	u8 ssid_len;
	struct wpa_bss *bss;

	/* Check network profile SSID aganst the SSID in the
	 * OWE Transition Mode element. */

	bss = wpa_bss_get_bssid_latest(wpa_s, bssid);
	if (!bss)
		return 0;

	owe = wpa_bss_get_vendor_ie(bss, OWE_IE_VENDOR_TYPE);
	if (!owe)
		return 0;

	pos = owe + 6;
	end = owe + 2 + owe[1];

	if (end - pos < ETH_ALEN + 1)
		return 0;
	pos += ETH_ALEN;
	ssid_len = *pos++;
	if (end - pos < ssid_len || ssid_len > SSID_MAX_LEN)
		return 0;

	return entry_ssid_len == ssid_len &&
		os_memcmp(pos, entry_ssid, ssid_len) == 0;
}
#endif /* CONFIG_OWE */


/**
 * wpa_supplicant_get_ssid - Get a pointer to the current network structure
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: A pointer to the current network structure or %NULL on failure
 */
struct wpa_ssid * wpa_supplicant_get_ssid(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *entry;
	u8 ssid[SSID_MAX_LEN];
	int res;
	size_t ssid_len;
	u8 bssid[ETH_ALEN];
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	int wired = 0;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	res = wpa_drv_get_ssid(wpa_s, ssid);
	if (res < 0) {
		wpa_dbg(wpa_s, MSG_WARNING, "Could not read SSID from "
			"driver");
		return NULL;
	}
	ssid_len = res;

	if (wpa_drv_get_bssid(wpa_s, bssid) < 0) {
		wpa_dbg(wpa_s, MSG_WARNING, "Could not read BSSID from "
			"driver");
		return NULL;
	}

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	wired = wpa_s->conf->ap_scan == 0 &&
		(wpa_s->drv_flags & WPA_DRIVER_FLAGS_WIRED);
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	entry = wpa_s->conf->ssid;
	while (entry) {
		if (!wpas_network_disabled(wpa_s, entry) &&
		    ((ssid_len == entry->ssid_len &&
		      os_memcmp(ssid, entry->ssid, ssid_len) == 0)
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		      || wired
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

			) &&
		    (!entry->bssid_set ||
		     os_memcmp(bssid, entry->bssid, ETH_ALEN) == 0))
			return entry;
#ifdef CONFIG_WPS
		if (!wpas_network_disabled(wpa_s, entry) &&
		    (entry->key_mgmt & WPA_KEY_MGMT_WPS) &&
		    (entry->ssid == NULL || entry->ssid_len == 0) &&
		    (!entry->bssid_set ||
		     os_memcmp(bssid, entry->bssid, ETH_ALEN) == 0))
			return entry;
#endif /* CONFIG_WPS */

#ifdef CONFIG_OWE
		if (!wpas_network_disabled(wpa_s, entry) &&
		    owe_trans_ssid_match(wpa_s, bssid, entry->ssid,
		    entry->ssid_len) &&
		    (!entry->bssid_set ||
		     os_memcmp(bssid, entry->bssid, ETH_ALEN) == 0))
			return entry;
#endif /* CONFIG_OWE */

		if (!wpas_network_disabled(wpa_s, entry) && entry->bssid_set &&
		    entry->ssid_len == 0 &&
		    os_memcmp(bssid, entry->bssid, ETH_ALEN) == 0)
			return entry;

		entry = entry->next;
	}

	return NULL;
}


static int select_driver(struct wpa_supplicant *wpa_s, int i)
{
	struct wpa_global *global = wpa_s->global;

	TX_FUNC_START("          ");

#if 1 /* FC9000 */
	if (wpa_drivers->global_init && global->drv_priv[i] == NULL) {
		global->drv_priv[i] = wpa_drivers->global_init();

		if (global->drv_priv[i] == NULL) {
			wpa_printf(MSG_ERROR, "Failed to initialize driver "
				   "'%s'", __func__, wpa_drivers->name);
			return -1;
		}
	}

	wpa_s->driver = wpa_drivers;
#else
	if (wpa_drivers[i]->global_init && global->drv_priv[i] == NULL) {
		global->drv_priv[i] = wpa_drivers[i]->global_init(global);
		if (global->drv_priv[i] == NULL) {
			da16x_err_prt("Failed to initialize driver "
				   "'%s'", wpa_drivers[i]->name);
			return -1;
		}
	}

	wpa_s->driver = wpa_drivers[i];
#endif /* 1 */
	wpa_s->global_drv_priv = global->drv_priv[i];

	TX_FUNC_END("          ");
	return 0;
}


static int wpa_supplicant_set_driver(struct wpa_supplicant *wpa_s,
				     const char *name)
{
#if 1 /* FC9000 */
	if (wpa_drivers == NULL) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "No driver interfaces build into "
			"wpa_supplicant", __func__);
#else
        da16x_err_prt("[%s] No driver interfaces build into "
            "wpa_supplicant\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	return select_driver(wpa_s, 0);
#else

	int i;
	size_t len;
	const char *pos, *driver = name;



	if (wpa_s == NULL)
		return -1;

	if (wpa_drivers[0] == NULL) {
		wpa_msg(wpa_s, MSG_ERROR, "No driver interfaces build into "
			"wpa_supplicant");
		return -1;
	}

	if (name == NULL) {
		/* default to first driver in the list */
		return select_driver(wpa_s, 0);
	}

	do {
		pos = os_strchr(driver, ',');
		if (pos)
			len = pos - driver;
		else
			len = os_strlen(driver);

		for (i = 0; wpa_drivers[i]; i++) {
			if (os_strlen(wpa_drivers[i]->name) == len &&
			    os_strncmp(driver, wpa_drivers[i]->name, len) ==
			    0) {
				/* First driver that succeeds wins */
				if (select_driver(wpa_s, i) == 0)
					return 0;
			}
		}

		driver = pos + 1;
	} while (pos);

	wpa_msg(wpa_s, MSG_ERROR, "Unsupported driver '%s'", name);

	return -1;
#endif /* FC9000 */
}


/**
 * wpa_supplicant_rx_eapol - Deliver a received EAPOL frame to wpa_supplicant
 * @ctx: Context pointer (wpa_s); this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @src_addr: Source address of the EAPOL frame
 * @buf: EAPOL data starting from the EAPOL header (i.e., no Ethernet header)
 * @len: Length of the EAPOL data
 *
 * This function is called for each received EAPOL frame. Most driver
 * interfaces rely on more generic OS mechanism for receiving frames through
 * l2_packet, but if such a mechanism is not available, the driver wrapper may
 * take care of received EAPOL frames and deliver them to the core supplicant
 * code by calling this function.
 */
void wpa_supplicant_rx_eapol(void *ctx, const u8 *src_addr,
			     const u8 *buf, size_t len)
{
#ifdef	IEEE8021X_EAPOL
	struct wpa_supplicant *wpa_s = ctx;

	wpa_dbg(wpa_s, MSG_DEBUG, "RX EAPOL from " MACSTR, MAC2STR(src_addr));
	wpa_hexdump(MSG_MSGDUMP, "RX EAPOL", buf, len);

#ifdef CONFIG_TESTING_OPTIONS
	if (wpa_s->ignore_auth_resp) {
		wpa_printf(MSG_INFO, "RX EAPOL - ignore_auth_resp active!");
		return;
	}
#endif /* CONFIG_TESTING_OPTIONS */

	if (wpa_s->wpa_state < WPA_ASSOCIATED ||
	    (wpa_s->last_eapol_matches_bssid &&
#ifdef CONFIG_AP
	     !wpa_s->ap_iface &&
#endif /* CONFIG_AP */
	     os_memcmp(src_addr, wpa_s->bssid, ETH_ALEN) != 0)) {
		/*
		 * There is possible race condition between receiving the
		 * association event and the EAPOL frame since they are coming
		 * through different paths from the driver. In order to avoid
		 * issues in trying to process the EAPOL frame before receiving
		 * association information, lets queue it for processing until
		 * the association event is received. This may also be needed in
		 * driver-based roaming case, so also use src_addr != BSSID as a
		 * trigger if we have previously confirmed that the
		 * Authenticator uses BSSID as the src_addr (which is not the
		 * case with wired IEEE 802.1X).
		 */
		wpa_dbg(wpa_s, MSG_DEBUG, "Not associated - Delay processing "
			"of received EAPOL frame (state=%s (%d) bssid=" MACSTR ")",
			wpa_supplicant_state_txt(wpa_s->wpa_state), wpa_s->wpa_state,
			MAC2STR(wpa_s->bssid));
		wpabuf_free(wpa_s->pending_eapol_rx);
		wpa_s->pending_eapol_rx = wpabuf_alloc_copy(buf, len);
		if (wpa_s->pending_eapol_rx) {
			os_get_reltime(&wpa_s->pending_eapol_rx_time);
			os_memcpy(wpa_s->pending_eapol_rx_src, src_addr,
				  ETH_ALEN);
		}
		return;
	}

	wpa_s->last_eapol_matches_bssid =
		os_memcmp(src_addr, wpa_s->bssid, ETH_ALEN) == 0;

#ifdef CONFIG_AP
	if (wpa_s->ap_iface) {
		wpa_supplicant_ap_rx_eapol(wpa_s, src_addr, buf, len);
		return;
	}
#endif /* CONFIG_AP */

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignored received EAPOL frame since "
			"no key management is configured");
		return;
	}

	if (wpa_s->eapol_received == 0 &&
	    (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE) ||
	     !wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt) ||
	     wpa_s->wpa_state != WPA_COMPLETED) &&
	    (wpa_s->current_ssid == NULL ||
	     wpa_s->current_ssid->mode != IEEE80211_MODE_IBSS)) {
		/* Timeout for completing IEEE 802.1X and WPA authentication */
		int timeout = 10;

		if (wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) ||
		    wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA ||
		    wpa_s->key_mgmt == WPA_KEY_MGMT_WPS) {
			/* Use longer timeout for IEEE 802.1X/EAP */
			timeout = 70;
		}

#ifdef CONFIG_WPS
		if (wpa_s->current_ssid && wpa_s->current_bss &&
		    (wpa_s->current_ssid->key_mgmt & WPA_KEY_MGMT_WPS) &&
		    eap_is_wps_pin_enrollee(&wpa_s->current_ssid->eap)) {
			/*
			 * Use shorter timeout if going through WPS AP iteration
			 * for PIN config method with an AP that does not
			 * advertise Selected Registrar.
			 */
			struct wpabuf *wps_ie;

			wps_ie = wpa_bss_get_vendor_ie_multi(
				wpa_s->current_bss, WPS_IE_VENDOR_TYPE);
			if (wps_ie &&
			    !wps_is_addr_authorized(wps_ie, wpa_s->own_addr, 1))
				timeout = 10;
			wpabuf_free(wps_ie);
		}
#endif /* CONFIG_WPS */

		wpa_supplicant_req_auth_timeout(wpa_s, timeout, 0);
	}
	wpa_s->eapol_received++;

	if (wpa_s->countermeasures) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, "WPA: Countermeasures - dropped "
			"EAPOL packet");
#else
        da16x_eapol_prt("[%s] Countermeasures - dropped EAPOL "
                "packet\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return;
	}

#ifdef CONFIG_IBSS_RSN
	if (wpa_s->current_ssid &&
	    wpa_s->current_ssid->mode == WPAS_MODE_IBSS) {
		ibss_rsn_rx_eapol(wpa_s->ibss_rsn, src_addr, buf, len);
		return;
	}
#endif /* CONFIG_IBSS_RSN */

	/* Source address of the incoming EAPOL frame could be compared to the
	 * current BSSID. However, it is possible that a centralized
	 * Authenticator could be using another MAC address than the BSSID of
	 * an AP, so just allow any address to be used for now. The replies are
	 * still sent to the current BSSID (if available), though. */

#ifdef	IEEE8021X_EAPOL
	os_memcpy(wpa_s->last_eapol_src, src_addr, ETH_ALEN);
#endif	/* IEEE8021X_EAPOL */
	if (!wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt) &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_OWE &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_DPP &&
	    eapol_sm_rx_eapol(wpa_s->eapol, src_addr, buf, len) > 0)
		return;
	wpa_drv_poll(wpa_s);
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE))
		wpa_sm_rx_eapol(wpa_s->wpa, src_addr, buf, len);
	else if (wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt)) {
		/*
		 * Set portValid = TRUE here since we are going to skip 4-way
		 * handshake processing which would normally set portValid. We
		 * need this to allow the EAPOL state machines to be completed
		 * without going through EAPOL-Key handshake.
		 */
		eapol_sm_notify_portValid(wpa_s->eapol, TRUE);
	}
#endif	/* IEEE8021X_EAPOL */
}


#if 1 // FC9000
int wpa_supp_update_mac_addr(struct wpa_supplicant *wpa_s)
{
	TX_FUNC_START("        ");

	if (wpa_s->driver->send_eapol) {
		const u8 *addr = wpa_drv_get_mac_addr(wpa_s);

		if (addr)
			os_memcpy(wpa_s->own_addr, addr, ETH_ALEN);
#ifdef	CONFIG_P2P
	} else if (  (!wpa_s->p2p_mgmt || !(wpa_s->drv_flags &
				WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE))
		   && !(wpa_s->drv_flags &
			WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE)) {
		const u8 *addr = wpa_drv_get_mac_addr(wpa_s);

		l2_packet_deinit(wpa_s->l2);

		if (addr)
			os_memcpy(wpa_s->own_addr, addr, ETH_ALEN);

		wpa_s->l2 = l2_packet_init(wpa_s->ifname,
					   wpa_drv_get_mac_addr(wpa_s),
					   ETH_P_EAPOL,
					   wpa_supplicant_rx_eapol, wpa_s, 0);
		if (wpa_s->l2 == NULL)
			return -1;
		if (wpa_supp_ifname_to_ifindex(wpa_s->ifname) == 0)
			wpa_s->l2->ifindex = 0;
		else
			wpa_s->l2->ifindex = 1;
	} else {
		const u8 *addr = wpa_drv_get_mac_addr(wpa_s);

		if (addr)
			os_memcpy(wpa_s->own_addr, addr, ETH_ALEN);
#else
	} else {
		const u8 *addr = wpa_drv_get_mac_addr(wpa_s);

		l2_packet_deinit(wpa_s->l2);

		if (addr)
			os_memcpy(wpa_s->own_addr, addr, ETH_ALEN);

		wpa_s->l2 = l2_packet_init(wpa_s->ifname,
					   wpa_drv_get_mac_addr(wpa_s),
					   ETH_P_EAPOL,
					   wpa_supplicant_rx_eapol, wpa_s, 0);

		if (wpa_s->l2 == NULL)
			return -1;
#endif	/* CONFIG_P2P */
	}

	if (wpa_s->l2 && l2_packet_get_own_addr(wpa_s->l2, wpa_s->own_addr)) {
		da16x_fatal_prt("        [%s] Failed to get own L2 address\n",
						__func__);
		return -1;
	}


	wpa_sm_set_own_addr(wpa_s->wpa, wpa_s->own_addr);

	TX_FUNC_END("        ");

	return 0;
}

#else

int wpa_supp_update_mac_addr(struct wpa_supplicant *wpa_s)
{
	TX_FUNC_START("        ");
#if 0
	if ((!wpa_s->p2p_mgmt ||
	     !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE)) &&
	    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE))
	    {
		l2_packet_deinit(wpa_s->l2);
		wpa_s->l2 = l2_packet_init(wpa_s->ifname,
					   wpa_drv_get_mac_addr(wpa_s),
					   ETH_P_EAPOL,
					   wpa_supplicant_rx_eapol, wpa_s, 0);
		if (wpa_s->l2 == NULL)
			return -1;

		if (l2_packet_set_packet_filter(wpa_s->l2,
						L2_PACKET_FILTER_PKTTYPE))
			wpa_dbg(wpa_s, MSG_DEBUG,
				"Failed to attach pkt_type filter");
	} else
#endif /* 0 */
	{
		const u8 *addr = wpa_drv_get_mac_addr(wpa_s);
		if (addr)
			os_memcpy(wpa_s->own_addr, addr, ETH_ALEN);
	}

	if (wpa_s->l2 && l2_packet_get_own_addr(wpa_s->l2, wpa_s->own_addr)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Failed to get own L2 address");
#else
		da16x_err_prt("[%s] Failed to get own L2 address\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	wpa_sm_set_own_addr(wpa_s->wpa, wpa_s->own_addr);
	TX_FUNC_END("        ");

	return 0;
}
#endif /* 1 */


#ifdef	CONFIG_BRIDGE_IFACE
static void wpa_supplicant_rx_eapol_bridge(void *ctx, const u8 *src_addr,
					   const u8 *buf, size_t len)
{
	struct wpa_supplicant *wpa_s = ctx;
	const struct l2_ethhdr *eth;

	if (len < sizeof(*eth))
		return;
	eth = (const struct l2_ethhdr *) buf;

	if (os_memcmp(eth->h_dest, wpa_s->own_addr, ETH_ALEN) != 0 &&
	    !(eth->h_dest[0] & 0x01)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "RX EAPOL from " MACSTR " to " MACSTR
			" (bridge - not for this interface - ignore)",
			MAC2STR(src_addr), MAC2STR(eth->h_dest));
		return;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "RX EAPOL from " MACSTR " to " MACSTR
		" (bridge)", MAC2STR(src_addr), MAC2STR(eth->h_dest));
	wpa_supplicant_rx_eapol(wpa_s, src_addr, buf + sizeof(*eth),
				len - sizeof(*eth));
}
#endif	/* CONFIG_BRIDGE_IFACE */


/**
 * wpa_supplicant_driver_init - Initialize driver interface parameters
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 on success, -1 on failure
 *
 * This function is called to initialize driver interface parameters.
 * wpa_drv_init() must have been called before this function to initialize the
 * driver interface.
 */
int wpa_supplicant_driver_init(struct wpa_supplicant *wpa_s)
{
	static int interface_count = 0;

	TX_FUNC_START("      ");

	if (wpa_supp_update_mac_addr(wpa_s) < 0) {
		da16x_err_prt("      [%s] Error #1\n", __func__);
		return -1;
	}

	/* for DPM fast booting */
	if (chk_dpm_wakeup() == pdFALSE) {
		if ((os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) &&
		    (   get_run_mode() == AP_ONLY_MODE
#ifdef CONFIG_P2P
			 || get_run_mode() == P2P_ONLY_MODE
			 || get_run_mode() == P2P_GO_FIXED_MODE
#endif /* CONFIG_P2P */
            )
		   ) {
			da16x_iface_prt(">>> MAC address (%s) : " MACSTR "\n",
				       wpa_s->ifname, MAC2STR(wpa_s->own_addr));
		} else {
			da16x_notice_prt(">>> MAC address (%s) : " MACSTR "\n",
					wpa_s->ifname, MAC2STR(wpa_s->own_addr));
		}
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Own MAC address: " MACSTR,
		MAC2STR(wpa_s->own_addr));
#ifdef	__SUPP_27_SUPPORT__
	os_memcpy(wpa_s->perm_addr, wpa_s->own_addr, ETH_ALEN);
#endif	/* __SUPP_27_SUPPORT__ */
	wpa_sm_set_own_addr(wpa_s->wpa, wpa_s->own_addr);

#ifdef	CONFIG_BRIDGE_IFACE
	if (wpa_s->bridge_ifname[0]) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Receiving packets from bridge "
			"interface '%s'", wpa_s->bridge_ifname);
		wpa_s->l2_br = l2_packet_init_bridge(
			wpa_s->bridge_ifname, wpa_s->ifname, wpa_s->own_addr,
			ETH_P_EAPOL, wpa_supplicant_rx_eapol_bridge, wpa_s, 1);
		if (wpa_s->l2_br == NULL) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_ERROR, "Failed to open l2_packet "
				"connection for the bridge interface '%s'",
				wpa_s->bridge_ifname);
#else
			da16x_err_prt("[%s] Failed to open l2_packet "
				"connection for the bridge interface '%s'\n",
				__func__, wpa_s->bridge_ifname);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return -1;
		}
	}
#endif	/* CONFIG_BRIDGE_IFACE */

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (wpa_s->conf->ap_scan == 2 &&
	    os_strcmp(wpa_s->driver->name, "nld11") == 0) {
		wpa_printf(MSG_INFO,
			   "Note: nld11 driver interface is not designed to be used with ap_scan=2; this can result in connection failures");
	}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	wpa_clear_keys(wpa_s, NULL);

	/* Make sure that TKIP countermeasures are not left enabled (could
	 * happen if wpa_supplicant is killed during countermeasures. */
	wpa_drv_set_countermeasures(wpa_s, 0);

	wpa_dbg(wpa_s, MSG_DEBUG, "[%s] RSN: flushing PMKID list in the driver", __func__);
	wpa_drv_flush_pmkid(wpa_s);

	wpa_s->prev_scan_ssid = WILDCARD_SSID_SCAN;
	wpa_s->prev_scan_wildcard = 0;

#if 1 /* FC9000 */
	if (chk_dpm_wakeup() == pdFALSE) {
		if (wpa_supplicant_enabled_networks(wpa_s)) {
#if 0	/* by Shingu 20160706 (P2P) */
			if (   !os_strcmp(wpa_s->ifname, SOFTAP_DEVICE_NAME) == 0
			    || !os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
#else
			if ((os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) &&
			    (   get_run_mode() != AP_ONLY_MODE
#ifdef CONFIG_P2P
			     && get_run_mode() != P2P_ONLY_MODE
			     && get_run_mode() != P2P_GO_FIXED_MODE
#endif /* CONFIG_P2P */
				))
#endif	/* 1 */
			{
				/* First SCAN request */
				if (fast_connection_sleep_flag == pdTRUE) {
					wpa_supplicant_req_scan(wpa_s, interface_count % 3, 30000);
				} else {
					wpa_supplicant_req_scan(wpa_s, interface_count % 3, 100000);
				}
			}

			interface_count++;
		} else {
			wpa_supplicant_set_state(wpa_s, WPA_INACTIVE);
		}
	}
#else

	if (wpa_supplicant_enabled_networks(wpa_s)) {
		if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
			wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
			interface_count = 0;
		}
#ifndef ANDROID
		if (!wpa_s->p2p_mgmt &&
		    wpa_supplicant_delayed_sched_scan(wpa_s,
						      interface_count % 3,
						      100000))
			wpa_supplicant_req_scan(wpa_s, interface_count % 3,
						100000);
#endif /* ANDROID */
		interface_count++;
	} else
		wpa_supplicant_set_state(wpa_s, WPA_INACTIVE);
#endif /* FC9000 */

	TX_FUNC_END("      ");

	return 0;
}


#if 0	/* by Bright : Merge 2.7 */
static int wpa_supplicant_daemon(const char *pid_file)
{
	return os_daemonize(pid_file);
}
#endif	/* 0 */


#if 0 /* FC9000 */
static struct wpa_supplicant * wpa_supplicant_alloc(struct wpa_supplicant *parent)
{
	struct wpa_supplicant *wpa_s;

	wpa_s = os_zalloc(sizeof(*wpa_s));

	if (wpa_s == NULL)
		return NULL;
	wpa_s->scan_req = INITIAL_SCAN_REQ;
	wpa_s->scan_interval = SCAN_TIMER_INTERVAL_SEC;
	wpa_s->new_connection = 1;
	wpa_s->parent = wpa_s;

	return wpa_s;
}


#else

static struct wpa_supplicant *
wpa_supplicant_alloc(struct wpa_supplicant *parent)
{
	struct wpa_supplicant *wpa_s;

	wpa_s = os_zalloc(sizeof(*wpa_s));

	if (wpa_s == NULL)
		return NULL;
	wpa_s->scan_req = INITIAL_SCAN_REQ;
	wpa_s->scan_interval = SCAN_TIMER_INTERVAL_SEC; // 5;
	wpa_s->new_connection = 1;
	wpa_s->parent = wpa_s;
#if defined ( CONFIG_SCHED_SCAN )
	wpa_s->sched_scanning = 0;
#endif	// CONFIG_SCHED_SCAN

#if defined ( __SUPP_27_SUPPORT__ )
	dl_list_init(&wpa_s->bss_tmp_disallowed);
#endif // __SUPP_27_SUPPORT__
#ifdef CONFIG_FILS
	dl_list_init(&wpa_s->fils_hlp_req);
#endif /* CONFIG_FILS */

	return wpa_s;
}
#endif /* 1 */


#ifdef CONFIG_HT_OVERRIDES

static int wpa_set_htcap_mcs(struct wpa_supplicant *wpa_s,
			     struct ieee80211_ht_capabilities *htcaps,
			     struct ieee80211_ht_capabilities *htcaps_mask,
			     const char *ht_mcs)
{
	/* parse ht_mcs into hex array */
	int i;
	const char *tmp = ht_mcs;
	char *end = NULL;
	//[[ trinity_0150605_BEGIN -- tmp
	int err_no = 0;
	//]] trinity_0150605_END -- tmp

	/* If ht_mcs is null, do not set anything */
	if (!ht_mcs)
		return 0;

	/* This is what we are setting in the kernel */
	os_memset(&htcaps->supported_mcs_set, 0, IEEE80211_HT_MCS_MASK_LEN);

	wpa_dbg(wpa_s, MSG_DEBUG, "set_htcap, ht_mcs -:%s:-", ht_mcs);

	for (i = 0; i < IEEE80211_HT_MCS_MASK_LEN; i++) {
		long v;

		//[[ trinity_0150605_BEGIN -- tmp
		//err_no = 0;
		//]] trinity_0150605_END -- tmp
		v = strtol(tmp, &end, 16);

		if (err_no == 0) {
			wpa_dbg(wpa_s, MSG_DEBUG,
				"htcap value[%i]: %ld end: %p  tmp: %p",
				i, v, end, tmp);
			if (end == tmp)
				break;

			htcaps->supported_mcs_set[i] = v;
			tmp = end;
		} else {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_ERROR,
				"Failed to parse ht-mcs: %s, error: %s\n",
				ht_mcs, strerror(err_no));
#else
			da16x_err_prt("[%s] Failed to parse ht-mcs: %s, error: %s\n",
				__func__, ht_mcs, strerror(err_no));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return -1;
		}
	}

	/*
	 * If we were able to parse any values, then set mask for the MCS set.
	 */
	if (i) {
		os_memset(&htcaps_mask->supported_mcs_set, 0xff,
			  IEEE80211_HT_MCS_MASK_LEN - 1);
		/* skip the 3 reserved bits */
		htcaps_mask->supported_mcs_set[IEEE80211_HT_MCS_MASK_LEN - 1] =
			0x1f;
	}

	return 0;
}


static int wpa_disable_max_amsdu(struct wpa_supplicant *wpa_s,
				 struct ieee80211_ht_capabilities *htcaps,
				 struct ieee80211_ht_capabilities *htcaps_mask,
				 int disabled)
{
	le16 msk;

	wpa_dbg(wpa_s, MSG_DEBUG, "set_disable_max_amsdu: %d", disabled);

	if (disabled == -1)
		return 0;

	msk = host_to_le16(HT_CAP_INFO_MAX_AMSDU_SIZE);
	htcaps_mask->ht_capabilities_info |= msk;
	if (disabled)
		htcaps->ht_capabilities_info &= msk;
	else
		htcaps->ht_capabilities_info |= msk;

	return 0;
}


static int wpa_set_ampdu_factor(struct wpa_supplicant *wpa_s,
				struct ieee80211_ht_capabilities *htcaps,
				struct ieee80211_ht_capabilities *htcaps_mask,
				int factor)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "set_ampdu_factor: %d", factor);

	if (factor == -1)
		return 0;

	if (factor < 0 || factor > 3) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "ampdu_factor: %d out of range. "
			"Must be 0-3 or -1", factor);
#else
		da16x_err_prt("[%s] ampdu_factor: %d out of range. "
			"Must be 0-3 or -1\n", __func__, factor);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -EINVAL;
	}

	htcaps_mask->a_mpdu_params |= 0x3; /* 2 bits for factor */
	htcaps->a_mpdu_params &= ~0x3;
	htcaps->a_mpdu_params |= factor & 0x3;

	return 0;
}


static int wpa_set_ampdu_density(struct wpa_supplicant *wpa_s,
				 struct ieee80211_ht_capabilities *htcaps,
				 struct ieee80211_ht_capabilities *htcaps_mask,
				 int density)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "set_ampdu_density: %d", density);

	if (density == -1)
		return 0;

	if (density < 0 || density > 7) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR,
			"ampdu_density: %d out of range. Must be 0-7 or -1.",
			density);
#else
		da16x_err_prt("[%s] ampdu_density: %d out of range. Must be 0-7 or -1.\n",
			__func__, density);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -EINVAL;
	}

	htcaps_mask->a_mpdu_params |= 0x1C;
	htcaps->a_mpdu_params &= ~(0x1C);
	htcaps->a_mpdu_params |= (density << 2) & 0x1C;

	return 0;
}


static int wpa_set_disable_ht40(struct wpa_supplicant *wpa_s,
				struct ieee80211_ht_capabilities *htcaps,
				struct ieee80211_ht_capabilities *htcaps_mask,
				int disabled)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "set_disable_ht40: %d", disabled);

	set_disable_ht40(htcaps, disabled);
	set_disable_ht40(htcaps_mask, 0);

	return 0;
}


static int wpa_set_disable_sgi(struct wpa_supplicant *wpa_s,
			       struct ieee80211_ht_capabilities *htcaps,
			       struct ieee80211_ht_capabilities *htcaps_mask,
			       int disabled)
{
	/* Masking these out disables SGI */
	le16 msk = host_to_le16(HT_CAP_INFO_SHORT_GI20MHZ |
				HT_CAP_INFO_SHORT_GI40MHZ);

	wpa_dbg(wpa_s, MSG_DEBUG, "set_disable_sgi: %d", disabled);

	if (disabled)
		htcaps->ht_capabilities_info &= ~msk;
	else
		htcaps->ht_capabilities_info |= msk;

	htcaps_mask->ht_capabilities_info |= msk;

	return 0;
}


static int wpa_set_disable_ldpc(struct wpa_supplicant *wpa_s,
			       struct ieee80211_ht_capabilities *htcaps,
			       struct ieee80211_ht_capabilities *htcaps_mask,
			       int disabled)
{
	/* Masking these out disables LDPC */
	le16 msk = host_to_le16(HT_CAP_INFO_LDPC_CODING_CAP);

	wpa_dbg(wpa_s, MSG_DEBUG, "set_disable_ldpc: %d", disabled);

	if (disabled)
		htcaps->ht_capabilities_info &= ~msk;
	else
		htcaps->ht_capabilities_info |= msk;

	htcaps_mask->ht_capabilities_info |= msk;

	return 0;
}


void wpa_supplicant_apply_ht_overrides(
	struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
	struct wpa_driver_associate_params *params)
{
	struct ieee80211_ht_capabilities *htcaps;
	struct ieee80211_ht_capabilities *htcaps_mask;

	if (!ssid)
		return;

	params->disable_ht = ssid->disable_ht;
	if (!params->htcaps || !params->htcaps_mask)
		return;

	htcaps = (struct ieee80211_ht_capabilities *) params->htcaps;
	htcaps_mask = (struct ieee80211_ht_capabilities *) params->htcaps_mask;
	wpa_set_htcap_mcs(wpa_s, htcaps, htcaps_mask, ssid->ht_mcs);
	wpa_disable_max_amsdu(wpa_s, htcaps, htcaps_mask,
			      ssid->disable_max_amsdu);
	wpa_set_ampdu_factor(wpa_s, htcaps, htcaps_mask, ssid->ampdu_factor);
	wpa_set_ampdu_density(wpa_s, htcaps, htcaps_mask, ssid->ampdu_density);
	wpa_set_disable_ht40(wpa_s, htcaps, htcaps_mask, ssid->disable_ht40);
	wpa_set_disable_sgi(wpa_s, htcaps, htcaps_mask, ssid->disable_sgi);
	wpa_set_disable_ldpc(wpa_s, htcaps, htcaps_mask, ssid->disable_ldpc);

	if (ssid->ht40_intolerant) {
		le16 bit = host_to_le16(HT_CAP_INFO_40MHZ_INTOLERANT);
		htcaps->ht_capabilities_info |= bit;
		htcaps_mask->ht_capabilities_info |= bit;
	}
}

#endif /* CONFIG_HT_OVERRIDES */


#ifdef CONFIG_VHT_OVERRIDES
void wpa_supplicant_apply_vht_overrides(
	struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
	struct wpa_driver_associate_params *params)
{
	struct ieee80211_vht_capabilities *vhtcaps;
	struct ieee80211_vht_capabilities *vhtcaps_mask;

	if (!ssid)
		return;

	params->disable_vht = ssid->disable_vht;

	vhtcaps = (void *) params->vhtcaps;
	vhtcaps_mask = (void *) params->vhtcaps_mask;

	if (!vhtcaps || !vhtcaps_mask)
		return;

	vhtcaps->vht_capabilities_info = host_to_le32(ssid->vht_capa);
	vhtcaps_mask->vht_capabilities_info = host_to_le32(ssid->vht_capa_mask);

#ifdef CONFIG_HT_OVERRIDES
	/* if max ampdu is <= 3, we have to make the HT cap the same */
	if (ssid->vht_capa_mask & VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX) {
		int max_ampdu;

		max_ampdu = (ssid->vht_capa &
			     VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX) >>
			VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX_SHIFT;

		max_ampdu = max_ampdu < 3 ? max_ampdu : 3;
		wpa_set_ampdu_factor(wpa_s,
				     (void *) params->htcaps,
				     (void *) params->htcaps_mask,
				     max_ampdu);
	}
#endif /* CONFIG_HT_OVERRIDES */

#define OVERRIDE_MCS(i)							\
	if (ssid->vht_tx_mcs_nss_ ##i >= 0) {				\
		vhtcaps_mask->vht_supported_mcs_set.tx_map |=		\
			host_to_le16(3 << 2 * (i - 1));			\
		vhtcaps->vht_supported_mcs_set.tx_map |=		\
			host_to_le16(ssid->vht_tx_mcs_nss_ ##i <<	\
				     2 * (i - 1));			\
	}								\
	if (ssid->vht_rx_mcs_nss_ ##i >= 0) {				\
		vhtcaps_mask->vht_supported_mcs_set.rx_map |=		\
			host_to_le16(3 << 2 * (i - 1));			\
		vhtcaps->vht_supported_mcs_set.rx_map |=		\
			host_to_le16(ssid->vht_rx_mcs_nss_ ##i <<	\
				     2 * (i - 1));			\
	}

	OVERRIDE_MCS(1);
	OVERRIDE_MCS(2);
	OVERRIDE_MCS(3);
	OVERRIDE_MCS(4);
	OVERRIDE_MCS(5);
	OVERRIDE_MCS(6);
	OVERRIDE_MCS(7);
	OVERRIDE_MCS(8);
}
#endif /* CONFIG_VHT_OVERRIDES */


#ifdef PCSC_FUNCS
static int pcsc_reader_init(struct wpa_supplicant *wpa_s)
{
	size_t len;

	if (!wpa_s->conf->pcsc_reader)
		return 0;

	wpa_s->scard = scard_init(wpa_s->conf->pcsc_reader);
	if (!wpa_s->scard)
		return 1;

	if (wpa_s->conf->pcsc_pin &&
	    scard_set_pin(wpa_s->scard, wpa_s->conf->pcsc_pin) < 0) {
		scard_deinit(wpa_s->scard);
		wpa_s->scard = NULL;
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "PC/SC PIN validation failed");
#else
		da16x_err_prt("[%s] PC/SC PIN validation failed\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	len = sizeof(wpa_s->imsi) - 1;
	if (scard_get_imsi(wpa_s->scard, wpa_s->imsi, &len)) {
		scard_deinit(wpa_s->scard);
		wpa_s->scard = NULL;
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Could not read IMSI");
#else
		da16x_err_prt("[%s] Could not read IMSI\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
	wpa_s->imsi[len] = '\0';

	wpa_s->mnc_len = scard_get_mnc_len(wpa_s->scard);

	wpa_printf_dbg(MSG_DEBUG, "SCARD: IMSI %s (MNC length %d)",
		   wpa_s->imsi, wpa_s->mnc_len);

	wpa_sm_set_scard_ctx(wpa_s->wpa, wpa_s->scard);
	eapol_sm_register_scard_ctx(wpa_s->eapol, wpa_s->scard);

	return 0;
}
#endif /* PCSC_FUNCS */


#ifdef CONFIG_EXT_PASSWORD
int wpas_init_ext_pw(struct wpa_supplicant *wpa_s)
{
	char *val, *pos;

	ext_password_deinit(wpa_s->ext_pw);
	wpa_s->ext_pw = NULL;
	eapol_sm_set_ext_pw_ctx(wpa_s->eapol, NULL);

	if (!wpa_s->conf->ext_password_backend)
		return 0;

	val = os_strdup(wpa_s->conf->ext_password_backend);
	if (val == NULL)
		return -1;
	pos = os_strchr(val, ':');
	if (pos)
		*pos++ = '\0';

	da16x_notice_prt("EXT PW: Initialize backend '%s'", val);

	wpa_s->ext_pw = ext_password_init(val, pos);
	os_free(val);
	if (wpa_s->ext_pw == NULL) {
		da16x_notice_prt("EXT PW: Failed to initialize backend");
		return -1;
	}
	eapol_sm_set_ext_pw_ctx(wpa_s->eapol, wpa_s->ext_pw);

	return 0;
}
#endif /* CONFIG_EXT_PASSWORD */


#ifdef CONFIG_FST

static const u8 * wpas_fst_get_bssid_cb(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;

	return (is_zero_ether_addr(wpa_s->bssid) ||
		wpa_s->wpa_state != WPA_COMPLETED) ? NULL : wpa_s->bssid;
}


static void wpas_fst_get_channel_info_cb(void *ctx,
					 enum hostapd_hw_mode *hw_mode,
					 u8 *channel)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->current_bss) {
		*hw_mode = ieee80211_freq_to_chan(wpa_s->current_bss->freq,
						  channel);
	} else if (wpa_s->hw.num_modes) {
		*hw_mode = wpa_s->hw.modes[0].mode;
	} else {
		WPA_ASSERT(0);
		*hw_mode = 0;
	}
}


static int wpas_fst_get_hw_modes(void *ctx, struct hostapd_hw_modes **modes)
{
	struct wpa_supplicant *wpa_s = ctx;

	*modes = wpa_s->hw.modes;
	return wpa_s->hw.num_modes;
}


static void wpas_fst_set_ies_cb(void *ctx, const struct wpabuf *fst_ies)
{
	struct wpa_supplicant *wpa_s = ctx;

	wpa_hexdump_buf(MSG_DEBUG, "FST: Set IEs", fst_ies);
	wpa_s->fst_ies = fst_ies;
}


static int wpas_fst_send_action_cb(void *ctx, const u8 *da, struct wpabuf *data)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (os_memcmp(wpa_s->bssid, da, ETH_ALEN) != 0) {
		da16x_notice_prt("FST:%s:bssid=" MACSTR " != da=" MACSTR,
			   __func__, MAC2STR(wpa_s->bssid), MAC2STR(da));
		return -1;
	}
	return wpa_drv_send_action(wpa_s, wpa_s->assoc_freq, 0, wpa_s->bssid,
				   wpa_s->own_addr, wpa_s->bssid,
				   wpabuf_head(data), wpabuf_len(data),
				   0);
}


static const struct wpabuf * wpas_fst_get_mb_ie_cb(void *ctx, const u8 *addr)
{
	struct wpa_supplicant *wpa_s = ctx;

	WPA_ASSERT(os_memcmp(wpa_s->bssid, addr, ETH_ALEN) == 0);
	return wpa_s->received_mb_ies;
}


static void wpas_fst_update_mb_ie_cb(void *ctx, const u8 *addr,
				     const u8 *buf, size_t size)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct mb_ies_info info;

	WPA_ASSERT(os_memcmp(wpa_s->bssid, addr, ETH_ALEN) == 0);

	if (!mb_ies_info_by_ies(&info, buf, size)) {
		wpabuf_free(wpa_s->received_mb_ies);
		wpa_s->received_mb_ies = mb_ies_by_info(&info);
	}
}


static const u8 * wpas_fst_get_peer_first(void *ctx,
					  struct fst_get_peer_ctx **get_ctx,
					  Boolean mb_only)
{
	struct wpa_supplicant *wpa_s = ctx;

	*get_ctx = NULL;
	if (!is_zero_ether_addr(wpa_s->bssid))
		return (wpa_s->received_mb_ies || !mb_only) ?
			wpa_s->bssid : NULL;
	return NULL;
}


static const u8 * wpas_fst_get_peer_next(void *ctx,
					 struct fst_get_peer_ctx **get_ctx,
					 Boolean mb_only)
{
	return NULL;
}

void fst_wpa_supplicant_fill_iface_obj(struct wpa_supplicant *wpa_s,
				       struct fst_wpa_obj *iface_obj)
{
	iface_obj->ctx              = wpa_s;
	iface_obj->get_bssid        = wpas_fst_get_bssid_cb;
	iface_obj->get_channel_info = wpas_fst_get_channel_info_cb;
	iface_obj->get_hw_modes     = wpas_fst_get_hw_modes;
	iface_obj->set_ies          = wpas_fst_set_ies_cb;
	iface_obj->send_action      = wpas_fst_send_action_cb;
	iface_obj->get_mb_ie        = wpas_fst_get_mb_ie_cb;
	iface_obj->update_mb_ie     = wpas_fst_update_mb_ie_cb;
	iface_obj->get_peer_first   = wpas_fst_get_peer_first;
	iface_obj->get_peer_next    = wpas_fst_get_peer_next;
}
#endif /* CONFIG_FST */

#if 0	/* by Bright : Merge 2.7 */
static int wpas_set_wowlan_triggers(struct wpa_supplicant *wpa_s,
				    const struct wpa_driver_capa *capa)
{
	struct wowlan_triggers *triggers;
	int ret = 0;

	if (!wpa_s->conf->wowlan_triggers)
		return 0;

	triggers = wpa_get_wowlan_triggers(wpa_s->conf->wowlan_triggers, capa);
	if (triggers) {
		ret = wpa_drv_wowlan(wpa_s, triggers);
		os_free(triggers);
	}
	return ret;
}
#endif	/* 0 */

#ifdef	CONFIG_BAND
enum wpa_radio_work_band wpas_freq_to_band(int freq)
{
	if (freq < 3000)
		return BAND_2_4_GHZ;
	if (freq > 50000)
		return BAND_60_GHZ;
	return BAND_5_GHZ;
}
#endif	/* CONFIG_BAND */


#ifdef	CONFIG_BAND
unsigned int wpas_get_bands(struct wpa_supplicant *wpa_s, const int *freqs)
{
	int i;
	unsigned int band = 0;

	if (freqs) {
		/* freqs are specified for the radio work */
		for (i = 0; freqs[i]; i++)
			band |= wpas_freq_to_band(freqs[i]);
	} else {
		/*
		 * freqs are not specified, implies all
		 * the supported freqs by HW
		 */
		for (i = 0; i < wpa_s->hw.num_modes; i++) {
			if (wpa_s->hw.modes[i].num_channels != 0) {
				if (wpa_s->hw.modes[i].mode ==
				    HOSTAPD_MODE_IEEE80211B ||
				    wpa_s->hw.modes[i].mode ==
				    HOSTAPD_MODE_IEEE80211G)
					band |= BAND_2_4_GHZ;
				else if (wpa_s->hw.modes[i].mode ==
					 HOSTAPD_MODE_IEEE80211A)
					band |= BAND_5_GHZ;
				else if (wpa_s->hw.modes[i].mode ==
					 HOSTAPD_MODE_IEEE80211AD)
					band |= BAND_60_GHZ;
				else if (wpa_s->hw.modes[i].mode ==
					 HOSTAPD_MODE_IEEE80211ANY)
					band = BAND_2_4_GHZ | BAND_5_GHZ |
						BAND_60_GHZ;
			}
		}
	}

	return band;
}
#endif	/* CONFIG_BAND */

#if defined ( __SUPP_27_SUPPORT__ )
static struct wpa_radio * radio_add_interface(struct wpa_supplicant *wpa_s,
					      const char *rn)
{
	struct wpa_supplicant *iface = wpa_s->global->ifaces;
	struct wpa_radio *radio;

	while (rn && iface) {
		radio = iface->radio;
		if (radio && os_strcmp(rn, radio->name) == 0) {
			wpa_printf_dbg(MSG_DEBUG, "Add interface %s to existing radio %s",
				   wpa_s->ifname, rn);
			dl_list_add(&radio->ifaces, &wpa_s->radio_list);
			return radio;
		}

		iface = iface->next;
	}

	wpa_printf_dbg(MSG_DEBUG, "Add interface %s to a new radio %s", wpa_s->ifname, rn ? rn : "N/A");
	radio = os_zalloc(sizeof(*radio));
	if (radio == NULL)
		return NULL;

	if (rn)
		os_strlcpy(radio->name, rn, sizeof(radio->name));
	dl_list_init(&radio->ifaces);
	dl_list_init(&radio->work);
	dl_list_add(&radio->ifaces, &wpa_s->radio_list);

	return radio;
}


static void radio_work_free(struct wpa_radio_work *work)
{
#if defined ( CONFIG_SCAN_WORK )
	if (work->wpa_s->scan_work == work) {
		/* This should not really happen. */
		wpa_dbg(work->wpa_s, MSG_INFO, "Freeing radio work '%s'@%p (started=%d) that is marked as scan_work",
			work->type, work, work->started);
		work->wpa_s->scan_work = NULL;
	}
#endif	// CONFIG_SCAN_WORK

#ifdef CONFIG_P2P
	if (work->wpa_s->p2p_scan_work == work) {
		/* This should not really happen. */
		wpa_dbg(work->wpa_s, MSG_INFO, "Freeing radio work '%s'@%p (started=%d) that is marked as p2p_scan_work",
			work->type, work, work->started);
		work->wpa_s->p2p_scan_work = NULL;
	}
#endif /* CONFIG_P2P */

	if (work->started) {
		work->wpa_s->radio->num_active_works--;
		wpa_dbg(work->wpa_s, MSG_DEBUG,
			"radio_work_free('%s'@%p): num_active_works --> %u",
			work->type, work,
			work->wpa_s->radio->num_active_works);
	}

	dl_list_del(&work->list);
	os_free(work);
}


static int radio_work_is_connect(struct wpa_radio_work *work)
{
	return os_strcmp(work->type, "sme-connect") == 0 ||
		os_strcmp(work->type, "connect") == 0;
}


static int radio_work_is_scan(struct wpa_radio_work *work)
{
	return os_strcmp(work->type, "scan") == 0 ||
		os_strcmp(work->type, "p2p-scan") == 0;
}


static struct wpa_radio_work * radio_work_get_next_work(struct wpa_radio *radio)
{
	struct wpa_radio_work *active_work = NULL;
	struct wpa_radio_work *tmp;

	/* Get the active work to know the type and band. */
	dl_list_for_each(tmp, &radio->work, struct wpa_radio_work, list) {
		if (tmp->started) {
			active_work = tmp;
			break;
		}
	}

	if (!active_work) {
		/* No active work, start one */
		radio->num_active_works = 0;
		dl_list_for_each(tmp, &radio->work, struct wpa_radio_work,
				 list) {
			if (os_strcmp(tmp->type, "scan") == 0 &&
			    radio->external_scan_running &&
			    (((struct wpa_driver_scan_params *)
			      tmp->ctx)->only_new_results ||
			     tmp->wpa_s->clear_driver_scan_cache))
				continue;
			return tmp;
		}
		return NULL;
	}

	if (radio_work_is_connect(active_work)) {
		/*
		 * If the active work is either connect or sme-connect,
		 * do not parallelize them with other radio works.
		 */
		wpa_dbg(active_work->wpa_s, MSG_DEBUG,
			"Do not parallelize radio work with %s",
			active_work->type);
		return NULL;
	}

	dl_list_for_each(tmp, &radio->work, struct wpa_radio_work, list) {
		if (tmp->started)
			continue;

		/*
		 * If connect or sme-connect are enqueued, parallelize only
		 * those operations ahead of them in the queue.
		 */
		if (radio_work_is_connect(tmp))
			break;

		/* Serialize parallel scan and p2p_scan operations on the same
		 * interface since the driver_nld11 mechanism for tracking
		 * scan cookies does not yet have support for this. */
		if (active_work->wpa_s == tmp->wpa_s &&
		    radio_work_is_scan(active_work) &&
		    radio_work_is_scan(tmp)) {
			wpa_dbg(active_work->wpa_s, MSG_DEBUG,
				"Do not start work '%s' when another work '%s' is already scheduled",
				tmp->type, active_work->type);
			continue;
		}
		/*
		 * Check that the radio works are distinct and
		 * on different bands.
		 */
		if (os_strcmp(active_work->type, tmp->type) != 0 &&
		    (active_work->bands != tmp->bands)) {
			/*
			 * If a scan has to be scheduled through nld11 scan
			 * interface and if an external scan is already running,
			 * do not schedule the scan since it is likely to get
			 * rejected by kernel.
			 */
			if (os_strcmp(tmp->type, "scan") == 0 &&
			    radio->external_scan_running &&
			    (((struct wpa_driver_scan_params *)
			      tmp->ctx)->only_new_results ||
			     tmp->wpa_s->clear_driver_scan_cache))
				continue;

			wpa_dbg(active_work->wpa_s, MSG_DEBUG,
				"active_work:%s new_work:%s",
				active_work->type, tmp->type);
			return tmp;
		}
	}

	/* Did not find a radio work to schedule in parallel. */
	return NULL;
}


static void radio_start_next_work(void *eloop_ctx, void *timeout_ctx)
{
//	struct wpa_radio *radio = eloop_ctx;
	struct wpa_radio *radio = NULL;
	struct wpa_radio_work *work=NULL;
	struct os_reltime now, diff;
	struct wpa_supplicant *wpa_s=NULL;

	if(eloop_ctx == NULL)
		return;
	
       radio = eloop_ctx;  
	   
	work = dl_list_first(&radio->work, struct wpa_radio_work, list);
	if (work == NULL) {
		radio->num_active_works = 0;
		return;
	}

	wpa_s = dl_list_first(&radio->ifaces, struct wpa_supplicant,
			      radio_list);

	if (!(wpa_s &&
	      wpa_s->drv_flags & WPA_DRIVER_FLAGS_OFFCHANNEL_SIMULTANEOUS)) {
		if (work->started)
			return; /* already started and still in progress */

		if (wpa_s && wpa_s->radio->external_scan_running) {
			da16x_notice_prt("Delay radio work start until externally triggered scan completes");
			return;
		}
	} else {
		work = NULL;
		if (radio->num_active_works < MAX_ACTIVE_WORKS) {
			/* get the work to schedule next */
			work = radio_work_get_next_work(radio);
		}
		if (!work)
			return;
	}

	wpa_s = work->wpa_s;
	os_get_reltime(&now);
	os_reltime_sub(&now, &work->time, &diff);
	wpa_dbg(wpa_s, MSG_DEBUG,
		"Starting radio work '%s'@%p after %ld.%06ld second wait",
		work->type, work, diff.sec, diff.usec);
	work->started = 1;
	work->time = now;
	radio->num_active_works++;

	work->cb(work, 0);

	if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_OFFCHANNEL_SIMULTANEOUS) &&
	    radio->num_active_works < MAX_ACTIVE_WORKS)
		radio_work_check_next(wpa_s);
}


/*
 * This function removes both started and pending radio works running on
 * the provided interface's radio.
 * Prior to the removal of the radio work, its callback (cb) is called with
 * deinit set to be 1. Each work's callback is responsible for clearing its
 * internal data and restoring to a correct state.
 * @wpa_s: wpa_supplicant data
 * @type: type of works to be removed
 * @remove_all: 1 to remove all the works on this radio, 0 to remove only
 * this interface's works.
 */
void radio_remove_works(struct wpa_supplicant *wpa_s,
			const char *type, int remove_all)
{
	struct wpa_radio_work *work, *tmp;
	struct wpa_radio *radio = wpa_s->radio;

	dl_list_for_each_safe(work, tmp, &radio->work, struct wpa_radio_work,
			      list) {
		if (type && os_strcmp(type, work->type) != 0)
			continue;

		/* skip other ifaces' works */
		if (!remove_all && work->wpa_s != wpa_s)
			continue;

		wpa_dbg(wpa_s, MSG_DEBUG, "Remove radio work '%s'@%p%s",
			work->type, work, work->started ? " (started)" : "");
		work->cb(work, 1);
		radio_work_free(work);
	}

	/* in case we removed the started work */
	radio_work_check_next(wpa_s);
}


void radio_remove_pending_work(struct wpa_supplicant *wpa_s, void *ctx)
{
	struct wpa_radio_work *work;
	struct wpa_radio *radio = wpa_s->radio;

	dl_list_for_each(work, &radio->work, struct wpa_radio_work, list) {
		if (work->ctx != ctx)
			continue;
		wpa_dbg(wpa_s, MSG_DEBUG, "Free pending radio work '%s'@%p%s",
			work->type, work, work->started ? " (started)" : "");
		radio_work_free(work);
		break;
	}
}


static void radio_remove_interface(struct wpa_supplicant *wpa_s)
{
	struct wpa_radio *radio = wpa_s->radio;

	if (!radio)
		return;

	da16x_notice_prt("Remove interface %s from radio %s\n", wpa_s->ifname, radio->name);
	dl_list_del(&wpa_s->radio_list);
	radio_remove_works(wpa_s, NULL, 0);
	wpa_s->radio = NULL;
	if (!dl_list_empty(&radio->ifaces))
		return; /* Interfaces remain for this radio */

	da16x_notice_prt("Remove radio %s\n", radio->name);
	da16x_eloop_cancel_timeout(radio_start_next_work, radio, NULL);
	os_free(radio);
}


void radio_work_check_next(struct wpa_supplicant *wpa_s)
{
	struct wpa_radio *radio = wpa_s->radio;

	if (dl_list_empty(&radio->work))
		return;
	if (wpa_s->ext_work_in_progress) {
		da16x_notice_prt("External radio work in progress - delay start of pending item");
		return;
	}
	da16x_eloop_cancel_timeout(radio_start_next_work, radio, NULL);
	da16x_eloop_register_timeout(0, 0, radio_start_next_work, radio, NULL);
}


/**
 * radio_add_work - Add a radio work item
 * @wpa_s: Pointer to wpa_supplicant data
 * @freq: Frequency of the offchannel operation in MHz or 0
 * @type: Unique identifier for each type of work
 * @next: Force as the next work to be executed
 * @cb: Callback function for indicating when radio is available
 * @ctx: Context pointer for the work (work->ctx in cb())
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to request time for an operation that requires
 * exclusive radio control. Once the radio is available, the registered callback
 * function will be called. radio_work_done() must be called once the exclusive
 * radio operation has been completed, so that the radio is freed for other
 * operations. The special case of deinit=1 is used to free the context data
 * during interface removal. That does not allow the callback function to start
 * the radio operation, i.e., it must free any resources allocated for the radio
 * work and return.
 *
 * The @freq parameter can be used to indicate a single channel on which the
 * offchannel operation will occur. This may allow multiple radio work
 * operations to be performed in parallel if they apply for the same channel.
 * Setting this to 0 indicates that the work item may use multiple channels or
 * requires exclusive control of the radio.
 */
int radio_add_work(struct wpa_supplicant *wpa_s, unsigned int freq,
		   const char *type, int next,
		   void (*cb)(struct wpa_radio_work *work, int deinit),
		   void *ctx)
{
	struct wpa_radio *radio = wpa_s->radio;
	struct wpa_radio_work *work;
	int was_empty;

	work = os_zalloc(sizeof(*work));
	if (work == NULL)
		return -1;
	wpa_dbg(wpa_s, MSG_DEBUG, "Add radio work '%s'@%p", type, work);
	os_get_reltime(&work->time);
	work->freq = freq;
	work->type = type;
	work->wpa_s = wpa_s;
	work->cb = cb;
	work->ctx = ctx;

#ifdef	CONFIG_BAND
	if (freq)
		work->bands = wpas_freq_to_band(freq);
	else if (os_strcmp(type, "scan") == 0 ||
		 os_strcmp(type, "p2p-scan") == 0)
		work->bands = wpas_get_bands(wpa_s,
					     ((struct wpa_driver_scan_params *)
					      ctx)->freqs);
	else
		work->bands = wpas_get_bands(wpa_s, NULL);
#endif	/* CONFIG_BAND */

	was_empty = dl_list_empty(&wpa_s->radio->work);

	if (next)
		dl_list_add(&wpa_s->radio->work, &work->list);
	else
		dl_list_add_tail(&wpa_s->radio->work, &work->list);

	if (was_empty) {
		wpa_dbg(wpa_s, MSG_DEBUG, "First radio work item in the queue - schedule start immediately");
		radio_work_check_next(wpa_s);
	} else if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_OFFCHANNEL_SIMULTANEOUS)
		   && radio->num_active_works < MAX_ACTIVE_WORKS) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Try to schedule a radio work (num_active_works=%u)",
			radio->num_active_works);
		radio_work_check_next(wpa_s);
	}

	return 0;
}


#if 1	/* by Bright : Merge 2.7 */
/**
 * radio_work_done - Indicate that a radio work item has been completed
 * @work: Completed work
 *
 * This function is called once the callback function registered with
 * radio_add_work() has completed its work.
 */
void radio_work_done(struct wpa_radio_work *work)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct os_reltime now, diff;
	unsigned int started = work->started;

	os_get_reltime(&now);
	os_reltime_sub(&now, &work->time, &diff);
	wpa_dbg(wpa_s, MSG_DEBUG, "Radio work '%s'@%p %s in %ld.%06ld seconds",
		work->type, work, started ? "done" : "canceled",
		diff.sec, diff.usec);
	radio_work_free(work);
	if (started)
		radio_work_check_next(wpa_s);
}


struct wpa_radio_work *
radio_work_pending(struct wpa_supplicant *wpa_s, const char *type)
{
	struct wpa_radio_work *work;
	struct wpa_radio *radio = wpa_s->radio;

	dl_list_for_each(work, &radio->work, struct wpa_radio_work, list) {
		if (work->wpa_s == wpa_s && os_strcmp(work->type, type) == 0)
			return work;
	}

	return NULL;
}
#endif	/* 1 */
#endif // __SUPP_27_SUPPORT__


static int wpas_init_driver(struct wpa_supplicant *wpa_s,
			    struct wpa_interface *iface)
{
	const char *ifname, *driver;
#ifdef __SUPP_27_SUPPORT__
	const char *rn;
#endif /* __SUPP_27_SUPPORT__ */

	TX_FUNC_START("      ");

	driver = iface->driver;

	/* call OPS global_init() */
	if (wpa_supplicant_set_driver(wpa_s, driver) < 0)
		return -1;

	/* call OPS init2() */
	wpa_s->drv_priv = wpa_drv_init(wpa_s, wpa_s->ifname);
	if (wpa_s->drv_priv == NULL) {
		da16x_fatal_prt("      [%s] Failed to initialize driver interface\n",
						__func__);
		return -1;
	}

	/* call OPS set_param() */
	if (wpa_drv_set_param(wpa_s, wpa_s->conf->driver_param) < 0) {
		da16x_fatal_prt("      [%s] Driver interface rejected "
			"driver_param '%s'\n",
				__func__, wpa_s->conf->driver_param);
		return -1;
	}

	/* not support in fc driver */
	ifname = wpa_drv_get_ifname(wpa_s);
	if (ifname && os_strcmp(ifname, wpa_s->ifname) != 0) {
		da16x_warn_prt("[%s] Driver interface replaced "
			"interface name with '%s'\n", __func__, ifname);

		os_strlcpy(wpa_s->ifname, ifname, sizeof(wpa_s->ifname));
	}

#if defined ( __SUPP_27_SUPPORT__ )
#if 1 //Supp 2.7
	rn = wpa_driver_get_radio_name(wpa_s);
	if (rn && rn[0] == '\0')
		rn = NULL;

	wpa_s->radio = radio_add_interface(wpa_s, rn);
	if (wpa_s->radio == NULL)
		return -1;
#endif
#endif // __SUPP_27_SUPPORT__

	TX_FUNC_END("      ");

	return 0;
}

#if 0 //def CONFIG_MESH
struct wpa_supplicant * wpas_add_meshpoint_interface(struct wpa_supplicant *wpa_s)
{
	struct wpa_interface iface;
	struct wpa_supplicant *softap_wpa_s;
	char ifname[32];
	char force_name[32];
	int ret;
	struct wpa_ssid *ssid_p = NULL;

	os_snprintf(ifname, sizeof(ifname), SOFTAP_DEVICE_NAME);
	da16x_notice_prt(">>> Add MESH Inteface (%s) ...\n", ifname);

	force_name[0] = '\0';
	ret = wpa_drv_if_add(wpa_s, WPA_IF_MESH, ifname, NULL, NULL,
			     force_name, wpa_s->pending_interface_addr, NULL);
	if (ret < 0) {
		da16x_fatal_prt("[%s] Failed to create SoftAP Device interface\n",
				__func__);
		return NULL;
	}
	os_strlcpy(wpa_s->pending_interface_name, ifname,
		   sizeof(wpa_s->pending_interface_name));

	os_memset(&iface, 0, sizeof(iface));
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;
	iface.driver_param = wpa_s->conf->driver_param;

	//softap_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface, NULL);

	if (wpas_mesh_add_interface(wpa_s, ifname, sizeof(ifname)) < 0) {
		da16x_err_prt(">>> DA16X Supplicant Failed to add "
				"MESH Point interface...\n");
		goto supp_init_fail;
	}


	softap_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface, NULL);
	
	if (!softap_wpa_s) {
		da16x_fatal_prt("[%s] Failed to add SoftAP Device interface\n",
				__func__);
		return NULL;
	}
	softap_wpa_s->parent = wpa_s;

	wpa_s->pending_interface_name[0] = '\0';

	for(ssid_p = softap_wpa_s->conf->ssid ; ssid_p ; ssid_p = ssid_p->next)
	{
		if (ssid_p->ssid == NULL || ssid_p->ssid_len == 0) {
			da16x_ap_prt("[%s] No SSID configured for AP mode\n", __func__);
			continue;
		} else if (ssid_p->mode != WPAS_MODE_AP) {
			da16x_ap_prt("[%s] Not AP mode\n", __func__);
			continue;
		}
		break;
	}

	if (ssid_p && wpa_supplicant_create_ap(softap_wpa_s, ssid_p) < 0) {
		da16x_fatal_prt("[%s] Failed to Create AP\n", __func__);
		return NULL; //CONFIG_ACS_TEST
	}

	return softap_wpa_s;
}

#endif /* CONFIG_MESH */

#ifdef CONFIG_AP
struct wpa_supplicant * wpas_add_softap_interface(struct wpa_supplicant *wpa_s)
{
	struct wpa_interface iface;
	struct wpa_supplicant *softap_wpa_s;
	char ifname[32];
	char force_name[32];
	int ret;
	struct wpa_ssid *ssid_p = NULL;

	os_snprintf(ifname, sizeof(ifname), SOFTAP_DEVICE_NAME);
	da16x_notice_prt(">>> Add SoftAP Inteface (%s) ...\n", ifname);

	force_name[0] = '\0';
	ret = wpa_drv_if_add(wpa_s, WPA_IF_AP_BSS, ifname, NULL, NULL,
			     force_name, wpa_s->pending_interface_addr, NULL);
	if (ret < 0) {
		da16x_fatal_prt("[%s] Failed to create SoftAP Device interface\n",
				__func__);
		return NULL;
	}
	os_strlcpy(wpa_s->pending_interface_name, ifname,
		   sizeof(wpa_s->pending_interface_name));

	os_memset(&iface, 0, sizeof(iface));
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;
	iface.driver_param = wpa_s->conf->driver_param;

	softap_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface, NULL);
	if (!softap_wpa_s) {
		da16x_fatal_prt("[%s] Failed to add SoftAP Device interface\n",
				__func__);
		return NULL;
	}
	softap_wpa_s->parent = wpa_s;

	wpa_s->pending_interface_name[0] = '\0';

	for (ssid_p = softap_wpa_s->conf->ssid; ssid_p; ssid_p = ssid_p->next) {
		if (ssid_p->ssid == NULL || ssid_p->ssid_len == 0) {
			da16x_ap_prt("[%s] No SSID configured for AP mode\n", __func__);
			continue;
		} else if (ssid_p->mode != WPAS_MODE_AP) {
			da16x_ap_prt("[%s] Not AP mode\n", __func__);
			continue;
		}
		break;
	}

	if (ssid_p && wpa_supplicant_create_ap(softap_wpa_s, ssid_p) < 0) {
		da16x_fatal_prt("[%s] Failed to Create AP\n", __func__);
		return NULL; //CONFIG_ACS_TEST
	}

	return softap_wpa_s;
}

int wpas_switch_to_softap_interface(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid_p = NULL;

	da16x_ap_prt("\n\n>>> Start SoftAP (%s) ...\n", SOFTAP_DEVICE_NAME);

	os_strlcpy(wpa_s->ifname, SOFTAP_DEVICE_NAME, sizeof(wpa_s->ifname));

	for (ssid_p = wpa_s->conf->ssid ; ssid_p ; ssid_p = ssid_p->next) {
		if (ssid_p->ssid == NULL || ssid_p->ssid_len == 0) {
			da16x_ap_prt("[%s] No SSID configured for AP mode\n", __func__);
			continue;
		} else if (ssid_p->mode != WPAS_MODE_AP) {
			da16x_ap_prt("[%s] Not AP mode\n", __func__);
			continue;
		}
		break;
	}

	if (ssid_p && wpa_supplicant_create_ap(wpa_s, ssid_p) < 0) {
		da16x_fatal_prt("[%s] Failed to Create AP\n", __func__);
		return -1;
	}

	return 0;
}

#endif	/* CONFIG_AP */


#ifdef CONFIG_GAS_SERVER

static void wpas_gas_server_tx_status(struct wpa_supplicant *wpa_s,
				      unsigned int freq, const u8 *dst,
				      const u8 *src, const u8 *bssid,
				      const u8 *data, size_t data_len,
				      enum offchannel_send_action_result result)
{
	wpa_printf_dbg(MSG_DEBUG, "GAS: TX status: freq=%u dst=" MACSTR
		   " result=%s",
		   freq, MAC2STR(dst),
		   result == OFFCHANNEL_SEND_ACTION_SUCCESS ? "SUCCESS" :
		   (result == OFFCHANNEL_SEND_ACTION_NO_ACK ? "no-ACK" :
		    "FAILED"));
	gas_server_tx_status(wpa_s->gas_server, dst, data, data_len,
			     result == OFFCHANNEL_SEND_ACTION_SUCCESS);
}


static void wpas_gas_server_tx(void *ctx, int freq, const u8 *da,
			       struct wpabuf *buf, unsigned int wait_time)
{
	struct wpa_supplicant *wpa_s = ctx;
	const u8 broadcast[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (wait_time > wpa_s->max_remain_on_chan)
		wait_time = wpa_s->max_remain_on_chan;

	offchannel_send_action(wpa_s, freq, da, wpa_s->own_addr, broadcast,
			       wpabuf_head(buf), wpabuf_len(buf),
			       wait_time, wpas_gas_server_tx_status, 0);
}

#endif /* CONFIG_GAS_SERVER */


static int wpa_supplicant_init_iface(struct wpa_supplicant *wpa_s,
			       struct wpa_interface *iface)
{
	struct wpa_driver_capa capa;
	int capa_res;
	//u8 dfs_domain;

	int num_channel = 14;
	int min_chan; //, max_chan;

#if 1 /* supplicant 2.7 */
	wpa_printf_dbg(MSG_DEBUG, "Initializing interface '%s' conf '%s' driver "
		   "'%s' ctrl_interface '%s' bridge '%s'", iface->ifname,
#ifdef UNUSED_CODE
		   iface->confname ? iface->confname :
#endif /* UNUSED_CODE */
		   "N/A",
		   iface->driver ? iface->driver : "default",
		   iface->ctrl_interface ? iface->ctrl_interface : "N/A",
#ifdef CONFIG_BRIDGE_IFACE
		   iface->bridge_ifname ? iface->bridge_ifname :
#endif /* CONFIG_BRIDGE_IFACE */
		   "N/A");

	if ((   get_run_mode() == AP_ONLY_MODE
#ifdef CONFIG_MESH
	     || get_run_mode() == MESH_POINT_MODE
#endif /* CONFIG_MESH */
#ifdef CONFIG_P2P
	     || get_run_mode() == P2P_ONLY_MODE
	     || get_run_mode() == P2P_GO_FIXED_MODE
#endif /* CONFIG_P2P */
		)
		&& os_strcmp(iface->ifname, STA_DEVICE_NAME) == 0) {
	     				       wpa_s->conf = wpa_config_alloc_empty(iface->ctrl_interface,
						    iface->driver_param);
	} else {
		os_strlcpy(wpa_s->ifname, iface->ifname, sizeof(wpa_s->ifname));

#ifdef CONFIG_BACKEND_FILE
		wpa_s->confname = os_rel2abs_path(iface->confname);
		if (wpa_s->confname == NULL) {
			wpa_printf(MSG_ERROR, "Failed to get absolute path "
				   "for configuration file '%s'.",
				   iface->confname);
			return -1;
		}
		wpa_printf_dbg(MSG_DEBUG, "Configuration file '%s' -> '%s'",
			   iface->confname, wpa_s->confname);
#else /* CONFIG_BACKEND_FILE */
#ifdef	UNUSED_CODE
		wpa_s->confname = os_strdup(iface->confname);
#endif	/* UNUSED_CODE */
#endif /* CONFIG_BACKEND_FILE */

#if defined ( __SUPP_27_SUPPORT__ )
		wpa_s->conf = wpa_config_read(wpa_s->confname, wpa_s->ifname);
#else
		wpa_s->conf = wpa_config_read(wpa_s->ifname);
#endif // __SUPP_27_SUPPORT__

		if (wpa_s->conf == NULL) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_printf(MSG_ERROR, "Failed to read or parse "
				   "configuration '%s'.", wpa_s->confname);
#else
			wpa_printf(MSG_ERROR, "Failed to read or parse configuration.");
#endif	/* _CONFIG_DBG_LOG_MSG__ */
			return -1;
		}

#if 0 /* FC9000 */
		wpa_s->confanother = os_rel2abs_path(iface->confanother);
		if (wpa_s->confanother &&
#if 1 /* FC9000 */
		    !wpa_config_read(wpa_s->confanother, NULL))
#else
		    !wpa_config_read(wpa_s->confanother, wpa_s->conf))
#endif /* 1 */
		{
			wpa_printf(MSG_ERROR,
				   "Failed to read or parse configuration '%s'.",
				   wpa_s->confanother);
			return -1;
		}
#endif /* 0 */

		/*
		 * Override ctrl_interface and driver_param if set on command
		 * line.
		 */
		if (iface->ctrl_interface) {
			os_free(wpa_s->conf->ctrl_interface);
			wpa_s->conf->ctrl_interface = os_strdup(iface->ctrl_interface);
		}

		if (iface->driver_param) {
			os_free(wpa_s->conf->driver_param);
			wpa_s->conf->driver_param = os_strdup(iface->driver_param);
		}

#ifdef	CONFIG_P2P
		if (iface->p2p_mgmt && !iface->ctrl_interface) {
			os_free(wpa_s->conf->ctrl_interface);
			wpa_s->conf->ctrl_interface = NULL;
		}
#endif	/* CONFIG_P2P */
	}
#else
	if (iface->ifname == NULL) {
		da16x_fatal_prt("    [%s] Interface name is required\n", __func__);
		return -1;
	}

	os_strlcpy(wpa_s->ifname, iface->ifname, sizeof(wpa_s->ifname));

	if ((get_run_mode() == AP_ONLY_MODE ||
	     get_run_mode() == P2P_ONLY_MODE ||
	     get_run_mode() == P2P_GO_FIXED_MODE) &&
	    os_strcmp(iface->ifname, STA_DEVICE_NAME) == 0) {
		wpa_s->conf = wpa_config_alloc_empty(NULL, NULL);
	} else {
		/* DA16X needs to read the NVRAM before anything else. */
#if 0	/* by Shingu 20160901 (Concurrent) */
		wpa_s->conf = wpa_config_read(NULL);
#else
		wpa_s->conf = wpa_config_read(NULL, wpa_s->ifname);
#endif	/* 0 */
	}
#endif /* suppicant 2.7 */


	if (wpa_s->conf == NULL) {
		wpa_printf(MSG_ERROR, "\nNo configuration found.");
		return -1;
	}

	if (iface->ifname == NULL) {
		wpa_printf(MSG_ERROR, "\nInterface name is required.");
		return -1;
	}
	if (os_strlen(iface->ifname) >= sizeof(wpa_s->ifname)) {
		wpa_printf(MSG_ERROR, "\nToo long interface name '%s'.",
			   iface->ifname);
		return -1;
	}
	os_strlcpy(wpa_s->ifname, iface->ifname, sizeof(wpa_s->ifname));

#ifdef	CONFIG_BRIDGE_IFACE
	if (iface->bridge_ifname) {
		if (os_strlen(iface->bridge_ifname) >=
		    sizeof(wpa_s->bridge_ifname)) {
			wpa_printf(MSG_ERROR, "\nToo long bridge interface "
				   "name '%s'.", iface->bridge_ifname);
			return -1;
		}
		os_strlcpy(wpa_s->bridge_ifname, iface->bridge_ifname,
			   sizeof(wpa_s->bridge_ifname));
	}
#endif	/* CONFIG_BRIDGE_IFACE */

#ifdef	IEEE8021X_EAPOL
	/* RSNA Supplicant Key Management - INITIALIZE */
	eapol_sm_notify_portEnabled(wpa_s->eapol, FALSE);
	eapol_sm_notify_portValid(wpa_s->eapol, FALSE);
#endif	/* IEEE8021X_EAPOL */

	/* Initialize driver interface and register driver event handler before
	 * L2 receive handler so that association events are processed before
	 * EAPOL-Key packets if both become available for the same select()
	 * call. */
	if (wpas_init_driver(wpa_s, iface) < 0) {
		return -1;
	}

	/* for WPA function */
	if (wpa_supplicant_init_wpa(wpa_s) < 0) {
		return -1;
	}

#ifdef	CONFIG_BRIDGE_IFACE
	wpa_sm_set_ifname(wpa_s->wpa, wpa_s->ifname,
			  wpa_s->bridge_ifname[0] ? wpa_s->bridge_ifname :
			  NULL);
#else
	wpa_sm_set_ifname(wpa_s->wpa, wpa_s->ifname);
#endif	/* CONFIG_BRIDGE_IFACE */
#ifdef	CONFIG_FAST_REAUTH
	wpa_sm_set_fast_reauth(wpa_s->wpa, wpa_s->conf->fast_reauth);
#endif	/* CONFIG_FAST_REAUTH */

	if (wpa_s->conf->dot11RSNAConfigPMKLifetime &&
	    wpa_sm_set_param(wpa_s->wpa, RSNA_PMK_LIFETIME,
			     wpa_s->conf->dot11RSNAConfigPMKLifetime)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Invalid WPA parameter value for "
			"dot11RSNAConfigPMKLifetime");
#else
		da16x_err_prt("[%s] Invalid WPA parameter value for "
			"dot11RSNAConfigPMKLifetime\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	if (wpa_s->conf->dot11RSNAConfigPMKReauthThreshold &&
	    wpa_sm_set_param(wpa_s->wpa, RSNA_PMK_REAUTH_THRESHOLD,
			     wpa_s->conf->dot11RSNAConfigPMKReauthThreshold)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Invalid WPA parameter value for "
			"dot11RSNAConfigPMKReauthThreshold");
#else
		da16x_err_prt("[%s] Invalid WPA parameter value for "
			"dot11RSNAConfigPMKReauthThreshold\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

	if (wpa_s->conf->dot11RSNAConfigSATimeout &&
	    wpa_sm_set_param(wpa_s->wpa, RSNA_SA_TIMEOUT,
			     wpa_s->conf->dot11RSNAConfigSATimeout)) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Invalid WPA parameter value for "
			"dot11RSNAConfigSATimeout");
#else
		da16x_err_prt("[%s] Invalid WPA parameter value for "
			"dot11RSNAConfigSATimeout\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}

#ifdef CONFIG_DELAYED_MIC_ERROR_REPORT
	wpa_s->delayed_mic_error_enabled = 1;
#endif /* CONFIG_DELAYED_MIC_ERROR_REPORT */

int wifi_mode = 0;
int set_greenfield = 0;

#ifdef	CONFIG_AP_WIFI_MODE
	/* P2P Only, AP Ony ï¿½ï¿½ï¿½ï¿½ï¿?ï¿½ï¿½ï¿?iface 1 ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ config Read ï¿½ï¿½ï¿½ï¿½. */
	if (get_run_mode() != STA_ONLY_MODE &&
 	    (os_strcmp(iface->ifname, SOFTAP_DEVICE_NAME) == 0
		|| os_strcmp(iface->ifname, P2P_DEVICE_NAME) == 0 
#ifdef CONFIG_MESH
		|| os_strcmp(iface->ifname, MESH_POINT_DEVICE_NAME) == 0
#endif /* CONFIG_MESH */
	     )) {
		if (wpa_s->conf->ssid->wifi_mode >= DA16X_WIFI_MODE_BGN
			&& wpa_s->conf->ssid->wifi_mode < DA16X_WIFI_MODE_MAX) {
			wifi_mode = wpa_s->conf->ssid->wifi_mode;
		}

		//da16x_info_prt("\nwpa_s->conf->greenfield=%d\n\n", wpa_s->conf->greenfield);
		if (wpa_s->conf->greenfield == 0 || wpa_s->conf->greenfield == 1) {
			set_greenfield = wpa_s->conf->greenfield;
		}
	}
#endif	/* CONFIG_AP_WIFI_MODE */
	//[[ trinity_0170113_BEGIN -- hw.modes should be initialized in both STA and AT modes
#if 0
	for (int i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
		const struct country_ch_power_level *field = (void*)cc_power_table(i);

		country_channel_range(field->country, &min_chan, &max_chan);
		if (os_strcasecmp(wpa_s->conf->country, field->country) != 0)
			continue;
		num_channel = max_chan - min_chan + 1;
		break;
	}
#else
	min_chan = 1;
#endif /* 0 */

	wpa_s->hw.modes = fc80211_get_hw_feature_data(wifi_mode,
				&wpa_s->hw.num_modes, &wpa_s->hw.flags, (u16)set_greenfield, min_chan, num_channel, wpa_s->conf->country);

	capa_res = wpa_drv_get_capa(wpa_s, &capa);
	if (capa_res == 0) {
#if 1 /* FC9000 */
		wpa_s->drv_capa_known = 1;
		wpa_s->drv_flags = capa.flags;
		wpa_s->drv_enc = capa.enc;
		wpa_s->probe_resp_offloads = capa.probe_resp_offloads;
		wpa_s->max_scan_ssids = capa.max_scan_ssids;
		wpa_s->max_match_sets = capa.max_match_sets;
		wpa_s->max_remain_on_chan = capa.max_remain_on_chan;
		wpa_s->max_stations = capa.max_stations;
		wpa_s->extended_capa = capa.extended_capa;
		wpa_s->extended_capa_mask = capa.extended_capa_mask;
		wpa_s->extended_capa_len = capa.extended_capa_len;
		wpa_s->num_multichan_concurrent =
			capa.num_multichan_concurrent;

#else
		wpa_s->drv_capa_known = 1;
		wpa_s->drv_flags = capa.flags;
		wpa_s->drv_enc = capa.enc;
		wpa_s->drv_smps_modes = capa.smps_modes;
		wpa_s->drv_rrm_flags = capa.rrm_flags;
		wpa_s->probe_resp_offloads = capa.probe_resp_offloads;
		wpa_s->max_scan_ssids = capa.max_scan_ssids;
		wpa_s->max_sched_scan_ssids = capa.max_sched_scan_ssids;
		wpa_s->max_sched_scan_plans = capa.max_sched_scan_plans;
		wpa_s->max_sched_scan_plan_interval =
			capa.max_sched_scan_plan_interval;
		wpa_s->max_sched_scan_plan_iterations =
			capa.max_sched_scan_plan_iterations;
		wpa_s->sched_scan_supported = capa.sched_scan_supported;
		wpa_s->max_match_sets = capa.max_match_sets;
		wpa_s->max_remain_on_chan = capa.max_remain_on_chan;
		wpa_s->max_stations = capa.max_stations;
		wpa_s->extended_capa = capa.extended_capa;
		wpa_s->extended_capa_mask = capa.extended_capa_mask;
		wpa_s->extended_capa_len = capa.extended_capa_len;
		wpa_s->num_multichan_concurrent =
			capa.num_multichan_concurrent;
		wpa_s->wmm_ac_supported = capa.wmm_ac_supported;

		if (capa.mac_addr_rand_scan_supported)
			wpa_s->mac_addr_rand_supported |= MAC_ADDR_RAND_SCAN;
		if (wpa_s->sched_scan_supported &&
		    capa.mac_addr_rand_sched_scan_supported)
			wpa_s->mac_addr_rand_supported |=
				(MAC_ADDR_RAND_SCHED_SCAN | MAC_ADDR_RAND_PNO);
#endif /* 1 */
	}

	if (wpa_s->max_remain_on_chan == 0)
		wpa_s->max_remain_on_chan = 1000;

#ifdef	CONFIG_P2P
	/*
	 * Only take p2p_mgmt parameters when P2P Device is supported.
	 * Doing it here as it determines whether l2_packet_init() will be done
	 * during wpa_supplicant_driver_init().
	 */
	if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE)
		wpa_s->p2p_mgmt = iface->p2p_mgmt;
	else
		iface->p2p_mgmt = 1;
#endif	/* CONFIG_P2P */

	if (wpa_s->num_multichan_concurrent == 0)
		wpa_s->num_multichan_concurrent = 1;

	if (wpa_supplicant_driver_init(wpa_s) < 0) {
		return -1;
	}

#if 1 /* supplicant 2.7 */
#ifdef CONFIG_TDLS
	if (!iface->p2p_mgmt && wpa_tdls_init(wpa_s->wpa))
		return -1;
#endif /* CONFIG_TDLS */

	if (wpa_s->conf->country[0] && wpa_s->conf->country[1] &&
	    wpa_drv_set_country(wpa_s, wpa_s->conf->country)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Failed to set country");
		return -1;
	}

#ifdef CONFIG_FST
	if (wpa_s->conf->fst_group_id) {
		struct fst_iface_cfg cfg;
		struct fst_wpa_obj iface_obj;

		fst_wpa_supplicant_fill_iface_obj(wpa_s, &iface_obj);
		os_strlcpy(cfg.group_id, wpa_s->conf->fst_group_id,
			   sizeof(cfg.group_id));
		cfg.priority = wpa_s->conf->fst_priority;
		cfg.llt = wpa_s->conf->fst_llt;

		wpa_s->fst = fst_attach(wpa_s->ifname, wpa_s->own_addr,
					&iface_obj, &cfg);
		if (!wpa_s->fst) {
#ifdef	__CONFIG_DBG_LOG_MSG__
			wpa_msg(wpa_s, MSG_ERROR,
				"FST: Cannot attach iface %s to group %s",
				wpa_s->ifname, cfg.group_id);
#else
			da16x_err_prt("[%s] FST: Cannot attach iface %s to group %s\n",
				__func__, wpa_s->ifname, cfg.group_id);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
			return -1;
		}
	}
#endif /* CONFIG_FST */
#endif /* supplicant 2.7 */

	if (wpas_wps_init(wpa_s))
		return -1;

#ifdef CONFIG_GAS_SERVER
	wpa_s->gas_server = gas_server_init(wpa_s, wpas_gas_server_tx);
	if (!wpa_s->gas_server) {
		wpa_printf(MSG_ERROR, "Failed to initialize GAS server");
		return -1;
	}
#endif /* CONFIG_GAS_SERVER */

#ifdef CONFIG_DPP
	if (wpas_dpp_init(wpa_s) < 0)
		return -1;
#endif /* CONFIG_DPP */

	/* Initialize EAPOL */
	if (wpa_supplicant_init_eapol(wpa_s) < 0) {
		return -1;
	}
	wpa_sm_set_eapol(wpa_s->wpa, wpa_s->eapol);

#ifdef CONFIG_GAS
	wpa_s->gas = gas_query_init(wpa_s);
	if (wpa_s->gas == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize GAS query");
		return -1;
	}
#endif /* CONFIG_GAS */

#ifdef CONFIG_P2P
#if 1 // FC9000 Only
	if ((get_run_mode() == P2P_ONLY_MODE
#ifdef	CONFIG_CONCURRENT
	     || get_run_mode() == STA_P2P_CONCURRENT_MODE
#endif	/* CONFIG_CONCURRENT */
	     || get_run_mode() == P2P_GO_FIXED_MODE
		)
		&& wpa_supp_ifname_to_ifindex(wpa_s->ifname) == 1) {
		if (wpas_p2p_init(wpa_s->global, wpa_s) < 0) {
			da16x_err_prt("    [%s] Error wpas_p2p_init()\n",
						__func__);
			return -1;
		}
	}
#else
	if ((!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) ||
	     wpa_s->p2p_mgmt) &&
	    wpas_p2p_init(wpa_s->global, wpa_s) < 0) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_ERROR, "Failed to init P2P");
#else
		da16x_err_prt("[%s] Failed to init P2P\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
		return -1;
	}
#endif  // FC9000 Only
#endif /* CONFIG_P2P */

	if (wpa_bss_init(wpa_s) < 0)
		return -1;

#ifdef __SUPP_27_SUPPORT__

#ifdef CONFIG_PMKSA_CACHE_EXTERNAL
#ifdef CONFIG_MESH
	dl_list_init(&wpa_s->mesh_external_pmksa_cache);
#endif /* CONFIG_MESH */
#endif /* CONFIG_PMKSA_CACHE_EXTERNAL */

#if 0	/* by Bright : Merge 2.7 */
	/*
	 * Set Wake-on-WLAN triggers, if configured.
	 * Note: We don't restore/remove the triggers on shutdown (it doesn't
	 * have effect anyway when the interface is down).
	 */
	if (capa_res == 0 && wpas_set_wowlan_triggers(wpa_s, &capa) < 0)
		return -1;
#endif	/* 0 */

#ifdef CONFIG_EAP_PROXY
	{
		size_t len;
		wpa_s->mnc_len = eapol_sm_get_eap_proxy_imsi(wpa_s->eapol, -1,
						     wpa_s->imsi, &len);
		if (wpa_s->mnc_len > 0) {
			wpa_s->imsi[len] = '\0';
			wpa_printf_dbg(MSG_DEBUG, "eap_proxy: IMSI %s (MNC length %d)",
			   	wpa_s->imsi, wpa_s->mnc_len);
		} else {
			wpa_printf_dbg(MSG_DEBUG, "eap_proxy: IMSI not available");
		}
	}
#endif /* CONFIG_EAP_PROXY */

#ifdef PCSC_FUNCS
	if (pcsc_reader_init(wpa_s) < 0)
		return -1;
#endif /* PCSC_FUNCS */

#ifdef CONFIG_EXT_PASSWORD
	if (wpas_init_ext_pw(wpa_s) < 0)
		return -1;
#endif /* CONFIG_EXT_PASSWORD */

#if 0	/* by Bright : Merge 2.7 */
	wpas_rrm_reset(wpa_s);
#endif	/* 0 */

#if defined ( CONFIG_SCHED_SCAN )
	wpas_sched_scan_plans_set(wpa_s, wpa_s->conf->sched_scan_plans);
#endif	// CONFIG_SCHED_SCAN

#ifdef CONFIG_HS20
	hs20_init(wpa_s);
#endif /* CONFIG_HS20 */
#ifdef CONFIG_MBO
	if (wpa_s->conf->oce) {
		if ((wpa_s->conf->oce & OCE_STA) &&
		    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_OCE_STA))
			wpa_s->enable_oce = OCE_STA;
		if ((wpa_s->conf->oce & OCE_STA_CFON) &&
		    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_OCE_STA_CFON)) {
			/* TODO: Need to add STA-CFON support */
			wpa_printf(MSG_ERROR,
				   "OCE STA-CFON feature is not yet supported");
		}
	}
	wpas_mbo_update_non_pref_chan(wpa_s, wpa_s->conf->non_pref_chan);
#endif /* CONFIG_MBO */

	wpa_supplicant_set_default_scan_ies(wpa_s);
#endif	/* __SUPP_27_SUPPORT__ */

	return 0;
}


static void wpa_supplicant_deinit_iface(struct wpa_supplicant *wpa_s,
					int notify, int terminate)
{

	wpa_s->disconnected = 1;
	if (wpa_s->drv_priv) {
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);

		wpa_drv_set_countermeasures(wpa_s, 0);
		wpa_clear_keys(wpa_s, NULL);
	}

	wpa_supplicant_cleanup(wpa_s);

#if defined ( __SUPP_27_SUPPORT__ )
#if 1 //Supp 2.7
	//wpas_ctrl_radio_work_flush(wpa_s);
	radio_remove_interface(wpa_s);
#endif
#endif // __SUPP_27_SUPPORT__

#ifdef CONFIG_P2P
	if (wpa_s == wpa_s->parent)
		wpas_p2p_disconnect(wpa_s);
	if (wpa_s == wpa_s->global->p2p_init_wpa_s && wpa_s->global->p2p) {
		da16x_p2p_prt("[%s] P2P: Disable P2P since removing "
			"the management interface is being removed\n",
				__func__);

		wpas_p2p_deinit_global(wpa_s->global);
	}
#endif /* CONFIG_P2P */

	if (wpa_s->drv_priv)
		wpa_drv_deinit(wpa_s);

#if 0	/* by Bright : Merge 2.7 */
	if (notify)
		wpas_notify_iface_removed(wpa_s);
#endif	/* 0 */

	if (terminate)
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_TERMINATING);
#else
		da16x_info_prt("[%s] " WPA_EVENT_TERMINATING"\n", __func__);
#endif	/* __CONFIG_DBG_LOG_MSG__ */

	if (wpa_s->ctrl_iface) {
#if 0	/* by Bright : Merge 2.7 */
		wpa_supp_ctrl_iface_deinit(wpa_s->ctrl_iface);
#endif	/* 0 */
		wpa_s->ctrl_iface = NULL;
	}

#ifdef CONFIG_MESH
	if (wpa_s->ifmsh) {
		wpa_supplicant_mesh_iface_deinit(wpa_s, wpa_s->ifmsh);
		wpa_s->ifmsh = NULL;
	}
#endif /* CONFIG_MESH */

	if (wpa_s->conf != NULL) {
		wpa_config_free(wpa_s->conf);
		wpa_s->conf = NULL;
	}

//	os_free(wpa_s->ssids_from_scan_req);

	os_free(wpa_s);
}


#ifdef CONFIG_MATCH_IFACE

/**
 * wpa_supplicant_match_iface - Match an interface description to a name
 * @global: Pointer to global data from wpa_supplicant_init()
 * @ifname: Name of the interface to match
 * Returns: Pointer to the created interface description or %NULL on failure
 */
struct wpa_interface * wpa_supplicant_match_iface(struct wpa_global *global,
						  const char *ifname)
{
	int i;
	struct wpa_interface *iface, *miface;

	for (i = 0; i < global->params.match_iface_count; i++) {
		miface = &global->params.match_ifaces[i];
		if (!miface->ifname ||
		    fnmatch(miface->ifname, ifname, 0) == 0) {
			iface = os_zalloc(sizeof(*iface));
			if (!iface)
				return NULL;
			*iface = *miface;
			iface->ifname = ifname;
			return iface;
		}
	}

	return NULL;
}


/**
 * wpa_supplicant_match_existing - Match existing interfaces
 * @global: Pointer to global data from wpa_supplicant_init()
 * Returns: 0 on success, -1 on failure
 */
static int wpa_supplicant_match_existing(struct wpa_global *global)
{
	struct if_nameindex *ifi, *ifp;
	struct wpa_supplicant *wpa_s;
	struct wpa_interface *iface;

	ifp = if_nameindex();
	if (!ifp) {
		wpa_printf(MSG_ERROR, "if_nameindex: %s", strerror(errno));
		return -1;
	}

	for (ifi = ifp; ifi->if_name; ifi++) {
		wpa_s = wpa_supplicant_get_iface(global, ifi->if_name);
		if (wpa_s)
			continue;
		iface = wpa_supplicant_match_iface(global, ifi->if_name);
		if (iface) {
			wpa_s = wpa_supplicant_add_iface(global, iface, NULL);
			os_free(iface);
			if (wpa_s)
				wpa_s->matched = 1;
		}
	}

	if_freenameindex(ifp);
	return 0;
}

#endif /* CONFIG_MATCH_IFACE */


/**
 * wpa_supplicant_add_iface - Add a new network interface
 * @global: Pointer to global data from wpa_supplicant_init()
 * @iface: Interface configuration options
 * @parent: Parent interface or %NULL to assign new interface as parent
 * Returns: Pointer to the created interface or %NULL on failure
 *
 * This function is used to add new network interfaces for %wpa_supplicant.
 * This can be called before wpa_supplicant_run() to add interfaces before the
 * main event loop has been started. In addition, new interfaces can be added
 * dynamically while %wpa_supplicant is already running. This could happen,
 * e.g., when a hotplug network adapter is inserted.
 */
#if 1 /* FC9000 */
struct wpa_supplicant * wpa_supplicant_add_iface(struct wpa_global *global,
						 struct wpa_interface *iface,
						 struct wpa_supplicant *parent)
{
	struct wpa_supplicant *new_wpa_s;
	struct wpa_interface new_iface;

	if (global == NULL || iface == NULL)
		return NULL;

	new_wpa_s = wpa_supplicant_alloc(NULL);
	if (new_wpa_s == NULL)
		return NULL;

	new_wpa_s->global = global;

	new_iface = *iface;

	if (wpa_supplicant_init_iface(new_wpa_s, &new_iface)) {
		da16x_iface_prt("[%s] Failed to add interface %s\n",
			   	__func__, iface->ifname);
		wpa_supplicant_deinit_iface(new_wpa_s, 0, 0);
		return NULL;
	}

	/* Register new wpa_s on global structure */
	new_wpa_s->next = global->ifaces;
	global->ifaces = new_wpa_s;

	if (chk_dpm_wakeup() == pdFALSE) {
		if ((os_strcmp(new_wpa_s->ifname, STA_DEVICE_NAME) == 0) &&
		    (get_run_mode() == AP_ONLY_MODE
#ifdef	CONFIG_P2P
		     || get_run_mode() == P2P_ONLY_MODE
			 || get_run_mode() == P2P_GO_FIXED_MODE
#endif	/* CONFIG_P2P */
#ifdef	CONFIG_MESH
			 || get_run_mode() == MESH_POINT_MODE
#endif	/* CONFIG_MESH */
			)
		) {
			da16x_iface_prt(">>> %s interface add OK\n", new_wpa_s->ifname);
		} else {
			da16x_notice_prt(">>> %s interface add OK\n", new_wpa_s->ifname);
		}

		/* Set default state : Disconected */
		wpa_supplicant_set_state(new_wpa_s, WPA_DISCONNECTED);
	}

	return new_wpa_s;
}

#else
struct wpa_supplicant * wpa_supplicant_add_iface(struct wpa_global *global,
						 struct wpa_interface *iface,
						 struct wpa_supplicant *parent)
{
	struct wpa_supplicant *wpa_s;
	struct wpa_interface t_iface;
	struct wpa_ssid *ssid;

	if (global == NULL || iface == NULL)
		return NULL;

	wpa_s = wpa_supplicant_alloc(parent);
	if (wpa_s == NULL)
		return NULL;

	wpa_s->global = global;

	t_iface = *iface;

#if 1 /* FC9000 Only */
	if (wpa_supplicant_init_iface(wpa_s, &t_iface)) {
		da16x_iface_prt("  [%s] Failed to add interface %s\n",
			   	__func__, iface->ifname);
		wpa_supplicant_deinit_iface(wpa_s, 0, 0);
		return NULL;
	}

	/* Register wpa_s on global structure */
	wpa_s->next = global->ifaces;
	global->ifaces = wpa_s;

	if (chk_dpm_wakeup() == pdFALSE) {
		if ((os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) &&
		    (get_run_mode() == AP_ONLY_MODE ||
		     get_run_mode() == P2P_ONLY_MODE ||
		     get_run_mode() == P2P_GO_FIXED_MODE)) {
			da16x_iface_prt(">>> %s interface add OK\n",
				       wpa_s->ifname);
		} else {
			da16x_notice_prt(">>> %s interface add OK\n",
					wpa_s->ifname);
		}

		/* Set default state : Disconected */
		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
	}
#else /* FC9000 Only */
	if (global->params.override_driver) {
		wpa_printf_dbg(MSG_DEBUG, "Override interface parameter: driver "
			   "('%s' -> '%s')",
			   iface->driver, global->params.override_driver);
		t_iface.driver = global->params.override_driver;
	}
	if (global->params.override_ctrl_interface) {
		wpa_printf_dbg(MSG_DEBUG, "Override interface parameter: "
			   "ctrl_interface ('%s' -> '%s')",
			   iface->ctrl_interface,
			   global->params.override_ctrl_interface);
		t_iface.ctrl_interface =
			global->params.override_ctrl_interface;
	}
	if (wpa_supplicant_init_iface(wpa_s, &t_iface)) {
		wpa_printf_dbg(MSG_DEBUG, "Failed to add interface %s",
			   iface->ifname);
		wpa_supplicant_deinit_iface(wpa_s, 0, 0);
		return NULL;
	}

#ifdef CONFIG_P2P
	if (iface->p2p_mgmt == 0) {
		/* Notify the control interfaces about new iface */
		if (wpas_notify_iface_added(wpa_s)) {
			wpa_supplicant_deinit_iface(wpa_s, 1, 0);
			return NULL;
		}

		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next)
			wpas_notify_network_added(wpa_s, ssid);
	}
#endif /* CONFIG_P2P */

	/* Register wpa_s on global structure */
	wpa_s->next = global->ifaces;
	global->ifaces = wpa_s;

	if (chk_dpm_wakeup() == pdFALSE) {
		if ((os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) &&
		    (get_run_mode() == AP_ONLY_MODE ||
		     get_run_mode() == P2P_ONLY_MODE ||
		     get_run_mode() == P2P_GO_FIXED_MODE)) {
			da16x_iface_prt(">>> %s interface add OK\n",
				       wpa_s->ifname);
		} else {
			da16x_notice_prt(">>> %s interface add OK\n",
					wpa_s->ifname);
		}

		/* Set default state : Disconected */
		wpa_dbg(wpa_s, MSG_DEBUG, "Added interface %s", wpa_s->ifname);
		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
	}

#ifdef CONFIG_P2P
	if (wpa_s->global->p2p == NULL &&
	    !wpa_s->global->p2p_disabled && !wpa_s->conf->p2p_disabled &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
	    wpas_p2p_add_p2pdev_interface(
		    wpa_s, wpa_s->global->params.conf_p2p_dev) < 0) {
		wpa_printf(MSG_INFO,
			   "P2P: Failed to enable P2P Device interface");
		/* Try to continue without. P2P will be disabled. */
	}
#endif /* CONFIG_P2P */
#endif /* FC9000 Only */

	return wpa_s;
}
#endif /* FC9000 */


/**
 * wpa_supplicant_remove_iface - Remove a network interface
 * @global: Pointer to global data from wpa_supplicant_init()
 * @wpa_s: Pointer to the network interface to be removed
 * Returns: 0 if interface was removed, -1 if interface was not found
 *
 * This function can be used to dynamically remove network interfaces from
 * %wpa_supplicant, e.g., when a hotplug network adapter is ejected. In
 * addition, this function is used to remove all remaining interfaces when
 * %wpa_supplicant is terminated.
 */
int wpa_supplicant_remove_iface(struct wpa_global *global,
				struct wpa_supplicant *wpa_s,
				int terminate)
{
	struct wpa_supplicant *prev;
#ifdef CONFIG_MESH
	unsigned int mesh_if_created = wpa_s->mesh_if_created;
	char *ifname = NULL;
	struct wpa_supplicant *parent = wpa_s->parent;
#endif /* CONFIG_MESH */

	/* Remove interface from the global list of interfaces */
	prev = global->ifaces;
	if (prev == wpa_s) {
		global->ifaces = wpa_s->next;
	} else {
		while (prev && prev->next != wpa_s)
			prev = prev->next;
		if (prev == NULL)
			return -1;
		prev->next = wpa_s->next;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Removing interface %s", wpa_s->ifname);

#ifdef CONFIG_MESH
	if (mesh_if_created) {
		ifname = os_strdup(wpa_s->ifname);
		if (ifname == NULL) {
			wpa_dbg(wpa_s, MSG_ERROR,
				"mesh: Failed to malloc ifname");
			return -1;
		}
	}
#endif /* CONFIG_MESH */

#ifdef	CONFIG_P2P
	if (global->p2p_group_formation == wpa_s)
		global->p2p_group_formation = NULL;

#ifdef	CONFIG_P2P_OPTION
	if (global->p2p_invite_group == wpa_s)
		global->p2p_invite_group = NULL;
#endif	/* CONFIG_P2P_OPTION */
#endif	/* CONFIG_P2P */

	wpa_supplicant_deinit_iface(wpa_s, 1, terminate);

#ifdef CONFIG_MESH
	if (mesh_if_created) {
		wpa_drv_if_remove(parent, WPA_IF_MESH, ifname);
		os_free(ifname);
	}
#endif /* CONFIG_MESH */

	return 0;
}


/**
 * wpa_supplicant_get_eap_mode - Get the current EAP mode
 * @wpa_s: Pointer to the network interface
 * Returns: Pointer to the eap mode or the string "UNKNOWN" if not found
 */
const char * wpa_supplicant_get_eap_mode(struct wpa_supplicant *wpa_s)
{
	const char *eapol_method;

        if (wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) == 0 &&
            wpa_s->key_mgmt != WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		return "NO-EAP";
	}

	eapol_method = eapol_sm_get_method_name(wpa_s->eapol);
	if (eapol_method == NULL)
		return "UNKNOWN-EAP";

	return eapol_method;
}


/**
 * wpa_supplicant_get_iface - Get a new network interface
 * @global: Pointer to global data from wpa_supplicant_init()
 * @ifname: Interface name
 * Returns: Pointer to the interface or %NULL if not found
 */
struct wpa_supplicant * wpa_supplicant_get_iface(struct wpa_global *global,
						 const char *ifname)
{
	struct wpa_supplicant *wpa_s;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp(wpa_s->ifname, ifname) == 0)
			return wpa_s;
	}
	return NULL;
}


#if defined ( __SUPP_27_SUPPORT__ )
#ifndef CONFIG_NO_WPA_MSG
static const char * wpa_supplicant_msg_ifname_cb(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	if (wpa_s == NULL)
		return NULL;
	return wpa_s->ifname;
}
#endif /* CONFIG_NO_WPA_MSG */
#endif /* __SUPP_27_SUPPORT__ */


#ifndef WPA_SUPPLICANT_CLEANUP_INTERVAL
#define WPA_SUPPLICANT_CLEANUP_INTERVAL 10
#endif /* WPA_SUPPLICANT_CLEANUP_INTERVAL */

#ifdef	UNUSED_CODE	/* by Bright : Merge 2.7 */
/* Periodic cleanup tasks */
static void wpas_periodic(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_global *global = eloop_ctx;
	struct wpa_supplicant *wpa_s;

	da16x_eloop_register_timeout(WPA_SUPPLICANT_CLEANUP_INTERVAL, 0,
			       wpas_periodic, global, NULL);

#ifdef CONFIG_P2P
	if (global->p2p)
		p2p_expire_peers(global->p2p);
#endif /* CONFIG_P2P */

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
#ifndef CONFIG_IMMEDIATE_SCAN
		wpa_bss_flush_by_age(wpa_s, wpa_s->conf->bss_expiration_age);
#endif /* CONFIG_IMMEDIATE_SCAN */

#ifdef CONFIG_AP
		ap_periodic(wpa_s);
#endif /* CONFIG_AP */
	}
}
#endif	/* UNUSED_CODE */


extern int da16x_eloop_init(void);
extern void da16x_eloop_run(struct wpa_global *global,
					struct wpa_supplicant *wpa_s);

/**
 * wpa_supplicant_init - Initialize %wpa_supplicant
 * @params: Parameters for %wpa_supplicant
 * Returns: Pointer to global %wpa_supplicant data, or %NULL on failure
 *
 * This function is used to initialize %wpa_supplicant. After successful
 * initialization, the returned data pointer can be used to add and remove
 * network interfaces, and eventually, to deinitialize %wpa_supplicant.
 */
extern int da16x_eloop_init(void);
extern void da16x_eloop_run(struct wpa_global *global,
					struct wpa_supplicant *wpa_s);

struct wpa_global * wpa_supplicant_init(struct wpa_params *params)
{
	struct wpa_global *global;
#ifdef CONFIG_EAP_METHOD
	int ret;
#endif /* CONFIG_EAP_METHOD */
#if 0
	int i;
#endif /* 0 */

	if (params == NULL)
		return NULL;

#ifdef CONFIG_EAP_METHOD
	/* register EAP methods */
	ret = eap_register_methods();
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to register EAP methods");
		if (ret == -2)
			wpa_printf(MSG_ERROR, "Two or more EAP methods used "
				   "the same EAP type.");
		return NULL;
	}
#endif /* CONFIG_EAP_METHOD */

	global = os_zalloc(sizeof(*global));
	if (global == NULL)
		return NULL;

#ifdef	CONFIG_P2P_OPTION
	dl_list_init(&global->p2p_srv_bonjour);
	dl_list_init(&global->p2p_srv_upnp);
#endif	/* CONFIG_P2P_OPTION */

#if 0	/* by Bright : Merge 2.7 */
#if 1 /* supplicant 2.7 */
	global->params.daemonize = params->daemonize;
	global->params.wait_for_monitor = params->wait_for_monitor;
	global->params.dbus_ctrl_interface = params->dbus_ctrl_interface;
	if (params->pid_file)
		global->params.pid_file = os_strdup(params->pid_file);
	if (params->ctrl_interface)
		global->params.ctrl_interface =
			os_strdup(params->ctrl_interface);
	if (params->ctrl_interface_group)
		global->params.ctrl_interface_group =
			os_strdup(params->ctrl_interface_group);
	if (params->override_driver)
		global->params.override_driver =
			os_strdup(params->override_driver);
	if (params->override_ctrl_interface)
		global->params.override_ctrl_interface =
			os_strdup(params->override_ctrl_interface);
#ifdef CONFIG_MATCH_IFACE
	global->params.match_iface_count = params->match_iface_count;
	if (params->match_iface_count) {
		global->params.match_ifaces =
			os_calloc(params->match_iface_count,
				  sizeof(struct wpa_interface));
		os_memcpy(global->params.match_ifaces,
			  params->match_ifaces,
			  params->match_iface_count *
			  sizeof(struct wpa_interface));
	}
#endif /* CONFIG_MATCH_IFACE */
#ifdef CONFIG_P2P
	if (params->conf_p2p_dev)
		global->params.conf_p2p_dev =
			os_strdup(params->conf_p2p_dev);
#endif /* CONFIG_P2P */
	wpa_debug_level = global->params.wpa_debug_level =
		params->wpa_debug_level;
	wpa_debug_show_keys = global->params.wpa_debug_show_keys =
		params->wpa_debug_show_keys;
	wpa_debug_timestamp = global->params.wpa_debug_timestamp =
		params->wpa_debug_timestamp;
#endif /* supplicant 2.7 */
#endif	/* 0 */

	/* for DPM fast booting */
	if (chk_dpm_wakeup() == pdFALSE) {
#ifdef	RELEASE_VER
		da16x_notice_prt(">>> DA16x Supp Ver" VER_NUM RELEASE_VER_STR"\n");
#else
		da16x_notice_prt(">>> DA16x Supp Ver" VER_NUM DEVELOPE_VER_STR"\n");
#endif	/* RELEASE_VER */
	}

	if (da16x_eloop_init()) {
		wpa_printf(MSG_ERROR, "Failed to initialize event loop");
		wpa_supplicant_deinit(global);
		return NULL;
	}

#ifdef __SUPP_27_SUPPORT__
	random_init(params->entropy_file);
#endif /* __SUPP_27_SUPPORT__ */


#if 0 /* FC9000 */
	global->ctrl_iface = wpa_supplicant_global_ctrl_iface_init(global);
#endif /* 0 */

#if 1 /* FC9000 Only ?? */
	global->drv_count = 1;
	global->drv_priv = os_zalloc(global->drv_count * sizeof(void *));
#else /* FC9000 Only ?? */

	if (global->ctrl_iface == NULL) {
		wpa_supplicant_deinit(global);
		return NULL;
	}

	if (wpas_notify_supplicant_initialized(global)) {
		wpa_supplicant_deinit(global);
		return NULL;
	}

	for (i = 0; wpa_drivers[i]; i++)
		global->drv_count++;
	if (global->drv_count == 0) {
		wpa_printf(MSG_ERROR, "No drivers enabled");
		wpa_supplicant_deinit(global);
		return NULL;
	}
	global->drv_priv = os_calloc(global->drv_count, sizeof(void *));
#endif /* FC9000 Only ?? */

	if (global->drv_priv == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize drv_priv");
		wpa_supplicant_deinit(global);
		return NULL;
	}

#ifdef CONFIG_WIFI_DISPLAY
	if (wifi_display_init(global) < 0) {
		wpa_printf(MSG_ERROR, "Failed to initialize Wi-Fi Display");
		wpa_supplicant_deinit(global);
		return NULL;
	}
#endif /* CONFIG_WIFI_DISPLAY */

#if 0	/* by Bright : Merge 2.7 */
	da16x_eloop_register_timeout(WPA_SUPPLICANT_CLEANUP_INTERVAL, 0,
			       wpas_periodic, global, NULL);
#endif	/* 0 */

	return global;
}


extern int wpa_supp_dpm_restore_key_info(struct wpa_supplicant *wpa_s);
#undef DPM_RECONNECT_WO_SCAN
#if defined(DPM_RECONNECT_WO_SCAN) && defined(CONFIG_FAST_RECONNECT_V2)
extern void dpm_supplicant_done(unsigned int *freq, unsigned short *cap);
bool is_dpm_fast_reconnect(void)
{
	if (chk_dpm_wakeup()) {
		if( da16x_is_fast_reconnect && (fast_reconnect_tries == 1  || fast_reconnect_tries == 2 )) return 1;
	}
	return 0;
}
#else
extern void dpm_supplicant_done(unsigned int *freq, unsigned short *cap);
//extern void dpm_supplicant_done(void);
#endif

UINT supplicant_done_flag = 0;
void supplicant_done(void)
{
	supplicant_done_flag = 1;
}

UINT is_supplicant_done(void)
{
	return supplicant_done_flag;
}


/**
 * wpa_supplicant_run - Run the %wpa_supplicant main event loop
 * @global: Pointer to global data from wpa_supplicant_init()
 * Returns: 0 after successful event loop run, -1 on failure
 *
 * This function starts the main event loop and continues running as long as
 * there are any remaining events. In most cases, this function is running as
 * long as the %wpa_supplicant process in still in use.
 */
int wpa_supplicant_run(struct wpa_global *global, struct wpa_supplicant *wpa_s)
{
	TX_FUNC_START("  ");
#if 0 // for debug
		PRINTF("[%s] supp_run: dpm_id=%d, ssid_id=%d, net_s(%d)\n", 
					__func__, dpm_supp_conn_info->id, wpa_s->conf->ssid->id, wifi_netif_status(0));
#endif
	if (chk_dpm_wakeup() == pdTRUE && dpm_supp_conn_info->id == wpa_s->conf->ssid->id) {
		wpa_supp_dpm_restore_conn_info(wpa_s);
		wpa_supp_dpm_restore_key_info(wpa_s);
	}

	if (chk_dpm_wakeup() == pdTRUE) {
 		/* for DPM Reconnection without scan */
//#if defined(DPM_RECONNECT_WO_SCAN) && defined(CONFIG_FAST_RECONNECT_V2)
#if 1 //20171205_trinity, "cli status" ssid print fix
		struct wpa_bss *cur_bss;
		unsigned int freq;
		unsigned short cap;

		size_t size =  sizeof(*cur_bss);

		dpm_supplicant_done(&freq, &cap);

		//da16x_notice_prt("[%s] Fill the current bss +++\n", __func__);
		cur_bss = os_zalloc(size);
		if (cur_bss == NULL)
			return NULL;

		/* Fill the SSID  */
		os_memcpy(cur_bss->ssid, wpa_s->conf->ssid->ssid, wpa_s->conf->ssid->ssid_len);
		cur_bss->ssid_len = wpa_s->conf->ssid->ssid_len;

		/* Fill the bssid */
		os_memcpy(cur_bss->bssid, wpa_s->bssid, ETH_ALEN);

		/* Fill the freq */
		cur_bss->freq = freq;
		/* Fill the capability */
		cur_bss->caps = cap;

		//dl_list_add_tail(&wpa_s->bss, &cur_bss->list);
		//dl_list_add_tail(&wpa_s->bss_id, &cur_bss->list_id);
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(cur_bss->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = cur_bss;

		wpa_s->assoc_freq = freq;
#ifdef CONFIG_RECONNECT_OPTIMIZE
		wpa_s->reassoc_freq = freq;
#endif
		//da16x_notice_prt("[%s] Fill the current bss ---\n", __func__);
#else
		unsigned int freq;
		unsigned short cap;

		dpm_supplicant_done(&freq, &cap);

		wpa_s->assoc_freq = freq;
		wpa_s->reassoc_freq = freq;
		//da16x_notice_prt("[%s] Restore assoc frequency : %d\n", __func__, wpa_s->assoc_freq);

#endif
	}

#if 1 /* Not SoftAP ACS */
//	if (!(wpa_s->conf->ssid->mode == WPAS_MODE_AP && wpa_s->conf->ssid->frequency== 0)) {
		//PRINTF("[%s] mode=%d\n", __func__, wpa_s->conf->ssid->mode);
		supplicant_done();
//	}
#endif /* Not SoftAP ACS */

	da16x_eloop_run(global, wpa_s);

	TX_FUNC_END("  ");

	return 0;
}


/**
 * wpa_supplicant_deinit - Deinitialize %wpa_supplicant
 * @global: Pointer to global data from wpa_supplicant_init()
 *
 * This function is called to deinitialize %wpa_supplicant and to free all
 * allocated resources. Remaining network interfaces will also be removed.
 */
void wpa_supplicant_deinit(struct wpa_global *global)
{
#if 0
	int i;
#endif /* 0 */

	if (global == NULL)
		return;

#if 0	/* by Bright : Merge 2.7 */
	da16x_eloop_cancel_timeout(wpas_periodic, global, NULL);
#endif	/* 0 */

#ifdef CONFIG_WIFI_DISPLAY
	wifi_display_deinit(global);
#endif /* CONFIG_WIFI_DISPLAY */

	while (global->ifaces)
		wpa_supplicant_remove_iface(global, global->ifaces, 1);

#if 0	/* by Bright : For WPA3 : Don't need control-interface on FC9000's supplicant */
	if (global->ctrl_iface)
		wpa_supplicant_global_ctrl_iface_deinit(global->ctrl_iface);

	wpas_notify_supplicant_deinitialized(global);
#endif	/* 0 */

#ifdef	CONFIG_EAP_METHOD
	eap_peer_unregister_methods();
#ifdef CONFIG_AP
#ifdef	CONFIG_EAP_SERVER
	eap_server_unregister_methods();
#endif	/* CONFIG_EAP_SERVER */
#endif /* CONFIG_AP */
#endif	/* CONFIG_EAP_METHOD */

#if 1 /* FC9000 */
	wpa_drivers->global_deinit(global->drv_priv[0]);
#else
	for (i = 0; wpa_drivers[i] && global->drv_priv; i++) {
		if (!global->drv_priv[i])
			continue;
		wpa_drivers[i]->global_deinit(global->drv_priv[i]);
	}
#endif /* FC9000 */

	os_free(global->drv_priv);

	random_deinit();

#if 0
	eloop_destroy();
#endif /* 0 */

#if 0	/* by Bright : For WPA3 : Don't need control-interface on FC9000's supplicant */
	if (global->params.pid_file) {
		os_daemonize_terminate(global->params.pid_file);
		os_free(global->params.pid_file);
	}
#endif	/* 0 */
	os_free(global->params.ctrl_interface);
#ifdef CONFIG_OPENSSL_MOD
	os_free(global->params.ctrl_interface_group);
	os_free(global->params.override_driver);
	os_free(global->params.override_ctrl_interface);
#endif /* CONFIG_OPENSSL_MOD */

#ifdef CONFIG_MATCH_IFACE
	os_free(global->params.match_ifaces);
#endif /* CONFIG_MATCH_IFACE */
#ifdef CONFIG_P2P
	os_free(global->params.conf_p2p_dev);
#endif /* CONFIG_P2P */

#ifdef	CONFIG_P2P_UNUSED_CMD
	os_free(global->p2p_disallow_freq.range);
	os_free(global->p2p_go_avoid_freq.range);
#endif	/* CONFIG_P2P_UNUSED_CMD */
	os_free(global->add_psk);

	os_free(global);
#if 0
	wpa_debug_close_syslog();
	wpa_debug_close_file();
	wpa_debug_close_linux_tracing();
#endif /* 0 */
	supplicant_done_flag = 0;

	// for get_cur_connected_bssid() API
    os_memset(cur_connected_bssid, 0, ETH_ALEN);
}


void wpa_supplicant_update_config(struct wpa_supplicant *wpa_s)
{
	if ((wpa_s->conf->changed_parameters & CFG_CHANGED_COUNTRY) &&
	    wpa_s->conf->country[0] && wpa_s->conf->country[1]) {
		char country[3];
		country[0] = wpa_s->conf->country[0];
		country[1] = wpa_s->conf->country[1];
		country[2] = '\0';
		if (wpa_drv_set_country(wpa_s, country) < 0) {
			wpa_printf(MSG_ERROR, "Failed to set country code "
				   "'%s'", country);
		}
	}

#ifdef CONFIG_P2P_UNUSED_CMD
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_EXT_PW_BACKEND)
		wpas_init_ext_pw(wpa_s);
#endif /* CONFIG_P2P_UNUSED_CMD */

#if defined ( CONFIG_SCHED_SCAN )
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_SCHED_SCAN_PLANS)
		wpas_sched_scan_plans_set(wpa_s, wpa_s->conf->sched_scan_plans);
#endif	// CONFIG_SCHED_SCAN

#if 0	/* by Bright : Merge 2.7 */
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_WOWLAN_TRIGGERS) {
		struct wpa_driver_capa capa;
		int res = wpa_drv_get_capa(wpa_s, &capa);

		if (res == 0 && wpas_set_wowlan_triggers(wpa_s, &capa) < 0) {
			wpa_printf(MSG_ERROR,
				   "Failed to update wowlan_triggers to '%s'",
				   wpa_s->conf->wowlan_triggers);
		}
	}
#endif	/* 0 */

#ifdef CONFIG_WPS
	wpas_wps_update_config(wpa_s);
#endif /* CONFIG_WPS */

#ifdef	CONFIG_P2P
	wpas_p2p_update_config(wpa_s);
#endif /* CONFIG_P2P */

	wpa_s->conf->changed_parameters = 0;

	TX_FUNC_END("");
}


void add_freq(int *freqs, int *num_freqs, int freq)
{
	int i;

	for (i = 0; i < *num_freqs; i++) {
		if (freqs[i] == freq)
			return;
	}

	freqs[*num_freqs] = freq;
	(*num_freqs)++;
}


static int * get_bss_freqs_in_ess(struct wpa_supplicant *wpa_s)
{
	struct wpa_bss *bss, *cbss;
	const int max_freqs = 10;
	int *freqs;
	int num_freqs = 0;

	freqs = os_calloc(max_freqs + 1, sizeof(int));
	if (freqs == NULL)
		return NULL;

	cbss = wpa_s->current_bss;

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		if (bss == cbss)
			continue;
		if (bss->ssid_len == cbss->ssid_len &&
		    os_memcmp(bss->ssid, cbss->ssid, bss->ssid_len) == 0 &&
		    wpa_blacklist_get(wpa_s, bss->bssid) == NULL) {
			add_freq(freqs, &num_freqs, bss->freq);
			if (num_freqs == max_freqs)
				break;
		}
	}

	if (num_freqs == 0) {
		os_free(freqs);
		freqs = NULL;
	}

	return freqs;
}


void wpas_connection_failed(struct wpa_supplicant *wpa_s, const u8 *bssid)
{
	int timeout;
	int count = 1;
	int *freqs = NULL;
#ifdef CONFIG_FAST_RECONNECT_V2
	struct wpa_bss *fast_reconnect = NULL;
	struct wpa_ssid *fast_reconnect_ssid = NULL;
#endif /* CONFIG_FAST_RECONNECT_V2 */

	wpas_connect_work_done(wpa_s);

	/*
	 * Remove possible authentication timeout since the connection failed.
	 */
	da16x_eloop_cancel_timeout(wpa_supplicant_timeout, wpa_s, NULL);

	/*
	 * There is no point in blacklisting the AP if this event is
	 * generated based on local request to disconnect.
	 */
	if (wpa_s->own_disconnect_req) {
		wpa_s->own_disconnect_req = 0;
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Ignore connection failure due to local request to disconnect");
		return;
	}
	if (wpa_s->disconnected) {
		/*
		 * There is no point in blacklisting the AP if this event is
		 * generated based on local request to disconnect.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignore connection failure "
			"indication since interface has been put into "
			"disconnected state");
		return;
	}

#ifdef CONFIG_FAST_RECONNECT_V2
	fast_reconnect = wpa_s->current_bss;
	fast_reconnect_ssid = wpa_s->current_ssid;
#endif /* CONFIG_FAST_RECONNECT */

	/*
	 * Add the failed BSSID into the blacklist and speed up next scan
	 * attempt if there could be other APs that could accept association.
	 * The current blacklist count indicates how many times we have tried
	 * connecting to this AP and multiple attempts mean that other APs are
	 * either not available or has already been tried, so that we can start
	 * increasing the delay here to avoid constant scanning.
	 */
#ifdef AUTO_WEPKEY_INDEX
	if (wpa_s->conf->ssid->key_mgmt == WPA_KEY_MGMT_NONE
		&& wpa_s->conf->ssid->wep_key_len[0]) {

		if (wpa_s->conf->ssid->wep_tx_keyidx < 3) {
			wpa_s->conf->ssid->wep_tx_keyidx += 1;
		} else {
			wpa_s->conf->ssid->wep_tx_keyidx = 0;
		}

		da16x_assoc_prt("[%s]Changed WEP_KEY_INDEX=%d\n", __func__, wpa_s->conf->ssid->wep_tx_keyidx);
	}
#endif /* AUTO_WEPKEY_INDEX */
#ifdef CONFIG_RECONNECT_OPTIMIZE
	if(chk_dpm_wakeup() == pdFALSE &&
		!(wpa_s->reassoc_freq && wpa_s->reassoc_try <= 3))
		count = wpa_blacklist_add(wpa_s, bssid);

	if (count == 1 && wpa_s->current_bss) {
		/*
		 * This BSS was not in the blacklist before. If there is
		 * another BSS available for the same ESS, we should try that
		 * next. Otherwise, we may as well try this one once more
		 * before allowing other, likely worse, ESSes to be considered.
		 */
		freqs = get_bss_freqs_in_ess(wpa_s);
		if (freqs) {
			wpa_dbg(wpa_s, MSG_DEBUG, "Another BSS in this ESS "
				"has been seen; try it next");
			wpa_blacklist_add(wpa_s, bssid);
			/*
			 * On the next scan, go through only the known channels
			 * used in this ESS based on previous scans to speed up
			 * common load balancing use case.
			 */
			os_free(wpa_s->next_scan_freqs);
			wpa_s->next_scan_freqs = freqs;
		}
	}

	/*
	 * Add previous failure count in case the temporary blacklist was
	 * cleared due to no other BSSes being available.
	 */
	count += wpa_s->extra_blacklist_count;

	if (count > 3 && wpa_s->current_ssid) {
		wpa_printf_dbg(MSG_DEBUG, "Continuous association failures - "
			   "consider temporary network disabling");
		wpas_auth_failed(wpa_s, "CONN_FAILED");

		/* Notify atcmd host of Wi-Fi connection status */
		extern int wifi_conn_fail_reason_code;
		extern void wifi_conn_fail_noti_to_atcmd_host(void);
		wifi_conn_fail_reason_code = WLAN_REASON_TIMEOUT;
		wifi_conn_fail_noti_to_atcmd_host();

		/* Notify Wi-Fi connection status on user callback function */
		if (wifi_conn_fail_notify_cb != NULL) {
			wifi_conn_fail_notify_cb(WLAN_REASON_TIMEOUT);
		}
	}

	switch (count) {
#if 1 // supplicant 2.7
	case 1:
		timeout = 100;
		break;
	case 2:
		timeout = 500;
		break;
#else
	case 1 :
	case 2 :
#endif
	case 3:
		timeout = 1000;
		break;
	case 4:
		timeout = 5000;
		break;
	default:
		timeout = 10000;
		break;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Blacklist count %d --> request scan in %d "
		"ms", count, timeout);
#endif /* CONFIG_RECONNECT_OPTIMIZE */
	/*
	 * TODO: if more than one possible AP is available in scan results,
	 * could try the other ones before requesting a new scan.
	 */
#ifdef CONFIG_FAST_RECONNECT_V2
	if (fast_reconnect && chk_dpm_wakeup()/*&& !wpas_network_disabled(wpa_s, fast_reconnect_ssid) &&
		!wpas_temp_disabled(wpa_s, fast_reconnect_ssid)*/
		&& fast_reconnect_tries < 2) {
			fast_reconnect_tries++;
			da16x_notice_prt("[%s] Try to reconnect to the same BSS without Scan(tries=%d)\n", __func__, fast_reconnect_tries);
			wpa_s->reassociate = 1;
			da16x_is_fast_reconnect = 1;
		if (wpa_supplicant_connect(wpa_s, fast_reconnect, fast_reconnect_ssid) < 0) {
			/* Recover through full scan */
			da16x_is_fast_reconnect = 0;
			wpa_supplicant_req_scan(wpa_s, timeout / 1000, 1000 * (timeout % 1000));
		}
	} else {
		da16x_notice_prt("[%s] Not a DPM wakeup. Request scan. \n", __func__);
		wpa_supplicant_req_scan(wpa_s, timeout / 1000,
					1000 * (timeout % 1000));
	}
#else

#ifdef CONFIG_RECONNECT_OPTIMIZE
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */

		if (wpa_s->reassoc_freq && wpa_s->reassoc_try <= 3) {
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		} else {
#if 1 /* by Shingu 20200515 */
#ifdef	CONFIG_P2P	// Check by Bright
#if 0	// Original by Shingu debugging : for JIRA #132
			if (wpa_s->global->p2p)
				return;
#else
			if (   wpa_s->global->p2p
				&& get_run_mode() == P2P_ONLY_MODE)
				return;
#endif	/* 0 */
#endif	/* CONFIG_P2P */
#endif
			wpa_supplicant_req_scan(wpa_s, timeout / 1000,
						1000 * (timeout % 1000));
		}
#else
	wpa_supplicant_req_scan(wpa_s, timeout / 1000,
				1000 * (timeout % 1000));
#endif  /* CONFIG_RECONNECT_OPTIMIZE */
#endif  /* CONFIG_FAST_RECONNECT_V2 */
}


int wpas_driver_bss_selection(struct wpa_supplicant *wpa_s)
{
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	return wpa_s->conf->ap_scan == 2 ||
		(wpa_s->drv_flags & WPA_DRIVER_FLAGS_BSS_SELECTION);
#else
	return (wpa_s->drv_flags & WPA_DRIVER_FLAGS_BSS_SELECTION);
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
}


#ifdef	CONFIG_INTERWORKING
#if defined(CONFIG_CTRL_IFACE) || defined(CONFIG_CTRL_IFACE_DBUS_NEW)
int wpa_supplicant_ctrl_iface_ctrl_rsp_handle(struct wpa_supplicant *wpa_s,
					      struct wpa_ssid *ssid,
					      const char *field,
					      const char *value)
{
#ifdef IEEE8021X_EAPOL
	struct eap_peer_config *eap = &ssid->eap;

	wpa_printf_dbg(MSG_DEBUG, "CTRL_IFACE: response handle field=%s", field);
	wpa_hexdump_ascii_key(MSG_DEBUG, "CTRL_IFACE: response value",
			      (const u8 *) value, os_strlen(value));

	switch (wpa_supplicant_ctrl_req_from_string(field)) {
	case WPA_CTRL_REQ_EAP_IDENTITY:
		os_free(eap->identity);
		eap->identity = (u8 *) os_strdup(value);
		eap->identity_len = os_strlen(value);
		eap->pending_req_identity = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
		break;
	case WPA_CTRL_REQ_EAP_PASSWORD:
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(eap->password, eap->password_len);
#else
		os_free(eap->password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		eap->password = (u8 *) os_strdup(value);
		eap->password_len = os_strlen(value);
		eap->pending_req_password = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
		break;
	case WPA_CTRL_REQ_EAP_NEW_PASSWORD:
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(eap->new_password, eap->new_password_len);
#else
		os_free(eap->new_password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		eap->new_password = (u8 *) os_strdup(value);
		eap->new_password_len = os_strlen(value);
		eap->pending_req_new_password = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
		break;
	case WPA_CTRL_REQ_EAP_PIN:
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(eap->pin);
#else
		os_free(eap->pin);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		eap->pin = os_strdup(value);
		eap->pending_req_pin = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
		break;
	case WPA_CTRL_REQ_EAP_OTP:
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(eap->otp, eap->otp_len);
#else
		os_free(eap->otp);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		eap->otp = (u8 *) os_strdup(value);
		eap->otp_len = os_strlen(value);
		os_free(eap->pending_req_otp);
		eap->pending_req_otp = NULL;
		eap->pending_req_otp_len = 0;
		break;
	case WPA_CTRL_REQ_EAP_PASSPHRASE:
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(eap->private_key_passwd);
#else
		os_free(eap->private_key_passwd);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		eap->private_key_passwd = os_strdup(value);
		eap->pending_req_passphrase = 0;
		if (ssid == wpa_s->current_ssid)
			wpa_s->reassociate = 1;
		break;
#ifdef	CONFIG_EXT_SIM
	case WPA_CTRL_REQ_SIM:
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(eap->external_sim_resp);
#else
		os_free(eap->external_sim_resp);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		eap->external_sim_resp = os_strdup(value);
		eap->pending_req_sim = 0;
		break;
#endif /* CONFIG_EXT_SIM */
	case WPA_CTRL_REQ_PSK_PASSPHRASE:
		if (wpa_config_set(ssid, "psk", value) < 0)
			return -1;
		ssid->mem_only_psk = 1;
		if (ssid->passphrase)
			wpa_config_update_psk(ssid);
		if (wpa_s->wpa_state == WPA_SCANNING && !wpa_s->scanning)
			wpa_supplicant_req_scan(wpa_s, 0, 0);
		break;
	case WPA_CTRL_REQ_EXT_CERT_CHECK:
		if (eap->pending_ext_cert_check != PENDING_CHECK)
			return -1;
		if (os_strcmp(value, "good") == 0)
			eap->pending_ext_cert_check = EXT_CERT_CHECK_GOOD;
		else if (os_strcmp(value, "bad") == 0)
			eap->pending_ext_cert_check = EXT_CERT_CHECK_BAD;
		else
			return -1;
		break;
	default:
		wpa_printf_dbg(MSG_DEBUG, "CTRL_IFACE: Unknown field '%s'", field);
		return -1;
	}

	return 0;
#else /* IEEE8021X_EAPOL */
#ifdef	CONFIG_AP
	wpa_printf_dbg(MSG_DEBUG, "CTRL_IFACE: IEEE 802.1X not included");
#endif	/* CONFIG_AP */
	return -1;
#endif /* IEEE8021X_EAPOL */
}
#endif /* CONFIG_CTRL_IFACE || CONFIG_CTRL_IFACE_DBUS_NEW */
#endif /* CONFIG_INTERWORKING */

#ifdef CONFIG_AP
int wpas_is_network_ap(struct wpa_ssid *ssid)
{
	if (ssid->mode == WPAS_MODE_AP) {
		da16x_ap_prt("[%s] AP ssid\n", __func__);
		return 1;
	} else {
		return 0;
	}
}
#endif /* CONFIG_AP */

int wpas_network_disabled(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	int i;
	unsigned int drv_enc;

	TX_FUNC_START("");

#ifdef CONFIG_P2P
	if (wpa_s->p2p_mgmt)
		return 1; /* no normal network profiles on p2p_mgmt interface */
#endif /* CONFIG_P2P */

	if (ssid == NULL || wpa_s == NULL) {
		da16x_err_prt("[%s] NULL ssid or wpa_s\n", __func__);
		return 1;
	}

	if (ssid->disabled) {
		da16x_drv_prt("[%s] ssid disabled\n", __func__);
		return 1;
	}

	if (wpa_s->drv_capa_known)
		drv_enc = wpa_s->drv_enc;
	else
		drv_enc = (unsigned int) -1;

	for (i = 0; i < NUM_WEP_KEYS; i++) {
		size_t len = ssid->wep_key_len[i];

		if (len == 0)
			continue;
		if (len == 5 && (drv_enc & WPA_DRIVER_CAPA_ENC_WEP40))
			continue;
		if (len == 13 && (drv_enc & WPA_DRIVER_CAPA_ENC_WEP104))
			continue;
		if (len == 16 && (drv_enc & WPA_DRIVER_CAPA_ENC_WEP128))
			continue;

		da16x_err_prt("[%s] Invalid WEP key\n", __func__);

		return 1; /* invalid WEP key */
	}

	if (wpa_key_mgmt_wpa_psk(ssid->key_mgmt) && !ssid->psk_set &&
	    (!ssid->passphrase || ssid->ssid_len != 0) && !ssid->ext_psk &&
#ifdef CONFIG_SAE
	    !(wpa_key_mgmt_sae(ssid->key_mgmt) && ssid->sae_password) &&
#endif /* CONFIG_SAE */
	    !ssid->mem_only_psk)
		return 1;

	TX_FUNC_END("");

	return 0;
}


int wpas_get_ssid_pmf(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
#ifdef CONFIG_IEEE80211W
	if (ssid == NULL || ssid->ieee80211w == MGMT_FRAME_PROTECTION_DEFAULT) {
		if (wpa_s->conf->pmf == MGMT_FRAME_PROTECTION_OPTIONAL &&
		    !(wpa_s->drv_enc & WPA_DRIVER_CAPA_ENC_BIP)) {
			/*
			 * Driver does not support BIP -- ignore pmf=1 default
			 * since the connection with PMF would fail and the
			 * configuration does not require PMF to be enabled.
			 */
			return NO_MGMT_FRAME_PROTECTION;
		}

		if (ssid &&
		    (ssid->key_mgmt &
		     ~(WPA_KEY_MGMT_NONE | WPA_KEY_MGMT_WPS |
		       WPA_KEY_MGMT_IEEE8021X_NO_WPA)) == 0) {
			/*
			 * Do not use the default PMF value for non-RSN networks
			 * since PMF is available only with RSN and pmf=2
			 * configuration would otherwise prevent connections to
			 * all open networks.
			 */
			return NO_MGMT_FRAME_PROTECTION;
		}

		return wpa_s->conf->pmf;
	}

	return ssid->ieee80211w;
#else /* CONFIG_IEEE80211W */
	return NO_MGMT_FRAME_PROTECTION;
#endif /* CONFIG_IEEE80211W */
}


#ifdef	CONFIG_P2P_UNUSED_CMD
int wpas_is_p2p_prioritized(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->global->conc_pref == WPA_CONC_PREF_P2P)
		return 1;
	if (wpa_s->global->conc_pref == WPA_CONC_PREF_STA)
		return 0;

	return -1;
}
#endif	/* CONFIG_P2P_UNUSED_CMD */

void wpas_auth_failed(struct wpa_supplicant *wpa_s, char *reason)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	int dur = 0;
	struct os_reltime now;

	if (ssid == NULL) {
		wpa_printf_dbg(MSG_DEBUG, "Authentication failure but no known "
			   "SSID block");
		return;
	}

#ifdef	CONFIG_WPS
	if (ssid->key_mgmt == WPA_KEY_MGMT_WPS) {
		da16x_assoc_prt("  [%s] key_mgmt is WPS.\n", __func__);
		return;
	}
#endif	/* CONFIG_WPS */

	ssid->auth_failures++;

#ifdef CONFIG_P2P
	if (ssid->p2p_group &&
	    (wpa_s->p2p_in_provisioning || wpa_s->show_group_started)) {
		/*
		 * Skip the wait time since there is a short timeout on the
		 * connection to a P2P group.
		 */
		return;
	}
#endif /* CONFIG_P2P */

#ifndef CONFIG_RECONNECT_OPTIMIZE /* by Shingu 20170717 (Scan Optimization) */
	ssid->auth_failures++;

	if (ssid->auth_failures > 50)
		dur = 300;
	else if (ssid->auth_failures > 10)
		dur = 120;
	else if (ssid->auth_failures > 5)
		dur = 90;
	else if (ssid->auth_failures > 3)
		dur = 60;
	else if (ssid->auth_failures > 2)
		dur = 30;
	else if (ssid->auth_failures > 1)
		dur = 20;
	else
		dur = 10;

	if (ssid->auth_failures > 1 &&
	    wpa_key_mgmt_wpa_ieee8021x(ssid->key_mgmt))
		dur += os_random() % (ssid->auth_failures * 10);
#endif	/* CONFIG_RECONNECT_OPTIMIZE */

	os_get_reltime(&now);
	if (now.sec + dur <= ssid->disabled_until.sec)
		return;


	ssid->disabled_until.sec = now.sec + dur;

#ifdef	__CONFIG_DBG_LOG_MSG__
	wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_TEMP_DISABLED
		"id=%d ssid=\"%s\" auth_failures=%u duration=%d reason=%s",
		ssid->id, wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
		ssid->auth_failures, dur, reason);
#else
	da16x_info_prt("[%s] " WPA_EVENT_TEMP_DISABLED
		"id=%d ssid=\"%s\" auth_failures=%u duration=%d reason=%s\n",
		__func__,
		ssid->id, wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
		ssid->auth_failures, dur, reason);
#endif	/* __CONFIG_DBG_LOG_MSG__ */
}


void wpas_clear_temp_disabled(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid, int clear_failures)
{
	if (ssid == NULL)
		return;

	if (ssid->disabled_until.sec) {
#ifdef	__CONFIG_DBG_LOG_MSG__
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_REENABLED
			"id=%d ssid=\"%s\"",
			ssid->id, wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
#else
		da16x_info_prt("[%s] " WPA_EVENT_REENABLED
			"id=%d ssid=\"%s\"\n",
			__func__,
			ssid->id, wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
#endif	/* __CONFIG_DBG_LOG_MSG__ */
	}
	ssid->disabled_until.sec = 0;
	ssid->disabled_until.usec = 0;
	if (clear_failures)
		ssid->auth_failures = 0;
}


#ifdef	CONFIG_DISALLOW_BSSID
int disallowed_bssid(struct wpa_supplicant *wpa_s, const u8 *bssid)
{
	size_t i;

	if (wpa_s->disallow_aps_bssid == NULL)
		return 0;

	for (i = 0; i < wpa_s->disallow_aps_bssid_count; i++) {
		if (os_memcmp(wpa_s->disallow_aps_bssid + i * ETH_ALEN,
			      bssid, ETH_ALEN) == 0)
			return 1;
	}

	return 0;
}
#endif	/* CONFIG_DISALLOW_BSSID */

#ifdef	CONFIG_DISALLOW_SSID
int disallowed_ssid(struct wpa_supplicant *wpa_s, const u8 *ssid,
		    size_t ssid_len)
{
	size_t i;

	if (wpa_s->disallow_aps_ssid == NULL || ssid == NULL)
		return 0;

	for (i = 0; i < wpa_s->disallow_aps_ssid_count; i++) {
		struct wpa_ssid_value *s = &wpa_s->disallow_aps_ssid[i];
		if (ssid_len == s->ssid_len &&
		    os_memcmp(ssid, s->ssid, ssid_len) == 0)
			return 1;
	}

	return 0;
}
#endif	/* CONFIG_DISALLOW_SSID */

/**
 * wpas_request_connection - Request a new connection
 * @wpa_s: Pointer to the network interface
 *
 * This function is used to request a new connection to be found. It will mark
 * the interface to allow reassociation and request a new scan to find a
 * suitable network to connect to.
 */
#if 1 /* FC9000 */
int wpas_request_connection(struct wpa_supplicant *wpa_s)
{
	if (os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) != 0)
		return -1;

	wpa_s->normal_scans = 0;
#ifdef CONFIG_AUTOSCAN
	wpa_supplicant_reinit_autoscan(wpa_s);
#endif
	wpa_s->extra_blacklist_count = 0;
	wpa_s->disconnected = 0;
	wpa_s->reassociate = 1;

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	if (wpa_supplicant_fast_associate(wpa_s) != 1)
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
		wpa_supplicant_req_scan(wpa_s, 0, 0);

	return 0;
}

#else
void wpas_request_connection(struct wpa_supplicant *wpa_s)
{
	wpa_s->normal_scans = 0;
	wpa_s->scan_req = NORMAL_SCAN_REQ;
#ifdef CONFIG_AUTOSCAN
	wpa_supplicant_reinit_autoscan(wpa_s);
#endif /* CONFIG_AUTOSCAN */
	wpa_s->extra_blacklist_count = 0;
	wpa_s->disconnected = 0;
	wpa_s->reassociate = 1;
	wpa_s->last_owe_group = 0;

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	if (wpa_supplicant_fast_associate(wpa_s) != 1)
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	else
		wpa_s->reattach = 0;
#else
	wpa_supplicant_req_scan(wpa_s, 0, 0);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
}
#endif /* 1 */


/**
 * wpas_request_disconnection - Request disconnection
 * @wpa_s: Pointer to the network interface
 *
 * This function is used to request disconnection from the currently connected
 * network. This will stop any ongoing scans and initiate deauthentication.
 */
void wpas_request_disconnection(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_SME
	wpa_s->sme.prev_bssid_set = 0;
#endif /* CONFIG_SME */
	wpa_s->reassociate = 0;
	wpa_s->disconnected = 1;
#if defined ( CONFIG_SCHED_SCAN )
	wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN
	wpa_supplicant_cancel_scan(wpa_s);
	wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
	da16x_eloop_cancel_timeout(wpas_network_reenabled, wpa_s, NULL);
}

void dump_freq_array(struct wpa_supplicant *wpa_s, const char *title,
		     int *freq_array, unsigned int len)
{
	unsigned int i;

	da16x_scan_prt("[%s] Shared frequencies (len=%u): %s\n", __func__, len, title);

	for (i = 0; i < len; i++)
		da16x_scan_prt("\tfreq[%u]: %d\n", i, freq_array[i]);
}

void dump_freq_data(struct wpa_supplicant *wpa_s, const char *title,
		    struct wpa_used_freq_data *freqs_data,
		    unsigned int len)
{
	unsigned int i;

	wpa_dbg(wpa_s, MSG_DEBUG, "Shared frequencies (len=%u): %s",
		len, title);
	for (i = 0; i < len; i++) {
#ifdef CONFIG_WPA_DBG
		struct wpa_used_freq_data *cur = &freqs_data[i];
#endif /* CONFIG_WPA_DBG */
		wpa_dbg(wpa_s, MSG_DEBUG, "freq[%u]: %d, flags=0x%X",
			i, cur->freq, cur->flags);
	}
}


#ifdef CONFIG_5G_SUPPORT
/*
 * Find the operating frequencies of any of the virtual interfaces that
 * are using the same radio as the current interface, and in addition, get
 * information about the interface types that are using the frequency.
 */
int get_shared_radio_freqs_data(struct wpa_supplicant *wpa_s,
				struct wpa_used_freq_data *freqs_data,
				unsigned int len)
{
	struct wpa_supplicant *ifs;
	u8 bssid[ETH_ALEN];
	int freq;
	unsigned int idx = 0, i;

	wpa_dbg(wpa_s, MSG_DEBUG,
		"Determining shared radio frequencies (max len %u)", len);
	os_memset(freqs_data, 0, sizeof(struct wpa_used_freq_data) * len);

	dl_list_for_each(ifs, &wpa_s->radio->ifaces, struct wpa_supplicant,
			 radio_list) {
		if (idx == len)
			break;

		if (ifs->current_ssid == NULL || ifs->assoc_freq == 0)
			continue;

		if (ifs->current_ssid->mode == WPAS_MODE_AP
#ifdef	CONFIG_P2P
			|| ifs->current_ssid->mode == WPAS_MODE_P2P_GO
#endif	/* CONFIG_P2P */
#ifdef CONFIG_MESH
		    || ifs->current_ssid->mode == WPAS_MODE_MESH
#endif /* CONFIG_MESH */
		    )
			freq = ifs->current_ssid->frequency;
		else if (wpa_drv_get_bssid(ifs, bssid) == 0)
			freq = ifs->assoc_freq;
		else
			continue;

		/* Hold only distinct freqs */
		for (i = 0; i < idx; i++)
			if (freqs_data[i].freq == freq)
				break;

		if (i == idx)
			freqs_data[idx++].freq = freq;

		if (ifs->current_ssid->mode == WPAS_MODE_INFRA) {
			freqs_data[i].flags |= ifs->current_ssid->p2p_group ?
				WPA_FREQ_USED_BY_P2P_CLIENT :
				WPA_FREQ_USED_BY_INFRA_STATION;
		}
	}

	return idx;
}
#endif /* CONFIG_5G_SUPPORT */

/*
 * Find the operating frequencies of any of the virtual interfaces that
 * are using the same radio as the current interface.
 */
#if 1	/* By Shingu */
int get_shared_radio_freqs(struct wpa_supplicant *wpa_s,
			   int *freq_array, unsigned int len)
{
	u8 bssid[ETH_ALEN];
	int freq;
	unsigned int idx = 0;

	da16x_scan_prt("[%s] Determining shared radio frequencies (max len %u)\n",
					__func__, len);

	os_memset(freq_array, 0, sizeof(int) * len);

	/* First add the frequency of the local interface */
	if (wpa_s->current_ssid != NULL && wpa_s->assoc_freq != 0) {
		if (wpa_s->current_ssid->mode == WPAS_MODE_AP
#ifdef	CONFIG_P2P
			|| wpa_s->current_ssid->mode == WPAS_MODE_P2P_GO
#endif	/* CONFIG_P2P */
		)
			freq_array[idx++] = wpa_s->current_ssid->frequency;
		else if (wpa_drv_get_bssid(wpa_s, bssid) == 0)
			freq_array[idx++] = wpa_s->assoc_freq;
	}

	/* If get_radio_name is not supported, use only the local freq */
	if (!wpa_driver_get_radio_name(wpa_s)) {
		freq = wpa_drv_shared_freq(wpa_s);
		if (freq > 0 && idx < len &&
		    (idx == 0 || freq_array[0] != freq))
			freq_array[idx++] = freq;
		dump_freq_array(wpa_s, "No get_radio_name", freq_array, idx);
		return idx;
	}

	dump_freq_array(wpa_s, "completed iteration", freq_array, idx);
	return idx;
}
#else
int get_shared_radio_freqs(struct wpa_supplicant *wpa_s,
			   int *freq_array, unsigned int len)
{
	struct wpa_used_freq_data *freqs_data;
	int num, i;

	os_memset(freq_array, 0, sizeof(int) * len);

	freqs_data = os_calloc(len, sizeof(struct wpa_used_freq_data));
	if (!freqs_data)
		return -1;

	num = get_shared_radio_freqs_data(wpa_s, freqs_data, len);
	for (i = 0; i < num; i++)
		freq_array[i] = freqs_data[i].freq;

	os_free(freqs_data);

	return num;
}
#endif

#if defined ( CONFIG_VENDOR_ELEM )
struct wpa_supplicant *
wpas_vendor_elem(struct wpa_supplicant *wpa_s, enum wpa_vendor_elem_frame frame)
{
	switch (frame) {
#ifdef CONFIG_P2P
	case VENDOR_ELEM_PROBE_REQ_P2P:
	case VENDOR_ELEM_PROBE_RESP_P2P:
	case VENDOR_ELEM_PROBE_RESP_P2P_GO:
	case VENDOR_ELEM_BEACON_P2P_GO:
	case VENDOR_ELEM_P2P_PD_REQ:
	case VENDOR_ELEM_P2P_PD_RESP:
	case VENDOR_ELEM_P2P_GO_NEG_REQ:
	case VENDOR_ELEM_P2P_GO_NEG_RESP:
	case VENDOR_ELEM_P2P_GO_NEG_CONF:
	case VENDOR_ELEM_P2P_INV_REQ:
	case VENDOR_ELEM_P2P_INV_RESP:
	case VENDOR_ELEM_P2P_ASSOC_REQ:
	case VENDOR_ELEM_P2P_ASSOC_RESP:
		return wpa_s->parent;
#endif /* CONFIG_P2P */
	default:
		return wpa_s;
	}
}


void wpas_vendor_elem_update(struct wpa_supplicant *wpa_s)
{
	unsigned int i;
	char buf[30];

	wpa_printf_dbg(MSG_DEBUG, "Update vendor elements");

	for (i = 0; i < NUM_VENDOR_ELEM_FRAMES; i++) {
		if (wpa_s->vendor_elem[i]) {
			int res;

			res = os_snprintf(buf, sizeof(buf), "frame[%u]", i);
			if (!os_snprintf_error(sizeof(buf), res)) {
				wpa_hexdump_buf(MSG_DEBUG, buf,
						wpa_s->vendor_elem[i]);
			}
		}
	}

#ifdef CONFIG_P2P
	if (wpa_s->parent == wpa_s &&
	    wpa_s->global->p2p &&
	    !wpa_s->global->p2p_disabled)
		p2p_set_vendor_elems(wpa_s->global->p2p, wpa_s->vendor_elem);
#endif /* CONFIG_P2P */
}


int wpas_vendor_elem_remove(struct wpa_supplicant *wpa_s, int frame,
			    const u8 *elem, size_t len)
{
	u8 *ie, *end;

	ie = wpabuf_mhead_u8(wpa_s->vendor_elem[frame]);
	end = ie + wpabuf_len(wpa_s->vendor_elem[frame]);

	for (; ie + 1 < end; ie += 2 + ie[1]) {
		if (ie + len > end)
			break;
		if (os_memcmp(ie, elem, len) != 0)
			continue;

		if (wpabuf_len(wpa_s->vendor_elem[frame]) == len) {
			wpabuf_free(wpa_s->vendor_elem[frame]);
			wpa_s->vendor_elem[frame] = NULL;
		} else {
			os_memmove(ie, ie + len, end - (ie + len));
			wpa_s->vendor_elem[frame]->used -= len;
		}
		wpas_vendor_elem_update(wpa_s);
		return 0;
	}

	return -1;
}
#endif	// CONFIG_VENDOR_ELEM


struct hostapd_hw_modes * get_mode(struct hostapd_hw_modes *modes,
				   u16 num_modes, enum hostapd_hw_mode mode)
{
	u16 i;

	for (i = 0; i < num_modes; i++) {
		if (modes[i].mode == mode)
			return &modes[i];
	}

	return NULL;
}


#if defined ( __SUPP_27_SUPPORT__ )
static struct
wpa_bss_tmp_disallowed * wpas_get_disallowed_bss(struct wpa_supplicant *wpa_s,
						 const u8 *bssid)
{
	struct wpa_bss_tmp_disallowed *bss;

	dl_list_for_each(bss, &wpa_s->bss_tmp_disallowed,
			 struct wpa_bss_tmp_disallowed, list) {
		if (os_memcmp(bssid, bss->bssid, ETH_ALEN) == 0)
			return bss;
	}

	return NULL;
}


static int wpa_set_driver_tmp_disallow_list(struct wpa_supplicant *wpa_s)
{
	struct wpa_bss_tmp_disallowed *tmp;
	unsigned int num_bssid = 0;
	u8 *bssids;
	int ret;

	bssids = os_malloc(dl_list_len(&wpa_s->bss_tmp_disallowed) * ETH_ALEN);
	if (!bssids)
		return -1;
	dl_list_for_each(tmp, &wpa_s->bss_tmp_disallowed,
			 struct wpa_bss_tmp_disallowed, list) {
		os_memcpy(&bssids[num_bssid * ETH_ALEN], tmp->bssid,
			  ETH_ALEN);
		num_bssid++;
	}
	ret = wpa_drv_set_bssid_blacklist(wpa_s, num_bssid, bssids);
	os_free(bssids);
	return ret;
}


static void wpa_bss_tmp_disallow_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct wpa_bss_tmp_disallowed *tmp, *bss = timeout_ctx;

	/* Make sure the bss is not already freed */
	dl_list_for_each(tmp, &wpa_s->bss_tmp_disallowed,
			 struct wpa_bss_tmp_disallowed, list) {
		if (bss == tmp) {
			dl_list_del(&tmp->list);
			os_free(tmp);
			wpa_set_driver_tmp_disallow_list(wpa_s);
			break;
		}
	}
}


void wpa_bss_tmp_disallow(struct wpa_supplicant *wpa_s, const u8 *bssid,
			  unsigned int sec)
{
	struct wpa_bss_tmp_disallowed *bss;

	bss = wpas_get_disallowed_bss(wpa_s, bssid);
	if (bss) {
		da16x_eloop_cancel_timeout(wpa_bss_tmp_disallow_timeout, wpa_s, bss);
		da16x_eloop_register_timeout(sec, 0, wpa_bss_tmp_disallow_timeout,
				       wpa_s, bss);
		return;
	}

	bss = os_malloc(sizeof(*bss));
	if (!bss) {
		wpa_printf_dbg(MSG_DEBUG,
			   "Failed to allocate memory for temp disallow BSS");
		return;
	}

	os_memcpy(bss->bssid, bssid, ETH_ALEN);
	dl_list_add(&wpa_s->bss_tmp_disallowed, &bss->list);
	wpa_set_driver_tmp_disallow_list(wpa_s);
	da16x_eloop_register_timeout(sec, 0, wpa_bss_tmp_disallow_timeout,
			       wpa_s, bss);
}


int wpa_is_bss_tmp_disallowed(struct wpa_supplicant *wpa_s, const u8 *bssid)
{
	struct wpa_bss_tmp_disallowed *bss = NULL, *tmp, *prev;

	dl_list_for_each_safe(tmp, prev, &wpa_s->bss_tmp_disallowed,
			 struct wpa_bss_tmp_disallowed, list) {
		if (os_memcmp(bssid, tmp->bssid, ETH_ALEN) == 0) {
			bss = tmp;
			break;
		}
	}
	if (!bss)
		return 0;

	return 1;
}

#endif // __SUPP_27_SUPPORT__

/* by Bright :
 *	set current da16x_supplicant state to use in global function
 */
#if 0
int get_cur_supp_state(void)
{
	return (int)supp_cur_state;
}
#endif

/* by Trinity:
* Porting: if_nametoindex()
* TODO: return -1 when interface is not running
*/
int wpa_supp_ifname_to_ifindex(const char *ifname)
{
	if (os_strcmp(ifname, STA_DEVICE_NAME) == 0) {
		return 0;
	} else if (
#ifdef	CONFIG_P2P
			os_strcmp(ifname, P2P_DEVICE_NAME) == 0
		   ||
#endif	/* CONFIG_P2P */
			os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0) {
		return 1;
	} else {
		return -1;
	}
}

#if 0	/* by Shingu 20160901 (Optimize) */
struct wpa_supplicant *wpa_supp_ifindex_to_iface(struct wpa_supplicant *wpa_s,
						 int index)
{
	struct wpa_global *global = wpa_s->global;

	if (index == 0){
		return global->ifaces->next;
	} else{
		return global->ifaces;
	}
}
#endif	/* 0 */

static int wpa_supp_dpm_backup_wep_key(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;

	if (!chk_rtm_exist())
		return 0;

	ssid = (struct wpa_ssid *)wpa_s->current_ssid;

	if ((ssid->mode != WPAS_MODE_INFRA)
		&& (ssid->mode != WPAS_MODE_IBSS)) {
		da16x_dpm_prt("[%s] Not supported this mode. \n", __func__);
		return 0;
	}

	os_memcpy(dpm_supp_key_info->wep_key,
				ssid->wep_key[ssid->wep_tx_keyidx],
				MAX_WEP_KEY_LEN);

	dpm_supp_key_info->wep_key_len =
				ssid->wep_key_len[ssid->wep_tx_keyidx];

	dpm_supp_key_info->wep_tx_keyidx = ssid->wep_tx_keyidx;

	return 1;
}

static int wpa_supp_dpm_backup_conn_info(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;

	if (0 >= chk_dpm_mode())
		return 0;

	if (chk_supp_connected())
		return 0;

	ssid = (struct wpa_ssid *)wpa_s->current_ssid;
	if (ssid == NULL) {
		da16x_dpm_prt("[%s] Failed to parse network profile\n",
				__func__);
		return -1;
	}

	if ((ssid->mode != WPAS_MODE_INFRA)
		&& (ssid->mode != WPAS_MODE_IBSS)) {
		da16x_dpm_prt("[%s] Not supported this mode. \n", __func__);
		return 0;
	}

	os_memset(dpm_supp_conn_info, 0, sizeof(dpm_supp_conn_info_t));
	os_memset(dpm_supp_conn_info->bssid, NULL, ETH_ALEN);
	os_memset(dpm_supp_conn_info->ssid, NULL, DPM_MAX_SSID_LEN);
	os_memset(dpm_supp_conn_info->psk, NULL, DPM_PSK_LEN);

	dpm_supp_conn_info->mode = ssid->mode;
	dpm_supp_conn_info->disabled = ssid->disabled;
	dpm_supp_conn_info->id = ssid->id;
	dpm_supp_conn_info->ssid_len = ssid->ssid_len;
	dpm_supp_conn_info->scan_ssid = ssid->scan_ssid;
	dpm_supp_conn_info->psk_set = ssid->psk_set;
	dpm_supp_conn_info->auth_alg = ssid->auth_alg;

	os_memcpy(dpm_supp_conn_info->bssid, wpa_s->bssid, ETH_ALEN);
	os_memcpy(dpm_supp_conn_info->ssid, ssid->ssid, ssid->ssid_len);
	os_memcpy(dpm_supp_conn_info->psk, ssid->psk, DPM_PSK_LEN);

#ifdef DEF_SAVE_DPM_WIFI_MODE
	/* Backup the Connection WiFi mode in DPM wakeup , 180528*/
	dpm_supp_conn_info->wifi_mode = ssid->wifi_mode;
	
#ifdef  CONFIG_DPM_OPT_WIFI_MODE
	dpm_supp_conn_info->dpm_opt = ssid->dpm_opt_wifi_mode;
#endif  /* CONFIG_DPM_OPT_WIFI_MODE */
#ifdef CONFIG_IEEE80211W
	dpm_supp_conn_info->pmf = wpa_s->conf->pmf;
#endif // CONFIG_IEEE80211W
#endif
	da16x_dpm_prt("[%s] Finish to backup basic WiFi information\n",
				__func__);
	return 1;
}

void wpa_supp_dpm_clear_conn_info(struct wpa_supplicant *wpa_s)
{
	int	dpm_sts, dpm_retry_cnt;

	if (chk_dpm_mode()) {
		dpm_retry_cnt = 0;

dpm_clr_retry :
		if (dpm_retry_cnt++ < 10) {
			dpm_sts = clr_dpm_sleep(REG_NAME_DPM_SUPPLICANT);
			switch (dpm_sts) {
			case DPM_SET_ERR :
				vTaskDelay(1);
				goto dpm_clr_retry;
				break;

			case DPM_SET_ERR_BLOCK :
				/* Don't need try continues */
				;
			case DPM_SET_OK :
				break;
			}
		}

		if (!chk_dpm_wakeup() 
#ifdef __SUPPORT_WPA_ENTERPRISE_CORE__
			|| (chk_dpm_wakeup() && wpa_key_mgmt_wpa_ieee8021x(wpa_s->conf->ssid->key_mgmt))
#endif // __SUPPORT_WPA_ENTERPRISE_CORE__
		)
		{
			dpm_disconn_reconf(wpa_s->conf, wpa_s->ifname);
		}
	}

	clr_dpm_conn_info();

	da16x_dpm_prt("[%s] Clear RTM Connected info...\n", __func__);

}

extern int fc80211_get_bssid(int ifindex, u8 *bssid);
extern int fc80211_get_sta_status(int ifindex, u8 *bssid);
void wpa_supp_dpm_restore_conn_info(struct wpa_supplicant *wpa_s)
{
	int wpa_state = 0;

	wpa_state = get_supp_conn_state();
	wpa_s->wpa_state = (enum wpa_states)wpa_state;
	wpa_supplicant_set_state(wpa_s, wpa_s->wpa_state);

	os_memcpy(wpa_s->bssid, dpm_supp_conn_info->bssid, ETH_ALEN);

	wpa_supp_dpm_restore_config(wpa_s->conf);

	wpa_s->wpa_proto = dpm_supp_key_info->proto;
	wpa_s->pairwise_cipher = dpm_supp_key_info->pairwise_cipher;
	wpa_s->group_cipher = dpm_supp_key_info->group_cipher;
	wpa_s->key_mgmt = dpm_supp_key_info->key_mgmt;
	wpa_sm_set_bssid(wpa_s->wpa, wpa_s->bssid);

	/* Restore wpa_sm */
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_PROTO, dpm_supp_key_info->proto);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_KEY_MGMT, dpm_supp_key_info->key_mgmt);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_PAIRWISE, dpm_supp_key_info->pairwise_cipher);
	wpa_sm_set_param(wpa_s->wpa, WPA_PARAM_GROUP, dpm_supp_key_info->group_cipher);
	wpa_sm_set_bssid(wpa_s->wpa, wpa_s->bssid);

	wpa_s->current_ssid = wpa_s->conf->ssid;
#ifdef CONFIG_IEEE80211W
	wpa_s->conf->pmf    = dpm_supp_conn_info->pmf;
#endif // CONFIG_IEEE80211W

#ifdef	ENABLE_DPM_DBG
	da16x_dpm_prt("[DPM] Done restore the Connection Information\n");
#endif	/* ENABLE_DPM_DBG */
}

/* EOF */
