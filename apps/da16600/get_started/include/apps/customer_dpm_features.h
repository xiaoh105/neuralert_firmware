/**
 ****************************************************************************************
 *
 * @file customer_dpm_features.h
 *
 * @brief Defines and macros for customer's DPM operation
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

#ifndef    __CUSTOMER_DPM_FEATURES_H__
#define    __CUSTOMER_DPM_FEATURES_H__

#pragma GCC diagnostic ignored "-Wunused-function"

#define DA16X_TIM_AP_SYNC

#ifdef GENERIC_SDK
  #define DA16X_DPM_TIM_BCN_CHK_INT           2
  #define DA16X_DPM_TIM_BCN_CHK_STEP          4
  #define DA16X_DPM_BCN_TIMEOUT               10
  #define DA16X_DPM_BCN_TIMEOUT_DTIM          10
  #define DA16X_DPM_TIM_FILTER_TIMEOUT        16000
  #define DA16X_DPM_TIM_UC_MAX                3
  #define DA16X_DPM_TIM_PS_POLL_TIMEOUT       100
  #define DA16X_DPM_TIM_TBTT_DELAY            0
  #define DA16X_DPM_TIM_DEAUTH_MAX            5
#else    /* default */
  #define DA16X_DPM_TIM_BCN_CHK_INT           2
  #define DA16X_DPM_TIM_BCN_CHK_STEP          4
  #define DA16X_DPM_BCN_TIMEOUT               10
  #define DA16X_DPM_BCN_TIMEOUT_DTIM          10
  #define DA16X_DPM_TIM_FILTER_TIMEOUT        16000
  #define DA16X_DPM_TIM_UC_MAX                3
  #define DA16X_DPM_TIM_PS_POLL_TIMEOUT       100
  #define DA16X_DPM_TIM_TBTT_DELAY            0
  #define DA16X_DPM_TIM_DEAUTH_MAX            5
#endif

#ifdef DA16X_TIM_AP_SYNC
  #define DA16X_DPM_TIM_TBTT_DELAY_OFF        3072
#else
  #define DA16X_DPM_TIM_TBTT_DELAY_OFF        100
#endif

#define DA16X_DPM_AP_SYNC_READY_TIME          1 /* 1ms, Actual value = 500us */

static inline unsigned char da16x_dpm_tim_bcn_chk_int(void)
{
    return DA16X_DPM_TIM_BCN_CHK_INT;
}

static inline unsigned char da16x_dpm_tim_bcn_chk_step(void)
{
    return DA16X_DPM_TIM_BCN_CHK_STEP;
}

static inline unsigned char da16x_dpm_get_bcn_timeout(void)
{
    return DA16X_DPM_BCN_TIMEOUT;
}

static inline unsigned char da16x_dpm_get_bcn_timeout_dtim(void)
{
    return DA16X_DPM_BCN_TIMEOUT_DTIM;
}

static inline unsigned short da16x_dpm_get_tim_filter_timeout(void)
{
    return DA16X_DPM_TIM_FILTER_TIMEOUT;
}

static inline unsigned char da16x_dpm_get_tim_uc_max(void)
{
    return DA16X_DPM_TIM_UC_MAX;
}

static inline unsigned long da16x_dpm_get_tim_ps_poll_timeout(void)
{
    return DA16X_DPM_TIM_PS_POLL_TIMEOUT;
}

static inline unsigned long da16x_dpm_tim_tbtt_delay(void)
{
    return DA16X_DPM_TIM_TBTT_DELAY;
}

static inline long da16x_dpm_tim_tbtt_delay_off(void)
{
    return DA16X_DPM_TIM_TBTT_DELAY_OFF;
}

static inline unsigned char da16x_dpm_get_tim_deauth_max(void)
{
    return DA16X_DPM_TIM_DEAUTH_MAX;
}

#endif    /* __CUSTOMER_DPM_FEATURES_H__ */

/* EOF */
