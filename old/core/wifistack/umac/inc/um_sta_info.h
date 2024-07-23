/**
 ****************************************************************************************
 *
 * @file um_sta_info.h
 *
 * @brief Header file for WiFi MAC Protocol Station Information Control
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

#ifndef STA_INFO_H
#define STA_INFO_H

/// Stations flags.
/* These flags are used with &struct um_sta_info's @flags member, but only indirectly with set_sta_flag() and friends. */
enum i3ed11_sta_info_flags
{
	/// Station is authenticated.
	DIW_STA_AUTH,
	/// Station is associated.
	DIW_STA_ASSOC,
	/// Station is in power-save mode.
	DIW_STA_PS_STA,
	/// Station is authorized to send/receive traffic.
	DIW_STA_AUTHORIZED,
	/// Station is capable of receiving short-preamble frames.
	DIW_STA_SHORT_PREAMBLE,
	/// Station is a QoS-STA.
	DIW_STA_WME,
	/// Station is one of our WDS peers.
	DIW_STA_WDS,
	/// Clear PS filter in hardware when the next frame to this station is transmitted.
	DIW_STA_CLEAR_PS_FILT,
	/// Management frame protection is used with this STA.
	DIW_STA_MFP,
	/// Used to deny ADDBA requests during suspend/resume and station removal.
	DIW_STA_BLOCK_BA,
	/// Driver requires keeping this station in power-save mode logically to flush frames that might still be in the queues.
	DIW_STA_PS_DRIVER,
	/// Station sent PS-poll.
	DIW_STA_PSPOLL,
	/// Station is a TDLS peer.
	DIW_STA_TDLS_PEER,
	/// This TDLS peer is authorized to send direct packets. This means the link is enabled.
	DIW_STA_TDLS_PEER_AUTH,
	/// Station requested unscheduled SP while driver was keeping station in power-save mode, reply when the driver unblocks the station.
	DIW_STA_UAPSD,
	/// Station is in a service period, so don't try to reply to other uAPSD trigger frames or PS-Poll.
	DIW_STA_SP,
	/// 4-addr event was already sent for this frame.
	DIW_STA_4ADDR_EVENT,
	/// This station is inserted into the hash table.
	DIW_STA_INSERTED,
	/// Rate control was initialized for this station.
	DIW_STA_RATE_CONTROL,
	/// Toffset calculated for this station is valid.
	DIW_STA_TOFFSET_KNOWN,
	/// Local STA is owner of a mesh Peer Service Period.
	DIW_STA_MPSP_OWNER,
	/// Local STA is recipient of a MPSP.
	DIW_STA_MPSP_RECIPIENT,
};

#define ADDBA_RESP_INTERVAL HZ

/*
 * Currently DA16X MAC allows 15 (DIW_HT_AGG_MAX_RETRIES) addba requests.
 * When this limit is reached aggregation is turned off for given TID permanently.
 * If air status is bad : it is much addba requests.
 * Old version is 3, so let's try 5.
 */
#define DIW_HT_AGG_MAX_RETRIES		5	//7	//15
#define DIW_HT_AGG_BURST_RETRIES		3
#define DIW_HT_AGG_RETRIES_PERIOD		(15 * HZ)

#define DIW_HT_AGG_STATE_DRV_READY		0
#define DIW_HT_AGG_STATE_RESP_RECEIVED	1
#define DIW_HT_AGG_STATE_OPERATIONAL	2
#define DIW_HT_AGG_STATE_STOPPING		3
#define DIW_HT_AGG_STATE_WANT_START		4
#define DIW_HT_AGG_STATE_WANT_STOP		5

enum i3ed11_agg_stop_reason
{
	AGG_STOP_DECLINED,
	AGG_STOP_LOCAL_REQUEST,
	AGG_STOP_PEER_REQUEST,
	AGG_STOP_DESTROY_STA,
};

/// TID aggregation information (Tx).
/* This structure's lifetime is managed by RCU, assignments to the array holding it must hold the aggregation mutex. */
/* The TX path can access it under RCU lock-free if, and only if, the state has the flag %DIW_HT_AGG_STATE_OPERATIONAL set. */
/* Otherwise, the TX path must also acquire the spinlock and re-check the state, see comments in the tx code touching it. */
struct tid_ampdu_tx
{
	//rcu_head for freeing structure.
	//struct rcu_head rcu_head;	//not used
	/// Check if we keep Tx-ing on the TID (by timeout value).
	TimerHandle_t session_timer;
		/// Timer for peer's response to addba request.
	TimerHandle_t addba_resp_timer;

	/// Pending frames queue -- use sta's spinlock to protect.
	struct wpktbuff_head pending;
	/// Session state (see above).
	unsigned long state;
	/// jiffies of last tx activity.
	unsigned long last_tx;
	/// Session timeout value to be filled in ADDBA requests.
	u16 timeout;
	/// Dialog token for aggregation session.
	u8 dialog_token;
	/// Initiator of a session stop.
	u8 stop_initiator;
	/// TX DelBA frame when stopping.
	bool tx_stop;
	/// Reorder buffer size at receiver.
	u8 buf_sz;
	/// SSN of the last failed BAR tx attempt.
	u16 failed_bar_ssn;
	/// BAR needs to be re-sent.
	bool bar_pending;
};

/// TID aggregation information (Rx).
/* This structure's lifetime is managed by RCU, assignments to the array holding it must hold the aggregation mutex. */
/* The @reorder_lock is used to protect the members of this struct, except for @timeout, @buf_sz and @dialog_token,
 * which are constant across the lifetime of the struct (the dialog token being used only for debugging). */
struct tid_ampdu_rx
{
	//RCU head used for freeing this struct.
	//struct rcu_head rcu_head;	//not used
	/// Serializes access to reorder buffer, see below.
	SemaphoreHandle_t reorder_lock;
	/// Buffer to reorder incoming aggregated MPDUs.
#ifdef UMAC_CHANGE_REORDER_BUF
	struct wpktbuff_head *reorder_buf;
#else
	struct wpktbuff **reorder_buf;
#endif
	/// jiffies when wpkt was added.
	unsigned long *reorder_time;
	/// Check if peer keeps Tx-ing on the TID (by timeout value).
	TimerHandle_t session_timer;
	/// Releases expired frames from the reorder buffer.
	TimerHandle_t reorder_timer;
	/// jiffies of last rx activity.
	unsigned long last_rx;
	/// Head sequence number in reordering buffer.
	u16 head_seq_num;
	/// Number of MPDUs in reordering buffer.
	u16 stored_mpdu_num;
	/// Starting Sequence Number expected to be aggregated.
	u16 ssn;
	/// Buffer size for incoming A-MPDUs.
	u16 buf_sz;
	/// Reset timer value (in TUs).
	u16 timeout;
	/// Dialog token for aggregation session.
	u8 dialog_token;
};

/// STA aggregation information.
struct sta_ampdu_mlme
{
	/// mutex to protect all TX data.
	/* Except non-NULL assignments to tid_tx[idx], which are protected by the sta spinlock. */
	/* tid_start_tx is also protected by sta->lock. */
	SemaphoreHandle_t mtx;	/* threadx OS api */

	/* RX */
#ifdef FEATURE_ADD_REORDER_WORK
	///
	struct work_struct reorder_work;
#endif
	/// Aggregation info for Rx per TID -- RCU protected.
	struct tid_ampdu_rx __rcu *tid_rx[I3ED11_NUM_TIDS];
	/// Bitmap indicating on which TIDs the RX timer expired until the work for it runs.
	unsigned long tid_rx_timer_expired[BITS_TO_LONGS(I3ED11_NUM_TIDS)];
	/// Bitmap indicating which BA sessions per TID the driver requested to close until the work for it runs.
	unsigned long tid_rx_stop_requested[BITS_TO_LONGS(I3ED11_NUM_TIDS)];

	/* TX */
	/// Work struct for starting/stopping aggregation.
	struct work_struct work;
	/// Aggregation info for Tx per TID.
	struct tid_ampdu_tx __rcu *tid_tx[I3ED11_NUM_TIDS];
	/// Sessions where start was requested.
	struct tid_ampdu_tx *tid_start_tx[I3ED11_NUM_TIDS];
	/// Timestamp of the last addBA request.
	unsigned long last_addba_req_time[I3ED11_NUM_TIDS];
	/// Number of times addBA request has been sent.
	u8 addba_req_num[I3ED11_NUM_TIDS];
	/// Dialog token enumerator for each new session.
	u8 dialog_token_alloc; /* dialog token allocator */
};

#ifdef FEATURE_MESH_STAR_LINK_CONTROL
#define MESH_STAR_LINK_IE_LEN 160
#endif

/// STA information.
/* This structure collects information about a station that macd11 is communicating with. */
struct um_sta_info
{
	struct diw_list_head list;
	//struct rcu_head rcu_head;
	struct um_sta_info __rcu *hnext;
	struct i3ed11_macctrl *macctrl;
	struct i3ed11_wifisubif *wsubif;
	struct i3ed11_key __rcu *gtk[NUM_DEFAULT_KEYS + NUM_DEFAULT_MGMT_KEYS];
	struct i3ed11_key __rcu *ptk[NUM_DEFAULT_KEYS];
	u8 gtk_idx;
	u8 ptk_idx;
	struct diw_rc_ref *rate_ctrl;
	void *rate_ctrl_priv;
	SemaphoreHandle_t lock;	/* threadx OS api */

#ifdef UMAC_PS_RACE_AVOID
	/* STA powersave lock and frame queues */
	SemaphoreHandle_t ps_lock;
#endif
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_2 // CONFIG_DIW_RATE_CTRL
	u8 listen_interval;
#else
	u16 listen_interval;
#endif
	bool dead;

	bool uploaded;

	enum i3ed11_sta_state sta_state;

	/* Use the accessors defined below */
	unsigned long _flags;

	/* STA powersave frame queues, no more than the internal locking required. */
	struct wpktbuff_head ps_tx_buf[I3ED11_NUM_ACS];
	struct wpktbuff_head tx_filtered[I3ED11_NUM_ACS];
	unsigned long driver_buffered_tids;
	char tim_recal_fail_cnt;

	/* Updated from RX path only, no locking requirements. */
	unsigned long rx_packets;
	unsigned long rx_data_packets;
	unsigned long rx_bytes;
	//unsigned long wep_weak_iv_count; //Not used
	unsigned long last_rx;
	long last_connected;
	//unsigned long num_duplicates; //Not used
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_2 // CONFIG_DIW_RATE_CTRL
	//unsigned long rx_fragments; //Not used
#else
	unsigned long rx_fragments;
#endif
	unsigned long rx_dropped;
	int last_signal;
	int last_ack_signal;

	u8 chains;
	s8 chain_signal_last[I3ED11_MAX_CHAINS];

	__le16 last_seq_ctrl[I3ED11_NUM_TIDS + 1];

	//unsigned long tx_filtered_count;	//Not used , use DPM
	unsigned long tx_retry_failed, tx_retry_count;

#ifdef CONFIG_MAC80211_MESH
	/* moving percentage of failed MSDUs */
	unsigned int fail_avg;
#endif /* CONFIG_MAC80211_MESH */

	//u32 tx_fragments;		// Not used.
	unsigned long tx_packets[I3ED11_NUM_ACS];
	unsigned long tx_bytes[I3ED11_NUM_ACS];
	struct i3ed11_tx_rate last_tx_rate;
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_2 // CONFIG_DIW_RATE_CTRL
	u8 last_rx_rate_idx;
#else
	int last_rx_rate_idx;
#endif
	u32 last_rx_rate_flag;
	u16 tid_seq[I3ED11_QOS_CTL_TID_MASK + 1];

	struct sta_ampdu_mlme ampdu_mlme;
	u8 timer_to_tid[I3ED11_NUM_TIDS];

#ifdef FEATURE_ADD_REORDER_WORK
	unsigned long timer_to_tid_reorder;
#endif

#ifdef CONFIG_MAC80211_MESH
		/*
		 * Mesh peer link attributes, protected by plink_lock.
		 * TODO: move to a sub-structure that is referenced with pointer?
		 */
		#if 0		 
		spinlock_t plink_lock;
		#else
		SemaphoreHandle_t plink_lock;	/* threadx OS api */
		#endif
		u16 llid;
		u16 plid;
		u16 reason;
		u8 plink_retries;
		enum nld11_plink_state plink_state;
#ifdef FEATURE_UMAC_USE_MPM 
		u32 plink_timeout;
		TX_TIMER plink_timer;
#endif	
		s64 t_offset;
		s64 t_offset_setpoint;
		/* mesh power save */
		enum nld11_mesh_power_mode local_pm;
		enum nld11_mesh_power_mode peer_pm;
		enum nld11_mesh_power_mode nonpeer_pm;
		bool processed_beacon;
		s32 avg_signal;
#ifdef FEATURE_MESH_STAR_LINK_CONTROL
		u8 star_linked_sta[MESH_STAR_LINK_IE_LEN];
		size_t star_linked_sta_len;
#endif
#endif /* CONFIG_MAC80211_MESH */
	
#ifdef CONFIG_MACD11_DEBUGFS
		struct sta_info_debugfsdentries {
			struct dentry *dir;
			bool add_has_run;
		} debugfs;
#endif /* CONFIG_MACD11_DEBUGFS */

	enum i3ed11_sta_rx_bandwidth cur_max_bandwidth;
#ifdef UMAC_LOST_PACKETS
	unsigned int lost_packets;
#endif
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_2 // CONFIG_DIW_RATE_CTRL
	unsigned char beacon_loss_count;
#else
	unsigned int beacon_loss_count;
#endif
	const struct i3ed11_cipher_scheme *cipher_scheme;

	/* Keep last! */
	struct i3ed11_sta sta;
};
//#pragma pack()

#ifdef FEATURE_UMAC_MESH
static inline enum nld11_plink_state sta_plink_state(struct um_sta_info *sta)
{
#ifdef CONFIG_MAC80211_MESH
	return sta->plink_state;
#else
	return NLD11_PLINK_LISTEN;
#endif /* CONFIG_MAC80211_MESH */
}
#endif /* FEATURE_UMAC_MESH */

static inline void set_sta_flag(struct um_sta_info *sta,
								enum i3ed11_sta_info_flags flag)
{
	set_bit(flag, &sta->_flags);
}

static inline void clear_sta_flag(struct um_sta_info *sta,
								  enum i3ed11_sta_info_flags flag)
{
	clear_bit(flag, &sta->_flags);
}

static inline int test_sta_flag(struct um_sta_info *sta,
								enum i3ed11_sta_info_flags flag)
{
	return test_bit(flag, &sta->_flags);
}

static inline int test_and_clear_sta_flag(struct um_sta_info *sta,
										  enum i3ed11_sta_info_flags flag)
{
	return test_and_clear_bit(flag, &sta->_flags);
}

static inline int test_and_set_sta_flag(struct um_sta_info *sta,
										enum i3ed11_sta_info_flags flag)
{
	return test_and_set_bit(flag, &sta->_flags);
}

int um_sta_info_move_state(struct um_sta_info *sta,
						   enum i3ed11_sta_state new_state);

static inline void sta_info_pre_move_state(struct um_sta_info *sta,
										   enum i3ed11_sta_state new_state)
{
	test_sta_flag(sta, DIW_STA_INSERTED);
	um_sta_info_move_state(sta, new_state);
}

void i3ed11_assign_tid_tx(struct um_sta_info *sta, int tid,
						  struct tid_ampdu_tx *diwtx);

static inline struct tid_ampdu_tx *
rcu_dereference_protected_tid_tx(struct um_sta_info *sta, int tid)
{
	return sta->ampdu_mlme.tid_tx[tid];
}

#define STA_HASH_SIZE 256
#define STA_HASH(sta) (sta[5])

#define STA_MAX_TX_BUFFER	8    //4	//64	//160503
#define STA_MAX_TX_PENDING_BUFFER 	8

#define STA_TX_BUFFER_EXPIRE (10 * HZ)

#define STA_INFO_CLEANUP_INTERVAL (10 * HZ)

/**
 ****************************************************************************************
 * @brief Get a STA info, must be under RCU read lock.
 * @param[in] wsubif
 * @param[in] addr
 ****************************************************************************************
 */
struct um_sta_info *um_sta_info_get(struct i3ed11_wifisubif *wsubif, const u8 *addr);

struct um_sta_info *um_sta_info_get_bss(struct i3ed11_wifisubif *wsubif, const u8 *addr);

static inline
void for_each_sta_info_type_check(struct i3ed11_macctrl *macctrl,
								  const u8 *addr,
								  struct um_sta_info *sta,
								  struct um_sta_info *nxt)
{
}

#define for_each_sta_info(macctrl, _addr, _sta, nxt)			\
	for (	/* initialise loop */					\
		_sta = macctrl->sta_hash[STA_HASH(_addr)],\
		nxt = _sta ? _sta->hnext : NULL;	\
		/* typecheck */						\
		for_each_sta_info_type_check(macctrl, (_addr), _sta, nxt),\
		/* continue condition */				\
		_sta;							\
		/* advance loop */					\
		_sta = nxt,						\
		nxt = _sta ? _sta->hnext : NULL	\
	)								\
	/* compare address and run code only if it matches */		\
	if (is_equal_ether_addr(_sta->sta.addr, (_addr)))

#if 0
/*
 * Get STA info by index, BROKEN!
 */
struct um_sta_info *sta_info_get_by_idx(struct i3ed11_wifisubif *wsubif, int idx);
#endif

/**
 ****************************************************************************************
 * @brief Create a new STA info, caller owns returned structure until um_sta_info_insert().
 * @param[in] wsubif
 * @param[in] addr
 * @param[in] gfp
 ****************************************************************************************
 */
struct um_sta_info *um_sta_info_alloc(struct i3ed11_wifisubif *wsubif,
									  const u8 *addr, gfp_t gfp);

void um_sta_info_free(struct i3ed11_macctrl *macctrl, struct um_sta_info *sta);

/**
 ****************************************************************************************
 * @brief Insert STA info into hash table/list, returns zero or a EEXIST
 * (if the same MAC address is already present).
 * @details
 * Calling the non-rcu version makes the caller relinquish,
 * the _rcu version calls read_lock_rcu() and must be called without it held.
 * @param[in] sta
 ****************************************************************************************
 */
int um_sta_info_insert(struct um_sta_info *sta);
int um_sta_info_insert_rcu(struct um_sta_info *sta) __acquires(RCU);

int __um_sta_info_destroy(struct um_sta_info *sta);
int um_sta_info_destroy_addr(struct i3ed11_wifisubif *wsubif, const u8 *addr);
int um_sta_info_destroy_addr_bss(struct i3ed11_wifisubif *wsubif, const u8 *addr);

void um_sta_info_recalc_tim(struct um_sta_info *sta);

void um_sta_info_init(struct i3ed11_macctrl *macctrl);
void um_sta_info_stop(struct i3ed11_macctrl *macctrl);
int um_sta_info_flush_defer(struct i3ed11_wifisubif *wsubif);

/**
 ****************************************************************************************
 * @brief Flush the diw_um_sta_info cleanup queue.
 * @param[in] wsubif The interface.
 * @details Flushes the diw_um_sta_info cleanup queue for a given interface;\n
 * this is necessary before the interface is removed or, for AP/mesh interfaces, before it is deconfigured.
 * @note An rcu_barrier() must precede the function, after all stations have been flushed/removed
 * to ensure the call_rcu() calls that add stations to the cleanup queue have completed.
 ****************************************************************************************
 */
void um_sta_info_flush_cleanup(struct i3ed11_wifisubif *wsubif);

/**
 ****************************************************************************************
 * @brief Flush matching STA entries from the STA table.
 * @param[in] wsubif To remove all stations from.
 * @return The number of removed STA entries.
 ****************************************************************************************
 */
#if 0
static inline int um_sta_info_flush(struct i3ed11_wifisubif *wsubif)
{
	int ret = um_sta_info_flush_defer(wsubif);

	um_sta_info_flush_cleanup(wsubif);

	return ret;
}
#else
int um_sta_info_flush(struct i3ed11_wifisubif *wsubif);
#endif

void um_sta_set_rate_info_tx(struct um_sta_info *sta,
						  const struct i3ed11_tx_rate *rate,
						  struct rate_info *diwrate);
void um_sta_set_rate_info_rx(struct um_sta_info *sta,
						  struct rate_info *diwrate);

#if 0
void i3ed11_sta_expire(struct i3ed11_wifisubif *wsubif,
					   unsigned long exp_time);
#endif
//u8 sta_info_tx_streams(struct um_sta_info *sta);

void i3ed11_sta_ps_deliver_wakeup(struct um_sta_info *sta);
void i3ed11_sta_ps_deliver_poll_resp(struct um_sta_info *sta);
void i3ed11_sta_ps_deliver_uapsd(struct um_sta_info *sta);

void i3ed11_cleanup_sdata_stas(struct i3ed11_wifisubif *wsubif);

#ifdef FEATURE_MESH_STAR_LINK_CONTROL
void mesh_all_sta_info_get(struct i3ed11_wifisubif *wsubif);
int mesh_linked_sta_discard(struct i3ed11_wifisubif *wsubif,
			      const u8 *addr);
#endif

#endif /* STA_INFO_H */
