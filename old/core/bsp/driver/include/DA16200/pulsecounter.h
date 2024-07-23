/**
 ****************************************************************************************
 *
 * @file pulsecounter.h
 *
 * @brief Defines and macros for Pulse-Counter
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

/**
  @file 	pulsecounter.h
  @brief

  @author
  @date 	2019/07/20
  @note
  @bug
 **/
#ifndef __pulsecounter_h__
#define __pulsecounter_h__

//---------------------------------------------------------
//	DRIVER :: Structure 
//---------------------------------------------------------
struct  __pulse_counter_handler__ 
{
     UINT32              dev_addr;  // Unique Address
     // Device-dependant
     UINT32              dev_unit;
     UINT32              instance;
     UINT32              clock;

     // Register Map
     RTC_REG_MAP         *regmap;

     VOID                *mutex;
     USR_CALLBACK        callback[1];
     VOID                *cb_param[1];
};
typedef     struct __pulse_counter_handler__    PULSE_COUNTER_HANDLER_TYPE;     // deferred


//---------------------------------------------------------
//	DRIVER :: APP-Interface 
//---------------------------------------------------------

HANDLE  DRV_PULSE_COUNTER_CREATE( UINT32 dev_id );
int DRV_PULSE_COUNTER_INIT (HANDLE handler);
unsigned int DRV_PULSE_COUNTER_READ(HANDLE handler);
int DRV_PULSE_COUNTER_INTERRUPT_CLEAR(HANDLE handler);
int DRV_PULSE_COUNTER_INTERRUPT_ENABLE(HANDLE handler, unsigned int en, unsigned int thd);
void DRV_PULSE_COUNTER_INTERRUPT_HANDLER(void);
void    DRV_PULSE_COUNTER_INTERRUPT(void);
int DRV_PULSE_COUNTER_ENABLE(HANDLE handler, unsigned int en);
int DRV_PULSE_COUNTER_RESET(HANDLE handler);
int DRV_PULSE_COUNTER_EDGE(HANDLE handler, unsigned int isfalling);
int DRV_PULSE_COUNTER_CLKSEL(HANDLE handler, unsigned int sel);
int DRV_PULSE_COUNTER_OSCEN(HANDLE handler, unsigned int en);
int DRV_PULSE_COUNTER_FILTER(HANDLE handler, unsigned int en, unsigned int val);
int DRV_PULSE_COUNTER_SET_GPIO(HANDLE handler, unsigned int num, unsigned int mode);
int  DRV_PULSE_COUNTER_CALLBACK_REGISTER(HANDLE handler, UINT32 vector, UINT32 callback, UINT32 param);

#endif /* __pulsecounter_h__ */

