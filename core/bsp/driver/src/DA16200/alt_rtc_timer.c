/**
 ****************************************************************************************
 *
 * @file sys_specific.c
 *
 * @brief 
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


#include <stdio.h>
#include <stdlib.h>
#include "hal.h"

#include "alt_rtc_timer.h"

static unsigned long long ALT_COUNTER;
static HANDLE hTimer;

#define ALT_TIMER_125MS		125*1000

static void alt_timer_hisr(void *handler)
{
	DA16X_UNUSED_ARG(handler);

	ALT_COUNTER += 0x1000;
}

int ALT_RTC_ENABLE(void)
{
	UINT32 ioctldata[3], sysclock;
	hTimer = STIMER_CREATE(STIMER_UNIT_0);

	if (hTimer == NULL)
		return ALT_TIMER_ERR;

	ioctldata[0] = 0 ;
	ioctldata[1] = (UINT32)&alt_timer_hisr;
	ioctldata[2] = (UINT32)hTimer;
	STIMER_IOCTL(hTimer, STIMER_SET_CALLACK, ioctldata);

	STIMER_INIT(hTimer);

	 _sys_clock_read(&sysclock, sizeof(UINT32));

	ioctldata[0] = sysclock;		// Clock
	ioctldata[1] = ALT_TIMER_125MS;		// reload value
	STIMER_IOCTL(hTimer , STIMER_SET_UTIME, ioctldata );

	ioctldata[0] = STIMER_DEV_INTCLK_MODE
			| STIMER_DEV_INTEN_MODE
			| STIMER_DEV_INTR_ENABLE ;

	STIMER_IOCTL(hTimer, STIMER_SET_MODE, ioctldata );
	STIMER_IOCTL(hTimer, STIMER_SET_ACTIVE, NULL);

	return ALT_TIMER_OK;
}

unsigned long long ALT_RTC_GET_COUNTER(void)
{
	return ALT_COUNTER;
}

int ALT_RTC_DISABLE(void)
{
	STIMER_IOCTL(hTimer, STIMER_SET_DEACTIVE, NULL);
	STIMER_CLOSE(hTimer);
	return ALT_TIMER_OK;
}

/* EOF */
