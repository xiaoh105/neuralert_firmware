/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SDIO_SLAVE
 * \{
 * \brief Driver for SDIO 2.0 SLAVE.
 */
/**
 ****************************************************************************************
 *
 * @file sdio_slave.h
 *
 * @brief Driver for SDIO 2.0 Slave.
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


#ifndef __sdio_slave_h__
#define __sdio_slave_h__

#include "oal.h"

/**
 ****************************************************************************************
 * @brief SDIO Slave Initialization.
 * @return If succeeded return ERR_SDIO_NONE.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_INIT( void );

/**
 ****************************************************************************************
 * @brief Raises an interrupt via DATA1 pin.
 * @param[in] data not used.
 * @param[in] length not used.
 * @return void.
 ****************************************************************************************
 */
extern void SDIO_SLAVE_TX(UINT8 *data, UINT32 length);

/**
 ****************************************************************************************
 * @brief Regster callback function for host protocol.
 * @param[in] p_rx_callback_func function pointer.
 * @return ERR_SDIO_NONE.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_CALLBACK_REGISTER(void (* p_rx_callback_func)(UINT32 status));

/**
 ****************************************************************************************
 * @brief Deregster callback function.
 * @return ERR_SDIO_NONE.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_CALLBACK_DEREGISTER(void);

/**
 ****************************************************************************************
 * @brief Set the OCR value.
 * @param[in] ocr OCR value to set.
 * @return OCR value.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_SET_OCR(UINT32 ocr);

/**
 ****************************************************************************************
 * @brief Get the OCR value.
 * @return OCR value.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_GET_OCR( void );
/* if the user want to protect the SRAM */

/**
 ****************************************************************************************
 * @brief Set the SDIO buffer. DA16200 can protect the internal memory from host via this function.
 * @return void.
 ****************************************************************************************
 */
extern void SDIO_SLAVE_SET_BUFFER( UINT8 *buffer);

/**
 ****************************************************************************************
 * @brief SDIO Slave de-initialization.
 * @return ERR_SDIO_NONE.
 ****************************************************************************************
 */
extern UINT32 SDIO_SLAVE_DEINIT( void );

#endif
