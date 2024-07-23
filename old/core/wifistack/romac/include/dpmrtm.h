/**
 ****************************************************************************************
 *
 * @file dpmrtm.h
 *
 * @brief Defining an RTM Structure for DPM
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

#ifndef __DPMRTM_H__
#define __DPMRTM_H__

/**
 * @file     dpmrtm.h
 * @brief    Defining an RTM Structure for DPM
 * @details  This header file is used in RTOS & PTIM.
 * @defgroup ROMAC_COM_RTM
 * @ingroup  ROMAC_COM
 * @{
 */
#include "timd.h"

#ifdef TIM_SIMULATION
#define DPM_SCHE_MAX_CNT        4
#else
#define DPM_SCHE_MAX_CNT        16
#endif

#define DPM_PREP_TIME_MAX       4

#define DPM_MAX_SSID            32
#define DPM_MAX_ELE             8
#define DPM_PHY_CFG_BUF_SIZE    16
#define DPM_DPDGAININFO_SIZE    2
#define DPM_ANTDIV_SIZE         1
#define DPM_ACCEPT_MC_CNT       8
#define DPM_ACCEPT_MC_IP_CNT    8
#define DPM_MAX_UDP_FILTER      8
#define DPM_MAX_TCP_FILTER      8

#define DPM_ADC_CAL_CNT         20
#define DPM_PREAMBLE            0x600DF00D
#define DPM_WDOG_PREAMBLE       0xa7a7a7a7
//#define DPM_INF_INITED_PREAMBLE 0xC0DEC1EA

#define KEY_STORAGE_SIZE        8

/* There are two interface for short-term & long-term */
#define TIMP_INTERFACE_CNT      2
#define TIMP_BANK_CNT           2

#define DPM_SCHE_PERIOD_TY_MASK 0x000c
#define DPM_SCHE_PERIOD_TY_POS  2
#define DPM_SCHE_PREP_MASK      0x00e0
#define DPM_SCHE_PREP_POS       5

#define PRODUCT_INFO_RF9050     0x30353039
#define PRODUCT_INFO_RF9060     0x30363039

#ifdef TIM_SIMULATION
#pragma pack(push, 1)
#endif

struct bcn_offset {
	// 0x180500 ~ 0x18053f
	unsigned short bcn_loss_cnt;
	short          bcn_stage;

	unsigned char  ap_sync_en: 8;
	unsigned char  lock: 4;
	unsigned char  tracking_updated: 4;
	short          ready_time: 16;
	long           optimal_tbtt;
	long long      r0;          /* BCN 0 */
	long long      r1;          /* BCN 1 */
	long           reserved0;
	long           min_d;
	long           min_to;
	long           min_d_to;
	long           min_tsf_offset; /* deprecated */

	/* Tracking Mode */
	short          cnt;
	short          reserved1;
	short          unlock_max_cnt;
	short          coarse_max_cnt;
	short          over_max_cnt;
	short          under_max_cnt;
};

struct dpm_txiv {
	// CCMP, TKIP, WEP
	unsigned char  iv[8];
	unsigned short key_idx;
	unsigned char  iv_len;

	// WEP
	unsigned char wep_key_len;

	// TKIP
	unsigned char mic_key[8];
};

struct dpm_status {
	/// symbol 0xa7a7a7a7
	unsigned long  wdog_preamble;
	unsigned long  trigger_ue;
	unsigned long  trigger_fboot;
	unsigned long  trigger_ra;
	unsigned long  trigger_ptim;
	unsigned long  tim_app_cnt;
	//unsigned long  mode; /* 0: Normal, 1: Full */
	unsigned long  status;

	int            pm_status: 1;
	int            bcn_sync_status: 1;
	int            rssi_status: 1;
	int            reserved2: 26;
	int            ps_type: 2;            ///< smart ps-poll type
	int            determined_ps_type: 1; ///< determining ps-poll type

	unsigned long  ele_crc;

	unsigned char  current_ch; /* DSSS Parameter Set */
	unsigned char  ssid_check_sum;
	unsigned short max_bcn_size;  ///< The size with 4 bytes of CRC

	//unsigned char  bcn_ready_clk; /* Beacon ready time */
	unsigned short max_bcn_duration;
	/// previous DTIM CNT
	unsigned char  prev_dtim_cnt;
	unsigned char  bcn_chk_cnt;

	unsigned short tim_off; /* TIM element offset */
	unsigned char  reserved3;
	signed char    rssi;

	unsigned char  ps_data_cnt[3];
	unsigned char  nouc_cnt;

	unsigned long  bcn_chk_step;

	//long long      sleep_time;

	/// The next wake-up clk expected just before calling the tim_sleep function
	long long      expected_tbtt_clk;
	long long      last_bcn_clk;
	/// The time to full-boot power down
	long long      pd_clk;
	/// The time to ptim power down
	long long      ptim_pd_clk;

	/// tx_seqn is the sequence number of Non-QoS.
	unsigned short tx_seqn;
	/// tx_qos_seqn is the sequence number of QoS.
	unsigned short tx_qos_seqn[5];

	unsigned short rx_seqn;
	unsigned short rx_qos_seqn[5];

	unsigned long  reserved[32];
};

struct timp_feature_ty {
	union {
		unsigned short  feature;
		struct {
			unsigned short reserved: 12;
			// Enable TIMP_0 Weak-signal
			unsigned char weaksig_0: 1;
			// Enable TIMP_1 Weak-signal
			unsigned char weaksig_1: 1;
			// Enable TIMP_0 Update
			unsigned char update_0: 1;
			// Enable TIMP_1 Update
			unsigned char update_1: 1;
		};
	};

	unsigned short sz;
};

struct tim_feature_ty {
	union {
		unsigned short feature;
		struct {
			// Enable TIM
			unsigned char en: 1;
			// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;        // Don't care
			unsigned char prep: 3;
			// Wait for Beacon to receive.
			// Used when response from AP but no beacon is received.
			unsigned char wait_bcn: 1;
			unsigned char detect_ssid: 1;
			unsigned char detect_channel: 1;
			unsigned char dont_care_loss: 1;
			unsigned char pbr: 1;
			unsigned char bcn_sync: 1;
			unsigned char bcn_sync_wait: 1;
			unsigned char reserved: 1;
		};
	};
	unsigned char  bcn_timeout_tu;
	unsigned char  chk_cnt;

	unsigned long  chk_step;

	unsigned char  bcn_sync_clk;
	unsigned char  reserved1;
	unsigned short reserved2;

	long long      reserved3;
};

struct bcmc_feature_ty {
	union {
		unsigned short feature;
		struct {
			unsigned char en: 1;        ///< Waiting for BC/MC
			// Constance period
			unsigned char fix: 1;       // Don't care
			unsigned char period_ty: 2; // Don't care
			unsigned char ta: 1;        // Don't care
			unsigned char prep: 3;      // Don't care
			unsigned char morechk: 1;
			unsigned char rx_timeout_control: 1;
			unsigned char reserved: 6;
		};
	};

	unsigned char  rx_timeout_tu;
	unsigned char  timeout_tu;

	unsigned long  accept_oui[DPM_ACCEPT_MC_CNT];

	unsigned char  max;
	unsigned char  reserved1;
	unsigned short reserved2;
	long  long     reserved3;
};

struct ps_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable PS-POLL
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char reserved: 8;
			/*unsigned char typecnt: 4;*/
		};
	};

	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned char  nouc_max;
	unsigned char  ps_data_max;
	unsigned short reserved1;
	long long      reserved2;
	long long      reserved3;
};

struct uc_feature_ty {
	union {
		unsigned short feature;
		struct {
			unsigned char en: 1;        ///< Waiting for UC
			// Constance period
			unsigned char fix: 1;       // Don't care
			unsigned char period_ty: 2; // Don't care
			unsigned char ta: 1;        // Don't care
			unsigned char prep: 3;      // Don't care
			unsigned char morechk: 1;
			unsigned char rx_timeout_control: 1;
			unsigned char reserved: 6;
		};
	};

	unsigned char  rx_timeout_tu;
	unsigned char  timeout_tu;

	unsigned char  max;
	unsigned char  reserved1;
	unsigned short reserved2;
	long long      reserved3;
	long long      reserved4;
};

struct ka_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable KA
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			/// Attempt KA from last TX time (TX traffic detect)
			unsigned char td: 1;
			unsigned char reserved: 7;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;
	long long      reserved1;
	long long      reserved2;
};

/* ARP Rx */
struct arp_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable Auto ARP
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char type: 2;
			unsigned char reserved: 6;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	unsigned short target_mac[4];
	unsigned long  target_ip;
	long long      reserved1;
	long long      reserved2;
};

/* STA sends ARP response to AP */
struct arpresp_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable ARP Response
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char reserved: 8;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	unsigned short da[3];         ///< Destination MAC
	unsigned short sender_mac[3]; ///< Sender MAC of ARP Request
	unsigned long  sender_ip;     ///< Sender IP of ARP Request
	long long      reserved1;
	long long      reserved2;
};

struct udph_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable UDP hole-punch
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char reserved: 8;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	unsigned short sport;    ///< source port
	unsigned short dport;    ///< destination port
	unsigned short target_mac[4];
	unsigned long  dip;      ///< destination IP

	unsigned int   type: 8;
	unsigned int   payload_len: 24;
	unsigned long  user_data; ///< The base address of user data
	long long      reserved1;
	long long      reserved2;
};

struct setps_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable set-ps mode
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char reserved: 8;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;
	long long      reserved1;
	long long      reserved2;
};

struct arpreq_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable Auto ARP
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char type: 2;
			unsigned char reserved: 6;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	unsigned short target_mac[4];
	unsigned long  target_ip;
	long long      reserved1;
	long long      reserved2;
};

struct tcpka_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable Auto ARP
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			unsigned char type: 2;
			unsigned char reserved: 6;
		};
	};
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	unsigned short target_mac[4];
	unsigned long  target_ip;
	long long      reserved1;
	long long      reserved2;
	long long      reserved3;
	long long      reserved4;
};

struct deauth_feature_ty {
	union {
		unsigned short feature;
		struct {
			/// Enable deauth check
			unsigned char en: 1;
			/// Constance period
			unsigned char fix: 1;
			unsigned char period_ty: 2;
			unsigned char ta: 1;
			unsigned char prep: 3;
			/// Null frame type: 0, ARP req type: 1
			unsigned char type: 1;
			unsigned char reserved: 7;
		};
	};

	/// Timeout that KA or ARP REQ for deauth check
	unsigned char  timeout_tu;
	unsigned char  retry;

	unsigned long  period;

	/// Deauthentification  & Deassociation or ARP response frame waiting time to receive
	unsigned char  chk_timeout_tu;
	unsigned char  reserved1;
	unsigned short reserved2;
	long long      reserved3;
	long long      reserved4;
};

struct dpm_env {
	unsigned long  version;
	unsigned long  feature;
	unsigned long  timd_feature;
	unsigned long  dbg_feature;
	unsigned long  rx_filter;
	//unsigned long  ip_filter;
	unsigned long  mac_rx_filter;

	unsigned char  sys_clk;
	unsigned char  sys_clk2;
	unsigned char  mac_clk;
	unsigned char  reserved_clk;

	unsigned short ptim_timeout_tu;
	unsigned short fixed_tbtt;    /* excluding APTRK */

	unsigned short bcn_int;  /* TUs */
	unsigned short half_int; /* TUs */

	/// force_period is generally a unit of BCN_INT. On the other hand,
	/// the Test mode is the unit us to determine the exact sleep time.
	unsigned long  force_period;

	unsigned short listen_intv;
	unsigned char  dtim_period;
	unsigned char  chnum;

	unsigned short aid;
	unsigned short freq;

	unsigned long  ip;
	unsigned long  ap_ip;

	unsigned short my_mac[3];
	unsigned short bssid[3];

	unsigned long  basic_rates;
	unsigned long  edca[4];
	unsigned long  max_tx_power;

	unsigned char  ele[DPM_MAX_ELE];

	unsigned char  ssid[DPM_MAX_SSID];

	unsigned short ssid_len;
	signed char    rssi_thold;
	signed char    rssi_hyst;

	unsigned long  mc_ip[DPM_ACCEPT_MC_IP_CNT];
	unsigned short udp_dport[DPM_MAX_UDP_FILTER];
	unsigned short tcp_dport[DPM_MAX_TCP_FILTER];

	struct tim_feature_ty tim;
	struct bcmc_feature_ty bcmc;
	struct ka_feature_ty ka;
	struct ps_feature_ty ps;
	struct uc_feature_ty uc;
	struct arp_feature_ty arp;
	struct arpresp_feature_ty arpresp;
	struct udph_feature_ty udph;
	struct setps_feature_ty setps;
	struct arpreq_feature_ty arpreq;
	struct deauth_feature_ty deauth;
	struct timp_feature_ty timp;
	struct tcpka_feature_ty tcpka;

	unsigned long reserved[8];
};

struct dpm_rtclk {
	long long clk;
	long long wakeup_clk;
};

struct dpm_schedule_node {
	unsigned long long function_bit;
	union {
		unsigned long long next_count;
		unsigned long long interval;
	};
	///< CLK
	unsigned int       arbitrary: 26;
	///< Check DTIM_CNT. If DTIM_CNT is not 0, adjust the time.
	unsigned int       align : 1;
	unsigned int       half: 1;
	unsigned int       preparation: 4;
};

struct dpm_schedule {
	unsigned long long counter;

	/* RTC CLK */
	unsigned char      prep_clk[DPM_PREP_TIME_MAX];

	///< If the sleep time is less than min_sleep_time, the sleep time is recalculated.
	unsigned short     min_sleep_time;
	unsigned char      post_prep;
	unsigned char      reserved0;

	long long          reserved1;
	long long          reserved2;

	struct dpm_schedule_node node[DPM_SCHE_MAX_CNT];
};

struct dpm_aptrk {
	unsigned int   en: 8;
	unsigned int   lock: 4;
	unsigned int   tracking_updated: 4;
	unsigned int   tsf_normalization: 16; /* Normalization for TSF */

	/* aptrk_timeout_tu should not be greater than or equal to 255. */
	unsigned long  fine_lock_max;

	unsigned char  aptrk_timeout_tu;
	unsigned short coarse_lock_max;
	unsigned char  under_max_cnt;

	unsigned long  over_max_cnt;
	unsigned long  cnt;
	long           ready_time; ///< ready_time is impending time when aptrk is unlock.
	long           guard_time; ///< guard_time is the miniumu D.
	long           impending_time; ///< impending_time is optimal time.

	long long      start_clk;       ///< RTC CNT after association
	long long      start_timestamp; ///< BCN TSF after association
	long long      bcn_clk;         ///< CLK of last received BCN
	long long      bcn_timestamp;   ///< Timestamp of last received BCN
	long long      apsync_update;

	long           min_d;
	long           min_to;
	long           min_cca;

	union {
		long   reserved[16];
		struct bcn_offset bo;
	};
};

struct dpm_adccal {
	unsigned long parameters[DPM_ADC_CAL_CNT];
};

/*struct dpm_phy_cfg {
	unsigned long parameters[DPM_PHY_CFG_BUF_SIZE];
};*/

struct dpm_phy_otp_cal {

        ///< OTP Tx Power Offset RR reg#1
        unsigned long reg1;
        ///< OTP Tx Power Offset RR reg#1
        unsigned long reg2;
};

struct dpm_dpdgaininfo {
	///< DPD GAIN OUT3
	unsigned long out3;

	///< DPD LUT SCALER1
	unsigned long lut_scaler1;
};

struct dpm_phy_cfg {
	union {
		unsigned long parameters[DPM_PHY_CFG_BUF_SIZE];
		struct {
			unsigned long reserved0;
			unsigned long rf_product_info;

			///< RF OP SKIP
			unsigned long rf_op_skip;

			///< CSF RX CN
			unsigned long csf_rx_cn;

			///< OTP XTAL40 OFFSET
			unsigned long otp_xtal40_offset;

			struct dpm_dpdgaininfo dpdgain;

			///< CURR TEMP
			unsigned long curr_temp;

			///< RX PGA1
			unsigned short rx_pga1;
			///< RX PGA1
			unsigned short rx_pga2;

			///< RX BUFF
			unsigned short rx_buff;
			///< TX MIX
			unsigned short tx_mix;

			///< TX PGA1
			unsigned short tx_pga1;
			///< TX PGA1
			unsigned short tx_pga2;

			///< TX IQC
			unsigned long tx_iqc;

			struct dpm_phy_otp_cal otp_cal;

			///< CAL RTC1
			unsigned long calrtc_1;
			///< CAL RTC2
			unsigned long calrtc_2;
		};
	};
};

struct dpm_antdiv {
	unsigned long parameters[DPM_ANTDIV_SIZE];
};

/* Total 30 x 4 bytes */
struct timp_data_tag {
	unsigned long  tim_count;
	unsigned long  rx_on_time_tu;

	unsigned long  bcn_rx_count;
	unsigned long  bcn_timeout_count;

	unsigned short connection_loss_cnt;
	unsigned short deauth_frame_count;

	/*unsigned short tx_ka_count;
	unsigned short tx_bcn_loss_confirm_count;
	unsigned short tx_autoarp_count;
	unsigned short tx_arpresp_count;
	unsigned short tx_arpreq_count;
	unsigned short tx_pspoll_count;
	unsigned short tx_udph_count;
	unsigned short tx_tcp_ka_count;

	unsigned long  rx_uc_count;
	unsigned long  rx_bc_count;
	unsigned long  rx_mc_count;*/

	unsigned long  reserved[11];
};

struct timp_interface_tag {
	unsigned long update_period_tu[TIMP_INTERFACE_CNT];
	unsigned long start_time_tu[TIMP_INTERFACE_CNT];
	struct timp_data_tag d[TIMP_INTERFACE_CNT][TIMP_BANK_CNT];
	struct timp_data_tag c[TIMP_INTERFACE_CNT];
	unsigned char bank[TIMP_INTERFACE_CNT];
};

struct dpm_keyconf {
	unsigned long enc_key[4];
	unsigned long enc_wpi[4];
	unsigned long enc_cntrl;
	unsigned short mac_addr[3];
	unsigned char key_index;
	unsigned char in_use;
};

struct dpm_mm_req {
	/// Structure containing the parameters of the @ref MM_START_REQ message
	struct start_req {
		/// PHY configuration
		//struct phy_cfg_tag phy_cfg;
		unsigned long phy_cfg[16];
		/// UAPSD timeout
		unsigned int uapsd_timeout;
		/// Local LP clock accuracy (in ppm)
		unsigned short lp_clk_accuracy;
		unsigned short reserved;
	} start;

	/// Structure containing the parameters of the @ref MM_ADD_IF_REQ message.
	struct add_if_req
	{
		/// Type of the interface (AP, STA, ADHOC, ...)
		unsigned char type;
		unsigned char reserved;
		/// MAC ADDR of the interface to start
		unsigned char addr[6];
		/// P2P Interface
		unsigned char p2p;
		unsigned char reserved1[3];
	} add_if;

	/// Structure containing the parameters of the @ref MM_SET_BSSID_REQ message
	struct set_bssid_req
	{
		/// BSSID to be configured in HW
		//struct mac_addr bssid;
		unsigned char bssid[6];
		/// Index of the interface for which the parameter is configured
		unsigned char inst_nbr;
		unsigned char reserved;
	} set_bssid;

	/// Structure containing the parameters of the @ref MM_SET_BEACON_INT_REQ message
	struct set_beacon_int_req
	{
		/// Beacon interval
		unsigned short beacon_int;
		/// Index of the interface for which the parameter is configured
		unsigned char inst_nbr;
		unsigned char reserved;
	} set_beacon_int;

	/// Structure containing the parameters of the @ref MM_SET_BASIC_RATES_REQ message
	struct set_basic_rates_req
	{
		/// Basic rate set (as expected by bssBasicRateSet field of Rates MAC HW register)
		unsigned int rates;
		/// Index of the interface for which the parameter is configured
		unsigned char inst_nbr;
		/// Band on which the interface will operate
		unsigned char band;
		unsigned short reserved;
	} set_basic_rates;

	/// Structure containing the parameters of the @ref MM_SET_EDCA_REQ message
	struct set_edca_req
	{
		/// EDCA parameters of the queue (as expected by edcaACxReg HW register)
		unsigned int ac_param;
		/// Flag indicating if UAPSD can be used on this queue
		unsigned char uapsd;
		/// HW queue for which the parameters are configured
		unsigned char hw_queue;
		/// Index of the interface for which the parameters are configured
		unsigned char inst_nbr;
		unsigned char reserved;
	} edca[4];

	/// Structure containing the parameters of the @ref MM_SET_VIF_STATE_REQ message
	struct set_vif_state_req
	{
		/// Association Id received from the AP (valid only if the VIF is of STA type)
		unsigned short aid;
		/// Flag indicating if the VIF is active or not
		unsigned char active;
		/// Interface index
		unsigned char inst_nbr;
	} set_vif_state;

	/// Structure containing the parameters of the @ref MM_SET_POWER_REQ message
	struct set_power_req
	{
		/// TX power (formatted as expected by the MAX_POWER_LEVEL MAC HW register)
		unsigned int power;
	} set_power;

	/// Structure containing the parameters of the @ref MM_SET_SLOTTIME_REQ message
	struct set_slottime_req
	{
		/// Slot time expressed in us
		unsigned char slottime;
		unsigned char reserved;
		unsigned short reserved1;
	} set_slottime;

	/// Structure containing the parameters of the @ref MM_BA_ADD_REQ message.
	struct ba_add_req
	{
		///Type of agreement (0: TX, 1: RX)
		unsigned char  type;
		///Index of peer station with which the agreement is made
		unsigned char  sta_idx;
		///TID for which the agreement is made with peer station
		unsigned char  tid;
		///Buffer size - number of MPDUs that can be held in its buffer per TID
		unsigned char  bufsz;
		/// Start sequence number negotiated during BA setup - the one in first aggregated MPDU counts more
		unsigned short ssn;
		unsigned short reserved;
	} ba_add;

	/// Structure containing the parameters of the @ref MM_SET_PS_OPTIONS_REQ message.
	struct set_ps_options_req
	{
		/// VIF Index
		unsigned char vif_index;
		/// Listen interval (0 if wake up shall be based on DTIM period)
		unsigned short listen_interval;
		/// Flag indicating if we shall listen the BC/MC traffic or not
		unsigned char dont_listen_bc_mc;
	} set_ps_options;

	/// Structure containing the parameters of the @ref MM_SET_FILTER_REQ message
	struct set_filter_req
	{
		/// RX filter to be put into rxCntrlReg HW register
		unsigned int filter;
	} set_filter;

	struct dpm_keyconf keyc[KEY_STORAGE_SIZE];
};

/**
 * @struct dpm_param
 */
struct dpm_param {
	/// The DPMP guard symbol. The value is 0x0df00d60 */
	unsigned long preamble;

	struct dpm_adccal adc;
	struct dpm_phy_cfg phy;
	//struct dpm_dpdgaininfo dpd;
	struct dpm_antdiv ant;

	struct dpm_env env;
	struct dpm_mm_req mm;
	struct dpm_status st;
	struct dpm_rtclk rtclk;
	struct dpm_aptrk aptrk;
	struct dpm_txiv txiv;
	struct dpm_schedule sche;

	// TIM Application Statistics
	struct timp_interface_tag timp;
};

#ifdef TIM_SIMULATION
#pragma pack(pop)
#endif

/*******************************************************************************
 *
 * RTM Pointer
 *
 ******************************************************************************/
static inline struct dpm_adccal *dpm_get_adc(struct dpm_param *dpmp)
{
	return &dpmp->adc;
}

static inline struct dpm_phy_cfg *dpm_get_phy(struct dpm_param *dpmp)
{
	return &dpmp->phy;
}

static inline struct dpm_phy_otp_cal *dpm_get_phy_otp_cal(
		struct dpm_param *dpmp)
{
	return &dpmp->phy.otp_cal;
}

static inline struct dpm_dpdgaininfo *dpm_get_dpd(struct dpm_param *dpmp)
{
	return &dpmp->phy.dpdgain;
}

static inline struct dpm_antdiv *dpm_get_ant(struct dpm_param *dpmp)
{
	return &dpmp->ant;
}

static inline struct dpm_env *dpm_get_env(struct dpm_param *dpmp)
{
	return &dpmp->env;
}

static inline struct dpm_status *dpm_get_st(struct dpm_param *dpmp)
{
	return &dpmp->st;
}

static inline struct dpm_rtclk *dpm_get_rtclk(struct dpm_param *dpmp)
{
	return &dpmp->rtclk;
}

static inline struct dpm_aptrk *dpm_get_aptrk(struct dpm_param *dpmp)
{
	return &dpmp->aptrk;
}

static inline struct dpm_schedule *dpm_get_sche(struct dpm_param *dpmp)
{
	return &dpmp->sche;
}

static inline struct timp_interface_tag *dpm_get_timp(struct dpm_param *dpmp)
{
	return &dpmp->timp;
}

static inline struct dpm_keyconf *dpm_get_keyc(struct dpm_param *dpmp)
{
	return &dpmp->mm.keyc[0];
}

static inline struct dpm_txiv *dpm_get_txiv(struct dpm_param *dpmp)
{
	return &dpmp->txiv;
}

static inline struct dpm_mm_req *dpm_get_mm(struct dpm_param *dpmp)
{
	return &dpmp->mm;
}


/*******************************************************************************
 *
 * RTM EVN Interface
 *
 ******************************************************************************/
static inline void dpm_set_tim_version(struct dpm_param *dpmp,
		unsigned long version)
{
	dpmp->env.version = version;
}

static inline unsigned long dpm_get_tim_version(struct dpm_param *dpmp)
{
	return dpmp->env.version;
}

static inline unsigned long dpm_get_env_feature(struct dpm_param *dpmp)
{
	return dpmp->env.feature;
}

static inline void dpm_set_env_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.feature = feat;
}

/*static inline unsigned long dpm_get_env_tx_feature(struct dpm_param *dpmp)
{
	return dpmp->env.tx_feature;
}

static inline void dpm_set_env_tx_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.tx_feature = feat;
}*/

static inline unsigned long dpm_get_env_timd_feature(struct dpm_param *dpmp)
{
	return dpmp->env.timd_feature;
}

static inline void dpm_set_env_timd_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.timd_feature = feat;
}

static inline unsigned long dpm_get_env_dbg_feature(struct dpm_param *dpmp)
{
	return dpmp->env.dbg_feature;
}

static inline void dpm_set_env_dbg_feature(struct dpm_param *dpmp, unsigned long feat)
{
	dpmp->env.dbg_feature = feat;
}

static inline unsigned long dpm_get_env_ip(struct dpm_param *dpmp)
{
	return dpmp->env.ip;
}

static inline void dpm_set_env_ip(struct dpm_param *dpmp, unsigned long ip)
{
	dpmp->env.ip = ip;
}

static inline unsigned long dpm_get_env_ap_ip(struct dpm_param *dpmp)
{
	return dpmp->env.ap_ip;
}

static inline void dpm_set_env_ap_ip(struct dpm_param *dpmp, unsigned long ap_ip)
{
	dpmp->env.ap_ip = ap_ip;
}

static inline unsigned short *dpm_get_env_my_mac(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.my_mac[0];
}

static inline void dpm_set_env_my_mac(struct dpm_param *dpmp,
		unsigned char *my_mac)
{
	unsigned char *env_my_mac = (unsigned char *) &dpmp->env.my_mac[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		env_my_mac[i] = my_mac[i];
}

static inline unsigned short *dpm_get_env_bssid(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.bssid[0];
}

static inline void dpm_set_env_bssid(struct dpm_param *dpmp,
		unsigned char *bssid)
{
	unsigned char *env_bssid = (unsigned char *) &dpmp->env.bssid[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		env_bssid[i] = bssid[i];
}

static inline unsigned short dpm_get_env_ssid_len(struct dpm_param *dpmp)
{
	return dpmp->env.ssid_len;
}

static inline void dpm_set_env_ssid_len(struct dpm_param *dpmp,
		unsigned short ssid_len)
{
	dpmp->env.ssid_len = ssid_len;
}

static inline long dpm_get_env_freq(struct dpm_param *dpmp)
{
	return dpmp->env.freq;
}

static inline void dpm_set_env_freq(struct dpm_param *dpmp, long freq)
{
	dpmp->env.freq = freq;
}

static inline long dpm_get_env_aid(struct dpm_param *dpmp)
{
	return dpmp->env.aid;
}

static inline void dpm_set_env_aid(struct dpm_param *dpmp, long aid)
{
	dpmp->env.aid = aid;
}

static inline long dpm_get_env_bcn_int(struct dpm_param *dpmp)
{
	return dpmp->env.bcn_int;
}

static inline void dpm_set_env_bcn_int(struct dpm_param *dpmp, long bcn_int)
{
	dpmp->env.bcn_int = bcn_int;
}

static inline long dpm_get_env_half_int(struct dpm_param *dpmp)
{
	return dpmp->env.half_int;
}

static inline void dpm_set_env_half_int(struct dpm_param *dpmp, long half_int)
{
	dpmp->env.half_int = half_int;
}

static inline unsigned long dpm_get_env_force_period(struct dpm_param *dpmp)
{
	return dpmp->env.force_period;
}

static inline void dpm_set_env_force_period(struct dpm_param *dpmp,
		unsigned long force_period)
{
	int dtim_period = dpmp->env.dtim_period;

	/* Patch */
	/* When dpm_set_env_force_period is invoked, DTIM_PERIOD is not set. */
	if (dtim_period == 0)
		dtim_period = force_period;

	if (force_period <= dtim_period)
		force_period = dtim_period;
	else
		force_period -= (force_period % dtim_period);

	dpmp->env.force_period = force_period;
}

static inline unsigned short dpm_get_env_listen_intv(struct dpm_param *dpmp)
{
	return dpmp->env.listen_intv;
}

static inline void dpm_set_env_listen_intv(struct dpm_param *dpmp,
		unsigned short listen_intv)
{
	dpmp->env.listen_intv = listen_intv;
}

static inline unsigned short dpm_get_env_fixed_tbtt(struct dpm_param *dpmp)
{
	return dpmp->env.fixed_tbtt;
}

static inline void dpm_set_env_fixed_tbtt(struct dpm_param *dpmp,
		unsigned short fixed_tbtt)
{
	dpmp->env.fixed_tbtt = fixed_tbtt;
}

static inline unsigned long dpm_get_env_ptim_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.ptim_timeout_tu;
}

static inline void dpm_set_env_ptim_timeout_tu(struct dpm_param *dpmp,
		unsigned long ptim_timeout_tu)
{
	dpmp->env.ptim_timeout_tu = ptim_timeout_tu;
}

static inline unsigned char dpm_get_env_dtim_period(struct dpm_param *dpmp)
{
	return dpmp->env.dtim_period;
}

static inline void dpm_set_env_dtim_period(struct dpm_param *dpmp,
		unsigned char dtim_period)
{
	unsigned long force_period = dpmp->env.force_period;
	if (force_period <= dtim_period)
		force_period = dtim_period;
	else
		force_period -=	(force_period % dtim_period);

	dpmp->env.force_period = force_period;
	dpmp->env.dtim_period = dtim_period;
}

static inline unsigned char dpm_get_env_chnum(struct dpm_param *dpmp)
{
	return dpmp->env.chnum;
}

static inline void dpm_set_env_chnum(struct dpm_param *dpmp,
		unsigned char chnum)
{
	dpmp->env.chnum = chnum;
}

static inline unsigned long dpm_get_env_basic_rates(struct dpm_param *dpmp)
{
	return dpmp->env.basic_rates;
}

static inline void dpm_set_env_basic_rates(struct dpm_param *dpmp,
		unsigned long basic_rates)
{
	dpmp->env.basic_rates = basic_rates;
}

static inline unsigned long dpm_get_env_edca(struct dpm_param *dpmp,
		unsigned char ac)
{
	return dpmp->env.edca[ac];
}

static inline void dpm_set_env_edca(struct dpm_param *dpmp, unsigned char ac,
		unsigned long edca)
{
	dpmp->env.edca[ac] = edca;
}

static inline unsigned long dpm_get_env_max_tx_power(struct dpm_param *dpmp)
{
	return dpmp->env.max_tx_power;
}

static inline void dpm_set_env_max_tx_power(struct dpm_param *dpmp,
		unsigned long max_power)
{
	dpmp->env.max_tx_power = max_power;
}

/*inline long dpm_get_env_tim_boot_addr(struct dpm_param *dpmp)
{
	return dpmp->env.tim_boot_addr;
}

static inline void dpm_set_env_tim_boot_addr(struct dpm_param *dpmp,
		unsigned long tim_boot_addr)
{
	dpmp->env.tim_boot_addr = tim_boot_addr;
}*/

static inline unsigned char dpm_get_env_sys_clk(struct dpm_param *dpmp)
{
	return dpmp->env.sys_clk;
}

static inline void dpm_set_env_sys_clk(struct dpm_param *dpmp,
		unsigned char sys_clk)
{
	dpmp->env.sys_clk = sys_clk;
}

static inline unsigned char dpm_get_env_sys_clk2(struct dpm_param *dpmp)
{
	return dpmp->env.sys_clk2;
}

static inline void dpm_set_env_sys_clk2(struct dpm_param *dpmp,
		unsigned char sys_clk2)
{
	dpmp->env.sys_clk2 = sys_clk2;
}

static inline unsigned char dpm_get_env_mac_clk(struct dpm_param *dpmp)
{
	return dpmp->env.mac_clk;
}

static inline void dpm_set_env_mac_clk(struct dpm_param *dpmp,
		unsigned char mac_clk)
{
	dpmp->env.mac_clk = mac_clk;
}

static inline unsigned char dpm_get_env_ele(struct dpm_param *dpmp, int n)
{
	return dpmp->env.ele[n];
}

static inline void dpm_set_env_ele(struct dpm_param *dpmp, int n,
		unsigned char ele_id)
{
	dpmp->env.ele[n] = ele_id;
}

static inline unsigned char *dpm_get_env_ssid(struct dpm_param *dpmp)
{
	return &dpmp->env.ssid[0];
}

static inline void dpm_set_env_ssid(struct dpm_param *dpmp,
		unsigned char *ssid, int len)
{
	int i;
	/*for (i = 0; (i < DPM_MAX_SSID) && (i < len); i++)
		dpmp->env.ssid[i] = ssid[i];
	for (; i < DPM_MAX_SSID; i++)
		dpmp->env.ssid[i] = '\0';*/
	for (i = 0; i < DPM_MAX_SSID; i++)
		dpmp->env.ssid[i] = ssid[i];
}

/*static inline unsigned long dpm_get_env_ssid0(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[0];
}

static inline void dpm_set_env_ssid0(struct dpm_param *dpmp, unsigned long ssid0)
{
	dpmp->env.ssid[0] = ssid0;
}

static inline unsigned long dpm_get_env_ssid1(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[1];
}

static inline void dpm_set_env_ssid1(struct dpm_param *dpmp, unsigned long ssid1)
{
	dpmp->env.ssid[1] = ssid1;
}

static inline unsigned long dpm_get_env_ssid2(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[2];
}

static inline void dpm_set_env_ssid2(struct dpm_param *dpmp, unsigned long ssid2)
{
	dpmp->env.ssid[2] = ssid2;
}

static inline unsigned long dpm_get_env_ssid3(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[3];
}

static inline void dpm_set_env_ssid3(struct dpm_param *dpmp, unsigned long ssid3)
{
	dpmp->env.ssid[3] = ssid3;
}

static inline unsigned long dpm_get_env_ssid4(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[4];
}

static inline void dpm_set_env_ssid4(struct dpm_param *dpmp, unsigned long ssid4)
{
	dpmp->env.ssid[4] = ssid4;
}

static inline unsigned long dpm_get_env_ssid5(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[5];
}

static inline void dpm_set_env_ssid5(struct dpm_param *dpmp, unsigned long ssid5)
{
	dpmp->env.ssid[5] = ssid5;
}

static inline unsigned long dpm_get_env_ssid6(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[6];
}

static inline void dpm_set_env_ssid6(struct dpm_param *dpmp, unsigned long ssid6)
{
	dpmp->env.ssid[6] = ssid6;
}

static inline unsigned long dpm_get_env_ssid7(struct dpm_param *dpmp)
{
	return dpmp->env.ssid[7];
}

static inline void dpm_set_env_ssid7(struct dpm_param *dpmp, unsigned long ssid7)
{
	dpmp->env.ssid[7] = ssid7;
}*/

static inline signed char dpm_get_env_rssi_thold(struct dpm_param *dpmp)
{
	return dpmp->env.rssi_thold;
}

static inline void dpm_set_env_rssi_thold(struct dpm_param *dpmp,
		signed char rssi_thold)
{
	dpmp->env.rssi_thold = rssi_thold;
}

static inline signed char dpm_get_env_rssi_hyst(struct dpm_param *dpmp)
{
	return dpmp->env.rssi_hyst;
}

static inline void dpm_set_env_rssi_hyst(struct dpm_param *dpmp,
		signed char rssi_hyst)
{
	dpmp->env.rssi_hyst = rssi_hyst;
}

static inline unsigned long dpm_get_env_rx_filter(struct dpm_param *dpmp)
{
	return dpmp->env.rx_filter;
}

static inline void dpm_set_env_rx_filter(struct dpm_param *dpmp,
		unsigned long rx_filter)
{
	dpmp->env.rx_filter = rx_filter;
}

/*static inline unsigned long dpm_get_env_ip_filter(struct dpm_param *dpmp)
{
	return dpmp->env.ip_filter;
}

static inline void dpm_set_env_ip_filter(struct dpm_param *dpmp,
		unsigned long ip_filter)
{
	dpmp->env.ip_filter = ip_filter;
}*/

static inline unsigned long dpm_get_env_mac_rx_filter(struct dpm_param *dpmp)
{
	return dpmp->env.mac_rx_filter;
}

static inline void dpm_set_env_mac_rx_filter(struct dpm_param *dpmp,
		unsigned long mac_rx_filter)
{
	dpmp->env.mac_rx_filter = mac_rx_filter;
}

/*inline unsigned long dpm_get_env_timp_backup_addr(struct dpm_param *dpmp)
{
	return dpmp->env.timp_backup_addr;
}

static inline void dpm_set_env_timp_backup_addr(struct dpm_param *dpmp, unsigned long timp_backup_addr)
{
	dpmp->env.timp_backup_addr = timp_backup_addr;
}*/

static inline unsigned long dpm_get_env_mc_ip(struct dpm_param *dpmp, int no)
{
	return dpmp->env.mc_ip[no];
}

static inline void dpm_set_env_mc_ip(struct dpm_param *dpmp, int no,
		unsigned long mc_ip)
{
	dpmp->env.mc_ip[no] = mc_ip;
}

static inline unsigned short dpm_get_env_udp_dport(struct dpm_param *dpmp, int no)
{
	return dpmp->env.udp_dport[no];
}

static inline void dpm_set_env_udp_dport(struct dpm_param *dpmp, int no,
		unsigned short udp_dport)
{
	dpmp->env.udp_dport[no] = udp_dport;
}

static inline unsigned short dpm_get_env_tcp_dport(struct dpm_param *dpmp, int no)
{
	return dpmp->env.tcp_dport[no];
}

static inline void dpm_set_env_tcp_dport(struct dpm_param *dpmp, int no,
		unsigned short tcp_dport)
{
	dpmp->env.tcp_dport[no] = tcp_dport;
}

/* TIMP feature */
static inline unsigned long dpm_get_env_timp_feature(struct dpm_param *dpmp)
{
	return dpmp->env.timp.feature;
}

static inline void dpm_set_env_timp_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.timp.feature = feat;
}

static inline void dpm_set_env_timp(struct dpm_param *dpmp,
		unsigned char weaksig_0, unsigned char weaksig_1,
		unsigned char update_0, unsigned char update_1)
{
	dpmp->env.timp.weaksig_0 = weaksig_0;
	dpmp->env.timp.weaksig_1 = weaksig_1;
	dpmp->env.timp.update_0 = update_0;
	dpmp->env.timp.update_1 = update_1;
}

static inline void dpm_set_env_timp_weaksig_0(struct dpm_param *dpmp)
{
	dpmp->env.timp.weaksig_0 = 1;
}

static inline void dpm_reset_env_timp_weaksig_0(struct dpm_param *dpmp)
{
	dpmp->env.timp.weaksig_0 = 0;
}

static inline unsigned char dpm_get_env_timp_weaksig_0(struct dpm_param *dpmp)
{
	return dpmp->env.timp.weaksig_0;
}

static inline void dpm_set_env_timp_weaksig_1(struct dpm_param *dpmp)
{
	dpmp->env.timp.weaksig_1 = 1;
}

static inline void dpm_reset_env_timp_weaksig_1(struct dpm_param *dpmp)
{
	dpmp->env.timp.weaksig_1 = 0;
}

static inline unsigned char dpm_get_env_timp_weaksig_1(struct dpm_param *dpmp)
{
	return dpmp->env.timp.weaksig_1;
}

static inline void dpm_set_env_timp_update_0(struct dpm_param *dpmp)
{
	dpmp->env.timp.update_0 = 1;
}

static inline void dpm_reset_env_timp_update_0(struct dpm_param *dpmp)
{
	dpmp->env.timp.update_0 = 0;
}

static inline unsigned char dpm_get_env_timp_update_0(struct dpm_param *dpmp)
{
	return dpmp->env.timp.update_0;
}

static inline void dpm_set_env_timp_update_1(struct dpm_param *dpmp)
{
	dpmp->env.timp.update_1 = 1;
}

static inline void dpm_reset_env_timp_update_1(struct dpm_param *dpmp)
{
	dpmp->env.timp.update_1 = 1;
}

static inline unsigned char dpm_get_env_timp_update_1(struct dpm_param *dpmp)
{
	return dpmp->env.timp.update_1;
}

static inline unsigned short dpm_get_env_timp_sz(struct dpm_param *dpmp)
{
	return dpmp->env.timp.sz;
}

static inline void dpm_set_env_timp_sz(struct dpm_param *dpmp,
						unsigned short sz)
{
	dpmp->env.timp.sz = sz;
}


/*
 * TIM feature
 */
static inline unsigned long dpm_get_env_tim_feature(struct dpm_param *dpmp)
{
	return dpmp->env.tim.feature;
}

static inline void dpm_set_env_tim_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.tim.feature = feat;
}

static inline void dpm_set_env_tim(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty,
		unsigned char prep, unsigned char wait_bcn,
		unsigned char detect_ssid, unsigned char detect_channel,
		unsigned char dont_care_loss, unsigned char pbr,
		unsigned char bcn_sync, unsigned char bcn_timeout_tu,
		unsigned char chk_cnt, unsigned long chk_step,
		unsigned char bcn_sync_clk)
{
	dpmp->env.tim.en = en;
	dpmp->env.tim.fix = fix;
	dpmp->env.tim.period_ty = period_ty;
	dpmp->env.tim.prep = prep;
	dpmp->env.tim.wait_bcn = wait_bcn;
	dpmp->env.tim.detect_ssid = detect_ssid;
	dpmp->env.tim.detect_channel = detect_channel;
	dpmp->env.tim.dont_care_loss = dont_care_loss;
	dpmp->env.tim.pbr = pbr;
	dpmp->env.tim.bcn_sync = bcn_sync;
	dpmp->env.tim.bcn_timeout_tu = bcn_timeout_tu;
	dpmp->env.tim.chk_cnt = chk_cnt;
	dpmp->env.tim.chk_step = chk_step;
	dpmp->env.tim.bcn_sync_clk = bcn_sync_clk;
}

static inline unsigned char dpm_get_env_tim_en(struct dpm_param *dpmp)
{
	return dpmp->env.tim.en;
}

static inline void dpm_reset_env_tim_en(struct dpm_param *dpmp)
{
	dpmp->env.tim.en = 0;
}

static inline void dpm_set_env_tim_en(struct dpm_param *dpmp)
{
	dpmp->env.tim.en = 1;
}

static inline unsigned char dpm_get_env_tim_fix(struct dpm_param *dpmp)
{
	return dpmp->env.tim.fix;
}

static inline void dpm_reset_env_tim_fix(struct dpm_param *dpmp)
{
	dpmp->env.tim.fix = 0;
}

static inline void dpm_set_env_tim_fix(struct dpm_param *dpmp)
{
	dpmp->env.tim.fix = 1;
}

static inline unsigned char dpm_get_env_tim_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.tim.period_ty;
}

static inline void dpm_reset_env_tim_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.tim.period_ty = 0;
}

static inline void dpm_set_env_tim_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.tim.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_tim_prep(struct dpm_param *dpmp)
{
	return dpmp->env.tim.prep;
}

static inline void dpm_set_env_tim_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.tim.prep = prep;
}

static inline unsigned char dpm_get_env_tim_wait_bcn(struct dpm_param *dpmp)
{
	return dpmp->env.tim.wait_bcn;
}

static inline void dpm_reset_env_tim_wait_bcn(struct dpm_param *dpmp)
{
	dpmp->env.tim.wait_bcn = 0;
}

static inline void dpm_set_env_tim_wait_bcn(struct dpm_param *dpmp)
{
	dpmp->env.tim.wait_bcn = 1;
}

static inline unsigned char dpm_get_env_tim_detect_ssid(struct dpm_param *dpmp)
{
	return dpmp->env.tim.detect_ssid;
}

static inline void dpm_reset_env_tim_detect_ssid(struct dpm_param *dpmp)
{
	dpmp->env.tim.detect_ssid = 0;
}

static inline void dpm_set_env_tim_detect_ssid(struct dpm_param *dpmp)
{
	dpmp->env.tim.detect_ssid = 1;
}

static inline unsigned char dpm_get_env_tim_detect_channel(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.detect_channel;
}

static inline void dpm_reset_env_tim_detect_channel(struct dpm_param *dpmp)
{
	dpmp->env.tim.detect_channel = 0;
}

static inline void dpm_set_env_tim_detect_channel(struct dpm_param *dpmp)
{
	dpmp->env.tim.detect_channel = 1;
}

static inline unsigned char dpm_get_env_tim_dont_care_loss(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.dont_care_loss;
}

static inline void dpm_reset_env_tim_dont_care_loss(struct dpm_param *dpmp)
{
	dpmp->env.tim.dont_care_loss = 0;
}

static inline void dpm_set_env_tim_dont_care_loss(struct dpm_param *dpmp)
{
	dpmp->env.tim.dont_care_loss = 1;
}

static inline unsigned char dpm_get_env_tim_pbr(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.pbr;
}

static inline void dpm_reset_env_tim_pbr(struct dpm_param *dpmp)
{
	dpmp->env.tim.pbr = 0;
}

static inline void dpm_set_env_tim_pbr(struct dpm_param *dpmp)
{
	dpmp->env.tim.pbr = 1;
}

static inline unsigned char dpm_get_env_tim_bcn_sync(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.bcn_sync;
}

static inline void dpm_reset_env_tim_bcn_sync(struct dpm_param *dpmp)
{
	dpmp->env.tim.bcn_sync = 0;
}

static inline void dpm_set_env_tim_bcn_sync(struct dpm_param *dpmp)
{
	dpmp->env.tim.bcn_sync = 1;
}

static inline unsigned char dpm_get_env_tim_bcn_sync_wait(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.bcn_sync_wait;
}

static inline void dpm_reset_env_tim_bcn_sync_wait(struct dpm_param *dpmp)
{
	dpmp->env.tim.bcn_sync_wait = 0;
}

static inline void dpm_set_env_tim_bcn_sync_wait(struct dpm_param *dpmp)
{
	dpmp->env.tim.bcn_sync_wait = 1;
}

static inline unsigned char dpm_get_env_tim_bcn_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.tim.bcn_timeout_tu;
}

static inline void dpm_set_env_tim_bcn_timeout_tu(struct dpm_param *dpmp,
		unsigned char bcn_timeout_tu)
{
	dpmp->env.tim.bcn_timeout_tu = bcn_timeout_tu;
}

static inline unsigned char dpm_get_env_tim_chk_cnt(struct dpm_param *dpmp)
{
	return dpmp->env.tim.chk_cnt;
}

static inline void dpm_set_env_tim_chk_cnt(struct dpm_param *dpmp,
		unsigned char chk_cnt)
{
	dpmp->env.tim.chk_cnt = chk_cnt;
}

static inline unsigned long dpm_get_env_tim_chk_step(struct dpm_param *dpmp)
{
	return dpmp->env.tim.chk_step;
}

static inline void dpm_set_env_tim_chk_step(struct dpm_param *dpmp,
		unsigned long chk_step)
{
	dpmp->env.tim.chk_step = chk_step;
}

static inline unsigned long dpm_get_env_tim_bcn_sync_clk(struct dpm_param *dpmp)
{
	return dpmp->env.tim.bcn_sync_clk;
}

static inline void dpm_set_env_tim_bcn_sync_clk(struct dpm_param *dpmp,
		unsigned long bcn_sync_clk)
{
	dpmp->env.tim.bcn_sync_clk = bcn_sync_clk;
}

/*
 * BC/MC feature
 */
static inline void dpm_set_env_bcmc(struct dpm_param *dpmp, unsigned char en,
		unsigned char morechk, unsigned char rx_timeout_control,
		unsigned char rx_timeout_tu, unsigned char timeout_tu,
		unsigned char max)
{
	dpmp->env.bcmc.en = en;
	dpmp->env.bcmc.morechk = morechk;
	dpmp->env.bcmc.rx_timeout_control = rx_timeout_control;
	dpmp->env.bcmc.rx_timeout_tu = rx_timeout_tu;
	dpmp->env.bcmc.timeout_tu = timeout_tu;
	dpmp->env.bcmc.max = max;
}

static inline unsigned char dpm_get_env_bcmc_en(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.en;
}

static inline void dpm_reset_env_bcmc_en(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.en = 0;
}

static inline void dpm_set_env_bcmc_en(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.en = 1;
}

static inline unsigned char dpm_get_env_bcmc_morechk(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.morechk;
}

static inline void dpm_reset_env_bcmc_morechk(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.morechk = 0;
}

static inline void dpm_set_env_bcmc_morechk(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.morechk = 1;
}

static inline unsigned char dpm_get_env_bcmc_rx_timeout_control(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.rx_timeout_control;
}

static inline void dpm_reset_env_bcmc_rx_timeout_control(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.rx_timeout_control = 0;
}

static inline void dpm_set_env_bcmc_rx_timeout_control(struct dpm_param *dpmp)
{
	dpmp->env.bcmc.rx_timeout_control = 1;
}

static inline unsigned char dpm_get_env_bcmc_rx_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.rx_timeout_tu;
}

static inline void dpm_set_env_bcmc_rx_timeout_tu(struct dpm_param *dpmp,
		unsigned char rx_timeout_tu)
{
	dpmp->env.bcmc.rx_timeout_tu = rx_timeout_tu;
}

static inline unsigned char dpm_get_env_bcmc_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.timeout_tu;
}

static inline void dpm_set_env_bcmc_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.bcmc.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_bcmc_max(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.max;
}

static inline void dpm_set_env_bcmc_max(struct dpm_param *dpmp,
		unsigned char max)
{
	dpmp->env.bcmc.max = max;
}

static inline unsigned long *dpm_get_env_bcmc_accept_oui(
		struct dpm_param *dpmp)
{
	return dpmp->env.bcmc.accept_oui;
}

static inline void dpm_set_env_bcmc_accept_oui(struct dpm_param *dpmp,
		unsigned long *accept_oui)
{
	int i;
	for(i = 0; i < DPM_ACCEPT_MC_CNT; i++)
		dpmp->env.bcmc.accept_oui[i] = accept_oui[i];
}

static inline unsigned long dpm_get_env_bcmc_accept_oui_n(
		struct dpm_param *dpmp, int n)
{
	return dpmp->env.bcmc.accept_oui[n];
}

static inline void dpm_set_env_bcmc_accept_oui_n(struct dpm_param *dpmp,
		int n, unsigned long accept_oui)
{
	dpmp->env.bcmc.accept_oui[n] = accept_oui;
}

/*
 * PS feature
 */
static inline unsigned long dpm_get_env_ps_feature(struct dpm_param *dpmp)
{
	return dpmp->env.ps.feature;
}

static inline void dpm_set_env_ps_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.ps.feature = feat;
}

static inline void dpm_set_env_ps(struct dpm_param *dpmp, unsigned char en,
		unsigned char period_ty, unsigned char ta, unsigned char prep,
		/*unsigned char typecnt,*/ unsigned char timeout_tu,
		unsigned char retry, unsigned char nouc_max,
		unsigned char ps_data_max)
{
	dpmp->env.ps.en = en;
	dpmp->env.ps.period_ty = period_ty;
	dpmp->env.ps.ta = ta;
	dpmp->env.ps.prep = prep;
	/*dpmp->env.ps.typecnt = typecnt;*/
	dpmp->env.ps.timeout_tu = timeout_tu;
	dpmp->env.ps.retry = retry;
	dpmp->env.ps.nouc_max = nouc_max;
	dpmp->env.ps.ps_data_max = ps_data_max;
}

static inline unsigned char dpm_get_env_ps_en(struct dpm_param *dpmp)
{
	return dpmp->env.ps.en;
}

static inline void dpm_reset_env_ps_en(struct dpm_param *dpmp)
{
	dpmp->env.ps.en = 0;
}

static inline void dpm_set_env_ps_en(struct dpm_param *dpmp)
{
	dpmp->env.ps.en = 1;
}

static inline unsigned char dpm_get_env_ps_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.ps.period_ty;
}

static inline void dpm_reset_env_ps_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.ps.period_ty = 0;
}

static inline void dpm_set_env_ps_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.ps.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_ps_prep(struct dpm_param *dpmp)
{
	return dpmp->env.ps.prep;
}

static inline void dpm_set_env_ps_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.ps.prep = prep;
}

static inline unsigned char dpm_get_env_ps_ta(struct dpm_param *dpmp)
{
	return dpmp->env.ps.ta;
}

static inline void dpm_reset_env_ps_ta(struct dpm_param *dpmp)
{
	dpmp->env.ps.ta = 0;
}

static inline void dpm_set_env_ps_ta(struct dpm_param *dpmp)
{
	dpmp->env.ps.ta = 1;
}

/*static inline unsigned char dpm_get_env_ps_typecnt(struct dpm_param *dpmp)
{
	return dpmp->env.ps.typecnt;
}

static inline void dpm_set_env_ps_typecnt(struct dpm_param *dpmp,
		unsigned char typecnt)
{
	dpmp->env.ps.typecnt = typecnt;
}*/

static inline unsigned char dpm_get_env_ps_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.ps.timeout_tu;
}

static inline void dpm_set_env_ps_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.ps.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_ps_retry(struct dpm_param *dpmp)
{
	return dpmp->env.ps.retry;
}

static inline void dpm_set_env_ps_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.ps.retry = retry;
}

static inline unsigned char dpm_get_env_ps_nouc_max(struct dpm_param *dpmp)
{
	return dpmp->env.ps.nouc_max;
}

static inline void dpm_set_env_ps_nouc_max(struct dpm_param *dpmp,
		unsigned char nouc_max)
{
	dpmp->env.ps.nouc_max = nouc_max;
}

static inline unsigned char dpm_get_env_ps_data_max(struct dpm_param *dpmp)
{
	return dpmp->env.ps.ps_data_max;
}

static inline void dpm_set_env_ps_data_max(struct dpm_param *dpmp,
		unsigned char ps_data_max)
{
	dpmp->env.ps.ps_data_max = ps_data_max;
}


/*
 * UC feature
 */
static inline void dpm_set_env_uc(struct dpm_param *dpmp, unsigned char en,
		unsigned char morechk, unsigned char rx_timeout_control,
		unsigned char rx_timeout_tu, unsigned char timeout_tu,
		unsigned char max)
{
	dpmp->env.uc.en = en;
	dpmp->env.uc.morechk = morechk;
	dpmp->env.uc.rx_timeout_control = rx_timeout_control;
	dpmp->env.uc.rx_timeout_tu = rx_timeout_tu;
	dpmp->env.uc.timeout_tu = timeout_tu;
	dpmp->env.uc.max = max;
}

static inline unsigned char dpm_get_env_uc_en(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.en;
}

static inline void dpm_reset_env_uc_en(struct dpm_param *dpmp)
{
	dpmp->env.uc.en = 0;
}

static inline void dpm_set_env_uc_en(struct dpm_param *dpmp)
{
	dpmp->env.uc.en = 1;
}

static inline unsigned char dpm_get_env_uc_morechk(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.morechk;
}

static inline void dpm_reset_env_uc_morechk(struct dpm_param *dpmp)
{
	dpmp->env.uc.morechk = 0;
}

static inline void dpm_set_env_uc_morechk(struct dpm_param *dpmp)
{
	dpmp->env.uc.morechk = 1;
}

static inline unsigned char dpm_get_env_uc_rx_timeout_control(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.rx_timeout_control;
}

static inline void dpm_reset_env_uc_rx_timeout_control(struct dpm_param *dpmp)
{
	dpmp->env.uc.rx_timeout_control = 0;
}

static inline void dpm_set_env_uc_rx_timeout_control(struct dpm_param *dpmp)
{
	dpmp->env.uc.rx_timeout_control = 1;
}

static inline unsigned char dpm_get_env_uc_rx_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.rx_timeout_tu;
}

static inline void dpm_set_env_uc_rx_timeout_tu(struct dpm_param *dpmp,
		unsigned char rx_timeout_tu)
{
	dpmp->env.uc.rx_timeout_tu = rx_timeout_tu;
}

static inline unsigned char dpm_get_env_uc_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.timeout_tu;
}

static inline void dpm_set_env_uc_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.uc.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_uc_max(
		struct dpm_param *dpmp)
{
	return dpmp->env.uc.max;
}

static inline void dpm_set_env_uc_max(struct dpm_param *dpmp,
		unsigned char max)
{
	dpmp->env.uc.max = max;
}

/*
 * KA feature
 */
static inline unsigned long dpm_get_env_ka_feature(struct dpm_param *dpmp)
{
	return dpmp->env.ka.feature;
}

static inline void dpm_set_env_ka_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.ka.feature = feat;
}

static inline void dpm_set_env_ka(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char td, unsigned char timeout_tu,
		unsigned char retry, unsigned long period)
{
	dpmp->env.ka.en = en;
	dpmp->env.ka.fix = fix;
	dpmp->env.ka.period_ty = period_ty;
	dpmp->env.ka.prep = prep;
	dpmp->env.ka.ta = ta;
	dpmp->env.ka.td = td;
	dpmp->env.ka.timeout_tu = timeout_tu;
	dpmp->env.ka.retry = retry;
	dpmp->env.ka.period = period;
}

static inline unsigned char dpm_get_env_ka_en(struct dpm_param *dpmp)
{
	return dpmp->env.ka.en;
}

static inline void dpm_reset_env_ka_en(struct dpm_param *dpmp)
{
	dpmp->env.ka.en = 0;
}

static inline void dpm_set_env_ka_en(struct dpm_param *dpmp)
{
	dpmp->env.ka.en = 1;
}

static inline unsigned char dpm_get_env_ka_fix(struct dpm_param *dpmp)
{
	return dpmp->env.ka.fix;
}

static inline void dpm_reset_env_ka_fix(struct dpm_param *dpmp)
{
	dpmp->env.ka.fix = 0;
}

static inline void dpm_set_env_ka_fix(struct dpm_param *dpmp)
{
	dpmp->env.ka.fix = 1;
}

static inline unsigned char dpm_get_env_ka_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.ka.period_ty;
}

static inline void dpm_reset_env_ka_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.ka.period_ty = 0;
}

static inline void dpm_set_env_ka_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.ka.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_ka_prep(struct dpm_param *dpmp)
{
	return dpmp->env.ka.prep;
}

static inline void dpm_set_env_ka_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.ka.prep = prep;
}

static inline unsigned char dpm_get_env_ka_ta(struct dpm_param *dpmp)
{
	return dpmp->env.ka.ta;
}

static inline void dpm_reset_env_ka_ta(struct dpm_param *dpmp)
{
	dpmp->env.ka.ta = 0;
}

static inline void dpm_set_env_ka_ta(struct dpm_param *dpmp)
{
	dpmp->env.ka.ta = 1;
}

static inline unsigned char dpm_get_env_ka_td(struct dpm_param *dpmp)
{
	return dpmp->env.ka.td;
}

static inline void dpm_reset_env_ka_td(struct dpm_param *dpmp)
{
	dpmp->env.ka.td = 0;
}

static inline void dpm_set_env_ka_td(struct dpm_param *dpmp)
{
	dpmp->env.ka.td = 1;
}

static inline unsigned char dpm_get_env_ka_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.ka.timeout_tu;
}

static inline void dpm_set_env_ka_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.ka.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_ka_retry(struct dpm_param *dpmp)
{
	return dpmp->env.ka.retry;
}

static inline void dpm_set_env_ka_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.ka.retry = retry;
}

static inline unsigned long dpm_get_env_ka_period(struct dpm_param *dpmp)
{
	return dpmp->env.ka.period;
}

static inline void dpm_set_env_ka_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.ka.period = period;
}

/*
 * Auto arp feature
 */
static inline unsigned long dpm_get_env_arp_feature(struct dpm_param *dpmp)
{
	return dpmp->env.arp.feature;
}

static inline void dpm_set_env_arp_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.arp.feature = feat;
}

static inline void dpm_set_env_arp(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char timeout_tu, unsigned char retry,
		unsigned long period, unsigned char type)
{
	dpmp->env.arp.en = en;
	dpmp->env.arp.fix = fix;
	dpmp->env.arp.period_ty = period_ty;
	dpmp->env.arp.prep = prep;
	dpmp->env.arp.ta = ta;
	dpmp->env.arp.timeout_tu = timeout_tu;
	dpmp->env.arp.retry = retry;
	dpmp->env.arp.period = period;
	dpmp->env.arp.type = type;
}

static inline unsigned char dpm_get_env_arp_en(struct dpm_param *dpmp)
{
	return dpmp->env.arp.en;
}

static inline void dpm_reset_env_arp_en(struct dpm_param *dpmp)
{
	dpmp->env.arp.en = 0;
}

static inline void dpm_set_env_arp_en(struct dpm_param *dpmp)
{
	dpmp->env.arp.en = 1;
}

static inline unsigned char dpm_get_env_arp_fix(struct dpm_param *dpmp)
{
	return dpmp->env.arp.fix;
}

static inline void dpm_reset_env_arp_fix(struct dpm_param *dpmp)
{
	dpmp->env.arp.fix = 0;
}

static inline void dpm_set_env_arp_fix(struct dpm_param *dpmp)
{
	dpmp->env.arp.fix = 1;
}

static inline unsigned char dpm_get_env_arp_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.arp.period_ty;
}

static inline void dpm_reset_env_arp_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.arp.period_ty = 0;
}

static inline void dpm_set_env_arp_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.arp.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_arp_prep(struct dpm_param *dpmp)
{
	return dpmp->env.arp.prep;
}

static inline void dpm_set_env_arp_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.arp.prep = prep;
}

static inline unsigned char dpm_get_env_arp_ta(struct dpm_param *dpmp)
{
	return dpmp->env.arp.ta;
}

static inline void dpm_reset_env_arp_ta(struct dpm_param *dpmp)
{
	dpmp->env.arp.ta = 0;
}

static inline void dpm_set_env_arp_ta(struct dpm_param *dpmp)
{
	dpmp->env.arp.ta = 1;
}

static inline unsigned char dpm_get_env_arp_retry(struct dpm_param *dpmp)
{
	return dpmp->env.arp.retry;
}

static inline void dpm_set_env_arp_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.arp.retry = retry;
}

static inline unsigned long dpm_get_env_arp_period(struct dpm_param *dpmp)
{
	return dpmp->env.arp.period;
}

static inline void dpm_set_env_arp_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.arp.period = period;
}

static inline unsigned char dpm_get_env_arp_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.arp.timeout_tu;
}

static inline void dpm_set_env_arp_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.arp.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_arp_type(struct dpm_param *dpmp)
{
	return dpmp->env.arp.type;
}

static inline void dpm_set_env_arp_type(struct dpm_param *dpmp,
		unsigned char ty)
{
	dpmp->env.arp.type = ty;
}

static inline unsigned long dpm_get_env_arp_target_ip(struct dpm_param *dpmp)
{
	return dpmp->env.arp.target_ip;
}

static inline void dpm_set_env_arp_target_ip(struct dpm_param *dpmp, unsigned long target_ip)
{
	dpmp->env.arp.target_ip = target_ip;
}

static inline unsigned short *dpm_get_env_arp_target_mac(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.arp.target_mac[0];
}

static inline void dpm_set_env_arp_target_mac(struct dpm_param *dpmp,
		unsigned char *target_mac)
{
	unsigned char *arp_target_mac =
		(unsigned char *) &dpmp->env.arp.target_mac[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		arp_target_mac[i] = target_mac[i];
}


/*
 * ARP response feature
 */
static inline unsigned long dpm_get_env_arpresp_feature(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.feature;
}

static inline void dpm_set_env_arpresp_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.arpresp.feature = feat;
}

static inline void dpm_set_env_arpresp(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char timeout_tu, unsigned char retry,
		unsigned long period)
{
	dpmp->env.arpresp.en = en;
	dpmp->env.arpresp.fix = fix;
	dpmp->env.arpresp.period_ty = period_ty;
	dpmp->env.arpresp.prep = prep;
	dpmp->env.arpresp.ta = ta;
	dpmp->env.arpresp.timeout_tu = timeout_tu;
	dpmp->env.arpresp.retry = retry;
	dpmp->env.arpresp.period = period;
}

static inline unsigned char dpm_get_env_arpresp_en(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.en;
}

static inline void dpm_reset_env_arpresp_en(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.en = 0;
}

static inline void dpm_set_env_arpresp_en(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.en = 1;
}

static inline unsigned char dpm_get_env_arpresp_fix(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.fix;
}

static inline void dpm_reset_env_arpresp_fix(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.fix = 0;
}

static inline void dpm_set_env_arpresp_fix(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.fix = 1;
}

static inline unsigned char dpm_get_env_arpresp_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.period_ty;
}

static inline void dpm_reset_env_arpresp_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.period_ty = 0;
}

static inline void dpm_set_env_arpresp_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.arpresp.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_arpresp_prep(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.prep;
}

static inline void dpm_set_env_arpresp_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.arpresp.prep = prep;
}

static inline unsigned char dpm_get_env_arpresp_ta(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.ta;
}

static inline void dpm_reset_env_arpresp_ta(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.ta = 0;
}

static inline void dpm_set_env_arpresp_ta(struct dpm_param *dpmp)
{
	dpmp->env.arpresp.ta = 1;
}

static inline unsigned char dpm_get_env_arpresp_retry(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.retry;
}

static inline void dpm_set_env_arpresp_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.arpresp.retry = retry;
}

static inline unsigned long dpm_get_env_arpresp_period(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.period;
}

static inline void dpm_set_env_arpresp_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.arpresp.period = period;
}

static inline unsigned char dpm_get_env_arpresp_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.timeout_tu;
}

static inline void dpm_set_env_arpresp_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.arpresp.timeout_tu = timeout_tu;
}

static inline unsigned short *dpm_get_env_arpresp_da(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.arpresp.da[0];
}

static inline void dpm_set_env_arpresp_da(struct dpm_param *dpmp,
		unsigned char *da)
{
	unsigned char *arpresp_da =
		(unsigned char *) &dpmp->env.arpresp.da[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		arpresp_da[i] = da[i];
}

static inline unsigned long dpm_get_env_arpresp_sender_ip(struct dpm_param *dpmp)
{
	return dpmp->env.arpresp.sender_ip;
}

static inline void dpm_set_env_arpresp_sender_ip(struct dpm_param *dpmp,
		unsigned char *sender_ip)
{
	unsigned char *arpresp_sender_ip =
		(unsigned char *) &dpmp->env.arpresp.sender_ip;

	arpresp_sender_ip[0] = sender_ip[0];
	arpresp_sender_ip[1] = sender_ip[1];
	arpresp_sender_ip[2] = sender_ip[2];
	arpresp_sender_ip[3] = sender_ip[3];
}

static inline unsigned short *dpm_get_env_arpresp_sender_mac(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.arpresp.sender_mac[0];
}

static inline void dpm_set_env_arpresp_sender_mac(struct dpm_param *dpmp,
		unsigned char *sender_mac)
{
	unsigned char *arpresp_sender_mac =
		(unsigned char *) &dpmp->env.arpresp.sender_mac[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		arpresp_sender_mac[i] = sender_mac[i];
}


/*
 * UDP hole punch feature
 */
static inline unsigned long dpm_get_env_udph_feature(struct dpm_param *dpmp)
{
	return dpmp->env.udph.feature;
}

static inline void dpm_set_env_udph_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.udph.feature = feat;
}

static inline void dpm_set_env_udph(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char timeout_tu, unsigned char retry,
		unsigned long period, unsigned char type,
		unsigned int payload_len)
{
	dpmp->env.udph.en = en;
	dpmp->env.udph.fix = fix;
	dpmp->env.udph.period_ty = period_ty;
	dpmp->env.udph.prep = prep;
	dpmp->env.udph.ta = ta;
	dpmp->env.udph.timeout_tu = timeout_tu;
	dpmp->env.udph.retry = retry;
	dpmp->env.udph.period = period;
	dpmp->env.udph.type = type;
	dpmp->env.udph.payload_len = payload_len;
}

static inline unsigned char dpm_get_env_udph_en(struct dpm_param *dpmp)
{
	return dpmp->env.udph.en;
}

static inline void dpm_reset_env_udph_en(struct dpm_param *dpmp)
{
	dpmp->env.udph.en = 0;
}

static inline void dpm_set_env_udph_en(struct dpm_param *dpmp)
{
	dpmp->env.udph.en = 1;
}

static inline unsigned char dpm_get_env_udph_fix(struct dpm_param *dpmp)
{
	return dpmp->env.udph.fix;
}

static inline void dpm_reset_env_udph_fix(struct dpm_param *dpmp)
{
	dpmp->env.udph.fix = 0;
}

static inline void dpm_set_env_udph_fix(struct dpm_param *dpmp)
{
	dpmp->env.udph.fix = 1;
}

static inline unsigned char dpm_get_env_udph_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.udph.period_ty;
}

static inline void dpm_reset_env_udph_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.udph.period_ty = 0;
}

static inline void dpm_set_env_udph_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.udph.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_udph_prep(struct dpm_param *dpmp)
{
	return dpmp->env.udph.prep;
}

static inline void dpm_set_env_udph_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.udph.prep = prep;
}

static inline unsigned char dpm_get_env_udph_ta(struct dpm_param *dpmp)
{
	return dpmp->env.udph.ta;
}

static inline void dpm_reset_env_udph_ta(struct dpm_param *dpmp)
{
	dpmp->env.udph.ta = 0;
}

static inline void dpm_set_env_udph_ta(struct dpm_param *dpmp)
{
	dpmp->env.udph.ta = 1;
}

static inline unsigned char dpm_get_env_udph_retry(struct dpm_param *dpmp)
{
	return dpmp->env.udph.retry;
}

static inline void dpm_set_env_udph_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.udph.retry = retry;
}

static inline unsigned long dpm_get_env_udph_period(struct dpm_param *dpmp)
{
	return dpmp->env.udph.period;
}

static inline void dpm_set_env_udph_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.udph.period = period;
}

static inline unsigned char dpm_get_env_udph_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.udph.timeout_tu;
}

static inline void dpm_set_env_udph_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.udph.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_udph_type(struct dpm_param *dpmp)
{
	return dpmp->env.udph.type;
}

static inline void dpm_set_env_udph_type(struct dpm_param *dpmp,
		unsigned char type)
{
	dpmp->env.udph.type = type;
}

static inline unsigned int dpm_get_env_udph_payload_len(struct dpm_param *dpmp)
{
	return dpmp->env.udph.payload_len;
}

static inline void dpm_set_env_udph_payload_len(struct dpm_param *dpmp,
		unsigned int payload_len)
{
	dpmp->env.udph.payload_len = payload_len;
}

static inline unsigned long dpm_get_env_udph_dip(struct dpm_param *dpmp)
{
	return dpmp->env.udph.dip;
}

static inline void dpm_set_env_udph_dip(struct dpm_param *dpmp,
		unsigned long dip)
{
	dpmp->env.udph.dip = dip;
}

static inline unsigned short dpm_get_env_udph_sport(struct dpm_param *dpmp)
{
	return dpmp->env.udph.sport;
}

static inline void dpm_set_env_udph_sport(struct dpm_param *dpmp,
		unsigned short sport)
{
	dpmp->env.udph.sport = sport;
}

static inline unsigned short dpm_get_env_udph_dport(struct dpm_param *dpmp)
{
	return dpmp->env.udph.dport;
}

static inline void dpm_set_env_udph_dport(struct dpm_param *dpmp,
		unsigned short dport)
{
	dpmp->env.udph.dport = dport;
}

static inline unsigned short *dpm_get_env_udph_target_mac(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.udph.target_mac[0];
}

static inline void dpm_set_env_udph_target_mac(struct dpm_param *dpmp,
		unsigned char *target_mac)
{
	unsigned char *udph_target_mac =
		(unsigned char *) &dpmp->env.udph.target_mac[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		udph_target_mac[i] = target_mac[i];
}

static inline unsigned long dpm_get_env_udph_user_data(struct dpm_param *dpmp)
{
	return dpmp->env.udph.user_data;
}

static inline void dpm_set_env_udph_user_data(struct dpm_param *dpmp,
		unsigned long user_data)
{
	dpmp->env.udph.user_data = user_data;
}


/*
 * SetPS feature
 */
static inline unsigned long dpm_get_env_setps_feature(struct dpm_param *dpmp)
{
	return dpmp->env.setps.feature;
}

static inline void dpm_set_env_setps_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.setps.feature = feat;
}

static inline void dpm_set_env_setps(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char timeout_tu, unsigned char retry,
		unsigned long period)
{
	dpmp->env.setps.en = en;
	dpmp->env.setps.fix = fix;
	dpmp->env.setps.period_ty = period_ty;
	dpmp->env.setps.prep = prep;
	dpmp->env.setps.ta = ta;
	dpmp->env.setps.timeout_tu = timeout_tu;
	dpmp->env.setps.retry = retry;
	dpmp->env.setps.period = period;
}

static inline unsigned char dpm_get_env_setps_en(struct dpm_param *dpmp)
{
	return dpmp->env.setps.en;
}

static inline void dpm_reset_env_setps_en(struct dpm_param *dpmp)
{
	dpmp->env.setps.en = 0;
}

static inline void dpm_set_env_setps_en(struct dpm_param *dpmp)
{
	dpmp->env.setps.en = 1;
}

static inline unsigned char dpm_get_env_setps_fix(struct dpm_param *dpmp)
{
	return dpmp->env.setps.fix;
}

static inline void dpm_reset_env_setps_fix(struct dpm_param *dpmp)
{
	dpmp->env.setps.fix = 0;
}

static inline void dpm_set_env_setps_fix(struct dpm_param *dpmp)
{
	dpmp->env.setps.fix = 1;
}

static inline unsigned char dpm_get_env_setps_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.setps.period_ty;
}

static inline void dpm_reset_env_setps_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.setps.period_ty = 0;
}

static inline void dpm_set_env_setps_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.setps.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_setps_prep(struct dpm_param *dpmp)
{
	return dpmp->env.setps.prep;
}

static inline void dpm_set_env_setps_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.setps.prep = prep;
}

static inline unsigned char dpm_get_env_setps_ta(struct dpm_param *dpmp)
{
	return dpmp->env.setps.ta;
}

static inline void dpm_reset_env_setps_ta(struct dpm_param *dpmp)
{
	dpmp->env.setps.ta = 0;
}

static inline void dpm_set_env_setps_ta(struct dpm_param *dpmp)
{
	dpmp->env.setps.ta = 1;
}

static inline unsigned char dpm_get_env_setps_timeout_tu(struct dpm_param *dpmp)
{
	return dpmp->env.setps.timeout_tu;
}

static inline void dpm_set_env_setps_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.setps.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_setps_retry(struct dpm_param *dpmp)
{
	return dpmp->env.setps.retry;
}

static inline void dpm_set_env_setps_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.setps.retry = retry;
}

static inline unsigned long dpm_get_env_setps_period(struct dpm_param *dpmp)
{
	return dpmp->env.setps.period;
}

static inline void dpm_set_env_setps_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.setps.period = period;
}


/*
 * ARP request feature
 */
static inline unsigned long dpm_get_env_arpreq_feature(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.feature;
}

static inline void dpm_set_env_arpreq_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.arpreq.feature = feat;
}

static inline void dpm_set_env_arpreq(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char retry, unsigned char type,
		unsigned char timeout_tu, unsigned long period)
{
	dpmp->env.arpreq.en = en;
	dpmp->env.arpreq.fix = fix;
	dpmp->env.arpreq.period_ty = period_ty;
	dpmp->env.arpreq.prep = prep;
	dpmp->env.arpreq.ta = ta;
	dpmp->env.arpreq.retry = retry;
	dpmp->env.arpreq.type = type;
	dpmp->env.arpreq.timeout_tu = timeout_tu;
	dpmp->env.arpreq.period = period;
}

static inline unsigned char dpm_get_env_arpreq_en(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.en;
}

static inline void dpm_reset_env_arpreq_en(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.en = 0;
}

static inline void dpm_set_env_arpreq_en(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.en = 1;
}

static inline unsigned char dpm_get_env_arpreq_fix(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.fix;
}

static inline void dpm_reset_env_arpreq_fix(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.fix = 0;
}

static inline void dpm_set_env_arpreq_fix(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.fix = 1;
}

static inline unsigned char dpm_get_env_arpreq_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.period_ty;
}

static inline void dpm_reset_env_arpreq_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.period_ty = 0;
}

static inline void dpm_set_env_arpreq_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.arpreq.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_arpreq_prep(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.prep;
}

static inline void dpm_set_env_arpreq_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.arpreq.prep = prep;
}

static inline unsigned char dpm_get_env_arpreq_ta(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.ta;
}

static inline void dpm_reset_env_arpreq_ta(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.ta = 0;
}

static inline void dpm_set_env_arpreq_ta(struct dpm_param *dpmp)
{
	dpmp->env.arpreq.ta = 1;
}

static inline unsigned char dpm_get_env_arpreq_retry(
		struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.retry;
}

static inline void dpm_set_env_arpreq_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.arpreq.retry = retry;
}

static inline unsigned char dpm_get_env_arpreq_type(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.type;
}

static inline void dpm_set_env_arpreq_type(struct dpm_param *dpmp,
		unsigned char ty)
{
	dpmp->env.arpreq.type = ty;
}

static inline unsigned char dpm_get_env_arpreq_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.timeout_tu;
}

static inline void dpm_set_env_arpreq_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.arpreq.timeout_tu = timeout_tu;
}

static inline unsigned long dpm_get_env_arpreq_period(
		struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.period;
}

static inline void dpm_set_env_arpreq_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.arpreq.period = period;
}

static inline unsigned long dpm_get_env_arpreq_target_ip(struct dpm_param *dpmp)
{
	return dpmp->env.arpreq.target_ip;
}

static inline void dpm_set_env_arpreq_target_ip(struct dpm_param *dpmp, unsigned long target_ip)
{
	dpmp->env.arpreq.target_ip = target_ip;
}

static inline unsigned short *dpm_get_env_arpreq_target_mac(struct dpm_param *dpmp)
{
	return (unsigned short *) &dpmp->env.arpreq.target_mac[0];
}

static inline void dpm_set_env_arpreq_target_mac(struct dpm_param *dpmp,
		unsigned char *target_mac)
{
	unsigned char *arpreq_target_mac =
		(unsigned char *) &dpmp->env.arpreq.target_mac[0];
	int i;

	for (i = 0; i < DPM_MAC_LEN; i++)
		arpreq_target_mac[i] = target_mac[i];
}

/*
 * Deauth chk feature
 */
static inline unsigned long dpm_get_env_deauth_feature(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.feature;
}

static inline void dpm_set_env_deauth_feature(struct dpm_param *dpmp,
		unsigned long feat)
{
	dpmp->env.deauth.feature = feat;
}

static inline void dpm_set_env_deauth(struct dpm_param *dpmp, unsigned char en,
		unsigned char fix, unsigned char period_ty, unsigned char prep,
		unsigned char ta, unsigned char retry, unsigned char type,
		unsigned char timeout_tu, unsigned char chk_timeout_tu,
		unsigned long period)
{
	dpmp->env.deauth.en = en;
	dpmp->env.deauth.fix = fix;
	dpmp->env.deauth.period_ty = period_ty;
	dpmp->env.deauth.prep = prep;
	dpmp->env.deauth.ta = ta;
	dpmp->env.deauth.retry = retry;
	dpmp->env.deauth.type = type;
	dpmp->env.deauth.timeout_tu = timeout_tu;
	dpmp->env.deauth.chk_timeout_tu = chk_timeout_tu;
	dpmp->env.deauth.period = period;
}

static inline unsigned char dpm_get_env_deauth_en(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.en;
}

static inline void dpm_reset_env_deauth_en(struct dpm_param *dpmp)
{
	dpmp->env.deauth.en = 0;
}

static inline void dpm_set_env_deauth_en(struct dpm_param *dpmp)
{
	dpmp->env.deauth.en = 1;
}

static inline unsigned char dpm_get_env_deauth_fix(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.fix;
}

static inline void dpm_reset_env_deauth_fix(struct dpm_param *dpmp)
{
	dpmp->env.deauth.fix = 0;
}

static inline void dpm_set_env_deauth_fix(struct dpm_param *dpmp)
{
	dpmp->env.deauth.fix = 1;
}

static inline unsigned char dpm_get_env_deauth_period_ty(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.period_ty;
}

static inline void dpm_reset_env_deauth_period_ty(struct dpm_param *dpmp)
{
	dpmp->env.deauth.period_ty = 0;
}

static inline void dpm_set_env_deauth_period_ty(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	dpmp->env.deauth.period_ty = period_ty;
}

static inline unsigned char dpm_get_env_deauth_prep(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.prep;
}

static inline void dpm_set_env_deauth_prep(struct dpm_param *dpmp,
		unsigned char prep)
{
	dpmp->env.deauth.prep = prep;
}

static inline unsigned char dpm_get_env_deauth_ta(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.ta;
}

static inline void dpm_reset_env_deauth_ta(struct dpm_param *dpmp)
{
	dpmp->env.deauth.ta = 0;
}

static inline void dpm_set_env_deauth_ta(struct dpm_param *dpmp)
{
	dpmp->env.deauth.ta = 1;
}

static inline unsigned char dpm_get_env_deauth_retry(
		struct dpm_param *dpmp)
{
	return dpmp->env.deauth.retry;
}

static inline void dpm_set_env_deauth_retry(struct dpm_param *dpmp,
		unsigned char retry)
{
	dpmp->env.deauth.retry = retry;
}

static inline unsigned char dpm_get_env_deauth_type(struct dpm_param *dpmp)
{
	return dpmp->env.deauth.type;
}

static inline void dpm_set_env_deauth_type(struct dpm_param *dpmp,
		unsigned char ty)
{
	dpmp->env.deauth.type = ty;
}

static inline unsigned char dpm_get_env_deauth_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.deauth.timeout_tu;
}

static inline void dpm_set_env_deauth_timeout_tu(struct dpm_param *dpmp,
		unsigned char timeout_tu)
{
	dpmp->env.deauth.timeout_tu = timeout_tu;
}

static inline unsigned char dpm_get_env_deauth_chk_timeout_tu(
		struct dpm_param *dpmp)
{
	return dpmp->env.deauth.chk_timeout_tu;
}

static inline void dpm_set_env_deauth_chk_timeout_tu(struct dpm_param *dpmp,
		unsigned char chk_timeout_tu)
{
	dpmp->env.deauth.chk_timeout_tu = chk_timeout_tu;
}

static inline unsigned long dpm_get_env_deauth_period(
		struct dpm_param *dpmp)
{
	return dpmp->env.deauth.period;
}

static inline void dpm_set_env_deauth_period(struct dpm_param *dpmp,
		unsigned long period)
{
	dpmp->env.deauth.period = period;
}

/*******************************************************************************
 *
 * RTM MM Interface
 *
 ******************************************************************************/
static inline struct start_req *dpm_get_mm_start_req(struct dpm_param *dpmp)
{
	return &dpmp->mm.start;
}

static inline struct add_if_req *dpm_get_mm_add_if_req(struct dpm_param *dpmp)
{
	return &dpmp->mm.add_if;
}

static inline struct set_bssid_req *dpm_get_mm_set_bssid_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_bssid;
}

static inline struct set_beacon_int_req *dpm_get_mm_set_beacon_int_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_beacon_int;
}

static inline struct set_basic_rates_req *dpm_get_mm_set_basic_rates_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_basic_rates;
}

static inline struct set_edca_req *dpm_get_mm_set_edca_req(
		struct dpm_param *dpmp, int n)
{
	return &dpmp->mm.edca[n];
}

static inline struct set_vif_state_req *dpm_get_mm_set_vif_state_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_vif_state;
}

static inline struct set_power_req *dpm_get_mm_set_power_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_power;
}

static inline struct set_slottime_req *dpm_get_mm_set_slottime_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_slottime;
}

static inline struct ba_add_req *dpm_get_mm_ba_add_req(struct dpm_param *dpmp)
{
	return &dpmp->mm.ba_add;
}

static inline struct set_ps_options_req *dpm_get_mm_set_ps_options_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_ps_options;
}

static inline struct set_filter_req *dpm_get_mm_set_filter_req(
		struct dpm_param *dpmp)
{
	return &dpmp->mm.set_filter;
}

/*******************************************************************************
 *
 * RTM ST Interface
 *
 ******************************************************************************/
static inline unsigned long dpm_get_st_status(struct dpm_param *dpmp)
{
	return dpmp->st.status;
}

static inline void dpm_set_st_status(struct dpm_param *dpmp,
		unsigned long status)
{
	dpmp->st.status |= status;
}

static inline void dpm_set_st_err_status(struct dpm_param *dpmp,
		unsigned long e_status)
{
	dpmp->st.status = (dpmp->st.status & 0x00ffffff) | e_status;
}

static inline unsigned long dpm_get_st_err_status(struct dpm_param *dpmp)
{
	return dpmp->st.status & 0xff000000;
}

static inline void dpm_reset_st_status(struct dpm_param *dpmp)
{
	dpmp->st.status = 0;
}

static inline unsigned char dpm_get_st_pm_status(struct dpm_param *dpmp)
{
	return dpmp->st.pm_status;
}

static inline void dpm_reset_st_pm_status(struct dpm_param *dpmp)
{
	dpmp->st.pm_status = 0;
}

static inline void dpm_set_st_pm_status(struct dpm_param *dpmp)
{
	dpmp->st.pm_status = 1;
}

static inline unsigned char dpm_get_st_bcn_sync_status(struct dpm_param *dpmp)
{
	return dpmp->st.bcn_sync_status;
}

static inline void dpm_reset_st_bcn_sync_status(struct dpm_param *dpmp)
{
	dpmp->st.bcn_sync_status = 0;
}

static inline void dpm_set_st_bcn_sync_status(struct dpm_param *dpmp)
{
	dpmp->st.bcn_sync_status = 1;
}

static inline signed char dpm_get_st_rssi(struct dpm_param *dpmp)
{
	return dpmp->st.rssi;
}

static inline void dpm_set_st_rssi(struct dpm_param *dpmp,
		signed char rssi)
{
	dpmp->st.rssi = rssi;
}

static inline signed char dpm_get_st_rssi_status(struct dpm_param *dpmp)
{
	return dpmp->st.rssi_status;
}

static inline void dpm_set_st_rssi_status(struct dpm_param *dpmp,
		signed char rssi_status)
{
	dpmp->st.rssi_status = rssi_status;
}

static inline unsigned char dpm_get_st_ps_type(struct dpm_param *dpmp)
{
	return dpmp->st.ps_type;
}

static inline void dpm_set_st_ps_type(struct dpm_param *dpmp,
		unsigned char type)
{
	dpmp->st.ps_type = type;
}

static inline unsigned char dpm_get_st_determined_ps_type(
		struct dpm_param *dpmp)
{
	return dpmp->st.determined_ps_type;
}

static inline void dpm_reset_st_determined_ps_type(struct dpm_param *dpmp)
{
	dpmp->st.determined_ps_type = 0;
}

static inline void dpm_set_st_determined_ps_type(struct dpm_param *dpmp)
{
	dpmp->st.determined_ps_type = 1;
}

/*static inline signed char dpm_get_st_ps_poll_typecnt(struct dpm_param *dpmp)
{
	return dpmp->st.ps_poll_typecnt;
}

static inline void dpm_reset_st_ps_poll_typecnt(struct dpm_param *dpmp)
{
	dpmp->st.ps_poll_typecnt = 0;
}

static inline void dpm_set_st_ps_poll_typecnt(struct dpm_param *dpmp)
{
	dpmp->st.ps_poll_typecnt++;
}*/

static inline unsigned long dpm_get_st_wdog(struct dpm_param *dpmp)
{
	return dpmp->st.wdog_preamble;
}

static inline void dpm_set_st_wdog(struct dpm_param *dpmp)
{
	dpmp->st.wdog_preamble = DPM_WDOG_PREAMBLE;
}

static inline void dpm_reset_st_wdog(struct dpm_param *dpmp)
{
	dpmp->st.wdog_preamble = 0;
}

static inline void dpm_set_st_trigger_ue(struct dpm_param *dpmp,
		unsigned long trg)
{
	dpmp->st.trigger_ue = trg;
}

static inline void dpm_set_st_trigger_fboot(struct dpm_param *dpmp,
		unsigned long trg)
{
	dpmp->st.trigger_fboot = trg;
}

static inline void dpm_set_st_trigger_ra(struct dpm_param *dpmp,
		unsigned long trg)
{
	dpmp->st.trigger_ra = trg;
}

static inline void dpm_set_st_trigger_ptim(struct dpm_param *dpmp,
		unsigned long trg)
{
	dpmp->st.trigger_ptim = trg;
}

static inline unsigned long dpm_get_st_app_cnt(struct dpm_param *dpmp)
{
	return dpmp->st.tim_app_cnt;
}

static inline void dpm_reset_st_app_cnt(struct dpm_param *dpmp)
{
	dpmp->st.tim_app_cnt = 0;
}

static inline void dpm_set_st_app_cnt(struct dpm_param *dpmp)
{
	dpmp->st.tim_app_cnt++;
}

static inline unsigned short dpm_get_st_max_bcn_size(struct dpm_param *dpmp)
{
	return dpmp->st.max_bcn_size;
}

static inline void dpm_set_st_max_bcn_size(struct dpm_param *dpmp,
		unsigned short bcn_size)
{
	dpmp->st.max_bcn_size = bcn_size;
}

static inline unsigned short dpm_get_st_max_bcn_duration(struct dpm_param *dpmp)
{
	return dpmp->st.max_bcn_duration;
}

static inline void dpm_set_st_max_bcn_duration(struct dpm_param *dpmp,
		unsigned short max_bcn_duration)
{
	dpmp->st.max_bcn_duration = max_bcn_duration;
}

static inline unsigned char dpm_get_st_bcn_chk_cnt(struct dpm_param *dpmp)
{
	return dpmp->st.bcn_chk_cnt;
}

static inline void dpm_reset_st_bcn_chk_cnt(struct dpm_param *dpmp)
{
	dpmp->st.bcn_chk_cnt = 0;
}

static inline unsigned short dpm_get_st_tim_off(struct dpm_param *dpmp)
{
	return dpmp->st.tim_off;
}

static inline void dpm_set_st_tim_off(struct dpm_param *dpmp,
		unsigned short tim_off)
{
	dpmp->st.tim_off = tim_off;
}

static inline unsigned char dpm_get_st_nouc_cnt(struct dpm_param *dpmp)
{
	return dpmp->st.nouc_cnt;
}

static inline void dpm_reset_st_nouc_cnt(struct dpm_param *dpmp)
{
	dpmp->st.nouc_cnt = 0;
}

static inline void dpm_set_st_nouc_cnt(struct dpm_param *dpmp)
{
	dpmp->st.nouc_cnt++;
}

static inline unsigned char dpm_get_st_ps_data_cnt(struct dpm_param *dpmp,
		int ty)
{
	return dpmp->st.ps_data_cnt[ty];
}

static inline void dpm_reset_st_ps_data_cnt(struct dpm_param *dpmp, int ty)
{
	dpmp->st.ps_data_cnt[ty] = 0;
}

static inline void dpm_set_st_ps_data_cnt(struct dpm_param *dpmp, int ty)
{
	dpmp->st.ps_data_cnt[ty]++;
}

static inline void dpm_set_st_bcn_chk_cnt(struct dpm_param *dpmp)
{
	dpmp->st.bcn_chk_cnt++;
}

static inline unsigned long dpm_get_st_bcn_chk_step(struct dpm_param *dpmp)
{
	return dpmp->st.bcn_chk_step;
}

static inline void dpm_reset_st_bcn_chk_step(struct dpm_param *dpmp)
{
	dpmp->st.bcn_chk_step = 0;
}

static inline void dpm_set_st_bcn_chk_step(struct dpm_param *dpmp)
{
	dpmp->st.bcn_chk_step++;
}

/*inline unsigned char dpm_get_st_bcn_ready_clk(struct dpm_param *dpmp)
{
	return dpmp->st.bcn_ready_clk;
}

static inline void dpm_set_st_bcn_ready_clk(struct dpm_param *dpmp,
		unsigned char bcn_ready_clk)
{
	dpmp->st.bcn_ready_clk = bcn_ready_clk;
}*/

static inline unsigned long dpm_get_st_ele_crc(struct dpm_param *dpmp)
{
	return dpmp->st.ele_crc;
}

static inline void dpm_set_st_ele_crc(struct dpm_param *dpmp,
		unsigned long ele_crc)
{
	dpmp->st.ele_crc = ele_crc;
}

static inline unsigned char dpm_get_st_current_ch(struct dpm_param *dpmp)
{
	return dpmp->st.current_ch;
}

static inline void dpm_set_st_current_ch(struct dpm_param *dpmp,
		unsigned char current_ch)
{
	dpmp->st.current_ch = current_ch;
}

static inline unsigned char dpm_get_st_ssid_check_sum(struct dpm_param *dpmp)
{
	return dpmp->st.ssid_check_sum;
}

static inline void dpm_set_st_ssid_check_sum(struct dpm_param *dpmp,
		unsigned char ssid_check_sum)
{
	dpmp->st.ssid_check_sum = ssid_check_sum;
}

static inline unsigned char dpm_get_st_prev_dtim_cnt(struct dpm_param *dpmp)
{
	return dpmp->st.prev_dtim_cnt;
}

static inline void dpm_set_st_prev_dtim_cnt(struct dpm_param *dpmp,
		unsigned char prev_dtim_cnt)
{
	dpmp->st.prev_dtim_cnt = prev_dtim_cnt;
}

/*inline long long dpm_get_st_sleep_time(struct dpm_param *dpmp)
{
	return dpmp->st.sleep_time;
}

static inline void dpm_set_st_sleep_time(struct dpm_param *dpmp,
		long long sleep_time)
{
	dpmp->st.sleep_time = sleep_time;
}*/

static inline long long dpm_get_st_expected_tbtt_clk(struct dpm_param *dpmp)
{
	return dpmp->st.expected_tbtt_clk;
}

static inline void dpm_set_st_expected_tbtt_clk(struct dpm_param *dpmp,
		long long expected_tbtt_clk)
{
	dpmp->st.expected_tbtt_clk = expected_tbtt_clk;
}

static inline long long dpm_get_st_last_bcn_clk(struct dpm_param *dpmp)
{
	return dpmp->st.last_bcn_clk;
}

static inline void dpm_set_st_last_bcn_clk(struct dpm_param *dpmp,
		long long last_bcn_clk)
{
	dpmp->st.last_bcn_clk = last_bcn_clk;
}

static inline long long dpm_get_st_pd_clk(struct dpm_param *dpmp)
{
	return dpmp->st.pd_clk;
}

static inline void dpm_set_st_pd_clk(struct dpm_param *dpmp, long long pd_clk)
{
	dpmp->st.pd_clk = pd_clk;
}

static inline long long dpm_get_st_ptim_pd_clk(struct dpm_param *dpmp)
{
	return dpmp->st.ptim_pd_clk;
}

static inline void dpm_set_st_ptim_pd_clk(struct dpm_param *dpmp,
		long long ptim_pd_clk)
{
	dpmp->st.ptim_pd_clk = ptim_pd_clk;
}

static inline unsigned short dpm_get_st_tx_seqn(struct dpm_param *dpmp)
{
	return dpmp->st.tx_seqn;
}

static inline void dpm_reset_st_tx_seqn(struct dpm_param *dpmp)
{
	dpmp->st.tx_seqn = 0;
}

static inline void dpm_set_st_tx_seqn(struct dpm_param *dpmp,
		unsigned short tx_seqn)
{
	dpmp->st.tx_seqn = tx_seqn;
}

static inline void dpm_inc_st_tx_seqn(struct dpm_param *dpmp)
{
	dpmp->st.tx_seqn = (dpmp->st.tx_seqn + 1) & 0xfff;
}

static inline unsigned short dpm_get_st_tx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac)
{
	return dpmp->st.tx_qos_seqn[ac];
}

static inline void dpm_reset_st_tx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac)
{
	dpmp->st.tx_qos_seqn[ac] = 0;
}

static inline void dpm_set_st_tx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac, unsigned short tx_qos_seqn)
{
	dpmp->st.tx_qos_seqn[ac] = tx_qos_seqn;
}

static inline void dpm_inc_st_tx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac)
{
	dpmp->st.tx_qos_seqn[ac] = (dpmp->st.tx_qos_seqn[ac] + 1) & 0xfff;
}

static inline unsigned short dpm_get_st_rx_seqn(struct dpm_param *dpmp)
{
	return dpmp->st.rx_seqn;
}

static inline void dpm_reset_st_rx_seqn(struct dpm_param *dpmp)
{
	dpmp->st.rx_seqn = 0;
}

static inline void dpm_set_st_rx_seqn(struct dpm_param *dpmp,
		unsigned short rx_seqn)
{
	dpmp->st.rx_seqn = rx_seqn;
}

static inline unsigned short dpm_get_st_rx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac)
{
	return dpmp->st.rx_qos_seqn[ac];
}

static inline void dpm_reset_st_rx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac)
{
	dpmp->st.rx_qos_seqn[ac] = 0;
}

static inline void dpm_set_st_rx_qos_seqn(struct dpm_param *dpmp,
		unsigned char ac,
		unsigned short rx_qos_seqn)
{
	dpmp->st.rx_qos_seqn[ac] = rx_qos_seqn;
}


/*******************************************************************************
 *
 * RTM Interface
 *
 ******************************************************************************/
static inline unsigned long dpm_get_preamble(struct dpm_param *dpmp)
{
	return dpmp->preamble;
}

static inline void dpm_set_preamble(struct dpm_param *dpmp)
{
	dpmp->preamble = DPM_PREAMBLE;
}

/*******************************************************************************
 *
 * RTM INF Interface
 *
 ******************************************************************************/
/*inline unsigned long dpm_get_inf_preamble(struct dpm_param *dpmp)
{
	return dpmp->inf.inf_preamble;
}

static inline void dpm_set_inf_preamble(struct dpm_param *dpmp)
{
	dpmp->inf.inf_preamble = DPM_INF_INITED_PREAMBLE;
}

static inline void dpm_clear_inf_preamble(struct dpm_param *dpmp)
{
	dpmp->inf.inf_preamble = 0;
}*/

/*******************************************************************************
 *
 * RTM TXIV Interface
 *
 ******************************************************************************/
static inline unsigned char *dpm_get_txiv_iv(struct dpm_param *dpmp)
{
	return &dpmp->txiv.iv[0];
}

static inline void dpm_set_txiv_iv(struct dpm_param *dpmp, unsigned char *iv,
		int iv_len)
{
	int i;
	unsigned char *p = &dpmp->txiv.iv[0];

	for (i = 0; i < iv_len; i++)
		p[i] = iv[i];
}

static inline unsigned short dpm_get_txiv_key_idx(struct dpm_param *dpmp)
{
	return dpmp->txiv.key_idx;
}

static inline void dpm_set_txiv_key_idx(struct dpm_param *dpmp,
		unsigned short key_idx)
{
	dpmp->txiv.key_idx = key_idx;
}

static inline unsigned char dpm_get_txiv_iv_len(struct dpm_param *dpmp)
{
	return dpmp->txiv.iv_len;
}

static inline void dpm_set_txiv_iv_len(struct dpm_param *dpmp,
		unsigned char iv_len)
{
	dpmp->txiv.iv_len = iv_len;
}

static inline unsigned char dpm_get_txiv_wep_key_len(struct dpm_param *dpmp)
{
	return dpmp->txiv.wep_key_len;
}

static inline void dpm_set_txiv_wep_key_len(struct dpm_param *dpmp,
		unsigned char wep_key_len)
{
	dpmp->txiv.wep_key_len = wep_key_len;
}

static inline unsigned char *dpm_get_txiv_mic_key(struct dpm_param *dpmp)
{
	return &dpmp->txiv.mic_key[0];
}

static inline void dpm_set_txiv_mic_key(struct dpm_param *dpmp,
		unsigned char *mic_key)
{
	int i;
	unsigned char *p = &dpmp->txiv.mic_key[0];

	for (i = 0; i < 8; i++)
		p[i] = mic_key[i];
}

/*******************************************************************************
 *
 * DPM APTRK feature
 *
 ******************************************************************************/
static inline unsigned char dpm_get_aptrk_en(struct dpm_param *dpmp)
{
	return dpmp->aptrk.en;
}

static inline void dpm_reset_aptrk_en(struct dpm_param *dpmp)
{
	dpmp->aptrk.en = 0;
}

static inline void dpm_set_aptrk_en(struct dpm_param *dpmp, unsigned char en)
{
	dpmp->aptrk.en = en;
}

static inline unsigned char dpm_get_aptrk_lock(struct dpm_param *dpmp)
{
	return dpmp->aptrk.lock;
}

static inline void dpm_reset_aptrk_lock(struct dpm_param *dpmp)
{
	dpmp->aptrk.lock = 0;
}

static inline void dpm_set_aptrk_lock(struct dpm_param *dpmp,
		unsigned char lock)
{
	dpmp->aptrk.lock = lock;
}

static inline short dpm_get_aptrk_tsf_normalization(struct dpm_param *dpmp)
{
	return dpmp->aptrk.tsf_normalization;
}

static inline void dpm_set_aptrk_tsf_normalization(struct dpm_param *dpmp,
		short tsf_normalization)
{
	dpmp->aptrk.tsf_normalization = tsf_normalization;
}

static inline long dpm_get_aptrk_ready_time(struct dpm_param *dpmp)
{
	return dpmp->aptrk.ready_time;
}

static inline void dpm_set_aptrk_ready_time(struct dpm_param *dpmp,
		long ready_time)
{
	dpmp->aptrk.ready_time = ready_time;
}

static inline long dpm_get_aptrk_guard_time(struct dpm_param *dpmp)
{
	return dpmp->aptrk.guard_time;
}

static inline void dpm_set_aptrk_guard_time(struct dpm_param *dpmp,
		long guard_time)
{
	dpmp->aptrk.guard_time = guard_time;
}


/*******************************************************************************
 *
 * DPM Sche feature
 *
 ******************************************************************************/
static inline unsigned long long dpm_get_sche_counter(struct dpm_param *dpmp)
{
	return dpmp->sche.counter;
}

static inline void dpm_set_sche_counter(struct dpm_param *dpmp,
		unsigned long long counter)
{
	dpmp->sche.counter = counter;
}

static inline unsigned char dpm_get_sche_prep_clk(struct dpm_param *dpmp, int n)
{
	return dpmp->sche.prep_clk[n];
}

static inline void dpm_set_sche_prep_clk(struct dpm_param *dpmp,
		int n, unsigned char prep_clk)
{
	dpmp->sche.prep_clk[n] = prep_clk;
}

static inline unsigned char dpm_get_sche_post_prep(struct dpm_param *dpmp)
{
	return dpmp->sche.post_prep;
}

static inline void dpm_set_sche_post_prep(struct dpm_param *dpmp,
		unsigned char post_prep)
{
	dpmp->sche.post_prep = post_prep;
}

static inline unsigned long long dpm_get_sche_function_bit(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].function_bit;
}

static inline void dpm_set_sche_function_bit(
		struct dpm_param *dpmp, int n, unsigned long long function_bit)
{
	dpmp->sche.node[n].function_bit = function_bit;
}

static inline unsigned long dpm_get_sche_interval(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].interval;
}

static inline void dpm_set_sche_interval(
		struct dpm_param *dpmp, int n, unsigned long interval)
{
	dpmp->sche.node[n].interval = interval;
}

static inline unsigned int dpm_get_sche_arbitrary(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].arbitrary;
}

static inline void dpm_set_sche_arbitrary(
		struct dpm_param *dpmp, int n, unsigned int arbitrary)
{
	dpmp->sche.node[n].arbitrary = arbitrary;
}

static inline unsigned int dpm_get_sche_align(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].align;
}

static inline void dpm_set_sche_align(
		struct dpm_param *dpmp, int n, unsigned int align)
{
	dpmp->sche.node[n].align = align;
}

static inline unsigned int dpm_get_sche_half(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].half;
}

static inline void dpm_set_sche_half(
		struct dpm_param *dpmp, int n, unsigned int half)
{
	dpmp->sche.node[n].half = half;
}

static inline unsigned int dpm_get_sche_preparation(
		struct dpm_param *dpmp, int n)
{
	return dpmp->sche.node[n].preparation;
}

static inline void dpm_set_sche_preparation(
		struct dpm_param *dpmp, int n, unsigned int preparation)
{
	dpmp->sche.node[n].preparation = preparation;
}

/*static inline unsigned long long dpm_sche_get_func(struct dpm_param *dpmp)
{
	return dpmp->sche.node[0].function_bit;
}*/

/*******************************************************************************
 *
 * RTM PHY Interface
 *
 ******************************************************************************/

static inline unsigned long dpm_get_phy_reserved0(struct dpm_param *dpmp)
{
	return dpmp->phy.reserved0;
}

static inline void dpm_set_phy_reserved0(struct dpm_param *dpmp,
		unsigned long reserved0)
{
	dpmp->phy.reserved0 = reserved0;
}

static inline unsigned long dpm_get_phy_rf_product_info(struct dpm_param *dpmp)
{
	return dpmp->phy.rf_product_info;
}

static inline void dpm_set_phy_rf_product_info(struct dpm_param *dpmp,
		unsigned long rf_product_info)
{
	dpmp->phy.rf_product_info = rf_product_info;
}

static inline unsigned long dpm_get_phy_rf_op_skip(struct dpm_param *dpmp)
{
	return dpmp->phy.rf_op_skip;
}

static inline void dpm_set_phy_rf_op_skip(struct dpm_param *dpmp,
		unsigned long rf_op_skip)
{
	dpmp->phy.rf_op_skip = rf_op_skip;
}

static inline unsigned long dpm_get_phy_csf_rx_cn(struct dpm_param *dpmp)
{
	return dpmp->phy.csf_rx_cn;
}

static inline void dpm_set_phy_csf_rx_cn(struct dpm_param *dpmp,
		unsigned long csf_rx_cn)
{
	dpmp->phy.csf_rx_cn = csf_rx_cn;
}

static inline unsigned long dpm_get_phy_otp_xtal40_offset(struct dpm_param *dpmp)
{
	return dpmp->phy.otp_xtal40_offset;
}

static inline void dpm_set_phy_otp_xtal40_offset(struct dpm_param *dpmp,
		unsigned long otp_xtal40_offset)
{
	dpmp->phy.otp_xtal40_offset = otp_xtal40_offset;
}

static inline unsigned long dpm_get_phy_dpdgain_out3(struct dpm_param *dpmp)
{
	return dpmp->phy.dpdgain.out3;
}

static inline void dpm_set_phy_dpdgain_out3(struct dpm_param *dpmp,
		unsigned long out3)
{
	dpmp->phy.dpdgain.out3 = out3;
}

static inline unsigned long dpm_get_phy_dpdgain_lut_scaler1(
		struct dpm_param *dpmp)
{
	return dpmp->phy.dpdgain.lut_scaler1;
}

static inline void dpm_set_phy_dpdgain_lut_scaler1(struct dpm_param *dpmp,
		unsigned long lut_scaler1)
{
	dpmp->phy.dpdgain.lut_scaler1 = lut_scaler1;
}

static inline unsigned long dpm_get_phy_curr_temp(struct dpm_param *dpmp)
{
	return dpmp->phy.curr_temp;
}

static inline void dpm_set_phy_curr_temp(struct dpm_param *dpmp,
		unsigned long curr_temp)
{
	dpmp->phy.curr_temp = curr_temp;
}

static inline unsigned short dpm_get_phy_rx_pga1(struct dpm_param *dpmp)
{
	return dpmp->phy.rx_pga1;
}

static inline void dpm_set_phy_rx_pga1(struct dpm_param *dpmp,
		unsigned short rx_pga1)
{
	dpmp->phy.rx_pga1 = rx_pga1;
}

static inline unsigned short dpm_get_phy_rx_pga2(struct dpm_param *dpmp)
{
	return dpmp->phy.rx_pga2;
}

static inline void dpm_set_phy_rx_pga2(struct dpm_param *dpmp,
		unsigned short rx_pga2)
{
	dpmp->phy.rx_pga2 = rx_pga2;
}

static inline unsigned short dpm_get_phy_rx_buff(struct dpm_param *dpmp)
{
	return dpmp->phy.rx_buff;
}

static inline void dpm_set_phy_rx_buff(struct dpm_param *dpmp,
		unsigned short rx_buff)
{
	dpmp->phy.rx_buff = rx_buff;
}

static inline unsigned short dpm_get_phy_tx_mix(struct dpm_param *dpmp)
{
	return dpmp->phy.tx_mix;
}

static inline void dpm_set_phy_tx_mix(struct dpm_param *dpmp,
		unsigned short tx_mix)
{
	dpmp->phy.tx_mix = tx_mix;
}

static inline unsigned short dpm_get_phy_tx_pga1(struct dpm_param *dpmp)
{
	return dpmp->phy.tx_pga1;
}

static inline void dpm_set_phy_tx_pga1(struct dpm_param *dpmp,
		unsigned short tx_pga1)
{
	dpmp->phy.tx_pga1 = tx_pga1;
}

static inline unsigned short dpm_get_phy_tx_pga2(struct dpm_param *dpmp)
{
	return dpmp->phy.tx_pga2;
}

static inline void dpm_set_phy_tx_pga2(struct dpm_param *dpmp,
		unsigned short tx_pga2)
{
	dpmp->phy.tx_pga2 = tx_pga2;
}

static inline unsigned long dpm_get_phy_tx_iqc(struct dpm_param *dpmp)
{
	return dpmp->phy.tx_iqc;
}

static inline void dpm_set_phy_tx_iqc(struct dpm_param *dpmp,
		unsigned long tx_iqc)
{
	dpmp->phy.tx_iqc = tx_iqc;
}

/*static inline unsigned char dpm_get_phy_otp_cal_da_high(struct dpm_param *dpmp)
{
	return dpmp->phy.otp_cal.da_high;
}

static inline void dpm_set_phy_otp_cal_da_high(struct dpm_param *dpmp,
		unsigned char otp_cal_da_high)
{
	dpmp->phy.otp_cal.da_high = otp_cal.da_high;
}

static inline unsigned char dpm_get_phy_otp_cal_da_low(struct dpm_param *dpmp)
{
	return dpmp->phy.otp_cal.da_low;
}

static inline void dpm_set_phy_otp_cal_da_low(struct dpm_param *dpmp,
		unsigned char otp_cal_da_low)
{
	dpmp->phy.otp_cal.da_low = otp_cal.da_low;
}*/

static inline unsigned long dpm_get_phy_otp_cal_reg1(struct dpm_param *dpmp)
{
	return dpmp->phy.otp_cal.reg1;
}

static inline void dpm_set_phy_otp_cal_reg1(struct dpm_param *dpmp,
		unsigned long reg1)
{
	dpmp->phy.otp_cal.reg1 = reg1;
}

static inline unsigned long dpm_get_phy_otp_cal_reg2(struct dpm_param *dpmp)
{
	return dpmp->phy.otp_cal.reg2;
}

static inline void dpm_set_phy_otp_cal_reg2(struct dpm_param *dpmp,
		unsigned long reg2)
{
	dpmp->phy.otp_cal.reg2 = reg2;
}

static inline unsigned long dpm_get_phy_calrtc_1(struct dpm_param *dpmp)
{
	return dpmp->phy.calrtc_1;
}

static inline void dpm_set_phy_calrtc_1(struct dpm_param *dpmp,
		unsigned long calrtc_1)
{
	dpmp->phy.calrtc_1 = calrtc_1;
}

static inline unsigned long dpm_get_phy_calrtc_2(struct dpm_param *dpmp)
{
	return dpmp->phy.calrtc_2;
}

static inline void dpm_set_phy_calrtc_2(struct dpm_param *dpmp,
		unsigned long calrtc_2)
{
	dpmp->phy.calrtc_2 = calrtc_2;
}

extern struct dpm_param *dpmp_base;
extern void dpmrtm_set_dpmp(unsigned long dpmp_base);

/// @} end of group ROMAC_COM_RTM

#endif /* __DPMRTM_H__ */
