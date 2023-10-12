/**
 ****************************************************************************************
 *
 * @file adc.c
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
#include "da16200_regs.h"
#include "driver.h"
#include "sys_dma.h"
#include "adc.h"
#include "common_def.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define FC9000_FPGA_ADC_SUPPORT

static int	_adc_instance[1] = {0};
static UINT8	drv_adc_flag = FALSE;

#define	ADC_INSTANCE		0	//adc->dev_unit
#ifdef	SUPPORT_KDMA
#define DMA_SCAT_LEN    (4096)
#else
#define DMA_SCAT_LEN    (2048)
#endif

static void drv_adc_term0(void *param);
static void drv_adc_term1(void *param);
static void drv_adc_term2(void *param);
static void drv_adc_term3(void *param);
static void drv_adc_empty(void *param);
static void drv_adc_error(void *param);
#if 0 //unused function
static int	drv_adc_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param);
#endif /*0*/

unsigned int g_divider = 0;
unsigned int g_interrupt_cnt = 0;

//---------------------------------------------------------
//
//---------------------------------------------------------


ADC_HANDLER_TYPE	*g_drv_adc = NULL;


HANDLE	DRV_ADC_CREATE( UINT32 dev_id )
{
	ADC_HANDLER_TYPE	*adc;

	// Allocate
	if((adc = (ADC_HANDLER_TYPE *) pvPortMalloc(sizeof(ADC_HANDLER_TYPE)) ) == NULL){
		return NULL;
	}

	DA16X_CLOCK_SCGATE->Off_DAPB_AuxA = 0;
	DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

	// Clear
	memset(adc, 0, sizeof(ADC_HANDLER_TYPE));

	adc->dev_addr = SYS_ADC_BASE;
	adc->instance = (UINT32)(_adc_instance[0]);
	_adc_instance[0] ++;

	// Set Default Para
	adc->regmap   = (ADC_REG_MAP *)(adc->dev_addr);
	adc->regmap_rtc = (RTC_REG_MAP *)RTC_BASE_0;
	adc->dev_unit = dev_id ;
	adc->clock = 0;
	adc->dmachan_id_ADC_FIFO0 = CH_DMA_PERI_ADC_FIFO0;
	adc->dmachan_id_ADC_FIFO1 = CH_DMA_PERI_ADC_FIFO1;
	adc->dmachan_id_ADC_FIFO2 = CH_DMA_PERI_ADC_FIFO2;
	adc->dmachan_id_ADC_FIFO3 = CH_DMA_PERI_ADC_FIFO3;
	adc->dma_maxlli = DMA_MAX_LLI_NUM ;
//    adc->mutex = &(_mutex_adc_instance[ADC_INSTANCE]);

	g_drv_adc = adc;

	PRINT_ADC("(%p) adc Driver create, base %p\r\n", (UINT32 *)adc, (UINT32 *)adc->dev_addr );
	return (HANDLE) adc;
}

int	DRV_ADC_INIT (HANDLE handler,  unsigned int use_timestamp)
{
	ADC_HANDLER_TYPE	*adc;
	UINT32	dma_control, dma_config = 0;


	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;
	adc->use_timestamp = use_timestamp;


	
	// Normal Mode
	if(adc->instance == 0){

		// DMA Test

		if(use_timestamp) // 32bit transfer
		{
	        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
	        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD) ;
	        dma_control |= DMA_CHCTRL_BURSTLENGTH(2);
		}
		else // 16bit transfer
		{
	        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_HALFWORD);
	        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_HALFWORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_HALFWORD) ;
	        dma_control |= DMA_CHCTRL_BURSTLENGTH(2);
		}
		adc->dmachannel_ADC_FIFO0= SYS_DMA_OBTAIN_CHANNEL(adc->dmachan_id_ADC_FIFO0, dma_control, dma_config, adc->dma_maxlli );
		if(adc->dmachannel_ADC_FIFO0 != NULL){
			SYS_DMA_REGISTRY_CALLBACK( adc->dmachannel_ADC_FIFO0, drv_adc_term0,  drv_adc_empty, drv_adc_error  , (HANDLE) adc );
		}
		
		adc->dmachannel_ADC_FIFO1= SYS_DMA_OBTAIN_CHANNEL(adc->dmachan_id_ADC_FIFO1, dma_control, dma_config, adc->dma_maxlli );
		if(adc->dmachannel_ADC_FIFO1 != NULL){
			SYS_DMA_REGISTRY_CALLBACK( adc->dmachannel_ADC_FIFO1, drv_adc_term1,  drv_adc_empty, drv_adc_error  , (HANDLE) adc );
		}
		
		adc->dmachannel_ADC_FIFO2= SYS_DMA_OBTAIN_CHANNEL(adc->dmachan_id_ADC_FIFO2, dma_control, dma_config, adc->dma_maxlli );
		if(adc->dmachannel_ADC_FIFO2 != NULL){
			SYS_DMA_REGISTRY_CALLBACK( adc->dmachannel_ADC_FIFO2, drv_adc_term2,  drv_adc_empty, drv_adc_error  , (HANDLE) adc );
		}
		
		adc->dmachannel_ADC_FIFO3= SYS_DMA_OBTAIN_CHANNEL(adc->dmachan_id_ADC_FIFO3, dma_control, dma_config, adc->dma_maxlli );
		if(adc->dmachannel_ADC_FIFO3 != NULL){
			SYS_DMA_REGISTRY_CALLBACK( adc->dmachannel_ADC_FIFO3, drv_adc_term3,  drv_adc_empty, drv_adc_error  , (HANDLE) adc );
		}


		// Post-Action
		if( drv_adc_flag == FALSE ){
			drv_adc_flag = TRUE ;
		}

	}

	adc->interrupt_mode = 0;

	// Semaphore
	{
		adc->mutex = xEventGroupCreate();
		Printf("[%s] adc->mutex addr = 0x%x\n",__func__, (unsigned int)adc->mutex);
		if (adc->mutex == NULL)
		{
			Printf("[%s] adc->mutex Event Flags Create Error!\n",__func__);
		}
	}
	_sys_nvic_write(AUX_ADC_IRQn, (void *)adc_drv_interrupt, 0x7);
	
	// ADC Initialization finished ....
	return TRUE;
}

int DRV_ADC_START(HANDLE handler, UINT32 divider12, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	if(handler == NULL){
		return FALSE;
	}


#if 0//jason180211
	g_divider    = divider16;
//	DA16200_BOOTCON->CLK_EN &= ~(0x180);
	DA16200_BOOTCON->CLK_EN &= ~((0x1<<CLKEN_ADC12B)) ;//ADC jason170213
	DA16200_BOOTCON->AUX_ADC_clk_ctrl &= ~(0xFF);
	DA16200_BOOTCON->AUX_ADC_clk_ctrl |= ((exp12&0xf)|((divider12&0xf)<<4));

	DA16200_BOOTCON->AUX_ADC_clk_ctrl &= ~(0x1FF00);
	DA16200_BOOTCON->AUX_ADC_clk_ctrl |= ((exp16&0xf)<<8|((divider16&0x1f)<<12));
//	DA16200_BOOTCON->CLK_EN |= (0x180);
	DA16200_BOOTCON->CLK_EN |= ((0x1<<CLKEN_ADC12B)) ;//ADC jason170213
#else
	DA16200_SYSCLOCK->CLK_EN_AUXA = 0x0;
	DA16200_SYSCLOCK->PLL_CLK_EN_6_AUXA = 0x0;
	DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA = 0x1f;
#if 0
	for(i=0;i<100;i++)
	{
		if(DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA == 0x1f)
			break;
		DRIVER_DBG_ERROR("PLL_CLK_DIV_6_AUXA = 0x%x\r\n",DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA) ;
		DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA = 0x1f;
	}
#endif
	DA16200_SYSCLOCK->CLK_DIV_AUXA = divider12;
	DA16200_SYSCLOCK->SRC_CLK_SEL_6_AUXA = 0x1;
	DA16200_SYSCLOCK->PLL_CLK_EN_6_AUXA = 0x1;
//	*(volatile unsigned char *)(0x50003096) = 1;
	DA16200_SYSCLOCK->CLK_EN_AUXA = 0x1;

	if(divider12 == 0)
	{
		DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA = DA16200_SYSCLOCK->PLL_CLK_DIV_0_CPU;
		DA16200_SYSCLOCK->CLK_DIV_AUXA = divider12;
		DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA = 0x1f;
#if 0
		for(i=0;i<100;i++)
		{
			if(DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA == 0x1f)
				break;
			DRIVER_DBG_ERROR("PLL_CLK_DIV_6_AUXA = 0x%x\r\n",DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA) ;
			DA16200_SYSCLOCK->PLL_CLK_DIV_6_AUXA = 0x1f;
		}
#endif
	}

#endif
	//Enable

//	PRINT_ADC("[0x50001010] : 0X%08x\r\n",*(volatile unsigned int*)(0x50001010));
//	PRINT_ADC("[0x5000101c] : 0X%08x\r\n",*(volatile unsigned int*)(0x5000101c));

	return TRUE;
}

int DRV_ADC_READ(HANDLE handler,  UINT32 channel, UINT32 *data,  UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE    *adc;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(channel){
		case    ADC_FIFO0:
			*data = adc->regmap->fifo0_data_curr;
			break;

		case    ADC_FIFO1:
			*data = adc->regmap->fifo1_data_curr;
			break;

		case    ADC_FIFO2:
			*data = adc->regmap->fifo2_data_curr;
			break;

		case    ADC_FIFO3:
			*data = adc->regmap->fifo3_data_curr;
			break;

		default:
			DRIVER_DBG_ERROR("illegal unit,\r\n");
			return FALSE;
	}

	return TRUE;
}

int DRV_ADC_READ_FIFO(HANDLE handler,  UINT32 channel, UINT32 *data,  UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE    *adc;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(channel){
		case    ADC_FIFO0:
			*data = adc->regmap->fifo0_data;
			break;

		case    ADC_FIFO1:
			*data = adc->regmap->fifo1_data;
			break;

		case    ADC_FIFO2:
			*data = adc->regmap->fifo2_data;
			break;

		case    ADC_FIFO3:
			*data = adc->regmap->fifo3_data;
			break;

		default:
			DRIVER_DBG_ERROR("illegal unit,\r\n");
			return FALSE;
	}

	return TRUE;
}

int DRV_ADC_READ_DMA(HANDLE handler,  UINT32 channel, UINT16 *p_data, UINT32 p_dlen, UINT32 wait, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE    *adc;
	unsigned int status;
	UINT32  scat_len ,stuff_len;
	VOID   *scat_offset ;
	UNSIGNED masked_evt ;
	unsigned int    dma_en;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	if(channel >= ADC_MAX)
	{
		DRIVER_DBG_ERROR("illegal unit,\r\n");
		return FALSE;
	}

//	data = (unsigned char*)p_data;

	if(p_dlen == 0){
		return FALSE;
	}

	scat_offset = p_data ;

	//Stop ADC
	//Clear
#if 1 //using runtime add link
	do{
		stuff_len = p_dlen % 2;
		p_dlen = (p_dlen/2)*2;
		if(p_dlen > (DMA_SCAT_LEN/2))
		{
			scat_len = (DMA_SCAT_LEN/2);
			p_dlen -= (DMA_SCAT_LEN/2);
		}
		else
		{
			scat_len = p_dlen;
			p_dlen = 0;
		}

		// DMA Action
		switch(channel){
			case    ADC_FIFO0:
				status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO0, scat_offset, (UINT32 *)(&(adc->regmap->fifo0_data)), scat_len);
				break;

			case    ADC_FIFO1:
				status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO1 , scat_offset, (UINT32 *)(&(adc->regmap->fifo1_data)), scat_len);
				break;

			case    ADC_FIFO2:
				status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO2 , scat_offset, (UINT32 *)(&(adc->regmap->fifo2_data)), scat_len);
				break;

			case    ADC_FIFO3:
				status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO3 , scat_offset, (UINT32 *)(&(adc->regmap->fifo3_data)), scat_len);
				break;

			default:
				status = 0xffffffff;//klocwork

		}

		if(status != 0){
			p_dlen = 0;
			continue;
		}
		else
		{
			DRV_ADC_IOCTL(adc,ADC_GET_DMA_RD, &dma_en);
			if(!((1<<channel)&dma_en))//jason150327 for ASIC Simulation Error
			{
				dma_en |= (1<<channel);
				DRV_ADC_IOCTL(adc,ADC_SET_DMA_RD, &dma_en);
			}
				
//Start ADC				
		}

		// Post Action
		scat_offset =  (VOID *)((UINT32)scat_offset + scat_len) ;

	}while(p_dlen > 0);
#else//using dma's own exceeding process

	switch(channel){
		case	ADC_FIFO0:
			status = SYS_DMA_COPY(adc->dmachannel_ADC_FIFO0, scat_offset, &(adc->regmap->fifo0_data), p_dlen);
			break;

		case	ADC_FIFO1:
			status = SYS_DMA_COPY(adc->dmachannel_ADC_FIFO1 , scat_offset, &(adc->regmap->fifo1_data), p_dlen);
			break;

		case	ADC_FIFO2:
			status = SYS_DMA_COPY(adc->dmachannel_ADC_FIFO2 , scat_offset, &(adc->regmap->fifo2_data), p_dlen);
			break;

		case	ADC_FIFO3:
			status = SYS_DMA_COPY(adc->dmachannel_ADC_FIFO3 , scat_offset, &(adc->regmap->fifo3_data), p_dlen);
			break;

	}
	DRV_ADC_IOCTL(adc,ADC_GET_DMA_RD, &dma_en);
	if(!((1<<channel)&dma_en))//jason150327 for ASIC Simulation Error
	{
		dma_en |= (1<<channel);
		DRV_ADC_IOCTL(adc,ADC_SET_DMA_RD, &dma_en);
	}

#endif

	if(wait > 0)
	{
		/*
		uxBits = xEventGroupWaitBits(
				xEventGroup,        : The event group being tested.
				BIT_0 | BIT_4,      : The bits within the event group to wait for.
				pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
				pdFALSE,            : Don't wait for both bits, either bit will do.
				xTicksToWait )
		*/
		unsigned long	mask_evt;
		mask_evt = xEventGroupWaitBits((adc->mutex), ADC_DMA_HISR_DONE, pdTRUE, pdFALSE, wait);
		if( (mask_evt & ADC_DMA_HISR_DONE) != (ADC_DMA_HISR_DONE) )
		{
			PRINT_ADC("[%s] wait timeout !!!!! \r\n",__func__);
		}

		//Clear

		if(masked_evt & ADC_DMA_HISR_DONE) 
		{

		}
		for(unsigned int i=0; i<stuff_len; i++) //copy stuffing bytes
		{
			switch(channel){
				case    ADC_FIFO0:
					*(((unsigned short *)scat_offset)+i) = adc->regmap->fifo0_data;
					break;

				case    ADC_FIFO1:
					*(((unsigned short *)scat_offset)+i) = adc->regmap->fifo1_data;
					break;

				case    ADC_FIFO2:
					*(((unsigned short *)scat_offset)+i) = adc->regmap->fifo2_data;
					break;

				case    ADC_FIFO3:
					*(((unsigned short *)scat_offset)+i) = adc->regmap->fifo3_data;
					break;
			}
		}
	}

	return TRUE;
}

int DRV_ADC_READ_DMA_MULTI(HANDLE handler,  UINT32 channel, UINT16 **p_data, UINT32 p_dlen, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE    *adc;
	unsigned int status;
	UINT32  scat_len ,stuff_len;
	VOID   *scat_offset[4] ;
	UNSIGNED masked_evt ;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	if(channel > 0x1f0 || !(channel&0xff))
	{
		DRIVER_DBG_ERROR("illegal unit,\r\n");
		return FALSE;
	}

//	data = (unsigned char*)p_data;

	if(p_dlen == 0){
		return FALSE;
	}

	scat_offset[0] = p_data[0] ;
	scat_offset[1] = p_data[1] ;
	scat_offset[2] = p_data[2] ;
	scat_offset[3] = p_data[3] ;

	do{
		stuff_len = p_dlen % 2;
		p_dlen = (p_dlen/2)*2;
		if(p_dlen > (DMA_SCAT_LEN/2))
		{
			scat_len = (DMA_SCAT_LEN/2);
			p_dlen -= (DMA_SCAT_LEN/2);
		}
		else
		{
			scat_len = p_dlen;
			p_dlen = 0;
		}

		// DMA Action

		//Clear

		status = 0xffffffff;

		if(channel & 0x11)
		{
			status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO0, scat_offset[0], (UINT32 *)(&(adc->regmap->fifo0_data)), scat_len);
		}
		if(channel & 0x22)
		{
			status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO1 , scat_offset[1], (UINT32 *)(&(adc->regmap->fifo1_data)), scat_len);
		}
		if(channel & 0x44)
		{
			status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO2 , scat_offset[2], (UINT32 *)(&(adc->regmap->fifo2_data)), scat_len);
		}
		if(channel & 0x88)
		{
			status = (unsigned int)SYS_DMA_COPY(adc->dmachannel_ADC_FIFO3 , scat_offset[3], (UINT32 *)(&(adc->regmap->fifo3_data)), scat_len);
		}

		if(status != 0){
			p_dlen = 0;
			continue;
		}
		else
		{
#if 0// it's different from original dma read
			DRV_ADC_IOCTL(adc,ADC_GET_DMA_RD, &dma_en);
			if(!dma_en)//jason150327 for ASIC Simulation Error
			{
				dma_en = TRUE;
				DRV_ADC_IOCTL(adc,ADC_SET_DMA_RD, &dma_en);
			}
#else
			DRV_ADC_IOCTL(adc,ADC_SET_DMA_RD, &channel);
								
#endif
		}

		// Post Action
		scat_offset[0] =  (VOID *)((UINT32)scat_offset[0] + scat_len) ;
		scat_offset[1] =  (VOID *)((UINT32)scat_offset[1] + scat_len) ;
		scat_offset[2] =  (VOID *)((UINT32)scat_offset[2] + scat_len) ;
		scat_offset[3] =  (VOID *)((UINT32)scat_offset[3] + scat_len) ;

	}while(p_dlen > 0);

	/*
	uxBits = xEventGroupWaitBits(
					xEventGroup,        : The event group being tested.
					BIT_0 | BIT_4,      : The bits within the event group to wait for.
					pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
					pdFALSE,            : Don't wait for both bits, either bit will do.
					xTicksToWait )
	*/
	unsigned long   mask_evt;
	mask_evt = xEventGroupWaitBits((adc->mutex), ADC_DMA_HISR_DONE, pdTRUE, pdFALSE, 200);
	if( (mask_evt & ADC_DMA_HISR_DONE) != (ADC_DMA_HISR_DONE) )
	{
		PRINT_ADC("[%s] wait timeout !!!!! \r\n",__func__);
	}

	//Clear

	if(masked_evt & ADC_DMA_HISR_DONE) 
	{

	}

	for(unsigned int i=0; i<stuff_len; i++) //copy stuffing bytes
	{
		if(channel & 0x11)
		{
			*(((unsigned short *)scat_offset[0])+i) = adc->regmap->fifo0_data;
		}

		if(channel & 0x22)
		{
			*(((unsigned short *)scat_offset[1])+i) = adc->regmap->fifo1_data;
		}

		if(channel & 0x44)
		{
			*(((unsigned short *)scat_offset[2])+i) = adc->regmap->fifo2_data;
		}

		if(channel & 0x88)
		{
			*(((unsigned short *)scat_offset[3])+i) = adc->regmap->fifo3_data;
		}
	}

	return TRUE;
}


int	DRV_ADC_STOP(HANDLE handler, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE    *adc;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	adc->regmap->axadc12B_ctrl |= 0x3;

	return TRUE;
}


int	DRV_ADC_CLOSE(HANDLE handler)
{
	ADC_HANDLER_TYPE	*adc;
//	UNSIGNED 			masked_evt;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	if ( _adc_instance[adc->dev_unit] > 0) {
		_adc_instance[adc->dev_unit] -- ;

		if(_adc_instance[adc->dev_unit] == 0) {

			adc->regmap->adc_en_ch &= ~(0xff);

//			DA16200_BOOTCON->CLK_EN &= ~(0x100);
//			DA16200_BOOTCON->CLK_EN &= ~(0x80);
//jason171221		DA16200_BOOTCON->CLK_EN &= ~((0x1<<CLKEN_ADC12B) ) ;//ADC jason170213

			adc->regmap->axadc12B_ctrl |= 0x3;
		}
	}

	SYS_DMA_RELEASE_CHANNEL(adc->dmachannel_ADC_FIFO0);
	SYS_DMA_RELEASE_CHANNEL(adc->dmachannel_ADC_FIFO1);
	SYS_DMA_RELEASE_CHANNEL(adc->dmachannel_ADC_FIFO2);
	SYS_DMA_RELEASE_CHANNEL(adc->dmachannel_ADC_FIFO3);
	
	vEventGroupDelete((EventGroupHandle_t)(adc->mutex));

	vPortFree(handler);
//jason171221    FC9000_CLKGATE_OFF(clkoff_AuxAdc);

	DA16200_SYSCLOCK->CLK_EN_AUXA = 0x0;
	DA16200_SYSCLOCK->PLL_CLK_EN_6_AUXA = 0x0;

	return TRUE;
}


int DRV_ADC_ENABLE_CHANNEL(HANDLE handler,  UINT32 channel, unsigned int sel_adc, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE	*adc;
	
	if(handler == NULL || channel > 3){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	if(sel_adc == 12)
	{
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch & 0xff;
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch & ~(1 << channel);
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch | (1 << (channel+4));
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch | (0x100);
	}
	else if(sel_adc == 0)
	{
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch & 0xff;
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch & ~((1 << (channel+4)) | (1 << channel));
		adc->regmap->adc_en_ch = adc->regmap->adc_en_ch | (0x100);
	}

	
//If ADC IP Power Control
		
	adc->regmap_rtc->auxadc_control = (0x1582);//Reset High
	
	PRINT_ADC("adc_en_ch : 0x%08x\r\n",adc->regmap->adc_en_ch);
	PRINT_ADC("AuxADC12_Cntl : 0x%08x\r\n",adc->regmap_rtc->auxadc_control);
			
	return TRUE;
}

int DRV_ADC_SET_INTERRUPT(HANDLE handler, UINT32 channel, UINT32 enable, UINT32 type, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE	*adc;
	
	if(handler == NULL || channel > 3){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(type)
	{
		case ADC_INTERRUPT_FIFO_HALF:
			if(enable == 1)
				adc->regmap->xadc_intr_ctrl_fifo |= (1<<(channel));
			else
				adc->regmap->xadc_intr_ctrl_fifo &= ~(1<<(channel));
			break;
			
		case ADC_INTERRUPT_FIFO_FULL:
			if(enable == 1)
				adc->regmap->xadc_intr_ctrl_fifo |= (1<<(channel+4));
			else
				adc->regmap->xadc_intr_ctrl_fifo &= ~(1<<(channel+4));
			break;
			
		case ADC_INTERRUPT_THD_OVER:
			if(enable == 1)
				adc->regmap->xadc_thr_intr_mask |= (1<<(channel + ADC_BIT_POS_THD_OVER_INTR_SET));
			else
				adc->regmap->xadc_thr_intr_mask &= ~(1<<(channel+ ADC_BIT_POS_THD_OVER_INTR_SET));
			break;
				
		case ADC_INTERRUPT_THD_UNDER:
			if(enable == 1)
				adc->regmap->xadc_thr_intr_mask |= (1<<(channel+ADC_BIT_POS_THD_UNDER_INTR_SET));
			else
				adc->regmap->xadc_thr_intr_mask &= ~(1<<(channel+ADC_BIT_POS_THD_UNDER_INTR_SET));
			break;
				
		case ADC_INTERRUPT_THD_DIFF:
			if(enable == 1)
				adc->regmap->xadc_thr_intr_mask |= (1<<(channel+ADC_BIT_POS_THD_DIFF_INTR_SET));
			else
				adc->regmap->xadc_thr_intr_mask &= ~(1<<(channel+ADC_BIT_POS_THD_DIFF_INTR_SET));
			break;
				
		case ADC_INTERRUPT_ALL:
			if(enable == 1)
			{
				adc->regmap->xadc_intr_ctrl_fifo = 0xff;
				adc->regmap->xadc_thr_intr_mask = 0xfff;
			}
			else
			{
				adc->regmap->xadc_intr_ctrl_fifo = 0;
				adc->regmap->xadc_thr_intr_mask = 0;
			}
			break;
						
		default :
			break;
								

	}

	return TRUE;
}

int DRV_ADC_SET_THD_VALUE(HANDLE handler, UINT32 type, UINT32 thd, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	ADC_HANDLER_TYPE	*adc;
	
	if(handler == NULL ){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(type)
	{
		case ADC_THRESHOLD_TYPE_12B_OVER:
			adc->regmap->adc12B_intr_thr_over = (thd & 0xffff) ;
			break;

		case ADC_THRESHOLD_TYPE_12B_UNDER:
			adc->regmap->adc12B_intr_thr_under= (thd & 0xffff) ;
			break;
		
		case ADC_THRESHOLD_TYPE_12B_DIFF:
			adc->regmap->adc12B_intr_thr_diff = (thd & 0xffff);
			break;
		
		default:
			break;
	
	}
	
	return TRUE;
}

int	DRV_ADC_WAIT_INTERRUPT(HANDLE handler, UNSIGNED *mask_evt)
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
	*mask_evt = xEventGroupWaitBits((g_drv_adc->mutex), 0xffffff, pdTRUE, pdFALSE, portMAX_DELAY);
	if( !(*mask_evt))
	{
		//Todo : Fill Code Here for Managing Abnormal Cases
//		PRINTF("host_rx_thread NO EVENTS\n");
		return -1;
	}
	
	//Clear

	return TRUE;
}

int	DRV_ADC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
#if 1
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(cmd){
		case ADC_SET_CONFIG:
			break;
		case ADC_GET_CONFIG:
//			*((UINT32 *)data) =  HW_REG_READ32_ADC( adc->regmap->adc_cr0);
			break;



		case ADC_GET_STATUS:
#if 0
			if( data != NULL ){
				*((UINT32 *)data) = HW_REG_READ32_ADC( adc->regmap->adc_sr);
			}else{
				while( (HW_REG_READ32_ADC(adc->regmap->adc_sr) & ADC_STATUS_TFE) != ADC_STATUS_TFE ){
					//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(adc->mutex), (adc_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
					//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(adc->mutex), ~(adc_DMA_HISR_DONE) , OAL_AND );								// Clear
					OAL_MSLEEP(1);
				}
			}
#endif            
			break;

		case ADC_SET_DMA_RD:
			HW_REG_OR_WRITE32_ADC( adc->regmap->adc_dmaen, *(UINT32 *)data ) ;
			break;

		case ADC_UNSET_DMA_RD:
			HW_REG_AND_WRITE32_ADC( adc->regmap->adc_dmaen, (~(*(UINT32 *)data)) ) ;
			break;

		case ADC_GET_DMA_RD:
			*((UINT32 *)data) = HW_REG_READ32_ADC(adc->regmap->adc_dmaen)&0xf;
			break;

		case ADC_SET_RESET:
			if( *((UINT32 *)data) == TRUE ){
//				HW_REG_WRITE32_ADC( adc->regmap->adc_dmacr,  0 ) ;

			}else{
//				HW_REG_AND_WRITE32_ADC(	adc->regmap->adc_cr0, (~ADC_CFG_POWER_DOWN) );


			}
			break;

		case ADC_SET_HISR_CALLACK:
//			drv_adc_callback_registry( adc, ((UINT32 *)data)[0], ((UINT32 *)data)[1], ((UINT32 *)data)[2] );
			break;

		case ADC_SET_INTERRUPT_MODE:
			{
				g_drv_adc->interrupt_mode = *((UINT32 *)data);
			}
			break;
				
		case ADC_GET_INTERRUPT_MODE:
			{
				*((UINT32 *)data) = g_drv_adc->interrupt_mode; 
			}
			break;
		case ADC_ENABLE_INTERRUPT:
			{
				HW_REG_WRITE32_ADC( adc->regmap->xadc_intr_ctrl_fifo, *((UINT32 *)data)); 
			}
			break;
		case ADC_SET_PORT:
			{
				switch(adc->dev_unit){
					case	ADC_FIFO0:
								adc->regmap->axadc12B_ctrl &= ~(0xc);
								adc->regmap->axadc12B_ctrl |= ((*((UINT32 *)data))<< 2);
							break;
					
					default:
							DRIVER_DBG_ERROR("illegal unit, \r\n");
				}
			}
			break;
			
			
		case ADC_SET_CHANNEL:
			break;
		
		case ADC_GET_CHANNEL:
			*((UINT32 *)data) = adc->regmap->adc_en_ch;
			break;
		
		case ADC_GET_STEP:
			*((UINT32 *)data) = adc->regmap->fxadc_step_sample;
			break;
			
		case ADC_SET_STEP:
			adc->regmap->fxadc_step_sample = (*((UINT32 *)data))<<4; //[0:3] 16b, [4:7] 12b,   0,1 every ready capture data,  2~ every n step capture data
			break;
				
		case ADC_GET_MODE_TIMESTAMP:
			*((UINT32 *)data) = adc->use_timestamp;
			break;
		case ADC_SET_DATA_MODE:
			if(*((UINT32 *)data))
			{
				*((UINT32 *)data) = adc->regmap->axadc12B_ctrl |= 0x100 ;
			}
			else
			{
				*((UINT32 *)data) = adc->regmap->axadc12B_ctrl &= ~(0x100) ;
			}
			break;
			
		case ADC_GET_RESOLUTION:
			*((UINT32 *)data) = adc->regmap_rtc->auxadc_control>>14;
			break;
			
		case ADC_SET_RESOLUTION:
			adc->regmap_rtc->auxadc_control =  adc->regmap_rtc->auxadc_control & ~(0xc000);
			adc->regmap_rtc->auxadc_control =  adc->regmap_rtc->auxadc_control | ((*((UINT32 *)data) & 0x3)<<14);
			PRINT_ADC("control reg = 0x%x\r\n",adc->regmap_rtc->auxadc_control);
			break;


		case  ADC_GET_RTC_THD0:
			*((UINT32 *)data) = adc->regmap_rtc->auxadc_threshold_lev0_1 & 0xfff;
			break;
		case  ADC_SET_RTC_THD0:
			adc->regmap_rtc->auxadc_threshold_lev0_1 &= ~(0xfff);
			adc->regmap_rtc->auxadc_threshold_lev0_1 |= (*((UINT32 *)data)& 0xfff);
			break;
		
		case  ADC_GET_RTC_COMPARE_MODE:
			*((UINT32 *)data) = (adc->regmap_rtc->auxadc_threshold_lev0_1 & 0x3000);
			break;
			
		case  ADC_SET_RTC_COMPARE_MODE:
			adc->regmap_rtc->auxadc_threshold_lev0_1 &= ~(0x3000);
			adc->regmap_rtc->auxadc_threshold_lev0_1 |= (*((UINT32 *)data));
			break;
		
		case  ADC_GET_RTC_CYCLE_BEFORE_ON:
			*((UINT32 *)data) = (adc->regmap_rtc->auxadc_timer & 0xf0000)>>16;
			break;
		case  ADC_SET_RTC_CYCLE_BEFORE_ON:
			adc->regmap_rtc->auxadc_timer &= ~(0xf0000);
			adc->regmap_rtc->auxadc_timer |= ((*((UINT32 *)data))<<16)&0xf0000;
			break;
		
		case  ADC_GET_RTC_CYCLE_BEFORE_CAPTURE:
			*((UINT32 *)data) = (adc->regmap_rtc->auxadc_timer & 0x3f00)>>8;
			break;
		case  ADC_SET_RTC_CYCLE_BEFORE_CAPTURE:
			if((*((UINT32 *)data)) < 1)
				(*((UINT32 *)data)) = 1;
			adc->regmap_rtc->auxadc_timer &= ~(0x3f00);
			adc->regmap_rtc->auxadc_timer |= ((*((UINT32 *)data))<<8)&0x3f00;
			break;
		
		case  ADC_ENABLE_RTC_SENSOR:
			adc->regmap_rtc->auxadc_timer |= 0x80;
			break;
		case  ADC_DISABLE_RTC_SENSOR:
			adc->regmap_rtc->auxadc_timer &= ~(0x80);
			break;
		
		case  ADC_SET_RTC_TIMER1:
			adc->regmap_rtc->auxadc_timer &= ~(0x70);
			adc->regmap_rtc->auxadc_timer |= ((*((UINT32 *)data))<<4)&0x70;
			break;
			
		case  ADC_SET_RTC_TIMER2:
			if((*((UINT32 *)data)) < 1)
					(*((UINT32 *)data)) = 1;
			adc->regmap_rtc->auxadc_timer &= ~(0xf);
			adc->regmap_rtc->auxadc_timer |= ((*((UINT32 *)data))<<0)&0xf;
			break;
		
		case  ADC_SET_CAPTURE_STEP:
			adc->regmap_rtc->auxadc_sample_num &= ~(0xf8);
			adc->regmap_rtc->auxadc_sample_num |= ((*((UINT32 *)data))<<3)&0xf8;
			break;
		
		case  ADC_SET_NUMBER_SAMPLES: //(2^(n+2))
			adc->regmap_rtc->auxadc_sample_num &= ~(0x7);
			adc->regmap_rtc->auxadc_sample_num |= ((*((UINT32 *)data))<<0)&0x7;
			break;
		
		default:
			break;
	}
#endif
	return TRUE;
}

unsigned int x12_clk_table[8] = 
{
	4, 16, 64, 256, 1024, 4096, 16384, 65536
};

int	DRV_ADC_SET_THRESHOLD(HANDLE handler, UNSIGNED channel, UNSIGNED threshold,  UNSIGNED mode)
{
	
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}
	
	adc = (ADC_HANDLER_TYPE *)handler ;

	switch(channel)

	{
		case 0:
			adc->regmap_rtc->auxadc_threshold_lev0_1 &= ~(0xffff);
			adc->regmap_rtc->auxadc_threshold_lev0_1 |=  (mode) | (threshold & 0xfff);
			break;

		case 1:
			adc->regmap_rtc->auxadc_threshold_lev0_1 &= ~(0xffff0000);
			adc->regmap_rtc->auxadc_threshold_lev0_1 |=  ((mode << 16) | ((threshold & 0xfff)<<16));
			break;

		case 2:
			adc->regmap_rtc->auxadc_threshold_lev2_3 &= ~(0xffff);
			adc->regmap_rtc->auxadc_threshold_lev2_3 |=  (mode) | (threshold & 0xfff);
			break;
	
		case 3:
			adc->regmap_rtc->auxadc_threshold_lev2_3 &= ~(0xffff0000);
			adc->regmap_rtc->auxadc_threshold_lev2_3 |=  ((mode << 16) | ((threshold & 0xfff)<<16));
			break;
	}

	return 0;
}

int	DRV_ADC_SET_DELAY_AFTER_WKUP(HANDLE handler, UNSIGNED delay1, UNSIGNED delay2)
{
	
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}
	
	adc = (ADC_HANDLER_TYPE *)handler ;

	adc->regmap_rtc->auxadc_timer = (adc->regmap_rtc->auxadc_timer & ~(0xf0000)) | ((delay1 << 16)&0xf0000);
	adc->regmap_rtc->auxadc_timer = (adc->regmap_rtc->auxadc_timer & ~(0x3f00)) | ((delay2 << 8)&0x3f00);
	
	return 0;
}

int	DRV_ADC_SET_RTC_ENABLE_CHANNEL(HANDLE handler, UNSIGNED ch, UNSIGNED enable)
{
	
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}
	
	adc = (ADC_HANDLER_TYPE *)handler ;

	adc->regmap_rtc->auxadc_timer |=  (1<<28);
	adc->regmap_rtc->auxadc_timer = adc->regmap_rtc->auxadc_timer & ~(1<<(ch+24));
	adc->regmap_rtc->auxadc_timer = adc->regmap_rtc->auxadc_timer | (enable<<(ch+24));

	if(adc->regmap_rtc->auxadc_timer & 0xf000000)
		adc->regmap_rtc->auxadc_timer |= 0x80;
	else
		adc->regmap_rtc->auxadc_timer &= ~(0x80);

	return 0;
}


int	DRV_ADC_SET_SLEEP_MODE(HANDLE handler, UNSIGNED	x12_clk, UNSIGNED reg_ax12b_timer, UNSIGNED adc_step)
{
	
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}

	
	adc = (ADC_HANDLER_TYPE *)handler ;
	adc->regmap_rtc->auxadc_timer = (adc->regmap_rtc->auxadc_timer & ~(0x70)) | ((x12_clk << 4)&0x70);
	adc->regmap_rtc->auxadc_timer = (adc->regmap_rtc->auxadc_timer & ~(0xf)) | ((reg_ax12b_timer << 0)&0xf);
	adc->regmap_rtc->auxadc_sample_num = (adc->regmap_rtc->auxadc_sample_num & ~(0x7)) | adc_step;
	adc->regmap_rtc->auxadc_timer |= 0x80;

	return 0;
}

void	adc_drv_interrupt(void)
{
	INTR_CNTXT_CALL(adc_drv_interrupt_handler);
}

void adc_drv_interrupt_handler(void)
{
	unsigned int 	status = 0;
	
	status = g_drv_adc->regmap->xadc_intr_status;
	
	if(status & 0xfff00)
	{
		g_interrupt_cnt++;
		if(g_drv_adc->interrupt_mode & ADC_INTERRUPT_MODE_MASK)
			g_drv_adc->regmap->xadc_thr_intr_mask = 0;
		
		g_drv_adc->regmap->xadc_thr_intr_clr = 0xfff;
		if(g_drv_adc->interrupt_mode & ADC_INTERRUPT_MODE_EVENT)
		{
			BaseType_t xHigherPriorityTaskWoken, xResult;
			xHigherPriorityTaskWoken = pdFALSE;
			xResult = xEventGroupSetBitsFromISR(g_drv_adc->mutex, (status), &xHigherPriorityTaskWoken);

		}
	}
	
	if(status & 0xff)
	{
		g_interrupt_cnt++;
			g_drv_adc->regmap->xadc_intr_ctrl_fifo = 0;
		if(g_drv_adc->interrupt_mode & ADC_INTERRUPT_MODE_EVENT)
		{
			BaseType_t xHigherPriorityTaskWoken, xResult;
			xHigherPriorityTaskWoken = pdFALSE;
			xResult = xEventGroupSetBitsFromISR(g_drv_adc->mutex, (status), &xHigherPriorityTaskWoken);

		}
	}
	
}

//---------------------------------------------------------
// DRIVER :: Callback
//---------------------------------------------------------
#if 0	//unused function
static int	 drv_adc_callback_registry(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param)
{
	ADC_HANDLER_TYPE	*adc;

	if(handler == NULL){
		return FALSE;
	}

	adc = (ADC_HANDLER_TYPE *) handler;

	if( vector <= ADC_CB_MAX ){
		adc->callback[vector] = (USR_CALLBACK)callback ;
		adc->cb_param[vector] = (VOID *)param ;
	}

	return TRUE;
}
#endif /*0*/


static void drv_adc_term0(void *param)
{
	ADC_HANDLER_TYPE	*adc;
	unsigned int data = 0x1;

	if( param == NULL){
		return ;
	}
	adc = (ADC_HANDLER_TYPE *) param;

	DRV_ADC_IOCTL(adc, ADC_UNSET_DMA_RD, &data);

	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(adc->mutex, (ADC_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
	if( xResult != pdFAIL )
	{
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		switch should be requested.  The macro used is port specific and will
		be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		the documentation page for the port being used. */
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
//    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));

	if(adc->callback[ADC_CB_TERM] != NULL){
		adc->callback[ADC_CB_TERM](adc->cb_param[ADC_CB_TERM]);
	}
}

static void drv_adc_term1(void *param)
{
	ADC_HANDLER_TYPE	*adc;
	unsigned int data = 0x2;

	if( param == NULL){
		return ;
	}
	adc = (ADC_HANDLER_TYPE *) param;

	DRV_ADC_IOCTL(adc, ADC_UNSET_DMA_RD, &data);

	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(adc->mutex, (ADC_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
	if( xResult != pdFAIL )
	{
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		switch should be requested.  The macro used is port specific and will
		be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		the documentation page for the port being used. */
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
//    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));

	if(adc->callback[ADC_CB_TERM] != NULL){
		adc->callback[ADC_CB_TERM](adc->cb_param[ADC_CB_TERM]);
	}
}

static void drv_adc_term2(void *param)
{
	ADC_HANDLER_TYPE	*adc;
	unsigned int data = 0x4;

	if( param == NULL){
		return ;
	}
	adc = (ADC_HANDLER_TYPE *) param;

	DRV_ADC_IOCTL(adc, ADC_UNSET_DMA_RD, &data);

	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(adc->mutex, (ADC_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
	if( xResult != pdFAIL )
	{
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		switch should be requested.  The macro used is port specific and will
		be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		the documentation page for the port being used. */
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
//    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));

	if(adc->callback[ADC_CB_TERM] != NULL){
		adc->callback[ADC_CB_TERM](adc->cb_param[ADC_CB_TERM]);
	}
}

static void drv_adc_term3(void *param)
{
	ADC_HANDLER_TYPE	*adc;
	unsigned int data = 0x8;

	if( param == NULL){
		return ;
	}
	adc = (ADC_HANDLER_TYPE *) param;

	DRV_ADC_IOCTL(adc, ADC_UNSET_DMA_RD, &data);

	BaseType_t xHigherPriorityTaskWoken, xResult;
	xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(adc->mutex, (ADC_DMA_HISR_DONE), &xHigherPriorityTaskWoken);
	if( xResult != pdFAIL )
	{
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
		switch should be requested.  The macro used is port specific and will
		be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
		the documentation page for the port being used. */
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
//    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));

	if(adc->callback[ADC_CB_TERM] != NULL){
		adc->callback[ADC_CB_TERM](adc->cb_param[ADC_CB_TERM]);
	}
}

static void drv_adc_empty(void *param)
{
	ADC_HANDLER_TYPE	*adc;

	if( param == NULL){
		return ;
	}
	adc = (ADC_HANDLER_TYPE *) param;

	//OAL_SET_EVENTS_HISR((OAL_EVENT_GROUP *)(adc->mutex), (adc_DMA_HISR_DONE) , OAL_OR );

	if(adc->callback[ADC_CB_EMPTY] != NULL){
		adc->callback[ADC_CB_EMPTY](adc->cb_param[ADC_CB_EMPTY]);
	}
}

static void drv_adc_error(void *param)
{
	DA16X_UNUSED_ARG(param);
}

