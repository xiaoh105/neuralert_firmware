/**
 ****************************************************************************************
 *
 * @file da16x_image.h
 *
 * @brief Image management
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


#ifndef	__da16x_image_h__
#define	__da16x_image_h__

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#define DA16X_IH_NMLEN	40				/* Image Name Length */
#define DA16X_IH_MAGIC	0x4B394346		/* Image Magic Number */
#define	DA16X_IMG_BOOT	0x80000000
#define	DA16X_IMG_SKIP	0x40000000

typedef struct {
	unsigned int	magic;
	unsigned int	hcrc;
	unsigned int	sfdpcrc;
	unsigned int	version;

	unsigned char	name[DA16X_IH_NMLEN];
	unsigned int	datsiz;
	unsigned int	datcrc;

	unsigned int	dbgkey    :  1;
	unsigned int	keysize   : 15;
	unsigned int	keynum    :  8;
	unsigned int	loadscheme:  4;
	unsigned int	encrscheme:  4;
	unsigned int	ccrc;
	unsigned int	csize;
	unsigned int	cpoint;
} DA16X_IMGHEADER_TYPE;


typedef	struct	{
	unsigned int	tag;
	unsigned int	length;
	unsigned int	loadaddr;
	unsigned int	chksum;
}	DA16X_RTMHEADER_TYPE;

/******************************************************************************
 *
 *
 *
 ******************************************************************************/

extern HANDLE	flash_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker);
extern void		flash_image_close(HANDLE handler);
extern UINT32	flash_image_check(HANDLE handler, UINT32 imgtype, UINT32 imghdr_offset);
extern UINT32	flash_image_test(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr);
extern void		*flash_image_certificate(HANDLE handler, UINT32 imghdr_offset, UINT32 certindex, UINT32 *certsize);
extern UINT32	flash_image_extract(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr);
extern UINT32	flash_image_load(HANDLE handler, UINT32 imghdr_offset, UINT32 *load_addr, UINT32 *jmp_addr);
extern UINT32	flash_image_write(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize);
extern UINT32	flash_image_read(HANDLE handler, UINT32 img_offset, VOID *load_addr, UINT32 img_secsize);
extern UINT32	flash_image_erase(HANDLE handler, UINT32 img_offset, UINT32 img_size);
extern UINT32	flash_image_force(HANDLE handler, UINT32 force);

/******************************************************************************
 *
 *
 *
 ******************************************************************************/

extern HANDLE	retm_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker);
extern void		retm_image_close(HANDLE handler);
extern UINT32	retm_image_test(HANDLE handler, UINT32 *jmp_addr);
extern UINT32	retm_image_load(HANDLE handler, UINT32 *jmp_addr);

#define INDEPENDENT_FLASH_SIZE

#ifdef INDEPENDENT_FLASH_SIZE

#define IMAGE0_INDEX						0
#define IMAGE1_INDEX						1
#define DEFAULT_OFFSET				IMAGE0_INDEX
#define DEFAULT_OFFSET_ADDRESS		DA16X_FLASH_RTOS_MODEL0
#define UNKNOWN_OFFSET_ADDRESS		0xFF

#define OFFSET_MAGIC_CODE	0xDEADBEAF
#define SFLASH_BOOT_OFFSET	SFLASH_BOOT_INDEX_BASE
typedef struct boot_offset {
    UINT    offset_magic;
    UINT    offset_index;
    UINT    offset_address;
} BOOT_OFFSET_TYPE;

extern void write_boot_offset(int offset, int address);
extern int read_boot_offset(int    *address);
#endif

#ifdef __TIM_LOAD_FROM_IMAGE__
extern const unsigned char rlib_image[];
extern const unsigned char tim_image[];
#endif	/* __TIM_LOAD_FROM_IMAGE__ */

#endif	/*__da16x_image_h__*/

/* EOF */
