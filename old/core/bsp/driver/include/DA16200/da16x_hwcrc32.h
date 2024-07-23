/**
 * \addtogroup HALayer
 * \{
 * \addtogroup HW_CRC32	HW CRC-32
 * \{
 * \brief HW CRC-32 with a polynomial 0xEDB88320 that is used to validate a image
 */
 
/**
 ****************************************************************************************
 *
 * @file da16x_hwcrc32.h
 *
 * @brief HWCrc32
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


#ifndef __da16x_hwcrc32_h__
#define __da16x_hwcrc32_h__

#include "hal.h"

//------------------------------------------------------------------------------
// CRC32 : mode
//------------------------------------------------------------------------------

#define	CRC_CHK_MST		(1<<25)
#define	CRC_CHK_ADDR		(1<<24)
#define	CRC_MST_TYPE(x)		(((x)&0x1f)<<16)
#define	CRC_SWAP_MODE(x)	(((x)&0x1)<<13)
#define	CRC_ENDIAN(x)		(((x)&0x1)<<12)
#define	CRC_DAT_TYPE(x)		(((x)&0x1)<<11)
#define	CRC_PAR_TYPE(x)		(((x)&0x03)<<9)
#define	CRC_ACC_TYPE(x)		(((x)&0x01)<<8)
#define	CRC_PATH_SEL(x)		((x)&0x1f)

#define	CRC_CTRL_CACHE		0x800
#define	CRC_CTRL_DWMASK		0x0FF
//------------------------------------------------------------------------------
// CRC32
//	polynomial 0xEDB88320
//	x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1
//------------------------------------------------------------------------------

extern void	da16x_crc32_start(UINT32 mode, UINT32 startaddr, UINT32 endaddr,  UINT32 seed);
extern void	da16x_crc32_restart(void);
extern UINT32	da16x_crc32_stop(void);
extern UINT32	da16x_crc32_pause(void);
extern void	da16x_crc32_pseudo8(UINT8 *value, UINT32 length);
extern void	da16x_crc32_pseudo16(UINT16 *value, UINT32 length);
extern void	da16x_crc32_pseudo32(UINT32 *value, UINT32 length);

/**
 * \brief Calcurate CRC32
 *
 * \param [in] dwidth		data access size (sizeof(UINT8), sizeof(UINT16), sizeof(UINT32))
 * \param [in] data			source data buffer
 * \param [in] length		source data length
 * \param [in] seed			seed value for CRC32
 *
 * \return	CRC32 			CRC32 value
 *
 * \note  This functions is designed in combination with HWCRC32 and FDMA to speed up the operation. \n
 * 		  (Reserved channel id, 0xFDC0xxxx  in \ref  FDMA)
 */
extern UINT32	da16x_hwcrc32(UINT32 dwidth, UINT8 *data, UINT32 length, UINT32 seed);
/**
 * \brief Calcurate CRC32 and copy data conconrrently.
 *
 * \param [in] dwidth		data access size (sizeof(UINT8), sizeof(UINT16), sizeof(UINT32))
 * \param [in] dst			destination data buffer
 * \param [in] src			source data buffer
 * \param [in] length		source data length
 * \param [in] seed			seed value for CRC32
 *
 * \return	CRC32 			CRC32 value
 *
 * \note  This functions is designed in combination with HWCRC32 and FDMA to speed up the operation. \n
 * 		  (Reserved channel id, 0xFDC0xxxx  in \ref  FDMA)
 */
extern UINT32	da16x_hwcopycrc32(UINT32 dwidth, UINT8 *dst, UINT8 *src, UINT32 length, UINT32 seed);

#endif /* __da16x_hwcrc32_h__ */
/**
 * \}
 * \}
 */
