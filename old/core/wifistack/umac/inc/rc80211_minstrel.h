/**
 ****************************************************************************************
 *
 * @file rc80211_minstrel.h
 *
 * @brief Header file for Rate Control Algorithm
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

#ifndef __RC_MSTRL_H
#define __RC_MSTRL_H

#if 1 // CONFIG_DIW_RATE_CTRL
#include "share_rssimon.h"
#endif

/* Exponentially Weighted Moving Average */

#define EWMA_LEVEL	96
#define EWMA_DIV	128
#define SAMPLE_COLUMNS	10

#define MSTRL_SCALE  16
#define MSTRL_FRAC(val, div) (((val) << MSTRL_SCALE) / div)
#define MSTRL_TRUNC(val) ((val) >> MSTRL_SCALE)

#define MAX_THR_RATES 4

#if 1 // CONFIG_DIW_RATE_CTRL
#define MAX_UP_FAIL_HIST    3
#endif

static inline int
mstrl_ewma(int old, int new, int weight)
{
#if 1 // CONFIG_DIW_RATE_CTRL
	int diff, incr;

	diff = new - old;
	incr = (EWMA_DIV - weight) * diff / EWMA_DIV;

	return old + incr;
#else
	return (new * (EWMA_DIV - weight) + old * weight) / EWMA_DIV;
#endif
}

struct mstrl_rate
{
#if 0 //def FEATURE_UMAC_CODE_OPTIMIZE_1
	u8 bitrate;
	u8 rix;
#else
	/* rix can be assigned negative value, so rix should not be defined as nagative value */
	int bitrate;
	int rix;
#endif
	uint32_t perfect_tx_time;
	uint32_t ack_time;

#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	char sample_limit;
	uint8_t retry_count;
	uint8_t retry_count_cts;
	uint8_t retry_count_rtscts;
	uint8_t adjusted_retry_count;

	u8 success;
	u8 attempts;
	u8 last_attempts;
	u8 last_success;
#else
	int sample_limit;
	uint32_t retry_count;
	uint32_t retry_count_cts;
	uint32_t retry_count_rtscts;
	uint32_t adjusted_retry_count;

	u32 success;
	u32 attempts;
	u32 last_attempts;
	u32 last_success;
#endif
	u8 sample_skipped;

	u32 cur_prob;
	u32 probability;

	u32 cur_tp;

#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	//ULONG succ_hist;
	//ULONG att_hist;
#else
	ULONG succ_hist;
	ULONG att_hist;
#endif

#if 1 // CONFIG_DIW_RATE_CTRL
	// attempt count since last sample sent
	uint32_t att_last_prob;
#endif
};

struct mstrl_sta_info
{
	struct i3ed11_sta *sta;

	unsigned long stats_update;
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t sp_ack_dur;
	uint8_t rate_avg;

	uint8_t lowest_rix;
#else
	uint32_t sp_ack_dur;
	uint32_t rate_avg;

	uint32_t lowest_rix;
#endif

	u8 max_tp_rate[MAX_THR_RATES];
	u8 max_prob_rate;
	uint32_t packet_count;
	uint32_t sample_count;
	int sample_deferred;

#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t sample_row;
	uint8_t sample_column;

	char n_rates;
#else
	uint32_t sample_row;
	uint32_t sample_column;

	int n_rates;
#endif

#if 1 // CONFIG_DIW_RATE_CTRL
	/* signal's rssi from HW(TA rssi) */
	int ave_sig_rssi;
	int last_sig_rssi;

	/* next probe rate up index */
	char next_rate2up;

	/* next probe rate down index */
	char next_rate2down;

	/* flag to send probe frame */
	u32 flag2probe;

	/* expired time slot */
	unsigned long exp_tslot;

	/* threshold to up rate */
	u8 up_threshold;

	/* threshold to down rate */
	u8 down_threshold;

	/* up try failure history : when 3 times try has failed, hold up */
	u8 up_fail_history[MAX_UP_FAIL_HIST];

	/* counter that is pointing next write position of up_fail_history */
	u8 up_fail_hist_cnt;

	/* hold up rate info */
	u8 hold_up_rate;

	/* measured rssi when hold up rate */
	int rssi_at_hold_up;

	/* list to share rssi monitor hw */
	struct srm_list_head list;

	/* rssi monitor state */
	char srm_state;

	u16 tx_cnt;

	/* time of last clear tx_cnt */
	unsigned long clear_tx_cnt;
#endif

	struct mstrl_rate *r;
	bool prev_sample;

	/* sampling table */
	u8 *sample_table;

};

struct mstrl_priv
{
	struct i3ed11_hw *hw;
	bool has_mrr;
	uint32_t cwmin;
	uint32_t cwmax;
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t max_retry;
#else
	uint32_t max_retry;
#endif
	uint32_t segment_size;
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t update_interval;
	uint8_t lookaround_rate;
	uint8_t lookaround_rate_mrr;
#else
	uint32_t update_interval;
	uint32_t lookaround_rate;
	uint32_t lookaround_rate_mrr;
#endif
	u8 cck_rates[4];

};

extern struct diw_rc_ops macd11_mstrl;
extern int srm_td_cal_tx_data(char mode, void *sta_addr);

#endif
