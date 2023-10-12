/**
 ****************************************************************************************
 *
 * @file command.h
 *
 * @brief Define and types for System command table
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

#ifndef __command_h__
#define __command_h__

#include "hal.h"
#include "monitor.h"


/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#define	MAX_CMD_STACK		8


/******************************************************************************
 *
 *  Types
 *
 ******************************************************************************/

typedef enum  __command_mark_type__ {
	CMD_NULL_NODE = 0,
	CMD_FUNC_NODE  ,
	CMD_FORCE_NODE ,
	CMD_MY_NODE    ,
	CMD_TOP_NODE   ,		// Parent
	CMD_SUB_NODE   ,		// Child
	CMD_ROOT_NODE  ,		// Orign
	CMD_MARK_NODE  ,
	CMD_MAX
} COMMAND_MARK_TYPE ;

typedef struct	__command_tree_type__ {
	char			*cmd ;
	COMMAND_MARK_TYPE	mark;
	const struct __command_tree_type__ *node;
	void			(*func)(int argc, char *argv[]);
	char			*help;
} COMMAND_TREE_TYPE;

typedef enum __system_reboot__
{
	SYS_RESET,
	SYS_REBOOT_POR,
	SYS_REBOOT
} SYSTEM_REBOOT;

typedef enum __send_disconnect__
{
	DISCONNECT_NONE,
	DISCONNECT_SEND,
	DISCONNECT_NONE_CLEAR
} SEND_DISCON;


/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

extern const COMMAND_TREE_TYPE	cmd_root_list[];
extern const char terminal_color_red[6];
extern const char terminal_color_normal[4];
extern const char terminal_color_blue[6]; 

/******************************************************************************
 *
 *  Functions
 *
 ******************************************************************************/

extern int	 cmd_stack_initialize(void);
extern COMMAND_TREE_TYPE	*cmd_get_current_stack(void);
extern COMMAND_TREE_TYPE	*cmd_get_top_stack(void);
extern int	cmd_get_full_command_path(char *path);
extern COMMAND_TREE_TYPE	*cmd_stack_action(int quick, COMMAND_TREE_TYPE *node);

extern int	cmd_interface_action(int argc, char *argv[]);

extern void	cmd_dump_print(UINT addr, UCHAR *data, UINT length, UINT endian);
extern void	cmd_help_func(int argc, char *argv[]);

extern void	cmd_echo_func(int argc, char *argv[]);
extern void	cmd_repeat_func(int argc, char *argv[]);

extern void	cmd_logview_func(int argc, char *argv[]);
extern void	cmd_reboot_func(int argc, char *argv[]);
extern void	cmd_hidden_func(int argc, char *argv[]);

extern void	print_full_path_name(char *prompt, char *buf, int buflen);
extern void	reboot_func(UINT flag);
extern void	cmd_passwd_func(int argc, char *argv[]);

extern void	version_display(int argc, char *argv[]);

extern unsigned int nReturn_from_system;//jason151207

/******************************************************************************
 *
 *  Sub-Commands
 *
 ******************************************************************************/

#include "command_sys.h"
#include "command_nvedit.h"
#include "command_mem.h"
#include "command_sys_hal.h"
#include "command_spi_master.h"

#endif /* __command_h__ */

/* EOF */
