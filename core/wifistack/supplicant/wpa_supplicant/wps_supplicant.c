/*
 * wpa_supplicant / WPS integration
 * Copyright (c) 2008-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_WPS

#include "supp_eloop.h"
#include "uuid.h"
#include "crypto/random.h"
#include "crypto/dh_group5.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/wpa_common.h"
#include "wpa_ctrl.h"
#include "eap_common/eap_wsc_common.h"
#include "eap_peer/eap.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "rsn_supp/wpa.h"
#include "wps/wps_attr_parse.h"
#include "supp_config.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "blacklist.h"
#include "bss.h"
#include "supp_scan.h"
#include "ap.h"
#include "p2p/supp_p2p.h"
#include "p2p_supplicant.h"
#include "wps_supplicant.h"
#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"

#pragma GCC diagnostic ignored "-Wswitch"

#ifndef WPS_PIN_SCAN_IGNORE_SEL_REG
#define WPS_PIN_SCAN_IGNORE_SEL_REG 3
#endif /* WPS_PIN_SCAN_IGNORE_SEL_REG */

extern int	wpa_supplicant_abort_scan(struct wpa_supplicant *wpa_s,
				struct wpa_driver_scan_params *params);
void wpa_supplicant_cancel_scan(struct wpa_supplicant *wpa_s);

static void wpas_wps_timeout(void *eloop_ctx, void *timeout_ctx);
static void wpas_clear_wps(struct wpa_supplicant *wpa_s);

extern int get_run_mode(void);

static u8 is_wps_in_use = 0;

static void wpas_wps_clear_ap_info(struct wpa_supplicant *wpa_s)
{
	os_free(wpa_s->wps_ap);
	wpa_s->wps_ap = NULL;
	wpa_s->num_wps_ap = 0;
	wpa_s->wps_ap_iter = 0;
}


static void wpas_wps_assoc_with_cred(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	int use_fast_assoc = timeout_ctx != NULL;
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */

	da16x_wps_prt("     [%s] WPS: Continuing association after eapol_cb\n",
		     __func__);

#ifndef CONFIG_SCAN_RESULT_OPTIMIZE
	if (!use_fast_assoc ||
	    wpa_supplicant_fast_associate(wpa_s) != 1)
#endif /* CONFIG_SCAN_RESULT_OPTIMIZE */
	{
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	}
}


static void wpas_wps_assoc_with_cred_cancel(struct wpa_supplicant *wpa_s)
{
	da16x_eloop_cancel_timeout(wpas_wps_assoc_with_cred, wpa_s, (void *) 0);
	da16x_eloop_cancel_timeout(wpas_wps_assoc_with_cred, wpa_s, (void *) 1);
}


int wpas_wps_eapol_cb(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_P2P
	if (wpas_p2p_wps_eapol_cb(wpa_s) > 0)
		return 1;
#endif /* CONFIG_P2P */

	if (!wpa_s->wps_success &&
	    wpa_s->current_ssid &&
	    eap_is_wps_pin_enrollee(&wpa_s->current_ssid->eap)) {
		const u8 *bssid = wpa_s->bssid;
		if (is_zero_ether_addr(bssid))
			bssid = wpa_s->pending_bssid;

		da16x_wps_prt("     [%s] WPS: PIN registration with " MACSTR
			   " did not succeed - continue trying to find "
			   "suitable AP\n\n", __func__, MAC2STR(bssid));

		wpa_blacklist_add(wpa_s, bssid);

		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		wpa_s->reassociate = 1;
#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
		wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
		wpa_supplicant_req_scan(wpa_s, wpa_s->blacklist_cleared ? 5 : 0, 0);
		wpa_s->blacklist_cleared = 0;
		return 1;
	}

	wpas_wps_clear_ap_info(wpa_s);
	da16x_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPS && !wpa_s->wps_success)
		da16x_wps_prt("[WPA_MSG]\n" WPS_EVENT_FAIL);

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPS && wpa_s->current_ssid &&
	    !(wpa_s->current_ssid->key_mgmt & WPA_KEY_MGMT_WPS)) {
		int disabled = wpa_s->current_ssid->disabled;
		unsigned int freq = wpa_s->assoc_freq;
		struct wpa_bss *bss;
		struct wpa_ssid *ssid = NULL;
		int use_fast_assoc = 0;

		da16x_wps_prt("     [%s] WPS: Network configuration replaced - "
			   "try to associate with the received credential "
			   "(freq=%u)\n\n", __func__, freq);
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		if (disabled) {
			da16x_wps_prt("     [%s] WPS: Current network is "
				     "disabled - wait for user to enable\n",
				     __func__);
			return 1;
		}
		wpa_s->after_wps = 5;
		wpa_s->wps_freq = freq;
		wpa_s->normal_scans = 0;
		wpa_s->reassociate = 1;

		da16x_wps_prt("     [%s] WPS: Checking whether fast association "
			   "without a new scan can be used\n", __func__);
		bss = wpa_supplicant_pick_network(wpa_s, &ssid);
		if (bss) {
			struct wpabuf *wps;
			struct wps_parse_attr attr;

			wps = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
			if (wps && wps_parse_msg(wps, &attr) == 0 &&
			    attr.wps_state &&
			    *attr.wps_state == WPS_STATE_CONFIGURED)
				use_fast_assoc = 1;
			wpabuf_free(wps);
		}

		/*
		 * Complete the next step from an eloop timeout to allow pending
		 * driver events related to the disconnection to be processed
		 * first. This makes it less likely for disconnection event to
		 * cause problems with the following connection.
		 */
		da16x_wps_prt("     [%s] WPS: Continue association "
			     "from timeout\n", __func__);
		wpas_wps_assoc_with_cred_cancel(wpa_s);
		da16x_eloop_register_timeout(0, 10000,
				       wpas_wps_assoc_with_cred, wpa_s,
				       use_fast_assoc ? (void *) 1 :
				       (void *) 0);

		return 1;
	}

	if (wpa_s->key_mgmt == WPA_KEY_MGMT_WPS && wpa_s->current_ssid) {
		da16x_wps_prt("     [%s] WPS: Registration completed - waiting "
			   "for external credential processing\n", __func__);
		wpas_clear_wps(wpa_s);
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
		return 1;
	}

	return 0;
}

static void wpas_wps_security_workaround(struct wpa_supplicant *wpa_s,
					 struct wpa_ssid *ssid,
					 const struct wps_credential *cred)
{
	struct wpa_driver_capa capa;
	struct wpa_bss *bss;
	const u8 *ie;
	struct wpa_ie_data adv;
	int wpa2 = 0, ccmp = 0;

	/*
	 * Many existing WPS APs do not know how to negotiate WPA2 or CCMP in
	 * case they are configured for mixed mode operation (WPA+WPA2 and
	 * TKIP+CCMP). Try to use scan results to figure out whether the AP
	 * actually supports stronger security and select that if the client
	 * has support for it, too.
	 */

	if (wpa_drv_get_capa(wpa_s, &capa))
		return; /* Unknown what driver supports */

	if (ssid->ssid == NULL)
		return;
#if 0
	bss = wpa_bss_get(wpa_s, cred->mac_addr, ssid->ssid, ssid->ssid_len);
	if (!bss) {
		bss = wpa_bss_get(wpa_s, wpa_s->bssid, ssid->ssid, ssid->ssid_len);
	}
	if (bss == NULL) {
		da16x_wps_prt("     [%s] WPS: The AP was not found from BSS "
			   "table - use credential as-is\n", __func__);
		return;
	}
#else
	for (int i = 0; i < 2; i++) {
		bss = wpa_bss_get(wpa_s, cred->mac_addr, ssid->ssid, ssid->ssid_len);
		if (bss == NULL && i == 1) {
			da16x_wps_prt("[%s] WPS: The AP was not found from BSS "
				"table - use credential as-is\n", __func__);
			return;
		}
	}
#endif	/* 0 */

	da16x_wps_prt("[%s] WPS: AP found from BSS table\n", __func__);

	ie = wpa_bss_get_ie(bss, WLAN_EID_RSN);
	if (ie && wpa_parse_wpa_ie(ie, 2 + ie[1], &adv) == 0) {
		wpa2 = 1;
		if (adv.pairwise_cipher & WPA_CIPHER_CCMP)
			ccmp = 1;
	} else {
		ie = wpa_bss_get_vendor_ie(bss, WPA_IE_VENDOR_TYPE);
		if (ie && wpa_parse_wpa_ie(ie, 2 + ie[1], &adv) == 0 &&
		    adv.pairwise_cipher & WPA_CIPHER_CCMP)
			ccmp = 1;
	}

	if (ie == NULL && (ssid->proto & WPA_PROTO_WPA) &&
	    (ssid->pairwise_cipher & WPA_CIPHER_TKIP)) {
		/*
		 * TODO: This could be the initial AP configuration and the
		 * Beacon contents could change shortly. Should request a new
		 * scan and delay addition of the network until the updated
		 * scan results are available.
		 */
		da16x_wps_prt("[%s] WPS: The AP did not yet advertise WPA "
			   "support - use credential as-is\n", __func__);
		return;
	}

	if (ccmp && !(ssid->pairwise_cipher & WPA_CIPHER_CCMP) &&
	    (ssid->pairwise_cipher & WPA_CIPHER_TKIP) &&
	    (capa.key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
		da16x_wps_prt("[%s] WPS: Add CCMP into the credential "
			   "based on scan results\n", __func__);
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		if (wpa_s->conf->ap_scan == 1)
			ssid->pairwise_cipher |= WPA_CIPHER_CCMP;
		else
			ssid->pairwise_cipher = WPA_CIPHER_CCMP;
#else
		ssid->pairwise_cipher |= WPA_CIPHER_CCMP;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	}

	if (wpa2 && !(ssid->proto & WPA_PROTO_RSN) &&
	    (ssid->proto & WPA_PROTO_WPA) &&
	    (capa.enc & WPA_DRIVER_CAPA_ENC_CCMP)) {
		da16x_wps_prt("     [%s] WPS: Add WPA2 into the credential "
			   "based on scan results\n", __func__);
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
		if (wpa_s->conf->ap_scan == 1)
			ssid->proto |= WPA_PROTO_RSN;
		else
			ssid->proto = WPA_PROTO_RSN;
#else
	ssid->proto |= WPA_PROTO_RSN;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
	}
}


static void wpas_wps_remove_dup_network(struct wpa_supplicant *wpa_s,
					struct wpa_ssid *new_ssid)
{
	struct wpa_ssid *ssid, *next;

	for (ssid = wpa_s->conf->ssid, next = ssid ? ssid->next : NULL; ssid;
	     ssid = next, next = ssid ? ssid->next : NULL) {
		/*
		 * new_ssid has already been added to the list in
		 * wpas_wps_add_network(), so skip it.
		 */
		if (ssid == new_ssid)
			continue;

		if (ssid->bssid_set || new_ssid->bssid_set) {
			if (ssid->bssid_set != new_ssid->bssid_set)
				continue;
			if (os_memcmp(ssid->bssid, new_ssid->bssid, ETH_ALEN) != 0)
				continue;
		}

		/* compare SSID */
		if (ssid->ssid_len == 0 || ssid->ssid_len != new_ssid->ssid_len)
			continue;

		if (ssid->ssid && new_ssid->ssid) {
			if (os_memcmp(ssid->ssid, new_ssid->ssid, ssid->ssid_len) != 0)
				continue;
		} else if (ssid->ssid || new_ssid->ssid) {
			continue;
		}

		/* compare security parameters */
		if (ssid->auth_alg != new_ssid->auth_alg ||
		    ssid->key_mgmt != new_ssid->key_mgmt ||
		    ssid->proto != new_ssid->proto ||
		    ssid->pairwise_cipher != new_ssid->pairwise_cipher ||
		    ssid->group_cipher != new_ssid->group_cipher)
			continue;

		/* Remove the duplicated older network entry. */
		da16x_wps_prt("[%s] Remove duplicate network %d\n\n", __func__, ssid->id);
		if (wpa_s->next_ssid == ssid)
			wpa_s->next_ssid = NULL;
		if (wpa_s->wpa)
			wpa_sm_pmksa_cache_flush(wpa_s->wpa, ssid);

#ifdef CONFIG_P2P
		wpas_p2p_network_removed(wpa_s, ssid);
#endif /* CONFIG_P2P */

		wpa_config_remove_network(wpa_s->conf, ssid->id);
	}
}


static int wpa_supplicant_wps_cred(void *ctx,
				   const struct wps_credential *cred)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct wpa_ssid *ssid = wpa_s->current_ssid;
	u16 auth_type;
#ifdef CONFIG_WPS_REG_DISABLE_OPEN
	int registrar = 0;
#endif /* CONFIG_WPS_REG_DISABLE_OPEN */

	if ((wpa_s->conf->wps_cred_processing == 1 ||
	     wpa_s->conf->wps_cred_processing == 2) && cred->cred_attr) {
		size_t blen = cred->cred_attr_len * 2 + 1;
		char *buf = os_malloc(blen);
		if (buf) {
			wpa_snprintf_hex(buf, blen, cred->cred_attr, cred->cred_attr_len);
			da16x_wps_prt("[WPA_MSG]\n" "%s%s", WPS_EVENT_CRED_RECEIVED, buf);
			os_free(buf);
		}
	} else
		da16x_wps_prt("[WPA_MSG]\n" WPS_EVENT_CRED_RECEIVED);

#ifdef ENABLE_WPS_DBG
	da16x_dump("WPS: Received Credential attribute",
		  cred->cred_attr, cred->cred_attr_len);
#endif

	if (wpa_s->conf->wps_cred_processing == 1)
		return 0;

#ifdef ENABLE_WPS_DBG
	da16x_ascii_dump("WPS: SSID", cred->ssid, cred->ssid_len);
#endif
	da16x_wps_prt("[%s] WPS: Authentication Type 0x%x\n\n", __func__,
		     cred->auth_type);
	da16x_wps_prt("[%s] WPS: Encryption Type 0x%x\n\n", __func__,
		     cred->encr_type);
	da16x_wps_prt("[%s] WPS: Network Key Index %d\n\n", __func__,
		     cred->key_idx);
#ifdef ENABLE_WPS_DBG
	da16x_dump("WPS: Network Key", cred->key, cred->key_len);
#endif
	da16x_wps_prt("[%s] WPS: MAC Address " MACSTR, __func__,
		   MAC2STR(cred->mac_addr));

	auth_type = cred->auth_type;
	if (auth_type == (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) {
		da16x_wps_prt("[%s] WPS: Workaround - convert mixed-mode "
			     "auth_type into WPA2PSK\n", __func__);
		auth_type = WPS_AUTH_WPA2PSK;
	}

	if (auth_type != WPS_AUTH_OPEN &&
	    auth_type != WPS_AUTH_WPAPSK &&
	    auth_type != WPS_AUTH_WPA2PSK) {
		da16x_wps_prt("[%s] WPS: Ignored credentials for "
			     "unsupported authentication type 0x%x\n\n",
			     __func__, auth_type);
		return 0;
	}

	if (auth_type == WPS_AUTH_WPAPSK || auth_type == WPS_AUTH_WPA2PSK) {
		if (cred->key_len < 8 || cred->key_len > 2 * PMK_LEN) {
			da16x_wps_prt("[%s] WPS: Reject PSK credential with "
				     "invalid Network Key length %lu\n",
				     __func__, (unsigned long) cred->key_len);
			return -1;
		}
	}

	if (ssid && (ssid->key_mgmt & WPA_KEY_MGMT_WPS)) {
		da16x_wps_prt("[%s] WPS: Replace WPS network block based "
			     "on the received credential\n", __func__);
#ifdef CONFIG_WPS_REG_DISABLE_OPEN
		if (ssid->eap.identity &&
		    ssid->eap.identity_len == WSC_ID_REGISTRAR_LEN &&
		    os_memcmp(ssid->eap.identity, WSC_ID_REGISTRAR,
			      WSC_ID_REGISTRAR_LEN) == 0)
			registrar = 1;
#endif /* CONFIG_WPS_REG_DISABLE_OPEN */
		os_free(ssid->eap.identity);
		ssid->eap.identity = NULL;
		ssid->eap.identity_len = 0;
		os_free(ssid->eap.phase1);
		ssid->eap.phase1 = NULL;
		os_free(ssid->eap.eap_methods);
		ssid->eap.eap_methods = NULL;
#ifdef CONFIG_P2P
		if (!ssid->p2p_group)
#endif	/* CONFIG_P2P */
		{
			ssid->temporary = 0;
			ssid->bssid_set = 0;
		}
		ssid->disabled_until.sec = 0;
		ssid->disabled_until.usec = 0;
		ssid->auth_failures = 0;
	} else {
		da16x_wps_prt("     [%s] WPS: Create a new network based on the "
			   "received credential\n", __func__);

		ssid = wpa_config_add_network(wpa_s->conf, wpa_s->ifname, FIXED_NETWORK_ID_NONE);
		if (ssid == NULL)
			return -1;
		if (wpa_s->current_ssid) {
			/*
			 * Should the GO issue multiple credentials for some
			 * reason, each credential should be marked as a
			 * temporary P2P group similarly to the one that gets
			 * marked as such based on the pre-configured values
			 * used for the WPS network block.
			 */
#ifdef CONFIG_P2P
			ssid->p2p_group = wpa_s->current_ssid->p2p_group;
#endif	/* CONFIG_P2P */
			ssid->temporary = wpa_s->current_ssid->temporary;
		}
	}

	wpa_config_set_network_defaults(ssid);

	os_free(ssid->ssid);
	ssid->ssid = os_malloc(cred->ssid_len);
	if (ssid->ssid) {
		os_memcpy(ssid->ssid, cred->ssid, cred->ssid_len);
		ssid->ssid_len = cred->ssid_len;
	}

	switch (cred->encr_type) {
	case WPS_ENCR_NONE:
		break;
	case WPS_ENCR_TKIP:
		ssid->pairwise_cipher = WPA_CIPHER_TKIP;
		break;
	case WPS_ENCR_AES:
		ssid->pairwise_cipher = WPA_CIPHER_CCMP;
		break;
	}

	switch (auth_type) {
	case WPS_AUTH_OPEN:
		ssid->auth_alg = WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = WPA_KEY_MGMT_NONE;
		ssid->proto = 0;
#ifdef CONFIG_WPS_REG_DISABLE_OPEN
		if (registrar) {
			da16x_wps_prt("[WPA_MSG]\n" WPS_EVENT_OPEN_NETWORK
				     "id=%d - Credentials for an open "
				     "network disabled by default - use "
				     "'select_network %d' to enable",
				     ssid->id, ssid->id);
			ssid->disabled = 1;
		}
#endif /* CONFIG_WPS_REG_DISABLE_OPEN */
		break;
	case WPS_AUTH_WPAPSK:
		ssid->auth_alg = WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = WPA_KEY_MGMT_PSK;
		ssid->proto = WPA_PROTO_WPA;
		break;
	case WPS_AUTH_WPA2PSK:
		ssid->auth_alg = WPA_AUTH_ALG_OPEN;
		ssid->key_mgmt = WPA_KEY_MGMT_PSK;
		ssid->proto = WPA_PROTO_RSN;
		break;
	}

	if (ssid->key_mgmt == WPA_KEY_MGMT_PSK) {
		if (cred->key_len == 2 * PMK_LEN) {
			if (hexstr2bin((const char *) cred->key, ssid->psk,
				       PMK_LEN)) {
				da16x_wps_prt("     [%s] WPS: Invalid Network "
					   "Key\n", __func__);
				return -1;
			}
			ssid->psk_set = 1;
			ssid->export_keys = 1;
		} else if (cred->key_len >= 8 && cred->key_len < 2 * PMK_LEN) {
			os_free(ssid->passphrase);
			ssid->passphrase = os_malloc(cred->key_len + 1);
			if (ssid->passphrase == NULL)
				return -1;
			os_memcpy(ssid->passphrase, cred->key, cred->key_len);
			ssid->passphrase[cred->key_len] = '\0';
			wpa_config_update_psk(ssid);
			ssid->export_keys = 1;
		} else {
			da16x_wps_prt("     [%s] WPS: Invalid Network Key "
				   "length %lu\n\n", __func__,
				   (unsigned long) cred->key_len);
			return -1;
		}
	}

	wpas_wps_security_workaround(wpa_s, ssid, cred);

	wpas_wps_remove_dup_network(wpa_s, ssid);

#ifdef CONFIG_P2P
	/* Do not write to NVRAM in the P2P mode. */
	if (   get_run_mode() != P2P_ONLY_MODE
#ifdef	CONFIG_CONCURRENT
	    && get_run_mode() != STA_P2P_CONCURRENT_MODE
#endif	/* CONFIG_CONCURRENT */
	    && get_run_mode() != P2P_GO_FIXED_MODE
	   )
#endif /* CONFIG_P2P */
	{
		if (wpa_s->conf->update_config &&
#if defined ( __SUPP_27_SUPPORT__ )
		    wpa_config_write(wpa_s->confname, wpa_s->conf, wpa_s->ifname)
#else
		    wpa_config_write(wpa_s->conf, wpa_s->ifname)
#endif // __SUPP_27_SUPPORT__
		   )
		{
			da16x_wps_prt("     [%s] WPS: Failed to update "
				     "configuration\n", __func__);
			return -1;
		}
	}

	/*
	 * Optimize the post-WPS scan based on the channel used during
	 * the provisioning in case EAP-Failure is not received.
	 */
	wpa_s->after_wps = 5;
	wpa_s->wps_freq = wpa_s->assoc_freq;

	return 0;
}


static void wpa_supplicant_wps_event_m2d(struct wpa_supplicant *wpa_s,
					 struct wps_event_m2d *m2d)
{
	da16x_wps_prt(" [%s]" WPS_EVENT_M2D
		"dev_password_id=%d config_error=%d\n",
		__func__, m2d->dev_password_id, m2d->config_error);

#ifdef CONFIG_P2P
	if (wpa_s->parent && wpa_s->parent != wpa_s) {
		da16x_p2p_prt("  [%s] " WPS_EVENT_M2D
			"dev_password_id=%d config_error=%d\n",
			__func__, m2d->dev_password_id, m2d->config_error);
	}
	if (m2d->config_error == WPS_CFG_MULTIPLE_PBC_DETECTED) {
		/*
		 * Notify P2P from eloop timeout to avoid issues with the
		 * interface getting removed while processing a message.
		 */
		da16x_eloop_register_timeout(0, 0,
				wpas_p2p_pbc_overlap_cb, wpa_s, NULL);
	}
#endif /* CONFIG_P2P */
}


static void wpas_wps_clear_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	da16x_wps_prt("     [%s] WPS: Clear WPS network from timeout\n",
		     __func__);
	wpas_clear_wps(wpa_s);
}

static void wpa_supplicant_wps_event_fail(struct wpa_supplicant *wpa_s,
					  struct wps_event_fail *fail)
{
	if (fail->error_indication > 0 &&
	    fail->error_indication < NUM_WPS_EI_VALUES) {
		da16x_wps_prt("               [WPA_MSG]\n"
			WPS_EVENT_FAIL "msg=%d config_error=%d reason=%d (%s)",
			fail->msg, fail->config_error, fail->error_indication,
#if 0	/* by Bright */
			wps_ei_str((enum wps_error_indication)fail->error_indication)
#else
			fail->error_indication
#endif	/* 0 */
			);

		if (wpa_s->parent && wpa_s->parent != wpa_s)
			da16x_wps_prt("               [WPA_MSG]\n" WPS_EVENT_FAIL
				"msg=%d config_error=%d reason=%d (%s)",
				fail->msg, fail->config_error,
				fail->error_indication,
#if 0	/* by Bright */
				wps_ei_str((enum wps_error_indication)fail->error_indication)
#else
				fail->error_indication
#endif	/* 0 */
				);
	} else {
		da16x_wps_prt("               [WPA_MSG]\n"
			WPS_EVENT_FAIL "msg=%d config_error=%d",
			fail->msg, fail->config_error);
		if (wpa_s->parent && wpa_s->parent != wpa_s)
			da16x_wps_prt("               [WPA_MSG]\n" WPS_EVENT_FAIL
				"msg=%d config_error=%d",
				fail->msg, fail->config_error);
	}

	/*
	 * Need to allow WPS processing to complete, e.g., by sending WSC_NACK.
	 */
	da16x_wps_prt("     [%s] WPS: Register timeout to clear "
					"WPS network\n", __func__);
	da16x_eloop_cancel_timeout(wpas_wps_clear_timeout, wpa_s, NULL);
	da16x_eloop_register_timeout(0, 100000, wpas_wps_clear_timeout, wpa_s,
				    NULL);

#ifdef CONFIG_P2P
	wpas_p2p_wps_failed(wpa_s, fail);
#endif /* CONFIG_P2P */
}

static void wpas_wps_reenable_networks_cb(void *eloop_ctx, void *timeout_ctx);

static void wpas_wps_reenable_networks(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;
	int changed = 0;

	da16x_eloop_cancel_timeout(wpas_wps_reenable_networks_cb, wpa_s, NULL);

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (ssid->disabled_for_connect && ssid->disabled) {
			ssid->disabled_for_connect = 0;
			ssid->disabled = 0;
			changed++;
		}
	}

	if (changed) {
		if (wpa_s->conf->update_config &&
#if defined ( __SUPP_27_SUPPORT__ )
		    wpa_config_write(wpa_s->confname, wpa_s->conf, wpa_s->ifname)
#else
		    wpa_config_write(wpa_s->conf, wpa_s->ifname)
#endif // __SUPP_27_SUPPORT__
		    )
		{
			da16x_wps_prt("     [%s] WPS: Failed to update "
				   "configuration\n", __func__);
		}
	}
}


static void wpas_wps_reenable_networks_cb(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;
	/* Enable the networks disabled during wpas_wps_reassoc */
	wpas_wps_reenable_networks(wpa_s);
}

#ifdef	CONFIG_WPS
static void wpa_supplicant_wps_event_success(struct wpa_supplicant *wpa_s)
{
	da16x_wps_prt("               [WPA_MSG]\n" WPS_EVENT_SUCCESS);
	wpa_s->wps_success = 1;
	if (wpa_s->current_ssid)
		wpas_clear_temp_disabled(wpa_s, wpa_s->current_ssid, 1);
	wpa_s->extra_blacklist_count = 0;

	/*
	 * Enable the networks disabled during wpas_wps_reassoc after 10
	 * seconds. The 10 seconds timer is to allow the data connection to be
	 * formed before allowing other networks to be selected.
	 */
	da16x_eloop_register_timeout(10, 0, wpas_wps_reenable_networks_cb,
				    wpa_s, NULL);

#ifdef CONFIG_P2P
	wpas_p2p_wps_success(wpa_s, wpa_s->bssid, 0);
#endif /* CONFIG_P2P */
}

#ifdef CONFIG_WPS_ER
static void wpa_supplicant_wps_event_er_ap_add(struct wpa_supplicant *wpa_s,
					       struct wps_event_er_ap *ap)
{
	char uuid_str[100];
	char dev_type[WPS_DEV_TYPE_BUFSIZE];

	uuid_bin2str(ap->uuid, uuid_str, sizeof(uuid_str));
	if (ap->pri_dev_type)
		wps_dev_type_bin2str(ap->pri_dev_type, dev_type,
				     sizeof(dev_type));
	else
		dev_type[0] = '\0';

	da16x_wps_prt("      [WPA_MSG]\n" WPS_EVENT_ER_AP_ADD "%s " MACSTR
		" pri_dev_type=%s wps_state=%d |%s|%s|%s|%s|%s|%s|",
		uuid_str, MAC2STR(ap->mac_addr), dev_type, ap->wps_state,
		ap->friendly_name ? ap->friendly_name : "",
		ap->manufacturer ? ap->manufacturer : "",
		ap->model_description ? ap->model_description : "",
		ap->model_name ? ap->model_name : "",
		ap->manufacturer_url ? ap->manufacturer_url : "",
		ap->model_url ? ap->model_url : "");
}

static void wpa_supplicant_wps_event_er_ap_remove(struct wpa_supplicant *wpa_s,
						  struct wps_event_er_ap *ap)
{
	char uuid_str[100];
	uuid_bin2str(ap->uuid, uuid_str, sizeof(uuid_str));
	da16x_wps_prt("     [WPA_MSG]\n" WPS_EVENT_ER_AP_REMOVE "%s", uuid_str);
}

static void wpa_supplicant_wps_event_er_enrollee_add(
	struct wpa_supplicant *wpa_s, struct wps_event_er_enrollee *enrollee)
{
	char uuid_str[100];
	char dev_type[WPS_DEV_TYPE_BUFSIZE];

	uuid_bin2str(enrollee->uuid, uuid_str, sizeof(uuid_str));
	if (enrollee->pri_dev_type)
		wps_dev_type_bin2str(enrollee->pri_dev_type, dev_type,
				     sizeof(dev_type));
	else
		dev_type[0] = '\0';

	da16x_wps_prt("     [WPA_MSG]\n" WPS_EVENT_ER_ENROLLEE_ADD "%s " MACSTR
		" M1=%d config_methods=0x%x dev_passwd_id=%d pri_dev_type=%s "
		"|%s|%s|%s|%s|%s|",
		uuid_str, MAC2STR(enrollee->mac_addr), enrollee->m1_received,
		enrollee->config_methods, enrollee->dev_passwd_id, dev_type,
		enrollee->dev_name ? enrollee->dev_name : "",
		enrollee->manufacturer ? enrollee->manufacturer : "",
		enrollee->model_name ? enrollee->model_name : "",
		enrollee->model_number ? enrollee->model_number : "",
		enrollee->serial_number ? enrollee->serial_number : "");
}

static void wpa_supplicant_wps_event_er_enrollee_remove(
	struct wpa_supplicant *wpa_s, struct wps_event_er_enrollee *enrollee)
{
	char uuid_str[100];
	uuid_bin2str(enrollee->uuid, uuid_str, sizeof(uuid_str));
	da16x_wps_prt("[WPA_MSG]\n" WPS_EVENT_ER_ENROLLEE_REMOVE "%s " MACSTR,
		uuid_str, MAC2STR(enrollee->mac_addr));
}

static void wpa_supplicant_wps_event_er_ap_settings(
	struct wpa_supplicant *wpa_s,
	struct wps_event_er_ap_settings *ap_settings)
{
	char uuid_str[100];
	char key_str[65];
	const struct wps_credential *cred = ap_settings->cred;

	key_str[0] = '\0';
	if (cred->auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK)) {
		if (cred->key_len >= 8 && cred->key_len <= 64) {
			os_memcpy(key_str, cred->key, cred->key_len);
			key_str[cred->key_len] = '\0';
		}
	}

	uuid_bin2str(ap_settings->uuid, uuid_str, sizeof(uuid_str));
	/* Use da16x_wps_prt to avoid showing the key in debug log */
	da16x_wps_prt("          [WPA_MSG]\n" WPS_EVENT_ER_AP_SETTINGS
		     "uuid=%s ssid=%s auth_type=0x%04x encr_type=0x%04x "
		     "key=%s",
		     uuid_str, wpa_ssid_txt(cred->ssid, cred->ssid_len),
		     cred->auth_type, cred->encr_type, key_str);
}

static void wpa_supplicant_wps_event_er_set_sel_reg(
	struct wpa_supplicant *wpa_s,
	struct wps_event_er_set_selected_registrar *ev)
{
	char uuid_str[100];

	uuid_bin2str(ev->uuid, uuid_str, sizeof(uuid_str));
	switch (ev->state) {
	case WPS_ER_SET_SEL_REG_START:
		da16x_wps_prt("         [WPA_MSG]\n" WPS_EVENT_ER_SET_SEL_REG
			"uuid=%s state=START sel_reg=%d dev_passwd_id=%u "
			"sel_reg_config_methods=0x%x",
			uuid_str, ev->sel_reg, ev->dev_passwd_id,
			ev->sel_reg_config_methods);
		break;
	case WPS_ER_SET_SEL_REG_DONE:
		da16x_wps_prt("         [WPA_MSG]\n" WPS_EVENT_ER_SET_SEL_REG
			"uuid=%s state=DONE", uuid_str);
		break;
	case WPS_ER_SET_SEL_REG_FAILED:
		da16x_wps_prt("         [WPA_MSG]\n" WPS_EVENT_ER_SET_SEL_REG
			"uuid=%s state=FAILED", uuid_str);
		break;
	}
}
#endif

static void wpa_supplicant_wps_event(void *ctx, enum wps_event event,
				     union wps_event_data *data)
{
	struct wpa_supplicant *wpa_s = ctx;
	switch (event) {
	case WPS_EV_M2D:
		wpa_supplicant_wps_event_m2d(wpa_s, &data->m2d);
		break;
	case WPS_EV_FAIL:
		wpa_supplicant_wps_event_fail(wpa_s, &data->fail);
		break;
	case WPS_EV_SUCCESS:
		wpa_supplicant_wps_event_success(wpa_s);
		break;
	case WPS_EV_PWD_AUTH_FAIL:
#ifdef CONFIG_AP
		if (wpa_s->ap_iface && data->pwd_auth_fail.enrollee)
			wpa_supplicant_ap_pwd_auth_fail(wpa_s);
#endif /* CONFIG_AP */
		break;
	case WPS_EV_PBC_OVERLAP:
		break;
	case WPS_EV_PBC_TIMEOUT:
		break;
	case WPS_EV_PBC_ACTIVE:
		da16x_wps_prt("               [WPA_MSG]\n" WPS_EVENT_ACTIVE);
		break;
	case WPS_EV_PBC_DISABLE:
		da16x_wps_prt("               [WPA_MSG]\n" WPS_EVENT_DISABLE);
		break;
#ifdef CONFIG_WPS_ER
	case WPS_EV_ER_AP_ADD:
		wpa_supplicant_wps_event_er_ap_add(wpa_s, &data->ap);
		break;
	case WPS_EV_ER_AP_REMOVE:
		wpa_supplicant_wps_event_er_ap_remove(wpa_s, &data->ap);
		break;
	case WPS_EV_ER_ENROLLEE_ADD:
		wpa_supplicant_wps_event_er_enrollee_add(wpa_s,
							 &data->enrollee);
		break;
	case WPS_EV_ER_ENROLLEE_REMOVE:
		wpa_supplicant_wps_event_er_enrollee_remove(wpa_s,
							    &data->enrollee);
		break;
	case WPS_EV_ER_AP_SETTINGS:
		wpa_supplicant_wps_event_er_ap_settings(wpa_s,
							&data->ap_settings);
		break;
	case WPS_EV_ER_SET_SELECTED_REGISTRAR:
		wpa_supplicant_wps_event_er_set_sel_reg(wpa_s,
							&data->set_sel_reg);
		break;
	case WPS_EV_AP_PIN_SUCCESS:
		break;
#endif
	}
}

static int wpa_supplicant_wps_rf_band(void *ctx)
{
	struct wpa_supplicant *wpa_s = ctx;

	if (!wpa_s->current_ssid || !wpa_s->assoc_freq)
		return 0;

#ifdef	CONFIG_AP_SUPPORT_5GHZ
	return (wpa_s->assoc_freq > 2484) ? WPS_RF_50GHZ : WPS_RF_24GHZ;
#else
	return WPS_RF_24GHZ;
#endif	/* CONFIG_AP_SUPPORT_5GHZ */
}

enum wps_request_type wpas_wps_get_req_type(struct wpa_ssid *ssid)
{
	if (eap_is_wps_pbc_enrollee(&ssid->eap) ||
	    eap_is_wps_pin_enrollee(&ssid->eap))
		return WPS_REQ_ENROLLEE;
	else
		return WPS_REQ_REGISTRAR;
}

static void wpas_clear_wps(struct wpa_supplicant *wpa_s)
{
	int id;
	struct wpa_ssid *ssid, *remove_ssid = NULL, *prev_current;

	wpa_s->after_wps = 0;
	wpa_s->known_wps_freq = 0;

	prev_current = wpa_s->current_ssid;

	/* Enable the networks disabled during wpas_wps_reassoc */
	wpas_wps_reenable_networks(wpa_s);

	da16x_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	da16x_eloop_cancel_timeout(wpas_wps_clear_timeout, wpa_s, NULL);

	/* Remove any existing WPS network from configuration */
	ssid = wpa_s->conf->ssid;
	while (ssid) {
		if (ssid->key_mgmt & WPA_KEY_MGMT_WPS) {
			if (ssid == wpa_s->current_ssid) {
				wpa_supplicant_deauthenticate(
					wpa_s, WLAN_REASON_DEAUTH_LEAVING);
			}
			id = ssid->id;
			remove_ssid = ssid;
		} else
			id = -1;
		ssid = ssid->next;
		if (id >= 0) {
			if (prev_current == remove_ssid) {
				wpa_sm_set_config(wpa_s->wpa, NULL);
				eapol_sm_notify_config(wpa_s->eapol, NULL,
						       NULL);
			}
			if (wpa_s->next_ssid == remove_ssid)
				wpa_s->next_ssid = NULL;
			if (wpa_s->wpa)
				wpa_sm_pmksa_cache_flush(wpa_s->wpa,
							 remove_ssid);

#ifdef CONFIG_P2P
			wpas_p2p_network_removed(wpa_s, remove_ssid);
#endif /* CONFIG_P2P */
			wpa_config_remove_network(wpa_s->conf, id);
		}
	}

	is_wps_in_use = 0;

	wpas_wps_clear_ap_info(wpa_s);
}

static void wpas_wps_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	da16x_wps_prt("[WPA_MSG]\n" WPS_EVENT_TIMEOUT
			"Requested operation timed out");
	wpas_clear_wps(wpa_s);
}

static struct wpa_ssid * wpas_wps_add_network(struct wpa_supplicant *wpa_s,
					      int registrar, const u8 *dev_addr,
					      const u8 *bssid)
{
	struct wpa_ssid *ssid;

	if (chk_dpm_wakeup() == pdTRUE) {
		wpa_config_read_global(wpa_s->conf, wpa_s->ifname);
		wpas_wps_update_config(wpa_s);
	}

	ssid = wpa_config_add_network(wpa_s->conf, wpa_s->ifname,
				      FIXED_NETWORK_ID_NONE);
	if (ssid == NULL)
		return NULL;
	wpa_config_set_network_defaults(ssid);
	ssid->temporary = 1;
	if (wpa_config_set(ssid, "key_mgmt", "WPS") < 0 ||
	    wpa_config_set(ssid, "eap", "WSC") < 0 ||
	    wpa_config_set(ssid, "identity", registrar ?
			   "\"" WSC_ID_REGISTRAR "\"" :
			   "\"" WSC_ID_ENROLLEE "\"") < 0) {
		if (wpa_s->next_ssid == ssid)
			wpa_s->next_ssid = NULL;
		if (wpa_s->wpa)
			wpa_sm_pmksa_cache_flush(wpa_s->wpa, ssid);

#ifdef CONFIG_P2P
		wpas_p2p_network_removed(wpa_s, ssid);
#endif /* CONFIG_P2P */

		wpa_config_remove_network(wpa_s->conf, ssid->id);
		return NULL;
	}

#ifdef CONFIG_P2P
	if (dev_addr)
		os_memcpy(ssid->go_p2p_dev_addr, dev_addr, ETH_ALEN);
#endif /* CONFIG_P2P */

	if (bssid) {
#ifndef CONFIG_P2P
		struct wpa_bss *bss;
		int count = 0;
#endif /* CONFIG_P2P */

		os_memcpy(ssid->bssid, bssid, ETH_ALEN);
		ssid->bssid_set = 1;

		/*
		 * Note: With P2P, the SSID may change at the time the WPS
		 * provisioning is started, so better not filter the AP based
		 * on the current SSID in the scan results.
		 */
#ifndef CONFIG_P2P
		dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
			if (os_memcmp(bssid, bss->bssid, ETH_ALEN) != 0)
				continue;

			os_free(ssid->ssid);
			ssid->ssid = os_malloc(bss->ssid_len);
			if (ssid->ssid == NULL)
				break;
			os_memcpy(ssid->ssid, bss->ssid, bss->ssid_len);
			ssid->ssid_len = bss->ssid_len;
			da16x_ascii_dump("WPS: Picked SSID from "
					  "scan results",
					  ssid->ssid, ssid->ssid_len);
			count++;
		}

		if (count > 1) {
			da16x_wps_prt("     [%s] WPS: More than one SSID found "
				   "for the AP; use wildcard\n", __func__);
			os_free(ssid->ssid);
			ssid->ssid = NULL;
			ssid->ssid_len = 0;
		}
#endif /* CONFIG_P2P */
	}

	return ssid;
}

static void wpas_wps_temp_disable(struct wpa_supplicant *wpa_s,
				  struct wpa_ssid *selected)
{
	struct wpa_ssid *ssid;

	if (wpa_s->current_ssid)
		wpa_supplicant_deauthenticate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);

	/* Mark all other networks disabled and trigger reassociation */
	ssid = wpa_s->conf->ssid;
	while (ssid) {
		int was_disabled = ssid->disabled;
		ssid->disabled_for_connect = 0;
		/*
		 * In case the network object corresponds to a persistent group
		 * then do not send out network disabled signal. In addition,
		 * do not change disabled status of persistent network objects
		 * from 2 to 1 should we connect to another network.
		 */
		if (was_disabled != 2) {
			ssid->disabled = ssid != selected;
			if (was_disabled != ssid->disabled) {
				if (ssid->disabled)
					ssid->disabled_for_connect = 1;
			}
		}
		ssid = ssid->next;
	}
}

static void wpas_wps_reassoc(struct wpa_supplicant *wpa_s,
			     struct wpa_ssid *selected, const u8 *bssid,
			     int freq)
{
	struct wpa_bss *bss;

	wpa_s->after_wps = 0;
	wpa_s->known_wps_freq = 0;
	if (freq) {
		wpa_s->after_wps = 5;
		wpa_s->wps_freq = freq;
	} else if (bssid) {
		bss = wpa_bss_get_bssid_latest(wpa_s, bssid);
		if (bss && bss->freq > 0) {
			wpa_s->known_wps_freq = 1;
			wpa_s->wps_freq = bss->freq;
		}
	}

	wpas_wps_temp_disable(wpa_s, selected);

	wpa_s->disconnected = 0;
	wpa_s->reassociate = 1;
	wpa_s->scan_runs = 0;
	wpa_s->normal_scans = 0;
	wpa_s->wps_success = 0;
	wpa_s->blacklist_cleared = 0;

#ifdef PROBE_REQ_WITH_SSID_FOR_ASSOC
	wpa_s->scan_for_connection = 1;
#endif /* PROBE_REQ_WITH_SSID_FOR_ASSOC */
	wpa_supplicant_req_scan(wpa_s, 0, 0);
}

int wpas_wps_start_pbc(struct wpa_supplicant *wpa_s, const u8 *bssid,
		       int p2p_group)
{
	struct wpa_ssid *ssid;
	wpas_clear_wps(wpa_s);
	ssid = wpas_wps_add_network(wpa_s, 0, NULL, bssid);
	if (ssid == NULL)
		return -1;
	ssid->temporary = 1;
	is_wps_in_use = 1;
#ifdef CONFIG_P2P
	ssid->p2p_group = p2p_group;
	if (p2p_group && wpa_s->go_params && wpa_s->go_params->ssid_len) {
		ssid->ssid = os_zalloc(wpa_s->go_params->ssid_len + 1);
		if (ssid->ssid) {
			ssid->ssid_len = wpa_s->go_params->ssid_len;
			os_memcpy(ssid->ssid, wpa_s->go_params->ssid,
				  ssid->ssid_len);
			da16x_ascii_dump("WPS: Use specific AP "
					  "SSID", ssid->ssid, ssid->ssid_len);
		}
	}
#endif /* CONFIG_P2P */
	if (wpa_config_set(ssid, "phase1", "\"pbc=1\"") < 0)
		return -1;


	if (wpa_s->wps_fragment_size)
		ssid->eap.fragment_size = wpa_s->wps_fragment_size;

	da16x_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout,
			       wpa_s, NULL);
	wpas_wps_reassoc(wpa_s, ssid, bssid, 0);

	return 0;
}

static int wpas_wps_start_dev_pw(struct wpa_supplicant *wpa_s,
				 const u8 *dev_addr, const u8 *bssid,
				 const char *pin, int p2p_group, u16 dev_pw_id,
				 const u8 *peer_pubkey_hash,
				 const u8 *ssid_val, size_t ssid_len, int freq)
{
	struct wpa_ssid *ssid;
	char val[128 + 2 * WPS_OOB_PUBKEY_HASH_LEN];
	unsigned int rpin = 0;
	char hash[2 * WPS_OOB_PUBKEY_HASH_LEN + 10];

	wpas_clear_wps(wpa_s);
	if (bssid && is_zero_ether_addr(bssid))
		bssid = NULL;
	ssid = wpas_wps_add_network(wpa_s, 0, dev_addr, bssid);
	if (ssid == NULL) {
		da16x_wps_prt("    [%s] WPS: Could not add network\n", __func__);
		return -1;
	}
	ssid->temporary = 1;
#ifdef CONFIG_P2P
	ssid->p2p_group = p2p_group;
#endif	/* CONFIG_P2P */
	if (ssid_val) {
		ssid->ssid = os_malloc(ssid_len);
		if (ssid->ssid) {
			os_memcpy(ssid->ssid, ssid_val, ssid_len);
			ssid->ssid_len = ssid_len;
		}
	}
	if (peer_pubkey_hash) {
		os_memcpy(hash, " pkhash=", 8);
		wpa_snprintf_hex_uppercase(hash + 8, sizeof(hash) - 8,
					   peer_pubkey_hash,
					   WPS_OOB_PUBKEY_HASH_LEN);
	} else {
		hash[0] = '\0';
	}
#ifdef CONFIG_P2P
	if (p2p_group && wpa_s->go_params && wpa_s->go_params->ssid_len) {
		ssid->ssid = os_zalloc(wpa_s->go_params->ssid_len + 1);
		if (ssid->ssid) {
			ssid->ssid_len = wpa_s->go_params->ssid_len;
			os_memcpy(ssid->ssid, wpa_s->go_params->ssid,
				  ssid->ssid_len);
			da16x_ascii_dump("WPS: Use specific AP "
					  "SSID", ssid->ssid, ssid->ssid_len);
		}
	}
#endif /* CONFIG_P2P */
	if (pin) {
		os_snprintf(val, sizeof(val), "\"pin=%s dev_pw_id=%u%s\"", pin, dev_pw_id, hash);
	} else {
		rpin = wps_generate_pin();
		os_snprintf(val, sizeof(val), "\"pin=%08d dev_pw_id=%u%s\"",
			    rpin, dev_pw_id, hash);
	}
	if (wpa_config_set(ssid, "phase1", val) < 0) {
		da16x_wps_prt("     [%s] WPS: Failed to set phase1 '%s'\n\n",
			     __func__, val);
		return -1;
	}
	if (wpa_s->wps_fragment_size)
		ssid->eap.fragment_size = wpa_s->wps_fragment_size;

	da16x_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout,
			       				wpa_s, NULL);

	wpa_s->wps_ap_iter = 1;
	wpas_wps_reassoc(wpa_s, ssid, bssid, freq);
	return rpin;
}

#ifdef CONFIG_WPS_PIN
int wpas_wps_start_pin(struct wpa_supplicant *wpa_s, const u8 *bssid,
		       const char *pin, int p2p_group, u16 dev_pw_id)
{
	is_wps_in_use = 1;
	return wpas_wps_start_dev_pw(wpa_s, NULL, bssid, pin, p2p_group,
				     dev_pw_id, NULL, NULL, 0, 0);
}
#endif

/* Cancel the wps pbc/pin requests */
int wpas_wps_cancel(struct wpa_supplicant *wpa_s, char *iface)
{
#ifdef CONFIG_AP
	/* for Concurrent WPS) */
	if (os_strcmp(wpa_s->ifname, SOFTAP_DEVICE_NAME) == 0 &&
	    !wpa_s->ap_iface)
		return -1;

	if (wpa_s->ap_iface) {
		da16x_wps_prt("     [%s] WPS: Cancelling in AP mode\n",
			     __func__);
		return wpa_supplicant_ap_wps_cancel(wpa_s);
	}
#endif /* CONFIG_AP */

#ifdef	CONFIG_P2P
	if (os_strcmp(wpa_s->ifname, P2P_DEVICE_NAME) == 0 &&
	    wpa_s->current_ssid && !wpa_s->p2p_group)
		return -1;
#endif	/* CONFIG_P2P */

	if (wpa_s->wpa_state == WPA_SCANNING ||
	    wpa_s->wpa_state == WPA_DISCONNECTED) {
		da16x_wps_prt("     [%s] WPS: Cancel operation - cancel scan\n",
			     __func__);
		wpa_supplicant_cancel_scan(wpa_s);
		wpas_clear_wps(wpa_s);
	} else if (wpa_s->wpa_state >= WPA_ASSOCIATED) {
#if 0	/* by Shingu 20171122 */
		da16x_wps_prt("     [%s] WPS: Cancel operation - "
			   "deauthenticate\n", __func__);
		wpa_supplicant_deauthenticate(wpa_s,
					      WLAN_REASON_DEAUTH_LEAVING);
		wpas_clear_wps(wpa_s);
#endif	/* 0 */
	} else {
		wpas_wps_reenable_networks(wpa_s);
		wpas_wps_clear_ap_info(wpa_s);

		if (da16x_eloop_cancel_timeout(wpas_wps_clear_timeout,
							wpa_s, NULL) > 0)
			wpas_clear_wps(wpa_s);
	}

	wpa_s->after_wps = 0;

	return 0;
}

#ifdef CONFIG_WPS_REGISTRAR
int wpas_wps_start_reg(struct wpa_supplicant *wpa_s, const u8 *bssid,
		       const char *pin, struct wps_new_ap_settings *settings)
{
	struct wpa_ssid *ssid;
	char val[200];
	char *pos, *end;
	int res;

	if (!pin)
		return -1;
	wpas_clear_wps(wpa_s);
	ssid = wpas_wps_add_network(wpa_s, 1, NULL, bssid);
	if (ssid == NULL)
		return -1;
	ssid->temporary = 1;
	is_wps_in_use = 1;
	pos = val;
	end = pos + sizeof(val);
	res = os_snprintf(pos, end - pos, "\"pin=%s", pin);
	if (res < 0 || res >= end - pos)
		return -1;
	pos += res;
	if (settings) {
		res = os_snprintf(pos, end - pos, " new_ssid=%s new_auth=%s "
				  "new_encr=%s new_key=%s",
				  settings->ssid_hex, settings->auth,
				  settings->encr, settings->key_hex);
		if (res < 0 || res >= end - pos)
			return -1;
		pos += res;
	}
	res = os_snprintf(pos, end - pos, "\"");
	if (res < 0 || res >= end - pos)
		return -1;
	if (wpa_config_set(ssid, "phase1", val) < 0)
		return -1;
	if (wpa_s->wps_fragment_size)
		ssid->eap.fragment_size = wpa_s->wps_fragment_size;

	da16x_eloop_register_timeout(WPS_PBC_WALK_TIME, 0, wpas_wps_timeout,
				    wpa_s, NULL);

	wpas_wps_reassoc(wpa_s, ssid, bssid, 0);
	return 0;
}

static int wpas_wps_new_psk_cb(void *ctx, const u8 *mac_addr,
			       const u8 *p2p_dev_addr, const u8 *psk,
			       size_t psk_len)
{
	if (is_zero_ether_addr(p2p_dev_addr)) {
		da16x_wps_prt("     [%s] Received new WPA/WPA2-PSK from WPS for "
			     "STA " MACSTR, __func__, MAC2STR(mac_addr));
	} else {
		da16x_wps_prt("     [%s] "
			     "Received new WPA/WPA2-PSK from WPS for STA "
			     MACSTR " P2P Device Addr " MACSTR, __func__,
			     MAC2STR(mac_addr), MAC2STR(p2p_dev_addr));
	}
#ifdef ENABLE_WPS_DBG
	da16x_dump("Per-device PSK", psk, psk_len);
#endif
	/* TODO */

	return 0;
}
#endif

#ifdef CONFIG_WPS_PIN
static void wpas_wps_pin_needed_cb(void *ctx, const u8 *uuid_e,
				   const struct wps_device_data *dev)
{
	char uuid[40], txt[400];
	int len;
	char devtype[WPS_DEV_TYPE_BUFSIZE];
	if (uuid_bin2str(uuid_e, uuid, sizeof(uuid)))
		return;
	da16x_wps_prt("     [%s] WPS: PIN needed for UUID-E %s\n\n", __func__,
		     uuid);
	len = os_snprintf(txt, sizeof(txt), "WPS-EVENT-PIN-NEEDED %s " MACSTR
			  " [%s|%s|%s|%s|%s|%s]",
			  uuid, MAC2STR(dev->mac_addr), dev->device_name,
			  dev->manufacturer, dev->model_name,
			  dev->model_number, dev->serial_number,
			  wps_dev_type_bin2str(dev->pri_dev_type, devtype,
					       sizeof(devtype)));
	if (len > 0 && len < (int) sizeof(txt))
		da16x_wps_prt("     [%s] %s\n\n", __func__, txt);
}
#endif

static void wpas_wps_set_sel_reg_cb(void *ctx, int sel_reg, u16 dev_passwd_id,
				    u16 sel_reg_config_methods)
{
#ifdef CONFIG_WPS_ER
	struct wpa_supplicant *wpa_s = ctx;

	if (wpa_s->wps_er == NULL)
		return;
	da16x_wps_prt("     [%s] WPS ER: SetSelectedRegistrar - sel_reg=%d "
		   "dev_password_id=%u sel_reg_config_methods=0x%x\n\n",
					__func__,
		   sel_reg, dev_passwd_id, sel_reg_config_methods);
	wps_er_set_sel_reg(wpa_s->wps_er, sel_reg, dev_passwd_id,
			   sel_reg_config_methods);
#endif /* CONFIG_WPS_ER */
}

static u16 wps_fix_config_methods(u16 config_methods)
{
	if ((config_methods &
	     (WPS_CONFIG_DISPLAY | WPS_CONFIG_VIRT_DISPLAY |
	      WPS_CONFIG_PHY_DISPLAY)) == WPS_CONFIG_DISPLAY) {
		da16x_wps_prt("     [%s] WPS: Converting display to "
			   "virtual_display for WPS 2.0 compliance\n",
					__func__);
		config_methods |= WPS_CONFIG_VIRT_DISPLAY;
	}
	if ((config_methods &
	     (WPS_CONFIG_PUSHBUTTON | WPS_CONFIG_VIRT_PUSHBUTTON |
	      WPS_CONFIG_PHY_PUSHBUTTON)) == WPS_CONFIG_PUSHBUTTON) {
		da16x_wps_prt("     [%s] WPS: Converting push_button to "
			   "virtual_push_button for WPS 2.0 compliance\n",
					__func__);
		config_methods |= WPS_CONFIG_VIRT_PUSHBUTTON;
	}

	return config_methods;
}

static void wpas_wps_set_uuid(struct wpa_supplicant *wpa_s,
			      struct wps_context *wps)
{
	char buf[50] = { 0, };
	char src[128] = { 0, };

#if 0	/* by Bright */
	memset(src, 0, 128);
#endif	/* 0 */

	if (is_nil_uuid(wpa_s->conf->uuid)) {
		uuid_gen_mac_addr(wpa_s->own_addr, wps->uuid);
		sprintf(src, "%s", "based on MAC address");
	} else {
		os_memcpy(wps->uuid, wpa_s->conf->uuid, WPS_UUID_LEN);
		sprintf(src, "%s", "based on configuration");
	}

	uuid_bin2str(wps->uuid, buf, sizeof(buf));
	da16x_wps_prt("[%s]WPS: UUID %s: %s\n", __func__, src, buf);
}

static void wpas_wps_set_vendor_ext_m1(struct wpa_supplicant *wpa_s,
				       struct wps_context *wps)
{
	wpabuf_free(wps->dev.vendor_ext_m1);
	wps->dev.vendor_ext_m1 = NULL;

	if (wpa_s->conf->wps_vendor_ext_m1) {
		wps->dev.vendor_ext_m1 =
			wpabuf_dup(wpa_s->conf->wps_vendor_ext_m1);
		if (!wps->dev.vendor_ext_m1) {
			da16x_wps_prt("[%s] WPS: Cannot "
				   "allocate memory for vendor_ext_m1\n", __func__);
		}
	}
}

int wpas_wps_init(struct wpa_supplicant *wpa_s)
{
	struct wps_context *wps;
	struct wps_registrar_config rcfg;
	struct hostapd_hw_modes *modes;
	u16 m;

	wps = os_zalloc(sizeof(*wps));
	if (wps == NULL)
		return -1;

	wps->cred_cb = wpa_supplicant_wps_cred;
	wps->event_cb = wpa_supplicant_wps_event;
	wps->rf_band_cb = wpa_supplicant_wps_rf_band;
	wps->cb_ctx = wpa_s;

	wps->dev.device_name = wpa_s->conf->device_name;
	wps->dev.manufacturer = wpa_s->conf->manufacturer;
	wps->dev.model_name = wpa_s->conf->model_name;
	wps->dev.model_number = wpa_s->conf->model_number;
	wps->dev.serial_number = wpa_s->conf->serial_number;
	wps->config_methods = wps_config_methods_str2bin(wpa_s->conf->config_methods);

	if ((wps->config_methods & (WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) ==
	    (WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) {
		da16x_err_prt("[%s] Both Label and Display config "
			   "methods are not allowed at the same time\n", __func__);
		os_free(wps);
		return -1;
	}
	wps->config_methods = wps_fix_config_methods(wps->config_methods);
	wps->dev.config_methods = wps->config_methods;
	os_memcpy(wps->dev.pri_dev_type, wpa_s->conf->device_type,
		  WPS_DEV_TYPE_LEN);

#ifdef	CONFIG_SUPP27_WPS_2ND_DEV
	wps->dev.num_sec_dev_types = wpa_s->conf->num_sec_device_types;
	os_memcpy(wps->dev.sec_dev_type, wpa_s->conf->sec_device_type,
		  WPS_DEV_TYPE_LEN * wps->dev.num_sec_dev_types);
#endif	/* CONFIG_SUPP27_WPS_2ND_DEV */

	wpas_wps_set_vendor_ext_m1(wpa_s, wps);

	wps->dev.os_version = WPA_GET_BE32(wpa_s->conf->os_version);
	modes = wpa_s->hw.modes;
	if (modes) {
		for (m = 0; m < wpa_s->hw.num_modes; m++) {
			if (modes[m].mode == HOSTAPD_MODE_IEEE80211B ||
			    modes[m].mode == HOSTAPD_MODE_IEEE80211G)
				wps->dev.rf_bands |= WPS_RF_24GHZ;
#ifdef	CONFIG_AP_SUPPORT_5GHZ
			else if (modes[m].mode == HOSTAPD_MODE_IEEE80211A)
				wps->dev.rf_bands |= WPS_RF_50GHZ;
#endif	/* CONFIG_AP_SUPPORT_5GHZ */
		}
	}
	if (wps->dev.rf_bands == 0) {
		/*
		 * Default to claiming support for both bands if the driver
		 * does not provide support for fetching supported bands.
		 */
#ifdef	CONFIG_AP_SUPPORT_5GHZ
		wps->dev.rf_bands = WPS_RF_24GHZ | WPS_RF_50GHZ;
#else
		wps->dev.rf_bands = WPS_RF_24GHZ;
#endif	/* CONFIG_AP_SUPPORT_5GHZ */
	}
	os_memcpy(wps->dev.mac_addr, wpa_s->own_addr, ETH_ALEN);
	wpas_wps_set_uuid(wpa_s, wps);

	wps->auth_types = WPS_AUTH_WPA2PSK | WPS_AUTH_WPAPSK;
	wps->encr_types = WPS_ENCR_AES | WPS_ENCR_TKIP;

	os_memset(&rcfg, 0, sizeof(rcfg));
#ifdef CONFIG_WPS_REGISTRAR
	rcfg.new_psk_cb = wpas_wps_new_psk_cb;
#endif
#ifdef CONFIG_WPS_PIN
	rcfg.pin_needed_cb = wpas_wps_pin_needed_cb;
#endif
	rcfg.set_sel_reg_cb = wpas_wps_set_sel_reg_cb;
	rcfg.cb_ctx = wpa_s;

#ifdef CONFIG_WPS_REGISTRAR
	if (os_strcmp(wpa_s->ifname, STA_DEVICE_NAME) == 0) {
		wps->registrar = wps_registrar_init(wps, &rcfg);

		if (wps->registrar == NULL) {
			da16x_err_prt("     [%s] Failed to initialize "
				     "WPS Registrar\n", __func__);
			os_free(wps);
			return -1;
		}
	}
#endif

	wpa_s->wps = wps;

	return 0;
}
#endif	/* CONFIG_WPS */

#ifdef CONFIG_WPS_ER
static void wpas_wps_nfc_clear(struct wps_context *wps)
{
	wps->ap_nfc_dev_pw_id = 0;
	wpabuf_free(wps->ap_nfc_dh_pubkey);
	wps->ap_nfc_dh_pubkey = NULL;
	wpabuf_free(wps->ap_nfc_dh_privkey);
	wps->ap_nfc_dh_privkey = NULL;
	wpabuf_free(wps->ap_nfc_dev_pw);
	wps->ap_nfc_dev_pw = NULL;
}
#endif /* CONFIG_WPS_ER */

#ifdef CONFIG_WPS
void wpas_wps_deinit(struct wpa_supplicant *wpa_s)
{
	wpas_wps_assoc_with_cred_cancel(wpa_s);
	da16x_eloop_cancel_timeout(wpas_wps_timeout, wpa_s, NULL);
	da16x_eloop_cancel_timeout(wpas_wps_clear_timeout, wpa_s, NULL);
	da16x_eloop_cancel_timeout(wpas_wps_reenable_networks_cb, wpa_s, NULL);

	wpas_wps_clear_ap_info(wpa_s);

#ifdef CONFIG_P2P
	da16x_eloop_cancel_timeout(wpas_p2p_pbc_overlap_cb, wpa_s, NULL);
#endif /* CONFIG_P2P */

	if (wpa_s->wps == NULL)
		return;

#ifdef CONFIG_WPS_ER
	wps_er_deinit(wpa_s->wps_er, NULL, NULL);
	wpa_s->wps_er = NULL;
	wpas_wps_nfc_clear(wpa_s->wps);
#endif /* CONFIG_WPS_ER */

//[[ trinity_0150108_BEGIN -- tmp
#ifdef CONFIG_WPS_REGISTRAR
	wps_registrar_deinit(wpa_s->wps->registrar);
#endif
//]] trinity_0150108_END -- tmp
	wpabuf_free(wpa_s->wps->dh_pubkey);
	wpabuf_free(wpa_s->wps->dh_privkey);
	wpabuf_free(wpa_s->wps->dev.vendor_ext_m1);
	os_free(wpa_s->wps->network_key);
	os_free(wpa_s->wps);
	wpa_s->wps = NULL;
}

int wpas_wps_ssid_bss_match(struct wpa_supplicant *wpa_s,
			    struct wpa_ssid *ssid, struct wpa_bss *bss)
{
	struct wpabuf *wps_ie;

	if (!(ssid->key_mgmt & WPA_KEY_MGMT_WPS))
		return -1;

	wps_ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
	if (eap_is_wps_pbc_enrollee(&ssid->eap)) {
		if (!wps_ie) {
			da16x_wps_prt("     [%s]    skip - non-WPS AP\n",
				     __func__);
			return 0;
		}

		if (!wps_is_selected_pbc_registrar(wps_ie)) {
			da16x_wps_prt("     [%s]    skip - WPS AP "
				   "without active PBC Registrar\n", __func__);
			wpabuf_free(wps_ie);
			return 0;
		}

		/* TODO: overlap detection */
		da16x_wps_prt("     [%s]    selected based on WPS IE "
			   "(Active PBC)\n", __func__);
		wpabuf_free(wps_ie);
		return 1;
	}

	if (eap_is_wps_pin_enrollee(&ssid->eap)) {
		if (!wps_ie) {
			da16x_wps_prt("     [%s]    skip - non-WPS AP\n", __func__);
			return 0;
		}

		/*
		 * Start with WPS APs that advertise our address as an
		 * authorized MAC (v2.0) or active PIN Registrar (v1.0) and
		 * allow any WPS AP after couple of scans since some APs do not
		 * set Selected Registrar attribute properly when using
		 * external Registrar.
		 */
		if (!wps_is_addr_authorized(wps_ie, wpa_s->own_addr, 1)) {
			if (wpa_s->scan_runs < WPS_PIN_SCAN_IGNORE_SEL_REG) {
				da16x_wps_prt("     [%s]    skip - WPS AP "
					     "without active PIN Registrar\n",
					     __func__);
				wpabuf_free(wps_ie);
				return 0;
			}
			da16x_wps_prt("     [%s]    selected based on WPS IE\n",
				     __func__);
		} else {
			da16x_wps_prt("     [%s]    selected based on WPS IE "
				     "(Authorized MAC or Active PIN)\n",
				     __func__);
		}
		wpabuf_free(wps_ie);
		return 1;
	}

	if (wps_ie) {
		da16x_wps_prt("     [%s]    selected based on WPS IE\n",
			     __func__);
		wpabuf_free(wps_ie);
		return 1;
	}

	return -1;
}

int wpas_wps_ssid_wildcard_ok(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid,
			      struct wpa_bss *bss)
{
	struct wpabuf *wps_ie = NULL;
	int ret = 0;

	if (eap_is_wps_pbc_enrollee(&ssid->eap)) {
		wps_ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
		if (wps_ie && wps_is_selected_pbc_registrar(wps_ie)) {
			/* allow wildcard SSID for WPS PBC */
			ret = 1;
		}
	} else if (eap_is_wps_pin_enrollee(&ssid->eap)) {
		wps_ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
		if (wps_ie &&
		    (wps_is_addr_authorized(wps_ie, wpa_s->own_addr, 1) ||
		     wpa_s->scan_runs >= WPS_PIN_SCAN_IGNORE_SEL_REG)) {
			/* allow wildcard SSID for WPS PIN */
			ret = 1;
		}
	}

	if (!ret && ssid->bssid_set &&
	    os_memcmp(ssid->bssid, bss->bssid, ETH_ALEN) == 0) {
		/* allow wildcard SSID due to hardcoded BSSID match */
		ret = 1;
	}

#ifdef CONFIG_WPS_STRICT
	if (wps_ie) {
		if (wps_validate_beacon_probe_resp(wps_ie, bss->beacon_ie_len >
						   0, bss->bssid) < 0)
			ret = 0;
		if (bss->beacon_ie_len) {
			struct wpabuf *bcn_wps;
			bcn_wps = wpa_bss_get_vendor_ie_multi_beacon(
				bss, WPS_IE_VENDOR_TYPE);
			if (bcn_wps == NULL) {
				da16x_wps_prt("     [%s] WPS: Mandatory WPS IE "
					     "missing from AP Beacon\n",
					     __func__);
				ret = 0;
			} else {
				if (wps_validate_beacon(wps_ie) < 0)
					ret = 0;
				wpabuf_free(bcn_wps);
			}
		}
	}
#endif /* CONFIG_WPS_STRICT */

	wpabuf_free(wps_ie);

	return ret;
}

int wpas_wps_scan_pbc_overlap(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *selected, struct wpa_ssid *ssid)
{
	const u8 *sel_uuid, *uuid;
	struct wpabuf *wps_ie;
	int ret = 0;
	struct wpa_bss *bss;

	if (!eap_is_wps_pbc_enrollee(&ssid->eap))
		return 0;

	da16x_wps_prt("     [%s] WPS: Check whether PBC session overlap is "
		   "present in scan results; selected BSSID " MACSTR, __func__,
		   MAC2STR(selected->bssid));

	/* Make sure that only one AP is in active PBC mode */
	wps_ie = wpa_bss_get_vendor_ie_multi(selected, WPS_IE_VENDOR_TYPE);
	if (wps_ie) {
		sel_uuid = wps_get_uuid_e(wps_ie);
#ifdef ENABLE_WPS_DBG
		da16x_dump("WPS: UUID of the selected BSS",
			    sel_uuid, UUID_LEN);
#endif
	} else {
		da16x_wps_prt("     [%s] WPS: Selected BSS does not include "
			   "WPS IE?!\n", __func__);
		sel_uuid = NULL;
	}

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		struct wpabuf *ie;
		if (bss == selected)
			continue;
		ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
		if (!ie)
			continue;
		if (!wps_is_selected_pbc_registrar(ie)) {
			wpabuf_free(ie);
			continue;
		}
		da16x_wps_prt("     [%s] WPS: Another BSS in active PBC mode: "
			   MACSTR, MAC2STR(bss->bssid), __func__);
		uuid = wps_get_uuid_e(ie);
#ifdef ENABLE_WPS_DBG
		da16x_dump("WPS: UUID of the other BSS",
			    uuid, UUID_LEN);
#endif
		if (sel_uuid == NULL || uuid == NULL ||
		    os_memcmp(sel_uuid, uuid, UUID_LEN) != 0) {
			ret = 1; /* PBC overlap */
			da16x_wps_prt("               [WPA_MSG]\n" "WPS: PBC overlap detected: "
				MACSTR " and " MACSTR,
				MAC2STR(selected->bssid),
				MAC2STR(bss->bssid));
			wpabuf_free(ie);
			break;
		}

		/* TODO: verify that this is reasonable dual-band situation */

		wpabuf_free(ie);
	}

	wpabuf_free(wps_ie);

	return ret;
}


void wpas_wps_notify_scan_results(struct wpa_supplicant *wpa_s)
{
	struct wpa_bss *bss;
	unsigned int pbc = 0, auth = 0, pin = 0, wps = 0;

	if (wpa_s->disconnected || wpa_s->wpa_state >= WPA_ASSOCIATED)
		return;

	dl_list_for_each(bss, &wpa_s->bss, struct wpa_bss, list) {
		struct wpabuf *ie;
		ie = wpa_bss_get_vendor_ie_multi(bss, WPS_IE_VENDOR_TYPE);
		if (!ie)
			continue;
		if (wps_is_selected_pbc_registrar(ie))
			pbc++;
		else if (wps_is_addr_authorized(ie, wpa_s->own_addr, 0))
			auth++;
		else if (wps_is_selected_pin_registrar(ie))
			pin++;
		else
			wps++;
		wpabuf_free(ie);
	}

	if (pbc) {
		da16x_wps_prt("[WPA_MSG]\n"  WPS_EVENT_AP_AVAILABLE_PBC);
	} else if (auth) {
		da16x_wps_prt("[WPA_MSG]\n"  WPS_EVENT_AP_AVAILABLE_AUTH);
	} else if (pin) {
		da16x_wps_prt("[WPA_MSG]\n"  WPS_EVENT_AP_AVAILABLE_PIN);
	} else if (wps) {
		da16x_wps_prt("[WPA_MSG]\n"  WPS_EVENT_AP_AVAILABLE);
	}
}


int wpas_wps_searching(struct wpa_supplicant *wpa_s)
{
	struct wpa_ssid *ssid;

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if ((ssid->key_mgmt & WPA_KEY_MGMT_WPS) && !ssid->disabled)
			return 1;
	}

	return 0;
}
#endif

int wpas_wps_scan_result_text(const u8 *ies, size_t ies_len, char *buf,
			      char *end)
{
	struct wpabuf *wps_ie;
	int ret;

	wps_ie = ieee802_11_vendor_ie_concat(ies, ies_len, WPS_DEV_OUI_WFA);
	if (wps_ie == NULL)
		return 0;

	ret = wps_attr_text(wps_ie, buf, end);
	wpabuf_free(wps_ie);
	return ret;
}

#ifdef CONFIG_WPS_ER
int wpas_wps_er_start(struct wpa_supplicant *wpa_s, const char *filter)
{
	if (wpa_s->wps_er) {
		wps_er_refresh(wpa_s->wps_er);
		return 0;
	}
	wpa_s->wps_er = wps_er_init(wpa_s->wps, wpa_s->ifname, filter);
	if (wpa_s->wps_er == NULL)
		return -1;
	return 0;
}

int wpas_wps_er_stop(struct wpa_supplicant *wpa_s)
{
	wps_er_deinit(wpa_s->wps_er, NULL, NULL);
	wpa_s->wps_er = NULL;
	return 0;
}


int wpas_wps_er_add_pin(struct wpa_supplicant *wpa_s, const u8 *addr,
			const char *uuid, const char *pin)
{
	u8 u[UUID_LEN];
	const u8 *use_uuid = NULL;
	u8 addr_buf[ETH_ALEN];

	if (os_strcmp(uuid, "any") == 0) {
	} else if (uuid_str2bin(uuid, u) == 0) {
		use_uuid = u;
	} else if (hwaddr_aton(uuid, addr_buf) == 0) {
		use_uuid = wps_er_get_sta_uuid(wpa_s->wps_er, addr_buf);
		if (use_uuid == NULL)
			return -1;
	} else
		return -1;
	return wps_registrar_add_pin(wpa_s->wps->registrar, addr,
				     use_uuid,
				     (const u8 *) pin, os_strlen(pin), 300);
}


int wpas_wps_er_pbc(struct wpa_supplicant *wpa_s, const char *uuid)
{
	u8 u[UUID_LEN], *use_uuid = NULL;
	u8 addr[ETH_ALEN], *use_addr = NULL;

	if (uuid_str2bin(uuid, u) == 0)
		use_uuid = u;
	else if (hwaddr_aton(uuid, addr) == 0)
		use_addr = addr;
	else
		return -1;
	return wps_er_pbc(wpa_s->wps_er, use_uuid, use_addr);
}


int wpas_wps_er_learn(struct wpa_supplicant *wpa_s, const char *uuid,
		      const char *pin)
{
	u8 u[UUID_LEN], *use_uuid = NULL;
	u8 addr[ETH_ALEN], *use_addr = NULL;

	if (uuid_str2bin(uuid, u) == 0)
		use_uuid = u;
	else if (hwaddr_aton(uuid, addr) == 0)
		use_addr = addr;
	else
		return -1;

	return wps_er_learn(wpa_s->wps_er, use_uuid, use_addr, (const u8 *) pin,
			    os_strlen(pin));
}


static int wpas_wps_network_to_cred(struct wpa_ssid *ssid,
				    struct wps_credential *cred)
{
	os_memset(cred, 0, sizeof(*cred));
	if (ssid->ssid_len > 32)
		return -1;
	os_memcpy(cred->ssid, ssid->ssid, ssid->ssid_len);
	cred->ssid_len = ssid->ssid_len;
	if (ssid->key_mgmt & WPA_KEY_MGMT_PSK) {
		cred->auth_type = (ssid->proto & WPA_PROTO_RSN) ?
			WPS_AUTH_WPA2PSK : WPS_AUTH_WPAPSK;
		if (ssid->pairwise_cipher & WPA_CIPHER_CCMP)
			cred->encr_type = WPS_ENCR_AES;
		else
			cred->encr_type = WPS_ENCR_TKIP;
		if (ssid->passphrase) {
			cred->key_len = os_strlen(ssid->passphrase);
			if (cred->key_len >= 64)
				return -1;
			os_memcpy(cred->key, ssid->passphrase, cred->key_len);
		} else if (ssid->psk_set) {
			cred->key_len = 32;
			os_memcpy(cred->key, ssid->psk, 32);
		} else
			return -1;
	} else {
		cred->auth_type = WPS_AUTH_OPEN;
		cred->encr_type = WPS_ENCR_NONE;
	}

	return 0;
}


int wpas_wps_er_set_config(struct wpa_supplicant *wpa_s, const char *uuid,
			   int id)
{
	u8 u[UUID_LEN], *use_uuid = NULL;
	u8 addr[ETH_ALEN], *use_addr = NULL;
	struct wpa_ssid *ssid;
	struct wps_credential cred;

	if (uuid_str2bin(uuid, u) == 0)
		use_uuid = u;
	else if (hwaddr_aton(uuid, addr) == 0)
		use_addr = addr;
	else
		return -1;
	ssid = wpa_config_get_network(wpa_s->conf, id);
	if (ssid == NULL || ssid->ssid == NULL)
		return -1;

	if (wpas_wps_network_to_cred(ssid, &cred) < 0)
		return -1;
	return wps_er_set_config(wpa_s->wps_er, use_uuid, use_addr, &cred);
}


int wpas_wps_er_config(struct wpa_supplicant *wpa_s, const char *uuid,
		       const char *pin, struct wps_new_ap_settings *settings)
{
	u8 u[UUID_LEN], *use_uuid = NULL;
	u8 addr[ETH_ALEN], *use_addr = NULL;
	struct wps_credential cred;
	size_t len;

	if (uuid_str2bin(uuid, u) == 0)
		use_uuid = u;
	else if (hwaddr_aton(uuid, addr) == 0)
		use_addr = addr;
	else
		return -1;
	if (settings->ssid_hex == NULL || settings->auth == NULL ||
	    settings->encr == NULL || settings->key_hex == NULL)
		return -1;

	os_memset(&cred, 0, sizeof(cred));
	len = os_strlen(settings->ssid_hex);
	if ((len & 1) || len > 2 * sizeof(cred.ssid) ||
	    hexstr2bin(settings->ssid_hex, cred.ssid, len / 2))
		return -1;
	cred.ssid_len = len / 2;

	len = os_strlen(settings->key_hex);
	if ((len & 1) || len > 2 * sizeof(cred.key) ||
	    hexstr2bin(settings->key_hex, cred.key, len / 2))
		return -1;
	cred.key_len = len / 2;

	if (os_strcmp(settings->auth, "OPEN") == 0)
		cred.auth_type = WPS_AUTH_OPEN;
	else if (os_strcmp(settings->auth, "WPAPSK") == 0)
		cred.auth_type = WPS_AUTH_WPAPSK;
	else if (os_strcmp(settings->auth, "WPA2PSK") == 0)
		cred.auth_type = WPS_AUTH_WPA2PSK;
	else
		return -1;

	if (os_strcmp(settings->encr, "NONE") == 0)
		cred.encr_type = WPS_ENCR_NONE;
#ifdef CONFIG_TESTING_OPTIONS
	else if (os_strcmp(settings->encr, "WEP") == 0)
		cred.encr_type = WPS_ENCR_WEP;
#endif /* CONFIG_TESTING_OPTIONS */
	else if (os_strcmp(settings->encr, "TKIP") == 0)
		cred.encr_type = WPS_ENCR_TKIP;
	else if (os_strcmp(settings->encr, "CCMP") == 0)
		cred.encr_type = WPS_ENCR_AES;
	else
		return -1;

	return wps_er_config(wpa_s->wps_er, use_uuid, use_addr,
			     (const u8 *) pin, os_strlen(pin), &cred);
}

static int callbacks_pending = 0;

static void wpas_wps_terminate_cb(void *ctx)
{
	da16x_wps_prt("     [%s] WPS ER: Terminated\n", __func__);
	if (--callbacks_pending <= 0)
		eloop_terminate();
}
#endif /* CONFIG_WPS_ER */


int wpas_wps_terminate_pending(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_WPS_ER
	if (wpa_s->wps_er) {
		callbacks_pending++;
		wps_er_deinit(wpa_s->wps_er, wpas_wps_terminate_cb, wpa_s);
		wpa_s->wps_er = NULL;
		return 1;
	}
#endif /* CONFIG_WPS_ER */
	return 0;
}


void wpas_wps_update_config(struct wpa_supplicant *wpa_s)
{
	struct wps_context *wps = wpa_s->wps;

	if (wps == NULL)
		return;

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_CONFIG_METHODS) {
		wps->config_methods = wps_config_methods_str2bin(
			wpa_s->conf->config_methods);
		if ((wps->config_methods &
		     (WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) ==
		    (WPS_CONFIG_DISPLAY | WPS_CONFIG_LABEL)) {
			da16x_wps_prt("     [%s] WPS: Both Label and Display "
				   "config methods are not allowed at the "
				   "same time\n", __func__);
			wps->config_methods &= ~WPS_CONFIG_LABEL;
		}
	}
	wps->config_methods = wps_fix_config_methods(wps->config_methods);
	wps->dev.config_methods = wps->config_methods;

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_DEVICE_TYPE)
		os_memcpy(wps->dev.pri_dev_type, wpa_s->conf->device_type,
			  WPS_DEV_TYPE_LEN);

#ifdef	CONFIG_SUPP27_WPS_2ND_DEV
	if (wpa_s->conf->changed_parameters & CFG_CHANGED_SEC_DEVICE_TYPE) {
		wps->dev.num_sec_dev_types = wpa_s->conf->num_sec_device_types;
		os_memcpy(wps->dev.sec_dev_type, wpa_s->conf->sec_device_type,
			  wps->dev.num_sec_dev_types * WPS_DEV_TYPE_LEN);
	}
#endif	/* CONFIG_SUPP27_WPS_2ND_DEV */

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_VENDOR_EXTENSION)
		wpas_wps_set_vendor_ext_m1(wpa_s, wps);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_OS_VERSION)
		wps->dev.os_version = WPA_GET_BE32(wpa_s->conf->os_version);

	if (wpa_s->conf->changed_parameters & CFG_CHANGED_UUID)
		wpas_wps_set_uuid(wpa_s, wps);

#if 0 //20150129_trinity
	if (wpa_s->conf->changed_parameters &
	    (CFG_CHANGED_DEVICE_NAME | CFG_CHANGED_WPS_STRING)) {
#endif
		/* Update pointers to make sure they refer current values */
		wps->dev.device_name = wpa_s->conf->device_name;
		wps->dev.manufacturer = wpa_s->conf->manufacturer;
		wps->dev.model_name = wpa_s->conf->model_name;
		wps->dev.model_number = wpa_s->conf->model_number;
		wps->dev.serial_number = wpa_s->conf->serial_number;
#if 0 //20150129_trinity
	}
#endif
}


#ifdef ENABLE_WPS_DBG
static void wpas_wps_dump_ap_info(struct wpa_supplicant *wpa_s)
{
	size_t i;
	struct os_reltime now;

	if (wpa_s->wps_ap == NULL)
		return;

	os_get_reltime(&now);

	for (i = 0; i < wpa_s->num_wps_ap; i++) {
		struct wps_ap_info *ap = &wpa_s->wps_ap[i];
		struct wpa_blacklist *e = wpa_blacklist_get(wpa_s, ap->bssid);

		da16x_wps_prt("     [%s] WPS: AP[%d] " MACSTR " type=%d "
			   "tries=%d last_attempt=%d sec ago blacklist=%d\n\n", __func__,
			   (int) i, MAC2STR(ap->bssid), ap->type, ap->tries,
			   ap->last_attempt.sec > 0 ?
			   (int) now.sec - (int) ap->last_attempt.sec : -1,
			   e ? e->count : 0);
	}
}
#endif

static struct wps_ap_info * wpas_wps_get_ap_info(struct wpa_supplicant *wpa_s,
						 const u8 *bssid)
{
	size_t i;

	if (wpa_s->wps_ap == NULL)
		return NULL;

	for (i = 0; i < wpa_s->num_wps_ap; i++) {
		struct wps_ap_info *ap = &wpa_s->wps_ap[i];
		if (os_memcmp(ap->bssid, bssid, ETH_ALEN) == 0)
			return ap;
	}

	return NULL;
}


static void wpas_wps_update_ap_info_bss(struct wpa_supplicant *wpa_s,
					struct wpa_scan_res *res)
{
	struct wpabuf *wps;
	enum wps_ap_info_type type;
	struct wps_ap_info *ap;
	int r;

	if (wpa_scan_get_vendor_ie(res, WPS_IE_VENDOR_TYPE) == NULL)
		return;

	wps = wpa_scan_get_vendor_ie_multi(res, WPS_IE_VENDOR_TYPE);
	if (wps == NULL)
		return;

	r = wps_is_addr_authorized(wps, wpa_s->own_addr, 1);
	if (r == 2)
		type = WPS_AP_SEL_REG_OUR;
	else if (r == 1)
		type = WPS_AP_SEL_REG;
	else
		type = WPS_AP_NOT_SEL_REG;

	wpabuf_free(wps);

	ap = wpas_wps_get_ap_info(wpa_s, res->bssid);
	if (ap) {
		if (ap->type != type) {
			da16x_wps_prt("     [%s] WPS: AP " MACSTR
				   " changed type %d -> %d\n\n", __func__,
				   MAC2STR(res->bssid), ap->type, type);
			ap->type = type;
			if (type != WPS_AP_NOT_SEL_REG)
				wpa_blacklist_del(wpa_s, ap->bssid);
		}
		return;
	}

	ap = os_realloc_array(wpa_s->wps_ap, wpa_s->num_wps_ap + 1,
			      sizeof(struct wps_ap_info));
	if (ap == NULL)
		return;

	wpa_s->wps_ap = ap;
	ap = &wpa_s->wps_ap[wpa_s->num_wps_ap];
	wpa_s->num_wps_ap++;

	os_memset(ap, 0, sizeof(*ap));
	os_memcpy(ap->bssid, res->bssid, ETH_ALEN);
	ap->type = type;
	da16x_wps_prt("     [%s] WPS: AP " MACSTR " type %d added\n\n", __func__,
		   MAC2STR(ap->bssid), ap->type);
}

void wpas_wps_update_ap_info(struct wpa_supplicant *wpa_s,
			     struct wpa_scan_results *scan_res)
{
	size_t i;

	for (i = 0; i < scan_res->num; i++)
		wpas_wps_update_ap_info_bss(wpa_s, scan_res->res[i]);

#ifdef ENABLE_WPS_DBG
	wpas_wps_dump_ap_info(wpa_s);
#endif
}

void wpas_wps_notify_assoc(struct wpa_supplicant *wpa_s, const u8 *bssid)
{
	struct wps_ap_info *ap;

	wpa_s->after_wps = 0;

	if (!wpa_s->wps_ap_iter)
		return;
	ap = wpas_wps_get_ap_info(wpa_s, bssid);
	if (ap == NULL)
		return;
	ap->tries++;
	os_get_reltime(&ap->last_attempt);
}
#endif	/* CONFIG_WPS */

u8 wpa_supp_wps_in_use(void)
{
#ifdef CONFIG_WPS
	return is_wps_in_use;
#else
	return 0;
#endif /* CONFIG_WPS */
}


/* EOF */
