/**
 ****************************************************************************************
 *
 * @file atcmd_ble.h
 *
 * @brief AT-CMD BLE Controller
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

#ifndef CORE_SYSTEM_INCLUDE_AT_CMD_ATCMD_BLE_H_
#define CORE_SYSTEM_INCLUDE_AT_CMD_ATCMD_BLE_H_

#include "atcmd.h"

#if defined (__SUPPORT_ATCMD__) && defined (__BLE_COMBO_REF__)

enum {
	ATCMD_BLE_ADV_STOP,
	ATCMD_BLE_ADV_START,
	ATCMD_BLEPERI_CMD_LAST,
};

__attribute__((weak)) int atcmd_ble_set_device_name_handler(uint8_t *name);

int atcmd_bleperi_cmd_handler(uint8_t cmd);
int atcmd_ble(int argc, char *argv[]);

extern uint8_t *app_get_ble_dev_name(void);

#endif // __SUPPORT_ATCMD__

#endif /* CORE_SYSTEM_INCLUDE_AT_CMD_ATCMD_BLE_H_ */

/* EOF */
