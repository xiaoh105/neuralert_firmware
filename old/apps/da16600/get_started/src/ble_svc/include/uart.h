/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief Definitions for uart interface.
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

#ifndef _UART_H_
#define _UART_H_

#define MAX_PACKET_LENGTH 350
#define MIN_PACKET_LENGTH 9

#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
extern void combo_ble_wkup(void);
#endif

/*
****************************************************************************************
* @brief Write message to UART.
*
*  @param[in] size  Message's size.
*  @param[in] data  Pointer to message's data.
*
* @return void.
****************************************************************************************
*/
void UARTSend(unsigned short size, unsigned char *data);
/*
 ****************************************************************************************
 * @brief Send message received from UART to application's main thread.
 *
 *  @param[in] length           Message's size.
 *  @param[in] bInputDataPtr    Pointer to message's data.
 *
 * @return void.
 ****************************************************************************************
*/
void SendToMain(unsigned short length, uint8_t *bInputDataPtr);
/*
 ****************************************************************************************
 * @brief UART Reception thread loop.
 *
 * @return void.
 ****************************************************************************************
*/
void UARTProc(void);

/*
 ****************************************************************************************
 * @brief Init UART iface.
 *
 *  @param[in] Port         COM prot number.
 *  @param[in] BaudRate     Baud rate.
 *
 * @return -1 on failure / 0 on success.
 ****************************************************************************************
*/
uint8_t InitUART(int Port, int BaudRate);


#endif /* _UART_H_ */

/* EOF */
