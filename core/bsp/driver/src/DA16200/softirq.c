/**
 ****************************************************************************************
 *
 * @file softirq.c
 *
 * @brief Software IRQ Driver
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

#if (dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define traceISR_ENTER()
#  define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

//#ifndef  uSING_MROM_FUNCTION

/******************************************************************************
 *
 *  Local Definitions & Declaration
 *
 ******************************************************************************/

#define	SOFTIRQ_KEY_MARK	0xFCDB0000
#define	SOFTIRQ_HASH		8

typedef	struct	 _softirq_item_ {
	UINT32			idxkey;
	UINT16			activate;
	UINT16			pushmark;
	USR_CALLBACK		callback;
	VOID			*param;
	struct	 _softirq_item_	*next;
} SOFTIRQ_ITEM_TYPE;

static SOFTIRQ_ITEM_TYPE *softirq_list[SOFTIRQ_HASH];
static UINT32 		  softirq_keycnt;


static  void	_lowlevel_softirq_interrupt(void);

/******************************************************************************
 *  softirq_create ( )
 *
 *  Purpose :   
 *  Input   :   baud
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

void softirq_create(void)
{
	UINT32 i;
	for( i=0 ; i<SOFTIRQ_HASH ; i++ ){
		softirq_list[i] = NULL;
	}
	softirq_keycnt = 0;
}

/******************************************************************************
 *  softirq_init ( )
 *
 *  Purpose :   
 *  Input   :   baud
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
void softirq_init(void)
{
	_sys_nvic_write(SOFT_IRQn, (void *)_lowlevel_softirq_interrupt, 0x7);
}

/******************************************************************************
 *  softirq_close ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

void softirq_close(void)
{
	UINT32	i;
	
	_sys_nvic_write(SOFT_IRQn, (void *)NULL, 0x7);

	for( i=0 ; i<SOFTIRQ_HASH ; i++ ){
		if(softirq_list[i] != NULL){

			while(softirq_list[i] != NULL ){
				SOFTIRQ_ITEM_TYPE *next;

				next = softirq_list[i]->next;

				vPortFree(softirq_list[i]);

				softirq_list[i] = next;
			}

			softirq_list[i] = NULL;
		}
	}

}

/******************************************************************************
 *  _softirq_interrupt ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
void _softirq_interrupt (void)
{
	UINT32	i;	

	for( i=0 ; i<SOFTIRQ_HASH ; i++ ){

		if( softirq_list[i] != NULL ){
			SOFTIRQ_ITEM_TYPE *softirq;

			softirq = softirq_list[i];
			
			
			while(softirq != NULL ){
				if( softirq->activate == TRUE
				   && softirq->pushmark == TRUE ){
					softirq->pushmark = FALSE;

					if( softirq->callback != NULL ){
						softirq->callback( softirq->param );
					}
				}
				softirq = softirq->next;
			}
		}		

	}
}

static  void	_lowlevel_softirq_interrupt(void)
{
	traceISR_ENTER();
	_softirq_interrupt();
	traceISR_EXIT();
}

/******************************************************************************
 *  softirq_registry ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

HANDLE	softirq_registry(USR_CALLBACK callback, VOID *param, UINT32 startmode)
{
	SOFTIRQ_ITEM_TYPE *softirq;
	UINT32	index;

	softirq = (SOFTIRQ_ITEM_TYPE *)pvPortMalloc(sizeof(SOFTIRQ_ITEM_TYPE));

	if( softirq != NULL ){
		index = softirq_keycnt;
		softirq->idxkey = SOFTIRQ_KEY_MARK | index;

		softirq->callback = callback;
		softirq->param 	  = (VOID *)param;
		softirq->activate = (UINT16)startmode;
		softirq->pushmark = FALSE;
		softirq_keycnt    = (softirq_keycnt+1)%0x0FFFF;

		index = index % SOFTIRQ_HASH ;
		softirq->next = NULL ;
		
		NVIC_DisableIRQ(SOFT_IRQn);
		
		if( softirq_list[index] != NULL ){
			SOFTIRQ_ITEM_TYPE *next;

			next = softirq_list[index];

			while( next->next != NULL ){
				next = next->next;
			}

			next->next = softirq->next;
		}else{
			softirq_list[index] = softirq ;
		}

		NVIC_EnableIRQ(SOFT_IRQn);

		return (HANDLE)(softirq->idxkey);
	}
	
	return NULL;
}

/******************************************************************************
 *  softirq_unregistry ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

UINT32	softirq_unregistry(HANDLE doorbell)
{
	SOFTIRQ_ITEM_TYPE *softirq, *prev;
	UINT32	index;

	index = ((UINT32)doorbell) % SOFTIRQ_HASH ;
	
	softirq = softirq_list[index];
	prev = NULL;
	
	while(softirq != NULL ){
		
		if( softirq->idxkey == (UINT32)doorbell ){

			NVIC_DisableIRQ(SOFT_IRQn);
			if( prev == NULL ){
				softirq_list[index] = softirq->next;
			}else
			{
				prev->next = softirq->next;
			}
			NVIC_EnableIRQ(SOFT_IRQn);
			
			vPortFree(softirq);
			
			return TRUE;
		}
		else
		{
			prev = softirq;
			softirq = softirq->next;
		}
	}	
	
	return FALSE;
}

/******************************************************************************
 *  softirq_push ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

UINT32	softirq_push(HANDLE doorbell)
{
	return softirq_push_param(doorbell, NULL);
}

UINT32 softirq_push_param(HANDLE doorbell, VOID *param)
{
	SOFTIRQ_ITEM_TYPE *softirq;
	UINT32	index;
	
	index = ((UINT32)doorbell) % SOFTIRQ_HASH ;
	softirq = softirq_list[index];

	while(softirq != NULL ){
		
		if( softirq->idxkey == (UINT32)doorbell ){

			NVIC_DisableIRQ(SOFT_IRQn);

			softirq->pushmark = TRUE;
			if(param != NULL){
				softirq->param = param ;
			}
			DA16200_BOOTCON->IRQ_Test_SW = (SOFT_IRQn);
			
			NVIC_EnableIRQ(SOFT_IRQn);
			
			return TRUE;
		}
		else
		{
			softirq = softirq->next;
		}
	}			

	return FALSE;
}

/******************************************************************************
 *  softirq_activate ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

UINT32	softirq_activate(HANDLE doorbell, UINT32 mode)
{
	SOFTIRQ_ITEM_TYPE *softirq;
	UINT32	index;
	
	index = ((UINT32)doorbell) % SOFTIRQ_HASH ;
	softirq = softirq_list[index];

	while(softirq != NULL ){
		if( softirq->idxkey == (UINT32)doorbell ){
			softirq->activate = (UINT16)mode;
			return TRUE;
		}
		else
		{
			softirq = softirq->next;
		}
	}

	return FALSE;
}

//#endif	/* uSING_MROM_FUNCTION */
