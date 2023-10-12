/**
 ****************************************************************************************
 *
 * @file console.h
 *
 * @brief Console
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


#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


//--------------------------------------------------------------------
//	Definitions
//--------------------------------------------------------------------

#define	SUPPORT_DA16X_LONGLONG

#define	CON_BUF_SIZ			256
#define	CON_LBUF_SIZ		128
#define ESC_KEY				0x1b
#define CON_MAX_VSPRINTF	4096
#define	CON_DEFAULT_LFEED	1	/* 1 : true , 0 : false */

//--------------------------------------------------------------------
//	Declarations
//--------------------------------------------------------------------

extern void	console_hidden_mode(int mode);
extern void	console_init(unsigned int port, unsigned int baud, unsigned int clock);
extern int	console_hidden_check(void);
extern int	console_deinit(unsigned int waitcnt);

extern void	Printf(const char *fmt,...);
extern void	DBGT_VPrintf(const char *fmt, va_list args);
extern void	DBGT_Printf(const char *fmt,...);

extern int	putchar(int ch);
extern int	putrawchar(int ch);
extern int	getchar(void);
extern int	getrawchar(int mode);
extern int	getrawstr(char *buf, int length);

extern int 	da16x_printf(const char *fmt,...);

extern int 	da16x_vsnprintf(char *buf, size_t n, int linefeed, const char *fmt, va_list args);
#define	da16x_vsprintf(buf, ...)	da16x_vsnprintf(buf, CON_MAX_VSPRINTF, 0, __VA_ARGS__ )

extern int 	da16x_snprintf_lfeed(char *buf, size_t n, int linefeed, const char *fmt, ...);
extern int	da16x_putstring(char *s, int len);

extern int 	da16x_sprintf(char *buf, const char *fmt, ...);
extern int 	da16x_vprintf(const char *format, va_list arg);
#define	da16x_snprintf(buf, n, ...)  da16x_snprintf_lfeed(buf, n, 0, __VA_ARGS__ )

extern int da16x_asprintf (char **buf, const char *fmt, ...);
extern int da16x_vasprintf (char **buf, const char *fmt, va_list arg);

extern HANDLE	da16x_console_handler(void);

extern int 	da16x_console_txempty(void);
extern int 	da16x_console_rxempty(void);

/**
 ****************************************************************************************
 * @brief Control console feature (on/off). it should be used to control console in user application. \n
 *        And it takes effect after reboot.
 * @param[in] _console_ctrl     Console control flag
 * @return                      None
 ****************************************************************************************
 */
 
extern void console_control(int _console_ctrl);

/**
 ****************************************************************************************
 * @brief Set UART register to make enable console
 * @param[in]  None
 * @return 0 - succeed to set
 ****************************************************************************************
 */

extern int console_on(void);

/**
 ****************************************************************************************
 * @brief Set UART register to make disable console
 * @param[in]  None
 * @return 0 - succeed to set
 ****************************************************************************************
 */

extern int console_off(void);

/**
 ****************************************************************************************
 * @brief Set flag to make enable console
 * @param[in]     None
 * @return        None
 ****************************************************************************************
 */

extern void console_enable(void);

/**
 ****************************************************************************************
 * @brief Set flag to make disable console
 * @param[in]     None
 * @return        None
 ****************************************************************************************
 */

extern void console_disable(void);




#if defined	( __CONSOLE_CONTROL__ )
#define __UART_lowlevel_Printf(...) {           \
    extern UINT    console_ctrl;                \
    if (console_ctrl) {                         \
        UART_lowlevel_Printf(__VA_ARGS__);      \
    }                                           \
    else {                                      \
    }                                           \
}
#else
#define __UART_lowlevel_Printf(...) UART_lowlevel_Printf(__VA_ARGS__);
#endif	/* __CONSOLE_CONTROL__ */

#endif  /*__CONSOLE_H__*/

/* EOF */
