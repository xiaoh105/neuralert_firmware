/**
 ****************************************************************************************
 *
 * @file sys_i2c.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "common_def.h"

static void drv_i2c_term(void *param);
static void drv_i2c_term_rx(void *param);
static void drv_i2c_empty(void *param);
static void drv_i2c_error(void *param);
static int	drv_i2c_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param);

static void drv_i2c_pll_down(void);

static void	drv_i2c_ctrl_down(I2C_HANDLER_TYPE *i2c);
static void i2c_pll_callback(UINT32 clock, void *param);

//static void i2c_drv_thread(unsigned long thread_input);

//---------------------------------------------------------
//
//---------------------------------------------------------

#define	DAC_SAMPLE_LIMIT		0x1FF
#define	DAC_SAMPLE_ALIGN(x)		((x) & (~DAC_SAMPLE_LIMIT) )

#define	I2C_DMA_HISR_DONE		0x00800000
#define	I2C_DMA_HISR_DONE_RX	0x00400000


#define DMA_SCAT_LEN    (8192)

static unsigned int sys_clock ;


#define DRIVER_DBG_NONE		DRV_PRINTF
#define DRIVER_DBG_ERROR	DRV_PRINTF

#ifdef I2C_DEBUG_ON
#define PRINT_I2C 	DRV_PRINTF
#define PRINT_DMA	DRV_PRINTF
#else
#define PRINT_I2C 
#define PRINT_DMA
#endif

//---------------------------------------------------------
//
//---------------------------------------------------------

int	_i2c_instance[1];
// Semaphore
static EventGroupHandle_t	_mutex_i2c_instance[1];
static CLOCK_LIST_TYPE			i2c_pll_info;

#define	I2C_INSTANCE		0	//i2c->dev_unit


//#define I2C_USE_GDMA

//---------------------------------------------------------
//
//---------------------------------------------------------

static UINT8	drv_i2c_flag;
//---------------------------------------------------------
//
//---------------------------------------------------------
typedef struct
{
	void *src_addr;
	unsigned int length;
	void *next;
} i2c_descriptor_block_t;

static int i2c_irq_occurred;
static EventGroupHandle_t	i2c_mutex;;

static I2C_REG_MAP *i2c_reg;

HANDLE	DRV_I2C_CREATE( UINT32 dev_id )
{
	I2C_HANDLER_TYPE	*i2c;

	// Allocate
	if((i2c = (I2C_HANDLER_TYPE *) HAL_MALLOC(sizeof(I2C_HANDLER_TYPE)) ) == NULL){
		return NULL;
	}

	// Clear
	DRV_MEMSET(i2c, 0, sizeof(I2C_HANDLER_TYPE));

	// Address Mapping
	switch((I2C_LIST)dev_id){
		case	i2c_0:
				i2c->dev_addr = SYS_I2C_BASE_0;
				i2c->instance = (UINT32)_i2c_instance[I2C_INSTANCE];
				_i2c_instance[I2C_INSTANCE] ++;
				i2c->mutex = &(_mutex_i2c_instance[I2C_INSTANCE]);
				break;

		default:
				DRIVER_DBG_ERROR("illegal unit, %d\r\n", dev_id);
				HAL_FREE(i2c);
				return NULL;
	}


	i2c_reg = (I2C_REG_MAP *)(SYS_I2C_BASE_0);

	// Set Default Para
	i2c->regmap   = (I2C_REG_MAP *)(i2c->dev_addr);
	i2c->dev_unit = dev_id ;
	i2c->i2cclock = 400;
	i2c->dmachan_id_wr = DMA_CH_I2C_WRITE;
	i2c->dmachan_id_rd = DMA_CH_I2C_READ;
	i2c->dma_maxlli = DMA_MAX_LLI_NUM ;

	//PRINT_I2C("(%p) i2c Driver create, base %p\r\n", (UINT32 *)i2c, (UINT32 *)i2c->dev_addr );
	return (HANDLE) i2c;
}

#ifdef I2C_THREAD_ENABLE
OAL_THREAD_TYPE i2c_drv_thread_type;
unsigned int    i2c_drv_thread_stack[512];
unsigned int    i2c_drv_thread_stacksize = 512*sizeof(unsigned int);
#endif

int	DRV_I2C_INIT (HANDLE handler)
{
	I2C_HANDLER_TYPE	*i2c;
	UINT32	dma_control, dma_config = 0;

	if(handler == NULL){
		return FALSE;
	}
	i2c = (I2C_HANDLER_TYPE *)handler ;

	DA16X_CLOCK_SCGATE->Off_DAPB_I2CM = 0;
	DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

	// Normal Mode
	if(i2c->instance == 0){
		HW_REG_AND_WRITE32_I2C(	i2c->regmap->i2c_cr1, (~I2C_CTRL_ENABLE) );

		// DMA Test
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_BYTE) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE) ;
		dma_control |= DMA_CHCTRL_BURSTLENGTH(2);

//		dma_config	= DMA_CONFIG_ITC_MSK | DMA_CONFIG_IE_MSK | DMA_CONFIG_FLOWCTL(DMA_MEM2PERI) ;
//		dma_config	|= DMA_CONFIG_DST_PERI(DMA_DEV_i2cTX) | DMA_CONFIG_SRC_PERI(DMA_DEV_MEM);

		i2c->dmachannel_wr = SYS_DMA_OBTAIN_CHANNEL(i2c->dmachan_id_wr, dma_control, dma_config, i2c->dma_maxlli );


		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_BYTE);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE) ;
		dma_control |= DMA_CHCTRL_BURSTLENGTH(2);

//      dma_config  = DMA_CONFIG_ITC_MSK | DMA_CONFIG_IE_MSK | DMA_CONFIG_FLOWCTL(DMA_MEM2PERI) ;
//      dma_config  |= DMA_CONFIG_DST_PERI(DMA_DEV_i2cTX) | DMA_CONFIG_SRC_PERI(DMA_DEV_MEM);
		i2c->dmachannel_rd = SYS_DMA_OBTAIN_CHANNEL(i2c->dmachan_id_rd, dma_control, dma_config, i2c->dma_maxlli );

		if(i2c->dmachannel_wr != NULL){
			SYS_DMA_REGISTRY_CALLBACK( i2c->dmachannel_wr, drv_i2c_term,  drv_i2c_empty, drv_i2c_error  , (HANDLE) i2c );
			SYS_DMA_REGISTRY_CALLBACK( i2c->dmachannel_rd, drv_i2c_term_rx,  drv_i2c_empty, drv_i2c_error  , (HANDLE) i2c );
		}

		// Post-Action
		_sys_clock_read(&sys_clock,sizeof(unsigned int));
		if( drv_i2c_flag == FALSE ){
			
			DRV_I2C_IOCTL(i2c,I2C_SET_CLOCK, &i2c->i2cclock);
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_buscr,I2C_BUS_STOPEN|0x1); //rd access en
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr1,I2C_CTRL_ENABLE); //enable i2c port
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_en,1); //enable i2c
			drv_i2c_flag = TRUE ;
		}

		// Preset Mode
//		HW_REG_WRITE32_i2c( i2c->regmap->dac_ctrl, (i2c_DAC_SRC_EXT | i2c_DAC_STANDBY | i2c_DAC_MUTE) );



		// DAC Initialization finished ....
	}

	// Semaphore
	i2c->mutex = (VOID *)xEventGroupCreate();
	i2c_mutex = i2c->mutex;

#ifdef I2C_THREAD_ENABLE
	i2c_desc_head.length = 0;
	i2c_desc_head.src_addr = NULL;
	i2c_desc_head.next = NULL;

	
	OAL_CREATE_THREAD( &i2c_drv_thread_type, "@i2cdrv", i2c_drv_thread, (unsigned int)handler,
		i2c_drv_thread_stack, i2c_drv_thread_stacksize, 2, _NO_TIME_SLICE, 
		1, _AUTO_START ); 
#endif 

	_sys_nvic_write(I2C_Master_IRQn, (void *)i2c_drv_interrupt, 0x7);

//pll callback
	i2c_pll_info.callback.func = i2c_pll_callback;
	i2c_pll_info.callback.param = i2c;
	i2c_pll_info.next = NULL;
	_sys_clock_ioctl(SYSCLK_SET_CALLACK, &(i2c_pll_info));

	drv_i2c_flag = FALSE;

	return TRUE;
}


int	DRV_I2C_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	I2C_HANDLER_TYPE	*i2c;
//	UNSIGNED			masked_evt;

	if(handler == NULL){
		return FALSE;
	}
	i2c = (I2C_HANDLER_TYPE *)handler ;

	switch(cmd){
		case I2C_SET_CONFIG:
			break;
		case I2C_GET_CONFIG:
			*((UINT32 *)data) =  HW_REG_READ32_I2C( i2c->regmap->i2c_cr0);
			break;



		case I2C_GET_STATUS:
			if( data != NULL ){
				*((UINT32 *)data) = HW_REG_READ32_I2C( i2c->regmap->i2c_sr);
			}else{
				while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_TFE) != I2C_STATUS_TFE ){
					//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), (i2c_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
					//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), ~(i2c_DMA_HISR_DONE) , OAL_AND ); // Clear
					vTaskDelay(1);
				}
			}
			break;

		case I2C_SET_DMA_WR:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_OR_WRITE32_I2C( i2c->regmap->i2c_dmacr, I2C_DMA_TXEN ) ;
			}else{
				HW_REG_AND_WRITE32_I2C( i2c->regmap->i2c_dmacr, (~I2C_DMA_TXEN) ) ;
			}
			break;
		case I2C_SET_DMA_RD:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_OR_WRITE32_I2C( i2c->regmap->i2c_dmacr, I2C_DMA_RXEN ) ;
			}else{
				HW_REG_AND_WRITE32_I2C( i2c->regmap->i2c_dmacr, (~I2C_DMA_RXEN) ) ;
			}
			break;

		case I2C_GET_DMA_WR:
			*((UINT32 *)data) = HW_REG_READ32_I2C(i2c->regmap->i2c_dmacr)&I2C_DMA_TXEN;
			break;
		case I2C_GET_DMA_RD:
			*((UINT32 *)data) = HW_REG_READ32_I2C(i2c->regmap->i2c_dmacr)&I2C_DMA_RXEN;
			break;

		case I2C_SET_MAXSIZE:
//			i2c->dma_maxlli = ( ( (*((UINT32 *)data)) / DAC_SAMPLE_ALIGN(DMA_TRANSFER_SIZE) ) * 1 ) + 1 ;
			break;

		case I2C_SET_RESET:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_WRITE32_I2C( i2c->regmap->i2c_dmacr,  0 ) ;

			}else{
				HW_REG_AND_WRITE32_I2C(	i2c->regmap->i2c_cr0, (~I2C_CFG_POWER_DOWN) );


			}
			break;

		case I2C_SET_HISR_CALLACK:
			drv_i2c_callback_registry( i2c, ((UINT32 *)data)[0], ((UINT32 *)data)[1], ((UINT32 *)data)[2] );
			break;

		case I2C_SET_CHIPADDR:
			HW_REG_WRITE32_I2C(i2c->regmap->i2c_drvid, *((char*)data));
			break;

		case I2C_GET_CHIPADDR:
			*((char*)data) = HW_REG_READ32_I2C(i2c->regmap->i2c_drvid);
			break;

		case I2C_SET_CLOCK:
			{
				_sys_clock_read(&sys_clock,sizeof(unsigned int));

			/* patch for low cpu clock & high i2c clock speed */                
				if(sys_clock < 80000000)
					HW_REG_WRITE32_I2C(i2c->regmap->i2c_clkdat_delay, 0);

				unsigned int val = (sys_clock / ((*((unsigned int *) data))*2000))-1;
				if(val>255)
				{
					val = (sys_clock / ((*((unsigned int *) data))*4000))-1;
					if(val>255)
					{
						val = (sys_clock / ((*((unsigned int *) data))*6000))-1;
						
						if(val>255)
						{
							val = (sys_clock / ((*((unsigned int *) data))*8000))-1;
							
							if(val>255)
								val = 255;
							HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,0x8); 
							HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,(val&0xff)<<8|0x7); 
						}
						else
						{
							HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,0x6); 
							HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,(val&0xff)<<8|0x7); 
						}
					}
					else
					{
					HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,0x4); 
					HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,(val&0xff)<<8|0x7); 
					}
				}
				else
				{
					HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,0x2); 
					HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,(val&0xff)<<8|0x7); 
				}
				i2c->i2cclock = *((unsigned int *) data);

			}
			break;
			
		case I2C_SET_CLK_DAT_DELAY:
//#if	defined(BUILD_OPT_SLR)
			{
				HW_REG_WRITE32_I2C(i2c->regmap->i2c_clkdat_delay, *((unsigned int*)data));
			}
//#else
//			return FALSE;
//#endif		
			break;
		default:
			break;
	}
	return TRUE;
}

#undef DMA_BUS_LOAD_TEST

int	DRV_I2C_WRITE(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 stopen, UINT32 reset_en)
{
		DA16X_UNUSED_ARG(reset_en);

        I2C_HANDLER_TYPE    *i2c;
    //  UNSIGNED            masked_evt;
        unsigned char*      data;
        unsigned int    dma_en;
        VOID   *scat_offset ;
        UNSIGNED masked_evt ;
        int   status;
    
        if(handler == NULL){
            return FALSE;
        }
        i2c = (I2C_HANDLER_TYPE *)handler ;
#if 1//jason161112
		unsigned int	caddr, cpsr, cr0, i2c_clkdat_delay ;

		caddr = HW_REG_READ32_I2C(i2c->regmap->i2c_drvid);
		cpsr = HW_REG_READ32_I2C( i2c->regmap->i2c_cpsr); 
		cr0 = HW_REG_READ32_I2C( i2c->regmap->i2c_cr0); 
		i2c_clkdat_delay = HW_REG_READ32_I2C( i2c->regmap->i2c_clkdat_delay); 

		*(volatile unsigned int *)(0x500802d0) &= ~(0x2);
		*(volatile unsigned int *)(0x500802d0) |= (0x2);

		HW_REG_WRITE32_I2C( i2c->regmap->i2c_buscr,I2C_BUS_STOPEN|0x1); //rd access en
		HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr1,I2C_CTRL_ENABLE); //enable i2c port
		HW_REG_WRITE32_I2C( i2c->regmap->i2c_en,1); //enable i2c
		HW_REG_WRITE32_I2C(i2c->regmap->i2c_drvid,caddr);
		HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,cpsr); 
		HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,cr0); 
		HW_REG_WRITE32_I2C( i2c->regmap->i2c_clkdat_delay,i2c_clkdat_delay); 
#endif
        i2c->regmap->i2c_en = 0; // disable
        i2c->regmap->i2c_en = 1; // enable
        i2c->regmap->i2c_buscr &= ~(I2C_BUS_RDACC); //write access
        if(stopen)
        {
            i2c->regmap->i2c_cr1 &= ~(I2C_CTRL_STEP2); // 1 step read
            i2c->regmap->i2c_buscr |= I2C_BUS_STOPEN; //stop enable
        }
        else
        {
            i2c->regmap->i2c_cr1 |= (I2C_CTRL_STEP2); // 1 step read
            i2c->regmap->i2c_buscr &= ~(I2C_BUS_STOPEN); //stop disable
        }
        
        data = (unsigned char*)p_data;


        i2c->regmap->i2c_msc |= (I2C_MASK_NACK); //NACK interrupt enable
        if(p_dlen <= 8)
        {
            for(UINT32 i=0; i<p_dlen; i++)
            {
                HW_REG_WRITE32_I2C( i2c->regmap->i2c_dr,  *data++ ) ;
                
            }
            i2c->regmap->i2c_msc |= (I2C_MASK_WR_DONE); //write fifo interrupt enable
            
        /*
            uxBits = xEventGroupWaitBits(
                    xEventGroup,        : The event group being tested.
                    BIT_0 | BIT_4,      : The bits within the event group to wait for.
                    pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
                    pdFALSE,            : Don't wait for both bits, either bit will do.
                    xTicksToWait )
        */
            masked_evt = xEventGroupWaitBits((i2c->mutex), I2C_STATUS_WR_DONE | I2C_STATUS_NACK, pdTRUE, pdFALSE, 5);
            if( !(masked_evt & (I2C_STATUS_WR_DONE | I2C_STATUS_NACK)) )
            {
                PRINT_I2C("[%s] wait timeout !!!!! \r\n",__func__);
            }
//            PRINT_I2C("status = 0x%08x\r\n",masked_evt);
            
            if(masked_evt & ((unsigned int)I2C_STATUS_NACK)) //No-Ack Error
            {
                PRINT_I2C("[%s] No ACK error !!!!! \r\n",__func__);
                i2c->regmap->i2c_en = 0; // disable
                i2c->regmap->i2c_en = 1; // enable
                return FALSE;
            }
            while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_BUSY) == I2C_STATUS_BUSY )

                {
                   PRINT_I2C("Wait Busy Flag one more time\r\n");
                   vTaskDelay(1);
                }

       }
        else
        {
            if(p_dlen == 0){
                return FALSE;
            }
        
            scat_offset = p_data ;
        
//            stuff_len = p_dlen % 4;
//            p_dlen = (p_dlen/4)*4;
                
			SYS_DMA_RESET_CH(i2c->dmachannel_wr);//jason151105
            
            
            // DMA Action
#ifdef DMA_LLI_MODE_SUPPORT
            status = SYS_DMA_COPY(i2c->dmachannel_wr, (UINT32 *)(&(i2c->regmap->i2c_dr)) , scat_offset, p_dlen);
    
            if(status != 0){
                p_dlen = 0;
                return FALSE;
            }
            else
            {
                DRV_I2C_IOCTL(i2c,I2C_GET_DMA_WR, &dma_en);
                if(!dma_en)
                {
                    dma_en = TRUE;
                    DRV_I2C_IOCTL(i2c,I2C_SET_DMA_WR, &dma_en);
                }
            }
                
#endif
#ifdef DMA_BUS_LOAD_TEST
{
		int i =0;
		volatile int	test_val[2];
		for(i=0;i<5000000;i++)
		{
			test_val[0] = i;
			*(unsigned volatile int *)(0x50001250) = test_val[0];
		}
		

}
#endif
        
        /*
            uxBits = xEventGroupWaitBits(
                    xEventGroup,        : The event group being tested.
                    BIT_0 | BIT_4,      : The bits within the event group to wait for.
                    pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
                    pdFALSE,            : Don't wait for both bits, either bit will do.
                    xTicksToWait )
        */
            masked_evt = xEventGroupWaitBits((i2c->mutex), (I2C_DMA_HISR_DONE|I2C_STATUS_NACK), pdTRUE, pdFALSE, 100);
            if( !(masked_evt & (I2C_DMA_HISR_DONE|I2C_STATUS_NACK)) )
            	PRINT_DMA("DRV_I2C_WRITE Timed out \r\n");
#if 0
            for(unsigned int i=0; i<stuff_len; i++) //copy stuffing bytes
             {
                 HW_REG_WRITE32_I2C( i2c->regmap->i2c_dr,  *(((unsigned char *)scat_offset)+i) ) ;
             }
#endif            
        }

    return TRUE;
}
#undef MAKE_SLAVE_UNSTABLE

#define GLOBAL_INT_DISABLE() taskDISABLE_INTERRUPTS()
#define GLOBAL_INT_RESTORE() taskENABLE_INTERRUPTS()

int	DRV_I2C_READ(HANDLE handler,  VOID *p_data, UINT32 p_dlen, UINT32 addr_len, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);
	I2C_HANDLER_TYPE    *i2c;
//  UNSIGNED            masked_evt;
	unsigned char*      data ;
	unsigned int    dma_en;
	UINT32  scat_len ,stuff_len;
	VOID   *scat_offset ;
	UNSIGNED masked_evt ;
	int   status;

	if(handler == NULL){
		return FALSE;
	}
	i2c = (I2C_HANDLER_TYPE *)handler ;

	data = (unsigned char*)p_data;
#if 1//jason161112
	unsigned int	caddr, cpsr, cr0, i2c_clkdat_delay ;

	caddr = HW_REG_READ32_I2C(i2c->regmap->i2c_drvid);
	cpsr = HW_REG_READ32_I2C( i2c->regmap->i2c_cpsr);
	cr0 = HW_REG_READ32_I2C( i2c->regmap->i2c_cr0);
	i2c_clkdat_delay = HW_REG_READ32_I2C( i2c->regmap->i2c_clkdat_delay);

	*(volatile unsigned int *)(0x500802d0) &= ~(0x2);
	*(volatile unsigned int *)(0x500802d0) |= (0x2);

	HW_REG_WRITE32_I2C( i2c->regmap->i2c_buscr,I2C_BUS_STOPEN|0x1); //rd access en
	HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr1,I2C_CTRL_ENABLE); //enable i2c port
	HW_REG_WRITE32_I2C( i2c->regmap->i2c_en,1); //enable i2c
	HW_REG_WRITE32_I2C(i2c->regmap->i2c_drvid,caddr);
	HW_REG_WRITE32_I2C( i2c->regmap->i2c_cpsr,cpsr);
	HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,cr0);
	HW_REG_WRITE32_I2C( i2c->regmap->i2c_clkdat_delay,i2c_clkdat_delay);
#endif
	if(addr_len > 0)
	{
		i2c->regmap->i2c_en = 0; // disable
		i2c->regmap->i2c_en = 1; // enable
		i2c->regmap->i2c_cr1 &= ~(I2C_CTRL_STEP2); // 1 step read
	}
	else
		i2c->regmap->i2c_cr1 |= (I2C_CTRL_STEP2); // 2 step read

	i2c->regmap->i2c_rdlen = p_dlen;
	i2c->regmap->i2c_buscr &= ~(0xf); //read access
	i2c->regmap->i2c_buscr |= I2C_BUS_RDACC|I2C_BUS_STOPEN|(addr_len&0x7); //read access



	i2c->regmap->i2c_msc |= (I2C_MASK_NACK); //NACK interrupt enable
	if(p_dlen <= 8)
	{
		i2c->regmap->i2c_msc &= ~(I2C_MASK_RD_DONE); //write fifo interrupt enable
		i2c->regmap->i2c_msc |= (I2C_MASK_RD_DONE); //write fifo interrupt enable
		for(UINT32 i=0; i<addr_len; i++)
		{
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_dr,  *data++ ) ;
		}

		data = (unsigned char*)p_data;

//            OAL_MSLEEP(10);  // 10000 usec
	/*
		uxBits = xEventGroupWaitBits(
				xEventGroup,        : The event group being tested.
				BIT_0 | BIT_4,      : The bits within the event group to wait for.
				pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
				pdFALSE,            : Don't wait for both bits, either bit will do.
				xTicksToWait )
	*/
		masked_evt = xEventGroupWaitBits((i2c->mutex), (I2C_STATUS_RD_DONE | I2C_STATUS_NACK), pdTRUE, pdFALSE, 5);
		if( !(masked_evt & (I2C_STATUS_RD_DONE | I2C_STATUS_NACK)) )
		{
			PRINT_I2C("[%s] wait timeout !!!!! \r\n",__func__);
		}

		if(masked_evt & ((unsigned int)I2C_STATUS_NACK)) //No-Ack Error
		{
			PRINT_I2C("[%s] No ACK error !!!!! \r\n",__func__);
			i2c->regmap->i2c_en = 0; // disable
			i2c->regmap->i2c_en = 1; // enable
			return FALSE;
		}

		if(masked_evt & I2C_STATUS_RD_DONE)
		{

		}

		while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_BUSY) == I2C_STATUS_BUSY )

			{
			   PRINT_I2C("Wait Busy Flag one more time\r\n");
			  vTaskDelay(1);
			}

//            PRINT_I2C("status = 0x%08x\r\n",masked_evt);

		for(UINT32 i=0; i<p_dlen; i++)
		{
			data[i] = (unsigned char)(HW_REG_READ32_I2C( i2c->regmap->i2c_dr)) ;
		}

		i2c->regmap->i2c_buscr &= ~(I2C_BUS_RDACC); //write access
	}
	else
	{
		data = (unsigned char*)p_data;

		if(p_dlen == 0){
			return FALSE;
		}

		scat_offset = p_data ;


		SYS_DMA_RESET_CH(i2c->dmachannel_rd);//jason151105
		do{
			stuff_len = p_dlen % 4;
			p_dlen = (p_dlen/4)*4;
			if(p_dlen > (DMA_SCAT_LEN/4))
			{
				scat_len = (DMA_SCAT_LEN/4);
				p_dlen -= (DMA_SCAT_LEN/4);
			}
			else
			{
				scat_len = p_dlen;
				p_dlen = 0;
			}


			// DMA Action
#ifdef DMA_LLI_MODE_SUPPORT
			status = SYS_DMA_COPY(i2c->dmachannel_rd , scat_offset, (UINT32 *)(&(i2c->regmap->i2c_dr)), scat_len);

			if(status != 0){
				p_dlen = 0;
				continue;
			}
			else
			{
				DRV_I2C_IOCTL(i2c,I2C_GET_DMA_RD, &dma_en);
				if(!dma_en)
				{
					dma_en = TRUE;
					DRV_I2C_IOCTL(i2c,I2C_SET_DMA_RD, &dma_en);
				}
			}
#ifdef MAKE_SLAVE_UNSTABLE
			for(int i=0; i<addr_len; i++)
			{
				HW_REG_WRITE32_I2C( i2c->regmap->i2c_dr,  *data++ ) ;
			}
#endif                    
#endif

			// Post Action
			scat_offset =  (VOID *)((UINT32)scat_offset + scat_len) ;

		}while(p_dlen > 0);

#ifndef MAKE_SLAVE_UNSTABLE

		GLOBAL_INT_DISABLE();
		for(UINT32 i=0; i<addr_len; i++)
		{
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_dr,  *data++ ) ;
		}
		GLOBAL_INT_RESTORE();
#endif                    
#ifdef DMA_BUS_LOAD_TEST
		{
				int i =0;
				volatile int	test_val[2];
				for(i=0;i<5000000;i++)
				{
					test_val[0] = i;
					*(unsigned volatile int *)(0x50001250) = test_val[0];
				}


		}
#endif
	/*
		uxBits = xEventGroupWaitBits(
				xEventGroup,        : The event group being tested.
				BIT_0 | BIT_4,      : The bits within the event group to wait for.
				pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
				pdFALSE,            : Don't wait for both bits, either bit will do.
				xTicksToWait )
	*/
		masked_evt = xEventGroupWaitBits((i2c->mutex), (I2C_DMA_HISR_DONE_RX|I2C_STATUS_NACK), pdTRUE, pdFALSE, 100);
		if( !(masked_evt & (I2C_DMA_HISR_DONE_RX|I2C_STATUS_NACK)) )
			PRINT_DMA("DRV_I2C_READ Timed out \r\n");

		for(unsigned int i=0; i<stuff_len; i++) //copy stuffing bytes
		 {
		  *(((unsigned char *)scat_offset)+i) =  HW_REG_READ32_I2C( i2c->regmap->i2c_dr) ;
		 }

//       _sys_dma_wait(i2c->dmachan_id_rd, 1000);

	}
	return TRUE;
        
}




int	DRV_I2C_CLOSE(HANDLE handler)
{
	I2C_HANDLER_TYPE	*i2c;
//	UNSIGNED 			masked_evt;

	if(handler == NULL){
		return FALSE;
	}
	i2c = (I2C_HANDLER_TYPE *)handler ;

	if ( _i2c_instance[I2C_INSTANCE] > 0) {
		_i2c_instance[I2C_INSTANCE] -- ;

		if(_i2c_instance[I2C_INSTANCE] == 0) {
			// Wait Until i2c TX-Term
			while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_BUSY) == I2C_STATUS_BUSY ){
				//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), (i2c_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
				//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), ~(i2c_DMA_HISR_DONE) , OAL_AND );								// Clear
                  vTaskDelay(1);
			}
#if 0//jason160703 Why???????????????????
			// Wait Until i2c
			while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_TFE) == I2C_STATUS_TFE ){
				OAL_MSLEEP(1);	// 1000 usec
			}
#endif
			SYS_DMA_RELEASE_CHANNEL(i2c->dmachannel_wr);
			SYS_DMA_RELEASE_CHANNEL(i2c->dmachannel_rd);

#ifdef I2C_THREAD_ENABLE
			// Delete Semaphore
            vEventGroupDelete((EventGroupHandle_t)(i2c->mutex));
#endif
			// DMA disable
			HW_REG_WRITE32_I2C( i2c->regmap->i2c_dmacr,  0 ) ;

			drv_i2c_ctrl_down( i2c );

			switch((I2C_LIST)i2c->dev_unit){
				case	i2c_0:
						// Clock disable
						//!2010.09.07// HW_REG_SYSCON0->i2c_EN = 0x00 ;
						drv_i2c_pll_down();
						// Reset
						//!2010.09.07// HW_REG_SYSCON0->ni2cRST = 0x00;
						//!2010.09.07// HW_REG_SYSCON0->ni2cRST = 0x01;
						break;

				default:
					break;
			}
		}
	}
	_sys_clock_ioctl(SYSCLK_RESET_CALLACK, &(i2c_pll_info));

	HAL_FREE(handler);
	
	DA16X_CLKGATE_OFF(clkoff_i2cM);
	return TRUE;
}

void	i2c_drv_interrupt(void)
{
	INTR_CNTXT_CALL(i2c_drv_interrupt_handler);
}

void i2c_drv_interrupt_handler(void)
{
    UINT32  status;
    status = i2c_reg->i2c_sr;
    
    i2c_irq_occurred++;
    if (i2c_reg->i2c_ris & I2C_INT_NACK )  {
      /* I2C interrupt is caused by  error */
      status |= (unsigned int)I2C_STATUS_NACK;
      i2c_reg->i2c_msc &= ~(I2C_MASK_NACK); //NACK interrupt disable
      }
    
    if(status & I2C_STATUS_TFE) {
      // I2C interrupt is caused by FIFO
      i2c_reg->i2c_msc &= ~(I2C_MASK_TXIM); //write fifo interrupt disable
      }

    if(status & I2C_STATUS_TFE) {
      // I2C interrupt is caused by FIFO
      i2c_reg->i2c_msc &= ~(I2C_MASK_WR_DONE); //write fifo interrupt disable
      }

    if(status & I2C_STATUS_RD_DONE) {
      // I2C interrupt is caused by FIFO
      i2c_reg->i2c_msc &= ~(I2C_MASK_RD_DONE); //read done interrupt disable
      }

    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR((i2c_mutex), status, &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
    {
         /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         switch should be requested.  The macro used is port specific and will
         be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         the documentation page for the port being used. */
         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//    xEventGroupSetBits((EventGroupHandle_t)(i2c_mutex), (status));
}

#ifdef I2C_THREAD_ENABLE

void i2c_drv_thread(unsigned long thread_input)
{
	I2C_HANDLER_TYPE	*i2c;
	UNSIGNED masked_evt ;
	
	if(thread_input == NULL){
		return;
	}
    
	i2c = (I2C_HANDLER_TYPE *)thread_input ;
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
        masked_evt = xEventGroupWaitBits((i2c->mutex), (I2C_DMA_HISR_DONE), pdTRUE, pdFALSE, portMAX_DELAY);
        PRINT_I2C("%s OAL_RETRIEVE_EVENTS\n",__func__);


    
    }
}
#endif
//---------------------------------------------------------
// DRIVER :: Utilities
//---------------------------------------------------------
static void drv_i2c_pll_down(void)
{
}

//---------------------------------------------------------
//
//---------------------------------------------------------


static void	drv_i2c_ctrl_down(I2C_HANDLER_TYPE *i2c)
{
	DA16X_UNUSED_ARG(i2c);
}


//---------------------------------------------------------
// DRIVER :: Callback
//---------------------------------------------------------

static int	 drv_i2c_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param)
{
	I2C_HANDLER_TYPE	*i2c;

	if(handler == NULL){
		return FALSE;
	}

	i2c = (I2C_HANDLER_TYPE *) handler;

	if( vector <= I2C_CB_MAX ){
		i2c->callback[vector] = (USR_CALLBACK)callback ;
		i2c->cb_param[vector] = (VOID *)param ;
	}

	return TRUE;
}


static void drv_i2c_term(void *param)
{
	I2C_HANDLER_TYPE	*i2c;
    unsigned int data = FALSE;

	if( param == NULL){
		return ;
	}
	i2c = (I2C_HANDLER_TYPE *) param;
    while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_BUSY) == I2C_STATUS_BUSY );

    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR((i2c->mutex), (I2C_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
    {
         /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         switch should be requested.  The macro used is port specific and will
         be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         the documentation page for the port being used. */
         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//    xEventGroupSetBits((EventGroupHandle_t)(i2c_mutex), (status));

    DRV_I2C_IOCTL(i2c,I2C_SET_DMA_WR, &data);

	if(i2c->callback[I2C_CB_TERM] != NULL){
		i2c->callback[I2C_CB_TERM](i2c->cb_param[I2C_CB_TERM]);
	}
}

static void drv_i2c_term_rx(void *param)
{
	I2C_HANDLER_TYPE	*i2c;
    unsigned int data = FALSE;

	if( param == NULL){
		return ;
	}
	i2c = (I2C_HANDLER_TYPE *) param;
    while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_BUSY) == I2C_STATUS_BUSY );

    BaseType_t xHigherPriorityTaskWoken, xResult;
    xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR((i2c->mutex), (I2C_DMA_HISR_DONE_RX), &xHigherPriorityTaskWoken);
    if( xResult != pdFAIL )
    {
         /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         switch should be requested.  The macro used is port specific and will
         be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         the documentation page for the port being used. */
         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
//    xEventGroupSetBits((EventGroupHandle_t)(i2c_mutex), (status));

    DRV_I2C_IOCTL(i2c,I2C_SET_DMA_RD, &data);

	if(i2c->callback[I2C_CB_TERM] != NULL){
		i2c->callback[I2C_CB_TERM](i2c->cb_param[I2C_CB_TERM]);
	}
}

static void drv_i2c_empty(void *param)
{
	I2C_HANDLER_TYPE	*i2c;

	if( param == NULL){
		return ;
	}
	i2c = (I2C_HANDLER_TYPE *) param;

	//OAL_SET_EVENTS_HISR((OAL_EVENT_GROUP *)(i2c->mutex), (i2c_DMA_HISR_DONE) , OAL_OR );

	if(i2c->callback[I2C_CB_EMPTY] != NULL){
		i2c->callback[I2C_CB_EMPTY](i2c->cb_param[I2C_CB_EMPTY]);
	}
}

static void drv_i2c_error(void *param)
{
	DA16X_UNUSED_ARG(param);
}

static void i2c_pll_callback(UINT32 clock, void *param)
{
	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}	

	I2C_HANDLER_TYPE *i2c = (I2C_HANDLER_TYPE *) param;
	sys_clock = clock;
	DRV_I2C_IOCTL(i2c,I2C_SET_CLOCK, &i2c->i2cclock);
}

