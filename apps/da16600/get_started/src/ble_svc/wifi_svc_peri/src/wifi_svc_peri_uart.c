/**
 ****************************************************************************************
 *
 * @file wifi_svc_uart.c
 *
 * @brief UART interface for host messages.
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

#if defined (__BLE_PERI_WIFI_SVC_PERIPHERAL__)


#include "../../include/queue.h"
#include "../../include/uart.h"

#if defined (__GTL_IFC_UART__)
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "command.h"
#include "da16_gtl_features.h"

#if defined (__GTL_PKT_DUMP__)
#define PKT_DUMP	PRINTF
#else
#define PKT_DUMP(...)	do { } while (0);
#endif

// Packet type for fully embedded interface messages 
// (RW BLE non-standard Higher Layer interface).
// See "RW BLE Host Interface Specification" (RW-BLE-HOST-IS).
#define FE_MSG_PACKET_TYPE 0x05
#define RESET_MSG_PACKET_TYPE 0x02

extern uint8_t g_is_ble_reset_by_ext_host;
extern SemaphoreHandle_t gtl_q_mutex;
extern void PUTS_UART1(char *data, int data_len);
extern int getchar_uart1(UINT32 mode);
#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
extern HANDLE uart1;
#endif

void UARTSend(unsigned short size, unsigned char *data)
{
	unsigned short bSenderSize;
	char *bTransmit232ElementArr = NULL;

#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
    // check if tx fifo is empty
    uint8_t status = 0;
    do {
        UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &status);
    }while(!status);

    combo_ble_wkup();
#endif

    bSenderSize = size + 1;
    bTransmit232ElementArr = (char *)MEMALLOC(bSenderSize);
    if (bTransmit232ElementArr) {
        bTransmit232ElementArr[0] = FE_MSG_PACKET_TYPE;
        memcpy(&bTransmit232ElementArr[1], data, size);

        PUTS_UART1((char *)bTransmit232ElementArr, bSenderSize);
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

extern void boot_ble_from_uart();

#define RECEIVE232ELEMENT_MAX_LEN       (1024)

void UARTProc(void)
{
	//unsigned long dwBytesRead;
	unsigned char tmp;
	unsigned short wReceive232Pos = 0;
	unsigned short wDataLength;
	unsigned char * bReceive232ElementArr = NULL;   // bReceive232ElementArr[1000];
	unsigned char bReceiveState = 0;
	unsigned char bHdrBytesRead = 0;

	bReceive232ElementArr = (unsigned char *)MEMALLOC(RECEIVE232ELEMENT_MAX_LEN * sizeof(unsigned char));
	configASSERT(bReceive232ElementArr != NULL);

	while (1)
	{
		/* Get one byte from uart1 comm port */
		tmp = getchar_uart1(portMAX_DELAY);
#if defined (__TMP_DSPS_TEST__)
		continue;
#endif

		switch (bReceiveState)
		{
			case 0:   // Receive FE_MSG
			{
				if (tmp == FE_MSG_PACKET_TYPE)
				{
					bReceiveState = 1;
					wDataLength = 0;
					wReceive232Pos = 0;
					bHdrBytesRead = 0;

					bReceive232ElementArr[wReceive232Pos] = tmp;
					wReceive232Pos++;

					PKT_DUMP("\nI: ");
					PKT_DUMP("%02X ", tmp);
				}
#if defined (__DA14531_BOOT_FROM_UART__)				
				else if ((tmp == RESET_MSG_PACKET_TYPE) && 
						  (g_is_ble_reset_by_ext_host == FALSE))
				{
					// in case BLE is reset unexpectedly
					PRINTF("BLE reset ... \n");
					vTaskDelay(30); // to check with sec_booter with 921600
					boot_ble_from_uart();
				}
#endif
				else
				{
					PKT_DUMP("%02X ", tmp);
				}

			} break;

			case 1:   // Receive Header size = 6
			{
				PKT_DUMP("%02X ", tmp);
				bHdrBytesRead++;
				bReceive232ElementArr[wReceive232Pos] = tmp;
				wReceive232Pos++;

				if (bHdrBytesRead == 6)
				{
					bReceiveState = 2;
				}

			} break;

			case 2:   // Receive LSB of the length
			{
				PKT_DUMP("%02X ", tmp);
				wDataLength += tmp;

				if (wDataLength > MAX_PACKET_LENGTH)
				{
					bReceiveState = 0;
				}
				else
				{
					bReceive232ElementArr[wReceive232Pos] = tmp;
					wReceive232Pos++;
					bReceiveState = 3;
				}

			} break;

			case 3:   // Receive MSB of the length
			{
				PKT_DUMP("%02X ", tmp);
				wDataLength += (unsigned short) (tmp * 256);

				if (wDataLength > MAX_PACKET_LENGTH)
				{
					PKT_DUMP("\nSIZE: %d ", wDataLength);
					bReceiveState = 0;
				}
				else if (wDataLength == 0)
				{
					PKT_DUMP("\nSIZE: %d ", wDataLength);
					SendToMain((unsigned short) (wReceive232Pos - 1), &bReceive232ElementArr[1]);
					bReceiveState = 0;
				}
				else
				{
					bReceive232ElementArr[wReceive232Pos] = tmp;
					wReceive232Pos++;
					bReceiveState = 4;
				}

			} break;

			case 4:   // Receive Data
			{
				PKT_DUMP("%02X ", tmp);
				bReceive232ElementArr[wReceive232Pos] = tmp;
				wReceive232Pos++;

				// 1 ( first byte = FE_MSG_PACKET_TYPE) + 2 (Type) + 2 (dstid) + 2 (srcid) + 2 (lengths size)
				if (wReceive232Pos == wDataLength + 9 )
				{
					// Sendmail program
					SendToMain((unsigned short) (wReceive232Pos - 1), &bReceive232ElementArr[1]);
					bReceiveState = 0;
					PKT_DUMP("\nSIZE: %d ", wDataLength);
				}

			} break;
		}
	}

	if (bReceive232ElementArr) {
	    MEMFREE(bReceive232ElementArr);
	}
}
#endif

#endif /* __BLE_PERI_WIFI_SVC_PERIPHERAL__ */

/* EOF */
