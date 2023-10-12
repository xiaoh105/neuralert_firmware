/**
 * \addtogroup HALayer
 * \{
 * \addtogroup SPI
 * \{
 * \brief SPI Master driver
 */

/**
 ****************************************************************************************
 *
 * @file spi.h
 *
 * @brief SPI Master driver
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


#ifndef __spi_h__

#define __spi_h__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef 	struct __spi_handler__		SPI_HANDLER_TYPE; 	// deferred
typedef 	struct __spi_common__		SPI_COMMON_TYPE; 	// deferred
typedef 	struct __spi_spi_domain__	SPI_SPI_DOMAIN_TYPE; 	// deferred
typedef 	struct __spi_dma_domain__	SPI_DMA_DOMAIN_TYPE; 	// deferred
typedef 	struct __spi_xip_domain__	SPI_XIP_DOMAIN_TYPE; 	// deferred


//---------------------------------------------------------
//	HAL :: Commands & Parameters
//---------------------------------------------------------

typedef enum {
	SPI_UNIT_0 = 0,
	SPI_UNIT_MAX
} SPI_UNIT_LIST;

typedef enum {
	SPI_SET_DEBUG = 0,
	SPI_SET_SPEED,
	SPI_GET_SPEED,
	SPI_SET_MAX_LENGTH,
	SPI_GET_MAX_LENGTH,
	SPI_SET_FORMAT,
	SPI_SET_DATASIZE,
	SPI_SET_INTRMODE,
	SPI_SET_DMAMODE,
	SPI_GET_XFERMODE,
	SPI_SET_CALLACK,

	// additional cmd
	SPI_CHECK_BUSY,
	SPI_CHECK_RX_FULL,
	SPI_CHECK_RX_NO_EMPTY,
	SPI_CHECK_TX_NO_FULL,
	SPI_CHECK_TX_EMPTY,

	SPI_SET_SLAVEMODE,
	SPI_SET_OUTEN,
	SPI_SET_CONCAT,

	SPI_GET_BUSCONTROL,
	SPI_SET_BUSCONTROL,
	SPI_GET_DELAYSEL,
	SPI_SET_DELAYSEL,

	SPI_SET_LOCK,

	SPI_SET_CORECLOCK,
	SPI_SET_TIMEOUT,
	SPI_SET_WIRE,
	SPI_SET_DMA_CFG,
	SPI_SET_PINCONTROL,

	SPI_SET_XIPMODE,
	SPI_SET_DELAY_INDEX,
	SPI_GET_DELAY_INDEX,

	SPI_SET_MAX

} SPI_IOCTL_LIST;



typedef enum	{
	SPI_CSEL_0	= 0,
	SPI_CSEL_1,
	SPI_CSEL_2,
	SPI_CSEL_3,

	SPI_CSEL_MAX
} SPI_CSEL_LIST;

typedef enum	{
	SPI_TYPE_MOTOROLA_O0H0 = 0,
	SPI_TYPE_MOTOROLA_O0H1,
	SPI_TYPE_MOTOROLA_O1H0,
	SPI_TYPE_MOTOROLA_O1H1,

	SPI_TYPE_MAX
} SPI_FORMAT_TYPE_LIST;

typedef enum	{
	SPI_DELAY_INDEX_LOW	= 0,
	SPI_DELAY_INDEX_MEDIUM,
	SPI_DELAY_INDEX_HIGH,
	SPI_DELAY_INDEX_MAX
} SPI_DELAY_INDEX_LIST;


//---------------------------------------------------------
//	HAL :: Offset
//---------------------------------------------------------

struct	__spi_common__
{
	volatile UINT32 SPI_OP_EN;
	volatile UINT32 SPI_REQ_START;
	volatile UINT32 SPI_REQ_CLR_IRQ;
	volatile UINT32 SPI_REQ_CLR_OP;
	volatile UINT32 SPI_IRQ_STATUS;
	volatile UINT32 IRQ_STATUS_DMA;
	volatile UINT32 IRQ_STATUS_SPI;
	volatile UINT32 IRQ_STATUS_XIP;

	volatile UINT32 SPI_CORE_RATIO;
	volatile UINT32 SPI_CORE_FREQ;
	volatile UINT32 _reserved0;
	volatile UINT32 _reserved1;

	volatile UINT32 CLK_TO_UNIT;
	volatile UINT32 CLK_TO_COUNT;
	volatile UINT32 _reserved2;
	volatile UINT32 _reserved3;

	volatile UINT32 CLK_CE_SEL_CYC;
	volatile UINT32 CLK_CE_DES_CYC;
	volatile UINT32 _reserved4;
	volatile UINT32 _reserved5;

	volatile UINT32 XFC_DBG_RX_SHIFT;
	volatile UINT32 XFC_SPI_RX_SHIFT_EN;
	volatile UINT32 XFC_SPI_CLK_1x1_EN;
	volatile UINT32 _reserved8;

	volatile UINT32 SPI_SPI_DLY_0_TXC;
	volatile UINT32 SPI_SPI_DLY_0_RXC;
	volatile UINT32 _reserved9;
	volatile UINT32 _reserved10;

	volatile UINT32 SPI_SPI_DLY_1_TXC;
	volatile UINT32 SPI_SPI_DLY_1_RXC;
	volatile UINT32 _reserved11;
	volatile UINT32 _reserved12;

	volatile UINT32 SPI_SPI_DLY_2_TXC;
	volatile UINT32 SPI_SPI_DLY_2_RXC;
	volatile UINT32 _reserved13;
	volatile UINT32 _reserved14;

	volatile UINT32 SPI_SPI_DLY_3_TXC;
	volatile UINT32 SPI_SPI_DLY_3_RXC;
};

struct	__spi_spi_domain__
{
	volatile UINT32	SPI_CLK_FREQ_S;
	volatile UINT32	SPI_CLK_FREQ_R;
	volatile UINT32	SPI_DMY_CYC;
	volatile UINT32	SPI_WAIT_CYC;

	volatile UINT32	SPI_INT_MODE;
	volatile UINT32	SPI_PHASE_MODE;
	volatile UINT32	SPI_BYTE_SWAP;
	volatile UINT32	SPI_WRAP_EN;

	volatile UINT32	SPI_BUS_TYPE;
	volatile UINT32	SPI_IO_DX_TYPE;
	volatile UINT32	SPI_IO_CK_TYPE;
	volatile UINT32	SPI_IO_CS_TYPE;

	volatile UINT32	SPI_CS_SEL;
	volatile UINT32	SPI_IO_1_TYPE;
	volatile UINT32	SPI_TWIN_QSPI_EN;
	volatile UINT32 _reserved0;

	volatile UINT32	SPI_REG_RW;
	volatile UINT32	SPI_REG_CMD;
	volatile UINT32	SPI_REG_ADDR;
	volatile UINT32	SPI_REG_MODE;

	volatile UINT32	SPI_TTL;
	volatile UINT32	SPI_DLY_INDEX;
	volatile UINT32 _reserved1;
	volatile UINT32 _reserved2;

	volatile UINT32	SPI_ACC_DIN;
	volatile UINT32	SPI_ACC_DOUT;
	volatile UINT32 _reserved3;
	volatile UINT32 _reserved4;

	volatile UINT32	SPI_OP_STATUS;
	volatile UINT32 _reserved5;
	volatile UINT32 _reserved6;
	volatile UINT32 _reserved7;
	volatile UINT32 SPI_LEGACY_EN;
	volatile UINT32 SPI_LEGACY_TX_SIZE;
	volatile UINT32 _reserved8;
	volatile UINT32 _reserved9;
	volatile UINT32 SPI_LEGACY_TX_REG_00;
};

struct	__spi_dma_domain__
{
	volatile UINT32	DMA_MP0_CFG;
	volatile UINT32	DMA_MP0_TASK;
	volatile UINT32	DMA_IRQ_MSK_EN;
	volatile UINT32	DMA_MP0_FIFO_TH;

	volatile UINT32	DMA_TSK_0_Addr;
	volatile UINT32	DMA_TSK_0_TTL;
	volatile UINT32	DMA_TSK_1_Addr;
	volatile UINT32	DMA_TSK_1_TTL;
	volatile UINT32	DMA_TSK_2_Addr;
	volatile UINT32	DMA_TSK_2_TTL;
	volatile UINT32	DMA_TSK_3_Addr;
	volatile UINT32	DMA_TSK_3_TTL;
	volatile UINT32	DMA_TSK_4_Addr;
	volatile UINT32	DMA_TSK_4_TTL;
	volatile UINT32	DMA_TSK_5_Addr;
	volatile UINT32	DMA_TSK_5_TTL;
	volatile UINT32	DMA_TSK_6_Addr;
	volatile UINT32	DMA_TSK_6_TTL;
	volatile UINT32	DMA_TSK_7_Addr;
	volatile UINT32	DMA_TSK_7_TTL;

	volatile UINT32	_reserved0;
	volatile UINT32	_reserved1;
	volatile UINT32	_reserved2;
	volatile UINT32	_reserved3;

	volatile UINT32	DMA_OP_STATUS;
	volatile UINT32	DMA_NEXT_OP_TASK;
	volatile UINT32	DMA_CURR_TSK_Addr;
	volatile UINT32	DMA_CURR_TSK_TTL;

	volatile UINT32	DMA_FIFO_ADDR;
	volatile UINT32	_reserved4;
	volatile UINT32	_reserved5;
	volatile UINT32	_reserved6;

	volatile UINT32	DMA_CLK_FREQ_S;
	volatile UINT32	DMA_CLK_FREQ_R;
	volatile UINT32	DMA_DLY_INDEX;
};

struct	__spi_xip_domain__
{
	volatile UINT32	XIP_CLK_FREQ_S;
	volatile UINT32	XIP_CLK_FREQ_R;
	volatile UINT32	XIP_DMY_CYC;
	volatile UINT32	XIP_WAIT_CYC;

	volatile UINT32	XIP_INT_MODE;
	volatile UINT32	XIP_PHASE_MODE;
	volatile UINT32	XIP_BYTE_SWAP;
	volatile UINT32	XIP_WRAP_EN;

	volatile UINT32	XIP_BUS_TYPE;
	volatile UINT32	XIP_IO_DX_TYPE;
	volatile UINT32	XIP_IO_CK_TYPE;
	volatile UINT32	XIP_IO_CS_TYPE;

	volatile UINT32	XIP_CS_SEL;
	volatile UINT32	XIP_IO_1_TYPE;
	volatile UINT32	XIP_TWIN_QSPI_EN;
	volatile UINT32 _reserved0;

	volatile UINT32	XIP_REG_RW;
	volatile UINT32	XIP_REG_CMD;
	volatile UINT32	_reserved1;
	volatile UINT32	XIP_REG_MODE;

	volatile UINT32	XIP_SPI_BST;
	volatile UINT32	XIP_DLY_INDEX;
	volatile UINT32	_reserved2;
	volatile UINT32	_reserved3;

	volatile UINT32	_reserved4;
	volatile UINT32	XIP_CURR_DOUT;
	volatile UINT32	XIP_CURR_ADDR;
	volatile UINT32	_reserved5;

	volatile UINT32	XIP_OP_STATUS;
};



//---------------------------------------------------------
//	HAL :: Definition
//---------------------------------------------------------
// BUS CONTROL [0]
#define SPI_SPI_TIMEOUT_EN			(1 << 4)

#define SPI_SPI_PHASE_MODE_8BIT		((0) << 2)
#define SPI_SPI_PHASE_MODE_4BIT		((1) << 2)
#define SPI_SPI_PHASE_MODE_2BIT		((2) << 2)
#define SPI_SPI_PHASE_MODE_1BIT		((3) << 2)
#define SPI_SPI_PHASE_CMD_1BYTE		((0) << 1)
#define SPI_SPI_PHASE_CMD_2BYTE		((1) << 1)
#define SPI_SPI_PHASE_ADDR_3BYTE	((0) << 0)
#define SPI_SPI_PHASE_ADDR_4BYTE	((1) << 0)

#define	SPI_SPI_PHASE_DUMMY_MASK	((0x1f) << 16)
#define	SPI_SET_SPI_DUMMY_CYCLE(x)	(((x) & 0x1f) << 16)
#define	SPI_GET_SPI_DUMMY_CYCLE(x)	(((x) >> 16) & 0x1f)

// BUS CONTROL [1]
#define SPI_BUS_TYPE(on,dtr,bits)	(((on)<<3) | ((dtr)<<2) | (bits))
#define	SPI_BUS_SPI				(0)
#define	SPI_BUS_DPI				(1)
#define	SPI_BUS_QPI				(2)
#define	SPI_BUS_OPI				(3)

#define SPI_SET_SPI_BUS_TYPE(cmd, add, mod, dat)			\
		(((dat)<<24) | ((mod)<<16) | ((add)<<8) | ((cmd)<<0))

#define	SPI_SET_ON_UPDATE(mode, mask, con,aon,mon,don)		\
		( (mode & (~(0x08888&((mask)<<3)))) | (((con)&0x01)<<15) | (((aon)&0x01)<<11) | (((mon)&0x01)<<7) | (((don)&0x01)<<3) )

// DMA Configuration
#define SPI_DMA_MP0_BST(x)		(((x) & 0x0F) << 12)
#define SPI_DMA_MP0_IDLE(x)		(((x) & 0x0F) << 8)

#define SPI_DMA_MP0_HSIZE(x)	(((x) & 0x03) << 4)
#define SPI_XHSIZE_BYTE			(0)
#define SPI_XHSIZE_HWORD		(1)
#define SPI_XHSIZE_DWORD		(2)

#define SPI_DMA_MP0_AI(x)		((x) << 3)
#define	SPI_ADDR_FIX			(0)
#define	SPI_ADDR_INCR			(1)


//---------------------------------------------------------
//	HAL :: Structure
//---------------------------------------------------------

struct	__spi_handler__
{
	UINT32			dev_unit;  // Unique Address
	// Device-dependant
	UINT32			instance;
	UINT8			debug;
	UINT8			dsize;
	UINT8			xfermode;
	UINT32 			maxlength;

	// Register Map
	SPI_COMMON_TYPE		*reg_cmn;
	SPI_SPI_DOMAIN_TYPE	*reg_spi;
	SPI_DMA_DOMAIN_TYPE	*reg_dma;
	SPI_XIP_DOMAIN_TYPE	*reg_xip;

	UINT32			concat;
	UINT32			cselinfo;
	UINT32			cselbkup;
	UINT32			coreclock;
	UINT32			busctrl[2];
	UINT32			spiclock;
	VOID			*workbuf;
	EventGroupHandle_t event;

	SemaphoreHandle_t buslock;
	VOID			*pllinfo;

	UINT8			*dlytab;
	UINT8			dlyidx;
};

//---------------------------------------------------------
//	HAL :: APP-Interface
//---------------------------------------------------------

/**
 ****************************************************************************************
 * @brief This function create SPI master handler.
 * @param[in] dev_id Instance Number of SPI
 * @return Handler of SPI Driver
 * 
 ****************************************************************************************
 */
extern HANDLE	SPI_CREATE( UINT32 dev_id );

/**
 ****************************************************************************************
 * @brief This function initializes the SPI Handler to set up GPIO and activate the ISR.
 * @param[in] handler Handler of SPI Driver
 * @return TRUE / FALSE
 * 
 ****************************************************************************************
 */
extern int	SPI_INIT(HANDLE handler);

/**
 ****************************************************************************************
 * @brief This function configures the SPI master driver.
 * @param[in] handler Handler of SPI Driver
 * @param[in] cmd refer to the below information
 *   - SPI_SET_SPEED
 *     * set the target SPI clock
 *   - SPI_GET_SPEED
 *     * get the current value of SPI clock
 *   - SPI_SET_FORMAT
 *     * set the SPI interface mode
 *       - SPI_TYPE_MOTOROLA_O0H0
 *       - SPI_TYPE_MOTOROLA_O1H1
 *   - SPI_SET_DMAMODE
 *     * set the DMA transfer mode to the DMA mode
 *   - SPI_GET_MAX_LENGTH
 *     * the maximum burst size
 *   - SPI_SET_MAX_LENGTH
 *     * set the maximum burst size (up to 63 KB)
 *   - SPI_SET_CALLACK
 *     * set the user defined callbacks.
 *       - SPI_INTIDX_RORINT: the receive overrun interrupt
 *       - SPI_INTIDX_RTMINT: the receive timeout interrupt
 *       - SPI_INTIDX_RXINT: when there are four or more data in the RX FIFO
 *       - SPI_INTIDX_TXINT: when there are four or less data in the TX FIFO
 *   - SPI_SET_CONCAT
 *     * set the SPI burst mode to the concatenation mode
 *   - SPI_SET_BUSCONTROL
 *     * set the SPU bus access mode
 *   - SPI_GET_BUSCONTROL
 *     * get the current value of SPI bus access mode
 *   - SPI_GET_DELAYSEL
 *     * get the parameters of current delay model
 *   - SPI_SET_LOCK
 *     * lock/unlock the mutex of SPI driver
 * @param[in] data IOCTL parameters
 * @return TRUE / FALSE
 * 
 ****************************************************************************************
 */
extern int	SPI_IOCTL(HANDLE handler, UINT32 cmd , VOID *data);

/**
 ****************************************************************************************
 * @brief This function write bytes to the SPI master.
 * @param[in] handler handler of SPI driver
 * @param[in] pdata: pointer to the tx buffer
 * @param[in] dlen data length
 * @return zero - false, non-zero - data length
 * 
 ****************************************************************************************
 */
extern int	SPI_WRITE(HANDLE handler, void *pdata, UINT32 dlen);

/**
 ****************************************************************************************
 * @brief This function read bytes from the SPI master.
 * @param[in] handler handler of SPI driver
 * @param[in] pdata: pointer to the rx buffer
 * @param[in] dlen data length
 * @return zero - false, non-zero - data length
 * 
 ****************************************************************************************
 */
extern int	SPI_READ(HANDLE handler, void *pdata, UINT32 dlen);

/**
 ****************************************************************************************
 * @brief This function write and read bytes from the SPI master.
 * @param[in] handler handler of SPI driver
 * @param[in] snddata:: pointer to the write data buffer
 * @param[in] sndlen: data length
 * @param[in] rcvdata:: pointer to the read data buffer
 * @param[in] rcvlen: data length
 * @return zero - false, non-zero - data length
 * 
 ****************************************************************************************
 */
extern int	SPI_WRITE_READ(HANDLE handler
					, void *snddata, UINT32 sndlen
					, void *rcvdata, UINT32 rcvlen );

/**
 ****************************************************************************************
 * @brief Basic operation running once in SPI burst mode(send before receive). This function does not support to change a bus mode automatically
 * @param[in] handler handler of SPI driver
 * @param[in] snddata:: pointer to the write data buffer
 * @param[in] sndlen: data length
 * @param[in] rcvdata:: pointer to the read data buffer
 * @param[in] rcvlen: data length
 * @return zero - false, non-zero - data length
 * 
 ****************************************************************************************
 */
extern int	SPI_TRANSMIT(HANDLE handler, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen );

/**
 ****************************************************************************************
 * @brief Send a SPI transaction to serial flash
 * @param[in] HANDLE handler: Device handle obtained using SPI_CREATE
 * @param[in] cmd: Command to send the flash device
 * @param[in] addr: Address to send the flash device
 * @param[in] mode: Optional control bits that follow the address
 * @param[in] snddata: Send data buffer
 * @param[in] sndlen: Send data length
 * @param[in] rcvdata: Receive data buffer
 * @param[in] rcvlen: Receive data length
 * @return zero - false, non-zero - data length
 ****************************************************************************************
 */
extern int	SPI_SFLASH_TRANSMIT(HANDLE handler
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

/**
 ****************************************************************************************
 * @brief This function release the SPI handler.
 * @param[in] handler Handler of SPI Driver
 * @return TRUE / FALSE
 * 
 ****************************************************************************************
 */
extern int	SPI_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	HAL :: DEVICE-Interface
//---------------------------------------------------------

extern void	SPI_SYS_INIT(UINT32 mode);
extern void	SPI_SYS_DEINIT(void);
extern HANDLE	SPI_SYS_GET_HANDLER(void);

//---------------------------------------------------------
//	HAL :: ISR
//---------------------------------------------------------

#endif /* __spi_h__ */
