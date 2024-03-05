/**
 ****************************************************************************************
 *
 * @file hello_world.c
 *
 * @brief Sample functions for Customer applicatons.
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


#include "da16x_system.h"
#include "da16x_types.h"
#include "command_net.h"
#include "da16x_sys_watchdog.h"

/* External functions */
#if defined ( __RUNTIME_CALCULATION__ )
extern void printf_with_run_time(char * string);
#endif    // __RUNTIME_CALCULATION__


/* External variables */


/* Glocal variables */


/* Global functions */


/* Local functions */


void customer_hello_world_1(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

#if defined ( __RUNTIME_CALCULATION__ )
    printf_with_run_time("Run Apps1");
#endif // __RUNTIME_CALCULATION__

    da16x_sys_watchdog_notify(sys_wdog_id);

    PRINTF(">>> Hello World #1 ( Non network dependent application ) !!!\n");

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

void customer_hello_world_2(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

#if defined ( __RUNTIME_CALCULATION__ )
    printf_with_run_time("Run Apps2");
#endif // __RUNTIME_CALCULATION__

    da16x_sys_watchdog_notify(sys_wdog_id);

    PRINTF(">>> Hello World #2 ( network dependent application ) !!!\n");

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

/* EOF */
