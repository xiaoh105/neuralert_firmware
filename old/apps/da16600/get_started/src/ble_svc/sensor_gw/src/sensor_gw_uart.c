/**
 ****************************************************************************************
 *
 * @file sensor_gw_uart.c
 *
 * @brief UART interface for host interface messages.
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

#include "sdk_type.h"

#if defined (__BLE_CENT_SENSOR_GW__)


#include "../../include/queue.h"
#include "../../include/uart.h"

#if defined (__GTL_IFC_UART__)
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "command.h"

#include "da16_gtl_features.h"

#if defined (__GTL_PKT_DUMP__)
#define PKT_DUMP	Printf
#else
#define PKT_DUMP(...)	do { } while (0);
#endif

// Packet type for fully embedded interface messages (RW BLE non-standard Higher Layer interface).
// See "RW BLE Host Interface Specification" (RW-BLE-HOST-IS).
#define FE_MSG_PACKET_TYPE 0x05
#define RESET_MSG_PACKET_TYPE 0x02

extern uint8_t g_is_ble_reset_by_ext_host;
#ifndef __FREERTOS__
extern SemaphoreHandle_t gtl_q_mutex;
#endif	//__FREERTOS__
extern void PUTS_UART1(char *data, int data_len);
extern int getchar_uart1(UINT32 mode);
extern void boot_ble_from_uart();
#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
extern HANDLE uart1;
#endif

void UARTSend(unsigned short size, unsigned char *data)
{
	uint8_t *bTransmit232ElementArr
				=(uint8_t *)MEMALLOC(size+1);

#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
    // check if tx fifo is empty
    uint8_t status = 0;
    do {
        UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &status);
    }while(!status);

    combo_ble_wkup();
#endif
	if (bTransmit232ElementArr) {
		*bTransmit232ElementArr = FE_MSG_PACKET_TYPE;
		memcpy((bTransmit232ElementArr+1), data, size);
		PUTS_UART1((char *)bTransmit232ElementArr, size+1);
		MEMFREE(bTransmit232ElementArr);
	}
}

void SendToMain(unsigned short length, uint8_t *bInputDataPtr)
{
#ifdef __FREERTOS__
	if (msg_queue_send(&GtlRxQueue, 0, 0, bInputDataPtr, length, portMAX_DELAY) == OS_QUEUE_FULL) {
		configASSERT(0);
	}
#else
	unsigned char *bDataPtr;

	xSemaphoreTake(gtl_q_mutex, portMAX_DELAY);
	bDataPtr = (unsigned char *)MEMALLOC(length);
	memcpy(bDataPtr, bInputDataPtr, length);

	EnQueue(&GtlRxQueue, bDataPtr);
	xSemaphoreGive(gtl_q_mutex);
#endif
}

#define UART_MSG_HEADER_LEN			(8)
#define UART_MSG_AVAILABLE_LEN		(10 * 1024)

enum {
	UART_STATE_INIT,
	UART_STATE_HEADER,
	UART_STATE_PARAM,
};

void UARTProc(void)
{
	unsigned char data;
	uint8_t *message = NULL;
	uint8_t msg_header[UART_MSG_HEADER_LEN];
	uint8_t state = UART_STATE_INIT;
	uint16_t msg_len = 0;
	uint16_t receive_index = 0;
	uint8_t in_sync = FALSE;

	while (1)
	{
		/* Get one byte from uart1 comm port */
		data = getchar_uart1(portMAX_DELAY);

		switch (state)
		{
			case UART_STATE_INIT:   // Receive FE_MSG
			{
				if (data == FE_MSG_PACKET_TYPE)
				{
					MEMFREE(message)
					memset(&msg_header, 0, sizeof(msg_header));
					receive_index = 0;
					in_sync = TRUE;
					state = UART_STATE_HEADER;
				}
#if defined (__DA14531_BOOT_FROM_UART__)
				else if ((data == RESET_MSG_PACKET_TYPE) &&
						  (g_is_ble_reset_by_ext_host == FALSE) && in_sync == TRUE)
				{
					// in case BLE is reset unexpectedly
					DBG_ERR("\r\nBLE is reset unexpectedly...\n");
					in_sync = FALSE;
					vTaskDelay(30); // to check with sec_booter with 921600
					boot_ble_from_uart();
				}
#endif /* defined (__DA14531_BOOT_FROM_UART__) */
			} break;
			case UART_STATE_HEADER:   // header part
			{
				if (receive_index < UART_MSG_HEADER_LEN) {
					msg_header[receive_index++] = data;
				}

				if (receive_index == UART_MSG_HEADER_LEN) {
					uint16_t *header = (uint16_t *)msg_header;
					uint16_t len = header[3];
					msg_len = sizeof(msg_header) + len;
					if (len > 0) {
						if (len > UART_MSG_AVAILABLE_LEN) {
							DBG_ERR("\r\n! The packet is too big (%d bytes)!\r\n", len);
						} else {
							message = (uint8_t *)MEMALLOC(msg_len * sizeof(uint8_t));
						}

						if (message == NULL) {
							DBG_INFO("\r\nBLE > msg_id:0x%04x, dest_id:0x%04x, src_id:0x%04x, param_len:%d\n",
									header[0], header[1], header[2], header[3]);
							DBG_ERR("\r\n! Not enough memory !\r\n");
						} else {
							memcpy(message, msg_header, sizeof(msg_header));
						}
						state = UART_STATE_PARAM;
					} else {
						SendToMain(msg_len, msg_header);
						state = UART_STATE_INIT;
					}
				} else if (receive_index > UART_MSG_HEADER_LEN) {
					DBG_ERR("\r\n! Wrong index !\r\n");
					in_sync = FALSE;
					state = UART_STATE_INIT;
				}
			} break;
			case UART_STATE_PARAM:	// parameter part
			{
				if (receive_index < msg_len) {
					if (message != NULL) {
						message[receive_index] = data;
					}
					receive_index++;
				}

				if (receive_index == msg_len) {
					if (message != NULL) {
						SendToMain(msg_len, message);
						MEMFREE(message)
					}
					state = UART_STATE_INIT;
				} else if (receive_index > msg_len) {
					DBG_ERR("! Wrong index !\r\n");
					in_sync = FALSE;
					state = UART_STATE_INIT;
				}
			} break;
			default:
				DBG_ERR("! Unknown state !\r\n");
				in_sync = FALSE;
				state = UART_STATE_INIT;
				break;
		}
	}

	MEMFREE(message)
}
#endif

#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */
