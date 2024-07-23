/**
 ****************************************************************************************
 *
 * @file app_task.h
 *
 * @brief Header file for application handlers for ble events and responses.
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

#ifndef APP_TASK_H_
#define APP_TASK_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "../../../../../../../core/ble_interface/ble_inc/platform/core_modules/ke/api/ke_task.h"
#include "../../../../../../../core/ble_interface/ble_inc/platform/core_modules/ke/api/ke_msg.h"
#include "gapm_task.h"
#include "gapc_task.h"
#include "proxm_task.h"
#include "disc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/// number of APP Process
#define APP_IDX_MAX  0x01

/// states of APP task
enum
{
	/// Idle state
	APP_IDLE,
	/// Scanning state
	APP_SCAN,
	/// Connected state
	APP_CONNECTED,
	/// Number of defined states.
	APP_STATE_MAX
};

/*
 * LOCAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
struct gap_reset_req_cmp_evt
{
	USHORT temp;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct app_env_tag app_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles GAPM command completion events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_cmp_evt_handler(ke_msg_id_t msgid,
						 struct gapm_cmp_evt *param,
						 ke_task_id_t dest_id,
						 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GAPC_CMP_EVT messages.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_cmp_evt_handler(ke_msg_id_t msgid,
						 struct gapc_cmp_evt *param,
						 ke_task_id_t dest_id,
						 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles ready indication from GAPM.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_device_ready_ind_handler(ke_msg_id_t msgid,
								  void *param,
								  ke_task_id_t dest_id,
								  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GAPM_ADV_REPORT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
								struct gapm_adv_report_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles the GAPC_CONNECTION_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
									struct gapc_connection_req_ind *param,
									ke_task_id_t dest_id,
									ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GAPC_DISCONNECT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
								struct gapc_disconnect_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GAPC_CON_RSSI_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_con_rssi_ind_handler(ke_msg_id_t msgid,
							  struct gapc_con_rssi_ind *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Get dev info req indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_get_dev_info_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_get_dev_info_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Set dev info req indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_set_dev_info_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_set_dev_info_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Param update req indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_param_update_req_ind_handler(ke_msg_id_t msgid,
									  struct gapc_param_update_req_ind *param,
									  ke_task_id_t dest_id,
									  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handle Bond indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_bond_ind_handler(ke_msg_id_t msgid,
						  struct gapc_bond_ind *param,
						  ke_task_id_t dest_id,
						  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GAPM_PROFILE_ADDED_IND event.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
*/
int gapm_profile_added_ind_handler(ke_msg_id_t msgid,
								   struct gapm_profile_added_ind *param,
								   ke_task_id_t dest_id,
								   ke_task_id_t src_id);
/**
  ****************************************************************************************
 * @brief Handle GAPC_BOND_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_bond_req_ind_handler(ke_msg_id_t msgid,
							  struct gapc_bond_req_ind *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id);

int gapc_le_pkt_size_ind_handler(ke_msg_id_t msgid,
								 struct gapc_le_pkt_size_ind *param,
								 ke_task_id_t dest_id,
								 ke_task_id_t src_id);


/**
 ****************************************************************************************
 * @brief Handle reset GAP request.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_reset_req_cmp_evt_handler(ke_msg_id_t msgid,
								  struct gap_reset_req_cmp_evt *param,
								  ke_task_id_t dest_id,
								  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_SVC_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_svc_ind_handler(ke_msg_id_t msgid,
							   struct gattc_disc_svc_ind *param,
							   ke_task_id_t dest_id,
							   ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_CHAR_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_char_ind_handler(ke_msg_id_t msgid,
								struct gattc_disc_char_ind *param,
								ke_task_id_t dest_id,
								ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_CHAR_DESC_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_char_desc_ind_handler(ke_msg_id_t msgid,
									 struct gattc_disc_char_desc_ind *param,
									 ke_task_id_t dest_id,
									 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_EVENT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_event_ind_handler(ke_msg_id_t msgid,
							struct gattc_event_ind *param,
							ke_task_id_t dest_id,
							ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC command completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_cmp_evt_handler(ke_msg_id_t msgid,
						  struct gattc_cmp_evt *param,
						  ke_task_id_t dest_id,
						  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_READ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_read_ind_handler(ke_msg_id_t msgid,
						   struct gattc_read_ind *param,
						   ke_task_id_t dest_id,
						   ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles GATTC_MTU_CHANGED_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_mtu_changed_ind_handler(ke_msg_id_t msgid,
								  struct gattc_mtu_changed_ind *param,
								  ke_task_id_t dest_id,
								  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles Proximity Monitor profile enable confirmation.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_enable_rsp_handler(ke_msg_id_t msgid,
							  struct proxm_enable_rsp *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles Read characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_rd_char_rsp_handler(ke_msg_id_t msgid,
							   struct proxm_rd_rsp *param,
							   ke_task_id_t dest_id,
							   ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles write characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  proxm_wr_alert_lvl_rsp_handler(ke_msg_id_t msgid,
									struct proxm_wr_alert_lvl_rsp *param,
									ke_task_id_t dest_id,
									ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles DIS Client profile enable confirmation.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  disc_enable_rsp_handler(ke_msg_id_t msgid,
							 struct disc_enable_rsp *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles DIS read characteristic response.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int  disc_rd_char_rsp_handler(ke_msg_id_t msgid,
							  struct disc_rd_char_rsp *param,
							  ke_task_id_t dest_id,
							  ke_task_id_t src_id);

void read_disc_chars(uint8_t id, uint8_t con_id);
/// @} APPTASK

#endif // APP_TASK_H_
