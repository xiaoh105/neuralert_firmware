/**
 * \addtogroup HALayer
 * \{
 * \addtogroup I2S
 * \{
 * \brief Intergrated Interchip Sound
 */
 
/**
 ****************************************************************************************
 *
 * @file sys_i2s.h
 *
 * @brief I2S
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


#ifndef __i2s_h__
#define __i2s_h__

#include <stdio.h>
#include <stdlib.h>

#include "hal.h"
//#include "typedef.h"
//#include "comdef.h"

//#include "fc3860_reg_map.h"

//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
typedef 	struct __i2s_handler__	I2S_HANDLER_TYPE; 	// deferred
typedef 	struct __i2s_regmap__ 	I2S_REG_MAP;		// deferred

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
//
//	Equipment Type
//


/**
 * \brief I2S Device ID
 */
typedef enum	{
	I2S_0	= 0, /**< -*/
	I2S_MAX
} I2S_LIST;

//
//	CALLBACK List
//

/**
 * \brief I2S Callback Type
 */
typedef	enum	{
	I2S_CB_TERM  = 0,
	I2S_CB_EMPTY ,
	I2S_CB_MAX

} I2S_CALLBACK_LIST;

//
//	IOCTL Commands
//
#define	I2S_DMA_HISR_DONE	(0x00000001)

/**
 * \brief I2S IO Control List
 */
typedef enum	{
	I2S_SET_DEBUG = 0,
	I2S_SET_CONFIG ,
	I2S_GET_CONFIG ,
	I2S_SET_CR1,
	I2S_GET_CR1,
	I2S_SET_FS ,
	I2S_GET_FS ,
	I2S_SET_DMA,
	I2S_GET_DMA,
	I2S_SET_DMA_RX,
	I2S_GET_DMA_RX,
	I2S_GET_STATUS,
	I2S_SET_MAXSIZE,
	I2S_SET_RESET,
	I2S_SET_HISR_CALLACK,
	I2S_SET_MONO,
	I2S_SET_STEREO,
	I2S_SET_PCM_RESOLUTION,
	I2S_SET_FIFO_RESET,
	I2S_SET_BIG_ENDIAN,
	I2S_SET_LITTLE_ENDIAN,
	I2S_SET_LCLK_FIRST,
	I2S_SET_RCLK_FIRST,

	I2S_SET_I2S_MODE,
	I2S_SET_LEFT_JUSTIFIED,

	I2S_SET_MAX
} I2S_IOCTL_LIST;

//---------------------------------------------------------
//	DRIVER :: Bit Field Definition
//---------------------------------------------------------

//
//	I2S Control Register
//
#define 	I2S_CFG_96KHz		((0x3)<<12)
#define		I2S_CFG_48KHz		((0x3)<<12)
#define		I2S_CFG_44KHz		((0x3)<<12)
#define		I2S_CFG_32KHz		((0x3)<<12)
#define 	I2S_CFG_24KHz		((0x3)<<12)
#define 	I2S_CFG_22KHz		((0x3)<<12)
#define 	I2S_CFG_16KHz		((0x3)<<12)

#define		I2S_CFG_STEREO		((0x1)<<11)
#define		I2S_CFG_MONO		((0x0)<<11)

#define		I2S_CFG_PCM_8		((0x0)<<9)
#define 	I2S_CFG_PCM_16		((0x1)<<9)
#define 	I2S_CFG_PCM_20		((0x2)<<9)
#define 	I2S_CFG_PCM_24		((0x3)<<9)

#define		I2S_CFG_PCM_RX_8	((0x0)<<9)
#define 	I2S_CFG_PCM_RX_16	((0x1)<<9)
#define 	I2S_CFG_PCM_RX_24	((0x2)<<9)
#define 	I2S_CFG_PCM_RX_32	((0x3)<<9)

#define		I2S_CFG_MUTE		((0x1)<<8)

#define		I2S_CFG_SCLK_FALL	((0x1)<<7)
#define 	I2S_CFG_SCLK_RISE	((0x0)<<7)

#define 	I2S_CFG_PCMM_LEFT	((0x0)<<5)
#define 	I2S_CFG_PCMM_RIGHT	((0x1)<<5)
#define 	I2S_CFG_PCMM_I2S	((0x2)<<5)

#define 	I2S_CFG_BIG			((0x1)<<4)
#define 	I2S_CFG_LITTLE		((0x0)<<4)

#define 	I2S_CFG_MCLK_FALL	((0x1)<<3)
#define 	I2S_CFG_MCLK_RISE	((0x0)<<3)

#define 	I2S_CFG_LRCK_HIGH	((0x1)<<2)
#define 	I2S_CFG_LRCK_LOW 	((0x0)<<2)

#define 	I2S_CFG_BLK_ENABLE	((0x1)<<1)
#define 	I2S_CFG_BLK_DISABLE	((0x0)<<1)

#define 	I2S_CFG_POWER_DOWN	((0x1)<<0)
#define 	I2S_CFG_NORMAL_MODE ((0x0)<<0)

//
//	I2S Status Register
//
#define         I2S_DAC_TX_EN           ((0x0)<<3)
#define         I2S_DAC_RX_EN           ((0x1)<<3)
#define         I2S_CR1_RESET_FIFO      ((0x1)<<7)


#define         I2S_DAC_RX_W16          ((0x2)<<4)
#define         I2S_DAC_RX_W24          ((0x1)<<4)
#define         I2S_DAC_RX_W32          ((0x0)<<4)
#define         I2S_DAC_RX_CH_R         ((0x1)<<1)
#define         I2S_DAC_RX_CH_L         ((0x0)<<1)
//
//	I2S Status Register
//

#define 	I2S_STT_RX_NOT_EMPTY 	((0x1)<<5)
#define 	I2S_STT_RX_FULL 	((0x1)<<4)
#define		I2S_STT_BUSY		((0x1)<<2)
#define 	I2S_STT_TX_NOTFULL	((0x1)<<1)
#define 	I2S_STT_TX_EMPTY	((0x1)<<0)


//
//	I2S	ATTN Register
//

#define		I2S_ATTN_VOL(x)		(((x)&0x3FF)<<3)
#define 	I2S_ATTN_LATCH		((0x1)<<2)
#define 	I2S_ATTN_VOL_L		((0x1)<<1)
#define 	I2S_ATTN_VOL_R		((0x0)<<1)
#define 	I2S_ATTN_CS			((0x0)<<1)

//
//	I2S DMA Register
//

#define 	I2S_DMA_TX_EN		((0x1)<<1)
#define 	I2S_DMA_RX_EN		((0x1)<<0)

//
//	DMA Limitation

#define	I2S_TRANSFER_ALIGN		36


#define SYS_I2S_BASE_0  (DA16200_I2S_BASE)


#define HW_REG_WRITE32_I2S(addr,value)  (addr) = (value)
#define HW_REG_READ32_I2S(addr)   ((addr))
#define HW_REG_AND_WRITE32_I2S(addr,value)  (addr) = ((addr)&(value))
#define HW_REG_OR_WRITE32_I2S(addr,value) (addr) = ((addr)|(value))

#define  DMA_CH_I2S_TX (2)
#define  DMA_CH_I2S_RX (1)
#define  DMA_MAX_LLI_NUM (10)

#define DRIVER_DBG_NONE DRV_PRINTF
#define DRIVER_DBG_ERROR DRV_PRINTF

#define DMA_TRANSFER_SIZE   (32)

#define DMA_LLI_MODE_SUPPORT // DMA Linked List support


//typedef 	VOID	(*USR_CALLBACK )(VOID *);

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------

/**
 * \brief I2S Register MAP
 */
struct	__i2s_regmap__
{
	volatile UINT32	ctrl_reg;		// 0x00
	volatile UINT32	ctrl_reg1;		// 0x04
	volatile UINT32	i2s_data;		// 0x08
	volatile UINT32	i2s_sr;			// 0x0C
	volatile UINT32  _reserved[5];	// 0x10
	volatile UINT32	dma_en;			// 0x24
};


/**
 * \brief I2S Handle Structure
 */
struct	__i2s_handler__
{
	UINT32				dev_addr;  // Unique Address
	// Device-dependant
	UINT32				dev_unit;
	UINT32				instance;
	UINT32				i2sclock;
	UINT32				sample_freq;

	UINT32				dmachan_id_tx;
	HANDLE				dmachannel_tx;
	UINT32				dmachan_id_rx;
	HANDLE				dmachannel_rx;
	UINT32				dma_maxlli;

	USR_CALLBACK		callback[I2S_CB_MAX];
	VOID				*cb_param[I2S_CB_MAX];

	// Register Map
	I2S_REG_MAP			*regmap;

	VOID				*mutex;
};

typedef struct
{
	void *src_addr;
	unsigned int length;
	void *next;
} i2s_descriptor_block_t;

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create I2S Handle
 *
 * This function will create I2S handle
 *
 * \param [in] dev_id I2S controller ID \parblock
 *      - ::I2S_0
 * \endparblock
 *
 * \return HANDLE for adc access
 *
 */
extern HANDLE	DRV_I2S_CREATE( UINT32 dev_id );

/**
 * \brief Initialize I2S 
 *
 * This function will init I2S 
 *
 * \param [in] handler handle for intialization I2S block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2S_INIT (HANDLE handler, unsigned int mode);

/**
 * \brief misc. control for I2S  block 
 *
 * This function will control miscellaneous I2S  controller
 *
 * \param [in] handler handle for control I2S  block
 * \param [in] cmd Command
 * \param [in,out] Buffer Pointer to Write or Read
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2S_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief I2S Write Function 
 *
 * This function will write data wirh I2S bus format
 *
 * \param [in] handler handle for write I2S block
 * \param [in] p_data buffer to write
 * \param [in] p_dlen length to write
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_I2S_WRITE(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 dummy);

/**
 * \brief I2S Read Function 
 *
 * This function will read data wirh I2S bus format
 *
 * \param [in] handler handle for write I2S block
 * \param [in] p_data buffer to read
 * \param [in] p_dlen length to read
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2S_READ(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 dummy);

/**
 * \brief  Close I2S Handle
 *
 * \param [in] handler handle for Close I2S block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2S_CLOSE(HANDLE handler);
extern int	DRV_I2S_WAIT_INTERRUPT(HANDLE handler, UNSIGNED *mask_evt);

/**
 * \brief I2S Clock Setting FUnction 
 *
 * This function will set I2C MCLK
 *
 * \param [in] handler handle for write I2S block
 * \param [in] clk_src buffer to read \parblock
 *  - 0 : Clock Source from External PAD. eg. External 24.576MHz crystal
 *  - 1 : Clock Source from Internal PLL
 *  \endparblock
 * \param [in] clk_div clock divider :
 *              - internal clock mode : Fs = 48MHz / (clk_div+1)
 *              - external clock mode : Fs = Crystal Input / (clk_div+1)
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_I2S_SET_CLOCK(HANDLE handler, UNSIGNED clk_src, UNSIGNED clk_div);


//---------------------------------------------------------
//	DRIVER :: DEVICE-Interface
//---------------------------------------------------------


//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------



//---------------------------------------------------------
//	DRIVER :: Utility
//---------------------------------------------------------


#define I2S_DEBUG_ON

#ifdef I2S_DEBUG_ON
#define PRINT_I2S DRV_PRINTF
#else
#define PRINT_I2S
#endif

#endif /* __i2s_h__ */

/**
 * \}
 * \}
 */


