/**
 ****************************************************************************************
 *
 * @file atcmd_ble.c
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

#include "atcmd_ble.h"
#include "util_api.h"

#if defined(__SUPPORT_ATCMD__) && defined(__BLE_COMBO_REF__)
__attribute__((weak)) int atcmd_ble_set_device_name_handler(uint8_t *name)
{
	DA16X_UNUSED_ARG(name);

	return AT_CMD_ERR_NOT_SUPPORTED;
}

int atcmd_ble(int argc, char *argv[])
{
	int ret = AT_CMD_ERR_CMD_OK;

	// AT+BLENAME
	if (strcasecmp(argv[0] + 6, "NAME") == 0) {
		/* AT+BLENAME=<device_name> */
		if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
			if (app_get_ble_dev_name())
				PRINTF_ATCMD("\r\n+BLENAME:%s\r\n", app_get_ble_dev_name());
			else
				return AT_CMD_ERR_NO_RESULT;
		} else if (argc == 2) {
			ret = atcmd_ble_set_device_name_handler((uint8_t *)argv[1]);
		} else if (argc < 2) {
			return AT_CMD_ERR_INSUFFICENT_ARGS;
		} else if (argc > 2) {
			return AT_CMD_ERR_TOO_MANY_ARGS;
		}
	} else if (strcasecmp(argv[0] + 3, "ADVSTOP") == 0) {
		ret = atcmd_bleperi_cmd_handler(ATCMD_BLE_ADV_STOP);
	} else if (strcasecmp(argv[0] + 3, "ADVSTART") == 0) {
		ret = atcmd_bleperi_cmd_handler(ATCMD_BLE_ADV_START);
	}

	return ret;
}
#endif // __SUPPORT_ATCMD__

/* EOF */
