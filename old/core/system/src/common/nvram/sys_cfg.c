 /**
 ****************************************************************************************
 *
 * @file sys_cfg.c
 *
 * @brief System Monitor
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

#include "stdio.h"
#include "clib.h"

#include "hal.h"
#include "oal.h"
#include "driver.h"

#include "da16x_system.h"
#include "nvedit.h"
#include "sys_cfg.h"
#include "common_def.h"
#include "sys_common_features.h"
#include "da16x_compile_opt.h"

//---------------------------------------------------------

#ifdef SFLASH_SYS_CFG_DEBUGGING
#define	NVCFG_PRINTF(...)		Printf(__VA_ARGS__)
#else
#define	NVCFG_PRINTF(...)
#endif

//---------------------------------------------------------
// External Variables
//---------------------------------------------------------

SYS_CONFIG_TYPE	sys_cfg_tab = {
	//-----------------------------------------------------
	// Boot-Loader & OS Common
	//-----------------------------------------------------
	// boot
	"DA16200        ", 		//char	boot_chip[16];
	// boot.clk
	DA16X_BOOT_CLOCK,		//UINT32 boot_clk_bus;
	// boot.con
	DA16X_BAUDRATE,			//UINT32 boot_con_baudrate;
	// boot.auto
	(DA16X_CACHE_BASE),		//UINT32 boot_auto_base;

#if	(NVCFG_FLASH_PROTECT != 0)
	NVCFG_FLASH_PROTECT,		//UINT16 boot_auto_protect;
	0,				//UINT16 boot_auto_nvindex;
#endif	//(NVCFG_FLASH_PROTECT != 0)
	//-----------------------------------------------------
	// tail
	//-----------------------------------------------------
};

static const SYS_CONFIG_ITEM	sys_cfg_item[ ] = {
	//-----------------------------------------------------
	// Boot-Loader & OS Common
	//-----------------------------------------------------
	{"boot.chip",		(NVITEM_VAR_MARK|(sizeof(UINT8)*16)),	&sys_cfg_tab.boot_chip},
	{"boot.clk.bus",	(sizeof(UINT32))	, &sys_cfg_tab.boot_clk_bus 		},
	{"boot.con.baudrate",	(sizeof(UINT32))	, &sys_cfg_tab.boot_con_baudrate	},
	{"boot.auto.base",	(sizeof(UINT32))	, &sys_cfg_tab.boot_auto_base		},
#if	(NVCFG_FLASH_PROTECT != 0)
	{"boot.auto.protect",	(sizeof(UINT16))	, &sys_cfg_tab.boot_auto_protect	},
	{"boot.auto.nvindex",	(sizeof(UINT16))	, &sys_cfg_tab.boot_auto_nvindex	},
#endif	//(NVCFG_FLASH_PROTECT != 0)

	/* user defined parameters */

	//-----------------------------------------------------
	// tail
	//-----------------------------------------------------
	{	NULL,		0			, NULL					}
};

#ifdef	SUPPORT_NVEDIT

/******************************************************************************
 *
 *   Internal Variables
 *
 ******************************************************************************/

static const char	sys_boot_name[][6]= {
	"boot",
	"debug",
	"dev",
	"app",
	"\0"	// tail
};

//	NVEDIT Setup
static HANDLE sys_nvedit = NULL;
static UINT32 sys_nvedit_device ;
static UINT32 sys_nvedit_unit   ;

#if	defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NVRAM_NOR)
static const UINT32 sys_nvedit_devlist[1] = {NVEDIT_NOR};
static const UINT32 sys_nvedit_devunit[1] = {NOR_UNIT_0};
#else	//defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NVRAM_NOR)
static const UINT32 sys_nvedit_devlist[1] = {NVEDIT_SFLASH};	// 1
static const UINT32 sys_nvedit_devunit[1] = {SFLASH_UNIT_0};	// 0
#endif	//defined(BUILD_OPT_DA16200_FPGA) && defined(SUPPORT_NVRAM_NOR)

/******************************************************************************
 *   system_config_create( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	system_config_create(UINT32 nvcfgsize)
{
	// NVEDIT
	sys_nvedit = NVEDIT_CREATE(NVEDIT_0);
	if(sys_nvedit != NULL){
		UINT32	ioctldata[1];

		ioctldata[0] = nvcfgsize;
		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_SETSIZE, ioctldata);
		NVEDIT_INIT(sys_nvedit);
	}

	return TRUE;
}

/******************************************************************************
 *   system_config_init( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32	system_config_subinit(UINT32 flag);

UINT32	system_config_init(UINT32 autocreate)
{
	DA16X_UNUSED_ARG(autocreate);

#if	(NVCFG_FLASH_PROTECT != 0)
	if( system_config_subinit(0) == TRUE ){

		NVCFG_PRINTF("Mirror NVCFG condition - %d, %d \r\n"
				, sys_cfg_tab.boot_auto_nvindex
				, sys_cfg_tab.boot_auto_protect );

		if( (sys_cfg_tab.boot_auto_nvindex % NVCFG_FLASH_SECNUM) != 0){
			UINT32 nvcfgoffset;
			UINT32 ioctldata[3];

			ioctldata[0] = sys_nvedit_device; // dev_type
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_GETCOUNT, ioctldata);

			nvcfgoffset = sys_cfg_tab.boot_auto_nvindex ;
			nvcfgoffset = nvcfgoffset % NVCFG_FLASH_SECNUM;
			nvcfgoffset = nvcfgoffset * NVCFG_FLASH_SECSIZE;

			nvcfgoffset = ioctldata[1] + nvcfgoffset; // base+offset

			NVCFG_PRINTF("Mirror NVCFG search - (%d) %x \r\n"
					, sys_nvedit_device, nvcfgoffset);

			// change NVCFG offset
			ioctldata[0] = sys_nvedit_device;
			ioctldata[1] = nvcfgoffset;
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CHANGE_BASE, ioctldata);

			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CLEAR, NULL);

			return system_config_subinit(1);
		}
		else if ( autocreate == TRUE
		    && sys_cfg_tab.boot_auto_nvindex == 0
		    && sys_cfg_tab.boot_auto_protect == NVCFG_FLASH_PROTECT){

			NVCFG_PRINTF("NVCFG auto creation - (%d)\r\n"
					, sys_nvedit_device);

			sys_cfg_tab.boot_auto_protect --;
			SYS_NVCONFIG_UPDATE(sys_nvedit_device);
		}

		return TRUE;
	}
	return FALSE;
#else	//(NVCFG_FLASH_PROTECT != 0)
	return system_config_subinit(0);
#endif	//(NVCFG_FLASH_PROTECT != 0)
}

#ifdef	SUPPORT_NVEDIT_FULL
static UINT32	system_config_searchnvcfg(UINT32 *ioctldata, UINT32 *datlen)
{
	//ioctldata[0] = sys_nvedit_device;
	//ioctldata[1] = sys_nvedit_unit; /* UNIT_0 */
	NVCFG_PRINTF(" [%s] NVEDIT_CMD_LOAD  %d,%d \r\n", __func__, ioctldata[0], ioctldata[1]);
	SYS_NVEDIT_IOCTL(NVEDIT_CMD_LOAD, ioctldata);

	ioctldata[0] = (UINT32)&(sys_boot_name[0][0]);
	ioctldata[1] = (UINT32)NULL;
	ioctldata[2] = (UINT32)0;

	*datlen = 0;

	/* find a key */
	NVCFG_PRINTF(" [%s] NVEDIT_CMD_FIND  0x%x \r\n", __func__, ioctldata[0]);
	if (SYS_NVEDIT_IOCTL(NVEDIT_CMD_FIND, ioctldata) == TRUE) {
		if((VOID *)(ioctldata[1]) == NULL){
			NVCFG_PRINTF(" [%s] NVEDIT_CMD_FIND error \r\n", __func__);
			/* if key node is not exist */
			return FALSE;
		}
		else {
			*datlen = ioctldata[2];
		}
	}
	return TRUE;
}
#else	//SUPPORT_NVEDIT_FULL
static UINT32	system_config_searchnvcfg(UINT32 *ioctldata, UINT32 *datlen)
{
	//ioctldata[0] = sys_nvedit_devlist[devidx];
	//ioctldata[1] = sys_nvedit_devunit[devidx];
	ioctldata[2] = (UINT32)&(sys_boot_name[0][0]);
	ioctldata[3] = (UINT32)datlen;
	ioctldata[4] = (UINT32)NULL;

	return NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEFIND, ioctldata);
}
#endif	//SUPPORT_NVEDIT_FULL

static UINT32	system_config_subinit(UINT32 flag)
{
	// NVEDIT
	NVCFG_PRINTF("subinit: SYS_NVEDIT = %p , flag = %d\r\n", sys_nvedit, flag);

	if (sys_nvedit != NULL) {
		volatile unsigned int idx , devidx ;
		UINT32	ioctldata[5], datlen;

		idx = 0;
		devidx = 0;

		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_INIT, NULL);
		NVCFG_PRINTF(" [%s] >> done cmd_init=%p , cmd=%d\r\n", __func__, sys_nvedit, NVEDIT_CMD_INIT);

		// Check config
		if ( flag == 0 ) {
			for (devidx = 0; devidx < (sizeof(sys_nvedit_devlist)/sizeof(UINT32)); devidx++) {
				ioctldata[0] = sys_nvedit_devlist[devidx];
				ioctldata[1] = sys_nvedit_devunit[devidx]; /* UNIT_0 */

				NVCFG_PRINTF(" [%s] >> Start system_config_searchnvcfg devlist:%d devunit:%d \r\n", __func__, ioctldata[0], ioctldata[1]);
				if ( system_config_searchnvcfg(ioctldata, &datlen) == TRUE ){
					/* if key node is exist */
					NVCFG_PRINTF("subinit: 1st, key node is exist\r\n");
					break;
				}
				NVCFG_PRINTF(" [%s] >> Done system_config_searchnvcfg \r\n", __func__);
			}

			if(devidx == (sizeof(sys_nvedit_devlist)/sizeof(UINT32))){
				NVCFG_PRINTF("subinit: 1st, key node is not exist\r\n");
				return FALSE;
			}

			sys_nvedit_device = sys_nvedit_devlist[devidx];
			sys_nvedit_unit   = sys_nvedit_devunit[devidx];
		}
		else {
				ioctldata[0] = sys_nvedit_device;
				ioctldata[1] = sys_nvedit_unit; /* UNIT_0 */

				if( system_config_searchnvcfg(ioctldata, &datlen) == FALSE ){
					/* if key node is not exist */
					NVCFG_PRINTF("subinit: 2nd, key node is not exist\r\n");
					return FALSE;
				}
		}

#ifdef	SUPPORT_NVEDIT_FULL
		// Load config
		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){

			ioctldata[0] = (UINT32)sys_cfg_item[idx].name;

			if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_FIND, ioctldata) == TRUE){
				NVEDIT_READ(sys_nvedit, ioctldata[1], sys_cfg_item[idx].data, ioctldata[2]);
				/* Don't delete ... Timming issue */
				//Printf(" node found, %s \r\n", sys_cfg_item[idx].name);
			}
			else {
				/* Don't delete ... Timming issue */
				//Printf(" node NOT found, %s \r\n", sys_cfg_item[idx].name);
			}
		}
#else	//SUPPORT_NVEDIT_FULL
		// Load config
		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){

			ioctldata[0] = sys_nvedit_device;
			ioctldata[1] = sys_nvedit_unit;
			ioctldata[2] = (UINT32)sys_cfg_item[idx].name;
			ioctldata[3] = (UINT32)&datlen;
			ioctldata[4] = (UINT32)sys_cfg_item[idx].data;

			if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEFIND, ioctldata) == TRUE){
				//NVCFG_PRINTF("node found, %s\r\n", sys_cfg_item[idx].name);
			}else{
				//NVCFG_PRINTF("node NOT found, %s\r\n", sys_cfg_item[idx].name);
			}
		}
#endif	//SUPPORT_NVEDIT_FULL
	}

	return TRUE;
}

/******************************************************************************
 *   system_config_close( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	system_config_close(void)
{
	// NVEDIT
	if(sys_nvedit != NULL){
		NVEDIT_CLOSE(sys_nvedit);
		sys_nvedit = NULL;
	}

	return TRUE;
}


/******************************************************************************
 *   SYS_NVEDIT_IOCTL( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#if	(NVCFG_FLASH_PROTECT != 0)
static int system_config_management(UINT32 cmd , VOID *data )
{
	UINT32 ioctldata[5];

	ioctldata[0] = ((UINT32 *)data)[0]; // dev_type

	if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_GETCOUNT, ioctldata) == TRUE ){
		UINT32 idx, newoffset, newprotect, newnvindex;
		UINT32 updateprotect, saveprotect, writelen;

		newoffset = (sys_cfg_tab.boot_auto_nvindex % NVCFG_FLASH_SECNUM);
		newoffset = newoffset * NVCFG_FLASH_SECSIZE;
		newoffset = ioctldata[1] - newoffset; // base

		// 1. Update boot_auto_protect
		saveprotect = ioctldata[2];

		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++ ){
			if( sys_cfg_item[idx].data == &(sys_cfg_tab.boot_auto_protect) ){
				break;
			}
		}

		if( sys_cfg_item[idx].name == NULL ){
			NVCFG_PRINTF("protect not registered - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

#ifdef	SUPPORT_NVEDIT_FULL
		ioctldata[0] = (UINT32)sys_cfg_item[idx].name ;
		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_FIND, ioctldata) == FALSE){
			NVCFG_PRINTF("protect not found - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

		NVEDIT_READ(sys_nvedit, ioctldata[1], sys_cfg_item[idx].data, ioctldata[2]);

		saveprotect = saveprotect + 1;
		newprotect = (sys_cfg_tab.boot_auto_protect - saveprotect);

		if( newprotect >= NVCFG_FLASH_PROTECT ){
			newprotect = 0;
		}

		updateprotect = (newprotect == 0) ? NVCFG_FLASH_PROTECT : newprotect ;
		writelen = (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) ;

		if( NVEDIT_WRITE(sys_nvedit
			, ioctldata[1]
			, (VOID *)&updateprotect
			, writelen ) > 0 ){ // + '\0'
			NVCFG_PRINTF("boot_auto_protect updated (%d, %d, %d)\r\n"
					, newprotect
					, saveprotect
					, updateprotect
					);
		}else{
			NVCFG_PRINTF("boot_auto_protect failed \r\n");
		}
#else	//SUPPORT_NVEDIT_FULL
		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = sys_nvedit_unit;
		ioctldata[2] = (UINT32)sys_cfg_item[idx].name;
		ioctldata[3] = (UINT32)(&writelen);
		ioctldata[4] = (UINT32)sys_cfg_item[idx].data;

		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEFIND, ioctldata) == FALSE){
			NVCFG_PRINTF("protect not found - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

		saveprotect = saveprotect + 1;
		newprotect = (sys_cfg_tab.boot_auto_protect - saveprotect);

		if( newprotect >= NVCFG_FLASH_PROTECT ){
			newprotect = 0;
		}

		updateprotect = (newprotect == 0) ? NVCFG_FLASH_PROTECT : newprotect ;
		writelen = (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) ;

		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = sys_nvedit_unit;
		ioctldata[2] = (UINT32)sys_cfg_item[idx].name;
		ioctldata[3] = (UINT32)(&writelen);
		ioctldata[4] = (UINT32)&updateprotect;

		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEUPDATE, ioctldata) == FALSE){
			NVCFG_PRINTF("protect not updated - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}else{
			NVCFG_PRINTF("protect updated (%d, %d, %d)\r\n"
				, newprotect
				, saveprotect
				, updateprotect
				);
		}
#endif	//SUPPORT_NVEDIT_FULL

		sys_cfg_tab.boot_auto_protect = updateprotect;

		// if counter doesn't reaches a threshold
		if(newprotect != 0){
			// 1.1. User Action
			NVEDIT_IOCTL(sys_nvedit, cmd, data);

			// 1.2. Clear Counter
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_RESETCOUNT, NULL);

			return TRUE;
		}

		// if counter reaches a threshold
		// 2. Update boot_auto_nvindex
		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++ ){
			if( sys_cfg_item[idx].data == &(sys_cfg_tab.boot_auto_nvindex) ){
				break;
			}
		}

		if( sys_cfg_item[idx].name == NULL ){
			NVCFG_PRINTF("nvindex not registered - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

		// 3. Upate Master NVCFG

#ifdef	SUPPORT_NVEDIT_FULL
		ioctldata[0] = (UINT32)sys_cfg_item[idx].name ;
		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_FIND, ioctldata) == FALSE){
			NVCFG_PRINTF("nvindex not found - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

		NVEDIT_READ(sys_nvedit, ioctldata[1], sys_cfg_item[idx].data, ioctldata[2]);

		newnvindex = sys_cfg_tab.boot_auto_nvindex + 1;
		writelen = (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) ;

		if( NVEDIT_WRITE(sys_nvedit
			, ioctldata[1]
			, (VOID *)&newnvindex
			, writelen ) > 0 ){ // + '\0'
			NVCFG_PRINTF("boot_auto_nvindex updated (%d)\r\n", newnvindex);
		}else{
			NVCFG_PRINTF("boot_auto_nvindex failed \r\n");
		}

		if( (newnvindex % NVCFG_FLASH_SECNUM) != 0 ){
			// 4. Change offset to Master NVCFG
			ioctldata[0] = sys_nvedit_device;
			ioctldata[1] = newoffset;
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CHANGE_BASE, ioctldata);

			// 5. Save Master NVCFG
			ioctldata[0] = sys_nvedit_device;
			ioctldata[1] = sys_nvedit_unit;

			if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_SAVE, ioctldata) != TRUE){
				NVCFG_PRINTF("Master NVCFG failed \r\n");
				return FALSE;
			}

			NVCFG_PRINTF("Master NVCFG updated - (%d) %x \r\n"
					, sys_nvedit_device, newoffset);
		}
#else	//SUPPORT_NVEDIT_FULL
		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = sys_nvedit_unit;
		ioctldata[2] = (UINT32)sys_cfg_item[idx].name;
		ioctldata[3] = (UINT32)(&writelen);
		ioctldata[4] = (UINT32)sys_cfg_item[idx].data;

		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEFIND, ioctldata) == FALSE){
			NVCFG_PRINTF("protect not found - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}

		newnvindex = sys_cfg_tab.boot_auto_nvindex + 1;
		writelen = (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) ;

		// 4. Change offset to Master NVCFG
		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = newoffset;
		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CHANGE_BASE, ioctldata);

		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = sys_nvedit_unit;
		ioctldata[2] = (UINT32)sys_cfg_item[idx].name;
		ioctldata[3] = (UINT32)(&writelen);
		ioctldata[4] = (UINT32)&newnvindex;

		if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LITEUPDATE, ioctldata) == FALSE){
			NVCFG_PRINTF("nvindex not updated - %s\r\n", sys_cfg_item[idx].name );
			return FALSE; // mismatch
		}else{
			NVCFG_PRINTF("nvindex updated (%d)\r\n", newnvindex);
		}

		NVCFG_PRINTF("Master NVCFG updated - (%d) %x \r\n"
				, sys_nvedit_device, newoffset);
#endif	//SUPPORT_NVEDIT_FULL

		sys_cfg_tab.boot_auto_nvindex ++;

		if( (newnvindex % NVCFG_FLASH_SECNUM) == 0 ){
			NVCFG_PRINTF("Master equals Mirror - (%d) %x \r\n"
					, sys_nvedit_device, newoffset);
		}

		// 4. Change offset to Mirror NVCFG

		ioctldata[0] = sys_nvedit_device;
		ioctldata[1] = newoffset ; // base

		newoffset = (sys_cfg_tab.boot_auto_nvindex % NVCFG_FLASH_SECNUM);
		newoffset = newoffset * NVCFG_FLASH_SECSIZE;
		ioctldata[1] = ioctldata[1] + newoffset; // base+offset

		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CHANGE_BASE, ioctldata);

		NVCFG_PRINTF("Mirror NVCFG changed - (%d) %x \r\n"
				, sys_nvedit_device, ioctldata[1]);

		// 7. User Action
		NVEDIT_IOCTL(sys_nvedit, cmd, data);

		// 8. Clear Counter
		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_RESETCOUNT, NULL);

                return TRUE;
	}

	return FALSE;
}

static int system_erase_management(VOID *data)
{
	UINT32 ioctldata[5];

	ioctldata[0] = ((UINT32 *)data)[0]; // dev_type

	if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_GETCOUNT, ioctldata) == TRUE ){
		UINT32 idx, newoffset;

		newoffset = (sys_cfg_tab.boot_auto_nvindex % NVCFG_FLASH_SECNUM);
		newoffset = newoffset * NVCFG_FLASH_SECSIZE;
		newoffset = ioctldata[1] - newoffset; // base

		idx = NVCFG_FLASH_SECNUM;
		while( idx > 0 ){
			idx --;
			ioctldata[1] = newoffset + idx * NVCFG_FLASH_SECSIZE; // base+offset
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CHANGE_BASE, ioctldata);

			NVCFG_PRINTF("NVCFG erase - %x \r\n", ioctldata[1]);

			ioctldata[1] = ((UINT32 *)data)[1]; // dev_unit
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_ERASE, ioctldata);
		}

		sys_cfg_tab.boot_auto_nvindex = 0;

		return TRUE;
	}

	return FALSE;
}
#endif	//(NVCFG_FLASH_PROTECT != 0)

int	SYS_NVEDIT_IOCTL(UINT32 cmd , VOID *data )
{
	if( sys_nvedit == NULL)
		return FALSE;

#if	(NVCFG_FLASH_PROTECT != 0)
	if ( cmd == NVEDIT_CMD_SAVE || cmd == NVEDIT_CMD_LITEUPDATE ) {
		if ( system_config_management(cmd, data) == TRUE ){
			return TRUE;
		}
	}
	else if( cmd == NVEDIT_CMD_ERASE ) {
		if ( system_erase_management(data) == TRUE ) {
			return TRUE;
		}
	}
#endif	//(NVCFG_FLASH_PROTECT != 0)

	return NVEDIT_IOCTL(sys_nvedit, cmd, data);
}

/******************************************************************************
 *   SYS_NVEDIT_READ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SYS_NVEDIT_READ(UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	if( sys_nvedit == NULL) return FALSE;

	return NVEDIT_READ(sys_nvedit, addr, p_data, p_dlen);
}

/******************************************************************************
 *   SYS_NVEDIT_WRITE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SYS_NVEDIT_WRITE(UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	if( sys_nvedit == NULL) return FALSE;

	return NVEDIT_WRITE(sys_nvedit, addr, p_data, p_dlen);
}

/******************************************************************************
 *   SYS_NVCONFIG_UPDATE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_NVEDIT_FULL
static	void system_config_subadd(char *fulltoken, SYS_CONFIG_ITEM *item)
{
	char	*nameoffset, *token;
	UINT32	subiodata[4], fidx;

	nameoffset = item->name;
	fidx = 0 ;

	// check boot node
	// extract token
	for( fidx = 0 ; *nameoffset != '\0' && *nameoffset != '.' ; fidx++, nameoffset++ ){
		fulltoken[fidx] = *nameoffset;
	}

	fulltoken[fidx] = '\0';

	if(*nameoffset != '\0'){
		nameoffset++;
		fidx ++;
	}

	// check sub node
	while(*nameoffset != '\0'){
		// extract token
		token = &(fulltoken[fidx]);

		for( ; *nameoffset != '\0' && *nameoffset != '.' ; fidx++, nameoffset++ ){
			fulltoken[fidx] = *nameoffset;
		}
		fulltoken[fidx++] = '\0';

		// add item
		if(*nameoffset != '\0'){
			nameoffset++;
			// add node
			subiodata[0] = (UINT32)fulltoken;
			subiodata[1] = (UINT32)token;
			subiodata[2] = NVITEM_NOD_MARK;
			subiodata[3] = (UINT32)NULL;

			NVCFG_PRINTF("add-node %s, %s\r\n", fulltoken, token);

			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_ADD, subiodata);
		}else{
			// add value
			NVCFG_PRINTF("add-value %s, %s\r\n", fulltoken, token);

			subiodata[0] = (UINT32)fulltoken;
			subiodata[1] = (UINT32)token;
			subiodata[2] = item->dsiz;
			subiodata[3] = (UINT32)item->data;
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_ADD, subiodata);
		}

		// post processing
		if(*nameoffset != '\0'){
			*((char *)token-1) = '.';
		}
	}
}
#endif	//SUPPORT_NVEDIT_FULL

int	SYS_NVCONFIG_UPDATE(UINT32 device)
{
	UINT32	ioctldata[5], devidx, idx;
	char	*fulltoken;

	if( sys_nvedit == NULL) return FALSE;

	// Search Device
	for(devidx = 0; devidx < (sizeof(sys_nvedit_devlist)/sizeof(UINT32)); devidx++){
		if( device == sys_nvedit_devlist[devidx] ){
			break;
		}
	}
	if(devidx == (sizeof(sys_nvedit_devlist)/sizeof(UINT32))){
		return FALSE;
	}

	fulltoken = (char *)pvPortMalloc(NVITEM_NAME_MAX+1);
	if( fulltoken == NULL ){
		return FALSE;
	}
	DRIVER_MEMSET(fulltoken, 0x00, NVITEM_NAME_MAX+1);

	// Load
	ioctldata[0] = sys_nvedit_devlist[devidx];
	ioctldata[1] = sys_nvedit_devunit[devidx];

	NVCFG_PRINTF("SYS_NVCONFIG_UPDATE (%d/%d)\r\n", sys_nvedit_devlist[devidx], sys_nvedit_devunit[devidx] );

#ifdef	SUPPORT_NVEDIT_FULL
	if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_LOAD, ioctldata) != TRUE){

		NVCFG_PRINTF("NVEDIT_CMD_LOAD failed\r\n");
		/* Clear nvram tree */
		SYS_NVEDIT_IOCTL(NVEDIT_CMD_CLEAR, NULL);

		// new config
		for(idx = 0; sys_boot_name[idx][0] != '\0'; idx++){
			ioctldata[0] = (UINT32)&(sys_boot_name[idx][0]);
			ioctldata[1] = NVITEM_NOD_MARK;
			ioctldata[2] = (UINT32)NULL;
			NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_ROOT, ioctldata);
		}

		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){
			system_config_subadd(fulltoken, (SYS_CONFIG_ITEM *)&(sys_cfg_item[idx]));
		}

	}else{
		// Update config
		NVCFG_PRINTF("NVEDIT_CMD_LOAD success\r\n");

		for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){

			ioctldata[0] = (UINT32)sys_cfg_item[idx].name ;
			if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_FIND, ioctldata) == TRUE){

				NVCFG_PRINTF("NVEDIT_CMD_FIND success - %s\r\n", sys_cfg_item[idx].name );

				if( NVEDIT_WRITE(sys_nvedit
					, ioctldata[1]
					, (VOID *)sys_cfg_item[idx].data
					, (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) ) > 0 ){
					NVCFG_PRINTF("NVEDIT_WRITE success \r\n");
				}else{
					NVCFG_PRINTF("NVEDIT_WRITE failed \r\n");
				}
			}
			else
			{
				NVCFG_PRINTF("NVEDIT_CMD_FIND failed - %s\r\n", sys_cfg_item[idx].name );

				system_config_subadd(fulltoken, (SYS_CONFIG_ITEM *)&(sys_cfg_item[idx]));
			}
		}
	}

	vPortFree(fulltoken);

#if defined(SUPPORT_NVRAM_SECU)

#ifndef __SUPPORT_SECURE_NVRAM__  
	#error	"Feature __SUPPORT_SECURE_NVRAM__ does not defined."     
#endif

#if __SUPPORT_SECURE_NVRAM__ == 0
//	#warning	"Built-in Secure NVRAM is inactive."
#elif __SUPPORT_SECURE_NVRAM__ == 1
	if(DA16X_SecureBoot_SecureLCS() == TRUE)
	{
		ioctldata[0] = ASSET_ROOT_KEY;
		SYS_NVEDIT_IOCTL( NVEDIT_CMD_SECURE, ioctldata);		
	}
#elif __SUPPORT_SECURE_NVRAM__ == 2
	if(DA16X_SecureBoot_SecureLCS() == TRUE)
	{
		ioctldata[0] = ASSET_KCP_KEY;
		SYS_NVEDIT_IOCTL( NVEDIT_CMD_SECURE, ioctldata);		
	}
#elif __SUPPORT_SECURE_NVRAM__ == 3
	if(DA16X_SecureBoot_SecureLCS() == TRUE)
	{
		ioctldata[0] = ASSET_KPICV_KEY;
		SYS_NVEDIT_IOCTL( NVEDIT_CMD_SECURE, ioctldata);		
	}
#else
	#error	"This key is not allowed in Secure NVRAM."
#endif

#endif	//defined(SUPPORT_NVRAM_SECU)

	// Save
	ioctldata[0] = sys_nvedit_devlist[devidx];
	ioctldata[1] = sys_nvedit_devunit[devidx];

	if( SYS_NVEDIT_IOCTL(NVEDIT_CMD_SAVE, ioctldata) != TRUE){
		NVCFG_PRINTF("NVEDIT_CMD_SAVE failed \r\n");
		return FALSE;
	}

	if( NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_CLEAR, NULL) != TRUE){
		NVCFG_PRINTF("NVEDIT_CMD_CLEAR failed \r\n");
		return FALSE;
	}

#else	//SUPPORT_NVEDIT_FULL

	for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){
		UINT32	subiodata[5];

		subiodata[0] = (UINT32)sys_nvedit_devlist[devidx] ;
		subiodata[1] = (UINT32)sys_nvedit_devunit[devidx] ;
		subiodata[2] = (UINT32)sys_cfg_item[idx].name ;
		subiodata[3] = (UINT32)&(sys_cfg_item[idx].dsiz) ;
		subiodata[4] = (UINT32)sys_cfg_item[idx].data ;

		if( SYS_NVEDIT_IOCTL(NVEDIT_CMD_LITEUPDATE, subiodata) == TRUE){
			NVCFG_PRINTF("NVEDIT_CMD_LITEUPDATE success - %s\r\n", sys_cfg_item[idx].name );
		}
		else
		{
			NVCFG_PRINTF("NVEDIT_CMD_LITEUPDATE failed - %s\r\n", sys_cfg_item[idx].name );
		}
	}

	vPortFree(fulltoken);

#endif	//SUPPORT_NVEDIT_FULL

	// Clear
	for(idx = 0; sys_boot_name[idx][0] != '\0'; idx++){
		ioctldata[0] = (UINT32) &(sys_boot_name[idx][0]) ;
		ioctldata[1] = FALSE;
		NVEDIT_IOCTL(sys_nvedit, NVEDIT_CMD_DEL, ioctldata);
		NVCFG_PRINTF("Clear - %s\r\n", sys_boot_name[idx] );
	}

	return TRUE;
}

#else	//SUPPORT_NVEDIT

UINT32	system_config_create(UINT32 nvcfgsize)
{
	return TRUE;
}

UINT32	system_config_init(UINT32 autocreate)
{
	return TRUE;
}

UINT32	system_config_close(void)
{
	return TRUE;
}

int	SYS_NVEDIT_IOCTL(UINT32 cmd , VOID *data )
{
	return TRUE;
}

int	SYS_NVEDIT_READ(UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	return TRUE;
}

int	SYS_NVEDIT_WRITE(UINT32 addr, VOID *p_data, UINT32 p_dlen)
{
	return TRUE;
}

int	SYS_NVCONFIG_UPDATE(UINT32 device)
{
	return TRUE;
}

#endif	//SUPPORT_NVEDIT

/******************************************************************************
 *   SYS_NVCONFIG_VIEW( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SYS_NVCONFIG_VIEW(char *token)
{
	int idx;

	for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){
		if( token != NULL ){
			if( DRIVER_STRNCMP(token, sys_cfg_item[idx].name, DRIVER_STRLEN(token)) != 0 ){
				continue;
			}
		}

		switch((sys_cfg_item[idx].dsiz & NVITEM_TYPE_MASK)){

		case NVITEM_VAR_MARK:
			PRINTF("%s - str (%d), \"%s\"\r\n", sys_cfg_item[idx].name
					, (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK)
					, ((char *)sys_cfg_item[idx].data)
				);
			break;
		case NVITEM_NOD_MARK:
			PRINTF("%s - node, (%d)\r\n", sys_cfg_item[idx].name
				, (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK)
			);
			break;
		default:
			switch((sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK)){

			case sizeof(UINT8):
				PRINTF("%s - u8, %c (%x)\r\n", sys_cfg_item[idx].name
					, *((char *)sys_cfg_item[idx].data)
					, *((char *)sys_cfg_item[idx].data)
				);
				break;
			case sizeof(UINT16):
				PRINTF("%s - u16, %d (%x)\r\n", sys_cfg_item[idx].name
					, *((UINT16 *)sys_cfg_item[idx].data)
					, *((UINT16 *)sys_cfg_item[idx].data)
				);
				break;
			case sizeof(UINT32):
				PRINTF("%s - u32, %d (%x)\r\n", sys_cfg_item[idx].name
					, *((UINT32 *)sys_cfg_item[idx].data)
					, *((UINT32 *)sys_cfg_item[idx].data)
				);
				break;
			default:
				PRINTF("%s - unknown, (%d)\r\n", sys_cfg_item[idx].name
					, (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK)
				);
				break;
			}
			break;
		}
	}

	return TRUE;
}

/******************************************************************************
 *   SYS_NVCONFIG_FIND( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SYS_NVCONFIG_FIND(char *name, VOID *data)
{
	int idx;

	for(idx = 0; sys_cfg_item[idx].name != NULL; idx++){
		if( DRIVER_STRCMP(sys_cfg_item[idx].name, name) == 0){
			if( data == NULL ){
				data = sys_cfg_item[idx].data ;
				return sys_cfg_item[idx].dsiz ;
			}else{
				DRIVER_MEMCPY( data, sys_cfg_item[idx].data, (sys_cfg_item[idx].dsiz & NVITEM_LEN_MASK) );
				return sys_cfg_item[idx].dsiz ;
			}
		}
	}
	//data = NULL;
	return 0;
}

/******************************************************************************
 *   SYS_OTP_MAC_ADDR_WRITE( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#define OTP_MAC_AVAILABLE	4
#define OTP_MAC_ADDR1		0x100
#define OTP_MAC_ADDR2		0x102
#define OTP_MAC_ADDR3		0x104
#define OTP_MAC_ADDR4		0x106
#define OTP_MAC_ADDR5		0x108

#define __OTP_MAC_ADDR__

extern void	DA16X_SecureBoot_OTPLock(UINT32 mode);
int	SYS_OTP_MAC_ADDR_WRITE(char *mac_addr)
{
#ifdef __OTP_MAC_ADDR__
	UINT32 rdata, rdata_1, wdata;
	UINT8 tmp_mac[8] = {0,}, clkbackup;
	int i, status;

	// reload otp lock status
	clkbackup = DA16X_CLOCK_SCGATE->Off_CC_OTPW;
	DA16X_CLOCK_SCGATE->Off_CC_OTPW = 0;
	DA16X_SecureBoot_OTPLock(1); // unlock
	otp_mem_create();
	otp_mem_lock_read((0x3ffc>>2), &rdata);
	otp_mem_close();

	for (i = 0; i < 6; i++) {
		tmp_mac[i] = toint(mac_addr[i*2]) << 4 | toint(mac_addr[(i*2)+1]);
	}

	otp_mem_create();

	for (i = 0; i < OTP_MAC_AVAILABLE; i++) {
		status = otp_mem_read(OTP_MAC_ADDR1+i+(i+1), &rdata);
		status |= otp_mem_read(OTP_MAC_ADDR1+i+(i), &rdata_1);


		if (status != OTP_OK)
			goto OTP_ERR;

		if ((rdata == 0) && (rdata_1 == 0))
			break;

#if 0
    	if (i != (OTP_MAC_AVAILABLE-1)) {
    		wdata = 0xffffffff;
    		status = otp_mem_write(OTP_MAC_ADDR1+(i)*2+1, wdata);

    		if (status != OTP_OK)
    			goto OTP_ERR;
    	}
#endif

	}

	if (i == OTP_MAC_AVAILABLE)
		goto OTP_ERR;

	otp_mem_close();

	otp_mem_create();
	wdata = *(UINT32 *)(&tmp_mac[0]);
	status = otp_mem_write(OTP_MAC_ADDR1+(i*2), wdata);

	if (status != OTP_OK)
		goto OTP_ERR;

	wdata = (*(UINT32 *)(&tmp_mac[4])) | 0x01000000;
	status = otp_mem_write(OTP_MAC_ADDR1+(i*2)+1, wdata);

	if (status != OTP_OK)
		goto OTP_ERR;


OTP_ERR:
	otp_mem_close();
	DA16X_CLOCK_SCGATE->Off_CC_OTPW = clkbackup;
	DA16X_SecureBoot_OTPLock(0); // unlock

	if (status == OTP_OK) {
		return 1;
	} else
#endif /* __OTP_MAC_ADDR__ */
	{
		return 0;
	}
}

/******************************************************************************
 *   SYS_OTP_MAC_ADDR_READ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int	SYS_OTP_MAC_ADDR_READ(char *mac_addr)
{
#ifdef __OTP_MAC_ADDR__
	UINT8 clkbackup, tmp_mac[17] = {0,};
	UINT32 status, swap_mac[2], rdata;
	int i;

	clkbackup = DA16X_CLOCK_SCGATE->Off_CC_OTPW;
	DA16X_CLOCK_SCGATE->Off_CC_OTPW = 0;

	otp_mem_create();

	for (i = (OTP_MAC_AVAILABLE-1); i >= 0; i--) {
		status = otp_mem_read(OTP_MAC_ADDR1+i+(i+1), &rdata);

		if (status != OTP_OK)
			goto OTP_ERR;

		if ((rdata& 0xffff0000) == 0x01000000)
			break;
	}

	if (i < 0)
		goto OTP_ERR;

	status = otp_mem_read(OTP_MAC_ADDR1+i*2, &rdata);

	if (status != OTP_OK)
		goto OTP_ERR;

	swap_mac[0] = *((UINT32*)((UINT32)&rdata + (UINT32)0x20800000));
	status = otp_mem_read(OTP_MAC_ADDR1+i*2+1, &rdata);

	if (status != OTP_OK)
		goto OTP_ERR;

	swap_mac[1] = *((UINT32*)((UINT32)&rdata + (UINT32)0x20800000));

	sprintf((char*)tmp_mac, "%08x%08x", swap_mac[0], swap_mac[1]);

	memset(mac_addr, 0x00, 13);
	memcpy(mac_addr, tmp_mac, 12);

OTP_ERR:
	otp_mem_close();
	DA16X_CLOCK_SCGATE->Off_CC_OTPW = clkbackup;
	*((volatile UINT32 *)(0x40120000)) = 0x04000000;

	if (status == OTP_OK) {
		return i;
	} else
#endif /* __OTP_MAC_ADDR__ */
	{
		return -1;
	}
}

/* EOF */
