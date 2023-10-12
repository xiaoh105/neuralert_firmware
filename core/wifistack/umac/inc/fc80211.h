/**
 ****************************************************************************************
 *
 * @file fc80211.h
 *
 * @brief Header file for WiFi MAC Protocol diwd11 Interface
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

//#include <os_con.h>
//#include <net_con.h>

//#include <if_ether.h>
#include <cfg80211.h>

#include "features.h"

#include "drivers/supp_driver.h"
#include "common/defs.h"
#include "utils/supp_eloop.h"
#include "drivers/fc80211_copy.h"

#include "rbtree.h"

/*
 * TRINITY.SONG
 ****************************************************************************************
 * Start
 */

static void fc80211_send_scan_start(struct cfgd11_reged_dev *rdev,
									struct wifidev *wdev);
static void fc80211_send_scan_done(struct cfgd11_reged_dev *rdev,
								   struct wifidev *wdev);
static void fc80211_send_scan_aborted(struct cfgd11_reged_dev *rdev,
									  struct wifidev *wdev);
static void fc80211_send_rx_auth(struct cfgd11_reged_dev *rdev,
								 struct net_device *netdev,
								 const u8 *buf, size_t len);
static void fc80211_send_rx_assoc(struct cfgd11_reged_dev *rdev,
								  struct net_device *netdev,
								  const u8 *buf, size_t len);
static void fc80211_send_deauth(struct cfgd11_reged_dev *rdev,
								struct net_device *netdev,
								const u8 *buf, size_t len);
static void fc80211_send_disassoc(struct cfgd11_reged_dev *rdev,
								  struct net_device *netdev,
								  const u8 *buf, size_t len);
static void fc80211_send_auth_timeout(struct cfgd11_reged_dev *rdev,
									  struct net_device *netdev, const u8 *addr);
static void fc80211_send_assoc_timeout(struct cfgd11_reged_dev *rdev,
									   struct net_device *netdev, const u8 *addr);

static void fc80211_send_mic_failure_event(struct cfgd11_reged_dev *rdev,
		struct net_device *netdev, enum nld11_key_type key_type, int key_id,
		const u8 *addr, enum fc80211_commands cmd);
static int fc80211_send_event(da16x_drv_msg_buf_t *drv_msg_buf_p, u8 probe_req);

int fc80211_ctrl_bridge(bool bridge_control);
#if 0
void fc80211_send_connect_result(struct cfgd11_reged_dev *rdev,
								 struct net_device *netdev, const u8 *bssid,
								 const u8 *req_ie, size_t req_ie_len,
								 const u8 *resp_ie, size_t resp_ie_len, u16 stat);
void fc80211_send_disconnected(struct cfgd11_reged_dev *rdev,
							   struct net_device *netdev, u16 reason,
							   const u8 *ie, size_t ie_len, bool from_ap);
#endif

/*
 * TRINITY.SONG
 ****************************************************************************************
 * End
 */

#define NOTIFY_DONE		0x0000		/* Don't care */
//#define NOTIFY_OK		0x0001		/* Suits me */
//#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
//#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)

#define DIW_CIPHER_SUITE_BIP_GMAC_128  0x000FAC0B
#define DIW_CIPHER_SUITE_BIP_GMAC_256  0x000FAC0C
#define DIW_CIPHER_SUITE_BIP_CMAC_256  0x000FAC0D

#define HT_CAP_INFO_SMPS_DISABLED		((u16) (BIT(2) | BIT(3)))
#define HT_CAP_INFO_SHORT_GI20MHZ		((u16) BIT(5))
#define HT_CAP_INFO_GREEN_FIELD			((u16) BIT(4))
#define HT_CAP_INFO_RX_STBC_1			((u16) BIT(8))

/* Redefine here when compiling ...
enum da16x_wifi_mode
{
	DA16X_WIFI_MODE_BGN,
	DA16X_WIFI_MODE_GN,
	DA16X_WIFI_MODE_BG,
	DA16X_WIFI_MODE_N_ONLY,
	DA16X_WIFI_MODE_G_ONLY,
	DA16X_WIFI_MODE_B_ONLY,
	DA16X_WIFI_MODE_MAX
};

 */

struct cfgd11_mgmt_registration
{
	struct diw_list_head list;

	/* Current we do not support REPORT OBSS BEACON  */
#ifdef UMAC_BEACON_REGISTER
	u32 nlportid;
#endif
	int match_len;

	__le16 frame_type;
	u8 tmp_padding[2];
	u8 match[];
};

struct wpa_driver_fc80211_data
{
	struct fc80211_global *global;
	struct dl_list list;
	struct dl_list softmac_list;
	char	phyname[16];
	void	*ctx;
	int	ifindex;
	int	if_removed;
	int	if_disabled;
	int	ignore_if_down_event;
#ifdef	SUPPORT_UNUSED_UMAC_FUNC
	struct rfkill_data *rfkill;
#endif	/* SUPPORT_UNUSED_UMAC_FUNC */
	struct wpa_driver_capa capa;
	UCHAR *extended_capa, *extended_capa_mask;
	uint32_t extended_capa_len;
	int	has_capability;

	int	operstate;

	int	scan_complete_events;
	enum scan_states
	{
		_NO_SCAN, _SCAN_REQUESTED, _SCAN_STARTED, _SCAN_COMPLETED,
		_SCAN_ABORTED
#ifdef	SUPPORT_UNUSED_UMAC_FUNC
		, _SCHED_SCAN_STARTED, _SCHED_SCAN_STOPPED, _SCHED_SCAN_RESULTS
#endif	/* SUPPORT_UNUSED_UMAC_FUNC */
	} scan_state;

	UCHAR	auth_bssid[DIW_ETH_ALEN];
	UCHAR	auth_attempt_bssid[DIW_ETH_ALEN];
	UCHAR	bssid[DIW_ETH_ALEN];
	UCHAR	prev_bssid[DIW_ETH_ALEN];
	int	associated;
	UCHAR	ssid[32];
	size_t	ssid_len;
	enum nld11_iftype	nlmode;
	enum nld11_iftype	ap_scan_as_station;
	uint32_t	assoc_freq;

#ifdef	UNUSED_FUNC_IN_SUPPLICANT
	int	monitor_sock;
	int	monitor_ifidx;
	int	monitor_refcount;
#endif	/* UNUSED_FUNC_IN_SUPPLICANT */

	uint32_t disabled_11b_rates: 1;
	uint32_t pending_remain_on_chan: 1;
	uint32_t in_interface_list: 1;
	uint32_t device_ap_sme: 1;
	uint32_t poll_command_supported: 1;
	uint32_t data_tx_status: 1;
	uint32_t scan_for_auth: 1;
	uint32_t retry_auth: 1;
	uint32_t use_monitor: 1;
	uint32_t ignore_next_local_disconnect: 1;
	uint32_t ignore_next_local_deauth: 1;
	uint32_t allow_p2p_device: 1;
	uint32_t hostapd: 1;
	uint32_t start_mode_ap: 1;
	uint32_t start_iface_up: 1;
	uint32_t test_use_roc_tx: 1;
	uint32_t ignore_deauth_event: 1;
	uint32_t dfs_vendor_cmd_avail: 1;

	ULONG	remain_on_chan_cookie;
	ULONG	send_action_cookie;

	uint32_t	last_mgmt_freq;

	struct wpa_driver_scan_filter	*filter_ssids;
	size_t num_filter_ssids;

	struct i802_bss *first_bss;

#ifdef	UNUSED_FUNC_IN_SUPPLICANT
	int	eapol_tx_sock;
	int	eapol_sock; /* socket for EAPOL frames */
#endif	/* UNUSED_FUNC_IN_SUPPLICANT */

	int	default_if_indices[16];
	int	*if_indices;
	int	num_if_indices;

	int	auth_freq;
	UCHAR	auth_bssid_[DIW_ETH_ALEN];
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
};

#if 1
/// Buffer of WPA.
struct wpabuf
{
	/// Total size of the allocated buffer.
	size_t size;
	/// Length of data in the buffer.
	size_t used;
	/// Pointer to the head of the buffer.
	u8 *buf;
	///
	uint32_t flags;
	/* Optionally followed by the allocated buffer. */
};
#endif

struct fc80211_bss_info_buf
{
	struct wpa_driver_fc80211_data *drv;
	struct wpa_scan_results *res;
	uint32_t assoc_freq;
	uint32_t ibss_freq;
	u8 assoc_bssid[DIW_ETH_ALEN];
};

struct fc80211_global
{
	struct dl_list interfaces;

	int if_add_ifindex;
	ULONG if_add_wdevid;
	int if_add_wdevid_set;
	int fc80211_id;
};

enum cfgd11_chan_mode
{
	DIW_CHAN_MODE_UNDEFINED,
	DIW_CHAN_MODE_SHARED,
	DIW_CHAN_MODE_EXCLUSIVE,
};

enum cfgd11_event_type
{
	DIW_EVENT_CONNECT_RESULT,
	DIW_EVENT_ROAMED,
	DIW_EVENT_DISCONNECTED,
	DIW_EVENT_IBSS_JOINED,
};

enum bss_compare_mode
{
	DIW_BSS_CMP_REGULAR,
	DIW_BSS_CMP_HIDE_ZLEN,
	DIW_BSS_CMP_HIDE_NUL,
};

struct cfgd11_reged_dev
{
	//	const struct cfgd11_ops *ops;
	struct diw_list_head list;

	/* softmac index, internal only */
	int softmac_idx;
	/* for registered beacon list mgmt. */
#ifdef	UMAC_BEACON_REGISTER
	struct diw_list_head	beacon_registrations;
	SemaphoreHandle_t beacon_registrations_lock;
#endif
	struct wifidev *wdev;

	/* BSSes/scanning */
	SemaphoreHandle_t bss_lock;
#ifdef UMAC_FRAME_MGMT_LOCK
	SemaphoreHandle_t mgmt_registrations_lock;
#endif

	struct diw_list_head bss_list;
	struct diw_rb_root bss_tree;

	struct cfgd11_scan_request *scan_req;
	struct work_struct scan_done_wk;

	/* must be last because of the way we do softmac_priv(),
	 * and it should at least be aligned to NETDEV_ALIGN */
	struct softmac softmac;
};

struct cfgd11_event
{
	struct diw_list_head list;
	enum cfgd11_event_type type;

	union
	{
		struct
		{
			uint8_t bssid[DIW_ETH_ALEN];
			const uint8_t *req_ie;
			const uint8_t *resp_ie;
			size_t req_ie_len;
			size_t resp_ie_len;
			uint16_t status;
		} cr;
		struct
		{
			const uint8_t *req_ie;
			const uint8_t *resp_ie;
			size_t req_ie_len;
			size_t resp_ie_len;
			struct cfgd11_bss *bss;
		} rm;
		struct
		{
			const uint8_t *ie;
			size_t ie_len;
			uint16_t reason;
		} dc;
		struct
		{
			uint8_t bssid[DIW_ETH_ALEN];
		} ij;
	};
};

struct cfgd11_internal_bss
{
	struct diw_list_head list;
	struct diw_list_head hidden_list;
	struct diw_rb_node rbn;
	uint32_t ts;
	uint32_t refcount;
	atomic_t hold;

	/* must be last because of priv member */
	struct cfgd11_bss pub;
};

#if 0
struct cfgd11_cached_keys
{
	struct key_params params[6];
	u8 data[6][DIW_MAX_KEY_LEN];
	int def, defmgmt;
};
#endif

#ifdef UMAC_BEACON_REGISTER
/* for mgmt beacons registration */
struct cfgd11_beacon_registration
{
	struct diw_list_head list;
	u32	nlportid;
};
#endif

static inline void wdev_lock(struct wifidev *wdev)
__acquires(wdev)
{
	xSemaphoreTake(wdev->mtx, portMAX_DELAY);

	__acquire(wdev->mtx);
}

static inline void wdev_unlock(struct wifidev *wdev)
__releases(wdev)
{
	__release(wdev->mtx);
	xSemaphoreGive(wdev->mtx);
}


struct key_parse
{
	struct key_params p;
	int idx;
	int type;
	bool def, defmgmt;
	bool def_uni, def_multi;
};

struct softmac_info_data
{
	struct wpa_driver_fc80211_data *drv;
	struct wpa_driver_capa *diwcap;

	uint32_t num_multichan_concurrent;

	uint32_t error: 1;
	uint32_t device_ap_sme: 1;
	uint32_t poll_command_supported: 1;
	uint32_t data_tx_status: 1;
	uint32_t monitor_supported: 1;
	uint32_t auth_supported: 1;
	uint32_t connect_supported: 1;
	uint32_t p2p_go_supported: 1;
	uint32_t p2p_client_supported: 1;
	uint32_t p2p_concurrent: 1;
	uint32_t channel_switch_supported: 1;
	uint32_t set_qos_map_supported: 1;
};
