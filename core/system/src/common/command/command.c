/**
 ****************************************************************************************
 *
 * @file command.c
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

#include <stdbool.h>
#include "sdk_type.h"


#include "stdio.h"
#include "string.h"
#include "clib.h"

#include "oal.h"
#include "driver.h"

#include "monitor.h"
#include "command.h"
#include "schd_trace.h"
#include "da16x_types.h"

#include "console.h"
#include "command_sys.h"
#include "command_mem.h"
#include "command_nvedit.h"

#include "command_lmac.h"
#include "command_rf.h"
#include "command_net.h"

#include "command_i2c.h"
#include "command_pwm.h"
#include "command_i2s.h"
#include "command_host.h"
#include "command_adc.h"
#include "user_command.h"
#include "user_dpm_api.h"
#include "util_api.h"
#include "da16x_compile_opt.h"

#pragma GCC diagnostic ignored "-Wunused-variable"

//-----------------------------------------------------------------------
#ifdef BUILD_OPT_VSIM
#define	SUPPORT_VSIM_PERI_ONLY
#endif	//BUILD_OPT_VSIM
//-----------------------------------------------------------------------

#define	MAX_CMD_LENGTH	16

//-----------------------------------------------------------------------
// Command Stack
//-----------------------------------------------------------------------

//static COMMAND_TREE_TYPE	*command_stack[MAX_CMD_STACK] RW_SEC_UNINIT;
//static unsigned int		cmd_stack_ptr RW_SEC_UNINIT;

static COMMAND_TREE_TYPE	*command_stack[MAX_CMD_STACK];
static unsigned int			cmd_stack_ptr = 0;


extern void	disable_dpm_mode(void);
extern int	isdigit(int c);

/* Root CMD Table : command_root_cmd.c */
extern const COMMAND_TREE_TYPE	cmd_root_list[];

extern unsigned char 	ble_combo_ref_flag;

//-----------------------------------------------------------------------
// Command Stack Functions
//-----------------------------------------------------------------------

int	cmd_stack_initialize(void)
{
	cmd_stack_ptr = 0; 								// Clear
	command_stack[cmd_stack_ptr] = (COMMAND_TREE_TYPE *)cmd_root_list ;
	return 1;
}

COMMAND_TREE_TYPE	*cmd_get_current_stack(void)
{
	if(cmd_stack_ptr == MAX_CMD_STACK){
		return NULL;
	}
	return command_stack[cmd_stack_ptr] ;
}


COMMAND_TREE_TYPE *cmd_get_top_stack(void)
{
	return command_stack[0] ;
}

int cmd_get_full_command_path(char *path)
{
	unsigned int i ;
	COMMAND_TREE_TYPE *current;

	if( path == NULL) {
		return 0;
	}

	if( cmd_stack_ptr == 0 && command_stack[cmd_stack_ptr] != cmd_root_list ){
		// Not Initialized
		return 0;
	}

	path[0] = '\0';

	for(i=0; i <= cmd_stack_ptr ; i ++){
		DRIVER_STRCAT(path, ((i==0)? "" : "/"));

		current = command_stack[i] ;
		if( current[0].cmd != NULL ){
			DRIVER_STRCAT(path, current[0].cmd);
		}
	}

	return 0 ;
}

COMMAND_TREE_TYPE 	*cmd_stack_action(int quick, COMMAND_TREE_TYPE *node)
{
	if( node == NULL ){
		return NULL;
	}

	switch(node->mark){
		case CMD_TOP_NODE:
			if(cmd_stack_ptr != 0 ){
				cmd_stack_ptr -- ;	// Move to Parent Node
			}
			break;
		case CMD_SUB_NODE:
			if(node->node == NULL) {
				return NULL;		// Error Case
			}
			if(quick == TRUE){	        // quick == TRUE,
				cmd_stack_ptr = 0;	//    Move to Root Node.
			}
			if(cmd_stack_ptr != MAX_CMD_STACK ){
				cmd_stack_ptr ++ ;
				command_stack[cmd_stack_ptr] = (COMMAND_TREE_TYPE *)(node->node) ;	// Move to Child Node
			}
			break;
		case CMD_ROOT_NODE:
			cmd_stack_initialize();		// Move to Root Node
			break;
		default:
			// No Action
			break;
	}

	return command_stack[cmd_stack_ptr];
}

void	print_full_path_name(char *prompt, char *buf, int buflen)
{
	unsigned int i;
	unsigned int buflen_t = (unsigned int)buflen;
	int n, len;

	n = da16x_snprintf(buf, buflen_t, "[%s", prompt);

	for(i= 1; i <= cmd_stack_ptr; i++ ){
		len = da16x_snprintf(&(buf[n]), (buflen_t - (unsigned int)n)
					, "%c%s"
					, ((i == 1) ? '/' : '.')
					, command_stack[i]->node->cmd );
		n = n + len ;
	}

	len = da16x_snprintf(&(buf[n]), (buflen_t - (unsigned int)n), "] # " );

	PUTS(buf);
}

//-----------------------------------------------------------------------
// Command Interface Functions
//-----------------------------------------------------------------------

static char *cmd_find_delimeter(char *cmd)
{
	char *tail;
	tail = cmd;

	while(*tail != '\0' && *tail != '.' ){
		tail ++;
	}

	if( cmd == tail ){
		return NULL;
	}
	return tail;
}

int cmd_interface_action(int argc, char *argv[])
{
	int		i;
	COMMAND_TREE_TYPE *current , *history ;
	void (*function)(int argc, char *argv[]);
	char		*head, *tail, bkup;

	//! Get Current Cmd-List
	history = cmd_get_current_stack();

	if (history == NULL || argc == 0) {
		return 0;
	}

	//! Search Cmd
	current = NULL ;

	head = argv[0];
	tail = NULL;

	while ((tail = cmd_find_delimeter(head)) != NULL ) {
		bkup = *tail;
		*tail = '\0';

		if (console_hidden_check( ) != FALSE) {
			for (i = 0; history[i].mark != CMD_NULL_NODE; i ++) {
				if ( DRIVER_STRCMP(history[i].cmd, head) == 0) {
					// Matched Cmd
					current = &(history[i]) ;
					history = cmd_stack_action(FALSE, &(history[i]));
					break;
				}
			}
		}

		if (current == NULL) {
			//! Search Top Cmd
			for (i = 0; cmd_root_list[i].mark != CMD_NULL_NODE; i++) {
				if (DRIVER_STRCMP(cmd_root_list[i].cmd, head) == 0) {
					// Matched Cmd
					if (cmd_root_list[i].mark == CMD_SUB_NODE) {
						cmd_stack_initialize();
					}

					history = cmd_get_current_stack();

					current = (COMMAND_TREE_TYPE *)&(cmd_root_list[i]) ;
					history = cmd_stack_action(TRUE, (COMMAND_TREE_TYPE *)&(cmd_root_list[i]));	// Top-Node
					break;
				}
			}

			if (current == NULL) {
				break;
			}
		}

		if (bkup == '\0') {
			break;
		} else {
			*tail = bkup;
			head = ++tail;
		}
	}

	if (current == NULL) {
		PRINTF(" No such command - type help\n");
		return 0;
	}

	if (history == NULL) {
		PRINTF(" No Sub-command list.\n");
		return 0;
	}

	//!  Run Cmd
	if (current->mark != CMD_FUNC_NODE) {
		// Print Current Node
		PRINTF("    Command-List is changed, \"%s\"\n", history->cmd );
	} else if (current->func == NULL) {
		// Report Malfunction
		PRINTF("	Command Action is NULL, %s\n", current->help );
	} else {
		if((current->mark == CMD_FUNC_NODE) && (console_hidden_check() == FALSE))
		{
			// Skip
		} else {
			char *orghead;
			// Run Command
			orghead = argv[0];
			argv[0] = head;

			function = current->func ;
			function(argc, argv);
			argv[0] = orghead;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------
// Common Functions
//-----------------------------------------------------------------------
/******************************************************************************
 *  cmd_dump_print ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
// DUMP Print Mode
#define puthexa(x)	( ((x)<10) ? (char)('0' + (x)) : (char)('A' + (x) - 10) )
#define	putascii(x) 	( ((x)<32) ? ('.') : ( ((x)>126) ? ('.') : (x) ) )

void cmd_dump_print(UINT32 addr, UCHAR *data, UINT length, UINT endian)
{
	UINT32  i, h, s, spc, trim, lmask;
	UINT8  *data8;
	char	*hexadump;
	char	*strdump;
	char	hexa;

	data8 = (UINT8 *)data;
	endian = (endian&0x0f);
	endian = (endian == 0) ? 1 : endian ;

	lmask = (endian == 4) ? 0x03 : ((endian == 2)? 0x01 : 0 );
	lmask = ~lmask;

	hexadump = (char *)pvPortMalloc(52);
	strdump = (char *)pvPortMalloc(20);

	for(i=0, h=0, s =0; i<length; i++){
		trim = i & (~lmask);
		hexa = (char)data8[(i&lmask)|(~(trim|lmask))] ;

		hexadump[h++] = puthexa( ((hexa>>4)&0x0f) );
		hexadump[h++] = puthexa( ((hexa)&0x0f) );
		if( trim == (endian-1) ){
			hexadump[h++] = ' ';
		}

		strdump[s++] = putascii( hexa );

		if( ((i%16) == 15) || ((i+1) == length) ){
			hexadump[h] = '\0';
			strdump[s] = '\0';

			PRINTF("[%08lX] : ", (addr+i-(i%16)) );
			PRINTF(hexadump);
			for( spc = (17 - (i%16)); spc > 0; spc-- ){
				PRINTF("   ");
			}
			PRINTF("%s", strdump);
			PRINTF("\n");
			h = 0;
			s = 0;
		}
	}

	vPortFree(hexadump);
	vPortFree(strdump);
}

/******************************************************************************
 *  cmd_help_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_help_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	int		i;
	COMMAND_TREE_TYPE *history, *top ;
	char	*pathname;

	i = 0;
	//! Get Top Cmd-List
	top = cmd_get_top_stack();
	if(top == NULL ){
		return ;
	}
	//! Get Current Cmd-List
	history = cmd_get_current_stack();
	if(history == NULL ){
		return ;
	}
	//! Print CMD-List
	{
		pathname = (char *)pvPortMalloc(256);

		cmd_get_full_command_path(pathname);
		PRINTF("\n--------------------------------------------------\n" );
		PRINTF(" Current CMD-List is \"%s\"\n", pathname );
		PRINTF("--------------------------------------------------\n\n" );
		if( top != history ){
			// If current is not top,
			for(i=0; top[i].mark != CMD_NULL_NODE; i ++ ){
				if(DRIVER_STRLEN(top[i].cmd) > MAX_CMD_LENGTH ){
					DRIVER_STRCPY(pathname, top[i].cmd);
				}else{
					DRIVER_SPRINTF(pathname, "%s                ", top[i].cmd);
					pathname[MAX_CMD_LENGTH] = '\0';
				}
				PRINTF("%s : %s\n", pathname, top[i].help );
			}
			PRINTF("\n--------------------------------------------------\n\n" );
		}
		for(i=0; history[i].mark != CMD_NULL_NODE; i ++ ){
			if(DRIVER_STRLEN(history[i].cmd) > MAX_CMD_LENGTH ){
				DRIVER_STRCPY(pathname, history[i].cmd);
			}else{
				DRIVER_SPRINTF(&(pathname[0]), "%s                ", history[i].cmd);
				pathname[MAX_CMD_LENGTH] = '\0';
			}
			PRINTF("%s : %s\n", pathname, history[i].help );
		}
		PRINTF("\n" );

		vPortFree(pathname);
	}
}

/******************************************************************************
 *  cmd_echo_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_echo_func(int argc, char *argv[])
{
	int 	i;

	for(i=1; i < argc; i ++ ){
		PRINTF("%s%s", ((argc==1) ? "" : " "), argv[i] );
	}
	PRINTF("\n" );
}

/******************************************************************************
 *  cmd_repeat_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_repeat_func(int argc, char *argv[])
{
	int 	count;

	if(argc < 3){
		PRINTF("usage : %s [count] command...\n", argv[0]);
		return ;
	}
	count = (int)ctoi(argv[1]);

	for(; count > 0; count --) {
		int ch;
		ch = GETC_NOWAIT() ;
		if ( ch == 0x03 ) { /* CTRL+C */
			PRINTF("\nBreaked\n");
			return;
		}

		PRINTF("### LOOP COUNT:%d ###\n", ctoi(argv[1]) - ((unsigned int)count-1));
		cmd_interface_action(argc-2, &(argv[2]) );
	}
}

/******************************************************************************
 *  cmd_hidden_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_hidden_func(int argc, char *argv[])
{
	int count;

	if(argc < 2){
		return ;
	}

	count = (int)htoi(argv[1]);

	//console_hidden_mode( count );
	if( count == 1 ){
		trc_que_proc_conmode( TRACE_MODE_ON|TRACE_MODE_ECHO );
	}else{
		trc_que_proc_conmode( TRACE_MODE_OFF|TRACE_MODE_ECHO );
	}
}

/******************************************************************************
 *  cmd_passwd_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_passwd_func(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	trc_que_proc_conmode( TRACE_MODE_ON|TRACE_MODE_PASSWD );
	while( GETC() != '\r' );
	trc_que_proc_conmode( TRACE_MODE_OFF|TRACE_MODE_PASSWD );
	PRINTF("\n");
}

/******************************************************************************
 *  cmd_logview_func ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_logview_func(int argc, char *argv[])
{
	if( argc == 2 ){
		UINT32 mode;

		if( DRIVER_STRCMP("on", argv[1]) == 0 ){
			mode = TRACE_CMD_STATE | TRUE ;
		}else
		if( DRIVER_STRCMP("off", argv[1]) == 0 ){
			mode = TRACE_CMD_STATE | FALSE ;
		}else
		if( DRIVER_STRCMP("last", argv[1]) == 0 ){
			mode = TRACE_CMD_MODE | TRUE ;
		}else
		if( DRIVER_STRCMP("first", argv[1]) == 0 ){
			mode = TRACE_CMD_MODE | FALSE ;
		}else
		if( DRIVER_STRCMP("info", argv[1]) == 0 ){
			mode = TRACE_CMD_DEBUG ;
		}else{
			mode = ctoi(argv[1]);
			mode = mode | TRACE_CMD_LEVEL ;
		}

		PRINTF("%s: tarce %x\n", __func__, mode );
		trc_que_proc_active( mode );
	}
	else
	{
		PRINTF("CON Max Usage - %d\n", trc_que_proc_getusage() );
	}
}

extern void do_set_all_timer_kill(void);
extern void RTC_CLEAR_RETENTION_FLAG(void);

//#include "ramsymbols.h"

#if defined (XIP_CACHE_BOOT)
extern int dpm_get_wakeup_source(void);
void clear_wakeup_src_all(void)
{
    WAKEUP_SOURCE wakeup_source;
    wakeup_source = (WAKEUP_SOURCE)dpm_get_wakeup_source();
	if (((uint8_t)wakeup_source) & ((uint8_t)WAKEUP_SENSOR))
	{
		/* Check which wakeup source occur */
		UINT32 source, gpio_source, i;
		source = RTC_GET_AUX_WAKEUP_SOURCE();
		// PRINTF("aux wakeup source %x\n", source);
		if ((source & WAKEUP_GPIO) == WAKEUP_GPIO)	  // GPIO wakeup
		{
			gpio_source = RTC_GET_GPIO_SOURCE();
			//PRINTF("aux gpio wakeup %x\n", gpio_source);
			RTC_CLEAR_EXT_SIGNAL();
			RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
			gpio_source |= (0x3ff);
			RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
			//check zero
			for (i = 0; i < 10; i++)
			{
				RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
				if ((gpio_source &0x3ff) == 0x00)
				{
					break;
				}
				else
				{
					//PRINTF("gpio wakeup clear %x\n", gpio_source);
				}
			}
			gpio_source &= (UINT32)(~(0x3ff));
			RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
			for (i = 0; i < 10; i++)
			{
				RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
				if ((gpio_source &0x3ff) == 0x00)
				{
					break;
				}
				else
				{
					//PRINTF("gpio wakeup clear %x\n", gpio_source);
				}
			}
			RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
		}
	}
	RTC_CLEAR_EXT_SIGNAL();
}
#endif // XIP_CACHE_BOOT

void reboot_func(UINT flag)
{
#ifdef __ENABLE_UNUSED__
	UINT	status;
#endif	//__ENABLE_UNUSED__

	int	i;

#if defined (XIP_CACHE_BOOT)
	clear_wakeup_src_all();
#endif

#ifdef __ENABLE_UNUSED__
	extern void hal_machw_reset(void);
	extern void check_environ(void);

	PRINTF("\n");
	vTaskDelay(10);

	/* Check Wi-Fi RamLib running state */
	ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;

	/* check RAM-library offset if it really exist. */
	if (   ((ramlib->magic & DA16X_RAMSYM_MAGIC_MASK) != DA16X_RAMSYM_MAGIC_TAG)
	    || (ramlib->checkramlib(ramlib->build) != TRUE)) {
		ramlib_chk_status = FALSE;
	} else {
		ramlib_chk_status = TRUE;

		/* In case of DPM Wakeup, do not disconnect */
		if (!chk_dpm_wakeup()) {
				char reply[5] = {0, };

				da16x_cli_reply("flush", NULL, reply);
				vTaskDelay(100);
		}
	}

	if (flag != SYS_RESET) {
		/* Check NVRAM operation ... */
		//check_environ();

		/* Prohibit thread preemption */
		cur_thread = tx_thread_identify();
		status = tx_thread_preemption_change(cur_thread, 0, &old_threshold);
		if (status != ERR_OK) {
			PRINTF("[%s] Error to tx_thread_preemption_change() ...\n", __func__);
			//vTaskDelay(100);
		}
		status = tx_thread_priority_change(cur_thread, 0, &old_threshold);
		if (status != ERR_OK) {
			PRINTF("[%s] Error to tx_thread_priority_change() ...\n", __func__);
			//vTaskDelay(10);
		}

		tx_interrupt_control(TX_INT_DISABLE);
		kill_all_tx_timer();
		kill_all_tx_thread();
		tx_interrupt_control(TX_INT_ENABLE);

		for (i = 0; i < 0x200000; i++) __NOP();
	}

	if (ramlib_chk_status == TRUE) {
#ifndef	__LIGHT_SUPPLICANT__
		/*
		 * When P2P GO PS is on and issue reboot cmd,
		 *	sometimes SW is going to be hang in hal_machw_reset().
		 *	So need to turn off P2P GO PS before reboot operation.
		 *
		 * Note: When the interface removed,
		 *	fc80211_p2p_go_ps_on_off() doesn't work/need/matter.
		 */
		if (   get_run_mode() == 3		// P2P GO fixed mode
		    || get_run_mode() == 2) {		// P2P only mode
			fc80211_p2p_go_ps_on_off(1, 0);	// Trun off P2P GO PS.
			vTaskDelay(100);
		}
#endif	/* __LIGHT_SUPPLICANT__ */

	        hal_machw_reset();

		for (i = 0; i < 0x400000; i++) __NOP();
	}
#endif	//__ENABLE_UNUSED__

	switch (flag)
	{
		case SYS_RESET :
			SYSSET_SWVERSION(0xFFFFC000);
			break;

		case SYS_REBOOT_POR:	/* Reboot  por */
		case SYS_REBOOT :		/* Reboot */
			SYSSET_SWVERSION(0x00000000);
			break;
	}

#if defined(XIP_CACHE_BOOT)
	/* RTC Clearing */
	{
		/* REBOOT Patch */
		UINT32 	temp_addr = 0x50091028;

		*(UINT *)(temp_addr) = 0;
		for (i = 0; i < 0x100000; i++) __NOP();
		*(UINT *)(temp_addr) = 2;

		/* Timer Clearing  */
		do_set_all_timer_kill();

		/* Retention memory clearing */
		RTC_CLEAR_RETENTION_FLAG();
		RTC_READY_POWER_DOWN(1);
	}
#endif

	// By below thread_sleep, BootRom hardware fault(ffff00f1) might be happened,
	// So do as comment
	//vTaskDelay(20);

	/*
	 * Instead of OAL_SYS_RESET() function call,,,
	 */

	_sys_nvic_close();
#if 0	/* by Bright */	// Remove unnessary operation
	da16x_sysdbg_dbgt_backup();
#endif	/* 0 */
	NVIC_SystemReset();
}

void cmd_reboot_func(int argc, char *argv[])
{
#if defined(XIP_CACHE_BOOT)
	if (ble_combo_ref_flag == pdTRUE) {
		extern void combo_ble_sw_reset(void);
		combo_ble_sw_reset();
	}
#endif // XIP_CACHE_BOOT

	if (DRIVER_STRCMP("reset", argv[0]) == 0) {
		reboot_func(SYS_RESET);	/* Reset */
	} else if ((argc == 2) && (DRIVER_STRCMP("por", argv[1]) == 0)) {
		reboot_func(SYS_REBOOT_POR);	/* Reboot & por */
	} else {
		reboot_func(SYS_REBOOT);	/* Reboot */
	}
}

void cmd_set_boot_idx(int argc, char *argv[])
{
	int	index;

	extern UCHAR set_boot_index(int index);

	if (argc != 2) {
		PRINTF("Usage : boot_idx [0 | 1]\n");
		return;
	}

	index = atoi(argv[1]);
	if (index < 0 && index > 1) {
		PRINTF("Usage : boot_idx [0 | 1]\n");
		return;
	}

	set_boot_index(index);
}

#if defined (__BLE_COMBO_REF__)
extern int combo_set_sleep2(char* context, int rtm, int seconds);
#endif // __BLE_COMBO_REF__

void cmd_setsleep(int argc, char *argv[])
{
	unsigned long long msec = 0x0ULL;
	int rtm_on = 0;

	/* Help */
	if (argc == 2 && (strcasecmp(argv[1], "HELP") == 0)) {
help:
		PRINTF("%s : Sleep 2 Mode commands\n", argv[0]);
		PRINTF("\t%s 2 [SleepTime(msec.)]       : Sleep mode 2 & RTM Off\n", argv[0]);
		PRINTF("\t%s 2 [SleepTime(msec.)] [1|0] : Sleep mode 2 & RTM [On/Off]\n", argv[0]);
		PRINTF("\n");

		return;
	} else if ((strcasecmp(argv[1], "2") == 0) && (3 == argc || argc == 4)) {
		char *tmp_str;
		int	i;

		//Check second parameter
		tmp_str = &argv[2][0];
		for (i = 0; i < (int)strlen(tmp_str); i++) {
			if (!isdigit((int)(tmp_str[i]))) {
				goto help;
			}
		}
		tmp_str = NULL;

		msec = atoi(argv[2]);
		if (is_in_valid_range(msec, 0, 0x7CFFFC18) == pdFALSE) {
			goto help;
		}

		//Check RTM parameter
		if (argc == 4) {
			tmp_str = &argv[3][0];
			for (i = 0; i < (int)strlen(tmp_str); i++) {
				if (!isdigit((int)(tmp_str[i]))) {
					goto help;
				}
			}
			tmp_str = NULL;

			rtm_on = atoi(argv[3]);
			if (is_in_valid_range(rtm_on, 0, 1) == pdFALSE) {
				goto help;
			}
		} else {
			rtm_on = 0;
		}

#if defined (__BLE_COMBO_REF__)
		int tmp_sec = 0;
		tmp_sec = msec/1000;

		int resul = combo_set_sleep2(NULL, rtm_on, tmp_sec);
		if (resul == 0) {
			return;
		} else if (resul < 0) {
			goto help;
		}
#endif // __BLE_COMBO_REF__

		/* Disable DPM running mode */
		if (rtm_on == 0) {
			disable_dpm_mode();
			PRINTF("Activate Sleep mode 2 & RTM Off (%d msec.)\n", msec);
		} else {
			PRINTF("Activate Sleep mode 2 & RTM On (%d msec.)\n", msec);
		}

		// Enter Sleep mode 2
		dpm_sleep_start_mode_2(msec * 1000ULL, rtm_on);
	} else {
		goto help;
	}

	return ;
}

/* EOF */
