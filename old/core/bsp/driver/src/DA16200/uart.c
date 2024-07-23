/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief CMSDK PL011 UART
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

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#if (dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define traceISR_ENTER()
#  define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

#ifdef	SUPPORT_PL011UART

#define UART_TX_ROOM_VALID	0x01
#define UART_TX_ROOM_FLUSH	0x02

#define UART_FIFO_DEPTH			32
#define UART_CHK_OVERRUN		31
#define UART_QUEUE_NAME_SIZE	8
#define GET_FIFO_LENGTH(x)	((x == UART_ONEEIGHTH)? 4:		\
				(x == UART_ONEQUARTER)? 8:		\
				(x == UART_HALF)? 16:  			\
				(x == UART_THREEQUARTERS)? 24: 	\
				(x == UART_SEVENEIGHTHS)? 28:1)

#define HW_REG_WRITE8(x, v) 		( *((volatile UINT8   *) &(x)) = (v) )
#define	HW_REG_READ8(x)				( *((volatile UINT8   *) &(x)) )
typedef union {
	UINT32 i;
	UINT8 c[4];
} UART_DATA;

static UINT32  uart_baudrate_calc(UINT32 uartclk, UINT32 BaudRate, UINT32 *FractionalBaud, UINT32 *IntegralVal);
static int	uart_check_rxempty(UART_HANDLER_TYPE *uart);

static void _uart_isr0(void);
static void _uart_isr1(void);
static void _uart_isr2(void);
//---------------------------------------------------------
//	DRIVER :: internal variables
//---------------------------------------------------------
static int						_uart_instance[UART_UNIT_MAX];
static UART_HANDLER_TYPE		*_uart_handler[UART_UNIT_MAX];
static uart_dma_primitive_type * uart_dma_primitives;

UART_HANDLER_TYPE uart_handler[UART_UNIT_MAX];
unsigned char uart_rx_buffer[UART_RX_BUFSIZ*sizeof(UINT32)];

UINT	rxQueueFlag = 0;
UINT	txQueueFlag = 0;
UINT	dmaQueueFlag = 0;
UINT	uart_rx_intr_cnt = 0;
UINT	uart_tx_intr_cnt = 0;
UINT	uart_timeout_intr_cnt = 0;
UINT	uart_timeout_rx_intr_cnt = 0;

UINT8	rxQueueGlobal = 0;

//---------------------------------------------------------
//	DRIVER :: private functions
//---------------------------------------------------------

//
//	DMA callback
//
static void uart_dma_callback(void *param)
{
	UART_HANDLER_TYPE	*uart;
	unsigned int data = 0;
	BaseType_t	xHigherPriorityTaskWoken = pdFALSE;

	if( param == NULL){
		return ;
	}
	uart = (UART_HANDLER_TYPE*) param;
	HW_REG_WRITE32(uart->regmap->DmaCon, (UINT32) (0x00));
	xQueueSendFromISR(uart->dma_queue, (VOID *)&data, &xHigherPriorityTaskWoken);
	dmaQueueFlag = 1;
}

static void sys_dma_copy(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len)
{
	if (uart_dma_primitives == NULL)
		return;
	if (uart_dma_primitives->sys_dma_copy == NULL)
		return;

	uart_dma_primitives->sys_dma_copy(hdl, dst, src, len);
}

static void sys_dma_release_channel(HANDLE hdl) {
	if (uart_dma_primitives == NULL)
			return;
	if (uart_dma_primitives->sys_dma_release_channel == NULL)
		return;

	uart_dma_primitives->sys_dma_release_channel(hdl);
}

static void sys_dma_registry_callback(HANDLE hdl, USR_CALLBACK term,  USR_CALLBACK empty, USR_CALLBACK error  , HANDLE dev_handle)
{
	if (uart_dma_primitives == NULL)
		return;
	if (uart_dma_primitives->sys_dma_registry_callback == NULL)
		return;

	uart_dma_primitives->sys_dma_registry_callback(hdl, term, empty, error, dev_handle);
}

static void _uart_dma_init(HANDLE handler)
{
	UART_HANDLER_TYPE	*uart;
	uart = (UART_HANDLER_TYPE *)handler;

	sys_dma_registry_callback( uart->dma_channel_rx, uart_dma_callback,  NULL, NULL, (HANDLE) uart);
	sys_dma_registry_callback( uart->dma_channel_tx, uart_dma_callback,  NULL, NULL, (HANDLE) uart);
}

static BaseType_t xQueueIsQueueEmpty(const QueueHandle_t xQueue)
{
	BaseType_t result;

	taskENTER_CRITICAL();
	{
		result = xQueueIsQueueEmptyFromISR(xQueue);
	}
    taskEXIT_CRITICAL();
    return result;

}

//
// Create Handle
//
HANDLE	UART_CREATE( UART_UNIT_IDX dev_idx )
{
	UART_HANDLER_TYPE	*uart;

	if (dev_idx >= UART_UNIT_MAX)
		return NULL;

	// prevent multi alloccation
	if (_uart_instance[dev_idx] > 0)
		return NULL;

	uart = &uart_handler[dev_idx];

	// Clear
	memset(uart, 0, sizeof(UART_HANDLER_TYPE));
	_uart_handler[dev_idx] = uart;

	DA16200_SYSCLOCK->PLL_CLK_EN_2_UART = 1;
	DA16200_SYSCLOCK->CLK_EN_UART = 1;

	// Address Mapping
	switch(dev_idx){
		case	UART_UNIT_0:
				/* clock gating on*/
				DA16X_CLKGATE_ON(clkoff_uart[0]);
				uart->dev_unit	 = dev_idx ;
				uart->dev_addr	 = (UINT32)SYS_UART_BASE_0 ;
				uart->instance	 = (UINT32)(_uart_instance[dev_idx]) ;
				_uart_instance[dev_idx] ++;
				break;
		case	UART_UNIT_1:
			/* clock gating on*/
				DA16X_CLKGATE_ON(clkoff_uart[1]);
				uart->dev_unit	 = dev_idx ;
				uart->dev_addr	 = (UINT32)SYS_UART_BASE_1 ;
				uart->instance	 = (UINT32)(_uart_instance[dev_idx]) ;
				_uart_instance[dev_idx] ++;
				break;
		case	UART_UNIT_2:
			/* clock gating on*/
				DA16X_CLKGATE_ON(clkoff_uart[2]);
				uart->dev_unit	 = dev_idx ;
				uart->dev_addr	 = (UINT32)SYS_UART_BASE_2 ;
				uart->instance	 = (UINT32)(_uart_instance[dev_idx]) ;
				_uart_instance[dev_idx] ++;
				break;
		default:
				DRV_DBG_ERROR("illegal unit number");
				return NULL;
	}

	DRV_DBG_NONE("(%p) UART create, base %p", uart, (UINT32 *)uart->dev_addr );

	// Binding into Physical Addr
	uart->regmap		= (UART_REG_MAP *)(uart->dev_addr);
	uart->rx_suspend	= portMAX_DELAY ;
	uart->que_size      = UART_RX_BUFSIZ ;
	uart->rw_word 		= 0; // byte access

	return (HANDLE) uart;
}

int UART_DMA_INIT(HANDLE handler, HANDLE dma_channel_tx, HANDLE dma_channel_rx, const uart_dma_primitive_type * primitive)
{
	UART_HANDLER_TYPE	*uart;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler;
	uart->dma_channel_tx = dma_channel_tx;
	uart->dma_channel_rx = dma_channel_rx;

	if (uart_dma_primitives == NULL)
		uart_dma_primitives = (uart_dma_primitive_type *)primitive;

	return TRUE;
}

//
//	UART Initialization
//

int 	UART_INIT (HANDLE handler)
{
	UART_HANDLER_TYPE	*uart;
	UINT32	FractionalBaud, IntegralBaud;
	volatile UINT32 Trash, trashcnt;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler;
	FractionalBaud = 0;
	IntegralBaud   = 0;

	// Init Condition Check
	if( uart->instance == 0 ){
		/*
		 * Disable UART Interrupt
		 */
		HW_REG_AND_WRITE32( uart->regmap->Control, (UART_DISABLE));

	    // Clock Enable
	    switch(uart->dev_unit){
			case	UART_UNIT_0:
					// register interrupt vector
					_sys_nvic_write(UART_0_IRQn, (void *)_uart_isr0, 0x7);
					break;
			case	UART_UNIT_1:
			  		// register interrupt vector
					_sys_nvic_write(UART_1_IRQn, (void *)_uart_isr1, 0x7);
					break;
			case	UART_UNIT_2:
			  		// register interrupt vector
					_sys_nvic_write(UART_2_IRQn, (void *)_uart_isr2, 0x7);
					break;
	    }

		if ( uart_baudrate_calc(uart->uart_clk, uart->baudrate, &FractionalBaud, &IntegralBaud) == FALSE){
			DRV_DBG_ERROR("(%p) Unformal baudrate, base %p - %d bps", uart, (UINT32 *)uart->dev_addr , uart->baudrate);
			return FALSE;
		}

		HW_REG_WRITE32( uart->regmap->IntBaudDivisor, (IntegralBaud & UART_BAUD_INTEGER_MASK) );
		HW_REG_WRITE32( uart->regmap->FractBaudDivisor, (FractionalBaud & UART_BAUD_FRACTION_MASK) );

		HW_REG_WRITE32( uart->regmap->LineCon_H, uart->linectrl );
		HW_REG_WRITE32( uart->regmap->Control, uart->control );

		HW_REG_WRITE32( uart->regmap->IntClear, UART_INTBIT_ALL ); // clearing all the interrupts
		HW_REG_OR_WRITE32( uart->regmap->Control, UART_ENABLE );

		HW_REG_WRITE32( uart->regmap->FifoIntLevel, uart->fifo_level ) ;

		for(trashcnt =0; trashcnt < 8; trashcnt++ ){
			if ( (HW_REG_READ32(uart->regmap->Flag) & UART_RX_FIFO_EMPTY_MASK ) == 0x00 ) {
				Trash = HW_REG_READ32( uart->regmap->DataRead );
			} else {
				break;
			}
		}

		if (uart->use_word_access) {
			HW_REG_WRITE32(uart->regmap->WordAccess, 0x1);
		} else {
			HW_REG_WRITE32(uart->regmap->WordAccess, 0x0);
		}

		if (uart->use_sw_flow_control) {
			HW_REG_WRITE32(uart->regmap->SWFlowControl, 0x1);
		} else {
			HW_REG_WRITE32(uart->regmap->SWFlowControl, 0x0);
		}

		if (uart->use_rs485) {
			DA16X_SLAVECTRL->RS485_Mode_Sel = 1;
		} else {
			DA16X_SLAVECTRL->RS485_Mode_Sel = 0;
		}

		if (uart->use_rs485) {
			HW_REG_WRITE32(uart->regmap->RS485En, 0x1);
		} else {
			HW_REG_WRITE32(uart->regmap->RS485En, 0x0);
		}

		/*
		 * initialize UART DMA
		 */
		if (uart->use_dma)
			_uart_dma_init(handler);

		uart->rx_quebuf = (unsigned int *) &uart_rx_buffer[0];

		DRV_SPRINTF((char *)uart->rx_que_name, "uart%drx", (int)(uart->dev_unit));
		DRV_SPRINTF((char *)uart->tx_que_name, "uart%dtx", (int)(uart->dev_unit));
		DRV_SPRINTF((char *)uart->dma_que_name, "uart%ddm", (int)(uart->dev_unit));

		uart->rx_queue = xQueueCreate(uart->que_size, sizeof(UINT));
		if (uart->rx_queue == NULL) {
			DRV_DBG_ERROR(" >> UART rx Queue create fail \r\n");
		}

		uart->tx_queue = xQueueCreate(10, 4);
		if (uart->tx_queue == NULL) {
			DRV_DBG_ERROR(" >> UART tx Queue create fail \r\n");
		}

		uart->dma_queue = xQueueCreate(10, 4);
		if (uart->dma_queue == NULL) {
			DRV_DBG_ERROR(" >> UART dma Queue create fail \r\n");
		}

		/*
		 * Enable UART Interrupt
		 */
		HW_REG_WRITE32( uart->regmap->IntMask, uart->intctrl );
		memset((void*)uart->ch, 0x00, 32);
	}

	return TRUE;
}

int 	UART_CHANGE_BAUERATE (HANDLE handler, UINT32 baudrate)
{
	UART_HANDLER_TYPE	*uart;
	UINT32	FractionalBaud, IntegralBaud;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	uart = (UART_HANDLER_TYPE *)handler;

	if (baudrate)
		uart->baudrate = baudrate;
	FractionalBaud = 0;
	IntegralBaud   = 0;

	HW_REG_AND_WRITE32( uart->regmap->Control, ( UART_DISABLE ) );
	if ( uart_baudrate_calc(uart->uart_clk, uart->baudrate, &FractionalBaud, &IntegralBaud) == FALSE){
		DRV_DBG_ERROR("(%p) Unformal baudrate, base %p - %d bps", uart, (UINT32 *)uart->dev_addr , uart->baudrate);
	}
	DRV_DBG_NONE("%d baudrate", uart->baudrate);

	HW_REG_WRITE32( uart->regmap->IntBaudDivisor, (IntegralBaud & UART_BAUD_INTEGER_MASK) );
	HW_REG_WRITE32( uart->regmap->FractBaudDivisor, (FractionalBaud & UART_BAUD_FRACTION_MASK) );

	HW_REG_WRITE32( uart->regmap->LineCon_H, uart->linectrl );
	HW_REG_WRITE32( uart->regmap->Control, uart->control );

	HW_REG_WRITE32( uart->regmap->IntClear, UART_INTBIT_ALL ); // clearing all the interrupts
	HW_REG_OR_WRITE32( uart->regmap->Control, UART_ENABLE );
	return TRUE;
}

//
//	Register Access & Callback Registration
//
int UART_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	UART_HANDLER_TYPE	*uart;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	uart = (UART_HANDLER_TYPE *)handler;

	switch(cmd){
		case	UART_SET_CLOCK:
				uart->uart_clk  = *((UINT32 *)data);
				break;
		case	UART_SET_BAUDRATE:
				uart->baudrate	= *((UINT32 *)data);
				break;
		case	UART_GET_BAUDRATE:
				*((UINT32 *)data) = uart->baudrate ;
				break;

		case	UART_SET_LINECTRL:
				uart->linectrl = *((UINT32 *)data);
				break;
		case	UART_GET_LINECTRL:
				*((UINT32 *)data) = uart->linectrl ;
				break;

		case	UART_SET_CONTROL:
				uart->control = *((UINT32 *)data);
				break;
		case	UART_GET_CONTROL:
				*((UINT32 *)data) = uart->control ;
				break;

		case	UART_SET_INT:
				uart->intctrl = *((UINT32 *)data);
				break;

		case	UART_GET_INT:
				*((UINT32 *)data) = uart->intctrl;
				break;
		case    UART_SET_FIFO_INT_LEVEL:
		  		uart->fifo_level = *((UINT32 *)data);
	  			break;

		case    UART_GET_FIFO_INT_LEVEL:
		  		*((UINT32 *)data) = uart->fifo_level;
				break;

		case 	UART_SET_USE_DMA:
			  	uart->use_dma = *((UINT32 *)data);
	  			break;

		case	UART_GET_USE_DMA:
				*((UINT32 *)data) = uart->use_dma;
				break;

		case	UART_CHECK_RXEMPTY:
				{
					UINT32 temp = (UINT32)uart_check_rxempty(uart);
					*((UINT32 *)data) = temp ;
				}
				break;
		case	UART_CHECK_RXFULL:
				if( (HW_REG_READ32(uart->regmap->Flag) & UART_RX_FIFO_FULL_MASK) == UART_RX_FIFO_FULL_MASK ){
					*((UINT32 *)data) = TRUE ;
				} else {
					*((UINT32 *)data) = FALSE ;
				}
				break;
		case	UART_CHECK_TXEMPTY:
				if( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_EMPTY_MASK) == UART_TX_FIFO_EMPTY_MASK ){
					*((UINT32 *)data) = TRUE ;
				} else {
					*((UINT32 *)data) = FALSE ;
				}
				break;
		case	UART_CHECK_TXFULL:
				if( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) == UART_TX_FIFO_FULL_MASK ){
					*((UINT32 *)data) = TRUE ;
				} else {
					*((UINT32 *)data) = FALSE ;
				}
				break;
		case	UART_CHECK_BUSY:
				if( (HW_REG_READ32(uart->regmap->Flag) & UART_MODEM_BUSY_MASK) == UART_MODEM_BUSY_MASK ){
					*((UINT32 *)data) = TRUE ;
				} else {
					*((UINT32 *)data) = FALSE ;
				}
				break;

		case	UART_SET_RX_SUSPEND:
				if( *((UINT32 *)data) == TRUE){
					uart->rx_suspend	= portMAX_DELAY ;
				} else {
					uart->rx_suspend	= 0 ;
				}
				break;
		case	UART_GET_RX_SUSPEND:
				if( data != NULL ){
					*((UINT32 *)data) = uart->rx_suspend;
				}
				break;

		case	UART_SET_WORD_ACCESS:
				uart->use_word_access = *((UINT32 *)data);
				break;

		case	UART_GET_WORD_ACCESS:
				*((UINT32 *)data) = uart->use_word_access;
				break;

		case	UART_SET_RW_WORD:
				uart->rw_word = *((UINT32 *)data);
				break;

		case	UART_GET_RW_WORD:
				*((UINT32 *)data) = uart->rw_word;
				break;

		case	UART_SET_SW_FLOW_CONTROL:
				uart->use_sw_flow_control = *((UINT32 *)data);
				break;

		case	UART_GET_SW_FLOW_CONTROL:
				*((UINT32 *)data) = uart->use_sw_flow_control;
				break;

		case	UART_SET_RS485:
				uart->use_rs485 = *((UINT32 *)data);
				break;

		case	UART_GET_RS485:
				*((UINT32 *)data) = uart->use_rs485;
				break;

		case 	UART_CLEAR_ERR_INT_CNT:
				uart->err_int_cnt = 0;
				break;
		case 	UART_GET_ERR_INT_CNT:
				*((UINT32 *)data) = uart->err_int_cnt;
				break;
		case 	UART_SET_ERR_INT_CALLBACK:
				uart->err_int_callback = (void (*)(HANDLE))(data);
				break;

		case 	UART_CLEAR_FRAME_INT_CNT:
				uart->frame_int_cnt = 0;
				break;
		case 	UART_GET_FRAME_INT_CNT:
				*((UINT32 *)data) = uart->frame_int_cnt;
				break;
		case 	UART_SET_FRAME_INT_CALLBACK:
				uart->frame_int_callback = (void (*)(HANDLE))(data);
				break;

		case	UART_CLEAR_PARITY_INT_CNT:
				uart->parity_int_cnt = 0;
				break;
		case 	UART_GET_PARITY_INT_CNT:
				*((UINT32 *)data) = uart->parity_int_cnt;
				break;
		case 	UART_SET_PARITY_INT_CALLBACK:
				uart->parity_int_callback = (void (*)(HANDLE))(data);
				break;

		case 	UART_CLEAR_BREAK_INT_CNT:
				uart->break_int_cnt = 0;
				break;
		case	UART_GET_BREAK_INT_CNT:
				*((UINT32 *)data) = uart->break_int_cnt;
				break;
		case 	UART_SET_BREAK_INT_CALLBACK:
				uart->break_int_callback = (void (*)(HANDLE))(data);
				break;

		case 	UART_CLEAR_OVERRUN_INT_CNT:
				uart->overrun_int_cnt = 0;
				break;
		case 	UART_GET_OVERRUN_INT_CNT:
				*((UINT32 *)data) = uart->overrun_int_cnt;
				break;
		case 	UART_SET_OVERRUN_INT_CALLBACK:
				uart->overrun_int_callback = (void (*)(HANDLE))(data);
				break;
		case	UART_GET_PORT:
				*((UINT32 *)data) = uart->dev_unit;
				break;
		case UART_SET_SW_RX_QUESIZE:
				uart->que_size = *((UINT32 *)data);
				break;
		default:
				return FALSE;
	}

	return TRUE;
}

//
//	UART_READ
//

int UART_READ (HANDLE handler, VOID *p_data, UINT32 p_dlen)
{
	UART_HANDLER_TYPE	*uart;
	UINT32				p_data32;
	UINT32				i/*, tdlen*/;
	INT32				questatus;
	UINT8				*p_data8;
	UINT32				ret_len = (INT32)p_dlen;
	UINT32				fifo;
	UINT32				*p_fifo;
	UINT32				fifo_length;
	UINT32				backup_int;
	p_fifo = &fifo;

	// Handler Checking
	if(handler == NULL || p_data == NULL) {
		return FALSE;
	}
	uart = (UART_HANDLER_TYPE *)handler ;
	p_data8 = (UINT8 *)p_data;

	// check the length with fifo level
	if (uart->linectrl & UART_FIFO_ENABLE(1)) {
		fifo_length = GET_FIFO_LENGTH(HW_REG_READ32(uart->regmap->FifoIntLevel) >> 3);

		// Correct data of rx_queue
		if(xQueueIsQueueEmpty(uart->rx_queue) != pdTRUE)
		{
			while(p_dlen){
				questatus = xQueueReceive(uart->rx_queue, (VOID *)(&fifo), 0);
				if (questatus == pdTRUE ) {
					*p_data8 = (UINT8) fifo ;
					p_data8++;
					p_dlen -= 1;
				}
				if(xQueueIsQueueEmpty(uart->rx_queue) == pdTRUE)
				{
					HW_REG_OR_WRITE32(uart->regmap->IntMask, UART_INTBIT_RECEIVE);
					break;
				}
			}

			if(p_dlen==0)
			{
				goto end_process;
			}
		}

		HW_REG_OR_WRITE32(uart->regmap->IntMask, UART_INTBIT_RECEIVE);
		while (p_dlen >= fifo_length) {
			// disable timeout interrupt
			uart->fifo_memcpy = 1;
			HW_REG_AND_WRITE32( uart->regmap->IntMask, ~UART_INTBIT_TIMEOUT );

			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&fifo), (UNSIGNED)(uart->rx_suspend));

			if (questatus == pdTRUE ) {
				if (fifo == (UINT32)(&(uart->ch)) ) {
					memcpy(p_data8, (void const *)(*p_fifo), fifo_length );
					p_dlen -= fifo_length;
					p_data8 += fifo_length;
					uart->ch[UART_CHK_OVERRUN] = 0;
				} else {
					*p_data8 = (UINT8) fifo ;
					p_data8++;
					p_dlen -= 1;
				}
			} else {
				return ret_len - p_dlen;
			}
		}
		uart->fifo_memcpy = 0;

		// enable timeout interrupt
		HW_REG_OR_WRITE32( uart->regmap->IntMask, UART_INTBIT_TIMEOUT );
		for (i = 0; i < p_dlen ; i++) {
			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&(p_data32)), (UNSIGNED)(uart->rx_suspend));
			if(questatus != pdTRUE ) {
				DRV_DBG_ERROR(" %p, questatus %d", uart , questatus );
				return i;
			}
			*p_data8 = (UINT8) p_data32 ;
			p_data8++;
		}
	} else {
		for(i=0; i < p_dlen; i++) {
			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&(p_data32)), (UNSIGNED)(uart->rx_suspend));
			if(questatus != pdTRUE ) {
				DRV_DBG_ERROR(" %p, questatus %d", uart , questatus );
				return i;
			}
			*p_data8 = (UINT8) p_data32 ;
			p_data8++;
		}
	}

end_process:
	if (uart->err_int_cnt) {
		/* atomic code */
		backup_int = HW_REG_READ32(uart->regmap->IntMask);
		HW_REG_WRITE32( uart->regmap->IntMask, 0 );
		uart->err_int_cnt = (uart->err_int_cnt > ret_len)? (uart->err_int_cnt - ret_len):0;
		HW_REG_WRITE32( uart->regmap->IntMask, backup_int );
		/* atomic code end */
		return (-ret_len);
	}

	return ret_len;
}

int UART_READ_POLLING (HANDLE handler, VOID *p_data, UINT32 p_dlen)
{
	UART_HANDLER_TYPE   *uart;
	UINT32				p_data32;
	UINT32				i;
	INT32				questatus;
	UINT8				*p_data8;
	UINT32				ret_len = p_dlen;
	UINT32				fifo;
	UINT32				*p_fifo;
	UINT32				fifo_length;
	p_fifo = &fifo;
	UINT32				loop_count = 0;
	UINT32				backup_int;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	uart = (UART_HANDLER_TYPE *)handler ;
	p_data8 = (UINT8 *)p_data;

	// check the length with fifo level
	if (uart->linectrl & UART_FIFO_ENABLE(1)) {
		fifo_length = GET_FIFO_LENGTH(HW_REG_READ32(uart->regmap->FifoIntLevel) >> 3);

		// more than fifo level  uart->regmap->FifoIntLevel
		while (p_dlen >= fifo_length) {
			// disable timeout interrupt
			uart->fifo_memcpy = 1;
			HW_REG_AND_WRITE32( uart->regmap->IntMask, ~UART_INTBIT_TIMEOUT );

			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&fifo), (UNSIGNED)(uart->rx_suspend));

			if (fifo == (UINT32)(&(uart->ch)) ) {
				memcpy(p_data8, (void const *)(*p_fifo), fifo_length );
				p_dlen -= fifo_length;
				p_data8 += fifo_length;
			} else {
				*p_data8 = (UINT8) fifo ;
				p_data8++;
				p_dlen -= 1;
			}
		}

		uart->fifo_memcpy = 0;

		// enable timeout interrupt
		HW_REG_OR_WRITE32( uart->regmap->IntMask, UART_INTBIT_TIMEOUT );
		for (i = 0; i < p_dlen ; i++) {
			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&(p_data32)), (UNSIGNED)(uart->rx_suspend));
			if(questatus != pdPASS ){
				DRV_DBG_ERROR(" %p, questatus %d", uart , questatus );
				return i;
			}
			*p_data8 = (UINT8) p_data32 ;
			p_data8++;

loop2:
			if (rxQueueFlag == 0) {
				goto loop2;
			}
			loop_count = 0;
			*p_data8 = rxQueueGlobal ;
			p_data8++;
			rxQueueFlag = 0;

		}
	} else {
		for(i=0; i < p_dlen; i++) {
			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&(p_data32)), (UNSIGNED)(uart->rx_suspend));
			if(questatus != pdPASS ) {
				DRV_DBG_ERROR(" %p, questatus %d", uart , questatus );
				return i;
			}

			*p_data8 = (UINT8) p_data32 ;
			p_data8++;
		}
	}
	if (uart->err_int_cnt) {
		/* atomic code */
		backup_int = HW_REG_READ32(uart->regmap->IntMask);
		HW_REG_WRITE32( uart->regmap->IntMask, 0 );
		uart->err_int_cnt = (uart->err_int_cnt > ret_len)? (uart->err_int_cnt - ret_len):0;
		HW_REG_WRITE32( uart->regmap->IntMask, backup_int );
		/* atomic code end */
		return -ret_len;
	}
	return ret_len;
}

//
//	UART_WRITE
//

int UART_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen)
{

	UART_HANDLER_TYPE	*uart;
	UINT32				i;
	UINT8				*p_data8;
	volatile UART_DATA 	data32;
	volatile int j;
	UINT32 remained_data_size;
	INT32				questatus;
	UINT32 				count = 0;
	UINT32 				p_data32/*, tdlen*/;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler ;
	p_data8 = (UINT8 *)p_data;

	// check the length with fifo level
	if (uart->linectrl & UART_FIFO_ENABLE(1)) {
		while( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) == 0) {

			if (uart->rw_word) {
				remained_data_size = p_dlen - count;
				if (remained_data_size < 4) {
					HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[count++])) );
				} else {
					for (j=0; j<4; j++) {
						data32.c[j] = (UINT8)p_data8[count++];
					}
					HW_REG_WRITE32(uart->regmap->DataRead, ((UINT32)data32.i));
				}
			} else {
				HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[count++])) );
			}

			if (count == p_dlen)
				return count;
		}

		while ( count < p_dlen) {
			if (uart->tx_queue) {
				questatus = xQueueReceive(uart->tx_queue, (VOID *)(&(p_data32)), portMAX_DELAY);
				if(questatus != pdPASS ){
					DRV_DBG_ERROR(" %p, questatus %d ", uart , questatus );
					return count;
				}
			}

			while( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) == 0) {
				if (uart->rw_word) {
					remained_data_size = p_dlen - count;
					if (remained_data_size < 4) {
						HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[count++])) );
					} else {
						for (j=0; j<4; j++) {
							data32.c[j] = (UINT8)p_data8[count++];
						}
						HW_REG_WRITE32(uart->regmap->DataRead, ((UINT32)data32.i));
					}
				} else {
					HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[count++])) );
				}

				if (count == p_dlen)
					return count;
			}
		}
		return count;
	} else {
		for(i=0; i<p_dlen; i++) {
			while( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) != 0);
			HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[i])) );
		}
		return i;
	}
}

int UART_DEBUG_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen)
{
	UART_HANDLER_TYPE	*uart;
	UINT32				i;
	UINT8		*p_data8;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler ;
	p_data8 = (UINT8 *)p_data;

	for(i=0; i<p_dlen; i++) {
		while( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) != 0)
			;

		HW_REG_WRITE32(uart->regmap->DataRead, ((unsigned int) (p_data8[i])) );
	}
	return (int)i;
}

int	UART_DMA_READ_TIMEOUT(HANDLE handler, VOID *p_data, UINT32 p_dlen, UINT32 timeout)
{
	UART_HANDLER_TYPE	*uart;
	UINT32 temp, queue,/*tdlen,*/ extra, i, interrupt;
	UINT32 ret_len = p_dlen;
	UINT8 *p8_data;
	UINT8 fifo;
	INT32				questatus;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	p8_data = (UINT8 *)p_data;
	uart = (UART_HANDLER_TYPE *)handler ;

	interrupt = HW_REG_READ32(uart->regmap->IntMask);
	taskENTER_CRITICAL();
	HW_REG_WRITE32(uart->regmap->IntMask, (UINT32) (0x00) );

	if(xQueueIsQueueEmpty(uart->rx_queue) != pdTRUE)
	{
		while(p_dlen){
			questatus = xQueueReceive(uart->rx_queue, (VOID *)(&fifo), 0);
			if (questatus == pdTRUE ) {
				*p8_data = (UINT8) fifo ;
				p8_data++;
				p_dlen -= 1;
			}
			if(xQueueIsQueueEmpty(uart->rx_queue) == pdTRUE)
			{
				break;
			}
		}

		if(p_dlen == 0)
		{
			taskEXIT_CRITICAL();
			return ret_len - p_dlen;
		}
	}

	temp = GET_FIFO_LENGTH(uart->fifo_level >> 3);
	extra = p_dlen % temp;

	sys_dma_copy(uart->dma_channel_rx, (UINT32 *)p8_data, (UINT32 *)(&(uart->regmap->DataRead)), p_dlen - extra);
	HW_REG_WRITE32(uart->regmap->DmaCon, (UINT32) (0x01) );
	taskEXIT_CRITICAL();

	xQueueReceive(uart->dma_queue, (VOID *)(&queue), timeout);
	HW_REG_WRITE32(uart->regmap->DmaCon, (UINT32) (0x00) );

	if (extra) {
		for(i = 0; i < extra; i++) {
			while((HW_REG_READ32(uart->regmap->Flag) & UART_RX_FIFO_EMPTY_MASK));
			p8_data[p_dlen - extra + i] = (UINT8)HW_REG_READ8(uart->regmap->DataRead);
		}
	}

	HW_REG_WRITE32(uart->regmap->IntMask, interrupt);

	return ret_len;
}

int	UART_DMA_READ(HANDLE handler, VOID *p_data, UINT32 p_dlen)
{
    return UART_DMA_READ_TIMEOUT(handler, p_data, p_dlen, portMAX_DELAY);
}

int UART_DMA_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen)
{
	UART_HANDLER_TYPE	*uart;
	UINT32 temp;
	UINT32 remained_data_size = 0;
	UINT32 dma_data_len = 0;
	UINT8 *p_data8 = (UINT8 *)p_data;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler ;
	if (uart->use_word_access) {
		remained_data_size = p_dlen % 4;
		dma_data_len = p_dlen - remained_data_size;
		sys_dma_copy(uart->dma_channel_tx, (UINT32 *)(&(uart->regmap->DataRead)),(UINT32 *)p_data, dma_data_len);
	} else {
		sys_dma_copy(uart->dma_channel_tx, (UINT32 *)(&(uart->regmap->DataRead)),(UINT32 *)p_data, p_dlen);
	}

	HW_REG_WRITE32(uart->regmap->DmaCon, (UINT32) (0x02) );

	xQueueReceive(uart->dma_queue, (VOID *)(&(temp)), portMAX_DELAY);

	HW_REG_WRITE32(uart->regmap->DmaCon, (UINT32) (0x00) );

	if (uart->use_word_access)	{
		for (volatile unsigned int i = 0; i < remained_data_size; i++) {
			while( (HW_REG_READ32(uart->regmap->Flag) & UART_TX_FIFO_FULL_MASK) != 0);
			HW_REG_WRITE8(uart->regmap->DataRead, ((UINT8) (p_data8[dma_data_len + i])));
		}
	}

	return p_dlen;
}
//
//	Close Handle
//

int 	UART_CLOSE(HANDLE handler)
{
	UART_HANDLER_TYPE	*uart;

	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}
	uart = (UART_HANDLER_TYPE *)handler;

	vQueueDelete(uart->rx_queue);
	vQueueDelete(uart->tx_queue);
	vQueueDelete(uart->dma_queue);


	if ( _uart_instance[uart->dev_unit] > 0) {
		_uart_instance[uart->dev_unit] -- ;

		if(_uart_instance[uart->dev_unit] == 0) {

			/*
			 * Disable UART Interrupt
			 */
			HW_REG_AND_WRITE32( uart->regmap->Control, (UART_DISABLE));

		    // Clock Enable
		    switch(uart->dev_unit){
				case	UART_UNIT_0:
				  		// Close ISR
				  		_sys_nvic_write(UART_0_IRQn, NULL, 0x7);
						/* clock gating off */
						DA16X_CLKGATE_OFF(clkoff_uart[0]);
						break;
				case	UART_UNIT_1:
				  		// Close ISR
				  		_sys_nvic_write(UART_1_IRQn, NULL, 0x7);
						/* clock gating off */
						DA16X_CLKGATE_OFF(clkoff_uart[1]);
						break;
				case	UART_UNIT_2:
				  		// Close ISR
				  		_sys_nvic_write(UART_2_IRQn, NULL, 0x7);
						/* clock gating off */
						DA16X_CLKGATE_OFF(clkoff_uart[1]);
						break;
		    }
		}
	}

	if (uart->use_dma)	{
		sys_dma_release_channel(uart->dma_channel_rx);
		sys_dma_release_channel(uart->dma_channel_tx);
	}

	return TRUE;
}

//---------------------------------------------------------
//	DRIVER :: Internal Functions
//---------------------------------------------------------


static UINT32  uart_baudrate_calc(UINT32 uartclk, UINT32 BaudRate, UINT32 *FractionalBaud, UINT32 *IntegralVal)
{
	UINT32 MultFactor, NewClockRate;

	//If NULL Pointer is sent then return error.
	if(FractionalBaud == NULL)
		return FALSE;
	if(IntegralVal == NULL)
		return FALSE;

	//Finding the divisor value
	*IntegralVal = uartclk /(16*BaudRate);
	NewClockRate = uartclk - (*IntegralVal)*16*BaudRate;
	if (BaudRate > 230400)	{
		MultFactor = 10;
		*FractionalBaud = ((((NewClockRate * MultFactor)/(16*BaudRate))*64 + 5*MultFactor/10)/MultFactor);
	} else {
		MultFactor = 1000;
		*FractionalBaud = ((((NewClockRate * MultFactor)/(16*BaudRate))*64 + 5*MultFactor/10)/MultFactor);
	}

	return TRUE;
}

//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------
void _sys_uart_isr(UINT32 idx)
{
	UINT32 int_status;
	UINT32 length, i;
	UINT8   *ch;
	void 	*p_msg;
	void 	*pp_msg;
	volatile UINT32 data = 0;
	volatile UART_DATA *p_data;
	BaseType_t	xHigherPriorityTaskWoken = pdFALSE;

	ch = _uart_handler[idx]->ch;
	p_msg = (void*)(&(_uart_handler[idx]->ch));
	pp_msg = &p_msg;

	int_status = _uart_handler[idx]->regmap->Interrupt;

	if (int_status & UART_INTBIT_RECEIVE) {
		// check fifo mode
		if ((_uart_handler[idx]->regmap->LineCon_H) & 0x10) {
			// check fifo level
			length = GET_FIFO_LENGTH((_uart_handler[idx]->regmap->FifoIntLevel) >> 3);

			if (_uart_handler[idx]->rw_word)
				length = length / 4;

			for(i = 0; i < length; i++) {
				if (_uart_handler[idx]->fifo_memcpy) {
					if (_uart_handler[idx]->rw_word) {
						data = HW_REG_READ32(_uart_handler[idx]->regmap->DataRead);
						p_data = (UART_DATA *)&data;
						for (UINT32 j = 0; j < 4; j++) {
							ch[i*4+j] = p_data->c[j];
						}
					} else {
						if (ch[UART_CHK_OVERRUN] != 0) {
							int_status |= UART_INTBIT_OVERRUN;
						}
						ch[i] = HW_REG_READ8(_uart_handler[idx]->regmap->DataRead);
					}
				} else {
					if (_uart_handler[idx]->rw_word) {
						data = HW_REG_READ32(_uart_handler[idx]->regmap->DataRead);
						p_data = (UART_DATA *)&data;
						for (UINT32 j = 0; j < 4; j++) {
							ch[i*4+j] = p_data->c[j];
						}
					} else {
						if(xQueueIsQueueFullFromISR(_uart_handler[idx]->rx_queue) != pdTRUE )
						{
							ch[0] = HW_REG_READ8(_uart_handler[idx]->regmap->DataRead);
							xQueueSendFromISR(_uart_handler[idx]->rx_queue, ch, &xHigherPriorityTaskWoken );
						}else{
							HW_REG_AND_WRITE32( _uart_handler[idx]->regmap->IntMask, ~(UART_INTBIT_TIMEOUT| UART_INTBIT_RECEIVE) );
							break;
						}
					}

				}
			}

			if (_uart_handler[idx]->fifo_memcpy) {
				if(xQueueIsQueueFullFromISR(_uart_handler[idx]->rx_queue) != pdTRUE )
				{
					ch[UART_CHK_OVERRUN] = 1;
					xQueueSendFromISR(_uart_handler[idx]->rx_queue, pp_msg, &xHigherPriorityTaskWoken);
				}else{
					HW_REG_AND_WRITE32( _uart_handler[idx]->regmap->IntMask, ~(UART_INTBIT_TIMEOUT| UART_INTBIT_RECEIVE) );

				}
			}

		}else {
			if (_uart_handler[idx]->rw_word) {
				data = HW_REG_READ32(_uart_handler[idx]->regmap->DataRead);
				p_data = (UART_DATA *)&data;
				for (volatile UINT32 data_idx=0; data_idx < 4; data_idx++) {
					if(xQueueIsQueueFullFromISR(_uart_handler[idx]->rx_queue) != pdTRUE )
					{
						xQueueSendFromISR(_uart_handler[idx]->rx_queue, (UINT8 *)&p_data->c[data_idx], &xHigherPriorityTaskWoken);
					}else{
						HW_REG_AND_WRITE32( _uart_handler[idx]->regmap->IntMask, ~(UART_INTBIT_TIMEOUT| UART_INTBIT_RECEIVE) );

					}
				}
			} else {
				if(xQueueIsQueueFullFromISR(_uart_handler[idx]->rx_queue) != pdTRUE )
				{
					ch[0] = HW_REG_READ8(_uart_handler[idx]->regmap->DataRead);
					xQueueSendFromISR(_uart_handler[idx]->rx_queue, ch, &xHigherPriorityTaskWoken);
				}else{
					HW_REG_AND_WRITE32( _uart_handler[idx]->regmap->IntMask, ~(UART_INTBIT_TIMEOUT| UART_INTBIT_RECEIVE) );

				}
			}
		}
	}

	if (int_status & UART_INTBIT_TRANSMIT) {
		if (_uart_handler[idx]->tx_queue) {
			xQueueSendFromISR(_uart_handler[idx]->tx_queue, ch, &xHigherPriorityTaskWoken);
		}
	}

	if (int_status & UART_INTBIT_TIMEOUT) {
		// check fifo mode
	    if ((_uart_handler[idx]->regmap->LineCon_H) & 0x10) {
			while(HW_REG_READ32(_uart_handler[idx]->regmap->Interrupt) & UART_INTBIT_TIMEOUT) {
				if(xQueueIsQueueFullFromISR(_uart_handler[idx]->rx_queue) != pdTRUE )
				{
					ch[0] = HW_REG_READ8(_uart_handler[idx]->regmap->DataRead);
					xQueueSendFromISR(_uart_handler[idx]->rx_queue, ch, &xHigherPriorityTaskWoken);
				}else{
					HW_REG_AND_WRITE32( _uart_handler[idx]->regmap->IntMask, ~(UART_INTBIT_TIMEOUT| UART_INTBIT_RECEIVE) );
					break;
				}
			}

		}
	}

	if (int_status & UART_INTBIT_ERROR)	{
		_uart_handler[idx]->err_int_cnt++;
		if (_uart_handler[idx]->err_int_callback != NULL)
			_uart_handler[idx]->err_int_callback((HANDLE)(&_uart_handler[idx]));
	}
	if (int_status & UART_INTBIT_FRAME)	{
		_uart_handler[idx]->frame_int_cnt++;
		if (_uart_handler[idx]->frame_int_callback != NULL)
			_uart_handler[idx]->frame_int_callback((HANDLE)(&_uart_handler[idx]));
	}
	if (int_status & UART_INTBIT_PARITY) {
		_uart_handler[idx]->parity_int_cnt++;
		if (_uart_handler[idx]->parity_int_callback != NULL)
			_uart_handler[idx]->parity_int_callback((HANDLE)(&_uart_handler[idx]));
	}
	if (int_status & UART_INTBIT_BREAK)	{
		_uart_handler[idx]->break_int_cnt++;
		if (_uart_handler[idx]->break_int_callback != NULL)
			_uart_handler[idx]->break_int_callback((HANDLE)(&_uart_handler[idx]));
	}
	if (int_status & UART_INTBIT_OVERRUN) {
		_uart_handler[idx]->overrun_int_cnt++;
		if (_uart_handler[idx]->overrun_int_callback != NULL)
			_uart_handler[idx]->overrun_int_callback((HANDLE)(&_uart_handler[idx]));
	}

	_uart_handler[idx]->regmap->IntClear = int_status;
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void _uart_isr0(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL_PARA(_sys_uart_isr, UART_UNIT_0);
	traceISR_EXIT();
}

static void _uart_isr1(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL_PARA(_sys_uart_isr, UART_UNIT_1);
	traceISR_EXIT();
}

static void _uart_isr2(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL_PARA(_sys_uart_isr, UART_UNIT_2);
	traceISR_EXIT();
}

/******************************************************************************
 *  uart_check_rxempty( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
static int	uart_check_rxempty(UART_HANDLER_TYPE *uart)
{
	if (uxQueueSpacesAvailable(uart->rx_queue) == uart->que_size ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int UART_FLUSH(HANDLE handler)
{
	UART_HANDLER_TYPE	*uart;
	UINT32 backup_int;
	// Handler Checking
	if(handler == NULL) {
		return FALSE;
	}

	uart = (UART_HANDLER_TYPE *)handler;

    /* atomic code */
	backup_int = HW_REG_READ32(uart->regmap->IntMask);
	HW_REG_WRITE32( uart->regmap->IntMask, 0 );
	uart->err_int_cnt = 0;
	uart->frame_int_cnt = 0;
	uart->parity_int_cnt = 0;
	uart->break_int_cnt = 0;
	uart->overrun_int_cnt = 0;
	while( !(HW_REG_READ32(uart->regmap->Flag) & UART_RX_FIFO_EMPTY_MASK)) {
		HW_REG_READ32( uart->regmap->DataRead);
	}
	xQueueReset(uart->rx_queue);
	HW_REG_WRITE32( uart->regmap->IntMask, backup_int );
	/* atomic code end */
	return TRUE;
}

#endif	/*SUPPORT_PL011UART*/
