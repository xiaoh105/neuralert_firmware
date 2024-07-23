/**
 ****************************************************************************************
 *
 * @file wifi_svc_ble_msg.c
 *
 * @brief GTL BLE message handler
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

#if defined (__BLE_PERI_WIFI_SVC_PERIPHERAL__)

#include "compiler.h"
#include "../../include/da14585_config.h"

#include "gapc_task.h"
#include "gapm_task.h"
#include "gattc_task.h"
#include "gattm_task.h"
#include "proxr_task.h"
#include "diss_task.h"
#include "rwble_hl_config.h"

#include "da16_gtl_features.h"
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif

#include "../../include/gatt_handlers.h"
#include "../../include/ble_msg.h"
#include "../../include/queue.h"
#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif
#include "../../include/app.h"
#include "../../include/app_task.h"

#include "application.h"

#if defined (__BLE_PROVISIONING_SAMPLE__)
#include "app_provision.h"
#endif

#if defined (__GTL_IFC_UART__)
#include "../../include/uart.h"
#endif
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"
#include "../../user_util/user_util.h"

extern int gapc_param_updated_ind_handler(ke_msg_id_t const msgid,
		struct gapc_param_updated_ind *param,
		ke_task_id_t const dest_id,
		ke_task_id_t const src_id);

extern int gattc_mtu_changed_ind_handler (ke_msg_id_t msgid,
		struct gattc_mtu_changed_ind *param,
		ke_task_id_t dest_id,
		ke_task_id_t src_id);

extern int gapm_addr_solved_ind_handler(ke_msg_id_t const msgid,
										struct gapm_addr_solved_ind *param,
										ke_task_id_t const dest_id,
										ke_task_id_t const src_id);

extern void handle_cmp_evt_for_gapm_resolv_addr_cmd(uint8_t error);

/* Internal Functions */

int HandleGapmCmpEvt(ke_msg_id_t msgid, struct gapm_cmp_evt *param, ke_task_id_t dest_id,
					 ke_task_id_t src_id);
int HandleGapcCmpEvt(ke_msg_id_t msgid, struct gapc_cmp_evt *param, ke_task_id_t dest_id,
					 ke_task_id_t src_id);
int HandleGattcCmpEvt(ke_msg_id_t msgid, struct gattc_cmp_evt *param,
					  ke_task_id_t dest_id, ke_task_id_t src_id);

void BleSendMsg(void *message)
{
	ble_msg *blemsg = (ble_msg *)((unsigned char *)message - sizeof (ble_hdr));

	if (blemsg) {
#if defined(__GTL_IFC_UART__)
		UARTSend(blemsg->bLength + sizeof(ble_hdr), (unsigned char *) blemsg);
		MEMFREE(blemsg);
#endif
	}
}

void *BleMsgAlloc(unsigned short id, unsigned short dest_id,
				  unsigned short src_id, unsigned short param_len)
{
	ble_msg *blemsg = (ble_msg *)MEMALLOC(sizeof(ble_msg) + param_len - sizeof (unsigned char));

	if (blemsg == NULL)
	{
		DBG_ERR("malloc error in BleMsg(id=%d)\n", id);
		return NULL;
	}

	blemsg->bType    = id;
	blemsg->bDstid   = dest_id;
	blemsg->bSrcid   = src_id;
	blemsg->bLength  = param_len;

	if (param_len)
	{
		memset(blemsg->bData, 0, param_len);
	}

	return blemsg->bData;
}

void *BleMsgDynAlloc(unsigned short id, unsigned short dest_id,
					 unsigned short src_id, unsigned short param_len, 
					 unsigned short length)
{
	return (BleMsgAlloc(id, dest_id, src_id, (param_len + length)));
}

void BleFreeMsg(void *message)
{
	ble_msg *blemsg = (ble_msg *)((unsigned char *)message - sizeof (ble_hdr));

	APP_FREE(blemsg);
}

void HandleBleMsg(ble_msg *blemsg)
{
	if (blemsg->bDstid != TASK_ID_GTL)
	{
		DBG_ERR("[%s] Destination task should be TASK_ID_GTL (16)", __func__);
		return;
	}

	switch (blemsg->bType)
	{

		/* GAPM events */

		// Command Complete event
		case GAPM_CMP_EVT:
		{
			HandleGapmCmpEvt(blemsg->bType, (struct gapm_cmp_evt *)blemsg->bData, 
							blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Event triggered to inform that lower layers are ready
		case GAPM_DEVICE_READY_IND:
		{
			DBG_INFO("<<< GAPM_DEVICE_READY_IND \n");
			gapm_device_ready_ind_handler(blemsg->bType, 
										 (struct gap_ready_evt *)blemsg->bData,
										 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Local device version indication event
		case GAPM_DEV_VERSION_IND:
			DBG_INFO("<<< GAPM_DEV_VERSION_IND \n");
			break;

		// Local device BD Address indication event
		case GAPM_DEV_BDADDR_IND:
		{
			DBG_INFO("<<< GAPM_DEV_BDADDR_IND \n");
			int i;
			struct gap_bdaddr dev_addr;
			memcpy(&dev_addr, 
				  &(((struct gapm_dev_bdaddr_ind *)(blemsg->bData))->addr), 
				  sizeof(struct gap_bdaddr));

			DBG_INFO("addr_type = %d\n", dev_addr.addr_type);
			
			DBG_INFO("addr = ");
			for (i = 0; i < BD_ADDR_LEN; i++)
			{
				PRINTF ("%02x", dev_addr.addr.addr[BD_ADDR_LEN - 1 - i]);
				if (i < BD_ADDR_LEN - 1)
					PRINTF (":", dev_addr.addr.addr[BD_ADDR_LEN - 1 - i]);
			}
			DBG_INFO("\n");
			
		} break;

		// Advertising channel Tx power level
		case GAPM_DEV_ADV_TX_POWER_IND:
			break;

		// Indication containing information about memory usage
		case GAPM_DBG_MEM_INFO_IND:
			break;

		// White List Size indication event
		case GAPM_WHITE_LIST_SIZE_IND:
			break;

		// Advertising or scanning report information event
		case GAPM_ADV_REPORT_IND:
		{
			gapm_adv_report_ind_handler(blemsg->bType, 
										(struct gapm_adv_report_ind *) blemsg->bData,
										blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Name of peer device indication
		case GAPM_PEER_NAME_IND:
			break;

		// Indicate that Resolvable Private Address has been solved
		case GAPM_ADDR_SOLVED_IND:
		{
			DBG_INFO("<<< GAPM_ADDR_SOLVED_IND \n");
			gapm_addr_solved_ind_handler(blemsg->bType, 
										(struct gapm_addr_solved_ind *)&blemsg->bData,
										 blemsg->bDstid, blemsg->bSrcid);
		} break;

		//  AES-128 block result indication
		case GAPM_USE_ENC_BLOCK_IND:
			break;

		// Random Number Indication
		case GAPM_GEN_RAND_NB_IND:
			break;

		// Inform that profile task has been added.
		case GAPM_PROFILE_ADDED_IND:
		{
			gapm_profile_added_ind_handler(blemsg->bType,
										   (struct gapm_profile_added_ind *)blemsg->bData, 
										   blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that a message has been received on an unknown task
		case GAPM_UNKNOWN_TASK_IND:
			break;

		// Suggested Default Data Length indication
		case GAPM_SUGG_DFLT_DATA_LEN_IND:
			break;

		// Maximum Data Length indication
		case GAPM_MAX_DATA_LEN_IND:
			break;

		// Resolving address list address indication
		case GAPM_RAL_SIZE_IND:
			break;

		// Resolving address list address indication
		case GAPM_RAL_ADDR_IND:
			break;

		// Limited discoverable timeout indication
		case GAPM_LIM_DISC_TO_IND:
			break;

		// Scan timeout indication
		case GAPM_SCAN_TO_IND:
			break;

		// Address renewal timeout indication
		case GAPM_ADDR_RENEW_TO_IND:
			break;

		// DHKEY P256 block result indication
		case GAPM_USE_P256_BLOCK_IND:
			break;

		/* GAPC events */

		// Command Complete event
		case GAPC_CMP_EVT:
		{
			HandleGapcCmpEvt(blemsg->bType, (struct gapc_cmp_evt *)blemsg->bData, 
							 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that a connection has been established
		case GAPC_CONNECTION_REQ_IND:
		{
			DBG_INFO(YELLOW_COLOR"<<< GAPC_CONNECTION_REQ_IND \n"CLEAR_COLOR);
			gapc_connection_req_ind_handler(blemsg->bType,
										(struct gapc_connection_req_ind *) blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);
#if defined (__BLE_PROVISIONING_SAMPLE__)
            app_check_provision(1);
#endif
		} break;

		// Indicate that a link has been disconnected
		case GAPC_DISCONNECT_IND:
		{
			DBG_INFO(YELLOW_COLOR"<<< GAPC_DISCONNECT_IND \n"CLEAR_COLOR);
			gapc_disconnect_ind_handler(blemsg->bType,  
										(struct gapc_disconnect_ind *) blemsg->bData,
										blemsg->bDstid, blemsg->bSrcid);
#if defined (__BLE_PROVISIONING_SAMPLE__)
            app_check_provision(0);
#endif
		} break;

		// Peer device attribute DB info such as Device Name, 
		// Appearance or Slave Preferred Parameters
		case GAPC_PEER_ATT_INFO_IND:
			break;

		// Indication of peer version info
		case GAPC_PEER_VERSION_IND:
			break;

		// Indication of peer features info
		case GAPC_PEER_FEATURES_IND:
			break;

		// Indication of ongoing connection RSSI
		case GAPC_CON_RSSI_IND:
		{
			DBG_INFO("<<< GAPC_CON_RSSI_IND\n");
			gapc_con_rssi_ind_handler(blemsg->bType, 
									 (struct gapc_con_rssi_ind *) blemsg->bData,
									  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Device info request indication
		case GAPC_GET_DEV_INFO_REQ_IND:
		{
			gapc_get_dev_info_req_ind_handler(blemsg->bType,
									   (struct gapc_get_dev_info_req_ind *) blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Peer device request to modify local device info such as name or appearance
		case GAPC_SET_DEV_INFO_REQ_IND:
		{
			gapc_set_dev_info_req_ind_handler(blemsg->bType,
									   (struct gapc_set_dev_info_req_ind *) blemsg->bData, 
										   blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Request of updating connection parameters indication
		case GAPC_PARAM_UPDATE_REQ_IND:
		{
			DBG_INFO("<<< GAPC_PARAM_UPDATE_REQ_IND\n");
			gapc_param_update_req_ind_handler(blemsg->bType,
									   (struct gapc_param_update_req_ind *) blemsg->bData, 
											  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Connection parameters updated indication
		case GAPC_PARAM_UPDATED_IND:
		{
			DBG_INFO("<<< GAPC_PARAM_UPDATED_IND \n");
			gapc_param_updated_ind_handler(blemsg->bType,
										   (struct gapc_param_updated_ind *)blemsg->bData, 
										   blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Bonding requested by peer device indication message.
		case GAPC_BOND_REQ_IND:
		{
			gapc_bond_req_ind_handler(blemsg->bType, 
									 (struct gapc_bond_req_ind *) blemsg->bData,
									  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Bonding information indication message
		case GAPC_BOND_IND:
		{
			gapc_bond_ind_handler(blemsg->bType,  (struct gapc_bond_ind *) blemsg->bData,
								  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Encryption requested by peer device indication message.
		case GAPC_ENCRYPT_REQ_IND:
		{
			DBG_INFO("<<< GAPC_ENCRYPT_REQ_IND \n");
			gapc_encrypt_req_ind_handler(blemsg->bType, 
										(struct gapc_encrypt_req_ind *) blemsg->bData,
										 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Encryption information indication message
		case GAPC_ENCRYPT_IND:
		{
			DBG_INFO("<<< GAPC_ENCRYPT_IND \n");
			gapc_encrypt_ind_handler(blemsg->bType, 
									(struct gapc_encrypt_ind *)blemsg->bData,
									 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Security requested by peer device indication message
		case GAPC_SECURITY_IND:
			break;

		// Indicate the current sign counters to the application
		case GAPC_SIGN_COUNTER_IND:
			break;

		// Indication of ongoing connection Channel Map
		case GAPC_CON_CHANNEL_MAP_IND:
			break;

		// LE credit based connection request indication
		case GAPC_LECB_CONNECT_REQ_IND:
			break;

		// LE credit based connection indication
		case GAPC_LECB_CONNECT_IND:
			break;

		// LE credit based credit addition indication
		case GAPC_LECB_ADD_IND:
			break;

		// disconnect indication
		case GAPC_LECB_DISCONNECT_IND:
			break;

		// LE Ping timeout indication
		case GAPC_LE_PING_TO_VAL_IND:
			break;

		// LE Ping timeout expires indication
		case GAPC_LE_PING_TO_IND:
			break;

		//Indication of LE Data Length
		case GAPC_LE_PKT_SIZE_IND:
		{
			DBG_INFO("<<< GAPC_LE_PKT_SIZE_IND \n");
			gapc_le_pkt_size_ind_handler(blemsg->bType, 
										(struct gapc_le_pkt_size_ind *)blemsg->bData,
										 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Signature result
		case GAPC_SIGN_IND:
			break;

		// Parameter update procedure timeout indication
		case GAPC_PARAM_UPDATE_TO_IND:
			break;

		// Pairing procedure timeout indication
		case GAPC_SMP_TIMEOUT_TIMER_IND:
			break;

		// Pairing repeated attempts procedure timeout indication
		case GAPC_SMP_REP_ATTEMPTS_TIMER_IND:
			break;

		// Connection procedure timeout indication
		case GAPC_LECB_CONN_TO_IND:
			break;

		// Disconnection procedure timeout indication
		case GAPC_LECB_DISCONN_TO_IND:
			break;

		// Peer device sent a keypress notification
		case GAPC_KEYPRESS_NOTIFICATION_IND:
			break;

		/* GATTM events */

		// Add service in database response
		case GATTM_ADD_SVC_RSP:
		{
			gattm_add_svc_rsp_hnd(blemsg->bType, 
								 (struct gattm_add_svc_rsp const *)&blemsg->bData,
								  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Get permission settings of service response
		case GATTM_SVC_GET_PERMISSION_RSP:
			break;

		// Set permission settings of service response
		case GATTM_SVC_SET_PERMISSION_RSP:
		{
			gattm_svc_set_permission_rsp_hnd(blemsg->bType,
							   (struct gattm_svc_set_permission_rsp const *)blemsg->bData, 
								blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Get permission settings of attribute response
		case GATTM_ATT_GET_PERMISSION_RSP:
			break;

		// Set permission settings of attribute response
		case GATTM_ATT_SET_PERMISSION_RSP:
			break;

		// Get attribute value response
		case GATTM_ATT_GET_VALUE_RSP:
		{
			DBG_INFO("<<< GATTM_ATT_GET_VALUE_RSP \n");
		} break;

		// Set attribute value response
		case GATTM_ATT_SET_VALUE_RSP:
		{
			gattm_att_set_value_rsp_hnd(blemsg->bType,
									(struct gattm_att_set_value_rsp const *)blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
		} break;

		// DEBUG ONLY: Destroy Attribute database response
		case GATTM_DESTROY_DB_RSP:
			break;

		/// DEBUG ONLY: Retrieve list of services response
		case GATTM_SVC_GET_LIST_RSP:
			break;

		/// DEBUG ONLY: Retrieve information of attribute response
		case GATTM_ATT_GET_INFO_RSP:
			break;

		/* GATTC events */

		// Command Complete event
		case GATTC_CMP_EVT:
		{
			HandleGattcCmpEvt(blemsg->bType, (struct gattc_cmp_evt *)blemsg->bData, 
							  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that the ATT MTU has been updated (negotiated)
		case GATTC_MTU_CHANGED_IND:
		{
			DBG_INFO("<<< GATTC_MTU_CHANGED_IND \n");
			gattc_mtu_changed_ind_handler(blemsg->bType,
										  (struct gattc_mtu_changed_ind *)blemsg->bData, 
										  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that Service Changed indications has been enabled or disabled
		case GATTC_SVC_CHANGED_CFG_IND:
		    break;

		// Discovery services indication
		case GATTC_DISC_SVC_IND:
			break;

		// Discover included services indication
		case GATTC_DISC_SVC_INCL_IND:
			break;

		// Discover characteristic indication
		case GATTC_DISC_CHAR_IND:
			break;

		// Discovery characteristic descriptor indication
		case GATTC_DISC_CHAR_DESC_IND:
			break;

		// Read response
		case GATTC_READ_IND:
			break;

		// peer device triggers an event (notification)
		case GATTC_EVENT_IND:
			break;

		// peer device triggers an event that requires a confirmation (indication)
		case GATTC_EVENT_REQ_IND:
			break;

		// Read command indicated to upper layers.
		case GATTC_READ_REQ_IND:
		{
			gattc_read_req_ind_hnd(blemsg->bType, 
								  (struct gattc_read_req_ind const *)blemsg->bData,
								   blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Write command indicated to upper layers.
		case GATTC_WRITE_REQ_IND:
		{
			gattc_write_req_ind_hnd(blemsg->bType, 
									(struct gattc_write_req_ind const *)blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid, 0);
		} break;

		// Request Attribute info to upper layer - could be trigger during prepare write
		case GATTC_ATT_INFO_REQ_IND:
		{
			gattc_att_info_req_ind_hnd(blemsg->bType,
								     (struct gattc_att_info_req_ind const *)blemsg->bData, 
									   blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Service Discovery indicate that a service has been found.
		case GATTC_SDP_SVC_IND:
			break;

		// Transaction Timeout Error Event no more transaction will be accepted
		case GATTC_TRANSACTION_TO_ERROR_IND:
			break;

		// Client Response timeout indication
		case GATTC_CLIENT_RTX_IND:
			break;

		// Server indication confirmation timeout indication
		case GATTC_SERVER_RTX_IND:
			break;

		/* PROXR events */

		case PROXR_ALERT_IND:
		{
			proxr_alert_ind_handler(blemsg->bDstid, (struct proxr_alert_ind *) blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid);
		} break;

		/* DISS events */

		case DISS_VALUE_REQ_IND:
		{
			diss_value_req_ind_handler(blemsg->bDstid, 
									  (struct diss_value_req_ind *) blemsg->bData,
									   blemsg->bDstid, blemsg->bSrcid);
		} break;

		/* Custom message events */

		case APP_FW_VERSION_IND:
		{
			DBG_INFO("\n<<< APP_FW_VERSION_IND \n");
			app_fw_version_ind_handler(blemsg->bType, (app_fw_version_ind_t *)blemsg->bData, 
									blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_BLINKY_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_BLINKY_START_IND \n");
			app_peri_blinky_start_ind_handler(blemsg->bType, 
											(app_peri_blinky_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_SYSTICK_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_SYSTICK_START_IND \n");
			app_peri_systick_start_ind_handler(blemsg->bType, 
											(app_peri_systick_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_SYSTICK_STOP_IND:
		{
			DBG_INFO("\n<<< APP_PERI_SYSTICK_STOP_IND \n");
			app_peri_systick_stop_ind_handler(blemsg->bType, 
											(app_peri_systick_stop_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;
		

		case APP_PERI_TIMER0_GEN_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_TIMER0_GEN_START_IND \n");
			app_peri_timer0_gen_start_ind_handler(blemsg->bType, 
											(app_peri_timer0_gen_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;


		case APP_PERI_TIMER0_BUZ_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_TIMER0_BUZ_START_IND \n");
			app_peri_timer0_buz_start_ind_handler(blemsg->bType, 
											(app_peri_timer0_buz_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_TIMER2_PWM_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_TIMER2_PWM_START_IND \n");
			app_peri_timer2_pwm_start_ind_handler(blemsg->bType, 
											(app_peri_timer2_pwm_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_BATT_LVL_IND:
		{
			DBG_INFO("\n<<< APP_PERI_BATT_LVL_IND \n");
			app_peri_batt_lvl_ind_handler(blemsg->bType, 
										(app_peri_batt_lvl_ind_t *)blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_I2C_EEPROM_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_I2C_EEPROM_START_IND \n");
			app_peri_i2c_eeprom_start_ind_handler(blemsg->bType, 
											(app_peri_i2c_eeprom_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;


		case APP_PERI_SPI_FLASH_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_SPI_FLASH_START_IND \n");
			app_peri_spi_flash_start_ind_handler(blemsg->bType, 
											(app_peri_spi_flash_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_QUAD_DEC_START_IND:
		{
			DBG_INFO("\n<<< APP_PERI_QUAD_DEC_START_IND \n");
			app_peri_quad_dec_start_ind_handler(blemsg->bType, 
											(app_peri_quad_dec_start_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

		case APP_PERI_QUAD_DEC_STOP_IND:
		{
			DBG_INFO("\n<<< APP_PERI_QUAD_DEC_STOP_IND \n");
			app_peri_quad_dec_stop_ind_handler(blemsg->bType, 
											(app_peri_quad_dec_stop_ind_t *)blemsg->bData, 
											blemsg->bDstid, blemsg->bSrcid);						 
		} break;

#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
		case APP_PERI_GPIO_SET_IND:
		case APP_PERI_GPIO_GET_IND:
		{
			if (blemsg->bType == APP_PERI_GPIO_SET_IND)
				DBG_INFO("\n<<< APP_PERI_GPIO_SET_IND\n");
			else
				DBG_INFO("\n<<< APP_PERI_GPIO_GET_IND\n");

			app_peri_gpio_cmd_ind_handler(blemsg->bType,
					(app_peri_gpio_cmd_ind_t *)blemsg->bData, 
					blemsg->bDstid, blemsg->bSrcid);
		}; break;
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */

		/* Unknown event */

		default:
		{
			DBG_ERR("Rcved UNKNOWN Msg 0x%x\n", blemsg->bType);
		} break;

	}
}

/*
 ****************************************************************************************
 * @brief Receives ble message from GTL host iface.
 *
 * @return void.
 ****************************************************************************************
*/
void BleReceiveMsg(void)
{
#if defined(__GTL_IFC_UART__)
#ifdef __FREERTOS__
	msg queue_msg;
	if (msg_queue_get(&GtlRxQueue, &queue_msg, portMAX_DELAY) == OS_QUEUE_EMPTY) {
		configASSERT(0);
	} else {
		ble_msg *message = (ble_msg *)queue_msg.data;
		if (message) {
			HandleBleMsg(message);
		}
		msg_release(&queue_msg);
	}
#else
	ble_msg *message;

	if ( gtl_rx_q_mutex == NULL ) {
		return;
	}

	xSemaphoreTake(gtl_rx_q_mutex, portMAX_DELAY);
	if (GtlRxQueue.First != NULL)
	{

		message = (ble_msg *) DeQueue(&GtlRxQueue);

		HandleBleMsg(message);
		MEMFREE(message);
	}
	xSemaphoreGive(gtl_rx_q_mutex);
#endif /* __FREERTOS__ */
#endif /* __GTL_IFC_UART__ */
}

int HandleGapmCmpEvt(ke_msg_id_t msgid,
					 struct gapm_cmp_evt *param,
					 ke_task_id_t dest_id,
					 ke_task_id_t src_id)
{
	if (param->status == GAP_ERR_NO_ERROR)
	{
		switch (param->operation)
		{
			case GAPM_NO_OP:// No operation.
				break;

			case GAPM_RESET:// Reset BLE subsystem: LL and HL.
			{
				gapm_reset_completion_handler (msgid, 
											  (struct gapm_cmp_evt *)param, dest_id, src_id);
			} break;

			case GAPM_CANCEL:// Cancel currently executed operation.
				break;

			case GAPM_SET_DEV_CONFIG:// Set device configuration
			{
				gapm_set_dev_config_completion_handler(msgid, 
													  (struct gapm_cmp_evt *)param, dest_id,
													   src_id);
			} break;

			case GAPM_SET_CHANNEL_MAP:
				break;

			case GAPM_GET_DEV_VERSION:
				DBG_INFO("<<< GAPM_CMP_EVT (GAPM_GET_DEV_VERSION) \n");
				break;

			case GAPM_GET_DEV_BDADDR:
				DBG_INFO("<<< GAPM_CMP_EVT (GAPM_GET_DEV_BDADDR) \n");
				break;

			case GAPM_GET_DEV_ADV_TX_POWER:
				break;

			case GAPM_GET_WLIST_SIZE:
				break;

			case GAPM_ADD_DEV_IN_WLIST:
				break;

			case GAPM_RMV_DEV_FRM_WLIST:
				break;

			case GAPM_CLEAR_WLIST:
				break;

			case GAPM_ADV_NON_CONN:
				break;

			case GAPM_ADV_UNDIRECT:
			{
				DBG_INFO("<<< GAPM_CMP_EVT (GAPM_ADV_UNDIRECT)\n");
				app_set_ble_adv_status(pdFALSE);
			} break;

			case GAPM_ADV_DIRECT:
				break;

			case GAPM_ADV_DIRECT_LDC:
				break;

			case GAPM_UPDATE_ADVERTISE_DATA:
				DBG_INFO("<<< GAPM_CMP_EVT (GAPM_UPDATE_ADVERTISE_DATA)\n");
				break;

			case GAPM_SCAN_ACTIVE:
				break;

			case GAPM_SCAN_PASSIVE:
				break;

			case GAPM_CONNECTION_DIRECT:
				break;

			case GAPM_CONNECTION_AUTO:
				break;

			case GAPM_CONNECTION_SELECTIVE:
				break;

			case GAPM_CONNECTION_NAME_REQUEST:
				break;

			case GAPM_RESOLV_ADDR:
			{
				DBG_INFO("<<< GATTM_CMP_EVT (GAPM_RESOLV_ADDR) \n");
				handle_cmp_evt_for_gapm_resolv_addr_cmd(param->status != GAP_ERR_NO_ERROR);
			} break;

			case GAPM_GEN_RAND_ADDR:
				break;

			case GAPM_USE_ENC_BLOCK:
				break;

			case GAPM_GEN_RAND_NB:
				break;

			case GAPM_PROFILE_TASK_ADD:
				break;

			case GAPM_DBG_GET_MEM_INFO:
				break;

			case GAPM_PLF_RESET:
				break;

			case GAPM_SET_SUGGESTED_DFLT_LE_DATA_LEN:
				break;

			case GAPM_GET_SUGGESTED_DFLT_LE_DATA_LEN:
				break;

			case GAPM_GET_MAX_LE_DATA_LEN:
				break;

			case GAPM_GET_RAL_SIZE:
				break;

			case GAPM_GET_RAL_LOC_ADDR:
				break;

			case GAPM_GET_RAL_PEER_ADDR:
				break;

			case GAPM_ADD_DEV_IN_RAL:
				break;

			case GAPM_RMV_DEV_FRM_RAL:
				break;

			case GAPM_CLEAR_RAL:
				break;

			case GAPM_USE_P256_BLOCK:
				break;

			case GAPM_NETWORK_MODE_RAL:
				break;

			case GAPM_DEVICE_MODE_RAL:
				break;

			case GAPM_LAST:
				break;
			default:
				break;
		}
	} else if (param->status == GAP_ERR_CANCELED) {
		if (param->operation == GAPM_ADV_UNDIRECT) {
			app_set_ble_adv_status(pdFALSE);
			app_set_ble_evt(EVT_BLE_ADV_CANCELLED);
		}
	}

	return (KE_MSG_CONSUMED);
}

int HandleGapcCmpEvt(ke_msg_id_t msgid,
					 struct gapc_cmp_evt *param,
					 ke_task_id_t dest_id,
					 ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	switch (param->operation)
	{
		case GAPC_NO_OP:
			break;

		case GAPC_DISCONNECT:
		{
			if (app_is_no_adv_forced()) {
				app_set_ble_evt(EVT_BLE_DISCONNECTED);
			}
		} break;

		case GAPC_GET_PEER_NAME:
			break;

		case GAPC_GET_PEER_VERSION:
			break;

		case GAPC_GET_PEER_FEATURES:
			break;

		case GAPC_GET_PEER_APPEARANCE:
			break;

		case GAPC_GET_PEER_SLV_PREF_PARAMS:
			break;

		case GAPC_GET_CON_RSSI:
			break;

		case GAPC_GET_CON_CHANNEL_MAP:
			break;

		case GAPC_UPDATE_PARAMS:
		{	
			if (param->status)
			{ 
				/*
					for error code, please refer to rwble_hl_error.h > enum hl_err				
				*/
				DBG_ERR("<<< GAPC_CMP_EVT (GAPC_UPDATE_PARAMS): not successful, error code = %d\n", param->status);				
			}
			else
			{ 
				DBG_INFO("<<< GAPC_CMP_EVT (GAPC_UPDATE_PARAMS): successful! \n");
			}
		} break;
		case GAPC_BOND:
			break;

		case GAPC_ENCRYPT:
			break;

		case GAPC_SECURITY_REQ:
			break;

		case GAPC_LE_CB_CREATE:
			break;

		case GAPC_LE_CB_DESTROY:
			break;

		case GAPC_LE_CB_CONNECTION:
			break;

		case GAPC_LE_CB_DISCONNECTION:
			break;

		case GAPC_LE_CB_ADDITION:
			break;

		case GAPC_GET_LE_PING_TO:
			break;

		case GAPC_SET_LE_PING_TO:
			break;

		case GAPC_SET_LE_PKT_SIZE:
			break;

		case GAPC_GET_PEER_CENTRAL_RPA:
			break;

		case GAPC_GET_PEER_RPA_ONLY:
			break;

		case GAPC_SIGN_PACKET:
			break;

		case GAPC_SIGN_CHECK:
			break;

		case GAPC_LAST:
			break;
		default:
			break;
	}

	return (KE_MSG_CONSUMED);
}

int HandleGattcCmpEvt(ke_msg_id_t msgid,
					  struct gattc_cmp_evt *param,
					  ke_task_id_t dest_id,
					  ke_task_id_t src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	switch (param->operation)
	{
		case GATTC_NO_OP:
			break;

		case GATTC_MTU_EXCH:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_MTU_EXCH) \n");

			if (param->status != ATT_ERR_NO_ERROR)
			{
				DBG_ERR("    MTU negotiation failed, disconnecting..., Status : %d\n", 
						param->status);
			}
			else
			{
				DBG_INFO("    MTU negotiation successful\n");
#if defined (__CONN_PARAM_UPDATE__)				
				app_update_conn_params();
#endif
			}

		} break;

		case GATTC_DISC_ALL_SVC:
			break;

		case GATTC_DISC_BY_UUID_SVC:
			break;

		case GATTC_DISC_INCLUDED_SVC:
			break;

		case GATTC_DISC_ALL_CHAR:
			break;

		case GATTC_DISC_BY_UUID_CHAR:
			break;

		case GATTC_DISC_DESC_CHAR:
			break;

		case GATTC_READ:
			break;

		case GATTC_READ_LONG:
			break;

		case GATTC_READ_BY_UUID:
			break;

		case GATTC_READ_MULTIPLE:
			break;

		case GATTC_WRITE:
			break;

		case GATTC_WRITE_NO_RESPONSE:
			break;

		case GATTC_WRITE_SIGNED:
			break;

		case GATTC_EXEC_WRITE:
			break;

		case GATTC_REGISTER:
			break;

		case GATTC_UNREGISTER:
			break;

		case GATTC_NOTIFY:
			break;

		case GATTC_INDICATE:
			break;

		case GATTC_SVC_CHANGED:
		{
			DBG_INFO("<<< GATTC_CMP_EVT (GATTC_SVC_CHANGED) \n");

			if (param->status != ATT_ERR_NO_ERROR)
			{
				DBG_ERR("        GATTC_SVC_CHANGED: error\n");
			}
			else
			{
				DBG_INFO("        GATTC_SVC_CHANGED: no error\n");
			}

		} break;

		case GATTC_SDP_DISC_SVC:
			break;

		case GATTC_SDP_DISC_SVC_ALL:
			break;

		case GATTC_SDP_DISC_CANCEL:
			break;

		case GATTC_LAST:
			break;
		default:
			break;
	}

	return (KE_MSG_CONSUMED);
}

#endif /* __BLE_PERI_WIFI_SVC_PERIPHERAL__ */

/* EOF */
