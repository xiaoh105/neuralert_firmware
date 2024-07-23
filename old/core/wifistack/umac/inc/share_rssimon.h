/**
 ****************************************************************************************
 *
 * @file share_rssimon.h
 *
 * @brief Header file for Sharing RSSI monitor (SRM) HW
 *
 * Copyright (c) 2012-2022 Renesas Electronics. All rights reserved.
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

#ifndef SHARE_RSSI_MON_H
#define SHARE_RSSI_MON_H

#define RC_DEBUG_EN    (0)

#if (RC_DEBUG_EN)
#define RC_PRTF(mask, ...)   UM_PRTF(##mask,##__VA_ARGS__)
#else
#define RC_PRTF(...)
#endif


enum wlan_mode
{
	SRM_11G = 1,
	SRM_11N,
	SRM_MODE_MAX,
};

enum srm_turn
{
	SRM_YES = 1,
	SRM_NO,
	SRM_TRUN_MAX,
};

enum srm_state
{
	SRM_IDLE = 1,
	SRM_SET,
	SRM_STATE_MAX,
};

struct srm_list_head
{
	struct srm_list_head *next, *prev;
	unsigned short mode;
};

/**
 ****************************************************************************************
 * @brief Timer call back function pointer.
 ****************************************************************************************
 */

#if 0
typedef void (*srm_timer_cb_fp)(unsigned long input);

struct srm_timer
{
	TimerHandle_t conb;
	srm_timer_cb_fp cb;
	unsigned long exp_t;
	unsigned long exp_interval;
	void *mp;
};
#else
//typedef void (*srm_timer_cb_fp)(void *);

struct srm_timer
{
	TimerHandle_t conb;
    int  (*cb)(void *);
	unsigned long exp_t;
	unsigned long exp_interval;
	void *mp;
};
#endif
/// Information structure containing info to use TA RSSI HW together.
struct rssimon_info
{
	/// Allocated time slot, based on OS time tick, 10ms.
	unsigned long alloc_tslot;

	/// Total time slot that all STA will share.
	unsigned long total_tslot;

	/// Number of associated STA.
	unsigned short assoc_sta_num;
	unsigned short g_assoc_sta_num;

	/// Address of structure of each station's rate control information.
	void *cur_turn_g;
	void *cur_turn_n;

	/// List of rate control station info.
	struct srm_list_head sta_list;
};

static inline void
__lst_add(struct srm_list_head *new, struct srm_list_head *prev,
		  struct srm_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void
lst_add(struct srm_list_head *new, struct srm_list_head *head)
{
	__lst_add(new, head, head->next);
}

static inline void
__lst_del(struct srm_list_head *prev, struct srm_list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void
__lst_del_entry(struct srm_list_head *entry)
{
	__lst_del(entry->prev, entry->next);
}

void srm_init(void);

int srm_unreg_sta (void *sta_info, char mode);

int srm_reg_sta (void *sta_info, char mode, char *mac_addr);

void srm_set_monitor(unsigned char *addr);

void srm_clear_monitor(void);

void srm_get_rssi(void *sta_addr, char mode);

void srm_apmode_get_rssi(void *sta_tab, char mode, char *sta_mac);

int srm_get_tsta_num(void);

#endif // SHARE_RSSI_MON_H
