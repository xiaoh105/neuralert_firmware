/**
 ****************************************************************************************
 *
 * @file da16x_dpm_system.h
 *
 * @brief DPM System macros
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


#ifndef __da16x_dpm_system_h__
#define __da16x_dpm_system_h__

/**
 * !!! Caution !!!
 *
 *	DPM TIM Unicast Rx Max Timeout
 * 	 : UC Max default 3, range is ~ 255
 */
#define DPM_TIM_UC_MAX_TWAIT		3	//0x30



#define DPM_SYS_CLK					120 ///< PTIM Initialization SYS CLK
#define DPM_SYS_CLK2				30  ///< PTIM RX SYS CLK
#define DPM_SYS_CLK3				80  ///< CLK when PTIM goes to full-boot.
#define DPM_MAC_CLK					20
#define DPM_PTIM_TIMEOUT_TU			1000

#undef  DPM_CHK_EXT_SIGNAL

#define MIN_DPM_TIM_WAKEUP			1     // 100 msec : 1 dtim period
#define MAX_DPM_TIM_WAKEUP			6000 // 65535 dtim period
#define DA16X_TIM_BOOT_OFFSET		(BOOT_MEM_SET(BOOT_RETMEM) | \
									BOOT_OFFSET_SET(DA16X_RETM_PTIM_OFFSET))

// SLR Version should be moved.
#define RETMEM_NULL_SEQ_NUMBER		(DA16X_RTM_UMAC_BASE + 0x70)

#endif /* __da16x_dpm_system_h__ */

/* EOF */
