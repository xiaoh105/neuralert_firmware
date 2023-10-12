/**
 ****************************************************************************************
 *
 * @file wifi_svc_queue.c
 *
 * @brief Software for queues and threads.
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

#if defined (__BLE_PERI_WIFI_SVC_TCP_DPM__)

#include "../../include/queue.h"

#if defined(__GTL_IFC_UART__)
#include "../../include/uart.h"
#endif
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "command.h"
#include "da16_gtl_features.h"

#ifdef __FREERTOS__
#include "msg_queue.h"
#endif

#if defined(__GTL_IFC_UART__)
#ifdef __FREERTOS__
#define GTL_RX_QUEUE_LENGTH	(10)
msg_queue GtlRxQueue;
#else
// GTL Rx msg Q
QueueRecord GtlRxQueue;
SemaphoreHandle_t gtl_rx_q_mutex;
SemaphoreHandle_t gtl_q_mutex;
#endif /* __FREERTOS__ */
#endif /* __GTL_IFC_UART__ */

#ifdef __FREERTOS__
#define CONSOLE_QUEUE_LENGTH	(10)
msg_queue ConsoleQueue;
#else
// User Command Q
QueueRecord ConsoleQueue;

// Command Q protector
SemaphoreHandle_t ConsoleQueueMut; 
#endif

void InitTasks(void)
{
#if defined(__GTL_IFC_UART__)
#ifdef __FREERTOS__
	msg_queue_create(&GtlRxQueue, GTL_RX_QUEUE_LENGTH, NULL);
#else
	gtl_rx_q_mutex = xSemaphoreCreateMutex();
	if (gtl_rx_q_mutex == NULL)
	{
		DBG_ERR("\n\nFailed to create gtl rx q mutex\n\n");
		return;
	}

	gtl_q_mutex = xSemaphoreCreateMutex();
	if (gtl_q_mutex == NULL)
	{
		DBG_ERR("\n\nFailed to create gtl rx q mutex\n\n");
		return;
	}

#endif /* __FREERTOS__ */
#endif /* __GTL_IFC_UART__ */
}

void InitTasksUserCmd(void)
{
#ifdef __FREERTOS__
	msg_queue_create(&ConsoleQueue, CONSOLE_QUEUE_LENGTH, NULL);
#else
	ConsoleQueueMut = xSemaphoreCreateMutex();
	if (ConsoleQueueMut == NULL)
	{
		DBG_ERR("\n\nFailed to create console command Q mutex\n\n");
	}
#endif

}

void EnQueue(QueueRecord *rec, void *vdata)
{
	struct QueueStorage *tmp;

	tmp = (struct QueueStorage *)MEMALLOC(sizeof(struct QueueStorage));

	if (tmp == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	tmp->Next = NULL;
	tmp->Data = vdata;

	if (rec->First == NULL)
	{
		rec->First = tmp;
		rec->Last = tmp;
	}
	else
	{
		rec->Last->Next = tmp;
		rec->Last = tmp;
	}
}

void *DeQueue(QueueRecord *rec)
{
	void *tmp;
	struct QueueStorage *tmpqe;

	if (rec->First == NULL)
	{
		return NULL;
	}
	else
	{
		tmp = rec->First->Data;
		tmpqe = rec->First;
		rec->First = tmpqe->Next;
		MEMFREE(tmpqe);

		if (rec->First == NULL)
		{
			rec->First = rec->Last = NULL;
		}

	}

	return tmp;
}

#endif /* __BLE_PERI_WIFI_SVC_TCP_DPM__ */

/* EOF */
