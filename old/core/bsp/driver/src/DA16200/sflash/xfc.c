/**
 ****************************************************************************************
 *
 * @file xfc.c
 *
 * @brief XFC, Flash Controller
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
#include "sys_clock.h"
#include "xfc.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
//---------------------------------------------------------
//
//---------------------------------------------------------

#undef	SUPPORT_XFC_DEBUG
#define	SUPPORT_XFC_POLLMODE
#define SUPPORT_XFC_TEST_180713A

#define	XFC_IRQn		Flash_Ctrl_IRQn
#define	XCACHE_IRQn		Cache_Ctrl_IRQn

#define	DA16X_XFC_COMMON_BASE	(DA16X_XFC_BASE|0x0000)
#define	DA16X_XFC_SPI_BASE	(DA16X_XFC_BASE|0x0100)
#define	DA16X_XFC_DMA_BASE	(DA16X_XFC_BASE|0x0200)
#define	DA16X_XFC_XIP_BASE	(DA16X_XFC_BASE|0x0300)

//---------------------------------------------------------
// Common Domain
//---------------------------------------------------------

#define	XFC_TIMEOUT_OP_EN 		(1<<3)
#define	XFC_XIP_ACC_OP_EN 		(1<<2)
#define	XFC_NON_DMA_OP_EN 		(1<<1)
#define	XFC_DMA_ACC_OP_EN 		(1<<0)

#define	XFC_REQ_GET_IRQMASK(x)		((x)>>8)
#define	XFC_REQ_SET_IRQMASK(x)		((x)<<8)

#define	XFC_REQ_GET_CLR_STATUS(x)	((x)>>8)
#define	XFC_REQ_SET_CLR_STATUS(x)	((x)<<8)

#define XFC_IRQ_STATUS_ALLMARK		(0xC00000FF)
#define XFC_IRQ_STATUS_SW_FORCE		(1<<31)
#define XFC_IRQ_STATUS_TO_FORCE		(1<<30)
#define XFC_IRQ_STATUS_TXFIFO_UNDR	(1<<7)
#define XFC_IRQ_STATUS_TXFIFO_OVER	(1<<6)
#define XFC_IRQ_STATUS_RXFIFO_UNDR	(1<<5)
#define XFC_IRQ_STATUS_RXFIFO_OVER	(1<<4)
#define XFC_IRQ_STATUS_AHB_ERR		(1<<3)
#define XFC_IRQ_STATUS_ACC_ERR		(1<<2)
#define XFC_IRQ_STATUS_SET_ERR		(1<<1)
#define	XFC_IRQ_STATUS_ACC_DONE		(1<<0)

#define XFC_CORE_CLK_RATIO(x)		((x)&0x0F)
#define XFC_CORE_SET_FREQ_MHz(x)	(((x)-1)&0x01FF)
#define XFC_CORE_GET_FREQ_MHz(x)	(((x)&0x01FF)+1)

#define XFC_TIMEOUT_UNIT_8us(x)		((x)&0x07)
#define XFC_TIMEOUT_COUNT(x)		((x)&0x1F)

#define XFC_CE_SEL_CYCLE(x)		((x)&0x07)
#define XFC_CE_DESEL_CYCLE(x)		((x)&0x7F)

#define XFC_SPI_DELAY_TXC_EN		(1<<0)
#define XFC_SPI_DELAY_RXC_EN		(1<<1)
#define XFC_SPI_DELAY_TXCRXC_EN		(1<<2)

#define	XFC_SPI_DELAY_TXC(pol,cyc)	(((pol)<<8)|((cyc)&0x0FF))
#define	XFC_SPI_DELAY_RXC(pha,cyc)	((((pha)&0x03)<<8)|((cyc)&0x0FF))
#define XFC_RX_0dot0_CYCLE		(0)
#define XFC_RX_0dot5_CYCLE		(1)
#define XFC_RX_1dot0_CYCLE		(2)
#define XFC_RX_1dot5_CYCLE		(3)

//---------------------------------------------------------
// SPI Domain
//---------------------------------------------------------

#define XFC_SPI_SET_CORE_FREQ_MHz(x)	(((x)-1)&0x01FF)
#define XFC_SPI_GET_CORE_FREQ_MHz(x)	(((x)&0x01FF)+1)

#define XFC_SPI_SET_FREQ_MHz(x)		(((x))&0x01FF)
#define XFC_SPI_GET_FREQ_MHz(x)		(((x)&0x01FF))

#define XFC_SPI_CHK_SAFE_Hz(spi,core)	(((spi)<<1)<=(core))

#define XFC_CORE_SET_RATIO(x)		((x)-1)
#define XFC_CORE_GET_RATIO(x)		((x)+1)

#define	XFC_SPI_CPOLCPHASE_MODE0	(0)
#define	XFC_SPI_CPOLCPHASE_MODE3	(3)

#define XFC_SPI_BYTE_SWAP_NORMAL	(0)
#define XFC_SPI_BYTE_SWAP_BE32		(1)
#define XFC_SPI_BYTE_SWAP_BE16		(2)
#define XFC_SPI_BYTE_SWAP_LE16		(3)

#define XFC_SPI_WRAP_AROUND_EN		(1)

#define XFC_SPI_IOPIN_AUTO()		(0)
#define XFC_SPI_IOPIN_MANUAL(dir,lvl,man)	(((lvl)<<16)|((dir)<<8)|(man))
#define XPIN_OUT(x)			(1<<(x))
#define XPIN_IN(x)			(0)
#define XPIN_HIGH(x)			(1<<(x))
#define XPIN_LOW(x)			(0)

#define XFC_SPI_CLKPIN_AUTO()		(0)
#define XFC_SPI_CLKPIN_MANUAL(dir,lvl,man)	(((lvl)<<2)|((dir)<<1)|(man))

#define XFC_SPI_CSELPIN_AUTO(pol)	((pol)<<24)
#define XFC_SPI_CSELPIN_MANUAL(dir,lvl,man)	(((lvl)<<16)|((dir)<<8)|(man))

#define XFC_SPI_SET_CSEL_LOCK(pin)	(pin)

#define	XPIN_3WIRE(x)			(0)
#define	XPIN_4WIRE(x)			(1<<(x))

#define XFC_SPI_SET_TWIN_ENABLE(pin)	(pin)

#define	XFC_SPI_DATA_RD			(0)
#define	XFC_SPI_DATA_WR			(1)

#define XFC_SPI_TOTAL_LEN_BYTE(x)	((x)&0x0FFFFF)
#define XFC_SPI_MAX_LENGTH		((2*MBYTE)-1)

#define XFC_SPI_OPPHASE_IDLE		(0)
#define XFC_SPI_OPPHASE_CMD		(1)
#define XFC_SPI_OPPHASE_ADDR		(2)
#define XFC_SPI_OPPHASE_DUMMY		(3)
#define XFC_SPI_OPPHASE_WRDATA		(4)
#define XFC_SPI_OPPHASE_RDDATA		(5)
#define XFC_SPI_OPPHASE_WAIT		(6)
#define XFC_SPI_OPPHASE_ERROR		(7)

#define XFC_SPI_SET_CSEL(x)		(1<<(x))

#define	XFC_PIN_WPHOLD_DISABLE		(0x00000000)
#define	XFC_PIN_WPHOLD_ENABLE		(0x00cccccc)

//---------------------------------------------------------
// DMA Domain
//---------------------------------------------------------

#define XFC_DMA_TASK_SIZE(x)		(((x)&0x03)<<8)
#define XFC_DMA_OP_RD			(1)
#define XFC_DMA_OP_WR			(0)

//---------------------------------------------------------
// XIP Domain
//---------------------------------------------------------

//---------------------------------------------------------
//
//---------------------------------------------------------

#define	XFC_MEM_MALLOC(x)	HAL_MALLOC(x)
#define	XFC_MEM_FREE(x)		HAL_FREE(x)

#define XFC_TRACE_PRINT(...)	DRV_PRINTF(__VA_ARGS__)

#ifdef SFLASH_XFC_DEBUGGING
#define	SUPPORT_XFC_DEBUG
#else
#undef	SUPPORT_XFC_DEBUG
#endif

#ifdef	SUPPORT_XFC_DEBUG
#undef  SUPPORT_XFC_DEBUG_DBGT

#ifdef	SUPPORT_XFC_DEBUG_DBGT
#define XFC_REG_ACCESS(ACCESS)		DBGT_Printf( "XFC: " #ACCESS " at line %d\n", __LINE__ ); ACCESS
#define XFC_REG_ACCESS_ISR(ACCESS)	DBGT_Printf( "XFC: " #ACCESS " at line %d\n", __LINE__ ); ACCESS
#define XFC_REG_ACCESS_DUMP(x)		DBGT_Printf( "XFC: %08x\n", x)
#else
#define XFC_REG_ACCESS(ACCESS)		/*DRV_DBG_NONE( #ACCESS " @ %d", __LINE__ );*/ ACCESS
#define XFC_REG_ACCESS_ISR(ACCESS)	/*DRV_DBG_NONE( #ACCESS " @ %d", __LINE__ );*/ ACCESS
#define XFC_REG_ACCESS_DUMP(x)
#endif

#define	XFC_DBG_NONE(...)		DRV_DBG_NONE( __VA_ARGS__ )
#define	XFC_DBG_BASE(...)		DRV_DBG_BASE( __VA_ARGS__ )
#define	XFC_DBG_INFO(...)		DRV_DBG_INFO( __VA_ARGS__ )
#define	XFC_DBG_WARN(...)		DRV_DBG_WARN( __VA_ARGS__ )
#define	XFC_DBG_ERROR(...)		DRV_DBG_ERROR( __VA_ARGS__ )
#define	XFC_DBG_DUMP(...)		DRV_DBG_DUMP( __VA_ARGS__ )
#define	XFC_DBG_SPECIAL(...)	DRV_DBG_INFO( __VA_ARGS__ )
#else
#define XFC_REG_ACCESS(ACCESS)		ACCESS
#define XFC_REG_ACCESS_ISR(ACCESS)	ACCESS
#define XFC_REG_ACCESS_DUMP(x)

#define	XFC_DBG_NONE(...)
#define	XFC_DBG_BASE(...)
#define	XFC_DBG_INFO(...)
#define	XFC_DBG_WARN(...)		//DRV_DBG_WARN( __VA_ARGS__ )
#define	XFC_DBG_ERROR(...)		//DRV_DBG_ERROR( __VA_ARGS__ )
#define	XFC_DBG_DUMP(...)
#define	XFC_DBG_SPECIAL(...)	//DRV_DBG_INFO( __VA_ARGS__ )
#endif

//---------------------------------------------------------
//
//---------------------------------------------------------
// Zero-init(BSS) or Uninit data does not need to be placed in the RAMDATA section.
DATA_ALIGN(4) static XFC_HANDLER_TYPE	*_xfc_handler[XFC_UNIT_MAX];
DATA_ALIGN(4) static int		_xfc_instance[XFC_UNIT_MAX];
DATA_ALIGN(4) static UINT		_xfc_buslock[XFC_UNIT_MAX];	/* chg by_sjlee OS_INDEPENDENCY */
DATA_ALIGN(4) static CLOCK_LIST_TYPE	_xfc_pll_info[XFC_UNIT_MAX];
DATA_ALIGN(4) static char	STATIC_XFC[sizeof(XFC_HANDLER_TYPE)];	/* chg by_sjlee OS_INDEPENDENCY */

static int  xfc_isr_create(HANDLE handler);
static int  xfc_isr_init(HANDLE handler);
static int  xfc_isr_close(HANDLE handler);

static void _tx_specific_xfc0_interrupt(void);
void xfc_isr_core(UINT32 idx);
static void _tx_specific_xcch0_interrupt(void);
void xcch0_isr_core(UINT32 idx);

static void xfc_set_pll_div(XFC_HANDLER_TYPE *xfc);
static void xfc_pll_callback(UINT32 clock, void *param);

static int  xfc_set_format(XFC_HANDLER_TYPE *xfc, UINT32 format);
static int  xfc_set_coreclock(XFC_HANDLER_TYPE *xfc, UINT32 speed);
static int  xfc_set_speed(XFC_HANDLER_TYPE *xfc, UINT32 speed);
static int  xfc_get_speed(XFC_HANDLER_TYPE *xfc, UINT32 *speed);
static int  xfc_set_timeout(XFC_HANDLER_TYPE *xfc, UINT32 unit, UINT32 count);
static int  xfc_set_wiremodel(XFC_HANDLER_TYPE *xfc, UINT32 unit, UINT32 model);
static int  xfc_set_dma_config(XFC_HANDLER_TYPE *xfc, UINT32 config);
static int  xfc_set_pincontrol(XFC_HANDLER_TYPE *xfc, UINT32 config);
static int  xfc_set_cloning(XFC_HANDLER_TYPE *xfc, UINT32 *config);
static void xfc_isr_convert(XFC_HANDLER_TYPE *xfc, UINT32 *pirqstatus, UINT32 *psubirq);
static void xfc_set_delay_parameter(XFC_HANDLER_TYPE *xfc, UINT8 *config);
static void xfc_set_delay_index(XFC_HANDLER_TYPE *xfc);

/******************************************************************************
 *  XFC_CREATE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
HANDLE	XFC_CREATE( UINT32 dev_id )
{
	XFC_HANDLER_TYPE *xfc;

	// Allocate
	if( dev_id >= XFC_UNIT_MAX ){
		XFC_DBG_ERROR("XFC: ilegal unit, %d \r\n", dev_id);
		return NULL;
	}

	XFC_DBG_INFO(" [%s] XFC create, dev_id:%d \r\n", __func__, dev_id );

	// Multi-instance
	if( _xfc_handler[dev_id] == NULL ){
		XFC_DBG_INFO(" [%s] XFC alloc, dev_id:%d \r\n", __func__, dev_id);
		xfc = (XFC_HANDLER_TYPE *) &STATIC_XFC[0];	/* chg by_sjlee OS_INDEPENDENCY */
		if( xfc == NULL ){
			XFC_DBG_ERROR("XFC: mem alloc fail \r\n");
			return NULL;
		}
		DRV_MEMSET(xfc, 0, sizeof(XFC_HANDLER_TYPE));
	}
	else {
		xfc = (XFC_HANDLER_TYPE *) &STATIC_XFC[0];	/* chg by_sjlee OS_INDEPENDENCY */
		if( xfc == NULL ){
			XFC_DBG_ERROR("XFC: MEM alloc fail \r\n");
			return NULL;
		}

		//Skip

	}

	// Address Mapping
	switch((XFC_UNIT_LIST)dev_id){
		case	XFC_UNIT_0:
			xfc->dev_unit = XFC_UNIT_0;
			xfc->instance = _xfc_instance[XFC_UNIT_0];
			_xfc_instance[XFC_UNIT_0] ++;

			xfc->reg_cmn = (XFC_COMMON_TYPE *)DA16X_XFC_COMMON_BASE;
			xfc->reg_spi = (XFC_SPI_DOMAIN_TYPE *)DA16X_XFC_SPI_BASE;
			xfc->reg_dma = (XFC_DMA_DOMAIN_TYPE *)DA16X_XFC_DMA_BASE;
			xfc->reg_xip = (XFC_XIP_DOMAIN_TYPE *)DA16X_XFC_XIP_BASE;

			xfc->buslock  = _xfc_buslock[XFC_UNIT_0];
			xfc->pllinfo  = (VOID *)&(_xfc_pll_info[XFC_UNIT_0]);
			break;

		default:
			break;
	}

	if (xfc->instance == 0) {
		// Set Default Para
		XFC_DBG_INFO("(%p) XFC create, base %p", xfc, (UINT32 *)xfc->reg_cmn );

		_xfc_handler[dev_id] = xfc;

		xfc->concat	= FALSE;
		xfc->cselinfo 	= XFC_SPI_SET_CSEL(XFC_CSEL_0);
		XFC_REG_ACCESS( xfc->cselbkup = xfc->reg_xip->XIP_CS_SEL ); // MPW Issue
		xfc->debug 	= FALSE;
		xfc->maxlength	= (1*KBYTE); // under 1KB
		xfc->dsize	= sizeof(UINT32);
		xfc->xfermode	= XFC_SET_DMAMODE;
		xfc->coreclock  = 0; // init value
		xfc->spiclock   = 10*MHz ;
		xfc->busctrl[0] = 0;
		xfc->busctrl[1] = 0;
		xfc->workbuf	= NULL;
		xfc->event	= NULL;
		xfc->dlytab	= NULL;

		// Create ISR
		xfc_isr_create(xfc);

		// PLL callback
		xfc_set_pll_div(xfc);

		{
			CLOCK_LIST_TYPE *pllinfo;

			pllinfo = (CLOCK_LIST_TYPE *)xfc->pllinfo;

			pllinfo->callback.func = xfc_pll_callback;
			pllinfo->callback.param = xfc;
			pllinfo->next = NULL;

			_sys_clock_ioctl( SYSCLK_SET_CALLACK, pllinfo);
		}
	}
	else
	{
		XFC_DBG_INFO("(%p) XFC re-create, base %p", xfc, (UINT32 *)xfc->reg_cmn );
	}

	return (HANDLE) xfc;
}

/******************************************************************************
 *  XFC_INIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int	XFC_INIT(HANDLE handler)
{
	XFC_HANDLER_TYPE *xfc;

	if(handler == NULL){
		return FALSE;
	}
	xfc = (XFC_HANDLER_TYPE *)handler ;

	if( xfc->instance != 0 ){
		return TRUE;
	}

	if( xfc->buslock == 0 ){
		_xfc_buslock[xfc->dev_unit] = xfc->buslock;	/* chg by_sjlee OS_INDEPENDENCY */
	}

#ifdef	SUPPORT_XFC_POLLMODE
	XFC_REG_ACCESS(
	    xfc->reg_cmn->XFC_OP_EN
		= (XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN|XFC_XIP_ACC_OP_EN)
		);
#else	//SUPPORT_XFC_POLLMODE
	if( xfc->event == NULL ){
		xfc->event = xEventGroupCreate();
	}

	XFC_REG_ACCESS(
	    xfc->reg_cmn->XFC_OP_EN
		= XFC_REQ_SET_IRQMASK(XFC_TIMEOUT_OP_EN|XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN)
		  | (XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN|XFC_XIP_ACC_OP_EN)
		);
#endif	//SUPPORT_XFC_POLLMODE

	if( xfc_set_coreclock(xfc, xfc->coreclock) == TRUE ){
		xfc_set_speed(xfc, xfc->spiclock);
	}

	//No-INIT :: xfc_set_delay_parameter(xfc, NULL);

#ifdef	SUPPORT_XFC_POLLMODE
	xfc_isr_init(xfc);
#endif	//SUPPORT_XFC_POLLMODE

	return TRUE;
}

/******************************************************************************
 *  XFC_IOCTL( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
// XFC is used as a sub-driver of SFLASH.
// So, interrupt locking method should be the same as SFLASH
// to prevent an error that interrupt locking does not match each.

ATTRIBUTE_RAM_FUNC UINT xSemaphoreTake_freertos(XFC_HANDLER_TYPE *xfc, UINT32 wait)
{
	UINT32 bkupintr;
	UINT	retv;

	retv = pdFALSE;
	
	if( wait == 0 ){
		CHECK_INTERRUPTS(bkupintr);
		PREVENT_INTERRUPTS(1);	
		if (xfc->buslock == 0) {
			xfc->buslock = 1;
			retv = pdTRUE;
		}
		PREVENT_INTERRUPTS(bkupintr);		
	}else{
		CHECK_INTERRUPTS(bkupintr);
		PREVENT_INTERRUPTS(1);	
		while (xfc->buslock != 0) {
			PREVENT_INTERRUPTS(bkupintr);
			if(bkupintr == 0){
				vTaskDelay(1); // task-switching if interrupt is enabled. 
			}
			PREVENT_INTERRUPTS(1);
			if(wait != portMAX_DELAY){
				wait--;
			}
			if(wait == 0){
				break;
			}
		}

		if( xfc->buslock == 0 ){
			xfc->buslock = 1;
			retv = pdTRUE;
		}
		PREVENT_INTERRUPTS(bkupintr);	
	}

	return retv;
}

ATTRIBUTE_RAM_FUNC
UINT xSemaphoreGive_freertos(XFC_HANDLER_TYPE *xfc) 
{
	UINT32 bkupintr;

	CHECK_INTERRUPTS(bkupintr);
	PREVENT_INTERRUPTS(1);	
	
	xfc->buslock = 0;

	PREVENT_INTERRUPTS(bkupintr);	
	return pdTRUE;
}

ATTRIBUTE_RAM_FUNC
int	XFC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data)
{
	XFC_HANDLER_TYPE *xfc;
	int status;

	if(handler == NULL){
		return FALSE;
	}
	xfc = (XFC_HANDLER_TYPE *)handler ;

	status = TRUE;

	switch(cmd)
	{
	case XFC_SET_DEBUG:
		xfc->debug = TRUE;
		break;

	//-------------------------------------------------------
	case XFC_SET_CORECLOCK:
		status = xfc_set_coreclock( xfc, ((UINT32 *)data)[0] );
		if( status == TRUE ){
			status = xfc_set_speed( xfc, xfc->spiclock );
		}
		break;

	case XFC_SET_SPEED:
		status = xfc_set_speed( xfc, ((UINT32 *)data)[0] );
		break;
	case XFC_GET_SPEED:
		status = xfc_get_speed( xfc, ((UINT32 *)data) );
		break;

	case XFC_GET_MAX_LENGTH:
		((UINT32 *)data)[0] = xfc->maxlength;
		break;
	case XFC_SET_MAX_LENGTH:
		if( ((UINT32 *)data)[0] < XFC_SPI_MAX_LENGTH ){
			xfc->maxlength = ((UINT32 *)data)[0];
		}else{
			xfc->maxlength = XFC_SPI_MAX_LENGTH;
		}
		break;

	case XFC_SET_FORMAT:
		status = xfc_set_format(xfc, *((UINT32 *)data) );
		break;
	case XFC_SET_DATASIZE:
		xfc->dsize = *((UINT32 *)data) ;
		break;

	case XFC_SET_INTRMODE:
		xfc->xfermode = XFC_SET_INTRMODE ;
		break;
	case XFC_SET_DMAMODE:
		xfc->xfermode = XFC_SET_DMAMODE ;
		break;
	case XFC_GET_XFERMODE:
		((UINT32 *)data)[0] = xfc->xfermode;
		break;

	case XFC_SET_CALLACK:
		break;

	//-------------------------------------------------------
	case XFC_CHECK_BUSY:
		break;
	case XFC_CHECK_RX_FULL:
		break;
	case XFC_CHECK_RX_NO_EMPTY:
		break;
	case XFC_CHECK_TX_NO_FULL:
		break;
	case XFC_CHECK_TX_EMPTY:
		break;

	case XFC_SET_OUTEN:
		break;
	case XFC_SET_CONCAT:
		xfc->concat	= TRUE;
		break;

	case XFC_GET_BUSCONTROL:
		((UINT32 *)data)[0] = xfc->busctrl[0];
		((UINT32 *)data)[1] = xfc->busctrl[1];
		break;
	case XFC_SET_BUSCONTROL:
		xfc->busctrl[0] = ((UINT32 *)data)[0] ;
		xfc->busctrl[1] = ((UINT32 *)data)[1] ;
		break;
	case XFC_SET_PINCONTROL:
		status = xfc_set_pincontrol(xfc, ((UINT32 *)data)[0]);
		break;
	case XFC_GET_DELAYSEL:
		((UINT32 *)data)[0] = (UINT32)( xfc->dlytab );
		break;
	case XFC_SET_DELAYSEL:
		xfc_set_delay_parameter(xfc, (UINT8 *)data );
		break;
	case XFC_SET_TIMEOUT:
		// ((UINT32 *)data)[0] : Unit
		// ((UINT32 *)data)[1] : Count
		status = xfc_set_timeout(xfc, ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
		break;

	case XFC_SET_WIRE:
		// ((UINT32 *)data)[0] : Unit
		// ((UINT32 *)data)[1] : Wire model
		status = xfc_set_wiremodel(xfc, ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
		break;

	case XFC_SET_DMA_CFG:
		// ((UINT32 *)data)[0] : Config
		status = xfc_set_dma_config(xfc, ((UINT32 *)data)[0]);
		break;

	case XFC_SET_XIPMODE:
		// ((UINT32 *)data)[0] : AXh
		// ((UINT32 *)data)[1] : Read OPCode
		status = xfc_set_cloning(xfc, ((UINT32 *)data));
		break;
	//-------------------------------------------------------
	case XFC_SET_LOCK:
/* del by_sjlee OS_INDEPENDENCY  
		if( xfc->buslock == NULL ){
			status =  FALSE;
		}
		else 
*/
		if ( ((UINT32 *)data)[0] == TRUE ) {	/* LOCK */
			if (xSemaphoreTake_freertos(xfc, (UNSIGNED)((UINT32 *)data)[1]  ) != pdTRUE)	/* chg by_sjlee OS_INDEPENDENCY */
			{
				status =  FALSE;
			}
			else {
				xfc->cselinfo = XFC_SPI_SET_CSEL( ((UINT32 *)data)[2] );
				// Re-Configuration for Multi-Instance
			}
		}
		else {										/* UNLOCK */
			xSemaphoreGive_freertos(xfc);		/* chg by_sjlee OS_INDEPENDENCY */
			xfc->cselinfo	= XFC_SPI_SET_CSEL(XFC_CSEL_MAX);
		}
		break;

	// not support	-----------------------------------------
	case XFC_SET_SLAVEMODE:
	default:
		status = FALSE;
		break;
	}

	return status;
}

/******************************************************************************
 *  XFC_TRANSMIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int	XFC_TRANSMIT(HANDLE handler, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen )
{
	XFC_HANDLER_TYPE *xfc;

	if(handler == NULL){
		return FALSE;
	}
	xfc = (XFC_HANDLER_TYPE *)handler ;

	xfc->busctrl[0] = XFC_SET_ON_UPDATE( xfc->busctrl[0], 0x1111, 0, 0, 0, 1);	//@suppress("Suggested parenthesis around expression")

	return XFC_SFLASH_TRANSMIT(handler, 0, 0, 0 , snddata, sndlen, rcvdata, rcvlen );
}

/******************************************************************************
 *  XFC_SFLASH_TRANSMIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int	xfc_sflash_interrupt_mode(XFC_HANDLER_TYPE *xfc
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

ATTRIBUTE_RAM_FUNC
static int	xfc_sflash_dma_mode(XFC_HANDLER_TYPE *xfc
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );


ATTRIBUTE_RAM_FUNC
int	XFC_SFLASH_TRANSMIT(HANDLE handler
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	XFC_HANDLER_TYPE *xfc;
	UINT32	xfcilen;

	if(handler == NULL){
		return -1;
	}
	xfc = (XFC_HANDLER_TYPE *)handler ;

	XFC_DBG_SPECIAL("XFC: cmd %02x addr %08x mod %02x snd %p/%d rcv %p/%d [%08x.%08x]"
			, cmd, addr, mode, snddata, sndlen, rcvdata, rcvlen
			, xfc->busctrl[0], xfc->busctrl[1]);


	xfc_set_delay_index(xfc);

	if ( (xfc->xfermode == XFC_SET_INTRMODE) || ((sndlen == 0) && (rcvlen == 0)) ) {
		xfcilen = xfc_sflash_interrupt_mode( xfc , cmd, addr, mode , snddata, sndlen, rcvdata, rcvlen );
	}
	else {
		xfcilen = xfc_sflash_dma_mode( xfc , cmd, addr, mode , snddata, sndlen, rcvdata, rcvlen );
	}

	//xfc_set_timeout( xfc, 0, 0 );
	XFC_REG_ACCESS( xfc->reg_cmn->CLK_TO_UNIT = 0 );
	XFC_REG_ACCESS( xfc->reg_cmn->CLK_TO_COUNT = 0 );
	xfc->concat = FALSE;

	return xfcilen;
}

ATTRIBUTE_RAM_FUNC
static int	xfc_sflash_interrupt_mode(XFC_HANDLER_TYPE *xfc
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	xfcilen, xfcmode, xfc_2pass_on;
	UINT32  *snddata32;
	UINT32  *rcvdata32;

	xfcilen = sndlen + rcvlen ;
	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;

	xfcmode  = (rcvlen == 0) ? 0 : 0x01;
	xfcmode |= (sndlen == 0) ? 0 : 0x02;

	do {
		UNSIGNED chkflag, maskevt;

		xfc_2pass_on = 0;

		XFC_REG_ACCESS( xfc->reg_spi->SPI_CS_SEL = xfc->cselinfo ); // MPW issue
		XFC_REG_ACCESS( xfc->reg_xip->XIP_CS_SEL = xfc->cselinfo ); // MPW issue

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_CLR_OP = XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN );

		XFC_REG_ACCESS( xfc->reg_spi->SPI_PHASE_MODE = (xfc->busctrl[0]) & (~XFC_SPI_PHASE_DUMMY_MASK) );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_DMY_CYC = XFC_GET_SPI_DUMMY_CYCLE(xfc->busctrl[0]) );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_BUS_TYPE = xfc->busctrl[1] );

		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_CMD = cmd );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_ADDR = addr );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_MODE = mode );

		switch(xfcmode){
		case 0: // No-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(0) );
			break;
		case 1: // Read-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_RD );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(rcvlen) );
			break;
		case 3: // Write-and-Read
			xfc_2pass_on = 1;
			xfcmode = 1; // pass-2, read
			XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_CS_TYPE = XFC_SPI_CSELPIN_MANUAL(1,0,1) );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen) );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_ACC_DIN = *snddata32 );
			XFC_REG_ACCESS_DUMP( *snddata32 );
			break;

		default: // Write-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen) );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_ACC_DIN = *snddata32 );
			XFC_REG_ACCESS_DUMP( *snddata32 );
			break;
		}

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_START = XFC_NON_DMA_OP_EN );

		// Check Access-Done
		chkflag = XFC_IRQ_STATUS_ALLMARK ;
#ifdef	SUPPORT_XFC_POLLMODE
		do {
			UINT32 irqstatus, subirq;

			xfc_isr_convert(xfc, &irqstatus, &subirq);

			XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ
				= XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );

			maskevt = (UNSIGNED)(subirq & chkflag);

		} while( (maskevt) == 0 );
#else	//SUPPORT_XFC_POLLMODE
		maskevt = xEventGroupWaitBits((OAL_EVENT_GROUP *)(xfc->event), chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_XFC_POLLMODE
		XFC_DBG_NONE(">> Polling end (%d) \r\n", maskevt);

		if( xfc_2pass_on == 0 && xfcmode == 1 ){
			XFC_REG_ACCESS( *rcvdata32 = xfc->reg_spi->SPI_ACC_DOUT );
			XFC_REG_ACCESS_DUMP( *rcvdata32 );
		}
		if( xfc_2pass_on == 0 && xfc->concat == FALSE ){
			XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_CS_TYPE = XFC_SPI_CSELPIN_MANUAL(0,0,0) );
		}

		// Check Time-Out

		if( ((xfc->busctrl[0] & XFC_SPI_TIMEOUT_EN) != 0)
		    && ((maskevt & XFC_IRQ_STATUS_TO_FORCE) == 0) ){
			chkflag = XFC_IRQ_STATUS_TO_FORCE ;
#ifdef	SUPPORT_XFC_POLLMODE
			do {
				UINT32 irqstatus, subirq;

				xfc_isr_convert(xfc, &irqstatus, &subirq);

				XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ
					= XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );

				maskevt = (UNSIGNED)(subirq & chkflag);

			} while( (maskevt) == 0 );
#else	//SUPPORT_XFC_POLLMODE
			maskevt = xEventGroupWaitBits((OAL_EVENT_GROUP *)(xfc->event), chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_XFC_POLLMODE
		}

#ifdef	SUPPORT_XFC_POLLMODE
#else	//SUPPORT_XFC_POLLMODE
		// Clear
#endif	//SUPPORT_XFC_POLLMODE

		XFC_REG_ACCESS( xfc->reg_spi->SPI_CS_SEL = xfc->cselbkup ); // MPW issue
		XFC_REG_ACCESS( xfc->reg_xip->XIP_CS_SEL = xfc->cselbkup ); // MPW issue
	}
	while( xfc_2pass_on == 1 );

	return xfcilen;
}

ATTRIBUTE_RAM_FUNC
static int	xfc_sflash_dma_mode(XFC_HANDLER_TYPE *xfc
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	xfcilen, xfcmode, xfc_2pass_on, bkupcfg = 0;
	UINT32  *snddata32;
	UINT32  *rcvdata32;

	xfcilen = sndlen + rcvlen ;
	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;

	xfcmode  = (rcvlen == 0) ? 0 : 0x01;
	xfcmode |= (sndlen == 0) ? 0 : 0x02;

	do{
		UNSIGNED chkflag, maskevt;

		xfc_2pass_on = 0;

		XFC_REG_ACCESS( xfc->reg_spi->SPI_CS_SEL = xfc->cselinfo ); // MPW issue
		XFC_REG_ACCESS( xfc->reg_xip->XIP_CS_SEL = xfc->cselinfo ); // MPW issue

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_CLR_OP = XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN );

		XFC_REG_ACCESS( xfc->reg_spi->SPI_PHASE_MODE = (xfc->busctrl[0]) & (~XFC_SPI_PHASE_DUMMY_MASK) );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_DMY_CYC = XFC_GET_SPI_DUMMY_CYCLE(xfc->busctrl[0]) );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_BUS_TYPE = xfc->busctrl[1] );

		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_CMD = cmd );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_ADDR = addr );
		XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_MODE = mode );

		switch(xfcmode){
		case 0: // No-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(0) );

			XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_TASK = XFC_DMA_TASK_SIZE(0)|XFC_DMA_OP_WR );
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_TTL = XFC_SPI_TOTAL_LEN_BYTE(0) );
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );
			break;
		case 1: // Read-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_RD );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(rcvlen) );

			XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_TASK = XFC_DMA_TASK_SIZE(0)|XFC_DMA_OP_RD );
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_TTL = XFC_SPI_TOTAL_LEN_BYTE(rcvlen-1) ); /// fixed

			if( rcvdata32 != NULL ){
				XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );
			}else{
				// backup dma-config
				XFC_REG_ACCESS( bkupcfg = xfc->reg_dma->DMA_MP0_CFG );
				// update dma-config
				bkupcfg = bkupcfg & ~(XFC_DMA_MP0_AI(XFC_ADDR_INCR));
				XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_CFG = bkupcfg );

				XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_Addr = (UINT32)(&rcvdata32) );
			}
			break;
		case 3: // Write-and-Read
			xfc_2pass_on = 1;
			XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_CS_TYPE = XFC_SPI_CSELPIN_MANUAL(1,0,1) );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen) );

			XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_TASK = XFC_DMA_TASK_SIZE(0)|XFC_DMA_OP_WR );
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen-1) ); /// fixed
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_Addr = (UINT32)snddata32 );
			break;

		default: // Write-Data
			XFC_REG_ACCESS( xfc->reg_spi->SPI_REG_RW = XFC_SPI_DATA_WR );
			XFC_REG_ACCESS( xfc->reg_spi->SPI_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen) );

			XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_TASK = XFC_DMA_TASK_SIZE(0)|XFC_DMA_OP_WR );
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_TTL = XFC_SPI_TOTAL_LEN_BYTE(sndlen-1) ); /// fixed
			XFC_REG_ACCESS( xfc->reg_dma->DMA_TSK_0_Addr = (UINT32)snddata32 );
			break;
		}

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_START = XFC_DMA_ACC_OP_EN );

		// Check Access-Done
		chkflag = XFC_IRQ_STATUS_ALLMARK ;
#ifdef	SUPPORT_XFC_POLLMODE
		do{
			UINT32 irqstatus, subirq;

			xfc_isr_convert(xfc, &irqstatus, &subirq);

			XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ
				= XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );

			maskevt = (UNSIGNED)(subirq & chkflag);

		} while( (maskevt) == 0 );
#else	//SUPPORT_XFC_POLLMODE
		maskevt = xEventGroupWaitBits((OAL_EVENT_GROUP *)(xfc->event), chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_XFC_POLLMODE

		if( xfc_2pass_on == 0 && xfc->concat == FALSE ){
			XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_CS_TYPE = XFC_SPI_CSELPIN_MANUAL(0,0,0) );
		}

		// Check Time-Out

		if( ((xfc->busctrl[0] & XFC_SPI_TIMEOUT_EN) != 0)
		    && ((maskevt & XFC_IRQ_STATUS_TO_FORCE) == 0) ){
			chkflag = XFC_IRQ_STATUS_TO_FORCE ;
#ifdef	SUPPORT_XFC_POLLMODE
			do {
				UINT32 irqstatus, subirq;

				xfc_isr_convert(xfc, &irqstatus, &subirq);

				XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ
					= XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );

				maskevt = (UNSIGNED)(subirq & chkflag);

			} while( (maskevt) == 0 );
#else	//SUPPORT_XFC_POLLMODE
			maskevt = xEventGroupWaitBits((OAL_EVENT_GROUP *)(xfc->event), chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_XFC_POLLMODE
		}

#ifdef	SUPPORT_XFC_POLLMODE
#else	//SUPPORT_XFC_POLLMODE
		// Clear

#endif	//SUPPORT_XFC_POLLMODE

		switch(xfcmode){
		case 1: // Read-Data
			// restore dma-config
			if( rcvdata32 == NULL ){
				XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_CFG = bkupcfg );
			}
			break;
		default:
			if( xfc_2pass_on == 1 ){
				xfcmode = 1; // pass-2, read
			}
			break;
		}

		XFC_REG_ACCESS( xfc->reg_spi->SPI_CS_SEL = xfc->cselbkup ); // MPW issue
		XFC_REG_ACCESS( xfc->reg_xip->XIP_CS_SEL = xfc->cselbkup ); // MPW issue

	}
	while( xfc_2pass_on == 1 );

	return xfcilen;
}

/******************************************************************************
 *  XFC_CLOSE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int	XFC_CLOSE(HANDLE handler)
{
	XFC_HANDLER_TYPE *xfc;

	if(handler == NULL){
		return FALSE;
	}
	xfc = (XFC_HANDLER_TYPE *)handler ;

	if( _xfc_instance[xfc->dev_unit] > 0){
		_xfc_instance[xfc->dev_unit] --;
		xfc->instance -- ;

		if(_xfc_instance[xfc->dev_unit] == 0){
#ifdef	SUPPORT_XFC_POLLMODE
			xfc_isr_close(xfc);
#endif	//SUPPORT_XFC_POLLMODE
			_sys_clock_ioctl( SYSCLK_RESET_CALLACK, xfc->pllinfo);

			XFC_REG_ACCESS(
			    xfc->reg_cmn->XFC_OP_EN
				&= ~( XFC_REQ_SET_IRQMASK(XFC_TIMEOUT_OP_EN|XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN|XFC_XIP_ACC_OP_EN)
				    | (XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN) )
			);

			// Event
#ifdef	SUPPORT_XFC_POLLMODE
#else	//SUPPORT_XFC_POLLMODE
			if( xfc->event != NULL ){
				vEventGroupDelete(xfc->event);
			}
#endif	//SUPPORT_XFC_POLLMODE

			// Semaphore
			if( xfc->buslock != 0 ){
				xSemaphoreGive_freertos(xfc);
				_xfc_buslock[xfc->dev_unit] = 0;
			}

			_xfc_handler[xfc->dev_unit] = NULL;

			// Skip
		}
	}

	return TRUE;
}

/******************************************************************************
 *  XFC Control
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int  xfc_set_format(XFC_HANDLER_TYPE *xfc, UINT32 format)
{
	switch(format){
		case	XFC_TYPE_MOTOROLA_O0H0:
			XFC_REG_ACCESS( xfc->reg_spi->SPI_INT_MODE = XFC_SPI_CPOLCPHASE_MODE0 );
			break;
		case	XFC_TYPE_MOTOROLA_O1H1:
			XFC_REG_ACCESS( xfc->reg_spi->SPI_INT_MODE = XFC_SPI_CPOLCPHASE_MODE3 );
			break;

		case	XFC_TYPE_MOTOROLA_O0H1:
		case	XFC_TYPE_MOTOROLA_O1H0:
		default:
			return FALSE;
	}

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_coreclock(XFC_HANDLER_TYPE *xfc, UINT32 speed)
{
	if( speed == 0 ){
		return FALSE;
	}

	xfc->coreclock = speed ;

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_speed(XFC_HANDLER_TYPE *xfc, UINT32 speed)
{
	UINT32 coreclock, margintick;

	if( speed == 0 ){
		return FALSE;
	}

	XFC_REG_ACCESS( coreclock = XFC_SPI_GET_CORE_FREQ_MHz(xfc->reg_cmn->XFC_CORE_FREQ) );

	if( XFC_SPI_CHK_SAFE_Hz( speed, (coreclock*MHz) ) ){
		XFC_REG_ACCESS( xfc->reg_spi->SPI_CLK_FREQ_S = XFC_SPI_SET_FREQ_MHz(speed/MHz) );
		XFC_REG_ACCESS( xfc->reg_dma->DMA_CLK_FREQ_S = XFC_SPI_SET_FREQ_MHz(speed/MHz) );
		xfc->spiclock =speed ;
	}else{
		coreclock = coreclock >> 1;
		XFC_REG_ACCESS( xfc->reg_spi->SPI_CLK_FREQ_S = XFC_SPI_SET_FREQ_MHz(coreclock) );
		XFC_REG_ACCESS( xfc->reg_dma->DMA_CLK_FREQ_S = XFC_SPI_SET_FREQ_MHz(coreclock) );
		xfc->spiclock = coreclock * MHz ;
	}

	margintick = ((speed / (10*MHz)) >> 2);
	XFC_REG_ACCESS( xfc->reg_cmn->CLK_CE_SEL_CYC = margintick + 1 );
	XFC_REG_ACCESS( xfc->reg_cmn->CLK_CE_DES_CYC = margintick + 3 );

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_get_speed(XFC_HANDLER_TYPE *xfc, UINT32 *speed)
{
	UINT32 coreclock;
	UINT32 divider;

	if( speed == NULL ){
		return FALSE;
	}

	XFC_REG_ACCESS( coreclock = XFC_SPI_GET_CORE_FREQ_MHz(xfc->reg_cmn->XFC_CORE_FREQ) );
	XFC_REG_ACCESS( divider = xfc->reg_spi->SPI_CLK_FREQ_R );

	*speed = (coreclock*MHz) / (divider+1) ;

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_timeout(XFC_HANDLER_TYPE *xfc, UINT32 unit, UINT32 count)
{
	UINT32 coreclock;

	XFC_REG_ACCESS( coreclock = XFC_SPI_GET_CORE_FREQ_MHz(xfc->reg_cmn->XFC_CORE_FREQ) );

	if( coreclock == 0 ){
		return FALSE;
	}

	XFC_REG_ACCESS( xfc->reg_cmn->CLK_TO_UNIT = unit );
	XFC_REG_ACCESS( xfc->reg_cmn->CLK_TO_COUNT = count );

	XFC_DBG_NONE("XFC:TO - U:%d/C:%d", unit, count);

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_wiremodel(XFC_HANDLER_TYPE *xfc, UINT32 unit, UINT32 model)
{
	if( unit >= XFC_CSEL_MAX ){
		return FALSE;
	}

	if( model == 3 ){
		XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_1_TYPE &= ~(XPIN_3WIRE(unit)) );
	}else{
		XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_1_TYPE |= (XPIN_4WIRE(unit)) );
	}

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_dma_config(XFC_HANDLER_TYPE *xfc, UINT32 config)
{
	XFC_REG_ACCESS( xfc->reg_dma->DMA_MP0_CFG = config );

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_pincontrol(XFC_HANDLER_TYPE *xfc, UINT32 config)
{
	if( config == TRUE ){ // Quad
		XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_DX_TYPE = XFC_PIN_WPHOLD_DISABLE );
	}else{
		XFC_REG_ACCESS( xfc->reg_spi->SPI_IO_DX_TYPE = XFC_PIN_WPHOLD_ENABLE  );
	}
	XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_CLR_OP = XFC_XIP_ACC_OP_EN|XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN );
	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_set_cloning(XFC_HANDLER_TYPE *xfc, UINT32 *config)
{
	if( config[1] == 0x00){
		// Skip
		return TRUE;
	}

	XFC_REG_ACCESS( xfc->reg_xip->XIP_CLK_FREQ_S = xfc->reg_spi->SPI_CLK_FREQ_S  );
	//XFC_REG_ACCESS( xfc->reg_xip->XIP_CLK_FREQ_R = xfc->reg_spi->SPI_CLK_FREQ_R  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_DMY_CYC = XFC_GET_SPI_DUMMY_CYCLE(xfc->busctrl[0]) );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_WAIT_CYC = xfc->reg_spi->SPI_WAIT_CYC  );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_INT_MODE = xfc->reg_spi->SPI_INT_MODE  );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_PHASE_MODE = (xfc->busctrl[0]) & (~XFC_SPI_PHASE_DUMMY_MASK) );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_PHASE_MODE &= (~XFC_SPI_TIMEOUT_EN)  );
	if( config[0] != 0x00 ){
		XFC_REG_ACCESS( xfc->reg_xip->XIP_PHASE_MODE &= (~XFC_SPI_PHASE_MODE_1BIT)  );
		switch(config[0]){
		case	0xA5:
			XFC_REG_ACCESS( xfc->reg_xip->XIP_PHASE_MODE |= (XFC_SPI_PHASE_MODE_8BIT)  );
			break;
		case	0xA0:
			XFC_REG_ACCESS( xfc->reg_xip->XIP_PHASE_MODE |= (XFC_SPI_PHASE_MODE_4BIT)  );
			break;
		default:
			break;
		}
	}

	XFC_REG_ACCESS( xfc->reg_xip->XIP_BYTE_SWAP = xfc->reg_spi->SPI_BYTE_SWAP  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_WRAP_EN = xfc->reg_spi->SPI_WRAP_EN  );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_BUS_TYPE = xfc->busctrl[1] );
	if( config[0] != 0x00 ){
		UINT32 busmask;
		busmask = XFC_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(0,1,XFC_BUS_OPI)
					, SPI_BUS_TYPE(1,1,XFC_BUS_OPI) // ADDR
					, SPI_BUS_TYPE(1,1,XFC_BUS_OPI) // MODE
					, SPI_BUS_TYPE(1,1,XFC_BUS_OPI) // DATA
				);
		XFC_REG_ACCESS( xfc->reg_xip->XIP_BUS_TYPE &= busmask  );
	}

	XFC_REG_ACCESS( xfc->reg_xip->XIP_IO_DX_TYPE = xfc->reg_spi->SPI_IO_DX_TYPE  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_IO_CK_TYPE = xfc->reg_spi->SPI_IO_CK_TYPE  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_IO_CS_TYPE = xfc->reg_spi->SPI_IO_CS_TYPE  );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_CS_SEL = xfc->reg_spi->SPI_CS_SEL  );
	XFC_REG_ACCESS( xfc->cselbkup = xfc->cselinfo ); // MPW issue

	XFC_REG_ACCESS( xfc->reg_xip->XIP_IO_1_TYPE = xfc->reg_spi->SPI_IO_1_TYPE  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_TWIN_QSPI_EN = xfc->reg_spi->SPI_TWIN_QSPI_EN  );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_REG_RW = 0 );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_REG_CMD = config[1]  );
	XFC_REG_ACCESS( xfc->reg_xip->XIP_REG_MODE = xfc->reg_spi->SPI_REG_MODE );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_DLY_INDEX = xfc->reg_spi->SPI_DLY_INDEX );

	XFC_REG_ACCESS( xfc->reg_xip->XIP_SPI_BST = sizeof(UINT32) );

	XFC_REG_ACCESS( xfc->reg_cmn->XFC_REQ_CLR_OP = XFC_XIP_ACC_OP_EN|XFC_NON_DMA_OP_EN|XFC_DMA_ACC_OP_EN );

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static void xfc_isr_convert(XFC_HANDLER_TYPE *xfc, UINT32 *pirqstatus, UINT32 *psubirq)
{
	UINT32 			irqstatus, subirq;

	XFC_REG_ACCESS_ISR( irqstatus = xfc->reg_cmn->XFC_IRQ_STATUS );
	irqstatus = irqstatus & (~XFC_XIP_ACC_OP_EN);
	XFC_REG_ACCESS_DUMP( irqstatus );

	subirq = 0;
	if ( (irqstatus & XFC_TIMEOUT_OP_EN) != 0 ){
		subirq |= XFC_IRQ_STATUS_TO_FORCE;
	}

	if ( (irqstatus & XFC_NON_DMA_OP_EN) != 0 ){
		XFC_REG_ACCESS_ISR( subirq |= xfc->reg_cmn->IRQ_STATUS_SPI );

		if( (subirq & XFC_IRQ_STATUS_TXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_TXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_AHB_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_SET_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_DONE) != 0 ){
		}
	}

	if ( (irqstatus & XFC_DMA_ACC_OP_EN) != 0 ){
		XFC_REG_ACCESS_ISR( subirq |= xfc->reg_cmn->IRQ_STATUS_DMA );

		if( (subirq & XFC_IRQ_STATUS_TXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_TXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_AHB_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_SET_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_DONE) != 0 ){
		}
	}

	XFC_REG_ACCESS_DUMP( subirq );

	*pirqstatus = irqstatus;
	*psubirq = subirq;
}

/******************************************************************************
 *  ISR management
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int  xfc_isr_create(HANDLE handler)
{
	XFC_HANDLER_TYPE	*xfc;

	if(handler == NULL){
		return FALSE;
	}

	xfc = (XFC_HANDLER_TYPE *) handler ;

	// Disable All Interrupts

	return TRUE;

}

ATTRIBUTE_RAM_FUNC
static int  xfc_isr_init(HANDLE handler)
{
	XFC_HANDLER_TYPE	*xfc;

	if(handler == NULL){
		return FALSE;
	}

	xfc = (XFC_HANDLER_TYPE *) handler ;

	// Registry LISR
	switch(xfc->dev_unit){
	case	XFC_UNIT_0:
		_sys_nvic_write(XFC_IRQn, (void *)_tx_specific_xfc0_interrupt, 0x7);
		_sys_nvic_write(XCACHE_IRQn, (void *)_tx_specific_xcch0_interrupt, 0x7);
		break;
	}

	// Disalbe All Interrupts

	return TRUE;
}

ATTRIBUTE_RAM_FUNC
static int  xfc_isr_close(HANDLE handler)
{
	XFC_HANDLER_TYPE	*xfc;

	if(handler == NULL){
		return FALSE;
	}

	xfc = (XFC_HANDLER_TYPE *) handler ;

	// Disable All Interrupts

	// UN-Registry LISR
	switch(xfc->dev_unit){
		case	XFC_UNIT_0:
			_sys_nvic_write(XFC_IRQn, NULL, 0x7);
			_sys_nvic_write(XCACHE_IRQn, NULL, 0x7);
			break;
	}

	return TRUE;

}

#if 0
ATTRIBUTE_RAM_FUNC
static int  xfc_callback_registry(HANDLE handler, UINT32 index
			, UINT32 callback, UINT32 userhandler)
{
	return TRUE;
}
#endif /*0*/

/******************************************************************************
 *  ISR
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static void _tx_specific_xfc0_interrupt(void)
{
	xfc_isr_core(XFC_UNIT_0);
	//INTR_CNTXT_CALL_PARA(xfc_isr_core, XFC_UNIT_0);
}

ATTRIBUTE_RAM_FUNC
void xfc_isr_core(UINT32 idx)
{
	XFC_HANDLER_TYPE	*xfc;
	UINT32 			irqstatus, subirq;
	
	xfc = _xfc_handler[idx];

	xfc_isr_convert(xfc, &irqstatus, &subirq);

#ifdef	SUPPORT_XFC_POLLMODE
#else	//SUPPORT_XFC_POLLMODE
	if( subirq != 0 ){
		xEventGroupSetBits((OAL_EVENT_GROUP *)(xfc->event), subirq);
	}
#endif	//SUPPORT_XFC_POLLMODE

	XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ = XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );
}

ATTRIBUTE_RAM_FUNC
static void _tx_specific_xcch0_interrupt(void)
{
	xfc_isr_core(XFC_UNIT_0);
	//INTR_CNTXT_CALL_PARA(xfc_isr_core, XFC_UNIT_0);
}

ATTRIBUTE_RAM_FUNC
void xcch0_isr_core(UINT32 idx)
{
	XFC_HANDLER_TYPE	*xfc;
	UINT32 			irqstatus, subirq;

	xfc = _xfc_handler[idx];

	if( xfc == NULL ){
		return;
	}

	XFC_REG_ACCESS_ISR( irqstatus = xfc->reg_cmn->XFC_IRQ_STATUS );
	irqstatus = irqstatus & (XFC_XIP_ACC_OP_EN);
	XFC_REG_ACCESS_DUMP( irqstatus );

	subirq = 0;
	if( (irqstatus & XFC_XIP_ACC_OP_EN) != 0 ){
		XFC_REG_ACCESS_ISR( subirq = xfc->reg_cmn->IRQ_STATUS_XIP );

		if( (subirq & XFC_IRQ_STATUS_TXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_TXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_UNDR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_RXFIFO_OVER) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_AHB_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_SET_ERR) != 0 ){
		}
		if( (subirq & XFC_IRQ_STATUS_ACC_DONE) != 0 ){
		}
	}

	XFC_REG_ACCESS_DUMP( subirq );

#ifdef	SUPPORT_XFC_POLLMODE
#else	//SUPPORT_XFC_POLLMODE
	if( subirq != 0 ){
		xEventGroupSetBitsFromISR((OAL_EVENT_GROUP *)(xfc->event), subirq);
	}
#endif	//SUPPORT_XFC_POLLMODE

	XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_REQ_CLR_IRQ = XFC_REQ_SET_IRQMASK(irqstatus)|irqstatus );
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static void xfc_set_delay_parameter(XFC_HANDLER_TYPE *xfc, UINT8 *config)
{
	if ( (config == NULL) || (config[0] == 0) ) {
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_0_RXC = XFC_SPI_DELAY_RXC(XFC_RX_1dot5_CYCLE,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_1_RXC = XFC_SPI_DELAY_RXC(XFC_RX_1dot5_CYCLE,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_2_RXC = XFC_SPI_DELAY_RXC(XFC_RX_1dot5_CYCLE,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_3_RXC = XFC_SPI_DELAY_RXC(XFC_RX_1dot5_CYCLE,0) );

		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_0_TXC = XFC_SPI_DELAY_TXC(0,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_1_TXC = XFC_SPI_DELAY_TXC(0,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_2_TXC = XFC_SPI_DELAY_TXC(0,0) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_3_TXC = XFC_SPI_DELAY_TXC(0,0) );
	}else{
		// howto? SPI_DLY_INDEX
		xfc->dlytab = config;

		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_0_RXC = ((xfc->dlytab)[ 4]<<2) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_1_RXC = ((xfc->dlytab)[ 5]<<2) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_2_RXC = ((xfc->dlytab)[ 6]<<2) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_3_RXC = ((xfc->dlytab)[ 7]<<2) );

		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_0_TXC = ((xfc->dlytab)[ 8]<<1) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_1_TXC = ((xfc->dlytab)[ 9]<<1) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_2_TXC = ((xfc->dlytab)[10]<<1) );
		XFC_REG_ACCESS_ISR( xfc->reg_cmn->XFC_SPI_DLY_3_TXC = ((xfc->dlytab)[11]<<1) );
	}
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static void xfc_set_delay_index(XFC_HANDLER_TYPE *xfc)
{
	if( (xfc->dlytab != NULL) && (xfc->dlytab[0] != 0) ){
		UINT32 idx;
		for( idx = 0; idx < 4; idx++ ){
			// Delay Model changed 18.06.26
			if( xfc->reg_cmn->XFC_CORE_FREQ <= (xfc->dlytab[idx]) ){
				break;
			}
		}
		XFC_REG_ACCESS( xfc->reg_spi->SPI_DLY_INDEX = idx );

		XFC_REG_ACCESS( xfc->reg_dma->DMA_DLY_INDEX = idx );
	}else{
		XFC_REG_ACCESS( xfc->reg_spi->SPI_DLY_INDEX = 0 );

		XFC_REG_ACCESS( xfc->reg_dma->DMA_DLY_INDEX = 0 );
	}
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static void xfc_set_pll_div(XFC_HANDLER_TYPE *xfc)
{
#ifdef	BUILD_OPT_DA16200_ASIC
	UINT32 xfcclock;
	UINT32 pllhz, xtalhz, busclock;

	// Set Clock Div.
	DA16200_SYSCLOCK->CLK_EN_XFC = 0;

	DA16200_SYSCLOCK->PLL_CLK_EN_1_XFC = 0;

	DA16200_SYSCLOCK->PLL_CLK_DIV_1_XFC = da16x_pll_x2check(); // x2 or x1

	DA16200_SYSCLOCK->PLL_CLK_EN_1_XFC = 1;
	DA16200_SYSCLOCK->SRC_CLK_SEL_1_XFC = 1; // PLL

	DA16200_SYSCLOCK->CLK_EN_XFC = 1;

	// Set Clock Freq.
	da16x_get_default_xtal( &xtalhz, &busclock );
	xtalhz = xtalhz / KHz ;
	pllhz  = da16x_pll_getspeed(xtalhz);
	xfcclock  = pllhz / (DA16X_CLK_DIV(DA16200_SYSCLOCK->PLL_CLK_DIV_1_XFC) + 1);

	if ( DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU == DA16200_SYSCLOCK->PLL_CLK_DIV_1_XFC ) {
		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_RATIO = XFC_CORE_SET_RATIO(1) ); // x1
	}else{
		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_RATIO = XFC_CORE_SET_RATIO(2) ); // x2
	}

	XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_FREQ = XFC_SPI_SET_CORE_FREQ_MHz(xfcclock/KHz) );

	xfc_set_delay_index( xfc );

	//XFC_REG_ACCESS( xfc->reg_xip->XIP_DLY_INDEX = xfc->reg_spi->SPI_DLY_INDEX );

#else	//BUILD_OPT_DA16200_ASIC
	UINT32 xfcclock;
	UINT32 pllhz, xtalhz, busclock;

	//ISSUE: The first read data is invalid after running the steps below !
	//DA16200_SYSCLOCK->CLK_EN_XFC = 0;
	//NOP_EXECUTION();
	//NOP_EXECUTION();
	//DA16200_SYSCLOCK->CLK_EN_XFC = 1;

#ifdef	SUPPORT_XFC_TEST_180713A
	DA16200_SYSCLOCK->CLK_EN_XFC = 1; /* TEST Model 180713A */
#endif	//SUPPORT_XFC_TEST_180713A

	// Set Clock Freq.
	da16x_get_default_xtal( &xtalhz, &busclock );
	xtalhz = xtalhz / KHz ;
	pllhz  = da16x_pll_getspeed(xtalhz);
	xfcclock  = pllhz ;

	if( DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU == DA16200_SYSCLOCK->PLL_CLK_DIV_1_XFC ){
		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_RATIO = XFC_CORE_SET_RATIO(1) ); // x1

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_FREQ = XFC_SPI_SET_CORE_FREQ_MHz(xfcclock/KHz) );
	}else{
		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_RATIO = XFC_CORE_SET_RATIO(2) ); // x2

		XFC_REG_ACCESS( xfc->reg_cmn->XFC_CORE_FREQ = XFC_SPI_SET_CORE_FREQ_MHz((xfcclock*2)/KHz) );
	}

	xfc_set_delay_index( xfc );

	//XFC_REG_ACCESS( xfc->reg_xip->XIP_DLY_INDEX = xfc->reg_spi->SPI_DLY_INDEX );

#endif	//BUILD_OPT_DA16200_ASIC
}

ATTRIBUTE_RAM_FUNC
static void xfc_pll_callback(UINT32 clock, void *param)
{
	XFC_HANDLER_TYPE *xfc;

	xfc = (XFC_HANDLER_TYPE*) param;

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
#ifdef	BUILD_OPT_DA16200_ASIC
#else	//BUILD_OPT_DA16200_ASIC
#ifdef	SUPPORT_XFC_TEST_180713A
		DA16200_SYSCLOCK->CLK_EN_XFC = 0; /* TEST Model 180713A */
#endif	//SUPPORT_XFC_TEST_180713A
#endif	//BUILD_OPT_DA16200_ASIC
		return;
	}

	xfc_set_pll_div(xfc);

	if( xfc_set_coreclock( xfc, clock ) == TRUE ){
		xfc_set_speed(xfc, xfc->spiclock);
	}
}

