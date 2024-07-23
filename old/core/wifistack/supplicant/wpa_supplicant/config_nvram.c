/**
 *****************************************************************************************
 * @file	config_nvram.c
 * @brief	WPA Supplicant / Configuration backend: Windows registry from
 * wpa_supplicant-2.4
 *****************************************************************************************
 */

/*
 * WPA Supplicant / Configuration backend: Windows registry
 * Copyright (c) 2003-2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file implements a configuration backend for Windows registry. All the
 * configuration information is stored in the registry and the format for
 * network configuration fields is same as described in the sample
 * configuration file, wpa_supplicant.conf.
 *
 * Configuration data is in
 * \a HKEY_LOCAL_MACHINE\\SOFTWARE\\%wpa_supplicant\\configs
 * key. Each configuration profile has its own key under this. In terms of text
 * files, each profile would map to a separate text file with possibly multiple
 * networks. Under each profile, there is a networks key that lists all
 * networks as a subkey. Each network has set of values in the same way as
 * network block in the configuration file. In addition, blobs subkey has
 * possible blobs as values.
 *
 * Example network configuration block:
 * \verbatim
HKEY_LOCAL_MACHINE\SOFTWARE\wpa_supplicant\configs\test\networks\0000
   ssid="example"
   key_mgmt=WPA-PSK
\endverbatim
 *
 * Copyright (c) 2020-2022 Modified by Renesas Electronics.
 */

#include "includes.h"

#include "supp_common.h"
#include "supp_config.h"

#include "nvedit.h"
#include "environ.h"

#include "supp_driver.h"
#include "wpa_supplicant_i.h"

#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"

#include "uuid.h"

#define	ENVIRON_DEVICE	NVEDIT_SFLASH

#ifdef UNICODE
#define TSTR "%S"
#else /* UNICODE */
#define TSTR "%s"
#endif /* UNICODE */

#include "config_ssid.h"
#include "common_def.h"

#pragma GCC diagnostic ignored "-Wtype-limits"

extern int	get_run_mode(void);
extern u16	da16x_tls_version;
#ifdef	EAP_PEAP
extern int	da16x_peap_version;
#endif	/* EAP_PEAP */

extern dpm_supp_key_info_t	*dpm_supp_key_info;
extern dpm_supp_conn_info_t	*dpm_supp_conn_info;
extern UCHAR	fast_connection_sleep_flag;
extern char * wpa_config_write_string_hex(const u8 *value, size_t len);


int wpa_config_read_nvram_int(const char *name, int *_val)
{
	int val;
	char *valstr;

	valstr = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

	if (valstr == NULL) {
		da16x_nvram_prt("[%s] '%s' Key not found.\n", __func__, name);
		//*_val = -1;
		return -1;
	}

	val = atoi(valstr);
	*_val = val;
	return 0;
}

char * wpa_config_read_nvram_string(const char *name)
{
	char *valstr;
	valstr = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

	da16x_nvram_prt("[%s] valstr=0x%x\n", __func__, valstr);
	return valstr;
}

static int wpa_config_read_global_country(struct wpa_config *config)
{
	char *str;
	int ret = 0;

	str = wpa_config_read_nvram_string(ENV_COUNTRY);
	if (str == NULL)
		return 0;

	os_memcpy(config->country, str, sizeof(config->country));

	return ret;
}

#ifdef CONFIG_ACL
static int wpa_config_read_global_acl_mac(struct wpa_config *config)
{
	char *str;
	int ret = 0;
	int i = 0;

	char *_field;

	_field = os_malloc(9);

	if (_field == NULL)
		return -1;

	memset(_field, 0, 9);

	for(i=0; i < NUM_MAX_ACL; i++) {
		sprintf(_field, ENV_ACL_MAC"%d", i);
		str = wpa_config_read_nvram_string(_field);
		if (str == NULL) {
			os_free(_field);
			return 0;
		}

		if (hwaddr_aton(str, (u8 *)config->acl_mac[i].addr)) {
			os_free(_field);
			ret = -1;
		}
		
		if (!is_zero_ether_addr(config->acl_mac[i].addr))
			config->acl_num += 1;
	}

	os_free(_field);
	return ret;
}

#endif /* CONFIG_ACL */

#ifdef CONFIG_AP_WMM
#define WMM_MAX_STRLEN 15
int read_wmm_params_ap(struct hostapd_config *iconf)
{
	da16x_nvram_prt("[%s] START\n", __func__);

	char *_field;
	int i, val;

	_field = os_malloc(WMM_MAX_STRLEN);
	if (_field == NULL)
		return -1;

	for(i = 0; i <= 3; i++) {
		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, i, ENV_WMM_PS_CWMIN);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->tx_queue[i].cwmin = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, i, ENV_WMM_PS_CWMAX);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->tx_queue[i].cwmax = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, i, ENV_WMM_PS_AIFS);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->tx_queue[i].aifs = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, i, ENV_WMM_PS_BURST);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->tx_queue[i].burst = val;
	}
	os_free(_field);
	return 0;
}

int read_wmm_params_sta(struct hostapd_config *iconf)
{
	char *_field;
	int i, val;

	_field = os_malloc(WMM_MAX_STRLEN);
	if (_field == NULL)
		return -1;

	for(i = 0; i <= 3; i++) {
		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, i, ENV_WMM_PS_CWMIN);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->wmm_ac_params[i].cwmin = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, i, ENV_WMM_PS_CWMAX);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->wmm_ac_params[i].cwmax = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, i, ENV_WMM_PS_AIFS);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->wmm_ac_params[i].aifs = val;

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, i, ENV_WMM_PS_TXOP);
		if (wpa_config_read_nvram_int(_field, &val) != -1)
			iconf->wmm_ac_params[i].txop_limit = val;
	}
	os_free(_field);
	return 0;
}
#endif /* CONFIG_AP_WMM */


int hostapd_iconf_read(struct hostapd_config *iconf)
{
	int errors = 0;

	da16x_nvram_prt("[%s] Reading configuration NVRAM\n", __func__);

#ifdef CONFIG_AP_WMM
	if (read_wmm_params_ap(iconf))
		errors++;

	if (read_wmm_params_sta(iconf))
		errors++;
#endif /* CONFIG_AP_WMM */

	if (errors) {
		da16x_nvram_prt("[%s] Failed to read hostapd(softAP) "
				"configuration data\n", __func__);
		return -1;
	}

	da16x_nvram_prt("[%s] Hostapd(softAP) Configuration NVRAM read "
		       "successfully\n", __func__);
	return 0;
}

#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
static int wpa_config_read_global_uuid(struct wpa_config *config)
{
	char *str;
	int ret = 0;

	str = wpa_config_read_nvram_string("uuid");
	if (str == NULL)
		return 0;

	if (uuid_str2bin(str, config->uuid))
		ret = -1;

	os_free(str);

	return ret;
}
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

int wpa_config_read_global(struct wpa_config *config, char *ifname)
{
	int errors = 0;
	int val;

	extern void fc80211_set_roaming_mode(int mode);
	extern int fc80211_set_threshold(int min_thold, int max_thold);

	da16x_nvram_prt("[%s] START\n", __func__);

	wpa_config_set_global_defaults(config); /* FC9000 Only */

#ifdef	CONFIG_FAST_REAUTH
	if (wpa_config_read_nvram_int(ENV_FAST_REAUTH, &val) != -1) {
		config->fast_reauth = val;
	}
#endif	/* CONFIG_FAST_REAUTH */

#ifdef	CONFIG_AUTOSCAN
	config->autoscan = wpa_config_read_nvram_string(ENV_AUTOSCAN);
#endif	/* CONFIG_AUTOSCAN */

#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
	if (wpa_config_read_nvram_int("eapol_version", &config->eapol_version) == 0) {
		if (config->eapol_version < 1 ||
		    config->eapol_version > 2) {
			wpa_printf(MSG_ERROR, "Invalid EAPOL version (%d)",
				   config->eapol_version);
			errors++;
		}
	}
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#if 0 /* not used. 20181120 */
	if (wpa_config_read_nvram_int(ENV_TLS_VER, &val) != -1) {
		da16x_tls_version = (u16)val;
	}
#endif

#ifdef	CONFIG_SIMPLE_ROAMING
	if (wpa_config_read_nvram_int(ENV_ROAM, &val) != -1) {
		config->roam_state = (enum simple_roam_state) val;
		if (config->roam_state >= 1)
			fc80211_set_roaming_mode(1);
	}

	if (wpa_config_read_nvram_int(ENV_ROAM_THRESHOLD, &val) != -1) {
		config->roam_threshold = val;
		fc80211_set_threshold(config->roam_threshold, 0);
	}
#endif	/* CONFIG_SIMPLE_ROAMING */


#ifdef	CONFIG_P2P
	if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0) {	/* P2P */
		char *t1 = wpa_config_read_nvram_string(ENV_P2P_SSID_POSTFIX);
		if (t1)
			os_memcpy(config->p2p_ssid_postfix, t1, strlen(t1));

		if (wpa_config_read_nvram_int(ENV_P2P_LISTEN_CHANNEL, &val) != -1) {
			config->p2p_listen_channel = val;
		}

		if (wpa_config_read_nvram_int(ENV_P2P_OPER_CHANNEL, &val) != -1) {
			config->p2p_oper_channel = val;
		}

		if (wpa_config_read_nvram_int(ENV_P2P_FIND_TIMEOUT, &val) != -1) {
			config->p2p_find_timeout = val;
		}

		if (wpa_config_read_nvram_int(ENV_P2P_GO_INTENT, &val) != -1) {
			config->p2p_go_intent = val;
		}
#ifdef CONFIG_5G_SUPPORT
		if (wpa_config_read_nvram_int(ENV_BAND, &val) == -1) {
			config->band = WPA_SETBAND_AUTO;
		} else {
			config->band = (enum set_band) val;
		}
#endif /* CONFIG_5G_SUPPORT */
		
	}

#endif	/* CONFIG_P2P */


#ifdef	CONFIG_AP
	if (os_strcmp(ifname, STA_DEVICE_NAME) != 0) { /* Soft-AP & P2P */
		if (wpa_config_read_nvram_int(ENV_AP_MAX_INACTIVITY, &val) != -1) {
			config->ap_max_inactivity = val;
		}
		if (wpa_config_read_nvram_int(ENV_AP_SEND_KA, &val) != -1) {
			config->ap_send_ka = val;
		}

#ifdef	CONFIG_AP_WMM
		if (wpa_config_read_nvram_int(ENV_WMM_ENABLED, &val) != -1) {
			config->hostapd_wmm_enabled = val;
		}
		if (wpa_config_read_nvram_int(ENV_WMM_PS_ENABLED, &val) != -1) {
				config->hostapd_wmm_ps_enabled = val;
		}
#endif	/* CONFIG_AP_WMM */

#ifdef	CONFIG_AP_PARAMETERS
		if (wpa_config_read_nvram_int(ENV_AP_SET_GREENFIELD,
					      &config->greenfield) == -1) {
			config->greenfield = DEFAULT_GREENFIELD; //default value
		}

		if (wpa_config_read_nvram_int(ENV_AP_SET_HT_PROTECTION,
					      (int *)&config->ht_protection) == -1) {
			config->ht_protection = DEFAULT_HT_PROTECTION; //default value
		}

		if (wpa_config_read_nvram_int(ENV_RTS_THRESHOLD,
					      (int *)&config->rts_threshold) == -1) {
			config->rts_threshold = DEFAULT_RTS_THRESHOLD;
		}
#endif	/* CONFIG_AP_PARAMETERS */

#ifdef	CONFIG_ACL
		if (wpa_config_read_nvram_int(ENV_ACL_CMD, &val) != -1) {
			config->macaddr_acl = val;
		}
		wpa_config_read_global_acl_mac(config);
#endif	/* CONFIG_ACL */
	}
#endif	/* CONFIG_AP */

#ifdef	CONFIG_WPS
#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
	if (wpa_config_read_global_uuid(config))
		errors++;
	wpa_config_read_nvram_int("auto_uuid", &config->auto_uuid);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
	{
		char t_size = 0;

		t_size = strlen(wpa_config_read_nvram_string(ENV_DEVICE_NAME));
		if (t_size)
		{
			t_size++;
			if (strlen(config->device_name))
			{
				os_free(config->device_name);
			}
			config->device_name = os_malloc(t_size);
			memset(config->device_name, 0, t_size);
			os_memcpy(config->device_name,	wpa_config_read_nvram_string(ENV_DEVICE_NAME), t_size);
		}

		t_size = strlen(wpa_config_read_nvram_string(ENV_MANUFACTURER));
		if (t_size)
		{
			t_size++;
			
			if (strlen(config->manufacturer))
			{
				os_free(config->manufacturer);
			}
			config->manufacturer = os_malloc(t_size);
			memset(config->manufacturer, 0, t_size);
			os_memcpy(config->manufacturer,	wpa_config_read_nvram_string(ENV_MANUFACTURER), t_size);
		}

		t_size = strlen(wpa_config_read_nvram_string(ENV_MODEL_NAME));
		if (t_size)
		{
			t_size++;
			if (strlen(config->model_name))
			{
				os_free(config->model_name);
			}
			config->model_name = os_malloc(t_size);
			memset(config->model_name, 0, t_size);
			os_memcpy(config->model_name,	wpa_config_read_nvram_string(ENV_MODEL_NAME), t_size);
		}

		t_size = strlen(wpa_config_read_nvram_string(ENV_MODEL_NUMBER));
		if (t_size)
		{
			t_size++;
			if (strlen(config->model_number))
			{
				os_free(config->model_number);
			}
			config->model_number = os_malloc(t_size);
			memset(config->model_number, 0, t_size);
			os_memcpy(config->model_number,	wpa_config_read_nvram_string(ENV_MODEL_NUMBER), t_size);
		}

		t_size = strlen(wpa_config_read_nvram_string(ENV_SERIAL_NUMBER));
		if (t_size)
		{
			t_size++;
			if (strlen(config->serial_number))
			{
				os_free(config->serial_number);
			}
			config->serial_number = os_malloc(t_size);
			memset(config->serial_number, 0, t_size);
			os_memcpy(config->serial_number,	wpa_config_read_nvram_string(ENV_SERIAL_NUMBER), t_size);
		}
		
		t_size = strlen(wpa_config_read_nvram_string(ENV_CONFIG_METHODS));
		if (t_size)
		{
			t_size++;
			if (strlen(config->config_methods))
			{
				os_free(config->config_methods);
			}
			config->config_methods = os_malloc(t_size);
			memset(config->config_methods, 0, t_size);
			os_memcpy(config->config_methods,	wpa_config_read_nvram_string(ENV_CONFIG_METHODS), t_size);
		}
	}

	{
		char *t2 = wpa_config_read_nvram_string(ENV_DEVICE_TYPE);
		if (t2 && wps_dev_type_str2bin(t2, config->device_type))
			errors++;
	}
#endif	/* CONFIG_WPS */

#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
	wpa_config_read_nvram_int("bss_max_count", (int *) &config->bss_max_count);
	wpa_config_read_nvram_int("filter_ssids", &config->filter_ssids);
#if 0
	wpa_config_read_nvram_int("max_num_sta", (int *) &config->max_num_sta);
#endif /* 0 */
	wpa_config_read_nvram_int("disassoc_low_ack", (int *) &config->disassoc_low_ack);

	wpa_config_read_nvram_int("okc", &config->okc);
#ifdef CONFIG_IEEE80211W
	wpa_config_read_nvram_int("pmf", &val);
	config->pmf = val;
#endif /* CONFIG_IEEE80211W */
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#ifdef CONFIG_SAE
	/* SAE_GROUPS */
	{
		char tmp_sae_groups[3*MAX_SAE_GROUPS];
		memset(tmp_sae_groups, 0, 3*MAX_SAE_GROUPS);
		strcpy(tmp_sae_groups, wpa_config_read_nvram_string("sae_groups"));

		if (config->sae_groups == NULL)
		{
			config->sae_groups = os_malloc(sizeof(int)*6);
		}

		if (tmp_sae_groups[0] == '\0')
		{
			extern const unsigned char support_sae_groups[];
			for (int i = 0; i < MAX_SAE_GROUPS; i++) {
				config->sae_groups[i] = support_sae_groups[i];
				if (config->sae_groups[i] == 0) {
					break;
				}
			}
		}
		else
		{
			config->sae_groups[0] = atoi(strtok(tmp_sae_groups, " "));

			if (config->sae_groups[0] >= 0) {
				for (int i = 1; i <= MAX_SAE_GROUPS; i++) {
					config->sae_groups[i] = atoi(strtok(NULL, " "));
					if (config->sae_groups[i] == 0) {
						break;
					}
				}
			}
		}
	}
#endif /* CONFIG_SAE */

	if (wpa_config_read_global_country(config)) {
		errors++;
	}

#ifdef CONFIG_STA_COUNTRY_CODE
	 country_to_freq_range_list(&config->country_range, config->country);
#endif /* CONFIG_STA_COUNTRY_CODE */

#ifdef	CONFIG_LOG_MASK
	if (wpa_config_read_nvram_int(ENV_SUPP_LOG_MASK, &val) != -1) {
        extern void set_log_mask_whole(unsigned long mask);
        set_log_mask_whole(val);
	}
    
	if (wpa_config_read_nvram_int(ENV_SUPP_WPA_LOG_MASK, &val) != -1) {
        extern void set_log_mask_whole_wpa(int mask);
        set_log_mask_whole_wpa(val);
	}
#endif	/* CONFIG_LOG_MASK */

#ifdef CONFIG_OWE_TRANS
	wpa_config_read_nvram_int("owe_transition_mode", (int *) &config->owe_transition_mode);
#endif // CONFIG_OWE_TRANS

    if (wpa_config_read_nvram_int(ENV_PMK_LIFE_TIME, &val) != -1) {
        config->dot11RSNAConfigPMKLifetime = val;
    }

	return errors ? -1 : 0;
}

#define PRE_PSK							"PRE_PSK"
#define	PRE_PSK_CNT						"PRE_PSK_CNT"
#define	PRE_PSK_MAX_CNT					1
static struct wpa_ssid * wpa_config_read_network(int id)
{
	struct wpa_ssid *ssid;
	int i, errors = 0, datalen = 0;

	//const struct parse_data *field = &ssid_fields[id];

	TX_FUNC_START("");

	da16x_nvram_prt("[%s] Start of a new network(%d)\n", __func__, id);

	ssid = os_zalloc(sizeof(*ssid));

	if (ssid == NULL) {
		da16x_nvram_prt("[%s] ssid = NULL\n", __func__);
		return NULL;
	}

	dl_list_init(&ssid->psk_list);
	ssid->id = id;

	wpa_config_set_network_defaults(ssid);

	for (i = 0; network_fields[i].name != NULL; i++)
	{
		char *data;
		char name[32] = {0, };
		memset(name, 0, sizeof(name));
		datalen = 0;
		data = NULL;

		/* N0_ssid */
		sprintf(name, PREFIX_NETWORK_PROFILE, id, network_fields[i].name);
		data = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

		if (os_strcmp(network_fields[i].name, "ssid") == 0)
		{
			if (data != NULL) {
				ssid->ssid_len = strlen(data);
			} else { //trinity_0151110, trinity_0151110id is NULL in AP Mode -> Configure default SSID
				if(ssid->id == FIXED_NETWORK_ID_AP) {
					ssid->ssid = os_malloc(strlen(DEFAULT_AP_SSID)+1);
					if (ssid->ssid == NULL)
						return NULL;
					os_memcpy(ssid->ssid, DEFAULT_AP_SSID, strlen(DEFAULT_AP_SSID));
					os_memset(ssid->ssid + strlen(DEFAULT_AP_SSID), 0x0, 1);
					ssid->ssid_len = strlen(DEFAULT_AP_SSID);
					da16x_nvram_prt("[%s] SSID is not set. "
						"Configure default ssid with \"%s\"\n", __func__, DEFAULT_AP_SSID);
					write_str("ssid", ssid);
				}
			}
 		} else {
			datalen = strlen(data);
		}

#define MAX_DATA_LEN 64
		if (datalen > (MAX_DATA_LEN+2)) {
			if (data[0] == '\"' && data[datalen-1] != '\"') {
				data[MAX_DATA_LEN+1] = '\"';
			}

			datalen = (MAX_DATA_LEN+2);
			data[datalen] = '\0';
		}

		wpa_config_set(ssid, (char *)network_fields[i].name, (char *)data);
	}

	if (!(ssid->key_mgmt & WPA_KEY_MGMT_NONE) && (ssid->passphrase
#ifdef CONFIG_SAE
			|| ssid->sae_password
#endif /* CONFIG_SAE */
			)) {
		if (ssid->psk_set) {
			da16x_nvram_prt("[%s] Both PSK and passphrase "
				   "configured for network.\n", __func__);
			errors++;
		}

#undef	PSK_RAW_KEY_TIME_PRINT	//for log

		/* setup addition of Fast connect in sleep mode 1,2 */
		if (fast_connection_sleep_flag == pdTRUE && id == 0) {
			char *psk_raw_key = NULL;

#ifdef PSK_RAW_KEY_TIME_PRINT
			struct os_time now;
			int time = xTaskGetTickCount() * portTICK_PERIOD_MS;
			int time2 = time;
#endif /* PSK_RAW_KEY_TIME_PRINT */

			// ssid, passphrase, sae_passwrod º¯°æ ½Ã NVR_KEY_PSK_RAW_0 »èÁ¦ ÇÊ¿ä
			psk_raw_key = wpa_config_read_nvram_string(NVR_KEY_PSK_RAW_0);

			if (strlen(psk_raw_key) == 0)
			{
				wpa_config_update_psk(ssid);

				char *psk_buf = wpa_config_write_string_hex(ssid->psk, PMK_LEN);

				if (psk_buf)
				{
#ifdef PSK_RAW_KEY_TIME_PRINT
					time = (xTaskGetTickCount() * portTICK_PERIOD_MS - time;
					da16x_notice_prt("Update PSK: %d msec\n", time * 10);
#endif /* PSK_RAW_KEY_TIME_PRINT */

					write_nvram_string(NVR_KEY_PSK_RAW_0, psk_buf);
#ifdef PSK_RAW_KEY_TIME_PRINT
					time = (xTaskGetTickCount() * portTICK_PERIOD_MS) - time2;
					da16x_notice_prt("nvram Write PSK: %d msec\n", time * 10);
#endif /* PSK_RAW_KEY_TIME_PRINT */
					os_free(psk_buf);
				}
			}
			else
			{
				// set psk from NVR_KEY_PSK_RAW_0
				if (hexstr2bin(psk_raw_key, ssid->psk, PMK_LEN) == 0 && psk_raw_key[PMK_LEN * 2] == '\0')
				{
					ssid->psk_set = 1;
				}
			}
		}
		else
		{
			wpa_config_update_psk(ssid);
		}
	}

	if ((ssid->group_cipher & WPA_CIPHER_CCMP) &&
	    !(ssid->pairwise_cipher & WPA_CIPHER_CCMP) &&
	    !(ssid->pairwise_cipher & WPA_CIPHER_NONE)) {
		/* Group cipher cannot be stronger than the pairwise cipher. */
		da16x_nvram_prt("[%s] Removed CCMP from group cipher "
			   "list since it was not allowed for pairwise "
			   "cipher for network.\n", __func__);

		ssid->group_cipher &= ~WPA_CIPHER_CCMP;
	}

#ifdef	CONFIG_DPM_OPT_WIFI_MODE /* by Shingu 20180207 (DPM Optimize) */
	if (chk_dpm_mode() == pdTRUE) {
		if (ssid->wifi_mode == DA16X_WIFI_MODE_BGN) {
			da16x_notice_prt(">>> Wi-Fi mode : b/g/n -> b/g (for DPM)\n");
			ssid->wifi_mode = DA16X_WIFI_MODE_BG;
			ssid->disable_ht = 1;
			ssid->dpm_opt_wifi_mode = 1;
		} else if (ssid->wifi_mode == DA16X_WIFI_MODE_GN) {
			da16x_notice_prt(">>> Wi-Fi mode : g/n -> g-only (for DPM)\n");
			ssid->wifi_mode = DA16X_WIFI_MODE_G_ONLY;
			ssid->disable_ht = 1;
			ssid->dpm_opt_wifi_mode = 1;
		}
	}
#endif	/* CONFIG_DPM_OPT_WIFI_MODE */

	if (errors) {
		da16x_nvram_prt("[%s] errors = %d\n", __func__, errors);
		wpa_config_free_ssid(ssid);
		ssid = NULL;
	}

	return ssid;
}


int wpa_config_read_networks(struct wpa_config *config, char *ifname)
{
	struct wpa_ssid *ssid;
	int id = 0;
	char profile[11];
	int val;
	
	if (os_strcmp(ifname, STA_DEVICE_NAME) == 0)
		id = 0;
	else if (os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0)
		id = 1;
	else if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0)
		id = 2;
#ifdef CONFIG_MESH
	else if (os_strcmp(ifname, MESH_POINT_DEVICE_NAME) == 0)
		id = 3;
#endif /* CONFIG_MESH */

	/* Check  Network Profile */
	os_memset(profile, 0, 11);
	/* N0_Profile */
	sprintf(profile, PREFIX_NETWORK_PROFILE, id, "Profile");

	if (da16x_getenv(ENVIRON_DEVICE, "app", profile) == NULL
		|| strcmp(da16x_getenv(ENVIRON_DEVICE, "app", profile), "0") == 0) {
		da16x_nvram_prt("[%s] Continue(id:%d)\n",
					__func__, id);
		return -1;
	}

	if (wpa_config_read_nvram_int(ENV_TLS_VER, &val) != -1)
		da16x_tls_version = (u16)val;
#ifdef	EAP_PEAP
	if (wpa_config_read_nvram_int(ENV_PEAP_VER, &val) != -1)
		da16x_peap_version = val;
#endif	/* EAP_PEAP */

	ssid = wpa_config_read_network(id);

	if (ssid == NULL) {
		da16x_nvram_prt("[%s] Failed to parse network "
				"profile '%d'\n", __func__, id);
		return -1;
	}

	config->ssid = ssid;

#ifdef	CONFIG_AP
	//NVRAM name doesn't have NX prefix for ap_max_inactivity parameter. Required to sync up.
	//ap_max_inactivity parameter does not have NX prefix in NVRAM. So, Required to sync up.
	if (config->ap_max_inactivity) {
		config->ssid->ap_max_inactivity = config->ap_max_inactivity;
	}
#endif /* CONFIG_AP */

	return 0;
}

#if defined ( __SUPP_27_SUPPORT__ )
struct wpa_config * wpa_config_read(const char *name, char *ifname)
#else
struct wpa_config * wpa_config_read(char *ifname)
#endif // __SUPP_27_SUPPORT__
{
	struct wpa_config *config;

	config = wpa_config_alloc_empty(NULL, NULL);

	if (config == NULL) {
		da16x_nvram_prt("[%s] config == NULL\n", __func__);
		return NULL;
	}

	da16x_nvram_prt("[%s] Reading configuration profile(%s)\n", __func__, ifname);
	if (chk_dpm_wakeup() == pdTRUE) {
		if (wpa_supp_dpm_restore_config(config)) {
			da16x_dpm_prt("[DPM] %s : Failed to restore network "
					"infomation from Retention Memory\n",
					__func__);

			wpa_config_free(config);
			return NULL;
		}
	} else {
		wpa_config_read_global(config, ifname);
		wpa_config_read_networks(config, ifname);

#ifdef FAST_CONN_ASSOC_CH
extern int i3ed11_freq_to_ch(int freq);
		if (fast_connection_sleep_flag == pdTRUE) {
			int val;

			if (wpa_config_read_nvram_int(NVR_KEY_ASSOC_CH, &val) == 0) {
				if (val > 13) {
					config->scan_cur_freq = i3ed11_freq_to_ch(val);
				} else {
					config->scan_cur_freq = (u16)val;
				}
			}
		}
#endif /* FAST_CONN_ASSOC_CH */
	}
	
#ifdef	CONFIG_CONCURRENT
	if ((get_run_mode() == SYSMODE_STA_N_AP
#ifdef CONFIG_MESH
		|| get_run_mode() == SYSMODE_STA_N_MESH_PORTAL
#endif /* CONFIG_MESH */
		) && os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0) {

		int val;

		if (config->ssid == NULL)
			return config;

		if (wpa_config_read_nvram_int("N1_frequency", &val) == 0 && val)
			config->ssid->frequency = val;
		else
			config->ssid->frequency = FREQUENCE_DEFAULT;
#ifdef	CONFIG_P2P
	} else if (get_run_mode() == SYSMODE_STA_N_P2P &&
		   os_strcmp(ifname, P2P_DEVICE_NAME) == 0) {
		config->p2p_oper_reg_class = 81;
		config->p2p_oper_channel = 1;
		config->p2p_listen_reg_class = 81;
		config->p2p_listen_channel = 1;
#endif	/* CONFIG_P2P */
	}
#endif	/* CONFIG_CONCURRENT */

	return config;
}

void dpm_disconn_reconf(struct wpa_config *config, char *ifname)
{
	TX_FUNC_START("");

	wpa_config_free_ssid(config->ssid);
	wpa_config_read_networks(config, ifname);

	TX_FUNC_END("");
}

int wpa_config_write_nvram_int(const char *name, int val, int def)
{
	char *valstr_env = NULL;
	char valstr[11];

	da16x_nvram_prt("[%s] <%s>=<%d>\n", __func__, name, val);

	valstr_env = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

	if (val == def) {
		if (valstr_env == NULL) {
			da16x_nvram_prt("[%s] NVRAM Do Not Write: Default data "
				       "%s=%d\n", __func__, name, val);
		} else {
			da16x_nvram_prt("[%s] NVRAM Delete: Default data "
				       "%s=%d\n", __func__, name, val);
			da16x_unsetenv_temp(ENVIRON_DEVICE, "app", (char *)name);
		}
		return 0;
	}

	if (valstr_env != NULL && atoi(valstr_env) == val) {
		da16x_nvram_prt("[%s] NVRAM Do Not Write: Same data %s=%d\n",
			__func__, name, val);
		return 0 ;
	}

	da16x_nvram_prt("[%s] NVRAM Write: %s=%d\n", __func__, name, val);

	os_memset(valstr, 0, 11);
	sprintf(valstr, "%d", val);

	if (da16x_setenv_temp(ENVIRON_DEVICE, "app",
		(char *)name, valstr) == 0) {
		da16x_nvram_prt("[%s] NVRAM Write: Failed to set %s=%d: "
			"error %d\n", __func__, name, valstr);

		return -1;
	}

	return 0;
}


int wpa_config_write_nvram_string(const char *name, const char *val)
{
	char *valstr;

	da16x_nvram_prt("[%s] %s=%s\n", __func__, name, val);

	valstr = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

	if (strlen(val) == 0 && valstr != NULL) {
		da16x_nvram_prt("[%s] NVRAM unsetenv: %s=NULL\n", __func__, name);
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", (char *)name);
	} else if (strlen(val) > 0 && os_strcmp(valstr, val) != 0) {
		da16x_nvram_prt("[%s] NVRAM Write:%s=%s\n", __func__, name, val);

		if (da16x_setenv_temp(ENVIRON_DEVICE, "app",
			(char *)name, (char *)val) == 0) {
			da16x_nvram_prt("[%s] NVRAM Write: Failed to set %s=%d: "
				"error %s\n", __func__, name, val);
			return -1;
		}

		if (fast_connection_sleep_flag == pdTRUE) {
			if (strcmp(name, "N0_ssid") == 0
				|| strcmp(name, "N0_sae_password") == 0
				|| strcmp(name, "N0_psk") == 0)
			{
				//N0_ssid, N0_sae_password, N0_psk
				//PRINTF("%s=%s ==> %s\n", name, valstr, val); // degug
				//delete_tmp_nvram_env(name, NVR_KEY_PSK_RAW_0);
				da16x_unsetenv_temp(ENVIRON_DEVICE, "app", NVR_KEY_PSK_RAW_0);
			}
		}
	} else {
		da16x_nvram_prt("[%s] NVRAM Write: Same data %s=%s\n",
				__func__, name, val);
	}

	return 0;
}

#ifdef CONFIG_ACL
static int wpa_config_write_ap_acl_mac(struct wpa_config *config)
{
	int i;
	char *_field;
	char *value;

	_field = os_malloc(9);
	if (_field == NULL)
		return -1;
	memset(_field, 0, 9);

	value = os_malloc(25);
	if (value == NULL) {
		os_free(_field);
		return -1;
	}
	memset(value, 0, 25);

	if (config->acl_num == 0) { //acl clear
		sprintf(_field, ENV_ACL_MAC"*");
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", (char *)_field);
	} else {
		for(i=0; i < NUM_MAX_ACL; i++) {
	//	da16x_ap_prt("[%s:%d]%d acl_mac = "MACSTR"\n",__func__,__LINE__,i, MAC2STR(config->acl_mac[i].addr));
			if (config->acl_mac[i].addr == NULL) {
				da16x_nvram_prt("[%s] ACL_MAC is NULL!!\n", __func__);
				os_free(_field);
				os_free(value);
				return -1;
			}

			if (config->acl_num != 0) {
				sprintf(_field, ENV_ACL_MAC"%d", i);
				sprintf(value, MACSTR, MAC2STR(config->acl_mac[i].addr));
				wpa_config_write_nvram_string(_field, value);
				memset(_field, 0, 9);
				memset(value, 0, 25);
			}
		}

	}

	wpa_config_write_nvram_int(ENV_ACL_CMD, config->macaddr_acl, DEFAULT_ACL_CLEAR);

	os_free(_field);
	os_free(value);
	return 0;
}
#endif /* CONFIG_ACL */

#ifdef CONFIG_AP_WMM
static int write_wmm_params_ap(struct hostapd_config *iconf)
{
#define ecw2cw(ecw) ((1 << (ecw)) - 1)

	char *_field;
	const int aCWmin = 4, aCWmax = 10;
	int ac_idx, def_cwmin, def_cwmax, def_aifs, def_burst;

	_field = os_malloc(WMM_MAX_STRLEN);
	if (_field == NULL)
		return -1;

	da16x_unsetenv_temp(ENVIRON_DEVICE, "app", "wmm_AP_*");

	for(ac_idx = 0; ac_idx <= 3; ac_idx++) {
		if (ac_idx == 3) {			/* voice traffic */
			def_cwmin = ecw2cw(aCWmin);
			def_cwmax = ecw2cw(aCWmax);
			def_aifs = 7;
			def_burst = 0;
		} else if (ac_idx == 2) {	/* video traffic */
			def_cwmin = ecw2cw(aCWmin);
			def_cwmax = 4 * (ecw2cw(aCWmin) + 1) - 1;
			def_aifs = 3;
			def_burst = 0;
		} else if (ac_idx == 1) {	/* background traffic */
			def_cwmin = (ecw2cw(aCWmin) + 1) / 2 - 1;
			def_cwmax = ecw2cw(aCWmin);
			def_aifs = 1;
			def_burst = 30	;
		} else if (ac_idx == 0) {	/* best effort traffic */
			def_cwmin = (ecw2cw(aCWmin) + 1) / 4 - 1;
			def_cwmax = (ecw2cw(aCWmin) + 1) / 2 - 1;
			def_aifs = 1;
			def_burst = 15;
		}

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, ac_idx, ENV_WMM_PS_CWMIN);
		wpa_config_write_nvram_int(_field,
					   iconf->tx_queue[ac_idx].cwmin, def_cwmin);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, ac_idx, ENV_WMM_PS_CWMAX);
		wpa_config_write_nvram_int(_field,
					   iconf->tx_queue[ac_idx].cwmax, def_cwmax);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, ac_idx, ENV_WMM_PS_AIFS);
		wpa_config_write_nvram_int(_field,
					   iconf->tx_queue[ac_idx].aifs, def_aifs);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_AP, ac_idx, ENV_WMM_PS_BURST);
		wpa_config_write_nvram_int(_field,
					   iconf->tx_queue[ac_idx].burst, def_burst);
	}
	os_free(_field);

	return 0;
}

static int write_wmm_params_sta(struct hostapd_config *iconf)
{
	char *_field;
	const int aCWmin = 4, aCWmax = 10;
	int ac_idx, def_cwmin, def_cwmax, def_aifs, def_txop;

	_field = os_malloc(WMM_MAX_STRLEN);
	if (_field == NULL)
		return -1;

	da16x_unsetenv_temp(ENVIRON_DEVICE, "app", "wmm_STA_*");

	for(ac_idx = 0; ac_idx <= 3; ac_idx++) {
		if (ac_idx == 0) {			/* best effort traffic */
			def_cwmin = aCWmin;
			def_cwmax = aCWmax;
			def_aifs = 3;
			def_txop = 0;
		} else if (ac_idx == 1) {	/* background traffic */
			def_cwmin = aCWmin;
			def_cwmax = aCWmax;
			def_aifs = 7;
			def_txop = 0;
		} else if (ac_idx == 2) {	/* video traffic */
			def_cwmin = aCWmin - 1;
			def_cwmax = aCWmin;
			def_aifs = 2;
			def_txop = 3008 / 32;
		} else if (ac_idx == 3) {	/* voice traffic */
			def_cwmin = aCWmin - 2;
			def_cwmax = aCWmin - 1;
			def_aifs = 2;
			def_txop = 1504 / 32;
		}

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, ac_idx, ENV_WMM_PS_CWMIN);
		wpa_config_write_nvram_int(_field,
					   iconf->wmm_ac_params[ac_idx].cwmin, def_cwmin);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, ac_idx, ENV_WMM_PS_CWMAX);
		wpa_config_write_nvram_int(_field,
					   iconf->wmm_ac_params[ac_idx].cwmax, def_cwmax);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, ac_idx, ENV_WMM_PS_AIFS);
		wpa_config_write_nvram_int(_field,
					   iconf->wmm_ac_params[ac_idx].aifs, def_aifs);

		memset(_field, 0, WMM_MAX_STRLEN);
		sprintf(_field, ENV_WMM_PRAMS_STA, ac_idx, ENV_WMM_PS_TXOP);
		wpa_config_write_nvram_int(_field,
					   iconf->wmm_ac_params[ac_idx].txop_limit, def_txop);
	}
	os_free(_field);

	return 0;
}
#endif /* CONFIG_AP_WMM */

static int wpa_config_write_global(struct wpa_config *config)
{
	da16x_nvram_prt("[%s:%d] %s\n", __func__, __LINE__, "START");

	if (os_strcmp(config->country, DEFAULT_AP_COUNTRY) != 0)
		wpa_config_write_nvram_string(ENV_COUNTRY, config->country);
	else
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_COUNTRY);

	wpa_config_write_nvram_int(ENV_TLS_VER, da16x_tls_version, 0x303);
#ifdef	EAP_PEAP
	wpa_config_write_nvram_int(ENV_PEAP_VER, da16x_peap_version, 0);
#endif	/* EAP_PEAP */

#ifdef CONFIG_WPS
#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
	if (!is_nil_uuid(config->uuid)) {
		char buf[40];
		uuid_bin2str(config->uuid, buf, sizeof(buf));
		wpa_config_write_nvram_string("uuid", buf);
	}

	wpa_config_write_nvram_int("auto_uuid", config->auto_uuid, 0);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

	if (strlen(config->device_name))
	{
		wpa_config_write_nvram_string(ENV_DEVICE_NAME, config->device_name);
	}
	else
	{
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_DEVICE_NAME);
	}

	if (strlen(config->manufacturer))
	{
		wpa_config_write_nvram_string(ENV_MANUFACTURER, config->manufacturer);
	}
	else
	{
	    da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_MANUFACTURER);
	}

	if (strlen(config->model_name))
	{
		wpa_config_write_nvram_string(ENV_MODEL_NAME, config->model_name);
	}
	else
	{
	    da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_MODEL_NAME);
	}

	if (strlen(config->model_number))
	{
		wpa_config_write_nvram_string(ENV_MODEL_NUMBER, config->model_number);
	}
	else
	{
	    da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_MODEL_NUMBER);
	}

	if (strlen(config->serial_number))
	{
		wpa_config_write_nvram_string(ENV_SERIAL_NUMBER, config->serial_number);
	}
	else
	{
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_SERIAL_NUMBER);
	}

	{
		char _buf[WPS_DEV_TYPE_BUFSIZE], *buf;
		buf = wps_dev_type_bin2str(config->device_type,
					   _buf, sizeof(_buf));
		if (os_strcmp(buf, DEFAULT_WPS_DEVICE_TYPE) != 0)
			wpa_config_write_nvram_string(ENV_DEVICE_TYPE, buf);
		else
			da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_DEVICE_TYPE);
	}


	if (strlen(config->config_methods))
	{
		wpa_config_write_nvram_string(ENV_CONFIG_METHODS,
				      config->config_methods);
	}
	else
	{
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", ENV_CONFIG_METHODS);
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
	wpa_config_write_nvram_string(ENV_P2P_SSID_POSTFIX,
				      config->p2p_ssid_postfix);
	wpa_config_write_nvram_int("p2p_group_idle",
				   config->p2p_group_idle, 0);
	wpa_config_write_nvram_int(ENV_P2P_LISTEN_CHANNEL,
			 	   config->p2p_listen_channel,
				   DEFAULT_P2P_LISTEN_CHANNEL);
	wpa_config_write_nvram_int(ENV_P2P_OPER_CHANNEL,
				   config->p2p_oper_channel,
				   DEFAULT_P2P_OPER_CHANNEL);
	wpa_config_write_nvram_int(ENV_P2P_FIND_TIMEOUT,
				   config->p2p_find_timeout,
				   DEFAULT_P2P_FIND_TIMEOUT);
	wpa_config_write_nvram_int(ENV_P2P_GO_INTENT,
				   config->p2p_go_intent,
				   DEFAULT_P2P_GO_INTENT);
#endif /* CONFIG_P2P */

#ifdef CONFIG_AP_WMM
	wpa_config_write_nvram_int(ENV_WMM_ENABLED,
				   config->hostapd_wmm_enabled, 1);
	wpa_config_write_nvram_int(ENV_WMM_PS_ENABLED,
				   config->hostapd_wmm_ps_enabled, 0);
#endif /* CONFIG_AP_WMM */

#ifdef CONFIG_AP
	wpa_config_write_nvram_int(ENV_AP_MAX_INACTIVITY, config->ap_max_inactivity, DEFAULT_AP_MAX_INACTIVITY);
	wpa_config_write_nvram_int(ENV_AP_SEND_KA, config->ap_send_ka, 0);

#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
	wpa_config_write_nvram_int("bss_max_count", config->bss_max_count, DEFAULT_BSS_MAX_COUNT);
#ifndef CONFIG_IMMEDIATE_SCAN
	wpa_config_write_nvram_int("bss_expiration_age", config->bss_expiration_age, DEFAULT_BSS_EXPIRATION_AGE);
	wpa_config_write_nvram_int("bss_expiration_scan_count", config->bss_expiration_scan_count, DEFAULT_BSS_EXPIRATION_SCAN_COUNT);
#endif /* CONFIG_IMMEDIATE_SCAN */

	wpa_config_write_nvram_int("filter_ssids", config->filter_ssids, 0);
#ifdef CONFIG_SCAN_FILTER_RSSI
	wpa_config_write_nvram_int("filter_rssi", config->filter_rssi, 0);
#endif /* CONFIG_SCAN_FILTER_RSSI */
#if 0
	wpa_config_write_nvram_int("max_num_sta", config->max_num_sta, DEFAULT_MAX_NUM_STA);
	wpa_config_write_nvram_int("ap_isolate", config->ap_isolate, DEFAULT_AP_ISOLATE);
#endif /* 0 */

	wpa_config_write_nvram_int("disassoc_low_ack", config->disassoc_low_ack, 0);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#endif /* CONFIG_AP */

#ifdef CONFIG_HS20
	wpa_config_write_nvram_int("hs20", 1, 0);
#endif /* CONFIG_HS20 */

#ifdef CONFIG_INTERWORKING
	wpa_config_write_nvram_int("interworking", config->interworking, 0);
	
	if (!is_zero_ether_addr(config->hessid)) {
		wpa_config_write_nvram_string("hessid", config->hessid);
	}
	
	wpa_config_write_nvram_int("access_network_type", config->access_network_type, DEFAULT_ACCESS_NETWORK_TYPE);
#ifdef CONFIG_P2P
	wpa_config_write_nvram_int("go_interworking", config->go_interworking, 0);
#endif /* CONFIG_P2P */
	wpa_config_write_nvram_int("go_access_network_type", config->go_access_network_type, 0);	
	wpa_config_write_nvram_int("go_internet", config->go_internet, 0);
	wpa_config_write_nvram_int("go_venue_group", config->go_venue_group, 0);
	wpa_config_write_nvram_int("go_venue_type", config->go_venue_type, 0);
#endif /* CONFIG_INTERWORKING */
	wpa_config_write_nvram_int("pbc_in_m1", config->pbc_in_m1, 0);

#ifdef CONFIG_WPS_NFC
	if (config->wps_nfc_pw_from_config) { 
		wpa_config_write_nvram_int("wps_nfc_dev_pw_id", config->wps_nfc_dev_pw_id, 0);
		
#if  0 // NFC ï¿½ï¿½ï¿½ï¿½ ï¿½ß°ï¿½ ï¿½Û¾ï¿½ ï¿½Ê¿ï¿½
	write_global_bin(f, "wps_nfc_dh_pubkey",
			 config->wps_nfc_dh_pubkey);
	write_global_bin(f, "wps_nfc_dh_privkey",
			 config->wps_nfc_dh_privkey);
	write_global_bin(f, "wps_nfc_dev_pw", config->wps_nfc_dev_pw);
#endif /* 0 */
	}
#endif /* CONFIG_WPS_NFC */

#ifdef CONFIG_EXT_PASSWORD
	wpa_config_write_nvram_string("ext_password_backend", config->ext_password_backend);
#endif /* CONFIG_EXT_PASSWORD */
#if 0
	wpa_config_write_nvram_int("p2p_go_max_inactivity", config->p2p_go_max_inactivity, DEFAULT_P2P_GO_MAX_INACTIVITY);
#endif /* 0 */

#ifdef	CONFIG_SUPP27_CONFIG_NVRAM
#ifdef	CONFIG_INTERWORKING
	wpa_config_write_nvram_int("auto_interworking", config->auto_interworking, 0);
#endif	/* CONFIG_INTERWORKING */
	wpa_config_write_nvram_int("okc", config->okc, 0);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#ifdef CONFIG_IEEE80211W
	wpa_config_write_nvram_int("pmf", config->pmf, 0);
#endif /* CONFIG_IEEE80211W */

#if 0	/* wpa_config_write_network ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½. */
	wpa_config_write_nvram_int("dtim_period", config->dtim_period, DTIM_PERIOD_DEFAULT);
	wpa_config_write_nvram_int("beacon_int", config->beacon_int, BEACON_INT_DEFAULT);
#endif /* 0 */

#ifdef CONFIG_SAE
	/* SAE_GROUPS */
	extern const unsigned char support_sae_groups[];
	for (int idx = 0; support_sae_groups[idx] >= 0; idx++) {
		if (config->sae_groups[idx] != support_sae_groups[idx])
		{
			char tmp_sae_groups[3*MAX_SAE_GROUPS];
			memset(tmp_sae_groups, 0, 3*MAX_SAE_GROUPS);
			for (int i = 0; config->sae_groups[i] != 0; i++) {
				sprintf(tmp_sae_groups+strlen(tmp_sae_groups), "%s%d", i > 0 ? " ":"", config->sae_groups[i]);
			}
			wpa_config_write_nvram_string("sae_groups", tmp_sae_groups);
			break;
		}
		else if (support_sae_groups[idx] == 0)
		{
			wpa_config_write_nvram_string("sae_groups", "");
			break;
		}
	}
#endif /* CONFIG_SAE */
	
#if 0 // ï¿½ß°ï¿½ ï¿½Û¾ï¿½ ï¿½Ê¿ï¿½
	if (config->ap_vendor_elements) {
		int i, len = wpabuf_len(config->ap_vendor_elements);
		const u8 *p = wpabuf_head_u8(config->ap_vendor_elements);

		if (len > 0) {
			fprintf(f, "ap_vendor_elements=");
			for (i = 0; i < len; i++)
				fprintf(f, "%02x", *p++);
			fprintf(f, "\n");
		}
	}
#endif /* 0 */

#ifdef CONFIG_IGNOR_OLD_SCAN
		wpa_config_write_nvram_int("ignore_old_scan_res", config->ignore_old_scan_res, 0);
#endif /* CONFIG_IGNOR_OLD_SCAN */

#if 0 // ï¿½ß°ï¿½ ï¿½Û¾ï¿½ ï¿½Ê¿ï¿½
	if (config->freq_list && config->freq_list[0]) {
		int i;
		fprintf(f, "freq_list=");
		for (i = 0; config->freq_list[i]; i++) {
			fprintf(f, "%s%d", i > 0 ? " " : "",
				config->freq_list[i]);
		}
		fprintf(f, "\n");
	}
#endif /* 0 */

#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	wpa_config_write_nvram_int("scan_cur_freq", config->scan_cur_freq, DEFAULT_SCAN_CUR_FREQ);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#ifdef CONFIG_SCHED_SCAN
	wpa_config_write_nvram_int("sched_scan_interval", config->sched_scan_interval, 0);
	wpa_config_write_nvram_int("sched_scan_start_delay", config->sched_scan_start_delay, 0);
#endif /* CONFIG_SCHED_SCAN */

#ifdef	CONFIG_EXT_SIM
	wpa_config_write_nvram_int(ENV_EXTERNAL_SIM, config->external_sim, 0);
#endif	/* CONFIG_EXT_SIM */

#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	wpa_config_write_nvram_int("tdls_external_control", config->tdls_external_control, 0);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#ifdef CONFIG_WOWLAN
	wpa_config_write_nvram_string("wowlan_triggers", config->wowlan_triggers);
#endif /* CONFIG_WOWLAN */

#ifdef CONFIG_BGSCAN
	wpa_config_write_nvram_string("bgscan", config->bgscan);
#endif /* CONFIG_BGSCAN */
#ifdef CONFIG_INTERWORKING
	wpa_config_write_nvram_string("autoscan", config->autoscan);
#endif /* CONFIG_INTERWORKING */
#ifdef CONFIG_P2P
	wpa_config_write_nvram_int("p2p_search_delay", config->p2p_search_delay, DEFAULT_P2P_SEARCH_DELAY);
#endif /* CONFIG_P2P */

#ifdef CONFIG_INTERWORKING
	wpa_config_write_nvram_int("mac_addr", config->mac_addr, 0);
#endif /* CONFIG_INTERWORKING */
#ifdef CONFIG_RANDOM_ADDR
	wpa_config_write_nvram_int("rand_addr_lifetime", config->rand_addr_lifetime, DEFAULT_RAND_ADDR_LIFETIME);
	wpa_config_write_nvram_int("preassoc_mac_addr", config->preassoc_mac_addr, 0);
#endif /* CONFIG_RANDOM_ADDR */
	wpa_config_write_nvram_int("key_mgmt_offload", config->key_mgmt_offload, DEFAULT_KEY_MGMT_OFFLOAD);
#ifdef CONFIG_MESH
	wpa_config_write_nvram_int("user_mpm", config->user_mpm, DEFAULT_USER_MPM);
	wpa_config_write_nvram_int("max_peer_links", config->max_peer_links, DEFAULT_MAX_PEER_LINKS);
#endif /* CONFIG_MESH */
	wpa_config_write_nvram_int("cert_in_cb", config->cert_in_cb, DEFAULT_CERT_IN_CB);
#ifdef CONFIG_MESH
	wpa_config_write_nvram_int("mesh_max_inactivity", config->mesh_max_inactivity, DEFAULT_MESH_MAX_INACTIVITY);
#endif /* CONFIG_MESH */
#ifdef CONFIG_SAE
	wpa_config_write_nvram_int("dot11RSNASAERetransPeriod", config->dot11RSNASAERetransPeriod, DEFAULT_DOT11_RSNA_SAE_RETRANS_PERIOD);
#endif /* CONFIG_SAE */
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	wpa_config_write_nvram_int("passive_scan", config->passive_scan, 0);
	wpa_config_write_nvram_int("reassoc_same_bss_optim", config->reassoc_same_bss_optim, 0);
	wpa_config_write_nvram_int("wps_priority", config->wps_priority, 0);
	wpa_config_write_nvram_int("wpa_rsc_relaxation", config->wpa_rsc_relaxation, DEFAULT_WPA_RSC_RELAXATION);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#ifdef CONFIG_SCHED_SCAN
	wpa_config_write_nvram_string("sched_scan_plans", config->sched_scan_plans);
#endif /* CONFIG_SCHED_SCAN */

#ifdef CONFIG_MBO
	wpa_config_write_nvram_string("non_pref_chan", config->non_pref_chan);
	wpa_config_write_nvram_int("mbo_cell_capa", config->mbo_cell_capa, DEFAULT_MBO_CELL_CAPA);
	wpa_config_write_nvram_int("disassoc_imminent_rssi_threshold", config->disassoc_imminent_rssi_threshold, DEFAULT_DISASSOC_IMMINENT_RSSI_THRESHOLD);
	wpa_config_write_nvram_int("oce", config->oce, DEFAULT_OCE_SUPPORT);
#endif /* CONFIG_MBO */
#ifdef CONFIG_GAS
	wpa_config_write_nvram_int("gas_address3", config->gas_address3, 0);
#endif /* CONFIG_GAS */
	wpa_config_write_nvram_int("ftm_responder", config->ftm_responder, 0);
	wpa_config_write_nvram_int("ftm_initiator", config->ftm_initiator, 0);
	wpa_config_write_nvram_string("osu_dir", config->osu_dir);
#ifdef CONFIG_FST
	wpa_config_write_nvram_string("fst_group_id", config->fst_group_id);
	wpa_config_write_nvram_int("fst_priority", config->fst_priority,  0);
	wpa_config_write_nvram_int("fst_llt", config->fst_llt, 0);
#endif /* CONFIG_FST */
#ifdef CONFIG_GAS
	wpa_config_write_nvram_int("gas_rand_addr_lifetime", config->gas_rand_addr_lifetime, DEFAULT_RAND_ADDR_LIFETIME);
	wpa_config_write_nvram_int("gas_rand_mac_addr", config->gas_rand_mac_addr, 0);
#endif /* CONFIG_GAS */
#ifdef CONFIG_DPP
	wpa_config_write_nvram_int("dpp_config_processing", config->dpp_config_processing, 0);
#endif /* CONFIG_DPP */

#ifdef CONFIG_ACL
	wpa_config_write_ap_acl_mac(config);
#endif /* CONFIG_ACL */

#ifdef	CONFIG_SIMPLE_ROAMING
	if (config->roam_state >= 1) {
		wpa_config_write_nvram_int(ENV_ROAM, 1, DEFAULT_ROAM_DISABLE);
	} else {
		wpa_config_write_nvram_int(ENV_ROAM, 0, DEFAULT_ROAM_DISABLE);
	}
	wpa_config_write_nvram_int(ENV_ROAM_THRESHOLD,
				   config->roam_threshold, ROAM_RSSI_THRESHOLD);
#endif	/* CONFIG_SIMPLE_ROAMING */

#ifdef	CONFIG_AP_PARAMETERS
	//da16x_info_prt("greenfield=%d\n", config->greenfield);
	wpa_config_write_nvram_int(ENV_AP_SET_GREENFIELD,
				config->greenfield, DEFAULT_GREENFIELD);

	wpa_config_write_nvram_int(ENV_AP_SET_HT_PROTECTION,
				config->ht_protection, DEFAULT_HT_PROTECTION);

	wpa_config_write_nvram_int(ENV_RTS_THRESHOLD,
				config->rts_threshold, DEFAULT_RTS_THRESHOLD);

#endif /* CONFIG_AP_PARAMETERS */

#ifdef CONFIG_LOG_MASK
	wpa_config_write_nvram_int(ENV_SUPP_LOG_MASK, get_log_mask(),
				   SUPP_LOG_DEFAULT);
#endif /* CONFIG_LOG_MASK */

    wpa_config_write_nvram_int(ENV_PMK_LIFE_TIME, config->dot11RSNAConfigPMKLifetime, DEFAULT_PMK_LIFE_TIME);

#ifdef CONFIG_5G_SUPPORT
	wpa_config_write_nvram_int(ENV_BAND,
				   config->band, WPA_SETBAND_AUTO);
#endif /* CONFIG_5G_SUPPORT */

	da16x_nvram_prt("[%s] %s\n", __func__, "FINISH");

	return 0;
}

void write_str(const char *field, struct wpa_ssid *ssid)
{
	char *_field;
	char *value = wpa_config_get(ssid, field);

	da16x_nvram_prt("    [%s] field=%s\n", __func__, field);

	if (value == NULL)
		return;

	/* ENV_NAME */
	_field = os_malloc(32+3);
	if (_field == NULL) {
		os_free(value);
		return;
	}

	//_field = os_malloc(7);  //20150710_trinity
	memset(_field, 0, 32+3);
	//memset(_field, 0, 8);

	/* N0_xxxxx ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ field name ï¿½ï¿½ï¿½ï¿½ */
	sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, field);
	wpa_config_write_nvram_string(_field, value);

	os_free(_field);
	os_free(value);
}


static void write_int(int id, const char *field, int value, int def)
{
	char *_field;

	da16x_nvram_prt("    [%s] id=%d field=%s value=%d def=%d\n",
			__func__, id, field, value, def);

	_field = os_malloc(32+3);
	if (_field == NULL) {
		return;
	}

	memset(_field, 0, 32+3);

	/* N0_xxxxx ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ field name ï¿½ï¿½ï¿½ï¿½ */
	sprintf(_field, PREFIX_NETWORK_PROFILE, id, field);
	wpa_config_write_nvram_int(_field, value, def);
	os_free(_field);
}

static void write_bssid(struct wpa_ssid *ssid)
{
	char *_field;
	char *value = wpa_config_get(ssid, ENV_BSSID);

	if (value == NULL)
		return;

	//_field = os_malloc(strlen(value));
	_field = os_malloc(9); //20150710_trinity
	if (_field == NULL) {
		os_free(value);
		return;
	}

	memset(_field, 0, 9);

	sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_BSSID);
	wpa_config_write_nvram_string(_field, value);
	os_free(_field);
	os_free(value);
}

#ifdef CONFIG_P2P_UNUSED_CMD
static void write_bssid_hint(struct wpa_ssid *ssid)
{
	char *_field;
	char *value = wpa_config_get(ssid, ENV_BSSID_HINT);

	if (value == NULL)
		return;

	_field = os_malloc(9);
	if (_field == NULL) {
		os_free(value);
		return;
	}

	memset(_field, 0, 9);

	sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_BSSID_HINT);
	wpa_config_write_nvram_string(_field, value);
	os_free(_field);
	os_free(value);
}
#endif	/* CONFIG_P2P_UNUSED_CMD */

static void write_psk(struct wpa_ssid *ssid)
{
	char *_field;
	char *value = wpa_config_get(ssid, ENV_PSK);

	if (value == NULL)
		return;

#ifdef CONFIG_MESH
	if (ssid->mode == WPAS_MODE_MESH) {
		return;
	}
#endif /* CONFIG_MESH */

#ifdef CONFIG_SAE
	if ((ssid->key_mgmt & WPA_KEY_MGMT_SAE) && 
	    !(ssid->key_mgmt & WPA_KEY_MGMT_PSK)) {
		return;
	}
#endif /* CONFIG_SAE */

	//_field = os_malloc(strlen(value));
	_field = os_malloc(7); //20150710_trinity
	if (_field == NULL) {
		os_free(value);
		return;
	}

	memset(_field, 0, 7);
	sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_PSK);
	wpa_config_write_nvram_string(_field, value);
	os_free(_field);
	os_free(value);
}


static void write_proto(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	if (ssid->proto == DEFAULT_PROTO)
		return;

	value = wpa_config_get(ssid, ENV_PROTO);
	if (value == NULL)
		return;

	if (value[0]) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(9); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 9);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_PROTO);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}


static void write_key_mgmt(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	if (ssid->key_mgmt == DEFAULT_KEY_MGMT)
		return;

	value = wpa_config_get(ssid, ENV_KEY_MGMT);

	if (value == NULL)
		return;

	if (value[0]) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(12); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 12);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_KEY_MGMT);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}


static void write_pairwise(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	if (ssid->pairwise_cipher == DEFAULT_PAIRWISE)
		return;

	value = wpa_config_get(ssid, ENV_PAIRWISE);

	if (value == NULL)
		return;

	if (value[0]) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(12); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 12);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_PAIRWISE);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}


static void write_group(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;


	if (ssid->group_cipher == DEFAULT_GROUP)
		return;

	value = wpa_config_get(ssid, ENV_GROUP);

	if (value == NULL)
		return;

	if (value[0]) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(9); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 9);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_GROUP);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}


#ifdef CONFIG_SUPP27_CONFIG_NVRAM
static void write_group_mgmt(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	if (!ssid->group_mgmt_cipher)
		return;

	value = wpa_config_get(ssid, ENV_GROUP_MGMT);

	if (value == NULL)
		return;

	if (value[0]) {

		_field = os_malloc(3+10);
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 3+10);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_GROUP_MGMT);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}


static void write_auth_alg(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	if (ssid->auth_alg == 0)
		return;

	value = wpa_config_get(ssid, ENV_AUTH_ALG);
	if (value == NULL)
		return;

	if (value[0]) {
		_field = os_malloc(3+9);
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 3+9);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_AUTH_ALG);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */


#ifdef IEEE8021X_EAPOL
static void write_eap(struct wpa_ssid *ssid)
{
	char *_field;
	char *value;

	value = wpa_config_get(ssid, ENV_EAP);

	if (value == NULL)
		return;

	if (value[0]) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(7); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 7);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, ENV_EAP);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
	}
	os_free(value);
}
#endif /* IEEE8021X_EAPOL */


static void write_wep_key(int idx, struct wpa_ssid *ssid)
{
	char *_field;
	char field[20], *value;

	os_snprintf(field, sizeof(field), ENV_WEP_KEY_X, idx);
	value = wpa_config_get(ssid, field);
	if (value) {
		//_field = os_malloc(strlen(value));
		_field = os_malloc(12); //20150710_trinity
		if (_field == NULL) {
			os_free(value);
			return;
		}
		memset(_field, 0, 12);
		sprintf(_field, PREFIX_NETWORK_PROFILE, ssid->id, field);
		wpa_config_write_nvram_string(_field, value);
		os_free(_field);
		os_free(value);
	}
}

static void reset_network_id_field(int id)
{
	char _field[5] = {NULL,};
	sprintf(_field, PREFIX_NETWORK_PROFILE, id, "*");
	da16x_unsetenv_temp(ENVIRON_DEVICE, "app", _field);
}

static int wpa_config_write_network(struct wpa_ssid *ssid)
{
	int i, errors = 0;
	char profile[12];

#ifdef CONFIG_P2P
	if (ssid->id == FIXED_NETWORK_ID_P2P && !ssid->p2p_force)
		return 0; /* do not save p2p networks */
#endif	/* CONFIG_P2P */
	reset_network_id_field(ssid->id); //Reset before write

	/* Check  Network Profile */
	memset(profile, 0, 12);
	sprintf(profile, PREFIX_NETWORK_PROFILE, ssid->id, "Profile"); /* Nx_Profile */

	if (da16x_getenv(ENVIRON_DEVICE, "app", profile) == NULL)
		da16x_setenv_temp(ENVIRON_DEVICE, "app", profile, "1");

#define STR(t) write_str(#t, ssid)
#define INT(t) write_int(ssid->id, #t, ssid->t, 0)
#define INTe(t) write_int(ssid->id, #t, ssid->eap.t, 0)
#define INT_DEF(t, def) write_int(ssid->id, #t, ssid->t, def)
#define INT_DEFe(t, def) write_int(ssid->id, #t, ssid->eap.t, def)

	STR(ssid);
	INT(scan_ssid);
	write_bssid(ssid);
#ifdef CONFIG_P2P_UNUSED_CMD
	write_bssid_hint(ssid);
	write_str("bssid_blacklist", ssid);
	write_str("bssid_whitelist", ssid);
#endif	/* CONFIG_P2P_UNUSED_CMD */
	write_psk(ssid);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	INT(mem_only_psk);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#ifdef CONFIG_SAE
	STR(sae_password);
	STR(sae_password_id);
#endif /* CONFIG_SAE */
	write_proto(ssid);
	write_key_mgmt(ssid);
#ifdef CONFIG_BGSCAN
	INT_DEF(bg_scan_period, DEFAULT_BG_SCAN_PERIOD);
#endif /* CONFIG_BGSCAN */
	write_pairwise(ssid);
	write_group(ssid);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	write_group_mgmt(ssid);
	write_auth_alg(ssid);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
	STR(bgscan);
	STR(autoscan);
	STR(scan_freq);
#ifdef IEEE8021X_EAPOL
	write_eap(ssid);
	STR(identity);
	STR(anonymous_identity);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	STR(imsi_identity);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
	STR(password);
	STR(ca_cert);
	STR(ca_path);
	STR(client_cert);
	STR(private_key);
	STR(private_key_passwd);

#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	STR(dh_file);
	STR(subject_match);
	STR(altsubject_match);
	STR(domain_suffix_match);
	STR(domain_match);
	STR(ca_cert2);
	STR(ca_path2);
	STR(client_cert2);
	STR(private_key2);
	STR(private_key2_passwd);
	STR(dh_file2);
	STR(subject_match2);
	STR(altsubject_match2);
	STR(domain_suffix_match2);
	STR(domain_match2);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
	STR(phase1);
	STR(phase2);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	STR(pcsc);
	STR(pin);
	STR(engine_id);
	STR(key_id);
	STR(cert_id);
	STR(ca_cert_id);
	STR(key2_id);
	STR(pin2);
	STR(engine2_id);
	STR(cert2_id);
	STR(ca_cert2_id);
	INTe(engine);
	INTe(engine2);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
	INT_DEF(eapol_flags, DEFAULT_EAPOL_FLAGS);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	STR(openssl_ciphers);
	INTe(erp);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#endif /* IEEE8021X_EAPOL */

	for (i = 0; i < 4; i++)
		write_wep_key(i, ssid);
	INT(wep_tx_keyidx);
#ifdef CONFIG_PRIO_GROUP
	INT(priority);
#endif /* CONFIG_PRIO_GROUP */
#ifdef IEEE8021X_EAPOL
	INT_DEF(eap_workaround, (int)DEFAULT_EAP_WORKAROUND);
#ifdef  CONFIG_FAST_FILE
	STR(pac_file);
#endif  /* CONFIG_FAST_FILE */
	INT_DEFe(fragment_size, DEFAULT_FRAGMENT_SIZE);
#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	INTe(ocsp);
	INT_DEFe(sim_num, DEFAULT_USER_SELECTED_SIM);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */
#endif /* IEEE8021X_EAPOL */

	if(ssid->id == FIXED_NETWORK_ID_AP)
		ssid->mode = WPAS_MODE_AP;

	INT(mode);

#ifdef CONFIG_MESH
	INT(no_auto_peer);	/* for MESH */
#endif /* CONFIG_MESH */
	if (ssid->acs == 1)
	{
		ssid->frequency = 0;
	}

	INT_DEF(frequency, FREQUENCE_DEFAULT);
#ifdef CONFIG_MESH
	INT(fixed_freq);	 /* for MESH */
#endif /* CONFIG_MESH */

#ifdef CONFIG_ACS
#if 0	/* by Bright */
	INT(acs);
#endif	/* 0 */
#endif /* CONFIG_ACS */

#if 0	/* by Bright */
	INT_DEF(proactive_key_caching, 0);
#else
	write_int(ssid->id, ENV_PROACTIVE_KEY_CACHING, ssid->proactive_key_caching, -1);
#endif	/* 0 */
	INT(disabled);
	INT(pbss);
	INT(wps_disabled);
#ifdef CONFIG_FILS
	INT(fils_dh_group);
#endif /* CONFIG_FILS */
#ifdef CONFIG_IEEE80211W
	write_int(ssid->id, ENV_IEEE80211W, ssid->ieee80211w, MGMT_FRAME_PROTECTION_DEFAULT);
#endif /* CONFIG_IEEE80211W */
	STR(id_str);

	INT(ignore_broadcast_ssid);

#if 0	/* by Bright */
	INT_DEF(dtim_period, DTIM_PERIOD_DEFAULT);
	INT_DEF(beacon_int, BEACON_INT_DEFAULT);
#else
	write_int(ssid->id, ENV_DTIM_PERIOD, ssid->dtim_period, DTIM_PERIOD_DEFAULT);
	write_int(ssid->id, ENV_BEACON_INT, ssid->beacon_int, BEACON_INT_DEFAULT);
#endif	/* 0 */

#ifdef CONFIG_AP_ISOLATION
#if 0	/* by Bright */
	INT_DEF(isolate, ISOLATE_DEFAULT);
#else
	write_int(ssid->id, ENV_ISOLATE, ssid->isolate, ISOLATE_DEFAULT);
#endif	/* 0 */
#endif /* CONFIG_AP_ISOLATION */

#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	//INT_DEF(ap_max_inactivity, AP_MAX_INACTIVITY);
	INT(ap_max_inactivity);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#ifdef CONFIG_AUTH_MAX_FAILURES
	INT(auth_max_failures);
#endif // CONFIG_AUTH_MAX_FAILURES

#ifdef CONFIG_AP_POWER
	STR(ap_power);
#endif /* CONFIG_AP_POWER */

#ifdef CONFIG_MACSEC
	INT(macsec_policy);
	write_mka_cak(f, ssid);
	write_mka_ckn(f, ssid);
	INT(macsec_integ_only);
	INT(macsec_port);
	INT_DEF(mka_priority, DEFAULT_PRIO_NOT_KEY_SERVER);
#endif /* CONFIG_MACSEC */
#ifdef CONFIG_HS20
	INT(update_identifier);
	STR(roaming_consortium_selection);
#endif /* CONFIG_HS20 */
#ifdef CONFIG_RANDOM_ADDR
	write_int(ssid->id, "mac_addr", ssid->mac_addr, -1);
#endif /* CONFIG_RANDOM_ADDR */
#ifdef CONFIG_MESH
	STR(mesh_basic_rates);
	INT_DEF(dot11MeshMaxRetries, DEFAULT_MESH_MAX_RETRIES);
	INT_DEF(dot11MeshRetryTimeout, DEFAULT_MESH_RETRY_TIMEOUT);
	INT_DEF(dot11MeshConfirmTimeout, DEFAULT_MESH_CONFIRM_TIMEOUT);
	INT_DEF(dot11MeshHoldingTimeout, DEFAULT_MESH_HOLDING_TIMEOUT);
	INT_DEF(mesh_rssi_threshold, DEFAULT_MESH_RSSI_THRESHOLD);
#endif /* CONFIG_MESH */
	INT(wpa_ptk_rekey);
	INT(group_rekey);

#ifdef CONFIG_DPP
	STR(dpp_connector);
	STR(dpp_netaccesskey);
	INT(dpp_netaccesskey_expiry);
	STR(dpp_csign);
#endif /* CONFIG_DPP */
#ifdef CONFIG_OWE
	INT(owe_group);
	INT(owe_only);
#endif /* CONFIG_OWE */

#ifdef CONFIG_AP_WIFI_MODE
	INT(wifi_mode);
#endif /* CONFIG_AP_WIFI_MODE */

#ifdef CONFIG_SUPP27_CONFIG_NVRAM
	INT(mixed_cell);
#endif	/* CONFIG_SUPP27_CONFIG_NVRAM */

#ifdef CONFIG_IEEE80211AC
	INT(vht);
#endif /* CONFIG_IEEE80211AC */
#ifdef CONFIG_HT_OVERRIDES
 	INT(disable_ht);
#if 0 //def CONFIG_HT_OVERRIDES
	INT_DEF(ht, 1);
#endif /* CONFIG_HT_OVERRIDES */
#endif /* CONFIG_HT_OVERRIDES */
	INT(ht40);
#ifdef CONFIG_IEEE80211AC
	INT(max_oper_chwidth);
	INT(vht_center_freq1);
	INT(vht_center_freq2);
#endif /* CONFIG_IEEE80211AC */

#ifdef CONFIG_HT_OVERRIDES
	INT_DEF(disable_ht, DEFAULT_DISABLE_HT);
	INT_DEF(disable_ht40, DEFAULT_DISABLE_HT40);
	INT_DEF(disable_sgi, DEFAULT_DISABLE_SGI);
	INT_DEF(disable_ldpc, DEFAULT_DISABLE_LDPC);
	INT(ht40_intolerant);
	INT_DEF(disable_max_amsdu, DEFAULT_DISABLE_MAX_AMSDU);
	INT_DEF(ampdu_factor, DEFAULT_AMPDU_FACTOR);
	INT_DEF(ampdu_density, DEFAULT_AMPDU_DENSITY);
	STR(ht_mcs);
#endif /* CONFIG_HT_OVERRIDES */
#ifdef CONFIG_VHT_OVERRIDES
	INT(disable_vht);
	INT(vht_capa);
	INT(vht_capa_mask);
	INT_DEF(vht_rx_mcs_nss_1, -1);
	INT_DEF(vht_rx_mcs_nss_2, -1);
	INT_DEF(vht_rx_mcs_nss_3, -1);
	INT_DEF(vht_rx_mcs_nss_4, -1);
	INT_DEF(vht_rx_mcs_nss_5, -1);
	INT_DEF(vht_rx_mcs_nss_6, -1);
	INT_DEF(vht_rx_mcs_nss_7, -1);
	INT_DEF(vht_rx_mcs_nss_8, -1);
	INT_DEF(vht_tx_mcs_nss_1, -1);
	INT_DEF(vht_tx_mcs_nss_2, -1);
	INT_DEF(vht_tx_mcs_nss_3, -1);
	INT_DEF(vht_tx_mcs_nss_4, -1);
	INT_DEF(vht_tx_mcs_nss_5, -1);
	INT_DEF(vht_tx_mcs_nss_6, -1);
	INT_DEF(vht_tx_mcs_nss_7, -1);
	INT_DEF(vht_tx_mcs_nss_8, -1);
#endif /* CONFIG_VHT_OVERRIDES */

#undef STR
#undef INT
#undef INT_DEF

	return errors ? -1 : 0;
}


#define PROFILE_SAVE_FLAG	1
#define PROFILE_ERASE_FLAG	0

#if defined ( __SUPP_27_SUPPORT__ )
int wpa_config_write(char *confname, struct wpa_config *config, char *ifname)
#else
int wpa_config_write(struct wpa_config *config, char *ifname)
#endif // __SUPP_27_SUPPORT__
{
	int errors = 0;
	struct wpa_ssid *ssid;
	int clear_id_flag[MAX_WPA_PROFILE];

#ifdef	CONFIG_CONCURRENT /* by Shingu 20160902 (Concurrent) */
	int mode_id = 0;	// 0: STA, 1: Soft-AP, 2: P2P, 3: MESH Point
#endif	/* CONFIG_CONCURRENT */

	TX_FUNC_START("");

	for (int id = 0; id < MAX_WPA_PROFILE; id++) {
		clear_id_flag[id] = PROFILE_ERASE_FLAG;
	}

#ifdef	CONFIG_CONCURRENT /* by Shingu 20160902 (Concurrent) */
	if (os_strcmp(ifname, STA_DEVICE_NAME) == 0)
		mode_id = FIXED_NETWORK_ID_STA;
	else if (os_strcmp(ifname, SOFTAP_DEVICE_NAME) == 0)
		mode_id = FIXED_NETWORK_ID_AP;
	else if (os_strcmp(ifname, P2P_DEVICE_NAME) == 0)
		mode_id = FIXED_NETWORK_ID_P2P;
#ifdef __SUPPORT_MESH__
	else if (os_strcmp(ifname, MESH_POINT_DEVICE_NAME) == 0)
		mode_id = FIXED_NETWORK_ID_MESH_POINT;
#endif /* __SUPPORT_MESH__ */
#endif	/* CONFIG_CONCURRENT */

	da16x_nvram_prt("[%s] Writing configuration NVRAM\n", __func__);

#ifdef	CONFIG_CONCURRENT /* by Shingu 20160907 (Concurrent) */
	if (    mode_id == FIXED_NETWORK_ID_STA
		&& (   get_run_mode() == SYSMODE_STA_N_AP
#ifdef	CONFIG_P2P
			|| get_run_mode() == SYSMODE_STA_N_P2P
#endif	/* CONFIG_P2P */
#ifdef __SUPPORT_MESH__
			|| get_run_mode() == SYSMODE_STA_N_MESH_PORTAL
#endif /* __SUPPORT_MESH__ */
	    	)) {
		da16x_nvram_prt("[%s] Skip writing sta0 configuration to NVRAM "
			       "on Concurrent mode\n", __func__);
	} else
#endif	/* CONFIG_CONCURRENT */
	{
		if (wpa_config_write_global(config)) {
			da16x_nvram_prt("[%s] Failed to write global "
					"configuration data\n", __func__);
			errors++;
		}
	}

	for (ssid = config->ssid; ssid; ssid = ssid->next) {
		da16x_nvram_prt("[%s] ssid->id=%d\n", __func__, ssid->id);

		clear_id_flag[ssid->id] = PROFILE_SAVE_FLAG; // undelete profile flag

		if (ssid->key_mgmt == WPA_KEY_MGMT_WPS)
			continue; /* do not save temporary WPS networks */

#ifdef	CONFIG_AP_WIFI_MODE
		if (ssid->wifi_mode == DA16X_WIFI_MODE_BG ||
			ssid->wifi_mode == DA16X_WIFI_MODE_G_ONLY ||
			ssid->wifi_mode == DA16X_WIFI_MODE_B_ONLY) {
			ssid->disable_ht = 1;
		} else {
			ssid->disable_ht = 0;
		}
#endif	/* CONFIG_AP_WIFI_MODE */

		if (wpa_config_write_network(ssid))
			errors++;
	}

	/* Erase Profile */
	for (int id = 0; id < MAX_WPA_PROFILE; id++) {
#ifdef	CONFIG_CONCURRENT /* by Shingu 20160902 (Concurrent) */
		if (   get_run_mode() == SYSMODE_STA_N_AP
#ifdef	CONFIG_P2P
		    || get_run_mode() == SYSMODE_STA_N_P2P
#endif	/* CONFIG_P2P */
#ifdef __SUPPORT_MESH__
		    || get_run_mode() == SYSMODE_STA_N_MESH_PORTAL
#endif /* __SUPPORT_MESH__ */
			) {
			if (id != mode_id)
				continue;
		}
#endif	/* CONFIG_CONCURRENT */

		if (clear_id_flag[id] == PROFILE_ERASE_FLAG) {
			char id_str[5];
			memset(id_str, 0, 5);
			sprintf(id_str, "N%d_*", id);
			da16x_unsetenv_temp(ENVIRON_DEVICE, "app", id_str);

			da16x_nvram_prt("[%s] Clear Network Profile %d\n",
				       __func__, id);
		}
	}

	da16x_nvram_prt("[%s] Configuration NVRAM cache written %ssuccessfully\n",
		       __func__, errors ? "un" : "");

	da16x_saveenv(ENVIRON_DEVICE); /* Save env */

	return errors ? -1 : 0;
}

int hostapd_iconf_write(struct hostapd_config *iconf)
{
	int errors = 0;
	TX_FUNC_START("");

	da16x_nvram_prt("[%s] Writing hostapd(softAP) configuration NVRAM\n", __func__);
#ifdef CONFIG_AP_WMM
	if (write_wmm_params_ap(iconf)) {
		errors++;
	}

	if (write_wmm_params_sta(iconf)) {
		errors++;
	}
#endif /* CONFIG_AP_WMM */

	if (errors) {
		da16x_nvram_prt("[%s] Failed to write hostapd(softAP) "
				"configuration data\n", __func__);
		return -1;
	}

	da16x_nvram_prt("[%s] Hostapd(softAP) Configuration NVRAM written successfully\n",
		   __func__);
	return 0;
}

int wpa_supp_dpm_restore_config(struct wpa_config *config)
{
	struct wpa_ssid *ssid;
	int id;
#ifdef CONFIG_STA_COUNTRY_CODE
	const char *country;
#endif /* CONFIG_STA_COUNTRY_CODE */

	ssid = config->ssid;

	if (ssid == NULL) {
		if (get_run_mode() == SYSMODE_STA_ONLY) {
			id = 0;
#ifdef FEATURE_UMAC_MESH
		} else if (get_run_mode() == SYSMODE_P2P_ONLY) {
			id = 2;
#endif /* FEATURE_UMAC_MESH */
		} else {
			da16x_err_prt("[DPM] %s : DPM mode not supported "
					"in this mode\n", __func__);
			return -1;
		}

		ssid = os_zalloc(sizeof(*ssid));
		if (ssid == NULL) {
			da16x_mem_prt("[DPM] %s: mem alloc fail...\n", __func__);
			return NULL;
		}

		dl_list_init(&ssid->psk_list);
		wpa_config_set_network_defaults(ssid);
#ifdef CONFIG_STA_COUNTRY_CODE
		country = dpm_restore_country_code();
		if(country != NULL) {
			memset(config->country, NULL, 3);
			memcpy(config->country, country , 3);
			country_to_freq_range_list(&config->country_range, config->country);
		} else {
			strcpy(config->country, "US");
		}
#endif /* CONFIG_STA_COUNTRY_CODE */

		if (dpm_supp_conn_info->id == id) {
			/* connection data */
			ssid->mode = (enum wpas_mode)dpm_supp_conn_info->mode;
			ssid->disabled = dpm_supp_conn_info->disabled;

#ifdef DEF_SAVE_DPM_WIFI_MODE
			/* wifi mode addition, 180528 */
			ssid->wifi_mode = dpm_supp_conn_info->wifi_mode;
#ifdef  CONFIG_DPM_OPT_WIFI_MODE
			ssid->dpm_opt_wifi_mode = dpm_supp_conn_info->dpm_opt;
#endif  /* CONFIG_DPM_OPT_WIFI_MODE */
			/** In DPM Wakeup, If WiFi mode is B/G , let's set the disable HT **/
			if (ssid->wifi_mode == DA16X_WIFI_MODE_BG) {
				ssid->disable_ht = 1;
				ssid->ht = 0;
			}
#endif			
			ssid->id = dpm_supp_conn_info->id;
			ssid->ssid_len = dpm_supp_conn_info->ssid_len;
			ssid->scan_ssid = dpm_supp_conn_info->scan_ssid;
			ssid->psk_set = dpm_supp_conn_info->psk_set;
			ssid->auth_alg = dpm_supp_conn_info->auth_alg;

			os_memcpy(ssid->bssid, dpm_supp_conn_info->bssid, ETH_ALEN);

			ssid->ssid = os_calloc(32, sizeof(char));
			if(ssid->ssid == NULL) {
				da16x_err_prt("[DPM] %s :Failed to restore ssid.\n", __func__);
				return -1;
			}
			
			os_memcpy(ssid->ssid, dpm_supp_conn_info->ssid, dpm_supp_conn_info->ssid_len);

			/* key data */
			ssid->proto = dpm_supp_key_info->proto;
			ssid->key_mgmt = dpm_supp_key_info->key_mgmt;

			if (dpm_supp_key_info->wep_key_len > 0)
			{
				ssid->wep_key_len[dpm_supp_key_info->wep_tx_keyidx] = dpm_supp_key_info->wep_key_len;
				ssid->wep_tx_keyidx = dpm_supp_key_info->wep_tx_keyidx;

				os_memcpy(ssid->wep_key[dpm_supp_key_info->wep_tx_keyidx],
				dpm_supp_key_info->wep_key, MAX_WEP_KEY_LEN);
			}
			else
			{
				os_memcpy(ssid->psk, dpm_supp_conn_info->psk, 32);
			}

			ssid->pairwise_cipher = dpm_supp_key_info->pairwise_cipher;
			ssid->group_cipher = dpm_supp_key_info->group_cipher;
		}

		config->ssid = ssid;
	}

	da16x_dpm_prt("[DPM] Done restore Config Information\n");

	return 0;
}

/* EOF */
