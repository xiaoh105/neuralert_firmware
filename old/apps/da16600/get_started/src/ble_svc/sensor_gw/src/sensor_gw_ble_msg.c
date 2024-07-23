/**
 ****************************************************************************************
 *
 * @file sensor_gw_ble_msg.c
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

#if defined (__BLE_CENT_SENSOR_GW__)

// ble config
#include "../inc/da14585_config.h"

// ble platform common include
#include "gapc_task.h"
#include "gapm_task.h"
#include "disc_task.h"
#include "proxm_task.h"

// gtl platform common include
#include "../../include/ble_msg.h"
#include "../../include/queue.h"
#include "../../include/uart.h"
#include "../../include/gatt_handlers.h"

// project specific include
#include "../inc/app_task.h"
#include "da16_gtl_features.h"
#ifdef __FREERTOS__
#include "msg_queue.h"
#endif

void *BleMsgAlloc(unsigned short id, unsigned short dest_id,
				  unsigned short src_id, unsigned short param_len)
{
	ble_msg *blemsg=(ble_msg *)MEMALLOC(sizeof(ble_msg)+param_len-sizeof(unsigned char));

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

	MEMFREE(blemsg);
}

void BleSendMsg(void *message)
{
	ble_msg *blemsg = (ble_msg *)((unsigned char *)message - sizeof(ble_hdr));

	if (blemsg) {
#if defined(__GTL_IFC_UART__)
		UARTSend(blemsg->bLength + sizeof(ble_hdr), (unsigned char *) blemsg);
		MEMFREE(blemsg);
#endif
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
#endif
#endif
}

void HandleBleMsg(ble_msg *blemsg)
{

	if (blemsg->bDstid != TASK_ID_GTL )
	{
		DBG_ERR("[%s] Destination task should be TASK_ID_GTL (16)", __func__);
		return;
	}

	switch (blemsg->bType)
	{
		/*
		 *	GAPM events
		 */
	
		// Command Complete event
		case GAPM_CMP_EVT:
		{
			gapm_cmp_evt_handler(GAPM_CMP_EVT, (struct gapm_cmp_evt *)blemsg->bData, 
								 blemsg->bDstid,
								 blemsg->bSrcid);
		} break;
		// Event triggered to inform that lower layers are ready
		case GAPM_DEVICE_READY_IND:
		{
			DBG_INFO("<<< GAPM_DEVICE_READY_IND \n");
			gapm_device_ready_ind_handler(GAPM_DEVICE_READY_IND,
										  (struct gap_ready_evt *)blemsg->bData, 
										  blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Local device version indication event
		case GAPM_DEV_VERSION_IND:
			break;

		// Local device BD Address indication event
		case GAPM_DEV_BDADDR_IND:
			break;

		// White List Size indication event
		case GAPM_WHITE_LIST_SIZE_IND:
			break;

		// Advertising or scanning report information event
		case GAPM_ADV_REPORT_IND:
		{
			gapm_adv_report_ind_handler(GAPM_ADV_REPORT_IND,
										(struct gapm_adv_report_ind *) blemsg->bData, 
										blemsg->bDstid, 
										blemsg->bSrcid);
		} break;
		// Name of peer device indication
		case GAPM_PEER_NAME_IND:
			break;

		// Indicate that Resolvable Private Address has been solved
		case GAPM_ADDR_SOLVED_IND:
			break;

		//  AES-128 block result indication
		case GAPM_USE_ENC_BLOCK_IND:
			break;

		// Random Number Indication
		case GAPM_GEN_RAND_NB_IND:
			break;

		// Inform that profile task has been added.
		case GAPM_PROFILE_ADDED_IND:
			break;

		// Indication containing information about memory usage.
		case GAPM_DBG_MEM_INFO_IND:
			break;

		// Advertising channel Tx power level
		case GAPM_DEV_ADV_TX_POWER_IND:
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

		/*
		 *	GAPC events
		 */

		// Command Complete event
		case GAPC_CMP_EVT:
		{
			gapc_cmp_evt_handler(GAPC_CMP_EVT, (struct gapc_cmp_evt *)blemsg->bData, 
								blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Indicate that a connection has been established
		case GAPC_CONNECTION_REQ_IND:
		{
			DBG_INFO("<<< GAPC_CONNECTION_REQ_IND \n");
			gapc_connection_req_ind_handler(GAPC_CONNECTION_REQ_IND,
										(struct gapc_connection_req_ind *) blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
#if defined (__BLE_PROVISIONING_SAMPLE__)
			app_check_provision(1);
#endif
		} break;
		// Indicate that a link has been disconnected
		case GAPC_DISCONNECT_IND:
		{
			DBG_INFO("<<< GAPC_DISCONNECT_IND \n");
			gapc_disconnect_ind_handler(GAPC_DISCONNECT_IND,
										(struct gapc_disconnect_ind *) blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
#if defined (__BLE_PROVISIONING_SAMPLE__)
			app_check_provision(0);
#endif
		} break;
		// Name,Appearance of peer device indication
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
			gapc_con_rssi_ind_handler(GAPC_CON_RSSI_IND, 
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
			DBG_INFO("<<< GAPC_SET_DEV_INFO_REQ_IND\n");
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
			// user able to check connection paramters agreed: see gapc_param_updated_ind
			DBG_INFO("<<< GAPC_PARAM_UPDATED_IND\n");
		} break;
		// Bonding requested by peer device indication message.
		case GAPC_BOND_REQ_IND:
		{
			DBG_INFO("<<< GAPC_BOND_REQ_IND\n");
			gapc_bond_req_ind_handler(GAPC_BOND_REQ_IND, 
									 (struct gapc_bond_req_ind *) blemsg->bData,
									 blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Bonding information indication message
		case GAPC_BOND_IND:
		{
			DBG_INFO("<<< GAPC_BOND_IND\n");
			gapc_bond_ind_handler(GAPC_BOND_IND, (struct gapc_bond_ind *) blemsg->bData,
								  blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Encryption requested by peer device indication message.
		case GAPC_ENCRYPT_REQ_IND:
		{
			DBG_INFO("<<< GAPC_ENCRYPT_REQ_IND\n");
		} break;
		// Encryption information indication message
		case GAPC_ENCRYPT_IND:
		{
			DBG_INFO("<<< GAPC_ENCRYPT_IND\n");
		} break;
		// Security requested by peer device indication message
		case GAPC_SECURITY_IND:
		{
			DBG_INFO("<<< GAPC_SECURITY_IND\n");
		} break;
		// Indicate the current sign counters to the application
		case GAPC_SIGN_COUNTER_IND:
			break;
			
		// Indication of ongoing connection Channel Map
		case GAPC_CON_CHANNEL_MAP_IND:
			break;

		//Indication of LE Data Length
		case GAPC_LE_PKT_SIZE_IND:
		{
			DBG_INFO("<<< GAPC_LE_PKT_SIZE_IND\n");
			gapc_le_pkt_size_ind_handler(GAPC_LE_PKT_SIZE_IND,
										 (struct gapc_le_pkt_size_ind *) blemsg->bData, 
										 blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Parameter update procedure timeout indication
		case GAPC_PARAM_UPDATE_TO_IND:
			break;

		/*
		 *	 GATTM events
		 */

		// Add service in database response
		case GATTM_ADD_SVC_RSP:
		{
			gattm_add_svc_rsp_hnd(blemsg->bType, 
								 (struct gattm_add_svc_rsp const *)&blemsg->bData,
								 blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Set permission settings of service response
		case GATTM_SVC_SET_PERMISSION_RSP:
		{
			gattm_svc_set_permission_rsp_hnd(blemsg->bType,
							(struct gattm_svc_set_permission_rsp const *)blemsg->bData, 
							blemsg->bDstid, blemsg->bSrcid);
		} break;
		// Set attribute value response
		case GATTM_ATT_SET_VALUE_RSP:
		{
			gattm_att_set_value_rsp_hnd(blemsg->bType,
									(struct gattm_att_set_value_rsp const *)blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
		} break;

		/*
		 *	GATTC events
		 */

		// Command Complete event
		case GATTC_CMP_EVT:
		{
			gattc_cmp_evt_handler(GATTC_CMP_EVT, (struct gattc_cmp_evt *) blemsg->bData,
								  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that the ATT MTU has been updated (negotiated)
		case GATTC_MTU_CHANGED_IND:
		{
			DBG_INFO("<<< GATTC_MTU_CHANGED_IND \n");
			gattc_mtu_changed_ind_handler(GATTC_MTU_CHANGED_IND,
										  (struct gattc_mtu_changed_ind *) blemsg->bData, 
										  blemsg->bDstid, blemsg->bSrcid);
		} break;

		// Indicate that Service Changed indications has been enabled or disabled
		case GATTC_SVC_CHANGED_CFG_IND:
		    break;

		case GATTC_DISC_SVC_IND:
		{
			DBG_INFO("<<< GATTC_DISC_SVC_IND \n");
			gattc_disc_svc_ind_handler(GATTC_DISC_SVC_IND,
									  (struct gattc_disc_svc_ind *) blemsg->bData, 
									   blemsg->bDstid, blemsg->bSrcid);
		} break;
		case GATTC_DISC_CHAR_IND:
		{
			DBG_INFO("<<< GATTC_DISC_CHAR_IND \n");
			gattc_disc_char_ind_handler(GATTC_DISC_CHAR_IND,
										(struct gattc_disc_char_ind *) blemsg->bData, 
										blemsg->bDstid, blemsg->bSrcid);
		} break;
		case GATTC_DISC_CHAR_DESC_IND:
		{
			DBG_INFO("<<< GATTC_DISC_CHAR_DESC_IND \n");
			gattc_disc_char_desc_ind_handler(GATTC_DISC_CHAR_IND,
										(struct gattc_disc_char_desc_ind *) blemsg->bData,
										blemsg->bDstid, blemsg->bSrcid);
		} break;
		case GATTC_READ_IND:
		{
			DBG_INFO("<<< GATTC_READ_IND \n");
			gattc_read_ind_handler(GATTC_READ_IND, 
								   (struct gattc_read_ind *) blemsg->bData,
								   blemsg->bDstid, blemsg->bSrcid);
		} break;
		case GATTC_EVENT_IND:
		{
			DBG_INFO("<<< GATTC_EVENT_IND (NOTI or INDI) \n");
			gattc_event_ind_handler(GATTC_EVENT_IND, 
									(struct gattc_event_ind *) blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid);
		} break;
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

		/*
		 *	 PROXM events
		 */
		case PROXM_ENABLE_RSP:
		{
			DBG_INFO("<<< PROXM_ENABLE_RSP \n");
			proxm_enable_rsp_handler(PROXM_ENABLE_RSP, 
									(struct proxm_enable_rsp *) blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid);
		} break;
		case PROXM_RD_RSP:
		{
			DBG_INFO("<<< PROXM_RD_RSP \n");
			proxm_rd_char_rsp_handler(PROXM_RD_RSP, (struct proxm_rd_rsp *) blemsg->bData,
									  blemsg->bDstid, blemsg->bSrcid);
		} break;
		case PROXM_WR_ALERT_LVL_RSP:
		{
			proxm_wr_alert_lvl_rsp_handler(PROXM_WR_ALERT_LVL_RSP,
										  (struct proxm_wr_alert_lvl_rsp *) blemsg->bData, 
										   blemsg->bDstid, blemsg->bSrcid);
		} break;

		/*
		 *	 DISC events
		 */
		
		case DISC_ENABLE_RSP:
		{
			DBG_INFO("<<< DISC_ENABLE_RSP \n");
			disc_enable_rsp_handler(DISC_ENABLE_RSP, 
									(struct disc_enable_rsp *) blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid);
		} break;
		case DISC_RD_CHAR_RSP:
		{
			DBG_INFO("<<< DISC_RD_CHAR_RSP \n");
			disc_rd_char_rsp_handler(DISC_RD_CHAR_RSP, 
									(struct disc_rd_char_rsp *) blemsg->bData,
									blemsg->bDstid, blemsg->bSrcid);
		} break;
		default:
		{
			DBG_INFO("<<< UNKNOWN CMD, bType = 0x%x \n", blemsg->bType);
		}break;
	}

}
#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */
