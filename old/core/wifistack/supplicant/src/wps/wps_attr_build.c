/*
 * Wi-Fi Protected Setup - attribute building
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */


#include "includes.h"

#ifdef	CONFIG_WPS

#include "supp_common.h"
#include "crypto/aes_wrap.h"
#include "crypto/crypto.h"
#include "crypto/dh_group5.h"
#include "crypto/sha256.h"
#include "crypto/random.h"
#include "common/ieee802_11_defs.h"
#include "wps_i.h"

int wps_build_public_key(struct wps_data *wps, struct wpabuf *msg)
{
	struct wpabuf *pubkey = NULL;

	da16x_wps_prt("[%s] WPS:  * Public Key\n", __func__);
	wpabuf_free(wps->dh_privkey);
	wps->dh_privkey = NULL;
	
	if (wps->dev_pw_id != DEV_PW_DEFAULT && wps->wps->dh_privkey &&
	    wps->wps->dh_ctx) {
		da16x_wps_prt("[%s] WPS: Using pre-configured DH keys\n",
						__func__);
		if (wps->wps->dh_pubkey == NULL) {
			da16x_wps_prt("[%s] WPS: wps->wps->dh_pubkey "
				"== NULL\n", __func__);
			return -1;
		}
		wps->dh_privkey = wpabuf_dup(wps->wps->dh_privkey);
		wps->dh_ctx = wps->wps->dh_ctx;
		wps->wps->dh_ctx = NULL;
		pubkey = wpabuf_dup(wps->wps->dh_pubkey);
	} else {
		da16x_wps_prt("[%s] WPS: Generate new DH keys\n", __func__);
		dh5_free(wps->dh_ctx);
		wps->dh_ctx = dh5_init(&wps->dh_privkey, &pubkey);
		pubkey = wpabuf_zeropad(pubkey, 192);
	}
	if (wps->dh_ctx == NULL || wps->dh_privkey == NULL || pubkey == NULL) {
		da16x_wps_prt("[%s] WPS: Failed to initialize "
			   "Diffie-Hellman handshake\n", __func__);
		wpabuf_free(pubkey);
		return -1;
	}

#ifdef ENABLE_WPS_DBG
	da16x_buf_dump("WPS: DH Private Key", wps->dh_privkey);
	da16x_buf_dump("WPS: DH own Public Key", pubkey);
#endif

	wpabuf_put_be16(msg, ATTR_PUBLIC_KEY);
	wpabuf_put_be16(msg, wpabuf_len(pubkey));
	wpabuf_put_buf(msg, pubkey);

	if (wps->registrar) {
		wpabuf_free(wps->dh_pubkey_r);
		wps->dh_pubkey_r = pubkey;
	} else {
		wpabuf_free(wps->dh_pubkey_e);
		wps->dh_pubkey_e = pubkey;
	}

	return 0;
}


int wps_build_req_type(struct wpabuf *msg, enum wps_request_type type)
{
	da16x_wps_prt("[%s] WPS:  * Request Type\n", __func__);
	wpabuf_put_be16(msg, ATTR_REQUEST_TYPE);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, type);
	return 0;
}


int wps_build_resp_type(struct wpabuf *msg, enum wps_response_type type)
{
	da16x_wps_prt("[%s] WPS:  * Response Type (%d)\n", __func__, type);
	wpabuf_put_be16(msg, ATTR_RESPONSE_TYPE);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, type);
	return 0;
}


int wps_build_config_methods(struct wpabuf *msg, u16 methods)
{
	da16x_wps_prt("[%s] WPS:  * Config Methods (%x)\n", __func__, methods);
	wpabuf_put_be16(msg, ATTR_CONFIG_METHODS);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, methods);
	return 0;
}


int wps_build_uuid_e(struct wpabuf *msg, const u8 *uuid)
{
	if (wpabuf_tailroom(msg) < 4 + WPS_UUID_LEN)
		return -1;
	da16x_wps_prt("[%s] WPS:  * UUID-E\n", __func__);
	wpabuf_put_be16(msg, ATTR_UUID_E);
	wpabuf_put_be16(msg, WPS_UUID_LEN);
	wpabuf_put_data(msg, uuid, WPS_UUID_LEN);
	return 0;
}


int wps_build_dev_password_id(struct wpabuf *msg, u16 id)
{
	da16x_wps_prt("[%s] WPS:  * Device Password ID (%d)\n", __func__, id);
	wpabuf_put_be16(msg, ATTR_DEV_PASSWORD_ID);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, id);
	return 0;
}


int wps_build_config_error(struct wpabuf *msg, u16 err)
{
	da16x_wps_prt("[%s] WPS:  * Configuration Error (%d)\n", __func__, err);
	wpabuf_put_be16(msg, ATTR_CONFIG_ERROR);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, err);
	return 0;
}


int wps_build_authenticator(struct wps_data *wps, struct wpabuf *msg)
{
	u8 hash[SHA256_MAC_LEN];
	const u8 *addr[2];
	size_t len[2];

	if (wps->last_msg == NULL) {
		da16x_wps_prt("[%s] WPS: Last message not available for "
			   "building authenticator\n", __func__);
		return -1;
	}

	/* Authenticator = HMAC-SHA256_AuthKey(M_prev || M_curr*)
	 * (M_curr* is M_curr without the Authenticator attribute)
	 */
	addr[0] = wpabuf_head(wps->last_msg);
	len[0] = wpabuf_len(wps->last_msg);
	addr[1] = wpabuf_head(msg);
	len[1] = wpabuf_len(msg);
	hmac_sha256_vector(wps->authkey, WPS_AUTHKEY_LEN, 2, addr, len, hash);

	da16x_wps_prt("[%s] WPS:  * Authenticator\n", __func__);
	wpabuf_put_be16(msg, ATTR_AUTHENTICATOR);
	wpabuf_put_be16(msg, WPS_AUTHENTICATOR_LEN);
	wpabuf_put_data(msg, hash, WPS_AUTHENTICATOR_LEN);

	return 0;
}


int wps_build_version(struct wpabuf *msg)
{
	/*
	 * Note: This attribute is deprecated and set to hardcoded 0x10 for
	 * backwards compatibility reasons. The real version negotiation is
	 * done with Version2.
	 */
	if (wpabuf_tailroom(msg) < 5)
		return -1;
	da16x_wps_prt("[%s] WPS:  * Version (hardcoded 0x10)\n", __func__);
	wpabuf_put_be16(msg, ATTR_VERSION);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, 0x10);
	return 0;
}


int wps_build_wfa_ext(struct wpabuf *msg, int req_to_enroll,
		      const u8 *auth_macs, size_t auth_macs_count)
{
	u8 *len;

#ifdef CONFIG_WPS_TESTING
	if (WPS_VERSION == 0x10)
		return 0;
#endif /* CONFIG_WPS_TESTING */

	if (wpabuf_tailroom(msg) <
	    7 + 3 + (req_to_enroll ? 3 : 0) +
	    (auth_macs ? 2 + auth_macs_count * ETH_ALEN : 0))
		return -1;
	wpabuf_put_be16(msg, ATTR_VENDOR_EXT);
	len = wpabuf_put(msg, 2); /* to be filled */
	wpabuf_put_be24(msg, WPS_VENDOR_ID_WFA);

	da16x_wps_prt("[%s] WPS:  * Version2 (0x%x)\n",
					__func__, WPS_VERSION);
	wpabuf_put_u8(msg, WFA_ELEM_VERSION2);
	wpabuf_put_u8(msg, 1);
	wpabuf_put_u8(msg, WPS_VERSION);

	if (req_to_enroll) {
		da16x_wps_prt("[%s] WPS:  * Request to Enroll (1)\n",
							__func__);
		wpabuf_put_u8(msg, WFA_ELEM_REQUEST_TO_ENROLL);
		wpabuf_put_u8(msg, 1);
		wpabuf_put_u8(msg, 1);
	}

	if (auth_macs && auth_macs_count) {
		size_t i;
		da16x_wps_prt("[%s] WPS:  * AuthorizedMACs (count=%d)\n",
							__func__,
			   (int) auth_macs_count);
		wpabuf_put_u8(msg, WFA_ELEM_AUTHORIZEDMACS);
		wpabuf_put_u8(msg, auth_macs_count * ETH_ALEN);
		wpabuf_put_data(msg, auth_macs, auth_macs_count * ETH_ALEN);
		for (i = 0; i < auth_macs_count; i++)
			da16x_wps_prt("[%s] WPS:    AuthorizedMAC: "
					MACSTR "\n", __func__,
				   MAC2STR(&auth_macs[i * ETH_ALEN]));
	}

	WPA_PUT_BE16(len, (u8 *) wpabuf_put(msg, 0) - len - 2);

#ifdef CONFIG_WPS_TESTING
	if (WPS_VERSION > 0x20) {
		if (wpabuf_tailroom(msg) < 5)
			return -1;
		da16x_wps_prt("[%s] WPS:  * Extensibility Testing - extra "
			   "attribute\n", __func__);
		wpabuf_put_be16(msg, ATTR_EXTENSIBILITY_TEST);
		wpabuf_put_be16(msg, 1);
		wpabuf_put_u8(msg, 42);
	}
#endif /* CONFIG_WPS_TESTING */
	return 0;
}


int wps_build_msg_type(struct wpabuf *msg, enum wps_msg_type msg_type)
{
	da16x_wps_prt("[%s] WPS:  * Message Type (%d)\n", __func__, msg_type);
	wpabuf_put_be16(msg, ATTR_MSG_TYPE);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, msg_type);
	return 0;
}


int wps_build_enrollee_nonce(struct wps_data *wps, struct wpabuf *msg)
{
	da16x_wps_prt("[%s] WPS:  * Enrollee Nonce\n", __func__);
	wpabuf_put_be16(msg, ATTR_ENROLLEE_NONCE);
	wpabuf_put_be16(msg, WPS_NONCE_LEN);
	wpabuf_put_data(msg, wps->nonce_e, WPS_NONCE_LEN);
	return 0;
}


int wps_build_registrar_nonce(struct wps_data *wps, struct wpabuf *msg)
{
	da16x_wps_prt("[%s] WPS:  * Registrar Nonce\n", __func__);
	wpabuf_put_be16(msg, ATTR_REGISTRAR_NONCE);
	wpabuf_put_be16(msg, WPS_NONCE_LEN);
	wpabuf_put_data(msg, wps->nonce_r, WPS_NONCE_LEN);
	return 0;
}


int wps_build_auth_type_flags(struct wps_data *wps, struct wpabuf *msg)
{
	u16 auth_types = WPS_AUTH_TYPES;
	/* WPA/WPA2-Enterprise enrollment not supported through WPS */
	auth_types &= ~WPS_AUTH_WPA;
	auth_types &= ~WPS_AUTH_WPA2;
	auth_types &= ~WPS_AUTH_SHARED;
	da16x_wps_prt("[%s] WPS:  * Authentication Type Flags\n", __func__);
	wpabuf_put_be16(msg, ATTR_AUTH_TYPE_FLAGS);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, auth_types);
	return 0;
}


int wps_build_encr_type_flags(struct wps_data *wps, struct wpabuf *msg)
{
	u16 encr_types = WPS_ENCR_TYPES;
	encr_types &= ~WPS_ENCR_WEP;
	da16x_wps_prt("[%s] WPS:  * Encryption Type Flags\n", __func__);
	wpabuf_put_be16(msg, ATTR_ENCR_TYPE_FLAGS);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, encr_types);
	return 0;
}


int wps_build_conn_type_flags(struct wps_data *wps, struct wpabuf *msg)
{
	da16x_wps_prt("[%s] WPS:  * Connection Type Flags\n", __func__);
	wpabuf_put_be16(msg, ATTR_CONN_TYPE_FLAGS);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, WPS_CONN_ESS);
	return 0;
}


int wps_build_assoc_state(struct wps_data *wps, struct wpabuf *msg)
{
	da16x_wps_prt("[%s] WPS:  * Association State\n", __func__);
	wpabuf_put_be16(msg, ATTR_ASSOC_STATE);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, WPS_ASSOC_NOT_ASSOC);
	return 0;
}


int wps_build_key_wrap_auth(struct wps_data *wps, struct wpabuf *msg)
{
	u8 hash[SHA256_MAC_LEN];

	da16x_wps_prt("[%s] WPS:  * Key Wrap Authenticator\n", __func__);
	hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN, wpabuf_head(msg),
		    wpabuf_len(msg), hash);

	wpabuf_put_be16(msg, ATTR_KEY_WRAP_AUTH);
	wpabuf_put_be16(msg, WPS_KWA_LEN);
	wpabuf_put_data(msg, hash, WPS_KWA_LEN);
	return 0;
}


int wps_build_encr_settings(struct wps_data *wps, struct wpabuf *msg,
			    struct wpabuf *plain)
{
	size_t pad_len;
	const size_t block_size = 16;
	u8 *iv, *data;

	da16x_wps_prt("[%s] WPS:  * Encrypted Settings\n", __func__);

	/* PKCS#5 v2.0 pad */
	pad_len = block_size - wpabuf_len(plain) % block_size;
	os_memset(wpabuf_put(plain, pad_len), pad_len, pad_len);

	wpabuf_put_be16(msg, ATTR_ENCR_SETTINGS);
	wpabuf_put_be16(msg, block_size + wpabuf_len(plain));

	iv = wpabuf_put(msg, block_size);
	if (random_get_bytes(iv, block_size) < 0)
		return -1;

	data = wpabuf_put(msg, 0);
	wpabuf_put_buf(msg, plain);
	if (aes_128_cbc_encrypt(wps->keywrapkey, iv, data, wpabuf_len(plain)))
		return -1;

	return 0;
}


#ifdef CONFIG_WPS_OOB
int wps_build_oob_dev_pw(struct wpabuf *msg, u16 dev_pw_id,
			 const struct wpabuf *pubkey, const u8 *dev_pw,
			 size_t dev_pw_len)
{
	size_t hash_len;
	const u8 *addr[1];
	u8 pubkey_hash[WPS_HASH_LEN];

	da16x_wps_prt("[%s] WPS:  * OOB Device Password (dev_pw_id=%u)\n", __func__,
		   dev_pw_id);
	addr[0] = wpabuf_head(pubkey);
	hash_len = wpabuf_len(pubkey);
	sha256_vector(1, addr, &hash_len, pubkey_hash);
#ifdef CONFIG_WPS_TESTING
	if (wps_corrupt_pkhash) {
		da16x_dump("WPS: Real Public Key Hash",
			    pubkey_hash, WPS_OOB_PUBKEY_HASH_LEN);
		da16x_wps_prt("[%s] WPS: Testing - corrupt public key hash\n", __func__);
		pubkey_hash[WPS_OOB_PUBKEY_HASH_LEN - 2]++;
	}
#endif /* CONFIG_WPS_TESTING */

	wpabuf_put_be16(msg, ATTR_OOB_DEVICE_PASSWORD);
	wpabuf_put_be16(msg, WPS_OOB_PUBKEY_HASH_LEN + 2 + dev_pw_len);

#ifdef ENABLE_WPS_DBG
	da16x_dump("WPS: Public Key Hash",
		    pubkey_hash, WPS_OOB_PUBKEY_HASH_LEN);
#endif

	wpabuf_put_data(msg, pubkey_hash, WPS_OOB_PUBKEY_HASH_LEN);
	wpabuf_put_be16(msg, dev_pw_id);
	if (dev_pw) {
#ifdef ENABLE_WPS_DBG
		da16x_dump("WPS: OOB Device Password",
				dev_pw, dev_pw_len);
#endif
		wpabuf_put_data(msg, dev_pw, dev_pw_len);
	}

	return 0;
}
#endif /* CONFIG_WPS_OOB */


/* Encapsulate WPS IE data with one (or more, if needed) IE headers */
struct wpabuf * wps_ie_encapsulate(struct wpabuf *data)
{
	struct wpabuf *ie;
	const u8 *pos, *end;

	ie = wpabuf_alloc(wpabuf_len(data) + 100);
	if (ie == NULL) {
		wpabuf_free(data);
		return NULL;
	}

	pos = wpabuf_head(data);
	end = pos + wpabuf_len(data);

	while (end > pos) {
		size_t frag_len = end - pos;
		if (frag_len > 251)
			frag_len = 251;
		wpabuf_put_u8(ie, WLAN_EID_VENDOR_SPECIFIC);
		wpabuf_put_u8(ie, 4 + frag_len);
		wpabuf_put_be32(ie, WPS_DEV_OUI_WFA);
		wpabuf_put_data(ie, pos, frag_len);
		pos += frag_len;
	}

	wpabuf_free(data);

	return ie;
}


int wps_build_mac_addr(struct wpabuf *msg, const u8 *addr)
{
	da16x_wps_prt("[%s] WPS:  * MAC Address (" MACSTR ")\n", __func__,
		   MAC2STR(addr));
	wpabuf_put_be16(msg, ATTR_MAC_ADDR);
	wpabuf_put_be16(msg, ETH_ALEN);
	wpabuf_put_data(msg, addr, ETH_ALEN);
	return 0;
}


int wps_build_rf_bands_attr(struct wpabuf *msg, u8 rf_bands)
{
	da16x_wps_prt("[%s] WPS:  * RF Bands (%x)\n", __func__, rf_bands);
	wpabuf_put_be16(msg, ATTR_RF_BANDS);
	wpabuf_put_be16(msg, 1);
	wpabuf_put_u8(msg, rf_bands);
	return 0;
}


int wps_build_ap_channel(struct wpabuf *msg, u16 ap_channel)
{
	da16x_wps_prt("[%s] WPS:  * AP Channel (%u)\n", __func__, ap_channel);
	wpabuf_put_be16(msg, ATTR_AP_CHANNEL);
	wpabuf_put_be16(msg, 2);
	wpabuf_put_be16(msg, ap_channel);
	return 0;
}

#endif	/* CONFIG_WPS */

/* EOF */
