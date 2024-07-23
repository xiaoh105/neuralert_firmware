/**
 ****************************************************************************************
 *
 * @file DualTimer_sample.c
 *
 * @brief Sample app of DualTimer
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

#if defined (__DUALTIMER_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"


typedef	struct	{
    HANDLE			timer;
    EventGroupHandle_t	event;
    UINT32			cmd;
} TIMER_INFO_TYPE;

#define	DTIMER_CMD_RUN			0x0
#define	DTIMER_CMD_STOP			0x1

#define DTIMER_FLAG_0			0x4
#define DTIMER_FLAG_1			0x8

/* External Variables/Functions */

#define	OAL_MSLEEP(wtime)		{							\
        portTickType xFlashRate, xLastFlashTime;		\
        xFlashRate = wtime/portTICK_RATE_MS;			\
        xLastFlashTime = xTaskGetTickCount();			\
        vTaskDelayUntil( &xLastFlashTime, xFlashRate );	\
    }

/* Internal Variables/Functions */

static 	void dtimer_callback_0(void *param)
{
    TIMER_INFO_TYPE	*tinfo;
    UINT32	ioctldata[1];
    BaseType_t	taskwoken;

    if ( param == NULL) {
        return ;
    }
    tinfo = (TIMER_INFO_TYPE *) param;

    if ( tinfo->cmd == DTIMER_CMD_STOP ) {
        ioctldata[0] = DTIMER_DEV_INTR_DISABLE ;
        DTIMER_IOCTL(tinfo->timer, DTIMER_SET_MODE, ioctldata );
    }

    xEventGroupSetBitsFromISR(tinfo->event, (DTIMER_FLAG_0), &taskwoken );
}

static 	void dtimer_callback_1(void *param)
{
    TIMER_INFO_TYPE	*tinfo;
    //ONESHOT Test: UINT32	ioctldata[1];
    BaseType_t	taskwoken;

    if ( param == NULL) {
        return ;
    }
    tinfo = (TIMER_INFO_TYPE *) param;

    //ONESHOT Test: if( tinfo->cmd == DTIMER_CMD_STOP ){
    //ONESHOT Test: 	ioctldata[0] = DTIMER_DEV_INTR_DISABLE ;
    //ONESHOT Test: 	DTIMER_IOCTL(tinfo->timer, DTIMER_SET_MODE, ioctldata );
    //ONESHOT Test: }

    xEventGroupSetBitsFromISR(tinfo->event, (DTIMER_FLAG_1), &taskwoken );
}


/*
 * Sample Functions for TEMPLATE
 */
void DualTimer_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    HANDLE	dtimer0, dtimer1;
    EventGroupHandle_t	event;
    UINT32	ioctldata[3], tickvalue[2];
    TickType_t starttick;
    TIMER_INFO_TYPE tinfo[2];
    EventBits_t masked_evt, checkflag, stackflag;
    UINT32 	cursysclock;

    /* DO SOMETHING */
    PRINTF("DualTimer Sample\n");

    checkflag = 0;

    starttick = xTaskGetTickCount();

    _sys_clock_read( &cursysclock, sizeof(UINT32));

    //======================================================
    // Timer Creation
    //
    event = xEventGroupCreate();
    xEventGroupClearBits(event, (DTIMER_FLAG_0 | DTIMER_FLAG_1) );

    dtimer0 = DTIMER_CREATE(DTIMER_UNIT_00);
    dtimer1 = DTIMER_CREATE(DTIMER_UNIT_01);

    tinfo[0].timer = dtimer0;
    tinfo[0].event = event;
    tinfo[0].cmd   = DTIMER_CMD_RUN;

    tinfo[1].timer = dtimer1;
    tinfo[1].event = event;
    tinfo[1].cmd   = DTIMER_CMD_RUN;

    //======================================================
    // Timer Initializaiton
    //
    if ( dtimer0 != NULL ) {
        DTIMER_INIT(dtimer0);

        ioctldata[0] = DTIMER_DEV_INTR_DISABLE ;
        DTIMER_IOCTL(dtimer0, DTIMER_SET_MODE, ioctldata );

        // Timer Callback
        ioctldata[0] = 0;
        ioctldata[1] = (UINT32)dtimer_callback_0;
        ioctldata[2] = (UINT32) & (tinfo[0]);
        DTIMER_IOCTL(dtimer0, DTIMER_SET_CALLACK, ioctldata );

        // Timer Period
        ioctldata[0] = cursysclock ;	// Clock
        ioctldata[1] = (cursysclock / (20 * MHz)) ;	// Divider
        DTIMER_IOCTL(dtimer0, DTIMER_SET_LOAD, ioctldata );

        // Timer Configuration
        ioctldata[0] = DTIMER_DEV_INTR_ENABLE
                       | DTIMER_DEV_PERIODIC_MODE
                       | DTIMER_DEV_PRESCALE_1
                       | DTIMER_DEV_32BIT_SIZE
                       | DTIMER_DEV_WRAPPING ;
        DTIMER_IOCTL(dtimer0, DTIMER_SET_MODE, ioctldata );

        DTIMER_IOCTL(dtimer0, DTIMER_SET_ACTIVE, ioctldata );

        checkflag |= 0x0004;
    }
    //======================================================
    if ( dtimer1 != NULL ) {
        DTIMER_INIT(dtimer1);

        ioctldata[0] = DTIMER_DEV_INTR_DISABLE ;
        DTIMER_IOCTL(dtimer1, DTIMER_SET_MODE, ioctldata );

        ioctldata[0] = 0;
        ioctldata[1] = (UINT32)dtimer_callback_1;
        ioctldata[2] = (UINT32) & (tinfo[1]);
        DTIMER_IOCTL(dtimer1, DTIMER_SET_CALLACK, ioctldata );

        ioctldata[0] = cursysclock ;	// Clock
        ioctldata[1] = (cursysclock / (20 * MHz)) ;	// Divider
        DTIMER_IOCTL(dtimer1, DTIMER_SET_LOAD, ioctldata );

        ioctldata[0] = DTIMER_DEV_INTR_ENABLE
                       | DTIMER_DEV_PERIODIC_MODE
                       | DTIMER_DEV_PRESCALE_16
                       | DTIMER_DEV_32BIT_SIZE
                       | DTIMER_DEV_ONESHOT ;
        DTIMER_IOCTL(dtimer1, DTIMER_SET_MODE, ioctldata );

        DTIMER_IOCTL(dtimer1, DTIMER_SET_ACTIVE, ioctldata );

        checkflag |= 0x0008;
    }
    //======================================================

    stackflag = 0;

    do {
        masked_evt = 0;

        masked_evt = xEventGroupWaitBits(
                         event,    		// The event group being tested.
                         checkflag,  	// The bits within the event group to wait for.
                         pdFALSE,        // The bits should not be cleared before returning.
                         pdFALSE,        // Don't wait for both bits, either bit will do.
                         portMAX_DELAY );  // Wait forever for the bits to be set.

        xEventGroupClearBits(event, (masked_evt) );

        DTIMER_IOCTL(dtimer0, DTIMER_GET_TICK, &(tickvalue[0]) );
        DTIMER_IOCTL(dtimer1, DTIMER_GET_TICK, &(tickvalue[1]) );

        PRINTF("[%6d] Rcv-Evt: %s %s - Tick [%8d:%8d]\n"
               , ( xTaskGetTickCount() - starttick )
               , ((masked_evt & DTIMER_FLAG_0) ? "DT0" : "   ")
               , ((masked_evt & DTIMER_FLAG_1) ? "DT1" : "   ")
               , tickvalue[0], tickvalue[1] );

        stackflag = stackflag | masked_evt;

    } while (stackflag != checkflag);

    //======================================================
    // Check Tick Count
    PRINTF(">>> wait for 4 seconds\r\n" );
    OAL_MSLEEP(4000);

    DTIMER_IOCTL(dtimer0, DTIMER_GET_TICK, &(tickvalue[0]) );
    DTIMER_IOCTL(dtimer1, DTIMER_GET_TICK, &(tickvalue[1]) );

    xEventGroupWaitBits(event, checkflag, pdFALSE, pdFALSE, 0 );
    PRINTF("[%6d] Rcv-Evt: %s %s - Tick [%8d:%8d]\n"
           , ( xTaskGetTickCount() - starttick )
           , ((masked_evt & DTIMER_FLAG_0) ? "DT0" : "   ")
           , ((masked_evt & DTIMER_FLAG_1) ? "DT1" : "   ")
           , tickvalue[0], tickvalue[1] );

    //======================================================

    tinfo[0].cmd   = DTIMER_CMD_STOP;
    //ONESHOT Test: tinfo[1].cmd   = DTIMER_CMD_STOP;

    PRINTF(">>> DualTimer Test has finished...\r\n" );

    //======================================================
    // Timer Deinitialization
    //
    DTIMER_CLOSE(dtimer0);
    DTIMER_CLOSE(dtimer1);

    vEventGroupDelete(event);

    vTaskDelete(NULL);

    return ;
}
#endif // (__DUALTIMER_SAMPLE__)
