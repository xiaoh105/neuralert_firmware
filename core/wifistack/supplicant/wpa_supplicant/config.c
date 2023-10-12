/*
 * WPA Supplicant / Configuration parser and common functions
 * Copyright (c) 2003-2018, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "sdk_type.h"  // After app_features.h

#include "includes.h"

#include "supp_common.h"
#include "utils/uuid.h"
#include "utils/ip_addr.h"
#include "common/ieee802_1x_defs.h"
#include "crypto/sha1.h"
#include "rsn_supp/wpa.h"
#include "eap_peer/eap.h"
#ifdef	CONFIG_P2P
#include "p2p/supp_p2p.h"
#endif	/* CONFIG_P2P */
#ifdef	CONFIG_AP
#include "sta_info.h"
#endif	/* CONFIG_AP */
#ifdef CONFIG_FST
#include "fst/fst.h"
#endif /* CONFIG_FST */
#include "supp_config.h"
#include "supp_driver.h"
#include "supp_eloop.h"
#include "wpa_supplicant_i.h"
#include "fc80211_copy.h"
#include "driver_fc80211.h"

#include "mbedtls/config.h"

network_field_t network_fields[] = {
	{ "ssid",						0 },
	{ "scan_ssid",					0 },
	{ "bssid",						0 },
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ "bssid_hint",					0 },
	{ "bssid_blacklist",			0 },
	{ "bssid_whitelist",			0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */
	{ "psk",						0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "mem_only_psk",				0 },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef	CONFIG_SAE
	{ "sae_password",				0 },
	{ "sae_password_id",			0 },
#endif	/* CONFIG_SAE */
	{ "proto",						0 },
	{ "key_mgmt",					0 },
#ifdef	CONFIG_BGSCAN
	{ "bg_scan_period",				0 },
#endif	/* CONFIG_BGSCAN */
	{ "pairwise",					0 },
	{ "group",						0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "group_mgmt",			 		0 },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ "auth_alg",					0 },
	{ "scan_freq",					0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "freq_list",					0 },
	{ "ht",							0 },
	{ "ht40",						0 },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_IEEE80211AC
	{ "vht",						0 },
#endif /* CONFIG_IEEE80211AC */
#ifdef CONFIG_IEEE80211AC
	{ "max_oper_chwidth",			0 },
	{ "vht_center_freq1",			0 },
	{ "vht_center_freq2",			0 },
#endif /* CONFIG_IEEE80211AC */

#ifdef	CONFIG_EAP_METHOD
	{ "eap",						0 },
#endif	/* CONFIG_EAP_METHOD */

#ifdef IEEE8021X_EAPOL
	{ "identity",					0 },
	{ "anonymous_identity",			0 },
	{ "password",					0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "imsi_identity",				0 },
	{ "private_key",				0 },
	{ "private_key_passwd",			0 },
	{ "dh_file",					0 },
	{ "subject_match",				0 },
	{ "altsubject_match",			0 },
	{ "domain_suffix_match",		0 },
	{ "domain_match",				0 },
	{ "dh_file2",					0 },
	{ "subject_match2",				0 },
	{ "altsubject_match2",			0 },
	{ "domain_suffix_match2",		0 },
	{ "domain_match2",				0 },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ "ca_cert2",					0 },
	{ "ca_path2",					0 },
	{ "client_cert2",				0 },
	{ "private_key2",				0 },
	{ "private_key2_passwd",		0 },
	{ "phase1",						0 },
	{ "phase2",						0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "pcsc",						0 },
	{ "pin",						0 },
	{ "engine_id",					0 },
	{ "key_id",						0 },
	{ "cert_id",					0 },
	{ "ca_cert_id",					0 },
	{ "pin2",						0 },
	{ "engine2_id",					0 },
	{ "key2_id",					0 },
	{ "cert2_id",					0 },
	{ "ca_cert2_id",				0 },
	{ "engine",						0 },
	{ "engine2",					0 },
	{ "eapol_flags",				0 },
	{ "sim_num",					0 },
	{ "openssl_ciphers",			0 },
	{ "erp",						0 },
#endif	/* CONFIG_SUPP27_CONFIG */
#endif /* IEEE8021X_EAPOL */

	{ "wep_key0",					0 },
	{ "wep_key1",					0 },
	{ "wep_key2",					0 },
	{ "wep_key3",					0 },
	{ "wep_tx_keyidx",				0 },
#ifdef CONFIG_PRIO_GROUP
	{ "priority",					0 },
#endif /* CONFIG_PRIO_GROUP */
#ifdef IEEE8021X_EAPOL
	{ "eap_workaround",				0 },
	{ "pac_file",					0 },
	{ "fragment_size",				0 },
	{ "ocsp",						0 },
#endif /* IEEE8021X_EAPOL */

	{ "mode",						0 },

#ifdef CONFIG_MESH
	{ "no_auto_peer", 				0 },
	{ "mesh_rssi_threshold",		0 },
#endif /* CONFIG_MESH */

#ifdef	CONFIG_SUPP27_CONFIG
	{ "proactive_key_caching",		0 },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ "disabled",					0 },
	{ "id_str",						0 },

#ifdef CONFIG_IEEE80211W
	{ "ieee80211w",					0 },
#endif /* CONFIG_IEEE80211W */

#ifdef	CONFIG_SUPP27_CONFIG
	{ "peerkey",					0 },	/* obsolete - removed */
	{ "mixed_cell",					0 },
#endif	/* CONFIG_SUPP27_CONFIG */
    { "frequency",					0 },
#ifdef	CONFIG_SUPP27_CONFIG
	{ "fixed_freq", 				0 },
#endif	/* CONFIG_SUPP27_CONFIG */

#ifdef CONFIG_ACS
	{ "acs",						0 },
#endif /* CONFIG_ACS */

#ifdef CONFIG_MESH
	{ "mesh_basic_rates",			0 },
	{ "dot11MeshMaxRetries",		0 },
	{ "dot11MeshRetryTimeout",		0 },
	{ "dot11MeshConfirmTimeout",	0 },
	{ "dot11MeshHoldingTimeout",	0 },
#endif /* CONFIG_MESH */

	{ "wpa_ptk_rekey",				0 },
	{ "group_rekey",				0 },

#ifdef	CONFIG_BGSCAN
	{ "bgscan",						0 },
#endif	/* CONFIG_BGSCAN */
	{ "ignore_broadcast_ssid",		0 },
#ifdef CONFIG_P2P
	{ "go_p2p_dev_addr",			0 },
	{ "p2p_client_list",			0 },
	{ "psk_list",					0 },
#endif /* CONFIG_P2P */

#ifdef CONFIG_HT_OVERRIDES
	{ "disable_ht",					0 },
#ifdef CONFIG_AP_WIFI_MODE
	{ "wifi_mode",					0 },
#endif /* CONFIG_AP_WIFI_MODE */
	{ "disable_sgi",				0 },
	{ "disable_ldpc",				0 },
	{ "ht40_intolerant",			0 },
	{ "disable_max_amsdu",			0 },
	{ "ampdu_factor",				0 },
	{ "ampdu_density",				0 },
	{ "ht_mcs",						0 },
#endif /* CONFIG_HT_OVERRIDES */

#ifdef CONFIG_VHT_OVERRIDES
	{ "disable_vht",				0 },
	{ "vht_capa",					0 },
	{ "vht_capa_mask",				0 },
	{ "vht_rx_mcs_nss_1",			0 },
	{ "vht_rx_mcs_nss_2",			0 },
	{ "vht_rx_mcs_nss_3",			0 },
	{ "vht_rx_mcs_nss_4",			0 },
	{ "vht_rx_mcs_nss_5",			0 },
	{ "vht_rx_mcs_nss_6",			0 },
	{ "vht_rx_mcs_nss_7",			0 },
	{ "vht_rx_mcs_nss_8",			0 },
	{ "vht_tx_mcs_nss_1",			0 },
	{ "vht_tx_mcs_nss_2",			0 },
	{ "vht_tx_mcs_nss_3",			0 },
	{ "vht_tx_mcs_nss_4",			0 },
	{ "vht_tx_mcs_nss_5",			0 },
	{ "vht_tx_mcs_nss_6",			0 },
	{ "vht_tx_mcs_nss_7",			0 },
	{ "vht_tx_mcs_nss_8",			0 },
#endif /* CONFIG_VHT_OVERRIDES */
	{ "ap_max_inactivity",			0 },
	{ "dtim_period",				0 },
	{ "beacon_int",					0 },
#ifdef CONFIG_AUTH_MAX_FAILURES
	{ "auth_max_failures",			0 },
#endif // CONFIG_AUTH_MAX_FAILURES
#ifdef CONFIG_AP_ISOLATION
	{ "isolate",					0 },
#endif /* CONFIG_AP_ISOLATION */

#ifdef CONFIG_AP_POWER
	{ "ap_power",					0 },
#endif /* CONFIG_AP_POWER */
#ifdef CONFIG_MACSEC
	{ "macsec_policy",				0 },
	{ "macsec_integ_only",			0 },
	{ "macsec_port",				0 },
	{ "mka_priority",				0 },
	{ "mka_cak",					0 },
	{ "mka_ckn",					0 },
#endif /* CONFIG_MACSEC */
#ifdef CONFIG_HS20
	{ "update_identifier",			0 },
	{ "roaming_consortium_selection", 0 },
#endif /* CONFIG_HS20 */
#ifdef CONFIG_RANDOM_ADDR
	{ "mac_addr", 					0 },
#endif /* CONFIG_RANDOM_ADDR */
#ifdef	CONFIG_SUPP27_CONFIG
	{ "pbss",						0 },
	{ "wps_disabled",				0 },
	{ "fils_dh_group",				0 },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_DPP
	{ "dpp_connector",				0 },
	{ "dpp_netaccesskey",			0 },
	{ "dpp_netaccesskey_expiry",	0 },
	{ "dpp_csign",					0 },
#endif /* CONFIG_DPP */
#ifdef CONFIG_OWE
	{ "owe_group",					0 },
	{ "owe_only", 					0 },
#endif /* CONFIG_OWE */
	{ NULL,							0 }
};


/*
 * Structure for network configuration parsing. This data is used to implement
 * a generic parser for each network block variable. The table of configuration
 * variables is defined below in this file (ssid_fields[]).
 */
struct parse_data {
	/* Configuration variable name */
	char *name;

	/* Parser function for this variable. The parser functions return 0 or 1
	 * to indicate success. Value 0 indicates that the parameter value may
	 * have changed while value 1 means that the value did not change.
	 * Error cases (failure to parse the string) are indicated by returning
	 * -1. */
	int (*parser)(const struct parse_data *data, struct wpa_ssid *ssid,
		      const char *value);

	/* Writer function (i.e., to get the variable in text format from
	 * internal presentation). */
	char * (*writer)(const struct parse_data *data, struct wpa_ssid *ssid);

	/* Variable specific parameters for the parser. */
	void *param1, *param2, *param3, *param4;

	/* 0 = this variable can be included in debug output and ctrl_iface
	 * 1 = this variable contains key/private data and it must not be
	 *     included in debug output unless explicitly requested. In
	 *     addition, this variable will not be readable through the
	 *     ctrl_iface.
	 */
	int key_data;
};


static int wpa_config_parse_str(const struct parse_data *data,
				struct wpa_ssid *ssid,
				const char *value)
{
	size_t res_len, *dst_len, prev_len;
	char **dst, *tmp;

	TX_FUNC_START("");

	da16x_cli_prt("[%s] value=[%s]\n", __func__, value);

	if (os_strcmp(value, "NULL") == 0) {
		wpa_printf_dbg(MSG_DEBUG, "Unset configuration string '%s'",
			   data->name);
		tmp = NULL;
		res_len = 0;
		goto set;
	}

	tmp = wpa_config_parse_string(value, &res_len);
	if (tmp == NULL) {
		wpa_printf(MSG_ERROR, "failed to parse %s '%s'.",
			   data->name,
			   data->key_data ? "[KEY DATA REMOVED]" : value);
		return -1;
	}

	if (data->key_data) {
		wpa_hexdump_ascii_key(MSG_MSGDUMP, data->name,
				      (u8 *) tmp, res_len);
	} else {
		wpa_hexdump_ascii(MSG_MSGDUMP, data->name,
				  (u8 *) tmp, res_len);
	}

	if (data->param3 && res_len < (size_t) data->param3) {
		wpa_printf(MSG_ERROR, "too short %s (len=%lu "
			   "min_len=%ld)", data->name,
			   (unsigned long) res_len, (long) data->param3);
		os_free(tmp);
		return -1;
	}

	if (data->param4 && res_len > (size_t) data->param4) {
		wpa_printf(MSG_ERROR, "too long %s (len=%lu "
			   "max_len=%ld)", data->name,
			   (unsigned long) res_len, (long) data->param4);
		os_free(tmp);
		return -1;
	}

set:
	dst = (char **) (((u8 *) ssid) + (long) data->param1);
	dst_len = (size_t *) (((u8 *) ssid) + (long) data->param2);

	if (data->param2)
		prev_len = *dst_len;
	else if (*dst)
		prev_len = os_strlen(*dst);
	else
		prev_len = 0;
	if ((*dst == NULL && tmp == NULL) ||
	    (*dst && tmp && prev_len == res_len &&
	     os_memcmp(*dst, tmp, res_len) == 0)) {
		/* No change to the previously configured value */
		os_free(tmp);
		return 1;
	}

	os_free(*dst);
	*dst = tmp;
	if (data->param2)
		*dst_len = res_len;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_string_ascii(const u8 *value, size_t len)
{
	char *buf;

	da16x_cli_prt("[%s] value=%s\n", __func__, (char*)value);

	buf = os_malloc(len + 3);
	if (buf == NULL)
		return NULL;
	buf[0] = '"';
	os_memcpy(buf + 1, value, len);
	buf[len + 1] = '"';
	buf[len + 2] = '\0';

	return buf;
}

char * wpa_config_write_string_hex(const u8 *value, size_t len)
{
	char *buf;

	buf = os_zalloc(2 * len + 1);
	if (buf == NULL)
		return NULL;
	wpa_snprintf_hex(buf, 2 * len + 1, value, len);

	return buf;
}

/* static */ char * wpa_config_write_string(const u8 *value, size_t len)
{
	if (value == NULL)
		return NULL;

	if (is_hex(value, len))
		return wpa_config_write_string_hex(value, len);
	else
		return wpa_config_write_string_ascii(value, len);
}


static char * wpa_config_write_str(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	size_t len;
	char **src;

	src = (char **) (((u8 *) ssid) + (long) data->param1);
	if (*src == NULL)
		return NULL;

	if (data->param2)
		len = *((size_t *) (((u8 *) ssid) + (long) data->param2));
	else
		len = os_strlen(*src);

	return wpa_config_write_string((const u8 *) *src, len);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_int(const struct parse_data *data,
				struct wpa_ssid *ssid,
				const char *value)
{
	int val, *dst;
	char *end;

	dst = (int *) (((u8 *) ssid) + (long) data->param1);
	val = strtol(value, &end, 0);
	if (*end) {
		wpa_printf(MSG_ERROR, "Invalid number \"%s\"",
			   value);
		return -1;
	}

#if 0
	if (*dst == val)
		return 1;
#endif /* 0 */

	*dst = val;
	wpa_printf(MSG_MSGDUMP, "%s=%d (0x%x)", data->name, *dst, *dst);

	if (data->param3 && *dst < (long) data->param3) {
		wpa_printf(MSG_ERROR, "Too small %s (value=%d "
			   "min_value=%ld)", data->name, *dst,
			   (long) data->param3);
		*dst = (long) data->param3;
		return -1;
	}

	if (data->param4 && *dst > (long) data->param4) {
		wpa_printf(MSG_ERROR, "Too large %s (value=%d "
			   "max_value=%ld)", data->name, *dst,
			   (long) data->param4);
		*dst = (long) data->param4;
		return -1;
	}

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_int(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	int *src, res;
	char *value;

	src = (int *) (((u8 *) ssid) + (long) data->param1);

	value = os_malloc(20);
	if (value == NULL)
		return NULL;
	res = os_snprintf(value, 20, "%d", *src);
	if (os_snprintf_error(20, res)) {
		os_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}

#if defined ( CONFIG_P2P )
static int wpa_config_parse_addr_list(const struct parse_data *data,
				      const char *value,
				      u8 **list, size_t *num, char *name,
				      u8 abort_on_error, u8 masked)
{
	const char *pos;
	u8 *buf, *n, addr[2 * ETH_ALEN];
	size_t count;

	buf = NULL;
	count = 0;

	pos = value;
	while (pos && *pos) {
		while (*pos == ' ')
			pos++;

		if (hwaddr_masked_aton(pos, addr, &addr[ETH_ALEN], masked)) {
			if (abort_on_error || count == 0) {
				wpa_printf(MSG_ERROR,
					   "Invalid %s address '%s'",
					   name, value);
				os_free(buf);
				return -1;
			}
			/* continue anyway since this could have been from a
			 * truncated configuration file line */
			wpa_printf(MSG_INFO,
				   "Ignore likely truncated %s address '%s'",
				   name, pos);
		} else {
			n = os_realloc_array(buf, count + 1, 2 * ETH_ALEN);
			if (n == NULL) {
				os_free(buf);
				return -1;
			}
			buf = n;
			os_memmove(buf + 2 * ETH_ALEN, buf,
				   count * 2 * ETH_ALEN);
			os_memcpy(buf, addr, 2 * ETH_ALEN);
			count++;
			wpa_printf(MSG_MSGDUMP,
				   "%s: addr=" MACSTR " mask=" MACSTR,
				   name, MAC2STR(addr),
				   MAC2STR(&addr[ETH_ALEN]));
		}

		pos = os_strchr(pos, ' ');
	}

	os_free(*list);
	*list = buf;
	*num = count;

	return 0;
}


static char * wpa_config_write_addr_list(const struct parse_data *data,
					 const u8 *list, size_t num, char *name)
{
	char *value, *end, *pos;
	int res;
	size_t i;

	if (list == NULL || num == 0)
		return NULL;

	value = os_malloc(2 * 20 * num);
	if (value == NULL)
		return NULL;
	pos = value;
	end = value + 2 * 20 * num;

	for (i = num; i > 0; i--) {
		const u8 *a = list + (i - 1) * 2 * ETH_ALEN;
		const u8 *m = a + ETH_ALEN;

		if (i < num)
			*pos++ = ' ';
		res = hwaddr_mask_txt(pos, end - pos, a, m);
		if (res < 0) {
			os_free(value);
			return NULL;
		}
		pos += res;
	}

	return value;
}
#endif	// CONFIG_P2P
#endif /* NO_CONFIG_WRITE */

#ifdef CONFIG_AP_POWER
static int wpa_config_parse_ap_power(const struct parse_data *data,
				  struct wpa_ssid *ssid, const char *value)
{
	if (value == NULL)
		return -1;

	os_free(ssid->ap_power);
	ssid->ap_power = os_strdup(value);
	if (ssid->ap_power == NULL)
		return -1;

	return 0;
}
static char *wpa_config_write_ap_power(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	char *buf;

	buf = os_strdup(ssid->ap_power);
	if (buf == NULL)
		return NULL;

	return buf;
}
#endif /* CONFIG_AP_POWER */

static int wpa_config_parse_bssid(const struct parse_data *data,
				  struct wpa_ssid *ssid,
				  const char *value)
{
	if (value[0] == '\0' || os_strcmp(value, "\"\"") == 0 ||
	    os_strcmp(value, "any") == 0) {
		ssid->bssid_set = 0;
		wpa_printf(MSG_MSGDUMP, "BSSID any");
		return 0;
	}
	if (hwaddr_aton(value, ssid->bssid)) {
		wpa_printf(MSG_ERROR, "Invalid BSSID '%s'.",
			   value);
		return -1;
	}
	ssid->bssid_set = 1;
	wpa_hexdump(MSG_MSGDUMP, "BSSID", ssid->bssid, ETH_ALEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_bssid(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	char *value;
	int res;

	if (!ssid->bssid_set)
		return NULL;

	value = os_malloc(20);
	if (value == NULL)
		return NULL;
	res = os_snprintf(value, 20, MACSTR, MAC2STR(ssid->bssid));
	if (os_snprintf_error(20, res)) {
		os_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}
#endif /* NO_CONFIG_WRITE */

#ifdef	CONFIG_P2P_UNUSED_CMD
static int wpa_config_parse_bssid_hint(const struct parse_data *data,
				       struct wpa_ssid *ssid,
				       const char *value)
{
	if (value[0] == '\0' || os_strcmp(value, "\"\"") == 0 ||
	    os_strcmp(value, "any") == 0) {
		ssid->bssid_hint_set = 0;
		wpa_printf(MSG_MSGDUMP, "BSSID hint any");
		return 0;
	}
	if (hwaddr_aton(value, ssid->bssid_hint)) {
		wpa_printf(MSG_ERROR, "Invalid BSSID hint '%s'.",
			   value);
		return -1;
	}
	ssid->bssid_hint_set = 1;
	wpa_hexdump(MSG_MSGDUMP, "BSSID hint", ssid->bssid_hint, ETH_ALEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_bssid_hint(const struct parse_data *data,
					  struct wpa_ssid *ssid)
{
	char *value;
	int res;

	if (!ssid->bssid_hint_set)
		return NULL;

	value = os_malloc(20);
	if (!value)
		return NULL;
	res = os_snprintf(value, 20, MACSTR, MAC2STR(ssid->bssid_hint));
	if (os_snprintf_error(20, res)) {
		os_free(value);
		return NULL;
	}
	return value;
}
#endif /* NO_CONFIG_WRITE */


#ifdef	CONFIG_P2P_UNUSED_CMD
static int wpa_config_parse_bssid_blacklist(const struct parse_data *data,
					    struct wpa_ssid *ssid,
					    const char *value)
{
	return wpa_config_parse_addr_list(data, value,
					  &ssid->bssid_blacklist,
					  &ssid->num_bssid_blacklist,
					  "bssid_blacklist", 1, 1);
}
#endif	/* CONFIG_P2P_UNUSED_CMD */

#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_bssid_blacklist(const struct parse_data *data,
					       struct wpa_ssid *ssid)
{
	return wpa_config_write_addr_list(data, ssid->bssid_blacklist,
					  ssid->num_bssid_blacklist,
					  "bssid_blacklist");
}
#endif /* NO_CONFIG_WRITE */


#if 0	/* Not used code */
static int wpa_config_parse_bssid_whitelist(const struct parse_data *data,
					    struct wpa_ssid *ssid,
					    const char *value)
{
	return wpa_config_parse_addr_list(data, value,
					  &ssid->bssid_whitelist,
					  &ssid->num_bssid_whitelist,
					  "bssid_whitelist", 1, 1);
}
#endif	/* 0 */


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_bssid_whitelist(const struct parse_data *data,
					       struct wpa_ssid *ssid)
{
	return wpa_config_write_addr_list(data, ssid->bssid_whitelist,
					  ssid->num_bssid_whitelist,
					  "bssid_whitelist");
}
#endif /* NO_CONFIG_WRITE */
#endif	/* CONFIG_P2P_UNUSED_CMD */

static int wpa_config_parse_psk(const struct parse_data *data,
				struct wpa_ssid *ssid,
				const char *value)
{
#ifdef CONFIG_EXT_PASSWORD
	if (os_strncmp(value, "ext:", 4) == 0) {
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(ssid->passphrase);
#else
		os_free(ssid->passphrase);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		ssid->passphrase = NULL;
		ssid->psk_set = 0;
		os_free(ssid->ext_psk);
		ssid->ext_psk = os_strdup(value + 4);
		if (ssid->ext_psk == NULL)
			return -1;
		wpa_printf_dbg(MSG_DEBUG, "PSK: External password '%s'",
			   ssid->ext_psk);
		return 0;
	}
#endif /* CONFIG_EXT_PASSWORD */

	if (*value == '\'' || *value == '\"') {
#ifndef CONFIG_NO_PBKDF2
		const char *pos = NULL;
		size_t len;

		if (*value == '\'') {
			value++;
			pos = os_strrchr(value, '\'');
		} else if(*value == '\"') {
			value++;
			pos = os_strrchr(value, '\"');
		}

		if (pos)
			len = pos - value;
		else
			len = os_strlen(value);

		if (len < 8 || len > 63) {
			wpa_printf(MSG_ERROR, "Invalid passphrase "
				   "length %lu (expected: 8..63) '%s'.",
				   (unsigned long) len, value);
			return -1;
		}
		wpa_hexdump_ascii_key(MSG_MSGDUMP, "PSK (ASCII passphrase)", (u8 *) value, len);
#if 0	/* by Bright : Code sync with Supp2.4
	 *	     : DA16200 need special character [ ' ] in cli command line distinguisher.
	 */
		if (has_ctrl_char((u8 *) value, len)) {
			wpa_printf(MSG_ERROR, "Invalid passphrase character");
			return -1;
		}
#endif	/* 0 */
		if (ssid->passphrase && os_strlen(ssid->passphrase) == len &&
		    os_memcmp(ssid->passphrase, value, len) == 0) {
			/* No change to the previously configured value */
#if 0	/* by Bright */
			return 1;
#else
			return 0;
#endif	/* 0 */
		}
		ssid->psk_set = 0;
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(ssid->passphrase);
#else
		os_free(ssid->passphrase);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		ssid->passphrase = dup_binstr(value, len);
		if (ssid->passphrase == NULL)
			return -1;
		return 0;
#else /* CONFIG_NO_PBKDF2 */
		wpa_printf(MSG_ERROR, "ASCII passphrase not supported.");
		return -1;
#endif /* CONFIG_NO_PBKDF2 */
	}

	if (hexstr2bin(value, ssid->psk, PMK_LEN) ||
	    value[PMK_LEN * 2] != '\0') {
		wpa_printf(MSG_ERROR, "Invalid PSK '%s'.", value);
		return -1;
	}

#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(ssid->passphrase);
#else
	os_free(ssid->passphrase);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	ssid->passphrase = NULL;

	ssid->psk_set = 1;
	wpa_hexdump_key(MSG_MSGDUMP, "PSK", ssid->psk, PMK_LEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_psk(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
#ifdef CONFIG_EXT_PASSWORD
	if (ssid->ext_psk) {
		size_t len = 4 + os_strlen(ssid->ext_psk) + 1;
		char *buf = os_malloc(len);
		int res;

		if (buf == NULL)
			return NULL;
		res = os_snprintf(buf, len, "ext:%s", ssid->ext_psk);
		if (os_snprintf_error(len, res)) {
			os_free(buf);
			buf = NULL;
		}
		return buf;
	}
#endif /* CONFIG_EXT_PASSWORD */

	if (ssid->passphrase)
		return wpa_config_write_string_ascii(
			(const u8 *) ssid->passphrase,
			os_strlen(ssid->passphrase));

	if (ssid->psk_set)
		return wpa_config_write_string_hex(ssid->psk, PMK_LEN);

	return NULL;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_proto(const struct parse_data *data,
				  struct wpa_ssid *ssid,
				  const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "WPA") == 0)
			val |= WPA_PROTO_WPA;
		else if (os_strcmp(start, "RSN") == 0 ||
			 os_strcmp(start, "WPA2") == 0)
			val |= WPA_PROTO_RSN;
		else if (os_strcmp(start, "OSEN") == 0)
			val |= WPA_PROTO_OSEN;
		else {
			wpa_printf(MSG_ERROR, "Invalid proto '%s'",
				   start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR,
			   "No proto values configured.");
		errors++;
	} else {
		wpa_printf(MSG_MSGDUMP, "proto: 0x%x", val);
		ssid->proto = val;
	}
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_proto(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	int first = 1, ret;
	char *buf, *pos, *end;

	pos = buf = os_zalloc(20);
	if (buf == NULL)
		return NULL;
	end = buf + 20;

	if (ssid->proto & WPA_PROTO_WPA) {
		ret = os_snprintf(pos, end - pos, "%sWPA", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return buf;
		pos += ret;
		first = 0;
	}

	if (ssid->proto & WPA_PROTO_RSN) {
		ret = os_snprintf(pos, end - pos, "%sRSN", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return buf;
		pos += ret;
		first = 0;
	}

	if (ssid->proto & WPA_PROTO_OSEN) {
		ret = os_snprintf(pos, end - pos, "%sOSEN", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return buf;
		pos += ret;
		first = 0;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_key_mgmt(const struct parse_data *data,
				     struct wpa_ssid *ssid, const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';

		if (os_strcmp(start, "WPA-PSK") == 0)
			val |= WPA_KEY_MGMT_PSK;
		else if (os_strcmp(start, "WPA-EAP") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X;
		else if (os_strcmp(start, "IEEE8021X") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_NO_WPA;
		else if (os_strcmp(start, "NONE") == 0)
			val |= WPA_KEY_MGMT_NONE;
		else if (os_strcmp(start, "WPA-NONE") == 0)
			val |= WPA_KEY_MGMT_WPA_NONE;
#ifdef CONFIG_IEEE80211R
		else if (os_strcmp(start, "FT-PSK") == 0)
			val |= WPA_KEY_MGMT_FT_PSK;
		else if (os_strcmp(start, "FT-EAP") == 0)
			val |= WPA_KEY_MGMT_FT_IEEE8021X;
#ifdef CONFIG_SHA384
		else if (os_strcmp(start, "FT-EAP-SHA384") == 0)
			val |= WPA_KEY_MGMT_FT_IEEE8021X_SHA384;
#endif /* CONFIG_SHA384 */
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IEEE80211W
		else if (os_strcmp(start, "WPA-PSK-SHA256") == 0)
			val |= WPA_KEY_MGMT_PSK_SHA256;
		else if (os_strcmp(start, "WPA-EAP-SHA256") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_SHA256;
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_WPS
		else if (os_strcmp(start, "WPS") == 0)
			val |= WPA_KEY_MGMT_WPS;
#endif /* CONFIG_WPS */
#ifdef CONFIG_SAE
		else if (os_strcmp(start, "SAE") == 0)
			val |= WPA_KEY_MGMT_SAE;
		else if (os_strcmp(start, "FT-SAE") == 0)
			val |= WPA_KEY_MGMT_FT_SAE;
#endif /* CONFIG_SAE */
#ifdef CONFIG_HS20
		else if (os_strcmp(start, "OSEN") == 0)
			val |= WPA_KEY_MGMT_OSEN;
#endif /* CONFIG_HS20 */
#ifdef CONFIG_SUITEB
		else if (os_strcmp(start, "WPA-EAP-SUITE-B") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_SUITE_B;
#endif /* CONFIG_SUITEB */
#ifdef CONFIG_SUITEB192
		else if (os_strcmp(start, "WPA-EAP-SUITE-B-192") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_SUITE_B_192;
#endif /* CONFIG_SUITEB192 */
#ifdef CONFIG_FILS
		else if (os_strcmp(start, "FILS-SHA256") == 0)
			val |= WPA_KEY_MGMT_FILS_SHA256;
#ifdef CONFIG_SHA384
		else if (os_strcmp(start, "FILS-SHA384") == 0)
			val |= WPA_KEY_MGMT_FILS_SHA384;
#endif /* CONFIG_SHA384 */
#ifdef CONFIG_IEEE80211R
		else if (os_strcmp(start, "FT-FILS-SHA256") == 0)
			val |= WPA_KEY_MGMT_FT_FILS_SHA256;
#ifdef CONFIG_SHA384
		else if (os_strcmp(start, "FT-FILS-SHA384") == 0)
			val |= WPA_KEY_MGMT_FT_FILS_SHA384;
#endif /* CONFIG_SHA384 */
#endif /* CONFIG_IEEE80211R */
#endif /* CONFIG_FILS */
#ifdef CONFIG_OWE
		else if (os_strcmp(start, "OWE") == 0)
			val |= WPA_KEY_MGMT_OWE;
#endif /* CONFIG_OWE */
#ifdef CONFIG_DPP
		else if (os_strcmp(start, "DPP") == 0)
			val |= WPA_KEY_MGMT_DPP;
#endif /* CONFIG_DPP */
		else {
			wpa_printf(MSG_ERROR, "Invalid key_mgmt '%s'",
				   start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		da16x_cli_prt("[%s] No key_mgmt values configured\n", __func__);

		errors++;
	} else {
		da16x_cli_prt("[%s] key_mgmt: 0x%x\n", __func__, val);
		ssid->key_mgmt = val;
	}
	
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_key_mgmt(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf, *pos, *end;
	int ret;

	pos = buf = os_zalloc(100);
	if (buf == NULL)
		return NULL;
	end = buf + 100;

	if (ssid->key_mgmt & WPA_KEY_MGMT_PSK) {
		ret = os_snprintf(pos, end - pos, "%sWPA-PSK",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X) {
		ret = os_snprintf(pos, end - pos, "%sWPA-EAP",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		ret = os_snprintf(pos, end - pos, "%sIEEE8021X",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNONE",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_WPA_NONE) {
		ret = os_snprintf(pos, end - pos, "%sWPA-NONE",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

#ifdef CONFIG_IEEE80211R
	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_PSK) {
		ret = os_snprintf(pos, end - pos, "%sFT-PSK",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_IEEE8021X) {
		ret = os_snprintf(pos, end - pos, "%sFT-EAP",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

#ifdef CONFIG_SHA384
	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_IEEE8021X_SHA384) {
		ret = os_snprintf(pos, end - pos, "%sFT-EAP-SHA384",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SHA384 */
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211W
	if (ssid->key_mgmt & WPA_KEY_MGMT_PSK_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sWPA-PSK-SHA256",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sWPA-EAP-SHA256",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_WPS
	if (ssid->key_mgmt & WPA_KEY_MGMT_WPS) {
		ret = os_snprintf(pos, end - pos, "%sWPS",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_SAE
	if (ssid->key_mgmt & WPA_KEY_MGMT_SAE) {
		ret = os_snprintf(pos, end - pos, "%sSAE",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_SAE) {
		ret = os_snprintf(pos, end - pos, "%sFT-SAE",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SAE */

#ifdef CONFIG_OWE
	if (ssid->key_mgmt & WPA_KEY_MGMT_OWE) {
		ret = os_snprintf(pos, end - pos, "%sOWE",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_OWE */

#ifdef CONFIG_HS20
	if (ssid->key_mgmt & WPA_KEY_MGMT_OSEN) {
		ret = os_snprintf(pos, end - pos, "%sOSEN",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_HS20 */

#ifdef CONFIG_SUITEB
	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_SUITE_B) {
		ret = os_snprintf(pos, end - pos, "%sWPA-EAP-SUITE-B",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SUITEB */

#ifdef CONFIG_SUITEB192
	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_SUITE_B_192) {
		ret = os_snprintf(pos, end - pos, "%sWPA-EAP-SUITE-B-192",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SUITEB192 */

#ifdef CONFIG_FILS
	if (ssid->key_mgmt & WPA_KEY_MGMT_FILS_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sFILS-SHA256",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#ifdef CONFIG_SHA384
	if (ssid->key_mgmt & WPA_KEY_MGMT_FILS_SHA384) {
		ret = os_snprintf(pos, end - pos, "%sFILS-SHA384",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SHA384 */
#ifdef CONFIG_IEEE80211R
	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_FILS_SHA256) {
		ret = os_snprintf(pos, end - pos, "%sFT-FILS-SHA256",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#ifdef CONFIG_SHA384
	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_FILS_SHA384) {
		ret = os_snprintf(pos, end - pos, "%sFT-FILS-SHA384",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}
#endif /* CONFIG_SHA384 */
#endif /* CONFIG_IEEE80211R */
#endif /* CONFIG_FILS */

	if (pos == buf) {
		os_free(buf);
		buf = NULL;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_cipher(const char *value)
{
#ifdef CONFIG_NO_WPA
	return -1;
#else /* CONFIG_NO_WPA */
	int val = wpa_parse_cipher(value);
	if (val < 0) {
		wpa_printf(MSG_ERROR, "Invalid cipher '%s'.", value);
		return -1;
	}
	if (val == 0) {
		wpa_printf(MSG_ERROR, "No cipher values configured.");
		return -1;
	}
	return val;
#endif /* CONFIG_NO_WPA */
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_cipher(int cipher)
{
#ifdef CONFIG_NO_WPA
	return NULL;
#else /* CONFIG_NO_WPA */
	char *buf = os_zalloc(50);

	if (buf == NULL)
		return NULL;

	if (wpa_write_ciphers(buf, buf + 50, cipher, " ") < 0) {
		os_free(buf);
		return NULL;
	}

	return buf;
#endif /* CONFIG_NO_WPA */
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_pairwise(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
	int val;
	val = wpa_config_parse_cipher(value);
	if (val == -1)
		return -1;

#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(val == WPA_CIPHER_TKIP &&
		ssid->wifi_mode == DA16X_WIFI_MODE_N_ONLY) {
		da16x_cli_prt("[%s] Not allowed	TKIP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	if (val & ~WPA_ALLOWED_PAIRWISE_CIPHERS) {
		wpa_printf(MSG_ERROR, "Not allowed pairwise cipher "
			   "(0x%x).", val);
		return -1;
	}

	wpa_printf(MSG_MSGDUMP, "pairwise: 0x%x", val);
	ssid->pairwise_cipher = val;
	ssid->group_cipher |= val;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_pairwise(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_cipher(ssid->pairwise_cipher);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_group(const struct parse_data *data,
				  struct wpa_ssid *ssid,
				  const char *value)
{
	int val;
	val = wpa_config_parse_cipher(value);

#if 0
	if (val == -1)
		return -1;
#endif /* 0 */

#if 0 /* FC9000 Station WEP ï¿½ï¿½ï¿½ï¿½ */
	/*
	 * Backwards compatibility - filter out WEP ciphers that were previously
	 * allowed.
	 */
	val &= ~(WPA_CIPHER_WEP104 | WPA_CIPHER_WEP40);
#endif /* 0 */

	if (val & ~WPA_ALLOWED_GROUP_CIPHERS) {
		wpa_printf(MSG_ERROR, "Not allowed group cipher "
			   "(0x%x).", val);
		return -1;
	}

	wpa_printf(MSG_MSGDUMP, "group: 0x%x", val);
	ssid->group_cipher = val;
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_group(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	return wpa_config_write_cipher(ssid->group_cipher);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_group_mgmt(const struct parse_data *data,
				       struct wpa_ssid *ssid,
				       const char *value)
{
	int val;

	val = wpa_config_parse_cipher(value);
	if (val == -1)
		return -1;

#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(val == WPA_CIPHER_TKIP &&
 		ssid->wifi_mode == DA16X_WIFI_MODE_N_ONLY) {
		da16x_cli_prt("[%s] Not allowed  TKIP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	if (val & ~WPA_ALLOWED_GROUP_MGMT_CIPHERS) {
		wpa_printf(MSG_ERROR,
			   "Not allowed group management cipher (0x%x).",
			   val);
		return -1;
	}

	wpa_printf(MSG_MSGDUMP, "group_mgmt: 0x%x", val);
	ssid->group_mgmt_cipher = val;
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_group_mgmt(const struct parse_data *data,
					  struct wpa_ssid *ssid)
{
	return wpa_config_write_cipher(ssid->group_mgmt_cipher);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_auth_alg(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "OPEN") == 0)
			val |= WPA_AUTH_ALG_OPEN;
		else if (os_strcmp(start, "SHARED") == 0)
			val |= WPA_AUTH_ALG_SHARED;
		else if (os_strcmp(start, "LEAP") == 0)
			val |= WPA_AUTH_ALG_LEAP;
		else {
			wpa_printf(MSG_ERROR, "Invalid auth_alg '%s'",
				   start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR,
			   "No auth_alg values configured.");
		errors++;
	} else {
		wpa_printf(MSG_MSGDUMP, "auth_alg: 0x%x", val);
		ssid->auth_alg = val;
	}
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_auth_alg(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf, *pos, *end;
	int ret;

	pos = buf = os_zalloc(30);
	if (buf == NULL)
		return NULL;
	end = buf + 30;

	if (ssid->auth_alg & WPA_AUTH_ALG_OPEN) {
		ret = os_snprintf(pos, end - pos, "%sOPEN",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->auth_alg & WPA_AUTH_ALG_SHARED) {
		ret = os_snprintf(pos, end - pos, "%sSHARED",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->auth_alg & WPA_AUTH_ALG_LEAP) {
		ret = os_snprintf(pos, end - pos, "%sLEAP",
				  pos == buf ? "" : " ");
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (pos == buf) {
		os_free(buf);
		buf = NULL;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int * wpa_config_parse_int_array(const char *value)
{
	int *freqs;
	size_t used, len;
	const char *pos;

	used = 0;
	len = 10;
	freqs = os_calloc(len + 1, sizeof(int));
	if (freqs == NULL)
		return NULL;

	pos = value;
	while (pos) {
		while (*pos == ' ')
			pos++;
		if (used == len) {
			int *n;
			size_t i;
			n = os_realloc_array(freqs, len * 2 + 1, sizeof(int));
			if (n == NULL) {
				os_free(freqs);
				return NULL;
			}
			for (i = len; i <= len * 2; i++)
				n[i] = 0;
			freqs = n;
			len *= 2;
		}

		freqs[used] = atoi(pos);
		if (freqs[used] == 0)
			break;
		used++;
		pos = os_strchr(pos + 1, ' ');
	}

	return freqs;
}


static int wpa_config_parse_scan_freq(const struct parse_data *data,
				      struct wpa_ssid *ssid,
				      const char *value)
{
	int *freqs;

	freqs = wpa_config_parse_int_array(value);
	if (freqs == NULL)
		return -1;
	if (freqs[0] == 0) {
		os_free(freqs);
		freqs = NULL;
	}
	os_free(ssid->scan_freq);
	ssid->scan_freq = freqs;

	return 0;
}


static int wpa_config_parse_freq_list(const struct parse_data *data,
				      struct wpa_ssid *ssid,
				      const char *value)
{
	int *freqs;

	freqs = wpa_config_parse_int_array(value);
	if (freqs == NULL)
		return -1;
	if (freqs[0] == 0) {
		os_free(freqs);
		freqs = NULL;
	}
	os_free(ssid->freq_list);
	ssid->freq_list = freqs;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_freqs(const struct parse_data *data,
				     const int *freqs)
{
	char *buf, *pos, *end;
	int i, ret;
	size_t count;

	if (freqs == NULL)
		return NULL;

	count = 0;
	for (i = 0; freqs[i]; i++)
		count++;

	pos = buf = os_zalloc(10 * count + 1);
	if (buf == NULL)
		return NULL;
	end = buf + 10 * count + 1;

	for (i = 0; freqs[i]; i++) {
		ret = os_snprintf(pos, end - pos, "%s%u",
				  i == 0 ? "" : " ", freqs[i]);
		if (os_snprintf_error(end - pos, ret)) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	return buf;
}


static char * wpa_config_write_scan_freq(const struct parse_data *data,
					 struct wpa_ssid *ssid)
{
	return wpa_config_write_freqs(data, ssid->scan_freq);
}


static char * wpa_config_write_freq_list(const struct parse_data *data,
					 struct wpa_ssid *ssid)
{
	return wpa_config_write_freqs(data, ssid->freq_list);
}
#endif /* NO_CONFIG_WRITE */


#ifdef IEEE8021X_EAPOL
static int wpa_config_parse_eap(const struct parse_data *data,
				struct wpa_ssid *ssid,
				const char *value)
{
	int last, errors = 0;
	char *start, *end, *buf;
	struct eap_method_type *methods = NULL, *tmp;
	size_t num_methods = 0;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		tmp = methods;
		methods = os_realloc_array(methods, num_methods + 1,
					   sizeof(*methods));
		if (methods == NULL) {
			os_free(tmp);
			os_free(buf);
			return -1;
		}
		methods[num_methods].method = eap_peer_get_type(
			start, &methods[num_methods].vendor);

		if (methods[num_methods].vendor == EAP_VENDOR_IETF &&
		    methods[num_methods].method == EAP_TYPE_NONE) {
			wpa_printf(MSG_ERROR, "Unknown EAP method "
				   "'%s'", start);
			wpa_printf(MSG_ERROR, "You may need to add support for"
				   " this EAP method during wpa_supplicant\n"
				   "build time configuration.\n"
				   "See README for more information.");
			errors++;
		} else if (methods[num_methods].vendor == EAP_VENDOR_IETF &&
			   methods[num_methods].method == EAP_TYPE_LEAP)
			ssid->leap++;
		else
			ssid->non_leap++;
		num_methods++;
		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	tmp = methods;
	methods = os_realloc_array(methods, num_methods + 1, sizeof(*methods));
	if (methods == NULL) {
		os_free(tmp);
		return -1;
	}
	methods[num_methods].vendor = EAP_VENDOR_IETF;
	methods[num_methods].method = EAP_TYPE_NONE;
	num_methods++;

	if (!errors && ssid->eap.eap_methods) {
		struct eap_method_type *prev_m;
		size_t i, j, prev_methods, match = 0;

		prev_m = ssid->eap.eap_methods;
		for (i = 0; prev_m[i].vendor != EAP_VENDOR_IETF ||
			     prev_m[i].method != EAP_TYPE_NONE; i++) {
			/* Count the methods */
		}
		prev_methods = i + 1;

		for (i = 0; prev_methods == num_methods && i < prev_methods;
		     i++) {
			for (j = 0; j < num_methods; j++) {
				if (prev_m[i].vendor == methods[j].vendor &&
				    prev_m[i].method == methods[j].method) {
					match++;
					break;
				}
			}
		}
		if (match == num_methods) {
			os_free(methods);
			return 1;
		}
	}
	wpa_hexdump(MSG_MSGDUMP, "eap methods",
		    (u8 *) methods, num_methods * sizeof(*methods));
	os_free(ssid->eap.eap_methods);
	ssid->eap.eap_methods = methods;
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_eap(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	int i, ret;
	char *buf, *pos, *end;
	const struct eap_method_type *eap_methods = ssid->eap.eap_methods;
	const char *name;

	if (eap_methods == NULL)
		return NULL;

	pos = buf = os_zalloc(100);
	if (buf == NULL)
		return NULL;
	end = buf + 100;

	for (i = 0; eap_methods[i].vendor != EAP_VENDOR_IETF ||
		     eap_methods[i].method != EAP_TYPE_NONE; i++) {
		name = eap_get_name(eap_methods[i].vendor,
				    (EapType)eap_methods[i].method);
		if (name) {
			ret = os_snprintf(pos, end - pos, "%s%s",
					  pos == buf ? "" : " ", name);
			if (os_snprintf_error(end - pos, ret))
				break;
			pos += ret;
		}
	}
	end[-1] = '\0';

    // Erase old fast_pac, fast_pac_len for EAP-FAST
    wpa_config_write_nvram_string(ENV_FAST_PAC, "");
    wpa_config_write_nvram_int(ENV_FAST_PAC_LEN, 0, 0);
	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_password(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
	u8 *hash;

	if (os_strcmp(value, "NULL") == 0) {
		if (!ssid->eap.password)
			return 1; /* Already unset */
		wpa_printf_dbg(MSG_DEBUG, "Unset configuration string 'password'");
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(ssid->eap.password, ssid->eap.password_len);
#else
		os_free(ssid->eap.password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		ssid->eap.password = NULL;
		ssid->eap.password_len = 0;
		return 0;
	}

#ifdef CONFIG_EXT_PASSWORD
	if (os_strncmp(value, "ext:", 4) == 0) {
		char *name = os_strdup(value + 4);
		if (name == NULL)
			return -1;
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(ssid->eap.password, ssid->eap.password_len);
#else
		os_free(ssid->eap.password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		ssid->eap.password = (u8 *) name;
		ssid->eap.password_len = os_strlen(name);
		ssid->eap.flags &= ~EAP_CONFIG_FLAGS_PASSWORD_NTHASH;
		ssid->eap.flags |= EAP_CONFIG_FLAGS_EXT_PASSWORD;
		return 0;
	}
#endif /* CONFIG_EXT_PASSWORD */

	if (os_strncmp(value, "hash:", 5) != 0) {
		char *tmp;
		size_t res_len;

		tmp = wpa_config_parse_string(value, &res_len);
		if (tmp == NULL) {
			wpa_printf(MSG_ERROR, "Failed to parse "
				   "password.");
			return -1;
		}
		wpa_hexdump_ascii_key(MSG_MSGDUMP, data->name,
				      (u8 *) tmp, res_len);

#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(ssid->eap.password, ssid->eap.password_len);
#else
		os_free(ssid->eap.password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		ssid->eap.password = (u8 *) tmp;
		ssid->eap.password_len = res_len;
		ssid->eap.flags &= ~EAP_CONFIG_FLAGS_PASSWORD_NTHASH;
		ssid->eap.flags &= ~EAP_CONFIG_FLAGS_EXT_PASSWORD;

		return 0;
	}


	/* NtPasswordHash: hash:<32 hex digits> */
	if (os_strlen(value + 5) != 2 * 16) {
		wpa_printf(MSG_ERROR, "Invalid password hash length "
			   "(expected 32 hex digits)");
		return -1;
	}

	hash = os_malloc(16);
	if (hash == NULL)
		return -1;

	if (hexstr2bin(value + 5, hash, 16)) {
		os_free(hash);
		wpa_printf(MSG_ERROR, "Invalid password hash");
		return -1;
	}

	wpa_hexdump_key(MSG_MSGDUMP, data->name, hash, 16);

	if (ssid->eap.password && ssid->eap.password_len == 16 &&
	    os_memcmp(ssid->eap.password, hash, 16) == 0 &&
	    (ssid->eap.flags & EAP_CONFIG_FLAGS_PASSWORD_NTHASH)) {
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(hash, 16);
#else
		os_free(hash);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		return 1;
	}
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(ssid->eap.password, ssid->eap.password_len);
#else
	os_free(ssid->eap.password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
	ssid->eap.password = hash;
	ssid->eap.password_len = 16;
	ssid->eap.flags |= EAP_CONFIG_FLAGS_PASSWORD_NTHASH;
	ssid->eap.flags &= ~EAP_CONFIG_FLAGS_EXT_PASSWORD;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_password(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf;

	if (ssid->eap.password == NULL)
		return NULL;

#ifdef CONFIG_EXT_PASSWORD
	if (ssid->eap.flags & EAP_CONFIG_FLAGS_EXT_PASSWORD) {
		buf = os_zalloc(4 + ssid->eap.password_len + 1);
		if (buf == NULL)
			return NULL;
		os_memcpy(buf, "ext:", 4);
		os_memcpy(buf + 4, ssid->eap.password, ssid->eap.password_len);
		return buf;
	}
#endif /* CONFIG_EXT_PASSWORD */

	if (!(ssid->eap.flags & EAP_CONFIG_FLAGS_PASSWORD_NTHASH)) {
		return wpa_config_write_string(
			ssid->eap.password, ssid->eap.password_len);
	}

	buf = os_malloc(5 + 32 + 1);
	if (buf == NULL)
		return NULL;

	os_memcpy(buf, "hash:", 5);
	wpa_snprintf_hex(buf + 5, 32 + 1, ssid->eap.password, 16);

	return buf;
}
#endif /* NO_CONFIG_WRITE */
#endif /* IEEE8021X_EAPOL */


static int wpa_config_parse_wep_key(u8 *key, size_t *len,
				    const char *value, int idx)
{
	char *buf, title[20];
	int res;

	buf = wpa_config_parse_string(value, len);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "Invalid WEP key %d '%s'.",
			   idx, value);
		return -1;
	}
	if (*len > MAX_WEP_KEY_LEN) {
		wpa_printf(MSG_ERROR, "Too long WEP key %d '%s'.",
			   idx, value);
		os_free(buf);
		return -1;
	}
	if (*len && *len != 5 && *len != 13 && *len != 16) {
		wpa_printf(MSG_ERROR, "Invalid WEP key length %u - "
			   "this network block will be ignored",
			   (unsigned int) *len);
	}
	os_memcpy(key, buf, *len);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(buf);
#else
	os_free(buf);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	res = os_snprintf(title, sizeof(title), "wep_key%d", idx);
	if (!os_snprintf_error(sizeof(title), res))
		wpa_hexdump_key(MSG_MSGDUMP, title, key, *len);
	return 0;
}


static int wpa_config_parse_wep_key0(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(ssid->mode == WPAS_MODE_AP) {
		da16x_cli_prt("[%s] Not allowed WEP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	return wpa_config_parse_wep_key(ssid->wep_key[0],
					&ssid->wep_key_len[0],
					value, 0);
}


static int wpa_config_parse_wep_key1(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(ssid->mode == WPAS_MODE_AP) {
		da16x_cli_prt("[%s] Not allowed WEP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	return wpa_config_parse_wep_key(ssid->wep_key[1],
					&ssid->wep_key_len[1],
					value, 1);
}


static int wpa_config_parse_wep_key2(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(ssid->mode == WPAS_MODE_AP) {
		da16x_cli_prt("[%s] Not allowed WEP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	return wpa_config_parse_wep_key(ssid->wep_key[2],
					&ssid->wep_key_len[2],
					value, 2);
}


static int wpa_config_parse_wep_key3(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(ssid->mode == WPAS_MODE_AP) {
		da16x_cli_prt("[%s] Not allowed WEP in N only mode\n",
				__func__);
		return -1;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	return wpa_config_parse_wep_key(ssid->wep_key[3],
					&ssid->wep_key_len[3],
					value, 3);
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_wep_key(struct wpa_ssid *ssid, int idx)
{
#ifdef CONFIG_AP_WIFI_MODE	//[[ trinity_0160711 -- Issue #100
	if(ssid->mode == WPAS_MODE_AP) {
		da16x_cli_prt("[%s] Not allowed WEP in AP mode\n",
				__func__);
		return NULL;
	}
#endif /* CONFIG_AP_WIFI_MODE */

	if (ssid->wep_key_len[idx] == 0)
		return NULL;
	return wpa_config_write_string(ssid->wep_key[idx],
				       ssid->wep_key_len[idx]);
}


static char * wpa_config_write_wep_key0(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 0);
}


static char * wpa_config_write_wep_key1(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 1);
}


static char * wpa_config_write_wep_key2(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 2);
}


static char * wpa_config_write_wep_key3(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 3);
}
#endif /* NO_CONFIG_WRITE */


#ifdef CONFIG_P2P

static int wpa_config_parse_go_p2p_dev_addr(const struct parse_data *data,
					    struct wpa_ssid *ssid,
					    const char *value)
{
	if (value[0] == '\0' || os_strcmp(value, "\"\"") == 0 ||
	    os_strcmp(value, "any") == 0) {
		os_memset(ssid->go_p2p_dev_addr, 0, ETH_ALEN);
		wpa_printf(MSG_MSGDUMP, "GO P2P Device Address any");
		return 0;
	}
	if (hwaddr_aton(value, ssid->go_p2p_dev_addr)) {
		wpa_printf(MSG_ERROR, "Invalid GO P2P Device Address '%s'.",
			   value);
		return -1;
	}
	ssid->bssid_set = 1;
	wpa_printf(MSG_MSGDUMP, "GO P2P Device Address " MACSTR,
		   MAC2STR(ssid->go_p2p_dev_addr));
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_go_p2p_dev_addr(const struct parse_data *data,
					       struct wpa_ssid *ssid)
{
	char *value;
	int res;

	if (is_zero_ether_addr(ssid->go_p2p_dev_addr))
		return NULL;

	value = os_malloc(20);
	if (value == NULL)
		return NULL;
	res = os_snprintf(value, 20, MACSTR, MAC2STR(ssid->go_p2p_dev_addr));
	if (os_snprintf_error(20, res)) {
		os_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}
#endif /* NO_CONFIG_WRITE */


#ifdef	CONFIG_P2P
static int wpa_config_parse_p2p_client_list(const struct parse_data *data,
					    struct wpa_ssid *ssid,
					    const char *value)
{
	return wpa_config_parse_addr_list(data, value,
					  &ssid->p2p_client_list,
					  &ssid->num_p2p_clients,
					  "p2p_client_list", 0, 0);
}

#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_p2p_client_list(const struct parse_data *data,
					       struct wpa_ssid *ssid)
{
	return (char *)wpa_config_write_addr_list(data, ssid->p2p_client_list,
					  ssid->num_p2p_clients,
					  "p2p_client_list");
}
#endif /* NO_CONFIG_WRITE */
#endif	/* CONFIG_P2P */

static int wpa_config_parse_psk_list(const struct parse_data *data,
				     struct wpa_ssid *ssid,
				     const char *value)
{
	struct psk_list_entry *p;
	const char *pos;

	p = os_zalloc(sizeof(*p));
	if (p == NULL)
		return -1;

	pos = value;
	if (os_strncmp(pos, "P2P-", 4) == 0) {
		p->p2p = 1;
		pos += 4;
	}

	if (hwaddr_aton(pos, p->addr)) {
		wpa_printf(MSG_ERROR, "Invalid psk_list address '%s'",
			   pos);
		os_free(p);
		return -1;
	}
	pos += 17;
	if (*pos != '-') {
		wpa_printf(MSG_ERROR, "Invalid psk_list '%s'",
			   pos);
		os_free(p);
		return -1;
	}
	pos++;

	if (hexstr2bin(pos, p->psk, PMK_LEN) || pos[PMK_LEN * 2] != '\0') {
		wpa_printf(MSG_ERROR, "Invalid psk_list PSK '%s'",
			   pos);
		os_free(p);
		return -1;
	}

	dl_list_add(&ssid->psk_list, &p->list);

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_psk_list(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return NULL;
}
#endif /* NO_CONFIG_WRITE */

#endif /* CONFIG_P2P */


#ifdef CONFIG_MESH

static int wpa_config_parse_mesh_basic_rates(const struct parse_data *data,
					     struct wpa_ssid *ssid,
					     const char *value)
{
	int *rates = wpa_config_parse_int_array(value);

	if (rates == NULL) {
		wpa_printf(MSG_ERROR, "Invalid mesh_basic_rates '%s'",
			   value);
		return -1;
	}
	if (rates[0] == 0) {
		os_free(rates);
		rates = NULL;
	}

	os_free(ssid->mesh_basic_rates);
	ssid->mesh_basic_rates = rates;

	return 0;
}


#ifndef NO_CONFIG_WRITE

static char * wpa_config_write_mesh_basic_rates(const struct parse_data *data,
						struct wpa_ssid *ssid)
{
	return wpa_config_write_freqs(data, ssid->mesh_basic_rates);
}

#endif /* NO_CONFIG_WRITE */

#endif /* CONFIG_MESH */


#ifdef CONFIG_MACSEC

static int wpa_config_parse_mka_cak(const struct parse_data *data,
				    struct wpa_ssid *ssid,
				    const char *value)
{
	if (hexstr2bin(value, ssid->mka_cak, MACSEC_CAK_LEN) ||
	    value[MACSEC_CAK_LEN * 2] != '\0') {
		wpa_printf(MSG_ERROR, "Invalid MKA-CAK '%s'.",
			   value);
		return -1;
	}

	ssid->mka_psk_set |= MKA_PSK_SET_CAK;

	wpa_hexdump_key(MSG_MSGDUMP, "MKA-CAK", ssid->mka_cak, MACSEC_CAK_LEN);
	return 0;
}


static int wpa_config_parse_mka_ckn(const struct parse_data *data,
				    struct wpa_ssid *ssid,
				    const char *value)
{
	if (hexstr2bin(value, ssid->mka_ckn, MACSEC_CKN_LEN) ||
	    value[MACSEC_CKN_LEN * 2] != '\0') {
		wpa_printf(MSG_ERROR, "Invalid MKA-CKN '%s'.",
			   value);
		return -1;
	}

	ssid->mka_psk_set |= MKA_PSK_SET_CKN;

	wpa_hexdump_key(MSG_MSGDUMP, "MKA-CKN", ssid->mka_ckn, MACSEC_CKN_LEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE

static char * wpa_config_write_mka_cak(const struct parse_data *data,
				       struct wpa_ssid *ssid)
{
	if (!(ssid->mka_psk_set & MKA_PSK_SET_CAK))
		return NULL;

	return wpa_config_write_string_hex(ssid->mka_cak, MACSEC_CAK_LEN);
}


static char * wpa_config_write_mka_ckn(const struct parse_data *data,
				       struct wpa_ssid *ssid)
{
	if (!(ssid->mka_psk_set & MKA_PSK_SET_CKN))
		return NULL;
	return wpa_config_write_string_hex(ssid->mka_ckn, MACSEC_CKN_LEN);
}

#endif /* NO_CONFIG_WRITE */

#endif /* CONFIG_MACSEC */


#if 0	/* Not used code */
static int wpa_config_parse_peerkey(const struct parse_data *data,
				    struct wpa_ssid *ssid,
				    const char *value)
{
	wpa_printf(MSG_INFO, "NOTE: Obsolete peerkey parameter ignored");
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_peerkey(const struct parse_data *data,
				       struct wpa_ssid *ssid)
{
	return NULL;
}
#endif /* NO_CONFIG_WRITE */
#endif	/* 0 */


/* Helper macros for network block parser */

#ifdef OFFSET
#undef OFFSET
#endif /* OFFSET */
/* OFFSET: Get offset of a variable within the wpa_ssid structure */
#define OFFSET(v) ((void *) &((struct wpa_ssid *) 0)->v)

/* STR: Define a string variable for an ASCII string; f = field name */
#if 0 //def NO_CONFIG_WRITE
#define _STR(f) #f, wpa_config_parse_str, OFFSET(f)
#define _STRe(f) #f, wpa_config_parse_str, OFFSET(eap.f)
#else /* NO_CONFIG_WRITE */
#define _STR(f) #f, wpa_config_parse_str, wpa_config_write_str, OFFSET(f)
#define _STRe(f) #f, wpa_config_parse_str, wpa_config_write_str, OFFSET(eap.f)
#endif /* NO_CONFIG_WRITE */
#define STR(f) _STR(f), NULL, NULL, NULL, 0
#define STRe(f) _STRe(f), NULL, NULL, NULL, 0
#define STR_KEY(f) _STR(f), NULL, NULL, NULL, 1
#define STR_KEYe(f) _STRe(f), NULL, NULL, NULL, 1

/* STR_LEN: Define a string variable with a separate variable for storing the
 * data length. Unlike STR(), this can be used to store arbitrary binary data
 * (i.e., even nul termination character). */
#define _STR_LEN(f) _STR(f), OFFSET(f ## _len)
#define _STR_LENe(f) _STRe(f), OFFSET(eap.f ## _len)
#define STR_LEN(f) _STR_LEN(f), NULL, NULL, 0
#define STR_LENe(f) _STR_LENe(f), NULL, NULL, 0
#define STR_LEN_KEY(f) _STR_LEN(f), NULL, NULL, 1

/* STR_RANGE: Like STR_LEN(), but with minimum and maximum allowed length
 * explicitly specified. */
#define _STR_RANGE(f, min, max) _STR_LEN(f), (void *) (min), (void *) (max)
#define STR_RANGE(f, min, max) _STR_RANGE(f, min, max), 0
#define STR_RANGE_KEY(f, min, max) _STR_RANGE(f, min, max), 1

#if 0 //def NO_CONFIG_WRITE
#define _INT(f) #f, wpa_config_parse_int, OFFSET(f), (void *) 0
#define _INTe(f) #f, wpa_config_parse_int, OFFSET(eap.f), (void *) 0
#else /* NO_CONFIG_WRITE */
#define _INT(f) #f, wpa_config_parse_int, wpa_config_write_int, \
	OFFSET(f), (void *) 0
#define _INTe(f) #f, wpa_config_parse_int, wpa_config_write_int, \
	OFFSET(eap.f), (void *) 0
#endif /* NO_CONFIG_WRITE */

/* INT: Define an integer variable */
#define INT(f) _INT(f), NULL, NULL, 0
#define INTe(f) _INTe(f), NULL, NULL, 0

/* INT_RANGE: Define an integer variable with allowed value range */
#define INT_RANGE(f, min, max) _INT(f), (void *) (min), (void *) (max), 0

/* FUNC: Define a configuration variable that uses a custom function for
 * parsing and writing the value. */
#if 0 //def NO_CONFIG_WRITE
#define _FUNC(f) #f, wpa_config_parse_ ## f, NULL, NULL, NULL, NULL
#else /* NO_CONFIG_WRITE */
#define _FUNC(f) #f, wpa_config_parse_ ## f, wpa_config_write_ ## f, \
	NULL, NULL, NULL, NULL
#endif /* NO_CONFIG_WRITE */
#define FUNC(f) _FUNC(f), 0
#define FUNC_KEY(f) _FUNC(f), 1

/*
 * Table of network configuration variables. This table is used to parse each
 * network configuration variable, e.g., each line in wpa_supplicant.conf file
 * that is inside a network block.
 *
 * This table is generated using the helper macros defined above and with
 * generous help from the C pre-processor. The field name is stored as a string
 * into .name and for STR and INT types, the offset of the target buffer within
 * struct wpa_ssid is stored in .param1. .param2 (if not NULL) is similar
 * offset to the field containing the length of the configuration variable.
 * .param3 and .param4 can be used to mark the allowed range (length for STR
 * and value for INT).
 *
 * For each configuration line in wpa_supplicant.conf, the parser goes through
 * this table and select the entry that matches with the field name. The parser
 * function (.parser) is then called to parse the actual value of the field.
 *
 * This kind of mechanism makes it easy to add new configuration parameters,
 * since only one line needs to be added into this table and into the
 * struct wpa_ssid definition if the new variable is either a string or
 * integer. More complex types will need to use their own parser and writer
 * functions.
 */
static const struct parse_data ssid_fields[] = {
	{ STR_RANGE(ssid, 0, SSID_MAX_LEN) },
	{ INT_RANGE(scan_ssid, 0, 1) },
	{ FUNC(bssid) },
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ FUNC(bssid_hint) },
	{ FUNC(bssid_blacklist) },
	{ FUNC(bssid_whitelist) },
#endif	/* CONFIG_P2P_UNUSED_CMD */
	{ FUNC_KEY(psk) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT(mem_only_psk) },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef	CONFIG_SAE
	{ STR_KEY(sae_password) },
	{ STR(sae_password_id) },
#endif	/* CONFIG_SAE */
	{ FUNC(proto) },
	{ FUNC(key_mgmt) },
#ifdef	CONFIG_BGSCAN
	{ INT(bg_scan_period) },
#endif	/* CONFIG_BGSCAN */
	{ FUNC(pairwise) },
	{ FUNC(group) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ FUNC(group_mgmt) },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ FUNC(auth_alg) },
	{ FUNC(scan_freq) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ FUNC(freq_list) },
	{ INT_RANGE(ht, 0, 1) },
	{ INT_RANGE(ht40, -1, 1) },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_IEEE80211AC
	{ INT_RANGE(vht, 0, 1) },
#endif /* CONFIG_IEEE80211AC */

#ifdef CONFIG_IEEE80211AC
	{ INT_RANGE(max_oper_chwidth, VHT_CHANWIDTH_USE_HT, VHT_CHANWIDTH_80P80MHZ) },
	{ INT(vht_center_freq1) },
	{ INT(vht_center_freq2) },
#endif /* CONFIG_IEEE80211AC */

#ifdef	CONFIG_EAP_METHOD
	{ FUNC(eap) },
#endif	/* CONFIG_EAP_METHOD */
#ifdef IEEE8021X_EAPOL
	{ STR_LENe(identity) },
	{ STR_LENe(anonymous_identity) },

	{ FUNC_KEY(password) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ STR_LENe(imsi_identity) },
	{ STRe(private_key) },
	{ STR_KEYe(private_key_passwd) },
	{ STRe(dh_file) },
	{ STRe(subject_match) },
	{ STRe(altsubject_match) },
	{ STRe(domain_suffix_match) },
	{ STRe(domain_match) },
	{ STRe(dh_file2) },
	{ STRe(subject_match2) },
	{ STRe(altsubject_match2) },
	{ STRe(domain_suffix_match2) },
	{ STRe(domain_match2) },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ STRe(ca_cert2) },
	{ STRe(ca_path2) },
	{ STRe(client_cert2) },
	{ STRe(private_key2) },
	{ STR_KEYe(private_key2_passwd) },
	{ STRe(phase1) },
	{ STRe(phase2) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ STRe(pcsc) },
	{ STR_KEYe(pin) },
	{ STRe(engine_id) },
	{ STRe(key_id) },
	{ STRe(cert_id) },
	{ STRe(ca_cert_id) },
	{ STR_KEYe(pin2) },
	{ STRe(engine2_id) },
	{ STRe(key2_id) },
	{ STRe(cert2_id) },
	{ STRe(ca_cert2_id) },
	{ INTe(engine) },
	{ INTe(engine2) },
	{ INT(eapol_flags) },
	{ INTe(sim_num) },
	{ STRe(openssl_ciphers) },
	{ INTe(erp) },
#endif	/* CONFIG_SUPP27_CONFIG */
#endif /* IEEE8021X_EAPOL */

	{ FUNC_KEY(wep_key0) },
	{ FUNC_KEY(wep_key1) },
	{ FUNC_KEY(wep_key2) },
	{ FUNC_KEY(wep_key3) },
	{ INT(wep_tx_keyidx) },
#ifdef CONFIG_PRIO_GROUP
	{ INT(priority) },
#endif /* CONFIG_PRIO_GROUP */
#ifdef IEEE8021X_EAPOL
	{ INT(eap_workaround) },
	{ STRe(pac_file) },
	{ INTe(fragment_size) },
	{ INTe(ocsp) },
#endif /* IEEE8021X_EAPOL */
#ifdef CONFIG_MESH
	{ INT_RANGE(mode, 0, 5) },
	{ INT_RANGE(no_auto_peer, 0, 1) },
	{ INT_RANGE(mesh_rssi_threshold, -255, 1) },
#else /* CONFIG_MESH */
	{ INT_RANGE(mode, 0, 4) },
#endif /* CONFIG_MESH */
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT_RANGE(proactive_key_caching, 0, 1) },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ INT_RANGE(disabled, 0, 2) },
	{ STR(id_str) },
#ifdef CONFIG_IEEE80211W
	{ INT_RANGE(ieee80211w, 0, 2) },
#endif /* CONFIG_IEEE80211W */
#ifdef	CONFIG_SUPP27_CONFIG
	{ FUNC(peerkey) /* obsolete - removed */ },
	{ INT_RANGE(mixed_cell, 0, 1) },
#endif	/* CONFIG_SUPP27_CONFIG */
	{ INT_RANGE(frequency, 0, 65000) },
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT_RANGE(fixed_freq, 0, 1) },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_ACS
	{ INT_RANGE(acs, 0, 1) },
#endif /* CONFIG_ACS */
#ifdef CONFIG_MESH
	{ FUNC(mesh_basic_rates) },
	{ INT(dot11MeshMaxRetries) },
	{ INT(dot11MeshRetryTimeout) },
	{ INT(dot11MeshConfirmTimeout) },
	{ INT(dot11MeshHoldingTimeout) },
#endif /* CONFIG_MESH */
	{ INT(wpa_ptk_rekey) },
	{ INT(group_rekey) },
#ifdef	CONFIG_BGSCAN
	{ STR(bgscan) },
#endif	/* CONFIG_BGSCAN */
	{ INT_RANGE(ignore_broadcast_ssid, 0, 2) },
#ifdef CONFIG_P2P
	{ FUNC(go_p2p_dev_addr) },
	{ FUNC(p2p_client_list) },
	{ FUNC(psk_list) },
#endif /* CONFIG_P2P */
#ifdef CONFIG_HT_OVERRIDES
	{ INT_RANGE(disable_ht, 0, 1) },
#ifdef CONFIG_AP_WIFI_MODE
	{ INT_RANGE(wifi_mode, 0, 5) },
#endif /* CONFIG_AP_WIFI_MODE */
	{ INT_RANGE(disable_sgi, 0, 1) },
	{ INT_RANGE(disable_ldpc, 0, 1) },
	{ INT_RANGE(ht40_intolerant, 0, 1) },
	{ INT_RANGE(disable_max_amsdu, -1, 1) },
	{ INT_RANGE(ampdu_factor, -1, 3) },
	{ INT_RANGE(ampdu_density, -1, 7) },
	{ STR(ht_mcs) },
#endif /* CONFIG_HT_OVERRIDES */

#ifdef CONFIG_VHT_OVERRIDES
	{ INT_RANGE(disable_vht, 0, 1) },
	{ INT(vht_capa) },
	{ INT(vht_capa_mask) },
	{ INT_RANGE(vht_rx_mcs_nss_1, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_2, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_3, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_4, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_5, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_6, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_7, -1, 3) },
	{ INT_RANGE(vht_rx_mcs_nss_8, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_1, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_2, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_3, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_4, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_5, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_6, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_7, -1, 3) },
	{ INT_RANGE(vht_tx_mcs_nss_8, -1, 3) },
#endif /* CONFIG_VHT_OVERRIDES */
	{ INT(ap_max_inactivity) },
	{ INT_RANGE(dtim_period, 1, 100) },
	{ INT_RANGE(beacon_int, 20, 1000) }, /* 20~1000 */
#ifdef CONFIG_AUTH_MAX_FAILURES
	{ INT(auth_max_failures) },
#endif // CONFIG_AUTH_MAX_FAILURES
#ifdef CONFIG_AP_ISOLATION
	{ INT_RANGE(isolate, 0, 1) },
#endif /* CONFIG_AP_ISOLATION */

#ifdef CONFIG_AP_POWER
	{ FUNC(ap_power) },
#endif /* CONFIG_AP_POWER */
#ifdef CONFIG_MACSEC
	{ INT_RANGE(macsec_policy, 0, 1) },
	{ INT_RANGE(macsec_integ_only, 0, 1) },
	{ INT_RANGE(macsec_port, 1, 65534) },
	{ INT_RANGE(mka_priority, 0, 255) },
	{ FUNC_KEY(mka_cak) },
	{ FUNC_KEY(mka_ckn) },
#endif /* CONFIG_MACSEC */
#ifdef CONFIG_HS20
	{ INT(update_identifier) },
	{ STR_RANGE(roaming_consortium_selection, 0, MAX_ROAMING_CONS_OI_LEN) },
#endif /* CONFIG_HS20 */
#ifdef CONFIG_RANDOM_ADDR
	{ INT_RANGE(mac_addr, 0, 2) },
#endif /* CONFIG_RANDOM_ADDR */
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT_RANGE(pbss, 0, 2) },
	{ INT_RANGE(wps_disabled, 0, 1) },
	{ INT_RANGE(fils_dh_group, 0, 65535) },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_DPP
	{ STR(dpp_connector) },
	{ STR_LEN(dpp_netaccesskey) },
	{ INT(dpp_netaccesskey_expiry) },
	{ STR_LEN(dpp_csign) },
#endif /* CONFIG_DPP */
#ifdef CONFIG_OWE
	{ INT_RANGE(owe_group, 0, 65535) },
	{ INT_RANGE(owe_only, 0, 1) },
#endif /* CONFIG_OWE */
};

#undef OFFSET
#undef _STR
#undef STR
#undef STR_KEY
#undef _STR_LEN
#undef STR_LEN
#undef STR_LEN_KEY
#undef _STR_RANGE
#undef STR_RANGE
#undef STR_RANGE_KEY
#undef _INT
#undef INT
#undef INT_RANGE
#undef _FUNC
#undef FUNC
#undef FUNC_KEY
#define NUM_SSID_FIELDS ARRAY_SIZE(ssid_fields)


int country_ch_pwr_MinMax (char* country, int *min, int *max)
{
	int ret = -1;
	unsigned int i = 0;
	
	int t_min = -1;
	int t_max = -1;
	
	for (i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
		const struct country_ch_power_level *field = (void*)cc_power_table(i);

		if (os_strcasecmp(country, field->country) == 0) {

			for (int ch_num = 0; ch_num < 14; ch_num++) {
 
				/* Min */
				if (field->ch[ch_num] >= 0x1 && field->ch[ch_num] < 0xf && t_min == -1) {
					t_min = field->ch[ch_num];
				} else {
				
					if (t_min > field->ch[ch_num] && field->ch[ch_num] != 0x0) {
						t_min = field->ch[ch_num];	
					}
				}

				/* Max */
				if (field->ch[ch_num] >= 0x1 && field->ch[ch_num] < 0xf && t_max == -1) {
					t_max = field->ch[ch_num];
				} else {
					if (field->ch[ch_num] == 0x0) {
						t_max = 0x0;
					} else if (t_max < field->ch[ch_num]) {
						t_max = field->ch[ch_num];	
					}
				}
			}
			ret = 0;
		}
	}
	return ret;
}


/* ï¿½ï¿½ï¿½Óµï¿½ Channel ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ Ã£ï¿½ï¿½ ï¿½Ô¼ï¿½ï¿½ï¿½. 5GHz ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½Ê¿ï¿½ */
/* cc_power_level.ch[x] ï¿½ï¿½ï¿½ï¿½ 0xf  ï¿½ï¿½ ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?ï¿½Ê´ï¿½ Ã¤ï¿½ï¿½ ï¿½ï¿½. */
int country_channel_range (char* country, int *startCH, int *endCH)
{
	int ret = -1;
	unsigned int i = 0;
	
	int t_startCH = -1;
	int t_endCH = -1;
	
	for (i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
		const struct country_ch_power_level *field = (void*)cc_power_table(i);

		if (os_strcmp(country, field->country) == 0) {
			
			for (int ch_num = 0; ch_num < 14; ch_num++) {

				/* Start Ch. Ã£ï¿½ï¿½ */
				if (field->ch[ch_num] < 0xf && t_startCH == -1) {
					t_startCH = ch_num + 1;
				}

				/* End Ch. Ã£ï¿½ï¿½ */
				if (field->ch[ch_num] < 0xf) {
					t_endCH = ch_num + 1;
				}
			}
			ret = 0;
		}
	}

	*startCH = t_startCH;
	*endCH = t_endCH;

	//da16x_notice_prt("%s %d~%d\n", country, *firstCH, *finshCH);
	return ret;
}


void printf_country_data(void)
{
	unsigned int i = 0;
	int j = 0, k = 0;
	char ch_pair_set0[128] = {0, }; // 1 - 11
	char ch_pair_set1[700] = {0, }; // 1 - 13
#if 0	/* by Bright : Not supported channel ranges */
	char ch_pair_set2[160] = {0, }; // 1 - 14
	char ch_pair_set3[16] = {0, }; // 5 - 13
	char ch_pair_set4[4] = {0, }; // ETC.
#endif	/* 0 */

	da16x_notice_prt("\nCountry Code to support ch:\n");

	while (i < NUM_SUPPORTS_COUNTRY - 1) {
		const struct country_ch_power_level *field = (void*)cc_power_table(i);


		int min_chan, max_chan;
		country_channel_range(field->country, &min_chan, &max_chan);
		

		if (min_chan == 1 && max_chan == 11) {
			if (strlen(ch_pair_set0))
				strcat(ch_pair_set0, ", ");
			if (j == 15) {
				strcat(ch_pair_set0, "\n\t");
				j = 0;
			}
			strcat(ch_pair_set0, field->country);
			j++;

		} else if (min_chan == 1 && max_chan == 13) {
			if (strlen(ch_pair_set1))
				strcat(ch_pair_set1, ", ");
			if (k == 15) {
				strcat(ch_pair_set1, "\n\t");
				k = 0;
			}
			strcat(ch_pair_set1, field->country);
			k++;

#if 0	/* by Bright : Not supported channel ranges */
		} else if (min_chan == 1 && max_chan == 14) {
			if (strlen(ch_pair_set2))
				strcat(ch_pair_set2, ", ");
			strcat(ch_pair_set2, field->country);

		} else if (min_chan == 5 && max_chan == 13) {
			if (strlen(ch_pair_set3))
				strcat(ch_pair_set3, ", ");
			strcat(ch_pair_set3, field->country);
#endif	/* 0 */
		}
		i++;
	}

	da16x_notice_prt("\t[1 - 11]\n\t%s\n\n", ch_pair_set0);
	da16x_notice_prt("\t[1 - 13]\n\t%s\n\n", ch_pair_set1);
#if 0	/* by Bright : Not supported channel ranges */
	da16x_notice_prt("\t[1 - 14]\n\t%s\n\n", ch_pair_set2);
	da16x_notice_prt("\t[5 - 13]\n\t%s\n\n", ch_pair_set3);
#endif	/* 0 */
}

/**
 * chk_channel_by_country
 * @country: Pointer to country string
 * @set_channel: channel or frequency and -1 (-1 is don't care channel value. Just check country code.)
 * @mode: 1 frequency (GHz) / 0 Channel (1~14)
 * @rec_channel: recommanded channel in the specific country
 * Returns: channel or frequency value
 */
int chk_channel_by_country(char *country, int set_channel, int mode, int *rec_channel)
{
#if 0	/* by Shingu 20161024 (Concurrent) */
	int ret_channel = -1;
#endif	/* 0 */
	unsigned int i = 0;

#ifdef CONFIG_ACS
	if (set_channel == 0) {
		for (i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
			const struct country_ch_power_level *field = (void*)cc_power_table(i);

			if (os_strcasecmp(country, field->country) == 0)
				return set_channel;
		}
		printf_country_data();
		da16x_err_prt("Incorrect Country Code (%s)\n", country);
		return -1;
	}
#endif /* CONFIG_ACS */


	if (mode) {
#if 1
		set_channel = i3ed11_freq_to_ch(set_channel);
#else
		if (set_channel == 2484)
			set_channel = 14;
		else if ((set_channel < 2412 || set_channel > 2472) ||
			 (set_channel - 2) % 5 != 0)
			return -1;
		else
			set_channel = (set_channel - 2412) / 5 + 1;
#endif /* 1 */
	}

	for (i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
		//const struct country_ch_power_level *field = &cc_power_level[i];
		const struct country_ch_power_level *field = (void*)cc_power_table(i);

		int min_chan, max_chan;
		
		country_channel_range(field->country, &min_chan, &max_chan);

		if (os_strcasecmp(country, field->country) != 0)
			continue;

		if (set_channel == -1)
			return 0;

		if (min_chan <= set_channel && max_chan >= set_channel)
		{
			if (mode) {
#if 1
				return i3ed11_ch_to_freq(set_channel, I3ED11_BAND_2GHZ);
#else
				if (set_channel == 14)
					return 2484;
				else
					return 2407 + set_channel * 5;
#endif /* 1 */
			} else {
				return set_channel;
			}
		} else {
#if 0	/* by Shingu 20161024 (Concurrent) */
			if (set_channel == NOTAPPLICABLE_CHANNEL)
				return 0;

			if (mode)
				ret_channel = 2407 + field->min_chan * 5;
			else
				ret_channel = field->min_chan;
			goto set_default;
#else
			if (rec_channel) {
				
				if (mode) {
					*rec_channel = i3ed11_ch_to_freq(min_chan, I3ED11_BAND_2GHZ);
				} else {
					*rec_channel = min_chan;
				}
			}
			break;
#endif	/* 0 */
		}
	}

	if (i >= NUM_SUPPORTS_COUNTRY) {
		da16x_err_prt("Incorrect Country Code!!\n");
		*country = NULL;
		printf_country_data();
		return NOTAPPLICABLE_CHANNEL;
	}
#if 0	/* by Shingu 20161024 (Concurrent) */
set_default :
	da16x_info_prt("\nCountry Code (%s) Not Supports this %s \n",
		      country, mode? "Frequency":"Channel");
	da16x_info_prt("Set Default %s = [%d] \n",
		      mode ? "Frequency":"Channel", ret_channel);
	return ret_channel;
#else
	da16x_notice_prt("Current country code \"%s\" does not support ch %d.\n",
			country, set_channel);

	return NOTINCLUDED_CHANNEL;
#endif	/* 0 */
}

#ifdef CONFIG_PRIO_GROUP
/**
 * wpa_config_add_prio_network - Add a network to priority lists
 * @config: Configuration data from wpa_config_read()
 * @ssid: Pointer to the network configuration to be added to the list
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to add a network block to the priority list of
 * networks. This must be called for each network when reading in the full
 * configuration. In addition, this can be used indirectly when updating
 * priorities by calling wpa_config_update_prio_list().
 */
int wpa_config_add_prio_network(struct wpa_config *config,
				struct wpa_ssid *ssid)
{
	int prio;
	struct wpa_ssid *prev, **nlist;

	/*
	 * Add to an existing priority list if one is available for the
	 * configured priority level for this network.
	 */
	for (prio = 0; prio < config->num_prio; prio++) {
		prev = config->pssid[prio];
		if (prev->priority == ssid->priority) {
			while (prev->pnext)
				prev = prev->pnext;
			prev->pnext = ssid;
			return 0;
		}
	}

	/* First network for this priority - add a new priority list */
	nlist = os_realloc_array(config->pssid, config->num_prio + 1,
				 sizeof(struct wpa_ssid *));
	if (nlist == NULL)
		return -1;

	for (prio = 0; prio < config->num_prio; prio++) {
		if (nlist[prio]->priority < ssid->priority) {
			os_memmove(&nlist[prio + 1], &nlist[prio],
				   (config->num_prio - prio) *
				   sizeof(struct wpa_ssid *));
			break;
		}
	}

	nlist[prio] = ssid;
	config->num_prio++;
	config->pssid = nlist;

	return 0;
}


/**
 * wpa_config_update_prio_list - Update network priority list
 * @config: Configuration data from wpa_config_read()
 * Returns: 0 on success, -1 on failure
 *
 * This function is called to update the priority list of networks in the
 * configuration when a network is being added or removed. This is also called
 * if a priority for a network is changed.
 */
int wpa_config_update_prio_list(struct wpa_config *config)
{
	struct wpa_ssid *ssid;
	int ret = 0;

	os_free(config->pssid);
	config->pssid = NULL;
	config->num_prio = 0;

	ssid = config->ssid;
	while (ssid) {
		ssid->pnext = NULL;
		if (wpa_config_add_prio_network(config, ssid) < 0)
			ret = -1;
		ssid = ssid->next;
	}

	return ret;
}
#endif /* CONFIG_PRIO_GROUP */

#ifdef IEEE8021X_EAPOL
static void eap_peer_config_free(struct eap_peer_config *eap)
{
	os_free(eap->eap_methods);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(eap->identity, eap->identity_len);
#else
	os_free(eap->identity);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
	os_free(eap->anonymous_identity);
	os_free(eap->imsi_identity);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(eap->password, eap->password_len);
#else
	os_free(eap->password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
	os_free(eap->ca_cert);
	os_free(eap->ca_path);
	os_free(eap->client_cert);
	os_free(eap->private_key);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(eap->private_key_passwd);
#else
	os_free(eap->private_key_passwd);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(eap->dh_file);
	os_free(eap->subject_match);
	os_free(eap->altsubject_match);
	os_free(eap->domain_suffix_match);
	os_free(eap->domain_match);
	os_free(eap->ca_cert2);
	os_free(eap->ca_path2);
	os_free(eap->client_cert2);
	os_free(eap->private_key2);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(eap->private_key2_passwd);
#else
	os_free(eap->private_key2_passwd);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(eap->dh_file2);
	os_free(eap->subject_match2);
	os_free(eap->altsubject_match2);
	os_free(eap->domain_suffix_match2);
	os_free(eap->domain_match2);
	os_free(eap->phase1);
	os_free(eap->phase2);
	os_free(eap->pcsc);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(eap->pin);
#else
	os_free(eap->pin);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(eap->engine_id);
	os_free(eap->key_id);
	os_free(eap->cert_id);
	os_free(eap->ca_cert_id);
	os_free(eap->key2_id);
	os_free(eap->cert2_id);
	os_free(eap->ca_cert2_id);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(eap->pin2);
#else
	os_free(eap->pin2);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(eap->engine2_id);
	os_free(eap->otp);
	os_free(eap->pending_req_otp);
	os_free(eap->pac_file);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(eap->new_password, eap->new_password_len);
#else
	os_free(eap->new_password);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
#ifdef	CONFIG_EXT_SIM
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(eap->external_sim_resp);
#else
	os_free(eap->external_sim_resp);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
#endif	/* CONFIG_EXT_SIM */
	os_free(eap->openssl_ciphers);
}
#endif /* IEEE8021X_EAPOL */


/**
 * wpa_config_free_ssid - Free network/ssid configuration data
 * @ssid: Configuration data for the network
 *
 * This function frees all resources allocated for the network configuration
 * data.
 */
void wpa_config_free_ssid(struct wpa_ssid *ssid)
{
	struct psk_list_entry *psk;

	os_free(ssid->ssid);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(ssid->passphrase);
#else
	os_free(ssid->passphrase);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(ssid->ext_psk);
#ifdef CONFIG_SAE
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(ssid->sae_password);
#else
	os_free(ssid->sae_password);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
	os_free(ssid->sae_password_id);
#endif /* CONFIG_SAE */
#ifdef IEEE8021X_EAPOL
	eap_peer_config_free(&ssid->eap);
#endif /* IEEE8021X_EAPOL */
	os_free(ssid->id_str);
	os_free(ssid->scan_freq);
	os_free(ssid->freq_list);
#ifdef	CONFIG_BGSCAN
	os_free(ssid->bgscan);
#endif	/* CONFIG_BGSCAN */
#ifdef CONFIG_P2P
	os_free(ssid->p2p_client_list);
#endif	/* CONFIG_P2P */
#ifdef	CONFIG_P2P_UNUSED_CMD
	os_free(ssid->bssid_blacklist);
	os_free(ssid->bssid_whitelist);
#endif	/* CONFIG_P2P_UNUSED_CMD */
#ifdef CONFIG_HT_OVERRIDES
	os_free(ssid->ht_mcs);
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_MESH
	os_free(ssid->mesh_basic_rates);
#endif /* CONFIG_MESH */
#ifdef CONFIG_HS20
	os_free(ssid->roaming_consortium_selection);
#endif /* CONFIG_HS20 */
#ifdef CONFIG_DPP
	os_free(ssid->dpp_connector);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(ssid->dpp_netaccesskey, ssid->dpp_netaccesskey_len);
#else
	os_free(ssid->dpp_netaccesskey);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
	os_free(ssid->dpp_csign);
#endif /* CONFIG_DPP */
	while ((psk = dl_list_first(&ssid->psk_list, struct psk_list_entry,
				    list))) {
		dl_list_del(&psk->list);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(psk, sizeof(*psk));
#else
		os_free(psk);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
	}
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(ssid, sizeof(*ssid));
#else
	os_free(ssid);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
}


#ifdef CONFIG_WPA_CRED
void wpa_config_free_cred(struct wpa_cred *cred)
{
	size_t i;

#ifdef	CONFIG_INTERWORKING
	os_free(cred->realm);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(cred->username);
	str_clear_free(cred->password);
#else
	os_free(cred->username);
	os_free(cred->password);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
#endif	/* CONFIG_INTERWORKING */
	os_free(cred->ca_cert);
	os_free(cred->client_cert);
	os_free(cred->private_key);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(cred->private_key_passwd);
#else
	os_free(cred->private_key_passwd);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
#ifdef CONFIG_PCSC_FUNCS
	os_free(cred->imsi);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(cred->milenage);
#else
	os_free(cred->milenage);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
#endif /* CONFIG_PCSC_FUNCS */
	for (i = 0; i < cred->num_domain; i++)
		os_free(cred->domain[i]);
	os_free(cred->domain);
	os_free(cred->domain_suffix_match);
	os_free(cred->eap_method);
	os_free(cred->phase1);
	os_free(cred->phase2);

#ifdef	CONFIG_INTERWORKING
	os_free(cred->excluded_ssid);
#endif	/* CONFIG_INTERWORKING */

#ifdef	CONFIG_ROAMING_PARTNER
	os_free(cred->roaming_partner);
#endif	/* CONFIG_ROAMING_PARTNER */

	os_free(cred->provisioning_sp);
	for (i = 0; i < cred->num_req_conn_capab; i++)
		os_free(cred->req_conn_capab_port[i]);
	os_free(cred->req_conn_capab_port);
	os_free(cred->req_conn_capab_proto);
	os_free(cred);
}
#endif /* CONFIG_WPA_CRED */


#ifndef CONFIG_NO_CONFIG_BLOBS
void wpa_config_flush_blobs(struct wpa_config *config)
{
	struct wpa_config_blob *blob, *prev;

	blob = config->blobs;
	config->blobs = NULL;
	while (blob) {
		prev = blob;
		blob = blob->next;
		wpa_config_free_blob(prev);
	}
}
#endif /* CONFIG_NO_CONFIG_BLOBS */


/**
 * wpa_config_free - Free configuration data
 * @config: Configuration data from wpa_config_read()
 *
 * This function frees all resources allocated for the configuration data by
 * wpa_config_read().
 */
void wpa_config_free(struct wpa_config *config)
{
	struct wpa_ssid *ssid, *prev = NULL;
#ifdef CONFIG_WPA_CRED
	struct wpa_cred *cred, *cprev;
#endif /* CONFIG_WPA_CRED */
	int i;

	ssid = config->ssid;
	while (ssid) {
		prev = ssid;
		ssid = ssid->next;
		wpa_config_free_ssid(prev);
	}

#ifdef CONFIG_WPA_CRED
	cred = config->cred;
	while (cred) {
		cprev = cred;
		cred = cred->next;
		wpa_config_free_cred(cprev);
	}
#endif /* CONFIG_WPA_CRED */

#ifndef CONFIG_NO_CONFIG_BLOBS
	wpa_config_flush_blobs(config);
#endif /* CONFIG_NO_CONFIG_BLOBS */

#ifdef	CONFIG_WPS
	wpabuf_free(config->wps_vendor_ext_m1);
	for (i = 0; i < MAX_WPS_VENDOR_EXT; i++)
		wpabuf_free(config->wps_vendor_ext[i]);
#endif	/* CONFIG_WPS */

	os_free(config->ctrl_interface);
#ifdef CONFIG_SUPP27_IFACE
	os_free(config->ctrl_interface_group);
#endif /* CONFIG_SUPP27_IFACE */
#ifdef CONFIG_OPENSSL_MOD
	os_free(config->opensc_engine_path);
	os_free(config->pkcs11_engine_path);
	os_free(config->pkcs11_module_path);
	os_free(config->openssl_ciphers);
#endif /* CONFIG_OPENSSL_MOD */
#ifdef CONFIG_PCSC_FUNCS
	os_free(config->pcsc_reader);
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
	str_clear_free(config->pcsc_pin);
#else
	os_free(config->pcsc_pin);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
#endif /* CONFIG_PCSC_FUNCS */

#ifdef	CONFIG_WPS
	if (config->driver_param)
	{
		os_free(config->driver_param);
		config->driver_param = NULL;
	}

	if (config->device_name)
	{
		os_free(config->device_name);
		config->device_name = NULL;
	}

	if (config->manufacturer)
	{
		os_free(config->manufacturer);
		config->manufacturer = NULL;
	}

	if (config->model_name)
	{
		os_free(config->model_name);
		config->model_name = NULL;
	}

	if (config->driver_param)
	{
		os_free(config->model_number);
		config->model_number = NULL;
	}

	if (config->serial_number)
	{
		os_free(config->serial_number);
		config->serial_number = NULL;
	}		

	if (config->config_methods)
	{
		os_free(config->config_methods);
		config->config_methods = NULL;
	}
#endif	/* CONFIG_WPS */
#ifdef	CONFIG_P2P
	//os_free(config->p2p_ssid_postfix);
#endif	/* CONFIG_P2P */

#ifdef CONFIG_PRIO_GROUP
	os_free(config->pssid);
#endif /* CONFIG_PRIO_GROUP */
#ifdef	CONFIG_P2P_UNUSED_CMD
	os_free(config->p2p_pref_chan);
	os_free(config->p2p_no_go_freq.range);
#endif	/* CONFIG_P2P_UNUSED_CMD */
	os_free(config->autoscan);
	os_free(config->freq_list);
#ifdef CONFIG_WPS_NFC
	wpabuf_free(config->wps_nfc_dh_pubkey);
	wpabuf_free(config->wps_nfc_dh_privkey);
	wpabuf_free(config->wps_nfc_dev_pw);
#endif /* CONFIG_WPS_NFC */
#ifdef CONFIG_EXT_PASSWORD
	os_free(config->ext_password_backend);
#endif /* CONFIG_EXT_PASSWORD */
#ifdef CONFIG_SAE
	os_free(config->sae_groups);
#endif /* CONFIG_SAE */
	wpabuf_free(config->ap_vendor_elements);
	os_free(config->osu_dir);
#ifdef CONFIG_BGSCAN
	os_free(config->bgscan);
#endif /* CONFIG_BGSCAN */
#ifdef CONFIG_WOWLAN
	os_free(config->wowlan_triggers);
#endif /* CONFIG_WOWLAN */
#ifdef CONFIG_FST
	os_free(config->fst_group_id);
#endif /* CONFIG_FST */
#ifdef CONFIG_SCHED_SCAN
	os_free(config->sched_scan_plans);
#endif /* CONFIG_SCHED_SCAN */
#ifdef CONFIG_MBO
	os_free(config->non_pref_chan);
#endif /* CONFIG_MBO */

	os_free(config);
}


/**
 * wpa_config_foreach_network - Iterate over each configured network
 * @config: Configuration data from wpa_config_read()
 * @func: Callback function to process each network
 * @arg: Opaque argument to pass to callback function
 *
 * Iterate over the set of configured networks calling the specified
 * function for each item. We guard against callbacks removing the
 * supplied network.
 */
void wpa_config_foreach_network(struct wpa_config *config,
				void (*func)(void *, struct wpa_ssid *),
				void *arg)
{
	struct wpa_ssid *ssid, *next;

	ssid = config->ssid;
	while (ssid) {
		next = ssid->next;
		func(arg, ssid);
		ssid = next;
	}
}


/**
 * wpa_config_get_network - Get configured network based on id
 * @config: Configuration data from wpa_config_read()
 * @id: Unique network id to search for
 * Returns: Network configuration or %NULL if not found
 */
struct wpa_ssid * wpa_config_get_network(struct wpa_config *config, int id)
{
	struct wpa_ssid *ssid;

	ssid = config->ssid;
	while (ssid) {
		if (id == ssid->id)
			break;
		ssid = ssid->next;
	}

	return ssid;
}


/**
 * wpa_config_add_network - Add a new network with empty configuration
 * @config: Configuration data from wpa_config_read()
 * Returns: The new network configuration or %NULL if operation failed
 */
#if 1 /* FC9000 only */
struct wpa_ssid * wpa_config_add_network(struct wpa_config *config,
		char *ifname, int index)
{
	int id = -1;
	struct wpa_ssid *ssid, *prev = NULL;

	if ((index >= FIXED_NETWORK_ID_STA)
#ifdef CONFIG_MESH
			&& (index <= FIXED_NETWORK_ID_MESH_POINT)
#else
			&& (index <= FIXED_NETWORK_ID_P2P)
#endif /* CONFIG_MESH */
			) {
		id = index;
	} else {
		if (os_strcmp(ifname, STA_DEVICE_NAME) == 0) {
			id = FIXED_NETWORK_ID_STA;
		}
#ifdef	CONFIG_AP
		else if (os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0) {
			id = FIXED_NETWORK_ID_AP;
		}
#endif	/* CONFIG_AP */
#ifdef	CONFIG_P2P
		else if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0) {
			id = FIXED_NETWORK_ID_P2P;
		}
#endif	/* CONFIG_P2P */
#ifdef	CONFIG_MESH
		else if (os_strcmp(ifname, MESH_POINT_DEVICE_NAME) == 0
		  || index == FIXED_NETWORK_ID_MESH_POINT) {
			id = FIXED_NETWORK_ID_MESH_POINT;
		}
#endif	/* CONFIG_P2P */
		else {
			da16x_err_prt("[%s] Unknown ID %d [STA:0 / AP:1"
#ifdef	CONFIG_P2P
				" / P2P:2"
#endif	/* CONFIG_P2P */
#ifdef CONFIG_MESH
				" / MESH : 3"
#endif /* CONFIG_MESH */
				"]\n", __func__, id);
			return NULL;
		}
	}

	ssid = config->ssid;

	while (ssid) {
		if (ssid->id == id) {
			wpa_config_remove_network(config, id);
			ssid = config->ssid;
			break;
		}
		prev = ssid;
		ssid = ssid->next;
	}

	ssid = os_zalloc(sizeof(*ssid));

	if (ssid == NULL)
		return NULL;
	ssid->id = id;
	dl_list_init(&ssid->psk_list);

	if (prev)
		prev->next = ssid;
	else
		config->ssid = ssid;

#ifdef CONFIG_PRIO_GROUP
	wpa_config_update_prio_list(config);
#endif /* CONFIG_PRIO_GROUP */

	return ssid;
}

#else
struct wpa_ssid * wpa_config_add_network(struct wpa_config *config)
{
	int id;
	struct wpa_ssid *ssid, *last = NULL;

	id = -1;
	ssid = config->ssid;
	while (ssid) {
		if (ssid->id > id)
			id = ssid->id;
		last = ssid;
		ssid = ssid->next;
	}
	id++;

	ssid = os_zalloc(sizeof(*ssid));
	if (ssid == NULL)
		return NULL;
	ssid->id = id;
	dl_list_init(&ssid->psk_list);
	if (last)
		last->next = ssid;
	else
		config->ssid = ssid;

	wpa_config_update_prio_list(config);

	return ssid;
}
#endif /* FC9000 only */


/**
 * wpa_config_remove_network - Remove a configured network based on id
 * @config: Configuration data from wpa_config_read()
 * @id: Unique network id to search for
 * Returns: 0 on success, or -1 if the network was not found
 */
int wpa_config_remove_network(struct wpa_config *config, int id)
{
	struct wpa_ssid *ssid, *prev = NULL;

	ssid = config->ssid;
	while (ssid) {
		if (id == ssid->id)
			break;
		prev = ssid;
		ssid = ssid->next;
	}

	if (ssid == NULL)
		return -1;

	if (prev)
		prev->next = ssid->next;
	else
		config->ssid = ssid->next;
#ifdef CONFIG_PRIO_GROUP
	wpa_config_update_prio_list(config);
#endif /* CONFIG_PRIO_GROUP */
	wpa_config_free_ssid(ssid);
	return 0;
}

void wpa_config_set_global_defaults(struct wpa_config *config)
{
	extern u16 da16x_tls_version;

#ifdef CONFIG_LOG_MASK
	set_log_mask(0, 0);
#endif /* CONFIG_LOG_MASK */
	strcpy(config->country, DEFAULT_AP_COUNTRY);

#ifdef	CONFIG_ACL
	config->macaddr_acl = MACADDR_ACL_DEFAULT;
	memset(config->acl_mac, 0, sizeof(struct mac_acl_entry));
	config->acl_num = 0;
#endif /* CONFIG_ACL */

#ifdef	CONFIG_P2P
	config->p2p_oper_reg_class = 81;
	config->p2p_oper_channel = DEFAULT_P2P_OPER_CHANNEL;
	config->p2p_listen_reg_class = 81;
	config->p2p_listen_channel = DEFAULT_P2P_LISTEN_CHANNEL;

	config->p2p_go_intent = DEFAULT_P2P_GO_INTENT;
	config->p2p_find_timeout = DEFAULT_P2P_FIND_TIMEOUT;
#endif	/* CONFIG_P2P */

#ifdef	CONFIG_AP
	config->ap_max_inactivity = AP_MAX_INACTIVITY;
	config->ap_send_ka = 0;

#ifdef	CONFIG_AP_WMM
	config->hostapd_wmm_enabled = 1;
	config->hostapd_wmm_ps_enabled = 0;
#endif	/* CONFIG_AP_WMM */

#ifdef	CONFIG_AP_PARAMETERS
	config->greenfield = DEFAULT_GREENFIELD;
	config->ht_protection = DEFAULT_HT_PROTECTION;
	config->rts_threshold = DEFAULT_RTS_THRESHOLD;
#endif	/* CONFIG_AP_PARAMETERS */
#endif	/* CONFIG_AP */

#ifdef CONFIG_ALLOW_ANY_OPEN_AP
	config->allow_any_open_ap = 0;
#endif

#ifdef CONFIG_SIMPLE_ROAMING
	config->roam_state = ROAM_DISABLE;
	config->roam_threshold = ROAM_RSSI_THRESHOLD;
#endif /* CONFIG_SIMPLE_ROAMING */

#ifdef CONFIG_5G_SUPPORT
	config->band = WPA_SETBAND_AUTO;
#endif /* CONFIG_5G_SUPPORT */

    config->dot11RSNAConfigPMKLifetime = DEFAULT_PMK_LIFE_TIME;

	//Set default tls version
	if ((da16x_tls_version != 0x301)
		&& (da16x_tls_version != 0x302)
		&& (da16x_tls_version != 0x303)) {
#if defined(MBEDTLS_SSL_PROTO_TLS1)
		da16x_tls_version = 0x301;
#elif defined(MBEDTLS_SSL_PROTO_TLS1_1)
		da16x_tls_version = 0x302;
#else
		da16x_tls_version = 0x303;
#endif
	}
}


void wpa_config_set_config_clean(struct wpa_config *config)
{
	if (config->manufacturer)
	{
		os_free(config->manufacturer);
		config->manufacturer = NULL;
	}

	if (config->model_name)
	{
		os_free(config->model_name);
		config->model_name = NULL;
	}

	if (config->model_number)
	{
		os_free(config->model_number);
		config->model_number = NULL;
	}

	if (config->serial_number)
	{
		os_free(config->serial_number);
		config->serial_number = NULL;
	}

	if (config->device_name)
	{
		os_free(config->device_name);
		config->device_name = NULL;
	}

	if (config->config_methods)
	{
		os_free(config->config_methods);
		config->config_methods = NULL;
	}

	os_memset(config->device_type, 0, WPS_DEV_TYPE_LEN);
}


/**
 * wpa_config_set_network_defaults - Set network default values
 * @ssid: Pointer to network configuration data
 */
void wpa_config_set_network_defaults(struct wpa_ssid *ssid)
{
	da16x_cli_prt("[%s] Start\n", __func__);

	ssid->frequency = FREQUENCE_DEFAULT;
	ssid->proto = DEFAULT_PROTO;
	ssid->pairwise_cipher = DEFAULT_PAIRWISE;
	ssid->group_cipher = DEFAULT_GROUP;
	ssid->key_mgmt = DEFAULT_KEY_MGMT;
	ssid->dtim_period = DTIM_PERIOD_DEFAULT;
	ssid->beacon_int = BEACON_INT_DEFAULT;
#ifdef	CONFIG_BGSCAN
	ssid->bg_scan_period = DEFAULT_BG_SCAN_PERIOD;
#endif	/* CONFIG_BGSCAN */
	ssid->ht = 1;
#ifdef IEEE8021X_EAPOL
	ssid->eapol_flags = DEFAULT_EAPOL_FLAGS;
	ssid->eap_workaround = DEFAULT_EAP_WORKAROUND;
	ssid->eap.fragment_size = DEFAULT_FRAGMENT_SIZE;
	ssid->eap.sim_num = DEFAULT_USER_SELECTED_SIM;
#endif /* IEEE8021X_EAPOL */
#ifdef CONFIG_MESH
	ssid->dot11MeshMaxRetries = DEFAULT_MESH_MAX_RETRIES;
	ssid->dot11MeshRetryTimeout = DEFAULT_MESH_RETRY_TIMEOUT;
	ssid->dot11MeshConfirmTimeout = DEFAULT_MESH_CONFIRM_TIMEOUT;
	ssid->dot11MeshHoldingTimeout = DEFAULT_MESH_HOLDING_TIMEOUT;
	ssid->mesh_rssi_threshold = DEFAULT_MESH_RSSI_THRESHOLD;
#endif /* CONFIG_MESH */
#ifdef CONFIG_HT_OVERRIDES
	ssid->disable_ht = DEFAULT_DISABLE_HT;
#ifdef CONFIG_AP_WIFI_MODE
	ssid->require_ht = DEFAULT_REQUIRE_HT;
	ssid->wifi_mode = DEFAULT_WIFI_MODE;
#ifdef	CONFIG_DPM_OPT_WIFI_MODE /* by Shingu 20180207 (DPM Optimize) */
	ssid->dpm_opt_wifi_mode = 0;
#endif	/* CONFIG_DPM_OPT_WIFI_MODE */
#endif /* CONFIG_AP_WIFI_MODE */
	ssid->disable_ht40 = DEFAULT_DISABLE_HT40;
	ssid->disable_sgi = DEFAULT_DISABLE_SGI;
	ssid->disable_ldpc = DEFAULT_DISABLE_LDPC;
	ssid->disable_max_amsdu = DEFAULT_DISABLE_MAX_AMSDU;
	ssid->ampdu_factor = DEFAULT_AMPDU_FACTOR;
	ssid->ampdu_density = DEFAULT_AMPDU_DENSITY;
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_VHT_OVERRIDES
	ssid->vht_rx_mcs_nss_1 = -1;
	ssid->vht_rx_mcs_nss_2 = -1;
	ssid->vht_rx_mcs_nss_3 = -1;
	ssid->vht_rx_mcs_nss_4 = -1;
	ssid->vht_rx_mcs_nss_5 = -1;
	ssid->vht_rx_mcs_nss_6 = -1;
	ssid->vht_rx_mcs_nss_7 = -1;
	ssid->vht_rx_mcs_nss_8 = -1;
	ssid->vht_tx_mcs_nss_1 = -1;
	ssid->vht_tx_mcs_nss_2 = -1;
	ssid->vht_tx_mcs_nss_3 = -1;
	ssid->vht_tx_mcs_nss_4 = -1;
	ssid->vht_tx_mcs_nss_5 = -1;
	ssid->vht_tx_mcs_nss_6 = -1;
	ssid->vht_tx_mcs_nss_7 = -1;
	ssid->vht_tx_mcs_nss_8 = -1;
#endif /* CONFIG_VHT_OVERRIDES */
	ssid->proactive_key_caching = 0; // -1
#ifdef CONFIG_IEEE80211W
	ssid->ieee80211w = (enum mfp_options)MGMT_FRAME_PROTECTION_DEFAULT;
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_AP_ISOLATION
	ssid->isolate = ISOLATE_DEFAULT;
#endif /* CONFIG_AP_ISOLATION */

#ifdef CONFIG_AUTH_MAX_FAILURES
    ssid->auth_max_failures = DEFAULT_AUTH_MAX_FAILURES;
#endif // CONFIG_AUTH_MAX_FAILURES

#if 0 //def CONFIG_AP_GREEN_FIELD
	ssid->greenfield = DEFAULT_GREENFIELD;
#endif /* CONFIG_AP_GREEN_FIELD */

#ifdef CONFIG_MACSEC
	ssid->mka_priority = DEFAULT_PRIO_NOT_KEY_SERVER;
#endif /* CONFIG_MACSEC */
#ifdef CONFIG_RANDOM_ADDR
	ssid->mac_addr = -1;
#endif /* CONFIG_RANDOM_ADDR */

	da16x_cli_prt("[%s] End\n", __func__);
}


/**
 * wpa_config_set - Set a variable in network configuration
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * @value: Variable value
 * @line: Line number in configuration file or 0 if not used
 * Returns: 0 on success with possible change in the value, 1 on success with
 * no change to previously configured value, or -1 on failure
 *
 * This function can be used to set network configuration variables based on
 * both the configuration file and management interface input. The value
 * parameter must be in the same format as the text-based configuration file is
 * using. For example, strings are using double quotation marks.
 */
#if 1 // FC9000
int wpa_config_set(struct wpa_ssid *ssid, const char *var, const char *value)
{
	size_t i;
	int ret = 0;

	if (ssid == NULL || var == NULL || value == NULL)
		return -1;

#ifdef CONFIG_SAE
	/* WiFi SAE Certification item 4.1 */
	if (ssid->key_mgmt & WPA_KEY_MGMT_SAE) { /* SAE */
		if (os_strcmp(var, "pairwise") == 0) {
			if (os_strstr(value, "TKIP") != 0) { /* TKIP */
				return -1;
			}
		} else if (os_strcmp(var, "proto") == 0) {
			if (os_strstr(value, "WPA") != 0) { /* WPA1 */
				return -1;
			}
		}

		if (ssid->key_mgmt & WPA_KEY_MGMT_PSK) { /* SAE & WPA2-PSK */
			if (os_strcmp(var, "sae_password") == 0) {
				if (os_strlen(value) > (63+2)) {
					return -1;
				}
			}
		}
	}
#endif /* CONFIG_SAE */

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];

#ifdef AUTO_WEPKEY_INDEX
		if (os_strncmp("wep_key", var, 7) == 0 && os_strncmp("wep_key", field->name, 7) == 0) {
			;
		} else
#endif /* AUTO_WEPKEY_INDEX */
		if (os_strcmp(var, field->name) != 0) {
			continue;
		}

		/* SSID  set_network 0 ssid */
		if ((os_strcmp(network_fields[0].name /* "ssid" */, var) == 0) && (strlen(value) <= 2)) {
			ret = -1;
			break;
		}

		if (field->parser(field, ssid, value) < 0) {
			da16x_err_prt("[%s] Failed to parse %s '%s'\n", __func__, var, value);
			ret = -1;
		}

		network_fields[i].set = 1;

#ifdef AUTO_WEPKEY_INDEX
		if (os_strncmp("wep_key", var, 7) == 0) {
			for (int idx = 0; idx < 3; idx++) {
				field = &ssid_fields[++i];	
				
				if (field->parser(field, ssid, value)) {
					wpa_printf(MSG_ERROR, "[%s] Failed to parse %s '%s'.", __func__, var, value);
					ret = -1;
				}
				network_fields[i+1+idx].set = 1; /* wep_key0 */
			}
		}
#endif /* AUTO_WEPKEY_INDEX */		
		break;
	}

	if (i == NUM_SSID_FIELDS) {
		da16x_err_prt("[%s] Unknown network field '%s'\n", __func__, var);
		ret = -1;
	}

	return ret;
}
#else
int wpa_config_set(struct wpa_ssid *ssid, const char *var, const char *value)
{
	size_t i;
	int ret = 0;

	if (ssid == NULL || var == NULL || value == NULL)
		return -1;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];
		if (os_strcmp(var, field->name) != 0)
			continue;

		ret = field->parser(field, ssid, line, value);
		if (ret < 0) {
			wpa_printf(MSG_ERROR, "Failed to "
				   "parse %s '%s'.", var, value);
			ret = -1;
		}
		break;
	}
	if (i == NUM_SSID_FIELDS) {
		wpa_printf(MSG_ERROR, "Unknown network field "
			   "'%s'.", var);
		ret = -1;
	}

	return ret;
}
#endif

int wpa_config_set_quoted(struct wpa_ssid *ssid, const char *var,
			  const char *value)
{
	size_t len;
	char *buf;
	int ret;

	len = os_strlen(value);
	buf = os_malloc(len + 3);
	if (buf == NULL)
		return -1;
	buf[0] = '\"';
	os_memcpy(buf + 1, value, len);
	buf[len + 1] = '\"';
	buf[len + 2] = '\0';
	ret = wpa_config_set(ssid, var, buf);
	os_free(buf);
	return ret;
}


/**
 * wpa_config_get_all - Get all options from network configuration
 * @ssid: Pointer to network configuration data
 * @get_keys: Determines if keys/passwords will be included in returned list
 *	(if they may be exported)
 * Returns: %NULL terminated list of all set keys and their values in the form
 * of [key1, val1, key2, val2, ... , NULL]
 *
 * This function can be used to get list of all configured network properties.
 * The caller is responsible for freeing the returned list and all its
 * elements.
 */
char ** wpa_config_get_all(struct wpa_ssid *ssid, int get_keys)
{
#ifdef NO_CONFIG_WRITE
	return NULL;
#else /* NO_CONFIG_WRITE */
	const struct parse_data *field;
	char *key, *value;
	size_t i;
	char **props;
	int fields_num;

	get_keys = get_keys && ssid->export_keys;

	props = os_calloc(2 * NUM_SSID_FIELDS + 1, sizeof(char *));

	if (!props)
		return NULL;

	fields_num = 0;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {

		field = &ssid_fields[i];

		if (field->key_data && !get_keys)
			continue;

		value = field->writer(field, ssid);

		if (value == NULL)
			continue;

		if (os_strlen(value) == 0) {
			os_free(value);
			continue;
		}

		key = os_strdup(field->name);

		if (key == NULL) {
			os_free(value);
			goto err;
		}

		props[fields_num * 2] = key;
		props[fields_num * 2 + 1] = value;

		fields_num++;
	}

	return props;

err:
	for (i = 0; props[i]; i++)
		os_free(props[i]);
	os_free(props);
	return NULL;
#endif /* NO_CONFIG_WRITE */
}


#ifndef NO_CONFIG_WRITE
/**
 * wpa_config_get - Get a variable in network configuration
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * Returns: Value of the variable or %NULL on failure
 *
 * This function can be used to get network configuration variables. The
 * returned value is a copy of the configuration variable in text format, i.e,.
 * the same format that the text-based configuration file and wpa_config_set()
 * are using for the value. The caller is responsible for freeing the returned
 * value.
 */
char * wpa_config_get(struct wpa_ssid *ssid, const char *var)
{
	size_t i;

	if (ssid == NULL || var == NULL)
		return NULL;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];
		if (os_strcmp(var, field->name) == 0) {
			char *ret = field->writer(field, ssid);

			if (ret && has_newline(ret)) {
				wpa_printf(MSG_ERROR,
					   "Found newline in value for %s; not returning it",
					   var);
				os_free(ret);
				ret = NULL;
			}

			return ret;
		}
	}

	return NULL;
}


/**
 * wpa_config_get_no_key - Get a variable in network configuration (no keys)
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * Returns: Value of the variable or %NULL on failure
 *
 * This function can be used to get network configuration variable like
 * wpa_config_get(). The only difference is that this functions does not expose
 * key/password material from the configuration. In case a key/password field
 * is requested, the returned value is an empty string or %NULL if the variable
 * is not set or "*" if the variable is set (regardless of its value). The
 * returned value is a copy of the configuration variable in text format, i.e,.
 * the same format that the text-based configuration file and wpa_config_set()
 * are using for the value. The caller is responsible for freeing the returned
 * value.
 */
char * wpa_config_get_no_key(struct wpa_ssid *ssid, const char *var)
{
	size_t i;

	if (ssid == NULL || var == NULL)
		return NULL;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];

		if (os_strcmp(var, field->name) == 0) {
			char *res = field->writer(field, ssid);
#if 0 /* Hidden Key */
			if (field->key_data) {
				if (res && res[0]) {
					wpa_printf_dbg(MSG_DEBUG, "Do not allow "
						   "key_data field to be "
						   "exposed");
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
					str_clear_free(res);
#else
					os_free(res);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
					return os_strdup("*"); /* <== free È®ï¿½ï¿½ ï¿½Ê¿ï¿½ */
				}

				os_free(res);
				return NULL;
			}
#endif /* 0 */
			return res;
		}
	}

	return NULL;
}
#endif /* NO_CONFIG_WRITE */

extern unsigned char fast_connection_sleep_flag;

/**
 * wpa_config_update_psk - Update WPA PSK based on passphrase and SSID
 * @ssid: Pointer to network configuration data
 *
 * This function must be called to update WPA PSK when either SSID or the
 * passphrase has changed for the network configuration.
 */
void wpa_config_update_psk(struct wpa_ssid *ssid)
{
#ifdef PSK_RAW_KEY_TIME_PRINT 
	struct os_time now;
	int time;
	//da16x_notice_prt("[%s] start\n", __func__);
	//da16x_notice_prt("ssid->passphrase=%s\n", ssid->passphrase);
	//da16x_notice_prt("ssid->sae_password=%s\n", ssid->sae_password);
	//fc9k_notice_prt("ssid->psk=%s\n",  wpa_config_write_string_hex(ssid->psk, PMK_LEN));

	if (fast_connection_sleep_flag == pdTRUE) {
		time = xTaskGetTickCount() * portTICK_PERIOD_MS;
	}
#endif /* PSK_RAW_KEY_TIME_PRINT */
#ifndef CONFIG_NO_PBKDF2
	pbkdf2_sha1(ssid->passphrase, ssid->ssid, ssid->ssid_len, 4096,
		    ssid->psk, PMK_LEN);
	wpa_hexdump_key(MSG_MSGDUMP, "PSK (from passphrase)",
			ssid->psk, PMK_LEN);
	ssid->psk_set = 1;
#endif /* CONFIG_NO_PBKDF2 */

#ifdef PSK_RAW_KEY_TIME_PRINT 
	if (fast_connection_sleep_flag == pdTRUE) {
		time = (xTaskGetTickCount() * portTICK_PERIOD_MS) - time;
		fc9k_notice_prt("[%s] pbkdf2_sha1: %d msec(%d)\n", __func__, time * 10);
	}

	//da16x_notice_prt("[%s] end\n", __func__);
	//da16x_notice_prt("ssid->passphrase=%s\n", ssid->passphrase);
	//da16x_notice_prt("ssid->sae_password=%s\n", ssid->sae_password);
	//fc9k_notice_prt("ssid->psk=%s\n", wpa_config_write_string_hex(ssid->psk, PMK_LEN));
#endif /* PSK_RAW_KEY_TIME_PRINT */

}

#ifdef CONFIG_INTERWORKING
static int wpa_config_set_cred_req_conn_capab(struct wpa_cred *cred,
					      const char *value)
{
	u8 *proto;
	int **port;
	int *ports, *nports;
	const char *pos;
	unsigned int num_ports;

	proto = os_realloc_array(cred->req_conn_capab_proto,
				 cred->num_req_conn_capab + 1, sizeof(u8));
	if (proto == NULL)
		return -1;
	cred->req_conn_capab_proto = proto;

	port = os_realloc_array(cred->req_conn_capab_port,
				cred->num_req_conn_capab + 1, sizeof(int *));
	if (port == NULL)
		return -1;
	cred->req_conn_capab_port = port;

	proto[cred->num_req_conn_capab] = atoi(value);

	pos = os_strchr(value, ':');
	if (pos == NULL) {
		port[cred->num_req_conn_capab] = NULL;
		cred->num_req_conn_capab++;
		return 0;
	}
	pos++;

	ports = NULL;
	num_ports = 0;

	while (*pos) {
		nports = os_realloc_array(ports, num_ports + 1, sizeof(int));
		if (nports == NULL) {
			os_free(ports);
			return -1;
		}
		ports = nports;
		ports[num_ports++] = atoi(pos);

		pos = os_strchr(pos, ',');
		if (pos == NULL)
			break;
		pos++;
	}

	nports = os_realloc_array(ports, num_ports + 1, sizeof(int));
	if (nports == NULL) {
		os_free(ports);
		return -1;
	}
	ports = nports;
	ports[num_ports] = -1;

	port[cred->num_req_conn_capab] = ports;
	cred->num_req_conn_capab++;
	return 0;
}


static int wpa_config_set_cred_roaming_consortiums(struct wpa_cred *cred,
						   const char *value)
{
	u8 roaming_consortiums[MAX_ROAMING_CONS][MAX_ROAMING_CONS_OI_LEN];
	size_t roaming_consortiums_len[MAX_ROAMING_CONS];
	unsigned int num_roaming_consortiums = 0;
	const char *pos, *end;
	size_t len;

	os_memset(roaming_consortiums, 0, sizeof(roaming_consortiums));
	os_memset(roaming_consortiums_len, 0, sizeof(roaming_consortiums_len));

	for (pos = value;;) {
		end = os_strchr(pos, ',');
		len = end ? (size_t) (end - pos) : os_strlen(pos);
		if (!end && len == 0)
			break;
		if (len == 0 || (len & 1) != 0 ||
		    len / 2 > MAX_ROAMING_CONS_OI_LEN ||
		    hexstr2bin(pos,
			       roaming_consortiums[num_roaming_consortiums],
			       len / 2) < 0) {
			wpa_printf(MSG_INFO,
				   "Invalid roaming_consortiums entry: %s",
				   pos);
			return -1;
		}
		roaming_consortiums_len[num_roaming_consortiums] = len / 2;
		num_roaming_consortiums++;
		if (num_roaming_consortiums > MAX_ROAMING_CONS) {
			wpa_printf(MSG_INFO,
				   "Too many roaming_consortiums OIs");
			return -1;
		}

		if (!end)
			break;
		pos = end + 1;
	}

	os_memcpy(cred->roaming_consortiums, roaming_consortiums,
		  sizeof(roaming_consortiums));
	os_memcpy(cred->roaming_consortiums_len, roaming_consortiums_len,
		  sizeof(roaming_consortiums_len));
	cred->num_roaming_consortiums = num_roaming_consortiums;

	return 0;
}


int wpa_config_set_cred(struct wpa_cred *cred, const char *var,
			const char *value)
{
	char *val;
	size_t len;
	int res;

	if (os_strcmp(var, "temporary") == 0) {
		cred->temporary = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "priority") == 0) {
		cred->priority = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "sp_priority") == 0) {
		int prio = atoi(value);
		if (prio < 0 || prio > 255)
			return -1;
		cred->sp_priority = prio;
		return 0;
	}

	if (os_strcmp(var, "pcsc") == 0) {
		cred->pcsc = atoi(value);
		return 0;
	}

#ifdef CONFIG_EAP_METHOD
	if (os_strcmp(var, "eap") == 0) {
		struct eap_method_type method;
		method.method = eap_peer_get_type(value, &method.vendor);
		if (method.vendor == EAP_VENDOR_IETF &&
		    method.method == EAP_TYPE_NONE) {
			wpa_printf(MSG_ERROR, "Unknown EAP type '%s' "
				   "for a credential", value);
			return -1;
		}
		os_free(cred->eap_method);
		cred->eap_method = os_malloc(sizeof(*cred->eap_method));
		if (cred->eap_method == NULL)
			return -1;
		os_memcpy(cred->eap_method, &method, sizeof(method));
		return 0;
	}
#endif	/* CONFIG_EAP_METHOD */

#ifdef	CONFIG_INTERWORKING
	if (os_strcmp(var, "password") == 0 &&
	    os_strncmp(value, "ext:", 4) == 0) {
		if (has_newline(value))
			return -1;
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(cred->password);
#else
		os_free(cred->password);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		cred->password = os_strdup(value);
		cred->ext_password = 1;
		return 0;
	}
#endif	/* CONFIG_INTERWORKING */

	if (os_strcmp(var, "update_identifier") == 0) {
		cred->update_identifier = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "min_dl_bandwidth_home") == 0) {
		cred->min_dl_bandwidth_home = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "min_ul_bandwidth_home") == 0) {
		cred->min_ul_bandwidth_home = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "min_dl_bandwidth_roaming") == 0) {
		cred->min_dl_bandwidth_roaming = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "min_ul_bandwidth_roaming") == 0) {
		cred->min_ul_bandwidth_roaming = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "max_bss_load") == 0) {
		cred->max_bss_load = atoi(value);
		return 0;
	}

	if (os_strcmp(var, "req_conn_capab") == 0)
		return wpa_config_set_cred_req_conn_capab(cred, value);

	if (os_strcmp(var, "ocsp") == 0) {
		cred->ocsp = atoi(value);
		return 0;
	}

#ifdef	UNUSED_FUNC_IN_SUPPLICANT
	if (os_strcmp(var, "sim_num") == 0) {
		cred->sim_num = atoi(value);
		return 0;
	}
#endif	/* UNUSED_FUNC_IN_SUPPLICANT */

	val = wpa_config_parse_string(value, &len);
	if (val == NULL ||
	    (os_strcmp(var, "excluded_ssid") != 0 &&
	     os_strcmp(var, "roaming_consortium") != 0 &&
	     os_strcmp(var, "required_roaming_consortium") != 0 &&
	     has_newline(val))) {
		wpa_printf(MSG_ERROR, "Invalid field '%s' string "
			   "value '%s'.", var, value);
		os_free(val);
		return -1;
	}

#ifdef	CONFIG_INTERWORKING
	if (os_strcmp(var, "realm") == 0) {
		os_free(cred->realm);
		cred->realm = val;
		return 0;
	}

	if (os_strcmp(var, "username") == 0) {
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(cred->username);
#else
		os_free(cred->username);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		cred->username = val;
		return 0;
	}

	if (os_strcmp(var, "password") == 0) {
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(cred->password);
#else
		os_free(cred->password);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		cred->password = val;
		cred->ext_password = 0;
		return 0;
	}
#endif	/* CONFIG_INTERWORKING */

	if (os_strcmp(var, "ca_cert") == 0) {
		os_free(cred->ca_cert);
		cred->ca_cert = val;
		return 0;
	}

	if (os_strcmp(var, "client_cert") == 0) {
		os_free(cred->client_cert);
		cred->client_cert = val;
		return 0;
	}

	if (os_strcmp(var, "private_key") == 0) {
		os_free(cred->private_key);
		cred->private_key = val;
		return 0;
	}

	if (os_strcmp(var, "private_key_passwd") == 0) {
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(cred->private_key_passwd);
#else
		os_free(cred->private_key_passwd);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		cred->private_key_passwd = val;
		return 0;
	}

	if (os_strcmp(var, "imsi") == 0) {
		os_free(cred->imsi);
		cred->imsi = val;
		return 0;
	}

	if (os_strcmp(var, "milenage") == 0) {
#ifdef	CONFIG_SUPP27_STR_CLR_FREE
		str_clear_free(cred->milenage);
#else
		os_free(cred->milenage);
#endif	/* CONFIG_SUPP27_STR_CLR_FREE */
		cred->milenage = val;
		return 0;
	}

	if (os_strcmp(var, "domain_suffix_match") == 0) {
		os_free(cred->domain_suffix_match);
		cred->domain_suffix_match = val;
		return 0;
	}

	if (os_strcmp(var, "domain") == 0) {
		char **new_domain;
		new_domain = os_realloc_array(cred->domain,
					      cred->num_domain + 1,
					      sizeof(char *));
		if (new_domain == NULL) {
			os_free(val);
			return -1;
		}
		new_domain[cred->num_domain++] = val;
		cred->domain = new_domain;
		return 0;
	}

	if (os_strcmp(var, "phase1") == 0) {
		os_free(cred->phase1);
		cred->phase1 = val;
		return 0;
	}

	if (os_strcmp(var, "phase2") == 0) {
		os_free(cred->phase2);
		cred->phase2 = val;
		return 0;
	}

#ifdef	CONFIG_ROAM_CONSORTIUM
	if (os_strcmp(var, "roaming_consortium") == 0) {
		if (len < 3 || len > sizeof(cred->roaming_consortium)) {
			wpa_printf(MSG_ERROR, "Invalid "
				   "roaming_consortium length %d (3..15 "
				   "expected)", (int) len);
			os_free(val);
			return -1;
		}
		os_memcpy(cred->roaming_consortium, val, len);
		cred->roaming_consortium_len = len;
		os_free(val);
		return 0;
	}

	if (os_strcmp(var, "required_roaming_consortium") == 0) {
		if (len < 3 || len > sizeof(cred->required_roaming_consortium))
		{
			wpa_printf(MSG_ERROR, "Invalid "
				   "required_roaming_consortium length %d "
				   "(3..15 expected)", (int) len);
			os_free(val);
			return -1;
		}
		os_memcpy(cred->required_roaming_consortium, val, len);
		cred->required_roaming_consortium_len = len;
		os_free(val);
		return 0;
	}
#endif	/* CONFIG_ROAM_CONSORTIUM */

	if (os_strcmp(var, "roaming_consortiums") == 0) {
		res = wpa_config_set_cred_roaming_consortiums(cred, val);
		if (res < 0)
			wpa_printf(MSG_ERROR,
				   "Invalid roaming_consortiums");
		os_free(val);
		return res;
	}

#ifdef	CONFIG_INTERWORKING
	if (os_strcmp(var, "excluded_ssid") == 0) {
		struct excluded_ssid *e;

		if (len > SSID_MAX_LEN) {
			wpa_printf(MSG_ERROR, "Invalid "
				   "excluded_ssid length %d", (int) len);
			os_free(val);
			return -1;
		}

		e = os_realloc_array(cred->excluded_ssid,
				     cred->num_excluded_ssid + 1,
				     sizeof(struct excluded_ssid));
		if (e == NULL) {
			os_free(val);
			return -1;
		}
		cred->excluded_ssid = e;

		e = &cred->excluded_ssid[cred->num_excluded_ssid++];
		os_memcpy(e->ssid, val, len);
		e->ssid_len = len;

		os_free(val);

		return 0;
	}
#endif	/* CONFIG_INTERWORKING */

#ifdef	CONFIG_ROAMING_PARTNER
	if (os_strcmp(var, "roaming_partner") == 0) {
		struct roaming_partner *p;
		char *pos;

		p = os_realloc_array(cred->roaming_partner,
				     cred->num_roaming_partner + 1,
				     sizeof(struct roaming_partner));
		if (p == NULL) {
			os_free(val);
			return -1;
		}
		cred->roaming_partner = p;

		p = &cred->roaming_partner[cred->num_roaming_partner];

		pos = os_strchr(val, ',');
		if (pos == NULL) {
			os_free(val);
			return -1;
		}
		*pos++ = '\0';
		if (pos - val - 1 >= (int) sizeof(p->fqdn)) {
			os_free(val);
			return -1;
		}
		os_memcpy(p->fqdn, val, pos - val);

		p->exact_match = atoi(pos);

		pos = os_strchr(pos, ',');
		if (pos == NULL) {
			os_free(val);
			return -1;
		}
		*pos++ = '\0';

		p->priority = atoi(pos);

		pos = os_strchr(pos, ',');
		if (pos == NULL) {
			os_free(val);
			return -1;
		}
		*pos++ = '\0';

		if (os_strlen(pos) >= sizeof(p->country)) {
			os_free(val);
			return -1;
		}
		os_memcpy(p->country, pos, os_strlen(pos) + 1);

		cred->num_roaming_partner++;
		os_free(val);

		return 0;
	}
#endif	/* CONFIG_ROAMING_PARTNER */

	if (os_strcmp(var, "provisioning_sp") == 0) {
		os_free(cred->provisioning_sp);
		cred->provisioning_sp = val;
		return 0;
	}

#if 0
	if (line) {
		wpa_printf(MSG_ERROR, "Line %d: unknown cred field '%s'.",
			   line, var);
	}
#endif /* 0 */

	os_free(val);

	return -1;
}
#endif /* CONFIG_INTERWORKING */


char * alloc_int_str(int val)
{
	const unsigned int bufsize = 20;
	char *buf;
	int res;

	buf = os_malloc(bufsize);
	if (buf == NULL)
		return NULL;
	res = os_snprintf(buf, bufsize, "%d", val);
	if (os_snprintf_error(bufsize, res)) {
		os_free(buf);
		buf = NULL;
	}
	return buf;
}


char * alloc_strdup(const char *str)
{
	if (str == NULL)
		return NULL;
	return os_strdup(str);
}

#ifdef CONFIG_INTERWORKING
char * wpa_config_get_cred_no_key(struct wpa_cred *cred, const char *var)
{
	if (os_strcmp(var, "temporary") == 0)
		return alloc_int_str(cred->temporary);

	if (os_strcmp(var, "priority") == 0)
		return alloc_int_str(cred->priority);

	if (os_strcmp(var, "sp_priority") == 0)
		return alloc_int_str(cred->sp_priority);

	if (os_strcmp(var, "pcsc") == 0)
		return alloc_int_str(cred->pcsc);

#ifdef	CONFIG_EAP_METHOD
	if (os_strcmp(var, "eap") == 0) {
		if (!cred->eap_method)
			return NULL;
		return alloc_strdup(eap_get_name(cred->eap_method[0].vendor,
						 cred->eap_method[0].method));
	}
#endif	/* CONFIG_EAP_METHOD */

	if (os_strcmp(var, "update_identifier") == 0)
		return alloc_int_str(cred->update_identifier);

	if (os_strcmp(var, "min_dl_bandwidth_home") == 0)
		return alloc_int_str(cred->min_dl_bandwidth_home);

	if (os_strcmp(var, "min_ul_bandwidth_home") == 0)
		return alloc_int_str(cred->min_ul_bandwidth_home);

	if (os_strcmp(var, "min_dl_bandwidth_roaming") == 0)
		return alloc_int_str(cred->min_dl_bandwidth_roaming);

	if (os_strcmp(var, "min_ul_bandwidth_roaming") == 0)
		return alloc_int_str(cred->min_ul_bandwidth_roaming);

	if (os_strcmp(var, "max_bss_load") == 0)
		return alloc_int_str(cred->max_bss_load);

	if (os_strcmp(var, "req_conn_capab") == 0) {
		unsigned int i;
		char *buf, *end, *pos;
		int ret;

		if (!cred->num_req_conn_capab)
			return NULL;

		buf = os_malloc(4000);
		if (buf == NULL)
			return NULL;
		pos = buf;
		end = pos + 4000;
		for (i = 0; i < cred->num_req_conn_capab; i++) {
			int *ports;

			ret = os_snprintf(pos, end - pos, "%s%u",
					  i > 0 ? "\n" : "",
					  cred->req_conn_capab_proto[i]);
			if (os_snprintf_error(end - pos, ret))
				return buf;
			pos += ret;

			ports = cred->req_conn_capab_port[i];
			if (ports) {
				int j;
				for (j = 0; ports[j] != -1; j++) {
					ret = os_snprintf(pos, end - pos,
							  "%s%d",
							  j > 0 ? "," : ":",
							  ports[j]);
					if (os_snprintf_error(end - pos, ret))
						return buf;
					pos += ret;
				}
			}
		}

		return buf;
	}

	if (os_strcmp(var, "ocsp") == 0)
		return alloc_int_str(cred->ocsp);

#ifdef	CONFIG_INTERWORKING
	if (os_strcmp(var, "realm") == 0)
		return alloc_strdup(cred->realm);

	if (os_strcmp(var, "username") == 0)
		return alloc_strdup(cred->username);

	if (os_strcmp(var, "password") == 0) {
		if (!cred->password)
			return NULL;
		return alloc_strdup("*");
	}
#endif	/* CONFIG_INTERWORKING */

	if (os_strcmp(var, "ca_cert") == 0)
		return alloc_strdup(cred->ca_cert);

	if (os_strcmp(var, "client_cert") == 0)
		return alloc_strdup(cred->client_cert);

	if (os_strcmp(var, "private_key") == 0)
		return alloc_strdup(cred->private_key);

	if (os_strcmp(var, "private_key_passwd") == 0) {
		if (!cred->private_key_passwd)
			return NULL;
		return alloc_strdup("*");
	}

	if (os_strcmp(var, "imsi") == 0)
		return alloc_strdup(cred->imsi);

	if (os_strcmp(var, "milenage") == 0) {
		if (!(cred->milenage))
			return NULL;
		return alloc_strdup("*");
	}

	if (os_strcmp(var, "domain_suffix_match") == 0)
		return alloc_strdup(cred->domain_suffix_match);

	if (os_strcmp(var, "domain") == 0) {
		unsigned int i;
		char *buf, *end, *pos;
		int ret;

		if (!cred->num_domain)
			return NULL;

		buf = os_malloc(4000);
		if (buf == NULL)
			return NULL;
		pos = buf;
		end = pos + 4000;

		for (i = 0; i < cred->num_domain; i++) {
			ret = os_snprintf(pos, end - pos, "%s%s",
					  i > 0 ? "\n" : "", cred->domain[i]);
			if (os_snprintf_error(end - pos, ret))
				return buf;
			pos += ret;
		}

		return buf;
	}

	if (os_strcmp(var, "phase1") == 0)
		return alloc_strdup(cred->phase1);

	if (os_strcmp(var, "phase2") == 0)
		return alloc_strdup(cred->phase2);

#ifdef	CONFIG_ROAM_CONSORTIUM
	if (os_strcmp(var, "roaming_consortium") == 0) {
		size_t buflen;
		char *buf;

		if (!cred->roaming_consortium_len)
			return NULL;
		buflen = cred->roaming_consortium_len * 2 + 1;
		buf = os_malloc(buflen);
		if (buf == NULL)
			return NULL;
		wpa_snprintf_hex(buf, buflen, cred->roaming_consortium,
				 cred->roaming_consortium_len);
		return buf;
	}

	if (os_strcmp(var, "required_roaming_consortium") == 0) {
		size_t buflen;
		char *buf;

		if (!cred->required_roaming_consortium_len)
			return NULL;
		buflen = cred->required_roaming_consortium_len * 2 + 1;
		buf = os_malloc(buflen);
		if (buf == NULL)
			return NULL;
		wpa_snprintf_hex(buf, buflen, cred->required_roaming_consortium,
				 cred->required_roaming_consortium_len);
		return buf;
	}
#endif	/* CONFIG_ROAM_CONSORTIUM */

	if (os_strcmp(var, "roaming_consortiums") == 0) {
		size_t buflen;
		char *buf, *pos;
		size_t i;

		if (!cred->num_roaming_consortiums)
			return NULL;
		buflen = cred->num_roaming_consortiums *
			MAX_ROAMING_CONS_OI_LEN * 2 + 1;
		buf = os_malloc(buflen);
		if (!buf)
			return NULL;
		pos = buf;
		for (i = 0; i < cred->num_roaming_consortiums; i++) {
			if (i > 0)
				*pos++ = ',';
			pos += wpa_snprintf_hex(
				pos, buf + buflen - pos,
				cred->roaming_consortiums[i],
				cred->roaming_consortiums_len[i]);
		}
		*pos = '\0';
		return buf;
	}

#ifdef	CONFIG_INTERWORKING
	if (os_strcmp(var, "excluded_ssid") == 0) {
		unsigned int i;
		char *buf, *end, *pos;

		if (!cred->num_excluded_ssid)
			return NULL;

		buf = os_malloc(4000);
		if (buf == NULL)
			return NULL;
		pos = buf;
		end = pos + 4000;

		for (i = 0; i < cred->num_excluded_ssid; i++) {
			struct excluded_ssid *e;
			int ret;

			e = &cred->excluded_ssid[i];
			ret = os_snprintf(pos, end - pos, "%s%s",
					  i > 0 ? "\n" : "",
					  wpa_ssid_txt(e->ssid, e->ssid_len));
			if (os_snprintf_error(end - pos, ret))
				return buf;
			pos += ret;
		}

		return buf;
	}
#endif	/* CONFIG_INTERWORKING */

#ifdef	CONFIG_ROAMING_PARTNER
	if (os_strcmp(var, "roaming_partner") == 0) {
		unsigned int i;
		char *buf, *end, *pos;

		if (!cred->num_roaming_partner)
			return NULL;

		buf = os_malloc(4000);
		if (buf == NULL)
			return NULL;
		pos = buf;
		end = pos + 4000;

		for (i = 0; i < cred->num_roaming_partner; i++) {
			struct roaming_partner *p;
			int ret;

			p = &cred->roaming_partner[i];
			ret = os_snprintf(pos, end - pos, "%s%s,%d,%u,%s",
					  i > 0 ? "\n" : "",
					  p->fqdn, p->exact_match, p->priority,
					  p->country);
			if (os_snprintf_error(end - pos, ret))
				return buf;
			pos += ret;
		}

		return buf;
	}
#endif	/* CONFIG_ROAMING_PARTNER */

	if (os_strcmp(var, "provisioning_sp") == 0)
		return alloc_strdup(cred->provisioning_sp);

	return NULL;
}


#ifdef CONFIG_WPA_CRED
struct wpa_cred * wpa_config_get_cred(struct wpa_config *config, int id)
{
	struct wpa_cred *cred;

	cred = config->cred;
	while (cred) {
		if (id == cred->id)
			break;
		cred = cred->next;
	}

	return cred;
}


struct wpa_cred * wpa_config_add_cred(struct wpa_config *config)
{
	int id;
	struct wpa_cred *cred, *last = NULL;

	id = -1;
	cred = config->cred;
	while (cred) {
		if (cred->id > id)
			id = cred->id;
		last = cred;
		cred = cred->next;
	}
	id++;

	cred = os_zalloc(sizeof(*cred));
	if (cred == NULL)
		return NULL;
	cred->id = id;
	cred->sim_num = DEFAULT_USER_SELECTED_SIM;
	if (last)
		last->next = cred;
	else
		config->cred = cred;

	return cred;
}


int wpa_config_remove_cred(struct wpa_config *config, int id)
{
	struct wpa_cred *cred, *prev = NULL;

	cred = config->cred;
	while (cred) {
		if (id == cred->id)
			break;
		prev = cred;
		cred = cred->next;
	}

	if (cred == NULL)
		return -1;

	if (prev)
		prev->next = cred->next;
	else
		config->cred = cred->next;

	wpa_config_free_cred(cred);
	return 0;
}
#endif /* CONFIG_WPA_CRED */
#endif /* CONFIG_INTERWORKING */


#ifndef CONFIG_NO_CONFIG_BLOBS
/**
 * wpa_config_get_blob - Get a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @name: Name of the blob
 * Returns: Pointer to blob data or %NULL if not found
 */
const struct wpa_config_blob * wpa_config_get_blob(struct wpa_config *config,
						   const char *name)
{
	struct wpa_config_blob *blob = config->blobs;

	while (blob) {
		if (os_strcmp(blob->name, name) == 0)
			return blob;
		blob = blob->next;
	}
	return NULL;
}


/**
 * wpa_config_set_blob - Set or add a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @blob: New value for the blob
 *
 * Adds a new configuration blob or replaces the current value of an existing
 * blob.
 */
void wpa_config_set_blob(struct wpa_config *config,
			 struct wpa_config_blob *blob)
{
	wpa_config_remove_blob(config, blob->name);
	blob->next = config->blobs;
	config->blobs = blob;
}


/**
 * wpa_config_free_blob - Free blob data
 * @blob: Pointer to blob to be freed
 */
void wpa_config_free_blob(struct wpa_config_blob *blob)
{
	if (blob) {
		os_free(blob->name);
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(blob->data, blob->len);
#else
		os_free(blob->data);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		os_free(blob);
	}
}


/**
 * wpa_config_remove_blob - Remove a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @name: Name of the blob to remove
 * Returns: 0 if blob was removed or -1 if blob was not found
 */
int wpa_config_remove_blob(struct wpa_config *config, const char *name)
{
	struct wpa_config_blob *pos = config->blobs, *prev = NULL;

	while (pos) {
		if (os_strcmp(pos->name, name) == 0) {
			if (prev)
				prev->next = pos->next;
			else
				config->blobs = pos->next;
			wpa_config_free_blob(pos);
			return 0;
		}
		prev = pos;
		pos = pos->next;
	}

	return -1;
}
#endif /* CONFIG_NO_CONFIG_BLOBS */



#ifdef	CONFIG_CONCURRENT
extern int get_run_mode(void);
#endif	/* CONFIG_CONCURRENT */

/**
 * wpa_config_alloc_empty - Allocate an empty configuration
 * @ctrl_interface: Control interface parameters, e.g., path to UNIX domain
 * socket
 * @driver_param: Driver parameters
 * Returns: Pointer to allocated configuration data or %NULL on failure
 */
struct wpa_config * wpa_config_alloc_empty(const char *ctrl_interface,
					   const char *driver_param)
{
	struct wpa_config *config;
#ifdef	CONFIG_AP_WMM
	const int aCWmin = 4, aCWmax = 10;

	const struct hostapd_wmm_ac_params ac_bk =
		{ aCWmin, aCWmax, 7, 0, 0 }; /* background traffic */

	const struct hostapd_wmm_ac_params ac_be =
		{ aCWmin, aCWmax, 3, 0, 0 }; /* best effort traffic */

	const struct hostapd_wmm_ac_params ac_vi = /* video traffic */
		{ aCWmin - 1, aCWmin, 2, 3008 / 32, 0 };
//		{ aCWmin - 1, aCWmin, 2, 3000 / 32, 0 };

	const struct hostapd_wmm_ac_params ac_vo = /* voice traffic */
		{ aCWmin - 2, aCWmin - 1, 2, 1504 / 32, 0 };
//		{ aCWmin - 2, aCWmin - 1, 2, 1500 / 32, 0 };
#endif	/* CONFIG_AP_WMM */

	TX_FUNC_START("      ");

	config = os_zalloc(sizeof(*config));

	if (config == NULL) {
		da16x_mem_prt("      [%s] config alloc fail\n", __func__);
		return NULL;
	}

	config->eapol_version = DEFAULT_EAPOL_VERSION;
#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	config->ap_scan = DEFAULT_AP_SCAN;
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */
#ifdef CONFIG_MESH
	config->user_mpm = DEFAULT_USER_MPM;

	if (get_run_mode() == MESH_POINT_MODE)
		config->max_peer_links = DEFAULT_POINT_MAX_PEER_LINKS; 
	else if (get_run_mode() == STA_MESH_PORTAL_MODE)
		config->max_peer_links = DEFAULT_POTAL_MAX_PEER_LINKS; 
	else
		config->max_peer_links = DEFAULT_MAX_PEER_LINKS;

	config->mesh_max_inactivity = DEFAULT_MESH_MAX_INACTIVITY;
#endif	/* CONFIG_MESH */
#ifdef CONFIG_SAE
	config->dot11RSNASAERetransPeriod = DEFAULT_DOT11_RSNA_SAE_RETRANS_PERIOD;
#endif /* CONFIG_SAE */
#ifdef	CONFIG_FAST_REAUTH
	config->fast_reauth = DEFAULT_FAST_REAUTH;
#endif	/* CONFIG_FAST_REAUTH */
	config->bss_max_count = DEFAULT_BSS_MAX_COUNT;
#ifndef CONFIG_IMMEDIATE_SCAN
	config->bss_expiration_age = DEFAULT_BSS_EXPIRATION_AGE;
	config->bss_expiration_scan_count = DEFAULT_BSS_EXPIRATION_SCAN_COUNT;
#endif /* CONFIG_IMMEDIATE_SCAN */

#ifdef	CONFIG_CONCURRENT
	if (   get_run_mode() == STA_SOFT_AP_CONCURRENT_MODE
#if defined ( P2P_ONLY_MODE )
		|| get_run_mode() == STA_P2P_CONCURRENT_MODE
#endif	// P2P_ONLY_MODE
       ) {
		config->max_num_sta = DEFAULT_CONCURRENT_MAX_NUM_STA;
	} else {
		config->max_num_sta = DEFAULT_MAX_NUM_STA;
	}
#else
	config->max_num_sta = DEFAULT_MAX_NUM_STA;
#endif	/* CONFIG_CONCURRENT */

	config->ap_isolate = DEFAULT_AP_ISOLATE;

	config->access_network_type = DEFAULT_ACCESS_NETWORK_TYPE;
	config->scan_cur_freq = DEFAULT_SCAN_CUR_FREQ;

#ifdef	CONFIG_AP_WMM
	config->wmm_ac_params[0] = ac_be;
	config->wmm_ac_params[1] = ac_bk;
	config->wmm_ac_params[2] = ac_vi;
	config->wmm_ac_params[3] = ac_vo;
#endif	/* CONFIG_AP_WMM */

#ifdef CONFIG_P2P
	config->p2p_search_delay = DEFAULT_P2P_SEARCH_DELAY;
#endif	/* CONFIG_P2P */
#ifdef CONFIG_RAMDOM_ADDR
	config->rand_addr_lifetime = DEFAULT_RAND_ADDR_LIFETIME;
#endif /* CONFIG_RAMDOM_ADDR */
	config->key_mgmt_offload = DEFAULT_KEY_MGMT_OFFLOAD;
	config->cert_in_cb = DEFAULT_CERT_IN_CB;
	config->wpa_rsc_relaxation = DEFAULT_WPA_RSC_RELAXATION;

#ifdef CONFIG_MBO
	config->mbo_cell_capa = DEFAULT_MBO_CELL_CAPA;
	config->disassoc_imminent_rssi_threshold =
		DEFAULT_DISASSOC_IMMINENT_RSSI_THRESHOLD;
	config->oce = DEFAULT_OCE_SUPPORT;
#endif /* CONFIG_MBO */

	config->update_config = 1;

	if (ctrl_interface)
		config->ctrl_interface = os_strdup(ctrl_interface);

	if (driver_param)
		config->driver_param = os_strdup(driver_param);
#ifdef CONFIG_GAS
	config->gas_rand_addr_lifetime = DEFAULT_RAND_ADDR_LIFETIME;
#endif /* CONFIG_GAS */

    config->dot11RSNAConfigPMKLifetime = DEFAULT_PMK_LIFE_TIME;

	TX_FUNC_END("      ");

	return config;
}


#ifndef CONFIG_NO_STDOUT_DEBUG
/**
 * wpa_config_debug_dump_networks - Debug dump of configured networks
 * @config: Configuration data from wpa_config_read()
 */
void wpa_config_debug_dump_networks(struct wpa_config *config)
{
#ifdef CONFIG_PRIO_GROUP
	int prio;
	struct wpa_ssid *ssid;

	for (prio = 0; prio < config->num_prio; prio++) {
		ssid = config->pssid[prio];
		wpa_printf_dbg(MSG_DEBUG, "Priority group %d",
			   ssid->priority);
		while (ssid) {
			wpa_printf_dbg(MSG_DEBUG, "   id=%d ssid='%s'",
				   ssid->id,
				   wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
			ssid = ssid->pnext;
		}
	}
#endif /* CONFIG_PRIO_GROUP */
}
#endif /* CONFIG_NO_STDOUT_DEBUG */


struct global_parse_data {
	char *name;
	int (*parser)(const struct global_parse_data *data,
		      struct wpa_config *config, const char *value);
#if 1
	int (*get)(const char *name, struct wpa_config *config, long offset,
		   char *buf, size_t buflen, int pretty_print);
#endif /* 0 */
	void *param1, *param2, *param3;
	unsigned int changed_flag;
};


static int wpa_global_config_parse_int(const struct global_parse_data *data,
				       struct wpa_config *config, const char *pos)
{
	int val, *dst;
	char *end;

	dst = (int *) (((u8 *) config) + (long) data->param1);
	val = strtol(pos, &end, 0);
	if (*end) {
		wpa_printf(MSG_ERROR, "Invalid number \"%s\"", pos);
		return -1;
	}
	*dst = val;

	wpa_printf_dbg(MSG_DEBUG, "%s=%d", data->name, *dst);

	if (data->param2 && *dst < (long) data->param2) {
		wpa_printf(MSG_ERROR, "Too small %s (value=%d "
			   "min_value=%ld)", data->name, *dst,
			   (long) data->param2);
		*dst = (long) data->param2;
		return -1;
	}

	if (data->param3 && *dst > (long) data->param3) {
		wpa_printf(MSG_ERROR, "Too large %s (value=%d "
			   "max_value=%ld)", data->name, *dst,
			   (long) data->param3);
		*dst = (long) data->param3;
		return -1;
	}

	return 0;
}


static int wpa_global_config_parse_str(const struct global_parse_data *data,
				       struct wpa_config *config,
				       const char *pos)
{
	size_t len;
	char **dst, *tmp;

	len = os_strlen(pos);
	if (data->param2 && len < (size_t) data->param2) {
		wpa_printf(MSG_ERROR, "Too short %s (len=%lu "
			   "min_len=%ld)", data->name,
			   (unsigned long) len, (long) data->param2);
		return -1;
	}

	if (data->param3 && len > (size_t) data->param3) {
		wpa_printf(MSG_ERROR, "Too long %s (len=%lu "
			   "max_len=%ld)", data->name,
			   (unsigned long) len, (long) data->param3);
		return -1;
	}

	if (has_newline(pos)) {
		wpa_printf(MSG_ERROR, "Invalid %s value with newline",
			   data->name);
		return -1;
	}

	tmp = os_strdup(pos);
	if (tmp == NULL)
		return -1;

	dst = (char **) (((u8 *) config) + (long) data->param1);
	os_free(*dst);
	*dst = tmp;
	wpa_printf_dbg(MSG_DEBUG, "%s='%s'", data->name, *dst);

	return 0;
}


#ifdef	CONFIG_BGSCAN
static int wpa_config_process_bgscan(const struct global_parse_data *data,
				     struct wpa_config *config,
				     const char *pos)
{
	size_t len;
	char *tmp;
	int res;

	tmp = wpa_config_parse_string(pos, &len);
	if (tmp == NULL) {
		wpa_printf(MSG_ERROR, "Failed to parse %s",
			   data->name);
		return -1;
	}

	res = wpa_global_config_parse_str(data, config, tmp);
	os_free(tmp);
	return res;
}
#endif	/* CONFIG_BGSCAN */


#ifdef CONFIG_SUPP27_CONFIG
static int wpa_global_config_parse_bin(const struct global_parse_data *data,
				       struct wpa_config *config, const char *pos)
{
	struct wpabuf **dst, *tmp;

	tmp = wpabuf_parse_bin(pos);
	if (!tmp)
		return -1;

	dst = (struct wpabuf **) (((u8 *) config) + (long) data->param1);
	wpabuf_free(*dst);
	*dst = tmp;
	wpa_printf_dbg(MSG_DEBUG, "%s", data->name);

	return 0;
}
#endif /* CONFIG_SUPP27_CONFIG */


static int wpa_config_process_freq_list(const struct global_parse_data *data,
					struct wpa_config *config, const char *value)
{
	int *freqs;

	freqs = wpa_config_parse_int_array(value);
	if (freqs == NULL)
		return -1;
	if (freqs[0] == 0) {
		os_free(freqs);
		freqs = NULL;
	}
	os_free(config->freq_list);
	config->freq_list = freqs;
	return 0;
}


#ifdef CONFIG_P2P
#ifdef	CONFIG_P2P_UNUSED_CMD
static int wpa_global_config_parse_ipv4(const struct global_parse_data *data,
					struct wpa_config *config, const char *pos)
{
#if 0	/* by Bright */
	u32 *dst;
	struct hostapd_ip_addr addr;

	if (hostapd_parse_ip_addr(pos, &addr) < 0)
		return -1;
	if (addr.af != AF_INET)
		return -1;

	dst = (u32 *) (((u8 *) config) + (long) data->param1);
	os_memcpy(dst, &addr.u.v4.s_addr, 4);
	wpa_printf_dbg(MSG_DEBUG, "%s = 0x%x", data->name,
		   WPA_GET_BE32((u8 *) dst));
#else
da16x_p2p_prt("[%s] What do I do for this function ???\n", __func__);
#endif	/* 0 */

	return 0;
}
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif /* CONFIG_P2P */


static int wpa_config_process_country(const struct global_parse_data *data,
				      struct wpa_config *config, const char *pos)
{
	if (!pos[0] || !pos[1]) {
		wpa_printf_dbg(MSG_DEBUG, "Invalid country set");
		return -1;
	}
	config->country[0] = pos[0];
	config->country[1] = pos[1];
	wpa_printf_dbg(MSG_DEBUG, "country='%c%c'",
		   config->country[0], config->country[1]);
	return 0;
}


static int wpa_config_process_load_dynamic_eap(
	const struct global_parse_data *data, struct wpa_config *config,
	const char *so)
{
	int ret;
	wpa_printf_dbg(MSG_DEBUG, "load_dynamic_eap=%s", so);
	ret = eap_peer_method_load(so);
	if (ret == -2) {
		wpa_printf_dbg(MSG_DEBUG, "This EAP type was already loaded - not "
			   "reloading.");
	} else if (ret) {
		wpa_printf(MSG_ERROR, "Failed to load dynamic EAP "
			   "method '%s'.", so);
		return -1;
	}

	return 0;
}


#ifdef CONFIG_WPS

static int wpa_config_process_uuid(const struct global_parse_data *data,
				   struct wpa_config *config, const char *pos)
{
	char buf[40];
	if (uuid_str2bin(pos, config->uuid)) {
		wpa_printf(MSG_ERROR, "Line %d: invalid UUID");
		return -1;
	}
	uuid_bin2str(config->uuid, buf, sizeof(buf));
	wpa_printf_dbg(MSG_DEBUG, "uuid=%s", buf);
	return 0;
}


static int wpa_config_process_device_type(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
#ifdef	CONFIG_WPS
	return wps_dev_type_str2bin(pos, config->device_type);
#else
	da16x_p2p_prt("[%s] Have to include CONIFIG_WPS feature...\n", __func__);
	return -1;
#endif	/* CONFIG_WPS */
}


static int wpa_config_process_os_version(const struct global_parse_data *data,
					 struct wpa_config *config, const char *pos)
{
	if (hexstr2bin(pos, config->os_version, 4)) {
		wpa_printf(MSG_ERROR, "Invalid os_version");
		return -1;
	}
	wpa_printf_dbg(MSG_DEBUG, "os_version=%08x",
		   WPA_GET_BE32(config->os_version));
	return 0;
}


static int wpa_config_process_wps_vendor_ext_m1(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	struct wpabuf *tmp;
	int len = os_strlen(pos) / 2;
	u8 *p;

	if (!len) {
		wpa_printf(MSG_ERROR,
			   "Invalid wps_vendor_ext_m1");
		return -1;
	}

	tmp = wpabuf_alloc(len);
	if (tmp) {
		p = wpabuf_put(tmp, len);

		if (hexstr2bin(pos, p, len)) {
			wpa_printf(MSG_ERROR,
				   "Invalid wps_vendor_ext_m1");
			wpabuf_free(tmp);
			return -1;
		}

		wpabuf_free(config->wps_vendor_ext_m1);
		config->wps_vendor_ext_m1 = tmp;
	} else {
		wpa_printf(MSG_ERROR, "Can not allocate "
			   "memory for wps_vendor_ext_m1");
		return -1;
	}

	return 0;
}

#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
static int wpa_config_process_sec_device_type(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
#ifdef	CONFIG_WPS
	int idx;
#endif	/* CONFIG_WPS */

	if (config->num_sec_device_types >= MAX_SEC_DEVICE_TYPES) {
		wpa_printf(MSG_ERROR, "Too many sec_device_type "
			   "items");
		return -1;
	}

#ifdef	CONFIG_WPS
	idx = config->num_sec_device_types;

	if (wps_dev_type_str2bin(pos, config->sec_device_type[idx]))
		return -1;

	config->num_sec_device_types++;
	return 0;
#else
da16x_p2p_prt("[%s] Have to include CONIFIG_WPS feature...\n", __func__);
	return -1;
#endif	/* CONFIG_WPS */
}


#ifdef	CONFIG_P2P_UNUSED_CMD
static int wpa_config_process_p2p_pref_chan(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	struct p2p_channel *pref = NULL, *n;
	unsigned int num = 0;
	const char *pos2;
	u8 op_class, chan;

	/* format: class:chan,class:chan,... */

	while (*pos) {
		op_class = atoi(pos);
		pos2 = os_strchr(pos, ':');
		if (pos2 == NULL)
			goto fail;
		pos2++;
		chan = atoi(pos2);

		n = os_realloc_array(pref, num + 1,
				     sizeof(struct p2p_channel));
		if (n == NULL)
			goto fail;
		pref = n;
		pref[num].op_class = op_class;
		pref[num].chan = chan;
		num++;

		pos = os_strchr(pos2, ',');
		if (pos == NULL)
			break;
		pos++;
	}

	os_free(config->p2p_pref_chan);
	config->p2p_pref_chan = pref;
	config->num_p2p_pref_chan = num;
	wpa_hexdump(MSG_DEBUG, "P2P: Preferred class/channel pairs",
		    (u8 *) config->p2p_pref_chan,
		    config->num_p2p_pref_chan * sizeof(struct p2p_channel));

	return 0;

fail:
	os_free(pref);
	wpa_printf(MSG_ERROR, "Invalid p2p_pref_chan list");
	return -1;
}


static int wpa_config_process_p2p_no_go_freq(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	int ret;

	ret = freq_range_list_parse(&config->p2p_no_go_freq, pos);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Invalid p2p_no_go_freq");
		return -1;
	}

	wpa_printf_dbg(MSG_DEBUG, "P2P: p2p_no_go_freq with %u items",
		   config->p2p_no_go_freq.num);

	return 0;
}
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif /* CONFIG_P2P */

#ifdef CONFIG_INTERWORKING
static int wpa_config_process_hessid(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	if (hwaddr_aton2(pos, config->hessid) < 0) {
		wpa_printf(MSG_ERROR, "Invalid hessid '%s'",
			   pos);
		return -1;
	}

	return 0;
}
#endif /* CONFIG_INTERWORKING */


#ifdef CONFIG_SAE
static int wpa_config_process_sae_groups(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	int *groups = wpa_config_parse_int_array(pos);
	if (groups == NULL) {
		wpa_printf(MSG_ERROR, "Invalid sae_groups '%s'",
			   pos);
		return -1;
	}

	os_free(config->sae_groups);
	config->sae_groups = groups;

	return 0;
}
#endif /* CONFIG_SAE */

#ifdef	CONFIG_AP_VENDOR_ELEM
static int wpa_config_process_ap_vendor_elements(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	struct wpabuf *tmp;
	int len = os_strlen(pos) / 2;
	u8 *p;

	if (!len) {
		wpa_printf(MSG_ERROR, "Invalid ap_vendor_elements");
		return -1;
	}

	tmp = wpabuf_alloc(len);
	if (tmp) {
		p = wpabuf_put(tmp, len);

		if (hexstr2bin(pos, p, len)) {
			wpa_printf(MSG_ERROR, "Invalid "
				   "ap_vendor_elements");
			wpabuf_free(tmp);
			return -1;
		}

		wpabuf_free(config->ap_vendor_elements);
		config->ap_vendor_elements = tmp;
	} else {
		wpa_printf(MSG_ERROR, "Cannot allocate memory for "
			   "ap_vendor_elements");
		return -1;
	}

	return 0;
}
#endif	/* CONFIG_AP_VENDOR_ELEM */


#ifdef CONFIG_CTRL_IFACE
static int wpa_config_process_no_ctrl_interface(
	const struct global_parse_data *data,
	struct wpa_config *config, const char *pos)
{
	wpa_printf_dbg(MSG_DEBUG, "no_ctrl_interface -> ctrl_interface=NULL");
	os_free(config->ctrl_interface);
	config->ctrl_interface = NULL;
	return 0;
}
#endif /* CONFIG_CTRL_IFACE */


static int wpa_config_get_int(const char *name, struct wpa_config *config,
			      long offset, char *buf, size_t buflen,
			      int pretty_print)
{
	int *val = (int *) (((u8 *) config) + (long) offset);

	if (pretty_print)
		return os_snprintf(buf, buflen, "%s=%d\n", name, *val);
	return os_snprintf(buf, buflen, "%d", *val);
}


static int wpa_config_get_str(const char *name, struct wpa_config *config,
			      long offset, char *buf, size_t buflen,
			      int pretty_print)
{
	char **val = (char **) (((u8 *) config) + (long) offset);
	int res;

	if (pretty_print)
		res = os_snprintf(buf, buflen, "%s=%s\n", name,
				  *val ? *val : "null");
	else if (!*val)
		return -1;
	else
		res = os_snprintf(buf, buflen, "%s", *val);
	if (os_snprintf_error(buflen, res))
		res = -1;

	return res;
}


#ifdef OFFSET
#undef OFFSET
#endif /* OFFSET */
/* OFFSET: Get offset of a variable within the wpa_config structure */
#define OFFSET(v) ((void *) &((struct wpa_config *) 0)->v)

#define FUNC(f) #f, wpa_config_process_ ## f, NULL, OFFSET(f), NULL, NULL
#define FUNC_NO_VAR(f) #f, wpa_config_process_ ## f, NULL, NULL, NULL, NULL
#define _INT(f) #f, wpa_global_config_parse_int, wpa_config_get_int, OFFSET(f)
#define INT(f) _INT(f), NULL, NULL
#define INT_RANGE(f, min, max) _INT(f), (void *) min, (void *) max
#define _STR(f) #f, wpa_global_config_parse_str, wpa_config_get_str, OFFSET(f)
#define STR(f) _STR(f), NULL, NULL
#define STR_RANGE(f, min, max) _STR(f), (void *) min, (void *) max
#ifdef CONFIG_SUPP27_CONFIG
#define BIN(f) #f, wpa_global_config_parse_bin, NULL, OFFSET(f), NULL, NULL
#endif /* CONFIG_SUPP27_CONFIG */

#define IPV4(f) #f, wpa_global_config_parse_ipv4, wpa_config_get_ipv4,  \
	OFFSET(f), NULL, NULL

static const struct global_parse_data global_fields[] = {
#ifdef CONFIG_CTRL_IFACE
	{ STR(ctrl_interface), 0 },
	{ FUNC_NO_VAR(no_ctrl_interface), 0 },
#ifdef CONFIG_SUPP27_IFACE
	{ STR(ctrl_interface_group), 0 } /* deprecated */,
#endif /* CONFIG_SUPP27_IFACE */
#endif /* CONFIG_CTRL_IFACE */
#ifdef CONFIG_MACSEC
	{ INT_RANGE(eapol_version, 1, 3), 0 },
#else /* CONFIG_MACSEC */
	{ INT_RANGE(eapol_version, 1, 2), 0 },
#endif /* CONFIG_MACSEC */

#ifndef FEATURE_USE_DEFAULT_AP_SCAN
	{ INT(ap_scan), 0 },
#endif /* FEATURE_USE_DEFAULT_AP_SCAN */

#ifdef	CONFIG_BGSCAN
	{ FUNC(bgscan), 0 },
#endif	/* CONFIG_BGSCAN */

#ifdef CONFIG_MESH
	{ INT(user_mpm), 0 },
	{ INT_RANGE(max_peer_links, 0, 255), 0 },
	{ INT(mesh_max_inactivity), 0 },
	{ INT(dot11RSNASAERetransPeriod), 0 },
#endif /* CONFIG_MESH */

#ifdef CONFIG_SCAN_OFFLOAD
	{ INT(disable_scan_offload), 0 },
#endif /* CONFIG_SCAN_OFFLOAD */
#ifdef	CONFIG_FAST_REAUTH
	{ INT(fast_reauth), 0 },
#endif	/* CONFIG_FAST_REAUTH */

#ifdef	CONFIG_OPENSSL_MOD
	{ STR(opensc_engine_path), 0 },
	{ STR(pkcs11_engine_path), 0 },
	{ STR(pkcs11_module_path), 0 },

	{ STR(openssl_ciphers), 0 },
#endif	/* CONFIG_OPENSSL_MOD */

#ifdef CONFIG_PCSC_FUNCS
	{ STR(pcsc_reader), 0 },
	{ STR(pcsc_pin), 0 },
#endif /* CONFIG_PCSC_FUNCS */
#ifdef CONFIG_EXT_SIM
	{ INT(external_sim), 0 },
#endif /* CONFIG_EXT_SIM */

	{ STR(driver_param), 0 },
	{ INT(dot11RSNAConfigPMKLifetime), 0 },
	{ INT(dot11RSNAConfigPMKReauthThreshold), 0 },
	{ INT(dot11RSNAConfigSATimeout), 0 },

#ifndef CONFIG_NO_CONFIG_WRITE
	{ INT(update_config), 0 },
#endif /* CONFIG_NO_CONFIG_WRITE */
	{ FUNC_NO_VAR(load_dynamic_eap), 0 },
#ifdef CONFIG_WPS
	{ FUNC(uuid), CFG_CHANGED_UUID },
	{ INT_RANGE(auto_uuid, 0, 1), 0 },
	{ STR_RANGE(device_name, 0, 32), CFG_CHANGED_DEVICE_NAME },
	{ STR_RANGE(manufacturer, 0, 64), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(model_name, 0, 32), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(model_number, 0, 32), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(serial_number, 0, 32), CFG_CHANGED_WPS_STRING },
	{ FUNC(device_type), CFG_CHANGED_DEVICE_TYPE },
	{ FUNC(os_version), CFG_CHANGED_OS_VERSION },
	{ STR(config_methods), CFG_CHANGED_CONFIG_METHODS },
	{ INT_RANGE(wps_cred_processing, 0, 2), 0 },
	{ FUNC(wps_vendor_ext_m1), CFG_CHANGED_VENDOR_EXTENSION },
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
	{ FUNC(sec_device_type), CFG_CHANGED_SEC_DEVICE_TYPE },
	{ INT(p2p_listen_reg_class), 0 },
	{ INT(p2p_listen_channel), 0 },
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ INT(p2p_oper_reg_class), CFG_CHANGED_P2P_OPER_CHANNEL },
	{ INT(p2p_oper_channel), CFG_CHANGED_P2P_OPER_CHANNEL },
#endif	/* CONFIG_P2P_UNUSED_CMD */

#ifdef	CONFIG_GAS
	{ INT_RANGE(p2p_go_intent, 0, 15), 0 },
	{ STR(p2p_ssid_postfix), CFG_CHANGED_P2P_SSID_POSTFIX },
#endif	/* CONFIG_GAS */

#ifdef	CONFIG_P2P_UNUSED_CMD
	{ INT_RANGE(persistent_reconnect, 0, 1), 0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */

#ifdef	CONFIG_P2P_UNUSED_CMD
	{ INT_RANGE(p2p_intra_bss, 0, 1), CFG_CHANGED_P2P_INTRA_BSS },
#endif	/* CONFIG_P2P_UNUSED_CMD */

	{ INT(p2p_group_idle), 0 },
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ FUNC(p2p_pref_chan), CFG_CHANGED_P2P_PREF_CHAN },
	{ FUNC(p2p_no_go_freq), CFG_CHANGED_P2P_PREF_CHAN },
	{ INT_RANGE(p2p_add_cli_chan, 0, 1), 0 },
	{ INT_RANGE(p2p_optimize_listen_chan, 0, 1), 0 },
	{ INT(p2p_go_ht40), 0 },
	{ INT(p2p_go_vht), 0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */
	{ INT(p2p_disabled), 0 },
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ INT_RANGE(p2p_go_ctwindow, 0, 127), 0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */
	{ INT(p2p_no_group_iface), 0 },
#ifdef	CONFIG_P2P_OPTION
	{ INT_RANGE(p2p_ignore_shared_freq, 0, 1), 0 },
#endif	/* CONFIG_P2P_OPTION */
#ifdef	CONFIG_P2P_UNUSED_CMD
	{ IPV4(ip_addr_go), 0 },
	{ IPV4(ip_addr_mask), 0 },
	{ IPV4(ip_addr_start), 0 },
	{ IPV4(ip_addr_end), 0 },
	{ INT_RANGE(p2p_cli_probe, 0, 1), 0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif /* CONFIG_P2P */

	{ FUNC(country), CFG_CHANGED_COUNTRY },
	{ INT(bss_max_count), 0 },

#ifndef CONFIG_IMMEDIATE_SCAN
	{ INT(bss_expiration_age), 0 },
	{ INT(bss_expiration_scan_count), 0 },
#endif /* CONFIG_IMMEDIATE_SCAN */

	{ INT_RANGE(filter_ssids, 0, 1), 0 },
#ifdef CONFIG_SCAN_FILTER_RSSI
	{ INT_RANGE(filter_rssi, -100, 0), 0 },
#endif /* CONFIG_SCAN_FILTER_RSSI */
	{ INT(max_num_sta), 0 },
	{ INT_RANGE(ap_isolate, 0, 1), 0 },
	{ INT_RANGE(disassoc_low_ack, 0, 1), 0 },

#ifdef CONFIG_HS20
	{ INT_RANGE(hs20, 0, 1), 0 },
#endif /* CONFIG_HS20 */

#ifdef	CONFIG_INTERWORKING
	{ INT_RANGE(interworking, 0, 1), 0 },
	{ FUNC(hessid), 0 },
#endif	/* CONFIG_INTERWORKING */

	{ INT_RANGE(access_network_type, 0, 15), 0 },

#ifdef	CONFIG_P2P_UNUSED_CMD
	{ INT_RANGE(go_interworking, 0, 1), 0 },
	{ INT_RANGE(go_access_network_type, 0, 15), 0 },
	{ INT_RANGE(go_internet, 0, 1), 0 },
	{ INT_RANGE(go_venue_group, 0, 255), 0 },
	{ INT_RANGE(go_venue_type, 0, 255), 0 },
#endif	/* CONFIG_P2P_UNUSED_CMD */

	{ INT_RANGE(pbc_in_m1, 0, 1), 0 },
	{ STR(autoscan), 0 },

#ifdef CONFIG_WPS_NFC
	{ INT_RANGE(wps_nfc_dev_pw_id, 0x10, 0xffff),
	  CFG_CHANGED_NFC_PASSWORD_TOKEN },
	{ BIN(wps_nfc_dh_pubkey), CFG_CHANGED_NFC_PASSWORD_TOKEN },
	{ BIN(wps_nfc_dh_privkey), CFG_CHANGED_NFC_PASSWORD_TOKEN },
	{ BIN(wps_nfc_dev_pw), CFG_CHANGED_NFC_PASSWORD_TOKEN },
#endif /* CONFIG_WPS_NFC */

#ifdef CONFIG_P2P_UNUSED_CMD
	{ STR(ext_password_backend), CFG_CHANGED_EXT_PW_BACKEND },
#endif /* CONFIG_P2P_UNUSED_CMD */
#ifdef CONFIG_P2P
	{ INT(p2p_go_max_inactivity), 0 },
#endif /* CONFIG_P2P */

#ifdef	CONFIG_AP
	{ INT(ap_max_inactivity), 0 },
#endif	/* CONFIG_AP */
#ifdef	CONFIG_INTERWORKING
	{ INT_RANGE(auto_interworking, 0, 1), 0 },
#endif	/* CONFIG_INTERWORKING */
	{ INT(okc), 0 },
#ifdef CONFIG_IEEE80211W
	{ INT(pmf), 0 },
#endif /* CONFIG_IEEE80211W */
#ifdef	CONFIG_SAE
	{ FUNC(sae_groups), 0 },
#endif	/* CONFIG_SAE */
	{ INT(dtim_period), DTIM_PERIOD_DEFAULT },
	{ INT(beacon_int), BEACON_INT_DEFAULT },
#ifdef	CONFIG_AP_VENDOR_ELEM
	{ FUNC(ap_vendor_elements), 0 },
#endif	/* CONFIG_AP_VENDOR_ELEM */
#ifdef CONFIG_IGNOR_OLD_SCAN
	{ INT_RANGE(ignore_old_scan_res, 0, 1), 0 },
#endif /* CONFIG_IGNOR_OLD_SCAN */
	{ FUNC(freq_list), 0 },
	{ INT(scan_cur_freq), 0 },
#ifdef	CONFIG_SCHED_SCAN
	{ INT(sched_scan_interval), 0 },
	{ INT(sched_scan_start_delay), 0 },
#endif	/* CONFIG_SCHED_SCAN */

#ifdef CONFIG_TDLS
	{ INT(tdls_external_control), 0},
#endif /* CONFIG_TDLS */
	{ STR(osu_dir), 0 },
#ifdef	CONFIG_WOWLAN
	{ STR(wowlan_triggers), CFG_CHANGED_WOWLAN_TRIGGERS },
#endif	/* CONFIG_WOWLAN */
#ifdef	CONFIG_P2P
	{ INT(p2p_search_delay), 0},
#endif	/* CONFIG_P2P */

	{ INT(mac_addr), 0 },
#ifdef CONFIG_RANDOM_ADDR
	{ INT(rand_addr_lifetime), 0 },
	{ INT(preassoc_mac_addr), 0 },
#endif /* CONFIG_RANDOM_ADDR */
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT(key_mgmt_offload), 0},
	{ INT(passive_scan), 0 },
	{ INT(reassoc_same_bss_optim), 0 },
	{ INT(wps_priority), 0},
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef CONFIG_FST
	{ STR_RANGE(fst_group_id, 1, FST_MAX_GROUP_ID_LEN), 0 },
	{ INT_RANGE(fst_priority, 1, FST_MAX_PRIO_VALUE), 0 },
	{ INT_RANGE(fst_llt, 1, FST_MAX_LLT_MS), 0 },
#endif /* CONFIG_FST */
#ifdef	CONFIG_SUPP27_CONFIG
	{ INT_RANGE(cert_in_cb, 0, 1), 0 },
	{ INT_RANGE(wpa_rsc_relaxation, 0, 1), 0 },
#endif	/* CONFIG_SUPP27_CONFIG */

#ifdef	CONFIG_SCHED_SCAN
	{ STR(sched_scan_plans), CFG_CHANGED_SCHED_SCAN_PLANS },
#endif	/* CONFIG_SCHED_SCAN */

#ifdef CONFIG_MBO
	{ STR(non_pref_chan), 0 },
	{ INT_RANGE(mbo_cell_capa, MBO_CELL_CAPA_AVAILABLE,
		    MBO_CELL_CAPA_NOT_SUPPORTED), 0 },
	{ INT_RANGE(disassoc_imminent_rssi_threshold, -120, 0), 0 },
	{ INT_RANGE(oce, 0, 3), 0 },
#endif /* CONFIG_MBO */

#ifdef	CONFIG_GAS
	{ INT(gas_address3), 0 },
#endif	/* CONFIG_GAS */

#ifdef	CONFIG_SUPP27_CONFIG
	{ INT_RANGE(ftm_responder, 0, 1), 0 },
	{ INT_RANGE(ftm_initiator, 0, 1), 0 },
#endif	/* CONFIG_SUPP27_CONFIG */
#ifdef	CONFIG_GAS
	{ INT(gas_rand_addr_lifetime), 0 },
	{ INT_RANGE(gas_rand_mac_addr, 0, 2), 0 },
#endif	/* CONFIG_GAS */
#ifdef	CONFIG_DPP
	{ INT_RANGE(dpp_config_processing, 0, 2), 0 },
#endif	/* CONFIG_DPP */
};

#undef FUNC
#undef _INT
#undef INT
#undef INT_RANGE
#undef _STR
#undef STR
#undef STR_RANGE
#undef BIN
#undef IPV4
#define NUM_GLOBAL_FIELDS ARRAY_SIZE(global_fields)


#ifdef	UNUSED_CODE
int wpa_config_dump_values(struct wpa_config *config, char *buf, size_t buflen)
{
	int result = 0;
	size_t i;

	for (i = 0; i < NUM_GLOBAL_FIELDS; i++) {
		const struct global_parse_data *field = &global_fields[i];
		int tmp;

		if (!field->get)
			continue;

		tmp = field->get(field->name, config, (long) field->param1,
				 buf, buflen, 1);
		if (tmp < 0)
			return -1;
		buf += tmp;
		buflen -= tmp;
		result += tmp;
	}
	return result;
}


int wpa_config_get_value(const char *name, struct wpa_config *config,
			 char *buf, size_t buflen)
{
	size_t i;

	for (i = 0; i < NUM_GLOBAL_FIELDS; i++) {
		const struct global_parse_data *field = &global_fields[i];

		if (os_strcmp(name, field->name) != 0)
			continue;
		if (!field->get)
			break;
		return field->get(name, config, (long) field->param1,
				  buf, buflen, 0);
	}

	return -1;
}

int wpa_config_get_num_global_field_names(void)
{
	return NUM_GLOBAL_FIELDS;
}


const char * wpa_config_get_global_field_name(unsigned int i, int *no_var)
{
	if (i >= NUM_GLOBAL_FIELDS)
		return NULL;

	if (no_var)
		*no_var = !global_fields[i].param1;
	return global_fields[i].name;
}
#endif	/* UNUSED_CODE */


int wpa_config_process_global(struct wpa_config *config, char *pos)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < NUM_GLOBAL_FIELDS; i++) {
		const struct global_parse_data *field = &global_fields[i];
		size_t flen = os_strlen(field->name);

		if (os_strncmp(pos, field->name, flen) != 0 ||
		    pos[flen] != '=')
			continue;

		if (field->parser(field, config, pos + flen + 1)) {
			wpa_printf(MSG_ERROR, "failed to "
				   "parse '%s'.", pos);
			ret = -1;
		}
#ifdef CONFIG_WPS_NFC
		if (field->changed_flag == CFG_CHANGED_NFC_PASSWORD_TOKEN)
			config->wps_nfc_pw_from_config = 1;
#endif /* CONFIG_WPS_NFC */
		config->changed_parameters |= field->changed_flag;
		break;
	}

	if (i == NUM_GLOBAL_FIELDS) {
#ifdef CONFIG_AP
		if (os_strncmp(pos, "wmm_ac_", 7) == 0) {
			char *tmp = os_strchr(pos, '=');
			if (tmp == NULL) {
				wpa_printf(MSG_ERROR, "Invalid "
					   "'%s'", pos);
				return -1;
			}
			*tmp++ = '\0';
#ifdef	CONFIG_AP_WMM
			if (hostapd_config_wmm_ac(config->wmm_ac_params, pos,
						  tmp)) {
				wpa_printf(MSG_ERROR, "invalid WMM "
					   "AC item");
				return -1;
			}
#endif	/* CONFIG_AP_WMM */
		}
#endif /* CONFIG_AP */
#if 0
		if (line < 0)
			return -1;
#endif /* 0 */

		wpa_printf(MSG_ERROR, "unknown global field '%s'.", pos);
		ret = -1;
	}

	return ret;
}

/* EOF */
