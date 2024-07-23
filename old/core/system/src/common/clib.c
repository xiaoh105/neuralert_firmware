/**
 ****************************************************************************************
 *
 * @file clib.c
 *
 * @brief C Library
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clib.h"

#define KSTRTOX_OVERFLOW        (1U << 31)
#define likely(x)       	((x) == 1)
#define unlikely(x)     	((x) == 0)
#define ULLONG_MAX		18446744073709551615ULL

int	isdigit (int c)
{
	return((c>='0') && (c<='9'));
}

int	isxdigit(int c)
{
	return (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

unsigned char toint(char c)
{
	unsigned char rslt;
	
	if ( (c >= '0') && (c <= '9') ){ 
		rslt = (unsigned char)(c-'0');
	}else if ( (c >= 'a') && (c <= 'f') ){ 
		rslt = (unsigned char)(c-'a'+10);
	}else if ( (c >= 'A') && (c <= 'F') ){ 
		rslt = (unsigned char)(c-'A'+10);
	}else{ 
		rslt =  (unsigned char)0; 
	}

	return rslt;
}

unsigned int htoi(char *s)
{
	unsigned int sum = 0;

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
		s = s + 2;
	}

	while ((*s >= '0') && (*s <= 'f')) {
		sum = sum * 16 + toint(*s++);
	}
	return(sum);
}

int btoi(char *s)
{
	short sum = 0;

	while ((*s >= '0') && (*s <= '1')){
		sum = (short)(sum * 2 + toint(*s++));
	}
	return(sum);
}

unsigned int ctoi(char *s)
{
	unsigned int sum = 0;
	while((*s >= '0') && (*s <= '9')){
		sum = sum * 10 + toint(*s++);
	}
	return sum;

}

int stoi(char *s)
{
	int sum = 0, sign;

	if( *s == '-'){
		sign = -1;
		s++;
	}else{
		sign = 1;
	}
	
	while(*s){
		sum = sum * 10 + toint(*s++);
	}
	sum = sign * sum ;
	return sum;
}

unsigned int hex2dec(unsigned int data)
{
    char hex_str[9];
    
    memset(hex_str, 0x00, 9);
    sprintf(hex_str, "%X", data);
    return strtoul(hex_str, NULL, 16);
}

#if defined (__ENABLE_UNUSED__)
static char _tolower (char c)
{
  if ((c >= 'A') && (c <= 'Z'))
    {
      c = (c - 'A') + 'a';
    }
  return c;
}
#endif // __ENABLE_UNUSED__

/* EOF */
