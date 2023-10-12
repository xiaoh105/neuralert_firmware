/*
 * hostapd / Configuration definitions and helpers functions
 * Copyright (c) 2003-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HOSTAPD_CONFIG_H
#define HOSTAPD_CONFIG_H

#include "sdk_type.h"

#include "common/defs.h"
#include "utils/list.h"
#include "ip_addr.h"
#include "common/wpa_common.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "wps/wps.h"
#ifdef CONFIG_IEEE80211R
#include "fst/fst.h"
#endif /* CONFIG_IEEE80211R */
#ifndef CONFIG_NO_VLAN
#include "vlan.h"
#endif /* CONFIG_NO_VLAN */

#ifdef CONFIG_MESH
/**
 * mesh_conf - local MBSS state and settings
 */
struct mesh_conf {
	u8 meshid[32];
	u8 meshid_len;
	/* Active Path Selection Protocol Identifier */
	u8 mesh_pp_id;
	/* Active Path Selection Metric Identifier */
	u8 mesh_pm_id;
	/* Congestion Control Mode Identifier */
	u8 mesh_cc_id;
	/* Synchronization Protocol Identifier */
	u8 mesh_sp_id;
	/* Authentication Protocol Identifier */
	u8 mesh_auth_id;
	u8 *rsn_ie;
	int rsn_ie_len;
#define MESH_CONF_SEC_NONE BIT(0)
#define MESH_CONF_SEC_AUTH BIT(1)
#define MESH_CONF_SEC_AMPE BIT(2)
	unsigned int security;
	enum mfp_options ieee80211w;
	unsigned int pairwise_cipher;
	unsigned int group_cipher;
	unsigned int mgmt_group_cipher;
	int dot11MeshMaxRetries;
	int dot11MeshRetryTimeout; /* msec */
	int dot11MeshConfirmTimeout; /* msec */
	int dot11MeshHoldingTimeout; /* msec */
};
#endif /* CONFIG_MESH */

#define MAX_STA_COUNT 2007
#define MAX_VLAN_ID 4094

typedef u8 macaddr[ETH_ALEN];

struct mac_acl_entry {
	macaddr addr;
#ifndef CONFIG_NO_VLAN
#if 1	/* by Bright : Code sync with Supp 2.4 */
	int	vlan_id;
#else
	struct vlan_description vlan_id;
#endif	/* 0 */
#endif /* CONFIG_NO_VLAN */
};

#ifdef	RADIUS_SERVER
struct hostapd_radius_servers;
#endif	/* RADIUS_SERVER */
#ifdef CONFIG_IEEE80211R
struct ft_remote_r0kh;
struct ft_remote_r1kh;
#endif /* CONFIG_IEEE80211R */

#define NUM_WEP_KEYS 4
struct hostapd_wep_keys {
	u8 idx;
	u8 *key[NUM_WEP_KEYS];
	size_t len[NUM_WEP_KEYS];
	int keys_set;
	size_t default_len; /* key length used for dynamic key generation */
};

typedef enum hostap_security_policy {
	SECURITY_PLAINTEXT = 0,
	SECURITY_STATIC_WEP = 1,
	SECURITY_IEEE_802_1X = 2,
	SECURITY_WPA_PSK = 3,
	SECURITY_WPA = 4,
	SECURITY_OSEN = 5
} secpolicy;

struct hostapd_ssid {
	u8 ssid[SSID_MAX_LEN];
	size_t ssid_len;
	unsigned int ssid_set:1;
	unsigned int utf8_ssid:1;
	unsigned int wpa_passphrase_set:1;
	unsigned int wpa_psk_set:1;

#ifdef	CONFIG_AP_VLAN
	char vlan[IFNAMSIZ + 1];
#endif	/* CONFIG_AP_VLAN */
	secpolicy security_policy;

	struct hostapd_wpa_psk *wpa_psk;
	char *wpa_passphrase;
	char *wpa_psk_file;

#ifdef	CONFIG_AP_WEP_KEY
	struct hostapd_wep_keys wep;
#endif	/* CONFIG_AP_WEP_KEY */

#ifdef	CONFIG_AP_VLAN
#define DYNAMIC_VLAN_DISABLED 0
#define DYNAMIC_VLAN_OPTIONAL 1
#define DYNAMIC_VLAN_REQUIRED 2
	int dynamic_vlan;
#define DYNAMIC_VLAN_NAMING_WITHOUT_DEVICE 0
#define DYNAMIC_VLAN_NAMING_WITH_DEVICE 1
#define DYNAMIC_VLAN_NAMING_END 2

#ifdef	CONFIG_P2P_UNUSED_CMD
	int vlan_naming;
	int per_sta_vif;
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif	/* CONFIG_AP_VLAN */

#ifdef CONFIG_FULL_DYNAMIC_VLAN
	char *vlan_tagged_interface;
#endif /* CONFIG_FULL_DYNAMIC_VLAN */
};

#ifdef	CONFIG_AP_VLAN
#define VLAN_ID_WILDCARD -1

struct hostapd_vlan {
	struct hostapd_vlan *next;
	int vlan_id; /* VLAN ID or -1 (VLAN_ID_WILDCARD) for wildcard entry */
#ifdef	CONFIG_P2P_UNUSED_CMD
	struct vlan_description vlan_desc;
#endif	/* CONFIG_P2P_UNUSED_CMD */
	char ifname[IFNAMSIZ + 1];
#ifdef	CONFIG_P2P_UNUSED_CMD
	int configured;
	int dynamic_vlan;
#endif	/* CONFIG_P2P_UNUSED_CMD */
#ifdef CONFIG_FULL_DYNAMIC_VLAN

#define DVLAN_CLEAN_WLAN_PORT	0x8
	int clean;
#endif /* CONFIG_FULL_DYNAMIC_VLAN */
};
#endif	/* CONFIG_AP_VLAN */

#define PMK_LEN 32
#define MIN_PASSPHRASE_LEN 8
#define MAX_PASSPHRASE_LEN 63
struct hostapd_sta_wpa_psk_short {
	struct hostapd_sta_wpa_psk_short *next;
	unsigned int is_passphrase:1;
	u8 psk[PMK_LEN];
	char passphrase[MAX_PASSPHRASE_LEN + 1];
	int ref; /* (number of references held) - 1 */
};

struct hostapd_wpa_psk {
	struct hostapd_wpa_psk *next;
	int group;
	u8 psk[PMK_LEN];
	u8 addr[ETH_ALEN];
#ifdef	CONFIG_P2P
	u8 p2p_dev_addr[ETH_ALEN];
#endif	/* CONFIG_P2P */
};

struct hostapd_eap_user {
	struct hostapd_eap_user *next;
	u8 *identity;
	size_t identity_len;
	struct {
		int vendor;
		u32 method;
	} methods[EAP_MAX_METHODS];
	u8 *password;
	size_t password_len;
	u8 *salt;
	size_t salt_len; /* non-zero when password is salted */
	int phase2;
	int force_version;
	unsigned int wildcard_prefix:1;
	unsigned int password_hash:1; /* whether password is hashed with
				       * nt_password_hash() */
	unsigned int remediation:1;
	unsigned int macacl:1;
	int ttls_auth; /* EAP_TTLS_AUTH_* bitfield */
	struct hostapd_radius_attr *accept_attr;
	u32 t_c_timestamp;
};

#ifdef	CONFIG_RADIUS
struct hostapd_radius_attr {
	u8 type;
	struct wpabuf *val;
	struct hostapd_radius_attr *next;
};
#endif	/* CONFIG_RADIUS */


#if defined ( __SUPPORT_CON_EASY__ )
  #define NUM_MAX_ACL_CNT	6

  #define NUM_MAX_ACL		NUM_MAX_ACL_CNT
  #define NUM_MAX_ACL_ALLOW	DEFAULT_MAX_NUM_STA
  #define NUM_MAX_ACL_DENY	NUM_MAX_ACL_CNT
#elif defined ( __SMALL_SYSTEM__ )
  #define NUM_MAX_ACL_CNT	6

  #define NUM_MAX_ACL		NUM_MAX_ACL_CNT
  #define NUM_MAX_ACL_ALLOW	DEFAULT_MAX_NUM_STA
  #define NUM_MAX_ACL_DENY	NUM_MAX_ACL_CNT
#else
  #define NUM_MAX_ACL_CNT	DEFAULT_MAX_NUM_STA

  #define NUM_MAX_ACL		NUM_MAX_ACL_CNT
  #define NUM_MAX_ACL_ALLOW	NUM_MAX_ACL_CNT
  #define NUM_MAX_ACL_DENY	NUM_MAX_ACL_CNT
#endif /* ... */

#define NUM_TX_QUEUES		4

struct hostapd_tx_queue_params {
	int aifs;
	int cwmin;
	int cwmax;
	int burst; /* maximum burst time in 0.1 ms, i.e., 10 = 1 ms */
};


#ifdef	CONFIG_ROAM_CONSORTIUM
#define MAX_ROAMING_CONSORTIUM_LEN 15

struct hostapd_roaming_consortium {
	u8 len;
	u8 oi[MAX_ROAMING_CONSORTIUM_LEN];
};
#endif	/* CONFIG_ROAM_CONSORTIUM */

#ifdef	CONFIG_VENUE_NAME
struct hostapd_lang_string {
	u8 lang[3];
	u8 name_len;
	u8 name[252];
};

struct hostapd_venue_url {
	u8 venue_number;
	u8 url_len;
	u8 url[254];
};
#endif	/* CONFIG_VENUE_NAME */

#ifdef	CONFIG_NAI_REALM
#define MAX_NAI_REALMS 10
#define MAX_NAI_REALMLEN 255
#define MAX_NAI_EAP_METHODS 5
#define MAX_NAI_AUTH_TYPES 4
struct hostapd_nai_realm_data {
	u8 encoding;
	char realm_buf[MAX_NAI_REALMLEN + 1];
	char *realm[MAX_NAI_REALMS];
	u8 eap_method_count;
	struct hostapd_nai_realm_eap {
		u8 eap_method;
		u8 num_auths;
		u8 auth_id[MAX_NAI_AUTH_TYPES];
		u8 auth_val[MAX_NAI_AUTH_TYPES];
	} eap_method[MAX_NAI_EAP_METHODS];
};
#endif	/* CONFIG_NAI_REALM */

struct anqp_element {
	struct dl_list list;
	u16 infoid;
	struct wpabuf *payload;
};

struct fils_realm {
	struct dl_list list;
	u8 hash[2];
	char realm[];
};

struct sae_password_entry {
	struct sae_password_entry *next;
	char *password;
	char *identifier;
	u8 peer_addr[ETH_ALEN];
};

/**
 * struct hostapd_bss_config - Per-BSS configuration
 */
struct hostapd_bss_config {
	char iface[IFNAMSIZ + 1];
	char bridge[IFNAMSIZ + 1];
#ifdef	CONFIG_AP_VLAN
	char vlan_bridge[IFNAMSIZ + 1];
#endif	/* CONFIG_AP_VLAN */
#ifdef	CONFIG_AP_WDS
	char wds_bridge[IFNAMSIZ + 1];
#endif	/* CONFIG_AP_WDS */

#ifdef	CONFIG_AP_SYSLOG
	enum hostapd_logger_level logger_syslog_level, logger_stdout_level;

	unsigned int logger_syslog; /* module bitfield */
	unsigned int logger_stdout; /* module bitfield */
#endif	/* CONFIG_AP_SYSLOG */

	int max_num_sta; /* maximum number of STAs in station table */

	int dtim_period;

#ifdef	CONFIG_AP_BSS_LOAD_UPDATE
	unsigned int bss_load_update_period;
#endif	/* CONFIG_AP_BSS_LOAD_UPDATE */
#ifdef	CONFIG_CHANNEL_UTILIZATION
	unsigned int chan_util_avg_period;
#endif	/* CONFIG_CHANNEL_UTILIZATION */

	int ieee802_1x; /* use IEEE 802.1X */
	int eapol_version;
	
	int eap_server; /* Use internal EAP server instead of external
			 * RADIUS server */
	struct hostapd_eap_user *eap_user;
	char *eap_user_sqlite;
#ifdef	UNUSED_CODE
	char *eap_sim_db;
	unsigned int eap_sim_db_timeout;
#endif	/* UNUSED_CODE */
#ifdef	CONFIG_ERP
	int eap_server_erp; /* Whether ERP is enabled on internal EAP server */
#endif	/* CONFIG_ERP */

	struct hostapd_ip_addr own_ip_addr;
	char *nas_identifier;
#ifdef CONFIG_RADIUS
	struct hostapd_radius_servers *radius;
#endif /* CONFIG_RADIUS */
	int acct_interim_interval;

#ifdef	CONFIG_RADIUS
	int radius_request_cui;
	struct hostapd_radius_attr *radius_auth_req_attr;
	struct hostapd_radius_attr *radius_acct_req_attr;
	int radius_das_port;
	unsigned int radius_das_time_window;
	int radius_das_require_event_timestamp;
	int radius_das_require_message_authenticator;
	struct hostapd_ip_addr radius_das_client_addr;
	u8 *radius_das_shared_secret;
	size_t radius_das_shared_secret_len;
#endif	/* CONFIG_RADIUS */

	struct hostapd_ssid ssid;

	char *eap_req_id_text; /* optional displayable message sent with
				* EAP Request-Identity */
	size_t eap_req_id_text_len;
	int eapol_key_index_workaround;

#ifdef	CONFIG_AP_WEP_KEY
	size_t default_wep_key_len;
	int individual_wep_key_len;
	int wep_rekeying_period;
	int broadcast_key_idx_min, broadcast_key_idx_max;
#endif	/* CONFIG_AP_WEP_KEY */
	int eap_reauth_period;
#ifdef	CONFIG_ERP
	int erp_send_reauth_start;
	char *erp_domain;
#endif	/* CONFIG_ERP */

#ifdef	CONFIG_IEEE80211F
	int ieee802_11f; /* use IEEE 802.11f (IAPP) */
	char iapp_iface[IFNAMSIZ + 1]; /* interface used with IAPP broadcast
					* frames */
#endif	/* CONFIG_IEEE80211F */

#ifdef	CONFIG_ACL
	enum macaddr_acl {
		ACCEPT_UNLESS_DENIED = 0,
		DENY_UNLESS_ACCEPTED = 1,
		USE_EXTERNAL_RADIUS_AUTH = 2
	} macaddr_acl;
	struct mac_acl_entry *accept_mac;
	int num_accept_mac;
	struct mac_acl_entry *deny_mac;
	int num_deny_mac;
#endif	/* CONFIG_ACL */

#ifdef	CONFIG_AP_WDS
	int wds_sta;
#endif	/* CONFIG_AP_WDS */

	int isolate;
	int start_disabled;

	int auth_algs; /* bitfield of allowed IEEE 802.11 authentication
			* algorithms, WPA_AUTH_ALG_{OPEN,SHARED,LEAP} */

	int wpa; /* bitfield of WPA_PROTO_WPA, WPA_PROTO_RSN */
	int wpa_key_mgmt;

#ifdef CONFIG_IEEE80211W
	enum mfp_options ieee80211w;
	int group_mgmt_cipher;
	/* dot11AssociationSAQueryMaximumTimeout (in TUs) */
	unsigned int assoc_sa_query_max_timeout;
	/* dot11AssociationSAQueryRetryTimeout (in TUs) */
	int assoc_sa_query_retry_timeout;
#endif /* CONFIG_IEEE80211W */

#ifdef	CONFIG_RADIUS
	enum {
		PSK_RADIUS_IGNORED = 0,
		PSK_RADIUS_ACCEPTED = 1,
		PSK_RADIUS_REQUIRED = 2
	} wpa_psk_radius;
#endif	/* CONFIG_RADIUS */

	int wpa_pairwise;
	int group_cipher; /* wpa_group value override from configuation */
	int wpa_group;
	int wpa_group_rekey;
	int wpa_group_rekey_set;
	int wpa_strict_rekey;
	int wpa_gmk_rekey;
	int wpa_ptk_rekey;
	u32 wpa_group_update_count;
	u32 wpa_pairwise_update_count;
	int wpa_disable_eapol_key_retries;
	int rsn_pairwise;
#ifdef	CONFIG_AP_PRE_AUTH
	int rsn_preauth;
	char *rsn_preauth_interfaces;
#endif	/* CONFIG_AP_PRE_AUTH */

#ifdef CONFIG_IEEE80211R_AP
	/* IEEE 802.11r - Fast BSS Transition */
	u8 mobility_domain[MOBILITY_DOMAIN_ID_LEN];
	u8 r1_key_holder[FT_R1KH_ID_LEN];
	u32 r0_key_lifetime; /* PMK-R0 lifetime seconds */
	int rkh_pos_timeout;
	int rkh_neg_timeout;
	int rkh_pull_timeout; /* ms */
	int rkh_pull_retries;
	u32 reassociation_deadline;
	struct ft_remote_r0kh *r0kh_list;
	struct ft_remote_r1kh *r1kh_list;
	int pmk_r1_push;
	int ft_over_ds;
	int ft_psk_generate_local;
	int r1_max_key_lifetime;
#endif /* CONFIG_IEEE80211R_AP */

	char *ctrl_interface; /* directory for UNIX domain sockets */
#if 0//ndef CONFIG_NATIVE_WINDOWS
	gid_t ctrl_interface_gid;
#endif /* CONFIG_NATIVE_WINDOWS */
	int ctrl_interface_gid_set;

	char *ca_cert;
	char *server_cert;
	char *private_key;
	char *private_key_passwd;
	int check_crl;
	unsigned int tls_session_lifetime;
	unsigned int tls_flags;
	char *ocsp_stapling_response;
	char *ocsp_stapling_response_multi;
	char *dh_file;
#ifdef CONFIG_OPENSSL_MOD
	char *openssl_ciphers;
#endif /* CONFIG_OPENSSL_MOD */
	u8 *pac_opaque_encr_key;
	u8 *eap_fast_a_id;
	size_t eap_fast_a_id_len;
	char *eap_fast_a_id_info;
	int eap_fast_prov;
	int pac_key_lifetime;
	int pac_key_refresh_time;
#if defined(EAP_SERVER_SIM) || defined(EAP_SERVER_AKA)
	int eap_sim_aka_result_ind;
#endif	/* defined(EAP_SERVER_SIM) || defined(EAP_SERVER_AKA) */
	int tnc;
	int fragment_size;
	u16 pwd_group;

#ifdef	RADIUS_SERVER
	char *radius_server_clients;
	int radius_server_auth_port;
	int radius_server_acct_port;
	int radius_server_ipv6;
#endif	/* RADIUS_SERVER */

	int use_pae_group_addr; /* Whether to send EAPOL frames to PAE group
				 * address instead of individual address
				 * (for driver_wired.c).
				 */

	int ap_max_inactivity;
#if 1	/* by Shingu 20161010 (Keep-alive) */
	int ap_send_ka;
#endif	/* CONFIG_AP */
	int ignore_broadcast_ssid;
#ifdef CONFIG_NO_PROBE_RESP_IF_MAX_STA
	int no_probe_resp_if_max_sta;
#endif /* CONFIG_NO_PROBE_RESP_IF_MAX_STA */

	int wmm_enabled;
	int wmm_uapsd;

#ifdef	CONFIG_AP_VLAN
	struct hostapd_vlan *vlan;
#endif	/* CONFIG_AP_VLAN */

	macaddr bssid;

	/*
	 * Maximum listen interval that STAs can use when associating with this
	 * BSS. If a STA tries to use larger value, the association will be
	 * denied with status code 51.
	 */
	u16 max_listen_interval;

#ifdef	CONFIG_AP_PMKSA
	int disable_pmksa_caching;
#endif	/* CONFIG_AP_PMKSA */
	int okc; /* Opportunistic Key Caching */

	int wps_state;
#ifdef CONFIG_WPS
	int wps_independent;
	int ap_setup_locked;
	u8 uuid[16];
	char *wps_pin_requests;
	char *device_name;
	char *manufacturer;
	char *model_name;
	char *model_number;
	char *serial_number;
	u8 device_type[WPS_DEV_TYPE_LEN];
	char *config_methods;
	u8 os_version[4];
	char *ap_pin;
	int skip_cred_build;
	u8 *extra_cred;
	size_t extra_cred_len;
	int wps_cred_processing;
	int force_per_enrollee_psk;
	u8 *ap_settings;
	size_t ap_settings_len;
	char *upnp_iface;
	char *friendly_name;
	char *manufacturer_url;
	char *model_description;
	char *model_url;
	char *upc;
	struct wpabuf *wps_vendor_ext[MAX_WPS_VENDOR_EXTENSIONS];
#ifdef	CONFIG_P2P_UNUSED_CMD
	int wps_nfc_pw_from_config;
	int wps_nfc_dev_pw_id;
	struct wpabuf *wps_nfc_dh_pubkey;
	struct wpabuf *wps_nfc_dh_privkey;
	struct wpabuf *wps_nfc_dev_pw;
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif /* CONFIG_WPS */
	int pbc_in_m1;
	char *server_id;

#define P2P_ENABLED BIT(0)
#define P2P_GROUP_OWNER BIT(1)
#define P2P_GROUP_FORMATION BIT(2)
#define P2P_MANAGE BIT(3)
#define P2P_ALLOW_CROSS_CONNECTION BIT(4)
	int p2p;
#ifdef CONFIG_P2P
	u8 ip_addr_go[4];
	u8 ip_addr_mask[4];
	u8 ip_addr_start[4];
	u8 ip_addr_end[4];
#endif /* CONFIG_P2P */

	int disassoc_low_ack;
	int skip_inactivity_poll;

#ifdef	CONFIG_TDLS
#define TDLS_PROHIBIT BIT(0)
#define TDLS_PROHIBIT_CHAN_SWITCH BIT(1)
	int tdls;
#endif	/* CONFIG_TDLS */
	int disable_11n;
#ifdef	CONFIG_IEEE80211AC
	int disable_11ac;
#endif	/* CONFIG_IEEE80211AC */

#ifdef	CONFIG_TIME_ADV
	/* IEEE 802.11v */
	int time_advertisement;
	char *time_zone;
#endif	/* CONFIG_TIME_ADV */
#ifdef	CONFIG_WNM_SLEEP_MODE
	int wnm_sleep_mode;
	int wnm_sleep_mode_no_keys;
#endif	/* CONFIG_WNM_SLEEP_MODE */
#ifdef CONFIG_WNM_BSS_TRANS_MGMT
	int bss_transition;
#endif /* CONFIG_WNM_BSS_TRANS_MGMT */
#ifdef	CONFIG_INTERWORKING

	/* IEEE 802.11u - Interworking */
	int interworking;
	int access_network_type;
	int internet;
	int asra;
	int esr;
	int uesa;
	int venue_info_set;
	u8 venue_group;
	u8 venue_type;
	u8 hessid[ETH_ALEN];
#endif	/* CONFIG_INTERWORKING */

#ifdef	CONFIG_ROAM_CONSORTIUM
	/* IEEE 802.11u - Roaming Consortium list */
	unsigned int roaming_consortium_count;
	struct hostapd_roaming_consortium *roaming_consortium;
#endif	/* CONFIG_ROAM_CONSORTIUM */

#ifdef	CONFIG_VENUE_NAME
	/* IEEE 802.11u - Venue Name duples */
	unsigned int venue_name_count;
	struct hostapd_lang_string *venue_name;

	/* Venue URL duples */
	unsigned int venue_url_count;
	struct hostapd_venue_url *venue_url;
#endif	/* CONFIG_VENUE_NAME */

#ifdef	CONFIG_NETWORK_AUTH_TYPE
	/* IEEE 802.11u - Network Authentication Type */
	u8 *network_auth_type;
	size_t network_auth_type_len;
#endif	/* CONFIG_NETWORK_AUTH_TYPE */

#ifdef	CONFIG_IP_ADDR_AUTH_TYPE
	/* IEEE 802.11u - IP Address Type Availability */
	u8 ipaddr_type_availability;
	u8 ipaddr_type_configured;
#endif	/* CONFIG_IP_ADDR_AUTH_TYPE */

#ifdef	CONFIG_3GPP
	/* IEEE 802.11u - 3GPP Cellular Network */
	u8 *anqp_3gpp_cell_net;
	size_t anqp_3gpp_cell_net_len;
#endif	/* CONFIG_3GPP */

#ifdef	CONFIG_DOMAIN_NAME
	/* IEEE 802.11u - Domain Name */
	u8 *domain_name;
	size_t domain_name_len;
#endif	/* CONFIG_DOMAIN_NAME */

#ifdef	CONFIG_NAI_REALM
	unsigned int nai_realm_count;
	struct hostapd_nai_realm_data *nai_realm_data;
#endif	/* CONFIG_NAI_REALM */

	struct dl_list anqp_elem; /* list of struct anqp_element */

#ifdef CONFIG_GAS
	u16 gas_comeback_delay;
	size_t gas_frag_limit;
	int gas_address3;
#endif /* CONFIG_GAS */

	u8 qos_map_set[16 + 2 * 21];
	unsigned int qos_map_set_len;

	int osen;
#ifdef CONFIG_PROXYARP
	int proxy_arp;
#endif /* CONFIG_PROXYARP */
	int na_mcast_to_ucast;
#ifdef CONFIG_HS20
	int hs20;
	int disable_dgaf;
	u16 anqp_domain_id;
	unsigned int hs20_oper_friendly_name_count;
	struct hostapd_lang_string *hs20_oper_friendly_name;
	u8 *hs20_wan_metrics;
	u8 *hs20_connection_capability;
	size_t hs20_connection_capability_len;
	u8 *hs20_operating_class;
	u8 hs20_operating_class_len;
	struct hs20_icon {
		u16 width;
		u16 height;
		char language[3];
		char type[256];
		char name[256];
		char file[256];
	} *hs20_icons;
	size_t hs20_icons_count;
	u8 osu_ssid[SSID_MAX_LEN];
	size_t osu_ssid_len;
	struct hs20_osu_provider {
		unsigned int friendly_name_count;
		struct hostapd_lang_string *friendly_name;
		char *server_uri;
		int *method_list;
		char **icons;
		size_t icons_count;
		char *osu_nai;
		unsigned int service_desc_count;
		struct hostapd_lang_string *service_desc;
	} *hs20_osu_providers, *last_osu;
	size_t hs20_osu_providers_count;
	char **hs20_operator_icon;
	size_t hs20_operator_icon_count;
	unsigned int hs20_deauth_req_timeout;
	char *subscr_remediation_url;
	u8 subscr_remediation_method;
	char *t_c_filename;
	u32 t_c_timestamp;
	char *t_c_server_url;
#endif /* CONFIG_HS20 */

	u8 wps_rf_bands; /* RF bands for WPS (WPS_RF_*) */

#ifdef CONFIG_RADIUS_TEST
	char *dump_msk_file;
#endif /* CONFIG_RADIUS_TEST */

#ifdef	CONFIG_AP_VENDOR_ELEM
	struct wpabuf *vendor_elements;
#endif	/* CONFIG_AP_VENDOR_ELEM */
	struct wpabuf *assocresp_elements;

#if defined ( CONFIG_SAE )
	unsigned int sae_anti_clogging_threshold;
	unsigned int sae_sync;
	int sae_require_mfp;
	int *sae_groups;
	struct sae_password_entry *sae_passwords;
#endif	// CONFIG_SAE

#if defined ( CONFIG_WOWLAN )
	char *wowlan_triggers; /* Wake-on-WLAN triggers */
#endif	// CONFIG_WOWLAN

#ifdef CONFIG_TESTING_OPTIONS
	u8 bss_load_test[5];
	u8 bss_load_test_set;
	struct wpabuf *own_ie_override;
	int sae_reflection_attack;
	struct wpabuf *sae_commit_override;
#endif /* CONFIG_TESTING_OPTIONS */

#ifdef	CONFIG_MESH
#define MESH_ENABLED BIT(0)
	int mesh;
#endif	/* CONFIG_MESH */

#ifdef CONFIG_RADIO_MEASUREMENTS
	u8 radio_measurements[RRM_CAPABILITIES_IE_LEN];
#endif /* CONFIG_RADIO_MEASUREMENTS */

#ifdef CONFIG_IEEE80211AC
	int vendor_vht;
#endif /* CONFIG_IEEE80211AC */
	int use_sta_nsts;

#ifdef CONFIG_NO_PROBE_RESP_IF_SEEN_ON
	char *no_probe_resp_if_seen_on;
#endif /* CONFIG_NO_PROBE_RESP_IF_SEEN_ON */
#ifdef CONFIG_NO_AUTH_IF_SEEN_ON
	char *no_auth_if_seen_on;
#endif /* CONFIG_NO_AUTH_IF_SEEN_ON */

	u16 set_greenfield;
	u8 ht_protection;

	int pbss;

#ifdef CONFIG_MBO
	int mbo_enabled;
	/**
	 * oce - Enable OCE in AP and/or STA-CFON mode
	 *  - BIT(0) is Reserved
	 *  - Set BIT(1) to enable OCE in STA-CFON mode
	 *  - Set BIT(2) to enable OCE in AP mode
	 */
	unsigned int oce;
	int mbo_cell_data_conn_pref;
#endif /* CONFIG_MBO */

	int ftm_responder;
	int ftm_initiator;

#ifdef CONFIG_FILS
	u8 fils_cache_id[FILS_CACHE_ID_LEN];
	int fils_cache_id_set;
	struct dl_list fils_realms; /* list of struct fils_realm */
	int fils_dh_group;
	struct hostapd_ip_addr dhcp_server;
	int dhcp_rapid_commit_proxy;
	unsigned int fils_hlp_wait_time;
	u16 dhcp_server_port;
	u16 dhcp_relay_port;
#endif /* CONFIG_FILS */

#ifdef CONFIG_M2U_BC_DEAUTH
	int multicast_to_unicast;

	int broadcast_deauth;
#endif /* CONFIG_M2U_BC_DEAUTH */

#ifdef CONFIG_DPP
	char *dpp_connector;
	struct wpabuf *dpp_netaccesskey;
	unsigned int dpp_netaccesskey_expiry;
	struct wpabuf *dpp_csign;
#endif /* CONFIG_DPP */

#ifdef CONFIG_OWE_BEACON
	macaddr owe_transition_bssid;
	u8 owe_transition_ssid[SSID_MAX_LEN];
	size_t owe_transition_ssid_len;
	char owe_transition_ifname[IFNAMSIZ + 1];
	int *owe_groups;
#ifdef CONFIG_OWE_TRANS
	int owe_transition_mode;
#endif // CONFIG_OWE_TRANS
#endif /* CONFIG_OWE_BEACON */
};

/**
 * struct he_phy_capabilities_info - HE PHY capabilities
 */
struct he_phy_capabilities_info {
	Boolean he_su_beamformer;
	Boolean he_su_beamformee;
	Boolean he_mu_beamformer;
};

/**
 * struct he_operation - HE operation
 */
struct he_operation {
	u8 he_bss_color;
	u8 he_default_pe_duration;
	u8 he_twt_required;
	u8 he_rts_threshold;
};

/**
 * struct hostapd_config - Per-radio interface configuration
 */
struct hostapd_config {
	struct hostapd_bss_config **bss, *last_bss;
	size_t num_bss;

	u16 beacon_int;
	int rts_threshold;
	int fragm_threshold;
	u8 send_probe_response;
	u8 channel;
	u8 acs;
	struct wpa_freq_range_list acs_ch_list;
	int acs_exclude_dfs;
	enum hostapd_hw_mode hw_mode; /* HOSTAPD_MODE_IEEE80211A, .. */
	enum {
		LONG_PREAMBLE = 0,
		SHORT_PREAMBLE = 1
	} preamble;

	int *supported_rates;
	int *basic_rates;
	unsigned int beacon_rate;
	enum beacon_rate_type rate_type;

	const struct wpa_driver_ops *driver;
	char *driver_params;

	int ap_table_max_size;
	int ap_table_expiration_time;

	unsigned int track_sta_max_num;
	unsigned int track_sta_max_age;

	char country[3]; /* first two octets: country code as described in
			  * ISO/IEC 3166-1. Third octet:
			  * ' ' (ascii 32): all environments
			  * 'O': Outdoor environemnt only
			  * 'I': Indoor environment only
			  * 'X': Used with noncountry entity ("XXX")
			  * 0x00..0x31: identifying IEEE 802.11 standard
			  *	Annex E table (0x04 = global table)
			  */

#ifdef	CONFIG_IEEE80211D
	int ieee80211d;
#endif	/* CONFIG_IEEE80211D */

#ifdef	CONFIG_IEEE80211H
	int ieee80211h; /* DFS */
#endif	/* CONFIG_IEEE80211H */

	/*
	 * Local power constraint is an octet encoded as an unsigned integer in
	 * units of decibels. Invalid value -1 indicates that Power Constraint
	 * element will not be added.
	 */
	int local_pwr_constraint;

	/* Control Spectrum Management bit */
	int spectrum_mgmt_required;

	struct hostapd_tx_queue_params tx_queue[NUM_TX_QUEUES];

	/*
	 * WMM AC parameters, in same order as 802.1D, i.e.
	 * 0 = BE (best effort)
	 * 1 = BK (background)
	 * 2 = VI (video)
	 * 3 = VO (voice)
	 */
	struct hostapd_wmm_ac_params wmm_ac_params[4];

	int ht_op_mode_fixed;
	u16 ht_capab;
	int ieee80211n;
	int secondary_channel;
	int no_pri_sec_switch;
	int require_ht;

#ifdef CONFIG_AP_WIFI_MODE
	/* Wi-Fi mode setting
	* 0 = DA16X_WIFI_MODE_BGN,
	* 1 = DA16X_WIFI_MODE_GN,
	* 2 = DA16X_WIFI_MODE_BG,
	* 3 = DA16X_WIFI_MODE_N_ONLY,
	* 4 = DA16X_WIFI_MODE_G_ONLY,
	* 5 = DA16X_WIFI_MODE_B_ONLY,
	*/
	int wifi_mode;
#endif /* CONFIG_AP_WIFI_MODE */

	int obss_interval;

#ifdef	CONFIG_IEEE80211AC
	u32 vht_capab;
	int ieee80211ac;
	int require_vht;
	u8 vht_oper_chwidth;
	u8 vht_oper_centr_freq_seg0_idx;
	u8 vht_oper_centr_freq_seg1_idx;
#endif	/* CONFIG_IEEE80211AC */

	u8 ht40_plus_minus_allowed;

	/* Use driver-generated interface addresses when adding multiple BSSs */
	u8 use_driver_iface_addr;

#ifdef CONFIG_FST
	struct fst_iface_cfg fst_cfg;
#endif /* CONFIG_FST */

#ifdef CONFIG_P2P
#ifdef	CONFIG_P2P_UNUSED_CMD
	u8 p2p_go_ctwindow;
#endif	/* CONFIG_P2P_UNUSED_CMD */
#endif /* CONFIG_P2P */

#ifdef CONFIG_TESTING_OPTIONS
	double ignore_probe_probability;
	double ignore_auth_probability;
	double ignore_assoc_probability;
	double ignore_reassoc_probability;
	double corrupt_gtk_rekey_mic_probability;
	int ecsa_ie_only;
#endif /* CONFIG_TESTING_OPTIONS */

#ifdef CONFIG_ACS
	unsigned int acs_num_scans;
	struct acs_bias {
		int channel;
		double bias;
	} *acs_chan_bias;
	unsigned int num_acs_chan_bias;
#endif /* CONFIG_ACS */

	struct wpabuf *lci;
	struct wpabuf *civic;
	int stationary_ap;

#ifdef CONFIG_IEEE80211AX
	int ieee80211ax;
	struct he_phy_capabilities_info he_phy_capab;
	struct he_operation he_op;
#endif /* CONFIG_IEEE80211AX */

#ifdef CONFIG_IEEE80211AC
	/* VHT enable/disable config from CHAN_SWITCH */
#define CH_SWITCH_VHT_ENABLED BIT(0)
#define CH_SWITCH_VHT_DISABLED BIT(1)
	unsigned int ch_switch_vht_config;
#endif /* CONFIG_IEEE80211AC */
#if 1 /* FC9000 Only */
	u8 addba_reject;
#endif /* FC9000 Only */
};


int hostapd_mac_comp(const void *a, const void *b);
struct hostapd_config * hostapd_config_defaults(void);
void hostapd_config_defaults_bss(struct hostapd_bss_config *bss);
void hostapd_config_free_eap_user(struct hostapd_eap_user *user);
void hostapd_config_free_eap_users(struct hostapd_eap_user *user);
void hostapd_config_clear_wpa_psk(struct hostapd_wpa_psk **p);
void hostapd_config_free_bss(struct hostapd_bss_config *conf);
void hostapd_config_free(struct hostapd_config *conf);
int hostapd_maclist_found(struct mac_acl_entry *list, int num_entries,
			  const u8 *addr
#ifdef CONFIG_AP_VLAN
			  , struct vlan_description *vlan_id
#endif /* CONFIG_AP_VLAN */
			  );
int hostapd_rate_found(int *list, int rate);
const u8 * hostapd_get_psk(const struct hostapd_bss_config *conf,
			   const u8 *addr, const u8 *p2p_dev_addr,
			   const u8 *prev_psk);
int hostapd_setup_wpa_psk(struct hostapd_bss_config *conf);
#ifdef	CONFIG_AP_VLAN
int hostapd_vlan_valid(struct hostapd_vlan *vlan,
		       struct vlan_description *vlan_desc);
const char * hostapd_get_vlan_id_ifname(struct hostapd_vlan *vlan,
					int vlan_id);
#endif	/* CONFIG_AP_VLAN */
#ifdef	CONFIG_RADIUS
struct hostapd_radius_attr *
hostapd_config_get_radius_attr(struct hostapd_radius_attr *attr, u8 type);
#endif	/* CONFIG_RADIUS */

int hostapd_config_check(struct hostapd_config *conf, int full_config);
void hostapd_set_security_params(struct hostapd_bss_config *bss,
				 int full_config);

#endif /* HOSTAPD_CONFIG_H */
