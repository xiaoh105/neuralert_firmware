/**
 ****************************************************************************************
 *
 * @file sys_cfg.h
 *
 * @brief NVRAM Driver
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

#ifndef __SYSTEM_CONFIG_H__
#define __SYSTEM_CONFIG_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "da16x_system.h"

#include "nvedit.h"

//---------------------------------------------------------
// FLASH Endurance Management
//---------------------------------------------------------

// FLASH's Endurance Cycle is typically 100,000.
#if 0	/* by Bright */
#define	NVCFG_FLASH_PROTECT	10000		/* erase & program cycles, 0 : disable,  65535 : max */
#else
#define	NVCFG_FLASH_PROTECT	0		/* Protect disable */
#endif	/* 0 */

#if 0	/* by SG-SDT */
	#if	defined(SUPPORT_NVRAM_NOR)
		#define	NVCFG_FLASH_SECSIZE	(64*1024*2)	/* Max NVRAM Size */
	#else	//defined(SUPPORT_NVRAM_SFLASH)
		#define	NVCFG_FLASH_SECSIZE	(4*1024)	/* Max NVRAM Size */
	#endif

	#if	(NVEDIT_WORBUF_MAX > NVCFG_FLASH_SECSIZE)
		#define	NVCFG_FLASH_SECSIZE	NVEDIT_WORBUF_MAX
	#endif	//(NVEDIT_WORBUF_MAX > NVCFG_FLASH_OFFSET)
#else

#if	(NVEDIT_WORBUF_DEFAULT > NVCFG_FLASH_SECSIZE)
	#define	NVCFG_FLASH_SECSIZE	NVEDIT_WORBUF_DEFAULT
#else
	#if	defined(SUPPORT_NVRAM_NOR)
		#define	NVCFG_FLASH_SECSIZE	(64*1024*2)	/* Max NVRAM Size */
	#else	//defined(SUPPORT_NVRAM_SFLASH)
		#define	NVCFG_FLASH_SECSIZE	(4*1024)	/* Max NVRAM Size */
	#endif
#endif	//(NVEDIT_WORBUF_DEFAULT > NVCFG_FLASH_OFFSET)
#endif

#define	NVCFG_FLASH_SECNUM	1		/* Max NVRAM Storage */
#define	NVCFG_INIT_SECSIZE	NVCFG_FLASH_SECSIZE

//---------------------------------------------------------
// System Config Table
//---------------------------------------------------------

typedef	struct	__sys_config_table__ 	SYS_CONFIG_TYPE;
typedef	struct	__sys_config_item__ 	SYS_CONFIG_ITEM;


//---------------------------------------------------------
//
//---------------------------------------------------------

struct	__sys_config_item__ {
	char	*name;
	UINT32	dsiz;
	VOID	*data;
};


struct	__sys_config_table__ {
	// boot
	char	boot_chip[16];
	// boot.clock
	UINT32	boot_clk_bus;
	// boot.con
	UINT32	boot_con_baudrate;
	// boot.auto
	UINT32	boot_auto_base;
#if	(NVCFG_FLASH_PROTECT != 0)
	UINT16	boot_auto_protect;	// auto managment, do not access !!
	UINT16	boot_auto_nvindex;	// auto managment, do not access !!
#endif	//(NVCFG_FLASH_PROTECT != 0)
	/* user defined parameters */
	// OS parameter

	// tail
};

//---------------------------------------------------------
//
//---------------------------------------------------------

extern SYS_CONFIG_TYPE	sys_cfg_tab;

//---------------------------------------------------------
// CONFIG Daemon
//---------------------------------------------------------

extern UINT32	system_config_create(UINT32 nvcfgsize);
extern UINT32	system_config_init(UINT32 autocreate);
extern UINT32	system_config_close(void);

//---------------------------------------------------------
//
//---------------------------------------------------------

extern int	SYS_NVEDIT_IOCTL(UINT32 cmd , VOID *data );
extern int	SYS_NVEDIT_READ(UINT32 addr, VOID *p_data, UINT32 p_dlen);
extern int	SYS_NVEDIT_WRITE(UINT32 addr, VOID *p_data, UINT32 p_dlen);

extern int	SYS_NVCONFIG_UPDATE(UINT32 device);
extern int	SYS_NVCONFIG_VIEW(char *token);
extern int	SYS_NVCONFIG_FIND(char *name, VOID *data);



#endif	/*__SYSTEM_CONFIG_H__*/

/* EOF */
