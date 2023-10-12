/**
****************************************************************************************
*
* @file pulsecounter.c
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
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "pulsecounter.h"
#include "common_def.h"

static int  _pcounter_instance[1] = {0};
static UINT8    drv_pcounter_flag = FALSE;
HANDLE  g_pcounter_handle;

HANDLE  DRV_PULSE_COUNTER_CREATE( UINT32 dev_id )
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;

    // Allocate
    if((pcounter = (PULSE_COUNTER_HANDLER_TYPE *) HAL_MALLOC(sizeof(PULSE_COUNTER_HANDLER_TYPE)) ) == NULL){
        return NULL;
    }

    // Clear
    DRV_MEMSET(pcounter, 0, sizeof(PULSE_COUNTER_HANDLER_TYPE));

    // Address Mapping
    switch(dev_id){
        case    0:
                pcounter->dev_addr =  RTC_BASE_0;;
                pcounter->instance = (UINT32)(_pcounter_instance[0]);
                _pcounter_instance[0] ++;
                break;

        default:
                DRIVER_DBG_ERROR("illegal unit, %d\r\n", dev_id);
                HAL_FREE(pcounter);
                return NULL;
    }

    // Set Default Para
    pcounter->regmap   = (RTC_REG_MAP *)(pcounter->dev_addr);
    pcounter->dev_unit = dev_id ;
    pcounter->clock = 0;

    g_pcounter_handle = (HANDLE)pcounter;
    return (HANDLE) pcounter;
}


int DRV_PULSE_COUNTER_INIT (HANDLE handler)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;

    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;

    // Normal Mode
    if(pcounter->instance == 0){
        // Post-Action
        if( drv_pcounter_flag == FALSE ){
            drv_pcounter_flag = TRUE ;
        }
    }

    return TRUE;
}

#define	DA16X_SLR_PULSE_COUNTER
unsigned int DRV_PULSE_COUNTER_READ(HANDLE handler)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    unsigned int val0, val1, val2 = 0, val_return = 0, i;

    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;

    for(i=0;i<5;i++)
    {
        val0 = pcounter->regmap->pulse_cnt_reg ;
        val1 = pcounter->regmap->pulse_cnt_reg ;
        PRINT_PWM("0x%x, 0x%x, 0x%x\r\n",val0,val1,val2);
        if(val1 <= val0+10)
        {
            val_return = val0;
            break;
        }
    }   

    return val_return;
}

int DRV_PULSE_COUNTER_INTERRUPT_CLEAR(HANDLE handler)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    pcounter->regmap->pulse_control |= 0x8000;
    pcounter->regmap->pulse_control &= ~(0x8000);

    return TRUE;
}


int DRV_PULSE_COUNTER_INTERRUPT_ENABLE(HANDLE handler, unsigned int en, unsigned int thd)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;

    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
//  *(volatile unsigned int *)(0x50091078) = thd;
    pcounter->regmap->pulse_int_thr = thd;

    if(en)
    {
//      *(volatile unsigned int *)(0x50091074) |= 0x4000;
        pcounter->regmap->pulse_control |= 0x4000;
    }
    else
    {
//      *(volatile unsigned int *)(0x50091074) &= ~(0x4000);
        pcounter->regmap->pulse_control &= ~(0x4000);
    }
    return TRUE;
}

void DRV_PULSE_COUNTER_INTERRUPT_HANDLER(void)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter = (PULSE_COUNTER_HANDLER_TYPE *)g_pcounter_handle;

    DRV_PULSE_COUNTER_INTERRUPT_ENABLE(g_pcounter_handle, 0, 0);
    DRV_PULSE_COUNTER_INTERRUPT_CLEAR(g_pcounter_handle);
    if(pcounter->callback[0] != NULL)
        pcounter->callback[0](NULL);
}

void    DRV_PULSE_COUNTER_INTERRUPT(void)
{
	INTR_CNTXT_CALL(DRV_PULSE_COUNTER_INTERRUPT_HANDLER);
}

int DRV_PULSE_COUNTER_ENABLE(HANDLE handler, unsigned int en)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    if(en)
    {
        pcounter->regmap->pulse_control |= 0x2;
        _sys_nvic_write(17, (void *)DRV_PULSE_COUNTER_INTERRUPT, 0x7);

    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x2);
    }   

    return TRUE;
}

int DRV_PULSE_COUNTER_RESET(HANDLE handler)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    pcounter->regmap->pulse_control |= 0x4;
    pcounter->regmap->pulse_control &= ~(0x4);
    return TRUE;

}

int DRV_PULSE_COUNTER_EDGE(HANDLE handler, unsigned int isfalling)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    if(isfalling)
    {
        pcounter->regmap->pulse_control |= 0x8;
    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x8);
    }
    return TRUE;
}

int DRV_PULSE_COUNTER_CLKSEL(HANDLE handler, unsigned int sel)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
#ifdef DA16X_SLR_PULSE_COUNTER
    if(sel)
    {
        pcounter->regmap->pulse_control |= (0x100000);
    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x100000);
    }

#else
    if(sel)
    {
        pcounter->regmap->pulse_control |= (0x10000);
    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x10000);
    }
#endif	
    return TRUE;
}

int DRV_PULSE_COUNTER_OSCEN(HANDLE handler, unsigned int en)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    if(en)
    {
        pcounter->regmap->pulse_control |= (0x1);
    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x1);
    }
    return TRUE;
}

int DRV_PULSE_COUNTER_FILTER(HANDLE handler, unsigned int en, unsigned int val)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    if(en)
    {
        pcounter->regmap->pulse_control |= 0x10;
    }
    else
    {
        pcounter->regmap->pulse_control &= ~(0x10);
    }

    pcounter->regmap->pulse_control &= ~(0x1f00);
    pcounter->regmap->pulse_control |= (val<<8)&0x1f00;

    return TRUE;
}

int DRV_PULSE_COUNTER_SET_GPIO(HANDLE handler, unsigned int num, unsigned int mode)
{
    PULSE_COUNTER_HANDLER_TYPE  *pcounter;
    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *)handler ;
    

    pcounter->regmap->pulse_control &= ~(0xf0000);
    pcounter->regmap->pulse_control |= num<<(16);

    if(mode==1)
        pcounter->regmap->gpio_wakeup_config |= 1<<(16+num);
    else
        pcounter->regmap->gpio_wakeup_config &= ~(1<<(16+num));
    return TRUE;
}

int  DRV_PULSE_COUNTER_CALLBACK_REGISTER(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param)
{
	DA16X_UNUSED_ARG(vector);

    PULSE_COUNTER_HANDLER_TYPE  *pcounter;

    if(handler == NULL){
        return FALSE;
    }

    pcounter = (PULSE_COUNTER_HANDLER_TYPE *) handler;

    pcounter->callback[0] = (USR_CALLBACK)callback ;
    pcounter->cb_param[0] = (VOID *)param ;

    return TRUE;
}

/* EOF */
