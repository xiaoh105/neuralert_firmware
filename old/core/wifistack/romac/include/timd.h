/**
 ****************************************************************************************
 *
 * @file     timd.h
 *
 * @brief    TIM debug configuration header
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

#ifndef __TIMD_H__
#define __TIMD_H__

/**
 * @file     timd.h
 * @brief    TIM debug configuration
 * @details  This file defines the TIM debug configuration.
 *           This header file can be used in RTOS & PTIM,
 *           but is usually used only in TIM.
 * @ingroup  DPMCOMMON
 * @{
 */
#include "dpmty.h"

/* lltoa max string length */
#define TIMD_LLTOA_MAX 66

#define TIMD_PRINT_EN
//#undef TIMD_LOG_EN ///< Deprecated
#undef TIMD_DATA_EN
#undef TIMD_INFO_EN
#undef TIMD_WARNING_EN
#undef TIMD_ERROR_EN
#undef TIMD_C_EN
#undef TIMD_PREP_EN
#undef TIMD_SCHE_EN
#undef TIMD_P_EN
#undef TIMD_APTRK_EN
#undef TIMD_PBR_EN
#undef TIMD_T_EN
#undef TIMD_KA_EN
#undef TIMD_AUTO_ARP_EN    ///< Auto ARP
#undef TIMD_UDPH_EN  ///< UDP Hole-punch
#undef TIMD_TIME_EN
#undef TIMD_RX_EN
#undef TIMD_BCN_EN
#undef TIMD_BCMC_EN
#undef TIMD_PS_EN
#undef TIMD_UC_EN
#undef TIMD_TX_EN
#undef TIMD_ARPREQ_EN
#undef TIMD_DEAUTH_EN


/*#define TIMD_PRINTF(timd, n, ...) \
		if (((timd) & (n)) && ((timd) & (TIMD_PRINT))) \
				embromac_print(0, __VA_ARGS__ )
				//TIM_PRINT(0, __VA_ARGS__)*/

#ifdef TIMD_PRINT_EN
	#ifdef BUILD_OPT_DA16200_ROMAC
	#define TIMD_PRINTF(timd, n, ...) \
			if (((timd) & (n)) && ((timd) & (TIMD_PRINT))) \
					embromac_print(0, __VA_ARGS__)
	#else
	#define TIMD_PRINTF(timd, n, ...) \
			if (((timd) & (n)) && ((timd) & (TIMD_PRINT))) \
					PRINTF(__VA_ARGS__)
	#endif
#else
	#define TIMD_PRINTF(timd, n, ...)
#endif

enum TIMD_FEATURE {
	TIMD_NONE       = 0x00000000,
	TIMD_PRINT      = 0x00000001,
	TIMD_LOG        = 0x00000002, ///< PTIM LOG
	TIMD_INFO       = 0x00000004,
	TIMD_WARNING    = 0x00000008,
	TIMD_ERROR      = 0x00000010,
	TIMD_DATA       = 0x00000020,

	TIMD_PREP       = 0x00000040, ///< PREP
	TIMD_SCHE       = 0x00000080, ///< Scheduler
	TIMD_P          = 0x00000100, ///< TIMP
	TIMD_APTRK      = 0x00000200, ///< AP Tracking
	TIMD_PBR        = 0x00000400, ///< PBR
	TIMD_T          = 0x00000800, ///< TIM
	TIMD_KA         = 0x00001000, ///< KA
	TIMD_AUTO_ARP   = 0x00002000, ///< Auto ARP
	TIMD_ARPREQ     = 0x00004000, ///< ARP Request
	TIMD_DEAUTH     = 0x00008000, ///< Deauth chk
	TIMD_UDPH       = 0x00010000, ///< UDP Hole-punch
	TIMD_TIME       = 0x00020000, ///< Time Measure
	TIMD_RX         = 0x00040000,
	TIMD_BCN        = 0x00080000,
	TIMD_BCMC       = 0x00100000,
	TIMD_PS         = 0x00200000,
	TIMD_UC         = 0x00400000,
	TIMD_TX         = 0x00800000,
	/*TIMD_CONF       = 0x00000010,
	TIMD_MAC        = 0x00000100,
	TIMD_TX_DATA    = 0x00000400*/
	TIMD_MAX        = 0x80000000
};

enum TIMD_STR_N {
	TIMD_STR_Y_K_N,          /* KEY */
	TIMD_STR_Y_K_S,          /* KEY & String */
	TIMD_STR_Y_N,            /* Integer */
	TIMD_STR_Y_S,            /* String */
	TIMD_STR_Y_H,            /* Hex */
	TIMD_STR_Y_M,            /* MAC Address */
#ifdef TIMD_I_EN
//	TIMD_STR_I_RTC_OVERFLOW, /* RTC Overflow */
//	TIMD_STR_I_WAKEUP_OVERFLOW, /* Wakeup Overflow */
#endif
#ifdef TIMD_W_EN
//	TIMD_STR_W_PREP_MEAS_E,  /* Preparation time measurement error */
	//TIMD_STR_W_OUT_OF_STATUS_BUF, /* Out of status buffers */
//	TIMD_STR_W_FOUND_SAME_TIMER, /* Found same timer */
	//TIMD_STR_W_INVALID_NEXT_SCHE_BUF, /* Invalid next schedule buffer */
//	TIMD_STR_W_DTIM_CNT_NOT_ZERO, /* Received dtim_cnt for beacon not zero */
#endif
#ifdef TIMD_E_EN
	//TIMD_STR_E_STATUS_BUF,    ///< Invalid Status Buffer
//	TIMD_STR_E_OUT_OF_SCHE_BUF, ///< Out of schedule buffers
//	TIMD_STR_E_NO_SCHE,         ///< No schedule
//	TIMD_STR_E_PBD_NULL,        ///< PBD null pointer
//	TIMD_STR_E_EMPTY_SCHE,      ///< There are no schedule.
#endif
//#ifdef TIMD_D_EN
	TIMD_STR_D_DATA_FIELD,      ///< Data field name
	TIMD_STR_D_DATA_FORMAT,     ///< Data field format
//#endif
#ifdef TIMD_SCHE_EN
//	TIMD_STR_SCHE_PREP_TIME,
//	TIMD_STR_SCHE_POST_PREP_TIME,
#endif
#ifdef TIMD_APTRK_EN
//	TIMD_STR_APTRK_BCN_CLK,  /* RTC time for Beacon received */
//	TIMD_STR_APTRK_TIMESTAMP,/* Timestamp of Beacon */
//	TIMD_STR_APTRK_CCA,      /* CCA time while receiving Beacon */
//	TIMD_STR_APTRK_STATUS,   /* The status of APTRK */
//	TIMD_STR_APTRK_STATUS_FORMAT, /* The status of APTRK */
#endif
	TIMD_STR_MAX
};

#define TIMD_K_N(timd, s, n) \
	timd_y_k_n(timd, TIMD_PRINT, s, n)
#define TIMD_K_S(timd, s, s1) \
	timd_y_k_s(timd, TIMD_PRINT, s, s1)

/* TIM LOG */
#define TIMD_L_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_LOG, s, n)
#define TIMD_L_N(timd, s, n) \
	timd_y_n(timd, TIMD_LOG, s, n)
#define TIMD_L_X(timd, s, n) \
	timd_y_x(timd, TIMD_CONF, s, n)
#define TIMD_L_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_LOG, s, n64)
#define TIMD_L_C(timd, s, c) \
	timd_y_c(timd, TIMD_LOG, s, c)
#define TIMD_L_S(timd, s, s1) \
	timd_y_s(timd, TIMD_LOG, s, s1)

/* TIM INFO */
#ifdef TIMD_INFO_EN
#define TIMD_INFO_S(timd, s, s1) \
	timd_y_s(timd, TIMD_INFO, s, s1)
#define TIMD_INFO(timd, s) \
	timd_y(timd, TIMD_INFO, s)
#define TIMD_INFO_N(timd, s, n) \
	timd_y_n(timd, TIMD_INFO, s, n)
#define TIMD_INFO_X(timd, s, n) \
	timd_y_x(timd, TIMD_INFO, s, n)
#define TIMD_INFO_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_INFO, s, n64)
#define TIMD_INFO_X64(timd, s, x64) \
	timd_y_x64(timd, TIMD_INFO, s, x64)
#else
#define TIMD_INFO_S(timd, s, s1)
#define TIMD_INFO(timd, s)
#define TIMD_INFO_N(timd, s, n)
#define TIMD_INFO_X(timd, s, n)
#define TIMD_INFO_N64(timd, s, n64)
#define TIMD_INFO_X64(timd, s, x64)
#endif

/* TIM WARNING */
#ifdef TIMD_WARNING_EN
#define TIMD_W_S(timd, s, s1) \
	timd_y_s(timd, TIMD_WARNING, s, s1)
#define TIMD_W(timd, s) \
	timd_y(timd, TIMD_WARNING, s)
#define TIMD_W_N(timd, s, n) \
	timd_y_n(timd, TIMD_WARNING, s, n)
#define TIMD_W_X(timd, s, n) \
	timd_y_x(timd, TIMD_WARNING, s, n)
#else
#define TIMD_W_S(timd, s, s1)
#define TIMD_W(timd, s)
#define TIMD_W_N(timd, s, n)
#define TIMD_W_X(timd, s, n)
#endif

/* TIM ERROR */
#ifdef TIMD_ERROR_EN
#define TIMD_E_S(timd, s, s1) \
	timd_y_s(timd, TIMD_ERROR, s, s1)
#define TIMD_E(timd, s) \
	timd_y(timd, TIMD_ERROR, s)
#define TIMD_E_N(timd, s, n) \
	timd_y_n(timd, TIMD_ERROR, s, n)
#define TIMD_E_X(timd, s, n) \
	timd_y_x(timd, TIMD_ERROR, s, n)
#else
#define TIMD_E_S(timd, s, s1)
#define TIMD_E(timd, s)
#define TIMD_E_N(timd, s, n)
#define TIMD_E_X(timd, s, n)
#endif

/* TIM CONF */
#ifdef TIMD_C_EN
#define TIMD_C_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_CONF, s, n)
#define TIMD_C_N(timd, s, n) \
	timd_y_n(timd, TIMD_CONF, s, n)
#define TIMD_C_X(timd, s, n) \
	timd_y_x(timd, TIMD_CONF, s, n)
#define TIMD_C_M(timd, s, m) \
	timd_y_m(timd, TIMD_CONF, s, m)
#define TIMD_C_S(timd, s, s1) \
	timd_y_s(timd, TIMD_CONF, s, s1)
#else
#define TIMD_C_K(timd, s, n)
#define TIMD_C_N(timd, s, n)
#define TIMD_C_X(timd, s, n)
#define TIMD_C_M(timd, s, m)
#define TIMD_C_S(timd, s, s1)
#endif

#ifdef TIMD_DATA_EN
/**
 * @brief           TIM data dump
 * @param[in] timd  TIMD feature
 * @param[in] time  Data receive time (RTC CLK)
 * @param[in] hd Pointer to data
 */
#define TIMD_D_D(timd, time, hd) \
	timd_y_d(timd, TIMD_DATA, time, hd)
#else
#define TIMD_D_D(timd, time, hd)
#endif

/* Preparation */
#ifdef TIMD_PREP_EN
#define TIMD_PREP_N(timd, s, n) \
	timd_y_n(timd, TIMD_PREP, s, n)
#define TIMD_PREP_X(timd, s, n) \
	timd_y_x(timd, TIMD_PREP, s, n)
#define TIMD_PREP_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_PREP, s, n64)
#define TIMD_PREP_X64(timd, s, x64) \
	timd_y_x64(timd, TIMD_PREP, s, x64)
#else
#define TIMD_PREP_N(timd, s, n)
#define TIMD_PREP_X(timd, s, n)
#define TIMD_PREP_N64(timd, s, n64)
#define TIMD_PREP_X64(timd, s, x64)
#endif

/* DPM Scheduler */
#ifdef TIMD_SCHE_EN
#define TIMD_SCHE_N(timd, s, n) \
	timd_y_n(timd, TIMD_SCHE, s, n)
#define TIMD_SCHE_X(timd, s, n) \
	timd_y_x(timd, TIMD_SCHE, s, n)
#define TIMD_SCHE_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_SCHE, s, n64)
#define TIMD_SCHE_X64(timd, s, x64) \
	timd_y_x64(timd, TIMD_SCHE, s, x64)
#else
#define TIMD_SCHE_N(timd, s, n)
#define TIMD_SCHE_X(timd, s, n)
#define TIMD_SCHE_N64(timd, s, n64)
#define TIMD_SCHE_X64(timd, s, x64)
#endif

/* TIM Performance LOG */
#ifdef TIMD_P_EN_
#define TIMD_P_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_TIMP, s, n)
#define TIMD_P_N(timd, s, n) \
	timd_y_n(timd, TIMD_TIMP, s, n)
#define TIMD_P_S(timd, s, s1) \
	timd_y_s(timd, TIMD_TIMP, s, s1)
#else
#define TIMD_P_K(timd, s, n)
#define TIMD_P_N(timd, s, n)
#define TIMD_P_S(timd, s, s1)
#endif

/* DPM AP Tracking */
#ifdef TIMD_APTRK_EN
#define TIMD_APTRK_N(timd, s, n) \
	timd_y_n(timd, TIMD_APTRK, s, n)
#define TIMD_APTRK_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_APTRK, s, n64)
#define TIMD_APTRK_S(timd, s, s1) \
	timd_y_s(timd, TIMD_APTRK, s, s1)
#else
#define TIMD_APTRK_N(timd, s, n)
#define TIMD_APTRK_N64(timd, s, n64)
#define TIMD_APTRK_S(timd, s, s1)
#endif

/* PBR LOG */
#ifdef TIMD_PBR_EN
#define TIMD_PBR_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_PBR, s, n)
#define TIMD_PBR_N(timd, s, n) \
	timd_y_n(timd, TIMD_PBR, s, n)
#define TIMD_PBR_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_PBR, s, n64)
#define TIMD_PBR_S(timd, s, s1) \
	timd_y_s(timd, TIMD_PBR, s, s1)
#else
#define TIMD_PBR_K(timd, s, n)
#define TIMD_PBR_N(timd, s, n)
#define TIMD_PBR_N64(timd, s, n64)
#define TIMD_PBR_S(timd, s, s1)
#endif

/* TIM LOG */
#ifdef TIMD_T_EN
#define TIMD_T_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_T, s, n)
#define TIMD_T_N(timd, s, n) \
	timd_y_n(timd, TIMD_T, s, n)
#define TIMD_T_X(timd, s, n) \
	timd_y_x(timd, TIMD_T, s, n)
#define TIMD_T_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_T, s, n64)
#define TIMD_T_S(timd, s, s1) \
	timd_y_s(timd, TIMD_T, s, s1)
#else
#define TIMD_T_K(timd, s, n)
#define TIMD_T_N(timd, s, n)
#define TIMD_T_X(timd, s, n)
#define TIMD_T_N64(timd, s, n64)
#define TIMD_T_S(timd, s, s1)
#endif

/* KA LOG */
#ifdef TIMD_KA_EN
#define TIMD_KA_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_KA, s, n)
#define TIMD_KA_N(timd, s, n) \
	timd_y_n(timd, TIMD_KA, s, n)
#define TIMD_KA_X(timd, s, n) \
	timd_y_x(timd, TIMD_KA, s, n)
#define TIMD_KA_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_KA, s, n64)
#define TIMD_KA_S(timd, s, s1) \
	timd_y_s(timd, TIMD_KA, s, s1)
#else
#define TIMD_KA_K(timd, s, n)
#define TIMD_KA_N(timd, s, n)
#define TIMD_KA_X(timd, s, n)
#define TIMD_KA_N64(timd, s, n64)
#define TIMD_KA_S(timd, s, s1)
#endif

/* Auto ARP LOG */
#ifdef TIMD_AUTO_ARP_EN
#define TIMD_AUTO_ARP_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_AUTO_ARP, s, n)
#define TIMD_AUTO_ARP_N(timd, s, n) \
	timd_y_n(timd, TIMD_AUTO_ARP, s, n)
#define TIMD_AUTO_ARP_X(timd, s, n) \
	timd_y_x(timd, TIMD_AUTO_ARP, s, n)
#define TIMD_AUTO_ARP_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_AUTO_ARP, s, n64)
#define TIMD_AUTO_ARP_S(timd, s, s1) \
	timd_y_s(timd, TIMD_AUTO_ARP, s, s1)
#else
#define TIMD_AUTO_ARP_K(timd, s, n)
#define TIMD_AUTO_ARP_N(timd, s, n)
#define TIMD_AUTO_ARP_X(timd, s, n)
#define TIMD_AUTO_ARP_N64(timd, s, n64)
#define TIMD_AUTO_ARP_S(timd, s, s1)
#endif

/* UDP Hole-punch LOG */
#ifdef TIMD_UDPH_EN
#define TIMD_UDPH_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_UDPH, s, n)
#define TIMD_UDPH_N(timd, s, n) \
	timd_y_n(timd, TIMD_UDPH, s, n)
#define TIMD_UDPH_X(timd, s, n) \
	timd_y_x(timd, TIMD_UDPH, s, n)
#define TIMD_UDPH_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_UDPH, s, n64)
#define TIMD_UDPH_S(timd, s, s1) \
	timd_y_s(timd, TIMD_UDPH, s, s1)
#else
#define TIMD_UDPH_K(timd, s, n)
#define TIMD_UDPH_N(timd, s, n)
#define TIMD_UDPH_X(timd, s, n)
#define TIMD_UDPH_N64(timd, s, n64)
#define TIMD_UDPH_S(timd, s, s1)
#endif

/* Time LOG */
#ifdef TIMD_TIME_EN
#define TIMD_TIME_N(timd, s, n) \
	timd_y_n(timd, TIMD_KA, s, n)
#define TIMD_TIME_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_KA, s, n64)
#else
#define TIMD_TIME_N(timd, s, n)
#define TIMD_TIME_N64(timd, s, n64)
#endif

/* RX LOG */
#ifdef TIMD_RX_EN
#define TIMD_RX_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_RX, s, n)
#define TIMD_RX_N(timd, s, n) \
	timd_y_n(timd, TIMD_RX, s, n)
#define TIMD_RX_X(timd, s, n) \
	timd_y_x(timd, TIMD_RX, s, n)
#define TIMD_RX_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_RX, s, n64)
#define TIMD_RX_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_RX, s, n64)
#else
#define TIMD_RX_K(timd, s, n)
#define TIMD_RX_N(timd, s, n)
#define TIMD_RX_X(timd, s, n)
#define TIMD_RX_N64(timd, s, n64)
#define TIMD_RX_X64(timd, s, n64)
#endif

/* BCN LOG */
#ifdef TIMD_BCN_EN
#define TIMD_BCN_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_BCN, s, n)
#define TIMD_BCN_N(timd, s, n) \
	timd_y_n(timd, TIMD_BCN, s, n)
#define TIMD_BCN_X(timd, s, n) \
	timd_y_x(timd, TIMD_BCN, s, n)
#define TIMD_BCN_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_BCN, s, n64)
#define TIMD_BCN_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_BCN, s, n64)
#else
#define TIMD_BCN_K(timd, s, n)
#define TIMD_BCN_N(timd, s, n)
#define TIMD_BCN_X(timd, s, n)
#define TIMD_BCN_N64(timd, s, n64)
#define TIMD_BCN_X64(timd, s, n64)
#endif

/* BCMC LOG */
#ifdef TIMD_BCMC_EN
#define TIMD_BCMC_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_BCMC, s, n)
#define TIMD_BCMC_N(timd, s, n) \
	timd_y_n(timd, TIMD_BCMC, s, n)
#define TIMD_BCMC_X(timd, s, n) \
	timd_y_x(timd, TIMD_BCMC, s, n)
#define TIMD_BCMC_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_BCMC, s, n64)
#define TIMD_BCMC_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_BCMC, s, n64)
#else
#define TIMD_BCMC_K(timd, s, n)
#define TIMD_BCMC_N(timd, s, n)
#define TIMD_BCMC_X(timd, s, n)
#define TIMD_BCMC_N64(timd, s, n64)
#define TIMD_BCMC_X64(timd, s, n64)
#endif

/* PS LOG */
#ifdef TIMD_PS_EN
#define TIMD_PS_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_PS, s, n)
#define TIMD_PS_N(timd, s, n) \
	timd_y_n(timd, TIMD_PS, s, n)
#define TIMD_PS_X(timd, s, n) \
	timd_y_x(timd, TIMD_PS, s, n)
#define TIMD_PS_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_PS, s, n64)
#define TIMD_PS_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_PS, s, n64)
#else
#define TIMD_PS_K(timd, s, n)
#define TIMD_PS_N(timd, s, n)
#define TIMD_PS_X(timd, s, n)
#define TIMD_PS_N64(timd, s, n64)
#define TIMD_PS_X64(timd, s, n64)
#endif

/* UC LOG */
#ifdef TIMD_UC_EN
#define TIMD_UC_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_UC, s, n)
#define TIMD_UC_N(timd, s, n) \
	timd_y_n(timd, TIMD_UC, s, n)
#define TIMD_UC_X(timd, s, n) \
	timd_y_x(timd, TIMD_UC, s, n)
#define TIMD_UC_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_UC, s, n64)
#define TIMD_UC_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_UC, s, n64)
#else
#define TIMD_UC_K(timd, s, n)
#define TIMD_UC_N(timd, s, n)
#define TIMD_UC_X(timd, s, n)
#define TIMD_UC_N64(timd, s, n64)
#define TIMD_UC_X64(timd, s, n64)
#endif

/* TX LOG */
#ifdef TIMD_TX_EN
#define TIMD_TX_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_TX, s, n)
#define TIMD_TX_N(timd, s, n) \
	timd_y_n(timd, TIMD_TX, s, n)
#define TIMD_TX_X(timd, s, n) \
	timd_y_x(timd, TIMD_TX, s, n)
#define TIMD_TX_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_TX, s, n64)
#define TIMD_TX_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_TX, s, n64)
#else
#define TIMD_TX_K(timd, s, n)
#define TIMD_TX_N(timd, s, n)
#define TIMD_TX_X(timd, s, n)
#define TIMD_TX_N64(timd, s, n64)
#define TIMD_TX_X64(timd, s, n64)
#endif

#ifdef TIMD_ARPREQ_EN
#define TIMD_ARPREQ_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_ARPREQ, s, n)
#define TIMD_ARPREQ_N(timd, s, n) \
	timd_y_n(timd, TIMD_ARPREQ, s, n)
#define TIMD_ARPREQ_X(timd, s, n) \
	timd_y_x(timd, TIMD_ARPREQ, s, n)
#define TIMD_ARPREQ_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_ARPREQ, s, n64)
#define TIMD_ARPREQ_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_ARPREQ, s, n64)
#else
#define TIMD_ARPREQ_K(timd, s, n)
#define TIMD_ARPREQ_N(timd, s, n)
#define TIMD_ARPREQ_X(timd, s, n)
#define TIMD_ARPREQ_N64(timd, s, n64)
#define TIMD_ARPREQ_X64(timd, s, n64)
#endif

#ifdef TIMD_DEAUTH_EN
#define TIMD_DEAUTH_K(timd, s, n) \
	timd_y_k_n(timd, TIMD_DEAUTH, s, n)
#define TIMD_DEAUTH_N(timd, s, n) \
	timd_y_n(timd, TIMD_DEAUTH, s, n)
#define TIMD_DEAUTH_X(timd, s, n) \
	timd_y_x(timd, TIMD_DEAUTH, s, n)
#define TIMD_DEAUTH_N64(timd, s, n64) \
	timd_y_n64(timd, TIMD_DEAUTH, s, n64)
#define TIMD_DEAUTH_X64(timd, s, n64) \
	timd_y_x64(timd, TIMD_DEAUTH, s, n64)
#else
#define TIMD_DEAUTH_K(timd, s, n)
#define TIMD_DEAUTH_N(timd, s, n)
#define TIMD_DEAUTH_X(timd, s, n)
#define TIMD_DEAUTH_N64(timd, s, n64)
#define TIMD_DEAUTH_X64(timd, s, n64)
#endif

/******************************************************************************
 *
 * print utilities for DPM
 *
 *****************************************************************************/
extern char* lltoa(long long val, char *buffer, int base);
extern void timd_y_d(unsigned long timd, enum TIMD_FEATURE f, long long rtclk,
			void *hd);

/******************************************************************************
 *
 * String table configuration
 *
 *****************************************************************************/
extern void timd_init(char *str_table[]);
extern char* timd_gets(enum TIMD_STR_N n);

/*inline char* timd_gets(enum TIMD_STR_N n, char *table[])
{
	if (n < TIMD_STR_MAX && table)
		return table[n];
	return NULL;
}*/

/******************************************************************************
 *
 * YAML functions
 *
 *****************************************************************************/
/* YAML Header */
extern void timd_y_k_n(unsigned long timd, enum TIMD_FEATURE f,
						char *s, long n);
extern void timd_y_k_s(unsigned long timd, enum TIMD_FEATURE f, char *s,
						char *s1);

/* YAML Node */
extern void timd_y(unsigned long timd, enum TIMD_FEATURE f, char *s);
extern void timd_y_n(unsigned long timd, enum TIMD_FEATURE f, char *s, long n);
extern void timd_y_s(unsigned long timd, enum TIMD_FEATURE f, char *s,
						char *s1);
extern void timd_y_n64(unsigned long timd, enum TIMD_FEATURE f, char *s,
						long long n64);
extern void timd_y_x(unsigned long timd, enum TIMD_FEATURE f, char *s,
						unsigned long n);
extern void timd_y_x64(unsigned long timd, enum TIMD_FEATURE f, char *s,
						unsigned long long n64);
extern void timd_y_m(unsigned long timd, enum TIMD_FEATURE f, char *s,
						char *m_a);

/******************************************************************************
 *
 * MAC dbg functions
 *
 *****************************************************************************/
/* MAC data dump */
/*extern void timd_dump_data(unsigned long timd, enum TIMD_FEATURE f,
													void *frame);*/
/// @} end of group DPMCOMMON

#endif /* __TIMD_H__ */
