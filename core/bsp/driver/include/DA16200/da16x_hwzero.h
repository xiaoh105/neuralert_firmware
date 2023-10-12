/**
 * \addtogroup HALayer
 * \{
 * \addtogroup HW_ZERO	HW Zero
 * \{
 * \brief HW Driver to quickly fill a buffer with a 4-byte value.
 */
 
/**
 ****************************************************************************************
 *
 * @file da16x_hwzero.h
 *
 * @brief HWZero
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


#ifndef __da16x_hwzero_h__
#define __da16x_hwzero_h__

#include "hal.h"

extern void	da16x_zeroing_init(void);
extern void	da16x_zeroing_deinit(void);

extern void	da16x_zeroing_start(UINT32 addr, UINT32 len, UINT32 seed);
extern void	da16x_zeroing_stop(void);

/**
 * \brief Fill a buffer with a 4-byte value, seed
 *
 * \param [in] data			data pointer
 * \param [in] seed			4-byte pattern data
 * \param [in] length		data size
 *
 */
extern void	da16x_memset32(UINT32 *data, UINT32 seed, UINT32 length);

#endif /* __da16x_hwzero_h__ */
/**
 * \}
 * \}
 */
