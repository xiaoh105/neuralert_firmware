/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SOFTIRQ	Soft-IRQ
 * \{
 * \brief Doorbell interrupt used in which a thread signals another thread.
 */

/**
 ****************************************************************************************
 *
 * @file softirq.h
 *
 * @brief Software IRQ Driver
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


#ifndef __softirq_h__
#define __softirq_h__

/**
 * \brief Create SOFTIRQ driver
 *
 */
extern void softirq_create(void);

/**
 * \brief Initialize SOFTIRQ driver
 *
 */
extern void softirq_init(void);

/**
 * \brief Close SOFTIRQ driver
 *
 */
extern void softirq_close(void);

/**
 * \brief Add a callback in list of SOFTIRQ
 *
 * \param [in] callback		callback function
 * \param [in] param		parameters of callback
 * \param [in] startmode	(TRUE: enable, FALSE: disable)
 *
 * \return	doorbell handler
 *
 */
extern HANDLE softirq_registry(USR_CALLBACK callback, VOID *param, UINT32 startmode);

/**
 * \brief Delete a callback in list of SOFTIRQ
 *
 * \param [in] doorbell		doorbell handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 softirq_unregistry(HANDLE doorbell);

/**
 * \brief Push button for signaling a callback
 *
 * \param [in] doorbell		doorbell handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 softirq_push(HANDLE doorbell);

/**
 * \brief Push button for signaling a callback with parameters
 *
 * \param [in] doorbell		doorbell handler
 * \param [in] param		parameters of callback
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 softirq_push_param(HANDLE doorbell, VOID *param);

/**
 * \brief Enable/Disable a callback
 *
 * \param [in] doorbell		doorbell handler
 * \param [in] mode			(TRUE: enable, FALSE: disable)
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 softirq_activate(HANDLE doorbell, UINT32 mode);

#endif	/*__softirq_h__*/
