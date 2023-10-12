/**
 ****************************************************************************************
 *
 * @file key.h
 *
 * @brief Header file for WiFi MAC Protocol Key Processing
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

#ifndef I3ED11_KEY_H
#define I3ED11_KEY_H

#include "ieee80211.h"
#include "mac80211.h"

#define NUM_DEFAULT_KEYS 4
#define NUM_DEFAULT_MGMT_KEYS 2
#define MAX_PN_LEN 16

struct i3ed11_macctrl;
struct i3ed11_wifisubif;
struct um_sta_info;

/// internal key flags
enum i3ed11_internal_key_flags
{
	/// Indicates that this key is present in the hardware for TX crypto hardware acceleration.
	KEY_FLAG_UPLOADED_TO_HARDWARE	= BIT(0),
	/// Key is tainted and packets should be dropped.
	KEY_FLAG_TAINTED		= BIT(1),
};

enum i3ed11_internal_tkip_state
{
	DIW_TKIP_STATE_NOT_INIT,
	DIW_TKIP_STATE_PHASE1_DONE,
	DIW_TKIP_STATE_PHASE1_HW_UPLOADED,
};

struct tkip_ctx
{
	u32 iv32;
	u16 iv16;
	u16 p1k[5];
	u32 p1k_iv32;
	enum i3ed11_internal_tkip_state state;
};

struct i3ed11_key
{
	struct i3ed11_macctrl *macctrl;
	struct i3ed11_wifisubif *wsubif;
	struct um_sta_info *sta;
	struct diw_list_head list;

	unsigned int flags;

	struct
	{
		struct
		{
			SemaphoreHandle_t txlock;//spinlock_t txlock;

			struct tkip_ctx tx;

			struct tkip_ctx rx[I3ED11_NUM_TIDS];

			u32 mic_failures;
		} tkip;
		struct
		{
			atomic64_t tx_pn;
			u8 rx_pn[I3ED11_NUM_TIDS + 1][I3ED11_CCMP_PN_LEN];
			//struct crypto_aead *tfm;
			u32 replays;
		} ccmp;
		struct
		{
			atomic64_t tx_pn;
			u8 rx_pn[I3ED11_CMAC_PN_LEN];
			struct crypto_cipher *tfm;
			u32 replays;
			u32 icverrors;
		} aes_cmac;
		struct
		{
			/* generic cipher scheme */
			u8 rx_pn[I3ED11_NUM_TIDS + 1][MAX_PN_LEN];
		} gen;
	} u;

	// Number of times this key has been used.
	int tx_rx_count;

	// key config, must be last because it contains key material as variable length member.
	struct i3ed11_key_conf conf;
};

struct i3ed11_key *
i3ed11_key_alloc(u32 cipher, int idx, size_t key_len,
				 const u8 *key_data,
				 size_t seq_len, const u8 *seq,
				 const struct i3ed11_cipher_scheme *cs);
/**
 ****************************************************************************************
 * @brief Insert a key into data structures (wsubif, sta if necessary) to make it used, free old key.
 * @details On failure, also free the new key.
 ****************************************************************************************
 */
int i3ed11_key_link(struct i3ed11_key *key,
					struct i3ed11_wifisubif *wsubif,
					struct um_sta_info *sta);
void i3ed11_key_free(struct i3ed11_key *key, bool delay_tailroom);
void i3ed11_key_free_unused(struct i3ed11_key *key);
void i3ed11_set_default_key(struct i3ed11_wifisubif *wsubif, int idx,
							bool uni, bool multi);
void i3ed11_set_default_mgmt_key(struct i3ed11_wifisubif *wsubif,
								 int idx);
void i3ed11_free_keys(struct i3ed11_wifisubif *wsubif);
void i3ed11_free_sta_keys(struct i3ed11_macctrl *macctrl,
						  struct um_sta_info *sta);
#if 0	// lmj for UMAC ENCRYPTION
void i3ed11_enable_keys(struct i3ed11_wifisubif *wsubif);
#endif

void i3ed11_delayed_tailroom_dec(struct work_struct *wk);

#endif /* I3ED11_KEY_H */
