/**
 * \addtogroup HALayer
 * \{
 * \addtogroup XFC		QSPI Controller
 * \{
 * \brief QSPI Flash controller to access a serial flash memory.
 */

/**
 ****************************************************************************************
 *
 * @file xfc.h
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


#ifndef __xfc_h__
#define __xfc_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------

typedef 	struct __xfc_handler__		XFC_HANDLER_TYPE; 	// deferred
typedef 	struct __xfc_common__		XFC_COMMON_TYPE; 	// deferred
typedef 	struct __xfc_spi_domain__	XFC_SPI_DOMAIN_TYPE; 	// deferred
typedef 	struct __xfc_dma_domain__	XFC_DMA_DOMAIN_TYPE; 	// deferred
typedef 	struct __xfc_xip_domain__	XFC_XIP_DOMAIN_TYPE; 	// deferred

//---------------------------------------------------------
//	HAL :: Commands & Parameters
//---------------------------------------------------------
/**
 * \brief XFC ID
 */
typedef enum	{
	XFC_UNIT_0	= 0,		/**< XFC */
	XFC_UNIT_MAX
} XFC_UNIT_LIST;

/**
 * \brief IOCTL Commands
 */
typedef enum	{
	XFC_SET_DEBUG = 0,		/**< Not used **/

	XFC_SET_SPEED	,		/**< Set SPI Interface Clock (data[0] : target freq. Hz) */
	XFC_GET_SPEED  ,		/**< Get SPI Interface Clock (data[0] : target freq. Hz) */
	XFC_GET_MAX_LENGTH,		/**< Not used **/
	XFC_SET_MAX_LENGTH,		/**< Not used **/
	XFC_SET_FORMAT  ,		/**< Set the polarity and phase of SPI Interface (data[0] : format) */
	XFC_SET_DATASIZE,		/**< Not used **/

	XFC_SET_INTRMODE,		/**< Set the Interrupt mode to access a flash device. Do not use this mode in the cache boot. */
	XFC_SET_DMAMODE,		/**< Set the DMA mode to access a flash device. Do not use this mode in the cache boot. */
	XFC_GET_XFERMODE,		/**< Get current access mode (data[0] : return data) */

	XFC_SET_CALLACK,		/**< Not used **/	/* 10 */

	// additional cmd
	XFC_CHECK_BUSY,			/**< Not used **/
	XFC_CHECK_RX_FULL,		/**< Not used **/
	XFC_CHECK_RX_NO_EMPTY,	/**< Not used **/
	XFC_CHECK_TX_NO_FULL,	/**< Not used **/
	XFC_CHECK_TX_EMPTY,		/**< Not used **/

	XFC_SET_SLAVEMODE,		/**< Not used **/
	XFC_SET_OUTEN,			/**< Not used **/
	XFC_SET_CONCAT,			/**< Preset a concatenation mode for handling multiple operations in a single transactions. (data[0] : TRUE/FALSE) */

	XFC_GET_BUSCONTROL,		/**< Get current bus access mode. (data[0] : byte format, data[1] : lane format) */
	XFC_SET_BUSCONTROL,		/**< Set the bus access mode. (data[0] : byte format, data[1] : lane format) */ /*20 */
	XFC_GET_DELAYSEL,		/**< Get current delay parameters. (data[0] : list of parameters) */
	XFC_SET_DELAYSEL,		/**< Set the delay parameters. (data[0] : list of parameters) */	

	XFC_SET_LOCK,			/**< Lock/Unlock the SPI Bus. (data[0] : Lock/Unlock, data[1] : suspend mode, data[2] : CSEL number) */

	XFC_SET_CORECLOCK,		/**< Preset the XFC Core Clock (data[0] : current freq. Hz) */
	XFC_SET_TIMEOUT,		/**< Set the CS Deselect time, tSHSL. (data[0] : unit, data[1] : count) */
	XFC_SET_WIRE,			/**< Set the SPI Wire Model. (data[0] : csel id, data[1] : 3 or 4 wire) */
	XFC_SET_DMA_CFG,		/**< Set the XFC DMA Cofiguration (data[0]: config data) */
	XFC_SET_PINCONTROL,		/**< Set the WP & HOLD Pin mode.  (TRUE : tri-state, FALSE : output mode.) */

	XFC_SET_XIPMODE,		/**< Configure XIP registers with the current value of DMA reigsters. */

	XFC_SET_MAX				/* 30 */
} XFC_IOCTL_LIST;

/**
 * \brief CSEL list
 */
typedef enum	{
	XFC_CSEL_0	= 0,		/**< External serial flash. */
	XFC_CSEL_1,				/**< Internal/stacked serial flash. */
	XFC_CSEL_2,				/**< Not used **/
	XFC_CSEL_3,				/**< Not used **/

	XFC_CSEL_MAX
} XFC_CSEL_LIST;

/**
 * \brief Clock Polarity & Phase
 */
typedef enum	{
	XFC_TYPE_MOTOROLA_O0H0 = 0,		/**< SPI mode 0, (CPOL=0, CPHA=0) */
	XFC_TYPE_MOTOROLA_O0H1,			/**< SPI mode 1, (CPOL=0, CPHA=1) */
	XFC_TYPE_MOTOROLA_O1H0,			/**< SPI mode 2, (CPOL=1, CPHA=0) */
	XFC_TYPE_MOTOROLA_O1H1,			/**< SPI mode 3, (CPOL=1, CPHA=1) */

	XFC_TYPE_MAX
} XFC_FORMAT_TYPE_LIST;

//---------------------------------------------------------
//	HAL :: Offset
//---------------------------------------------------------

struct	__xfc_common__
{
	volatile UINT32 XFC_OP_EN;
	volatile UINT32 XFC_REQ_START;
	volatile UINT32 XFC_REQ_CLR_IRQ;
	volatile UINT32 XFC_REQ_CLR_OP;
	volatile UINT32 XFC_IRQ_STATUS;
	volatile UINT32 IRQ_STATUS_DMA;
	volatile UINT32 IRQ_STATUS_SPI;
	volatile UINT32 IRQ_STATUS_XIP;

	volatile UINT32 XFC_CORE_RATIO;
	volatile UINT32 XFC_CORE_FREQ;
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

	volatile UINT32 XFC_SPI_DLY_EN;
	volatile UINT32 _reserved6;
	volatile UINT32 _reserved7;
	volatile UINT32 _reserved8;

	volatile UINT32 XFC_SPI_DLY_0_TXC;
	volatile UINT32 XFC_SPI_DLY_0_RXC;
	volatile UINT32 _reserved9;
	volatile UINT32 _reserved10;

	volatile UINT32 XFC_SPI_DLY_1_TXC;
	volatile UINT32 XFC_SPI_DLY_1_RXC;
	volatile UINT32 _reserved11;
	volatile UINT32 _reserved12;

	volatile UINT32 XFC_SPI_DLY_2_TXC;
	volatile UINT32 XFC_SPI_DLY_2_RXC;
	volatile UINT32 _reserved13;
	volatile UINT32 _reserved14;

	volatile UINT32 XFC_SPI_DLY_3_TXC;
	volatile UINT32 XFC_SPI_DLY_3_RXC;
};

struct	__xfc_spi_domain__
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
};

struct	__xfc_dma_domain__
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

struct	__xfc_xip_domain__
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
/**
 * \brief BUS CONTROL [0]  - Enable CSEL Delsect timeout
 */
#define XFC_SPI_TIMEOUT_EN		(1<<4)
/**
 * \brief BUS CONTROL [0]  - mode phase, 8 bit
 */
#define XFC_SPI_PHASE_MODE_8BIT		((0)<<2)
/**
 * \brief BUS CONTROL [0]  - mode phase, 4 bit
 */
#define XFC_SPI_PHASE_MODE_4BIT		((1)<<2)
/**
 * \brief BUS CONTROL [0]  - mode phase, 2 bit
 */
#define XFC_SPI_PHASE_MODE_2BIT		((2)<<2)
/**
 * \brief BUS CONTROL [0]  - mode phase, 1 bit
 */
#define XFC_SPI_PHASE_MODE_1BIT		((3)<<2)
/**
 * \brief BUS CONTROL [0]  - command phase, 1 byte
 */
#define XFC_SPI_PHASE_CMD_1BYTE		((0)<<1)
/**
 * \brief BUS CONTROL [0]  - command phase, 2 byte
 */
#define XFC_SPI_PHASE_CMD_2BYTE		((1)<<1)
/**
 * \brief BUS CONTROL [0]  - address phase, 3 byte
 */
#define XFC_SPI_PHASE_ADDR_3BYTE	((0)<<0)
/**
 * \brief BUS CONTROL [0]  - address phase, 5 byte
 */
#define XFC_SPI_PHASE_ADDR_4BYTE	((1)<<0)

#define	XFC_SPI_PHASE_DUMMY_MASK	((0x1f)<<16)
/**
 * \brief BUS CONTROL [0]  - set the dummy cycle
 */
#define	XFC_SET_SPI_DUMMY_CYCLE(x)	(((x)&0x1f)<<16)
/**
 * \brief BUS CONTROL [0]  - get the dummy cycle
 */
#define	XFC_GET_SPI_DUMMY_CYCLE(x)	(((x)>>16)&0x1f)

/**
 * \brief BUS CONTROL [1]  - tuple of lane (ON/OFF, 0, line info) 
 */
#define XFC_SPI_BUS_TYPE(on,dtr,bits)	(((on)<<3)|((dtr)<<2)|(bits))
/**
 * \brief BUS CONTROL [1]  - line info, single line
 */
#define	XFC_BUS_SPI			(0)
/**
 * \brief BUS CONTROL [1]  - line info, dual lines
 */
#define	XFC_BUS_DPI			(1)
/**
 * \brief BUS CONTROL [1]  - line info, Quad lines
 */
#define	XFC_BUS_QPI			(2)
#define	XFC_BUS_OPI			(3)
/**
 * \brief BUS CONTROL [1]  - lane format (tuple of CMD, ADDR, MODE and DATA)
 */
#define XFC_SET_SPI_BUS_TYPE(cmd, add, mod, dat) (((dat)<<24)|((mod)<<16)|((add)<<8)|((cmd)<<0))

#define	XFC_SET_ON_UPDATE(mode, mask, con,aon,mon,don)		\
		(  (mode & (~(0x08888&((mask)<<3))))				\
		 | (((con)&0x01)<<15)|(((aon)&0x01)<<11)			\
		 | (((mon)&0x01)<<7)|(((don)&0x01)<<3)  )

/**
 * \brief DMA Configuration  - burst length
 */
#define XFC_DMA_MP0_BST(x)		(((x)&0x0F)<<12)
/**
 * \brief DMA Configuration  - idle cycle
 */
#define XFC_DMA_MP0_IDLE(x)		(((x)&0x0F)<< 8)

/**
 * \brief DMA Configuration  - bus width 
 */
#define XFC_DMA_MP0_HSIZE(x)		(((x)&0x03)<< 4)
/**
 * \brief DMA Configuration  - 1-byte bus width
 */
#define XHSIZE_BYTE			(0)
/**
 * \brief DMA Configuration  - 2-byte bus width
 */
#define XHSIZE_HWORD			(1)
/**
 * \brief DMA Configuration  - 4-byte bus width
 */
#define XHSIZE_DWORD			(2)

/**
 * \brief DMA Configuration  - address increment mode
 */
#define XFC_DMA_MP0_AI(x)		((x)<<3)
#define	XFC_ADDR_FIX			(0)
#define	XFC_ADDR_INCR			(1)

//---------------------------------------------------------
//	HAL :: Structure
//---------------------------------------------------------

/**
 * \brief Structure of XFC Driver
 */
struct	__xfc_handler__
{
	UINT32			dev_unit;  		/**< Physical address for identification */
	// Device-dependant
	UINT32			instance;		/**< Number of times that a driver with the same ID is created */
	UINT8			debug;			/**< internal field */
	UINT8			dsize;			/**< internal field */
	UINT8			xfermode;		/**< current operation mode */
	UINT32 			maxlength;		/**< internal field */	

	// Register Map
	XFC_COMMON_TYPE		*reg_cmn;	/**< common dommain of registers */
	XFC_SPI_DOMAIN_TYPE	*reg_spi;	/**< SPI domain of registers */
	XFC_DMA_DOMAIN_TYPE	*reg_dma;	/**< DMA domain of registers */
	XFC_XIP_DOMAIN_TYPE	*reg_xip;	/**< XIP domain of registers */
	
	UINT32			concat;			/**< concatenation flag */
	UINT32			cselinfo;		/**< CSEL info */		
	UINT32			cselbkup;		/**< internal field */	
	UINT32			coreclock;		/**< XFC Core clock */
	UINT32			busctrl[2];		/**< bus control data */
	UINT32			spiclock;		/**< SPI interface clock */
	VOID			*workbuf;		/**< internal field */
	VOID			*event;			/**< internal field */

	UINT			buslock;		/**< mutex for dirver */	/* chg by_sjlee for OS_INDEPENDENCY */
	VOID			*pllinfo;		/**< PLL callback structure */

	UINT8			*dlytab;		/**< Delay parameters */
};

//---------------------------------------------------------
//	HAL :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create XFC driver
 *
 * \param [in] dev_type		id
 *
 * \return	HANDLE			pointer to the driver instance
 *
 */
extern HANDLE	XFC_CREATE( UINT32 dev_id );

/**
 * \brief Initialize XFC driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	XFC_INIT(HANDLE handler);

/**
 * \brief Call for device specific operations
 *
 * \param [in] handler		driver handler
 * \param [in] cmd			IOCTL command
 * \param [inout] data		command specific data array
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	XFC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data);

/**
 * \brief SPI style Transmit Function
 *
 * \param [in] handler		driver handler
 * \param [in] snddata		TX data buffer
 * \param [in] sndlen		TX length
 * \param [out] rcvdata		RX data buffer
 * \param [in] rcvlen		RX length 
 *
 * \return	Non-Zero,		total transmit length
 * \return	Zero, 			function failed 
 *
 */
extern int	XFC_TRANSMIT(HANDLE handler, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen );

/**
 * \brief SFLASH style Transmit Function
 *
 * \param [in] handler		driver handler
 * \param [in] cmd			command data
 * \param [in] addr			address data
 * \param [in] mode			mode data
 * \param [in] snddata		write data
 * \param [in] sndlen		WR length
 * \param [out] rcvdata		read data 
 * \param [in] rcvlen		RD length
 *
 * \return	Non-Zero,		total transmit length
 * \return	Zero, 			function failed 
 *
 */
extern int	XFC_SFLASH_TRANSMIT(HANDLE handler
				, UINT32 cmd, UINT32 addr, UINT32 mode
				, VOID *snddata, UINT32 sndlen
				, VOID *rcvdata, UINT32 rcvlen );

/**
 * \brief Close XFC driver
 *
 * \param [in] handler		driver handler
 *
 * \return	TRUE,			function succeeded
 * \return	FALSE, 			function failed 
 *
 */
extern int	XFC_CLOSE(HANDLE handler);

//---------------------------------------------------------
//	HAL :: DEVICE-Interface
//---------------------------------------------------------

/**
 * \brief Batch function to initialize XFC at boot
 *
 * \param [in] mode			running mode
 *
 */
extern void	XFC_SYS_INIT(UINT32 mode);

/**
 * \brief Batch function to de-initialize XFC
 *
 */
extern void	XFC_SYS_DEINIT(void);

/**
 * \brief Get Current XFC handler
 *
 * \return	driver handler
 *
 */
extern HANDLE	XFC_SYS_GET_HANDLER(void);

//---------------------------------------------------------
//	HAL :: ISR
//---------------------------------------------------------


#endif /* __xfc_h__ */
/**
 * \}
 * \}
 */
