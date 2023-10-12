/**
 ****************************************************************************************
 *
 * @file sensor_gw_app_task.c
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

#include "sdk_type.h"

#if defined (__BLE_CENT_SENSOR_GW__)

// ble config
#include "../inc/da14585_config.h"

// ble platform common include
#include "gapc_task.h"
#include "gapm_task.h"
#include "proxm.h"
#include "proxm_task.h"
#include "disc.h"
#include "disc_task.h"

// gtl platform common include
#include "../../include/queue.h"
#include "../../include/ble_msg.h"
#include "../../include/uart.h"

// project specific include
#include "../inc/app_task.h"
#include "../inc/app.h"
#include "../../include/user_config.h"
#include "../inc/console.h"

#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif /* __WFSVC_ENABLE__ */

#include "da16_gtl_features.h"

extern unsigned int proxm_trans_in_prog;
extern uint8_t last_char[MAX_CONN_NUMBER];
extern bool connected_to_at_least_one_peer;

#if defined (__WFSVC_ENABLE__)
extern void attm_svc_create_custom_db_wfsvc(void);
extern void set_wifi_sta_scan_result_str_idx(uint32_t idx);
#endif /* __WFSVC_ENABLE__ */


/**
 ****************************************************************************************
 * @brief Extracts device name from adv data if present and copy it to app_env.
 *
 * @param[in] adv_data      Pointer to advertise data.
 * @param[in] adv_data_len  Advertise data length.
 * @param[in] dev_indx      Devices index in device list.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static void app_find_device_name(unsigned char *adv_data, unsigned char adv_data_len,
								 unsigned char dev_indx)
{
	unsigned char indx = 0;

	// [AD0_Len:1byte][AD0_Type:1byte][AD0_Data:~]...[AD1_Len:1byte][AD1_Type:1byte][AD1_Data:~]
	while (indx < adv_data_len)
	{
		if (adv_data[indx + 1] == 0x09)
		{
			memcpy(app_env.devices[dev_indx].data, &adv_data[indx + 2], (size_t) adv_data[indx]);
			app_env.devices[dev_indx].data_len = (unsigned char ) adv_data[indx];
		}

		indx += (unsigned char ) adv_data[indx] + 1;
	}
}


static uint8_t app_device_name_exist(unsigned char *adv_data, unsigned char adv_data_len,
									 unsigned char dev_indx)
{
	DA16X_UNUSED_ARG(dev_indx);

	unsigned char indx = 0;

	// [AD0_Len:1byte][AD0_Type:1byte][AD0_Data:~]...[AD1_Len:1byte][AD1_Type:1byte][AD1_Data:~]
	while (indx < adv_data_len)
	{
		if ((adv_data[indx + 1] == 0x09) || (adv_data[indx + 1] == 0x08))
		{
			return 1; // device name exists
		}

		indx += (unsigned char ) adv_data[indx] + 1;
	}

	return 0; // device name not exist.
}

int gapm_cmp_evt_handler(ke_msg_id_t msgid,
						 struct gapm_cmp_evt *param,
						 ke_task_id_t dest_id,
						 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->status ==  GAP_ERR_NO_ERROR)
	{
		if (param->operation == GAPM_RESET)
		{
			DBG_INFO("<<< GAPM_CMP_EVT (ops=GAPM_RESET) \n");
#if defined (__WFSVC_ENABLE__)

			if (app_wf_prov_mode_is_enabled())
			{
				app_set_mode_peri();
			}
			else
#endif
				app_set_mode_cent();
		}
		else if (param->operation == GAPM_SET_DEV_CONFIG)
		{
			DBG_INFO("<<< GAPM_CMP_EVT (ops=GAPM_SET_DEV_CONFIG) \n");
#if defined (__WFSVC_ENABLE__)

			if (app_wf_prov_mode_is_enabled())
			{
				attm_svc_create_custom_db_wfsvc();
			}
			else
#endif
			{
				app_disc_create();
				vTaskDelay(10);
				app_proxm_create();
				app_env.state = APP_SCAN;
				vTaskDelay(10);

				app_inq();  //start scanning
				ConsoleScan();
			}
		}
	}
	else
	{
		if (!connected_to_at_least_one_peer)
		{
			if (param->operation == GAPM_SCAN_ACTIVE || param->operation == GAPM_SCAN_PASSIVE)
			{
				app_env.state = APP_IDLE;
				DBG_INFO("\r >>> Please connect or rescan. Type in \"[/DA16600] # ble.proxm_sensor_gw\" for cmd options \n");
			}
		}
		else // in "connected" state
		{
			if (param->operation == GAPM_SCAN_ACTIVE || param->operation == GAPM_SCAN_PASSIVE)
			{
				//app_env.state = APP_IDLE;
				DBG_INFO("\r >>> Please connect or rescan. Type in \"[/DA16600] # ble.proxm_sensor_gw\" for cmd options \n");
			}
		}
	}

	return (KE_MSG_CONSUMED);
}


int gapc_cmp_evt_handler(ke_msg_id_t msgid,
						 struct gapc_cmp_evt *param,
						 ke_task_id_t dest_id,
						 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int con_id;
	con_id = res_conidx(KE_IDX_GET(src_id));

	switch (param->operation)
	{
		case GAPC_ENCRYPT:
		{
			DBG_INFO("<<< GAPC_CMP_EVT (ops=GAPC_ENCRYPT) \n");

			if (param->status != ATT_ERR_NO_ERROR)
			{
				app_env.proxr_device[con_id].bonded = 0;
				app_disconnect(con_id);
				DBG_INFO(">>> GAPC_DISCONNECT_CMD \n");
			}

		} break;
		default:
			break;
	}

	return 0;
}


int gapm_device_ready_ind_handler(ke_msg_id_t msgid,
								  void *param,
								  ke_task_id_t dest_id,
								  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	app_rst_gap();
	return 0;
}


int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
								struct gapm_adv_report_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	unsigned char recorded;

	//DBG_INFO("GAPM_ADV_REPORT_IND\n");

	recorded = app_device_recorded(&param->report.adv_addr);

	if (recorded < MAX_SCAN_DEVICES) //update Name
	{
		app_find_device_name(param->report.data, param->report.data_len, recorded);
		ConsoleScan();
	}
	else if (app_env.num_of_devices < MAX_SCAN_DEVICES)
	{
		// add only if Device Name exists, customer can change this behavior later if they want
		if (app_device_name_exist(param->report.data, param->report.data_len,
								  app_env.num_of_devices))
		{
		    app_env.devices[app_env.num_of_devices].adv_addr_type = param->report.adv_addr_type;
			app_env.devices[app_env.num_of_devices].free = false;
			app_env.devices[app_env.num_of_devices].rssi = param->report.rssi;
			DBG_INFO("param->report.rssi = %d, app_env.devices[%d].rssi = %d \n", param->report.rssi,
					 app_env.num_of_devices, (CHAR)(app_env.devices[app_env.num_of_devices].rssi) );
			memcpy(app_env.devices[app_env.num_of_devices].adv_addr.addr, param->report.adv_addr.addr,
				   BD_ADDR_LEN );
			app_find_device_name(param->report.data, param->report.data_len, app_env.num_of_devices);
			ConsoleScan();
			app_env.num_of_devices++;
		}
	}

	return 0;
}


unsigned int start_pair;
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
									struct gapc_connection_req_ind *param,
									ke_task_id_t dest_id,
									ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int i;
	int con_id;
	con_id = rtrn_fst_avail();
	app_env.proxr_device[con_id].isactive = true;
	start_pair = 1;

	if (app_env.state == APP_IDLE || connected_to_at_least_one_peer)
	{
		// We are now connected

		app_env.state = APP_CONNECTED;

		// Retrieve the connection index from the GAPC task instance for this connection
		app_env.proxr_device[con_id].device.conidx = KE_IDX_GET(src_id);

		// Retrieve the connection handle from the parameters
		app_env.proxr_device[con_id].device.conhdl = param->conhdl;

		// On Reconnection check if device is bonded and send pairing request. Otherwise it is not bonded.
		if (bdaddr_compare(&app_env.proxr_device[con_id].device.adv_addr, &param->peer_addr))
		{
			if (app_env.proxr_device[con_id].bonded)
			{
				start_pair = 0;
			}
		}

		memcpy(app_env.proxr_device[con_id].device.adv_addr.addr, param->peer_addr.addr,
			   sizeof(struct bd_addr));

		for (i = 0 ; i < RSSI_SAMPLES; i++)
		{
			app_env.proxr_device[con_id].rssi[i] = (CHAR) - 127;
		}

		app_env.proxr_device[con_id].alert = 0;
		app_env.proxr_device[con_id].rssi_indx = 0;
		app_env.proxr_device[con_id].avg_rssi = (CHAR) - 127;
		app_env.proxr_device[con_id].txp = (CHAR) - 127;
		app_env.proxr_device[con_id].llv = 0xFF;
		memset(&app_env.proxr_device[con_id].dis, 0, sizeof(dis_env));

#if defined (__WFSVC_ENABLE__)
		if (!app_wf_prov_mode_is_enabled())
#endif
		{
			ConsoleClear();
			ConsoleConnected(1);
		}

		app_connect_confirm(GAP_AUTH_REQ_NO_MITM_NO_BOND, con_id);

#if defined (__WFSVC_ENABLE__)
		if (app_wf_prov_mode_is_enabled())
		{
			user_custom_profile_wfsvc_enable(0);
			user_gattc_exc_mtu_cmd();
			return 0;
		}
#endif
		app_proxm_enable(con_id);
		app_disc_enable(con_id);

		// init local svc data
		app_env.proxr_device[con_id].iot_sensor.svc_found = 0;
		app_env.proxr_device[con_id].iot_sensor.svc_start_handle = 0;
		app_env.proxr_device[con_id].iot_sensor.svc_end_handle = 0;
		app_env.proxr_device[con_id].iot_sensor.temp_char_val_handle = 0;
		app_env.proxr_device[con_id].iot_sensor.temp_cccd_hdl = 0;
		app_env.proxr_device[con_id].iot_sensor.iot_sensor_act_sta = IOT_SENSOR_UNDEF;
		app_env.proxr_device[con_id].iot_sensor.temp_post_enabled = 0;
		app_env.proxr_device[con_id].iot_sensor.temp_val[0] = 100;

		app_discover_svc_by_uuid_128(app_env.proxr_device[con_id].device.conidx,
									 IOT_SENSOR_SVC_UUID);

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

	int con_id;
	con_id = res_conidx(KE_IDX_GET(src_id));
	DBG_INFO("disconnect reason = %d, see co_error.h \n", param->reason);

	if (param->conhdl == app_env.proxr_device[con_id].device.conhdl)
	{
		if (!connected_to_at_least_one_peer)
		{
#if defined (__WFSVC_ENABLE__)
			if (!app_wf_prov_mode_is_enabled())
#endif				
			{
				app_env.state = APP_SCAN;
				vTaskDelay(10);

				if ((param->reason != CO_ERROR_REMOTE_USER_TERM_CON) &&
						(param->reason != CO_ERROR_CON_TERM_BY_LOCAL_HOST))
				{

					app_rst_gap();
				}
			}
		}

		app_env.proxr_device[con_id].isactive = false;
		app_env.proxr_device[con_id].bonded = 0;
		app_env.proxr_device[con_id].llv = 0xFF;
		app_env.proxr_device[app_env.cur_dev].txp = -127;
		last_char[con_id] = 0xFF;
		app_env.cur_dev = rtrn_prev_avail_conn();
		conn_num_flag = false;

#if defined (__WFSVC_ENABLE__)
		set_wifi_sta_scan_result_str_idx(0);

		if (app_wf_prov_mode_is_enabled())
		{
			app_env.state = APP_IDLE;
			app_set_mode_peri();
			return 0;
		}	
#endif
		ConsoleScan();

		if (rtrn_con_avail() == 0 && connected_to_at_least_one_peer)
		{
			connected_to_at_least_one_peer = false;
			app_env.state = APP_IDLE;
			ConsoleSendScan();
		}
	}

	return 0;
}


int gapc_con_rssi_ind_handler(  ke_msg_id_t msgid,
								struct gapc_con_rssi_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int count = 0;
	int sum = 0;
	int con_id;

	con_id = res_conidx(KE_IDX_GET(src_id));

	app_env.proxr_device[con_id].rssi[(uint8_t)app_env.proxr_device[con_id].rssi_indx] = param->rssi;
	app_env.proxr_device[con_id].rssi_indx ++;

	if (app_env.proxr_device[con_id].rssi_indx == RSSI_SAMPLES)
	{
		app_env.proxr_device[con_id].rssi_indx = 0;
	}

	while ((count < RSSI_SAMPLES) && (app_env.proxr_device[con_id].rssi[count] != -127))
	{
		DBG_INFO("app_env.proxr_device[%d].rssi[%d] = %d \n", con_id, count,
				 app_env.proxr_device[con_id].rssi[count]);
		sum += (int)(app_env.proxr_device[con_id].rssi[count]);
		count ++;
	}

	app_env.proxr_device[con_id].avg_rssi = (CHAR) (sum / count);
	DBG_INFO("app_env.proxr_device[%d].avg_rssi = %d\n", con_id,
			 app_env.proxr_device[con_id].avg_rssi);

	if (((CHAR)app_env.proxr_device[con_id].avg_rssi - (CHAR)app_env.proxr_device[con_id].txp) < -80)
	{
		if (!app_env.proxr_device[con_id].alert)
		{
			app_proxm_write(PROXM_SET_IMMDT_ALERT, PROXM_ALERT_MILD, con_id);
			app_env.proxr_device[con_id].alert = 1;
		}
	}
	else if (app_env.proxr_device[con_id].alert)
	{
		app_proxm_write(PROXM_SET_IMMDT_ALERT, PROXM_ALERT_NONE, con_id);
		app_env.proxr_device[con_id].alert = 0;

	}

	vTaskDelay(10);
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
			DBG_INFO("<<< GAPC_GET_DEV_INFO_REQ_IND (GAPC_DEV_NAME) \n");
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
			DBG_INFO("<<< GAPC_GET_DEV_INFO_REQ_IND (GAPC_DEV_APPEARANCE) \n");
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
			DBG_INFO("<<< GAPC_GET_DEV_INFO_REQ_IND (GAPC_DEV_SLV_PREF_PARAMS) \n");
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
			DBG_INFO("<<< GAPC_GET_DEV_INFO_REQ_IND (GAPC_DEV_CENTRAL_RPA) \n");
		} break;
		case GAPC_DEV_RPA_ONLY:
		{
			DBG_INFO("<<< GAPC_GET_DEV_INFO_REQ_IND (GAPC_DEV_RPA_ONLY) \n");
		} break;
		default:
		{
		} break;
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

int gapc_bond_ind_handler(ke_msg_id_t msgid,
						  struct gapc_bond_ind *param,
						  ke_task_id_t dest_id,
						  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int con_id = res_conidx(KE_IDX_GET(src_id));

	switch (param->info)
	{
		case GAPC_PAIRING_SUCCEED:
		{
			if (param->data.auth & GAP_AUTH_BOND)
			{
				app_env.proxr_device[con_id].bonded = 1;
			}

		} break;
		case GAPC_IRK_EXCH:
		{
			memcpy (app_env.proxr_device[con_id].irk.irk.key, param->data.irk.irk.key, 
					KEY_LEN);
			memcpy (app_env.proxr_device[con_id].irk.addr.addr.addr, 
					param->data.irk.addr.addr.addr, KEY_LEN);
			app_env.proxr_device[con_id].irk.addr.addr_type = param->data.irk.addr.addr_type;
		} break;
		case GAPC_LTK_EXCH:
		{
			app_env.proxr_device[con_id].ediv = param->data.ltk.ediv;
			memcpy (app_env.proxr_device[con_id].rand_nb, param->data.ltk.randnb.nb, 
					RAND_NB_LEN);
			app_env.proxr_device[con_id].ltk.key_size = param->data.ltk.key_size;
			memcpy (app_env.proxr_device[con_id].ltk.ltk.key, param->data.ltk.ltk.key,
					param->data.ltk.key_size);
			app_env.proxr_device[con_id].ltk.ediv = param->data.ltk.ediv;
			memcpy (app_env.proxr_device[con_id].ltk.randnb.nb, param->data.ltk.randnb.nb,
					RAND_NB_LEN);
		} break;
		case GAPC_PAIRING_FAILED:
		{
			app_env.proxr_device[con_id].bonded = 0;
			app_disconnect(con_id);
		} break;
	}

	return 0;
}


int gapc_bond_req_ind_handler(ke_msg_id_t msgid,
							  struct gapc_bond_req_ind *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int con_id = res_conidx(KE_IDX_GET(src_id));

	switch (param->request)
	{
		case GAPC_CSRK_EXCH:
		{
			app_gap_bond_cfm(con_id);
		} break;
		default:
		{
		} break;
	}

	return 0;
}


int gapc_le_pkt_size_ind_handler(ke_msg_id_t msgid,
								 struct gapc_le_pkt_size_ind *param,
								 ke_task_id_t dest_id,
								 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	DBG_INFO("DLE successfully. LL_PKT_SIZE set to : %d\n", param->max_tx_octets);

	app_start_gatt_mtu_negotiation();
	DBG_INFO(">>> GATTC_EXC_MTU_CMD (GATTC_MTU_EXCH) \n");

	return 0;
}


int gap_reset_req_cmp_evt_handler(ke_msg_id_t msgid,
								  struct gap_reset_req_cmp_evt *param,
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
		app_set_mode_cent(); //initialize gap mode

	}

	return 0;
}


int  proxm_enable_rsp_handler(ke_msg_id_t msgid,
							  struct proxm_enable_rsp *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int con_id;
	con_id = res_conidx(KE_IDX_GET(src_id));

	if (param->status == ATT_ERR_NO_ERROR)
	{
		//initialize proximity reporter's values to non valid
		app_env.proxr_device[con_id].llv = 0xFF;
		app_env.proxr_device[con_id].txp = 0xFF;

		ConsoleClear();
		ConsoleConnected(1);

		if (start_pair)
		{
			app_env.proxr_device[con_id].bonded = 0;
			app_security_enable(con_id);
			DBG_INFO(">>> GAPC_BOND_CMD (ops=GAPC_BOND): start bonding procedure \n");
		}
		else
		{
			app_start_encryption(con_id);
			DBG_INFO(">>> GAPC_ENCRYPT_CMD (ops=GAPC_ENCRYPT): Start encryption procedure \n");
		}

		app_proxm_read_txp(con_id);
		DBG_INFO(">>> PROXM_RD_REQ (svc_code=PROXM_RD_TX_POWER_LVL) \n");
	}
	else
	{
		DBG_INFO("Maybe a non-PROXR peer is connected. PROXM_ENABLE_RSP.status=%d \n",
				 param->status);
	}

	return 0;
}


int  proxm_rd_char_rsp_handler(ke_msg_id_t msgid,
							   struct proxm_rd_rsp *param,
							   ke_task_id_t dest_id,
							   ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	int con_id;
	con_id = res_conidx(KE_IDX_GET(src_id));

	if (param->status == ATT_ERR_NO_ERROR)
	{
		if (last_char[con_id] == PROXM_RD_LL_ALERT_LVL)
		{
			app_env.proxr_device[con_id].llv = param->value;
		}
		else if (last_char[con_id] == PROXM_RD_TX_POWER_LVL)
		{
			app_env.proxr_device[con_id].txp = param->value;
		}

		last_char[con_id] = 0xFF;
		proxm_trans_in_prog = 0;

		ConsoleConnected(1);
	}

	return 0;
}


int  proxm_wr_alert_lvl_rsp_handler(ke_msg_id_t msgid,
									struct proxm_wr_alert_lvl_rsp *param,
									ke_task_id_t dest_id,
									ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->status == ATT_ERR_NO_ERROR)
	{
		proxm_trans_in_prog = 0;
		ConsoleConnected(1);
	}

	return 0;
}


int  disc_enable_rsp_handler(ke_msg_id_t msgid,
							 struct disc_enable_rsp *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t i;
	int con_id = res_conidx(KE_IDX_GET(src_id));

	if (param->status == ATT_ERR_NO_ERROR)
	{

		for ( i = 0; i < DISC_CHAR_MAX; i++)
		{
			app_env.proxr_device[con_id].dis.chars[i].val_hdl = param->dis.chars[i].val_hdl;
		}

		app_disc_rd_char(DISC_MANUFACTURER_NAME_CHAR, con_id);

	}

	return 0;
}


int  disc_rd_char_rsp_handler(ke_msg_id_t msgid,
							  struct disc_rd_char_rsp *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t i;
	uint8_t char_code = DISC_CHAR_MAX;
	static uint8_t char_code_idx = 0;
	uint8_t con_id = res_conidx(KE_IDX_GET(src_id));


	if (param->info.status == ATT_ERR_NO_ERROR)
	{

		for ( i = 0; i < DISC_CHAR_MAX; i++)
		{
			if (app_env.proxr_device[con_id].dis.chars[i].val_hdl == param->info.handle)
			{
				char_code = i;
				break;
			}
		}

		if (char_code < DISC_CHAR_MAX)
		{
			memcpy (app_env.proxr_device[con_id].dis.chars[char_code].val, 
					param->info.value,
					param->info.length);
			app_env.proxr_device[con_id].dis.chars[char_code].val[param->info.length] = '\0';
			app_env.proxr_device[con_id].dis.chars[char_code].len = param->info.length;
		}
	}

	// read next DISS characteristic value
	read_disc_chars(++char_code_idx, con_id);

	if (char_code_idx == DISC_CHAR_MAX)
	{
		char_code_idx = 0;
	}

	return 0;
}


void read_disc_chars(uint8_t id, uint8_t con_id)
{

	switch (id)
	{
		case DISC_MODEL_NB_STR_CHAR:
		{
			app_disc_rd_char(DISC_MODEL_NB_STR_CHAR, con_id);
		} break;
		case DISC_SERIAL_NB_STR_CHAR:
		{
			app_disc_rd_char(DISC_SERIAL_NB_STR_CHAR, con_id);
		} break;
		case DISC_HARD_REV_STR_CHAR:
		{
			app_disc_rd_char(DISC_HARD_REV_STR_CHAR, con_id);
		} break;
		case DISC_FIRM_REV_STR_CHAR:
		{
			app_disc_rd_char(DISC_FIRM_REV_STR_CHAR, con_id);
		}break;
		case DISC_SW_REV_STR_CHAR:
		{
			app_disc_rd_char(DISC_SW_REV_STR_CHAR, con_id);
		} break;
		case DISC_SYSTEM_ID_CHAR:
		{
			app_disc_rd_char(DISC_SYSTEM_ID_CHAR, con_id);
		} break;
		case DISC_IEEE_CHAR:
		{
			app_disc_rd_char(DISC_IEEE_CHAR, con_id);
		} break;
		case DISC_PNP_ID_CHAR:
		{
			app_disc_rd_char(DISC_PNP_ID_CHAR, con_id);
		} break;
		default:
		{
		} break;
	}

	return;
}


int uuid_128_compare(uint8_t *uuid1, uint8_t *uuid2)
{
	int i;
	uint8_t *uuid1_ptr = uuid1;
	uint8_t *uuid2_ptr = uuid2;

	for (i = 0; i < ATT_UUID_128_LEN; i++)
	{
		if (*uuid1_ptr == *uuid2_ptr)
		{
			uuid1_ptr++;
			uuid2_ptr++;
			continue;
		}
		else
		{
			// not matched
			return 1;
		}
	}
	// matched
	return 0;
}

int gattc_disc_char_ind_handler(ke_msg_id_t msgid,
								struct gattc_disc_char_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t slot_idx;

	// characteristics have 128 bit UUIDs
	if (param->uuid_len != 16 )
	{
		return 0;
	}

	if ( 0 == memcmp(param->uuid, IOT_SENSOR_SVC_TEMP_CHAR_UUID, 16) )
	{
		slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

		if (slot_idx == MAX_CONN_NUMBER)
		{
			return 0;
		}


		DBG_INFO("iot sensor svc's temperature value handle saved \n");
		app_env.proxr_device[slot_idx].iot_sensor.temp_char_val_handle = param->pointer_hdl;
	}

	return 0;
}


int gattc_disc_svc_ind_handler(ke_msg_id_t msgid,
							   struct gattc_disc_svc_ind *param,
							   ke_task_id_t dest_id,
							   ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t slot_idx;

	if (param->uuid_len == ATT_UUID_128_LEN)
	{
		if ( 0 == memcmp(param->uuid, IOT_SENSOR_SVC_UUID, 16) )
		{
			slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

			if (slot_idx == MAX_CONN_NUMBER)
			{
				return 0;
			}

			DBG_INFO(" IoT Sensor SVC found ! \n");
			app_env.proxr_device[slot_idx].iot_sensor.svc_found = 1;
			app_env.proxr_device[slot_idx].iot_sensor.svc_start_handle = param->start_hdl;
			app_env.proxr_device[slot_idx].iot_sensor.svc_end_handle = param->end_hdl;
		}
	}

	return 0;
}

int gattc_cmp_evt_handler(ke_msg_id_t msgid,
						  struct gattc_cmp_evt *param,
						  ke_task_id_t dest_id,
						  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

	if (slot_idx == MAX_CONN_NUMBER)
	{
		return 0;
	}

	switch (param->operation)
	{
		case GATTC_DISC_BY_UUID_SVC:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_DISC_BY_UUID_SVC) \n");

			if (param->status != ATT_ERR_NO_ERROR && 
				param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND)
			{
				// Place something if need,
			}
			else
			{
				if (app_env.proxr_device[slot_idx].iot_sensor.svc_found != 1)
				{
					DBG_ERR("Peer device does not support the iot sensor service \n");
				}
				else
				{
					DBG_INFO(">>> GATTC_DISC_ALL_CHAR (discover all chars) ! \n");
					// iot sensor service was discovered. Now discover characteristics.
					app_discover_all_char_uuid_128( KE_IDX_GET(src_id),
							app_env.proxr_device[slot_idx].iot_sensor.svc_start_handle,
							app_env.proxr_device[slot_idx].iot_sensor.svc_end_handle );
				}
			};
		} break;
		case GATTC_DISC_ALL_CHAR:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_DISC_ALL_CHAR) \n");

			if (param->status != ATT_ERR_NO_ERROR 
				&& param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND)
			{
				DBG_ERR("Error while trying to discover IoT Sensor SVC characteristics (status = 0x%02X) \n",
						param->status);
			}
			else if (app_env.proxr_device[slot_idx].iot_sensor.temp_char_val_handle == 0)
			{
				DBG_ERR("Error: Missing characteristics in peer device's IoT Sensor SVC.\n");
			}
			else
			{
				// trigger next step - discover Client char descriptor
				DBG_INFO(">>> GATTC_DISC_CMD (GATTC_DISC_DESC_CHAR) \n");
				app_discover_char_desc_uuid_128(KE_IDX_GET(src_id),
							app_env.proxr_device[slot_idx].iot_sensor.svc_start_handle,
							app_env.proxr_device[slot_idx].iot_sensor.svc_end_handle );
			}

		} break;
		case GATTC_DISC_DESC_CHAR:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_DISC_DESC_CHAR) \n");

			if (param->status != ATT_ERR_NO_ERROR && 
				param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND )
			{
				DBG_ERR("Error while trying to discover IoT Sensor SVC characteristic descriptors (status = 0x%02X) \n",
						param->status);
			}
			else if ( app_env.proxr_device[slot_idx].iot_sensor.temp_cccd_hdl == 0 )
			{
				DBG_ERR("Error: Missing Client Char. Conf. descriptor for IoT Sensor SVC characteristic.\n");
			}
			else
			{
				// trigger next step - shows service/characteristics discovery result
				DBG_INFO("IoT Sensor SVC & characteristics discovery done !! \n");
				ConsoleConnected(1);
			}

		} break;
		case GATTC_WRITE:
		case GATTC_WRITE_NO_RESPONSE:
		{
			DBG_INFO("\n<<< GATTC_CMP_EVT (GATTC_WRITE / GATTC_WRITE_NO_RESPONSE) \n");

			if (param->status != ATT_ERR_NO_ERROR) // write characteristic command failed
			{
				if ((param->status == ATT_ERR_INSUFF_AUTHEN || 
					 param->status == ATT_ERR_INSUFF_ENC)
						&& (start_pair == 1))
				{
					DBG_ERR("warning: failed to write a char (status=0x%02X) due to security not properly established yet, pairing ... \n",
							param->status);
					app_security_enable(app_env.proxr_device[slot_idx].device.conidx);
					start_pair = 0;
				}
				else
				{
					DBG_ERR("error: failed to write a characteristic (status=0x%02X). Aborting connection... \n",
							param->status);
					// disconnect
					app_env.state = APP_IDLE;
					app_disconnect(app_env.proxr_device[slot_idx].device.conidx);
				}
			}
			else if (app_env.proxr_device[slot_idx].iot_sensor.iot_sensor_act_sta == IOT_SENSOR_WR_EN_NOTI)
			{
				app_env.proxr_device[slot_idx].iot_sensor.temp_post_enabled = 1;
				app_env.proxr_device[slot_idx].iot_sensor.iot_sensor_act_sta = IOT_SENSOR_UNDEF;

			}
			else if (app_env.proxr_device[slot_idx].iot_sensor.iot_sensor_act_sta ==
					 IOT_SENSOR_WR_DIS_NOTI)
			{
				app_env.proxr_device[slot_idx].iot_sensor.temp_post_enabled = 0;
				app_env.proxr_device[slot_idx].iot_sensor.iot_sensor_act_sta = IOT_SENSOR_UNDEF;
			}

		} break;
		case GATTC_READ:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_READ) \n");

			if (param->status != ATT_ERR_NO_ERROR) // read characteristic command failed
			{
				if ((param->status == ATT_ERR_INSUFF_AUTHEN || 
					 param->status == ATT_ERR_INSUFF_ENC)
						&& (start_pair == 1))
				{
					DBG_ERR("warning: failed to read a characteristic. (status=0x%02X). due to security not properly established yet, pairing ... \n",
							param->status);
					app_security_enable(app_env.proxr_device[slot_idx].device.conidx);
					start_pair = 0;
				}
				else
				{
					DBG_ERR("error: failed to read a characteristic... \n");
				}
			}
			else
			{
				// update temp with a value read
				DBG_ERR("read characteristic success !! ... \n");
			}

		} break;
		case GATTC_MTU_EXCH:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_MTU_EXCH) \n");
		} break;
		case GATTC_SVC_CHANGED:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_SVC_CHANGED) \n");
		} break;
		default:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (ops=%d) \n", param->operation);
		} break;
	}

	return 0;
}


int gattc_disc_char_desc_ind_handler(ke_msg_id_t msgid,
									 struct gattc_disc_char_desc_ind *param,
									 ke_task_id_t dest_id,
									 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint16_t uuid = 0;
	uint8_t slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

	if (slot_idx == MAX_CONN_NUMBER)
	{
		return 0;
	}

	if (param->uuid_len == 2)
	{
		uuid = (param->uuid[1] << 8) | param->uuid[0];

		switch (uuid)
		{
			case CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR_UUID:
			{
				DBG_INFO("CCCD (uuid=0x%x) handle is saved \n", uuid);
				app_env.proxr_device[slot_idx].iot_sensor.temp_cccd_hdl = param->attr_hdl;
			} break;

			default:
			{
				DBG_INFO("characteristic descriptor (uuid=0x%x) is indicated, \n", uuid);
			} break;
		}
	}

	return 0;
};

int gattc_read_ind_handler(ke_msg_id_t msgid,
						   struct gattc_read_ind *param,
						   ke_task_id_t dest_id,
						   ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t slot_idx;

	slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

	if (slot_idx == MAX_CONN_NUMBER)
	{
		return 0;
	}

	if (param->handle == app_env.proxr_device[slot_idx].iot_sensor.temp_char_val_handle)
	{
		DBG_INFO("temp value read and set done \n");
		memcpy(app_env.proxr_device[slot_idx].iot_sensor.temp_val, param->value, 
				param->length);
		ConsoleConnected(1);
	}

	return 0;
}

int noti_cnt[MAX_CONN_NUMBER] = {0,};
extern void enqueue_sensor_gw_udp_client(uint8_t param, uint8_t slot);
int gattc_event_ind_handler(ke_msg_id_t msgid,
							struct gattc_event_ind *param,
							ke_task_id_t dest_id,
							ke_task_id_t src_id)

{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);

	uint8_t slot_idx;
	uint8_t temperature;

	slot_idx = app_get_slot_id_from_conidx(KE_IDX_GET(src_id));

	if (slot_idx == MAX_CONN_NUMBER)
	{
		return 0;
	}

	if (param->handle != app_env.proxr_device[slot_idx].iot_sensor.temp_char_val_handle)
	{
		return 0;
	}

	// retrieve notification value
	memcpy(app_env.proxr_device[slot_idx].iot_sensor.temp_val, param->value, param->length);
	temperature = param->value[0];
	noti_cnt[slot_idx]++;

	/* update console user */
	ConsoleConnected(1); 

	// send the current temperature to server every (notification_interval x 10) second
	if ( (noti_cnt[slot_idx] != 0) &&  !(noti_cnt[slot_idx] % 10) )
	{
		DBG_INFO("iot_sensor[%d]: the current temperature %d is going to be sent to server ! \n",
				 slot_idx + 1, 
				 temperature);
		enqueue_sensor_gw_udp_client(temperature, slot_idx+1);
	}

	return 0;
}

#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */
