/**
 ****************************************************************************************
 *
 * @file sflash.c
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
#include "common_def.h"

#include "sflash.h"
#include "sflash/sflash_bus.h"
#include "sflash/sflash_device.h"
#include "sflash/sflash_jedec.h"
#include "sflash/sflash_primitive.h"

#include "da16x_compile_opt.h"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#ifdef	SUPPORT_SFLASH_DEVICE

//---------------------------------------------------------
//	DRIVER :: Declaration
//---------------------------------------------------------

#undef	SFLASH_INFO_DETAIL
#undef	SFLASH_CONTROL_WDOG
#undef	SFLASH_CONTROL_RESET

#define SFLASH_BOOT_LDO

#define SUPPORT_STD_AT25SF321B
#define SUPPORT_STD_MX25R1635F_ULTRALOWPOWER
#define SUPPORT_STD_W25Q32JW_CMP


#define	SFLASH_INTR_MASK	(0x80000000)
#define	SFLASH_MAX_ITER_PROBE	64

#define	SFLASH_PRINTF(...)	//PRINTF( __VA_ARGS__ )
#define SFLASH_DUMP(...)	//DRV_DBG_DUMP( __VA_ARGS__ )
#define	SFLASH_DBG_JTAG(...)	Printf( __VA_ARGS__ )

extern UINT32	da16x_sflash_setup_parameter(UINT32 *parameters);

static UINT32	find_sflash_phy_addr(void);
static int 	sflash_set_bus_interface(SFLASH_HANDLER_TYPE *sflash, VOID *data);

// Zero-init(BSS) or Uninit data does not need to be placed in the RAMDATA section.
DATA_ALIGN(4) static char STATIC_SFLASH[sizeof(SFLASH_HANDLER_TYPE)];
DATA_ALIGN(4) static char STATIC_TX_BUFFER[SFLASH_JEDEC_BUFSIZ];
DATA_ALIGN(4) static char STATIC_RX_BUFFER[SFLASH_JEDEC_BUFSIZ];
DATA_ALIGN(4) static char STATIC_WORK[sizeof(SFLASH_WORK_TYPE)];

//---------------------------------------------------------
//	DRIVER :: Switchable Functions
//---------------------------------------------------------
#ifdef	SUPPORT_SFLASH_ROMLIB

extern int	sflash_mrom_low_init(HANDLE handler);
extern int	sflash_mrom_low_ioctl(HANDLE handler, UINT32 cmd , VOID *data );
extern int	sflash_mrom_low_read(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);
extern int	sflash_mrom_low_write(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);

extern int	sflash_mrom_device_print_info(SFLASH_HANDLER_TYPE *sflash
				, UINT32 *mode);
extern int	sflash_mrom_device_sector_op(SFLASH_HANDLER_TYPE *sflash
			, UINT32 mode, UINT32 addr, UINT32 dsize);
extern int	sflash_mrom_device_sector_vend(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
extern int	sflash_mrom_device_power_down(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
extern int	sflash_mrom_set_bus_mode(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
extern int	sflash_mrom_set_sfdp_info(SFLASH_HANDLER_TYPE *sflash, VOID *data);
extern int	sflash_mrom_get_mode(SFLASH_HANDLER_TYPE *sflash, VOID *data);

extern void	sflash_mrom_interrupt_deactive(SFLASH_HANDLER_TYPE *sflash);
extern void	sflash_mrom_interrupt_active(SFLASH_HANDLER_TYPE *sflash);

extern void	sflash_mrom_device_init(SFLASH_HANDLER_TYPE *sflash);
extern void	sflash_mrom_primitive_init(SFLASH_HANDLER_TYPE *sflash);

extern const SFLASH_DEV_TYPE	sflash_mrom_anonymous;

#define	sflash_low_init(...)		sflash_mrom_low_init( __VA_ARGS__ )
#define	sflash_low_ioctl(...)		sflash_mrom_low_ioctl( __VA_ARGS__ )
#define	sflash_low_read(...)		sflash_mrom_low_read( __VA_ARGS__ )
#define	sflash_low_write(...)		sflash_mrom_low_write( __VA_ARGS__ )
#define	sflash_device_print_info(...)	sflash_mrom_device_print_info( __VA_ARGS__ )
#define	sflash_device_sector_op(...)	sflash_mrom_device_sector_op( __VA_ARGS__ )
#define	sflash_device_sector_vend(...)	sflash_mrom_device_sector_vend( __VA_ARGS__ )
#define	sflash_device_power_down(...)	sflash_mrom_device_power_down( __VA_ARGS__ )
#define	sflash_set_bus_mode(...)	sflash_mrom_set_bus_mode( __VA_ARGS__ )
#define	sflash_set_sfdp_info(...)	sflash_mrom_set_sfdp_info( __VA_ARGS__ )
#define	sflash_get_mode(...)		sflash_mrom_get_mode( __VA_ARGS__ )
#define	sflash_interrupt_deactive(...)	sflash_mrom_interrupt_deactive( __VA_ARGS__ )
#define	sflash_interrupt_active(...)	sflash_mrom_interrupt_active( __VA_ARGS__ )
#define	sflash_primitive_init(...)	sflash_mrom_primitive_init( __VA_ARGS__ )
#define	sflash_device_init(...)		sflash_mrom_device_init( __VA_ARGS__ )
#define	sflash_anonymous		sflash_mrom_anonymous

#else	//SUPPORT_SFLASH_ROMLIB

int	sflash_local_low_init(HANDLE handler);
int	sflash_local_low_ioctl(HANDLE handler, UINT32 cmd , VOID *data );
int	sflash_local_low_read(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);
int	sflash_local_low_write(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen);

static int	sflash_local_device_print_info(SFLASH_HANDLER_TYPE *sflash
				, UINT32 *mode);
static int	sflash_local_device_sector_op(SFLASH_HANDLER_TYPE *sflash
			, UINT32 mode, UINT32 addr, UINT32 dsize);
static int	sflash_local_device_sector_vend(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
static int	sflash_local_device_power_down(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
static int	sflash_local_set_bus_mode(SFLASH_HANDLER_TYPE *sflash
			, UINT32 cmd, VOID *data);
static int	sflash_local_set_sfdp_info(SFLASH_HANDLER_TYPE *sflash, VOID *data);
static int	sflash_local_get_mode(SFLASH_HANDLER_TYPE *sflash, VOID *data);

static void	sflash_local_interrupt_deactive(SFLASH_HANDLER_TYPE *sflash);
static void	sflash_local_interrupt_active(SFLASH_HANDLER_TYPE *sflash);

#define	sflash_low_init(...)		sflash_local_low_init( __VA_ARGS__ )
#define	sflash_low_ioctl(...)		sflash_local_low_ioctl( __VA_ARGS__ )
#define	sflash_low_read(...)		sflash_local_low_read( __VA_ARGS__ )
#define	sflash_low_write(...)		sflash_local_low_write( __VA_ARGS__ )
#define	sflash_device_print_info(...)	sflash_local_device_print_info( __VA_ARGS__ )
#define	sflash_device_sector_op(...)	sflash_local_device_sector_op( __VA_ARGS__ )
#define	sflash_device_sector_vend(...)	sflash_local_device_sector_vend( __VA_ARGS__ )
#define	sflash_device_power_down(...)	sflash_local_device_power_down( __VA_ARGS__ )
#define	sflash_set_bus_mode(...)	sflash_local_set_bus_mode( __VA_ARGS__ )
#define	sflash_set_sfdp_info(...)	sflash_local_set_sfdp_info( __VA_ARGS__ )
#define	sflash_get_mode(...)		sflash_local_get_mode( __VA_ARGS__ )
#define	sflash_interrupt_deactive(...)	sflash_local_interrupt_deactive( __VA_ARGS__ )
#define	sflash_interrupt_active(...)	sflash_local_interrupt_active( __VA_ARGS__ )
#define	sflash_primitive_init(...)	sflash_local_primitive_init( __VA_ARGS__ )
#define	sflash_device_init(...)		sflash_local_device_init( __VA_ARGS__ )
#define	sflash_anonymous		sflash_local_anonymous

#endif	//SUPPORT_SFLASH_ROMLIB

int 	SFLASH_PATCH_ULPower (HANDLE handler, UINT32 rtosmode);
int	SFLASH_PATCH_W25Q32JW(HANDLE handler, UINT32 rtosmode);

//---------------------------------------------------------
//	DRIVER :: internal variables
//---------------------------------------------------------

static SFLASH_HANDLER_TYPE *_sflash_driver[SFLASH_UNIT_MAX]	;
static int		_sflash_instance[SFLASH_UNIT_MAX]	;


//---------------------------------------------------------
//	DRIVER :: DEVICE
//---------------------------------------------------------

#define	sys_sflash_dev_list	(&sflash_anonymous)

//---------------------------------------------------------
//	DRIVER :: BUS
//---------------------------------------------------------

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
ATTRIBUTE_RAM_FUNC
HANDLE	SFLASH_CREATE( UINT32 dev_type )
{
	SFLASH_HANDLER_TYPE	*sflash;

	/* Allocate */
	if( _sflash_driver[dev_type] == NULL ){
		//sflash = (SFLASH_HANDLER_TYPE *)DRIVER_MALLOC(sizeof(SFLASH_HANDLER_TYPE));
		sflash = (SFLASH_HANDLER_TYPE *)&STATIC_SFLASH[0];	
		if( sflash == NULL ){
			return NULL;
		}
		/* Clear */
		DRIVER_MEMSET(sflash, 0, sizeof(SFLASH_HANDLER_TYPE));
	}else{
		sflash = _sflash_driver[dev_type];
	}

	/* Address Mapping */
	switch((SFLASH_UNIT_TYPE)dev_type){
		case	SFLASH_UNIT_0:
				sflash->dev_unit = dev_type ;
				sflash->dev_addr = find_sflash_phy_addr();
				sflash->instance = (_sflash_instance[dev_type]) ;
#if	defined(SUPPORT_SFLASH_BUS_XFC)
				sflash->bus      = (VOID *)&sflash_bus_xfc;
#elif	defined(SUPPORT_SFLASH_BUS_QSPI)
				sflash->bus      = (VOID *)&sflash_bus_qspi;
#elif   defined(SUPPORT_SFLASH_BUS_SPI)
				sflash->bus      = (VOID *)&sflash_bus_spi;
#else
#error	"bus controller is not defined"
#endif //SUPPORT_SFLASH_BUS_SPI

				_sflash_instance[dev_type] ++;
				break;

		default:
				SFLASH_DBG_ERROR("ilegal unit number");

				return NULL;
	}

	if (sflash->instance == 0) {
		SFLASH_BUS_TYPE *bus;

		_sflash_driver[sflash->dev_unit] = sflash;

		bus = (SFLASH_BUS_TYPE *)sflash->bus ;

		sflash->dwidth = sizeof(UINT32)*8;
		sflash->jedecid = 0;
		sflash->chbkup  = 0;
		sflash->xipmode = 0; // None
		sflash->bussel	= SFLASH_CSEL_0;
		sflash->slptim	= 0;
		sflash->susmode = 0; // suspend mode
		sflash->psustime = 0;
		sflash->esustime = 0;

		/* chg by_sjlee */
		sflash->txbuffer = (UINT8 *)&STATIC_TX_BUFFER[0];
		sflash->rxbuffer = (UINT8 *)&STATIC_RX_BUFFER[0];
		sflash->work	 = (SFLASH_WORK_TYPE *)&STATIC_WORK[0];
		//sflash->txbuffer = (UINT8 *)HAL_MALLOC(SFLASH_JEDEC_BUFSIZ);
		//sflash->rxbuffer = (UINT8 *)HAL_MALLOC(SFLASH_JEDEC_BUFSIZ);
		//sflash->work	 = (SFLASH_WORK_TYPE *)HAL_MALLOC(sizeof(SFLASH_WORK_TYPE));
		DRV_MEMSET(sflash->work, 0x00, sizeof(SFLASH_WORK_TYPE));

		sflash->bushdlr = bus->create(sflash);

		sflash->device = (VOID *)sys_sflash_dev_list ;

		sflash_bus_init(sflash);
		sflash_primitive_init(sflash);
		sflash_device_init(sflash);
	}

	SFLASH_DBG_INFO("(%p) SFLASH create, base %p", sflash, (UINT32 *)sflash->dev_addr );

	return (HANDLE) sflash;
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
int 	SFLASH_INIT (HANDLE handler)
{
	return sflash_low_init(handler);
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   Register Access & Callback Registration
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int SFLASH_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	if( cmd == SFLASH_BUS_INTERFACE ){
		return sflash_set_bus_interface((SFLASH_HANDLER_TYPE *) handler, data );
	}

	return sflash_low_ioctl(handler, cmd, data);
}

ATTRIBUTE_RAM_FUNC
static int sflash_set_bus_interface(SFLASH_HANDLER_TYPE *sflash, VOID *data)
{
	SFLASH_BUS_TYPE *bus;

	SFLASH_DBG_NONE("%s:%d", __func__, ((UINT32 *)data)[0] );

	if( ((UINT32 *)data)[0] == 1 ){
#ifdef	SUPPORT_SFLASH_BUS_XFC
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->close(sflash);
		sflash->bus = (VOID *)&sflash_bus_xfc;
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->create(sflash);
		bus->init(sflash);
#endif	//SUPPORT_SFLASH_BUS_QSPI
	}else
	if( ((UINT32 *)data)[0] == 2 ){
#ifdef	SUPPORT_SFLASH_BUS_QSPI
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->close(sflash);
		sflash->bus = (VOID *)&sflash_bus_qspi;
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->create(sflash);
		bus->init(sflash);
#endif	//SUPPORT_SFLASH_BUS_QSPI
	}else{
#ifdef	SUPPORT_SFLASH_BUS_SPI
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->close(sflash);
		sflash->bus = (VOID *)&sflash_bus_spi;
		bus = (SFLASH_BUS_TYPE *)sflash->bus ;
		bus->create(sflash);
		bus->init(sflash);
#endif	//SUPPORT_SFLASH_BUS_SPI
	}

	return TRUE;
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
int SFLASH_READ (HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	return sflash_low_read(handler, addr, p_data, p_dlen);
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
int SFLASH_WRITE (HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	return sflash_low_write(handler, addr, p_data, p_dlen);
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
int 	SFLASH_CLOSE(HANDLE handler)
{
	SFLASH_HANDLER_TYPE	*sflash;
	SFLASH_BUS_TYPE		*bus;

	/* Handler Checking */
	if(handler == NULL) {
		return FALSE;
	}
	sflash = (SFLASH_HANDLER_TYPE *)handler;

	if ( _sflash_instance[sflash->dev_unit] > 0) {
		_sflash_instance[sflash->dev_unit] -- ;

		if(  _sflash_instance[sflash->dev_unit] == 0) {
			bus = sflash->bus;

			bus->close(sflash);

			//HAL_FREE(sflash->work);
			//HAL_FREE(sflash->txbuffer);
			//HAL_FREE(sflash->rxbuffer);

			_sflash_driver[sflash->dev_unit] = NULL;

			//DRIVER_FREE(sflash);
		}
	}

	return TRUE;
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
static UINT32	find_sflash_phy_addr(void)
{
	UINT32	src;
	/* Check Remapped Address */
#if	defined(SUPPORT_SFLASH_BUS_XFC)
	src = DA16X_XFC_BASE;
#elif	defined(SUPPORT_SFLASH_BUS_QSPI)
	src = QSPI_BASE_0;
#elif   defined(SUPPORT_SFLASH_BUS_SPI)
	src = SPI_BASE_0;
#else
#error	"bus controller is not defined"
#endif //SUPPORT_SFLASH_BUS_SPI
	return src  ;
}

#ifdef	SUPPORT_SFLASH_ROMLIB
	// ROM Library linked

#else	//SUPPORT_SFLASH_ROMLIB

/******************************************************************************
 *   ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int 	sflash_local_low_init(HANDLE handler)
{
	SFLASH_HANDLER_TYPE	*sflash;
	int			status, passcnt;
	SFLASH_DEV_TYPE		*sflash_dev;
	UINT8			*iddata;
	UINT32	 		max_cfi;
	SFLASH_BUS_TYPE 	*bus;
	SFLASH_WORK_TYPE	*work;

	/* Handler Checking */
	if(handler == NULL) {
		return FALSE;
	}
	sflash = (SFLASH_HANDLER_TYPE *)handler;

	/* Init Condition Check */
	if( sflash->device == NULL )
	{
		return FALSE;
	}

	if( sflash->instance != 0 )
	{
		return TRUE;
	}

	bus = (SFLASH_BUS_TYPE *)sflash->bus ;
	bus->init(sflash);

	status = FALSE;

	sflash_dev = (SFLASH_DEV_TYPE *)(sflash->device);
	sflash->accmode = SFLASH_BUS_3BADDR|SFLASH_BUS_111 ;

	if ( sflash->jedecid != 0 && sflash->jedecid != 0xffffffff ) {
		UINT32	jedecid;

		work = (SFLASH_WORK_TYPE *)(sflash->work);

		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5910));

		sflash->max_cfi = 0;
		sflash_dev->buslock(sflash, TRUE);

		sflash_interrupt_deactive(sflash);
#ifdef	SFLASH_CONTROL_RESET		
		status = sflash_dev->reset(sflash, 1); // SLR, Fast Reset
#endif	//SFLASH_CONTROL_RESET		
		max_cfi = sflash_dev->probe(sflash, (VOID *)(sflash_dev), work->id_cfi);
		sflash_interrupt_active(sflash);

		sflash_dev->buslock(sflash, FALSE);
		iddata = work->id_cfi;

		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5911));

		jedecid = ((UINT32)(iddata[0]) << 24)
			| ((UINT32)(iddata[1]) << 16)
			| ((UINT32)(iddata[2]) <<  8)
			| ((UINT32)(iddata[4]) <<  0);

		if( sflash->jedecid != jedecid ){
			return FALSE;
		}

		return TRUE;
	}

	work = (SFLASH_WORK_TYPE *)(sflash->work);
	work->num_secgrp	= ID_CFI_MAX_SECGRP_NUM;

	for (passcnt = 0; passcnt < SFLASH_MAX_ITER_PROBE; passcnt++ ) {

		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5912));

		sflash->max_cfi = 0;
		sflash_dev->buslock(sflash, TRUE);

		sflash_interrupt_deactive(sflash);
		switch(passcnt){
			case	0:
#ifdef SFLASH_CONTROL_RESET				
				status = sflash_dev->reset(sflash, 1); // SLR, Fast Reset (for VSIM)
#endif //SFLASH_CONTROL_RESET				
				break;
			case	4:
				status = sflash_dev->reset(sflash, 2); // SLR, Slow Reset
				break;
			case	8:
				status = sflash_dev->reset(sflash, 0); // SLR, Wakeup
				break;
			default:
				status = TRUE; // SLR, Wait, No Access
				break;
		}		
		max_cfi = sflash_dev->probe(sflash, (VOID *)(sflash_dev), work->id_cfi);
		sflash_interrupt_active(sflash);

		sflash_dev->buslock(sflash, FALSE);
		iddata = work->id_cfi;

		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5913));

		if( max_cfi == 0 ){
			SFLASH_DBG_ERROR("unknown sflash device");
			return FALSE;
		}

		status = FALSE;
		{
			sflash_dev = (SFLASH_DEV_TYPE	*)sys_sflash_dev_list;

			SFLASH_DBG_INFO("%s:compare with %s [%02x.%02x.%02x.%02x.%02x]"
				, __func__, sflash_dev->name
				, iddata[0], iddata[1], iddata[2], iddata[3], iddata[4]);

			if( sflash_dev->mfr_id == iddata[0]
			   && sflash_dev->dev_id == iddata[1]
			   && sflash_dev->density == iddata[2] )
			{
				status = TRUE ;
				sflash->jedec.cmdlist = sflash_dev->cmdlist ;
			}
		}

		sflash->jedecid = ((UINT32)(iddata[0]) << 24)
				| ((UINT32)(iddata[1]) << 16)
				| ((UINT32)(iddata[2]) <<  8)
				| ((UINT32)(iddata[4]) <<  0);

		ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5914));
		ASIC_DBG_TRIGGER(sflash->jedecid);

		if( ((sflash->jedecid & 0xffffff00) == 0x00000000)
		    || ((sflash->jedecid & 0xffffff00) == 0xffffff00) ){
			if( passcnt >= (SFLASH_MAX_ITER_PROBE-1) ){
				return FALSE;
			}
			sflash->accmode = /*SFLASH_BUS_3BADDR|*/SFLASH_BUS_111 ;
		}
		else {
			break;
		}
	}

	status = TRUE;

	{
		if( sflash->jedec.sfdp == NULL ){
			sflash->max_cfi = max_cfi;

			/* SFDP or CFI */
			sflash_dev->buslock(sflash, TRUE);

			ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5915));

			sflash_interrupt_deactive(sflash);
			max_cfi = sflash_dev->probe(sflash, sflash->device, work->id_cfi );
			sflash_interrupt_active(sflash);

			ASIC_DBG_TRIGGER(MODE_HAL_STEP(0xF5916));

			sflash_dev->buslock(sflash, FALSE);

			if( max_cfi > 0 ){
				sflash->max_cfi = max_cfi ;
			}
		}
	}

	return status;
}

/******************************************************************************
 *   ( )
 *
 *  Purpose :   Register Access & Callback Registration
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int sflash_local_low_ioctl(HANDLE handler, UINT32 cmd , VOID *data )
{
	SFLASH_HANDLER_TYPE	*sflash;
	SFLASH_DEV_TYPE		*device;
	int			status ;

	/* Handler Checking */
	if(handler == NULL) {
		return FALSE;
	}
	sflash = (SFLASH_HANDLER_TYPE *)handler;

	if( sflash->device == NULL ){
		SFLASH_DBG_INFO("SFLASH Driver not ready");

		switch(cmd){
			case SFLASH_BUS_POLLMODE:
			case SFLASH_BUS_INTRMODE:
			case SFLASH_BUS_DMAMODE:
			case SFLASH_SET_MAXBURST:
				status = sflash_local_set_bus_mode( sflash, cmd, data );
				break;

			case SFLASH_BUS_MAXSPEED:
				sflash->maxspeed = ((UINT32 *)data)[0];
				status = TRUE;
				break;

			case SFLASH_SET_INFO:
				status = sflash_local_set_sfdp_info(sflash, data);
				break;

			case SFLASH_SET_BOOTMODE:
				if( ((UINT32 *)data)[0] == TRUE ){
					sflash->chbkup = sflash->chbkup | SFLASH_INTR_MASK;
				}else{
					sflash->chbkup = sflash->chbkup & (~SFLASH_INTR_MASK);
				}
				status = TRUE;
				break;

			case SFLASH_SET_BUSSEL:
				if( ((UINT32 *)data)[0] < SFLASH_CSEL_MAX ){
					sflash->bussel = ((UINT32 *)data)[0];
					status = TRUE;
				}else{
					status = FALSE;
				}
				break;
			case SFLASH_GET_BUSSEL:
				((UINT32 *)data)[0] = sflash->bussel;
				status = TRUE;
				break;

			case SFLASH_SET_UNSUSPEND:
				sflash->susmode = ((UINT32 *)data)[0];
				status = TRUE;
				break;

			default:
				status = FALSE;
				break;
		}

		return status;
	}

	device = (SFLASH_DEV_TYPE *) sflash->device;

	switch(cmd){
		case	SFLASH_GET_SIZE:
			if( sflash->jedec.sfdp != NULL ){
				SFLASH_SFDP_TYPE *sfdp;

				sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
				*((UINT32 *)data) = (sfdp->density + 1);
			}else{
				*((UINT32 *)data) = device->size ;
			}
			status = TRUE;
			break;

		case	SFLASH_GET_INFO:
			status = sflash_local_device_print_info(sflash, ((UINT32 *)data));
			break;

		case	SFLASH_SET_RESET:

			device->buslock(sflash, TRUE);

			sflash_interrupt_deactive(sflash);
			if( data == NULL ){
				status =  device->reset(sflash, 2); // SLR, Slow Reset
			}else{
				status =  device->reset(sflash, 1); // SLR, Fast Reset
			}
			sflash_interrupt_active(sflash);

			device->buslock(sflash, FALSE);

			break;

		case	SFLASH_CMD_VERIFY:
		case	SFLASH_SET_LOCK:
		case	SFLASH_SET_UNLOCK:
		case	SFLASH_CMD_ERASE:
			status =  sflash_local_device_sector_op(sflash
						, cmd
				   		, ((UINT32 *)data)[0]
				   		, ((UINT32 *)data)[1]);
			break;

		case	SFLASH_CMD_CHIPERASE:
			if(  device->erase != NULL ){

				device->buslock(sflash, TRUE);

				sflash_interrupt_deactive(sflash);
				device->erase(sflash, 0, 0xFFFFFFFF);
				sflash_interrupt_active(sflash);

				device->buslock(sflash, FALSE);

			}
			status = TRUE;
			break;

		case SFLASH_SET_PPBLOCK:
		case SFLASH_GET_PPBLOCK:
		case SFLASH_SET_VENDOR:
			status =  sflash_local_device_sector_vend(sflash
						, cmd, data);
			break;

		case SFLASH_CMD_POWERDOWN:
			status =  sflash_device_power_down(sflash
						, TRUE, data);
			break;
		case SFLASH_CMD_WAKEUP:
			status =  sflash_local_device_power_down(sflash, FALSE, data);
			break;

		case SFLASH_BUS_POLLMODE:
		case SFLASH_BUS_INTRMODE:
		case SFLASH_BUS_DMAMODE:
		case SFLASH_BUS_CONTROL:
		case SFLASH_TEST_CONTROL:
		case SFLASH_BUS_FORMAT:
		case SFLASH_BUS_DWIDTH:
		case SFLASH_SET_MAXBURST:
			status = sflash_local_set_bus_mode( sflash, cmd, data );
			break;

		case SFLASH_BUS_MAXSPEED:
			sflash->maxspeed = ((UINT32 *)data)[0];
			SFLASH_DBG_INFO("SFLASH: Max. %d Hz", sflash->maxspeed );
			status = TRUE;
			break;

		case SFLASH_SET_INFO:
			status = sflash_local_set_sfdp_info(sflash, data );
			break;

		case SFLASH_GET_MODE:
			status = sflash_local_get_mode(sflash, data);
			break;

		case SFLASH_SET_XIPMODE:
			sflash->xipmode = ((UINT32 *)data)[0];
			status = TRUE;
			break;
		case SFLASH_SET_BOOTMODE:
			if( ((UINT32 *)data)[0] == TRUE ){
				sflash->chbkup = sflash->chbkup | SFLASH_INTR_MASK;
			}else{
				sflash->chbkup = sflash->chbkup & (~SFLASH_INTR_MASK);
			}
			status = TRUE;
			break;

		case SFLASH_SET_BUSSEL:
			if( ((UINT32 *)data)[0] < SFLASH_CSEL_MAX ){
				sflash->bussel = ((UINT32 *)data)[0];
				status = TRUE;
			}else{
				status = FALSE;
			}
			break;
		case SFLASH_GET_BUSSEL:
			((UINT32 *)data)[0] = sflash->bussel;
			status = TRUE;
			break;

		case SFLASH_SET_UNSUSPEND:
			sflash->susmode = ((UINT32 *)data)[0];
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
int sflash_local_low_read(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	SFLASH_HANDLER_TYPE	*sflash;
	SFLASH_DEV_TYPE		*device;
	UINT8		*p_data8;

	/* Handler Checking */
	if(handler == NULL || p_dlen == 0) {
		return FALSE;
	}
	sflash = (SFLASH_HANDLER_TYPE *)handler;

	if( sflash->device == NULL ){
		SFLASH_DBG_INFO("SFLASH Driver not ready");
		return FALSE;
	}

	device = (SFLASH_DEV_TYPE *) sflash->device;

	// if p_data == NULL, fixed address mode ( only crc-check )
	p_data8 = (UINT8 *)p_data;

	device->buslock(sflash, TRUE);

	sflash_interrupt_deactive(sflash);
	if( device->read(sflash, addr, p_data8, p_dlen) == 0 ){
		p_dlen = 0; /* FALSE */
	}
	sflash_interrupt_active(sflash);

	device->buslock(sflash, FALSE);

	return p_dlen;
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
int sflash_local_low_write(HANDLE handler, UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	SFLASH_HANDLER_TYPE	*sflash;
	SFLASH_DEV_TYPE		*device;
	UINT32  page, bstlen, rmdlen;
	UINT8	*p_data8;
	int	status;

	/* Handler Checking */
	if(handler == NULL || p_dlen == 0) {
		return FALSE;
	}
	sflash = (SFLASH_HANDLER_TYPE *)handler;

	if( sflash->device == NULL ){
		SFLASH_DBG_INFO("SFLASH Driver not ready");
		return FALSE;
	}


	device = (SFLASH_DEV_TYPE *) sflash->device;

	page = sflash_find_sfdppage(sflash);

	if( page < 2 ){
		page = device->page;
	}

	bstlen = page - (addr % page);
	rmdlen = p_dlen;
	p_data8 = (UINT8 *)p_data;

	device->buslock(sflash, TRUE);

	do{
		if( bstlen > rmdlen ){
			bstlen = rmdlen;
		}

		//TEST: if( (sflash->susmode & 0x02) == 0 ){
		//TEST: 	sflash->psustime = device->suspend(sflash, addr, SUS_GET_PROGRAM, 0);
		//TEST: }else{
			sflash->psustime = 0;
		//TEST: }

		if( sflash->psustime == 0 ){
			sflash_interrupt_deactive(sflash);
			status = device->write(sflash, addr, p_data8, bstlen);
			sflash_interrupt_active(sflash);

			if( status == 0 ){
				p_dlen = 0; /* FALSE */
				break;
			}
		}else{
			sflash_interrupt_deactive(sflash);
			if( device->write(sflash, addr, p_data8, bstlen) == 0 ){
				p_dlen = 0; /* FALSE */
				sflash_interrupt_active(sflash);
				break;
			}
			device->suspend(sflash, addr, SUS_RUN_PROGRAM, 0);
			sflash_interrupt_active(sflash);

			// Wait Until sflash->psustime, iteration
			while(sflash->psustime > 0){
				sflash_interrupt_deactive(sflash);
				status = device->suspend(sflash, addr, RES_RUN_PROGRAM, SFLASH_UITME_100US(1));
				device->suspend(sflash, addr, SUS_RUN_PROGRAM, 0);
				sflash_interrupt_active(sflash);
				sflash->psustime = sflash->psustime - 1;
				if( status == TRUE ){
					break;
				}
			}
			sflash_interrupt_deactive(sflash);
			device->suspend(sflash, addr, RES_RUN_PROGRAM, SFLASH_UITME_100US(1));
			sflash_interrupt_active(sflash);
		}


		addr   += bstlen ;
		p_data8 =  &(p_data8[bstlen]);
		rmdlen -= bstlen ;
		bstlen = page ;

	}while(rmdlen > 0);

	device->buslock(sflash, FALSE);

	return p_dlen;
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
static int sflash_local_device_print_info(SFLASH_HANDLER_TYPE *sflash, UINT32 *mode)
{
	SFLASH_DEV_TYPE *device;
#ifdef	SFLASH_INFO_DETAIL
	SFLASH_WORK_TYPE *work;
	UINT32	num , i, addr, num_acc;
#endif	//SFLASH_INFO_DETAIL

	device = (SFLASH_DEV_TYPE *) sflash->device;

	if( mode != NULL && mode[0] != 0xFFFFFFFF ){
		mode[0] = (UINT32)(device->name);	// string
		mode[1] = sflash->jedecid;		// id
		mode[2] = (UINT32)(sflash->jedec.sfdp);	// sfdp data
		mode[3] = (UINT32)(sflash->jedec.len);	// sfdp len
#ifdef	SFLASH_INFO_DETAIL
#else	//SFLASH_INFO_DETAIL
		return TRUE;
#endif	//SFLASH_INFO_DETAIL
	}

#ifdef	SFLASH_INFO_DETAIL
	SFLASH_PRINTF("SFLASH device: %s\n", device->name);
	SFLASH_PRINTF("size/page: %d/%d\n"
				, device->size, device->page );

	SFLASH_PRINTF("SFLASH device: %s (%08x)\n"
			, device->name
			, sflash->jedecid );

	num_acc = 0;
	work = (SFLASH_WORK_TYPE *)(sflash->work);

	device->buslock(sflash, TRUE);

	/* SFDP or CFI */
	//	max_cfi = device->probe(sflash, (VOID *)(device), (work->id_cfi) );
	//	if( max_cfi > 0 ){
	//		sflash->max_cfi = max_cfi ;
	//	}

	if( sflash->jedec.sfdp != NULL ){
		SFLASH_PRINTF("SFLASH SFDP:\n");
		SFLASH_DUMP(0, sflash->jedec.sfdp, sflash->jedec.len );
		sflash_print_sfdp( sflash );
	}else
	if( sflash->jedec.cfi != NULL ){
		SFLASH_PRINTF("SFLASH CFI:\n");
		SFLASH_DUMP(0, sflash->jedec.cfi, sflash->jedec.len );
		sflash_print_cfi( sflash );
	}

	if( mode == NULL && device->verify != NULL ){
		for(i =0; i < work->num_secgrp; i++){
			for(num = 0; num < work->sec_grp[i].blknum; num++){
			        UINT32  lock;

				addr = work->sec_grp[i].offset
					+ (num * work->sec_grp[i].blksiz) ;
				lock = device->verify(sflash, addr, num_acc);
				SFLASH_PRINTF("SFLASH Block %08lx : %s\n"
					, addr, ((lock == TRUE)? "lock" : "unlock") );
				num_acc ++;
			}
		}
	}

	device->buslock(sflash, FALSE);
#endif	//SFLASH_INFO_DETAIL

	return TRUE;
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
static int sflash_local_device_sector_op(SFLASH_HANDLER_TYPE *sflash, UINT32 mode, UINT32 addr, UINT32 dsize)
{
	SFLASH_DEV_TYPE *device;
	SFLASH_WORK_TYPE *work;
	UINT32 i, i_sav, num, num_sav, num_acc;
	UINT32 saddr, eaddr, prev, next ;
	int status ;
	int	(*sflash_func)   (SFLASH_HANDLER_TYPE *sflash, UINT32 addr, UINT32 num);

	device = (SFLASH_DEV_TYPE *)(sflash->device);

	switch(mode){
		case SFLASH_SET_LOCK:
			sflash_func = device->lock ;
			break;
		case SFLASH_SET_UNLOCK:
			sflash_func = device->unlock ;
			break;
		case SFLASH_CMD_VERIFY:
			sflash_func = device->verify ;
			break;
		case SFLASH_CMD_ERASE:
			sflash_func = device->erase ;
			break;
		default:
			return FALSE;
	}

	if( sflash_func == NULL ){
		SFLASH_DBG_WARN("[%d] function not support", mode);
		return FALSE;
	}

	/* Find Start Sector */
	work = (SFLASH_WORK_TYPE *)(sflash->work);
	saddr = 0xFFFFFFFF;
	prev  = 0;
	num_acc = 0;

	for(i =0; i < work->num_secgrp; i++){
		for(num = 0; num < work->sec_grp[i].blknum; num++){

			next = work->sec_grp[i].offset
				+ ((num+1) * work->sec_grp[i].blksiz) ;

			//SFLASH_DBG_BASE("if( %x >= %x && %x < %x )"
			//	, addr, prev, addr, next );
			if( addr >= prev && addr < next  )
			{
				saddr = prev ;
				i_sav = i;
				num_sav = num ;
				i = work->num_secgrp ; /* break condition */
				break;
			}
			prev = next ;
			num_acc ++  ;
		}
	}

	if(saddr == 0xFFFFFFFF){
		SFLASH_DBG_WARN("[%d] sector not found, %lx - %p"
			, mode, addr, device);
		return FALSE;
	}

	/* Processing */

	status = TRUE;
	eaddr  = addr + dsize ;

	device->buslock(sflash, TRUE);

	for( i= i_sav; i < work->num_secgrp; i++){
		for( num = num_sav ; num < work->sec_grp[i].blknum; num++){

			if( (sflash_func == device->erase) && ((sflash->susmode & 0x01) == 0) ){
				sflash->esustime = device->suspend(sflash, addr, SUS_GET_ERASE, work->sec_grp[i].blksiz);
			}else{
				sflash->esustime = 0;
			}

			if( sflash->esustime == 0 ){
				sflash_interrupt_deactive(sflash);
				status = sflash_func(sflash, saddr, work->sec_grp[i].blksiz);
				sflash_interrupt_active(sflash);
			}else{
				sflash_interrupt_deactive(sflash);
				status = sflash_func(sflash, saddr, work->sec_grp[i].blksiz);
				device->suspend(sflash, addr, SUS_RUN_ERASE, 0);
				sflash_interrupt_active(sflash);

				// Wait Until sflash->esustime, iteration
				while(sflash->esustime > 0){
					sflash_interrupt_deactive(sflash);
					status = device->suspend(sflash, addr, RES_RUN_ERASE, SFLASH_UITME_100US(1));
					device->suspend(sflash, addr, SUS_RUN_ERASE, 0);
					sflash_interrupt_active(sflash);
					sflash->esustime = sflash->esustime - 1;

					if(status == TRUE){
						break;
					}
				}
				sflash_interrupt_deactive(sflash);
				device->suspend(sflash, addr, RES_RUN_ERASE, SFLASH_UITME_100US(1));
				sflash_interrupt_active(sflash);
			}

			if ( (status == FALSE) && (mode != SFLASH_CMD_VERIFY) ){
				SFLASH_DBG_WARN("[%d] sector error, %lx - %p"
					, mode, saddr, device);
			}

			saddr = work->sec_grp[i].offset
				+ ((num+1) * work->sec_grp[i].blksiz) ;

			if(saddr >= eaddr){
				SFLASH_DBG_BASE("completed %x>%x", saddr, eaddr);
				i = work->num_secgrp ; /* break condition */
				break;
			}
			num_acc ++ ;
		}
		num_sav = 0;	/* Clear */
	}

	device->buslock(sflash, FALSE);

	return status;
}

/******************************************************************************
 *   sflash_local_device_sector_vend( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int sflash_local_device_sector_vend(SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data)
{
	SFLASH_DEV_TYPE *device;
	int	status;
	UINT32  devcmd;
	int	(*sflash_func)   (SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data);


	device = (SFLASH_DEV_TYPE *)(sflash->device);
	sflash_func = device->vendor;

	if( sflash_func == NULL ){
		SFLASH_DBG_WARN("vendor function not support");
		return FALSE;
	}

	switch(cmd){
		case SFLASH_SET_PPBLOCK:
			devcmd = SFLASH_DEV_SET_PPBL;
			break;
		case SFLASH_GET_PPBLOCK:
			devcmd = SFLASH_DEV_GET_PPBL;
			break;

		case SFLASH_SET_VENDOR:
			devcmd = SFLASH_DEV_VENDOR;
			break;
		default:
			return FALSE;
	}

	device->buslock(sflash, TRUE);

	sflash_interrupt_deactive(sflash);
	status = sflash_func(sflash, devcmd, data);
	sflash_interrupt_active(sflash);

	if( status == FALSE ){
		SFLASH_DBG_WARN("%s error - %p", "vendor", device);
	}

	device->buslock(sflash, FALSE);


	return status;
}

/******************************************************************************
 *   sflash_local_device_power_down( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int sflash_local_device_power_down(SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data)
{
	SFLASH_DEV_TYPE *device;
	int	status;
	int	(*pwrdown) (SFLASH_HANDLER_TYPE *sflash, UINT32 mode, UINT32 *data);


	device = (SFLASH_DEV_TYPE *)(sflash->device);
	pwrdown = device->pwrdown;

	if( pwrdown == NULL ){
		SFLASH_DBG_WARN("powerdown function not support");
		return FALSE;
	}

	device->buslock(sflash, TRUE);

	sflash_interrupt_deactive(sflash);
	status = pwrdown(sflash, cmd, data);
	sflash_interrupt_active(sflash);

	if( status == FALSE ){
		SFLASH_DBG_WARN("%s error - %p", "pwrdown", device);
	}

	device->buslock(sflash, FALSE);

	return status;
}

/******************************************************************************
 *   sflash_local_set_bus_mode( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
static int sflash_local_set_bus_mode(SFLASH_HANDLER_TYPE *sflash, UINT32 cmd, VOID *data)
{
	SFLASH_BUS_TYPE		*bus;
	int			status;

	bus = (SFLASH_BUS_TYPE *)sflash->bus;

	if ( sflash->instance == 0 ){
		status = TRUE;

		/* sflash_xfc_ioctl */
		switch(cmd){
			case SFLASH_BUS_POLLMODE:
				bus->ioctl( sflash , SFLASH_BUS_SET_POLLMODE, NULL );
				break;
			case SFLASH_BUS_INTRMODE:
				bus->ioctl( sflash , SFLASH_BUS_SET_INTRMODE, NULL );
				break;
			case SFLASH_BUS_DMAMODE:
				bus->ioctl( sflash , SFLASH_BUS_SET_DMAMODE, NULL );
				break;
			case SFLASH_TEST_CONTROL:
				bus->ioctl( sflash , SFLASH_BUS_SET_BUSCONTROL, data );
				break;
			case SFLASH_BUS_FORMAT:
				bus->ioctl( sflash , SFLASH_BUS_SET_FORMAT, data );
				break;
			case SFLASH_BUS_DWIDTH:
				sflash->dwidth = ((UINT32 *)data)[0] ;
				bus->ioctl( sflash , SFLASH_BUS_SET_DATA_WIDTH, data );			/* sflash_xfc_ioctl */
				break;
			case SFLASH_SET_MAXBURST:
				bus->ioctl( sflash , SFLASH_BUS_SET_MAXRDSIZE, data );
				break;
			case SFLASH_BUS_CONTROL:
				{
					SFLASH_DEV_TYPE *device;

					device = (SFLASH_DEV_TYPE *)(sflash->device);

					if( device != NULL && device->busmode != NULL ){
						device->buslock(sflash, TRUE);
						sflash_interrupt_deactive(sflash);
						sflash->accmode = device->busmode( sflash, *((UINT32 *)data));
						sflash_interrupt_active(sflash);
						device->buslock(sflash, FALSE);
					}else{
						sflash->accmode = SFLASH_BUS_3BADDR|SFLASH_BUS_111 ;
					}
				}
				break;
			default:
				status = FALSE;
				break;

		}
	}
	else
	{
		status = TRUE;

		switch(cmd){
			case SFLASH_BUS_CONTROL:
				{
					SFLASH_DEV_TYPE *device;

					device = (SFLASH_DEV_TYPE *)(sflash->device);

					if( device != NULL && device->busmode != NULL ){
						device->buslock(sflash, TRUE);
						sflash_interrupt_deactive(sflash);
						sflash->accmode = device->busmode( sflash, *((UINT32 *)data));
						sflash_interrupt_active(sflash);
						device->buslock(sflash, FALSE);
					}else{
						sflash->accmode = SFLASH_BUS_3BADDR|SFLASH_BUS_111 ;
					}
				}
				break;
			default:
				status = FALSE;
				break;
		}
	}

	return status;
}

/******************************************************************************
 *   sflash_local_set_sfdp_info( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifdef	SUPPORT_SFLASH_UNKNOWN

ATTRIBUTE_RAM_FUNC
static void sflash_set_sectorgroup(SFLASH_WORK_TYPE *work, char *title, UINT32 i)
{
	DA16X_UNUSED_ARG(title);

	if( work->num_secgrp < (i+1) ){
		work->sec_grp[i+1].offset
			= work->sec_grp[i].offset
			+ ( work->sec_grp[i].blksiz * work->sec_grp[i].blknum ) ;
	}
#ifdef	SFLASH_INFO_DETAIL
	SFLASH_PRINTF("%s : [%08x] %d \n"
			, title
			, work->sec_grp[i].offset
			, work->sec_grp[i].blknum );
#endif	//SFLASH_INFO_DETAIL
}

#endif	//SUPPORT_SFLASH_UNKNOWN

ATTRIBUTE_RAM_FUNC
static int sflash_local_set_sfdp_info(SFLASH_HANDLER_TYPE *sflash, VOID *data)
{
	if( data == NULL ){
		return FALSE;
	}

	SFLASH_DBG_NONE("%s: %08x, %d - %p", __func__
			, ((UINT32 *)data)[0]
			, ((UINT32 *)data)[1]
			, (VOID *)(((UINT32 *)data)[2]) );

	// Device ID, MPW3 Issue!!
	// Do NOT update jedecid !!
	/* sflash->jedecid = ((UINT32 *)data)[0] ;*/

	sflash->max_cfi = ((UINT32 *)data)[1] ;		// sfdp len
	sflash->jedec.len = ((UINT32 *)data)[1] ;		// sfdp len
	sflash->jedec.sfdp = (VOID *)((UINT32 *)data)[2];	// pseudo sfdp

	if( ((UINT32 *)data)[3] != (UINT32)NULL ){
		sflash->device = (VOID *)(((UINT32 *)data)[3]);	// device
	}

	if( ((UINT32 *)data)[4] != (UINT32)NULL ){
		sflash->jedec.extra = (VOID *)(((UINT32 *)data)[4]);	// extra

#ifdef	SUPPORT_SFLASH_UNKNOWN
		{
			UINT32		i;
			SFLASH_EXTRA_TYPE *extra;
			SFLASH_WORK_TYPE  *work;

			extra = (SFLASH_EXTRA_TYPE *)(sflash->jedec.extra);
			work = (SFLASH_WORK_TYPE *)(sflash->work);

			i= 0;
			work->sec_grp[0].offset = 0;
			work->num_secgrp = 0;

			if( extra->n_top4ksec != 0 ){
				work->sec_grp[i].blknum = extra->n_top4ksec ;
				work->sec_grp[i].blksiz = 4*1024 ;

				sflash_set_sectorgroup( (SFLASH_WORK_TYPE *)(sflash->work) , "top4ksec", i);
				i++;
			}

			if( extra->n_32ksec != 0 ){
				work->sec_grp[i].blknum = extra->n_32ksec ;
				work->sec_grp[i].blksiz = 32*1024 ;

				sflash_set_sectorgroup( (SFLASH_WORK_TYPE *)(sflash->work) , "32ksec", i);
				i++;
			}

			if( extra->n_64ksec != 0 ){
				work->sec_grp[i].blknum = extra->n_64ksec ;
				work->sec_grp[i].blksiz = 64*1024 ;

				sflash_set_sectorgroup( (SFLASH_WORK_TYPE *)(sflash->work) , "64ksec", i);
				i++;
			}

			if( (extra->total_sec
				- extra->n_top4ksec
				- extra->n_bot4ksec
				- extra->n_32ksec
				- extra->n_64ksec ) != 0 ){

				work->sec_grp[i].blknum
					= (extra->total_sec
						- extra->n_top4ksec
					  	- extra->n_bot4ksec
					  	- extra->n_32ksec
					  	- extra->n_64ksec ) ;
				work->sec_grp[i].blksiz = 128*1024 ;

				sflash_set_sectorgroup( (SFLASH_WORK_TYPE *)(sflash->work) , "128ksec", i);
				i++;
			}

			if( extra->n_bot4ksec != 0 ){
				work->sec_grp[i].blknum = extra->n_bot4ksec ;
				work->sec_grp[i].blksiz = 4*1024 ;

				sflash_set_sectorgroup( (SFLASH_WORK_TYPE *)(sflash->work) , "bot4ksec", i);
				i++;
			}

			work->num_secgrp = i;
		}
#endif //SUPPORT_SFLASH_UNKNOWN
	}

	if( ((UINT32 *)data)[5] != (UINT32)NULL ){

		sflash->jedec.cmdlist = (VOID *)(((UINT32 *)data)[5]);	// cmdlist
	}

	if( ((UINT32 *)data)[6] != (UINT32)NULL ){
		SFLASH_BUS_TYPE *bus;

		bus = (SFLASH_BUS_TYPE *)sflash->bus;

		sflash->jedec.delay = (VOID *)(((UINT32 *)data)[6]);	// delay
		bus->ioctl( sflash, SFLASH_BUS_SET_DELAYSEL, (VOID *)(sflash->jedec.delay) );
	}

	return TRUE;
}

/******************************************************************************
 *   sflash_local_get_mode( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	CHECK_SFLASHFUNC(x)		(((x) == 0 || (x) == 0xFF) ? 0 : 1)

ATTRIBUTE_RAM_FUNC
static int sflash_local_get_mode(SFLASH_HANDLER_TYPE *sflash, VOID *data)
{
	if( data == NULL ){
		return FALSE;
	}

	if( sflash->jedec.extra != NULL ){
		SFLASH_EXTRA_TYPE *extra;

		extra = (SFLASH_EXTRA_TYPE *)(sflash->jedec.extra);
		((UINT32 *)data)[0] = (extra->max_speed)*MHz ;
	}else{
		((UINT32 *)data)[0] = sflash->maxspeed ; // unknown
	}

	if( sflash->jedec.cmdlist != NULL ){
		SFLASH_CMDLIST_TYPE *cmdlist;
		UINT32	accessmode;

		cmdlist = (SFLASH_CMDLIST_TYPE *)(sflash->jedec.cmdlist);
		accessmode = 0;

		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_PP]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_QPP]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_QPPA]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_4PP]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_4QPP]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_4QPPA]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_P4E]);
		accessmode = (accessmode << 4) | CHECK_SFLASHFUNC(cmdlist[SFLASH_CMD_4P4E]);

		((UINT32 *)data)[1] = accessmode ;
	}else{
		((UINT32 *)data)[1] = 0 ; // unknown
	}

	if( sflash->jedec.delay != NULL ){
		UINT8 *delay;

		delay = (UINT8 *)(sflash->jedec.delay);
		((UINT32 *)data)[2] = (UINT32)(delay[0]) ;
	}else{
		((UINT32 *)data)[2] = 0 ; // unknown
	}

	return TRUE;
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
static void	sflash_local_interrupt_deactive(SFLASH_HANDLER_TYPE *sflash)
{
	if( (sflash->chbkup & SFLASH_INTR_MASK) == SFLASH_INTR_MASK ){
		return;
	}

	sflash->chbkup = 0;

#ifdef SFLASH_CONTROL_WDOG
	{
		HANDLE wdog;
		UINT32 wdogstatus;

		wdog = WDOG_CREATE(WDOG_UNIT_0);
		WDOG_INIT(wdog);
		WDOG_IOCTL(wdog, WDOG_GET_STATUS, &wdogstatus );
		WDOG_IOCTL(wdog, WDOG_SET_DISABLE, NULL );
		WDOG_CLOSE(wdog);

		sflash->chbkup = sflash->chbkup | (wdogstatus<<24);
	}
#endif	//SFLASH_CONTROL_WDOG
	{
		UINT32 bkupintr;

		CHECK_INTERRUPTS(bkupintr);
		PREVENT_INTERRUPTS(1);

		sflash->chbkup = sflash->chbkup | (bkupintr<<16);
	}
	{
		UINT32 bkupcache;

		bkupcache = da16x_cache_status();

		if( (bkupcache == TRUE) ){
			da16x_cache_disable();
		}

		sflash->chbkup = sflash->chbkup | (bkupcache<<8);
	}
	{
		UINT32 curtype;

		curtype = DA16XFPGA_GET_NORTYPE();

		if( curtype == DA16X_SFLASHTYPE_CACHE ){
			DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_XIP);
		}

		sflash->chbkup = sflash->chbkup | (curtype<<0);
	}

	return;
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
static void	sflash_local_interrupt_active(SFLASH_HANDLER_TYPE *sflash)
{
	UINT32 bkupintr, bkupcache, wdogstatus, curtype;

	sflash->slptim = 0; // Reset
	((SFLASH_BUS_TYPE *)sflash->bus)->ioctl( sflash, SFLASH_BUS_PRESET_USLEEP, &(sflash->slptim) );

	if( (sflash->chbkup & SFLASH_INTR_MASK) == SFLASH_INTR_MASK ){
		return;
	}

	curtype  = (sflash->chbkup & 0x0ff);
	bkupcache  = (sflash->chbkup >> 8) & 0x0ff;
	bkupintr   = (sflash->chbkup >> 16) & 0x0ff;
	wdogstatus = (sflash->chbkup >> 24) & 0x07f;

	if( curtype == DA16X_SFLASHTYPE_CACHE ){
		DA16XFPGA_SET_NORTYPE(DA16X_SFLASHTYPE_CACHE);
	}

	if( (bkupcache == TRUE) ){
		da16x_cache_enable(FALSE);
	}
	
	PREVENT_INTERRUPTS(bkupintr);
#ifdef	SFLASH_CONTROL_WDOG
	if( wdogstatus != FALSE ){
		HANDLE wdog;
		wdog = WDOG_CREATE(WDOG_UNIT_0);
		WDOG_INIT(wdog);
		WDOG_IOCTL(wdog, WDOG_SET_ENABLE, NULL );
		WDOG_CLOSE(wdog);
	}
#endif	//SFLASH_CONTROL_WDOG
	return;
}

#endif	//SUPPORT_SFLASH_ROMLIB


/******************************************************************************
 *  SFLASH_PATCH ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int 	SFLASH_PATCH (HANDLE handler, UINT32 rtosmode)
{
	SFLASH_HANDLER_TYPE	*sflash;
	UINT32 ioctldata[8];

	/* Handler Checking */
	if(handler == NULL) 
	{
		return FALSE;
	}

	sflash = (SFLASH_HANDLER_TYPE *)handler;	


#ifdef SUPPORT_STD_AT25SF321B
	// Adesto AT25SF321B : changed from non-suspend to suspend.
	if( sflash->jedecid == 0x1f870115 )
	{
		if((sflash->jedec.sfdp) != NULL && (sflash->jedec.len >= 64) )
		{

			SFLASH_SFDP_TYPE *sfdp;
			
			sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;			

			sfdp->srsupp = 0; // force the erase-suspend mode
			sfdp->unuse120 = 0; // ERTSI scaler changed from x64 to x512 

			SFLASH_DBG_INFO("%s:sfdp(%p)->srsupp & unuse120 = 0", 
							__func__, sfdp);

			if( da16x_sflash_setup_parameter(ioctldata) == TRUE )
			{
				//parameters[1] = 64;
				//parameters[2] = (UINT32)&(sfdp_table->chunk.sfdptab.sfdp[0]);
				if((ioctldata[2] != 0) && (ioctldata[1] >= 64) )
				{

					sfdp = (SFLASH_SFDP_TYPE *)(ioctldata[2]);	

					sfdp->srsupp = 0; // force the erase-suspend mode
					sfdp->unuse120 = 0; // ERTSI scaler changed from x64 to x512 

					SFLASH_DBG_INFO("%s:sfdp(%p)->srsupp & unuse120 = 0",
								__func__, sfdp);					
				}
			}
			else
			{
				SFLASH_DBG_INFO("%s:setup_parameter() empty", __func__);
			}
		}
		else
		{
			SFLASH_DBG_INFO("%s:jedec.sfdp empty", __func__);
		}
	}
	else
	{
		SFLASH_DBG_INFO("%s:jedecid %08x", __func__, sflash->jedecid);
	}
#endif //SUPPORT_STD_AT25SF321B	

#ifdef SUPPORT_STD_MX25R1635F_ULTRALOWPOWER
	SFLASH_PATCH_ULPower(handler, rtosmode);
#endif //SUPPORT_STD_MX25R1635F_ULTRALOWPOWER

#ifdef SUPPORT_STD_W25Q32JW_CMP
	SFLASH_PATCH_W25Q32JW(handler, rtosmode);
#endif	//SUPPORT_STD_W25Q32JW_CMP

	return TRUE;
}

/******************************************************************************
 *  SFLASH_PATCH_ULPower ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

ATTRIBUTE_RAM_FUNC
int 	SFLASH_PATCH_ULPower (HANDLE handler, UINT32 rtosmode)
{
	SFLASH_HANDLER_TYPE	*sflash;

	/* Handler Checking */
	if(handler == NULL) 
	{
		return FALSE;
	}

	SFLASH_DBG_INFO("%s: start", __func__);	
	

	sflash = (SFLASH_HANDLER_TYPE *)handler;	

#if	defined(SUPPORT_STD_MX25R1635F_ULTRALOWPOWER)

	if( sflash->jedecid == 0xc2281515 )
	{
		SFLASH_DEV_TYPE *sflash_dev;
		UINT32		phase, flagchk, bkbusmode, busmode;
		UINT8		sdata[4], cdata[4];

		sflash_dev = (SFLASH_DEV_TYPE *)(sflash->device);

		sflash_dev->buslock(sflash, TRUE);

		sflash_interrupt_deactive(sflash);

		switch(SFLASH_BUS_GRP(sflash->accmode))
		{
			case	SFLASH_BUS_444:
			case	SFLASH_BUS_044:
			case	SFLASH_BUS_004:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_444);
				break;
			default:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_111);
				break;
		}

		bkbusmode = sflash->accmode;
		sflash->accmode = busmode;	// change busmode

		sflash_unknown_setspeed(sflash, 0); // very slow
		
		sflash_unknown_set_wren(sflash);

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		phase = sflash_jedec_bus_update(sflash, phase);			
		
		sflash_jedec_read( 0x15, sflash, phase, cdata, 2);

		sflash_jedec_read( 0x05, sflash, phase, sdata, 1);

		// waiting until WEL==1
		sflash_jedec_atomic_check_iter(0x05, sflash
					, phase, SR1_WEL, 0 , (SFLASH_MAX_LOOP<<1));
		
		flagchk = 0;
		
		if( (sdata[0] & (1<<6)) == 0 )	// QE
		{
			sdata[0] = sdata[0] |  (1<<6) ;
			SFLASH_DBG_INFO("QE[%02x]", sdata[0]);
			flagchk = 1;
		}

		if( (cdata[1] & (1<<1)) == 0 )	// L/H Switch
		{
			cdata[1] = cdata[1] |  (1<<1) ;
			SFLASH_DBG_INFO("HPerf[%02x.%02x]", cdata[0], cdata[1]);
			flagchk = 1;
		}

		sdata[1] = cdata[0];
		sdata[2] = cdata[1]; 
		
		if( flagchk == 1 ){ 
			// Update Status & Config Registers 
			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			phase = sflash_jedec_bus_update(sflash, phase);			
			sflash_jedec_write( 0x01, sflash, phase, sdata, 3);

			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			phase = sflash_jedec_bus_update(sflash, phase);			
			sflash_jedec_atomic_check_iter(0x05, sflash
						, phase, SR1_WIP, SR1_WIP , (SFLASH_MAX_LOOP<<1));
		}

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		phase = sflash_jedec_bus_update(sflash, phase);			
		sflash_jedec_read( 0x05, sflash, phase, sdata, 1);

		sflash_jedec_read( 0x15, sflash, phase, cdata, 2);


		SFLASH_DBG_INFO("S&C[%02x.%02x.%02x]", sdata[0], cdata[0], cdata[1]);


		sflash_unknown_set_wrdi(sflash);

		if((sflash->jedec.sfdp) != NULL && (sflash->jedec.len >= 64) )
		{

			SFLASH_SFDP_TYPE *sfdp;
			SFLASH_EXTRA_TYPE *extra;
            UINT32 ioctldata[8];
			
			sfdp = (SFLASH_SFDP_TYPE *)sflash->jedec.sfdp;
			extra = (SFLASH_EXTRA_TYPE *)sflash->jedec.extra;

			if( rtosmode == FALSE ){
				// UEBoot
				sfdp->frd122 = 1;		// Dual 
				sfdp->frd112 = 1;		// Dual 
				sfdp->frd144 = 1;		// Quad 
				sfdp->frd114 = 1;		// Quad 

				extra->fsr_speed = 30;
				extra->ddr_speed = 80;
				extra->qdr_speed = 80;	 // 25MHz 
			}else{
				sfdp->frd122 = 0;		// Dual 
				sfdp->frd112 = 0;		// Dual 
				sfdp->frd144 = 0;		// Quad disable
				sfdp->frd114 = 0;		// Quad disable

				extra->fsr_speed = 30;
				extra->ddr_speed =  5;
				extra->qdr_speed =  5;	 // 25MHz
			}

			if( da16x_sflash_setup_parameter(ioctldata) == TRUE )
			{
				if((ioctldata[2] != 0) && (ioctldata[1] >= 64) )
				{

					sfdp = (SFLASH_SFDP_TYPE *)(ioctldata[2]);	
					extra = (SFLASH_EXTRA_TYPE *)(ioctldata[4]);


					if( rtosmode == FALSE ){
						// UEBoot
						sfdp->frd122 = 1;		// Dual 
						sfdp->frd112 = 1;		// Dual 
						sfdp->frd144 = 1;		// Quad 
						sfdp->frd114 = 1;		// Quad 

						extra->fsr_speed = 30;
						extra->ddr_speed = 80;
						extra->qdr_speed = 80;	 // 25MHz
						
						SFLASH_DBG_INFO(
									"Change from ULPwr to HPerf. "
									"frd (%d,%d,%d,%d) speed (%d,%d,%d)"
									, sfdp->frd122, sfdp->frd112
									, sfdp->frd144, sfdp->frd114
									, extra->fsr_speed
									, extra->ddr_speed
									, extra->qdr_speed);

					}else{
						// RTOS
						sfdp->frd122 = 0;		// Dual 
						sfdp->frd112 = 0;		// Dual 
						sfdp->frd144 = 0;		// Quad disable
						sfdp->frd114 = 0;		// Quad disable

						extra->fsr_speed = 30;
						extra->ddr_speed =  5;
						extra->qdr_speed =  5;

						SFLASH_DBG_INFO(
									"Change from HPerf to ULPwr. "
									"frd (%d,%d,%d,%d) speed (%d,%d,%d)"
									, sfdp->frd122, sfdp->frd112
									, sfdp->frd144, sfdp->frd114
									, extra->fsr_speed
									, extra->ddr_speed
									, extra->qdr_speed);
					}
						
				}
			}
		}


		sflash->accmode = bkbusmode;	// restore busmode	

		sflash_interrupt_active(sflash);
		
		sflash_dev->buslock(sflash, FALSE);

	}
#endif //defined(SUPPORT_STD_MX25R1635F_ULTRALOWPOWER)

	return TRUE;
}

/******************************************************************************
 *  SFLASH_PATCH_W25Q32JW ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

DATA_ALIGN(4) static char *printflagformat = "CMP %d, SRL %d, QE %d\r\n";
DATA_ALIGN(4) static char *printflagformat_rtos = "[RTOS] CMP %d, SRL %d, QE %d\r\n";
DATA_ALIGN(4) static char *printwrsr2format   = "W25Q32JW.WrSR2(0x31):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr1format   = "W25Q32JW.RdSR1(0x05):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr2format   = "W25Q32JW.RdSR2(0x35):%02x\r\n";
DATA_ALIGN(4) static char *printrdsr3format   = "W25Q32JW.RdSR3(0x15):%02x\r\n";


ATTRIBUTE_RAM_FUNC
int 	SFLASH_PATCH_W25Q32JW (HANDLE handler, UINT32 rtosmode)
{
	SFLASH_HANDLER_TYPE	*sflash;

	/* Handler Checking */
	if(handler == NULL) 
	{
		return FALSE;
	}

	SFLASH_DBG_INFO("%s: start", __func__);	
	

	sflash = (SFLASH_HANDLER_TYPE *)handler;	

#if	defined(SUPPORT_STD_W25Q32JW_CMP)

	if( sflash->jedecid == 0xef601615 )
	{
		SFLASH_DEV_TYPE *sflash_dev;
		UINT32		phase, bkbusmode, busmode;
		UINT8		sdata[4], cdata[4];

		sflash_dev = (SFLASH_DEV_TYPE *)(sflash->device);

		sflash_dev->buslock(sflash, TRUE);

		sflash_interrupt_deactive(sflash);

		switch(SFLASH_BUS_GRP(sflash->accmode))
		{
			case	SFLASH_BUS_444:
			case	SFLASH_BUS_044:
			case	SFLASH_BUS_004:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_444);
				break;
			default:
				busmode = DARWMDCADD_NOBUS(sflash->accmode)
								|DARWMDCADD_BUS(SFLASH_BUS_111);
				break;
		}

		bkbusmode = sflash->accmode;
		sflash->accmode = busmode;	// change busmode

		sflash_unknown_setspeed(sflash, 0); // very slow
		
		sflash_unknown_set_wren(sflash);

		phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
		phase = sflash_jedec_bus_update(sflash, phase);			

		sflash_jedec_read( 0x05, sflash, phase, cdata, 1);
		SFLASH_DBG_INFO(printrdsr1format,cdata[0]);

		sflash_jedec_read( 0x15, sflash, phase, cdata, 1);
		SFLASH_DBG_INFO(printrdsr3format,cdata[0]);

		sflash_jedec_read( 0x35, sflash, phase, cdata, 1);
		SFLASH_DBG_INFO(printrdsr2format,cdata[0]);

		//TEST trick: cdata[0] = cdata[0] | 0x40; /// Do NOT modify it !!!

		if( (cdata[0] & 0x41) != 0 ){
			// 0xEF601615 --> CMP, SRL := 0 & QE := 1
			if( rtosmode == TRUE ){
				SFLASH_DBG_JTAG(printflagformat_rtos
						, ((cdata[0] & 0x40) != 0)
						, ((cdata[0] & 0x01) != 0)
						, ((cdata[0] & 0x02) != 0)
						);
			}else{
				SFLASH_DBG_JTAG(printflagformat
						, ((cdata[0] & 0x40) != 0)
						, ((cdata[0] & 0x01) != 0)
						, ((cdata[0] & 0x02) != 0)
						);
			}

			sdata[0] = (cdata[0] & (~0x41)) |  (1<<1) ;

			phase = DARWMDCADD(0, 0,1,3, 0,1, 0,0, 0,0,0,0);
			sflash_jedec_write( 0x31, sflash, phase, sdata, 1);	

			SFLASH_DBG_JTAG(printwrsr2format, sdata[0]);

			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			sflash_jedec_atomic_check(0x35, sflash
						, phase, (1<<1), 0x00 );


			phase = DARWMDCADD(0, 0,1,3, 1,0, 0,0, 0,0,0,0);
			phase = sflash_jedec_bus_update(sflash, phase);			

			sflash_jedec_read( 0x05, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr1format,cdata[0]);

			sflash_jedec_read( 0x15, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr3format,cdata[0]);

			sflash_jedec_read( 0x35, sflash, phase, cdata, 1);
			SFLASH_DBG_JTAG(printrdsr2format,cdata[0]);	

			if( rtosmode == TRUE ){
				SFLASH_DBG_JTAG(printflagformat_rtos
						, ((cdata[0] & 0x40) != 0)
						, ((cdata[0] & 0x01) != 0)
						, ((cdata[0] & 0x02) != 0)
						);
			}else{
				SFLASH_DBG_JTAG(printflagformat
						, ((cdata[0] & 0x40) != 0)
						, ((cdata[0] & 0x01) != 0)
						, ((cdata[0] & 0x02) != 0)
						);
			}			
		}


		sflash_unknown_set_wrdi(sflash);

		sflash_unknown_reset(sflash, 2); // slow reset

		sflash->accmode = bkbusmode;	// restore busmode	

		sflash_interrupt_active(sflash);
		
		sflash_dev->buslock(sflash, FALSE);

	}
#endif //defined(SUPPORT_STD_W25Q32JW_CMP)

	return TRUE;
}


#endif /*SUPPORT_SFLASH_DEVICE*/
