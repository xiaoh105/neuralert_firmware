/**
 * \addtogroup HALayer
 * \{
 * \addtogroup RTCIFBCFM
 * \{
 * \brief RTC Mirror & Bus Clock Measurement
 */

/**
 ****************************************************************************************
 *
 * @file da16x_rtcifbcfm.h
 *
 * @brief RTCIFBCFM Controller
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


#ifndef	__da16x_rtcifbcfm_h__
#define	__da16x_rtcifbcfm_h__


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

/******************************************************************************
 *
 ******************************************************************************/

/**
 * \brief Initialize a RTC Mirror.
 *
 * \param [in] mode			If mode == TRUE, it runs in RTOS mode; otherwise, in Non-OS mode.
 *
 */
extern void da16x_rtc_init_mirror(int mode);

/**
 * \brief Get current 64-bit value of the free-running RTC counter.
 *
 * \return	current 64-bit value.
 *
 */
extern UINT64 da16x_rtc_get_mirror(void);

/**
 * \brief Get current 32-bit value of the free-running RTC counter.
 *
 * \param [in] mode		If mode == TRUE, the upper part; otherwise, the lower part.
 *
 * \return	current 32-bit value.
 *
 */
extern UINT32 da16x_rtc_get_mirror32(int mode);

/******************************************************************************
 *
 ******************************************************************************/

/**
 * \brief Run a RTC Mirror in RTOS mode.
 *
 */
extern void da16x_bcf_measure_init(void);

/**
 * \brief Get current 64-bit timestamp derived from 32KHz XTAL.
 *
 * \param [in] x32k_cycle	(cycle+1) divider
 *
 * \return	current 64-bit value.
 *
 */
extern UINT64 da16x_bcf_measure(UINT32 x32k_cycle);

/******************************************************************************
 *
 ******************************************************************************/

#define RTC_EXP_STATUS_EXPIRE	(1<<0)
#define RTC_EXP_STATUS_TARND	(1<<1)
#define RTC_EXP_STATUS_ABNORMAL	(1<<2)
#define	RTC_EXP_STATUS_RUN	(1<<8)

extern void rtc_exp_timer_init(void);
extern UINT32 registry_rtc_exp_timer(UINT64 exptime, VOID *isrcallback);
extern UINT32 clear_rtc_exp_status(UINT32 mode);

#if 1
/******************************************************************************
 * easy wrapper functions
 ******************************************************************************/
	 //
//	Driver Structure
//
typedef struct	__da16x_rtcif_handler__
{
	UINT64  			set_time;
} RTC_MCNT_TYPE;
extern HANDLE	MCNT_CREATE(void);
// count means RTC clock count.
extern int		MCNT_START(HANDLE handler, UINT64 count, ISR_CALLBACK_TYPE *callback);
extern int 		MCNT_STOP(HANDLE handler, UINT64 *remain);
extern void		MCNT_CLOSE(HANDLE handler);
#endif
#endif	/*__da16x_rtcifbcfm_h__*/
/**
 * \}
 * \}
 */
