/**
 ****************************************************************************************
 *
 * @file iot_sensor_user_custom_profile.h
 *
 * @brief Header file for the custom profile for IoT sensor service.
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

#ifndef COMBO_INCLUDE_IOT_SENSOR_USER_CUSTOM_PROFILE_H_
#define COMBO_INCLUDE_IOT_SENSOR_USER_CUSTOM_PROFILE_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "sdk_type.h"

#if defined (__IOT_SENSOR_SVC_ENABLE__)
#include "da14585_config.h"
#include "ke_msg.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

enum IOT_SENSOR_SVC_ATT_IDX
{
        IOT_SENSOR_SVC_TEMP_CHAR_DECL_IDX = 0,
        IOT_SENSOR_SVC_TEMP_CHAR_VALUE_IDX,
        IOT_SENSOR_SVC_TEMP_CHAR_CONF_DESC_IDX,
        IOT_SENSOR_SVC_TEMP_CHAR_USER_DESC_IDX,

        CUSTOM_DB_IOT_SENSOR_SVC_IDX_NB
};

#define IOT_SENSOR_SVC_NB_ATTS CUSTOM_DB_IOT_SENSOR_SVC_IDX_NB

/*
 ****************************************************************************************
 uuid:
 IoT Sensor service              12345678-1234-5678-1234-56789abcdef0
 Temp                            12345678-1234-5678-1234-56789abcdef1
 ****************************************************************************************
 */
// uuid for Iot Sensor service:                         12345678-1234-5678-1234-56789abcdef0
#define DEF_IOT_SENSOR_SVC_UUID_128                     {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12}
// uuid for Temp characteristic (value) :               12345678-1234-5678-1234-56789abcdef1
#define DEF_IOT_SENSOR_SVC_CHAR_TEMP_UUID_128           {0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12}
// characteristic value max length
#define DEF_IOT_SENSOR_SVC_CHAR_VALUE_MAX_LENGTH_TEMP         (1)

typedef void (*iot_sensor_scv_enabled_t)(bool enabled);

struct iot_sensor_svc_cb {
        iot_sensor_scv_enabled_t enabled;
        iot_sensor_scv_enabled_t notify_enabled;
};

struct iot_sensor_env_tag {
    uint16_t                    svc_handle;
    uint8_t                     svc_value;
    bool                        svc_value_notify_enabled;
    struct iot_sensor_svc_cb    *svc_callback;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the IoT sensor service environment
 ****************************************************************************************
 */
void iot_sensor_svc_init(void);

/**
 ****************************************************************************************
 * @brief Get the IoT sensor service environment
 ****************************************************************************************
 */
struct iot_sensor_env_tag *iot_sensor_svc_get_env(void);

/**
 ****************************************************************************************
 * @brief Create the database for the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_create_custom_db(uint16_t start_hdl);

/**
 ****************************************************************************************
 * @brief Set the handle of the database for the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_set_handle(uint16_t handle);

/**
 ****************************************************************************************
 * @brief Get the handle of the database for the IoT sensor service
 ****************************************************************************************
 */
uint16_t iot_sensor_svc_get_handle(void);

/**
 ****************************************************************************************
 * @brief Enable the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_enable(uint16_t conhdl, struct iot_sensor_svc_cb *callback);

/**
 ****************************************************************************************
 * @brief Disable the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_disable(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Set the temperature value
 ****************************************************************************************
 */
void iot_sensor_svc_set_temp(uint8_t value);

/**
 ****************************************************************************************
 * @brief Handler for the read request of the IoT sensor service
 ****************************************************************************************
 */
uint8_t iot_sensor_svc_gattc_read_req_ind(ke_msg_id_t const msgid,
        void *req_param, ke_task_id_t const dest_id,
        ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handler for the write request of the IoT sensor service
 ****************************************************************************************
 */
uint8_t iot_sensor_svc_gattc_write_req_ind(ke_msg_id_t const msgid,
        void *req_param, ke_task_id_t const dest_id,
        ke_task_id_t const src_id);

#endif // __IOT_SENSOR_SVC_ENABLE__

#endif /* COMBO_INCLUDE_IOT_SENSOR_USER_CUSTOM_PROFILE_H_ */

/* EOF */
