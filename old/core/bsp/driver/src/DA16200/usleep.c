/**
 ****************************************************************************************
 *
 * @file usleep.c
 *
 * @brief Usleep Driver
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
#include "sys_nvic.h"

//---------------------------------------------------------
// 
//---------------------------------------------------------

#define	USLEEP_DYNAMIC_ACTIVE

#define	USLEEP_SEQ_GEN(x)	x = (0x80000000 | ((x + 1)%0xFFFF))

#define USLEEP_EVT_BIT		(0x0000FFFF)

//---------------------------------------------------------
// 
//---------------------------------------------------------

static void usleep_driver_hisr(void *handler);

//---------------------------------------------------------
// 
//---------------------------------------------------------

static HANDLE	_usleep_handler[USLEEP_MAX];

/******************************************************************************
 *  USLEEP_CREATE ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

HANDLE	USLEEP_HANDLER(void)
{
	return _usleep_handler[0];
}
 
/******************************************************************************
 *  USLEEP_CREATE ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

HANDLE	USLEEP_CREATE( UINT32 dev_id )
{
	USLEEP_HANDLER_TYPE	*usleep;
	
	// Allocate
	if( _usleep_handler[dev_id] != NULL ){
		return _usleep_handler[dev_id];
	}
	
	if((usleep = (USLEEP_HANDLER_TYPE *) pvPortMalloc(sizeof(USLEEP_HANDLER_TYPE)) ) == NULL){
		return NULL;
	}else{
		DRV_MEMSET(usleep, 0, sizeof(USLEEP_HANDLER_TYPE));
	}

	// Address Mapping
	switch(dev_id){
		case	USLEEP_0:
			usleep->dev_unit = dev_id ;
			usleep->instance = 0;
			usleep->timer_id = STIMER_UNIT_1 ;
			break;
		default:
			vPortFree(usleep);
			DRV_DBG_ERROR("ilegal unit number");
			return NULL;
	}

	usleep->runmode	  = FALSE;
	usleep->blknum	  = USLEEP_MAX_ITEM ; 
	usleep->resolution = 500 ;	// 500 usec
	usleep->timer	= STIMER_CREATE(usleep->timer_id);
	if(usleep->timer == NULL){
		DRV_DBG_ERROR("(%p) Timer Create Error", usleep);
		vPortFree(usleep);
		return NULL;
	}	

	//DRV_DBG_BASE("(%p) USleep Driver create- timer %ld", usleep, usleep->timer_id);
	return (HANDLE) usleep;	
}

/******************************************************************************
 *  USLEEP_INIT ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
int	USLEEP_INIT (HANDLE handler)
{
	USLEEP_HANDLER_TYPE	*usleep;
	UINT32	ioctldata[3], i;
	
	if(handler == NULL){
		return FALSE;
	}
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	// Normal Mode
	if(_usleep_handler[usleep->dev_unit] == NULL){
		_usleep_handler[usleep->dev_unit] = usleep;

		// Create Item
		usleep->block = (USLEEP_INFO_TYPE *)pvPortMalloc(sizeof(USLEEP_INFO_TYPE) * usleep->blknum);
		
		if(usleep->block == NULL){
			DRV_DBG_ERROR("Memory allocation error");
			return FALSE;
		}

		for(i = 0; i < usleep->blknum; i++){
			usleep->block[i].next  = NULL ; 
			usleep->block[i].seq   = 0 ;
			usleep->block[i].ticks = 0 ;
			DRV_SPRINTF(usleep->block[i].evtname, "usv%02d", i);
			
			usleep->block[i].event = xEventGroupCreate();
		}
		for(i = 1; i < usleep->blknum; i++){
			usleep->block[i-1].next  = &(usleep->block[i]) ; 
		}

		usleep->block[usleep->blknum-1].next = NULL;
		
		// Initial Value
		USLEEP_SEQ_GEN(usleep->genseq);
		usleep->block[0].seq = 0 ;
		usleep->block[0].ticks = 0 ;
		
		// Create Semaphore
		vSemaphoreCreateBinary( usleep->mutex );
		vQueueAddToRegistry((xQueueHandle)(usleep->mutex), "mutuslp");		

		if(usleep->timer != NULL){
			UINT32 sysclock;

			_sys_clock_read( &sysclock, sizeof(UINT32));

			ioctldata[0] = sysclock ;		// Clock
			ioctldata[1] = (1000000UL / usleep->resolution) ; // Divider
			STIMER_IOCTL(usleep->timer, STIMER_SET_LOAD, ioctldata );

			ioctldata[0] = STIMER_DEV_INTCLK_MODE
					| STIMER_DEV_INTEN_MODE 
					| STIMER_DEV_INTR_ENABLE ;
			
			STIMER_IOCTL(usleep->timer, STIMER_SET_MODE, ioctldata );

			ioctldata[0] = 0 ;
			ioctldata[1] = (UINT32)&usleep_driver_hisr ;
			ioctldata[2] = (UINT32)usleep ;
			STIMER_IOCTL(usleep->timer, STIMER_SET_CALLACK, ioctldata);

			STIMER_INIT(usleep->timer);
		}	

#ifndef	USLEEP_DYNAMIC_ACTIVE
		STIMER_IOCTL(usleep->timer, STIMER_SET_ACTIVE, NULL);
#endif
	}
	
	return TRUE;
}

/******************************************************************************
 *  USLEEP_IOCTL ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
 int	USLEEP_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	USLEEP_HANDLER_TYPE	*usleep;
	unsigned int	i;
	UINT32 ioctldata[2];
	
	if(handler == NULL){
		return FALSE;
	}
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	if(cmd < USLEEP_SET_DEBUG){
		return STIMER_IOCTL(usleep->timer, cmd, data );
	}

	switch(cmd){
		case	USLEEP_SET_RUNMODE:
			usleep->runmode = *((UINT32 *)data);
			break;
			
		case	USLEEP_SET_ITEMNUM:
			if( usleep->timer == NULL ){
				usleep->blknum = *((UINT32 *)data);
			}
			break;
			
		case	USLEEP_SET_RESOLUTION:
			usleep->resolution = *((UINT32 *)data);

			if( usleep->timer != NULL ){
				_sys_clock_read( &(ioctldata[0]), sizeof(UINT32));

				//ioctldata[0] = sysclock ;		// Clock
				ioctldata[1] = (1000000UL / usleep->resolution) ; // Divider
				STIMER_IOCTL(usleep->timer, STIMER_SET_LOAD, ioctldata );			
			}
			break;
			
		case	USLEEP_GET_INFO:
			DRV_PRINTF("UsageCount:%d\n", usleep->usecnt );
			for( i = 0; i < usleep->blknum ; i++){
				if(usleep->block[i].seq != 0 ){
					if( usleep->block[i].callback == NULL ){
						DRV_PRINTF("usleep[%d] %x, %d, event %p\n"
							, i, usleep->block[i].seq
							, usleep->block[i].ticks
							, &(usleep->block[i].event) );
					}else{
						DRV_PRINTF("utimer[%d] %x, %d, callback %p\n"
							, i, usleep->block[i].seq
							, usleep->block[i].ticks
							, usleep->block[i].callback );
					}
				}else{
						DRV_PRINTF("usleep[%d] null\n", i );
				}
			}			
			break
				;
		case	USLEEP_GET_TICK:
			*((UINT32 *)data) = usleep->ticks;
			break;
			
		default:
			break;
	}

	return TRUE;
}

/******************************************************************************
 *  USLEEP_READ ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
int	USLEEP_READ(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	USLEEP_HANDLER_TYPE	*usleep;
	
	if(handler == NULL){
		return FALSE;
	}
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	return STIMER_READ(usleep->timer, addr, p_data, p_dlen);
}

/******************************************************************************
 *  USLEEP_CLOSE ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
int	USLEEP_CLOSE(HANDLE handler)
{
	USLEEP_HANDLER_TYPE	*usleep;
	UINT32 i;
	
	if(handler == NULL){
		return FALSE;
	}
	usleep = (USLEEP_HANDLER_TYPE *)handler ;
	
	vSemaphoreDelete( usleep->mutex );
	
	STIMER_IOCTL(usleep->timer, STIMER_SET_DEACTIVE, NULL);

	for(i = 0; i < usleep->blknum ; i++ ){
		xEventGroupClearBits(usleep->block[i].event,(USLEEP_EVT_BIT));
		vEventGroupDelete(usleep->block[i].event);
	}
	vPortFree(usleep->block);

	STIMER_CLOSE(usleep->timer);
	usleep->timer = NULL;

	usleep->mutex = NULL;

	_usleep_handler[usleep->dev_unit] = NULL;	
	vPortFree(handler);

	return TRUE;
}

/******************************************************************************
 *  USLEEP_SUSPEND ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

UINT32	USLEEP_SUSPEND(HANDLE handler, UINT32 ticks)
{
	USLEEP_HANDLER_TYPE	*usleep;
	USLEEP_INFO_TYPE 	*item ;
	UINT32 i;
	
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	if(usleep == NULL){
		return FALSE;
	}
	
	item = NULL;

	if( xTaskGetCurrentTaskHandle() == NULL ) {
		// HISR
		return FALSE;
	}
	
	xSemaphoreTake(usleep->mutex, portMAX_DELAY);

	ticks = ticks / usleep->resolution ;
	ticks = (ticks == 0) ? 1 : ticks;

	//do{
		// search free item
		for( i = 0; i < usleep->blknum ; i++){
			if(usleep->block[i].ticks == 0 && usleep->block[i].seq == 0){
				item = &(usleep->block[i]);
				break;
			}
		}
		if( usleep->usecnt < i ){
			usleep->usecnt = i; 
		}
		
		// mark info
		if( item != NULL ){
			xEventGroupClearBits(usleep->block[i].event,(USLEEP_EVT_BIT));
			
			USLEEP_SEQ_GEN(usleep->genseq);
			
			item->seq	= usleep->genseq;
			
			item->param 	= NULL ; 
			item->callback	= NULL ; 
			item->next	= NULL ; 

			item->ticks 	= ticks;
		}
	//}while(item == NULL);
	
	xSemaphoreGive(usleep->mutex);

#ifdef USLEEP_DYNAMIC_ACTIVE
	if( item != NULL){
		STIMER_IOCTL(usleep->timer, STIMER_SET_ACTIVE, NULL);
	}
#endif

	if(item != NULL){
		//DRV_DBG_BASE("usleep %d, %p", item->ticks, item->event);
		xEventGroupWaitBits(item->event, (USLEEP_EVT_BIT), pdTRUE, pdFALSE, portMAX_DELAY);
		//xEventGroupClearBits(usleep->block[i].event,(~(0)));	

		return TRUE;
	}
	
	return FALSE;
}

/******************************************************************************
 *  USLEEP_REGISTRY_TIMER ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/
 
HANDLE	USLEEP_REGISTRY_TIMER(HANDLE handler, UINT32 ticks, USR_CALLBACK expire_func, VOID *param)
{
	USLEEP_HANDLER_TYPE	*usleep;
	USLEEP_INFO_TYPE *item;
	UINT32 i;
	
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	if(usleep == NULL ){
		return NULL;
	}

	ticks = ticks / usleep->resolution ;
	ticks = (ticks == 0) ? 1 : ticks;
	
	item = NULL;

	xSemaphoreTake(usleep->mutex, portMAX_DELAY);

	//do{
		// search free item
		for( i = 0; i < usleep->blknum ; i++){
			if(usleep->block[i].ticks == 0 && usleep->block[i].seq == 0){
				item = &(usleep->block[i]);
				break;
			}
		}
		if( usleep->usecnt < i ){
			usleep->usecnt = i; 
		}

		// mark info
		if( item != NULL ){
			
			USLEEP_SEQ_GEN(usleep->genseq);
			
			item->seq	= usleep->genseq;

			item->param 	= param ; 
			item->callback	= expire_func ; 
			item->next	= NULL ; 
			
			item->ticks 	= ticks ;	
		}
	//}while(item == NULL);
	
	xSemaphoreGive(usleep->mutex);

#ifdef	USLEEP_DYNAMIC_ACTIVE
	if( item != NULL){
		STIMER_IOCTL(usleep->timer, STIMER_SET_ACTIVE, NULL);
	}
#endif
	if( item != NULL ){
		return (HANDLE)(item->seq);	
	}
	return NULL;
}

/******************************************************************************
 *  USLEEP_REGISTRY_TIMER ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

UINT32	USLEEP_UNREGISTRY_TIMER(HANDLE handler, HANDLE timer)
{
	USLEEP_HANDLER_TYPE	*usleep;
	USLEEP_INFO_TYPE *item;
	UINT32 i;
	
	if(handler == NULL){
		return FALSE;
	}
	usleep = (USLEEP_HANDLER_TYPE *)handler ;

	item = NULL;

	xSemaphoreTake(usleep->mutex, portMAX_DELAY);
	
	// search free item
	for( i = 0; i < usleep->blknum ; i++){
		if(usleep->block[i].seq == (UINT32)timer ){
			item = &(usleep->block[i]);
		}
	}

	// mark info
	if( item != NULL ){
		
		item->param 	= NULL ; 
		item->callback	= NULL ; 
		item->next	= NULL ; 
		
		item->ticks 	= 0 ;
		item->seq	= 0 ;
	}

	xSemaphoreGive(usleep->mutex);

	if(item != NULL){
		//DRV_DBG_BASE("delete %d", item->ticks);
		return TRUE;
	}
	
	return FALSE;	
}

/******************************************************************************
 *  usleep_driver_hisr ( )
 *
 *  Purpose :   
 *  Input   :   
 *  Output  :   
 *  Return  :   
 ******************************************************************************/

static void usleep_driver_hisr(void *handler)
{
	USLEEP_HANDLER_TYPE	*usleep;
	USLEEP_INFO_TYPE 	*item;
	VOID			*param;
	USR_CALLBACK		callback;
	unsigned int i, exist;
	BaseType_t		xHigherPriorityTaskWoken = pdTRUE;

	usleep = (USLEEP_HANDLER_TYPE *) handler;

	xSemaphoreTakeFromISR(usleep->mutex, &xHigherPriorityTaskWoken);

	usleep->ticks++ ;

	// Tick update
	exist = 0;
	callback = NULL ;
	
	for( i = 0; i < usleep->blknum ; i++){
		if(usleep->block[i].ticks != 0 ){
			usleep->block[i].ticks -- ;
			exist = 1;

			// Check Expired Items
			if(usleep->block[i].ticks == 0 && usleep->block[i].seq != 0 ){
				// expire !!
				exist = 2;
				item = &(usleep->block[i]);

				if(item->callback != NULL){
					param    = item->param ;
					callback = item->callback ;

					callback( param );	// Timer

				}else{
					xEventGroupSetBitsFromISR(usleep->block[i].event,(USLEEP_EVT_BIT), &xHigherPriorityTaskWoken);
				}

				item->param = NULL;
				item->callback = NULL;
				item->seq   = 0;
			}
		}
	}

	//PUTC((char)(0x2C-exist));

	if(exist == 0 && usleep->runmode == FALSE){
#ifdef USLEEP_DYNAMIC_ACTIVE
		STIMER_IOCTL(usleep->timer, STIMER_SET_DEACTIVE, NULL);
#endif
	}

	xSemaphoreGiveFromISR(usleep->mutex, &xHigherPriorityTaskWoken);

}


