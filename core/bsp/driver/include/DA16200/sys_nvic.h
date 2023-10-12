/**
 * \addtogroup HALayer
 * \{
 * \addtogroup CMSDK_NVIC	NVIC
 * \{
 * \brief Nested Vectored Interrupt Controller
 */

/**
 ****************************************************************************************
 *
 * @file sys_nvic.h
 *
 * @brief NVIC Driver
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


#ifndef __sys_nvic_h__
#define __sys_nvic_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

/**
 * \brief Create NVIC driver
 *
 */
extern void	_sys_nvic_create(void);

/**
 * \brief Initialize NVIC driver
 *
 * \param [in] vec_src		new address of the vector table
 *
 */
extern void	_sys_nvic_init(unsigned int *vec_src);

/**
 * \brief Close NVIC driver
 *
 */
extern void	_sys_nvic_close(void);

/**
 * \brief Get current handler in NVIC
 *
 * \param [in] index				IRQ Number 
 * \param [out] handleroffset		address of the IRQ handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	_sys_nvic_read(int index, void *handleroffset);

/**
 * \brief Enable or disable a new IRQ handler in NVIC
 *
 * \param [in] index				IRQ Number 
 * \param [in] handleroffset		address of the IRQ handler (If value is null, IRQ will be disabled.)
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	_sys_nvic_write(int index, void *handleroffset, uint32_t priority);

#endif	/*__sys_nvic_h__*/
/**
 * \}
 * \}
 */
