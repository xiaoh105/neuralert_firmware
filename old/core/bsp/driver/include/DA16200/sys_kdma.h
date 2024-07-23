/**
 ****************************************************************************************
 *
 * @file sys_kdma.h
 *
 * @brief KDMA
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


#ifndef __sys_kdma_h__
#define __sys_kdma_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"

#ifdef	SUPPORT_KDMA
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
#define	CH_DMA_PERI_UART2_RX		(13)
#define	CH_DMA_PERI_UART2_TX		(14)
#define	CH_DMA_PERI_MAX 			(15)

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

#define KDMA_CYCLE_CTRL_INVALID        (0x0)
#define KDMA_CYCLE_CTRL_BASIC          (0x1)
#define KDMA_CYCLE_CTRL_AUTOREQ        (0x2)
#define KDMA_CYCLE_CTRL_PINGPONG       (0x3)
#define KDMA_CYCLE_CTRL_MEMSCAT_PRIM   (0x4)
#define KDMA_CYCLE_CTRL_MEMSCAT_ALT    (0x5)
#define KDMA_CYCLE_CTRL_PERISCAT_PRIM  (0x6)
#define KDMA_CYCLE_CTRL_PERISCAT_ALT   (0x7)

#define DMA_TRANSFER_WIDTH_BYTE       (0x0)
#define DMA_TRANSFER_WIDTH_HALFWORD   (0x1)
#define DMA_TRANSFER_WIDTH_WORD       (0x2)

#define DMA_TRANSFER_INC_BYTE         (0x0)
#define DMA_TRANSFER_INC_HALFWORD     (0x1)
#define DMA_TRANSFER_INC_WORD         (0x2)
#define DMA_TRANSFER_NO_INCREMENT     (0x3)

#define DMA_CONFIG_MEM2MEM            (0x1)
#define DMA_CONFIG_MEM2PERI           (0x0)


#define SYS_DMA_INT_EVT (1<<0)
#define SYS_DMA_MAX_WAIT_TIME (30)

#define SYS_DMA_CONTROL_BASE_ADDR       (0x20010000)

                              /* Maximum to 32 DMA channel */
#define MAX_NUM_OF_DMA_CHANNELS   16
                              /* SRAM in example system is 64K bytes */
#define RAM_ADDRESS_MAX       0x200FFFFF

#define ERROR_DMA_FULL (1)

#define DMA_CHCTRL_DWIDTH(x)   ((x)<<1)
#define DMA_CHCTRL_SWIDTH(x)   ((x)<<1)

#define DMA_CHCTRL_DINCR(x)    ((x)<<16)
#define DMA_CHCTRL_SINCR(x)    ((x)<<14)

#define DMA_CHCTRL_BURSTLENGTH(x)    ((x)<<18) // burst length <=  2^x

#define MEMCPY_DMA_MINIMUM_SIZE 1024


#define	KDMA_DESC_ARB_DONE_POS		(21)//The number of completed arbitration periods//Initially set to 0
#define	KDMA_DESC_ARB_PERIOD_POS	(18)//Arbitration period, arbitrate after every 2^(arb_period) transfers//0 to 7
#define	KDMA_DESC_DST_ADDR_INC_POS	(16)//Destination address increment at every DMA transfer//0x0: 1, 0x1: 2, 0x2: 4, 0x3: fixed address
#define	KDMA_DESC_SRC_ADDR_INC_POS	(14)//Source address increment at every DMA transfer//0x0: 1, 0x1: 2, 0x2: 4, 0x3: fixed address
#define	KDMA_DESC_TF_LEN_POS		(3)//Length of DMA transfers in this DMA task [byte] (rounddown)// 1 to 2047 (0: not allowed)
#define	KDMA_DESC_TF_SIZE_POS		(1)//The read/write size of each DMA transfer//0x0: byte, 0x1: hword, 0x2: word, 0x3: not allowed
#define	KDMA_DESC_TF_REQ_MODE_POS	(0)//Burst / basic mode//0: basic mode, 1: burst mode

typedef	enum	{
	DMA_CB_TERM  = 0,
    DMA_CB_EMPTY ,
    DMA_CB_ERROR ,
	DMA_CB_MAX

} DMA_CALLBACK_LIST;

//typedef 	VOID	(*USR_CALLBACK )(VOID *);


typedef struct /* 4 words */
{
	volatile unsigned int	command;
	volatile unsigned int	src_start_addr;
	volatile unsigned int	dst_start_addr;
	volatile unsigned int	next_descriptor;
} kdma_channel_data;


typedef struct /* 8 words per channel */
{ /* only one channel in the example uDMA setup */
  volatile kdma_channel_data Primary[32];//jason150331 HW fixed Block force to use #32
} kdma_data_structure;
typedef struct /* 4 words */
{
	volatile unsigned int	dma_enable;
	volatile unsigned int	dma_reset;
	volatile unsigned int	cfg_descriptor_addr;
	volatile unsigned int	cfg_channel_enable;
	volatile unsigned int	cfg_irq_done_type;
	volatile unsigned int	sw_request;
	volatile unsigned int	irq_done_chs;
	volatile unsigned int	irq_done_clr;
	volatile unsigned int	irq_err_chs;
	volatile unsigned int	irq_err_clr;
	volatile unsigned int	status_dma;
	volatile unsigned int	status_desc_addr;
	volatile unsigned int	status_counter;
	volatile unsigned int	status_descriptor;
	volatile unsigned int	status_desc_addr_pre;
}	KDMA_REGMAP;

typedef struct	__dma_handler__
{
	UINT32				dev_addr;  // Unique Address
	// Device-dependant
	UINT32				dmachan_id;
	UINT32				dma_maxlli;

    volatile kdma_channel_data *Primary;
    UINT32  control;
    UINT32  config;

	USR_CALLBACK		callback[DMA_CB_MAX];
	VOID				*cb_param[DMA_CB_MAX];

	// Register Map
	VOID				*mutex;
	KDMA_REGMAP		*regmap;
}DMA_HANDLE_TYPE;


/******************************************************************************
 *
 *  Declarations
 *
 ******************************************************************************/

void	_sys_dma_init( void );
void	_sys_dma_deinit(void);
int	    _sys_kdma_open( unsigned int ch );
int	    _sys_kdma_close( unsigned int ch );
int	    _sys_kdma_wait( unsigned int ch, unsigned int wait_time );
void	_sys_kdma_set_control( unsigned int ch, unsigned int size, unsigned int num_word, unsigned int cycle_ctrl, unsigned int r_pow,void *desc );
void	_sys_kdma_set_src_addr( unsigned int ch, unsigned int src, unsigned int size, unsigned int num_word, void *desc );
void    _sys_kdma_set_dst_addr( unsigned int ch, unsigned int dst, unsigned int size, unsigned int num_word, void *desc );
void	_sys_kdma_set_next( unsigned int ch, unsigned int next );
void	_sys_kdma_sw_request( unsigned int ch);
int     memcpy_dma (void *dest, void *src, unsigned int len, unsigned int wait_time);
int     SYS_DMA_COPY(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len);
int 	SYS_DMA_LINK(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len, unsigned int enable);
HANDLE  SYS_DMA_OBTAIN_CHANNEL(unsigned int dmachan_id, unsigned int dma_control, unsigned int dma_config, unsigned int dma_maxlli );
int     SYS_DMA_RELEASE_CHANNEL(HANDLE hdl )  ;
int     SYS_DMA_REGISTRY_CALLBACK( HANDLE hdl, USR_CALLBACK term,  USR_CALLBACK empty, USR_CALLBACK error  , HANDLE dev );

int     SYS_DMA_RESET_CH(HANDLE hdl);//jason151105

void	_sys_kdma_interrupt(void);
void    _sys_kdma_interrupt_handler(void);

kdma_channel_data*    _sys_dma_get_last_descriptor(unsigned int ch);//jason160530

#endif	//SUPPORT_UDMA

#endif	/*__sys_dma_h__*/
