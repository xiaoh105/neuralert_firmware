/*
 * WPA Supplicant - Scanning
 * Copyright (c) 2003-2019, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 * 
 *
 * Copyright (c) 2019-2022 Modified by Renesas Electronics.
 *
 */

#include "includes.h"

#include "supp_common.h"
#include "supp_eloop.h"
#include "common/ieee802_11_defs.h"
#include "wpa_ctrl.h"
#include "supp_config.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "wps_supplicant.h"
#ifdef	CONFIG_P2P
#include "p2p_supplicant.h"
#include "p2p/supp_p2p.h"
#endif	/* CONFIG_P2P */
#ifdef CONFIG_HS20
#include "hs20_supplicant.h"
#endif /* CONFIG_HS20 */
#include "bss.h"
#include "supp_scan.h"
#ifdef CONFIG_MESH
#include "supp_mesh.h"
#endif /* CONFIG_MESH */

#include "utils/supp_eloop.h"
#if 0 /* Debug */
#include "common_utils.h"
#endif /* Debug */

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"

#ifdef CONFIG_IMMEDIATE_SCAN
extern EventGroupHandle_t    da16x_sp_event_group;
#endif /* CONFIG_IMMEDIATE_SCAN */

extern unsigned char	passive_scan_feature_flag;

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
static void wpa_supplicant_gen_assoc_event(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;
	union wpa_event_data data;

	ssid = wpa_supplicant_get_ssid(wpa_s);
	if (ssid == NULL)
		return;

	if (wpa_s->current_ssid == NULL) {
		wpa_s->current_ssid = ssid;
#ifdef CONFIG_NOTIFY
		wpas_notify_network_changed(wpa_s);
#endif /* CONFIG_NOTIFY */
	}
#ifdef	IEEE8021X_EAPOL
	wpa_supplicant_initiate_eapol(wpa_s);
#endif	/* IEEE8021X_EAPOL */
	wpa_dbg(wpa_s, MSG_DEBUG, "Already associated with a configured "
		"network - generating associated event");
	os_memset(&data, 0, sizeof(data));
	wpa_supplicant_event(wpa_s, EVENT_ASSOC, &data);
}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

#ifdef CONFIG_WPS
static int wpas_wps_in_use(struct wpa_supplicant *wpa_s,
			   enum wps_request_type *req_type)
{
	struct wpa_ssid *ssid;
	int wps = 0;

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (!(ssid->key_mgmt & WPA_KEY_MGMT_WPS))
			continue;

		wps = 1;
		*req_type = wpas_wps_get_req_type(ssid);
		if (ssid->eap.phase1 && os_strstr(ssid->eap.phase1, "pbc=1"))
			return 2;
	}

#ifdef CONFIG_P2P
	if (!wpa_s->global->p2p_disabled && wpa_s->global->p2p &&
	    !wpa_s->conf->p2p_disabled) {
		wpa_s->wps->dev.p2p = 1;
		if (!wps) {
			wps = 1;
			*req_type = WPS_REQ_ENROLLEE_INFO;
		}
	}
#endif /* CONFIG_P2P */

	return wps;
}
#endif /* CONFIG_WPS */


/**
 * wpa_supplicant_enabled_networks - Check whether there are enabled networks
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 if no networks are enabled, >0 if networks are enabled
 *
 * This function is used to figure out whether any networks (or Interworking
 * with enabled credentials and auto_interworking) are present in the current
 * configuration.
 */
int wpa_supplicant_enabled_networks(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid = wpa_s->conf->ssid;
	int count = 0, disabled = 0;

#ifdef CONFIG_P2P
	if (wpa_s->p2p_mgmt)
		return 0; /* no normal network profiles on p2p_mgmt interface */
#endif /* CONFIG_P2P */

	while (ssid) {
#ifdef CONFIG_AP
		if (wpas_is_network_ap(ssid))
			disabled++;
#if 0	/* by Shingu 20161107 (PROFILE_ERROR) */
		else if (!wpas_network_disabled(wpa_s, ssid))
			count++;
#else
		else if (!wpas_network_disabled(wpa_s, ssid) &&
			 (ssid->ssid != NULL || ssid->temporary == 1))
			count++;
#endif	/* 1 */
		else
			disabled++;
#else /* CONFIG_AP */
		if (!wpas_network_disabled(wpa_s, ssid))
			count++;
		else
			disabled++;
#endif /* CONFIG_AP */
		ssid = ssid->next;
	}

#ifdef	CONFIG_INTERWORKING
	if (wpa_s->conf->cred && wpa_s->conf->interworking &&
	    wpa_s->conf->auto_interworking)
		count++;
#endif	/* CONFIG_INTERWORKING */

	if (count == 0 && disabled > 0) {
		wpa_dbg(wpa_s, MSG_DEBUG, "No enabled networks (%d disabled "
			"networks)", disabled);
	}

	return count;
}


#ifndef FEATURE_USE_DEFAULT_AP_SCAN
static void wpa_supplicant_assoc_try(struct wpa_supplicant *wpa_s,
				     struct wpa_ssid *ssid)
{
#ifdef	__SUPP_27_SUPPORT__
	int min_temp_disabled = 0;
#endif	/* __SUPP_27_SUPPORT__ */

	while (ssid) {
		if (!wpas_network_disabled(wpa_s, ssid)) {
#ifdef	__SUPP_27_SUPPORT__
			int temp_disabled = wpas_temp_disabled(wpa_s, ssid);

			if (temp_disabled <= 0)
				break;

			if (!min_temp_disabled ||
			    temp_disabled < min_temp_disabled)
				min_temp_disabled = temp_disabled;
#else
			break;
#endif	/* __SUPP_27_SUPPORT__ */
		}
		ssid = ssid->next;
	}

	/* ap_scan=2 mode - try to associate with each SSID. */
	if (ssid == NULL) {
		wpa_dbg(wpa_s, MSG_DEBUG, "wpa_supplicant_assoc_try: Reached "
			"end of scan list - go back to beginning");
		wpa_s->prev_scan_ssid = WILDCARD_SSID_SCAN;
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
#ifdef	__SUPP_27_SUPPORT__
		wpa_supplicant_req_scan(wpa_s, min_temp_disabled, 0);
#else
		wpa_supplicant_req_scan(wpa_s, 0, 0);
#endif	/* __SUPP_27_SUPPORT__ */
		return;
	}
	if (ssid->next) {
		/* Continue from the next SSID on the next attempt. */
		wpa_s->prev_scan_ssid = ssid;
	} else {
		/* Start from the beginning of the SSID list. */
		wpa_s->prev_scan_ssid = WILDCARD_SSID_SCAN;
	}
	wpa_supplicant_associate(wpa_s, NULL, ssid);
}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */


int wpa_supplicant_abort_scan(struct wpa_supplicant *wpa_s,
				struct wpa_driver_scan_params *params)
{
#if	defined(CONFIG_SCAN_WITH_BSSID) || defined(CONFIG_SCAN_WITH_DIR_SSID)
	wpa_s->manual_scan_promisc = 0;
#endif /* CONFIG_SCAN_WITH_BSSID || CONFIG_SCAN_WITH_DIR_SSID */
	return 0;
}


#ifdef UNUSED_CODE
static void wpas_trigger_scan_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct wpa_driver_scan_params *params = work->ctx;
	int ret;

	if (deinit) {
		if (!work->started) {
			wpa_scan_free_params(params);
			return;
		}
		wpa_supplicant_notify_scanning(wpa_s, 0);
#if 0	/* by Bright : Merge 2.7 */
		wpas_notify_scan_done(wpa_s, 0);
#endif	/* 0 */
#if defined ( CONFIG_SCAN_WORK )
		wpa_s->scan_work = NULL;
#endif	// CONFIG_SCAN_WORK
		return;
	}

	if (wpas_update_random_addr_disassoc(wpa_s) < 0) {
		wpa_msg(wpa_s, MSG_INFO,
			"Failed to assign random MAC address for a scan");
		wpa_scan_free_params(params);
#if 0	/* by Bright : Merge 2.7 */
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_SCAN_FAILED "ret=-1");
		radio_work_done(work);
#endif	/* 0 */
		return;
	}

	wpa_supplicant_notify_scanning(wpa_s, 1);

	if (wpa_s->clear_driver_scan_cache) {
		wpa_printf_dbg(MSG_DEBUG,
			   "Request driver to clear scan cache due to local BSS flush");
		params->only_new_results = 1;
	}
	ret = wpa_drv_scan(wpa_s, params);
	/*
	 * Store the obtained vendor scan cookie (if any) in wpa_s context.
	 * The current design is to allow only one scan request on each
	 * interface, hence having this scan cookie stored in wpa_s context is
	 * fine for now.
	 *
	 * Revisit this logic if concurrent scan operations per interface
	 * is supported.
	 */
#ifdef UNUSED_CODE
	if (ret == 0)
		wpa_s->curr_scan_cookie = params->scan_cookie;
#endif /* UNUSED_CODE */

	wpa_scan_free_params(params);
	work->ctx = NULL;
	if (ret) {
		int retry = wpa_s->last_scan_req != MANUAL_SCAN_REQ &&
			!wpa_s->beacon_rep_data.token;

		if (wpa_s->disconnected)
			retry = 0;

		wpa_supplicant_notify_scanning(wpa_s, 0);
#if 0	/* by Bright : Merge 2.7 */
		wpas_notify_scan_done(wpa_s, 0);
#endif	/* 0 */
		if (wpa_s->wpa_state == WPA_SCANNING)
			wpa_supplicant_set_state(wpa_s,
						 wpa_s->scan_prev_wpa_state);
		wpa_msg(wpa_s, MSG_INFO, WPA_EVENT_SCAN_FAILED "ret=%d%s",
			ret, retry ? " retry=1" : "");
#if 0	/* by Bright : Merge 2.7 */
		radio_work_done(work);
#endif	/* 0 */

		if (retry) {
			/* Restore scan_req since we will try to scan again */
			wpa_s->scan_req = wpa_s->last_scan_req;
			wpa_supplicant_req_scan(wpa_s, 1, 0);
		} else if (wpa_s->scan_res_handler) {
			/* Clear the scan_res_handler */
			wpa_s->scan_res_handler = NULL;
		}

#ifdef	CONFIG_RRM
		if (wpa_s->beacon_rep_data.token)
			wpas_rrm_refuse_request(wpa_s);
#endif	/* CONFIG_RRM */

		return;
	}

	os_get_reltime(&wpa_s->scan_trigger_time);
	wpa_s->scan_runs++;
	wpa_s->normal_scans++;
	wpa_s->own_scan_requested = 1;
	wpa_s->clear_driver_scan_cache = 0;
#if defined ( CONFIG_SCAN_WORK )
	wpa_s->scan_work = work;
#endif	// CONFIG_SCAN_WORK
}
#endif /* UNUSED_CODE */


/**
 * wpa_supplicant_trigger_scan - Request driver to start a scan
 * @wpa_s: Pointer to wpa_supplicant data
 * @params: Scan parameters
 * Returns: 0 on success, -1 on failure
 */
int wpa_supplicant_trigger_scan(struct wpa_supplicant *wpa_s,
				struct wpa_driver_scan_params *params)
{
	int	ret;

	if (wpa_s->scanning != 1)
		wpa_s->scanning = 1;

	if (wpa_s->clear_driver_scan_cache)
		params->only_new_results = 1;

	/* Call Init2() OPS */
	ret = wpa_drv_scan(wpa_s, params);
	if (ret) {
		if (wpa_s->scanning != 0)
			wpa_s->scanning = 0;

		return ret;
	}

	os_get_reltime(&wpa_s->scan_trigger_time);
	wpa_s->scan_runs++;
	wpa_s->normal_scans++;
	wpa_s->own_scan_requested = 1;
	wpa_s->clear_driver_scan_cache = 0;

#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
	wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */

	return 0;
}


#if defined ( CONFIG_SCHED_SCAN )
static void
wpa_supplicant_delayed_sched_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	wpa_dbg(wpa_s, MSG_DEBUG, "Starting delayed sched scan");

	if (wpa_supplicant_req_sched_scan(wpa_s))
		wpa_supplicant_req_scan(wpa_s, 0, 0);
}


static void
wpa_supplicant_sched_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	wpa_dbg(wpa_s, MSG_DEBUG, "Sched scan timeout - stopping it");

	wpa_s->sched_scan_timed_out = 1;
	wpa_supplicant_cancel_sched_scan(wpa_s);
}


static int
wpa_supplicant_start_sched_scan(struct wpa_supplicant *wpa_s,
				struct wpa_driver_scan_params *params)
{
	int ret;

	wpa_supplicant_notify_scanning(wpa_s, 1);
	ret = wpa_drv_sched_scan(wpa_s, params);
	if (ret)
		wpa_supplicant_notify_scanning(wpa_s, 0);
	else
		wpa_s->sched_scanning = 1;

	return ret;
}


static int wpa_supplicant_stop_sched_scan(struct wpa_supplicant *wpa_s)
{
	int ret;

	ret = wpa_drv_stop_sched_scan(wpa_s);
	if (ret) {
		wpa_dbg(wpa_s, MSG_DEBUG, "stopping sched_scan failed!");
		/* TODO: what to do if stopping fails? */
		return -1;
	}

	return ret;
}

#endif	// CONFIG_SCHED_SCAN

struct wpa_driver_scan_filter*
wpa_supplicant_build_filter_ssids(struct wpa_config *conf, size_t *num_ssids
#ifdef SUPPORT_SELECT_NETWORK_FILTER
					, int set
#endif /* SUPPORT_SELECT_NETWORK_FILTER */
					)
{
	struct wpa_driver_scan_filter *ssids=NULL;
	struct wpa_ssid *ssid=NULL;
	size_t count=0;

	*num_ssids = 0;

#ifdef SUPPORT_SELECT_NETWORK_FILTER
	if (set != 1)
#endif /* SUPPORT_SELECT_NETWORK_FILTER */
	{
		if (!conf->filter_ssids)
			return NULL;
	}

	for (count = 0, ssid = conf->ssid; ssid; ssid = ssid->next) {
		if (ssid->ssid && ssid->ssid_len)
			count++;
	}

	if (count == 0)
		return NULL;

	ssids = os_calloc(count, sizeof(struct wpa_driver_scan_filter));

	if (ssids == NULL)
		return NULL;

	for (ssid = conf->ssid; ssid; ssid = ssid->next) {
		if (!ssid->ssid || !ssid->ssid_len)
			continue;
		os_memcpy(ssids[*num_ssids].ssid, ssid->ssid, ssid->ssid_len);
		ssids[*num_ssids].ssid_len = ssid->ssid_len;
		(*num_ssids)++;
	}

	return ssids;
}


static void wpa_supplicant_optimize_freqs(
	struct wpa_supplicant *wpa_s, struct wpa_driver_scan_params *params)
{
#ifdef CONFIG_P2P
	if (params->freqs == NULL && wpa_s->p2p_in_provisioning &&
	    wpa_s->go_params) {
		/* Optimize provisioning state scan based on GO information */
		if (wpa_s->p2p_in_provisioning < 5 &&
		    wpa_s->go_params->freq > 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Scan only GO "
				"preferred frequency %d MHz",
				wpa_s->go_params->freq);
			params->freqs = os_calloc(2, sizeof(int));
			if (params->freqs)
				params->freqs[0] = wpa_s->go_params->freq;
		} else if (wpa_s->p2p_in_provisioning < 8 &&
			   wpa_s->go_params->freq_list[0]) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Scan only common "
				"channels");
			int_array_concat(&params->freqs,
					 wpa_s->go_params->freq_list);
			if (params->freqs)
				int_array_sort_unique(params->freqs);
		}
		wpa_s->p2p_in_provisioning++;
	}

#ifdef	CONFIG_P2P_OPTION
	if (params->freqs == NULL && wpa_s->p2p_in_invitation) {
		/*
		 * Optimize scan based on GO information during persistent
		 * group reinvocation
		 */
		if (wpa_s->p2p_in_invitation < 5 &&
		    wpa_s->p2p_invite_go_freq > 0) {
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Scan only GO preferred frequency %d MHz during invitation",
				wpa_s->p2p_invite_go_freq);
			params->freqs = os_calloc(2, sizeof(int));
			if (params->freqs)
				params->freqs[0] = wpa_s->p2p_invite_go_freq;
		}
		wpa_s->p2p_in_invitation++;
		if (wpa_s->p2p_in_invitation > 20) {
			/*
			 * This should not really happen since the variable is
			 * cleared on group removal, but if it does happen, make
			 * sure we do not get stuck in special invitation scan
			 * mode.
			 */
			wpa_dbg(wpa_s, MSG_DEBUG, "P2P: Clear p2p_in_invitation");
			wpa_s->p2p_in_invitation = 0;
		}
	}
#endif	/* CONFIG_P2P_OPTION */
#endif	/* CONFIG_P2P */

#ifdef CONFIG_WPS
	if (params->freqs == NULL && wpa_s->after_wps && wpa_s->wps_freq) {
		/*
		 * Optimize post-provisioning scan based on channel used
		 * during provisioning.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG, "WPS: Scan only frequency %u MHz "
			"that was used during provisioning", wpa_s->wps_freq);
		params->freqs = os_calloc(2, sizeof(int));
		if (params->freqs)
			params->freqs[0] = wpa_s->wps_freq;
		wpa_s->after_wps--;
	} else if (wpa_s->after_wps)
		wpa_s->after_wps--;

	if (params->freqs == NULL && wpa_s->known_wps_freq && wpa_s->wps_freq)
	{
		/* Optimize provisioning scan based on already known channel */
		wpa_dbg(wpa_s, MSG_DEBUG, "WPS: Scan only frequency %u MHz",
			wpa_s->wps_freq);
		params->freqs = os_calloc(2, sizeof(int));
		if (params->freqs)
			params->freqs[0] = wpa_s->wps_freq;
		wpa_s->known_wps_freq = 0; /* only do this once */
	}
#endif /* CONFIG_WPS */
}


#ifdef CONFIG_INTERWORKING
static void wpas_add_interworking_elements(struct wpa_supplicant *wpa_s,
					   struct wpabuf *buf)
{
	wpabuf_put_u8(buf, WLAN_EID_INTERWORKING);
	wpabuf_put_u8(buf, is_zero_ether_addr(wpa_s->conf->hessid) ? 1 :
		      1 + ETH_ALEN);
	wpabuf_put_u8(buf, wpa_s->conf->access_network_type);
	/* No Venue Info */
	if (!is_zero_ether_addr(wpa_s->conf->hessid))
		wpabuf_put_data(buf, wpa_s->conf->hessid, ETH_ALEN);
}
#endif /* CONFIG_INTERWORKING */


#ifdef	__SUPP_27_SUPPORT__
void wpa_supplicant_set_default_scan_ies(struct wpa_supplicant *wpa_s)
{
	struct wpabuf *default_ies = NULL;
	u8 ext_capab[18];
	int ext_capab_len;
	enum wpa_driver_if_type type = WPA_IF_STATION;

#ifdef CONFIG_P2P
	if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT)
		type = WPA_IF_P2P_CLIENT;
#endif /* CONFIG_P2P */

	wpa_drv_get_ext_capa(wpa_s, type);

	ext_capab_len = wpas_build_ext_capab(wpa_s, ext_capab,
					     sizeof(ext_capab));
	if (ext_capab_len > 0 &&
	    wpabuf_resize(&default_ies, ext_capab_len) == 0)
		wpabuf_put_data(default_ies, ext_capab, ext_capab_len);

#ifdef CONFIG_MBO
	/* Send MBO and OCE capabilities */
	if (wpabuf_resize(&default_ies, 12) == 0)
		wpas_mbo_scan_ie(wpa_s, default_ies);
#endif /* CONFIG_MBO */

	if (default_ies)
		wpa_drv_set_default_scan_ies(wpa_s, wpabuf_head(default_ies),
					     wpabuf_len(default_ies));
	wpabuf_free(default_ies);
}
#endif	/* __SUPP_27_SUPPORT__ */


static struct wpabuf * wpa_supplicant_extra_ies(struct wpa_supplicant *wpa_s)
{
	struct wpabuf *extra_ie = NULL;
#ifdef	__SUPP_27_SUPPORT__
	u8 ext_capab[18];
	int ext_capab_len;
#endif	/* __SUPP_27_SUPPORT__ */
#ifdef CONFIG_WPS
	int wps = 0;
	enum wps_request_type req_type = WPS_REQ_ENROLLEE_INFO;
#endif /* CONFIG_WPS */

#ifdef	__SUPP_27_SUPPORT__
#ifdef CONFIG_P2P
	if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT)
		wpa_drv_get_ext_capa(wpa_s, WPA_IF_P2P_CLIENT);
	else
#endif /* CONFIG_P2P */
		wpa_drv_get_ext_capa(wpa_s, WPA_IF_STATION);

	ext_capab_len = wpas_build_ext_capab(wpa_s, ext_capab,
					     sizeof(ext_capab));
	if (ext_capab_len > 0 &&
	    wpabuf_resize(&extra_ie, ext_capab_len) == 0)
		wpabuf_put_data(extra_ie, ext_capab, ext_capab_len);
#endif	/* __SUPP_27_SUPPORT__ */

#ifdef CONFIG_INTERWORKING
	if (wpa_s->conf->interworking &&
	    wpabuf_resize(&extra_ie, 100) == 0)
		wpas_add_interworking_elements(wpa_s, extra_ie);
#endif /* CONFIG_INTERWORKING */

#ifdef CONFIG_WPS
	wps = wpas_wps_in_use(wpa_s, &req_type);

	if (wps) {
		struct wpabuf *wps_ie;
		wps_ie = wps_build_probe_req_ie(wps == 2 ? DEV_PW_PUSHBUTTON :
						DEV_PW_DEFAULT,
						&wpa_s->wps->dev,
						wpa_s->wps->uuid, req_type,
						0, NULL);
		if (wps_ie) {
			if (wpabuf_resize(&extra_ie, wpabuf_len(wps_ie)) == 0)
				wpabuf_put_buf(extra_ie, wps_ie);
			wpabuf_free(wps_ie);
		}
	}

#ifdef CONFIG_P2P
	if (wps) {
#ifdef	CONFIG_P2P_UNUSED_CMD
		size_t ielen = p2p_scan_ie_buf_len(wpa_s->global->p2p);
#else
		size_t ielen = 100;
#endif	/* CONFIG_P2P_UNUSED_CMD */
		if (wpabuf_resize(&extra_ie, ielen) == 0)
			wpas_p2p_scan_ie(wpa_s, extra_ie);
	}
#endif /* CONFIG_P2P */

#ifdef	CONFIG_MESH
	wpa_supplicant_mesh_add_scan_ie(wpa_s, &extra_ie);
#endif	/* CONFIG_MESH */

#endif /* CONFIG_WPS */

#ifdef CONFIG_HS20
	if (wpa_s->conf->hs20 && wpabuf_resize(&extra_ie, 7) == 0)
		wpas_hs20_add_indication(extra_ie, -1);
#endif /* CONFIG_HS20 */

#ifdef CONFIG_FST
	if (wpa_s->fst_ies &&
	    wpabuf_resize(&extra_ie, wpabuf_len(wpa_s->fst_ies)) == 0)
		wpabuf_put_buf(extra_ie, wpa_s->fst_ies);
#endif /* CONFIG_FST */

#ifdef CONFIG_MBO
	/* Send MBO and OCE capabilities */
	if (wpabuf_resize(&extra_ie, 12) == 0)
		wpas_mbo_scan_ie(wpa_s, extra_ie);
#endif /* CONFIG_MBO */

#if defined ( CONFIG_VENDOR_ELEM )
	if (wpa_s->vendor_elem[VENDOR_ELEM_PROBE_REQ]) {
		struct wpabuf *buf = wpa_s->vendor_elem[VENDOR_ELEM_PROBE_REQ];

		if (wpabuf_resize(&extra_ie, wpabuf_len(buf)) == 0)
			wpabuf_put_buf(extra_ie, buf);
	}
#endif	// CONFIG_VENDOR_ELEM

	return extra_ie;
}


#ifdef CONFIG_P2P

/*
 * Check whether there are any enabled networks or credentials that could be
 * used for a non-P2P connection.
 */
static int non_p2p_network_enabled(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (wpas_network_disabled(wpa_s, ssid))
			continue;
		if (!ssid->p2p_group)
			return 1;
	}

#ifdef	CONFIG_INTERWORKING
	if (wpa_s->conf->cred && wpa_s->conf->interworking &&
	    wpa_s->conf->auto_interworking)
		return 1;
#endif	/* CONFIG_INTERWORKING */

	return 0;
}

#endif /* CONFIG_P2P */


static void wpa_setband_scan_freqs_list(struct wpa_supplicant *wpa_s,
					enum hostapd_hw_mode band,
					struct wpa_driver_scan_params *params)
{
	/* Include only supported channels for the specified band */
	struct hostapd_hw_modes *mode;
	int count, i;

	mode = get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes, band);
	if (mode == NULL) {
		/* No channels supported in this band - use empty list */
		params->freqs = os_zalloc(sizeof(int));
		return;
	}

	params->freqs = os_calloc(mode->num_channels + 1, sizeof(int));
	if (params->freqs == NULL)
		return;
	for (count = 0, i = 0; i < mode->num_channels; i++) {
		if (mode->channels[i].flag & HOSTAPD_CHAN_DISABLED)
			continue;
		params->freqs[count++] = mode->channels[i].freq;
	}
}


static void wpa_setband_scan_freqs(struct wpa_supplicant *wpa_s,
				   struct wpa_driver_scan_params *params)
{
	if (wpa_s->hw.modes == NULL)
		return; /* unknown what channels the driver supports */

	if (params->freqs)
		return; /* already using a limited channel set */

#ifdef CONFIG_5G_SUPPORT
	if (wpa_s->setband == WPA_SETBAND_5G)
		wpa_setband_scan_freqs_list(wpa_s, HOSTAPD_MODE_IEEE80211A, params);
	else if (wpa_s->setband == WPA_SETBAND_2G)
#endif /* CONFIG_5G_SUPPORT */
		wpa_setband_scan_freqs_list(wpa_s, HOSTAPD_MODE_IEEE80211G, params);
}

extern void supp_scan_result_event(void *eloop_ctx, void *timeout_ctx);

#if 0	/* by Shingu 20160901 (Concurrent) */
#if defined(CONFIG_CONCURRENT) && defined(CONFIG_DISALLOW_CONCURRENT_SCAN)
static int is_other_iface_scan_ongoing(struct wpa_global *global, struct wpa_supplicant *wpa_s) {
	struct wpa_supplicant *iface;

	for (iface = global->ifaces; iface; iface = iface->next) {
		if(iface != wpa_s && iface->scan_ongoing) {
			da16x_warn_prt("[%s] Scan is on going for %s\n", __func__, iface->ifname);
			return 1;
		}
	}
	return 0;
}
#endif /* defined(CONFIG_CONCURRENT) && defined(CONFIG_DISALLOW_CONCURRENT_SCAN) */
#endif	/* 0 */

#ifdef FEATURE_SCAN_FREQ_ORDER_TOGGLE /* FC9000 Only */
void update_freqs_order_reversed(int num_freq,  int *freqs)
{
	int *freq_temp = (int *)os_calloc(num_freq, sizeof(int));

	if(freq_temp == NULL)
		return;

	for(int i = 0; i < num_freq  ; i++) {
		freq_temp[i] = freqs[i];
	}

	for(int i = 0 ; i < num_freq ; i++) {
		freqs[num_freq - i - 1] = freq_temp[i];
	}

	freqs[num_freq] = 0;

	os_free(freq_temp);
}
#endif /* FEATURE_SCAN_FREQ_ORDER_TOGGLE */  /* FC9000 Only */

#ifdef CONFIG_STA_COUNTRY_CODE
void update_freqs_with_country_code(struct wpa_supplicant *wpa_s, int **freqs)
{
	int i;
	int *freq_temp;
	struct wpa_freq_range_list ranges;
	unsigned int max_freq = wpa_s->conf->country_range.max;
	unsigned int min_freq = wpa_s->conf->country_range.min;
	int num_freq = 0;

#if 1	/* by Shingu 20170104 (JP Channel) */
	if (wpa_s->conf->country_range.max == 2484 &&
	    os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
			max_freq = 2472; /* Channel 14 selected on 11b only */
	}
#endif	/* 1*/

	ranges.range = &(wpa_s->conf->country_range);
	ranges.num = 1;

	if (*freqs) {
		freq_temp = *freqs;

		for (i = 0; freq_temp[i]; i++) {
			if (!freq_range_list_includes(&ranges, freq_temp[i])) {
				freq_temp[i] = 0;
			} else {
				num_freq++;
			}
			//PRINTF("[%s:%d] min_freq=%d max_freq=%d freq_temp[%d]=%d\n", __func__, __LINE__, min_freq, max_freq, i, freq_temp[i]);
		}
	} else {
		freq_temp = (int *)os_calloc(15, sizeof(int));

		if (!freq_temp) {
			return;
		}

		if (max_freq == 2484) {
			num_freq = 14;
		} else {
			num_freq = (max_freq - min_freq) / 5 + 1;
		}
		
		for (i = 0; i < num_freq; i++) {
			freq_temp[i] = min_freq + i * 5;
			//PRINTF("[%s:%d] min_freq=%d max_freq=%d freq_temp[%d]=%d\n", __func__, __LINE__, min_freq, max_freq, i, freq_temp[i]);
		}

		if (max_freq == 2484) {
			freq_temp[13] = 2484;
		}

		freq_temp[num_freq] = 0;

		//PRINTF("[%s:%d] min_freq=%d max_freq=%d freq_temp[%d]=%d\n", __func__, __LINE__, min_freq, max_freq, i, freq_temp[i]);
	}

#ifdef FEATURE_SCAN_FREQ_ORDER_TOGGLE
	if(wpa_s->reverse_scan_freq) {
		update_freqs_order_reversed(num_freq, freq_temp);
		wpa_s->reverse_scan_freq = 0;
	} else {
		wpa_s->reverse_scan_freq = 1;
	}
#endif /* FEATURE_SCAN_FREQ_ORDER_TOGGLE */

	*freqs = freq_temp;
}
#endif /* CONFIG_STA_COUNTRY_CODE */

#ifdef CONFIG_OWE
static void wpa_add_scan_ssid(struct wpa_supplicant *wpa_s,
			      struct wpa_driver_scan_params *params,
			      size_t max_ssids, const u8 *ssid, size_t ssid_len)
{
	unsigned int j;

	for (j = 0; j < params->num_ssids; j++) {
		if (params->ssids[j].ssid_len == ssid_len &&
		    params->ssids[j].ssid &&
		    os_memcmp(params->ssids[j].ssid, ssid, ssid_len) == 0)
			return; /* already in the list */
	}

	if (params->num_ssids + 1 > max_ssids) {
		wpa_printf(MSG_DEBUG, "Over max scan SSIDs for manual request");
		return;
	}

	wpa_printf(MSG_DEBUG, "Scan SSID (manual request): %s",
		   wpa_ssid_txt(ssid, ssid_len));

	params->ssids[params->num_ssids].ssid = ssid;
	params->ssids[params->num_ssids].ssid_len = ssid_len;
	params->num_ssids++;
}

static void wpa_add_owe_scan_ssid(struct wpa_supplicant *wpa_s,
				  struct wpa_driver_scan_params *params,
				  struct wpa_ssid *ssid, size_t max_ssids)
{
	struct wpa_bss *bss;

	if (!(ssid->key_mgmt & WPA_KEY_MGMT_OWE))
		return;

	wpa_printf(MSG_DEBUG, "OWE: Look for transition mode AP. ssid=%s",
		   wpa_ssid_txt(ssid->ssid, ssid->ssid_len));

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		const u8 *owe, *pos, *end;
		const u8 *owe_ssid;
		size_t owe_ssid_len;

		if (bss->ssid_len != ssid->ssid_len ||
		    os_memcmp(bss->ssid, ssid->ssid, ssid->ssid_len) != 0)
			continue;

		owe = wpa_bss_get_vendor_ie(bss, OWE_IE_VENDOR_TYPE);
		if (!owe || owe[1] < 4)
			continue;

		pos = owe + 6;
		end = owe + 2 + owe[1];

		/* Must include BSSID and ssid_len */
		if (end - pos < ETH_ALEN + 1)
			return;

		/* Skip BSSID */
		pos += ETH_ALEN;
		owe_ssid_len = *pos++;
		owe_ssid = pos;

		if ((size_t) (end - pos) < owe_ssid_len ||
		    owe_ssid_len > SSID_MAX_LEN)
			return;

		wpa_printf(MSG_DEBUG,
			   "OWE: scan_ssids: transition mode OWE ssid=%s",
			   wpa_ssid_txt(owe_ssid, owe_ssid_len));

		wpa_add_scan_ssid(wpa_s, params, max_ssids,
				  owe_ssid, owe_ssid_len);
		return;
	}

}
#endif /* CONFIG_OWE */


#ifdef	__SUPP_27_SUPPORT__
static void wpa_set_scan_ssids(struct wpa_supplicant *wpa_s,
			       struct wpa_driver_scan_params *params,
			       size_t max_ssids)
{
	unsigned int i;
	struct wpa_ssid *ssid;

	/*
	 * For devices with max_ssids greater than 1, leave the last slot empty
	 * for adding the wildcard scan entry.
	 */
	max_ssids = max_ssids > 1 ? max_ssids - 1 : max_ssids;

	for (i = 0; i < wpa_s->scan_id_count; i++) {
		unsigned int j;

		ssid = wpa_config_get_network(wpa_s->conf, wpa_s->scan_id[i]);
		if (!ssid || !ssid->scan_ssid)
			continue;
#ifdef CONFIG_OWE
		if (ssid->scan_ssid)
			wpa_add_scan_ssid(wpa_s, params, max_ssids,
					  ssid->ssid, ssid->ssid_len);
		/*
		 * Also add the SSID of the OWE BSS, to allow discovery of
		 * transition mode APs more quickly.
		 */
		wpa_add_owe_scan_ssid(wpa_s, params, ssid, max_ssids);
#else
		for (j = 0; j < params->num_ssids; j++) {
			if (params->ssids[j].ssid_len == ssid->ssid_len &&
			    params->ssids[j].ssid &&
			    os_memcmp(params->ssids[j].ssid, ssid->ssid,
				      ssid->ssid_len) == 0)
				break;
		}
		if (j < params->num_ssids)
			continue; /* already in the list */

		if (params->num_ssids + 1 > max_ssids) {
			wpa_printf_dbg(MSG_DEBUG,
				   "Over max scan SSIDs for manual request");
			break;
		}

		wpa_printf_dbg(MSG_DEBUG, "Scan SSID (manual request): %s",
			   wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
		params->ssids[params->num_ssids].ssid = ssid->ssid;
		params->ssids[params->num_ssids].ssid_len = ssid->ssid_len;
		params->num_ssids++;
#endif
	}

	wpa_s->scan_id_count = 0;
}
#endif	/* __SUPP_27_SUPPORT__ */


#ifdef	UNUSED_CODE
static int wpa_set_ssids_from_scan_req(struct wpa_supplicant *wpa_s,
				       struct wpa_driver_scan_params *params,
				       size_t max_ssids)
{
	unsigned int i;

	if (wpa_s->ssids_from_scan_req == NULL ||
	    wpa_s->num_ssids_from_scan_req == 0)
		return 0;

	if (wpa_s->num_ssids_from_scan_req > max_ssids) {
		wpa_s->num_ssids_from_scan_req = max_ssids;
		wpa_printf_dbg(MSG_DEBUG, "Over max scan SSIDs from scan req: %u",
			   (unsigned int) max_ssids);
	}

	for (i = 0; i < wpa_s->num_ssids_from_scan_req; i++) {
		params->ssids[i].ssid = wpa_s->ssids_from_scan_req[i].ssid;
		params->ssids[i].ssid_len =
			wpa_s->ssids_from_scan_req[i].ssid_len;
		wpa_hexdump_ascii(MSG_DEBUG, "specific SSID",
				  params->ssids[i].ssid,
				  params->ssids[i].ssid_len);
	}

	params->num_ssids = wpa_s->num_ssids_from_scan_req;
	wpa_s->num_ssids_from_scan_req = 0;
	return 1;
}
#endif	/* UNUSED_CODE */

#ifdef CONFIG_RECONNECT_OPTIMIZE
u8 fast_reconnect_scan = 0;
#endif /* CONFIG_RECONNECT_OPTIMIZE */

extern TaskHandle_t passive_scan_task;
void wpa_supplicant_scan(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct wpa_ssid *ssid = NULL;
	int ret;
	struct wpabuf *extra_ie = NULL;
	struct wpa_driver_scan_params params;
	struct wpa_driver_scan_params *scan_params = NULL;
	size_t max_ssids;
	enum wpa_states prev_state;
#ifdef	__SUPP_27_SUPPORT__
	int connect_without_scan = 0;
#endif /* __SUPP_27_SUPPORT__ */

	wpa_s->ignore_post_flush_scan_res = 0;

#if 0	/* by Shingu 20160901 (Concurrent) */
#if defined(CONFIG_CONCURRENT) && defined(CONFIG_DISALLOW_CONCURRENT_SCAN)
	if (get_run_mode() == STA_P2P_CONCURRENT_MODE) {
		if (is_other_iface_scan_ongoing(wpa_s->global, wpa_s)) {
			return;
		}
	}
#endif /* defined(CONFIG_CONCURRENT) && defined(CONFIG_DISALLOW_CONCURRENT_SCAN) */
#endif	/* 0 */

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	wpa_bss_flush(wpa_s);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

#ifdef CONFIG_CTRL_PNO
	if (wpa_s->pno || wpa_s->pno_sched_pending) {
		da16x_scan_prt("[%s] Skip scan - PNO is in progress\n", __func__);
		return;
	}
#endif /* CONFIG_CTRL_PNO */

	if (wpa_s->disconnected && wpa_s->scan_req == NORMAL_SCAN_REQ) {
		da16x_debug_prt("[%s] Disconnected - do not scan\n", __func__);

		wpa_supplicant_set_state(wpa_s, WPA_DISCONNECTED);
		return;
	}

	da16x_scan_prt("[%s] wpa_s->scanning=%d\n", __func__, wpa_s->scanning);

	if (wpa_s->scanning) {
		/*
		 * If we are already in scanning state, we shall reschedule the
		 * the incoming scan request.
		 */
		da16x_scan_prt("[%s] Already scanning - "
			"Reschedule the incoming scan req\n", __func__);

		wpa_supplicant_req_scan(wpa_s, wpa_s->scan_interval, 0);

		da16x_scan_prt("[%s] FINISH : #1\n", __func__);
		return;
	}

	if (!wpa_supplicant_enabled_networks(wpa_s) &&
	    wpa_s->scan_req == NORMAL_SCAN_REQ) {
		da16x_debug_prt("[%s] No enabled networks - do not scan\n",
			 __func__);
		wpa_supplicant_set_state(wpa_s, WPA_INACTIVE);

		da16x_scan_prt("[%s] FINISH : #2\n", __func__);
		return;
	}

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (wpa_s->conf->ap_scan != 0 &&
	    (wpa_s->drv_flags & WPA_DRIVER_FLAGS_WIRED)) {
		da16x_scan_prt("            [%s] Using wired authentication - "
			"overriding ap_scan configuration\n", __func__);

		wpa_s->conf->ap_scan = 0;
	}

	if (wpa_s->conf->ap_scan == 0) {
		wpa_supplicant_gen_assoc_event(wpa_s);

		da16x_scan_prt("            [%s] FINISH : #3\n", __func__);
		return;
	}
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

#ifdef	__SUPP_27_SUPPORT__
	if (wpa_s->scan_req != MANUAL_SCAN_REQ &&
		wpa_s->connect_without_scan) {
		connect_without_scan = 1;
		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
			if (ssid == wpa_s->connect_without_scan)
				break;
		}
	}
#endif /* __SUPP_27_SUPPORT__ */

#ifdef	CONFIG_P2P
	if (wpas_p2p_in_progress(wpa_s)) {
		da16x_scan_prt("            [%s] Delay station mode scan "
			"while P2P operation is in progress\n", __func__);

		wpa_supplicant_req_scan(wpa_s, 5, 0);

		da16x_scan_prt("            [%s] FINISH : #4\n", __func__);

		return;
	}
#endif	/* CONFIG_P2P */

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (wpa_s->conf->ap_scan == 2)
		max_ssids = 1;
	else
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	{
		max_ssids = wpa_s->max_scan_ssids;
		if (max_ssids > WPAS_MAX_SCAN_SSIDS)
			max_ssids = WPAS_MAX_SCAN_SSIDS;
	}

	wpa_s->last_scan_req = wpa_s->scan_req;
	wpa_s->scan_req = NORMAL_SCAN_REQ;

#ifdef	__SUPP_27_SUPPORT__
	if (connect_without_scan) {
		wpa_s->connect_without_scan = NULL;
		if (ssid) {
			da16x_scan_prt("[%s] Start a pre-selected network "
				   "without scan step\n", __func__);
			wpa_supplicant_associate(wpa_s, NULL, ssid);
			return;
		}
	}
#endif /* __SUPP_27_SUPPORT__ */

	os_memset(&params, 0, sizeof(params));

#ifdef CONFIG_RECONNECT_OPTIMIZE
	fast_reconnect_scan = 0;
#endif /* CONFIG_RECONNECT_OPTIMIZE */

	prev_state = wpa_s->wpa_state;
	if (wpa_s->wpa_state == WPA_DISCONNECTED ||
	    wpa_s->wpa_state == WPA_INACTIVE)
		wpa_supplicant_set_state(wpa_s, WPA_SCANNING);

	/*
	 * If autoscan has set its own scanning parameters
	 */
	if (wpa_s->autoscan_params != NULL) {
		scan_params = wpa_s->autoscan_params;
		goto scan;
	}

	if (wpa_s->last_scan_req != MANUAL_SCAN_REQ &&
	    wpa_s->connect_without_scan) {
		for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
			if (ssid == wpa_s->connect_without_scan)
				break;
		}

		wpa_s->connect_without_scan = NULL;

		if (ssid) {
			da16x_scan_prt("[%s] Start a pre-selected "
				"network without scan step\n", __func__);

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
			wpa_bss_flush(wpa_s);
#endif /* CONFIG_REUSED_UMAC_BSS_LIST */

			wpa_supplicant_associate(wpa_s, NULL, ssid);

			da16x_scan_prt("[%s] FINISH : #5\n", __func__);
			return;
		}
	}

#ifdef CONFIG_P2P
	if ((wpa_s->p2p_in_provisioning || wpa_s->show_group_started) &&
	    wpa_s->go_params) {
		da16x_scan_prt("[%s] P2P : Use specific SSID for scan during P2P "
			"group formation (p2p_in_provisioning=%d "
			"show_group_started=%d)\n",
				__func__,
				wpa_s->p2p_in_provisioning,
				wpa_s->show_group_started);

		params.ssids[0].ssid = wpa_s->go_params->ssid;
		params.ssids[0].ssid_len = wpa_s->go_params->ssid_len;
		params.num_ssids = 1;

		goto ssid_list_set;
	}

#ifdef	CONFIG_P2P_OPTION
	if (wpa_s->p2p_in_invitation) {
		if (wpa_s->current_ssid) {
			da16x_scan_prt("[%s] P2P: Use specific SSID for scan "
				"during invitation\n", __func__);

			params.ssids[0].ssid = wpa_s->current_ssid->ssid;
			params.ssids[0].ssid_len =
				wpa_s->current_ssid->ssid_len;
			params.num_ssids = 1;
		} else {
			da16x_scan_prt("[%s] P2P: No specific SSID known for scan "
				"during invitation\n", __func__);
		}
		goto ssid_list_set;
	}
#endif	/* CONFIG_P2P_OPTION */
#endif	/* CONFIG_P2P */

	/* Find the starting point from which to continue scanning */
	ssid = wpa_s->conf->ssid;
	if (wpa_s->prev_scan_ssid != WILDCARD_SSID_SCAN) {
		while (ssid) {
			if (ssid == wpa_s->prev_scan_ssid) {
				ssid = ssid->next;
				break;
			}
			ssid = ssid->next;
		}
	}

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	if (wpa_s->last_scan_req != MANUAL_SCAN_REQ && wpa_s->conf->ap_scan == 2) {
		wpa_s->connect_without_scan = NULL;
		wpa_s->prev_scan_wildcard = 0;
		wpa_supplicant_assoc_try(wpa_s, ssid);
		da16x_scan_prt("            [%s] FINISH : #6\n", __func__);
		return;
	} else if (wpa_s->conf->ap_scan == 2) {
		/*
		 * User-initiated scan request in ap_scan == 2; scan with
		 * wildcard SSID.
		 */
		ssid = NULL;
	} else
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

	if (wpa_s->reattach && wpa_s->current_ssid != NULL) {
		/*
		 * Perform single-channel single-SSID scan for
		 * reassociate-to-same-BSS operation.
		 */
		/* Setup SSID */
		ssid = wpa_s->current_ssid;

		da16x_ascii_dump("Scan SSID", ssid->ssid, ssid->ssid_len);

		params.ssids[0].ssid = ssid->ssid;
		params.ssids[0].ssid_len = ssid->ssid_len;
		params.num_ssids = 1;

		/*
		 * Allocate memory for frequency array, allocate one extra
		 * slot for the zero-terminator.
		 */
		params.freqs = (int *)os_malloc(sizeof(int) * 2);
		if (params.freqs == NULL) {
			da16x_scan_prt("[%s] Memory allocation failed\n", __func__);
			return;
		}
		params.freqs[0] = wpa_s->assoc_freq;
		params.freqs[1] = 0;

		/*
		 * Reset the reattach flag so that we fall back to full scan if
		 * this scan fails.
		 */
		wpa_s->reattach = 0;
	} else {
		struct wpa_ssid *start = ssid, *tssid;
		int freqs_set = 0;
		if (ssid == NULL && max_ssids > 1)
			ssid = wpa_s->conf->ssid;

		while (ssid) {
#if 0 /* Debug */
			PRINTF(ANSI_BCOLOR_YELLOW "[%s] %d\n"
			"prev_scan_wildcard=%d\n"
			"scan_req=%d\n"
			"last_scan_req=%d\n"
			"scan_runs=%d\n"
			"reverse_scan_freq=%d\n"
			"manual_scan_passive=%d\n"
			"manual_scan_use_id=%d\n"
			"manual_scan_only_new=%d\n"
			"manual_scan_promisc=%d\n"
			"own_scan_requested=%d\n"
			"own_scan_running=%d\n"
			"clear_driver_scan_cache=%d\n"
			"manual_scan_id=%d\n"
			"scan_interval=%d\n"
			"normal_scans=%d\n"
			"scan_for_connection=%d\n"
			"params.num_ssids=%d\n"
			 ANSI_BCOLOR_DEFULT"\n",
			 __func__, __LINE__,
			wpa_s->prev_scan_wildcard, 
			wpa_s->scan_req,
			wpa_s->last_scan_req,
			wpa_s->scan_runs,
			wpa_s->reverse_scan_freq,
			wpa_s->manual_scan_passive,
			wpa_s->manual_scan_use_id,
			wpa_s->manual_scan_only_new,
			wpa_s->manual_scan_promisc,
			wpa_s->own_scan_requested,
			wpa_s->own_scan_running,
			wpa_s->clear_driver_scan_cache,
			wpa_s->manual_scan_id,
			wpa_s->scan_interval,
			wpa_s->normal_scans,
			wpa_s->scan_for_connection,
			params.num_ssids
			);
#endif /* Debug */

			if (!wpas_network_disabled(wpa_s, ssid) && ssid->mode < 2 /* Hidden ssid or reassoc_freq */
				&& (ssid->scan_ssid
#ifdef CONFIG_RECONNECT_OPTIMIZE
					|| wpa_s->reassoc_freq && wpa_s->reassoc_try <= 3
#endif /* CONFIG_RECONNECT_OPTIMIZE */
				) && wpa_s->last_scan_req < MANUAL_SCAN_REQ
			) {
				da16x_ascii_dump("Scan SSID ",
						ssid->ssid, ssid->ssid_len);

				params.ssids[params.num_ssids].ssid =
					ssid->ssid;
				params.ssids[params.num_ssids].ssid_len =
					ssid->ssid_len;

#if 0 /* Debug */
				PRINTF(ANSI_COLOR_LIGHT_GREEN "[%s] %d\n"
				"prev_scan_wildcard=%d\n"
				"scan_req=%d\n"
				"last_scan_req=%d\n"
				"scan_runs=%d\n"
				"reverse_scan_freq=%d\n"
				"manual_scan_passive=%d\n"
				"manual_scan_use_id=%d\n"
				"manual_scan_only_new=%d\n"
				"manual_scan_promisc=%d\n"
				"own_scan_requested=%d\n"
				"own_scan_running=%d\n"
				"clear_driver_scan_cache=%d\n"
				"manual_scan_id=%d\n"
				"scan_interval=%d\n"
				"normal_scans=%d\n"
				"scan_for_connection=%d\n"
				"params.num_ssids=%d\n"
				 ANSI_BCOLOR_DEFULT"\n",
				 __func__, __LINE__,
				wpa_s->prev_scan_wildcard, 
				wpa_s->scan_req,
				wpa_s->last_scan_req,
				wpa_s->scan_runs,
				wpa_s->reverse_scan_freq,
				wpa_s->manual_scan_passive,
				wpa_s->manual_scan_use_id,
				wpa_s->manual_scan_only_new,
				wpa_s->manual_scan_promisc,
				wpa_s->own_scan_requested,
				wpa_s->own_scan_running,
				wpa_s->clear_driver_scan_cache,
				wpa_s->manual_scan_id,
				wpa_s->scan_interval,
				wpa_s->normal_scans,
				wpa_s->scan_for_connection,
				params.num_ssids
				);
#endif /* Debug */

#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
				if (wpa_s->scan_for_connection == 0 || wpa_s->last_scan_req > INITIAL_SCAN_REQ)
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
				{
					params.num_ssids++;
				}

				if ((size_t)params.num_ssids + 1 >= max_ssids)
					break;
			}
#ifdef CONFIG_OWE
			if (!wpas_network_disabled(wpa_s, ssid)) {
				/*
				 * Also add the SSID of the OWE BSS, to allow
				 * discovery of transition mode APs more
				 * quickly.
				 */
				wpa_add_owe_scan_ssid(wpa_s, &params, ssid,
							  max_ssids);
			}
#endif
			ssid = ssid->next;
			if (ssid == start)
				break;
			if (ssid == NULL && max_ssids > 1 &&
			    start != wpa_s->conf->ssid)
				ssid = wpa_s->conf->ssid;
		}

		for (tssid = wpa_s->conf->ssid; tssid; tssid = tssid->next) {
			if (wpas_network_disabled(wpa_s, tssid))
				continue;
			if ((params.freqs || !freqs_set) && tssid->scan_freq) {
				int_array_concat(&params.freqs,
						 tssid->scan_freq);
			} else {
				os_free(params.freqs);
				params.freqs = NULL;
			}
			freqs_set = 1;
		}
		int_array_sort_unique(params.freqs);
	}

	if (ssid && max_ssids == 1) {
		/*
		 * If the driver is limited to 1 SSID at a time interleave
		 * wildcard SSID scans with specific SSID scans to avoid
		 * waiting a long time for a wildcard scan.
		 */
		if (!wpa_s->prev_scan_wildcard) {
			params.ssids[0].ssid = NULL;
			params.ssids[0].ssid_len = 0;
			wpa_s->prev_scan_wildcard = 1;

			da16x_scan_prt("            [%s] Starting AP scan for "
				"wildcard SSID (Interleave with specific)\n",
				__func__);
		} else {
			wpa_s->prev_scan_ssid = ssid;
			wpa_s->prev_scan_wildcard = 0;

			da16x_scan_prt("            [%s] Starting AP scan for "
				"specific SSID: %s\n",
				__func__,
				wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
		}
	} else if (ssid) {
		/* max_ssids > 1 */

#ifdef CONFIG_RECONNECT_OPTIMIZE
		if (ssid->scan_ssid == 0 && !(wpa_s->reassoc_freq && wpa_s->reassoc_try <= 3)) {
			wpa_s->prev_scan_ssid = ssid;

			da16x_scan_prt("            [%s] Include wildcard SSID in "
				"the scan request\n", __func__);

#if 0 /* Debug */
			PRINTF(ANSI_COLOR_LIGHT_MAGENTA"[%s] %d\n"
				"prev_scan_wildcard=%d\n"
				"scan_req=%d\n"
				"last_scan_req=%d\n"
				"scan_runs=%d\n"
				"reverse_scan_freq=%d\n"
				"manual_scan_passive=%d\n"
				"manual_scan_use_id=%d\n"
				"manual_scan_only_new=%d\n"
				"manual_scan_promisc=%d\n"
				"own_scan_requested=%d\n"
				"own_scan_running=%d\n"
				"clear_driver_scan_cache=%d\n"
				"manual_scan_id=%d\n"
				"scan_interval=%d\n"
				"normal_scans=%d\n"
				"scan_for_connection=%d\n"
				"params.num_ssids=%d\n"
				 ANSI_BCOLOR_DEFULT"\n",
				 __func__, __LINE__,
				wpa_s->prev_scan_wildcard, 
				wpa_s->scan_req,
				wpa_s->last_scan_req,
				wpa_s->scan_runs,
				wpa_s->reverse_scan_freq,
				wpa_s->manual_scan_passive,
				wpa_s->manual_scan_use_id,
				wpa_s->manual_scan_only_new,
				wpa_s->manual_scan_promisc,
				wpa_s->own_scan_requested,
				wpa_s->own_scan_running,
				wpa_s->clear_driver_scan_cache,
				wpa_s->manual_scan_id,
				wpa_s->scan_interval,
				wpa_s->normal_scans,
				wpa_s->scan_for_connection,
				params.num_ssids
				);
#endif /* Debug */


#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
			if (wpa_s->scan_for_connection == 0 || wpa_s->last_scan_req > INITIAL_SCAN_REQ)
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
			{ 
				params.num_ssids++;
			}
		}
#endif /* CONFIG_RECONNECT_OPTIMIZE */
	} else if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
		   wpa_s->manual_scan_passive && params.num_ssids == 0) {
		da16x_scan_prt("            [%s] Use passive scan based on manual "
			"request\n", __func__);
	} else {
#if 0 /* Debug */
		PRINTF(ANSI_BCOLOR_CYAN "[%s] %d\n"
			"prev_scan_wildcard=%d\n"
			"scan_req=%d\n"
			"last_scan_req=%d\n"
			"scan_runs=%d\n"
			"reverse_scan_freq=%d\n"
			"manual_scan_passive=%d\n"
			"manual_scan_use_id=%d\n"
			"manual_scan_only_new=%d\n"
			"manual_scan_promisc=%d\n"
			"own_scan_requested=%d\n"
			"own_scan_running=%d\n"
			"clear_driver_scan_cache=%d\n"
			"manual_scan_id=%d\n"
			"scan_interval=%d\n"
			"normal_scans=%d\n"
			"scan_for_connection=%d\n"
			"params.num_ssids=%d\n"
			 ANSI_BCOLOR_DEFULT"\n",
			 __func__, __LINE__,
			wpa_s->prev_scan_wildcard, 
			wpa_s->scan_req,
			wpa_s->last_scan_req,
			wpa_s->scan_runs,
			wpa_s->reverse_scan_freq,
			wpa_s->manual_scan_passive,
			wpa_s->manual_scan_use_id,
			wpa_s->manual_scan_only_new,
			wpa_s->manual_scan_promisc,
			wpa_s->own_scan_requested,
			wpa_s->own_scan_running,
			wpa_s->clear_driver_scan_cache,
			wpa_s->manual_scan_id,
			wpa_s->scan_interval,
			wpa_s->normal_scans,
			wpa_s->scan_for_connection,
			params.num_ssids
			);
#endif /* Debug */


		wpa_s->prev_scan_ssid = WILDCARD_SSID_SCAN;
		params.num_ssids++;

		da16x_scan_prt("            [%s] Starting AP scan for wildcard\n",
			__func__);
	}
#ifdef CONFIG_P2P
ssid_list_set:
#endif /* CONFIG_P2P */

	wpa_supplicant_optimize_freqs(wpa_s, &params);
	extra_ie = wpa_supplicant_extra_ies(wpa_s);

	if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
	    wpa_s->manual_scan_only_new)
		params.only_new_results = 1;

	if (wpa_s->last_scan_req == MANUAL_SCAN_REQ && params.freqs == NULL &&
	    wpa_s->manual_scan_freqs) {
		da16x_scan_prt("            [%s] Limit manual scan to specified \n"
			"channels\n", __func__);

		params.freqs = wpa_s->manual_scan_freqs;
		wpa_s->manual_scan_freqs = NULL;
	}

	if (params.freqs == NULL && wpa_s->next_scan_freqs) {
		da16x_scan_prt("            [%s] Optimize scan based on previously "
			"generated frequency list\n", __func__);

		params.freqs = wpa_s->next_scan_freqs;
	} else
		os_free(wpa_s->next_scan_freqs);

	wpa_s->next_scan_freqs = NULL;
	wpa_setband_scan_freqs(wpa_s, &params);

	/* See if user specified frequencies. If so, scan only those. */
	if (wpa_s->conf->freq_list && !params.freqs) {
		da16x_scan_prt("            [%s] Optimize scan based on "
			"conf->freq_list\n", __func__);

		int_array_concat(&params.freqs, wpa_s->conf->freq_list);
	}

	/* Use current associated channel? */
	if (wpa_s->conf->scan_cur_freq && !params.freqs) {
		unsigned int num = wpa_s->num_multichan_concurrent;

		params.freqs = os_calloc(num + 1, sizeof(int));
		if (params.freqs) {
			num = get_shared_radio_freqs(wpa_s, params.freqs, num);
			if (num > 0) {
				da16x_scan_prt("            [%s] Scan only the "
					"current operating channels since "
					"scan_cur_freq is enabled\n", __func__);
			} else {
				os_free(params.freqs);
				params.freqs = NULL;
			}
		}
	}

#ifdef SUPPORT_SELECT_NETWORK_FILTER
	if(wpa_s->last_scan_req != MANUAL_SCAN_REQ)
#endif /* SUPPORT_SELECT_NETWORK_FILTER */
	params.filter_ssids = wpa_supplicant_build_filter_ssids(
					wpa_s->conf, &params.num_filter_ssids
#ifdef SUPPORT_SELECT_NETWORK_FILTER
					, 0
#endif /* SUPPORT_SELECT_NETWORK_FILTER */
					);

	if (extra_ie) {
		params.extra_ies = wpabuf_head(extra_ie);
		params.extra_ies_len = wpabuf_len(extra_ie);
	}

#ifdef CONFIG_P2P
#ifdef	CONFIG_P2P_OPTION
	if (wpa_s->p2p_in_provisioning || wpa_s->p2p_in_invitation ||
	    (wpa_s->show_group_started && wpa_s->go_params))
#else	/* CONFIG_P2P_OPTION */
	if (wpa_s->p2p_in_provisioning ||
	    (wpa_s->show_group_started && wpa_s->go_params))
#endif	/* CONFIG_P2P_OPTION */
	{
		/*
		 * The interface may not yet be in P2P mode, so we have to
		 * explicitly request P2P probe to disable CCK rates.
		 */
		params.p2p_probe = 1;
	}
#endif /* CONFIG_P2P */

    if ((passive_scan_feature_flag == pdTRUE) && (passive_scan_task != NULL))
    {
        if (wpa_s->manual_scan_passive)
        {
            params.passive_scan_flag = 1;
            if (wpa_s->passive_scan_duration)
            {
                params.passive_duration = wpa_s->passive_scan_duration;
    	    }
        } else {
            params.passive_duration = 0;
        }
    }
	scan_params = &params;

scan:
#ifdef CONFIG_P2P
	/*
	 * If the driver does not support multi-channel concurrency and a
	 * virtual interface that shares the same radio with the wpa_s interface
	 * is operating there may not be need to scan other channels apart from
	 * the current operating channel on the other virtual interface. Filter
	 * out other channels in case we are trying to find a connection for a
	 * station interface when we are not configured to prefer station
	 * connection and a concurrent operation is already in process.
	 */
	if (wpa_s->scan_for_connection &&
	    wpa_s->last_scan_req == NORMAL_SCAN_REQ &&
	    !scan_params->freqs && !params.freqs &&
#ifdef	CONFIG_P2P_UNUSED_CMD
	    wpas_is_p2p_prioritized(wpa_s) &&
#endif	/* CONFIG_P2P_UNUSED_CMD */
	    wpa_s->p2p_group_interface == NOT_P2P_GROUP_INTERFACE &&
	    non_p2p_network_enabled(wpa_s)) {
		unsigned int num = wpa_s->num_multichan_concurrent;

		params.freqs = os_calloc(num + 1, sizeof(int));
		if (params.freqs) {
			num = get_shared_radio_freqs(wpa_s, params.freqs, num);
			if (num > 0 && num == wpa_s->num_multichan_concurrent) {
				da16x_scan_prt("            [%s] Scan only the "
					"current operating channels since "
					"all channels are already used\n",
					__func__);
			} else {
				os_free(params.freqs);
				params.freqs = NULL;
			}
		}
	}
#endif /* CONFIG_P2P */

#ifdef CONFIG_STA_COUNTRY_CODE
	update_freqs_with_country_code(wpa_s, &params.freqs);
#endif /* CONFIG_STA_COUNTRY_CODE */

#ifdef	CONFIG_RECONNECT_OPTIMIZE  
	/** When performing a scan in the connected state by CLI Command & Setup  
	 * skip the fast scan & do normal scan. 
	 */
#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpa_s->conf->roam_state != ROAM_SCANNING) {
#endif	/* CONFIG_SIMPLE_ROAMING */
	//if (wpa_s->reassoc_freq && (wpa_s->scan_for_connection || wpa_s->wpa_state != WPA_COMPLETED)) {
	if (wpa_s->reassoc_freq && wpa_s->scan_for_connection) {
		wpa_s->reassoc_try++;
		if (wpa_s->reassoc_try <= 3) {
			if (params.freqs != NULL)
				os_free(params.freqs);
			params.freqs = (int *)os_malloc(sizeof(int) * 2);
			if (params.freqs == NULL) {
				da16x_scan_prt("[%s] Memory allocation failed\n",
						__func__);
				return;
			}
			da16x_notice_prt("Fast scan, freq=%d, num_ssids=%d\n", wpa_s->reassoc_freq, params.num_ssids);
			params.freqs[0] = wpa_s->reassoc_freq;
			params.freqs[1] = 0;
			fast_reconnect_scan = 1;
		}
	}
#ifdef	CONFIG_SIMPLE_ROAMING
	}
#endif	/* CONFIG_SIMPLE_ROAMING */

#endif	/* CONFIG_RECONNECT_OPTIMIZE */

#ifdef FAST_CONN_ASSOC_CH	/* Fast connect for sleep mode 1. */
	if(!chk_dpm_wakeup() /* not dpm wakeup */
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		&& wpa_s->conf->ap_scan != 2	/* not AP Scan */
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
		&& wpa_s->conf->scan_cur_freq /* scan freq is written in nvram */
		&& wpa_s->scan_req != MANUAL_SCAN_REQ) {
		wpa_s->reassoc_try++;
		if (wpa_s->reassoc_try <= 3) {
			if(wpa_s->conf->scan_cur_freq > 15 || wpa_s->conf->scan_cur_freq < 0) {
				da16x_notice_prt("[%s] Cur Scan ch Failed in NVRAM\n",__func__);
				return;
			}
			else {	
				int cur_freq = wpa_s->conf->scan_cur_freq;
				if (params.freqs != NULL)
					os_free(params.freqs);
				params.freqs = (int *)os_malloc(sizeof(int) * 2);
				if (params.freqs == NULL) {
					da16x_scan_prt("[%s] Memory allocation failed\n",
							__func__);
					return;
				}
				params.num_ssids = 1;
				if (cur_freq == 14)
					params.freqs[0] = 2484;
				else 
					params.freqs[0] = 2407 + cur_freq * 5;
			
				params.freqs[1] = 0;
				fast_reconnect_scan = 1;
				da16x_notice_prt("Fast scan(Manual=%d), freq=%d, num_ssids=%d\n", wpa_s->scan_req , params.freqs[0], params.num_ssids);	
			}
		}
	}
#endif /* FAST_CONN_ASSOC_CH */

#ifdef CONFIG_SCAN_WITH_BSSID
	if (wpa_s->manual_scan_promisc && !is_zero_ether_addr(wpa_s->scanbssid))
	{
		scan_params->bssid_scan_flag = 1;
		memcpy(scan_params->scanbssid , wpa_s->scanbssid, ETH_ALEN);
		//da16x_notice_prt("Scan with BSSID %x:%x:%x:%x:%x:%x\n",
		//				scan_params->scanbssid[0], scan_params->scanbssid[1], scan_params->scanbssid[2],
		//				scan_params->scanbssid[3], scan_params->scanbssid[4], scan_params->scanbssid[5]);			
	}
#endif

#ifdef CONFIG_SCAN_WITH_DIR_SSID
	if (wpa_s->manual_scan_promisc && wpa_s->ssids_from_scan_req && wpa_s->num_ssids_from_scan_req != 0)
	{
		int ssid_index;

		for (ssid_index = 0; ssid_index < wpa_s->num_ssids_from_scan_req; ssid_index++)
		{
			scan_params->ssids[ssid_index].ssid = wpa_s->ssids_from_scan_req[ssid_index].ssid;
			scan_params->ssids[ssid_index].ssid_len = wpa_s->ssids_from_scan_req[ssid_index].ssid_len;
			//da16x_notice_prt("[%s] %dth SSID scan, ssid = %s\n", __func__, ssid_index, scan_params->ssids[ssid_index].ssid);
		}

		scan_params->dir_ssid_scan_flag = 1;
		scan_params->num_ssids = wpa_s->num_ssids_from_scan_req;
		wpa_s->num_ssids_from_scan_req = 0;
	}
	else
	{
		scan_params->dir_ssid_scan_flag = 0;
	}
#endif

	da16x_scan_prt("[%s] call TRIGGER_SCAN\n", __func__);

	ret = wpa_supplicant_trigger_scan(wpa_s, scan_params);

	if (ret && wpa_s->last_scan_req == MANUAL_SCAN_REQ && params.freqs &&
	    !wpa_s->manual_scan_freqs) {
		/* Restore manual_scan_freqs for the next attempt */
		wpa_s->manual_scan_freqs = params.freqs;
		params.freqs = NULL;
	}

	wpabuf_free(extra_ie);
	os_free(params.freqs);
	os_free(params.filter_ssids);

	if (ret) {
		da16x_err_prt("[%s] Failed to init AP scan\n", __func__);

		if (prev_state != wpa_s->wpa_state)
			wpa_supplicant_set_state(wpa_s, prev_state);

		xEventGroupSetBits(da16x_sp_event_group, DA16X_SCAN_RESULTS_FAIL_EV);

		/* Restore scan_req since we will try to scan again */
		wpa_s->scan_req = wpa_s->last_scan_req;
		wpa_supplicant_req_scan(wpa_s, 1, 0);
	} else {
		wpa_s->scan_for_connection = 0;
	}
}


#if 0 /* unused for DA16xxx supp 2.7 */
void wpa_supp_update_scan_int(struct wpa_supplicant *wpa_s, int sec)
{
	struct os_reltime remaining, new_int;
	int cancelled;

	cancelled = da16x_eloop_cancel_timeout_one(wpa_supplicant_scan, wpa_s, NULL,
					     &remaining);

	new_int.sec = sec;
	new_int.usec = 0;
	if (cancelled && os_reltime_before(&remaining, &new_int)) {
		new_int.sec = remaining.sec;
		new_int.usec = remaining.usec;
	}

	if (cancelled) {
		da16x_eloop_register_timeout(new_int.sec, new_int.usec,
				       wpa_supplicant_scan, wpa_s, NULL);
	}
	wpa_s->scan_interval = sec;
}
#endif /* unused for DA16xxx supp 2.7 */


/**
 * wpa_supplicant_req_scan - Schedule a scan for neighboring access points
 * @wpa_s: Pointer to wpa_supplicant data
 * @sec: Number of seconds after which to scan
 * @usec: Number of microseconds after which to scan
 *
 * This function is used to schedule a scan for neighboring access points after
 * the specified time.
 */
void wpa_supplicant_req_scan(struct wpa_supplicant *wpa_s, int sec, int usec)
{
	int res;

#ifdef CONFIG_P2P
	if (wpa_s->p2p_mgmt) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Ignore scan request (%d.%06d sec) on p2p_mgmt interface",
			sec, usec);
		return;
	}
#endif /* CONFIG_P2P */

#ifndef ENABLE_SCAN_ON_AP_MODE
	if (get_run_mode() == AP_ONLY_MODE) {
		da16x_scan_prt("[%s] STA interface is not enabled."
			"Do not scan\n", __func__, sec, usec);
		return;
	}
#endif /* ! ENABLE_SCAN_ON_AP_MODE */

	res = da16x_eloop_deplete_timeout(sec, usec, wpa_supplicant_scan, wpa_s, NULL);
	if (res == 1) {
		da16x_scan_prt("[%s] Rescheduling scan request : "
            "%d.%06d sec\n", __func__, sec, usec);
	} else if (res == 0) {
		da16x_scan_prt("[%s] Ignore new scan request for "
            "%d.%06d sec since an earlier request is "
            "scheduled to trigger sooner\n",
            __func__, sec, usec);
	} else {
		da16x_scan_prt("[%s] Setting scan request: %d.%06d sec",
			__func__, sec, usec);
		da16x_eloop_register_timeout(sec, usec, wpa_supplicant_scan, wpa_s, NULL);
	}
}


#if defined ( CONFIG_SCHED_SCAN )
/**
 * wpa_supplicant_delayed_sched_scan - Request a delayed scheduled scan
 * @wpa_s: Pointer to wpa_supplicant data
 * @sec: Number of seconds after which to scan
 * @usec: Number of microseconds after which to scan
 * Returns: 0 on success or -1 otherwise
 *
 * This function is used to schedule periodic scans for neighboring
 * access points after the specified time.
 */
int wpa_supplicant_delayed_sched_scan(struct wpa_supplicant *wpa_s,
				      int sec, int usec)
{
	if (!wpa_s->sched_scan_supported)
		return -1;

	da16x_eloop_register_timeout(sec, usec,
			       wpa_supplicant_delayed_sched_scan_timeout,
			       wpa_s, NULL);

	return 0;
}
#endif	// CONFIG_SCAN_WORK


#if defined ( CONFIG_SRP )
static void
wpa_scan_set_relative_rssi_params(struct wpa_supplicant *wpa_s,
				  struct wpa_driver_scan_params *params)
{
	if (wpa_s->wpa_state != WPA_COMPLETED ||
	    !(wpa_s->drv_flags & WPA_DRIVER_FLAGS_SCHED_SCAN_RELATIVE_RSSI) ||
	    wpa_s->srp.relative_rssi_set == 0)
		return;

	params->relative_rssi_set = 1;
	params->relative_rssi = wpa_s->srp.relative_rssi;

	if (wpa_s->srp.relative_adjust_rssi == 0)
		return;

#ifdef CONFIG_BAND_5GHZ
	params->relative_adjust_band = wpa_s->srp.relative_adjust_band;
#endif /* CONFIG_BAND_5GHZ */
	params->relative_adjust_rssi = wpa_s->srp.relative_adjust_rssi;
}
#endif // CONFIG_SRP


#if defined ( CONFIG_SCHED_SCAN )
/**
 * wpa_supplicant_req_sched_scan - Start a periodic scheduled scan
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 is sched_scan was started or -1 otherwise
 *
 * This function is used to schedule periodic scans for neighboring
 * access points repeating the scan continuously.
 */
int wpa_supplicant_req_sched_scan(struct wpa_supplicant *wpa_s)
{
	struct wpa_driver_scan_params params;
	struct wpa_driver_scan_params *scan_params;
	enum wpa_states prev_state;
	struct wpa_ssid *ssid = NULL;
	struct wpabuf *extra_ie = NULL;
	int ret;
	unsigned int max_sched_scan_ssids;
	int wildcard = 0;
	int need_ssids;
	struct sched_scan_plan scan_plan;

	if (!wpa_s->sched_scan_supported)
		return -1;

	if (wpa_s->max_sched_scan_ssids > WPAS_MAX_SCAN_SSIDS)
		max_sched_scan_ssids = WPAS_MAX_SCAN_SSIDS;
	else
		max_sched_scan_ssids = wpa_s->max_sched_scan_ssids;
	if (max_sched_scan_ssids < 1
#ifdef CONFIG_SCAN_OFFLOAD
		|| wpa_s->conf->disable_scan_offload
#endif /* CONFIG_SCAN_OFFLOAD */
		)
		return -1;

	wpa_s->sched_scan_stop_req = 0;

	if (wpa_s->sched_scanning) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Already sched scanning");
		return 0;
	}

	need_ssids = 0;
	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (!wpas_network_disabled(wpa_s, ssid) && !ssid->scan_ssid) {
			/* Use wildcard SSID to find this network */
			wildcard = 1;
		} else if (!wpas_network_disabled(wpa_s, ssid) &&
			   ssid->ssid_len)
			need_ssids++;

#ifdef CONFIG_WPS
		if (!wpas_network_disabled(wpa_s, ssid) &&
		    ssid->key_mgmt == WPA_KEY_MGMT_WPS) {
			/*
			 * Normal scan is more reliable and faster for WPS
			 * operations and since these are for short periods of
			 * time, the benefit of trying to use sched_scan would
			 * be limited.
			 */
			wpa_dbg(wpa_s, MSG_DEBUG, "Use normal scan instead of "
				"sched_scan for WPS");
			return -1;
		}
#endif /* CONFIG_WPS */
	}
	if (wildcard)
		need_ssids++;

	if (wpa_s->normal_scans < 3 &&
	    (need_ssids <= wpa_s->max_scan_ssids ||
	     wpa_s->max_scan_ssids >= (int) max_sched_scan_ssids)) {
		/*
		 * When normal scan can speed up operations, use that for the
		 * first operations before starting the sched_scan to allow
		 * user space sleep more. We do this only if the normal scan
		 * has functionality that is suitable for this or if the
		 * sched_scan does not have better support for multiple SSIDs.
		 */
		wpa_dbg(wpa_s, MSG_DEBUG, "Use normal scan instead of "
			"sched_scan for initial scans (normal_scans=%d)",
			wpa_s->normal_scans);
		return -1;
	}

	os_memset(&params, 0, sizeof(params));

	/* If we can't allocate space for the filters, we just don't filter */
	params.filter_ssids = os_calloc(wpa_s->max_match_sets,
					sizeof(struct wpa_driver_scan_filter));

	prev_state = wpa_s->wpa_state;
	if (wpa_s->wpa_state == WPA_DISCONNECTED ||
	    wpa_s->wpa_state == WPA_INACTIVE)
		wpa_supplicant_set_state(wpa_s, WPA_SCANNING);

	if (wpa_s->autoscan_params != NULL) {
		scan_params = wpa_s->autoscan_params;
		goto scan;
	}

	/* Find the starting point from which to continue scanning */
	ssid = wpa_s->conf->ssid;
	if (wpa_s->prev_sched_ssid) {
		while (ssid) {
			if (ssid == wpa_s->prev_sched_ssid) {
				ssid = ssid->next;
				break;
			}
			ssid = ssid->next;
		}
	}

	if (!ssid || !wpa_s->prev_sched_ssid) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Beginning of SSID list");
		wpa_s->sched_scan_timeout = max_sched_scan_ssids * 2;
		wpa_s->first_sched_scan = 1;
		ssid = wpa_s->conf->ssid;
		wpa_s->prev_sched_ssid = ssid;
	}

	if (wildcard) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Add wildcard SSID to sched_scan");
		params.num_ssids++;
	}

	while (ssid) {
		if (wpas_network_disabled(wpa_s, ssid))
			goto next;

		if (params.num_filter_ssids < wpa_s->max_match_sets &&
		    params.filter_ssids && ssid->ssid && ssid->ssid_len) {
			wpa_dbg(wpa_s, MSG_DEBUG, "add to filter ssid: %s",
				wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
			os_memcpy(params.filter_ssids[params.num_filter_ssids].ssid,
				  ssid->ssid, ssid->ssid_len);
			params.filter_ssids[params.num_filter_ssids].ssid_len =
				ssid->ssid_len;
			params.num_filter_ssids++;
		} else if (params.filter_ssids && ssid->ssid && ssid->ssid_len)
		{
			wpa_dbg(wpa_s, MSG_DEBUG, "Not enough room for SSID "
				"filter for sched_scan - drop filter");
			os_free(params.filter_ssids);
			params.filter_ssids = NULL;
			params.num_filter_ssids = 0;
		}

		if (ssid->scan_ssid && ssid->ssid && ssid->ssid_len) {
			if (params.num_ssids == max_sched_scan_ssids)
				break; /* only room for broadcast SSID */
			wpa_dbg(wpa_s, MSG_DEBUG,
				"add to active scan ssid: %s",
				wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
			params.ssids[params.num_ssids].ssid =
				ssid->ssid;
			params.ssids[params.num_ssids].ssid_len =
				ssid->ssid_len;
			params.num_ssids++;
			if (params.num_ssids >= max_sched_scan_ssids) {
				wpa_s->prev_sched_ssid = ssid;
				do {
					ssid = ssid->next;
				} while (ssid &&
					 (wpas_network_disabled(wpa_s, ssid) ||
					  !ssid->scan_ssid));
				break;
			}
		}

	next:
		wpa_s->prev_sched_ssid = ssid;
		ssid = ssid->next;
	}

	if (params.num_filter_ssids == 0) {
		os_free(params.filter_ssids);
		params.filter_ssids = NULL;
	}

	extra_ie = wpa_supplicant_extra_ies(wpa_s);
	if (extra_ie) {
		params.extra_ies = wpabuf_head(extra_ie);
		params.extra_ies_len = wpabuf_len(extra_ie);
	}

#ifdef CONFIG_SCAN_FILTER_RSSI
	if (wpa_s->conf->filter_rssi)
		params.filter_rssi = wpa_s->conf->filter_rssi;
#endif /* CONFIG_SCAN_FILTER_RSSI */

	/* See if user specified frequencies. If so, scan only those. */
	if (wpa_s->conf->freq_list && !params.freqs) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Optimize scan based on conf->freq_list");
		int_array_concat(&params.freqs, wpa_s->conf->freq_list);
	}

	scan_params = &params;

scan:
	wpa_s->sched_scan_timed_out = 0;

	/*
	 * We cannot support multiple scan plans if the scan request includes
	 * too many SSID's, so in this case use only the last scan plan and make
	 * it run infinitely. It will be stopped by the timeout.
	 */
	if (wpa_s->sched_scan_plans_num == 1 ||
	    (wpa_s->sched_scan_plans_num && !ssid && wpa_s->first_sched_scan)) {
		params.sched_scan_plans = wpa_s->sched_scan_plans;
		params.sched_scan_plans_num = wpa_s->sched_scan_plans_num;
	} else if (wpa_s->sched_scan_plans_num > 1) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Too many SSIDs. Default to using single scheduled_scan plan");
		params.sched_scan_plans =
			&wpa_s->sched_scan_plans[wpa_s->sched_scan_plans_num -
						 1];
		params.sched_scan_plans_num = 1;
	} else {
		if (wpa_s->conf->sched_scan_interval)
			scan_plan.interval = wpa_s->conf->sched_scan_interval;
		else
			scan_plan.interval = 10;

		if (scan_plan.interval > wpa_s->max_sched_scan_plan_interval) {
			wpa_printf(MSG_WARNING,
				   "Scan interval too long(%u), use the maximum allowed(%u)",
				   scan_plan.interval,
				   wpa_s->max_sched_scan_plan_interval);
			scan_plan.interval =
				wpa_s->max_sched_scan_plan_interval;
		}

		scan_plan.iterations = 0;
		params.sched_scan_plans = &scan_plan;
		params.sched_scan_plans_num = 1;
	}

	params.sched_scan_start_delay = wpa_s->conf->sched_scan_start_delay;

	if (ssid || !wpa_s->first_sched_scan) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Starting sched scan after %u seconds: interval %u timeout %d",
			params.sched_scan_start_delay,
			params.sched_scan_plans[0].interval,
			wpa_s->sched_scan_timeout);
	} else {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Starting sched scan after %u seconds (no timeout)",
			params.sched_scan_start_delay);
	}

	wpa_setband_scan_freqs(wpa_s, scan_params);

	if ((wpa_s->mac_addr_rand_enable & MAC_ADDR_RAND_SCHED_SCAN) &&
	    wpa_s->wpa_state <= WPA_SCANNING) {
		params.mac_addr_rand = 1;
		if (wpa_s->mac_addr_sched_scan) {
			params.mac_addr = wpa_s->mac_addr_sched_scan;
			params.mac_addr_mask = wpa_s->mac_addr_sched_scan +
				ETH_ALEN;
		}
	}

#if defined ( CONFIG_SRP )
	wpa_scan_set_relative_rssi_params(wpa_s, scan_params);
#endif // CONFIG_SRP

	ret = wpa_supplicant_start_sched_scan(wpa_s, scan_params);
	wpabuf_free(extra_ie);
	os_free(params.filter_ssids);
	if (ret) {
		wpa_dbg(wpa_s, MSG_WARNING, "Failed to initiate sched scan");
		if (prev_state != wpa_s->wpa_state)
			wpa_supplicant_set_state(wpa_s, prev_state);
		return ret;
	}

	/* If we have more SSIDs to scan, add a timeout so we scan them too */
	if (ssid || !wpa_s->first_sched_scan) {
		wpa_s->sched_scan_timed_out = 0;
		da16x_eloop_register_timeout(wpa_s->sched_scan_timeout, 0,
				       wpa_supplicant_sched_scan_timeout,
				       wpa_s, NULL);
		wpa_s->first_sched_scan = 0;
		wpa_s->sched_scan_timeout /= 2;
		params.sched_scan_plans[0].interval *= 2;
		if ((unsigned int) wpa_s->sched_scan_timeout <
		    params.sched_scan_plans[0].interval ||
		    params.sched_scan_plans[0].interval >
		    wpa_s->max_sched_scan_plan_interval) {
			params.sched_scan_plans[0].interval = 10;
			wpa_s->sched_scan_timeout = max_sched_scan_ssids * 2;
		}
	}

	/* If there is no more ssids, start next time from the beginning */
	if (!ssid)
		wpa_s->prev_sched_ssid = NULL;

	return 0;
}
#endif // CONFIG_SCHED_SCAN


/**
 * wpa_supplicant_cancel_scan - Cancel a scheduled scan request
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to cancel a scan request scheduled with
 * wpa_supplicant_req_scan().
 */
void wpa_supplicant_cancel_scan(struct wpa_supplicant *wpa_s)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "Cancelling scan request");
	da16x_eloop_cancel_timeout(wpa_supplicant_scan, wpa_s, NULL);
}


#if defined ( CONFIG_SCHED_SCAN )
/**
 * wpa_supplicant_cancel_delayed_sched_scan - Stop a delayed scheduled scan
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to stop a delayed scheduled scan.
 */
void wpa_supplicant_cancel_delayed_sched_scan(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->sched_scan_supported)
		return;

	wpa_dbg(wpa_s, MSG_DEBUG, "Cancelling delayed sched scan");
	da16x_eloop_cancel_timeout(wpa_supplicant_delayed_sched_scan_timeout,
			     wpa_s, NULL);
}


/**
 * wpa_supplicant_cancel_sched_scan - Stop running scheduled scans
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to stop a periodic scheduled scan.
 */
void wpa_supplicant_cancel_sched_scan(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->sched_scanning)
		return;

	if (wpa_s->sched_scanning)
		wpa_s->sched_scan_stop_req = 1;

	wpa_dbg(wpa_s, MSG_DEBUG, "Cancelling sched scan");
	da16x_eloop_cancel_timeout(wpa_supplicant_sched_scan_timeout, wpa_s, NULL);
	wpa_supplicant_stop_sched_scan(wpa_s);
}
#endif // CONFIG_SCHED_SCAN

#if defined ( __SUPP_27_SUPPORT__ )
/**
 * wpa_supplicant_notify_scanning - Indicate possible scan state change
 * @wpa_s: Pointer to wpa_supplicant data
 * @scanning: Whether scanning is currently in progress
 *
 * This function is to generate scanning notifycations. It is called whenever
 * there may have been a change in scanning (scan started, completed, stopped).
 * wpas_notify_scanning() is called whenever the scanning state changed from the
 * previously notified state.
 */
void wpa_supplicant_notify_scanning(struct wpa_supplicant *wpa_s,
				    int scanning)
{
	if (wpa_s->scanning != scanning) {
		wpa_s->scanning = scanning;
#if 0	/* by Bright : Merge 2.7 */
		wpas_notify_scanning(wpa_s);
#endif	/* 0 */
	}
}
#endif // __SUPP_27_SUPPORT__

static int wpa_scan_get_max_rate(const struct wpa_scan_res *res)
{
	int rate = 0;
	const u8 *ie;
	const u8 *ie_ext;
	int i;

	ie = wpa_scan_get_ie(res, WLAN_EID_SUPP_RATES);

	if(ie != NULL)
	for (i = 0; ie && i < ie[1]; i++) {
		if ((ie[i + 2] & 0x7f) > rate)
			rate = ie[i + 2] & 0x7f;
	}

	ie_ext = wpa_scan_get_ie(res, WLAN_EID_EXT_SUPP_RATES);
	if(ie_ext != NULL)
	for (i = 0; ie && i < ie[1]; i++) {
		if ((ie[i + 2] & 0x7f) > rate)
			rate = ie[i + 2] & 0x7f;
	}

	return rate;
}


/**
 * wpa_scan_get_ie - Fetch a specified information element from a scan result
 * @res: Scan result entry
 * @ie: Information element identitifier (WLAN_EID_*)
 * Returns: Pointer to the information element (id field) or %NULL if not found
 *
 * This function returns the first matching information element in the scan
 * result.
 */
const u8 * wpa_scan_get_ie(const struct wpa_scan_res *res, u8 ie)
{
#if 1 /* FC9000 supplicant 2.6 */
	const u8 *end=NULL, *pos=NULL;
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	pos = (const u8 *) (res + 1);
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
	pos = (const u8 *) (res + 1);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#else
	if(res->ie == NULL)
		return NULL;
	pos = (const u8 *)res->ie;
#endif
	end = pos + res->ie_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ie)
			return pos;
		pos += 2 + pos[1];
	}

	return (const u8*)NULL;
#else
	size_t ie_len = res->ie_len;

	/* Use the Beacon frame IEs if res->ie_len is not available */
	if (!ie_len)
		ie_len = res->beacon_ie_len;

	return get_ie((const u8 *) (res + 1), ie_len, ie);
#endif /* FC9000 supplicant 2.6 */
}


/**
 * wpa_scan_get_vendor_ie - Fetch vendor information element from a scan result
 * @res: Scan result entry
 * @vendor_type: Vendor type (four octets starting the IE payload)
 * Returns: Pointer to the information element (id field) or %NULL if not found
 *
 * This function returns the first matching information element in the scan
 * result.
 */
const u8 * wpa_scan_get_vendor_ie(const struct wpa_scan_res *res,
				  u32 vendor_type)
{
	const u8 *end=NULL, *pos=NULL;

#if 1 /* FC9000 supplicant 2.6 */
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
		pos = (const u8 *) (res + 1);
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
	pos = (const u8 *) (res + 1);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#else
	if(res->ie == NULL)
		return (const u8*)NULL;
	pos = (const u8 *) res->ie;
#endif
	end = pos + res->ie_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			return pos;
		pos += 2 + pos[1];
	}

#else
	pos = (const u8 *) (res + 1);
	end = pos + res->ie_len;


	while (end - pos > 1) {
		if (2 + pos[1] > end - pos)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			return pos;
		pos += 2 + pos[1];
	}
#endif /* FC9000 supplicant 2.6 */
	return (const u8*)NULL;
}


/**
 * wpa_scan_get_vendor_ie_beacon - Fetch vendor information from a scan result
 * @res: Scan result entry
 * @vendor_type: Vendor type (four octets starting the IE payload)
 * Returns: Pointer to the information element (id field) or %NULL if not found
 *
 * This function returns the first matching information element in the scan
 * result.
 *
 * This function is like wpa_scan_get_vendor_ie(), but uses IE buffer only
 * from Beacon frames instead of either Beacon or Probe Response frames.
 */
const u8 * wpa_scan_get_vendor_ie_beacon(const struct wpa_scan_res *res,
					 u32 vendor_type)
{
	const u8 *end, *pos;

	if (res->beacon_ie_len == 0)
		return NULL;

#if 1 /* FC9000 supplicant 2.6 */
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC			
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	 pos = (const u8 *) (res + 1);
        pos += res->ie_len;
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
	pos = (const u8 *) (res + 1);
	pos += res->ie_len;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#else
	if (res->beacon_ie == NULL)
		return NULL;

	pos = (const u8 *) res->beacon_ie;
#endif
	end = pos + res->beacon_ie_len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			return pos;
		pos += 2 + pos[1];
	}
#else
	pos = (const u8 *) (res + 1);
	pos += res->ie_len;
	end = pos + res->beacon_ie_len;

	while (end - pos > 1) {
		if (2 + pos[1] > end - pos)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			return pos;
		pos += 2 + pos[1];
	}
#endif /* FC9000 supplicant 2.6 */

	return (const u8*)NULL;
}


/**
 * wpa_scan_get_vendor_ie_multi - Fetch vendor IE data from a scan result
 * @res: Scan result entry
 * @vendor_type: Vendor type (four octets starting the IE payload)
 * Returns: Pointer to the information element payload or %NULL if not found
 *
 * This function returns concatenated payload of possibly fragmented vendor
 * specific information elements in the scan result. The caller is responsible
 * for freeing the returned buffer.
 */
struct wpabuf * wpa_scan_get_vendor_ie_multi(const struct wpa_scan_res *res,
					     u32 vendor_type)
{
	struct wpabuf *buf;
	const u8 *end, *pos;

	buf = wpabuf_alloc(res->ie_len);
	if (buf == NULL)
		return NULL;

#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
	pos = (const u8 *) (res + 1);
#else
	if(res->ie == NULL)
	{
		wpabuf_free(buf);
		return NULL;
	}	
	pos = (const u8 *)res->ie;
#endif
	end = pos + res->ie_len;
#if 1 /* FC9000 supplicant 2.6 */
	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			wpabuf_put_data(buf, pos + 2 + 4, pos[1] - 4);
		pos += 2 + pos[1];
	}
#else
	while (end - pos > 1) {
		if (2 + pos[1] > end - pos)
			break;
		if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		    vendor_type == WPA_GET_BE32(&pos[2]))
			wpabuf_put_data(buf, pos + 2 + 4, pos[1] - 4);
		pos += 2 + pos[1];
	}
#endif /* FC9000 supplicant 2.6 */

	if (wpabuf_len(buf) == 0) {
		wpabuf_free(buf);
		buf = NULL;
	}

	return buf;
}


/*
 * Channels with a great SNR can operate at full rate. What is a great SNR?
 * This doc https://supportforums.cisco.com/docs/DOC-12954 says, "the general
 * rule of thumb is that any SNR above 20 is good." This one
 * http://www.cisco.com/en/US/tech/tk722/tk809/technologies_q_and_a_item09186a00805e9a96.shtml#qa23
 * recommends 25 as a minimum SNR for 54 Mbps data rate. The estimates used in
 * scan_est_throughput() allow even smaller SNR values for the maximum rates
 * (21 for 54 Mbps, 22 for VHT80 MCS9, 24 for HT40 and HT20 MCS7). Use 25 as a
 * somewhat conservative value here.
 */
#define GREAT_SNR 25

#define IS_5GHZ(n) (n > 4000)

#ifdef CONFIG_TOGGLE_SCAN_SORT_TYPE
enum compar_type {
	COMPAR_SECURITY,
	//COMPAR_RATE,
	COMPAR_SNR
};

static int wpa_scan_compar(int wpa_a, int wpa_b, int snr_a, int snr_b, struct wpa_scan_res * wa, struct wpa_scan_res * wb, enum compar_type type)
{
	switch(type) {
		case COMPAR_SECURITY:
			if (wpa_b && !wpa_a)
				return 1;
			if (!wpa_b && wpa_a)
				return -1;

			/* privacy support preferred */
			if ((wa->caps & IEEE80211_CAP_PRIVACY) == 0 &&
				(wb->caps & IEEE80211_CAP_PRIVACY))
				return 1;
			if ((wa->caps & IEEE80211_CAP_PRIVACY) &&
				(wb->caps & IEEE80211_CAP_PRIVACY) == 0)
				return -1;
#if 0 //[[ trinity_0170811_BEGIN -- test
		case COMPAR_RATE:
			/* best/max rate preferred if SNR close enough */
			if ((snr_a && snr_b && abs(snr_b - snr_a) < 5) ||
				(wa->qual && wb->qual && abs(wb->qual - wa->qual) < 10)) {
				maxrate_a = wpa_scan_get_max_rate(wa);
				maxrate_b = wpa_scan_get_max_rate(wb);
				if (maxrate_a != maxrate_b)
					return maxrate_b - maxrate_a;
			}
#endif //]] trinity_0170811_END

		case COMPAR_SNR:
			if (snr_b == snr_a)
				return wb->qual - wa->qual;
			return snr_b - snr_a;
	}

	return 0;
}

static enum compar_type current_compar_type = COMPAR_SECURITY;

static int wpa_scan_result_compar_advanced(const void *a, const void *b)
{
#define MIN(a,b) a < b ? a : b
	struct wpa_scan_res **_wa = (void *) a;
	struct wpa_scan_res **_wb = (void *) b;
	struct wpa_scan_res *wa = *_wa;
	struct wpa_scan_res *wb = *_wb;
	int wpa_a, wpa_b;
	int snr_a, snr_b;
	enum compar_type type1, type2;
	int temp_res = 0;

	type1 = current_compar_type;
	if(current_compar_type == COMPAR_SECURITY) {
		type2 = COMPAR_SNR;
	} else {
		type2 = COMPAR_SECURITY;
	}

	/* WPA/WPA2 support preferred */
	wpa_a = wpa_scan_get_vendor_ie(wa, WPA_IE_VENDOR_TYPE) != NULL ||
		wpa_scan_get_ie(wa, WLAN_EID_RSN) != NULL;
	wpa_b = wpa_scan_get_vendor_ie(wb, WPA_IE_VENDOR_TYPE) != NULL ||
		wpa_scan_get_ie(wb, WLAN_EID_RSN) != NULL;

	if ((wa->flags & wb->flags & WPA_SCAN_LEVEL_DBM) &&
		!((wa->flags | wb->flags) & WPA_SCAN_NOISE_INVALID)) {
		snr_a = MIN(wa->level - wa->noise, GREAT_SNR);
		snr_b = MIN(wb->level - wb->noise, GREAT_SNR);
	} else {
		/* Not suitable information to calculate SNR, so use level */
		snr_a = wa->level;
		snr_b = wb->level;
	}

	temp_res = wpa_scan_compar(wpa_a, wpa_b, snr_a, snr_b, wa, wb, type1);
	if(temp_res)
		return temp_res;

	temp_res = wpa_scan_compar(wpa_a, wpa_b, snr_a, snr_b, wa, wb, type2);
	if(temp_res)
		return temp_res;

#if 0 //[[ trinity_0170811_BEGIN -- test
	temp_res = wpa_scan_compar(wpa_a, wpa_b, snr_a, snr_b, wa, wb, type3);
	if(temp_res)
		return temp_res;
#endif //]] trinity_0170811_END

	return 0;
#undef MIN
}
#else  /* CONFIG_TOGGLE_SCAN_SORT_TYPE */
/* Compare function for sorting scan results. Return >0 if @b is considered
 * better. */
static int wpa_scan_result_compar(const void *a, const void *b)
{
#define MIN(a,b) a < b ? a : b
	struct wpa_scan_res **_wa = (void *) a;
	struct wpa_scan_res **_wb = (void *) b;
	struct wpa_scan_res *wa = *_wa;
	struct wpa_scan_res *wb = *_wb;
	int wpa_a, wpa_b;
	int snr_a, snr_b, snr_a_full, snr_b_full;

	/* WPA/WPA2 support preferred */
	wpa_a = wpa_scan_get_vendor_ie(wa, WPA_IE_VENDOR_TYPE) != NULL ||
		wpa_scan_get_ie(wa, WLAN_EID_RSN) != NULL;
	wpa_b = wpa_scan_get_vendor_ie(wb, WPA_IE_VENDOR_TYPE) != NULL ||
		wpa_scan_get_ie(wb, WLAN_EID_RSN) != NULL;

	if (wpa_b && !wpa_a)
		return 1;
	if (!wpa_b && wpa_a)
		return -1;

	/* privacy support preferred */
	if ((wa->caps & IEEE80211_CAP_PRIVACY) == 0 &&
	    (wb->caps & IEEE80211_CAP_PRIVACY))
		return 1;
	if ((wa->caps & IEEE80211_CAP_PRIVACY) &&
	    (wb->caps & IEEE80211_CAP_PRIVACY) == 0)
		return -1;

	if (wa->flags & wb->flags & WPA_SCAN_LEVEL_DBM) {
		snr_a_full = wa->snr;
		snr_a = MIN(wa->snr, GREAT_SNR);
		snr_b_full = wb->snr;
		snr_b = MIN(wb->snr, GREAT_SNR);
	} else {
		/* Level is not in dBm, so we can't calculate
		 * SNR. Just use raw level (units unknown). */
		snr_a = snr_a_full = wa->level;
		snr_b = snr_b_full = wb->level;
	}

	/* if SNR is close, decide by max rate or frequency band */
	if (snr_a && snr_b && abs(snr_b - snr_a) < 7) {
		if (wa->est_throughput != wb->est_throughput)
			return wb->est_throughput - wa->est_throughput;
	}
	if ((snr_a && snr_b && abs(snr_b - snr_a) < 5) ||
	    (wa->qual && wb->qual && abs(wb->qual - wa->qual) < 10)) {
		if (IS_5GHZ(wa->freq) ^ IS_5GHZ(wb->freq))
			return IS_5GHZ(wa->freq) ? -1 : 1;
	}

	/* all things being equal, use SNR; if SNRs are
	 * identical, use quality values since some drivers may only report
	 * that value and leave the signal level zero */
	if (snr_b_full == snr_a_full)
		return wb->qual - wa->qual;
	return snr_b_full - snr_a_full;
#undef MIN
}
#endif /* CONFIG_TOGGLE_SCAN_SORT_TYPE */

#ifdef CONFIG_WPS
/* Compare function for sorting scan results when searching a WPS AP for
 * provisioning. Return >0 if @b is considered better. */
static int wpa_scan_result_wps_compar(const void *a, const void *b)
{
	struct wpa_scan_res **_wa = (void *) a;
	struct wpa_scan_res **_wb = (void *) b;
	struct wpa_scan_res *wa = *_wa;
	struct wpa_scan_res *wb = *_wb;
	int uses_wps_a, uses_wps_b;
	struct wpabuf *wps_a, *wps_b;
	int res;

	/* Optimization - check WPS IE existence before allocated memory and
	 * doing full reassembly. */
	uses_wps_a = wpa_scan_get_vendor_ie(wa, WPS_IE_VENDOR_TYPE) != NULL;
	uses_wps_b = wpa_scan_get_vendor_ie(wb, WPS_IE_VENDOR_TYPE) != NULL;
	if (uses_wps_a && !uses_wps_b)
		return -1;
	if (!uses_wps_a && uses_wps_b)
		return 1;

	if (uses_wps_a && uses_wps_b) {
		wps_a = wpa_scan_get_vendor_ie_multi(wa, WPS_IE_VENDOR_TYPE);
		wps_b = wpa_scan_get_vendor_ie_multi(wb, WPS_IE_VENDOR_TYPE);
		res = wps_ap_priority_compar(wps_a, wps_b);
		wpabuf_free(wps_a);
		wpabuf_free(wps_b);
		if (res)
			return res;
	}

	/*
	 * Do not use current AP security policy as a sorting criteria during
	 * WPS provisioning step since the AP may get reconfigured at the
	 * completion of provisioning.
	 */

	/* all things being equal, use signal level; if signal levels are
	 * identical, use quality values since some drivers may only report
	 * that value and leave the signal level zero */
	if (wb->level == wa->level)
		return wb->qual - wa->qual;
	return wb->level - wa->level;
}
#endif /* CONFIG_WPS */

static void dump_scan_res(struct wpa_scan_results *scan_res)
{
	size_t i;

	da16x_scan_prt("<%s> res=0x%x, num=%d\n", __func__, scan_res->res, scan_res->num);

	if (scan_res->res == NULL || scan_res->num == 0) {
		da16x_scan_prt("<%s> None...\n", __func__);
		return;
	}

	da16x_scan_prt("<%s> Sorted Scan Lists ------\n", __func__);

	for (i = 0; i < scan_res->num; i++) {
		struct wpa_scan_res *r = scan_res->res[i];
#if 1
		int maxrate = wpa_scan_get_max_rate(r);

#ifdef ENABLE_SCAN_DBG
		int wpa = 0;
		int wpa2 = 0;
#endif /* ENABLE_SCAN_DBG */
		
#ifdef ENABLE_SCAN_DBG
		wpa = 
#endif /* ENABLE_SCAN_DBG */
				(int)(wpa_scan_get_vendor_ie(r, WPA_IE_VENDOR_TYPE) != NULL);
#ifdef ENABLE_SCAN_DBG
		wpa2 = 
#endif /* ENABLE_SCAN_DBG */
				(int)(wpa_scan_get_ie(r, WLAN_EID_RSN) != NULL);
#else
		int wpa = wpa_scan_get_vendor_ie(r, WPA_IE_VENDOR_TYPE) != NULL ||
			wpa_scan_get_ie(r, WLAN_EID_RSN) != NULL;
		int maxrate = wpa_scan_get_max_rate(r);
#endif /* 1 */

		if ((r->flags & (WPA_SCAN_LEVEL_DBM | WPA_SCAN_NOISE_INVALID)) == WPA_SCAN_LEVEL_DBM) {
#ifdef	ENABLE_SCAN_DBG
			int snr = r->level - r->noise;
#endif	/* ENABLE_SCAN_DBG */

#ifdef	ENABLE_SCAN_DBG
			da16x_scan_prt(MACSTR " freq=%d level=%d snr=%d%s "
#else
			da16x_scan_prt(MACSTR " freq=%d level=%d "
#endif	/* ENABLE_SCAN_DBG */
				"flags=0x%x age=%3u caps=0x%03x\n",
				   MAC2STR(r->bssid), r->freq,
				   r->level,
#ifdef	ENABLE_SCAN_DBG
				   snr,
				   snr >= GREAT_SNR ? "*" : "", r->flags,
#endif	/* ENABLE_SCAN_DBG */
				   r->age, r->caps);
		} else {
#if 1
			da16x_scan_prt(" 		%02d) " MACSTR
				" freq=%d level=%d flags=0x%x age=%3u, maxrate=%3d, caps=0x%03x, %s%s%s%s\n",
				i, MAC2STR(r->bssid),
				r->freq, r->level, r->flags, r->age, maxrate, r->caps,
				wpa ? "WPA " : "",
				wpa2 ? "WPA2 " : "",
				(!wpa && !wpa2 && r->caps & IEEE80211_CAP_PRIVACY) ? "WEP":"",
				(!wpa && !wpa2 && !(r->caps & IEEE80211_CAP_PRIVACY)) ? "OPEN":""
				);
#else
			da16x_scan_prt("                 %d) " MACSTR
				" freq=%d level=%d flags=0x%x age=%u, wpa=%d, maxrate=%d\n",
				i, MAC2STR(r->bssid),
				r->freq, r->level, r->flags, r->age, wpa, maxrate);
#endif /* 1 */
		}
	}
}

/**
 * wpa_supplicant_filter_bssid_match - Is the specified BSSID allowed
 * @wpa_s: Pointer to wpa_supplicant data
 * @bssid: BSSID to check
 * Returns: 0 if the BSSID is filtered or 1 if not
 *
 * This function is used to filter out specific BSSIDs from scan reslts mainly
 * for testing purposes (SET bssid_filter ctrl_iface command).
 */
#ifdef CONFIG_STA_BSSID_FILTER
int wpa_supplicant_filter_bssid_match(struct wpa_supplicant *wpa_s,
				      const u8 *bssid)
{
	size_t i;

	if (wpa_s->bssid_filter == NULL)
		return 1;

	for (i = 0; i < wpa_s->bssid_filter_count; i++) {
		if (os_memcmp(wpa_s->bssid_filter + i * ETH_ALEN, bssid,
			      ETH_ALEN) == 0)
			return 1;
	}

	return 0;
}
#endif /* CONFIG_STA_BSSID_FILTER */

#ifdef CONFIG_STA_COUNTRY_CODE
static int wpa_supplicant_filter_country_match(struct wpa_freq_range_list ranges,
				       int freq)
{
	if(freq_range_list_includes(&ranges, freq)) {
		return 1;
	}

	da16x_scan_prt("[%s] freq %d is not included in current country\n", __func__, freq);
	return 0;
}
#endif /* CONFIG_STA_COUNTRY_CODE */

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
extern void rdev_bss_lock();
extern void rdev_bss_unlock();
#endif

#define UMAC_MEM_USE_TX_MEM

void filter_scan_res(struct wpa_supplicant *wpa_s,
		     struct wpa_scan_results *res)
{
	size_t i, j;
#ifdef CONFIG_STA_COUNTRY_CODE
	struct wpa_freq_range_list ranges;
	ranges.range = &(wpa_s->conf->country_range);
	ranges.num = 1;
#endif /* CONFIG_STA_COUNTRY_CODE */

#ifdef CONFIG_STA_BSSID_FILTER
	if (wpa_s->bssid_filter == NULL)
		return;

	for (i = 0, j = 0; i < res->num; i++) {
		if (wpa_supplicant_filter_bssid_match(wpa_s,
						      res->res[i]->bssid)
#ifdef CONFIG_STA_COUNTRY_CODE
		     && wpa_supplicant_filter_country_match(ranges,
						      res->res[i]->freq)
#endif /* CONFIG_STA_COUNTRY_CODE */
		) {
			res->res[j++] = res->res[i];
		} else {
			os_free(res->res[i]);
			res->res[i] = NULL;
		}
	}

	if (res->num != j) {
		wpa_printf_dbg(MSG_DEBUG, "Filtered out %d scan results",
			   (int) (res->num - j));
		res->num = j;
	}
#else /* CONFIG_STA_BSSID_FILTER */
#ifdef CONFIG_STA_COUNTRY_CODE
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	rdev_bss_lock();
	for( int i = 0 ; i <res->num; i++)
	{
		if (!wpa_supplicant_filter_country_match(ranges, res->res[i]->freq)) 
		{
			struct wpa_scan_res *temp= NULL;

			temp = res->res[i];
			if(temp->internal_bss != NULL)
			#if 0	
				supp_unhold_bss(temp->internal_bss);
			#else
			{
				rdev_bss_unlock();
				sup_unlink_bss(temp->internal_bss);
				rdev_bss_lock();
			}	
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
	}
	rdev_bss_unlock();
#else
	for (i = 0, j = 0; i < res->num; i++) {
		if (wpa_supplicant_filter_country_match(ranges,
											  res->res[i]->freq)) {
			res->res[j++] = res->res[i];
		} else {
			umac_heap_free(res->res[i]);
			res->res[i] = NULL;
		}
	}

	if (res->num != j) {
		da16x_scan_prt("[%s] Filtered out %d scan results\n",
			   __func__, (int) (res->num - j));
		res->num = j;
	}
#endif
#endif /* CONFIG_STA_COUNTRY_CODE */
#endif /* CONFIG_STA_BSSID_FILTER */
}


#ifdef	CONFIG_SUPP27_SCAN
/*
 * Noise floor values to use when we have signal strength
 * measurements, but no noise floor measurements. These values were
 * measured in an office environment with many APs.
 */
#define DEFAULT_NOISE_FLOOR_2GHZ (-89)
#define DEFAULT_NOISE_FLOOR_5GHZ (-92)

void scan_snr(struct wpa_scan_res *res)
{
	if (res->flags & WPA_SCAN_NOISE_INVALID) {
		res->noise = IS_5GHZ(res->freq) ?
			DEFAULT_NOISE_FLOOR_5GHZ :
			DEFAULT_NOISE_FLOOR_2GHZ;
	}

#if 0	//[[ trinity_0171215_BEGIN -- test
	if (res->flags & WPA_SCAN_LEVEL_DBM) {
		res->snr = res->level - res->noise;
	} else {
		/* Level is not in dBm, so we can't calculate
		 * SNR. Just use raw level (units unknown). */
		res->snr = res->level;
	}
#endif	/* 0 */	//]] trinity_0171215_END
}


static unsigned int max_ht20_rate(int snr)
{
	if (snr < 6)
		return 6500; /* HT20 MCS0 */
	if (snr < 8)
		return 13000; /* HT20 MCS1 */
	if (snr < 13)
		return 19500; /* HT20 MCS2 */
	if (snr < 17)
		return 26000; /* HT20 MCS3 */
	if (snr < 20)
		return 39000; /* HT20 MCS4 */
	if (snr < 23)
		return 52000; /* HT20 MCS5 */
	if (snr < 24)
		return 58500; /* HT20 MCS6 */
	return 65000; /* HT20 MCS7 */
}


static unsigned int max_ht40_rate(int snr)
{
	if (snr < 3)
		return 13500; /* HT40 MCS0 */
	if (snr < 6)
		return 27000; /* HT40 MCS1 */
	if (snr < 10)
		return 40500; /* HT40 MCS2 */
	if (snr < 15)
		return 54000; /* HT40 MCS3 */
	if (snr < 17)
		return 81000; /* HT40 MCS4 */
	if (snr < 22)
		return 108000; /* HT40 MCS5 */
	if (snr < 24)
		return 121500; /* HT40 MCS6 */
	return 135000; /* HT40 MCS7 */
}


static unsigned int max_vht80_rate(int snr)
{
	if (snr < 1)
		return 0;
	if (snr < 2)
		return 29300; /* VHT80 MCS0 */
	if (snr < 5)
		return 58500; /* VHT80 MCS1 */
	if (snr < 9)
		return 87800; /* VHT80 MCS2 */
	if (snr < 11)
		return 117000; /* VHT80 MCS3 */
	if (snr < 15)
		return 175500; /* VHT80 MCS4 */
	if (snr < 16)
		return 234000; /* VHT80 MCS5 */
	if (snr < 18)
		return 263300; /* VHT80 MCS6 */
	if (snr < 20)
		return 292500; /* VHT80 MCS7 */
	if (snr < 22)
		return 351000; /* VHT80 MCS8 */
	return 390000; /* VHT80 MCS9 */
}


void scan_est_throughput(struct wpa_supplicant *wpa_s,
			 struct wpa_scan_res *res)
{
	enum local_hw_capab capab = wpa_s->hw_capab;
	int rate; /* max legacy rate in 500 kb/s units */
	const u8 *ie;
	unsigned int est, tmp;
	int snr = res->snr;

	if (res->est_throughput)
		return;

	/* Get maximum legacy rate */
	rate = wpa_scan_get_max_rate(res);

	/* Limit based on estimated SNR */
	if (rate > 1 * 2 && snr < 1)
		rate = 1 * 2;
	else if (rate > 2 * 2 && snr < 4)
		rate = 2 * 2;
	else if (rate > 6 * 2 && snr < 5)
		rate = 6 * 2;
	else if (rate > 9 * 2 && snr < 6)
		rate = 9 * 2;
	else if (rate > 12 * 2 && snr < 7)
		rate = 12 * 2;
	else if (rate > 18 * 2 && snr < 10)
		rate = 18 * 2;
	else if (rate > 24 * 2 && snr < 11)
		rate = 24 * 2;
	else if (rate > 36 * 2 && snr < 15)
		rate = 36 * 2;
	else if (rate > 48 * 2 && snr < 19)
		rate = 48 * 2;
	else if (rate > 54 * 2 && snr < 21)
		rate = 54 * 2;
	est = rate * 500;

	if (capab == CAPAB_HT || capab == CAPAB_HT40 || capab == CAPAB_VHT) {
		ie = wpa_scan_get_ie(res, WLAN_EID_HT_CAP);
		if (ie) {
			tmp = max_ht20_rate(snr);
			if (tmp > est)
				est = tmp;
		}
	}

	if (capab == CAPAB_HT40 || capab == CAPAB_VHT) {
		ie = wpa_scan_get_ie(res, WLAN_EID_HT_OPERATION);
		if (ie && ie[1] >= 2 &&
		    (ie[3] & HT_INFO_HT_PARAM_SECONDARY_CHNL_OFF_MASK)) {
			tmp = max_ht40_rate(snr);
			if (tmp > est)
				est = tmp;
		}
	}

	if (capab == CAPAB_VHT) {
		/* Use +1 to assume VHT is always faster than HT */
		ie = wpa_scan_get_ie(res, WLAN_EID_VHT_CAP);
		if (ie) {
			tmp = max_ht20_rate(snr) + 1;
			if (tmp > est)
				est = tmp;

			ie = wpa_scan_get_ie(res, WLAN_EID_HT_OPERATION);
			if (ie && ie[1] >= 2 &&
			    (ie[3] &
			     HT_INFO_HT_PARAM_SECONDARY_CHNL_OFF_MASK)) {
				tmp = max_ht40_rate(snr) + 1;
				if (tmp > est)
					est = tmp;
			}

			ie = wpa_scan_get_ie(res, WLAN_EID_VHT_OPERATION);
			if (ie && ie[1] >= 1 &&
			    (ie[2] & VHT_OPMODE_CHANNEL_WIDTH_MASK)) {
				tmp = max_vht80_rate(snr) + 1;
				if (tmp > est)
					est = tmp;
			}
		}
	}

	/* TODO: channel utilization and AP load (e.g., from AP Beacon) */

	res->est_throughput = est;
}
#endif	/* CONFIG_SUPP27_SCAN */

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
//extern char scan_reply[2048];
extern char *scan_reply;
extern int scan_reply_size;
extern int scan_reply_len;
extern size_t scan_resp_len;
extern int wpa_supp_ctrl_iface_scan_results(
	struct wpa_supplicant *wpa_s, char *buf, size_t buflen);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

/**
 * wpa_supplicant_get_scan_results - Get scan results
 * @wpa_s: Pointer to wpa_supplicant data
 * @info: Information about what was scanned or %NULL if not available
 * @new_scan: Whether a new scan was performed
 * Returns: Scan results, %NULL on failure
 *
 * This function request the current scan results from the driver and updates
 * the local BSS list wpa_s->bss. The caller is responsible for freeing the
 * results with wpa_scan_results_free().
 */
struct wpa_scan_results *
wpa_supplicant_get_scan_results(struct wpa_supplicant *wpa_s,
				struct scan_info *info, int new_scan)
{
	struct wpa_scan_results *scan_res=NULL;
	size_t i;
#ifdef CONFIG_REUSED_UMAC_BSS_LIST
	int iface = 0;
#endif //CONFIG_REUSED_UMAC_BSS_LIST

#ifdef CONFIG_TOGGLE_SCAN_SORT_TYPE
	int (*compar)(const void *, const void *) = wpa_scan_result_compar_advanced;
#else
	int (*compar)(const void *, const void *) = wpa_scan_result_compar;
#endif /* CONFIG_TOGGLE_SCAN_SORT_TYPE */

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	UINT	status;
	ULONG	da16x_events;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

	scan_res = wpa_drv_get_scan_results2(wpa_s);
	if (scan_res == NULL) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Failed to get scan results");
		return NULL;
	} else {
		da16x_scan_prt("[%s] Fetched %d scan results\n",
			      __func__, scan_res->num);
	}

	if (scan_res->fetch_time.sec == 0) {
		/*
		 * Make sure we have a valid timestamp if the driver wrapper
		 * does not set this.
		 */
		os_get_reltime(&scan_res->fetch_time);
	}
	filter_scan_res(wpa_s, scan_res);

#ifdef	CONFIG_SUPP27_SCAN
	for (i = 0; i < scan_res->num; i++) {
		struct wpa_scan_res *scan_res_item = scan_res->res[i];

		scan_snr(scan_res_item);
		scan_est_throughput(wpa_s, scan_res_item);
	}
#endif	/* CONFIG_SUPP27_SCAN */

#ifdef CONFIG_WPS
	if (wpas_wps_searching(wpa_s)) {
		wpa_dbg(wpa_s, MSG_DEBUG, "WPS: Order scan results with WPS "
			"provisioning rules");
		compar = wpa_scan_result_wps_compar;
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
/* Insert for simple roaming - 2022-11-14
  The bss of the currently connected AP is not updated by CONFIG_REUSED_UMAC_BSS_LIST.
  Therefore, update the level value with beacon */

	switch (get_run_mode()) {
		case STA_ONLY_MODE:
		case STA_SOFT_AP_CONCURRENT_MODE:
#if defined ( CONFIG_P2P )
		case STA_P2P_CONCURRENT_MODE:
#endif /* ( CONFIG_P2P ) */
			iface = 0;	// WLAN0_IFACE
			break;
#if defined ( CONFIG_P2P )
		case P2P_ONLY_MODE:
			iface = 1;	// WLAN1_IFACE
			break;
#endif /* ( CONFIG_P2P ) */
	}
	/* Update the level of the current AP */
	for (i = 0; i < scan_res->num; i++) {
		if (os_memcmp(scan_res->res[i]->bssid, wpa_s->current_bss->bssid, ETH_ALEN) == 0) {
			scan_res->res[i]->level = fc80211_get_sta_rssi_value(iface);
			scan_res->res[i]->age = 0;
			break;
		}
	}
#endif //CONFIG_REUSED_UMAC_BSS_LIST

/* Sort scan_resuts */
	if (scan_res->res) {
		qsort(scan_res->res, scan_res->num,
		      sizeof(struct wpa_scan_res *), compar);
	}

	dump_scan_res(scan_res);

#ifdef	CONFIG_SUPP27_SCAN
	if (wpa_s->ignore_post_flush_scan_res) {
		/* FLUSH command aborted an ongoing scan and these are the
		 * results from the aborted scan. Do not process the results to
		 * maintain flushed state. */
		wpa_dbg(wpa_s, MSG_DEBUG,
			"Do not update BSS table based on pending post-FLUSH scan results");
		wpa_s->ignore_post_flush_scan_res = 0;
		return scan_res;
	}
#endif	/* CONFIG_SUPP27_SCAN */

	wpa_bss_update_start(wpa_s);
	for (i = 0; i < scan_res->num; i++)
		wpa_bss_update_scan_res(wpa_s, scan_res->res[i],
					&scan_res->fetch_time);
	wpa_bss_update_end(wpa_s, info, new_scan);

#ifdef	CONFIG_P2P /* by Shingu 20170418 (P2P ACS) */
	if ((get_run_mode() == P2P_ONLY_MODE ||
	     get_run_mode() == STA_P2P_CONCURRENT_MODE) &&
	    (!wpa_s->conf->p2p_oper_channel || !wpa_s->conf->p2p_listen_channel) &&
	    os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0) {
		wpas_p2p_get_best_social(wpa_s, scan_res);
	}
#endif	/* CONFIG_P2P */

#ifdef CONFIG_IMMEDIATE_SCAN
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
	da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
						DA16X_SCAN_RESULTS_TX_EV,
						pdTRUE,
						pdFALSE,
						portNO_DELAY);

	 if (da16x_events & DA16X_SCAN_RESULTS_TX_EV) {
		da16x_scan_prt("<%s> Scan result command received from cli"
			"(status=%d)\n", __func__, status);

		if (!scan_reply) {
			scan_reply = pvPortMalloc(scan_reply_size);
		}

		if (scan_reply == NULL) {
			scan_resp_len = 1;
			xEventGroupSetBits(da16x_sp_event_group, DA16X_SCAN_RESULTS_FAIL_EV);
			da16x_info_prt("[%s] Malloc Error for scan_reply(%d)\n", __func__, scan_reply_size);
		} else {
			memset(scan_reply, 0, scan_reply_size);
			os_memcpy(scan_reply, "OK", 2);
			scan_reply_len = 2;
			scan_reply_len = wpa_supp_ctrl_iface_scan_results(
				select_sta0(wpa_s), scan_reply, scan_reply_size);
			xEventGroupSetBits(da16x_sp_event_group, DA16X_SCAN_RESULTS_RX_EV);
		}
	}
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
	xEventGroupSetBits(da16x_sp_event_group, DA16X_SCAN_RESULTS_RX_EV);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#endif /* CONFIG_IMMEDIATE_SCAN */

	da16x_scan_prt("Scan result sorted by: ");

	if (current_compar_type == COMPAR_SECURITY) {
		da16x_scan_prt("Security -> SNR \n");
	} else {
		da16x_scan_prt("SNR -> Security \n");
	}

	if (current_compar_type == COMPAR_SECURITY) {
		current_compar_type = COMPAR_SNR;
	} else {
		current_compar_type = COMPAR_SECURITY;
	}

	return scan_res;
}


/**
 * wpa_supplicant_update_scan_results - Update scan results from the driver
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 on success, -1 on failure
 *
 * This function updates the BSS table within wpa_supplicant based on the
 * currently available scan results from the driver without requesting a new
 * scan. This is used in cases where the driver indicates an association
 * (including roaming within ESS) and wpa_supplicant does not yet have the
 * needed information to complete the connection (e.g., to perform validation
 * steps in 4-way handshake).
 */
int wpa_supplicant_update_scan_results(struct wpa_supplicant *wpa_s)
{
	struct wpa_scan_results *scan_res;
	scan_res = wpa_supplicant_get_scan_results(wpa_s, NULL, 0);
	if (scan_res == NULL)
		return -1;
	wpa_scan_results_free(scan_res);

	return 0;
}


/**
 * scan_only_handler - Reports scan results
 */
void scan_only_handler(struct wpa_supplicant *wpa_s,
		       struct wpa_scan_results *scan_res)
{
	wpa_dbg(wpa_s, MSG_DEBUG, "Scan-only results received");
	if (wpa_s->last_scan_req == MANUAL_SCAN_REQ &&
	    wpa_s->manual_scan_use_id && wpa_s->own_scan_running) {
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
#if defined ( CONFIG_SCAN_WORK )
	if (wpa_s->scan_work) {
		struct wpa_radio_work *work = wpa_s->scan_work;
		wpa_s->scan_work = NULL;
#if 0	/* by Bright : Merge 2.7 */
		radio_work_done(work);
#endif	/* 0 */
	}
#endif	// CONFIG_SCAN_WORK

	if (wpa_s->wpa_state == WPA_SCANNING)
		wpa_supplicant_set_state(wpa_s, wpa_s->scan_prev_wpa_state);
}


#ifdef CONFIG_SCHED_SCAN
int wpas_scan_scheduled(struct wpa_supplicant *wpa_s)
{
	TX_FUNC_START(" 	   ");

	return da16x_eloop_is_timeout_registered(wpa_supplicant_scan, wpa_s, NULL);
}


struct wpa_driver_scan_params *
wpa_scan_clone_params(const struct wpa_driver_scan_params *src)
{
	struct wpa_driver_scan_params *params;
	size_t i;
	u8 *n;

	params = os_zalloc(sizeof(*params));
	if (params == NULL)
		return NULL;

	for (i = 0; i < src->num_ssids; i++) {
		if (src->ssids[i].ssid) {
			n = os_memdup(src->ssids[i].ssid,
				      src->ssids[i].ssid_len);
			if (n == NULL)
				goto failed;
			params->ssids[i].ssid = n;
			params->ssids[i].ssid_len = src->ssids[i].ssid_len;
		}
	}
	params->num_ssids = src->num_ssids;

	if (src->extra_ies) {
		n = os_memdup(src->extra_ies, src->extra_ies_len);
		if (n == NULL)
			goto failed;
		params->extra_ies = n;
		params->extra_ies_len = src->extra_ies_len;
	}

	if (src->freqs) {
		int len = int_array_len(src->freqs);
		params->freqs = os_memdup(src->freqs, (len + 1) * sizeof(int));
		if (params->freqs == NULL)
			goto failed;
	}

	if (src->filter_ssids) {
		params->filter_ssids = os_memdup(src->filter_ssids,
						 sizeof(*params->filter_ssids) *
						 src->num_filter_ssids);
		if (params->filter_ssids == NULL)
			goto failed;
		params->num_filter_ssids = src->num_filter_ssids;
	}

#ifdef CONFIG_SCAN_FILTER_RSSI
	params->filter_rssi = src->filter_rssi;
#endif /* CONFIG_SCAN_FILTER_RSSI */
#ifdef CONFIG_P2P
	params->p2p_probe = src->p2p_probe;
#endif /* CONFIG_P2P */
	params->only_new_results = src->only_new_results;
	params->low_priority = src->low_priority;
	params->duration = src->duration;
	params->duration_mandatory = src->duration_mandatory;

	if (src->sched_scan_plans_num > 0) {
		params->sched_scan_plans =
			os_memdup(src->sched_scan_plans,
				  sizeof(*src->sched_scan_plans) *
				  src->sched_scan_plans_num);
		if (!params->sched_scan_plans)
			goto failed;

		params->sched_scan_plans_num = src->sched_scan_plans_num;
	}

	if (src->mac_addr_rand) {
		params->mac_addr_rand = src->mac_addr_rand;

		if (src->mac_addr && src->mac_addr_mask) {
			u8 *mac_addr;

			mac_addr = os_malloc(2 * ETH_ALEN);
			if (!mac_addr)
				goto failed;

			os_memcpy(mac_addr, src->mac_addr, ETH_ALEN);
			os_memcpy(mac_addr + ETH_ALEN, src->mac_addr_mask,
				  ETH_ALEN);
			params->mac_addr = mac_addr;
			params->mac_addr_mask = mac_addr + ETH_ALEN;
		}
	}

	if (src->bssid) {
		u8 *bssid;

		bssid = os_memdup(src->bssid, ETH_ALEN);
		if (!bssid)
			goto failed;
		params->bssid = bssid;
	}

	params->relative_rssi_set = src->relative_rssi_set;
	params->relative_rssi = src->relative_rssi;
#ifdef CONFIG_BAND_5GHZ
	params->relative_adjust_band = src->relative_adjust_band;
#endif /* CONFIG_BAND_5GHZ */
	params->relative_adjust_rssi = src->relative_adjust_rssi;
	return params;

failed:
	wpa_scan_free_params(params);
	return NULL;
}
#endif /* CONFIG_SCHED_SCAN */


void wpa_scan_free_params(struct wpa_driver_scan_params *params)
{
	size_t i;

	if (params == NULL)
		return;

	for (i = 0; i < params->num_ssids; i++)
		os_free((u8 *) params->ssids[i].ssid);
	os_free((u8 *) params->extra_ies);
	os_free(params->freqs);
	os_free(params->filter_ssids);
#ifdef CONFIG_SCHED_SCAN
	os_free(params->sched_scan_plans);
#endif /* CONFIG_SCHED_SCAN */

#ifdef UNUSED_CODE
	/*
	 * Note: params->mac_addr_mask points to same memory allocation and
	 * must not be freed separately.
	 */
	os_free((u8 *) params->mac_addr);
	os_free((u8 *) params->bssid);
#endif /* UNUSED_CODE */

	os_free(params);
}

/*
 * Function : da16x_scan_results_timer
 *
 * - arguments  :
 *		: timeout_ctx	- wpa_s structure pointer
 *		: timeout	- timout sleep time (sec)
 * - return     : status	- treadX timer create result
 *
 * - Discription :
 *      Tmeout function after sending TRIGGER_SCAN
 */
int da16x_scan_results_timer(void *timeout_ctx, int timeout)
{
	UINT	status = 0;

	RX_FUNC_START("");

	da16x_eloop_register_timeout(timeout, 0,
				supp_scan_result_event,
				(void *)timeout_ctx, NULL);

	RX_FUNC_END("");

	return status;
}


#ifdef CONFIG_RECONNECT_OPTIMIZE
u8 wpa_scan_is_fast_reconnect(void) {
	if(fast_reconnect_scan == 1)
		return 1;
	else
		return 0;
}
#else
u8 wpa_scan_is_fast_reconnect(void) {
	return 0;
}
#endif /* CONFIG_RECONNECT_OPTIMIZE */


#ifdef CONFIG_CTRL_PNO
int wpas_start_pno(struct wpa_supplicant *wpa_s)
{
	int ret, prio;
	size_t i, num_ssid, num_match_ssid;
	struct wpa_ssid *ssid;
	struct wpa_driver_scan_params params;
	struct sched_scan_plan scan_plan;
	unsigned int max_sched_scan_ssids;

#if defined ( CONFIG_SCHED_SCAN )
	if (!wpa_s->sched_scan_supported)
		return -1;

	if (wpa_s->max_sched_scan_ssids > WPAS_MAX_SCAN_SSIDS)
		max_sched_scan_ssids = WPAS_MAX_SCAN_SSIDS;
	else
		max_sched_scan_ssids = wpa_s->max_sched_scan_ssids;
	if (max_sched_scan_ssids < 1)
		return -1;
#endif	// CONFIG_SCAN_WORK

	if (wpa_s->pno || wpa_s->pno_sched_pending)
		return 0;

	if ((wpa_s->wpa_state > WPA_SCANNING) &&
	    (wpa_s->wpa_state < WPA_COMPLETED)) {
		wpa_printf(MSG_ERROR, "PNO: In assoc process");
		return -EAGAIN;
	}

	if (wpa_s->wpa_state == WPA_SCANNING) {
		wpa_supplicant_cancel_scan(wpa_s);
#if defined ( CONFIG_SCHED_SCAN )
		if (wpa_s->sched_scanning) {
			wpa_printf_dbg(MSG_DEBUG, "Schedule PNO on completion of "
				   "ongoing sched scan");
			wpa_supplicant_cancel_sched_scan(wpa_s);
			wpa_s->pno_sched_pending = 1;
			return 0;
		}
#endif	// CONFIG_SCAN_WORK
	}

#if defined ( CONFIG_SCHED_SCAN )
	if (wpa_s->sched_scan_stop_req) {
		wpa_printf_dbg(MSG_DEBUG,
			   "Schedule PNO after previous sched scan has stopped");
		wpa_s->pno_sched_pending = 1;
		return 0;
	}
#endif	// CONFIG_SCAN_WORK

	os_memset(&params, 0, sizeof(params));

	num_ssid = num_match_ssid = 0;
	ssid = wpa_s->conf->ssid;
	while (ssid) {
		if (!wpas_network_disabled(wpa_s, ssid)) {
			num_match_ssid++;
			if (ssid->scan_ssid)
				num_ssid++;
		}
		ssid = ssid->next;
	}

	if (num_match_ssid == 0) {
		wpa_printf_dbg(MSG_DEBUG, "PNO: No configured SSIDs");
		return -1;
	}

	if (num_match_ssid > num_ssid) {
		params.num_ssids++; /* wildcard */
		num_ssid++;
	}

	if (num_ssid > max_sched_scan_ssids) {
		wpa_printf_dbg(MSG_DEBUG, "PNO: Use only the first %u SSIDs from "
			   "%u", max_sched_scan_ssids, (unsigned int) num_ssid);
		num_ssid = max_sched_scan_ssids;
	}

	if (num_match_ssid > wpa_s->max_match_sets) {
		num_match_ssid = wpa_s->max_match_sets;
		wpa_dbg(wpa_s, MSG_DEBUG, "PNO: Too many SSIDs to match");
	}
	params.filter_ssids = os_calloc(num_match_ssid,
					sizeof(struct wpa_driver_scan_filter));
	if (params.filter_ssids == NULL)
		return -1;

#ifdef CONFIG_PRIO_GROUP
	i = 0;
	prio = 0;
	ssid = wpa_s->conf->pssid[prio];
	while (ssid) {
		if (!wpas_network_disabled(wpa_s, ssid)) {
			if (ssid->scan_ssid && params.num_ssids < num_ssid) {
				params.ssids[params.num_ssids].ssid =
					ssid->ssid;
				params.ssids[params.num_ssids].ssid_len =
					 ssid->ssid_len;
				params.num_ssids++;
			}
			os_memcpy(params.filter_ssids[i].ssid, ssid->ssid,
				  ssid->ssid_len);
			params.filter_ssids[i].ssid_len = ssid->ssid_len;
			params.num_filter_ssids++;
			i++;
			if (i == num_match_ssid)
				break;
		}
		if (ssid->pnext)
			ssid = ssid->pnext;
		else if (prio + 1 == wpa_s->conf->num_prio)
			break;
		else
			ssid = wpa_s->conf->pssid[++prio];
	}
#endif /* CONFIG_PRIO_GROUP */

#ifdef CONFIG_SCAN_FILTER_RSSI
	if (wpa_s->conf->filter_rssi)
		params.filter_rssi = wpa_s->conf->filter_rssi;
#endif /* CONFIG_SCAN_FILTER_RSSI */

#ifdef CONFIG_SCHED_SCAN
	if (wpa_s->sched_scan_plans_num) {
		params.sched_scan_plans = wpa_s->sched_scan_plans;
		params.sched_scan_plans_num = wpa_s->sched_scan_plans_num;
	} else {
		/* Set one scan plan that will run infinitely */
		if (wpa_s->conf->sched_scan_interval)
			scan_plan.interval = wpa_s->conf->sched_scan_interval;
		else
			scan_plan.interval = 10;

		scan_plan.iterations = 0;
		params.sched_scan_plans = &scan_plan;
		params.sched_scan_plans_num = 1;
	}

	params.sched_scan_start_delay = wpa_s->conf->sched_scan_start_delay;
#endif /* CONFIG_SCHED_SCAN */

	if (params.freqs == NULL && wpa_s->manual_sched_scan_freqs) {
		wpa_dbg(wpa_s, MSG_DEBUG, "Limit sched scan to specified channels");
		params.freqs = wpa_s->manual_sched_scan_freqs;
	}

	if ((wpa_s->mac_addr_rand_enable & MAC_ADDR_RAND_PNO) &&
	    wpa_s->wpa_state <= WPA_SCANNING) {
		params.mac_addr_rand = 1;
		if (wpa_s->mac_addr_pno) {
			params.mac_addr = wpa_s->mac_addr_pno;
			params.mac_addr_mask = wpa_s->mac_addr_pno + ETH_ALEN;
		}
	}

	wpa_scan_set_relative_rssi_params(wpa_s, &params);

	ret = wpa_supplicant_start_sched_scan(wpa_s, &params);
	os_free(params.filter_ssids);
	if (ret == 0)
		wpa_s->pno = 1;
	else
		wpa_msg(wpa_s, MSG_ERROR, "Failed to schedule PNO");
	return ret;
}


int wpas_stop_pno(struct wpa_supplicant *wpa_s)
{
	int ret = 0;

	if (!wpa_s->pno)
		return 0;

#if defined ( CONFIG_SCHED_SCAN )
	ret = wpa_supplicant_stop_sched_scan(wpa_s);
	wpa_s->sched_scan_stop_req = 1;
#endif	// CONFIG_SCAN_WORK

	wpa_s->pno = 0;
	wpa_s->pno_sched_pending = 0;

	if (wpa_s->wpa_state == WPA_SCANNING)
		wpa_supplicant_req_scan(wpa_s, 0, 0);

	return ret;
}
#endif /* CONFIG_CTRL_PNO */


#ifdef	CONFIG_MAC_RAND_SCAN
void wpas_mac_addr_rand_scan_clear(struct wpa_supplicant *wpa_s,
				    unsigned int type)
{
	type &= MAC_ADDR_RAND_ALL;
	wpa_s->mac_addr_rand_enable &= ~type;

	if (type & MAC_ADDR_RAND_SCAN) {
		os_free(wpa_s->mac_addr_scan);
		wpa_s->mac_addr_scan = NULL;
	}

	if (type & MAC_ADDR_RAND_SCHED_SCAN) {
		os_free(wpa_s->mac_addr_sched_scan);
		wpa_s->mac_addr_sched_scan = NULL;
	}

	if (type & MAC_ADDR_RAND_PNO) {
		os_free(wpa_s->mac_addr_pno);
		wpa_s->mac_addr_pno = NULL;
	}
}


int wpas_mac_addr_rand_scan_set(struct wpa_supplicant *wpa_s,
				unsigned int type, const u8 *addr,
				const u8 *mask)
{
	u8 *tmp = NULL;

	wpas_mac_addr_rand_scan_clear(wpa_s, type);

	if (addr) {
		tmp = os_malloc(2 * ETH_ALEN);
		if (!tmp)
			return -1;
		os_memcpy(tmp, addr, ETH_ALEN);
		os_memcpy(tmp + ETH_ALEN, mask, ETH_ALEN);
	}

	if (type == MAC_ADDR_RAND_SCAN) {
		wpa_s->mac_addr_scan = tmp;
	} else if (type == MAC_ADDR_RAND_SCHED_SCAN) {
		wpa_s->mac_addr_sched_scan = tmp;
	} else if (type == MAC_ADDR_RAND_PNO) {
		wpa_s->mac_addr_pno = tmp;
	} else {
		wpa_printf(MSG_INFO,
			   "scan: Invalid MAC randomization type=0x%x",
			   type);
		os_free(tmp);
		return -1;
	}

	wpa_s->mac_addr_rand_enable |= type;
	return 0;
}
#endif	/* CONFIG_MAC_RAND_SCAN */


#if defined ( CONFIG_RADIO_WORK )
int wpas_abort_ongoing_scan(struct wpa_supplicant *wpa_s)
{
	struct wpa_radio_work *work;
	struct wpa_radio *radio = wpa_s->radio;

	dl_list_for_each(work, &radio->work, struct wpa_radio_work, list) {
		if (work->wpa_s != wpa_s || !work->started ||
		    (os_strcmp(work->type, "scan") != 0 &&
		     os_strcmp(work->type, "p2p-scan") != 0))
			continue;

		wpa_dbg(wpa_s, MSG_DEBUG, "Abort an ongoing scan");
#ifdef	__SUPPORT_VIRTUAL_ONELINK__
		return wpa_drv_abort_scan(wpa_s, NULL);
#else
		return 0;
#endif	/* __SUPPORT_VIRTUAL_ONELINK__ */
	}

	wpa_dbg(wpa_s, MSG_DEBUG, "No ongoing scan/p2p-scan found to abort");
	return -1;
}
#endif // CONFIG_RADIO_WORK


#if defined ( CONFIG_SCHED_SCAN )
int wpas_sched_scan_plans_set(struct wpa_supplicant *wpa_s, const char *cmd)
{
	struct sched_scan_plan *scan_plans = NULL;
	const char *token, *context = NULL;
	unsigned int num = 0;

	if (!cmd)
		return -1;

	if (!cmd[0]) {
		wpa_printf_dbg(MSG_DEBUG, "Clear sched scan plans");
		os_free(wpa_s->sched_scan_plans);
		wpa_s->sched_scan_plans = NULL;
		wpa_s->sched_scan_plans_num = 0;
		return 0;
	}

	while ((token = cstr_token(cmd, " ", &context))) {
		int ret;
		struct sched_scan_plan *scan_plan, *n;

		n = os_realloc_array(scan_plans, num + 1, sizeof(*scan_plans));
		if (!n)
			goto fail;

		scan_plans = n;
		scan_plan = &scan_plans[num];
		num++;

		ret = sscanf(token, "%u:%u", &scan_plan->interval,
			     &scan_plan->iterations);
		if (ret <= 0 || ret > 2 || !scan_plan->interval) {
			wpa_printf(MSG_ERROR,
				   "Invalid sched scan plan input: %s", token);
			goto fail;
		}

		if (scan_plan->interval > wpa_s->max_sched_scan_plan_interval) {
			wpa_printf(MSG_WARNING,
				   "scan plan %u: Scan interval too long(%u), use the maximum allowed(%u)",
				   num, scan_plan->interval,
				   wpa_s->max_sched_scan_plan_interval);
			scan_plan->interval =
				wpa_s->max_sched_scan_plan_interval;
		}

		if (ret == 1) {
			scan_plan->iterations = 0;
			break;
		}

		if (!scan_plan->iterations) {
			wpa_printf(MSG_ERROR,
				   "scan plan %u: Number of iterations cannot be zero",
				   num);
			goto fail;
		}

		if (scan_plan->iterations >
		    wpa_s->max_sched_scan_plan_iterations) {
			wpa_printf(MSG_WARNING,
				   "scan plan %u: Too many iterations(%u), use the maximum allowed(%u)",
				   num, scan_plan->iterations,
				   wpa_s->max_sched_scan_plan_iterations);
			scan_plan->iterations =
				wpa_s->max_sched_scan_plan_iterations;
		}

		wpa_printf_dbg(MSG_DEBUG,
			   "scan plan %u: interval=%u iterations=%u",
			   num, scan_plan->interval, scan_plan->iterations);
	}

	if (!scan_plans) {
		wpa_printf(MSG_ERROR, "Invalid scan plans entry");
		goto fail;
	}

	if (cstr_token(cmd, " ", &context) || scan_plans[num - 1].iterations) {
		wpa_printf(MSG_ERROR,
			   "All scan plans but the last must specify a number of iterations");
		goto fail;
	}

	wpa_printf_dbg(MSG_DEBUG, "scan plan %u (last plan): interval=%u",
		   num, scan_plans[num - 1].interval);

	if (num > wpa_s->max_sched_scan_plans) {
		wpa_printf(MSG_WARNING,
			   "Too many scheduled scan plans (only %u supported)",
			   wpa_s->max_sched_scan_plans);
		wpa_printf(MSG_WARNING,
			   "Use only the first %u scan plans, and the last one (in infinite loop)",
			   wpa_s->max_sched_scan_plans - 1);
		os_memcpy(&scan_plans[wpa_s->max_sched_scan_plans - 1],
			  &scan_plans[num - 1], sizeof(*scan_plans));
		num = wpa_s->max_sched_scan_plans;
	}

	os_free(wpa_s->sched_scan_plans);
	wpa_s->sched_scan_plans = scan_plans;
	wpa_s->sched_scan_plans_num = num;

	return 0;

fail:
	os_free(scan_plans);
	wpa_printf(MSG_ERROR, "invalid scan plans list");
	return -1;
}


/**
 * wpas_scan_reset_sched_scan - Reset sched_scan state
 * @wpa_s: Pointer to wpa_supplicant data
 *
 * This function is used to cancel a running scheduled scan and to reset an
 * internal scan state to continue with a regular scan on the following
 * wpa_supplicant_req_scan() calls.
 */
void wpas_scan_reset_sched_scan(struct wpa_supplicant *wpa_s)
{
	wpa_s->normal_scans = 0;
	if (wpa_s->sched_scanning) {
		wpa_s->sched_scan_timed_out = 0;
		wpa_s->prev_sched_ssid = NULL;
		wpa_supplicant_cancel_sched_scan(wpa_s);
	}
}


void wpas_scan_restart_sched_scan(struct wpa_supplicant *wpa_s)
{
	/* simulate timeout to restart the sched scan */
	wpa_s->sched_scan_timed_out = 1;
	wpa_s->prev_sched_ssid = NULL;
	wpa_supplicant_cancel_sched_scan(wpa_s);
}
#endif	// CONFIG_SCHED_SCAN

/* EOF */
