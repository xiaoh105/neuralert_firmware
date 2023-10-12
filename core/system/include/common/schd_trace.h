/**
 ****************************************************************************************
 *
 * @file schd_trace.h
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

#ifndef __schd_trace_h__
#define __schd_trace_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "oal.h"

/******************************************************************************
 *
 *  Definition
 *
 ******************************************************************************/

#define TRACE_CMD_MASK           0xF0000000
#define TRACE_CMD_DEBUG          0x80000000
#define TRACE_CMD_MODE           0x40000000
#define TRACE_CMD_STATE          0x20000000
#define TRACE_CMD_LEVEL          0x00000000


#define TRACE_MODE_ON            0x80000000
#define TRACE_MODE_OFF           0x00000000
#define TRACE_MODE_SET           0x40000000
#define TRACE_MODE_MASK          0x0FFFFFFF
#define TRACE_MODE_ECHO          0x08000000
#define TRACE_MODE_PASSWD        0x04000000


#undef    TRACE_ERROR_INFO
#define TRACE_CONBUF_INFO
#define TRACE_SUPPORT_ONTHEFLY

#define TRC_VPRINT(...)          {             \
                                    da16x_vprintf( __VA_ARGS__ );         \
                                    while(da16x_console_txempty() == FALSE);    \
                                 }
#define TRC_DEBUG(...)           Printf( __VA_ARGS__ )

#define TRC_MAX_PRINT            4096    /* Min 128 - Max 4096 */
#define TRC_MAX_DBGPRINT         32    /* Min 128 - Max 1024 */
#define TRC_QUE_LOG_MAX         (32+(1024/TRC_MAX_PRINT))
#define TRC_QUE_DBGLOG_MAX      (128)
#define TRC_MAX_EVENT           (TRC_QUE_LOG_MAX+TRC_QUE_DBGLOG_MAX)
#define TRC_POOL_SIZE           (TRC_MAX_EVENT+1)
#define TRC_TICK_TIME           5
#define TRC_LOOP_MAXSIZE        ((DA16X_BAUDRATE/400)*TRC_TICK_TIME) /* characters */

#define TRC_TAG_MASK            0x0fff
#define TRC_TAG_TYPE            0x7000
#define TRC_TAG_DUMP            0x0000
#define TRC_TAG_TEXT            0x4000
#define TRC_TAG_FAST            0x8000

#define TRC_ALIGN_SIZE(x)       (((x)+3)&(~0x3))

typedef struct {
    UINT32   stamp;    // log count
    UINT32   time;     // OAL_RETRIEVE_CLOCK( )
    UINT16   tag;      // type of msg
    UINT16   len;      // length of msg
    char     *msg;
} TRC_MSG_TYPE;

typedef struct {
    UINT32   previous;
    UINT32   current;
} TRC_ERR_TYPE;

typedef struct {
    UINT32   max;
    UINT32   front;
    UINT32   rear;
    UINT32   used;     // Log Flow Control
    TRC_MSG_TYPE    *list[TRC_QUE_DBGLOG_MAX];
} TRC_MSGPOOL_TYPE;


typedef     struct {
    UINT8    mode;
    UINT8    state;
    UINT16   level;
    UINT32   conmode;
    SemaphoreHandle_t   sema;
    QueueHandle_t       queue;    
    UINT8               *queuebuf;
    TimerHandle_t       timer;

    TRC_MSGPOOL_TYPE    logpool;

    TRC_ERR_TYPE        dumperror;
    TRC_ERR_TYPE        printerror;
    TRC_ERR_TYPE        queueerror;

    UINT32              logstamp;
#ifdef    TRACE_CONBUF_INFO
    UINT32              maxusage;
    EventGroupHandle_t  usagechk;
#endif    //TRACE_CONBUF_INFO

    char  print_vbuf[TRC_MAX_DBGPRINT];
    char  work_vbuf[TRC_MAX_PRINT];
} TRC_FSM_TYPE;

typedef struct {
    char *code;
    int  len;
} COLOR_SET_TYPE;

typedef enum {
    EVT_REQ_NOP     = 0,
    EVT_REQ_SYNC,

    EVT_REQ_MON_ON,
    EVT_REQ_MON_OFF,
    EVT_REQ_LOCAL,
    EVT_REQ_DEBUG,
    EVT_REQ_CONMODE,

    EVT_TSCHD_RESET,
    EVT_TSCHD_LOG,
    EVT_TSCHD_DUMP,
    EVT_TSCHD_PRINT,
    EVT_TSCHD_PUTC,

    EVT_REQ_MAX
} TRC_EVENT_REQ_LIST;

typedef  struct __obu_event__ {
    UINT16      from;
    UINT16      cmd;
    VOID        *param;
} TRC_EVT_TYPE;

/******************************************************************************
 *
 *  API
 *
 ******************************************************************************/

extern void trace_scheduler(void* thread_input);

extern void trc_que_proc_dump(UINT16 tag, void *srcdata, UINT16 len);
extern void trc_que_proc_text(UINT16 tag, void *srcdata, UINT16 len);
extern void trc_que_proc_vprint(UINT16 tag, const char *format, va_list arg);
extern void trc_que_proc_print(UINT16 tag, const char *fmt,...);
extern char trc_que_proc_getchar(UINT32 mode );
extern void trc_que_proc_active( UINT32 mode );
extern int  trc_que_proc_getusage(void);
extern void trc_que_proc_conmode( UINT32 mode );

#endif /* __schd_trace_h__ */

/* EOF */
