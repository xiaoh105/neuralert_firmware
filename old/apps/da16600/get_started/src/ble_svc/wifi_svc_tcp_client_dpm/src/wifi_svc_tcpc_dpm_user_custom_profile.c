/**
 ****************************************************************************************
 *
 * @file wifi_svc_user_custom_profile.c
 *
 * @brief Wi-Fi SVC implementation
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

#if defined (__BLE_PERI_WIFI_SVC_TCP_DPM__)

#if defined (__WFSVC_ENABLE__)

#include "../../include/da14585_config.h"

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#include "da16_gtl_features.h"
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif
#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif
#include "../../include/app.h"

#if defined (__GTL_IFC_UART__)
#include "../../include/uart.h"
#endif

#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "common_config.h"
#include "user_dpm_api.h"
#include "json.h"
#include "ota_update.h"
#include "defs.h"
#if defined (__SUPPORT_BLE_PROVISIONING__)
#include "command.h"
#include "iface_defs.h"
#include "../../provision/inc/app_provision.h"

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__) || defined (__DA14531_BOOT_FROM_UART__) || defined ( __SUPPORT_WIFI_CONN_CB__ )
#include "../../user_util/user_util.h"
#endif

#define DEFAULT_GOOGLE_PING_IP          "8.8.8.8"
#define PROVISIONING_RESULT_BUF_SIZE    200

int32_t select_ap_cmd_result = 1;
uint32_t flag_provisioning = 0;

extern int check_net_init(int iface);
extern int wifi_netif_status(int intf);
extern int check_net_ip_status(int iface);
extern char *dns_A_Query(char* domain_name, ULONG wait_option);
extern int get_supp_conn_state(void);
#endif //__SUPPORT_BLE_PROVISIONING__

static uint32_t app_gatt_seq_num_to_send;

extern int chk_supp_connected(void);
extern void reboot_func(UINT flag);
extern void combo_ble_sw_reset(void);
extern UINT factory_reset(int reboot_flag);
extern void hold_dpm_sleepd(void);
#if defined (__SUPPORT_BLE_PROVISIONING__)
extern void setSleepModeDtim(int _sleepMode, int _useDpmSet, int _rtcTime, int _dpmKeepAlive, int _userWakeup, int _timWakeup);
#endif // __SUPPORT_BLE_PROVISIONING__

uint32_t wifi_sta_scan_request(void);
int32_t wifi_sta_select_ap(void);
int32_t wifi_conf_parse_json(int8_t *value, uint32_t *cmd);

#if defined (__SUPPORT_BLE_PROVISIONING__)
void clear_wifi_provisioning_result_str(void);
int32_t wifi_set_dpm_info(void);
uint32_t wifi_svc_get_provisioning_flag(void);

typedef struct _wifi_provisioning_result
{
	char prov_result_str[PROVISIONING_RESULT_BUF_SIZE];
	uint32_t command_result;
	uint32_t len_of_result;
} wifi_provisioning_result_t;

wifi_provisioning_result_t wifi_prov_result;
#endif // __SUPPORT_BLE_PROVISIONING__

typedef struct wifi_config
{
	char	ssid[50];

	/* 0 = none, 1 = WEP, 2 = WPA, 3 = WPA2 */
	int		security_type;

	/* 0 = Static IP, 1 = DHCP, 2 = RFC3927 Link Local IP */
	int 	dhcp;
	char	password[90];
	char	ipv4_address[15];
	char	mask[15];
	char	gateway[15];
	char	dns_address[15];

	/* 0: non-dpm mode, 1: dpm-modes */
	int		dpm_mode;

	// for demo only; custom provision data
	char	srv_ip[15];
	int		srv_port;
#if defined (__SUPPORT_BLE_PROVISIONING__)
	char	ping_addr[15];
	char	svr_url[128]; //user URL
	int		sleep_mode;
	int		rtc_timer_sec;
	int		use_dpm;
	int		dpm_keep_alive;
	int		user_wakeup;
	int		tim_wakeup;
#endif	// __SUPPORT_BLE_PROVISIONING__

} wifi_conf_t;

wifi_conf_t wifi_conf;

char wfsvc_wfcmd_desc[] = "Wi-Fi Cmd";
char wfsvc_wfact_res_desc[] = "Wi-Fi Action Result";
char wfsvc_apscan_res_desc[] = "AP Scan Result";
#if defined (__SUPPORT_BLE_PROVISIONING__)
char wfsvc_provisioning_res_desc[] = "Provisioning Result";
#endif //__SUPPORT_BLE_PROVISIONING__

// characteristic data value
local_wfsvc_values_t wfsvc_local_wfcmd_values;
local_wfsvc_values_t wfsvc_local_wfact_res_values;

/*
	Wi-Fi scan data is not maintained here, but dynamically created and deleted after use,
	please see 	gattc_read_req_ind_hnd( ). But still some info is stored in this global 
	data storage in case needed. e.g.) CCCD, etc.
*/
local_wfsvc_values_t wfsvc_local_apscan_res_values;
#if defined (__SUPPORT_BLE_PROVISIONING__)
local_wfsvc_values_t wfsvc_local_provisioning_res_values;
#endif //__SUPPORT_BLE_PROVISIONING__

// service definition
struct gattm_svc_desc_custom_wfsvc_t gattm_svc_desc_wfsvc =
{
	.start_hdl = 0,
	.task_id = TASK_ID_GTL,
#if defined (__WIFI_SVC_SECURITY__)
	.perm = PERM(SVC_PRIMARY, ENABLE) | PERM(SVC_AUTH, UNAUTH) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128),
#else
	.perm = PERM(SVC_PRIMARY, ENABLE) | PERM(SVC_AUTH, ENABLE) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128),
#endif
	.nb_att = CUSTOM_SERVICE_WFSVC_NB_ATTS,
	.uuid = DEF_CUSTOM_SERVICE_WFSVC_UUID_128,
	.atts = {

		// --- CHARACTERISTIC WFCMD ---

		// custom characteristic wfcmd Declaration
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_DECL_IDX] = {{(uint8_t)ATT_DECL_CHARACTERISTIC, ATT_DECL_CHARACTERISTIC >> 8}, PERM(RD, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_16), 0},

		// custom characteristic wfcmd Value attribute
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_VALUE_IDX] = {DEF_CUSTOM_CHAR_WFSVC_WFCMD_UUID_128, PERM_VAL(UUID_LEN, PERM_UUID_128) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE), DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD},

		// custom characteristic wfcmd User Description
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_USER_DESC_IDX] = {{(uint8_t)ATT_DESC_CHAR_USER_DESCRIPTION, ATT_DESC_CHAR_USER_DESCRIPTION >> 8}, PERM(RD, ENABLE),  (sizeof(wfsvc_wfcmd_desc))},


		// --- CHARACTERISTIC WFACT RESULT REPORT ---

		// custom characteristic wfact res Declaration
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_DECL_IDX] = {{(uint8_t)ATT_DECL_CHARACTERISTIC, ATT_DECL_CHARACTERISTIC >> 8}, PERM(RD, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_16), 0},

		//  custom characteristic wfact res Value attribute
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_VALUE_IDX] = {DEF_CUSTOM_CHAR_WFSVC_WFACT_RES_UUID_128, PERM_VAL(UUID_LEN, PERM_UUID_128) | PERM(RD, ENABLE) | PERM(NTF, ENABLE), PERM(RI, ENABLE) | DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD_RES},

		// custom characteristic wfact res Configuration Descriptor (RI is Enforced)
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX] = {{(uint8_t)ATT_DESC_CLIENT_CHAR_CFG, ATT_DESC_CLIENT_CHAR_CFG >> 8}, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), sizeof(ATT_DESC_CLIENT_CHAR_CFG) },

		// custom characteristic wfact res User Description
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_USER_DESC_IDX] = {{(uint8_t)ATT_DESC_CHAR_USER_DESCRIPTION, ATT_DESC_CHAR_USER_DESCRIPTION >> 8}, PERM(RD, ENABLE), (sizeof(wfsvc_wfact_res_desc))},


		// --- CHARACTERISTIC AP SCAN RESULT ---

		// custom characteristic apscan result Declaration
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_DECL_IDX] = {{(uint8_t)ATT_DECL_CHARACTERISTIC, ATT_DECL_CHARACTERISTIC >> 8}, PERM(RD, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_16), 0},

		// custom characteristic apscan result Value attribute
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_VALUE_IDX] = {DEF_CUSTOM_CHAR_WFSVC_APSCAN_RES_UUID_128, PERM_VAL(UUID_LEN, PERM_UUID_128) | PERM(RD, ENABLE) /*| PERM(NTF, ENABLE)*/, PERM(RI, ENABLE) | DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES},

		// custom characteristic apscan result Configuration Descriptor (RI is Enforced)
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX] = {{(uint8_t)ATT_DESC_CLIENT_CHAR_CFG, ATT_DESC_CLIENT_CHAR_CFG >> 8}, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), sizeof(ATT_DESC_CLIENT_CHAR_CFG) },

		// custom characteristic apscan result User Description
		[USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_USER_DESC_IDX] = {{(uint8_t)ATT_DESC_CHAR_USER_DESCRIPTION, ATT_DESC_CHAR_USER_DESCRIPTION >> 8}, PERM(RD, ENABLE), (sizeof(wfsvc_apscan_res_desc))},

#if defined (__SUPPORT_BLE_PROVISIONING__)
        // --- CHARACTERISTIC PROVISIONING TEST RESULT ---
        // custom characteristic provisioning result Declaration
        [USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_DECL_IDX] = {
            {(uint8_t)ATT_DECL_CHARACTERISTIC, ATT_DECL_CHARACTERISTIC >> 8},
            PERM(RD, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_16),
            0
        },

        // custom characteristic provisioning test result value attribute
        [USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_VALUE_IDX] = {
            DEF_CUSTOM_CHAR_WFSVC_PROVISIONING_RES_UUID_128,
            PERM_VAL(UUID_LEN, PERM_UUID_128) | PERM(RD, ENABLE) /*| PERM(NTF, ENABLE)*/,
            PERM(RI, ENABLE) | DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_PROVISIONING_RES
        },

        // custom characteristic provisioning result Configuration Descriptor (RI is Enforced)
        [USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX] = {
            {(uint8_t)ATT_DESC_CLIENT_CHAR_CFG, ATT_DESC_CLIENT_CHAR_CFG >> 8},
            PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE),
            sizeof(ATT_DESC_CLIENT_CHAR_CFG)
        },

        // custom characteristic provisioning result User Description
        [USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_USER_DESC_IDX] = {
            {(uint8_t)ATT_DESC_CHAR_USER_DESCRIPTION, ATT_DESC_CHAR_USER_DESCRIPTION >> 8},
            PERM(RD, ENABLE),
            (sizeof(wfsvc_provisioning_res_desc))
        }
#endif // __SUPPORT_BLE_PROVISIONING__
    }
};

extern user_custom_profile_wfsvc_env_t user_custom_profile_wfsvc_env;

uint32_t app_get_gatt_seq_num_and_advance(void)
{
	return ++app_gatt_seq_num_to_send % 0xFFFFFFFF;
}

void attm_svc_create_custom_db_wfsvc(void)
{
	struct gattm_add_svc_req *svc_req = BleMsgAlloc(GATTM_ADD_SVC_REQ, TASK_ID_GATTM,
										TASK_ID_GTL, sizeof(struct gattm_svc_desc_custom_wfsvc_t));

	memcpy(svc_req, &gattm_svc_desc_wfsvc, sizeof(gattm_svc_desc_wfsvc));
	BleSendMsg(svc_req);
	return;
}

uint8_t wfsvc_gattc_notify_cmd_send(uint8_t handle, uint8_t *data, uint16_t length)
{
	struct gattc_send_evt_cmd *cmd = BleMsgAlloc(GATTC_SEND_EVT_CMD, TASK_ID_GATTC,
									 TASK_ID_GTL, 
									 sizeof (struct gattc_send_evt_cmd) + length);
	cmd->operation = GATTC_NOTIFY;
	cmd->seq_num = app_get_gatt_seq_num_and_advance();
	cmd->handle = handle;
	cmd->length = length;
	memcpy(cmd->value, data, length);
	BleSendMsg(cmd);
	return 0;
}

uint8_t wfsvc_gattc_indicate_cmd_send(uint8_t handle, uint8_t *data, uint16_t length)
{
	struct gattc_send_evt_cmd *cmd = BleMsgAlloc(GATTC_SEND_EVT_CMD, TASK_ID_GATTC,
									 TASK_ID_GTL, 
									 sizeof (struct gattc_send_evt_cmd) + length);
	cmd->operation = GATTC_INDICATE;
	cmd->seq_num = app_get_gatt_seq_num_and_advance();
	cmd->handle = handle;
	cmd->length = length;
	memcpy(cmd->value, data, length);
	BleSendMsg(cmd);
	return 0;
}

uint8_t notify_init = 0x01;
void user_custom_profile_wfsvc_enable(uint16_t conhdl)
{
	DA16X_UNUSED_ARG(conhdl);

	//Set User Description in Service DB on BLE side
	gattm_att_set_value_req_send(user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_USER_DESC_IDX, 
								(uint8_t *)wfsvc_wfcmd_desc,
								 LEN_WFSVC_WFCMD_DESC + 1);
	gattm_att_set_value_req_send(user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_USER_DESC_IDX,
								 (uint8_t *)wfsvc_wfact_res_desc, 
								 LEN_WFSVC_WFACT_RES_DESC + 1);
	gattm_att_set_value_req_send(user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_USER_DESC_IDX,
								 (uint8_t *)wfsvc_apscan_res_desc, 
								 LEN_WFSVC_APSCAN_RES_DESC + 1);

#if defined (__SUPPORT_BLE_PROVISIONING__)
    gattm_att_set_value_req_send(
        user_custom_profile_wfsvc_env.start_handle
            + USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_USER_DESC_IDX,
        (uint8_t *)wfsvc_provisioning_res_desc,
        LEN_WFSVC_PROVISIONING_RES_DESC + 1);
#endif //__SUPPORT_BLE_PROVISIONING__

	//initialize configuration handles to enable notification/indication
	wfsvc_gattc_notify_cmd_send(user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX, 
								(uint8_t *)&notify_init,
								sizeof(uint8_t));

	// local values should be set as well
	wfsvc_local_wfact_res_values.conf = (uint16_t)WFSVC_ENABLE_NOTIFY;

	return;
}

/**
 ****************************************************************************************
 */

void wfsvc_wifi_cmd_result_nofity_user(enum WIFI_ACTION_RESULT wifi_act_res)
{

	// save the result to Wi-Fi Act Result charc
	wfsvc_local_wfact_res_values.value[0] = wifi_act_res;

	/* handle */ wfsvc_gattc_notify_cmd_send(user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_VALUE_IDX,
	/* data */(uint8_t *)wfsvc_local_wfact_res_values.value,
	/* length*/ sizeof(uint16_t));

}

void wfsvc_gattc_write_cmd_cfm_send(struct gattc_write_req_ind const *param,
									ke_task_id_t const dest_id,
									ke_task_id_t const src_id,
									uint8_t status)
{
	struct gattc_write_cfm *cfm = BleMsgAlloc(GATTC_WRITE_CFM, src_id, dest_id,
								  sizeof(struct gattc_write_cfm));
	cfm->handle = param->handle;
	cfm->status = status;
	BleSendMsg((void *) cfm);
}

void provisioning_result_update(uint32_t command_result, char *svr_url, char *ping_ip, char *svr_ip)
{
	cJSON *root;
	char *buf;

	clear_wifi_provisioning_result_str();
	wifi_prov_result.command_result = command_result;
	root = cJSON_CreateObject();
	if (root) {
		cJSON_AddNumberToObject(root, "result", command_result);
		if (svr_url)
			cJSON_AddStringToObject(root, "svr_url", wifi_conf.svr_url);
		if (ping_ip)
			cJSON_AddStringToObject(root, "ping_ip", ping_ip);
		if (svr_ip)
			cJSON_AddStringToObject(root, "svr_ip", svr_ip);

		buf = cJSON_Print(root);
		if (buf) {
			wifi_prov_result.len_of_result = strlen(buf);
			memcpy(wifi_prov_result.prov_result_str, buf, wifi_prov_result.len_of_result);
			MEMFREE(buf);
		}
		cJSON_Delete(root);
	}

	// set wifi-cmd execution result and notify a ble peer
	wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_ACK);
}

void user_custom_profile_report_result(uint32_t num)
{
	cJSON *root;
	char *buf;
	clear_wifi_provisioning_result_str();
	wifi_prov_result.command_result = num;
	root = cJSON_CreateObject();
	if (root) {
		cJSON_AddNumberToObject(root, "result", (double)num);
		buf = cJSON_Print(root);
		if (buf) {
			wifi_prov_result.len_of_result = strlen(buf);
			memcpy(wifi_prov_result.prov_result_str, buf, wifi_prov_result.len_of_result);
			MEMFREE(buf);
		}
		cJSON_Delete(root);
	}

	// set wifi-cmd execution result and notify a ble peer
	wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_ACK);
}

uint8_t user_custom_profile_wfsvc_write_cmd_ind_xxx(ke_msg_id_t const msgid,
		struct gattc_write_req_ind const *param,
		ke_task_id_t const dest_id,
		ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);

	uint32_t cmd = 0;
	int8_t *value = NULL;
#if defined (__SUPPORT_BLE_PROVISIONING__)
	char return_str[16] = {0, };
	int tmp;
#endif

	DBG_INFO("<<< GATTC_WRITE_REQ_IND \n");
	DBG_TMP("    start_handle = %d \n", user_custom_profile_wfsvc_env.start_handle);
	DBG_TMP("    param->handle = %d \n", param->handle);

	// Write "Wi-Fi command" by a ble peer
	if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_VALUE_IDX)
	{
		// first send a "write" confirm response
		wfsvc_gattc_write_cmd_cfm_send(param, dest_id, src_id, ATT_ERR_NO_ERROR);

		value = (int8_t *)MEMALLOC(param->length+1);

		if (value == NULL)
		{
			DBG_ERR("malloc failed \n");
			wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_MEM_ALLOC_FAIL);
			return KE_MSG_CONSUMED;
		}

		memset(value, 0x00, param->length+1);
		memcpy(value, param->value, param->length);
		DBG_TMP("[String from APP]\n");
		DBG_TMP("\t%s \n", value);

		/* Parse JSON */
		if (wifi_conf_parse_json(value, &cmd) != 0)
		{
			DBG_ERR(" UNKNOWN COMMAND!!! \n");
			wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_UNKNOWN_RCV);
			goto finish;
		}

		if ( cmd == COMBO_WIFI_CMD_SCAN_AP)
		{
			DBG_INFO(" COMBO_WIFI_CMD_SCAN_AP received \n");

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
			if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM)
			{
				app_del_ble_peer_idle_timer();
			}
#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */

			// set scan result and notify a ble peer
			if (wifi_sta_scan_request())
			{
				// set wifi-cmd execution result and notify a ble peer
				wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_SCAN_AP_SUCCESS);
			}
			else
			{
				wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_SCAN_AP_FAIL);
			}
		}
		else if ( cmd == COMBO_WIFI_CMD_SELECT_AP)
		{
			DBG_INFO(" COMBO_WIFI_CMD_SELECT_AP received \n");

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
			if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM)
			{
				app_del_ble_peer_idle_timer();
			}
#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */
#if !defined (__SUPPORT_BLE_PROVISIONING__)
			// set scan result and notify a ble peer
			wifi_sta_select_ap();

			// reboot
			combo_ble_sw_reset();
			reboot_func(1);
#else //__SUPPORT_BLE_PROVISIONING__

			// Remove network first
			tmp = da16x_cli_reply("remove_network 0", NULL, return_str);
			if (tmp < 0 || strcmp(return_str, "FAIL") == 0){
				DBG_INFO("Not connected before\n");
			} else {
				DBG_INFO("Remove network and profile first if connected\n");
			}

			// Sync up the profile after removing successfully
			tmp = da16x_cli_reply("save_config", NULL, return_str);
			if (tmp < 0 || strcmp(return_str, "FAIL") == 0){
				wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_SELECT_AP_FAIL);
				DBG_ERR("Error for sync up the profile\n");
				goto finish;
			}

			select_ap_cmd_result = wifi_sta_select_ap();
			if (select_ap_cmd_result == 0)
			{
				flag_provisioning = 1;	// move from Set DPM Info 
				user_custom_profile_report_result(COMBO_WIFI_CMD_SELECT_AP_SUCCESS);
			}
			else
			{
				user_custom_profile_report_result(COMBO_WIFI_CMD_SELECT_AP_FAIL);
			}
#endif // !__SUPPORT_BLE_PROVISIONING__
		}
#if defined (__SUPPORT_BLE_PROVISIONING__)
		else if (cmd == COMBO_WIFI_CMD_SET_NETWORK_INFO)
		{
			DBG_INFO(" COMBO_WIFI_CMD_SET_NETWORK_INFO received \n");
			user_custom_profile_report_result(COMBO_WIFI_PROV_NETWORK_INFO);
		}	
		else if (cmd == COMBO_WIFI_CMD_SET_DPM_INFO)
		{
			DBG_INFO(" COMBO_WIFI_CMD_SET_DPM_INFO received \n");

		}
		else if (cmd == COMBO_WIFI_CMD_CHK_NETWORK)
		{	
				
			if (flag_provisioning != 1 || wifi_prov_result.command_result != COMBO_WIFI_CMD_SELECT_AP_SUCCESS)			
			{
				DBG_INFO("current cmd not available - invalid state\n");
			}
			else
			{
				char *buf;
				cJSON *root;
				int32_t status;
				char *ip = NULL;
				int conn_state = 0;
				int32_t ping_rc = 0;
				int32_t ping_rc2 = 0;
				int32_t tryCount = 0;
				char url_exist_flag = 0;
				char default_ping_ip[32] = {0,};
				uint32_t dns_query_wait_option = 400;

				//for post processing
				if (dpm_mode_is_enabled())
				{
					dpm_mode_disable();
				}

				DBG_INFO("now try to connect to AP selected...\n");
				tmp = da16x_cli_reply("select_network 0", NULL, return_str);
				if (tmp < 0 || strcmp(return_str, "FAIL") == 0){
					wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_PROV_AP_FAIL);
					DBG_ERR("Error when select network 0\n");
					goto finish;
				}

				while (1)
				{
					tryCount++;

					if (conn_state != get_supp_conn_state()) {
						conn_state = get_supp_conn_state();
						PRINTF("Connecting State : [ %d ]\n", get_supp_conn_state());
					}					

					if (get_supp_conn_state() == WPA_COMPLETED)
					{
						PRINTF("\n=====================\n");
						PRINTF("[AP connection is OK] \n");
						PRINTF("=====================\n");
						break;
					}
					vTaskDelay(50);

					if (tryCount > 30)
					{
						PRINTF("\n=====================\n");
						PRINTF("[AP connection is failed] \n");
						PRINTF("=====================\n");
						da16x_cli_reply("disconnect", NULL, NULL);
						#ifndef ENABLE_WPAS_AUTH_FAILED_AFTER_PROVISIONING_FAILED
						delete_nvram_env(NVR_KEY_SSID_0);
						delete_nvram_env(NVR_KEY_ENCKEY_0);
						#endif
						vTaskDelay(10);

						//Send BLE peer the result
						clear_wifi_provisioning_result_str();

						root = cJSON_CreateObject();
						if (root) {
#if defined ( __SUPPORT_WIFI_CONN_CB__ )
                            EventBits_t wifi_conn_ev_bits;

                            wifi_conn_ev_bits = xEventGroupWaitBits(evt_grp_wifi_conn_notify,
                                                                    WIFI_CONN_FAIL_STA_4_BLE,
                                                                    pdTRUE,
                                                                    pdFALSE,
                                                                    WIFI_CONN_NOTI_WAIT_TICK);
                            if (wifi_conn_ev_bits & WIFI_CONN_FAIL_STA_4_BLE) {
                                xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_FAIL_STA_4_BLE);

                                if (wifi_conn_fail_reason == 45) { // 45 == WLAN_REASON_PEERKEY_MISMATCH in ieee802_11_defs.h
                                    PRINTF(" COMBO_WIFI_CMD_INQ_WIFI_STATUS WRONG PW \n");
                                    wifi_prov_result.command_result = COMBO_WIFI_PROV_WRONG_PW;
                                    cJSON_AddNumberToObject(root, "result", COMBO_WIFI_PROV_WRONG_PW);
                                }
                            } else
#endif // __SUPPORT_WIFI_CONN_CB__
							{
								wifi_prov_result.command_result = COMBO_WIFI_PROV_AP_FAIL;
								cJSON_AddNumberToObject(root, "result", COMBO_WIFI_PROV_AP_FAIL);
							}

							cJSON_AddStringToObject(root, "ssid", wifi_conf.ssid);
							cJSON_AddStringToObject(root, "passwd", wifi_conf.password);
							cJSON_AddNumberToObject(root, "security", wifi_conf.security_type);
							buf = cJSON_Print(root);
							if (buf) {
								wifi_prov_result.len_of_result = strlen(buf);
								memcpy(wifi_prov_result.prov_result_str, buf, wifi_prov_result.len_of_result);
								MEMFREE(buf);
							}

							cJSON_Delete(root);
						}

						// set wifi-cmd execution result and notify a ble peer
						wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_ACK);

						goto finish;
					}
				}

//Network initialization & IP status check
				/* wait for network initialization */
				while (1)
				{
					if (check_net_init(WLAN0_IFACE) == pdPASS)
					{
						break;
					}

					vTaskDelay(5);
				}

				/* Waiting netif status */
				status = wifi_netif_status(WLAN0_IFACE);

				while (status == 0xFF || status != pdTRUE)
				{
					vTaskDelay(5);
					status = wifi_netif_status(WLAN0_IFACE);
				}

				/* Check IP address resolution status */
				while (check_net_ip_status(WLAN0_IFACE))
				{
					vTaskDelay(5);
				}

				//DNS queury check
				PRINTF("user server URL = \"%s\"\n", wifi_conf.svr_url);
				if (strlen(wifi_conf.svr_url) > 0)
				{
					url_exist_flag = 1;
					ip = dns_A_Query(wifi_conf.svr_url, dns_query_wait_option);
					if (NULL == ip)
					{
						PRINTF("failed to query DNS => check the DNS or broker domain!\n");
						PRINTF("seriously dns_A_Query() failed\n");
					}
				}
				// No user server url
				else
				{
					url_exist_flag = 0;
				}

				//PING check
				memset(default_ping_ip, 0, sizeof(default_ping_ip));
				strcpy(default_ping_ip, DEFAULT_GOOGLE_PING_IP);
				PRINTF("default user ping addr = %s\n", wifi_conf.ping_addr);
				if (strlen(wifi_conf.ping_addr) > 0 && is_in_valid_ip_class(wifi_conf.ping_addr))
				{
					if (strcmp(default_ping_ip, wifi_conf.ping_addr))
					{
						memset(default_ping_ip, 0, sizeof(default_ping_ip));
						strcpy(default_ping_ip, wifi_conf.ping_addr);
						PRINTF("default ping ip changed to %s\n", wifi_conf.ping_addr);
					}
				}

				if (NULL == ip)
				{
					PRINTF("ping to default ping ip (%s)\n", default_ping_ip);
					ping_rc = app_check_internet_with_ping(default_ping_ip);
					if (url_exist_flag == 1)
					{
						// DNS fail and Google OK
						if (ping_rc == 1)
						{
							provisioning_result_update(COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_OK,
								wifi_conf.svr_url, default_ping_ip, NULL);
						}
						// DNS fail and Google fail
						else
						{
							provisioning_result_update(COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_FAIL,
								wifi_conf.svr_url, default_ping_ip, NULL);
						}
					}
					else
					{
						if (ping_rc == 1)
						{
							provisioning_result_update(COMBO_WIFI_PROV_NO_URL_PING_OK,
								wifi_conf.svr_url, default_ping_ip, NULL);
						}
						else
						{
							provisioning_result_update(COMBO_WIFI_PROV_NO_URL_PING_FAIL,
								wifi_conf.svr_url, default_ping_ip, NULL);
						}
					}
				}
				else
				{
					PRINTF("ping to server url ip (%s)\n", ip);
					ping_rc = app_check_internet_with_ping(ip);
					// DNS OK and PING FAIL
					if (ping_rc != 1)
					{
						PRINTF("server url ip failed n ping to default ping ip (%s)\n", default_ping_ip);
						ping_rc2 = app_check_internet_with_ping(default_ping_ip);
						// DNS OK Ping fail Google OK
						if (ping_rc2 == 1)
						{
							provisioning_result_update(COMBO_WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK,
							wifi_conf.svr_url, default_ping_ip, ip);
						}
						// DNS OK Ping fail Google fail
						else
						{
							provisioning_result_update(COMBO_WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL,
								wifi_conf.svr_url, default_ping_ip, ip);
						}
					}
					// DNS OK and PING OK
					else
					{
						provisioning_result_update(COMBO_WIFI_PROV_DNS_OK_PING_OK,
							wifi_conf.svr_url, NULL, ip);
					}
				}		
			}
		}
#endif // __SUPPORT_BLE_PROVISIONING__
		else if ( cmd == COMBO_WIFI_CMD_FACT_RST)
		{
#if defined (__SUPPORT_AWS_IOT__) && defined(_NVRAM_SAVE_THING_FACORY_) 
			char* AWSThingName = NULL;
			int foundThingName = 0;

	
			if (checkAWSThingNameAtNVRAM() == 1)
			{
				foundThingName = 1;
	
				AWSThingName = getAWSThingName();
			}			
#endif	//	__SUPPORT_AWS_IOT__ && _NVRAM_SAVE_THING_FACORY_
			DBG_INFO(" COMBO_WIFI_CMD_FACT_RST received \n");

			PRINTF("\n" ANSI_COLOR_RED "Start Factory-Reset ...\n" ANSI_COLOR_DEFULT);
#if defined (__SUPPORT_BLE_PROVISIONING__)
			vTaskDelay(5);

			factory_reset(0);

#if defined (__SUPPORT_AWS_IOT__) && defined(_NVRAM_SAVE_THING_FACORY_)
			if (foundThingName == 1)
			{

				write_nvram_string((const char *)AWS_NVRAM_CONFIG_THINGNAME, AWSThingName);
				tx_thread_sleep(10);
			}
#endif // __SUPPORT_AWS_IOT__ && _NVRAM_SAVE_THING_FACTORY_

			combo_ble_sw_reset(); // ble reset

			reboot_func(SYS_REBOOT_POR);

			/* Wait for system-reboot */
			while (1)
			{
				vTaskDelay(5);
			}
#else // !__SUPPORT_BLE_PROVISIONING__
			factory_reset(1);
#endif // __SUPPORT_BLE_PROVISIONING__
		}
		else if ( cmd == COMBO_WIFI_CMD_REBOOT)
		{
			DBG_INFO(" COMBO_WIFI_CMD_REBOOT received \n");
#if !defined (__SUPPORT_BLE_PROVISIONING__)
			// reboot
			combo_ble_sw_reset();
			reboot_func(1);
#else
			user_custom_profile_report_result(COMBO_WIFI_PROV_REBOOT_ACK);
#endif	// __SUPPORT_BLE_PROVISIONING__
		}
#if defined (__DA14531_BOOT_FROM_UART__)
		else if ( cmd == COMBO_WIFI_CMD_FW_UPDATE)
		{
			DBG_INFO(" COMBO_WIFI_CMD_FW_BLE_DOWNLOAD received \n");

			// download ble firmware from the URL to a special area in sflash

			/* DEMO - TEST CODE */
			ota_fw_update_combo();

			// notify wifi-cmd execution result
			wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_FW_BLE_DOWNLOAD_SUCCESS);
		}
#endif /* __DA14531_BOOT_FROM_UART__ */
		else if ( cmd == COMBO_WIFI_CMD_INQ_WIFI_STATUS)
		{
			DBG_INFO(" COMBO_WIFI_CMD_INQ_WIFI_STATUS received \n");

			// check Wi-Fi connection status
			// notify wifi-cmd execution result
			if (chk_supp_connected())
			{
				wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_INQ_WIFI_STATUS_CONNECTED);
			}
			else
			{
#if defined (__SUPPORT_WIFI_CONN_FAIL_BY_WK_IND__)
				if (wifi_conn_fail_by_wrong_pass == TRUE)
					wfsvc_wifi_cmd_result_nofity_user(
						COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED_BY_WRONG_KEY);
				else
#endif
				wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED);
			}
		}
		else if ( cmd == COMBO_WIFI_CMD_DISCONN)
		{
			DBG_INFO(" COMBO_WIFI_CMD_DISCONN received \n");

			// check Wi-Fi connection status. If connected, disconnect
			if (chk_supp_connected() == 1)
			{
				da16x_cli_reply("disconnect", NULL, NULL);
				vTaskDelay(10);
				hold_dpm_sleepd(); // to avoid entering Abnormal DPM Sleep
			}
			wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED);
		}
		else
		{
			DBG_INFO(" COMBO_WIFI_CMD_UNKNOWN received \n");
			wfsvc_wifi_cmd_result_nofity_user(COMBO_WIFI_CMD_UNKNOWN_RCV);
		}
	}

	/*
	  A ble GATT Client is able to either subscribe or unsubscribe to a characteristic notify,
	  ble (gtl slave) handles it and just notify it to host
	*/
	else if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX)
	{
		wfsvc_gattc_write_cmd_cfm_send(param, dest_id, src_id, ATT_ERR_NO_ERROR);

		/*
			If a ble peer asks "subscription" on this characteristic, the subscription is enabled on BLE Stack which indicates the event
			thru this event. On receipt of this event, what applcation should do is to synchronize the status with BLE Stack.

			When the subsribed characteristic's value changes later, and wfsvc_wifi_cmd_result_nofity_user( ) is called, depending on
			subscription status, it may or may not be sent to a peer
		*/
		if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX)
		{
			memcpy((uint16_t *)&wfsvc_local_wfact_res_values.conf, &param->value, sizeof(uint16_t));
		}

		if (wfsvc_local_wfact_res_values.conf == WFSVC_ENABLE_NOTIFY)
		{
			DBG_INFO(" A BLE Peer subscribed to WiFi command result \n");
		}
		else
		{
			DBG_INFO(" A BLE Peer unsubscribed to WiFi command result \n");
		}
	}
	else if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX)
	{
#ifdef __USE_NOTIFY_MECHANISM_FOR_APSCAN_RES_CHAR
		/* BLE peer application should also be synchronized as well*/
		wfsvc_gattc_write_cmd_cfm_send(param, dest_id, src_id, ATT_ERR_NO_ERROR);

		if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX)
		{
			memcpy((uint16_t *)&wfsvc_local_apscan_res_values.conf, &param->value,
				   DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES);
		}

		if (wfsvc_local_wfact_res_values.conf == WFSVC_ENABLE_NOTIFY)
		{
			DBG_INFO(" A BLE Peer subscribed to WiFi command result \n");
		}
		else
		{
			DBG_INFO(" A BLE Peer unsubscribed to WiFi command result \n");
		}
#endif
	}
#if defined (__SUPPORT_BLE_PROVISIONING__)
	else if (param->handle  == user_custom_profile_wfsvc_env.start_handle + USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX)
	{
		DBG_INFO("%s:%d ---- USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX\n", __func__, __LINE__);
	}
#endif // __SUPPORT_BLE_PROVISIONING__
	else
	{
#if !defined(__IOT_SENSOR_SVC_ENABLE__)
		DBG_INFO(" Write on a wrong characteristic index from a BLE Peer \n");
		wfsvc_gattc_write_cmd_cfm_send(param, dest_id, src_id, ATT_ERR_INVALID_HANDLE);
#endif
	}

finish:

	if (value)
	{
		MEMFREE(value);
	}

	return KE_MSG_CONSUMED;
}


uint8_t user_custom_profile_wfsvc_att_set_value_rsp_hnd(ke_msg_id_t const msgid,
		struct gattm_att_set_value_rsp const *param,
		ke_task_id_t const dest_id,
		ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(param);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);
	return KE_MSG_CONSUMED;
}

uint8_t user_custom_profile_wfsvc_gattm_svc_set_permission_rsp_hnd(
	ke_msg_id_t const msgid,
	struct gattm_svc_set_permission_rsp const *param,
	ke_task_id_t const dest_id,
	ke_task_id_t const src_id)
{
	DA16X_UNUSED_ARG(msgid);
	DA16X_UNUSED_ARG(dest_id);
	DA16X_UNUSED_ARG(src_id);

	if (param->status != ATT_ERR_NO_ERROR)
	{
		DBG_ERR("ERROR : set permission on service, status = %d (see enum hl_err) \n",
				param->status);
	}

	return KE_MSG_CONSUMED;
}

#define HIDDEN_SSID_DETECTION_CHAR	'\t'
#define	CLI_SCAN_RSP_BUF_SIZE		3584
#define	SCAN_RESULT_MAX				30
#define	SCAN_RESULT_BUF_SIZE		4096
#define SSID_BASE64_LEN_MAX			48

typedef struct _wifi_sta_scan_result
{
	char scan_result_str[SCAN_RESULT_BUF_SIZE];
	uint32_t scan_result_str_len;
	uint32_t scan_result_str_idx;
} wifi_sta_scan_result_t;

wifi_sta_scan_result_t wifi_scan_result;

static int rssi_to_percentage(int rssi)
{
	int ret = (rssi + 95) * 2;

	if (ret > 100)
	{
		ret = 100;
	}

	return ret;
}
uint32_t wifi_sta_scan_request(void)
{
	cJSON *root = NULL;
	cJSON *scan_res_json = NULL;
	char *buf;
	UINT length = 0;

	char *scan_res_text, *p;
	int scan_res_num = -1;

	typedef struct _struct_scan_res
	{
		char ssid[SSID_BASE64_LEN_MAX];
		int security_type;
		int signal_strength;
		int freq;
	} struct_scan_res;

	struct_scan_res *scan_res_array = (struct_scan_res *)MEMALLOC(sizeof(struct_scan_res) * SCAN_RESULT_MAX);
	if (scan_res_array == NULL)
	{
		DBG_ERR("[%s] malloc failed. \n", __func__);
		return 0;
	}
	memset(scan_res_array, 0, sizeof(struct_scan_res) * SCAN_RESULT_MAX);

	scan_res_text = (char *)MEMALLOC(CLI_SCAN_RSP_BUF_SIZE);
	if (scan_res_text == NULL)
	{
		DBG_ERR("[%s] malloc failed. \n", __func__);
		return 0;
	}
	memset(scan_res_text, '\0', CLI_SCAN_RSP_BUF_SIZE);

	/* cli scan */
	if (da16x_get_config_str(DA16X_CONF_STR_SCAN, scan_res_text))
	{
		DBG_ERR("[%s] Wi-Fi Scan request failed. \n", __func__);
		MEMFREE(scan_res_text);
		return 0;
	}

	DBG_INFO("[%s] Wi-Fi Scan request success. \n", __func__);

	if (strtok(scan_res_text, "\n") == NULL)   /* Title Skip */
	{
		MEMFREE(scan_res_text);
		return 0;
	}

	for (scan_res_num = 0; scan_res_num < SCAN_RESULT_MAX;)
	{
		if (strtok(NULL, "\t") == NULL)  	/* BSSID => Skip! */
		{
			break;
		}

		p = strtok(NULL, "\t");			/* frequency */
		scan_res_array[scan_res_num].freq = atoi(p);

		p = strtok(NULL, "\t");			/* RSSI */
		scan_res_array[scan_res_num].signal_strength = rssi_to_percentage(atoi(p));

		p = strtok(NULL, "\t");			/* Security */

		if (strstr(p, "WPA2"))
		{
			scan_res_array[scan_res_num].security_type = 3;    /* WPA2 */
		}
		else if (strstr(p, "WPA"))
		{
			scan_res_array[scan_res_num].security_type = 2;    /* WPA */
		}
		else if (strstr(p, "WEP"))
		{
			scan_res_array[scan_res_num].security_type = 1;    /* WEP */
		}
		else
		{
			scan_res_array[scan_res_num].security_type = 0;    /* OPEN */
		}

		p = strtok(NULL, "\n");			/* SSID */

		/* Discard hidden SSID. */
		if (p[0] == HIDDEN_SSID_DETECTION_CHAR)
		{
			continue;
		}

		memcpy(scan_res_array[scan_res_num].ssid, p, strlen(p));
		DBG_INFO("[%s:%d] (%d) %s / %d / %d / %d \n", __func__, __LINE__,
			   scan_res_num,
			   scan_res_array[scan_res_num].ssid,
			   scan_res_array[scan_res_num].security_type,
			   scan_res_array[scan_res_num].signal_strength,
			   scan_res_array[scan_res_num].freq);
		scan_res_num++;
	}

	if (scan_res_text)
	{
		MEMFREE(scan_res_text);
	}

	memset(wifi_scan_result.scan_result_str, '\0', SCAN_RESULT_BUF_SIZE);
	wifi_scan_result.scan_result_str_len = 0;
	length = 0;

	root = cJSON_CreateArray();
	if (root) {
	    for (int i = 0; i < scan_res_num; i++)
        {
            scan_res_json = cJSON_CreateObject();
            if (scan_res_json) {
                cJSON_AddStringToObject(scan_res_json, "SSID", scan_res_array[i].ssid);
                cJSON_AddNumberToObject(scan_res_json, "security_type", scan_res_array[i].security_type);
                cJSON_AddNumberToObject(scan_res_json, "signal_strength",
                                        scan_res_array[i].signal_strength);
                cJSON_AddItemToArray(root, scan_res_json);
            }
        }

        buf = cJSON_Print(root);
        if (buf) {
            length = strlen(buf);
            if (length <= SCAN_RESULT_BUF_SIZE)
            {
                memcpy((char *)wifi_scan_result.scan_result_str, (char *)buf, length);
                wifi_scan_result.scan_result_str_len = length;
            }
            else
            {
                length = 0; /* ERROR */
            }
            MEMFREE(buf);
        }

        cJSON_Delete(root);
	}

	MEMFREE(scan_res_array);
	return length;
}


/* API return values for JSON.  */
#define JSON_SUCCESS			0
#define JSON_FAILED				1
#define JSON_BAD_SEQUENCE		2
#define JSON_BAD_COMMAND		3
#define JSON_UNKNOWN_VERSION	4
#define JSON_SAME_VERSION		5
#define JSON_INCOMPATI_VERSION	6
#define JSON_BASE64_ERROR		7
#define JSON_CRC_ERROR			8
#define JSON_NOT_FOUND_BODY		9

uint8_t Flag_Hidden_AP = 0;

int32_t wifi_conf_parse_json(int8_t *string, uint32_t *cmd)
{
	cJSON *root = NULL;
	cJSON *obj = NULL;

	char *buf = NULL;
	int8_t *string_ptr = NULL;
	int32_t ret = JSON_FAILED;
	int32_t psk_len = 0;

	*cmd = COMBO_WIFI_CMD_UNKNOWN;

	if (string == NULL)
	{
		return JSON_FAILED;
	}

	string_ptr = string;

	buf = strstr((char *)string_ptr, "{");

	if (buf == NULL)
	{
		buf = strstr((char *)string_ptr, "\r\n\r\n{"); /* 0x0d 0x0a 0x0d 0x0a*/

		if (buf == NULL)
		{
			return JSON_NOT_FOUND_BODY;
		}
		else
		{
			buf += 4;    /* 0x0d 0x0a 0x0d 0x0a*/
		}
	}

	root = cJSON_Parse(buf);

	if (root == NULL)
	{
		return JSON_FAILED;
	}

	obj = cJSON_GetObjectItem(root, "dialog_cmd");

	if ((obj == NULL) || strlen(obj->valuestring) <= 0)
	{
		DBG_ERR(" UNKNOWN COMMAND\n");
		goto finish;
	}

	if (strcmp(obj->valuestring, "scan") == 0)
	{
		DBG_INFO(" Receive - SCAN \n");
		*cmd = COMBO_WIFI_CMD_SCAN_AP;
		ret = JSON_SUCCESS;

	}
	else if (strcmp(obj->valuestring, "factory_reset") == 0)
	{
		DBG_INFO(" Receive - Factory Reset \n");
		*cmd = COMBO_WIFI_CMD_FACT_RST;
		ret = JSON_SUCCESS;
	}
	else if (strcmp(obj->valuestring, "wifi_status") == 0)
	{
		DBG_INFO(" Receive - wifi_status \n");
		*cmd = COMBO_WIFI_CMD_INQ_WIFI_STATUS;
		ret = JSON_SUCCESS;
	}
	else if (strcmp(obj->valuestring, "disconnect") == 0)
	{
		DBG_INFO(" Receive - disconnect \n");
		*cmd = COMBO_WIFI_CMD_DISCONN;
		ret = JSON_SUCCESS;
	}
	else if (strcmp(obj->valuestring, "reboot") == 0)
	{
		DBG_INFO(" Receive - reboot \n");
		*cmd = COMBO_WIFI_CMD_REBOOT;
		ret = JSON_SUCCESS;
	}	
	else if (strcmp(obj->valuestring, "select_ap") == 0)
	{
		DBG_INFO(" Receive - SELECT_AP \n");
		*cmd = COMBO_WIFI_CMD_SELECT_AP;

		/* SSID */
		obj = cJSON_GetObjectItem(root, "SSID");

		if ((obj == NULL) || strlen(obj->valuestring) <= 0)
		{
			goto finish;
		}

		memset(wifi_conf.ssid, 0, sizeof(wifi_conf.ssid));
		memcpy(wifi_conf.ssid, obj->valuestring, strlen(obj->valuestring));

		/* security_type */
		obj = cJSON_GetObjectItem(root, "security_type");

		if (obj == NULL)
		{
			goto finish;
		}

		wifi_conf.security_type = obj->valueint;

		if (wifi_conf.security_type == 1 || wifi_conf.security_type == 2
				|| wifi_conf.security_type == 3)
		{

			/* password */
			obj = cJSON_GetObjectItem(root, "password");

			if ((obj == NULL) || (strlen(obj->valuestring) <= 0))
			{
				goto finish;
			}

			memset(wifi_conf.password, 0, sizeof(wifi_conf.password));
			memcpy(wifi_conf.password, obj->valuestring, strlen(obj->valuestring));

			psk_len = strlen(wifi_conf.password);

			if (psk_len <= 0)
			{
				goto finish;
			}

			if (wifi_conf.security_type == 1)
			{
				/* WEP */
				if (psk_len == 5 || psk_len == 13)  		/* ASCII */
				{
					;
				}
				else if (psk_len == 10 || psk_len == 26)  	/* HEXA */
				{
					int keyindex = 0;//, hexErr = 0;

					for (keyindex = 0; keyindex < psk_len; keyindex++)
					{
						if (isxdigit((int)wifi_conf.password[keyindex]) == 0)
						{
							goto finish;
						}
					}
				}
				else
				{
					goto finish;
				}
			}
		}

		/* Hidden SSID use set */
		obj = cJSON_GetObjectItem(root, "isHidden");        

		if (obj == NULL)
		{
			goto finish;
		}

		if ( obj->valueint == 1)
		{				
			DBG_INFO(" Hidden_SSID_Set\n");
            Flag_Hidden_AP = 1;	           
		}
		else{
		    Flag_Hidden_AP = 0;
		}

		ret = JSON_SUCCESS;
	}
#if defined (__SUPPORT_BLE_PROVISIONING__)
	else if (strcmp(obj->valuestring, "network_info") == 0)
	{
		DBG_INFO(" Receive - network_info \n");
		*cmd = COMBO_WIFI_CMD_SET_NETWORK_INFO;	
		
		wifi_conf.dhcp = 1; // dhcp enable fix

		obj = cJSON_GetObjectItem(root, "ping_addr");

		if ((obj == NULL) || (strlen(obj->valuestring) <= 0))
		{
			goto finish;
		}

		memset(wifi_conf.ping_addr, 0, sizeof(wifi_conf.ping_addr));
		strcpy(wifi_conf.ping_addr, obj->valuestring);

		/* server ip address */
		obj = cJSON_GetObjectItem(root, "svr_addr");

		if ((obj == NULL) || strlen(obj->valuestring) <= 0)
		{
			goto finish;
		}

		memset(wifi_conf.srv_ip, 0, sizeof(wifi_conf.srv_ip));
		strcpy(wifi_conf.srv_ip, obj->valuestring);


		/* server port */
		obj = cJSON_GetObjectItem(root, "svr_port");

		if (obj == NULL)
		{
			goto finish;
		}

		wifi_conf.srv_port = obj->valueint;

		/* user server URL */   // move from set dpm
		obj = cJSON_GetObjectItem(root, "svr_url");

		if (obj == NULL)
		{
			goto finish;
		}

		memset(wifi_conf.svr_url, 0, sizeof(wifi_conf.svr_url));
		if (obj != NULL && strlen(obj->valuestring) > 0)
		{
			strcpy(wifi_conf.svr_url, obj->valuestring);
		}

		ret = JSON_SUCCESS;
#ifdef DEBUG_SUPPORT_PROVISION
		PRINTF("\n=========================================\n");
		test_print("ssid", wifi_conf.ssid, 50);
		PRINTF("security_type = %d\n", wifi_conf.security_type);
		PRINTF("dhcp = %d\n", wifi_conf.dhcp);
		test_print("password", wifi_conf.password, 90);
		test_print("ipv4_adress", wifi_conf.ipv4_address, 15);
		test_print("mask", wifi_conf.mask, 15);
		test_print("gateway", wifi_conf.gateway, 15);
		test_print("dns_address", wifi_conf.dns_address, 15);
		PRINTF("=========================================\n"\n);
#endif

	}
	else if (strcmp(obj->valuestring, "set_dpm") == 0)
	{
		DBG_INFO(" Receive - set_dpm \n");
		*cmd = COMBO_WIFI_CMD_SET_DPM_INFO;

		/* user server URL */
		obj = cJSON_GetObjectItem(root, "svr_url");

		if (obj == NULL)
		{
			DBG_INFO("%s:%d ---- user svr_url null\n", __func__, __LINE__);
		}

		memset(wifi_conf.svr_url, 0, sizeof(wifi_conf.svr_url));
		if (obj != NULL && strlen(obj->valuestring) > 0)
		{
			strcpy(wifi_conf.svr_url, obj->valuestring);
		}
		DBG_INFO("%s:%d ---- user svr_url = %s\n", __func__, __LINE__, wifi_conf.svr_url);
		
		/* sleep mode */
		obj = cJSON_GetObjectItem(root, "sleepMode");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- sleepMode null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.sleep_mode = obj->valueint;

		DBG_INFO("%s:%d ---- sleep_mode = %d\n", __func__, __LINE__, wifi_conf.sleep_mode);
		/* rtc timer interval (sec) */
		obj = cJSON_GetObjectItem(root, "rtcTimer");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- rtcTimer null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.rtc_timer_sec = obj->valueint;
		DBG_INFO("%s:%d ---- rtc_timer_sec = %d\n", __func__, __LINE__, wifi_conf.rtc_timer_sec);

		/* use flag for dpm */
		obj = cJSON_GetObjectItem(root, "useDPM");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- useDPM null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.use_dpm = obj->valueint;
		wifi_conf.dpm_mode = wifi_conf.use_dpm;

		DBG_INFO("%s:%d ---- dpm mode = %d\n", __func__, __LINE__, wifi_conf.dpm_mode);

		/* dpm keepalive */
		obj = cJSON_GetObjectItem(root, "dpmKeepAlive");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- dpmKeepAlive null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.dpm_keep_alive = obj->valueint;

		DBG_INFO("%s:%d ---- dpm keep alive = %d\n", __func__, __LINE__, wifi_conf.dpm_keep_alive);
		
		/* user wakeup */
		obj = cJSON_GetObjectItem(root, "userWakeup");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- user wakeup null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.user_wakeup = obj->valueint;

		DBG_INFO("%s:%d ---- user wakeup = %d\n", __func__, __LINE__, wifi_conf.user_wakeup);
		
		/* tim wakeup */
		obj = cJSON_GetObjectItem(root, "timWakeup");

		if (obj == NULL)
		{
			DBG_ERR("%s:%d ---- timWakeup null\n", __func__, __LINE__);
			goto finish;
		}

		wifi_conf.tim_wakeup = obj->valueint;

		DBG_INFO("%s:%d ---- tim wakeup = %d\n", __func__, __LINE__, wifi_conf.tim_wakeup);
						
		setSleepModeDtim(wifi_conf.sleep_mode, wifi_conf.use_dpm, wifi_conf.rtc_timer_sec,
							wifi_conf.dpm_keep_alive, wifi_conf.user_wakeup, wifi_conf.tim_wakeup);
		ret = JSON_SUCCESS;

	}
	else if (strcmp(obj->valuestring, "chk_network") == 0)
	{
		DBG_INFO(" Receive - chk_network \n");
		*cmd = COMBO_WIFI_CMD_CHK_NETWORK;

		ret = JSON_SUCCESS;
	}
#endif // __SUPPORT_BLE_PROVISIONING__

	else if (strcmp(obj->valuestring, "fw_update") == 0)
	{
		DBG_INFO(" Receive - FW_UPDATE \n");
		*cmd = COMBO_WIFI_CMD_FW_UPDATE;
		ret = JSON_SUCCESS;
	}
	else
	{
		DBG_INFO(" Receive - UNKNOWN \n");
	}

finish:

	cJSON_Delete(root);
	return ret;
}


int32_t wifi_sta_select_ap(void)
{
	cJSON *root = NULL;
	int ret = JSON_SUCCESS;
	int psk_len = 0;

	delete_nvram_env(NVR_KEY_SSID_0);
	delete_nvram_env(NVR_KEY_ENCKEY_0);

	if (strlen(wifi_conf.ssid) > 0)
	{
		ret += da16x_set_config_str(DA16X_CONF_STR_SSID_0, wifi_conf.ssid);
		ret += da16x_set_config_int(DA16X_CONF_INT_AUTH_MODE_0, wifi_conf.security_type);

		if (wifi_conf.security_type == 1)
		{
			/* WEP */
			psk_len = strlen(wifi_conf.password);

			if (psk_len > 0 )
			{
				if (psk_len == 5 || psk_len == 13)  		/* ASCII */
				{
					ret += da16x_set_config_str(DA16X_CONF_STR_WEP_KEY0, wifi_conf.password);
				}
				else if (psk_len == 10 || psk_len == 26)  	/* HEXA */
				{
					int keyindex, hexErr = 0;

					for (keyindex = 0; keyindex < psk_len; keyindex++)
					{
						if (isxdigit((int)wifi_conf.password[keyindex]) == 0)
						{
							hexErr += 1;
							break;
						}
					}

					if (hexErr == 0)
					{
						ret += da16x_set_config_str(DA16X_CONF_STR_WEP_KEY0, wifi_conf.password);
					}
				}
			}
			else
			{
				ret += 1;
			}

		}
		else if (wifi_conf.security_type == 2)
		{
			/* WPA */
			ret += da16x_set_config_str(DA16X_CONF_STR_PSK_0, wifi_conf.password);
		}
		else if (wifi_conf.security_type == 3)
		{
			/* WPA2 */
			ret += da16x_set_config_str(DA16X_CONF_STR_PSK_0, wifi_conf.password);
		}

		ret += da16x_set_config_int(DA16X_CONF_INT_DHCP_CLIENT, wifi_conf.dhcp);
		ret += da16x_set_config_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_AUTO);

		if (wifi_conf.dhcp == 0)
		{
			ret += da16x_set_config_str(DA16X_CONF_STR_IP_0, wifi_conf.ipv4_address);
			ret += da16x_set_config_str(DA16X_CONF_STR_NETMASK_0, wifi_conf.mask);
			ret += da16x_set_config_str(DA16X_CONF_STR_GATEWAY_0, wifi_conf.gateway);
			ret += da16x_set_config_str(DA16X_CONF_STR_DNS_0, wifi_conf.dns_address);
		}

		ret += da16x_set_config_int(DA16X_CONF_INT_MODE, 0);

		// save custom data in nvram - demo only
		write_nvram_string((const char *)"SERVER_IP", wifi_conf.srv_ip);
		write_nvram_int((const char *)"SERVER_PORT", wifi_conf.srv_port);
		if (Flag_Hidden_AP) {
		    da16x_cli_reply("set_network 0 scan_ssid 1", NULL, NULL);
		}
        
		da16x_cli_reply("save_config", NULL, NULL);

		// if DPM mode is enabled, enable it
		if (wifi_conf.dpm_mode)
		{
#if !defined (__SUPPORT_BLE_PROVISIONING__)
			dpm_mode_enable();
#else //added for combo
			DBG_INFO(" Skipped dpm_mode_enable in BLE Provision \n");
#endif
		}
		else
		{
			dpm_mode_disable();
		}
	}

	ret = JSON_SUCCESS;

	//finish:
	cJSON_Delete(root);
	return ret;
}


char *get_wifi_sta_scan_result_str(void)
{
	return wifi_scan_result.scan_result_str;
}
uint32_t get_wifi_sta_scan_result_str_len(void)
{
	return wifi_scan_result.scan_result_str_len;
}

void set_wifi_sta_scan_result_str_idx(uint32_t idx)
{
	wifi_scan_result.scan_result_str_idx = idx;
	return;
}
uint32_t get_wifi_sta_scan_result_str_idx(void)
{
	return wifi_scan_result.scan_result_str_idx;
}

#if defined (__SUPPORT_BLE_PROVISIONING__)
void clear_wifi_provisioning_result_str(void)
{
	wifi_prov_result.command_result = 0;
	wifi_prov_result.len_of_result = 0;
	memset(wifi_prov_result.prov_result_str, '\0', PROVISIONING_RESULT_BUF_SIZE);
}

uint32_t get_wifi_provisioning_command_result(void)
{
	return wifi_prov_result.command_result;
}

uint32_t get_wifi_provisioning_result_len(void)
{
	return wifi_prov_result.len_of_result;
}

char *get_wifi_provisioning_result_str(void)
{
	return wifi_prov_result.prov_result_str;
}

int32_t get_wifi_provisioning_dpm_mode(void)
{
	return wifi_conf.dpm_mode;
}

uint32_t wifi_svc_get_provisioning_flag(void)
{
	return flag_provisioning;
}

int32_t wifi_set_dpm_info(void)
{
	if (wifi_conf.dpm_mode == 1)
	{
		//write_tmp_nvram_int(NVR_KEY_DPM_MODE, wifi_conf.dpm_mode);

		if(app_get_DPM_set_value(TYPE_SLEEP_MODE)!=3)
		{

			if (wifi_conf.dpm_keep_alive > 0)
			{
				write_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, wifi_conf.dpm_keep_alive);
			}
			else
			{
				write_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, DFLT_DPM_KEEPALIVE_TIME);
			}
			if (wifi_conf.user_wakeup > 0)
			{
				write_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME,	wifi_conf.user_wakeup);
			}
			else
			{
				write_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME,	MIN_DPM_USER_WAKEUP_TIME);
			}
			if (wifi_conf.tim_wakeup > 0)
			{
				write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME,	wifi_conf.tim_wakeup);
			}
			else
			{
				write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME,	DFLT_DPM_TIM_WAKEUP_COUNT);
			}
		}else
		{
			PRINTF("[SleepMode 3] Set DPM , KA : %d , UW : %d , TW : %d \n", app_get_DPM_set_value(TYPE_DPM_KEEP_ALIVE),app_get_DPM_set_value(TYPE_USER_WAKE_UP),app_get_DPM_set_value(TYPE_TIM_WAKE_UP));
			write_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, app_get_DPM_set_value(TYPE_DPM_KEEP_ALIVE));
			write_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME,	app_get_DPM_set_value(TYPE_USER_WAKE_UP));
			write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME,	app_get_DPM_set_value(TYPE_TIM_WAKE_UP));
		}

	}
	else 
	{
		//write_nvram_int(NVR_KEY_DPM_MODE, 0);
		dpm_mode_disable();
	}

#if defined (__SUPPORT_AWS_IOT__)
	PRINTF(">>> Save Sleep mode to %d \n",app_get_DPM_set_value(TYPE_SLEEP_MODE));	 
	write_nvram_int(SLEEP_MODE_FOR_NARAM, app_get_DPM_set_value(TYPE_SLEEP_MODE));
	PRINTF(">>> Save Sleep mode2 RTC time %d \n",app_get_DPM_set_value(TYPE_RTC_TIME));
	write_nvram_int(SLEEP_MODE2_RTC_TIME, app_get_DPM_set_value(TYPE_RTC_TIME));
#endif // __SUPPORT_AWS_IOT__

	return 0;
}
#endif // __SUPPORT_BLE_PROVISIONING__

#endif /* __WFSVC_ENABLE__ */

/// @} APP

#endif /* __BLE_PERI_WIFI_SVC_TCP_DPM__ */

/* EOF */
