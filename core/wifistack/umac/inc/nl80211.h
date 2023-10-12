/**
 ****************************************************************************************
 *
 * @file nl80211.h
 *
 * @brief Header file for Public Netlink Interface
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

#ifndef __DIW_NLD11_H
#define __DIW_NLD11_H

#include "net_con.h"

#define NLD11_MAX_SUPP_RATES			32
#define NLD11_MAX_SUPP_HT_RATES			77
#define NLD11_MAX_SUPP_REG_RULES		32
#define NLD11_TKIP_DATA_OFFSET_ENCR_KEY		0
#define NLD11_TKIP_DATA_OFFSET_TX_MIC_KEY	16
#define NLD11_TKIP_DATA_OFFSET_RX_MIC_KEY	24
#define NLD11_HT_CAPABILITY_LEN			26
#define NLD11_VHT_CAPABILITY_LEN		12

#define NLD11_MAX_NR_CIPHER_SUITES		5
#define NLD11_MAX_NR_AKM_SUITES			2

#define NLD11_MIN_REMAIN_ON_CHANNEL_TIME	10

/// (virtual) interface types
/* These values are used with the %NLD11_ATTR_IFTYPE to set the type of an interface. */
enum nld11_iftype
{
	/// Unspecified type, driver decides.
	NLD11_IFTYPE_UNSPECIFIED,
	/// Independent BSS member.
	NLD11_IFTYPE_ADHOC,
	/// Managed BSS member.
	NLD11_IFTYPE_STATION,
	/// Access point.
	NLD11_IFTYPE_AP,
	/// VLAN interface for access points.
	/* VLAN interfaces are a bit special in that they must always be tied to a pre-existing AP type interface. */
	NLD11_IFTYPE_AP_VLAN,
	/// Wireless distribution interface.
	NLD11_IFTYPE_WDS,
	/// Monitor interface receiving all frames.
	NLD11_IFTYPE_MONITOR,
	/// Mesh point.
	NLD11_IFTYPE_MESH_POINT,
	/// P2P client.
	NLD11_IFTYPE_P2P_CLIENT,
	/// P2P group owner.
	NLD11_IFTYPE_P2P_GO,
	/// P2P device interface type.
	/* This is not a netdev and therefore can't be created in the normal ways,
	 * use the %NLD11_CMD_START_P2P_DEVICE
	 * and %NLD11_CMD_STOP_P2P_DEVICE commands to create and destroy one. */
	NLD11_IFTYPE_P2P_DEVICE,

	/* Keep last */
	/// Number of defined interface types.
	NUM_NLD11_IFTYPES,
	/// Highest interface type number currently defined.
	NLD11_IFTYPE_MAX = NUM_NLD11_IFTYPES - 1
};

/// Station flags.
/* When a station is added to an AP interface, it is assumed to be already associated (and hence authenticated). */
enum nld11_sta_flags
{
	/// Attribute number 0 is reserved.
	__NLD11_STA_FLAG_INVALID,
	/// Station is authorized (802.1X).
	NLD11_STA_FLAG_AUTHORIZED,
	/// Station is capable of receiving frames with short barker preamble.
	NLD11_STA_FLAG_SHORT_PREAMBLE,
	/// Station is WME/QoS capable.
	NLD11_STA_FLAG_WME,
	/// Station uses management frame protection.
	NLD11_STA_FLAG_MFP,
	/// Station is authenticated.
	NLD11_STA_FLAG_AUTHENTICATED,
	/// Station is a TDLS peer.
	/* This flag should only be used in managed mode (even in the flags mask). */
	/* Note that the flag can't be changed, it is only valid while adding a station,
	 * and attempts to change it will silently be ignored (rather than rejected as errors). */
	NLD11_STA_FLAG_TDLS_PEER,
	/// Station is associated.
	/* Used with drivers that support %NLD11_FEATURE_FULL_AP_CLIENT_STATE
	 * to transition a previously added station into associated state. */
	NLD11_STA_FLAG_ASSOCIATED,

	/* Keep last */
	/// Internal use.
	__NLD11_STA_FLAG_AFTER_LAST,
	/// Highest station flag number currently defined.
	NLD11_STA_FLAG_MAX = __NLD11_STA_FLAG_AFTER_LAST - 1
};

#define NLD11_STA_FLAG_MAX_OLD_API	NLD11_STA_FLAG_TDLS_PEER

/// Station flags mask/set.
/* Both mask and set contain bits as per &enum nld11_sta_flags. */
struct nld11_sta_flag_update
{
	/// Mask of station flags to set.
	__u32 mask;
	/// Which values to set them to.
	__u32 set;
} ;

/// Monitor configuration flags.
enum nld11_mntr_flags
{
	/// Reserved.
	__NLD11_MNTR_FLAG_INVALID,
	/// Pass frames with bad FCS.
	NLD11_MNTR_FLAG_FCSFAIL,
	/// Pass frames with bad PLCP.
	NLD11_MNTR_FLAG_PLCPFAIL,
	/// Pass control frames.
	NLD11_MNTR_FLAG_CONTROL,
	/// Disable BSSID filtering.
	NLD11_MNTR_FLAG_OTHER_BSS,
	/// Report frames after processing. Overrides all other flags.
	NLD11_MNTR_FLAG_COOK_FRAMES,
	/// Use the configured MAC address and ACK incoming unicast packets.
	NLD11_MNTR_FLAG_ACTIVE,

	/* Keep last */
	/// Internal use.
	__NLD11_MNTR_FLAG_AFTER_LAST,
	/// Highest possible monitor flag.
	NLD11_MNTR_FLAG_MAX = __NLD11_MNTR_FLAG_AFTER_LAST - 1
};

#ifdef FEATURE_UMAC_MESH
/**
 * enum nld11_plink_state - state of a mesh peer link finite state machine
 *
 * @NLD11_PLINK_LISTEN: initial state, considered the implicit
 *	state of non existant mesh peer links
 * @NLD11_PLINK_OPN_SNT: mesh plink open frame has been sent to
 *	this mesh peer
 * @NLD11_PLINK_OPN_RCVD: mesh plink open frame has been received
 *	from this mesh peer
 * @NLD11_PLINK_CNF_RCVD: mesh plink confirm frame has been
 *	received from this mesh peer
 * @NLD11_PLINK_ESTAB: mesh peer link is established
 * @NLD11_PLINK_HOLDING: mesh peer link is being closed or cancelled
 * @NLD11_PLINK_BLOCKED: all frames transmitted from this mesh
 *	plink are discarded
 * @NUM_NLD11_PLINK_STATES: number of peer link states
 * @MAX_NLD11_PLINK_STATES: highest numerical value of plink states
 */
enum nld11_plink_state {
	NLD11_PLINK_LISTEN,
	NLD11_PLINK_OPN_SNT,
	NLD11_PLINK_OPN_RCVD,
	NLD11_PLINK_CNF_RCVD,
	NLD11_PLINK_ESTAB,
	NLD11_PLINK_HOLDING,
	NLD11_PLINK_BLOCKED,

	/* keep last */
	NUM_NLD11_PLINK_STATES,
	MAX_NLD11_PLINK_STATES = NUM_NLD11_PLINK_STATES - 1
};

/**
 * enum nld11_plink_action - actions to perform in mesh peers
 *
 * @NLD11_PLINK_ACTION_NO_ACTION: perform no action
 * @NLD11_PLINK_ACTION_OPEN: start mesh peer link establishment
 * @NLD11_PLINK_ACTION_BLOCK: block traffic from this mesh peer
 * @NUM_NLD11_PLINK_ACTIONS: number of possible actions
 */
#if 0 
enum plink_actions {
	NLD11_PLINK_ACTION_NO_ACTION,
	NLD11_PLINK_ACTION_OPEN,
	NLD11_PLINK_ACTION_BLOCK,

	NUM_NLD11_PLINK_ACTIONS,
};
#endif /* 0 */

/**
 * enum nld11_mpath_flags - nld11 mesh path flags
 *
 * @NLD11_MPATH_FLAG_ACTIVE: the mesh path is active
 * @NLD11_MPATH_FLAG_RESOLVING: the mesh path discovery process is running
 * @NLD11_MPATH_FLAG_SN_VALID: the mesh path contains a valid SN
 * @NLD11_MPATH_FLAG_FIXED: the mesh path has been manually set
 * @NLD11_MPATH_FLAG_RESOLVED: the mesh path discovery process succeeded
 */
enum nld11_mpath_flags {
	NLD11_MPATH_FLAG_ACTIVE =	1<<0,
	NLD11_MPATH_FLAG_RESOLVING =	1<<1,
	NLD11_MPATH_FLAG_SN_VALID =	1<<2,
	NLD11_MPATH_FLAG_FIXED =	1<<3,
	NLD11_MPATH_FLAG_RESOLVED =	1<<4,
};

/**
 * enum nld11_mpath_info - mesh path information
 *
 * These attribute types are used with %NLD11_ATTR_MPATH_INFO when getting
 * information about a mesh path.
 *
 * @__NLD11_MESH_PATH_INFO_INVALID: attribute number 0 is reserved
 * @NLD11_MESH_PATH_INFO_FRAME_QLEN: number of queued frames for this destination
 * @NLD11_MESH_PATH_INFO_SN: destination sequence number
 * @NLD11_MESH_PATH_INFO_METRIC: metric (cost) of this mesh path
 * @NLD11_MESH_PATH_INFO_EXPTIME: expiration time for the path, in msec from now
 * @NLD11_MESH_PATH_INFO_FLAGS: mesh path flags, enumerated in
 * 	&enum nld11_mpath_flags;
 * @NLD11_MESH_PATH_INFO_DISCOVERY_TIMEOUT: total path discovery timeout, in msec
 * @NLD11_MESH_PATH_INFO_DISCOVERY_RETRIES: mesh path discovery retries
 * @NLD11_MESH_PATH_INFO_MAX: highest mesh path information attribute number
 *	currently defind
 * @__NLD11_MESH_PATH_INFO_AFTER_LAST: internal use
 */
enum nld11_mpath_info {
	__NLD11_MESH_PATH_INFO_INVALID,
	NLD11_MESH_PATH_INFO_FRAME_QLEN,
	NLD11_MESH_PATH_INFO_SN,
	NLD11_MESH_PATH_INFO_METRIC,
	NLD11_MESH_PATH_INFO_EXPTIME,
	NLD11_MESH_PATH_INFO_FLAGS,
	NLD11_MESH_PATH_INFO_DISCOVERY_TIMEOUT,
	NLD11_MESH_PATH_INFO_DISCOVERY_RETRIES,

	/* keep last */
	__NLD11_MESH_PATH_INFO_AFTER_LAST,
	NLD11_MESH_PATH_INFO_MAX = __NLD11_MESH_PATH_INFO_AFTER_LAST - 1
};

/// Mesh power save modes.
enum nld11_mesh_power_mode
{
	/// The mesh power mode of the mesh STA is not known or has not been set yet.
	NLD11_MESH_POWER_UNKNOWN,
	/// Active mesh power mode. The mesh STA is in Awake state all the time.
	NLD11_MESH_POWER_ACTIVE,
	/// Light sleep mode. The mesh STA will alternate between Active and Doze states, but will wake up for neighbor's beacons.
	NLD11_MESH_POWER_LIGHT_SLEEP,
	/// Deep sleep mode. The mesh STA will alternate between Active and Doze states, but may not wake up for neighbor's beacons.
	NLD11_MESH_POWER_DEEP_SLEEP,
	/// Internal use.
	__NLD11_MESH_POWER_AFTER_LAST,
	/// Highest possible power save level.
	NLD11_MESH_POWER_MAX = __NLD11_MESH_POWER_AFTER_LAST - 1
};

enum nld11_mesh_conf_params {
	__NLD11_MESHCONF_INVALID,
	NLD11_MESHCONF_RETRY_TIMEOUT,
	NLD11_MESHCONF_CONFIRM_TIMEOUT,
	NLD11_MESHCONF_HOLDING_TIMEOUT,
	NLD11_MESHCONF_MAX_PEER_LINKS,
	NLD11_MESHCONF_MAX_RETRIES,
	NLD11_MESHCONF_TTL,
	NLD11_MESHCONF_AUTO_OPEN_PLINKS,
	NLD11_MESHCONF_HWMP_MAX_PREQ_RETRIES,
	NLD11_MESHCONF_PATH_REFRESH_TIME,
	NLD11_MESHCONF_MIN_DISCOVERY_TIMEOUT,
	NLD11_MESHCONF_HWMP_ACTIVE_PATH_TIMEOUT,
	NLD11_MESHCONF_HWMP_PREQ_MIN_INTERVAL,
	NLD11_MESHCONF_HWMP_NET_DIAM_TRVS_TIME,
	NLD11_MESHCONF_HWMP_ROOTMODE,
	NLD11_MESHCONF_ELEMENT_TTL,
	NLD11_MESHCONF_HWMP_RANN_INTERVAL,
	NLD11_MESHCONF_GATE_ANNOUNCEMENTS,
	NLD11_MESHCONF_HWMP_PERR_MIN_INTERVAL,
	NLD11_MESHCONF_FORWARDING,
	NLD11_MESHCONF_RSSI_THRESHOLD,
	NLD11_MESHCONF_SYNC_OFFSET_MAX_NEIGHBOR,
	NLD11_MESHCONF_HT_OPMODE,
	NLD11_MESHCONF_HWMP_PATH_TO_ROOT_TIMEOUT,
	NLD11_MESHCONF_HWMP_ROOT_INTERVAL,
	NLD11_MESHCONF_HWMP_CONFIRMATION_INTERVAL,
	NLD11_MESHCONF_POWER_MODE,
	NLD11_MESHCONF_AWAKE_WINDOW,
	NLD11_MESHCONF_PLINK_TIMEOUT,

	/* keep last */
	__NLD11_MESHCONF_ATTR_AFTER_LAST,
	NLD11_MESHCONF_ATTR_MAX = __NLD11_MESHCONF_ATTR_AFTER_LAST - 1
};

enum nld11_mesh_setup_params {
	__NLD11_MESH_SETUP_INVALID,
	NLD11_MESH_SETUP_ENABLE_VENDOR_PATH_SEL,
	NLD11_MESH_SETUP_ENABLE_VENDOR_METRIC,
	NLD11_MESH_SETUP_IE,
	NLD11_MESH_SETUP_USERSPACE_AUTH,
	NLD11_MESH_SETUP_USERSPACE_AMPE,
	NLD11_MESH_SETUP_ENABLE_VENDOR_SYNC,
	NLD11_MESH_SETUP_USERSPACE_MPM,
	NLD11_MESH_SETUP_AUTH_PROTOCOL,

	/* keep last */
	__NLD11_MESH_SETUP_ATTR_AFTER_LAST,
	NLD11_MESH_SETUP_ATTR_MAX = __NLD11_MESH_SETUP_ATTR_AFTER_LAST - 1
};
#endif // FEATURE_UMAC_MESH

enum nld11_ac
{
	NLD11_AC_VO,
	NLD11_AC_VI,
	NLD11_AC_BE,
	NLD11_AC_BK,
	NLD11_NUM_ACS
};

/* Backward compat */
//#define NLD11_TXQ_ATTR_QUEUE	NLD11_TXQ_ATTR_AC
#define NLD11_TXQ_Q_VO	NLD11_AC_VO
#define NLD11_TXQ_Q_VI	NLD11_AC_VI
#define NLD11_TXQ_Q_BE	NLD11_AC_BE
#define NLD11_TXQ_Q_BK	NLD11_AC_BK

/// Channel type.
enum nld11_channel_type
{
	/// 20 MHz, non-HT channel.
	NLD11_CHAN_NO_HT,
	/// 20 MHz HT channel.
	NLD11_CHAN_HT20,
	/// HT40 channel, secondary channel below the control channel.
	NLD11_CHAN_HT40MINUS,
	/// HT40 channel, secondary channel above the control channel.
	NLD11_CHAN_HT40PLUS
};

/// Channel width definitions.
/* These values are used with the %NLD11_ATTR_CHANNEL_WIDTH attribute. */
enum nld11_chan_width
{
	/// 20 MHz, non-HT channel.
	NLD11_CHAN_WIDTH_20_NOHT,
	/// 20 MHz HT channel.
	NLD11_CHAN_WIDTH_20,
	/// 40 MHz channel, the %NLD11_ATTR_CENTER_FREQ1 attribute must be provided as well.
	NLD11_CHAN_WIDTH_40,
	/// 80 MHz channel, the %NLD11_ATTR_CENTER_FREQ1 attribute must be provided as well.
	NLD11_CHAN_WIDTH_80,
	/// 80+80 MHz channel, the %NLD11_ATTR_CENTER_FREQ1 and %NLD11_ATTR_CENTER_FREQ2 attributes must be provided as well.
	NLD11_CHAN_WIDTH_80P80,
	/// 160 MHz channel, the %NLD11_ATTR_CENTER_FREQ1 attribute must be provided as well.
	NLD11_CHAN_WIDTH_160,
	/// 5 MHz OFDM channel.
	NLD11_CHAN_WIDTH_5,
	/// 10 MHz OFDM channel.
	NLD11_CHAN_WIDTH_10,
};

/// Control channel width for a BSS.
/* These values are used with the %NLD11_BSS_CHAN_WIDTH attribute. */
enum nld11_bss_scan_width
{
	/// Control channel is 20 MHz wide or compatible.
	NLD11_BSS_CHAN_WIDTH_20,
	/// Control channel is 10 MHz wide.
	NLD11_BSS_CHAN_WIDTH_10,
	/// Control channel is 5 MHz wide.
	NLD11_BSS_CHAN_WIDTH_5,
};

/// BSS status.
/* The BSS status is a BSS attribute in scan dumps, which indicates the status the interface has wrt. this BSS. */
enum nld11_bss_status
{
	/// Nothing.
	NLD11_BSS_STATUS_NOTHING,
	/// Authenticated with this BSS.
	NLD11_BSS_STATUS_AUTHENTICATED,
	/// Associated with this BSS.
	NLD11_BSS_STATUS_ASSOCIATED,
	/// Joined to this IBSS.
	NLD11_BSS_STATUS_IBSS_JOINED,
};

/// Authentication type.
enum nld11_auth_type
{
	/// Open System authentication.
	NLD11_AUTHTYPE_OPEN_SYSTEM,
	/// Shared Key authentication (WEP only).
	NLD11_AUTHTYPE_SHARED_KEY,
	/// Fast BSS Transition (IEEE 802.11r).
	NLD11_AUTHTYPE_FT,
	/// Network EAP (some Cisco APs and mainly LEAP).
	NLD11_AUTHTYPE_NETWORK_EAP,
	/// Simultaneous authentication of equals.
	NLD11_AUTHTYPE_SAE,

	/* Keep last */
	/// Internal.
	__NLD11_AUTHTYPE_NUM,
	/// Maximum valid auth algorithm.
	NLD11_AUTHTYPE_MAX = __NLD11_AUTHTYPE_NUM - 1,
	/// Determine automatically (if necessary by trying multiple times).
	/* This is invalid in netlink - Leave out the attribute for this on CONNECT commands. */
	NLD11_AUTHTYPE_AUTOMATIC
};

/// Key type.
enum nld11_key_type
{
	/// Group (broadcast/multicast) key.
	NLD11_KEYTYPE_GROUP,
	/// Pairwise (unicast/individual) key.
	NLD11_KEYTYPE_PAIRWISE,
	/// PeerKey (DLS).
	NLD11_KEYTYPE_PEERKEY,
	/// Number of defined key types.
	NUM_NLD11_KEYTYPES
};

/// Management frame protection state.
enum nld11_mfp
{
	/// Management frame protection not used.
	NLD11_MFP_NO,
	/// Management frame protection required.
	NLD11_MFP_REQUIRED,
};

enum nld11_wpa_versions
{
	NLD11_WPA_VERSION_1 = 1 << 0,
	NLD11_WPA_VERSION_2 = 1 << 1,
};

/// Key default types.
enum nld11_key_default_types
{
	/// Invalid.
	__NLD11_KEY_DEFAULT_TYPE_INVALID,
	/// Key should be used as default unicast key.
	NLD11_KEY_DEFAULT_TYPE_UNICAST,
	/// Key should be used as default multicast key.
	NLD11_KEY_DEFAULT_TYPE_MULTICAST,
	/// Number of default types.
	NUM_NLD11_KEY_DEFAULT_TYPES
};

/// Key attributes.
enum nld11_key_attributes
{
	/// Invalid.
	__NLD11_KEY_INVALID,
	/// (temporal) Key data.
	/* For TKIP this consists of 16 bytes encryption key followed by 8 bytes each for TX and RX MIC keys. */
	NLD11_KEY_DATA,
	/// Key ID (u8, 0-3).
	NLD11_KEY_IDX,
	/// Key cipher suite (u32, as defined by IEEE 802.11 section 7.3.2.25.1, e.g. 0x000FAC04).
	NLD11_KEY_CIPHER,
	/// Transmit key sequence number (IV/PN) for TKIP and CCMP keys, each six bytes in little endian.
	NLD11_KEY_SEQ,
	/// Flag indicating default key.
	NLD11_KEY_DEFAULT,
	/// Flag indicating default management key.
	NLD11_KEY_DEFAULT_MGMT,
	/// The key type from enum nld11_key_type.
	/* If not specified the default depends on whether a MAC address was given with the command using the key or not (u32). */
	NLD11_KEY_TYPE,
	/// A nested attribute containing flags attributes, specifying what a key should be set as default as.
	/* See &enum nld11_key_default_types. */
	NLD11_KEY_DEFAULT_TYPES,

	/* Keep last */
	/// Internal.
	__NLD11_KEY_AFTER_LAST,
	/// Highest key attribute.
	NLD11_KEY_MAX = __NLD11_KEY_AFTER_LAST - 1
};

/// TX rate set attributes.
enum nld11_tx_rate_attributes
{
	/// Invalid.
	__NLD11_TXRATE_INVALID,
	/// Legacy (non-MCS) rates allowed for TX rate selection in an array of rates.
	/* Defined in IEEE 802.11 7.3.2.2 (u8 values with 1 = 500 kbps) */
	/* But without the IE length restriction (at most %NLD11_MAX_SUPP_RATES in a single array). */
	NLD11_TXRATE_LEGACY,
	/// HT (MCS) rates allowed for TX rate selection in an array of MCS numbers.
	NLD11_TXRATE_MCS,

	/* Keep last */
	/// Internal.
	__NLD11_TXRATE_AFTER_LAST,
	/// Highest TX rate attribute.
	NLD11_TXRATE_MAX = __NLD11_TXRATE_AFTER_LAST - 1
};


/// Frequency band.
enum nld11_band
{
	/// 2.4 GHz ISM band.
	NLD11_BAND_2GHZ,
	/// Around 5 GHz band (4.9 - 5.7 GHz).
	NLD11_BAND_5GHZ,
	/// Around 60 GHz band (58.32 - 64.80 GHz).
	NLD11_BAND_60GHZ,
};

/// RSSI threshold event.
enum nld11_cqm_rssi_threshold_event
{
	/// The RSSI level is lower than the configured threshold.
	NLD11_CQM_RSSI_THRESHOLD_EVENT_LOW,
	/// The RSSI is higher than the configured threshold.
	NLD11_CQM_RSSI_THRESHOLD_EVENT_HIGH,
	/// The device experienced beacon loss.
	/* Note that deauth/disassoc will still follow if the AP is not available. This event might get used as roaming event, etc. */
	NLD11_CQM_RSSI_BEACON_LOSS_EVENT,
};

/// TX power adjustment.
enum nld11_tx_power_setting
{
	/// Automatically determine transmit power.
	NLD11_TX_PWR_AUTOMATIC,
	/// Limit TX power by the mBm parameter.
	NLD11_TX_PWR_LIMITED,
	/// Fix TX power to the mBm parameter.
	NLD11_TX_PWR_FIXED,
	/// Get TX power.
	NLD11_TX_PWR_GET = 9
};

/// Values for %NLD11_ATTR_HIDDEN_SSID.
enum nld11_hidden_ssid
{
	/// Do not hide SSID (i.e., broadcast it in Beacon frames).
	NLD11_HIDDEN_SSID_NOT_IN_USE,
	/// Hide SSID by using zero-length SSID element in Beacon frames.
	NLD11_HIDDEN_SSID_ZERO_LEN,
	/// Hide SSID by using correct length of SSID element in Beacon frames but zero out each byte in the SSID.
	NLD11_HIDDEN_SSID_ZERO_CONTENTS
};

/// Device/driver features.
enum nld11_feature_flags
{
	NLD11_FEATURE_SK_TX_STATUS			= 1 << 0,
	NLD11_FEATURE_HT_IBSS				= 1 << 1,
	NLD11_FEATURE_INACTIVITY_TIMER		= 1 << 2,
	NLD11_FEATURE_CELL_BASE_REG_HINTS		= 1 << 3,
	NLD11_FEATURE_P2P_DEVICE_NEEDS_CHANNEL	= 1 << 4,
	NLD11_FEATURE_SAE				= 1 << 5,
	NLD11_FEATURE_LOW_PRIORITY_SCAN		= 1 << 6,
	NLD11_FEATURE_SCAN_FLUSH			= 1 << 7,
	NLD11_FEATURE_AP_SCAN				= 1 << 8,
	NLD11_FEATURE_VIF_TXPOWER			= 1 << 9,
	NLD11_FEATURE_NEED_OBSS_SCAN			= 1 << 10,
	NLD11_FEATURE_P2P_GO_CTWIN			= 1 << 11,
	NLD11_FEATURE_P2P_GO_OPPPS			= 1 << 12,
	/* bit 13 is reserved */
	NLD11_FEATURE_ADVERTISE_CHAN_LIMITS		= 1 << 14,
	NLD11_FEATURE_FULL_AP_CLIENT_STATE		= 1 << 15,
	NLD11_FEATURE_USERSPACE_MPM			= 1 << 16,
	NLD11_FEATURE_ACTIVE_MONITOR			= 1 << 17,
#if 1 /* 20141002 shingu.kim add */
	NLD11_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE   = 1 << 18,
#endif
};

/// Connection request failed reasons.
enum nld11_connect_failed_reason
{
	/// Maximum number of clients that can be handled by the AP is reached.
	NLD11_CONN_FAIL_MAX_CLIENTS,
	/// Connection request is rejected due to ACL.
	NLD11_CONN_FAIL_BLOCKED_CLIENT,
};

/// Scan request control flags.
/* Scan request control flags are used
 * to control the handling of NLD11_CMD_TRIGGER_SCAN and NLD11_CMD_START_SCHED_SCAN requests. */
enum nld11_scan_flags
{
	/// Scan request has low priority.
	NLD11_SCAN_FLAG_LOW_PRIORITY = 1 << 0,
	/// Flush cache before scanning.
	NLD11_SCAN_FLAG_FLUSH = 1 << 1,
	/// Force a scan even if the interface is configured as AP and the beaconing has already been configured.
	/* This attribute is dangerous because will destroy stations performance as a lot of frames will be lost while scanning off-channel,
	 * therefore it must be used only when really needed. */
	NLD11_SCAN_FLAG_AP = 1 << 2,
};

/// Access control policy.
/* Access control policy is applied on a MAC list set by %NLD11_CMD_START_AP and %NLD11_CMD_SET_MAC_ACL,
 * to be used with %NLD11_ATTR_ACL_POLICY. */
enum nld11_acl_policy
{
	/// Deny stations which are listed in ACL, i.e. allow all the stations which are not listed in ACL to authenticate.
	NLD11_ACL_POLICY_ACCEPT_UNLESS_LISTED,
	/// Allow the stations which are listed in ACL, i.e. deny all the stations which are not listed in ACL.
	NLD11_ACL_POLICY_DENY_UNLESS_LISTED,
};

#endif /* __DIW_NLD11_H */
