/**
 ****************************************************************************************
 *
 * @file ieee80211_i.h
 *
 * @brief Header file
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

#ifndef I3ED11_I_H
#define I3ED11_I_H

#include "skbuff.h"
#include "os_con.h"
#include "net_con.h"
#include "ieee80211_radiotap.h"
#include "cfg80211.h"
#include "mac80211.h"
#include "key.h"
#include "um_sta_info.h"
#include "debug.h"

#include "netdevice.h"

/// Scan state machine states.
enum macd11_scan_state
{
	/// Main entry point to the scan state machine.
	SCAN_DECISION,
	/// Set the next channel to be scanned.
	SCAN_SET_CHANNEL,
	/// Send probe requests and wait for probe responses.
	SCAN_SEND_PROBE,
	/// Suspend the scan and go back to operating channel to send out data.
	SCAN_SUSPEND,
	/// Resume the scan and scan the next channel.
	SCAN_RESUME,
	/// Abort the scan and go back to operating channel.
	SCAN_ABORT,
};

struct i3ed11_macctrl
{
	struct i3ed11_hw hw;

	const struct i3ed11_ops *ops;

	uint32_t queue_stop_reasons[I3ED11_MAX_QUEUES];

	int open_count;
#ifndef FEATURE_UMAC_MESH
	int fif_pspoll, fif_probe_req;
#else
	int fif_fcsfail, fif_plcpfail, fif_control, fif_other_bss, fif_pspoll, fif_probe_req;
	bool quiescing;
#endif /* FEATURE_UMAC_MESH */
	int probe_req_reg;
	uint32_t filter_flags; /* FIF_* */

	bool softmac_ciphers_allocated;
#if defined (UMAC_USE_NO_CHANCTX) || defined (FEATURE_UMAC_MESH)
	bool use_chanctx;
#endif

	bool tim_in_locked_section; /* see i3ed11_beacon_get() */

	struct work_struct reconfig_filter;

#ifdef UMAC_MCAST_ADDRLIST
	/* aggregated multicast list */
	struct netdev_hw_addr_list mc_list;
#endif

	/* device is started */
	bool started;
	/* number of RX chains the hardware has */
	uint8_t rx_chains;

	int tx_headroom; /* required headroom for hardware/radiotap */


	struct wpktbuff_head wpkt_queue_to_sp;

	/* Station data */
	/* The mutex only protects the list, hash table and counter, reads are done with RCU. */
	SemaphoreHandle_t queue_stop_reason_lock;

	SemaphoreHandle_t sta_mtx;
	SemaphoreHandle_t tim_lock;
	TimerHandle_t sta_cleanup;

	SemaphoreHandle_t iflist_mtx;
	SemaphoreHandle_t key_mtx;
	SemaphoreHandle_t mtx;
	SemaphoreHandle_t chanctx_mtx;

	uint32_t num_sta;
	struct diw_list_head sta_list;
	struct um_sta_info __rcu *sta_hash[STA_HASH_SIZE];

#ifdef FEATURE_STA_CLEANUP_WORK
	struct work_struct sta_cleanup_wk;
#endif
	int sta_generation;

	struct wpktbuff_head pending[I3ED11_MAX_QUEUES];

	atomic_t agg_queue_stop[I3ED11_MAX_QUEUES];

	atomic_t iff_allmultis;
	atomic_t iff_promiscs;

	struct diw_rc_ref *rate_ctrl;

#if 0 /* Not used */
	struct crypto_cipher *wep_tx_tfm;
	struct crypto_cipher *wep_rx_tfm;
#endif
	uint32_t wep_iv;

	struct diw_list_head interfaces;


	uint32_t scanning;
	struct cfgd11_ssid scan_ssid;
	struct cfgd11_scan_request *scan_req, *hw_scan_req;
#ifdef UMAC_USE_NO_CHANCTX
	struct cfgd11_chan_def scan_chandef;
#endif
	enum i3ed11_band hw_scan_band;
	int scan_channel_idx;
	int scan_ies_len;
	int hw_scan_ies_bufsize;
	uint32_t leave_oper_channel_time;
	enum macd11_scan_state next_scan_state;
	struct delayed_work scan_work;
	struct i3ed11_wifisubif __rcu *scan_sdata;
#ifdef UMAC_USE_NO_CHANCTX
	struct cfgd11_chan_def _oper_chandef;
	struct i3ed11_channel *tmp_channel;
#endif
	struct diw_list_head chanctx_list;

	int total_ps_buffered;

	bool pspolling;
	bool offchannel_ps_enabled;

	struct i3ed11_wifisubif *ps_sdata;
#ifdef UMAC_HW_SUPPORTS_DYNAMIC_PS
	struct work_struct dyn_ps_enable_work;
	//	struct work_struct dynamic_ps_disable_work;
	TX_TIMER dyn_ps_timer;
#endif

	//	struct notifier_block network_latency_notifier;  /*** del by wyyang 20141016 not used ***/
	//	struct notifier_block ifa_notifier;
	//	struct notifier_block ifa6_notifier;

	int dynamic_ps_forced_timeout;

	int user_pwr_level; /* in dBm, for all interfaces */

#ifdef WITH_UMAC_DPM
	unsigned char dpmfastfullboot;	//dpmfastfullboot : before that, it was connection status
#endif
	/*
	 * Remain-on-channel support
	 */
	struct diw_list_head roc_list;
	struct work_struct hw_roc_start, hw_roc_done;
	uint32_t hw_roc_start_time;
	ULONG roc_cookie_counter;

	struct i3ed11_wifisubif __rcu *p2p_sdata;

	/* To avoid exception crash during GO association due to many incomming pro_req */
	unsigned char suspended;

};

#define AP_MAX_BC_BUFFER 128

#define TOTAL_MAX_TX_BUFFER 20	 //512 

#define I3ED11_ENCRYPT_HEADROOM 8
#define I3ED11_ENCRYPT_TAILROOM 18
#define I3ED11_FRAGMENT_MAX 4
#define I3ED11_UNSET_POWER_LEVEL	INT_MIN
#define I3ED11_DEFAULT_UAPSD_QUEUES 0x0f
#define I3ED11_DEFAULT_MAX_SP_LEN I3ED11_WMM_IE_STA_QOSINFO_SP_ALL
#define I3ED11_DEAUTH_FRAME_LEN	(24 /* hdr */ + 2 /* reason */)

struct i3ed11_fragment_entry
{
	uint32_t first_frag_time;
	uint32_t seq;
	uint32_t rx_queue;
	uint32_t last_frag;
	uint32_t extra_len;
	struct wpktbuff_head wpkt_list;
	// Whether fragments were encrypted with CCMP.
	int ccmp;
	// PN of the last fragment if CCMP was used.
	uint8_t last_pn[6];
};

struct i3ed11_bss
{
	uint32_t device_ts_beacon, device_ts_presp;

	bool wmm_used;
	bool uapsd_supported;

#define I3ED11_MAX_SUPP_RATES 32
	uint8_t supp_rates[I3ED11_MAX_SUPP_RATES];
	size_t supp_rates_len; 
	struct i3ed11_rate *beacon_rate;

	bool has_erp_value;
	uint8_t erp_value;
	uint8_t corrupt_data;
	uint8_t valid_data;
};

/// BSS data corruption flags.
enum i3ed11_bss_corrupt_data_flags
{
	/// Last beacon frame received was corrupted.
	I3ED11_BSS_CORRUPT_BEACON		= BIT(0),
	/// Last probe response received was corrupted.
	I3ED11_BSS_CORRUPT_PROBE_RESP	= BIT(1)
};

/// BSS valid data flags
enum i3ed11_bss_valid_data_flags
{
	/// WMM/UAPSD data was gathered from non-corrupt IE
	I3ED11_BSS_VALID_WMM			= BIT(1),
	/// Supported rates were gathered from non-corrupt IE
	I3ED11_BSS_VALID_RATES		= BIT(2),
	/// ERP flag was gathered from non-corrupt IE
	I3ED11_BSS_VALID_ERP			= BIT(3)
};

typedef unsigned __bitwise__ i3ed11_tx_result;
#define TX_CONTINUE	((__force i3ed11_tx_result) 0u)
#define TX_DROP		((__force i3ed11_tx_result) 1u)
#define TX_QUEUED	((__force i3ed11_tx_result) 2u)

#define I3ED11_TX_UNICAST		BIT(1)
#define I3ED11_TX_PS_BUFFERED	BIT(2)

struct i3ed11_tx_data
{
	struct wpktbuff *wpkt;
	struct wpktbuff_head wpkts;
	struct i3ed11_macctrl *macctrl;
	struct i3ed11_wifisubif *wsubif;
	struct um_sta_info *sta;
	struct i3ed11_key *key;
	struct i3ed11_tx_rate rate;

	uint32_t flags;
};

typedef unsigned __bitwise__ i3ed11_rx_result;
#define RX_CONTINUE		((__force i3ed11_rx_result) 0u)
#define RX_DROP_UNUSABLE	((__force i3ed11_rx_result) 1u)
#define RX_DROP_MONITOR		((__force i3ed11_rx_result) 2u)
#define RX_QUEUED		((__force i3ed11_rx_result) 3u)
#define RX_SKB_REL		((__force i3ed11_rx_result) 4u)	//Added for RX SKB Mem release

/// Packet RX flags.
enum i3ed11_packet_rx_flags
{
	/// Frame is destined to interface currently processed (incl. multicast frames).
	I3ED11_RX_RA_MATCH			= BIT(1),
	/// Fragmented frame.
	I3ED11_RX_FRAGMENTED			= BIT(2),
	/// A-MSDU packet.
	I3ED11_RX_AMSDU			= BIT(3),
	/// Action frame is malformed.
	I3ED11_RX_MALFORMED_ACTION_FRM	= BIT(4),
	/// Frame was subjected to receive reordering.
	I3ED11_RX_DEFERRED_RELEASE		= BIT(5),
};

/// RX data flags.
enum i3ed11_rx_flags
{
	/// Received on cooked monitor already.
	I3ED11_RX_CMNTR		= BIT(0),
	/// This frame was already reported to cfgd11_report_obss_beacon().
	I3ED11_RX_BEACON_REPORTED	= BIT(1),
};

struct i3ed11_rx_data
{
	struct wpktbuff *wpkt;
	struct i3ed11_macctrl *macctrl;
	struct i3ed11_wifisubif *wsubif;
	struct um_sta_info *sta;
	struct i3ed11_key *key;

	uint32_t flags;

	int seqno_idx;
	int security_idx;

	uint32_t tkip_iv32;
	uint16_t tkip_iv16;
};

#ifdef FEATURE_UMAC_MESH
#define I3ED11_MAX_CSA_COUNTERS_NUM 32

struct i3ed11_csa_settings
{
	const u16 *counter_offsets_beacon;
	const u16 *counter_offsets_presp;

	int n_counter_offsets_beacon;
	int n_counter_offsets_presp;

	u8 count;
};
#endif /* FEATURE_UMAC_MESH */

struct beacon_data
{
	uint8_t *head, *tail;
	int head_len, tail_len;
#ifdef FEATURE_UMAC_MESH
	struct i3ed11_meshconf_ie *meshconf;	
	u16 csa_counter_offsets[I3ED11_MAX_CSA_COUNTERS_NUM];
	u8 csa_current_counter;
#endif /* FEATURE_UMAC_MESH */
};

struct probe_resp
{
	//struct rcu_head rcu_head;
	int len;
#ifdef FEATURE_UMAC_MESH
	u16 csa_counter_offsets[I3ED11_MAX_CSA_COUNTERS_NUM];
#endif /* FEATURE_UMAC_MESH */
	uint8_t data[0];
};

struct ps_data
{
	uint8_t tim[sizeof(uint32_t) * BITS_TO_LONGS(I3ED11_MAX_AID + 1)];
#ifndef UMAC_USE_DIRECT_BC_BEACON_BUCKET
	struct wpktbuff_head bc_buf;
#else
#ifdef FEATURE_UMAC_MESH
	struct wpktbuff_head bc_buf;
#endif /* FEATURE_UMAC_MESH */
#endif /* UMAC_USE_DIRECT_BC_BEACON_BUCKET */
	atomic_t num_sta_ps;
	int dtim_count;
	bool dtim_bc_mc;
};

struct i3ed11_if_ap
{
	struct beacon_data __rcu *beacon;
	struct probe_resp __rcu *probe_resp;

	/* to be used after channel switch. */
#ifdef CONFIG_OWE_TRANSITION_MODE
	TX_TIMER next_timer;

	unsigned long next_bss_changed;

	struct beacon_data *next_beacon;
#else
#if 0 /* Not used */
	struct cfgd11_beacon_data *next_beacon;
#endif
#endif
	struct diw_list_head vlans;

	struct ps_data ps;
	atomic_t num_mcast_sta;
};

struct i3ed11_if_wds
{
	struct um_sta_info *sta;
	uint8_t remote_addr[DIW_ETH_ALEN];
};

struct i3ed11_if_vlan
{
	struct diw_list_head list;

	/* used for all tx if the VLAN is configured to 4-addr mode */
	struct um_sta_info __rcu *sta;
};

#ifdef FEATURE_UMAC_MESH
struct mesh_stats
{
	/// Mesh forwarded multicast frames
	__u32 fwded_mcast;
	/// Mesh forwarded unicast frames
	__u32 fwded_unicast;
	/// Mesh total forwarded frames
	__u32 fwded_frames;
	/// Not transmitted since mesh_ttl == 0
	__u32 dropped_frames_ttl;
	/// Not transmitted, no route found
	__u32 dropped_frames_no_route;
	/// Not forwarded due to congestion
	__u32 dropped_frames_congestion;
};

#define PREQ_Q_F_START		0x1
#define PREQ_Q_F_REFRESH	0x2

struct mesh_preq_queue {
	struct diw_list_head list;
	u8 dst[DIW_ETH_ALEN];
	u8 flags;
};
#endif /* FEATURE_UMAC_MESH */

#if HZ/100 == 0
#define I3ED11_ROC_MIN_LEFT	1
#else
#define I3ED11_ROC_MIN_LEFT	(HZ/100)
#endif

struct i3ed11_roc_work
{
	struct diw_list_head list;
	struct diw_list_head dependents;

	struct delayed_work work;

	struct i3ed11_wifisubif *wsubif;

	struct i3ed11_channel *chan;

	bool started, abort, hw_begun, notified;
	//bool to_be_freed;

	uint32_t hw_start_time;

	uint32_t duration, req_duration;
	struct wpktbuff *frame;
	ULONG cookie, mgmt_tx_cookie;
	enum i3ed11_roc_type type;
};

/* flags used in struct i3ed11_if_managed.flags */
enum i3ed11_sta_flags
{
	I3ED11_STA_CONNECTION_POLL	= BIT(1),
	I3ED11_STA_CTRL_PORT	= BIT(2),
	I3ED11_STA_DISABLE_HT	= BIT(4),
	I3ED11_STA_CSA_RECEIVED	= BIT(5),
	I3ED11_STA_MFP_ENABLED	= BIT(6),
	I3ED11_STA_UAPSD_ENABLED	= BIT(7),
	I3ED11_STA_NULLFN_ACKED	= BIT(8),
	I3ED11_STA_RESET_SIGNAL_AVE	= BIT(9),
	I3ED11_STA_DISABLE_40MHZ	= BIT(10),
	I3ED11_STA_DISABLE_VHT	= BIT(11),
	I3ED11_STA_DISABLE_80P80MHZ	= BIT(12),
	I3ED11_STA_DISABLE_160MHZ	= BIT(13),
	I3ED11_STA_DISABLE_WMM	= BIT(14),
};

struct i3ed11_mgd_auth_data
{
	struct cfgd11_bss *bss;
	uint32_t timeout;
	int tries;
	uint16_t algorithm, expected_transaction;

	uint8_t key[DIW_KEY_LEN_WEP104];
	uint8_t key_len, key_idx;
	bool done;
#ifdef FEATURE_WPA3_SAE
	bool peer_confirmed;
#endif
	bool timeout_started;

	uint16_t sae_trans, sae_status;
	size_t data_len;
	uint8_t data[];
};

struct i3ed11_mgd_assoc_data
{
	struct cfgd11_bss *bss;
	const uint8_t *supp_rates;

	uint32_t timeout;
	int tries;

	uint16_t capability;
	uint8_t prev_bssid[DIW_ETH_ALEN];
	uint8_t ssid[I3ED11_MAX_SSID_LEN];
	uint8_t ssid_len;
	uint8_t supp_rates_len;
	bool wmm, uapsd;
	bool need_beacon;
	bool synced;
	bool timeout_started;

	uint8_t ap_ht_param;
	size_t ie_len;
	uint8_t ie[];
};

struct i3ed11_if_managed
{

	TimerHandle_t timer;
	TimerHandle_t conn_mon_timer;
	TimerHandle_t bcn_mon_timer;
	bool timer_deleted;

	struct work_struct monitor_work;
	struct work_struct beacon_connection_loss_work;

	uint32_t beacon_timeout;
	uint32_t probe_timeout;
	int probe_send_count;
	bool nullfunc_failed;
	bool connection_loss;

	struct cfgd11_bss *associated;
	struct i3ed11_mgd_auth_data *auth_data;
	struct i3ed11_mgd_assoc_data *assoc_data;

	uint8_t bssid[DIW_ETH_ALEN];

	uint16_t aid;

	bool powersave;
	bool broken_ap;
	bool have_beacon;
	uint8_t dtim_period;

	uint32_t flags;

	bool beacon_crc_valid;
	uint32_t beacon_crc;

	bool status_acked;
	bool status_received;
	__le16 status_fc;

	enum
	{
		I3ED11_MFP_DISABLED,
		I3ED11_MFP_OPTIONAL,
		I3ED11_MFP_REQUIRED
	} mfp; /* management frame protection */

	uint32_t uapsd_queues;

	uint32_t uapsd_max_sp_len;

	int wmm_last_param_set;

	uint8_t use_4addr;

	s16 p2p_noa_index;

	int last_beacon_signal;

	int ave_beacon_signal;

	uint32_t count_beacon_signal;

	int last_cqm_event_signal;

	int rssi_min_thold, rssi_max_thold;
	int last_ave_beacon_signal;

	struct i3ed11_ht_cap ht_capa;
	struct i3ed11_ht_cap ht_capa_mask;
};

#ifdef CONFIG_MAC80211_IBSS		// lmj remove IBSS feature
struct i3ed11_if_ibss
{
	TX_TIMER timer;

	struct work_struct csa_connection_drop_work;

	uint32_t last_scan_completed;

	uint32_t basic_rates;

	bool fixed_bssid;
	bool fixed_channel;
	bool privacy;

	bool control_port;
	bool userspace_handles_dfs;

	uint8_t bssid[DIW_ETH_ALEN];     // __aligned(2);
	uint8_t ssid[I3ED11_MAX_SSID_LEN];
	uint8_t ssid_len, ie_len;
	uint8_t *ie;
	struct cfgd11_chan_def chandef;

	uint32_t ibss_join_req;

	struct beacon_data __rcu *presp;

	struct i3ed11_ht_cap ht_capa; /* configured ht-cap over-rides */
	struct i3ed11_ht_cap ht_capa_mask; /* Valid parts of ht_capa */

	TX_SEMAPHORE incomplete_lock;     /* ThreadX OS api */
	struct diw_list_head incomplete_stations;

	enum
	{
		I3ED11_IBSS_MLME_SEARCH,
		I3ED11_IBSS_MLME_JOINED,
	} state;
};
#endif

#ifdef FEATURE_UMAC_MESH
/**
 * struct i3ed11_mesh_sync_ops - Extensible synchronization framework interface
 *
 * these declarations define the interface, which enables
 * vendor-specific mesh synchronization
 *
 */
struct i3ed11_elems;
struct i3ed11_mesh_sync_ops {
	void (*rx_bcn_presp)(struct i3ed11_wifisubif *wsubif,
			     u16 stype,
			     struct i3ed11_mgmt *mgmt,
			     struct i3ed11_elems *elems,
			     struct i3ed11_rx_status *rx_status);

	/* should be called with beacon_data under RCU read lock */
	void (*adjust_tbtt)(struct i3ed11_wifisubif *wsubif,
			    struct beacon_data *beacon);
	/* add other framework functions here */
};

struct mesh_csa_settings {
	//struct rcu_head rcu_head;	//not used
	struct cfgd11_csa_settings settings;
};

#ifdef FEATURE_MESH_STAR_LINK_CONTROL
#define MESH_STAR_LINK_LEN 30/*DIW_ETH_ALEN(4+1)*/
#define MESH_PEER_CANDIDATE_DISCARD_TIME 200 /* 2s */
#define MESH_DISCARD_LIST 5
#define MESH_ADJOIN_PEER_COUNT_LIST_LEN (DIW_ETH_ALEN*5/*peer + 1*/)

struct mesh_link_adjoin_list{
	u8 addr_list[MESH_ADJOIN_PEER_COUNT_LIST_LEN];
	u8 len;
	unsigned long time;
};

#endif

struct i3ed11_if_mesh {
#ifdef FEATURE_MESH_FREERTOS
	TimerHandle_t housekeeping_timer;
	TimerHandle_t diw_mesh_path_timer;
	TimerHandle_t diw_mesh_path_root_timer;
	TimerHandle_t switching_indication_timer;
#else
	TX_TIMER housekeeping_timer;
	TX_TIMER diw_mesh_path_timer;
	TX_TIMER diw_mesh_path_root_timer;
	TX_TIMER switching_indication_timer;
#endif	

	unsigned long wrkq_flags;
	unsigned long mbss_changed;

	u8 mesh_id[I3ED11_MAX_MESH_ID_LEN];
	size_t mesh_id_len;

	// active Path selection Protocol id
	u8 mesh_pp_id;
	// active Path selection Metric id
	u8 mesh_pm_id;
	// Congestion Control mode id
	u8 mesh_cc_id;
	// Synchronization Protocol id
	u8 mesh_sp_id;
	// Authentication id
	u8 mesh_auth_id;
	// local mesh Sequence Number
	u32 sn;
	// last used PREQ id
	u32 preq_id;

	//
	atomic_t mpaths;

	// timestamp of last Sequence Number update
	unsigned long last_sn_update;
	// Time when it's ok to send next PERR
	unsigned long next_perr;
	// Timestamp of last PREQ sent
	unsigned long last_preq;

	struct mesh_rmc *rmc;
	SemaphoreHandle_t mesh_preq_queue_lock;
	struct mesh_preq_queue preq_queue;
	int preq_queue_len;
	struct mesh_stats mshstats;
	struct mesh_config mshcfg;
	atomic_t estab_plinks;
	u32 mesh_seqnum;
	bool accepting_plinks;
	int num_gates;
	struct beacon_data __rcu *beacon;
	const u8 *ie;
	u8 ie_len;

	enum
	{
		I3ED11_MESH_SEC_NONE = 0x0,
		I3ED11_MESH_SEC_AUTHED = 0x1,
		I3ED11_MESH_SEC_SECURED = 0x2,
	} security;

	bool user_mpm;

	/* Extensible Synchronization Framework */
	const struct i3ed11_mesh_sync_ops *sync_ops;
	s64 sync_offset_clockdrift_max;
	SemaphoreHandle_t sync_offset_lock;
	u8/*bool*/ adjusting_tbtt;

	/* mesh power save */
	enum nld11_mesh_power_mode nonpeer_pm;
	int ps_peers_light_sleep;
	int ps_peers_deep_sleep;
	struct ps_data ps;

	// Channel Switching Support
	struct mesh_csa_settings __rcu *csa;

	enum
	{
		I3ED11_MESH_CSA_ROLE_NONE,
		I3ED11_MESH_CSA_ROLE_INIT,
		I3ED11_MESH_CSA_ROLE_REPEATER,
	} csa_role;

	u8 chsw_ttl;
	u16 pre_value;

#ifdef FEATURE_MESH_STAR_LINK_CONTROL
	u8 mesh_star_link[MESH_STAR_LINK_IE_LEN];
	size_t mesh_star_link_len;
	struct mesh_link_adjoin_list discard_list[MESH_DISCARD_LIST];
#endif

	/* offset from wpkt->data while building IE */
	int meshconf_offset;
};

#ifdef CONFIG_MAC80211_MESH
#define I3ED11_IFSTA_MESH_CTR_INC(msh, name)	\
	do { (msh)->mshstats.name++; } while (0)
#else
#define I3ED11_IFSTA_MESH_CTR_INC(msh, name) \
	do { } while (0)
#endif /* CONFIG_MAC80211_MESH */
#endif /* FEATURE_UMAC_MESH */

/// Virtual interface flags.
enum i3ed11_wifisubif_flags
{
	/// Interface wants all multicast packets.
	I3ED11_SDATA_ALLMULTI		= BIT(0),
	/// Interface is promisc.
	I3ED11_SDATA_PROMISC			= BIT(1),
	/// Operating in G-only mode.
	I3ED11_SDATA_OPERATING_GMODE		= BIT(2),
	/// Bridge packets between associated stations and deliver multicast frames both back to wireless media and to the local net stack.
	I3ED11_SDATA_DONT_BRIDGE_PACKETS	= BIT(3),
	/// Disconnect after resume.
	I3ED11_SDATA_DISCONNECT_RESUME	= BIT(4),
	/// Indicates interface was added to driver.
	I3ED11_SDATA_IN_DRIVER		= BIT(5),
};

/// Virtual interface state bits.
enum i3ed11_sdata_state_bits
{
	/// Virtual interface is up & running.
	SDATA_STATE_RUNNING,
	/// This interface is currently in offchannel mode, so queues are stopped.
	SDATA_STATE_OFFCHANNEL,
	/// Beaconing was stopped due to offchannel, reset when offchannel returns.
	SDATA_STATE_OFFCHANNEL_BEACON_STOPPED,
};

/// Channel context configuration mode.
enum i3ed11_chanctx_mode
{
	/// Channel context may be used by multiple interfaces.
	I3ED11_CHANCTX_SHARED,
	/// Channel context can be used only by a single interface.
	/* This can be used for example for non-fixed channel IBSS. */
	I3ED11_CHANCTX_EXCLUSIVE
};

/// channel context replacement state
/* This is used for channel context in-place reservations that require channel context switch/swap. */
enum i3ed11_chanctx_replace_state {
	/// no replacement is taking place
	I3ED11_CHANCTX_REPLACE_NONE,
	/// this channel context will be replaced by a (not yet registered) channel context pointed by %replace_ctx
	I3ED11_CHANCTX_WILL_BE_REPLACED,
	/// this (not yet registered) channel context replaces an existing channel context pointed to by %replace_ctx
	I3ED11_CHANCTX_REPLACES_OTHER,
};

struct i3ed11_chanctx
{
	struct diw_list_head list;
	//struct rcu_head rcu_head;

	struct diw_list_head assigned_vifs;
	struct diw_list_head reserved_vifs;

	enum i3ed11_chanctx_replace_state replace_state;
	struct i3ed11_chanctx *replace_ctx;

	enum i3ed11_chanctx_mode mode;
	int refcount;
	bool driver_present;

	struct i3ed11_chanctx_conf conf;
};

struct i3ed11_wifisubif
{
	struct diw_list_head list;

	/* Chang. changed as wifidev pointer field for self using. 141013 */
	//struct wifidev wdev;
	struct wifidev *wdev;

	/* keys */
	struct diw_list_head key_list;

	/* count for keys needing tailroom space allocation */
	int crypto_tx_tailroom_needed_cnt;
	int crypto_tx_tailroom_pending_dec;
	struct delayed_work dec_tailroom_needed_wk;

	struct net_device *dev;
	struct i3ed11_macctrl *macctrl;

	uint32_t flags;

	uint32_t state;

	int drop_unencrypted;

	//char name[IFNAMSIZ];

	/* Fragment table for host-based reassembly */
	struct i3ed11_fragment_entry	fragments[I3ED11_FRAGMENT_MAX];
	uint32_t fragment_next;

	uint16_t noack_map;

	uint8_t wmm_acm;

	struct i3ed11_key __rcu *keys[NUM_DEFAULT_KEYS + NUM_DEFAULT_MGMT_KEYS];
	struct i3ed11_key __rcu *default_unicast_key;
	struct i3ed11_key __rcu *default_multicast_key;
	struct i3ed11_key __rcu *default_mgmt_key;
	struct i3ed11_key __rcu *default_key;

	uint16_t sequence_number;
	__be16 control_port_protocol;
	bool control_port_no_encrypt;
	int encrypt_headroom;

	struct i3ed11_tx_queue_params tx_conf[I3ED11_NUM_ACS];

	struct work_struct work;
	struct wpktbuff_head wpkt_queue;

#ifdef FEATURE_UMAC_MESH
	struct work_struct csa_finalize_work;
	bool csa_block_tx; /* write-protected by macctrl->mtx */
	struct cfgd11_chan_def csa_chandef;
	struct cfgd11_chan_def beforeCsaChandef;
#endif

	uint8_t needed_rx_chains;

#ifdef FEATURE_UMAC_MESH
	enum i3ed11_smps_mode smps_mode;
#endif /* FEATURE_UMAC_MESH */
	int user_pwr_level; /* in dBm */
	int ap_power_level; /* in dBm */

	struct i3ed11_if_ap *bss;

	/* bitmap of allowed (non-MCS) rate indexes for rate control */
	uint32_t rc_rateidx_mask[I3ED11_NUM_BANDS];

	bool rc_has_mcs_mask[I3ED11_NUM_BANDS];
	uint8_t  rc_rateidx_mcs_mask[I3ED11_NUM_BANDS][I3ED11_HT_MCS_MASK_LEN];

	struct u
	{
		struct i3ed11_if_ap ap;
		struct i3ed11_if_wds wds;
		struct i3ed11_if_vlan vlan;
		struct i3ed11_if_managed mgd;
#ifdef FEATURE_UMAC_MESH
		struct i3ed11_if_mesh mesh;
#endif /* FEATURE_UMAC_MESH */
		uint32_t mntr_flags;
	} u;

	/* mgmt tx xmit function locking */
	SemaphoreHandle_t xmit_lock;     /* ThreadX OS api */

	SemaphoreHandle_t cleanup_stations_lock;     /* ThreadX OS api */

#ifdef FEATURE_DESTROY_AUTH_DATA_BSS_LOCK
	SemaphoreHandle_t destory_auth_data_bss_lock;
#endif

	struct diw_list_head cleanup_stations;
	struct work_struct cleanup_stations_wk;

	/* must be last, dynamically sized area in this! */
	struct i3ed11_vif vif;
};

static inline
struct i3ed11_wifisubif *vif_to_sdata(struct i3ed11_vif *p)
{
	return container_of(p, struct i3ed11_wifisubif, vif);
}

static inline void sdata_lock(struct i3ed11_wifisubif *wsubif)
__acquires(&wsubif->wdev.mtx)
{
#ifdef FREERTOS_AVIOD_EXCEP_STA
	xSemaphoreTake(wsubif->wdev->mtx, portMAX_DELAY);
#endif
}

static inline void sdata_unlock(struct i3ed11_wifisubif *wsubif)
__releases(&wsubif->wdev.mtx)
{
#ifdef FREERTOS_AVIOD_EXCEP_STA
	xSemaphoreGive(wsubif->wdev->mtx);
#endif
}

static inline enum i3ed11_band
i3ed11_get_sdata_band(struct i3ed11_wifisubif *wsubif)
{
	enum i3ed11_band band = I3ED11_BAND_2GHZ;

	return band;
}

static inline int
i3ed11_chandef_get_shift(struct cfgd11_chan_def *chandef)
{
	switch (chandef->width)
	{
	case NLD11_CHAN_WIDTH_5:
		return 2;

	case NLD11_CHAN_WIDTH_10:
		return 1;

	default:
		return 0;
	}
}

static inline int
i3ed11_vif_get_shift(struct i3ed11_vif *vif)
{
	struct i3ed11_chanctx_conf *chanctx_conf = NULL;
	int shift = 0;

	chanctx_conf = vif->chanctx_conf;

	if (chanctx_conf)
	{
		shift = i3ed11_chandef_get_shift(&chanctx_conf->def);
	}

	return shift;
}

enum sdata_queue_type
{
	I3ED11_SDATA_QUEUE_TYPE_FRAME	= 0,
	I3ED11_SDATA_QUEUE_AGG_START		= 1,
	I3ED11_SDATA_QUEUE_AGG_STOP		= 2,
};

enum
{
	I3ED11_RX_MSG	= 1,
	I3ED11_TX_STATUS_MSG	= 2,
};

enum que_stop_reason
{
	I3ED11_QUE_STOP_REASON_DRIVER,
	I3ED11_QUE_STOP_REASON_PS,
	I3ED11_QUE_STOP_REASON_CSA,
	I3ED11_QUE_STOP_REASON_AGGREGATION,
	I3ED11_QUE_STOP_REASON_SUSPEND,
	I3ED11_QUE_STOP_REASON_SKB_ADD,
	I3ED11_QUE_STOP_REASON_OFFCHANNEL,
	I3ED11_QUE_STOP_REASON_FLUSH,
};

/// Currently active scan mode.
enum
{
	/// We're currently in the process of scanning but may as well be on the operating channel.
	SCAN_SW_SCANNING = 0,
	/// The hardware is scanning for us, we have no way to determine if we are on the operating channel or not.
	SCAN_HW_SCANNING,
	/// Do a software scan on only the current operating channel. This should not interrupt normal traffic.
	SCAN_ONCHANNEL_SCANNING,
	/// Set for our scan work function when the driver reported that the scan completed.
	SCAN_COMPLETED,
	/// Set for our scan work function when the driver reported a scan complete for an aborted scan.
	SCAN_ABORTED,
	/// Set for our scan work function when the scan is being cancelled.
	SCAN_HW_CANCELLED,
};

static inline struct i3ed11_wifisubif *
I3ED11_DEV_TO_SUB_IF(struct net_device *dev)
{
	/* Chang. wifisubif is included net device . 141008 */
	return dev->wsubif;
}

static inline struct i3ed11_wifisubif *
I3ED11_WDEV_TO_SUB_IF(struct wifidev *wdev)
{
	struct net_device   *netdev;
	netdev = wdev->netdev;

	return netdev->wsubif;
}

/* this struct represents 802.11n's RA/TID combination */
struct i3ed11_ra_tid
{
	uint8_t ra[DIW_ETH_ALEN];
	uint16_t tid;
};

#ifdef FEATURE_UMAC_MESH
/* this struct holds the value parsing from channel switch IE  */
struct i3ed11_csa_ie
{
	struct cfgd11_chan_def chandef;
	u8 mode;
	u8 count;
	u8 ttl;
	u16 pre_value;
	u16 reason_code;
};
#endif /* FEATURE_UMAC_MESH */

/// Parsed Information Elements.
struct i3ed11_elems
{
	const uint8_t *ie_start;
	size_t total_len;

	/* pointers to IEs */
	const uint8_t *ext_capab;
	const uint8_t *ssid;
	const uint8_t *supp_rates;
	const uint8_t *ds_params;
	const struct i3ed11_tim_ie *tim;
	const uint8_t *challenge;
	const uint8_t *rsn;
	const uint8_t *erp_info;
	const uint8_t *ext_supp_rates;
	const uint8_t *wmm_info;
	const uint8_t *wmm_param;
	const struct i3ed11_ht_cap *ht_cap_elem;
	const struct i3ed11_ht_operation *ht_operation;
	const struct i3ed11_meshconf_ie *mesh_config;
	const uint8_t *mesh_id;
	const uint8_t *peering;
	const __le16 *awake_window;
	const uint8_t *preq;
	const uint8_t *prep;
	const uint8_t *perr;
	const struct i3ed11_rann_ie *rann;
	const struct i3ed11_channel_sw_ie *ch_switch_ie;
	const struct i3ed11_ext_chansw_ie *ext_chansw_ie;
	const struct i3ed11_wide_bw_chansw_ie *wide_bw_chansw_ie;
	const uint8_t *country_elem;
	const uint8_t *pwr_constr_elem;
	const struct i3ed11_timeout_interval_ie *timeout_int;
	const uint8_t *opmode_notif;
	const struct i3ed11_sec_chan_offs_ie *sec_chan_offs;
	const struct i3ed11_mesh_chansw_params_ie *mesh_chansw_params_ie;

#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED	
	const uint8_t *mesh_star_link;
#endif

	/* length of them, respectively */
	uint8_t ext_capab_len;
	uint8_t ssid_len;
	uint8_t supp_rates_len;
	uint8_t ds_params_len;	//Added for scan optimize
	uint8_t tim_len;
	uint8_t challenge_len;
	uint8_t rsn_len;
	uint8_t ext_supp_rates_len;
	uint8_t wmm_info_len;
	uint8_t wmm_param_len;
	uint8_t mesh_id_len;
	uint8_t peering_len;
	uint8_t preq_len;
	uint8_t prep_len;
	uint8_t perr_len;
	uint8_t country_elem_len;

#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
	uint8_t mesh_config_len;
	uint8_t mesh_star_link_len;
#endif

	/* whether a parse error occurred while retrieving these elements */
	bool parse_error;
};

static inline struct i3ed11_macctrl *hw_to_macctrl(
	struct i3ed11_hw *hw)
{
	struct i3ed11_macctrl *data;
	struct softmac *softmac;

	softmac = hw->softmac;

	data =  (struct i3ed11_macctrl *)softmac_priv(softmac);

	return data;
}

static inline int i3ed11_bssid_match(const u8 *raddr, const u8 *addr)
{
	return is_equal_ether_addr(raddr, addr) || is_broadcast_ether_addr(raddr);
}

static inline bool
i3ed11_have_rx_timestamp(struct i3ed11_rx_status *status)
{
	return status->flag & (DIW_RX_FLAG_MACTIME_START | DIW_RX_FLAG_MACTIME_END);
}
int i3ed11_hw_cfg(struct i3ed11_macctrl *macctrl, u32 changed);
void i3ed11_tx_set_protected(struct i3ed11_tx_data *tx);
void i3ed11_bss_info_change_notify(struct i3ed11_wifisubif *wsubif, u32 changed);
void i3ed11_configure_filter(struct i3ed11_macctrl *macctrl);
u32 i3ed11_reset_erp_info(struct i3ed11_wifisubif *wsubif);

/* STA code */
void i3ed11_sta_setup_sdata(struct i3ed11_wifisubif *wsubif);
int i3ed11_mgd_auth(struct i3ed11_wifisubif *wsubif,
					struct cfgd11_auth_req *req);
int i3ed11_mgd_assoc(struct i3ed11_wifisubif *wsubif,
					 struct cfgd11_assoc_req *req);
int i3ed11_mgd_deauth(struct i3ed11_wifisubif *wsubif,
					  struct cfgd11_deauth_req *req);
int i3ed11_mgd_disassoc(struct i3ed11_wifisubif *wsubif,
						struct cfgd11_disassoc_req *req);
void i3ed11_send_pspoll(struct i3ed11_macctrl *macctrl,
						struct i3ed11_wifisubif *wsubif);
#ifdef UMAC_HW_SUPPORTS_DYNAMIC_PS
void i3ed11_recalc_ps(struct i3ed11_macctrl *macctrl, s32 latency);
void i3ed11_recalc_ps_vif(struct i3ed11_wifisubif *wsubif);
#endif
int i3ed11_set_arp_filter(struct i3ed11_wifisubif *wsubif);
void i3ed11_sta_work(struct i3ed11_wifisubif *wsubif);
void i3ed11_sta_rx_queued_mgmt(struct i3ed11_wifisubif *wsubif,
							   struct wpktbuff *wpkt);
void i3ed11_sta_reset_beacon_monitor(struct i3ed11_wifisubif *wsubif);
void i3ed11_sta_reset_conn_monitor(struct i3ed11_wifisubif *wsubif);
void i3ed11_mgd_stop(struct i3ed11_wifisubif *wsubif);
void i3ed11_mgd_conn_tx_status(struct i3ed11_wifisubif *wsubif,
							   __le16 fc, bool acked);

/* mesh code */
#ifdef FEATURE_UMAC_MESH
void i3ed11_mesh_work(struct i3ed11_wifisubif *wsubif);
void i3ed11_mesh_rx_queued_mgmt(struct i3ed11_wifisubif *wsubif,
				   struct wpktbuff *wpkt);
int i3ed11_mesh_csa_beacon(struct i3ed11_wifisubif *wsubif,
			      struct cfgd11_csa_settings *csa_settings);
int i3ed11_mesh_finish_csa(struct i3ed11_wifisubif *wsubif);
#endif /* FEATURE_UMAC_MESH */

/* scan/BSS handling */
void i3ed11_scan_work(struct work_struct *work);
#ifdef CONFIG_MAC80211_IBSS		// lmj remove IBSS feature
int i3ed11_request_ibss_scan(struct i3ed11_wifisubif *wsubif,
							 const u8 *ssid, u8 ssid_len,
							 struct i3ed11_channel *chan,
							 enum nld11_bss_scan_width scan_width);
#endif
int i3ed11_request_scan(struct i3ed11_wifisubif *wsubif,
						struct cfgd11_scan_request *req);
void i3ed11_scan_cancel(struct i3ed11_macctrl *macctrl);
void i3ed11_run_deferred_scan(struct i3ed11_macctrl *macctrl);
void i3ed11_scan_rx(struct i3ed11_macctrl *macctrl, struct wpktbuff *wpkt);

void i3ed11_mlme_notify_scan_completed(struct i3ed11_macctrl *macctrl);
struct i3ed11_bss *
i3ed11_bss_info_update(struct i3ed11_macctrl *macctrl,
					   struct i3ed11_rx_status *rx_status,
					   struct i3ed11_mgmt *mgmt,
					   size_t len,
					   struct i3ed11_elems *elems,
					   struct i3ed11_channel *channel);
void i3ed11_rx_bss_put(struct i3ed11_macctrl *macctrl,
					   struct i3ed11_bss *bss);

/* off-channel helpers */
void i3ed11_offchannel_stop_vifs(struct i3ed11_macctrl *macctrl);
void i3ed11_offchannel_return(struct i3ed11_macctrl *macctrl);
void i3ed11_roc_setup(struct i3ed11_macctrl *macctrl);
void i3ed11_start_next_roc(struct i3ed11_macctrl *macctrl);
void i3ed11_roc_purge(struct i3ed11_macctrl *macctrl,
					  struct i3ed11_wifisubif *wsubif);
void i3ed11_roc_notify_destroy(struct i3ed11_roc_work *roc, bool free);
void i3ed11_handle_roc_started(struct i3ed11_roc_work *roc);

#ifdef FEATURE_UMAC_MESH
/* channel switch handling */
int i3ed11_channel_switch(struct softmac *softmac, struct net_device *dev,
			     struct cfgd11_csa_settings *params);
#endif /* FEATURE_UMAC_MESH */

/* interface handling */

int i3ed11_if_add(struct i3ed11_macctrl *macctrl, const char *name,
				  struct wifidev **new_wdev, enum nld11_iftype type,
				  struct vif_params *params);
int i3ed11_if_change_type(struct i3ed11_wifisubif *wsubif,
						  enum nld11_iftype type);
void i3ed11_if_remove(struct i3ed11_wifisubif *wsubif);
void i3ed11_remove_interfaces(struct i3ed11_macctrl *macctrl);
u32 i3ed11_idle_off(struct i3ed11_macctrl *macctrl);
void i3ed11_recalc_idle(struct i3ed11_macctrl *macctrl);
void i3ed11_adjust_monitor_flags(struct i3ed11_wifisubif *wsubif,
								 const int offset);
int i3ed11_do_open(struct wifidev *wdev, bool coming_up);
void i3ed11_sdata_stop(struct i3ed11_wifisubif *wsubif);

bool __i3ed11_recalc_txpower(struct i3ed11_wifisubif *wsubif);
void i3ed11_recalc_txpower(struct i3ed11_wifisubif *wsubif);
int i3ed11_assign_beacon(struct i3ed11_wifisubif *wsubif,
						 struct cfgd11_beacon_data *params);

static inline bool i3ed11_sdata_running(struct i3ed11_wifisubif *wsubif)
{
	//return test_bit(SDATA_STATE_RUNNING, &wsubif->state);
	return true; //courmagu TEST ������ running???
}

/* tx handling */
void i3ed11_clear_tx_pending(struct i3ed11_macctrl *macctrl);
void i3ed11_tx_pending(unsigned long data);
netdev_tx_t i3ed11_subif_start_xmit(struct wpktbuff *wpkt,
									struct net_device *dev, bool bridging);
void i3ed11_purge_tx_queue(struct i3ed11_hw *hw,
						   struct wpktbuff_head *wpkts);

/* HT */
void i3ed11_apply_htcap_overrides(struct i3ed11_wifisubif *wsubif,
								  struct i3ed11_sta_ht_cap *ht_cap);
bool i3ed11_ht_cap_ie_to_sta_ht_cap(struct i3ed11_wifisubif *wsubif,
									struct i3ed11_supported_band *diwband,
									const struct i3ed11_ht_cap *ht_cap_ie,
									struct um_sta_info *sta);
void i3ed11_send_delba(struct i3ed11_wifisubif *wsubif,
					   const u8 *da, u16 tid,
					   u16 initiator, u16 reason_code);

void ___i3ed11_stop_rx_ba_session(struct um_sta_info *sta, u16 tid,
								  u16 initiator, u16 reason, bool stop);
void __i3ed11_stop_rx_ba_session(struct um_sta_info *sta, u16 tid,
								 u16 initiator, u16 reason, bool stop);
void i3ed11_sta_tear_down_BA_sessions(struct um_sta_info *sta,
									  enum i3ed11_agg_stop_reason reason);
void i3ed11_process_delba(struct i3ed11_wifisubif *wsubif,
						  struct um_sta_info *sta,
						  struct i3ed11_mgmt *mgmt, size_t len);
void i3ed11_process_addba_resp(struct i3ed11_macctrl *macctrl,
							   struct um_sta_info *sta,
							   struct i3ed11_mgmt *mgmt,
							   size_t len);
void i3ed11_process_addba_request(struct i3ed11_macctrl *macctrl,
								  struct um_sta_info *sta,
								  struct i3ed11_mgmt *mgmt,
								  size_t len);

int __i3ed11_stop_tx_ba_session(struct um_sta_info *sta, u16 tid,
								enum i3ed11_agg_stop_reason reason);
int ___i3ed11_stop_tx_ba_session(struct um_sta_info *sta, u16 tid,
								 enum i3ed11_agg_stop_reason reason);
void i3ed11_start_tx_ba_cb(struct i3ed11_vif *vif, u8 *ra, u16 tid);
void i3ed11_stop_tx_ba_cb(struct i3ed11_vif *vif, u8 *ra, u8 tid);
void i3ed11_ba_session_work(struct work_struct *work);
void i3ed11_tx_ba_session_handle_start(struct um_sta_info *sta, int tid);
void i3ed11_release_reorder_timeout(struct um_sta_info *sta, int tid);

u8 i3ed11_mcs_to_chains(const struct i3ed11_mcs_info *mcs);
#ifdef FEATURE_ADD_REORDER_WORK
void sta_rx_agg_reorder_timer_work(struct work_struct *work);
#endif
#ifdef SPECTRUM_MANAGEMENT_FOR_5GHZ
/* Spectrum management */
void i3ed11_process_measurement_req(struct i3ed11_wifisubif *wsubif,
									struct i3ed11_mgmt *mgmt,
									size_t len);
#endif

#ifdef FEATURE_UMAC_MESH
/**
 ****************************************************************************************
 * @brief Parses channel switch IEs.
 * @param[in] wsubif Interface which has received the frame.
 * @param[in] diwelems Parsed 802.11 elements received with the frame.
 * @param[in] current_band Indicates the current band.
 * @param[in] sta_flags Contains information about own capabilities and restrictions.
 * to decide which channel switch announcements can be accepted.
 * Only the following subset of &enum i3ed11_sta_flags are evaluated:
 *	%I3ED11_STA_DISABLE_HT, %I3ED11_STA_DISABLE_VHT,
 *	%I3ED11_STA_DISABLE_40MHZ, %I3ED11_STA_DISABLE_80P80MHZ, %I3ED11_STA_DISABLE_160MHZ.
 * @param[in] bssid Currently connected bssid (for reporting).
 * @param[in] csa_ie Parsed 802.11 csa elements on count, mode, chandef and mesh ttl.
 * All of them will be filled with if success only.
 * @return 0 on success, <0 on error and >0 if there is nothing to parse.
 ****************************************************************************************
 */
int i3ed11_parse_ch_switch_ie(struct i3ed11_wifisubif *wsubif,
								   struct i3ed11_elems *diwelems,
								   enum nld11_band current_band,
								   u32 sta_flags, u8 *bssid,
								   struct i3ed11_csa_ie *csa_ie);
#endif /* FEATURE_UMAC_MESH */

/* utility functions/constants */
u8 *i3ed11_get_bssid(struct i3ed11_hdr *hdr, size_t len,
					 enum nld11_iftype type);
int i3ed11_frame_duration(enum i3ed11_band band, size_t len,
						  int rate, int erp, int short_preamble,
						  int shift);

/**
 ****************************************************************************************
 * @brief Indicate a failed Michael MIC to userspace.
 ****************************************************************************************
 */
void macd11_ev_michael_mic_failure(struct i3ed11_wifisubif *wsubif, int keyidx,
								   struct i3ed11_hdr *hdr, const u8 *tsc, gfp_t gfp);

void i3ed11_set_wmm_default(struct i3ed11_wifisubif *wsubif,
							bool bss_notify, bool enable_qos);

void i3ed11_xmit(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt,
				 enum i3ed11_band band);

void __i3ed11_tx_wpkt_tid_band(struct i3ed11_wifisubif *wsubif,
							   struct wpktbuff *wpkt, int tid,
							   enum i3ed11_band band);

static inline void
i3ed11_tx_wpkt_tid_band(struct i3ed11_wifisubif *wsubif,
						struct wpktbuff *wpkt, int tid,
						enum i3ed11_band band)
{
	__i3ed11_tx_wpkt_tid_band(wsubif, wpkt, tid, band);
}

static inline void i3ed11_tx_wpkt_tid(struct i3ed11_wifisubif *wsubif,
									  struct wpktbuff *wpkt, int tid)
{
	__i3ed11_tx_wpkt_tid_band(wsubif, wpkt, tid, I3ED11_BAND_2GHZ );
}

static inline void i3ed11_tx_wpkt(struct i3ed11_wifisubif *wsubif,
								  struct wpktbuff *wpkt)
{
	/* Send all internal mgmt frames on VO. Accordingly set TID to 7. */
	i3ed11_tx_wpkt_tid(wsubif, wpkt, 7);
}

u32 i3ed11_parse_elems_crc(const u8 *start, size_t len, bool action,
						   struct i3ed11_elems *elems,
						   __u64 filter, u32 crc);
static inline void i3ed11_parse_elems(const u8 *start, size_t len,
									  bool action,
									  struct i3ed11_elems *elems)
{
	i3ed11_parse_elems_crc(start, len, action, elems, 0, 0);
}

#ifdef UMAC_HW_SUPPORTS_DYNAMIC_PS
void i3ed11_dynamic_ps_enable_work(struct work_struct *work);
//void i3ed11_dynamic_ps_disable_work(struct work_struct *work); // do not need workque function
void i3ed11_dynamic_ps_disable_work_direct(struct i3ed11_macctrl
		*macctrl); // we do direct call function
void i3ed11_dynamic_ps_timer(unsigned long data);
#endif

void i3ed11_send_nullfn(struct i3ed11_macctrl *macctrl,
						struct i3ed11_wifisubif *wsubif,
						int powersave);
void i3ed11_sta_rx_notify(struct i3ed11_wifisubif *wsubif,
						  struct i3ed11_hdr *hdr);
void i3ed11_sta_tx_notify(struct i3ed11_wifisubif *wsubif,
						  struct i3ed11_hdr *hdr, bool ack);

void i3ed11_wake_ques_by_reason(struct i3ed11_hw *hw,
								unsigned long queues,
								enum que_stop_reason reason);
void i3ed11_stop_ques_by_reason(struct i3ed11_hw *hw,
								unsigned long queues,
								enum que_stop_reason reason);
void i3ed11_wake_que_by_reason(struct i3ed11_hw *hw, int queue,
							   enum que_stop_reason reason);
void i3ed11_stop_que_by_reason(struct i3ed11_hw *hw, int queue,
							   enum que_stop_reason reason);
void i3ed11_propagate_queue_wake(struct i3ed11_macctrl *macctrl, int queue);
void i3ed11_add_pending_wpkt(struct i3ed11_macctrl *macctrl,
							 struct wpktbuff *wpkt);
void i3ed11_add_pending_wpkts_fn(struct i3ed11_macctrl *macctrl,
								 struct wpktbuff_head *wpkts,
								 void (*fn)(void *data), void *data);
static inline void i3ed11_add_pending_wpkts(struct i3ed11_macctrl *macctrl,
		struct wpktbuff_head *wpkts)
{
	i3ed11_add_pending_wpkts_fn(macctrl, wpkts, NULL, NULL);
}
void i3ed11_flush_ques(struct i3ed11_macctrl *macctrl,
					   struct i3ed11_wifisubif *wsubif);

void i3ed11_send_auth(struct i3ed11_wifisubif *wsubif,
					  u16 transaction, u16 auth_alg, u16 status,
					  const u8 *extra, size_t extra_len, const u8 *bssid,
					  const u8 *da, const u8 *key, u8 key_len, u8 key_idx,
					  u32 tx_flags);
void i3ed11_send_deauth_disassoc(struct i3ed11_wifisubif *wsubif,
								 const u8 *bssid, u16 stype, u16 reason,
								 bool send_frame, u8 *frame_buf);
int i3ed11_build_probe_req_ies(struct i3ed11_macctrl *macctrl, u8 *buffer,
							   size_t buffer_len, const u8 *ie, size_t ie_len,
							   enum i3ed11_band band, u32 rate_mask,
							   struct cfgd11_chan_def *chandef);
struct wpktbuff *i3ed11_build_probe_req(struct i3ed11_wifisubif *wsubif,
										u8 *dst, u32 ratemask,
										struct i3ed11_channel *chan,
										const u8 *ssid, size_t ssid_len,
										const u8 *ie, size_t ie_len,
										bool directed);
void i3ed11_send_probe_req(struct i3ed11_wifisubif *wsubif, u8 *dst,
						   const u8 *ssid, size_t ssid_len,
						   const u8 *ie, size_t ie_len,
						   u32 ratemask, bool directed, u32 tx_flags,
						   struct i3ed11_channel *channel, bool scan);

u32 i3ed11_sta_get_rates(struct i3ed11_wifisubif *wsubif,
						 struct i3ed11_elems *elems,
						 enum i3ed11_band band,
						 u32 *basic_rates);

void i3ed11_recalc_min_chandef(struct i3ed11_wifisubif *wsubif);

size_t i3ed11_ie_split(const u8 *ies, size_t ielen, const u8 *ids, int n_ids,
					   size_t offset);
size_t i3ed11_ie_split_vendor(const u8 *ies, size_t ielen, size_t offset);

u8 *i3ed11_ie_build_ht_cap(u8 *cur, struct i3ed11_sta_ht_cap *ht_cap, u16 cap);

#ifdef FEATURE_UMAC_MESH
u8 *i3ed11_ie_build_ht_oper(u8 *cur, struct i3ed11_sta_ht_cap *ht_cap,
			       const struct cfgd11_chan_def *chandef,
			       u16 prot_mode);
#endif /* FEATURE_UMAC_MESH */

#ifdef CONFIG_MAC80211_VHT	// lmj remove VHT feature
u8 *i3ed11_ie_build_vht_cap(u8 *cur, struct i3ed11_sta_vht_cap *vht_cap, u32 cap);
#endif

int i3ed11_parse_bitrates(struct cfgd11_chan_def *chandef,
						  const struct i3ed11_supported_band *diwband,
						  const u8 *srates, int srates_len, u32 *rates);

#ifdef FEATURE_UMAC_MESH
int i3ed11_add_srates_ie(struct i3ed11_wifisubif *wsubif,
			    struct wpktbuff *wpkt, bool need_basic,
			    enum i3ed11_band band);
int i3ed11_add_ext_srates_ie(struct i3ed11_wifisubif *wsubif,
				struct wpktbuff *wpkt, bool need_basic,
				enum i3ed11_band band);

/* channel management */
void i3ed11_ht_oper_to_chandef(struct i3ed11_channel *control_chan,
				  const struct i3ed11_ht_operation *ht_oper,
				  struct cfgd11_chan_def *chandef);
#endif /* FEATURE_UMAC_MESH */

u32 i3ed11_chandef_downgrade(struct cfgd11_chan_def *c);

int i3ed11_vif_use_channel(struct i3ed11_wifisubif *wsubif,
						   const struct cfgd11_chan_def *chandef,
						   enum i3ed11_chanctx_mode mode);
int i3ed11_vif_change_bandwidth(struct i3ed11_wifisubif *wsubif,
								const struct cfgd11_chan_def *chandef,
								u32 *changed);
void i3ed11_vif_release_channel(struct i3ed11_wifisubif *wsubif);
void i3ed11_vif_vlan_copy_chanctx(struct i3ed11_wifisubif *wsubif);
void i3ed11_vif_copy_chanctx_to_vlans(struct i3ed11_wifisubif *wsubif,
									  bool clear);

void i3ed11_recalc_chanctx_min_def(struct i3ed11_macctrl *macctrl,
								   struct i3ed11_chanctx *ctx);

#ifdef FEATURE_UMAC_MESH
int i3ed11_send_action_csa(struct i3ed11_wifisubif *wsubif,
			      struct cfgd11_csa_settings *csa_settings);
#endif

const struct i3ed11_cipher_scheme *
i3ed11_cs_get(struct i3ed11_macctrl *macctrl, u32 cipher,
			  enum nld11_iftype iftype);
int i3ed11_cs_headroom(struct i3ed11_macctrl *macctrl,
					   struct cfgd11_crypto_settings *crypto,
					   enum nld11_iftype iftype);

#ifdef FEATURE_UMAC_MESH
void i3ed11_recalc_dtim(struct i3ed11_macctrl *macctrl,
			   struct i3ed11_wifisubif *wsubif);
#endif /* FEATURE_UMAC_MESH */

#ifdef FEATURE_USED_LOW_MEMORY_SCAN
void i3ed11_rx_mgmt_beacon(struct i3ed11_wifisubif *wsubif,
						   struct i3ed11_mgmt *mgmt, size_t len,
						   struct i3ed11_rx_status *rx_status);

void i3ed11_rx_mgmt_probe_resp(struct i3ed11_wifisubif *wsubif,
							   struct wpktbuff *wpkt);
#endif /* FEATURE_USED_LOW_MEMORY_SCAN */

u32 i3ed11_handle_bss_capability(struct i3ed11_wifisubif *wsubif, u16 capab,
								 bool erp_valid, u8 erp);

#ifdef FEATURE_UMAC_MESH
int i3ed11_rx_to_tx(struct wpktbuff *wpkt);
#endif /* FEATURE_UMAC_MESH */

#ifdef FEATURE_BSS_OTHER_DEL
void cfgd11_bss_other_del(struct softmac *softmac,
					  const u8 *bssid,
					  const u8 *ssid, size_t ssid_len);
#endif

#endif /* I3ED11_I_H */

