/**
 ****************************************************************************************
 *
 * @file sys_dma.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "driver.h"
#include "common_def.h"

#ifdef	SUPPORT_UDMA
//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

#undef DMA_DEBUG_ON

#ifdef DMA_DEBUG_ON
#define PRINT_DMA(...) 		DRV_PRINTF( __VA_ARGS__ )
#else
#define PRINT_DMA(...)
#endif

#define UDMA_PRINTF(...)  	DRV_PRINTF( __VA_ARGS__ )

#define	CONVERT_MEM2UDMA(x)	x	//CONVERT_C2SYS_MEM( x )
#define	CONVERT_UDMA2MEM(x)	x	//CONVERT_SYS2C_MEM( x )

//======================================================================
//
//======================================================================

int dma_done_irq_occurred[MAX_NUM_OF_DMA_CHANNELS];
int dma_done_irq_occurred_all = 0;
volatile int dma_done_irq_expected;
volatile int dma_error_irq_occurred;
volatile int dma_error_irq_expected;


pl230_dma_data_structure *dma_data;

volatile unsigned int source_data_array[4];  /* Data array for memory DMA test */
volatile unsigned int dest_data_array[4];    /* Data array for memory DMA test */

EventGroupHandle_t	sys_dma_evt;

unsigned int wait_int_flag = 0, wait_handler_flag = 0;
unsigned int g_dma_usage_flag = 0;

DMA_HANDLE_TYPE dma_handle[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND;
unsigned int *dma_descriptor[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND = {NULL,};
int exceed_block[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND = {0,};
int num_blocks_e[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND = {0,};
int block_cnt_e[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND = {0,};
pl230_dma_channel_data dma_garbage_head[MAX_NUM_OF_DMA_CHANNELS] RW_SEC_DMA2ND = {NULL,};

unsigned int	g_link_alloc_count = 0, g_link_free_count = 0;

//======================================================================
//
//======================================================================
int ID_Check(const unsigned int id_array[], unsigned int offset)
{
int i;
unsigned long expected_val, actual_val;
unsigned long compare_mask;
int           mismatch = 0;
unsigned long test_addr;

  /* Check the peripheral ID and component ID */
  for (i=0;i<8;i++) {
    test_addr = offset + 4*i + 0xFE0;
    expected_val = id_array[i];
    actual_val   = (*((volatile unsigned long  *)(test_addr)));

    /* create mask to ignore version numbers */
    if (i==2) { compare_mask = 0xF0;}  // mask out version field
    else      { compare_mask = 0x00;}  // compare whole value

    if ((expected_val & (~compare_mask)) != (actual_val & (~compare_mask))) {
      PRINT_DMA ("Difference found: %x, expected %x, actual %x\n", test_addr, expected_val, actual_val);
      mismatch++;
      }

    } // end_for
return (mismatch);
}

/* --------------------------------------------------------------- */
/*  Detect if DMA controller is present or not                     */
/* --------------------------------------------------------------- */

int pl230_dma_detect(void)
{
  int result;

  unsigned const int pl230_id[12] = {
                                 0x30, 0xB2, 0x0B, 0x00,
                                 0x0D, 0xF0, 0x05, 0xB1};
  PRINT_DMA("Detect if DMA controller is present...");
  result = ID_Check(&pl230_id[0]   , CMSDK_PL230_BASE);
  if (result!=0) {
    PRINT_DMA("** TEST SKIPPED ** DMA controller is not present.\n");
  }
  return(result);
}


/* --------------------------------------------------------------- */
/*  Initialize DMA data structure                                  */
/* --------------------------------------------------------------- */
DATA_ALIGN(1024)static unsigned int ptr[256] RW_SEC_DMA;

void dma_data_struct_init(void)
{
  int          i;   /* loop counter */

//jason141014  unsigned int ptr;


  /* Create DMA data structure in RAM after stack
  In the linker script, a 1KB memory stack above stack is reserved
  so we can use this space for putting the DMA data structure.
  */


//jason141014 ptr     = SYS_DMA_CONTROL_BASE_ADDR;                     /* Read Top of Stack */

  /* the DMA data structure must be aligned to the size of the data structure */
//jason141014  if ((ptr & blkmask) != 0x0)
//jason141014    ptr     = (ptr + blksize) & ~blkmask;

//jason141014  if ((ptr + blksize) > (RAM_ADDRESS_MAX + 1)) {
//jason141014    PRINT_DMA ("ERROR : Not enough RAM space for DMA data structure.");
//jason141014   }

  /* Set pointer to the reserved space */
  dma_data = (pl230_dma_data_structure *) ptr;
//jason141014  ptr = (unsigned int) &dma_data->Primary->SrcEndPointer;

 PRINT_DMA ("dma structure block address = %x\n", ptr);

  for (i=0; i<MAX_NUM_OF_DMA_CHANNELS; i++) {
    dma_data->Primary[i].SrcEndPointer  = 0;
    dma_data->Primary[i].DestEndPointer = 0;
    dma_data->Primary[i].Control        = 0;
    dma_data->Primary[i].unused= 0;
    dma_data->Alternate[i].SrcEndPointer  = 0;
    dma_data->Alternate[i].DestEndPointer = 0;
    dma_data->Alternate[i].Control        = 0;
    dma_data->Alternate[i].unused        = 0;
    }

	memset(dma_garbage_head,0,sizeof(pl230_dma_channel_data)*MAX_NUM_OF_DMA_CHANNELS);//jason160312
	memset(dma_handle,0,sizeof(DMA_HANDLE_TYPE)*MAX_NUM_OF_DMA_CHANNELS);//jason160312
	memset(dma_descriptor,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);//jason160312
	memset(exceed_block,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);//jason160312
	memset(num_blocks_e,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);//jason160312
	memset(block_cnt_e,0,sizeof(int)*MAX_NUM_OF_DMA_CHANNELS);//jason160312

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
    unsigned int current_state;
    PRINT_DMA ("Initialize PL230");

    DA16X_CLKGATE_ON(clkoff_udma);

	DA16200_BOOTCON->sel_uDMA_kDMA = 0x0; // Enable uDMA

    pl230_dma_detect();

    current_state = CMSDK_DMA->DMA_STATUS;
    PRINT_DMA ("- # of channels allowed : %d\n",(((current_state) >> 16) & 0x1F)+1);

    /* Wait until current DMA complete */
    current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
    if (!((current_state==0) || (current_state==0x8) || (current_state==0x9))) {
      PRINT_DMA ("- wait for DMA IDLE/STALLED/DONE");
      current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
      PRINT_DMA ("- Current status        : %x\n",(((current_state) >> 4)  & 0xF));

      }
    while (!((current_state==0) || (current_state==0x8) || (current_state==0x9))){
      /* Wait if not IDLE/STALLED/DONE */
      current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
      PRINT_DMA ("- Current status        : %x\n",(((current_state) >> 4)  & 0xF));
      }

    dma_data_struct_init();

    CMSDK_DMA->DMA_CFG = 0; /* Disable DMA controller for initialization */
    CMSDK_DMA->CTRL_BASE_PTR = CONVERT_MEM2UDMA(((unsigned int) &(dma_data->Primary->SrcEndPointer)));
                             /* Set DMA data structure address */
    CMSDK_DMA->CHNL_ENABLE_CLR = 0xFFFFFFFF; /* Disable all channels */
//    CMSDK_DMA->CHNL_ENABLE_SET = (1<<0); /* Enable channel 0 */
    CMSDK_DMA->DMA_CFG = 0;              /* Enable DMA controller */
    CMSDK_DMA->DMA_CFG = 1;              /* Enable DMA controller */


    sys_dma_evt = xEventGroupCreate();
    wait_int_flag = 0; wait_handler_flag = 0;

    _sys_nvic_write(uDMA_IRQn, (void *)_sys_dma_interrupt, 0x7);

//    NVIC_ClearPendingIRQ(uDMA_IRQn);
//    NVIC_EnableIRQ(uDMA_IRQn);

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
    PRINT_DMA ("Deinit PL230");
    current_state = CMSDK_DMA->DMA_STATUS;

    /* Wait until current DMA complete */
    current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
    if (!((current_state==0) || (current_state==0x8) || (current_state==0x9))) {
      PRINT_DMA ("- wait for DMA IDLE/STALLED/DONE");
      current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
      PRINT_DMA ("- Current status        : %x\n",(((current_state) >> 4)  & 0xF));

      }
    while (!((current_state==0) || (current_state==0x8) || (current_state==0x9))){
      /* Wait if not IDLE/STALLED/DONE */
      current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
      PRINT_DMA ("- Current status        : %x\n",(((current_state) >> 4)  & 0xF));
      }
    CMSDK_DMA->DMA_CFG = 0; /* Disable DMA controller for initialization */
    CMSDK_DMA->CTRL_BASE_PTR = 0;
                             /* Set DMA data structure address */
    CMSDK_DMA->CHNL_ENABLE_CLR = 0xFFFFFFFF; /* Disable all channels */

    OAL_DELETE_EVENT_GROUP( &sys_dma_evt);

//    NVIC_DisableIRQ(uDMA_IRQn);
    _sys_nvic_write(uDMA_IRQn, NULL, 0x7);

    DA16X_CLKGATE_OFF(clkoff_udma);

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
int	_sys_dma_open( unsigned int ch )
{
//    PRINT_DMA ("Open PL230 channel enabled : 0x%x\n",CMSDK_DMA->CHNL_ENABLE_SET);
    if(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)){
        return -1;
    }

    CMSDK_DMA->CHNL_ENABLE_SET = (1<<ch); /* Enable channel  */

//    PRINT_DMA ("- channel enabled : 0x%x\n",CMSDK_DMA->CHNL_ENABLE_SET);

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
int	_sys_dma_close( unsigned int ch )
{
    unsigned int enabled;
    UDMA_PRINTF ("Close PL230 channel");
    enabled = CMSDK_DMA->CHNL_ENABLE_SET;
    if((enabled & (1<<ch)) == 0)
        return -1;

    PRINT_DMA ("- channel disabled : 0x%x\n",ch);

    CMSDK_DMA->CHNL_ENABLE_CLR = (1<<ch); /* Disable channel  */

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
int	_sys_dma_wait( unsigned int ch, unsigned int wait_time )
{
    unsigned int enabled;
    unsigned long   mask_evt;
    int ret = 0;
    enabled = CMSDK_DMA->CHNL_ENABLE_SET;
//    PRINT_DMA ("%s Wait PL230 channel enabled = 0x%x\n",__func__,enabled);
    if((enabled & (1<<ch)) == 0)
        return -1;

    wait_int_flag |= (1<<ch);

//    PRINT_DMA ("- start to wait dma interrupt event : 0x%x\n",enabled);
    mask_evt = xEventGroupWaitBits(sys_dma_evt, (1<<ch), pdTRUE, pdFALSE, wait_time);

    if(mask_evt & (1<<ch) == pdFALSE)
    {


		//Todo : Fill Code Here for Managing Abnormal Cases

            PRINT_DMA ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++wait dma interrupt event %d ticks timed out\n",wait_time);
            for(int i=0;i<20;i++)
            {
                PRINT_DMA("0x%x : 0x%08x\n", 0x4000f000+(i*4), *((unsigned int *)(0x4000f000+(i*4))));

            }
            for(int i=0;i<3;i++)
            {
                PRINT_DMA("0x%x : 0x%08x , 0x%x : 0x%08x\n", 0x20000000+(16*27+i*4), *((unsigned int *)(0x20000000+(16*27+i*4))), 0x20000200+(16*27+i*4), *((unsigned int *)(0x20000200+(16*27+i*4))));

            }

            _sys_dma_close(ch);
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
void	_sys_dma_set_control( unsigned int ch, unsigned int size, unsigned int num_word, unsigned int cycle_ctrl, unsigned int r_pow, void *desc )
{
    unsigned long control         = (size << 30) |  /* dst_inc */
                                    (size << 28) |  /* dst_size */
                                    (size << 26) |  /* src_inc */
                                    (size << 24) |  /* src_size */
                                    (size << 21) |  /* dst_prot_ctrl - HPROT[3:1] */
                                    (size << 18) |  /* src_prot_ctrl - HPROT[3:1] */
                                    ((num_word-1)<< 4) | /* n_minus_1 */
                                    (0    <<  3) |  /* next_useburst */
                                    (cycle_ctrl    <<  0) ;  /* cycle_ctrl  */
    if((cycle_ctrl == PL230_CYCLE_CTRL_MEMSCAT_PRIM) || (cycle_ctrl == PL230_CYCLE_CTRL_PERISCAT_PRIM))
        {

        control |= (2 << 14);   /* R_power */

        }
    else
        {
        control |= (r_pow << 14);   /* R_power */

        }


    ((pl230_dma_channel_data *)desc)->Control        = control        ;
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
void	_sys_dma_set_src_addr( unsigned int ch, unsigned int src, unsigned int size, unsigned int num_word, void *desc )
{
    unsigned long src_end_pointer =  src + ((1<<size)*(num_word-1));
    ((pl230_dma_channel_data *)desc)->SrcEndPointer  = CONVERT_MEM2UDMA(src_end_pointer);
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
 void    _sys_dma_set_dst_addr( unsigned int ch, unsigned int dst, unsigned int size, unsigned int num_word, void *desc )
 {
     unsigned long dst_end_pointer = dst + ((1<<size)*(num_word-1));
     ((pl230_dma_channel_data *)desc)->DestEndPointer = CONVERT_MEM2UDMA(dst_end_pointer);
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
void	_sys_dma_set_next( unsigned int ch, unsigned int next )
{
    dma_data->Primary[ch].unused = next;
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
void	_sys_dma_sw_request( unsigned int ch)
{
    CMSDK_DMA->CHNL_SW_REQUEST = (1<<ch); /* request channel  */
}
//jason160530
pl230_dma_channel_data*    _sys_dma_get_last_descriptor(unsigned int ch)
{
    pl230_dma_channel_data  *buffer = NULL, *next;//klocwork

    next = (pl230_dma_channel_data *)(dma_garbage_head[ch].unused);

    while(next != NULL)
    {
        buffer = next;
        next = (pl230_dma_channel_data *)(buffer->unused);
    }
	return buffer;
}
void    _sys_dma_free_garbage(unsigned int ch)
{
    pl230_dma_channel_data  *buffer, *next;

    next = (pl230_dma_channel_data *)(dma_garbage_head[ch].unused);

    NVIC_DisableIRQ(uDMA_IRQn);
    while(next != NULL)
    {
        buffer = next;
        next = (pl230_dma_channel_data *)(buffer->unused);
        HAL_FREE((void *)buffer);
		g_link_free_count++;
    }
    dma_garbage_head[ch].unused = 0;
    NVIC_EnableIRQ(uDMA_IRQn);
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

    int  i;
    unsigned int dma_control, dma_config = 0, dma_maxlli = 0;


    DMA_HANDLE_TYPE *handle;
    if(len >= MEMCPY_DMA_MINIMUM_SIZE && ((unsigned int)dest)%4 == 0 && ((unsigned int)src)%4 == 0)
    {

        dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
        dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD) ;

        dma_config = DMA_CONFIG_MEM2MEM;

        handle = SYS_DMA_OBTAIN_CHANNEL(DMA_CH_15, dma_control, dma_config, dma_maxlli);
        SYS_DMA_COPY((HANDLE) handle, (unsigned int *)dest, (unsigned int *)src, (len/4)*4);
        if(wait_time)
        {
            if(_sys_dma_wait(DMA_CH_15,wait_time) == -1)
                PRINT_DMA("_sys_dma_wait error!!!!!!!!! len = %d\n",len);

        }

        for(i=0;i<=(len % 4);i++) //copy stuffing bytes
         {
             *(((unsigned char *)dest)+((len/4)*4)+i) =  *(((unsigned char *)src)+((len/4)*4)+i);
         }

        SYS_DMA_RELEASE_CHANNEL((HANDLE) handle);
    }
    else
    {
        memcpy((void *)dest, (void *)src, len);
    }
  return 0;
}


int _sys_dma_create_alt_ch_desc(HANDLE hdl,  unsigned int *dst, unsigned int *src, unsigned int len, unsigned int *desc)
{

    int i=0;
    unsigned int    num_blocks, srcwidth = 0, dstwidth = 0;
    unsigned int control;
    DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;

    control = handle->control & 0xffffc000;
    srcwidth = 1<<((control>>24) & 0x3);
    dstwidth = 1<<((control>>28) & 0x3);
    num_blocks = len/(srcwidth*1024);
    if((len%(srcwidth*1024))!=0)
        num_blocks++;

    /* Descriptor for Alternate Channel */
            for( i=0; i < num_blocks-1 ;i++)
                {
                if(((control>>26)&0x3) == DMA_TRANSFER_NO_INCREMENT)//Source No Increment
                    *(desc + (i*4+0)) = (unsigned int)src;
                else
                    *(desc + (i*4+0)) = (unsigned int)src + (i*(srcwidth*1024)) + (srcwidth*1023);

                if((control>>30) == DMA_TRANSFER_NO_INCREMENT)//Destination No Increment
                    *(desc + (i*4+1)) = (unsigned int)dst;
                else
                    *(desc + (i*4+1)) = (unsigned int)dst + (i*(dstwidth*1024)) + (dstwidth*1023);

                if(handle->config == DMA_CONFIG_MEM2MEM)
                {
                    *(desc + (i*4+2)) = control |
                                                    (4    << 14) |  /* R_power */
                                                    (1023<< 4) | /* n_minus_1 */
                                                    (0    <<  3) |  /* next_useburst */
                                                    (PL230_CYCLE_CTRL_MEMSCAT_ALT <<  0) ;  /* cycle_ctrl  */
                }
                else
                {
                    *(desc + (i*4+2)) = control |
                                                    (1023<< 4) | /* n_minus_1 */
                                                    (0    <<  3) |  /* next_useburst */
                                                    (PL230_CYCLE_CTRL_PERISCAT_ALT <<  0) ;  /* cycle_ctrl  */

                }
                    *(desc + (i*4+3)) = (unsigned int)(desc + (i*4+4));
#if 0
                    PRINT_DMA ("Alternate %d\n",i);
                    PRINT_DMA ("SrcEndPointer  = 0x%x\n", *(desc + (i*4+0)));
                    PRINT_DMA ("DestEndPointer = 0x%x\n", *(desc + (i*4+1)));
                    PRINT_DMA ("ctrl = 0x%x\n", *(desc + (i*4+2)));
                    PRINT_DMA ("Descriptor = 0x%x\n\n", (unsigned int)(desc + (i*4+0)));
#endif
                }

    /* Descriptor for Alternate Channel - Tail */
            if(((len-1)%(srcwidth*1024))>= 3)
                {
                if(((control>>26)&0x3) == DMA_TRANSFER_NO_INCREMENT)//Source No Increment
                    *(desc + (i*4+0)) = (unsigned int)src ;
                else
                    *(desc + (i*4+0)) = (unsigned int)src + (i*(srcwidth*1024)) + srcwidth*(((len-1)%(srcwidth*1024) + 1)/srcwidth - 1);

                if((control>>30) == DMA_TRANSFER_NO_INCREMENT)//Destination No Increment
                    *(desc + (i*4+1)) = (unsigned int)dst;
                else
                    *(desc + (i*4+1)) = (unsigned int)dst+ (i*(dstwidth*1024)) + dstwidth*(((len-1)%(dstwidth*1024) + 1)/dstwidth - 1);

                if(handle->config == DMA_CONFIG_MEM2MEM)
                {
                    *(desc + (i*4+2)) =  control |
                                                    (4    << 14) |  /* R_power */
                                                    ((((len-1)%(srcwidth*1024)+1)/srcwidth - 1) << 4) | /* n_minus_1 */
                                                    (0    <<  3) |  /* next_useburst */
                                                    (PL230_CYCLE_CTRL_AUTOREQ <<  0) ;  /* cycle_ctrl  */
                }
                else
                {
                    *(desc + (i*4+2)) =  control |
                                                    ((((len-1)%(srcwidth*1024)+1)/srcwidth - 1) << 4) | /* n_minus_1 */
                                                    (0    <<  3) |  /* next_useburst */
                                                    (PL230_CYCLE_CTRL_BASIC <<  0) ;  /* cycle_ctrl  */

                }
                    *(desc + (i*4+3)) = 0;
#if 0
                    PRINT_DMA ("Alternate %d\n",i);
                    PRINT_DMA ("SrcEndPointer  = 0x%x\n", *(desc + (i*4+0)));
                    PRINT_DMA ("DestEndPointer = 0x%x\n", *(desc + (i*4+1)));
                    PRINT_DMA ("ctrl = 0x%x\n", *(desc + (i*4+2)));
                    PRINT_DMA ("Descriptor = 0x%x\n\n", (unsigned int)(desc + (i*4+0)));
#endif
                }
            else /* change previous cycle_ctrl, memscat to basic */
                {
                if(handle->config == DMA_CONFIG_MEM2MEM)
                {
                    *(desc + ((i-1)*4+2)) &= 0xFFFFFFF8 ;  /* clear cycle_ctrl  */
                    *(desc + ((i-1)*4+2)) |= PL230_CYCLE_CTRL_AUTOREQ ;  /* cycle_ctrl  */
                    *(desc + ((i-1)*4+3)) = 0;
                }
                else
                {
                    *(desc + ((i-1)*4+2)) &= 0xFFFFFFF8 ;  /* clear cycle_ctrl  */
                    *(desc + ((i-1)*4+2)) |= PL230_CYCLE_CTRL_BASIC ;  /* cycle_ctrl  */
                    *(desc + ((i-1)*4+3)) = 0;
                }
#if 0
                    PRINT_DMA ("Alternate %d\n",i-1);
                    PRINT_DMA ("ctrl = 0x%x\n", *(desc + ((i-1)*4+2)));
                    PRINT_DMA ("Descriptor = 0x%x\n\n", (unsigned int)(desc + ((i-1)*4+2)));
#endif
                }

            return num_blocks;
}


int _sys_dma_create_primary_ch_desc(HANDLE hdl,  unsigned int *dst, unsigned int *src, unsigned int len, unsigned int *desc)
{
    unsigned int ch, srcwidth = 0, dstwidth = 0;
    unsigned int control;
    DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;

    ch = handle->dmachan_id;
    control = handle->control & 0xffffc000;
    srcwidth = 1<<((control>>24) & 0x3);
    dstwidth = 1<<((control>>28) & 0x3);


    if(handle->config == DMA_CONFIG_MEM2MEM)
    {
        control |=  ((len/srcwidth-1)<< 4) | /* n_minus_1 */
                    (PL230_CYCLE_CTRL_AUTOREQ    <<  0) |  /* cycle_ctrl  */
                    (7 << 14);   /* R_power */
    }
    else
        control |=  ((len/srcwidth-1)<< 4) | /* n_minus_1 */
                    (PL230_CYCLE_CTRL_BASIC    <<  0);  /* cycle_ctrl  */

    ((pl230_dma_channel_data *)desc)->Control = control;

    if(((control>>26)&0x3) == DMA_TRANSFER_NO_INCREMENT)//Source No Increment
      _sys_dma_set_src_addr(ch, (unsigned int)src, srcwidth>>1, 1, (void *)desc);
    else
      _sys_dma_set_src_addr(ch, (unsigned int)src, srcwidth>>1, len/srcwidth, (void *)desc);

    if((control>>30) == DMA_TRANSFER_NO_INCREMENT)//Destination No Increment
        _sys_dma_set_dst_addr(ch, (unsigned int)dst, dstwidth>>1, 1, (void *)desc);
    else
        _sys_dma_set_dst_addr(ch, (unsigned int)dst, dstwidth>>1, len/dstwidth, (void *)desc);

    ((pl230_dma_channel_data *)desc)->unused = 0;


    return 0;
}


int SYS_DMA_COPY(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len)
{

  unsigned int    num_blocks, ch, srcwidth = 0;
  DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;
  unsigned int control;


  if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) )
    {
 //klocwork     PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
      return -1;
    }

  control = handle->control & 0xffffc000;

  ch = handle->dmachan_id;

  srcwidth = 1<<((control>>24) & 0x3);


  CMSDK_DMA->CHNL_REQ_MASK_CLR = (1<<ch);
  /* By default the PL230 is little-endian; if the processor is configured
   * big-endian then the configuration data that is written to memory must be
   * byte-swapped before being written.  This is also true if the processor is
   * little-endian and the PL230 is big-endian.
   * Remove the __REV usage if the processor and PL230 are configured with the
   * same endianness
   * */

  _sys_dma_free_garbage(ch);

  if(len < ((srcwidth*1024)+srcwidth))
  {

    if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)) && (dma_data->Primary[ch].unused == 0))
    {
        NVIC_DisableIRQ(uDMA_IRQn);
        _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int *)(&(dma_data->Primary[ch].SrcEndPointer)));
        NVIC_EnableIRQ(uDMA_IRQn);

    }
    else
    {
        volatile unsigned long *link_next ;
        volatile unsigned long *link_temp;

        link_next = &(dma_data->Primary[ch].SrcEndPointer);

//jason160401 MPW2.0 Runtime LLI Issue		  while((((pl230_dma_channel_data *)link_next)->unused) > 0x20000000)
		while((((pl230_dma_channel_data *)link_next)->unused) > DA16X_SRAM_BASE)
        {
            link_next = (volatile unsigned long *)(((pl230_dma_channel_data *)link_next)->unused);
        }

		link_temp = pvPortMalloc(16);
		g_link_alloc_count++;
        (((pl230_dma_channel_data *)link_next)->unused) = (unsigned int)link_temp;

        _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int*)link_temp);

        if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)) && (dma_data->Primary[ch].unused == 0))
        {
            PRINT_DMA("%s inside recheck enabled\n",__func__);
            NVIC_DisableIRQ(uDMA_IRQn);
            _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int *)(&(dma_data->Primary[ch].SrcEndPointer)));
            NVIC_EnableIRQ(uDMA_IRQn);
            vPortFree((void *)link_temp);
			g_link_free_count++;
        }
#if 0
              /* Debugging printfs: */
              PRINT_DMA ("Primary\n");
              PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(link_temp));
              PRINT_DMA ("SrcEndPointer  = 0x%x\n", *link_temp);
              PRINT_DMA ("DestEndPointer = 0x%x\n", *(link_temp+1));
              PRINT_DMA ("ctrl = 0x%x\n", *(link_temp+2));
#endif
    }


#if 0
      /* Debugging printfs: */
      PRINT_DMA ("Primary\n");
      PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
      PRINT_DMA ("SrcEndPointer  = 0x%x\n", dma_data->Primary[ch].SrcEndPointer);
      PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
      PRINT_DMA ("ctrl = 0x%x\n", dma_data->Primary[ch].Control);
#endif
    }
  else
    {
        num_blocks = len/(srcwidth*1024);
        if((len%(srcwidth*1024)) >= srcwidth)
            num_blocks++;

          if(dma_descriptor[ch] != NULL)
          {
        	  vPortFree((void *)dma_descriptor[ch]);
          }

        if( !(dma_descriptor[ch] = HAL_MALLOC(num_blocks*16)))
		{
			return -1;
		}

        num_blocks = _sys_dma_create_alt_ch_desc(hdl, dst, src, len, dma_descriptor[ch]);

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

        if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)))
        {
            if(handle->config == DMA_CONFIG_MEM2MEM)
                _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, PL230_CYCLE_CTRL_MEMSCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            else
                _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, PL230_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));

            _sys_dma_set_src_addr(ch, (unsigned int)dma_descriptor[ch], DMA_TRANSFER_WIDTH_WORD, num_blocks*4,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            _sys_dma_set_dst_addr(ch, (unsigned int)(&dma_data->Alternate[ch].SrcEndPointer), DMA_TRANSFER_WIDTH_WORD, 4,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));

            dma_data->Primary[ch].unused = 0;
        }
        else
        {


        }

#if 0
        /* Debugging printfs: */
        PRINT_DMA ("Primary\n");
        PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
        PRINT_DMA ("SrcEndPointer  = 0x%x\n", dma_data->Primary[ch].SrcEndPointer);
        PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
        PRINT_DMA ("ctrl = 0x%x\n\n", dma_data->Primary[ch].Control);
#endif


    }

  if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)))
  {
      CMSDK_DMA->CHNL_PRI_ALT_CLR &= (1<<ch);
      _sys_dma_open(ch);

      wait_handler_flag |= (1<<ch);


      if(handle->config == DMA_CONFIG_MEM2MEM)
          _sys_dma_sw_request(ch);
     // _sys_dma_wait(ch,100);

    }

  return 0;


}


int SYS_DMA_LINK(HANDLE hdl, unsigned int *dst, unsigned int *src, unsigned int len, unsigned int enable)
{

  unsigned int    num_blocks, ch, srcwidth = 0;
  DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;
  unsigned int control;


  if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) )
    {
 //klocwork       PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
      return -1;
    }

  control = handle->control & 0xffffc000;

  ch = handle->dmachan_id;

  srcwidth = 1<<((control>>24) & 0x3);


  CMSDK_DMA->CHNL_REQ_MASK_CLR = (1<<ch);
  /* By default the PL230 is little-endian; if the processor is configured
   * big-endian then the configuration data that is written to memory must be
   * byte-swapped before being written.  This is also true if the processor is
   * little-endian and the PL230 is big-endian.
   * Remove the __REV usage if the processor and PL230 are configured with the
   * same endianness
   * */

  _sys_dma_free_garbage(ch);

  if(len < ((srcwidth*1024)+srcwidth))
  {

    if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)) && (dma_data->Primary[ch].unused == 0) && ((dma_data->Primary[ch].Control & 0x3ff0) == 0))
    {
        NVIC_DisableIRQ(uDMA_IRQn);
        _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int *)(&(dma_data->Primary[ch].SrcEndPointer)));
        NVIC_EnableIRQ(uDMA_IRQn);

    }
    else
    {
        volatile unsigned long *link_next ;
        volatile unsigned long *link_temp;

        link_next = &(dma_data->Primary[ch].SrcEndPointer);

        while((((pl230_dma_channel_data *)link_next)->unused) > 0x20000000)
        {
            link_next = (volatile unsigned long *)(((pl230_dma_channel_data *)link_next)->unused);
        }

        link_temp = pvPortMalloc(16);
		g_link_alloc_count++;
        (((pl230_dma_channel_data *)link_next)->unused) = (unsigned int)link_temp;

        _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int*)link_temp);

        if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)) && (dma_data->Primary[ch].unused == 0))
        {
            PRINT_DMA("%s inside recheck enabled\n",__func__);
            NVIC_DisableIRQ(uDMA_IRQn);
            _sys_dma_create_primary_ch_desc(hdl, dst, src, len, (unsigned int *)(&(dma_data->Primary[ch].SrcEndPointer)));
            NVIC_EnableIRQ(uDMA_IRQn);
            vPortFree((void *)link_temp);
			g_link_free_count++;
        }
#if 0
              /* Debugging printfs: */
              PRINT_DMA ("Primary\n");
              PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(link_temp));
              PRINT_DMA ("SrcEndPointer  = 0x%x\n", *link_temp);
              PRINT_DMA ("DestEndPointer = 0x%x\n", *(link_temp+1));
              PRINT_DMA ("ctrl = 0x%x\n", *(link_temp+2));
#endif
    }


#if 0
      /* Debugging printfs: */
      PRINT_DMA ("Primary\n");
      PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
      PRINT_DMA ("SrcEndPointer  = 0x%x\n", dma_data->Primary[ch].SrcEndPointer);
      PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
      PRINT_DMA ("ctrl = 0x%x\n", dma_data->Primary[ch].Control);
#endif
    }
  else
    {
        num_blocks = len/(srcwidth*1024);
        if((len%(srcwidth*1024)) >= srcwidth)
            num_blocks++;

		if(dma_descriptor[ch] != NULL)
		{
			vPortFree((void *)dma_descriptor[ch]);
		}

        if( !(dma_descriptor[ch] = HAL_MALLOC(num_blocks*16)))
		{
			return -1;
		}

        num_blocks = _sys_dma_create_alt_ch_desc(hdl, dst, src, len, dma_descriptor[ch]);

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

        if(!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch)))
        {
            if(handle->config == DMA_CONFIG_MEM2MEM)
                _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, PL230_CYCLE_CTRL_MEMSCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            else
                _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, num_blocks*4, PL230_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));

            _sys_dma_set_src_addr(ch, (unsigned int)dma_descriptor[ch], DMA_TRANSFER_WIDTH_WORD, num_blocks*4,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            _sys_dma_set_dst_addr(ch, (unsigned int)(&dma_data->Alternate[ch].SrcEndPointer), DMA_TRANSFER_WIDTH_WORD, 4,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));

            dma_data->Primary[ch].unused = 0;
        }
        else
        {


        }

#if 0
        /* Debugging printfs: */
        PRINT_DMA ("Primary\n");
        PRINT_DMA ("DMAControlAddr  = 0x%x\n", (unsigned int)(dma_data));
        PRINT_DMA ("SrcEndPointer  = 0x%x\n", dma_data->Primary[ch].SrcEndPointer);
        PRINT_DMA ("DestEndPointer = 0x%x\n", dma_data->Primary[ch].DestEndPointer);
        PRINT_DMA ("ctrl = 0x%x\n\n", dma_data->Primary[ch].Control);
#endif


    }

  if((!(CMSDK_DMA->CHNL_ENABLE_SET & (1<<ch))) && (enable))
  {
      CMSDK_DMA->CHNL_PRI_ALT_CLR &= (1<<ch);
      _sys_dma_open(ch);

      wait_handler_flag |= (1<<ch);


      if(handle->config == DMA_CONFIG_MEM2MEM)
          _sys_dma_sw_request(ch);
     // _sys_dma_wait(ch,100);

    }

  return 0;


}

HANDLE SYS_DMA_OBTAIN_CHANNEL(unsigned int dmachan_id, unsigned int dma_control, unsigned int dma_config, unsigned int dma_maxlli )
{


    if((g_dma_usage_flag & (1 << dmachan_id)) || (dmachan_id >= MAX_NUM_OF_DMA_CHANNELS))
    {
        PRINT_DMA ("+%s %d channel is already obtained\n",__func__,dmachan_id);
        return NULL;
    }
    dma_handle[dmachan_id].control = dma_control;
    dma_handle[dmachan_id].config = dma_config;
    dma_handle[dmachan_id].dev_addr = CMSDK_PL230_BASE;
    dma_handle[dmachan_id].Primary = &dma_data->Primary[dmachan_id];
    dma_handle[dmachan_id].Alternate= &dma_data->Alternate[dmachan_id];
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
 //klocwork        PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
       return -1;
     }
   g_dma_usage_flag &= ~(1<<handle->dmachan_id);

    dma_handle[handle->dmachan_id].control = 0;
    dma_handle[handle->dmachan_id].dev_addr = CMSDK_PL230_BASE;
    dma_handle[handle->dmachan_id].Primary = NULL;
    dma_handle[handle->dmachan_id].Alternate= NULL;
    dma_handle[handle->dmachan_id].dmachan_id = 0;

    handle->callback[DMA_CB_TERM] = NULL ;
    handle->cb_param[DMA_CB_TERM] = NULL ;

    handle->callback[DMA_CB_EMPTY] = NULL ;
    handle->cb_param[DMA_CB_EMPTY] = NULL ;

    handle->callback[DMA_CB_ERROR] = NULL ;
    handle->cb_param[DMA_CB_ERROR] = NULL ;

    return 0;
}


int SYS_DMA_REGISTRY_CALLBACK( HANDLE hdl, USR_CALLBACK term,  USR_CALLBACK empty, USR_CALLBACK error  , HANDLE dev_handle )
{
    DMA_HANDLE_TYPE *handle;
    handle = (DMA_HANDLE_TYPE *)hdl;

    if( !(g_dma_usage_flag & (1 << handle->dmachan_id)) || (handle == NULL) )
      {
 //klocwork         PRINT_DMA ("+%s %d channel is not obtained\n",__func__,handle->dmachan_id);
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


int     SYS_DMA_RESET_CH(HANDLE hdl)//jason151105
{

	unsigned int ch;
	DMA_HANDLE_TYPE *handle = (DMA_HANDLE_TYPE *)hdl;

	
	ch = handle->dmachan_id;
	CMSDK_DMA->CHNL_REQ_MASK_CLR = (1<<ch);
    CMSDK_DMA->CHNL_ENABLE_CLR = (1<<ch); /* Disable channel */
	
	if(handle->config == DMA_CONFIG_MEM2MEM)
		_sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, 0, PL230_CYCLE_CTRL_MEMSCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
	else
		_sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, 0, PL230_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));

	return 0;

}
/* --------------------------------------------------------------- */
/*  DMA interrupt handlers                                         */
/* --------------------------------------------------------------- */



void process_dma_event(unsigned int ch)
{

    if((wait_int_flag & (1<<ch))/*&&  !((dma_data->Primary[ch].Control>>4) & 0x3ff)*/)
    {
    	xEventGroupSetBits(sys_dma_evt,(1<<ch));
        wait_int_flag &= ~(1<<ch);
    }
}

void process_dma_done_callback(unsigned int ch)
{

    if(/*(wait_handler_flag & (1<<ch))&&*/ !((dma_data->Primary[ch].Control>>4) & 0x3ff))
    {
        if(dma_handle[ch].callback[DMA_CB_TERM]!=NULL )
        {
            if(exceed_block[ch] == 0 || (exceed_block[ch] < block_cnt_e[ch]))
            {
                dma_handle[ch].callback[DMA_CB_TERM](dma_handle[ch].cb_param[DMA_CB_TERM]);
                wait_handler_flag &= ~(1<<ch);
                block_cnt_e[ch] = 0;
            }

        }
        //               CMSDK_DMA->CHNL_REQ_MASK_SET = CMSDK_DMA->CHNL_REQ_MASK_SET | (1<<i);
    }
}

__INLINE void process_dma_err_callback(unsigned int ch)
{

//    if(wait_handler_flag & (1<<ch))
    {
        if(dma_handle[ch].callback[DMA_CB_ERROR]!=NULL )
        {
            dma_handle[ch].callback[DMA_CB_ERROR](dma_handle[ch].cb_param[DMA_CB_ERROR]);
//????            wait_handler_flag &= ~(1<<ch);
        }
        //               CMSDK_DMA->CHNL_REQ_MASK_SET = CMSDK_DMA->CHNL_REQ_MASK_SET | (1<<i);
    }
}

void process_dma_exceeded_block(unsigned int ch)
{

    if(!((dma_data->Primary[ch].Control>>4) & 0x3ff))
    {
#if 1
        if((exceed_block[ch]-block_cnt_e[ch])>1)
        {
            block_cnt_e[ch]++;
            _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, 1024, PL230_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            _sys_dma_set_src_addr(ch, ((unsigned int)dma_descriptor[ch])+(4096*block_cnt_e[ch]), DMA_TRANSFER_WIDTH_WORD, 1024, (void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            CMSDK_DMA->CHNL_PRI_ALT_CLR &= (1<<ch);
            _sys_dma_open(ch);
        }
        else
        if((exceed_block[ch]-block_cnt_e[ch]) == 1)
        {
            block_cnt_e[ch]++;
            _sys_dma_set_control(ch, DMA_TRANSFER_WIDTH_WORD, (num_blocks_e[ch]%256)*4, PL230_CYCLE_CTRL_PERISCAT_PRIM, 2,(void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            _sys_dma_set_src_addr(ch, ((unsigned int)dma_descriptor[ch])+(4096*block_cnt_e[ch]), DMA_TRANSFER_WIDTH_WORD, (num_blocks_e[ch]%256)*4, (void *)(&(dma_data->Primary[ch].SrcEndPointer)));
            CMSDK_DMA->CHNL_PRI_ALT_CLR &= (1<<ch);
            _sys_dma_open(ch);
        }
        else
#endif
           block_cnt_e[ch]++;
    }
}

void process_dma_linked_block(unsigned int ch)
{
    unsigned int prev_list;
    pl230_dma_channel_data *next;


    if((exceed_block[ch] == 0 || (exceed_block[ch] < block_cnt_e[ch])) && (dma_data->Primary[ch].unused != 0) /* && !((dma_data->Primary[ch].Control>>4) & 0x3ff)*/)
    {
        block_cnt_e[ch] = 0;
        prev_list = dma_data->Primary[ch].unused;

/* change to next dma chain */
        dma_data->Primary[ch].Control = ((pl230_dma_channel_data *)(dma_data->Primary[ch].unused))->Control;
        dma_data->Primary[ch].SrcEndPointer= ((pl230_dma_channel_data *)(dma_data->Primary[ch].unused))->SrcEndPointer;
        dma_data->Primary[ch].DestEndPointer= ((pl230_dma_channel_data *)(dma_data->Primary[ch].unused))->DestEndPointer;
        dma_data->Primary[ch].unused = ((pl230_dma_channel_data *)(dma_data->Primary[ch].unused))->unused;

/* restart dma */
        CMSDK_DMA->CHNL_PRI_ALT_CLR &= (1<<ch);
        _sys_dma_open(ch);
		if((ch >= CH_DMA_PERI_MAX) && (ch < MAX_NUM_OF_DMA_CHANNELS))
			_sys_dma_sw_request(ch);

/* process dangled list */
        ((pl230_dma_channel_data *)prev_list)->unused = 0;

        next = &dma_garbage_head[ch];
        while(next->unused != 0)
        {
            next = (pl230_dma_channel_data *)(next->unused);
        }
        next->unused = prev_list;
    }

}

void	_sys_dma_interrupt(void)
{
#if 1
    INTR_CNTXT_CALL(_sys_dma_interrupt_handler);
#else
	asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=_sys_dma_interrupt_handler \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
	);
#endif
}


void _sys_dma_interrupt_handler(void)
{
//  PRINT_DMA ("+%s \n",__func__);
if ((CMSDK_DMA->ERR_CLR & 1) != 0)  {
  /* DMA interrupt is caused by DMA error */
  dma_error_irq_occurred ++;
  CMSDK_DMA->ERR_CLR = 1; /* Clear dma_err */
  PRINT_DMA ("+%s error %d\n",__func__,dma_error_irq_occurred);
  }
else {
  // DMA interrupt is caused by DMA done
  dma_done_irq_occurred_all++;

//  PRINT_DMA ("+%s done %d\n",__func__,dma_done_irq_occurred);
//  NVIC_ClearPendingIRQ(DMA_IRQn);
    if(CMSDK_DMA->DMA_DONE_CH_STS)
        for(int i=0; i<MAX_NUM_OF_DMA_CHANNELS; i++)
        {
            if(CMSDK_DMA->DMA_DONE_CH_STS & (1<<i))
            {
                dma_done_irq_occurred[i]++;
                process_dma_exceeded_block(i);
                process_dma_linked_block(i);
                process_dma_event(i);
                process_dma_done_callback(i);
                CMSDK_DMA->DMA_DONE_CH_CLEAR = (1<<i);
            }
        }
 //       CMSDK_DMA->DMA_DONE_CH_CLEAR = 0xffffffff;
  }
}


#ifdef  PL230_TEST_DMA

void delay(void)
{
  int i;
  for (i=0;i<5;i++){
    __ISB();
    }
  return;
}
/* --------------------------------------------------------------- */
/*  Simple software DMA test                                       */
/* --------------------------------------------------------------- */
int dma_simple_test(void)
{
  int return_val=0;
  int err_code=0;
  int i;
  unsigned int current_state;


  UDMA_PRINTF("uDMA simple test");
  CMSDK_DMA->CHNL_ENABLE_SET = (1<<0); /* Enable channel 0 */

  /* setup data for DMA */
  for (i=0;i<4;i++) {
    source_data_array[i] = i;
    dest_data_array[i]   = 0;
  }

  dma_memory_copy ((unsigned int) &source_data_array[0],(unsigned int) &dest_data_array[0], 2, 4);
  do { /* Wait until PL230 DMA controller return to idle state */
    current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
  } while (current_state!=0);

  for (i=0;i<4;i++) {
    /* Debugging printf: */
    /*UDMA_PRINTF (" - dest[i] = %x\n", dest_data_array[i]);*/
    if (dest_data_array[i]!= i){
      UDMA_PRINTF ("ERROR:dest_data_array[%d], expected %x, actual %x\n", i, i, dest_data_array[i]);
      err_code |= (1<<i);
    }
  }

  /* Generate return value */
  if (err_code != 0) {
    UDMA_PRINTF ("ERROR : simple DMA failed (0x%x)\n", err_code);
    return_val=1;
  } else {
    UDMA_PRINTF ("-Passed");
  }

  return(return_val);
}
/* --------------------------------------------------------------- */
/*  Simple DMA interrupt test                                      */
/* --------------------------------------------------------------- */
int dma_interrupt_test(void)
{
  int return_val=0;
  int err_code=0;
  int i;
  unsigned int current_state;


  UDMA_PRINTF("DMA interrupt test");
  UDMA_PRINTF("- DMA done");

  CMSDK_DMA->CHNL_ENABLE_SET = (1<<0); /* Enable channel 0 */

  /* setup data for DMA */
  for (i=0;i<4;i++) {
    source_data_array[i] = i;
    dest_data_array[i]   = 0;
  }

  dma_done_irq_expected = 1;
  dma_done_irq_occurred = 0;
  NVIC_ClearPendingIRQ(uDMA_IRQn);
  NVIC_EnableIRQ(uDMA_IRQn);

  dma_memory_copy ((unsigned int) &source_data_array[0],(unsigned int) &dest_data_array[0], 2, 4);
  delay();
  /* Can't guarantee that there is sleep support, so use a polling loop */
  do { /* Wait until PL230 DMA controller return to idle state */
    current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
  } while (current_state!=0);

  for (i=0;i<4;i++) {
    /* Debugging printf: */
    /*UDMA_PRINTF (" - dest[i] = %x\n", dest_data_array[i]);*/
    if (dest_data_array[i]!= i){
      UDMA_PRINTF ("ERROR:dest_data_array[%d], expected %x, actual %x\n", i, i, dest_data_array[i]);
      err_code |= (1<<i);
    }
  }

  if (dma_done_irq_occurred==0){
    UDMA_PRINTF ("ERROR: DMA done IRQ missing");
    err_code |= (1<<4);
  }

  UDMA_PRINTF("- DMA err");
  CMSDK_DMA->CHNL_ENABLE_SET = (1<<0); /* Enable channel 0 */

  /* setup data for DMA */
  for (i=0;i<4;i++) {
    source_data_array[i] = i;
    dest_data_array[i]   = 0;
  }

  dma_error_irq_expected = 1;
  dma_error_irq_occurred = 0;
  NVIC_ClearPendingIRQ(uDMA_IRQn);
  NVIC_EnableIRQ(uDMA_IRQn);

  /* Generate DMA transfer to invalid memory location */
  dma_memory_copy ((unsigned int) &source_data_array[0],0x1F000000, 2, 4);
  delay();
  /* Can't guarantee that there is sleep support, so use a polling loop */
  do { /* Wait until PL230 DMA controller return to idle state */
    current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
  } while (current_state!=0);

  if (dma_error_irq_occurred==0){
    UDMA_PRINTF ("ERROR: DMA err IRQ missing");
    err_code |= (1<<5);
  }


  /* Clear up */
  dma_done_irq_expected = 0;
  dma_done_irq_occurred = 0;
  dma_error_irq_expected = 0;
  dma_error_irq_occurred = 0;
  NVIC_DisableIRQ(uDMA_IRQn);

  /* Generate return value */
  if (err_code != 0) {
    UDMA_PRINTF ("ERROR : DMA done interrupt failed (0x%x)\n", err_code);
    return_val=1;
  } else {
    UDMA_PRINTF ("-Passed");
  }

  return(return_val);
}

/* --------------------------------------------------------------- */
/*  DMA event test                                                 */
/* --------------------------------------------------------------- */
int dma_event_test(void)
{
  int return_val=0;
  int err_code=0;
  int i;
  unsigned int current_state;


  UDMA_PRINTF("DMA event test");
  UDMA_PRINTF("- DMA done event to RXEV");

  CMSDK_DMA->CHNL_ENABLE_SET = (1<<0); /* Enable channel 0 */

  /* setup data for DMA */
  for (i=0;i<4;i++) {
    source_data_array[i] = i;
    dest_data_array[i]   = 0;
  }

  dma_done_irq_expected = 1;
  dma_done_irq_occurred = 0;
  NVIC_ClearPendingIRQ(uDMA_IRQn);
  NVIC_DisableIRQ(uDMA_IRQn);

  /* Clear event register - by setting event with SEV and then clear it with WFE */
  __SEV();
  __WFE(); /* First WFE will not enter sleep because of previous event */

  dma_memory_copy ((unsigned int) &source_data_array[0],(unsigned int) &dest_data_array[0], 2, 4);
  __WFE(); /* This will cause the processor to enter sleep */

  /* Processor woken up */
  current_state = (CMSDK_DMA->DMA_STATUS >> 4)  & 0xF;
  if (current_state!=0) {
    UDMA_PRINTF ("ERROR: DMA status should be IDLE after wake up");
    err_code |= (1<<5);
  }

  for (i=0;i<4;i++) {
    /*printf (" - dest[i] = %x\n", dest_data_array[i]);*/
    if (dest_data_array[i]!= i){
      UDMA_PRINTF ("ERROR:dest_data_array[%d], expected %x, actual %x\n", i, i, dest_data_array[i]);
      err_code |= (1<<i);
    }
  }

  /* Generate return value */
  if (err_code != 0) {
    UDMA_PRINTF ("ERROR : DMA event failed (0x%x)\n", err_code);
    return_val=1;
  } else {
    UDMA_PRINTF ("-Passed");
  }

  return(return_val);
}
#endif

#endif	/*SUPPORT_UDMA*/
