/**
 ****************************************************************************************
 *
 * @file queue.h
 *
 * @brief Header file for queues and threads definitions.
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

#ifndef QUEUE_H_
#define QUEUE_H_

/* include for platform start*/
#include "sdk_type.h"
#include "lwip/err.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
/* include for platform end*/

// Queue stuff.
struct QueueStorage
{
	struct QueueStorage *Next;
	void *Data;
};

typedef struct
{
	struct QueueStorage *First, *Last;
} QueueRecord;

typedef struct
{
	unsigned char  bLength;
	unsigned char  bData[1];
} QueueElement;

#ifdef __FREERTOS__
#include "msg_queue.h"
extern msg_queue ConsoleQueue, GtlRxQueue;
#else
extern SemaphoreHandle_t gtl_rx_q_mutex;
extern QueueRecord ConsoleQueue, GtlRxQueue;
#endif

 /*
 ****************************************************************************************
 * @brief Initialize the UART RX thread and the console key handling thread.
 *
 * @return void
 ****************************************************************************************
*/
void InitTasks(void);
/*
 ****************************************************************************************
 * @brief Adds an item to the queue
 *
 *  @param[in] rec    Queue.
 *  @param[in] vdata  Pointer to the item to be added.
 *
 * @return void
 ****************************************************************************************
*/
void EnQueue(QueueRecord *rec, void *vdata);
/*
 ****************************************************************************************
 * @brief Removes an item from the queue
 *
 *  @param[in] rec  Queue.
 *
 * @return pointer to the item that was removed
 ****************************************************************************************
*/
void *DeQueue(QueueRecord *rec);



#endif //QUEUE_H_

/* EOF */
