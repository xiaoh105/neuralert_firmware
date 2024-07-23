/**
 ****************************************************************************************
 *
 * @file wifi_svc_user_custom_profile.h
 *
 * @brief Header file for Wi-Fi SVC
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

#ifndef _USER_CUSTOM_PROFILE_WFSVC_H_
#define _USER_CUSTOM_PROFILE_WFSVC_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"
#include "gattm_task.h"
#ifndef BLE_CONNECTION_MAX
#warning "explicitly setting BLE_CONNECTION_MAX"
#define BLE_CONNECTION_MAX
#endif
#include "gattc_task.h"
#include "../../../../../../core/ble_interface/ble_inc/platform/core_modules/common/api/co_error.h"
#include "att.h"
#include "attm_db_128.h"
#include "ble_msg.h"
#include "gatt_msgs.h"

#include "gatt_msgs.h"
#include "gattm_task.h"


#include "provision_config.h"
#include "da16_gtl_features.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/**
uuid:

Wi-Fi service				9161b201-1b4b-4727-a3ca-47b35cdcf5c1
	Wi-Fi cmd 				9161b202-1b4b-4727-a3ca-47b35cdcf5c1
	Wi-Fi act result report	9161b203-1b4b-4727-a3ca-47b35cdcf5c1
	AP Scan result			9161b204-1b4b-4727-a3ca-47b35cdcf5c1
*/

// uuid for Wi-Fi service:                       9161b201-1b4b-4727-a3ca-47b35cdcf5c1
#define DEF_CUSTOM_SERVICE_WFSVC_UUID_128       		  {0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x01, 0xb2, 0x61, 0x91}

// uuid for Wi-Fi cmd characteristic (value) :            9161b202-1b4b-4727-a3ca-47b35cdcf5c1
#define DEF_CUSTOM_CHAR_WFSVC_WFCMD_UUID_128              {0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x02, 0xb2, 0x61, 0x91}

// uuid for Wi-Fi act result characteristic (value) :     9161b203-1b4b-4727-a3ca-47b35cdcf5c1
#define DEF_CUSTOM_CHAR_WFSVC_WFACT_RES_UUID_128          {0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x03, 0xb2, 0x61, 0x91}

// uuid for AP Scan Result characteristic (value) :       9161b204-1b4b-4727-a3ca-47b35cdcf5c1
#define DEF_CUSTOM_CHAR_WFSVC_APSCAN_RES_UUID_128         {0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x04, 0xb2, 0x61, 0x91}

#if defined (__SUPPORT_BLE_PROVISIONING__)
// uuid for provisioning test result characteristic (value) : 9161b205-1b4b-4727-a3ca-47b35cdcf5c1
#define DEF_CUSTOM_CHAR_WFSVC_PROVISIONING_RES_UUID_128   {0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x05, 0xb2, 0x61, 0x91}
#endif // __SUPPORT_BLE_PROVISIONING__

// characteristic value max length
#define DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD (DEMO_PEER_MAX_MTU-ATT_HEADER_SIZE)
#define DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_WFCMD_RES 	(2)
#define DEF_CUSTOM_CHAR_VALUE_HEADER_LENGTH_WFSVC_APSCAN_RES_SIZE	(sizeof(uint16_t) + sizeof(uint16_t))
#define DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES 	(DEMO_PEER_MAX_MTU-ATT_HEADER_SIZE)
#if defined (__SUPPORT_BLE_PROVISIONING__)
#define DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_PROVISIONING_RES         (DEMO_PEER_MAX_MTU-ATT_HEADER_SIZE)
#endif  //__SUPPORT_BLE_PROVISIONING__

#define WFSVC_DESC_MAX_LEN (26)

typedef struct
{
	uint16_t    value[DEF_CUSTOM_CHAR_VALUE_MAX_LENGTH_WFSVC_APSCAN_RES / 2];
	uint16_t    desc[WFSVC_DESC_MAX_LEN / 2];
	uint16_t    conf;
} local_wfsvc_values_t;

enum CUSTOM_CHAR_WFSVC_ATT_IDX
{
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_DECL_IDX = 0,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_VALUE_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFCMD_CHAR_USER_DESC_IDX,

	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_DECL_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_VALUE_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_CONF_DESC_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_WFACT_RES_CHAR_USER_DESC_IDX,

	USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_DECL_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_VALUE_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_CONF_DESC_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_APSCAN_RES_CHAR_USER_DESC_IDX,

#if defined (__SUPPORT_BLE_PROVISIONING__)
	USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_DECL_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_VALUE_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_CONF_DESC_IDX,
	USER_CUSTOM_CHARACTERISTIC_WFSVC_PROVISIONING_RES_CHAR_USER_DESC_IDX,
#endif //__SUPPORT_BLE_PROVISIONING__

	CUSTOM_DB_WFSVC_B_IDX_NB
};

enum WIFI_CMD
{
	COMBO_WIFI_CMD_SCAN_AP = 1,
	COMBO_WIFI_CMD_SELECT_AP,

	COMBO_WIFI_CMD_FW_UPDATE,

	COMBO_WIFI_CMD_FACT_RST,

	COMBO_WIFI_CMD_INQ_WIFI_STATUS,

	COMBO_WIFI_CMD_DISCONN,
	
	COMBO_WIFI_CMD_REBOOT,	

#if defined (__SUPPORT_BLE_PROVISIONING__)	
	COMBO_WIFI_CMD_SET_DPM_INFO,
	COMBO_WIFI_CMD_CHK_NETWORK,
	COMBO_WIFI_CMD_SET_NETWORK_INFO,
#endif // __SUPPORT_BLE_PROVISIONING__	

	COMBO_WIFI_CMD_UNKNOWN
};

// Wi-Fi Action Result
enum WIFI_ACTION_RESULT
{
	COMBO_WIFI_CMD_SCAN_AP_SUCCESS = 1,
	COMBO_WIFI_CMD_SCAN_AP_FAIL,

	COMBO_WIFI_CMD_FW_BLE_DOWNLOAD_SUCCESS,
	COMBO_WIFI_CMD_FW_BLE_DOWNLOAD_FAIL,

	COMBO_WIFI_CMD_INQ_WIFI_STATUS_CONNECTED,
	COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED,

	COMBO_WIFI_PROV_DATA_VALIDITY_CHK_ERR, // feedback for Provisioning data write
	COMBO_WIFI_PROV_DATA_SAVE_SUCCESS,

	COMBO_WIFI_CMD_MEM_ALLOC_FAIL,
#if defined (__SUPPORT_WIFI_CONN_FAIL_BY_WK_IND__)
	COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED_BY_WRONG_KEY,
#endif

#if defined (__SUPPORT_BLE_PROVISIONING__)
	COMBO_WIFI_CMD_ACK = 100,
	COMBO_WIFI_CMD_SELECT_AP_SUCCESS = 101,
	COMBO_WIFI_CMD_SELECT_AP_FAIL = 102,
	COMBO_WIFI_PROV_WRONG_PW = 103,
	COMBO_WIFI_PROV_NETWORK_INFO = 104,
	COMBO_WIFI_PROV_AP_FAIL = 105,
	COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_FAIL = 106,
	COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_OK = 107, // in case wrong DNS name
	COMBO_WIFI_PROV_NO_URL_PING_FAIL = 108,
	COMBO_WIFI_PROV_NO_URL_PING_OK = 109,
	COMBO_WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK = 110, // in case AP gives wrong IP
	COMBO_WIFI_PROV_DNS_OK_PING_OK = 111,
	COMBO_WIFI_PROV_REBOOT_ACK = 112,
	COMBO_WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL = 113, // in case default ping fail
#endif // __SUPPORT_BLE_PROVISIONING__

	COMBO_WIFI_CMD_UNKNOWN_RCV
};

#define CUSTOM_SERVICE_WFSVC_NB_ATTS CUSTOM_DB_WFSVC_B_IDX_NB

#define WFSVC_ENABLE_NOTIFY 0x01
#define WFSVC_DISABLE_NOTIFY 0x00

/**
 * Service description
 */
struct gattm_svc_desc_custom_wfsvc_t
{
	/// Attribute Start Handle (0 = dynamically allocated)
	uint16_t start_hdl;
	/// Task identifier that manages service
	uint16_t task_id;

	/**
	 *  7    6    5    4    3    2    1    0
	 * +----+----+----+----+----+----+----+----+
	 * |RFU | P  |UUID_LEN |  AUTH   |EKS | MI |
	 * +----+----+----+----+----+----+----+----+
	 *
	 * Bit [0]  : Task that manage service is multi-instantiated (Connection index is conveyed)
	 * Bit [1]  : Encryption key Size must be 16 bytes
	 * Bit [2-3]: Service Permission      (0 = Disable, 1 = Enable, 2 = UNAUTH, 3 = AUTH)
	 * Bit [4-5]: UUID Length             (0 = 16 bits, 1 = 32 bits, 2 = 128 bits, 3 = RFU)
	 * Bit [6]  : Primary Service         (1 = Primary Service, 0 = Secondary Service)
	 * Bit [7]  : Reserved for future use
	 */
	uint8_t perm;

	/// Number of attributes
	uint8_t nb_att;

	/** Service  UUID */
	uint8_t uuid[ATT_UUID_128_LEN];
	/**
	 * List of attribute description present in service.
	 */

	/* To be checked */

	#if RWBLE_SW_VERSION_MAJOR >= 8
	    uint16_t __padding__;
	#endif

	//*/

#pragma pack(push, 1)
	struct gattm_att_desc atts[CUSTOM_SERVICE_WFSVC_NB_ATTS];
};
#pragma pack(pop)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
extern char wfsvc_wfcmd_desc[];
extern char wfsvc_wfact_res_desc[];
extern char wfsvc_apscan_res_desc[];
#if defined (__SUPPORT_BLE_PROVISIONING__)
extern char wfsvc_provisioning_res_desc[];
#endif // __SUPPORT_BLE_PROVISIONING__

#define LEN_WFSVC_WFCMD_DESC      	strlen(wfsvc_wfcmd_desc)
#define LEN_WFSVC_WFACT_RES_DESC        strlen(wfsvc_wfact_res_desc)
#define LEN_WFSVC_APSCAN_RES_DESC       strlen(wfsvc_apscan_res_desc)
#if defined (__SUPPORT_BLE_PROVISIONING__)
#define LEN_WFSVC_PROVISIONING_RES_DESC strlen(wfsvc_provisioning_res_desc)
#endif // __SUPPORT_BLE_PROVISIONING__

typedef struct
{
	uint16_t start_handle;
	uint16_t end_handle;
	bool demo_disable_service_until_system_reset;
} user_custom_profile_wfsvc_env_t;

/*
 * ATTRIBUTES
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

// void user_custom_profile_wfsvc_db_create(void);

void user_custom_profile_wfsvc_enable(uint16_t conhdl);

uint8_t user_custom_profile_wfsvc_write_cmd_ind_xxx(ke_msg_id_t const msgid,
												struct gattc_write_req_ind const *param,
												ke_task_id_t const dest_id,
												ke_task_id_t const src_id);

uint8_t user_custom_profile_wfsvc_att_set_value_rsp_hnd(ke_msg_id_t const msgid,
											struct gattm_att_set_value_rsp const *param,
											ke_task_id_t const dest_id,
											ke_task_id_t const src_id);

uint8_t user_custom_profile_wfsvc_gattm_svc_set_permission_rsp_hnd(
										ke_msg_id_t const msgid,
										struct gattm_svc_set_permission_rsp const *param,
										ke_task_id_t const dest_id,
										ke_task_id_t const src_id);
#endif

/* EOF */
