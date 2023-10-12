/**
 ****************************************************************************************
 *
 * @file da16x_timer.h
 *
 * @brief System Driver
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

#ifndef __da16x_timer_h__
#define __da16x_timer_h__

#include "target.h"
#include "rtc.h"

#define DA16X_POWER_DOWN		1
#define DA16X_POWER_ON			0

#define DA16X_IDX_USING		0
#define DA16X_IDX_NEXT			1
#define DA16X_IDX_POWER_STATUS 2
#define DA16X_IDX_RESERVED0	3


typedef struct _DA16X_TIMER_ {
	union {
    	UINT32      space;
		UINT8		content[4];
	}type;
	ISR_CALLBACK_TYPE callback;
	void* booting_offset;
    unsigned long long time;
} DA16X_TIMER;

typedef struct _DA16X_TIMER_PARAM_ {
	void *callback_param;
	void *callback_func;
	void *booting_offset;
} DA16X_TIMER_PARAM;

#define MAX_TIMER   16

typedef enum _DA16X_TIMER_ID_ {
    TIMER_0 = 0,
    TIMER_1,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    TIMER_5,
    TIMER_6,
    TIMER_7,
    TIMER_8,
    TIMER_9,
    TIMER_10,
    TIMER_11,
    TIMER_12,
    TIMER_13,
    TIMER_14,
    TIMER_15,
    TIMER_ERR
} DA16X_TIMER_ID;

#define SYSTIME_OFFSET  ((DA16X_RETMEM_BASE + 0x100) + (sizeof(DA16X_TIMER) * TIMER_ERR) + sizeof(UINT32*) + sizeof(UINT32*) + sizeof(UINT32*))
#define SYSTIME_OFFSET_MSEC (SYSTIME_OFFSET + sizeof(UINT32*))
#define TIMER_OFFSET (SYSTIME_OFFSET_MSEC + sizeof(UINT32*))			// size unsigned long long(64bit)
#define ISR_FLAG (TIMER_OFFSET + sizeof(UINT32*) + sizeof(UINT32*))			// size unsigned long long(64bit)

// Return available ID value
DA16X_TIMER_ID DA16X_GET_EMPTY_ID(void);

/* param us */
DA16X_TIMER_ID DA16X_SET_TIMER(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param);

DA16X_TIMER_ID DA16X_SET_WAKEUP_TIMER(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param);

DA16X_TIMER_ID DA16X_KILL_TIMER(DA16X_TIMER_ID id);

WAKEUP_SOURCE DA16X_TIMER_SCHEDULER(void);

DA16X_TIMER_ID DA16X_SET_PERIOD(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param);
void DA16X_SLEEP_EN(void);

#endif

/* EOF */
