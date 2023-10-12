/**
 * \addtogroup HALayer
 * \{
 * \addtogroup UART
 * \{
 * \brief UART driver
 */

/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief UART driver
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


#ifndef __uart_h__
#define __uart_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"
#include "oal.h"


#ifdef	SUPPORT_PL011UART
//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

#define SYS_UART_BASE_0		((UART_REG_MAP*)UART_BASE_0)
#define SYS_UART_BASE_1		((UART_REG_MAP*)UART_BASE_1)
#define SYS_UART_BASE_2		((UART_REG_MAP*)UART_BASE_2)
#ifndef HW_REG_AND_WRITE32
#define	HW_REG_AND_WRITE32(x, v)	( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define	HW_REG_OR_WRITE32(x, v)		( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v) 		( *((volatile unsigned int   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define	HW_REG_READ32(x)			( *((volatile unsigned int   *) &(x)) )
#endif
//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef  enum __uart_unit__ {
	UART_UNIT_0 = 0,
	UART_UNIT_1 ,
	UART_UNIT_2 ,
	UART_UNIT_MAX
} UART_UNIT_IDX;

typedef		enum	__uart_parity__ {
	UART_PARITY_NONE = 0,
	UART_PARITY_EVEN,
	UART_PARITY_ODD,
} UART_PARITY_TYPE;

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
//
//	IOCTL Commands
//

typedef enum	{
	UART_GET_DEVREG = 1,

	UART_SET_CLOCK,
	UART_SET_BAUDRATE,
	UART_GET_BAUDRATE,

	UART_SET_LINECTRL,
	UART_GET_LINECTRL,

	UART_SET_CONTROL,
	UART_GET_CONTROL,
	/*UART_SET_QUESIZE,*/

	UART_SET_SW_RX_QUESIZE,

	UART_SET_INT,
	UART_GET_INT,

	UART_SET_FIFO_INT_LEVEL,
	UART_GET_FIFO_INT_LEVEL,

	UART_SET_USE_DMA,
	UART_GET_USE_DMA,

	UART_CHECK_RXEMPTY,
	UART_CHECK_RXFULL,
	UART_CHECK_TXEMPTY,
	UART_CHECK_TXFULL,
	UART_CHECK_BUSY,

	UART_SET_RX_SUSPEND,
	UART_GET_RX_SUSPEND,

	UART_SET_WORD_ACCESS,
	UART_GET_WORD_ACCESS,
	UART_SET_RW_WORD,
	UART_GET_RW_WORD,
	UART_SET_SW_FLOW_CONTROL,
	UART_GET_SW_FLOW_CONTROL,
	UART_SET_RS485,
	UART_GET_RS485,

	UART_CLEAR_ERR_INT_CNT,
	UART_GET_ERR_INT_CNT,
	UART_SET_ERR_INT_CALLBACK,

	UART_CLEAR_FRAME_INT_CNT,
	UART_GET_FRAME_INT_CNT,
	UART_SET_FRAME_INT_CALLBACK,

	UART_CLEAR_PARITY_INT_CNT,
	UART_GET_PARITY_INT_CNT,
	UART_SET_PARITY_INT_CALLBACK,

	UART_CLEAR_BREAK_INT_CNT,
	UART_GET_BREAK_INT_CNT,
	UART_SET_BREAK_INT_CALLBACK,

	UART_CLEAR_OVERRUN_INT_CNT,
	UART_GET_OVERRUN_INT_CNT,
	UART_SET_OVERRUN_INT_CALLBACK,

	UART_GET_PORT,

	UART_SET_MAX
} UART_DEVICE_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: Interrupt
//---------------------------------------------------------
//
//	Interrupt Register
//


typedef enum  {
	UART_INTIDX_MODEM = 0,
	UART_INTIDX_RX,
	UART_INTIDX_TX,
	UART_INTIDX_TIMEOUT,

	UART_INTIDX_MAX
}	UART_INTIDX_TYPE;

#define UART_INTBIT_RECEIVE 	   0x0010
#define UART_INTBIT_TRANSMIT	   0x0020
#define UART_INTBIT_TIMEOUT 	   0x0040
#define UART_INTBIT_MODEM		   0x000F
#define UART_INTBIT_MODEMRI 	   0x0001
#define UART_INTBIT_MODEMCTS	   0x0002
#define UART_INTBIT_MODEMDCD	   0x0004
#define UART_INTBIT_MODEMDSR	   0x0008
#define UART_INTBIT_ERROR		   0x0780
#define UART_INTBIT_FRAME		   0x0080
#define UART_INTBIT_PARITY		   0x0100
#define UART_INTBIT_BREAK		   0x0200
#define UART_INTBIT_OVERRUN 	   0x0400

#define	UART_INTBIT_ALL				(0x07F0)


#define UART_ISR_PRIORITY	DRIVER_ISR_PRIORITY
#define INT_MAX_UART_VEC	UART_INTIDX_MAX

//
//	UART Flag Register
//
typedef enum
{
	UART_MODEM_BUSY_MASK	= 0x00000008,
	UART_RX_FIFO_EMPTY_MASK	= 0x00000010,
	UART_TX_FIFO_FULL_MASK	= 0x00000020,
	UART_RX_FIFO_FULL_MASK	= 0x00000040,
	UART_TX_FIFO_EMPTY_MASK	= 0x00000080
}	T_UART_FLAG_REGISTER;

//UART Control Register
#define	UART_CONTROL_LOWBITS(x)		((x&0x07)<<0)
#define	UART_ENABLE					(0x01)
#define	UART_DISABLE				(~UART_ENABLE)
#define UART_SIR_ENABLE(x)			((x&0x1)<<1)
#define	UART_LOW_POWER_MODE(x)		((x&0x1)<<2)
#define UART_LOOP_BACK(x)			((x&0x1)<<7)

#define UART_CONTROL_HIGHBITS(x) 	((x&0x1F)<<7)
#define	UART_TRANSMIT_ENABLE(x)		((x&0x1)<<8)
#define UART_RECEIVE_ENABLE(x) 		((x&0x1)<<9)
#define UART_TRANSMIT_READY(x) 		((x&0x1)<<10)
#define UART_REQUEST_SEND(x)		((x&0x1)<<11)
#define UART_UART_OUT1(x)			((x&0x1)<<12)
#define UART_UART_OUT2(x)			((x&0x1)<<13)
#define UART_HWFLOW_RTS(x)			((x&0x1)<<14)
#define UART_HWFLOW_CTS(x)			((x&0x1)<<15)
#define UART_HWFLOW_ENABLE(x)		(x == 1? (UART_HWFLOW_RTS(1) | UART_HWFLOW_CTS(1)) : 0)

//UART Line Control Register
#define	UART_FIFO_ENABLE(x)			((x&0x01)<<4)
#define UART_SEND_BREAK_ENABLE(x)	((x&0x01)<<0)
#define UART_PARITY_ENABLE(x)		((x&0x01)<<1)
#define UART_EVEN_PARITY(x)			((x&0x01)<<2)
#define UART_STOP_BIT(x) 			(((x-1)&0x01)<<3)
#define UART_WORD_LENGTH(x)			(((x-5)&0x03)<<5)
#define UART_STICK_PARITY_ENABLE(x) ((x&0x01)<<7)

//UART Baud Rate Register
typedef enum
{
	UART_BAUD_INTEGER_MASK	= 0x0000ffff,
	UART_BAUD_FRACTION_MASK	= 0x0000003f
}	T_UART_BAUDRATE_REGISTER;

//FIFO Level Register
#define	UART_TX_INT_LEVEL(x)	((x&0x07)<<0)
#define UART_RX_INT_LEVEL(x) 	((x&0x07)<<3)

#define	UART_ONEEIGHTH		0	// receive FIFO >= 1/8 full ( <= 1/8 for transmit).
#define	UART_ONEQUARTER		1   // receive FIFO >= 1/4 full ( <= for transmit).
#define UART_HALF			2   // receive FIFO >= 1/2 full ( <= for transmit).
#define UART_THREEQUARTERS  3	// receive FIFO >= 3/4 full ( <= for transmit).
#define UART_SEVENEIGHTHS   4   // receive FIFO >= 7/8 full ( <= 7/8 for transmit).

/* errors */
#define ERR_NONE					0x00000000
//
// Rx Buffer Size
//

#ifdef __BLE_COMBO_REF__
#define	UART_RX_BUFSIZ	256
#else
#define	UART_RX_BUFSIZ	128   /* Max fifo size 256*/
#endif // __BLE_COMBO_REF__

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------

//
//	Register MAP
//

typedef struct	__uart_reg_map__
{
	volatile UINT32	 DataRead;					 // 0x00
	volatile UINT32	 StatusClear;				 // 0x04
	volatile UINT32	 Reserved1; 				 // 0x08
	volatile UINT32	 Reserved2; 				 // 0x0C
	volatile UINT32	 Reserved3; 				 // 0x10
	volatile UINT32	 Reserved4; 				 // 0x14
	volatile UINT32	 Flag;						 // 0x18
	volatile UINT32	 Reserved5; 				 // 0x1C
	volatile UINT32	 LowPower;					 // 0x20
	volatile UINT32	 IntBaudDivisor;			 // 0x24
	volatile UINT32	 FractBaudDivisor;			 // 0x28
	volatile UINT32	 LineCon_H; 				 // 0x2C
	volatile UINT32	 Control;					 // 0x30
	volatile UINT32	 FifoIntLevel;				 // 0x34
	volatile UINT32	 IntMask;					 // 0x38
	volatile UINT32	 IntRaw;					 // 0x3C
	volatile UINT32	 Interrupt; 				 // 0x40
	volatile UINT32	 IntClear;					 // 0x44
	volatile UINT32	 DmaCon;					 // 0x48
	volatile UINT32  WordAccess;				 // 0x4C
	volatile UINT32  SWFlowControl;				 // 0x50
	volatile UINT32  RS485En;					 // 0x54
	volatile UINT32	 Reserved6[10]; 			 // 0x58
	volatile UINT32	 SIRTest;					 // 0x80
} UART_REG_MAP;

//
//	Driver Structure
//
typedef struct	__uart_device_handler__
{
	UINT32				dev_addr;  // Unique Address

	// Device-dependant
	UINT32				dev_unit;		// UINT number
	UINT32				uart_clk;
	UINT32				baudrate;
	UINT32				linectrl;
	UINT32				control;
	UINT32				intctrl;
	UINT32				fifo_level;
	UNSIGNED			rx_suspend;

	UINT32				instance;
	UART_REG_MAP		*regmap;

	// Sync b/w HISR & RW-Func
	UINT32				que_size;
	UINT32				*rx_quebuf;
	UINT32				tx_quebuf;
	UINT32				dma_quebuf;
	UINT32				use_dma;
	HANDLE				dma_channel_rx;
	HANDLE				dma_channel_tx;

	UINT32 				use_word_access;  	 // word access register configuration
	UINT32 				use_sw_flow_control; // sw flow control register configuration
	UINT32 				rw_word;
	UINT32 				use_rs485;
	UINT32				fifo_memcpy;

	// interrupt counter
	UINT32				err_int_cnt;
	UINT32				frame_int_cnt;
	UINT32				parity_int_cnt;
	UINT32				break_int_cnt;
	UINT32 				overrun_int_cnt;

	// interrupt callbacks
	void				(*err_int_callback)(HANDLE);
	void				(*frame_int_callback)(HANDLE);
	void				(*parity_int_callback)(HANDLE);
	void				(*break_int_callback)(HANDLE);
	void				(*overrun_int_callback)(HANDLE);

	UINT8				ch[32];
	char				rx_que_name[8];
	char				tx_que_name[8];
	char				dma_que_name[8];
	QueueHandle_t		rx_queue;
	QueueHandle_t		tx_queue;
	QueueHandle_t		dma_queue;

} UART_HANDLER_TYPE;

typedef struct {
	int (*sys_dma_copy)(HANDLE hdl, UINT32 *dst, UINT32 *src, UINT32 len);
	int (*sys_dma_registry_callback)(HANDLE hdl, USR_CALLBACK term, USR_CALLBACK empty, USR_CALLBACK error, HANDLE dev_handle);
	int (*sys_dma_release_channel)(HANDLE hdl);
} uart_dma_primitive_type;

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

/**
 ****************************************************************************************
 * @brief This function create UART handle. Normally, UART0 is used for debug console, and UART1 and UART2 are used for data transfer.
 * @param[in] dev_type Device index. The DA16200 can set UART_UNIT_0, UART_UNIT_1 and UART_UNIT_2.
 * @return If succeeded return handle for the device, if failed return NULL.
 *
 ****************************************************************************************
 */
extern HANDLE	UART_CREATE( UART_UNIT_IDX dev_idx );

/**
 ****************************************************************************************
 * @brief This function initialize UART instance. The UART configuration should be set before this function is called.
 * @param[in] handler Device handle.
 * @return TRUE if success, or FALSE if fails
 *
 ****************************************************************************************
 */
extern int 	UART_INIT (HANDLE handler);

/**
 ****************************************************************************************
 * @brief This function changes the baud rate of the UART during operation.
 * @param[in] handler Device handle.
 * @param[in] baudrate Baud rate
 * @return TRUE if success, or FALSE if fails
 *
 ****************************************************************************************
 */
extern int 	UART_CHANGE_BAUERATE (HANDLE handler, UINT32 baudrate);


/**
 ****************************************************************************************
 * @brief This function configures the UART.
 * @param[in] handler Device handle.
 * @param[in] cmd refer to the below information
 *   - UART_GET_DEVREG, // get device physical address
 *   - UART_SET_CLOCK, // set base clock
 *   - UART_SET_BAUDRATE, // set baud rate
 *   - UART_GET_BAUDRATE, // get baud rate
 *   - UART_SET_LINECTRL, // set line control
 *   - UART_GET_LINECTRL, // get line control
 *   - UART_SET_CONTROL, // set UART control
 *   - UART_GET_CONTROL, // get UART control
 *   - UART_SET_QUESIZE, // set queue size
 *   - UART_SET_INT, // set interrupt configuration
 *   - UART_GET_INT, // get interrupt configuration
 *   - UART_SET_FIFO_INT_LEVEL, // set fifo level
 *   - UART_GET_FIFO_INT_LEVEL, // get fifo level
 *   - UART_SET_USE_DMA, // set DMA use
 *   - UART_GET_USE_DMA, // get DMA use
 *   - UART_CHECK_RXEMPTY, // check RX fifo empty
 *   - UART_CHECK_RXFULL, // check RF fifo full
 *   - UART_CHECK_TXEMPTY, // check TX fifo empty
 *   - UART_CHECK_TXFULL, // check TX fifo full
 *   - UART_CHECK_BUSY, // check UART busy
 *   - UART_SET_RX_SUSPEND, // set the RX function to suspend
 *   - UART_CLEAR_ERR_INT_CNT, // clear error interrupt counter
 *   - UART_GET_ERR_INT_CNT, // gets error interrupt counter
 *   - UART_SET_ERR_INT_CALLBACK, // set error interrupt callback function
 *   - UART_CLEAR_FRAME_INT_CNT, //clear frame error interrupt counter
 *   - UART_GET_FRAME_INT_CNT, // get frame error interrupt counter.
 *   - UART_SET_FRAME_INT_CALLBACK, // set frame error interrupt callback
 *   - UART_CLEAR_PARITY_INT_CNT, // clear parity error interrupt counter
 *   - UART_GET_PARITY_INT_CNT, // get frame error interrupt counter
 *   - UART_SET_PARITY_INT_CALLBACK, // set frame error interrupt callback
 *   - UART_CLEAR_BREAK_INT_CNT, // clear break error interrupt counter
 *   - UART_GET_BREAK_INT_CNT, // get break error interrupt counter
 *   - UART_SET_BREAK_INT_CALLBACK, // set break error interrupt callback
 *   - UART_CLEAR_OVERRUN_INT_CNT, // clear overrun error interrupt counter
 *   - UART_GET_OVERRUN_INT_CNT, // get overrun error interrupt counter
 *   - UART_SET_OVERRUN_INT_CALLBACK, // set overrun interrupt callback
 * @param[in] Data pointer
 * @return TRUE if success, or FALSE if fails
 *
 ****************************************************************************************
 */
extern int 	UART_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 ****************************************************************************************
 * @brief This function read bytes from the UART.
 * @param[in] handler Device handle.
 * @param[in] p_data pointer to the buffer
 * @param[in] p_dlen data length
 * @return If succeeded return number of received data, else negative number.
 *
 ****************************************************************************************
 */
extern int 	UART_READ(HANDLE handler, VOID *p_data, UINT32 p_dlen);

/**
 ****************************************************************************************
 * @brief This function write bytes to the UART.
 * @param[in] handler Device handle.
 * @param[in] p_data pointer to the buffer
 * @param[in] p_dlen data length
 * @return Number of sent data
 *
 ****************************************************************************************
 */
extern int 	UART_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen);

/**
 ****************************************************************************************
 * @brief This function read bytes from the UART using DMA with waiting timeout.
 * @param[in] handler Device handle.
 * @param[in] p_data pointer to the buffer
 * @param[in] p_dlen data length
 * @param[in] timeout Wait option to receive data
 * @return Number of received data
 *
 ****************************************************************************************
 */
extern int	UART_DMA_READ_TIMEOUT(HANDLE handler, VOID *p_data, UINT32 p_dlen, UINT32 timeout);

/**
 ****************************************************************************************
 * @brief This function read bytes from the UART using DMA.
 * @param[in] handler Device handle.
 * @param[in] p_data pointer to the buffer
 * @param[in] p_dlen data length
 * @return Number of received data
 *
 ****************************************************************************************
 */
extern int 	UART_DMA_READ(HANDLE handler, VOID *p_data, UINT32 p_dlen);

/**
 ****************************************************************************************
 * @brief This function write bytes to the UART using DMA.
 * @param[in] handler Device handle.
 * @param[in] p_data pointer to the buffer
 * @param[in] p_dlen data length
 * @return Number of sent data
 *
 ****************************************************************************************
 */
extern int  UART_DMA_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen);

/**
 ****************************************************************************************
 * @brief This function flush the rx fifo of the UART.
 * @param[in] handler Device handle.
 * @return TRUE if success, or FALSE if fails
 *
 ****************************************************************************************
 */
extern int  UART_FLUSH(HANDLE handler);

/**
 ****************************************************************************************
 * @brief This function close the UART driver.
 * @param[in] handler Device handle.
 * @return TRUE if success, or FALSE if fails
 *
 ****************************************************************************************
 */
extern int 	UART_CLOSE(HANDLE handler);

extern int 	UART_DEBUG_WRITE(HANDLE handler, VOID *p_data, UINT32 p_dlen);
extern int UART_DMA_INIT(HANDLE handler, HANDLE dma_channel_tx, HANDLE dma_channel_rx, const uart_dma_primitive_type * primitive);

#endif	//SUPPORT_PL011UART

#endif	/*__uart_h__*/
