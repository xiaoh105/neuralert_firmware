/**
 ****************************************************************************************
 *
 * @file sensor_gw_wifi_svc_gatt_handlers.c
 *
 * @brief GATTM / GATTC handlers.
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



#include "sdk_type.h"

#if defined (__BLE_CENT_SENSOR_GW__)

#include "../inc/da14585_config.h"
#include "rwip_config.h"

#include "../inc/app.h"
#include "../../include/gatt_handlers.h"
#include "../../include/user_config.h"

#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif

#include "application.h"

/*
	below code is needed because UNKNOWN_TYPE is defined as enum in DA16200, 
	but in ble sdk, it is defined as non-enum
*/
#undef UNKNOWN_TYPE
#include "da16x_system.h"
#define UNKNOWN_TYPE 3

#include "common_def.h"
#include "common_utils.h"
#include "common_config.h"

#include "user_dpm_api.h"

#include "da16_gtl_features.h"
#include "json.h"

#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

#if defined (__WFSVC_ENABLE__)
extern user_custom_profile_wfsvc_env_t user_custom_profile_wfsvc_env;

extern local_wfsvc_values_t wfsvc_local_wfact_res_values;
extern local_wfsvc_values_t wfsvc_local_apscan_res_values;
#if defined (__SUPPORT_BLE_PROVISIONING__)
extern local_wfsvc_values_t wfsvc_local_provisioning_res_values;
#endif // __SUPPORT_BLE_PROVISIONING__

uint16_t my_custom_service_wfsvc_start_handle = 0xFFFF;
uint16_t my_custom_service_start_handle = 0xFFFF;

char *inv_str = "Invalid for WFSVC";

#endif /* __WFSVC_ENABLE__ */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#if defined (__WFSVC_ENABLE__)
extern char *get_wifi_sta_scan_result_str(void);
extern uint32_t get_wifi_sta_scan_result_str_len(void);
extern void set_wifi_sta_scan_result_str_idx(uint32_t idx);
extern uint32_t get_wifi_sta_scan_result_str_idx(void);
#if defined (__SUPPORT_BLE_PROVISIONING__)
extern uint32_t get_wifi_provisioning_command_result(void);
extern uint32_t get_wifi_provisioning_result_len(void);
extern char *get_wifi_provisioning_result_str(void);
extern int32_t get_wifi_provisioning_dpm_mode(void);
#endif // __SUPPORT_BLE_PROVISIONING__
#endif

// GATTM_ADD_SVC_RSP
uint8_t gattm_add_svc_rsp_hnd(ke_msg_id_t const msgid,
							  struct gattm_add_svc_rsp const *param,
							  ke_task_id_t const dest_id,
							  ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->status == ATT_ERR_NO_ERROR)
	{
#if defined (__WFSVC_ENABLE__)
		//service wfsvc
		my_custom_service_wfsvc_start_handle = param->start_hdl;

		user_custom_profile_wfsvc_env.start_handle = param->start_hdl + 1;
		user_custom_profile_wfsvc_env.end_handle = param->start_hdl + CUSTOM_DB_WFSVC_B_IDX_NB + 1;
#endif

		app_adv_start();
	}

	return KE_MSG_CONSUMED;
}

/**
	@brief
	dstStr should be freed after use

*/
uint16_t copy_str_buf(char **dstStr, char *srcStr)
{
	uint16_t str_buf_size = strlen(srcStr) + 1;
	char *dst_str = NULL;

	dst_str = (char *)MEMALLOC(str_buf_size);

	if (dst_str == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return 0;
	}

	memset(dst_str, 0x00, str_buf_size);
	strcpy(dst_str, srcStr);

	*dstStr = dst_str;
	return str_buf_size;
}

#if defined (__SUPPORT_BLE_PROVISIONING__)
extern void combo_ble_sw_reset(void);
extern void reboot_func(UINT flag);
#endif //__SUPPORT_BLE_PROVISIONING__
uint8_t gattc_read_req_ind_hnd (ke_msg_id_t const msgid,
								struct gattc_read_req_ind const *param,
								ke_task_id_t const dest_id,
								ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);

#if defined (__WFSVC_ENABLE__)

	uint8_t cfm_msg_status = GAP_ERR_NO_ERROR;
	uint16_t cfm_msg_val_length = 0;
	char *value_to_set = NULL;
	char *scan_result = NULL;
#if defined (__SUPPORT_BLE_PROVISIONING__)
    uint8_t reboot_flag = 0;
#endif //__SUPPORT_BLE_PROVISIONING__

	if (my_custom_service_wfsvc_start_handle == 0xFFFF)
	{
		return 1;
	}

	uint16_t handle_index = param->handle - my_custom_service_wfsvc_start_handle - 1;

	DBG_INFO("[%s] GATTC_READ_REQ_IND, char_enum_idx = %d \n", __func__, handle_index);


	if (value_to_set != NULL)
	{
		cfm_msg_status = GAP_ERR_NO_ERROR;
	}

	switch (handle_index)
	{
#ifdef ENABLE_CHAR_USER_DESC_ARE_RI
		// char(per: RD) > user descr
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_USER_DESC_IDX:
			cfm_msg_val_length = copy_str_buf(&value_to_set, wfsvc_wfact_res_desc);
			break;
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_USER_DESC_IDX:
			cfm_msg_val_length = copy_str_buf(&value_to_set, wfsvc_apscan_res_desc);
			break;
#endif

		// client config descr
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX:
		{
			value_to_set = (char *)&wfsvc_local_wfact_res_values.conf;
			cfm_msg_val_length = sizeof(uint16_t);
		} break;
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX:
		{
			value_to_set = (char *)&wfsvc_local_apscan_res_values.conf;
			cfm_msg_val_length = sizeof(uint16_t);
		} break;
#if defined ( __SUPPORT_BLE_PROVISIONING__)	
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX:
		{
			value_to_set = (char *)&wfsvc_local_provisioning_res_values.conf;
			cfm_msg_val_length = sizeof(uint16_t);
		} break;
#endif // __SUPPORT_BLE_PROVISIONING__			
		// value (read characteristic)
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_VALUE_IDX:
		{
			value_to_set = (char *)(wfsvc_local_wfact_res_values.value);
			cfm_msg_val_length = DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD_RES;
		} break;
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_VALUE_IDX:
		{
			DBG_INFO("read request on WFSVC_APSCAN_RES from a peer\n");

			char *scan_result_ptr = NULL;
			char *ptr = NULL;
			uint16_t total_len;
			uint16_t remain_len;
			uint16_t idx;

			total_len = get_wifi_sta_scan_result_str_len();
			idx = get_wifi_sta_scan_result_str_idx();
			DBG_INFO("total_scan_result_len = %d, offset = %d \n", total_len, idx);

			if (!total_len)
			{
				DBG_ERR("Please let Wi-Fi Scan cmd be issued first by a peer \n");
				goto read_err;
			}

			scan_result = (char *)MEMALLOC(DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES);

			if (scan_result == NULL)
			{
				DBG_ERR("malloc failed.\n");
				value_to_set = inv_str;
				cfm_msg_val_length = strlen(inv_str);
				cfm_msg_status = ATT_ERR_APP_ERROR;
				break;
			}

			memset(scan_result, 0x00, DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES);
			ptr = scan_result;

			if ((total_len - idx) > (uint16_t)(DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES - DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE))
			{
				cfm_msg_val_length = DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES;
			}
			else
			{
				cfm_msg_val_length = (total_len - idx) + DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE;
			}

			DBG_INFO("cfm_msg_val_length = %d \n", cfm_msg_val_length);

			remain_len = (total_len - idx) - (cfm_msg_val_length - DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE);
			DBG_INFO("remain_len = %d \n", remain_len);
			memcpy(ptr, &remain_len, sizeof(uint16_t));

			ptr += sizeof(uint16_t);
			memcpy(ptr, &total_len, sizeof(uint16_t));

			ptr += sizeof(uint16_t);
			scan_result_ptr = get_wifi_sta_scan_result_str();
			memcpy(ptr, &scan_result_ptr[idx],
			(cfm_msg_val_length - DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE));

			value_to_set = scan_result;

			idx += (cfm_msg_val_length - DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE);

			if (idx >= total_len)
			{
				idx = 0;
			}

			set_wifi_sta_scan_result_str_idx(idx);

#ifdef DBG_PRINT_DUMP
			DBG_DUMP("[%s:%d] cfm_msg_val_length = %d, idx = %d, remain_len = %d, total_len = %d\n",
				   __func__, __LINE__, cfm_msg_val_length, idx, remain_len, total_len);
			hex_dump(value_to_set, cfm_msg_val_length);
#endif /* DEBUG */
		} break;
		
#if defined ( __SUPPORT_BLE_PROVISIONING__)	
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_VALUE_IDX:
		{
			uint32_t cmd = 0;
			uint32_t len = 0;

			DBG_INFO("read request on WFSVC_PROVISIONING_RES from a peer\n");

			cmd = get_wifi_provisioning_command_result();
			len = get_wifi_provisioning_result_len();

			DBG_INFO("current cmd=%d, result len=%d\n", cmd, len);
			if (cmd >= COMBO_WIFI_CMD_SELECT_AP_SUCCESS &&
				cmd <= COMBO_WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL &&
				len > strlen("result"))
			{
				value_to_set = get_wifi_provisioning_result_str();
				cfm_msg_val_length = len;
				DBG_INFO("result str=>\n");
				DBG_INFO("===========================\n");
				DBG_INFO("%s\n", value_to_set);
				DBG_INFO("===========================\n");
				if (cmd == COMBO_WIFI_PROV_REBOOT_ACK)
				{
					reboot_flag = 1;
				}
				else
				{
					reboot_flag = 0;
				}

#if defined (__PROVISION_ATCMD__)
			atcmd_provstat(cmd);
#endif
			}
			else
			{
				DBG_ERR("invalid state or result length \n");
				goto read_err;
			}
				
		} break;
#endif // __SUPPORT_BLE_PROVISIONING__	

		// wrong case
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_VALUE_IDX:
		default:
		{
	read_err:
			value_to_set = inv_str;
			cfm_msg_val_length = strlen(inv_str);
			cfm_msg_status = ATT_ERR_APP_ERROR;

		} break;
	}

	struct gattc_read_cfm *cfm = BleMsgAlloc(GATTC_READ_CFM,
								 src_id,
								 dest_id,
								 sizeof (struct gattc_read_cfm) + (cfm_msg_val_length));

	// Set parameters
	cfm->handle = param->handle;
	memcpy(cfm->value, value_to_set, cfm_msg_val_length);
	cfm->length = cfm_msg_val_length;
	cfm->status = cfm_msg_status;

	BleSendMsg((void *) cfm);

	if (scan_result != NULL)
	{
		MEMFREE(scan_result);
	}


#if defined ( __SUPPORT_BLE_PROVISIONING__)
	if (reboot_flag == 1)
	{

		if (get_wifi_provisioning_dpm_mode())
		{
#if !defined (__BLE_PROVISIONING_SAMPLE__)
			dpm_mode_enable();
#endif
		}

		DBG_INFO("now reboot to complete provisioning sequence...\n");

		vTaskDelay(30);

		// reboot
		combo_ble_sw_reset(); // ble reset
		reboot_func(1);

		while (1)
		{
			vTaskDelay(10);
		}
	}
#endif // __SUPPORT_BLE_PROVISIONING__

#endif /* __WFSVC_ENABLE__ */
	return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Sends a exchange MTU command
 * @param none
 * @return void
 ****************************************************************************************
 */
void user_gapc_set_le_pkt_size_cmd(void)
{
	struct gapc_set_le_pkt_size_cmd *cmd =  BleMsgAlloc(GAPC_SET_LE_PKT_SIZE_CMD,
											TASK_ID_GAPC, TASK_ID_GTL, 
											sizeof(struct gapc_set_le_pkt_size_cmd));
	cmd->operation = GAPC_SET_LE_PKT_SIZE;
	cmd->tx_octets = MY_MAX_MTU;
	cmd->tx_time = MY_MAX_MTU_TIME;
	BleSendMsg(cmd);
}

uint8_t gattc_write_req_ind_hnd (ke_msg_id_t const msgid,
								 struct gattc_write_req_ind const *param,
								 ke_task_id_t const dest_id,
								 ke_task_id_t const src_id, 
								 uint8_t triggered_by_write_cmd)
{
	DA16X_UNUSED_ARG(triggered_by_write_cmd);

#if defined (__WFSVC_ENABLE__)
	user_custom_profile_wfsvc_write_cmd_ind_xxx(msgid, param, dest_id, src_id);
#endif
	return KE_MSG_CONSUMED;
}

/*
 * Indicate that write operation is requested.
*/
uint8_t gattc_att_info_req_ind_hnd (ke_msg_id_t const msgid,
									struct gattc_att_info_req_ind const *param,
									ke_task_id_t const dest_id,
									ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);

#if defined (__WFSVC_ENABLE__)
	struct gattc_att_info_cfm *cfm = BleMsgAlloc(GATTC_ATT_INFO_CFM, src_id, dest_id,
									 sizeof (struct gattc_att_info_cfm));

	// Set parameters
	cfm->handle = param->handle;
	cfm->status = ATT_ERR_NO_ERROR;


	uint8_t temp_handle = param->handle - user_custom_profile_wfsvc_env.start_handle;

	switch (temp_handle) //Only RI characteristic Descriptors are included in the switch
	{

		// User Characteristic Descriptors
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_USER_DESC_IDX:
		{
			cfm->length = LEN_WFSVC_WFCMD_DESC + 1;
		} break;
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_USER_DESC_IDX:
		{
			cfm->length = LEN_WFSVC_WFACT_RES_DESC + 1;
		} break;
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_USER_DESC_IDX:
		{
			cfm->length = LEN_WFSVC_APSCAN_RES_DESC + 1;
		} break;
#if defined ( __SUPPORT_BLE_PROVISIONING__)
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_USER_DESC_IDX:
		{
			cfm->length = LEN_WFSVC_PROVISIONING_RES_DESC + 1;
		} break;
#endif	//  __SUPPORT_BLE_PROVISIONING__		
		// Configuration Characteristic Descriptors (always RI)
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX:
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX:
#if defined ( __SUPPORT_BLE_PROVISIONING__)
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX:
#endif //  __SUPPORT_BLE_PROVISIONING__			
		{
			cfm->length = 0x02;
		} break;
		// characteristic values
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_VALUE_IDX:
		{
			cfm->length = DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD_RES;
		} break;
		// for the following characteristics, return invalid
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_VALUE_IDX: // no read access
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_VALUE_IDX:
#if defined ( __SUPPORT_BLE_PROVISIONING__)
		case USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_VALUE_IDX:
#endif //  __SUPPORT_BLE_PROVISIONING__			
		default:
		{
			cfm->status = ATT_ERR_INVALID_HANDLE;
			cfm->length = 0;
		} break;
	}

	BleSendMsg(cfm);
#endif
	return KE_MSG_CONSUMED;
}

uint8_t gattm_att_set_value_rsp_hnd(ke_msg_id_t const msgid,
									struct gattm_att_set_value_rsp const *param,
									ke_task_id_t const dest_id,
									ke_task_id_t const src_id)
{
#if defined (__WFSVC_ENABLE__)

	DBG_LOW("[%s]	GATTM_ATT_SET_VALUE_RSP, char_enum_idx = %d, status = %d \n",
			__func__, param->handle-my_custom_service_wfsvc_start_handle-1, param->status);

	if ((user_custom_profile_wfsvc_env.start_handle <= param->handle)
			&& (user_custom_profile_wfsvc_env.end_handle >= param->handle))
	{
		user_custom_profile_wfsvc_att_set_value_rsp_hnd(msgid, param, dest_id, src_id);
	}

#endif /* __WFSVC_ENABLE__ */
	return KE_MSG_CONSUMED;
}

/*
GATTM > Service management

	Set permission settings of service response
*/
uint8_t gattm_svc_set_permission_rsp_hnd(ke_msg_id_t const msgid,
		struct gattm_svc_set_permission_rsp const *param,
		ke_task_id_t const dest_id,
		ke_task_id_t const src_id)
{

#if defined (__WFSVC_ENABLE__)

	if (user_custom_profile_wfsvc_env.start_handle == param->start_hdl)
	{
		user_custom_profile_wfsvc_gattm_svc_set_permission_rsp_hnd(msgid, param, dest_id, src_id);
	}

#endif /* __WFSVC_ENABLE__ */

	return KE_MSG_CONSUMED;
}


/*
GATTM > Attribute Manipulation

	Set permission settings of attribute response
*/
uint8_t gattm_att_set_permission_rsp_hnd(ke_msg_id_t const msgid,
		struct gattm_att_set_permission_rsp const *param,
		ke_task_id_t const dest_id,
		ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	return KE_MSG_CONSUMED;
}

uint8_t gattc_cmp_evt_hnd(ke_msg_id_t const msgid,
						  struct gattc_cmp_evt const *param,
						  ke_task_id_t const dest_id,
						  ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	switch (param->operation)
	{
		case GATTC_MTU_EXCH:
		{
			if (param->status == ATT_ERR_NO_ERROR)
			{
				user_gapc_set_le_pkt_size_cmd();
			}
		} break;
#if defined (__WFSVC_ENABLE__)
		case GATTC_NOTIFY:
		{
			if (param->status == ATT_ERR_NO_ERROR)
			{
				// to-do if any
			}
		} break;
#endif /* __WFSVC_ENABLE__ */
		default:
		{
			if (param->status != ATT_ERR_NO_ERROR)
			{
				// to-do if any
			}
		} break;
	}

	return KE_MSG_CONSUMED;
}
#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */
