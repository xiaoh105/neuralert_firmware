/**
 ****************************************************************************************
 *
 * @file emmc.c
 *
 * @brief Driver for SDIO 2.0 host. it can access SD card, MMC card and SDIO device.
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

#include "oal.h"
#include "hal.h"
#include "console.h"
#include "emmc.h"
#include "sd.h"
#include "sdio.h"
#include "sdio_cis.h"
#include "da16x_compile_opt.h"

#define FREE_RTOS
#undef OAL_WRAPPER

//#undef BUILD_OPT_VSIM

#ifndef BUILD_OPT_VSIM
#ifdef OAL_WRAPPER
#define EMMC_MSLEEP(x)		OAL_MSLEEP(x)
#else
#define EMMC_MSLEEP(x)      //vTaskDelay(x/portTICK_PERIOD_MS)
#endif
#else
static void emmc_vsim_delay(int x)
{
    int i, temp;
    for (i = 0; i < 10; i++)
        temp = *((volatile UINT *)0x50001000);
}
#define EMMC_MSLEEP(x)		emmc_vsim_delay(x)
#endif


/* default setting ****************************************/
static EMMC_DRV_TYPE emmc_drv_type = EMMC_INTERRUPT_DRIVEN;
static int emmc_delay = 0;
static int emmc_debug = 0;
static EMMC_CLK_TYPE emmc_clk_type = EMMC_CLK_ALWAYS_ON;
/* default setting ****************************************/

#define EMMC_PRINTF(...)		if (emmc_debug) {Printf(__VA_ARGS__); }

#define SYS_EMMC_BASE_0		((EMMC_REG_MAP*)EMMC_BASE_0)

#ifndef HW_REG_AND_WRITE32
#define	HW_REG_AND_WRITE32(x, v)	( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define	HW_REG_OR_WRITE32(x, v)		( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v) 		( *((volatile unsigned int   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define	HW_REG_READ32(x)			( *((volatile unsigned int   *) &(x)) )
#endif

#ifndef ntoh32
#define ntoh32(x)					( ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24) )
#endif


static UINT32 g_INT_VALUE;
static EMMC_HANDLER_TYPE *irq_emmc;

#if 0
static UINT32 intreg;
#endif

void _emmc_interrupt(void)
{
    g_INT_VALUE = HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    //g_INT_VALUE = HW_REG_READ32(irq_emmc->regmap->HIF_EVNT_CTL);
    //g_INT_VALUE |= (HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL) & 0x0fff0000);
#if 0
	if (emmc_debug)
        intreg = HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL);
#endif
	ASIC_DBG_TRIGGER(MODE_HAL_STEP(0x44));
#ifdef OAL_WRAPPER
    OAL_SEND_TO_QUEUE_ISR(&(irq_emmc->emmc_queue), &g_INT_VALUE, 1, OAL_NO_SUSPEND );
#else
    xQueueSendFromISR((irq_emmc->emmc_queue), &g_INT_VALUE ,&xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
	ASIC_DBG_TRIGGER(MODE_HAL_STEP(0x55));
    // clear all event -> 현재 뜬 인터럽트만 clear 하게끔 변경
    HW_REG_AND_WRITE32(irq_emmc->regmap->HIF_INT_CTL, ~(0x00008000));
    HW_REG_WRITE32(irq_emmc->regmap->HIF_EVNT_CTL, (g_INT_VALUE & 0x3fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_EVNT_CTL) & 0x0000ffff));
    HW_REG_WRITE32(irq_emmc->regmap->HIF_INT_CTL, (g_INT_VALUE & 0x0fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL) & 0x0000ffff));
#if 0

#endif

}

static void	_tx_specific_emmc_interrupt(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(_emmc_interrupt);
	traceISR_EXIT();
}

typedef enum _emmc_clock_
{
    emmc_clock_off,
    emmc_clock_on
} emmc_clock;

static void emmc_clock_control(const HANDLE handler, emmc_clock onoff)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    if (emmc_clk_type == EMMC_CLK_ALWAYS_ON)
        return;

    if (onoff == emmc_clock_off)
        HW_REG_WRITE32(emmc->regmap->HIF_CLK_CTL_CNT, 0x00000000);
    else
        HW_REG_WRITE32(emmc->regmap->HIF_CLK_CTL_CNT, 0x00160000);

}


static UINT32 check_address(const HANDLE handler, UINT32 addr, UINT32 block_count)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;


    // sd 1.1일 때 2.0일 때 mmc 일때
    switch(emmc->card_type)
    {
    case SD_CARD_1_1:
        if ( (emmc->csd.capa_size << 10) > ((addr + (block_count - 1)) << 9))
            return 1;
        else
            return 0;
        break;
    case SD_CARD:
        if ( (emmc->csd.capa_size) > (addr + (block_count - 1)))
            return 1;
        else
            return 0;
        break;
    case MMC_CARD:
        if ( (emmc->ext_csd.sectors & 0x00FFFFFF) > (addr + (block_count - 1)))
            return 1;
        else
            return 0;
        break;
    default:
        return 0;
    }
}

static void emmc_cmd_without_res(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg)
{

    UINT32 RSP_type, DATA_type;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
        //OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#ifdef OAL_WRAPPER
		OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),1000);
#else
		xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        // enable interrupt
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }

    //while ( CMDQ_TRIG = 0 )  register HIF_CMD_IDX
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set CMD arguments
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    // set cmd index 1 rsp_type 0, data_type 0, cmdq_trig 1
    RSP_type = (0 << 11);
    DATA_type = (0 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));
    // h/w send the cmd to bus

    // reponse가 없더라도 rsp done가 뜬다
    // receive rsp?
    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&p_inter), 1000);
        xSemaphoreGive((emmc->emmc_sema));
#endif
    }
    else
    {
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );
    }
}

static int emmc_cmd_with_res(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp)
{
    UINT32 RSP_type, DATA_type;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;


    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
		OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),1000);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        // enable interrupt
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }
    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    // set cmd index rsp_type = 0x01, 0x10 or 0x11, data_type 0x00 cmdq_trig = 1
    RSP_type = ((GET_CMD_RESP_TYPE(cmd)) << 11);
    DATA_type = (0 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0x11));
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#ifdef OAL_WRAPPER
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0x66));
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if (( p_inter & 0x00040000) == 0x00040000)
        {
            if (GET_CMD_RESP_TYPE(cmd) < 3)
                *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            else
            {
                rsp[0] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
                rsp[1] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_1);
                rsp[2] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_2);
                rsp[3] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_3);
            }
            return ERR_NONE;
        }
        else
        {
            return ERR_RES;
        }
    }
    else
    {
        // receive rsp?
#ifndef	 BUILD_OPT_VSIM
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );
#else
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000) == 0x00 );
#endif

        //EMMC_PRINTF("HIF_EVNT_CTL 0x%08x\n",HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL));
        // true hw stores the received rsp
        if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000) == 0x00040000)
        {
            if (GET_CMD_RESP_TYPE(cmd) < 3)
                *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            else
            {
                rsp[0] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
                rsp[1] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_1);
                rsp[2] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_2);
                rsp[3] = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_3);
            }
            return ERR_NONE;
        }
        else
        {
            // false rsp time - out
            // rsp time out handling
            // 만약 response를 못 받는 경우가 있다면 emmc block을 reset 해야 될 경우도 있다
            // end
            return ERR_RES;
        }
    }
}

static int emmc_single_read_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf)
{
    UINT32 RSP_type, DATA_type;
    int err;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) - 1)));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) )));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
		OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),1000);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0xfff70000);
    }


    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set transfer block count 0x01
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, 0x01);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x02 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type));
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT;
        }

        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)	// err occur
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_SINGLE_READ | ERR_RES;
            }
        }
        // 이 시점에 원하는 이밴트가 이미 발생했을 수 있기때문에 clear 동작 하지 않게 변경
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008f00);
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#ifdef OAL_WRAPPER
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if (p_inter & 0x08000000)
            return ERR_NONE;
        else if (p_inter & 0x04000000)
            return ERR_NONE;
        else
        {
            // time out handling then send cmd12
            err = emmc_cmd_with_res(handler,DIALOG_MMC_STOP_TRANSM,0, rsp);
            return ERR_SINGLE_READ | err;
        }
    }
    else
    {
        // receive rsp?
#ifndef	 BUILD_OPT_VSIM
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );
#else
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000) == 0x00 );
#endif
        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            return ERR_TIME_OUT;
        }

        //  HW start to read
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read done bit[26] read time out bit[20]
        // 실제 해보니 bit[26] 은 안 움직이고 bit[27]이 set이 되었다
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08100000) == 0x00 );

        if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
        {
            // read one block data
            return ERR_NONE;
        }
        else
        {
            // time out handling then send cmd12
            err = emmc_cmd_with_res(handler,DIALOG_MMC_STOP_TRANSM,0, rsp);
            return ERR_SINGLE_READ | err;
        }
    }
}

/* fc9050에서 사용될 일이 없을 듯 함 -> sd2.x는 closed read를 지원 하지 않기 때문에 open으로 해야 한다. */
// 함수의 hif_blk_cnt 가 0x00ffffff로 지정이 되어 있기 때문에 open data transfer
static int emmc_multi_read_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_count)
{

    UINT32 RSP_type, DATA_type, ret_rsp;
    int err= 0;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;


    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(block_count *(512/4))));
#ifndef END_ADDR_ERR
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count) - 1)));
#else
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * (block_count*2)) - 1)));
#endif

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
		OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),1000);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }

    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
        HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);
    }
    else
    {
        // set transfer block count 0x01
#ifndef END_ADDR_ERR
        HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);
#else
        HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, 0x00ffffff);
#endif
    }

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x02 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT;
        }
        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)	// error occur
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_MULTI_READ | ERR_RES;
            }
        }
        HW_REG_WRITE32(emmc->regmap->HIF_ERR_CNT, 0x00ffffff);

        // 이 시점에 원하는 이벤트가 이미 발생 했을 수 있기 때문에 clear 동작을 없앰
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008b00);

#ifdef OAL_WRAPPER
        //OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
		OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, 1000);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x08000000)
        {
            EMMC_PRINTF("multi read interrupt flag 0x%08x\n",p_inter);
        }
        else if (p_inter & 0x00400000)
        {
            err = ERR_READ_BLOCK_CRC;
        }
        // time out handling then send cmd12
#ifdef OAL_WRAPPER
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        err |= emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, &ret_rsp);

        return err;
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            return ERR_TIME_OUT;
        }

#if 0
        //  HW start to read
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read done bit[26] read time out bit[20]
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00100000) == 0x00 )
        {
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x04000000))
            {
                // read block done
            }
        }
#else
        HW_REG_WRITE32(emmc->regmap->HIF_ERR_CNT, 0x00ffffff);
        // check number of read data blocks
        while((HW_REG_READ32(emmc->regmap->HIF_XTR_CNT)) < block_count)
        {
            if (((HW_REG_READ32(emmc->regmap->HIF_ERR_CNT))&0x00ffffff) >= block_count)
            {
                err = ERR_MULTI_READ;
                break;
            }
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
                break;

        }
#endif

        // time out handling then send cmd12
        err |= emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, &ret_rsp);

        return err;
    }
}

// send cmd23 & defined length read
static int emmc_block_read_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_count)
{
    UINT32 RSP_type, DATA_type, ret_rsp/*, read_num = 0,*/;
    EMMC_HANDLER_TYPE *emmc = NULL;
    int err;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    // send cmd23
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SET_BLK_CNT, (block_count & 0x0000ffff), &ret_rsp);
    if(err || (ret_rsp & 0xfff80000))
        return ERR_BLOCK_READ | err | ERR_RES;

    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(block_count *(512/4))));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count) - 1)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
		OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),1000);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }
    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set transfer block count
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x02 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
             xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_BLOCK_READ;
        }
        // true hw stores the received rsp total 48bits
        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                 xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_BLOCK_READ | ERR_RES;
            }
        }

        // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있으므로 clear 동작을 없앰
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008830);
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if (p_inter & 0x08000000)
            return ERR_NONE;
        else if (p_inter & 0x00400000)
        {
            EMMC_PRINTF("brock read ERR intterrupt flag 0x%08x\n",p_inter);
            err = ERR_READ_BLOCK_CRC;
            err |= emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, &ret_rsp);
        }
        else if (p_inter & 0x00100000)
        {
            return ERR_TIME_OUT | ERR_BLOCK_READ;
        }
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            return ERR_TIME_OUT | ERR_BLOCK_READ;
        }
        // clear flag regisger
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
        //  HW start to read
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read done bit[23] read time out bit[20]
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00100000) == 0x00 )
        {
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00800000))
            {
                // read block done
                // [10] bit를 write 하여 [26]bit가 clear 되는지 확인 한다.
                /*
                read_num++;
                if (read_num == block_count)
                return 0;
                */
                return ERR_NONE;
            }
            else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
            {
                return ERR_NONE;
            }
            else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00200000))
            {
                err = ERR_READ_BLOCK_CRC;
                break;
            }
        }
        // time out handling then send cmd12
        err |= emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, &ret_rsp);
    }
    return ERR_BLOCK_READ | err;
}

static int emmc_single_write_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf)
{
    UINT32 RSP_type, DATA_type;/*, ret_rsp;*/
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;
    UINT32 ret_rsp;
    int err = ERR_NONE;

    // block size가 최소 단위가 되기 때문에 end address는 다시 생각할 필요가 있다
    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+127));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
    }

#if manual_trig
    //write trigger manual
    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000200);
    HW_REG_AND_WRITE32(emmc->regmap->HIF_CTL_00, 0xfffffeff);
#endif

    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set transfer block count 0x01
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, 0x01);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x03 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_SINGLE_WRITE;
        }
        if(p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_SINGLE_WRITE | ERR_RES;
            }
        }
        // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있기 때문에 clear동작을 없앰
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008d00);
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if ( p_inter & 0x08000000)
        {
            EMMC_PRINTF("block write done %x\n",p_inter);
        }
        else if (p_inter & 0x04000000)
        {
            EMMC_PRINTF("block write done %x\n",p_inter);
        }
        else if (p_inter & 0x00400000)
        {
            err = ERR_SINGLE_WRITE;
        }
        return err;
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            EMMC_PRINTF("emmc multi wirte fail\n");
            return ERR_TIME_OUT | ERR_SINGLE_WRITE;
        }
#ifdef manual_trig
        HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000300);
#endif

#if 1
        //  HW start to write
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read done bit[23] read time out bit[20]
        // 0x001에서 0x09로 변경
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x01000000) == 0x00 )
        {
            // 0x008 -> 0x080
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
            {
                // write one block data
                // flag reset
                EMMC_PRINTF("emmc one block write done\n");
                //return ERR_NONE;
                break;
            }
        }
#else
        while((HW_REG_READ32(emmc->regmap->HIF_XTR_CNT)) < 1);
#endif
        // time out handling then send cmd12
        err = emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, &ret_rsp);
        if (err)
            return err | ERR_SINGLE_WRITE;

        return ERR_NONE;
    }
}

/* multi write open mode */
static int emmc_multi_write_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_count)
{
    UINT32 RSP_type, DATA_type;
    int err;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    // block size가 최소 단위가 되기 때문에 end address는 다시 생각할 필요가 있다
    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(block_count * 512 / 4)));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count)- 1)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }

    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
        HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);
    }
    else
    {
        // set transfer block count 0x00ffffff
        HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, 0x00ffffff);
    }

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x03 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }
        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_MULTI_WRITE | ERR_RES;
            }
        }
        // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있기 때문에 clear 동작을 없앰
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008900);
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if ( p_inter & 0x08000000)
        {
            EMMC_PRINTF("multi write done\n");
        }
        else
        {
            EMMC_PRINTF("multi write err interupt flag 0x%08x\n", p_inter);
        }
        err = emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, rsp);
        //after busy clear return.
        while(((HW_REG_READ32(emmc->regmap->HIF_BUS_ST)) & 0x01) == 0 );
        if (err)
            return err | ERR_MULTI_WRITE;
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            EMMC_PRINTF("multi write time out\n");
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }

#if 0
        //  HW start to write
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read/write done bit[26] write time out bit[24]
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x01000000) == 0x00 )
        {
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
            {
                // write one block data
                return ERR_NONE;
            }
            else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x02000000))
            {
                EMMC_PRINTF("multi write fail\n");
                return ERR_WRITE_BLOCK_CRC;
            }
        }
#else
        while((HW_REG_READ32(emmc->regmap->HIF_XTR_CNT)) < block_count);
#endif

        // send cmd12
        err = emmc_cmd_with_res(handler, DIALOG_MMC_STOP_TRANSM, 0, rsp);

        //after busy clear return.
        while(((HW_REG_READ32(emmc->regmap->HIF_BUS_ST)) & 0x01) == 0 );
    }
    return err;
}

/* closed mode */
static int emmc_block_write_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_count)
{
    UINT32 RSP_type, DATA_type, ret_rsp;
    int err;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    // send cmd23
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SET_BLK_CNT, (block_count & 0x0000ffff), &ret_rsp);
    if(err || (ret_rsp & 0xfff80000))
        return ERR_BLOCK_READ | err | ERR_RES;

    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count)- 1)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008005);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }

    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x03 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00010000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }
        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
            if (*rsp & 0xfff80000)
            {
#ifdef OAL_WRAPPER
                OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
                xSemaphoreGive( (emmc->emmc_sema));
#endif
                return ERR_MULTI_WRITE | ERR_RES;
            }
        }
        // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있기 때문에 clear 동작을 없앰
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008900);
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
        OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
        xSemaphoreGive( (emmc->emmc_sema));
#endif
        if ( p_inter & 0x08000000)
        {
            EMMC_PRINTF("multi write done\n");
        }
        else if (p_inter & 0x02000000)
        {
            EMMC_PRINTF("multi write err interrupt flag 0x%08x\n", p_inter);
            err = ERR_WRITE_BLOCK_CRC;
        }

        //after busy clear return.
        while(((HW_REG_READ32(emmc->regmap->HIF_BUS_ST)) & 0x01) == 0 );
        if (err)
            return err | ERR_MULTI_WRITE;
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00050000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        else
        {
            // time-out일 경우 처리
            EMMC_PRINTF("multi write time out fail\n");
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }

#if 1
        //  HW start to write
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read/write done bit[26] write time out bit[24]
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x01000000) == 0x00 )
        {
            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
            {
                // write one block data
                return ERR_NONE;
            }
            else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x02000000))
            {
                EMMC_PRINTF("multi write fail\n");
                return ERR_WRITE_BLOCK_CRC;
            }
        }
#else
        while((HW_REG_READ32(emmc->regmap->HIF_XTR_CNT)) < block_count);
#endif

        //after busy clear return.
        while(((HW_REG_READ32(emmc->regmap->HIF_BUS_ST)) & 0x01) == 0 );
    }
    return err;
}

static int sdio_block_read_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_size, UINT32 block_count)
{
    UINT32 RSP_type, DATA_type/*, ret_rsp, read_num = 0,*/;
    EMMC_HANDLER_TYPE *emmc = NULL;

    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;
    int err;

    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (block_size << 16));

    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count))));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count) - 1)));


    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008007);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }


    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set transfer block count
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x02 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        // true hw stores the received rsp total 48bits
        if (p_inter & 0x00030000)
        {
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_BLOCK_READ;
        }

        if (p_inter & 0x00040000)
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);

        if (block_count == 1)
        {
            // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있기 때문에 clear 동작을 없앰
            // HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
            HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008c10);
#ifdef OAL_WRAPPER
            OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            if (p_inter & 0x08000000)
                return ERR_NONE;
            else if (p_inter & 0x04000000)
                return ERR_NONE;
            else
                return ERR_SINGLE_READ;
        }
        else
        {
            // 이 시점에 원하는 flag가 이미 set 되어 있을 수 있기 때문에 clear 동작을 없앰
            //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
            HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008830);
#ifdef OAL_WRAPPER
            OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            if (p_inter & 0x08000000)
            {
                return ERR_NONE;
            }
            else if (p_inter & 0x00200000)
            {
                return ERR_READ_BLOCK_CRC;
            }
            else
                return ERR_BLOCK_READ;
        }
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00070000) == 0x00 );

        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00030000)
        {
            // time-out일 경우 처리
            return ERR_TIME_OUT | ERR_BLOCK_READ;
        }

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }
        // clear flag regisger
        //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
        //  HW start to read
        // if (in_time) // time out이 뜨기전에 data를 읽으면 아래 loop안에서 return
        // 아래 loop를 빠져나올 경우 time out return;  read done bit[23] read time out bit[20]

        if (block_count == 1)
        {
            while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08100000) == 0x00 );

            if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
                return ERR_NONE;
            else
                return ERR_SINGLE_READ;
        }
        else
        {
            while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00100000) == 0x00 )
            {
                if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00800000))
                {
                    return ERR_NONE;
                }
                else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
                {
                    return ERR_NONE;
                }
                else if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00200000))
                {
                    err = ERR_READ_BLOCK_CRC;
                    break;
                }
            }
            return ERR_BLOCK_READ | err;
        }
    }
}

static int sdio_multi_write_data(const HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp, UINT32 *buf, UINT32 block_size, UINT32 block_count)
{
    UINT32 RSP_type, DATA_type;
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    UINT32 p_inter;

    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (block_size << 16));
    // block size가 최소 단위가 되기 때문에 end address는 다시 생각할 필요가 있다
    // set ahb address, hif_ahb_sa(start address) hif_ahb_ea(end address)
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_SA, (UINT32)(buf));
    HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count)- 1)));
    //HW_REG_WRITE32(emmc->regmap->HIF_AHB_EA, (UINT32)((buf)+(((HW_REG_READ32(emmc->regmap->HIF_BLK_LG) >> 18) * block_count))));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_OBTAIN_SEMAPHORE(&(emmc->emmc_sema),OAL_SUSPEND);
#else
        xSemaphoreTake( (emmc->emmc_sema), ( TickType_t ) 1000 );
#endif
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
        HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008007);
    }
    else
    {
        // clear flag regisger
        HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x00070000);
    }

    // cmdq_trig = 0
    while(HW_REG_READ32(emmc->regmap->HIF_CMD_IDX) & (1<<15) );

    // set transfer block count 0x00ffffff
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_CNT, block_count);

    // set arg
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_ARG, cmd_arg);

    RSP_type = 0x02 << 11;
    DATA_type = (0x03 << 9);
    HW_REG_WRITE32(emmc->regmap->HIF_CMD_IDX, (cmd | RSP_type | DATA_type | (0x01 << 15)));

    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
#ifdef OAL_WRAPPER
        OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
#else
        xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
#endif
        if (p_inter & 0x00030000)
        {
            // time-out일 경우 처리
            EMMC_PRINTF("multi write time out fail\n");
#ifdef OAL_WRAPPER
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }

        if (p_inter & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }

        if (block_count == 1)
        {
            // 이 시점에 이미 이벤트가 발생되어 있을 수 있기 때문에 인터럽트만 enable하여서 이벤트를 받는다.
            //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
            HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008d00);
#ifdef OAL_WRAPPER
            OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            if (p_inter & 0x08000000)
            {
                EMMC_PRINTF("sdio one block write done\n");
                return ERR_NONE;
            }
            else if (p_inter & 0x04000000)
            {
                EMMC_PRINTF("sdio one block write done\n");
                return ERR_NONE;
            }
            else
            {
                EMMC_PRINTF("sdio one block write error interrupt flag 0x%08x\n", p_inter);
                return ERR_MULTI_WRITE;
            }
        }
        else
        {
            //HW_REG_OR_WRITE32(emmc->regmap->HIF_EVNT_CTL, 0x0fff0000);
            HW_REG_WRITE32(emmc->regmap->HIF_INT_CTL, 0x00008900);
#ifdef OAL_WRPPER
            OAL_RECEIVE_FROM_QUEUE(&(emmc->emmc_queue), (VOID *)(&(p_inter)) ,0, 0, OAL_SUSPEND);
            OAL_RELEASE_SEMAPHORE(&(emmc->emmc_sema));
#else
            xQueueReceive((emmc->emmc_queue),(VOID *)(&(p_inter)), 1000);
            xSemaphoreGive( (emmc->emmc_sema));
#endif
            if (p_inter & 0x08000000)
            {
                EMMC_PRINTF("sdio blocks write done\n");
                return ERR_NONE;
            }
            else
            {
                EMMC_PRINTF("sdio blocks write error interrupt flag 0x%08x\n", p_inter);
                return ERR_MULTI_WRITE;
            }
        }
    }
    else
    {
        // receive rsp?
        while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00070000) == 0x00 );

        // true hw stores the received rsp total 48bits
        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00040000)
        {
            *rsp = HW_REG_READ32(emmc->regmap->HIF_RSP_ARG_0);
        }

        if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x00030000)
        {
            // time-out일 경우 처리
            EMMC_PRINTF("multi write time out fail\n");
            return ERR_TIME_OUT | ERR_MULTI_WRITE;
        }

        if (block_count == 1)
        {
            while((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x01000000) == 0x00 )
            {
                // 0x008 -> 0x080
                if ((HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x08000000))
                {
                    // write one block data
                    // flag reset
                    EMMC_PRINTF("sdio one block write done\n");
                    //return ERR_NONE;
                    break;
                }
            }
        }
        else
        {
            while((HW_REG_READ32(emmc->regmap->HIF_XTR_CNT)) < block_count);
        }
        //after busy clear return.
        while(((HW_REG_READ32(emmc->regmap->HIF_BUS_ST)) & 0x01) == 0 );
        return ERR_NONE;
    }
}

static int sdio_io_rw_direct(const HANDLE handler, UINT8 write, UINT8 fn, UINT32 addr, UINT8 in, UINT8 *value)
{
    UINT32 arg, rsp;
    int err;

    arg = (write == 1) ? 0x80000000 : 0x00000000;
    arg |= fn << 28;
    arg |= addr << 9;
    arg |= in;

    err = emmc_cmd_with_res(handler, DIALOG_SDIO_RW_DIR, arg, &rsp);
    if (!write)
        *value = rsp & 0xff;
    return err;
}

static int sdio_io_rw_extended(const HANDLE handler, UINT8 write, UINT8 fn,	UINT32 addr, UINT32 incr_addr, UINT8 *buf, UINT32 block_count, UINT32 blksz)
{
    UINT32 arg, rsp;
    /* sanity check */
    if (addr & ~0x1FFFF)
        return ERR_INVAL;

    arg = write ? 0x80000000 : 0x00000000;
    arg |= fn << 28;
    arg |= incr_addr ? 0x04000000 : 0x00000000;
    arg |= addr << 9;

    if (blksz == 1 && block_count <= 512)
        arg |= block_count;	/* byte mode */ //arg |= 1;	/* byte mode */
    else
        arg |= 0x08000000 | block_count;		/* block mode */


    if (write)		// write
    {
        if (arg & 0x08000000)
            sdio_multi_write_data(handler, DIALOG_SDIO_RW_EXT, arg, &rsp, (UINT32*)buf, blksz, block_count);
        else
            sdio_multi_write_data(handler, DIALOG_SDIO_RW_EXT, arg, &rsp, (UINT32*)buf, block_count, blksz);
    }
    else		// read
    {
        if (arg & 0x08000000)
            sdio_block_read_data(handler, DIALOG_SDIO_RW_EXT, arg, &rsp, (UINT32*)buf, blksz, block_count);
        else
            sdio_block_read_data(handler, DIALOG_SDIO_RW_EXT, arg, &rsp, (UINT32*)buf, block_count, blksz);
    }

    if (rsp & DIALOG_R5_ERROR)
        return ERR_SDIO_IO;
    if (rsp & DIALOG_R5_FUNC_NO)
        return ERR_INVAL;
    if (rsp & DIALOG_R5_OUT_OF_RANGE)
        return ERR_SDIO_RANGE;

    return ERR_NONE;
}

static const unsigned int tran_exp[] = {
    10000,		100000,		1000000,	10000000,
    0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
    0,	10,	12,	13,	15,	20,	25,	30,
    35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
    1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
    0,	10,	12,	13,	15,	20,	25,	30,
    35,	40,	45,	50,	55,	60,	70,	80,
};


static inline UINT32 BITS_FOR_UNSTUFF(UINT32 *resp, UINT32 start, UINT32 size)
{
    INT32 __size = (INT32)size;
    UINT32 __mask = (__size < 32 ? 1 << __size : 0) - 1;
    INT32 __off = 3 - ((start) / 32);
    INT32 __shft = (start) & 31;
    UINT32 __res;

    __res = resp[__off] >> __shft;
    if (__size + __shft > 32)
        __res |= resp[__off-1] << ((32 - __shft) % 32);
    return __res & __mask;
}



static int emmc_decode_csd(HANDLE handler)
{
    EMMC_HANDLER_TYPE *dialog_local_emmc = NULL;
    dialog_local_emmc = (EMMC_HANDLER_TYPE *)handler;

    struct mmc_csd *tmp_mmc_csd = &(dialog_local_emmc->csd);
    UINT32 e, m, a, b;
    UINT32 *decode_rsp = dialog_local_emmc->raw_csd;

    tmp_mmc_csd->mmc_struct = (UINT8)BITS_FOR_UNSTUFF(decode_rsp, 126, 2);
    if (tmp_mmc_csd->mmc_struct == 0) {
        return ERR_CSD;
    }

    tmp_mmc_csd->mmca_vsn = (UINT8)BITS_FOR_UNSTUFF(decode_rsp, 122, 4);
    m = (UINT32)BITS_FOR_UNSTUFF(decode_rsp, 115, 4);
    e = (UINT32)BITS_FOR_UNSTUFF(decode_rsp, 112, 3);
    tmp_mmc_csd->ns_for_tacc	= (tacc_exp[e] * tacc_mant[m] + 9) / 10;
    tmp_mmc_csd->clks_for_tacc	= (UINT16)BITS_FOR_UNSTUFF(decode_rsp, 104, 8) * 100;

    m = BITS_FOR_UNSTUFF(decode_rsp, 99, 4);
    e = BITS_FOR_UNSTUFF(decode_rsp, 96, 3);
    tmp_mmc_csd->max_dtr	  = tran_exp[e] * tran_mant[m];
    tmp_mmc_csd->command_class	  = (UINT16)BITS_FOR_UNSTUFF(decode_rsp, 84, 12);

    e = BITS_FOR_UNSTUFF(decode_rsp, 47, 3);
    m = BITS_FOR_UNSTUFF(decode_rsp, 62, 12);
    tmp_mmc_csd->capa_size	  = (1 + m) << (e + 2);

    tmp_mmc_csd->rd_bits_for_blk = BITS_FOR_UNSTUFF(decode_rsp, 80, 4);
    tmp_mmc_csd->rd_part = BITS_FOR_UNSTUFF(decode_rsp, 79, 1);
    tmp_mmc_csd->wr_misalign_flag = BITS_FOR_UNSTUFF(decode_rsp, 78, 1);
    tmp_mmc_csd->rd_misalign_flag = BITS_FOR_UNSTUFF(decode_rsp, 77, 1);
    tmp_mmc_csd->factor_for_r2w = BITS_FOR_UNSTUFF(decode_rsp, 26, 3);
    tmp_mmc_csd->wr_bits_for_blk = BITS_FOR_UNSTUFF(decode_rsp, 22, 4);
    tmp_mmc_csd->wr_part = BITS_FOR_UNSTUFF(decode_rsp, 21, 1);

    if (tmp_mmc_csd->wr_bits_for_blk >= 9) {
        a = BITS_FOR_UNSTUFF(decode_rsp, 42, 5);
        b = BITS_FOR_UNSTUFF(decode_rsp, 37, 5);
        tmp_mmc_csd->sz_for_erase = (a + 1) * (b + 1);
        tmp_mmc_csd->sz_for_erase <<= tmp_mmc_csd->wr_bits_for_blk - 9;
    }

    return ERR_NONE;
}

static int emmc_decode_ext_csd(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE *)handler;
    int err;

    if (emmc->pext_csd == NULL)
        return ERR_CSD;

    emmc->ext_csd.raw_ext_csd_structure = emmc->pext_csd[E_CSD_STRUCTURE];
    if (emmc->csd.mmc_struct == 3) {
        if (emmc->ext_csd.raw_ext_csd_structure > 2) {
            EMMC_PRINTF( "unrecognised EXT_CSD structure version\n");
            err = ERR_EXT_CSD;
            return err;
        }
    }

    emmc->ext_csd.rev = emmc->pext_csd[E_CSD_REV];
#if 0   // EMMC revision 5.01 support
    if (emmc->ext_csd.rev > 6) {
        EMMC_PRINTF( "unrecognised EXT_CSD revision\n");
        err = ERR_EXT_CSD;
        return err;
    }
#endif

    emmc->ext_csd.raw_sectors[0] = emmc->pext_csd[E_CSD_SEC_CNT + 0];
    emmc->ext_csd.raw_sectors[1] = emmc->pext_csd[E_CSD_SEC_CNT + 1];
    emmc->ext_csd.raw_sectors[2] = emmc->pext_csd[E_CSD_SEC_CNT + 2];
    emmc->ext_csd.raw_sectors[3] = emmc->pext_csd[E_CSD_SEC_CNT + 3];
    if (emmc->ext_csd.rev >= 2) {
        emmc->ext_csd.sectors =
            emmc->pext_csd[E_CSD_SEC_CNT + 0] << 0 |
            emmc->pext_csd[E_CSD_SEC_CNT + 1] << 8 |
            emmc->pext_csd[E_CSD_SEC_CNT + 2] << 16 |
            emmc->pext_csd[E_CSD_SEC_CNT + 3] << 24;

#if 0
        /* mmc card 의 용량이 2GB 아래면 byte 단위의 address 형식을 가진다. */
        /* emmcs with density > 2GiB are sector addressed */
        if (emmc->ext_csd.sectors > (2u * 1024 * 1024 * 1024) / 512)
            mmc_CARD_set_blockaddr(emmc);
#endif
    }
    emmc->ext_csd.raw_card_type = emmc->pext_csd[E_CSD_CD_TYPE];
    switch (emmc->pext_csd[E_CSD_CD_TYPE] & E_CSD_CD_TYPE_MASK) {
        case E_CSD_CD_TYPE_DDR_52 | E_CSD_CD_TYPE_52 | E_CSD_CD_TYPE_26:
            emmc->ext_csd.hs_max_dtr = 52000000;
            emmc->ext_csd.card_type = E_CSD_CD_TYPE_DDR_52;
            break;
            case E_CSD_CD_TYPE_DDR_1_2V | E_CSD_CD_TYPE_52 | E_CSD_CD_TYPE_26:
                emmc->ext_csd.hs_max_dtr = 52000000;
                emmc->ext_csd.card_type = E_CSD_CD_TYPE_DDR_1_2V;
                break;
                case E_CSD_CD_TYPE_DDR_1_8V | E_CSD_CD_TYPE_52 | E_CSD_CD_TYPE_26:
                    emmc->ext_csd.hs_max_dtr = 52000000;
                    emmc->ext_csd.card_type = E_CSD_CD_TYPE_DDR_1_8V;
                    break;
                case E_CSD_CD_TYPE_52 | E_CSD_CD_TYPE_26:
                    emmc->ext_csd.hs_max_dtr = 52000000;
                    break;
                case E_CSD_CD_TYPE_26:
                    emmc->ext_csd.hs_max_dtr = 26000000;
                    break;
                default:
                    /* MMC v4 spec says this cannot happen */
                    EMMC_PRINTF("emmc is mmc v4 but doesn't support any high-speed modes.\n");
                    break;
    }

    emmc->ext_csd.raw_s_a_timeout = emmc->pext_csd[E_CSD_S_A_TIMEOUT];
    emmc->ext_csd.raw_erase_timeout_mult = emmc->pext_csd[E_CSD_ER_TO_MULT];
    emmc->ext_csd.raw_hc_erase_grp_size = emmc->pext_csd[E_CSD_HC_ERASE_GRP_SIZE];
    if (emmc->ext_csd.rev >= 3) {
        UINT8 sa_shift = emmc->pext_csd[E_CSD_S_A_TIMEOUT];
        emmc->ext_csd.part_config = emmc->pext_csd[E_CSD_PART_CFG];

        /* EXT_CSD value is in units of 10ms, but we store in ms */
        emmc->ext_csd.part_time = 10 * emmc->pext_csd[E_CSD_PART_SWCH_TM];

        /* Sleep / awake timeout in 100ns units */
        if (sa_shift > 0 && sa_shift <= 0x17)
            emmc->ext_csd.sa_timeout = 1 << emmc->pext_csd[E_CSD_S_A_TIMEOUT];
        emmc->ext_csd.erase_group_def = emmc->pext_csd[E_CSD_ER_GRP_DEF];
        emmc->ext_csd.hc_erase_timeout = 300 * emmc->pext_csd[E_CSD_ER_TO_MULT];
        emmc->ext_csd.hc_erase_size = emmc->pext_csd[E_CSD_HC_ERASE_GRP_SIZE] << 10;

        emmc->ext_csd.rel_sectors = emmc->pext_csd[E_CSD_REL_WR_SEC_C];

        /*
        * There are two boot regions of equal size, defined in
        * multiples of 128K.
        */
        emmc->ext_csd.boot_size = emmc->pext_csd[E_CSD_BOOT_MULT] << 17;
    }

    emmc->ext_csd.raw_hc_erase_gap_size = emmc->pext_csd[E_CSD_PART_ATTR];
    emmc->ext_csd.raw_sec_trim_mult = emmc->pext_csd[E_CSD_SEC_TRIM_MULT];
    emmc->ext_csd.raw_sec_erase_mult = emmc->pext_csd[E_CSD_SEC_ERASE_MULT];
    emmc->ext_csd.raw_sec_feature_support = emmc->pext_csd[E_CSD_SEC_FT_SUPP];
    emmc->ext_csd.raw_trim_mult = emmc->pext_csd[E_CSD_TRIM_MULT];
    if (emmc->ext_csd.rev >= 4) {
        /*
        * Enhanced area feature support -- check whether the eMMC
        * emmc has the Enhanced area enabled.  If so, export enhanced
        * area offset and size to user by adding sysfs interface.
        */
        emmc->ext_csd.raw_partition_support = emmc->pext_csd[E_CSD_PART_SUPP];
        if ((emmc->pext_csd[E_CSD_PART_SUPP] & 0x2) && (emmc->pext_csd[E_CSD_PART_ATTR] & 0x1)) {
            UINT8 hc_erase_grp_sz = emmc->pext_csd[E_CSD_HC_ERASE_GRP_SIZE];
            UINT8 hc_wp_grp_sz = emmc->pext_csd[E_CSD_HC_WP_GRP_SIZE];

            emmc->ext_csd.enhanced_area_en = 1;
            /*
            * calculate the enhanced data area offset, in bytes
            */
            emmc->ext_csd.enhanced_area_offset = (emmc->pext_csd[139] << 24) + (emmc->pext_csd[138] << 16) + (emmc->pext_csd[137] << 8) + emmc->pext_csd[136];
#if 0		// block addr 함수 기능 확인 필요
            if (mmc_CARD_blockaddr(emmc))
                emmc->ext_csd.enhanced_area_offset <<= 9;
#endif
            /*
            * calculate the enhanced data area size, in kilobytes
            */
            emmc->ext_csd.enhanced_area_size = (emmc->pext_csd[142] << 16) + (emmc->pext_csd[141] << 8) + emmc->pext_csd[140];
            emmc->ext_csd.enhanced_area_size *= (size_t)(hc_erase_grp_sz * hc_wp_grp_sz);
            emmc->ext_csd.enhanced_area_size <<= 9;
        } else {
            /*
            * If the enhanced area is not enabled, disable these
            * device attributes.
            */
            emmc->ext_csd.enhanced_area_offset = -1;
            emmc->ext_csd.enhanced_area_size = -1;
        }
        emmc->ext_csd.sec_trim_mult = emmc->pext_csd[E_CSD_SEC_TRIM_MULT];
        emmc->ext_csd.sec_erase_mult = emmc->pext_csd[E_CSD_SEC_ERASE_MULT];
        emmc->ext_csd.sec_feature_support = emmc->pext_csd[E_CSD_SEC_FT_SUPP];
        emmc->ext_csd.trim_timeout = 300 * emmc->pext_csd[E_CSD_TRIM_MULT];
    }

    if (emmc->ext_csd.rev >= 5) {
#if 0
        /* check whether the eMMC emmc supports HPI */
        if (ext_csd[E_CSD_HPI_FEATURES] & 0x1) {
            emmc->ext_csd.hpi = 1;
            if (ext_csd[E_CSD_HPI_FEATURES] & 0x2)
                emmc->ext_csd.hpi_cmd =	DIALOG_MMC_STOP_TRANSM;
            else
                emmc->ext_csd.hpi_cmd = MMC_SEND_STATUS;
            /*
            * Indicate the maximum timeout to close
            * a command interrupted by HPI
            */
            emmc->ext_csd.out_of_int_time =
                ext_csd[E_CSD_OUT_OF_INTERRUPT_TIME] * 10;
        }

        emmc->ext_csd.rel_param = ext_csd[E_CSD_WR_REL_PARAM];
        emmc->ext_csd.rst_n_function = ext_csd[E_CSD_RST_N_FUNCTION];
#endif
        EMMC_PRINTF("da16200 does not support ver5\n");
    }

    emmc->ext_csd.raw_erased_mem_count = emmc->pext_csd[E_CSD_ERASED_MEM_CONT];
    if (emmc->pext_csd[E_CSD_ERASED_MEM_CONT])
        emmc->erased_byte = 0xFF;
    else
        emmc->erased_byte = 0x0;
#if 0
    /* eMMC v4.5 or later */
    if (emmc->ext_csd.rev >= 6) {
        emmc->ext_csd.feature_support |= MMC_DISCARD_FEATURE;

        emmc->ext_csd.generic_cmd6_time = 10 *
            ext_csd[E_CSD_GENERIC_CMD6_TIME];
        emmc->ext_csd.power_off_longtime = 10 *
            ext_csd[E_CSD_POWER_OFF_LONG_TIME];

        emmc->ext_csd.cache_size =
            ext_csd[E_CSD_CACHE_SIZE + 0] << 0 |
            ext_csd[E_CSD_CACHE_SIZE + 1] << 8 |
            ext_csd[E_CSD_CACHE_SIZE + 2] << 16 |
            ext_csd[E_CSD_CACHE_SIZE + 3] << 24;

        emmc->ext_csd.max_packed_writes =
            ext_csd[E_CSD_MAX_PACKED_WRITES];
        emmc->ext_csd.max_packed_reads =
            ext_csd[E_CSD_MAX_PACKED_READS];
    }
#endif

    return ERR_NONE;
}

static int sd_decode_csd(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE *)handler;
    struct mmc_csd *csd = &(emmc->csd);
    UINT32 e, m;
    UINT32 *resp = emmc->raw_csd;


    csd->mmc_struct = (UINT8)BITS_FOR_UNSTUFF(resp, 126, 2);

    switch (csd->mmc_struct) {
    case 0:
        m = BITS_FOR_UNSTUFF(resp, 115, 4);
        e = BITS_FOR_UNSTUFF(resp, 112, 3);
        csd->ns_for_tacc	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
        csd->clks_for_tacc	 = (UINT16)BITS_FOR_UNSTUFF(resp, 104, 8) * 100;

        m = BITS_FOR_UNSTUFF(resp, 99, 4);
        e = BITS_FOR_UNSTUFF(resp, 96, 3);
        csd->max_dtr	  = tran_exp[e] * tran_mant[m];
        csd->command_class	  = (UINT16)BITS_FOR_UNSTUFF(resp, 84, 12);

        e = BITS_FOR_UNSTUFF(resp, 47, 3);
        m = BITS_FOR_UNSTUFF(resp, 62, 12);
        csd->capa_size	  = (1 + m) << (e + 2);

        csd->rd_bits_for_blk = (UINT32)BITS_FOR_UNSTUFF(resp, 80, 4);
        csd->rd_part = BITS_FOR_UNSTUFF(resp, 79, 1);
        csd->wr_misalign_flag = BITS_FOR_UNSTUFF(resp, 78, 1);
        csd->rd_misalign_flag = BITS_FOR_UNSTUFF(resp, 77, 1);
        csd->factor_for_r2w = (UINT32)BITS_FOR_UNSTUFF(resp, 26, 3);
        csd->wr_bits_for_blk = (UINT32)BITS_FOR_UNSTUFF(resp, 22, 4);
        csd->wr_part = BITS_FOR_UNSTUFF(resp, 21, 1);

        if (BITS_FOR_UNSTUFF(resp, 46, 1)) {
            csd->sz_for_erase = 1;
        } else if (csd->wr_bits_for_blk >= 9) {
            csd->sz_for_erase = BITS_FOR_UNSTUFF(resp, 39, 7) + 1;
            csd->sz_for_erase <<= csd->wr_bits_for_blk - 9;
        }
        break;
    case 1:
        /*
        * This is a block-addressed SDHC or SDXC emmc. Most
        * interesting fields are unused and have fixed
        * values. To avoid getting tripped by buggy emmcs,
        * we assume those fixed values ourselves.
        */

        //mmc_emmc_set_blockaddr(emmc);

        csd->ns_for_tacc	 = 0; /* Unused */
        csd->clks_for_tacc	 = 0; /* Unused */

        m = BITS_FOR_UNSTUFF(resp, 99, 4);
        e = BITS_FOR_UNSTUFF(resp, 96, 3);
        csd->max_dtr	  = tran_exp[e] * tran_mant[m];
        csd->command_class	  = (UINT16)BITS_FOR_UNSTUFF(resp, 84, 12);
        csd->c_sz	  = BITS_FOR_UNSTUFF(resp, 48, 22);

        /* SDXC emmcs have a minimum C_SIZE of 0x00FFFF */
        /*
        if (csd->c_sz >= 0xFFFF)
        mmc_emmc_set_ext_capacity(emmc);
        */
        m = BITS_FOR_UNSTUFF(resp, 48, 22);
        csd->capa_size     = (1 + m) << 10;

        csd->rd_bits_for_blk = 9;
        csd->rd_part = 0;
        csd->wr_misalign_flag = 0;
        csd->rd_misalign_flag = 0;
        csd->factor_for_r2w = 4; /* Unused */
        csd->wr_bits_for_blk = 9;
        csd->wr_part = 0;
        csd->sz_for_erase = 1;
        break;
    default:
        EMMC_PRINTF("%d is unrecognised CSD structure version\n",csd->mmc_struct);
        return ERR_CSD;
    }

    //emmc->sz_for_erase = csd->sz_for_erase;

    return ERR_NONE;
}

static int emmc_decode_cid(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    UINT32 *resp = emmc->raw_cid;

    /*
    * The selection of the format here is based upon published
    * specs from sandisk and from what people have reported.
    */
    switch (emmc->csd.mmca_vsn) {
    case 0: /* MMC v1.0 - v1.2 */
    case 1: /* MMC v1.4 */
        emmc->cid.manfid	= BITS_FOR_UNSTUFF(resp, 104, 24);
        emmc->cid.prod_name[0]	= (INT8)BITS_FOR_UNSTUFF(resp, 96, 8);
        emmc->cid.prod_name[1]	= (INT8)BITS_FOR_UNSTUFF(resp, 88, 8);
        emmc->cid.prod_name[2]	= (INT8)BITS_FOR_UNSTUFF(resp, 80, 8);
        emmc->cid.prod_name[3]	= (INT8)BITS_FOR_UNSTUFF(resp, 72, 8);
        emmc->cid.prod_name[4]	= (INT8)BITS_FOR_UNSTUFF(resp, 64, 8);
        emmc->cid.prod_name[5]	= (INT8)BITS_FOR_UNSTUFF(resp, 56, 8);
        emmc->cid.prod_name[6]	= (INT8)BITS_FOR_UNSTUFF(resp, 48, 8);
        emmc->cid.hwrev		= (UINT8)BITS_FOR_UNSTUFF(resp, 44, 4);
        emmc->cid.fwrev		= (UINT8)BITS_FOR_UNSTUFF(resp, 40, 4);
        emmc->cid.serial	= (UINT32)BITS_FOR_UNSTUFF(resp, 16, 24);
        emmc->cid.month		= (UINT8)BITS_FOR_UNSTUFF(resp, 12, 4);
        emmc->cid.year		= (UINT16)BITS_FOR_UNSTUFF(resp, 8, 4) + 1997;
        break;

    case 2: /* MMC v2.0 - v2.2 */
    case 3: /* MMC v3.1 - v3.3 */
    case 4: /* MMC v4 */
        emmc->cid.manfid	= BITS_FOR_UNSTUFF(resp, 120, 8);
        emmc->cid.oemid		= (UINT16)BITS_FOR_UNSTUFF(resp, 104, 16);
        emmc->cid.prod_name[0]	= (INT8)BITS_FOR_UNSTUFF(resp, 96, 8);
        emmc->cid.prod_name[1]	= (INT8)BITS_FOR_UNSTUFF(resp, 88, 8);
        emmc->cid.prod_name[2]	= (INT8)BITS_FOR_UNSTUFF(resp, 80, 8);
        emmc->cid.prod_name[3]	= (INT8)BITS_FOR_UNSTUFF(resp, 72, 8);
        emmc->cid.prod_name[4]	= (INT8)BITS_FOR_UNSTUFF(resp, 64, 8);
        emmc->cid.prod_name[5]	= (INT8)BITS_FOR_UNSTUFF(resp, 56, 8);
        emmc->cid.serial	= (UINT32)BITS_FOR_UNSTUFF(resp, 16, 32);
        emmc->cid.month		= (UINT8)BITS_FOR_UNSTUFF(resp, 12, 4);
        emmc->cid.year		= (UINT16)BITS_FOR_UNSTUFF(resp, 8, 4) + 1997;
        break;
    default:
        EMMC_PRINTF("emmc has unknown MMCA version %d\n", emmc->csd.mmca_vsn);
        return ERR_CID;
    }
    return ERR_NONE;
}

static int sd_decode_cid(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    UINT32 *resp = emmc->raw_cid;

    emmc->cid.manfid		= BITS_FOR_UNSTUFF(resp, 120, 8);
    emmc->cid.oemid			= (UINT16)BITS_FOR_UNSTUFF(resp, 104, 16);
    emmc->cid.prod_name[0]		= (INT8)BITS_FOR_UNSTUFF(resp, 96, 8);
    emmc->cid.prod_name[1]		= (INT8)BITS_FOR_UNSTUFF(resp, 88, 8);
    emmc->cid.prod_name[2]		= (INT8)BITS_FOR_UNSTUFF(resp, 80, 8);
    emmc->cid.prod_name[3]		= (INT8)BITS_FOR_UNSTUFF(resp, 72, 8);
    emmc->cid.prod_name[4]		= (INT8)BITS_FOR_UNSTUFF(resp, 64, 8);
    emmc->cid.hwrev			= (UINT8)BITS_FOR_UNSTUFF(resp, 60, 4);
    emmc->cid.fwrev			= (UINT8)BITS_FOR_UNSTUFF(resp, 56, 4);
    emmc->cid.serial		= (UINT32)BITS_FOR_UNSTUFF(resp, 24, 32);
    emmc->cid.year			= (UINT16)BITS_FOR_UNSTUFF(resp, 12, 8);
    emmc->cid.month			= (UINT8)BITS_FOR_UNSTUFF(resp, 8, 4);

    emmc->cid.year += 2000; /* SD emmcs year offset */

    return ERR_NONE;
}

/* scr data는 data0 line으로 수신되기 때문에 AMBA에 적재되는 모양이 다르다. */
static int mmc_decode_scr(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    struct dialog_sd_scr *scr = &(emmc->scr);
    unsigned int scr_struct;
    UINT32 resp[4];

    resp[3]	= ntoh32(emmc->raw_scr[1]);
    resp[2]	= ntoh32(emmc->raw_scr[0]);
    resp[1] = 0;
    resp[0] = 0;

    scr_struct = BITS_FOR_UNSTUFF(resp, 60, 4);
    if (scr_struct != 0) {
        return ERR_SCR;
    }

    scr->dialog_sda_vsn = (unsigned char)BITS_FOR_UNSTUFF(resp, 56, 4);
    scr->dialog_bus_wid = (unsigned char)BITS_FOR_UNSTUFF(resp, 48, 4);
    if (scr->dialog_sda_vsn == SCR_SPEC_VER_2)
        /* Check if Physical Layer Spec v3.0 is supported */
        scr->dialog_sda_spec3 = (unsigned char)BITS_FOR_UNSTUFF(resp, 47, 1);
    /*
    if (BITS_FOR_UNSTUFF(resp, 55, 1))
    card->erased_byte = 0xFF;
    else
    card->erased_byte = 0x0;
    */
    if (scr->dialog_sda_spec3)
        scr->dialog_sd_cmds = (unsigned char)BITS_FOR_UNSTUFF(resp, 32, 2);
    return 0;
}

static int mmd_decode_ssr(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    struct dialog_sd_ssr *ssr = &(emmc->ssr);
    unsigned int au, es, et, eo;
    UINT32 *resp, i;

    for (i = 0; i < 16; i++)
        emmc->raw_ssr[i] = ntoh32(emmc->raw_ssr[i]);

    resp = emmc->raw_ssr;

    ssr->bus_width = BITS_FOR_UNSTUFF(resp, 510 - 384, 2);
    ssr->cd_type = (unsigned short)BITS_FOR_UNSTUFF(resp, 480 - 384, 16);
    ssr->area_protected = BITS_FOR_UNSTUFF(resp, 448 - 384, 32);
    ssr->spd_class = BITS_FOR_UNSTUFF(resp, 440 - 384, 8);
    ssr->perf_mv = (unsigned char)BITS_FOR_UNSTUFF(resp, 432 - 384, 8);
    au = BITS_FOR_UNSTUFF(resp, 428 - 384, 4);
    ssr->au = 1 << (au+4);
    es = BITS_FOR_UNSTUFF(resp, 408 - 384, 16);
    et = BITS_FOR_UNSTUFF(resp, 402 - 384, 6);
    eo = BITS_FOR_UNSTUFF(resp, 400 - 384, 2);
    if (es && et)
    {
        ssr->erase_to = (et*1000) / es;		/* In milliseconds */
        ssr->erase_off= eo * 1000;				/* In milliseconds */
    }

    return 0;
}


static int emmc_wait_ready(HANDLE handler, UINT32 *rocr)
{
	DA16X_UNUSED_ARG(rocr);

    UINT32 rsp, try_count = 100, i;
    int err;

    for (i = 0; i < try_count; i++)
    {
        err = emmc_cmd_with_res(handler, DIALOG_MMC_SEND_OP_COND, 0x40ff8080 | 0x01<< 30, &rsp);
        if (err)
        {
            return err;
        }
        if ((rsp == 0x80ff8080) || (rsp == 0xc0ff8080))
        {
            return ERR_NONE;
        }
        rsp = 0;
        EMMC_MSLEEP(500);
    }
    return ERR_CARD_NOT_READY;

}

static UINT32 emmc_make_switch_arg(UINT8 set, UINT8 index, UINT8 value)
{
    return (UINT32)((MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8) | set);
}

static UINT32 sd_make_switch_arg(int mode, int group, int value)
{
    UINT32 arg;
    arg = mode << 31 | 0x00FFFFFF;
    arg &= ~(0xF << (group * 4));
    arg |= value << (group * 4);

    return arg;
}

static int emmc_send_op_cond(HANDLE handler, UINT32 ocr, UINT32 *rocr)
{
    UINT32 rsp;
    int err;

    DA16X_UNUSED_ARG(ocr);
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEND_OP_COND, 0, &rsp);
    if (err)
        return ERR_SEND_OP_COND | err;

    if (rsp == 0x00ff8080)
    {
        // device capacity is less than or to 2GB
        EMMC_PRINTF("emmc capacity is less than or to 2GB\n");
        err = emmc_wait_ready(handler,&rsp);
    }
    else if (rsp == 0x40ff8080)
    {
        EMMC_PRINTF("emmc capacity is greater than 2GB\n");
        err = emmc_wait_ready(handler,&rsp);
    }
    else
    {
        EMMC_PRINTF("emmc ocr response 0x%08x\n", rsp);
        return ERR_UNKNOWN_OCR;
    }

    *rocr = rsp;
    return err;
}

static int emmc_init_card(HANDLE handler, UINT32 ocr)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    UINT32 rocr, rsp;
    //UINT32 CID_RES[4], CSD_RES[4];		// memcpy를 없애기 위해 raw_data를 바로 전달 한다.
    //UINT32 EXT_CSD[128];					// byte access를 편하게 하기 위해서 malloc으로 변경함
    int err;

    DA16X_UNUSED_ARG(ocr);
    emmc = (EMMC_HANDLER_TYPE*)handler;
    // push pull up  // 초기화에서 원래는 open drian 방식인데 FPGA 특성인지 몰라도 push pull up 모드여야 초기화가 된다.
    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000001);
#if 0
    err = emmc_send_op_cond(handler, ocr, &rocr);
    EMMC_PRINTF("HIF_EVNT_CTL 0x%08x\n",HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL));
    if (err)
        EMMC_PRINTF("emmc_send_op_cond error\n");
    // fc9050 support spi mode

    // 받은 ocr 값을 토데로 전압 설정 지원 가능한 전압인지 확인 할 필요가 없다고 simo에서 답변함
#endif
    // mmc_go_idle
    emmc_cmd_without_res(handler, DIALOG_MMC_GO_IDLE_ST, 0);
    // delay 1ms
    EMMC_MSLEEP(1);
    // we support high capacity
    err = emmc_send_op_cond(handler, 0, &rocr);
    if (err)
    {
        EMMC_PRINTF("emmc_send_op_cond error\n");
        return err;
    }

    // set block length
    //err = emmc_cmd_with_res(handler, MMC_SET_BLOCKLEN, 512, &rsp);

    // send cid
    err = emmc_cmd_with_res(handler, DIALOG_MMC_ALL_SEND_CID, 0, emmc->raw_cid);
    //memcpy(emmc->raw_cid, CID_RES, sizeof(emmc->raw_cid));
    if (err)
    {
        EMMC_PRINTF("emmc_cid_error\n");
        err = ERR_CID;
        goto MMC_INIT_OUT;
    }
    EMMC_MSLEEP(emmc_delay);

    // sets the rca to 1 // test
    emmc->rca = 1; // 이 값을 변경해 가면서 sector write가 어떻게 되는지 확인이 필요하다.
    // set_relative_addr
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SET_REL_ADDR, emmc->rca << 16, &rsp);
    if (err)
    {
        EMMC_PRINTF("emmc_set_relative_addr\n");
        goto MMC_INIT_OUT;
    }
    EMMC_MSLEEP(emmc_delay);
    // send CSD The Device-Specific Data (CSD)
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEND_CSD, emmc->rca << 16, emmc->raw_csd);
    if (err)
    {
        EMMC_PRINTF("emmc_send_csd\n");
        err = ERR_CSD;
        goto MMC_INIT_OUT;
    }
    //memcpy(emmc->raw_csd, CSD_RES, sizeof(emmc->raw_csd));

    // decode csd
    err = emmc_decode_csd(handler);
    if (err)
    {
        EMMC_PRINTF("emmc_decode_csd error\n");
        goto MMC_INIT_OUT;
    }

    // decode cid
    err = emmc_decode_cid(handler);
    if (err)
    {
        EMMC_PRINTF("emmc_decode_cid error\n");
        err = ERR_CID;
        goto MMC_INIT_OUT;
    }
    EMMC_MSLEEP(emmc_delay);

    // select card // move to transfer statue
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEL_CARD, emmc->rca<<16, &rsp);

    // set block length to 512
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (512 << 16));
    // get ext_csd
    if (!(emmc->csd.mmca_vsn < 4))
    {
        EMMC_PRINTF("emmc_vsn 0x%02x\n", emmc->csd.mmca_vsn);
        emmc_single_read_data(handler, DIALOG_MMC_SND_EXT_CSD, 0, &rsp, (UINT32*)emmc->pext_csd);
    }
    else
    {
        EMMC_PRINTF("emmc_vsn 0x%02x not surport ext_csd\n", emmc->csd.mmca_vsn);
        err = ERR_EMMC_SETUP;
        goto MMC_INIT_OUT;
    }
    err = emmc_decode_ext_csd(handler);
    if (err)
        goto MMC_INIT_OUT;

    // send cmd6 (high speed)
    // mmc_switch의 response는 r1b으로 response 이후에 card의 busy가 있음, command 이후에 delay 삽입함
    // high speed mode가 bus width 설정 전에 결정되어야 한다.
#if 1
    if (emmc->ext_csd.hs_max_dtr != 0)
    {
        err = emmc_cmd_with_res(handler, DIALOG_MMC_SWITCH,
            emmc_make_switch_arg(E_CSD_CMD_SET_NORMAL,E_CSD_HS_TIMING,1)
            , &rsp);
        if (err)
        {
            EMMC_PRINTF("emmc switch to highspeed failed\n");
            err = 0;
        }
    }
    EMMC_MSLEEP(10);
    // bus width를 변경하기 전에 power class를 먼저 변경해 주어야 한다.
#endif

    // send cmd6 bus width 4bit
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SWITCH,
        emmc_make_switch_arg(E_CSD_CMD_SET_NORMAL,E_CSD_BUS_WIDTH,1), &rsp);
    if (err)
    {
        EMMC_PRINTF("emmc switch to 4bit bus failed\n");
        goto MMC_INIT_OUT;
    }

    EMMC_MSLEEP(10);
    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000005); // 4bit, high speed, push_pull up*******

    emmc->card_type = MMC_CARD;

    // bus test ??

    return ERR_NONE;

MMC_INIT_OUT:
    return ERR_EMMC_SETUP | err;
}

#if 0
static int sd_setup_card(HANDLE handler)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (EMMC_HANDLER_TYPE*)handler;
    UINT32 rsp, err;
#ifndef BUILD_OPT_VSIM
    // fetch scr from card.
    emmc_cmd_with_res(handler, DIALOG_APP_CMD, emmc->rca << 16, &rsp);
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (8 << 16) );
    err = emmc_single_read_data(handler, DIALOG_SD_APP_SEND_SCR, 0, &rsp, emmc->raw_scr);

#endif
    return err;
}
#endif

static int sd_init_card(const HANDLE handler, UINT32 ocr)
{
    EMMC_HANDLER_TYPE *emmc = NULL;
    UINT32 rocr, rsp;
    /* cid와 csd는 emmc 구조체의 데이터로 바로 저장 */
    //UINT32 CID_RES[4], CSD_RES[4];//, SWITCH[16];  // cmd6의 data를 쉽게 관리 하기 위해서 uint8로 잡는다.
    //UINT8 SWITCH[64];
#ifdef BUILD_OPT_DA16200_FPGA
    UINT8* SWITCH;
#else
    UINT8 SWITCH[64];
#endif
    int err, i;
    DA16X_UNUSED_ARG(ocr);

    emmc = (EMMC_HANDLER_TYPE*)handler;

    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000001);
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (64 << 16) );

    // cmd0 go to idle
    emmc_cmd_without_res(handler, DIALOG_MMC_GO_IDLE_ST, 0x12345678);
    EMMC_MSLEEP(emmc_delay);

    // cmd0 go to idle
    emmc_cmd_without_res(handler, DIALOG_MMC_GO_IDLE_ST, 0xffffffff);
    EMMC_MSLEEP(emmc_delay);

    // cmd8
    emmc_cmd_with_res(handler, DIALOG_SD_SND_IF_COND, 0x15a, &rsp);
    EMMC_MSLEEP(emmc_delay);

    ////////////////////////////////////////////////////////////
    // cmd 55
    emmc_cmd_with_res(handler, DIALOG_APP_CMD, 0xffff, &rsp);
    EMMC_MSLEEP(10);

    // send DIALOG_SD_APP_OP_COND;
    err = emmc_cmd_with_res(emmc,DIALOG_SD_APP_OP_COND,0,&rocr);
    if(err)
    {
        EMMC_PRINTF("sd_app_op_cond error\n");
        goto SD_INIT_OUT;
    }
    ///////////////////////////////////////////////////////////
    for (i = 0; i < 100; i++)
    {
        // cmd 55
        err = emmc_cmd_with_res(handler, DIALOG_APP_CMD, 0xffff, &rsp);
        EMMC_MSLEEP(10);
        err |= emmc_cmd_with_res(emmc,DIALOG_SD_APP_OP_COND,0x40ff0000,&rocr);

        if (err)
            goto SD_INIT_OUT;
        if (rocr & CARD_READY)
            break;

        EMMC_MSLEEP(emmc_delay);
    }
    emmc->ocr = rocr;

#if 0
    // cmd8 test interface
    {
        static UCHAR test_pattern = 0xaa;
        UCHAR result_pattern;

        // send command 리눅스를 참조한 arg인데 << 8이 맞는지 확인 필요
        //err = emmc_cmd_with_res(handler, SD_SND_IF_COND, ((emmc->ocr & 0xFF8000) != 0) << 8 | test_pattern, &rsp);
        err = emmc_cmd_with_res(handler, SD_SND_IF_COND, ((emmc->ocr) << 8 | test_pattern), &rsp);
        if (err)
        {
            EMMC_PRINTF("sd_init_send_if_cond\n");
            goto SD_INIT_OUT;
        }
        // check result
        result_pattern = rsp & 0xff;

        if (result_pattern != test_pattern)
        {
            err = ERR_SD_IF_COND;
            goto SD_INIT_OUT;
        }
    }
#endif

    // send cid
    err = emmc_cmd_with_res(handler, DIALOG_MMC_ALL_SEND_CID, 0xffffffff, emmc->raw_cid);
    if (err)
    {
        EMMC_PRINTF("sd send cid err\n");
        goto SD_INIT_OUT;
    }
    sd_decode_cid(handler);

    EMMC_MSLEEP(emmc_delay);
    // send relative_addr
    err = emmc_cmd_with_res(handler, DIALOG_SD_SND_REL_ADDR, 0, &rsp);
    if (err)
    {
        EMMC_PRINTF("sd send relative addr\n");
        goto SD_INIT_OUT;
    }
    emmc->rca = rsp>>16;
    EMMC_PRINTF("sd rca %d\n", emmc->rca);
    EMMC_MSLEEP(emmc_delay);

    // send CSD The Device-Specific Data (CSD)
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEND_CSD, emmc->rca << 16, emmc->raw_csd);
    if(err)
    {
        EMMC_PRINTF("sd send csd err\n");
        goto SD_INIT_OUT;
    }
    // decode csd
    err = sd_decode_csd(handler) ;
    if (err)
    {
        EMMC_PRINTF("sd_decode_csd error\n");
        goto SD_INIT_OUT;
    }

    // select card // move to transfer status
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEL_CARD, emmc->rca<<16, &rsp);
    EMMC_MSLEEP(emmc_delay);

    // check sd card's status with cmd13
    err = emmc_cmd_with_res(handler, DIALOG_SD_APP_SD_STS, emmc->rca<<16, &rsp);
    EMMC_MSLEEP(emmc_delay);

    // fetch scr from card.
    emmc_cmd_with_res(handler, DIALOG_APP_CMD, emmc->rca << 16, &rsp);
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (8 << 16) );
    err = emmc_single_read_data(handler, DIALOG_SD_APP_SEND_SCR, 0, &rsp, emmc->raw_scr);
    if (err)
    {
        EMMC_PRINTF("sd get scr error\n");
        goto SD_INIT_OUT;
    }
    err = mmc_decode_scr(handler);
    if (err)
    {
        EMMC_PRINTF("sd scr decode error\n");
        goto SD_INIT_OUT;
    }

    // Checks whether CMD23 support or not
#ifdef FC9000_MPW2
    // spec 3.0 일 때만 수행
    // send CMD23 and check response
    if (emmc->scr.sda_spec3)
    {
        err = emmc_cmd_with_res(handler, DIALOG_MMC_SET_BLK_CNT, 0x1, &rsp);
        if (rsp & RES_ILLEGAL_CMD)	// not support
        {
        }
        else						// support
        {
            if (rsp & RES_APP_CMD)	// 1.1 카드가 app cmd로 인식함 따라서 풀어주는 command를 하나 더 날림
            {
                err = emmc_cmd_with_res(handler, 5, 0, &rsp);
            }
            else
            {
                emmc->scr.dialog_sd_cmds |= SD_SCR_CMD23_SUPPORT;
            }
        }
    }
#endif

    // fetch SSR from card.
    emmc_cmd_with_res(handler, DIALOG_APP_CMD, emmc->rca << 16, &rsp);
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (64 << 16) );
    err = emmc_single_read_data(handler, DIALOG_SD_APP_SD_STS, 0, &rsp, emmc->raw_ssr);
    if (err)
    {
        EMMC_PRINTF("sd get ssr error\n");
        goto SD_INIT_OUT;
    }
    err = mmd_decode_ssr(handler);
    if (err)
    {
        EMMC_PRINTF("sd ssr decode error\n");
        goto SD_INIT_OUT;
    }

#ifndef  BUILD_OPT_VSIM
    // set to high speed cmd6 is valid only in transfer status
    // get card's support functions
#ifdef BUILD_OPT_DA16200_FPGA
    SWITCH = HAL_MALLOC(64);
    if (SWITCH == NULL)
        goto SD_INIT_OUT;
#endif
    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (64 << 16) );
    err = emmc_single_read_data(handler, DIALOG_SD_SWITCH, sd_make_switch_arg(0,0,1), &rsp, (UINT32*)SWITCH);
    if (err)
    {
        EMMC_PRINTF("sd_switch err 0,0,1\n");
        err = ERR_SD_SETUP;
#ifdef BUILD_OPT_DA16200_FPGA
        HAL_FREE(SWITCH);
#endif
        goto SD_INIT_OUT;
    }
    /* check data group1 high speed  */
    if (SWITCH[13] & 0x02)
    {
        emmc->caps.dialog_hs_max_dtr = 50000000;
        EMMC_PRINTF("sd_card MAX DTR = 50000000\n");
        // set hs sd card
        // check version of card
        // check cmd class
        // hs에 실퍠하는 경우가 있어서 한번 더 write 함
        err = emmc_single_read_data(handler, DIALOG_SD_SWITCH, sd_make_switch_arg(1,0,1), &rsp, (UINT32*)SWITCH);

        err = emmc_single_read_data(handler, DIALOG_SD_SWITCH, sd_make_switch_arg(1,0,1), &rsp, (UINT32*)SWITCH);
        if (err)
        {
            EMMC_PRINTF("sd_card switch cmd fail\n");
            err = ERR_SD_SETUP;
#ifdef BUILD_OPT_DA16200_FPGA
            HAL_FREE(SWITCH);
#endif
            goto SD_INIT_OUT;
        }
        //check SWITCH[16] & 0x0f != 1 cannot change to hs
        if ((SWITCH[16] & 0x0f) != 1)
        {
            EMMC_PRINTF("sd_card hs change fail\n");
            err = ERR_SD_SETUP;
#ifdef BUILD_OPT_DA16200_FPGA
            HAL_FREE(SWITCH);
#endif
            goto SD_INIT_OUT;
        }
    }
#ifdef BUILD_OPT_DA16200_FPGA
    HAL_FREE(SWITCH);
#endif
#if SD_SPEC_3_0
    // set uhs sd card -> fc9050에 적용된 sd/emmc ip는 sd spec 2.X 여서 uhs는 지원하지 않는다.
    if (emmc->scr.sda_spec3)
    {
        emmc->caps.sd3_bus_mode = SWITCH[13];
        /* Find out driver strengths supportsed by the card */
        err = emmc_single_read_data(handler, DIALOG_SD_SWITCH, sd_make_switch_arg(0,2,1), &rsp, (UINT32*)SWITCH);
        if (err)
        {
            EMMC_PRINTF("sd_switch err 0,2,1\n");
        }
        emmc->caps.sd3_drv_type = SWITCH[9];

        err = emmc_single_read_data(handler, DIALOG_SD_SWITCH, sd_make_switch_arg(0,3,1), &rsp, (UINT32*)SWITCH);
        if (err)
        {
            EMMC_PRINTF("sd_switch err 0,3,1\n");
        }
        emmc->caps.sd3_curr_limit = SWITCH[7];
    }
#endif


#endif
    EMMC_MSLEEP(emmc_delay);

    // sd_switch  set bus width 4bit  set retry number
    // check scr bus_wid
    err = emmc_cmd_with_res(handler, DIALOG_APP_CMD, emmc->rca<<16 | 0xffff, &rsp);
    EMMC_MSLEEP(10);
    err = emmc_cmd_with_res(handler, DIALOG_SD_SWITCH, SD_BUS_WIDTH_4, &rsp);

    EMMC_MSLEEP(emmc_delay);
    // register change to 4bit // sd spec 1.0에선 high speed 옵션을 끈다. -> 1.1에서도 high speed 지원됨
    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000005); // 4bit, high speed, push_pull up

    // for sd card spec 1.1
    if (emmc->csd.capa_size < 2048000 )
    {
        // set block length to 512
        err = emmc_cmd_with_res(handler, DIALOG_SD_SET_BLK_LEN, 512, &rsp);
        emmc->card_type = SD_CARD_1_1;
    }
    else
        emmc->card_type = SD_CARD;

    HW_REG_WRITE32(emmc->regmap->HIF_BLK_LG, (512 << 16) );
    err = emmc_cmd_with_res(handler, DIALOG_SD_APP_SD_STS, emmc->rca<<16 | 0xffff, &rsp);
    //emmc->card_type = SD_CARD;
    return ERR_NONE;

SD_INIT_OUT:
    return err | ERR_SD_SETUP;
}

static int sdio_init_card(const HANDLE handler, UINT32 ocr)
{
	DA16X_UNUSED_ARG(ocr);

    EMMC_HANDLER_TYPE *emmc = NULL;
    UINT32 rocr, rsp, cis_ptr = 0;
    UINT8 data, cccr_vsn;
    int err;
    UINT32 i;

    emmc = (EMMC_HANDLER_TYPE*)handler;

    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000001);

    // cmd0 go to idle	// sdio에서 cmd0는 spi모드로 진입을 의미한다.
    //emmc_cmd_without_res(handler, MMC_GO_IDLE_STATE, 0x0);

    for (i = 0; i < MMC_RETRY_COUNT; i++)
    {
        ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_SDIO_SEND_OP_COND));
        err = emmc_cmd_with_res(handler,DIALOG_SDIO_SEND_OP_COND,rocr,&rocr);

        if (err)
        {
            EMMC_PRINTF("sdio send op cond fail\n");
            goto SDIO_INIT_OUT;
        }
        if (rocr == 0)
        {
            err = ERR_UNKNOWN_OCR;
            //goto SDIO_INIT_OUT;
        }
        EMMC_PRINTF("%x\n", rocr);
        if ((rocr & 0x00300000) == 0)
        {
            err = ERR_NON_SUPPORT_VOLTAGE;
            goto SDIO_INIT_OUT;
        }
        if (rocr & CARD_READY)
            break;

        EMMC_MSLEEP(emmc_delay);
    }
    if (i == MMC_RETRY_COUNT)
        goto SDIO_INIT_OUT;

    emmc->ocr = rocr & FC9000_SUPPORT_VOLTAGE;

    // select voltage
    for (i = 0; i < MMC_RETRY_COUNT; i++)
    {
        ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_SDIO_SEND_OP_COND));
        err = emmc_cmd_with_res(handler,DIALOG_SDIO_SEND_OP_COND,emmc->ocr,&rocr);

        if (err)
        {
            EMMC_PRINTF("sdio send op cond fail\n");
            goto SDIO_INIT_OUT;
        }
        if (rocr == 0)
        {
            err = ERR_UNKNOWN_OCR;
            goto SDIO_INIT_OUT;
        }

        if (rocr & CARD_READY)
            break;

        EMMC_MSLEEP(emmc_delay);
    }
    if (i == MMC_RETRY_COUNT)
        goto SDIO_INIT_OUT;

    if (emmc->ocr & DIALOG_R4_MEM_PR)
    {
        EMMC_PRINTF("sdio combo card not support\n");
        err = ERR_SDIO_COMBO_NOT_SUPPORT;
        goto SDIO_INIT_OUT;
    }

    EMMC_MSLEEP(emmc_delay);

    // send relative_addr // 사용할 voltage를 ocr 값으로 전달 뒤에 cmd3에 응답함
    ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_SD_SND_REL_ADDR));
    err = emmc_cmd_with_res(handler, DIALOG_SD_SND_REL_ADDR, 0, &rsp);
    if (err)
    {
        EMMC_PRINTF("sdio send relative addr\n");
        goto SDIO_INIT_OUT;
    }
    emmc->rca = rsp>>16;
    EMMC_PRINTF("sdio rca %d\n", emmc->rca);
    EMMC_MSLEEP(emmc_delay);

    // select card
    ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_MMC_SEL_CARD));
    err = emmc_cmd_with_res(handler, DIALOG_MMC_SEL_CARD, emmc->rca<<16, &rsp);
    if(err)
        goto SDIO_INIT_OUT;

    // read cccr
    ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_SDIO_CCCR_CCCR));
    err = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_CCCR, 0, &data);
    if(err)
        goto SDIO_INIT_OUT;

    cccr_vsn = data & 0x0f;
    if ( cccr_vsn > DIALOG_SDIO_CCCR_REV_1_20)
    {
        EMMC_PRINTF("sdio unrecognised CCCR version\n");
        goto SDIO_INIT_OUT;
    }

    emmc->cccr.sdio_vsn = (data & 0xf0) >> 4;

    ASIC_DBG_TRIGGER(MODE_HAL_STEP(DIALOG_SDIO_CCCR_CAPS));
    err = sdio_io_rw_direct(handler, 0 ,0, DIALOG_SDIO_CCCR_CAPS, 0, &data);
    if (err)
        goto SDIO_INIT_OUT;

    if (data & DIALOG_SDIO_CCCR_CAP_SMB)
        emmc->cccr.multi_block = 1;
    if (data & DIALOG_SDIO_CCCR_CAP_LSC)
        emmc->cccr.low_speed = 1;
    if (data & DIALOG_SDIO_CCCR_CAP_4BLS)
        emmc->cccr.wide_bus = 1;

    if (cccr_vsn >= DIALOG_SDIO_CCCR_R_1_10) {
        err = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_POWER, 0, &data);
        if (err)
            goto SDIO_INIT_OUT;

        if (data & DIALOG_SDIO_PWR_SMPC)
            emmc->cccr.high_power = 1;
    }

    if (cccr_vsn >= DIALOG_SDIO_CCCR_R_1_20) {
        err = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_SPEED, 0, &data);
        if (err)
            goto SDIO_INIT_OUT;

        if (data & DIALOG_SDIO_SPEED_SHS)
            emmc->cccr.high_speed = 1;
    }
    //end read cccr


    for (i = 0; i < 3; i++)
    {
        err = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_FBR_BASE(0) + DIALOG_SDIO_FBR_CIS + i,0, &data);
        if (err)
            goto SDIO_INIT_OUT;

        cis_ptr |= data << (i * 8);
    }

    do {
        UINT8 tpl_code, tpl_link;
        err = sdio_io_rw_direct(handler, 0, 0, cis_ptr++, 0, &tpl_code);
        if (err)
            break;

        /* 0xff means we're done */
        if (tpl_code == 0xff)
            break;

        /* null entries have no link field or data */
        if (tpl_code == 0x00)
            continue;

        err = sdio_io_rw_direct(handler, 0, 0, cis_ptr++, 0, &tpl_link);
        if (err)
            break;

        /* a size of 0xff also means we're done */
        if (tpl_link == 0xff)
            break;

        emmc->psdio_cis = (UINT8*)HAL_MALLOC(tpl_link);
        if (!(emmc->psdio_cis))
        {
            err = ERR_SDIO_MEM;
            goto SDIO_INIT_OUT;
        }

        EMMC_PRINTF("tpl_link %d\n", tpl_link);

        for (i = 0; i < tpl_link; i++)
        {
            err = sdio_io_rw_direct(handler, 0, 0, cis_ptr + i, 0, &(emmc->psdio_cis[i]));
            EMMC_PRINTF("0x%02x\n", emmc->psdio_cis[i]);
            if (err)
            {
                HAL_FREE(emmc->psdio_cis);
                goto SDIO_INIT_OUT;
            }
        }
        EMMC_PRINTF("end\n");

        /* Try to parse the CIS tuple */
        err = sdio_cis_tpl_parse(handler, emmc->psdio_cis, tpl_code, tpl_link);
        HAL_FREE(emmc->psdio_cis);
        if (err) {
            break;
        }
        cis_ptr += tpl_link;

    }while (!err);

    //
    HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00000005); // 4bit, push_pull up

    return ERR_NONE;
SDIO_INIT_OUT:
    return ERR_SDIO_SETUP | err;
}

/*
*	extern functions
*/
EMMC_DRV_TYPE EMMC_SET_DRV_TYPE(EMMC_DRV_TYPE type)
{
	if (type ==EMMC_POLLING)
		emmc_drv_type = EMMC_POLLING;
	else
		emmc_drv_type = EMMC_INTERRUPT_DRIVEN;

	return emmc_drv_type;
}
int EMMC_SET_DELAY(int delay)
{
	if (delay > 0)
		emmc_delay = delay;
	return emmc_delay;
}
int EMMC_SET_DEBUG(int debug_on)
{
	if (debug_on != 0)
		emmc_debug = 1;
	else
		emmc_debug = 0;
	return emmc_debug;
}
EMMC_CLK_TYPE EMMC_SET_CLK_ALWAYS_ON(EMMC_CLK_TYPE type)
{
	if (type == EMMC_CLK_RW_ON)
		emmc_clk_type = EMMC_CLK_RW_ON;
	else
		emmc_clk_type = EMMC_CLK_ALWAYS_ON;
	return emmc_clk_type;
}

HANDLE EMMC_CREATE( void )
{
    // check card exist
    // malloc memory
    EMMC_HANDLER_TYPE *emmc = NULL;
    emmc = (HANDLE)pvPortMalloc(sizeof(EMMC_HANDLER_TYPE));
    if (emmc == NULL)
        return NULL;

	/* clock gating on */
	DA16X_CLKGATE_ON(clkoff_hif);

    memset(emmc, 0x00, sizeof(EMMC_HANDLER_TYPE));
    emmc->dev_addr = (UINT32)SYS_EMMC_BASE ;
    emmc->regmap = (EMMC_REG_MAP *)(emmc->dev_addr);
    emmc->card_type = UNKNOWN_TYPE;
    emmc->pext_csd = pvPortMalloc(BLOCK_SIZE);

#ifdef OAL_WRPPER
    if( OAL_CREATE_QUEUE(&(emmc->emmc_queue), "emmc_que", &(emmc->emmc_queue_buf), 4, OAL_FIFO, 1/*4byte*/, OAL_SUSPEND) != OAL_SUCCESS )
    {
        HAL_FREE(emmc);
        return NULL;
    }
    if (OAL_CREATE_SEMAPHORE(&(emmc->emmc_sema), "emmc_sem", 1, OAL_SUSPEND) != OAL_SUCCESS)
    {
        HAL_FREE(emmc);
        return NULL;
    }
#else
    emmc->emmc_queue = xQueueCreate(1, 4/*sizeof(UINT32)*/);
    if(emmc->emmc_queue == 0)
    {
        vPortFree(emmc);
        return NULL;
    }
    emmc->emmc_sema = xSemaphoreCreateMutex();

    if (emmc->emmc_sema == 0)
    {
        vQueueDelete(emmc->emmc_queue);
        vPortFree(emmc);
        return NULL;
    }
#endif
    irq_emmc = emmc;
    //HW_REG_WRITE32(irq_emmc->regmap->HIF_INT_CTL, 0x00008fff);
    _sys_nvic_write(EMMC_IRQn, (void *)_tx_specific_emmc_interrupt, 0x7);

    // return handle
    if (emmc != NULL)
        return emmc;
    else
        return NULL;
}

int EMMC_INIT (HANDLE handler)
{
    /* UINT32 ocr;*/
    int err;
    EMMC_HANDLER_TYPE *emmc;
    emmc = (EMMC_HANDLER_TYPE *)handler;
    // mmc_send_op_cond
#if 0
    err = emmc_send_op_cond(emmc, 0, &ocr);
    if (err)
    {
        EMMC_PRINTF("emmc init send op cond fail\n");
    }
#endif
    // mmc_select_voltage  spec 6.4.2 에 for emmc devices, the boltage range in CMD1 is no longer valid 라고 기술 되어 있음
    //emmc->ocr = emmc_select_voltage(emmc, ocr);

    // sdio 카드인지 먼저 검사하고, sd 카드 인지 검사 한다.
    err = sdio_init_card(emmc, emmc->ocr);
    if (err)
    {
        EMMC_PRINTF("sdio init fail\n");
        HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00008000);
#ifdef OAL_WRAPPER
        OAL_RESET_QUEUE(&(emmc->emmc_queue));
#else
        xQueueReset((emmc->emmc_queue));
#endif
    }
    else
    {
        emmc->card_type = SDIO_CARD;
        emmc_clock_control(emmc, emmc_clock_off);
        return ERR_NONE;
    }

    EMMC_MSLEEP(10);
    // sd 카드인지를 먼저 검사하고, mmc 카드인지 검사한다
    err = sd_init_card(emmc, emmc->ocr);
    if (err)
    {
        EMMC_PRINTF("sd init fail\n");
        HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00008000);
#ifdef OAL_WRAPPER
        OAL_RESET_QUEUE(&(emmc->emmc_queue));
#else
        xQueueReset((emmc->emmc_queue));
#endif
    }
    else
    {
        /* the inserted card is sd card sdcard의 버전이 1.1과 2.0이 있을 수 있기 때문에 sd_init_card안에서 결정한다. */
        //emmc->card_type = SD_CARD;
        emmc_clock_control(emmc, emmc_clock_off);
        return ERR_NONE;
    }
    EMMC_MSLEEP(10);
    // mmc_init_card
    err = emmc_init_card(emmc, emmc->ocr);

    if (err)
    {
        EMMC_PRINTF("mmc init fail\n");
        HW_REG_OR_WRITE32(emmc->regmap->HIF_CTL_00, 0x00008000);
        return ERR_MMC_INIT;
    }
    else
    {
        /* the inserted card is sd card */
        emmc->card_type = MMC_CARD;
        emmc_clock_control(emmc, emmc_clock_off);
        return ERR_NONE;
    }
}
void EMMC_SEND_CMD(HANDLE handler, UINT32 cmd, UINT32 cmd_arg)
{
    if (!handler)
        return;
    emmc_clock_control(handler, emmc_clock_on);
    emmc_cmd_without_res(handler, cmd, cmd_arg);
    emmc_clock_control(handler, emmc_clock_off);
}
void EMMC_SEND_CMD_RES(HANDLE handler, UINT32 cmd, UINT32 cmd_arg, UINT32 *rsp)
{
    if (!handler)
        return;
    emmc_clock_control(handler, emmc_clock_on);
    emmc_cmd_with_res(handler, cmd, cmd_arg, rsp);
    emmc_clock_control(handler, emmc_clock_off);
}

int EMMC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
    EMMC_HANDLER_TYPE *emmc;

    if (handler == NULL)
        return ERR_HANDLE;
    emmc_clock_control(handler, emmc_clock_on);
    emmc = (EMMC_HANDLER_TYPE *)handler;

    switch(cmd){
    case	EMMC_SET_BLK_SIZE:						/* set block size */
        emmc->cur_blk_size = *((UINT32 *)data);
        break;
    case 	EMMC_GET_BLK_SIZE:
        *((UINT32 *)data) = emmc->cur_blk_size;
        break;
    default:
        return ERR_UNKNOWN_CONTROL;
    }

    emmc_clock_control(handler, emmc_clock_off);
    return ERR_NONE;
}

/* block length 512만 고려한다. */
int EMMC_READ(HANDLE handler, UINT32 dev_addr, VOID *p_data, UINT32 block_count)
{
    UINT32 rsp = 0;
    EMMC_HANDLER_TYPE *emmc;
    int err = ERR_NONE;

    if (handler == NULL)
        return ERR_HANDLE;

    emmc = (EMMC_HANDLER_TYPE *)handler;

    if (emmc->card_type == UNKNOWN_TYPE)
        return ERR_CARD_NOT_EXIST;

    /* check the capasity */
    if (!check_address(handler, dev_addr, block_count))
        return ERR_INVAL;

    if (emmc->card_type == SD_CARD_1_1)
        dev_addr = dev_addr << 9;

    if (block_count > 1)
    {
        // 카드가 emmc일 경우 closed 가능 sd일 경우 open으로만 동작해야 함
        // 15.10.12 sd scr의 support command에 CMD23이 있을 경우 closed 모드로 동작
        if (emmc->card_type != MMC_CARD)
        {
            if (emmc->scr.dialog_sd_cmds & SD_SCR_CMD23_SUPPORT)	// closed
            {
                /* read multi block */
                emmc_clock_control(handler, emmc_clock_on);
                err = emmc_block_read_data(handler, DIALOG_MMC_RD_BLK, dev_addr, &rsp, p_data, block_count);
                emmc_clock_control(handler, emmc_clock_off);
                if (!err && rsp)
                {
                    // 예외처리 추가
                    EMMC_PRINTF("read multi block done rsp %x\n",rsp);
                }
                else if(err)
                {
                    EMMC_PRINTF("read multi block fail\n");
                }
            }
            else
            {
                /* read open mode */
                emmc_clock_control(handler, emmc_clock_on);
                err = emmc_multi_read_data(handler, DIALOG_MMC_RD_BLK, dev_addr, &rsp, p_data, block_count);
                emmc_clock_control(handler, emmc_clock_off);
                if (!err && rsp)
                {
                    // 예외처리 추가
                    EMMC_PRINTF("read multi block done rsp %x\n",rsp);
                }
                else if(err)
                {
                    EMMC_PRINTF("read multi block fail\n");
                }
            }
        }
        else
        {
            /* read multi block */
            emmc_clock_control(handler, emmc_clock_on);
            err = emmc_block_read_data(handler, DIALOG_MMC_RD_BLK, dev_addr, &rsp, p_data, block_count);
            emmc_clock_control(handler, emmc_clock_off);
            if (!err && rsp)
            {
                // 예외처리 추가
                EMMC_PRINTF("read multi block done rsp %x\n",rsp);
            }
            else if(err)
            {
                EMMC_PRINTF("read multi block fail\n");
            }
        }

    }
    else
    {
        /* read single block */
        emmc_clock_control(handler, emmc_clock_on);
        err = emmc_single_read_data(handler, DIALOG_MMC_RD_SINGLE_BLK, dev_addr, &rsp, p_data);
        emmc_clock_control(handler, emmc_clock_off);
        if (rsp)
        {
            // 예외처리 추가
            EMMC_PRINTF("read single block done rsp %x\n",rsp);
        }
        else if(err)
        {
            EMMC_PRINTF("read single block fail\n");
        }
    }
    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
        if (err)
        {
            HW_REG_WRITE32(irq_emmc->regmap->HIF_EVNT_CTL, (0x3fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_EVNT_CTL) & 0x0000ffff));
            HW_REG_WRITE32(irq_emmc->regmap->HIF_INT_CTL, (0x0fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL) & 0x0000ffff));
        }
    }
    return err;
}

int EMMC_WRITE(HANDLE handler, UINT32 dev_addr, VOID *p_data, UINT32 block_count)
{
    UINT32 rsp;
    int err = ERR_NONE;
    EMMC_HANDLER_TYPE *emmc;
    emmc = (EMMC_HANDLER_TYPE *)handler;

    /* check the write protect if read only condition return fail */

    if (HW_REG_READ32(emmc->regmap->HIF_EVNT_CTL) & 0x40000000)
        return ERR_SINGLE_WRITE | ERR_MULTI_WRITE;

    /* check the capasity */
    if (!check_address(handler, dev_addr, block_count))
        return ERR_INVAL;

    /* sd 1.1 card's addr is byte based so multi 512 */
    if (emmc->card_type == SD_CARD_1_1)
        dev_addr = dev_addr << 9;

    if (block_count > 1)
    {
        if (emmc->card_type != MMC_CARD)		// sd card
        {
            if (emmc->scr.dialog_sd_cmds & SD_SCR_CMD23_SUPPORT)	// closed
            {
                emmc_clock_control(handler, emmc_clock_on);
                err = emmc_block_write_data(handler, DIALOG_MMC_WR_MBLOCK, dev_addr, &rsp, p_data, block_count);
                emmc_clock_control(handler, emmc_clock_off);
            }
            else										// open mode
            {
                /* write multi block */
                emmc_clock_control(handler, emmc_clock_on);
                err = emmc_multi_write_data(handler, DIALOG_MMC_WR_MBLOCK, dev_addr, &rsp, p_data, block_count);
                emmc_clock_control(handler, emmc_clock_off);
                if (err)
                {
                    EMMC_PRINTF("multi write fail\n");
                }
                else
                {
                    EMMC_PRINTF("multi write done\n");
                }
            }
        }
        else							// closed mode
        {
            emmc_clock_control(handler, emmc_clock_on);
            err = emmc_block_write_data(handler, DIALOG_MMC_WR_MBLOCK, dev_addr, &rsp, p_data, block_count);
            emmc_clock_control(handler, emmc_clock_off);
        }
    }
    else
    {
        /* write single block */
        emmc_clock_control(handler, emmc_clock_on);
        err = emmc_single_write_data(handler, DIALOG_MMC_WR_BLK, dev_addr , &rsp, p_data);
        emmc_clock_control(handler, emmc_clock_off);
        if (err)
        {
            EMMC_PRINTF("single write fail\n");
        }
        else
        {
            EMMC_PRINTF("single write done\n");
        }
    }
    if (emmc_drv_type == EMMC_INTERRUPT_DRIVEN)
    {
        if (err)
        {
            HW_REG_WRITE32(irq_emmc->regmap->HIF_EVNT_CTL, (0x3fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_EVNT_CTL) & 0x0000ffff));
            HW_REG_WRITE32(irq_emmc->regmap->HIF_INT_CTL, (0x0fff0000) | (HW_REG_READ32(irq_emmc->regmap->HIF_INT_CTL) & 0x0000ffff));
        }
    }
    return err;
}

int EMMC_ERASE(HANDLE handler, UINT32 start_addr, UINT32 block_count)
{
	DA16X_UNUSED_ARG(start_addr);
	DA16X_UNUSED_ARG(block_count);

    if (!handler)
        return -1;
    return 0;
}

int EMMC_CLOSE(HANDLE handler)
{
    if (handler != NULL)
    {
        // free memory
        EMMC_HANDLER_TYPE *emmc;
        emmc = (EMMC_HANDLER_TYPE *)handler;
        if (emmc->pext_csd != NULL)
            vPortFree((void*)emmc->pext_csd);
        if (emmc->psdio_info != NULL)
            vPortFree((void*)emmc->psdio_info);
#ifdef OAL_WRAPPER
        OAL_DELETE_QUEUE(&(emmc->emmc_queue));
        OAL_DELETE_SEMAPHORE(&(emmc->emmc_sema));

        HAL_FREE((void*)handler);
#else
        vQueueDelete((emmc->emmc_queue));
        vSemaphoreDelete((emmc->emmc_sema));
        vPortFree((void*)handler);
#endif
        _sys_nvic_write(EMMC_IRQn, NULL, 0x7);
		/* clock gating off */
		DA16X_CLKGATE_OFF(clkoff_hif);
        return ERR_NONE;
    }
    else
    {
        return ERR_HANDLE;
    }
}

int EMMC_DETECT(void)
{
    // asic에서 사용할 gpio를 지정하고, 해당 핀의 값이 low일 경우 card inserted 상태
    // 0x50030004 [31] bit return
    return ((*((volatile UINT *)(EMMC_BASE_0+0x04)) & 0x80000000) == 0x80000000)?0:1;
}

int SDIO_ENABLE_FUNC(HANDLE handler, UINT32 func_num)
{
    int ret, i;
    UINT8 reg;

    ret = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_IOEx, 0, &reg);
    if (ret)
        goto err;

    reg |= 1 << func_num;

    ret = sdio_io_rw_direct(handler, 1, 0, DIALOG_SDIO_CCCR_IOEx, reg, NULL);
    if (ret)
        goto err;

    for (i = 0; i < 10; i ++)
    {
        ret = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_IORx, 0, &reg);
        if (ret)
            goto err;
        if (reg & (1 << func_num))
            break;
        ret = ERR_SDIO_TIME_OUT;
        // OAL_MSLEEP(100); // todo
    }
    if (i == 10)
    {
        ret = ERR_SDIO_TIME_OUT;
        goto err;
    }

    return ERR_NONE;

err:
    EMMC_PRINTF("SDIO: Failed to enable function\n");
    return ret;
}


int SDIO_DISABLE_FUNC(HANDLE handler, UINT32 func_num)
{
    int ret;
    unsigned char reg;

    ret = sdio_io_rw_direct(handler, 0, 0, DIALOG_SDIO_CCCR_IOEx, 0, &reg);
    if (ret)
        return ERR_SDIO_IO;

    reg &= ~(1 << func_num);

    ret = sdio_io_rw_direct(handler, 1, 0, DIALOG_SDIO_CCCR_IOEx, reg, NULL);
    if (ret)
        return ERR_SDIO_IO;

    return ERR_NONE;
}


int SDIO_SET_BLOCK_SIZE(HANDLE handler, UINT32 func_num, UINT32 blk_size)
{
    int ret;
    EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE*)handler;


    if ((emmc->max_blk_size > 0) && (blk_size > emmc->max_blk_size))
        return ERR_INVAL;

    if (blk_size == 0) {
        return ERR_INVAL;
    }

    ret = sdio_io_rw_direct(handler, 1, 0, DIALOG_SDIO_FBR_BASE(func_num) + DIALOG_SDIO_FBR_BLKSZ, blk_size & 0xff, NULL);
    if (ret)
        return ret;
    ret = sdio_io_rw_direct(handler, 1, 0, DIALOG_SDIO_FBR_BASE(func_num) + DIALOG_SDIO_FBR_BLKSZ + 1, (blk_size >> 8) & 0xff, NULL);
    if (ret)
        return ret;

    emmc->cur_blk_size = blk_size;

    return ERR_NONE;
}


int SDIO_READ_BYTE(HANDLE handler, UINT32 func_num, UINT32 addr, UINT8 *data)
{
    int ret;

    if (data == NULL)
        return ERR_INVAL;

    ret = sdio_io_rw_direct(handler , 0, (UINT8)func_num, addr, 0, data);
    if (ret) {
        return ret;
    }

    return ERR_NONE;
}

int SDIO_WRITE_BYTE(HANDLE handler, UINT32 func_num, UINT32 addr, UINT8 data)
{
    int ret;

    ret = sdio_io_rw_direct(handler , 1, (UINT8)func_num, addr, data, NULL);
    if (ret) {
        return ret;
    }
    return ERR_NONE;
}

int SDIO_READ_BURST(HANDLE handler, UINT32 func_num, UINT32 addr, UINT32 incr_addr, UINT8 *data, UINT32 count, UINT32 blksz)
{
    return sdio_io_rw_extended(handler, 0, (UINT8)func_num, addr, incr_addr, data,  count, blksz);
}

int SDIO_WRITE_BURST(HANDLE handler, UINT32 func_num, UINT32 addr, UINT32 incr_addr, UINT8 *data, UINT32 count, UINT32 blksz)
{
    return sdio_io_rw_extended(handler, 1, (UINT8)func_num, addr, incr_addr, data,  count, blksz);
}
#ifdef BUILD_OPT_VSIM
#undef BUILD_OPT_VSIM
#endif

