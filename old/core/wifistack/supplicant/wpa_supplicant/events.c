/*
 * WPA Supplicant - Driver event processing
 * Copyright (c) 2003-2017, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "sdk_type.h"

#include "includes.h"

#include "supp_common.h"
#ifdef	IEEE8021X_EAPOL
#include "eapol_supp/eapol_supp_sm.h"
#endif	/* IEEE8021X_EAPOL */

#include "rsn_supp/wpa.h"
#include "supp_eloop.h"
#include "supp_config.h"
#include "l2_packet/l2_packet.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#ifdef CONFIG_PCSC_FUNCS
#include "pcsc_funcs.h"
#endif /* CONFIG_PCSC_FUNCS */
#include "rsn_supp/preauth.h"
#include "rsn_supp/pmksa_cache.h"
#include "wpa_ctrl.h"
#ifdef	IEEE8021X_EAPOL
#include "eap_peer/eap.h"
#endif	/* IEEE8021X_EAPOL */
#include "ap/hostapd.h"
#ifdef	CONFIG_P2P
#include "p2p/supp_p2p.h"
#endif	/* CONFIG_P2P */
#ifdef CONFIG_FST
#include "fst/fst.h"
#endif /* CONFIG_FST */
#include "wnm_sta.h"
//#include "notify.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#ifdef CONFIG_GAS
#include "common/gas_server.h"
#endif /* CONFIG_GAS */
#include "crypto/random.h"
#include "blacklist.h"
#include "wpas_glue.h"
#include "wps_supplicant.h"
#include "ibss_rsn.h"
#include "supp_sme.h"
#include "common_def.h"

#ifdef CONFIG_GAS
#include "gas_query.h"
#endif /* CONFIG_GAS */

#ifdef	CONFIG_P2P
#include "p2p_supplicant.h"
#endif	/* CONFIG_P2P */

#ifdef CONFIG_BGSCAN
#include "bgscan.h"
#endif /* CONFIG_BGSCAN */
#include "autoscan.h"
#include "ap.h"
#include "tkip_countermeasures.h"
#include "bss.h"
#include "supp_scan.h"
#include "offchannel.h"
#include "interworking.h"
#ifdef CONFIG_MESH
#include "supp_mesh.h"
#include "mesh_mpm.h"
#endif /* CONFIG_MESH */
#ifdef CONFIG_WMM_AC
#include "wmm_ac.h"
#endif /* CONFIG_WMM_AC */
#ifdef CONFIG_DPP
#include "dpp_supplicant.h"
#endif /* CONFIG_DPP */

#ifdef	BUILD_OPT_FC9000_ROMNDK
#include "rom_schd_mem.h"
#endif  /* BUILD_OPT_FC9000_ROMNDK */

#include "common_utils.h"

#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wempty-body"
#pragma GCC diagnostic ignored "-Wextra"

#ifdef CONFIG_SIMULATED_MIC_FAILURE
union wpa_event_data	mic_event;
#endif /* CONFIG_SIMULATED_MIC_FAILURE */

// for get_cur_connected_bssid() API
u8	cur_connected_bssid[ETH_ALEN] = { 0, };

/* Definition values */
#define UMAC_MEM_USE_TX_MEM

/* External variables */
#ifdef CONFIG_FAST_RECONNECT_V2
extern u8               da16x_is_fast_reconnect;
extern u8               fast_reconnect_tries;
#endif /* CONFIG_FAST_RECONNECT_V2 */
extern int              wifi_conn_fail_reason_code;
extern unsigned char	dpm_abnormal_sleep1_flag;
extern unsigned char	stop_wifi_conn_try_at_wrong_key_flag;

/* External functions */
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
extern void supp_rehold_bss(void *temp);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
#ifdef	CONFIG_SIMPLE_ROAMING
extern void fc80211_set_roaming_mode(int mode);
#endif	/* CONFIG_SIMPLE_ROAMING */
extern void sme_clear_on_disassoc(struct wpa_supplicant *wpa_s);
extern void wpas_send_ctrl_req(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,	\
			const char *field_name, const char *txt);
extern void wpa_supplicant_cancel_scan(struct wpa_supplicant *wpa_s);
extern void sme_event_assoc_timed_out(struct wpa_supplicant *wpa_s, union wpa_event_data *data);
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
extern void supp_unhold_bss(void *temp);
#endif	/* CONFIG_REUSED_UMAC_BSS_LIST */
extern void force_dpm_abnormal_sleep_by_wifi_conn_fail(void);
extern int  dpm_get_wifi_conn_retry_cnt(void);
extern int  dpm_decr_wifi_conn_retry_cnt(void);
extern void force_dpm_abnormal_sleep1(void);
extern void wpa_passive_scan_done(void);
extern void (*wifi_conn_fail_notify_cb)(short reason_code);

/* Local static functions */
#ifndef CONFIG_NO_SCAN_PROCESSING
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
static int wpas_select_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      int new_scan, int own_request, struct wpa_bss **selected, struct wpa_ssid **ssid);
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
static int wpas_select_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      int new_scan, int own_request);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#endif /* CONFIG_NO_SCAN_PROCESSING */
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
static int wpas_connect_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      struct wpa_bss *selected, struct wpa_ssid *ssid);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

/* Local functions */
void wpa_scan_results_free(struct wpa_scan_results *res);

int wpas_temp_disabled(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	struct os_reltime now;

	if (ssid == NULL || ssid->disabled_until.sec == 0)
		return 0;

	os_get_reltime(&now);
	if (ssid->disabled_until.sec > now.sec)
		return ssid->disabled_until.sec - now.sec;

	wpas_clear_temp_disabled(wpa_s, ssid, 0);

	return 0;
}


#ifndef CONFIG_NO_SCAN_PROCESSING
/**
 * wpas_reenabled_network_time - Time until first network is re-enabled
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: If all enabled networks are temporarily disabled, returns the time
 *	(in sec) until the first network is re-enabled. Otherwise returns 0.
 *
 * This function is used in case all enabled networks are temporarily disabled,
 * in which case it returns the time (in sec) that the first network will be
 * re-enabled. The function assumes that at least one network is enabled.
 */
static int wpas_reenabled_network_time(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;
	int disabled_for, res = 0;

#ifdef CONFIG_INTERWORKING
	if (wpa_s->conf->auto_interworking && wpa_s->conf->interworking &&
	    wpa_s->conf->cred)
		return 0;
#endif /* CONFIG_INTERWORKING */

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (ssid->disabled)
			continue;

		disabled_for = wpas_temp_disabled(wpa_s, ssid);
		if (!disabled_for)
			return 0;

		if (!res || disabled_for < res)
			res = disabled_for;
	}

	return res;
}
#endif /* CONFIG_NO_SCAN_PROCESSING */


void wpas_network_reenabled(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (wpa_s->disconnected || wpa_s->wpa_state != WPA_SCANNING)
		return;

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	wpa_dbg(wpa_s, MSG_DEBUG,
		"Try to associate due to network getting re-enabled");
	if (wpa_supplicant_fast_associate(wpa_s) != 1) {
#if defined ( CONFIG_SCHED_SCAN )
		wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	}
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
}


#ifndef FEATURE_USE_DEFAULT_AP_SCAN
static struct wpa_bss * wpa_supplicant_get_new_bss(
	struct wpa_supplicant *wpa_s, const u8 *bssid)
{
	struct wpa_bss *bss = NULL;
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (ssid->ssid_len > 0)
		bss = wpa_bss_get(wpa_s, bssid, ssid->ssid, ssid->ssid_len);
	if (!bss)
		bss = wpa_bss_get_bssid(wpa_s, bssid);

	return bss;
}


static void wpa_supplicant_update_current_bss(struct wpa_supplicant *wpa_s)
{
	struct wpa_bss *bss = wpa_supplicant_get_new_bss(wpa_s, wpa_s->bssid);

	if (!bss) {
		wpa_supplicant_update_scan_results(wpa_s);

		/* Get the BSS from the new scan results */
		bss = wpa_supplicant_get_new_bss(wpa_s, wpa_s->bssid);
	}

	if (bss)
	{
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);
#endif	/* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = bss;
	}	
}


static int wpa_supplicant_select_config(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid=NULL, *old_ssid=NULL
#if 1 /* FC9000 Only ?? supplicant 2.6 */
	struct wpa_bss *bss=NULL;
#endif /*  FC9000 Only ?? supplicant 2.6 */
	u8 drv_ssid[SSID_MAX_LEN];
	size_t drv_ssid_len;
	int res;

	if (wpa_s->conf->ap_scan == 1 && wpa_s->current_ssid) {
		wpa_supplicant_update_current_bss(wpa_s);

		if (wpa_s->current_ssid->ssid_len == 0)
			return 0; /* current profile still in use */
#if 0
		res = wpa_drv_get_ssid(wpa_s, drv_ssid);
		if (res < 0) {
			wpa_msg(wpa_s, MSG_INFO,
				"Failed to read SSID from driver");
			return 0; /* try to use current profile */
		}
		drv_ssid_len = res;

		if (drv_ssid_len == wpa_s->current_ssid->ssid_len &&
		    os_memcmp(drv_ssid, wpa_s->current_ssid->ssid,
			      drv_ssid_len) == 0)
			return 0; /* current profile still in use */

		wpa_dbg(wpa_s, MSG_DEBUG,
			"Driver-initiated BSS selection changed the SSID to %s",
			wpa_ssid_txt(drv_ssid, drv_ssid_len));
		/* continue selecting a new network profile */
#endif	/* 0 */
	}

#ifdef	__SUPP_27_SUPPORT__
	wpa_dbg(wpa_s, MSG_DEBUG, "Select network based on association "
		"information");
#else
	da16x_assoc_prt("[%s] Select network based on assoc_info\n",
            __func__);
#endif	/* __SUPP_27_SUPPORT__ */

	ssid = wpa_supplicant_get_ssid(wpa_s);
	if (ssid == NULL) {
#ifdef	__SUPP_27_SUPPORT__
		wpa_msg(wpa_s, MSG_INFO,
			"No network configuration for the current AP");
#else
		da16x_err_prt("[%s] No network configuration for the current AP\n", __func__);
#endif	/* __SUPP_27_SUPPORT__ */
		return -1;
	}

	if (wpas_network_disabled(wpa_s, ssid)) {
#ifdef	__SUPP_27_SUPPORT__
		wpa_dbg(wpa_s, MSG_DEBUG, "Selected network is disabled");
#else
		da16x_assoc_prt("[%s] Selected network is disabled\n", __func__);
#endif	/* __SUPP_27_SUPPORT__ */
		return -1;
	}

#ifdef	CONFIG_DISALLOW_BSSID
	if (disallowed_bssid(wpa_s, wpa_s->bssid) ||
	    disallowed_ssid(wpa_s, ssid->ssid, ssid->ssid_len)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Selected BSS is disallowed");
		
		return -1;
	}
#endif	/* CONFIG_DISALLOW_BSSID */

	res = wpas_temp_disabled(wpa_s, ssid);
	if (res > 0) {
#ifdef	__SUPP_27_SUPPORT__
		wpa_dbg(wpa_s, MSG_DEBUG, "Selected network is temporarily "
			"disabled for %d second(s)", res);
#else
		da16x_err_prt("[%s] Selected network is temporarily "
            "disabled for %d second(s)\n", __func__, res);
#endif	/* __SUPP_27_SUPPORT__ */
		return -1;
	}

#ifdef	__SUPP_27_SUPPORT__
	wpa_dbg(wpa_s, MSG_DEBUG, "Network configuration found for the "
		"current AP");
#else
	da16x_assoc_prt("[%s] Network configuration found for the current AP\n",
                __func__);
#endif	/* __SUPP_27_SUPPORT__ */

	if (wpa_key_mgmt_wpa_any(ssid->key_mgmt)) {
		u8 wpa_ie[80];
		size_t wpa_ie_len = sizeof(wpa_ie);
		if (wpa_supplicant_set_suites(wpa_s, NULL, ssid,
					      wpa_ie, &wpa_ie_len) < 0)
#ifdef	__SUPP_27_SUPPORT__
			wpa_dbg(wpa_s, MSG_DEBUG, "Could not set WPA suites");
#else
			da16x_assoc_prt("[%s] Could not set WPA suites\n", __func__);
#endif	/* __SUPP_27_SUPPORT__ */
	} else {
		wpa_supplicant_set_non_wpa_policy(wpa_s, ssid);
	}

#ifdef	IEEE8021X_EAPOL
	if (wpa_s->current_ssid && wpa_s->current_ssid != ssid)
		eapol_sm_invalidate_cached_session(wpa_s->eapol);
#endif	/* IEEE8021X_EAPOL */
	wpa_s->current_ssid = ssid;

#if 1 /* FC9000 Only ?? supplicant 2.6 */
	bss = wpa_supplicant_get_new_bss(wpa_s, wpa_s->bssid);
	if (!bss) {
		wpa_supplicant_update_scan_results(wpa_s);

		/* Get the BSS from the new scan results */
		bss = wpa_supplicant_get_new_bss(wpa_s, wpa_s->bssid);
	}

	if (bss) {
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_s->current_bss = bss;
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
		wpa_supplicant_copy_ie_for_current_bss(wpa_s, bss);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
	}
#else
	wpa_supplicant_update_current_bss(wpa_s);
#endif  /* FC9000 Only ?? supplicant 2.6 */

	wpa_supplicant_rsn_supp_set_config(wpa_s, wpa_s->current_ssid);

#ifdef	IEEE8021X_EAPOL
	wpa_supplicant_initiate_eapol(wpa_s);
#endif /* IEEE8021X_EAPOL */

#ifdef CONFIG_NOTIFY
	if (old_ssid != wpa_s->current_ssid)
		wpas_notify_network_changed(wpa_s);
#endif /* CONFIG_NOTIFY */

	return 0;
}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

void wpa_supplicant_stop_countermeasures(void *eloop_ctx, void *sock_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (wpa_s->countermeasures) {
		wpa_s->countermeasures = 0;
		wpa_drv_set_countermeasures(wpa_s, 0);
		wpa_msg(wpa_s, MSG_INFO, "WPA: TKIP countermeasures stopped");

#if defined ( CONFIG_SCHED_SCAN )
		/*
		 * It is possible that the device is sched scanning, which means
		 * that a connection attempt will be done only when we receive
		 * scan results. However, in this case, it would be preferable
		 * to scan and connect immediately, so cancel the sched_scan and
		 * issue a regular scan flow.
		 */
		wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	}
}

void wpa_supplicant_mark_disassoc(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_NOTIFY
	int bssid_changed;
#endif /* CONFIG_NOTIFY */

#ifdef CONFIG_WNM_BSS_MAX_IDLE_PERIOD
	wnm_bss_keep_alive_deinit(wpa_s);
#endif /* CONFIG_WNM_BSS_MAX_IDLE_PERIOD */

#ifdef CONFIG_IBSS_RSN
	ibss_rsn_deinit(wpa_s->ibss_rsn);
	wpa_s->ibss_rsn = NULL;
#endif /* CONFIG_IBSS_RSN */

#if 0 //trinity_0150401
#ifdef CONFIG_AP
	wpa_supplicant_ap_deinit(wpa_s);
#endif /* CONFIG_AP */
#endif

#ifdef CONFIG_HS20
	/* Clear possibly configured frame filters */
	wpa_drv_configure_frame_filters(wpa_s, 0);
#endif /* CONFIG_HS20 */

	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED)
		return;

	wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);

#ifdef CONFIG_NOTIFY
	bssid_changed = !is_zero_ether_addr(wpa_s->bssid);
#endif /* CONFIG_NOTIFY */


	os_memset(wpa_s->bssid, 0, ETH_ALEN);
	os_memset(wpa_s->pending_bssid, 0, ETH_ALEN);

#ifdef CONFIG_P2P
	os_memset(wpa_s->go_dev_addr, 0, ETH_ALEN);
#endif /* CONFIG_P2P */

	sme_clear_on_disassoc(wpa_s);
#ifdef CONFIG_P2P
	os_memset(wpa_s->go_dev_addr, 0, ETH_ALEN);
#endif /* CONFIG_P2P */
#ifndef CONFIG_FAST_RECONNECT_V2
		if(wpa_s->current_bss) {
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
			wpa_supplicant_clear_current_bss(wpa_s);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
			wpa_s->current_bss = NULL;
		}
#endif /* CONFIG_FAST_RECONNECT_V2 */
	wpa_s->assoc_freq = 0;

#ifdef CONFIG_NOTIFY
	if (bssid_changed)
		wpas_notify_bssid_changed(wpa_s);
#endif /* CONFIG_NOTIFY */

#ifdef	CONFIG_EAPOL
	eapol_sm_notify_portEnabled(wpa_s->eapol, FALSE);
	eapol_sm_notify_portValid(wpa_s->eapol, FALSE);
	if (wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt) ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_OWE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_DPP)
		eapol_sm_notify_eap_success(wpa_s->eapol, FALSE);
#endif /* CONFIG_EAPOL */

	wpa_s->ap_ies_from_associnfo = 0;
	wpa_s->current_ssid = NULL;

#ifdef	CONFIG_EAPOL
	eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
#endif	/* SUPPORT_NVRAM */
	wpa_s->key_mgmt = 0;

#if 0
	wpas_rrm_reset(wpa_s);
#endif /* 0 */
	wpa_s->wnmsleep_used = 0;

#ifdef CONFIG_TESTING_OPTIONS
	wpa_s->last_tk_alg = WPA_ALG_NONE;
	os_memset(wpa_s->last_tk, 0, sizeof(wpa_s->last_tk));
#endif /* CONFIG_TESTING_OPTIONS */
#ifdef CONFIG_IEEE80211AC
	wpa_s->ieee80211ac = 0;
#endif /* CONFIG_IEEE80211AC */

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	wpa_bss_flush(wpa_s);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

}

static void wpa_find_assoc_pmkid(struct wpa_supplicant *wpa_s)
{
	struct wpa_ie_data ie;
	int pmksa_set = -1;
	size_t i;

	if (wpa_sm_parse_own_wpa_ie(wpa_s->wpa, &ie) < 0 ||
	    ie.pmkid == NULL)
		return;

	for (i = 0; i < ie.num_pmkid; i++) {
		pmksa_set = pmksa_cache_set_current(wpa_s->wpa,
						    ie.pmkid + i * PMKID_LEN,
						    NULL, NULL, 0, NULL, 0);
		if (pmksa_set == 0) {
#ifdef	IEEE8021X_EAPOL
			eapol_sm_notify_pmkid_attempt(wpa_s->eapol);
#endif	/* SUPPORT_NVRAM */
			break;
		}
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "RSN: PMKID from assoc IE %sfound from "
		"PMKSA cache", pmksa_set == 0 ? "" : "not ");
}

#ifdef	CONFIG_PRE_AUTH
static void wpa_supplicant_event_pmkid_candidate(struct wpa_supplicant *wpa_s,
						 union wpa_event_data *data)
{
	if (data == NULL) {
		wpa_dbg(wpa_s, MSG_DEBUG, "RSN: No data in PMKID candidate "
			"event");
		return;
	}
	wpa_dbg(wpa_s, MSG_DEBUG, "RSN: PMKID candidate event - bssid=" MACSTR
		" index=%d preauth=%d",
		MAC2STR(data->pmkid_candidate.bssid),
		data->pmkid_candidate.index,
		data->pmkid_candidate.preauth);

	pmksa_candidate_add(wpa_s->wpa, data->pmkid_candidate.bssid,
			    data->pmkid_candidate.index,
			    data->pmkid_candidate.preauth);
}
#endif	/* CONFIG_PRE_AUTH */

static int wpa_supplicant_dynamic_keys(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE)
		return 0;

#ifdef IEEE8021X_EAPOL
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA &&
	    wpa_s->current_ssid &&
	    !(wpa_s->current_ssid->eapol_flags &
	      (EAPOL_FLAG_REQUIRE_KEY_UNICAST |
	       EAPOL_FLAG_REQUIRE_KEY_BROADCAST))) {
		/* IEEE 802.1X, but not using dynamic WEP keys (i.e., either
		 * plaintext or static WEP keys). */
		return 0;
	}
#endif /* IEEE8021X_EAPOL */

	return 1;
}


#if defined ( CONFIG_SCHED_SCAN )
/**
 * wpa_supplicant_scard_init - Initialize SIM/USIM access with PC/SC
 * @wpa_s: pointer to wpa_supplicant data
 * @ssid: Configuration data for the network
 * Returns: 0 on success, -1 on failure
 *
 * This function is called when starting authentication with a network that is
 * configured to use PC/SC for SIM/USIM access (EAP-SIM or EAP-AKA).
 */
int wpa_supplicant_scard_init(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid)
{
#ifdef IEEE8021X_EAPOL
#ifdef PCSC_FUNCS
	int aka = 0, sim = 0;

	if ((ssid != NULL && ssid->eap.pcsc == NULL) ||
	    wpa_s->scard != NULL || wpa_s->conf->external_sim)
		return 0;

	if (ssid == NULL || ssid->eap.eap_methods == NULL) {
		sim = 1;
		aka = 1;
	} else {
		struct eap_method_type *eap = ssid->eap.eap_methods;
		while (eap->vendor != EAP_VENDOR_IETF ||
		       eap->method != EAP_TYPE_NONE) {
			if (eap->vendor == EAP_VENDOR_IETF) {
				if (eap->method == EAP_TYPE_SIM)
					sim = 1;
				else if (eap->method == EAP_TYPE_AKA ||
					 eap->method == EAP_TYPE_AKA_PRIME)
					aka = 1;
			}
			eap++;
		}
	}

	if (eap_peer_get_eap_method(EAP_VENDOR_IETF, EAP_TYPE_SIM) == NULL)
		sim = 0;
	if (eap_peer_get_eap_method(EAP_VENDOR_IETF, EAP_TYPE_AKA) == NULL &&
	    eap_peer_get_eap_method(EAP_VENDOR_IETF, EAP_TYPE_AKA_PRIME) ==
	    NULL)
		aka = 0;

	if (!sim && !aka) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Selected network is configured to "
			"use SIM, but neither EAP-SIM nor EAP-AKA are "
			"enabled");
		return 0;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "Selected network is configured to use SIM "
		"(sim=%d aka=%d) - initialize PCSC", sim, aka);

	wpa_s->scard = scard_init(wpa_s->conf->pcsc_reader);
	if (wpa_s->scard == NULL) {
		wpa_dbg(wpa_s, MSG_WARNING, "Failed to initialize SIM "
			"(pcsc-lite)");
		return -1;
	}
	wpa_sm_set_scard_ctx(wpa_s->wpa, wpa_s->scard);
	eapol_sm_register_scard_ctx(wpa_s->eapol, wpa_s->scard);
#endif /* PCSC_FUNCS */
#endif /* IEEE8021X_EAPOL */

	return 0;
}
#endif // CONFIG_SCHED_SCAN


#ifndef CONFIG_NO_SCAN_PROCESSING

static int has_wep_key(struct wpa_ssid *ssid)
{
	int i;

	for (i = 0; i < NUM_WEP_KEYS; i++) {
		if (ssid->wep_key_len[i])
			return 1;
	}

	return 0;
}


static int wpa_supplicant_match_privacy(struct wpa_bss *bss,
					struct wpa_ssid *ssid)
{
	int privacy = 0;

	if (ssid->mixed_cell)
		return 1;

#ifdef CONFIG_WPS
	if (ssid->key_mgmt & WPA_KEY_MGMT_WPS)
		return 1;
#endif /* CONFIG_WPS */

#ifdef CONFIG_OWE
	if ((ssid->key_mgmt & WPA_KEY_MGMT_OWE) && !ssid->owe_only)
		return 1;
#endif /* CONFIG_OWE */

	if (has_wep_key(ssid))
		privacy = 1;

#ifdef IEEE8021X_EAPOL
	if ((ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) &&
	    ssid->eapol_flags & (EAPOL_FLAG_REQUIRE_KEY_UNICAST |
				 EAPOL_FLAG_REQUIRE_KEY_BROADCAST))
		privacy = 1;
#endif /* IEEE8021X_EAPOL */

	if (wpa_key_mgmt_wpa(ssid->key_mgmt))
		privacy = 1;

	if (ssid->key_mgmt & WPA_KEY_MGMT_OSEN)
		privacy = 1;

	if (bss->caps & IEEE80211_CAP_PRIVACY)
		return privacy;
	return !privacy;
}


static int wpa_supplicant_ssid_bss_match(struct wpa_supplicant *wpa_s,
					 struct wpa_ssid *ssid,
					 struct wpa_bss *bss, int debug_print)
{
	struct wpa_ie_data ie;
	int proto_match = 0;
	const u8 *rsn_ie=NULL, *wpa_ie=NULL;
	int ret;
	int wep_ok;

	memset(&ie, NULL, sizeof(struct wpa_ie_data));

	ret = wpas_wps_ssid_bss_match(wpa_s, ssid, bss);
	if (ret >= 0)
		return ret;

	/* Allow TSN if local configuration accepts WEP use without WPA/WPA2 */
	wep_ok = !wpa_key_mgmt_wpa(ssid->key_mgmt) &&
		(((ssid->key_mgmt & WPA_KEY_MGMT_NONE) &&
		  ssid->wep_key_len[ssid->wep_tx_keyidx] > 0) ||
		 (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA));

	rsn_ie = wpa_bss_get_ie(bss, WLAN_EID_RSN);
	while ((ssid->proto & (WPA_PROTO_RSN | WPA_PROTO_OSEN)) && rsn_ie) {
		proto_match++;

		if (wpa_parse_wpa_ie(rsn_ie, 2 + rsn_ie[1], &ie)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - parse failed");
			break;
		}

		if (wep_ok &&
		    (ie.group_cipher & (WPA_CIPHER_WEP40 | WPA_CIPHER_WEP104)))
		{
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   selected based on TSN in RSN IE");
			return 1;
		}

		if (!(ie.proto & ssid->proto) &&
		    !(ssid->proto & WPA_PROTO_OSEN)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - proto mismatch");
			break;
		}

		if (!(ie.pairwise_cipher & ssid->pairwise_cipher)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - PTK cipher mismatch");
			break;
		}

		if (!(ie.group_cipher & ssid->group_cipher)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - GTK cipher mismatch");
			break;
		}

		if (ssid->group_mgmt_cipher &&
		    !(ie.mgmt_group_cipher & ssid->group_mgmt_cipher)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - group mgmt cipher mismatch");
			break;
		}

		if (!(ie.key_mgmt & ssid->key_mgmt)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - key mgmt mismatch");
			break;
		}

#ifdef CONFIG_IEEE80211W
		if (!(ie.capabilities & WPA_CAPABILITY_MFPC) &&
		    wpas_get_ssid_pmf(wpa_s, ssid) ==
		    MGMT_FRAME_PROTECTION_REQUIRED) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - no mgmt frame protection");
			break;
		}
#endif /* CONFIG_IEEE80211W */
		if ((ie.capabilities & WPA_CAPABILITY_MFPR) &&
		    wpas_get_ssid_pmf(wpa_s, ssid) ==
		    NO_MGMT_FRAME_PROTECTION) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - no mgmt frame protection enabled but AP requires it - %s",
					wpa_ssid_plain_txt(bss->ssid, bss->ssid_len));
			break;
		}
#ifdef CONFIG_MBO
		if (!(ie.capabilities & WPA_CAPABILITY_MFPC) &&
		    wpas_mbo_get_bss_attr(bss, MBO_ATTR_ID_AP_CAPA_IND) &&
		    wpas_get_ssid_pmf(wpa_s, ssid) !=
		    NO_MGMT_FRAME_PROTECTION) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip RSN IE - no mgmt frame protection enabled on MBO AP");
			break;
		}
#endif /* CONFIG_MBO */

		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   selected based on RSN IE");
		return 1;
	}

#if defined CONFIG_IEEE80211W && defined CONFIG_OWE 
	if (wpas_get_ssid_pmf(wpa_s, ssid) == MGMT_FRAME_PROTECTION_REQUIRED &&
	    (!(ssid->key_mgmt & WPA_KEY_MGMT_OWE)
	    || ssid->owe_only
	    )) {
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   skip - MFP Required but network not MFP Capable");
		return 0;
	}
#endif /* CONFIG_IEEE80211W && CONFIG_OWE */

	wpa_ie = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
	while ((ssid->proto & WPA_PROTO_WPA) && wpa_ie) {
		proto_match++;

		if (wpa_parse_wpa_ie(wpa_ie, 2 + wpa_ie[1], &ie)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip WPA IE - parse failed");
			break;
		}

		if (wep_ok &&
		    (ie.group_cipher & (WPA_CIPHER_WEP40 | WPA_CIPHER_WEP104)))
		{
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   selected based on TSN in WPA IE");
			return 1;
		}

		if (!(ie.proto & ssid->proto)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip WPA IE - proto mismatch");
			break;
		}

		if (!(ie.pairwise_cipher & ssid->pairwise_cipher)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip WPA IE - PTK cipher mismatch");
			break;
		}

		if (!(ie.group_cipher & ssid->group_cipher)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip WPA IE - GTK cipher mismatch");
			break;
		}

		if (!(ie.key_mgmt & ssid->key_mgmt)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip WPA IE - key mgmt mismatch");
			break;
		}

		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   selected based on WPA IE");
		return 1;
	}

	if ((ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) && !wpa_ie &&
	    !rsn_ie) {
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   allow for non-WPA IEEE 802.1X");
		return 1;
	}

#ifdef CONFIG_OWE
#define MAX_OWE_TRANSITION_BSS_SELECT_COUNT 5

	if ((ssid->key_mgmt & WPA_KEY_MGMT_OWE) && !ssid->owe_only &&
	    !wpa_ie && !rsn_ie) {
		if (wpa_s->owe_transition_select &&
		    wpa_bss_get_vendor_ie(bss, OWE_IE_VENDOR_TYPE) &&
		    ssid->owe_transition_bss_select_count + 1 <=
		    MAX_OWE_TRANSITION_BSS_SELECT_COUNT) {
			ssid->owe_transition_bss_select_count++;
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip OWE transition BSS (selection count %d does not exceed %d)",
					ssid->owe_transition_bss_select_count,
					MAX_OWE_TRANSITION_BSS_SELECT_COUNT);
			wpa_s->owe_transition_search = 1;
			return 0;
		}	    
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   allow in OWE transition mode");
		return 1;
	}
#endif /* CONFIG_OWE */

	if ((ssid->proto & (WPA_PROTO_WPA | WPA_PROTO_RSN)) &&
	    wpa_key_mgmt_wpa(ssid->key_mgmt) && proto_match == 0) {
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG,
				"   skip - no WPA/RSN proto match");
		return 0;
	}

	if ((ssid->key_mgmt & WPA_KEY_MGMT_OSEN) &&
	    wpa_bss_get_vendor_ie(bss, OSEN_IE_VENDOR_TYPE)) {
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG, "   allow in OSEN");
		return 1;
	}

	if (!wpa_key_mgmt_wpa(ssid->key_mgmt)) {
		if (debug_print)
			wpa_dbg(wpa_s, MSG_DEBUG, "   allow in non-WPA/WPA2");
		return 1;
	}

	if (debug_print)
		wpa_dbg(wpa_s, MSG_DEBUG,
			"   reject due to mismatch with WPA/WPA2");

	return 0;
}


static int freq_allowed(int *freqs, int freq)
{
	int i;

	if (freqs == NULL)
		return 1;

	for (i = 0; freqs[i]; i++)
		if (freqs[i] == freq)
			return 1;
	return 0;
}

int ht_supported(const struct hostapd_hw_modes *mode)
{
	if (!(mode->flags & HOSTAPD_MODE_FLAG_HT_INFO_KNOWN)) {
		/*
		 * The driver did not indicate whether it supports HT. Assume
		 * it does to avoid connection issues.
		 */
		return 1;
	}

	/*
	 * IEEE Std 802.11n-2009 20.1.1:
	 * An HT non-AP STA shall support all EQM rates for one spatial stream.
	 */
	return mode->mcs_set[0] == 0xff;
}


#ifdef CONFIG_IEEE80211AC
int vht_supported(const struct hostapd_hw_modes *mode)
{
	if (!(mode->flags & HOSTAPD_MODE_FLAG_VHT_INFO_KNOWN)) {
		/*
		 * The driver did not indicate whether it supports VHT. Assume
		 * it does to avoid connection issues.
		 */
		return 1;
	}

	/*
	 * A VHT non-AP STA shall support MCS 0-7 for one spatial stream.
	 * TODO: Verify if this complies with the standard
	 */
	return (mode->vht_mcs_set[0] & 0x3) != 3;
}
#endif /* CONFIG_IEEE80211AC */


static int rate_match(struct wpa_supplicant *wpa_s, struct wpa_bss *bss,
		      int debug_print)
{
	const struct hostapd_hw_modes *mode = NULL, *modes;
	const u8 scan_ie[2] = { WLAN_EID_SUPP_RATES, WLAN_EID_EXT_SUPP_RATES };
	const u8 *rate_ie;
	int i, j, k;

	if (bss->freq == 0)
		return 1; /* Cannot do matching without knowing band */

	modes = wpa_s->hw.modes;
	if (modes == NULL) {
		/*
		 * The driver does not provide any additional information
		 * about the utilized hardware, so allow the connection attempt
		 * to continue.
		 */
		return 1;
	}

	for (i = 0; i < wpa_s->hw.num_modes; i++) {
		for (j = 0; j < modes[i].num_channels; j++) {
			int freq = modes[i].channels[j].freq;
			if (freq == bss->freq) {
				if (mode &&
				    mode->mode == HOSTAPD_MODE_IEEE80211G)
					break; /* do not allow 802.11b replace
						* 802.11g */
				mode = &modes[i];
				break;
			}
		}
	}

	if (mode == NULL)
		return 0;

	for (i = 0; i < (int) sizeof(scan_ie); i++) {
		rate_ie = wpa_bss_get_ie(bss, scan_ie[i]);
		if (rate_ie == NULL)
			continue;

		for (j = 2; j < rate_ie[1] + 2; j++) {
			int flagged = !!(rate_ie[j] & 0x80);
			int r = (rate_ie[j] & 0x7f) * 5;

			/*
			 * IEEE Std 802.11n-2009 7.3.2.2:
			 * The new BSS Membership selector value is encoded
			 * like a legacy basic rate, but it is not a rate and
			 * only indicates if the BSS members are required to
			 * support the mandatory features of Clause 20 [HT PHY]
			 * in order to join the BSS.
			 */
			if (flagged && ((rate_ie[j] & 0x7f) ==
					BSS_MEMBERSHIP_SELECTOR_HT_PHY)) {
				if (!ht_supported(mode)) {
					if (debug_print)
						wpa_dbg(wpa_s, MSG_DEBUG,
							"   hardware does not support HT PHY");
					return 0;
				}
				continue;
			}

#ifdef CONFIG_IEEE80211AC
			/* There's also a VHT selector for 802.11ac */
			if (flagged && ((rate_ie[j] & 0x7f) ==
					BSS_MEMBERSHIP_SELECTOR_VHT_PHY)) {
				if (!vht_supported(mode)) {
					if (debug_print)
						wpa_dbg(wpa_s, MSG_DEBUG,
							"   hardware does not support VHT PHY");
					return 0;
				}
				continue;
			}
#endif /* CONFIG_IEEE80211AC */

			if (!flagged)
				continue;

			/* check for legacy basic rates */
			for (k = 0; k < mode->num_rates; k++) {
				if (mode->rates[k] == r)
					break;
			}
			if (k == mode->num_rates) {
				/*
				 * IEEE Std 802.11-2007 7.3.2.2 demands that in
				 * order to join a BSS all required rates
				 * have to be supported by the hardware.
				 */
				if (debug_print)
					wpa_dbg(wpa_s, MSG_DEBUG,
						"   hardware does not support required rate %d.%d Mbps (freq=%d mode==%d num_rates=%d)",
						r / 10, r % 10,
						bss->freq, mode->mode, mode->num_rates);
				return 0;
			}
		}
	}

	return 1;
}


/*
 * Test whether BSS is in an ESS.
 * This is done differently in DMG (60 GHz) and non-DMG bands
 */
static int bss_is_ess(struct wpa_bss *bss)
{
	if (bss_is_dmg(bss)) {
		return (bss->caps & IEEE80211_CAP_DMG_MASK) ==
			IEEE80211_CAP_DMG_AP;
	}

	return ((bss->caps & (IEEE80211_CAP_ESS | IEEE80211_CAP_IBSS)) ==
		IEEE80211_CAP_ESS);
}

#ifdef CONFIG_P2P_UNUSED_CMD
static int match_mac_mask(const u8 *addr_a, const u8 *addr_b, const u8 *mask)
{
	size_t i;

	for (i = 0; i < ETH_ALEN; i++) {
		if ((addr_a[i] & mask[i]) != (addr_b[i] & mask[i]))
			return 0;
	}
	return 1;
}


static int addr_in_list(const u8 *addr, const u8 *list, size_t num)
{
	size_t i;

	for (i = 0; i < num; i++) {
		const u8 *a = list + i * ETH_ALEN * 2;
		const u8 *m = a + ETH_ALEN;

		if (match_mac_mask(a, addr, m))
			return 1;
	}
	return 0;
}
#endif /* CONFIG_P2P_UNUSED_CMD */


#ifdef CONFIG_OWE
static void owe_trans_ssid(struct wpa_supplicant *wpa_s, struct wpa_bss *bss,
			   const u8 **ret_ssid, size_t *ret_ssid_len)
{
	const u8 *owe, *pos, *end, *bssid;
	u8 ssid_len;
	struct wpa_bss *open_bss;

	owe = wpa_bss_get_vendor_ie(bss, OWE_IE_VENDOR_TYPE);
	if (!owe || !wpa_bss_get_ie(bss, WLAN_EID_RSN))
		return;

	pos = owe + 6;
	end = owe + 2 + owe[1];

	if (end - pos < ETH_ALEN + 1)
		return;
	bssid = pos;
	pos += ETH_ALEN;
	ssid_len = *pos++;
	if (end - pos < ssid_len || ssid_len > SSID_MAX_LEN)
		return;

	/* Match the profile SSID against the OWE transition mode SSID on the
	 * open network. */
	wpa_dbg(wpa_s, MSG_DEBUG, "OWE: transition mode BSSID: " MACSTR
		" SSID: %s", MAC2STR(bssid), wpa_ssid_txt(pos, ssid_len));
	*ret_ssid = pos;
	*ret_ssid_len = ssid_len;

	if (bss->ssid_len > 0)
		return;

	open_bss = wpa_bss_get_bssid_latest(wpa_s, bssid);
	if (!open_bss)
		return;
	if (ssid_len != open_bss->ssid_len ||
	    os_memcmp(pos, open_bss->ssid, ssid_len) != 0) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"OWE: transition mode SSID mismatch: %s",
			wpa_ssid_txt(open_bss->ssid, open_bss->ssid_len));
		return;
	}

	owe = wpa_bss_get_vendor_ie(open_bss, OWE_IE_VENDOR_TYPE);
	if (!owe || wpa_bss_get_ie(open_bss, WLAN_EID_RSN)) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"OWE: transition mode open BSS unexpected info");
		return;
	}

	pos = owe + 6;
	end = owe + 2 + owe[1];

	if (end - pos < ETH_ALEN + 1)
		return;
	if (os_memcmp(pos, bss->bssid, ETH_ALEN) != 0) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"OWE: transition mode BSSID mismatch: " MACSTR,
			MAC2STR(pos));
		return;
	}
	pos += ETH_ALEN;
	ssid_len = *pos++;
	if (end - pos < ssid_len || ssid_len > SSID_MAX_LEN)
		return;
	wpa_dbg(wpa_s, MSG_DEBUG, "OWE: learned transition mode OWE SSID: %s",
		wpa_ssid_txt(pos, ssid_len));
	os_memcpy(bss->ssid, pos, ssid_len);
	bss->ssid_len = ssid_len;
}
#endif /* CONFIG_OWE */


struct wpa_ssid * wpa_scan_res_match(struct wpa_supplicant *wpa_s,
				     int i, struct wpa_bss *bss,
				     struct wpa_ssid *group,
				     int only_first_ssid, int debug_print)
{
	u8 wpa_ie_len, rsn_ie_len;
	int wpa;
	struct wpa_blacklist *e=NULL;
	const u8 *ie=NULL;
	struct wpa_ssid *ssid=NULL;
	int osen, rsn_osen = 0;
#ifdef CONFIG_MBO
	const u8 *assoc_disallow;
#endif /* CONFIG_MBO */
#ifdef	CONFIG_OWE
	const u8 *match_ssid;
	size_t match_ssid_len;
#endif	/* CONFIG_OWE */
	struct wpa_ie_data data;

	if(bss == NULL)
		return NULL;

	ie = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
	wpa_ie_len = ie ? ie[1] : 0;

	ie = wpa_bss_get_ie(bss, WLAN_EID_RSN);
	rsn_ie_len = ie ? ie[1] : 0;
	if (ie && wpa_parse_wpa_ie_rsn(ie, 2 + ie[1], &data) == 0 &&
	    (data.key_mgmt & WPA_KEY_MGMT_OSEN))
		rsn_osen = 1;

	ie = wpa_bss_get_vendor_ie(bss, OSEN_IE_VENDOR_TYPE);
	osen = ie != NULL;

#ifdef	CONFIG_P2P
	da16x_scan_prt("          [%s] %d: " MACSTR " (%s) "
		"wpa_ie_len=%u rsn_ie_len=%u caps=0x%x lvl=%d, %s%s%s\n",
		__func__,
		i, MAC2STR(bss->bssid), wpa_ssid_txt(bss->ssid, bss->ssid_len),
		wpa_ie_len, rsn_ie_len, bss->caps, bss->level,
		wpa_bss_get_vendor_ie(bss, WPS_IE_VENDOR_TYPE) ? " wps" : "",
		(wpa_bss_get_vendor_ie(bss, P2P_IE_VENDOR_TYPE) ||
		 wpa_bss_get_vendor_ie_beacon(bss, P2P_IE_VENDOR_TYPE)) ?
		" p2p" : "",
		osen ? " osen=1" : "");
#else
	da16x_scan_prt("          [%s] %d: " MACSTR " (%s) "
		"wpa_ie_len=%u rsn_ie_len=%u caps=0x%x lvl=%d, %s%s\n",
		__func__,
		i, MAC2STR(bss->bssid), wpa_ssid_txt(bss->ssid, bss->ssid_len),
		wpa_ie_len, rsn_ie_len, bss->caps, bss->level,
		wpa_bss_get_vendor_ie(bss, WPS_IE_VENDOR_TYPE) ? " wps" : "",
		osen ? " osen=1" : "");
#endif	/* CONFIG_P2P */

	e = wpa_blacklist_get(wpa_s, bss->bssid);
	if (e) {
		int limit = 1;
		if (wpa_supplicant_enabled_networks(wpa_s) == 1) {
			/*
			 * When only a single network is enabled, we can
			 * trigger blacklisting on the first failure. This
			 * should not be done with multiple enabled networks to
			 * avoid getting forced to move into a worse ESS on
			 * single error if there are no other BSSes of the
			 * current ESS.
			 */
			limit = 0;
		}
		if (e->count > limit) {
			da16x_debug_prt("[%s] skip- blacklisted "
					"(count=%d limit=%d)\n",
					__func__,  e->count, limit);
			return NULL;
		}
	}

#ifdef	CONFIG_OWE
	match_ssid = bss->ssid;
	match_ssid_len = bss->ssid_len;
	owe_trans_ssid(wpa_s, bss, &match_ssid, &match_ssid_len);

	if (match_ssid_len == 0) {
		da16x_debug_prt("[%s] SKIP - SSID not known - %s\n", __func__, wpa_ssid_plain_txt(match_ssid, match_ssid_len));
		return NULL;
	}
#endif	/* CONFIG_OWE */

#ifdef	CONFIG_DISALLOW_BSSID
	if (disallowed_bssid(wpa_s, bss->bssid)) {
		da16x_debug_prt("[%s] SKIP - BSSID disallowed\n", __func__);
		return NULL;
	}
#endif	/* CONFIG_DISALLOW_BSSID */

#ifdef	CONFIG_OWE
#ifdef	CONFIG_DISALLOW_SSID
	if (disallowed_ssid(wpa_s, match_ssid, match_ssid_len)) {
		da16x_debug_prt("[%s] SKIP - SSID disallowed\n", __func__);
		return NULL;
	}
#endif	/* CONFIG_DISALLOW_SSID */
#endif	/* CONFIG_OWE */

	wpa = wpa_ie_len > 0 || rsn_ie_len > 0;

#ifdef CONFIG_PRIO_GROUP
	for (ssid = group; ssid; ssid = only_first_ssid ? NULL : ssid->pnext)
#else
	for (ssid = group; ssid; ssid = only_first_ssid ? NULL : ssid->next)
#endif /* CONFIG_PRIO_GROUP */
	{
		int check_ssid, res;
#ifdef CONFIG_ALLOW_ANY_OPEN_AP
		if (wpa_s->conf->allow_any_open_ap) {
			check_ssid = wpa ? 1 : (ssid->ssid_len != 0);
		} else {
			check_ssid = 1;
		}
#else
		check_ssid = wpa ? 1 : (ssid->ssid_len != 0);
#endif /* CONFIG_ALLOW_ANY_OPEN_AP */

		if (wpas_network_disabled(wpa_s, ssid)) {
			da16x_debug_prt("[%s] SKIP - disabled\n", __func__);
			continue;
		}

		res = wpas_temp_disabled(wpa_s, ssid);
		if (res > 0) {
			da16x_debug_prt("[%s] SKIP - disabled temporarily for %d second(s)\n",
					__func__, res);
			continue;
		}

#ifdef CONFIG_WPS
		if ((ssid->key_mgmt & WPA_KEY_MGMT_WPS) && e && e->count > 0) {
			da16x_debug_prt("[%s] SKIP - blacklisted (WPS)\n", __func__);
			continue;
		}

		if (wpa && ssid->ssid_len == 0 &&
		    wpas_wps_ssid_wildcard_ok(wpa_s, ssid, bss))
			check_ssid = 0;

		if (!wpa && (ssid->key_mgmt & WPA_KEY_MGMT_WPS)) {
			/* Only allow wildcard SSID match if an AP
			 * advertises active WPS operation that matches
			 * with our mode. */
			check_ssid = 1;
			if (ssid->ssid_len == 0 &&
			    wpas_wps_ssid_wildcard_ok(wpa_s, ssid, bss))
				check_ssid = 0;
		}
#endif /* CONFIG_WPS */

		if (ssid->bssid_set && ssid->ssid_len == 0 &&
		    os_memcmp(bss->bssid, ssid->bssid, ETH_ALEN) == 0)
			check_ssid = 0;

#ifdef	CONFIG_OWE
		if (check_ssid &&
		    (match_ssid_len != ssid->ssid_len ||
		     os_memcmp(match_ssid, ssid->ssid, match_ssid_len) != 0)) {
			da16x_debug_prt("[%s] SKIP - SSID mismatch - %s\n", __func__, wpa_ssid_plain_txt(match_ssid, match_ssid_len));
			continue;
		}
#else
		if (check_ssid &&
			(bss->ssid_len != ssid->ssid_len ||
			os_memcmp(bss->ssid, ssid->ssid, bss->ssid_len) != 0)) {
			da16x_debug_prt("[%s] SKIP - SSID mismatch\n", __func__);
			continue;
		}
#endif	/* CONFIG_OWE */

		if (ssid->bssid_set &&
		    os_memcmp(bss->bssid, ssid->bssid, ETH_ALEN) != 0) {
			da16x_debug_prt("[%s] SKIP - BSSID mismatch\n", __func__);
			continue;
		}

#ifdef	CONFIG_P2P_UNUSED_CMD
		/* check blacklist */
		if (ssid->num_bssid_blacklist &&
		    addr_in_list(bss->bssid, ssid->bssid_blacklist,
				 ssid->num_bssid_blacklist)) {
			da16x_debug_prt("[%s] SKIP - BSSID blacklisted\n", __func__);
			continue;
		}

		/* if there is a whitelist, only accept those APs */
		if (ssid->num_bssid_whitelist &&
		    !addr_in_list(bss->bssid, ssid->bssid_whitelist,
				  ssid->num_bssid_whitelist)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - BSSID not in whitelist");
			continue;
		}
#endif	/* CONFIG_P2P_UNUSED_CMD */

		if (!wpa_supplicant_ssid_bss_match(wpa_s, ssid, bss,
						   debug_print))
			continue;

		if (!osen && !wpa &&
		    !(ssid->key_mgmt & WPA_KEY_MGMT_NONE) &&
		    !(ssid->key_mgmt & WPA_KEY_MGMT_WPS) &&
#ifdef CONFIG_OWE
		    !(ssid->key_mgmt & WPA_KEY_MGMT_OWE) &&
#endif /* CONFIG_OWE */
		    !(ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - non-WPA network not allowed");
			continue;
		}

		if (wpa && !wpa_key_mgmt_wpa(ssid->key_mgmt) &&
		    has_wep_key(ssid)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - ignore WPA/WPA2 AP for WEP network block");
			continue;
		}

		if ((ssid->key_mgmt & WPA_KEY_MGMT_OSEN) && !osen &&
		    !rsn_osen) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - non-OSEN network not allowed");
			continue;
		}

		if (!wpa_supplicant_match_privacy(bss, ssid)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - privacy mismatch");
			continue;
		}

		if (ssid->mode != IEEE80211_MODE_MESH && !bss_is_ess(bss) &&
		    !bss_is_pbss(bss)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - not ESS, PBSS, or MBSS");
			continue;
		}

		if (ssid->pbss != 2 && ssid->pbss != bss_is_pbss(bss)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - PBSS mismatch (ssid %d bss %d)",
					ssid->pbss, bss_is_pbss(bss));
			continue;
		}

		if (!freq_allowed(ssid->freq_list, bss->freq)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - frequency not allowed");
			continue;
		}

#ifdef CONFIG_MESH
		if (ssid->mode == IEEE80211_MODE_MESH && ssid->frequency > 0 &&
		    ssid->frequency != bss->freq) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - frequency not allowed (mesh)");
			continue;
		}
#endif /* CONFIG_MESH */

		if (!rate_match(wpa_s, bss, debug_print)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - rate sets do not match");
			continue;
		}

#ifndef CONFIG_IBSS_RSN
		if (ssid->mode == WPAS_MODE_IBSS &&
		    !(ssid->key_mgmt & (WPA_KEY_MGMT_NONE |
					WPA_KEY_MGMT_WPA_NONE))) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - IBSS RSN not supported in the build");
			continue;
		}
#endif /* !CONFIG_IBSS_RSN */

#ifdef CONFIG_P2P
		if (ssid->p2p_group &&
		    !wpa_bss_get_vendor_ie(bss, P2P_IE_VENDOR_TYPE) &&
		    !wpa_bss_get_vendor_ie_beacon(bss, P2P_IE_VENDOR_TYPE)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - no P2P IE seen");
			continue;
		}

		if (!is_zero_ether_addr(ssid->go_p2p_dev_addr)) {
			struct wpabuf *p2p_ie;
			u8 dev_addr[ETH_ALEN];

			ie = wpa_bss_get_vendor_ie(bss, P2P_IE_VENDOR_TYPE);
			if (ie == NULL) {
				if (debug_print)
					wpa_dbg(wpa_s, MSG_DEBUG,
						"   skip - no P2P element");
				continue;
			}
			p2p_ie = wpa_bss_get_vendor_ie_multi(
				bss, P2P_IE_VENDOR_TYPE);
			if (p2p_ie == NULL) {
				if (debug_print)
					wpa_dbg(wpa_s, MSG_DEBUG,
						"   skip - could not fetch P2P element");
				continue;
			}

			if (p2p_parse_dev_addr_in_p2p_ie(p2p_ie, dev_addr) < 0
			    || os_memcmp(dev_addr, ssid->go_p2p_dev_addr,
					 ETH_ALEN) != 0) {
				if (debug_print)
					wpa_dbg(wpa_s, MSG_DEBUG,
						"   skip - no matching GO P2P Device Address in P2P element");
				wpabuf_free(p2p_ie);
				continue;
			}
			wpabuf_free(p2p_ie);
		}

		/*
		 * TODO: skip the AP if its P2P IE has Group Formation
		 * bit set in the P2P Group Capability Bitmap and we
		 * are not in Group Formation with that device.
		 */
#endif /* CONFIG_P2P */

		if (os_reltime_before(&bss->last_update, &wpa_s->scan_min_time))
		{
			struct os_reltime diff;

			os_reltime_sub(&wpa_s->scan_min_time,
				       &bss->last_update, &diff);
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - scan result not recent enough (%u.%06u seconds too old)",
				(unsigned int) diff.sec,
				(unsigned int) diff.usec);
			continue;
		}
#ifdef CONFIG_MBO
#ifdef CONFIG_TESTING_OPTIONS
		if (wpa_s->ignore_assoc_disallow)
			goto skip_assoc_disallow;
#endif /* CONFIG_TESTING_OPTIONS */
		assoc_disallow = wpas_mbo_get_bss_attr(
			bss, MBO_ATTR_ID_ASSOC_DISALLOW);
		if (assoc_disallow && assoc_disallow[1] >= 1) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - MBO association disallowed (reason %u)",
				assoc_disallow[2]);
			continue;
		}

		if (wpa_is_bss_tmp_disallowed(wpa_s, bss->bssid)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - MBO retry delay has not passed yet");
			continue;
		}
#ifdef CONFIG_TESTING_OPTIONS
	skip_assoc_disallow:
#endif /* CONFIG_TESTING_OPTIONS */
#endif /* CONFIG_MBO */

#ifdef CONFIG_DPP
		if ((ssid->key_mgmt & WPA_KEY_MGMT_DPP) &&
		    !wpa_sm_pmksa_exists(wpa_s->wpa, bss->bssid, ssid) &&
		    (!ssid->dpp_connector ||
		     !ssid->dpp_netaccesskey ||
		     !ssid->dpp_csign)) {
			if (debug_print)
				wpa_dbg(wpa_s, MSG_DEBUG,
					"   skip - no PMKSA entry for DPP");
			continue;
		}
#endif /* CONFIG_DPP */

		/* Matching configuration found */
		return ssid;
	}

	/* No matching configuration found */
	return NULL;
}


static struct wpa_bss *
wpa_supplicant_select_bss(struct wpa_supplicant *wpa_s,
			  struct wpa_ssid *group,
			  struct wpa_ssid **selected_ssid,
			  int only_first_ssid)
{
	unsigned int i;

#ifdef	CONFIG_SUPP27_EVENTS
	if (wpa_s->current_ssid) {
		struct wpa_ssid *ssid;

		da16x_ev_prt("[%s] Scan results matching the currently selected network\n", __func__);
		for (i = 0; i < wpa_s->last_scan_res_used; i++) {
			struct wpa_bss *bss = wpa_s->last_scan_res[i];

			ssid = wpa_scan_res_match(wpa_s, i, bss, group, only_first_ssid, 0);
			if (ssid != wpa_s->current_ssid)
				continue;
#if 0
			da16x_assoc_prt("[%s] %u" MACSTR " ssid='%s' (%d)\n",
				__func__,
				i, MAC2STR(bss->bssid),
				wpa_ssid_txt(bss->ssid, bss->ssid_len), bss->level);
#endif /* 0 */
		}
	}
#endif	/* CONFIG_SUPP27_EVENTS */

	if (only_first_ssid) {
		da16x_ev_prt("[%s] Try to find BSS matching pre-selected network id=%d\n",
			__func__, group->id);
	}
#ifdef CONFIG_PRIO_GROUP
	else {
		da16x_ev_prt("[%s] Selecting BSS from priority group %d\n",
			__func__, group->priority);
	}
#endif /* CONFIG_PRIO_GROUP */

	for (i = 0; i < wpa_s->last_scan_res_used; i++) {
		struct wpa_bss *bss = wpa_s->last_scan_res[i];
#ifdef CONFIG_OWE
		wpa_s->owe_transition_select = 1;
#endif
		*selected_ssid = wpa_scan_res_match(wpa_s, i, bss, group, only_first_ssid, 1);
#ifdef CONFIG_OWE
		wpa_s->owe_transition_select = 0;
#endif
		if (!*selected_ssid)
			continue;

#ifdef	CONFIG_SIMPLE_ROAMING
        if (((wpa_s->conf->roam_state >= ROAM_ENABLE))
                && (strlen((char *)wpa_s->conf->candi_bssid) > 0)) {
    		if (bss->ssid_len == wpa_s->current_ssid->ssid_len &&
    				os_memcmp(bss->ssid, wpa_s->current_ssid->ssid, bss->ssid_len) == 0) {
    			PRINTF(">>> Roam Scan[%d/%d] BSS " MACSTR " ssid='%s' (%d)\n",
    					i,
    					wpa_s->last_scan_res_used,
    					MAC2STR(bss->bssid),
    					wpa_ssid_txt(bss->ssid, bss->ssid_len), bss->level);
    		}
        }
#endif /* CONFIG_SIMPLE_ROAMING */

		if (wpa_s->scan_for_connection) {
			da16x_notice_prt(">>> Selected BSS " MACSTR " ssid='%s' (%d)\n",
					MAC2STR(bss->bssid),
					wpa_ssid_txt(bss->ssid, bss->ssid_len), bss->level);
		}
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(bss->internal_bss);				
#endif	/* CONFIG_REUSED_UMAC_BSS_LIST */	
		return bss;
	}

	return NULL;
}


struct wpa_bss * wpa_supplicant_pick_network(struct wpa_supplicant *wpa_s,
					     struct wpa_ssid **selected_ssid)
{
	struct wpa_bss *selected = NULL;
#ifdef CONFIG_PRIO_GROUP
	int prio;
	struct wpa_ssid *next_ssid = NULL;
#endif /* CONFIG_PRIO_GROUP */
	struct wpa_ssid *ssid;

	RX_FUNC_START("")

	if (wpa_s->last_scan_res == NULL ||
	    wpa_s->last_scan_res_used == 0)
		return NULL; /* no scan results from last update */

#ifdef CONFIG_PRIO_GROUP
	if (wpa_s->next_ssid) {
		/* check that next_ssid is still valid */
		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
			if (ssid == wpa_s->next_ssid)
				break;
		}
		next_ssid = ssid;
		wpa_s->next_ssid = NULL;
	}
#endif /* CONFIG_PRIO_GROUP */

	while (selected == NULL) {
#ifdef CONFIG_PRIO_GROUP
		for (prio = 0; prio < wpa_s->conf->num_prio; prio++) {
			if (next_ssid && next_ssid->priority ==
			    wpa_s->conf->pssid[prio]->priority) {
				selected = wpa_supplicant_select_bss(
					wpa_s, next_ssid, selected_ssid, 1);
				if (selected)
					break;
			}
			selected = wpa_supplicant_select_bss(
				wpa_s, wpa_s->conf->pssid[prio],
				selected_ssid, 0);
			if (selected)
				break;
		}
#else
		selected = wpa_supplicant_select_bss(
			wpa_s, wpa_s->conf->ssid, selected_ssid, 0);

		if (selected)
			break;
#endif /* CONFIG_PRIO_GROUP */
		if (selected == NULL && wpa_s->blacklist &&
		    !wpa_s->countermeasures) {
			da16x_notice_prt("\n!!! No proper APs found - "
					"It will be try again !!!\n\n");

			wpa_blacklist_clear(wpa_s);
			wpa_s->blacklist_cleared++;
		} else if (selected == NULL)
			break;
	}

	ssid = *selected_ssid;
	if (selected && ssid && ssid->mem_only_psk && !ssid->psk_set &&
	    !ssid->passphrase && !ssid->ext_psk) {
		const char *field_name, *txt = NULL;

		da16x_notice_prt("[%s] PSK/passphrase not yet available for the selected network\n",
				__func__);

#ifdef CONFIG_NOTIFY
		wpas_notify_network_request(wpa_s, ssid,
					    WPA_CTRL_REQ_PSK_PASSPHRASE, NULL);
#endif /* CONFIG_NOTIFY */

		field_name = wpa_supplicant_ctrl_req_to_string(
			WPA_CTRL_REQ_PSK_PASSPHRASE, NULL, &txt);
		if (field_name == NULL)
			return NULL;

		wpas_send_ctrl_req(wpa_s, ssid, field_name, txt);

		selected = NULL;
	}

	RX_FUNC_END("")
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
	if(selected != NULL)
		supp_rehold_bss(selected->internal_bss);	
#endif
	return selected;
}


static void wpa_supplicant_req_new_scan(struct wpa_supplicant *wpa_s,
					int timeout_sec, int timeout_usec)
{
	TX_FUNC_START("  ");
	if (!wpa_supplicant_enabled_networks(wpa_s)) {
		/*
		 * No networks are enabled; short-circuit request so
		 * we don't wait timeout seconds before transitioning
		 * to INACTIVE state.
		 */
		da16x_ev_prt("[%s] Short-circuit new scan request "
				"since there are no enabled networks\n", __func__);
		wpa_supplicant_set_state(wpa_s, WPA_INACTIVE);
		return;
	}

#if 1	/* WPS debugging */
	wpa_s->scan_for_connection = 1;
#endif	/* 1 */

	wpa_supplicant_req_scan(wpa_s, timeout_sec, timeout_usec);
	TX_FUNC_END("  ");
}


int wpa_supplicant_connect(struct wpa_supplicant *wpa_s,
			   struct wpa_bss *selected,
			   struct wpa_ssid *ssid)
{
#ifdef	CONFIG_WPS
	if (wpas_wps_scan_pbc_overlap(wpa_s, selected, ssid)) {
		da16x_notice_prt(WPS_EVENT_OVERLAP "PBC session overlap\n");

#if 0	/* by Bright : Merge 2.7 */
		wpas_notify_wps_event_pbc_overlap(wpa_s);
#endif	/* 0 */
#ifdef CONFIG_P2P
		if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT ||
		    wpa_s->p2p_in_provisioning) {
			da16x_eloop_register_timeout(0, 0, wpas_p2p_pbc_overlap_cb,
					       wpa_s, NULL);
			return -1;
		}
#endif /* CONFIG_P2P */

#ifdef CONFIG_WPS
//		wpas_wps_pbc_overlap(wpa_s);
#if 1 /* FC9000 Only */
		wpas_wps_cancel(wpa_s, NULL);
#else
		wpas_wps_cancel(wpa_s);
#endif /* FC9000 Only */
#endif /* CONFIG_WPS */
		return -1;
	}
#endif	/* CONFIG_WPS */

#if 1
	wpa_dbg(wpa_s, MSG_DEBUG,
		"Considering connect request: reassociate: %d  selected: "
		MACSTR "  bssid: " MACSTR "  pending: " MACSTR
		"  wpa_state: %s (%d)  ssid=%p  current_ssid=%p",
		wpa_s->reassociate, MAC2STR(selected->bssid),
		MAC2STR(wpa_s->bssid), MAC2STR(wpa_s->pending_bssid),
		wpa_supplicant_state_txt(wpa_s->wpa_state), wpa_s->wpa_state,
		ssid, wpa_s->current_ssid);
#else
	da16x_assoc_prt("[%s] Considering connect request: \n"
			"\t\treassociate: %d selected: " MACSTR
			"\n\t\tbssid: " MACSTR
			"\n\t\tpending: " MACSTR
			"\n\t\twpa_state: %d"
			"\n\t\tssid=%p, current_ssid=%p\n",
			__func__,
			wpa_s->reassociate, MAC2STR(selected->bssid),
			MAC2STR(wpa_s->bssid), MAC2STR(wpa_s->pending_bssid),
			wpa_s->wpa_state,
			ssid, wpa_s->current_ssid);
#endif /* 1 */

	/*
	 * Do not trigger new association unless the BSSID has changed or if
	 * reassociation is requested. If we are in process of associating with
	 * the selected BSSID, do not trigger new attempt.
	 */
	if (wpa_s->reassociate ||
	    (os_memcmp(selected->bssid, wpa_s->bssid, ETH_ALEN) != 0 &&
	     ((wpa_s->wpa_state != WPA_ASSOCIATING &&
	       wpa_s->wpa_state != WPA_AUTHENTICATING) ||
	      (!is_zero_ether_addr(wpa_s->pending_bssid) &&
	       os_memcmp(selected->bssid, wpa_s->pending_bssid, ETH_ALEN) !=
	       0) ||
	      (is_zero_ether_addr(wpa_s->pending_bssid) &&
	       ssid != wpa_s->current_ssid)))) {
#if defined ( CONFIG_SCHED_SCAN )
		if (wpa_supplicant_scard_init(wpa_s, ssid)) {
			wpa_supplicant_req_new_scan(wpa_s, 10, 0);
			return 0;
		}
#endif // CONFIG_SCHED_SCAN
		da16x_assoc_prt("[%s] Request association with " MACSTR "\n",
			__func__, MAC2STR(selected->bssid));
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(selected->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_supplicant_associate(wpa_s, selected, ssid);
	} else {
		da16x_notice_prt("[%s] Already associated or trying to "
				"connect with the selected AP\n", __func__);
	}

	return 0;
}

#ifdef	CONFIG_AP
static struct wpa_ssid *
wpa_supplicant_pick_new_network(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;
#ifdef CONFIG_PRIO_GROUP
	int prio;
	struct wpa_ssid *ssid;

	for (prio = 0; prio < wpa_s->conf->num_prio; prio++) {
		for (ssid = wpa_s->conf->pssid[prio]; ssid; ssid = ssid->pnext)
		{
			if (wpas_network_disabled(wpa_s, ssid))
				continue;
#ifndef CONFIG_IBSS_RSN
			if (ssid->mode == WPAS_MODE_IBSS &&
			    !(ssid->key_mgmt & (WPA_KEY_MGMT_NONE |
						WPA_KEY_MGMT_WPA_NONE))) {
				wpa_msg(wpa_s, MSG_INFO,
					"IBSS RSN not supported in the build - cannot use the profile for SSID '%s'",
					wpa_ssid_txt(ssid->ssid,
						     ssid->ssid_len));
				continue;
			}
#endif /* !CONFIG_IBSS_RSN */
			if (ssid->mode == IEEE80211_MODE_IBSS ||
			    ssid->mode == IEEE80211_MODE_AP ||
			    ssid->mode == IEEE80211_MODE_MESH)
				return ssid;
		}
	}
#else
	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next)
	{
		if (wpas_network_disabled(wpa_s, ssid))
			continue;
		if (ssid->mode == IEEE80211_MODE_IBSS ||
		    ssid->mode == IEEE80211_MODE_AP)
			return ssid;
	}
#endif /* CONFIG_PRIO_GROUP */
	return NULL;
}
#endif	/* CONFIG_AP */


#ifdef	CONFIG_PRE_AUTH
/* TODO: move the rsn_preauth_scan_result*() to be called from notify.c based
 * on BSS added and BSS changed events */
static void wpa_supplicant_rsn_preauth_scan_results(
	struct wpa_supplicant *wpa_s)
{
	struct wpa_bss *bss;

	if (rsn_preauth_scan_results(wpa_s->wpa) < 0)
		return;

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		const u8 *ssid, *rsn;

		ssid = wpa_bss_get_ie(bss, WLAN_EID_SSID);
		if (ssid == NULL)
			continue;

		rsn = wpa_bss_get_ie(bss, WLAN_EID_RSN);
		if (rsn == NULL)
			continue;

		rsn_preauth_scan_result(wpa_s->wpa, bss->bssid, ssid, rsn);
	}

}
#endif	/* CONFIG_PRE_AUTH */

#ifdef	CONFIG_SIMPLE_ROAMING
static int wpas_simple_roaming_proc(struct wpa_supplicant *wpa_s, struct wpa_bss *selected)
{
	struct wpa_config *conf = wpa_s->conf;

	if (0 == os_memcmp(conf->candi_bssid, selected->bssid, ETH_ALEN)) {
		if (conf->roam_cnt <= 0) {
			/* Data init - Roam process finished */
			os_memset(conf->candi_bssid, 0, ETH_ALEN);
			conf->roam_cnt = ROAM_SUSPEND_CNT;
			if (wpa_s->conf->roam_state == ROAM_SCANNING)
				wpa_s->conf->roam_state = ROAM_ENABLE;
			da16x_scan_prt("[%s] Selected BSSID "MACSTR" \n",
					__func__, MAC2STR(selected->bssid));

			da16x_eloop_register_timeout(0, 0, wpa_supplicant_reconfig_net, wpa_s, NULL); /* 0 ms */
			return 0;
		} else {
			PRINTF(GREEN_COLOR">>> [Roaming]   %d  - BSSID["MACSTR"], Level[%d]\n" CLEAR_COLOR,
					conf->roam_cnt,
					MAC2STR(selected->bssid),
					selected->level);
			conf->roam_cnt--;
		}
	} else {
		PRINTF(GREEN_COLOR "\n>>> [Roaming] New  - BSSID["MACSTR"], Level[%d]\n" CLEAR_COLOR,
				MAC2STR(selected->bssid), selected->level);

		os_memset(conf->candi_bssid, 0, ETH_ALEN);
		os_memcpy(conf->candi_bssid, selected->bssid, ETH_ALEN);
		conf->roam_cnt = ROAM_SUSPEND_CNT;
	}

	return -1;
}
#endif	/* CONFIG_SIMPLE_ROAMING */

static int wpa_supplicant_need_to_roam(struct wpa_supplicant *wpa_s,
				       struct wpa_bss *selected,
				       struct wpa_ssid *ssid)
{
	struct wpa_bss *current_bss = NULL;
#ifndef CONFIG_NO_ROAMING
	int min_diff, diff;
	int to_5ghz;
	int cur_est, sel_est;
#endif /* CONFIG_NO_ROAMING */

	if (wpa_s->reassociate)
		return 1; /* explicit request to reassociate */
	if (wpa_s->wpa_state < WPA_ASSOCIATED)
		return 1; /* we are not associated; continue */
	if (wpa_s->current_ssid == NULL)
		return 1; /* unknown current SSID */
	if (wpa_s->current_ssid != ssid)
		return 1; /* different network block */

	if (wpas_driver_bss_selection(wpa_s))
		return 0; /* Driver-based roaming */

	if (wpa_s->current_ssid->ssid)
		current_bss = wpa_bss_get(wpa_s, wpa_s->bssid,
					  wpa_s->current_ssid->ssid,
					  wpa_s->current_ssid->ssid_len);
	if (!current_bss)
		current_bss = wpa_bss_get_bssid(wpa_s, wpa_s->bssid);

	if (!current_bss)
		return 1; /* current BSS not seen in scan results */

#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpa_s->conf->roam_state == ROAM_DISABLE)
		return 0;

	if ((current_bss->level < 0)
		&& (current_bss->level > wpa_s->conf->roam_threshold)){

		os_memset(wpa_s->conf->candi_bssid, 0, ETH_ALEN);
		wpa_s->conf->roam_cnt = ROAM_SUSPEND_CNT;

		if (wpa_s->conf->roam_state == ROAM_SCANNING){
			wpa_s->conf->roam_state = ROAM_ENABLE;
			fc80211_set_roaming_mode(1);
		}
		if (wpa_s->conf->roam_cnt < ROAM_SUSPEND_CNT) {
			PRINTF(">>> [Roaming] Skip - BSSID["MACSTR"] signal level[%d]"
				"> Threshold[%d].\n",
				MAC2STR(current_bss->bssid),
				current_bss->level,
				wpa_s->conf->roam_threshold);
		}
		return 0;
	}
#endif	/* CONFIG_SIMPLE_ROAMING */

	if (current_bss == selected) {
#ifdef	CONFIG_SIMPLE_ROAMING
		if (strlen((char *)wpa_s->conf->candi_bssid) > 0) {
			PRINTF(RED_COLOR ">>> [Roaming] Skip - "
				"BSSID["MACSTR"], Level[%d]\n" CLEAR_COLOR,
				MAC2STR(selected->bssid), selected->level);
		}
		os_memset(wpa_s->conf->candi_bssid, 0, ETH_ALEN);
		wpa_s->conf->roam_cnt = ROAM_SUSPEND_CNT;
#endif	/* CONFIG_SIMPLE_ROAMING */
		return 0;
	}

#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpa_s->conf->roam_state == ROAM_DISABLE)
#endif /* CONFIG_SIMPLE_ROAMING */
	if (selected->last_update_idx > current_bss->last_update_idx)
		return 1; /* current BSS not seen in the last scan */

#ifndef CONFIG_NO_ROAMING
	wpa_dbg(wpa_s, MSG_DEBUG, "Considering within-ESS reassociation");
	wpa_dbg(wpa_s, MSG_DEBUG, "Current BSS: " MACSTR
		" freq=%d level=%d snr=%d est_throughput=%u",
		MAC2STR(current_bss->bssid),
		current_bss->freq, current_bss->level,
		current_bss->snr, current_bss->est_throughput);
	wpa_dbg(wpa_s, MSG_DEBUG, "Selected BSS: " MACSTR
		" freq=%d level=%d snr=%d est_throughput=%u",
		MAC2STR(selected->bssid), selected->freq, selected->level,
		selected->snr, selected->est_throughput);

	if (wpa_s->current_ssid->bssid_set &&
	    os_memcmp(selected->bssid, wpa_s->current_ssid->bssid, ETH_ALEN)
	    == 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Allow reassociation - selected BSS "
			"has preferred BSSID");
		return 1;
	}

	if (selected->est_throughput > current_bss->est_throughput + 5000) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Allow reassociation - selected BSS has better estimated throughput");
		return 1;
	}

	to_5ghz = selected->freq > 4000 && current_bss->freq < 4000;

	if (current_bss->level < 0 &&
	    current_bss->level > selected->level + to_5ghz * 2) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Skip roam - Current BSS has better "
			"signal level");

#ifdef	CONFIG_SIMPLE_ROAMING
		if (strlen((char *)wpa_s->conf->candi_bssid) > 0) {
			PRINTF(RED_COLOR ">>> [Roaming] Skip - BSSID["MACSTR"] Level[%d]"
				" > New BSSID["MACSTR"], Level[%d]\n" CLEAR_COLOR,
				MAC2STR(current_bss->bssid),
				current_bss->level,
				MAC2STR(selected->bssid),
				selected->level);
		}
		os_memset(wpa_s->conf->candi_bssid, 0, ETH_ALEN);
		wpa_s->conf->roam_cnt = ROAM_SUSPEND_CNT;
#endif	/* CONFIG_SIMPLE_ROAMING */
		return 0;
	}

	if (current_bss->est_throughput > selected->est_throughput + 5000) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Skip roam - Current BSS has better estimated throughput");
		return 0;
	}

	cur_est = current_bss->est_throughput;
	sel_est = selected->est_throughput;
	min_diff = 2;
	if (current_bss->level < 0) {
		if (current_bss->level < -85)
			min_diff = 1;
		else if (current_bss->level < -80)
			min_diff = 2;
		else if (current_bss->level < -75)
			min_diff = 3;
		else if (current_bss->level < -70)
			min_diff = 4;
		else
			min_diff = 5;
		if (cur_est > sel_est * 1.5)
			min_diff += 10;
		else if (cur_est > sel_est * 1.2)
			min_diff += 5;
		else if (cur_est > sel_est * 1.1)
			min_diff += 2;
		else if (cur_est > sel_est)
			min_diff++;
	}
	if (to_5ghz) {
		int reduce = 2;

		/* Make it easier to move to 5 GHz band */
		if (sel_est > cur_est * 1.5)
			reduce = 5;
		else if (sel_est > cur_est * 1.2)
			reduce = 4;
		else if (sel_est > cur_est * 1.1)
			reduce = 3;

		if (min_diff > reduce)
			min_diff -= reduce;
		else
			min_diff = 0;
	}

	diff = abs(current_bss->level - selected->level);
	if (diff < min_diff) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Skip roam - too small difference in signal level (%d < %d)",
			diff, min_diff);

#ifdef	CONFIG_SIMPLE_ROAMING
		if (strlen((char *)wpa_s->conf->candi_bssid) > 0) {
			PRINTF(RED_COLOR ">>> [Roaming] Skip - Too small difference "
				"in signal level (diff=%d < min_diff=%d)\n" CLEAR_COLOR,
				abs(current_bss->level - selected->level), min_diff);
		}

		os_memset(wpa_s->conf->candi_bssid, 0, ETH_ALEN);
		wpa_s->conf->roam_cnt = ROAM_SUSPEND_CNT;
#endif	/* CONFIG_SIMPLE_ROAMING */
		return 0;
	}

	wpa_dbg(wpa_s, MSG_DEBUG,
		"Allow reassociation due to difference in signal level (%d >= %d)",
		diff, min_diff);

#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpas_simple_roaming_proc(wpa_s, selected))
			return 0;
#endif	/* CONFIG_SIMPLE_ROAMING */

	return 1;
#else /* CONFIG_NO_ROAMING */
	return 0;
#endif /* CONFIG_NO_ROAMING */
}


#ifdef __SUPP_27_SUPPORT__
/*
 * Return a negative value if no scan results could be fetched or if scan
 * results should not be shared with other virtual interfaces.
 * Return 0 if scan results were fetched and may be shared with other
 * interfaces.
 * Return 1 if scan results may be shared with other virtual interfaces but may
 * not trigger any operations.
 * Return 2 if the interface was removed and cannot be used.
 */
static int _wpa_supp_ev_scan_results(struct wpa_supplicant *wpa_s,
					      union wpa_event_data *data,
					      int own_request, int update_only)
{
	struct wpa_scan_results *scan_res = NULL;
	int ret = 0;
	int ap = 0;
#ifndef CONFIG_NO_RANDOM_POOL
	size_t i, num;
#endif /* CONFIG_NO_RANDOM_POOL */

#ifdef CONFIG_AP
	if (wpa_s->ap_iface)
		ap = 1;
#endif /* CONFIG_AP */

	wpa_supplicant_notify_scanning(wpa_s, 0);

	scan_res = wpa_supplicant_get_scan_results(wpa_s,
						   data ? &data->scan_info :
						   NULL, 1);
	if (scan_res == NULL) {
#ifdef CONFIG_AP
		if (ap ||
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		    wpa_s->conf->ap_scan == 2 ||
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
		    wpa_s->scan_res_handler == scan_only_handler) {
			return -1;
		}
#endif /* CONFIG_AP */

		if (!own_request)
			return -1;
		if (data && data->scan_info.external_scan)
			return -1;
		wpa_dbg(wpa_s, MSG_DEBUG, "Failed to get scan results - try "
			"scanning again");
		wpa_supplicant_req_new_scan(wpa_s, 1, 0);
		ret = -1;
		goto scan_work_done;
	}

#ifndef CONFIG_NO_RANDOM_POOL
	num = scan_res->num;
	if (num > 10)
		num = 10;
	for (i = 0; i < num; i++) {
		u8 buf[5];
		struct wpa_scan_res *res = scan_res->res[i];
		buf[0] = res->bssid[5];
		buf[1] = res->qual & 0xff;
		buf[2] = res->noise & 0xff;
		buf[3] = res->level & 0xff;
		buf[4] = res->tsf & 0xff;
		random_add_randomness(buf, sizeof(buf));
	}
#endif /* CONFIG_NO_RANDOM_POOL */

	if (update_only) {
		ret = 1;
		goto scan_work_done;
	}

	if (own_request && wpa_s->scan_res_handler &&
	    !(data && data->scan_info.external_scan)) {
		void (*scan_res_handler)(struct wpa_supplicant *wpa_s,
					 struct wpa_scan_results *scan_res);

		scan_res_handler = wpa_s->scan_res_handler;
		wpa_s->scan_res_handler = NULL;
		scan_res_handler(wpa_s, scan_res);
		ret = 1;
		goto scan_work_done;
	}

	if (ap) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignore scan results in AP mode");
#ifdef CONFIG_AP
		if (wpa_s->ap_iface->scan_cb)
			wpa_s->ap_iface->scan_cb(wpa_s->ap_iface);
#endif /* CONFIG_AP */
		goto scan_work_done;
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "New scan results available (own=%u ext=%u)",
		wpa_s->own_scan_running,
		data ? data->scan_info.external_scan : 0);
	if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
	    wpa_s->manual_scan_use_id && wpa_s->own_scan_running &&
	    own_request && !(data && data->scan_info.external_scan)) {
		wpa_msg_ctrl(wpa_s, MSG_INFO, WPA_EVENT_SCAN_RESULTS "id=%u",
			     wpa_s->manual_scan_id);
		wpa_s->manual_scan_use_id = 0;
	} else {
		wpa_msg_ctrl(wpa_s, MSG_INFO, WPA_EVENT_SCAN_RESULTS);
	}

#if 0	/* by Bright : Merge 2.7 */
	wpas_notify_scan_results(wpa_s);

	wpas_notify_scan_done(wpa_s, 1);
#endif	/* 0 */

	if (data && data->scan_info.external_scan) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Do not use results from externally requested scan operation for network selection");
		wpa_scan_results_free(scan_res);
		return 0;
	}

	if (wnm_scan_process(wpa_s, 1) > 0)
		goto scan_work_done;

	if (sme_proc_obss_scan(wpa_s) > 0)
		goto scan_work_done;

	if (own_request &&
	    wpas_beacon_rep_scan_process(wpa_s, scan_res, &data->scan_info) > 0)
		goto scan_work_done;

	if (
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		wpa_s->conf->ap_scan == 2 &&
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
		!wpas_wps_searching(wpa_s))
		goto scan_work_done;

	if (autoscan_notify_scan(wpa_s, scan_res))
		goto scan_work_done;

	if (wpa_s->disconnected) {
		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
		goto scan_work_done;
	}

#ifdef	CONFIG_BGSCAN
	if (!wpas_driver_bss_selection(wpa_s) &&
	    bgscan_notify_scan(wpa_s, scan_res) == 1)
		goto scan_work_done;
#endif	/* CONFIG_BGSCAN */

	wpas_wps_update_ap_info(wpa_s, scan_res);

	if (wpa_s->wpa_state >= WPA_AUTHENTICATING &&
	    wpa_s->wpa_state < WPA_COMPLETED)
		goto scan_work_done;

	wpa_scan_results_free(scan_res);

#ifdef	CONFIG_SCAN_WORK
	if (own_request && wpa_s->scan_work) {
		struct wpa_radio_work *work = wpa_s->scan_work;
		wpa_s->scan_work = NULL;
		radio_work_done(work);
	}
#endif	/* CONFIG_SCAN_WORK */

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	return wpas_select_network_from_last_scan(wpa_s, 1, 1, NULL, NULL);
#else
	return wpas_select_network_from_last_scan(wpa_s, 1, own_request);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

scan_work_done:
	wpa_scan_results_free(scan_res);
#ifdef	CONFIG_SCAN_WORK
	if (own_request && wpa_s->scan_work) {
		struct wpa_radio_work *work = wpa_s->scan_work;
		wpa_s->scan_work = NULL;
		radio_work_done(work);
	}
#endif	/* CONFIG_SCAN_WORK */
	return ret;
}
#endif /* __SUPP_27_SUPPORT__ */


#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
static int wpas_select_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      int new_scan, int own_request, struct wpa_bss **selected_p, struct wpa_ssid **ssid_p)
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
static int wpas_select_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      int new_scan, int own_request)
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
{
	struct wpa_bss *selected=NULL;
	struct wpa_ssid *ssid = NULL;
	int time_to_reenable = wpas_reenabled_network_time(wpa_s);

	RX_FUNC_START("                ");

	if (time_to_reenable > 0) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Postpone network selection by %d seconds since all networks are disabled",
			time_to_reenable);
		da16x_eloop_cancel_timeout(wpas_network_reenabled, wpa_s, NULL);
		da16x_eloop_register_timeout(time_to_reenable, 0,
				       wpas_network_reenabled, wpa_s, NULL);
		return 0;
	}

#ifdef CONFIG_P2P
	if (wpa_s->p2p_mgmt)
		return 0; /* no normal connection on p2p_mgmt interface */
#endif /* CONFIG_P2P */

#ifdef CONFIG_OWE
	wpa_s->owe_transition_search = 0;
#endif
	selected = wpa_supplicant_pick_network(wpa_s, &ssid);

#ifdef CONFIG_MESH
	if (wpa_s->ifmsh) {
		wpa_msg(wpa_s, MSG_DEBUG,
			"Avoiding join because we already joined a mesh group");
		return 0;
	}
#endif /* CONFIG_MESH */

	if (selected) {
		int skip;
		
		skip = !wpa_supplicant_need_to_roam(wpa_s, selected, ssid);

		if (skip) {
#ifdef	CONFIG_SIMPLE_ROAMING
			if (wpa_s->conf->roam_state == ROAM_SCANNING) {
				wpa_supplicant_cancel_scan(wpa_s);
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
				wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */

				if (!wpa_s->scanning &&
				    ((wpa_s->wpa_state <= WPA_SCANNING) ||
				     (wpa_s->wpa_state == WPA_COMPLETED)))
				{
					wpa_s->normal_scans = 0;
					wpa_s->scan_req = MANUAL_SCAN_REQ;
#ifdef	CONFIG_WPS
					wpa_s->after_wps = 0;
					wpa_s->known_wps_freq = 0;
#endif	/* CONFIG_WPS */
					wpa_supplicant_req_scan(wpa_s, ROAM_SCAN_INT, 0);

					if (wpa_s->manual_scan_use_id) {
						wpa_s->manual_scan_id++;
						da16x_scan_prt("[%s] Assigned scan id %u\n",
							__func__, wpa_s->manual_scan_id);
					}
				}
			}
#endif	/* CONFIG_SIMPLE_ROAMING */
#ifdef	CONFIG_PRE_AUTH
			if (new_scan)
				wpa_supplicant_rsn_preauth_scan_results(wpa_s);
#endif	/* CONFIG_PRE_AUTH */
			return 0;
		}

		if (ssid != wpa_s->current_ssid &&
		    wpa_s->wpa_state >= WPA_AUTHENTICATING) {
			wpa_s->own_disconnect_req = 1;
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		}

#ifdef CONFIG_FAST_CONNECTION
		wpa_s->num_retries = 0;
		da16x_scan_prt("[%s] wpa_s->num_retries = %d\n", __func__, wpa_s->num_retries);
#endif /* CONFIG_FAST_CONNECTION */

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
		supp_rehold_bss(selected->internal_bss);				
		wpa_s->current_bss = selected;
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		if (wpa_supplicant_connect(wpa_s, selected, ssid) < 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "Connect failed");
			return -1;
		}
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
#ifdef	CONFIG_WPS
		if (wpas_wps_scan_pbc_overlap(wpa_s, selected, ssid)) {
			da16x_notice_prt(WPS_EVENT_OVERLAP "PBC session overlap\n");
#ifdef CONFIG_P2P
			if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT ||
		    	wpa_s->p2p_in_provisioning) {
				return -1;
			}
#endif /* CONFIG_P2P */

			wpas_wps_cancel(wpa_s, NULL);
			return -1;
		}
#endif	/* CONFIG_WPS */

		if (   wpa_s->reassociate
			|| (   os_memcmp((selected)->bssid, wpa_s->bssid, ETH_ALEN) != 0
				&& (   (wpa_s->wpa_state != WPA_ASSOCIATING && wpa_s->wpa_state != WPA_AUTHENTICATING)
					|| (!is_zero_ether_addr(wpa_s->pending_bssid) && os_memcmp((selected)->bssid, wpa_s->pending_bssid, ETH_ALEN) != 0)
						|| (is_zero_ether_addr(wpa_s->pending_bssid) && ssid != wpa_s->current_ssid)
				   )
			   )
		   ) {
			da16x_assoc_prt("[%s] Request association with " MACSTR "\n",
					__func__, MAC2STR(selected->bssid));

			if (wpa_s->current_bss) {
				wpa_supplicant_clear_current_bss(wpa_s);
				wpa_s->current_bss = NULL;
			}
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
			supp_rehold_bss(selected->internal_bss);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
			wpa_s->current_bss = selected;
			wpa_supplicant_copy_ie_for_current_bss(wpa_s, selected);
		}
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

#ifdef	CONFIG_PRE_AUTH
		if (new_scan)
			wpa_supplicant_rsn_preauth_scan_results(wpa_s);
#endif	/* CONFIG_PRE_AUTH */

		/*
		 * Do not allow other virtual radios to trigger operations based
		 * on these scan results since we do not want them to start
		 * other associations at the same time.
		 */
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
		*selected_p = selected;
		*ssid_p = ssid;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
		return 1;
	} else {
		if (wpa_s->scan_for_connection || wpa_s->last_scan_req != MANUAL_SCAN_REQ) {
			//wpa_dbg(wpa_s, MSG_INFO, "No suitable network found");
			da16x_notice_prt("!!! No selected network !!!\n");
			wpa_s->disconnect_reason = 0;
			wifi_conn_fail_reason_code = wpa_s->disconnect_reason;

			if (dpm_get_wifi_conn_retry_cnt() > 0) {
				if (dpm_decr_wifi_conn_retry_cnt() == 0) {
					force_dpm_abnormal_sleep_by_wifi_conn_fail();
				}
			}
		} else {
			da16x_scan_prt("[%s] wpa_supplicant_cancel_scan()\n", __func__);

			wpa_supplicant_cancel_scan(wpa_s);
	
			if (wpa_s->wpa_state == WPA_COMPLETED) {
				return 0;
			}
		}
#ifdef __SUPPORT_CON_EASY__
		/* set the Connection try status as "AP not found(2)" */
		ce_set_conn_try_result_status(2);
#endif	/* __SUPPORT_CON_EASY__ */

#ifdef CONFIG_FAST_CONNECTION
		if (wpa_s->num_retries > 254)
			wpa_s->num_retries = 0;
		else
			wpa_s->num_retries++;
		
		da16x_scan_prt("[%s] wpa_s->num_retries = %d\n", __func__, wpa_s->num_retries);

#endif /* CONFIG_FAST_CONNECTION */
#ifdef CONFIG_AP
		if (wpa_s->ap_iface)	//20150430_Trinity.Song
			ssid = wpa_supplicant_pick_new_network(wpa_s);
#endif /* CONFIG_AP */

		if (ssid) {
			wpa_dbg(wpa_s, MSG_DEBUG, "Setup a new network");
			wpa_supplicant_associate(wpa_s, NULL, ssid);
#ifdef	CONFIG_PRE_AUTH
			if (new_scan)
				wpa_supplicant_rsn_preauth_scan_results(wpa_s);
#endif	/* CONFIG_PRE_AUTH */
		} else if (own_request) {
			/*
			 * No SSID found. If SCAN results are as a result of
			 * own scan request and not due to a scan request on
			 * another shared interface, try another scan.
			 */
			int timeout_sec = wpa_s->scan_interval;
			int timeout_usec = 0;

#ifdef CONFIG_FAST_CONNECTION
#ifdef FAST_CONN_ASSOC_CH
#if 1
			/* Scan for Fast Connection if having Assoc channel */
			if (wpa_s->num_retries < 4 && (wpa_s->conf->scan_cur_freq || wpa_s->reassoc_freq != 0))
				timeout_sec = 1;
#else
			/* Scan for Fast Connection if having Assoc channel */
			if (wpa_s->num_retries < 4 && wpa_s->conf->scan_cur_freq)
				timeout_sec = 1;
#endif /* 1 */
#else
			if (wpa_s->num_retries < 4 && wpa_s->reassoc_freq != 0)
				timeout_sec = 1;
#endif  // FAST_CONN_ASSOC_CH
#endif /* CONFIG_FAST_CONNECTION */

#ifdef CONFIG_P2P
#ifdef	CONFIG_P2P_UNUSED_CMD
			int res;

			res = wpas_p2p_scan_no_go_seen(wpa_s);
			if (res == 2)
				return 2;
			if (res == 1)
				return 0;
#endif 	/* CONFIG_P2P_UNUSED_CMD */

			if (wpa_s->p2p_in_provisioning ||
			    wpa_s->show_group_started
#ifdef	CONFIG_P2P_OPTION
			    || wpa_s->p2p_in_invitation
#endif	/* CONFIG_P2P_OPTION */
			    ) {
				/*
				 * Use shorter wait during P2P Provisioning
				 * state and during P2P join-a-group operation
				 * to speed up group formation.
				 */
				timeout_sec = 0;
				timeout_usec = 250000;
				wpa_supplicant_req_new_scan(wpa_s, timeout_sec,
							    timeout_usec);
				return 0;
			}
#endif /* CONFIG_P2P */
#ifdef CONFIG_INTERWORKING
			if (wpa_s->conf->auto_interworking &&
			    wpa_s->conf->interworking &&
			    wpa_s->conf->cred) {
				wpa_dbg(wpa_s, MSG_DEBUG, "Interworking: "
					"start ANQP fetch since no matching "
					"networks found");
				wpa_s->network_select = 1;
				wpa_s->auto_network_select = 1;
				interworking_start_fetch_anqp(wpa_s);
				return 1;
			}
#endif /* CONFIG_INTERWORKING */
#ifdef CONFIG_WPS
			if (wpa_s->after_wps > 0 || wpas_wps_searching(wpa_s)) {
				wpa_dbg(wpa_s, MSG_DEBUG, "Use shorter wait during WPS processing");
				timeout_sec = 0;
				timeout_usec = 500000;
				wpa_supplicant_req_new_scan(wpa_s, timeout_sec,
							    timeout_usec);
				return 0;
			}
#endif /* CONFIG_WPS */
#ifdef CONFIG_OWE
			if (wpa_s->owe_transition_search) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"OWE: Use shorter wait during transition mode search");
				timeout_sec = 0;
				timeout_usec = 500000;
				wpa_supplicant_req_new_scan(wpa_s, timeout_sec,
							    timeout_usec);
				return 0;
			}
#endif /* CONFIG_OWE */

#if 0 /* Supp 2.7 DA16xxx */
			if (wpa_supplicant_req_sched_scan(wpa_s))
#endif /* Supp 2.7 DA16xxx */
			{
				wpa_supplicant_req_new_scan(wpa_s, timeout_sec,
							    timeout_usec);
			}

			wpa_msg_ctrl(wpa_s, MSG_INFO,
				     WPA_EVENT_NETWORK_NOT_FOUND);
		}
	}

	RX_FUNC_END("                ");

	return 0;
}


#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
static int wpas_connect_network_from_last_scan(struct wpa_supplicant *wpa_s,
					      struct wpa_bss *selected, struct wpa_ssid *ssid)
{
	da16x_scan_prt("[%s] Start connecting to the selected network. \n", __func__);
#ifdef CONFIG_FAST_RECONNECT_V2
	da16x_is_fast_reconnect = 0;
#endif
	if (wpa_supplicant_connect(wpa_s, selected, ssid) < 0) {
		da16x_scan_prt("[%s] Connect failed\n", __func__);
		return -1;
	}

	return 1;
}
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */


#if 1 /* FC9000 Only */
/* Supplicant 2.7 Code */
//static void wpa_supp_ev_scan_results(struct wpa_supplicant *wpa_s,
//					      union wpa_event_data *data)
#ifdef SUPPORT_SELECT_NETWORK_FILTER
extern int wpa_driver_scan_select_ssid(struct wpa_supplicant *wpa_s, int set);
#endif /* SUPPORT_SELECT_NETWORK_FILTER */


static int wpa_supp_ev_scan_results(struct wpa_supplicant *wpa_s,
					      union wpa_event_data *data)

{
	struct wpa_scan_results *scan_res = NULL;
#ifdef	CONFIG_AP
	int ap = 0;
#endif	/* CONFIG_AP */
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	struct wpa_bss *selected = NULL;
	struct wpa_ssid *ssid = NULL;
	int do_connect = 0;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
	RX_FUNC_START("              ");

#ifdef CONFIG_AP
	if (wpa_s->ap_iface)
		ap = 1;
#endif /* CONFIG_AP */


	wpa_s->scanning = 0;
	scan_res = wpa_supplicant_get_scan_results(wpa_s,
						   data ? &data->scan_info :
						   NULL, 1);

	if (scan_res == NULL) {

#ifdef	CONFIG_AP
		if (
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
			wpa_s->conf->ap_scan == 2
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
			ap || wpa_s->scan_res_handler == scan_only_handler) {
			return 0;
		}
#endif /* CONFIG_AP */

		da16x_scan_prt("              <%s> Failed to get scan results "
				"- try scanning again\n", __func__);

		wpa_supplicant_req_new_scan(wpa_s, 1, 0);
		goto scan_work_done;
	}

	if (wpa_s->scan_res_handler &&
	    (wpa_s->own_scan_running
#if defined ( __SUPP_27_SUPPORT__ )
			|| !wpa_s->radio->external_scan_running
#endif // __SUPP_27_SUPPORT__
		)
	   ) {
		void (*scan_res_handler)(struct wpa_supplicant *wpa_s,
					 struct wpa_scan_results *scan_res);

		scan_res_handler = wpa_s->scan_res_handler;
		wpa_s->scan_res_handler = NULL;
		scan_res_handler(wpa_s, scan_res);
		goto scan_work_done;
	}

#ifdef CONFIG_AP
	if (ap) {
		da16x_scan_prt("<%s> Ignore scan results in AP mode\n", __func__);
		if (wpa_s->ap_iface->scan_cb)
			wpa_s->ap_iface->scan_cb(wpa_s->ap_iface);
		wpa_bss_flush(wpa_s);
		goto scan_work_done;
	}
#endif /* CONFIG_AP */

#if 0
	da16x_scan_prt("<%s> New scan results available (own=%u ext=%u)\n",
			__func__,
			wpa_s->own_scan_running, wpa_s->radio->external_scan_running);
#endif /* 0 */

	if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
	    wpa_s->manual_scan_use_id && wpa_s->own_scan_running) {
		da16x_scan_prt("[%s] " WPA_EVENT_SCAN_RESULTS "id=%u\n",
				__func__, wpa_s->manual_scan_id);
		wpa_s->manual_scan_use_id = 0;
	}

#ifdef	CONFIG_WPS
	wpas_wps_notify_scan_results(wpa_s);
#endif	/* CONFIG_WPS */

	if (!wpa_s->own_scan_running
#if defined ( __SUPP_27_SUPPORT__ )
		&& wpa_s->radio->external_scan_running
#endif // __SUPP_27_SUPPORT__
	   ) {
		da16x_scan_prt("              <%s> Do not use results from "
			"externally requested scan operation for network "
			"selection\n", __func__);

		wpa_scan_results_free(scan_res);
#if 1 /* FC9000 */
		return 0;
#else
		return;
#endif /* FC9000 */
	}

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if ((wpa_s->conf->ap_scan == 2 && !wpas_wps_searching(wpa_s)))
		goto scan_work_done;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	if (autoscan_notify_scan(wpa_s, scan_res))
		goto scan_work_done;

	if (wpa_s->disconnected) {
		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
		goto scan_work_done;
	}
#if 0 /* fixed - for simple roaming /20180524 / Jin.lee */
	else if (wpa_s->wpa_state == WPA_COMPLETED) {
		goto scan_work_done;
	}
#endif
#ifdef	CONFIG_BGSCAN
	if (!wpas_driver_bss_selection(wpa_s) &&
	    bgscan_notify_scan(wpa_s, scan_res) == 1)
		goto scan_work_done;
#endif	/* CONFIG_BGSCAN */

#ifdef	CONFIG_WPS
	wpas_wps_update_ap_info(wpa_s, scan_res);
#endif	/* CONFIG_WPS */

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	do_connect = wpas_select_network_from_last_scan(wpa_s, 1, 1, &selected, &ssid);
	wpa_scan_results_free(scan_res);
#ifndef CONFIG_REUSED_UMAC_BSS_LIST	
	wpa_bss_flush(wpa_s);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

	if (do_connect) {
		wpas_connect_network_from_last_scan(wpa_s, selected, ssid);
	} else {	// for Issue #454
		if (wpa_s->wpa_state < WPA_AUTHENTICATING) /* Do not clear during roamnig */
			wpa_s->current_ssid = NULL;
	}

#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
	wpa_scan_results_free(scan_res);
	wpas_select_network_from_last_scan(wpa_s, 1, 1);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

#if	defined(CONFIG_SCAN_WITH_BSSID) || defined(CONFIG_SCAN_WITH_DIR_SSID)
	/* Scan with BSSID */
	if (wpa_s->manual_scan_promisc)
	{
		/* Clearing the BSSID scan flag */
		wpa_s->manual_scan_promisc = 0; 
	}	
#endif /* CONFIG_SCAN_WITH_BSSID || CONFIG_SCAN_WITH_DIR_SSID */

	RX_FUNC_END("");

#if 1 /* FC9000 */
	return 1;
#else
	return;
#endif /* FC9000 */

#if	defined(CONFIG_SCAN_WITH_BSSID) || defined(CONFIG_SCAN_WITH_DIR_SSID)
	/* Scan with BSSID */
	if (wpa_s->manual_scan_promisc)
	{
		/* Clearing the BSSID scan flag */
		wpa_s->manual_scan_promisc = 0;
	}
#endif /* CONFIG_SCAN_WITH_BSSID || CONFIG_SCAN_WITH_DIR_SSID */

scan_work_done:
	wpa_scan_results_free(scan_res);

#if 1 /* FC9000 */
	return 0;
#else
	return;
#endif /* FC9000 */
}

#else /* FC9000 Only */

static int wpa_supp_ev_scan_results(struct wpa_supplicant *wpa_s,
					     union wpa_event_data *data)
{
	struct wpa_supplicant *ifs;
	int res;

	res = _wpa_supp_ev_scan_results(wpa_s, data, 1, 0);
	if (res == 2) {
		/*
		 * Interface may have been removed, so must not dereference
		 * wpa_s after this.
		 */
		return 1;
	}

	if (res < 0) {
		/*
		 * If no scan results could be fetched, then no need to
		 * notify those interfaces that did not actually request
		 * this scan. Similarly, if scan results started a new operation on this
		 * interface, do not notify other interfaces to avoid concurrent
		 * operations during a connection attempt.
		 */
		return 0;
	}

	/*
	 * Check other interfaces to see if they share the same radio. If
	 * so, they get updated with this same scan info.
	 */
	dl_list_for_each(ifs, &wpa_s->radio->ifaces, struct wpa_supplicant,
			 radio_list) {
		if (ifs != wpa_s) {
			wpa_printf_dbg(MSG_DEBUG, "%s: Updating scan results from "
				   "sibling", ifs->ifname);
			res = _wpa_supp_ev_scan_results(ifs, data, 0,
								 res > 0);
			if (res < 0)
				return 0;
		}
	}

	return 0;
}
#endif /* FC9000 Only */
#endif /* CONFIG_NO_SCAN_PROCESSING */


#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
int wpa_supplicant_fast_associate(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_NO_SCAN_PROCESSING
	return -1;
#else /* CONFIG_NO_SCAN_PROCESSING */
	struct os_reltime now;
	wpa_s->ignore_post_flush_scan_res = 0;
	if (wpa_s->last_scan_res_used == 0)
		return -1;

	os_get_reltime(&now);
	if (os_reltime_expired(&now, &wpa_s->last_scan, 5)) {
		wpa_printf_dbg(MSG_DEBUG, "Fast associate: Old scan results");
		return -1;
	}

	return wpas_select_network_from_last_scan(wpa_s, 0, 1);
#endif /* CONFIG_NO_SCAN_PROCESSING */
}
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */


#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_BSS_MAX_IDLE_PERIOD
static void wnm_bss_keep_alive(void *eloop_ctx, void *sock_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (wpa_s->wpa_state < WPA_ASSOCIATED)
		return;

	if (!wpa_s->no_keep_alive) {
		wpa_printf_dbg(MSG_DEBUG, "WNM: Send keep-alive to AP " MACSTR,
			   MAC2STR(wpa_s->bssid));
		/* TODO: could skip this if normal data traffic has been sent */
		/* TODO: Consider using some more appropriate data frame for
		 * this */
		if (wpa_s->l2)
			l2_packet_send(wpa_s->l2, wpa_s->bssid, 0x0800,
				       (u8 *) "", 0, 0);
	}

#ifdef CONFIG_SME
	if (wpa_s->sme.bss_max_idle_period) {
		unsigned int msec;
		msec = wpa_s->sme.bss_max_idle_period * 1024; /* times 1000 */
		if (msec > 100)
			msec -= 100;

		da16x_eloop_register_timeout(msec / 1000, msec % 1000 * 1000,
				       wnm_bss_keep_alive, wpa_s, NULL);
	}
#endif /* CONFIG_SME */
}


static void wnm_process_assoc_resp(struct wpa_supplicant *wpa_s,
				   const u8 *ies, size_t ies_len)
{
	struct ieee802_11_elems elems;

	if (ies == NULL)
		return;

	if (ieee802_11_parse_elements(ies, ies_len, &elems, 1) == ParseFailed)
		return;

#ifdef CONFIG_SME
	if (elems.bss_max_idle_period) {
		unsigned int msec;
		wpa_s->sme.bss_max_idle_period =
			WPA_GET_LE16(elems.bss_max_idle_period);
		wpa_printf_dbg(MSG_DEBUG, "WNM: BSS Max Idle Period: %u (* 1000 "
			   "TU)%s", wpa_s->sme.bss_max_idle_period,
			   (elems.bss_max_idle_period[2] & 0x01) ?
			   " (protected keep-live required)" : "");
		if (wpa_s->sme.bss_max_idle_period == 0)
			wpa_s->sme.bss_max_idle_period = 1;
		if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) {
			da16x_eloop_cancel_timeout(wnm_bss_keep_alive, wpa_s, NULL);
			 /* msec times 1000 */
			msec = wpa_s->sme.bss_max_idle_period * 1024;
			if (msec > 100)
				msec -= 100;
			da16x_eloop_register_timeout(msec / 1000, msec % 1000 * 1000,
					       wnm_bss_keep_alive, wpa_s,
					       NULL);
		}
	}
#endif /* CONFIG_SME */
}

#endif /* CONFIG_WNM */


void wnm_bss_keep_alive_deinit(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_WNM
	da16x_eloop_cancel_timeout(wnm_bss_keep_alive, wpa_s, NULL);
#endif /* CONFIG_WNM */
}
#endif /* CONFIG_WNM_BSS_MAX_IDLE_PERIOD */

#ifdef CONFIG_INTERWORKING

static int wpas_qos_map_set(struct wpa_supplicant *wpa_s, const u8 *qos_map,
			    size_t len)
{
	int res;

	wpa_hexdump(MSG_DEBUG, "Interworking: QoS Map Set", qos_map, len);
	res = wpa_drv_set_qos_map(wpa_s, qos_map, len);
	if (res) {
		wpa_printf_dbg(MSG_DEBUG, "Interworking: Failed to configure QoS Map Set to the driver");
	}

	return res;
}


static void interworking_process_assoc_resp(struct wpa_supplicant *wpa_s,
					    const u8 *ies, size_t ies_len)
{
	struct ieee802_11_elems elems;

	if (ies == NULL)
		return;

	if (ieee802_11_parse_elements(ies, ies_len, &elems, 1) == ParseFailed)
		return;

	if (elems.qos_map_set) {
		wpas_qos_map_set(wpa_s, elems.qos_map_set,
				 elems.qos_map_set_len);
	}
}

#endif /* CONFIG_INTERWORKING */


#ifdef CONFIG_FST
static int wpas_fst_update_mbie(struct wpa_supplicant *wpa_s,
				const u8 *ie, size_t ie_len)
{
	struct mb_ies_info mb_ies;

	if (!ie || !ie_len || !wpa_s->fst)
	    return -ENOENT;

	os_memset(&mb_ies, 0, sizeof(mb_ies));

	while (ie_len >= 2 && mb_ies.nof_ies < MAX_NOF_MB_IES_SUPPORTED) {
		size_t len;

		len = 2 + ie[1];
		if (len > ie_len) {
			wpa_hexdump(MSG_DEBUG, "FST: Truncated IE found",
				    ie, ie_len);
			break;
		}

		if (ie[0] == WLAN_EID_MULTI_BAND) {
			wpa_printf_dbg(MSG_DEBUG, "MB IE of %u bytes found",
				   (unsigned int) len);
			mb_ies.ies[mb_ies.nof_ies].ie = ie + 2;
			mb_ies.ies[mb_ies.nof_ies].ie_len = len - 2;
			mb_ies.nof_ies++;
		}

		ie_len -= len;
		ie += len;
	}

	if (mb_ies.nof_ies > 0) {
		wpabuf_free(wpa_s->received_mb_ies);
		wpa_s->received_mb_ies = mb_ies_by_info(&mb_ies);
		return 0;
	}

	return -ENOENT;
}
#endif /* CONFIG_FST */


static int wpa_supplicant_event_associnfo(struct wpa_supplicant *wpa_s,
					  union wpa_event_data *data)
{
	int l, len, found = 0, wpa_found, rsn_found;
	const u8 *p;
#if defined(CONFIG_IEEE80211R) || defined(CONFIG_OWE)
	u8 bssid[ETH_ALEN];
#endif /* CONFIG_IEEE80211R || CONFIG_OWE */

	wpa_dbg(wpa_s, MSG_DEBUG, "Association info event");

	if (data->assoc_info.req_ies)
		wpa_hexdump(MSG_DEBUG, "req_ies", data->assoc_info.req_ies,
			    data->assoc_info.req_ies_len);

	if (data->assoc_info.resp_ies) {
		wpa_hexdump(MSG_DEBUG, "resp_ies", data->assoc_info.resp_ies,
			    data->assoc_info.resp_ies_len);
#ifdef CONFIG_TDLS
		wpa_tdls_assoc_resp_ies(wpa_s->wpa, data->assoc_info.resp_ies,
					data->assoc_info.resp_ies_len);
#endif /* CONFIG_TDLS */
#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_BSS_MAX_IDLE_PERIOD
		wnm_process_assoc_resp(wpa_s, data->assoc_info.resp_ies,
				       data->assoc_info.resp_ies_len);
#endif /* CONFIG_WNM_BSS_MAX_IDLE_PERIOD */
#endif /* CONFIG_WNM */
#ifdef CONFIG_INTERWORKING
		interworking_process_assoc_resp(wpa_s, data->assoc_info.resp_ies,
						data->assoc_info.resp_ies_len);
#endif /* CONFIG_INTERWORKING */
#ifdef CONFIG_IEEE80211AC
		if (wpa_s->hw_capab == CAPAB_VHT &&
		    get_ie(data->assoc_info.resp_ies,
			   data->assoc_info.resp_ies_len, WLAN_EID_VHT_CAP))
			wpa_s->ieee80211ac = 1;
#endif /* CONFIG_IEEE80211AC */
	}

	if (data->assoc_info.beacon_ies)
		wpa_hexdump(MSG_DEBUG, "beacon_ies",
			    data->assoc_info.beacon_ies,
			    data->assoc_info.beacon_ies_len);

	if (data->assoc_info.freq)
		wpa_dbg(wpa_s, MSG_DEBUG, "freq=%u MHz",
			data->assoc_info.freq);

	p = data->assoc_info.req_ies;
	l = data->assoc_info.req_ies_len;

	/* Go through the IEs and make a copy of the WPA/RSN IE, if present. */
	while (p && l >= 2) {
		len = p[1] + 2;
		if (len > l) {
			wpa_hexdump(MSG_DEBUG, "Truncated IE in assoc_info",
				    p, l);
			break;
		}
		if ((p[0] == WLAN_EID_VENDOR_SPECIFIC && p[1] >= 6 &&
		     (os_memcmp(&p[2], "\x00\x50\xF2\x01\x01\x00", 6) == 0)) ||
		    (p[0] == WLAN_EID_VENDOR_SPECIFIC && p[1] >= 4 &&
		     (os_memcmp(&p[2], "\x50\x6F\x9A\x12", 4) == 0)) ||
		    (p[0] == WLAN_EID_RSN && p[1] >= 2)) {
			if (wpa_sm_set_assoc_wpa_ie(wpa_s->wpa, p, len))
				break;
			found = 1;
			wpa_find_assoc_pmkid(wpa_s);
			break;
		}
		l -= len;
		p += len;
	}
	if (!found && data->assoc_info.req_ies)
		wpa_sm_set_assoc_wpa_ie(wpa_s->wpa, NULL, 0);

#ifdef CONFIG_FILS
#ifdef CONFIG_SME
	if ((wpa_s->sme.auth_alg == WPA_AUTH_ALG_FILS ||
	     wpa_s->sme.auth_alg == WPA_AUTH_ALG_FILS_SK_PFS) &&
	    (!data->assoc_info.resp_frame ||
	     fils_process_assoc_resp(wpa_s->wpa,
				     data->assoc_info.resp_frame,
				     data->assoc_info.resp_frame_len) < 0)) {
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_UNSPECIFIED);
		return -1;
	}
#endif /* CONFIG_SME */

	/* Additional processing for FILS when SME is in driver */
	if (wpa_s->auth_alg == WPA_AUTH_ALG_FILS &&
	    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME))
		wpa_sm_set_reset_fils_completed(wpa_s->wpa, 1);
#endif /* CONFIG_FILS */

#ifdef CONFIG_OWE
	if (wpa_s->key_mgmt == WPA_KEY_MGMT_OWE &&
	    (wpa_drv_get_bssid(wpa_s, bssid) < 0 ||
	     owe_process_assoc_resp(wpa_s->wpa, bssid,
				    data->assoc_info.resp_ies,
				    data->assoc_info.resp_ies_len) < 0)) {
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_UNSPECIFIED);
		return -1;
	}
#endif /* CONFIG_OWE */

#ifdef CONFIG_IEEE80211R
#ifdef CONFIG_SME
	if (wpa_s->sme.auth_alg == WPA_AUTH_ALG_FT) {
		if (wpa_drv_get_bssid(wpa_s, bssid) < 0 ||
		    wpa_ft_validate_reassoc_resp(wpa_s->wpa,
						 data->assoc_info.resp_ies,
						 data->assoc_info.resp_ies_len,
						 bssid) < 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "FT: Validation of "
				"Reassociation Response failed");
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_INVALID_IE);
			return -1;
		}
	}

	p = data->assoc_info.resp_ies;
	l = data->assoc_info.resp_ies_len;

#ifdef CONFIG_WPS_STRICT
	if (p && wpa_s->current_ssid &&
	    wpa_s->current_ssid->key_mgmt == WPA_KEY_MGMT_WPS) {
		struct wpabuf *wps;
		wps = ieee802_11_vendor_ie_concat(p, l, WPS_IE_VENDOR_TYPE);
		if (wps == NULL) {
			wpa_msg(wpa_s, MSG_INFO, "WPS-STRICT: AP did not "
				"include WPS IE in (Re)Association Response");
			return -1;
		}

		if (wps_validate_assoc_resp(wps) < 0) {
			wpabuf_free(wps);
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_INVALID_IE);
			return -1;
		}
		wpabuf_free(wps);
	}
#endif /* CONFIG_WPS_STRICT */

	/* Go through the IEs and make a copy of the MDIE, if present. */
	while (p && l >= 2) {
		len = p[1] + 2;
		if (len > l) {
			wpa_hexdump(MSG_DEBUG, "Truncated IE in assoc_info",
				    p, l);
			break;
		}
#ifdef CONFIG_IEEE80211R
		if (p[0] == WLAN_EID_MOBILITY_DOMAIN &&
		    p[1] >= MOBILITY_DOMAIN_ID_LEN) {
			wpa_s->sme.ft_used = 1;
			os_memcpy(wpa_s->sme.mobility_domain, p + 2,
				  MOBILITY_DOMAIN_ID_LEN);
			break;
		}
#endif /* CONFIG_IEEE80211R */
		l -= len;
		p += len;
	}
#endif /* CONFIG_SME */

	/* Process FT when SME is in the driver */
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
	    wpa_ft_is_completed(wpa_s->wpa)) {
		if (wpa_drv_get_bssid(wpa_s, bssid) < 0 ||
		    wpa_ft_validate_reassoc_resp(wpa_s->wpa,
						 data->assoc_info.resp_ies,
						 data->assoc_info.resp_ies_len,
						 bssid) < 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "FT: Validation of "
				"Reassociation Response failed");
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_INVALID_IE);
			return -1;
		}
		wpa_dbg(wpa_s, MSG_DEBUG, "FT: Reassociation Response done");
	}

	wpa_sm_set_ft_params(wpa_s->wpa, data->assoc_info.resp_ies,
			     data->assoc_info.resp_ies_len);
#endif /* CONFIG_IEEE80211R */

	/* WPA/RSN IE from Beacon/ProbeResp */
	p = data->assoc_info.beacon_ies;
	l = data->assoc_info.beacon_ies_len;

	/* Go through the IEs and make a copy of the WPA/RSN IEs, if present.
	 */
	wpa_found = rsn_found = 0;
	while (p && l >= 2) {
		len = p[1] + 2;
		if (len > l) {
			wpa_hexdump(MSG_DEBUG, "Truncated IE in beacon_ies",
				    p, l);
			break;
		}
		if (!wpa_found &&
		    p[0] == WLAN_EID_VENDOR_SPECIFIC && p[1] >= 6 &&
		    os_memcmp(&p[2], "\x00\x50\xF2\x01\x01\x00", 6) == 0) {
			wpa_found = 1;
			wpa_sm_set_ap_wpa_ie(wpa_s->wpa, p, len);
		}

		if (!rsn_found &&
		    p[0] == WLAN_EID_RSN && p[1] >= 2) {
			rsn_found = 1;
			wpa_sm_set_ap_rsn_ie(wpa_s->wpa, p, len);
		}

		l -= len;
		p += len;
	}

	if (!wpa_found && data->assoc_info.beacon_ies)
		wpa_sm_set_ap_wpa_ie(wpa_s->wpa, NULL, 0);
	if (!rsn_found && data->assoc_info.beacon_ies)
		wpa_sm_set_ap_rsn_ie(wpa_s->wpa, NULL, 0);
	if (wpa_found || rsn_found)
		wpa_s->ap_ies_from_associnfo = 1;

	if (wpa_s->assoc_freq && data->assoc_info.freq &&
	    wpa_s->assoc_freq != data->assoc_info.freq) {
		wpa_printf_dbg(MSG_DEBUG, "Operating frequency changed from "
			   "%u to %u MHz",
			   wpa_s->assoc_freq, data->assoc_info.freq);
		wpa_supplicant_update_scan_results(wpa_s);
	}

	wpa_s->assoc_freq = data->assoc_info.freq;

	return 0;
}


#ifdef	CONFIG_SUPP27_EVENTS
static int wpa_supplicant_assoc_update_ie(struct wpa_supplicant *wpa_s)
{
	const u8 *bss_wpa = NULL, *bss_rsn = NULL;

	if (!wpa_s->current_bss || !wpa_s->current_ssid)
		return -1;

	if (!wpa_key_mgmt_wpa_any(wpa_s->current_ssid->key_mgmt))
		return 0;

	bss_wpa = wpa_bss_get_vendor_ie(wpa_s->current_bss,
					WPA_IE_VENDOR_TYPE);
	bss_rsn = wpa_bss_get_ie(wpa_s->current_bss, WLAN_EID_RSN);

	if (wpa_sm_set_ap_wpa_ie(wpa_s->wpa, bss_wpa,
				 bss_wpa ? 2 + bss_wpa[1] : 0) ||
	    wpa_sm_set_ap_rsn_ie(wpa_s->wpa, bss_rsn,
				 bss_rsn ? 2 + bss_rsn[1] : 0))
		return -1;

	return 0;
}
#endif	/* CONFIG_SUPP27_EVENTS */


#ifdef CONFIG_FST
static void wpas_fst_update_mb_assoc(struct wpa_supplicant *wpa_s,
				     union wpa_event_data *data)
{
	struct assoc_info *ai = data ? &data->assoc_info : NULL;
	struct wpa_bss *bss = wpa_s->current_bss;
	const u8 *ieprb, *iebcn;

	wpabuf_free(wpa_s->received_mb_ies);
	wpa_s->received_mb_ies = NULL;

	if (ai &&
	    !wpas_fst_update_mbie(wpa_s, ai->resp_ies, ai->resp_ies_len)) {
		wpa_printf_dbg(MSG_DEBUG,
			   "FST: MB IEs updated from Association Response frame");
		return;
	}

	if (ai &&
	    !wpas_fst_update_mbie(wpa_s, ai->beacon_ies, ai->beacon_ies_len)) {
		wpa_printf_dbg(MSG_DEBUG,
			   "FST: MB IEs updated from association event Beacon IEs");
		return;
	}

	if (!bss)
		return;
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
	ieprb = (const u8 *) (bss + 1);
#else
	ieprb = (const u8 *) bss->ie;
#endif
	iebcn = ieprb + bss->ie_len;

	if (!wpas_fst_update_mbie(wpa_s, ieprb, bss->ie_len))
		wpa_printf_dbg(MSG_DEBUG, "FST: MB IEs updated from bss IE");
	else if (!wpas_fst_update_mbie(wpa_s, iebcn, bss->beacon_ie_len))
		wpa_printf_dbg(MSG_DEBUG, "FST: MB IEs updated from bss beacon IE");
}
#endif /* CONFIG_FST */


static void wpa_supplicant_event_assoc(struct wpa_supplicant *wpa_s,
				       union wpa_event_data *data)
{
	u8 bssid[ETH_ALEN];
	int ft_completed;
	int already_authorized;
#ifdef	CONFIG_SUPP27_EVENTS
	int new_bss = 0;
#endif	/* CONFIG_SUPP27_EVENTS */

#ifdef	CONFIG_SUPP27_EVENTS
#ifdef CONFIG_AP
	if (wpa_s->ap_iface) {
		if (!data)
			return;
		hostapd_notif_assoc(wpa_s->ap_iface->bss[0],
				    data->assoc_info.addr,
				    data->assoc_info.req_ies,
				    data->assoc_info.req_ies_len,
				    data->assoc_info.reassoc);
		return;
	}
#endif /* CONFIG_AP */

	da16x_eloop_cancel_timeout(wpas_network_reenabled, wpa_s, NULL);
#endif	/* CONFIG_SUPP27_EVENTS */

	ft_completed = wpa_ft_is_completed(wpa_s->wpa);

	if (data && wpa_supplicant_event_associnfo(wpa_s, data) < 0)
		return;

#ifdef	CONFIG_FILS
	/*
	 * FILS authentication can share the same mechanism to mark the
	 * connection fully authenticated, so set ft_completed also based on
	 * FILS result.
	 */
	if (!ft_completed)
		ft_completed = wpa_fils_is_completed(wpa_s->wpa);
#endif	/* CONFIG_FILS */

	if (wpa_drv_get_bssid(wpa_s, bssid) < 0) {
		wpa_dbg(wpa_s, MSG_ERROR, "Failed to get BSSID");
		wpa_supplicant_deauthenticate(
			wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		return;
	}

	wpa_supplicant_set_state(wpa_s, WPA_ASSOCIATED);
	if (os_memcmp(bssid, wpa_s->bssid, ETH_ALEN) != 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Associated to a new BSS: BSSID="
			MACSTR, MAC2STR(bssid));
#ifdef	CONFIG_SUPP27_EVENTS
		new_bss = 1;
#endif	/* CONFIG_SUPP27_EVENTS */

		random_add_randomness(bssid, ETH_ALEN);
		os_memcpy(wpa_s->bssid, bssid, ETH_ALEN);
		os_memset(wpa_s->pending_bssid, 0, ETH_ALEN);

#ifdef CONFIG_NOTIFY
		wpas_notify_bssid_changed(wpa_s);
#endif /* CONFIG_NOTIFY */

		if (wpa_supplicant_dynamic_keys(wpa_s) && !ft_completed) {
			wpa_clear_keys(wpa_s, bssid);
		}
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		if (wpa_supplicant_select_config(wpa_s) < 0) {
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_DEAUTH_LEAVING);
			return;
		}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	}

#ifdef	CONFIG_SUPP27_EVENTS
	if (
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	    wpa_s->conf->ap_scan == 1 &&
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	    wpa_s->drv_flags & WPA_DRIVER_FLAGS_BSS_SELECTION) {
		if (wpa_supplicant_assoc_update_ie(wpa_s) < 0 && new_bss)
			wpa_dbg(wpa_s, MSG_WARNING,
				"WPA/RSN IEs not updated");
	}
#endif	/* CONFIG_SUPP27_EVENTS */

#ifdef	CONFIG_FST
	wpas_fst_update_mb_assoc(wpa_s, data);
#endif	/* CONFIG_FST */

#ifdef CONFIG_SME
	os_memcpy(wpa_s->sme.prev_bssid, bssid, ETH_ALEN);
	wpa_s->sme.prev_bssid_set = 1;
	wpa_s->sme.last_unprot_disconnect.sec = 0;
#endif /* CONFIG_SME */

	da16x_notice_prt(">>> Associated with " MACSTR "\n", MAC2STR(bssid));

#if defined ( CONFIG_SCHED_SCAN )
	if (wpa_s->current_ssid) {
		/* When using scanning (ap_scan=1), SIM PC/SC interface can be
		 * initialized before association, but for other modes,
		 * initialize PC/SC here, if the current configuration needs
		 * smartcard or SIM/USIM. */
		wpa_supplicant_scard_init(wpa_s, wpa_s->current_ssid);
	}
#endif // CONFIG_SCHED_SCAN

	wpa_sm_notify_assoc(wpa_s->wpa, bssid);
	if (wpa_s->l2)
		l2_packet_notify_auth_start(wpa_s->l2);

	already_authorized = data && data->assoc_info.authorized;

	/*
	 * Set portEnabled first to FALSE in order to get EAP state machine out
	 * of the SUCCESS state and eapSuccess cleared. Without this, EAPOL PAE
	 * state machine may transit to AUTHENTICATING state based on obsolete
	 * eapSuccess and then trigger BE_AUTH to SUCCESS and PAE to
	 * AUTHENTICATED without ever giving chance to EAP state machine to
	 * reset the state.
	 */
#ifdef	IEEE8021X_EAPOL
	if (!ft_completed && !already_authorized) {
		eapol_sm_notify_portEnabled(wpa_s->eapol, FALSE);
		eapol_sm_notify_portValid(wpa_s->eapol, FALSE);
	}
	if (wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt) ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_DPP ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_OWE ||
		ft_completed ||
	    already_authorized)
		eapol_sm_notify_eap_success(wpa_s->eapol, FALSE);

	/* 802.1X::portControl = Auto */
	eapol_sm_notify_portEnabled(wpa_s->eapol, TRUE);
	wpa_s->eapol_received = 0;
#endif	/* IEEE8021X_EAPOL */

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_NONE ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE ||
	    (wpa_s->current_ssid &&
	     wpa_s->current_ssid->mode == IEEE80211_MODE_IBSS)) {
		if (wpa_s->current_ssid &&
		    wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE &&
		    (wpa_s->drv_flags &
		     WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE)) {
			/*
			 * Set the key after having received joined-IBSS event
			 * from the driver.
			 */
			wpa_supplicant_set_wpa_none_key(wpa_s,
							wpa_s->current_ssid);
		}
		wpa_supplicant_cancel_auth_timeout(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_COMPLETED);
	} else if (!ft_completed)
	{
		/* Timeout for receiving the first EAPOL packet */
		wpa_supplicant_req_auth_timeout(wpa_s, 10, 0);
	}
	wpa_supplicant_cancel_scan(wpa_s);

	if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE) &&
	    wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt)) {
		/*
		 * We are done; the driver will take care of RSN 4-way
		 * handshake.
		 */
		wpa_supplicant_cancel_auth_timeout(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_COMPLETED);

#ifdef	IEEE8021X_EAPOL
		eapol_sm_notify_portValid(wpa_s->eapol, TRUE);
		eapol_sm_notify_eap_success(wpa_s->eapol, TRUE);
#endif	/* IEEE8021X_EAPOL */
	} else if ((wpa_s->drv_flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE) &&
		   wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt)) {
		/*
		 * The driver will take care of RSN 4-way handshake, so we need
		 * to allow EAPOL supplicant to complete its work without
		 * waiting for WPA supplicant.
		 */
#ifdef	IEEE8021X_EAPOL
		eapol_sm_notify_portValid(wpa_s->eapol, TRUE);
#endif	/* IEEE8021X_EAPOL */
	} else if (ft_completed) {
		/*
		 * FT protocol completed - make sure EAPOL state machine ends
		 * up in authenticated.
		 */
		wpa_supplicant_cancel_auth_timeout(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_COMPLETED);

#ifdef	IEEE8021X_EAPOL
		eapol_sm_notify_portValid(wpa_s->eapol, TRUE);
		eapol_sm_notify_eap_success(wpa_s->eapol, TRUE);
#endif	/* IEEE8021X_EAPOL */
	}

#ifdef	IEEE8021X_EAPOL
	wpa_s->last_eapol_matches_bssid = 0;
#endif	/* IEEE8021X_EAPOL */

#ifdef	IEEE8021X_EAPOL
	if (wpa_s->pending_eapol_rx) {
		struct os_reltime now, age;
		os_get_reltime(&now);
		os_reltime_sub(&now, &wpa_s->pending_eapol_rx_time, &age);
		if (age.sec == 0 && age.usec < 200000 &&
		    os_memcmp(wpa_s->pending_eapol_rx_src, bssid, ETH_ALEN) ==
		    0) {
			da16x_assoc_prt("<%s> Process pending EAPOL "
                "frame that was received just before "
                "association notification\n", __func__);

			wpa_supplicant_rx_eapol(
				wpa_s, wpa_s->pending_eapol_rx_src,
				wpabuf_head(wpa_s->pending_eapol_rx),
				wpabuf_len(wpa_s->pending_eapol_rx));
		}
		wpabuf_free(wpa_s->pending_eapol_rx);
		wpa_s->pending_eapol_rx = NULL;
	}
#endif	/* IEEE8021X_EAPOL */

	if ((wpa_s->key_mgmt == WPA_KEY_MGMT_NONE ||
	     wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) &&
	    wpa_s->current_ssid &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SET_KEYS_AFTER_ASSOC_DONE)) {
		/* Set static WEP keys again */
		wpa_set_wep_keys(wpa_s, wpa_s->current_ssid);
	}

#ifdef CONFIG_IBSS_RSN
	if (wpa_s->current_ssid &&
	    wpa_s->current_ssid->mode == WPAS_MODE_IBSS &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_NONE &&
	    wpa_s->key_mgmt != WPA_KEY_MGMT_WPA_NONE &&
	    wpa_s->ibss_rsn == NULL) {
		wpa_s->ibss_rsn = ibss_rsn_init(wpa_s, wpa_s->current_ssid);
		if (!wpa_s->ibss_rsn) {
			wpa_msg(wpa_s, MSG_INFO, "Failed to init IBSS RSN");
			wpa_supplicant_deauthenticate(
				wpa_s, WLAN_REASON_DEAUTH_LEAVING);
			return;
		}

		ibss_rsn_set_psk(wpa_s->ibss_rsn, wpa_s->current_ssid->psk);
	}
#endif /* CONFIG_IBSS_RSN */

	wpas_wps_notify_assoc(wpa_s, bssid);

#if 0	/* by Bright : Merge 2.7 */
	if (data) {
		wmm_ac_notify_assoc(wpa_s, data->assoc_info.resp_ies,
				    data->assoc_info.resp_ies_len,
				    &data->assoc_info.wmm_params);

		if (wpa_s->reassoc_same_bss)
			wmm_ac_restore_tspecs(wpa_s);
	}

#ifdef CONFIG_FILS
	if (wpa_key_mgmt_fils(wpa_s->key_mgmt)) {
		struct wpa_bss *bss = wpa_bss_get_bssid(wpa_s, bssid);
		const u8 *fils_cache_id = wpa_bss_get_fils_cache_id(bss);

		if (fils_cache_id)
			wpa_sm_set_fils_cache_id(wpa_s->wpa, fils_cache_id);
	}
#endif /* CONFIG_FILS */
#endif	/* 0 */

	// for get_cur_connected_bssid() API
	os_memcpy(cur_connected_bssid, bssid, ETH_ALEN);
}


static int disconnect_reason_recoverable(u16 reason_code)
{
	return reason_code == WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY ||
		reason_code == WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
		reason_code == WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA;
}

static void wpa_supplicant_event_disassoc(struct wpa_supplicant *wpa_s,
					  u16 reason_code,
					  int locally_generated)
{
	const u8 *bssid;

	extern int get_run_mode(void);
	extern void wpa_supp_dpm_clear_conn_info(struct wpa_supplicant *wpa_s);

    //
    // When enabled DPM mode, need to maintain "auth_failures" filed in case of contineous Wrong-PSK.
    //
    if (reason_code != WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT)
	    wpa_supp_dpm_clear_conn_info(wpa_s);

#ifdef	CONFIG_CONCURRENT
#ifdef	CONFIG_P2P
	if (get_run_mode() == STA_P2P_CONCURRENT_MODE) {
		struct wpa_supplicant *p2p_wpa_s = wpa_s->global->ifaces;

		if (!p2p_wpa_s->global->p2p) {
			da16x_notice_prt("Now P2P interface is reactivated.\n");
			wpas_p2p_init(p2p_wpa_s->global, p2p_wpa_s);
		}
	}
#endif	/* CONFIG_P2P */
#endif	/* CONFIG_CONCURRENT */

	extern int wifi_disconn_reason_code;
	wifi_disconn_reason_code = reason_code;
	/* Notify Wi-Fi connection status on AT-CMD UART1 */
	atcmd_asynchony_event(1, NULL);
	
	/* Notify Wi-Fi connection status on user callback function */	
	if (wifi_disconn_notify_cb != NULL) {
		wifi_disconn_notify_cb(reason_code);
	}

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE) {
		/*
		 * At least Host AP driver and a Prism3 card seemed to be
		 * generating streams of disconnected events when configuring
		 * IBSS for WPA-None. Ignore them for now.
		 */
		return;
	}

	bssid = wpa_s->bssid;
	if (is_zero_ether_addr(bssid))
		bssid = wpa_s->pending_bssid;

	if (!is_zero_ether_addr(bssid) ||
	    wpa_s->wpa_state >= WPA_AUTHENTICATING) {

		da16x_notice_prt(RED_COLOR "[%s] " WPA_EVENT_DISCONNECTED "bssid=" MACSTR	/* color OTA_DPM_TEST */
				" reason=%d%s\n" CLEAR_COLOR,
				__func__, MAC2STR(bssid), reason_code,
				locally_generated ? " locally_generated=1" : "");

		if (os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0)
		{
			da16x_eloop_register_timeout(0, 0, wpa_supplicant_reconfig_net, wpa_s, NULL);
		}
	}
}


static int could_be_psk_mismatch(struct wpa_supplicant *wpa_s, u16 reason_code,
				 int locally_generated)
{
	if (wpa_s->wpa_state != WPA_4WAY_HANDSHAKE ||
	    !wpa_key_mgmt_wpa_psk(wpa_s->key_mgmt))
		return 0; /* Not in 4-way handshake with PSK */

	/*
	 * It looks like connection was lost while trying to go through PSK
	 * 4-way handshake. Filter out known disconnection cases that are caused
	 * by something else than PSK mismatch to avoid confusing reports.
	 */

	if (locally_generated) {
		if (reason_code == WLAN_REASON_IE_IN_4WAY_DIFFERS)
			return 0;
	}

	return 1;
}

static void wpa_supplicant_event_disassoc_finish(struct wpa_supplicant *wpa_s,
						 u16 reason_code,
						 int locally_generated)
{
	const u8 *bssid;
	int authenticating;
	u8 prev_pending_bssid[ETH_ALEN];
#if defined(CONFIG_FAST_RECONNECT) || defined(CONFIG_FAST_RECONNECT_V2)
	struct wpa_bss *fast_reconnect = NULL;
	struct wpa_ssid *fast_reconnect_ssid = NULL;
#endif /* CONFIG_FAST_RECONNECT */
	struct wpa_ssid *last_ssid;
	struct wpa_bss *curr = NULL;

	authenticating = wpa_s->wpa_state == WPA_AUTHENTICATING;
	os_memcpy(prev_pending_bssid, wpa_s->pending_bssid, ETH_ALEN);

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPA_NONE) {
		/*
		 * At least Host AP driver and a Prism3 card seemed to be
		 * generating streams of disconnected events when configuring
		 * IBSS for WPA-None. Ignore them for now.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG, "Disconnect event - ignore in "
			"IBSS/WPA-None mode");
		return;
	}

#ifdef	CONFIG_SUPP27_EVENTS
	if (!wpa_s->disconnected && wpa_s->wpa_state >= WPA_AUTHENTICATING &&
	    reason_code == WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY &&
	    locally_generated)
		/*
		 * Remove the inactive AP (which is probably out of range) from
		 * the BSS list after marking disassociation. In particular
		 * macd11-based drivers use the
		 * WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY reason code in
		 * locally generated disconnection events for cases where the
		 * AP does not reply anymore.
		 */
		curr = wpa_s->current_bss;
#endif	/* CONFIG_SUPP27_EVENTS */

	if (could_be_psk_mismatch(wpa_s, reason_code, locally_generated)) {
		wpa_msg(wpa_s, MSG_INFO, "WPA: 4-Way Handshake failed - "
			"pre-shared key may be incorrect");
#ifdef	CONFIG_P2P
		if (wpas_p2p_4way_hs_failed(wpa_s) > 0)
			return; /* P2P group removed */
#endif	/* CONFIG_P2P */
		wpas_auth_failed(wpa_s, "WRONG_KEY");
		reason_code = WLAN_REASON_PEERKEY_MISMATCH; //  Add by Dialog

		if (stop_wifi_conn_try_at_wrong_key_flag == TRUE) {
#ifdef  CONFIG_RECONNECT_OPTIMIZE /* by Shingu 20170717 (Scan Optimization) */
			wpa_s->reassoc_try = 0;
#endif  /* CONFIG_RECONNECT_OPTIMIZE */

			wpa_s->reassociate = 0;
			wpa_s->disconnected = 1;
			wpa_supplicant_cancel_scan(wpa_s);

			if (dpm_abnormal_sleep1_flag == TRUE) {
				// disable the current network profile
				wpa_s->current_ssid->disabled = 1;				
				write_nvram_int("N0_disabled", 1);
			
				// let dpm_sleep_daemon trigger sleep1 instead of sleep2
				force_dpm_abnormal_sleep1();
			}

			force_dpm_abnormal_sleep_by_wifi_conn_fail();
		}

		/* Notify atcmd host of Wi-Fi connection status */
		extern int wifi_conn_fail_reason_code;
		extern void wifi_conn_fail_noti_to_atcmd_host(void);
		wifi_conn_fail_reason_code = reason_code;
		wifi_conn_fail_noti_to_atcmd_host();

		/* Notify Wi-Fi connection status on user callback function */
		if (wifi_conn_fail_notify_cb != NULL) {
			wifi_conn_fail_notify_cb(reason_code);
		}

#ifdef __SUPPORT_CON_EASY__
		/* set the Connection try status as "WRONG PW(1)" */
		ce_set_conn_try_result_status(1);
#endif	/* __SUPPORT_CON_EASY__ */

	}

	if (   !wpa_s->disconnected
		&& ((   !wpa_s->auto_reconnect_disabled
			|| wpa_s->key_mgmt == WPA_KEY_MGMT_WPS
#ifdef	CONFIG_SUPP27_EVENTS
			|| wpas_wps_searching(wpa_s)
#endif	/* CONFIG_SUPP27_EVENTS */
#ifdef CONFIG_AUTH_MAX_FAILURES
             )  && (wpa_s->current_ssid->auth_failures < wpa_s->current_ssid->auth_max_failures
                    || wpa_s->current_ssid->auth_max_failures == 0)
#endif // CONFIG_AUTH_MAX_FAILURES
			)
		) {
		da16x_ev_prt("[%s] Auto connect enabled: try to "
            "reconnect (wps=%d wpa_state=%d)\n",
            __func__,
            wpa_s->key_mgmt == WPA_KEY_MGMT_WPS,
            wpa_s->wpa_state);

		if (wpa_s->wpa_state == WPA_COMPLETED &&
		    wpa_s->current_ssid &&
		    wpa_s->current_ssid->mode == WPAS_MODE_INFRA
#ifndef CONFIG_FAST_RECONNECT_V2
		    && !locally_generated &&
		    disconnect_reason_recoverable(reason_code)
#endif /* CONFIG_FAST_RECONNECT_V2 */
		 ) {
			/*
			 * It looks like the AP has dropped association with
			 * us, but could allow us to get back in. Try to
			 * reconnect to the same BSS without full scan to save
			 * time for some common cases.
			 */
#if defined(CONFIG_FAST_RECONNECT) || defined(CONFIG_FAST_RECONNECT_V2)
			fast_reconnect = wpa_s->current_bss;
			fast_reconnect_ssid = wpa_s->current_ssid;
			fast_reconnect_tries = 0;
#endif /* CONFIG_FAST_RECONNECT */
		} else if (wpa_s->wpa_state >= WPA_ASSOCIATING) {
#ifdef	CONFIG_RECONNECT_OPTIMIZE /* by Shingu 20170717 (Scan Optimization) */
			wpa_s->reassoc_freq = wpa_s->assoc_freq;
#endif	/* CONFIG_RECONNECT_OPTIMIZE */
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
			wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
			wpa_supplicant_req_scan(wpa_s, 0, 100000);
		} else {
			wpa_dbg(wpa_s, MSG_DEBUG, "Do not request new immediate scan");
		}
	} else {
#ifdef CONFIG_AUTH_MAX_FAILURES
	    if (wpa_s->current_ssid->auth_failures >= wpa_s->current_ssid->auth_max_failures) {
    	    wpa_dbg(wpa_s, MSG_DEBUG, "Auto connect: do not try to re-connect, auth_failures=%d/%d\n",
    	            wpa_s->current_ssid->auth_failures, wpa_s->current_ssid->auth_max_failures);

    	    wpa_s->current_ssid->auth_failures = 0;
	    } else
#endif // CONFIG_AUTH_MAX_FAILURES
        {
    		wpa_dbg(wpa_s, MSG_DEBUG, "Auto connect disabled: do not "
    			"try to re-connect");
        }
		wpa_s->reassociate = 0;
		wpa_s->disconnected = 1;
#if defined ( CONFIG_SCHED_SCAN )
		if (!wpa_s->pno)
			wpa_supplicant_cancel_sched_scan(wpa_s);
#endif // CONFIG_SCHED_SCAN
	}

	bssid = wpa_s->bssid;
	if (is_zero_ether_addr(bssid))
		bssid = wpa_s->pending_bssid;
	if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
#ifdef CONFIG_FAST_RECONNECT_V2
		if(!fast_reconnect && !chk_dpm_wakeup())
#endif  /* CONFIG_FAST_RECONNECT_V2 */
			wpas_connection_failed(wpa_s, bssid);

	wpa_sm_notify_disassoc(wpa_s->wpa);

	if (locally_generated)
		wpa_s->disconnect_reason = -reason_code;
	else
		wpa_s->disconnect_reason = reason_code;

#ifdef CONFIG_NOTIFY
	wpas_notify_disconnect_reason(wpa_s);
#endif /* CONFIG_NOTIFY */

	if (wpa_supplicant_dynamic_keys(wpa_s)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Disconnect event - remove keys");
		wpa_clear_keys(wpa_s, wpa_s->bssid);
	}
	last_ssid = wpa_s->current_ssid;
#if defined (CONFIG_SAE)
	if (wpa_s->current_ssid &&
		wpa_key_mgmt_sae(wpa_s->current_ssid->key_mgmt)) {
			wpa_dbg(wpa_s, MSG_DEBUG, "Flushing PMKSA ...");
			wpa_sm_aborted_cached(wpa_s->wpa);
			wpa_sm_pmksa_cache_flush(wpa_s->wpa, wpa_s->current_ssid);
	}
#endif // CONFIG_SAE
	wpa_supplicant_mark_disassoc(wpa_s);

#ifndef CONFIG_REUSED_UMAC_BSS_LIST
	if (curr)
		wpa_bss_remove(wpa_s, curr, "Connection to AP lost");
#else
	if(curr)
#if defined(CONFIG_FAST_RECONNECT) || defined(CONFIG_FAST_RECONNECT_V2)		
		if(fast_reconnect ==NULL)
#endif			
		wpa_s->current_bss = NULL;		
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

	if (authenticating && (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME)) {
#ifdef	CONFIG_SUPP27_EVENTS
		sme_disassoc_while_authenticating(wpa_s, prev_pending_bssid);
#endif	/* CONFIG_SUPP27_EVENTS */
		wpa_s->current_ssid = last_ssid;
	}
#ifdef CONFIG_FAST_RECONNECT
	if (fast_reconnect &&
	    !wpas_network_disabled(wpa_s, fast_reconnect_ssid) &&
#ifdef	CONFIG_DISALLOW_BSSID
	    !disallowed_bssid(wpa_s, fast_reconnect->bssid) &&
#endif	/* CONFIG_DISALLOW_BSSID */
#ifdef	CONFIG_DISALLOW_SSID
	    !disallowed_ssid(wpa_s, fast_reconnect->ssid,
			     fast_reconnect->ssid_len) &&
#endif	/* CONFIG_DISALLOW_SSID */
	    !wpas_temp_disabled(wpa_s, fast_reconnect_ssid)
#ifdef	CONFIG_SUPP27_EVENTS
		&& !wpa_is_bss_tmp_disallowed(wpa_s, fast_reconnect->bssid)
#endif	/* CONFIG_SUPP27_EVENTS */
		) {
#ifndef CONFIG_NO_SCAN_PROCESSING
		da16x_ev_prt("[%s] Try to reconnect to the same BSS\n", __func__);
		da16x_is_fast_reconnect = 1; /* FC9000 Only */
		if (wpa_supplicant_connect(wpa_s, fast_reconnect,
					   fast_reconnect_ssid) < 0) {
			/* Recover through full scan */
			da16x_is_fast_reconnect = 0; /* FC9000 Only */
			wpa_supplicant_req_scan(wpa_s, 0, 100000);
		}
#endif /* CONFIG_NO_SCAN_PROCESSING */
	} else if (fast_reconnect) {
		/*
		 * Could not reconnect to the same BSS due to network being
		 * disabled. Use a new scan to match the alternative behavior
		 * above, i.e., to continue automatic reconnection attempt in a
		 * way that enforces disabled network rules.
		 */
		wpa_supplicant_req_scan(wpa_s, 0, 100000);
	}
#endif /* CONFIG_FAST_RECONNECT */

#ifdef CONFIG_FAST_RECONNECT_V2
	if (fast_reconnect && !wpas_network_disabled(wpa_s, fast_reconnect_ssid) &&
	    !wpas_temp_disabled(wpa_s, fast_reconnect_ssid) && chk_dpm_wakeup()) {
	       fast_reconnect_tries++;
	    	da16x_notice_prt("[%s] Try to reconnect to the same BSS without Scan(tries=%d)\n", __func__, fast_reconnect_tries );
		da16x_is_fast_reconnect = 1;
		if (wpa_supplicant_connect(wpa_s, fast_reconnect,
					   fast_reconnect_ssid) < 0) {
			/* Recover through full scan */
			da16x_is_fast_reconnect = 0;
			da16x_notice_prt("[%s] Fast reconnect failed. Request scan. \n", __func__);
			wpa_supplicant_req_scan(wpa_s, 0, 100000);
		}
	}
#endif  /* CONFIG_FAST_RECONNECT_V2 */
}


#ifdef CONFIG_DELAYED_MIC_ERROR_REPORT
void wpa_supplicant_delayed_mic_error_report(void *eloop_ctx, void *sock_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (!wpa_s->pending_mic_error_report)
		return;

	wpa_dbg(wpa_s, MSG_DEBUG, "WPA: Sending pending MIC error report");
	wpa_sm_key_request(wpa_s->wpa, 1, wpa_s->pending_mic_error_pairwise);
	wpa_s->pending_mic_error_report = 0;
}
#endif /* CONFIG_DELAYED_MIC_ERROR_REPORT */

#ifdef	CONFIG_MIC_FAILURE_RSP
static void
wpa_supplicant_event_michael_mic_failure(struct wpa_supplicant *wpa_s,
					 union wpa_event_data *data)
{
	int pairwise;
	struct os_reltime t;

	wpa_dbg(wpa_s, MSG_WARNING, "Michael MIC failure detected");
	pairwise = (data && data->michael_mic_failure.unicast);
	os_get_reltime(&t);
	if ((wpa_s->last_michael_mic_error.sec &&
	     !os_reltime_expired(&t, &wpa_s->last_michael_mic_error, 60)) ||
	    wpa_s->pending_mic_error_report) {
		if (wpa_s->pending_mic_error_report) {
			/*
			 * Send the pending MIC error report immediately since
			 * we are going to start countermeasures and AP better
			 * do the same.
			 */
			wpa_sm_key_request(wpa_s->wpa, 1,
					   wpa_s->pending_mic_error_pairwise);
		}

		/* Send the new MIC error report immediately since we are going
		 * to start countermeasures and AP better do the same.
		 */
		wpa_sm_key_request(wpa_s->wpa, 1, pairwise);

		/* initialize countermeasures */
		wpa_s->countermeasures = 1;

		wpa_blacklist_add(wpa_s, wpa_s->bssid);

		wpa_dbg(wpa_s, MSG_WARNING, "TKIP countermeasures started");

		/*
		 * Need to wait for completion of request frame. We do not get
		 * any callback for the message completion, so just wait a
		 * short while and hope for the best. */
		os_sleep(0, 10000);

		wpa_drv_set_countermeasures(wpa_s, 1);
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_MICHAEL_MIC_FAILURE);
		da16x_eloop_cancel_timeout(wpa_supplicant_stop_countermeasures,
				     wpa_s, NULL);
		da16x_eloop_register_timeout(60, 0,
				       wpa_supplicant_stop_countermeasures,
				       wpa_s, NULL);

		/* TODO: mark the AP rejected for 60 second. STA is
		 * allowed to associate with another AP.. */
	} else {
#ifdef CONFIG_DELAYED_MIC_ERROR_REPORT
		if (wpa_s->mic_errors_seen) {
			/*
			 * Reduce the effectiveness of Michael MIC error
			 * reports as a means for attacking against TKIP if
			 * more than one MIC failure is noticed with the same
			 * PTK. We delay the transmission of the reports by a
			 * random time between 0 and 60 seconds in order to
			 * force the attacker wait 60 seconds before getting
			 * the information on whether a frame resulted in a MIC
			 * failure.
			 */
			u8 rval[4];
			int sec;

			if (os_get_random(rval, sizeof(rval)) < 0)
				sec = os_random() % 60;
			else
				sec = WPA_GET_BE32(rval) % 60;
			wpa_dbg(wpa_s, MSG_DEBUG, "WPA: Delay MIC error "
				"report %d seconds", sec);
			wpa_s->pending_mic_error_report = 1;
			wpa_s->pending_mic_error_pairwise = pairwise;

			da16x_eloop_cancel_timeout(
				wpa_supplicant_delayed_mic_error_report,
				wpa_s, NULL);
			da16x_eloop_register_timeout(
				sec, os_random() % 1000000,
				wpa_supplicant_delayed_mic_error_report,
				wpa_s, NULL);
		} else {
			wpa_sm_key_request(wpa_s->wpa, 1, pairwise);
		}
#else /* CONFIG_DELAYED_MIC_ERROR_REPORT */
		wpa_sm_key_request(wpa_s->wpa, 1, pairwise);
#endif /* CONFIG_DELAYED_MIC_ERROR_REPORT */
	}
	wpa_s->last_michael_mic_error = t;
	wpa_s->mic_errors_seen++;
}
#endif	/* CONFIG_MIC_FAILURE_RSP */

#ifdef CONFIG_TERMINATE_ONLASTIF
static int any_interfaces(struct wpa_supplicant *head)
{
	struct wpa_supplicant *wpa_s;

	for (wpa_s = head; wpa_s != NULL; wpa_s = wpa_s->next)
		if (!wpa_s->interface_removed)
			return 1;
	return 0;
}
#endif /* CONFIG_TERMINATE_ONLASTIF */


static void
wpa_supplicant_event_interface_status(struct wpa_supplicant *wpa_s,
				      union wpa_event_data *data)
{
	if (os_strcmp(wpa_s->ifname, data->interface_status.ifname) != 0)
		return;

	switch (data->interface_status.ievent) {
	case EVENT_INTERFACE_ADDED:
		if (!wpa_s->interface_removed)
			break;
		wpa_s->interface_removed = 0;
		wpa_dbg(wpa_s, MSG_DEBUG, "Configured interface was added");
		if (wpa_supplicant_driver_init(wpa_s) < 0) {
			wpa_dbg(wpa_s, MSG_INFO, "Failed to initialize the "
				"driver after interface was added");
		}

#ifdef CONFIG_P2P
		if (!wpa_s->global->p2p &&
		    !wpa_s->global->p2p_disabled &&
		    !wpa_s->conf->p2p_disabled &&
		    (wpa_s->drv_flags &
		     WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
		    wpas_p2p_add_p2pdev_interface(
			    wpa_s) < 0) {
			wpa_printf(MSG_INFO,
				   "P2P: Failed to enable P2P Device interface");
			/* Try to continue without. P2P will be disabled. */
		}
#endif /* CONFIG_P2P */

		break;
	case EVENT_INTERFACE_REMOVED:
		wpa_dbg(wpa_s, MSG_DEBUG, "Configured interface was removed");
		wpa_s->interface_removed = 1;
		wpa_supplicant_mark_disassoc(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_INTERFACE_DISABLED);
		l2_packet_deinit(wpa_s->l2);
		wpa_s->l2 = NULL;

#ifdef CONFIG_P2P
		if (wpa_s->global->p2p &&
		    wpa_s->global->p2p_init_wpa_s->parent == wpa_s &&
		    (wpa_s->drv_flags &
		     WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE)) {
			wpa_dbg(wpa_s, MSG_DEBUG,
				"Removing P2P Device interface");
			wpa_supplicant_remove_iface(
				wpa_s->global, wpa_s->global->p2p_init_wpa_s,
				0);
			wpa_s->global->p2p_init_wpa_s = NULL;
		}
#endif /* CONFIG_P2P */

#ifdef CONFIG_MATCH_IFACE
		if (wpa_s->matched) {
			wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
			break;
		}
#endif /* CONFIG_MATCH_IFACE */

#ifdef CONFIG_TERMINATE_ONLASTIF
		/* check if last interface */
		if (!any_interfaces(wpa_s->global->ifaces))
#if 0	/* by Bright */
			eloop_terminate();
#else
			da16x_ev_prt("[%s] What do I do for timer terminate\n", __func__);
#endif  /* 0 */
#endif /* CONFIG_TERMINATE_ONLASTIF */
		break;
	}
}


#ifdef CONFIG_TDLS
static void wpa_supplicant_event_tdls(struct wpa_supplicant *wpa_s,
				      union wpa_event_data *data)
{
	if (data == NULL)
		return;
	switch (data->tdls.oper) {
	case TDLS_REQUEST_SETUP:
		wpa_tdls_remove(wpa_s->wpa, data->tdls.peer);
		if (wpa_tdls_is_external_setup(wpa_s->wpa))
			wpa_tdls_start(wpa_s->wpa, data->tdls.peer);
		else
			wpa_drv_tdls_oper(wpa_s, TDLS_SETUP, data->tdls.peer);
		break;
	case TDLS_REQUEST_TEARDOWN:
		if (wpa_tdls_is_external_setup(wpa_s->wpa))
			wpa_tdls_teardown_link(wpa_s->wpa, data->tdls.peer,
					       data->tdls.reason_code);
		else
			wpa_drv_tdls_oper(wpa_s, TDLS_TEARDOWN,
					  data->tdls.peer);
		break;
	case TDLS_REQUEST_DISCOVER:
			wpa_tdls_send_discovery_request(wpa_s->wpa,
							data->tdls.peer);
		break;
	}
}
#endif /* CONFIG_TDLS */

#ifdef CONFIG_WNM_SLEEP_MODE
extern int ieee802_11_send_wnmsleep_req(struct wpa_supplicant *wpa_s,
			 u8 action, u16 intval, struct wpabuf *tfs_req);

#ifdef CONFIG_WNM
static void wpa_supplicant_event_wnm(struct wpa_supplicant *wpa_s,
				     union wpa_event_data *data)
{
	if (data == NULL)
		return;
	switch (data->wnm.oper) {
	case WNM_OPER_SLEEP:
		wpa_printf_dbg(MSG_DEBUG, "Start sending WNM-Sleep Request "
			   "(action=%d, intval=%d)",
			   data->wnm.sleep_action, data->wnm.sleep_intval);
		ieee802_11_send_wnmsleep_req(wpa_s, data->wnm.sleep_action,
					     data->wnm.sleep_intval, NULL);
		break;
	}
}
#endif	/* CONFIG_WNM_SLEEP_MODE */
#endif /* CONFIG_WNM */


#ifdef CONFIG_IEEE80211R
static void
wpa_supplicant_event_ft_response(struct wpa_supplicant *wpa_s,
				 union wpa_event_data *data)
{
	if (data == NULL)
		return;

	if (wpa_ft_process_response(wpa_s->wpa, data->ft_ies.ies,
				    data->ft_ies.ies_len,
				    data->ft_ies.ft_action,
				    data->ft_ies.target_ap,
				    data->ft_ies.ric_ies,
				    data->ft_ies.ric_ies_len) < 0) {
		/* TODO: prevent MLME/driver from trying to associate? */
	}
}
#endif /* CONFIG_IEEE80211R */


#ifdef CONFIG_IBSS_RSN
static void wpa_supplicant_event_ibss_rsn_start(struct wpa_supplicant *wpa_s,
						union wpa_event_data *data)
{
	struct wpa_ssid *ssid;
	if (wpa_s->wpa_state < WPA_ASSOCIATED)
		return;
	if (data == NULL)
		return;
	ssid = wpa_s->current_ssid;
	if (ssid == NULL)
		return;
	if (ssid->mode != WPAS_MODE_IBSS || !wpa_key_mgmt_wpa(ssid->key_mgmt))
		return;

	ibss_rsn_start(wpa_s->ibss_rsn, data->ibss_rsn_start.peer);
}


static void wpa_supplicant_event_ibss_auth(struct wpa_supplicant *wpa_s,
					   union wpa_event_data *data)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (ssid == NULL)
		return;

	/* check if the ssid is correctly configured as IBSS/RSN */
	if (ssid->mode != WPAS_MODE_IBSS || !wpa_key_mgmt_wpa(ssid->key_mgmt))
		return;

	ibss_rsn_handle_auth(wpa_s->ibss_rsn, data->rx_mgmt.frame,
			     data->rx_mgmt.frame_len);
}
#endif /* CONFIG_IBSS_RSN */


#ifdef CONFIG_IEEE80211R
static void ft_rx_action(struct wpa_supplicant *wpa_s, const u8 *data,
			 size_t len)
{
	const u8 *sta_addr, *target_ap_addr;
	u16 status;

	wpa_hexdump(MSG_MSGDUMP, "FT: RX Action", data, len);
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME))
		return; /* only SME case supported for now */
	if (len < 1 + 2 * ETH_ALEN + 2)
		return;
	if (data[0] != 2)
		return; /* Only FT Action Response is supported for now */
	sta_addr = data + 1;
	target_ap_addr = data + 1 + ETH_ALEN;
	status = WPA_GET_LE16(data + 1 + 2 * ETH_ALEN);
	wpa_dbg(wpa_s, MSG_DEBUG, "FT: Received FT Action Response: STA "
		MACSTR " TargetAP " MACSTR " status %u",
		MAC2STR(sta_addr), MAC2STR(target_ap_addr), status);

	if (os_memcmp(sta_addr, wpa_s->own_addr, ETH_ALEN) != 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "FT: Foreign STA Address " MACSTR
			" in FT Action Response", MAC2STR(sta_addr));
		return;
	}

	if (status) {
		wpa_dbg(wpa_s, MSG_DEBUG, "FT: FT Action Response indicates "
			"failure (status code %d)", status);
		/* TODO: report error to FT code(?) */
		return;
	}

	if (wpa_ft_process_response(wpa_s->wpa, data + 1 + 2 * ETH_ALEN + 2,
				    len - (1 + 2 * ETH_ALEN + 2), 1,
				    target_ap_addr, NULL, 0) < 0)
		return;

#ifdef CONFIG_SME
	{
		struct wpa_bss *bss;
		bss = wpa_bss_get_bssid(wpa_s, target_ap_addr);
		if (bss)
			wpa_s->sme.freq = bss->freq;
		wpa_s->sme.auth_alg = WPA_AUTH_ALG_FT;
		sme_associate(wpa_s, WPAS_MODE_INFRA, target_ap_addr,
			      WLAN_AUTH_FT);
	}
#endif /* CONFIG_SME */
}
#endif /* CONFIG_IEEE80211R */


#ifdef CONFIG_IEEE80211W
static void wpa_supplicant_event_unprot_deauth(struct wpa_supplicant *wpa_s,
					       struct unprot_deauth *e)
{
#ifdef CONFIG_IEEE80211W
	wpa_printf_dbg(MSG_DEBUG, "Unprotected Deauthentication frame "
		   "dropped: " MACSTR " -> " MACSTR
		   " (reason code %u)",
		   MAC2STR(e->sa), MAC2STR(e->da), e->reason_code);
	da16x_sme_event_unprot_disconnect(wpa_s, e->sa, e->da, e->reason_code);
#endif /* CONFIG_IEEE80211W */
}


static void wpa_supplicant_event_unprot_disassoc(struct wpa_supplicant *wpa_s,
						 struct unprot_disassoc *e)
{
#ifdef CONFIG_IEEE80211W
	wpa_printf_dbg(MSG_DEBUG, "Unprotected Disassociation frame "
		   "dropped: " MACSTR " -> " MACSTR
		   " (reason code %u)",
		   MAC2STR(e->sa), MAC2STR(e->da), e->reason_code);
	da16x_sme_event_unprot_disconnect(wpa_s, e->sa, e->da, e->reason_code);
#endif /* CONFIG_IEEE80211W */
}
#endif /* CONFIG_IEEE80211W */


static void wpas_event_disconnect(struct wpa_supplicant *wpa_s, const u8 *addr,
				  u16 reason_code, int locally_generated,
				  const u8 *ie, size_t ie_len, int deauth)
{
#if 1 /* FC9000 Only ?? */
	struct wpa_ssid *ssid = NULL;
#endif /* FC9000 Only ?? */

#ifdef CONFIG_AP
	if (wpa_s->ap_iface && addr) {
		hostapd_notif_disassoc(wpa_s->ap_iface->bss[0], addr);
		return;
	}

	if (wpa_s->ap_iface) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignore deauth event in AP mode");
		return;
	}
#endif /* CONFIG_AP */

	if (!locally_generated)
		wpa_s->own_disconnect_req = 0;

	wpa_supplicant_event_disassoc(wpa_s, reason_code, locally_generated);

#ifdef	CONFIG_EAPOL
	if (((reason_code == WLAN_REASON_IEEE_802_1X_AUTH_FAILED ||
	      ((wpa_key_mgmt_wpa_ieee8021x(wpa_s->key_mgmt) ||
		(wpa_s->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA)) &&
	       eapol_sm_failed(wpa_s->eapol))) &&
	     !wpa_s->eap_expected_failure))
		wpas_auth_failed(wpa_s, "AUTH_FAILED");
#endif	/* CONFIG_EAPOL */

#ifdef CONFIG_P2P
	if (deauth && reason_code > 0 &&
	    os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
		if (wpas_p2p_deauth_notif(wpa_s, addr, reason_code, ie, ie_len,
					  locally_generated) > 0) {
			/*
			 * The interface was removed, so cannot continue
			 * processing any additional operations after this.
			 */
			return;
		}
	}
#endif /* CONFIG_P2P */

#if 1 /* FC9000 Only ?? */
	ssid = wpa_s->current_ssid;
	if (ssid != NULL)
		ssid->auth_alg = 0;
#endif /* FC9000 Only ?? */
	wpa_supplicant_event_disassoc_finish(wpa_s, reason_code,
					     locally_generated);
}


static void wpas_event_disassoc(struct wpa_supplicant *wpa_s,
				struct disassoc_info *info)
{
	u16 reason_code = 0;
	int locally_generated = 0;
	const u8 *addr = NULL;
	const u8 *ie = NULL;
	size_t ie_len = 0;

	wpa_dbg(wpa_s, MSG_DEBUG, "Disassociation notification");

	if (info) {
		addr = info->addr;
		ie = info->ie;
		ie_len = info->ie_len;
		reason_code = info->reason_code;
		locally_generated = info->locally_generated;
		wpa_dbg(wpa_s, MSG_DEBUG, " * reason %u%s", reason_code,
			locally_generated ? " (locally generated)" : "");
		if (addr)
			wpa_dbg(wpa_s, MSG_DEBUG, " * address " MACSTR,
				MAC2STR(addr));
		wpa_hexdump(MSG_DEBUG, "Disassociation frame IE(s)",
			    ie, ie_len);
	}

#ifdef CONFIG_AP
	if (wpa_s->ap_iface && info && info->addr) {
		hostapd_notif_disassoc(wpa_s->ap_iface->bss[0], info->addr);
		return;
	}

	if (wpa_s->ap_iface) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Ignore disassoc event in AP mode");
		return;
	}
#endif /* CONFIG_AP */

#ifdef CONFIG_P2P
	if (info && os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
		wpas_p2p_disassoc_notif(
			wpa_s, info->addr, reason_code, info->ie, info->ie_len,
			locally_generated);
	}
#endif /* CONFIG_P2P */

	if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME)
		sme_event_disassoc(wpa_s, info);

	wpas_event_disconnect(wpa_s, addr, reason_code, locally_generated,
			      ie, ie_len, 0);
}


static void wpas_event_deauth(struct wpa_supplicant *wpa_s,
			      struct deauth_info *info)
{
	u16 reason_code = 0;
	int locally_generated = 0;
	const u8 *addr = NULL;
	const u8 *ie = NULL;
	size_t ie_len = 0;

#if 1	/* by Shingu 20190813 (P2P) */
	if ((os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0 && wpa_s->key_mgmt == 0) ||
	    wpa_s->key_mgmt == WPA_KEY_MGMT_WPS) {
		da16x_ev_prt("[%s] Ignore deauth event in WPS mode\n", __func__);
		return;
	}
#endif /* 1 */

	wpa_dbg(wpa_s, MSG_DEBUG, "Deauthentication notification");

	if (info) {
		addr = info->addr;
		ie = info->ie;
		ie_len = info->ie_len;
		reason_code = info->reason_code;
		locally_generated = info->locally_generated;
		wpa_dbg(wpa_s, MSG_DEBUG, " * reason %u%s",
			reason_code,
			locally_generated ? " (locally generated)" : "");
		if (addr) {
			wpa_dbg(wpa_s, MSG_DEBUG, " * address " MACSTR,
				MAC2STR(addr));
		}
		wpa_hexdump(MSG_DEBUG, "Deauthentication frame IE(s)",
			    ie, ie_len);
	}

	wpa_reset_ft_completed(wpa_s->wpa);

	wpas_event_disconnect(wpa_s, addr, reason_code,
			      locally_generated, ie, ie_len, 1);
}


#if defined CONFIG_IEEE80211H || defined CONFIG_5G_SUPPORT
static const char * reg_init_str(enum reg_change_initiator init)
{
	switch (init) {
	case REGDOM_SET_BY_CORE:
		return "CORE";
	case REGDOM_SET_BY_USER:
		return "USER";
	case REGDOM_SET_BY_DRIVER:
		return "DRIVER";
	case REGDOM_SET_BY_COUNTRY_IE:
		return "COUNTRY_IE";
	case REGDOM_BEACON_HINT:
		return "BEACON_HINT";
	}
	return "?";
}


static const char * reg_type_str(enum reg_type type)
{
	switch (type) {
	case REGDOM_TYPE_UNKNOWN:
		return "UNKNOWN";
	case REGDOM_TYPE_COUNTRY:
		return "COUNTRY";
	case REGDOM_TYPE_WORLD:
		return "WORLD";
	case REGDOM_TYPE_CUSTOM_WORLD:
		return "CUSTOM_WORLD";
	case REGDOM_TYPE_INTERSECTION:
		return "INTERSECTION";
	}
	return "?";
}
#endif /* CONFIG_IEEE80211H */


#if defined CONFIG_IEEE80211H || defined CONFIG_5G_SUPPORT
static void wpa_supplicant_update_channel_list(
	struct wpa_supplicant *wpa_s, struct channel_list_changed *info)
{
	struct wpa_supplicant *ifs;
	u8 dfs_domain;

#ifdef	CONFIG_AP_WIFI_MODE
	/* In case of P2P Only, AP Ony mode, setting for iface 1 is not read. */
	if (da16x_run_mode != STA_ONLY_MODE &&
		(os_strcmp(wpa_s->ifname, SOFTAP_DEVICE_NAME) == 0 ||
		 os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0)) {

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


#if 0 //[[ trinity_0171214_BEGIN -- test
	/*
	 * To allow backwards compatibility with higher level layers that
	 * assumed the REGDOM_CHANGE event is sent over the initially added
	 * interface. Find the highest parent of this interface and use it to
	 * send the event.
	 */
	for (ifs = wpa_s; ifs->parent && ifs != ifs->parent; ifs = ifs->parent)
		;
#endif //]] trinity_0171214_END

	wpa_msg(ifs, MSG_INFO, WPA_EVENT_REGDOM_CHANGE "init=%s type=%s%s%s",
		reg_init_str(info->initiator), reg_type_str(info->type),
		info->alpha2[0] ? " alpha2=" : "",
		info->alpha2[0] ? info->alpha2 : "");

	if (wpa_s->drv_priv == NULL)
		return; /* Ignore event during drv initialization */

#if 0 //[[ trinity_0171214_BEGIN -- test

	dl_list_for_each(ifs, &wpa_s->radio->ifaces, struct wpa_supplicant,
			 radio_list) {
		wpa_printf_dbg(MSG_DEBUG, "%s: Updating hw mode",
			   ifs->ifname);
		free_hw_features(ifs);
		ifs->hw.modes = wpa_drv_get_hw_feature_data(
			ifs, &ifs->hw.num_modes, &ifs->hw.flags, &dfs_domain);

		/* Restart PNO/sched_scan with updated channel list */
		if (ifs->pno) {
			wpas_stop_pno(ifs);
			wpas_start_pno(ifs);
		} else if (ifs->sched_scanning && !ifs->pno_sched_pending) {
			wpa_dbg(ifs, MSG_DEBUG,
				"Channel list changed - restart sched_scan");
			wpas_scan_restart_sched_scan(ifs);
		}
	}
#else
		da16x_notice_prt("[%s] %s: Updating hw mode\n",
			  __func__,  wpa_s->ifname);
		free_hw_features(wpa_s);
		wpa_s->hw.modes = fc80211_get_hw_feature_data(wifi_mode,
					&wpa_s->hw.num_modes, &wpa_s->hw.flags, (u16)set_greenfield);
#endif //]] trinity_0171214_END

#if 1 //[[ trinity_0171214_BEGIN -- test
	wpas_p2p_update_channel_list(wpa_s, WPAS_P2P_CHANNEL_UPDATE_DRIVER);
#endif //]] trinity_0171214_END
}
#endif /* CONFIG_IEEE80211H */


static void wpas_event_rx_mgmt_action(struct wpa_supplicant *wpa_s,
				      const u8 *frame, size_t len, int freq,
				      int rssi)
{
	const struct _ieee80211_mgmt *mgmt;
	size_t plen;
	u8 category;
	const u8 *payload;

	if (len < IEEE80211_HDRLEN + 2)
		return;

	mgmt = (const struct _ieee80211_mgmt *) frame;
	payload = frame + IEEE80211_HDRLEN;
	category = *payload++;
	plen = len - IEEE80211_HDRLEN - 1;

	wpa_dbg(wpa_s, MSG_DEBUG, "Received Action frame: SA=" MACSTR
		" Category=%u DataLen=%d freq=%d MHz",
		MAC2STR(mgmt->sa), category, (int) plen, freq);

#ifdef CONFIG_WMM_ACTIONS
	if (category == WLAN_ACTION_WMM) {
		wmm_ac_rx_action(wpa_s, mgmt->da, mgmt->sa, payload, plen);
		return;
	}
#endif /* CONFIG_WMM_ACTIONS */

#ifdef CONFIG_IEEE80211R
	if (category == WLAN_ACTION_FT) {
		ft_rx_action(wpa_s, payload, plen);
		return;
	}
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211W
	if (category == WLAN_ACTION_SA_QUERY) {
		da16x_sme_sa_query_rx(wpa_s, mgmt->sa, payload, plen);
		return;
	}
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_ACTIONS
	if (mgmt->u.action.category == WLAN_ACTION_WNM) {
		ieee802_11_rx_wnm_action(wpa_s, mgmt, len);
		return;
	}
#endif /* CONFIG_WNM_ACTIONS */
#endif /* CONFIG_WNM */

#ifdef CONFIG_GAS
	if ((mgmt->u.action.category == WLAN_ACTION_PUBLIC ||
	     mgmt->u.action.category == WLAN_ACTION_PROTECTED_DUAL) &&
	    gas_query_rx(wpa_s->gas, mgmt->da, mgmt->sa, mgmt->bssid,
			 mgmt->u.action.category,
			 payload, plen, freq) == 0)
		return;
#endif /* CONFIG_GAS */

#ifdef CONFIG_GAS_SERVER
	if ((mgmt->u.action.category == WLAN_ACTION_PUBLIC ||
	     mgmt->u.action.category == WLAN_ACTION_PROTECTED_DUAL) &&
	    gas_server_rx(wpa_s->gas_server, mgmt->da, mgmt->sa, mgmt->bssid,
			  mgmt->u.action.category,
			  payload, plen, freq) == 0)
		return;
#endif /* CONFIG_GAS_SERVER */

#ifdef CONFIG_TDLS
	if (category == WLAN_ACTION_PUBLIC && plen >= 4 &&
	    payload[0] == WLAN_TDLS_DISCOVERY_RESPONSE) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"TDLS: Received Discovery Response from " MACSTR,
			MAC2STR(mgmt->sa));
		return;
	}
#endif /* CONFIG_TDLS */

#ifdef CONFIG_INTERWORKING
	if (category == WLAN_ACTION_QOS && plen >= 1 &&
	    payload[0] == QOS_QOS_MAP_CONFIG) {
		const u8 *pos = payload + 1;
		size_t qlen = plen - 1;
		wpa_dbg(wpa_s, MSG_DEBUG, "Interworking: Received QoS Map Configure frame from "
			MACSTR, MAC2STR(mgmt->sa));
		if (os_memcmp(mgmt->sa, wpa_s->bssid, ETH_ALEN) == 0 &&
		    qlen > 2 && pos[0] == WLAN_EID_QOS_MAP_SET &&
		    pos[1] <= qlen - 2 && pos[1] >= 16)
			wpas_qos_map_set(wpa_s, pos + 2, pos[1]);
		return;
	}
#endif /* CONFIG_INTERWORKING */

#ifdef CONFIG_RRM
	if (category == WLAN_ACTION_RADIO_MEASUREMENT &&
	    payload[0] == WLAN_RRM_RADIO_MEASUREMENT_REQUEST) {
		wpas_rrm_handle_radio_measurement_request(wpa_s, mgmt->sa,
							  mgmt->da,
							  payload + 1,
							  plen - 1);
		return;
	}

	if (category == WLAN_ACTION_RADIO_MEASUREMENT &&
	    payload[0] == WLAN_RRM_NEIGHBOR_REPORT_RESPONSE) {
		wpas_rrm_process_neighbor_rep(wpa_s, payload + 1, plen - 1);
		return;
	}

	if (category == WLAN_ACTION_RADIO_MEASUREMENT &&
	    payload[0] == WLAN_RRM_LINK_MEASUREMENT_REQUEST) {
		wpas_rrm_handle_link_measurement_request(wpa_s, mgmt->sa,
							 payload + 1, plen - 1,
							 rssi);
		return;
	}
#endif /* CONFIG_RRM */

#ifdef CONFIG_FST
	if (mgmt->u.action.category == WLAN_ACTION_FST && wpa_s->fst) {
		fst_rx_action(wpa_s->fst, mgmt, len);
		return;
	}
#endif /* CONFIG_FST */

#ifdef CONFIG_DPP
	if (category == WLAN_ACTION_PUBLIC && plen >= 5 &&
	    payload[0] == WLAN_PA_VENDOR_SPECIFIC &&
	    WPA_GET_BE24(&payload[1]) == OUI_WFA &&
	    payload[4] == DPP_OUI_TYPE) {
		payload++;
		plen--;
		wpas_dpp_rx_action(wpa_s, mgmt->sa, payload, plen, freq);
		return;
	}
#endif /* CONFIG_DPP */

#ifdef CONFIG_P2P
	wpas_p2p_rx_action(wpa_s, mgmt->da, mgmt->sa, mgmt->bssid,
			   category, payload, plen, freq);
#endif /* CONFIG_P2P */

#ifdef CONFIG_MESH
	if (wpa_s->ifmsh)
		mesh_mpm_action_rx(wpa_s, mgmt, len);
#endif /* CONFIG_MESH */

/* For removing compile warning */
	if (mgmt) ;
	if (plen) ;
	if (category) ;
}


#ifdef CONFIG_SUPP27_EVENTS
static void wpa_supplicant_notify_avoid_freq(struct wpa_supplicant *wpa_s,
					     union wpa_event_data *event)
{
	struct wpa_freq_range_list *list;
	char *str = NULL;

	list = &event->freq_range;

	if (list->num)
		str = freq_range_list_str(list);
	wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_AVOID_FREQ "ranges=%s",
		str ? str : "");

#ifdef CONFIG_P2P_UNUSED_CMD
	if (freq_range_list_parse(&wpa_s->global->p2p_go_avoid_freq, str)) {
		wpa_dbg(wpa_s, MSG_ERROR, "%s: Failed to parse freq range",
			__func__);
	} else {
		wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Update channel list based on frequency avoid event");

		/*
		 * The update channel flow will also take care of moving a GO
		 * from the unsafe frequency if needed.
		 */
		wpas_p2p_update_channel_list(wpa_s,
					     WPAS_P2P_CHANNEL_UPDATE_AVOID);
	}
#endif /* CONFIG_P2P_UNUSED_CMD */

	os_free(str);
}
#endif /* CONFIG_SUPP27_EVENTS */


static void wpa_supplicant_event_port_authorized(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->wpa_state == WPA_ASSOCIATED) {
		wpa_supplicant_cancel_auth_timeout(wpa_s);
		wpa_supplicant_set_state(wpa_s, WPA_COMPLETED);
		eapol_sm_notify_portValid(wpa_s->eapol, TRUE);
		eapol_sm_notify_eap_success(wpa_s->eapol, TRUE);
	}
}


#ifdef CONFIG_IEEE80211H
static unsigned int wpas_event_cac_ms(const struct wpa_supplicant *wpa_s,
				      int freq)
{
	size_t i;
	int j;

	for (i = 0; i < wpa_s->hw.num_modes; i++) {
		const struct hostapd_hw_modes *mode = &wpa_s->hw.modes[i];

		for (j = 0; j < mode->num_channels; j++) {
			const struct hostapd_channel_data *chan;

			chan = &mode->channels[j];
			if (chan->freq == freq)
				return chan->dfs_cac_ms;
		}
	}

	return 0;
}


static void wpas_event_dfs_cac_started(struct wpa_supplicant *wpa_s,
				       struct dfs_event *radar)
{
#if defined(NEED_AP_MLME) && defined(CONFIG_AP)
	if (wpa_s->ap_iface) {
		wpas_ap_event_dfs_cac_started(wpa_s, radar);
	} else
#endif /* NEED_AP_MLME && CONFIG_AP */
	{
		unsigned int cac_time = wpas_event_cac_ms(wpa_s, radar->freq);

		cac_time /= 1000; /* convert from ms to sec */
		if (!cac_time)
			cac_time = 10 * 60; /* max timeout: 10 minutes */

		/* Restart auth timeout: CAC time added to initial timeout */
		wpas_auth_timeout_restart(wpa_s, cac_time);
	}
}


static void wpas_event_dfs_cac_finished(struct wpa_supplicant *wpa_s,
					struct dfs_event *radar)
{
#if defined(NEED_AP_MLME) && defined(CONFIG_AP)
	if (wpa_s->ap_iface) {
		wpas_ap_event_dfs_cac_finished(wpa_s, radar);
	} else
#endif /* NEED_AP_MLME && CONFIG_AP */
	{
		/* Restart auth timeout with original value after CAC is
		 * finished */
		wpas_auth_timeout_restart(wpa_s, 0);
	}
}


static void wpas_event_dfs_cac_aborted(struct wpa_supplicant *wpa_s,
				       struct dfs_event *radar)
{
#if defined(NEED_AP_MLME) && defined(CONFIG_AP)
	if (wpa_s->ap_iface) {
		wpas_ap_event_dfs_cac_aborted(wpa_s, radar);
	} else
#endif /* NEED_AP_MLME && CONFIG_AP */
	{
		/* Restart auth timeout with original value after CAC is
		 * aborted */
		wpas_auth_timeout_restart(wpa_s, 0);
	}
}
#endif /* CONFIG_IEEE80211H */


#ifdef CONFIG_FILS
static void wpa_supplicant_event_assoc_auth(struct wpa_supplicant *wpa_s,
					    union wpa_event_data *data)
{
	wpa_dbg(wpa_s, MSG_DEBUG,
		"Connection authorized by device, previous state %d",
		wpa_s->wpa_state);

	wpa_supplicant_event_port_authorized(wpa_s);

	wpa_sm_set_rx_replay_ctr(wpa_s->wpa, data->assoc_info.key_replay_ctr);
	wpa_sm_set_ptk_kck_kek(wpa_s->wpa, data->assoc_info.ptk_kck,
			       data->assoc_info.ptk_kck_len,
			       data->assoc_info.ptk_kek,
			       data->assoc_info.ptk_kek_len);
#ifdef CONFIG_FILS
	if (wpa_s->auth_alg == WPA_AUTH_ALG_FILS) {
		struct wpa_bss *bss = wpa_bss_get_bssid(wpa_s, wpa_s->bssid);
		const u8 *fils_cache_id = wpa_bss_get_fils_cache_id(bss);

		/* Update ERP next sequence number */
		eapol_sm_update_erp_next_seq_num(
			wpa_s->eapol, data->assoc_info.fils_erp_next_seq_num);

		if (data->assoc_info.fils_pmk && data->assoc_info.fils_pmkid) {
			/* Add the new PMK and PMKID to the PMKSA cache */
			wpa_sm_pmksa_cache_add(wpa_s->wpa,
					       data->assoc_info.fils_pmk,
					       data->assoc_info.fils_pmk_len,
					       data->assoc_info.fils_pmkid,
					       wpa_s->bssid, fils_cache_id);
		} else if (data->assoc_info.fils_pmkid) {
			/* Update the current PMKSA used for this connection */
			pmksa_cache_set_current(wpa_s->wpa,
						data->assoc_info.fils_pmkid,
						NULL, NULL, 0, NULL, 0);
		}
	}
#endif /* CONFIG_FILS */
}
#endif /* CONFIG_FILS */


static void wpas_event_assoc_reject(struct wpa_supplicant *wpa_s,
				    union wpa_event_data *data)
{
	const u8 *bssid = data->assoc_reject.bssid;
#if 1 /* by Shingu 20200513 */
	int is_wifi_mode_changed = 0;
#endif

	if (!bssid || is_zero_ether_addr(bssid))
		bssid = wpa_s->pending_bssid;

#ifdef	CONFIG_DPM_OPT_WIFI_MODE /* by Shingu 20180207 (DPM Optimize) */
	if (wpa_s->current_ssid->dpm_opt_wifi_mode &&
	    (data->assoc_reject.status_code == 0x12 || data->assoc_reject.status_code == 0x1b)) {
		if (wpa_s->current_ssid->wifi_mode == DA16X_WIFI_MODE_BG) {
			da16x_notice_prt(">>> Wi-Fi mode : b/g -> b/g/n\n");
			wpa_s->current_ssid->wifi_mode = DA16X_WIFI_MODE_BGN;
		} else if (wpa_s->current_ssid->wifi_mode == DA16X_WIFI_MODE_G_ONLY) {
			da16x_notice_prt(">>> Wi-Fi mode : g-only -> g/n\n");
			wpa_s->current_ssid->wifi_mode = DA16X_WIFI_MODE_GN;
		}
		wpa_s->current_ssid->disable_ht = 0;
		wpa_s->current_ssid->ht = 1;
		wpa_s->current_ssid->dpm_opt_wifi_mode = 0;
#if 1 /* by Shingu 20200513 */
		is_wifi_mode_changed = 1;
#endif
	}
#endif	/* CONFIG_DPM_OPT_WIFI_MODE */

	if (data->assoc_reject.bssid) {
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_ASSOC_REJECT
			"bssid=" MACSTR	" status_code=%u%s%s%s",
			MAC2STR(data->assoc_reject.bssid),
			data->assoc_reject.status_code,
			data->assoc_reject.timed_out ? " timeout" : "",
			data->assoc_reject.timeout_reason ? "=" : "",
			data->assoc_reject.timeout_reason ?
			data->assoc_reject.timeout_reason : "");
	} else {
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_ASSOC_REJECT
			"status_code=%u%s%s%s",
			data->assoc_reject.status_code,
			data->assoc_reject.timed_out ? " timeout" : "",
			data->assoc_reject.timeout_reason ? "=" : "",
			data->assoc_reject.timeout_reason ?
			data->assoc_reject.timeout_reason : "");
	}

	wpa_s->assoc_status_code = data->assoc_reject.status_code;

	//wpas_notify_assoc_status_code(wpa_s);

#ifdef CONFIG_OWE
	if (data->assoc_reject.status_code ==
	    WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED &&
	    wpa_s->key_mgmt == WPA_KEY_MGMT_OWE &&
	    wpa_s->current_ssid &&
	    wpa_s->current_ssid->owe_group == 0 &&
	    wpa_s->last_owe_group != 21) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		struct wpa_bss *bss = wpa_s->current_bss;

#if 0
		if (!bss) {
			bss = (struct wpa_bss*)wpa_supplicant_get_new_bss(wpa_s, bssid);
			if (!bss) {
				wpas_connection_failed(wpa_s, bssid);
				wpa_supplicant_mark_disassoc(wpa_s);
				return;
			}
		}
#endif /* 0 */
		wpa_printf_dbg(MSG_DEBUG, "OWE: Try next supported DH group");
		wpas_connect_work_done(wpa_s);
		wpa_supplicant_mark_disassoc(wpa_s);
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
		if(wpa_s->current_bss == NULL)
			bss = NULL;
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
		wpa_supplicant_connect(wpa_s, bss, ssid);
		return;
	}
#endif /* CONFIG_OWE */

#if 0 /* by Shingu 20200513 */
	if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) {
		sme_event_assoc_reject(wpa_s, data);
		return;
	}
#else
	if (is_wifi_mode_changed) {
		// Skip SSID reconfig by SME	
	} else if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) {
		sme_event_assoc_reject(wpa_s, data);
		return;
	}
#endif

	/* Driver-based SME cases */

#ifdef CONFIG_SAE
	if (wpa_s->current_ssid &&
	    wpa_key_mgmt_sae(wpa_s->current_ssid->key_mgmt) &&
	    !data->assoc_reject.timed_out) {
		wpa_dbg(wpa_s, MSG_DEBUG, "SAE: Drop PMKSA cache entry");
		wpa_sm_aborted_cached(wpa_s->wpa);
		wpa_sm_pmksa_cache_flush(wpa_s->wpa, wpa_s->current_ssid);
	}
#endif /* CONFIG_SAE */

#ifdef CONFIG_FILS
	/* Update ERP next sequence number */
	if (wpa_s->auth_alg == WPA_AUTH_ALG_FILS)
		eapol_sm_update_erp_next_seq_num(
			wpa_s->eapol,
			data->assoc_reject.fils_erp_next_seq_num);
#endif /* CONFIG_FILS */

	wpas_connection_failed(wpa_s, bssid);
	wpa_supplicant_mark_disassoc(wpa_s);
}


const char* wlan_fc_type_2String (int type)
{
	switch (type)
	{
		case WLAN_FC_TYPE_MGMT:
			return "MGMT";

		case WLAN_FC_TYPE_CTRL:
			return "CTRL";

		case WLAN_FC_TYPE_DATA:
			return "DATA";

		default:
			return "UNKNOWN_TYPE";
	}
}


const char* wlan_fc_stype2String (int type, int stype)
{
	switch (type)
	{
		case WLAN_FC_TYPE_MGMT:
		{
			switch (stype)
			{
				/* management */
				case WLAN_FC_STYPE_ASSOC_REQ:
					return "ASSOC_REQ";

				case WLAN_FC_STYPE_ASSOC_RESP:
					return "ASSOC_RESP";

				case WLAN_FC_STYPE_REASSOC_REQ:
					return "REASSOC_REQ";

				case WLAN_FC_STYPE_REASSOC_RESP:
					return "REASSOC_RESP";

				case WLAN_FC_STYPE_PROBE_REQ:
					return "PROBE_REQ";

				case WLAN_FC_STYPE_PROBE_RESP:
					return "PROBE_RESP";

				case WLAN_FC_STYPE_BEACON:
					return "BEACON";

				case WLAN_FC_STYPE_ATIM:
					return "ATIM";

				case WLAN_FC_STYPE_DISASSOC:
					return "DISASSOC";

				case WLAN_FC_STYPE_AUTH:
					return "AUTH";

				case WLAN_FC_STYPE_DEAUTH:
					return "DEAUTH";

				case WLAN_FC_STYPE_ACTION:
					return "ACTION";
			}
			break;
		}

		case WLAN_FC_TYPE_CTRL:
		{
			switch (stype)
			{
				/* ctrl */
				case WLAN_FC_STYPE_PSPOLL:
					return "PSPOLL";

				case WLAN_FC_STYPE_RTS:
					return "RTS";

				case WLAN_FC_STYPE_CTS:
					return "CTS";

				case WLAN_FC_STYPE_ACK:
					return "ACK";

				case WLAN_FC_STYPE_CFEND:
					return "CFEND";

				case WLAN_FC_STYPE_CFENDACK:
					return "CFENDACK";
			}
			break;
		}

		case WLAN_FC_TYPE_DATA:
		{
			switch (stype)
			{
				/* data */
				case WLAN_FC_STYPE_DATA:
					return "DATA";

				case WLAN_FC_STYPE_DATA_CFACK:
					return "DATA_CFACK";

				case WLAN_FC_STYPE_DATA_CFPOLL:
					return "DATA_CFPOLL";

				case WLAN_FC_STYPE_DATA_CFACKPOLL:
					return "DATA_CFACKPOLL";

				case WLAN_FC_STYPE_NULLFUNC:
					return "NULLFUNC";

				case WLAN_FC_STYPE_CFACK:
					return "CFACK";

				case WLAN_FC_STYPE_CFPOLL:
					return "CFPOLL";

				case WLAN_FC_STYPE_CFACKPOLL:
					return "CFACKPOLL";

				case WLAN_FC_STYPE_QOS_DATA:
					return "QOS_DATA";

				case WLAN_FC_STYPE_QOS_DATA_CFACK:
					return "QOS_DATA_CFACK";

				case WLAN_FC_STYPE_QOS_DATA_CFPOLL:
					return "DATA_CFPOLL";

				case WLAN_FC_STYPE_QOS_DATA_CFACKPOLL:
					return "DATA_CFACKPOLL";

				case WLAN_FC_STYPE_QOS_NULL:
					return "QOS_NULL";

				case WLAN_FC_STYPE_QOS_CFPOLL:
					return "QOS_CFPOLL";

				case WLAN_FC_STYPE_QOS_CFACKPOLL:
					return "QOS_CFACKPOLL";
			}
			break;
		}
	}
	return	"UNKNOWN_STYPE";
}

void wpa_supplicant_event(void *ctx, enum wpa_event_type event,
			  union wpa_event_data *data)
{
	struct wpa_supplicant *wpa_s = ctx;
#if defined ( CONFIG_SCHED_SCAN )
	int resched;
#endif // CONFIG_SCHED_SCAN

#ifdef	CONFIG_SUPP27_EVENTS
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED &&
	    event != EVENT_INTERFACE_ENABLED &&
	    event != EVENT_INTERFACE_STATUS &&
	    event != EVENT_SCAN_RESULTS &&
	    event != EVENT_SCHED_SCAN_STOPPED) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Ignore event %s (%d) while interface is disabled",
			event_to_string(event), event);
		return;
	}
#endif	/* CONFIG_SUPP27_EVENTS */

#ifndef CONFIG_NO_STDOUT_DEBUG
	{
#ifdef	CONFIG_WPA_DBG
		int level = MSG_DEBUG;

		if (event == EVENT_RX_MGMT && data->rx_mgmt.frame_len >= 24) {
			const struct _ieee80211_hdr *hdr;
			u16 fc;
			hdr = (const struct _ieee80211_hdr *) data->rx_mgmt.frame;
			fc = le_to_host16(hdr->frame_control);
			if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
		    	WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_BEACON)
				level = MSG_EXCESSIVE;
		}

		if (event == EVENT_RX_MGMT && wpa_s->current_ssid->mode == WPAS_MODE_AP) {
			level = MSG_EXCESSIVE;
		}
#endif	/* CONFIG_WPA_DBG */

		wpa_dbg(wpa_s, level, ANSI_COLOR_GREEN "[%s] EVENT_%s (%d) received" ANSI_COLOR_DEFULT, __func__,
		event_to_string(event), event);
	}
#endif /* CONFIG_NO_STDOUT_DEBUG */

#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpa_s->conf->roam_state >= ROAM_ENABLE) {
		if ((event == EVENT_ASSOC) || (event == EVENT_DISASSOC) || (event == EVENT_DEAUTH)) {
			fc80211_set_roaming_mode(1);
			wpa_supplicant_cancel_scan(wpa_s);
			os_memset(wpa_s->conf->candi_bssid, 0, ETH_ALEN);
			wpa_s->conf->roam_cnt = ROAM_SUSPEND_CNT;
			wpa_s->conf->roam_state = ROAM_ENABLE;
		}
	}
#endif	/* CONFIG_SIMPLE_ROAMING */

	switch (event) {
		case EVENT_AUTH:
#ifdef CONFIG_FST
			if (!wpas_fst_update_mbie(wpa_s, data->auth.ies,
						  data->auth.ies_len))
				wpa_printf_dbg(MSG_DEBUG,
					   "FST: MB IEs updated from auth IE");
#endif /* CONFIG_FST */
			//da16x_ev_prt("		   <%s> EVENT_AUTH  received\n", __func__);
			da16x_sme_event_auth(wpa_s, data);
			break;

		case EVENT_ASSOC:
#ifdef CONFIG_TESTING_OPTIONS
			if (wpa_s->ignore_auth_resp) {
				wpa_printf(MSG_INFO,
					   "EVENT_ASSOC - ignore_auth_resp active!");
				break;
			}

			if (wpa_s->testing_resend_assoc) {
				wpa_printf(MSG_INFO,
					   "EVENT_DEAUTH - testing_resend_assoc");
				break;
			}
#endif /* CONFIG_TESTING_OPTIONS */
			wpa_supplicant_event_assoc(wpa_s, data);

#ifdef CONFIG_FILS
			if (data &&
			    (data->assoc_info.authorized
				|| (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
			      wpa_fils_is_completed(wpa_s->wpa))
				))
				wpa_supplicant_event_assoc_auth(wpa_s, data);
			if (data) {
				wpa_msg(wpa_s, MSG_INFO,
					WPA_EVENT_SUBNET_STATUS_UPDATE "status=%u",
					data->assoc_info.subnet_status);
			}
#endif /* CONFIG_FILS */
			break;

		case EVENT_DISASSOC:
			da16x_ev_prt("<%s> EVENT_DISASSOC  received\n", __func__);
			wpas_event_disassoc(wpa_s, data ? &data->disassoc_info : NULL);
			break;

		case EVENT_DEAUTH:
#ifdef CONFIG_TESTING_OPTIONS
			if (wpa_s->ignore_auth_resp) {
				wpa_printf(MSG_INFO,
					   "EVENT_DEAUTH - ignore_auth_resp active!");
				break;
			}
			if (wpa_s->testing_resend_assoc) {
				wpa_printf(MSG_INFO,
					   "EVENT_DEAUTH - testing_resend_assoc");
				break;
			}
#endif /* CONFIG_TESTING_OPTIONS */
			wpas_event_deauth(wpa_s, data ? &data->deauth_info : NULL);
			break;

#ifdef	CONFIG_MIC_FAILURE_RSP
		case EVENT_MICHAEL_MIC_FAILURE:
#ifdef CONFIG_AP
			if(wpa_s->ap_iface)
				michael_mic_failure(wpa_s->ap_iface->bss[0], data->michael_mic_failure.src, 1);
			else
#endif /* CONFIG_AP */
			wpa_supplicant_event_michael_mic_failure(wpa_s, data);
			break;
#endif	/* CONFIG_MIC_FAILURE_RSP */

#ifndef CONFIG_NO_SCAN_PROCESSING

#if defined ( __SUPP_27_SUPPORT__ )
		case EVENT_SCAN_STARTED:
			da16x_scan_prt("<%s> EVENT_SCAN_STARTED\n", __func__);
			if (wpa_s->own_scan_requested ||
			    (data && !data->scan_info.external_scan)) {
				struct os_reltime diff;

				os_get_reltime(&wpa_s->scan_start_time);
				os_reltime_sub(&wpa_s->scan_start_time,
					       &wpa_s->scan_trigger_time, &diff);
				wpa_dbg(wpa_s, MSG_DEBUG, "Own scan request started a scan in %ld.%06ld seconds",
					diff.sec, diff.usec);
				wpa_s->own_scan_requested = 0;
				wpa_s->own_scan_running = 1;
				if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
				    wpa_s->manual_scan_use_id) {
					wpa_msg_ctrl(wpa_s, MSG_INFO,
						     WPA_EVENT_SCAN_STARTED "id=%u",
						     wpa_s->manual_scan_id);
				} else {
					wpa_msg_ctrl(wpa_s, MSG_INFO,
						     WPA_EVENT_SCAN_STARTED);
				}
			} else {
				wpa_dbg(wpa_s, MSG_DEBUG, "External program started a scan");
#if defined ( __SUPP_27_SUPPORT__ )
				wpa_s->radio->external_scan_running = 1;
#endif // __SUPP_27_SUPPORT__
				wpa_msg_ctrl(wpa_s, MSG_INFO, WPA_EVENT_SCAN_STARTED);
			}
			break;

		case EVENT_SCAN_RESULTS:
			if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
				wpa_s->scan_res_handler = NULL;
				wpa_s->own_scan_running = 0;
#if defined ( __SUPP_27_SUPPORT__ )
				wpa_s->radio->external_scan_running = 0;
#endif // __SUPP_27_SUPPORT__
				wpa_s->last_scan_req = NORMAL_SCAN_REQ;
				break;
			}

			if (!(data && data->scan_info.external_scan) &&
			    os_reltime_initialized(&wpa_s->scan_start_time)) {
				struct os_reltime now, diff;
				os_get_reltime(&now);
				os_reltime_sub(&now, &wpa_s->scan_start_time, &diff);
				wpa_s->scan_start_time.sec = 0;
				wpa_s->scan_start_time.usec = 0;
				wpa_dbg(wpa_s, MSG_DEBUG, "Scan completed in %ld.%06ld seconds",
					diff.sec, diff.usec);
			}

			if (wpa_supp_ev_scan_results(wpa_s, data))
				break; /* interface may have been removed */

			if (!(data && data->scan_info.external_scan))
				wpa_s->own_scan_running = 0;
#ifdef	__SUPP_27_SUPPORT__
			if (data && data->scan_info.nl_scan_event)
				wpa_s->radio->external_scan_running = 0;
			radio_work_check_next(wpa_s);
#endif /* __SUPP_27_SUPPORT__ */
			break;
#else
		case EVENT_SCAN_STARTED:
			if (wpa_s->own_scan_requested) {
				wpa_s->own_scan_requested = 0;
				wpa_s->own_scan_running = 1;
#ifdef	CONFIG_RADIO_WORK
			} else {
				wpa_s->radio->external_scan_running = 1;
#endif /* CONFIG_RADIO_WORK */
			}
			break;

		case EVENT_SCAN_RESULTS:	// 3
			da16x_scan_prt("<%s> EVENT_SCAN_RESULTS\n", __func__);

			/* Delete instance scan_results timer */
			wpa_supp_ev_scan_results(wpa_s, data);

			if( wpa_s->manual_scan_passive ) {
				vTaskDelay(30);
				wpa_passive_scan_done();
				wpa_s->manual_scan_passive = 0;
				wpa_s->passive_scan_duration = 0;
			}
			wpa_s->own_scan_running = 0;
#ifdef	CONFIG_RADIO_WORK
			wpa_s->radio->external_scan_running = 0;
#endif /* CONFIG_RADIO_WORK */
			break;
#endif	/* __SUPP_27_SUPPORT__ */

#endif /* CONFIG_NO_SCAN_PROCESSING */
		case EVENT_ASSOCINFO:
			wpa_supplicant_event_associnfo(wpa_s, data);
			break;
		case EVENT_INTERFACE_STATUS:
			wpa_supplicant_event_interface_status(wpa_s, data);
			break;

#ifdef	CONFIG_PRE_AUTH	/* by Bright : For WPA3 : Don't need control-interface on FC9000's supplicant */
		case EVENT_PMKID_CANDIDATE:
			wpa_supplicant_event_pmkid_candidate(wpa_s, data);
			break;
#endif	/* CONFIG_PRE_AUTH */
#ifdef CONFIG_TDLS
		case EVENT_TDLS:
			wpa_supplicant_event_tdls(wpa_s, data);
			break;
#endif /* CONFIG_TDLS */
#ifdef CONFIG_WNM
#ifdef CONFIG_WNM_SLEEP_MODE
		case EVENT_WNM:
			wpa_supplicant_event_wnm(wpa_s, data);
			break;
#endif /* CONFIG_WNM_SLEEP_MODE */
#endif /* CONFIG_WNM */
#ifdef CONFIG_IEEE80211R
		case EVENT_FT_RESPONSE:
			wpa_supplicant_event_ft_response(wpa_s, data);
			break;
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IBSS_RSN
		case EVENT_IBSS_RSN_START:
			wpa_supplicant_event_ibss_rsn_start(wpa_s, data);
			break;
#endif /* CONFIG_IBSS_RSN */

		case EVENT_ASSOC_REJECT:
#if 0 /* FC9000 Only */
#ifdef	CONFIG_DPM_OPT_WIFI_MODE /* by Shingu 20180207 (DPM Optimize) */
			if (wpa_s->current_ssid->dpm_opt_wifi_mode &&
			    (data->assoc_reject.status_code == 0x12 ||
			     data->assoc_reject.status_code == 0x1b)) {
				if (wpa_s->current_ssid->wifi_mode == DA16X_WIFI_MODE_BG) {
					da16x_notice_prt(">>> Wi-Fi mode : b/g -> b/g/n\n");
					wpa_s->current_ssid->wifi_mode = DA16X_WIFI_MODE_BGN;
				} else if (wpa_s->current_ssid->wifi_mode == DA16X_WIFI_MODE_G_ONLY) {
					da16x_notice_prt(">>> Wi-Fi mode : g-only -> g/n\n");
					wpa_s->current_ssid->wifi_mode = DA16X_WIFI_MODE_GN;
				}
				wpa_s->current_ssid->disable_ht = 0;
				wpa_s->current_ssid->ht = 1;
				wpa_s->current_ssid->dpm_opt_wifi_mode = 0;
				break;
			}
#endif	/* CONFIG_DPM_OPT_WIFI_MODE */
			if (data->assoc_reject.bssid) {
				da16x_notice_prt("<%s> " WPA_EVENT_ASSOC_REJECT
						"bssid=" MACSTR	" status_code=%u\n",
						__func__,
						MAC2STR(data->assoc_reject.bssid),
						data->assoc_reject.status_code);
			} else {
				da16x_notice_prt("<%s> " WPA_EVENT_ASSOC_REJECT
						"status_code=%u\n",
						__func__,
						data->assoc_reject.status_code);
			}

			{
				const u8 *bssid = data->assoc_reject.bssid;
				if (bssid == NULL || is_zero_ether_addr(bssid))
					bssid = wpa_s->pending_bssid;
				wpas_connection_failed(wpa_s, bssid);
				wpa_supplicant_mark_disassoc(wpa_s);
			}
#else
			wpas_event_assoc_reject(wpa_s, data);
#endif /* FC9000 Only */
			break;

		case EVENT_AUTH_TIMED_OUT:
			/* It is possible to get this event from earlier connection */
#ifdef	CONFIG_SIMPLE_ROAMING
			if (wpa_s->conf->roam_state >= ROAM_ENABLE) {
				PRINTF(RED_COLOR ">>> [Roaming] Failed - EVENT_AUTH_TIMED_OUT \n" CLEAR_COLOR);
			}
#endif	//CONFIG_SIMPLE_ROAMING
#ifdef CONFIG_MESH
			if (wpa_s->current_ssid &&
			    wpa_s->current_ssid->mode == WPAS_MODE_MESH) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"Ignore AUTH_TIMED_OUT in mesh configuration");
				break;
			}
#endif /* CONFIG_MESH */

			if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME)
				da16x_sme_event_auth_timed_out(wpa_s, data);
			break;

		case EVENT_ASSOC_TIMED_OUT:
			/* It is possible to get this event from earlier connection */
#ifdef	CONFIG_SIMPLE_ROAMING
			if (wpa_s->conf->roam_state >= ROAM_ENABLE) {
				PRINTF(RED_COLOR ">>> [Roaming] Failed - EVENT_ASSOC_TIMED_OUT \n" CLEAR_COLOR);
			}
#endif	//CONFIG_SIMPLE_ROAMING
#ifdef CONFIG_MESH
			if (wpa_s->current_ssid &&
			    wpa_s->current_ssid->mode == WPAS_MODE_MESH) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"Ignore ASSOC_TIMED_OUT in mesh configuration");
				break;
			}
#endif /* CONFIG_MESH */

			if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME)
				sme_event_assoc_timed_out(wpa_s, data);
			break;

#ifdef CONFIG_RECEIVE_DHCP_EVENT
		case EVENT_DHCP_NO_RESPONSE:
			if (wpa_s->wpa_state == WPA_COMPLETED && wpa_s->conf->ssid)
			{
				if (!(    wpa_s->conf->ssid->key_mgmt == WPA_KEY_MGMT_NONE
					   && wpa_s->conf->ssid->auth_alg == WPA_AUTH_ALG_OPEN)
					&& has_wep_key(wpa_s->conf->ssid)) /* WEP Auth Open */
				{
					da16x_notice_prt(YELLOW_COLOR "WEP key might be incorrect. \n" CLEAR_COLOR);
					wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_UNSPECIFIED);
	                wpa_s->disconnect_reason = WLAN_REASON_PEERKEY_MISMATCH;
				}
	            else
	            {
		            // station is connect but AP recognizes that the station is not connected, because the 4th packet in eapool does not come in 
	                da16x_notice_prt(YELLOW_COLOR "IP not assigned\n" CLEAR_COLOR);
	                wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_UNSPECIFIED);
	            }
            }
			break;

		case EVENT_DHCP_ACK_OK:
			if (wpa_s->wpa_state == WPA_COMPLETED && wpa_s->conf->ssid)
			{
				if (!(wpa_s->conf->ssid->key_mgmt == WPA_KEY_MGMT_NONE
					&& wpa_s->conf->ssid->auth_alg & WPA_AUTH_ALG_OPEN)) /* WEP Auth Open */
					{
						wpa_s->disconnect_reason = 0;
					}
			}
			break;
#endif /* CONFIG_RECEIVE_DHCP_EVENT */

		case EVENT_TX_STATUS:
			da16x_ev_prt("          <%s> EVENT_TX_STATUS dst=" MACSTR
					" type=%d stype=%d\n",
					__func__, MAC2STR(data->tx_status.dst),
					data->tx_status.type, data->tx_status.stype);
#ifdef CONFIG_AP
			if (wpa_s->ap_iface == NULL) {
				if (data->tx_status.type == WLAN_FC_TYPE_MGMT &&
				    data->tx_status.stype == WLAN_FC_STYPE_ACTION)
					offchannel_send_action_tx_status(
						wpa_s, data->tx_status.dst,
						data->tx_status.data,
						data->tx_status.data_len,
						data->tx_status.ack ?
						OFFCHANNEL_SEND_ACTION_SUCCESS :
						OFFCHANNEL_SEND_ACTION_NO_ACK);
				break;
			}
#endif /* CONFIG_AP */
			da16x_ev_prt("          <%s> EVENT_TX_STATUS pending_dst="
					MACSTR "\n",
					__func__, MAC2STR(wpa_s->pending_action_dst));
			/*
			 * Catch TX status events for Action frames we sent via group
			 * interface in GO mode, or via standalone AP interface.
			 * Note, wpa_s->p2pdev will be the same as wpa_s->parent,
			 * except when the primary interface is used as a GO interface
			 * (for drivers which do not have group interface concurrency)
			 */
			if (data->tx_status.type == WLAN_FC_TYPE_MGMT &&
			    data->tx_status.stype == WLAN_FC_STYPE_ACTION &&
			    os_memcmp(wpa_s->parent->pending_action_dst,
				      data->tx_status.dst, ETH_ALEN) == 0) {
				offchannel_send_action_tx_status(
					wpa_s->parent, data->tx_status.dst,
					data->tx_status.data,
					data->tx_status.data_len,
					data->tx_status.ack ?
					OFFCHANNEL_SEND_ACTION_SUCCESS :
					OFFCHANNEL_SEND_ACTION_NO_ACK);
				break;
			}
#ifdef CONFIG_AP
			switch (data->tx_status.type) {
				case WLAN_FC_TYPE_MGMT:
					ap_mgmt_tx_cb(wpa_s, data->tx_status.data,
						      data->tx_status.data_len,
						      data->tx_status.stype,
						      data->tx_status.ack);
					break;
#ifdef	CONFIG_SUPP27_EVENTS
				case WLAN_FC_TYPE_DATA:
					ap_tx_status(wpa_s, data->tx_status.dst,
						     data->tx_status.data,
						     data->tx_status.data_len,
						     data->tx_status.ack);
					break;
#endif	/* CONFIG_SUPP27_EVENTS */
			}
#endif /* CONFIG_AP */
			break;

#ifdef	CONFIG_SUPP27_EVENTS
#ifdef CONFIG_AP
		case EVENT_EAPOL_TX_STATUS:
			ap_eapol_tx_status(wpa_s, data->eapol_tx_status.dst,
					   data->eapol_tx_status.data,
					   data->eapol_tx_status.data_len,
					   data->eapol_tx_status.ack);
			break;

		case EVENT_DRIVER_CLIENT_POLL_OK:
			ap_client_poll_ok(wpa_s, data->client_poll.addr);
			break;

		case EVENT_RX_FROM_UNKNOWN:
			if (wpa_s->ap_iface == NULL)
				break;
			ap_rx_from_unknown_sta(wpa_s, data->rx_from_unknown.addr,
					       data->rx_from_unknown.wds);
			break;
#endif /* CONFIG_AP */
#endif	/* CONFIG_SUPP27_EVENTS */

		case EVENT_CH_SWITCH:
			if (!data || !wpa_s->current_ssid)
				break;

#if 0	/* by Bright : Merge 2.7 */
			da16x_info_prt("[%s] freq=%d ht_enabled=%d ch_offset=%d ch_width=%s cf1=%d cf2=%d",
				__func__,
				data->ch_switch.freq,
				data->ch_switch.ht_enabled,
				data->ch_switch.ch_offset,
				channel_width_to_string(data->ch_switch.ch_width),
				data->ch_switch.cf1,
				data->ch_switch.cf2);
#else
			da16x_info_prt("[%s] freq=%d ht_enabled=%d ch_offset=%d cf1=%d cf2=%d\n",
				__func__,
				data->ch_switch.freq,
				data->ch_switch.ht_enabled,
				data->ch_switch.ch_offset,
				data->ch_switch.cf1,
				data->ch_switch.cf2);
#endif	/* 0 */

			wpa_s->assoc_freq = data->ch_switch.freq;
			wpa_s->current_ssid->frequency = data->ch_switch.freq;

#ifdef CONFIG_AP
			if (wpa_s->current_ssid->mode == WPAS_MODE_AP ||
			    wpa_s->current_ssid->mode == WPAS_MODE_P2P_GO ||
			    wpa_s->current_ssid->mode ==
			    WPAS_MODE_P2P_GROUP_FORMATION) {
				wpas_ap_ch_switch(wpa_s, data->ch_switch.freq,
						  data->ch_switch.ht_enabled,
						  data->ch_switch.ch_offset,
						  data->ch_switch.ch_width,
						  data->ch_switch.cf1,
						  data->ch_switch.cf2);
			}
#endif /* CONFIG_AP */

#ifdef CONFIG_P2P_UNUSED_CMD
			wpas_p2p_update_channel_list(wpa_s, WPAS_P2P_CHANNEL_UPDATE_CS);
#endif /* CONFIG_P2P_UNUSED_CMD */
			break;


#ifdef CONFIG_IEEE80211H
#ifdef CONFIG_AP
#ifdef NEED_AP_MLME
		case EVENT_DFS_RADAR_DETECTED:
			if (data)
				wpas_ap_event_dfs_radar_detected(wpa_s,
								 &data->dfs_event);
			break;

		case EVENT_DFS_NOP_FINISHED:
			if (data)
				wpas_ap_event_dfs_cac_nop_finished(wpa_s,
								   &data->dfs_event);
			break;
#endif /* NEED_AP_MLME */
#endif /* CONFIG_AP */

		case EVENT_DFS_CAC_STARTED:
			if (data)
				wpas_event_dfs_cac_started(wpa_s, &data->dfs_event);
			break;

		case EVENT_DFS_CAC_FINISHED:
			if (data)
				wpas_event_dfs_cac_finished(wpa_s, &data->dfs_event);
			break;

		case EVENT_DFS_CAC_ABORTED:
			if (data)
				wpas_event_dfs_cac_aborted(wpa_s, &data->dfs_event);
			break;
#endif /* CONFIG_IEEE80211H */

		case EVENT_RX_MGMT: {
			u16 fc, stype;
			const struct _ieee80211_mgmt *mgmt;

#ifdef CONFIG_TESTING_OPTIONS
			if (wpa_s->ext_mgmt_frame_handling) {
				struct rx_mgmt *rx = &data->rx_mgmt;
				size_t hex_len = 2 * rx->frame_len + 1;
				char *hex = os_malloc(hex_len);
				if (hex) {
					wpa_snprintf_hex(hex, hex_len,
							 rx->frame, rx->frame_len);
					wpa_msg(wpa_s, MSG_INFO, "MGMT-RX freq=%d datarate=%u ssi_signal=%d %s",
						rx->freq, rx->datarate, rx->ssi_signal,
						hex);
					os_free(hex);
				}
				break;
			}
#endif /* CONFIG_TESTING_OPTIONS */

			mgmt = (const struct _ieee80211_mgmt *)
				data->rx_mgmt.frame;

#ifdef CONFIG_WNM_ACTIONS
			if (!data->rx_mgmt.frame)
				break;
#endif /* CONFIG_WNM_ACTIONS */

			fc = le_to_host16(mgmt->frame_control);
			stype = WLAN_FC_GET_STYPE(fc);

#ifdef CONFIG_AP
			if (wpa_s->ap_iface == NULL) {
#endif /* CONFIG_AP */
#ifdef CONFIG_P2P
				if (stype == WLAN_FC_STYPE_PROBE_REQ &&
				    data->rx_mgmt.frame_len > IEEE80211_HDRLEN) {
					const u8 *src = mgmt->sa;
					const u8 *ie;
					size_t ie_len;

					ie = data->rx_mgmt.frame + IEEE80211_HDRLEN;
					ie_len = data->rx_mgmt.frame_len -
						IEEE80211_HDRLEN;
					wpas_p2p_probe_req_rx(
						wpa_s, src, mgmt->da,
						mgmt->bssid, ie, ie_len,
						data->rx_mgmt.ssi_signal);
					break;
				}
#endif /* CONFIG_P2P */
#ifdef CONFIG_IBSS_RSN
				if (wpa_s->current_ssid &&
				    wpa_s->current_ssid->mode == WPAS_MODE_IBSS &&
				    stype == WLAN_FC_STYPE_AUTH &&
				    data->rx_mgmt.frame_len >= 30) {
					wpa_supplicant_event_ibss_auth(wpa_s, data);
					break;
				}
#endif /* CONFIG_IBSS_RSN */

				if (stype == WLAN_FC_STYPE_ACTION) {
					wpas_event_rx_mgmt_action(
						wpa_s, data->rx_mgmt.frame,
						data->rx_mgmt.frame_len,
						data->rx_mgmt.freq,
						data->rx_mgmt.ssi_signal);
					break;
				}

#ifdef	CONFIG_MESH
				if (wpa_s->ifmsh) {
					mesh_mpm_mgmt_rx(wpa_s, &data->rx_mgmt);
					break;
				}
#endif	/* CONFIG_MESH */

#ifdef CONFIG_SAE
				if (stype == WLAN_FC_STYPE_AUTH &&
				    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SME) &&
				    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SAE)) {
					sme_external_auth_mgmt_rx(
						wpa_s, data->rx_mgmt.frame,
						data->rx_mgmt.frame_len);
					break;
				}
#endif /* CONFIG_SAE */
				wpa_dbg(wpa_s, MSG_DEBUG, "AP: ignore received "
					"management frame in non-AP mode");
				break;
#ifdef CONFIG_AP
			}

			//[[ trinity_0170201_BEGIN -- p2p go mode crash bugfix
			if (wpa_s->ap_active && (ap_mgmt_rx(wpa_s, &data->rx_mgmt) > 0))
			//]] trinity_0170201_END
				break;

#if 0	/* by Bright : for WPA3 SAE */
			if (stype == WLAN_FC_STYPE_PROBE_REQ &&
			    data->rx_mgmt.frame_len > IEEE80211_HDRLEN) {
				const u8 *ie;
				size_t ie_len;

				ie = data->rx_mgmt.frame + IEEE80211_HDRLEN;
				ie_len = data->rx_mgmt.frame_len - IEEE80211_HDRLEN;

				wpas_notify_preq(wpa_s, mgmt->sa, mgmt->da,
						 mgmt->bssid, ie, ie_len,
						 data->rx_mgmt.ssi_signal);
			}
#endif	/* 0 */

			ap_mgmt_rx(wpa_s, &data->rx_mgmt);
#endif /* CONFIG_AP */
			break;
		}

#ifdef	CONFIG_SUPP27_EVENTS
		case EVENT_RX_PROBE_REQ:
			if (data->rx_probe_req.sa == NULL ||
			    data->rx_probe_req.ie == NULL)
				break;
#ifdef CONFIG_AP
			if (wpa_s->ap_iface) {
				hostapd_probe_req_rx(wpa_s->ap_iface->bss[0],
						     data->rx_probe_req.sa,
						     data->rx_probe_req.da,
						     data->rx_probe_req.bssid,
						     data->rx_probe_req.ie,
						     data->rx_probe_req.ie_len,
						     data->rx_probe_req.ssi_signal);
				break;
			}
#endif /* CONFIG_AP */
#ifdef CONFIG_P2P_UNUSED_CMD
			wpas_p2p_probe_req_rx(wpa_s, data->rx_probe_req.sa,
					      data->rx_probe_req.da,
					      data->rx_probe_req.bssid,
					      data->rx_probe_req.ie,
					      data->rx_probe_req.ie_len,
					      0,
					      data->rx_probe_req.ssi_signal);
#endif /* CONFIG_P2P_UNUSED_CMD */
			break;

		case EVENT_REMAIN_ON_CHANNEL:
#ifdef CONFIG_OFFCHANNEL
			offchannel_remain_on_channel_cb(
				wpa_s, data->remain_on_ch.freq,
				data->remain_on_ch.duration);
#endif /* CONFIG_OFFCHANNEL */
#ifdef CONFIG_P2P_UNUSED_CMD
			wpas_p2p_remain_on_channel_cb(
				wpa_s, data->remain_on_ch.freq,
				data->remain_on_ch.duration);
#endif /* CONFIG_P2P_UNUSED_CMD */
			break;
		case EVENT_CANCEL_REMAIN_ON_CHANNEL:
#ifdef CONFIG_OFFCHANNEL
			offchannel_cancel_remain_on_channel_cb(
				wpa_s, data->remain_on_ch.freq);
#endif /* CONFIG_OFFCHANNEL */
#ifdef CONFIG_P2P_UNUSED_CMD
			wpas_p2p_cancel_remain_on_channel_cb(
				wpa_s, data->remain_on_ch.freq);
#endif /* CONFIG_P2P_UNUSED_CMD */
#ifdef CONFIG_DPP
			wpas_dpp_cancel_remain_on_channel_cb(
				wpa_s, data->remain_on_ch.freq);
#endif /* CONFIG_DPP */
			break;

		case EVENT_EAPOL_RX:
			wpa_supplicant_rx_eapol(wpa_s, data->eapol_rx.src,
						data->eapol_rx.data,
						data->eapol_rx.data_len);
			break;
		case EVENT_SIGNAL_CHANGE:
			wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_SIGNAL_CHANGE
				"above=%d signal=%d noise=%d txrate=%d",
				data->signal_change.above_threshold,
				data->signal_change.current_signal,
				data->signal_change.current_noise,
				data->signal_change.current_txrate);
			wpa_bss_update_level(wpa_s->current_bss,
					     data->signal_change.current_signal);
#ifdef	CONFIG_BGSCAN
			bgscan_notify_signal_change(
				wpa_s, data->signal_change.above_threshold,
				data->signal_change.current_signal,
				data->signal_change.current_noise,
				data->signal_change.current_txrate);
#endif	/* CONFIG_BGSCAN */
			break;
		case EVENT_INTERFACE_MAC_CHANGED:
			wpa_supp_update_mac_addr(wpa_s);
			break;
		case EVENT_INTERFACE_ENABLED:
			wpa_dbg(wpa_s, MSG_DEBUG, "Interface was enabled");
			if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
				wpa_supp_update_mac_addr(wpa_s);
#ifdef CONFIG_P2P
				if (wpa_s->p2p_mgmt) {
					wpa_supplicant_set_state(wpa_s,
								 WPA_DISCONNECTED);
					break;
				}
#endif /* CONFIG_P2P */

#ifdef CONFIG_AP
				if (!wpa_s->ap_iface) {
					wpa_supplicant_set_state(wpa_s,
								 WPA_DISCONNECTED);
					wpa_s->scan_req = NORMAL_SCAN_REQ;
					wpa_supplicant_req_scan(wpa_s, 0, 0);
				} else
					wpa_supplicant_set_state(wpa_s,
								 WPA_COMPLETED);
#else /* CONFIG_AP */
				wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
				wpa_supplicant_req_scan(wpa_s, 0, 0);
#endif /* CONFIG_AP */
			}
			break;

		case EVENT_INTERFACE_DISABLED:
#ifdef CONFIG_P2P_UNUSED_CMD
			wpa_dbg(wpa_s, MSG_DEBUG, "Interface was disabled");

			if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_GO ||
			    (wpa_s->current_ssid && wpa_s->current_ssid->p2p_group &&
			     wpa_s->current_ssid->mode == WPAS_MODE_P2P_GO)) {
				/*
				 * Mark interface disabled if this happens to end up not
				 * being removed as a separate P2P group interface.
				 */
				wpa_supplicant_set_state(wpa_s, WPA_INTERFACE_DISABLED);
				/*
				 * The interface was externally disabled. Remove
				 * it assuming an external entity will start a
				 * new session if needed.
				 */
				if (wpa_s->current_ssid &&
				    wpa_s->current_ssid->p2p_group)
					wpas_p2p_interface_unavailable(wpa_s);
				else
					wpas_p2p_disconnect(wpa_s);
				/*
				 * wpa_s instance may have been freed, so must not use
				 * it here anymore.
				 */
				break;
			}
			if (wpa_s->p2p_scan_work && wpa_s->global->p2p &&
			    p2p_in_progress(wpa_s->global->p2p) > 1) {
				/* This radio work will be cancelled, so clear P2P
				 * state as well.
				 */
				p2p_stop_find(wpa_s->global->p2p);
			}
#endif /* CONFIG_P2P_UNUSED_CMD */

			if (wpa_s->wpa_state >= WPA_AUTHENTICATING) {
				/*
				 * Indicate disconnection to keep ctrl_iface events
				 * consistent.
				 */
				wpa_supplicant_event_disassoc(
					wpa_s, WLAN_REASON_DEAUTH_LEAVING, 1);
			}
			wpa_supplicant_mark_disassoc(wpa_s);
			wpa_bss_flush(wpa_s);
#if defined ( __SUPP_27_SUPPORT__ )
			radio_remove_works(wpa_s, NULL, 0);
#endif // __SUPP_27_SUPPORT__

			wpa_supplicant_set_state(wpa_s, WPA_INTERFACE_DISABLED);
			break;
#endif	/* CONFIG_SUPP27_EVENTS */

#ifdef CONFIG_IEEE80211H
		case EVENT_CHANNEL_LIST_CHANGED:
			wpa_supplicant_update_channel_list(
				wpa_s, &data->channel_list_changed);
			break;
#endif /* CONFIG_IEEE80211H */

		case EVENT_INTERFACE_UNAVAILABLE:
#ifdef CONFIG_P2P_UNUSED_CMD
			wpas_p2p_interface_unavailable(wpa_s);
#endif /* CONFIG_P2P_UNUSED_CMD */
			break;

#ifdef CONFIG_P2P_UNUSED_CMD
		case EVENT_BEST_CHANNEL:
			wpa_dbg(wpa_s, MSG_DEBUG, "Best channel event received "
				"(%d %d %d)",
				data->best_chan.freq_24, data->best_chan.freq_5,
				data->best_chan.freq_overall);

			wpa_s->best_24_freq = data->best_chan.freq_24;
#ifdef CONFIG_AP_SUPPORT_5GHZ
			wpa_s->best_5_freq = data->best_chan.freq_5;
#endif /* CONFIG_AP_SUPPORT_5GHZ */
			wpa_s->best_overall_freq = data->best_chan.freq_overall;

#ifdef CONFIG_P2P
			wpas_p2p_update_best_channels(wpa_s, data->best_chan.freq_24,
						      data->best_chan.freq_5,
						      data->best_chan.freq_overall);
#endif /* CONFIG_P2P */
			break;
#endif /* CONFIG_P2P_UNUSED_CMD */

#ifdef CONFIG_IEEE80211W
		case EVENT_UNPROT_DEAUTH:
			wpa_supplicant_event_unprot_deauth(wpa_s,
							   &data->unprot_deauth);
			break;
		case EVENT_UNPROT_DISASSOC:
			wpa_supplicant_event_unprot_disassoc(wpa_s,
							     &data->unprot_disassoc);
			break;
#endif /* CONFIG_IEEE80211W */

#ifdef	CONFIG_SUPP27_EVENTS
		case EVENT_STATION_LOW_ACK:
#ifdef CONFIG_AP
			if (wpa_s->ap_iface && data)
				hostapd_event_sta_low_ack(wpa_s->ap_iface->bss[0],
							  data->low_ack.addr);
#endif /* CONFIG_AP */
#ifdef CONFIG_TDLS
			if (data)
				wpa_tdls_disable_unreachable_link(wpa_s->wpa,
								  data->low_ack.addr);
#endif /* CONFIG_TDLS */
			break;

		case EVENT_IBSS_PEER_LOST:
#ifdef CONFIG_IBSS_RSN
			ibss_rsn_stop(wpa_s->ibss_rsn, data->ibss_peer_lost.peer);
#endif /* CONFIG_IBSS_RSN */
			break;
#endif /* CONFIG_SUPP27_EVENTS */

		case EVENT_DRIVER_GTK_REKEY:
			if (os_memcmp(data->driver_gtk_rekey.bssid,
				      wpa_s->bssid, ETH_ALEN))
				break;
			if (!wpa_s->wpa)
				break;
			wpa_sm_update_replay_ctr(wpa_s->wpa,
						 data->driver_gtk_rekey.replay_ctr);
			break;

#ifdef CONFIG_SUPP27_EVENTS
#if defined ( CONFIG_SCHED_SCAN )
		case EVENT_SCHED_SCAN_STOPPED:
			wpa_s->sched_scanning = 0;
			resched = wpa_s->scanning && wpas_scan_scheduled(wpa_s);
			wpa_supplicant_notify_scanning(wpa_s, 0);

			if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED)
				break;

			/*
			 * If the driver stopped scanning without being requested to,
			 * request a new scan to continue scanning for networks.
			 */
			if (!wpa_s->sched_scan_stop_req &&
			    wpa_s->wpa_state == WPA_SCANNING) {
				wpa_dbg(wpa_s, MSG_DEBUG,
					"Restart scanning after unexpected sched_scan stop event");
				wpa_supplicant_req_scan(wpa_s, 1, 0);
				break;
			}

			wpa_s->sched_scan_stop_req = 0;

			/*
			 * Start a new sched scan to continue searching for more SSIDs
			 * either if timed out or PNO schedule scan is pending.
			 */
			if (wpa_s->sched_scan_timed_out) {
				wpa_supplicant_req_sched_scan(wpa_s);
			} else if (wpa_s->pno_sched_pending) {
				wpa_s->pno_sched_pending = 0;
				wpas_start_pno(wpa_s);
			} else if (resched) {
				wpa_supplicant_req_scan(wpa_s, 0, 0);
			}

			break;
#endif // CONFIG_SCHED_SCAN

		case EVENT_WPS_BUTTON_PUSHED:
#ifdef CONFIG_WPS
			wpas_wps_start_pbc(wpa_s, NULL, 0);
#endif /* CONFIG_WPS */
			break;
		case EVENT_AVOID_FREQUENCIES:
			wpa_supplicant_notify_avoid_freq(wpa_s, data);
			break;

		case EVENT_CONNECT_FAILED_REASON:
#ifdef CONFIG_AP
			if (!wpa_s->ap_iface || !data)
				break;
			hostapd_event_connect_failed_reason(
				wpa_s->ap_iface->bss[0],
				data->connect_failed_reason.addr,
				data->connect_failed_reason.code);
#endif /* CONFIG_AP */
			break;
#endif /* CONFIG_SUPP27_EVENTS */

		case EVENT_NEW_PEER_CANDIDATE:
#ifdef CONFIG_MESH
			if (!wpa_s->ifmsh || !data)
				break;

#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
			if(data->mesh_peer.ie_len == 0)
			{
				mesh_mpm_sta_remove(wpa_s, data->mesh_peer.peer);
				//PRINTF("[%s]-------mesh_mpm_fsm_restart-------------", __func__);
				break;
			}
#endif // FEATURE_MESH_STAR_LINK_CONTROL_ADDED

#if 1 /* Added by Dialog */
			if (wpa_s->ifmsh) {
				struct hostapd_data *hapd;
				struct sta_info *mesh_sta;

				hapd = wpa_s->ifmsh->bss[0];

				mesh_sta = (struct sta_info *)ap_get_sta(hapd, data->mesh_peer.peer); /* ���� �õ� �ܸ� ������ ���Ӱ��� ���? sta_info)���� ã�� */

				if ((mesh_sta == NULL) && (hapd->num_sta == wpa_s->ifmsh->conf->bss[0]->max_num_sta)) { /* ���� �õ� �ܸ� ������ ���� ���?*/
					wpa_msg(wpa_s, MSG_DEBUG, "[%s] Discard NEW_PEER_CANDIDATE "MACSTR " num_plinks=[%d/%d] num_sta=[%d/%d]",
						__func__,
						MAC2STR(data->mesh_peer.peer),
						hapd->num_plinks,
						hapd->max_plinks,
						hapd->num_sta,
						wpa_s->ifmsh->conf->bss[0]->max_num_sta);

					mesh_mpm_sta_sate_remove(wpa_s);

					if (hapd->num_sta == wpa_s->ifmsh->conf->bss[0]->max_num_sta) {
					return;
					}
				} else {
					wpa_msg(wpa_s, MSG_DEBUG, "[%s] num_plinks=[%d/%d] num_sta=[%d/%d] - " MACSTR,
						__func__,
						hapd->num_plinks,
						hapd->max_plinks,
						hapd->num_sta,
						wpa_s->ifmsh->conf->bss[0]->max_num_sta,
						MAC2STR(data->mesh_peer.peer));
				}
			}
#endif  /* Added by Dialog */
			wpa_mesh_notify_peer(wpa_s, data->mesh_peer.peer,
					     data->mesh_peer.ies,
					     data->mesh_peer.ie_len);
#endif /* CONFIG_MESH */
			break;

#ifdef CONFIG_ACS
		case EVENT_SURVEY:
#ifdef CONFIG_AP
			if (!wpa_s->ap_iface)
				break;
			hostapd_event_get_survey(wpa_s->ap_iface,
						 &data->survey_results);
#endif /* CONFIG_AP */
			break;
#endif /* CONFIG_ACS */

		case EVENT_ACS_CHANNEL_SELECTED:
#ifdef CONFIG_AP
#ifdef CONFIG_ACS
			if (!wpa_s->ap_iface)
				break;
			hostapd_acs_channel_selected(wpa_s->ap_iface->bss[0],
						     &data->acs_selected_channels);
#endif /* CONFIG_ACS */
#endif /* CONFIG_AP */
			break;

#ifdef	CONFIG_P2P_UNUSED_CMD
		case EVENT_P2P_LO_STOP:
#ifdef CONFIG_P2P
			wpa_s->p2p_lo_started = 0;
			wpa_msg(wpa_s, MSG_INFO, P2P_EVENT_LISTEN_OFFLOAD_STOP
				P2P_LISTEN_OFFLOAD_STOP_REASON "reason=%d",
				data->p2p_lo_stop.reason_code);
#endif /* CONFIG_P2P */
			break;
#endif	/* CONFIG_P2P_UNUSED_CMD */

#ifdef	CONFIG_SUPP27_EVENTS
		case EVENT_BEACON_LOSS:
			if (!wpa_s->current_bss || !wpa_s->current_ssid)
				break;
			wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_BEACON_LOSS);
#ifdef	CONFIG_BGSCAN
			bgscan_notify_beacon_loss(wpa_s);
#endif	/* CONFIG_BGSCAN */
			break;
#endif	/* CONFIG_SUPP27_EVENTS */

		case EVENT_EXTERNAL_AUTH:
#ifdef CONFIG_SAE
			if (!wpa_s->current_ssid) {
				wpa_printf_dbg(MSG_DEBUG, "SAE: current_ssid is NULL");
				break;
			}
			sme_external_auth_trigger(wpa_s, data);
#endif /* CONFIG_SAE */
			break;

		case EVENT_PORT_AUTHORIZED:
			wpa_supplicant_event_port_authorized(wpa_s);
			break;

#ifdef	CONFIG_SUPP27_EVENTS
		case EVENT_STATION_OPMODE_CHANGED:
#ifdef CONFIG_AP
			if (!wpa_s->ap_iface || !data)
				break;

			hostapd_event_sta_opmode_changed(wpa_s->ap_iface->bss[0],
							 data->sta_opmode.addr,
							 data->sta_opmode.smps_mode,
							 data->sta_opmode.chan_width,
							 data->sta_opmode.rx_nss);
#endif /* CONFIG_AP */
			break;
#endif	/* CONFIG_SUPP27_EVENTS */

		default:
			wpa_msg(wpa_s, MSG_INFO, "Unknown event %d", event);
			break;
	}
}


void wpa_supplicant_event_global(void *ctx, enum wpa_event_type event,
				 union wpa_event_data *data)
{
	struct wpa_supplicant *wpa_s;

	if (event != EVENT_INTERFACE_STATUS)
		return;

	wpa_s = wpa_supplicant_get_iface(ctx, data->interface_status.ifname);
	if (wpa_s && wpa_s->driver->get_ifindex) {
		unsigned int ifindex;

		ifindex = wpa_s->driver->get_ifindex(wpa_s->drv_priv);
		if (ifindex != data->interface_status.ifindex) {
			wpa_dbg(wpa_s, MSG_DEBUG,
				"interface status ifindex %d mismatch (%d)",
				ifindex, data->interface_status.ifindex);
			return;
		}
	}
#ifdef CONFIG_MATCH_IFACE
	else if (data->interface_status.ievent == EVENT_INTERFACE_ADDED) {
		struct wpa_interface *wpa_i;

		wpa_i = wpa_supplicant_match_iface(
			ctx, data->interface_status.ifname);
		if (!wpa_i)
			return;
		wpa_s = wpa_supplicant_add_iface(ctx, wpa_i, NULL);
		os_free(wpa_i);
		if (wpa_s)
			wpa_s->matched = 1;
	}
#endif /* CONFIG_MATCH_IFACE */

	if (wpa_s)
		wpa_supplicant_event(wpa_s, event, data);
}


void supp_scan_result_event(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)eloop_ctx;

	wpa_supplicant_event(wpa_s, EVENT_SCAN_RESULTS, NULL);
}

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
extern void rdev_bss_lock();
extern void rdev_bss_unlock();
#endif

void wpa_scan_results_free(struct wpa_scan_results *res)
{
	size_t i;

	if (res == NULL)
		return;

	da16x_scan_prt("[%s] Free %d scan results\n", __func__,  res->num);

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	rdev_bss_lock();
	for(i = 0 ; i <res->num; i++)
	{
		struct wpa_scan_res *temp= NULL;

		temp = res->res[i];
		
		if(temp->internal_bss != NULL)
		#if 1
			supp_unhold_bss(temp->internal_bss);
		#else
			sup_unlink_bss(temp->internal_bss);
		#endif

		temp->internal_bss = NULL;
		temp->ie = NULL;
		temp->beacon_ie = NULL;
		umac_heap_free(res->res[i]);
		res->res[i] = NULL;
			
		os_memmove(&res->res[i], &res->res[i + 1],
			   (res->num - i - 1)* sizeof(struct wpa_bss *));
		res->res[res->num-1] = NULL;
		res->num--;
		i--; 
	}
	umac_heap_free(res->res);
	res->res = NULL;
	rdev_bss_unlock();
#else /* CONFIG_REUSED_UMAC_BSS_LIST */
#ifdef UMAC_MEM_USE_TX_MEM
	for (i = 0; i < res->num; i++) {
#ifdef CONFIG_SCAN_UMAC_HEAP_ALLOC
		struct wpa_scan_res *temp = NULL;

		temp = res->res[i];

		umac_heap_free(temp->ie);
		umac_heap_free(temp->beacon_ie);

		temp->ie = NULL;
		temp->beacon_ie = NULL;
		umac_heap_free(temp);
#else
		umac_heap_free(res->res[i]);
#endif
	}
	umac_heap_free(res->res);
#else /* UMAC_MEM_USE_TX_MEM */
	for (i = 0; i < res->num; i++) {
		os_free(res->res[i]);
	}
	os_free(res->res);
#endif /* UMAC_MEM_USE_TX_MEM */
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
	os_free(res);
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	extern void all_bss_list_expire();
	all_bss_list_expire();
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

}

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
void wpa_supplicant_clear_current_bss(struct wpa_supplicant *wpa_s)
{
	da16x_scan_prt("[%s] Clear current_bss information. 0x%x\n",
				__func__, wpa_s->current_bss);
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
	if(wpa_s->current_bss->ie_len) {
		da16x_scan_prt("[%s] Free bss->ie 0x%x, len = %d\n", __func__,wpa_s->current_bss->ie, wpa_s->current_bss->ie_len );
		os_free(wpa_s->current_bss->ie);
		wpa_s->current_bss->ie = NULL;
	}
	if(wpa_s->current_bss->beacon_ie_len) {
		da16x_scan_prt("[%s] Free bss->beacon_ie 0x%x, len = %d\n", __func__,wpa_s->current_bss->beacon_ie, wpa_s->current_bss->beacon_ie_len );
		os_free(wpa_s->current_bss->beacon_ie);
		wpa_s->current_bss->beacon_ie = NULL;
	}
#else
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	wpa_s->current_bss = NULL;
#else
	wpa_bss_remove(wpa_s, wpa_s->current_bss, __func__);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */
#endif

}

void wpa_supplicant_copy_ie_for_current_bss(struct wpa_supplicant *wpa_s, struct wpa_bss *bss)
{
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
	u8 *tmp_ptr = NULL;
#endif /* CONFIG_SCAN_UMAC_HEAP_ALLOC */

	da16x_scan_prt("[%s] Copy IE information for current_bss\n", __func__);
	da16x_scan_prt("[%s] wpa_s->current_bss  = 0x%x\n", __func__, wpa_s->current_bss);

#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
	if(bss->ie_len) {
		tmp_ptr = bss->ie;
		bss->ie = os_malloc(bss->ie_len);
		if(bss->ie) {
			da16x_scan_prt("[%s] Alloc  ie = 0x%x, len = %d\n", __func__,
						bss->ie, bss->ie_len);
			os_memcpy(bss->ie, tmp_ptr, bss->ie_len);
		}
	}
	if(bss->beacon_ie_len) {
		tmp_ptr = bss->beacon_ie;
		bss->beacon_ie = os_malloc(bss->beacon_ie_len);
		if(bss->beacon_ie) {
			da16x_scan_prt("[%s] Alloc  beacon_ie = 0x%x, len = %d\n", __func__,
						bss->beacon_ie, bss->beacon_ie_len);
			os_memcpy(bss->beacon_ie, tmp_ptr, bss->beacon_ie_len);
		}
	}
#endif // CONFIG_SCAN_UMAC_HEAP_ALLOC
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	wpa_s->current_bss = bss;
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

}
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

/* EOF */
