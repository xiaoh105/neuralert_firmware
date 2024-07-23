/**
 ****************************************************************************************
 *
 * @file rc80211_minstrel_ht.h
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

#ifndef __RC_MSTRL_HT_H
#define __RC_MSTRL_HT_H

#if 1 // CONFIG_DIW_RATE_CTRL
#include "share_rssimon.h"
#endif

/* The number of streams can be changed to 2 to reduce code size and memory footprint. */
#ifdef CONFIG_MAC80211_RC_MINSTREL_VHT
#define MSTRL_VHT_MAX_STREAMS	2
#else
#define MSTRL_VHT_MAX_STREAMS	0
#endif
#define MSTRL_VHT_STREAM_GROUPS	6 /* BW(=3)*SGI(=2) */

#if 1 // CONFIG_DIW_RATE_CTRL
#define MSTRL_HT_USE_1STREAM_HT20_ONLY
#else
#undef MSTRL_HT_USE_1STREAM_HT20_ONLY
#endif

#ifndef MSTRL_HT_USE_1STREAM_HT20_ONLY
#define MSTRL_HT_MAX_STREAMS	3
#define MSTRL_HT_STREAM_GROUPS	4
#else
#define MSTRL_HT_MAX_STREAMS 	1
#define MSTRL_HT_STREAM_GROUPS	2
#endif

#define MSTRL_HT_GROUPS_NB	(MSTRL_HT_MAX_STREAMS * MSTRL_HT_STREAM_GROUPS)
#define MSTRL_VHT_GROUPS_NB  (MSTRL_VHT_MAX_STREAMS * MSTRL_VHT_STREAM_GROUPS)
#define MSTRL_CCK_GROUPS_NB	1
#define MSTRL_GROUPS_NB (MSTRL_HT_GROUPS_NB +	\
						 MSTRL_VHT_GROUPS_NB +	\
						 MSTRL_CCK_GROUPS_NB)

#define MSTRL_HT_GROUP_0	0
#define MSTRL_CCK_GROUP		(MSTRL_HT_GROUP_0 + MSTRL_HT_GROUPS_NB)
#define MSTRL_VHT_GROUP_0	(MSTRL_CCK_GROUP + 1)

#ifdef CONFIG_MAC80211_RC_MINSTREL_VHT
#define DIW_MCS_GROUP_RATES		10
#else
#define DIW_MCS_GROUP_RATES		8
#endif

#if 1 // CONFIG_DIW_RATE_CTRL
#define MAX_UP_FAIL_HIST    3
#endif

struct mcs_group
{
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	u16 flags;
	uint8_t streams;
#else
	u32 flags;
	uint32_t streams;
#endif
	uint32_t duration[DIW_MCS_GROUP_RATES];
};

extern const struct mcs_group mstrl_mcs_groups[];

struct mstrl_rate_stats
{
	/* current / last sampling period attempts/success counters */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t attempts, last_attempts;
	uint8_t success, last_success;

	/* total attempts/success counters */
	//ULONG att_hist, succ_hist;
	uint8_t att_hist;
#else
	uint32_t attempts, last_attempts;
	uint32_t success, last_success;

#if 1 // CONFIG_DIW_RATE_CTRL
	/* attempt count since last sample sent */
	uint32_t att_last_prob;
#endif

	/* total attempts/success counters */
	ULONG att_hist, succ_hist;
#endif

	/* current throughput */
	uint32_t cur_tp;

	/* packet delivery probabilities */
	uint32_t cur_prob, probability;

	/* maximum retry counts */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t retry_count;
	uint8_t retry_count_rtscts;
#else
	uint32_t retry_count;
	uint32_t retry_count_rtscts;
#endif
	bool retry_updated;
	u8 sample_skipped;
};

struct mstrl_mcs_group_data
{
	u8 index;
	u8 column;

	/* bitfield of supported MCS rates of this group */
	u16 supported;

	/* selected primary rates */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t max_tp_rate;
	uint8_t max_tp_rate2;
	uint8_t max_prob_rate;
#else
	uint32_t max_tp_rate;
	uint32_t max_tp_rate2;
	uint32_t max_prob_rate;
#endif
	/* MCS rate statistics */
	struct mstrl_rate_stats rates[DIW_MCS_GROUP_RATES];
};

struct mstrl_ht_sta
{
	struct i3ed11_sta *sta;

	/* ampdu length (average, per sampling interval) */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t ampdu_len;
	uint8_t ampdu_packets;
#else
	uint32_t ampdu_len;
	uint32_t ampdu_packets;
#endif
	/* ampdu length (EWMA) */
	uint32_t avg_ampdu_len;

#if 1 // CONFIG_DIW_RATE_CTRL
	/* signal's rssi from HW(TA rssi) */
	int ave_sig_rssi;
	int last_sig_rssi;

	/* expired time slot */
	unsigned long exp_tslot;

	/* list to share rssi monitor hw */
	struct srm_list_head list;

	/* rssi monitor state */
	char srm_state;
#endif

	/* best throughput rate */
	uint32_t max_tp_rate;

	/* second best throughput rate */
	uint32_t max_tp_rate2;

	/* best probability rate */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t max_prob_rate;
	uint8_t max_prob_streams;
#else
	uint32_t max_prob_rate;
	uint32_t max_prob_streams;
#endif
	/* time of last status update */
	unsigned long stats_update;
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t aggr_check_pcnt;
#else
	uint32_t aggr_check_pcnt;
#endif
	/* overhead time in usec for each frame */
#ifdef FEATURE_UMAC_CODE_OPTIMIZE_1
	uint8_t overhead;
	uint8_t overhead_rtscts;
#else
	uint32_t overhead;
	uint32_t overhead_rtscts;
#endif

	uint32_t total_packets;
	uint32_t sample_packets;

	/* tx flags to add for frames for this sta */
	u32 tx_flags;

	u8 sample_wait;
	u8 sample_tries;
	u8 sample_count;
	u8 sample_slow;

#if 1 // CONFIG_DIW_RATE_CTRL
	/* next probe rate up index */
	char next_rate2up;

	/* next probe rate down index */
	char next_rate2down;

	/* flag to send probe frame */
	u32 flag2probe;

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

	//jhkim 1127
	u16 tx_cnt;

	/* time of last clear tx_cnt */
	unsigned long clear_tx_cnt;
#endif

	/* current MCS group to be sampled */
	u8 sample_group;

	u8 cck_supported;
	u8 cck_supported_short;

	/* MCS rate group info and statistics */
	struct mstrl_mcs_group_data groups[MSTRL_GROUPS_NB];
};

#define MAX_MINSTEL_RATE_CONTROL	12
struct mstrl_ht_sta_priv
{
#ifdef FREERTOS_AVIOD_EXCEP_STA
		struct mstrl_ht_sta ht;
		struct mstrl_sta_info legacy;
#else
	union
	{
		struct mstrl_ht_sta ht;
		struct mstrl_sta_info legacy;
	};
#endif

#ifdef UMAC_MSTRL_WITHOUT_MALLOC
	struct mstrl_rate ratelist[MAX_MINSTEL_RATE_CONTROL];
	u8	sample_table[10 * MAX_MINSTEL_RATE_CONTROL];
#else
	struct mstrl_rate *ratelist;
	u8	*sample_table;
#endif
	bool is_ht;
};

#endif
