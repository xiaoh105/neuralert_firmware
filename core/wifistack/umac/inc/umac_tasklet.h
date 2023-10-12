/**
 ****************************************************************************************
 *
 * @file umac_tasklet.h
 *
 * @brief Header file for WiFi MAC Protocol UMAC Tasklet
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

#ifndef _UMAC_TASKLET_H_
#define _UMAC_TASKLET_H_

//#include "os_con.h"
#include "net_con.h"

/// In case of Same priority Thread, we have to define this.
#define UMAC_KTHREAD_TIMESLICE   	2
/// Default thread priority. Fix me lator.
#define UMAC_KTHREAD_PRIORITY    	OS_TASK_PRIORITY_SYSTEM+9
/// Old 2048 Thread stack size , Fix me lator.
#define UMAC_KTHREAD_STACKSIZE   	(128*2) //2048 //3072		/* 256 * sizeof(int) */
#define UMAC_KTHREAD_WAIT_FLAG   	0x00000001

#define UMAC_THREAD_STOPPED	0
#define UMAC_THREAD_RUNNING	1

/// Max Number of TASKLET Event.
#define TASKLET_EVENT_NUM		2

/// A structure to store all information we needfor our thread.
typedef struct
{
	TaskHandle_t txThread;
	SemaphoreHandle_t tasklet_sem;
	EventGroupHandle_t waitQ;
	u32 				state;
	u32 				priv;
} umac_thread;

typedef enum
{
	TASKLET_IDLE_STATUS = 0,
	TASKLET_RUN_STATUS = 1,
	TASKLET_STOP_STATUS = 2
} TaskletStatus;

/// If you have more event for Tasklet processing, let's add in here.
typedef enum
{
	TASKLET_EVENT_IDLE = 0,
	TASKLET_EVENT_NORMAL = 1,
	TASKLET_EVENT_TXPENDING = 2
} TaskletEvent;

typedef struct _TASKLET_FUNC
{
	/// Event.
	TaskletEvent event;
	/// Tasklet Event Status.
	TaskletStatus status;
	/// Data.
	u32 data;
	/// Function for tasklet event.
	void (*func)();
} TASKLET_FUNC;

extern void umac_close();
extern void umac_init();
extern int umac_tasklet_init(u32 event, void (*func)(unsigned long), unsigned long data);
extern int umac_tasklet_schedule(u32 event);

int umac_tasklet_disable(u32 event);
int umac_tasklet_enable(u32 event);
int umac_tasklet_kill(u32 event);

#endif /* _UMAC_TASKLET_H_ */

