/**
 ****************************************************************************************
 *
 * @file features.h
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

#include "sdk_type.h"

/* Features header definition */
#ifndef _FEATURES_H
#define _FEATURES_H

#undef CONFIG_MAC80211_VHT

#undef UMAC_MLME_DEBUG
//#undef UMAC_TX_DEBUG
#undef UMAC_RX_DEBUG
#undef UMAC_MAIN_DEBUG
//#undef  UMAC_NI_DEBUG
#undef  UMAC_FC80211_DEBUG
#undef UMAC_IFACE_DEBUG
#undef UMAC_DBG_DRV_DEF
//#undef UMAC_SCAN_DEBUG

#ifdef __TEST_CMD_FOR_RATE__
#define CONFIG_UMAC_MCS_TEST //courmagu 20150227
#else
#undef CONFIG_UMAC_MCS_TEST //courmagu 20150227
#endif	/* __TEST_CMD_FOR_RATE__ */

/* As command(umac.aggtx) */
//#define FEATURE_DONT_USE_AGGTX

/// control of multicast address list set and filtering recommand as "undef".
#undef UMAC_MCAST_ADDRLIST

/// POWER Save monitoring flag recommand as "undef".
#undef UMAC_PS_CNTL_MON

/* SDK 3.0 FreeRTOS DPM Feature enable */
#define DPM_PORT

/// UMAC Additional Code for DPM.
#ifdef DPM_PORT
#define WITH_UMAC_DPM
#endif	/*DPM_PORT*/

/// PS related and frame queuing sync recommand as "define"  in AP Powersave function.
#define UMAC_PS_RACE_AVOID

/// It should enable the broadcast beacon bucket in lmac4.0.
#define UMAC_USE_DIRECT_BC_BEACON_BUCKET

/// SKB LIST Locking, don't need it.
#undef SKB_LIST_LOCK

/// By lmac tx queue stop/wakeup, tx queue flow control supporting.
#undef UMAC_SKB_TX_FLOWCONTROL

/// Locking for frame registeration and mgmt for P2P.
#define UMAC_FRAME_MGMT_LOCK

/// TX Power control for AP Mode Dev. version. do disable.
#define UMAC_TX_POWER_CTRL

/// RX A-MSDU Packet Dev. We don't process MSDU packet RX.
#define UMAC_NOT_AMSDU_RX_PROC

#undef SPECTRUM_MANAGEMENT_FOR_5GHZ

/// PS UMAC is not support Power save any action.
#undef UMAC_HW_SUPPORTS_DYNAMIC_PS

/// Delayed Work queue Disable.
#undef UMAC_DELAY_WORKQUEUE_ENABLE

/// AP mode ACS support.
#define UMAC_ACS_SUPPORT

/// UMAC Internal TX per Packet.
#undef  UMAC_INTERNAL_TX

/// Power down API check the UMAC TX/RX path.
#ifdef WITH_UMAC_DPM
#undef DPM_RECONNECT_WO_SCAN
#endif

/// DPM Full booting without Malloc.
/// In FreeRTOS, IT's not used.
#undef UMAC_DPM_BOOTING_WO_MALLOC

#define UMAC_THREAD_STACK_ENABLE

/// Using HW Michael Algorithm for TKIP.
#define HW_MICHAEL_BLOCK

/// has always channel ctx.
#undef UMAC_USE_NO_CHANCTX

/// In AP Mode, Beacon registering and filtering we don't use it, do disable.
#undef UMAC_BEACON_REGISTER
#undef UMAC_BEACON_RSSI_DBG

#define UMAC_DBG_LOG_EN

/// UMAC TX lost packets.
#undef UMAC_LOST_PACKETS

/// In HT MSTRL rate control station alloc, lagacy rate control allocated as static, but dynamic.
#undef UMAC_MSTRL_WITHOUT_MALLOC

/// During the DPM & Con/Discon , Structure Guide for STA Priv Rate control.
#define FEATURE_DPM_RATE_PRIVSTA_MUTEX

/// RX Reorder buf from wpkt_buff to wpkt_buff_list.
#undef  UMAC_CHANGE_REORDER_BUF

/// A-MSDU, RX Defregment.
#define FEATURE_AMSDU_AND_DEFRAGEMENT
#ifdef FEATURE_AMSDU_AND_DEFRAGEMENT
#ifdef UMAC_NOT_AMSDU_RX_PROC
#undef UMAC_NOT_AMSDU_RX_PROC
#endif
#define FEATURE_USED_LOW_MEMORY_SCAN
#define FEATURE_USED_TRANSFER_NX_PACKET
#undef FEATURE_AP_MODE_CON_MGMT_BYPASS
#undef FEATURE_AGG_RESP_NON_IFACE_WORK
#endif /* FEATURE_AMSDU_AND_DEFRAGEMENT */

/// Tx Power Control by grade index since MPW3 chip.
#define FEATURE_TX_PWR_CTL_GRADE_IDX

/// Tx power Control by power grade table.
#define FEATURE_TX_PWR_CTL_GRADE_IDX_TABLE

#if 1 // CONFIG_DIW_RATE_CTRL
// Most of UMAC
#define FEATURE_UMAC_CODE_OPTIMIZE_2
// rate control only
//#define FEATURE_UMAC_CODE_OPTIMIZE_1 /* ToDo : Need to optimize again */
#undef FEATURE_UMAC_CODE_OPTIMIZE_1
#else
#define FEATURE_UMAC_CODE_OPTIMIZE_1
#endif

#define FEATURE_EAPOL_DONT_ENCRYPT
#define FEATURE_RATECON_REARRANGE
#define FEATURE_NEW_QUEUE

/// Modify P2P PS' Absence and CTwidow value.
#if (defined __LIGHT_SUPPLICANT__) || (defined __SUPPORT_MESH__)
#undef FEATURE_P2P_PROBE
/// Modify P2P PS' Absence and CTwidow value.
#undef FEATURE_P2P_M_ABS_CTW_VAL
#else
#if (defined __SUPPORT_P2P__)
#define FEATURE_P2P_PROBE
/// Modify P2P PS' Absence and CTwidow value.
#define FEATURE_P2P_M_ABS_CTW_VAL
#else
#undef FEATURE_P2P_PROBE
/// Modify P2P PS' Absence and CTwidow value.
#undef FEATURE_P2P_M_ABS_CTW_VAL
#endif /* __SUPPORT_P2P__ */
#endif /* __LIGHT_SUPPLICANT__ || __SUPPORT_MESH__ */

#define FEATURE_STA_CLEANUP_WORK
#define FEATURE_ADD_REORDER_WORK
#define FEATURE_CHG_CHANNEL

#if defined ( __SMALL_SYSTEM__ )
#undef FEATURE_BRIDGE
#else
#define FEATURE_BRIDGE
#if (defined __LIGHT_SUPPLICANT__) || (defined __SUPPORT_MESH__)
/// Enable P2P GO bridge function.
#undef FEATURE_P2P_GO_BRIDGE
#else
#if (defined __SUPPORT_P2P__)
/// Enable P2P GO bridge function.
#define FEATURE_P2P_GO_BRIDGE
#else
#undef	FEATURE_P2P_GO_BRIDGE
#endif /* __SUPPORT_P2P__ */
#endif /* __LIGHT_SUPPLICANT__ || __SUPPORT_MESH__ */
#endif /*__SMALL_SYSTEM__*/

/// Feature of setting the MAX TX Pending Buffer size as 8 per ACs.
#define UMAC_LOCAL_PENDING_BUFFER_LIMIT

/// In case of AP Mode, during the TX, let's do check station's connection.
#define FEATURE_AP_MODE_STA_CHK
#undef FEATURE_MGMT_POOL
#define FEATURE_RWNX_QUEUE_LIST
#define FEATURE_NEW_WORK_LIST
#ifdef __SUPPORT_IEEE80211W__
#define FEATURE__IEEE80211W
#endif

#define FEATURE_DESTROY_AUTH_DATA_BSS_LOCK

#define	NEW_BSS_UPDATE

#define FEATURE_STAIC_RATES

#ifdef __SUPPORT_WPA3_SAE__
#define FEATURE_WPA3_SAE
#else
#undef FEATURE_WPA3_SAE
#endif

#ifdef __SUPPORT_WPA3_OWE__
#define FEATURE_OWE
#else
#undef FEATURE_OWE
#endif

#ifdef __SUPPORT_WPA_ENTERPRISE_CORE__
	#define FEATURE_WPA_ENTERPRISE__
	#ifdef __SUPPORT_WPA3_ENTERPRISE_192B_CORE__
		#define FEATURE_WPA3_ENTERPRISE_192B
	#endif // __SUPPORT_WPA3_ENTERPRISE_192B_CORE__
#endif // __SUPPORT_WPA_ENTERPRISE_CORE__

#ifdef __SUPPORT_MESH__
#define FEATURE_UMAC_MESH
//#define CPTCFG_MACD11_MESH
#define CONFIG_MAC80211_MESH

/* If want to use Mesh PS, Define this one. But we don't gurantee. */
#undef  FEATURE_UMAC_MESH_PS
/* If want to use Mesh MPM in MAC Layer, Define this one. Currently User MPM is used. */
#undef  FEATURE_UMAC_USE_MPM

#define FEATURE_MESH_STAR_LINK_CONTROL

/* Mesh Path timer should be avoided memory API so, Timer was changed as work queue. */
#define FEATURE_MESH_TIMER_RUN_TO_WORK

#endif /* __SUPPORT_MESH__ */

#if defined (__SUPPORT_SEND_MESH_BEACON__)
#define FEATREU_MESH_SEND_BEACON_DIRECT
#endif

#ifdef FEATURE_MESH_STAR_LINK_CONTROL_ADDED
/// Over hearing packet blocking feature.
#define FEATURE_UMAC_OVERHEARING_PATCH
#endif

/// Scan with BSSID
#ifdef __SUPPORT_BSSID_SCAN__
#define	FEATURE_UMAC_SCAN_WITH_BSSID
#else
#undef	FEATURE_UMAC_SCAN_WITH_BSSID
#endif

/// Scan with Direct SSID
#ifdef __SUPPORT_DIR_SSID_SCAN__
#define	FEATURE_UMAC_SCAN_WITH_DIR_SSID
#else
#undef	FEATURE_UMAC_SCAN_WITH_DIR_SSID
#endif

#if defined (__SUPPORT_OWE_TRANS__)
#define CONFIG_OWE_TRANSITION_MODE
#endif

#define PBUF_DIRECT


#ifndef STACK_TO_ZEROCOPY
// Do not copy pbuf on the MAC 802.11 stack from TCP/IP stack
#define STACK_TO_ZEROCOPY
#endif

#ifdef STACK_TO_ZEROCOPY
// Use queue event between network interface and umac
#define USE_UMAC_TX_SEND_EVENT
// Create IP stack thread 
#undef USE_UMAC_RX_IPSTACK_THREAD
// Use xQueueSendFromISR instead of xQueueSend in umac rx thread
#endif

// Use lwip mbox when transmittting rx network packet
#define RX_USE_LWIP_MBOX

#define AMSDU_FREERTOS
#undef RX_PKT_LOOP

#define THROUGHPUT_OPTIMIZE

// Avoid exception state when STA is connecting/disconnecting.
#define FREERTOS_AVIOD_EXCEP_STA

/* In AP mode, To recover UMAC Tx block pool full issue */
#undef FEATURE_TX_BUF_FULL_REC

#endif /* _FEATURES_H */
