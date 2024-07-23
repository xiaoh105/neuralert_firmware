/**
 ****************************************************************************************
 *
 * @file schd_trace.c
 *
 * @brief Trace Scheduler
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
#include <stdarg.h>

#include "driver.h"
#include "hal.h"
#include "console.h"

#include "da16x_system.h"
#include "portmacro.h"

#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#undef	TRACE_ERROR_INFO
#define	TRACE_CONBUF_INFO
#define	TRACE_SUPPORT_ONTHEFLY
#define SUPPORT_DA16X_VSPRINTF
#undef	FEATURE_USE_SEMAPHORE

#ifdef	SUPPORT_DA16X_VSPRINTF
#define	TRC_SNPRINT(buf,n,...)			da16x_snprintf_lfeed(buf, n, CON_DEFAULT_LFEED, __VA_ARGS__ )
#define	TRC_VSNPRINT(buf,n,...)			da16x_vsnprintf(buf, n, CON_DEFAULT_LFEED, __VA_ARGS__ )
#define TRC_NLINE_MARK				"\r\n"
#define	TRC_NLINE_LEN(x)			(x+1)
#else	//SUPPORT_DA16X_VSPRINTF
#define	TRC_SNPRINT(buf,n,...)			snprintf(buf, n, __VA_ARGS__ )
#define	TRC_VSNPRINT(buf,n,...)			vsnprintf(buf, n, __VA_ARGS__ )
#define TRC_NLINE_MARK				"\n"
#define	TRC_NLINE_LEN(x)			(x)
#endif	//SUPPORT_DA16X_VSPRINTF

#define	BUILD_OPT_DA16200_TRACE

/******************************************************************************
 *
 *  static variables
 *
 ******************************************************************************/

TRC_FSM_TYPE	*trc_fsm;

/******************************************************************************
 *
 *  local functions
 *
 ******************************************************************************/

#ifdef	BUILD_OPT_DA16200_TRACE

static int  trc_schduler_debug(TRC_FSM_TYPE *fsm, VOID *param);
static int  trc_schduler_sync(TRC_FSM_TYPE *fsm, VOID *param);
static int  trc_schduler_putc(TRC_FSM_TYPE *fsm, VOID *param);

static void trc_schd_on(TRC_FSM_TYPE *fsm, void* param);
static void trc_schd_off(TRC_FSM_TYPE *fsm, void* param);
static void trc_schd_conmode(TRC_FSM_TYPE *fsm, void* param);
static int  trc_que_insert(TRC_FSM_TYPE *fsm, TRC_MSG_TYPE *msg);
static TRC_MSG_TYPE *trc_que_get_msg(TRC_FSM_TYPE *fsm);
static int  trc_que_free_msg(TRC_FSM_TYPE *fsm, TRC_MSG_TYPE *msg);
static void trc_poll_timer_callback( TimerHandle_t expired_timer_id );

static int  trace_print_hexa(TRC_FSM_TYPE *fsm, char *tmpbuf, UINT8 *data, UINT16 length);
static void trace_scheduler_print_msg(TRC_FSM_TYPE *fsm);

static void trc_fast_putstring(TRC_FSM_TYPE *fsm, char *str, int len);

/******************************************************************************
 *  trace_scheduler()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void    trace_internal_scheduler(TRC_FSM_TYPE	*fsm, void *thread_input)
{
	DA16X_UNUSED_ARG(thread_input);

	int sys_wdog_id = da16x_sys_wdog_id_get_Console_OUT();
	TRC_EVT_TYPE 	event;
	int		status;
	//EventBits_t	uxBits;

	fsm->sema = xSemaphoreCreateMutex();
	if( fsm->sema == NULL ){
		TRC_DEBUG("%s:semaphore create error\n", __func__);
	}

	/* event queue */
	fsm->queuebuf = (UINT8 *)pvPortMalloc((sizeof(TRC_EVT_TYPE)*TRC_MAX_EVENT));
	fsm->queue = xQueueCreate(TRC_MAX_DBGPRINT, sizeof(TRC_EVT_TYPE));
	if (fsm->queue == NULL) {
		TRC_DEBUG(" [%s] queue create error\n", __func__);
	}
	else {
    	//Printf(" [%s] TRC_MAX_DBGPRINT Q create \r\n", __func__);
	}

#ifdef	TRACE_CONBUF_INFO
	/* free check event */
	fsm->usagechk = xEventGroupCreate();
#endif	//TRACE_CONBUF_INFO

	/* message pool */

	/* ?????? */
	//fsm->poolbuf = (UINT8 *)pvPortMalloc(sizeof(TRC_MSG_TYPE)*TRC_POOL_SIZE);
	//memset(fsm->poolbuf, 0, sizeof(TRC_MSG_TYPE)*TRC_POOL_SIZE);		// Last option Issue
	//fsm->pool = fsm->poolbuf;	/* ?????? F_F_S */

	fsm->logpool.max = TRC_QUE_DBGLOG_MAX;
	fsm->logpool.front = 0;
	fsm->logpool.rear  = 0;
	fsm->logpool.used  = 0;

	/* Now service is launched !! */
	event.param = NULL;
	trc_schd_on( fsm, event.param );

	fsm->conmode = TRACE_MODE_ECHO;

	while(1) {
		da16x_sys_watchdog_notify(sys_wdog_id);

		da16x_sys_watchdog_suspend(sys_wdog_id);

		status = xQueueReceive(fsm->queue, &event, portMAX_DELAY);

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

		if( status != pdPASS ) {
			continue;
		}

		da16x_sys_watchdog_suspend(sys_wdog_id);

		switch(event.cmd){
			case EVT_REQ_MON_ON:
				trc_schd_on( fsm, event.param );
				break;
			case EVT_REQ_MON_OFF:
				trc_schd_off( fsm, event.param );
				break;
			case EVT_TSCHD_RESET:
				break;
			case EVT_REQ_CONMODE:
				trc_schd_conmode( fsm, event.param );
				break;

			case EVT_TSCHD_LOG:
				if( event.param != NULL ){
					TRC_MSG_TYPE  *dmlog;

					dmlog = (TRC_MSG_TYPE *)(event.param) ;
					trc_que_insert(fsm,  dmlog);
				}
				break;
			case EVT_TSCHD_DUMP:
				{
					UINT32	mode;
					mode = ((UINT32)event.param);

					switch( (mode & TRACE_CMD_MASK) ){
					case	TRACE_CMD_MODE:
						fsm->mode  = (mode & 0x0FF);
						break;
					case	TRACE_CMD_STATE:
						fsm->state = (mode & 0x0FF);
						break;
					case	TRACE_CMD_LEVEL:
						fsm->level = (mode & 0x0FF);
						break;
					}

					if( fsm->state == FALSE ){
						xTimerStop(fsm->timer, 0);
					}else{
						xTimerStart(fsm->timer, 0);
					}
				}
				break;

			case EVT_TSCHD_PUTC:
				trc_schduler_putc( fsm, event.param );
				break;
			case EVT_TSCHD_PRINT:
				xTimerStop(fsm->timer, 0);
				trace_scheduler_print_msg(trc_fsm);
				xTimerStart(fsm->timer, 0);
				break;


			case EVT_REQ_SYNC:
				trc_schduler_sync( fsm, event.param );
				break;

			case EVT_REQ_DEBUG:
				trc_schduler_debug( fsm, event.param );
				break;

			default:
				TRC_DEBUG("DM:%d (%d, %p)\r\n", event.cmd, event.from, event.param );
				break;
		}

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
	}
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void	trc_fast_putstring(TRC_FSM_TYPE *fsm, char *str, int len)
{
	if(
	   console_hidden_check( ) != FALSE
	   && fsm != NULL
	   && ((fsm->conmode & TRACE_MODE_ECHO) == TRACE_MODE_ECHO )
	)
	{
#ifdef	SUPPORT_DA16X_VSPRINTF
		da16x_putstring( str, len );
#else	//SUPPORT_DA16X_VSPRINTF
		int i;

		for(i = 0; i < len; i++)
		{
			putchar(str[i]);
		}
#endif	//SUPPORT_DA16X_VSPRINTF
	}
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void	trc_schd_on(TRC_FSM_TYPE *fsm, void* param)
{
	DA16X_UNUSED_ARG(fsm);
	DA16X_UNUSED_ARG(param);
}

static void	trc_schd_off(TRC_FSM_TYPE *fsm, void* param)
{
	DA16X_UNUSED_ARG(fsm);
	DA16X_UNUSED_ARG(param);
}

static void	trc_schd_conmode(TRC_FSM_TYPE *fsm, void* param)
{
	UINT32	conmode ;

	conmode = (UINT32)param;

	if((conmode & TRACE_MODE_SET) == TRACE_MODE_SET){
		fsm->conmode = TRACE_MODE_MASK & conmode;
	}else
	if((conmode & TRACE_MODE_ON) == TRACE_MODE_ON){
		fsm->conmode |= TRACE_MODE_MASK & conmode;
	}
	else{
		fsm->conmode &= ~(TRACE_MODE_MASK & conmode);
	}
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int trc_schduler_debug(TRC_FSM_TYPE *fsm, VOID *param)
{
	DA16X_UNUSED_ARG(param);

	UINT32 len;

#ifdef	TRACE_ERROR_INFO
	// Skip
#else	//TRACE_ERROR_INFO
	len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
			, "TRC::dump error - %d\n"
			, fsm->dumperror.current );
	trc_fast_putstring(fsm, fsm->print_vbuf, len );

	if( fsm->dumperror.previous != fsm->dumperror.current ){
		fsm->dumperror.previous = fsm->dumperror.current;
	}

	len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
			, "TRC::print error - %d\n"
			, fsm->printerror.current );
	trc_fast_putstring(fsm, fsm->print_vbuf, len );

	if( fsm->printerror.previous != fsm->printerror.current ){
		fsm->printerror.previous = fsm->printerror.current;
	}

	len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
			, "TRC::queue error - %d\n"
			, fsm->queueerror.current );
	trc_fast_putstring(fsm, fsm->print_vbuf, len );

	if( fsm->queueerror.previous != fsm->queueerror.current ){
		fsm->queueerror.previous = fsm->queueerror.current;
	}
#endif	//TRACE_ERROR_INFO
        return TRUE;
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int trc_schduler_sync(TRC_FSM_TYPE *fsm, VOID *param)
{
	TRC_MSG_TYPE  *dmlog;
	dmlog = (TRC_MSG_TYPE *)(param) ;

	if(dmlog->msg == NULL ){
		trc_fast_putstring(fsm, "alloc error" TRC_NLINE_MARK, TRC_NLINE_LEN(12));
	}else{
		switch((dmlog->tag & TRC_TAG_TYPE)){
			case	TRC_TAG_TEXT:
				trc_fast_putstring(fsm, dmlog->msg, dmlog->len );
				break;
			case	TRC_TAG_DUMP:
				trc_fast_putstring(fsm, "\t\t", 2 );
				trace_print_hexa(fsm
						, fsm->print_vbuf
						, (UINT8 *)dmlog->msg
						, dmlog->len );
				trc_fast_putstring(fsm, TRC_NLINE_MARK, TRC_NLINE_LEN(1) );
				break;
		}

	}

	trc_que_free_msg( fsm, dmlog );

	return TRUE;
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int trc_schduler_putc(TRC_FSM_TYPE *fsm, VOID *param)
{
	char text;
	text = (char)((UINT32)param) ;

	if( text == '\r'){
		trc_fast_putstring(fsm, TRC_NLINE_MARK, TRC_NLINE_LEN(1) );
	}else{
		trc_fast_putstring(fsm, &text, 1);
	}
	return TRUE;
}

/******************************************************************************
 *
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	trc_que_insert(TRC_FSM_TYPE *fsm, TRC_MSG_TYPE *msg)
{
	unsigned int i;

	i = (fsm->logpool.front + 1) % fsm->logpool.max;

	if( i == fsm->logpool.rear ){
		// full case
		if( fsm->mode == TRUE ){
			// Last
			TRC_MSG_TYPE *oldmsg;

			oldmsg = fsm->logpool.list[i] ;
			fsm->logpool.list[i] = NULL; // Last option issue
			fsm->logpool.rear = (i + 1) % fsm->logpool.max;

			if(oldmsg != NULL){
				trc_que_free_msg( fsm, oldmsg);
			}
		}else{
			// First
			trc_que_free_msg( fsm, msg);
			return FALSE;
		}
	}

	i = fsm->logpool.front;
	fsm->logpool.list[i] = msg ;
	fsm->logpool.front = (i + 1) % fsm->logpool.max;

	return TRUE;
}

TRC_MSG_TYPE	*trc_que_get_msg(TRC_FSM_TYPE *fsm)
{
	int i;

	if( fsm->logpool.rear != fsm->logpool.front ){
		TRC_MSG_TYPE	*msg;

		i = fsm->logpool.rear ;

		msg = fsm->logpool.list[i];
		fsm->logpool.list[i] = NULL; // Last option issue
		fsm->logpool.rear = (i + 1) % fsm->logpool.max;

		return msg;
	}

	return NULL;
}

static int	trc_que_free_msg(TRC_FSM_TYPE *fsm, TRC_MSG_TYPE *msg)
{
	UINT16 tag = 0;

	if( msg->msg != NULL ){
        tag = msg->tag ;
#ifdef	TRACE_SUPPORT_ONTHEFLY
		if( (tag & TRC_TAG_FAST) == TRC_TAG_FAST ) {
			// experimental
			// Skip
		}else
#endif	//TRACE_SUPPORT_ONTHEFLY
		{
				vPortFree(msg->msg);
		}
#ifdef	TRACE_CONBUF_INFO
		xEventGroupSetBits(fsm->usagechk, 0x00000001);
#endif	//TRACE_CONBUF_INFO
	}

	vPortFree(msg);

	if((tag & TRC_TAG_MASK) != 0) {
		fsm->logpool.used = fsm->logpool.used - 1;
	}

	return TRUE;
}

/******************************************************************************
 *  trace_print_hexa()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	putascii(x) 	( (x)<32) ? ('.') : ( ((x)>126) ? ('.') : (x) )
#define	puthexa(x) 	( ((x)<10) ? (char)((x)+'0') : (char)((x)-10+'a') )

static  int trace_print_hexa(TRC_FSM_TYPE *fsm, char *tmpbuf, UINT8 *data, UINT16 length)
{
	int	i, len, count;
	char	dchr;
	char	*printscreen;
	char	*strlist, *hexalist;
	const	char *newline = TRC_NLINE_MARK "\t\t\t";

	len = 0;

	printscreen = tmpbuf;
	hexalist = &(printscreen[0]);
	strlist  = &(printscreen[16*3+4]);
	DRIVER_MEMSET(printscreen, ' ', 16*5 );

	count = 0;

	for(i=0; i<length; i++)
	{
		if((i % 16) == 0){
			if( i != 0 ){
				DRIVER_MEMCPY(&(strlist[16]), newline, TRC_NLINE_LEN(4));
				trc_fast_putstring(fsm, printscreen, ((16*4)+4+TRC_NLINE_LEN(4)));
				len = 0;
				count += ((16*4)+4+TRC_NLINE_LEN(4));
			}else{
				trc_fast_putstring(fsm, "\t", 1);
			}
		}
		dchr = data[i];
		strlist[(i%16)] = putascii( dchr );

		hexalist[len++] = puthexa( (dchr>>4) & 0x0f );
		hexalist[len++] = puthexa( (dchr>>0) & 0x0f );
		len++;
	}

	i = (i%16);

	if( i != 0 ){
		for( ; i < 16 ; i++ ){
			len = i * 3;
			strlist[i] = ' ';
			hexalist[len] = ' ';
			hexalist[len+1] = ' ';
			hexalist[len+2] = ' ';
		}
	}

	trc_fast_putstring(fsm, printscreen, ((16*4)+4));
	count += ((16*4)+4);

	return count;
}

/******************************************************************************
 *  trace_scheduler_print_msg()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void trace_scheduler_print_msg(TRC_FSM_TYPE *fsm)
{
	TRC_MSG_TYPE *prmsg;
	int	len, tag, count;

	if( fsm->state == FALSE ){
		return ;
	}

	count = 0;
	while( (prmsg = trc_que_get_msg( fsm )) != NULL ){

		tag = (prmsg->tag & TRC_TAG_MASK) ;

		if( tag > fsm->level ){
			trc_que_free_msg( fsm, prmsg );
			continue;
		}
		/* if msg is null */
		if( prmsg->msg == NULL ){
			len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
					, "LOG::%04X.%04X.%04X: alloc error\n"
					, (prmsg->time & 0x0FFFF)
					, (prmsg->stamp & 0x0FFFF)
					, prmsg->tag );

			trc_fast_putstring(fsm, fsm->print_vbuf, len );

			trc_que_free_msg( fsm, prmsg );
			continue;
		}
		/* if msg is not null */

		len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
				, "LOG::%04X.%04X.%04X: "
				, (prmsg->time & 0x0FFFF)
				, (prmsg->stamp & 0x0FFFF)
				, prmsg->tag );
		count += len;

		trc_fast_putstring(fsm, fsm->print_vbuf, len );
		switch((prmsg->tag & TRC_TAG_TYPE)){
			case	TRC_TAG_TEXT:
				trc_fast_putstring(fsm, prmsg->msg, prmsg->len );
				count += prmsg->len;
				break;
			case	TRC_TAG_DUMP:
				count += trace_print_hexa(fsm
						, fsm->print_vbuf
						, (UINT8 *)prmsg->msg
						, prmsg->len );
				break;
		}
		trc_fast_putstring(fsm, TRC_NLINE_MARK, TRC_NLINE_LEN(1) );

		trc_que_free_msg( fsm, prmsg );

		if( count >= TRC_LOOP_MAXSIZE ){
			break;
		}
	}
#ifdef	TRACE_ERROR_INFO
	if( fsm->dumperror.previous != fsm->dumperror.current ){

		len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
				, "ERR::dump error - %d\n", fsm->dumperror.current );
		trc_fast_putstring(fsm, fsm->print_vbuf, len );
		fsm->dumperror.previous = fsm->dumperror.current;

	}
	if( fsm->printerror.previous != fsm->printerror.current ){

		len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
				, "ERR::print error - %d\n", fsm->printerror.current );
		trc_fast_putstring(fsm, fsm->print_vbuf, len );
		fsm->printerror.previous = fsm->printerror.current;

	}
	if( fsm->queueerror.previous != fsm->queueerror.current ){

		len = TRC_SNPRINT( fsm->print_vbuf, TRC_MAX_DBGPRINT
				, "ERR::queue error - %d\n", fsm->queueerror.current );
		trc_fast_putstring(fsm, fsm->print_vbuf, len );
		fsm->queueerror.previous = fsm->queueerror.current;

	}
#endif	//TRACE_ERROR_INFO
}

/******************************************************************************
 *  trace_internal_que_proc_dump( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void 	trace_internal_que_proc_dump(TRC_FSM_TYPE *fsm, UINT16 tag, void *srcdata, UINT16 len)
{
#ifdef	BUILD_OPT_VSIM
	// Skip
#else	//BUILD_OPT_VSIM
	TRC_MSG_TYPE    *data;
	TRC_EVT_TYPE	event;
	UNSIGNED 	status, suspend;

	if( fsm == NULL || len == 0 ){
		return;
	}

	if((tag & TRC_TAG_MASK) != 0) {

		if( fsm->level  < (tag & TRC_TAG_MASK) )
		{	// drop
			return;
		}

		fsm->logstamp =  fsm->logstamp + 1;

		if( fsm->logpool.used >= fsm->logpool.max ){
			fsm->dumperror.current ++ ;
			return;
		}
	}

	/* ?????? F_F_S */
	suspend = 0;
	if (xSemaphoreTake(fsm->sema, suspend ) != pdTRUE) {
		fsm->dumperror.current ++ ;
		return;
	}

	data = pvPortMalloc(sizeof(TRC_MSG_TYPE));
	if (data == NULL ) 
	{

		if( (tag & TRC_TAG_MASK) == 0 ) {
			fsm->queueerror.current ++ ;
		}
		xSemaphoreGive(fsm->sema);
		return;
	}

	data->stamp = fsm->logstamp ;
	data->time = 0;	/* ?????? F_F_S */
	data->tag  = (((tag & TRC_TAG_TYPE) == TRC_TAG_DUMP) ? TRC_TAG_DUMP:TRC_TAG_TEXT)
		     | (tag & TRC_TAG_MASK);
 	data->len  = len;
	data->msg  = NULL;

#ifdef	TRACE_CONBUF_INFO
	if((tag & TRC_TAG_MASK) == 0 ){
#ifdef	TRACE_SUPPORT_ONTHEFLY
		if( (((tag & TRC_TAG_TYPE) == TRC_TAG_DUMP) || ((tag & TRC_TAG_TYPE) == TRC_TAG_TEXT))
			&& (len >= TRC_MAX_PRINT)  ){
			// experimental
			data->msg = srcdata;
			data->tag = TRC_TAG_FAST | data->tag ;
		}else
#endif	//TRACE_SUPPORT_ONTHEFLY
		{
			do{
				data->msg  = (char *)pvPortMalloc(TRC_ALIGN_SIZE(data->len));
				if( data->msg != NULL ){
					break;
				} else {
					vTaskDelay(1);
				}

    			xEventGroupWaitBits(fsm->usagechk, (0x00000001), pdTRUE, pdFALSE, suspend);
			} while( suspend == portMAX_DELAY );

			if( data->msg != NULL ){
				DRIVER_MEMCPY(data->msg, srcdata, (data->len) );
			}else{
			 	data->len  = 0;
			}
		}
	}else
#endif	//TRACE_CONBUF_INFO
	{
		data->msg  = (char *)pvPortMalloc(TRC_ALIGN_SIZE(data->len));

		if( data->msg != NULL ){
			DRIVER_MEMCPY(data->msg, srcdata, (data->len) );
		}else{
		 	data->len  = 0;
		}
	}

	event.from = 0;
	event.cmd  = ((tag & TRC_TAG_MASK) != 0) ? EVT_TSCHD_LOG : EVT_REQ_SYNC ;
	event.param = data;

    status = xQueueSend(fsm->queue, (VOID *)&event, suspend );
	if( status != pdPASS ){

		if( data->msg != NULL ){
#ifdef	TRACE_SUPPORT_ONTHEFLY
			if( (data->tag & TRC_TAG_FAST) == TRC_TAG_FAST ){
				// experimental
				// skip
			}else
#endif	//TRACE_SUPPORT_ONTHEFLY
			{
				vPortFree(data->msg);
			}
#ifdef	TRACE_CONBUF_INFO
			if((tag & TRC_TAG_MASK) == 0 ){
				xEventGroupSetBits(fsm->usagechk, 0x00000001);
			}
#endif	//TRACE_CONBUF_INFO
		}

		vPortFree(data);
		if( (tag & TRC_TAG_MASK) == 0 ) {
			fsm->queueerror.current ++ ;
		}
	}

	if((tag & TRC_TAG_MASK) != 0) {
		fsm->logpool.used = fsm->logpool.used + 1;
	}

	xSemaphoreGive(fsm->sema);
#endif	//BUILD_OPT_VSIM
}

/******************************************************************************
 *  trace_internal_que_proc_vprint( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	trace_internal_que_proc_vprint(TRC_FSM_TYPE *fsm, UINT16 tag, const char *format, va_list arg)
{
#ifdef	BUILD_OPT_VSIM
	// Skip
#else	//BUILD_OPT_VSIM
	TRC_MSG_TYPE    *data;
	TRC_EVT_TYPE	event;
	UNSIGNED 	status, suspend;

	if( fsm == NULL ){
		if(
			//(OAL_GET_KERNEL_STATE()== TRUE) && /* ?????? F_F_S */
			((tag & TRC_TAG_MASK) == 0)){
			TRC_VPRINT( format, arg );
		}
		return;
	}

	if((tag & TRC_TAG_MASK) != 0) {

		if( fsm->level  < (tag & TRC_TAG_MASK) )
		{	// drop
			return;
		}

		fsm->logstamp =  fsm->logstamp + 1;

		if( fsm->logpool.used >= fsm->logpool.max ){
			fsm->dumperror.current ++ ;
			return;
		}
	}

	suspend = 30 ;	/* F_F_S */
	if (xSemaphoreTake(fsm->sema, suspend ) != pdTRUE) {
		fsm->printerror.current ++ ;
		return;
	}

	data = pvPortMalloc(sizeof(TRC_MSG_TYPE));
	if (data == NULL ) 
	{
		if( tag == 0 ) {
			fsm->queueerror.current ++ ;
		}
		xSemaphoreGive(fsm->sema);
		return;
	}

	data->stamp = fsm->logstamp ;
	data->time = 0;	/* ?????? F_F_S */
	data->tag  = TRC_TAG_TEXT | (tag & TRC_TAG_MASK);
	memset(fsm->work_vbuf, 0, TRC_MAX_PRINT);
 	data->len  = (UINT16)TRC_VSNPRINT(fsm->work_vbuf, TRC_MAX_PRINT, format, arg);
	data->msg  = NULL;

#ifdef	TRACE_CONBUF_INFO
	if( data->len > fsm->maxusage ){
		fsm->maxusage = data->len ;
	}

	do {
		data->msg = (char *)pvPortMalloc(TRC_ALIGN_SIZE(data->len + 1));
		if( data->msg != NULL ){
			break;
		} else
		{
			vTaskDelay(1);
		}

    	xEventGroupWaitBits(fsm->usagechk, (0x00000001), pdTRUE, pdFALSE, suspend);
	} while( suspend == portMAX_DELAY );
#else	//TRACE_CONBUF_INFO
	data->msg = (UINT8 *)pvPortMalloc(TRC_ALIGN_SIZE(data->len + 1));
#endif	//TRACE_CONBUF_INFO

	if( data->msg != NULL ){
		DRIVER_MEMCPY(data->msg, fsm->work_vbuf, (data->len) );
	}else{
	 	data->len  = 0;
	}

	event.from = 0;
	event.cmd  = ((tag & TRC_TAG_MASK) != 0) ? EVT_TSCHD_LOG : EVT_REQ_SYNC ;
	event.param = data;

    status = xQueueSend(fsm->queue, (VOID *)&event, suspend );
	if( status != pdPASS ){

		if( data->msg != NULL ){
			vPortFree(data->msg);
#ifdef	TRACE_CONBUF_INFO
			xEventGroupSetBits(fsm->usagechk, 0x00000001);
#endif	//TRACE_CONBUF_INFO
		}
		vPortFree(data);
		if( tag == 0 ) {
			fsm->printerror.current ++ ;
		}
	}

	if((tag & TRC_TAG_MASK) != 0) {
		fsm->logpool.used = fsm->logpool.used + 1;
	}

	xSemaphoreGive(fsm->sema);

#endif //BUILD_OPT_VSIM
}

#else //BUILD_OPT_DA16200_ROMALL

static void trc_poll_timer_callback( UNSIGNED expired_timer_id );

extern void	trace_internal_scheduler(TRC_FSM_TYPE	*fsm, ULONG thread_input);
extern void	trace_internal_que_proc_dump(TRC_FSM_TYPE *fsm, UINT16 tag, void *srcdata, UINT16 len);
extern void	trace_internal_que_proc_vprint(TRC_FSM_TYPE *fsm, UINT16 tag, const char *format, va_list arg);

#endif //BUILD_OPT_DA16200_ROMALL

/******************************************************************************
 *  trace_scheduler()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
TRC_FSM_TYPE trc_fsm_type;
void trace_scheduler(void* thread_input)
{
	int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;
	TRC_FSM_TYPE	*tmpfsm;

    //Printf(" [%s] Started \r\n", __func__);
	
	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
	if (sys_wdog_id >= 0) {
		da16x_sys_wdog_id_set_Console_OUT(sys_wdog_id);
	}

	tmpfsm = &trc_fsm_type;

	trc_fsm = tmpfsm;
	memset(tmpfsm, 0x00, sizeof(TRC_FSM_TYPE));

	tmpfsm->timer = xTimerCreate("trctick",	10, pdTRUE, (void *)0, trc_poll_timer_callback);
	if (tmpfsm->timer == NULL) {
		// error	
	}

	trace_internal_scheduler(tmpfsm, thread_input);

	da16x_sys_watchdog_unregister(sys_wdog_id);

	vTaskDelete(NULL);
}

/******************************************************************************
 *  trc_poll_timer_callback()
 *
 *  Purpose :   callback
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void trc_poll_timer_callback( TimerHandle_t expired_timer_id )
{
	DA16X_UNUSED_ARG(expired_timer_id);

	static TRC_EVT_TYPE	event;

	event.from  = 0;
	event.cmd   = EVT_TSCHD_PRINT;
	event.param = NULL;

	if (xQueueSend(trc_fsm->queue, (VOID *)&event, portMAX_DELAY) != pdPASS) {
		TRC_DEBUG(" [%s] error \n", __func__);
	}
}

/******************************************************************************
 *  trc_que_proc_dump( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void 	trc_que_proc_dump(UINT16 tag, void *srcdata, UINT16 len)
{
#ifdef	BUILD_OPT_VSIM
	// Skip
#else	//BUILD_OPT_VSIM
	trace_internal_que_proc_dump(trc_fsm, tag, srcdata, len);
#endif	//BUILD_OPT_VSIM
}


/******************************************************************************
 *  trc_que_proc_dump( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	trc_que_proc_text(UINT16 tag, void *srcdata, UINT16 len)
{
	trc_que_proc_dump((TRC_TAG_TEXT|tag), srcdata, len );
}

/******************************************************************************
 *  trc_que_proc_vprint( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	trc_que_proc_vprint(UINT16 tag, const char *format, va_list arg)
{
#ifdef	BUILD_OPT_VSIM
	if( ((tag&TRC_TAG_MASK) >= 1) && ((tag&TRC_TAG_MASK) <= 5) ){
		DBGT_VPrintf(format, arg);
		DBGT_Printf("\n");
	}
#else	//BUILD_OPT_VSIM
	trace_internal_que_proc_vprint(trc_fsm, tag, format, arg);
#endif //BUILD_OPT_VSIM
}

/******************************************************************************
 *  trc_que_proc_print( )
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#if defined (__SUPPORT_CONSOLE_PWD__) && defined (XIP_CACHE_BOOT)
extern  unsigned char passwd_chk_flag;
extern  int     checkPasswordTimeout();
extern  UCHAR password_ok;
#endif // __SUPPORT_CONSOLE_PWD__ && XIP_CACHE_BOOT

void 	trc_que_proc_print(UINT16 tag, const char *fmt,...)
{
#ifdef	BUILD_OPT_VSIM
	// Skip
#else	//BUILD_OPT_VSIM
	va_list ap;

#if defined (__SUPPORT_CONSOLE_PWD__) && defined (XIP_CACHE_BOOT)
	/* Check Console password service */
	if (passwd_chk_flag == pdTRUE) {
		if (password_ok == pdFALSE) {
			return;
		}

		if (checkPasswordTimeout()) {   /* Occur timeout */
			password_ok = pdFALSE;
		}
	}
#endif // __SUPPORT_CONSOLE_PWD__ && XIP_CACHE_BOOT

	va_start(ap,fmt);

	trc_que_proc_vprint(tag, fmt, ap);

	va_end(ap);
#endif //BUILD_OPT_VSIM
}

//for UART1 STOP PING
extern HANDLE uart1;
int trc_que_proc_getchar_debug(UINT32 mode)
{
	char text;
	int temp ;
	UINT status;

	if (mode == 0) {
		temp = FALSE;
		text = '\0';
		UART_IOCTL(uart1, UART_SET_RX_SUSPEND, &temp);
		UART_READ(uart1, &text, sizeof(char));
		temp = TRUE;
		UART_IOCTL(uart1, UART_SET_RX_SUSPEND, &temp);
	} else {
		UART_READ(uart1, &text, sizeof(char));
	}

	if (
		//OAL_GET_KERNEL_STATE() == TRUE && 	/* ?????? F_F_S */
		!(text > 0x01 && text < 0x04)) { /* Console ¿¡¼­ Ctrl+C ¶Ç´Â Ctrl+B ?Ô·Â½Ã ASCII CODE È­¸é Ãâ·Â ¸·?½ */
#ifdef FEATURE_USE_SEMAPHORE
		TRC_EVT_TYPE	event;

		event.from  = 0;
		event.cmd   = EVT_TSCHD_PUTC;
		event.param = (VOID *)text;
#else
		TRC_EVT_TYPE	*event;
		event = (TRC_EVT_TYPE *)APP_MALLOC( TRC_ALIGN_SIZE(sizeof(TRC_EVT_TYPE)));

		if(event == NULL)
		{
			return (int)text;
		}

		event->from	= 0;
		event->cmd	= EVT_TSCHD_PUTC;
		event->param = (VOID *)((UINT32)text);
#endif

    	status = xQueueSend(trc_fsm->queue, (VOID *)&event, 0);
#ifndef FEATURE_USE_SEMAPHORE
		if (status != pdPASS) {
			vPortFree(event);
		}
#endif /* FEATURE_USE_SEMAPHORE */
	}

	return (int)text;
}

/******************************************************************************
 *  trc_que_proc_getchar()
 *
 *  Purpose :   Primitives
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
char trc_que_proc_getchar(UINT32 mode)
{
	char text;
	int temp ;

	if ( trc_fsm == NULL ){
		return 0;
	}

	if (mode == 0) {
		temp = FALSE;
	} else {
		temp = TRUE;
	}

#ifdef	BUILD_OPT_DA16200_ROMALL
	text = (char)getrawchar(temp);
#else	//BUILD_OPT_DA16200_ROMALL
	{
		UINT32	flag = FALSE;
		UART_IOCTL(da16x_console_handler(), UART_GET_RX_SUSPEND, &flag); // backup
		UART_IOCTL(da16x_console_handler(), UART_SET_RX_SUSPEND, &temp);
		if( UART_READ(da16x_console_handler(), &text, sizeof(char)) == 0 )
		{
			text = 0x00;
		}
		UART_IOCTL(da16x_console_handler(), UART_SET_RX_SUSPEND, &flag); // restore
	}
#endif	//BUILD_OPT_DA16200_ROMALL

	if (
		//OAL_GET_KERNEL_STATE() == TRUE &&	/* ?????? F_F_S */
		((trc_fsm->conmode & TRACE_MODE_ECHO) == TRACE_MODE_ECHO)
		/* No event send in case of GETC_NOWAIT() because of text is 0x00 */
		&& (!(   (CHAR)text >= 0x00 && text <= 0x1F) // Echo off when Control character input
		      || text == 0x08		/* 08 Ctrl-H BS */
		      || text == 0x0A		/* 10 Ctrl-J LF */
		      || text == 0x0D		/* 13 Ctrl-M CR */
		      || text == 0x1B		/* 27 Ctrl-[ ESC */
		   )
		)
	{
		TRC_EVT_TYPE	event;

		event.from  = 0;
		event.cmd   = EVT_TSCHD_PUTC;
		if((trc_fsm->conmode & TRACE_MODE_PASSWD) == TRACE_MODE_PASSWD){
			event.param = (VOID *)'*';
		}else{
			event.param = (VOID *)((UINT32)text);
		}

		xQueueSend(trc_fsm->queue, (VOID *)&event , mode);
	}

	return text;
}

/******************************************************************************
 *  trc_que_proc_active()
 *
 *  Purpose :   callback
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	trc_que_proc_active( UINT32 mode )
{
	if( trc_fsm != NULL ){
		TRC_EVT_TYPE	event;

		event.from  = 0;
		if( (mode & TRACE_CMD_MASK) == TRACE_CMD_DEBUG ){
			event.cmd   = EVT_REQ_DEBUG;
		}else{
			event.cmd   = EVT_TSCHD_DUMP;
		}
		event.param = (VOID *)mode;

		xQueueSend(trc_fsm->queue, (VOID *)&event, portMAX_DELAY );
	}
}

/******************************************************************************
 *  trc_que_proc_conmode()
 *
 *  Purpose :   callback
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	trc_que_proc_conmode( UINT32 mode )
{
	if( trc_fsm != NULL ){
		TRC_EVT_TYPE	event;

		event.from  = 0;
		event.cmd   = EVT_REQ_CONMODE;
		event.param = (VOID *)mode;

		xQueueSend(trc_fsm->queue, (VOID *)&event, portMAX_DELAY );
	}
}

/******************************************************************************
 *  trc_que_proc_getusage()
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	trc_que_proc_getusage(void)
{
#ifdef	TRACE_CONBUF_INFO
	if( trc_fsm != NULL ){
		return (int)(trc_fsm->maxusage) ;
	}
#endif	//TRACE_CONBUF_INFO
	return TRC_MAX_PRINT;
}

/* EOF */
