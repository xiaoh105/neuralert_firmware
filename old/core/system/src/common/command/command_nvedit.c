/**
 ****************************************************************************************
 *
 * @file command_nvedit.c
 *
 * @brief Command Utility
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
#include "string.h"
#include "ctype.h"
#include "clib.h"

#include "oal.h"
#include "hal.h"
#include "driver.h"
#include "da16x_system.h"

#include "monitor.h"
#include "command.h"
#include "command_nvedit.h"

#include "nvedit.h"
#include "sys_cfg.h"
#include "environ.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#if	defined(SUPPORT_NVEDIT)

#undef	SUPPORT_NVEDIT_LEGACY
#undef	SUPPORT_NVEDIT_LEGACY_AGC

#ifdef	SUPPORT_NVRAM_NOR
  #define	ENVIRON_DEVICE	NVEDIT_NOR
#else	//SUPPORT_NVRAM_NOR
  #ifdef	SUPPORT_NVRAM_SFLASH
    #define	ENVIRON_DEVICE	NVEDIT_SFLASH
  #else	//SUPPORT_NVRAM_SFLASH
    #define	ENVIRON_DEVICE	NVEDIT_RAM
  #endif	//SUPPORT_NVRAM_SFLASH
#endif	//SUPPORT_NVRAM_NOR

extern int recovery_NVRAM(void);

static void extract_flash_info(char *name, UINT32 *ioctldata);

#endif  //defined(SUPPORT_NVEDIT)

extern int chk_dpm_pdown_start(void);

extern int getStr(char *get_data, int get_len);

//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_nvram_list[] = {
  { "NVRAM",		CMD_MY_NODE,	cmd_nvram_list,	NULL,		"NVRAM utilities "}	,	// Head

  { "-------",		CMD_FUNC_NODE,	NULL,	NULL,				"--------------------------------"},

#if	defined(SUPPORT_NVEDIT)
  { "nvedit",		CMD_FUNC_NODE,	NULL,	&cmd_nvedit_cmd, 	"nvedit command "					},
#ifdef	XIP_CACHE_BOOT
  { "getenv",		CMD_FUNC_NODE,	NULL,	&cmd_getenv_cmd,	"getenv [variable]"					},
  { "setenv",		CMD_FUNC_NODE,	NULL,	&cmd_setenv_cmd,	"setenv [permanent var] [string]"	},
  { "unsetenv",		CMD_FUNC_NODE,	NULL,	&cmd_unsetenv_cmd,	"unsetenv [permanent var] "			},
  { "putenv",		CMD_FUNC_NODE,	NULL,	&cmd_setenv_cmd,	"putenv [permanent var]"			},
  { "clearenv",		CMD_FUNC_NODE,	NULL,	&cmd_unsetenv_cmd,	"clearenv"							},
  { "printenv",		CMD_FUNC_NODE,	NULL,	&cmd_getenv_cmd,	"printenv [variable]"				},

  { "tsetenv",		CMD_FUNC_NODE,	NULL,	&cmd_setenv_cmd,	"tsetenv [temporary var] [string]"	},
  { "tunsetenv",	CMD_FUNC_NODE,	NULL,	&cmd_unsetenv_cmd,	"tunsetenv [temporary var]"			},
  { "tputenv",		CMD_FUNC_NODE,	NULL,	&cmd_setenv_cmd,	"tputenv [temporary var]"			},
  { "tclearenv",	CMD_FUNC_NODE,	NULL,	&cmd_unsetenv_cmd,	"tclearenv"							},
  { "saveenv",		CMD_FUNC_NODE,	NULL,	&cmd_saveenv_cmd,	"saveenv"							},

#ifdef	SUPPORT_NVEDIT_LEGACY
  { "auto",			CMD_FUNC_NODE,	NULL,	&cmd_autoboot_cmd,	"batch file command"				},
  { "setauto",		CMD_FUNC_NODE,	NULL,	&cmd_autoboot_cmd,	"edit batch file"					},
  { "printauto",	CMD_FUNC_NODE,	NULL,	&cmd_printauto_cmd,	"print batch file"					},
#endif	//SUPPORT_NVEDIT_LEGACY

#ifdef	SUPPORT_NVEDIT_LEGACY_AGC
  { "setagc",		CMD_FUNC_NODE,	NULL,	&cmd_setagc_cmd,	"Device NVRAM command"				},
  { "unsetagc",		CMD_FUNC_NODE,	NULL,	&cmd_setagc_cmd,	"Device NVRAM command"				},
#endif	//SUPPORT_NVEDIT_LEGACY_AGC

#endif //XIP_CACHE_BOOT
#endif //defined(SUPPORT_NVEDIT)

  { "nvcfg",		CMD_FUNC_NODE,	NULL,	&cmd_nvconfig_cmd,	"nvconfig command"					},
#ifdef	XIP_CACHE_BOOT
  { "nv_aging",		CMD_FUNC_NODE,	NULL,	&cmd_nv_aging_func,	"NVRAM AGING TEST"					},
#endif //XIP_CACHE_BOOT

  { "",   CMD_FUNC_NODE,  NULL,       NULL,               ""	},
  { "backup_nvram",	CMD_FUNC_NODE,	NULL,	&cmd_backup_nvram,	"Backup nvram"						},
  { "restore_nvram",CMD_FUNC_NODE,	NULL,	&cmd_restore_nvram,	"Restore nvram"						},

  { NULL, 		CMD_NULL_NODE,	NULL,	NULL,			NULL }			// Tail
};


/******************************************************************************
 *  cmd_nvedit_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_nvedit_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)

	UINT8  *buffer;
	UINT32	ioctldata[5];
	UINT8	data8;
	UINT16	data16;
	UINT32	data32;

	if(argc < 2) {
		PRINTF("Usage : %s "
#ifdef SUPPORT_NVEDIT_FULL
			"[root|add|del|save|load|clear|erase|update|find|print]\n"
#else  //SUPPORT_NVEDIT_FULL
			"[lfind|lsave]\n"
#endif	//SUPPORT_NVEDIT_FULL
			, argv[0]);
		return;
	}

	data8 = 0;
	data16 = 0;
	data32 = 0;

	da16x_environ_lock(TRUE);

#ifdef SUPPORT_NVEDIT_FULL
	if(DRIVER_STRCMP(argv[1], "root") == 0 && argc == 5){
		ioctldata[0] = (UINT32) (argv[2]) ;

		if(DRIVER_STRCMP(argv[3], "u8") == 0){
			data8	= (UINT8)(*argv[4]);
			ioctldata[1] = sizeof(UINT8);
			ioctldata[2] = (UINT32) &data8;
		}else
		if(DRIVER_STRCMP(argv[3], "u16") == 0){
			data16	= (UINT16)ctoi(argv[4]);
			ioctldata[1] = sizeof(UINT16);
			ioctldata[2] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[3], "h16") == 0){
			data16	= (UINT16)htoi(argv[4]);
			ioctldata[1] = sizeof(UINT16);
			ioctldata[2] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[3], "u32") == 0){
			data32	= ctoi(argv[4]);
			ioctldata[1] = sizeof(UINT32);
			ioctldata[2] = (UINT32) &data32;
		}else
		if(DRIVER_STRCMP(argv[3], "h32") == 0){
			data32	= htoi(argv[4]);
			ioctldata[1] = sizeof(UINT32);
			ioctldata[2] = (UINT32) &data32;
		}else
		if(DRIVER_STRCMP(argv[3], "str") == 0){
			ioctldata[1] = DRIVER_STRLEN(argv[4]) + 1 ;
			ioctldata[1] = NVITEM_LEN_MASK & ioctldata[1] ;
			ioctldata[1] = NVITEM_VAR_MARK | ioctldata[1] ;
			ioctldata[2] = (UINT32) argv[4] ;
		}else
		if(DRIVER_STRCMP(argv[3], "node") == 0){
			ioctldata[1] = NVITEM_NOD_MARK  ;
			ioctldata[2] = 0 ;
		}else{
			ioctldata[1] = 0;
			ioctldata[2] = 0;
		}

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_ROOT, ioctldata) == TRUE){
			PRINTF("%s , %s completed\r\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}

	}else
	if(DRIVER_STRCMP(argv[1], "add") == 0 && argc == 6){
		ioctldata[0] = (UINT32) (argv[2]) ;
		ioctldata[1] = (UINT32) (argv[3]) ;

		if(DRIVER_STRCMP(argv[4], "u8") == 0){
			data8	= (UINT8)(*argv[5]);
			ioctldata[2] = sizeof(UINT8);
			ioctldata[3] = (UINT32) &data8;
		}else
		if(DRIVER_STRCMP(argv[4], "u16") == 0){
			data16	= (UINT16)ctoi(argv[5]);
			ioctldata[2] = sizeof(UINT16);
			ioctldata[3] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[4], "h16") == 0){
			data16	= (UINT16)htoi(argv[5]);
			ioctldata[2] = sizeof(UINT16);
			ioctldata[3] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[4], "u32") == 0){
			data32	= ctoi(argv[5]);
			ioctldata[2] = sizeof(UINT32);
			ioctldata[3] = (UINT32) &data32;

			/* check clock */
			if ((strcmp(argv[2], "boot.clk")==0) && (strcmp(argv[3], "bus")==0))
			{
				for (char clk = SYSCLK_DIV_160MHZ; clk < SYSCLK_MAX; clk++)
				{
					if (data32 == (DA16X_PLL_CLOCK / (unsigned long)clk))
					{
						break;
					}

					if (clk >= SYSCLK_MAX-1)
					{
						da16x_environ_lock(FALSE);
						PRINTF("\nError: clock %s\n", argv[5]);
						PRINTF("\nAvailable clock:\n");
						for (clk = SYSCLK_DIV_160MHZ; clk < SYSCLK_MAX; clk++)
						{
							PRINTF("\t%9d\n", (DA16X_PLL_CLOCK / (unsigned long)clk));
						}
						PRINTF("\n");
						return;
					}
				}
			}
		}else
		if(DRIVER_STRCMP(argv[4], "h32") == 0){
			data32	= htoi(argv[5]);
			ioctldata[2] = sizeof(UINT32);
			ioctldata[3] = (UINT32) &data32;
		}else
		if(DRIVER_STRCMP(argv[4], "str") == 0){
			ioctldata[2] = DRIVER_STRLEN(argv[5]) + 1 ;
			ioctldata[2] = NVITEM_LEN_MASK & ioctldata[2] ;
			ioctldata[2] = NVITEM_VAR_MARK | ioctldata[2] ;
			ioctldata[3] = (UINT32) argv[5] ;
		}else
		if(DRIVER_STRCMP(argv[4], "node") == 0){
			ioctldata[2] = NVITEM_NOD_MARK  ;
			ioctldata[3] = 0 ;
		}else{
			ioctldata[2] = 0;
			ioctldata[3] = 0;
		}

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_ADD, ioctldata) == TRUE){
			PRINTF("%s , %s completed\r\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
	if(DRIVER_STRCMP(argv[1], "del") == 0 && argc == 3){
		ioctldata[0] = (UINT32) (argv[2]) ;
		ioctldata[1] = TRUE ;

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_DEL, ioctldata) == TRUE){
			PRINTF("%s , %s completed\r\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
	if(DRIVER_STRCMP(argv[1], "update") == 0 && argc == 5){
		ioctldata[0] = (UINT32) (argv[2]) ;

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE){

			if(DRIVER_STRCMP(argv[3], "u8") == 0){
				data8	= (UINT8)(*argv[4]);
				ioctldata[2] = sizeof(UINT8);
				ioctldata[3] = (UINT32) &data8;
			}else
			if(DRIVER_STRCMP(argv[3], "u16") == 0){
				data16	= (UINT16)ctoi(argv[4]);
				ioctldata[2] = sizeof(UINT16);
				ioctldata[3] = (UINT32) &data16;
			}else
			if(DRIVER_STRCMP(argv[3], "h16") == 0){
				data16	= (UINT16)htoi(argv[4]);
				ioctldata[2] = sizeof(UINT16);
				ioctldata[3] = (UINT32) &data16;
			}else
			if(DRIVER_STRCMP(argv[3], "u32") == 0){
				data32	= ctoi(argv[4]);
				ioctldata[2] = sizeof(UINT32);
				ioctldata[3] = (UINT32) &data32;

				/* check clock */
				if (strcmp(argv[2], "boot.clk.bus")==0)
				{
					for (char clk = SYSCLK_DIV_160MHZ; clk < SYSCLK_MAX; clk++)
					{
						if (data32 == (DA16X_PLL_CLOCK / (unsigned long)clk))
						{
							break;
						}

						if (clk >= SYSCLK_MAX-1)
						{
							da16x_environ_lock(FALSE);
							PRINTF("\nError: clock %s\n", argv[4]);
							PRINTF("\nAvailable clock:\n");
							for (clk = SYSCLK_DIV_160MHZ; clk < SYSCLK_MAX; clk++)
							{
								PRINTF("\t%9d\n", (DA16X_PLL_CLOCK / (unsigned long)clk));
							}
							PRINTF("\n");
							return;
						}
					}
				}


			}else
			if(DRIVER_STRCMP(argv[3], "h32") == 0){
				data32	= htoi(argv[4]);
				ioctldata[2] = sizeof(UINT32);
				ioctldata[3] = (UINT32) &data32;
			}else
			if(DRIVER_STRCMP(argv[3], "str") == 0){
				ioctldata[2] = DRIVER_STRLEN(argv[4]) + 1 ;
				ioctldata[2] = NVITEM_LEN_MASK & ioctldata[2] ;
				ioctldata[3] = (UINT32) argv[4] ;
				PRINTF("str update - (%x) \"%s\"\r\n", ioctldata[2], argv[4] );
			}else
			if(DRIVER_STRCMP(argv[3], "node") == 0){
				ioctldata[2] = NVITEM_NOD_MARK  ;
				ioctldata[3] = 0;
			}else{
				ioctldata[2] = 0;
				ioctldata[3] = 0;
			}

			if( SYS_NVEDIT_WRITE( ioctldata[1], (VOID *)ioctldata[3], ioctldata[2]) != 0){
				PRINTF("%s , %s success\r\n", argv[0], argv[1]);
			}else{
				PRINTF("%s , %s failed\r\n", argv[0], argv[1]);
			}
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
	if(DRIVER_STRCMP(argv[1], "save") == 0 && argc == 3){
		extract_flash_info( argv[2], ioctldata );

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, ioctldata) == TRUE){
			PRINTF("%s , %s completed\r\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
	if(DRIVER_STRCMP(argv[1], "load") == 0 && argc == 3){
		extract_flash_info( argv[2], ioctldata );

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LOAD, ioctldata) == TRUE){
			PRINTF("%s , %s completed\r\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
#endif //SUPPORT_NVEDIT_FULL
#ifndef SUPPORT_NVEDIT_FULL
	if(DRIVER_STRCMP(argv[1], "lfind") == 0 && argc == 4){
		buffer = pvPortMalloc(4096);
		if(buffer == NULL){
			return ;
		}

		extract_flash_info( argv[2], ioctldata );

		ioctldata[2] = (UINT32)argv[3];
		ioctldata[3] = (UINT32)&data32;
		ioctldata[4] = (UINT32)buffer;

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEFIND, ioctldata) == TRUE){
			switch((data32&NVITEM_TYPE_MASK)){
				case NVITEM_NOD_MARK:
						PRINTF("%s - node (%d)\r\n", argv[3], (data32 & NVITEM_LEN_MASK) );
						break;
				case NVITEM_VAR_MARK:
						PRINTF("%s - string (%d) \"%s\"\r\n", argv[3], (data32 & NVITEM_LEN_MASK), (char *)buffer );
						break;
				default:
					switch((data32&NVITEM_LEN_MASK)){
						case	sizeof(UINT8):
							PRINTF("%s - u8, %d(%x)\r\n", argv[3], *((UINT8 *)buffer), *((UINT8 *)buffer) );
							break;
						case	sizeof(UINT16):
							PRINTF("%s - u16, %d(%x)\r\n", argv[3], *((UINT16 *)buffer), *((UINT16 *)buffer) );
							break;
						case	sizeof(UINT32):
							PRINTF("%s - u32, %d(%x)\r\n", argv[3], *((UINT32 *)buffer), *((UINT32 *)buffer) );
							break;
						default:
							PRINTF("%s - unknown (%d)\r\n", argv[3], (data32 & NVITEM_LEN_MASK) );
							cmd_dump_print(0, ((UINT8 *)buffer), (data32 & NVITEM_LEN_MASK), 0);
							PRINTF("\r\n");
							break;
					}
			}
			PRINTF("%s , %s completed - %08x\r\n", argv[0], argv[1], data32);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}

		vPortFree(buffer);

	}else
	if(DRIVER_STRCMP(argv[1], "lsave") == 0 && argc == 6){
		UINT32 datlen;

		extract_flash_info( argv[2], ioctldata );

		ioctldata[2] = (UINT32)argv[3];
		ioctldata[3] = (UINT32)&datlen;

		if(DRIVER_STRCMP(argv[4], "u8") == 0){
			data8	= *argv[5];
			ioctldata[4] = (UINT32) &data8;
		}else
		if(DRIVER_STRCMP(argv[4], "u16") == 0){
			data16	= ctoi(argv[5]);
			ioctldata[4] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[4], "h16") == 0){
			data16	= htoi(argv[5]);
			ioctldata[4] = (UINT32) &data16;
		}else
		if(DRIVER_STRCMP(argv[4], "u32") == 0){
			data32	= ctoi(argv[5]);
			ioctldata[4] = (UINT32) &data32;
		}else
		if(DRIVER_STRCMP(argv[4], "h32") == 0){
			data32	= htoi(argv[5]);
			ioctldata[4] = (UINT32) &data32;
		}else
		if(DRIVER_STRCMP(argv[4], "str") == 0){
			ioctldata[4] = (UINT32) argv[5] ;
		}else{
			ioctldata[4] = NULL;
		}

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_LITEUPDATE, ioctldata) == TRUE){
			PRINTF("%s , %s completed\n", argv[0], argv[1]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}
	}else
#endif //!SUPPORT_NVEDIT_FULL
#ifdef SUPPORT_NVEDIT_FULL
	if(DRIVER_STRCMP(argv[1], "find") == 0 && argc == 4){

		buffer = pvPortMalloc(4096);
		if(buffer == NULL){
			return ;
		}

		ioctldata[0] = (UINT32)argv[2];
		ioctldata[1] = (UINT32)NULL;

		if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE){
			if((VOID *)(ioctldata[1]) != NULL){
				data32 = (UINT32)SYS_NVEDIT_READ( ioctldata[1], buffer, ctoi(argv[3]));
			}
			if(data32 != FALSE){
				switch((data32&NVITEM_TYPE_MASK)){
					case NVITEM_NOD_MARK:
							PRINTF("%s - node (%d)\r\n", argv[2], (data32 & NVITEM_LEN_MASK) );
							break;
					case NVITEM_VAR_MARK:
							PRINTF("%s - string (%d) \"%s\"\r\n", argv[2], (data32 & NVITEM_LEN_MASK), (char *)buffer );
							break;
					default:
						switch((data32&NVITEM_LEN_MASK)){
							case	sizeof(UINT8):
								PRINTF("%s - u8, %d(%x)\r\n", argv[2], *((UINT8 *)buffer), *((UINT8 *)buffer) );
								break;
							case	sizeof(UINT16):
								PRINTF("%s - u16, %d(%x)\r\n", argv[2], *((UINT16 *)buffer), *((UINT16 *)buffer) );
								break;
							case	sizeof(UINT32):
								PRINTF("%s - u32, %d(%x)\r\n", argv[2], *((UINT32 *)buffer), *((UINT32 *)buffer) );
								break;
							default:
								PRINTF("%s - unknown (%d)\r\n", argv[2], (data32 & NVITEM_LEN_MASK) );
								cmd_dump_print(0, ((UINT8 *)buffer), (data32 & NVITEM_LEN_MASK), 0);
								PRINTF("\r\n");
								break;
						}
				}
			}
			PRINTF("%s , %s completed - %08x\r\n", argv[0], argv[1], data32);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed\r\n", __func__, __LINE__, argv[0], argv[1]);
		}

		vPortFree(buffer);

	}else
	if(DRIVER_STRCMP(argv[1], "clear") == 0 ) {
		SYS_NVEDIT_IOCTL( NVEDIT_CMD_CLEAR, NULL);
	}else
	if(DRIVER_STRCMP(argv[1], "erase") == 0 && argc == 3) {
		extract_flash_info( argv[2], ioctldata );

		SYS_NVEDIT_IOCTL( NVEDIT_CMD_ERASE, ioctldata);
	}else
	if(DRIVER_STRCMP(argv[1], "secure") == 0 && argc == 3) {
		ioctldata[0] = ctoi(argv[2]);

		SYS_NVEDIT_IOCTL( NVEDIT_CMD_SECURE, ioctldata);
	}
	else{
		SYS_NVEDIT_IOCTL( NVEDIT_CMD_PRINT, NULL);
	}
#endif //SUPPORT_NVEDIT_FULL

	da16x_environ_lock(FALSE);

#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  extract_flash_info ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#if	defined(SUPPORT_NVEDIT)

static void extract_flash_info(char *name, UINT32 *ioctldata)
{
#ifdef	SUPPORT_NVRAM_NOR
	if(DRIVER_STRCMP(name, "nor") == 0){
		ioctldata[0] = NVEDIT_NOR;
		ioctldata[1] = NOR_UNIT_0;
	}else
#endif	//SUPPORT_NVRAM_NOR
#ifdef	SUPPORT_NVRAM_SFLASH
	if(DRIVER_STRCMP(name, "sflash") == 0){
		ioctldata[0] = NVEDIT_SFLASH;
		ioctldata[1] = SFLASH_UNIT_0;
	}else
#endif	//SUPPORT_NVRAM_SFLASH
	{
		ioctldata[0] = ENVIRON_DEVICE;
		ioctldata[1] = 0;
	}
}

#endif  //defined(SUPPORT_NVEDIT)

/******************************************************************************
 *  cmd_nvconfig_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_nvconfig_cmd(int argc, char *argv[])
{
	UINT32 device;

	da16x_environ_lock(TRUE);

	if(argc == 3 && DRIVER_STRCMP(argv[1], "update") == 0){

#ifdef	SUPPORT_NVRAM_NOR
		if( DRIVER_STRCMP(argv[2], "nor") == 0 ){
			device = NVEDIT_NOR ;
		}else
#endif	//SUPPORT_NVRAM_NOR
#ifdef	SUPPORT_NVRAM_SFLASH
		if( DRIVER_STRCMP(argv[2], "sflash") == 0 ){
			device = NVEDIT_SFLASH ;
		}else
#endif	//SUPPORT_NVRAM_SFLASH
		{
#if	defined(SUPPORT_NVEDIT)
			device = ENVIRON_DEVICE ;
#else	//defined(SUPPORT_NVEDIT)
			device = NVEDIT_RAM ;
#endif	//defined(SUPPORT_NVEDIT)
		}

		if( SYS_NVCONFIG_UPDATE(device) == TRUE ){
			PRINTF("%s , %s completed \r\n", argv[1], argv[2]);
		}else{
			PRINTF(" [%s:%d] %s , %s NOT completed \r\n", __func__, __LINE__, argv[1], argv[2]);
		}
	}else
	if(DRIVER_STRCMP(argv[1], "init") == 0){
		system_config_init(FALSE);
	}else{
		SYS_NVCONFIG_VIEW(NULL);
	}

	da16x_environ_lock(FALSE);
}

/******************************************************************************
 *  cmd_getenv_cmd ( )
 *
 *  Purpose :   getenv, printenv
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_getenv_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)

	if (DRIVER_STRCMP(argv[0], "getenv") == 0 && argc == 2) {
		char *getvalue;
		getvalue = da16x_getenv(ENVIRON_DEVICE, "app", argv[1]);
		if ( getvalue != NULL ) {
			PRINTF("%s=", argv[1]);
			DRV_DBG_TEXT(0, getvalue, (UINT16)DRV_STRLEN(getvalue));
			PRINTF("\n");
		}
		else {
			PRINTF("%s is not matched\n", argv[1], getvalue);
		}
	}
	else {
		da16x_printenv(ENVIRON_DEVICE, "app");
	}
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  make_message ( )
 *
 *  Purpose :   setenv, putenv
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#undef  SKIP_DELIMETER

int make_message(char *title, char *buf, size_t buflen)
{
	char ch;
	unsigned int msglen = 0;
#ifdef  SKIP_DELIMETER
        int skip = FALSE;
#endif  //SKIP_DELIMETER

	if (buf == NULL) {
		PRINTF("[%s]Invalid buffer size\n", __func__);
		return -1;
	}

	PRINTF("Typing data: (%s)\n\tCancel - CTRL+D, "
		"End of Input - CTRL+C or CTRL+Z\n" , title );

	while (1) {
		ch = GETC() ;
		if ( ch == 0x03 || ch == 0x04 || ch == 0x1a) { /* CTRL+C, CTRL+D, CTRL+Z, */
			break;
		}

#ifdef  SKIP_DELIMETER
		if (ch == '[') {
			skip = TRUE;
			continue;
		} else if(ch == ']') {
			skip = FALSE;
			continue;
		} else if( skip == TRUE ) {
			continue;
		}
#endif  //SKIP_DELIMETER

		if (ch == 0x0D) {
			ch = 0x0A;
		}

		msglen++;
		if (msglen > (buflen - 1)) {
			PRINTF("\nToo long input data (MAX Length : %d byte)\n",
				buflen - 1);
			return -1;
		}
		buf[(msglen-1)] = (char)ch;
	}

	if (ch == 0x03 || ch == 0x1a) { /* CTRL+C, CTRL+Z, */
		//buf[msglen] = ' ';
		//buff[msglen+1] = '\0';
		buf[msglen] = '\0';
	} else { /* CTRL+D */
		// cancel
		buf[0] = '\0';
		msglen = 0;
	}

	return (int)msglen;
}

/******************************************************************************
 *  cmd_setenv_cmd ( )
 *
 *  Purpose :   setenv, putenv
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_setenv_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)
	if(DRIVER_STRCMP(argv[0], "setenv") == 0 ){
		if( argc == 3 ){
			da16x_setenv(ENVIRON_DEVICE, "app", argv[1], argv[2]);
		}
		else if( argc == 2 ) {
			char *buf = NULL;
			size_t buflen = (1024 * 4) + 2;
			int msglen = 0;

			buf = pvPortMalloc(buflen);
			if (buf) {
				msglen = make_message("setenv value", buf, buflen);
				if (msglen > 0) {
					da16x_setenv(ENVIRON_DEVICE, "app", argv[1], buf);
				}

				vPortFree(buf);
			}
		}
		/*
		    setenv test aaaaaa str ==> test (STR,70) ......... "aaaaaa"
		    NVRAM ����� ����ǥ ���� ����
		*/
		else if (argc == 4 || strcmp(argv[3], "str") == 0 ) {
			size_t len;
			char *tmp_str;

			len = DRIVER_STRLEN(argv[2]);
			tmp_str = pvPortMalloc(len+3);
			tmp_str[0] = '"';
			DRIVER_STRCPY(&(tmp_str[1]), argv[2]);
			tmp_str[len+1] = '"';
			tmp_str[len+2] = '\0';

			da16x_setenv(ENVIRON_DEVICE, "app", argv[1], tmp_str);
			vPortFree(tmp_str);
		}
	}
	else if(DRIVER_STRCMP(argv[0], "putenv") == 0 && argc == 2) {
		da16x_putenv(ENVIRON_DEVICE, "app", argv[1]);
	}
	else if(DRIVER_STRCMP(argv[0], "tsetenv") == 0 ) {
		if ( argc == 3 ) {
			da16x_setenv_temp(ENVIRON_DEVICE, "app", argv[1], argv[2]);
		}
		else if( argc == 2 ) {
			char *buf = NULL;
			size_t buflen = (1024 * 4) + 2;
			int msglen = 0;

			buf = pvPortMalloc(buflen);
			if (buf) {
				msglen = make_message("tsetenv value", buf, buflen);
				if (msglen > 0) {
					da16x_setenv(ENVIRON_DEVICE, "app", argv[1], buf);
				}

				vPortFree(buf);
			}
		}
#if 1 /* munchang.jung_20160427 */
		/*
		    tsetenv test aaaaaa str ==> test (STR,70) ................ "aaaaaa"
		    NVRAM ����� ����ǥ ���� ����
		*/
		else if (argc == 4 || strcmp(argv[3], "str") == 0 ) {
			size_t len;
			char *tmp_str;
			len = DRIVER_STRLEN(argv[2]);

			tmp_str = pvPortMalloc(len+3);
			tmp_str[0] = '"';
			DRIVER_STRCPY(&(tmp_str[1]), argv[2]);
			tmp_str[len+1] = '"';
			tmp_str[len+2] = '\0';

			da16x_setenv_temp(ENVIRON_DEVICE, "app", argv[1], tmp_str);
			vPortFree(tmp_str);
		}
#endif /* 1 */
	} else if(DRIVER_STRCMP(argv[0], "tputenv") == 0 && argc == 2){
		da16x_putenv_temp(ENVIRON_DEVICE, "app", argv[1]);
	}
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  cmd_unsetenv_cmd ( )
 *
 *  Purpose :   unsetenv, clearenv
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_unsetenv_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)
	if(DRIVER_STRCMP(argv[0], "unsetenv") == 0 && argc == 2){
		da16x_unsetenv(ENVIRON_DEVICE, "app", argv[1]);
	}else
	if(DRIVER_STRCMP(argv[0], "clearenv") == 0){
		da16x_clearenv(ENVIRON_DEVICE, "app");
	}else
	if(DRIVER_STRCMP(argv[0], "tunsetenv") == 0 && argc == 2){
		da16x_unsetenv_temp(ENVIRON_DEVICE, "app", argv[1]);
	}else
	if(DRIVER_STRCMP(argv[0], "tclearenv") == 0){
		da16x_clearenv_temp(ENVIRON_DEVICE, "app");
	}
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  cmd_saveenv_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_saveenv_cmd(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

#if	defined(SUPPORT_NVEDIT)
	da16x_saveenv(ENVIRON_DEVICE);
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  cmd_autoboot_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void cmd_autoboot_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT_LEGACY)
	unsigned int 	off, len, chop;
	char	*str_cmd, *end_cmd ;
	char	*autoexec, *batchcmd;

	if( argc != 2 ){
		PRINTF("%s [keyname]\n", argv[0]);
		return ;
	}

	if( DRIVER_STRCMP("setauto", argv[0]) == 0 ){
		char *buf = NULL;
		size_t buflen = (1024 * 4) + 2;
		int msglen = 0;

		buf = pvPortMalloc(buflen);
		if (buf) {
			msglen = make_message(argv[1], buf, buflen);
			if (msglen > 0) {
				da16x_setenv(ENVIRON_DEVICE, "debug", argv[1], buf);
			}

			vPortFree(buf);
		}
		return;
	}

	autoexec = da16x_getenv(ENVIRON_DEVICE, "debug", argv[1]);

	if(autoexec != NULL ){
		len  = DRIVER_STRLEN(autoexec);
		batchcmd = pvPortMalloc(len+1);
		DRIVER_STRCPY(batchcmd, autoexec);

		/* start offset */
		str_cmd = (char *)batchcmd ;
		chop = 0;

		/* truncating */
		for( off=0; off < len ; off++ )
		{
			if(  batchcmd[off] == '\0' ){
				break;
			}else
			if( chop == 0 ){
				if(    batchcmd[off] == '\r' || batchcmd[off] == '\n'
				    || batchcmd[off] == '\t' || batchcmd[off] == ' '
				    || batchcmd[off] == '\b' ) {
				    	// skip
				}else{
					str_cmd = (char *)&(batchcmd[off]);
					chop = 1;
				}
				//skip
			}else
			if( batchcmd[off] == ';' )
			{
				end_cmd = (char *)&(batchcmd[off]);
				*end_cmd = '\0';

				PRINTF("[AUTOEXEC] %s\n", str_cmd);
				exec_mon_cmd(str_cmd);
				*end_cmd = ';';

				chop = 0;
			}
		}

		vPortFree(batchcmd);
	}
#else
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);
#endif	//defined(SUPPORT_NVEDIT_LEGACY)
}

/******************************************************************************
 *  cmd_printauto_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/


void cmd_printauto_cmd(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

#if	defined(SUPPORT_NVEDIT_LEGACY)

	da16x_printenv(ENVIRON_DEVICE, "debug");

#endif	//defined(SUPPORT_NVEDIT_LEGACY)
}

/******************************************************************************
 *  make_binary ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int make_binary(char *message, UINT8 **perdata, UINT32 swap)
{
	UINT32 perlen , tot, i, j = 0;
	volatile UINT8  *buff;
        //volatile UINT8  *newbuff ;

	buff = (UINT8 *)pvPortMalloc(512 * 4);
	tot = 512 * 4 ;
	perlen = 0;
	i = 0;

	while( message[i] != ' ' && message[i+1] != '\0' ){
		// skip whitespace
		if(    message[i] == '.' || message[i] == ' ' || message[i] == '-'
			|| message[i] == '_' ||message[i] == '\t' || message[i] == '\r'
			|| message[i] == '\n' ){
			i++;
			continue ; // skip
		}

		if( (j) >tot ){
			/*
			newbuff = (UINT8 *)APP_MALLOC(tot + 1024);
			DRIVER_MEMCPY(newbuff, buff, perlen) ;
			APP_FREE(buff) ;
			buff = newbuff ;
			tot  = tot + 1024 ;
			*/
			PRINTF("agc code length error\n");
		}


		if( ( perlen % 2 ) == 0 ){
			buff[(perlen/2)] =  ( toint( message[i] )  ) ;
		}else{
			buff[(perlen/2)] = (UINT8)(( buff[(perlen/2)] << 4 ) | ( toint( message[i] )  )) ;
			j++;

			if( (swap == TRUE) && ( (j != 0) && ((j%4)==0) ) )
			{
				volatile UINT8 temp;
				temp = buff[j-4];
				buff[j-4] = buff[j-1];
				buff[j-1] = temp;
				temp = buff[j-3];
				buff[j-3] = buff[j-2];
				buff[j-2] = temp;
			}
		}
		perlen ++ ;
		i ++ ;
	}

	if( perlen == 0 ){
		vPortFree((void*)buff);
		*perdata = NULL;
	}else{
		*perdata = (UINT8 *)buff ;
	}

	return (int)((perlen+1)/2);
}

/******************************************************************************
 *  cmd_setdev_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_setdev_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)
	if( DRIVER_STRCMP("setdev", argv[0]) == 0 ){
		int binlen = 0;
		UINT8 *data = NULL;

		char *buf = NULL;
		size_t buflen = (1024 * 4) + 2;
		int msglen = 0;

		buf = pvPortMalloc(buflen);
		if (buf) {
			msglen = make_message(argv[1], buf, buflen);
			if (msglen > 0) {
				binlen = make_binary(buf, &data, TRUE);
				da16x_setenv_bin(ENVIRON_DEVICE, "dev", argv[1], data, (UINT32)binlen);

				if (data) {
					vPortFree(data);
				}
			}

			vPortFree(buf);
		}
	}else if(DRIVER_STRCMP(argv[0], "unsetdev") == 0 && argc == 2){
		da16x_unsetenv(ENVIRON_DEVICE, "dev", argv[1]);
	}
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  cmd_getdev_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_getdev_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)
	if(DRIVER_STRCMP(argv[0], "getdev") == 0 && argc == 2){
		UINT32 len;
		void *getvalue;
		getvalue = da16x_getenv_bin(ENVIRON_DEVICE, "dev", argv[1], &len);

		if( getvalue != NULL ){
			PRINTF("%s=%d\n", argv[1], len);
			DRV_DBG_DUMP(0, getvalue, (UINT16)len);
		}else{
			PRINTF("%s is not found\n", argv[1]);
		}
	}
#endif	//defined(SUPPORT_NVEDIT)
}

/******************************************************************************
 *  cmd_setagc_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_setagc_cmd(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);

#if	defined(SUPPORT_NVEDIT_LEGACY_AGC)
	if( DRIVER_STRCMP("setagc", argv[0]) == 0 ){
		int len;
		UINT8 *data = NULL;

		char *buf = NULL;
		size_t buflen = (1024 * 4) + 2;
		int msglen = 0;

		buf = pvPortMalloc(buflen);
		if (buf) {
			msglen = make_message("agcbin", buf, buflen);
			if (msglen > 0) {
				len = make_binary(buffer, &data, TRUE);
				da16x_setenv_bin(ENVIRON_DEVICE, "dev", "agcbin", data, len);

				if (data) {
					vPortFree(data);
				}
			}

			vPortFree(buf);
		}
	}else if(DRIVER_STRCMP(argv[0], "unsetagc") == 0){
		da16x_unsetenv(ENVIRON_DEVICE, "dev", "agcbin");
	}
#else
	DA16X_UNUSED_ARG(argv);
#endif	//defined(SUPPORT_NVEDIT_LEGACY_AGC)
}

/******************************************************************************
 *  cmd_ralib_cmd
 ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	puthexa(x) 	( ((x)<10) ? (char)((x)+'0') : (char)((x)-10+'a') )
#define	RAENV_ROOT	"app"
#define	RAENV_NAME	"ramlibrary"

void cmd_ralib_cmd(int argc, char *argv[])
{
#if	defined(SUPPORT_NVEDIT)
	if( argc == 4 && DRIVER_STRCMP("set", argv[1]) == 0 ){
		// ralib set [nor|sflash|otp] [offset]
		UINT32 loadaddr, i;
		char	rasymbols[12];

		loadaddr = htoi(argv[3]);

		if(DRIVER_STRCMP("nor", argv[2]) == 0){
#ifdef	BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_NOR, loadaddr);
#else	//BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_SFLASH, loadaddr);
#endif	//BUILD_OPT_DA16200_FPGA
		}else
		if(DRIVER_STRCMP("sflash", argv[2]) == 0){
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_SFLASH, loadaddr);
		}else
		if(DRIVER_STRCMP("eflash", argv[2]) == 0){
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_EFLASH, loadaddr);
		}else
		if(DRIVER_STRCMP("otp", argv[2]) == 0){
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_OTP, loadaddr);
		}else{
#ifdef	BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_NOR,loadaddr);
#else	//BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_SFLASH,loadaddr);
#endif	//BUILD_OPT_DA16200_FPGA
		}

		for(i=0; i<8; i++){
			rasymbols[i] = puthexa( ((loadaddr>>((7-i)*4))&0x0f) );
		}
		rasymbols[i] = '\0';

		da16x_setenv(ENVIRON_DEVICE, RAENV_ROOT, RAENV_NAME, rasymbols);

		//ramlib_clear();		/* F_F_S */
		da16x_boot_set_lock(FALSE);
		da16x_boot_set_offset(BOOT_IDX_RLIBLA, 0);
		da16x_boot_set_lock(TRUE);

		PRINTF("RaLIB, updated - %s %s\n", argv[2], rasymbols);

	}
	else
	if( argc == 2 && DRIVER_STRCMP("unset", argv[1]) == 0 ){
		// ralib unset
		da16x_unsetenv(ENVIRON_DEVICE, RAENV_ROOT, RAENV_NAME);
		PRINTF("RaLIB, erased\n");
	}else
	{
		// ralib
		char *raoffset, *memtype;
		UINT32 loadaddr;

		raoffset = da16x_getenv(ENVIRON_DEVICE, RAENV_ROOT, RAENV_NAME);

		if( raoffset != NULL ){
			loadaddr = htoi(raoffset);
		}else{
#ifdef	BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_NOR,0x100000);
#else	//BUILD_OPT_DA16200_FPGA
			loadaddr = CONVERT_BOOT_OFFSET(BOOT_SFLASH,0x100000);
#endif	//BUILD_OPT_DA16200_FPGA
		}

		switch(BOOT_MEM_GET(loadaddr)){
			case BOOT_NOR:	   	memtype = "nor"; 	break;
			case BOOT_SFLASH: 	memtype = "sflash"; 	break;
			case BOOT_EFLASH:	memtype = "eflash"; 	break;
			case BOOT_OTP:	   	memtype = "otp"; 	break;
			default:	   	memtype = "unknown"; 	break;
		}
		PRINTF("RaLIB, %s exist: %s - offset %8x\n"
				, ((raoffset == NULL)? "non" : "")
				, memtype, BOOT_OFFSET_GET(loadaddr));
	}
#endif	//defined(SUPPORT_NVEDIT)
}

extern int write_nvram_string(const char *name, const char *val);
void cmd_nv_aging_func(int argc, char *argv[])
{
	unsigned int loop = 1;

	/* loop, data_length */
	switch( argc ) {

		case 2:
			loop = (unsigned int)(atoi(argv[1]));
			break;
		default:
			break;
	}
}

#define	RET_QUIT		-99
#define	RET_DEFAULT		-88
#define	RET_NODIGIT		-1
#define	RET_MANUAL		-2

extern int backup_NVRAM(void);
extern int restore_NVRAM(void);
void cmd_backup_nvram(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	char	input_str[3];
	int		getStr_len = 0;

	do {
		PRINTF( ANSI_REVERSE " \n Backup nvram ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD
			   "N"  ANSI_NORMAL "o] : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1) {
			if (input_str[0] == 'N' || input_str[0] == 'Q' || getStr_len == RET_QUIT) {
				PRINTF("\n Backup nvram canceled.\n");
				goto END;
			}
		}
	} while (!(input_str[0] == 'Y' && getStr_len == 1));

	if (backup_NVRAM()) {
		PRINTF(RED_COLOR " [%s] NVRAM Backup Error!!!\n" CLEAR_COLOR, __func__);
	}
	else {
		PRINTF(YELLOW_COLOR " [%s] NVRAM Backup OK!!!\n" CLEAR_COLOR, __func__);
	}
END:
	return;
}

void cmd_restore_nvram(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	char	input_str[3];
	int		getStr_len = 0;

	do {
		PRINTF(ANSI_REVERSE " \n Restore nvram ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD
			   "N"  ANSI_NORMAL "o] : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1) {
			if (input_str[0] == 'N' || input_str[0] == 'Q' || getStr_len == RET_QUIT) {
				PRINTF("\n Restore nvram canceled.\n");
				goto END;
			}
		}
	} while (!(input_str[0] == 'Y' && getStr_len == 1));

	if (restore_NVRAM()) {
		PRINTF(RED_COLOR " [%s] NVRAM Restore Error!!!\n" CLEAR_COLOR, __func__);
	}
	else {
		PRINTF(YELLOW_COLOR " [%s] NVRAM Restore OK!!!\n" CLEAR_COLOR, __func__);
	}
END:
	return;
}

int read_nvram_int(const char *name, int *_val)
{
	int val;
	char *valstr;

	valstr = da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);

	if (valstr == NULL)
	{
		*_val = -1;
		return -1;
	}

	val = atoi(valstr);
	*_val = val;
	return 0;
}

char *read_nvram_string(const char *name)
{
	return da16x_getenv(ENVIRON_DEVICE, "app", (char *)name);
}

int write_nvram_int(const char *name, int val)
{
	char valstr[11];

	memset(valstr, 0, 11);
	sprintf(valstr, "%d", val);

	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start()) {
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	if (da16x_setenv(ENVIRON_DEVICE, "app", (char *)name, valstr) == 0) {
		PRINTF("[%s] NVRAM Write: Failed(%s=%d)\n", __func__, name, val);
		return -2;
	}

	return 0;
}

int write_nvram_string(const char *name, const char *val)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start()) {
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	if (da16x_setenv(ENVIRON_DEVICE, "app", (char *)name, (char *)val) == 0)
	{
		//PRINTF("[%s] NVRAM Write: Failed(%s=%s)\n", __func__, name, val);
		return -2;
	}

	return 0;
}

int delete_nvram_env(const char *name)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	return da16x_unsetenv(ENVIRON_DEVICE, "app", (char *)name);
}

int clear_nvram_env(void)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	return da16x_clearenv(ENVIRON_DEVICE, "app");
}

int write_tmp_nvram_int(const char *name, int val)
{
	char valstr[11];

	memset(valstr, 0, 11);
	sprintf(valstr, "%d", val);

	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	if (da16x_setenv_temp(ENVIRON_DEVICE, "app", (char *)name, valstr) == 0)
	{
		//PRINTF("[%s] NVRAM Write: Failed(%s=%d)\n", __func__, name, val);
		return -1;
	}

	return 0;
}

int write_tmp_nvram_string(const char *name, const char *val)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	if (da16x_setenv_temp(ENVIRON_DEVICE, "app", (char *)name, (char *)val) == 0)
	{
		//PRINTF("[%s] NVRAM Write: Failed(%s=%s)\n", __func__, name, val);
		return -1;
	}

	return 0;
}

int delete_tmp_nvram_env(const char *name)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	return da16x_unsetenv_temp(ENVIRON_DEVICE, "app", (char *)name);
}

int clear_tmp_nvram_env(void)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	return da16x_clearenv_temp(ENVIRON_DEVICE, "app");
}

int save_tmp_nvram(void)
{
	/* At this case,,, dpm_sleep operation was started already */
	if (chk_dpm_pdown_start())
	{
		PRINTF("[%s] Already DPM Sleep started !!!\n", __func__);
		return -1;
	}

	return da16x_saveenv(ENVIRON_DEVICE);
}

/* clock change primitive */
void do_write_clock_change(UINT32 data32, int disp_flag)
{
	UINT32	ioctldata[5];
	UINT32	sflashioctl[5];
	
	ioctldata[0] = (UINT32) ("boot.clk.bus");

	da16x_environ_lock(TRUE);	
	if (SYS_NVEDIT_IOCTL( NVEDIT_CMD_FIND, ioctldata) == TRUE)
	{
			ioctldata[2] = sizeof(UINT32);
			ioctldata[3] = (UINT32) &data32;

			if( SYS_NVEDIT_WRITE( ioctldata[1], (VOID *)ioctldata[3], ioctldata[2]) != 0) {

				extract_flash_info( "sflash", sflashioctl );

				if( SYS_NVEDIT_IOCTL( NVEDIT_CMD_SAVE, sflashioctl) == TRUE) {
					if (disp_flag == pdTRUE)
						PRINTF("\n\nSuccess to change CPU Clock %luMhz ...\n\n", data32/1000/1000);
				}
				da16x_environ_lock(FALSE);
				return;
			}
	}

	PRINTF("\n\nFailed to change CPU clock !!!\n\n");
	da16x_environ_lock(FALSE);

	recovery_NVRAM();
	return;
}

/* EOF */
