/**
 * \addtogroup HALayer
 * \{
 * \addtogroup I2C
 * \{
 * \brief Inter Integrated Circuit controller
 */
 
/**
 ****************************************************************************************
 *
 * @file sys_i2c.h
 *
 * @brief I2C
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


#ifndef __i2c_h__
#define __i2c_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
typedef 	struct __i2c_handler__	I2C_HANDLER_TYPE; 	// deferred
typedef 	struct __i2c_regmap__ 	I2C_REG_MAP;		// deferred

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
//
//	Equipment Type
//

/**
 * \brief I2C Device ID
 */
typedef enum	{
	i2c_0	= 0, /**< -*/
	I2C_MAX
} I2C_LIST;

//
//	CALLBACK List
//

/**
 * \brief I2C Callback ID
 */
typedef	enum	{
	I2C_CB_TERM  = 0,
	I2C_CB_EMPTY ,
	I2C_CB_MAX

} I2C_CALLBACK_LIST;

//
//	IOCTL Commands
//

/**
 * \brief I2C IO Control List
 */
typedef enum	{
	I2C_SET_DEBUG = 0,
	I2C_SET_CONFIG ,
	I2C_GET_CONFIG ,
	I2C_SET_DAC,
	I2C_GET_DAC,
	I2C_SET_FS ,
	I2C_GET_FS ,
	I2C_SET_ATTN,
	I2C_GET_ATTN,
	I2C_SET_DMA_WR,
	I2C_SET_DMA_RD,
	I2C_GET_DMA_WR,
	I2C_GET_DMA_RD,
	I2C_GET_STATUS,
	I2C_SET_MAXSIZE,
	I2C_SET_RESET,
	I2C_SET_HISR_CALLACK,
	I2C_SET_CHIPADDR    ,
	I2C_GET_CHIPADDR    ,
	I2C_SET_CLOCK    ,
	I2C_SET_CLK_DAT_DELAY,

	I2C_SET_MAX
} I2C_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: Bit Field Definition
//---------------------------------------------------------

//
//	i2c Control Register
//
#define 	I2C_CTRL_DATA8		((0x7)<<0)
#define		I2C_CTRL_STEP2		((0x1)<<4)
#define		I2C_CTRL_SOD		((0x1)<<3)
#define		I2C_CTRL_MASTER		((0x0)<<2)
#define		I2C_CTRL_SLAVE		((0x1)<<2)
#define 	I2C_CTRL_ENABLE		((0x1)<<1)
#define 	I2C_CTRL_LOOPBACK	((0x1)<<0)

//
//	I2C Status Register
//
#define		I2C_STATUS_BUSY		((0x1)<<15)
#define		I2C_STATUS_WR_DONE  ((0x1)<<5)
#define		I2C_STATUS_RD_DONE  ((0x1)<<4)
#define		I2C_STATUS_RFF		((0x1)<<3)
#define		I2C_STATUS_RNE		((0x1)<<2)
#define		I2C_STATUS_TNF		((0x1)<<1)
#define		I2C_STATUS_TFE		((0x1)<<0)

#define		I2C_MASK_WR_DONE	((0x1)<<6)
#define		I2C_MASK_RD_DONE	((0x1)<<5)
#define		I2C_MASK_NACK		((0x1)<<4)
#define		I2C_MASK_TXIM		((0x1)<<3)
#define		I2C_MASK_RXIM		((0x1)<<2)
#define		I2C_MASK_RTIM		((0x1)<<1)
#define		I2C_MASK_ROIM		((0x1)<<0)

#define		I2C_INT_WR_DONE		((0x1)<<6)
#define		I2C_INT_RD_DONE		((0x1)<<5)
#define		I2C_INT_NACK		((0x1)<<4)
#define		I2C_INT_TXRIS		((0x1)<<3)
#define		I2C_INT_RXRIS		((0x1)<<2)
#define		I2C_INT_RTRIS		((0x1)<<1)
#define		I2C_INT_RORRIS		((0x1)<<0)

#define		I2C_MASKED_TXMIS	((0x1)<<3)
#define		I2C_MASKED_RXMIS	((0x1)<<2)
#define		I2C_MASKED_RTMIS	((0x1)<<1)
#define		I2C_MASKED_RORMIS	((0x1)<<0)

#define		I2C_INTCLR_RTIC	    ((0x1)<<1)
#define		I2C_INTCLR_RORIC	((0x1)<<0)

#define		I2C_DMA_TXEN	    ((0x1)<<1)
#define		I2C_DMA_RXEN    	((0x1)<<0)

#define		I2C_BUS_RDACC	    ((0x1)<<3)
#define		I2C_BUS_STOPEN	    ((0x1)<<4)
#define		I2C_BUS_ADRSZ    	((0x1)<<0)

#define 	I2C_CFG_BLK_ENABLE	((0x1)<<1)
#define 	I2C_CFG_BLK_DISABLE	((0x0)<<1)

#define 	I2C_CFG_POWER_DOWN	((0x1)<<0)
#define 	I2C_CFG_NORMAL_MODE ((0x0)<<0)


#define     I2C_STATUS_NACK     ((0x1)<<16) //internal event only not for register


#define SYS_I2C_BASE_0  (I2C_BASE_0)


#define HW_REG_WRITE32_I2C(addr,value)  (addr) = (value)
#define HW_REG_READ32_I2C(addr)   ((addr))
#define HW_REG_AND_WRITE32_I2C(addr,value)  (addr) = ((addr)&(value))
#define HW_REG_OR_WRITE32_I2C(addr,value) (addr) = ((addr)|(value))

#define  DMA_CH_I2C_WRITE (4)
#define  DMA_CH_I2C_READ (3)
#define  DMA_MAX_LLI_NUM (10)

#define DMA_TRANSFER_SIZE   (32)

#define DMA_LLI_MODE_SUPPORT // DMA Linked List support


//typedef 	VOID	(*USR_CALLBACK )(VOID *);

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------

/**
 * \brief I2C Register MAP
 */
struct	__i2c_regmap__
{
	volatile UINT32	i2c_cr0;		// 0x00
	volatile UINT32	i2c_cr1;		// 0x04
	volatile UINT32	i2c_dr;		// 0x08
	volatile UINT32	i2c_sr;			// 0x0C
	volatile UINT32	i2c_cpsr;		// 0x10
	volatile UINT32	i2c_msc;		// 0x14
	volatile UINT32	i2c_ris;		// 0x18
	volatile UINT32	i2c_mis;		// 0x1c
	volatile UINT32	i2c_icr;		// 0x20
	volatile UINT32	i2c_dmacr;			// 0x24
	volatile UINT32	i2c_buscr;			// 0x28
	volatile UINT32	i2c_rdlen;			// 0x2c
	volatile UINT32	i2c_drvid;			// 0x30
	volatile UINT32	i2c_en;			// 0x34
	volatile UINT32 i2c_clkdat_delay; 		// 0x38
};


/**
 * \brief I2C Handle Structure
 */
struct	__i2c_handler__
{
	UINT32				dev_addr;  // Unique Address
	// Device-dependant
	UINT32				dev_unit;
	UINT32				instance;
	UINT32				i2cclock;

	UINT32				dmachan_id_wr;
	UINT32				dmachan_id_rd;
	HANDLE				dmachannel_wr;
	HANDLE				dmachannel_rd;
	HANDLE				g_dmachannel_tx;
	HANDLE				g_dmachannel_rx;
	UINT32				dma_maxlli;

	USR_CALLBACK		callback[I2C_CB_MAX];
	VOID				*cb_param[I2C_CB_MAX];

	// Register Map
	I2C_REG_MAP			*regmap;

	VOID				*mutex;
};

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create I2C Handle
 *
 * This function will create I2C handle
 *
 * \param [in] dev_id I2C controller ID \parblock
 *      - ::i2c_0
 * \endparblock
 *
 * \return HANDLE for adc access
 *
 */
extern HANDLE	DRV_I2C_CREATE( UINT32 dev_id );

/**
 * \brief Initialize I2C 
 *
 * This function will init I2C 
 *
 * \param [in] handler handle for intialization I2C block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2C_INIT (HANDLE handler);

/**
 * \brief misc. control for I2C block 
 *
 * This function will control miscellaneous I2C controller
 *
 * \param [in] handler handle for control I2C block
 * \param [in] cmd Command
 * \param [in,out] Buffer Pointer to Write or Read
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2C_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief I2C Write Function 
 *
 * This function will write data to slave device
 *
 * \param [in] handler handle for control I2C block
 * \param [in] p_data buffer to write
 * \param [in] p_dlen length to write
 * \param [in] stopen Make stop condition end of write function \parblock
 *  - 1 : enable stop condition after write sequence
 *  - 0 : disable stop condition after write sequence
 *  \endparblock
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_I2C_WRITE(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 stopen, UINT32 dummy);

/**
 * \brief I2C Read Function 
 *
 * This function will read data from slave device
 *
 * \param [in] handler handle for control I2C block
 * \param [in] p_data buffer to read
 * \param [in] p_dlen length to reade
 * \param [in] addr_len length of register or memory address to write
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_I2C_READ(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 addr_len,UINT32 dummy);

/**
 * \brief  Close I2C Handle
 *
 * \param [in] handler handle for Close I2C block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2C_CLOSE(HANDLE handler);
void	i2c_drv_interrupt(void);
void i2c_drv_interrupt_handler(void);
//static void i2c_pll_callback(UINT32 clock, void *param);		/* warning del by_sjlee */

//---------------------------------------------------------
//	DRIVER :: DEVICE-Interface
//---------------------------------------------------------


//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------



//---------------------------------------------------------
//	DRIVER :: Utility
//---------------------------------------------------------

#define I2C_DEBUG_ON

#endif /* __i2c_h__ */

/**
 * \}
 * \}
 */

