/**
 ****************************************************************************************
 *
 * @file umac_util.h
 *
 * @brief Header file for WiFi MAC Protocol Utility Function
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

#ifndef _UMAC_UTIL_H
#define _UMAC_UTIL_H

#include <stdio.h>
#include "features.h"
#ifdef WITH_UMAC_DPM

/* Total UMAC Retention memory :: 0x64 */
/* UMAC DPM Info  UMAC_BASE + 0X36 */
/* DPM MAC ADDRESS UMAC_BASE + 0x70 */
#define DPM_CCMP_RX_PN_MAX	8
typedef struct _umac_dpm_info_
{
	volatile unsigned char dpm_bssid[6];
	volatile unsigned short dpm_freq;
	volatile unsigned char	dpm_qos;
	volatile unsigned char 	dpm_11b_flag;
	volatile unsigned short dpm_cap;
	volatile unsigned char  dpm_ht_supported;
	volatile unsigned char	dpm_ch_width;
	volatile unsigned char  	dpm_key_seq_num[6];
	volatile unsigned short dpm_seq_num[0x10];	//I3ED11_QOS_CTL_TID_MASK		0x000f

	volatile unsigned char	dpm_rx_pn[DPM_CCMP_RX_PN_MAX][6];	// ccmp rx pn number
	//volatile unsigned char	dpm_dummy2;
	volatile unsigned short	dpm_tim_wakeup_dur;

	volatile unsigned char	dpm_ddps_flag;	//DDPS Enable flag
	volatile unsigned char	dpm_dummy;
} UMAC_DPM_INFO;
#endif
#define RETM_BYTE_READ(addr, data)	*(data) = *((volatile UINT8 *)(addr))
#define RETM_WORD_READ(addr, data)	*(data) = *((volatile UINT16 *)(addr))
#define RETM_LONG_READ(addr, data)	*(data) = *((volatile UINT32 *)(addr))

#define RETM_BYTE_WRITE(addr, data)	*((volatile UINT8 *)(addr)) = (data)
#define RETM_WORD_WRITE(addr, data)	*((volatile UINT16 *)(addr)) = (data)
#define RETM_LONG_WRITE(addr, data)	*((volatile UINT32 *)(addr)) = (data)

/*
MPW
DA16200 : 0x60F3FFFC / 0xC1AA
FC9060 : 0x60F3FFFC / 0xC1AB

SLR
SLR1.0 : 0x60F3FFFC / 0xC2AA
SLR1.1 : 0x60F3FFFC / 0xC2AB
*/

//#define PRODUCT_INFO_RL_RF9050		0x9050
//#define PRODUCT_INFO_RL_RF9060		0x9060

#define REG_RAMLIB_WR(addr, data) *((volatile uint32_t *)(addr)) = (data)
#define REG_RAMLIB_RD(addr)       (*(volatile uint32_t *)(addr))



#ifdef UMAC_DBG_LOG_EN

/// UMAC debug info.
struct um_dbg_info_
{
	/// Debug mask.
	u16 msk; // CONFIG_DIW_RATE_CTRL u8 -> u16
	/// Whether debug mode is turned on or not.
	u8 is_on;
};

extern struct um_dbg_info_ um_dbg_info;

/*
  * UMAC Debug Mask
  * To use 2nd argument in UM_PRTF macro.
  * e.g. UM_PRTF( UM_SC, " Start scan \n");
  */
#define UM_TX            BIT(0)    //0x01 :mask value, i3ed11_xmit
#define UM_TX2LM         BIT(1)    //0x02 ,                i3ed11_tx_frags
#define UM_RX            BIT(2)    //0x04,                 i3ed11_rx_irqsafe
#define UM_RX_NI         BIT(3)    //0x08,                 wifi_netif_rx_packet_proc
#define UM_SC            BIT(4)    //0x10,                 scan
#define UM_CN            BIT(5)    //0x20,                 connection
#define UM_MF			BIT(6)	//0x40, malloc fail
#define UM_MESH			BIT(7)	//0x80, Mesh DBG
#if 1 // CONFIG_DIW_RATE_CTRL
#define RC_INIT			BIT(8)	//0x100,
#define RC_GET			BIT(9)	//0x200,
#define RC_UP			BIT(10)	//0x400,
#define RC_DINT			BIT(11)	//0x800,
#endif

#if 0 // CONFIG_DIW_RATE_CTRL
#define UM_PRTF(mask, ...)
#else
#define UM_PRTF(mask, ...)                       \
	if( mask & um_dbg_info.msk ){            \
		PRINTF("["#mask"]"__VA_ARGS__ );     \
	}
#endif
#endif //UMAC_DBG_LOG_EN

#ifdef WITH_UMAC_DPM
unsigned char umac_dpm_check_init(void);
#endif

extern void fc80211_da16x_pri_pwr_down(unsigned char retention);
extern void fc80211_da16x_sec_pwr_down(unsigned long long usec, unsigned char retention);

#endif
