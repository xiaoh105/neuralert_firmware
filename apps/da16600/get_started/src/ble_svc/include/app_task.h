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
#include "../../../../../../core/ble_interface/ble_inc/platform/core_modules/ke/api/ke_task.h"
#include "../../../../../../core/ble_interface/ble_inc/platform/core_modules/ke/api/ke_msg.h"
#include "gapm_task.h"
#include "gapc_task.h"
#include "proxr_task.h"
#include "diss_task.h"
#include "da16_gtl_features.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// application states
enum
{
	/// Idle state
	APP_IDLE,
	/// Scanning state
	APP_CONNECTABLE,
	/// Connected state
	APP_CONNECTED,
	/// Number of defined states.
	APP_STATE_MAX,
	/// Scanning state
	APP_SCAN,
};

#if defined (__LOW_POWER_IOT_SENSOR__)
/// iot sensor state
enum IOT_SENSOR_STATE
{
	IOT_SENSOR_INACTIVE,
	IOT_SENSOR_ACTIVE,
};
#endif /* __LOW_POWER_IOT_SENSOR__ */

typedef enum
{
	APP_NO_ERROR = 0x00,
	APP_ERROR_INVALID_PARAMS,
	APP_ERROR_INVALID_SIZE,
} custom_command_status_t;

// iot_sensor demo purpose
enum APP_GAS_LEAK_STATUS
{
	APP_GAS_LEAK_NOT_OCCURRED,
	APP_GAS_LEAK_OCCURRED
};

typedef struct app_gas_leak_sensor_start_cfm
{
	custom_command_status_t status;
}
app_gas_leak_sensor_start_cfm_t;


typedef struct app_gas_leak_sensor_stop_cfm
{
	custom_command_status_t status;
}
app_gas_leak_sensor_stop_cfm_t;


typedef struct app_gas_leak_evt_ind
{
	uint8_t evt_status;
}
app_gas_leak_evt_ind_t;

typedef struct app_fw_version_ind
{
    uint8_t app_fw_version[APP_FW_VERSION_STRING_MAX_LENGTH];
}
app_fw_version_ind_t;

#ifdef __BLE_PERI_WIFI_SVC_PERIPHERAL__
// blinky
typedef struct app_peri_blinky_start_ind
{
    uint8_t result_code;
} app_peri_blinky_start_ind_t;

// systick
typedef struct app_peri_systick_start_ind
{
    uint8_t result_code;
} app_peri_systick_start_ind_t;


typedef struct app_peri_systick_stop_ind
{
    uint8_t result_code;
} app_peri_systick_stop_ind_t;

// timer0_gen
typedef struct app_peri_timer0_gen_start_ind
{
    uint8_t result_code;
} app_peri_timer0_gen_start_ind_t;

// timer0_buz
typedef struct app_peri_timer0_buz_start_ind
{
    uint8_t result_code;
} app_peri_timer0_buz_start_ind_t;

// timer2_pwm
typedef struct app_peri_timer2_pwm_start_ind
{
    uint8_t result_code;
} app_peri_timer2_pwm_start_ind_t;

// batt_lvl
/// Supported battery types
typedef enum
{
    /// CR2032 lithium cell battery
    BATT_CR2032   = 0,

    /// Alkaline cell battery
    BATT_ALKALINE = 1
} batt_t;
	
typedef struct app_peri_batt_lvl_ind
{
    uint8_t batt_lvl;
	batt_t batt_type;
} app_peri_batt_lvl_ind_t;

// i2c_eeprom
typedef struct app_peri_i2c_eeprom_start_ind
{
    uint8_t result_code;
} app_peri_i2c_eeprom_start_ind_t;

// spi_flash
typedef struct app_peri_spi_flash_start_ind
{
    uint8_t result_code;
} app_peri_spi_flash_start_ind_t;

// quad_dec
typedef struct app_peri_quad_dec_start_ind
{
    uint8_t result_code;
} app_peri_quad_dec_start_ind_t;

typedef struct app_peri_quad_dec_stop_ind
{
    uint8_t result_code;
} app_peri_quad_dec_stop_ind_t;

#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
typedef enum {
    APP_GTL_GPIO_STATUS_LOW = 0x00,
    APP_GTL_GPIO_STATUS_HIGH,
    APP_GTL_GPIO_STATUS_HANDLED,
    APP_GTL_GPIO_STATUS_NOT_IN_USED,
    APP_GTL_GPIO_STATUS_INVALID,
} app_gtl_gpio_status_t;

// gpio set/get
typedef struct app_peri_gpio_cmd_ind
{
    uint8_t result_code;
} app_peri_gpio_cmd_ind_t;
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
#endif

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

struct gap_ready_evt
{
	// to resolve a compile error in IAR. in GCC, there's no compiler error for this.
	short dummy; 
};

extern struct app_env_tag app_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
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
void app_find_device_name(unsigned char *adv_data, unsigned char adv_data_len,
						  unsigned char dev_indx);
/**
 ****************************************************************************************
 * @brief Handles GAPM_CMP_EVT event for GAPM_SET_DEV_CONFIG_CMD.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_set_dev_config_completion_handler(ke_msg_id_t msgid,
		struct gapm_cmp_evt *param,
		ke_task_id_t dest_id,
		ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP.
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
								  struct gap_ready_evt *param,
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
 * @brief Handles Connection request indication event.
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
 * @brief Handles Disconnection indication event.
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
 * @brief Handles RSSI indication event.
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


int gapc_le_pkt_size_ind_handler(ke_msg_id_t const msgid,
								 struct gapc_le_pkt_size_ind *param,
								 ke_task_id_t const dest_id,
								 ke_task_id_t const src_id);


/**
 ****************************************************************************************
 * @brief Handle reset GAP request completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_reset_completion_handler(ke_msg_id_t msgid,
								  struct gapm_cmp_evt *param,
								  ke_task_id_t  dest_id,
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
 * @brief Handles the GAPC_BOND_REQ_IND event.
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

/**
 ****************************************************************************************
 * @brief Handles the GAPC_ENCRYPT_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
								 struct gapc_encrypt_req_ind *param,
								 ke_task_id_t const dest_id,
								 ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles the GAPC_ENCRYPT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
							 struct gapc_encrypt_ind *param,
							 ke_task_id_t const dest_id,
							 ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles the DISS_VALUE_REQ_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
* @return void.
*****************************************************************************************
*/
void diss_value_req_ind_handler(ke_msg_id_t const msgid,
								struct diss_value_req_ind *param,
								ke_task_id_t const dest_id,
								ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles the PROXR_ALERT_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
* @return void.
*****************************************************************************************
*/
void proxr_alert_ind_handler(ke_msg_id_t msgid,
							 struct proxr_alert_ind  *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handles the APP_GAS_LEAK_SENSOR_START_CFM sent by ble (gtl peer).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
* @return void.
*****************************************************************************************
*/
int app_sensor_start_cfm_hnd(ke_msg_id_t msgid,
							 app_gas_leak_sensor_start_cfm_t *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles the APP_GAS_LEAK_SENSOR_STOP_CFM sent by ble (gtl peer).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
* @return void.
*****************************************************************************************
*/
int app_sensor_stop_cfm_hnd(ke_msg_id_t msgid,
							app_gas_leak_sensor_stop_cfm_t *param,
							ke_task_id_t dest_id,
							ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles the APP_GAS_LEAK_EVT_IND sent by ble (gtl peer).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
* @return void.
*****************************************************************************************
*/
int app_sensor_event_ind_hnd(ke_msg_id_t msgid,
							 app_gas_leak_evt_ind_t *param,
							 ke_task_id_t dest_id,
							 ke_task_id_t src_id);

/**
 ****************************************************************************************
 */
int app_fw_version_ind_handler (ke_msg_id_t msgid,
                                    app_fw_version_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

#ifdef __BLE_PERI_WIFI_SVC_PERIPHERAL__
// blinky
int app_peri_blinky_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_blinky_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// systick
int app_peri_systick_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_systick_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

int app_peri_systick_stop_ind_handler (ke_msg_id_t msgid,
                                    app_peri_systick_stop_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// timer0_gen
int app_peri_timer0_gen_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_timer0_gen_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// timer0_buz
int app_peri_timer0_buz_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_timer0_buz_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// timer0_pwm
int app_peri_timer2_pwm_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_timer2_pwm_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// batt_lvl
int app_peri_batt_lvl_ind_handler (ke_msg_id_t msgid,
                                    app_peri_batt_lvl_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// i2c_eeprom
int app_peri_i2c_eeprom_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_i2c_eeprom_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

// spi_flash
int app_peri_spi_flash_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_spi_flash_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);


// quad_dec
int app_peri_quad_dec_start_ind_handler (ke_msg_id_t msgid,
                                    app_peri_quad_dec_start_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

int app_peri_quad_dec_stop_ind_handler (ke_msg_id_t msgid,
                                    app_peri_quad_dec_stop_ind_t *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
// gpio set/get
int app_peri_gpio_cmd_ind_handler(ke_msg_id_t msgid,
				  app_peri_gpio_cmd_ind_t *param,
				  ke_task_id_t dest_id,
				  ke_task_id_t src_id);
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
#endif
#endif // APP_TASK_H_

/* EOF */
