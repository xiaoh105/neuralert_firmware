/**
 ****************************************************************************************
 *
 * @file syscommand.h
 *
 * @brief Definition for system command
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

#ifndef __sys_command_h__
#define __sys_command_h__



#define STATE_WHITESPACE (0)
#define STATE_WORD (1)

#define EINVAL      1   /* invalid argument */
#define EMAGIC      3   /* magic value failed */
#define ECOMMAND    4   /* invalid command */
#define EAMBIGCMD   11  /* ambiguous command */

#define CR	0x0D
#define LF	0x0A
#define BS	0x08
#define SP	0x20

#define EOS			'\0'

#define INPUT_TICK	10


void printTaskInfo(void);
void *getTaskInfo(char *task);
void semInfo(void);
void hisrInfo(void);
void qInfo(void);
void eventInfo(void);
void memoryPoolInfo(void);
void osHistory(int argc, char *argv[]);

#endif	/*__sys_command_h__*/

/* EOF */
