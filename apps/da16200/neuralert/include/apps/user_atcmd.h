/**
 ****************************************************************************************
 *
 * @file user_atcmd.h
 *
 * @brief Header file for User AT-CMD
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

#ifndef __USER_ATCMD_H__
#define __USER_ATCMD_H__

#include "sdk_type.h"


#if defined ( __SUPPORT_ATCMD__ )

#include "da16x_system.h"
#include "da16x_types.h"
#include "target.h"

#include "command.h"
#include "atcmd.h"
#include "common_uart.h"

#include "user_nvram_cmd_table.h"
#include "user_dpm.h"
#include "user_dpm_api.h"


#define ATCMD_JSON_MAX_LENGTH       256
#define ATCMD_JSON_IDENTITY         ",{\""


/*---------------------------------------------------------------------------*/

/* External functions */
extern int  parsingJsonDataGetLedStatus(char *buf);
extern int  parsingJsonDataGetDoorStatus(char *buf);
extern void PRINTF_ATCMD(const char *fmt,...);
extern void makeMqttClientId(char *deviceId, char *mqttClientId);
extern void mqtt_auto_start(void* arg);

/* For User AT-CMD help */
extern atcmd_error_code (* user_atcmd_help_cb_fn)(char *cmd);


/* Internal functions */


/**
 ****************************************************************************************
 * @brief            Parser for User AT-CMD
 * @param[in] line   Input string
 * @return           Result code
 *                   ERR_CMD_OK              =  0,
 *                   ERR_UNKNOWN_CMD         = -1,
 *                   ERR_INSUFFICENT_ARGS    = -2,
 *                   ERR_TOO_MANY_ARGS       = -3,
 *                   ERR_WRONG_ARGUMENTS     = -4,
 *                   ERR_NOT_SUPPORTED       = -5,
 *                   ERR_NOT_CONNECTED       = -6,
 *                   ERR_NO_RESULT           = -7,
 *                   ERR_TOO_LONG_RESULT     = -8,
 *                   ERR_INSUFFICENT_CONFIG  = -9,
 *                   ERR_TIMEOUT             = -10,
 *                   ERR_NVR_WRITE           = -11,
 *                   ERR_RTM_WRITE           = -12,
 *                   ERR_UNKNOWN             = -99
 ****************************************************************************************
 */
atcmd_error_code user_atcmd_parser(char *line);

/**
 ****************************************************************************************
 * @brief   Register display call-back function for User AT-CMD help
 * @return  None
 ****************************************************************************************
 */
void reg_user_atcmd_help_cb(void);


#endif // __SUPPORT_ATCMD__

#endif // __USER_ATCMD_H__

/* EOF */
