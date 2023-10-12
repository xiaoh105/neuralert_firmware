/**
 * \addtogroup HALayer
 * \{
 * \addtogroup DA16X_RNG	PRNG
 * \{
 * \brief Pseudo random number generator using an additional SW polynomial to increase an entropy.
 */

/**
 ****************************************************************************************
 *
 * @file da16x_rng.h
 *
 * @brief Pseudo random number generator (PRNG)
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

#ifndef __da16x_rng_h__
#define __da16x_rng_h__

#include "hal.h"

/******************************************************************************
 *
 *  RANDOM
 *
 ******************************************************************************/

/**
 * \brief Generate a random number
 *
 * \return	32-bit random value
 *
 */
extern UINT32 da16x_random(void);

/**
 * \brief Preset a seed value of PRNG
 *
 * \param [in] seed			32-bit seed value
 *
 */
extern void   da16x_random_seed(UINT32 seed);

#endif /* __da16x_rng_h__ */
/**
 * \}
 * \}
 */
