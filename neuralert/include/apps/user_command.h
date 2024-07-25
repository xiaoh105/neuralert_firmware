/**
 ****************************************************************************************
 *
 * @file user_command.h
 *
 * @brief Command Parser for user functions
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
 
#ifndef __USER_COMMAND_H__
#define __USER_COMMAND_H__

#include "command.h"

//-----------------------------------------------------------------------
// Command MEM-List
//-----------------------------------------------------------------------

extern const COMMAND_TREE_TYPE    cmd_user_list[];

//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------

extern void cmd_test(int argc, char *argv[]);
#if defined(__COAP_CLIENT_SAMPLE__)
extern void cmd_coap_client(int argc, char *argv[]);
#endif /* __COAP_CLIENT_SAMPLE__ */
extern void cmd_txpwr(int argc, char *argv[]);

#endif /* __USER_COMMAND_H__ */

/* EOF */
