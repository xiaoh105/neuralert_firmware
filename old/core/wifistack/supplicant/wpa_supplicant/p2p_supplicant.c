/*
 * wpa_supplicant - P2P
 * Copyright (c) 2009-2010, Atheros Communications
 * Copyright (c) 2010-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_P2P

#include "supp_common.h"
#include "supp_eloop.h"
#include "common/ieee802_11_common.h"
#include "common/ieee802_11_defs.h"
#include "wpa_ctrl.h"
#include "wps/wps_i.h"
#include "p2p/supp_p2p.h"
#include "p2p_i.h"
#include "ap/hostapd.h"
#include "ap/ap_config.h"
#include "ap/sta_info.h"
#include "ap/ap_drv_ops.h"
#include "ap/wps_hostapd.h"
#include "ap/p2p_hostapd.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "rsn_supp/wpa.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "ap.h"
#include "config_ssid.h"
#include "supp_config.h"
#include "supp_scan.h"
#include "bss.h"
#include "offchannel.h"
#include "wps_supplicant.h"
#include "p2p_supplicant.h"

#include "iface_defs.h"

#ifdef	CONFIG_WIFI_DISPLAY
#include "wifi_display.h"
#endif	/* CONFIG_WIFI_DISPLAY */
#include "lwip/err.h"

#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

/*
 * How many times to try to scan to find the GO before giving up on join
 * request.
 */
#define P2P_MAX_JOIN_SCAN_ATTEMPTS	10

#ifdef CONFIG_5G_SUPPORT
/**
 * Defines time interval in seconds when a GO needs to evacuate a frequency that
 * it is currently using, but is no longer valid for P2P use cases.
 */
#define P2P_GO_FREQ_CHANGE_TIME 5

/**
 * Defines CSA parameters which are used when GO evacuates the no longer valid
 * channel (and if the driver supports channel switch).
 */
#define P2P_GO_CSA_COUNT 7
#define P2P_GO_CSA_BLOCK_TX 0

#endif /* CONFIG_5G_SUPPORT */

#ifndef P2P_MAX_CLIENT_IDLE
/*
 * How many seconds to try to reconnect to the GO when connection in P2P client
 * role has been lost.
 */
#define P2P_MAX_CLIENT_IDLE		10
#endif /* P2P_MAX_CLIENT_IDLE */

#ifndef P2P_MAX_INITIAL_CONN_WAIT
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * WPS provisioning step or after the re-invocation of a persistent group on a
 * P2P Client.
 */
#define P2P_MAX_INITIAL_CONN_WAIT	10
#endif /* P2P_MAX_INITIAL_CONN_WAIT */

#ifndef P2P_MAX_INITIAL_CONN_WAIT_GO
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * WPS provisioning step on the GO. This controls the extra time the P2P
 * operation is considered to be in progress (e.g., to delay other scans) after
 * WPS provisioning has been completed on the GO during group formation.
 */
#define P2P_MAX_INITIAL_CONN_WAIT_GO	10
#endif /* P2P_MAX_INITIAL_CONN_WAIT_GO */

#ifndef P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE
/*
 * How many seconds to wait for initial 4-way handshake to get completed after
 * re-invocation of a persistent group on the GO when the client is expected
 * to connect automatically (no user interaction).
 */
#define P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE 15
#endif /* P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE */

#define P2P_MGMT_DEVICE_PREFIX		"p2p-dev-"

#ifdef CONFIG_5G_SUPPORT
/*
 * How many seconds to wait to re-attempt to move GOs, in case previous attempt
 * was not possible.
 */
#define P2P_RECONSIDER_GO_MOVE_DELAY 30

#endif /* CONFIG_5G_SUPPORT */

enum p2p_group_removal_reason {
	P2P_GROUP_REMOVAL_UNKNOWN,
	P2P_GROUP_REMOVAL_SILENT,
	P2P_GROUP_REMOVAL_FORMATION_FAILED,
	P2P_GROUP_REMOVAL_REQUESTED,
	P2P_GROUP_REMOVAL_IDLE_TIMEOUT,
	P2P_GROUP_REMOVAL_UNAVAILABLE,
	P2P_GROUP_REMOVAL_GO_ENDING_SESSION,
	P2P_GROUP_REMOVAL_PSK_FAILURE,
	P2P_GROUP_REMOVAL_FREQ_CONFLICT,
#ifdef CONFIG_5G_SUPPORT
	P2P_GROUP_REMOVAL_GO_LEAVE_CHANNEL
#endif /* CONFIG_5G_SUPPORT */
};

extern int get_run_mode(void);

static struct wpa_supplicant *
wpas_p2p_get_group_iface(struct wpa_supplicant *wpa_s, int addr_allocated,
			 int go);
static int wpas_p2p_join_start(struct wpa_supplicant *wpa_s, int freq,
			       const u8 *ssid, size_t ssid_len);
static void wpas_p2p_join_scan_req(struct wpa_supplicant *wpa_s, int freq,
				   const u8 *ssid, size_t ssid_len);
static void wpas_p2p_join_scan(void *eloop_ctx, void *timeout_ctx);
static int wpas_p2p_join(struct wpa_supplicant *wpa_s, const u8 *iface_addr,
			 const u8 *dev_addr, enum p2p_wps_method wps_method,
			 int freq, const u8 *ssid, size_t ssid_len);
static int wpas_p2p_create_iface(struct wpa_supplicant *wpa_s);
static void wpas_p2p_group_idle_timeout(void *eloop_ctx, void *timeout_ctx);
static void wpas_p2p_set_group_idle_timeout(struct wpa_supplicant *wpa_s);
static void wpas_p2p_group_formation_timeout(void *eloop_ctx,
					     void *timeout_ctx);
static void wpas_p2p_fallback_to_go_neg(struct wpa_supplicant *wpa_s,
					int group_added);
static int wpas_p2p_stop_find_oper(struct wpa_supplicant *wpa_s);
static void wpas_stop_listen(void *ctx);
static void wpas_p2p_psk_failure_removal(void *eloop_ctx, void *timeout_ctx);

#ifdef CONFIG_5G_SUPPORT
static void wpas_p2p_optimize_listen_channel(struct wpa_supplicant *wpa_s,
					     struct wpa_used_freq_data *freqs,
					     unsigned int num);
static void wpas_p2p_move_go(void *eloop_ctx, void *timeout_ctx);
static int wpas_p2p_go_is_peer_freq(struct wpa_supplicant *wpa_s, int freq);
static void
wpas_p2p_consider_moving_gos(struct wpa_supplicant *wpa_s,
			     struct wpa_used_freq_data *freqs, unsigned int num,
			     enum wpas_p2p_channel_update_trig trig);
static void wpas_p2p_reconsider_moving_go(void *eloop_ctx, void *timeout_ctx);
#endif /* CONFIG_5G_SUPPORT */

void p2p_reconfig_net(void);

/*
 * Get the number of concurrent channels that the HW can operate, but that are
 * currently not in use by any of the wpa_supplicant interfaces.
 */
static int wpas_p2p_num_unused_channels(struct wpa_supplicant *wpa_s)
{
	int *freqs;
	int num, unused;

	freqs = os_calloc(wpa_s->num_multichan_concurrent, sizeof(int));
	if (!freqs)
		return -1;

	num = get_shared_radio_freqs(wpa_s, freqs,
				     wpa_s->num_multichan_concurrent);
	os_free(freqs);

	unused = wpa_s->num_multichan_concurrent - num;
	da16x_p2p_prt("[%s] P2P: num_unused_channels: %d\n", __func__, unused);
	return unused;
}


/*
 * Get the frequencies that are currently in use by one or more of the virtual
 * interfaces, and that are also valid for P2P operation.
 */
static int wpas_p2p_valid_oper_freqs(struct wpa_supplicant *wpa_s,
				     int *p2p_freqs, unsigned int len)
{
	int *freqs;
	unsigned int num, i, j;

	freqs = os_calloc(wpa_s->num_multichan_concurrent, sizeof(int));
	if (!freqs)
		return -1;

	num = get_shared_radio_freqs(wpa_s, freqs,
				     wpa_s->num_multichan_concurrent);

	os_memset(p2p_freqs, 0, sizeof(int) * len);

#ifdef	CONFIG_P2P
	for (i = 0, j = 0; i < num && j < len; i++) {
		if (p2p_supported_freq(wpa_s->global->p2p, freqs[i]))
			p2p_freqs[j++] = freqs[i];
	}
#endif	/* CONFIG_P2P */

	os_free(freqs);

//	dump_freq_array(wpa_s, "valid for P2P", p2p_freqs, j);

	return j;
}

#ifdef CONFIG_REUSED_UMAC_BSS_LIST
extern void supp_rehold_bss(void *temp);
#endif

#ifdef	CONFIG_P2P
static void wpas_p2p_scan_res_handler(struct wpa_supplicant *wpa_s,
				      struct wpa_scan_results *scan_res)
{
	size_t i;
	
#ifdef CONFIG_REUSED_UMAC_BSS_LIST	
	int status = 0;
#endif

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;

	da16x_p2p_prt("[%s] P2P: Scan results received (%d BSS)\n",
		   	__func__, (int) scan_res->num);

	for (i = 0; i < scan_res->num; i++) {
		struct wpa_scan_res *bss = scan_res->res[i];
		struct os_reltime time_tmp_age, entry_ts;
		const u8 *ies = NULL;
		size_t ies_len = 0;

		time_tmp_age.sec = bss->age / 1000;
		time_tmp_age.usec = (bss->age % 1000) * 1000;
		os_reltime_sub(&scan_res->fetch_time, &time_tmp_age, &entry_ts);

#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
		ies = (const u8 *)(bss + 1);
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
		ies = (const u8 *) (bss + 1);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#else

		if(bss->ie == NULL)
		{
			struct wpa_bss *bss_t = NULL;
			
			dl_list_for_each(bss_t, &wpa_s->bss, struct wpa_bss, list) {
				if (os_memcmp(bss_t->bssid, bss->bssid, ETH_ALEN) == 0)
				{
					if(bss_t->ie != NULL)
						ies =  bss_t->ie;

					bss->ie = bss_t->ie;
					bss->ie_len = bss_t->ie_len;
					
#ifdef CONFIG_REUSED_UMAC_BSS_LIST						
					status =1;
#endif

					if((bss->beacon_ie == NULL)&&(bss_t->beacon_ie != NULL))
					{
						bss->beacon_ie = bss_t->beacon_ie;
						bss->beacon_ie_len = bss_t->beacon_ie_len;
#ifdef CONFIG_REUSED_UMAC_BSS_LIST		
						status =2;
#endif						
					}
					
				}	
				}	
			}
		else
		{
		ies = (const u8 *) bss->ie;		
		}	
		
#endif
		ies_len = bss->ie_len;
		if (bss->beacon_ie_len > 0 &&
		    !wpa_scan_get_vendor_ie(bss, P2P_IE_VENDOR_TYPE) &&
		    wpa_scan_get_vendor_ie_beacon(bss, P2P_IE_VENDOR_TYPE)) {
			da16x_p2p_prt("[%s] P2P: Use P2P IE(s) from Beacon "
				"frame since no P2P IE(s) in Probe Response "
				"frames received for " MACSTR "\n",
				__func__, MAC2STR(bss->bssid));
#ifndef CONFIG_SCAN_UMAC_HEAP_ALLOC			
#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
			ies = (const u8 *)(bss + 1) + bss->ie_len;
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
			ies = ies + ies_len;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
#else
			ies = (const u8 *) bss->beacon_ie;
#endif

			ies_len = bss->beacon_ie_len;
		}

#ifdef CONFIG_SCAN_UMAC_HEAP_ALLOC
		if(status ==2)
		{
		bss->ie = NULL;
		bss->beacon_ie = NULL;
		}else if(status = 1){
			bss->ie = NULL;
		}	
#endif

		if (p2p_scan_res_handler(wpa_s->global->p2p, bss->bssid,
					 bss->freq, &entry_ts, bss->level,
					 ies, ies_len) > 0)
			break;
	}

	p2p_scan_res_handled(wpa_s->global->p2p);
}
#endif	/* CONFIG_P2P */


#if 0	/* by Bright */
static void wpas_p2p_trigger_scan_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct wpa_driver_scan_params *params = work->ctx;
	int ret;

	if (deinit) {
		if (!work->started) {
			wpa_scan_free_params(params);
			return;
		}

		wpa_s->p2p_scan_work = NULL;
		return;
	}

	if (wpa_s->clear_driver_scan_cache) {
		wpa_printf_dbg(MSG_DEBUG,
			   "Request driver to clear scan cache due to local BSS flush");
		params->only_new_results = 1;
	}
	ret = wpa_drv_scan(wpa_s, params);
	if (ret == 0)
		wpa_s->curr_scan_cookie = params->scan_cookie;
	wpa_scan_free_params(params);
	work->ctx = NULL;
	if (ret) {
		radio_work_done(work);
		return;
	}

	os_get_reltime(&wpa_s->scan_trigger_time);
	wpa_s->scan_res_handler = wpas_p2p_scan_res_handler;
	wpa_s->own_scan_requested = 1;
	wpa_s->clear_driver_scan_cache = 0;
	wpa_s->p2p_scan_work = work;
}
#endif	/* 0 */


static int wpas_p2p_scan(void *ctx, enum p2p_scan_type type, u16 pw_id)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_driver_scan_params *params = NULL;
	struct wpabuf *wps_ie = NULL, *ies = NULL;
	size_t ielen;
	int ret;
	u8 *n;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	params = os_zalloc(sizeof(*params));
	if (params == NULL)
		return -1;

	/* P2P Wildcard SSID */
	params->num_ssids = 1;
	n = os_malloc(P2P_WILDCARD_SSID_LEN);
	if (n == NULL)
		goto fail;
	os_memcpy(n, P2P_WILDCARD_SSID, P2P_WILDCARD_SSID_LEN);
	params->ssids[0].ssid = n;
	params->ssids[0].ssid_len = P2P_WILDCARD_SSID_LEN;

#ifdef	CONFIG_WPS
	wpa_s->wps->dev.p2p = 1;

	wps_ie = wps_build_probe_req_ie(pw_id, &wpa_s->wps->dev,
					wpa_s->wps->uuid, WPS_REQ_ENROLLEE,
					0, NULL);
#else
da16x_p2p_prt("[%s] Have to include CONFIG_WPS features ...\n", __func__);
#endif	/* CONFIG_WPS */

	if (wps_ie == NULL)
		goto fail;
#ifdef	CONFIG_P2P_UNUSED_CMD
	ielen = p2p_scan_ie_buf_len(wpa_s->global->p2p);
#else
	ielen = 100;
#endif	/* CONFIG_P2P_UNUSED_CMD */
	ies = wpabuf_alloc(wpabuf_len(wps_ie) + ielen);
	if (ies == NULL) {
		wpabuf_free(wps_ie);
		goto fail;
	}
	wpabuf_put_buf(ies, wps_ie);
	wpabuf_free(wps_ie);

	p2p_scan_ie(wpa_s->global->p2p, ies, NULL);

	params->p2p_probe = 1;

    /* To flush bss entries from previous scans. Refer to RM#849 */
    params->only_new_results = 1;
    
	n = os_malloc(wpabuf_len(ies));
	if (n == NULL) {
		wpabuf_free(ies);
		goto fail;
	}
	os_memcpy(n, wpabuf_head(ies), wpabuf_len(ies));
	params->extra_ies = n;
	params->extra_ies_len = wpabuf_len(ies);
	wpabuf_free(ies);

	switch (type) {
	case P2P_SCAN_SOCIAL:
		params->freqs = os_malloc(4 * sizeof(int));
		if (params->freqs == NULL)
			goto fail;
		params->freqs[0] = 2412;
		params->freqs[1] = 2437;
		params->freqs[2] = 2462;
		params->freqs[3] = 0;
		break;
	case P2P_SCAN_FULL:
#ifdef CONFIG_STA_COUNTRY_CODE
		update_freqs_with_country_code(wpa_s, &params->freqs);
#else
		params->freqs = os_malloc(14 * sizeof(int));
		if (params->freqs == NULL)
			goto fail;
		for (int i = 0; i < 13; i++)
			params->freqs[i] = 2412 + (5 * i);
		params->freqs[13] = 0;
#endif /* CONFIG_STA_COUNTRY_CODE */
		break;
	}

        ret = wpa_drv_scan(wpa_s, params);
        wpa_scan_free_params(params);

	if (ret)
                goto fail;

        os_get_reltime(&wpa_s->scan_trigger_time);
        wpa_s->scan_res_handler = wpas_p2p_scan_res_handler;
        wpa_s->own_scan_requested = 1;

	return 0;

fail:
	wpa_scan_free_params(params);
	return -1;
}


static enum wpa_driver_if_type wpas_p2p_if_type(int p2p_group_interface)
{
	switch (p2p_group_interface) {
	case P2P_GROUP_INTERFACE_PENDING:
		return WPA_IF_P2P_GROUP;
	case P2P_GROUP_INTERFACE_GO:
		return WPA_IF_P2P_GO;
	case P2P_GROUP_INTERFACE_CLIENT:
		return WPA_IF_P2P_CLIENT;
	}

	return WPA_IF_P2P_GROUP;
}


#ifdef	CONFIG_P2P_OPTION
static struct wpa_supplicant * wpas_get_p2p_group(struct wpa_supplicant *wpa_s,
						  const u8 *ssid,
						  size_t ssid_len, int *go)
{
	struct wpa_ssid *s;

	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		for (s = wpa_s->conf->ssid; s; s = s->next) {
			if (s->disabled != 0 || !s->p2p_group ||
			    s->ssid_len != ssid_len ||
			    os_memcmp(ssid, s->ssid, ssid_len) != 0)
				continue;
			if (s->mode == WPAS_MODE_P2P_GO &&
			    s != wpa_s->current_ssid)
				continue;
			if (go)
				*go = s->mode == WPAS_MODE_P2P_GO;
			return wpa_s;
		}
	}

	return NULL;
}
#endif	/* CONFIG_P2P_OPTION */


static int wpas_p2p_group_delete(struct wpa_supplicant *wpa_s,
				 enum p2p_group_removal_reason removal_reason)
{
	struct wpa_ssid *ssid;
#ifdef	ENABLE_P2P_DBG
	char *gtype;
	const char *reason;
#endif	/* ENABLE_P2P_DBG */

	ssid = wpa_s->current_ssid;
	if (ssid == NULL) {
		/*
		 * The current SSID was not known, but there may still be a
		 * pending P2P group interface waiting for provisioning or a
		 * P2P group that is trying to reconnect.
		 */
		ssid = wpa_s->conf->ssid;
		while (ssid) {
			if (ssid->p2p_group && ssid->disabled != 2)
				break;
			ssid = ssid->next;
		}
		if (ssid == NULL &&
			wpa_s->p2p_group_interface == NOT_P2P_GROUP_INTERFACE)
		{
			da16x_p2p_prt("[%s] P2P: P2P group interface "
				     "not found\n", __func__);
			return 0;
		}
	}

	if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_GO) {
#ifdef	ENABLE_P2P_DBG
		gtype = "GO";
#endif	/* ENABLE_P2P_DBG */
	} else if (wpa_s->p2p_group_interface == P2P_GROUP_INTERFACE_CLIENT ||
		 (ssid && ssid->mode == WPAS_MODE_INFRA)) {
		wpa_s->reassociate = 0;
		wpa_s->disconnected = 1;
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);
#ifdef	ENABLE_P2P_DBG
		gtype = "client";
#endif	/* ENABLE_P2P_DBG */
	} else {
#ifdef	ENABLE_P2P_DBG
		gtype = "GO";
#endif	/* ENABLE_P2P_DBG */
	}

#ifdef	ENABLE_P2P_DBG
	switch (removal_reason) {
	case P2P_GROUP_REMOVAL_REQUESTED:
		reason = " reason=REQUESTED";
		break;
	case P2P_GROUP_REMOVAL_FORMATION_FAILED:
		reason = " reason=FORMATION_FAILED";
		break;
	case P2P_GROUP_REMOVAL_IDLE_TIMEOUT:
		reason = " reason=IDLE";
		break;
	case P2P_GROUP_REMOVAL_UNAVAILABLE:
		reason = " reason=UNAVAILABLE";
		break;
	case P2P_GROUP_REMOVAL_GO_ENDING_SESSION:
		reason = " reason=GO_ENDING_SESSION";
		break;
	case P2P_GROUP_REMOVAL_PSK_FAILURE:
		reason = " reason=PSK_FAILURE";
		break;
	case P2P_GROUP_REMOVAL_FREQ_CONFLICT:
		reason = " reason=FREQ_CONFLICT";
		break;
	default:
		reason = "";
		break;
	}

	if (removal_reason != P2P_GROUP_REMOVAL_SILENT) {
		da16x_p2p_prt("[%s] %s %s%s\n",
			       __func__, wpa_s->ifname, gtype, reason);
	}
#endif	/* ENABLE_P2P_DBG */

	if (da16x_eloop_cancel_timeout(wpas_p2p_group_idle_timeout,
				      wpa_s, NULL) > 0)
		da16x_p2p_prt("[%s] P2P: Cancelled P2P group idle timeout\n",
				__func__);

	if (da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				      wpa_s, NULL) > 0) {
		da16x_p2p_prt("[%s] P2P: Cancelled P2P group formation "
				"timeout\n", __func__);

		wpa_s->p2p_in_provisioning = 0;
	}

#ifdef	CONFIG_P2P_OPTION
	wpa_s->p2p_in_invitation = 0;
#endif	/* CONFIG_P2P_OPTION */
	
	/*
	 * Make sure wait for the first client does not remain active after the
	 * group has been removed.
	 */
	wpa_s->global->p2p_go_wait_client.sec = 0;

#if 0	/* by Bright */
	if (removal_reason != P2P_GROUP_REMOVAL_SILENT && ssid)
		wpas_notify_p2p_group_removed(wpa_s, ssid, gtype);
#endif	/* 0 */

	if (wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE) {
		struct wpa_global *global;
		char *ifname;
		enum wpa_driver_if_type type;

		da16x_p2p_prt("[%s] P2P: Remove group interface %s\n",
				__func__, wpa_s->ifname);

		global = wpa_s->global;
		ifname = os_strdup(wpa_s->ifname);
		type = wpas_p2p_if_type(wpa_s->p2p_group_interface);
		wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
		wpa_s = global->ifaces;
		if (wpa_s && ifname)
			wpa_drv_if_remove(wpa_s, type, ifname);
		os_free(ifname);
		return 1;
	}

	if (!wpa_s->p2p_go_group_formation_completed) {
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
	}

	wpa_s->show_group_started = 0;
	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;

	wpa_s->waiting_presence_resp = 0;

	da16x_p2p_prt("[%s] P2P: Remove temporary group network\n", __func__);
	if (ssid && (ssid->p2p_group ||
		     ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION ||
		     (ssid->key_mgmt & WPA_KEY_MGMT_WPS))) {
		int id = ssid->id;
		if (ssid == wpa_s->current_ssid) {
			wpa_sm_set_config(wpa_s->wpa, NULL);
			eapol_sm_notify_config(wpa_s->eapol, NULL, NULL);
			wpa_s->current_ssid = NULL;
		}
		/*
		 * Networks objects created during any P2P activities are not
		 * exposed out as they might/will confuse certain non-P2P aware
		 * applications since these network objects won't behave like
		 * regular ones.
		 *
		 * Likewise, we don't send out network removed signals for such
		 * network objects.
		 */
		wpa_config_remove_network(wpa_s->conf, id);
		wpa_supplicant_clear_status(wpa_s);
	} else {
		da16x_p2p_prt("[%s] P2P: Temporary group network not found\n",
				__func__);
	}
#ifdef	CONFIG_AP
	if (wpa_s->ap_iface) {
#ifdef	CONFIG_P2P_POWER_SAVE /* by Shingu 20180208 (P2P PS opt.) */
		hostapd_p2p_default_noa(wpa_s->ap_iface->bss[0], 0);
		vTaskDelay(10);
#endif   /* CONFIG_P2P_POWER_SAVE */
		wpa_supplicant_ap_deinit(wpa_s);
	}
#endif	/* CONFIG_AP */

	p2p_reconfig_net();
	
	return 0;
}

#ifdef	CONFIG_P2P_OPTION
static int wpas_p2p_persistent_group(struct wpa_supplicant *wpa_s,
				     u8 *go_dev_addr,
				     const u8 *ssid, size_t ssid_len)
{
	struct wpa_bss *bss;
	const u8 *bssid;
	struct wpabuf *p2p;
	u8 group_capab;
	const u8 *addr;

	if (wpa_s->go_params)
		bssid = wpa_s->go_params->peer_interface_addr;
	else
		bssid = wpa_s->bssid;

	bss = wpa_bss_get(wpa_s, bssid, ssid, ssid_len);
	if (bss == NULL && wpa_s->go_params &&
	    !is_zero_ether_addr(wpa_s->go_params->peer_device_addr))
		bss = wpa_bss_get_p2p_dev_addr(
			wpa_s, wpa_s->go_params->peer_device_addr);
	if (bss == NULL) {
		u8 iface_addr[ETH_ALEN];
		if (p2p_get_interface_addr(wpa_s->global->p2p, bssid,
					   iface_addr) == 0)
			bss = wpa_bss_get(wpa_s, iface_addr, ssid, ssid_len);
	}
	if (bss == NULL) {
		da16x_p2p_prt("[%s] P2P: Could not figure out whether "
			   "group is persistent - BSS " MACSTR " not found\n",
			   __func__, MAC2STR(bssid));

		return 0;
	}

	p2p = wpa_bss_get_vendor_ie_multi(bss, P2P_IE_VENDOR_TYPE);
	if (p2p == NULL)
		p2p = (struct wpabuf *)wpa_bss_get_vendor_ie_multi_beacon(bss,
						 (u32)P2P_IE_VENDOR_TYPE);
	if (p2p == NULL) {
		da16x_p2p_prt("[%s] P2P: Could not figure out whether "
			   "group is persistent - BSS " MACSTR
			   " did not include P2P IE\n",
				__func__, MAC2STR(bssid));

#ifdef CONFIG_SCAN_RESULT_OPTIMIZE
		da16x_dump("P2P: Probe Response IEs",
			    (u8 *) (bss->ie), bss->ie_len);
		da16x_dump("P2P: Beacon IEs",
			    ((u8 *) (bss->beacon_ie), bss->beacon_ie_len);
#else /* CONFIG_SCAN_RESULT_OPTIMIZE */
		da16x_dump("P2P: Probe Response IEs",
			    (u8 *) (bss + 1), bss->ie_len);
		da16x_dump("P2P: Beacon IEs",
			    ((u8 *) bss + 1) + bss->ie_len, bss->beacon_ie_len);
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
		return 0;
	}

	group_capab = p2p_get_group_capab(p2p);
	addr = p2p_get_go_dev_addr(p2p);

	da16x_p2p_prt("[%s] P2P: Checking whether group is persistent: "
		   "group_capab=0x%x\n", __func__, group_capab);
	if (addr) {
		os_memcpy(go_dev_addr, addr, ETH_ALEN);
		da16x_p2p_prt("[%s] P2P: GO Device Address " MACSTR "\n",
			   __func__, MAC2STR(addr));
	} else
		os_memset(go_dev_addr, 0, ETH_ALEN);
	wpabuf_free(p2p);

	da16x_p2p_prt("[%s] P2P: BSS " MACSTR " group_capab=0x%x "
		   "go_dev_addr=" MACSTR "\n",
		   __func__, MAC2STR(bssid), group_capab, MAC2STR(go_dev_addr));

	return group_capab & P2P_GROUP_CAPAB_PERSISTENT_GROUP;
}


static int wpas_p2p_store_persistent_group(struct wpa_supplicant *wpa_s,
					   struct wpa_ssid *ssid,
					   const u8 *go_dev_addr)
{
	struct wpa_ssid *s;
#if 0	/* by Bright */
	int changed = 0;
#endif	/* 0 */

	da16x_p2p_prt("[%s] P2P: Storing credentials for a persistent "
		   "group (GO Dev Addr " MACSTR ")\n",
			__func__, MAC2STR(go_dev_addr));

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    os_memcmp(go_dev_addr, s->bssid, ETH_ALEN) == 0 &&
		    s->ssid_len == ssid->ssid_len &&
		    os_memcmp(ssid->ssid, s->ssid, ssid->ssid_len) == 0)
			break;
	}

	if (s) {
		da16x_p2p_prt("[%s] P2P: Update existing persistent group "
			   "entry\n", __func__);

#if 0	/* by Bright */
		if (ssid->passphrase && !s->passphrase)
			changed = 1;
		else if (ssid->passphrase && s->passphrase &&
			 os_strcmp(ssid->passphrase, s->passphrase) != 0)
			changed = 1;
#endif	/* 0 */
	} else {
		da16x_p2p_prt("[%s] P2P: Create a new persistent group entry\n",
				__func__);
#if 0	/* by Bright */
		changed = 1;
#endif	/* 0 */

		s = wpa_config_add_network(wpa_s->conf, wpa_s->ifname,
					   FIXED_NETWORK_ID_NONE);
		if (s == NULL)
			return -1;

		/*
		 * Instead of network_added we emit persistent_group_added
		 * notification. Also to keep the defense checks in
		 * persistent_group obj registration method, we set the
		 * relevant flags in s to designate it as a persistent group.
		 */
		s->p2p_group = 1;
		s->p2p_persistent_group = 1;
#if 0	/* by Bright */
		wpas_notify_persistent_group_added(wpa_s, s);
#endif	/* 0 */
		wpa_config_set_network_defaults(s);
	}

	s->p2p_group = 1;
	s->p2p_persistent_group = 1;
	s->disabled = 2;
	s->bssid_set = 1;
	os_memcpy(s->bssid, go_dev_addr, ETH_ALEN);
	s->mode = ssid->mode;
	s->auth_alg = WPA_AUTH_ALG_OPEN;
	s->key_mgmt = WPA_KEY_MGMT_PSK;
	s->proto = WPA_PROTO_RSN;
	s->pairwise_cipher = WPA_CIPHER_CCMP;
	s->export_keys = 1;
	if (ssid->passphrase) {
		os_free(s->passphrase);
		s->passphrase = os_strdup(ssid->passphrase);
	}
	if (ssid->psk_set) {
		s->psk_set = 1;
		os_memcpy(s->psk, ssid->psk, 32);
	}
	if (s->passphrase && !s->psk_set)
		wpa_config_update_psk(s);
	if (s->ssid == NULL || s->ssid_len < ssid->ssid_len) {
		os_free(s->ssid);
		s->ssid = os_malloc(ssid->ssid_len);
	}
	if (s->ssid) {
		s->ssid_len = ssid->ssid_len;
		os_memcpy(s->ssid, ssid->ssid, s->ssid_len);
	}
	if (ssid->mode == WPAS_MODE_P2P_GO && wpa_s->global->add_psk) {
		dl_list_add(&s->psk_list, &wpa_s->global->add_psk->list);
		wpa_s->global->add_psk = NULL;
#if 0	/* by Bright */
		changed = 1;
#endif	/* 0 */
	}

#if 0	/* by Bright */
	if (changed && wpa_s->conf->update_config &&
		wpa_config_write(wpa_s->confname, wpa_s->conf, 2)) {
			da16x_p2p_prt("[%s] P2P: Failed to update "
				"configuration\n", __func__);
	}
#endif	/* 0 */

	return s->id;
}


static void wpas_p2p_add_persistent_group_client(struct wpa_supplicant *wpa_s,
						 const u8 *addr)
{
	struct wpa_ssid *ssid, *s;
	u8 *n;
	size_t i;
	int found = 0;

	ssid = wpa_s->current_ssid;
	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GO ||
	    !ssid->p2p_persistent_group)
		return;

	for (s = wpa_s->parent->conf->ssid; s; s = s->next) {
		if (s->disabled != 2 || s->mode != WPAS_MODE_P2P_GO)
			continue;

		if (s->ssid_len == ssid->ssid_len &&
		    os_memcmp(s->ssid, ssid->ssid, s->ssid_len) == 0)
			break;
	}

	if (s == NULL)
		return;

	for (i = 0; s->p2p_client_list && i < s->num_p2p_clients; i++) {
		if (os_memcmp(s->p2p_client_list + i * ETH_ALEN, addr,
			      ETH_ALEN) != 0)
			continue;

		if (i == s->num_p2p_clients - 1)
			return; /* already the most recent entry */

		/* move the entry to mark it most recent */
		os_memmove(s->p2p_client_list + i * ETH_ALEN,
			   s->p2p_client_list + (i + 1) * ETH_ALEN,
			   (s->num_p2p_clients - i - 1) * ETH_ALEN);
		os_memcpy(s->p2p_client_list +
			  (s->num_p2p_clients - 1) * ETH_ALEN, addr, ETH_ALEN);
		found = 1;
		break;
	}

	if (!found && s->num_p2p_clients < P2P_MAX_STORED_CLIENTS) {
		n = os_realloc_array(s->p2p_client_list,
				     s->num_p2p_clients + 1, ETH_ALEN);
		if (n == NULL)
			return;
		os_memcpy(n + s->num_p2p_clients * ETH_ALEN, addr, ETH_ALEN);
		s->p2p_client_list = n;
		s->num_p2p_clients++;
	} else if (!found) {
		/* Not enough room for an additional entry - drop the oldest
		 * entry */
		os_memmove(s->p2p_client_list,
			   s->p2p_client_list + ETH_ALEN,
			   (s->num_p2p_clients - 1) * ETH_ALEN);
		os_memcpy(s->p2p_client_list +
			  (s->num_p2p_clients - 1) * ETH_ALEN,
			  addr, ETH_ALEN);
	}
}
#endif	/* CONFIG_P2P_OPTION */

static void wpas_group_formation_completed(struct wpa_supplicant *wpa_s,
					   int success)
{
	struct wpa_ssid *ssid;
	int client;
#ifdef	CONFIG_P2P_OPTION
	const char *ssid_txt;
	int persistent;
#endif	/* CONFIG_P2P_OPTION */
	u8 go_dev_addr[ETH_ALEN];
	int network_id = -1;

	/*
	 * This callback is likely called for the main interface. Update wpa_s
	 * to use the group interface if a new interface was created for the
	 * group.
	 */
	if (wpa_s->global->p2p_group_formation)
		wpa_s = wpa_s->global->p2p_group_formation;
	if (wpa_s->p2p_go_group_formation_completed) {
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
	} else if (wpa_s->p2p_in_provisioning && !success) {
		wpa_dbg(wpa_s, MSG_DEBUG,
			"P2P: Stop provisioning state due to failure");
		wpa_s->p2p_in_provisioning = 0;
	}
#ifdef	CONFIG_P2P_OPTION
	wpa_s->p2p_in_invitation = 0;
#endif	/* CONFIG_P2P_OPTION */

	if (!success) {
		da16x_p2p_prt("[%s] " P2P_EVENT_GROUP_FORMATION_FAILURE "\n",
				__func__);

		wpas_p2p_group_delete(wpa_s,
				      P2P_GROUP_REMOVAL_FORMATION_FAILED);
		return;
	}

	da16x_p2p_prt("[%s] " P2P_EVENT_GROUP_FORMATION_SUCCESS "\n", __func__);

	ssid = wpa_s->current_ssid;
	if (ssid && ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION) {
		ssid->mode = WPAS_MODE_P2P_GO;
		p2p_group_notif_formation_done(wpa_s->p2p_group);

#ifdef	CONFIG_AP
#ifdef	CONFIG_ACL_P2P
		wpa_supplicant_ap_mac_addr_filter(wpa_s, NULL);
#endif	/* CONFIG_ACL */
#endif	/* CONFIG_AP */
	}

#ifdef	CONFIG_P2P_OPTION
	persistent = 0;
#endif	/* CONFIG_P2P_OPTION */
	if (ssid) {
#ifdef	CONFIG_P2P_OPTION
		ssid_txt = wpa_ssid_txt(ssid->ssid, ssid->ssid_len);
#endif	/* CONFIG_P2P_OPTION */
		client = ssid->mode == WPAS_MODE_INFRA;
		if (ssid->mode == WPAS_MODE_P2P_GO) {
#ifdef	CONFIG_P2P_OPTION
			persistent = ssid->p2p_persistent_group;
#endif	/* CONFIG_P2P_OPTION */
			os_memcpy(go_dev_addr, wpa_s->global->p2p_dev_addr,
				  ETH_ALEN);
#ifdef	CONFIG_P2P_OPTION
		} else
			persistent = wpas_p2p_persistent_group(wpa_s,
							       go_dev_addr,
							       ssid->ssid,
							       ssid->ssid_len);
#else
		}
#endif	/* CONFIG_P2P_OPTION */
	} else {
#ifdef	CONFIG_P2P_OPTION
		ssid_txt = "";
#endif	/* CONFIG_P2P_OPTION */
		client = wpa_s->p2p_group_interface ==
			P2P_GROUP_INTERFACE_CLIENT;
		os_memset(go_dev_addr, 0, ETH_ALEN);
	}

	wpa_s->show_group_started = 0;
	if (client) {
		/*
		 * Indicate event only after successfully completed 4-way
		 * handshake, i.e., when the interface is ready for data
		 * packets.
		 */
		wpa_s->show_group_started = 1;
	} else if (ssid && ssid->passphrase == NULL && ssid->psk_set) {
		char psk[65];
		wpa_snprintf_hex(psk, sizeof(psk), ssid->psk, 32);
#ifdef	CONFIG_P2P_OPTION
		da16x_notice_prt(P2P_EVENT_GROUP_STARTED
			       "%s GO ssid=\"%s\" freq=%d psk=%s go_dev_addr="
			       MACSTR "%s\n",
			       wpa_s->ifname, ssid_txt, ssid->frequency, psk,
			       MAC2STR(go_dev_addr),
			       persistent ? " [PERSISTENT]" : "");
#endif	/* CONFIG_P2P_OPTION */
		wpas_p2p_set_group_idle_timeout(wpa_s);
	} else {
#ifdef	CONFIG_P2P_OPTION
		da16x_notice_prt(P2P_EVENT_GROUP_STARTED
			       "%s GO ssid=\"%s\" freq=%d passphrase=\"%s\" "
			       "go_dev_addr=" MACSTR "%s\n",
			       wpa_s->ifname, ssid_txt,
			       ssid ? ssid->frequency : 0,
			       ssid && ssid->passphrase ? ssid->passphrase : "",
			       MAC2STR(go_dev_addr),
			       persistent ? " [PERSISTENT]" : "");
#endif	/* CONFIG_P2P_OPTION */
		wpas_p2p_set_group_idle_timeout(wpa_s);
	}

#ifdef	CONFIG_P2P_OPTION
	if (persistent) {
		network_id = wpas_p2p_store_persistent_group(wpa_s->parent,
							ssid, go_dev_addr);
	} else {
		os_free(wpa_s->global->add_psk);
		wpa_s->global->add_psk = NULL;
	}
#else
	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;
#endif	/* CONFIG_P2P_OPTION */
	if (network_id < 0 && ssid)
		network_id = ssid->id;
	if (!client) {
		os_get_reltime(&wpa_s->global->p2p_go_wait_client);
	}
}


struct send_action_work {
	unsigned int freq;
	u8 dst[ETH_ALEN];
	u8 src[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	size_t len;
	unsigned int wait_time;
	u8 buf[0];
};


static void wpas_p2p_send_action_tx_status(struct wpa_supplicant *wpa_s,
					   unsigned int freq,
					   const u8 *dst, const u8 *src,
					   const u8 *bssid,
					   const u8 *data, size_t data_len,
					   enum offchannel_send_action_result
					   result)
{
	enum p2p_send_action_result res = P2P_SEND_ACTION_SUCCESS;

	if (wpa_s->global->p2p == NULL || wpa_s->global->p2p_disabled)
		return;

	switch (result) {
	case OFFCHANNEL_SEND_ACTION_SUCCESS:
		res = P2P_SEND_ACTION_SUCCESS;
		break;
	case OFFCHANNEL_SEND_ACTION_NO_ACK:
		res = P2P_SEND_ACTION_NO_ACK;
		break;
	case OFFCHANNEL_SEND_ACTION_FAILED:
		res = P2P_SEND_ACTION_FAILED;
		break;
	}

	p2p_send_action_cb(wpa_s->global->p2p, freq, dst, src, bssid, res);

	if (result != OFFCHANNEL_SEND_ACTION_SUCCESS &&
	    wpa_s->pending_pd_before_join &&
	    (os_memcmp(dst, wpa_s->pending_join_dev_addr, ETH_ALEN) == 0 ||
	     os_memcmp(dst, wpa_s->pending_join_iface_addr, ETH_ALEN) == 0) &&
	    wpa_s->p2p_fallback_to_go_neg) {
		wpa_s->pending_pd_before_join = 0;

		da16x_p2p_prt("[%s] P2P: No ACK for PD Req during "
				"p2p_connect-auto\n", __func__);

		wpas_p2p_fallback_to_go_neg(wpa_s, 0);
		return;
	}
}


#if 0	/* by Bright */
static void wpas_send_action_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct send_action_work *awork = work->ctx;

	if (deinit) {
		if (work->started) {
#if 0	/* by Bright */
			eloop_cancel_timeout(wpas_p2p_send_action_work_timeout,
					     wpa_s, NULL);
#else
da16x_p2p_prt("[%s] What do I do for eloop_timer ???\n", __func__);
#endif	/* 0 */
			wpa_s->p2p_send_action_work = NULL;
			offchannel_send_action_done(wpa_s);
		}
		os_free(awork);
		return;
	}

	if (offchannel_send_action(wpa_s, awork->freq, awork->dst, awork->src,
				   awork->bssid, awork->buf, awork->len,
				   awork->wait_time,
				   wpas_p2p_send_action_tx_status, 1) < 0) {
		os_free(awork);
		radio_work_done(work);
		return;
	}
	wpa_s->p2p_send_action_work = work;
}


static int wpas_send_action_work(struct wpa_supplicant *wpa_s,
				 unsigned int freq, const u8 *dst,
				 const u8 *src, const u8 *bssid, const u8 *buf,
				 size_t len, unsigned int wait_time)
{
	struct send_action_work *awork;

	if (wpa_s->p2p_send_action_work) {
		da16x_p2p_prt("[%s] P2P: Cannot schedule new p2p-send-action "
			"work since one is already pending\n", __func__);
		return -1;
	}

	awork = os_zalloc(sizeof(*awork) + len);
	if (awork == NULL)
		return -1;

	awork->freq = freq;
	os_memcpy(awork->dst, dst, ETH_ALEN);
	os_memcpy(awork->src, src, ETH_ALEN);
	os_memcpy(awork->bssid, bssid, ETH_ALEN);
	awork->len = len;
	awork->wait_time = wait_time;
	os_memcpy(awork->buf, buf, len);

#if 0	/* by Bright */
	if (radio_add_work(wpa_s, freq, "p2p-send-action", 0,
			   wpas_send_action_cb, awork) < 0) {
		os_free(awork);
		return -1;
	}
#else
da16x_p2p_prt("[%s] What do I do for radio_work() ???\n", __func__);
#endif	/* 0 */

	return 0;
}
#endif	/* 0 */


static int wpas_send_action(void *ctx, unsigned int freq, const u8 *dst,
			    const u8 *src, const u8 *bssid, const u8 *buf,
			    size_t len, unsigned int wait_time)
{
	struct wpa_supplicant *wpa_s = ctx;

	da16x_p2p_prt("[%s] P2P: Use the frequency [%d] for Action frame TX\n",
			__func__, freq);

	return offchannel_send_action(wpa_s, freq, dst, src, bssid, buf, len,
				      wait_time,
				      wpas_p2p_send_action_tx_status, 1);
}


static void wpas_send_action_done(void *ctx)
{
#if 0	/* by Bright */
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->p2p_send_action_work) {
#if 0	/* by Bright */
		eloop_cancel_timeout(wpas_p2p_send_action_work_timeout,
				     wpa_s, NULL);
#else
da16x_p2p_prt("[%s] What do I do for eloop_timer ???\n", __func__);
#endif	/* 0 */
		os_free(wpa_s->p2p_send_action_work->ctx);
		radio_work_done(wpa_s->p2p_send_action_work);
		wpa_s->p2p_send_action_work = NULL;
	}

	offchannel_send_action_done(wpa_s);
#else
da16x_p2p_prt("[%s] What do I do for radio_work() ???\n", __func__);
#endif	/* 0 */
}


static int wpas_copy_go_neg_results(struct wpa_supplicant *wpa_s,
				    struct p2p_go_neg_results *params)
{
	if (wpa_s->go_params == NULL) {
		wpa_s->go_params = os_malloc(sizeof(*params));
		if (wpa_s->go_params == NULL)
			return -1;
	}
	os_memcpy(wpa_s->go_params, params, sizeof(*params));
	return 0;
}


static void wpas_start_wps_enrollee(struct wpa_supplicant *wpa_s,
				    struct p2p_go_neg_results *res)
{
	da16x_p2p_prt("[%s] P2P: Start WPS Enrollee for peer " MACSTR
		   " dev_addr " MACSTR " wps_method %d\n",
		   __func__,
		   MAC2STR(res->peer_interface_addr),
		   MAC2STR(res->peer_device_addr), res->wps_method);

	da16x_ascii_dump("P2P: Start WPS Enrollee for SSID",
			  res->ssid, res->ssid_len);

#ifdef	CONFIG_AP
	wpa_supplicant_ap_deinit(wpa_s);
#endif	/* CONFIG_AP */
	wpas_copy_go_neg_results(wpa_s, res);

#ifdef	CONFIG_WPS
	if (res->wps_method == WPS_PBC) {
		wpas_wps_start_pbc(wpa_s, res->peer_interface_addr, 1);
	} else {
		u16 dev_pw_id = DEV_PW_DEFAULT;
		if (wpa_s->p2p_wps_method == WPS_PIN_KEYPAD)
			dev_pw_id = DEV_PW_REGISTRAR_SPECIFIED;
		wpas_wps_start_pin(wpa_s, res->peer_interface_addr,
				   wpa_s->p2p_pin, 1, dev_pw_id);
	}
#endif	/* CONFIG_WPS */
}


#ifdef	CONFIG_AP
#ifdef	CONFIG_P2P_OPTION
static void wpas_p2p_add_psk_list(struct wpa_supplicant *wpa_s,
				  struct wpa_ssid *ssid)
{
	struct wpa_ssid *persistent;
	struct psk_list_entry *psk;
	struct hostapd_data *hapd;

	if (!wpa_s->ap_iface)
		return;

	persistent = wpas_p2p_get_persistent(wpa_s->parent, NULL, ssid->ssid,
					     ssid->ssid_len);
	if (persistent == NULL)
		return;

	hapd = wpa_s->ap_iface->bss[0];

	dl_list_for_each(psk, &persistent->psk_list, struct psk_list_entry,
			 list) {
		struct hostapd_wpa_psk *hpsk;

		da16x_p2p_prt("[%s] P2P: Add persistent group PSK entry for "
			MACSTR " psk=%d\n",
			__func__, MAC2STR(psk->addr), psk->p2p);

		hpsk = os_zalloc(sizeof(*hpsk));
		if (hpsk == NULL)
			break;
		os_memcpy(hpsk->psk, psk->psk, PMK_LEN);
		if (psk->p2p)
			os_memcpy(hpsk->p2p_dev_addr, psk->addr, ETH_ALEN);
		else
			os_memcpy(hpsk->addr, psk->addr, ETH_ALEN);
		hpsk->next = hapd->conf->ssid.wpa_psk;
		hapd->conf->ssid.wpa_psk = hpsk;
	}
}
#endif	/* CONFIG_P2P_OPTION */

static void p2p_go_configured(void *ctx, void *data)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct p2p_go_neg_results *params = data;
	struct wpa_ssid *ssid;
	int network_id = -1;

	ssid = wpa_s->current_ssid;
	if (ssid && ssid->mode == WPAS_MODE_P2P_GO) {
		da16x_p2p_prt("[%s] P2P: Group setup without provisioning\n",
				__func__);

		if (wpa_s->global->p2p_group_formation == wpa_s)
			wpa_s->global->p2p_group_formation = NULL;
		if (os_strlen(params->passphrase) > 0) {
#ifdef	CONFIG_P2P_OPTION
			da16x_notice_prt(P2P_EVENT_GROUP_STARTED
					"%s GO ssid=\"%s\" freq=%d "
					"passphrase=\"%s\" go_dev_addr=" MACSTR
					"%s\n", wpa_s->ifname,
					wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
					ssid->frequency, params->passphrase,
					MAC2STR(wpa_s->global->p2p_dev_addr),
					params->persistent_group ?
					" [PERSISTENT]" : "");
#endif	/* CONFIG_P2P_OPTION */
		} else {
			char psk[65];

			wpa_snprintf_hex(psk, sizeof(psk), params->psk,
					 sizeof(params->psk));
#ifdef	CONFIG_P2P_OPTION
			da16x_notice_prt(P2P_EVENT_GROUP_STARTED
					"%s GO ssid=\"%s\" freq=%d psk=%s "
					"go_dev_addr=" MACSTR "%s\n", wpa_s->ifname,
					wpa_ssid_txt(ssid->ssid, ssid->ssid_len),
					ssid->frequency, psk,
					MAC2STR(wpa_s->global->p2p_dev_addr),
					params->persistent_group ?
					" [PERSISTENT]" : "");
#endif	/* CONFIG_P2P_OPTION */
		}

		os_get_reltime(&wpa_s->global->p2p_go_wait_client);
#ifdef	CONFIG_P2P_OPTION
		if (params->persistent_group) {
			network_id = wpas_p2p_store_persistent_group(
				wpa_s->parent, ssid,
				wpa_s->global->p2p_dev_addr);
			wpas_p2p_add_psk_list(wpa_s, ssid);
		}
#endif	/* CONFIG_P2P_OPTION */
		if (network_id < 0)
			network_id = ssid->id;
		wpas_p2p_set_group_idle_timeout(wpa_s);

		if (wpa_s->p2p_first_connection_timeout) {
			da16x_p2p_prt("[%s] P2P: Start group formation timeout "
				     "of %d seconds until first data "
				     "connection on GO\n",
				__func__,
				wpa_s->p2p_first_connection_timeout);

			wpa_s->p2p_go_group_formation_completed = 0;
			wpa_s->global->p2p_group_formation = wpa_s;

			da16x_eloop_cancel_timeout(
				wpas_p2p_group_formation_timeout,
				wpa_s, NULL);
			da16x_eloop_register_timeout(
				wpa_s->p2p_first_connection_timeout, 0,
				wpas_p2p_group_formation_timeout,
				wpa_s, NULL);
		}

		return;
	}

	da16x_p2p_prt("[%s] P2P: Setting up WPS for GO provisioning\n",
			__func__);

#ifdef	CONFIG_AP
#ifdef	CONFIG_ACL_P2P
	if (wpa_supplicant_ap_mac_addr_filter(wpa_s,
					      params->peer_interface_addr)) {
		da16x_p2p_prt("[%s] P2P: Failed to setup MAC address "
			   "filtering\n", __func__);
		return;
	}
#endif	/* CONFIG_ACL */
#endif	/* CONFIG_AP */

	if (params->wps_method == WPS_PBC) {
		wpa_supplicant_ap_wps_pbc(wpa_s, params->peer_interface_addr,
					  params->peer_device_addr);
	} else if (wpa_s->p2p_pin[0])
		wpa_supplicant_ap_wps_pin(wpa_s, params->peer_interface_addr,
					  wpa_s->p2p_pin, NULL, 0, 0);
	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;
}
#endif	/* CONFIG_AP */


static void wpas_start_wps_go(struct wpa_supplicant *wpa_s,
			      struct p2p_go_neg_results *params,
			      int group_formation)
{
	struct wpa_ssid *ssid;

	da16x_p2p_prt("[%s] P2P: Starting GO\n", __func__);

	if (wpas_copy_go_neg_results(wpa_s, params) < 0) {
		da16x_p2p_prt("[%s] P2P: Could not copy GO Negotiation "
			"results\n", __func__);
		return;
	}
#if 1	/* by Shingu 20160726 (P2P Config) */
	if (wpa_s->conf->p2p_ssid && wpa_s->conf->p2p_passphrase) {
		os_memcpy(params->ssid, wpa_s->conf->p2p_ssid,
			  wpa_s->conf->p2p_ssid_len);
		params->ssid_len = wpa_s->conf->p2p_ssid_len;
		strcpy(params->passphrase, wpa_s->conf->p2p_passphrase);
	}
#endif	/* 1 */

	ssid = wpa_config_add_network(wpa_s->conf, wpa_s->ifname,
				      FIXED_NETWORK_ID_NONE);
	if (ssid == NULL) {
		da16x_p2p_prt("[%s] P2P: Could not add network for GO\n",
				__func__);
		return;
	}

	wpa_s->show_group_started = 0;

	wpa_config_set_network_defaults(ssid);
	ssid->temporary = 1;
	ssid->p2p_group = 1;
#ifdef	CONFIG_P2P_OPTION
	ssid->p2p_persistent_group = params->persistent_group;
#endif	/* CONFIG_P2P_OPTION */
	ssid->mode = group_formation ? WPAS_MODE_P2P_GROUP_FORMATION :
		WPAS_MODE_P2P_GO;
	ssid->frequency = params->freq;
	ssid->ht40 = params->ht40;
//	ssid->vht = params->vht;
	ssid->ssid = os_zalloc(params->ssid_len + 1);
	if (ssid->ssid) {
		os_memcpy(ssid->ssid, params->ssid, params->ssid_len);
		ssid->ssid_len = params->ssid_len;
	}
	ssid->auth_alg = WPA_AUTH_ALG_OPEN;
	ssid->key_mgmt = WPA_KEY_MGMT_PSK;
	ssid->proto = WPA_PROTO_RSN;
	ssid->pairwise_cipher = WPA_CIPHER_CCMP;
	if (os_strlen(params->passphrase) > 0) {
		ssid->passphrase = os_strdup(params->passphrase);
		if (ssid->passphrase == NULL) {
			da16x_p2p_prt("[%s] P2P: Failed to copy passphrase "
				"for GO\n", __func__);

			wpa_config_remove_network(wpa_s->conf, ssid->id);
			return;
		}
	} else {
		ssid->passphrase = NULL;
	}

	ssid->psk_set = params->psk_set;
	if (ssid->psk_set)
		os_memcpy(ssid->psk, params->psk, sizeof(ssid->psk));
	else if (ssid->passphrase)
		wpa_config_update_psk(ssid);
#ifdef	CONFIG_AP
	ssid->ap_max_inactivity = wpa_s->conf->ap_max_inactivity;

	wpa_s->ap_configured_cb = p2p_go_configured;
	wpa_s->ap_configured_cb_ctx = wpa_s;
	wpa_s->ap_configured_cb_data = wpa_s->go_params;
#endif	/* CONFIG_AP */
	wpa_s->scan_req = NORMAL_SCAN_REQ;
	wpa_s->connect_without_scan = ssid;
	wpa_s->reassociate = 1;
	wpa_s->disconnected = 0;

	da16x_p2p_prt("[%s] P2P: Request scan (that will be skipped) to "
		"start GO)\n", __func__);

	wpa_supplicant_req_scan(wpa_s, 0, 0);
}


static void wpas_p2p_clone_config(struct wpa_supplicant *dst,
				  const struct wpa_supplicant *src)
{
	struct wpa_config *d;
	const struct wpa_config *s;

	d = dst->conf;
	s = src->conf;

#define C(n) if (s->n) d->n = os_strdup(s->n)
	C(device_name);
	C(manufacturer);
	C(model_name);
	C(model_number);
	C(serial_number);
	C(config_methods);
#undef C

	os_memcpy(d->device_type, s->device_type, WPS_DEV_TYPE_LEN);
	os_memcpy(d->sec_device_type, s->sec_device_type,
		  sizeof(d->sec_device_type));
	d->num_sec_device_types = s->num_sec_device_types;

	d->p2p_group_idle = s->p2p_group_idle;
	d->p2p_intra_bss = s->p2p_intra_bss;
#ifdef	CONFIG_P2P_OPTION
	d->persistent_reconnect = s->persistent_reconnect;
#endif	/* CONFIG_P2P_OPTION */
	d->max_num_sta = s->max_num_sta;
	d->pbc_in_m1 = s->pbc_in_m1;
#ifdef CONFIG_IGNOR_OLD_SCAN
	d->ignore_old_scan_res = s->ignore_old_scan_res;
#endif /* CONFIG_IGNOR_OLD_SCAN */
	d->beacon_int = s->beacon_int;
	d->dtim_period = s->dtim_period;
	d->disassoc_low_ack = s->disassoc_low_ack;
}


static void wpas_p2p_get_group_ifname(struct wpa_supplicant *wpa_s,
				      char *ifname, size_t len)
{
	char *ifname_ptr = wpa_s->ifname;

	if (os_strncmp(wpa_s->ifname, P2P_MGMT_DEVICE_PREFIX,
		       os_strlen(P2P_MGMT_DEVICE_PREFIX)) == 0) {
		ifname_ptr = os_strrchr(wpa_s->ifname, '-') + 1;
	}

	os_snprintf(ifname, len, "p2p-%s-%d", ifname_ptr, wpa_s->p2p_group_idx);
	if (os_strlen(ifname) >= IFNAMSIZ &&
	    os_strlen(wpa_s->ifname) < IFNAMSIZ) {
		/* Try to avoid going over the IFNAMSIZ length limit */
		os_snprintf(ifname, len, "p2p-%d", wpa_s->p2p_group_idx);
	}
}


static int wpas_p2p_add_group_interface(struct wpa_supplicant *wpa_s,
					enum wpa_driver_if_type type)
{
	char ifname[120], force_ifname[120];

	if (wpa_s->pending_interface_name[0]) {
		da16x_p2p_prt("[%s] P2P: Pending virtual interface exists "
			   "- skip creation of a new one\n", __func__);

		if (is_zero_ether_addr(wpa_s->pending_interface_addr)) {
			da16x_p2p_prt("[%s] P2P: Pending virtual address "
				   "unknown?! ifname='%s'\n",
				   __func__,
				   wpa_s->pending_interface_name);
			return -1;
		}
		return 0;
	}

	wpas_p2p_get_group_ifname(wpa_s, ifname, sizeof(ifname));
	force_ifname[0] = '\0';

	da16x_p2p_prt("[%s] P2P: Create a new interface %s for the group\n",
		   	__func__, ifname);

	wpa_s->p2p_group_idx++;

	wpa_s->pending_interface_type = type;
	if (wpa_drv_if_add(wpa_s, type, ifname, NULL, NULL, force_ifname,
			   wpa_s->pending_interface_addr, NULL) < 0) {
		da16x_p2p_prt("[%s] P2P: Failed to create new group interface\n",
				__func__);
		return -1;
	}

	if (force_ifname[0]) {
		da16x_p2p_prt("[%s] P2P: Driver forced interface name %s\n",
			   __func__, force_ifname);

		os_strlcpy(wpa_s->pending_interface_name, force_ifname,
			   sizeof(wpa_s->pending_interface_name));
	} else {
		os_strlcpy(wpa_s->pending_interface_name, ifname,
			   sizeof(wpa_s->pending_interface_name));
	}

	da16x_p2p_prt("[%s] P2P: Created pending virtual interface %s addr "
		   	MACSTR "\n",
			__func__, wpa_s->pending_interface_name,
		   	MAC2STR(wpa_s->pending_interface_addr));

	return 0;
}


static void wpas_p2p_remove_pending_group_interface(
	struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->pending_interface_name[0] ||
	    is_zero_ether_addr(wpa_s->pending_interface_addr))
		return; /* No pending virtual interface */

	da16x_p2p_prt("[%s] P2P: Removing pending group interface %s\n",
			__func__, wpa_s->pending_interface_name);

	wpa_drv_if_remove(wpa_s,
			 (enum wpa_driver_if_type)wpa_s->pending_interface_type,
			  wpa_s->pending_interface_name);
	os_memset(wpa_s->pending_interface_addr, 0, ETH_ALEN);
	wpa_s->pending_interface_name[0] = '\0';
}


static struct wpa_supplicant *
wpas_p2p_init_group_interface(struct wpa_supplicant *wpa_s, int go)
{
	struct wpa_interface iface;
	struct wpa_supplicant *group_wpa_s;

	if (!wpa_s->pending_interface_name[0]) {
		da16x_p2p_prt("[%s] P2P: No pending group interface\n",
					__func__);

		if (!wpas_p2p_create_iface(wpa_s))
			return NULL;
		/*
		 * Something has forced us to remove the pending interface; try
		 * to create a new one and hope for the best that we will get
		 * the same local address.
		 */
		if (wpas_p2p_add_group_interface(wpa_s, go ? WPA_IF_P2P_GO :
						 WPA_IF_P2P_CLIENT) < 0)
			return NULL;
	}

	os_memset(&iface, 0, sizeof(iface));
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;

	iface.driver_param = wpa_s->conf->driver_param;
	group_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface, NULL);
	if (group_wpa_s == NULL) {
		da16x_p2p_prt("[%s] P2P: Failed to create new "
			   "wpa_supplicant interface\n", __func__);

		return NULL;
	}
	wpa_s->pending_interface_name[0] = '\0';
	group_wpa_s->parent = wpa_s;
	group_wpa_s->p2p_group_interface = go ? P2P_GROUP_INTERFACE_GO :
		P2P_GROUP_INTERFACE_CLIENT;
	wpa_s->global->p2p_group_formation = group_wpa_s;

	wpas_p2p_clone_config(group_wpa_s, wpa_s);

	return group_wpa_s;
}

static void wpas_p2p_group_formation_timeout(void *eloop_ctx,
					     void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	da16x_p2p_prt("[%s] P2P: Group Formation timed out\n", __func__);

	wpas_p2p_group_formation_failed(wpa_s);
}

void wpas_p2p_group_formation_failed(struct wpa_supplicant *wpa_s)
{
	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				  wpa_s, NULL);

	if (wpa_s->global->p2p)
		p2p_group_formation_failed(wpa_s->global->p2p);

	wpas_group_formation_completed(wpa_s, 0);
}


static void wpas_p2p_grpform_fail_after_wps(struct wpa_supplicant *wpa_s)
{
	da16x_p2p_prt("[%s] P2P: Reject group formation due to WPS "
			"provisioning failure\n", __func__);

	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				  wpa_s, NULL);
	da16x_eloop_register_timeout(0, 0, wpas_p2p_group_formation_timeout,
				    wpa_s, NULL);

	wpa_s->global->p2p_fail_on_wps_complete = 0;
}


void wpas_p2p_ap_setup_failed(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->global->p2p_group_formation != wpa_s)
		return;

	/* Speed up group formation timeout since this cannot succeed */
	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				  wpa_s, NULL);
	da16x_eloop_register_timeout(0, 0, wpas_p2p_group_formation_timeout,
				    wpa_s, NULL);
}


static void wpas_go_neg_completed(void *ctx, struct p2p_go_neg_results *res)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}

#if 1	/* by Shingu */
	wpa_bss_flush(wpa_s);
#endif	/* 1 */

	if (res->status) {
		da16x_p2p_prt("[%s] " P2P_EVENT_GO_NEG_FAILURE "status=%d\n",
				__func__, res->status);

#if 0	/* by Bright */
		wpas_notify_p2p_go_neg_completed(wpa_s, res);
#endif	/* 0 */
		wpas_p2p_remove_pending_group_interface(wpa_s);
		return;
	}

	da16x_p2p_prt("[%s] " P2P_EVENT_GO_NEG_SUCCESS "role=%s "
		     "freq=%d ht40=%d peer_dev=" MACSTR " peer_iface=" MACSTR
		     " wps_method=%s\n", __func__,
		     res->role_go ? "GO" : "client", res->freq, res->ht40,
		     MAC2STR(res->peer_device_addr),
		     MAC2STR(res->peer_interface_addr),
		     p2p_wps_method_text(res->wps_method));

#if 0	/* by Bright */
	wpas_notify_p2p_go_neg_completed(wpa_s, res);
#endif	/* 0 */

#ifdef	CONFIG_P2P_OPTION
	if (res->role_go && wpa_s->p2p_persistent_id >= 0) {
		struct wpa_ssid *ssid;
		ssid = wpa_config_get_network(wpa_s->conf,
					      wpa_s->p2p_persistent_id);
		if (ssid && ssid->disabled == 2 &&
		    ssid->mode == WPAS_MODE_P2P_GO && ssid->passphrase) {
			size_t len = os_strlen(ssid->passphrase);

			da16x_p2p_prt("[%s] P2P: Override passphrase based "
				   "on requested persistent group\n",
					__func__);

			os_memcpy(res->passphrase, ssid->passphrase, len);
			res->passphrase[len] = '\0';
		}
	}
#endif	/* CONFIG_P2P_OPTION */
	
#if 1   /* by Shingu 20160707 (P2P Optimize) */
        p2p_remove_device(wpa_s->global->p2p, NULL, 1);
#endif  /* 1 */

#ifdef	CONFIG_CONCURRENT /* by Shingu 20160906 (Concurrent) */
	if (get_run_mode() == STA_P2P_CONCURRENT_MODE) {
		struct wpa_supplicant *sta_wpa_s = select_sta0(wpa_s);

		if (!res->role_go &&
		    sta_wpa_s->wpa_state == WPA_COMPLETED &&
		    sta_wpa_s->assoc_freq != res->freq) {
			da16x_notice_prt("P2P GO (peer) channel(%d) is different"
					" from STA interface channel(%d).\n",
					res->freq == 2484 ? 
					14 : (res->freq - 2407)/5,
					sta_wpa_s->assoc_freq == 2484 ? 
					14 : (sta_wpa_s->assoc_freq - 2407)/5);

			wpas_p2p_remove_pending_group_interface(wpa_s);
			wpas_p2p_group_formation_failed(wpa_s);

			return;    
		}
	}
#endif	/* CONFIG_CONCURRENT */


	if (wpa_s->create_p2p_iface) {
		struct wpa_supplicant *group_wpa_s =
			wpas_p2p_init_group_interface(wpa_s, res->role_go);

		if (group_wpa_s == NULL) {
			wpas_p2p_remove_pending_group_interface(wpa_s);

			wpas_p2p_group_formation_failed(wpa_s);
			return;
		}
		if (group_wpa_s != wpa_s) {
			os_memcpy(group_wpa_s->p2p_pin, wpa_s->p2p_pin,
				  sizeof(group_wpa_s->p2p_pin));
			group_wpa_s->p2p_wps_method = wpa_s->p2p_wps_method;
		}
		os_memset(wpa_s->pending_interface_addr, 0, ETH_ALEN);
		wpa_s->pending_interface_name[0] = '\0';
		group_wpa_s->p2p_in_provisioning = 1;

		if (res->role_go)
			wpas_start_wps_go(group_wpa_s, res, 1);
		else
			wpas_start_wps_enrollee(group_wpa_s, res);
	} else {
		wpa_s->p2p_in_provisioning = 1;
		wpa_s->global->p2p_group_formation = wpa_s;

		if (res->role_go)
			wpas_start_wps_go(wpa_s, res, 1);
		else
			wpas_start_wps_enrollee(ctx, res);
	}

	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s,
				  NULL);
	da16x_eloop_register_timeout(15 + res->peer_config_timeout / 100,
				    (res->peer_config_timeout % 100) * 10000,
				    wpas_p2p_group_formation_timeout, wpa_s,
				    NULL);
}


static void wpas_go_neg_req_rx(void *ctx, const u8 *src, u16 dev_passwd_id)
{
#if 0	/* by Bright */
	struct wpa_supplicant *wpa_s = ctx;
#endif	/* 0 */

	da16x_p2p_prt("[%s] " P2P_EVENT_GO_NEG_REQUEST MACSTR
			" dev_passwd_id=%u\n",
			__func__, MAC2STR(src), dev_passwd_id);

#if 0	/* by Bright */
	wpas_notify_p2p_go_neg_req(wpa_s, src, dev_passwd_id);
#endif	/* 0 */
}


static void wpas_dev_found(void *ctx, const u8 *addr,
			   const struct p2p_peer_info *info,
			   int new_device)
{
#ifdef	CONFIG_WIFI_DISPLAY
	char devtype[WPS_DEV_TYPE_BUFSIZE];
	char *wfd_dev_info_hex = NULL;

	wfd_dev_info_hex = wifi_display_subelem_hex(info->wfd_subelems,
						    WFD_SUBELEM_DEVICE_INFO);
#endif	/* CONFIG_WIFI_DISPLAY */

	da16x_notice_prt("["P2P_EVENT_DEVICE_FOUND"]\nAddr: " MACSTR2 "\n"
			"Name: %s\n"
#ifdef	CONFIG_WIFI_DISPLAY
			"p2p_dev_addr=" MACSTR "\n"
			"pri_dev_type=%s\n"
			"config_methods=0x%x\n"
			"dev_capab=0x%x\n"
			"group_capab=0x%x\n"
			"%s%s\n"
#endif	/* CONFIG_WIFI_DISPLAY */
			"\n",
			MAC2STR(addr),
			info->device_name
#ifdef	CONFIG_WIFI_DISPLAY
			,
			MAC2STR(info->p2p_device_addr),
			wps_dev_type_bin2str(info->pri_dev_type, devtype,
					     sizeof(devtype)),
			info->config_methods,
			info->dev_capab,
			info->group_capab,
			wfd_dev_info_hex ? "wfd_dev_info=0x" : "",
			wfd_dev_info_hex ? wfd_dev_info_hex : ""
#endif	/* CONFIG_WIFI_DISPLAY */
			);

#ifdef	CONFIG_WIFI_DISPLAY
	os_free(wfd_dev_info_hex);
#endif	/* CONFIG_WIFI_DISPLAY */
}

static void wpas_dev_lost(void *ctx, const u8 *dev_addr)
{
	da16x_notice_prt("["P2P_EVENT_DEVICE_LOST"]\nAddr: " MACSTR 
			"\n\n", MAC2STR(dev_addr));
}


static void wpas_find_stopped(void *ctx)
{
	da16x_p2p_prt("[%s] " P2P_EVENT_FIND_STOPPED "\n", __func__);
}

static int wpas_start_listen(void *ctx, unsigned int freq,
			     unsigned int duration,
			     const struct wpabuf *probe_resp_ie)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_drv_probe_req_report(wpa_s, 1) < 0) {
		da16x_p2p_prt("[%s] P2P: Failed to request the driver to "
			   "report Rx Probe Request frames\n",
				__func__);

		return -1;
	}

	wpa_s->pending_listen_freq = freq;
	wpa_s->pending_listen_duration = duration;

	if (wpa_drv_remain_on_channel(wpa_s, freq, duration) < 0) {
		da16x_p2p_prt("[%s] P2P: Failed to request the driver "
			   "to remain on channel (%u MHz) for listen state\n",
				__func__, freq);

		wpa_s->pending_listen_freq = 0;
		return -1;
	}

	wpa_s->off_channel_freq = 0;
	wpa_s->roc_waiting_drv_freq = freq;

	return 0;
}


static void wpas_stop_listen(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}
	wpa_drv_set_ap_wps_ie(wpa_s, NULL, NULL, NULL);
	wpa_drv_probe_req_report(wpa_s, 0);
}


static int wpas_send_probe_resp(void *ctx, const struct wpabuf *buf)
{
	struct wpa_supplicant *wpa_s = ctx;
	return wpa_drv_send_mlme(wpa_s, wpabuf_head(buf), wpabuf_len(buf), 1);
}


#ifdef	CONFIG_P2P_OPTION
/*
 * DNS Header section is used only to calculate compression pointers, so the
 * contents of this data does not matter, but the length needs to be reserved
 * in the virtual packet.
 */
#define DNS_HEADER_LEN 12

/*
 * 27-octet in-memory packet from P2P specification containing two implied
 * queries for _tcp.lcoal. PTR IN and _udp.local. PTR IN
 */
#define P2P_SD_IN_MEMORY_LEN 27

static int p2p_sd_dns_uncompress_label(char **upos, char *uend, u8 *start,
				       u8 **spos, const u8 *end)
{
	while (*spos < end) {
		u8 val = ((*spos)[0] & 0xc0) >> 6;
		int len;

		if (val == 1 || val == 2) {
			/* These are reserved values in RFC 1035 */
			da16x_p2p_prt("[%s] P2P: Invalid domain name "
				   "sequence starting with 0x%x\n",
					__func__, val);
			return -1;
		}

		if (val == 3) {
			u16 offset;
			u8 *spos_tmp;

			/* Offset */
			if (*spos + 2 > end) {
				da16x_p2p_prt("[%s] P2P: No room for full "
					   "DNS offset field\n", __func__);
				return -1;
			}

			offset = (((*spos)[0] & 0x3f) << 8) | (*spos)[1];
			if (offset >= *spos - start) {
				da16x_p2p_prt("[%s] P2P: Invalid DNS "
					   "pointer offset %u\n",
						__func__, offset);
				return -1;
			}

			(*spos) += 2;
			spos_tmp = start + offset;
			return p2p_sd_dns_uncompress_label(upos, uend, start,
							   &spos_tmp,
							   *spos - 2);
		}

		/* Label */
		len = (*spos)[0] & 0x3f;
		if (len == 0)
			return 0;

		(*spos)++;
		if (*spos + len > end) {
			da16x_p2p_prt("[%s] P2P: Invalid domain name "
				   "sequence - no room for label with length "
				   "%u\n", __func__, len);
			return -1;
		}

		if (*upos + len + 2 > uend)
			return -2;

		os_memcpy(*upos, *spos, len);
		*spos += len;
		*upos += len;
		(*upos)[0] = '.';
		(*upos)++;
		(*upos)[0] = '\0';
	}

	return 0;
}


/* Uncompress domain names per RFC 1035 using the P2P SD in-memory packet.
 * Returns -1 on parsing error (invalid input sequence), -2 if output buffer is
 * not large enough */
static int p2p_sd_dns_uncompress(char *buf, size_t buf_len, const u8 *msg,
				 size_t msg_len, size_t offset)
{
	/* 27-octet in-memory packet from P2P specification */
	const char *prefix = "\x04_tcp\x05local\x00\x00\x0C\x00\x01"
		"\x04_udp\xC0\x11\x00\x0C\x00\x01";
	u8 *tmp, *end, *spos;
	char *upos, *uend;
	int ret = 0;

	if (buf_len < 2)
		return -1;
	if (offset > msg_len)
		return -1;

	tmp = os_malloc(DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN + msg_len);
	if (tmp == NULL)
		return -1;
	spos = tmp + DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN;
	end = spos + msg_len;
	spos += offset;

	os_memset(tmp, 0, DNS_HEADER_LEN);
	os_memcpy(tmp + DNS_HEADER_LEN, prefix, P2P_SD_IN_MEMORY_LEN);
	os_memcpy(tmp + DNS_HEADER_LEN + P2P_SD_IN_MEMORY_LEN, msg, msg_len);

	upos = buf;
	uend = buf + buf_len;

	ret = p2p_sd_dns_uncompress_label(&upos, uend, tmp, &spos, end);
	if (ret) {
		os_free(tmp);
		return ret;
	}

	if (upos == buf) {
		upos[0] = '.';
		upos[1] = '\0';
	} else if (upos[-1] == '.')
		upos[-1] = '\0';

	os_free(tmp);
	return 0;
}


static struct p2p_srv_bonjour *
wpas_p2p_service_get_bonjour(struct wpa_supplicant *wpa_s,
			     const struct wpabuf *query)
{
	struct p2p_srv_bonjour *bsrv;
	size_t len;

	len = wpabuf_len(query);
	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (len == wpabuf_len(bsrv->query) &&
		    os_memcmp(wpabuf_head(query), wpabuf_head(bsrv->query),
			      len) == 0)
			return bsrv;
	}
	return NULL;
}


static struct p2p_srv_upnp *
wpas_p2p_service_get_upnp(struct wpa_supplicant *wpa_s, u8 version,
			  const char *service)
{
	struct p2p_srv_upnp *usrv;

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (version == usrv->version &&
		    os_strcmp(service, usrv->service) == 0)
			return usrv;
	}
	return NULL;
}


static void wpas_sd_add_proto_not_avail(struct wpabuf *resp, u8 srv_proto,
					u8 srv_trans_id)
{
	u8 *len_pos;

	if (wpabuf_tailroom(resp) < 5)
		return;

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, srv_proto);
	wpabuf_put_u8(resp, srv_trans_id);
	/* Status Code */
	wpabuf_put_u8(resp, P2P_SD_PROTO_NOT_AVAILABLE);
	/* Response Data: empty */
	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}


static void wpas_sd_all_bonjour(struct wpa_supplicant *wpa_s,
				struct wpabuf *resp, u8 srv_trans_id)
{
	struct p2p_srv_bonjour *bsrv;
	u8 *len_pos;

	da16x_p2p_prt("[%s] P2P: SD Request for all Bonjour services\n",
			__func__);

	if (dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
		da16x_p2p_prt("[%s] P2P: Bonjour protocol not available\n",
				__func__);
		return;
	}

	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (wpabuf_tailroom(resp) <
		    5 + wpabuf_len(bsrv->query) + wpabuf_len(bsrv->resp))
			return;
		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);
		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);

		da16x_ascii_dump("P2P: Matching Bonjour service",
				  wpabuf_head(bsrv->resp),
				  wpabuf_len(bsrv->resp));

		/* Response Data */
		wpabuf_put_buf(resp, bsrv->query); /* Key */
		wpabuf_put_buf(resp, bsrv->resp); /* Value */
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos -
			     2);
	}
}


static int match_bonjour_query(struct p2p_srv_bonjour *bsrv, const u8 *query,
			       size_t query_len)
{
	char str_rx[256], str_srv[256];

	if (query_len < 3 || wpabuf_len(bsrv->query) < 3)
		return 0; /* Too short to include DNS Type and Version */
	if (os_memcmp(query + query_len - 3,
		      wpabuf_head_u8(bsrv->query) + wpabuf_len(bsrv->query) - 3,
		      3) != 0)
		return 0; /* Mismatch in DNS Type or Version */
	if (query_len == wpabuf_len(bsrv->query) &&
	    os_memcmp(query, wpabuf_head(bsrv->query), query_len - 3) == 0)
		return 1; /* Binary match */

	if (p2p_sd_dns_uncompress(str_rx, sizeof(str_rx), query, query_len - 3,
				  0))
		return 0; /* Failed to uncompress query */
	if (p2p_sd_dns_uncompress(str_srv, sizeof(str_srv),
				  wpabuf_head(bsrv->query),
				  wpabuf_len(bsrv->query) - 3, 0))
		return 0; /* Failed to uncompress service */

	return os_strcmp(str_rx, str_srv) == 0;
}


static void wpas_sd_req_bonjour(struct wpa_supplicant *wpa_s,
				struct wpabuf *resp, u8 srv_trans_id,
				const u8 *query, size_t query_len)
{
	struct p2p_srv_bonjour *bsrv;
	u8 *len_pos;
	int matches = 0;

	da16x_ascii_dump("P2P: SD Request for Bonjour", query, query_len);

	if (dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
		da16x_p2p_prt("[%s] P2P: Bonjour protocol not available\n",
				__func__);

		wpas_sd_add_proto_not_avail(resp, P2P_SERV_BONJOUR,
					    srv_trans_id);
		return;
	}

	if (query_len == 0) {
		wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
		return;
	}

	dl_list_for_each(bsrv, &wpa_s->global->p2p_srv_bonjour,
			 struct p2p_srv_bonjour, list) {
		if (!match_bonjour_query(bsrv, query, query_len))
			continue;

		if (wpabuf_tailroom(resp) <
		    5 + query_len + wpabuf_len(bsrv->resp))
			return;

		matches++;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);

		da16x_ascii_dump("P2P: Matching Bonjour service",
				  wpabuf_head(bsrv->resp),
				  wpabuf_len(bsrv->resp));

		/* Response Data */
		wpabuf_put_data(resp, query, query_len); /* Key */
		wpabuf_put_buf(resp, bsrv->resp); /* Value */

		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
	}

	if (matches == 0) {
		da16x_p2p_prt("[%s] P2P: Requested Bonjour service not "
			   "available\n", __func__);

		if (wpabuf_tailroom(resp) < 5)
			return;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_BONJOUR);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_REQUESTED_INFO_NOT_AVAILABLE);
		/* Response Data: empty */
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos -
			     2);
	}
}


static void wpas_sd_all_upnp(struct wpa_supplicant *wpa_s,
			     struct wpabuf *resp, u8 srv_trans_id)
{
	struct p2p_srv_upnp *usrv;
	u8 *len_pos;

	da16x_p2p_prt("[%s] P2P: SD Request for all UPnP services\n",
				__func__);

	if (dl_list_empty(&wpa_s->global->p2p_srv_upnp)) {
		da16x_p2p_prt("[%s] P2P: UPnP protocol not available\n",
				__func__);
		return;
	}

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (wpabuf_tailroom(resp) < 5 + 1 + os_strlen(usrv->service))
			return;

		/* Length (to be filled) */
		len_pos = wpabuf_put(resp, 2);
		wpabuf_put_u8(resp, P2P_SERV_UPNP);
		wpabuf_put_u8(resp, srv_trans_id);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_SUCCESS);
		/* Response Data */
		wpabuf_put_u8(resp, usrv->version);

		da16x_p2p_prt("[%s] P2P: Matching UPnP Service: %s\n",
			   __func__, usrv->service);

		wpabuf_put_str(resp, usrv->service);
		WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
	}
}


static void wpas_sd_req_upnp(struct wpa_supplicant *wpa_s,
			     struct wpabuf *resp, u8 srv_trans_id,
			     const u8 *query, size_t query_len)
{
	struct p2p_srv_upnp *usrv;
	u8 *len_pos;
	u8 version;
	char *str;
	int count = 0;

	da16x_ascii_dump("P2P: SD Request for UPnP", query, query_len);

	if (dl_list_empty(&wpa_s->global->p2p_srv_upnp)) {
		da16x_p2p_prt("[%s] P2P: UPnP protocol not available\n",
				__func__);

		wpas_sd_add_proto_not_avail(resp, P2P_SERV_UPNP, srv_trans_id);
		return;
	}

	if (query_len == 0) {
		wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
		return;
	}

	if (wpabuf_tailroom(resp) < 5)
		return;

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, P2P_SERV_UPNP);
	wpabuf_put_u8(resp, srv_trans_id);

	version = query[0];
	str = os_malloc(query_len);
	if (str == NULL)
		return;
	os_memcpy(str, query + 1, query_len - 1);
	str[query_len - 1] = '\0';

	dl_list_for_each(usrv, &wpa_s->global->p2p_srv_upnp,
			 struct p2p_srv_upnp, list) {
		if (version != usrv->version)
			continue;

		if (os_strcmp(str, "ssdp:all") != 0 &&
		    os_strstr(usrv->service, str) == NULL)
			continue;

		if (wpabuf_tailroom(resp) < 2)
			break;
		if (count == 0) {
			/* Status Code */
			wpabuf_put_u8(resp, P2P_SD_SUCCESS);
			/* Response Data */
			wpabuf_put_u8(resp, version);
		} else
			wpabuf_put_u8(resp, ',');

		count++;

		da16x_p2p_prt("[%s] P2P: Matching UPnP Service: %s\n",
			   __func__, usrv->service);

		if (wpabuf_tailroom(resp) < os_strlen(usrv->service))
			break;
		wpabuf_put_str(resp, usrv->service);
	}
	os_free(str);

	if (count == 0) {
		da16x_p2p_prt("[%s] P2P: Requested UPnP service not "
			   "available\n", __func__);

		/* Status Code */
		wpabuf_put_u8(resp, P2P_SD_REQUESTED_INFO_NOT_AVAILABLE);
		/* Response Data: empty */
	}

	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}


#ifdef CONFIG_WIFI_DISPLAY
static void wpas_sd_req_wfd(struct wpa_supplicant *wpa_s,
			    struct wpabuf *resp, u8 srv_trans_id,
			    const u8 *query, size_t query_len)
{
	const u8 *pos;
	u8 role;
	u8 *len_pos;

	da16x_dump("P2P: SD Request for WFD", query, query_len);

	if (!wpa_s->global->wifi_display) {
		da16x_p2p_prt("[%s] P2P: WFD protocol not available\n",
				__func__);

		wpas_sd_add_proto_not_avail(resp, P2P_SERV_WIFI_DISPLAY,
					    srv_trans_id);
		return;
	}

	if (query_len < 1) {
		da16x_p2p_prt("[%s] P2P: Missing WFD Requested Device Role\n",
				__func__);
		return;
	}

	if (wpabuf_tailroom(resp) < 5)
		return;

	pos = query;
	role = *pos++;
	da16x_p2p_prt("[%s] P2P: WSD for device role 0x%x\n", __func__, role);

	/* TODO: role specific handling */

	/* Length (to be filled) */
	len_pos = wpabuf_put(resp, 2);
	wpabuf_put_u8(resp, P2P_SERV_WIFI_DISPLAY);
	wpabuf_put_u8(resp, srv_trans_id);
	wpabuf_put_u8(resp, P2P_SD_SUCCESS); /* Status Code */

	while (pos < query + query_len) {
		if (*pos < MAX_WFD_SUBELEMS &&
		    wpa_s->global->wfd_subelem[*pos] &&
		    wpabuf_tailroom(resp) >=
		    wpabuf_len(wpa_s->global->wfd_subelem[*pos])) {
			da16x_p2p_prt("[%s] P2P: Add WSD response "
				   "subelement %u\n", __func__, *pos);

			wpabuf_put_buf(resp, wpa_s->global->wfd_subelem[*pos]);
		}
		pos++;
	}

	WPA_PUT_LE16(len_pos, (u8 *) wpabuf_put(resp, 0) - len_pos - 2);
}
#endif /* CONFIG_WIFI_DISPLAY */


static void wpas_sd_request(void *ctx, int freq, const u8 *sa, u8 dialog_token,
			    u16 update_indic, const u8 *tlvs, size_t tlvs_len)
{
	struct wpa_supplicant *wpa_s = ctx;
	const u8 *pos = tlvs;
	const u8 *end = tlvs + tlvs_len;
	const u8 *tlv_end;
	u16 slen;
	struct wpabuf *resp;
	u8 srv_proto, srv_trans_id;
	size_t buf_len;
	char *buf;

	da16x_dump("P2P: Service Discovery Request TLVs", tlvs, tlvs_len);

	buf_len = 2 * tlvs_len + 1;
	buf = os_malloc(buf_len);
	if (buf) {
		wpa_snprintf_hex(buf, buf_len, tlvs, tlvs_len);
		da16x_p2p_prt("[%s] " P2P_EVENT_SERV_DISC_REQ "%d "
			MACSTR " %u %u %s\n",
			__func__,
			freq, MAC2STR(sa), dialog_token, update_indic,
			buf);

		os_free(buf);
	}

	if (wpa_s->p2p_sd_over_ctrl_iface) {
		return; /* to be processed by an external program */
	}

	resp = wpabuf_alloc(10000);
	if (resp == NULL)
		return;

	while (pos + 1 < end) {
		da16x_p2p_prt("[%s] P2P: Service Request TLV\n", __func__);

		slen = WPA_GET_LE16(pos);
		pos += 2;
		if (pos + slen > end || slen < 2) {
			da16x_p2p_prt("[%s] P2P: Unexpected Query Data length\n",
					__func__);

			wpabuf_free(resp);
			return;
		}
		tlv_end = pos + slen;

		srv_proto = *pos++;
		da16x_p2p_prt("[%s] P2P: Service Protocol Type %u\n",
				__func__, srv_proto);

		srv_trans_id = *pos++;
		da16x_p2p_prt("[%s] P2P: Service Transaction ID %u\n",
			   	__func__, srv_trans_id);

		da16x_dump("P2P: Query Data", pos, tlv_end - pos);


		if (wpa_s->force_long_sd) {
			da16x_p2p_prt("[%s] P2P: SD test - "
				"force long response\n", __func__);

			wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
			wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
			goto done;
		}

		switch (srv_proto) {
		case P2P_SERV_ALL_SERVICES:
			da16x_p2p_prt("[%s] P2P: Service Discovery Request "
				   "for all services\n", __func__);

			if (dl_list_empty(&wpa_s->global->p2p_srv_upnp) &&
			    dl_list_empty(&wpa_s->global->p2p_srv_bonjour)) {
				da16x_p2p_prt("[%s] P2P: No service "
					   "discovery protocols available\n",
						__func__);

				wpas_sd_add_proto_not_avail(
					resp, P2P_SERV_ALL_SERVICES,
					srv_trans_id);
				break;
			}
			wpas_sd_all_bonjour(wpa_s, resp, srv_trans_id);
			wpas_sd_all_upnp(wpa_s, resp, srv_trans_id);
			break;
		case P2P_SERV_BONJOUR:
			wpas_sd_req_bonjour(wpa_s, resp, srv_trans_id,
					    pos, tlv_end - pos);
			break;
		case P2P_SERV_UPNP:
			wpas_sd_req_upnp(wpa_s, resp, srv_trans_id,
					 pos, tlv_end - pos);
			break;
#ifdef CONFIG_WIFI_DISPLAY
		case P2P_SERV_WIFI_DISPLAY:
			wpas_sd_req_wfd(wpa_s, resp, srv_trans_id,
					pos, tlv_end - pos);
			break;
#endif /* CONFIG_WIFI_DISPLAY */
		default:
			da16x_p2p_prt("[%s] P2P: Unavailable service "
				   "protocol %u\n", __func__, srv_proto);

			wpas_sd_add_proto_not_avail(resp, srv_proto,
						    srv_trans_id);
			break;
		}

		pos = tlv_end;
	}

done:
	wpas_p2p_sd_response(wpa_s, freq, sa, dialog_token, resp);

	wpabuf_free(resp);
}


static void wpas_sd_response(void *ctx, const u8 *sa, u16 update_indic,
			     const u8 *tlvs, size_t tlvs_len)
{
	const u8 *pos = tlvs;
	const u8 *end = tlvs + tlvs_len;
	const u8 *tlv_end;
	u16 slen;
	size_t buf_len;
	char *buf;

	da16x_dump("P2P: Service Discovery Response TLVs", tlvs, tlvs_len);

	if (tlvs_len > 1500) {
		/* TODO: better way for handling this */
		da16x_p2p_prt("[%s] " P2P_EVENT_SERV_DISC_RESP MACSTR
			" %u <long response: %u bytes>\n",
			__func__,
			MAC2STR(sa), update_indic,
			(unsigned int) tlvs_len);
	} else {
		buf_len = 2 * tlvs_len + 1;
		buf = os_malloc(buf_len);
		if (buf) {
			wpa_snprintf_hex(buf, buf_len, tlvs, tlvs_len);
			da16x_p2p_prt("[%s] " P2P_EVENT_SERV_DISC_RESP MACSTR
				" %u %s\n",
				__func__,
				MAC2STR(sa), update_indic, buf);
			os_free(buf);
		}
	}

	while (pos < end) {
#ifdef	ENABLE_P2P_DBG
		u8 srv_proto, srv_trans_id, status;
#endif	/* ENABLE_P2P_DBG */

		da16x_p2p_prt("[%s] P2P: Service Response TLV\n", __func__);

		slen = WPA_GET_LE16(pos);
		pos += 2;
		if (pos + slen > end || slen < 3) {
			da16x_p2p_prt("[%s] P2P: Unexpected Response Data "
				   "length\n", __func__);
			return;
		}
		tlv_end = pos + slen;

#ifdef	ENABLE_P2P_DBG
		srv_proto = *pos++;
#endif	/* ENABLE_P2P_DBG */
		da16x_p2p_prt("[%s] P2P: Service Protocol Type %u\n",
			   __func__, srv_proto);

#ifdef	ENABLE_P2P_DBG
		srv_trans_id = *pos++;
#endif	/* ENABLE_P2P_DBG */
		da16x_p2p_prt("[%s] P2P: Service Transaction ID %u\n",
			   __func__, srv_trans_id);

#ifdef	ENABLE_P2P_DBG
		status = *pos++;
#endif	/* ENABLE_P2P_DBG */
		da16x_p2p_prt("[%s] P2P: Status Code ID %u\n",
			   __func__, status);

		da16x_dump("[%s] P2P: Response Data", pos, tlv_end - pos);

		pos = tlv_end;
	}
}


u64 wpas_p2p_sd_request(struct wpa_supplicant *wpa_s, const u8 *dst,
			const struct wpabuf *tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;
	return (u64) p2p_sd_request(wpa_s->global->p2p, dst, tlvs);
}


u64 wpas_p2p_sd_request_upnp(struct wpa_supplicant *wpa_s, const u8 *dst,
			     u8 version, const char *query)
{
	struct wpabuf *tlvs;
	u64 ret;

	tlvs = wpabuf_alloc(2 + 1 + 1 + 1 + os_strlen(query));
	if (tlvs == NULL)
		return 0;
	wpabuf_put_le16(tlvs, 1 + 1 + 1 + os_strlen(query));
	wpabuf_put_u8(tlvs, P2P_SERV_UPNP); /* Service Protocol Type */
	wpabuf_put_u8(tlvs, 1); /* Service Transaction ID */
	wpabuf_put_u8(tlvs, version);
	wpabuf_put_str(tlvs, query);
	ret = wpas_p2p_sd_request(wpa_s, dst, tlvs);
	wpabuf_free(tlvs);
	return ret;
}


#ifdef CONFIG_WIFI_DISPLAY

static u64 wpas_p2p_sd_request_wfd(struct wpa_supplicant *wpa_s, const u8 *dst,
				   const struct wpabuf *tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;
	return (uintptr_t) p2p_sd_request_wfd(wpa_s->global->p2p, dst, tlvs);
}


#define MAX_WFD_SD_SUBELEMS 20

static void wfd_add_sd_req_role(struct wpabuf *tlvs, u8 id, u8 role,
				const char *subelems)
{
	u8 *len;
	const char *pos;
	int val;
	int count = 0;

	len = wpabuf_put(tlvs, 2);
	wpabuf_put_u8(tlvs, P2P_SERV_WIFI_DISPLAY); /* Service Protocol Type */
	wpabuf_put_u8(tlvs, id); /* Service Transaction ID */

	wpabuf_put_u8(tlvs, role);

	pos = subelems;
	while (*pos) {
		val = atoi(pos);
		if (val >= 0 && val < 256) {
			wpabuf_put_u8(tlvs, val);
			count++;
			if (count == MAX_WFD_SD_SUBELEMS)
				break;
		}
		pos = os_strchr(pos + 1, ',');
		if (pos == NULL)
			break;
		pos++;
	}

	WPA_PUT_LE16(len, (u8 *) wpabuf_put(tlvs, 0) - len - 2);
}


u64 wpas_p2p_sd_request_wifi_display(struct wpa_supplicant *wpa_s,
				     const u8 *dst, const char *role)
{
	struct wpabuf *tlvs;
	u64 ret;
	const char *subelems;
	u8 id = 1;

	subelems = os_strchr(role, ' ');
	if (subelems == NULL)
		return 0;
	subelems++;

	tlvs = wpabuf_alloc(4 * (2 + 1 + 1 + 1 + MAX_WFD_SD_SUBELEMS));
	if (tlvs == NULL)
		return 0;

	if (os_strstr(role, "[source]"))
		wfd_add_sd_req_role(tlvs, id++, 0x00, subelems);
	if (os_strstr(role, "[pri-sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x01, subelems);
	if (os_strstr(role, "[sec-sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x02, subelems);
	if (os_strstr(role, "[source+sink]"))
		wfd_add_sd_req_role(tlvs, id++, 0x03, subelems);

	ret = wpas_p2p_sd_request_wfd(wpa_s, dst, tlvs);
	wpabuf_free(tlvs);
	return ret;
}

#endif /* CONFIG_WIFI_DISPLAY */


int wpas_p2p_sd_cancel_request(struct wpa_supplicant *wpa_s, u64 req)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_sd_cancel_request(wpa_s->global->p2p,
				    (void *)(unsigned long int)req);
}


void wpas_p2p_sd_response(struct wpa_supplicant *wpa_s, int freq,
			  const u8 *dst, u8 dialog_token,
			  const struct wpabuf *resp_tlvs)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;
	p2p_sd_response(wpa_s->global->p2p, freq, dst, dialog_token,
			resp_tlvs);
}


void wpas_p2p_sd_service_update(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->global->p2p)
		p2p_sd_service_update(wpa_s->global->p2p);
}


static void wpas_p2p_srv_bonjour_free(struct p2p_srv_bonjour *bsrv)
{
	dl_list_del(&bsrv->list);
	wpabuf_free(bsrv->query);
	wpabuf_free(bsrv->resp);
	os_free(bsrv);
}


static void wpas_p2p_srv_upnp_free(struct p2p_srv_upnp *usrv)
{
	dl_list_del(&usrv->list);
	os_free(usrv->service);
	os_free(usrv);
}


void wpas_p2p_service_flush(struct wpa_supplicant *wpa_s)
{
	struct p2p_srv_bonjour *bsrv, *bn;
	struct p2p_srv_upnp *usrv, *un;

	dl_list_for_each_safe(bsrv, bn, &wpa_s->global->p2p_srv_bonjour,
			      struct p2p_srv_bonjour, list)
		wpas_p2p_srv_bonjour_free(bsrv);

	dl_list_for_each_safe(usrv, un, &wpa_s->global->p2p_srv_upnp,
			      struct p2p_srv_upnp, list)
		wpas_p2p_srv_upnp_free(usrv);

	wpas_p2p_sd_service_update(wpa_s);
}


int wpas_p2p_service_add_bonjour(struct wpa_supplicant *wpa_s,
				 struct wpabuf *query, struct wpabuf *resp)
{
	struct p2p_srv_bonjour *bsrv;

	bsrv = os_zalloc(sizeof(*bsrv));
	if (bsrv == NULL)
		return -1;
	bsrv->query = query;
	bsrv->resp = resp;
	dl_list_add(&wpa_s->global->p2p_srv_bonjour, &bsrv->list);

	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_del_bonjour(struct wpa_supplicant *wpa_s,
				 const struct wpabuf *query)
{
	struct p2p_srv_bonjour *bsrv;

	bsrv = wpas_p2p_service_get_bonjour(wpa_s, query);
	if (bsrv == NULL)
		return -1;
	wpas_p2p_srv_bonjour_free(bsrv);
	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_add_upnp(struct wpa_supplicant *wpa_s, u8 version,
			      const char *service)
{
	struct p2p_srv_upnp *usrv;

	if (wpas_p2p_service_get_upnp(wpa_s, version, service))
		return 0; /* Already listed */
	usrv = os_zalloc(sizeof(*usrv));
	if (usrv == NULL)
		return -1;
	usrv->version = version;
	usrv->service = os_strdup(service);
	if (usrv->service == NULL) {
		os_free(usrv);
		return -1;
	}
	dl_list_add(&wpa_s->global->p2p_srv_upnp, &usrv->list);

	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}


int wpas_p2p_service_del_upnp(struct wpa_supplicant *wpa_s, u8 version,
			      const char *service)
{
	struct p2p_srv_upnp *usrv;

	usrv = wpas_p2p_service_get_upnp(wpa_s, version, service);
	if (usrv == NULL)
		return -1;
	wpas_p2p_srv_upnp_free(usrv);
	wpas_p2p_sd_service_update(wpa_s);
	return 0;
}
#endif	/* CONFIG_P2P_OPTION */


#ifdef	CONFIG_WPS
static void wpas_prov_disc_local_display(struct wpa_supplicant *wpa_s,
					 const u8 *peer, const char *params,
					 unsigned int generated_pin)
{
	da16x_p2p_prt("[%s] " P2P_EVENT_PROV_DISC_SHOW_PIN MACSTR " %08d%s\n",
			__func__, MAC2STR(peer), generated_pin, params);
}

static void wpas_prov_disc_local_keypad(struct wpa_supplicant *wpa_s,
					const u8 *peer, const char *params)
{
	da16x_p2p_prt("[%s] " P2P_EVENT_PROV_DISC_ENTER_PIN MACSTR "%s\n",
			__func__, MAC2STR(peer), params);
}
#endif	/* CONFIG_WPS */


static void wpas_prov_disc_req(void *ctx, const u8 *peer, u16 config_methods,
			       const u8 *dev_addr, const u8 *pri_dev_type,
			       const char *dev_name, u16 supp_config_methods,
			       u8 dev_capab, u8 group_capab, const u8 *group_id,
			       size_t group_id_len)
{
	struct wpa_supplicant *wpa_s = ctx;
#ifdef	CONFIG_WPS
	char devtype[WPS_DEV_TYPE_BUFSIZE];
#endif	/* CONFIG_WPS */
	char params[300];
	u8 empty_dev_type[8];
#ifdef	CONFIG_WPS
	unsigned int generated_pin = 0;
#endif	/* CONFIG_WPS */
	struct wpa_supplicant *group = NULL;

	if (group_id) {
		for (group = wpa_s->global->ifaces; group; group = group->next)
		{
			struct wpa_ssid *s = group->current_ssid;
			if (s != NULL &&
			    s->mode == WPAS_MODE_P2P_GO &&
			    group_id_len - ETH_ALEN == s->ssid_len &&
			    os_memcmp(group_id + ETH_ALEN, s->ssid,
				      s->ssid_len) == 0)
				break;
		}
	}

	if (pri_dev_type == NULL) {
		os_memset(empty_dev_type, 0, sizeof(empty_dev_type));
		pri_dev_type = empty_dev_type;
	}
	os_snprintf(params, sizeof(params), " p2p_dev_addr=" MACSTR
		    " pri_dev_type=%s name='%s' config_methods=0x%x "
		    "dev_capab=0x%x group_capab=0x%x%s%s",
		    MAC2STR(dev_addr),
#ifdef	CONFIG_WPS
		    wps_dev_type_bin2str(pri_dev_type, devtype,
					 sizeof(devtype)),
#else
		    "WPS_NONE",
#endif	/* CONFIG_WPS */
		    dev_name, supp_config_methods, dev_capab, group_capab,
		    group ? " group=" : "",
		    group ? group->ifname : "");
	params[sizeof(params) - 1] = '\0';

#ifdef	CONFIG_WPS
	if (config_methods & WPS_CONFIG_DISPLAY) {
		generated_pin = wps_generate_pin();
		wpas_prov_disc_local_display(wpa_s, peer, params,
					     generated_pin);
	} else if (config_methods & WPS_CONFIG_KEYPAD) {
		wpas_prov_disc_local_keypad(wpa_s, peer, params);
	} else if (config_methods & WPS_CONFIG_PUSHBUTTON) {
		da16x_p2p_prt("[%s] " P2P_EVENT_PROV_DISC_PBC_REQ MACSTR "%s\n",
				__func__, MAC2STR(peer), params);
	}
#else
da16x_p2p_prt("[%s] Have to include CONFIG_WPS features ...\n", __func__);
#endif	/* CONFIG_WPS */
}


static void wpas_prov_disc_resp(void *ctx, const u8 *peer, u16 config_methods)
{
	struct wpa_supplicant *wpa_s = ctx;
#ifdef	CONFIG_WPS
	unsigned int generated_pin = 0;
#endif	/* CONFIG_WPS */
	char params[20];

	if (wpa_s->pending_pd_before_join &&
	    (os_memcmp(peer, wpa_s->pending_join_dev_addr, ETH_ALEN) == 0 ||
	     os_memcmp(peer, wpa_s->pending_join_iface_addr, ETH_ALEN) == 0)) {
		wpa_s->pending_pd_before_join = 0;
		da16x_p2p_prt("[%s] P2P: Starting pending "
			   "join-existing-group operation\n", __func__);

		wpas_p2p_join_start(wpa_s, 0, NULL, 0);
		return;
	}

	if (wpa_s->pending_pd_use == AUTO_PD_JOIN ||
	    wpa_s->pending_pd_use == AUTO_PD_GO_NEG)
		os_snprintf(params, sizeof(params), " peer_go=%d",
			    wpa_s->pending_pd_use == AUTO_PD_JOIN);
	else
		params[0] = '\0';

#ifdef	CONFIG_WPS
	if (config_methods & WPS_CONFIG_DISPLAY)
		wpas_prov_disc_local_keypad(wpa_s, peer, params);
	else if (config_methods & WPS_CONFIG_KEYPAD) {
		generated_pin = wps_generate_pin();
		wpas_prov_disc_local_display(wpa_s, peer, params,
					     generated_pin);
	} else if (config_methods & WPS_CONFIG_PUSHBUTTON) {
		da16x_p2p_prt("[%s] " P2P_EVENT_PROV_DISC_PBC_RESP MACSTR "%s\n",
				__func__, MAC2STR(peer), params);
	}
#else
da16x_p2p_prt("[%s] Have to include CONFIG_WPS features ...\n", __func__);
#endif	/* CONFIG_WPS */
}


static void wpas_prov_disc_fail(void *ctx, const u8 *peer,
				enum p2p_prov_disc_status status)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->p2p_fallback_to_go_neg) {
		da16x_p2p_prt("[%s] P2P: PD for p2p_connect-auto "
			"failed - fall back to GO Negotiation\n", __func__);
		wpas_p2p_fallback_to_go_neg(wpa_s, 0);
		return;
	}

	if (status == P2P_PROV_DISC_TIMEOUT_JOIN) {
		wpa_s->pending_pd_before_join = 0;
		da16x_p2p_prt("[%s] P2P: Starting pending "
			   "join-existing-group operation (no ACK for PD "
			   "Req attempts)\n", __func__);

		wpas_p2p_join_start(wpa_s, 0, NULL, 0);
		return;
	}

	da16x_p2p_prt("[%s] " P2P_EVENT_PROV_DISC_FAILURE " p2p_dev_addr="
			MACSTR " status=%d\n",
			__func__, MAC2STR(peer), status);
}


static int freq_included(const struct p2p_channels *channels, unsigned int freq)
{
	if (channels == NULL)
		return 1; /* Assume no restrictions */
	return p2p_channels_includes_freq(channels, freq);

}

#ifdef CONFIG_5G_SUPPORT
static void wpas_p2p_go_update_common_freqs(struct wpa_supplicant *wpa_s)
{
	unsigned int num = P2P_MAX_CHANNELS;
	int *common_freqs;
	int ret;

	//p2p_go_dump_common_freqs(wpa_s);
	common_freqs = os_calloc(num, sizeof(int));
	if (!common_freqs)
		return;

	ret = p2p_group_get_common_freqs(wpa_s->p2p_group, common_freqs, &num);
	if (ret < 0) {
		da16x_p2p_prt("[%s] "
			"P2P: Failed to get group common freqs\n", __func__);
		os_free(common_freqs);
		return;
	}

	os_free(wpa_s->p2p_group_common_freqs);
	wpa_s->p2p_group_common_freqs = common_freqs;
	wpa_s->p2p_group_common_freqs_num = num;
	//p2p_go_dump_common_freqs(wpa_s);
}
#endif /* CONFIG_5G_SUPPORT */


#ifdef	CONFIG_P2P_OPTION
static u8 wpas_invitation_process(void *ctx, const u8 *sa, const u8 *bssid,
				  const u8 *go_dev_addr, const u8 *ssid,
				  size_t ssid_len, int *go, u8 *group_bssid,
				  int *force_freq, int persistent_group,
				  const struct p2p_channels *channels,
				  int dev_pw_id)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *s;
	int res;
	struct wpa_supplicant *grp;

	if (!persistent_group) {
		da16x_p2p_prt("[%s] P2P: Invitation from " MACSTR
			   " to join an active group (SSID: %s)\n",
			   __func__,
			   MAC2STR(sa), wpa_ssid_txt(ssid, ssid_len));

		if (!is_zero_ether_addr(wpa_s->p2p_auth_invite) &&
		    (os_memcmp(go_dev_addr, wpa_s->p2p_auth_invite, ETH_ALEN)
		     == 0 ||
		     os_memcmp(sa, wpa_s->p2p_auth_invite, ETH_ALEN) == 0)) {
			da16x_p2p_prt("[%s] P2P: Accept previously "
				   "authorized invitation\n", __func__);
			goto accept_inv;
		}

		/*
		 * Do not accept the invitation automatically; notify user and
		 * request approval.
		 */
		return P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE;
	}

	grp = wpas_get_p2p_group(wpa_s, ssid, ssid_len, go);
	if (grp) {
		da16x_p2p_prt("[%s] P2P: Accept invitation to already "
			   "running persistent group\n", __func__);

		if (*go)
			os_memcpy(group_bssid, grp->own_addr, ETH_ALEN);
		goto accept_inv;
	}

	if (!is_zero_ether_addr(wpa_s->p2p_auth_invite) &&
	    os_memcmp(sa, wpa_s->p2p_auth_invite, ETH_ALEN) == 0) {
		da16x_p2p_prt("[%s] P2P: Accept previously initiated "
			   "invitation to re-invoke a persistent group\n",
				__func__);

		os_memset(wpa_s->p2p_auth_invite, 0, ETH_ALEN);
	} else if (!wpa_s->conf->persistent_reconnect)
		return P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    os_memcmp(s->bssid, go_dev_addr, ETH_ALEN) == 0 &&
		    s->ssid_len == ssid_len &&
		    os_memcmp(ssid, s->ssid, ssid_len) == 0)
			break;
	}

	if (!s) {
		da16x_p2p_prt("[%s] P2P: Invitation from " MACSTR
			   " requested reinvocation of an unknown group\n",
			   __func__, MAC2STR(sa));

		return P2P_SC_FAIL_UNKNOWN_GROUP;
	}

	if (s->mode == WPAS_MODE_P2P_GO && !wpas_p2p_create_iface(wpa_s)) {
		*go = 1;
		if (wpa_s->wpa_state >= WPA_AUTHENTICATING) {
			da16x_p2p_prt("[%s] P2P: The only available "
				   "interface is already in use - reject "
				   "invitation\n", __func__);

			return P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE;
		}
		os_memcpy(group_bssid, wpa_s->own_addr, ETH_ALEN);
	} else if (s->mode == WPAS_MODE_P2P_GO) {
		*go = 1;

		if (wpas_p2p_add_group_interface(wpa_s, WPA_IF_P2P_GO) < 0) {
			da16x_p2p_prt("[%s] P2P: Failed to allocate a new "
				   "interface address for the group\n",
					__func__);

			return P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE;
		}
		os_memcpy(group_bssid, wpa_s->pending_interface_addr,
			  ETH_ALEN);
	}

accept_inv:

	/* Get one of the frequencies currently in use */
	if (wpas_p2p_valid_oper_freqs(wpa_s, &res, 1) > 0) {
		da16x_p2p_prt("[%s] P2P: Trying to prefer a channel already "
				"used by one of the interfaces\n", __func__);
		
		if (wpa_s->num_multichan_concurrent < 2 ||
		    wpas_p2p_num_unused_channels(wpa_s) < 1) {
			da16x_p2p_prt("[%s] P2P: No extra channels available - "
				"trying to force channel to match a channel "
				"already used by one of the interfaces\n",
					__func__);
			*force_freq = res;
		}
	}

	if (*force_freq > 0 && wpa_s->num_multichan_concurrent > 1 &&
	    wpas_p2p_num_unused_channels(wpa_s) > 0) {
		if (*go == 0) {
			/* We are the client */
			da16x_p2p_prt("[%s] P2P: Peer was found to be "
				   "running a GO but we are capable of MCC, "
				   "figure out the best channel to use\n",
					__func__);

			*force_freq = 0;
		} else if (!freq_included(channels, *force_freq)) {
			/* We are the GO, and *force_freq is not in the
			 * intersection */
			da16x_p2p_prt("[%s] P2P: Forced GO freq %d MHz not "
				   "in intersection but we are capable of MCC, "
				   "figure out the best channel to use\n",
				   __func__, *force_freq);

			*force_freq = 0;
		}
	}

	return P2P_SC_SUCCESS;
}


static void wpas_invitation_received(void *ctx, const u8 *sa, const u8 *bssid,
				     const u8 *ssid, size_t ssid_len,
				     const u8 *go_dev_addr, u8 status,
				     int op_freq)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *s;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled == 2 &&
		    s->ssid_len == ssid_len &&
		    os_memcmp(ssid, s->ssid, ssid_len) == 0)
			break;
	}

	if (status == P2P_SC_SUCCESS) {
		da16x_p2p_prt("[%s] P2P: Invitation from peer " MACSTR
			   " was accepted; op_freq=%d MHz, SSID=%s\n",
			   __func__,
			   MAC2STR(sa), op_freq, wpa_ssid_txt(ssid, ssid_len));
		if (s) {
			int go = s->mode == WPAS_MODE_P2P_GO;
			wpas_p2p_group_add_persistent(
				wpa_s, s, go, 0, op_freq, 0, 0, NULL,
				go ? P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE : 0);
		} else if (bssid) {
			wpa_s->user_initiated_pd = 0;
			wpas_p2p_join(wpa_s, bssid, go_dev_addr,
				      (enum p2p_wps_method)wpa_s->p2p_wps_method,
				      op_freq, ssid, ssid_len);
		}
		return;
	}

	if (status != P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE) {
		da16x_p2p_prt("[%s] P2P: Invitation from peer " MACSTR
			   " was rejected (status %u)\n",
				__func__, MAC2STR(sa), status);
		return;
	}

	if (!s) {
		if (bssid) {
			da16x_p2p_prt(P2P_EVENT_INVITATION_RECEIVED
				"sa=" MACSTR " go_dev_addr=" MACSTR
				" bssid=" MACSTR " unknown-network\n",
				__func__,
				MAC2STR(sa), MAC2STR(go_dev_addr),
				MAC2STR(bssid));
		} else {
			da16x_p2p_prt("[%s] "P2P_EVENT_INVITATION_RECEIVED
				"sa=" MACSTR " go_dev_addr=" MACSTR
				" unknown-network\n",
				__func__,
				MAC2STR(sa), MAC2STR(go_dev_addr));
		}
		return;
	}

	if (s->mode == WPAS_MODE_P2P_GO && op_freq) {
		da16x_p2p_prt("[%s] " P2P_EVENT_INVITATION_RECEIVED
				"sa=" MACSTR " persistent=%d freq=%d\n",
				__func__, MAC2STR(sa), s->id, op_freq);
	} else {
		da16x_p2p_prt("[%s] " P2P_EVENT_INVITATION_RECEIVED "sa=" MACSTR
				" persistent=%d\n",
				__func__, MAC2STR(sa), s->id);
	}
}
#endif	/* CONFIG_P2P_OPTION */


#ifdef	CONFIG_P2P_OPTION
static void wpas_remove_persistent_peer(struct wpa_supplicant *wpa_s,
					struct wpa_ssid *ssid,
					const u8 *peer, int inv)
{
	size_t i;

	if (ssid == NULL)
		return;

	for (i = 0; ssid->p2p_client_list && i < ssid->num_p2p_clients; i++) {
		if (os_memcmp(ssid->p2p_client_list + i * ETH_ALEN, peer,
			      ETH_ALEN) == 0)
			break;
	}
	if (i >= ssid->num_p2p_clients) {
		if (ssid->mode != WPAS_MODE_P2P_GO &&
		    os_memcmp(ssid->bssid, peer, ETH_ALEN) == 0) {
			da16x_p2p_prt("[%s] P2P: Remove persistent group %d "
				   "due to invitation result\n",
					__func__, ssid->id);

			if (wpa_s->next_ssid == ssid)
				wpa_s->next_ssid = NULL;
			if (wpa_s->wpa)
				wpa_sm_pmksa_cache_flush(wpa_s->wpa, ssid);

#ifdef	CONFIG_P2P
			wpas_p2p_network_removed(wpa_s, ssid);
#endif	/* CONFIG_P2P */

			wpa_config_remove_network(wpa_s->conf, ssid->id);
			return;
		}
		return; /* Peer not found in client list */
	}

	da16x_p2p_prt("[%s] P2P: Remove peer " MACSTR " from persistent "
		   "group %d client list%s\n",
		   __func__,
		   MAC2STR(peer), ssid->id,
		   inv ? " due to invitation result" : "");

	os_memmove(ssid->p2p_client_list + i * ETH_ALEN,
		   ssid->p2p_client_list + (i + 1) * ETH_ALEN,
		   (ssid->num_p2p_clients - i - 1) * ETH_ALEN);
	ssid->num_p2p_clients--;
}
#endif	/* CONFIG_P2P_OPTION */

#ifdef	CONFIG_P2P_OPTION
static void wpas_remove_persistent_client(struct wpa_supplicant *wpa_s,
					  const u8 *peer)
{
	struct wpa_ssid *ssid;

	wpa_s = wpa_s->global->p2p_invite_group;
	if (wpa_s == NULL)
		return; /* No known invitation group */
	ssid = wpa_s->current_ssid;
	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GO ||
	    !ssid->p2p_persistent_group)
		return; /* Not operating as a GO in persistent group */
	ssid = wpas_p2p_get_persistent(wpa_s->parent, peer,
				       ssid->ssid, ssid->ssid_len);
	wpas_remove_persistent_peer(wpa_s, ssid, peer, 1);
}


static void wpas_invitation_result(void *ctx, int status, const u8 *bssid,
				   const struct p2p_channels *channels,
				   const u8 *peer, int neg_freq,
				   int peer_oper_freq)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *ssid;
	int freq;

	if (bssid) {
		da16x_p2p_prt("[%s] "P2P_EVENT_INVITATION_RESULT
			       "status=%d " MACSTR "\n",
			       __func__, status, MAC2STR(bssid));
	} else {
		da16x_p2p_prt("[%s] " P2P_EVENT_INVITATION_RESULT
			       "status=%d\n", __func__, status);
	}

#if 0	/* by Bright */
	wpas_notify_p2p_invitation_result(wpa_s, status, bssid);
#endif	/* 0 */

	da16x_p2p_prt("[%s] P2P: Invitation result - status=%d peer="
		   MACSTR "\n",
		   __func__, status, MAC2STR(peer));

	if (wpa_s->pending_invite_ssid_id == -1) {
		if (status == P2P_SC_FAIL_UNKNOWN_GROUP)
			wpas_remove_persistent_client(wpa_s, peer);
		return; /* Invitation to active group */
	}

	if (status == P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE) {
		da16x_p2p_prt("[%s] P2P: Waiting for peer to start another "
			   "invitation exchange to indicate readiness for "
			   "re-invocation\n", __func__);
	}

	if (status != P2P_SC_SUCCESS) {
		if (status == P2P_SC_FAIL_UNKNOWN_GROUP) {
			ssid = wpa_config_get_network(
				wpa_s->conf, wpa_s->pending_invite_ssid_id);
			wpas_remove_persistent_peer(wpa_s, ssid, peer, 1);
		}
		wpas_p2p_remove_pending_group_interface(wpa_s);
		return;
	}

	ssid = wpa_config_get_network(wpa_s->conf,
				      wpa_s->pending_invite_ssid_id);
	if (ssid == NULL) {
		da16x_p2p_prt("[%s] P2P: Could not find persistent group "
			   "data matching with invitation\n",
			   __func__);

		return;
	}

	/*
	 * The peer could have missed our ctrl::ack frame for Invitation
	 * Response and continue retransmitting the frame. To reduce the
	 * likelihood of the peer not getting successful TX status for the
	 * Invitation Response frame, wait a short time here before starting
	 * the persistent group so that we will remain on the current channel to
	 * acknowledge any possible retransmission from the peer.
	 */
	da16x_p2p_prt("[%s] P2P: 50 ms wait on current channel before "
		"starting persistent group\n", __func__);
	os_sleep(0, 50000);

	if (neg_freq > 0 && ssid->mode == WPAS_MODE_P2P_GO &&
	    freq_included(channels, neg_freq))
		freq = neg_freq;
	else if (peer_oper_freq > 0 && ssid->mode != WPAS_MODE_P2P_GO &&
		 freq_included(channels, peer_oper_freq))
		freq = peer_oper_freq;
	else
		freq = 0;

	da16x_p2p_prt("[%s] P2P: Persistent group invitation success - "
			"op_freq=%d MHz SSID=%s\n",
			__func__,
		   	freq, wpa_ssid_txt(ssid->ssid, ssid->ssid_len));

	wpas_p2p_group_add_persistent(wpa_s, ssid,
				      ssid->mode == WPAS_MODE_P2P_GO,
				      wpa_s->p2p_persistent_go_freq,
				      freq, 0, 0, channels,
				      ssid->mode == WPAS_MODE_P2P_GO ?
				      P2P_MAX_INITIAL_CONN_WAIT_GO_REINVOKE :
				      0);
}
#endif	/* CONFIG_P2P_OPTION */

#ifdef CONFIG_5G_SUPPORT
static int wpas_p2p_disallowed_freq(struct wpa_global *global,
				    unsigned int freq)
{
	if (freq_range_list_includes(&global->p2p_go_avoid_freq, freq))
		return 1;
	return freq_range_list_includes(&global->p2p_disallow_freq, freq);
}
#endif /* CONFIG_5G_SUPPORT */

static void wpas_p2p_add_chan(struct p2p_reg_class *reg, u8 chan)
{
	reg->channel[reg->channels] = chan;
	reg->channels++;
}


#if 0	/* by Shingu */
static int wpas_p2p_default_channels(struct wpa_supplicant *wpa_s,
				     struct p2p_channels *chan,
				     struct p2p_channels *cli_chan)
{
	int i, cla = 0;

	os_memset(cli_chan, 0, sizeof(*cli_chan));

	da16x_p2p_prt("P2P: Enable operating classes for 2.4 GHz band\n",
			__func__);

	/* Operating class 81 - 2.4 GHz band channels 1..13 */
	chan->reg_class[cla].reg_class = 81;
	chan->reg_class[cla].channels = 0;
	for (i = 0; i < 13; i++) {
#ifdef	CONFIG_P2P_UNUSED_CMD
		if (!wpas_p2p_disallowed_freq(wpa_s->global, 2412 + i * 5))
#endif	/* CONFIG_P2P_UNUSED_CMD */
			wpas_p2p_add_chan(&chan->reg_class[cla], i + 1);
	}
	if (chan->reg_class[cla].channels)
		cla++;

	chan->reg_classes = cla;
	return 0;
}
#else
static int wpas_p2p_default_channels(struct wpa_supplicant *wpa_s,
				     struct p2p_channels *chan,
				     struct p2p_channels *cli_chan)
{
	int cla = 0;
	
	os_memset(cli_chan, 0, sizeof(*cli_chan));
	
	for (int i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
		int min_chan, max_chan;
		const struct country_ch_power_level *field = (void*)cc_power_table(i);
		extern int country_channel_range (char* country, int *startCH, int *endCH);
		
		country_channel_range(field->country, &min_chan, &max_chan);
		if (os_strcasecmp(wpa_s->conf->country, field->country) != 0)
			continue;

		chan->reg_class[cla].reg_class = 81;
		chan->reg_class[cla].channels = 0;
		
		for (int j = min_chan - 1; j < max_chan; j++)
		{
#ifdef	CONFIG_P2P_UNUSED_CMD
			if (!wpas_p2p_disallowed_freq(wpa_s->global,
						      2412 + i * 5))
#endif	/* CONFIG_P2P_UNUSED_CMD */
				wpas_p2p_add_chan(&chan->reg_class[cla], j + 1);

			if (j >= 12)
				break;
		}
		if (chan->reg_class[cla].channels)
			cla++;
		
		break;
	}

	chan->reg_classes = cla;
	return 0;
}
#endif

static int wpas_p2p_setup_channels(struct wpa_supplicant *wpa_s,
				   struct p2p_channels *chan,
				   struct p2p_channels *cli_chan)
{
#if 1	/* by Shingu */	
	return wpas_p2p_default_channels(wpa_s, chan, cli_chan);
#else
	struct hostapd_hw_modes *mode;
	int cla, op, cli_cla;

	if (wpa_s->hw.modes == NULL) {
		da16x_p2p_prt("[%s] P2P: Driver did not support fetching "
			   "of all supported channels; assume dualband "
			   "support\n", __func__);
		return wpas_p2p_default_channels(wpa_s, chan, cli_chan);
	}

	cla = cli_cla = 0;

	for (op = 0; op_class[op].op_class; op++) {
		struct p2p_oper_class_map *o = &op_class[op];
		u8 ch;
		struct p2p_reg_class *reg = NULL, *cli_reg = NULL;

		mode = get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes, o->mode);
		if (mode == NULL)
			continue;
		for (ch = o->min_chan; ch <= o->max_chan; ch += o->inc) {
			enum chan_allowed res;
			res = wpas_p2p_verify_channel(wpa_s, mode, ch, o->bw);
			if (res == ALLOWED) {
				if (reg == NULL) {
					da16x_p2p_prt("[%s] P2P: Add operating "
						"class %u\n",
						__func__, o->op_class);

					reg = &chan->reg_class[cla];
					cla++;
					reg->reg_class = o->op_class;
				}
				reg->channel[reg->channels] = ch;
				reg->channels++;
			} else if (res == PASSIVE_ONLY &&
				   wpa_s->conf->p2p_add_cli_chan) {
				if (cli_reg == NULL) {
					da16x_p2p_prt("[%s] P2P: Add operating "
						"class %u (client only)\n",
						__func__, o->op_class);

					cli_reg = &cli_chan->reg_class[cli_cla];
					cli_cla++;
					cli_reg->reg_class = o->op_class;
				}
				cli_reg->channel[cli_reg->channels] = ch;
				cli_reg->channels++;
			}
		}
		if (reg) {
			da16x_dump("P2P: Channels", reg->channel, reg->channels);
		}
		if (cli_reg) {
			da16x_dump("P2P: Channels (client only)",
				    cli_reg->channel, cli_reg->channels);
		}
	}

	chan->reg_classes = cla;
	cli_chan->reg_classes = cli_cla;

	return 0;
#endif	/* 1 */
}

#ifdef	CONFIG_P2P_POWER_SAVE
#if 1	/* by Shingu 20180208 (P2P PS opt.) */
static void p2p_ps_resume(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct hostapd_data *hapd = NULL;
	
	if (!wpa_s->ap_iface)
		return;

	hapd = wpa_s->ap_iface->bss[0];
	if (hapd->num_sta_no_p2p == 0)
		hostapd_p2p_default_noa(hapd, 1);
}

static void wpas_p2p_ps_ctrl(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	
	if (!wpa_s->ap_iface)
		return;

	hostapd_p2p_default_noa(wpa_s->ap_iface->bss[0], 0);
	da16x_eloop_cancel_timeout(p2p_ps_resume, wpa_s, NULL);
	da16x_eloop_register_timeout(5, 0, p2p_ps_resume, wpa_s, NULL);
}
#endif	/* 1 */

static int wpas_get_noa(void *ctx, const u8 *interface_addr, u8 *buf,
			size_t buf_len)
{
	struct wpa_supplicant *wpa_s = ctx;

	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_memcmp(wpa_s->own_addr, interface_addr, ETH_ALEN) == 0)
			break;
	}
	if (wpa_s == NULL)
		return -1;

	return wpa_drv_get_noa(wpa_s, buf, buf_len);
}
#endif	/* CONFIG_P2P_POWER_SAVE */


static int wpas_go_connected(void *ctx, const u8 *dev_addr)
{
	struct wpa_supplicant *wpa_s = ctx;

	for (wpa_s = wpa_s->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		struct wpa_ssid *ssid = wpa_s->current_ssid;
		if (ssid == NULL)
			continue;
		if (ssid->mode != WPAS_MODE_INFRA)
			continue;
		if (wpa_s->wpa_state != WPA_COMPLETED &&
		    wpa_s->wpa_state != WPA_GROUP_HANDSHAKE)
			continue;
		if (os_memcmp(wpa_s->go_dev_addr, dev_addr, ETH_ALEN) == 0)
			return 1;
	}

	return 0;
}


#ifdef	CONFIG_P2P_UNUSED_CMD
static int wpas_is_concurrent_session_active(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_supplicant *ifs;

	for (ifs = wpa_s->global->ifaces; ifs; ifs = ifs->next) {
		if (ifs == wpa_s)
			continue;
		if (ifs->wpa_state > WPA_ASSOCIATED)
			return 1;
	}
	return 0;
}

static void wpas_p2p_debug_print(void *ctx, int level, const char *msg)
{
	da16x_p2p_prt("[%s] P2P: %s\n", __func__, msg);
}
#endif	/* CONFIG_P2P_UNUSED_CMD */

struct wpa_supplicant *
wpas_p2p_add_p2pdev_interface(struct wpa_supplicant *wpa_s)
{
	struct wpa_interface iface;
	struct wpa_supplicant *p2pdev_wpa_s;
	char ifname[32];
	char force_name[32];
	int ret;

	os_snprintf(ifname, sizeof(ifname), "%s", P2P_DEVICE_NAME);

	force_name[0] = '\0';
	if (get_run_mode() == P2P_GO_FIXED_MODE) {
		wpa_s->pending_interface_type = WPA_IF_P2P_GO;
		ret = wpa_drv_if_add(wpa_s, WPA_IF_P2P_GO, ifname, NULL, NULL,
				     force_name, wpa_s->pending_interface_addr,
				     NULL);
	} else {
		wpa_s->pending_interface_type = WPA_IF_P2P_DEVICE;
		ret = wpa_drv_if_add(wpa_s, WPA_IF_P2P_DEVICE, ifname, NULL,
				     NULL, force_name,
				     wpa_s->pending_interface_addr, NULL);
	}

	if (ret < 0) {
		da16x_p2p_prt("[%s] P2P: Failed to create P2P Device "
			     "interface\n", __func__);
		return NULL;
	}
	os_strlcpy(wpa_s->pending_interface_name, ifname,
		   sizeof(wpa_s->pending_interface_name));

	os_memset(&iface, 0, sizeof(iface));
	iface.p2p_mgmt = 1;
	iface.ifname = wpa_s->pending_interface_name;
	iface.driver = wpa_s->driver->name;
	iface.driver_param = wpa_s->conf->driver_param;

	p2pdev_wpa_s = wpa_supplicant_add_iface(wpa_s->global, &iface, NULL);
	if (!p2pdev_wpa_s) {
		da16x_p2p_prt("[%s] P2P: Failed to add P2P Device interface\n",
				__func__);
		return NULL;
	}
	p2pdev_wpa_s->parent = wpa_s;

	wpa_s->pending_interface_name[0] = '\0';
	return p2pdev_wpa_s;
}

#ifdef	CONFIG_P2P_POWER_SAVE
static void wpas_presence_resp(void *ctx, const u8 *src, u8 status,
			       const u8 *noa, size_t noa_len)
{
	struct wpa_supplicant *wpa_s, *intf = ctx;
	char hex[100];

	for (wpa_s = intf->global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s->waiting_presence_resp)
			break;
	}
	if (!wpa_s) {
		da16x_p2p_prt("[%s] P2P: No group interface was waiting for "
				"presence response\n", __func__);
		return;
	}
	wpa_s->waiting_presence_resp = 0;

	wpa_snprintf_hex(hex, sizeof(hex), noa, noa_len);

	da16x_p2p_prt("[%s] "P2P_EVENT_PRESENCE_RESPONSE "src=" MACSTR
			" status=%u noa=%s\n",
			__func__, MAC2STR(src), status, hex);
}
#endif	/* CONFIG_P2P_POWER_SAVE */

#ifdef	CONFIG_P2P_UNUSED_CMD
static int _wpas_p2p_in_progress(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;
	return wpas_p2p_in_progress(wpa_s);
}
#endif	/* CONFIG_P2P_UNUSED_CMD */


/**
 * wpas_p2p_init - Initialize P2P module for %wpa_supplicant
 * @global: Pointer to global data from wpa_supplicant_init()
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supp_add_iface()
 * Returns: 0 on success, -1 on failure
 */
int wpas_p2p_init(struct wpa_global *global, struct wpa_supplicant *wpa_s)
{
	struct p2p_config p2p;
#ifdef	CONFIG_WPS
	int i;
#endif	/* CONFIG_WPS */

	if (wpa_s->conf->p2p_disabled)
		return 0;

	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CAPABLE))
		return 0;

	if (global->p2p)
		return 0;

	os_memset(&p2p, 0, sizeof(p2p));
	p2p.cb_ctx = wpa_s;
#ifdef	CONFIG_P2P_UNUSED_CMD
	p2p.debug_print = wpas_p2p_debug_print;
#endif	/* CONFIG_P2P_UNUSED_CMD */
	p2p.p2p_scan = wpas_p2p_scan;
	p2p.send_action = wpas_send_action;
	p2p.send_action_done = wpas_send_action_done;
	p2p.go_neg_completed = wpas_go_neg_completed;
	p2p.go_neg_req_rx = wpas_go_neg_req_rx;
	p2p.dev_found = wpas_dev_found;
	p2p.dev_lost = wpas_dev_lost;
	p2p.find_stopped = wpas_find_stopped;
	p2p.start_listen = wpas_start_listen;
	p2p.stop_listen = wpas_stop_listen;
	p2p.send_probe_resp = wpas_send_probe_resp;
	p2p.prov_disc_req = wpas_prov_disc_req;
	p2p.prov_disc_resp = wpas_prov_disc_resp;
	p2p.prov_disc_fail = wpas_prov_disc_fail;
#ifdef	CONFIG_P2P_OPTION
	p2p.sd_request = wpas_sd_request;
	p2p.sd_response = wpas_sd_response;
	p2p.invitation_process = wpas_invitation_process;
	p2p.invitation_received = wpas_invitation_received;
	p2p.invitation_result = wpas_invitation_result;
#endif	/* CONFIG_P2P_OPTION */
#ifdef	CONFIG_P2P_POWER_SAVE
#if 1	/* by Shingu 20180208 (P2P PS opt.) */
	p2p.set_ps_ctrl = wpas_p2p_ps_ctrl;
#endif	/* 1 */
	p2p.get_noa = wpas_get_noa;
	p2p.presence_resp = wpas_presence_resp;
#endif	/* CONFIG_P2P_POWER_SAVE */
	p2p.go_connected = wpas_go_connected;
#ifdef	CONFIG_P2P_UNUSED_CMD
	p2p.is_concurrent_session_active = wpas_is_concurrent_session_active;
	p2p.is_p2p_in_progress = _wpas_p2p_in_progress;
#endif	/* CONFIG_P2P_UNUSED_CMD */

	os_memcpy(wpa_s->global->p2p_dev_addr, wpa_s->own_addr, ETH_ALEN);
	os_memcpy(p2p.dev_addr, wpa_s->global->p2p_dev_addr, ETH_ALEN);

	p2p.dev_name = wpa_s->conf->device_name;
	p2p.manufacturer = wpa_s->conf->manufacturer;
	p2p.model_name = wpa_s->conf->model_name;
	p2p.model_number = wpa_s->conf->model_number;
	p2p.serial_number = wpa_s->conf->serial_number;
#ifdef	CONFIG_WPS
	if (wpa_s->wps) {
		os_memcpy(p2p.uuid, wpa_s->wps->uuid, 16);
		p2p.config_methods = wpa_s->wps->config_methods;
	}
#endif	/* CONFIG_WPS */

	if (wpas_p2p_setup_channels(wpa_s, &p2p.channels, &p2p.cli_channels)) {
		da16x_p2p_prt("[%s] P2P: Failed to configure supported "
			     "channel list\n", __func__);
		return -1;
	}

	if (get_run_mode() != P2P_GO_FIXED_MODE) {
		if (wpa_s->conf->p2p_listen_reg_class &&
		    wpa_s->conf->p2p_listen_channel) {
			p2p.reg_class = wpa_s->conf->p2p_listen_reg_class;
			p2p.channel = wpa_s->conf->p2p_listen_channel;
#if 0	/* by Shingu 20170418 (P2P ACS) */
		} else {
			/*
			 * Pick one of the social channels randomly as
			 * the listen channel.
			 */
			if (p2p_config_get_random_social(&p2p, &p2p.reg_class,
							 &p2p.channel) != 0) {
				da16x_p2p_prt("[%s] P2P: Failed to select random "
					     "social channel as listen channel",
					     __func__);
				return -1;
			}
#endif	/* 0 */
		}
		da16x_notice_prt(">>> Own listen channel: %d\n", p2p.channel);
	}

	if (wpa_s->conf->p2p_oper_reg_class &&
	    wpa_s->conf->p2p_oper_channel) {
		p2p.op_reg_class = wpa_s->conf->p2p_oper_reg_class;
		p2p.op_channel = wpa_s->conf->p2p_oper_channel;
		p2p.cfg_op_channel = 1;
	} else {
		/*
                 * Use random operation channel from 2.4 GHz band social
                 * channels (1, 6, 11) or band 60 GHz social channel (2) if no
                 * other preference is indicated.
                 */
#if 0	/* by Shingu 20170418 (P2P ACS) */
		if (p2p_config_get_random_social(&p2p, &p2p.op_reg_class,
						 &p2p.op_channel) != 0) {
			da16x_p2p_prt("[%s] P2P: Failed to select random "
				     "social channel as operation channel",
				     __func__);
                        return -1;
		}
#else
		if (get_run_mode() == P2P_GO_FIXED_MODE &&
		    p2p_config_get_random_social(&p2p, &p2p.op_reg_class,
						 &p2p.op_channel) != 0) {
			da16x_p2p_prt("[%s] P2P: Failed to select random "
				     "social channel as operation channel",
				     __func__);
                        return -1;
		}
#endif	/* 0 */
	}
	da16x_notice_prt(">>> Own operating channel: %d\n", p2p.op_channel);

#ifdef	CONFIG_P2P_UNUSED_CMD
	if (wpa_s->conf->p2p_pref_chan && wpa_s->conf->num_p2p_pref_chan) {
		p2p.pref_chan = wpa_s->conf->p2p_pref_chan;
		p2p.num_pref_chan = wpa_s->conf->num_p2p_pref_chan;
	}
#endif	/* CONFIG_P2P_UNUSED_CMD */

	if (wpa_s->conf->country[0] && wpa_s->conf->country[1]) {
		os_memcpy(p2p.country, wpa_s->conf->country, 2);
		p2p.country[2] = 0x04;
	} else
		os_memcpy(p2p.country, "KR\x04", 3);

	os_memcpy(p2p.pri_dev_type, wpa_s->conf->device_type,
		  WPS_DEV_TYPE_LEN);

	p2p.num_sec_dev_types = wpa_s->conf->num_sec_device_types;
	os_memcpy(p2p.sec_dev_type, wpa_s->conf->sec_device_type,
		  p2p.num_sec_dev_types * WPS_DEV_TYPE_LEN);

	p2p.concurrent_operations = !!(wpa_s->drv_flags &
				       WPA_DRIVER_FLAGS_P2P_CONCURRENT);

	p2p.max_peers = 10;

	if (os_strlen(wpa_s->conf->p2p_ssid_postfix) != 0) {
		p2p.ssid_postfix_len =
			os_strlen(wpa_s->conf->p2p_ssid_postfix);
		if (p2p.ssid_postfix_len > sizeof(p2p.ssid_postfix))
			p2p.ssid_postfix_len = sizeof(p2p.ssid_postfix);
		os_memcpy(p2p.ssid_postfix, wpa_s->conf->p2p_ssid_postfix,
			  p2p.ssid_postfix_len);
	}

	p2p.p2p_intra_bss = wpa_s->conf->p2p_intra_bss;

#ifdef	CONFIG_P2P_UNUSED_CMD
	p2p.max_listen = wpa_s->max_remain_on_chan;
#endif	/* CONFIG_P2P_UNUSED_CMD */

	global->p2p = wpa_supplicant_p2p_init(&p2p);
	if (global->p2p == NULL)
		return -1;
	global->p2p_init_wpa_s = wpa_s;

#if 1   /* by Shingu 20160726 (P2P Config) */
	if (wpa_s->conf->ssid) {
		wpa_s->conf->p2p_ssid = os_malloc(32);
		if(wpa_s->conf->p2p_ssid == NULL) return -1;
		wpa_s->conf->p2p_passphrase = os_malloc(64);
		if(wpa_s->conf->p2p_passphrase == NULL) {
                    os_free(wpa_s->conf->p2p_ssid);
                    return -1;
                }
		os_memcpy(wpa_s->conf->p2p_ssid, wpa_s->conf->ssid->ssid,
			  wpa_s->conf->ssid->ssid_len);
		wpa_s->conf->p2p_ssid_len = wpa_s->conf->ssid->ssid_len;
		strcpy(wpa_s->conf->p2p_passphrase,
		       wpa_s->conf->ssid->passphrase);
	}
#endif  /* 1 */

#ifdef	CONFIG_WPS
	for (i = 0; i < MAX_WPS_VENDOR_EXT; i++) {
		if (wpa_s->conf->wps_vendor_ext[i] == NULL)
			continue;
		p2p_add_wps_vendor_extension(
			global->p2p, wpa_s->conf->wps_vendor_ext[i]);
	}
#endif	/* CONFIG_WPS */

#ifdef	CONFIG_P2P_UNUSED_CMD
	p2p_set_no_go_freq(global->p2p, &wpa_s->conf->p2p_no_go_freq);
#endif	/* CONFIG_P2P_UNUSED_CMD */

	return 0;
}


/**
 * wpas_p2p_deinit - Deinitialize per-interface P2P data
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supp_add_iface()
 *
 * This function deinitialize per-interface P2P data.
 */
void wpas_p2p_deinit(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->driver && wpa_s->drv_priv)
		wpa_drv_probe_req_report(wpa_s, 0);

	if (wpa_s->go_params) {
		/* Clear any stored provisioning info */
		p2p_clear_provisioning_info(
			wpa_s->global->p2p,
			wpa_s->go_params->peer_device_addr);
	}

	os_free(wpa_s->go_params);
	wpa_s->go_params = NULL;

	da16x_eloop_cancel_timeout(wpas_p2p_psk_failure_removal, wpa_s, NULL);
	da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
	da16x_eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s, NULL);

	wpas_p2p_remove_pending_group_interface(wpa_s);

	wpabuf_free(wpa_s->p2p_oob_dev_pw);
	wpa_s->p2p_oob_dev_pw = NULL;

	/* TODO: remove group interface from the driver if this wpa_s instance
	 * is on top of a P2P group interface */
}


/**
 * wpas_p2p_deinit_global - Deinitialize global P2P module
 * @global: Pointer to global data from wpa_supplicant_init()
 *
 * This function deinitializes the global (per device) P2P module.
 */
void wpas_p2p_deinit_global(struct wpa_global *global)
{
	struct wpa_supplicant *wpa_s, *tmp;

	wpa_s = global->ifaces;
#ifdef	CONFIG_P2P_OPTION
	if (wpa_s)
		wpas_p2p_service_flush(wpa_s);
#endif	/* CONFIG_P2P_OPTION */

	if (global->p2p == NULL)
		return;

	/* Remove remaining P2P group interfaces */
	while (wpa_s && wpa_s->p2p_group_interface != NOT_P2P_GROUP_INTERFACE)
		wpa_s = wpa_s->next;
	while (wpa_s) {
		tmp = global->ifaces;
		while (tmp &&
		       (tmp == wpa_s ||
			tmp->p2p_group_interface == NOT_P2P_GROUP_INTERFACE)) {
			tmp = tmp->next;
		}
		if (tmp == NULL)
			break;
		/* Disconnect from the P2P group and deinit the interface */
		wpas_p2p_disconnect(tmp);
	}

#ifdef	CONFIG_AP
	/*
	 * Deinit GO data on any possibly remaining interface (if main
	 * interface is used as GO).
	 */
	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s->ap_iface)
			wpas_p2p_group_deinit(wpa_s);
	}
#endif	/* CONFIG_AP */

	p2p_deinit(global->p2p);
	global->p2p = NULL;
	global->p2p_init_wpa_s = NULL;
}


static int wpas_p2p_create_iface(struct wpa_supplicant *wpa_s)
{
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_DEDICATED_P2P_DEVICE) &&
	    wpa_s->conf->p2p_no_group_iface)
		return 0; /* separate interface disabled per configuration */
	if (wpa_s->drv_flags &
	    (WPA_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE |
	     WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P))
		return 1; /* P2P group requires a new interface in every case
			   */
	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CONCURRENT))
		return 0; /* driver does not support concurrent operations */
	if (wpa_s->global->ifaces->next)
		return 1; /* more that one interface already in use */
	if (wpa_s->wpa_state >= WPA_AUTHENTICATING)
		return 1; /* this interface is already in use */
	return 0;
}


static int wpas_p2p_start_go_neg(struct wpa_supplicant *wpa_s,
				 const u8 *peer_addr,
				 enum p2p_wps_method wps_method,
				 int go_intent, const u8 *own_interface_addr)
{
#ifdef	CONFIG_P2P_OPTION
	if (persistent_group && wpa_s->conf->persistent_reconnect)
		persistent_group = 2;
#endif	/* CONFIG_P2P_OPTION */

	/*
	 * Increase GO config timeout if HT40 is used since it takes some time
	 * to scan channels for coex purposes before the BSS can be started.
	 */
	p2p_set_config_timeout(wpa_s->global->p2p, 100, 20);

	return p2p_connect(wpa_s->global->p2p, peer_addr, wps_method,
			   go_intent, own_interface_addr,
			   wpa_s->p2p_pd_before_go_neg);
}

static void wpas_p2p_check_join_scan_limit(struct wpa_supplicant *wpa_s)
{
	wpa_s->p2p_join_scan_count++;

	da16x_p2p_prt("[%s] P2P: Join scan attempt %d\n",
		   __func__, wpa_s->p2p_join_scan_count);

	if (wpa_s->p2p_join_scan_count > P2P_MAX_JOIN_SCAN_ATTEMPTS) {
		da16x_p2p_prt("[%s] P2P: Failed to find GO " MACSTR
			   " for join operationg - stop join attempt\n",
			   __func__, MAC2STR(wpa_s->pending_join_iface_addr));

		da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);

		da16x_p2p_prt("[%s] " P2P_EVENT_GROUP_FORMATION_FAILURE "\n",
			__func__);
	}
}


static int wpas_check_freq_conflict(struct wpa_supplicant *wpa_s, int freq)
{
	int *freqs, res, num, i;

	if (wpas_p2p_num_unused_channels(wpa_s) > 0) {
		/* Multiple channels are supported and not all are in use */
		return 0;
	}

	freqs = os_calloc(wpa_s->num_multichan_concurrent, sizeof(int));
	if (!freqs)
		return 1;

	num = wpas_p2p_valid_oper_freqs(wpa_s, freqs,
					wpa_s->num_multichan_concurrent);
	if (num < 0) {
		res = 1;
		goto exit_free;
	}

	for (i = 0; i < num; i++) {
		if (freqs[i] == freq) {
			da16x_p2p_prt("[%s] P2P: Frequency %d MHz in use by "
				"another virtual interface and can be used\n",
				__func__, freq);

			res = 0;
			goto exit_free;
		}
	}

	res = 1;

exit_free:
	os_free(freqs);
	return res;
}


#if 1	/* by Shingu 20170418 (P2P ACS) */
void wpas_p2p_get_best_social(struct wpa_supplicant *wpa_s,
			      struct wpa_scan_results *scan_res)
{
	int channelA = 0, channelB = 0, channelC = 0;
	int min_channel;

	for (int i = 0; i < scan_res->num; i++) {
		if (scan_res->res[i]->freq == 2412)
			channelA++;
		else if (scan_res->res[i]->freq == 2437)
			channelB++;
		else if (scan_res->res[i]->freq == 2462)
			channelC++;
	}
	
	if (channelA <= channelB) {
		if (channelA <= channelC) {
			min_channel = 1;
		} else {
			min_channel = 11;
		}
	} else {
		if (channelB <= channelC) {
			min_channel = 6;
		} else {
			min_channel = 11;
		}
	}

	if (!wpa_s->conf->p2p_oper_channel) {
		wpa_s->conf->p2p_oper_channel = min_channel;
		p2p_set_oper_channel(wpa_s->global->p2p, 81, min_channel, 1);
		da16x_notice_prt("P2P operating channel to %d\n", min_channel);
	}
	if (!wpa_s->conf->p2p_listen_channel) {
		wpa_s->conf->p2p_listen_channel = min_channel;
		p2p_set_listen_channel(wpa_s->global->p2p, 81, min_channel);
		da16x_notice_prt("P2P listen channel to %d\n", min_channel);
	}
//PRINTF("[Count] CH.1: %d / CH.6: %d / CH.11: %d\n", channelA, channelB, channelC);
}
#endif	/* 1 */


static void wpas_p2p_scan_res_join(struct wpa_supplicant *wpa_s,
				   struct wpa_scan_results *scan_res)
{
	struct wpa_bss *bss = NULL;
	int freq;
	u8 iface_addr[ETH_ALEN];

	da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);

	if (wpa_s->global->p2p_disabled)
		return;

	da16x_p2p_prt("[%s] P2P: Scan results received (%d BSS) for join\n",
		     __func__, scan_res ? (int) scan_res->num : -1);

	if (scan_res)
		wpas_p2p_scan_res_handler(wpa_s, scan_res);

	freq = p2p_get_oper_freq(wpa_s->global->p2p,
				 wpa_s->pending_join_iface_addr);
	if (freq < 0 &&
	    p2p_get_interface_addr(wpa_s->global->p2p,
				   wpa_s->pending_join_dev_addr,
				   iface_addr) == 0 &&
	    os_memcmp(iface_addr, wpa_s->pending_join_dev_addr, ETH_ALEN) != 0)
	{
		da16x_p2p_prt("[%s] P2P: Overwrite pending interface "
			   "address for join from " MACSTR " to " MACSTR
			   " based on newly discovered P2P peer entry\n",
			   __func__,
			   MAC2STR(wpa_s->pending_join_iface_addr),
			   MAC2STR(iface_addr));
		os_memcpy(wpa_s->pending_join_iface_addr, iface_addr,
			  ETH_ALEN);

		freq = p2p_get_oper_freq(wpa_s->global->p2p,
					 wpa_s->pending_join_iface_addr);
	}
	if (freq >= 0) {
		da16x_p2p_prt("[%s] P2P: Target GO operating frequency "
			   "from P2P peer table: %d MHz\n", __func__, freq);
	}
	if (wpa_s->p2p_join_ssid_len) {
		da16x_p2p_prt("[%s] P2P: Trying to find target GO BSS entry "
				"based on BSSID " MACSTR " and SSID %s\n",
			   __func__,
			   MAC2STR(wpa_s->pending_join_iface_addr),
			   wpa_ssid_txt(wpa_s->p2p_join_ssid,
					wpa_s->p2p_join_ssid_len));

		bss = wpa_bss_get(wpa_s, wpa_s->pending_join_iface_addr,
				  wpa_s->p2p_join_ssid,
				  wpa_s->p2p_join_ssid_len);
	}
	if (!bss) {
		da16x_p2p_prt("[%s] P2P: Trying to find target GO BSS "
				"entry based on BSSID " MACSTR "\n",
				__func__,
				MAC2STR(wpa_s->pending_join_iface_addr));

		bss = wpa_bss_get_bssid_latest(wpa_s,
					       wpa_s->pending_join_iface_addr);
	}
	if (bss) {
		freq = bss->freq;
		da16x_p2p_prt("[%s] P2P: Target GO operating frequency "
			   "from BSS table: %d MHz (SSID %s)\n",
			   __func__, freq,
			   wpa_ssid_txt(bss->ssid, bss->ssid_len));
	}
	if (freq > 0) {
		u16 method;

		if (wpas_check_freq_conflict(wpa_s, freq) > 0) {
			da16x_p2p_prt("[%s] "P2P_EVENT_GROUP_FORMATION_FAILURE
				"reason=FREQ_CONFLICT\n",
				__func__);

			return;
		}

		da16x_p2p_prt("[%s] P2P: Send Provision Discovery Request "
			   "prior to joining an existing group (GO " MACSTR
			   " freq=%u MHz)\n",
			   __func__,
			   MAC2STR(wpa_s->pending_join_dev_addr), freq);

		wpa_s->pending_pd_before_join = 1;

		switch (wpa_s->pending_join_wps_method) {
		case WPS_PIN_DISPLAY:
			method = WPS_CONFIG_KEYPAD;
			break;
		case WPS_PIN_KEYPAD:
			method = WPS_CONFIG_DISPLAY;
			break;
		case WPS_PBC:
			method = WPS_CONFIG_PUSHBUTTON;
			break;
		default:
			method = 0;
			break;
		}

		if ((p2p_get_provisioning_info(wpa_s->global->p2p,
					       wpa_s->pending_join_dev_addr) ==
		     method)) {
			/*
			 * We have already performed provision discovery for
			 * joining the group. Proceed directly to join
			 * operation without duplicated provision discovery. */
			da16x_p2p_prt("[%s] P2P: Provision discovery "
				   "with " MACSTR " already done - proceed to "
				   "join\n",
				   __func__,
				   MAC2STR(wpa_s->pending_join_dev_addr));
			wpa_s->pending_pd_before_join = 0;

			goto start;
		}

		if (p2p_prov_disc_req(wpa_s->global->p2p,
				      wpa_s->pending_join_dev_addr, method, 1,
				      freq, wpa_s->user_initiated_pd) < 0) {
			da16x_p2p_prt("[%s] P2P: Failed to send Provision "
				   "Discovery Request before joining an "
				   "existing group\n", __func__);

			wpa_s->pending_pd_before_join = 0;
			goto start;
		}
		return;
	}

	da16x_p2p_prt("[%s] P2P: Failed to find BSS/GO - try again later\n",
			__func__);

	da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
	da16x_eloop_register_timeout(1, 0, wpas_p2p_join_scan, wpa_s, NULL);

	wpas_p2p_check_join_scan_limit(wpa_s);
	return;

start:
	/* Start join operation immediately */
	wpas_p2p_join_start(wpa_s, 0, NULL, 0);
}


static void wpas_p2p_join_scan_req(struct wpa_supplicant *wpa_s, int freq,
				   const u8 *ssid, size_t ssid_len)
{
	int ret;
	struct wpa_driver_scan_params params;
	struct wpabuf *wps_ie = NULL, *ies = NULL;
	size_t ielen;
	int freqs[2] = { 0, 0 };

	os_memset(&params, 0, sizeof(params));

	/* P2P Wildcard SSID */
	params.num_ssids = 1;
	if (ssid && ssid_len) {
		params.ssids[0].ssid = ssid;
		params.ssids[0].ssid_len = ssid_len;
		os_memcpy(wpa_s->p2p_join_ssid, ssid, ssid_len);
		wpa_s->p2p_join_ssid_len = ssid_len;
	} else {
		params.ssids[0].ssid = (u8 *) P2P_WILDCARD_SSID;
		params.ssids[0].ssid_len = P2P_WILDCARD_SSID_LEN;
		wpa_s->p2p_join_ssid_len = 0;
	}

#ifdef	CONFIG_WPS
	wpa_s->wps->dev.p2p = 1;
	wps_ie = wps_build_probe_req_ie(DEV_PW_DEFAULT, &wpa_s->wps->dev,
					wpa_s->wps->uuid, WPS_REQ_ENROLLEE, 0,
					NULL);
	if (wps_ie == NULL) {
		wpas_p2p_scan_res_join(wpa_s, NULL);
		return;
	}
#else
da16x_p2p_prt("[%s] Have to include CONFIG_WPS features ...\n", __func__);
#endif	/* CONFIG_WPS */

#ifdef	CONFIG_P2P_UNUSED_CMD
	ielen = p2p_scan_ie_buf_len(wpa_s->global->p2p);
#else
	ielen = 100;
#endif	/* CONFIG_P2P_UNUSED_CMD */
	ies = wpabuf_alloc(wpabuf_len(wps_ie) + ielen);
	if (ies == NULL) {
		wpabuf_free(wps_ie);
		wpas_p2p_scan_res_join(wpa_s, NULL);
		return;
	}
	wpabuf_put_buf(ies, wps_ie);
	wpabuf_free(wps_ie);

	p2p_scan_ie(wpa_s->global->p2p, ies, NULL);

	params.p2p_probe = 1;
	params.extra_ies = wpabuf_head(ies);
	params.extra_ies_len = wpabuf_len(ies);

	if (!freq) {
		int oper_freq;
		/*
		 * If freq is not provided, check the operating freq of the GO
		 * and use a single channel scan on if possible.
		 */
		oper_freq = p2p_get_oper_freq(wpa_s->global->p2p,
					      wpa_s->pending_join_iface_addr);
		if (oper_freq > 0)
			freq = oper_freq;
	}
	if (freq > 0) {
		freqs[0] = freq;
		params.freqs = freqs;
	}

	/*
	 * Run a scan to update BSS table and start Provision Discovery once
	 * the new scan results become available.
	 */
	ret = wpa_drv_scan(wpa_s, &params);
	if (!ret) {
		os_get_reltime(&wpa_s->scan_trigger_time);
		wpa_s->scan_res_handler = wpas_p2p_scan_res_join;
		wpa_s->own_scan_requested = 1;
	}

	wpabuf_free(ies);

	if (ret) {
		da16x_p2p_prt("[%s] P2P: Failed to start scan for join - "
			   "try again later\n", __func__);

		da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);
		da16x_eloop_register_timeout(1, 0, wpas_p2p_join_scan, wpa_s,
					    NULL);

		wpas_p2p_check_join_scan_limit(wpa_s);
	}
}


static void wpas_p2p_join_scan(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_join_scan_req(wpa_s, 0, NULL, 0);
}


static int wpas_p2p_join(struct wpa_supplicant *wpa_s, const u8 *iface_addr,
			 const u8 *dev_addr, enum p2p_wps_method wps_method,
			 int op_freq, const u8 *ssid, size_t ssid_len)
{
	struct p2p_device *dev;

	da16x_p2p_prt("[%s] P2P: Request to join existing group (iface "
		     MACSTR " dev " MACSTR " op_freq=%d)\n", __func__,
		     MAC2STR(iface_addr), MAC2STR(dev_addr), op_freq);

	dev = p2p_get_device(wpa_s->global->p2p, dev_addr);
	if (dev == NULL || (dev->flags & P2P_DEV_PROBE_REQ_ONLY)) {
		da16x_p2p_prt("[%s] Cannot connect to unknown P2P Device "
				MACSTR "\n", __func__, MAC2STR(dev_addr));
		
		return -1;
	}

	if (ssid && ssid_len) {
		da16x_p2p_prt("P2P: Group SSID specified: %s\n",
			   __func__,
			   wpa_ssid_txt(ssid, ssid_len));
	}

	os_memcpy(wpa_s->pending_join_iface_addr, iface_addr, ETH_ALEN);
	os_memcpy(wpa_s->pending_join_dev_addr, dev_addr, ETH_ALEN);
	wpa_s->pending_join_wps_method = wps_method;

	/* Make sure we are not running find during connection establishment */
	wpas_p2p_stop_find(wpa_s);

	wpa_s->p2p_join_scan_count = 0;
	wpas_p2p_join_scan_req(wpa_s, op_freq, ssid, ssid_len);
	return 0;
}


static int wpas_p2p_join_start(struct wpa_supplicant *wpa_s, int freq,
			       const u8 *ssid, size_t ssid_len)
{
	struct wpa_supplicant *group;
	struct p2p_go_neg_results res;
	struct wpa_bss *bss;

	group = wpas_p2p_get_group_iface(wpa_s, 0, 0);
	if (group == NULL)
		return -1;
	if (group != wpa_s) {
		os_memcpy(group->p2p_pin, wpa_s->p2p_pin,
			  sizeof(group->p2p_pin));
		group->p2p_wps_method = wpa_s->p2p_wps_method;
	} else {
		/*
		 * Need to mark the current interface for p2p_group_formation
		 * when a separate group interface is not used. This is needed
		 * to allow p2p_cancel stop a pending p2p_connect-join.
		 * wpas_p2p_init_group_interface() addresses this for the case
		 * where a separate group interface is used.
		 */
		wpa_s->global->p2p_group_formation = wpa_s;
	}

	group->p2p_in_provisioning = 1;
	group->p2p_fallback_to_go_neg = wpa_s->p2p_fallback_to_go_neg;

	os_memset(&res, 0, sizeof(res));
	os_memcpy(res.peer_device_addr, wpa_s->pending_join_dev_addr, ETH_ALEN);
	os_memcpy(res.peer_interface_addr, wpa_s->pending_join_iface_addr,
		  ETH_ALEN);

	res.wps_method = (enum p2p_wps_method)wpa_s->pending_join_wps_method;
	if (freq && ssid && ssid_len) {
		res.freq = freq;
		res.ssid_len = ssid_len;
		os_memcpy(res.ssid, ssid, ssid_len);
	} else {
		bss = wpa_bss_get_bssid_latest(wpa_s,
					       wpa_s->pending_join_iface_addr);
		if (bss) {
			res.freq = bss->freq;
			res.ssid_len = bss->ssid_len;
			os_memcpy(res.ssid, bss->ssid, bss->ssid_len);

			da16x_p2p_prt("[%s] P2P: Join target GO operating "
				"frequency from BSS table: %d MHz (SSID %s)\n",
				__func__,
				bss->freq,
				wpa_ssid_txt(bss->ssid, bss->ssid_len));
		}
	}

	if (wpa_s->off_channel_freq || wpa_s->roc_waiting_drv_freq) {
		da16x_p2p_prt("[%s] P2P: Cancel remain-on-channel prior to "
			   "starting client\n", __func__);

		wpa_drv_cancel_remain_on_channel(wpa_s);
		wpa_s->off_channel_freq = 0;
		wpa_s->roc_waiting_drv_freq = 0;
	}
	wpas_start_wps_enrollee(group, &res);

	/*
	 * Allow a longer timeout for join-a-running-group than normal 15
	 * second group formation timeout since the GO may not have authorized
	 * our connection yet.
	 */
	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout, wpa_s, NULL);
	da16x_eloop_register_timeout(60, 0, wpas_p2p_group_formation_timeout,
				    wpa_s, NULL);
	return 0;
}


/**
 * wpas_p2p_connect - Request P2P Group Formation to be started
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supp_add_iface()
 * @peer_addr: Address of the peer P2P Device
 * @pin: PIN to use during provisioning or %NULL to indicate PBC mode
 * @join: Whether to join an existing group (as a client) instead of starting
 *	Group Owner negotiation; @peer_addr is BSSID in that case
 * @go_intent: GO Intent or -1 to use default
 * @freq: Frequency for the group or 0 for auto-selection
 * @pd: Whether to send Provision Discovery prior to GO Negotiation as an
 *	interoperability workaround when initiating group formation
 * Returns: 0 or new PIN (if pin was %NULL) on success, -1 on unspecified
 *	failure, -2 on failure due to channel not currently available,
 *	-3 if forced channel is not supported
 */
int wpas_p2p_connect(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
		     const char *pin, enum p2p_wps_method wps_method,
		     int join, int go_intent, int freq, int pd)
{
	int ret = 0;
	enum wpa_driver_if_type iftype;
	const u8 *if_addr;
#ifdef	CONFIG_P2P_OPTION
	struct wpa_ssid *ssid = NULL;

	if (persistent_id >= 0) {
		ssid = wpa_config_get_network(wpa_s->conf, persistent_id);
		if (ssid == NULL || ssid->disabled != 2 ||
		    ssid->mode != WPAS_MODE_P2P_GO)
			return -1;
	}
#endif	/* CONFIG_P2P_OPTION */

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	wpa_s->global->p2p_fail_on_wps_complete = 0;

	if (go_intent < 0)
		go_intent = wpa_s->conf->p2p_go_intent;

	wpa_s->p2p_wps_method = wps_method;
#ifdef	CONFIG_P2P_OPTION
	wpa_s->p2p_persistent_group = !!persistent_group;
	wpa_s->p2p_persistent_id = persistent_id;
#endif	/* CONFIG_P2P_OPTION */
	wpa_s->p2p_go_intent = go_intent;
	wpa_s->p2p_connect_freq = freq;
	wpa_s->p2p_fallback_to_go_neg = 0;
	wpa_s->p2p_pd_before_go_neg = !!pd;

	if (pin)
		os_strlcpy(wpa_s->p2p_pin, pin, sizeof(wpa_s->p2p_pin));
	else
#ifdef	CONFIG_WPS
	if (wps_method == WPS_PIN_DISPLAY) {
#if 0	/* by Shingu 20160406 (SIGMA) */
		ret = wps_generate_pin();
#else
		int val;

		if (wpa_config_read_nvram_int("TEMP_PIN", &val) != -1) {
			ret = val;
		} else {
			ret = wps_generate_pin();
		}
#endif	/* 0 */
		os_snprintf(wpa_s->p2p_pin, sizeof(wpa_s->p2p_pin), "%08d",
			    ret);

		da16x_p2p_prt("[%s] P2P: Randomly generated PIN: %s\n",
			   __func__, wpa_s->p2p_pin);
	} else
#endif	/* CONFIG_WPS */
	{
		wpa_s->p2p_pin[0] = '\0';
	}

	if (join) {
		u8 iface_addr[ETH_ALEN], dev_addr[ETH_ALEN];
#ifdef	CONFIG_P2P_OPTION
		if (auth) {
			da16x_p2p_prt("[%s] P2P: Authorize invitation to "
				   "connect a running group from " MACSTR "\n",
				   __func__, MAC2STR(peer_addr));
			os_memcpy(wpa_s->p2p_auth_invite, peer_addr, ETH_ALEN);

			return ret;
		}
#endif	/* CONFIG_P2P_OPTION */
		os_memcpy(dev_addr, peer_addr, ETH_ALEN);
		if (p2p_get_interface_addr(wpa_s->global->p2p, peer_addr,
					   iface_addr) < 0) {
			os_memcpy(iface_addr, peer_addr, ETH_ALEN);
			p2p_get_dev_addr(wpa_s->global->p2p, peer_addr,
					 dev_addr);
		}
#ifdef	CONFIG_P2P_OPTION
		if (auto_join) {
			os_get_reltime(&wpa_s->p2p_auto_started);
			da16x_p2p_prt("[%s] P2P: Auto join started at "
				     "%ld.%06ld\n", __func__,
				     wpa_s->p2p_auto_started.sec,
				     wpa_s->p2p_auto_started.usec);
		}
#endif	/* CONFIG_P2P_OPTION */
		wpa_s->user_initiated_pd = 1;
		if (wpas_p2p_join(wpa_s, iface_addr, dev_addr, wps_method,
				  freq, NULL, 0) < 0)
			return -1;
		return ret;
	}

	wpa_s->create_p2p_iface = wpas_p2p_create_iface(wpa_s);

	if (wpa_s->create_p2p_iface) {
		/* Prepare to add a new interface for the group */
		iftype = WPA_IF_P2P_GROUP;
		if (go_intent == 15)
			iftype = WPA_IF_P2P_GO;
		if (wpas_p2p_add_group_interface(wpa_s, iftype) < 0) {
			da16x_p2p_prt("[%s] P2P: Failed to allocate a new "
				   "interface for the group\n", __func__);
			return -1;
		}

		if_addr = wpa_s->pending_interface_addr;
	} else
		if_addr = wpa_s->own_addr;

	if (wpas_p2p_start_go_neg(wpa_s, peer_addr, wps_method,
				  go_intent, if_addr) < 0) {
		if (wpa_s->create_p2p_iface)
			wpas_p2p_remove_pending_group_interface(wpa_s);
		return -1;
	}
	return ret;
}

static int wpas_p2p_init_go_params(struct wpa_supplicant *wpa_s,
				   struct p2p_go_neg_results *params,
				   int freq, int ht40, int vht,
				   const struct p2p_channels *channels)
{
	int res, *freqs;
	unsigned int num, i;

	os_memset(params, 0, sizeof(*params));
	params->role_go = 1;
	params->ht40 = ht40;
	params->vht = vht;
	if (freq) {
		if (!freq_included(channels, freq)) {
			da16x_p2p_prt("[%s] P2P: Forced GO freq %d MHz not "
				   "accepted\n", __func__, freq);
			return -1;
		}
		da16x_p2p_prt("[%s] P2P: Set GO freq based on forced "
			   "frequency %d MHz\n", __func__, freq);

		params->freq = freq;
	} else if (wpa_s->conf->p2p_oper_reg_class == 81 &&
		   wpa_s->conf->p2p_oper_channel >= 1 &&
		   wpa_s->conf->p2p_oper_channel <= 11 &&
		   freq_included(channels,
				 2407 + 5 * wpa_s->conf->p2p_oper_channel)) {
		params->freq = 2407 + 5 * wpa_s->conf->p2p_oper_channel;

		da16x_p2p_prt("[%s] P2P: Set GO freq based on configured "
			   "frequency %d MHz\n", __func__, params->freq);
#ifdef	CONFIG_AP_SUPPORT_5GHZ
	} else if ((wpa_s->conf->p2p_oper_reg_class == 115 ||
		    wpa_s->conf->p2p_oper_reg_class == 116 ||
		    wpa_s->conf->p2p_oper_reg_class == 117 ||
		    wpa_s->conf->p2p_oper_reg_class == 124 ||
		    wpa_s->conf->p2p_oper_reg_class == 126 ||
		    wpa_s->conf->p2p_oper_reg_class == 127) &&
		   freq_included(channels,
				 5000 + 5 * wpa_s->conf->p2p_oper_channel)) {
		params->freq = 5000 + 5 * wpa_s->conf->p2p_oper_channel;

		da16x_p2p_prt("[%s] P2P: Set GO freq based on configured "
			   "frequency %d MHz\n", __func__, params->freq);
#endif	/* CONFIG_AP_SUPPORT_5GHZ */
#ifdef	CONFIG_P2P_UNUSED_CMD
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_overall_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_overall_freq) &&
		   freq_included(channels, wpa_s->best_overall_freq)) {
		params->freq = wpa_s->best_overall_freq;

		da16x_p2p_prt("[%s] P2P: Set GO freq based on best overall "
			   "channel %d MHz\n", __func__, params->freq);
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_24_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_24_freq) &&
		   freq_included(channels, wpa_s->best_24_freq)) {
		params->freq = wpa_s->best_24_freq;

		da16x_p2p_prt("[%s] P2P: Set GO freq based on best 2.4 GHz "
			   "channel %d MHz\n", __func__, params->freq);
#ifdef	CONFIG_AP_SUPPORT_5GHZ
	} else if (wpa_s->conf->p2p_oper_channel == 0 &&
		   wpa_s->best_5_freq > 0 &&
		   p2p_supported_freq_go(wpa_s->global->p2p,
					 wpa_s->best_5_freq) &&
		   freq_included(channels, wpa_s->best_5_freq)) {
		params->freq = wpa_s->best_5_freq;
		da16x_p2p_prt("[%a] P2P: Set GO freq based on best 5 GHz "
			   "channel %d MHz\n", __func__, params->freq);
#endif	/* CONFIG_AP_SUPPORT_5GHZ */
#endif	/* CONFIG_P2P_UNUSED_CMD */
#if 1	/* by Shingu 20160816 (P2P Channel) */
	} else if (wpa_s->conf->p2p_oper_reg_class == 81 &&
		   !wpa_s->conf->p2p_oper_channel &&
		   wpa_s->global->p2p->op_channel &&
		   freq_included(channels,
				 2407 + 5 * wpa_s->global->p2p->op_channel)) {
		params->freq = 2407 + 5 * wpa_s->global->p2p->op_channel;
#endif	/* 0 */
	} else {
		int chan;
		for (chan = 0; chan < 11; chan++) {
			params->freq = 2412 + chan * 5;
#ifdef	CONFIG_P2P_UNUSED_CMD
			if (!wpas_p2p_disallowed_freq(wpa_s->global,
						      params->freq) &&
			    freq_included(channels, params->freq))
				break;
#else
			if (freq_included(channels, params->freq))
				break;
#endif	/* CONFIG_P2P_UNUSED_CMD */
		}
		if (chan == 11) {
			da16x_p2p_prt("[%s] P2P: No 2.4 GHz channel allowed\n",
					__func__);
			return -1;
		}
		da16x_p2p_prt("[%s] P2P: Set GO freq %d MHz (no preference "
			   "known)\n", __func__, params->freq);
	}

	freqs = os_calloc(wpa_s->num_multichan_concurrent, sizeof(int));
	if (!freqs)
		return -1;

	res = wpas_p2p_valid_oper_freqs(wpa_s, freqs,
					wpa_s->num_multichan_concurrent);
	if (res < 0) {
		os_free(freqs);
		return -1;
	}
	num = res;

	for (i = 0; i < num; i++) {
		if (freq && freqs[i] == freq)
			break;
		if (!freq && freq_included(channels, freqs[i])) {
			da16x_p2p_prt("[%s] P2P: Force GO on a channel "
				"we are already using (%u MHz)\n",
				   __func__, freqs[i]);

			params->freq = freqs[i];
			break;
		}
	}

	if (i == num) {
		if (wpas_p2p_num_unused_channels(wpa_s) <= 0) {
			if (freq) {
				da16x_p2p_prt("[%s] P2P: Cannot force GO on "
					"freq (%u MHz) as all the channels "
					"are in use\n", __func__, freq);
			} else {
				da16x_p2p_prt("[%s] P2P: Cannot force GO on "
					"any of the channels we are already "
					"using\n", __func__);
			}

			os_free(freqs);
			return -1;
		} else if (num == 0) {
			da16x_p2p_prt("[%s] P2P: Use one of the free channels\n",
					__func__);
		} else {
			da16x_p2p_prt("[%s] P2P: Cannot force GO on any of the "
					"channels we are already using. "
					"Use one of the free channels\n",
					__func__);
		}
	}

	os_free(freqs);
	return 0;
}


static struct wpa_supplicant *
wpas_p2p_get_group_iface(struct wpa_supplicant *wpa_s, int addr_allocated,
			 int go)
{
	struct wpa_supplicant *group_wpa_s;

	if (!wpas_p2p_create_iface(wpa_s)) {
		da16x_p2p_prt("[%s] P2P: Use same interface for group "
			"operations\n", __func__);

		wpa_s->p2p_first_connection_timeout = 0;
		return wpa_s;
	}

	if (wpas_p2p_add_group_interface(wpa_s, go ? WPA_IF_P2P_GO :
					 WPA_IF_P2P_CLIENT) < 0) {
		da16x_p2p_prt("[%s] P2P: Failed to add group interface\n",
				__func__);

		return NULL;
	}
	group_wpa_s = wpas_p2p_init_group_interface(wpa_s, go);
	if (group_wpa_s == NULL) {
		da16x_p2p_prt("[%s] P2P: Failed to initialize group interface\n",
				__func__);

		wpas_p2p_remove_pending_group_interface(wpa_s);
		return NULL;
	}

	da16x_p2p_prt("[%s] P2P: Use separate group interface %s\n",
			__func__, group_wpa_s->ifname);
	group_wpa_s->p2p_first_connection_timeout = 0;
	return group_wpa_s;
}


/**
 * wpas_p2p_group_add - Add a new P2P group with local end as Group Owner
 * @wpa_s: Pointer to wpa_supplicant data from wpa_supp_add_iface()
 * @persistent_group: Whether to create a persistent group
 * @freq: Frequency for the group or 0 to indicate no hardcoding
 * @ht40: Start GO with 40 MHz channel width
 * @vht:  Start GO with VHT support
 * Returns: 0 on success, -1 on failure
 *
 * This function creates a new P2P group with the local end as the Group Owner,
 * i.e., without using Group Owner Negotiation.
 */
int wpas_p2p_group_add(struct wpa_supplicant *wpa_s, int persistent_group,
		       int freq, int ht40, int vht)
{
	struct p2p_go_neg_results params;

	if (!wpa_s->global->p2p || get_run_mode() != P2P_GO_FIXED_MODE)
		return -1;

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	/* Make sure we are not running find during connection establishment */
	da16x_p2p_prt("[%s] P2P: Stop any on-going P2P FIND\n", __func__);

	wpas_p2p_stop_find_oper(wpa_s);

	if (wpas_p2p_init_go_params(wpa_s, &params, freq, ht40, vht, NULL))
		return -1;
	if (params.freq &&
	    !p2p_supported_freq_go(wpa_s->global->p2p, params.freq)) {
		da16x_p2p_prt("[%s] P2P: The selected channel for GO "
			   "(%u MHz) is not supported for P2P uses\n",
			   __func__, params.freq);
		return -1;
	}

#if 0   /* by Shingu 20160726 (P2P Config) */
	p2p_go_params(wpa_s->global->p2p, &params);
#else
	if (wpa_s->conf->p2p_ssid && wpa_s->conf->p2p_passphrase) {
		os_memcpy(params.ssid, wpa_s->conf->p2p_ssid,
			  wpa_s->conf->p2p_ssid_len);
		params.ssid_len = wpa_s->conf->p2p_ssid_len;
		strcpy(params.passphrase, wpa_s->conf->p2p_passphrase);
	} else {
		p2p_go_params(wpa_s->global->p2p, &params);
	}
#endif	/* 1 */
#ifdef	CONFIG_P2P_OPTION
	params.persistent_group = persistent_group;
#endif	/* CONFIG_P2P_OPTION */

	wpa_s = wpas_p2p_get_group_iface(wpa_s, 0, 1);
	if (wpa_s == NULL)
		return -1;
	wpas_start_wps_go(wpa_s, &params, 0);

	return 0;
}


#ifdef	CONFIG_P2P_OPTION
static int wpas_start_p2p_client(struct wpa_supplicant *wpa_s,
				 struct wpa_ssid *params, int addr_allocated,
				 int freq)
{
	struct wpa_ssid *ssid;

	wpa_s = wpas_p2p_get_group_iface(wpa_s, addr_allocated, 0);
	if (wpa_s == NULL)
		return -1;
	wpa_s->p2p_last_4way_hs_fail = NULL;

#ifdef	CONFIG_AP
	wpa_supplicant_ap_deinit(wpa_s);
#endif	/* CONFIG_AP */

	ssid = wpa_config_add_network(wpa_s->conf, wpa_s->ifname,
				      FIXED_NETWORK_ID_NONE);
	if (ssid == NULL)
		return -1;
	wpa_config_set_network_defaults(ssid);
	ssid->temporary = 1;
	ssid->proto = WPA_PROTO_RSN;
	ssid->pairwise_cipher = WPA_CIPHER_CCMP;
	ssid->group_cipher = WPA_CIPHER_CCMP;
	ssid->key_mgmt = WPA_KEY_MGMT_PSK;
	ssid->ssid = os_malloc(params->ssid_len);
	if (ssid->ssid == NULL) {
		wpa_config_remove_network(wpa_s->conf, ssid->id);
		return -1;
	}
	os_memcpy(ssid->ssid, params->ssid, params->ssid_len);
	ssid->ssid_len = params->ssid_len;
	ssid->p2p_group = 1;
	ssid->export_keys = 1;
	if (params->psk_set) {
		os_memcpy(ssid->psk, params->psk, 32);
		ssid->psk_set = 1;
	}
	if (params->passphrase)
		ssid->passphrase = os_strdup(params->passphrase);

	wpa_s->show_group_started = 1;
#ifdef	CONFIG_P2P_OPTION
	wpa_s->p2p_in_invitation = 1;
	wpa_s->p2p_invite_go_freq = freq;
#endif	/* CONFIG_P2P_OPTION */

	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				  wpa_s, NULL);
	da16x_eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT, 0,
				    wpas_p2p_group_formation_timeout,
				    wpa_s, NULL);

	wpa_supplicant_select_network(wpa_s, ssid);

	return 0;
}
#endif	/* CONFIG_P2P_OPTION */


#ifdef	 CONFIG_P2P_OPTION
int wpas_p2p_group_add_persistent(struct wpa_supplicant *wpa_s,
				  struct wpa_ssid *ssid, int addr_allocated,
				  int force_freq, int neg_freq, int ht40,
				  int vht, const struct p2p_channels *channels,
				  int connection_timeout)
{
	struct p2p_go_neg_results params;
	int freq = 0;
#ifdef	CONFIG_P2P_OPTION
	int go = 0;
#endif	/* CONFIG_P2P_OPTION */

	if (ssid->disabled != 2 || ssid->ssid == NULL)
		return -1;

#ifdef	CONFIG_P2P_OPTION
	if (wpas_get_p2p_group(wpa_s, ssid->ssid, ssid->ssid_len, &go) &&
	    go == (ssid->mode == WPAS_MODE_P2P_GO)) {
		da16x_p2p_prt("[%s] P2P: Requested persistent group is "
			   "already running\n", __func__);
		return 0;
	}
#endif	/* CONFIG_P2P_OPTION */

	os_free(wpa_s->global->add_psk);
	wpa_s->global->add_psk = NULL;

	/* Make sure we are not running find during connection establishment */
	wpas_p2p_stop_find_oper(wpa_s);

	wpa_s->p2p_fallback_to_go_neg = 0;

#ifdef	CONFIG_P2P_OPTION
	if (ssid->mode == WPAS_MODE_INFRA)
		return wpas_start_p2p_client(wpa_s, ssid, addr_allocated, freq);
#endif	/* CONFIG_P2P_OPTION */

	if (ssid->mode != WPAS_MODE_P2P_GO)
		return -1;

	if (wpas_p2p_init_go_params(wpa_s, &params, freq, ht40, vht, channels))
		return -1;

	params.role_go = 1;
	params.psk_set = ssid->psk_set;
	if (params.psk_set)
		os_memcpy(params.psk, ssid->psk, sizeof(params.psk));
	if (ssid->passphrase) {
		if (os_strlen(ssid->passphrase) >= sizeof(params.passphrase)) {
			da16x_p2p_prt("[%s] P2P: Invalid passphrase in "
				   "persistent group\n", __func__);
			return -1;
		}
		os_strlcpy(params.passphrase, ssid->passphrase,
			   sizeof(params.passphrase));
	}
	os_memcpy(params.ssid, ssid->ssid, ssid->ssid_len);
	params.ssid_len = ssid->ssid_len;
	params.persistent_group = 1;

	wpa_s = wpas_p2p_get_group_iface(wpa_s, addr_allocated, 1);
	if (wpa_s == NULL)
		return -1;

	wpa_s->p2p_first_connection_timeout = connection_timeout;
	wpas_start_wps_go(wpa_s, &params, 0);

	return 0;
}
#endif	/* CONFIG_P2P_OPTION */


static void wpas_p2p_ie_update(void *ctx, struct wpabuf *beacon_ies,
			       struct wpabuf *proberesp_ies)
{
	struct wpa_supplicant *wpa_s = ctx;

#ifdef	CONFIG_AP
	if (wpa_s->ap_iface) {
		struct hostapd_data *hapd = wpa_s->ap_iface->bss[0];

		if (!(hapd->conf->p2p & P2P_GROUP_OWNER)) {
			wpabuf_free(beacon_ies);
			wpabuf_free(proberesp_ies);
			return;
		}
		if (beacon_ies) {
			wpabuf_free(hapd->p2p_beacon_ie);
			hapd->p2p_beacon_ie = beacon_ies;
		}
		wpabuf_free(hapd->p2p_probe_resp_ie);
		hapd->p2p_probe_resp_ie = proberesp_ies;
	} else
#endif	/* CONFIG_AP */
	{
		wpabuf_free(beacon_ies);
		wpabuf_free(proberesp_ies);
	}

	wpa_supplicant_ap_update_beacon(wpa_s);
}


static void wpas_p2p_idle_update(void *ctx, int idle)
{
#ifdef	CONFIG_AP
	struct wpa_supplicant *wpa_s = ctx;

	if (!wpa_s->ap_iface)
		return;

	da16x_p2p_prt("[%s] P2P: GO - group %sidle\n",
				__func__, idle ? "" : "not ");

	if (idle) {
		if (wpa_s->global->p2p_fail_on_wps_complete &&
		    wpa_s->p2p_in_provisioning) {
			wpas_p2p_grpform_fail_after_wps(wpa_s);
			return;
		}
		wpas_p2p_set_group_idle_timeout(wpa_s);
	} else {
		da16x_eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s,
					  NULL);
	}
#else
da16x_p2p_prt("[%s] What do I do for this one ???\n", __func__);
#endif	/* CONFIG_AP */
}


struct p2p_group * wpas_p2p_group_init(struct wpa_supplicant *wpa_s,
				       struct wpa_ssid *ssid)
{
	struct p2p_group *group;
	struct p2p_group_config *cfg;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return NULL;

	cfg = os_zalloc(sizeof(*cfg));
	if (cfg == NULL)
		return NULL;

#ifdef	CONFIG_P2P_OPTION
	if (ssid->p2p_persistent_group && wpa_s->conf->persistent_reconnect)
		cfg->persistent_group = 2;
	else if (ssid->p2p_persistent_group)
		cfg->persistent_group = 1;
#endif	/* CONFIG_P2P_OPTION */
	os_memcpy(cfg->interface_addr, wpa_s->own_addr, ETH_ALEN);
	if (wpa_s->max_stations &&
	    wpa_s->max_stations < wpa_s->conf->max_num_sta)
		cfg->max_clients = wpa_s->max_stations;
	else
		cfg->max_clients = wpa_s->conf->max_num_sta;
	os_memcpy(cfg->ssid, ssid->ssid, ssid->ssid_len);
	cfg->ssid_len = ssid->ssid_len;
	cfg->freq = ssid->frequency;
	cfg->cb_ctx = wpa_s;
	cfg->ie_update = wpas_p2p_ie_update;
	cfg->idle_update = wpas_p2p_idle_update;

	group = p2p_group_init(wpa_s->global->p2p, cfg);
	if (group == NULL)
		os_free(cfg);
	if (ssid->mode != WPAS_MODE_P2P_GROUP_FORMATION)
		p2p_group_notif_formation_done(group);
	wpa_s->p2p_group = group;
	return group;
}


void wpas_p2p_wps_success(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
			  int registrar)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (!wpa_s->p2p_in_provisioning) {
		da16x_p2p_prt("[%s] P2P: Ignore WPS success event - P2P "
			   "provisioning not in progress\n",
			   __func__);
		return;
	}

	if (ssid && ssid->mode == WPAS_MODE_INFRA) {
		u8 go_dev_addr[ETH_ALEN];
		os_memcpy(go_dev_addr, wpa_s->bssid, ETH_ALEN);

#ifdef	CONFIG_P2P_OPTION
		wpas_p2p_persistent_group(wpa_s, go_dev_addr, ssid->ssid,
					  ssid->ssid_len);
#endif	/* CONFIG_P2P_OPTION */
		/* Clear any stored provisioning info */
		p2p_clear_provisioning_info(wpa_s->global->p2p, go_dev_addr);
	}

	da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				  wpa_s, NULL);
	wpa_s->p2p_go_group_formation_completed = 1;
	if (ssid && ssid->mode == WPAS_MODE_INFRA) {
		/*
		 * Use a separate timeout for initial data connection to
		 * complete to allow the group to be removed automatically if
		 * something goes wrong in this step before the P2P group idle
		 * timeout mechanism is taken into use.
		 */
		da16x_p2p_prt("[%s] P2P: Re-start group formation timeout "
			"(%d seconds) as client for initial connection\n",
			__func__, P2P_MAX_INITIAL_CONN_WAIT);

		da16x_eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT, 0,
					    wpas_p2p_group_formation_timeout,
					    wpa_s, NULL);
	} else if (ssid) {
		/*
		 * Use a separate timeout for initial data connection to
		 * complete to allow the group to be removed automatically if
		 * the client does not complete data connection successfully.
		 */
		da16x_p2p_prt("[%s] P2P: Re-start group formation timeout "
				"(%d seconds) as GO for initial connection\n",
				__func__, P2P_MAX_INITIAL_CONN_WAIT_GO);

		da16x_eloop_register_timeout(P2P_MAX_INITIAL_CONN_WAIT_GO, 0,
					    wpas_p2p_group_formation_timeout,
					    wpa_s, NULL);
		/*
		 * Complete group formation on first successful data connection
		 */
		wpa_s->p2p_go_group_formation_completed = 0;
	}
	if (wpa_s->global->p2p)
		p2p_wps_success_cb(wpa_s->global->p2p, peer_addr);
	wpas_group_formation_completed(wpa_s, 1);
}


void wpas_p2p_wps_failed(struct wpa_supplicant *wpa_s,
			 struct wps_event_fail *fail)
{
	if (!wpa_s->p2p_in_provisioning) {
		da16x_p2p_prt("[%s] P2P: Ignore WPS fail event - P2P "
			   "provisioning not in progress\n",
			   __func__);
		return;
	}

	if (wpa_s->go_params) {
		p2p_clear_provisioning_info(
			wpa_s->global->p2p,
			wpa_s->go_params->peer_device_addr);
	}

#if 0	/* by Bright */
	wpas_notify_p2p_wps_failed(wpa_s, fail);
#endif	/* 0 */

	if (wpa_s == wpa_s->global->p2p_group_formation) {
		/*
		 * Allow some time for the failed WPS negotiation exchange to
		 * complete, but remove the group since group formation cannot
		 * succeed after provisioning failure.
		 */
		da16x_p2p_prt("[%s] P2P: WPS step failed during group "
			"formation - reject connection from timeout\n",
			__func__);

		wpa_s->global->p2p_fail_on_wps_complete = 1;

		da16x_eloop_deplete_timeout(0, 50000,
					   wpas_p2p_group_formation_timeout,
					   wpa_s, NULL);
	}
}


int wpas_p2p_wps_eapol_cb(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->global->p2p_fail_on_wps_complete ||
	    !wpa_s->p2p_in_provisioning)
		return 0;

	wpas_p2p_grpform_fail_after_wps(wpa_s);

	return 1;
}

int wpas_p2p_scan_result_text(const u8 *ies, size_t ies_len, char *buf,
			      char *end)
{
	return p2p_scan_result_text(ies, ies_len, buf, end);
}


static void wpas_p2p_clear_pending_action_tx(struct wpa_supplicant *wpa_s)
{
	if (!offchannel_pending_action_tx(wpa_s))
		return;

	da16x_p2p_prt("[%s] P2P: Drop pending Action TX due to new "
		   "operation request\n", __func__);

	offchannel_clear_pending_action_tx(wpa_s);
}


int wpas_p2p_find(struct wpa_supplicant *wpa_s, unsigned int timeout,
		  enum p2p_discovery_type type)
{
	wpas_p2p_clear_pending_action_tx(wpa_s);

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_find(wpa_s->global->p2p, timeout, type);
}


static int wpas_p2p_stop_find_oper(struct wpa_supplicant *wpa_s)
{
	wpas_p2p_clear_pending_action_tx(wpa_s);

	da16x_eloop_cancel_timeout(wpas_p2p_join_scan, wpa_s, NULL);

	if (wpa_s->global->p2p)
		p2p_stop_find(wpa_s->global->p2p);

	return 0;
}


int wpas_p2p_stop_find(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->global->p2p)
		return -1;

	if (wpas_p2p_stop_find_oper(wpa_s) > 0)
		return -1;

	wpas_p2p_remove_pending_group_interface(wpa_s);
	
	return 0;
}

int wpas_p2p_assoc_req_ie(struct wpa_supplicant *wpa_s, struct wpa_bss *bss,
			  u8 *buf, size_t len, int p2p_group)
{
	struct wpabuf *p2p_ie;
	int ret;

	if (wpa_s->global->p2p_disabled)
		return -1;
	if (wpa_s->global->p2p == NULL)
		return -1;
	if (bss == NULL)
		return -1;

	p2p_ie = wpa_bss_get_vendor_ie_multi(bss, P2P_IE_VENDOR_TYPE);
	ret = p2p_assoc_req_ie(wpa_s->global->p2p, bss->bssid, buf, len,
			       p2p_group, p2p_ie);
	wpabuf_free(p2p_ie);

	return ret;
}


int wpas_p2p_probe_req_rx(struct wpa_supplicant *wpa_s, const u8 *addr,
			  const u8 *dst, const u8 *bssid,
			  const u8 *ie, size_t ie_len, int ssi_signal)
{
	if (wpa_s->global->p2p_disabled)
		return 0;
	if (wpa_s->global->p2p == NULL)
		return 0;

	switch (p2p_probe_req_rx(wpa_s->global->p2p, addr, dst, bssid,
				 ie, ie_len)) {
	case P2P_PREQ_NOT_P2P:
#if 0	/* by Bright */
		wpas_notify_preq(wpa_s, addr, dst, bssid, ie, ie_len,
				 ssi_signal);
#endif	/* 0 */
		/* fall through */
	case P2P_PREQ_MALFORMED:
	case P2P_PREQ_NOT_LISTEN:
	case P2P_PREQ_NOT_PROCESSED:
	default: /* make gcc happy */
		return 0;
	case P2P_PREQ_PROCESSED:
		return 1;
	}
}


void wpas_p2p_rx_action(struct wpa_supplicant *wpa_s, const u8 *da,
			const u8 *sa, const u8 *bssid,
			u8 category, const u8 *data, size_t len, int freq)
{
	if (wpa_s->global->p2p_disabled)
		return;
	if (wpa_s->global->p2p == NULL)
		return;

	p2p_rx_action(wpa_s->global->p2p, da, sa, bssid, category, data, len,
		      freq);
}


void wpas_p2p_scan_ie(struct wpa_supplicant *wpa_s, struct wpabuf *ies)
{
	if (wpa_s->global->p2p_disabled)
		return;
	if (wpa_s->global->p2p == NULL)
		return;

	p2p_scan_ie(wpa_s->global->p2p, ies, NULL);
}

extern QueueHandle_t wifi_monitor_event_q;
#ifndef CONFIG_MONITOR_THREAD_EVENT_CHANGE
extern EventGroupHandle_t wifi_monitor_event_group;
#endif // CONFIG_MONITOR_THREAD_EVENT_CHANGE

void p2p_reconfig_net(void)
{
	int status;


	wifi_monitor_msg_buf_t *wifi_monitor_msg =
			os_malloc(sizeof(wifi_monitor_msg_buf_t));
	
	if (wifi_monitor_msg == NULL) {
		return;
	}

	/* P2P_NET_DEVICE */
	da16x_assoc_prt(">>> [%s] P2P_NET_DEVICE\n", __FUNCTION__);
	os_memcpy(wifi_monitor_msg->data, "2", 1);

	wifi_monitor_msg->cmd = WIFI_CMD_SEND_DHCP_RECONFIG;

	/* Send  Message */
#ifdef CONFIG_MONITOR_THREAD_EVENT_CHANGE
    status = xQueueSend(wifi_monitor_event_q, &wifi_monitor_msg, portNO_DELAY);
#else
    status = xQueueSend(wifi_monitor_event_q, wifi_monitor_msg, portNO_DELAY);
#endif
	if (status != pdTRUE) {
		da16x_iface_prt("[%s] tx_queue_send error !!! (%d)\n",
					__func__, status);
		os_free(wifi_monitor_msg);
		return;
	}
#ifndef CONFIG_MONITOR_THREAD_EVENT_CHANGE
	status = xEventGroupSetBits(wifi_monitor_event_group, WIFI_EVENT_SEND_MSG_TO_APP);
		
	if (status != ERR_OK) {
		da16x_err_prt("[%s] event set error !!! (%d)\n", __func__,
			     status);
		os_free(wifi_monitor_msg);
		return;
	}

	os_free(wifi_monitor_msg);

#endif
}


void wpas_p2p_group_deinit(struct wpa_supplicant *wpa_s)
{
#ifdef	CONFIG_P2P_POWER_SAVE /* by Shingu 20180208 (P2P PS opt.) */
	da16x_eloop_cancel_timeout(p2p_ps_resume, wpa_s, NULL);
#endif	/* CONFIG_P2P_POWER_SAVE */
	p2p_group_deinit(wpa_s->p2p_group);
	wpa_s->p2p_group = NULL;

#ifdef	CONFIG_AP
	wpa_s->ap_configured_cb = NULL;
	wpa_s->ap_configured_cb_ctx = NULL;
	wpa_s->ap_configured_cb_data = NULL;
#endif	/* CONFIG_AP */
	wpa_s->connect_without_scan = NULL;
}


#ifdef	CONFIG_P2P_OPTION
int wpas_p2p_reject(struct wpa_supplicant *wpa_s, const u8 *addr)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_reject(wpa_s->global->p2p, addr);
}

/* Invite to reinvoke a persistent group */
int wpas_p2p_invite(struct wpa_supplicant *wpa_s, const u8 *peer_addr,
		    struct wpa_ssid *ssid, const u8 *go_dev_addr, int freq,
		    int ht40, int vht, int pref_freq)
{
	enum p2p_invite_role role;
	u8 *bssid = NULL;
	int force_freq = 0;
	int no_pref_freq_given = pref_freq == 0;

	wpa_s->global->p2p_invite_group = NULL;
	if (peer_addr)
		os_memcpy(wpa_s->p2p_auth_invite, peer_addr, ETH_ALEN);
	else
		os_memset(wpa_s->p2p_auth_invite, 0, ETH_ALEN);

	wpa_s->p2p_persistent_go_freq = freq;

	if (ssid->mode == WPAS_MODE_P2P_GO) {
		role = P2P_INVITE_ROLE_GO;
		if (peer_addr == NULL) {
			da16x_p2p_prt("[%s] P2P: Missing peer "
				   "address in invitation command\n",
				   __func__);

			return -1;
		}
		if (wpas_p2p_create_iface(wpa_s)) {
			if (wpas_p2p_add_group_interface(wpa_s,
							 WPA_IF_P2P_GO) < 0) {
				da16x_p2p_prt("[%s] P2P: Failed to "
					   "allocate a new interface for the "
					   "group\n", __func__);
				return -1;
			}
			bssid = wpa_s->pending_interface_addr;
		} else {
			bssid = wpa_s->own_addr;
		}
	} else {
		role = P2P_INVITE_ROLE_CLIENT;
		peer_addr = ssid->bssid;
	}
	wpa_s->pending_invite_ssid_id = ssid->id;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (wpa_s->parent->conf->p2p_ignore_shared_freq &&
	    no_pref_freq_given && pref_freq > 0 &&
	    wpa_s->num_multichan_concurrent > 1 &&
	    wpas_p2p_num_unused_channels(wpa_s) > 0) {

		da16x_p2p_prt("[%s] P2P: Ignore own channel preference %d MHz "
			"for invitation due to p2p_ignore_shared_freq=1 "
			"configuration\n", __func__, pref_freq);

		pref_freq = 0;
	}

	return p2p_invite(wpa_s->global->p2p, peer_addr, role, bssid,
			  ssid->ssid, ssid->ssid_len, force_freq, go_dev_addr,
			  1, pref_freq, -1);
}


/* Invite to join an active group */
int wpas_p2p_invite_group(struct wpa_supplicant *wpa_s, const char *ifname,
			  const u8 *peer_addr, const u8 *go_dev_addr)
{
	struct wpa_global *global = wpa_s->global;
	enum p2p_invite_role role;
	u8 *bssid = NULL;
	struct wpa_ssid *ssid;
	int persistent;
	int force_freq = 0, pref_freq = 0;

	wpa_s->p2p_persistent_go_freq = 0;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (os_strcmp(wpa_s->ifname, ifname) == 0)
			break;
	}
	if (wpa_s == NULL) {
		da16x_p2p_prt("[%s] P2P: Interface '%s' not found\n",
				__func__, ifname);

		return -1;
	}

	ssid = wpa_s->current_ssid;
	if (ssid == NULL) {
		da16x_p2p_prt("[%s] P2P: No current SSID to use for "
			"invitation\n", __func__);

		return -1;
	}

	wpa_s->global->p2p_invite_group = wpa_s;
	persistent = ssid->p2p_persistent_group &&
		wpas_p2p_get_persistent(wpa_s->parent, peer_addr,
					ssid->ssid, ssid->ssid_len);

	if (ssid->mode == WPAS_MODE_P2P_GO) {
		role = P2P_INVITE_ROLE_ACTIVE_GO;
		bssid = wpa_s->own_addr;
		if (go_dev_addr == NULL)
			go_dev_addr = wpa_s->global->p2p_dev_addr;
	} else {
		role = P2P_INVITE_ROLE_CLIENT;
		if (wpa_s->wpa_state < WPA_ASSOCIATED) {
			da16x_p2p_prt("[%s] P2P: Not associated - cannot "
				   "invite to current group\n", __func__);
			return -1;
		}
		bssid = wpa_s->bssid;
		if (go_dev_addr == NULL &&
		    !is_zero_ether_addr(wpa_s->go_dev_addr))
			go_dev_addr = wpa_s->go_dev_addr;
	}
	wpa_s->parent->pending_invite_ssid_id = -1;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	return p2p_invite(wpa_s->global->p2p, peer_addr, role, bssid,
			  ssid->ssid, ssid->ssid_len, force_freq,
			  go_dev_addr, persistent, pref_freq, -1);
}
#endif	/* CONFIG_P2P_OPTION */


void wpas_p2p_completed(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
#ifdef	ENABLE_P2P_DBG
	const char *ssid_txt;
#endif	/* ENABLE_P2P_DBG */
	u8 go_dev_addr[ETH_ALEN];
	int network_id = -1;
#ifdef	ENABLE_P2P_DBG
	int freq;
#endif	/* ENABLE_P2P_DBG */
#ifdef	CONFIG_P2P_OPTION
	int persistent;
	u8 ip[3 * 4];
	char ip_addr[100];
#endif	/* CONFIG_P2P_OPTION */

	if (ssid == NULL || ssid->mode != WPAS_MODE_P2P_GROUP_FORMATION) {
		da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
					  wpa_s, NULL);
	}

#if 0 /* munchang.jung_20160225 */
	da16x_eloop_register_timeout(1, 0, wpas_p2p_net_init, wpa_s, NULL);
#endif /* 0 */

	if (!wpa_s->show_group_started || !ssid)
		return;

	wpa_s->show_group_started = 0;

#ifdef	ENABLE_P2P_DBG
	ssid_txt = wpa_ssid_txt(ssid->ssid, ssid->ssid_len);
#endif	/* ENABLE_P2P_DBG */
	os_memset(go_dev_addr, 0, ETH_ALEN);
	if (ssid->bssid_set)
		os_memcpy(go_dev_addr, ssid->bssid, ETH_ALEN);
#ifdef	CONFIG_P2P_OPTION
	persistent = wpas_p2p_persistent_group(wpa_s, go_dev_addr, ssid->ssid,
					       ssid->ssid_len);
#endif	/* CONFIG_P2P_OPTION */
	os_memcpy(wpa_s->go_dev_addr, go_dev_addr, ETH_ALEN);

	if (wpa_s->global->p2p_group_formation == wpa_s)
		wpa_s->global->p2p_group_formation = NULL;

#ifdef	ENABLE_P2P_DBG
	freq = wpa_s->current_bss ? wpa_s->current_bss->freq :
		(int) wpa_s->assoc_freq;
#endif	/* ENABLE_P2P_DBG */

#ifdef	CONFIG_P2P_OPTION
	ip_addr[0] = '\0';
	if (wpa_sm_get_p2p_ip_addr(wpa_s->wpa, ip) == 0) {
		os_snprintf(ip_addr, sizeof(ip_addr), " ip_addr=%u.%u.%u.%u "
			    "ip_mask=%u.%u.%u.%u go_ip_addr=%u.%u.%u.%u",
			    ip[0], ip[1], ip[2], ip[3],
			    ip[4], ip[5], ip[6], ip[7],
			    ip[8], ip[9], ip[10], ip[11]);
	}

	if (ssid->passphrase == NULL && ssid->psk_set) {
		char psk[65];

		wpa_snprintf_hex(psk, sizeof(psk), ssid->psk, 32);
		da16x_p2p_prt("[%s] " P2P_EVENT_GROUP_STARTED
			     "%s client ssid=\"%s\" freq=%d psk=%s "
			     "go_dev_addr=" MACSTR "%s%s\n", __func__,
			     wpa_s->ifname, ssid_txt, freq, psk,
			     MAC2STR(go_dev_addr),
			     persistent ? " [PERSISTENT]" : "", ip_addr);
	} else {
		da16x_p2p_prt("[%s] P2P_EVENT_GROUP_STARTED"
			     "%s client ssid=\"%s\" freq=%d "
			     "passphrase=\"%s\" go_dev_addr=" MACSTR "%s%s\n",
			     __func__, wpa_s->ifname, ssid_txt, freq,
			     ssid->passphrase ? ssid->passphrase : "",
			     MAC2STR(go_dev_addr),
			     persistent ? " [PERSISTENT]" : "", ip_addr);
	}

	if (persistent)
		network_id = wpas_p2p_store_persistent_group(wpa_s->parent,
							     ssid, go_dev_addr);
#endif	/* CONFIG_P2P_OPTION */

	if (network_id < 0)
		network_id = ssid->id;

#if 0	/* by Bright */
	wpas_notify_p2p_group_started(wpa_s, ssid, network_id, 1);
#endif	/* 0 */
}


int wpas_p2p_presence_req(struct wpa_supplicant *wpa_s, u32 duration1,
			  u32 interval1, u32 duration2, u32 interval2)
{
	int ret;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return -1;

	if (wpa_s->wpa_state < WPA_ASSOCIATED ||
	    wpa_s->current_ssid == NULL ||
	    wpa_s->current_ssid->mode != WPAS_MODE_INFRA)
		return -1;

	ret = p2p_presence_req(wpa_s->global->p2p, wpa_s->bssid,
			       wpa_s->own_addr, wpa_s->assoc_freq,
			       duration1, interval1, duration2, interval2);
	if (ret == 0)
		wpa_s->waiting_presence_resp = 1;

	return ret;
}



static int wpas_p2p_is_client(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL) {
		/*
		 * current_ssid can be cleared when P2P client interface gets
		 * disconnected, so assume this interface was used as P2P
		 * client.
		 */
		return 1;
	}
	return wpa_s->current_ssid->p2p_group &&
		wpa_s->current_ssid->mode == WPAS_MODE_INFRA;
}

static void wpas_p2p_group_idle_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (wpa_s->conf->p2p_group_idle == 0 && !wpas_p2p_is_client(wpa_s)) {
		da16x_p2p_prt("[%s] P2P: Ignore group idle timeout - disabled\n",
				__func__);
		return;
	}

	da16x_p2p_prt("[%s] P2P: Group idle timeout reached - terminate "
		   "group\n", __func__);

	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_IDLE_TIMEOUT);
}

static void wpas_p2p_set_group_idle_timeout(struct wpa_supplicant *wpa_s)
{
	int timeout;

	if (da16x_eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s,
				      NULL) > 0)
		da16x_p2p_prt("[%s] P2P: Cancelled P2P group idle timeout\n",
				__func__);

	if (wpa_s->current_ssid == NULL || !wpa_s->current_ssid->p2p_group)
		return;

	timeout = wpa_s->conf->p2p_group_idle;
	if (wpa_s->current_ssid->mode == WPAS_MODE_INFRA &&
	    (timeout == 0 || timeout > P2P_MAX_CLIENT_IDLE))
	    timeout = P2P_MAX_CLIENT_IDLE;

	if (timeout == 0)
		return;

	if (timeout < 0) {
		if (wpa_s->current_ssid->mode == WPAS_MODE_INFRA)
			timeout = 0; /* special client mode no-timeout */
		else
			return;
	}

	if (wpa_s->p2p_in_provisioning) {
		/*
		 * Use the normal group formation timeout during the
		 * provisioning phase to avoid terminating this process too
		 * early due to group idle timeout.
		 */
		da16x_p2p_prt("[%s] P2P: Do not use P2P group idle timeout "
			   "during provisioning\n", __func__);
		return;
	}

	if (wpa_s->show_group_started) {
		/*
		 * Use the normal group formation timeout between the end of
		 * the provisioning phase and completion of 4-way handshake to
		 * avoid terminating this process too early due to group idle
		 * timeout.
		 */
		da16x_p2p_prt("[%s] P2P: Do not use P2P group idle timeout "
			   "while waiting for initial 4-way handshake to "
			   "complete\n", __func__);
		return;
	}

	da16x_p2p_prt("[%s] P2P: Set P2P group idle timeout to %u seconds\n",
		   	__func__, timeout);

	da16x_eloop_register_timeout(timeout, 0, wpas_p2p_group_idle_timeout,
			       wpa_s, NULL);

}


/* Returns 1 if the interface was removed */
int wpas_p2p_deauth_notif(struct wpa_supplicant *wpa_s, const u8 *bssid,
			  u16 reason_code, const u8 *ie, size_t ie_len,
			  int locally_generated)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;

	if (!locally_generated)
		p2p_deauth_notif(wpa_s->global->p2p, bssid, reason_code, ie,
				 ie_len);

	if (reason_code == WLAN_REASON_DEAUTH_LEAVING && !locally_generated &&
	    wpa_s->current_ssid &&
	    wpa_s->current_ssid->p2p_group &&
	    wpa_s->current_ssid->mode == WPAS_MODE_INFRA) {
		da16x_p2p_prt("[%s] P2P: GO indicated that the P2P Group "
			   "session is ending\n", __func__);

		if (wpas_p2p_group_delete(wpa_s,
					  P2P_GROUP_REMOVAL_GO_ENDING_SESSION)
		    > 0)
			return 1;
	} else {
		wpa_drv_deinit_p2p_cli(wpa_s);
		wpas_p2p_group_deinit(wpa_s);

#if 1 /* by Shingu 20200518 */
	if (reason_code == WLAN_REASON_DEAUTH_LEAVING && locally_generated)
			return 1;
#endif
	}

	return 0;
}


void wpas_p2p_disassoc_notif(struct wpa_supplicant *wpa_s, const u8 *bssid,
			     u16 reason_code, const u8 *ie, size_t ie_len,
			     int locally_generated)
{
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return;

	if (!locally_generated)
		p2p_disassoc_notif(wpa_s->global->p2p, bssid, reason_code, ie,
				   ie_len);
}


#ifdef	CONFIG_P2P
void wpas_p2p_update_config(struct wpa_supplicant *wpa_s)
{
	struct p2p_data *p2p = wpa_s->global->p2p;

	if (p2p == NULL)
		return;

	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_P2P_CAPABLE))
		return;

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_DEVICE_NAME)
		p2p_set_dev_name(p2p, wpa_s->conf->device_name);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_DEVICE_TYPE)
		p2p_set_pri_dev_type(p2p, wpa_s->conf->device_type);

#ifdef	CONFIG_WPS
	if (wpa_s->wps &&
	    (wpa_s->conf->changed_parameters & CFG_CHANGED_CONFIG_METHODS))
		p2p_set_config_methods(p2p, wpa_s->wps->config_methods);

	if (wpa_s->wps && (wpa_s->conf->changed_parameters & CFG_CHANGED_UUID))
		p2p_set_uuid(p2p, wpa_s->wps->uuid);
#endif	/* CONFIG_WPS */

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_WPS_STRING) {
		p2p_set_manufacturer(p2p, wpa_s->conf->manufacturer);
		p2p_set_model_name(p2p, wpa_s->conf->model_name);
		p2p_set_model_number(p2p, wpa_s->conf->model_number);
		p2p_set_serial_number(p2p, wpa_s->conf->serial_number);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_SEC_DEVICE_TYPE)
		p2p_set_sec_dev_types(p2p,
				      (void *) wpa_s->conf->sec_device_type,
				      wpa_s->conf->num_sec_device_types);

#ifdef	CONFIG_WPS
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_VENDOR_EXTENSION) {
		int i;

		p2p_remove_wps_vendor_extensions(p2p);
		for (i = 0; i < MAX_WPS_VENDOR_EXT; i++) {
			if (wpa_s->conf->wps_vendor_ext[i] == NULL)
				continue;
			p2p_add_wps_vendor_extension(
				p2p, wpa_s->conf->wps_vendor_ext[i]);
		}
	}
#endif	/* CONFIG_WPS */

	if ((wpa_s->conf->changed_parameters & CFG_CHANGED_COUNTRY) &&
	    wpa_s->conf->country[0] && wpa_s->conf->country[1]) {
		char country[3];
		country[0] = wpa_s->conf->country[0];
		country[1] = wpa_s->conf->country[1];
		country[2] = 0x04;
		p2p_set_country(p2p, country);
	}

#ifdef	CONFIG_P2P_UNUSED_CMD
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_SSID_POSTFIX) {
		p2p_set_ssid_postfix(p2p, (u8 *) wpa_s->conf->p2p_ssid_postfix,
				     wpa_s->conf->p2p_ssid_postfix ?
				     os_strlen(wpa_s->conf->p2p_ssid_postfix) :
				     0);
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_INTRA_BSS)
		p2p_set_intra_bss_dist(p2p, wpa_s->conf->p2p_intra_bss);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_LISTEN_CHANNEL) {
		u8 reg_class, channel;
		int ret;
		unsigned int r;
		if (wpa_s->conf->p2p_listen_reg_class &&
		    wpa_s->conf->p2p_listen_channel) {
			reg_class = wpa_s->conf->p2p_listen_reg_class;
			channel = wpa_s->conf->p2p_listen_channel;
		} else {
			reg_class = 81;
			/*
			 * Pick one of the social channels randomly as the
			 * listen channel.
			 */
			os_get_random((u8 *) &r, sizeof(r));
			channel = 1 + (r % 3) * 5;
		}
		ret = p2p_set_listen_channel(p2p, reg_class, channel);
		if (ret) {
			da16x_p2p_prt("[%s] P2P: Own listen channel update "
				   "failed: %d\n", __func__, ret);
		}
	}
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_OPER_CHANNEL) {
		u8 op_reg_class, op_channel, cfg_op_channel;
		int ret = 0;
		unsigned int r;

		if (wpa_s->conf->p2p_oper_reg_class &&
		    wpa_s->conf->p2p_oper_channel) {
			op_reg_class = wpa_s->conf->p2p_oper_reg_class;
			op_channel = wpa_s->conf->p2p_oper_channel;
			cfg_op_channel = 1;
		} else {
			op_reg_class = 81;
			/*
			 * Use random operation channel from (1, 6, 11)
			 *if no other preference is indicated.
			 */
			os_get_random((u8 *) &r, sizeof(r));
			op_channel = 1 + (r % 3) * 5;
			cfg_op_channel = 0;
		}
		ret = p2p_set_oper_channel(p2p, op_reg_class, op_channel,
					   cfg_op_channel);
		if (ret) {
			da16x_p2p_prt("[%s] P2P: Own oper channel update "
				   "failed: %d\n", __func__, ret);
		}
	}

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_P2P_PREF_CHAN) {
		if (p2p_set_pref_chan(p2p, wpa_s->conf->num_p2p_pref_chan,
				      wpa_s->conf->p2p_pref_chan) < 0) {
			da16x_p2p_prt("[%s] P2P: Preferred channel list "
				   "update failed\n", __func__);
		}

		if (p2p_set_no_go_freq(p2p, &wpa_s->conf->p2p_no_go_freq) < 0) {
			da16x_p2p_prt("[%s] P2P: No GO channel list "
				   "update failed\n", __func__);
		}
	}
#endif	/* CONFIG_P2P_UNUSED_CMD */
}
#endif	/* CONFIG_P2P */

#ifdef	CONFIG_P2P_POWER_SAVE
int wpas_p2p_set_opp_ps(struct wpa_supplicant *wpa_s, int ctwindow)
{
#ifdef	CONFIG_AP
	if (!wpa_s->ap_iface)
		return -1;

	wpa_drv_set_p2p_powersave(wpa_s, 0, -1, -1, -1, -1, ctwindow);

	return hostapd_p2p_set_opp_ps(wpa_s->ap_iface->bss[0], ctwindow);
#else
da16x_p2p_prt("[%s] What do I do for this one ???\n", __func__);
	return 0;
#endif	/* CONFIG_AP */
}


int wpas_p2p_set_noa(struct wpa_supplicant *wpa_s, u8 count, int duration,
		     int interval, int start)
{
#ifdef	CONFIG_AP
	if (!wpa_s->ap_iface)
		return -1;

	wpa_drv_set_p2p_powersave(wpa_s, 1, count, duration, interval,
				  start, -1);

	return hostapd_p2p_set_noa(wpa_s->ap_iface->bss[0], count, duration,
				   interval, start);
#else
da16x_p2p_prt("[%s] What do I do for this one ???\n", __func__);
	return 0;
#endif	/* CONFIG_AP */
}
#endif	/* CONFIG_P2P_POWER_SAVE */

void wpas_p2p_notif_connected(struct wpa_supplicant *wpa_s)
{
	if (
#ifdef	CONFIG_AP
		!wpa_s->ap_iface &&
#endif	/* CONFIG_AP */
		da16x_eloop_cancel_timeout(wpas_p2p_group_idle_timeout, wpa_s,
					  NULL) > 0)
		da16x_p2p_prt("[%s] P2P: Cancelled P2P group idle timeout\n",
			     __func__);
}


void wpas_p2p_notif_disconnected(struct wpa_supplicant *wpa_s)
{
	if (
#ifdef	CONFIG_AP
		!wpa_s->ap_iface &&
#endif	/* CONFIG_AP */
	    !da16x_eloop_is_timeout_registered(wpas_p2p_group_idle_timeout,
					 wpa_s, NULL))
		wpas_p2p_set_group_idle_timeout(wpa_s);
}

int wpas_p2p_notif_pbc_overlap(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->p2p_group_interface != P2P_GROUP_INTERFACE_CLIENT &&
	    !wpa_s->p2p_in_provisioning)
		return 0; /* not P2P client operation */

	da16x_p2p_prt("[%s] P2P: Terminate connection due to WPS PBC "
		   "session overlap\n", __func__);

	if (wpa_s != wpa_s->parent) {
		da16x_p2p_prt("[%s] " WPS_EVENT_OVERLAP "\n", __func__);
	}

	wpas_p2p_group_formation_failed(wpa_s);
	return 1;
}

extern UINT is_supplicant_done(void );
void wpas_p2p_group_add_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	
	if (is_supplicant_done()) {
		wpas_p2p_group_add(wpa_s, 0, 0, 0, 0);
	} else {
		da16x_notice_prt("[%s] supplicant not initialized.\n", __func__);
	}
}

void wpas_p2p_find_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	extern int p2p_ctrl_find(struct wpa_supplicant *wpa_s);

	if (is_supplicant_done()) {
		p2p_ctrl_find(wpa_s);
	} else {
		da16x_notice_prt("[%s] supplicant not initialized.\n", __func__);
	}
}


void wpas_p2p_pbc_overlap_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_notif_pbc_overlap(wpa_s);
}

#ifdef CONFIG_5G_SUPPORT
#if 0 //[[ trinity_0171221_BEGIN -- temp
int wpas_p2p_get_ht40_mode(struct wpa_supplicant *wpa_s,
			   struct hostapd_hw_modes *mode, u8 channel)
{
	int op;
	enum chan_allowed ret;

	for (op = 0; global_op_class[op].op_class; op++) {
		const struct oper_class_map *o = &global_op_class[op];
		u8 ch;

		if (o->p2p == NO_P2P_SUPP)
			continue;

		for (ch = o->min_chan; ch <= o->max_chan; ch += o->inc) {
			if (o->mode != HOSTAPD_MODE_IEEE80211A ||
			    (o->bw != BW40PLUS && o->bw != BW40MINUS) ||
			    ch != channel)
				continue;
			ret = wpas_p2p_verify_channel(wpa_s, mode, ch, o->bw);
			if (ret == ALLOWED)
				return (o->bw == BW40MINUS) ? -1 : 1;
		}
	}
	return 0;
}
#endif //]] trinity_0171221_END

/*
 * Check if the given frequency is one of the possible operating frequencies
 * set after the completion of the GO Negotiation.
 */
static int wpas_p2p_go_is_peer_freq(struct wpa_supplicant *wpa_s, int freq)
{
	unsigned int i;

	//p2p_go_dump_common_freqs(wpa_s);

	/* assume no restrictions */
	if (!wpa_s->p2p_group_common_freqs_num)
		return 1;

	for (i = 0; i < wpa_s->p2p_group_common_freqs_num; i++) {
		if (wpa_s->p2p_group_common_freqs[i] == freq)
			return 1;
	}
	return 0;
}

/* Check if all the peers support eCSA */
static int wpas_sta_check_ecsa(struct hostapd_data *hapd,
			       struct sta_info *sta, void *ctx)
{
	int *ecsa_support = ctx;

	*ecsa_support &= sta->ecsa_supported;

	return 0;
}

static int wpas_p2p_go_clients_support_ecsa(struct wpa_supplicant *wpa_s)
{
	int ecsa_support = 1;

	ap_for_each_sta(wpa_s->ap_iface->bss[0], wpas_sta_check_ecsa,
			&ecsa_support);

	return ecsa_support;
}

static void wpas_p2p_optimize_listen_channel(struct wpa_supplicant *wpa_s,
					     struct wpa_used_freq_data *freqs,
					     unsigned int num)
{
	u8 curr_chan, cand, chan;
	unsigned int i;

	/*
	 * If possible, optimize the Listen channel to be a channel that is
	 * already used by one of the other interfaces.
	 */
	if (!wpa_s->conf->p2p_optimize_listen_chan)
		return;

	if (!wpa_s->current_ssid || wpa_s->wpa_state != WPA_COMPLETED)
		return;

	curr_chan = p2p_get_listen_channel(wpa_s->global->p2p);
	for (i = 0, cand = 0; i < num; i++) {
		ieee80211_freq_to_chan(freqs[i].freq, &chan);
		if (curr_chan == chan) {
			cand = 0;
			break;
		}

		if (chan == 1 || chan == 6 || chan == 11)
			cand = chan;
	}

	if (cand) {
		da16x_p2p_prt("[%s] "
			"P2P: Update Listen channel to %u based on operating channel\n",
			__func__, cand);
		p2p_set_listen_channel(wpa_s->global->p2p, 81, cand);
	}
}

static int wpas_p2p_move_go_csa(struct wpa_supplicant *wpa_s)
{
	struct hostapd_config *conf;
	struct p2p_go_neg_results params;
	struct csa_settings csa_settings;
	struct wpa_ssid *current_ssid = wpa_s->current_ssid;
	int old_freq = current_ssid->frequency;
	int ret;

	if (!(wpa_s->drv_flags & WPA_DRIVER_FLAGS_AP_CSA)) {
		da16x_p2p_prt("[%s] CSA is not enabled\n", __func__);
		return -1;
	}

	/*
	 * TODO: This function may not always work correctly. For example,
	 * when we have a running GO and a BSS on a DFS channel.
	 */
	if (wpas_p2p_init_go_params(wpa_s, &params, 0, 0, 0, NULL)) {
		da16x_p2p_prt("[%s] "
			"P2P CSA: Failed to select new frequency for GO\n", __func__);
		return -1;
	}

	if (current_ssid->frequency == params.freq) {
		da16x_p2p_prt("[%s] "
			"P2P CSA: Selected same frequency - not moving GO\n", __func__);
		return 0;
	}

	conf = hostapd_config_defaults();
	if (!conf) {
		da16x_p2p_prt("[%s] "
			"P2P CSA: Failed to allocate default config\n", __func__);
		return -1;
	}

	current_ssid->frequency = params.freq;
	if (wpa_supplicant_conf_ap_ht(wpa_s, current_ssid, conf)) {
		da16x_p2p_prt("[%s] "
			"P2P CSA: Failed to create new GO config\n", __func__);
		ret = -1;
		goto out;
	}

	if (conf->hw_mode != wpa_s->ap_iface->current_mode->mode) {
		da16x_p2p_prt("[%s] "
			"P2P CSA: CSA to a different band is not supported\n", __func__);
		ret = -1;
		goto out;
	}

	os_memset(&csa_settings, 0, sizeof(csa_settings));
	csa_settings.cs_count = P2P_GO_CSA_COUNT;
	csa_settings.block_tx = P2P_GO_CSA_BLOCK_TX;
	csa_settings.freq_params.freq = params.freq;
	csa_settings.freq_params.sec_channel_offset = conf->secondary_channel;
	csa_settings.freq_params.ht_enabled = conf->ieee80211n;
	csa_settings.freq_params.bandwidth = conf->secondary_channel ? 40 : 20;

#ifdef CONFIG_IEEE80211AC
	if (conf->ieee80211ac) {
		int freq1 = 0, freq2 = 0;
		u8 chan, opclass;

		if (ieee80211_freq_to_channel_ext(params.freq,
						  conf->secondary_channel,
						  conf->vht_oper_chwidth,
						  &opclass, &chan) ==
		    NUM_HOSTAPD_MODES) {
			da16x_p2p_prt("[%s] P2P CSA: Bad freq\n", __func__);
			ret = -1;
			goto out;
		}

		if (conf->vht_oper_centr_freq_seg0_idx)
			freq1 = ieee80211_chan_to_freq(
				NULL, opclass,
				conf->vht_oper_centr_freq_seg0_idx);

		if (conf->vht_oper_centr_freq_seg1_idx)
			freq2 = ieee80211_chan_to_freq(
				NULL, opclass,
				conf->vht_oper_centr_freq_seg1_idx);

		if (freq1 < 0 || freq2 < 0) {
			da16x_p2p_prt("[%s] "
				"P2P CSA: Selected invalid VHT center freqs\n", __func__);
			ret = -1;
			goto out;
		}

		csa_settings.freq_params.vht_enabled = conf->ieee80211ac;
		csa_settings.freq_params.cent_freq1 = freq1;
		csa_settings.freq_params.cent_freq2 = freq2;

		switch (conf->vht_oper_chwidth) {
		case VHT_CHANWIDTH_80MHZ:
		case VHT_CHANWIDTH_80P80MHZ:
			csa_settings.freq_params.bandwidth = 80;
			break;
		case VHT_CHANWIDTH_160MHZ:
			csa_settings.freq_params.bandwidth = 160;
			break;
		}
	}
#endif /* CONFIG_IEEE80211AC */
	ret = ap_switch_channel(wpa_s, &csa_settings);
out:
	current_ssid->frequency = old_freq;
	hostapd_config_free(conf);
	return ret;
}


static void wpas_p2p_move_go_no_csa(struct wpa_supplicant *wpa_s)
{
	struct p2p_go_neg_results params;
	struct wpa_ssid *current_ssid = wpa_s->current_ssid;

	da16x_notice_prt(P2P_EVENT_REMOVE_AND_REFORM_GROUP "\n");

	da16x_p2p_prt("[%s] P2P: Move GO from freq=%d MHz\n",
			__func__, current_ssid->frequency);

	/* Stop the AP functionality */
	/* TODO: Should do this in a way that does not indicated to possible
	 * P2P Clients in the group that the group is terminated. */
	wpa_supplicant_ap_deinit(wpa_s);

	/* Reselect the GO frequency */
	if (wpas_p2p_init_go_params(wpa_s, &params, 0, 0, 0, NULL)) {
		da16x_p2p_prt("[%s] P2P: Failed to reselect freq\n", __func__);
		wpas_p2p_group_delete(wpa_s,
				      P2P_GROUP_REMOVAL_GO_LEAVE_CHANNEL);
		return;
	}
	da16x_p2p_prt("[%s] P2P: New freq selected for the GO (%u MHz)\n",
		__func__, params.freq);

	if (params.freq &&
	    !p2p_supported_freq_go(wpa_s->global->p2p, params.freq)) {
		da16x_p2p_prt("[%s] "
			   "P2P: Selected freq (%u MHz) is not valid for P2P\n",
			   __func__, params.freq);
		wpas_p2p_group_delete(wpa_s,
				      P2P_GROUP_REMOVAL_GO_LEAVE_CHANNEL);
		return;
	}

	/* Update the frequency */
	current_ssid->frequency = params.freq;
	wpa_s->connect_without_scan = current_ssid;
	wpa_s->reassociate = 1;
	wpa_s->disconnected = 0;
	wpa_supplicant_req_scan(wpa_s, 0, 0);
}

static void wpas_p2p_move_go(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (!wpa_s->ap_iface || !wpa_s->current_ssid)
		return;

	wpas_p2p_go_update_common_freqs(wpa_s);

#if 0 //[[ trinity_0171220_BEGIN -- test
	/* Do not move GO in the middle of a CSA */
	if (hostapd_csa_in_progress(wpa_s->ap_iface)) {
		wpa_printf_dbg(MSG_DEBUG,
			   "P2P: CSA is in progress - not moving GO");
		return;
	}
#endif //]] trinity_0171220_END

	/*
	 * First, try a channel switch flow. If it is not supported or fails,
	 * take down the GO and bring it up again.
	 */
	if (wpas_p2p_move_go_csa(wpa_s) < 0)
		wpas_p2p_move_go_no_csa(wpa_s);
}

static void wpas_p2p_reconsider_moving_go(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	struct wpa_used_freq_data *freqs = NULL;
	unsigned int num = wpa_s->num_multichan_concurrent;

	freqs = os_calloc(num, sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return;

	num = get_shared_radio_freqs_data(wpa_s, freqs, num);

	/* Previous attempt to move a GO was not possible -- try again. */
	wpas_p2p_consider_moving_gos(wpa_s, freqs, num,
				     WPAS_P2P_CHANNEL_UPDATE_ANY);

	os_free(freqs);
}

/*
 * Consider moving a GO from its currently used frequency:
 * 1. It is possible that due to regulatory consideration the frequency
 *    can no longer be used and there is a need to evacuate the GO.
 * 2. It is possible that due to MCC considerations, it would be preferable
 *    to move the GO to a channel that is currently used by some other
 *    station interface.
 *
 * In case a frequency that became invalid is once again valid, cancel a
 * previously initiated GO frequency change.
 */
static void wpas_p2p_consider_moving_one_go(struct wpa_supplicant *wpa_s,
					    struct wpa_used_freq_data *freqs,
					    unsigned int num)
{
	unsigned int i, invalid_freq = 0, policy_move = 0, flags = 0;
	unsigned int timeout;
	int freq;

	wpas_p2p_go_update_common_freqs(wpa_s);

	freq = wpa_s->current_ssid->frequency;
	for (i = 0, invalid_freq = 0; i < num; i++) {
		if (freqs[i].freq == freq) {
			flags = freqs[i].flags;

			/* The channel is invalid, must change it */
			if (!p2p_supported_freq_go(wpa_s->global->p2p, freq)) {
				da16x_p2p_prt("[%s] "
					"P2P: Freq=%d MHz no longer valid for GO\n",
					__func__, freq);
				invalid_freq = 1;
			}
		} else if (freqs[i].flags == 0) {
			/* Freq is not used by any other station interface */
			continue;
		} else if (!p2p_supported_freq(wpa_s->global->p2p,
					       freqs[i].freq)) {
			/* Freq is not valid for P2P use cases */
			continue;
		} else if (wpa_s->conf->p2p_go_freq_change_policy ==
			   P2P_GO_FREQ_MOVE_SCM) {
			policy_move = 1;
		} else if (wpa_s->conf->p2p_go_freq_change_policy ==
			   P2P_GO_FREQ_MOVE_SCM_PEER_SUPPORTS &&
			   wpas_p2p_go_is_peer_freq(wpa_s, freqs[i].freq)) {
			policy_move = 1;
		} else if ((wpa_s->conf->p2p_go_freq_change_policy ==
			    P2P_GO_FREQ_MOVE_SCM_ECSA) &&
			   wpas_p2p_go_is_peer_freq(wpa_s, freqs[i].freq)) {
			if (!p2p_get_group_num_members(wpa_s->p2p_group)) {
				policy_move = 1;
			} else if ((wpa_s->drv_flags &
				    WPA_DRIVER_FLAGS_AP_CSA) &&
				   wpas_p2p_go_clients_support_ecsa(wpa_s)) {
				u8 chan;

				/*
				 * We do not support CSA between bands, so move
				 * GO only within the same band.
				 */
				if (wpa_s->ap_iface->current_mode->mode ==
				    ieee80211_freq_to_chan(freqs[i].freq,
							   &chan))
					policy_move = 1;
			}
		}
	}

	da16x_p2p_prt("[%s] "
		"P2P: GO move: invalid_freq=%u, policy_move=%u, flags=0x%X\n",
		__func__, invalid_freq, policy_move, flags);

	/*
	 * The channel is valid, or we are going to have a policy move, so
	 * cancel timeout.
	 */
	if (!invalid_freq || policy_move) {
		da16x_p2p_prt("[%s] "
			"P2P: Cancel a GO move from freq=%d MHz\n", __func__, freq);
		da16x_eloop_cancel_timeout(wpas_p2p_move_go, wpa_s, NULL);

		if (wpas_p2p_in_progress(wpa_s)) {
			da16x_p2p_prt("[%s] "
				"P2P: GO move: policy CS is not allowed - setting timeout to re-consider GO move\n", __func__);
			da16x_eloop_cancel_timeout(wpas_p2p_reconsider_moving_go,
					     wpa_s, NULL);
			da16x_eloop_register_timeout(P2P_RECONSIDER_GO_MOVE_DELAY, 0,
					       wpas_p2p_reconsider_moving_go,
					       wpa_s, NULL);
			return;
		}
	}

	if (!invalid_freq && (!policy_move || flags != 0)) {
		da16x_p2p_prt("[%s] "
			"P2P: Not initiating a GO frequency change\n", __func__);
		return;
	}

#if 0 //[[ trinity_0171220_BEGIN -- test
	/*
	 * Do not consider moving GO if it is in the middle of a CSA. When the
	 * CSA is finished this flow should be retriggered.
	 */
	if (hostapd_csa_in_progress(wpa_s->ap_iface)) {
		da16x_p2p_prt("[%s] "
			"P2P: Not initiating a GO frequency change - CSA is in progress\n", __func__);
		return;
	}
#endif //]] trinity_0171220_END

	if (invalid_freq && !wpas_p2p_disallowed_freq(wpa_s->global, freq))
		timeout = P2P_GO_FREQ_CHANGE_TIME;
	else
		timeout = 0;

	da16x_p2p_prt("[%s] P2P: Move GO from freq=%d MHz in %d secs\n",
		__func__, freq, timeout);
	da16x_eloop_cancel_timeout(wpas_p2p_move_go, wpa_s, NULL);
	da16x_eloop_register_timeout(timeout, 0, wpas_p2p_move_go, wpa_s, NULL);
}

static void wpas_p2p_consider_moving_gos(struct wpa_supplicant *wpa_s,
					 struct wpa_used_freq_data *freqs,
					 unsigned int num,
					 enum wpas_p2p_channel_update_trig trig)
{
	struct wpa_supplicant *ifs;

	da16x_eloop_cancel_timeout(wpas_p2p_reconsider_moving_go, ELOOP_ALL_CTX,
			     NULL);

	/*
	 * Travers all the radio interfaces, and for each GO interface, check
	 * if there is a need to move the GO from the frequency it is using,
	 * or in case the frequency is valid again, cancel the evacuation flow.
	 */
	for (ifs = wpa_s->global->ifaces; wpa_s ;
							wpa_s = wpa_s->next) {

		if (ifs->current_ssid == NULL ||
		    ifs->current_ssid->mode != WPAS_MODE_P2P_GO)
			continue;

		/*
		 * The GO was just started or completed channel switch, no need
		 * to move it.
		 */
		if (wpa_s == ifs &&
		    (trig == WPAS_P2P_CHANNEL_UPDATE_STATE_CHANGE ||
		     trig == WPAS_P2P_CHANNEL_UPDATE_CS)) {
			da16x_p2p_prt("[%s] "
				"P2P: GO move - schedule re-consideration\n", __func__);
			da16x_eloop_register_timeout(P2P_RECONSIDER_GO_MOVE_DELAY, 0,
					       wpas_p2p_reconsider_moving_go,
					       wpa_s, NULL);
			continue;
		}

		wpas_p2p_consider_moving_one_go(ifs, freqs, num);
	}
}

void wpas_p2p_update_channel_list(struct wpa_supplicant *wpa_s,
				  enum wpas_p2p_channel_update_trig trig)
{
	struct p2p_channels chan, cli_chan;
	struct wpa_used_freq_data *freqs = NULL;
	unsigned int num = wpa_s->num_multichan_concurrent;

	if (wpa_s->global == NULL || wpa_s->global->p2p == NULL)
		return;

	freqs = os_calloc(num, sizeof(struct wpa_used_freq_data));
	if (!freqs)
		return;

	num = get_shared_radio_freqs_data(wpa_s, freqs, num);

	os_memset(&chan, 0, sizeof(chan));
	os_memset(&cli_chan, 0, sizeof(cli_chan));
	if (wpas_p2p_setup_channels(wpa_s, &chan, &cli_chan)) {
		da16x_p2p_prt("[%s] P2P: Failed to update supported "
			   "channel list\n", __func__);
		return;
	}

	p2p_update_channel_list(wpa_s->global->p2p, &chan, &cli_chan);

	wpas_p2p_optimize_listen_channel(wpa_s, freqs, num);

	/*
	 * The used frequencies map changed, so it is possible that a GO is
	 * using a channel that is no longer valid for P2P use. It is also
	 * possible that due to policy consideration, it would be preferable to
	 * move it to a frequency already used by other station interfaces.
	 */
	wpas_p2p_consider_moving_gos(wpa_s, freqs, num, trig);

	os_free(freqs);
}
#endif /* CONFIG_5G_SUPPORT */

static void wpas_p2p_scan_res_ignore(struct wpa_supplicant *wpa_s,
				     struct wpa_scan_results *scan_res)
{
	da16x_p2p_prt("[%s] P2P: Ignore scan results\n", __func__);
}


int wpas_p2p_cancel(struct wpa_supplicant *wpa_s)
{
	struct wpa_global *global = wpa_s->global;
	int found = 0;
	const u8 *peer;

	if (global->p2p == NULL)
		return -1;

	da16x_p2p_prt("[%s] P2P: Request to cancel group formation\n", __func__);

	if (wpa_s->pending_interface_name[0] &&
	    !is_zero_ether_addr(wpa_s->pending_interface_addr))
		found = 1;

	peer = p2p_get_go_neg_peer(global->p2p);
	if (peer) {
		da16x_p2p_prt("[%s] P2P: Unauthorize pending GO Neg peer "
			   MACSTR "\n", __func__, MAC2STR(peer));

		found = 1;
	}

	if (wpa_s->scan_res_handler == wpas_p2p_scan_res_join) {
		da16x_p2p_prt("[%s] P2P: Stop pending scan for join\n",
				__func__);

		wpa_s->scan_res_handler = wpas_p2p_scan_res_ignore;
		found = 1;
	}

	if (wpa_s->pending_pd_before_join) {
		da16x_p2p_prt("[%s] P2P: Stop pending PD before join\n",
				__func__);

		wpa_s->pending_pd_before_join = 0;
		found = 1;
	}

	wpas_p2p_stop_find(wpa_s);

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		if (wpa_s == global->p2p_group_formation &&
		    (wpa_s->p2p_in_provisioning ||
		     wpa_s->parent->pending_interface_type ==
		     WPA_IF_P2P_CLIENT)) {
			da16x_p2p_prt("[%s] P2P: Interface %s in group "
				   "formation found - cancelling\n",
				   __func__, wpa_s->ifname);

			found = 1;

			da16x_eloop_cancel_timeout(
					wpas_p2p_group_formation_timeout,
					wpa_s, NULL);

			if (wpa_s->p2p_in_provisioning) {
				wpas_group_formation_completed(wpa_s, 0);
				break;
			}
			wpas_p2p_group_delete(wpa_s,
					      P2P_GROUP_REMOVAL_REQUESTED);
			break;
#ifdef	CONFIG_P2P_OPTION
		} else if (wpa_s->p2p_in_invitation) {
			da16x_p2p_prt("[%s] P2P: Interface %s in invitation "
					"found - cancelling\n",
					__func__, wpa_s->ifname);
			found = 1;
			wpas_p2p_group_formation_failed(wpa_s);
#endif	/* CONFIG_P2P_OPTION */
		}
	}

	if (!found) {
		da16x_p2p_prt("[%s] P2P: No ongoing group formation found\n",
				__func__);
		return -1;
	}

	return 0;
}


#ifdef	CONFIG_P2P_UNUSED_CMD
void wpas_p2p_interface_unavailable(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->current_ssid == NULL || !wpa_s->current_ssid->p2p_group)
		return;

	da16x_p2p_prt("[%s] P2P: Remove group due to driver resource not "
		   "being available anymore\n", __func__);

	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_UNAVAILABLE);
}
#endif	/* CONFIG_P2P_UNUSED_CMD */

/**
 * wpas_p2p_disconnect - Disconnect from a P2P Group
 * @wpa_s: Pointer to wpa_supplicant data
 * Returns: 0 on success, -1 on failure
 *
 * This can be used to disconnect from a group in which the local end is a P2P
 * Client or to end a P2P Group in case the local end is the Group Owner. If a
 * virtual network interface was created for this group, that interface will be
 * removed. Otherwise, only the configured P2P group network will be removed
 * from the interface.
 */
int wpas_p2p_disconnect(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->global->p2p)
		return -1;

	return wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_REQUESTED) < 0 ?
		-1 : 0;
}


int wpas_p2p_in_progress(struct wpa_supplicant *wpa_s)
{
	int ret;

	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL)
		return 0;

	ret = p2p_in_progress(wpa_s->global->p2p);
	if (ret == 0) {
		/*
		 * Check whether there is an ongoing WPS provisioning step (or
		 * other parts of group formation) on another interface since
		 * p2p_in_progress() does not report this to avoid issues for
		 * scans during such provisioning step.
		 */
		if (wpa_s->global->p2p_group_formation &&
		    wpa_s->global->p2p_group_formation != wpa_s) {
			da16x_p2p_prt("[%s] P2P: Another interface (%s) "
				"in group formation\n",
				__func__,
				wpa_s->global->p2p_group_formation->ifname);

			ret = 1;
		}
	}

	if (!ret && wpa_s->global->p2p_go_wait_client.sec) {
		struct os_reltime now;
		os_get_reltime(&now);
		if (os_reltime_expired(&now, &wpa_s->global->p2p_go_wait_client,
				       P2P_MAX_INITIAL_CONN_WAIT_GO)) {
			/* Wait for the first client has expired */
			wpa_s->global->p2p_go_wait_client.sec = 0;
		} else {
			da16x_p2p_prt("[%s] P2P: Waiting for initial client "
					"connection during group formation\n",
					__func__);
			ret = 1;
		}
	}

	return ret;
}


void wpas_p2p_network_removed(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid)
{
	if (wpa_s->p2p_in_provisioning && ssid->p2p_group &&
	    da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				      wpa_s, NULL) > 0) {
		/**
		 * Remove the network by scheduling the group formation
		 * timeout to happen immediately. The teardown code
		 * needs to be scheduled to run asynch later so that we
		 * don't delete data from under ourselves unexpectedly.
		 * Calling wpas_p2p_group_formation_timeout directly
		 * causes a series of crashes in WPS failure scenarios.
		 */
		da16x_p2p_prt("[%s] P2P: Canceled group formation due to "
			   "P2P group network getting removed\n",
				__func__);

		da16x_eloop_register_timeout(0, 0,
					    wpas_p2p_group_formation_timeout,
					    wpa_s, NULL);
	}
}


#ifdef	CONFIG_P2P_OPTION
struct wpa_ssid * wpas_p2p_get_persistent(struct wpa_supplicant *wpa_s,
					  const u8 *addr, const u8 *ssid,
					  size_t ssid_len)
{
	struct wpa_ssid *s;
	size_t i;

	for (s = wpa_s->conf->ssid; s; s = s->next) {
		if (s->disabled != 2)
			continue;
		if (ssid &&
		    (ssid_len != s->ssid_len ||
		     os_memcmp(ssid, s->ssid, ssid_len) != 0))
			continue;
		if (addr == NULL) {
			if (s->mode == WPAS_MODE_P2P_GO)
				return s;
			continue;
		}
		if (os_memcmp(s->bssid, addr, ETH_ALEN) == 0)
			return s; /* peer is GO in the persistent group */
		if (s->mode != WPAS_MODE_P2P_GO || s->p2p_client_list == NULL)
			continue;
		for (i = 0; i < s->num_p2p_clients; i++) {
			if (os_memcmp(s->p2p_client_list + i * ETH_ALEN,
				      addr, ETH_ALEN) == 0)
				return s; /* peer is P2P client in persistent
					   * group */
		}
	}

	return NULL;
}
#endif	/* CONFIG_P2P_OPTION */


void wpas_p2p_notify_ap_sta_authorized(struct wpa_supplicant *wpa_s,
                                       const u8 *addr)
{
	if (da16x_eloop_cancel_timeout(wpas_p2p_group_formation_timeout,
				      wpa_s, NULL) > 0) {
		da16x_p2p_prt("P2P: Canceled P2P group formation timeout on "
			     "data connection\n");
	}
	if (!wpa_s->p2p_go_group_formation_completed) {
		da16x_p2p_prt("P2P: Marking group formation completed on GO "
			     "on first data connection\n");
		wpa_s->p2p_go_group_formation_completed = 1;
		wpa_s->global->p2p_group_formation = NULL;
		wpa_s->p2p_in_provisioning = 0;
#ifdef	CONFIG_P2P_OPTION
		wpa_s->p2p_in_invitation = 0;
#endif	/* CONFIG_P2P_OPTION */
	}
	wpa_s->global->p2p_go_wait_client.sec = 0;
	if (addr == NULL)
		return;
#ifdef	CONFIG_P2P_OPTION
	wpas_p2p_add_persistent_group_client(wpa_s, addr);
#endif	/* CONFIG_P2P_OPTION */
}


static void wpas_p2p_fallback_to_go_neg(struct wpa_supplicant *wpa_s,
					int group_added)
{
	struct wpa_supplicant *group = wpa_s;
	if (wpa_s->global->p2p_group_formation)
		group = wpa_s->global->p2p_group_formation;
	wpa_s = wpa_s->parent;
	offchannel_send_action_done(wpa_s);
	if (group_added)
		wpas_p2p_group_delete(group, P2P_GROUP_REMOVAL_SILENT);

	da16x_p2p_prt("[%s] P2P: Fall back to GO Negotiation\n", __func__);

	wpas_p2p_connect(wpa_s, wpa_s->pending_join_dev_addr, wpa_s->p2p_pin,
			(enum p2p_wps_method)wpa_s->p2p_wps_method, 0,
			wpa_s->p2p_go_intent, wpa_s->p2p_connect_freq,
			wpa_s->p2p_pd_before_go_neg);
}

static int wpas_p2p_remove_psk_entry(struct wpa_supplicant *wpa_s,
				     struct wpa_ssid *s, const u8 *addr,
				     int iface_addr)
{
	struct psk_list_entry *psk, *tmp;
	int changed = 0;

	dl_list_for_each_safe(psk, tmp, &s->psk_list, struct psk_list_entry,
			      list) {
		if ((iface_addr && !psk->p2p &&
		     os_memcmp(addr, psk->addr, ETH_ALEN) == 0) ||
		    (!iface_addr && psk->p2p &&
		     os_memcmp(addr, psk->addr, ETH_ALEN) == 0)) {
			da16x_p2p_prt("[%s] P2P: Remove persistent group PSK "
					"list entry for " MACSTR " p2p=%u\n",
					__func__, MAC2STR(psk->addr), psk->p2p);

			dl_list_del(&psk->list);
			os_free(psk);
			changed++;
		}
	}

	return changed;
}


void wpas_p2p_new_psk_cb(struct wpa_supplicant *wpa_s, const u8 *mac_addr,
			 const u8 *p2p_dev_addr,
			 const u8 *psk, size_t psk_len)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;
#ifdef	CONFIG_P2P_OPTION
	struct wpa_ssid *persistent;
	struct psk_list_entry *last;
#endif	/* CONFIG_P2P_OPTION */
	struct psk_list_entry *p;

	if (psk_len != sizeof(p->psk))
		return;

	if (p2p_dev_addr) {
		da16x_p2p_prt("[%s] P2P: New PSK for addr=" MACSTR
			" p2p_dev_addr=" MACSTR "\n",
			__func__,
			MAC2STR(mac_addr), MAC2STR(p2p_dev_addr));
		if (is_zero_ether_addr(p2p_dev_addr))
			p2p_dev_addr = NULL;
	} else {
		da16x_p2p_prt("[%s] P2P: New PSK for addr=" MACSTR "\n",
				__func__, MAC2STR(mac_addr));
	}

	if (ssid->mode == WPAS_MODE_P2P_GROUP_FORMATION) {
		da16x_p2p_prt("[%s] P2P: new_psk_cb during group formation\n",
				__func__);

		/* To be added to persistent group once created */
		if (wpa_s->global->add_psk == NULL) {
			wpa_s->global->add_psk = os_zalloc(sizeof(*p));
			if (wpa_s->global->add_psk == NULL)
				return;
		}
		p = wpa_s->global->add_psk;
		if (p2p_dev_addr) {
			p->p2p = 1;
			os_memcpy(p->addr, p2p_dev_addr, ETH_ALEN);
		} else {
			p->p2p = 0;
			os_memcpy(p->addr, mac_addr, ETH_ALEN);
		}
		os_memcpy(p->psk, psk, psk_len);
		return;
	}

#ifdef	CONFIG_P2P_OPTION
	if (ssid->mode != WPAS_MODE_P2P_GO || !ssid->p2p_persistent_group) {
		da16x_p2p_prt("[%s] P2P: Ignore new_psk_cb on not-persistent "
				"GO\n", __func__);
		return;
	}
#else
	if (ssid->mode != WPAS_MODE_P2P_GO) {
		return;
	}
#endif	/* CONFIG_P2P_OPTION */

#ifdef	CONFIG_P2P_OPTION
	persistent = wpas_p2p_get_persistent(wpa_s->parent, NULL, ssid->ssid,
					     ssid->ssid_len);
	if (!persistent) {
		da16x_p2p_prt("[%s] P2P: Could not find persistent group "
				"information to store the new PSK\n",
				__func__);
		return;
	}
#endif	/* CONFIG_P2P_OPTION */

	p = os_zalloc(sizeof(*p));
	if (p == NULL)
		return;
	if (p2p_dev_addr) {
		p->p2p = 1;
		os_memcpy(p->addr, p2p_dev_addr, ETH_ALEN);
	} else {
		p->p2p = 0;
		os_memcpy(p->addr, mac_addr, ETH_ALEN);
	}
	os_memcpy(p->psk, psk, psk_len);

#ifdef	CONFIG_P2P_OPTION
	if (dl_list_len(&persistent->psk_list) > P2P_MAX_STORED_CLIENTS &&
	    (last = dl_list_last(&persistent->psk_list,
				 struct psk_list_entry, list))) {
		da16x_p2p_prt("[%s] P2P: Remove oldest PSK entry for "
			MACSTR " (p2p=%u) to make room for a new one\n",
			__func__, MAC2STR(last->addr), last->p2p);

		dl_list_del(&last->list);
		os_free(last);
	}
#endif	/* CONFIG_P2P_OPTION */

#ifdef	CONFIG_P2P_OPTION
	wpas_p2p_remove_psk_entry(wpa_s->parent, persistent,
				  p2p_dev_addr ? p2p_dev_addr : mac_addr,
				  p2p_dev_addr == NULL);
#else
	wpas_p2p_remove_psk_entry(wpa_s->parent, 0,
				  p2p_dev_addr ? p2p_dev_addr : mac_addr,
				  p2p_dev_addr == NULL);
#endif	/* CONFIG_P2P_OPTION */
	if (p2p_dev_addr) {
		da16x_p2p_prt("[%s] P2P: Add new PSK for p2p_dev_addr="
			MACSTR "\n", __func__, MAC2STR(p2p_dev_addr));
	} else {
		da16x_p2p_prt("[%s] P2P: Add new PSK for addr=" MACSTR "\n",
				__func__, MAC2STR(mac_addr));
	}
#ifdef	CONFIG_P2P_OPTION
	dl_list_add(&persistent->psk_list, &p->list);
#endif	/* CONFIG_P2P_OPTION */
}

static void wpas_p2p_psk_failure_removal(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	wpas_p2p_group_delete(wpa_s, P2P_GROUP_REMOVAL_PSK_FAILURE);
}

int wpas_p2p_4way_hs_failed(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid = wpa_s->current_ssid;

	if (ssid == NULL || !ssid->p2p_group)
		return 0;

	if (wpa_s->p2p_last_4way_hs_fail &&
	    wpa_s->p2p_last_4way_hs_fail == ssid) {
#ifdef	CONFIG_P2P_OPTION
		u8 go_dev_addr[ETH_ALEN];
		struct wpa_ssid *persistent;

		if (wpas_p2p_persistent_group(wpa_s, go_dev_addr,
					      ssid->ssid,
					      ssid->ssid_len) <= 0) {
			da16x_p2p_prt("[%s[ P2P: Could not determine whether "
					"4-way handshake failures were for "
					"a persistent group\n", __func__);
			goto disconnect;
		}

		da16x_p2p_prt("[%s] P2P: Two 4-way handshake failures for "
				"a P2P group - go_dev_addr=" MACSTR "\n",
				__func__, MAC2STR(go_dev_addr));

		persistent = wpas_p2p_get_persistent(wpa_s->parent, go_dev_addr,
						     ssid->ssid,
						     ssid->ssid_len);
		if (persistent == NULL || persistent->mode != WPAS_MODE_INFRA) {
			da16x_p2p_prt("[%s] P2P: No matching persistent "
					"group stored\n", __func__);

			goto disconnect;
		}
		da16x_p2p_prt("[%s] " P2P_EVENT_PERSISTENT_PSK_FAIL "%d\n",
				__func__, persistent->id);

disconnect:
#endif	/* CONFIG_P2P_OPTION */
		wpa_s->p2p_last_4way_hs_fail = NULL;

		/*
		 * Remove the group from a timeout to avoid issues with caller
		 * continuing to use the interface if this is on a P2P group
		 * interface.
		 */
		da16x_eloop_register_timeout(0, 0, wpas_p2p_psk_failure_removal,
				       wpa_s, NULL);

		return 1;
	}

	wpa_s->p2p_last_4way_hs_fail = ssid;
	return 0;
}

#endif	/* CONFIG_P2P */

/* EOF */
