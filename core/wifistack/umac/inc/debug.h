/**
 ****************************************************************************************
 *
 * @file debug.h
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

#ifndef __MACD11_DEBUG_H
#define __MACD11_DEBUG_H

#ifdef  UMAC_PS_CNTL_MON
#define wsubif_err(...)			trc_que_proc_print(0, __VA_ARGS__ )
#define wsubif_info(...) 		trc_que_proc_print(0, __VA_ARGS__ )
#define dbg_ht(...)			trc_que_proc_print(0, __VA_ARGS__ )

#define dbg_ht_ratelimited(...)		trc_que_proc_print(0, __VA_ARGS__ )

#define dbg_ps(...)			trc_que_proc_print(0, __VA_ARGS__ )
#define dbg_ps_hw(...)			trc_que_proc_print(0, __VA_ARGS__ )
#define dbg_ps_ratelimited(...)		trc_que_proc_print(0, __VA_ARGS__ )

#define sta_dbg(...)			trc_que_proc_print(0, __VA_ARGS__ )
#define mlme_dbg(...)			trc_que_proc_print(0, __VA_ARGS__ )
#define mlme_dbg_ratelimited(...)	trc_que_proc_print(0, __VA_ARGS__ )

#define softmac_warn(...)		trc_que_proc_print(0, __VA_ARGS__ )
#define softmac_debug(...)		trc_que_proc_print(0, __VA_ARGS__ )
#define mps_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)
#define mpl_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)
#define mcsa_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)
#define mhwmp_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)
#define msync_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)
#define mpath_dbg(...)			trc_que_proc_print(0, __VA_ARGS__)

#else
#define wsubif_err(...)			do {} while (0)
#define wsubif_info(...) 		do {} while (0)
#define dbg_ht(...)			do {} while (0)

#define dbg_ht_ratelimited(...)		do {} while (0)

#define dbg_ps(...)			do {} while (0)
#define dbg_ps_hw(...)			do {} while (0)
#define dbg_ps_ratelimited(...)		do {} while (0)

#define sta_dbg(...)			do {} while (0)
#define mlme_dbg(...)			do {} while (0)
#define mlme_dbg_ratelimited(...)	do {} while (0)

#define softmac_warn(...)		do {} while (0)
#define softmac_debug(...)		do {} while (0)

#define mps_dbg(...)			do {} while (0) 
#define mpl_dbg(...)			trc_que_proc_print(0, __VA_ARGS__ ) 	//do {} while (0) 
#define mcsa_dbg(...)			do {} while (0) 
//#define mhwmp_dbg(...)		trc_que_proc_print(0, __VA_ARGS__ ) 	//do {} while (0) 	
#define mhwmp_dbg(...)			do {} while (0) 	
#define msync_dbg(...)			do {} while (0)
#define mpath_dbg(...)			trc_que_proc_print(0, __VA_ARGS__ )	//do {} while (0) 	

#endif
#endif /* __MACD11_DEBUG_H */
