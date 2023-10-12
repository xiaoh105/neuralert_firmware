/**
 ****************************************************************************************
 *
 * @file ramlibmanager.c
 *
 * @brief RAM-Library Manager
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

#include "sdk_type.h"

#include <stdio.h>
#include <stdlib.h>

#include "da16x_system.h"
#include "da16x_oops.h"

#include "hal.h"
#include "sal.h"
#include "rtc.h"
#include "driver.h"
#include "library.h"
#include "ramlibmanager.h"
#include "environ.h"
#include "ramsymbols.h"
#include "da16x_system.h"
//#include "rfcalirq.h"
#include "da16x_secureboot.h"
#ifdef BUILD_OPT_DA16200_PTIM
#include "timro_ramlibmanager.h"
#endif

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/
#ifdef BUILD_OPT_DA16200_PTIM
#define TIM_RA_FROM_MASKROM
#endif

static UINT32	load_ramlib_image(UINT32 bootbase, UINT32 *epbase);
#ifndef BUILD_OPT_DA16200_MAC
static	int	ramlib_printf(char *fmt, ...);
#endif //BUILD_OPT_DA16200_MAC

#ifdef BUILD_OPT_DA16200_MAC
extern int dbg_test_print(char *fmt, ...);
#endif

#define	SUPPORT_FASTMEMCPY
#ifdef	SUPPORT_FASTMEMCPY
#define		RAMMAN_MEMCPY(...)	da16x_hwcopycrc32(sizeof(UINT32), __VA_ARGS__, (~0) );
#else	//SUPPORT_FASTMEMCPY
#define		RAMMAN_MEMCPY(...)	DRIVER_MEMCPY( __VA_ARGS__ )
#endif	//SUPPORT_FASTMEMCPY


/******************************************************************************
 *  ramlib_manager_check( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	ramlib_manager_check(void)
{
	volatile RAMLIB_SYMBOL_TYPE *ramlib;

	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;
	// 1. check RAM-library offset if it really exist.
	if( ((ramlib->magic & DA16X_RAMSYM_MAGIC_MASK) != DA16X_RAMSYM_MAGIC_TAG)
	       || (ramlib->checkramlib(ramlib->build) != TRUE) ){

	       return FALSE;
	}
	return TRUE;
}

/******************************************************************************
 *  ramlib_manager_init( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	ramlib_manager_init(UINT32 skip, UINT32 loadaddr)
{
#ifdef TIM_RA_FROM_MASKROM
	timro_ramlib_manager_init(skip, loadaddr);
#else
	UINT32	/*memory, offset,*/ ssize, entrypoint;

	// 1. check RAM-library offset if it really exist.
	if ( skip != TRUE ) {

		// 2. load RAM-library from SFLASH/OTP.
		if( loadaddr == 0xFFFFFFFF ){
			return FALSE;
		}

		ssize = load_ramlib_image(loadaddr, &entrypoint);

		if( ssize == 0 ){
			return FALSE;
		}

		// 3. check to reload
		if( (BOOT_MEM_GET(loadaddr) != BOOT_RETMEM)
		    && (BOOT_MEM_GET(entrypoint) == BOOT_RETMEM) ){
		    	// Re-loading
			loadaddr = entrypoint;
			ssize = load_ramlib_image(loadaddr, &entrypoint);
		}

		// 4. set up RAMLib fields of RETMEM_BOOTMODE_BASE.

		da16x_boot_set_lock(FALSE);

		da16x_boot_set_offset(BOOT_IDX_RLIBLA, loadaddr );
		da16x_boot_set_offset(BOOT_IDX_RLIBAP, entrypoint ); /* real address */
		da16x_boot_set_offset(BOOT_IDX_RLIBSZ, ssize );

		da16x_boot_set_lock(TRUE);
	}
#endif

	return TRUE;
}

/******************************************************************************
 *  ramlib_manager_bind( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	ramlib_manager_bind(UINT32 logo)
{
	DA16X_UNUSED_ARG(logo);

	RAMLIB_SYMBOL_TYPE *ramlib;
	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;
	// 5. bind the import symbols into a os-dependant primitives.

	ramlib->htoi  = &htoi ;
	ramlib->atoi  = &ctoi ;
#ifdef BUILD_OPT_DA16200_MAC
	ramlib->printf  = &dbg_test_print ;
#else
	ramlib->printf  = &ramlib_printf ;
#endif

	//ramlib->random  = &da16x_random ;
	ramlib->random  = (UINT32 (*)(void))&da16x_random ;

	/* rtc manater bind */
	ramlib->MCNT_CREATE = &MCNT_CREATE;
	ramlib->MCNT_START = &MCNT_START;
	ramlib->MCNT_STOP = &MCNT_STOP;
	ramlib->MCNT_CLOSE = &MCNT_CLOSE;
	ramlib->RTC_IOCTL = &RTC_IOCTL;
	ramlib->RTC_INTO_POWER_DOWN = &RTC_INTO_POWER_DOWN;
	ramlib->RTC_GET_WAKEUP_SOURCE =&RTC_GET_WAKEUP_SOURCE;
	ramlib->RTC_READY_POWER_DOWN = &RTC_READY_POWER_DOWN;
	ramlib->rfcal_irq_init = NULL;

	ramlib->init_ramlib();
	ramlib->checkramlib(ramlib->build); // Re-check
}

/******************************************************************************
 *  ramlib_manager_bind( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

char *ramlib_get_logo(void)
{
	RAMLIB_SYMBOL_TYPE *ramlib;

	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;

	if((ramlib->magic & DA16X_RAMSYM_MAGIC_MASK) != DA16X_RAMSYM_MAGIC_TAG){
		return NULL ;
	}
	return ramlib->logo ;
}

/******************************************************************************
 *  load_ramlib_image( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32	load_ramlib_image(UINT32 bootbase, UINT32 *epbase)
{
	HANDLE  handler;
	UINT32	status;
	UINT32	rlib_la;

	status = 0;

	switch(BOOT_MEM_GET(bootbase)){
#ifdef	BUILD_OPT_DA16200_FPGA
		case BOOT_NOR: /* NOR */
			da16x_environ_lock(TRUE);
			//=================================================
			// Load Flash RamLib
			//=================================================
			rlib_la = 0;
			status = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_RUN)
				, (DA16X_NOR_BASE|BOOT_OFFSET_GET(bootbase))
				, &rlib_la, epbase );

			da16x_environ_lock(FALSE);
			break;
#endif	//BUILD_OPT_DA16200_FPGA
		case BOOT_RETMEM: /* Retained Memory */
			// DMA Copy
			handler = retm_image_open(0, (DA16X_RETMEM_BASE|BOOT_MEMADR_GET(bootbase)), NULL);

			status = retm_image_load(handler, epbase);

			retm_image_close(handler);
			break;

		case BOOT_OTP:	/* internal OTP */
#if 0		// not used
			status = otp_image_load(BOOT_MEMADR_GET(bootbase)
						, &load_addr, epbase );
#endif
			break;

		case BOOT_EFLASH: /* SFLASH */
		case BOOT_SFLASH: /* SFLASH */
#if	!defined(BUILD_OPT_DA16200_PTIM) && !defined(BUILD_OPT_DA16200_UEBOOT)
			{
				UINT32 tjmpaddr = 0;
				status = DA16X_SecureBoot(BOOT_OFFSET_GET(bootbase), &tjmpaddr, NULL);
			}
			*epbase = 0;
			if( (status == FALSE) && (DA16X_SecureBoot_GetLock() == FALSE) ){
				// Unsecure RamLib Image
				// goto default case
			}else{
				break;
			}
			/* fall-through */
#endif	//!defined(BUILD_OPT_DA16200_PTIM) && !defined(BUILD_OPT_DA16200_UEBOOT)
		default:
			handler = flash_image_open((sizeof(UINT32)*8)
					, BOOT_OFFSET_GET(bootbase)
					, (VOID *)&da16x_environ_lock);

			//=================================================
			// Load Flash RamLib
			//=================================================
			status = flash_image_check(handler
				, 0
				, BOOT_OFFSET_GET(bootbase)
				);

			if( (status > 0) && (BOOT_MEM_GET(bootbase) != BOOT_SFLASH_C) ){
				status = flash_image_test(handler
							, BOOT_OFFSET_GET(bootbase)
							, &rlib_la, epbase
						);
			}

			if( status > 0 ){
				rlib_la = 0;
				flash_image_load(handler
					//, BOOT_OFFSET_GET(BOOT_OFFSET_GET(bootbase))
					, BOOT_OFFSET_GET(BOOT_OFFSET_GET(bootbase+0xD00))		/* F_F_S ??? */
					, &rlib_la, epbase
					);
			}

			flash_image_close(handler);
			*epbase = 0;
			break;
	}


	return status;
}

/******************************************************************************
 *  ramlib_printf( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifndef BUILD_OPT_DA16200_MAC
static	int	ramlib_printf(char *fmt, ...)
{
#ifndef BUILD_OPT_DA16200_PTIM
	va_list ap;
	va_start(ap,fmt);

	VPRINTF(fmt, ap);

	va_end(ap);
#endif

	return TRUE;
}
#endif //BUILD_OPT_DA16200_MAC

/******************************************************************************
 *  ramlib_getinfo( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT8	ramlib_get_info(void)
{
	RAMLIB_SYMBOL_TYPE *ramlib;

	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;

	return (ramlib->magic)&0xff;
}

/******************************************************************************
 *  ramlib_clear( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	ramlib_clear(void)
{
	RAMLIB_SYMBOL_TYPE *ramlib;

	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;

	ramlib->magic = 0;
	ramlib->build = 0;
}

/******************************************************************************
 *  ramlib_clear( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int ptim_manager_check(UINT32 storeaddr, UINT32 force)
{
	DA16X_UNUSED_ARG(force);

#ifdef TIM_RA_FROM_MASKROM
	return timro_ptim_manager_check(storeaddr, force);
#else
	UINT32	status, offset;
	HANDLE  rimhandler;

	offset = BOOT_OFFSET_GET(storeaddr);
	offset = (DA16X_RETMEM_BASE|offset); // load point

	rimhandler = retm_image_open(0
		, (offset)
		, NULL);

	status = retm_image_test(rimhandler, &offset);

	retm_image_close(rimhandler);

	return status;
#endif
}

/******************************************************************************
 *  ramlib_clear( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int ptim_manager_load(UINT32 loadaddr, UINT32 storeaddr )
{
#ifdef TIM_RA_FROM_MASKROM
	return timro_ptim_manager_load(loadaddr, storeaddr);
#else
	HANDLE  handler;
	UINT32	status;
	UINT32	ptim_lp, ptim_ep;
	UINT32	memory;

#ifdef	BUILD_OPT_DA16200_FPGA
	UINT32	offset;
	offset = BOOT_OFFSET_GET(storeaddr);
#endif //BUILD_OPT_DA16200_FPGA

	memory = BOOT_MEM_GET(storeaddr);


	if( memory != BOOT_RETMEM ){
		return FALSE;
	}

	status = 0;
	ptim_lp = 0;
	ptim_ep = 0;

	switch(BOOT_MEM_GET(loadaddr)){
#ifdef	BUILD_OPT_DA16200_FPGA
		case BOOT_NOR: /* NOR */
			da16x_environ_lock(TRUE);
			//=================================================
			// Load Flash RamLib
			//=================================================
			ptim_lp = (DA16X_RETMEM_BASE|offset)+(sizeof(UINT32)*2); // load point

			status = nor_image_load((NOR_BOOT_CHECK|NOR_BOOT_RUN)
				, (DA16X_NOR_BASE|BOOT_OFFSET_GET(loadaddr))
				, &ptim_lp, &ptim_ep );

			if( status > 0 ){
				((UINT32 *)(DA16X_RETMEM_BASE|offset))[0] = ptim_ep;
				((UINT32 *)(DA16X_RETMEM_BASE|offset))[1] = status;
			}

			da16x_environ_lock(FALSE);
			break;
#endif	//BUILD_OPT_DA16200_FPGA
		case BOOT_EFLASH: /* SFLASH */
		case BOOT_SFLASH: /* SFLASH */
			{
				UINT32 tjmpaddr = 0;
				status = DA16X_SecureBoot(BOOT_OFFSET_GET(loadaddr), &tjmpaddr, NULL);
			}
			if( (status == FALSE) && (DA16X_SecureBoot_GetLock() == FALSE) ){
				// Unsecure RamLib Image
				// goto defaut case
			}else{
				break;
			}
			/* fall-through */
		default:
			handler = flash_image_open((sizeof(UINT32)*8)
					, BOOT_OFFSET_GET(loadaddr)
					, (VOID *)&da16x_environ_lock);

			//=================================================
			// Load Flash RamLib
			//=================================================
			status = flash_image_check(handler
				, 0
				, BOOT_OFFSET_GET(loadaddr)
				);

			if( (status > 0) && (BOOT_MEM_GET(loadaddr) != BOOT_SFLASH_C) ){
				status = flash_image_test(handler
							, BOOT_OFFSET_GET(loadaddr)
							, &ptim_lp, &ptim_ep
						);
			}

			if( status > 0 ){
				flash_image_load(handler
					, BOOT_OFFSET_GET(BOOT_OFFSET_GET(loadaddr))
					, &ptim_lp, &ptim_ep
					);

				if(  (BOOT_MEM_GET(loadaddr) != BOOT_RETMEM)
				   && (BOOT_MEM_GET(ptim_ep) == BOOT_RETMEM) ){
					HANDLE  rimhandler;

					rimhandler = retm_image_open(0
						, (ptim_ep)
						, NULL);

					status = retm_image_test(rimhandler, &ptim_ep);

					retm_image_close(rimhandler);
				}
			}

			flash_image_close(handler);

			break;
	}

	return status;
#endif
}

/******************************************************************************
 *  ramlib_clear( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void ptim_clear(UINT32 storeaddr)
{
#ifdef TIM_RA_FROM_MASKROM
	timro_ptim_clear(storeaddr);
#else
  	UINT32	memory, offset;
	memory = BOOT_MEM_GET(storeaddr);
	offset = BOOT_OFFSET_GET(storeaddr);

	if( memory == BOOT_RETMEM ){
		((UINT32 *)(DA16X_RETMEM_BASE|offset))[0] = 0;
		((UINT32 *)(DA16X_RETMEM_BASE|offset))[1] = 0;
	}
#endif
}

/* EOF */
