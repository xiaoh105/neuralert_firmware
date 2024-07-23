/**
 ****************************************************************************************
 *
 * @file command_nvedit.h
 *
 * @brief Header file for Console Commands to handle and to test
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


#ifndef __COMMAND_NVEDIT_H__
#define __COMMAND_NVEDIT_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "da16x_system.h"

#include "command.h"

//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

extern const COMMAND_TREE_TYPE	cmd_nvram_list[] ;

//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

extern void cmd_nvedit_cmd(int argc, char *argv[]);
extern void cmd_nvconfig_cmd(int argc, char *argv[]);
extern void cmd_getenv_cmd(int argc, char *argv[]);
extern void cmd_setenv_cmd(int argc, char *argv[]);
extern void cmd_unsetenv_cmd(int argc, char *argv[]);
extern void cmd_autoboot_cmd(int argc, char *argv[]);
extern void cmd_printauto_cmd(int argc, char *argv[]);
extern void cmd_getdev_cmd(int argc, char *argv[]);
extern void cmd_setdev_cmd(int argc, char *argv[]);
extern void cmd_setagc_cmd(int argc, char *argv[]);
extern void cmd_saveenv_cmd(int argc, char *argv[]);

extern void cmd_ralib_cmd(int argc, char *argv[]);
extern void cmd_nv_aging_func(int argc, char *argv[]);

extern int  make_message(char *title, char *buf, size_t buflen);
extern int  make_binary(char *message, UINT8 **perdata, UINT32 swap);

extern void cmd_backup_nvram(int argc, char *argv[]);
extern void cmd_restore_nvram(int argc, char *argv[]);

#endif // __COMMAND_NVEDIT_H__

/* EOF */
