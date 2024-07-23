/**
 ****************************************************************************************
 *
 * @file cfg80211.h
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

#ifndef __NET_CFGD11_H
#define __NET_CFGD11_H

#include "features.h"

#include "net_con.h"

#include "nl80211.h"
#include "cfg80211.h"
#include "ieee80211.h"

#ifdef WITH_UMAC_DPM
#include "umac_util.h"
#endif


struct softmac;

/*
 * wireless hardware capability structures
 */

/// Supported frequency bands.
/* The bands are assigned this way because the supported bitrates differ in these bands. */
enum i3ed11_band
{
	/// 2.4GHz ISM band
	I3ED11_BAND_2GHZ = NLD11_BAND_2GHZ,
	//Around 5GHz band (4.9-5.7).
	//I3ED11_BAND_5GHZ = NLD11_BAND_5GHZ,

	/* Keep last */
	/// Number of defined bands.
	I3ED11_NUM_BANDS
};

/// Channel flags.
/* Channel flags set by the regulatory control code. */
enum i3ed11_channel_flags
{
	/// This channel is disabled.
	I3ED11_CHAN_DISABLED		= 1 << 0,
	/// Do not initiate radiation, this includes sending probe requests or beaconing.
	I3ED11_CHAN_NO_IR		= 1 << 1,
	/* Hole at 1<<2 */
	/// Radar detection is required on this channel.
	I3ED11_CHAN_RADAR		= 1 << 3,
	/// Extension channel above this channel is not permitted.
	I3ED11_CHAN_NO_HT40PLUS	= 1 << 4,
	/// Extension channel below this channel is not permitted.
	I3ED11_CHAN_NO_HT40MINUS	= 1 << 5,
	/// OFDM is not allowed on this channel.
	I3ED11_CHAN_NO_OFDM		= 1 << 6,
	/// If the driver supports 80 MHz on the band, this flag indicates that an 80 MHz channel cannot use this channel as the control or any of the secondary channels.
	/* This may be due to the driver or due to regulatory bandwidth restrictions. */
	I3ED11_CHAN_NO_80MHZ		= 1 << 7,
	/// If the driver supports 160 MHz on the band, this flag indicates that an 160 MHz channel cannot use this channel as the control or any of the secondary channels.
	/* This may be due to the driver or due to regulatory bandwidth restrictions. */
	I3ED11_CHAN_NO_160MHZ	= 1 << 8,
};

/// Channel definition.
/* This structure describes a single channel for use with cfgd11. */
struct i3ed11_channel
{
	/// Band this channel belongs to.
	enum i3ed11_band band;
	/// Center frequency in MHz.
	u16 cent_freq;
	/// Hardware-specific value for the channel.
	u16 hw_value;
	/// Channel flags from &enum i3ed11_channel_flags.
	u32 flags;
	/// Maximum antenna gain in dBi.
	int max_ant_gain;
	/// Maximum transmission power (in dBm).
	int max_pwr;
	/// Maximum regulatory transmission power (in dBm).
	int max_reg_pwr;
#ifdef FEATREU_MESH_SEND_BEACON_DIRECT
	//Helper to regulatory code to indicate when a beacon has been found on this channel.
	/* Use regulatory_hint_found_beacon() to enable this, this is useful only on 5 GHz band. */
	bool beacon_found;
#endif
	//Channel flags at registration time, used by regulatory code to support devices with additional restrictions.
	//u32 orig_flags;
	//Internal use.
	//int orig_mag;
	//Internal use.
	//int orig_mpwr;
};

/// Rate flags.
/* Hardware/specification flags for rates.
 * These are structured in a way that allows using the same bitrate structure for different bands/PHY modes. */
enum i3ed11_rate_flags
{
	/// Hardware can send with short preamble on this bitrate; only relevant in 2.4GHz band and with CCK rates.
	I3ED11_RATE_SHORT_PREAMBLE	= 1 << 0,
	/// This bitrate is a mandatory rate when used with 802.11a (on the 5 GHz band); filled by the core code when registering the softmac.
	I3ED11_RATE_MANDATORY_A	= 1 << 1,
	/// This bitrate is a mandatory rate when used with 802.11b (on the 2.4 GHz band); filled by the core code when registering the softmac.
	I3ED11_RATE_MANDATORY_B	= 1 << 2,
	/// This bitrate is a mandatory rate when used with 802.11g (on the 2.4 GHz band); filled by the core code when registering the softmac.
	I3ED11_RATE_MANDATORY_G	= 1 << 3,
	/// This is an ERP rate in 802.11g mode.
	I3ED11_RATE_ERP_G		= 1 << 4,
	/// Rate can be used in 5 MHz mode.
	I3ED11_RATE_SUPPORTS_5MHZ	= 1 << 5,
	/// Rate can be used in 10 MHz mode.
	I3ED11_RATE_SUPPORTS_10MHZ	= 1 << 6,
};

/// Bitrate definition.
/* This structure describes a bitrate that an 802.11 PHY can operate with.
 * The two values @hw_value and @hw_value_short are only for driver use when pointers to this structure are passed around. */
struct i3ed11_rate
{
	/// Rate-specific flags.
	u32 flags;
	/// Bitrate in units of 100 Kbps.
	u16 bitrate;
	/// Driver/hardware value for this rate.
	u16 hw_value;
	/// Driver/hardware value for this rate when short preamble is used.
	u16 hw_value_short;
};

/// STA's HT capabilities.
/* This structure describes most essential parameters needed to describe 802.11n HT capabilities for an STA. */
struct i3ed11_sta_ht_cap
{
	/// HT capabilities map as described in 802.11n spec.
	u16 cap; /* use I3ED11_HT_CAP_ */
	/// Is HT supported by the STA.
	bool ht_supported;
	/// Maximum A-MPDU length factor.
	u8 ampdu_factor;
	/// Minimum A-MPDU spacing.
	u8 ampdu_density;
	/// Supported MCS rates.
	struct i3ed11_mcs_info mcs;
};

/// Frequency band definition.
/* This structure describes a frequency band a softmac is able to operate in. */
struct i3ed11_supported_band
{
	/// Array of channels the hardware can operate in in this band.
	struct i3ed11_channel *channels;
	/// Array of bitrates the hardware can operate with in this band.
	/* Must be sorted to give a valid "supported rates" IE, i.e. CCK rates first, then OFDM. */
	struct i3ed11_rate *bitrates;
	/// The band this structure represents.
	enum i3ed11_band band;
	/// Number of channels in @channels.
	int n_channels;
	/// Number of bitrates in @bitrates.
	int n_bitrates;
	/// HT capabilities in this band.
	struct i3ed11_sta_ht_cap ht_cap;
};

/// Describes virtual interface parameters.
struct vif_params
{
	/// Use 4-address frames.
	int use_4addr;
	/// Address to use for this virtual interface.
	/* This will only be used for non-netdevice interfaces.
	 * If this parameter is set to zero address the driver may determine the address as needed. */
	u8 macaddr[DIW_ETH_ALEN];
};

/// Key information.
/* Information about a key. */
struct key_params
{
	/// Key material.
	u8 *key;
	/// Sequence counter (IV/PN) for TKIP and CCMP keys.
	/* Only used with the get_key() callback, must be in little endian, length given by @seq_len. */
	u8 *seq;
	/// Length of key material.
	int key_len;
	/// Length of @seq.
	int seq_len;
	/// Cipher suite selector.
	u32 cipher;
};

/// Channel definition.
struct cfgd11_chan_def
{
	/// The (control) channel.
	struct i3ed11_channel *chan;
	/// Channel width.
	enum nld11_chan_width width;
	/// Center frequency of first segment.
	u32 cent_freq1;
	/// Center frequency of second segment (only with 80+80 MHz).
	u32 cent_freq2;
};

#ifdef FEATURE_UMAC_MESH
/**
 ****************************************************************************************
 * @brief Return old channel type from chandef.
 * @param[in] Channel definition.
 * @return
 * The old channel type (NOHT, HT20, HT40+/-) from a given chandef,
 * which must have a bandwidth allowing this conversion.
 ****************************************************************************************
 */
static inline enum nld11_channel_type
cfgd11_get_chandef_type(const struct cfgd11_chan_def *chandef)
{
	switch (chandef->width) {
	case NLD11_CHAN_WIDTH_20_NOHT:
		return NLD11_CHAN_NO_HT;
	case NLD11_CHAN_WIDTH_20:
		return NLD11_CHAN_HT20;
	case NLD11_CHAN_WIDTH_40:
		if (chandef->cent_freq1 > chandef->chan->cent_freq)
			return NLD11_CHAN_HT40PLUS;
		return NLD11_CHAN_HT40MINUS;
	default:
//		WARN_ON(1);
		return NLD11_CHAN_NO_HT;
	}
}
#endif

/**
 ****************************************************************************************
 * @brief Create channel definition using channel type.
 * @param[in] chandef Channel definition struct to fill.
 * @param[in] channel Control channel.
 * @param[in] chantype Channel type.
 * @return void
 * @details Given a channel type, create a channel definition.
 ****************************************************************************************
 */
void cfgd11_chandef_create(struct cfgd11_chan_def *chandef,
						   struct i3ed11_channel *channel,
						   enum nld11_channel_type chantype);

/**
 ****************************************************************************************
 * @brief Check if two channel definitions are identical.
 * @param[in] chandef1 First channel definition.
 * @param[in] chandef2 Second channel definition.
 * @return True if the channels defined by the channel definitions are identical; false otherwise.
 ****************************************************************************************
 */
static inline bool
cfgd11_chandef_identical(const struct cfgd11_chan_def *chandef1,
						 const struct cfgd11_chan_def *chandef2)
{
	return (chandef1->chan == chandef2->chan &&
			chandef1->width == chandef2->width &&
			chandef1->cent_freq1 == chandef2->cent_freq1 &&
			chandef1->cent_freq2 == chandef2->cent_freq2);
}

/**
 ****************************************************************************************
 * @brief Check if two channel definitions are compatible.
 * @param[in] chandef1 First channel definition.
 * @param[in] chandef2 Second channel definition.
 * @return NULL if the given channel definitions are incompatible; chandef1 or chandef2 otherwise.
 ****************************************************************************************
 */
const struct cfgd11_chan_def *
cfgd11_chandef_compatible(const struct cfgd11_chan_def *chandef1,
						  const struct cfgd11_chan_def *chandef2);

/**
 ****************************************************************************************
 * @brief Check if a channel definition is valid.
 * @param[in] Chandef the channel definition to check.
 * @return True if the channel definition is valid; false otherwise.
 ****************************************************************************************
 */
bool cfgd11_chandef_valid(const struct cfgd11_chan_def *chandef);

/**
 ****************************************************************************************
 * @brief Check if secondary channels can be used.
 * @param[in] softmac The softmac to validate against.
 * @param[in] chandef The channel definition to check.
 * @param[in] prohibited_flags The regulatory channel flags that must not be set.
 * @return True if secondary channels are usable; false otherwise.
 ****************************************************************************************
 */
bool cfgd11_chandef_usable(struct softmac *softmac,
						   const struct cfgd11_chan_def *chandef,
						   u32 prohibited_flags);

#ifdef FEATURE_UMAC_MESH
/**
 ****************************************************************************************
 * @brief Checks if radar detection is required.
 * @param[in] softmac Softmac to validate against.
 * @param[in] chandef Channel definition to check.
 * @param[in] iftype Interface type as specified in &enum nld11_iftype.
 * @return 1 if radar detection is required, 0 if it is not, < 0 on error.
 ****************************************************************************************
 */
int cfgd11_chandef_dfs_required(struct softmac *softmac,
				  const struct cfgd11_chan_def *chandef,
				  enum nld11_iftype iftype);
#endif

/**
 ****************************************************************************************
 * @brief Returns rate flags for a channel.
 * @details In some channel types, not all rates may be used.
 * - for example CCK rates may not be used in 5/10 MHz channels.
 * @param[in] chandef Channel definition for the channel
 * @returns Rate flags which apply for this channel.
 ****************************************************************************************
 */
static inline enum i3ed11_rate_flags
i3ed11_chandef_rate_flags(struct cfgd11_chan_def *chandef)
{
	switch (chandef->width)
	{
	case NLD11_CHAN_WIDTH_5:
				return I3ED11_RATE_SUPPORTS_5MHZ;

	case NLD11_CHAN_WIDTH_10:
		return I3ED11_RATE_SUPPORTS_10MHZ;

	default:
		break;
	}

	return (enum i3ed11_rate_flags)0;
}

/**
 ****************************************************************************************
 * @brief Maximum transmission power for the chandef.
 * @details In some regulations, the transmit power may depend on the configured channel bandwidth
 * which may be defined as dBm/MHz. This function returns the actual max_pwr
 * for non-standard (20 MHz) channels.
 * @param[in] chandef Channel definition for the channel.
 * @returns Maximum allowed transmission power in dBm for the chandef.
 ****************************************************************************************
 */
static inline int
i3ed11_chandef_max_power(struct cfgd11_chan_def *chandef)
{
	switch (chandef->width)
	{
	case NLD11_CHAN_WIDTH_5:
		return min(chandef->chan->max_reg_pwr - 6,
				   chandef->chan->max_pwr);

	case NLD11_CHAN_WIDTH_10:
		return min(chandef->chan->max_reg_pwr - 3,
				   chandef->chan->max_pwr);

	default:
		break;
	}

	return chandef->chan->max_pwr;
}

#ifdef FEATURE_UMAC_MESH
/**
 ****************************************************************************************
 * @brief Update wdev channel and notify userspace.
 * @param[in] dev Device which switched channels.
 * @param[in] chandef New channel definition.
 * @details Caller must acquire wdev_lock, therefore must only be called from sleepable driver context!
 ****************************************************************************************
 */
void cfgd11_ch_switch_notify(struct net_device *dev, struct cfgd11_chan_def *chandef);

/**
 ****************************************************************************************
 * @brief Notify channel switch start.
 * @param[in] dev Device which the channel switch started.
 * @param[in] chandef Future channel definition.
 * @param[in] count Number of TBTTs until the channel switch happens.
 * @details Inform the userspace about the channel switch that has just started,
 * so that it can take appropriate actions (eg. starting channel switch on other vifs), if necessary.
 ****************************************************************************************
 */
void cfgd11_ch_switch_started_notify(struct net_device *dev,
									 struct cfgd11_chan_def *chandef,
									 u8 count);
#endif

/// Survey information flags.
/* Used by the driver to indicate which info in &struct survey_info it has filled in during the get_survey(). */
enum survey_info_flags
{
	/// Noise (in dBm) was filled in.
	SURVEY_INFO_NOISE_DBM = 1 << 0,
	/// Channel is currently being used.
	SURVEY_INFO_IN_USE = 1 << 1,
	/// Channel active time (in ms) was filled in.
	SURVEY_INFO_CHANNEL_TIME = 1 << 2,
	/// Channel busy time was filled in.
	SURVEY_INFO_CHANNEL_TIME_BUSY = 1 << 3,
	/// Extension channel busy time was filled in.
	SURVEY_INFO_CHANNEL_TIME_EXT_BUSY = 1 << 4,
	/// Channel receive time was filled in.
	SURVEY_INFO_CHANNEL_TIME_RX = 1 << 5,
	/// Channel transmit time was filled in.
	SURVEY_INFO_CHANNEL_TIME_TX = 1 << 6,
};

#ifdef UMAC_ACS_SUPPORT
/// Channel survey response.
/* Used by dump_survey() to report back per-channel survey information. */
/* This structure can later be expanded with things like channel duty cycle etc. */
struct survey_info
{
	/// The channel this survey record reports, mandatory.
	struct i3ed11_channel *channel;
	/// Amount of time in ms the radio spent on the channel.
	u64 channel_time;
	/// Amount of time the primary channel was sensed busy.
	u64 channel_time_busy;
	/// Amount of time the extension channel was sensed busy.
	u64 channel_time_ext_busy;
	/// Amount of time the radio spent receiving data.
	u64 channel_time_rx;
	/// Amount of time the radio spent transmitting data.
	u64 channel_time_tx;
	/// Bitflag of flags from &enum survey_info_flags.
	u32 filled;
	/// Channel noise in dBm. This and all following fields are optional.
	s8 noise;
};
#endif

/// Crypto settings.
struct cfgd11_crypto_settings
{
	/// Indicates which, if any, WPA versions are enabled (from enum nld11_wpa_versions).
	u32 wpa_versions;
	/// Group key cipher suite (or 0 if unset).
	u32 cipher_group;
	/// Number of AP supported unicast ciphers.
	int n_ciphers_pairwise;
	/// Unicast key cipher suites.
	u32 ciphers_pairwise[NLD11_MAX_NR_CIPHER_SUITES];
	/// Number of AKM suites.
	int n_akm_suites;
	/// AKM suites.
	u32 akm_suites[NLD11_MAX_NR_AKM_SUITES];
	/// Whether user space controls IEEE 802.1X port, i.e., sets/clears %NLD11_STA_FLAG_AUTHORIZED.
	bool control_port;
	/// The control port protocol that should be allowed through even on unauthorized ports.
	__be16 control_port_ethertype;
	/// TRUE to prevent encryption of control port protocol frames.
	bool control_port_no_encrypt;
};

/// Beacon data.
struct cfgd11_beacon_data
{
	/// Head portion of beacon (before TIM IE) or %NULL if not changed.
	const u8 *head;
	/// Tail portion of beacon (after TIM IE) or %NULL if not changed.
	const u8 *tail;
	/// Extra information element(s) to add into Beacon frames or %NULL.
	const u8 *beacon_ies;
	/// Extra information element(s) to add into Probe Response frames or %NULL.
	const u8 *proberesp_ies;
	/// Extra information element(s) to add into (Re)Association Response frames or %NULL.
	const u8 *assocresp_ies;
	/// Probe response template (AP mode only).
	const u8 *probe_resp;

	/// Length of @head.
	size_t head_len;
	/// Length of @tail.
	size_t tail_len;
	/// Length of beacon_ies in octets.
	size_t beacon_ies_len;
	/// Length of proberesp_ies in octets.
	size_t proberesp_ies_len;
	/// Length of assocresp_ies in octets.
	size_t assocresp_ies_len;
	/// Length of probe response template (@probe_resp).
	size_t probe_resp_len;
};

struct mac_address
{
	u8 addr[DIW_ETH_ALEN];
};

/// Access control list data.
struct cfgd11_acl_data
{
	/// ACL policy to be applied on the station's entry specified by mac_addr.
	enum nld11_acl_policy acl_policy;
	/// Number of MAC address entries passed.
	int n_acl_entries;

	/* Keep it last */
	/// List of MAC addresses of stations to be used for ACL.
	struct mac_address mac_addrs[];
};

/// AP configuration.
/* Used to configure an AP interface. */
struct cfgd11_ap_settings
{
	/// Defines the channel to use.
	struct cfgd11_chan_def chandef;

	/// Beacon data.
	struct cfgd11_beacon_data beacon;

	/// Beacon interval.
	int beacon_interval;
	/// DTIM period.
	int dtim_period;
	/// SSID to be used in the BSS (note: may be %NULL if not provided from user space).
	const u8 *ssid;
	/// Length of @ssid.
	size_t ssid_len;
	/// Whether to hide the SSID in Beacon/Probe Response frames.
	enum nld11_hidden_ssid hidden_ssid;
	/// Crypto settings.
	struct cfgd11_crypto_settings crypto;
	/// The BSS uses privacy.
	bool privacy;
	/// Authentication type (algorithm).
	enum nld11_auth_type auth_type;
	/// Time in seconds to determine station's inactivity.
	int inactivity_timeout;
	/// P2P CT Window.
	u8 p2p_ctw;
	/// P2P opportunistic PS.
	bool p2p_opp_ps;
	/// ACL configuration used by the drivers which has support for MAC address based access control.
	const struct cfgd11_acl_data *acl;
};

#ifdef FEATURE_UMAC_MESH
/// Channel switch settings that used for channel switch.
struct cfgd11_csa_settings {
	/// defines the channel to use after the switch
	struct cfgd11_chan_def chandef;
	/// beacon data while performing the switch
	struct cfgd11_beacon_data beacon_csa;
	/// offsets of the counters within the beacon (tail)
	const u16 *counter_offsets_beacon;
	/// offsets of the counters within the probe response
	const u16 *counter_offsets_presp;
	/// number of csa counters the beacon (tail)
	unsigned int n_counter_offsets_beacon;
	/// number of csa counters in the probe response
	unsigned int n_counter_offsets_presp;
	/// beacon data to be used on the new channel
	struct cfgd11_beacon_data beacon_after;
	/// whether radar detection is required on the new channel
	bool radar_required;
	/// whether transmissions should be blocked while changing
	bool block_tx;
	/// number of beacons until switch
	u8 count;
};
#endif

/// Station parameter values to apply.
/* Not all station parameters have in-band "no change" signalling, for those that don't these flags will are used. */
enum station_parameters_apply_mask
{
	/// Apply new uAPSD parameters (uapsd_queues, max_sp).
	STATION_PARAM_APPLY_UAPSD = BIT(0),
	/// Apply new capability.
	STATION_PARAM_APPLY_CAPABILITY = BIT(1),
	/// Apply new plink state.
	STATION_PARAM_APPLY_PLINK_STATE = BIT(2),
};

/// Station parameters.
/* Used to change and create a new station. */
struct station_parameters
{
	/// Supported rates in IEEE 802.11 format (or NULL for no change).
	const u8 *supported_rates;
	/// VLAN interface station should belong to.
	struct net_device *vlan;
	/// Station flags that changed (bitmask of BIT(NLD11_STA_FLAG_...)).
	u32 sta_flags_mask;
	/// Station flags values (bitmask of BIT(NLD11_STA_FLAG_...)).
	u32 sta_flags_set;
	/// Bitmap indicating which parameters changed (for those that don't have a natural "no change" value), see &enum station_parameters_apply_mask.
	u32 sta_modify_mask;
	/// Listen interval or -1 for no change.
	int listen_interval;
	/// AID or zero for no change.
	u16 aid;
	/// Number of supported rates.
	u8 supported_rates_len;
	/// Peer link action to take.
	u8 plink_action;
	/// Set the peer link state for a station.
	u8 plink_state;
	/// HT capabilities of station.
	const struct i3ed11_ht_cap *ht_capa;
	/// Bitmap of queues configured for uapsd.
	/* Same format as the AC bitmap in the QoS info field. */
	u8 uapsd_queues;
	/// Max Service Period.
	/* Same format as the MAX_SP in the QoS info field (but already shifted down). */
	u8 max_sp;
#ifdef FEATURE_UMAC_MESH
	/// Local link-specific mesh power save mode (no change when set to unknown).
	enum nld11_mesh_power_mode local_pm;
	/// AID of peer
	u16 peer_aid;
#endif
	/// Station capability.
	u16 capability;
	/// Extended capabilities of the station.
	const u8 *ext_capab;
	/// Number of extended capabilities.
	u8 ext_capab_len;
	/// Supported channels in IEEE 802.11 format.
	const u8 *supported_channels;
	/// Number of supported channels.
	u8 supported_channels_len;
	/// Supported oper classes in IEEE 802.11 format.
	const u8 *supported_oper_classes;
	/// Number of supported operating classes.
	u8 supported_oper_classes_len;
};

/// The type of station being modified.
enum cfgd11_station_type
{
	/// Client of an AP interface.
	CFGD11_STA_AP_CLIENT,
	/// Client of an AP interface that has the AP MLME in the device.
	CFGD11_STA_AP_MLME_CLIENT,
	/// AP station on managed interface.
	CFGD11_STA_AP_STA,
	/// IBSS station.
	CFGD11_STA_IBSS,
	/// TDLS peer on managed interface.
	/* Dummy entry while TDLS setup is in progress, it moves out of this state when being marked authorized;
	 * use this only if TDLS with external setup is supported/used. */
	CFGD11_STA_TDLS_PEER_SETUP,
	/// TDLS peer on managed interface (active entry that is operating, has been marked authorized by userspace).
	CFGD11_STA_TDLS_PEER_ACTIVE,
	/// Peer on mesh interface (kernel managed).
	CFGD11_STA_MESH_PEER_KERNEL,
	/// Peer on mesh interface (user managed).
	CFGD11_STA_MESH_PEER_USER,
};

/**
 ****************************************************************************************
 * @brief Validate parameter changes.
 * @param[in] softmac The softmac this operates on.
 * @param[in] params The new parameters for a sta.
 * @param[in] statype The type of sta being modified.
 * @details Utility function for the @change_station driver method.\n
 * Call this function with the appropriate sta type looking up the sta (and checking that it exists).\n
 * It will verify whether the sta change is acceptable, and if not will return an error code.\n
 * Note that it may modify the parameters for backward compatibility reasons,
 * so don't use them before calling this.
 ****************************************************************************************
 */
int cfgd11_check_station_change(struct softmac *softmac,
								struct station_parameters *params,
								enum cfgd11_station_type statype);

/// Station information flags.
/* Used by the driver to indicate which info in &struct station_info it has filled in during get_station() or dump_station(). */
enum station_info_flags
{
	DIW_STA_INFO_INACTIVE_TIME	= 1 << 0,
	DIW_STA_INFO_RX_BYTES		= 1 << 1,
	DIW_STA_INFO_TX_BYTES		= 1 << 2,
	DIW_STA_INFO_LLID		= 1 << 3,
	DIW_STA_INFO_PLID		= 1 << 4,
	DIW_STA_INFO_PLINK_STATE	= 1 << 5,
	DIW_STA_INFO_SIGNAL		= 1 << 6,
	DIW_STA_INFO_TX_BITRATE		= 1 << 7,
	DIW_STA_INFO_RX_PACKETS		= 1 << 8,
	DIW_STA_INFO_TX_PACKETS		= 1 << 9,
	DIW_STA_INFO_TX_RETRIES		= 1 << 10,
	DIW_STA_INFO_TX_FAILED		= 1 << 11,
	DIW_STA_INFO_RX_DROP_MISC	= 1 << 12,
	DIW_STA_INFO_SIGNAL_AVG		= 1 << 13,
	DIW_STA_INFO_RX_BITRATE		= 1 << 14,
	DIW_STA_INFO_BSS_PARAM          = 1 << 15,
	DIW_STA_INFO_CONNECTED_TIME	= 1 << 16,
	DIW_STA_INFO_ASSOC_REQ_IES	= 1 << 17,
	DIW_STA_INFO_STA_FLAGS		= 1 << 18,
	DIW_STA_INFO_BEACON_LOSS_COUNT	= 1 << 19,
	DIW_STA_INFO_T_OFFSET		= 1 << 20,
	DIW_STA_INFO_LOCAL_PM		= 1 << 21,
	DIW_STA_INFO_PEER_PM		= 1 << 22,
	DIW_STA_INFO_NONPEER_PM		= 1 << 23,
	DIW_STA_INFO_RX_BYTES64		= 1 << 24,
	DIW_STA_INFO_TX_BYTES64		= 1 << 25,
	DIW_STA_INFO_CHAIN_SIGNAL	= 1 << 26,
	DIW_STA_INFO_CHAIN_SIGNAL_AVG	= 1 << 27,
};

/// Bitrate info flags.
/* Used by the driver to indicate the specific rate transmission type for 802.11n transmissions. */
enum rate_info_flags
{
	DIW_RATE_INFO_FLAGS_MCS			= BIT(0),
	DIW_RATE_INFO_FLAGS_VHT_MCS			= BIT(1),
	DIW_RATE_INFO_FLAGS_40_MHZ_WIDTH		= BIT(2),
	DIW_RATE_INFO_FLAGS_80_MHZ_WIDTH		= BIT(3),
	DIW_RATE_INFO_FLAGS_80P80_MHZ_WIDTH		= BIT(4),
	DIW_RATE_INFO_FLAGS_160_MHZ_WIDTH		= BIT(5),
	DIW_RATE_INFO_FLAGS_SHORT_GI		= BIT(6),
	DIW_RATE_INFO_FLAGS_60G			= BIT(7),
};

/// Bitrate information.
/* Information about a receiving or transmitting bitrate. */
struct rate_info
{
	/// Bitflag of flags from &enum rate_info_flags.
	u8 flags;
	/// mcs index if struct describes a 802.11n bitrate.
	u8 mcs;
	/// Bitrate in 100kbit/s for 802.11abg.
	u16 legacy;
	/// Number of streams (VHT only).
	u8 nss;
};

/// Bitrate info flags.
/* Used by the driver to indicate the specific rate transmission type for 802.11n transmissions. */
enum bss_param_flags
{
	/// Whether CTS protection is enabled.
	DIW_BSS_PARAM_FLAGS_CTS_PROT	= 1 << 0,
	/// Whether short preamble is enabled.
	DIW_BSS_PARAM_FLAGS_SHORT_PREAMBLE	= 1 << 1,
	/// Whether short slot time is enabled.
	DIW_BSS_PARAM_FLAGS_SHORT_SLOT_TIME	= 1 << 2,
};

/// BSS parameters for the attached station.
/* Information about the currently associated BSS. */
struct sta_bss_parameters
{
	/// Bitflag of flags from &enum bss_param_flags.
	u8 flags;
	/// DTIM period for the BSS.
	u8 dtim_period;
	/// Beacon interval.
	u16 beacon_interval;
};

#define I3ED11_MAX_CHAINS	4

/// Station information.
/* Station information filled by driver for get_station() and dump_station. */
struct station_info
{
	/// Bitflag of flags from &enum station_info_flags.
	u32 filled;
	/// Time(in secs) since a station is last connected.
	u32 connected_time;
	/// Time since last station activity (tx/rx) in milliseconds.
	u32 inactive_time;
	/// Bytes received from this station.
	u64 rx_bytes;
	/// Bytes transmitted to this station.
	u64 tx_bytes;
	/// Mesh local link id.
	u16 llid;
	/// Mesh peer link id.
	u16 plid;
	/// Mesh peer link state.
	u8 plink_state;
	/// The signal strength, type depends on the softmac's signal_type. For CFGD11_SIGNAL_TYPE_MBM, value is expressed in _dBm_.
	s8 signal;
	/// Average signal strength, type depends on the softmac's signal_type. For CFGD11_SIGNAL_TYPE_MBM, value is expressed in _dBm_.
	s8 signal_avg;

	/// Bitmask for filled values in @chain_signal, @chain_signal_avg.
	u8 chains;
	/// Per-chain signal strength of last received packet in dBm.
	s8 chain_signal[I3ED11_MAX_CHAINS];
	/// Per-chain signal strength average in dBm.
	s8 chain_signal_avg[I3ED11_MAX_CHAINS];

	/// Current unicast bitrate from this station.
	struct rate_info txrate;
	/// Current unicast bitrate to this station.
	struct rate_info rxrate;
	/// Packets received from this station.
	u32 rx_packets;
	/// Packets (data) received from this station.
	u32 rx_data_packets;
	/// Packets transmitted to this station.
	u32 tx_packets;
	/// Cumulative retry counts.
	u32 tx_retries;
	/// Number of failed transmissions (retries exceeded, no ACK).
	u32 tx_failed;
	/// Dropped for un-specified reason.
	u32 rx_dropped_misc;
	/// Current BSS parameters.
	struct sta_bss_parameters bss_param;
	/// Station flags mask & values.
	struct nld11_sta_flag_update sta_flags;

	/// Generation number for nld11 dumps.
	int generation;

	/// IEs from (Re)Association Request.
	/* This is used only when in AP mode with drivers that do not use user space MLME/SME implementation.
	 * The information is provided for the cfgd11_new_sta() calls to notify user space of the IEs. */
	const u8 *assoc_req_ies;
	/// Length of assoc_req_ies buffer in octets.
	size_t assoc_req_ies_len;

	/// Number of times beacon loss event has triggered.
	u32 beacon_loss_count;

	//s64 t_offset;
	//enum nld11_mesh_power_mode local_pm;
	//enum nld11_mesh_power_mode peer_pm;
	//enum nld11_mesh_power_mode nonpeer_pm;

	/*
	 * Note: Add a new enum station_info_flags value for each new field and use it to check which fields are initialized.
	 */
};

/// Monitor flags.
/* Monitor interface configuration flags. Note that these must be the bits according to the nld11 flags. */
enum monitor_flags
{
	/// Pass frames with bad FCS.
	MONITOR_FLAG_FCSFAIL		= 1 << NLD11_MNTR_FLAG_FCSFAIL,
	/// Pass frames with bad PLCP.
	MONITOR_FLAG_PLCPFAIL		= 1 << NLD11_MNTR_FLAG_PLCPFAIL,
	/// Pass control frames.
	MONITOR_FLAG_CONTROL		= 1 << NLD11_MNTR_FLAG_CONTROL,
	/// Disable BSSID filtering.
	MONITOR_FLAG_OTHER_BSS		= 1 << NLD11_MNTR_FLAG_OTHER_BSS,
	/// Report frames after processing.
	MONITOR_FLAG_COOK_FRAMES	= 1 << NLD11_MNTR_FLAG_COOK_FRAMES,
	/// Active monitor, ACKs frames on its MAC address.
	MONITOR_FLAG_ACTIVE		= 1 << NLD11_MNTR_FLAG_ACTIVE,
};

#ifdef FEATURE_UMAC_MESH
/// mesh path information flags
/* Used by the driver to indicate which info in &struct mpath_info it has filled in during get_station() or dump_station(). */
enum mpath_info_flags {
	MESH_PATH_INFO_FRAME_QLEN		= BIT(0),
	MESH_PATH_INFO_SN			= BIT(1),
	MESH_PATH_INFO_METRIC		= BIT(2),
	MESH_PATH_INFO_EXPTIME		= BIT(3),
	MESH_PATH_INFO_DISCOVERY_TIMEOUT	= BIT(4),
	MESH_PATH_INFO_DISCOVERY_RETRIES	= BIT(5),
	MESH_PATH_INFO_FLAGS		= BIT(6),
};

/// Mesh path information.
/* Mesh path information filled by driver for get_mpath() and dump_mpath(). */
struct mpath_info
{
	/// Bitfield of flags from &enum mpath_info_flags.
	u32 filled;
	/// Number of queued frames for this destination.
	u32 frame_qlen;
	/// Target sequence number.
	u32 sn;
	/// Metric (cost) of this mesh path.
	u32 metric;
	/// Expiration time for the mesh path from now, in msecs.
	u32 exptime;
	/// Total mesh path discovery timeout, in msecs.
	u32 discovery_timeout;
	/// Mesh path discovery retries.
	u8 discovery_retries;
	/// Mesh path flags.
	u8 flags;
	/// Generation number for nld11 dumps.
	/* This number should increase every time the list of mesh paths changes, i.e.
	 * when a station is added or removed, so that userspace can tell whether it got a consistent snapshot. */
	int generation;
};
#endif

/// BSS parameters.
/* Used to change BSS parameters (mainly for AP mode). */
struct bss_parameters
{
	/// Whether to use CTS protection (0 = no, 1 = yes, -1 = do not change).
	int use_cts_prot;
	/// Whether the use of short preambles is allowed (0 = no, 1 = yes, -1 = do not change).
	int use_short_preamble;
	/// Whether the use of short slot time is allowed (0 = no, 1 = yes, -1 = do not change).
	int use_short_slot_time;
	/// Basic rates in IEEE 802.11 format (or NULL for no change).
	u8 *basic_rates;
	/// Number of basic rates.
	u8 basic_rates_len;
	/// Do not forward packets between connected stations.
	int ap_isolate;
	/// HT Operation mode (u16 = opmode, -1 = do not change).
	int ht_opmode;
	/// P2P CT Window (-1 = no change).
	s8 p2p_ctw;
	/// P2P opportunistic PS (-1 = no change).
	s8 p2p_opp_ps;
};

/// TX queue parameters.
struct i3ed11_txq_params
{
	/// AC identifier.
	enum nld11_ac ac;
	/// Maximum burst time in units of 32 usecs, 0 meaning disabled.
	u16 txop;
	/// Minimum contention window [a value of the form 2^n-1 in the range 1..32767].
	u16 cwmin;
	/// Maximum contention window [a value of the form 2^n-1 in the range 1..32767].
	u16 cwmax;
	/// Arbitration interframe space [0..255].
	u8 aifs;
};

#ifdef FEATURE_UMAC_MESH
struct mesh_config {
	u16 d11_mesh_retry_timeout;
	u16 d11_mesh_confirm_timeout;
	u16 d11_mesh_holding_timeout;
	u16 d11_mesh_max_peer_links;
	u8 d11_mesh_max_retries;
	u8 d11_mesh_ttl;
	u8 element_ttl;
	bool auto_open_plinks;
	u32 d11_mesh_nbr_offset_max_neighbor;
	u8 d11_mesh_hwmp_max_preq_retries;
	u32 path_refresh_time;
	u16 min_discovery_timeout;
	u32 d11_mesh_hwmp_active_path_timeout;
	u16 d11_mesh_hwmp_preq_min_interval;
	u16 d11_mesh_hwmp_perr_min_interval;
	u16 d11_mesh_hwmp_net_diameter_traversal_time;
	u8 d11_mesh_hwmp_root_mode;
	u16 d11_mesh_hwmp_rann_interval;
	bool d11_mesh_gate_announcement_protocol;
	bool d11_mesh_forwarding;
	s32 rssi_threshold;
	u16 ht_opmode;
	u32 d11_mesh_hwmp_active_path_to_root_timeout;
	u16 d11_mesh_hwmp_root_interval;
	u16 d11_mesh_hwmp_confirmation_interval;
	enum nld11_mesh_power_mode power_mode;
	u16 d11_mesh_awake_window_duration;
	u32 plink_timeout;
};

struct mesh_setup {
	struct cfgd11_chan_def chandef;
	const u8 *mesh_id;
	u8 mesh_id_len;
	u8 sync_method;
	u8 path_sel_proto;
	u8 path_metric;
	u8 auth_id;
	const u8 *ie;
	u8 ie_len;
	bool is_authenticated;
	bool is_secure;
	bool user_mpm;
	u8 dtim_period;
	u16 beacon_interval;
	int mcast_rate[I3ED11_NUM_BANDS];
	u32 basic_rates;
};
#endif

/// SSID description.
struct cfgd11_ssid
{
	u8 ssid[I3ED11_MAX_SSID_LEN];
	u8 ssid_len;
};

/// Scan request description.
struct cfgd11_scan_request
{
	/// SSIDs to scan for (active scan only).
	struct cfgd11_ssid *ssids;
	/// Number of SSIDs.
	int n_ssids;
	/// Total number of channels to scan.
	u32 n_channels;
	/// Channel width for scanning.
	enum nld11_bss_scan_width scan_width;
	/// Optional information element(s) to add into Probe Request or %NULL.
	const u8 *ie;
	/// Length of ie in octets.
	size_t ie_len;
	/// Bit field of flags controlling operation.
	u32 flags;
	/// Bitmap of rates to advertise for each band.
	u32 rates[I3ED11_NUM_BANDS];
	/// The wireless device to scan for.
	struct wifidev *wdev; //check releated i3ed11_wifisubif

	/* internal */
#if 1  /* 20140930 shingu.kim add */
	/// The softmac this was for.
	struct softmac *softmac;
	/// Time (in jiffies) when the scan started.
	unsigned long scan_start;
	/// (internal) Scan request was notified as aborted.
	bool aborted;
	/// (internal) Scan request was notified as done or aborted.
	bool notified;
#endif /* 20140930 shingu.kim add */
	/// Used to send probe requests at non CCK rate in 2GHz band.
	bool no_cck;

#ifdef 	FEATURE_UMAC_SCAN_WITH_BSSID /* Scan with BSSID */
	int bssid_scan_flag;
	u8 scanbssid[DIW_ETH_ALEN];
#endif

#ifdef 	FEATURE_UMAC_SCAN_WITH_DIR_SSID /* Scan with BSSID */
	int dir_ssid_scan_flag;
#endif
    int passive_scan_flag;
    int passive_duration;
	/* keep last */
	struct i3ed11_channel *channels[14];
};

/// Sets of attributes to match.
struct cfgd11_match_set
{
	/// SSID to be matched.
	struct cfgd11_ssid ssid;
};

/// Scheduled scan request description.
struct cfgd11_sched_scan_request
{
	/// SSIDs to scan for (passed in the probe_reqs in active scans).
	struct cfgd11_ssid *ssids;
	/// Number of SSIDs.
	int n_ssids;
	/// Total number of channels to scan.
	u32 n_channels;
	/// Channel width for scanning.
	enum nld11_bss_scan_width scan_width;
	/// Interval between each scheduled scan cycle.
	u32 interval;
	/// Optional information element(s) to add into Probe Request or %NULL.
	const u8 *ie;
	/// Length of ie in octets.
	size_t ie_len;
	/// Bit field of flags controlling operation.
	u32 flags;
	/// Sets of parameters to be matched for a scan result entry to be considered valid and to be passed to the host (others are filtered out).
	/* If ommited, all results are passed. */
	struct cfgd11_match_set *match_sets;
	/// Number of match sets.
	int n_match_sets;
	/// Don't report scan results below this threshold (in s32 dBm).
	s32 rssi_thold;

	/* internal */
	/// The softmac this was for.
	struct softmac *softmac;
	/// The interface.
	struct net_device *dev;
	/// Start time of the scheduled scan.
	unsigned long scan_start;

	/* keep last */
	/// Channels to scan.
	struct i3ed11_channel *channels[0];
};

/// Signal type.
enum cfgd11_signal_type
{
	/// No signal strength information available.
	CFGD11_SIGNAL_TYPE_NONE,
	/// Signal strength in mBm (100*dBm).
	CFGD11_SIGNAL_TYPE_MBM,
	/// Signal strength, increasing from 0 through 100.
	CFGD11_SIGNAL_TYPE_UNSPEC,
};

/// BSS entry IE data.
struct cfgd11_bss_ies
{
	/// TSF contained in the frame that carried these IEs.
	__u64 tsf;
	//internal use, for freeing.
	//struct rcu_head rcu_head;	//Not used.
	/// length of the IEs.
	int len;
	/// IE data.
	u8 data[];
};

/// BSS description.
/* This structure describes a BSS (which may also be a mesh network) for use in scan results and similar. */
struct cfgd11_bss
{
	/// Channel this BSS is on
	struct i3ed11_channel *channel;
	/// Width of the control channel
	enum nld11_bss_scan_width scan_width;

	/// The information elements (Note that there is no guarantee that these are well-formed!)
	const struct cfgd11_bss_ies  *ies;
	/// The information elements from the last Beacon frame
	const struct cfgd11_bss_ies  *beacon_ies;
	/// The information elements from the last Probe Response frame.
	const struct cfgd11_bss_ies  *proberesp_ies;

	/// In case this BSS struct represents a probe response from a BSS that hides the SSID in its beacon, this points to the BSS struct that holds the beacon data.
	struct cfgd11_bss *hidden_beacon_bss;

	/// Signal strength value (type depends on the softmac's signal_type).
	s32 signal;

	/// The beacon interval as from the frame.
	u16 beacon_interval;
	/// The capability field in host byte order.
	u16 capability;

	/// BSSID of the BSS.
	u8 bssid[DIW_ETH_ALEN];

	/// For priv address alignment.
	u8 padding[2];	//chang. important.
	/// Private area for driver use, has at least softmac->bss_priv_size bytes.
	u8 priv[0];     // __aligned(sizeof(void *));
	// Chang. Fix me lator.
	//struct i3ed11_bss *priv;
};

/**
 ****************************************************************************************
 * @brief Find IE with given ID.
 * @param[in] bss The bss to search.
 * @param[in] ie The IE ID.
 * @note The return value is an RCU-protected pointer,
 * so rcu_read_lock() must be held when calling this function.
 * @return NULL if not found.
 ****************************************************************************************
 */
const u8 *i3ed11_bss_get_ie(struct cfgd11_bss *bss, u8 ie);

/// Authentication request data.
struct cfgd11_auth_req
{
	/// The BSS to authenticate with, the callee must obtain a reference to it if it needs to keep it.
	struct cfgd11_bss *bss;
	/// Extra IEs to add to Authentication frame or %NULL.
	const u8 *ie;
	/// Length of ie buffer in octets.
	size_t ie_len;
	/// Authentication type (algorithm).
	enum nld11_auth_type auth_type;
	/// WEP key for shared key auth.
	const u8 *key;
	/// Length of WEP key for shared key auth.
	u8 key_len;
	/// Index of WEP key for shared key auth.
	u8 key_idx;
	/// Non-IE data to use with SAE or %NULL. This starts with Authentication transaction sequence number field.
	const u8 *sae_data;
	/// Length of sae_data buffer in octets.
	size_t sae_data_len;
};

/// Over-ride default behaviour in association.
enum cfgd11_assoc_req_flags
{
	/// Disable HT (802.11n).
	ASSOC_REQ_DISABLE_HT		= BIT(0),
	/// Disable VHT.
	ASSOC_REQ_DISABLE_VHT		= BIT(1),
};

/// (Re)Association request data.
/* This structure provides information needed to complete IEEE 802.11 (re)association. */
struct cfgd11_assoc_req
{
	/// The BSS to associate with.
	struct cfgd11_bss *bss;
	/// Extra IEs to add to (Re)Association Request frame or %NULL.
	const u8 *ie;
	/// Previous BSSID, if not %NULL use reassociate frame.
	const u8 *prev_bssid;
	/// Length of ie buffer in octets.
	size_t ie_len;
	/// Crypto settings.
	struct cfgd11_crypto_settings crypto;
	/// Use management frame protection (IEEE 802.11w) in this association.
	bool use_mfp;
	/// See &enum cfgd11_assoc_req_flags.
	u32 flags;

#ifdef FEATURE_OWE
	u8 ssid[I3ED11_MAX_SSID_LEN];
	u8 ssid_len;
#endif

	/// HT Capabilities over-rides.  Values set in ht_capa_mask will be used in ht_capa. Un-supported values will be ignored.
	struct i3ed11_ht_cap ht_capa;
	/// The bits of ht_capa which are to be used.
	struct i3ed11_ht_cap ht_capa_mask;
};

/// Deauth request data.
/* This structure provides information needed to complete IEEE 802.11 deauth. */
struct cfgd11_deauth_req
{
	/// The BSSID of the BSS to deauthenticate from.
	const u8 *bssid;
	/// Extra IEs to add to Deauth frame or %NULL.
	const u8 *ie;
	/// Length of ie buffer in octets.
	size_t ie_len;
	/// The reason code for the deauth.
	u16 reason_code;
	/// If set, change local state only and do not set a deauth frame.
	bool local_state_change;
};

/// Disassociation request data.
/* This structure provides information needed to complete IEEE 802.11 disassocation. */
struct cfgd11_disassoc_req
{
	/// The BSS to disassociate from.
	struct cfgd11_bss *bss;
	/// Extra IEs to add to Disassociation frame or %NULL.
	const u8 *ie;
	/// Length of ie buffer in octets.
	size_t ie_len;
	/// The reason code for the disassociation.
	u16 reason_code;
	/// This is a request for a local state only, i.e., no disassociation frame is to be transmitted.
	bool local_state_change;
};

/// IBSS parameters.
/* This structure defines the IBSS parameters for the join_ibss() method. */
struct cfgd11_ibss_params
{
	/// The SSID, will always be non-null.
	u8 *ssid;
	/// Fixed BSSID requested, maybe be %NULL, if set do not search for IBSSs with a different BSSID.
	u8 *bssid;
	/// Defines the channel to use if no other IBSS to join can be found.
	struct cfgd11_chan_def chandef;
	/// Information element(s) to include in the beacon.
	u8 *ie;
	/// The length of the SSID, will always be non-zero.
	u8 ssid_len;
	/// Length of that.
	u8 ie_len;
	/// Beacon interval to use.
	u16 beacon_interval;
	/// Bitmap of basic rates to use when creating the IBSS.
	u32 basic_rates;
	/// The channel should be fixed - Do not search for IBSSs to join on other channels.
	bool channel_fixed;
	/// This is a protected network, keys will be configured after joining.
	bool privacy;
	/// Whether user space controls IEEE 802.1X port, i.e., sets/clears %NLD11_STA_FLAG_AUTHORIZED.
	/* If true, the driver is required to assume that the port is unauthorized until authorized by user space. Otherwise, port is marked authorized by default. */
	bool control_port;
	/// Whether user space controls DFS operation, i.e. changes the channel when a radar is detected. This is required to operate on DFS channels.
	bool userspace_handles_dfs;
	/// Per-band multicast rate index + 1 (0: disabled).
	int mcast_rate[I3ED11_NUM_BANDS];
	/// HT Capabilities over-rides.  Values set in ht_capa_mask will be used in ht_capa. Un-supported values will be ignored.
	struct i3ed11_ht_cap ht_capa;
	/// The bits of ht_capa which are to be used.
	struct i3ed11_ht_cap ht_capa_mask;
};

/// Connection parameters.
/* This structure provides information needed to complete IEEE 802.11 auth and association. */
struct cfgd11_connect_params
{
	/// The channel to use or %NULL if not specified (auto-select based on scan results).
	struct i3ed11_channel *channel;
	/// The AP BSSID or %NULL if not specified (auto-select based on scan results).
	u8 *bssid;
	/// SSID.
	u8 *ssid;
	/// Length of ssid in octets.
	size_t ssid_len;
	/// Authentication type (algorithm).
	enum nld11_auth_type auth_type;
	/// IEs for association request.
	u8 *ie;
	/// Length of assoc_ie in octets.
	size_t ie_len;
	/// Indicates whether privacy-enabled APs should be used.
	bool privacy;
	/// Indicate whether management frame protection is used.
	enum nld11_mfp mfp;
	/// Crypto settings.
	struct cfgd11_crypto_settings crypto;
	/// WEP key for shared key auth.
	const u8 *key;
	/// Length of WEP key for shared key auth.
	u8 key_len;
	/// Index of WEP key for shared key auth.
	u8 key_idx;
	/// See &enum cfgd11_assoc_req_flags.
	u32 flags;
	/// Background scan period in seconds or -1 to indicate that default value is to be used.
	int bg_scan_period;
	/// HT Capabilities over-rides.  Values set in ht_capa_mask will be used in ht_capa.  Un-supported values will be ignored.
	struct i3ed11_ht_cap ht_capa;
	/// The bits of ht_capa which are to be used.
	struct i3ed11_ht_cap ht_capa_mask;
};

/// set_softmac_params bitfield values.
enum softmac_params_flags
{
	/// softmac->retry_short has changed.
	SOFTMAC_PARAM_RETRY_SHORT		= 1 << 0,
	/// softmac->retry_long has changed.
	SOFTMAC_PARAM_RETRY_LONG		= 1 << 1,
	/// softmac->frag_threshold has changed.
	SOFTMAC_PARAM_FRAG_THRESHOLD	= 1 << 2,
	/// softmac->rts_threshold has changed.
	SOFTMAC_PARAM_RTS_THRESHOLD	= 1 << 3,
	/// coverage class changed.
	SOFTMAC_PARAM_COVERAGE_CLASS	= 1 << 4,
};

/// Masks for bitrate control.
struct cfgd11_bitrate_mask
{
	struct
	{
		u32 legacy;
		u8 mcs[I3ED11_HT_MCS_MASK_LEN];
	} control[I3ED11_NUM_BANDS];
};

/// PMK Security Association.
/* This structure is passed to the set/del_pmksa() method for PMKSA caching. */
struct cfgd11_pmksa
{
	/// The AP's BSSID.
	u8 *bssid;
	/// The PMK material itself.
	u8 *pmkid;
};

/// Packet pattern.
/* Internal note: @mask and @pattern are allocated in one chunk of memory, free @mask only! */
struct cfgd11_pkt_pattern
{
	/// Bitmask where to match pattern and where to ignore bytes, one bit per byte, in same format as nld11.
	u8 *mask;
	/// Bytes to match where bitmask is 1.
	u8 *pattern;
	/// Length of pattern (in bytes).
	int pattern_len;
	/// Packet offset (in bytes).
	int pkt_offset;
};

/// FT IE Information.
/* This structure provides information needed to update the fast transition IE. */
struct cfgd11_update_ft_ies_params
{
	/// The Mobility Domain ID, 2 Octet value.
	u16 md;
	/// Fast Transition IEs.
	const u8 *ie;
	/// Length of ft_ie in octets.
	size_t ie_len;
};

/// mgmt tx parameters.
/* This structure provides information needed to transmit a mgmt frame. */
struct cfgd11_mgmt_tx_params
{
	/// Channel to use.
	struct i3ed11_channel *chan;
	/// Indicates wether off channel operation is required.
	bool offchan;
	/// Duration for ROC.
	unsigned int wait;
	/// Buffer to transmit.
	const u8 *buf;
	/// Buffer length.
	size_t len;
	/// Don't use cck rates for this frame.
	bool no_cck;
	/// Tells the low level not to wait for an ack.
	bool dont_wait_for_ack;
};

/*
 * Wireless hardware and networking interfaces structures and registration/helper functions.
 */

/// softmac capability flags.
enum softmac_flags
{
	/* use hole at 0 */
	/* use hole at 1 */
	/* use hole at 2 */
	/// If not set, do not allow changing the netns of this softmac at all.
	SOFTMAC_FLAG_NETNS_OK			= BIT(3),
	/// If set to true, powersave will be enabled by default.
	/* This flag will be set depending on the kernel's default on softmac_new(), but can be changed by the driver if it has a good reason to override the default. */
	SOFTMAC_FLAG_PS_ON_BY_DEFAULT		= BIT(4),
	/// Supports 4addr mode even on AP (with a single station on a VLAN interface).
	SOFTMAC_FLAG_4ADDR_AP			= BIT(5),
	/// Supports 4addr mode even as a station.
	SOFTMAC_FLAG_4ADDR_STATION		= BIT(6),
	/// This device supports setting the control port protocol ethertype. The device also honours the control_port_no_encrypt flag.
	SOFTMAC_FLAG_CONTROL_PORT_PROTOCOL	= BIT(7),
	/// The device supports IBSS RSN.
	SOFTMAC_FLAG_IBSS_RSN			= BIT(8),
	/// The device supports mesh auth by routing auth frames to userspace. See @NLD11_MESH_SETUP_USERSPACE_AUTH.
	SOFTMAC_FLAG_MESH_AUTH			= BIT(10),
	/// The device supports scheduled scans.
	SOFTMAC_FLAG_SUPPORTS_SCHED_SCAN		= BIT(11),
	/* use hole at 12 */
	/// The device supports roaming feature in the firmware.
	SOFTMAC_FLAG_SUPPORTS_FW_ROAM		= BIT(13),
	/// The device supports uapsd on AP.
	SOFTMAC_FLAG_AP_UAPSD			= BIT(14),
	/// The device supports TDLS (802.11z) operation.
	SOFTMAC_FLAG_SUPPORTS_TDLS		= BIT(15),
	/// The device does not handle TDLS (802.11z) link setup/discovery operations internally.
	SOFTMAC_FLAG_TDLS_EXTERNAL_SETUP		= BIT(16),
	/// Device integrates AP SME.
	SOFTMAC_FLAG_HAVE_AP_SME			= BIT(17),
	/// The device will report beacons from other BSSes when there are virtual interfaces in AP mode by calling cfgd11_report_obss_beacon().
	SOFTMAC_FLAG_REPORTS_OBSS			= BIT(18),
	/// When operating as an AP, the device responds to probe-requests in hardware.
	SOFTMAC_FLAG_AP_PROBE_RESP_OFFLOAD	= BIT(19),
	/// Device supports direct off-channel TX.
	SOFTMAC_FLAG_OFFCHAN_TX			= BIT(20),
	/// Device supports remain-on-channel call.
	SOFTMAC_FLAG_HAS_REMAIN_ON_CHANNEL	= BIT(21),
	/// Device supports 5 MHz and 10 MHz channels.
	SOFTMAC_FLAG_SUPPORTS_5_10_MHZ		= BIT(22),
	/// Device supports channel switch in beaconing mode (AP, IBSS, Mesh, ...).
	SOFTMAC_FLAG_HAS_CHANNEL_SWITCH		= BIT(23),
};

/// Limit on certain interface types.
struct i3ed11_iface_limit
{
	/// Maximum number of interfaces of these types.
	u16 max;
	/// Interface types (bits).
	u16 types;
};

struct i3ed11_txrx_stypes
{
	u16 tx, rx;
};

/// Wireless hardware description.
/* Chang. Optimized softmac structure. 141016 */
struct softmac
{

	/// permanent MAC address
	u8 perm_addr[DIW_ETH_ALEN];

	/// bitmasks of frame subtypes
	/* Can be subscribed to or transmitted through nld11, points to an array indexed by interface type */
	const struct i3ed11_txrx_stypes *mgmt_stypes;

	//bitmask of software interface types
	/* These are not subject to any restrictions since they are purely managed in SW. */
	//u16 software_iftypes;

	/// Supported interface modes, OR together BIT(NLD11_IFTYPE_...)
	u16 interface_modes;

	///
	u32 flags;

	/// features advertised to nld11
	u32 features;

	/// signal type reported in struct cfgd11_bss
	enum cfgd11_signal_type signal_type;

	/// each BSS struct has private data allocated with it, this variable determines its size
	int bss_priv_size;	// priv ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½ ï¿½Å¸ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½.

	/// maximum number of SSIDs the device can scan for in any given scan
	u8 max_scan_ssids;

	/// maximum number of match sets the device can handle when performing a scheduled scan, 0 if filtering is not supported.
	u8 max_match_sets;

	/// maximum length of user-controlled IEs device can add to probe request frames transmitted during a scan, must not include fixed IEs like supported rates
	u16 max_scan_ie_len;

	/// number of supported cipher suites
	int n_cipher_suites;

	/// supported cipher suites
	const u32 *cipher_suites;

	/// Retry limit for short frames (dot11ShortRetryLimit)
	u8 retry_short;

	/// Retry limit for long frames (dot11LongRetryLimit)
	u8 retry_long;

	/// Fragmentation threshold (dot11FragmentationThreshold); -1 = fragmentation disabled, only odd values >= 256 used
	u32 frag_threshold;

	/// RTS threshold (dot11RTSThreshold); -1 = RTS/CTS disabled
	u32 rts_threshold;

	//current coverage class
	//u8 coverage_class;

	u16 max_remain_on_channel_duration;

	//u8 max_num_pmkids;

	const u8 *extended_capabilities, *extended_capabilities_mask;
	u8 extended_capabilities_len;

	/// information about bands/channels supported by this device
	struct i3ed11_supported_band *bands[I3ED11_NUM_BANDS];

	// Chang, changed dev(struct dev) --> rdev(struct cfgd11_reged_dev). 141006 */
	struct cfgd11_reged_dev *rdev;

	// Chang, Added struct wifidev, 141013 */
	struct wifidev *wdev;
	struct wifidev *pwdev;

	/* protects ->resume, ->suspend sysfs callbacks against unregister hw */
	/* helps synchronize suspend/resume with softmac unregiste */
	bool registered;

	/** driver private data (sized according to softmac_new parameter) */
	/* struct i3ed11_macctrl pointerï¿½Î¼ï¿½ ï¿½ï¿½ï¿½ï¿½Ï°ï¿?ï¿½ï¿½ï¿½ï¿½. */
	const struct i3ed11_ht_cap *ht_capa_mod_mask;

	struct i3ed11_macctrl *macctrl;
#ifdef WITH_UMAC_DPM
	/// Currently this is not required
	volatile UMAC_DPM_INFO	*umacdpm;
#endif /* WITH_UMAC_DPM */
	bool enablebridge;
	//u8  cipher;	// AP Mode cipher
	u8 max_adj_channel_rssi_comp;
};

/**
 ****************************************************************************************
 * @brief Return priv from softmac.
 * @param[in] softmac The softmac whose priv pointer to return.
 * @return The priv of @softmac.
 ****************************************************************************************
 */
static inline void *softmac_priv(struct softmac *softmac)
{
	return softmac->macctrl;
}

/**
 ****************************************************************************************
 * @brief Create a new softmac for use with cfgd11.
 * @details Create a new softmac and associate the given operations with it.
 * @param[in] sizeof_priv Bytes are allocated for private use.
 * @param[in] wdev_tmp
 * @return A pointer to the new softmac.\n
 * This pointer must be assigned to each netdev's i3ed11_ptr for proper operation.
 ****************************************************************************************
 */
struct softmac *softmac_new(int sizeof_priv, struct wifidev *wdev_tmp);

/**
 ****************************************************************************************
 * @brief Register a softmac with cfgd11.
 * @param[in] softmac The softmac to register.
 * @return A non-negative softmac index or a negative error code.
 ****************************************************************************************
 */
int softmac_register(struct softmac *softmac);

/**
 ****************************************************************************************
 * @brief Deregister a softmac from cfgd11.
 * @param[in] softmac The softmac to unregister.
 * @details After this call, no more requests can be made with this priv pointer,\n
 * but the call may sleep to wait for an outstanding request that is being handled.
 ****************************************************************************************
 */
void softmac_unregister(struct softmac *softmac);

/**
 ****************************************************************************************
 * @brief Free softmac.
 * @param[in] softmac The softmac to free.
 ****************************************************************************************
 */
void softmac_free(struct softmac *softmac);

/* internal structs */
//struct cfgd11_conn;
struct cfgd11_internal_bss;
//struct cfgd11_cached_keys;

/// Wireless device state.
/* Chang. Optimized wifidev structure. 141016 */
struct wifidev
{

	/** pointer to hardware description */
	struct softmac *softmac;
	/** interface type	 */
	enum nld11_iftype iftype;
	struct net_device *netdev;
	struct diw_list_head mgmt_registrations;

	SemaphoreHandle_t mtx;

	/** indicates 4addr mode is used on this interface */
	/**  true if this is a P2P Device that has been started */
	bool use_4addr, p2p_started;

	/* The address for this device */
	//u8 address[DIW_ETH_ALEN];   // __aligned(sizeof(u16));

	/* private : currently used for IBSS and SME - might be rearranged later */
	u8 ssid[I3ED11_MAX_SSID_LEN];
	u8 ssid_len;

#ifdef FEATURE_UMAC_MESH
	//u8 mesh_id_len;
#endif

#if 0
	/** keys to set after connection is established */
	struct cfgd11_cached_keys *connect_keys;
#endif

	struct diw_list_head event_list;
	//	TX_SEMAPHORE event_lock;

	/** (private) Used by the internal configuration code */
	struct cfgd11_internal_bss *current_bss; /* associated / joined */

	/* for AP and mesh channel tracking */
	struct i3ed11_channel *channel;

	bool ps;
	int beacon_interval;
	//	bool cac_started;
};

/**
 ****************************************************************************************
 * @brief Return softmac priv from wifidev.
 * @param[in] wdev The wireless device whose softmac's priv pointer to return.
 * @return The softmac priv of @wdev.
 ****************************************************************************************
 */
static inline void *wdev_priv(struct wifidev *wdev)
{
	//BUG_ON(!wdev);
	return softmac_priv(wdev->softmac);
}

int i3ed11_ch_to_freq(int chan, enum i3ed11_band band);
int i3ed11_freq_to_ch(int freq);

/**
 ****************************************************************************************
 * @note Name indirection necessary
 * because the ieee80211 code also has a function named "diw_i3ed11_get_channel",\n
 * so if you include cfgd11's header file you get cfgd11's version,\n
 * if you try to include both header files you'll (rightfully!) get a symbol clash.
 ****************************************************************************************
 */
struct i3ed11_channel *__i3ed11_get_channel(struct softmac *softmac, int freq);

/**
 ****************************************************************************************
 * @brief Get channel struct from softmac for specified frequency.
 * @param[in] softmac The struct softmac to get the channel for.
 * @param[in] freq The center frequency of the channel.
 * @return The channel struct from @softmac at @freq.
 ****************************************************************************************
 */
static inline struct i3ed11_channel *
i3ed11_get_channel(struct softmac *softmac, int freq)
{
	return __i3ed11_get_channel(softmac, freq);
}

/**
 ****************************************************************************************
 * @brief Get basic rate for a given rate.
 * @param[in] diwband The band to look for rates in.
 * @param[in] basic_rates Bitmap of basic rates.
 * @param[in] bitrate The bitrate for which to find the basic rate.
 * @return The basic rate corresponding to a given bitrate,
 * that is the next lower bitrate contained in the basic rate map,\n
 * which is, for this function, given as a bitmap of indices of rates in the band's bitrate table.
 ****************************************************************************************
 */
struct i3ed11_rate *
i3ed11_get_response_rate(struct i3ed11_supported_band *diwband,
						 u32 basic_rates, int bitrate);

/**
 ****************************************************************************************
 * @brief Get mandatory rates for a given band.
 * @param[in] diwband The band to look for rates in.
 * @param[in] scan_width Width of the control channel.
 * @return This function returns a bitmap of the mandatory rates for the given band,\n
 * bits are set according to the rate position in the bitrates array.
 ****************************************************************************************
 */
u32 i3ed11_mandatory_rates(struct i3ed11_supported_band *diwband,
						   enum nld11_bss_scan_width scan_width);


extern const unsigned char rfc1042_header[6];
extern const unsigned char bridge_tunnel_header[6];

/**
 ****************************************************************************************
 * @brief Get header length from data.
 * @param[in] wpkt The frame.
 * @details
 * Given an wpkt with a raw 802.11 header at the data pointer this function returns the 802.11 header length.
 * @return The 802.11 header length in bytes (not including encryption headers).\n
 * Or 0 if the data in the wpktbuff is too short to contain a valid 802.11 header.
 ****************************************************************************************
 */
unsigned int i3ed11_get_hdrlen_from_wpkt(const struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief Get header length in bytes from frame control.
 * @param[in] fc Frame control field in little-endian format.
 * @return The header length in bytes.
 ****************************************************************************************
 */
//unsigned int __attribute_const__ i3ed11_hdrlen(__le16 fc);
unsigned int i3ed11_hdrlen(__le16 fc);

/**
 * DOC: Data path helpers
 *
 * In addition to generic utilities, cfgd11 also offers
 * functions that help implement the data path for devices
 * that do not do the 802.11/802.3 conversion on the device.
 */

/**
 ****************************************************************************************
 * @brief Convert an 802.11 data frame to 802.3.
 * @param[in] wpkt The 802.11 data frame.
 * @param[in] addr The device MAC address.
 * @param[in] iftype The virtual interface type.
 * @return 0 on success. Non-zero on error.
 ****************************************************************************************
 */
int i3ed11_data_to_8023(struct wpktbuff *wpkt, const u8 *addr,
						enum nld11_iftype iftype);

/**
 ****************************************************************************************
 * @brief Decode an IEEE 802.11n A-MSDU frame.
 * @details Decode an IEEE 802.11n A-MSDU frame and convert it to a list of 802.3 frames.\n
 * The @list will be empty if the decode fails. The @wpkt is consumed after the function returns.
 * @param[in] wpkt The input IEEE 802.11n A-MSDU frame.
 * @param[in] list The output list of 802.3 frames. It must be allocated and initialized by by the caller.
 * @param[in] addr The device MAC address.
 * @param[in] iftype The device interface type.
 * @param[in] extra_headroom The hardware extra headroom for SKBs in the @list.
 * @param[in] has_80211_header Set it true if SKB is with IEEE 802.11 header.
 ****************************************************************************************
 */
void i3ed11_amsdu_to_8023s(struct wpktbuff *wpkt, struct wpktbuff_head *list,
						   const u8 *addr, enum nld11_iftype iftype,
						   const unsigned int extra_headroom,
						   bool has_80211_header);

/**
 ****************************************************************************************
 * @brief Determine the 802.1p/1d tag for a data frame.
 * @param[in] wpkt The data frame.
 * @return The 802.1p/1d tag.
 ****************************************************************************************
 */
unsigned int cfgd11_classify8021d(struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief Find information element in data.
 * @param[in] eid Element ID.
 * @param[in] ies Data consisting of IEs.
 * @param[in] len Length of data.
 * @return NULL if the element ID could not be found or if the element is invalid
 * (claims to be longer than the given data),\n
 * or a pointer to the first byte of the requested element, that is the byte containing the element ID.
 * @note There are no checks on the element length other than having to fit into the given data.
 ****************************************************************************************
 */
const u8 *cfgd11_find_ie(u8 eid, const u8 *ies, int len);

/**
 ****************************************************************************************
 * @brief Find vendor specific information element in data.
 * @param[in] oui Vendor OUI.
 * @param[in] oui_type Vendor-specific OUI type.
 * @param[in] ies Data consisting of IEs.
 * @param[in] len Length of data.
 * @return NULL if the vendor specific element ID could not be found or if the element is invalid
 * (claims to be longer than the given data),\n
 * or a pointer to the first byte of the requested element, that is the byte containing the element ID.
 * @note There are no checks on the element length other than having to fit into the given data.
 ****************************************************************************************
 */
const u8 *cfgd11_find_vendor_ie(unsigned int oui, u8 oui_type,
								const u8 *ies, int len);

/*
 * Callbacks for asynchronous cfgd11 methods, notification functions and BSS handling helpers.
 */

/**
 ****************************************************************************************
 * @brief Noti that scan finished.
 * @param[in] request The corresponding scan request.
 * @param[in] aborted Set to true if the scan was aborted for any reason, userspace will be notified of that.
 ****************************************************************************************
 */
void cfgd11_scan_done(struct cfgd11_scan_request *request, bool aborted);

/**
 ****************************************************************************************
 * @brief Noti that new scan results are available.
 * @param[in] softmac The softmac which got scheduled scan results.
 ****************************************************************************************
 */
void cfgd11_sched_scan_results(struct softmac *softmac);

/**
 ****************************************************************************************
 * @brief Inform cfgd11 of a received BSS frame.
 * @param[in] softmac The softmac reporting the BSS.
 * @param[in] channel The channel the frame was received on.
 * @param[in] scan_width Width of the control channel.
 * @param[in] mgmt The management frame (probe response or beacon).
 * @param[in] len Length of the management frame.
 * @param[in] signal The signal strength, type depends on the softmac's signal_type.
 * @param[in] gfp Context flags.
 * @details This informs cfgd11 that BSS information was found and the BSS should be updated/added.
 * @return A referenced struct, must be released with cfgd11_put_bss()! Or %NULL on error.
 ****************************************************************************************
 */
struct cfgd11_bss *cfgd11_inform_bss_width_frame(struct softmac *softmac,
		struct i3ed11_channel *channel,
		enum nld11_bss_scan_width scan_width,
		struct i3ed11_mgmt *mgmt, size_t len,
		s32 signal, gfp_t gfp);

struct cfgd11_bss *cfgd11_get_bss(struct softmac *softmac,
								  struct i3ed11_channel *channel,
								  const u8 *bssid,
								  const u8 *ssid, size_t ssid_len,
								  u16 capa_mask, u16 capa_val);

/**
 ****************************************************************************************
 * @brief Reference BSS struct.
 * @param[in] softmac The softmac this BSS struct belongs to.
 * @param[in] bss The BSS struct to reference.
 * @details Increments the refcount of the given BSS struct.
 ****************************************************************************************
 */
void cfgd11_ref_bss(struct softmac *softmac, struct cfgd11_bss *bss);

/**
 ****************************************************************************************
 * @brief Unref BSS struct.
 * @param[in] softmac The softmac this BSS struct belongs to.
 * @param[in] bss The BSS struct.
 * @details Decrements the refcount of the given BSS struct.
 ****************************************************************************************
 */
void cfgd11_put_bss(struct softmac *softmac, struct cfgd11_bss *bss);

/**
 ****************************************************************************************
 * @brief Unlink BSS from internal data structures.
 * @param[in] softmac The softmac.
 * @param[in] bss The bss to remove.
 * @details This function removes the given BSS
 * from the internal data structures thereby making it no longer show up in scan results etc.\n
 * Use this function when you detect a BSS is gone.\n
 * Normally BSSes will also time out, so it is not necessary to use this function at all.
 ****************************************************************************************
 */
void cfgd11_unlink_bss(struct softmac *softmac, struct cfgd11_bss *bss);

/**
 ****************************************************************************************
 * @brief Noti of processed MLME management frame.
 * @param[in] dev Network device.
 * @param[in] buf Authentication frame (header + body).
 * @param[in] len Length of the frame data.
 * @details This function is called whenever an auth, disassociation or deauth frame
 * has been received and processed in sta mode.\n
 * After being asked to authenticate
 * via cfgd11_ops::auth() the driver must call either this function or diw_cfgd11_auth_timeout().\n
 * After being asked to associate
 * via cfgd11_ops::assoc() the driver must call either this function or diw_cfgd11_auth_timeout().\n
 * While connected,
 * the driver must calls this for received and processed disassociation and deauth frames.\n
 * If the frame couldn't be used because it was unprotected,
 * the driver must call the function diw_cfgd11_rx_unprot_mlme_mgmt() instead.\n\n
 * This function may sleep. The caller must hold the corresponding wdev's mutex.
 ****************************************************************************************
 */
void cfgd11_rx_mlme_mgmt(struct net_device *dev, const u8 *buf, size_t len);

/**
 ****************************************************************************************
 * @brief Noti of timed out auth.
 * @param[in] dev Network device.
 * @param[in] addr MAC address with which the auth timed out.
 ****************************************************************************************
 */
void cfgd11_auth_timeout(struct net_device *dev, const u8 *addr);

/**
 ****************************************************************************************
 * @brief Noti of processed assoc response.
 * @param[in] dev Network device.
 * @param[in] bss The BSS that assoc was requested with,
 * ownership of the pointer moves to cfg80211 in this call.
 * @param[in] buf Authentication frame (header + body).
 * @param[in] len Length of the frame data.
 ****************************************************************************************
 */
void cfgd11_rx_assoc_resp(struct net_device *dev,
						  struct cfgd11_bss *bss,
						  const u8 *buf, size_t len);

/**
 ****************************************************************************************
 * @brief Noti of timed out assoc.
 * @param[in] dev Network device.
 * @param[in] bss The BSS entry with which assoc timed out.
 * @details This function may sleep. The caller must hold the corresponding wdev's mutex.
 ****************************************************************************************
 */
void cfgd11_assoc_timeout(struct net_device *dev, struct cfgd11_bss *bss);


void cfgd11_mlme_purge_registrations(struct wifidev *wdev);

/**
 ****************************************************************************************
 * @brief Noti of transmitted deauth/disassoc frame.
 * @param[in] dev Network device.
 * @param[in] buf Frame header + body.
 * @param[in] len Length of the frame data.
 * @details This function is called whenever deauth has been processed in sta mode.\n
 * This includes both received deauth frames and locally generated ones.\n\n
 * This function may sleep. The caller must hold the corresponding wdev's mutex.
 ****************************************************************************************
 */
void cfgd11_tx_mlme_mgmt(struct net_device *dev, const u8 *buf, size_t len);

/**
 ****************************************************************************************
 * @brief Noti of mlme mgmt frame (unprotected).
 * @param[in] dev Network device.
 * @param[in] buf Deauth frame (header + body).
 * @param[in] len Length of the frame data.
 ****************************************************************************************
 */
void cfgd11_rx_unprot_mlme_mgmt(struct net_device *dev,
								const u8 *buf, size_t len);

/**
 ****************************************************************************************
 * @brief Noti of Michael MIC failure (TKIP).
 * @param[in] dev Network device.
 * @param[in] addr MAC address (Source).
 * @param[in] key_type Key type (received frame used).
 * @param[in] key_id Key identifier (0..3). Can be -1 if missing.
 * @param[in] tsc TSC value that generated the MIC failure (6 octets).
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_michael_mic_failure(struct net_device *dev, const u8 *addr,
								enum nld11_key_type key_type, int key_id,
								const u8 *tsc, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti cfgd11 of a new mesh peer candidate.
 * @param[in] dev Network device.
 * @param[in] macaddr The MAC address of the new candidate.
 * @param[in] ie Information elements advertised by the peer candidate.
 * @param[in] ie_len Lenght of the information elements buffer.
 * @param[in] gfp Allocation flags.
 * @details This function notifies cfgd11 that the mesh peer candidate has been detected,
 * most likely via a beacon or, less likely, via a probe response.\n
 * cfgd11 then sends a notification to userspace.
 ****************************************************************************************
 */
void cfgd11_notify_new_peer_candidate(struct net_device *dev,
									  const u8 *macaddr, const u8 *ie, u8 ie_len, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti cfgd11 about hw block state.
 * @param[in] softmac The softmac.
 * @param[in] blocked Block status.
 ****************************************************************************************
 */
void softmac_rfkill_set_hw_state(struct softmac *softmac, bool blocked);

/**
 ****************************************************************************************
 * @brief Start polling rfkill.
 * @param[in] softmac The softmac.
 ****************************************************************************************
 */
void softmac_rfkill_start_polling(struct softmac *softmac);

/**
 ****************************************************************************************
 * @brief Stop polling rfkill.
 * @param[in] softmac The softmac.
 ****************************************************************************************
 */
void softmac_rfkill_stop_polling(struct softmac *softmac);

#ifdef CONFIG_NL80211_TESTMODE
/**
 ****************************************************************************************
 * @brief Allocate testmode reply.
 * @param[in] softmac The softmac.
 * @param[in] approxlen An upper bound of the length of the data that will be put into the wpkt.
 * @details This function allocates and pre-fills an wpkt for a reply to the testmode command.\n
 * Since it is intended for a reply, calling it outside of the @testmode_cmd operation is invalid.\n\n
 * The returned wpkt is pre-filled with the softmac index and set up in a way that any data
 * that is put into the wpkt (with wpkt_put(), nla_put() or similar)
 * will end up being within the %NLD11_ATTR_TESTDATA attribute,
 * so all that needs to be done with the wpkt is adding data for the corresponding userspace tool
 * which can then read that data out of the testdata attribute.\n
 * You must not modify the wpkt in any other way.\n\n
 * When done, call cfgd11_testmode_reply() with the wpkt and return its error code
 * as the result of the @testmode_cmd operation.
 * @return An allocated and pre-filled wpkt. %NULL if any errors happen.
 ****************************************************************************************
 */
struct wpktbuff *cfgd11_testmode_alloc_reply_wpkt(struct softmac *softmac,
		int approxlen);

/**
 ****************************************************************************************
 * @brief Send the reply wpkt.
 * @param[in] wpkt The wpkt, must have been allocated with cfgd11_testmode_alloc_reply_wpkt().
 * @details Since calling this function will usually be the last thing before returning
 * from the @testmode_cmd you should return the error code.\n
 * Note that this function consumes the wpkt regardless of the return value.
 * @return An error code or 0 on success.
 ****************************************************************************************
 */
int cfgd11_testmode_reply(struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief Allocate testmode event.
 * @param[in] softmac The softmac.
 * @param[in] approxlen: an upper bound of the length of the data that will be put into the wpkt.
 * @param[in] gfp Allocation flags.
 * @details This function allocates and pre-fills an wpkt for an event on the testmode multicast group.\n\n
 * The returned wpkt is set up in the same way as with cfgd11_testmode_alloc_reply_wpkt()
 * but prepared for an event.\n
 * As there, you should simply add data to it that will then end up in the %NLD11_ATTR_TESTDATA attribute.
 * Again, you must not modify the wpkt in any other way.\n\n
 * When done filling the wpkt, call cfgd11_testmode_event() with the wpkt to send the event.
 * @return An allocated and pre-filled wpkt. %NULL if any errors happen.
 ****************************************************************************************
 */
struct wpktbuff *cfgd11_testmode_alloc_event_wpkt(struct softmac *softmac,
		int approxlen, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Send the event.
 * @param[in] wpkt The wpkt, must have been allocated with cfgd11_testmode_alloc_event_wpkt().
 * @param[in] gfp Allocation flags.
 * @details This function sends the given @wpkt,
 * which must have been allocated by cfgd11_testmode_alloc_event_wpkt(), as an event.\n
 * It always consumes it.
 ****************************************************************************************
 */
void cfgd11_testmode_event(struct wpktbuff *wpkt, gfp_t gfp);

#define CFGD11_TESTMODE_CMD(cmd)	.testmode_cmd = (cmd),
#define CFGD11_TESTMODE_DUMP(cmd)	.testmode_dump = (cmd),
#else
#define CFGD11_TESTMODE_CMD(cmd)
#define CFGD11_TESTMODE_DUMP(cmd)
#endif

/**
 ****************************************************************************************
 * @brief Noti cfgd11 of connection result.
 * @param[in] dev Network device.
 * @param[in] bssid The BSSID of the AP.
 * @param[in] req_ie Association request IEs (maybe be %NULL).
 * @param[in] req_ie_len Association request IEs length.
 * @param[in] resp_ie Association response IEs (may be %NULL).
 * @param[in] resp_ie_len Assoc response IEs length.
 * @param[in] status Status code, 0 for successful connection,
 * use %DIW_STATUS_UNSPECIFIED_FAILURE if your device cannot give you the real status code for failures.
 * @param[in] gfp Allocation flags.
 * @details It should be called by the underlying driver whenever connect() has succeeded.
 ****************************************************************************************
 */
void cfgd11_connect_result(struct net_device *dev, const u8 *bssid,
						   const u8 *req_ie, size_t req_ie_len,
						   const u8 *resp_ie, size_t resp_ie_len,
						   u16 status, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti cfgd11 of roaming.
 * @param[in] dev Network device.
 * @param[in] channel The channel of the new AP.
 * @param[in] bssid The BSSID of the new AP.
 * @param[in] req_ie Association request IEs (maybe be %NULL).
 * @param[in] req_ie_len Association request IEs length.
 * @param[in] resp_ie Association response IEs (may be %NULL).
 * @param[in] resp_ie_len Assoc response IEs length.
 * @param[in] gfp Allocation flags.
 * @details It should be called by the underlying driver
 * whenever it roamed from one AP to another while connected.
 ****************************************************************************************
 */
void cfgd11_roamed(struct net_device *dev,
				   struct i3ed11_channel *channel,
				   const u8 *bssid,
				   const u8 *req_ie, size_t req_ie_len,
				   const u8 *resp_ie, size_t resp_ie_len, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti cfgd11 of roaming.
 * @param[in] dev Network device.
 * @param[in] bss Entry of bss to which STA got roamed.
 * @param[in] req_ie Association request IEs (maybe be %NULL).
 * @param[in] req_ie_len Association request IEs length.
 * @param[in] resp_ie Association response IEs (may be %NULL).
 * @param[in] resp_ie_len Assoc response IEs length.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_roamed_bss(struct net_device *dev, struct cfgd11_bss *bss,
					   const u8 *req_ie, size_t req_ie_len,
					   const u8 *resp_ie, size_t resp_ie_len, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti cfgd11 that connection was dropped.
 * @param[in] dev Network device.
 * @param[in] ie Information elements of the deauth/disassoc frame (may be %NULL).
 * @param[in] ie_len Length of IEs.
 * @param[in] reason Reason code for the disconnection, set it to 0 if unknown.
 * @param[in] gfp Allocation flags.
 * @details After it calls this function,
 * the driver should enter an idle state and not try to connect to any AP any more.
 ****************************************************************************************
 */
void cfgd11_disconnected(struct net_device *dev, u16 reason,
						 u8 *ie, size_t ie_len, gfp_t gfp);

void cfgd11_ready_on_channel(struct wifidev *wdev, ULONG cookie,
							 struct i3ed11_channel *chan,
							 unsigned int duration, gfp_t gfp);

void cfgd11_remain_on_channel_expired(struct wifidev *wdev, ULONG cookie,
									  struct i3ed11_channel *chan,
									  gfp_t gfp);


/**
 ****************************************************************************************
 * @brief Noti userspace about station (new).
 * @param[in] dev Net device.
 * @param[in] mac_addr Address of station.
 * @param[in] sinfo The station information.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_new_sta(struct net_device *dev, const u8 *mac_addr,
					struct station_info *sinfo, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti userspace about station (delete).
 * @param[in] dev Net device.
 * @param[in] mac_addr Address of station.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_del_sta(struct net_device *dev, const u8 *mac_addr, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Connection request failed notification.
 * @param[in] dev Net device.
 * @param[in] mac_addr Address of station.
 * @param[in] reason The reason for connection failure.
 * @param[in] gfp Allocation flags.
 * @details Whenever a station tries to connect to an AP and if the station could not connect to the AP
 * as the AP has rejected the connection for some reasons, this function is called.\n\n
 * The reason for connection failure can be any of the value from nld11_connect_failed_reason enum.
 ****************************************************************************************
 */
void cfgd11_conn_failed(struct net_device *dev, const u8 *mac_addr,
						enum nld11_connect_failed_reason reason,
						gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti of received, unprocessed management frame.
 * @param[in] wdev Wireless device receiving the frame.
 * @param[in] freq Frequency on which the frame was received in MHz.
 * @param[in] sig_dbm Signal strength in mBm, or 0 if unknown.
 * @param[in] buf Management frame (header + body).
 * @param[in] len Length of the frame data.
 * @param[in] flags Flags, as defined in enum nld11_rxmgmt_flags.
 * @param[in] gfp Context flags.
 * @details This function is called whenever an Action frame is received for a station mode interface,
 * but is not processed in kernel.
 * @return %true if a user space application has registered for this frame.\n
 * For action frames, that makes it responsible for rejecting unrecognized action frames; %false otherwise,\n
 * in which case for action frames the driver is responsible for rejecting the frame.
 ****************************************************************************************
 */
bool cfgd11_rx_mgmt(struct wifidev *wdev, int freq, int sig_dbm,
					const u8 *buf, size_t len, u32 flags, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti of TX status for management frame.
 * @param[in] wdev Wireless device receiving the frame.
 * @param[in] cookie Cookie returned by cfgd11_ops::mgmt_tx().
 * @param[in] buf Management frame (header + body).
 * @param[in] len Length of the frame data.
 * @param[in] ack Whether frame was acknowledged.
 * @param[in] gfp Context flags.
 * @details This function is called whenever a management frame was requested to be transmitted
 * with cfgd11_ops::mgmt_tx() to report the TX status of the transmission attempt.
 ****************************************************************************************
 */
void cfgd11_mgmt_tx_status(struct wifidev *wdev, ULONG cookie,
						   const u8 *buf, size_t len, bool ack, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Connection quality monitoring rssi event.
 * @param[in] dev Net device.
 * @param[in] rssi_event The triggered RSSI event.
 * @param[in] gfp Context flags.
 * @details This function is called
 * when a configured connection quality monitoring rssi threshold reached event occurs.
 ****************************************************************************************
 */
void cfgd11_cqm_rssi_notify(struct net_device *dev,
							enum nld11_cqm_rssi_threshold_event rssi_event,
							gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti userspace about packetloss to peer.
 * @param[in] dev Net device.
 * @param[in] peer Peer's MAC address.
 * @param[in] num_packets How many packets were lost
 * - should be a fixed threshold but probably no less than maybe 50,
 * or maybe a throughput dependent threshold (to account for temporary interference)
 * @param[in] gfp Context flags.
 ****************************************************************************************
 */
void cfgd11_cqm_pktloss_notify(struct net_device *dev,
							   const u8 *peer, u32 num_packets, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief TX error rate event.
 * @param[in] dev Net device.
 * @param[in] peer Peer's MAC address.
 * @param[in] num_packets How many packets were lost.
 * @param[in] rate % of packets which failed transmission.
 * @param[in] intvl Interval (in s) over which the TX failure threshold was breached.
 * @param[in] gfp Context flags.
 * @details Noti userspace when configured % TX failures over number of packets
 * in a given interval is exceeded.
 ****************************************************************************************
 */
void cfgd11_cqm_txe_notify(struct net_device *dev, const u8 *peer,
						   u32 num_packets, u32 rate, u32 intvl, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti userspace about driver rekeying.
 * @param[in] dev Net device.
 * @param[in] bssid BSSID of AP (to avoid races).
 * @param[in] replay_ctr New replay counter.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_gtk_rekey_notify(struct net_device *dev, const u8 *bssid,
							 const u8 *replay_ctr, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti about PMKSA caching candidate.
 * @param[in] dev Net device.
 * @param[in] index Candidate index.
 * @param[in] bssid BSSID of AP.
 * @param[in] preauth Whether AP advertises support for RSN pre-auth.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_pmksa_candidate_notify(struct net_device *dev, int index,
								   const u8 *bssid, bool preauth, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Inform userspace about a spurious frame.
 * @param[in] dev The device the frame matched to.
 * @param[in] addr The transmitter address.
 * @param[in] gfp Context flags.
 * @details This function is used in AP mode (only!) to inform userspace.
 * that a spurious class 3 frame was received, to be able to deauth the sender.
 * @return %true if the frame was passed to userspace
 * (or this failed for a reason other than not having a subscription).
 ****************************************************************************************
 */
bool cfgd11_rx_spurious_frame(struct net_device *dev,
							  u8 *addr, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Inform about unexpected WDS frame.
 * @param[in] dev The device the frame matched to.
 * @param[in] addr The transmitter address.
 * @param[in] gfp Context flags.
 * @details This function is used in AP mode (only!) to inform userspace
 * that an associated station sent a 4addr frame but that wasn't expected.\n
 * It is allowed and desirable to send this event only once for each station to avoid event flooding.
 * @return %true if the frame was passed to userspace
 * (or this failed for a reason other than not having a subscription).
 ****************************************************************************************
 */
bool cfgd11_rx_unexpected_4addr_frame(struct net_device *dev,
									  const u8 *addr, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Noti userspace about probe status.
 * @param[in] dev The device the probe was sent on.
 * @param[in] addr The address of the peer.
 * @param[in] cookie The cookie filled in @probe_client previously.
 * @param[in] acked Indicates whether probe was acked or not.
 * @param[in] gfp Allocation flags.
 ****************************************************************************************
 */
void cfgd11_probe_status(struct net_device *dev, const u8 *addr,
						 ULONG cookie, bool acked, gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Report beacon from other APs.
 * @param[in] softmac The softmac that received the beacon.
 * @param[in] frame The frame.
 * @param[in] len Length of the frame.
 * @param[in] freq Frequency the frame was received on.
 * @param[in] sig_dbm Signal strength in mBm, or 0 if unknown.
 * @details Use this function to report to userspace when a beacon was received.
 * It is not useful to call this when there is no netdev that is in AP/GO mode.
 ****************************************************************************************
 */
void cfgd11_report_obss_beacon(struct softmac *softmac,
							   const u8 *frame, size_t len,
							   int freq, int sig_dbm);

#if 0
void cfgd11_ch_switch_notify(struct net_device *dev, struct cfgd11_chan_def *chandef);
#endif

/**
 ****************************************************************************************
 * @brief Convert operating class to band.
 * @param[in] operating_class The operating class to convert.
 * @param[in] band Band pointer to fill.
 * @returns %true if the conversion was successful, %false otherwise.
 ****************************************************************************************
 */
bool i3ed11_operating_class_to_band(u8 operating_class,
									enum i3ed11_band *band);

/*
 ****************************************************************************************
 * @brief Calculate actual bitrate (in 100Kbps units).
 * @param[in] rate Given rate_info to calculate bitrate from.
 * @return 0 if MCS index >= 32.
 ****************************************************************************************
 */
u32 cfgd11_calculate_bitrate(struct rate_info *rate);

/**
 ****************************************************************************************
 * @brief Remove the given wdev.
 * @param[in] wdev Struct wifidev to remove.
 ****************************************************************************************
 */
void cfgd11_unregister_wdev(struct wifidev *wdev);

int cfgd11_get_p2p_attr(const u8 *ies, unsigned int len,
						enum i3ed11_p2p_attr_id attr,
						u8 *buf, unsigned int bufsize);

#endif /* __NET_CFGD11_H */
