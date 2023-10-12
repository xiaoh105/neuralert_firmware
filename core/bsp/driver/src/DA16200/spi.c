/**
 ****************************************************************************************
 *
 * @file spi.c
 *
 * @brief SPI Controller
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
#include "common_def.h"

//---------------------------------------------------------
//
//---------------------------------------------------------

#undef	SUPPORT_SPI_DEBUG
//#define	SUPPORT_SPI_DEBUG

#undef	SUPPORT_SPI_POLLMODE
//#define	SUPPORT_SPI_POLLMODE

#define SUPPORT_SPI_TEST_180713A

#define	SPI_IRQn		ESH_Ctrl_IRQn
//#define	XCACHE_IRQn		Cache_Ctrl_IRQn

#define TEST_SPI_REQ_CLR_OP

#define SUPPORT_SPI_SHORT_WR_INTERVAL


#define	DA16X_SPI_COMMON_BASE	(DA16X_SPI_BASE|0x0000)
#define	DA16X_SPI_SPI_BASE	(DA16X_SPI_BASE|0x0100)
#define	DA16X_SPI_DMA_BASE	(DA16X_SPI_BASE|0x0200)
#define	DA16X_SPI_XIP_BASE	(DA16X_SPI_BASE|0x0300)

//---------------------------------------------------------
// Common Domain
//---------------------------------------------------------

#define	SPI_TIMEOUT_OP_EN 		(1<<3)
#define	SPI_XIP_ACC_OP_EN 		(1<<2)
#define	SPI_NON_DMA_OP_EN 		(1<<1)
#define	SPI_DMA_ACC_OP_EN 		(1<<0)

#define	SPI_REQ_GET_IRQMASK(x)		((x)>>8)
#define	SPI_REQ_SET_IRQMASK(x)		((x)<<8)

#define	SPI_REQ_GET_CLR_STATUS(x)	((x)>>8)
#define	SPI_REQ_SET_CLR_STATUS(x)	((x)<<8)

#define SPI_IRQ_STATUS_ALLMARK		(0x0000C0FF)
#define SPI_IRQ_STATUS_SW_FORCE		(1<<31)
#define SPI_IRQ_STATUS_TO_FORCE		(1<<14)
#define SPI_IRQ_STATUS_TXFIFO_UNDR	(1<<7)
#define SPI_IRQ_STATUS_TXFIFO_OVER	(1<<6)
#define SPI_IRQ_STATUS_RXFIFO_UNDR	(1<<5)
#define SPI_IRQ_STATUS_RXFIFO_OVER	(1<<4)
#define SPI_IRQ_STATUS_AHB_ERR		(1<<3)
#define SPI_IRQ_STATUS_ACC_ERR		(1<<2)
#define SPI_IRQ_STATUS_SET_ERR		(1<<1)
#define	SPI_IRQ_STATUS_ACC_DONE		(1<<0)

#define SPI_CORE_CLK_RATIO(x)		((x)&0x0F)
#define SPI_CORE_SET_FREQ_MHz(x)	(((x)-1)&0x01FF)
#define SPI_CORE_GET_FREQ_MHz(x)	(((x)&0x01FF)+1)

#define SPI_TIMEOUT_UNIT_8us(x)		((x)&0x07)
#define SPI_TIMEOUT_COUNT(x)		((x)&0x1F)

#define SPI_CE_SEL_CYCLE(x)		((x)&0x07)
#define SPI_CE_DESEL_CYCLE(x)		((x)&0x7F)

#define SPI_SPI_DELAY_TXC_EN		(1<<0)
#define SPI_SPI_DELAY_RXC_EN		(1<<1)
#define SPI_SPI_DELAY_TXCRXC_EN		(1<<2)

#define	SPI_SPI_DELAY_TXC(pol,cyc)	(((pol)<<8)|((cyc)&0x0FF))
#define	SPI_SPI_DELAY_RXC(pha,cyc)	((((pha)&0x03)<<8)|((cyc)&0x0FF))
#define SPI_RX_0dot0_CYCLE		(0)
#define SPI_RX_0dot5_CYCLE		(1)
#define SPI_RX_1dot0_CYCLE		(2)
#define SPI_RX_1dot5_CYCLE		(3)

//---------------------------------------------------------
// SPI Domain
//---------------------------------------------------------

#define SPI_SPI_SET_CORE_FREQ_MHz(x)	(((x)-1)&0x01FF)
#define SPI_SPI_GET_CORE_FREQ_MHz(x)	(((x)&0x01FF)+1)

#define SPI_SPI_SET_FREQ_MHz(x)		(((x))&0x01FF)
#define SPI_SPI_GET_FREQ_MHz(x)		(((x)&0x01FF))

#define SPI_SPI_CHK_SAFE_Hz(spi,core)	(((spi)<<1)<=(core))

#define SPI_CORE_SET_RATIO(x)		((x)-1)
#define SPI_CORE_GET_RATIO(x)		((x)+1)

#define	SPI_SPI_CPOLCPHASE_MODE0	(0)
#define	SPI_SPI_CPOLCPHASE_MODE3	(3)

#define SPI_SPI_BYTE_SWAP_NORMAL	(0)
#define SPI_SPI_BYTE_SWAP_BE32		(1)
#define SPI_SPI_BYTE_SWAP_BE16		(2)
#define SPI_SPI_BYTE_SWAP_LE16		(3)

#define SPI_SPI_WRAP_AROUND_EN		(1)

#define SPI_SPI_IOPIN_AUTO()		(0)
#define SPI_SPI_IOPIN_MANUAL(dir,lvl,man)	(((lvl)<<16)|((dir)<<8)|(man))
#define XPIN_OUT(x)			(1<<(x))
#define XPIN_IN(x)			(0)
#define XPIN_HIGH(x)			(1<<(x))
#define XPIN_LOW(x)			(0)

#define SPI_SPI_CLKPIN_AUTO()		(0)
#define SPI_SPI_CLKPIN_MANUAL(dir,lvl,man)	(((lvl)<<2)|((dir)<<1)|(man))

#define SPI_SPI_CSELPIN_AUTO(pol)	((pol)<<24)
#define SPI_SPI_CSELPIN_MANUAL(dir,lvl,man)	(((lvl)<<16)|((dir)<<8)|(man))

#define SPI_SPI_SET_CSEL_LOCK(pin)	(pin)

#define	XPIN_3WIRE(x)			(0)
#define	XPIN_4WIRE(x)			(1<<(x))

#define SPI_SPI_SET_TWIN_ENABLE(pin)	(pin)

#define	SPI_SPI_DATA_RD			(0)
#define	SPI_SPI_DATA_WR			(1)

#define SPI_SPI_TOTAL_LEN_BYTE(x)	((x)&0x0FFFFF)
#define SPI_SPI_MAX_LENGTH		((2*MBYTE)-1)

#define SPI_SPI_OPPHASE_IDLE		(0)
#define SPI_SPI_OPPHASE_CMD		(1)
#define SPI_SPI_OPPHASE_ADDR		(2)
#define SPI_SPI_OPPHASE_DUMMY		(3)
#define SPI_SPI_OPPHASE_WRDATA		(4)
#define SPI_SPI_OPPHASE_RDDATA		(5)
#define SPI_SPI_OPPHASE_WAIT		(6)
#define SPI_SPI_OPPHASE_ERROR		(7)

#define SPI_SPI_SET_CSEL(x)		(1<<(x))

#define	SPI_PIN_WPHOLD_DISABLE		(0x00000000)
#define	SPI_PIN_WPHOLD_ENABLE		(0x00cccccc)

//---------------------------------------------------------
// DMA Domain
//---------------------------------------------------------

#define SPI_DMA_TASK_SIZE(x)		(((x)&0x03)<<8)
#define SPI_DMA_OP_RD			(1)
#define SPI_DMA_OP_WR			(0)

//---------------------------------------------------------
// XIP Domain
//---------------------------------------------------------

//---------------------------------------------------------
//
//---------------------------------------------------------

#define	SPI_MEM_MALLOC(x)	pvPortMalloc(x)
#define	SPI_MEM_FREE(x)		vPortFree(x)

//#define SPI_TRACE_PRINT(...)	PRINTF(__VA_ARGS__)
#define SPI_TRACE_PRINT(...)


#ifdef	SUPPORT_SPI_DEBUG
#undef  SUPPORT_SPI_DEBUG_DBGT
#ifdef	SUPPORT_SPI_DEBUG_DBGT
#define SPI_REG_ACCESS(ACCESS)		DBGT_Printf( "SPI: " #ACCESS " at line %d\n", __LINE__ ); ACCESS
#define SPI_REG_ACCESS_ISR(ACCESS)	DBGT_Printf( "SPI: " #ACCESS " at line %d\n", __LINE__ ); ACCESS
#define SPI_REG_ACCESS_DUMP(x)		DBGT_Printf( "SPI: %08x\n", x)
#else
#define SPI_REG_ACCESS(ACCESS)		DRV_DBG_NONE( #ACCESS " @ %d", __LINE__ ); ACCESS
#define SPI_REG_ACCESS_ISR(ACCESS)	/*DRV_DBG_NONE( #ACCESS " @ %d", __LINE__ );*/ ACCESS
#define SPI_REG_ACCESS_DUMP(x)
#endif

#define	SPI_DBG_NONE(...)	DRV_DBG_NONE( __VA_ARGS__ )
#define	SPI_DBG_BASE(...)	DRV_DBG_BASE( __VA_ARGS__ )
#define	SPI_DBG_INFO(...)	DRV_DBG_INFO( __VA_ARGS__ )
#define	SPI_DBG_WARN(...)	DRV_DBG_WARN( __VA_ARGS__ )
#define	SPI_DBG_ERROR(...)	DRV_DBG_ERROR( __VA_ARGS__ )
#define	SPI_DBG_DUMP(...)	DRV_DBG_DUMP( __VA_ARGS__ )
#define	SPI_DBG_SPECIAL(...)	DRV_DBG_INFO( __VA_ARGS__ )
#else
#define SPI_REG_ACCESS(ACCESS)		ACCESS
#define SPI_REG_ACCESS_ISR(ACCESS)	ACCESS
#define SPI_REG_ACCESS_DUMP(x)

#define	SPI_DBG_NONE(...)
#define	SPI_DBG_BASE(...)
#define	SPI_DBG_INFO(...)
#define	SPI_DBG_WARN(...)	DRV_DBG_WARN( __VA_ARGS__ )
#define	SPI_DBG_ERROR(...)	DRV_DBG_ERROR( __VA_ARGS__ )
#define	SPI_DBG_DUMP(...)
#define	SPI_DBG_SPECIAL(...)	//DRV_DBG_INFO( __VA_ARGS__ )
#endif

//---------------------------------------------------------
//
//---------------------------------------------------------

SPI_HANDLER_TYPE	*_spi_handler[SPI_UNIT_MAX];
static int		_spi_instance[SPI_UNIT_MAX];
VOID *			_spi_buslock[SPI_UNIT_MAX];
static CLOCK_LIST_TYPE	_spi_pll_info[SPI_UNIT_MAX];


static int  spi_isr_create(HANDLE handler);
static int  spi_isr_init(HANDLE handler);
static int  spi_isr_close(HANDLE handler);
static void _tx_specific_spi0_interrupt(void);
void spi_isr_core(UINT32 idx);

static void spi_set_pll_div(SPI_HANDLER_TYPE *spi);
static void spi_pll_callback(UINT32 clock, void *param);

static int  spi_set_format(SPI_HANDLER_TYPE *spi, UINT32 format);
static int  spi_set_coreclock(SPI_HANDLER_TYPE *spi, UINT32 speed);
static int  spi_set_speed(SPI_HANDLER_TYPE *spi, UINT32 speed);
static int  spi_get_speed(SPI_HANDLER_TYPE *spi, UINT32 *speed);
static int  spi_set_timeout(SPI_HANDLER_TYPE *spi, UINT32 unit, UINT32 count);
static int  spi_set_wiremodel(SPI_HANDLER_TYPE *spi, UINT32 unit, UINT32 model);
static int  spi_set_dma_config(SPI_HANDLER_TYPE *spi, UINT32 config);
static int  spi_set_pincontrol(SPI_HANDLER_TYPE *spi, UINT32 config);
static int  spi_set_cloning(SPI_HANDLER_TYPE *spi, UINT32 *config);
static void spi_isr_convert(SPI_HANDLER_TYPE *spi, UINT32 *pirqstatus, UINT32 *psubirq);
static void spi_set_delay_parameter(SPI_HANDLER_TYPE *spi, UINT8 *config);
static void spi_set_delay_index(SPI_HANDLER_TYPE *spi);

/*
 * default delay parameters
 */
static UINT8 dlytab[12] = {
		   30,
		   40,
		   60,
		   80,
		  0x0,   // SPI_SPI_DLY_0_RXC
		  0x1,   // SPI_SPI_DLY_1_RXC
		  0x2,   // SPI_SPI_DLY_2_RXC
		  0x3,   // SPI_SPI_DLY_3_RXC
		  0x0,   // SPI_SPI_DLY_0_TXC
		  0x0,   // SPI_SPI_DLY_1_TXC
		  0x0,   // SPI_SPI_DLY_2_TXC
		  0x0    // SPI_SPI_DLY_3_TXC
};

/******************************************************************************
 *  SPI_CREATE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	SPI_CREATE( UINT32 dev_id )
{
	SPI_HANDLER_TYPE *spi;

	// Allocate
	if( dev_id >= SPI_UNIT_MAX ){
		SPI_DBG_ERROR("SPI: ilegal unit, %d", dev_id);
		return NULL;
	}

	// Multi-instance
	if( _spi_handler[dev_id] == NULL ){
		spi = (SPI_HANDLER_TYPE *)SPI_MEM_MALLOC(sizeof(SPI_HANDLER_TYPE));
		if( spi == NULL ){
			return NULL;
		}
		DRV_MEMSET(spi, 0, sizeof(SPI_HANDLER_TYPE));
	}else{
		spi = _spi_handler[dev_id];
		if( spi == NULL ){
			return NULL;
		}
	}

	// Address Mapping
	switch((SPI_UNIT_LIST)dev_id){
		case	SPI_UNIT_0:
			spi->dev_unit = SPI_UNIT_0;
			spi->instance = (UINT32)(_spi_instance[SPI_UNIT_0]);
			_spi_instance[SPI_UNIT_0] ++;

			spi->reg_cmn = (SPI_COMMON_TYPE *)DA16X_SPI_COMMON_BASE;
			spi->reg_spi = (SPI_SPI_DOMAIN_TYPE *)DA16X_SPI_SPI_BASE;
			spi->reg_dma = (SPI_DMA_DOMAIN_TYPE *)DA16X_SPI_DMA_BASE;
			spi->reg_xip = (SPI_XIP_DOMAIN_TYPE *)DA16X_SPI_XIP_BASE;

			spi->buslock  = _spi_buslock[SPI_UNIT_0];
			spi->pllinfo  = (VOID *)&(_spi_pll_info[SPI_UNIT_0]);
			break;

		default:
			break;
	}

	if(spi->instance == 0){
		// Set Default Para
		SPI_DBG_INFO("(%p) SPI create, base %p", spi, (UINT32 *)spi->reg_cmn );

		_spi_handler[dev_id] = spi;

		spi->concat	= FALSE;
		spi->cselinfo 	= SPI_SPI_SET_CSEL(SPI_CSEL_0);
		SPI_REG_ACCESS( spi->cselbkup = spi->reg_xip->XIP_CS_SEL ); // MPW Issue
		spi->debug 	= FALSE;
		spi->maxlength	= (1*KBYTE); // under 1KB
		spi->dsize	= sizeof(UINT32);
		spi->xfermode	= SPI_SET_DMAMODE;
		spi->coreclock  = 0; // init value
		spi->spiclock   = 10*MHz ;
		spi->busctrl[0] = 0;
		spi->busctrl[1] = 0;
		spi->workbuf	= NULL;
		spi->event	= NULL;
		spi->dlytab	= NULL;

		// Create ISR
		spi_isr_create(spi);

		// PLL callback
		spi_set_pll_div(spi);

		{
			CLOCK_LIST_TYPE *pllinfo;

			pllinfo = (CLOCK_LIST_TYPE *)spi->pllinfo;

			pllinfo->callback.func = spi_pll_callback;
			pllinfo->callback.param = spi;
			pllinfo->next = NULL;

			_sys_clock_ioctl( SYSCLK_SET_CALLACK, pllinfo);
		}
	}
	else
	{
		SPI_DBG_INFO("(%p) SPI re-create, base %p", spi, (UINT32 *)spi->reg_cmn );
	}

	return (HANDLE) spi;
}

/******************************************************************************
 *  SPI_INIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_INIT(HANDLE handler)
{
	SPI_HANDLER_TYPE *spi;

	if(handler == NULL){
		return FALSE;
	}
	spi = (SPI_HANDLER_TYPE *)handler ;

	if( spi->instance != 0 ){
		return TRUE;
	}

	if( spi->buslock == NULL ){
		spi->buslock  = xSemaphoreCreateMutex();
		_spi_buslock[spi->dev_unit] = spi->buslock;
	}

#ifdef	SUPPORT_SPI_POLLMODE
	SPI_REG_ACCESS(
	    spi->reg_cmn->SPI_OP_EN
		= (SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN|SPI_XIP_ACC_OP_EN)
		);
#else	//SUPPORT_SPI_POLLMODE
	if( spi->event == NULL ){
		spi->event = xEventGroupCreate();
	}

	SPI_REG_ACCESS(
	    spi->reg_cmn->SPI_OP_EN
		= SPI_REQ_SET_IRQMASK(SPI_TIMEOUT_OP_EN|SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN)
		  | (SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN|SPI_XIP_ACC_OP_EN)
		);

#endif	//SUPPORT_SPI_POLLMODE

	if( spi_set_coreclock(spi, spi->coreclock) == TRUE ){
		spi_set_speed(spi, spi->spiclock);
	}

	if (spi->dlytab == NULL) {
		spi_set_delay_parameter(spi, NULL);
	}

	spi_isr_init(spi);

	return TRUE;
}


/******************************************************************************
 *  SPI_IOCTL( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_IOCTL(HANDLE handler, UINT32 cmd , VOID *data)
{
	SPI_HANDLER_TYPE *spi;
	int status;

	if(handler == NULL){
		return FALSE;
	}
	spi = (SPI_HANDLER_TYPE *)handler ;

	status = TRUE;

	switch(cmd)
	{
	case SPI_SET_DEBUG:
		spi->debug = TRUE;
		break;

	//-------------------------------------------------------
	case SPI_SET_CORECLOCK:
		status = spi_set_coreclock( spi, ((UINT32 *)data)[0] );
		if( status == TRUE ){
			status = spi_set_speed( spi, spi->spiclock );
		}
		break;

	case SPI_SET_SPEED:
		status = spi_set_speed( spi, ((UINT32 *)data)[0] );
		break;
	case SPI_GET_SPEED:
		status = spi_get_speed( spi, ((UINT32 *)data) );
		break;

	case SPI_GET_MAX_LENGTH:
		((UINT32 *)data)[0] = spi->maxlength;
		break;
	case SPI_SET_MAX_LENGTH:
		if( ((UINT32 *)data)[0] < SPI_SPI_MAX_LENGTH ){
			spi->maxlength = ((UINT32 *)data)[0];
		}else{
			spi->maxlength = SPI_SPI_MAX_LENGTH;
		}
		break;

	case SPI_SET_FORMAT:
		status = spi_set_format(spi, *((UINT32 *)data) );
		break;
	case SPI_SET_DATASIZE:
		spi->dsize = *((UINT32 *)data) ;
		break;

	case SPI_SET_INTRMODE:
		spi->xfermode = SPI_SET_INTRMODE ;
		break;
	case SPI_SET_DMAMODE:
		spi->xfermode = SPI_SET_DMAMODE ;
		break;
	case SPI_GET_XFERMODE:
		((UINT32 *)data)[0] = spi->xfermode;
		break;

	case SPI_SET_CALLACK:
		break;

	//-------------------------------------------------------
	case SPI_CHECK_BUSY:
		break;
	case SPI_CHECK_RX_FULL:
		break;
	case SPI_CHECK_RX_NO_EMPTY:
		break;
	case SPI_CHECK_TX_NO_FULL:
		break;
	case SPI_CHECK_TX_EMPTY:
		break;

	case SPI_SET_OUTEN:
		break;
	case SPI_SET_CONCAT:
		spi->concat	= TRUE;
		break;

	case SPI_GET_BUSCONTROL:
		((UINT32 *)data)[0] = spi->busctrl[0];
		((UINT32 *)data)[1] = spi->busctrl[1];
		break;
	case SPI_SET_BUSCONTROL:
		spi->busctrl[0] = ((UINT32 *)data)[0] ;
		spi->busctrl[1] = ((UINT32 *)data)[1] ;
		break;
	case SPI_SET_PINCONTROL:
		status = spi_set_pincontrol(spi, ((UINT32 *)data)[0]);
		break;
	case SPI_GET_DELAYSEL:
		((UINT32 *)data)[0] = (UINT32)( spi->dlytab );
		break;
	case SPI_SET_DELAYSEL:
		spi_set_delay_parameter(spi, (UINT8 *)data );
		break;
	case SPI_SET_DELAY_INDEX:
		spi->dlyidx = ((UINT32 *)data)[0];
		break;
	case SPI_GET_DELAY_INDEX:
		((UINT32 *)data)[0] = spi->dlyidx;
		break;
	case SPI_SET_TIMEOUT:
		// ((UINT32 *)data)[0] : Unit
		// ((UINT32 *)data)[1] : Count
		status = spi_set_timeout(spi, ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
		break;

	case SPI_SET_WIRE:
		// ((UINT32 *)data)[0] : Unit
		// ((UINT32 *)data)[1] : Wire model
		status = spi_set_wiremodel(spi, ((UINT32 *)data)[0], ((UINT32 *)data)[1]);
		break;

	case SPI_SET_DMA_CFG:
		// ((UINT32 *)data)[0] : Config
		status = spi_set_dma_config(spi, ((UINT32 *)data)[0]);
		break;

	case SPI_SET_XIPMODE:
		// ((UINT32 *)data)[0] : AXh
		// ((UINT32 *)data)[1] : Read OPCode
		status = spi_set_cloning(spi, ((UINT32 *)data));
		break;
	//-------------------------------------------------------
	case SPI_SET_LOCK:

		if( spi->buslock == NULL ){
			status =  FALSE;
		}else {
			if( ((UINT32 *)data)[0] == TRUE ){
				if (xSemaphoreTake( (spi->buslock), (UNSIGNED)((UINT32 *)data)[1] ) != pdPASS) {
					status =  FALSE;
				} else {
					spi->cselinfo = SPI_SPI_SET_CSEL( ((UINT32 *)data)[2] );
				}
			}else{
				xSemaphoreGive(spi->buslock);
				spi->cselinfo	= SPI_SPI_SET_CSEL(SPI_CSEL_MAX);
			}
		}

		break;

	// not support	-----------------------------------------
	case SPI_SET_SLAVEMODE:
	default:
		status = FALSE;
		break;
	}


	return status;
}

/******************************************************************************
 *  SPI_TRANSMIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_TRANSMIT(HANDLE handler, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen )
{
	SPI_HANDLER_TYPE *spi;

	if(handler == NULL){
		return FALSE;
	}
	spi = (SPI_HANDLER_TYPE *)handler ;

	spi->busctrl[0] = (SPI_SET_ON_UPDATE( spi->busctrl[0], 0x1111, 0, 0, 0, 1));	//@suppress("Suggested parenthesis around expression")

	/*
	 * SPI Host Controller can't send 1byte with data bus. Only can send command bus.
	 */
	if (sndlen == 1 && rcvlen == 0) {
		UINT8 * _buf = (UINT8 *)snddata;
		spi->busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI) // DATA
				);
		return SPI_SFLASH_TRANSMIT(handler, _buf[0], 0, 0
					, snddata, 0, rcvdata, 0 );
	}


	return SPI_SFLASH_TRANSMIT(handler, 0, 0, 0
				, snddata, sndlen, rcvdata, rcvlen );
}

/******************************************************************************
 *  SPI_SFLASH_TRANSMIT( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int	spi_sflash_interrupt_mode(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

static int	spi_sflash_dma_mode(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

#ifdef SUPPORT_SPI_SHORT_WR_INTERVAL
static int	spi_sflash_dma_mode_write_read(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

static int	spi_sflash_dma_mode_write_read_2(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );
#endif

int	SPI_SFLASH_TRANSMIT(HANDLE handler
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	SPI_HANDLER_TYPE *spi;
	INT32	spiilen;

	if(handler == NULL){
		return -1;
	}
	spi = (SPI_HANDLER_TYPE *)handler ;

	SPI_DBG_SPECIAL("SPI: cmd %02x addr %08x mod %02x snd %p/%d rcv %p/%d [%08x.%08x]"
			, cmd, addr, mode, snddata, sndlen, rcvdata, rcvlen
			, spi->busctrl[0], spi->busctrl[1]);


	spi_set_delay_index(spi);

	if( (spi->xfermode == SPI_SET_INTRMODE) || ((sndlen == 0) && (rcvlen == 0)) )
	{
		spiilen = spi_sflash_interrupt_mode( spi
				, cmd, addr, mode
				, snddata, sndlen, rcvdata, rcvlen );
	}else
	{
		if (sndlen > 0 && rcvlen > 0)
		{

#ifdef SUPPORT_SPI_SHORT_WR_INTERVAL
			if (sndlen <= 32)
			{
				spiilen = spi_sflash_dma_mode_write_read_2( spi
						, cmd, addr, mode
						, snddata, sndlen, rcvdata, rcvlen );
			} else {
				spiilen = spi_sflash_dma_mode_write_read( spi
						, cmd, addr, mode
						, snddata, sndlen, rcvdata, rcvlen );
			}
#else
			spiilen = spi_sflash_dma_mode( spi
					, cmd, addr, mode
					, snddata, sndlen, rcvdata, rcvlen );
#endif

		} else {
			spiilen = spi_sflash_dma_mode( spi
					, cmd, addr, mode
					, snddata, sndlen, rcvdata, rcvlen );
		}

	}

	//spi_set_timeout( spi, 0, 0 );
	SPI_REG_ACCESS( spi->reg_cmn->CLK_TO_UNIT = 0 );
	SPI_REG_ACCESS( spi->reg_cmn->CLK_TO_COUNT = 0 );
	spi->concat = FALSE;

	return spiilen;
}

static int	spi_sflash_interrupt_mode(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	spiilen, spimode, spi_2pass_on;
	UINT32  *snddata32;
	UINT32  *rcvdata32;

	spiilen = sndlen + rcvlen ;
	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;

	spimode  = (rcvlen == 0) ? 0 : 0x01;
	spimode |= (sndlen == 0) ? 0 : 0x02;

	do{
		UNSIGNED chkflag, maskevt;

		spi_2pass_on = 0;

		SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselinfo ); // MPW issue
		SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselinfo ); // MPW issue

		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

		SPI_REG_ACCESS( spi->reg_spi->SPI_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
		SPI_REG_ACCESS( spi->reg_spi->SPI_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
		SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = spi->busctrl[1] );

		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_CMD = cmd );
		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_ADDR = addr );
		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_MODE = mode );

		switch(spimode){
		case 0: // No-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(0) );
			break;
		case 1: // Read-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_RD );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen) );
			break;
		case 3: // Write-and-Read
			spi_2pass_on = 1;
			spimode = 1; // pass-2, read
			SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(1,0,1) );
#ifdef TEST_SPI_REQ_CLR_OP
			// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
			SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen) );
			SPI_REG_ACCESS( spi->reg_spi->SPI_ACC_DIN = *snddata32 );
			SPI_REG_ACCESS_DUMP( *snddata32 );
			break;

		default: // Write-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen) );
			SPI_REG_ACCESS( spi->reg_spi->SPI_ACC_DIN = *snddata32 );
			SPI_REG_ACCESS_DUMP( *snddata32 );
			break;
		}

		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_START = SPI_NON_DMA_OP_EN );

		// Check Access-Done
		chkflag = SPI_IRQ_STATUS_ALLMARK ;
#ifdef	SUPPORT_SPI_POLLMODE
		do{
			UINT32 irqstatus, subirq;

			spi_isr_convert(spi, &irqstatus, &subirq);

			SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
				= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );

			maskevt = (UNSIGNED)(subirq & chkflag);

		}while( (maskevt) == 0 );
#else	//SUPPORT_SPI_POLLMODE
		maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_SPI_POLLMODE

		if( spi_2pass_on == 0 && spimode == 1 ){
			SPI_REG_ACCESS( *rcvdata32 = spi->reg_spi->SPI_ACC_DOUT );
			SPI_REG_ACCESS_DUMP( *rcvdata32 );
		}
		if( spi_2pass_on == 0 && spi->concat == FALSE ){
			SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(0,0,0) );
#ifdef TEST_SPI_REQ_CLR_OP
			// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
			SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif
		}

		// Check Time-Out

		if( ((spi->busctrl[0] & SPI_SPI_TIMEOUT_EN) != 0)
		    && ((maskevt & SPI_IRQ_STATUS_TO_FORCE) == 0) ){
			chkflag = SPI_IRQ_STATUS_TO_FORCE ;
#ifdef	SUPPORT_SPI_POLLMODE
			do{
				UINT32 irqstatus, subirq;

				spi_isr_convert(spi, &irqstatus, &subirq);

				SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
					= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );

				maskevt = (UNSIGNED)(subirq & chkflag);

			}while( (maskevt) == 0 );
#else	//SUPPORT_SPI_POLLMODE
			maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_SPI_POLLMODE
		}

#ifdef	SUPPORT_SPI_POLLMODE
#else	//SUPPORT_SPI_POLLMODE
		// Clear
//			xEventGroupSetBits(spi->event, 0);
//		xEventGroupClearBits(spi->event, SPI_IRQ_STATUS_TO_FORCE);
#endif	//SUPPORT_SPI_POLLMODE

		SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselbkup ); // MPW issue
		SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselbkup ); // MPW issue
	}
	while( spi_2pass_on == 1 );

	return spiilen;
}

#ifdef SUPPORT_SPI_SHORT_WR_INTERVAL
/*
 * 32byte address �� tx data ����
 * 2�� ����
 */
static int	spi_sflash_dma_mode_write_read_2(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	spiilen, bus_type;
//	UINT32  *snddata32;
	UINT32  *rcvdata32;

	// FIXME: address hard codded
	UINT32 * p_spi_legacy_tx_reg_base = (UINT32 *)0x500b1190;
	UINT8 * snddata8;

	UNSIGNED chkflag, maskevt;

	spiilen = sndlen + rcvlen ;
//	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;
	snddata8 = (UINT8 *)snddata;

	/*************************************************************
	 *
	 * read
	 *
	 *************************************************************/

	SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselinfo ); // MPW issue
	SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselinfo ); // MPW issue

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	SPI_REG_ACCESS( spi->reg_spi->SPI_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
	SPI_REG_ACCESS( spi->reg_spi->SPI_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
//	SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = spi->busctrl[1] );

	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_CMD = cmd );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_ADDR = addr );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_MODE = mode );

	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_RD );
	SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen) );

	SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_RD );
	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen-1) ); /// fixed

	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );


	/*
	 * address 32byte mode
	 */

	SPI_REG_ACCESS( spi->reg_spi->SPI_LEGACY_EN = TRUE);
	SPI_REG_ACCESS( spi->reg_spi->SPI_LEGACY_TX_SIZE = sndlen);


	DRV_MEMCPY(p_spi_legacy_tx_reg_base, snddata8, sndlen);

	bus_type = 0x08000800;

	SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = bus_type );

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_START = SPI_DMA_ACC_OP_EN );

	chkflag = SPI_IRQ_STATUS_ALLMARK ;
	maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
	// Check Time-Out

	if( ((spi->busctrl[0] & SPI_SPI_TIMEOUT_EN) != 0)
		&& ((maskevt & SPI_IRQ_STATUS_TO_FORCE) == 0) ) {
		chkflag = SPI_IRQ_STATUS_TO_FORCE ;
		maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
	}
//		xEventGroupSetBits(spi->event, 0);
//	xEventGroupClearBits(spi->event, SPI_IRQ_STATUS_TO_FORCE);
	return spiilen;
}


static int	spi_sflash_dma_mode_write_read(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	spiilen;
	UINT32  *snddata32;
	UINT32  *rcvdata32;

	// FIXME: address hard codded
//	UINT32 * p_spi_legacy_tx_reg_base = (UINT32 *)0x500b1190;
//	UINT8 * snddata8;

	UNSIGNED chkflag, maskevt, val = 0;

	spiilen = sndlen + rcvlen ;
	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;
//	snddata8 = (UINT8 *)snddata;

	/*************************************************************
	 *
	 * DMA write initialization
	 *
	 *************************************************************/


	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselinfo ); // MPW issue
	SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselinfo ); // MPW issue

//		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	SPI_REG_ACCESS( spi->reg_spi->SPI_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
	SPI_REG_ACCESS( spi->reg_spi->SPI_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
	SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = spi->busctrl[1] );

	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_CMD = cmd );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_ADDR = addr );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_MODE = mode );


	// masking SPI interrupt
	SPI_REG_ACCESS(
	    spi->reg_cmn->SPI_OP_EN
		= (SPI_REQ_SET_IRQMASK(0)
				| (SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN|SPI_XIP_ACC_OP_EN))
		);

	SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(1,0,1) );

#ifdef TEST_SPI_REQ_CLR_OP
	// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif


#if 1
	/*
	 * org
	 */
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
	SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen) );

	SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_WR );
	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen-1) ); /// fixed
	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)snddata32 );
#else
	/*
	 * address 32byte mode
	 */

	SPI_TRACE_PRINT("snddata8: ");
	for (int i = 0; i < sndlen; i++)
	{
		SPI_TRACE_PRINT("%x", snddata8[i]);
	}
	SPI_TRACE_PRINT("\n");

	SPI_REG_ACCESS( spi->reg_spi->SPI_LEGACY_EN = TRUE);
	SPI_REG_ACCESS( spi->reg_spi->SPI_LEGACY_TX_SIZE = sndlen);

	for (int i = 0; i < sndlen; i++)
	{
		*p_spi_legacy_tx_reg_base = snddata8[i];
		p_spi_legacy_tx_reg_base += 1;
	}

#endif

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_START = SPI_DMA_ACC_OP_EN );

	// Check Access-Done
	chkflag = SPI_IRQ_STATUS_ALLMARK ;

#if 0
	/*
	 * interrupt status polling
	 */
	do{
		UINT32 irqstatus, subirq;

		spi_isr_convert(spi, &irqstatus, &subirq);

		SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
			= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );

		maskevt = (UNSIGNED)(subirq & chkflag);

	}while( (maskevt) == 0 );
#else
	/*
	 * dma done
	 */
	while (1)
	{
		SPI_REG_ACCESS_ISR(val = spi->reg_spi->SPI_OP_STATUS);

		// S_D_WR
		if ((val == 0) || (val == 6))
			break;
	}
#endif

	/*************************************************************
	 *
	 * read
	 *
	 *************************************************************/

	SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselinfo ); // MPW issue
	SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselinfo ); // MPW issue

//		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	SPI_REG_ACCESS( spi->reg_spi->SPI_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
	SPI_REG_ACCESS( spi->reg_spi->SPI_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
	SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = spi->busctrl[1] );

	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_CMD = cmd );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_ADDR = addr );
	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_MODE = mode );

	SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_RD );
	SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen) );

	SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_RD );
	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen-1) ); /// fixed

	SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );

	// unmasking SPI interrupt
	SPI_REG_ACCESS(
		spi->reg_cmn->SPI_OP_EN
		= (SPI_REQ_SET_IRQMASK(SPI_TIMEOUT_OP_EN|SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN)
				| (SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN|SPI_XIP_ACC_OP_EN))
		);

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_START = SPI_DMA_ACC_OP_EN );
	maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
	if( spi->concat == FALSE ){
		SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(0,0,0) );
#ifdef TEST_SPI_REQ_CLR_OP
		// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif
	}

	// Check Time-Out

	if( ((spi->busctrl[0] & SPI_SPI_TIMEOUT_EN) != 0)
		&& ((maskevt & SPI_IRQ_STATUS_TO_FORCE) == 0) ) {
		chkflag = SPI_IRQ_STATUS_TO_FORCE ;
		maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
	}
//		xEventGroupSetBits(spi->event, 0);
//	xEventGroupClearBits(spi->event, SPI_IRQ_STATUS_TO_FORCE);
	return spiilen;
}
#endif

static int	spi_sflash_dma_mode(SPI_HANDLER_TYPE *spi
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen )
{
	UINT32	spiilen, spimode, spi_2pass_on, bkupcfg = 0;
	UINT32  *snddata32;
	UINT32  *rcvdata32;

	spiilen = sndlen + rcvlen ;
	snddata32 = (UINT32 *)snddata;
	rcvdata32 = (UINT32 *)rcvdata;

	spimode  = (rcvlen == 0) ? 0 : 0x01;
	spimode |= (sndlen == 0) ? 0 : 0x02;

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	do{
		UNSIGNED chkflag, maskevt;

		spi_2pass_on = 0;

		SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselinfo ); // MPW issue
		SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselinfo ); // MPW issue

//		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

		SPI_REG_ACCESS( spi->reg_spi->SPI_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
		SPI_REG_ACCESS( spi->reg_spi->SPI_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
		SPI_REG_ACCESS( spi->reg_spi->SPI_BUS_TYPE = spi->busctrl[1] );

		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_CMD = cmd );
		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_ADDR = addr );
		SPI_REG_ACCESS( spi->reg_spi->SPI_REG_MODE = mode );


		switch(spimode){
		case 0: // No-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(0) );

			SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_WR );
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(0) );
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );
			break;
		case 1: // Read-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_RD );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen) );

			SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_RD );
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(rcvlen-1) ); /// fixed

			if( rcvdata32 != NULL ){
				SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)rcvdata32 );
			}else{
				// backup dma-config
				SPI_REG_ACCESS( bkupcfg = spi->reg_dma->DMA_MP0_CFG );
				// update dma-config
				bkupcfg = bkupcfg & ~(SPI_DMA_MP0_AI(SPI_ADDR_INCR));
				SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_CFG = bkupcfg );

				SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)(&rcvdata32) );
			}
			break;
		case 3: // Write-and-Read
			spi_2pass_on = 1;
			SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(1,0,1) );
#ifdef TEST_SPI_REQ_CLR_OP
			// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
			SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen) );

			SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_WR );
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen-1) ); /// fixed
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)snddata32 );
			break;

		default: // Write-Data
			SPI_REG_ACCESS( spi->reg_spi->SPI_REG_RW = SPI_SPI_DATA_WR );
			SPI_REG_ACCESS( spi->reg_spi->SPI_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen) );

			SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_TASK = SPI_DMA_TASK_SIZE(0)|SPI_DMA_OP_WR );
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_TTL = SPI_SPI_TOTAL_LEN_BYTE(sndlen-1) ); /// fixed
			SPI_REG_ACCESS( spi->reg_dma->DMA_TSK_0_Addr = (UINT32)snddata32 );
			break;
		}

		SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_START = SPI_DMA_ACC_OP_EN );

		// Check Access-Done
		chkflag = SPI_IRQ_STATUS_ALLMARK ;
#ifdef	SUPPORT_SPI_POLLMODE
		do{
			UINT32 irqstatus, subirq;

			spi_isr_convert(spi, &irqstatus, &subirq);

			SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
				= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );

			maskevt = (UNSIGNED)(subirq & chkflag);

		}while( (maskevt) == 0 );
#else	//SUPPORT_SPI_POLLMODE
		maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_SPI_POLLMODE

		if( spi_2pass_on == 0 && spi->concat == FALSE ){
			SPI_REG_ACCESS( spi->reg_spi->SPI_IO_CS_TYPE = SPI_SPI_CSELPIN_MANUAL(0,0,0) );
#ifdef TEST_SPI_REQ_CLR_OP
			// FIXME: CS PIN manual ���� �� �̽� �߻��Ͽ� �߰���
			SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN | SPI_XIP_ACC_OP_EN );
#endif
		}

		// Check Time-Out

		if( ((spi->busctrl[0] & SPI_SPI_TIMEOUT_EN) != 0)
		    && ((maskevt & SPI_IRQ_STATUS_TO_FORCE) == 0) ){
			chkflag = SPI_IRQ_STATUS_TO_FORCE ;
#ifdef	SUPPORT_SPI_POLLMODE
			do{
				UINT32 irqstatus, subirq;

				spi_isr_convert(spi, &irqstatus, &subirq);

				SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
					= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );

				maskevt = (UNSIGNED)(subirq & chkflag);

			}while( (maskevt) == 0 );
#else	//SUPPORT_SPI_POLLMODE
		maskevt = xEventGroupWaitBits(spi->event, chkflag, pdTRUE, pdFALSE, portMAX_DELAY);
#endif	//SUPPORT_SPI_POLLMODE
		}

#ifdef	SUPPORT_SPI_POLLMODE
#else	//SUPPORT_SPI_POLLMODE
//		xEventGroupSetBits(spi->event, 0);
//		xEventGroupClearBits(spi->event, SPI_IRQ_STATUS_TO_FORCE);
#endif	//SUPPORT_SPI_POLLMODE

		switch(spimode){
		case 1: // Read-Data
			// restore dma-config
			if( rcvdata32 == NULL ){
				SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_CFG = bkupcfg );
			}
			break;
		default:
			if( spi_2pass_on == 1 ){
				spimode = 1; // pass-2, read
			}
			break;
		}

		SPI_REG_ACCESS( spi->reg_spi->SPI_CS_SEL = spi->cselbkup ); // MPW issue
		SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->cselbkup ); // MPW issue

	}
	while( spi_2pass_on == 1 );

	return spiilen;
}

/******************************************************************************
 *  SPI_CLOSE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_CLOSE(HANDLE handler)
{
	SPI_HANDLER_TYPE *spi;

	if(handler == NULL){
		return FALSE;
	}
	spi = (SPI_HANDLER_TYPE *)handler ;

	if( _spi_instance[spi->dev_unit] > 0){
		_spi_instance[spi->dev_unit] --;
		spi->instance -- ;

		if(_spi_instance[spi->dev_unit] == 0){

			spi_isr_close(spi);

			_sys_clock_ioctl( SYSCLK_RESET_CALLACK, spi->pllinfo);

			SPI_REG_ACCESS(
			    spi->reg_cmn->SPI_OP_EN
				&= ~( SPI_REQ_SET_IRQMASK(SPI_TIMEOUT_OP_EN|SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN|SPI_XIP_ACC_OP_EN)
				    | (SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN) )
			);

			// Event
#ifdef	SUPPORT_SPI_POLLMODE
#else	//SUPPORT_SPI_POLLMODE
			if( spi->event != NULL ){
				vEventGroupDelete(spi->event);
			}
#endif	//SUPPORT_SPI_POLLMODE

			// Semaphore
			if( spi->buslock != NULL ){
		        vSemaphoreDelete((spi->buslock));

				_spi_buslock[spi->dev_unit] = NULL;
			}

			_spi_handler[spi->dev_unit] = NULL;
			SPI_MEM_FREE(handler);
		}
	}

	return TRUE;
}


/******************************************************************************
 *  SPI_WRITE ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_WRITE(HANDLE handler, void *pdata, UINT32 dlen)
{


#if 0
	UINT32	ioctldata[1];
	switch(addr){
		case	2:
			ioctldata[0] = SPI_BUS_NONAUTO
				| SPI_ADDR_BYTE(0)
				| SPI_DMMY_CYCLE(0)
				| SPI_ACC_MODE(SPI_ACC_WRITE)
				| SPI_BUS_MODE(SPI_BUS_222);
			break;
		case	4:
			ioctldata[0] = SPI_BUS_NONAUTO
				| SPI_ADDR_BYTE(0)
				| SPI_DMMY_CYCLE(0)
				| SPI_ACC_MODE(SPI_ACC_WRITE)
				| SPI_BUS_MODE(SPI_BUS_444);
			break;
		default:
			ioctldata[0] = SPI_BUS_NONAUTO
				| SPI_ADDR_BYTE(0)
				| SPI_DMMY_CYCLE(0)
				| SPI_ACC_MODE(SPI_ACC_WRITE)
				| SPI_BUS_MODE(SPI_BUS_111);
			break;
	}
#else
	UINT32 busctrl[2];
	busctrl[0] = SPI_SPI_TIMEOUT_EN
		| SPI_SPI_PHASE_CMD_1BYTE
		| SPI_SPI_PHASE_ADDR_3BYTE
		| SPI_SET_SPI_DUMMY_CYCLE(0);



	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);

#endif
	SPI_IOCTL(handler, SPI_SET_BUSCONTROL, busctrl );

	return SPI_TRANSMIT( handler, pdata, dlen, NULL, 0);
}

int	SPI_READ(HANDLE handler, void *pdata, UINT32 dlen)
{
	UINT32 busctrl[2];
	busctrl[0] = SPI_SPI_TIMEOUT_EN
		| SPI_SPI_PHASE_CMD_1BYTE
		| SPI_SPI_PHASE_ADDR_3BYTE
		| SPI_SET_SPI_DUMMY_CYCLE(0);

	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
	SPI_IOCTL(handler, SPI_SET_BUSCONTROL, busctrl );

	return SPI_TRANSMIT( handler, NULL, 0, pdata, dlen);
}


/******************************************************************************
 *  SPI_WRITE_READ ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SPI_WRITE_READ(HANDLE handler
					, void *snddata, UINT32 sndlen
					, void *rcvdata, UINT32 rcvlen )
{
#if 0
	UINT32	ioctldata[1];
	if( sndlen > 8 ){
		switch(addr){
			case	2:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_222);
				break;
			case	4:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_444);
				break;
			default:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_111);
				break;
		}
		SPI_IOCTL(handler, SPI_SET_BUSCONTROL, ioctldata );

		SPI_IOCTL(handler, SPI_SET_CONCAT, NULL);

		SPI_TRANSMIT( handler, snddata, sndlen, NULL, 0);

		switch(addr){
			case	2:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_222);
				break;
			case	4:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_444);
				break;
			default:
				ioctldata[0] = SPI_BUS_NONAUTO
					| SPI_ADDR_BYTE(0)
					| SPI_DMMY_CYCLE(0)
					| SPI_ACC_MODE(SPI_ACC_WRITE)
					| SPI_BUS_MODE(SPI_BUS_111);
				break;
		}
		SPI_IOCTL(handler, SPI_SET_BUSCONTROL, ioctldata );

		return SPI_TRANSMIT( handler, rcvdata, rcvlen, NULL, 0);
	}

	switch(addr){
		case	2:
			ioctldata[0] = SPI_BUS_MODE(SPI_BUS_222);
			break;
		case	4:
			ioctldata[0] = SPI_BUS_MODE(SPI_BUS_444);
			break;
		default:
			ioctldata[0] = SPI_BUS_MODE(SPI_BUS_111);
			break;
	}

	ioctldata[0] = ioctldata[0]
			| SPI_BUS_NONAUTO | SPI_ACC_MODE(SPI_ACC_READ)
			| SPI_ADDR_BYTE((sndlen-1))
			| SPI_DMMY_CYCLE((sndlen-1)) ;
	SPI_IOCTL(handler, SPI_SET_BUSCONTROL, ioctldata );

	return SPI_TRANSMIT( handler, snddata, sndlen, rcvdata, rcvlen);
#else
	UINT32 busctrl[2];
	busctrl[0] = SPI_SPI_TIMEOUT_EN
		| SPI_SPI_PHASE_CMD_1BYTE
		| SPI_SPI_PHASE_ADDR_3BYTE
		| SPI_SET_SPI_DUMMY_CYCLE(0);

	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(0,0,SPI_BUS_SPI)
					, SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
	SPI_IOCTL(handler, SPI_SET_BUSCONTROL, busctrl );

	return SPI_TRANSMIT( handler, snddata, sndlen, rcvdata, rcvlen);
#endif
}



/******************************************************************************
 *  SPI Control
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static int  spi_set_format(SPI_HANDLER_TYPE *spi, UINT32 format)
{
	switch(format){
		case	SPI_TYPE_MOTOROLA_O0H0:
			SPI_REG_ACCESS( spi->reg_spi->SPI_INT_MODE = SPI_SPI_CPOLCPHASE_MODE0 );
			SPI_REG_ACCESS( spi->reg_xip->XIP_INT_MODE = SPI_SPI_CPOLCPHASE_MODE0 );
			break;
		case	SPI_TYPE_MOTOROLA_O1H1:

			SPI_REG_ACCESS( spi->reg_spi->SPI_INT_MODE = SPI_SPI_CPOLCPHASE_MODE3 );
			SPI_REG_ACCESS( spi->reg_xip->XIP_INT_MODE = SPI_SPI_CPOLCPHASE_MODE3 );

			break;

		case	SPI_TYPE_MOTOROLA_O0H1:
		case	SPI_TYPE_MOTOROLA_O1H0:
		default:
			return FALSE;
	}

	return TRUE;
}

static int  spi_set_coreclock(SPI_HANDLER_TYPE *spi, UINT32 speed)
{
	if( speed == 0 ){
		return FALSE;
	}

	spi->coreclock = speed ;

	return TRUE;
}

static int  spi_set_speed(SPI_HANDLER_TYPE *spi, UINT32 speed)
{
	UINT32 margintick;

	if( speed == 0 ){
		return FALSE;
	}

	/*
	 * FIXME: safe check �� ���� 1:1 ���� ������ �ȵ�. �����ϵ��� ������
	 */
#if 0 // org
	UINT32 coreclock;
	SPI_REG_ACCESS( coreclock = SPI_SPI_GET_CORE_FREQ_MHz(spi->reg_cmn->SPI_CORE_FREQ) );
	if( SPI_SPI_CHK_SAFE_Hz( speed, (coreclock*MHz) ) ){
		SPI_REG_ACCESS( spi->reg_spi->SPI_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(speed/MHz) );
		SPI_REG_ACCESS( spi->reg_dma->DMA_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(speed/MHz) );
		spi->spiclock =speed ;
	}else{
		coreclock = coreclock >> 1;
		SPI_REG_ACCESS( spi->reg_spi->SPI_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(coreclock) );
		SPI_REG_ACCESS( spi->reg_dma->DMA_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(coreclock) );
		spi->spiclock = coreclock * MHz ;
	}
#else
	SPI_REG_ACCESS( spi->reg_spi->SPI_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(speed/MHz) );
	SPI_REG_ACCESS( spi->reg_dma->DMA_CLK_FREQ_S = SPI_SPI_SET_FREQ_MHz(speed/MHz) );
	spi->spiclock =speed ;
#endif
	margintick = ((speed / (10*MHz)) >> 2);
	SPI_REG_ACCESS( spi->reg_cmn->CLK_CE_SEL_CYC = margintick + 1 );
	SPI_REG_ACCESS( spi->reg_cmn->CLK_CE_DES_CYC = margintick + 3 );

	return TRUE;
}

static int  spi_get_speed(SPI_HANDLER_TYPE *spi, UINT32 *speed)
{
	UINT32 coreclock;
	UINT32 divider;

	if( speed == NULL ){
		return FALSE;
	}

	SPI_REG_ACCESS( coreclock = SPI_SPI_GET_CORE_FREQ_MHz(spi->reg_cmn->SPI_CORE_FREQ) );
	SPI_REG_ACCESS( divider = spi->reg_spi->SPI_CLK_FREQ_R );

	*speed = (coreclock*MHz) / (divider+1) ;

	return TRUE;
}

static int  spi_set_timeout(SPI_HANDLER_TYPE *spi, UINT32 unit, UINT32 count)
{
	UINT32 coreclock;

	SPI_REG_ACCESS( coreclock = SPI_SPI_GET_CORE_FREQ_MHz(spi->reg_cmn->SPI_CORE_FREQ) );

	if( coreclock == 0 ){
		return FALSE;
	}

	SPI_REG_ACCESS( spi->reg_cmn->CLK_TO_UNIT = unit );
	SPI_REG_ACCESS( spi->reg_cmn->CLK_TO_COUNT = count );

	SPI_DBG_NONE("SPI:TO - U:%d/C:%d", unit, count);

	return TRUE;
}


static int  spi_set_wiremodel(SPI_HANDLER_TYPE *spi, UINT32 unit, UINT32 model)
{
	if( unit >= SPI_CSEL_MAX ){
		return FALSE;
	}

	if( model == 3 ){
		SPI_REG_ACCESS( spi->reg_spi->SPI_IO_1_TYPE &= ~(XPIN_3WIRE(unit)) );
	}else{
		SPI_REG_ACCESS( spi->reg_spi->SPI_IO_1_TYPE |= (XPIN_4WIRE(unit)) );
	}

	return TRUE;
}

static int  spi_set_dma_config(SPI_HANDLER_TYPE *spi, UINT32 config)
{
	SPI_REG_ACCESS( spi->reg_dma->DMA_MP0_CFG = config );

	return TRUE;
}

static int  spi_set_pincontrol(SPI_HANDLER_TYPE *spi, UINT32 config)
{
	if( config == TRUE ){ // Quad
		SPI_REG_ACCESS( spi->reg_spi->SPI_IO_DX_TYPE = SPI_PIN_WPHOLD_DISABLE );
	}else{
		SPI_REG_ACCESS( spi->reg_spi->SPI_IO_DX_TYPE = SPI_PIN_WPHOLD_ENABLE  );
	}
	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_XIP_ACC_OP_EN|SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );
	return TRUE;
}

static int  spi_set_cloning(SPI_HANDLER_TYPE *spi, UINT32 *config)
{
	if( config[1] == 0x00){
		// Skip
		return TRUE;
	}

	SPI_REG_ACCESS( spi->reg_xip->XIP_CLK_FREQ_S = spi->reg_spi->SPI_CLK_FREQ_S  );
	//SPI_REG_ACCESS( spi->reg_xip->XIP_CLK_FREQ_R = spi->reg_spi->SPI_CLK_FREQ_R  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_DMY_CYC = SPI_GET_SPI_DUMMY_CYCLE(spi->busctrl[0]) );
	SPI_REG_ACCESS( spi->reg_xip->XIP_WAIT_CYC = spi->reg_spi->SPI_WAIT_CYC  );

	SPI_REG_ACCESS( spi->reg_xip->XIP_INT_MODE = spi->reg_spi->SPI_INT_MODE  );

	SPI_REG_ACCESS( spi->reg_xip->XIP_PHASE_MODE = (spi->busctrl[0]) & (~SPI_SPI_PHASE_DUMMY_MASK) );
	SPI_REG_ACCESS( spi->reg_xip->XIP_PHASE_MODE &= (~SPI_SPI_TIMEOUT_EN)  );
	if( config[0] != 0x00 ){
		SPI_REG_ACCESS( spi->reg_xip->XIP_PHASE_MODE &= (~SPI_SPI_PHASE_MODE_1BIT)  );
		switch(config[0]){
		case	0xA5:
			SPI_REG_ACCESS( spi->reg_xip->XIP_PHASE_MODE |= (SPI_SPI_PHASE_MODE_8BIT)  );
			break;
		case	0xA0:
			SPI_REG_ACCESS( spi->reg_xip->XIP_PHASE_MODE |= (SPI_SPI_PHASE_MODE_4BIT)  );
			break;
		default:
			break;
		}
	}

	SPI_REG_ACCESS( spi->reg_xip->XIP_BYTE_SWAP = spi->reg_spi->SPI_BYTE_SWAP  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_WRAP_EN = spi->reg_spi->SPI_WRAP_EN  );

	SPI_REG_ACCESS( spi->reg_xip->XIP_BUS_TYPE = spi->busctrl[1] );
	if( config[0] != 0x00 ){
		UINT32 busmask;
		busmask = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(0,1,SPI_BUS_OPI)
					, SPI_BUS_TYPE(1,1,SPI_BUS_OPI) // ADDR
					, SPI_BUS_TYPE(1,1,SPI_BUS_OPI) // MODE
					, SPI_BUS_TYPE(1,1,SPI_BUS_OPI) // DATA
				);
		SPI_REG_ACCESS( spi->reg_xip->XIP_BUS_TYPE &= busmask  );
	}

	SPI_REG_ACCESS( spi->reg_xip->XIP_IO_DX_TYPE = spi->reg_spi->SPI_IO_DX_TYPE  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_IO_CK_TYPE = spi->reg_spi->SPI_IO_CK_TYPE  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_IO_CS_TYPE = spi->reg_spi->SPI_IO_CS_TYPE  );

	SPI_REG_ACCESS( spi->reg_xip->XIP_CS_SEL = spi->reg_spi->SPI_CS_SEL  );
	SPI_REG_ACCESS( spi->cselbkup = spi->cselinfo ); // MPW issue

	SPI_REG_ACCESS( spi->reg_xip->XIP_IO_1_TYPE = spi->reg_spi->SPI_IO_1_TYPE  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_TWIN_QSPI_EN = spi->reg_spi->SPI_TWIN_QSPI_EN  );

	SPI_REG_ACCESS( spi->reg_xip->XIP_REG_RW = 0 );
	SPI_REG_ACCESS( spi->reg_xip->XIP_REG_CMD = config[1]  );
	SPI_REG_ACCESS( spi->reg_xip->XIP_REG_MODE = spi->reg_spi->SPI_REG_MODE );

	SPI_REG_ACCESS( spi->reg_xip->XIP_DLY_INDEX = spi->reg_spi->SPI_DLY_INDEX );

	SPI_REG_ACCESS( spi->reg_xip->XIP_SPI_BST = sizeof(UINT32) );

	SPI_REG_ACCESS( spi->reg_cmn->SPI_REQ_CLR_OP = SPI_XIP_ACC_OP_EN|SPI_NON_DMA_OP_EN|SPI_DMA_ACC_OP_EN );

	return TRUE;
}

static void spi_isr_convert(SPI_HANDLER_TYPE *spi, UINT32 *pirqstatus, UINT32 *psubirq)
{
	UINT32 			irqstatus, subirq;

	SPI_REG_ACCESS_ISR( irqstatus = spi->reg_cmn->SPI_IRQ_STATUS );
	irqstatus = irqstatus & (~SPI_XIP_ACC_OP_EN);
	SPI_REG_ACCESS_DUMP( irqstatus );

	subirq = 0;
	if( (irqstatus & SPI_TIMEOUT_OP_EN) != 0 ){
		subirq |= SPI_IRQ_STATUS_TO_FORCE;
	}

	if( (irqstatus & SPI_NON_DMA_OP_EN) != 0 ){
		SPI_REG_ACCESS_ISR( subirq |= spi->reg_cmn->IRQ_STATUS_SPI );

		if( (subirq & SPI_IRQ_STATUS_TXFIFO_UNDR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_TXFIFO_OVER) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_RXFIFO_UNDR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_RXFIFO_OVER) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_AHB_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_ACC_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_SET_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_ACC_DONE) != 0 ){
		}
	}

	if( (irqstatus & SPI_DMA_ACC_OP_EN) != 0 ){
		SPI_REG_ACCESS_ISR( subirq |= spi->reg_cmn->IRQ_STATUS_DMA );

		if( (subirq & SPI_IRQ_STATUS_TXFIFO_UNDR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_TXFIFO_OVER) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_RXFIFO_UNDR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_RXFIFO_OVER) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_AHB_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_ACC_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_SET_ERR) != 0 ){
		}
		if( (subirq & SPI_IRQ_STATUS_ACC_DONE) != 0 ){
		}
	}

	SPI_REG_ACCESS_DUMP( subirq );

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

static int  spi_isr_create(HANDLE handler)
{
	if(handler == NULL){
		return FALSE;
	}

	// Disable All Interrupts

	return TRUE;

}

static int  spi_isr_init(HANDLE handler)
{
	SPI_HANDLER_TYPE	*spi;

	if(handler == NULL){
		return FALSE;
	}

	spi = (SPI_HANDLER_TYPE *) handler ;

	// Registry LISR
	switch(spi->dev_unit){
	case	SPI_UNIT_0:
		_sys_nvic_write(SPI_IRQn, (void *)_tx_specific_spi0_interrupt, 0x7);
		break;
	}

	// Disalbe All Interrupts

	return TRUE;
}


static int  spi_isr_close(HANDLE handler)
{
	SPI_HANDLER_TYPE	*spi;

	if(handler == NULL){
		return FALSE;
	}

	spi = (SPI_HANDLER_TYPE *) handler ;

	// Disable All Interrupts

	// UN-Registry LISR
	switch(spi->dev_unit){
		case	SPI_UNIT_0:
			_sys_nvic_write(SPI_IRQn, NULL, 0x7);
//			_sys_nvic_write(XCACHE_IRQn, NULL, 0x7);
			break;
	}

	return TRUE;

}

/******************************************************************************
 *  ISR
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void _tx_specific_spi0_interrupt(void)
{
	INTR_CNTXT_CALL_PARA(spi_isr_core, SPI_UNIT_0);
}

void spi_isr_core(UINT32 idx)
{
	SPI_HANDLER_TYPE	*spi;
	UINT32 			irqstatus, subirq;
	BaseType_t xHigherPriorityTaskWoken, xResult;

	xHigherPriorityTaskWoken = pdFALSE;
	xResult = pdFAIL;
	spi = _spi_handler[idx];

	spi_isr_convert(spi, &irqstatus, &subirq);

#ifdef	SUPPORT_SPI_POLLMODE
#else	//SUPPORT_SPI_POLLMODE
	if( subirq != 0 ){
		xResult = xEventGroupSetBitsFromISR(spi->event, subirq, &xHigherPriorityTaskWoken);
	    if( xResult != pdFAIL )
	     {
	         /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
	         switch should be requested.  The macro used is port specific and will
	         be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
	         the documentation page for the port being used. */
	         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	     }
	}
#endif	//SUPPORT_SPI_POLLMODE

	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_REQ_CLR_IRQ
				= SPI_REQ_SET_IRQMASK(irqstatus)|irqstatus );
}


/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void spi_set_delay_parameter(SPI_HANDLER_TYPE *spi, UINT8 *config)
{
	if( (config == NULL) || (config[0] == 0) ){
		spi->dlytab = dlytab;
	}else{
		spi->dlytab = config;
	}

	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_0_RXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 4], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_1_RXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 5], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_2_RXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 6], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_3_RXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 7], 0));

	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_0_TXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 8], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_1_TXC = SPI_SPI_DELAY_TXC(spi->dlytab[ 9], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_2_TXC = SPI_SPI_DELAY_TXC(spi->dlytab[10], 0));
	SPI_REG_ACCESS_ISR( spi->reg_cmn->SPI_SPI_DLY_3_TXC = SPI_SPI_DELAY_TXC(spi->dlytab[11], 0));

	SPI_TRACE_PRINT("SPI_SPI_DLY_0_RXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_0_RXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_1_RXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_1_RXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_2_RXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_2_RXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_3_RXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_3_RXC);

	SPI_TRACE_PRINT("SPI_SPI_DLY_0_TXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_0_TXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_1_TXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_1_TXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_2_TXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_2_TXC);
	SPI_TRACE_PRINT("SPI_SPI_DLY_3_TXC: 0x%08x\n", spi->reg_cmn->SPI_SPI_DLY_3_TXC);
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void spi_set_delay_index(SPI_HANDLER_TYPE *spi)
{
	SPI_TRACE_PRINT("SPI_CORE_FREQ: %d\n", spi->reg_cmn->SPI_CORE_FREQ);
	SPI_TRACE_PRINT("DMA_CLK_FREQ_R: %d\n", spi->reg_dma->DMA_CLK_FREQ_R);
	SPI_TRACE_PRINT("SPI_DLY_INDEX: %d\n", spi->dlyidx);

	SPI_REG_ACCESS( spi->reg_spi->SPI_DLY_INDEX = spi->dlyidx );
	SPI_REG_ACCESS( spi->reg_dma->DMA_DLY_INDEX = spi->dlyidx );
	SPI_REG_ACCESS( spi->reg_xip->XIP_DLY_INDEX = spi->dlyidx );
}

/******************************************************************************
 *  ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void spi_set_pll_div(SPI_HANDLER_TYPE *spi)
{
	UINT32 spiclock;
	UINT32 pllhz, xtalhz, busclock;

	// Set Clock Freq.
	da16x_get_default_xtal( &xtalhz, &busclock );
	xtalhz = xtalhz / KHz ;
	pllhz  = da16x_pll_getspeed(xtalhz);
	spiclock  = pllhz / (DA16X_CLK_DIV(DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU) + 1);

	SPI_TRACE_PRINT("spiclock: %dHz\n", spiclock);

	// debug disabled
	SPI_REG_ACCESS( spi->reg_cmn->XFC_DBG_RX_SHIFT = 0x0 );

//	SPI_REG_ACCESS( spi->reg_cmn->XFC_SPI_RX_SHIFT_EN = 0x1 );
//	SPI_REG_ACCESS( spi->reg_cmn->XFC_SPI_CLK_1x1_EN = 0x3 );

	SPI_REG_ACCESS( spi->reg_cmn->SPI_CORE_FREQ = SPI_SPI_SET_CORE_FREQ_MHz(spiclock/KHz) );
	SPI_TRACE_PRINT("SPI_CORE_FREQ: 0x%08x\n", spi->reg_cmn->SPI_CORE_FREQ);

	spi_set_delay_index( spi );
}

static void spi_pll_callback(UINT32 clock, void *param)
{
	SPI_HANDLER_TYPE *spi;

	spi = (SPI_HANDLER_TYPE*) param;

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		return;
	}

	spi_set_pll_div(spi);

	if( spi_set_coreclock( spi, clock ) == TRUE ){
		spi_set_speed(spi, spi->spiclock);
	}
}
