/**
 ****************************************************************************************
 *
 * @file schd_system.c
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

#include "sdk_type.h"

#include <stdio.h>

#include "hal.h"
#include "oal.h"
#include "driver.h"

#include "da16x_system.h"
#include "buildtime.h"
#ifdef	SUPPORT_FCI_TX_MONITOR
#include "tx_da16x_coroutine.h"
#endif	//SUPPORT_FCI_TX_MONITOR

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#ifdef	BUILD_OPT_DA16200_FPGA
#define	SW_BIN_BUILD_INFO	(0x40000000|((REV_BUILDFULLTIME/100)%100000000))
#else	//BUILD_OPT_DA16200_FPGA
#define	SW_BIN_BUILD_INFO	(0x00000000|((REV_BUILDFULLTIME/100)%100000000))
#endif	//BUILD_OPT_DA16200_FPGA


#define	SCHD_SYS_MAX_EVENT	(4)

#define	SCHD_SYS_MALLOC(...)	APP_MALLOC( __VA_ARGS__ )
#define	SCHD_SYS_FREE(...)	APP_FREE( __VA_ARGS__ )

#define	SCHD_SYS_PRINTF(...)	PRINTF( __VA_ARGS__ )
#define	SCHD_SYS_DEBUG(...)	PRINTF( __VA_ARGS__ )


#undef	SUPPORT_OS_WATCHDOG
#ifdef	BUILD_OPT_VSIM
#define	SCHD_WDOG_RESCALE(x)	((x)/50)		/*  20 msec */
#else	//BUILD_OPT_VSIM
//#define	SCHD_WDOG_RESCALE(x)	((x)/2)		/* 500 msec */
#define	SCHD_WDOG_RESCALE(x)	((x))			/* 1 sec */
//#define	SCHD_WDOG_RESCALE(x)	((x)*2)		/* 2 sec */
#endif	//BUILD_OPT_VSIM

/******************************************************************************
 *
 *  Types
 *
 ******************************************************************************/
typedef		enum	{
	SYSM_REQ_NOP	= 0,

	SYSM_REQ_MON_ON,
	SYSM_REQ_MON_OFF,

	SYSM_BUSMON_ON,
	SYSM_BUSMON_OFF,
	SYSM_CPUMON_ON,
	SYSM_CPUMON_OFF,

	SYSM_FORCE_PRINT,
	SYSM_FORCE_RTCCLK,
	SYSM_FORCE_WDOG,
	SYSM_REQ_MAX
} 	SYSM_EVT_REQ_LIST;

typedef	 struct		__sys_event__
{
	UINT32		cmd;
	VOID		*param;
} SCHD_SYS_EVT_TYPE;

typedef	 struct 	__sys_fsm__
{
	QueueHandle_t	queue;
	UINT8		*queuebuf;
	TimerHandle_t	timer;

#ifdef	BUILD_OPT_DA16200_ASIC
	TimerHandle_t	rtcswitch;
#endif	//BUILD_OPT_DA16200_ASIC

	UINT8		dismon;
	UINT8		busmon;
	UINT8		cpumon;
	UINT8		wdogmon;
	HANDLE		watchdog;
	UINT8		watchdog_halt;
	//EXECUTION_TIME 	watchdog_tstamp;
	unsigned long long 	watchdog_tstamp;

	UINT32		index;
	UINT32		statistics[SCHD_SYS_MAX_NUM][10];
	UINT32		transition[5];	// CPU Load
#ifdef	SUPPORT_FCI_TX_MONITOR
	UINT32		txnxstatistics[SCHD_SYS_MAX_NUM][10];
#endif	//SUPPORT_FCI_TX_MONITOR
	char		flag[7][4];

	SCHD_SYS_EVT_TYPE event;
}	SCHD_SYS_FSM_TYPE;

/******************************************************************************
 *
 *  static variables
 *
 ******************************************************************************/

static	SCHD_SYS_FSM_TYPE *schd_sys_fsm	= NULL;

static void display_btm_statistics(UINT32 index);
static int  display_btm_cpu_load(UINT32 index);

static void system_poll_timer_callback( void * expired_timer_id );
#ifdef	BUILD_OPT_DA16200_ASIC
#ifndef	__REMOVE_32KHZ_CRYSTAL__
static void system_rtcswitch_callback( TimerHandle_t expired_timer_id );
#endif	/* __REMOVE_32KHZ_CRYSTAL__ */
#endif	//BUILD_OPT_DA16200_ASIC
#ifdef	SUPPORT_OS_WATCHDOG
static void system_watchdog_callback( VOID *param );
#endif //SUPPORT_OS_WATCHDOG

#ifdef	SUPPORT_FCI_TX_MONITOR
static void tx_disable_callback(void);
static void tx_restore_callback(void);
#ifdef	SUPPORT_FCI_NX_CUSTOMIZE
static void nx_disable_callback(void);
static void nx_restore_callback(void);
#endif	//SUPPORT_FCI_NX_CUSTOMIZE
static void display_txnx_statistics(UINT32 index);
static void dump_txnx_statistics(UINT32 *data, UINT32 mode);
#else	//SUPPORT_FCI_TX_MONITOR
#define	display_txnx_statistics(...)
#define dump_txnx_statistics(...)
#endif //SUPPORT_FCI_TX_MONITOR

#ifdef WATCH_DOG_ENABLE
void watch_dog_kicking(UINT	rescale_time);
#endif	// WATCH_DOG_ENABLE

/******************************************************************************
 *  trace_scheduler()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
typedef		struct	_rtcif_auto_por_	DA16X_RTCIF_auto_TypeDef;

struct		_rtcif_auto_por_
{
	volatile UINT32 RTC_ACC_REQ_CLR;
	volatile UINT32 RTC_ACC_AUTO_EN;
	volatile UINT32 RTC_ACC_OP_TYPE;
	volatile UINT32 resevred;
	volatile UINT32 RTC_ACC_ADDR;
	volatile UINT32 RTC_ACC_WDATA;
};

#define RTCIF_AUTO_POR	((DA16X_RTCIF_auto_TypeDef *)(RTCIF_BASE_0 + 0x80))
#define	WATCH_DOG_TIME_1ST	3	/* SEC */
void wdt_disable_auto_goto_por()
{
	RTCIF_AUTO_POR->RTC_ACC_AUTO_EN = 0;
}

void wdt_enable_auto_goto_por()
{
	RTCIF_AUTO_POR->RTC_ACC_AUTO_EN = 1;
}

void	setup_wdog()
{
#ifdef WATCH_DOG_ENABLE
	UINT32	iodata;

	schd_sys_fsm = (SCHD_SYS_FSM_TYPE *)pvPortMalloc(sizeof(SCHD_SYS_FSM_TYPE));
	DRIVER_MEMSET( schd_sys_fsm, 0x00, sizeof(SCHD_SYS_FSM_TYPE));
	//da16x_memset32((UINT32 *)schd_sys_fsm, 0x00, sizeof(SCHD_SYS_FSM_TYPE));

	/* watchdog */
	schd_sys_fsm->watchdog = WDOG_CREATE(WDOG_UNIT_0) ;
	WDOG_INIT(schd_sys_fsm->watchdog);

	WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_RESET, NULL );

	if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)) {	//Debugger mode
		WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_DISABLE, NULL );
	} else {
		WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_ENABLE, NULL );

		schd_sys_fsm->watchdog_halt = FALSE;
		iodata = TRUE;
		WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_ABORT, &iodata);
	}

#if	defined(__AUTO_REBOOT_WDT__)
	/* if watchdog occur goto POR reset */
	wdt_enable_auto_goto_por();
#endif

#endif	/* WATCH_DOG_ENABLE */
}

void	system_schedule_preset()
{
#ifndef WATCH_DOG_ENABLE
	schd_sys_fsm = (SCHD_SYS_FSM_TYPE *)pvPortMalloc(sizeof(SCHD_SYS_FSM_TYPE));
	DRIVER_MEMSET( schd_sys_fsm, 0x00, sizeof(SCHD_SYS_FSM_TYPE));
#endif // WATCH_DOG_ENABLE

	/* version */
	SYSSET_SWVERSION(REV_BUILDDATE_HEX);
	SYSSET_SWVERSION(REV_BUILDTIME_HEX);
	SYSSET_SWVERSION(SVN_REVISION_NUMBER_HEX);
	SYSSET_SWVERSION(SW_BIN_BUILD_INFO);

	/* cpu sleep */
	da16x_idle_set_info(TRUE);

#ifdef WATCH_DOG_ENABLE
	watch_dog_kicking(WATCH_DOG_TIME_1ST);	/* 1st time watch_dog pre_scale */
#endif	/* WATCH_DOG_ENABLE */
}

#ifdef WATCH_DOG_ENABLE
int	watchDogKickingStopFlag = 0;
void watch_dog_kicking(UINT	rescale_time)
{
	UINT32  cursysclock;

	if (watchDogKickingStopFlag) {
		return;
	}

	_sys_clock_read( &cursysclock, sizeof(UINT32));
	cursysclock = SCHD_WDOG_RESCALE(cursysclock);

	if (rescale_time) {
		cursysclock = cursysclock * rescale_time; 
	}

	WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_DISABLE, NULL );
	WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_COUNTER, &cursysclock ); // update
	WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_ENABLE, NULL );
}

void watch_dog_kick_stop()
{
	watchDogKickingStopFlag = pdTRUE;
}

void watch_dog_kick_start()
{
	watchDogKickingStopFlag = pdFALSE;
}
#endif	/* WATCH_DOG_ENABLE */

void system_schedule(void *pvParameters)
{
	DA16X_UNUSED_ARG(pvParameters);

	if ( schd_sys_fsm == NULL ){
		return;
	}

#ifdef	SUPPORT_FCI_TX_MONITOR
	os_da16x_mon_registry( (void *)(&tx_disable_callback), (void *)(&tx_restore_callback) );
#ifdef	SUPPORT_FCI_NX_CUSTOMIZE
	net_da16x_mon_registry( (void *)(&nx_disable_callback), (void *)(&nx_restore_callback) );
#endif	//SUPPORT_FCI_NX_CUSTOMIZE
#endif	//SUPPORT_FCI_TX_MONITOR


	/* event queue */
	schd_sys_fsm->queuebuf = (UINT8 *)pvPortMalloc((sizeof(SCHD_SYS_EVT_TYPE)*SCHD_SYS_MAX_EVENT));
	schd_sys_fsm->queue = xQueueCreate(SCHD_SYS_MAX_EVENT, sizeof(SCHD_SYS_EVT_TYPE));
	if (schd_sys_fsm->queue == NULL) {
		PRINTF(" [%s] queue create error\n", __func__);
	}
	else {
		//PRINTF(" [%s] Q create \r\n", __func__);
	}

	schd_sys_fsm->timer = xTimerCreate("SysMtic",
										SCHD_SYS_TICK_TIME,
										pdTRUE,
										(void *)0,
										(TimerCallbackFunction_t) system_poll_timer_callback);
	if (schd_sys_fsm->timer == NULL) {
// error
		PRINTF(" [%s] Timer create error: SysMtic \n", __func__);
	}

#ifdef	SUPPORT_OS_WATCHDOG
	if ( schd_sys_fsm->watchdog != NULL ){
		UINT32 ioctldata[3];

		ioctldata[0] = 0; // index
		ioctldata[1] = (UINT32)&system_watchdog_callback; // callback
		ioctldata[2] = (UINT32)schd_sys_fsm; // para
		WDOG_IOCTL(schd_sys_fsm->watchdog, WDOG_SET_CALLACK, ioctldata);
	}
#endif	//SUPPORT_OS_WATCHDOG

	//PRINTF(" [%s] Wait event queue...... \n", __func__);	/* TEMPORARY */
	while(1) {
		{
			int		status;

			status = xQueueReceive(schd_sys_fsm->queue, &schd_sys_fsm->event, portMAX_DELAY);
			if ( status != pdPASS ) {
				//Printf(" [%s] Event abnormal(0x%x) \n", __func__, status);	/* TEMPORARY */

				continue;
			}
			else {
				//Printf(" [%s] %d Event occured (cmd:%d) \n", __func__, schd_sys_fsm->event.cmd);	/* TEMPORARY */
			}
		}

		switch(schd_sys_fsm->event.cmd){
			case	SYSM_REQ_MON_ON:
				xTimerStart(schd_sys_fsm->timer, 0);
				break;
			case	SYSM_REQ_MON_OFF:
				xTimerStop(schd_sys_fsm->timer, 0);
				break;

			case	SYSM_BUSMON_ON:
				schd_sys_fsm->busmon = TRUE;
				break;
			case	SYSM_BUSMON_OFF:
				schd_sys_fsm->busmon = FALSE;
				break;

			case	SYSM_CPUMON_ON:
				schd_sys_fsm->cpumon = (UINT8)((UINT32)(schd_sys_fsm->event.param));
				break;
			case	SYSM_CPUMON_OFF:
				schd_sys_fsm->cpumon = 0;
				break;

			case	SYSM_FORCE_PRINT:
				if ( schd_sys_fsm->cpumon == 0 ){
					display_btm_statistics((UINT32)(schd_sys_fsm->event.param));
					display_txnx_statistics((UINT32)(schd_sys_fsm->event.param));
				}else{
					if ( display_btm_cpu_load((UINT32)(schd_sys_fsm->event.param)) == TRUE ){
						display_txnx_statistics((UINT32)(schd_sys_fsm->event.param));
					}
				}
				break;
#ifdef	SUPPORT_OS_WATCHDOG
			case	SYSM_FORCE_WDOG:
				{
					if ( schd_sys_fsm->event.param != NULL ) {
						if ( schd_sys_fsm->wdogmon == FALSE ) {
							schd_sys_fsm->wdogmon = TRUE;
							PRINTF("WDOG reset enabled\n");
						}
						else {
							schd_sys_fsm->wdogmon = FALSE;
							PRINTF("WDOG reset disabled\n");
						}
					}

					if ( schd_sys_fsm->wdogmon == FALSE ){
						schd_sys_fsm->watchdog_halt = FALSE;
					}
				}
				break;
#endif	//SUPPORT_OS_WATCHDOG

			default:
				break;
		}
	}
}

/******************************************************************************
 *  display_btm_statistics( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	BTMSCALE	10	/* 10 : permill, 1 : percent */
#if	(BTMSCALE == 10)
#define	BTMDISPLAY	"%4d %%."
#else
#define	BTMDISPLAY	"%3d%%"
#endif	//(BTMSCALE == 10)

static void	display_btm_wordbook(UINT32 flag, char *word)
{
	const char btm_token_mode_word[4] = { 'C', 'W', 'S', '-' };
	const char btm_token_num_word[] = "0123456789abcdef";

	if ( flag & 0x80 ){
		word[0] = btm_token_mode_word[((flag >> 5) & 0x3)];
		word[1] = ( ((flag >> 4) & 0x1) == 0 ) ? 'S' : 'M' ;
		word[2] = btm_token_num_word[(flag & 0x0f)];
	}else{
		word[0] = '-';
		word[1] = '-';
		word[2] = '-';
	}
	word[3] = '\0';
}

static void	display_btm_statistics(UINT32 index)
{
	UINT32	*statistics;
	UINT32	scaler;

	statistics = schd_sys_fsm->statistics[index];

	display_btm_wordbook( ((statistics[8] >>0 ) & 0x0ff), (schd_sys_fsm->flag[0]) );
	display_btm_wordbook( ((statistics[8] >>8 ) & 0x0ff), (schd_sys_fsm->flag[1]) );
	display_btm_wordbook( ((statistics[8] >>16) & 0x0ff), (schd_sys_fsm->flag[2]) );
	display_btm_wordbook( ((statistics[8] >>24) & 0x0ff), (schd_sys_fsm->flag[3]) );
	display_btm_wordbook( ((statistics[9] >>0 ) & 0x0ff), (schd_sys_fsm->flag[4]) );
	display_btm_wordbook( ((statistics[9] >>8 ) & 0x0ff), (schd_sys_fsm->flag[5]) );
	display_btm_wordbook( ((statistics[9] >>16) & 0x0ff), (schd_sys_fsm->flag[6]) );

	scaler = ((50*BTMSCALE)<<13)/(statistics[0]>>(17));
	PRINTF("[BTM: FR %08x,"
		" %s %08x (" BTMDISPLAY "), %s %08x (" BTMDISPLAY "), %s %08x (" BTMDISPLAY "),"
		" %s %08x (" BTMDISPLAY "), %s %08x (" BTMDISPLAY "), %s %08x (" BTMDISPLAY "),"
		" MCW %s %08x ]\n"
		, statistics[0]
		, (schd_sys_fsm->flag[0]), statistics[1], (((statistics[1]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[1]), statistics[2], (((statistics[2]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[2]), statistics[3], (((statistics[3]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[3]), statistics[4], (((statistics[4]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[4]), statistics[5], (((statistics[5]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[5]), statistics[6], (((statistics[6]>>16)*scaler)>>13)
		, (schd_sys_fsm->flag[6]), statistics[7]
		);

}

/******************************************************************************
 *  display_btm_cpu_load( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	BTMABS(a,b)	((a>b)?(a-b):(b-a))

static int	display_btm_cpu_load(UINT32 index)
{
	UINT32	*statistics;
	UINT32	scaler;
	UINT32	quotient[2];

	statistics = schd_sys_fsm->statistics[index];

	scaler = ((50*BTMSCALE)<<13)/(statistics[0]>>(17));

	quotient[0] = (((statistics[1]>>16)*scaler)>>13); // CM0
	quotient[1] = (((statistics[2]>>16)*scaler)>>13); // WM0

	if ( (BTMABS((schd_sys_fsm->transition[0]), quotient[0]) <= (schd_sys_fsm->cpumon * BTMSCALE))
	   && (BTMABS((schd_sys_fsm->transition[1]), quotient[1]) < (schd_sys_fsm->cpumon * BTMSCALE)) ){
		// If fluxion is below the threshold, no display !!
		return FALSE;
	}
	schd_sys_fsm->transition[0] = quotient[0];
	schd_sys_fsm->transition[1] = quotient[1];

	schd_sys_fsm->transition[2] = (((statistics[3]>>16)*scaler)>>13); // CM1
	schd_sys_fsm->transition[3] = (((statistics[4]>>16)*scaler)>>13); // WM1
	schd_sys_fsm->transition[4] = (((statistics[6]>>16)*scaler)>>13); // SM0

	if ( schd_sys_fsm->transition[4] != 0 ){
		/* CPU Sleep enabled */
		PRINTF("[BTM: FR %08x,"
			" CPU %08x (" BTMDISPLAY ") = CM " BTMDISPLAY " + WM " BTMDISPLAY " + STALL " BTMDISPLAY " ( SM " BTMDISPLAY ") ,"
			" MAC %08x (" BTMDISPLAY ") = CM " BTMDISPLAY " + WM " BTMDISPLAY " ]\n"
			, statistics[0]
			, (statistics[0] - statistics[6] + statistics[1] + statistics[2])
			, (100*BTMSCALE - schd_sys_fsm->transition[4])
			, schd_sys_fsm->transition[0] // CM0
			, schd_sys_fsm->transition[1] // WM0
			, (100*BTMSCALE - schd_sys_fsm->transition[4] - schd_sys_fsm->transition[0] - schd_sys_fsm->transition[1]) // STALL
			, schd_sys_fsm->transition[4] // SM0

			, (statistics[3]+statistics[4]), (schd_sys_fsm->transition[2]+schd_sys_fsm->transition[3])
			, schd_sys_fsm->transition[2] // CM1
			, schd_sys_fsm->transition[3] // WM1
			);
	}else{
		/* CPU Sleep disabled */
		PRINTF("[BTM: FR %08x,"
			" CPU (" BTMDISPLAY ") = CM " BTMDISPLAY " + WM " BTMDISPLAY " + STALL " BTMDISPLAY " (SM " BTMDISPLAY ") ,"
			" MAC %08x (" BTMDISPLAY ") = CM " BTMDISPLAY " + WM " BTMDISPLAY " ]\n"
			, statistics[0], (100*BTMSCALE)
			, schd_sys_fsm->transition[0] // CM0
			, schd_sys_fsm->transition[1] // WM0
			, (100*BTMSCALE - schd_sys_fsm->transition[0] - schd_sys_fsm->transition[1])
			, schd_sys_fsm->transition[4] // SM0

			, (statistics[3]+statistics[4]), (schd_sys_fsm->transition[2]+schd_sys_fsm->transition[3])
			, schd_sys_fsm->transition[2] // CM1
			, schd_sys_fsm->transition[3] // WM1
			);
	}

	return TRUE;
}


/******************************************************************************
 *  system_poll_timer_callback( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void system_poll_timer_callback( void *expired_timer_id )
{
	DA16X_UNUSED_ARG(expired_timer_id);

	SCHD_SYS_EVT_TYPE	event;

	schd_sys_fsm->dismon = ( SCHD_SYS_TICK_PERIOD == 0 ) ?
					0 :  ((schd_sys_fsm->dismon + 1) % SCHD_SYS_TICK_PERIOD);

	if ( schd_sys_fsm->dismon == 0 ){

		da16x_btm_get_statistics(TRUE,
				schd_sys_fsm->statistics[schd_sys_fsm->index]
			);
		dump_txnx_statistics(
				schd_sys_fsm->txnxstatistics[schd_sys_fsm->index]
				, schd_sys_fsm->busmon
			);

		da16x_btm_control(TRUE);

		event.cmd	= SYSM_FORCE_PRINT;
		event.param = (VOID *)schd_sys_fsm->index;

		xQueueSend(schd_sys_fsm->queue, (VOID *)&event, 0);
	}
	else {
		da16x_btm_get_statistics(TRUE,
				schd_sys_fsm->statistics[schd_sys_fsm->index]
			);
		dump_txnx_statistics(
				schd_sys_fsm->txnxstatistics[schd_sys_fsm->index]
				, schd_sys_fsm->busmon
			);
	}

	schd_sys_fsm->index
		= (schd_sys_fsm->index + 1) % SCHD_SYS_MAX_NUM;
}

/******************************************************************************
 *  start_rtc_switch_xtal( )
 *
 *  Purpose : It starts timer to select rtc's clock
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void start_rtc_switch_xtal()
{
#ifdef	BUILD_OPT_DA16200_ASIC
#ifndef	__REMOVE_32KHZ_CRYSTAL__
        schd_sys_fsm->rtcswitch = xTimerCreate("SysRtic",
											SCHD_SYS_RTCSW_TICKS,
											pdTRUE,
											(void *)0,
											(TimerCallbackFunction_t) system_rtcswitch_callback);
		if (schd_sys_fsm->rtcswitch == NULL) {
			// error
			PRINTF(" [%s] Timer create error: SysRtic \n", __func__);
		}
		else {
			xTimerStart(schd_sys_fsm->rtcswitch, 0);
		}
#endif	/* __REMOVE_32KHZ_CRYSTAL__ */
#endif	//BUILD_OPT_DA16200_ASIC
}

#ifdef	BUILD_OPT_DA16200_ASIC
#ifndef	__REMOVE_32KHZ_CRYSTAL__
/******************************************************************************
 *  system_rtcswitch_callback( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void system_rtcswitch_callback( TimerHandle_t expired_timer_id )
{
	static int rc32k_chk_cnt = 0;

	if ( rc32k_chk_cnt++ < SCHD_SYS_RTCSW_MAX_NUM ) {
		if ( RTC_GET_CUR_XTAL() != EXTERNAL_XTAL ) {
			if ( RTC_SWITCH_XTAL(EXTERNAL_XTAL) != EXTERNAL_XTAL ) {
				return;
			} else{
				PRINTF("RTC switched to XTAL\n");
				RTC_OSC_OFF();
			}
		}
	}else{
		PRINTF("Failed to switch XTAL\n");
	}

	xTimerStop(expired_timer_id, 0);
	xTimerDelete(expired_timer_id, 0);
}

#endif	/* __REMOVE_32KHZ_CRYSTAL__ */
#endif	//BUILD_OPT_DA16200_ASIC


#ifdef	SUPPORT_OS_WATCHDOG
#ifdef WATCH_DOG_ENABLE
/******************************************************************************
 *  system_watchdog_callback( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
static unsigned int system_watchdog_total_task_time_get()
{
    uint32_t ulTotalRunTimeCounter = 0;

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) )
    TaskStatus_t *pxTaskStatusArray = NULL;
	UBaseType_t uxArraySize = 0;
    UBaseType_t x = 0;
    uint32_t ulTotalTime = 0;

	uxArraySize = uxTaskGetNumberOfTasks();

	pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
	if (pxTaskStatusArray != NULL) {
		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalTime);

        ulTotalTime /= 100UL;

        if (ulTotalTime > 0UL) {
            for (x = 0 ; x < uxArraySize ; x++) {
                ulTotalRunTimeCounter += pxTaskStatusArray[x].ulRunTimeCounter;
            }
        }

        vPortFree(pxTaskStatusArray);
        pxTaskStatusArray = NULL;
    }
#endif /* ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) && ( configSUPPORT_STATIC_ALLOCATION == 1 ) ) */


    return (unsigned int)ulTotalRunTimeCounter;
}

static void system_watchdog_callback( VOID *param )
{
	SCHD_SYS_FSM_TYPE *sys_fsm;
	SCHD_SYS_EVT_TYPE event;
	UINT32	cursysclock;
	//EXECUTION_TIME runtime;
	unsigned long long runtime;

	if ( param == NULL ){
		return;
	}

	sys_fsm = (SCHD_SYS_FSM_TYPE *)param;

	event.cmd	= SYSM_FORCE_WDOG;
	event.param = (VOID *)NULL;

	_sys_clock_read( &cursysclock, sizeof(UINT32));

	cursysclock = SCHD_WDOG_RESCALE(cursysclock);

	WDOG_IOCTL(sys_fsm->watchdog, WDOG_SET_DISABLE, NULL );
	WDOG_IOCTL(sys_fsm->watchdog, WDOG_SET_COUNTER, &cursysclock ); // update
	WDOG_IOCTL(sys_fsm->watchdog, WDOG_SET_ENABLE, NULL );

#if 0 /* FreeRTOS Porting - The total time for all tasks. */
	_tx_execution_thread_total_time_get(&runtime);
	if ( schd_sys_fsm->watchdog_tstamp != runtime ){
		schd_sys_fsm->watchdog_tstamp = runtime;
		return;
	}
#else
    runtime = (unsigned long long)system_watchdog_total_task_time_get();

    Printf("runtime(%d)\n", runtime);

    if (runtime == 0) {
        //Not supported.
        return;
    } else if ( schd_sys_fsm->watchdog_tstamp != runtime ){
		schd_sys_fsm->watchdog_tstamp = runtime;
		return;
	}
#endif

	if ( sys_fsm->watchdog_halt == FALSE ){
		sys_fsm->watchdog_halt = TRUE;
		xQueueSend(sys_fsm->queue, (VOID *)&event, 0);
	}
	else {
		WDOG_IOCTL(sys_fsm->watchdog, WDOG_SET_RESET, NULL ); // Enable
	}

}
#endif	/* WATCH_DOG_ENABLE */
#endif	//SUPPORT_OS_WATCHDOG

/******************************************************************************
 *  system_event_monitor( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void system_event_monitor(UINT32 mode)
{
	SCHD_SYS_EVT_TYPE	event;

	event.cmd	= (mode == TRUE)? SYSM_REQ_MON_ON : SYSM_REQ_MON_OFF ;
	event.param = NULL;
	xQueueSend(schd_sys_fsm->queue, (VOID *)&event, 0);
}

/******************************************************************************
 *  system_event_busmon( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void system_event_busmon(UINT32 mode, UINT32 para)
{
	SCHD_SYS_EVT_TYPE	event;

	event.cmd	= (mode == TRUE)? SYSM_BUSMON_ON : SYSM_BUSMON_OFF ;
	event.param = (VOID *)para;

	xQueueSend(schd_sys_fsm->queue, (VOID *)&event, portMAX_DELAY);
}

/******************************************************************************
 *  system_event_cpumon( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void system_event_cpumon(UINT32 mode, UINT32 para)
{
	SCHD_SYS_EVT_TYPE	event;

	event.cmd	= (mode == TRUE)? SYSM_CPUMON_ON : SYSM_CPUMON_OFF ;
	event.param = (VOID *)para;

	xQueueSend(schd_sys_fsm->queue, (VOID *)&event, portMAX_DELAY);
}

/******************************************************************************
 *  system_event_wdogmon( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void system_event_wdogmon(UINT32 mode)
{
	DA16X_UNUSED_ARG(mode);
#ifdef	SUPPORT_OS_WATCHDOG
	SCHD_SYS_EVT_TYPE	event;

	event.cmd	= SYSM_FORCE_WDOG ;
	event.param = (mode == TRUE) ? (VOID *)(0xFFFFFFFF): NULL;

	xQueueSend(schd_sys_fsm->queue, (VOID *)&event, 0);
#endif	//SUPPORT_OS_WATCHDOG
}

#ifdef	SUPPORT_FCI_TX_MONITOR

/******************************************************************************
 *  display_txnx_statistics()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32	nx_packet_txrx_cnt;
static UINT32	nx_intr_pending_cnt;
static UINT32	nx_ipend_tstamp_start;
static UINT32	nx_ipend_tstamp_end;
static UINT32	nx_ipend_tstamp_acc;
static UINT32	tx_intr_pending_cnt;
static UINT32	tx_ipend_tstamp_start;
static UINT32	tx_ipend_tstamp_end;
static UINT32	tx_ipend_tstamp_acc;

static UINT32	max_nx_packet_txrx_cnt;	// counter of ingress & egress packets
static UINT32	max_nx_intr_pending_cnt;
static UINT32	max_nx_ipend_tstamp_acc;
static UINT32	max_tx_intr_pending_cnt;
static UINT32	max_tx_ipend_tstamp_acc;

static void tx_disable_callback(void)
{
	tx_intr_pending_cnt++;
	tx_ipend_tstamp_start = da16x_btm_get_timestamp();
}

static void tx_restore_callback(void)
{
	tx_ipend_tstamp_end = da16x_btm_get_timestamp();
	tx_ipend_tstamp_acc += (tx_ipend_tstamp_end-tx_ipend_tstamp_start);
}

#ifdef	SUPPORT_FCI_NX_CUSTOMIZE
static void nx_disable_callback(void)
{
	nx_intr_pending_cnt++;
	nx_ipend_tstamp_start = da16x_btm_get_timestamp();
}

static void nx_restore_callback(void)
{
	nx_ipend_tstamp_end = da16x_btm_get_timestamp();
	nx_ipend_tstamp_acc += (nx_ipend_tstamp_end-nx_ipend_tstamp_start);
}
#endif	//SUPPORT_FCI_NX_CUSTOMIZE

static void display_txnx_statistics(UINT32 index)
{
	UINT32	*statistics;
	UINT32	*txnxstatistics;
	UINT32	scaler;

	statistics = schd_sys_fsm->statistics[index];
	txnxstatistics = schd_sys_fsm->txnxstatistics[index];

	scaler = ((50*BTMSCALE)<<13)/(statistics[0]>>(17));

	PRINTF("TX.lout (pend %8d, max %8d) = %d ticks [" BTMDISPLAY "], max %d [" BTMDISPLAY "]\n"
		, txnxstatistics[3], txnxstatistics[8]
		, txnxstatistics[4]
		, (((txnxstatistics[4]>>16)*scaler)>>13)
		, txnxstatistics[9]
		, (((txnxstatistics[9]>>16)*scaler)>>13)
		);
	PRINTF("NX.lout (pend %8d, max %8d) = %d ticks [" BTMDISPLAY "], max %d [" BTMDISPLAY "]\n"
		//(pkt %d, max %d) :: txnxstatistics[0],  txnxstatistics[5]
		, txnxstatistics[1], txnxstatistics[6]
		, txnxstatistics[2]
		, (((txnxstatistics[2]>>16)*scaler)>>13)
		, txnxstatistics[7]
		, (((txnxstatistics[7]>>16)*scaler)>>13)
		);

#define DIALOG_MINFO_TYPE	size_t
#define STRUCT_MALLINFO_DECLARED 1

	struct dialog_minfo_type {
		/* non-mmapped space allocated from system */
		DIALOG_MINFO_TYPE arena;

		/* number of free chunks */
		DIALOG_MINFO_TYPE ord_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE sm_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE hb_lks;

		/* space in mmapped regions */
		DIALOG_MINFO_TYPE hb_lkhd;

		/* maximum total allocated space */
		DIALOG_MINFO_TYPE usm_blks;

		/* always 0 */
		DIALOG_MINFO_TYPE fsm_blks;

		/* total allocated space */
		DIALOG_MINFO_TYPE uord_blks;

		/* total free space */
		DIALOG_MINFO_TYPE ford_blks;

		/* releasable (via malloc_trim) space */
		DIALOG_MINFO_TYPE keep_cost;
	};
	extern struct dialog_minfo_type __iar_dlmallinfo(void);

	struct dialog_minfo_type m;
	m = __iar_dlmallinfo();
	PRINTF("total alloc space = %u\n", m.uord_blks);
	PRINTF("total free  space = %u\n", m.ford_blks);
	PRINTF("asigned space = %u\n", HEAP_MEM_SIZE());
}

static void dump_txnx_statistics(UINT32 *data, UINT32 mode)
{
	UINT32 intrbkup = 0;

	data[0] = nx_packet_txrx_cnt;
	data[1] = nx_intr_pending_cnt;
	data[2] = nx_ipend_tstamp_acc;
	data[3] = tx_intr_pending_cnt;
	data[4] = tx_ipend_tstamp_acc;
	data[5] = max_nx_packet_txrx_cnt;
	data[6] = max_nx_intr_pending_cnt;
	data[7] = max_nx_ipend_tstamp_acc;
	data[8] = max_tx_intr_pending_cnt;
	data[9] = max_tx_ipend_tstamp_acc;

	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);

	da16x_btm_control(FALSE);

	if ( mode == TRUE ){
		//if ( max_nx_packet_txrx_cnt < nx_packet_txrx_cnt )
		if ( max_nx_intr_pending_cnt < nx_intr_pending_cnt ){
			max_nx_packet_txrx_cnt = nx_packet_txrx_cnt;
			max_nx_intr_pending_cnt = nx_intr_pending_cnt;
			max_nx_ipend_tstamp_acc = nx_ipend_tstamp_acc;
		}
		if ( max_tx_intr_pending_cnt < tx_intr_pending_cnt ){
			max_tx_intr_pending_cnt = tx_intr_pending_cnt;
			max_tx_ipend_tstamp_acc = tx_ipend_tstamp_acc;
		}
	}else{
		max_nx_packet_txrx_cnt = 0;
		max_nx_intr_pending_cnt = 0;
		max_nx_ipend_tstamp_acc = 0;
		max_tx_intr_pending_cnt = 0;
		max_tx_ipend_tstamp_acc = 0;
	}

	nx_packet_txrx_cnt = 0;
	nx_intr_pending_cnt = 0;
	nx_ipend_tstamp_start = 0;
	nx_ipend_tstamp_end = 0;
	nx_ipend_tstamp_acc = 0;

	tx_intr_pending_cnt = 0;
	tx_ipend_tstamp_start = 0;
	tx_ipend_tstamp_end = 0;
	tx_ipend_tstamp_acc = 0;

	da16x_btm_control(TRUE);

	PREVENT_INTERRUPTS(intrbkup);
}

#endif /*SUPPORT_FCI_TX_MONITOR*/


/* EOF */
