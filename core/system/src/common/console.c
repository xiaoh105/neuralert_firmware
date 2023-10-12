/**
 ****************************************************************************************
 *
 * @file console.c
 *
 * @brief Console
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2020-2022 Modified by Renesas Electronics.
 ****************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hal.h"
#include "console.h"
#include "driver.h"

#if defined	( __CONSOLE_CONTROL__ )
#include "sys_feature.h"
#include "da16x_dpm_regs.h"
#include "user_dpm_api.h"
#endif	/* __CONSOLE_CONTROL__ */

/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/

#if	defined(SUPPORT_SYSUART)
#undef	USE_PL011
#elif	defined(SUPPORT_PL011UART)
#define	USE_PL011
#else
#undef	USE_PL011
#endif

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

#undef	NOFLOAT
#ifdef	SUPPORT_DA16X_LONGLONG
#define da16x_num_type   long long
#else	//SUPPORT_DA16X_LONGLONG
#define da16x_num_type   long
#endif	//SUPPORT_DA16X_LONGLONG


#ifdef USE_PL011
static HANDLE con_uart0 = NULL;
#endif

#define ZEROPAD 1               // Pad with zero
#define SIGN    2               // Unsigned/signed long
#define PLUS    4               // Show plus
#define SPACE   8               // Space if plus
#define LEFT    16              // Left justified
#define SPECIAL 32              // 0x
#define LARGE   64              // Use 'ABCDEF' instead of 'abcdef'

#define CLK2US(clk)			((((unsigned long long )clk) * 15625ULL) >> 9ULL)
#define US2CLK(us)			((((unsigned long long ) us) << 9ULL) / 15625ULL)

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static const char g_digits[]
	= "0123456789abcdefghijklmnopqrstuvwxyz"
	  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static UINT	console_hidden_flag = FALSE;	// FALSE is hidden mode

//static	char *uart_print_buf RW_SEC_UNINIT;
//static	char *uart_lowprint_buf RW_SEC_UNINIT;
static	char uart_print_buf[CON_BUF_SIZ];
static	char uart_lowprint_buf[CON_LBUF_SIZ];

#if defined	( __CONSOLE_CONTROL__ )
UINT    console_ctrl;
#define	OAL_MSLEEP(wtime)		{							\
			portTickType xFlashRate, xLastFlashTime;		\
			xFlashRate = wtime/portTICK_RATE_MS;			\
			xLastFlashTime = xTaskGetTickCount();			\
			vTaskDelayUntil( &xLastFlashTime, xFlashRate );	\
		}
#endif	/* __CONSOLE_CONTROL__ */

/******************************************************************************
 *
 *  sub libraries
 *
 ******************************************************************************/

void	console_init(UINT32 port, UINT32 baud, UINT32 clock)
{

#ifndef USE_PL011
	_sys_uart_create();
	_sys_uart_init(baud, clock);
#else	//USE_PL011
	UINT32 temp;

	con_uart0 = UART_CREATE((UART_UNIT_IDX)port);
	UART_IOCTL(con_uart0, UART_SET_CLOCK, &clock);
	UART_IOCTL(con_uart0, UART_SET_BAUDRATE, &baud);

	temp = UART_WORD_LENGTH(8) | UART_FIFO_ENABLE(1) | UART_PARITY_ENABLE(0) | UART_EVEN_PARITY(0) /*parity*/ | UART_STOP_BIT(1) ;
	UART_IOCTL(con_uart0, UART_SET_LINECTRL, &temp);
	temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(0);
	UART_IOCTL(con_uart0, UART_SET_CONTROL, &temp);

	temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT | UART_INTBIT_TIMEOUT;
	UART_IOCTL(con_uart0, UART_SET_INT, &temp);

	temp = 0;
	UART_IOCTL(con_uart0, UART_SET_USE_DMA, &temp);

	/* tx fifo interrupt level�� one/eighth ���� �����ϸ� console ���� ���� data�� ����???�� �� ������ */
	/* command�� help ��ɾ�� �� ���� �� seven/eight ���� �� 884us ���� (�� ���� ���ͷ�Ʈ �߻�) */
	temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) |UART_TX_INT_LEVEL(UART_ONEEIGHTH);
	UART_IOCTL(con_uart0, UART_SET_FIFO_INT_LEVEL, &temp);

	UART_INIT(con_uart0);
#endif
	//uart_print_buf = (char *)SAL_MALLOC( CON_BUF_SIZ );
	//uart_lowprint_buf = (char *)SAL_MALLOC( CON_LBUF_SIZ );
	uart_print_buf[0] = '\0';
	uart_lowprint_buf[0] = '\0';
	console_hidden_flag = FALSE;
}

int	console_deinit(UINT32 waitcnt)
{
	volatile UINT32 itercnt = waitcnt;
	UINT32 currport = UART_UNIT_0;
#ifdef USE_PL011
	if (con_uart0 != NULL ){
		UINT32 ioctldata = UART_DISABLE;
		UART_IOCTL(con_uart0, UART_SET_CONTROL, &ioctldata);
		UART_IOCTL(con_uart0, UART_GET_PORT, &currport);
		UART_CLOSE(con_uart0);
		con_uart0 = NULL;
	}
#else
	_sys_uart_close();
#endif	//USE_PL011
	while( itercnt-- > 0 );

	return (int)currport;
}

static int da16x_toupper(int c)
{
	#define islower(c)     (c >= (int)'a' && c<= (int)'z')
	return islower(c) ? (c ^ 0x20) : c;
}

static size_t da16x_strnlen(const char *s, size_t count)
{
  const char *sc;
  for (sc = s; *sc != '\0' && count--; ++sc);
  return sc - s;
}

static int skip_atoi(const char **s)
{
	int i = 0;
	while (is_digit(**s)) i = i*10 + *((*s)++) - '0';
	return i;
}

static char *number(char *str, da16x_num_type num, int base, int size, int precision, int type)
{
	char tmp[68];
	char sign;
	char *dig = (char *)g_digits;
	int i;

	if (type & LARGE)  dig = (char *)&(g_digits[36]); //upper_digits
	if (type & LEFT) type &= ~ZEROPAD;
	if (base < 2 || base > 36) return 0;

	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		}
		else if (type & PLUS) {
			sign = '+';
			size--;
		}
		else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}

	if (type & SPECIAL) {
		if (base == 16){
			size -= 2;
		}
		else if (base == 8) {
			size--;
		}
	}

	i = 0;

	if (num == 0){
		tmp[i++] = '0';
	} else {
		while (num != 0) {
			tmp[i++] = dig[((unsigned long long) num) % (unsigned) base];
			num = ((unsigned long long) num) / (unsigned) base;
		}
	}

	if (i > precision){
		precision = i;
	}
	size -= precision;
	if (!(type & (ZEROPAD | LEFT))){
		while (size-- > 0) *str++ = ' ';
	}
	if (sign){
		*str++ = sign;
	}

	if (type & SPECIAL){
		if (base == 8){
			*str++ = '0';
		}
		else if (base == 16) {
			*str++ = '0';
			*str++ = g_digits[33];
		}
	}

	if (!(type & LEFT)) {
		if ((type & ZEROPAD) == ZEROPAD ){
			while (size-- > 0) *str++ = '0';
		}else{
			while (size-- > 0) *str++ = ' ';
		}
	}
	while (i < precision--) *str++ = '0';
	while (i-- > 0) *str++ = tmp[i];
	while (size-- > 0) *str++ = ' ';

	return str;
}

static char *eaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
	DA16X_UNUSED_ARG(precision);

	char tmp[24];
	char *dig = (char *)g_digits;
	int i, len;

	if (type & LARGE){
		dig = (char *)&(g_digits[36]); //upper_digits
	}
	len = 0;
	for (i = 0; i < 6; i++)	{
		if (i != 0) {
			tmp[len++] = ':';
		}
		tmp[len++] = dig[addr[i] >> 4];
		tmp[len++] = dig[addr[i] & 0x0F];
	}

	if (!(type & LEFT)){
		while (len < size--) *str++ = ' ';
	}
	for (i = 0; i < len; ++i) {
		*str++ = tmp[i];
	}
	while (len < size--) *str++ = ' ';

	return str;
}

static char *iaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
	DA16X_UNUSED_ARG(precision);

	char tmp[24];
	int i, n, len;

	len = 0;
	for (i = 0; i < 4; i++)	{
		if (i != 0) {
			tmp[len++] = '.';
		}
		n = addr[i];

		if (n == 0) {
			tmp[len++] = g_digits[0];
		} else	{
			if (n >= 100) {
				tmp[len++] = g_digits[n / 100];
				n = n % 100;
				tmp[len++] = g_digits[n / 10];
				n = n % 10;
			}
			else if (n >= 10) {
				tmp[len++] = g_digits[n / 10];
				n = n % 10;
			}
			tmp[len++] = g_digits[n];
		}
	}

	if (!(type & LEFT)) {
		while (len < size--) *str++ = ' ';
	}
	for (i = 0; i < len; ++i) {
		*str++ = tmp[i];
	}
	while (len < size--) *str++ = ' ';

	return str;
}

#ifndef NOFLOAT

static void da16x_memcpy(void *dst, void *src, size_t size)
{
	char	*src8, *dst8;

	src8 = (char *)src;
	dst8 = (char *)dst;

	while(size-- > 0 ){
		*dst8++ = *src8++ ;
	}
}

char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);

static void cfltcvt(double value, char *buffer, char fmt, int precision)
{
	int decpt, sign, exp, pos;
	char *digits = NULL;
	char cvtbuf[80];
	int capexp = 0;
	int magnitude;

	if (fmt == 'G' || fmt == 'E') {
		capexp = 1;
		fmt += 'a' - 'A';
	}

	if (fmt == 'g') {
		digits = ecvtbuf(value, precision, &decpt, &sign, cvtbuf);
		magnitude = decpt - 1;
		if (magnitude < -4  ||  magnitude > precision - 1) {
			fmt = 'e';
			precision -= 1;
		} else {
			fmt = 'f';
			precision -= decpt;
		}
	}

	if (fmt == 'e')	{
		digits = ecvtbuf(value, precision + 1, &decpt, &sign, cvtbuf);

		if (sign) {
			*buffer++ = '-';
		}
		*buffer++ = *digits;
		if (precision > 0) {
			*buffer++ = '.';
		}
		da16x_memcpy(buffer, digits + 1, precision);
		buffer += precision;
		*buffer++ = capexp ? 'E' : 'e';

		if (decpt == 0)	{
			exp = (value == 0.0) ? 0 : -1;
		} else {
			exp = decpt - 1;
		}

		if (exp < 0) {
			*buffer++ = '-';
			exp = -exp;
		} else {
			*buffer++ = '+';
		}

		buffer[2] = (char)((exp % 10) + '0');
		exp = exp / 10;
		buffer[1] = (char)((exp % 10) + '0');
		exp = exp / 10;
		buffer[0] = (char)((exp % 10) + '0');
		buffer += 3;
	}
	else if (fmt == 'f') {
		digits = fcvtbuf(value, precision, &decpt, &sign, cvtbuf);
		if (sign) { *buffer++ = '-'; }
		if (*digits) {
			if (decpt <= 0)
			{
				*buffer++ = '0';
				*buffer++ = '.';
				for (pos = 0; pos < -decpt; pos++) {
					*buffer++ = '0';
				}
				while (*digits) *buffer++ = *digits++;
			}
			else
			{
				pos = 0;
				while (*digits) {
				  if (pos++ == decpt) {
				  	*buffer++ = '.';
				  }
				  *buffer++ = *digits++;
				}
			}
		}
		else
		{
			*buffer++ = '0';
			if (precision > 0) {
				*buffer++ = '.';
				for (pos = 0; pos < precision; pos++) {
					*buffer++ = '0';
				}
			}
		}
	}

	*buffer = '\0';
}

static void forcdecpt(char *buffer)
{
	while (*buffer) {
		if (*buffer == '.') {
			return;
		}
		if (*buffer == 'e' || *buffer == 'E') {
			break;
		}
		buffer++;
	}

	if (*buffer) {
		int n = strlen(buffer);
		while (n > 0) {
			buffer[n + 1] = buffer[n];
			n--;
		}

		*buffer = '.';
	} else {
		*buffer++ = '.';
		*buffer = '\0';
	}
}

static void cropzeros(char *buffer)
{
	char *stop;

	while (*buffer && *buffer != '.')
		buffer++;

	if (*buffer++) {
		while (*buffer && *buffer != 'e' && *buffer != 'E') {
			buffer++;
		}

		stop = buffer--;

		while (*buffer == '0')
			buffer--;

		if (*buffer == '.') {
			buffer--;
		}

		while ((*++buffer = *stop++) != 0)
			;	//@suppress("Assignment in condition")
	}
}

static char *flt(char *str, double num, int size, int precision, char fmt, int flags)
{
	char tmp[80];
	char c, sign;
	int n, i;

	// Left align means no zero padding
	if (flags & LEFT) flags &= ~ZEROPAD;

	// Determine padding and sign char
	c = (flags & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (flags & SIGN) {
		if (num < 0.0) 	{
			sign = '-';
			num = -num;
			size--;
		}
		else if (flags & PLUS) 	{
			sign = '+';
			size--;
		}
		else if (flags & SPACE) {
			sign = ' ';
			size--;
		}
	}

	// Compute the precision value
	if (precision < 0) {
		precision = 6; // Default precision: 6
	}
	else if (precision == 0 && fmt == 'g') {
		precision = 1; // ANSI specified
	}

	// Convert floating point number to text
	cfltcvt(num, tmp, fmt, precision);

	// '#' and precision == 0 means force a decimal point
	if ((flags & SPECIAL) && precision == 0) {
		forcdecpt(tmp);
	}

	// 'g' format means crop zero unless '#' given
	if (fmt == 'g' && !(flags & SPECIAL)) {
		cropzeros(tmp);
	}

	n = strlen(tmp);

	// Output number with alignment and padding
	size -= n;
	if (!(flags & (ZEROPAD | LEFT))) {
		while (size-- > 0) *str++ = ' ';
	}
	if (sign) {
		*str++ = sign;
	}
	if (!(flags & LEFT)) {
		while (size-- > 0) *str++ = c;
	}
	for (i = 0; i < n; i++) {
		*str++ = tmp[i];
	}
	while (size-- > 0) *str++ = ' ';

	return str;
}

#endif

static char *da16x_string(char *buf, size_t ssize, char *s, int field_width, int precision, int flags, int linefeed)
{
	int len, i;
	if (s == 0)
	{
		//s = "<NULL>";
		s = "";
	}

	len = da16x_strnlen(s, precision);
	ssize = ssize - 1;

	if (!(flags & LEFT))
	{
		while ((len < field_width--) && ssize)
		{
			*buf++ = ' ';
			ssize--;
		}
	}

	for (i = 0; (i < len) && ssize; ++i)
	{
		if (linefeed == 1 && *s == '\n' && (*(buf-1) != '\r'))
		{
			*buf++ = '\r';
			ssize--;
			*buf++ = *s++;
			ssize--;
		}
		else
		{
			if (flags & LARGE)
			{
				*buf++ = (char)da16x_toupper(*s++);
			}
			else
			{
				*buf++ = *s++;
			}
			ssize--;
		}
	}

	while ((len < field_width--) && ssize)
	{
		*buf++ = ' ';
		ssize--;
	}

	if (ssize == 0 )
	{
		*(buf-1) = '.';
		*(buf-2) = '.';
		*(buf-3) = '.';
	}

	return buf;
}

static int udelay(long usecs)
{
	unsigned long long end_time = 0;
	unsigned long long curent_time = 0;
	unsigned long long wait_clk = US2CLK(usecs);

	// Convert microseconds to performance counter units per move.
	curent_time = RTC_GET_COUNTER();
	end_time = curent_time + wait_clk;

	do {
		//SYSUSLEEP(1);
		curent_time = RTC_GET_COUNTER();
	} while (curent_time < end_time);

	return (int)CLK2US(curent_time - end_time);
}

/******************************************************************************
 *  da16x_vsnprintf
 *
 *  Purpose :   vsnprintf
 *  Input   :   n, maximum number of bytes to be used in the buffer
 *              fmt, C string that contains a format string
 *              args, values identifying a variable arguments list initialized
 *                    with va_start.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/

int da16x_vsnprintf(char *buf, size_t n, int linefeed,  const char *fmt, va_list args)
{
	unsigned da16x_num_type num;
	int len;	/* temporary */
	int base;
	char *str;
	char *endstr;	/* end of string */
	char *s;	/* temporary */
	int flags;	// Flags to number()
	int field_width;// Width of output field
	int precision;  // Min. # of digits for integers;
			// max number of chars for from string
	int qualifier;	// 'h', 'l', or 'L' for integer fields
	int truncate;	/* temporary */

	if (n < 1 ){
		*buf = '\0';
		return 0;
	}

	endstr = (char *)((UINT32)buf + n - 1);

	for (str = buf; *fmt; fmt++)
	{
		if (str == endstr )
		{
			break;
		}

		if (*fmt != '%')
		{
			if (linefeed == 0 ){
				do {
					if (str == endstr ){  break;  }
					if (*fmt == '\0' ){ break; }
					*str++ = *fmt++;
				} while(*fmt != '%');
			}
			else
			{
				do {
					if (str == endstr)
					{
						break;
					}

					if (*fmt == '\0')
					{
						break;
					}

					if (*fmt == '\n' && (str == buf || *(str-1) != '\r'))
					{
						*str++ = '\r';

						if (str == endstr)
						{
							break;
						}
					}
					*str++ = *fmt++;

				} while(*fmt != '%');
			}

			if (*fmt != '%' ){
				fmt--;
				continue;
			}
		}

		// Process flags
		flags = 0;
		truncate = 1; /* temporary flag */

		do{
			fmt++; // This also skips first '%'
			switch (*fmt)
			{
			case '-': flags |= LEFT; break;
			case '+': flags |= PLUS; break;
			case ' ': flags |= SPACE; break;
			case '#': flags |= SPECIAL; break;
			case '0': flags |= ZEROPAD; break;
			default : truncate = 0; break;
			}
		}while(truncate==1);

		// Get field width
		field_width = -1;
		if (is_digit(*fmt)) {
			field_width = skip_atoi(&fmt);
		}
		else if (*fmt == '*') {
			fmt++;
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		// Get the precision
		precision = -1;
		if (*fmt == '.')
		{
			++fmt;
			if (is_digit(*fmt)) {
				precision = skip_atoi(&fmt);
			}
			else if (*fmt == '*') {
				++fmt;
				precision = va_arg(args, int);
			}
			if (precision < 0) {
				precision = 0;
			}
		}

		// Get the conversion qualifier
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			fmt++;
#ifdef	SUPPORT_DA16X_LONGLONG
			if (*fmt == 'l' ){
				qualifier = 'L';
				fmt++;
			}
#endif	//SUPPORT_DA16X_LONGLONG
		}

		// Default base
		base = 10;

		switch (*fmt)
		{
			case 'c':
			if (!(flags & LEFT)) {
				while (--field_width > 0) *str++ = ' ';
			}
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0) *str++ = ' ';
			continue;

			case 'S': /* upper, case insensitive */
			flags = flags | LARGE;
			s = va_arg(args, char *);

			len = ((int)(endstr - str) - field_width);
			if (len < 0 ){ 	/* overflow check */
				len = 0;
			}

			str = da16x_string(str, len, s, field_width, precision, flags, linefeed);
			continue;

			case 's': /* case sensitive */
			s = va_arg(args, char *);

			len = ((int)(endstr - str) - field_width);
			if (len < 0 ){ 	/* overflow check */
				len = 0;
			}

			str = da16x_string(str, len, s, field_width, precision, flags, linefeed);
			continue;

			case 'p':
			if (field_width == -1)	{
			  field_width = 2 * sizeof(void *);
			  flags |= ZEROPAD;
			}
			str = number(str, (unsigned long) va_arg(args, void *)
				, 16, field_width, precision, flags);
			continue;

			case 'n':
			if (qualifier == 'l') {
				long *ip = va_arg(args, long *);
				*ip = (str - buf);
			} else 	{
				int *ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

			case 'A':
			flags |= LARGE;
			if (qualifier == 'l') {
				str = eaddr(str, va_arg(args, unsigned char *)
					, field_width, precision, flags);
			} else {
				str = iaddr(str, va_arg(args, unsigned char *)
					, field_width, precision, flags);
			}
			continue;

			case 'a':
			if (qualifier == 'l') {
				str = eaddr(str, va_arg(args, unsigned char *)
					, field_width, precision, flags);
			} else {
				str = iaddr(str, va_arg(args, unsigned char *)
					, field_width, precision, flags);
			}
			continue;

			// Integer number formats - set up the flags and "break"
			case 'o':
			base = 8;
			break;

			case 'X':
			flags |= LARGE;
			base = 16;
			break;

			case 'x':
			base = 16;
			break;

			case 'd':
			case 'i':
			flags |= SIGN;
			break;

			case 'u':
			break;

#ifndef NOFLOAT

			case 'E':
			case 'G':
			case 'e':
			case 'f':
			case 'g':
			str = flt(str, va_arg(args, double)
				, field_width, precision, *fmt, flags | SIGN);
			continue;

#endif

			default:
			if (*fmt != '%') {
				*str++ = '%';
			}
			if (*fmt) {
				*str++ = *fmt;
			} else {
				--fmt;
			}
			continue;
		}

#ifdef	SUPPORT_DA16X_LONGLONG
		if (qualifier == 'L') {
			num = va_arg(args, unsigned long long);
		}else
#endif	//SUPPORT_DA16X_LONGLONG
		if (qualifier == 'l') {
			num = va_arg(args, unsigned long);
		}
		else if (qualifier == 'h') {
			if (flags & SIGN) {
				num = va_arg(args, int);
			} else {
				num = va_arg(args, unsigned int);
			}
		}
		else if (flags & SIGN) {
			num = va_arg(args, int);
		} else {
			num = va_arg(args, unsigned int);
		}

		str = number(str, num, base, field_width, precision, flags);

	}

	*str = '\0';
	return ( str - buf );
}

/******************************************************************************
 *  da16x_sprintf
 *
 *  Purpose :   sprintf
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/

int da16x_sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = da16x_vsprintf(buf, fmt, args);
	va_end(args);

	return n;
}

/******************************************************************************
 *  da16x_vprintf
 *
 *  Purpose :   vprintf
 *  Input   :   fmt, C string that contains a format string
 *              args, values identifying a variable arguments list initialized
 *                    with va_start.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/


int da16x_vprintf(const char *format, va_list arg)
{
	int len;

	if (console_hidden_flag == FALSE ){
		return 0; // Skip
	}

	len = da16x_vsnprintf(uart_print_buf,CON_BUF_SIZ, 0, format, arg);
#ifndef USE_PL011
	_sys_uart_write( uart_print_buf, len );
#else
	{
		int i;
		for(i = 0; i < len; i++)
		{
			putchar(uart_print_buf[i]);
		}
	}
#endif
	return len;
}

/******************************************************************************
 *  da16x_asprintf
 *
 *  Purpose :  asprintf
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/

int da16x_asprintf (char **buf, const char *fmt, ...) {
  va_list args;
  int len = 0;

  va_start(args, fmt);
  len = da16x_vasprintf(buf, fmt, args);
  va_end(args);

  return len;
}

/******************************************************************************
 *  da16x_vasprintf
 *
 *  Purpose :   vasprintf
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/

int da16x_vasprintf (char **buf, const char *fmt, va_list args) {
	int len = 0;

	len = da16x_vsnprintf(uart_print_buf,CON_BUF_SIZ, 0, fmt, args);

	if (len < 0) {
		return -1;
	}

	*buf = pvPortMalloc(len + 1);

	if (NULL==*buf) {
		return -1;
	}

	len = da16x_vsprintf(*buf, fmt, args);

	return len;
}

/******************************************************************************
 *  da16x_snprintf_lfeed
 *
 *  Purpose :   sprintf
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :   buf, pointer to a buffer where the resulting C string is stored.
 *  Return  :   string length
 ******************************************************************************/

int da16x_snprintf_lfeed(char *buf, size_t n, int linefeed, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	n = da16x_vsnprintf(buf, n, linefeed, fmt, args);
	va_end(args);

	return n;
}

/******************************************************************************
 *  console_hidden_mode / console_hidden_check
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	console_hidden_mode(int mode)
{
	console_hidden_flag = mode ;
}

int	console_hidden_check(void)
{
	return console_hidden_flag;
}

/******************************************************************************
 *  da16x_printf
 *
 *  Purpose :   writes the C string pointed by format to "stdout" (console)
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :
 *  Return  :   string length
 ******************************************************************************/

int da16x_printf(const char *fmt,...)
{
	int 	len;
	va_list ap;

	if (console_hidden_flag == FALSE ){
		return 0; // Skip
	}

	va_start(ap,fmt);

	len = da16x_vsnprintf(uart_print_buf,CON_BUF_SIZ, 0, fmt, ap);
#ifndef USE_PL011
	_sys_uart_write( uart_print_buf, len );
#else
	UART_WRITE(con_uart0, uart_print_buf, len );
#endif

	va_end(ap);

	return len;
}
/******************************************************************************
 *  Printf
 *
 *  Purpose :   special printf function for ISR routines
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :
 *  Return  :
 ******************************************************************************/

void Printf(const char *fmt,...)	/* lowlevel printf: UART_lowlevel_Printf */
{
	int	len;
	va_list ap;

	if (console_hidden_flag == FALSE ){
		return ; // Skip
	}

	va_start(ap,fmt);

	len = da16x_vsnprintf(uart_lowprint_buf,CON_LBUF_SIZ, 1, fmt, ap);
#ifndef USE_PL011
	_sys_uart_write(uart_lowprint_buf, len);
#else
	UART_DEBUG_WRITE(con_uart0,uart_lowprint_buf,len);
#endif

	va_end(ap);
}

/******************************************************************************
 *  DBGT_Printf
 *
 *  Purpose :   special printf function for DEBUGGING
 *  Input   :   fmt, C string that contains a format string
 *              ..., additional arguments.
 *  Output  :
 *  Return  :
 ******************************************************************************/

void DBGT_VPrintf(const char *fmt, va_list args)
{
	int len;

	if (console_hidden_flag == FALSE ){
		// VSIM Protection
		// some drivers seem to print out before initializing CONSOLE & DBGT.
		return ; // Skip
	}

	len = da16x_vsnprintf(uart_lowprint_buf,CON_LBUF_SIZ, 0, fmt, args);
	da16x_sysdbg_dbgt(uart_lowprint_buf, len);
}

void DBGT_Printf(const char *fmt,...)
{
	int len;
	va_list ap;

	if (console_hidden_flag == FALSE ){
		// VSIM Protection
		// some drivers seem to print out before initializing CONSOLE & DBGT.
		return ; // Skip
	}

	va_start(ap,fmt);

	len = da16x_vsnprintf(uart_lowprint_buf,CON_LBUF_SIZ, 0, fmt, ap);
	da16x_sysdbg_dbgt(uart_lowprint_buf, len);

	va_end(ap);
}

/******************************************************************************
 *  putchar
 *
 *  Purpose :   writes a character to "stdout" (console)
 *  Input   :   ch, the int promotion of the character to be written
 *  Output  :
 *  Return  :   On success, the character written is returned.
 ******************************************************************************/

int putchar(int ch)
{
#ifndef USE_PL011
	char text[2];

	if (ch == '\n')
	{
		text[0] = '\r';
		text[1] = (char)ch;
		_sys_uart_write( text, sizeof(char)*2);
	}else{
		text[0] = (char)ch;
		_sys_uart_write( text, sizeof(char));
	}

	return (ch);
#else
	char text[2];

	if (ch == '\n')
	{
		text[0] = '\r';
		text[1] = (char)ch;
		UART_WRITE(con_uart0, text, sizeof(char)*2);
	}else{
		text[0] = (char)ch;
		UART_WRITE(con_uart0, text, sizeof(char));
	}

	return (ch);
#endif
}

/******************************************************************************
 *  putrawchar
 *
 *  Purpose :   writes a character to "stdout" (console)
 *  Input   :   ch, the int promotion of the character to be written
 *  Output  :
 *  Return  :   On success, the character written is returned.
 ******************************************************************************/

int putrawchar(int ch)
{
#ifndef USE_PL011
	char text[1];

	text[0] = (char)ch;
	_sys_uart_write( text, sizeof(char));

	return (ch);
#else
	char text[1];

	text[0] = (char)ch;
	UART_DEBUG_WRITE(con_uart0, text, sizeof(char));

	return (ch);
#endif
}

/******************************************************************************
 *  da16x_putstring
 *
  *  Purpose :  puts
 ******************************************************************************/

int	da16x_putstring(char *s, int len)
{
#ifndef USE_PL011
	_sys_uart_write( s, len);
	return len;
#else
	if (con_uart0 != NULL)
	{
		return UART_WRITE(con_uart0, s, len);
	}
	return 0;
#endif
}

/******************************************************************************
 *  getchar
 *
 *  Purpose :   returns the next character from "stdin" (console)
 *  Input   :
 *  Output  :
 *  Return  :   On success, the character read is returned.
 ******************************************************************************/

int getchar(void)
{
#ifndef USE_PL011
	char text;
	_sys_uart_read( &text, sizeof(char));
	if (text == '\r'){
		putchar('\r');
		text = '\n';
	}
	putchar(text);
	return (int)text;
#else
	char text;
	UART_READ(con_uart0, &text, sizeof(char));
#if 0
	if (text == '\r'){
		putchar('\r');
		text = '\n';
	}
	//putchar(text);
#endif
	return (int)text;
#endif
}

/******************************************************************************
 *  getrawchar
 *
 *  Purpose :   returns the next character from "stdin" (console)
 *  Input   :
 *  Output  :
 *  Return  :   On success, the character read is returned.
 ******************************************************************************/

int getrawchar(int mode)
{
	DA16X_UNUSED_ARG(mode);

#ifndef USE_PL011
	char text;
	_sys_uart_read( &text, sizeof(char));
	return (int)text;
#else
#define HIGH_YMODEM
#ifdef HIGH_YMODEM										// baudrate 921600??쏙????쏙??占??옙??싹깍옙 ??쏙??占??쇽??占??옙 suspend ??심??옙 ??쏙??占??옙 ??쏙??占??옙 ??占??옙??쌔??옙??쏙??
	char text;

	UART_READ(con_uart0, &text, sizeof(char));
#else
	char text;
	UINT32	flag = FALSE;

	UART_IOCTL(con_uart0, UART_GET_RX_SUSPEND, &flag); // backup
	UART_IOCTL(con_uart0, UART_SET_RX_SUSPEND, &mode);
	UART_READ(con_uart0, &text, sizeof(char));
	UART_IOCTL(con_uart0, UART_SET_RX_SUSPEND, &flag); // restore
#endif
	return (int)text;
#endif
}

/******************************************************************************
 *  getstr
 *
 *  Purpose :   returns the next character from "stdin" (console)
 *  Input   :
 *  Output  :
 *  Return  :   On success, the character read is returned.
 ******************************************************************************/

int getrawstr(char *buf, int length)
{
#ifndef USE_PL011
	return (int)0;
#else
	return UART_READ(con_uart0, buf, length);
#endif
}


/******************************************************************************
 *  da16x_console_handler
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

HANDLE da16x_console_handler(void)
{
#ifdef USE_PL011
	return con_uart0;
#else
	return NULL;
#endif
}

/******************************************************************************
 *  da16x_console_txempty
 *
 *  Purpose :   empty check
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_console_txempty(void)
{
#ifndef USE_PL011
	return _sys_uart_txempty();
#else
	if (con_uart0 != NULL && console_hidden_flag != FALSE ){
		UINT32 status = FALSE;
		UART_IOCTL(con_uart0, UART_CHECK_TXEMPTY, &status);
		return ((status == TRUE) ? TRUE  : FALSE);
	}
	return TRUE; // Skip
#endif
}

/******************************************************************************
 *  da16x_console_rxempty
 *
 *  Purpose :   empty check
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

int da16x_console_rxempty(void)
{
#ifndef USE_PL011
	return _sys_uart_rxempty();
#else
	if (con_uart0 != NULL ){
		UINT32 status = FALSE;
		UART_IOCTL(con_uart0, UART_CHECK_RXEMPTY, &status);
		//return ((status == TRUE) ? TRUE  : FALSE);
		return status;
	}
	return 0; // Skip
#endif
}

// for SUPPORT_EMERGENCY_BOOT
int uart_read_timeout(VOID *p_data, UINT32 p_dlen, UINT32 timeout, UINT32 *recv_len)
{
	UINT8 *buf = (UINT8*)p_data;
	UINT32 status, repeat_count;
	*recv_len = 0;

	DA16X_UNUSED_ARG(recv_len);
	DA16X_UNUSED_ARG(p_dlen);

	repeat_count = (timeout*10)/5;
	if (con_uart0 == NULL) {
		return FALSE;
	}

	while(1) {
		UART_IOCTL(con_uart0, UART_CHECK_RXEMPTY, &status);

		if (!status) {
			UART_READ(con_uart0, buf, 1);
			(*recv_len)++; p_dlen--; buf++;
			if (p_dlen == 0) {
				return TRUE;
			}
		} else {
			udelay(500);
			repeat_count --;
			if (repeat_count == 0) {
				return FALSE;
			}
		}
	}
}

#if defined	( __CONSOLE_CONTROL__ )
/**
 ****************************************************************************************
 * @brief       Console control (On/Off)
 * @param[in] _console_ctrl - Console control flag
 * @return      None
 ****************************************************************************************
 */
void console_control(int _console_ctrl)
{
	if (_console_ctrl == 1) {
		if (!console_on()) {
			PRINTF(">>> Console on ... \n");
		} else {
			PRINTF(RED_COLOR "[%s] Failed Console on \n" CLEAR_COLOR, __func__);
		}
	} else if (_console_ctrl == 0) {
		if (!console_off()) {
			PRINTF(">>> Console off ...\n");
		} else {
			PRINTF(RED_COLOR "[%s] Failed Console off \n" CLEAR_COLOR, __func__);
		}
	}
}

/**
 ****************************************************************************************
 * @brief       Set UART register to make enable console
 * @param[in]   None
 * @return      0, Success
 ****************************************************************************************
 */
int console_on(void)
{
	UART_REG_MAP *dbgt = (UART_REG_MAP *) 0x40012000;

	// Line Break
	dbgt->LineCon_H = dbgt->LineCon_H & 0xfffffffe;
	OAL_MSLEEP(10);		// queue flush

	return 0;
}

/**
 ****************************************************************************************
 * @brief       Set UART register to make disable console
 * @param[in]   None
 * @return      0, Success
 ****************************************************************************************
 */
int console_off(void)
{
	UART_REG_MAP *dbgt = (UART_REG_MAP *) 0x40012000;

	OAL_MSLEEP(10);		// queue flush

	// Line Break
	dbgt->LineCon_H = dbgt->LineCon_H | 0x1;

	return 0;
}

/**
 ****************************************************************************************
 * @brief       Set flag to make enable console
 * @param[in]   None
 * @return      None
 ****************************************************************************************
 */
void console_enable(void)
{
	RTM_FLAG_PTR->console_ctrl_rtm = 1;

	if (dpm_mode_is_wakeup() != TRUE) { // dpm (x), dpm (o) && wakeup (x)
		write_nvram_int(NVR_KEY_DLC_CONSOLE_CTRL, 1);
	}

}

/**
 ****************************************************************************************
 * @brief       Set flag to make disable console
 * @param[in]   None
 * @return      None
 ****************************************************************************************
 */
void console_disable(void)
{
	RTM_FLAG_PTR->console_ctrl_rtm = 0;

	if (dpm_mode_is_wakeup() != TRUE) { // dpm (x), dpm (o) && wakeup (x)
		write_nvram_int(NVR_KEY_DLC_CONSOLE_CTRL, 0);
	}
}

#endif /* __CONSOLE_CONTROL__ */

/* EOF */
