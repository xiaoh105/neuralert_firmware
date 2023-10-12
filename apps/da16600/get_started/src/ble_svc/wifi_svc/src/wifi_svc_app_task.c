/**
 ****************************************************************************************
 *
 * @file wifi_svc_app_task.c
 *
 * @brief Handling of ble events and responses.
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

#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"
#include "iface_defs.h"

#include "../../include/da14585_config.h"
#include "da16_gtl_features.h"

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif

#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif
#include "../../include/app.h"
#include "../../include/app_task.h"
#include "../../include/queue.h"
#include "../../include/console.h"
#include "../../include/user_config.h"
#include "../../include/ble_msg.h"
#include "../../include/gatt_msgs.h"

#include "proxr_task.h"
#include "diss_task.h"
#include "proxr.h"
#if defined (__MULTI_BONDING_SUPPORT__)
#include "../../include/user_bonding.h"
#endif

#include "sleep2_monitor.h"
#ifdef __APP_SLEEP2_MONITOR__
extern uint8_t is_first_gtl_msg;
#endif

#if defined(__USER_SENSOR__)
#include "../../user_sensor/user_sensor.h"
#endif

#if defined (__IOT_SENSOR_SVC_ENABLE__)
#include "../../include/iot_sensor_user_custom_profile.h"
#endif
#include "../../user_util/user_util.h"

// application alert state structure
app_alert_state alert_state;
uint8_t ble_app_fw_version[APP_FW_VERSION_STRING_MAX_LENGTH];

#define IOT_SENSOR_EVT_POST 0x1

#if defined (__MULTI_BONDING_SUPPORT__)
extern struct app_sec_env_tag temp_bond_info;
extern struct app_sec_env_tag bond_info[];
extern struct gapc_ltk rand_ltk;
extern uint8_t bond_info_current_index;

struct app_sec_env_tag app_sec_env;
#endif /* __MULTI_BONDING_SUPPORT__ */

extern ble_boot_mode_t ble_boot_mode;
#if defined (__LOW_POWER_IOT_SENSOR__)
extern iot_sensor_data_t iot_sensor_data_info;
#endif /* __LOW_POWER_IOT_SENSOR__ */

#if defined (__WFSVC_ENABLE__)
extern void attm_svc_create_custom_db_wfsvc(void);
extern void set_wifi_sta_scan_result_str_idx(uint32_t idx);
#endif

extern void app_resolve_random_addr(uint8_t *addr, uint8_t *irks, uint16_t counter);
extern uint8_t addr_resolv_req_from_gapc_bond_ind;
extern uint8_t bonding_in_progress;

void app_find_device_name(unsigned char *adv_data, unsigned char adv_data_len,
						  unsigned char dev_indx)
{
	unsigned char indx = 0;

	while (indx < adv_data_len)
	{
		if (adv_data[indx + 1] == 0x09)
		{
			memcpy(app_env.devices[dev_indx].data, &adv_data[indx + 2], 
					(size_t) adv_data[indx]);
			app_env.devices[dev_indx].data_len = (unsigned char)adv_data[indx];
		}

		indx += (unsigned char ) adv_data[indx] + 1;
	}

}

int gapm_set_dev_config_completion_handler(ke_msg_id_t msgid,
		struct gapm_cmp_evt *param,
		ke_task_id_t dest_id,
		ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	app_env.state = APP_CONNECTABLE;

	app_diss_create_db();

	return (KE_MSG_CONSUMED);
}

int gapm_device_ready_ind_handler(ke_msg_id_t msgid,
				  struct gap_ready_evt *param,
				  ke_task_id_t dest_id,
				  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(src_id);

	// We are now in Connectable State
	if (dest_id == TASK_ID_GTL)
	{
		if (ble_boot_mode == BLE_BOOT_MODE_0)
		{
			app_rst_gap();
		}
	}

	return 0;
}

int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
				struct gapm_adv_report_ind *param,
				ke_task_id_t dest_id,
				ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (app_env.state != APP_SCAN)
	{
		return -1;
	}

	return 0;
}


int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
				 struct gapm_addr_solved_ind *param,
				 ke_task_id_t const dest_id,
				 ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	/*
	 *	path1: GAPC_BOND_IND (GAPC_PAIRING_SUCCEED)
	 *	path2: GAPC_ENCRYPT_REQ_IND
	 */

#if defined (__MULTI_BONDING_SUPPORT__)
    uint8_t indx;
    indx = user_resolved_rand_addr_success(&param->irk, addr_resolv_req_from_gapc_bond_ind);  
#endif
	
	DBG_INFO("    addr resolution successful \n");

	if (addr_resolv_req_from_gapc_bond_ind)
	{
		DBG_INFO("    addr_resolv_req_from_gapc_bond_ind(1) \n");
#if defined (__MULTI_BONDING_SUPPORT__)
        if (indx != BOND_INFO_CURRENT_INDEX_NONE)
        {
        	DBG_INFO("    irk is found! \n");
        	
	        memcpy (&app_sec_env, &bond_info[indx], sizeof (struct app_sec_env_tag)); 
			app_env.proxr_device.auth_lvl = app_sec_env.auth; // update app_env
	        app_connect_confirm((enum gap_auth)app_sec_env.auth, 1);
        }
		else
		{
			DBG_INFO("    irk not found! irk should've been found! see gap_bond_ind_handler(GAPC_PAIRING_SUCCEED) \n");
		}
#else	
		app_connect_confirm((enum gap_auth)app_env.proxr_device.auth_lvl, 1);
#endif
	}
	else   
	{
		// addr resolution request is from GAPC_ENCRYPT_REQ_IND (already bonded)
		DBG_INFO("    addr_resolv_req_from_gapc_bond_ind(0) \n");
		struct gapc_encrypt_cfm* cfm = BleMsgAlloc(GAPC_ENCRYPT_CFM, 
							KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx), 
							TASK_ID_GTL, 
							sizeof (struct gapc_encrypt_cfm));

#if defined (__MULTI_BONDING_SUPPORT__)
        if (indx != BOND_INFO_CURRENT_INDEX_NONE)
        {
			DBG_INFO("    irk is found! \n");
		
            memcpy((uint8_t *)&app_sec_env, (uint8_t *) &bond_info[indx], sizeof (struct app_sec_env_tag));

			// update app_env
			app_env.proxr_device.ltk.key_size = bond_info[indx].key_size;
			memcpy(&(app_env.proxr_device.ltk.ltk), &(bond_info[indx].ltk), KEY_LEN);
            
            if (app_sec_env.auth & GAP_AUTH_REQ_SECURE_CONNECTION)
            {
            	DBG_INFO("    app_connect_confirm \n");
                app_connect_confirm((enum gap_auth)app_sec_env.auth, 1);
            }
            
            cfm->found = true;
            cfm->key_size = bond_info[indx].key_size;
            memcpy(&(cfm->ltk), &(bond_info[indx].ltk), KEY_LEN);
            BleSendMsg(cfm);
        }
        else
        {
        	DBG_INFO("    irk not found! irk should've been found! see GAPC_ENCRYPT_REQ_IND \n");
            cfm->found = false;               
            BleSendMsg(cfm);
        }
#else			
		cfm->found = true;
		cfm->key_size = app_env.proxr_device.ltk.key_size;
		memcpy(&(cfm->ltk), 
			   &(app_env.proxr_device.ltk.ltk), 
			   app_env.proxr_device.ltk.key_size);

		if ((enum gap_auth)(app_env.proxr_device.auth_lvl) & GAP_AUTH_REQ_SECURE_CONNECTION)
		{
			DBG_INFO("    app_connect_confirm \n");
			app_connect_confirm((enum gap_auth)(app_env.proxr_device.auth_lvl), 1);
		}
		
		BleSendMsg(cfm);
#endif		
	}

	return (KE_MSG_CONSUMED);
}

void handle_cmp_evt_for_gapm_resolv_addr_cmd(uint8_t error)
{
	if (error == true)
	{
		DBG_INFO("    peer addr resolution not successful \n");

		if (addr_resolv_req_from_gapc_bond_ind == false)
		{

			DBG_INFO("    addr_resolv_req_from_gapc_bond_ind(0) \n");
			struct gapc_encrypt_cfm *cfm = BleMsgAlloc(GAPC_ENCRYPT_CFM,
							KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conhdl),
							TASK_ID_GTL,
							sizeof(struct gapc_encrypt_cfm));
			cfm->found = false;
			cfm->key_size = 0;
			memset(cfm->ltk.key, 0, KEY_LEN);
			BleSendMsg(cfm);
		}
		else
		{
			// addr resolution request is from GAPC_BOND_IND (GAPC_PAIRING_SUCCEED)
			DBG_INFO("    addr_resolv_req_from_gapc_bond_ind(1) \n");
			
#if defined (__MULTI_BONDING_SUPPORT__)
            for (uint8_t i=0; i<MAX_BOND_PEER; i++)
            {
            	// find an empty slot
                if (!bond_info[i].auth)
                {
                    memcpy ((uint8_t *) &bond_info[i],(uint8_t *) &temp_bond_info, sizeof(struct app_sec_env_tag));
                    break;
                }
            }
#endif
		}
	}
}
	
unsigned int start_pair;
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
									struct gapc_connection_req_ind *param,
									ke_task_id_t dest_id,
									ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	if (app_env.state == APP_IDLE || app_env.state == APP_CONNECTABLE)
	{	
#if defined (__APP_SLEEP2_MONITOR__)
		if (dpm_mode_is_enabled() == TRUE)
		{
			sleep2_monitor_set_state(SLEEP2_MON_NAME_PROV, 
						SLEEP2_CLR, __func__, __LINE__);

			// hold DPM Abnormal Checker (to prevent system from enter sleep2)
			DBG_TMP(CYAN_COLOR"[sleep2_mng] DPM Abnormal Checker HOLD (%s:%d) \n"CLEAR_COLOR, 
								__func__, __LINE__);
			dpm_abnormal_chk_hold();

			if (is_first_gtl_msg == TRUE)
			{
				// connection_ind is the 1st message
				is_first_gtl_msg = FALSE;

#if defined (__LOW_POWER_IOT_SENSOR__)
				if (iot_sensor_data_info.is_sensor_started == TRUE &&
					iot_sensor_data_info.is_gas_leak_happened == FALSE)
				{
					// if sensor leak happened (but not handled), don't set sleep2
					sleep2_monitor_set_state(SLEEP2_MON_NAME_IOT_SENSOR, 
										SLEEP2_SET, __func__, __LINE__);
				}
#endif /* __LOW_POWER_IOT_SENSOR__ */
			}
		}
#endif /* __APP_SLEEP2_MONITOR__ */

		// We are now connected
		app_env.state = APP_CONNECTED;

		/* proxr info init */
		// Retrieve the connection index from the GAPC task instance for the connection
		app_env.proxr_device.device.conidx = KE_IDX_GET(src_id);
		// Retrieve the connection handle from the parameters
		app_env.proxr_device.device.conhdl = param->conhdl;

		app_env.proxr_device.peer_addr_type = param->peer_addr_type;
		DBG_INFO("    peer bd addr type = 0x%x (0x0:public, 0x1:random)\n",
				 app_env.proxr_device.peer_addr_type);

		memcpy(app_env.proxr_device.device.adv_addr.addr, param->peer_addr.addr,
			   sizeof(struct bd_addr));
		app_env.proxr_device.rssi = 0xFF;
		app_env.proxr_device.txp = 0xFF;
		app_env.proxr_device.llv = 0xFF;

#if defined (__MULTI_BONDING_SUPPORT__)
        //reset index of current bonding data;
        user_bonding_deselect_bonded_device();
        memset(&app_sec_env, 0, sizeof(struct app_sec_env_tag));
        memset(&temp_bond_info, 0, sizeof(struct app_sec_env_tag));
#endif
		alert_state.txp_lvl = 0x00;

		DBG_INFO("    connection event intval = %d, connection latency = %d \n", 
					param->con_interval,
				 	param->con_latency);

		app_connect_confirm((enum gap_auth)(GAP_AUTH_REQ_NO_MITM_NO_BOND), 1);

#if defined (__WIFI_SVC_SECURITY__)
		/*
		Security request (saying "pls initiate bonding/pairing procedure") will be sent 
		anyway. In case the remote peer is already bonded, it will have no effect
		*/
		app_from_ext_host_security_start();
#endif /* __WIFI_SVC_SECURITY__ */

#if defined (__WFSVC_ENABLE__)
		// enable wfsvc
		user_custom_profile_wfsvc_enable(0);
#endif

#if defined (__IOT_SENSOR_SVC_ENABLE__)
#if defined (__USER_SENSOR__)
		iot_sensor_svc_enable(param->conhdl, &user_sensor_svc_callback);
#else
		iot_sensor_svc_enable(param->conhdl, NULL);
#endif
#endif /* __IOT_SENSOR_SVC_ENABLE__ */

		ConsoleConnected(0);

		user_gattc_exc_mtu_cmd();

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
		if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM)
		{
			app_set_ble_peer_idle_timer(IDLE_DURATION_AFTER_CONNECTION);
		}
#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */
	}

	return 0;
}

int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
								struct gapc_disconnect_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->conhdl == app_env.proxr_device.device.conhdl)
	{
		bonding_in_progress = false;
		app_env.state = APP_IDLE;
#if defined (__WFSVC_ENABLE__)
		set_wifi_sta_scan_result_str_idx(0);
#endif

#if defined (__IOT_SENSOR_SVC_ENABLE__)
		iot_sensor_svc_disable(param->conhdl);
#endif

#if defined (__MULTI_BONDING_SUPPORT__)
		user_bonding_clear_irks();
		user_bonding_deselect_bonded_device();
#endif
		/* Check the reason code referred in the co_error.h */
		DBG_INFO("    Device Disconnected, reason_code = 0x%x\n", param->reason);

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
		if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM)
		{
			app_del_ble_peer_idle_timer();
		}
#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */

		if (!app_is_no_adv_forced()) {
			app_adv_start();
		}

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
		if (dpm_mode_is_enabled() == TRUE)
		{
#if defined (__APP_SLEEP2_MONITOR__)
			sleep2_monitor_set_state(SLEEP2_MON_NAME_PROV, SLEEP2_SET, __func__, __LINE__);

			DBG_TMP(CYAN_COLOR"[sleep2_mng] DPM Abnormal Checker RESUME (%s:%d) \n"CLEAR_COLOR, __func__, __LINE__);
			dpm_abnormal_chk_resume();
#endif /* __APP_SLEEP2_MONITOR__ */
		}
#endif /* __ENABLE_DPM_FOR_GTL_BLE_APP__ */
	}

	return 0;
}

int gapc_con_rssi_ind_handler(ke_msg_id_t msgid,
							  struct gapc_con_rssi_ind *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	app_env.proxr_device.rssi = param->rssi;
	ConsoleConnected(1);

	return 0;
}

int gapc_get_dev_info_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_get_dev_info_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);

	switch (param->req)
	{
		case GAPC_DEV_NAME:
		{
			struct gapc_get_dev_info_cfm *cfm = BleMsgDynAlloc(GAPC_GET_DEV_INFO_CFM,
												src_id,
												dest_id,
												sizeof(struct gapc_get_dev_info_cfm),
												device_info.dev_name.length);

			cfm->req = GAPC_DEV_NAME;
			cfm->info.name.length = device_info.dev_name.length;
			memcpy(cfm->info.name.value, device_info.dev_name.name, 
					device_info.dev_name.length);
			BleSendMsg(cfm);
		} break;
		case GAPC_DEV_APPEARANCE:
		{
			struct gapc_get_dev_info_cfm *cfm = BleMsgAlloc(GAPC_GET_DEV_INFO_CFM,
												src_id,
												dest_id,
												sizeof(struct gapc_get_dev_info_cfm));

			cfm->req = GAPC_DEV_APPEARANCE;
			cfm->info.appearance = device_info.appearance;

			BleSendMsg(cfm);

		} break;
		case GAPC_DEV_SLV_PREF_PARAMS:
		{
			struct gapc_get_dev_info_cfm *cfm = BleMsgAlloc(GAPC_GET_DEV_INFO_CFM,
												src_id,
												dest_id,
												sizeof(struct gapc_get_dev_info_cfm));

			cfm->req = GAPC_DEV_SLV_PREF_PARAMS;
			cfm->info.slv_params.con_intv_min = MS_TO_DOUBLESLOTS(10);
			cfm->info.slv_params.con_intv_max = MS_TO_DOUBLESLOTS(20);
			cfm->info.slv_params.slave_latency = 0;
			cfm->info.slv_params.conn_timeout = MS_TO_TIMERUNITS(1250);

			BleSendMsg(cfm);
		} break;
		case GAPC_DEV_CENTRAL_RPA:
		{

		} break;

		case GAPC_DEV_RPA_ONLY:
		{

		} break;
		default: 
		  break;
	}

	return 0;
}

int gapc_set_dev_info_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_set_dev_info_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);

	uint8_t status = GAP_ERR_REJECTED;

	switch (param->req)
	{
		case GAPC_DEV_NAME:
		{
			device_info.dev_name.length = param->info.name.length;
			memcpy(device_info.dev_name.name, param->info.name.value, 
					device_info.dev_name.length);
			status = GAP_ERR_NO_ERROR;
		} break;
		case GAPC_DEV_APPEARANCE:
		{
			device_info.appearance = param->info.appearance;
			status = GAP_ERR_NO_ERROR;
		} break;
		default:
		{
			status = GAP_ERR_REJECTED;
		} break;
	}


	struct gapc_set_dev_info_cfm *cfm = BleMsgAlloc(GAPC_SET_DEV_INFO_CFM,
										src_id,
										dest_id,
										sizeof(struct gapc_set_dev_info_cfm));

	cfm->status = status;

	cfm->req = param->req;

	BleSendMsg(cfm);

	return 0;
}

int gapc_param_update_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_param_update_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);

	struct gapc_param_update_cfm *cfm = BleMsgAlloc(GAPC_PARAM_UPDATE_CFM,
										src_id,
										dest_id,
										sizeof(struct gapc_param_update_cfm));

	cfm->accept = true;
	cfm->ce_len_min = 0;
	cfm->ce_len_max = 0;
	BleSendMsg(cfm);
	return 0;
}

int gapc_le_pkt_size_ind_handler(ke_msg_id_t const msgid,
								 struct gapc_le_pkt_size_ind *param,
								 ke_task_id_t const dest_id,
								 ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	DBG_INFO("    max_tx_octets = %d, max_rx_octets = %d \n", param->max_tx_octets,
			 param->max_rx_octets );

	return (KE_MSG_CONSUMED);
}

int gapc_bond_ind_handler(ke_msg_id_t msgid,
						  struct gapc_bond_ind *param,
						  ke_task_id_t dest_id,
						  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

#if defined (__MULTI_BONDING_SUPPORT__)
    struct app_sec_env_tag *sec_env;
    sec_env=&temp_bond_info;
#endif

	switch (param->info)
	{
		case GAPC_PAIRING_SUCCEED:
		{
			DBG_INFO("<<< GAPC_BOND_IND (GAPC_PAIRING_SUCCEED) \n");
			app_env.proxr_device.auth_lvl = param->data.auth;
			DBG_INFO("        Auth level (0x5: Legacy Pairing, 0xD: SC) = 0x%x \n",
					 app_env.proxr_device.auth_lvl);

			if (param->data.auth & GAP_AUTH_BOND)
			{
#if defined (__MULTI_BONDING_SUPPORT__)
				sec_env->auth = param->data.auth;				
				sec_env->peer_addr_type = app_env.proxr_device.peer_addr_type;
				memcpy(sec_env->peer_addr.addr, app_env.proxr_device.device.adv_addr.addr, BD_ADDR_LEN);
				memcpy(&app_sec_env, sec_env, sizeof(struct app_sec_env_tag));
				
#endif
				if ( (app_env.proxr_device.peer_addr_type == ADDR_PUBLIC) ||
						( (app_env.proxr_device.peer_addr_type == ADDR_RAND)
						  && ((app_env.proxr_device.device.adv_addr.addr[BD_ADDR_LEN - 1] & GAP_STATIC_ADDR) == GAP_STATIC_ADDR)) )
				{
#if defined (__MULTI_BONDING_SUPPORT__)				
					DBG_INFO("         peer addr type is public / static random \n");
	                user_find_bonded_public((uint8_t *)&(app_env.proxr_device.device.adv_addr));
					user_store_bond_data();
#endif
				}
				else
				{
					DBG_INFO("         peer addr type is random resolvable \n");
#if defined (__MULTI_BONDING_SUPPORT__)
	                if (!user_resolve_addr_rand(app_env.proxr_device.device.adv_addr.addr, true))
					{
	                    user_store_bond_data();
					}
#else
					// peer addr type is random "resolvable"
					addr_resolv_req_from_gapc_bond_ind = true;
					app_resolve_random_addr(app_env.proxr_device.device.adv_addr.addr,
											app_env.proxr_device.irk.irk.key, 1);
#endif
				}
			}

			app_env.proxr_device.bonded = 1;
			bonding_in_progress = false;
			ConsoleConnected(0);	
		} break;

		case GAPC_IRK_EXCH:
		{
			DBG_INFO("<<< GAPC_BOND_IND (GAPC_IRK_EXCH) \n");
			memcpy (app_env.proxr_device.irk.irk.key, param->data.irk.irk.key, KEY_LEN);
#if defined (__MULTI_BONDING_SUPPORT__)
			memcpy (sec_env->irk.key, param->data.irk.irk.key, KEY_LEN);
			memcpy (sec_env->peer_addr.addr, app_env.proxr_device.device.adv_addr.addr, BD_ADDR_LEN);
			sec_env->peer_addr_type = app_env.proxr_device.peer_addr_type;
			sec_env->auth = app_env.proxr_device.auth_lvl;

			for (uint8_t i=0; i<MAX_BOND_PEER; i++)
			{
				// find an empty slot
				if (!bond_info[i].auth)
				{
					memcpy ((uint8_t *) &bond_info[i],(uint8_t *) sec_env, sizeof(struct app_sec_env_tag));
					user_store_bond_data_in_sflash();					
					break;
				}
			}		
#endif
		} break;

		case GAPC_LTK_EXCH:
		{
			DBG_INFO("<<< GAPC_BOND_IND (GAPC_LTK_EXCH) - Secure Connection \n");
			app_env.proxr_device.ltk.ediv = param->data.ltk.ediv;
			app_env.proxr_device.ltk.key_size = param->data.ltk.key_size;
			memcpy (app_env.proxr_device.ltk.ltk.key, param->data.ltk.ltk.key,
					param->data.ltk.key_size);
			memcpy (app_env.proxr_device.ltk.randnb.nb, 
					param->data.ltk.randnb.nb, RAND_NB_LEN);
			
#if defined (__MULTI_BONDING_SUPPORT__)
			sec_env->ediv = param->data.ltk.ediv;
			memcpy(&sec_env->rand_nb, &param->data.ltk.randnb, sizeof(struct rand_nb));
			sec_env->key_size = param->data.ltk.key_size;
			memcpy(&sec_env->ltk, &param->data.ltk.ltk, sizeof(struct gap_sec_key));
#endif
					
		} break;

		case GAPC_PAIRING_FAILED:
		{
			DBG_INFO("<<< GAPC_BOND_IND (GAPC_PAIRING_FAILED) \n");
#if defined (__MULTI_BONDING_SUPPORT__)
            user_bonding_clear_irks();
            user_bonding_deselect_bonded_device();			
#endif
			app_env.proxr_device.bonded = 0;
			app_disconnect();
		} break;

		default:
		{
			DBG_INFO("<<< GAPC_BOND_IND (No handler for param->info=%d) \n", param->info);
		} break;
	}

	return 0;
}

int gapm_reset_completion_handler(ke_msg_id_t msgid,
								  struct gapm_cmp_evt *param,
								  ke_task_id_t dest_id,
								  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(src_id);

	// We are now in Connectable State
	if (dest_id == TASK_ID_GTL)
	{
		app_env.state = APP_IDLE;
		alert_state.ll_alert_lvl = 2; // Link Loss default Alert Level is high
		alert_state.adv_toggle = 0; // clear advertise toggle
		app_set_mode(); // initialize gap mode

#if defined (__IOT_SENSOR_SVC_ENABLE__)
		iot_sensor_svc_init();
#endif

	}

	return 0;
}

int gapm_profile_added_ind_handler(ke_msg_id_t msgid,
								   struct gapm_profile_added_ind *param,
								   ke_task_id_t dest_id,
								   ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(src_id);

	if (dest_id != TASK_ID_GTL)
	{
		DBG_ERR("Error creating DB. Destination ID is not TASK_GTL \n");
		return (KE_MSG_CONSUMED);
	}
	else
	{
		switch (param->prf_task_id)
		{
			case TASK_ID_DISS:
			{
				if (app_env.state == APP_CONNECTABLE)
				{
					app_proxr_create_db();
				}

			} break;

			case TASK_ID_PROXR:
			{
				if (app_env.state == APP_CONNECTABLE)
				{
#if defined (__WFSVC_ENABLE__)
					attm_svc_create_custom_db_wfsvc(); // add Wi-Fi SVC
#elif defined (__IOT_SENSOR_SVC_ENABLE__)
					iot_sensor_svc_create_custom_db(0);
#else
					app_adv_start();
#endif
				}
			} break;

			default:
			{
				
			} break;
		}
	}

	return (KE_MSG_CONSUMED);
}

int gapc_bond_req_ind_handler(ke_msg_id_t msgid,
							  struct gapc_bond_req_ind *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	app_gap_bond_cfm(param);

	return 0;
}

int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
								 struct gapc_encrypt_req_ind *param,
								 ke_task_id_t const dest_id,
								 ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);

#if defined (__MULTI_BONDING_SUPPORT__)
	uint8_t i, indx;
#endif
	uint8_t connect_cfm_needed = false;
	uint8_t send_gapc_encrypt_cfm = true;
	
#ifdef __USE_SECURITY_INFO_DEBUG__
	int t;
	uint8_t *randnb = (uint8_t *)(app_env.proxr_device.ltk.randnb.nb);
	DBG_INFO("--------------------------\n");
	DBG_INFO("app_env: bonded=%d, ediv=%d \n", app_env.proxr_device.bonded,
			 app_env.proxr_device.ltk.ediv);

	DBG_INFO("app_env.proxr_device.ltk.randnb = \n");

	for (t = 0; t < RAND_NB_LEN; t++)
	{
		DBG_INFO("0x%x ", randnb[t]);
	}

	DBG_INFO("--------------------------\n");

	DBG_INFO("param->ediv=%d \n", param->ediv);

	randnb = (uint8_t *)(param->rand_nb.nb);
	DBG_INFO("param->rand_nb = \n");

	for (t = 0; t < RAND_NB_LEN; t++)
	{
		DBG_INFO("0x%x ", randnb[t]);
	}

	DBG_INFO("\n");
	DBG_INFO("--------------------------\n");
#endif

	struct gapc_encrypt_cfm *cfm = BleMsgAlloc(GAPC_ENCRYPT_CFM, src_id, dest_id,
								   sizeof(struct gapc_encrypt_cfm));
	cfm->found = false;

	if (bonding_in_progress == true)
	{
		DBG_INFO("    bonding is in progress\n");
		cfm->found = true;
		connect_cfm_needed = false;
		cfm->key_size = app_env.proxr_device.ltk.key_size;
		memcpy(&(cfm->ltk), &(app_env.proxr_device.ltk.ltk), 
				app_env.proxr_device.ltk.key_size);
	}
	else
	{
		DBG_INFO("    bonding not tried or bonding in place ... \n");
		
		const struct rand_nb empty_rand_nb = {{0, 0, 0, 0, 0, 0, 0, 0}};
		if ((memcmp(&(param->rand_nb), &empty_rand_nb, RAND_NB_LEN) == 0) && (param->ediv == 0))
		{
			DBG_INFO("    LE Secure Connection ... \n");
			//Secure Connection Pairing

			if ( (app_env.proxr_device.peer_addr_type == ADDR_PUBLIC) ||
					( (app_env.proxr_device.peer_addr_type == ADDR_RAND) &&
					  ((app_env.proxr_device.device.adv_addr.addr[BD_ADDR_LEN - 1] & GAP_STATIC_ADDR) == GAP_STATIC_ADDR)) )
			{
				// public / static random
#if defined (__MULTI_BONDING_SUPPORT__)
				indx = user_find_bonded_public((uint8_t *)(&(app_env.proxr_device.device.adv_addr.addr)));
                if(indx != 0xff)
                {
                	DBG_INFO("        peer address is public or static random, bonded=yes \n");
                    cfm->found = true;
                    connect_cfm_needed = true;
                    cfm->key_size = bond_info[indx].key_size;
                    memcpy(&(cfm->ltk), &(bond_info[indx].ltk), KEY_LEN);

					// update the current app_env
					app_env.proxr_device.ltk.key_size = bond_info[indx].key_size;
					memcpy(&(app_env.proxr_device.ltk.ltk), &(bond_info[indx].ltk), KEY_LEN);

					// update the current app_sec_env
                    memcpy ((uint8_t *)&app_sec_env, (uint8_t *) &bond_info[indx], sizeof (struct app_sec_env_tag));                
                }
                else
                {
					// peer addr is public or static random
					DBG_INFO("		  peer address is public or static random, bonded=no \n");
					DBG_INFO(YEL_COL "		  pls remove pairing in the peer and try connection again \n" CLR_COL);
					cfm->found = false;
					connect_cfm_needed = false;				
                }
#else
				if (app_env.proxr_device.bonded)
				{
					// peer addr is public or static random
					DBG_INFO("        peer address is public or static random, bonded=yes \n");
					cfm->found = true;
					connect_cfm_needed = true;
					cfm->key_size = app_env.proxr_device.ltk.key_size;
					memcpy(&(cfm->ltk), &(app_env.proxr_device.ltk.ltk), app_env.proxr_device.ltk.key_size);					
				}
				else
				{
					// peer addr is public or static random
					DBG_INFO("        peer address is public or static random \n");
					DBG_INFO(YEL_COL "        not bonded, pls remove pairing in the peer and try connection again \n" CLR_COL);
					cfm->found = false;
					connect_cfm_needed = false;
				}
#endif
			}
			else
			{
				// private random resolvable
				
				// peer addr is resolvable random -> address resolution req is to be sent to Stack
				DBG_INFO("        peer address is resolvable random \n");
				
#if defined (__MULTI_BONDING_SUPPORT__)
                if (user_resolve_addr_rand(app_env.proxr_device.device.adv_addr.addr, false) == true)
                {
                	/*
						At least, an irk is found in Security DB, connection will be confirmed 
						after address resolution is decided
					*/
					send_gapc_encrypt_cfm = false;
					connect_cfm_needed = false;
                }
				else
				{
					// an irk is not found
					send_gapc_encrypt_cfm = true;
					connect_cfm_needed = false;
				}
#else
				if (!app_env.proxr_device.bonded)
				{
					DBG_INFO(YEL_COL "        not bonded, pls remove pairing in the peer and try connection again \n" CLR_COL);
					cfm->found = false;               
            		send_gapc_encrypt_cfm = true;
					connect_cfm_needed = false;
				}
				else
				{
					DBG_INFO("        bonded \n");
					addr_resolv_req_from_gapc_bond_ind = false;
					send_gapc_encrypt_cfm = 0;
					app_resolve_random_addr(app_env.proxr_device.device.adv_addr.addr,
											app_env.proxr_device.irk.irk.key, 1);
				}
#endif
			}
		}
		else
		{
			DBG_INFO("    LE Legacy connection ... \n");
#if defined (__MULTI_BONDING_SUPPORT__)

            for (i=0; i<MAX_BOND_PEER; i++)
            {  
               if ((memcmp(&(bond_info[i].rand_nb), &(param->rand_nb), RAND_NB_LEN) == 0) && (bond_info[i].ediv == param->ediv))
                {
            		DBG_INFO("        bonded data of this peer found \n");
                    cfm->found = true;
                    connect_cfm_needed = true;
                    cfm->key_size = bond_info[i].key_size;                                  
                    memcpy(&(cfm->ltk), &(bond_info[i].ltk), KEY_LEN);

					// update app_env
                    app_env.proxr_device.ltk.key_size = bond_info[i].key_size;                                  
                    memcpy(&(app_env.proxr_device.ltk.ltk), &(bond_info[i].ltk), KEY_LEN);

                    memcpy ((uint8_t *)&app_sec_env, (uint8_t *) &bond_info[i], sizeof (struct app_sec_env_tag));
                    bond_info_current_index = i;
                    break;
                }
            }

			if (cfm->found == false)
			{
				DBG_INFO(YEL_COL "        not bonded, pls remove pairing in the peer and try connection again \n" CLR_COL);
				send_gapc_encrypt_cfm = true;
				connect_cfm_needed = false;
			}
#else
			// Legacy pairing
			if ((app_env.proxr_device.bonded) &&
					(memcmp(&(app_env.proxr_device.ltk.randnb), &(param->rand_nb), RAND_NB_LEN) == 0) &&
					(app_env.proxr_device.ltk.ediv == param->ediv))
			{
				DBG_INFO("        bonded data of this peer found \n");
				// LTK info bonded are matched !!, proceed confirm
				cfm->found = true;
				connect_cfm_needed = true;
				cfm->key_size = app_env.proxr_device.ltk.key_size;
				memcpy(&(cfm->ltk), &(app_env.proxr_device.ltk.ltk), KEY_LEN);
			}
			else
			{
				DBG_INFO(YEL_COL "        not bonded, pls remove pairing in the peer and try connection again \n" CLR_COL);
				cfm->found = false;               
           		send_gapc_encrypt_cfm = true;
				connect_cfm_needed = false;				
			}
#endif
		}
	}

	if (!send_gapc_encrypt_cfm)
	{
		DBG_INFO("    send_gapc_encrypt_cfm(0) \n");
		BleFreeMsg(cfm);
	}
	else
	{
		DBG_INFO("    send_gapc_encrypt_cfm(1) \n");
		BleSendMsg(cfm);
	}

	if (connect_cfm_needed == true)
	{
		DBG_INFO("    connect_cfm_needed(1) \n");
#if defined (__MULTI_BONDING_SUPPORT__)
		app_connect_confirm((enum gap_auth)app_sec_env.auth, 1);
#else
		app_connect_confirm((enum gap_auth)(app_env.proxr_device.auth_lvl), 1);
#endif
	}

	return (KE_MSG_CONSUMED);
}

int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
							 struct gapc_encrypt_ind *param,
							 ke_task_id_t const dest_id,
							 ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	DBG_INFO("    auth_level = 0x%x\n", param->auth);

	return (KE_MSG_CONSUMED);
}

int gapc_param_updated_ind_handler(ke_msg_id_t const msgid,
								   struct gapc_param_updated_ind *param,
								   ke_task_id_t const dest_id,
								   ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	DBG_INFO("    conn interval = %d, slave latency = %d, supervision timeout = %d \n", 
			param->con_interval, param->con_latency, param->sup_to);
			
	return (KE_MSG_CONSUMED);
}


int gattc_mtu_changed_ind_handler (ke_msg_id_t msgid,
								   struct gattc_mtu_changed_ind *param,
								   ke_task_id_t dest_id,
								   ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	// update tx payload size (in local application data structure) if needed

	DBG_INFO("    MTU = %d\n", param->mtu);

	return (KE_MSG_CONSUMED);
}


void diss_value_req_ind_handler(ke_msg_id_t const msgid,
								struct diss_value_req_ind *param,
								ke_task_id_t const dest_id,
								ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);

	// Initialize length
	uint8_t len = 0;
	// Pointer to the data
	uint8_t *data = NULL;

	// Check requested value
	switch (param->value)
	{
		case DIS_MANUFACTURER_NAME_CHAR:
		{
			// Set information
			len = APP_DIS_MANUFACTURER_NAME_LEN;
			data = (uint8_t *)APP_DIS_MANUFACTURER_NAME;
		} break;
		case DIS_MODEL_NB_STR_CHAR:
		{
			// Set information
			len = APP_DIS_MODEL_NB_STR_LEN;
			data = (uint8_t *)APP_DIS_MODEL_NB_STR;
		} break;
		case DIS_SYSTEM_ID_CHAR:
		{
			// Set information
			len = APP_DIS_SYSTEM_ID_LEN;
			data = (uint8_t *)APP_DIS_SYSTEM_ID;
		} break;
		case DIS_PNP_ID_CHAR:
		{
			// Set information
			len = APP_DIS_PNP_ID_LEN;
			data = (uint8_t *)APP_DIS_PNP_ID;
		} break;
		case DIS_SERIAL_NB_STR_CHAR:
		{
			// Set information
			len = APP_DIS_SERIAL_NB_STR_LEN;
			data = (uint8_t *)APP_DIS_SERIAL_NB_STR;
		} break;
		case DIS_HARD_REV_STR_CHAR:
		{
			// Set information
			len = APP_DIS_HARD_REV_STR_LEN;
			data = (uint8_t *)APP_DIS_HARD_REV_STR;
		} break;
		case DIS_FIRM_REV_STR_CHAR:
		{
			// Set information
			len = APP_DIS_FIRM_REV_STR_LEN;
			data = (uint8_t *)APP_DIS_FIRM_REV_STR;
		} break;
		case DIS_SW_REV_STR_CHAR:
		{
			// Set information
			len = APP_DIS_SW_REV_STR_LEN;
			data = (uint8_t *)APP_DIS_SW_REV_STR;
		} break;
		case DIS_IEEE_CHAR:
		{
			// Set information
			len = APP_DIS_IEEE_LEN;
			data = (uint8_t *)APP_DIS_IEEE;
		} break;
		default:
		  break;
	}

	// Allocate confirmation to send the value
	struct diss_value_cfm *cfm_value = BleMsgDynAlloc(DISS_VALUE_CFM,
									   src_id,
									   dest_id,
									   sizeof (struct diss_value_cfm),
									   (sizeof (uint8_t) * len));


	// Set parameters
	cfm_value->value = param->value;
	cfm_value->length = len;

	if (len)
	{
		// Copy data
		memcpy(&cfm_value->data[0], data, len);
	}

	// Send the message
	BleSendMsg((void *) cfm_value);
}

static void update_visual_alert_indication(uint8_t alert_level)
{
	if (alert_level == PROXR_ALERT_NONE)
	{
		DBG_INFO("ALERT STOPPED.\n");
	}
	else
	{
		DBG_INFO("ALERT STARTED. Level:%d\n", alert_level);
	}
}

void proxr_alert_ind_handler(ke_msg_id_t msgid,
							 struct proxr_alert_ind  *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->char_code == PROXR_IAS_CHAR)
	{
		DBG_INFO("IAS Alert Level updated by peer. New value = %d\n", param->alert_lvl);
		update_visual_alert_indication(param->alert_lvl);
	}
	else if (param->char_code == PROXR_LLS_CHAR)
	{
		DBG_INFO("LLS Alert Level updated by peer. New value = %d\n", param->alert_lvl);
		update_visual_alert_indication(param->alert_lvl);
	}

}

#if defined (__LOW_POWER_IOT_SENSOR__)
int app_sensor_start_cfm_hnd(ke_msg_id_t msgid,
							 app_gas_leak_sensor_start_cfm_t *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	switch (param->status)
	{
		case APP_NO_ERROR:
		{
			app_env.iot_sensor_state = IOT_SENSOR_ACTIVE;

			// update iot_sensor info (rtm) with latest info
			iot_sensor_data_info.is_sensor_started = 1;

#if defined (__APP_SLEEP2_MONITOR__)
			sleep2_monitor_set_state(SLEEP2_MON_NAME_IOT_SENSOR, 
									SLEEP2_SET, __func__, __LINE__);
#endif /* __APP_SLEEP2_MONITOR__ */
		} break;
		case APP_ERROR_INVALID_PARAMS:
			break;

		default:
			break;
	}
	return 0;
}


int app_sensor_stop_cfm_hnd(ke_msg_id_t msgid,
							app_gas_leak_sensor_stop_cfm_t *param,
							ke_task_id_t dest_id,
							ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	switch (param->status)
	{
		case APP_NO_ERROR:
		{
			app_env.iot_sensor_state = IOT_SENSOR_INACTIVE;
		} break;
		case APP_ERROR_INVALID_PARAMS:
		{
		} break;
		default:
			break;
	}

	return 0;
}

int app_sensor_event_ind_hnd(ke_msg_id_t msgid,
							 app_gas_leak_evt_ind_t *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	DBG_INFO("gas leak occurred !!! \n");

#if defined (__APP_SLEEP2_MONITOR__)
	if (dpm_mode_is_enabled() == TRUE)
	{
		sleep2_monitor_set_state(SLEEP2_MON_NAME_IOT_SENSOR, 
				SLEEP2_CLR, __func__, __LINE__);
		
		if (is_first_gtl_msg == TRUE)
		{
			// this is the 1st message
			is_first_gtl_msg = FALSE;

			sleep2_monitor_set_state(SLEEP2_MON_NAME_PROV, 
									SLEEP2_SET, __func__, __LINE__);
			
			DBG_TMP(CYAN_COLOR"[sleep2_mng] DPM abnormal RESUME (%s:%d) \n"CLEAR_COLOR, 
													__func__, __LINE__);
			dpm_abnormal_chk_resume();
		}
		else
		{
			if (sleep2_monitor_get_state(SLEEP2_MON_NAME_PROV) == SLEEP2_SET)
			{	
				DBG_TMP(CYAN_COLOR"[sleep2_mng] DPM abnormal RESUME (%s:%d) \n"CLEAR_COLOR, 
													__func__, __LINE__);
				dpm_abnormal_chk_resume();
			}
		}
	}
#endif /* __APP_SLEEP2_MONITOR__ */

	if (iot_sensor_data_info.is_provisioned == 0)
	{
		DBG_INFO(" DUT not provisioned, please do provision in order to post gas leak msg \n");
		return 0;
	}
	
	iot_sensor_data_info.is_gas_leak_happened = TRUE;

	return 0;
}
#endif /* __LOW_POWER_IOT_SENSOR__ */

int app_fw_version_ind_handler (ke_msg_id_t msgid,
                                    app_fw_version_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

    memcpy(&ble_app_fw_version, param->app_fw_version, APP_FW_VERSION_STRING_MAX_LENGTH);

	DBG_INFO(" BLE FW Version = %s\n", ble_app_fw_version);
    return (KE_MSG_CONSUMED);    
}

#endif /* End of __BLE_PERI_WIFI_SVC__ */

/* EOF */
