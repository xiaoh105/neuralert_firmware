/**
 ****************************************************************************************
 *
 * @file sys_kdma.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "oal.h"
#include "portmacro.h"
#include "driver.h"
#include "common_def.h"

#ifdef	SUPPORT_KDMA
//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

#undef DMA_DEBUG_ON

#ifdef DMA_DEBUG_ON
#define PRINT_DMA(...) 		Printf( __VA_ARGS__ )
#else
#define PRINT_DMA(...)
#endif

//#define UDMA_PRINTF(...)	DRV_PRINTF( __VA_ARGS__ )
#define UDMA_PRINTF(...)	Printf( __VA_ARGS__ )

#define	CONVERT_MEM2KDMA(x)	x	//CONVERT_C2SYS_MEM( x )
#define	CONVERT_UDMA2MEM(x)	x	//CONVERT_SYS2C_MEM( x )


#define	DMA_SCAT_TRANSFER	(2048)
//======================================================================
//
//======================================================================

#define DA16200_KDMA_REG     ((KDMA_REGMAP *)DA16200_KDMA_BASE)

int dma_done_irq_occurred[MAX_NUM_OF_DMA_CHANNELS];
int dma_done_irq_occurred_all = 0;
volatile int dma_done_irq_expected;
volatile int dma_error_irq_occurred;
volatile int dma_error_irq_expected;


kdma_data_structure *dma_data;
unsigned int *kdma_descriptor_ptr;

volatile unsigned int source_data_array[4];	/* Data array for memory DMA test */
volatile unsigned int dest_data_array[4];	/* Data array for memory DMA test */

EventGroupHandle_t	sys_kdma_evt;

unsigned int wait_int_flag = 0, wait_handler_flag = 0;
unsigned int g_dma_usage_flag = 0;

DMA_HANDLE_TYPE dma_handle[MAX_NUM_OF_DMA_CHANNELS];
kdma_channel_data dma_garbage_head[MAX_NUM_OF_DMA_CHANNELS];
kdma_channel_data dma_run_desc_head[MAX_NUM_OF_DMA_CHANNELS];

unsigned int	g_link_alloc_count = 0, g_link_free_count = 0;

//======================================================================
//
//======================================================================


/* --------------------------------------------------------------- */
/*  Initialize DMA data structure                                  */
/* --------------------------------------------------------------- */

//__attribute__ ((section(".rw_sec_dma"),used)) static unsigned int dma_data_ptr[256];

void dma_data_struct_init(void)
{
	int	i;	/* loop counter */

	/* Set pointer to the reserved space */
	//dma_data = (kdma_data_structure *) ptr;
	//dma_data = (kdma_data_structure *) dma_data_ptr;
	dma_data = (kdma_data_structure *) DA16X_SRAM_BASE;

	//PRINT_DMA ("dma structure block address = 0x%x\n", ptr);
	//PRINT_DMA ("dma structure block address = 0x%x\n", dma_data_ptr);
	PRINT_DMA ("dma structure block address = 0x%x\n", dma_data);

	for (i=0; i<MAX_NUM_OF_DMA_CHANNELS; i++) {
		dma_data->Primary[i].command = 0;
		dma_data->Primary[i].src_start_addr = 0;
		dma_data->Primary[i].dst_start_addr = 0;
		dma_data->Primary[i].next_descriptor= 0;
	}
	kdma_descriptor_ptr = (unsigned int *)(&(dma_data->Primary[16].command));
	
	for (i=0; i<MAX_NUM_OF_DMA_CHANNELS; i++) 
	{
		kdma_descriptor_ptr[i] = (unsigned int)(&(dma_data->Primary[i].command));
	}
	memset(dma_garbage_head, 0, sizeof(kdma_channel_data)*MAX_NUM_OF_DMA_CHANNELS);	//jason160312
	memset(dma_handle, 0, sizeof(DMA_HANDLE_TYPE)*MAX_NUM_OF_DMA_CHANNELS);			//jason160312
//	memset(dma_descriptor,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);					//jason160312
//	memset(exceed_block,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);						//jason160312
//	memset(num_blocks_e,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);						//jason160312
//	memset(block_cnt_e,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);						//jason160312

	return;
}


/**
 ****************************************************************************************
 * @brief Initialize PrimeCell PL-230 DMA
 *
 * This primitive set DMA, DMA Interrupt, DMA concerned flags
 *
 *
 * @return NULL
 ****************************************************************************************
 */

void	_sys_dma_init( void )
{
	PRINT_DMA ("Initialize KDMA");

	DA16X_CLKGATE_ON(clkoff_kdma);

	dma_data_struct_init();

	DA16200_BOOTCON->sel_uDMA_kDMA = 0x1; // Enable KDMA
	
	DA16200_KDMA_REG->dma_enable = 0;	/* Disable DMA controller for initialization */
	DA16200_KDMA_REG->dma_enable = 1;	/* Disable DMA controller */
	DA16200_KDMA_REG->cfg_descriptor_addr = CONVERT_MEM2KDMA(((unsigned int) &(dma_data->Primary[16].command))); /* Set DMA data structure address */
	DA16200_KDMA_REG->cfg_channel_enable = 0x0; /* Disable all channels */

	sys_kdma_evt = xEventGroupCreate();

	wait_int_flag = 0; wait_handler_flag = 0;

	_sys_nvic_write(kDMA_IRQn, (void *)_sys_kdma_interrupt, 0x7);

//	NVIC_ClearPendingIRQ(kDMA_IRQn);
//	NVIC_EnableIRQ(kDMA_IRQn);

	return;
}

/**
 ****************************************************************************************
 * @brief DeInitialize PrimeCell PL-230 DMA
 *
 * This primitive clear DMA, DMA Interrupt, DMA concerned flags
 *
 * @return NULL
 ****************************************************************************************
 */
void	_sys_dma_deinit(void)
{
	unsigned int current_state;
	PRINT_DMA ("Deinit KDMA");
	current_state = DA16200_KDMA_REG->status_dma;

	/* Wait until current DMA complete */
	current_state = (DA16200_KDMA_REG->status_dma >> 16) & 0xff;
	if (!((current_state==0) || (current_state==0x8) || (current_state==0x9))) {
		PRINT_DMA ("- wait for DMA IDLE/STALLED/DONE");
		current_state = (DA16200_KDMA_REG->status_dma >> 16) & 0xff;
		PRINT_DMA ("- Current status        : %x\n",current_state);

	}
	while (!((current_state==0) || (current_state==0x8) || (current_state==0x9))){
		/* Wait if not IDLE/STALLED/DONE */
		current_state = (DA16200_KDMA_REG->status_dma >> 16) & 0xff;
		PRINT_DMA ("- Current status        : %x\n",current_state);
	}
	DA16200_KDMA_REG->dma_enable = 0; /* Disable DMA controller for initialization */
	DA16200_KDMA_REG->cfg_descriptor_addr = 0; /* Set DMA data structure address */
	DA16200_KDMA_REG->cfg_channel_enable = 0x0; /* Disable all channels */

	vEventGroupDelete(sys_kdma_evt);

//	NVIC_DisableIRQ(kDMA_IRQn);
	_sys_nvic_write(kDMA_IRQn, NULL, 0x7);

	DA16X_CLKGATE_OFF(clkoff_kdma);

	return;
}

/**
 ****************************************************************************************
 * @brief Enable DMA Channel with input parameter [ch]
 *
 * This primitive Enable DMA Channel with input parameter [ch]
 *
 * @param[in] ch        channel to be enabled
 *
 * @return NULL
 ****************************************************************************************
 */
int	_sys_kdma_open( unsigned int ch )
{
//	PRINT_DMA ("Open KDMA channel enabled : 0x%x\n",DA16200_KDMA_REG->cfg_channel_enable);
	if(DA16200_KDMA_REG->cfg_channel_enable & (1<<ch)){
		return -1;
	}

	DA16200_KDMA_REG->cfg_channel_enable |= (1<<ch); /* Enable channel */

//	PRINT_DMA ("- channel enabled : 0x%x\n",DA16200_KDMA_REG->cfg_channel_enable);

	return 0;
}

/**
 ****************************************************************************************
 * @brief Disable DMA Channel with input parameter [ch]
 *
 * This primitive Disnable DMA Channel with input parameter [ch]
 *
 * @param[in] ch        channel to be disabled
 *
 * @return NULL
 ****************************************************************************************
 */
int	_sys_kdma_close( unsigned int ch )
{
	unsigned int enabled;
	UDMA_PRINTF ("Close KDMA channel");
	enabled = DA16200_KDMA_REG->cfg_channel_enable;
	if((enabled & (1<<ch)) == 0)
		return -1;

	PRINT_DMA ("- channel disabled : 0x%x\n",ch);

	DA16200_KDMA_REG->cfg_channel_enable &= ~(1<<ch); /* Disable channel */

	return 0;
}

/**
 ****************************************************************************************
 * @brief Wait DMA Interrupt Event
 *
 * This primitive wait for DMA interrupt event form DMA interrupt handler
 *
 * @param[in] ch        channel to wait
 * @param[in] wait_time        Max wait time for issued DMA interrupt
 *
 * @return NULL
 ****************************************************************************************
 */
int	_sys_kdma_wait( unsigned int ch, unsigned int wait_time )
{
	unsigned int	enabled;
	unsigned int	mask_evt;
	int ret = 0;
	enabled = DA16200_KDMA_REG->cfg_channel_enable;
	if((enabled & (1<<ch)) == 0)
		return -1;

	wait_int_flag |= (1<<ch);

//	PRINT_DMA ("- start to wait dma interrupt event : 0x%x\n",enabled);

/*
	uxBits = xEventGroupWaitBits(
			xEventGroup,		: The event group being tested.
			BIT_0 | BIT_4,		: The bits within the event group to wait for.
			pdTRUE,				: BIT_0 & BIT_4 should be cleared before returning.
			pdFALSE,			: Don't wait for both bits, either bit will do.
			xTicksToWait )
*/
	mask_evt = xEventGroupWaitBits(sys_kdma_evt, (1<<ch), pdTRUE, pdFALSE, wait_time);
	if( (mask_evt & (1<<ch)) != (unsigned int)(1<<ch) )
	{
		//Todo : Fill Code Here for Managing Abnormal Cases

		PRINT_DMA ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++wait dma interrupt event %d ticks timed out\n",wait_time);
		for(int i=0;i<20;i++) {
			PRINT_DMA("0x%x : 0x%08x\n", 0x4000f000+(i*4), *((unsigned int *)(0x4000f000+(i*4))));
		}

		for(int i=0;i<3;i++) {
			PRINT_DMA("0x%x : 0x%08x , 0x%x : 0x%08x\n", 0x20000000+(16*27+i*4), *((unsigned int *)(0x20000000+(16*27+i*4))), 0x20000200+(16*27+i*4), *((unsigned int *)(0x20000200+(16*27+i*4))));
		}

		_sys_kdma_close(ch);
		ret = -1;
	}

	return ret;
}

/**
 ****************************************************************************************
 * @brief Set Primary DMA Control Register
 *
 * This primitive set primary control register for a channel
 *
 * @param[in] ch        channel to be set
 * @param[in] size        width for dma channel, increment step
 * @param[in] num_word        total length for transfer / width
 * @param[in] cycle_ctrl        refer PL-230 UM
 * @param[in] r_pow        2^r_pow, burst size per 1 arbitration. max 10 (1024)
 *
 * @return NULL
 ****************************************************************************************
 */
void	_sys_kdma_set_control( unsigned int ch, unsigned int size, unsigned int num_word, unsigned int cycle_ctrl, unsigned int r_pow, void *desc )
{
	DA16X_UNUSED_ARG(ch);
	unsigned long control         = (size << 16) |  /* dst_inc */
		                            (size << 14) |  /* src_inc */
		                            (size << 1) |  /* tf_size */
		                            ((num_word)<< 3) | /* n_minus_1 */
		                            (cycle_ctrl    <<  0) ;  /* cycle_ctrl  */
		control |= (r_pow << 18);   /* R_power */

	((kdma_channel_data *)desc)->command = control;
}


/**
 ****************************************************************************************
 * @brief Set Primary DMA Source Address
 *
 * This primitive set primary source address
 *
 * @param[in] ch        channel to be set
 * @param[in] src        start address to be read. will be changed to end address in function.
 * @param[in] size        width for dma channel, increment step
 * @param[in] num_word        total length for transfer / width
 *
 * @return NULL
 ****************************************************************************************
 */
void	_sys_kdma_set_src_addr( unsigned int ch, unsigned int src, unsigned int size, unsigned int num_word, void *desc )
{
	DA16X_UNUSED_ARG(ch);
	DA16X_UNUSED_ARG(size);
	DA16X_UNUSED_ARG(num_word);
	((kdma_channel_data *)desc)->src_start_addr = CONVERT_MEM2KDMA(src);
}

/**
	****************************************************************************************
	* @brief Set Primary DMA Destination Address
	*
	* This primitive set primary destination address
	*
	* @param[in] ch        channel to be set
	* @param[in] dst        start address to be write. will be changed to end address in function.
	* @param[in] size        width for dma channel, increment step
	* @param[in] num_word        total length for transfer / width
	*
	* @return NULL
	****************************************************************************************
	*/
void	_sys_kdma_set_dst_addr( unsigned int ch, unsigned int dst, unsigned int size, unsigned int num_word, void *desc )
 {
	 DA16X_UNUSED_ARG(ch);
	 DA16X_UNUSED_ARG(size);
	 DA16X_UNUSED_ARG(num_word);
	 ((kdma_channel_data *)desc)->dst_start_addr = CONVERT_MEM2KDMA(dst);
 }

/**
 ****************************************************************************************
 * @brief Set Primary DMA Next Descriptor Address
 *
 * This primitive set primary next descriptor address
 *
 * @param[in] ch        channel to be set
 * @param[in] next     next descriptor address. 32-bit unsigned.
 *
 * @return NULL
 ****************************************************************************************
 */
void	_sys_kdma_set_next( unsigned int ch, unsigned int next )
{
	dma_data->Primary[ch].next_descriptor = next;
}

/**
 ****************************************************************************************
 * @brief Request Primary DMA to be Start
 *
 * This primitive request DMA channel to be started by SW
 *
 * @param[in] ch        channel to be started
 *
 * @return NULL
 ****************************************************************************************
 */
void	_sys_kdma_sw_request( unsigned int ch)
{
	DA16X_UNUSED_ARG(ch);
//	DA16200_KDMA_REG->sw_request = (1<<ch); /* request channel */
}
//jason160530
kdma_channel_data*	_sys_kdma_get_last_descriptor(unsigned int ch)
{
	kdma_channel_data *buffer = NULL, *next;//klocwork

	next = (kdma_channel_data *)(dma_garbage_head[ch].next_descriptor);

	while(next != NULL)
	{
		buffer = next;
		next = (kdma_channel_data *)(buffer->next_descriptor);
	}
	return buffer;
}

void	_sys_kdma_free_garbage(unsigned int ch)
{
	kdma_channel_data *buffer, *next;

	next = (kdma_channel_data *)(dma_garbage_head[ch].next_descriptor);

	NVIC_DisableIRQ(kDMA_IRQn);
	while((next != NULL) && ((unsigned int)next != next->next_descriptor) )
	{
		buffer = next;
		next = (kdma_channel_data *)(buffer->next_descriptor);
		vPortFree((void *)buffer);
		g_link_free_count++;
		PRINT_DMA("free count = %d\r\n",g_link_free_count);
	}
	dma_garbage_head[ch].next_descriptor = 0;
	NVIC_EnableIRQ(kDMA_IRQn);
}

/**
 ****************************************************************************************
 * @brief Memory Copy via DMA Transfer
 *
 * This primitive copy data from src to dst.
 * if size is bigger than 4099 using scattered method in PL-230.
 * if not, use only 1 primary descriptor
 *
 * @param[in] dest        start address to be write.
 * @param[in] src        start address to be read.
 * @param[in] len        total length to be copied
 * @param[in] wait_time        Max wait time for DMA done interrupt. if 0, do not wait for interrupt
*
 * @return NULL
 ****************************************************************************************
 */
int memcpy_dma (void *dest, void *src, unsigned int len, unsigned int wait_time)
{

	unsigned int	i;
	unsigned int dma_control, dma_config = 0, dma_maxlli = 0;


	DMA_HANDLE_TYPE *handle;
	if(len >= MEMCPY_DMA_MINIMUM_SIZE && ((unsigned int)dest)%4 == 0 && ((unsigned int)src)%4 == 0)
	{

		dma_control = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD) ;
//		_sys_kdma_set_control()

		dma_config = DMA_CONFIG_MEM2MEM;

		handle = SYS_DMA_OBTAIN_CHANNEL(2, dma_control, dma_config, dma_maxlli);
		SYS_DMA_COPY((HANDLE) handle, (unsigned int *)dest, (unsigned int *)src, (len/4)*4);
		if(wait_time)
		{
			if(_sys_kdma_wait(DMA_CH_15,wait_time) == -1)
			{
				PRINT_DMA("_sys_kdma_wait error!!!!!!!!! len = %d\n",len);
			}
		}

		for(i=0;i<=(len % 4);i++) //copy stuffing bytes
		{
			*(((unsigned char *)dest)+((len/4)*4)+i) = *(((unsigned char *)src)+((len/4)*4)+i);
		}

		SYS_DMA_RELEASE_CHANNEL((HANDLE) handle);
	}
	else
	{
		memcpy((void *)dest, (void *)src, len);
	}
	return 0;
}

int _sys_kdma_create_primary_ch_desc(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len, unsigned int *desc)
{
	unsigned int ch, srcwidth = 0, dstwidth = 0;
	unsigned int control;
	DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;

	ch = handle->dmachan_id;
	control = handle->control & 0xffffc006;
	srcwidth = 1<<((control>>1) & 0x3);
	dstwidth = srcwidth;

	if(handle->config == DMA_CONFIG_MEM2MEM) {
		control |= ((len/srcwidth-1)<< 3) |		/* n_minus_1 */
				(DMA_CONFIG_MEM2MEM	 << 0) |	/* cycle_ctrl */
				(7 << 18); 						/* R_power */
	}
	else
		control |= ((len/srcwidth-1)<< 3) | /* n_minus_1 */
				(DMA_CONFIG_MEM2PERI << 0); /* cycle_ctrl */

	((kdma_channel_data *)desc)->command = control;

	_sys_kdma_set_src_addr(ch, (unsigned int)src, srcwidth>>1, 1, (void *)desc);

	_sys_kdma_set_dst_addr(ch, (unsigned int)dst, dstwidth>>1, 1, (void *)desc);

	((kdma_channel_data *)desc)->next_descriptor = 0;

	return 0;
}


int SYS_DMA_COPY(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len)
{

	unsigned int	num_blocks, ch, srcwidth = 0;
	DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;
	unsigned int control, src_inc = 0, dst_inc = 0, scat_len;


	if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) || (len == 0) )
	{
		//klocwork	PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
		return -1;
	}

	control = handle->control & 0xffe3c006;

	ch = handle->dmachan_id;

	srcwidth = 1<<((control>>1) & 0x3);

	src_inc = (1<<((control>>14)&0x3))&0x7;
	dst_inc = (1<<((control>>16)&0x3))&0x7;

//	DA16200_KDMA_REG->cfg_irq_done_type = (DA16200_KDMA_REG->cfg_irq_done_type & ~(0x3<<(ch*2)))| (0x1<<(ch*2));// For Test

//jason180321 DA16200_KDMA_REG->cfg_channel_enable &= ~(1<<ch);

	_sys_kdma_free_garbage(ch);

	kdma_channel_data *link_next ;
	volatile unsigned long *link_temp, *link_start = NULL;

	num_blocks = len/(srcwidth*DMA_SCAT_TRANSFER);
	if((len%(srcwidth*DMA_SCAT_TRANSFER)) >= srcwidth)
		num_blocks++;

	g_link_alloc_count += num_blocks;

	for(unsigned int i=0;i<num_blocks;i++)
	{
		if(i != (num_blocks-1))
			scat_len = (srcwidth*DMA_SCAT_TRANSFER);
		else
			scat_len = len % (srcwidth*DMA_SCAT_TRANSFER);

		if(scat_len == 0 )
			scat_len = (srcwidth*DMA_SCAT_TRANSFER);

		link_temp = pvPortMalloc(16);
		if(link_temp == NULL)
			return -1;
		memset((void *)link_temp,0,16);

		_sys_kdma_create_primary_ch_desc(hdl,
								(unsigned int*)(((unsigned char*)dst) + (dst_inc*DMA_SCAT_TRANSFER*i)),
								(unsigned int*)(((unsigned char*)src) + (src_inc*DMA_SCAT_TRANSFER*i)),
								scat_len,
								(unsigned int*)link_temp);
		if(i == 0)
			link_start = link_temp;
		else
			(link_next->next_descriptor) = (unsigned int)link_temp;

		link_next = (kdma_channel_data *)link_temp;
		
	}

	PRINT_DMA("cfg_channel_enable = 0x%x, ch = 0x%x\r\n",DA16200_KDMA_REG->cfg_channel_enable, ch);
	if((DA16200_KDMA_REG->cfg_channel_enable & (1 << ch)) == 0)
	{
		kdma_descriptor_ptr[ch] = (unsigned int)link_start;
	}
	else
	{
		link_next = ((kdma_channel_data *)kdma_descriptor_ptr[ch]);
		
		while(((link_next->next_descriptor) > DA16X_SRAM_BASE) && ((unsigned int)link_next != (link_next->next_descriptor)))
		{
			link_next = (kdma_channel_data *)(((kdma_channel_data *)link_next)->next_descriptor);
			PRINT_DMA ("link_next = 0x%x\n", (unsigned int)link_next);
		}
			(link_next->next_descriptor) = (unsigned int)link_start;
		}
	dma_run_desc_head[ch].next_descriptor = (unsigned int)kdma_descriptor_ptr[ch];

#if 1
		/* Debugging printfs: */
			PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(link_start));
			PRINT_DMA ("command  = 0x%x\n", *link_start);
			PRINT_DMA ("src = 0x%x\n", *(link_start+1));
			PRINT_DMA ("dst = 0x%x\n", *(link_start+2));
			PRINT_DMA ("link = 0x%x\n", *(link_start+3));
#endif


#if 0
	/* Debugging printfs: */
	PRINT_DMA ("Primary\n");
	PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
	PRINT_DMA ("command  = 0x%x\n", dma_data->Primary[ch].command);
	PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
	PRINT_DMA ("ctrl = 0x%x\n", dma_data->Primary[ch].Control);
#endif
#if 0
	else
	{
		num_blocks = len/(srcwidth*DMA_SCAT_TRANSFER);
		if((len%(srcwidth*DMA_SCAT_TRANSFER)) >= srcwidth)
			num_blocks++;

		if(dma_descriptor[ch] != NULL)
			HAL_FREE((void *)dma_descriptor[ch]);

		if( !(dma_descriptor[ch] = HAL_MALLOC(num_blocks*16))) {
			return -1;
		}

 //	num_blocks = _sys_kdma_create_alt_ch_desc(hdl, dst, src, len, dma_descriptor[ch]);

/* Descriptor for Primary Channel */
		if(num_blocks > 256)
		{
			exceed_block[ch] = num_blocks/256 - 1;
			if(exceed_block[ch]%256)
			exceed_block[ch]++;
			num_blocks_e[ch] = num_blocks;
			num_blocks = 256;
			block_cnt_e[ch] = 0;
	   }
		else
		{
		   num_blocks_e[ch] = 0;
		   exceed_block[ch] = 0;
		   block_cnt_e[ch] = 0;
		}

		if(!(DA16200_KDMA_REG->cfg_channel_enable & (1<<ch))) {
			if(handle->config == DMA_CONFIG_MEM2MEM)
				_sys_kdma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, DMA_CONFIG_MEM2MEM, 2,(void *)(&(dma_data->Primary[ch].command)));
			else
				_sys_kdma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, DMA_CONFIG_MEM2PERI, 2,(void *)(&(dma_data->Primary[ch].command)));

			_sys_kdma_set_src_addr(ch, (unsigned int)dma_descriptor[ch], DMA_TRANSFER_WIDTH_WORD, num_blocks*4,(void *)(&(dma_data->Primary[ch].command)));

			dma_data->Primary[ch].next_descriptor = 0;
		}
		else {


		}

#if 0
		/* Debugging printfs: */
		PRINT_DMA ("Primary\n");
		PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
		PRINT_DMA ("command  = 0x%x\n", dma_data->Primary[ch].command);
		PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
		PRINT_DMA ("ctrl = 0x%x\n\n", dma_data->Primary[ch].Control);
#endif


	}
#endif
	if(!(DA16200_KDMA_REG->cfg_channel_enable & (1<<ch)))
	{
		_sys_kdma_open(ch);

		wait_handler_flag |= (1<<ch);


		if(handle->config == DMA_CONFIG_MEM2MEM)
				_sys_kdma_sw_request(ch);
	 // _sys_kdma_wait(ch,100);

	}

	return 0;
}


int SYS_DMA_LINK(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len, unsigned int enable)
{
	DA16X_UNUSED_ARG(hdl);
	DA16X_UNUSED_ARG(dst);
	DA16X_UNUSED_ARG(src);
	DA16X_UNUSED_ARG(len);
	DA16X_UNUSED_ARG(enable);
	return 0;
}

HANDLE SYS_DMA_OBTAIN_CHANNEL(unsigned int dmachan_id, unsigned int dma_control, unsigned int dma_config, unsigned int dma_maxlli )
{
	DA16X_UNUSED_ARG(dma_maxlli);

	if((g_dma_usage_flag & (1 << dmachan_id)) || (dmachan_id >= MAX_NUM_OF_DMA_CHANNELS))
	{
		PRINT_DMA ("+%s %d channel is already obtained\n",__func__,dmachan_id);
		return NULL;
	}
	dma_handle[dmachan_id].control = dma_control;
	dma_handle[dmachan_id].config = dma_config;
	dma_handle[dmachan_id].dev_addr = KDMA_BASE_0;
	dma_handle[dmachan_id].Primary = &dma_data->Primary[dmachan_id];
//	dma_handle[dmachan_id].Alternate= &dma_data->Alternate[dmachan_id];
	dma_handle[dmachan_id].dmachan_id = dmachan_id;

	g_dma_usage_flag |= (1<<dmachan_id);

	return &dma_handle[dmachan_id];
}

int SYS_DMA_RELEASE_CHANNEL(HANDLE hdl )
{
	 DMA_HANDLE_TYPE *handle;

	 handle = (DMA_HANDLE_TYPE *)hdl;

	 if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) )
	 {
	//klocwork	PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
		return -1;
	 }
	 g_dma_usage_flag &= ~(1<<handle->dmachan_id);

	dma_handle[handle->dmachan_id].control = 0;
	dma_handle[handle->dmachan_id].dev_addr = DA16200_KDMA_BASE;
	dma_handle[handle->dmachan_id].Primary = NULL;
//	dma_handle[handle->dmachan_id].Alternate= NULL;
	dma_handle[handle->dmachan_id].dmachan_id = 0;

	handle->callback[DMA_CB_TERM] = NULL ;
	handle->cb_param[DMA_CB_TERM] = NULL ;

	handle->callback[DMA_CB_EMPTY] = NULL ;
	handle->cb_param[DMA_CB_EMPTY] = NULL ;

	handle->callback[DMA_CB_ERROR] = NULL ;
	handle->cb_param[DMA_CB_ERROR] = NULL ;

	return 0;
}


int SYS_DMA_REGISTRY_CALLBACK( HANDLE hdl, USR_CALLBACK term, USR_CALLBACK empty, USR_CALLBACK error, HANDLE dev_handle )
{
	DMA_HANDLE_TYPE *handle;
	handle = (DMA_HANDLE_TYPE *)hdl;

	if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) )
	{
 //klocwork	PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
		return -1;
	}

	handle->callback[DMA_CB_TERM] = (USR_CALLBACK)term ;
	handle->cb_param[DMA_CB_TERM] = (VOID *)dev_handle ;

	handle->callback[DMA_CB_EMPTY] = (USR_CALLBACK)empty ;
	handle->cb_param[DMA_CB_EMPTY] = (VOID *)dev_handle ;

	handle->callback[DMA_CB_ERROR] = (USR_CALLBACK)error ;
	handle->cb_param[DMA_CB_ERROR] = (VOID *)dev_handle ;

	return TRUE;


}


int	SYS_DMA_RESET_CH(HANDLE hdl)//jason151105
{

	unsigned int ch;
	DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;

	
	ch = handle->dmachan_id;
	DA16200_KDMA_REG->cfg_channel_enable &= ~(1<<ch);
//jason180321	DA16200_KDMA_REG->cfg_channel_enable |= 	(1<<ch); /* Disable channel */
	
	if(handle->config == DMA_CONFIG_MEM2MEM)
		_sys_kdma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, 0, KDMA_CYCLE_CTRL_MEMSCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].command)));
	else
		_sys_kdma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, 0, KDMA_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].command)));

	return 0;

}
/* --------------------------------------------------------------- */
/*  DMA interrupt handlers                                         */
/* --------------------------------------------------------------- */

void process_dma_event(unsigned int ch)
{

	if((wait_int_flag & (1<<ch))/*&& !((dma_data->Primary[ch].Control>>4) & 0x3ff)*/)
	{
		xEventGroupSetBits(sys_kdma_evt, (1<<ch));
		wait_int_flag &= ~(1<<ch);
	}
}

void process_dma_done_callback(unsigned int ch)
{
	
//	if(/*(wait_handler_flag & (1<<ch))&&*/ ((dma_data->Primary[ch].command>>21) == 0x7ff))
	{
//		(dma_data->Primary[ch].command) &= 0x1fffff;
		if(dma_handle[ch].callback[DMA_CB_TERM]!=NULL )
		{
//			if(exceed_block[ch] == 0 || (exceed_block[ch] < block_cnt_e[ch]))
			{
				dma_handle[ch].callback[DMA_CB_TERM](dma_handle[ch].cb_param[DMA_CB_TERM]);
				wait_handler_flag &= ~(1<<ch);
//				block_cnt_e[ch] = 0;
			}

		}
//		DA16200_KDMA_REG->CHNL_REQ_MASK_SET = DA16200_KDMA_REG->CHNL_REQ_MASK_SET | (1<<i);
	}
}

__INLINE void process_dma_err_callback(unsigned int ch)
{

//	if(wait_handler_flag & (1<<ch))
	{
		if(dma_handle[ch].callback[DMA_CB_ERROR]!=NULL )
		{
			dma_handle[ch].callback[DMA_CB_ERROR](dma_handle[ch].cb_param[DMA_CB_ERROR]);
//????		wait_handler_flag &= ~(1<<ch);
		}
//		DA16200_KDMA_REG->CHNL_REQ_MASK_SET = DA16200_KDMA_REG->CHNL_REQ_MASK_SET | (1<<i);
	}
}


#if 0//jason180415
void process_dma_linked_block(unsigned int ch)
{
	unsigned int prev_list;
	kdma_channel_data *next;


	if((exceed_block[ch] == 0 || (exceed_block[ch] < block_cnt_e[ch])) && (dma_data->Primary[ch].next_descriptor != 0) /* && !((dma_data->Primary[ch].Control>>4) & 0x3ff)*/)
	{
		block_cnt_e[ch] = 0;
		prev_list = dma_data->Primary[ch].next_descriptor;


/* restart dma */
//	DA16200_KDMA_REG->CHNL_PRI_ALT_CLR &= (1<<ch);
//	_sys_kdma_open(ch);
		if((ch >= CH_DMA_PERI_MAX) && (ch < MAX_NUM_OF_DMA_CHANNELS))
			_sys_kdma_sw_request(ch);

/* process dangled list */
		((kdma_channel_data *)prev_list)->next_descriptor = 0;

		next = &dma_garbage_head[ch];
		while(next->next_descriptor != 0)
		{
			next = (kdma_channel_data *)(next->next_descriptor);
		}
		next->next_descriptor = prev_list;
	}

}
#else
void process_dma_linked_block(unsigned int ch)
{
#ifdef __ENABLE_UNUSED__
	unsigned int prev_list;
#endif	//__ENABLE_UNUSED__
	kdma_channel_data *next;

#ifdef __ENABLE_UNUSED__
	block_cnt_e[ch] = 0;
	prev_list = ((kdma_channel_data *)kdma_descriptor_ptr[ch])->next_descriptor;

/* process dangled list */
	((kdma_channel_data *)prev_list)->next_descriptor = 0;
#endif	//__ENABLE_UNUSED__

	next = &dma_garbage_head[ch];
	while(next->next_descriptor != 0)
	{
		next = (kdma_channel_data *)(next->next_descriptor);
	}
	next->next_descriptor = dma_run_desc_head[ch].next_descriptor;
	dma_run_desc_head[ch].next_descriptor = ((kdma_channel_data *)(dma_run_desc_head[ch].next_descriptor))->next_descriptor;

}
#endif

void	_sys_kdma_interrupt(void)
{
#if 1
	INTR_CNTXT_CALL(_sys_kdma_interrupt_handler);
#else
	asm volatile(
		" tst lr,#4		\n"
		" ite eq		\n"
		" mrseq r0,msp	\n"
		" mrsne r0,psp	\n"
		" mov r1,lr		\n"
		" ldr r2,=_sys_kdma_interrupt_handler \n"
		" bx r2"

		: /* Outputs */
		: /* Inputs */
		: /* Clobbers */
	);
#endif
}

void _sys_kdma_interrupt_handler(void)
{
//	PRINT_DMA ("+%s \n",__func__);
	if ((DA16200_KDMA_REG->irq_err_chs & 0xff) != 0) {
		/* DMA interrupt is caused by DMA error */
		dma_error_irq_occurred ++;
		DA16200_KDMA_REG->irq_err_clr = 0xff; /* Clear dma_err */
		PRINT_DMA ("+%s error %d\n",__func__,dma_error_irq_occurred);
	}
	else {
		// DMA interrupt is caused by DMA done
		dma_done_irq_occurred_all++;

	//	PRINT_DMA ("+%s done %d\n",__func__,dma_done_irq_occurred);
	//	NVIC_ClearPendingIRQ(DMA_IRQn);
		if(DA16200_KDMA_REG->irq_done_chs)
			for(int i=0; i<MAX_NUM_OF_DMA_CHANNELS; i++) {
				if(DA16200_KDMA_REG->irq_done_chs & (1<<i)) {
					dma_done_irq_occurred[i]++;
	//				process_dma_exceeded_block(i);
					if(((DA16200_KDMA_REG->cfg_irq_done_type>>(i*2)) == 0x1) || ((DA16200_KDMA_REG->cfg_irq_done_type>>(i*2)) == 0x0))
					process_dma_linked_block(i);
				
					if((DA16200_KDMA_REG->cfg_irq_done_type>>(i*2)) == 0x0) {
						process_dma_event(i);
						process_dma_done_callback(i);
					}
					DA16200_KDMA_REG->irq_done_clr = (1<<i);
				}
			}
 //		DA16200_KDMA_REG->DMA_DONE_CH_CLEAR = 0xffffffff;
	}
}



#endif	/*SUPPORT_UDMA*/
