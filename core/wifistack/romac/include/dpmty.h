/**
 ****************************************************************************************
 *
 * @file     dpmty.h
 *
 * @brief    DPM Types
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

#ifndef __DPMTY_H__
#define __DPMTY_H__

/**
 * @file     dpmty.h
 * @brief    DPM Types
 * @details  This file defines the DPM types.
 *           This header file is used in RTOS & PTIM.
 * @defgroup ROMAC_COM_TY
 * @ingroup  ROMAC_COM
 * @{
 */
#include "dpmconf.h"

#ifdef TIM_SIMULATION
#define CO_CLZ                  __lzcnt
#else
#define CO_CLZ                  __CLZ
#endif

#define DPM_MAC_LEN             (6)

#define CHK_NO_FEATURE(n)       (!(dpm_get_env_feature(dpmp) & (n)))
#define CHK_FEATURE(n)          (dpm_get_env_feature(dpmp) & (n))
#define MK_FEATURE(n)           (dpm_set_env_feature(dpmp, dpm_get_env_feature(dpmp) | (n)))
#define CHK_NO_TX_FEATURE(n)    (!(dpm_get_env_tx_feature(dpmp) & (n)))
#define CHK_TX_FEATURE(n)       (dpm_get_env_tx_feature(dpmp) & (n))
#define MK_TX_FEATURE(n)        (dpm_set_env_tx_feature(dpmp, dpm_get_env_tx_feature(dpmp) | (n)))
#define CHK_NO_ST_STATUS(n)     (!(dpm_get_st_status(dpmp) & (n)))
#define CHK_ST_STATUS(n)        (dpm_get_st_status(dpmp) & (n))
#define MK_ST_STATUS(n)         (dpm_set_st_status(dpmp, dpm_get_st_status(dpmp) | (n)))
#define CHK_NO_DBG_FEATURE(n)   (!(dpm_get_env_dbg_feature(dpmp) & (n)))
#define CHK_DBG_FEATURE(n)      (dpm_get_env_dbg_feature(dpmp) & (n))
#define MK_DBG_FEATURE(n)       (dpm_set_env_dbg_feature(dpmp, dpm_get_env_dbg_feature(dpmp) | (n)))
#define CHK_NO_FILTER(n)        (!(dpm_get_env_rx_filter(dpmp) & (n)))
#define CHK_FILTER(n)           (dpm_get_env_rx_filter(dpmp) & (n))
#define MK_FILTER(n)            (dpm_set_env_rx_filter(dpmp, dpm_get_env_rx_filter(dpmp) | (n)))
#define CHK_NO_IP_FILTER(n)     (!(dpm_get_env_ip_filter(dpmp) & (n)))
#define CHK_IP_FILTER(n)        (dpm_get_env_ip_filter(dpmp) & (n))
#define MK_IP_FILTER(n)         (dpm_set_env_ip_filter(dpmp, dpm_get_env_ip_filter(dpmp) | (n)))
#define CHK_NO_TIMD_FEATURE(n)  (!(dpm_get_env_timd_feature(dpmp) & (n)))
#define CHK_TIMD_FEATURE(n)     (dpm_get_env_timd_feature(dpmp) & (n))
#define MK_TIMD_FEATURE(n)      (dpm_set_env_timd_feature(dpmp, dpm_get_env_timd_feature(dpmp) | (n)))
//#define CHK_NO_TIM_FEATURE(n)   (!(dpm_get_env_tim_feature(dpmp) & (n)))
//#define CHK_TIM_FEATURE(n)      (dpm_get_env_tim_feature(dpmp) & (n))
//#define MK_TIM_FEATURE(n)       (dpm_set_env_tim_feature(dpmp, dpm_get_env_tim_feature(dpmp) | (n)))
//#define CHK_NO_TIMP_FEATURE(n)  (!(dpm_get_env_timp_feature(dpmp) & (n)))
//#define CHK_TIMP_FEATURE(n)     (dpm_get_env_timp_feature(dpmp) & (n))
//#define MK_TIMP_FEATURE(n)      (dpm_set_env_timp_feature(dpmp, dpm_get_env_timp_feature(dpmp) | (n)))

#define MK_TRG_UE(n)            (dpm_set_st_trigger_ue(dpmp, (n)))
#define MK_TRG_FBOOT(n)         (dpm_set_st_trigger_fboot(dpmp, (n)))
#define MK_TRG_RA(n)            (dpm_set_st_trigger_ra(dpmp, (n)))
#define MK_TRG_PTIM(n)          (dpm_set_st_trigger_ptim(dpmp, (n)))

#define CHK_WAKEUP(f, n)        ((f) & (n))

//#define PERIOD_TY_MASK          0x03
//#define NON_PERIOD_TY_MASK      0xfc
//#define GET_TIM_PERIOD_TY(dpmp) (dpm_get_env_tim_feature(dpmp) & PERIOD_TY_MASK)
//#define SET_TIM_PERIOD_TY(dpmp, ty) (dpm_set_env_tim_feature(dpmp, ((dpm_get_env_tim_feature(dpmp) & NON_PERIOD_TY_MASK) | ty)))

#define TIM_PHY_PRIM            (0)
#define TIM_VER_INIT            (0xffffffff)

#define US2CLK(us)   ((((long long ) (us)) << 9LL) / 15625LL)
#define SEC2CLK(sec) (((long long ) (sec)) << 15LL)

#define CLK2US(clk)  ((((long long ) (clk)) * 15625LL) >> 9LL)
#define CLK2MS(clk)  ((CLK2US(clk)) / 1000LL)
#define CLK2SEC(clk) (((long long ) (clk)) >> 15LL)

#define TU2US(tu)    (((long long) (tu)) << 10LL) /* 1 Tu = 1024 us */
#define US2TU(us)    (((long long) (us)) >> 10LL) /* 1 Tu = 1024 us */

#define CLK2TU(clk)  ((((long long) (clk)) * 15625LL) >> 19LL) /* 1 Tu = 1024 us */


/**
 * @enum DPM Result
 */
enum {
	DPM_SUCCESS             = 0,
	DPM_E_FULL              = -1,
	DPM_E_FAIL              = -2,
	DPM_E_NO_ENCR_KEY       = -3,
	DPM_E_AP_RESET          = -4,
	DPM_E_NO_ENTRY          = -5,
	DPM_E_INVALID_FUNC      = -6,
	DPM_E_CCMP_PN           = -7,
	DPM_E_DBG_MODE          = -8,
	DPM_E_FUNC_CHK_DONE     = -9
};

/**
 * @enum   DPM Status
 * @brief  TIM status & error status for application
 * @detail MSB 8 bits is STE mask and LSB 24 bits is ST mask.
 */
enum {
	DPM_ST_UNDEFINED                = 0x00000000,
	DPM_ST_UC                       = 0x00000001, ///< UC
	DPM_ST_BCMC                     = 0x00000002, ///< BC/MC
	DPM_ST_BCN_CHANGED              = 0x00000004,
	DPM_ST_NO_BCN                   = 0x00000008,
	DPM_ST_FROM_FAST                = 0x00000010,
	DPM_ST_KEEP_ALIVE_NO_ACK        = 0x00000020,
	DPM_ST_FROM_KEEP_ALIVE          = 0x00000040,
	DPM_ST_NO                       = 0x00000080,
	DPM_ST_NO_BCN_KEEP              = 0x00000100, ///< NO BCN & KEEP SUCCESS
	DPM_ST_UC_MORE                  = 0x00000200, ///< UC more
	DPM_ST_AP_RESET                 = 0x00000400,
	DPM_ST_DEAUTH                   = 0x00000800,
	DPM_ST_DETECTED_STA             = 0x00001000,
	/// DPM_ST_FROM_FULL is the state when Wake-up from PTIM to FULL-BOOT or
	/// RTOS changes to Sleep Mode.
	DPM_ST_FROM_FULL                = 0x00002000,
	DPM_ST_NORMAL                   = 0x00004000,
	DPM_ST_WEAK_SIGNAL_0            = 0x00008000,
	DPM_ST_WEAK_SIGNAL_1            = 0x00010000,
	DPM_ST_WEAK_SIGNAL_0_1          = 0x00018000,
	DPM_ST_TIMP_UPDATE_0            = 0x00020000,
	DPM_ST_TIMP_UPDATE_1            = 0x00040000,
	DPM_ST_TIMP_UPDATE_0_1          = 0x00060000,

	//DPM_ST_FULLBOOT_EXT             = 0x00080000,
	DPM_ST_FULLBOOT_0               = 0x00400000,
	DPM_ST_FULLBOOT_1               = 0x00800000,

	// Error status from below
	// No error status
	// DPM_STE_NO_ERROR                = 0x00000000,

	/// During PTIM boot, system fault was verified.
	/// The previous boot image was an abnormal shutdown and booting
	/// with PTIM.
	DPM_STE_FAULT_DETECTED          = 0x01000000,
	/// Image header of RamLIB is abnormal
	DPM_STE_INVALID_RAMLIB          = 0x02000000,
	DPM_STE_INVALID_PREAMBLE        = 0x03000000,
	/// During PTIM boot, WDOG was verified.
	/// The WDOG may be caused by a PTIM or other boot image.
	DPM_STE_WDOG_DETECTED           = 0x04000000,
	DPM_STE_INVALID_MY_MAC          = 0x05000000,
	DPM_STE_INVALID_BSSID           = 0x06000000,
	/// RTC_WDOG was caused by RTC HW
	DPM_STE_RTC_WDOG_DETECTED       = 0x07000000,
	DPM_STE_PTIM_TIMEOUT            = 0x08000000,
	DPM_STE_DPMSCHE                 = 0x09000000
};

/* MAC unrelated schedule */
//#define DPM_SCHE_FULLBOOT       0x00100000 /* DPMSCHE_FULL_BOOT */
//#define DPM_SCHE_RESERVED_0     0x00200000 /* DPMSCHE_RESERVED_0 */
//#define DPM_SCHE_RESERVED_1     0x00400000 /* DPMSCHE_RESERVED_1 */
//#define DPM_SCHE_RESERVED_2     0x00800000 /* DPMSCHE_RESERVED_2 */

enum DPM_FEATURE {
	DPM_FEATURE_DONT_USE_MACF      = 0x00000001,
	DPM_FEATURE_DONT_USE_SSIDF     = 0x00000002,
	DPM_FEATURE_ELE_ALL            = 0x00000004,
	/*DPM_FEATURE_CHK_BSS_LOAD       = 0x00000001,
	DPM_FEATURE_CHK_AP_RESET       = 0x00000002,
	DPM_FEATURE_AP_RESET           = 0x00000004,
	DPM_FEATURE_DETECTED_STA       = 0x00000008*/
	DPM_FEATURE_ALL                = 0xffffffff
};

///< DBG for testing purposes. It is not normally operated.
#define DPM_DBG_LO_PRIORITY_MASK   0x00ffffff

///< The DPM_DBG_HI_PRIORITY_MASK may be set because of an unimplemented function rather than for debugging purposes.
#define DPM_DBG_HI_PRIORITY_MASK   0xff000000

enum DPM_DBG_FEATURE {
	DPM_DBG_NORMAL_BOOT        = 0x00000001, ///< NO MAC INIT
	DPM_DBG_FREE_RUN           = 0x00000002,
	DPM_DBG_FULL_BOOT          = 0x00000004,
	DPM_DBG_UC                 = 0x00000008,
	DPM_DBG_BCMC               = 0x00000010,
	DPM_DBG_BCN_CHANGED        = 0x00000020,
	DPM_DBG_NO_BCN             = 0x00000040,
	DPM_DBG_NO_ACK             = 0x00000080,
	DPM_DBG_UC_MORE            = 0x00000100,
	DPM_DBG_AP_RESET           = 0x00000200,
	DPM_DBG_DEAUTH             = 0x00000400,
	DPM_DBG_DETECTED_STA       = 0x00000800,
	DPM_DBG_WEAK_SIGNAL_0_1    = 0x00001000,
	DPM_DBG_TIMP_UPDATE_0_1    = 0x00002000,
	DPM_DBG_FORBIDDEN_UC       = 0x00004000,
	DPM_DBG_FORBIDDEN_BCMC     = 0x00008000,
	DPM_DBG_FORBIDDEN_CRC      = 0x00010000,
	DPM_DBG_FORBIDDEN_NO_BCN   = 0x00020000,
	DPM_DBG_FORBIDDEN_AP_SYNC  = 0x00040000,
	DPM_DBG_FORBIDDEN_NO_ACK   = 0x00080000,
	DPM_DBG_FORBIDDEN_TIMP     = 0x00100000,
	DPM_DBG_FORBIDDEN_DEAUTH   = 0x00200000,
	DPM_DBG_CONSOLE_TIM_RX_RDY = 0x00400000,
	/*DPM_DBG_TIM_TEST_MODE      = 0x00200000,*/ /* Deprecated */
	/// PTIM always wake-up with pm.
	DPM_DBG_PTIM_PM            = 0x08000000,
	/// PTIM Timeout
	DPM_DBG_PTIM_TIMEOUT       = 0x10000000,
	/// In the first PTIM since BCN DM boot, RX has no timeout.
	DPM_DBG_BCN_ACQUISITION    = 0x20000000,
	/// Always wait for beacon.
	DPM_DBG_WAIT_BCN           = 0x40000000,
	/// Measure PREP-TIME only when it is a TIM.
	/// RTOS may not be able to measure the exact Wake-up time of the TIM.
	DPM_DBG_PREPTIME_TIM_ONLY  = 0x80000000,
	DPM_DBG_ALL                = 0xffffffff
};

/*enum DPM_TX_FEATURE {
	DPM_TX_KA                  = 0x00000001,
	DPM_TX_ARP_RESP            = 0x00000002,
	DPM_TX_AUTO_ARP            = 0x00000004,
	DPM_TX_PS_POLL             = 0x00000008,
	DPM_TX_UDP_HOLE_PUNCH      = 0x00000010,
	DPM_TX_CHK_DEAUTH          = 0x00000020
};*/

/******************************************************************************
 * PHY Configuration for Boot
 ******************************************************************************/
enum PHY_BOOT {
	PHY_BOOT_POR   = 0x00,
	PHY_BOOT_FULL  = 0x01,
	PHY_BOOT_MTIM  = 0x02,
	PHY_BOOT_PTIM  = 0x03,
	PHY_BOOT_MKEEP = 0x04,
	PHY_BOOT_PKEEP = 0x05,

	PHY_POST_POR   = 0x10,
	PHY_POST_FULL  = 0x11,
	PHY_POST_MTIM  = 0x12,
	PHY_POST_PTIM  = 0x13,
	PHY_POST_MKEEP = 0x14,
	PHY_POST_PKEEP = 0x15
};

enum DPM_FILTER {
	DPM_F_DROP_MY_DATA        = 0x00000001,
	DPM_F_BC                  = 0x00000002,
	DPM_F_MC                  = 0x00000004,
	DPM_F_MATCHED_MC          = 0x00000008,
	DPM_F_UC                  = 0x00000010,
	DPM_F_CTRL                = 0x00000020,
	DPM_F_MGMT                = 0x00000040,
	DPM_F_DEAUTH              = 0x00000080,
	DPM_F_DROP_ACTION         = 0x00000100,
	/*DPM_F_DROP_ACTION_NO_ACK  = 0x00000200,*/
	DPM_F_DATA                = 0x00000400,
	DPM_F_DROP_NULL           = 0x00000800,
	DPM_F_ARP                 = 0x00001000,
	DPM_F_DROP_IPv6           = 0x00002000,
	DPM_F_IP                  = 0x00004000,

	DPM_F_DROP_OTHER_IP       = 0x00008000,
	DPM_F_DROP_BC_IP          = 0x00010000,
	DPM_F_DROP_MC_IP          = 0x00020000,
	DPM_F_MATCHED_MC_IP       = 0x00040000,
	DPM_F_UC_IP               = 0x00080000,
	DPM_F_DROP_UDP_SERVICE    = 0x00100000,
	DPM_F_MATCHED_UDP_SERVICE = 0x00200000,
	DPM_F_DROP_TCP_SERVICE    = 0x00400000,
	DPM_F_MATCHED_TCP_SERVICE = 0x00800000
};

enum DPM_DATA_TYPE {
	DPM_DT_UNDEFINED           = 0x00000000,
	DPM_DT_DROP_MY_DATA        = 0x00000001,
	DPM_DT_BC                  = 0x00000002,
	DPM_DT_DROP_BC             = 0x00000004,
	DPM_DT_MC                  = 0x00000008,
	DPM_DT_DROP_MC             = 0x00000010,
	DPM_DT_UC                  = 0x00000020,
	DPM_DT_DROP_UC             = 0x00000040,
	DPM_DT_CTRL                = 0x00000080,
	DPM_DT_DROP_CTRL           = 0x00000100,
	DPM_DT_MGMT                = 0x00000200,
	DPM_DT_DROP_MGMT           = 0x00000400,
	DPM_DT_DEAUTH              = 0x00000800,
	DPM_DT_DROP_ACTION         = 0x00001000,
	/*DPM_DT_DROP_ACTION_NO_ACK  = 0x00002000,*/
	DPM_DT_DATA                = 0x00002000,
	DPM_DT_DROP_DATA           = 0x00004000,
	DPM_DT_DROP_NULL           = 0x00008000,
	DPM_DT_ARP                 = 0x00010000,
	//DPM_DT_BC_ARP              = 0x00080000,
	DPM_DT_DROP_OTHER_ARP      = 0x00020000,
	//DPM_DT_DROP_OTHER_BC_ARP   = 0x00200000,
	DPM_DT_ARP_RESP            = 0x00040000,
	DPM_DT_DROP_IPv6           = 0x00080000,
	DPM_DT_IP                  = 0x00100000,
	DPM_DT_DROP_IP             = 0x00200000,

	DPM_DT_DROP_OTHER_IP       = 0x00400000,
	DPM_DT_DROP_BC_IP          = 0x00800000,
	DPM_DT_MC_IP               = 0x01000000,
	DPM_DT_DROP_MC_IP          = 0x02000000,
	DPM_DT_UC_IP               = 0x04000000,
	DPM_DT_DROP_UDP_SERVICE    = 0x08000000,
	DPM_DT_MATCHED_UDP_SERVICE = 0x10000000,
	DPM_DT_DROP_TCP_SERVICE    = 0x20000000,
	DPM_DT_MATCHED_TCP_SERVICE = 0x40000000,

	DPM_DT_DATA_EAPOL          = 0x80000000
};

enum DPM_RXDATA_TY {
	DPM_RTY_INVALID,
	DPM_RTY_BCMC,
	DPM_RTY_UC
};

enum DPM_TRG_TY {
	DPM_TRG_START = 0x10000000,
	DPM_TRG_END   = 0xffffffff
};

enum DPM_TRG_PTIM {
	DPM_TRG_ARCHMAIN_END       = 0x70000000,
	DPM_TRG_TIM_STOP           = 0x80000000,
	DPM_TRG_PREPARE_POWER_DOWN = 0x90000000,
	DPM_TRG_SCHE_DO            = 0xa0000000,
	DPM_TRG_TIM_DOZE           = 0xb0000000,
	DPM_TRG_TIM_LOG            = 0xc0000000,
	DPM_TRG_TIM_POWERDOWN      = 0xd0000000,
	DPM_TRG_TIM_FULLBOOT       = 0xe0000000,
};

/*#define DPM_DT_UNDEFINED_BIT            (0)
#define DPM_DT_MC_BIT                   CO_BIT(31 - DPM_DT_MC)
#define DPM_DT_BC_BIT                   CO_BIT(31 - DPM_DT_BC)
#define DPM_DT_ARP_BIT                  CO_BIT(31 - DPM_DT_ARP)
#define DPM_DT_MGMT_BIT                 CO_BIT(31 - DPM_DT_MGMT)
#define DPM_DT_DEAUTH_BIT               CO_BIT(31 - DPM_DT_DEAUTH)
#define DPM_DT_DROP_MY_DATA_BIT         CO_BIT(31 - DPM_DT_DROP_MY_DATA)
#define DPM_DT_UC_BIT                   CO_BIT(31 - DPM_DT_UC)
#define DPM_DT_UC_MORE_BIT              CO_BIT(31 - DPM_DT_UC_MORE)
#define DPM_DT_DROP_NULL_BIT            CO_BIT(31 - DPM_DT_DROP_NULL)
#define DPM_DT_DROP_ACTION_BIT          CO_BIT(31 - DPM_DT_DROP_ACTION)
#define DPM_DT_UC_ARP_BIT               CO_BIT(31 - DPM_DT_UC_ARP)
#define DPM_DT_UC_ARP_RESP_BIT          CO_BIT(31 - DPM_DT_UC_ARP_RESP)
#define DPM_DT_DROP_IPv6_BIT            CO_BIT(31 - DPM_DT_DROP_IPv6)
#define DPM_DT_DROP_MC_IP_BIT           CO_BIT(31 - DPM_DT_DROP_MC_IP)
#define DPM_DT_MATCHED_MC_IP_BIT        CO_BIT(31 - DPM_DT_MATCHED_MC_IP)
#define DPM_DT_DROP_BC_IP_BIT           CO_BIT(31 - DPM_DT_DROP_BC_IP)
#define DPM_DT_DROP_OTHER_IP_BIT        CO_BIT(31 - DPM_DT_DROP_OTHER_IP)
#define DPM_DT_DROP_UDP_SERVICE_BIT     CO_BIT(31 - DPM_DT_DROP_UDP_SERVICE)
#define DPM_DT_MATCHED_UDP_SERVICE_BIT  CO_BIT(31 - DPM_DT_MATCHED_UDP_SERVICE)
#define DPM_DT_DECR_ERR_BIT             CO_BIT(31 - DPM_DT_DECR_ERR)*/

enum DPM_PERIOD_TY
{
	DPM_PERIOD_BI          = 0x00, ///< BCN INTERVAL
	DPM_PERIOD_DTIM        = 0x01, ///< DTIM PERIOD
	DPM_PERIOD_LI          = 0x02, ///< LISTEN INTERVAL
	DPM_PERIOD_FORCE       = 0x03  ///< ARBITRARY PERIOD, but multiplied by DTIM_PERIOD
};

/**
 * @enum  DPM_ARP_TY
 * @brief ARP type
 */
enum DPM_ARP_TY {
	DPM_ARP_TY_AUTO_RESP,       ///< Auto ARP RESP Type for sending to AP
	DPM_ARP_TY_AUTO_TARGET_MAC, ///< Auto GARP Type
	DPM_ARP_TY_RESP,            ///< ARP RESP type
	DPM_ARP_TY_REQ              ///< ARP Request type
};

/**
 * @enum  DPM_ARP_TY
 * @brief ARP type
 */
enum DPM_PS_TY {
	DPM_PS_TY_PM1  = 0x00,
	DPM_PS_TY_PM0  = 0x01,
	DPM_PS_TY_NULL = 0x02,
	DPM_PS_TY_MAX  = 0x03
};

enum DPM_DEAUTH_CHK_TY {
	DPM_DEAUTH_CHK_NULL,
	DPM_DEAUTH_CHK_ARP
};

enum DPM_UDPH_TY {
	DPM_UDPH_PATTERN_00 = 0x00,
	DPM_UDPH_USER_DATA  = 0x01, ///< User data
	DPM_UDPH_PATTERN_FF = 0xff,
	DPM_UDPH_PATTERN_AA = 0xaa
};


/**
 * @enum  DPM_SCHE_FEATURE
 * @brief SCHE common feature
 */
enum DPM_SCHE_FEATURE {
	DPMSCHE_F_EN           = 0x01,
	DPMSCHE_F_FIX          = 0x02
};

enum DPM_RX_PENDING {
	DPM_RX_PENDING_BCN,
	DPM_RX_PENDING_BCMC,
	DPM_RX_PENDING_UC,
	DPM_RX_PENDING_DEAUTH
};

enum DPM_TX_PENDING {
	DPM_TX_PENDING_PS,
	DPM_TX_PENDING_KA,
	DPM_TX_PENDING_AUTO_ARP,
	DPM_TX_PENDING_ARPREQ,
	DPM_TX_PENDING_ARPRESP,
	DPM_TX_PENDING_UDPH,
	DPM_TX_PENDING_TCP_KA,
	DPM_TX_PENDING_SETPS
};

/// @} end of group ROMAC_COM_TY

#endif //__DPMTY_H__
