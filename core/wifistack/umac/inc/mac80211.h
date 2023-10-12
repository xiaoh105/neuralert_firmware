/**
 ****************************************************************************************
 *
 * @file mac80211.h
 *
 * @brief Header file macd11 and Driver Interface
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

#ifndef MACD11_H
#define MACD11_H

//#include "skbuff.h"
#include "net_con.h"
#include "le_byteshift.h"
//#include "os_con.h"

#include "cfg80211.h"
#include "ieee80211.h"

#pragma GCC diagnostic ignored "-Wunused-function"

struct device;

/// Maximum number of queues.
enum i3ed11_max_queues
{
	/// Maximum number of regular device queues.
	/* We use only 8 queues, for opt 16 -> 8 */
	I3ED11_MAX_QUEUES = 8,
	/// Bitmap with maximum queues set.
	I3ED11_MAX_QUEUE_MAP = BIT(I3ED11_MAX_QUEUES) - 1,
};

#define I3ED11_INVAL_HW_QUEUE	0xff

/// AC numbers as used in macd11.
enum i3ed11_ac_numbers
{
	/// Voice
	I3ED11_AC_VO		= 0,
	/// Video
	I3ED11_AC_VI		= 1,
	/// Best Effort
	I3ED11_AC_BE		= 2,
	/// Background
	I3ED11_AC_BK		= 3,
};

#define I3ED11_NUM_ACS	4

/// Transmit queue configuration.
/* The information provided in this structure is required for QoS transmit queue configuration. */
/* Cf. IEEE 802.11 7.3.2.29. */
struct i3ed11_tx_queue_params
{
	/// Maximum burst time in units of 32 usecs, 0 meaning disabled.
	u16 txop;
	/// Minimum contention window [a value of the form 2^n-1 in the range 1..32767].
	u16 cwmin;
	/// Maximum contention window [like @cw_min].
	u16 cwmax;
	/// Arbitration interframe space [0..255].
	u8 aifs;
	/// Is mandatory admission control required for the access category.
	bool acm;
	/// Is U-APSD mode enabled for the queue.
	bool uapsd;
};

/// Change flag for channel context.
enum i3ed11_chanctx_change
{
	/// The channel width changed.
	I3ED11_CHANCTX_CHANGE_WIDTH		= BIT(0),
	/// The number of RX chains changed.
	I3ED11_CHANCTX_CHANGE_RX_CHAINS	= BIT(1),
	/// Radar detection flag changed.
	I3ED11_CHANCTX_CHANGE_RADAR		= BIT(2),
	/// Switched to another operating channel, this is used only with channel switching with CSA.
	I3ED11_CHANCTX_CHANGE_CHANNEL	= BIT(3),
	/// The min required channel width changed.
	I3ED11_CHANCTX_CHANGE_MIN_WIDTH	= BIT(4),
};

/// Channel context that vifs may be tuned to.
/* This is the driver-visible part. The i3ed11_chanctx that contains it is visible in macd11 only. */
struct i3ed11_chanctx_conf
{
	/// The channel definition.
	struct cfgd11_chan_def def;
	/// The minimum channel definition currently required.
	struct cfgd11_chan_def min_def;
	/// The number of RX chains that must always be active on the channel to receive MIMO transmissions.
	u8 rx_chains_static;
	/// The number of RX chains that must be enabled after RTS/CTS handshake to receive SMPS MIMO transmissions; this will always be >= @rx_chains_static.
	u8 rx_chains_dynamic;
	/// The temp padding.
	u8 tmp_padding[2];
	/// Data area for driver use, will always be aligned to sizeof(void *), size is determined in hw information.
	u8 drv_priv[0]; ////== __aligned(sizeof(void *));
	//void * drv_priv;
};

/// Channel context switch mode.
enum i3ed11_chanctx_switch_mode
{
	/// Both old and new contexts already exist (and will continue to exist), but the virtual interface needs to be switched from one to the other.
	CHANCTX_SWMODE_REASSIGN_VIF,
	/// The old context exists but will stop to exist with this call, the new context doesn't exist but will be active after this call, the virtual interface switches from the old to the new.
	CHANCTX_SWMODE_SWAP_CONTEXTS,
};

/// vif chanctx switch information
/* This is structure is used to pass information about a vif that needs to switch from one chanctx to another. */
/* The &i3ed11_chanctx_switch_mode defines how the switch should be done. */
struct i3ed11_vif_chanctx_switch
{
	/// the vif that should be switched from old_ctx to new_ctx
	struct i3ed11_vif *vif;
	/// the old context to which the vif was assigned
	struct i3ed11_chanctx_conf *old_ctx;
	/// the new context to which the vif must be assigned
	struct i3ed11_chanctx_conf *new_ctx;
};

/// BSS change notification flags.
/* These flags are used with the bss_info_changed() callback to indicate which BSS parameter changed. */
enum i3ed11_bss_change
{
	/// Association status changed (associated/disassociated), also implies a change in the AID.
	DIW_BSS_CHANGED_ASSOC		= 1 << 0,
	/// CTS protection changed.
	DIW_BSS_CHANGED_ERP_CTS_PROT	= 1 << 1,
	/// Preamble changed.
	DIW_BSS_CHANGED_ERP_PREAMBLE	= 1 << 2,
	/// Slot timing changed.
	DIW_BSS_CHANGED_ERP_SLOT		= 1 << 3,
	/// 802.11n parameters changed.
	DIW_BSS_CHANGED_HT			= 1 << 4,
	/// Basic rateset changed.
	DIW_BSS_CHANGED_BASIC_RATES		= 1 << 5,
	/// Beacon interval changed.
	DIW_BSS_CHANGED_BEACON_INT		= 1 << 6,
	/// BSSID changed, for whatever reason (IBSS and managed mode).
	DIW_BSS_CHANGED_BSSID		= 1 << 7,
	/// Beacon data changed, retrieve new beacon (beaconing modes).
	DIW_BSS_CHANGED_BEACON		= 1 << 8,
	/// Beaconing should be enabled/disabled (beaconing modes).
	DIW_BSS_CHANGED_BEACON_ENABLED	= 1 << 9,
	/// Connection quality monitor config changed.
	DIW_BSS_CHANGED_CQM			= 1 << 10,
	/// IBSS join status changed.
	DIW_BSS_CHANGED_IBSS		= 1 << 11,
	/// Hardware ARP filter address list or state changed.
	DIW_BSS_CHANGED_ARP_FILTER		= 1 << 12,
	/// QoS for this association was enabled/disabled. Note that it is only ever disabled for station mode.
	DIW_BSS_CHANGED_QOS			= 1 << 13,
	/// Idle changed for this BSS/interface.
	DIW_BSS_CHANGED_IDLE		= 1 << 14,
	/// SSID changed for this BSS (AP and IBSS mode).
	DIW_BSS_CHANGED_SSID		= 1 << 15,
	/// Probe Response changed for this BSS (AP mode).
	DIW_BSS_CHANGED_AP_PROBE_RESP	= 1 << 16,
	/// PS changed for this BSS (STA mode).
	DIW_BSS_CHANGED_PS			= 1 << 17,
	/// TX power setting changed for this interface.
	DIW_BSS_CHANGED_TXPOWER		= 1 << 18,
	/// P2P powersave settings (CTWindow, opportunistic PS) changed (currently only in P2P client mode, GO mode will be later).
	DIW_BSS_CHANGED_P2P_PS		= 1 << 19,
	/// Data from the AP's beacon became available: currently dtim_period only is under consideration.
	DIW_BSS_CHANGED_BEACON_INFO		= 1 << 20,
	/// The bandwidth used by this interface changed, note that this is only called when it changes after the channel context had been assigned.
	DIW_BSS_CHANGED_BANDWIDTH		= 1 << 21,

	/* When adding here, make sure to change i3ed11_reconfig */
};

/// Renesas MAC HW Control Command.
enum FCI_MAC_CNTRL_VAL
{
	/// Get the Empty Timer ID.
	FCI_MAC_CNTRL_GET_EMPTY_RID	= 1 << 0,
#ifdef  FEATURE_TX_PWR_CTL_GRADE_IDX
	/// Get the TX Power Grade.
	FCI_MAC_CNTRL_GET_TX_PWR_GRADE = 1 << 1,
	/// Set the TX Power Grade.
	FCI_MAC_CNTRL_SET_TX_PWR_GRADE = 1 << 2,
#endif
	/// In DPM, Suspend the RF ADC.
	FCI_MAC_CNTRL_DPM_RF_SUSPEND = 1 << 5,
	/// In DPM, Resume the RF ADC.
	FCI_MAC_CNTRL_DPM_RF_RESUME = 1 << 6,
	/// Control the HW Calibration.
	FCI_MAC_CNTRL_HW_CAL  = 1 << 7,
	/// Get the current Temperature.
	FCI_MAC_CNTRL_GET_TEMP = 1 << 8,
	/// To turn on/off P2P GO PS.
	FCI_MAC_CNTRL_P2P_GO_PS = 1 << 9,
#ifdef FEATURE_TX_PWR_CTL_GRADE_IDX_TABLE
	///
	FCI_MAC_CNTRL_SET_TX_PWR_GRADE_TABLE = 1 << 10,
#endif
};

/*
 * The maximum number of IPv4 addresses listed for ARP filtering.
 * If the number of addresses for an interface increase beyond this value, hardware ARP filtering will be disabled.
 */
#define I3ED11_BSS_ARP_ADDR_LIST_LEN 4

/// RSSI threshold event.
/* An indicator for when RSSI goes below/above a certain threshold. */
enum i3ed11_rssi_event
{
	/// AP's rssi crossed the high threshold set by the driver.
	RSSI_EVENT_HIGH,
	/// AP's rssi crossed the low threshold set by the driver.
	RSSI_EVENT_LOW,
};

/// Holds the BSS's changing parameters.
/* This structure keeps information about a BSS (and an association to that BSS) that can change during the lifetime of the BSS. */
struct i3ed11_bss_conf
{
	/// BSSID for this BSS.
	const u8 *bssid;

	/* Association related data */
	/// Association status.
	bool assoc;
	/// Association ID number.
	u16 aid;

	/* ERP related data */
	/// Use CTS protection.
	bool use_cts_prot;
	/// Use 802.11b short preamble.
	bool use_short_preamble;
	/// Use short slot time (only relevant for ERP).
	bool use_short_slot;
	/// Whether beaconing should be enabled or not.

	bool enable_beacon;
	/// Num of beacons before the next DTIM, for beaconing.
	u8 dtim_period;
	/// Beacon interval.
	u16 beacon_int;
	/// Capabilities taken from assoc resp.
	u16 assoc_capability;
	/// Last beacon's/probe response's TSF timestamp.
	__u64 sync_tsf;
	/// The device timestamp corresponding to the sync_tsf.
	u32 sync_device_ts;
	/// Only valid when %I3ED11_HW_TIMING_BEACON_ONLY is requested, see @sync_tsf/@sync_device_ts.
	u8 sync_dtim_count;
	/// Bitmap of basic rates, each bit stands for an index into the rate table configured by the driver in the current band.
	u32 basic_rates;
	/// Associated AP's beacon TX rate.
	struct i3ed11_rate *beacon_rate;
	/// Per-band multicast rate index + 1 (0: disabled).
	int mcast_rate[I3ED11_NUM_BANDS];
	/// HT operation mode like in &struct i3ed11_ht_operation.
	u16 ht_operation_mode;
#ifdef UMAC_USE_CQM_RSSI
	/// Connection quality monitor RSSI threshold, a zero value implies disabled.
	s32 cqm_rssi_thold;
	/// Connection quality monitor RSSI hysteresis.
	u32 cqm_rssi_hyst;
#endif
	/// Channel definition for this BSS.
	struct cfgd11_chan_def chandef;
#ifdef UMAC_ARP_FILTER
	/// List of IPv4 addresses for hardware ARP filtering.
	__be32 arp_addr_list[I3ED11_BSS_ARP_ADDR_LIST_LEN];
	/// Number of addresses currently on the list.
	int arp_addr_cnt;
#endif
	/// This is a QoS-enabled BSS.
	bool qos;
	/// This interface is idle.
	bool idle;
	/// Power-save mode (STA only).
	bool ps;
	/// The SSID of the current vif. Valid in AP and IBSS mode.
	u8 ssid[I3ED11_MAX_SSID_LEN];
	/// Length of SSID given in @ssid.
	size_t ssid_len;
	/// The SSID of the current vif is hidden. Only valid in AP-mode.
	bool hidden_ssid;
	/// TX power in dBm.
	int txpower;
	/// P2P NoA attribute for P2P powersave.
	struct i3ed11_p2p_noa_attr p2p_noa_attr;
};

/// Flags to describe transmission information/status.
/* These flags are used with the @flags member of &i3ed11_tx_info. */
/* Note: If you have to add new flags to the enumeration, then don't forget to update %I3ED11_TX_TEMPORARY_FLAGS when necessary. */
enum macd11_tx_info_flags
{
	I3ED11_TX_CTL_REQ_TX_STATUS		= BIT(0),
	I3ED11_TX_CTL_ASSIGN_SEQ		= BIT(1),
	I3ED11_TX_CTL_NO_ACK			= BIT(2),
	I3ED11_TX_CTL_CLEAR_PS_FILT		= BIT(3),
	I3ED11_TX_CTL_FIRST_FRAGMENT		= BIT(4),
	I3ED11_TX_CTL_SEND_AFTER_DTIM	= BIT(5),
	I3ED11_TX_CTL_AMPDU			= BIT(6),
	I3ED11_TX_CTL_INJECTED		= BIT(7),
	I3ED11_TX_STAT_TX_FILTERED		= BIT(8),
	I3ED11_TX_STAT_ACK			= BIT(9),
	I3ED11_TX_STAT_AMPDU			= BIT(10),
	I3ED11_TX_STAT_AMPDU_NO_BACK		= BIT(11),
	I3ED11_TX_CTL_RATE_CTRL_PROBE	= BIT(12),
	I3ED11_TX_INTFL_OFFCHAN_TX_OK	= BIT(13),
	I3ED11_TX_INTFL_NEED_TXPROCESSING	= BIT(14),
	I3ED11_TX_INTFL_RETRIED		= BIT(15),
	I3ED11_TX_INTFL_DONT_ENCRYPT		= BIT(16),
	I3ED11_TX_CTL_NO_PS_BUFFER		= BIT(17),
	I3ED11_TX_CTL_MORE_FRAMES		= BIT(18),
	I3ED11_TX_INTFL_RETRANSMISSION	= BIT(19),
	I3ED11_TX_INTFL_MLME_CONN_TX		= BIT(20),
	I3ED11_TX_INTFL_NLD11_FRAME_TX	= BIT(21),
	I3ED11_TX_CTL_LDPC			= BIT(22),
	I3ED11_TX_CTL_STBC			= BIT(23) | BIT(24),
	I3ED11_TX_CTL_TX_OFFCHAN		= BIT(25),
	I3ED11_TX_INTFL_TKIP_MIC_FAILURE	= BIT(26),
	I3ED11_TX_CTL_NO_CCK_RATE		= BIT(27),
	I3ED11_TX_STATUS_EOSP		= BIT(28),
	I3ED11_TX_CTL_USE_MINRATE		= BIT(29),
	I3ED11_TX_CTL_DONTFRAG		= BIT(30),
	I3ED11_TX_CTL_PS_RESPONSE		= BIT(31),
};

#define I3ED11_TX_CTL_STBC_SHIFT		23

/// Flags to describe transmit control.
/* These flags are used in tx_info->control.flags. */
enum macd11_tx_control_flags
{
	/// This frame is a port control protocol frame (e.g. EAP).
	I3ED11_TX_CTRL_PORT_CTRL_PROTO	= BIT(0),
};

/*
 * This definition is used as a mask to clear all temporary flags,
 * which are set by the tx handlers for each transmission attempt by the macd11 stack.
 */
#define I3ED11_TX_TEMPORARY_FLAGS (I3ED11_TX_CTL_NO_ACK |		      \
								   I3ED11_TX_CTL_CLEAR_PS_FILT | I3ED11_TX_CTL_FIRST_FRAGMENT |    \
								   I3ED11_TX_CTL_SEND_AFTER_DTIM | I3ED11_TX_CTL_AMPDU |	      \
								   I3ED11_TX_STAT_TX_FILTERED |	I3ED11_TX_STAT_ACK |		      \
								   I3ED11_TX_STAT_AMPDU | I3ED11_TX_STAT_AMPDU_NO_BACK |	      \
								   I3ED11_TX_CTL_RATE_CTRL_PROBE | I3ED11_TX_CTL_NO_PS_BUFFER |    \
								   I3ED11_TX_CTL_MORE_FRAMES | I3ED11_TX_CTL_LDPC |		      \
								   I3ED11_TX_CTL_STBC | I3ED11_TX_STATUS_EOSP)

/// Per-rate flags set by the Rate Control algorithm.
enum macd11_rate_control_flags
{
	/// Use RTS/CTS exchange for this rate.
	I3ED11_TX_RC_USE_RTS_CTS		= BIT(0),
	/// CTS-to-self protection is required.
	I3ED11_TX_RC_USE_CTS_PROTECT		= BIT(1),
	/// Use short preamble.
	I3ED11_TX_RC_USE_SHORT_PREAMBLE	= BIT(2),

	/// HT rate.
	I3ED11_TX_RC_MCS			= BIT(3),
	/// Indicates whether this rate should be used in Greenfield mode.
	I3ED11_TX_RC_GREEN_FIELD		= BIT(4),
	/// Indicates if the Channel Width should be 40 MHz.
	I3ED11_TX_RC_40_MHZ_WIDTH		= BIT(5),
	/// The frame should be transmitted on both of the adjacent 20 MHz channels.
	I3ED11_TX_RC_DUP_DATA		= BIT(6),
	/// Short Guard interval should be used for this rate.
	I3ED11_TX_RC_SHORT_GI		= BIT(7),
	/// VHT MCS rate.
	I3ED11_TX_RC_VHT_MCS			= BIT(8),
	/// Indicates 80 MHz transmission.
	I3ED11_TX_RC_80_MHZ_WIDTH		= BIT(9),
	/// Indicates 160 MHz transmission.
	I3ED11_TX_RC_160_MHZ_WIDTH		= BIT(10),
};

/* If you don't need the rateset to be kept, 40 bytes */
#define I3ED11_TX_INFO_DRIVER_DATA_SIZE 40

/* If you do need the rateset, then you have less space */
#define I3ED11_TX_INFO_RATE_DRIVER_DATA_SIZE 24

/* Max num of rate stages */
#define I3ED11_TX_MAX_RATES	4

/* Max num of rate table entries */
#define I3ED11_TX_RATE_TABLE_SIZE	4

/// Rate selection/status.
struct i3ed11_tx_rate
{
	/// Rate index to attempt to send with.
	s8 idx;
	/// Number of tries in this rate before going to the next rate.
	u16 count: 5,
		/// Rate control flags (&enum macd11_rate_control_flags).
		flags: 11;
}__packed ;

#define I3ED11_MAX_TX_RETRY		31

/// wpkt transmit information.
/*
 * This structure is placed in wpkt->cb for three uses:
 * (1) macd11 TX control - macd11 tells the driver what to do
 * (2) driver internal use (if applicable)
 * (3) TX status information - driver tells macd11 what happened
 */
struct i3ed11_tx_info
{
	/* Common information */
	/// Transmit info flags, defined above.
	u32 flags;
	/// The band to transmit on (use for checking for races).
	u8 band;
	/// HW queue to put the frame on, wpkt_get_queue_mapping() gives the AC.
	u8 hw_queue;
	/// Internal frame ID for TX status, used internally.
	u16 ack_frame_id;

	union
	{
		/// union for control data.
		struct
		{
			union
			{
				/* Rate control */
				struct
				{
					struct i3ed11_tx_rate rates[I3ED11_TX_MAX_RATES];
					s8 rts_cts_rate_idx;
					u8 use_rts: 1;
					u8 use_cts_prot: 1;
					u8 short_preamble: 1;
					u8 skip_table: 1;
					/* 2 bytes free */
				};
				/* only needed before rate control */
				unsigned long gjiffies; //jiffies
			};
			/* NB: vif can be NULL for injected frames */
			struct i3ed11_vif *vif;
			struct i3ed11_key_conf *hw_key;
			u32 flags;
			/* 4 bytes free */
		} control;
		/// union for status data.
		struct
		{
			struct i3ed11_tx_rate rates[I3ED11_TX_MAX_RATES];
			/// Signal strength of the ACK frame
			int ack_signal;
			/// Number of acked aggregated frames. relevant only if I3ED11_TX_STAT_AMPDU was set.
			u8 ampdu_ack_len;
			/// Number of aggregated frames. relevant only if I3ED11_TX_STAT_AMPDU was set.
			u8 ampdu_len;
			u8 antenna;
			/* 21 bytes free */
		} status;
		struct
		{
			struct i3ed11_tx_rate driver_rates[I3ED11_TX_MAX_RATES];
			u8 pad[4];

			void *rate_driver_data[
		I3ED11_TX_INFO_RATE_DRIVER_DATA_SIZE / sizeof(void *)];
		};
		/// Array of driver_data pointers.
		void *driver_data[
		I3ED11_TX_INFO_DRIVER_DATA_SIZE / sizeof(void *)];
	};
};

static inline struct i3ed11_tx_info *I3ED11_SKB_CB(struct wpktbuff *wpkt)
{
	return (struct i3ed11_tx_info *)wpkt->cb;
}

static inline struct i3ed11_rx_status *I3ED11_SKB_RXCB(struct wpktbuff *wpkt)
{
	return (struct i3ed11_rx_status *)wpkt->cb;
}

/// Receive flags.
/* These flags are used with the @flag member of &struct i3ed11_rx_status. */
enum macd11_rx_flags
{
	DIW_RX_FLAG_MMIC_ERROR		= BIT(0),
	DIW_RX_FLAG_DECRYPTED		= BIT(1),
	DIW_RX_FLAG_MMIC_STRIPPED		= BIT(3),
	DIW_RX_FLAG_IV_STRIPPED		= BIT(4),
	DIW_RX_FLAG_FAILED_FCS_CRC		= BIT(5),
	DIW_RX_FLAG_FAILED_PLCP_CRC 	= BIT(6),
	DIW_RX_FLAG_MACTIME_START		= BIT(7),
	DIW_RX_FLAG_SHORTPRE		= BIT(8),
	DIW_RX_FLAG_HT			= BIT(9),
	DIW_RX_FLAG_40MHZ			= BIT(10),
	DIW_RX_FLAG_SHORT_GI		= BIT(11),
	DIW_RX_FLAG_NO_SIGNAL_VAL		= BIT(12),
	DIW_RX_FLAG_HT_GF			= BIT(13),
	DIW_RX_FLAG_AMPDU_DETAILS		= BIT(14),
	DIW_RX_FLAG_AMPDU_REPORT_ZEROLEN	= BIT(15),
	DIW_RX_FLAG_AMPDU_IS_ZEROLEN	= BIT(16),
	DIW_RX_FLAG_AMPDU_LAST_KNOWN	= BIT(17),
	DIW_RX_FLAG_AMPDU_IS_LAST		= BIT(18),
	DIW_RX_FLAG_AMPDU_DELIM_CRC_ERROR	= BIT(19),
	DIW_RX_FLAG_AMPDU_DELIM_CRC_KNOWN	= BIT(20),
	DIW_RX_FLAG_MACTIME_END		= BIT(21),
	DIW_RX_FLAG_VHT			= BIT(22),
	DIW_RX_FLAG_80MHZ			= BIT(23),
	DIW_RX_FLAG_80P80MHZ		= BIT(24),
	DIW_RX_FLAG_160MHZ			= BIT(25),
	DIW_RX_FLAG_STBC_MASK		= BIT(26) | BIT(27),
	DIW_RX_FLAG_10MHZ			= BIT(28),
	DIW_RX_FLAG_5MHZ			= BIT(29),
	DIW_RX_FLAG_AMSDU_MORE		= BIT(30),
};

#define DIW_RX_FLAG_STBC_SHIFT		26

/// Receive status.
/* The low-level driver should provide this information (the subset supported by hardware) to the 802.11 code */
/* with each received frame, in the wpkt's control buffer (cb). */
struct i3ed11_rx_status
{
	__u64 mactime;
	u32 device_timestamp;
	u32 ampdu_reference;
	u32 flag;
	//	u32 vendor_radiotap_bitmap;
	//	u16 vendor_radiotap_len;
	u16 freq;
	u8 rate_idx;
	u8 vht_nss;
	u8 rx_flags;
	u8 band;
	u8 antenna;
	s8 signal;
	u8 chains;
	s8 chain_signal[I3ED11_MAX_CHAINS];
	//	u8 ampdu_delimiter_crc;
	//	u8 vendor_radiotap_align;
	//	u8 vendor_radiotap_oui[3];
	//	u8 vendor_radiotap_subns;
};

/// Configuration flags.
/* Flags to define PHY configuration options. */
enum i3ed11_conf_flags
{
	/// There's a monitor interface present.
	I3ED11_CONF_MONITOR		= (1 << 0),
	/// Enable 802.11 power save mode (managed mode only).
	I3ED11_CONF_PS		= (1 << 1),
	/// The device is running, but idle.
	I3ED11_CONF_IDLE		= (1 << 2),
	/// The device is currently not on its main operating channel.
	I3ED11_CONF_OFFCHANNEL	= (1 << 3),
};

/// Denotes which configuration changed.
enum i3ed11_conf_changed
{
	/// Spatial multiplexing powersave mode changed.
	/// Note that this is only valid if channel contexts are not used, otherwise each channel context has the number of chains listed.
	I3ED11_CONF_CHANGE_SMPS		= BIT(1),
	/// The listen interval changed.
	I3ED11_CONF_CHANGE_LISTEN_INTERVAL	= BIT(2),
	/// The monitor flag changed.
	I3ED11_CONF_CHANGE_MONITOR		= BIT(3),
	/// The PS flag or dynamic PS timeout changed.
	I3ED11_CONF_CHANGE_PS		= BIT(4),
	/// The TX power changed.
	I3ED11_CONF_CHANGE_POWER		= BIT(5),
	/// The channel/channel_type changed.
	I3ED11_CONF_CHANGE_CHANNEL		= BIT(6),
	/// Retry limits changed.
	I3ED11_CONF_CHANGE_RETRY_LIMITS	= BIT(7),
	/// Idle flag changed.
	I3ED11_CONF_CHANGE_IDLE		= BIT(8),
	///
	I3ED11_CONF_CHANGE_DPM_BOOT		= BIT(9),
	///
	I3ED11_CONF_CHANGE_P2P_GO_PS		= BIT(10),
};

#ifdef FEATURE_UMAC_MESH
/**
 * enum i3ed11_smps_mode - spatial multiplexing power save mode
 *
 * @I3ED11_SMPS_AUTOMATIC: automatic
 * @I3ED11_SMPS_OFF: off
 * @I3ED11_SMPS_STATIC: static
 * @I3ED11_SMPS_DYNAMIC: dynamic
 * @I3ED11_SMPS_NUM_MODES: internal, don't use
 */
enum i3ed11_smps_mode {
	I3ED11_SMPS_AUTOMATIC,
	I3ED11_SMPS_OFF,
	I3ED11_SMPS_STATIC,
	I3ED11_SMPS_DYNAMIC,

	/* keep last */
	I3ED11_SMPS_NUM_MODES,
};
#endif /* FEATURE_UMAC_MESH */

/// Configuration of the device.
/* This struct indicates how the driver shall configure the hardware. */
struct i3ed11_conf
{
	/// Configuration flags defined above.
	u32 flags;
	/// Requested transmit power (in dBm).
	int power_level;
	/// The dynamic powersave timeout (in ms).
	int dyn_ps_timeout;
	/// The maximum number of beacon intervals to sleep for before checking the beacon for a TIM bit (managed mode only).
	int max_sleep_period;
	/// Listen interval in units of beacon interval.
	u16 listen_interval;
	/// The DTIM period of the AP we're connected to, for use in power saving.
	u8 ps_dtim_period;
	/// Maximum number of transmissions for a "long" frame (a frame not RTS protected).
	u8 long_frame_max_tx_count;
	/// Maximum number of transmissions for a "short" frame.
	u8 short_frame_max_tx_count;
	/// The channel definition to tune to.
	struct cfgd11_chan_def chandef;
};

/// holds the channel switch data
/* The information provided in this structure is required for channel switch operation. */
struct i3ed11_channel_switch
{
	/// value in microseconds of the 64-bit Time Synchronization Function (TSF) timer when the frame containing the channel switch announcement was received.
	/* This is simply the rx.mactime parameter the driver passed into macd11. */
	u64 timestamp;
	/// arbitrary timestamp for the device, this is the rx. device_timestamp parameter the driver passed to macd11.
	u32 device_timestamp;
	/// Indicates whether transmission must be blocked before the scheduled channel switch, as indicated by the AP.
	bool block_tx;
	/// the new channel to switch to
	struct cfgd11_chan_def chandef;
	/// the number of TBTT's until the channel switch event
	u8 count;
};

/// Virtual interface flags.
enum i3ed11_vif_flags
{
	/// The device performs beacon filtering on this virtual interface to avoid unnecessary CPU wakeups.
	I3ED11_VIF_BEACON_FILTER		= BIT(0),
	/// The device can do connection quality monitoring on this virtual interface.
	/* i.e. it can monitor connection quality related parameters,
	 * such as the RSSI level and provide notifications if configured trigger levels are reached. */
	I3ED11_VIF_SUPPORTS_CQM_RSSI		= BIT(1),
};

/// Per-interface data.
/* Data in this structure is continually present for driver use during the life of a virtual interface. */
struct i3ed11_vif
{
	/// Type of this virtual interface.
	enum nld11_iftype type;
	/// BSS configuration for this interface, either our own or the BSS we're associated to.
	struct i3ed11_bss_conf bss_conf;
	/// Address of this interface.
	u8 addr[DIW_ETH_ALEN];
	/// Indicates whether this AP or STA interface is a p2p interface, i.e. a GO or p2p-sta respectively.
	bool p2p;
	/// Content-after-beacon (DTIM beacon really) queue, AP mode only.
	u8 cab_queue;
	/// Hardware queue for each AC.
	u8 hw_queue[I3ED11_NUM_ACS];
	/// The channel context this interface is assigned to, or %NULL when it is not assigned.
	struct i3ed11_chanctx_conf __rcu *chanctx_conf;
	/// Flags/capabilities the driver has for this interface.
	u32 driver_flags;

	/* vif structure should not be changed */
	/* Also we do not support CSA */
#ifdef FEATURE_UMAC_MESH
	bool csa_active;
	u8	reserved[3];	//for MAC rwnx_hw arrignment
#endif

	/* Must be last */
	/// Data area for driver use, will always be aligned to sizeof(void *).
	u8 drv_priv[0]; ////== __aligned(sizeof(void *));
};

#ifdef FEATURE_UMAC_MESH
static inline bool i3ed11_vif_is_mesh(struct i3ed11_vif *vif)
{
#ifdef CONFIG_MAC80211_MESH
	return vif->type == NLD11_IFTYPE_MESH_POINT;
#else
	return false;
#endif /* CONFIG_MAC80211_MESH */
}
#endif /* FEATURE_UMAC_MESH */

/// Key flags.
/* These flags are used for communication about keys between the driver and macd11, */
/* with the @flags parameter of &struct i3ed11_key_conf. */
enum i3ed11_key_flags
{
	///
	I3ED11_KEY_FLAG_GENERATE_IV_MGMT = BIT(0),
	/// This flag should be set by the driver to indicate that it requires IV generation for this particular key.
	I3ED11_KEY_FLAG_GENERATE_IV	= 1 << 1,
	/// This flag should be set by the driver for a TKIP key if it requires Michael MIC generation in software.
	I3ED11_KEY_FLAG_GENERATE_MMIC = 1 << 2,
	/// Set by macd11, this flag indicates that the key is pairwise rather then a shared key.
	I3ED11_KEY_FLAG_PAIRWISE = 1 << 3,
	/// This flag should be set by the driver for a CCMP key if it requires CCMP encryption of management frames (MFP) to be done in software.
	I3ED11_KEY_FLAG_SW_MGMT_TX = 1 << 4,
	/// This flag should be set by the driver if space should be prepared for the IV, but the IV itself should not be generated.
	I3ED11_KEY_FLAG_PUT_IV_SPACE = 1 << 5,
	/// This key will be used to decrypt received management frames.
	I3ED11_KEY_FLAG_RX_MGMT = 1 << 6,
	///
	I3ED11_KEY_FLAG_RESERVE_TAILROOM = 1 << 7,
	///
	I3ED11_KEY_FLAG_PUT_MIC_SPACE = 1 << 8,
};

/// Key information.
/* This key information is given by macd11 to the driver by the set_key() callback in &struct i3ed11_ops. */
struct i3ed11_key_conf
{
	/// The key's cipher suite selector.
	u32 cipher;
	/// The ICV length for this key type.
	u8 icv_len;
	/// The IV length for this key type.
	u8 iv_len;
	/// To be set by the driver, this is the key index the driver wants to be given when a frame is transmitted and needs to be encrypted in hardware.
	u8 hw_key_idx;
	/// Key flags, see &enum i3ed11_key_flags.
	u8 flags;
	/// The key index (0-3).
	s8 keyidx;
	/// Key material length.
	u8 keylen;
	/// Temp padding.
	u8 tmp_padding[2];
	/// Key material.
	/* For ALG_TKIP the key is encoded as a 256-bit (32 byte) data block:
	 * Temporal Encryption Key (128 bits)
	 * Temporal Authenticator Tx MIC Key (64 bits)
	 * Temporal Authenticator Rx MIC Key (64 bits) */
	u8 key[0];
};

/// Cipher scheme.
/* This structure contains a cipher scheme information defining the secure packet crypto handling. */
struct i3ed11_cipher_scheme
{
	/// A cipher suite selector.
	u32 cipher;
	/// A cipher iftype bit mask indicating an allowed cipher usage.
	u16 iftype;
	/// A length of a security header used the cipher.
	u8 hdr_len;
	/// A length of a packet number in the security header.
	u8 pn_len;
	/// An offset of pn from the beginning of the security header.
	u8 pn_off;
	/// An offset of key index byte in the security header.
	u8 key_idx_off;
	/// A bit mask of key_idx bits.
	u8 key_idx_mask;
	/// A bit shift needed to get key_idx.
	/* key_idx value calculation:
	 * (sec_header_base[key_idx_off] & key_idx_mask) >> key_idx_shift */
	u8 key_idx_shift;
	/// A mic length in bytes.
	u8 mic_len;
};

/// Key command.
/* Used with the set_key() callback in &struct i3ed11_ops, this indicates whether a key is being removed or added. */
enum set_key_cmd
{
	/// A key is set.
	SET_KEY,
	/// A key must be disabled.
	DISABLE_KEY,
};

/// Station state.
enum i3ed11_sta_state
{
	/* NOTE: These need to be ordered correctly! */
	/// Station doesn't exist at all, this is a special state for add/remove transitions.
	I3ED11_STA_NOTEXIST,
	/// Station exists without special state.
	I3ED11_STA_NONE,
	/// Station is authenticated.
	I3ED11_STA_AUTH,
	/// Station is associated.
	I3ED11_STA_ASSOC,
	/// Station is authorized (802.1X).
	I3ED11_STA_AUTHORIZED,
};

/// Station RX bandwidth.
/* Implementation note: 20 must be zero to be initialized correctly, the values must be sorted. */
enum i3ed11_sta_rx_bandwidth
{
	/// Station can only receive 20 MHz.
	I3ED11_STA_RX_BW_20 = 0,
	/// Station can receive up to 40 MHz.
	I3ED11_STA_RX_BW_40,
	/// Station can receive up to 80 MHz.
	I3ED11_STA_RX_BW_80,
	/// Station can receive up to 160 MHz (including 80+80 MHz).
	I3ED11_STA_RX_BW_160,
};

/// Station rate selection table.
struct i3ed11_sta_rates
{
	//RCU head used for freeing the table on update.
	//struct rcu_head rcu_head;	//not used
	/// Transmit rates/flags to be used by default.
	/* Overriding entries per-packet is possible by using cb tx control. */
	struct
	{
		s8 idx;
		u8 count;
		u8 count_cts;
		u8 count_rts;
		u16 flags;
	} rate[I3ED11_TX_RATE_TABLE_SIZE];
};

/// Station table entry.
struct i3ed11_sta
{
	/// Bitmap of supported rates (per band).
	u32 supp_rates[I3ED11_NUM_BANDS];
	/// MAC address.
	u8 addr[DIW_ETH_ALEN];
	/// AID we assigned to the station if we're an AP.
	u16 aid;
	/// HT capabilities of this STA; restricted to our own capabilities.
	struct i3ed11_sta_ht_cap ht_cap;
	/// Indicates whether the STA supports WME. Only valid during AP-mode.
	bool wme;
	/// Bitmap of queues configured for uapsd. Only valid if wme is supported.
	u8 uapsd_queues;
	/// Max Service Period. Only valid if wme is supported.
	u8 max_sp;
	/// Current bandwidth the station can receive with.
	enum i3ed11_sta_rx_bandwidth bandwidth;

	/// Rate control selection table.
#ifdef FEATURE_STAIC_RATES
	struct i3ed11_sta_rates rates;
#else
	struct i3ed11_sta_rates __rcu *rates;
#endif /* FEATURE_STAIC_RATES */

	/* Must be last */
	/// Data area for driver use, will always be aligned to sizeof(void *), size is determined in hw information.
#ifdef FEATURE_RWNX_QUEUE_LIST
	void *drv_priv;
#else
	u8 drv_priv[0]; ////== __aligned(sizeof(void *));
#endif
};

/// Sta notify command.
/* Used with the sta_notify() callback in &struct i3ed11_ops, this indicates if an associated station made a power state transition. */
enum sta_notify_cmd
{
	/// A station is now sleeping.
	STA_NOTIFY_SLEEP,
	/// A sleeping station woke up.
	STA_NOTIFY_AWAKE,
};

/// TX control data.
struct i3ed11_tx_control
{
	/// Station table entry, this sta pointer may be NULL and it is not allowed to copy the pointer, due to RCU.
	struct i3ed11_sta *sta;
};

/// Hardware flags
/*
 * These flags are used to indicate hardware capabilities to the stack.
 * Generally, flags here should have their meaning done in a way
 * that the simplest hardware doesn't need setting any particular flags.
 * There are some exceptions to this rule, however, so you are advised to review these flags carefully.
 */
enum i3ed11_hw_flags
{
	I3ED11_HW_HAS_RATE_CONTROL			= 1 << 0,
	I3ED11_HW_RX_INCLUDES_FCS			= 1 << 1,
	I3ED11_HW_HOST_BROADCAST_PS_BUFFERING	= 1 << 2,
	I3ED11_HW_2GHZ_SHORT_SLOT_INCAPABLE		= 1 << 3,
	I3ED11_HW_2GHZ_SHORT_PREAMBLE_INCAPABLE	= 1 << 4,
	I3ED11_HW_SIGNAL_UNSPEC			= 1 << 5,
	I3ED11_HW_SIGNAL_DBM				= 1 << 6,
	I3ED11_HW_NEED_DTIM_BEFORE_ASSOC		= 1 << 7,
	I3ED11_HW_SPECTRUM_MGMT			= 1 << 8,
	I3ED11_HW_AMPDU_AGGREGATION			= 1 << 9,
	I3ED11_HW_SUPPORTS_PS			= 1 << 10,
	I3ED11_HW_PS_NULLFN_STACK			= 1 << 11,
	I3ED11_HW_SUPPORTS_DYNAMIC_PS		= 1 << 12,
	I3ED11_HW_MFP_CAPABLE			= 1 << 13,
	I3ED11_HW_WANT_MONITOR_VIF			= 1 << 14,
	I3ED11_HW_SUPPORTS_STATIC_SMPS		= 1 << 15,
	I3ED11_HW_SUPPORTS_DYNAMIC_SMPS		= 1 << 16,
	I3ED11_HW_SUPPORTS_UAPSD			= 1 << 17,
	I3ED11_HW_REPORTS_TX_ACK_STATUS		= 1 << 18,
	I3ED11_HW_CONNECTION_MONITOR			= 1 << 19,
	I3ED11_HW_QUEUE_CONTROL			= 1 << 20,
	I3ED11_HW_SUPPORTS_PER_STA_GTK		= 1 << 21,
	I3ED11_HW_AP_LINK_PS				= 1 << 22,
	I3ED11_HW_TX_AMPDU_SETUP_IN_HW		= 1 << 23,
	I3ED11_HW_SUPPORTS_RC_TABLE			= 1 << 24,
	I3ED11_HW_P2P_DEV_ADDR_FOR_INTF		= 1 << 25,
	I3ED11_HW_TIMING_BEACON_ONLY			= 1 << 26,
	I3ED11_HW_SUPPORTS_HT_CCK_RATES		= 1 << 27,
	I3ED11_HW_CHANCTX_STA_CSA			= 1 << 28,

	/* Keep last, obviously */
	NUM_I3ED11_HW_FLAGS
};

/// Hardware information and state.
/* This structure contains the configuration and hardware information for an 802.11 PHY. */
struct i3ed11_hw
{
	struct i3ed11_conf conf;
	struct softmac *softmac;
	const char *rate_control_algorithm;
	void *priv;
	uint32_t flags;
	unsigned int extra_tx_headroom;
	unsigned int extra_beacon_tailroom;
	int channel_change_time;
	int vif_data_size;
	int sta_data_size;
	int chanctx_data_size;
	uint16_t queues;
	uint16_t max_listen_interval;
	s8 max_signal;
	uint8_t max_rates;
	uint8_t max_report_rates;
	uint8_t max_rate_tries;
	uint8_t max_rx_agg_subframes;
	uint8_t max_tx_agg_subframes;
	uint8_t offchannel_tx_hw_queue;
	uint8_t radiotap_mcs_details;
	uint8_t uapsd_queues;
	uint8_t uapsd_max_sp_len;
	uint8_t n_cipher_schemes;

	const struct i3ed11_cipher_scheme *cipher_schemes;
};

/**
 ****************************************************************************************
 * @brief Set the permanent MAC address for 802.11 hardware.
 * @param[in] hw The &struct i3ed11_hw to set the MAC address for.
 * @param[in] addr The address to set.
 ****************************************************************************************
 */
static inline void SET_I3ED11_PERM_ADDR(struct i3ed11_hw *hw, u8 *addr)
{
	memcpy(hw->softmac->perm_addr, addr, DIW_ETH_ALEN);
}

static inline struct i3ed11_rate *
i3ed11_get_rts_cts_rate(const struct i3ed11_hw *hw,
						const struct i3ed11_tx_info *c)
{
	if (c->control.rts_cts_rate_idx < 0)
	{
		return NULL;
	}

	return &hw->softmac->bands[c->band]->bitrates[c->control.rts_cts_rate_idx];
}

/**
 ****************************************************************************************
 * @brief Free TX wpkt.
 * @param[in] hw The hardware.
 * @param[in] wpkt The wpkt.
 * @details Free a transmit wpkt.\n
 * Use this funtion when some failure to transmit happened and thus status cannot be reported.
 ****************************************************************************************
 */
void i3ed11_free_txwpkt(struct i3ed11_hw *hw, struct wpktbuff *wpkt);

/// Hardware filter flags.
enum i3ed11_filter_flags
{
	DIW_FIF_PROMISC_IN_BSS	= 1 << 0,
	DIW_FIF_ALLMULTI		= 1 << 1,
	DIW_FIF_FCSFAIL			= 1 << 2,
	DIW_FIF_PLCPFAIL		= 1 << 3,
	DIW_FIF_BCN_PRBRESP_PROMISC	= 1 << 4,
	DIW_FIF_CONTROL			= 1 << 5,
	DIW_FIF_OTHER_BSS		= 1 << 6,
	DIW_FIF_PSPOLL			= 1 << 7,
	DIW_FIF_PROBE_REQ		= 1 << 8,
};

/// A-MPDU actions.
/*
 * These flags are used with the ampdu_action() callback in &struct i3ed11_ops to indicate
 * which action is needed.
 * Note that drivers MUST be able to deal with a TX aggregation session being stopped
 * even before they OK'ed starting it by calling i3ed11_start_tx_ba_cb_irqsafe,
 * because the peer might receive the addBA frame and send a delBA right away!
 */
enum i3ed11_ampdu_mlme_action
{
	/// Start RX aggregation.
	I3ED11_AMPDU_RX_START,
	/// Stop RX aggregation.
	I3ED11_AMPDU_RX_STOP,
	/// Start TX aggregation.
	I3ED11_AMPDU_TX_START,
	/// Stop TX aggregation but continue transmitting queued packets, now unaggregated.
	/* After all packets are transmitted the driver has to call i3ed11_stop_tx_ba_cb_irqsafe(). */
	I3ED11_AMPDU_TX_STOP_CONT,
	/// Stop TX aggregation and flush all packets, called when the station is removed.
	/* There's no need or reason to call i3ed11_stop_tx_ba_cb_irqsafe() in this case
	 * as macd11 assumes the session is gone and removes the station. */
	I3ED11_AMPDU_TX_STOP_FLUSH,
	/// Called when TX aggregation is stopped.
	/* But the driver hasn't called i3ed11_stop_tx_ba_cb_irqsafe() yet
	 * and now the connection is dropped and the station will be removed.
	 * Drivers should clean up and drop remaining packets when this is called. */
	I3ED11_AMPDU_TX_STOP_FLUSH_CONT,
	/// TX aggregation has become operational.
	I3ED11_AMPDU_TX_OPERATIONAL,
};

/// Frame release reason.
enum i3ed11_frame_release_type
{
	/// Frame released for PS-Poll.
	I3ED11_FRAME_RELEASE_PSPOLL,
	/// Frame(s) released due to frame received on trigger-enabled AC.
	I3ED11_FRAME_RELEASE_UAPSD,
};

/// Flags to indicate what changed.
enum i3ed11_rate_control_changed
{
	/// The bandwidth that can be used to transmit to this station changed.
	/* The actual bandwidth is in the station information. */
	/* For HT20/40 the I3ED11_HT_CAP_SUP_WIDTH_20_40 flag changes,
	 * for HT and VHT the bandwidth field changes. */
	I3ED11_RC_BW_CHANGED		= BIT(0),
	/// The SMPS state of the station changed.
	I3ED11_RC_SMPS_CHANGED	= BIT(1),
	/// The supported rate set of this peer changed (in IBSS mode) due to discovering more information about the peer.
	I3ED11_RC_SUPP_RATES_CHANGED	= BIT(2),
	/// N_SS (number of spatial streams) was changed by the peer.
	I3ED11_RC_NSS_CHANGED	= BIT(3),
};

/// Remain on channel type.
enum i3ed11_roc_type
{
	/// There are no special requirements for this ROC.
	I3ED11_ROC_TYPE_NORMAL = 0,
	/// The remain on channel request is required for sending managment frames offchannel.
	I3ED11_ROC_TYPE_MGMT_TX,
};

/// Callbacks from macd11 to the driver.
struct i3ed11_ops
{
	/// Handler that 802.11 module calls for each transmitted frame.
	void (*tx)(struct i3ed11_hw *hw,
			   struct i3ed11_tx_control *control,
			   struct wpktbuff *wpkt);
	/// Called before the first netdevice attached to the hardware is enabled.
	int (*start)(struct i3ed11_hw *hw);
	/// Called after last netdevice attached to the hardware is disabled.
	void (*stop)(struct i3ed11_hw *hw);
	/// Called when a netdevice attached to the hardware is enabled.
	int (*add_interface)(struct i3ed11_hw *hw,
						 struct i3ed11_vif *vif);
	/// Notifies a driver that an interface is going down.
	void (*remove_interface)(struct i3ed11_hw *hw,
							 struct i3ed11_vif *vif);
	/// Handler for configuration requests.
	int (*config)(struct i3ed11_hw *hw, u32 changed);
	/// Handler for configuration requests related to BSS parameters.
	void (*bss_info_changed)(struct i3ed11_hw *hw,
							 struct i3ed11_vif *vif,
							 struct i3ed11_bss_conf *info,
							 u32 changed);
	/// Prepare for multicast filter configuration.
	u64 (*prepare_multicast)(struct i3ed11_hw *hw,
							 struct netdev_hw_addr_list *mc_list);
	/// Configure the device's RX filter.
	void (*configure_filter)(struct i3ed11_hw *hw,
							 unsigned int changed_flags,
							 unsigned int *total_flags,
							 u64 multicast);
	/// Set TIM bit.
	int (*set_tim)(struct i3ed11_hw *hw, struct i3ed11_sta *sta, bool set);
	/// Set key.
	int (*set_key)(struct i3ed11_hw *hw, enum set_key_cmd cmd,
				   struct i3ed11_vif *vif, struct i3ed11_sta *sta,
				   struct i3ed11_key_conf *key);
	/// Ask the hardware to service the scan request, no need to start the scan state machine in stack.
	int (*hw_scan)(struct i3ed11_hw *hw, struct i3ed11_vif *vif,
				   struct cfgd11_scan_request *req);
	/// Ask the low-level tp cancel the active hw scan.
	void (*cancel_hw_scan)(struct i3ed11_hw *hw, struct i3ed11_vif *vif);
	/// Notifies low level driver about addition of an associated station, AP, IBSS/WDS/mesh peer etc.
	int (*sta_add)(struct i3ed11_hw *hw, struct i3ed11_vif *vif, struct i3ed11_sta *sta);
	/// Notifies low level driver about removal of an associated station, AP, IBSS/WDS/mesh peer etc.
	int (*sta_remove)(struct i3ed11_hw *hw, struct i3ed11_vif *vif, struct i3ed11_sta *sta);
	/// Configure TX queue parameters.
	int (*conf_tx)(struct i3ed11_hw *hw, struct i3ed11_vif *vif, u16 ac,
				   const struct i3ed11_tx_queue_params *params);
	/// Perform a certain A-MPDU action.
	int (*ampdu_action)(struct i3ed11_hw *hw,
						struct i3ed11_vif *vif,
						enum i3ed11_ampdu_mlme_action action,
						struct i3ed11_sta *sta, u16 tid, u16 *ssn,
						u8 buf_sz);
#ifdef UMAC_ACS_SUPPORT
	///Return per-channel survey information.
	int (* get_survey) (struct i3ed11_hw *hw, int idx, struct survey_info *survey);
#endif

#if 1 //#ifdef WITH_UMAC_DPM
	int (* get_rtccurtime) (struct i3ed11_hw *hw, u64 *rtccurtime);
	int (* get_rtctimegap) (struct i3ed11_hw *hw, u64 *rtctimegap);
	/* Not used */
	//int (* set_keepalivetime) (struct i3ed11_hw *hw, unsigned int sec,
	//						   void (* callback_func)(void));
	int (* set_app_keepalivetime) (struct i3ed11_hw *hw, unsigned char tid, unsigned int msec,
								   void (* callback_func)(unsigned int tid));
	void (* set_macaddress)(struct i3ed11_hw *hw);
	void (*set_dpmconf)(struct i3ed11_hw *hw, unsigned char ap_sync_cntrl, int ka_period_ms);
#endif
	void (* pwrdown_primary)(struct i3ed11_hw *hw, bool retention);
	void (* pwrdown_secondary)(struct i3ed11_hw *hw, u64 usec, bool retention);

	/// MAC HW Control command.
	int  (* cntrl_mac_hw)(struct i3ed11_hw *hw, u32 cntrl, u32 val);

#ifdef CONFIG_NL80211_TESTMODE
	/// Implement a cfg80211 test mode command.
	int (*testmode_cmd)(struct i3ed11_hw *hw, struct i3ed11_vif *vif,
						void *data, int len);
	/// Implement a cfg80211 test mode dump.
	int (*testmode_dump)(struct i3ed11_hw *hw, struct wpktbuff *wpkt,
						 struct netlink_callback *cb,
						 void *data, int len);
#endif
	/// Flush all pending frames from the hardware queue.
	void (*flush)(struct i3ed11_hw *hw, u32 queues, bool drop);
	/// Check if there is any pending frame in the hardware queues before entering power save.
	bool (*tx_frames_pending)(struct i3ed11_hw *hw);
	/// Prepare for transmitting a management frame for association before associated.
	void (*mgd_prepare_tx)(struct i3ed11_hw *hw, struct i3ed11_vif *vif);
	/// Notifies device driver about new channel context creation
	int (*add_chanctx)(struct i3ed11_hw *hw, struct i3ed11_chanctx_conf *ctx);
	/// Notifies device driver about channel context destruction
	void (*remove_chanctx)(struct i3ed11_hw *hw, struct i3ed11_chanctx_conf *ctx);
	/// Notifies device driver about channel context changes.
	void (*change_chanctx)(struct i3ed11_hw *hw,
						   struct i3ed11_chanctx_conf *ctx, u32 changed);
	/// Notifies device driver about channel context being bound to vif.
	int (*assign_vif_chanctx)(struct i3ed11_hw *hw,
							  struct i3ed11_vif *vif,
							  struct i3ed11_chanctx_conf *ctx);
	/// Starts an off-channel period on the given channel.
	int (*remain_on_ch)(struct i3ed11_hw *hw,
						struct i3ed11_vif *vif,
						struct i3ed11_channel *chan,
						int duration,
						enum i3ed11_roc_type type);
	/// Requests that an ongoing off-channel period is aborted before it expires.
	int (*cancel_remain_on_ch)(struct i3ed11_hw *hw);
	/// Notifies device driver about channel context being unbound from vif.
	void (*unassign_vif_chanctx)(struct i3ed11_hw *hw,
								 struct i3ed11_vif *vif,
								 struct i3ed11_chanctx_conf *ctx);
	///
	void (*restart_complete)(struct i3ed11_hw *hw);
#if CONFIG_IPV6
	/// IPv6 address assignment on the given interface changed.
	void (*ipv6_addr_change)(struct i3ed11_hw *hw,
							 struct i3ed11_vif *vif,
							 struct inet6_dev *idev);
#endif
};

/**
 ****************************************************************************************
 * @brief Allocate a new hardware device.
 * @param[in] priv_data_len Length of private data.
 * @param[in] ops Callbacks for this device.
 * @details
 * This must be called once for each hardware device.\n
 * The returned pointer must be used to refer to this device when calling other functions.\n
 * macd11 allocates a private data area for the driver pointed to by @priv in &struct i3ed11_hw,\n
 * the size of this area is given as @priv_data_len.
 * @return A pointer to the new hardware device, or %NULL on error.
 ****************************************************************************************
 */
struct i3ed11_hw *i3ed11_alloc_hw(size_t priv_data_len,
								  const struct i3ed11_ops *ops);

/**
 ****************************************************************************************
 * @brief Register hardware device.
 * @param[in] hw The device to register as returned by i3ed11_alloc_hw().
 * @details
 * You must call this function before any other functions in macd11.\n
 * Note that before a hardware can be registered, you need to fill the contained softmac's information.
 * @return 0 on success. An error code otherwise.
 ****************************************************************************************
 */
int i3ed11_register_hw(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Unregister a hardware device.
 * @param[in] hw The hardware to unregister.
 * @details
 * This function instructs macd11 to free allocated resources
 * and unregister netdevices from the networking subsystem.
 ****************************************************************************************
 */
void i3ed11_unregister_hw(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Free hardware descriptor.
 * @param[in] hw The hardware to free.
 * @details
 * This function frees everything that was allocated, including the private data for the driver.\n
 * You must call i3ed11_unregister_hw() before calling this function.
 ****************************************************************************************
 */
void i3ed11_free_hw(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Restart hardware completely.
 * @param[in] hw The hardware to restart.
 * @details
 * Call this function when the hardware was restarted for some reason (hardware error, ...)
 * and the driver is unable to restore its state by itself.\n
 * macd11 assumes that at this point the driver/hardware is completely uninitialised and stopped,
 * it starts the process by calling the ->start() operation.\n
 * The driver will need to reset all internal state that it has prior to calling this function.
 ****************************************************************************************
 */
void i3ed11_restart_hw(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Receive frame.
 * @param[in] hw The hardware this frame came in on.
 * @param[in] wpkt The buffer to receive, owned by macd11 after this call.
 * @details
 * Use this function to hand received frames to macd11.\n
 * The receive buffer in @wpkt must start with an i3ed11 header.\n
 * In case of a paged @wpkt is used,\n
 * the driver is recommended to put the i3ed11 header of the frame on the linear part of the @wpkt
 * to avoid memory allocation and/or memcpy by the stack.\n\n
 * This function may not be called in IRQ context.\n
 * Calls to this function for a single hardware must be synchronized against each other.\n
 * Calls to this function, i3ed11_rx_ni() and i3ed11_rx_irqsafe() may not be mixed for a single hardware.\n
 * Must not run concurrently with i3ed11_tx_status() or i3ed11_tx_status_ni().\n\n
 * In process context use instead i3ed11_rx_ni().
 ****************************************************************************************
 */
void i3ed11_rx(struct i3ed11_hw *hw, struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief Receive frame.
 * @details
 * Like i3ed11_rx() but can be called in IRQ context (internally defers to a tasklet).\n
 * Calls to this function, i3ed11_rx() or i3ed11_rx_ni() may not be mixed for a single hardware.\n
 * Must not run concurrently with i3ed11_tx_status() or i3ed11_tx_status_ni().
 * @param[in] hw The hardware this frame came in on.
 * @param[in] wpkt The buffer to receive, owned by macd11 after this call.
 ****************************************************************************************
 */
void i3ed11_rx_irqsafe(struct i3ed11_hw *hw, struct wpktbuff *wpkt);

/*
 * The TX headroom reserved by macd11 for its own tx_status functions.
 * This is enough for the radiotap header.
 */
#define I3ED11_TX_STATUS_HEADROOM	14

/**
 ****************************************************************************************
 * @brief Inform macd11 about driver-buffered frames.
 * @param[in] sta &struct i3ed11_sta pointer for the sleeping station.
 * @param[in] tid The TID that has buffered frames.
 * @param[in] buffered Indicates whether or not frames are buffered for this TID.
 ****************************************************************************************
 */
void i3ed11_sta_set_buffered(struct i3ed11_sta *sta, u8 tid, bool buffered);

/**
 ****************************************************************************************
 * @brief Get the selected transmit rates for a packet.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] sta The receiver station to which this packet is sent.
 * @param[in] wpkt The frame to be transmitted.
 * @param[in] dest Buffer for extracted rate/retry information.
 * @param[in] max_rates Maximum number of rates to fetch.
 * @details
 * Call this function in a driver with per-packet rate selection support to combine the rate info
 * in the packet tx info with the most recent rate selection table for the station entry.
 ****************************************************************************************
 */
void i3ed11_get_tx_rates(struct i3ed11_vif *vif,
						 struct i3ed11_sta *sta,
						 struct wpktbuff *wpkt,
						 struct i3ed11_tx_rate *dest,
						 int max_rates);

/**
 ****************************************************************************************
 * @brief Transmit status callback.
 * @param[in] hw The hardware the frame was transmitted by.
 * @param[in] wpkt The frame that was transmitted, owned by macd11 after this call.
 * @details
 * Call this function for all transmitted frames after they have been transmitted.\n
 * It is permissible to not call this function for multicast frames but this can affect statistics.\n\n
 * This function may not be called in IRQ context.\n
 * Calls to this function for a single hardware must be synchronized against each other.\n
 * Calls to this function,
 * i3ed11_tx_status_ni() and i3ed11_tx_status_irqsafe() may not be mixed for a single hardware.\n
 * Must not run concurrently with i3ed11_rx() or i3ed11_rx_ni().
 ****************************************************************************************
 */
void i3ed11_tx_status(struct i3ed11_hw *hw, struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief IRQ-safe transmit status callback.
 * @details Like i3ed11_tx_status() but can be called in IRQ context (internally defers to a tasklet).
 * @param[in] hw The hardware the frame was transmitted by.
 * @param[in] wpkt The frame that was transmitted, owned by macd11 after this call.
 * @note Calls to this function, i3ed11_tx_status() and i3ed11_tx_status_ni() may not be mixed
 * for a single hardware.
 ****************************************************************************************
 */
void i3ed11_tx_status_irqsafe(struct i3ed11_hw *hw, struct wpktbuff *wpkt);

/**
 ****************************************************************************************
 * @brief Beacon generation function.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] tim_offset Pointer to variable that will receive the TIM IE offset.\n
 * Set to 0 if invalid (in non-AP modes).
 * @param[in] tim_length Pointer to variable that will receive the TIM IE length,
 * (including the ID and length bytes!).\n
 * Set to 0 if invalid (in non-AP modes).
 * @return The beacon template. %NULL on error.
 ****************************************************************************************
 */
struct wpktbuff *i3ed11_beacon_get_tim(struct i3ed11_hw *hw,
									   struct i3ed11_vif *vif,
									   u16 *tim_offset, u16 *tim_length);

/**
 ****************************************************************************************
 * @brief Beacon generation function.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @see i3ed11_beacon_get_tim().
 * @return See i3ed11_beacon_get_tim().
 ****************************************************************************************
 */
static inline struct wpktbuff *i3ed11_beacon_get(struct i3ed11_hw *hw,
		struct i3ed11_vif *vif)
{
	return i3ed11_beacon_get_tim(hw, vif, NULL, NULL);
}

/**
 ****************************************************************************************
 * @brief Retrieve a PS Poll template.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @details
 * Creates a PS Poll a template which can, for example, uploaded to hardware.\n
 * The template must be updated after association so that correct AID, BSSID and MAC address is used.
 * @note Caller (or hardware) is responsible for setting the &I3ED11_FCTL_PM bit.
 * @return The PS Poll template. %NULL on error.
 ****************************************************************************************
 */
struct wpktbuff *i3ed11_pspoll_get(struct i3ed11_hw *hw, struct i3ed11_vif *vif);

/**
 ****************************************************************************************
 * @brief Retrieve a nullfunc template.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @details
 * Creates a Nullfunc template which can, for example, uploaded to hardware.\n
 * The template must be updated after association so that correct BSSID and address is used.
 * @note Caller (or hardware) is responsible for setting the &I3ED11_FCTL_PM bit
 * as well as Duration and Sequence Control fields.
 * @return The nullfunc template. %NULL on error.
 ****************************************************************************************
 */
struct wpktbuff *i3ed11_nullfunc_get(struct i3ed11_hw *hw,
									 struct i3ed11_vif *vif);

struct wpktbuff *i3ed11_nullfunction_get(struct i3ed11_hw *hw,
										 struct i3ed11_vif *vif, u8 *addr);

/**
 ****************************************************************************************
 * @brief Retrieve a Probe Request template.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] ssid SSID buffer.
 * @param[in] ssid_len Length of SSID.
 * @param[in] tailroom Tailroom to reserve at end of SKB for IEs.
 * @details Creates a Probe Request template which can, for example, be uploaded to hardware.
 * @return The Probe Request template. %NULL on error.
 ****************************************************************************************
 */
struct wpktbuff *i3ed11_probereq_get(struct i3ed11_hw *hw,
									 struct i3ed11_vif *vif,
									 const u8 *ssid, size_t ssid_len,
									 size_t tailroom);

/**
 ****************************************************************************************
 * @brief Accessing buffered broadcast and multicast frames.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @return A pointer to the next buffered wpkt or NULL if no more buffered frames are available.
 ****************************************************************************************
 */
struct wpktbuff *
i3ed11_get_buffered_bc(struct i3ed11_hw *hw, struct i3ed11_vif *vif);

/// Key sequence counter.
struct i3ed11_key_seq
{
	union
	{
		/// TKIP data, containing IV32 and IV16 in host byte order.
		struct
		{
			u32 iv32;
			u16 iv16;
		} tkip;
		/// PN data, most significant byte first (big endian, reverse order than in packet).
		struct
		{
			u8 pn[6];
		} ccmp;
		/// PN data, most significant byte first (big endian, reverse order than in packet).
		struct
		{
			u8 pn[6];
		} aes_cmac;
	};
};

/**
 ****************************************************************************************
 * @brief Wake specific queue.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @param[in] queue Queue number (counted from zero).
 * @details Drivers should use this function instead of netif_wake_queue.
 ****************************************************************************************
 */
void i3ed11_wake_queue(struct i3ed11_hw *hw, int queue);

/**
 ****************************************************************************************
 * @brief Stop specific queue.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @param[in] queue Queue number (counted from zero).
 * @details Drivers should use this function instead of netif_stop_queue.
 ****************************************************************************************
 */
void i3ed11_stop_queue(struct i3ed11_hw *hw, int queue);

/**
 ****************************************************************************************
 * @brief Test status of the queue.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @param[in] queue Queue number (counted from zero).
 * @details Drivers should use this function instead of netif_stop_queue.
 * @return %true if the queue is stopped. %false otherwise.
 ****************************************************************************************
 */
int i3ed11_queue_stopped(struct i3ed11_hw *hw, int queue);

/**
 ****************************************************************************************
 * @brief Stop all queues.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @details Drivers should use this function instead of netif_stop_queue.
 ****************************************************************************************
 */
void i3ed11_stop_queues(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Wake all queues.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @details Drivers should use this function instead of netif_wake_queue.
 ****************************************************************************************
 */
void i3ed11_wake_queues(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Completed hardware scan.
 * @details
 * When hardware scan offload is used (i.e. the hw_scan() callback is assigned) this function
 * needs to be called by the driver to notify macd11 that the scan finished.\n
 * This function can be called from any context, including hardirq context.
 * @param[in] hw The hardware that finished the scan.
 * @param[in] aborted Set to true if scan was aborted.
 ****************************************************************************************
 */
void i3ed11_scan_completed(struct i3ed11_hw *hw, bool aborted);

/**
 ****************************************************************************************
 * @brief Add work onto the macd11 workqueue.
 * @details
 * Drivers and macd11 use this to add work onto the macd11 workqueue.\n
 * This helper ensures drivers are not queueing work when they should not be.
 * @param[in] hw The hardware struct for the interface we are adding work for.
 * @param[in] work The work we want to add onto the macd11 workqueue.
 ****************************************************************************************
 */
int i3ed11_queue_work(struct i3ed11_hw *hw, struct work_struct *work);

/**
 ****************************************************************************************
 * @brief Add work onto the macd11 workqueue.
 * @details Drivers and macd11 use this to queue delayed work onto the macd11 workqueue.
 * @param[in] hw The hardware struct for the interface we are adding work for.
 * @param[in] dwork Delayable work to queue onto the macd11 workqueue.
 * @param[in] delay Number of jiffies to wait before queueing.
 ****************************************************************************************
 */
void i3ed11_queue_delayed_work(struct i3ed11_hw *hw,
							   struct delayed_work *dwork,
							   unsigned long delay);

/**
 ****************************************************************************************
 * @brief Start a tx Block Ack session.
 * @param[in] sta The station for which to start a BA session.
 * @param[in] tid The TID to BA on.
 * @param[in] timeout Session timeout value (in TUs).
 * @details Although macd11/low level driver/user space application can estimate the need
 * to start aggregation on a certain RA/TID, the session level will be managed by the macd11.
 * @return: success if addBA request was sent, failure otherwise.
 ****************************************************************************************
 */
int i3ed11_start_tx_ba_session(struct i3ed11_sta *sta, u16 tid, u16 timeout);

/**
 ****************************************************************************************
 * @brief Low level driver ready to aggregate.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback
 * @param[in] ra Receiver address of the BA session recipient.
 * @param[in] tid The TID to BA on.
 * @details
 * This function must be called by low level driver once it has finished with preparations for the BA session.
 * It can be called from any context.
 ****************************************************************************************
 */
void i3ed11_start_tx_ba_cb_irqsafe(struct i3ed11_vif *vif, const u8 *ra, u16 tid);

/**
 ****************************************************************************************
 * @brief Stop a Block Ack session.
 * @param[in] sta The station whose BA session to stop.
 * @param[in] tid The TID to stop BA.
 * @return Negative error if the TID is invalid, or no aggregation active.
 * @details Although macd11/low level driver/user space application can estimate the need
 * to stop aggregation on a certain RA/TID, the session level will be managed by the macd11.
 ****************************************************************************************
 */
int i3ed11_stop_tx_ba_session(struct i3ed11_sta *sta, u16 tid);

/**
 ****************************************************************************************
 * @brief Low level driver ready to stop aggregate.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback
 * @param[in] ra Receiver address of the BA session recipient.
 * @param[in] tid The desired TID to BA on.
 * @details
 * This function must be called by low level driver once it has finished with preparations
 * for the BA session tear down.\n
 * It can be called from any context.
 ****************************************************************************************
 */
void i3ed11_stop_tx_ba_cb_irqsafe(struct i3ed11_vif *vif, const u8 *ra, u16 tid);

/**
 ****************************************************************************************
 * @brief Retrieve a Probe Request template.
 * @param[in] hw Pointer obtained from i3ed11_alloc_hw().
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @return The Probe Request template. %NULL on error.
 ****************************************************************************************
 */
struct wpktbuff *i3ed11_ap_probereq_get(struct i3ed11_hw *hw,
										struct i3ed11_vif *vif);

/**
 ****************************************************************************************
 * @brief Inform hardware has lost connection to the AP.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @details
 * When beacon filtering is enabled with %I3ED11_VIF_BEACON_FILTER,
 * and %I3ED11_CONF_PS and %I3ED11_HW_CONNECTION_MONITOR are set,
 * the driver needs to inform if the connection to the AP has been lost.\n
 * The function may also be called if the connection needs to be terminated for some other reason,
 * even if %I3ED11_HW_CONNECTION_MONITOR isn't set.\n\n
 * This function will cause immediate change to disassociated state, without connection recovery attempts.
 ****************************************************************************************
 */
void i3ed11_connection_loss(struct i3ed11_vif *vif);

/**
 ****************************************************************************************
 * @brief Disconnect from AP after resume.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @details
 * Instructs macd11 to disconnect from the AP after resume.\n
 * Drivers can use this after WoWLAN if they know that the connection cannot be kept up,
 * for example because keys were used while the device was asleep
 * but the replay counters or similar cannot be retrieved from the device during resume.\n\n
 * Note that due to implementation issues,
 * if the driver uses the reconfiguration functionality during resume the interface will still be added
 * as associated first during resume and then disconnect normally later.\n\n
 * This function can only be called from the resume callback and the driver must not be holding
 * any of its own locks while it calls this function,
 * or at least not any locks it needs in the key configuration paths (if it supports HW crypto).
 ****************************************************************************************
 */
void i3ed11_resume_disconnect(struct i3ed11_vif *vif);

/**
 ****************************************************************************************
 * @brief Inform a configured connection quality monitoring rssi threshold triggered.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] rssi_event The RSSI trigger event type.
 * @param[in] gfp Context flags.
 * @details
 * When the %I3ED11_VIF_SUPPORTS_CQM_RSSI is set,
 * and a connection quality monitoring is configured with an rssi threshold,\n
 * the driver will inform whenever the rssi level reaches the threshold.
 ****************************************************************************************
 */
void i3ed11_cqm_rssi_notify(struct i3ed11_vif *vif,
							enum nld11_cqm_rssi_threshold_event rssi_event,
							gfp_t gfp);

/**
 ****************************************************************************************
 * @brief Notification of remain-on-channel start.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 ****************************************************************************************
 */
void i3ed11_ready_on_channel(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Remain_on_channel duration expired.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 ****************************************************************************************
 */
void i3ed11_remain_on_channel_expired(struct i3ed11_hw *hw);

/**
 ****************************************************************************************
 * @brief Callback to stop existing BA sessions.
 * @details
 * In order not to harm the system performance and user experience,
 * the device may request not to allow any rx ba session
 * and tear down existing rx ba sessions based on system constraints such as periodic BT activity
 * that needs to limit wlan activity (eg.sco or a2dp).\n
 * In such cases, the intention is to limit the duration of the rx ppdu and therefore prevent the peer device
 * to use a-mpdu aggregation.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] ba_rx_bitmap Bit map of open rx ba per tid.
 * @param[in] addr & to bssid mac address.
 ****************************************************************************************
 */
void i3ed11_stop_rx_ba_session(struct i3ed11_vif *vif, u16 ba_rx_bitmap,
							   const u8 *addr);

/**
 ****************************************************************************************
 * @brief Send a BlockAckReq frame.
 * @details Can be used to flush pending frames from the peer's aggregation reorder buffer.
 * @param[in] vif &struct i3ed11_vif pointer from the add_interface callback.
 * @param[in] ra The peer's destination address.
 * @param[in] tid The TID of the aggregation session.
 * @param[in] ssn The new starting sequence number for the receiver.
 ****************************************************************************************
 */
void i3ed11_send_bar(struct i3ed11_vif *vif, u8 *ra, u16 tid, u16 ssn);

/* Rate control API */

/// Rate control information for/from RC algo.
struct i3ed11_tx_rate_control
{
	/// The hardware the algorithm is invoked for.
	struct i3ed11_hw *hw;
	/// The band this frame is being transmitted on.
	struct i3ed11_supported_band *diwband;
	/// The current BSS configuration.
	struct i3ed11_bss_conf *bss_conf;
	/// Control information in it needs to be filled in the transmitted wpkt.
	struct wpktbuff *wpkt;
	/// Should be reported to userspace as the current rate.
	struct i3ed11_tx_rate reported_rate;
	/// If RTS will be used because it is longer than the RTS threshold.
	bool rts;
	/// If macd11 will request short-preamble transmission if the selected rate supports it.
	bool short_preamble;
	/// User-requested maximum (legacy) rate (deprecated; this will be removed once drivers get updated to use rate_idx_mask).
	u8 max_rate_idx;
	/// User-requested (legacy) rate mask.
	u32 rate_idx_mask;
	/// User-requested MCS rate mask (NULL if not in use).
	u8 *rate_idx_mcs_mask;
	/// If this frame is sent out in AP or IBSS mode.
	bool bss;
};

/// Rate control ops.
struct diw_rc_ops
{
	struct module *module;
	const char *name;
	void *(*alloc)(struct i3ed11_hw *hw);
	void (*free)(void *priv);

	void *(*alloc_sta)(void *priv, struct i3ed11_sta *sta, gfp_t gfp);
	void (*rate_init)(void *priv, struct i3ed11_supported_band *diwband,
					  struct cfgd11_chan_def *chandef,
					  struct i3ed11_sta *sta, void *priv_sta);
	void (*rate_update)(void *priv, struct i3ed11_supported_band *diwband,
						struct cfgd11_chan_def *chandef,
						struct i3ed11_sta *sta, void *priv_sta,
						u32 changed);
	void (*free_sta)(void *priv, struct i3ed11_sta *sta,
					 void *priv_sta);

	void (*tx_status)(void *priv, struct i3ed11_supported_band *diwband,
					  struct i3ed11_sta *sta, void *priv_sta,
					  struct wpktbuff *wpkt);
	void (*get_rate)(void *priv, struct i3ed11_sta *sta, void *priv_sta,
					 struct i3ed11_tx_rate_control *txrc);
};

static inline int rate_supported(struct i3ed11_sta *sta,
								 enum i3ed11_band band,
								 int index)
{
	return (sta == NULL || sta->supp_rates[band] & BIT(index));
}

/**
 ****************************************************************************************
 * @brief Helper for drivers for management/no-ack frames.
 * @details
 * Rate control algorithms that agree to use the lowest rate to send management frames
 * and NO_ACK data with the respective hw retries
 * should use this in the beginning of their macd11 get_rate callback.\n
 * If true is returned the rate control can simply return.\n
 * If false is returned we guarantee that sta and sta and priv_sta is not null.\n\n
 * Rate control algorithms wishing to do more intelligent selection of rate for multicast/broadcast frames
 * may choose to not use this.
 * @param[in] sta &struct i3ed11_sta pointer to the target destination. Note that this may be null.
 * @param[in] priv_sta Private rate control structure. This may be null.
 * @param[in] txrc Rate control information we sholud populate for macd11.
 ****************************************************************************************
 */
bool diw_rc_send_low(struct i3ed11_sta *sta,
					 void *priv_sta,
					 struct i3ed11_tx_rate_control *txrc);

static inline s8
rate_lowest_index(struct i3ed11_supported_band *diwband,
				  struct i3ed11_sta *sta)
{
	int i;

	for (i = 0; i < diwband->n_bitrates; i++)
		if (rate_supported(sta, diwband->band, i))
		{
			return i;
		}

	/* And return 0 (the lowest index) */
	return 0;
}

static inline
bool rate_usable_index_exists(struct i3ed11_supported_band *diwband,
							  struct i3ed11_sta *sta)
{
	int i;

	for (i = 0; i < diwband->n_bitrates; i++)
		if (rate_supported(sta, diwband->band, i))
		{
			return true;
		}

	return false;
}

/**
 ****************************************************************************************
 * @brief Pass the sta rate selection to macd11/driver.
 * @details
 * When not doing a rate control probe to test rates, rate control should pass its rate selection to macd11.\n
 * If the driver supports receiving a station rate table,
 * it will use it to ensure that frames are always sent based on the most recent rate control module decision.
 * @param[in] hw Pointer as obtained from i3ed11_alloc_hw().
 * @param[in] pubsta &struct i3ed11_sta pointer to the target destination.
 * @param[in] rates New tx rate set to be used for this station.
 ****************************************************************************************
 */
int diw_rc_set_rates(struct i3ed11_hw *hw,
					 struct i3ed11_sta *pubsta,
					 struct i3ed11_sta_rates *rates);

int i3ed11_rc_register(struct diw_rc_ops *ops);
void i3ed11_rc_unregister(struct diw_rc_ops *ops);

static inline enum nld11_iftype
i3ed11_iftype_p2p(enum nld11_iftype type, bool p2p)
{

	if (p2p)
	{
		switch (type)
		{
		case NLD11_IFTYPE_STATION:
					return NLD11_IFTYPE_P2P_CLIENT;

		case NLD11_IFTYPE_AP:
			return NLD11_IFTYPE_P2P_GO;

		default:
			break;
		}
	}

	return type;

}

static inline enum nld11_iftype
i3ed11_vif_type_p2p(struct i3ed11_vif *vif)
{
	return i3ed11_iftype_p2p(vif->type, vif->p2p);
}
#endif /* MACD11_H */
