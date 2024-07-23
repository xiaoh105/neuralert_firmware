/**
 ****************************************************************************************
 *
 * @file umac_workqueue.h
 *
 * @brief Header file for Workqueue of macd11
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

#include "os_con.h"

#define WQ_KTHREAD_TIMESLICE	1		/* In case of Same priority Thread, we have to define this */
#define WQ_KTHREAD_PRIORITY		OS_TASK_PRIORITY_SYSTEM+9		/* have to same priority with umac RX. Fix me lator */
/* In case of Many RX AMPDUs, need more stack */
/* [9050] In DPM Aging, during reconnection & tx fail it happen exception, so patch, 181210 */
#define WQ_KTHREAD_STACKSIZE	(128*8)	//1024	/* Thread stack size , Fix me lator 384 * sizeof(int) */
#define WQ_KTHREAD_WORK_FLAG 	( 1 << 4 )// 0x0004
#ifdef	UMAC_DELAY_WORKQUEUE_ENABLE
#define WQ_KTHREAD_WAIT_FLAG	( 1 << 1 )// 0x0001
#define WQ_KTHREAD_DELAY_FLAG	( 1 << 2 )// 0x0002
/* Max Number of Workqueue Event */
#define WQ_EVENT_NUM		8
#endif

/* workqueue list structure */
typedef struct __workNode
{
	struct work_struct *curwork;
	struct __workNode *next;
} workNode;

typedef struct
{
#ifdef FEATURE_NEW_WORK_LIST
	struct diw_list_head	list;
	int run;
#endif
	TaskHandle_t tx_work_Thread;
	EventGroupHandle_t event_flag;
	SemaphoreHandle_t wq_work_sem;
#ifndef FEATURE_NEW_WORK_LIST
	workNode	*head, *tail; /* workqueue control Node */
#endif
	u32			state;
} workqueue_thread;

typedef enum
{
	WQ_IDLE_STATUS = 0,
	WQ_RUN_STATUS = 1,
	WQ_STOP_STATUS = 2
} WQStatus;

#ifdef	UMAC_DELAY_WORKQUEUE_ENABLE
typedef struct _WQ_FUNC
{
	TX_TIMER *wq_timer;
	WQStatus status; /* workqueue Event Status */
	struct work_struct *delyed_time_work;
} WQ_FUNC;
#endif

