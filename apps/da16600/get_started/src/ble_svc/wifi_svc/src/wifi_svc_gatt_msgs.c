/**
 ****************************************************************************************
 *
 * @file wifi_svc_gatt_mgs.c
 *
 * @brief GATTM / GATTC outgoing messages.
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


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "sdk_type.h"

#if defined (__BLE_PERI_WIFI_SVC__)

#include "../../include/da14585_config.h"
#include "rwip_config.h"
#include "../../include/gatt_msgs.h"
#include "da16x_system.h"
#include "da16_gtl_features.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

/* attr set value : for non-RI attributes only (which reside in ble) */
uint8_t gattm_att_set_value_req_send(uint16_t handle, uint8_t *data, uint16_t length)
{
	struct gattm_att_set_value_req *set_value_req = BleMsgAlloc(GATTM_ATT_SET_VALUE_REQ,
			TASK_ID_GATTM, TASK_ID_GTL, sizeof (struct gattm_att_set_value_req) + length);

	set_value_req->handle = handle;
	set_value_req->length = length;
	memcpy(set_value_req->value, data, length);

	BleSendMsg(set_value_req);

	return 0;
}

/**
 ****************************************************************************************
 * @brief Sends a exchange MTU command
 * @param none
 * @return void
 ****************************************************************************************
 */
void user_gattc_exc_mtu_cmd(void)
{
	struct gattc_exc_mtu_cmd *exc_mtu_cmd =  BleMsgAlloc(GATTC_EXC_MTU_CMD, TASK_ID_GATTC,
			TASK_ID_GTL, sizeof(struct gattc_exc_mtu_cmd));

	exc_mtu_cmd->operation = GATTC_MTU_EXCH;
	exc_mtu_cmd->seq_num = 0x1234;

	BleSendMsg(exc_mtu_cmd);
	DBG_INFO("    GATTC_EXC_MTU_CMD (GATTC_MTU_EXCH) sent ! \n");
}

#endif /* __BLE_PERI_WIFI_SVC__ */

/* EOF */
