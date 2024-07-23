/**
 ****************************************************************************************
 *
 * @file dpmsche_patch.h
 *
 * @brief DPMSCHE patch header
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

#ifndef __DPMSCHE_H__
#define __DPMSCHE_H__

#include "timp_patch.h"

#define DPMSCHE_SUCCESS         0
#define DPMSCHE_FAIL            -1
#define DPMSCHE_EMPTY           -2
#define DPMSCHE_OUT_OF_SCHE_BUF -3

#define DPMSCHE_FUNC_MAC_MASK   0x00ffffffffffffffLL
#define DPMSCHE_FUNC_NOMAC_MASK 0xff00000000000000LL
#define DPMSCHE_FUNC_TIM_MASK   0x0000000000000007LL
#define DPMSCHE_FUNC_NOTIM_MASK 0xfffffffffffffff8LL
#define DPMSCHE_FUNC_TIM        (DPMSCHE_TIM | DPMSCHE_BCN_LOSS | DPMSCHE_BCN_LOSS_CONFIRM)
#define DPMSCHE_FUNC_BCMC       (DPMSCHE_BCMC)
#define DPMSCHE_FUNC_PS_POLL    (DPMSCHE_PS_POLL | DPMSCHE_PS_POLL_RETRY | DPMSCHE_PS_POLL_TA)
#define DPMSCHE_FUNC_UC         (DPMSCHE_UC)
#define DPMSCHE_FUNC_KA         (DPMSCHE_KA | DPMSCHE_KA_RETRY | DPMSCHE_KA_TA)
#define DPMSCHE_FUNC_DEAUTH_CHK (DPMSCHE_DEAUTH_CHK | DPMSCHE_DEAUTH_CHK_RETRY | DPMSCHE_DEAUTH_CHK_TA)
#define DPMSCHE_FUNC_AUTO_ARP   (DPMSCHE_AUTO_ARP | DPMSCHE_AUTO_ARP_RETRY | DPMSCHE_AUTO_ARP_TA)
#define DPMSCHE_FUNC_ARPREQ     (DPMSCHE_ARPREQ | DPMSCHE_ARPREQ_RETRY | DPMSCHE_ARPREQ_TA)
#define DPMSCHE_FUNC_UDPH       (DPMSCHE_UDPH | DPMSCHE_UDPH_RETRY | DPMSCHE_UDPH_TA)
#define DPMSCHE_FUNC_TCP_KA     (DPMSCHE_TCP_KA | DPMSCHE_TCP_KA_RETRY | DPMSCHE_TCP_KA_TA)
#define DPMSCHE_FUNC_ARP_RESP   (DPMSCHE_ARP_RESP | DPMSCHE_ARP_RESP_RETRY | DPMSCHE_ARP_RESP_TA)
#define DPMSCHE_FUNC_SETPS      (DPMSCHE_SETPS | DPMSCHE_SETPS_RETRY | DPMSCHE_SETPS_TA)

#define DPMSCHE_PREP_NOMAC_MASK 0x00
#define DPMSCHE_PREP_MAC_MASK   0x0f

#define DPMSCHE_GET_COUNTER(interval, half) (((interval) << 1LL) | (half))
#define DPMSCHE_GET_INTERVAL(counter)       ((counter) >> 1LL)
#define DPMSCHE_GET_HALF(counter)           ((counter) & 1LL)

/* Calculating the interval with PERIOD_TY and INTV */
#define DPMSCHE_GET_P2I(dpmp, ty, intv) (dpm_sche_get_interval((dpmp), (ty)) * (intv))

enum DPMSCHE_PREP {
	DPMSCHE_PREP_NO_MAC   = 0x00, /* A type of DPMSCHE_PREP_NO_MAC will be NO MAC and App preparation. */
	DPMSCHE_PREP_RX_DSSS  = 0x01,
	DPMSCHE_PREP_RX_OFDM  = 0x02,
	DPMSCHE_PREP_TX       = 0x04,
	DPMSCHE_PREP_APP      = 0x08  /* A type of DPMSCHE_PREP_APP will be MAC and App preparation. */
};

enum DPMSCHE_FUNCTION {
	DPMSCHE_NO_FUNC          = 0x0000000000000000LL,
	DPMSCHE_TIM              = 0x0000000000000001LL,
	DPMSCHE_BCN_LOSS         = 0x0000000000000002LL,
	DPMSCHE_BCN_LOSS_CONFIRM = 0x0000000000000004LL,
	DPMSCHE_BCMC             = 0x0000000000000008LL,
	DPMSCHE_PS_POLL          = 0x0000000000000010LL,
	DPMSCHE_PS_POLL_RETRY    = 0x0000000000000020LL,
	DPMSCHE_PS_POLL_TA       = 0x0000000000000040LL,
	DPMSCHE_UC               = 0x0000000000000080LL,
	DPMSCHE_AUTO_ARP         = 0x0000000000000100LL,
	DPMSCHE_AUTO_ARP_RETRY   = 0x0000000000000200LL,
	DPMSCHE_AUTO_ARP_TA      = 0x0000000000000400LL,
	DPMSCHE_ARPREQ           = 0x0000000000000800LL,
	DPMSCHE_ARPREQ_RETRY     = 0x0000000000001000LL,
	DPMSCHE_ARPREQ_TA        = 0x0000000000002000LL,
	DPMSCHE_ARP_RESP         = 0x0000000000004000LL,
	DPMSCHE_ARP_RESP_RETRY   = 0x0000000000008000LL,
	DPMSCHE_ARP_RESP_TA      = 0x0000000000010000LL,
	DPMSCHE_UDPH             = 0x0000000000020000LL,
	DPMSCHE_UDPH_RETRY       = 0x0000000000040000LL,
	DPMSCHE_UDPH_TA          = 0x0000000000080000LL,
	DPMSCHE_TCP_KA           = 0x0000000000100000LL,
	DPMSCHE_TCP_KA_RETRY     = 0x0000000000200000LL,
	DPMSCHE_TCP_KA_TA        = 0x0000000000400000LL,
	DPMSCHE_KA               = 0x0000000000800000LL,
	DPMSCHE_KA_RETRY         = 0x0000000001000000LL,
	DPMSCHE_KA_TA            = 0x0000000002000000LL,
	DPMSCHE_SETPS            = 0x0000000004000000LL,
	DPMSCHE_SETPS_RETRY      = 0x0000000008000000LL,
	DPMSCHE_SETPS_TA         = 0x0000000010000000LL,
	DPMSCHE_DEAUTH_CHK       = 0x0000000020000000LL,
	DPMSCHE_DEAUTH_CHK_RETRY = 0x0000000040000000LL,
	DPMSCHE_DEAUTH_CHK_TA    = 0x0000000080000000LL,

	DPMSCHE_UC_CHK           = 0x0000000100000000LL,

	/* MAC Unrelated Functions */
	DPMSCHE_FULL_BOOT0       = 0x0100000000000000LL,
	DPMSCHE_FULL_BOOT1       = 0x0200000000000000LL,
	DPMSCHE_RESERVED_0       = 0x0400000000000000LL,
	DPMSCHE_RESERVED_1       = 0x0800000000000000LL,
	/*DPMSCHE_XTAL32K_READY  = 0x8000000000000000LL,*/

	DPMSCHE_ALL_FUNCTION     = 0xffffffffffffffffLL
};

enum DPMSCHE_UPDATE_TY {
	DPMSCHE_UPDATE_RTOS,
	DPMSCHE_UPDATE_PTIM
};

/*static inline unsigned long dpm_sche_get_interval(struct dpm_param *dpmp,
		unsigned long feature)*/
static inline unsigned long dpm_sche_get_interval(struct dpm_param *dpmp,
		unsigned char period_ty)
{
	/*int period_ty = (feature & DPM_SCHE_PERIOD_TY_MASK) >>
				DPM_SCHE_PERIOD_TY_POS;*/
	switch (period_ty) {
	/*case DPM_PERIOD_BI:
		return 1;*/
	case DPM_PERIOD_DTIM:
		return dpm_get_env_dtim_period(dpmp);
	case DPM_PERIOD_LI:
		return dpm_get_env_listen_intv(dpmp);
	case DPM_PERIOD_FORCE:
		return dpm_get_env_force_period(dpmp);
	}

	return 1;
}

/*
 * Converting seconds to period
 */
static inline unsigned long dpm_sche_get_period(struct dpm_param *dpmp,
		unsigned char period_ty, long sec)
{
	long long us = (long long) sec * 1000000LL;
	long long period_us = DPMSCHE_GET_P2I(dpmp, period_ty, 1) *
		TU2US(dpm_get_env_bcn_int(dpmp));
	unsigned long period;

	if (us < period_us)
		period = 1;
	else
		period = (us - (us % period_us)) / period_us;

	return period;
}

/*
 * Converting us to period
 */
static inline unsigned long dpm_sche_get_us_to_period(struct dpm_param *dpmp,
		unsigned char period_ty, long long us)
{
	long long period_us = DPMSCHE_GET_P2I(dpmp, period_ty, 1) *
		TU2US(dpm_get_env_bcn_int(dpmp));
	unsigned long period;

	if (us < period_us)
		period = 1;
	else
		period = (us - (us % period_us)) / period_us;

	return period;
}

/*
 * Converting ms to period
 */
static inline unsigned long dpm_sche_get_ms_to_period(struct dpm_param *dpmp,
		unsigned char period_ty, long long ms)
{
	long long us = (long long) ms * 1000LL;
	long long period_us = DPMSCHE_GET_P2I(dpmp, period_ty, 1) *
		TU2US(dpm_get_env_bcn_int(dpmp));
	unsigned long period;

	if (us < period_us)
		period = 1;
	else
		period = (us - (us % period_us)) / period_us;

	return period;
}

/*static inline long long dpm_sche_get_count(unsigned long interval,
		unsigned char half)
{
	unsigned long long count = (interval << 1) & half;
	return count;
}

static inline void dpm_sche_get_interval_half(unsigned long long count,
		unsigned long *interval, unsigned char *half)
{
	unsigned long long count = (interval << 1) & half;
	return count;
}

static inline long long dpm_sche_set_count(struct dpm_param *dpmp,
		unsigned long interval, unsigned char half)
{
}*/

extern void dpm_sche_init(struct dpm_param *dpmp);
extern void dpm_sche_delete_all_timer(struct dpm_param *dpmp);
extern void dpm_sche_delete(struct dpm_param *dpmp, enum DPMSCHE_FUNCTION func);
extern int dpm_sche_do(struct dpm_param *dpmp);
extern void dpm_sche_chk_dtim_cnt(struct dpm_param *dpmp, int dtim);
extern int dpm_sche_set_timer(struct dpm_param *dpmp, enum DPMSCHE_PREP prep,
				enum DPMSCHE_FUNCTION func,
				/*enum DPMSCHE_ATTR attr,*/
				unsigned long interval,
				unsigned long half,
				unsigned long arbitrary,
				int align);
extern void dpm_sche_set_prep(struct dpm_param *dpmp, unsigned long prep);
extern unsigned long dpm_sche_get_prep(struct dpm_param *dpmp);
extern unsigned long long dpm_sche_get_func(struct dpm_param *dpmp);
/*extern long long dpm_sche_get_sleep_time(struct dpm_param *dpmp);*/
extern long long dpm_sche_get_sleep_time_patch(struct dpm_param *dpmp);

extern void dpm_sche_plus_time(struct dpm_schedule_node *a,
		struct dpm_schedule_node *b,
		struct dpm_schedule_node *c);
extern void dpm_sche_minus_time(struct dpm_schedule_node *a,
		struct dpm_schedule_node *b,
		struct dpm_schedule_node *c);
extern void dpm_sche_update_time(struct dpm_schedule_node *a,
		struct dpm_schedule_node *node0);
extern void dpm_sche_get_next_bi(struct dpm_param *dpmp,
		unsigned long *interval, unsigned char *half);
extern void dpm_sche_get_next_dtim(struct dpm_param *dpmp,
		unsigned long *interval, unsigned char *half);
/*extern void dpm_sche_get_next_li(struct dpm_param *dpmp,
		unsigned long *interval, unsigned char *half);*/
extern void dpm_sche_get_next_force(struct dpm_param *dpmp,
		unsigned long *interval, unsigned char *half);
extern void dpm_sche_set_func(struct dpm_param *dpmp, unsigned long long func);
extern void dpm_sche_reset_func(struct dpm_param *dpmp,
                                unsigned long long func);
extern int dpm_sche_check_func(struct dpm_param *dpmp, unsigned long long func);
extern struct dpm_schedule_node *dpm_sche_get_node(struct dpm_param *dpmp,
                                                    unsigned long long func);
extern void dpm_sche_update(struct dpm_param *dpmp, int renewal,
								enum DPMSCHE_UPDATE_TY ty);

#endif
