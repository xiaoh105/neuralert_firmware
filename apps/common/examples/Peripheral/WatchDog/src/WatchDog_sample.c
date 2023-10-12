/**
 ****************************************************************************************
 *
 * @file WatchDog_sample.c
 *
 * @brief Sample app for WatchDog
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

#if defined (__WATCHDOG_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "command.h"

#define MEGA			1000000
#define WDT_TIMEOUT		2

extern void watch_dog_kick_stop();
extern void set_wdt_rescale_time(int rescale_time);

/* Internal Variables/Functions */

typedef	 struct 	__wdog_fsm__ {
    QueueHandle_t		mutex;
    HANDLE				watchdog;
    TaskHandle_t		currtx;
}	WDOG_FSM_TYPE;

static WDOG_FSM_TYPE	*wdog_fsm = NULL;

/*
 * Sample Functions for WatchDog
 */


static void sample_watchdog_callback( VOID *param )
{
    WDOG_FSM_TYPE *wfsm;

    Printf(" [%s] Watch dog time-out! \r\n", __func__);

    if ( param == NULL ) {
        return;
    }

    wfsm = (WDOG_FSM_TYPE *)param;

    if ( wfsm->currtx != NULL ) {
        reboot_func(SYS_RESET);			// Reset
    }
}

static	UINT32 watchdog_active(WDOG_FSM_TYPE *wfsm, UINT32 wdogcount)
{
    if (	wfsm != NULL ) {
        if ( xSemaphoreTake((wfsm->mutex), portMAX_DELAY) == pdTRUE ) {

            WDOG_IOCTL(wfsm->watchdog, WDOG_SET_DISABLE, NULL );

            WDOG_IOCTL(wfsm->watchdog, WDOG_SET_COUNTER, &wdogcount ); // update

            wfsm->currtx 	= (TaskHandle_t)xTaskGetCurrentTaskHandle();

            WDOG_IOCTL(wfsm->watchdog, WDOG_SET_ENABLE, NULL );

            return TRUE;
        }
    }

    return FALSE;
}

static 	void watchdog_deactive(WDOG_FSM_TYPE *wfsm)
{
    if (	wfsm != NULL ) {
        if ( wfsm->currtx == (TaskHandle_t)xTaskGetCurrentTaskHandle() ) {

            wfsm->currtx 	= (TaskHandle_t)NULL;

            WDOG_IOCTL(wfsm->watchdog, WDOG_SET_DISABLE, NULL );
            xSemaphoreGive((wfsm->mutex));
        }
    }
}

void WatchDog_sample(void *param)
{
    extern void wdt_disable_auto_goto_por();
    extern void wdt_enable_auto_goto_por();
    DA16X_UNUSED_ARG(param);

    /* DO SOMETHING */

    UINT32		idx;
    UINT32		cursysclock;
    UINT32		ioctldata[3];

    PRINTF("WATCHDOG_SAMPLE\n");
    PRINTF("The system may reboot repeatedly while running this code.\n");
    PRINTF("If you don't want it, type \"reset\" to stop it.\n");

    vTaskDelay(500);	// 5 sec

    PRINTF(">>> WatchDog Test...\n" );
    PRINTF(" >> set watchdog timeout : %d sec \n", WDT_TIMEOUT);
    set_wdt_rescale_time(WDT_TIMEOUT);		// set rescale time (2 sec)
    wdog_fsm = (WDOG_FSM_TYPE *)pvPortMalloc(sizeof(WDOG_FSM_TYPE));
    if ( wdog_fsm == NULL ) {
        PRINTF(" [%s] alloc error\n", __func__);
        return;
    }

    // WDOG Creation
    wdog_fsm->watchdog = WDOG_CREATE(WDOG_UNIT_0) ;

    if ( wdog_fsm->watchdog == NULL ) {
        PRINTF("WDOG_CREATE(): error\n");
        vPortFree(wdog_fsm);
        return;
    }

    // WDOG Initialization
    WDOG_INIT(wdog_fsm->watchdog);

    wdog_fsm->currtx = NULL;
    vSemaphoreCreateBinary( wdog_fsm->mutex );

    // WDOG Callback
    ioctldata[0] = 0; // index
    ioctldata[1] = (UINT32)&sample_watchdog_callback; // callback
    ioctldata[2] = (UINT32)wdog_fsm; // para
    WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_CALLACK, ioctldata);

    WDOG_IOCTL(wdog_fsm->watchdog, WDOG_SET_RESET, ioctldata ); // Disable RESET

    wdt_disable_auto_goto_por();		// if watchdog occur goto reset
    //wdt_enable_auto_goto_por();		// if watchdog occur goto POR reset, aute reboot

    _sys_clock_read(&cursysclock, sizeof(UINT32));

    vTaskDelay(100);	// 1 sec

    for (idx = 1; idx < 100; idx++ ) {
        UINT32		loop_count = 0;
        UINT32		wdogcont;

        wdogcont = (cursysclock * WDT_TIMEOUT);	// 2 sec

        watchdog_active(wdog_fsm, wdogcont);

        PRINTF("*** %d'th Test ************\n", idx);
        vTaskDelay(10);	// 100 msec

        PRINTF(" >>> Start LOOP (loop count: %d MEGA): \n", idx * 2);
        vTaskDelay(10);	// 100 msec
        loop_count = 0;
        while (1) {
            if (!(++loop_count % (idx * 2 * MEGA))) {
                vTaskDelay(10);	// 100 msec
                break;
            }
            asm volatile ("nop       \n");
        }
        PRINTF(" >>> End LOOP \n\n");

        watchdog_deactive(wdog_fsm);
        vTaskDelay(100);
    }

    vSemaphoreDelete(wdog_fsm->mutex);
    wdog_fsm->mutex = NULL;

    // WDOG Uninitialization
    WDOG_CLOSE(wdog_fsm->watchdog);

    vPortFree(wdog_fsm);

    vTaskDelete(NULL);

    return ;
}
#endif // (__WATCHDOG_SAMPLE__)
