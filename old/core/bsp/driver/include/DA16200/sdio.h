/**
 ****************************************************************************************
 *
 * @file sdio.h
 *
 * @brief Defines and macros for SDIO
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

#ifndef __sdio_h__
#define __sdio_h__


//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
#define FC9000_SUPPORT_VOLTAGE 		0x00300000


#define DIALOG_SDIO_SEND_OP_COND		5
#define SD_IO_RW_DIRECT					52

#define DIALOG_R4_MEM_PR				(1 << 27)

#define DIALOG_SDIO_RW_DIR				52
#define DIALOG_SDIO_RW_EXT				53

#define DIALOG_R5_OUT_OF_RANGE			(1 << 8)
#define DIALOG_R5_FUNC_NO				(1 << 9)
#define DIALOG_R5_ERROR					(1 << 11)

#define DIALOG_SDIO_CCCR_CCCR			0x00
#define DIALOG_SDIO_CCCR_REV_1_20		2

#define DIALOG_SDIO_CCCR_CAPS			0x08
#define DIALOG_SDIO_CCCR_CAP_SMB		0x02
#define DIALOG_SDIO_CCCR_CAP_LSC		0x40
#define DIALOG_SDIO_CCCR_CAP_4BLS		0x80

#define DIALOG_SDIO_SPEED_SHS			0x01
#define DIALOG_SDIO_FBR_BASE(y)			(0x100 * (y))
#define DIALOG_SDIO_FBR_CIS				0x09

#define DIALOG_SDIO_CCCR_IOEx			0x02
#define DIALOG_SDIO_CCCR_IORx			0x03

#define DIALOG_SDIO_SDIO_R_1_00			0
#define DIALOG_SDIO_CCCR_R_1_10			1
#define DIALOG_SDIO_CCCR_R_1_20			2

#define DIALOG_SDIO_CCCR_POWER			0x12
#define DIALOG_SDIO_PWR_SMPC					0x01
#define DIALOG_SDIO_CCCR_SPEED			0x13
#define DIALOG_SDIO_FBR_BLKSZ			0x10

typedef struct sdio_cccr {
	unsigned int		sdio_vsn;
	unsigned int		sd_vsn;
	unsigned int		multi_block:1,
						low_speed:1,
						wide_bus:1,
						high_power:1,
						high_speed:1,
						disable_cd:1;
} _SDIO_CCCR;

typedef struct sdio_cis {
	unsigned short		vendor;
	unsigned short		device;
	unsigned short		blksize;
	unsigned int		max_dtr;
	unsigned int		enable_timeout;
} _SDIO_CIS;

#endif	/*__sdio_h__*/

/* EOF */
