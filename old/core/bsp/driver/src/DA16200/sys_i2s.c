/**
 ****************************************************************************************
 *
 * @file sys_i2s.c
 *
 * @brief 
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
#include <stdlib.h>

#include "oal.h"
#include "hal.h"

#include "driver.h"
#include "sys_dma.h"
#include "sys_i2s.h"

//---------------------------------------------------------

#undef	DAC_INIT_ISSUE_20120117

//---------------------------------------------------------
//
//---------------------------------------------------------


static void drv_i2s_term(void *param);
static void drv_i2s_empty(void *param);
static void drv_i2s_error(void *param);
static int	drv_i2s_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param);

//static void drv_i2s_pll_setup(UINT32 freq);
//static void drv_i2s_pll_down(void);

static void	drv_i2s_ctrl_down(I2S_HANDLER_TYPE *i2s);
static void i2s_pll_callback(UINT32 clock, void *param);
//static void i2s_set_downsync(UINT32 divider);

//static void i2s_drv_thread(unsigned long thread_input);

//---------------------------------------------------------
//
//---------------------------------------------------------


#define	DAC_SAMPLE_LIMIT		(0x1FF)
#define	DAC_SAMPLE_ALIGN(x)		((x) & (~DAC_SAMPLE_LIMIT) )



#define DMA_SCAT_LEN    (4096)

#define I2S_DMA_DESC_BLOCK_SIZE_KB      (128)
#define I2S_DMA_DESC_BLOCK_NUMBER       (2)

//---------------------------------------------------------
//
//---------------------------------------------------------

static int	_i2s_instance[1];
// Semaphore
static EventGroupHandle_t	_mutex_i2s_instance[1];

#define	I2S_INSTANCE		0	//i2s->dev_unit

//---------------------------------------------------------
//
//---------------------------------------------------------

static UINT8	drv_i2s_flag = FALSE;
//---------------------------------------------------------
//
//---------------------------------------------------------
#ifdef I2S_THREAD_ENABLE
i2s_descriptor_block_t i2s_desc_head;
#endif

static CLOCK_LIST_TYPE			i2s_pll_info;
unsigned int	g_archband_phy_occupied = 0;

#ifdef I2S_THREAD_ENABLE
OAL_THREAD_TYPE i2s_drv_thread_type;
unsigned int    i2s_drv_thread_stack[512];
unsigned int    i2s_drv_thread_stacksize = 512*sizeof(unsigned int);
#endif

I2S_HANDLER_TYPE	*g_drv_i2s;



HANDLE	DRV_I2S_CREATE( UINT32 dev_id )
{
	I2S_HANDLER_TYPE	*i2s;

	// Allocate
	if((i2s = (I2S_HANDLER_TYPE *) DRIVER_MALLOC(sizeof(I2S_HANDLER_TYPE)) ) == NULL){
		return NULL;
	}

	// Clear
	DRIVER_MEMSET(i2s, 0, sizeof(I2S_HANDLER_TYPE));

	// Address Mapping
	switch((I2S_LIST)dev_id){
		case	I2S_0:
				i2s->dev_addr = SYS_I2S_BASE_0;
				i2s->instance = (UINT32)(_i2s_instance[I2S_INSTANCE]);
				_i2s_instance[I2S_INSTANCE] ++;
				i2s->mutex = &(_mutex_i2s_instance[I2S_INSTANCE]);
				break;

		default:
				DRIVER_DBG_ERROR("ilegal unit, %d\n", dev_id);
				DRIVER_FREE(i2s);
				return NULL;
	}

	// Set Default Para
	i2s->regmap   = (I2S_REG_MAP *)(i2s->dev_addr);
	i2s->dev_unit = dev_id ;
	i2s->i2sclock = 0x01FF;
	i2s->dmachan_id_tx = DMA_CH_I2S_TX ;
	i2s->dmachan_id_rx = DMA_CH_I2S_RX ;
	i2s->dma_maxlli = DMA_MAX_LLI_NUM ;
	i2s->sample_freq = 48000 ;	// Hz


	g_drv_i2s = (I2S_HANDLER_TYPE*)i2s;
	
	PRINT_I2S("(%p) I2S Driver create, base %p\n", (UINT32 *)i2s, (UINT32 *)i2s->dev_addr );
	return (HANDLE) i2s;
}







int	DRV_I2S_INIT (HANDLE handler, unsigned int mode)
{
	I2S_HANDLER_TYPE	*i2s;
	UINT32	dma_control, dma_config = 0;
	unsigned int	reg_value;

	ASIC_DBG_TRIGGER(0x12500011);
	if(handler == NULL){
		return FALSE;
	}

	DA16X_CLOCK_SCGATE->Off_DAPB_I2S = 0;
	DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;
//	ASIC_DBG_TRIGGER(0x12500012);
//jason180212	DA16200_CLKGATE_ON(clkoff_AuxAdc);
//jason180212	DA16200_CLKGATE_ON(clkoff_iis);

//	ASIC_DBG_TRIGGER(0x12500013);
//	OAL_MSLEEP(3000);
//LDO for Fraction-N & IP3
//	RTC_IOCTL(RTC_GET_LDO_CONTROL_REG, &reg_value);
//	reg_value = reg_value | 0x140;//fraction-N
//	reg_value = reg_value | LDO_IP3_ENABLE(1)| FRACTIONAL_N_PLL_ENABLE(1);//jason170213
//	RTC_IOCTL(RTC_SET_LDO_CONTROL_REG, &reg_value);
//	OAL_MSLEEP(3000);
//ASIC_DBG_TRIGGER(0x12500014);

//Clock for Fraction-N
//	FC9000_BOOTCON->CLK_EN |= (0x040);//i2s jason170213
//jason180218	DA16200_BOOTCON->CLK_EN |= (0x1<<CLKEN_I2S);//i2s jason170213
//	FC9000_FRAC_PLL_CONTROL(1, 49152);
//jason180218	DA16200_EXTPHY->iis_clk_ctrl = 0x1; // 1 : 1/2 , 2: 1/4, 3: 1/8
//	OAL_MSLEEP(3000);
//	ASIC_DBG_TRIGGER(0x12500015);



	i2s = (I2S_HANDLER_TYPE *)handler ;

	// Normal Mode
	if(i2s->instance == 0){
		HW_REG_AND_WRITE32_I2S(	i2s->regmap->ctrl_reg, (~I2S_CFG_POWER_DOWN) );

	// DMA Tx
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD) ;
		dma_control |= DMA_CHCTRL_BURSTLENGTH(4);

		i2s->dmachannel_tx = SYS_DMA_OBTAIN_CHANNEL(i2s->dmachan_id_tx, dma_control, dma_config, i2s->dma_maxlli );
		if(i2s->dmachannel_tx != NULL){
			SYS_DMA_REGISTRY_CALLBACK( i2s->dmachannel_tx, drv_i2s_term,  drv_i2s_empty, drv_i2s_error  , (HANDLE) i2s );
			}
	// DMA Rx
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD) ;
		dma_control |= DMA_CHCTRL_BURSTLENGTH(4);

		i2s->dmachannel_rx = SYS_DMA_OBTAIN_CHANNEL(i2s->dmachan_id_rx, dma_control, dma_config, i2s->dma_maxlli );
		if(i2s->dmachannel_rx != NULL){
			SYS_DMA_REGISTRY_CALLBACK( i2s->dmachannel_rx, drv_i2s_term,  drv_i2s_empty, drv_i2s_error  , (HANDLE) i2s );
			
		}
//		OAL_MSLEEP(3000);
ASIC_DBG_TRIGGER(0x12500016);

        DRV_I2S_SET_CLOCK(i2s, 1, 0);

		// Post-Action
		// I2S Initialization start ....
		if( drv_i2s_flag == FALSE ){
			if(mode)//RX Mode
			{
				DRV_I2S_IOCTL(handler,I2S_GET_CR1,&reg_value);
				reg_value |= I2S_DAC_RX_EN;
				reg_value |= (I2S_CR1_RESET_FIFO);
				DRV_I2S_IOCTL(handler,I2S_SET_CR1,&reg_value);
				HW_REG_WRITE32_I2S( i2s->regmap->ctrl_reg,
					(I2S_CFG_48KHz|I2S_CFG_MONO|I2S_CFG_PCM_RX_24|I2S_CFG_SCLK_FALL|I2S_CFG_PCMM_I2S|I2S_CFG_LITTLE|I2S_CFG_MCLK_FALL|I2S_CFG_BLK_ENABLE) );
			}
			else
			{
				DRV_I2S_IOCTL(handler,I2S_GET_CR1,&reg_value);
				reg_value &= ~(I2S_DAC_RX_EN);
				reg_value |= (I2S_CR1_RESET_FIFO);
				DRV_I2S_IOCTL(handler,I2S_SET_CR1,&reg_value);
				HW_REG_WRITE32_I2S( i2s->regmap->ctrl_reg,
					(I2S_CFG_48KHz|I2S_CFG_STEREO|I2S_CFG_PCM_16|I2S_CFG_SCLK_FALL|I2S_CFG_PCMM_I2S|I2S_CFG_LITTLE|I2S_CFG_MCLK_FALL|I2S_CFG_BLK_ENABLE) );

			}
			
			HW_REG_WRITE32_I2S( i2s->regmap->dma_en,  0 ) ;
			drv_i2s_flag = TRUE ;
		}

		ASIC_DBG_TRIGGER(0x12500017);
//		OAL_MSLEEP(3000);

		// I2S Initialization finished ....
	}

	// Semaphore
    i2s->mutex = (VOID *)xEventGroupCreate();
#ifdef I2S_THREAD_ENABLE

    i2s_desc_head.length = 0;
    i2s_desc_head.src_addr = NULL;
    i2s_desc_head.next = NULL;

	
    OAL_CREATE_THREAD( &i2s_drv_thread_type, "@i2sdrv", i2s_drv_thread, (unsigned int)handler,
           i2s_drv_thread_stack, i2s_drv_thread_stacksize, 2, _NO_TIME_SLICE, 
           1, _AUTO_START ); 
#endif    
	
//pll callback
	i2s_pll_info.callback.func = i2s_pll_callback;
	i2s_pll_info.callback.param = i2s;
	i2s_pll_info.next = NULL;
	_sys_clock_ioctl(SYSCLK_SET_CALLACK, &(i2s_pll_info));
	ASIC_DBG_TRIGGER(0x12500018);
	
//	OAL_MSLEEP(3000);
	return TRUE;
}


int	DRV_I2S_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	I2S_HANDLER_TYPE	*i2s;

	if(handler == NULL){
		return FALSE;
	}
	i2s = (I2S_HANDLER_TYPE *)handler ;

	switch(cmd){
		case I2S_SET_CONFIG:
                        HW_REG_WRITE32_I2S( i2s->regmap->ctrl_reg, *((UINT32 *)data) );
			break;
		case I2S_GET_CONFIG:
			*((UINT32 *)data) =  HW_REG_READ32_I2S( i2s->regmap->ctrl_reg );
			break;

		case I2S_SET_CR1:
			HW_REG_WRITE32_I2S( i2s->regmap->ctrl_reg1, *((UINT32 *)data) );
			break;
		case I2S_GET_CR1:
			*((UINT32 *)data) =  HW_REG_READ32_I2S( i2s->regmap->ctrl_reg1 );
			break;

		case I2S_SET_FS:
			i2s->sample_freq = *((UINT32 *)data) ;
			DA16200_SYSCLOCK->CLK_EN_I2S = 0x0;
			DA16200_SYSCLOCK->PLL_CLK_EN_5_I2S = 0x0;
			DA16200_SYSCLOCK->PLL_CLK_DIV_5_I2S = *((UINT32 *)data);
//			DA16200_SYSCLOCK->CLK_DIV_I2S = *data;
			DA16200_SYSCLOCK->SRC_CLK_SEL_5_I2S = 0x1;
			DA16200_SYSCLOCK->PLL_CLK_EN_5_I2S = 0x1;
	//		*(volatile unsigned char *)(0x50003096) = 1;
			DA16200_SYSCLOCK->CLK_EN_AUXA = 0x1;
			break;

		case I2S_GET_FS:
			*((UINT32 *)data) = i2s->sample_freq ;
			break;

		case I2S_GET_STATUS:
			if( data != NULL ){
				*((UINT32 *)data) = HW_REG_READ32_I2S( i2s->regmap->i2s_sr );
			}else{
				while( (HW_REG_READ32_I2S(i2s->regmap->i2s_sr) & I2S_STT_TX_EMPTY) != I2S_STT_TX_EMPTY ){
					//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), (I2S_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
					//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), ~(I2S_DMA_HISR_DONE) , OAL_AND );								// Clear
                    vTaskDelay(1);
				}
			}
			break;

		case I2S_SET_DMA:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_OR_WRITE32_I2S( i2s->regmap->dma_en, I2S_DMA_TX_EN ) ;
			}else{
				HW_REG_AND_WRITE32_I2S( i2s->regmap->dma_en, (~I2S_DMA_TX_EN) ) ;
			}
			break;

		case I2S_SET_DMA_RX:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_OR_WRITE32_I2S( i2s->regmap->dma_en, I2S_DMA_RX_EN ) ;
			}else{
				HW_REG_AND_WRITE32_I2S( i2s->regmap->dma_en, (~I2S_DMA_RX_EN) ) ;
			}
			break;

		case I2S_GET_DMA:
			*((UINT32 *)data) = HW_REG_READ32_I2S(i2s->regmap->dma_en);
			break;

		case I2S_GET_DMA_RX:
			*((UINT32 *)data) = HW_REG_READ32_I2S(i2s->regmap->dma_en);
			break;

		case I2S_SET_MAXSIZE:
//			i2s->dma_maxlli = ( ( (*((UINT32 *)data)) / DAC_SAMPLE_ALIGN(DMA_TRANSFER_SIZE) ) * 1 ) + 1 ;
			break;

		case I2S_SET_RESET:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_WRITE32_I2S( i2s->regmap->dma_en,  0 ) ;
			}else{
				HW_REG_AND_WRITE32_I2S(	i2s->regmap->ctrl_reg, (~I2S_CFG_POWER_DOWN) );


			}
			break;

		case I2S_SET_HISR_CALLACK:
			drv_i2s_callback_registry( i2s, ((UINT32 *)data)[0], ((UINT32 *)data)[1], ((UINT32 *)data)[2] );
			break;
		case I2S_SET_MONO:
			i2s->regmap->ctrl_reg = (i2s->regmap->ctrl_reg & ~(I2S_CFG_STEREO) );
			break;

		case I2S_SET_STEREO:
			i2s->regmap->ctrl_reg = (i2s->regmap->ctrl_reg | I2S_CFG_STEREO );
			break;
			
		case I2S_SET_PCM_RESOLUTION:
			i2s->regmap->ctrl_reg &= ~(0x600);
			i2s->regmap->ctrl_reg |= *((unsigned int*)(data))<<9;
			break;
				
		case I2S_SET_FIFO_RESET:
			i2s->regmap->ctrl_reg1 &= ~(0x80);
			i2s->regmap->ctrl_reg1 |= (0x80);
			break;
					
		case I2S_SET_BIG_ENDIAN:
			i2s->regmap->ctrl_reg |= (0x10);
			break;
								
		case I2S_SET_LITTLE_ENDIAN:
			i2s->regmap->ctrl_reg &= ~(0x10);
			break;
					
		case I2S_SET_LCLK_FIRST:
			i2s->regmap->ctrl_reg1 &= ~(0x1);
			break;
					
		case I2S_SET_RCLK_FIRST:
			i2s->regmap->ctrl_reg1 |= (0x1);
			break;
											
		case I2S_SET_I2S_MODE:
			i2s->regmap->ctrl_reg1 &= ~(0x10);
			break;
		
		case I2S_SET_LEFT_JUSTIFIED:
			i2s->regmap->ctrl_reg1 |= (0x10);
			break;
																													


		default:
			break;
	}
	return TRUE;
}

int	DRV_I2S_WRITE(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	I2S_HANDLER_TYPE	*i2s;
	UINT32  scat_len ;
	VOID   *scat_offset ;
//	UNSIGNED masked_evt ;
	int   status;
    unsigned int    dma_en;

	if(handler == NULL){
		return FALSE;
	}
	i2s = (I2S_HANDLER_TYPE *)handler ;

	if(p_dlen == 0){
		return FALSE;
	}

	scat_offset = p_data ;

//	HW_REG_OR_WRITE32_I2S( i2s->regmap->attn_ctrl, I2S_ATTN_VOL_L );

	do{
        if(p_dlen > DMA_SCAT_LEN)
        {
            scat_len = DMA_SCAT_LEN;
            p_dlen -= DMA_SCAT_LEN;
        }
        else
        {
            scat_len = p_dlen;
            p_dlen = 0;
        }
        
        
		// DMA Action
#ifdef DMA_LLI_MODE_SUPPORT
#ifdef I2S_THREAD_ENABLE
#endif
		status = SYS_DMA_COPY(i2s->dmachannel_tx, (UINT32 *)(&(i2s->regmap->i2s_data)) , scat_offset, scat_len);

		if(status != 0){
			p_dlen = 0;
            continue;
		}
        else
        {
            DRV_I2S_IOCTL(i2s,I2S_GET_DMA, &dma_en);
            if(!dma_en)
            {
                dma_en = TRUE;
                DRV_I2S_IOCTL(i2s,I2S_SET_DMA, &dma_en);
            }
        }
            
#else
#ifdef I2S_THREAD_ENABLE
#endif
		do {
			status = SYS_DMA_COPY(i2s->dmachannel_tx, &(i2s->regmap->i2s_data) , scat_offset, scat_len);
			if(status == TRUE){
				break;
			}else if(status == ERROR_DMA_FULL ){
#ifdef I2S_THREAD_ENABLE
				OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), (I2S_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
#endif
			}else{
				return FALSE;
			}
		}while(1);

#endif

		// Post Action
		scat_offset =  (VOID *)((UINT32)scat_offset + scat_len) ;

	}while(p_dlen > 0);

	return (int)(p_dlen);
}


int	DRV_I2S_READ(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	I2S_HANDLER_TYPE	*i2s;
	UINT32  scat_len ;
	VOID   *scat_offset ;
//	UNSIGNED masked_evt ;
	int   status;
    unsigned int    dma_en;

	if(handler == NULL){
		return FALSE;
	}
	i2s = (I2S_HANDLER_TYPE *)handler ;

	if(p_dlen == 0){
		return FALSE;
	}

	scat_offset = p_data ;

//	HW_REG_OR_WRITE32_I2S( i2s->regmap->attn_ctrl, I2S_ATTN_VOL_L );

	do{
        if(p_dlen > DMA_SCAT_LEN)
        {
            scat_len = DMA_SCAT_LEN;
            p_dlen -= DMA_SCAT_LEN;
        }
        else
        {
            scat_len = p_dlen;
            p_dlen = 0;
        }
        
        
		// DMA Action
#ifdef DMA_LLI_MODE_SUPPORT
#ifdef I2S_THREAD_ENABLE
#endif
		status = SYS_DMA_COPY(i2s->dmachannel_rx, scat_offset, (UINT32 *)(&(i2s->regmap->i2s_data)), scat_len);

		if(status != 0){
			p_dlen = 0;
            continue;
		}
        else
        {
            DRV_I2S_IOCTL(i2s,I2S_GET_DMA, &dma_en);
            if(!dma_en)
            {
                dma_en = TRUE;
                DRV_I2S_IOCTL(i2s,I2S_SET_DMA, &dma_en);
            }
        }
            
#else
#ifdef I2S_THREAD_ENABLE
#endif
		do {
			status = SYS_DMA_COPY(i2s->dmachannel_rx, scat_offset, &(i2s->regmap->i2s_data) , scat_len);
			if(status == TRUE){
				break;
			}else if(status == ERROR_DMA_FULL ){
#ifdef I2S_THREAD_ENABLE
				OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), (I2S_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
#endif
			}else{
				return FALSE;
			}
		}while(1);

#endif

		// Post Action
		scat_offset =  (VOID *)((UINT32)scat_offset + scat_len) ;

	}while(p_dlen > 0);

	return (int)(p_dlen);
}

int	DRV_I2S_CLOSE(HANDLE handler)
{
	I2S_HANDLER_TYPE	*i2s;
	UNSIGNED 			value;

	if(handler == NULL){
		return FALSE;
	}
	i2s = (I2S_HANDLER_TYPE *)handler ;

	if ( _i2s_instance[I2S_INSTANCE] > 0) {
		_i2s_instance[I2S_INSTANCE] -- ;

		if(_i2s_instance[I2S_INSTANCE] == 0) {
			// Wait Until I2S TX-Term
			while( (HW_REG_READ32_I2S(i2s->regmap->i2s_sr) & I2S_STT_BUSY) == I2S_STT_BUSY ){
				//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), (I2S_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
				//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(i2s->mutex), ~(I2S_DMA_HISR_DONE) , OAL_AND );								// Clear
                vTaskDelay(1);
			}
			// Wait Until I2S
			while( (HW_REG_READ32_I2S(i2s->regmap->i2s_sr) & I2S_STT_TX_EMPTY) != I2S_STT_TX_EMPTY ){
                vTaskDelay(1);
			}

			SYS_DMA_RELEASE_CHANNEL(i2s->dmachannel_tx);
			SYS_DMA_RELEASE_CHANNEL(i2s->dmachannel_rx);

			// Delete Semaphore
            vEventGroupDelete((EventGroupHandle_t)(i2s->mutex));

			// DMA disable
			HW_REG_WRITE32_I2S( i2s->regmap->dma_en,  0 ) ;

			drv_i2s_ctrl_down( i2s );

			DRV_I2S_IOCTL(handler, I2S_GET_CONFIG, &value);
			
			value &= ~(I2S_CFG_BLK_ENABLE);//disable
			DRV_I2S_IOCTL(handler, I2S_SET_CONFIG, &value);
			switch((I2S_LIST)i2s->dev_unit){
				case I2S_0:
					// Clock disable
//jason180218		drv_i2s_pll_down();
					// Reset
					 break;
				default:
					break;
			}
		}
	}

	_sys_clock_ioctl(SYSCLK_RESET_CALLACK, &(i2s_pll_info));
	DRIVER_FREE(handler);
	
//jason180212	DA16200_CLKGATE_OFF(clkoff_iis);
//jason180212	DA16200_CLKGATE_OFF(clkoff_AuxAdc);
	
	return TRUE;
}

int	DRV_I2S_WAIT_INTERRUPT(HANDLE handler, UNSIGNED *mask_evt)
{
	DA16X_UNUSED_ARG(handler);
/*
    uxBits = xEventGroupWaitBits(
            xEventGroup,        : The event group being tested.
            BIT_0 | BIT_4,      : The bits within the event group to wait for.
            pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
            pdFALSE,            : Don't wait for both bits, either bit will do.
            xTicksToWait )
*/
    *mask_evt = xEventGroupWaitBits((g_drv_i2s->mutex), 0xffffffff, pdTRUE, pdFALSE, portMAX_DELAY);
    if( !(*mask_evt ) )
	{
		//Todo : Fill Code Here for Managing Abnormal Cases
		return -1;
	}
	
	return 0;
}

int	DRV_I2S_SET_CLOCK(HANDLE handler, UNSIGNED clk_src, UNSIGNED clk_div)
{
	I2S_HANDLER_TYPE	*i2s;

	if(handler == NULL){
		return FALSE;
	}
	
	i2s = (I2S_HANDLER_TYPE *)handler ;
	i2s->sample_freq = *((UINT32 *)clk_div) ;
	DA16200_SYSCLOCK->CLK_EN_I2S = 0x0;
	DA16200_SYSCLOCK->PLL_CLK_EN_5_I2S = 0x0;
	DA16200_SYSCLOCK->PLL_CLK_DIV_5_I2S = 19;
	DA16200_SYSCLOCK->CLK_DIV_I2S = clk_div;
	DA16200_SYSCLOCK->SRC_CLK_SEL_5_I2S = clk_src;
	DA16200_SYSCLOCK->PLL_CLK_EN_5_I2S = 0x1;
	DA16200_SYSCLOCK->CLK_EN_I2S = 0x1;

	if(clk_div == 0)
	{
		DA16200_SYSCLOCK->PLL_CLK_DIV_5_I2S = DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU;
		DA16200_SYSCLOCK->CLK_DIV_I2S = clk_div;
		DA16200_SYSCLOCK->PLL_CLK_DIV_5_I2S = 19;

	}
	return 0;
}


#ifdef I2S_THREAD_ENABLE

void i2s_drv_thread(unsigned long thread_input)
{
	I2S_HANDLER_TYPE	*i2s;
	UNSIGNED masked_evt ;
	
	if(thread_input == NULL){
		return;
	}
    
	i2s = (I2S_HANDLER_TYPE *)thread_input ;
    for (;;)
    {
    /*
        uxBits = xEventGroupWaitBits(
                xEventGroup,        : The event group being tested.
                BIT_0 | BIT_4,      : The bits within the event group to wait for.
                pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
                pdFALSE,            : Don't wait for both bits, either bit will do.
                xTicksToWait )
    */
        masked_evt = xEventGroupWaitBits((i2s->mutex), (I2S_DMA_HISR_DONE), pdTRUE, pdFALSE, portMAX_DELAY);
        PRINT_I2S("%s OAL_RETRIEVE_EVENTS\n",__func__);
    }
}
#endif
//---------------------------------------------------------
// DRIVER :: Utilities
//---------------------------------------------------------





//---------------------------------------------------------
//
//---------------------------------------------------------


static void	drv_i2s_ctrl_down(I2S_HANDLER_TYPE *i2s)
{
	DA16X_UNUSED_ARG(i2s);
	//!2010.09.07// HW_REG_SYSCON0->I2S_EN			= 0x01; // Enable
	//!2010.09.07// HW_REG_WRITE32_I2S( i2s->regmap->dac_ctrl, (I2S_DAC_SRC_EXT|I2S_DAC_STANDBY) ); // Reset=1, Standby=1
	//!2010.09.07// HW_REG_WRITE32_I2S( i2s->regmap->dac_ctrl, (I2S_DAC_STANDBY) );					// Reset=0
	//!2010.09.07// HW_REG_SYSCON0->I2S_EN			= 0x00; // Disable
}


//---------------------------------------------------------
// DRIVER :: Callback
//---------------------------------------------------------

static int	 drv_i2s_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param)
{
	I2S_HANDLER_TYPE	*i2s;

	if(handler == NULL){
		return FALSE;
	}

	i2s = (I2S_HANDLER_TYPE *) handler;

	if( vector <= I2S_CB_MAX ){
		i2s->callback[vector] = (USR_CALLBACK)callback ;
		i2s->cb_param[vector] = (VOID *)param ;
	}

	return TRUE;
}


static void drv_i2s_term(void *param)
{
	I2S_HANDLER_TYPE	*i2s;
    unsigned int data = FALSE;

	if( param == NULL){
		return ;
	}
	i2s = (I2S_HANDLER_TYPE *) param;
    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR((i2s->mutex), (I2S_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
    {
         /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         switch should be requested.  The macro used is port specific and will
         be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         the documentation page for the port being used. */
         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));
    while( (HW_REG_READ32_I2S(i2s->regmap->i2s_sr) & I2S_STT_BUSY) == I2S_STT_BUSY );
#if 1
    DRV_I2S_IOCTL(i2s,I2S_SET_DMA, &data);
	DRV_I2S_IOCTL(i2s,I2S_SET_DMA_RX, &data);
#endif

	if(i2s->callback[I2S_CB_TERM] != NULL){
		i2s->callback[I2S_CB_TERM](i2s->cb_param[I2S_CB_TERM]);
	}
}

static void drv_i2s_empty(void *param)
{
	I2S_HANDLER_TYPE	*i2s;

	if( param == NULL){
		return ;
	}
	i2s = (I2S_HANDLER_TYPE *) param;

	//OAL_SET_EVENTS_HISR((OAL_EVENT_GROUP *)(i2s->mutex), (I2S_DMA_HISR_DONE) , OAL_OR );

	if(i2s->callback[I2S_CB_EMPTY] != NULL){
		i2s->callback[I2S_CB_EMPTY](i2s->cb_param[I2S_CB_EMPTY]);
	}
}

static void drv_i2s_error(void *param)
{
	DA16X_UNUSED_ARG(param);
}

static void i2s_pll_callback(UINT32 clock, void *param)
{

	DA16X_UNUSED_ARG(param);

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		return;
	}

#if 0//jason180218
	if(clock>=100000000)
	{
		i2s_set_downsync(2);
			
	}
	else
	{
		i2s_set_downsync(1);
	}
#endif

}


