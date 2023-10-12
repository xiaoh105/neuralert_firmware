/**
 ****************************************************************************************
 *
 * @file     timp_patch.h
 *
 * @brief    The header for TIM performance
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

#ifndef __TIMP_PATCH_H__
#define __TIMP_PATCH_H__

/**
 * @file     timp_patch.h
 * @brief    The header for TIM performance
 * @details  This header file is used in RTOS & PTIM.
 *           TIMP is used by RTOS and TIM to measure the performance of DPM.
 * @defgroup ROMAC_TIMP
 * @ingroup  ROMAC_COM
 * @{
 */
#include "target.h"
#include "dpmrtm_patch.h"

#define TIMP_EN
#define TIMP_FULL_RTM

#define TIMP_AUTO_EN
#undef TIMP_DISPLAY_EN

#define TIMP_PERIOD_INF    0xffffffff /* Manual */

#define TIMP_0_PERIOD      TIMP_PERIOD_INF /* 1 hours = 1000ms * 60s * 60m * 1h = 3600000 */
#define TIMP_1_PERIOD      TIMP_PERIOD_INF /* 24 hours = 1000ms * 60s * 60m * 24h = 86400000*/
#define TIMP_UPDATE_BITS   4
#define TIMP_WEAK_BITS     4
#define TIMP_CHK_UPDATED(i, s)  ((1 << i) & s)
#define TIMP_CHK_WEAK_SIG(i, s) ((1 << (i + TIMP_UPDATE_BITS)) & s)

#ifdef TIMP_EN

//#define SET_TIMP(arg0, arg1) ((timp_get_performance()->##arg0) = (##arg1))
//#define GET_TIMP(arg0)       (timp_get_performance()->##arg0)
#define SET_TIMP(arg0, arg1)            ((((struct timp_performance_tag *) timp_buf)->arg0) = (arg1))
#define GET_TIMP(arg0)                  (((struct timp_performance_tag *) timp_buf)->arg0)
#define INC_TIMP(arg0)                  (SET_TIMP(arg0, GET_TIMP(arg0) + 1))
#define GET_TIMPS(arg0)                 (&(((struct timp_performance_tag *) timp_buf)->arg0)[0])
#define SET_TIMPS(arg0, arg1, len)      (ROMAC_MEMCPY(&(((struct timp_performance_tag *) timp_buf)->arg0)[0], (unsigned char *) &(arg1)[0], len))

enum TIMP_INTERFACE {
	TIMP_INTERFACE_0    = 0,
	TIMP_INTERFACE_1    = 1,
	TIMP_INTERFACE_MAX
};

enum TIMP_UPDATED_STATUS {
	TIMP_UPDATED_NONE = 0,
	TIMP_UPDATED_0    = 1,
	TIMP_UPDATED_1    = 2,
	TIMP_UPDATED_0_1  = 3,

	TIMP_WEAK_SIG_0   = 16,
	TIMP_WEAK_SIG_1   = 32,
	TIMP_WEAK_SIG_0_1 = 48
};

struct timp_performance_tag {
	struct timp_interface_tag i;

	/* TIM Information */
	unsigned long long sche_f;
	unsigned long long sche_pending;
	unsigned long      prev_status;
	unsigned long      timp_status;

	unsigned int   tcp_ka_en: 1;
	unsigned int   tcp_ka_done: 1;
	unsigned int   arp_resp_en: 1;
	unsigned int   arp_resp_done: 1;
	unsigned char  bcn_ready_clk; /* Beacon ready time */

	/* Info */
	int            prev_pm_status: 1;
	int            bcn_sync_status: 1;
    /// The flag indicates whether an ARP Reply is received from the AP.
	int            arpresp_rx_status: 1;
	int            bcn_wait_en: 1;
	int            bcn_sync_time_en: 1;
	int            activated_pm0: 1;
	/// The flag indicates whether BUF state was changed.
	int            bufp_state_change: 1;
	/// The flag indicates whether TxRetry failed.
	int            txretry_fail: 1;
	int            prev_ssid_checksum: 8;
	int            reserved: 13;
	/// In get_null_frame we need to identify whether the null frame is KA or
	// Null(pm=0) transmitted instead of PS-POLL.
	int            setps_or_ka: 1;
	int            tcpka_ack: 1;
	int            tcpka_timeout_en: 1;

	/* Warning flags */
	union {
		unsigned int       warning;
		struct {
			unsigned int       unexpected_dtim_cnt_warning: 1;
			unsigned int       same_timer_warning: 1;
			unsigned int       preptime_ovfl_warning: 1;
			unsigned int       posttime_ovfl_warning: 1;

			unsigned int       uc_morechk_warning: 1;
			unsigned int       bcmc_morechk_warning: 1;
			unsigned int       uc_max_warning: 1;
			unsigned int       bcmc_max_warning: 1;

			unsigned int       pbr_timestamp_warning: 1;
			unsigned int       deauthchk_noack_warning: 1;
			unsigned int       warning_reserved: 2;

			unsigned int       br_warning: 5;
		};
	};
	unsigned char      unexpected_dtim_cnt;

	/* Error flags */
	union {
		unsigned int       error;
		struct {
			unsigned int       pbd_null_error: 1;
			unsigned int       no_sche_error: 1;
			unsigned int       out_of_sche_buf_error: 1;
			unsigned int       empty_sche_error: 1;

			unsigned int       ptim_timeout_error: 1;
			unsigned int       deauth_error: 1;
			unsigned int       no_ap_error: 1;
			unsigned int       no_ack_error: 1;
		};
	};
	unsigned long      prevent_sleep; ///< The last prevent_sleep by ptim_timeout_error

	unsigned short     deauth_reason_code: 16;
	unsigned short     deauth_error_reserved: 16;

	/* TIMP status - PTIM */
	/* ... */
	/* TIMP status - DM */
	unsigned short     connection_loss_cnt: 16;
	unsigned short     reserved_reserved: 16;

	/* RX Status */
	unsigned int       rx_more_bit: 1;
	unsigned int       rx_reserved: 31;

	unsigned char      rx_sa[DPM_MAC_LEN];
	unsigned char      rx_cnt;
	unsigned char      rx_checked_cnt;
	unsigned char      rx_hd_err_cnt;
	unsigned char      rx_bcn_cnt;
	unsigned char      rx_mgmt_cnt;
	unsigned char      rx_data_cnt;
	unsigned char      rx_cntrl_cnt;
	unsigned char      qos_cnt; ///< QoS data count
	unsigned char      ht_cnt; ///< HT control field data count
	unsigned char      tods0_fromds0_cnt; ///< To DS = 0, from DS = 0
	unsigned char      tods1_fromds0_cnt; ///< To DS = 1, from DS = 0
	unsigned char      tods0_fromds1_cnt; ///< To DS = 0, from DS = 1
	unsigned char      tods1_fromds1_cnt; ///< To DS = 1, from DS = 1
	unsigned char      decr_err_cnt;
	unsigned char      invalid_bssid_cnt;
	unsigned char      invalid_ra_cnt;
	unsigned char      invalid_data_cnt;
	unsigned char      my_data_cnt; ///< SA data with my MAC
	unsigned char      bc_cnt; ///< The received BC data count
	unsigned char      mc_cnt; ///< The received MC data count
	unsigned char      matched_mc_cnt;
	unsigned char      uc_cnt; ///< The received UC data count
	unsigned char      null_cnt;
	unsigned char      bcmc_upload_cnt;
	unsigned char      uc_upload_cnt;
	unsigned char      dataty; ///< BC/MC: 1, UC: 2, INVALID DATA: 0
	unsigned long      rx_pending;

	/* BCN */
	unsigned long      bcn_hd;
	unsigned long      tim;
	unsigned long      bcn_timeout; ///< BCN Timeout
	long long          timestamp;
	unsigned long      tsflo_start;   ///< bcn_start is estimated from tsflo_end.
	unsigned long      tsflo_end;   ///< BCN RX done or Timeout
	unsigned short     bcn_sz; ///< + 4bytes CRC
	unsigned short     bcn_duration;

	unsigned long      duration_of_frame;
	unsigned long      duration_to_timestamp;
	unsigned long      bcn_cca;

	unsigned int       bcn_change: 1;
	unsigned int       tim_bcmc_en: 1;
	unsigned int       tim_uc_en: 1;
	unsigned int       rssi_status: 1;
	signed char        rssi;

	/* TIM */
	unsigned int       tim_en: 1;
	unsigned int       bcn_done: 1;         ///< bcn received successfully or bcn timeout
	unsigned int       bcn_rx_done: 1;      ///< bcn received successfully both BCN and PBR.
	unsigned int       bcn_rx_completed: 1; ///< completed bcn received successfully and not PBR
	unsigned int       bcn_timeout_en: 1;
	unsigned int       bcn_loss_confirm_en: 1;
	unsigned int       dtim_cnt: 8; ///< dtim_cnt field of TIM
	unsigned int       tim_done: 1;
	unsigned int       tim_reserved: 17;
	/// When dtim_cnt != 0, if necessary, fixed_dtim_cnt to dtim_period.
	unsigned char      fixed_dtim_cnt;

	/* BC/MC */
	unsigned int       bcmc_en: 1;
	unsigned int       bcmc_done: 1;
	unsigned int       bcmc_rx_timeout_en: 1;
	unsigned int       bcmc_reserved: 21;
	unsigned int       bcmc_max: 8;
	unsigned long      bcmc_rx_timeout;
	unsigned long      bcmc_timeout;

	/* DEAUTH CHK */
	unsigned int       deauth_en: 1;
	unsigned int       deauth_done: 1;
	unsigned int       deauth_timeout_en: 1;
	unsigned int       deauth_frame_ack: 1;
	unsigned int       deauth_reserved: 28;
	unsigned long      deauth_timeout;
	unsigned long      deauth_period;

	/* PS */
	unsigned int       ps_en: 1;
	unsigned int       ps_done: 1;
	unsigned int       ps_timeout_en: 1;
	//unsigned int       ps_retry_limit_en: 1;
	unsigned int       ps_ack: 1;
	unsigned int       ps_type: 2;
	unsigned int       ps_fixed_type: 1;
	unsigned int       ps_typecnt: 4;
	unsigned int       ps_more: 1;
	unsigned int       ps_reserved: 12;
	unsigned int       ps_retry_cnt: 8;
	unsigned long      ps_timeout;
	unsigned long      ps_stat;
	unsigned long      ps_mediumtimeused;

	/* UC */
	unsigned int       uc_en: 1;          ///< UC RX
	unsigned int       uc_done: 1;
	unsigned int       uc_rx_timeout_en: 1;
	unsigned int       uc_reserved: 21;
	unsigned int       uc_max: 8;
	unsigned long      uc_rx_timeout;
	unsigned long      uc_timeout;

	/* Preparation time */
	unsigned short     preptime;      ///< us
	unsigned short     post_preptime; ///< us
	unsigned short     preptime_ovfl;
	unsigned short     posttime_ovfl;
	unsigned short     next_preptime;

	/* KA */
	unsigned int       ka_en: 1;
	unsigned int       ka_done: 1;
	unsigned int       ka_timeout_en: 1;
	unsigned int       ka_ack: 1;
	unsigned int       ka_reserved: 28;
	unsigned long      ka_start;
	unsigned long      ka_stop;
	unsigned long      ka_timeout;
	unsigned long      ka_period;
	unsigned long      ka_stat;
	unsigned long      ka_mediumtimeused;
	unsigned char      ka_retry_cnt;

	/* Auto ARP */
	unsigned int       auto_arp_en: 1;
	unsigned int       auto_arp_done: 1;
	unsigned int       auto_arp_timeout_en: 1;
	unsigned int       auto_arp_ack: 1;
	unsigned int       auto_arp_reserved: 20;
	unsigned int       auto_arp_type: 8;
	unsigned long      auto_arp_start;
	unsigned long      auto_arp_stop;
	unsigned long      auto_arp_timeout;
	unsigned long      auto_arp_period;
	unsigned long      auto_arp_stat;
	unsigned long      auto_arp_mediumtimeused;
	unsigned char      auto_arp_retry_cnt;

	/* ARP Response */
	unsigned int       arpresp_en: 1;
	unsigned int       arpresp_done: 1;
	unsigned int       arpresp_timeout_en: 1;
	unsigned int       arpresp_ack: 1;
	unsigned int       arpresp_reserved: 20;
	unsigned int       arpresp_type: 8;
	unsigned long      arpresp_start;
	unsigned long      arpresp_stop;
	unsigned long      arpresp_timeout;
	unsigned long      arpresp_period;
	unsigned long      arpresp_stat;
	unsigned long      arpresp_mediumtimeused;
	unsigned char      arpresp_retry_cnt;

	/* UDP hole-punch */
	unsigned int       udph_en: 1;
	unsigned int       udph_done: 1;
	unsigned int       udph_timeout_en: 1;
	unsigned int       udph_ack: 1;
	unsigned int       udph_reserved: 4;
	unsigned int       udph_type: 8;
	unsigned int       udph_payload_len: 16;
	unsigned long      udph_start;
	unsigned long      udph_stop;
	unsigned long      udph_timeout;
	unsigned long      udph_period;
	unsigned long      udph_stat;
	unsigned long      udph_mediumtimeused;
	unsigned char      udph_retry_cnt;

	/* SETPS */
	unsigned int       setps_en: 1;
	unsigned int       setps_done: 1;
	unsigned int       setps_timeout_en: 1;
	unsigned int       setps_ack: 1;
	unsigned int       setps_reserved: 28;
	unsigned long      setps_start;
	unsigned long      setps_stop;
	unsigned long      setps_timeout;
	unsigned long      setps_period;
	unsigned long      setps_stat;
	unsigned long      setps_mediumtimeused;
	unsigned char      setps_retry_cnt;

	/* ARP request */
	unsigned int       arpreq_en: 1;
	unsigned int       arpreq_done: 1;
	unsigned int       arpreq_timeout_en: 1;
	unsigned int       arpreq_ack: 1;
	unsigned int       arpreq_type: 1;
	unsigned int       arpreq_reserved: 27;
	unsigned long      arpreq_start;
	unsigned long      arpreq_stop;
	unsigned long      arpreq_timeout;
	unsigned long      arpreq_period;
	unsigned long      arpreq_stat;
	unsigned long      arpreq_mediumtimeused;
	unsigned char      arpreq_retry_cnt;

	/* Current schedule */
	unsigned long long current_counter;
	unsigned long long current_function_bit;
	unsigned long      current_interval;
	unsigned int       current_arbitrary: 26;
	unsigned int       current_align: 1;
	unsigned int       current_half: 1;
	unsigned int       current_preparation: 4;
	unsigned long      ready_prevent_sleep;

	/* Next schedule */
	unsigned long long next_counter;
	unsigned long long next_function_bit;
	unsigned long      next_interval;
	unsigned int       next_arbitrary: 26;
	unsigned int       next_align: 1;
	unsigned int       next_half: 1;
	unsigned int       next_preparation: 4;

	/* PBR */
	unsigned int       pbr_en: 1;
	unsigned int       pbr_bssid_match: 1;
	unsigned int       pbr_timestamp_parsing_done: 1;
	unsigned int       pbr_detected_bcmc: 1;
	unsigned int       pbr_detected_aid: 1;
	unsigned int       pbr_tim_parsing_done: 1;
	unsigned int       pbr_changed_ssid: 1;
	unsigned int       pbr_changed_ds_parameter_set: 1;
	unsigned int       pbr_fcs_error: 1;
	unsigned int       pbr_bcn_cancellation: 1;
	unsigned int       pbr_bcn_timeout_cancellation: 1;
	long long          pbr_done_c;
	long long          pbr_timestamp;

	/* DPM CLK */
	unsigned int       rtc_clk_ovfl: 1;
	unsigned int       rtc_wakeup_ovfl: 1;

	/* Time Information */
	long long          wakeup_time_c;
	long long          sleep_us;

	long long          bcn_start_c;
	long long          bcn_stop_c;
	//unsigned long      rx_on_start_c; ///< RX on time by CLK. Type is long long.
	unsigned long      rx_on_start; ///< RX on time by CLK
	//unsigned long      rx_on_end_c;   ///< RX off time by CLK. Type is long long.
	unsigned long      rx_on_end;   ///< RX off time by CLK
	unsigned long      tsflo_rx_start; ///< RX on start time by TSF
	unsigned long      tsflo_rx_end;   ///< RX on end time by TSF

	unsigned long      bcmc_start;
	unsigned long      bcmc_stop;

	unsigned long      deauth_start;
	unsigned long      deauth_stop;

	unsigned long      ps_start;
	unsigned long      ps_stop;

	unsigned long      uc_start;
	unsigned long      uc_stop;

	/* TX */
	//unsigned int       tx_en: 1;
	//unsigned int       tx_done: 1;
	//unsigned int       tx_timeout_en: 1;
	//unsigned int       tx_retry_limit_en: 1;
	//unsigned int       tx_ack: 1;
	//unsigned int       tx_reserved: 27;
	unsigned long      tx_pending;
	unsigned long      tx_start;
	unsigned long      tx_stop;
	unsigned long      tx_stat;
	unsigned long      tx_mediumtimeused;
	unsigned char      tx_retry_cnt;
	unsigned short     tx_seqn;
	unsigned long      max_tx_power;

	long long          next_wakeup_time;
};

extern void *timp_buf;

//extern unsigned long timp_get_backup_addr(struct dpm_param *dpmp);
extern void timp_set_backup_addr(struct dpm_param *dpmp, void *backup_addr);
extern void timp_set_period(enum TIMP_INTERFACE no, unsigned long period);
extern void timp_set_condition(enum TIMP_INTERFACE no, struct timp_data_tag *c);
extern void timp_init(void *back_addr, void *buf);
extern void timp_reset(enum TIMP_INTERFACE no, unsigned long rtc_tu);
extern void timp_reset_buf();
extern enum TIMP_UPDATED_STATUS timp_update(unsigned long rtc_tu);
extern struct timp_performance_tag *timp_get_performance(void);
extern struct timp_data_tag *timp_get_interface_data(enum TIMP_INTERFACE no);
extern struct timp_data_tag *timp_get_interface_current(enum TIMP_INTERFACE no);
extern struct timp_data_tag *timp_get_condition(enum TIMP_INTERFACE no);
/*extern void timp_display_interface_data(enum TIMP_UPDATED_STATUS updated,
						unsigned long timd_feature);*/
/*extern enum TIMP_UPDATED_STATUS timp_check_cond(enum TIMP_INTERFACE no,
						struct timp_data_tag *c,
						struct timp_data_tag *d);*/

/*extern void timp_conf(struct dpm_param *dpmp);
extern void timp_log(struct dpm_param *dpmp);*/

#else

#define SET_TIMP(arg0, arg1) (0)
#define GET_TIMP(arg0)       (0)

#endif /* TIMP_EN */

/// @} end of group ROMAC_TIMP

#endif /* __TIMP_PATCH_H__ */
