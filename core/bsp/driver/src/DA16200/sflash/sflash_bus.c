/**
 ****************************************************************************************
 *
 * @file sflash_bus.c
 *
 * @brief SFLASH Driver
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
#include "driver.h"

#include "sflash.h"
#include "sflash/sflash_bus.h"
#include "sflash/sflash_device.h"
#include "sflash/sflash_jedec.h"
#include "sflash/sflash_primitive.h"

#define	CODE_SEC_SFLASH
#define	RO_SEC_SFLASH

#if defined (SUPPORT_SFLASH_BUS_SPI) || defined (SUPPORT_SFLASH_BUS_QSPI)
static void	MSLEEP(unsigned int	msec)
{
	portTickType xFlashRate, xLastFlashTime;	
	
	if((msec/portTICK_RATE_MS) == 0)
	{
		xFlashRate = 1;
	}
	else
	{
		xFlashRate = msec/portTICK_RATE_MS;
	}
	xLastFlashTime = xTaskGetTickCount();
	vTaskDelayUntil( &xLastFlashTime, xFlashRate );
}
#endif // defined (SUPPORT_SFLASH_BUS_SPI) || defined (SUPPORT_SFLASH_BUS_QSPI)

#ifdef	SUPPORT_SFLASH_BUS

#define	SSP_BUS_XFERMODE	SSP_SET_DMAMODE
#define	QSPI_BUS_XFERMODE	QSPI_SET_DMAMODE
#define XFC_BUS_XFERMODE	XFC_SET_DMAMODE

/******************************************************************************
 *   sflash_bus_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
void	sflash_bus_init(SFLASH_HANDLER_TYPE *sflash)
{
	DA16X_UNUSED_ARG(sflash);
}

/******************************************************************************
 * SPI
 ******************************************************************************/

#ifdef	SUPPORT_SFLASH_BUS_SPI

static	HANDLE	sflash_spi_create   (VOID *hndlr);
static	int	sflash_spi_init     (VOID *hndlr);
static	int	sflash_spi_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data);
static	int	sflash_spi_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen );
static	int	sflash_spi_lock     (VOID *hndlr, UINT32 mode);
static	int	sflash_spi_close    (VOID *hndlr);


ATTRIBUTE_RAM_RODATA
const SFLASH_BUS_TYPE	sflash_bus_spi
= {
	SSP_UNIT_0,
	&sflash_spi_create,  //(*create)
	&sflash_spi_init,    //(*init)
	&sflash_spi_ioctl,   //(*ioctl)
	&sflash_spi_transmit,//(*transmit)
	&sflash_spi_lock,    //(*lock)
	&sflash_spi_close,   //(*close)
};


/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	HANDLE	sflash_spi_create   (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;
	SFLASH_BUS_TYPE     *bus;
	HANDLE 		bushndlr;
	UINT32		ioctldata[1];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;
	bus = (SFLASH_BUS_TYPE *)(sflash->bus);

	bushndlr = SSP_CREATE(bus->devid);

	SFLASH_DBG_NONE("BUS:create, S(%p)/H(%p)", sflash, sflash->bushdlr);

	ioctldata[0] = FALSE ;
	SSP_IOCTL(bushndlr, SSP_SET_SLAVEMODE, ioctldata );
	ioctldata[0] = 100*MHz ;
	SSP_IOCTL(bushndlr, SSP_SET_SPEED, ioctldata );

	ioctldata[0] = SSP_TYPE_MOTOROLA_O0H0 ;
	SSP_IOCTL(bushndlr, SSP_SET_FORMAT, ioctldata );

	ioctldata[0] = sflash->dwidth  ;
	SSP_IOCTL(bushndlr, SSP_SET_DATASIZE, ioctldata );

//	if( OAL_GET_KERNEL_STATE( ) == TRUE ){
		SSP_IOCTL(bushndlr, SSP_BUS_XFERMODE, NULL );
//	}else{
//		SSP_IOCTL(bushndlr, SSP_SET_POLLMODE, NULL );
//	}

	ioctldata[0] = FALSE  ;
	SSP_IOCTL(bushndlr, SSP_SET_LOOPBACK, ioctldata );

	return bushndlr;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_spi_init     (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	SFLASH_DBG_NONE("BUS:init, S(%p)/H(%p)", sflash, sflash->bushdlr);

	return SSP_INIT(sflash->bushdlr);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_spi_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32  ioctldata[3];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	switch(cmd){
		case SFLASH_BUS_SET_SPEED:
			ioctldata[0] = *((UINT8 *)data) * MHz;

			if( sflash->maxspeed != 0 ){
				if( ioctldata[0] > sflash->maxspeed ){
					ioctldata[0] = sflash->maxspeed ;
				}
			}

			status = SSP_IOCTL(sflash->bushdlr, SSP_SET_SPEED, (VOID *)ioctldata);
			break;
		case SFLASH_BUS_SET_DATA_WIDTH:
			ioctldata[0] = *((UINT32 *)data);
			status = SSP_IOCTL(sflash->bushdlr, SSP_SET_DATASIZE, (VOID *)ioctldata);
			break;

		case SFLASH_BUS_SET_POLLMODE:
			status = SSP_IOCTL(sflash->bushdlr, SSP_SET_POLLMODE, NULL );
			break;
		case SFLASH_BUS_SET_INTRMODE:
			status = SSP_IOCTL(sflash->bushdlr, SSP_SET_INTRMODE, NULL );
			break;
		case SFLASH_BUS_SET_DMAMODE:
			status = SSP_IOCTL(sflash->bushdlr, SSP_SET_DMAMODE, NULL );
			break;

		case SFLASH_BUS_SET_BUSCONTROL:
			//SFLASH_BUS( *((UINT32 *)data) );
			status = TRUE;
			break;

		case SFLASH_BUS_GET_MAXRDSIZE:
			*((UINT32 *)data) = 1024;
			status = TRUE;
			break;

		case SFLASH_BUS_SET_MAXRDSIZE:
			status = TRUE;
			break;

		case SFLASH_BUS_SET_USLEEP:
			ioctldata[0] = *((UINT32 *)data);
			if( ioctldata[0] > 1000 ){
				ioctldata[0] = ioctldata[0] / 1000;
				MSLEEP(ioctldata[0]);
			}else{
				sflash_flash_sleep(ioctldata[0]);
			}
			break;

		case SFLASH_BUS_PRESET_USLEEP:
			status = TRUE;
			break;

		case SFLASH_BUS_SET_DELAYSEL:
			status = TRUE;
			break;

		default:
			status = FALSE;
			break;
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_spi_lock    (VOID *hndlr, UINT32 mode)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32  ioctldata[2];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	if( mode == TRUE ){
		// Bus Lock
		ioctldata[0] = TRUE;
		ioctldata[1] = 0;
	}else{
		// Bus Unlock
		ioctldata[0] = FALSE;
		ioctldata[1] = 0;
	}

	status = SSP_IOCTL(sflash->bushdlr, SSP_SET_LOCK, (VOID *)ioctldata);

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_spi_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen )
{
	SFLASH_HANDLER_TYPE *sflash;
	int		status;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	// TODO: if chipsel is outden pin,
	if( (mode & DARWMDCADD_CONCAT) == DARWMDCADD_CONCAT ){

		SFLASH_DBG_INFO("BUS:concatenate %d", sndlen);

		SSP_IOCTL(sflash->bushdlr, SSP_SET_CONCAT, NULL);
	}

	SFLASH_DBG_NONE("BUS:trans, %p, %p", snddata, rcvdata);

	if( sndlen > 0 ){
		SFLASH_DBG_BASE("SPI:snd");
		SFLASH_DBG_DUMP(snddata, sndlen);
	}

	status = SSP_TRANSMIT(sflash->bushdlr, snddata, sndlen, rcvdata, rcvlen);

	if( rcvlen > 0 ){
		SFLASH_DBG_BASE("SPI:rcv");
		SFLASH_DBG_DUMP(rcvdata, rcvlen);
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_spi_close    (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	SFLASH_DBG_NONE("BUS:close, S(%p)/H(%p)", sflash, sflash->bushdlr);

	return SSP_CLOSE(sflash->bushdlr);
}

#endif //SUPPORT_SFLASH_BUS_SPI


/******************************************************************************
 * QSPI
 ******************************************************************************/

#ifdef	SUPPORT_SFLASH_BUS_QSPI

static	HANDLE	sflash_qspi_create   (VOID *hndlr);
static	int	sflash_qspi_init     (VOID *hndlr);
static	int	sflash_qspi_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data);
static	int	sflash_qspi_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen );
static	int	sflash_qspi_lock     (VOID *hndlr, UINT32 mode);
static	int	sflash_qspi_close    (VOID *hndlr);


ATTRIBUTE_RAM_RODATA
const SFLASH_BUS_TYPE	sflash_bus_qspi
= {
	QSPI_UNIT_0,
	&sflash_qspi_create,  //(*create)
	&sflash_qspi_init,    //(*init)
	&sflash_qspi_ioctl,   //(*ioctl)
	&sflash_qspi_transmit,//(*transmit)
	&sflash_qspi_lock,    //(*lock)
	&sflash_qspi_close,   //(*close)
};


/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	HANDLE	sflash_qspi_create   (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;
	SFLASH_BUS_TYPE     *bus;
	HANDLE 		bushndlr;
	UINT32		ioctldata[1];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;
	bus = (SFLASH_BUS_TYPE *)(sflash->bus);

	bushndlr = QSPI_CREATE(bus->devid);

	SFLASH_DBG_NONE("BUS:create, S(%p)/H(%p)", sflash, sflash->bushdlr);

	ioctldata[0] = FALSE ;
	QSPI_IOCTL(bushndlr, QSPI_SET_SLAVEMODE, ioctldata );
	ioctldata[0] = 100*MHz ;
	QSPI_IOCTL(bushndlr, QSPI_SET_SPEED, ioctldata );

	ioctldata[0] = QSPI_TYPE_MOTOROLA_O0H0 ;
	QSPI_IOCTL(bushndlr, QSPI_SET_FORMAT, ioctldata );

	ioctldata[0] = sflash->dwidth  ;
	QSPI_IOCTL(bushndlr, QSPI_SET_DATASIZE, ioctldata );

//	if( OAL_GET_KERNEL_STATE( ) == TRUE ){
		QSPI_IOCTL(bushndlr, QSPI_BUS_XFERMODE, NULL );
//	}else{
//		QSPI_IOCTL(bushndlr, QSPI_SET_POLLMODE, NULL );
//	}

	ioctldata[0] = FALSE  ;
	QSPI_IOCTL(bushndlr, QSPI_SET_LOOPBACK, ioctldata );

	// default 1-1-1 mode
	ioctldata[0] = QSPI_BUS_NONAUTO
			| QSPI_ADDR_BYTE(0)
			| QSPI_DMMY_CYCLE(0)
			| QSPI_ACC_MODE(QSPI_ACC_WRITE)
			| QSPI_BUS_MODE(QSPI_BUS_111);

	QSPI_IOCTL(bushndlr, QSPI_SET_BUSCONTROL, ioctldata );

	return bushndlr;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_qspi_init     (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	SFLASH_DBG_NONE("BUS:init, S(%p)/H(%p)", sflash, sflash->bushdlr);

	return QSPI_INIT(sflash->bushdlr);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_qspi_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32	ioctldata[1];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	switch(cmd){
		case SFLASH_BUS_SET_SPEED:
			ioctldata[0] = *((UINT8 *)data) * MHz;

			if( sflash->maxspeed != 0 ){
				if( ioctldata[0] > sflash->maxspeed ){
					ioctldata[0] = sflash->maxspeed ;
					SFLASH_DBG_INFO("BUS:speed, %d Hz", ioctldata[0]);
				}
			}

			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_SPEED, (VOID *)ioctldata);
			break;
		case SFLASH_BUS_SET_FORMAT:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_FORMAT, (VOID *)data);
			break;

		case SFLASH_BUS_SET_DATA_WIDTH:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_DATASIZE, (VOID *)data);
			break;

		case SFLASH_BUS_SET_POLLMODE:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_POLLMODE, NULL );
			break;
		case SFLASH_BUS_SET_INTRMODE:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_INTRMODE, NULL );
			break;
		case SFLASH_BUS_SET_DMAMODE:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_DMAMODE, NULL );
			break;

		case SFLASH_BUS_SET_BUSCONTROL:
			ioctldata[0] = SFLASH_PHASE( *((UINT32 *)data) );
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_BUSCONTROL, ioctldata);
			break;

		case SFLASH_BUS_GET_MAXRDSIZE:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_GET_MAX_LENGTH, ioctldata);
			*((UINT32 *)data) = ioctldata[0];
			break;

		case SFLASH_BUS_SET_MAXRDSIZE:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_MAX_LENGTH, (VOID *)data);
			break;

		case SFLASH_BUS_SET_USLEEP:
			ioctldata[0] = *((UINT32 *)data);
			if( ioctldata[0] > 1000 ){
				ioctldata[0] = ioctldata[0] / 1000;
				MSLEEP(ioctldata[0]);
			}else{
				sflash_flash_sleep(ioctldata[0]);
			}
			break;

		case SFLASH_BUS_PRESET_USLEEP:
			status = TRUE;
			break;

		case SFLASH_BUS_SET_DELAYSEL:
			status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_DELAYSEL, (VOID *)data);
			break;

		default:
			status = FALSE;
			break;
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_qspi_lock    (VOID *hndlr, UINT32 mode)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32  ioctldata[3];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	if( mode == TRUE ){
		// Bus Lock
		ioctldata[0] = TRUE;
		ioctldata[1] = portMAX_DELAY;
		ioctldata[2] = QSPI_CSEL_0;
	}else{
		// Bus Unlock
		ioctldata[0] = FALSE;
		ioctldata[1] = portMAX_DELAY;
		ioctldata[2] = QSPI_CSEL_0;
	}

	status = QSPI_IOCTL(sflash->bushdlr, QSPI_SET_LOCK, (VOID *)ioctldata);

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_qspi_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen )
{
	SFLASH_HANDLER_TYPE *sflash;
	int		status;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	// TODO: if chipsel is outden pin,
	if( (mode & (UINT32)DARWMDCADD_CONCAT) == (UINT32)DARWMDCADD_CONCAT ){

		SFLASH_DBG_INFO("BUS:concatenate %d", sndlen);

		QSPI_IOCTL(sflash->bushdlr, QSPI_SET_CONCAT, NULL);
	}

	SFLASH_DBG_NONE("BUS:trans, %p, %p", snddata, rcvdata);

	if( sndlen > 0 ){
		SFLASH_DBG_BASE("QSPI:snd");
		SFLASH_DBG_DUMP(snddata, sndlen);
	}

	status = QSPI_TRANSMIT(sflash->bushdlr, snddata, sndlen, rcvdata, rcvlen);

	if( rcvlen > 0 ){
		SFLASH_DBG_BASE("QSPI:rcv");
		SFLASH_DBG_DUMP(rcvdata, rcvlen);
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_qspi_close    (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;
	UINT32	ioctldata[1];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	// default 1-1-1 mode
	ioctldata[0] = QSPI_BUS_NONAUTO
			| QSPI_ADDR_BYTE(0)
			| QSPI_DMMY_CYCLE(0)
			| QSPI_ACC_MODE(QSPI_ACC_WRITE)
			| QSPI_BUS_MODE(QSPI_BUS_111);

	QSPI_IOCTL(sflash->bushdlr, QSPI_SET_BUSCONTROL, ioctldata );

	SFLASH_DBG_NONE("BUS:close, S(%p)/H(%p)", sflash, sflash->bushdlr);

	return QSPI_CLOSE(sflash->bushdlr);
}

#endif //SUPPORT_SFLASH_BUS_QSPI

/******************************************************************************
 * XFC
 ******************************************************************************/

#ifdef	SUPPORT_SFLASH_BUS_XFC

static	HANDLE	sflash_xfc_create   (VOID *hndlr);
static	int	sflash_xfc_init     (VOID *hndlr);
static	int	sflash_xfc_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data);
static	int	sflash_xfc_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen );
static	int	sflash_xfc_lock     (VOID *hndlr, UINT32 mode);
static	int	sflash_xfc_close    (VOID *hndlr);


ATTRIBUTE_RAM_RODATA
const SFLASH_BUS_TYPE	sflash_bus_xfc
= {
	XFC_UNIT_0,
	&sflash_xfc_create,  //(*create)
	&sflash_xfc_init,    //(*init)
	&sflash_xfc_ioctl,   //(*ioctl)
	&sflash_xfc_transmit,//(*transmit)
	&sflash_xfc_lock,    //(*lock)
	&sflash_xfc_close,   //(*close)
};


/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	HANDLE	sflash_xfc_create   (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;
	SFLASH_BUS_TYPE     *bus;
	HANDLE 		bushndlr;
	UINT32		ioctldata[2];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;
	bus = (SFLASH_BUS_TYPE *)(sflash->bus);

	bushndlr = XFC_CREATE(bus->devid);

	SFLASH_DBG_NONE("BUS:create, S(%p)/H(%p)", sflash, sflash->bushdlr);

	//ioctldata[0] = FALSE ;
	//XFC_IOCTL(bushndlr, XFC_SET_SLAVEMODE, ioctldata );

	ioctldata[0] = (1*MBYTE) ;
	XFC_IOCTL(bushndlr, XFC_SET_MAX_LENGTH, ioctldata );

	_sys_clock_read( ioctldata, sizeof(UINT32) );
	XFC_IOCTL(bushndlr, XFC_SET_CORECLOCK, ioctldata );

	ioctldata[0] = 100*MHz ;
	XFC_IOCTL(bushndlr, XFC_SET_SPEED, ioctldata );

	ioctldata[0] = XFC_TYPE_MOTOROLA_O0H0 ;
	XFC_IOCTL(bushndlr, XFC_SET_FORMAT, ioctldata );

	ioctldata[0] = sflash->dwidth  ;
	XFC_IOCTL(bushndlr, XFC_SET_DATASIZE, ioctldata );

//	if ( OAL_GET_KERNEL_STATE( ) == TRUE ) {
		XFC_IOCTL(bushndlr, XFC_BUS_XFERMODE, NULL );
//	}else{
//		XFC_IOCTL(bushndlr, XFC_SET_INTRMODE, NULL );
//	}

	//ioctldata[0] = FALSE  ;
	//XFC_IOCTL(bushndlr, XFC_SET_LOOPBACK, ioctldata );

	// Config DMA
	ioctldata[0] =    XFC_DMA_MP0_BST(8)
			| XFC_DMA_MP0_IDLE(1)
			| XFC_DMA_MP0_HSIZE(XHSIZE_DWORD)
			| XFC_DMA_MP0_AI(XFC_ADDR_INCR)
			;
	XFC_IOCTL(bushndlr, XFC_SET_DMA_CFG, ioctldata);

	// Assign PORT
	ioctldata[0] = XFC_CSEL_0;
	ioctldata[1] = 4; // 4-wire
	XFC_IOCTL( bushndlr, XFC_SET_WIRE, (VOID *)ioctldata);

	ioctldata[0] = XFC_CSEL_1;
	ioctldata[1] = 4; // 4-wire
	XFC_IOCTL( bushndlr, XFC_SET_WIRE, (VOID *)ioctldata);


	// default 1-1-0-1 mode

	ioctldata[0] = XFC_SPI_TIMEOUT_EN
		| XFC_SPI_PHASE_CMD_1BYTE
		| XFC_SPI_PHASE_ADDR_3BYTE
		| XFC_SPI_PHASE_MODE_1BIT
		| XFC_SET_SPI_DUMMY_CYCLE(0);

	ioctldata[1] = XFC_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // CMD
					, SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // ADDR
					, SPI_BUS_TYPE(0,0,XFC_BUS_SPI)
					, SPI_BUS_TYPE(1,0,XFC_BUS_SPI) ); // DATA

	XFC_IOCTL(bushndlr, XFC_SET_BUSCONTROL, ioctldata );

	return bushndlr;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_xfc_init     (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	SFLASH_DBG_NONE(" [%s] BUS:init, S(%p)/H(%p) \r\n", __func__, sflash, sflash->bushdlr);

	return XFC_INIT(sflash->bushdlr);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define XFCBUS_PHASE_SET(phase,bit,flag)			\
	SPI_BUS_TYPE( ((((phase>>bit)&0x08)!=0)?1:0),flag, 	\
	((((phase>>bit)&0x08)!=0)?((phase>>bit)&0x07):0) )

ATTRIBUTE_RAM_FUNC
static  void   sflash_xfc_phase_conv(UINT32 phase, UINT32 *ioctldata)
{
	// BUS CONTROL [0]
	UINT32	flag_dtr;

	flag_dtr = ((phase&DARWMDCADD_DTR) != 0)? 1 : 0;

	ioctldata[0] = XFC_SPI_TIMEOUT_EN;

	switch((phase&DARWMDCADD_AXH))	{
	case DARWMDCADD_MODEBIT(3): ioctldata[0] |= XFC_SPI_PHASE_MODE_8BIT; break;
	case DARWMDCADD_MODEBIT(2): ioctldata[0] |= XFC_SPI_PHASE_MODE_4BIT; break;
	case DARWMDCADD_MODEBIT(1): ioctldata[0] |= XFC_SPI_PHASE_MODE_2BIT; break;
	//default:		ioctldata[0] |= XFC_SPI_PHASE_MODE_1BIT; break;
	default:		break;
	}

	ioctldata[0] |= ((phase&DARWMDCADD_CMD) != 0)
			? XFC_SPI_PHASE_CMD_2BYTE : XFC_SPI_PHASE_CMD_1BYTE;

	ioctldata[0] |= ((phase&DARWMDCADD_ADR) != 0)
			? XFC_SPI_PHASE_ADDR_4BYTE : XFC_SPI_PHASE_ADDR_3BYTE;

	ioctldata[0] |= XFC_SET_SPI_DUMMY_CYCLE(((phase>>16) & 0x1f));

	// BUS CONTROL [1]
	ioctldata[1] = XFC_SET_SPI_BUS_TYPE(
				  XFCBUS_PHASE_SET(phase, 12, flag_dtr) // CMD
				, XFCBUS_PHASE_SET(phase,  8, flag_dtr) // ADDR
				, XFCBUS_PHASE_SET(phase,  4, flag_dtr) // DMMY
				, XFCBUS_PHASE_SET(phase,  0, flag_dtr) // DATA
			);

	SFLASH_DBG_BUS(" [%s] BUS: [%08x (A %d) > %08x.%08x] \r\n"
			, __func__, phase
			, (((phase&DARWMDCADD_ADR) != 0)+3)
			, ioctldata[0], ioctldata[1] );
}

ATTRIBUTE_RAM_FUNC
static	int	sflash_xfc_ioctl    (VOID *hndlr, UINT32 cmd , VOID *data)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32	ioctldata[2];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	switch(cmd){
		case SFLASH_BUS_SET_SPEED:
			ioctldata[0] = *((UINT8 *)data) * MHz;

			if( sflash->maxspeed != 0 ){
				if( ioctldata[0] > sflash->maxspeed ){
					ioctldata[0] = sflash->maxspeed ;
				}
			}
			SFLASH_DBG_NONE("BUS:speed, %d Hz", ioctldata[0]);

			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_SPEED, (VOID *)ioctldata);
			break;
		case SFLASH_BUS_SET_FORMAT:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_FORMAT, (VOID *)data);
			break;

		case SFLASH_BUS_SET_DATA_WIDTH:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_DATASIZE, (VOID *)data);
			break;

		case SFLASH_BUS_SET_POLLMODE:
			status = FALSE;
			break;
		case SFLASH_BUS_SET_INTRMODE:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_INTRMODE, NULL );
			break;
		case SFLASH_BUS_SET_DMAMODE:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_DMAMODE, NULL );
			break;

		case SFLASH_BUS_SET_XIPMODE:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_XIPMODE, (VOID *)data);
			break;

		case SFLASH_BUS_SET_BUSCONTROL:
			// TODO:
			sflash_xfc_phase_conv( *((UINT32 *)data), ioctldata );
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_BUSCONTROL, ioctldata);

			switch(SFLASH_BUS_GRP(sflash->accmode)){
			case SFLASH_BUS_222_GRP:
			case SFLASH_BUS_122_GRP:
			case SFLASH_BUS_112_GRP:
			case SFLASH_BUS_111_GRP:
				ioctldata[0] = FALSE;
				break;
			default:	/* Quad */
				ioctldata[0] = TRUE;
				break;
			}
			XFC_IOCTL(sflash->bushdlr, XFC_SET_PINCONTROL, ioctldata);
			break;

		case SFLASH_BUS_GET_MAXRDSIZE:
			status = XFC_IOCTL(sflash->bushdlr, XFC_GET_MAX_LENGTH, ioctldata);
			*((UINT32 *)data) = ioctldata[0];
			break;

		case SFLASH_BUS_SET_MAXRDSIZE:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_MAX_LENGTH, (VOID *)data);
			break;

		case SFLASH_BUS_SET_USLEEP:
			status = TRUE;
			break;

		case SFLASH_BUS_PRESET_USLEEP:
			{
				ioctldata[0] = SFLASH_UTIME_UNIT(*((UINT32 *)data));
				ioctldata[1] = SFLASH_UTIME_COUNT(*((UINT32 *)data));
				status = XFC_IOCTL(sflash->bushdlr, XFC_SET_TIMEOUT, ioctldata);
			}
			break;

		case SFLASH_BUS_SET_DELAYSEL:
			status = XFC_IOCTL(sflash->bushdlr, XFC_SET_DELAYSEL, (VOID *)data);
			break;

		default:
			status = FALSE;
			break;
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_xfc_lock    (VOID *hndlr, UINT32 mode)
{
	SFLASH_HANDLER_TYPE *sflash;
	int	status;
	UINT32  ioctldata[3];

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	//SFLASH_DBG_NONE("BUS:ioctl, S(%p)/H(%p)", sflash, sflash->bushdlr);

	if( mode == TRUE ){
		// Bus Lock
		ioctldata[0] = TRUE;
		ioctldata[1] = portMAX_DELAY;

		switch(sflash->bussel){
		case SFLASH_CSEL_1:
			ioctldata[2] = XFC_CSEL_1; break;
		case SFLASH_CSEL_2:
			ioctldata[2] = XFC_CSEL_2; break;
		case SFLASH_CSEL_3:
			ioctldata[2] = XFC_CSEL_3; break;
		default:
			ioctldata[2] = XFC_CSEL_0; break;
		}
	}else{
		// Bus Unlock
		ioctldata[0] = FALSE;
		ioctldata[1] = portMAX_DELAY;
		ioctldata[2] = XFC_CSEL_0;
	}

	status = XFC_IOCTL(sflash->bushdlr, XFC_SET_LOCK, (VOID *)ioctldata);

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_xfc_transmit (VOID *hndlr
					, UINT32 cmd, UINT32 addr, UINT32 mode
					, VOID *snddata, UINT32 sndlen
					, VOID *rcvdata, UINT32 rcvlen )
{
	SFLASH_HANDLER_TYPE *sflash;
	int		status;

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

	// TODO: if chipsel is outden pin,
	if( (mode & (UINT32)DARWMDCADD_CONCAT) == (UINT32)DARWMDCADD_CONCAT ){

		SFLASH_DBG_INFO("BUS:concatenate %d", sndlen);

		XFC_IOCTL(sflash->bushdlr, XFC_SET_CONCAT, NULL);
	}

	SFLASH_DBG_NONE("BUS:trans, %p, %p", snddata, rcvdata);

	if( sndlen > 0 ){
		SFLASH_DBG_BASE("XFC:snd");
		SFLASH_DBG_DUMP(snddata, sndlen);
	}

	status = XFC_SFLASH_TRANSMIT(sflash->bushdlr
				, cmd, addr, mode
				, snddata, sndlen, rcvdata, rcvlen);

	if( rcvlen > 0 ){
		SFLASH_DBG_BASE("XFC:rcv");
		SFLASH_DBG_DUMP(rcvdata, rcvlen);
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static	int	sflash_xfc_close    (VOID *hndlr)
{
	SFLASH_HANDLER_TYPE *sflash;
#ifdef	SUPPORT_FLASH_CACHE
	// Skip
#else	//SUPPORT_FLASH_CACHE
	UINT32	ioctldata[2];
#endif	//SUPPORT_FLASH_CACHE

	sflash = (SFLASH_HANDLER_TYPE *)hndlr;

#ifdef	SUPPORT_FLASH_CACHE
	// Skip
#else	//SUPPORT_FLASH_CACHE
	// default 1-1-1 mode
	ioctldata[0] = XFC_SPI_TIMEOUT_EN
		| XFC_SPI_PHASE_CMD_1BYTE
		| XFC_SPI_PHASE_ADDR_3BYTE
		| XFC_SPI_PHASE_MODE_1BIT
		| XFC_SET_SPI_DUMMY_CYCLE(0);

	ioctldata[1] = XFC_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // CMD
					, SPI_BUS_TYPE(1,0,XFC_BUS_SPI) // ADDR
					, SPI_BUS_TYPE(0,0,XFC_BUS_SPI)
					, SPI_BUS_TYPE(1,0,XFC_BUS_SPI) ); // DATA

	XFC_IOCTL(sflash->bushdlr, XFC_SET_BUSCONTROL, ioctldata );
#endif	//SUPPORT_FLASH_CACHE

	SFLASH_DBG_NONE("BUS:close, S(%p)/H(%p)", sflash, sflash->bushdlr);

	return XFC_CLOSE(sflash->bushdlr);
}

#endif //SUPPORT_SFLASH_BUS_XFC


#endif /*SUPPORT_SFLASH_BUS*/
