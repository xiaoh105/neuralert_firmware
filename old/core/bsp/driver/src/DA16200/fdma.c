/**
 ****************************************************************************************
 *
 * @file fdma.c
 *
 * @brief FDMA Driver
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
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "oal.h"
#include "driver.h"
#include "common_def.h"

#if 0//(dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define traceISR_ENTER()
#  define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

#define	SUPPORT_FDMA_CLOCKGATING

//---------------------------------------------------------
//
//---------------------------------------------------------

#define FDMA_TRACE_PRINT(...)	DRV_PRINTF(__VA_ARGS__)

#define FDMA_DRV_DBG_NONE(...)	//DRV_DBG_NONE( __VA_ARGS__ )
#define FDMA_DRV_DBG_BASE(...)	//DRV_DBG_BASE( __VA_ARGS__ )
#define FDMA_DRV_DBG_INFO(...)	//DRV_DBG_INFO( __VA_ARGS__ )
#define FDMA_DRV_DBG_WARN(...)	//DRV_DBG_WARN( __VA_ARGS__ )
#define FDMA_DRV_DBG_ERROR(...)	//DRV_DBG_ERROR( __VA_ARGS__ )
#define FDMA_DRV_DBG_DUMP(...)	//DRV_DBG_DUMP( __VA_ARGS__ )


#define	FDMA_IRQn		fDMA_IRQn
#define	DA16X_FDMA_BASE		FDMA_BASE_0

#ifdef	SUPPORT_FDMA_CLOCKGATING
#define	FDMA_CLKGATE_ON()	DA16X_CLKGATE_ON(clkoff_fdma)
#define	FDMA_CLKGATE_OFF()	DA16X_CLKGATE_OFF(clkoff_fdma)
#else	//SUPPORT_FDMA_CLOCKGATING
#define	FDMA_CLKGATE_ON()
#define	FDMA_CLKGATE_OFF()
#endif	//SUPPORT_FDMA_CLOCKGATING

//---------------------------------------------------------
//
//---------------------------------------------------------

ATTRIBUTE_NOINIT_DATA
static FDMA_HANDLER_TYPE _fdma_handler_;
ATTRIBUTE_NOINIT_DATA
static HANDLE	_fdma_init_handler[FDMA_UNIT_MAX];

static const UINT8 fdma_intr_count_list[16]
		/* 0000, 0001, 0010, 0011, 0100, 0101, 0110, 0111 */
		/* 1000, 1001, 1010, 1011, 1100, 1101, 1110, 1111 */
		= {   0,    1,    2,    2,    3,    3,    3,    3,
			  4,    4,    4,    4,    4,    4,    4,    4 };

//---------------------------------------------------------
//
//---------------------------------------------------------

// Control
#define	FDMA_SET_REQ_HOLD(x,v)		((x)->ReqHOLD) |= (((0x1<<(v)) & FDMA_CHAN_MASK)<<8)
#define	FDMA_UNSET_REQ_HOLD(x)		((x)->ReqHOLD) &= ~((FDMA_CHAN_MASK)<<8)
#define	FDMA_GET_REQ_HOLD(x)		((((x)->ReqHOLD)>>8) & FDMA_CHAN_MASK)
#define	FDMA_SET_REQ_CLR_OP(x,v)	((x)->ReqHOLD) |= (((v) & FDMA_CHAN_MASK))

#define	FDMA_SET_REQ_CLR_STA(x)		((x)->ReqIRQ) = ((0x01)<<8)
#define	FDMA_SET_REQ_CLR_ERR(x)		((x)->ReqIRQ) = ((0x01)<<5)
#define	FDMA_SET_REQ_CLR_HOLD(x)	((x)->ReqIRQ) = ((0x01)<<4)
#define	FDMA_SET_REQ_CLR_IRQ(x,v)	((x)->ReqIRQ) = (((v) & FDMA_CHAN_MASK))

#define	FDMA_SET_OP_EN(x)		((x)->ReqOP) |= (0x01)
#define	FDMA_SET_OP_DIS(x)		((x)->ReqOP) &= ~(0x01)
#define	FDMA_GET_OP_EN(x)		(((x)->ReqOP) & 0x01)
#define	FDMA_SET_IRQ_MASK(x,v)		((x)->ReqOP) |= ((v) & (0x3f<<16))

#define	FDMA_SET_ST_CH_EN(x,v)		((x)->ReqCH) |= (((1<<(v)) & FDMA_CHAN_MASK))
#define	FDMA_MAKE_ST_CH_EN(v)		((1<<(v)) & FDMA_CHAN_MASK)
#define	FDMA_PUSH_ST_CH_EN(x,v)		(((x)->ReqCH) |= v)
#define	FDMA_GET_ST_CH_EN(x)		(((x)->ReqCH) & FDMA_CHAN_MASK)

// Status
#define	FDMA_GET_ERROR_TYPE(x)		(((x)->Status) & (0x0f<<28))
#define	FDMA_ERROR_LENMISS		(1<<28)
#define	FDMA_ERROR_ADRMISS		(2<<28)
#define	FDMA_ERROR_AHB0ERR		(3<<28)
#define	FDMA_ERROR_AHB1ERR		(4<<28)

#define	FDMA_GET_OP_STATUS(x)		(((x)->Status) & (0x03<<24))
#define	FDMA_STATUS_IDLE		(0)
#define	FDMA_STATUS_RUN			(1<<24)
#define	FDMA_STATUS_HOLD		(2<<24)
#define	FDMA_STATUS_ERR			(3<<24)

#define	FDMA_GET_IRQ_STATUS(x)		(((x)->Status) & (0x3f<<16))
#define	FDMA_IRQ_ERROR			(1<<(16+5))
#define	FDMA_IRQ_HOLD			(1<<(16+4))
#define	FDMA_IRQ_CH4DONE		(1<<(16+3))
#define	FDMA_IRQ_CH3DONE		(1<<(16+2))
#define	FDMA_IRQ_CH2DONE		(1<<(16+1))
#define	FDMA_IRQ_CH1DONE		(1<<(16+0))

#define	CONVERT_CLR_IRQ_OFFSET(x)	(((x)>>16)&0x0f)

#define	FDMA_GET_NEXT_CH(x)		((((x)->Status)>> 8) & FDMA_CHAN_MASK)
#define	FDMA_GET_CURR_CH(x)		(((x)->Status) & FDMA_CHAN_MASK)

// CHLLISize

#define	FDMA_GET_CHLLISize(x)		((((x)->CHLLISize)&0x07)+(sizeof(UINT32)*4))
#define	FDMA_SET_CHLLISize(x,v)		((x)->CHLLISize = (((v)/sizeof(UINT32))-4) & 0x07)

// History

#define	FDMA_GET_History(x)		(((x)->History[0]) & 0x07)
#define	FDMA_HIS_IDLE			0
#define	FDMA_HIS_CH1			1
#define	FDMA_HIS_CH2			2
#define	FDMA_HIS_CH3			3
#define	FDMA_HIS_CH4			4

// Channel :: Config
// Channel :: Control
#define	FDMA_CHAN_NXTCH(x)		(((((x)%FDMA_CHAN_MAX)+FDMA_HIS_CH1)&0x07)<<24)
// Channel :: SrcAddr
// Channel :: DstAddr

// Channel :: LLIbase
#define	FDMA_LLIbase(x)			((x)&(~0x03))	/* 4 byte aligned */

// Channel :: LLInum
#define	FDMA_LLItsk_CN(x)		(((x)&0x0ffff)<<16)
#define	FDMA_LLItsk_TN(x)		(((x)&0x0ffff))
#define	FDMA_LLItsk_Get_CN(x)		(((x)>>16)&0x0ffff)
#define	FDMA_LLItsk_Get_TN(x)		(((x)&0x0ffff))

// Channel :: LLIindex
#define	FDMA_LLItsk_SI(x)		(((x)&0x0ffff))

// Channel :: LLIstatus
#define	FDMA_LLItsk_Status(x)		(((x)>>16)&0x0f)
#define	FDMA_LLItsk_CurIdx(x)		(((x)&0x0ffff))

//---------------------------------------------------------
//
//---------------------------------------------------------
static void     fdma_update_swtsk(FDMA_HANDLER_TYPE *fdma, FDMA_SWTSK_MAP *swtsk);

static int	fdma_isr_create(HANDLE handler);
static int	fdma_isr_init(HANDLE handler);
static int	fdma_isr_close(HANDLE handler);
static void	_lowlevel_fdma_interrupt0(void);
void 		_fdma_interrupt_unit0(VOID);
/******************************************************************************
 *  FDMA_SYS_INIT ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	FDMA_SYS_INIT(UINT32 mode)
{
	DRV_MEMSET(&_fdma_handler_, 0x00, sizeof(FDMA_HANDLER_TYPE));
	_fdma_init_handler[FDMA_UNIT_0] = &_fdma_handler_;
	_fdma_handler_.instance = 0xFFFFFFFF;

	FDMA_CREATE(FDMA_UNIT_0);
	FDMA_INIT( (HANDLE)(_fdma_init_handler[FDMA_UNIT_0]), mode );
}

void	FDMA_SYS_DEINIT(void)
{
	FDMA_CLOSE( (HANDLE)(_fdma_init_handler[FDMA_UNIT_0]) );
	_fdma_init_handler[0] = NULL;
}

HANDLE	FDMA_SYS_GET_HANDLER(void)
{
	return (HANDLE)(_fdma_init_handler[FDMA_UNIT_0]);
}

/******************************************************************************
 *  FDMA_CREATE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	FDMA_CREATE( UINT32 dev_id )
{
	FDMA_HANDLER_TYPE	*fdma;
	UINT32			i;

	if (dev_id >= FDMA_UNIT_MAX){
		return NULL;
	}

	// Address Mapping
	switch((FDMA_UNIT_LIST)dev_id){
		case	FDMA_UNIT_0:
			if ( _fdma_init_handler[FDMA_UNIT_0] == NULL ) {
				_fdma_init_handler[FDMA_UNIT_0] = (FDMA_HANDLER_TYPE *)pvPortMalloc(sizeof(FDMA_HANDLER_TYPE));

				if( _fdma_init_handler[FDMA_UNIT_0] == NULL ){
					return NULL;
				}

				fdma = _fdma_init_handler[FDMA_UNIT_0];
				DRV_MEMSET(fdma, 0, sizeof(FDMA_HANDLER_TYPE));
			}
			else {
				fdma = _fdma_init_handler[FDMA_UNIT_0];
			}

			fdma->dev_addr = DA16X_FDMA_BASE;
			/*fdma->instance = 0;*/
			fdma->control = (FDMA_CTRLREG_MAP *)(fdma->dev_addr + DA16X_FDMA_CTRLREG_BASE );
			for(i = 0; i < FDMA_CHAN_MAX; i++) {
				fdma->hwtask[i]  = (FDMA_TASK_MAP *)(fdma->dev_addr + DA16X_FDMA_CHANREG_BASE*(i+1) );
			}
			/*fdma->swchannum = 0;*/
			/*fdma->hwindex = 0;*/
			/*fdma->swindex = 0;*/
			fdma->instance ++;
			break;

		default:
			break;
	}

	if(fdma->instance == 0){
		FDMA_CLKGATE_ON();

		FDMA_SET_REQ_CLR_OP(fdma->control, FDMA_CHAN_MASK);
		FDMA_SET_REQ_CLR_STA(fdma->control); /* history */

		fdma->control->ReqHOLD = 0;
		fdma->control->ReqIRQ  = 0;
		fdma->control->ReqOP   = 0;
		fdma->control->ReqCH   = 0;
		FDMA_SET_CHLLISize(fdma->control, sizeof(FDMA_SWLLI_MAP));
		// Create ISR
		fdma_isr_create(fdma);

		FDMA_CLKGATE_OFF();
	}

	return (HANDLE) fdma;
}

/******************************************************************************
 *  FDMA_INIT ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_INIT (HANDLE handler, UINT32 mode)
{
	FDMA_HANDLER_TYPE	*fdma;

	if(handler == NULL){
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *)handler ;

	if (fdma->instance == 0) {
		fdma->mode = mode;

		if ( fdma->mode == TRUE ) {
			fdma->mutex = xSemaphoreCreateMutex();
			fdma->event = xEventGroupCreate();
		}

		FDMA_CLKGATE_ON();

		fdma_isr_init(fdma);

		FDMA_SET_OP_EN(fdma->control);

		FDMA_CLKGATE_OFF();
	}

	return TRUE;
}

/******************************************************************************
 *  FDMA_CLOSE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_CLOSE(HANDLE handler)
{
	FDMA_HANDLER_TYPE	*fdma;

	if(handler == NULL){
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *)handler ;

	if(fdma->instance == 0){
		FDMA_CLKGATE_ON();

		FDMA_SET_OP_DIS(fdma->control);

		fdma_isr_close(fdma);

		FDMA_CLKGATE_OFF();

		if ( fdma->mode == TRUE ) {
			vEventGroupDelete(fdma->event);
			vSemaphoreDelete (fdma->mutex);
			vPortFree(fdma);
		}
	}

	return TRUE;
}

/******************************************************************************
 *  FDMA_OBTAIN_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	FDMA_OBTAIN_CHANNEL(HANDLE handler, UINT32 chan_id,
				UINT32 control, UINT32 config, UINT32 maxlli)
{
	FDMA_SWTSK_MAP	*swtsk;

	if( maxlli > 0x0ffff ){
		return NULL;
	}

	swtsk = pvPortMalloc(sizeof(FDMA_SWTSK_MAP));

	if( swtsk != NULL ){
		swtsk->fdma = handler;

		swtsk->id = (UINT16)chan_id;
		swtsk->Config = config;
		swtsk->Control = control;

		swtsk->swptr = 0;
		swtsk->hwptr = 0;
		swtsk->max = (UINT16)(maxlli + 1);
		swtsk->lli = (FDMA_SWLLI_MAP *)pvPortMalloc(sizeof(FDMA_SWLLI_MAP)*(swtsk->max));

		swtsk->nxt = NULL;
	}

	return swtsk;
}

/******************************************************************************
 *  FDMA_CHANGE_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_CHANGE_CHANNEL(HANDLE channel,
				UINT32 control, UINT32 config)
{
	FDMA_SWTSK_MAP	*swtsk;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;

	swtsk->Config = config;
	swtsk->Control = control;

	return TRUE;
}

/******************************************************************************
 *  FDMA_LOCK_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_LOCK_CHANNEL(HANDLE channel)
{
	FDMA_SWTSK_MAP		*swtsk;
	FDMA_HANDLER_TYPE	*fdma;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;
	fdma = (FDMA_HANDLER_TYPE *)swtsk->fdma;

	if ( ( fdma->mode == TRUE ) && (xSemaphoreTake(fdma->mutex, portMAX_DELAY) != pdTRUE)) {
		return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 *  FDMA_UNLOCK_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_UNLOCK_CHANNEL(HANDLE channel)
{
	FDMA_SWTSK_MAP		*swtsk;
	FDMA_HANDLER_TYPE	*fdma;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;
	fdma = (FDMA_HANDLER_TYPE *)swtsk->fdma;

	if ( fdma->mode == TRUE ) {
		xSemaphoreGive(fdma->mutex);
	}

	return TRUE;
}

/******************************************************************************
 *  FDMA_SCATTER_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_SCATTER_CHANNEL(HANDLE channel,
				VOID *dst, VOID *src, UINT32 dlen
				, UINT32 callbk, UINT32 param)
{
	FDMA_SWTSK_MAP		*swtsk;
	FDMA_HANDLER_TYPE	*fdma;
	UINT32		idx;
	UINT32	intrbkup = 0;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;

	if( swtsk->max < 2 ){
		// what ?
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *)swtsk->fdma;

	if ( ((swtsk->swptr + 1) % swtsk->max) == swtsk->hwptr ) {
		xEventGroupWaitBits(fdma->event, (0x00000001), pdTRUE, pdFALSE, portMAX_DELAY);
	}

	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);

	idx = swtsk->swptr ;

	swtsk->lli[idx].lli.Config = swtsk->Config | FDMA_CHAN_TOTALEN(dlen);
	if( (USR_CALLBACK)callbk == NULL ){
		swtsk->lli[idx].lli.Control = swtsk->Control;
	}else{
		swtsk->lli[idx].lli.Control = swtsk->Control | FDMA_CHAN_IRQEN ;
	}
	swtsk->lli[idx].lli.SrcAddr = (UINT32)src;
	swtsk->lli[idx].lli.DstAddr = (UINT32)dst;
	swtsk->lli[idx].callback = (USR_CALLBACK)callbk;
	swtsk->lli[idx].param    = (HANDLE)param;

	swtsk->swptr = (UINT16)((idx + 1) % swtsk->max);

	PREVENT_INTERRUPTS(intrbkup);

	return TRUE;
}

/******************************************************************************
 *  FDMA_START_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_START_CHANNEL(HANDLE channel)
{
	FDMA_HANDLER_TYPE *fdma;
	FDMA_SWTSK_MAP	*swtsk;
	UINT32		idx, nxt, cnt, hwptr, first;
	UINT32	intrbkup = 0;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;

	if( swtsk->swptr == swtsk->hwptr ){
		return FALSE;
	}

	hwptr = swtsk->hwptr;
	fdma = (FDMA_HANDLER_TYPE *)(swtsk->fdma);

	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);

	FDMA_CLKGATE_ON();

	idx = FDMA_GET_History(fdma->control);
	//TRC: TEST_DBG_TRIGGER((0xFDC01110|(idx)));

	if(FDMA_GET_OP_STATUS(fdma->control) == FDMA_STATUS_IDLE){
		idx = FDMA_HIS_IDLE;
	}

	if( idx >= FDMA_HIS_CH1 && idx <= FDMA_HIS_CH4 ){
		// hold next channel
		idx = fdma->hwindex;
		nxt = fdma->swindex;
		cnt = ( idx > nxt ) ? (idx - nxt) : ( FDMA_CHAN_MAX - nxt + idx);
	}else{
		nxt = 0;
		fdma->swindex = 0;
		fdma->hwindex = 0;
		cnt = FDMA_CHAN_MAX;
	}

	FDMA_SET_REQ_HOLD(fdma->control, nxt ); // Hold
	//TRC: TEST_DBG_TRIGGER((0xFDC02220|(nxt)));

	if( (cnt < FDMA_CHAN_MAX) || (fdma->swchannum > 0) ){
		// num of DMA tasks is MAX !!!!!
		// Wait until event
		fdma_update_swtsk( fdma, swtsk );
		first = 0;
	}else{
		first = nxt;

		while( cnt > 0 ){
			UINT32	nxthwptr;

			cnt-- ;
			nxthwptr = (hwptr + 1) % swtsk->max;

			fdma->hwtask[nxt]->LLI.Config = swtsk->lli[hwptr].lli.Config;
			if( cnt == 0 || nxthwptr == swtsk->swptr ){
				fdma->hwtask[nxt]->LLI.Control = swtsk->lli[hwptr].lli.Control;
				fdma->hwtask[nxt]->LLI.Control |= FDMA_CHAN_IRQEN; /* last */
			}else{
				fdma->hwtask[nxt]->LLI.Control = swtsk->lli[hwptr].lli.Control;
				fdma->hwtask[nxt]->LLI.Control |= FDMA_CHAN_NXTCH(nxt+1) ;
			}
			fdma->hwtask[nxt]->LLI.SrcAddr = swtsk->lli[hwptr].lli.SrcAddr;
			fdma->hwtask[nxt]->LLI.DstAddr = swtsk->lli[hwptr].lli.DstAddr;

			fdma->hwtask[nxt]->LLIbase  = (UINT32)( &(swtsk->lli[hwptr]) );
			fdma->hwtask[nxt]->LLInum   = FDMA_LLItsk_CN(0)|FDMA_LLItsk_TN(0);
			fdma->hwtask[nxt]->LLIindex = 0;

			//TRC: TEST_DBG_TRIGGER((0xFDC00000|(nxt)));
			//TRC: TEST_DBG_TRIGGER(swtsk->lli[hwptr].lli.SrcAddr);
			//TRC: TEST_DBG_TRIGGER(swtsk->lli[hwptr].lli.DstAddr);

			nxt   = (nxt + 1) % FDMA_CHAN_MAX;
			hwptr = nxthwptr;
			if( hwptr == swtsk->swptr ){
				break;
			}
		}

		swtsk->hwptr = (UINT16)hwptr;
		fdma->swindex = nxt;

		// if LLI of current swtsk remains,
		if( swtsk->hwptr != swtsk->swptr ){
			fdma_update_swtsk( fdma, swtsk );
		}

		FDMA_SET_ST_CH_EN(fdma->control, first);
		//TRC: TEST_DBG_TRIGGER((0xFDC03330|(first)));
	}

	FDMA_UNSET_REQ_HOLD(fdma->control); // Release
	//TRC: TEST_DBG_TRIGGER(0xFDC04440);

	PREVENT_INTERRUPTS(intrbkup);

	return TRUE;
}

/******************************************************************************
 *  FDMA_STOP_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	FDMA_STOP_CHANNEL(HANDLE channel)
{
	if( channel == NULL ){
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 *  FDMA_CHECK_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	FDMA_CHECK_CHANNEL(HANDLE channel)
{
	FDMA_SWTSK_MAP	*swtsk;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;

	if( swtsk->swptr == swtsk->hwptr ){
		return TRUE;
	}

	return FALSE;
}

/******************************************************************************
 *  FDMA_RELEASE_CHANNEL ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	FDMA_RELEASE_CHANNEL(HANDLE handler, HANDLE channel )
{
	DA16X_UNUSED_ARG(handler);

	FDMA_SWTSK_MAP	*swtsk;

	if( channel == NULL ){
		return FALSE;
	}

	swtsk = (FDMA_SWTSK_MAP *)channel;

	if( swtsk->lli != NULL ){
		vPortFree(swtsk->lli);
	}
	vPortFree(swtsk);

	return TRUE;
}

/******************************************************************************
 *  fdma_update_swtsk ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static  void fdma_update_swtsk(FDMA_HANDLER_TYPE *fdma, FDMA_SWTSK_MAP *swtsk)
{
	switch(fdma->dev_addr){
		case	DA16X_FDMA_BASE:
			NVIC_DisableIRQ( FDMA_IRQn );
			break;
	}

	if( fdma->swchannum == 0 ){
		fdma->swchanhead = swtsk;
	}else{
		fdma->swchantail->nxt = swtsk;
	}
	fdma->swchantail = swtsk;
	swtsk->nxt = NULL;
	fdma->swchannum++;

	switch(fdma->dev_addr){
		case	DA16X_FDMA_BASE:
			NVIC_EnableIRQ( FDMA_IRQn );
			break;
	}


}

/******************************************************************************
 *  FDMA_PRINT_STATUS ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int 	FDMA_PRINT_STATUS(HANDLE handler)
{
	FDMA_HANDLER_TYPE 	*fdma;
	UINT32 	i, nxt;

	if( handler == NULL ){
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *)handler;

	if (xSemaphoreTake(fdma->mutex, portMAX_DELAY) != pdTRUE) {
		return FALSE;
	}

	FDMA_CLKGATE_ON();

	nxt = FDMA_GET_History(fdma->control); // check history

	if( (FDMA_GET_OP_STATUS(fdma->control) == FDMA_STATUS_IDLE)
		|| (nxt == FDMA_HIS_IDLE) ){
		nxt = 0 ;
	}else{
		nxt = ((nxt - FDMA_HIS_CH1 + 1) % FDMA_CHAN_MAX) ;

		FDMA_SET_REQ_HOLD(fdma->control, nxt ); // Hold
	}



	FDMA_TRACE_PRINT("FDMA.hwindex = %d\n", fdma->hwindex);

	for( i = 0; i < FDMA_CHAN_MAX; i++ ){
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.cfg  = %08x\n", i, fdma->hwtask[i]->LLI.Config);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.ctrl = %08x\n", i, fdma->hwtask[i]->LLI.Control);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.src  = %08x\n", i, fdma->hwtask[i]->LLI.SrcAddr);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.dst  = %08x\n", i, fdma->hwtask[i]->LLI.DstAddr);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.base = %08x\n", i, fdma->hwtask[i]->LLIbase);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.num  = %08x\n", i, fdma->hwtask[i]->LLInum);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.idx  = %08x\n", i, fdma->hwtask[i]->LLIindex);
		FDMA_TRACE_PRINT("HWTsk[%d]LLI.stt  = %08x\n", i, fdma->hwtask[i]->LLIstatus);
	}

	FDMA_TRACE_PRINT("FDMA.swindex = %d\n", fdma->swindex);
	FDMA_TRACE_PRINT("FDMA.swchannum = %d\n", fdma->swchannum);

	FDMA_TRACE_PRINT("FDMA.statistics = %d\n", fdma->statistics);

	FDMA_UNSET_REQ_HOLD(fdma->control); // Release

	if( FDMA_GET_OP_STATUS(fdma->control) == FDMA_STATUS_IDLE ){
		FDMA_CLKGATE_OFF();
	}

	xSemaphoreGive(fdma->mutex);

	return TRUE;
}

/******************************************************************************
 *  fdma_isr_create ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	fdma_isr_create(HANDLE handler)
{
	//FDMA_HANDLER_TYPE	*fdma;

	if(handler == NULL){
		return FALSE;
	}

	//fdma = (FDMA_HANDLER_TYPE *) handler ;
	// Clear All Interrupts

	return TRUE;

}


static int	fdma_isr_init(HANDLE handler)
{
	FDMA_HANDLER_TYPE	*fdma;

	if(handler == NULL){
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *) handler ;

	// Registry LISR
	switch(fdma->dev_addr){
		case	DA16X_FDMA_BASE:
			_sys_nvic_write(FDMA_IRQn, (void *)_lowlevel_fdma_interrupt0, 0x7);
			break;
	}

	FDMA_SET_IRQ_MASK(fdma->control
			, (FDMA_IRQ_ERROR/*|FDMA_IRQ_HOLD*/
			  |FDMA_IRQ_CH4DONE|FDMA_IRQ_CH3DONE
			  |FDMA_IRQ_CH2DONE|FDMA_IRQ_CH1DONE
			  ));

	return TRUE;
}


static int	fdma_isr_close(HANDLE handler)
{
	FDMA_HANDLER_TYPE	*fdma;

	if(handler == NULL){
		return FALSE;
	}

	fdma = (FDMA_HANDLER_TYPE *) handler ;

	// Clear All Interrupts
	FDMA_SET_REQ_CLR_IRQ(fdma->control, 0);
	FDMA_SET_IRQ_MASK(fdma->control, 0);

	// UN-Registry LISR
	switch(fdma->dev_addr){
		case	DA16X_FDMA_BASE:
			_sys_nvic_write(FDMA_IRQn, NULL, 0x7);
			break;
	}

	return TRUE;

}

/******************************************************************************
 *  _lowlevel_fdma_interrupt ( )
 *
 *  Purpose :   LISR
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
//
//	LISR
//

void __fdma_interrupt_unit0(VOID)
{
	FDMA_HANDLER_TYPE	*fdma;
	UINT32			intr, hwdone, idx, nxt;
	UINT32			tskcnt, flagstart ;

	fdma = (FDMA_HANDLER_TYPE *) (_fdma_init_handler[0]) ;

	if( fdma == NULL ){
		return;
	}

	nxt = FDMA_GET_History(fdma->control); // check history

	if( (FDMA_GET_OP_STATUS(fdma->control) == FDMA_STATUS_IDLE)
		|| (nxt == FDMA_HIS_IDLE) ){
		nxt = 0 ;
	}
	else {
		nxt = ((nxt - FDMA_HIS_CH1 + 1) % FDMA_CHAN_MAX) ;

		FDMA_SET_REQ_HOLD(fdma->control, nxt ); // Hold
	}

	intr = FDMA_GET_IRQ_STATUS(fdma->control); // check interrupt

	if ( (intr & FDMA_IRQ_ERROR) == FDMA_IRQ_ERROR ) {
		FDMA_SET_REQ_CLR_OP(fdma->control, FDMA_CHAN_MASK);
	}

	intr = CONVERT_CLR_IRQ_OFFSET(intr);
	FDMA_SET_REQ_CLR_IRQ(fdma->control, intr);   // clear

	nxt = FDMA_GET_History(fdma->control);	// recheck history
	if( (FDMA_GET_OP_STATUS(fdma->control) == FDMA_STATUS_IDLE)
		|| (nxt == FDMA_HIS_IDLE) ){
		nxt = 0 ;
		tskcnt = FDMA_CHAN_MAX;
	}else{
		nxt = ((nxt - FDMA_HIS_CH1 + 1) % FDMA_CHAN_MAX) ;
		tskcnt = 0;
	}

	hwdone = (UINT32)(fdma_intr_count_list[(intr&FDMA_CHAN_MASK)]);

	flagstart = 0;

	if( hwdone != 0 ){

		idx = fdma->hwindex;

		if(tskcnt == 0 ){
			tskcnt = FDMA_CHAN_MAX + hwdone - idx;
			if( tskcnt > FDMA_CHAN_MAX ){
				tskcnt = tskcnt - FDMA_CHAN_MAX;
			}
		}

		hwdone = hwdone - 1;

		// processing
		while(1)
		{
			FDMA_SWLLI_MAP *swlli;
			UINT32  LLIindex;

			swlli = (FDMA_SWLLI_MAP *)(fdma->hwtask[idx]->LLIbase);

			if( swlli != NULL && swlli->callback != NULL ){
				swlli->callback( swlli->param );
			}

			LLIindex = fdma->hwtask[idx]->LLIindex;
			if( FDMA_LLItsk_Get_CN(LLIindex)
				== FDMA_LLItsk_Get_TN(LLIindex) ){
				fdma->hwtask[idx]->LLIbase = 0;
			}

			fdma->statistics ++;

			if( idx == hwdone ){
				idx = (idx + 1) % FDMA_CHAN_MAX;
				break;
			}

			idx = (idx + 1) % FDMA_CHAN_MAX;
		}

		fdma->hwindex = idx ; // update

		if( tskcnt == FDMA_CHAN_MAX ){
			idx = 0; // reset
		}else{
			idx = fdma->hwindex;
		}

		// check available rooms
		while(fdma->swchannum > 0)
		{
			// add delayed item

			if( tskcnt > 0 ){
				FDMA_SWTSK_MAP *swtsk = fdma->swchanhead;
				UINT32 hwptr    = swtsk->hwptr ;
				UINT32 newhwptr = (hwptr + 1) % swtsk->max ;

				fdma->hwtask[idx]->LLI.Config = swtsk->lli[hwptr].lli.Config;

				tskcnt--;

				if( (tskcnt == 0) || (newhwptr == swtsk->swptr) ){
					/* if room is not available or this swtsk is last  */
					fdma->hwtask[idx]->LLI.Control
						= FDMA_CHAN_IRQEN | swtsk->lli[hwptr].lli.Control ;

					//TRC: TEST_DBG_TRIGGER((0xFDD0FFF0));
				}else{
					fdma->hwtask[idx]->LLI.Control
						= swtsk->lli[hwptr].lli.Control;
				}

				fdma->hwtask[idx]->LLI.SrcAddr = swtsk->lli[hwptr].lli.SrcAddr;
				fdma->hwtask[idx]->LLI.DstAddr = swtsk->lli[hwptr].lli.DstAddr;

				fdma->hwtask[idx]->LLIbase  = (UINT32)( &(swtsk->lli[hwptr]) );
				fdma->hwtask[idx]->LLInum   = FDMA_LLItsk_CN(0)|FDMA_LLItsk_TN(0);
				fdma->hwtask[idx]->LLIindex = 0;

				//TRC: TEST_DBG_TRIGGER(swtsk->lli[hwptr].lli.SrcAddr);
				//TRC: TEST_DBG_TRIGGER(swtsk->lli[hwptr].lli.DstAddr);

				fdma->swindex = (idx + 1) % FDMA_CHAN_MAX;
				swtsk->hwptr = (UINT16)newhwptr ;

				if( newhwptr == swtsk->swptr ){

					fdma->swchanhead = swtsk->nxt;
					if( fdma->swchanhead == NULL ){
						fdma->swchantail = NULL;
					}
					fdma->swchannum --;
				}

				flagstart |= FDMA_MAKE_ST_CH_EN(idx);
			}
			else{
				break;
			}

			if( idx == hwdone ){
				break;
			}

			idx = (idx + 1) % FDMA_CHAN_MAX;
		}

		if( fdma->swchannum == 0 && tskcnt != 0 ){
			fdma->hwindex = 0; // reset
		}

		FDMA_PUSH_ST_CH_EN(fdma->control, flagstart);
		if ( fdma->mode == TRUE ) {
			//xEventGroupSetBits(fdma->event, 0x00000001);
			BaseType_t xHigherPriorityTaskWoken, xResult;
			xHigherPriorityTaskWoken = pdFALSE;
			xResult = xEventGroupSetBitsFromISR(fdma->event, 0x00000001, &xHigherPriorityTaskWoken);
			if( xResult != pdFAIL )
			{

				/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
				switch should be requested.  The macro used is port specific and will
				be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
				the documentation page for the port being used. */
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			}
		}
	}

	FDMA_UNSET_REQ_HOLD(fdma->control); // Release

	if( flagstart == 0 ){
		FDMA_CLKGATE_OFF();
	}
}

void _fdma_interrupt_unit0(VOID)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(__fdma_interrupt_unit0);
	traceISR_EXIT();
}

static  void	_lowlevel_fdma_interrupt0(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(__fdma_interrupt_unit0);
	traceISR_EXIT();
}

/******************************************************************************
 *  ROM_FDMA_COPY ( )
 *
 *  Purpose :   LISR
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	FDMA_RWCOPY_SIZE	(0x100000)

static void da16200_pseudo_fdma_intr_callback(void *param);

void	ROM_FDMA_COPY(UINT32 mode, UINT32 *dest, UINT32 *src, UINT32 size)
{
	FDMA_SWTSK_MAP	swtsk;
	FDMA_SWLLI_MAP	swlli[3];
	UINT32	acclen, transiz;
	UINT32	fdma_control, fdma_config, event;
	UINT32	bkup_irq_fdma;
	volatile UINT32	*writeregion32;
	volatile UINT32	*readregion32;

	_sys_nvic_read( FDMA_IRQn, (void *)&bkup_irq_fdma); // backup
	_sys_nvic_write(FDMA_IRQn, (void *)_fdma_interrupt_unit0, 0x7);

	fdma_control = FDMA_CHAN_CONTROL(0,0);
	fdma_config = FDMA_CHAN_CONFIG(
			FDMA_CHAN_AINCR, FDMA_CHAN_AHB1, FDMA_CHAN_HSIZE_4B
			, FDMA_CHAN_AINCR, FDMA_CHAN_AHB0, FDMA_CHAN_HSIZE_4B
		 );
	fdma_config |= FDMA_CHAN_BURST(10) ;

	swtsk.fdma = FDMA_SYS_GET_HANDLER();
	swtsk.id = (UINT16)size;
	swtsk.Config = fdma_config;
	swtsk.Control = fdma_control;

	DRV_MEMSET(&swlli, 0, sizeof(FDMA_SWLLI_MAP)*3);

	swtsk.swptr = 0;
	swtsk.hwptr = 0;
	swtsk.max = 3;
	swtsk.lli = (FDMA_SWLLI_MAP *)&swlli;
	swtsk.nxt = NULL;

	writeregion32 = (UINT32 *)((UINT32)dest);
	readregion32 = (UINT32 *)((UINT32)src);

	if( mode == DA16X_NORTYPE_CACHE ){
		// Cache to XIP
		DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP);
	}

	ENABLE_INTERRUPTS();

	for( acclen = 0, transiz = FDMA_RWCOPY_SIZE ; acclen  < size;  )
	{
		transiz = ( ( size - acclen ) > FDMA_RWCOPY_SIZE )
			? FDMA_RWCOPY_SIZE
			: ( ( size - acclen + 0x03 ) & ~0x03 );

		event = FALSE;

		FDMA_SCATTER_CHANNEL( (HANDLE)&swtsk
			, (void *)writeregion32
			, (void *)readregion32
			, transiz
			, (UINT32)&da16200_pseudo_fdma_intr_callback, (UINT32)(&event));

		FDMA_START_CHANNEL((HANDLE)&swtsk);

		writeregion32 = (UINT32 *)((UINT32)writeregion32+transiz);
		readregion32 = (UINT32 *)((UINT32)readregion32+transiz);
		acclen  += transiz ;

		while( event == FALSE){
			WAIT_FOR_INTR();
		}
	}

	DISABLE_INTERRUPTS();

	if( mode == DA16X_NORTYPE_CACHE ){
		// XIP to Cache
		DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_CACHE);
	}

	_sys_nvic_write(FDMA_IRQn, (void *)bkup_irq_fdma, 0x7); // restore
}

/******************************************************************************
 *  da16200_pseudo_fdma_intr_callback ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void da16200_pseudo_fdma_intr_callback(void *param)
{
	UINT32	*event;

	if( param == NULL){
		return ;
	}

	event = (UINT32 *) param;
	*event = TRUE;
}
