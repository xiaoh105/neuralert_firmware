/*
 * Wi-Fi Protected Setup - attribute processing
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_WPS

#include "supp_common.h"
#include "crypto/sha256.h"
#include "wps_i.h"


extern int hmac_sha256_vector(const u8 *key, size_t key_len, size_t num_elem,	\
		const u8 *addr[], const size_t *len, u8 *mac);
extern int hmac_sha256(const u8 *key, size_t key_len, const u8 *data,	\
		size_t data_len, u8 *mac);


int wps_process_authenticator(struct wps_data *wps, const u8 *authenticator,
			      const struct wpabuf *msg)
{
	u8 hash[SHA256_MAC_LEN];
	const u8 *addr[2];
	size_t len[2];

	if (authenticator == NULL) {
		da16x_wps_prt("     [%s] WPS: No Authenticator attribute "
			   "included\n", __func__);
		return -1;
	}

	if (wps->last_msg == NULL) {
		da16x_wps_prt("     [%s] WPS: Last message not available for "
			   "validating authenticator\n", __func__);
		return -1;
	}

	/* Authenticator = HMAC-SHA256_AuthKey(M_prev || M_curr*)
	 * (M_curr* is M_curr without the Authenticator attribute)
	 */
	addr[0] = wpabuf_head(wps->last_msg);
	len[0] = wpabuf_len(wps->last_msg);
	addr[1] = wpabuf_head(msg);
	len[1] = wpabuf_len(msg) - 4 - WPS_AUTHENTICATOR_LEN;
	hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 2, addr, len, hash);

	if (os_memcmp(hash, authenticator, WPS_AUTHENTICATOR_LEN) != 0) {
		da16x_wps_prt("     [%s] WPS: Incorrect Authenticator\n", __func__);
		return -1;
	}

	return 0;
}


int wps_process_key_wrap_auth(struct wps_data *wps, struct wpabuf *msg,
			      const u8 *key_wrap_auth)
{
	u8 hash[SHA256_MAC_LEN];
	const u8 *head;
	size_t len;

	if (key_wrap_auth == NULL) {
		da16x_wps_prt("     [%s] WPS: No KWA in decrypted attribute\n", __func__);
		return -1;
	}

	head = wpabuf_head(msg);
	len = wpabuf_len(msg) - 4 - WPS_KWA_LEN;
	if (head + len != key_wrap_auth - 4) {
		da16x_wps_prt("     [%s] WPS: KWA not in the end of the "
			   "decrypted attribute\n", __func__);
		return -1;
	}

	hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN, head, len, hash);
	if (os_memcmp(hash, key_wrap_auth, WPS_KWA_LEN) != 0) {
		da16x_wps_prt("     [%s] WPS: Invalid KWA\n", __func__);
		return -1;
	}

	return 0;
}


static int wps_process_cred_network_idx(struct wps_credential *cred,
					const u8 *idx)
{
	if (idx == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include "
			   "Network Index\n", __func__);
		return -1;
	}

	da16x_wps_prt("     [%s] WPS: Network Index: %d\n", __func__, *idx);

	return 0;
}


static int wps_process_cred_ssid(struct wps_credential *cred, const u8 *ssid,
				 size_t ssid_len)
{
	if (ssid == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include SSID\n", __func__);
		return -1;
	}

	/* Remove zero-padding since some Registrar implementations seem to use
	 * hardcoded 32-octet length for this attribute */
	while (ssid_len > 0 && ssid[ssid_len - 1] == 0)
		ssid_len--;

#ifdef ENABLE_WPS_DBG
	da16x_dump("WPS: SSID", ssid, ssid_len);
#endif

	if (ssid_len <= sizeof(cred->ssid)) {
		os_memcpy(cred->ssid, ssid, ssid_len);
		cred->ssid_len = ssid_len;
	}

	return 0;
}


static int wps_process_cred_auth_type(struct wps_credential *cred,
				      const u8 *auth_type)
{
	if (auth_type == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include "
			   "Authentication Type\n", __func__);
		return -1;
	}

	cred->auth_type = WPA_GET_BE16(auth_type);
	da16x_wps_prt("     [%s] WPS: Authentication Type: 0x%x\n", __func__,
		   cred->auth_type);

	return 0;
}


static int wps_process_cred_encr_type(struct wps_credential *cred,
				      const u8 *encr_type)
{
	if (encr_type == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include "
			   "Encryption Type\n", __func__);
		return -1;
	}

	cred->encr_type = WPA_GET_BE16(encr_type);
	da16x_wps_prt("     [%s] WPS: Encryption Type: 0x%x\n", __func__,
		   cred->encr_type);

	return 0;
}


static int wps_process_cred_network_key_idx(struct wps_credential *cred,
					    const u8 *key_idx)
{
	if (key_idx == NULL)
		return 0; /* optional attribute */

	da16x_wps_prt("     [%s] WPS: Network Key Index: %d\n", __func__, *key_idx);
	cred->key_idx = *key_idx;

	return 0;
}


static int wps_process_cred_network_key(struct wps_credential *cred,
					const u8 *key, size_t key_len)
{
	if (key == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include "
			   "Network Key\n", __func__);
		if (cred->auth_type == WPS_AUTH_OPEN &&
		    cred->encr_type == WPS_ENCR_NONE) {
			da16x_wps_prt("     [%s] WPS: Workaround - Allow "
				   "missing mandatory Network Key attribute "
				   "for open network\n", __func__);
			return 0;
		}
		return -1;
	}

#ifdef ENABLE_WPS_DBG
	da16x_dump("WPS: Network Key", key, key_len);
#endif

	if (key_len <= sizeof(cred->key)) {
		os_memcpy(cred->key, key, key_len);
		cred->key_len = key_len;
	}

	return 0;
}


static int wps_process_cred_mac_addr(struct wps_credential *cred,
				     const u8 *mac_addr)
{
	if (mac_addr == NULL) {
		da16x_wps_prt("     [%s] WPS: Credential did not include "
			   "MAC Address\n", __func__);
		return -1;
	}

	da16x_wps_prt("     [%s] WPS: MAC Address " MACSTR "\n", __func__, MAC2STR(mac_addr));
	os_memcpy(cred->mac_addr, mac_addr, ETH_ALEN);

	return 0;
}


static int wps_workaround_cred_key(struct wps_credential *cred)
{
	if (cred->auth_type & (WPS_AUTH_WPAPSK | WPS_AUTH_WPA2PSK) &&
	    cred->key_len > 8 && cred->key_len < 64 &&
	    cred->key[cred->key_len - 1] == 0) {
#ifdef CONFIG_WPS_STRICT
		da16x_wps_prt("     [%s] WPS: WPA/WPA2-Personal passphrase uses "
			   "forbidden NULL termination\n", __func__);
		wpa_hexdump_ascii_key(MSG_INFO, "WPS: Network Key",
				      cred->key, cred->key_len);
		return -1;
#else /* CONFIG_WPS_STRICT */
		/*
		 * A deployed external registrar is known to encode ASCII
		 * passphrases incorrectly. Remove the extra NULL termination
		 * to fix the encoding.
		 */
		da16x_wps_prt("     [%s] WPS: Workaround - remove NULL "
			   "termination from ASCII passphrase\n", __func__);
		cred->key_len--;
#endif /* CONFIG_WPS_STRICT */
	}
	return 0;
}


int wps_process_cred(struct wps_parse_attr *attr,
		     struct wps_credential *cred)
{
	da16x_wps_prt("     [%s] WPS: Process Credential\n", __func__);

	/* TODO: support multiple Network Keys */
	if (wps_process_cred_network_idx(cred, attr->network_idx) ||
	    wps_process_cred_ssid(cred, attr->ssid, attr->ssid_len) ||
	    wps_process_cred_auth_type(cred, attr->auth_type) ||
	    wps_process_cred_encr_type(cred, attr->encr_type) ||
	    wps_process_cred_network_key_idx(cred, attr->network_key_idx) ||
	    wps_process_cred_network_key(cred, attr->network_key,
					 attr->network_key_len) ||
	    wps_process_cred_mac_addr(cred, attr->mac_addr))
		return -1;

	return wps_workaround_cred_key(cred);
}


int wps_process_ap_settings(struct wps_parse_attr *attr,
			    struct wps_credential *cred)
{
	da16x_wps_prt("     [%s] WPS: Processing AP Settings\n", __func__);
	os_memset(cred, 0, sizeof(*cred));
	/* TODO: optional attributes New Password and Device Password ID */
	if (wps_process_cred_ssid(cred, attr->ssid, attr->ssid_len) ||
	    wps_process_cred_auth_type(cred, attr->auth_type) ||
	    wps_process_cred_encr_type(cred, attr->encr_type) ||
	    wps_process_cred_network_key_idx(cred, attr->network_key_idx) ||
	    wps_process_cred_network_key(cred, attr->network_key,
					 attr->network_key_len) ||
	    wps_process_cred_mac_addr(cred, attr->mac_addr))
		return -1;

	return wps_workaround_cred_key(cred);
}

#endif	/* CONFIG_WPS */

/* EOF */
