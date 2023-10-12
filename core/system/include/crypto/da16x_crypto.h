/**
 * \addtogroup CRYLayer
 * \{
 * \addtogroup CRYPTO_MGMT	Crypto Management
 * \{
 * \brief APIs to initialize HW crypto modules 
 */

/**
 ****************************************************************************************
 *
 * @file da16x_crypto.h
 *
 * @brief DA16200 crypto
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


#ifndef __da16x_crypto_h__
#define __da16x_crypto_h__

//--------------------------------------------------------------------
//	Target System
//--------------------------------------------------------------------

#include "cal.h"
#include "mbedtls/net_sockets.h"

/**
 * \addtogroup CRYPTO_MGMT_PERI	InitCrypto
 * \{
 * \brief Crypto Initialization
 */ 

/**
 * \brief Initialize Crypto Clock
 *
 * \param [in] mode			pll callbak mode \n
 * 							TRUE is support the pll callback of Crypto Engine.
 *
 */
extern void	DA16X_Crypto_ClkMgmt(UINT32 mode);
extern void	DA16X_Crypto_x2ClkMgmt(UINT32 mode); // Test only

/**
 * \brief Initialize Crypto Engine. 
 *
 * \param [in] rflag		parameter for TRNG initialization \n
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32	DA16X_Crypto_Init(UINT32 rflag);

/**
 * \brief Deinitialize Crypto Engine. 
 *
 * \return	void
 */
extern void	DA16X_Crypto_Finish(void);

extern void	DA16X_SetCryptoCoreClkGating(UINT32 mode);

/**
 * \}
 */ 

/**
 * \addtogroup CRYPTO_MGMT_MBEDTLS	InitMbedTLS
 * \{
 * \brief MbedTLS Initialization
 */ 

/**
 * \brief Initialize and bind MbedTLS with the platform specific functions.
 *
 * \param [in] mbedtls_platform		list of functions \n
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 mbedtls_platform_init(const mbedtls_net_primitive_type *mbedtls_platform);

/**
 * \brief Change the platform functions in MbedTLS.
 *
 * \param [in] mbedtls_platform		list of functions \n
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern UINT32 mbedtls_platform_bind(const mbedtls_net_primitive_type *mbedtls_platform);

/**
 * \}
 */ 

#endif /* __da16x_crypto_h__ */
/**
 * \}
 * \}
 */

/* EOF */
