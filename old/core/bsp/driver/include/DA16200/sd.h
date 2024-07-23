/**
 ****************************************************************************************
 *
 * @file sd.h
 *
 * @brief SD
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

#ifndef __sd_h__
#define __sd_h__


//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------


/* SD Class #0 */
#define DIALOG_SD_SND_REL_ADDR			3
#define DIALOG_SD_SND_IF_COND			8
#define DIALOG_SD_SET_BLK_LEN			16

/* SD Class #10 */
#define DIALOG_SD_SWITCH				6

/* Commands for applications */
#define DIALOG_SD_APP_SD_STS			13
#define DIALOG_SD_APP_OP_COND          	41
#define DIALOG_SD_APP_SEND_SCR			51

#define	DIALOG_APP_CMD					55

/*
 * SCR field definitions
 */
#define SCR_SPEC_VER_0			0	/* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1			1	/* Implements system specification 1.10 */
#define SCR_SPEC_VER_2			2	/* Implements system specification 2.00-3.0X */

/*
 * SD bus widths
 */
#define SD_BUS_WIDTH_1			0
#define SD_BUS_WIDTH_4			2

/*
 * SD_SWITCH mode
 */
#define SD_SWITCH_CHECK			0
#define SD_SWITCH_SET			1

/*
 * SD_SWITCH function groups
 */
#define SD_SWITCH_GRP_ACCESS	0

/* Mode : SD_SWITCH access */

#define SD_SCR_CMD23_SUPPORT	(1 << 1)
typedef struct dialog_sd_scr {
	unsigned char		dialog_sda_vsn;
	unsigned char		dialog_sda_spec3;
	unsigned char		dialog_bus_wid;
	unsigned char		dialog_sd_cmds;
} _SD_SCR;

typedef struct dialog_sd_ssr {
	unsigned int 		bus_width;
	unsigned short		cd_type;
	unsigned int 		area_protected;
	unsigned int		spd_class;
	unsigned char		perf_mv;
	unsigned int		au;
	unsigned int		erase_to;
	unsigned int		erase_off;
} _SD_SSR;


typedef struct dialog_sd_swt_caps {
	unsigned int		dialog_hs_max_dtr;
	unsigned int		dialog_uhs_max_dtr;
	unsigned int		dialog_sd3_bus_mode;
	unsigned int		dialog_sd3_drv_type;
	unsigned int		dialog_sd3_curr_limit;
} _SD_SWITCH_CAPS;


#endif	/*__sd_h__*/

