/**
 ****************************************************************************************
 *
 * @file sys_dma.h
 *
 * @brief CMSDK PL230 UDMA
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


#ifndef __sys_dma_h__
#define __sys_dma_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

#ifdef	SUPPORT_UDMA
//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

#define	CH_DMA_PERI_I2S		0
#define	CH_DMA_PERI_QSPI0_RX		(1)
#define	CH_DMA_PERI_QSPI0_TX		(2)
#define	CH_DMA_PERI_I2CM_RX		(3)
#define	CH_DMA_PERI_I2CM_TX		(4)
#define	CH_DMA_PERI_UART0_RX		(5)
#define	CH_DMA_PERI_UART0_TX		(6)
#define	CH_DMA_PERI_UART1_RX		(7)
#define	CH_DMA_PERI_UART1_TX		(8)
#define	CH_DMA_PERI_ADC_FIFO0	(9)
#define	CH_DMA_PERI_ADC_FIFO1	(10)
#define	CH_DMA_PERI_ADC_FIFO2	(11)
#define	CH_DMA_PERI_ADC_FIFO3	(12)
#define	CH_DMA_PERI_MAX		(13)

#define DMA_CH_0                  ( 0)
#define DMA_CH_1                  ( 1)
#define DMA_CH_2                  ( 2)
#define DMA_CH_3                  ( 3)
#define DMA_CH_4                  ( 4)
#define DMA_CH_5                  ( 5)
#define DMA_CH_6                  ( 6)
#define DMA_CH_7                  ( 7)
#define DMA_CH_8                  ( 8)
#define DMA_CH_9                  ( 9)
#define DMA_CH_10                 ( 10)
#define DMA_CH_11                 ( 11)
#define DMA_CH_12                 ( 12)
#define DMA_CH_13                 ( 13)
#define DMA_CH_14                 ( 14)
#define DMA_CH_15                 ( 15)
#define DMA_CH_16                 ( 16)
#define DMA_CH_17                 ( 17)
#define DMA_CH_18                 ( 18)
#define DMA_CH_19                 ( 19)
#define DMA_CH_20                 ( 20)
#define DMA_CH_21                 ( 21)
#define DMA_CH_22                 ( 22)
#define DMA_CH_23                 ( 23)
#define DMA_CH_24                 ( 24)
#define DMA_CH_25                 ( 25)
#define DMA_CH_26                 ( 26)
#define DMA_CH_27                 ( 27)
#define DMA_CH_28                 ( 28)
#define DMA_CH_29                 ( 29)
#define DMA_CH_30                 ( 30)
#define DMA_CH_31                 ( 31)

#define PL230_CYCLE_CTRL_INVALID        (0x0)
#define PL230_CYCLE_CTRL_BASIC          (0x1)
#define PL230_CYCLE_CTRL_AUTOREQ        (0x2)
#define PL230_CYCLE_CTRL_PINGPONG       (0x3)
#define PL230_CYCLE_CTRL_MEMSCAT_PRIM   (0x4)
#define PL230_CYCLE_CTRL_MEMSCAT_ALT    (0x5)
#define PL230_CYCLE_CTRL_PERISCAT_PRIM  (0x6)
#define PL230_CYCLE_CTRL_PERISCAT_ALT   (0x7)

#define DMA_TRANSFER_WIDTH_BYTE       (0x0)
#define DMA_TRANSFER_WIDTH_HALFWORD   (0x1)
#define DMA_TRANSFER_WIDTH_WORD       (0x2)

#define DMA_TRANSFER_INC_BYTE         (0x0)
#define DMA_TRANSFER_INC_HALFWORD     (0x1)
#define DMA_TRANSFER_INC_WORD         (0x2)
#define DMA_TRANSFER_NO_INCREMENT     (0x3)

#define DMA_CONFIG_MEM2MEM            (0x1)


#define SYS_DMA_INT_EVT (1<<0)
#define SYS_DMA_MAX_WAIT_TIME (30)

#define SYS_DMA_CONTROL_BASE_ADDR       (0x20010000)

                              /* Maximum to 32 DMA channel */
#define MAX_NUM_OF_DMA_CHANNELS   16
                              /* SRAM in example system is 64K bytes */
#define RAM_ADDRESS_MAX       0x200FFFFF

#define ERROR_DMA_FULL (1)

#define DMA_CHCTRL_DWIDTH(x)    (((x)<<28)|((x)<<21))
#define DMA_CHCTRL_SWIDTH(x)    (((x)<<24)|((x)<<18))

#define DMA_CHCTRL_DINCR(x)    ((x)<<30)
#define DMA_CHCTRL_SINCR(x)    ((x)<<26)

#define DMA_CHCTRL_BURSTLENGTH(x)    ((x)<<14) // burst length <=  2^x

#define MEMCPY_DMA_MINIMUM_SIZE 1024


typedef	enum	{
	DMA_CB_TERM  = 0,
    DMA_CB_EMPTY ,
    DMA_CB_ERROR ,
	DMA_CB_MAX

} DMA_CALLBACK_LIST;

//typedef 	VOID	(*USR_CALLBACK )(VOID *);


typedef struct /* 4 words */
{
  volatile unsigned long SrcEndPointer;
  volatile unsigned long DestEndPointer;
  volatile unsigned long Control;
  volatile unsigned long unused;
} pl230_dma_channel_data;


typedef struct /* 8 words per channel */
{ /* only one channel in the example uDMA setup */
  volatile pl230_dma_channel_data Primary[32];//jason150331 HW fixed Block force to use #32
  volatile pl230_dma_channel_data Alternate[32];//jason150331 HW fixed Block force to use #32
} pl230_dma_data_structure;

typedef struct	__dma_handler__
{
	UINT32				dev_addr;  // Unique Address
	// Device-dependant
	UINT32				dmachan_id;
	UINT32				dma_maxlli;

    volatile pl230_dma_channel_data *Primary;
    volatile pl230_dma_channel_data *Alternate;
    UINT32  control;
    UINT32  config;

	USR_CALLBACK		callback[DMA_CB_MAX];
	VOID				*cb_param[DMA_CB_MAX];

	// Register Map
	VOID				*mutex;
}DMA_HANDLE_TYPE;

/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

void	_sys_dma_init( void );
void	_sys_dma_deinit(void);
int	    _sys_dma_open( unsigned int ch );
int	    _sys_dma_close( unsigned int ch );
int	    _sys_dma_wait( unsigned int ch, unsigned int wait_time );
void	_sys_dma_set_control( unsigned int ch, unsigned int size, unsigned int num_word, unsigned int cycle_ctrl, unsigned int r_pow,void *desc );
void	_sys_dma_set_src_addr( unsigned int ch, unsigned int src, unsigned int size, unsigned int num_word, void *desc );
void    _sys_dma_set_dst_addr( unsigned int ch, unsigned int dst, unsigned int size, unsigned int num_word, void *desc );
void	_sys_dma_set_next( unsigned int ch, unsigned int next );
void	_sys_dma_sw_request( unsigned int ch);
int     memcpy_dma (void *dest, void *src, unsigned int len, unsigned int wait_time);
int     SYS_DMA_COPY(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len);
int 	SYS_DMA_LINK(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len, unsigned int enable);
HANDLE  SYS_DMA_OBTAIN_CHANNEL(unsigned int dmachan_id, unsigned int dma_control, unsigned int dma_config, unsigned int dma_maxlli );
int     SYS_DMA_RELEASE_CHANNEL(HANDLE hdl )  ;
int     SYS_DMA_REGISTRY_CALLBACK( HANDLE hdl, USR_CALLBACK term,  USR_CALLBACK empty, USR_CALLBACK error  , HANDLE dev );

int     SYS_DMA_RESET_CH(HANDLE hdl);//jason151105

void	_sys_dma_interrupt(void);
void    _sys_dma_interrupt_handler(void);

pl230_dma_channel_data*    _sys_dma_get_last_descriptor(unsigned int ch);//jason160530

#endif	//SUPPORT_UDMA

#endif	/*__sys_dma_h__*/
