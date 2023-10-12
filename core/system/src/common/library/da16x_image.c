/**
 ****************************************************************************************
 *
 * @file da16x_image.c
 *
 * @brief Binary image management
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
#include "da16x_system.h"
#include "library/library.h"
#include "library/da16x_image.h"
#include "driver.h"	
#include "common_def.h"


//------------------------------------------------------------
//
//------------------------------------------------------------
#ifdef	SUPPORT_SFLASH_DEVICE
#ifdef	BUILD_OPT_DA16200_ASIC
#define	SUPPORT_DA16XIMG_SFLASH
#else	//BUILD_OPT_DA16200_ASIC
#define	SUPPORT_DA16XIMG_SFLASH
#endif	//BUILD_OPT_DA16200_ASIC
#else	//SUPPORT_SFLASH_DEVICE
#undef	SUPPORT_DA16XIMG_SFLASH
#endif	//SUPPORT_SFLASH_DEVICE

#undef	SUPPORT_LOADING_TIME
#undef	SUPPORT_SFLASH_SPEEDUP
#undef	SUPPORT_FLASH_POWERDOWN

#ifdef	SUPPORT_DA16XIMG_SFLASH
#undef  SUPPORT_NOR_FASTACCESS
#else	//SUPPORT_DA16XIMG_SFLASH
#define	SUPPORT_NOR_FASTACCESS
#endif	//SUPPORT_DA16XIMG_SFLASH
//------------------------------------------------------------
//
//------------------------------------------------------------
#define	IMAGE_SFLASH_READ_MODE		(SFLASH_BUS_144) // TEST
#define	IMAGE_SFLASH_WRITE_MODE		(SFLASH_BUS_144) // TEST
#define	IMAGE_SFLASH_SAFEREAD_MODE	(SFLASH_BUS_111)
#define	IMAGE_SFLASH_SAFE_MODE		(SFLASH_BUS_111)
#define IMAGE_SFLASH_SAFEWRITE_MODE	(SFLASH_BUS_111)

#define	ALLOC_BUF_ALIGN(x)		((x + 15) & ~0x0F)
#define	ALLOC_KBUF_ALIGN(x)		((x + 15) & (UINT32)(~0x0F))
#define	ALLOC_CBUF_ALIGN(x)		((x + 15) & ~0x0F)
#define	ALLOC_SBUF_ALIGN(x)		((x + 15) & ~0x0F)

#define	FLASH_ADDRESS_MASK(x)		((x) & 0x00FFFFFF)

#define IMG_INFO_PRINT(...)	//PRINTF( __VA_ARGS__ )
#define IMG_LOAD_PRINT(...)	PRINTF( __VA_ARGS__ )	/* TEMPORARY */

#define	MAX_CERT_NUM	4

#define	DA16X_BOOT_OFFSET_GET(x)		((x)&0x00FFFFFF)
#define	DA16X_RETM_OFFSET_GET(x)		((x)&0x0000FFFF)

//------------------------------------------------------------
//
//------------------------------------------------------------

typedef		struct {
	UINT32	length;
	UINT32	crc;
}	CERT_INFO_TYPE;

typedef		struct	{
	UINT16	check_flag	;
	UINT16	check_boot	;
	HANDLE	boot_flash	;
	UINT8	*flash_imghdr	;
	VOID	*retmem_sfdp	;
	CERT_INFO_TYPE	*certchain;
#ifdef	SUPPORT_LOADING_TIME
	UINT32	timestamp[5];	// 0: init, 1: hdr, 2: sfdp, 3: data, 4: deinit
#endif	//SUPPORT_LOADING_TIME
	VOID	(*lockfunc)(UINT32);
	UINT32	load_offset;

	UINT32	erase_size;
	UINT32	erase_offset;
	UINT32	erase_mode;

	UINT32	ioctldata[8];

} DA16X_FLASH_BOOT_TYPE;

#ifdef __TIM_LOAD_FROM_IMAGE__
typedef	struct  {
	UINT16			check_flag;
	UINT16			check_boot;
	UINT8			*ram_imghdr;
	VOID			*retmem_sfdp;
	CERT_INFO_TYPE	*certchain;
	UINT32			load_offset;

	UINT32			erase_size;
	UINT32			erase_offset;
	UINT32			erase_mode;

} DA16X_RAM_BOOT_TYPE;
#endif  /* __TIM_LOAD_FROM_IMAGE__ */

#define	FLASH_BOOT_LOCKER(sboot,mode)		\
		if(sboot->lockfunc != NULL ){		\
			sboot->lockfunc(mode);			\
		}


#define	SFDP_TABLE_SIZE		0x40
#define	SFDP_TABLE_MAGIC	0x50444653

typedef union {
	UINT8	bulk[164]; // New SFDP 164, Old SFDP 180
	struct	{
		UINT8	sfdp[64];
		UINT8	extra[28];
		UINT8	cmdlst[44];
		UINT8	delay[16];
		UINT8	_dummy[12];
	} sfdptab;
} PSEUDO_SFDP_CHUNK_TYPE;

typedef		struct	{
	UINT32	magic;
	UINT32	devid;
	UINT16	offset;
	UINT16	length;
	PSEUDO_SFDP_CHUNK_TYPE chunk;
} PSEUDO_SFDP_TABLE_TYPE;


#if	defined(BUILD_OPT_DA16200_ASIC) && !defined(SUPPORT_SFLASH_DEVICE)
// Under Test
HANDLE	flash_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker)
{
	return NULL;
}

void	flash_image_close(HANDLE handler)
{
}

UINT32	flash_image_check(HANDLE handler, UINT32 imgtype, UINT32 imghdr_offset)
{
	return 0;
}

UINT32	flash_image_test(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	return 0;
}

void	*flash_image_certificate(HANDLE handler, UINT32 imghdr_offset, UINT32 certindex, UINT32 *certsize)
{
	return NULL;
}

UINT32	flash_image_extract(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	return 0;
}

UINT32	flash_image_load(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	return 0;
}

UINT32	flash_image_write(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize)
{
	return 0;
}

UINT32	flash_image_read(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize)
{
	return 0;
}

UINT32	flash_image_erase(HANDLE handler, UINT32 img_offset, UINT32 img_size)
{
	return 0;
}

UINT32	flash_image_force(HANDLE handler, UINT32 force)
{
	return 0;
}

#else	/*defined(BUILD_OPT_DA16200_ASIC) && !defined(SUPPORT_SFLASH_DEVICE)*/

/******************************************************************************
 *  flash_image_open
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	flash_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	UINT32	*ioctldata, paragood;
	DA16X_FLASH_BOOT_TYPE *da16x_flash_handlr;

	da16x_flash_handlr = (DA16X_FLASH_BOOT_TYPE *)pvPortMalloc(sizeof(DA16X_FLASH_BOOT_TYPE));

	if( da16x_flash_handlr == NULL ){
		return NULL;
	}

	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);

	da16x_flash_handlr->check_boot = (UINT16)(((mode&DA16X_IMG_BOOT) == DA16X_IMG_BOOT) ? 1 : 0) ; /* 1 : speedup, 0 : none */
	da16x_flash_handlr->check_flag = ((mode&DA16X_IMG_SKIP) == DA16X_IMG_SKIP) ? 1 : 0 ; /* 1 : skip, 0 : none */
	da16x_flash_handlr->boot_flash = NULL;
	da16x_flash_handlr->flash_imghdr = NULL;
	da16x_flash_handlr->retmem_sfdp = (&(((UINT32 *)RETMEM_SFLASH_BASE)[0]));
	da16x_flash_handlr->certchain = NULL;
	da16x_flash_handlr->erase_size = 0;

	da16x_flash_handlr->lockfunc = (VOID(*)(UINT32))locker;

	ioctldata = &(da16x_flash_handlr->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash_handlr, TRUE);

	da16x_flash_handlr->boot_flash = SFLASH_CREATE(SFLASH_UNIT_0);

	if( da16x_flash_handlr->boot_flash != NULL ){
		// setup bus width
		ioctldata[0] = mode;
		SFLASH_IOCTL(da16x_flash_handlr->boot_flash
				, SFLASH_BUS_DWIDTH, ioctldata);

		// setup speed
		ioctldata[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(da16x_flash_handlr->boot_flash
				, SFLASH_BUS_MAXSPEED, ioctldata);

		// setup bussel
		ioctldata[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(da16x_flash_handlr->boot_flash
				, SFLASH_SET_BUSSEL, ioctldata);

		// init sflash
#ifdef	SUPPORT_LOADING_TIME
		da16x_btm_control(TRUE);

		if( SFLASH_INIT(da16x_flash_handlr->boot_flash) == TRUE ){
			// to prevent a reinitialization
			if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
				SFLASH_IOCTL(da16x_flash_handlr->boot_flash
					, SFLASH_SET_INFO, ioctldata);
				paragood = TRUE;
			}else{
				paragood = FALSE;
			}
		}else{
			ioctldata[0] = 0;
			SFLASH_IOCTL(da16x_flash_handlr->boot_flash
					, SFLASH_GET_INFO, ioctldata);

			if( ioctldata[3] == SFDP_TABLE_SIZE ){
				paragood = TRUE;
			}else{
				paragood = FALSE;
			}
		}

		da16x_btm_control(FALSE);
		da16x_flash_handlr->timestamp[0]
				= da16x_btm_get_timestamp();

		da16x_flash_handlr->timestamp[1] = 0;
		da16x_flash_handlr->timestamp[2] = 0;
		da16x_flash_handlr->timestamp[3] = 0;
		da16x_flash_handlr->timestamp[4] = 0;
#else	//SUPPORT_LOADING_TIME
		if( SFLASH_INIT(da16x_flash_handlr->boot_flash) == TRUE ){
			// to prevent a reinitialization
			if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){
				SFLASH_IOCTL(da16x_flash_handlr->boot_flash
					, SFLASH_SET_INFO, ioctldata);
				paragood = TRUE;
			}else{
				paragood = FALSE;
			}
		}else{
			ioctldata[0] = 0;
			SFLASH_IOCTL(da16x_flash_handlr->boot_flash
					, SFLASH_GET_INFO, ioctldata);

			if( ioctldata[3] == SFDP_TABLE_SIZE ){
				paragood = TRUE;
			}else{
				paragood = FALSE;
			}
		}
#endif	//SUPPORT_LOADING_TIME

		// setup bus mode

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2A)|0x1);

		ioctldata[0] = 0;
		SFLASH_IOCTL(da16x_flash_handlr->boot_flash
				, SFLASH_CMD_WAKEUP, ioctldata);
		if( ioctldata[0] > 0 ){
			ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
			vTaskDelay( ioctldata[0]/10 );
		}

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2A)|0x2);

		if( paragood == TRUE ){
			ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
		}else{
			ioctldata[0] = IMAGE_SFLASH_SAFEREAD_MODE; // slow mode
		}
		SFLASH_IOCTL(da16x_flash_handlr->boot_flash
				, SFLASH_BUS_CONTROL, ioctldata);

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2A)|0x3);

		FLASH_BOOT_LOCKER(da16x_flash_handlr, FALSE);
	}
	else
	{
		FLASH_BOOT_LOCKER(da16x_flash_handlr, FALSE);

		vPortFree(da16x_flash_handlr);
		da16x_flash_handlr = NULL;
	}

	return da16x_flash_handlr;
#else	//SUPPORT_DA16XIMG_SFLASH
	UINT32 pagesize;
	UINT32 *ioctldata;
	DA16X_FLASH_BOOT_TYPE *da16x_flash_handlr;

	da16x_flash_handlr = (DA16X_FLASH_BOOT_TYPE *)pvPortMalloc(sizeof(DA16X_FLASH_BOOT_TYPE));

	if( da16x_flash_handlr == NULL ){
		return NULL;
	}

	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);

	da16x_flash_handlr->check_boot = ((mode&DA16X_IMG_BOOT) == DA16X_IMG_BOOT) ? 1 : 0 ; /* 1 : speedup, 0 : none */
	da16x_flash_handlr->check_flag = ((mode&DA16X_IMG_SKIP) == DA16X_IMG_SKIP) ? 1 : 0 ; /* 1 : skip, 0 : none */
	da16x_flash_handlr->boot_flash = NULL;
	da16x_flash_handlr->flash_imghdr = NULL;
	da16x_flash_handlr->retmem_sfdp = (&(((UINT32 *)RETMEM_SFLASH_BASE)[0]));
	da16x_flash_handlr->certchain = NULL;
	da16x_flash_handlr->erase_size = 0;

	da16x_flash_handlr->lockfunc = (VOID(*)(UINT32))locker;

	ioctldata = &(da16x_flash_handlr->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash_handlr, TRUE);

	da16x_flash_handlr->boot_flash = NOR_CREATE(NOR_UNIT_0);

	if( da16x_flash_handlr->boot_flash != NULL ){

		NOR_INIT(da16x_flash_handlr->boot_flash);

#ifdef	BUILD_OPT_DA16200_ROMALL
		ioctldata[0] = TRUE;
		NOR_IOCTL(da16x_flash_handlr->boot_flash, NOR_SET_BOOTMODE, ioctldata);
#endif	//BUILD_OPT_DA16200_ROMALL

		pagesize = 0;
		NOR_IOCTL(da16x_flash_handlr->boot_flash, NOR_GET_SIZE, &pagesize);

		FLASH_BOOT_LOCKER(da16x_flash_handlr, FALSE);
	}
	else
	{
		FLASH_BOOT_LOCKER(da16x_flash_handlr, FALSE);

		vPortFree(da16x_flash_handlr);
		da16x_flash_handlr = NULL;
	}

	return da16x_flash_handlr;
#endif	//SUPPORT_DA16XIMG_SFLASH
}

#ifdef	SUPPORT_LOADING_TIME
static const char *imp_print_format = "%s Time = %08x \n";
#endif //SUPPORT_LOADING_TIME

/******************************************************************************
 *  flash_image_close
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	flash_image_close(HANDLE handler)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	UINT32	*ioctldata;
	DA16X_FLASH_BOOT_TYPE *da16x_flash;

	if( handler == NULL ){
		return;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	if( da16x_flash->erase_size > 0 ){
		ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

		ioctldata[0] = da16x_flash->erase_offset;
		ioctldata[1] = da16x_flash->erase_size;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_LOCK, ioctldata);
	}

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(TRUE);

	ioctldata[0] = IMAGE_SFLASH_READ_MODE;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

#ifdef	SUPPORT_FLASH_POWERDOWN
	ioctldata[0] = 0;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_CMD_POWERDOWN, ioctldata);
	if( ioctldata[0] > 0 ){
		ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
		vTaskDelay( ioctldata[0]/10 );
	}
#endif	//SUPPORT_FLASH_POWERDOWN

	SFLASH_CLOSE(da16x_flash->boot_flash);

	da16x_btm_control(FALSE);
	da16x_flash->timestamp[4] = da16x_btm_get_timestamp();

	if( da16x_flash->timestamp[0] != 0 )
	{
		IMG_INFO_PRINT(imp_print_format, "Init", da16x_flash->timestamp[0]);
		IMG_INFO_PRINT(imp_print_format, "Hdr ", da16x_flash->timestamp[1]);
		IMG_INFO_PRINT(imp_print_format, "SFDP", da16x_flash->timestamp[2]);
		IMG_INFO_PRINT(imp_print_format, "Data", da16x_flash->timestamp[3]);
		IMG_INFO_PRINT(imp_print_format, "Dein", da16x_flash->timestamp[4]);
	}
#else	//SUPPORT_LOADING_TIME
	ioctldata[0] = IMAGE_SFLASH_READ_MODE;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

#ifdef	SUPPORT_FLASH_POWERDOWN
	ioctldata[0] = 0;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_CMD_POWERDOWN, ioctldata);
	if( ioctldata[0] > 0 ){
		ioctldata[0] = ioctldata[0] / 1000 ; /* msec */
		vTaskDelay( ioctldata[0]/10 );
	}
#endif	//SUPPORT_FLASH_POWERDOWN

	SFLASH_CLOSE(da16x_flash->boot_flash);
#endif	//SUPPORT_LOADING_TIME

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	if( da16x_flash->flash_imghdr != NULL ){
		vPortFree(da16x_flash->flash_imghdr);
		da16x_flash->flash_imghdr = NULL;
	}

	if( da16x_flash->certchain != NULL ){
		vPortFree(da16x_flash->certchain);
		da16x_flash->certchain = NULL;
	}

	vPortFree(da16x_flash);
#else	//SUPPORT_DA16XIMG_SFLASH
	UINT32	*ioctldata;
	DA16X_FLASH_BOOT_TYPE *da16x_flash;

	if( handler == NULL ){
		return;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	if( da16x_flash->erase_size > 0 ){
		ioctldata[0] = da16x_flash->erase_offset;
		ioctldata[1] = da16x_flash->erase_size;
		NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_LOCK, ioctldata);
	}

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(TRUE);

	NOR_CLOSE(da16x_flash->boot_flash);

	da16x_btm_control(FALSE);
	da16x_flash->timestamp[4] = da16x_btm_get_timestamp();

	if( da16x_flash->timestamp[0] != 0 )
	{
		IMG_INFO_PRINT(imp_print_format, "Init", da16x_flash->timestamp[0]);
		IMG_INFO_PRINT(imp_print_format, "Hdr ", da16x_flash->timestamp[1]);
		IMG_INFO_PRINT(imp_print_format, "SFDP", da16x_flash->timestamp[2]);
		IMG_INFO_PRINT(imp_print_format, "Data", da16x_flash->timestamp[3]);
		IMG_INFO_PRINT(imp_print_format, "Dein", da16x_flash->timestamp[4]);
	}
#else	//SUPPORT_LOADING_TIME

	NOR_CLOSE(da16x_flash->boot_flash);

#endif	//SUPPORT_LOADING_TIME

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	if( da16x_flash->flash_imghdr != NULL ){
		vPortFree(da16x_flash->flash_imghdr);
		da16x_flash->flash_imghdr = NULL;
	}

	if( da16x_flash->certchain != NULL ){
		vPortFree(da16x_flash->certchain);
		da16x_flash->certchain = NULL;
	}

	vPortFree(da16x_flash);
#endif	//SUPPORT_DA16XIMG_SFLASH
}

/******************************************************************************
 *  da16x_flash_speedup_loadtime
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void da16x_flash_speedup_loadtime(DA16X_FLASH_BOOT_TYPE *da16x_flash, UINT32 *ioctldata)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	if( SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_GET_MODE, ioctldata) == TRUE ){

		da16x_sflash_set_maxspeed( ioctldata[0] );

		SFLASH_IOCTL(da16x_flash->boot_flash
				, SFLASH_BUS_MAXSPEED, ioctldata);

#ifdef	SUPPORT_SFLASH_SPEEDUP
		if( da16x_flash->check_boot == 1 )
		{
			UINT32 sysclock = 0;
			_sys_clock_read((UINT32 *) &sysclock, sizeof(UINT32));

			if( sysclock < ioctldata[0] ){ // for DA16X
				sysclock = ioctldata[0];
				_sys_clock_write((UINT32 *) &sysclock, sizeof(UINT32));
			}
		}
#endif	//SUPPORT_SFLASH_SPEEDUP

		SFLASH_IOCTL(da16x_flash->boot_flash
				, SFLASH_GET_BUSSEL, ioctldata);

		da16x_sflash_set_bussel( ioctldata[0] );
	}
#endif	//SUPPORT_DA16XIMG_SFLASH
}

/******************************************************************************
 *  flash_image_check
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 flash_image_check(HANDLE handler, UINT32 imgtype, UINT32 imghdr_offset)
{
	DA16X_UNUSED_ARG(imgtype);

	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT8	*readregion ;
	DA16X_IMGHEADER_TYPE *imghdr;
	UINT32	image_crctemp;
	UINT32  *ioctldata;
	PSEUDO_SFDP_TABLE_TYPE *sfdp;

	//Printf(" [%s] imgtype:%d imghdr_offset:0x%x \n", __func__, imgtype, imghdr_offset);

	if( handler == NULL ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	// Read First Block :: Image Header
	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);
	readregion = (UINT8 *)(imghdr_offset);

	/* Speed Up SFLASH */
	da16x_flash_speedup_loadtime(da16x_flash, ioctldata);

	if(da16x_flash->flash_imghdr == NULL ){
		UINT32 bkuphcrc;

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x1);

		da16x_flash->flash_imghdr = (UINT8 *)pvPortMalloc(sizeof(DA16X_IMGHEADER_TYPE));
		imghdr = (DA16X_IMGHEADER_TYPE *)(da16x_flash->flash_imghdr);
		imghdr->magic = 0; // Reset

#ifdef	SUPPORT_DA16XIMG_SFLASH

#ifdef	SUPPORT_LOADING_TIME
		da16x_btm_control(TRUE);

		SFLASH_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((imghdr)), sizeof(DA16X_IMGHEADER_TYPE));
		bkuphcrc = imghdr->hcrc ;
		imghdr->hcrc = 0;
		image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)imghdr, sizeof(DA16X_IMGHEADER_TYPE), (~0));
		imghdr->hcrc = bkuphcrc;

		da16x_btm_control(FALSE);
		da16x_flash->timestamp[1] = da16x_btm_get_timestamp();
#else	//SUPPORT_LOADING_TIME
		SFLASH_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((imghdr)), sizeof(DA16X_IMGHEADER_TYPE));
		bkuphcrc = imghdr->hcrc ;
		imghdr->hcrc = 0;
		image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)imghdr, sizeof(DA16X_IMGHEADER_TYPE), (UINT32)(~0));
		imghdr->hcrc = bkuphcrc;
#endif	//SUPPORT_LOADING_TIME
#else	//SUPPORT_DA16XIMG_SFLASH
#ifdef	SUPPORT_LOADING_TIME
		da16x_btm_control(TRUE);

		NOR_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((imghdr)), sizeof(DA16X_IMGHEADER_TYPE));
		bkuphcrc = imghdr->hcrc ;
		imghdr->hcrc = 0;
		image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)imghdr, sizeof(DA16X_IMGHEADER_TYPE), (~0));
		imghdr->hcrc = bkuphcrc;

		da16x_btm_control(FALSE);
		da16x_flash->timestamp[1] = da16x_btm_get_timestamp();
#else	//SUPPORT_LOADING_TIME
		NOR_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((imghdr)), sizeof(DA16X_IMGHEADER_TYPE));
		bkuphcrc = imghdr->hcrc ;
		imghdr->hcrc = 0;
		image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)imghdr, sizeof(DA16X_IMGHEADER_TYPE), (~0));
		imghdr->hcrc = bkuphcrc;
#endif	//SUPPORT_LOADING_TIME
#endif	//SUPPORT_DA16XIMG_SFLASH

		// Check
		if((imghdr->magic) != (DA16X_IH_MAGIC)){
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xA);

			Printf(RED_COLOR " [%s] Wrong magic code: 0x%x (MAGIC:0x%x) \n" CLEAR_COLOR,
											__func__, (imghdr->magic), (DA16X_IH_MAGIC) );

			vPortFree(da16x_flash->flash_imghdr);
			da16x_flash->flash_imghdr = NULL;

			FLASH_BOOT_LOCKER(da16x_flash, FALSE);

			return 0;
		}
	}
	else
	{
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x2);

		imghdr = (DA16X_IMGHEADER_TYPE *)da16x_flash->flash_imghdr;
		FLASH_BOOT_LOCKER(da16x_flash, FALSE);

		return sizeof(DA16X_IMGHEADER_TYPE);
	}

	if( (imghdr->hcrc) != image_crctemp ){
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xB);

		PRINTF("Image Header, CRC Failed (%08x - %08x) [%08x]!!\n"
			, da16xfunc_ntohl(imghdr->hcrc)
			, image_crctemp
			, da16x_btm_get_timestamp() );

		vPortFree(da16x_flash->flash_imghdr);
		da16x_flash->flash_imghdr = NULL;

		FLASH_BOOT_LOCKER(da16x_flash, FALSE);

		return 0;
	}

	sfdp = (PSEUDO_SFDP_TABLE_TYPE *)da16x_flash->retmem_sfdp;

	if( (sfdp->magic != SFDP_TABLE_MAGIC) &&  (imghdr->sfdpcrc != 0) ){
		// TODO: SFDP Read
		image_crctemp	= 0x00;

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x3);

		readregion = readregion + sizeof(DA16X_IMGHEADER_TYPE);
		sfdp = (PSEUDO_SFDP_TABLE_TYPE *)pvPortMalloc(sizeof(PSEUDO_SFDP_TABLE_TYPE));

#ifdef	SUPPORT_DA16XIMG_SFLASH
		if( sfdp != NULL ){
			SFLASH_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)(sfdp), sizeof(PSEUDO_SFDP_TABLE_TYPE));
			image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)sfdp, sizeof(PSEUDO_SFDP_TABLE_TYPE), (UINT32)(~0));
		}
#else	//SUPPORT_DA16XIMG_SFLASH
		if( sfdp != NULL ){
			NOR_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)(sfdp), sizeof(PSEUDO_SFDP_TABLE_TYPE));
			image_crctemp = da16x_hwcrc32(sizeof(UINT32), (void *)sfdp, sizeof(PSEUDO_SFDP_TABLE_TYPE), (~0));
		}
#endif	//SUPPORT_DA16XIMG_SFLASH

		if( (sfdp == NULL) || ((imghdr->sfdpcrc) != image_crctemp) ){
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xC);

			PRINTF("SFDP, CRC Failed (%08x - %08x) [%08x]!!\n"
				, da16xfunc_ntohl(imghdr->sfdpcrc)
				, image_crctemp
				, da16x_btm_get_timestamp() );

			if( sfdp != NULL ) {
				vPortFree(sfdp);
			}
			vPortFree(da16x_flash->flash_imghdr);
			da16x_flash->flash_imghdr = NULL;

			FLASH_BOOT_LOCKER(da16x_flash, FALSE);

			return 0;
		}

		DRIVER_MEMCPY(da16x_flash->retmem_sfdp, sfdp, sizeof(PSEUDO_SFDP_TABLE_TYPE));
		vPortFree(sfdp);

#ifdef	SUPPORT_DA16XIMG_SFLASH
		if( da16x_sflash_setup_parameter(ioctldata) == TRUE ){

			SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_INFO, ioctldata);

			/* Speed Up SFLASH */
			da16x_flash_speedup_loadtime(da16x_flash, ioctldata);
		}

		// Check Bus mode
		ioctldata[0] = 0;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_GET_INFO, ioctldata);

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x4);

		if( ioctldata[3] == SFDP_TABLE_SIZE ){
			ioctldata[0] = IMAGE_SFLASH_READ_MODE;
			SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);
		}else{
			ioctldata[0] = IMAGE_SFLASH_SAFEREAD_MODE;
			SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);
		}

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x5);

#endif	//SUPPORT_DA16XIMG_SFLASH
	}

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return sizeof(DA16X_IMGHEADER_TYPE);
}

/******************************************************************************
 *  flash_image_load_internal
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	DA16X_SFLASH_TEST_SIZE	(sizeof(UINT8)*256)
#define	XFC_DMA_PORT		0x1A

static UINT32	flash_image_load_internal(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr, UINT32 skip)
{
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	DA16X_IMGHEADER_TYPE  *imghdr;
	UINT8	*writeregion, *readregion;
	UINT32	crc32value, ihdroffset, readsize;
	UINT32	*ioctldata;

	if( handler == NULL ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	imghdr = (DA16X_IMGHEADER_TYPE *)da16x_flash->flash_imghdr;

	if( imghdr == NULL ){
		return 0;
	}

	*load_addr = (UINT32)imghdr->cpoint;
	*jmp_addr = (UINT32)imghdr->cpoint;

	if( *load_addr == 0 || imghdr->ccrc == 0 || imghdr->csize == 0){
		return 0;
	}

	// Copy Image
	if( (*load_addr <= DA16X_ROM_END) || ((*load_addr >= DA16X_CACHE_BASE) && (*load_addr < DA16X_CACHE_END)) || ( skip == TRUE ) ){
		writeregion = (UINT8 *) 0xFFFFFFFF;
	}else{
		writeregion = (UINT8 *) (*load_addr) ;
	}

	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);

	//readregion = &(readregion[sizeof(image_header_t)]) ;
	ihdroffset = (imghdr_offset + sizeof(DA16X_IMGHEADER_TYPE));
	if( imghdr->sfdpcrc != 0 ){
		ihdroffset = (ihdroffset + sizeof(PSEUDO_SFDP_TABLE_TYPE));
	}

	if( da16x_flash->certchain == NULL ){
		UINT32 certnum, i;

		certnum = (UINT32)(imghdr->keynum) + ((imghdr->dbgkey == 1) ? 1 : 0);

		ihdroffset = ihdroffset + ( sizeof(CERT_INFO_TYPE)*((certnum+1)&(UINT32)(~0x01)) );

		for( i = 0; i < certnum; i++ ){
			ihdroffset = ihdroffset + ALLOC_KBUF_ALIGN(da16x_flash->certchain[i].length) ;
		}
	}

	//readregion = (UINT8 *)imghdr_offset;
	if( skip == TRUE ){
		// Immediate Offset
		readregion  = (UINT8 *)DA16X_BOOT_OFFSET_GET(imghdr_offset) + BOOT_MEMADR_GET( ((UINT32)*load_addr) ) ;
	}else if( writeregion != ((UINT8 *)0xFFFFFFFF) ){
		// Unsecure SRAM Boot, hidden offset
		readregion  = (UINT8 *)( imghdr->version + DA16X_BOOT_OFFSET_GET(imghdr_offset) );
	}else
	{
		// Absolute Offset
		if( imghdr->version < 100 ){ // Cache ???, Cache Address > Flash Offset @ DA16200
			readregion  = (UINT8 *)DA16X_BOOT_OFFSET_GET( ((UINT32)*load_addr) ) - DA16X_CACHE_BASE + da16x_get_cacheoffset();
		}else{	// SRAM ??
			readregion  = (UINT8 *)DA16X_BOOT_OFFSET_GET( ((UINT32)*load_addr) );
		}
	}

	if( da16x_flash->check_flag == 1 ){
		// Skip CRC
		readsize = DA16X_SFLASH_TEST_SIZE;
	}else{
		readsize = imghdr->csize;
	}

	IMG_INFO_PRINT("Image Data, CRC Check Start (%p, %p, %p) [%08x]\n"
		, (UINT8 *)(*load_addr), writeregion, readregion
		, da16x_btm_get_timestamp() );

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x6);

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(TRUE);
#endif	//SUPPORT_LOADING_TIME

#ifdef	SUPPORT_DA16XIMG_SFLASH
#ifdef	SUPPORT_NOR_FASTACCESS
	// TODO:
//da16200removed:	readregion = (UINT8 *)(DA16X_XIP_BASE|((UINT32)readregion));
	readregion = (UINT8 *)(DA16X_CACHE_BASE|((UINT32)readregion));

	da16x_cache_disable();

	ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x7);

	crc32value = da16x_hwcopycrc32( (sizeof(UINT32)), (UINT8 *)writeregion, (UINT8 *)readregion, readsize, (~0));
	da16x_cache_enable(FALSE);
#else	//SUPPORT_NOR_FASTACCESS
	{
		UINT32 burstoff, burstsiz;
		UINT32 testsize;

		if( writeregion == ((UINT8 *)0xFFFFFFFF) ){
			writeregion = (UINT8 *)NULL; // No-Incr !!
		}

		testsize = readsize;

		ioctldata[0] = da16x_flash->check_boot; // Normal-Read
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_XIPMODE, ioctldata);

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x9);

		da16x_crc32_start(
				(CRC_DAT_TYPE(0)|CRC_PAR_TYPE(2)|CRC_ACC_TYPE(1)|CRC_PATH_SEL(XFC_DMA_PORT))
				, ((UINT32)writeregion), ((UINT32)writeregion+testsize)
				, (UINT32)(~0)
			);

		for( burstoff = 0; burstoff < readsize; ){
			burstsiz = ((readsize - burstoff) > testsize ) ? testsize : (readsize - burstoff) ;
			SFLASH_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((writeregion)), burstsiz);
			burstoff += burstsiz;
			readregion = (UINT8 *)((UINT32)readregion + burstsiz);
		}

		ioctldata[0] = 0; // None
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_XIPMODE, ioctldata);

		crc32value = da16x_crc32_stop();
	}
#endif	//SUPPORT_NOR_FASTACCESS
#else	//SUPPORT_DA16XIMG_SFLASH
	readregion = (UINT8 *)(DA16X_NOR_BASE|((UINT32)readregion));

	crc32value = da16x_hwcopycrc32( (CRC_CTRL_CACHE|sizeof(UINT32)), (UINT8 *)writeregion, (UINT8 *)readregion, readsize, (~0));
#endif	//SUPPORT_DA16XIMG_SFLASH

	ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x8);

#ifdef	SUPPORT_LOADING_TIME
	da16x_btm_control(FALSE);
#endif	//SUPPORT_LOADING_TIME

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	if( ( da16x_flash->check_flag == 0 )
	    && ( imghdr->ccrc != crc32value ) ){
		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xE);

		PRINTF("Image Code, CRC Failed (%08x - %08x, %d)"
#ifdef	SUPPORT_LOADING_TIME
			"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
			"!!\n"
			, imghdr->ccrc
			, crc32value
			, (imghdr->csize)
#ifdef	SUPPORT_LOADING_TIME
			, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
			);
		return 0;
	}else{
		IMG_INFO_PRINT("Image Code, CRC Completed (%08x)"
#ifdef	SUPPORT_LOADING_TIME
			"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
			"!!\n"
			, imghdr->ccrc
#ifdef	SUPPORT_LOADING_TIME
			, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
			);
	}

        return imghdr->csize;
}

/******************************************************************************
 *  flash_image_test
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 flash_image_test(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	DA16X_IMGHEADER_TYPE  *imghdr;
	UINT8	*writeregion, *readregion;
	UINT32	crc32value, ihdroffset;


	if( handler == NULL ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;

	imghdr = (DA16X_IMGHEADER_TYPE *)da16x_flash->flash_imghdr;

	if( imghdr == NULL ){
		return 0;
	}

	*load_addr = (UINT32)imghdr->cpoint;
	*jmp_addr = (UINT32)imghdr->cpoint;


	if( *load_addr == 0 || imghdr->datsiz == 0){
		return 0;
	}

	// Test Image
	writeregion = (UINT8 *) 0xFFFFFFFF;

	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);

	ihdroffset = (imghdr_offset + sizeof(DA16X_IMGHEADER_TYPE));
	if( imghdr->sfdpcrc != 0 ){
		// Update Offset
		ihdroffset = (ihdroffset + sizeof(PSEUDO_SFDP_TABLE_TYPE));
	}

	readregion = (UINT8 *)ihdroffset;

	if( imghdr->datsiz != 0 ){
		IMG_INFO_PRINT("@ Image Data, CRC Check Start (%p, %p) [%08x]\n"
			, (UINT8 *)(ihdroffset), writeregion
			, da16x_btm_get_timestamp() );
		// Check DATA CRC

#ifdef	SUPPORT_LOADING_TIME
		da16x_btm_control(TRUE);
#endif	//SUPPORT_LOADING_TIME

#ifdef	SUPPORT_DA16XIMG_SFLASH
#ifdef	SUPPORT_NOR_FASTACCESS
		// TODO:
//da16200removed:		readregion = (UINT8 *)(DA16X_XIP_BASE|((UINT32)readregion));
		readregion = (UINT8 *)(DA16X_CACHE_BASE|((UINT32)readregion));

		da16x_cache_disable();

		ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

		crc32value = da16x_hwcopycrc32( (sizeof(UINT32)), (UINT8 *)writeregion, (UINT8 *)readregion, imghdr->datsiz, (~0));
#else	//SUPPORT_NOR_FASTACCESS
		{
			UINT32 burstoff, burstsiz;

			writeregion = (UINT8 *)pvPortMalloc(DA16X_SFLASH_TEST_SIZE);

			da16x_crc32_start(
					(CRC_DAT_TYPE(0)|CRC_PAR_TYPE(2)|CRC_ACC_TYPE(1)|CRC_PATH_SEL(XFC_DMA_PORT))
					, ((UINT32)writeregion), ((UINT32)writeregion+DA16X_SFLASH_TEST_SIZE)
					, (UINT32)(~0)
				);

			for( burstoff = 0; burstoff < imghdr->datsiz; ){
				burstsiz = ((imghdr->datsiz - burstoff) > DA16X_SFLASH_TEST_SIZE ) ? DA16X_SFLASH_TEST_SIZE : (imghdr->datsiz - burstoff) ;
				SFLASH_READ(da16x_flash->boot_flash, (UINT32)readregion, (VOID *)((writeregion)), burstsiz);
				burstoff += burstsiz;
				readregion = (UINT8 *)((UINT32)readregion + burstsiz);
			}

			crc32value = da16x_crc32_stop();

			vPortFree(writeregion);
		}
#endif	//SUPPORT_NOR_FASTACCESS
#else	//SUPPORT_DA16XIMG_SFLASH
		readregion = (UINT8 *)(DA16X_NOR_BASE|((UINT32)readregion));

		crc32value = da16x_hwcopycrc32( (CRC_CTRL_CACHE|sizeof(UINT32)), (UINT8 *)writeregion, (UINT8 *)readregion, imghdr->datsiz, (~0));
#endif	//SUPPORT_DA16XIMG_SFLASH

#ifdef	SUPPORT_LOADING_TIME
		da16x_btm_control(FALSE);
#endif	//SUPPORT_LOADING_TIME

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xF);

		if( imghdr->datcrc != crc32value ){
			ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0xD);

			PRINTF("Image Data, CRC Failed (%08x - %08x, %d)"
#ifdef	SUPPORT_LOADING_TIME
				"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
				"!!\n"
				, imghdr->datcrc
				, crc32value
				, (imghdr->datsiz)
#ifdef	SUPPORT_LOADING_TIME
				, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
				);
			return 0;
		}else{
			IMG_INFO_PRINT("Image Data, CRC Completed (%08x)"
#ifdef	SUPPORT_LOADING_TIME
				"  [%08x]"
#endif	//SUPPORT_LOADING_TIME
				"!!\n"
				, imghdr->datcrc
#ifdef	SUPPORT_LOADING_TIME
				, da16x_btm_get_timestamp()
#endif	//SUPPORT_LOADING_TIME
				);
		}
	}

	if( imghdr->sfdpcrc != 0 ){
	        return (sizeof(DA16X_IMGHEADER_TYPE) + sizeof(PSEUDO_SFDP_TABLE_TYPE) + imghdr->datsiz);
	}
        return (sizeof(DA16X_IMGHEADER_TYPE) + imghdr->datsiz);
}

/******************************************************************************
 *  flash_image_get_certificate
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	*flash_image_certificate(HANDLE handler, UINT32 imghdr_offset, UINT32 certindex, UINT32 *certsize)
{
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	DA16X_IMGHEADER_TYPE *imghdr;
	UINT32 certnum, i;
	UINT32 *ioctldata;

	if( handler == NULL || certsize == NULL ){
		return NULL;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	imghdr = (DA16X_IMGHEADER_TYPE *)(da16x_flash->flash_imghdr);

	if( imghdr == NULL ){
		return NULL;
	}

	certnum = (UINT32)(imghdr->keynum) + (UINT32)((imghdr->dbgkey == 1) ? 1 : 0);

	if( (certindex == 0xFFFFFFFF) && ( imghdr->dbgkey == 1 ) ){
			certindex = (certnum-1);
	}else{
		if( imghdr->keynum <= certindex ){
			return NULL;
		}
	}

	// Flash Offset
	imghdr_offset = DA16X_BOOT_OFFSET_GET(imghdr_offset);
	imghdr_offset = (imghdr_offset + sizeof(DA16X_IMGHEADER_TYPE));
	if( imghdr->sfdpcrc != 0 ){
		imghdr_offset = imghdr_offset + sizeof(PSEUDO_SFDP_TABLE_TYPE);
	}


	if( da16x_flash->certchain == NULL && certnum != 0){

		da16x_flash->certchain = (CERT_INFO_TYPE *)pvPortMalloc(sizeof(CERT_INFO_TYPE)*MAX_CERT_NUM);
		if( da16x_flash->certchain == NULL ){
			return NULL;
		}
#ifdef	SUPPORT_DA16XIMG_SFLASH

		ASIC_DBG_TRIGGER(MODE_INI_STEP(0xC2C)|0x0);

		ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

		ioctldata[0] = da16x_flash->check_boot; // Normal-Read
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_XIPMODE, ioctldata);

		SFLASH_READ(da16x_flash->boot_flash, (UINT32)imghdr_offset, (VOID *)(da16x_flash->certchain), (sizeof(CERT_INFO_TYPE)*certnum));

		ioctldata[0] = 0; // None
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_XIPMODE, ioctldata);

#else	//SUPPORT_DA16XIMG_SFLASH
		NOR_READ(da16x_flash->boot_flash, (UINT32)imghdr_offset, (VOID *)(da16x_flash->certchain), (sizeof(CERT_INFO_TYPE)*certnum));
#endif	//SUPPORT_DA16XIMG_SFLASH
	}

	// Flash Offset update
	imghdr_offset = imghdr_offset + (UINT32)( sizeof(CERT_INFO_TYPE)*((certnum+1)&(UINT32)(~0x01)) );

	for( i = 0; i < certindex; i++ ){
		imghdr_offset = imghdr_offset + ALLOC_KBUF_ALIGN(da16x_flash->certchain[i].length) ;
	}
	*certsize = da16x_flash->certchain[i].length;

	return (VOID *)imghdr_offset;
}

/******************************************************************************
 *  flash_image_extract
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_extract(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	DA16X_UNUSED_ARG(imghdr_offset);

	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	DA16X_IMGHEADER_TYPE  *imghdr;

	if( handler == NULL ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	imghdr = (DA16X_IMGHEADER_TYPE *)da16x_flash->flash_imghdr;

	if( imghdr == NULL ){
		return 0;
	}

	*load_addr = (UINT32)imghdr->cpoint;
	*jmp_addr = (UINT32)imghdr->cpoint;

	return imghdr->csize;
}

/******************************************************************************
 *  flash_image_load
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_load(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr)
{
	return flash_image_load_internal(handler, imghdr_offset, load_addr, jmp_addr, FALSE);
}

/******************************************************************************
 *  flash_image_read
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_read(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize)
{
#ifdef	SUPPORT_NOR_FASTACCESS
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	void	*readregion;
	UINT32	bkupnorype, bkupintr;
	UINT32	*ioctldata;
#ifdef	SUPPORT_DA16XIMG_SFLASH
	SFLASH_HANDLER_TYPE *sflash_flash;
#else	//SUPPORT_DA16XIMG_SFLASH
	NOR_HANDLER_TYPE *nor_flash;
#endif	//SUPPORT_DA16XIMG_SFLASH

#ifdef	SUPPORT_DA16XIMG_SFLASH
	sflash_flash = (SFLASH_HANDLER_TYPE *)da16x_flash->boot_flash;
	////////////////////////////////////////////////////////
	// Read First Block :: Image Header
	readregion = (void *)(sflash_flash->dev_addr + img_offset);
	////////////////////////////////////////////////////////
#else	//SUPPORT_DA16XIMG_SFLASH
	nor_flash = (NOR_HANDLER_TYPE *)da16x_flash->boot_flash;
	////////////////////////////////////////////////////////
	// Read First Block :: Image Header
	readregion = (void *)(nor_flash->dev_addr + img_offset);
	////////////////////////////////////////////////////////
#endif	//SUPPORT_DA16XIMG_SFLASH

	CHECK_INTERRUPTS(bkupintr);
	PREVENT_INTERRUPTS(1);

	ioctldata = &(da16x_flash->ioctldata[0]);

#ifdef	SUPPORT_DA16XIMG_SFLASH
	// TODO:
	da16x_cache_disable();

	ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);
#else	//SUPPORT_DA16XIMG_SFLASH
	bkupnorype = DA16XFPGA_GET_NORTYPE();
	if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
		da16x_cache_disable();
	}
	DA16XFPGA_SET_NORTYPE(DA16X_NORTYPE_XIP); // FLASH access
#endif	//SUPPORT_DA16XIMG_SFLASH
	////////////////////////////////////////////////////////

	DRIVER_MEMCPY( load_addr, (void *)readregion, img_secsize );

	////////////////////////////////////////////////////////
#ifdef	SUPPORT_DA16XIMG_SFLASH
	da16x_cache_enable(FALSE);
#else	//SUPPORT_DA16XIMG_SFLASH
	DA16XFPGA_SET_NORTYPE(bkupnorype);	// FLASH access
	if( (bkupnorype == DA16X_NORTYPE_CACHE) ){
		da16x_cache_enable(FALSE);
	}
#endif	//SUPPORT_DA16XIMG_SFLASH
	PREVENT_INTERRUPTS(bkupintr);
	////////////////////////////////////////////////////////

	return img_secsize;
#else	//SUPPORT_NOR_FASTACCESS
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT32	*ioctldata;

	if( handler == NULL ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	img_offset = DA16X_BOOT_OFFSET_GET(img_offset);

#ifdef	SUPPORT_DA16XIMG_SFLASH
	ioctldata[0] = IMAGE_SFLASH_READ_MODE; // fast mode
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	return (UINT32)SFLASH_READ(da16x_flash->boot_flash, img_offset, load_addr, img_secsize);
#else	//SUPPORT_DA16XIMG_SFLASH
	return NOR_READ(da16x_flash->boot_flash, img_offset, load_addr, img_secsize);
#endif	//SUPPORT_DA16XIMG_SFLASH

#endif	//SUPPORT_NOR_FASTACCESS
}

/******************************************************************************
 *  flash_image_write
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_write(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT32	*ioctldata, status;

	if( handler == NULL || img_secsize == 0 ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	if( da16x_flash->erase_size == 0 ){
		ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;

		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);

		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_CMD_ERASE, ioctldata);
	}

	if( (SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_GET_MODE, ioctldata) == TRUE)
	     && (SFLASH_SUPPORT_QPP(ioctldata[1]) == TRUE) ){
		ioctldata[0] = IMAGE_SFLASH_WRITE_MODE;
	}else{
		ioctldata[0] = IMAGE_SFLASH_SAFEWRITE_MODE;
	}
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	status = (UINT32)SFLASH_WRITE(da16x_flash->boot_flash, img_offset, (VOID *)load_addr, img_secsize);

	if( da16x_flash->erase_size == 0 ){
		ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_LOCK, ioctldata);
	}

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return status;
#else	//SUPPORT_DA16XIMG_SFLASH
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT32	*ioctldata, status;

	if( handler == NULL || img_secsize == 0 ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	if( da16x_flash->erase_size == 0 ){
		ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;

		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_UNLOCK, ioctldata);

		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_ERASE, ioctldata);
	}

	status = (UINT32)NOR_WRITE(da16x_flash->boot_flash, img_offset, (VOID *)load_addr, img_secsize);

	if( da16x_flash->erase_size == 0 ){
		ioctldata[0] = img_offset;
		ioctldata[1] = img_secsize;
		NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_LOCK, ioctldata);
	}

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return status;
#endif	//SUPPORT_DA16XIMG_SFLASH
}

/******************************************************************************
 *  flash_image_erase
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_erase(HANDLE handler, UINT32 img_offset, UINT32 img_size)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT32	*ioctldata;

	if( handler == NULL || img_size == 0 ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;

	da16x_flash->erase_mode   = ioctldata[0];
	da16x_flash->erase_offset = img_offset;
	da16x_flash->erase_size   = img_size;

	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_BUS_CONTROL, ioctldata);

	ioctldata[0] = img_offset;
	ioctldata[1] = img_size;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_UNLOCK, ioctldata);

	ioctldata[0] = img_offset;
	ioctldata[1] = img_size;
	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_CMD_ERASE, ioctldata);

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return img_size;
#else	//SUPPORT_DA16XIMG_SFLASH
	DA16X_FLASH_BOOT_TYPE *da16x_flash;
	UINT32	*ioctldata;

	if( handler == NULL || img_size == 0 ){
		return 0;
	}

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;
	ioctldata = &(da16x_flash->ioctldata[0]);

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	ioctldata[0] = IMAGE_SFLASH_SAFE_MODE;

	da16x_flash->erase_mode   = ioctldata[0];
	da16x_flash->erase_offset = img_offset;
	da16x_flash->erase_size   = img_size;

	ioctldata[0] = img_offset;
	ioctldata[1] = img_size;
	NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_UNLOCK, ioctldata);

	ioctldata[0] = img_offset;
	ioctldata[1] = img_size;
	NOR_IOCTL(da16x_flash->boot_flash, NOR_SET_ERASE, ioctldata);

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return img_size;
#endif	//SUPPORT_DA16XIMG_SFLASH
}

/******************************************************************************
 *  flash_image_force
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	flash_image_force(HANDLE handler, UINT32 force)
{
#ifdef	SUPPORT_DA16XIMG_SFLASH
	DA16X_FLASH_BOOT_TYPE *da16x_flash;

	da16x_flash = (DA16X_FLASH_BOOT_TYPE *)handler;

	if( (da16x_flash == NULL) || (da16x_flash->boot_flash == NULL) ){
		return FALSE;
	}

	FLASH_BOOT_LOCKER(da16x_flash, TRUE);

	SFLASH_IOCTL(da16x_flash->boot_flash, SFLASH_SET_UNSUSPEND, &force);

	FLASH_BOOT_LOCKER(da16x_flash, FALSE);

	return TRUE;
#else	//SUPPORT_DA16XIMG_SFLASH
	return TRUE;
#endif	//SUPPORT_DA16XIMG_SFLASH
}

#endif	/*defined(BUILD_OPT_DA16200_ASIC) && !defined(SUPPORT_SFLASH_DEVICE)*/

/******************************************************************************

 ******************************************************************************/

typedef		struct	{
	UINT16			check_flag;
	UINT32			load_offset;
	DA16X_RTMHEADER_TYPE	imghdr	;
	VOID			(*lockfunc)(UINT32);

	UINT32			ioctldata[8];
} DA16X_RETM_IMAGE_TYPE;

#define	RETM_IMAGE_LOCKER(rboot,mode)				\
		if(rboot->lockfunc != NULL ){			\
			rboot->lockfunc(mode);			\
		}

static DA16X_RETM_IMAGE_TYPE *da16x_retm_handlr;

/******************************************************************************
 *  retm_image_open
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE	retm_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker)
{
	DA16X_RETM_IMAGE_TYPE *retm_image;

	if( da16x_retm_handlr != NULL ){
		return (HANDLE)da16x_retm_handlr;
	}

	retm_image = (DA16X_RETM_IMAGE_TYPE *)pvPortMalloc(sizeof(DA16X_RETM_IMAGE_TYPE));

	if( retm_image != NULL ){
		DRIVER_MEMSET(retm_image, 0, sizeof(DA16X_RETM_IMAGE_TYPE));

		retm_image->check_flag = (UINT16)mode;
		retm_image->load_offset = DA16200_RETMEM_BASE | DA16X_RETM_OFFSET_GET(imghdr_offset);

		retm_image->lockfunc	= (VOID(*)(UINT32))locker;

		RETM_IMAGE_LOCKER(retm_image, TRUE);

		DRIVER_MEMCPY( &(retm_image->imghdr), (void *)(retm_image->load_offset), sizeof(DA16X_RTMHEADER_TYPE));

		RETM_IMAGE_LOCKER(retm_image, FALSE);

		if( da16xfunc_ntohl(retm_image->imghdr.tag) == 0xFC915249 ){
			da16x_retm_handlr = retm_image;
		}
		else{
			vPortFree(retm_image);
			retm_image = NULL;
		}
	}

	return (HANDLE)retm_image;
}

/******************************************************************************
 *  retm_image_close
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	retm_image_close(HANDLE handler)
{
	DA16X_RETM_IMAGE_TYPE *retm_image;

	if( handler == NULL ){
		return;
	}

	retm_image = (DA16X_RETM_IMAGE_TYPE *)handler;

	vPortFree(retm_image);

	da16x_retm_handlr = NULL;
}

/******************************************************************************
 *  retm_image_load_internal
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32	retm_image_load_internal(HANDLE handler, UINT32 *load_addr, UINT32 *jmp_addr)
{
	DA16X_RETM_IMAGE_TYPE *retm_image;
	UINT32	offset, length, crctmp, crcvalue;

	if( handler == NULL ){
		return 0;
	}

	retm_image = (DA16X_RETM_IMAGE_TYPE *)handler;

	crcvalue = retm_image->imghdr.chksum ;
	offset	= retm_image->load_offset + sizeof(DA16X_RTMHEADER_TYPE);
	length 	= retm_image->imghdr.length;

	RETM_IMAGE_LOCKER(retm_image, TRUE);

	crctmp = da16x_hwcopycrc32( sizeof(UINT32), (UINT8 *)load_addr, (void *)(offset), length, (UINT32)(~0));

	RETM_IMAGE_LOCKER(retm_image, FALSE);

	if( crctmp == crcvalue ){
		*jmp_addr = retm_image->imghdr.loadaddr ;
		return length;
	}

	PRINTF(RED_COLOR " [%s] Error jmp_addr:0x%x len:0x%x crcvalue:0x%x crctmp:0x%x \n" CLEAR_COLOR,
								__func__, retm_image->imghdr.loadaddr, length, crcvalue, crctmp);
	*jmp_addr = 0 ;
	return 0 ;
}


/******************************************************************************
 *  retm_image_test
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	retm_image_test(HANDLE handler, UINT32 *jmp_addr)
{
	return retm_image_load_internal(handler, (void *)0xFFFFFFFF, jmp_addr);
}

/******************************************************************************
 *  retm_image_load
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	retm_image_load(HANDLE handler, UINT32 *jmp_addr)
{
	DA16X_RETM_IMAGE_TYPE *retm_image;

	if( handler == NULL ){
		return 0;
	}

	retm_image = (DA16X_RETM_IMAGE_TYPE *)handler;
	return retm_image_load_internal(handler, (void *)(retm_image->imghdr.loadaddr), jmp_addr);
}

#ifdef INDEPENDENT_FLASH_SIZE
int read_boot_offset(int	*address)
{
	HANDLE sflash;
	UINT32 sfdp[8];
	int offset = 0xff;
	BOOT_OFFSET_TYPE boot_offset;

	memset(&boot_offset, 0, sizeof(BOOT_OFFSET_TYPE));

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);
	if (sflash != NULL) {

		sfdp[0] = da16x_sflash_get_maxspeed();
		SFLASH_IOCTL(sflash, SFLASH_BUS_MAXSPEED, sfdp);

		sfdp[0] = da16x_sflash_get_bussel();
		SFLASH_IOCTL(sflash, SFLASH_SET_BUSSEL, sfdp);

		if (SFLASH_INIT(sflash) == TRUE){
			// to prevent a reinitialization
			if (da16x_sflash_setup_parameter((UINT32*)sfdp) == TRUE){
				SFLASH_IOCTL(sflash, SFLASH_SET_INFO, (UINT32*)sfdp);
				//PRINTF(" sflash init success!! \r\n");
			}
			else{
				PRINTF(" sflash set param fail!! \r\n");
			}
		}
		else {
			sfdp[0] = 0;
			SFLASH_IOCTL(sflash, SFLASH_GET_INFO, (UINT32*)sfdp);

			if (sfdp[3] == 0x40){ 	//SFDP_TABLE_SIZE
				//PRINTF(" read:sflash get info sfdp table size!! \r\n");
			}
			else {
				PRINTF(" sflash get info fail sfdp table size!! \r\n");
			}
		}

		SFLASH_READ(sflash, SFLASH_BOOT_OFFSET, &boot_offset, sizeof(BOOT_OFFSET_TYPE));

		if (boot_offset.offset_magic == OFFSET_MAGIC_CODE) {
			//PRINTF(" boot offset_index: %d offset_address:0x%x \n", boot_offset.offset_index, boot_offset.offset_address);
			offset = (int)(boot_offset.offset_index);
			*address = (int)(boot_offset.offset_address);
		}
		else {
			//PRINTF(RED_COLOR " Abnormal OFFSET_MAGIC_CODE: 0x%x \n" CLEAR_COLOR, boot_offset.offset_magic);
			offset = DEFAULT_OFFSET;
			*address = UNKNOWN_OFFSET_ADDRESS;
		}
		// may need 1RTC clock delay
		SFLASH_CLOSE(sflash);
	}
	else {
		PRINTF(" sflash NULL \n");
	}

	return offset;
}

void write_boot_offset(int offset, int address)
{
	UINT32	ioctldata[8];
	HANDLE	sflash;
	UINT32	dest_addr;
	UINT32	dest_len;
	BOOT_OFFSET_TYPE boot_offset;

	dest_addr = SFLASH_BOOT_OFFSET;
	dest_len = sizeof(BOOT_OFFSET_TYPE);

	// Initialize
	memset(&boot_offset, 0, sizeof(BOOT_OFFSET_TYPE));

	sflash = SFLASH_CREATE(SFLASH_UNIT_0);
	if (sflash != NULL) {
		if (SFLASH_INIT(sflash) == TRUE) {
		}
		else {
			ioctldata[0] = 0;
			SFLASH_IOCTL(sflash, SFLASH_GET_INFO, (UINT32*)ioctldata);

			if (ioctldata[3] == 0x40){ //SFDP_TABLE_SIZE
				//PRINTF(" write:sflash get info sfdp table size!! \r\n");
			}
		}

		ioctldata[0] = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
		SFLASH_IOCTL(sflash, SFLASH_BUS_CONTROL, ioctldata);

		// unlock
		ioctldata[0] = dest_addr;
		ioctldata[1] = dest_len;
		SFLASH_IOCTL(sflash, SFLASH_SET_UNLOCK, ioctldata);

		// write �ϱ� ���� erase
		ioctldata[0] = dest_addr;
		ioctldata[1] = dest_len;
		SFLASH_IOCTL(sflash, SFLASH_CMD_ERASE, ioctldata);

		boot_offset.offset_magic = OFFSET_MAGIC_CODE;
		boot_offset.offset_index = (UINT)offset;
		boot_offset.offset_address = (UINT)address;

		dest_len = (UINT32)SFLASH_WRITE(sflash, dest_addr, &boot_offset, dest_len);
		//PRINTF(" [%s] write boot_offset index:%d  address:0x%x len:%d \r\n", __func__, offset, address, dest_len);
		SFLASH_CLOSE(sflash);
	}
}
#endif   /* INDEPENDENT_FLASH_SIZE */

/* EOF */
