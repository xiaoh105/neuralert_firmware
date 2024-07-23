/**
 ****************************************************************************************
 *
 * @file mesh.h
 *
 * @brief
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

#ifndef IEEE80211S_H
#define IEEE80211S_H

#include "os_con.h"
#include "ieee80211_i.h"

/// macd11 mesh path flags
enum diw_mesh_path_flags
{
	/// the mesh path can be used for forwarding
	MESH_PATH_ACTIVE =	BIT(0),
	/// the discovery process is running for this mesh path
	MESH_PATH_RESOLVING =	BIT(1),
	/// the mesh path contains a valid destination sequence number
	MESH_PATH_SN_VALID =	BIT(2),
	/// the mesh path has been manually set and should not be modified
	MESH_PATH_FIXED	=	BIT(3),
	/// the mesh path can has been resolved
	/* used by the mesh path timer to decide when to stop or cancel the mesh path discovery. */
	MESH_PATH_RESOLVED =	BIT(4),
	/// there is an unsent path request for this destination already queued up, waiting for the discovery process to start.
	MESH_PATH_REQ_QUEUED =	BIT(5),
};

/// macd11 mesh deferred tasks
enum mesh_deferred_task_flags
{
	/// run the periodic mesh housekeeping tasks
	MESH_WORK_HOUSEKEEPING,
	/// the mesh path table is full and needs to grow.
	MESH_WORK_GROW_MPATH_TABLE,
	/// the mesh portals table is full and needs to grow
	MESH_WORK_GROW_MPP_TABLE,
	/// the mesh root station needs to send a frame
	MESH_WORK_ROOT,
	/// time to compensate for clock drift relative to other mesh nodes
	MESH_WORK_DRIFT_ADJUST,
	/// rebuild beacon and notify driver of BSS changes
	MESH_WORK_MBSS_CHANGED,
	/// 
	MESH_WORK_CHANNEL_CHANGED,
	/// 
	MESH_WORK_MBSS_SEND,
};

/// macd11 mesh path structure
/*
 * The combination of dst and wsubif is unique in the mesh path table. Since the next_hop STA is only protected by RCU as well,
 * deleting the STA must also remove/substitute the mesh_path structure and wait until that is no longer reachable before destroying the STA completely.
 */
struct mesh_path
{
	/// mesh path destination mac address
	u8 dst[DIW_ETH_ALEN];
	/// used for MPP or MAP
	u8 mpp[DIW_ETH_ALEN];
	/// mesh subif
	struct i3ed11_wifisubif *wsubif;
	/// mesh neighbor to which frames for this destination will be forwarded
	struct um_sta_info __rcu *next_hop;
	/// mesh path discovery timer
	TimerHandle_t 	timer;	//struct timer_list timer;
	/// pending queue for frames sent to this destination while the path is unresolved
	struct wpktbuff_head frame_queue;
	//
	//struct rcu_head rcu;
	/// target sequence number
	u32 sn;
	/// current metric to this destination
	u32 metric;
	/// hops to destination
	u8 hop_count;
	/// in jiffies, when the path will expire or when it expired
	unsigned long exp_time;
	/// timeout (lapse in jiffies) used for the last discovery retry
	u32 discovery_timeout;
	/// number of discovery retries
	u8 discovery_retries;
	/// mesh path flags, as specified on &enum diw_mesh_path_flags
	enum diw_mesh_path_flags flags;
	/// mesh path state lock used to protect changes to the mpath itself. No need to take this lock when adding or removing an mpath to a hash bucket on a path table.
	SemaphoreHandle_t state_lock;
	/// the RANN sender address
	u8 rann_snd_addr[DIW_ETH_ALEN];
	/// the aggregated path metric towards the root node
	u32 rann_metric;
	/// Timestamp of last PREQ sent to root
	unsigned long last_preq_to_root;
	/// the destination station of this path is a root node
	bool is_root;
	/// the destination station of this path is a mesh gate
	bool is_gate;
#ifdef FEATURE_MESH_TIMER_RUN_TO_WORK
	/// 
	struct work_struct mpath_timer_work;
#endif
};

struct mesh_table
{
	/// array of hash buckets of the table
	/* Number of buckets will be 2^N */
	struct diw_hlist_head *hash_buckets;
#ifdef FEATURE_MESH_HASHWLOCK_ONE_USE
	/// array of locks to protect write operations, one per bucket
	SemaphoreHandle_t hashwlock;		
#else
	/// array of locks to protect write operations, one per bucket
	/* One per bucket, for add/del */
	SemaphoreHandle_t *hashwlock;
#endif
	/// 2^size_order - 1, used to compute hash idx
	unsigned int hash_mask;
	/// random value used for hash computations/generation
	__u32 hash_rnd;
	/// number of entries in the table
	/* Up to MAX_MESH_NEIGHBOURS */
	atomic_t entries;
	/// function to free nodes of the table
	void (*free_node) (struct diw_hlist_node *p, bool free_leafs);
	/// function to copy nodes of the table
	int (*copy_node) (struct diw_hlist_node *p, struct mesh_table *newtbl);
	/// determines size of the table, there will be 2^size_order hash buckets
	int size_order;
	/// maximum average length for the hash buckets' list, if it is reached, the table will grow
	int mean_chain_len;
	/// list of known mesh gates and their mpaths by the station. The gate's mpath may or may not be resolved and active.
	struct diw_hlist_head *known_gates;
	/// 
	SemaphoreHandle_t gates_lock;
	// RCU head to free the table
	//struct rcu_head rcu_head;
};

/* Recent multicast cache */
/// must be a power of 2, maximum 256
/* I think that it(16 bucket) is enouph in DA16200 Mesh Network */
#define RMC_BUCKETS		64 //16	//32	//256 , need optimize
#define RMC_QUEUE_MAX_LEN	4
//#define RMC_TIMEOUT		(3 * HZ)	
#define RMC_TIMEOUT		(10 * HZ)


/// entry in the Recent Multicast Cache
/*
 * The Recent Multicast Cache keeps track of the latest multicast frames
 * that have been received by a mesh interface and discards received multicast frames that are found in the cache.
 */
struct rmc_entry
{
	/// 
	struct diw_list_head list;
	/// mesh sequence number of the frame
	u32 seqnum;
	/// expiration time of the entry, in jiffies
	unsigned long exp_time;
	/// source address of the frame
	u8 sa[DIW_ETH_ALEN];
};

struct mesh_rmc {
	struct diw_list_head bucket[RMC_BUCKETS];
	u32 idx_mask;
};

#define I3ED11_MESH_HOUSEKEEPING_INTERVAL (60 * HZ)

#define MESH_SWITCH_INDICATION_INTERVLAL  (5*HZ)

#define MESH_PATH_EXPIRE  6000//2000// (2000*HZ) //(600 * HZ)

/// Default maximum number of plinks per interface
#define MESH_MAX_PLINKS		4 // 256

/// Maximum number of paths per interface
#define MESH_MAX_MPATHS		1024

/// Number of frames buffered per destination for unresolved destinations
#define MESH_FRAME_QUEUE_LEN	 3//10

int diw_mesh_add_mesh_conf_ie(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
int diw_mesh_add_mesh_id_ie(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
int diw_mesh_add_rsn_ie(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
int diw_mesh_add_vendor_ies(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
int diw_mesh_add_ht_cap_ie(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
int diw_mesh_add_ht_oper_ie(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);

int diw_mesh_rmc_init(struct i3ed11_wifisubif *wsubif);
int diw_mesh_rmc_check(struct i3ed11_wifisubif *wsubif, const u8 *addr, struct ieee80211s_hdr *mesh_hdr);
void diw_mesh_rmc_free(struct i3ed11_wifisubif *wsubif);
void diw_mesh_ids_set_default(struct i3ed11_if_mesh *mesh);
bool diw_mesh_matches_local(struct i3ed11_wifisubif *wsubif, struct i3ed11_elems *ie);

void i3ed11s_init(void);
void i3ed11s_update_metric(struct i3ed11_macctrl *macctrl, struct um_sta_info *sta, struct wpktbuff *wpkt);

int i3ed11_fill_mesh_addresses(struct i3ed11_hdr *hdr, __le16 *fc, const u8 *da, const u8 *sa);
int i3ed11_new_mesh_header(struct i3ed11_wifisubif *wsubif, struct ieee80211s_hdr *meshhdr, const char *addr4or5, const char *addr6);
void i3ed11_mesh_init_sdata(struct i3ed11_wifisubif *wsubif);
int i3ed11_start_mesh(struct i3ed11_wifisubif *wsubif);
void i3ed11_stop_mesh(struct i3ed11_wifisubif *wsubif);
void i3ed11_mesh_root_setup(struct i3ed11_if_mesh *ifmsh);
const struct i3ed11_mesh_sync_ops *i3ed11_mesh_sync_ops_get(u8 method);
void i3ed11_mbss_info_change_notify(struct i3ed11_wifisubif *wsubif, u32 changed);

/* mps : mesh power save */
u32 i3ed11_mps_local_status_update(struct i3ed11_wifisubif *wsubif);
u32 i3ed11_mps_set_sta_local_pm(struct um_sta_info *sta, enum nld11_mesh_power_mode pm);
void i3ed11_mps_set_frame_flags(struct i3ed11_wifisubif *wsubif, struct um_sta_info *sta, struct i3ed11_hdr *hdr);
void i3ed11_mps_sta_status_update(struct um_sta_info *sta);
void i3ed11_mps_rx_h_sta_process(struct um_sta_info *sta, struct i3ed11_hdr *hdr);
void i3ed11_mps_frame_release(struct um_sta_info *sta, struct i3ed11_elems *elems);
void i3ed11_mpsp_trigger_process(u8 *qc, struct um_sta_info *sta, bool tx, bool acked);

#if 0
int diw_mesh_nexthop_lookup(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
#else
int diw_mesh_nexthop_lookup(struct i3ed11_wifisubif *wsubif, struct i3ed11_hdr *hdr);
#endif
int diw_mesh_next_hop_resolve(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);

struct mesh_path *diw_mesh_path_add(struct i3ed11_wifisubif *wsubif, const u8 *dst);
void diw_mesh_path_start_discovery(struct i3ed11_wifisubif *wsubif);
void diw_mesh_path_fix_nexthop(struct mesh_path *mpath, struct um_sta_info *next_hop);
void diw_mesh_path_expire(struct i3ed11_wifisubif *wsubif);

int diw_mpp_path_add(struct i3ed11_wifisubif *wsubif, const u8 *dst, const u8 *mpp);
struct mesh_path *diw_mpp_path_lookup(struct i3ed11_wifisubif *wsubif, const u8 *dst);
struct mesh_path *diw_mpp_path_lookup_by_idx(struct i3ed11_wifisubif *wsubif, int idx);
struct mesh_path *diw_mesh_path_lookup(struct i3ed11_wifisubif *wsubif, const u8 *dst);
struct mesh_path *diw_mesh_path_lookup_by_idx(struct i3ed11_wifisubif *wsubif, int idx);

void diw_mesh_rx_path_sel_frame(struct i3ed11_wifisubif *wsubif, struct i3ed11_mgmt *mgmt, size_t len);

int diw_mesh_path_add_gate(struct mesh_path *mpath);
int diw_mesh_path_send_to_gates(struct mesh_path *mpath);
int diw_mesh_gate_num(struct i3ed11_wifisubif *wsubif);

u32 diw_mesh_plink_open(struct um_sta_info *sta);
u32 diw_mesh_plink_block(struct um_sta_info *sta);
u32 diw_mesh_plink_deactivate(struct um_sta_info *sta);
void diw_mesh_plink_broken(struct um_sta_info *sta);

void diw_mesh_neighbour_update(struct i3ed11_wifisubif *wsubif, u8 *hw_addr, struct i3ed11_elems *ie);
bool diw_mesh_peer_accepts_plinks(struct i3ed11_elems *ie);
u32 diw_mesh_accept_plinks_update(struct i3ed11_wifisubif *wsubif);

void diw_mesh_rx_plink_frame(struct i3ed11_wifisubif *wsubif, struct i3ed11_mgmt *mgmt, size_t len, struct i3ed11_rx_status *rx_status);
void diw_mesh_sta_cleanup(struct um_sta_info *sta);

void diw_mesh_mpath_tbl_grow(void);
void diw_mesh_mpp_tbl_grow(void);

int diw_mesh_path_error_tx(struct i3ed11_wifisubif *wsubif, u8 ttl, const u8 *target, u32 target_sn, u16 target_rcode, const u8 *ra);
void diw_mesh_path_assign_nexthop(struct mesh_path *mpath, struct um_sta_info *sta);
void diw_mesh_path_flush_pending(struct mesh_path *mpath);
void diw_mesh_path_tx_pending(struct mesh_path *mpath);
int diw_mesh_path_tbl_init(void);
void diw_mesh_path_tbl_unregister(void);
int diw_mesh_path_del(struct i3ed11_wifisubif *wsubif, const u8 *addr);
void diw_mesh_path_timer(unsigned long data);
void diw_mesh_path_flush_by_nexthop(struct um_sta_info *sta);
void diw_mesh_path_discard_frame(struct i3ed11_wifisubif *wsubif, struct wpktbuff *wpkt);
void diw_mesh_path_tx_root_frame(struct i3ed11_wifisubif *wsubif);

bool diw_mesh_action_is_path_sel(struct i3ed11_mgmt *mgmt);
extern int mesh_paths_generation;
extern int mpp_paths_generation;

#ifdef CONFIG_MAC80211_MESH
void i3ed11_mesh_notify_scan_completed(struct i3ed11_macctrl *macctrl);

void diw_mesh_path_flush_by_iface(struct i3ed11_wifisubif *wsubif);
void diw_mesh_sync_adjust_tbtt(struct i3ed11_wifisubif *wsubif);
void i3ed11s_stop(void);
#else
static inline void
i3ed11_mesh_notify_scan_completed(struct i3ed11_macctrl *macctrl) {}
static inline bool diw_mesh_path_sel_is_hwmp(struct i3ed11_wifisubif *wsubif)
{ return false; }
static inline void diw_mesh_path_flush_by_iface(struct i3ed11_wifisubif *wsubif)
{}
static inline void i3ed11s_stop(void) {}
#endif

void diw_mesh_path_activate(struct mesh_path *mpath);
int i3ed11_mesh_process_chnswitch(struct i3ed11_wifisubif *wsubif, struct i3ed11_elems *elems, bool beacon);

#ifdef FEATURE_MESH_TIMER_RUN_TO_WORK
void diw_mesh_path_timer_work(unsigned long data);
void diw_mesh_path_timer_work_f(struct work_struct *work);
#endif

void diw_mesh_bss_info_send(struct i3ed11_wifisubif *wsubif);

void i3ed11_mesh_rx_bcn_presp(struct i3ed11_wifisubif *wsubif,
					u16 stype,
					struct i3ed11_mgmt *mgmt,
					size_t len,
					struct i3ed11_rx_status *rx_status);
#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
void i3ed11_diw_mesh_path_root_timer(unsigned long data);
#endif

#endif /* IEEE80211S_H */
