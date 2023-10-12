/*
 * Wi-Fi Protected Setup - attribute parsing
 * Copyright (c) 2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_WPS

#include "supp_common.h"
#include "wps_defs.h"
#include "wps_attr_parse.h"

#ifndef CONFIG_WPS_STRICT
#define WPS_WORKAROUNDS
#endif /* CONFIG_WPS_STRICT */


static int wps_set_vendor_ext_wfa_subelem(struct wps_parse_attr *attr,
					  u8 id, u8 len, const u8 *pos)
{
	da16x_wps_vprt("     [%s] WPS: WFA subelement id=%u len=%u\n",
			__func__, id, len);

	switch (id) {
	case WFA_ELEM_VERSION2:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Version2 length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->version2 = pos;
		break;
	case WFA_ELEM_AUTHORIZEDMACS:
		attr->authorized_macs = pos;
		attr->authorized_macs_len = len;
		break;
	case WFA_ELEM_NETWORK_KEY_SHAREABLE:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Network Key "
				   "Shareable length %u\n", __func__, len);
			return -1;
		}
		attr->network_key_shareable = pos;
		break;
	case WFA_ELEM_REQUEST_TO_ENROLL:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Request to Enroll "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->request_to_enroll = pos;
		break;
	case WFA_ELEM_SETTINGS_DELAY_TIME:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Settings Delay "
				   "Time length %u\n", __func__, len);
			return -1;
		}
		attr->settings_delay_time = pos;
		break;
	case WFA_ELEM_REGISTRAR_CONFIGURATION_METHODS:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Registrar "
				"Configuration Methods length %u\n",
				__func__, len);
			return -1;
		}
		attr->registrar_configuration_methods = pos;
		break;
	default:
		da16x_wps_prt("     [%s] WPS: Skipped unknown WFA Vendor "
			   "Extension subelement %u\n", __func__, id);
		break;
	}

	return 0;
}


static int wps_parse_vendor_ext_wfa(struct wps_parse_attr *attr, const u8 *pos,
				    u16 len)
{
	const u8 *end = pos + len;
	u8 id, elen;

	while (pos + 2 <= end) {
		id = *pos++;
		elen = *pos++;
		if (pos + elen > end)
			break;
		if (wps_set_vendor_ext_wfa_subelem(attr, id, elen, pos) < 0)
			return -1;
		pos += elen;
	}

	return 0;
}


static int wps_parse_vendor_ext(struct wps_parse_attr *attr, const u8 *pos,
				u16 len)
{
	u32 vendor_id;

	if (len < 3) {
		da16x_wps_prt("     [%s] WPS: Skip invalid Vendor Extension\n",
				__func__);
		return 0;
	}

	vendor_id = WPA_GET_BE24(pos);
	switch (vendor_id) {
	case WPS_VENDOR_ID_WFA:
		return wps_parse_vendor_ext_wfa(attr, pos + 3, len - 3);
	}

	/* Handle unknown vendor extensions */

	da16x_wps_prt("     [%s] WPS: Unknown Vendor Extension (Vendor ID %u)\n",
				 __func__,
		   vendor_id);

	if (len > WPS_MAX_VENDOR_EXT_LEN) {
		da16x_wps_prt("     [%s] WPS: Too long Vendor Extension (%u)\n",
				__func__,
			   len);
		return -1;
	}

	if (attr->num_vendor_ext >= MAX_WPS_PARSE_VENDOR_EXT) {
		da16x_wps_prt("     [%s] WPS: Skipped Vendor Extension "
			   "attribute (max %d vendor extensions)\n",
			   __func__, MAX_WPS_PARSE_VENDOR_EXT);
		return -1;
	}
	attr->vendor_ext[attr->num_vendor_ext] = pos;
	attr->vendor_ext_len[attr->num_vendor_ext] = len;
	attr->num_vendor_ext++;

	return 0;
}


static int wps_set_attr(struct wps_parse_attr *attr, u16 type,
			const u8 *pos, u16 len)
{
	switch (type) {
	case ATTR_VERSION:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Version "
				"length %u\n", __func__, len);
			return -1;
		}
		attr->version = pos;
		break;
	case ATTR_MSG_TYPE:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Message Type "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->msg_type = pos;
		break;
	case ATTR_ENROLLEE_NONCE:
		if (len != WPS_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Enrollee Nonce "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->enrollee_nonce = pos;
		break;
	case ATTR_REGISTRAR_NONCE:
		if (len != WPS_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Registrar Nonce "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->registrar_nonce = pos;
		break;
	case ATTR_UUID_E:
		if (len != WPS_UUID_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid UUID-E length %u\n", __func__,
				   len);
			return -1;
		}
		attr->uuid_e = pos;
		break;
	case ATTR_UUID_R:
		if (len != WPS_UUID_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid UUID-R length %u\n", __func__,
				   len);
			return -1;
		}
		attr->uuid_r = pos;
		break;
	case ATTR_AUTH_TYPE_FLAGS:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Authentication "
				   "Type Flags length %u\n", __func__, len);
			return -1;
		}
		attr->auth_type_flags = pos;
		break;
	case ATTR_ENCR_TYPE_FLAGS:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Encryption Type "
				   "Flags length %u\n", __func__, len);
			return -1;
		}
		attr->encr_type_flags = pos;
		break;
	case ATTR_CONN_TYPE_FLAGS:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Connection Type "
				   "Flags length %u\n", __func__, len);
			return -1;
		}
		attr->conn_type_flags = pos;
		break;
	case ATTR_CONFIG_METHODS:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Config Methods "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->config_methods = pos;
		break;
	case ATTR_SELECTED_REGISTRAR_CONFIG_METHODS:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Selected "
				   "Registrar Config Methods length %u\n", __func__, len);
			return -1;
		}
		attr->sel_reg_config_methods = pos;
		break;
	case ATTR_PRIMARY_DEV_TYPE:
		if (len != WPS_DEV_TYPE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Primary Device "
				   "Type length %u\n", __func__, len);
			return -1;
		}
		attr->primary_dev_type = pos;
		break;
	case ATTR_RF_BANDS:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid RF Bands length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->rf_bands = pos;
		break;
	case ATTR_ASSOC_STATE:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Association State "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->assoc_state = pos;
		break;
	case ATTR_CONFIG_ERROR:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Configuration "
				   "Error length %u\n", __func__, len);
			return -1;
		}
		attr->config_error = pos;
		break;
	case ATTR_DEV_PASSWORD_ID:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Device Password "
				   "ID length %u\n", __func__, len);
			return -1;
		}
		attr->dev_password_id = pos;
		break;
	case ATTR_OOB_DEVICE_PASSWORD:
		if (len < WPS_OOB_PUBKEY_HASH_LEN + 2 ||
		    len > WPS_OOB_PUBKEY_HASH_LEN + 2 +
		    WPS_OOB_DEVICE_PASSWORD_LEN ||
		    (len < WPS_OOB_PUBKEY_HASH_LEN + 2 +
		     WPS_OOB_DEVICE_PASSWORD_MIN_LEN &&
		     WPA_GET_BE16(pos + WPS_OOB_PUBKEY_HASH_LEN) !=
		     DEV_PW_NFC_CONNECTION_HANDOVER)) {
			da16x_wps_prt("     [%s] WPS: Invalid OOB Device "
				   "Password length %u\n", __func__, len);
			return -1;
		}
		attr->oob_dev_password = pos;
		attr->oob_dev_password_len = len;
		break;
	case ATTR_OS_VERSION:
		if (len != 4) {
			da16x_wps_prt("     [%s] WPS: Invalid OS Version length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->os_version = pos;
		break;
	case ATTR_WPS_STATE:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Wi-Fi Protected "
				   "Setup State length %u\n", __func__, len);
			return -1;
		}
		attr->wps_state = pos;
		break;
	case ATTR_AUTHENTICATOR:
		if (len != WPS_AUTHENTICATOR_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Authenticator "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->authenticator = pos;
		break;
	case ATTR_R_HASH1:
		if (len != WPS_HASH_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid R-Hash1 length "
				"%u\n", __func__, len);
			return -1;
		}
		attr->r_hash1 = pos;
		break;
	case ATTR_R_HASH2:
		if (len != WPS_HASH_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid R-Hash2 length "
				"%u\n", __func__, len);
			return -1;
		}
		attr->r_hash2 = pos;
		break;
	case ATTR_E_HASH1:
		if (len != WPS_HASH_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid E-Hash1 length "
					"%u\n", __func__, len);
			return -1;
		}
		attr->e_hash1 = pos;
		break;
	case ATTR_E_HASH2:
		if (len != WPS_HASH_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid E-Hash2 length "
					"%u\n", __func__, len);
			return -1;
		}
		attr->e_hash2 = pos;
		break;
	case ATTR_R_SNONCE1:
		if (len != WPS_SECRET_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid R-SNonce1 length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->r_snonce1 = pos;
		break;
	case ATTR_R_SNONCE2:
		if (len != WPS_SECRET_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid R-SNonce2 length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->r_snonce2 = pos;
		break;
	case ATTR_E_SNONCE1:
		if (len != WPS_SECRET_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid E-SNonce1 length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->e_snonce1 = pos;
		break;
	case ATTR_E_SNONCE2:
		if (len != WPS_SECRET_NONCE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid E-SNonce2 length "
				   "%u\n", __func__, len);
			return -1;
		}
		attr->e_snonce2 = pos;
		break;
	case ATTR_KEY_WRAP_AUTH:
		if (len != WPS_KWA_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Key Wrap "
				   "Authenticator length %u\n", __func__, len);
			return -1;
		}
		attr->key_wrap_auth = pos;
		break;
	case ATTR_AUTH_TYPE:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Authentication "
				   "Type length %u\n", __func__, len);
			return -1;
		}
		attr->auth_type = pos;
		break;
	case ATTR_ENCR_TYPE:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid Encryption "
				   "Type length %u\n", __func__, len);
			return -1;
		}
		attr->encr_type = pos;
		break;
	case ATTR_NETWORK_INDEX:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Network Index "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->network_idx = pos;
		break;
	case ATTR_NETWORK_KEY_INDEX:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Network Key Index "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->network_key_idx = pos;
		break;
	case ATTR_MAC_ADDR:
		if (len != ETH_ALEN) {
			da16x_wps_prt("     [%s] WPS: Invalid MAC Address "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->mac_addr = pos;
		break;
	case ATTR_SELECTED_REGISTRAR:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Selected Registrar"
				   " length %u\n", __func__, len);
			return -1;
		}
		attr->selected_registrar = pos;
		break;
	case ATTR_REQUEST_TYPE:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Request Type "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->request_type = pos;
		break;
	case ATTR_RESPONSE_TYPE:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid Response Type "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->response_type = pos;
		break;
	case ATTR_MANUFACTURER:
		attr->manufacturer = pos;
		attr->manufacturer_len = len;
		break;
	case ATTR_MODEL_NAME:
		attr->model_name = pos;
		attr->model_name_len = len;
		break;
	case ATTR_MODEL_NUMBER:
		attr->model_number = pos;
		attr->model_number_len = len;
		break;
	case ATTR_SERIAL_NUMBER:
		attr->serial_number = pos;
		attr->serial_number_len = len;
		break;
	case ATTR_DEV_NAME:
		attr->dev_name = pos;
		attr->dev_name_len = len;
		break;
	case ATTR_PUBLIC_KEY:
		attr->public_key = pos;
		attr->public_key_len = len;
		break;
	case ATTR_ENCR_SETTINGS:
		attr->encr_settings = pos;
		attr->encr_settings_len = len;
		break;
	case ATTR_CRED:
		if (attr->num_cred >= MAX_CRED_COUNT) {
			da16x_wps_prt("     [%s] WPS: Skipped Credential "
				   "attribute (max %d credentials)\n",
					__func__, MAX_CRED_COUNT);
			break;
		}
		attr->cred[attr->num_cred] = pos;
		attr->cred_len[attr->num_cred] = len;
		attr->num_cred++;
		break;
	case ATTR_SSID:
		attr->ssid = pos;
		attr->ssid_len = len;
		break;
	case ATTR_NETWORK_KEY:
		attr->network_key = pos;
		attr->network_key_len = len;
		break;
	case ATTR_AP_SETUP_LOCKED:
		if (len != 1) {
			da16x_wps_prt("     [%s] WPS: Invalid AP Setup Locked "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->ap_setup_locked = pos;
		break;
	case ATTR_REQUESTED_DEV_TYPE:
		if (len != WPS_DEV_TYPE_LEN) {
			da16x_wps_prt("     [%s] WPS: Invalid Requested Device "
				   "Type length %u\n", __func__, len);
			return -1;
		}
		if (attr->num_req_dev_type >= MAX_REQ_DEV_TYPE_COUNT) {
			da16x_wps_prt("     [%s] WPS: Skipped Requested Device "
				   "Type attribute (max %u types)\n", __func__,
				   MAX_REQ_DEV_TYPE_COUNT);
			break;
		}
		attr->req_dev_type[attr->num_req_dev_type] = pos;
		attr->num_req_dev_type++;
		break;
	case ATTR_SECONDARY_DEV_TYPE_LIST:
		if (len > WPS_SEC_DEV_TYPE_MAX_LEN ||
		    (len % WPS_DEV_TYPE_LEN) > 0) {
			da16x_wps_prt("     [%s] WPS: Invalid Secondary Device "
				   "Type length %u\n", __func__, len);
			return -1;
		}
		attr->sec_dev_type_list = pos;
		attr->sec_dev_type_list_len = len;
		break;
	case ATTR_VENDOR_EXT:
		if (wps_parse_vendor_ext(attr, pos, len) < 0)
			return -1;
		break;
	case ATTR_AP_CHANNEL:
		if (len != 2) {
			da16x_wps_prt("     [%s] WPS: Invalid AP Channel "
				   "length %u\n", __func__, len);
			return -1;
		}
		attr->ap_channel = pos;
		break;
	default:
		da16x_wps_prt("     [%s] WPS: Unsupported attribute type 0x%x "
			   "len=%u\n", __func__, type, len);
		break;
	}

	return 0;
}


int wps_parse_msg(const struct wpabuf *msg, struct wps_parse_attr *attr)
{
	const u8 *pos, *end;
	u16 type, len;
#ifdef WPS_WORKAROUNDS
	u16 prev_type = 0;
#endif /* WPS_WORKAROUNDS */

	os_memset(attr, 0, sizeof(*attr));
	pos = wpabuf_head(msg);
	end = pos + wpabuf_len(msg);

	while (pos < end) {
		if (end - pos < 4) {
			da16x_wps_prt("     [%s] WPS: Invalid message - "
				   "%lu bytes remaining\n", __func__,
				   (unsigned long) (end - pos));
			return -1;
		}

		type = WPA_GET_BE16(pos);
		pos += 2;
		len = WPA_GET_BE16(pos);
		pos += 2;

		da16x_wps_vprt("     [%s] WPS: attr_type=0x%x, len=%u\n",
				__func__, type, len);

		if (len > end - pos) {
			da16x_wps_prt("     [%s] WPS: Attribute overflow\n", __func__);
			da16x_buf_dump("WPS: Message data", msg);
#ifdef WPS_WORKAROUNDS
			/*
			 * Some deployed APs seem to have a bug in encoding of
			 * Network Key attribute in the Credential attribute
			 * where they add an extra octet after the Network Key
			 * attribute at least when open network is being
			 * provisioned.
			 */
			if ((type & 0xff00) != 0x1000 &&
			    prev_type == ATTR_NETWORK_KEY) {
				da16x_wps_prt("     [%s] WPS: Workaround - try "
					   "to skip unexpected octet after "
					   "Network Key\n", __func__);
				pos -= 3;
				continue;
			}
#endif /* WPS_WORKAROUNDS */
			return -1;
		}

#ifdef WPS_WORKAROUNDS
		if (type == 0 && len == 0) {
			/*
			 * Mac OS X 10.6 seems to be adding 0x00 padding to the
			 * end of M1. Skip those to avoid interop issues.
			 */
			int i;
			for (i = 0; i < end - pos; i++) {
				if (pos[i])
					break;
			}
			if (i == end - pos) {
				da16x_wps_prt("     [%s] WPS: Workaround - skip "
					   "unexpected message padding\n", __func__);
				break;
			}
		}
#endif /* WPS_WORKAROUNDS */

		if (wps_set_attr(attr, type, pos, len) < 0)
			return -1;

#ifdef WPS_WORKAROUNDS
		prev_type = type;
#endif /* WPS_WORKAROUNDS */
		pos += len;
	}

	return 0;
}

#endif	/* CONFIG_WPS */

/* EOF */
