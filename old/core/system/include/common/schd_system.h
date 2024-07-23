/**
 ****************************************************************************************
 *
 * @file schd_system.h
 *
 * @brief System Monitor
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
 
#ifndef __schd_system_h__
#define __schd_system_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define	SCHD_SYS_TICK_PERIOD	0
#define SCHD_SYS_TICK_TIME	(100/(SCHD_SYS_TICK_PERIOD+1))
#define	SCHD_SYS_MAX_NUM	(100/SCHD_SYS_TICK_TIME)

#define	SCHD_SYS_RTCSW_PERIOD	10
#define SCHD_SYS_RTCSW_TICKS	(100/(SCHD_SYS_RTCSW_PERIOD+1))
#define SCHD_SYS_RTCSW_MAX_NUM (16)

extern void system_schedule_preset();
extern void system_schedule(void *pvParameters);
extern void start_rtc_switch_xtal();

extern void system_event_monitor(UINT32 mode);
extern void system_event_busmon(UINT32 mode, UINT32 para);
extern void system_event_cpumon(UINT32 mode, UINT32 para);
extern void system_event_wdogmon(UINT32 mode);

#endif /* __schd_system_h__ */

/* EOF */
