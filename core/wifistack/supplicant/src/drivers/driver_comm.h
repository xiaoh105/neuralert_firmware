/**
 ****************************************************************************************
 *
 * @file driver_comm.h
 *
 * @brief Driver interaction with DA16X fc80211
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


struct fc80211_global {
	struct	dl_list		interfaces;
	int	if_add_ifindex;
	ULONG	if_add_wdevid;
	int	if_add_wdevid_set;
	int	fc80211_id;
};

struct fc80211_softmac_data {
	struct dl_list	list;
	struct dl_list	bsss;
	struct dl_list	drvs;

	int softmac_idx;
};


struct i802_bss {
	struct wpa_driver_fc80211_data *drv;
	struct i802_bss *next;
	int ifindex;
	ULONG wdev_id;
	char ifname[IFNAMSIZ + 1];
#ifdef	CONFIG_BRIDGE_IFACE
	char brname[IFNAMSIZ];
#endif	/* CONFIG_BRIDGE_IFACE */
	unsigned int beacon_set:1;
#ifdef	CONFIG_BRIDGE_IFACE
	unsigned int added_if_into_bridge:1;
	unsigned int added_bridge:1;
#endif	/* CONFIG_BRIDGE_IFACE */
	unsigned int in_deinit:1;
	unsigned int wdev_id_set:1;
	unsigned int added_if:1;

	UCHAR addr[ETH_ALEN];

	int freq;
	int bandwidth;
	int if_dynamic;

	void *ctx;

	struct fc80211_softmac_data *softmac_data;
	struct dl_list softmac_list;
};

struct wpa_driver_fc80211_data {
	struct fc80211_global *global;
	struct dl_list list;
	struct dl_list softmac_list;
	char	phyname[16];
	void	*ctx;
	int	ifindex;
	int	if_removed;
	int	if_disabled;
	int	ignore_if_down_event;
	struct wpa_driver_capa capa;
	UCHAR *extended_capa, *extended_capa_mask;
	unsigned int extended_capa_len;
	int	has_capability;

	int	operstate;

	int	scan_complete_events;
	enum scan_states {
		_NO_SCAN, _SCAN_REQUESTED, _SCAN_STARTED, _SCAN_COMPLETED,
		_SCAN_ABORTED
	} scan_state;

	UCHAR	auth_bssid[ETH_ALEN];
	UCHAR	auth_attempt_bssid[ETH_ALEN];
	UCHAR	bssid[ETH_ALEN];
	UCHAR	prev_bssid[ETH_ALEN];
	int	associated;
	UCHAR	ssid[32];
	size_t	ssid_len;
	enum fc80211_iftype	nlmode;
	enum fc80211_iftype	ap_scan_as_station;
	unsigned int	assoc_freq;

	unsigned int disabled_11b_rates:1;
	unsigned int pending_remain_on_chan:1;
	unsigned int in_interface_list:1;
	unsigned int device_ap_sme:1;
	unsigned int poll_command_supported:1;
	unsigned int data_tx_status:1;
	unsigned int scan_for_auth:1;
	unsigned int retry_auth:1;
	unsigned int use_monitor:1;
	unsigned int ignore_next_local_disconnect:1;
	unsigned int ignore_next_local_deauth:1;
	unsigned int allow_p2p_device:1;
	unsigned int hostapd:1;
	unsigned int start_mode_ap:1;
	unsigned int start_iface_up:1;
	unsigned int test_use_roc_tx:1;
	unsigned int ignore_deauth_event:1;
	unsigned int dfs_vendor_cmd_avail:1;

	ULONG	remain_on_chan_cookie;
	ULONG	send_action_cookie;

	unsigned int	last_mgmt_freq;

	struct wpa_driver_scan_filter	*filter_ssids;
	size_t num_filter_ssids;

	struct i802_bss *first_bss;

	int	default_if_indices[16];
	int	*if_indices;
	int	num_if_indices;

	/* From failed authentication command */
	int	auth_freq;
	UCHAR	auth_bssid_[ETH_ALEN];
	UCHAR	auth_ssid[32];
	size_t	auth_ssid_len;
	int	auth_alg;
	UCHAR	*auth_ie;
	size_t	auth_ie_len;
	UCHAR	auth_wep_key[4][16];
	size_t	auth_wep_key_len[4];
	int	auth_wep_tx_keyidx;
	int	auth_local_state_change;
	int	auth_p2p;

	int wnm_intval; //CONFIG_WNM_SLEEP_MODE
#ifdef	CONFIG_SUPP27_DRV_80211
#if 1 //SUPPORT_SELECT_NETWORK_FILTER
	UCHAR	scan_ssid[32];
	size_t	scan_ssid_len;
	UCHAR	scan_bssid[ETH_ALEN];
#endif /* SUPPORT_SELECT_NETWORK_FILTER */
#endif	/* CONFIG_SUPP27_DRV_80211 */
};

struct fc80211_bss_info_buf {
	struct wpa_driver_fc80211_data *drv;
	struct wpa_scan_results *res;
	unsigned int assoc_freq;
	unsigned int ibss_freq;
	UCHAR assoc_bssid[ETH_ALEN];
};

struct softmac_info_data {
	struct wpa_driver_fc80211_data *drv;
	struct wpa_driver_capa *capa;

	unsigned int num_multichan_concurrent;

	unsigned int error:1;
	unsigned int device_ap_sme:1;
	unsigned int poll_command_supported:1;
	unsigned int data_tx_status:1;
	unsigned int monitor_supported:1;
	unsigned int auth_supported:1;
	unsigned int connect_supported:1;
	unsigned int p2p_go_supported:1;
	unsigned int p2p_client_supported:1;
	unsigned int p2p_concurrent:1;
	unsigned int channel_switch_supported:1;
	unsigned int set_qos_map_supported:1;
};


/* EOF */
