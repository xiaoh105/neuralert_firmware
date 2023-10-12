/**
 ****************************************************************************************
 *
 * @file iot_sensor_user_custom_profile.c
 *
 * @brief Implementation of the custom profile for IoT sensor service.
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

#include "../../include/da14585_config.h"
#include "../../include/iot_sensor_user_custom_profile.h"
#include "../../include/ble_msg.h"
#include "../../include/gatt_msgs.h"

#include "rwble_hl_config.h"
#include "rwble_config.h"
#include "att.h"
#include "attm.h"
#include "gattm_task.h"
#include "gattc_task.h"
#include "common_def.h"
#include "da16_gtl_features.h"
#include "FreeRTOSConfig.h"

#if defined (__IOT_SENSOR_SVC_ENABLE__)

#include "da16x_compile_opt.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */

#define IOT_SENSOR_SVC_GET_HANDLE(idx)      (iot_sensor_env.svc_handle + 1 + idx)
#define IOT_SENSOR_SVC_GET_IDX(handle)      (handle - (iot_sensor_env.svc_handle + 1))

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static const uint8_t iot_sensor_svc_uuid[ATT_UUID_128_LEN] = DEF_IOT_SENSOR_SVC_UUID_128;
static const char iotsensor_svc_temp_desc[] = "Temp";
static int iot_sensor_seq_num = 0;

static struct iot_sensor_env_tag iot_sensor_env;

// @formatter:off
static const struct gattm_att_desc iot_sensor_svc_atts[IOT_SENSOR_SVC_NB_ATTS] = {
        // --- CHARACTERISTIC TEMP ---
        // custom characteristic Temp Declaration
        [IOT_SENSOR_SVC_TEMP_CHAR_DECL_IDX] = {
                { (uint8_t)ATT_DECL_CHARACTERISTIC, ATT_DECL_CHARACTERISTIC >> 8 },
                PERM(RD,ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_16),
                0
        },

        // custom characteristic Temp Value attribute
        [IOT_SENSOR_SVC_TEMP_CHAR_VALUE_IDX] = {
                DEF_IOT_SENSOR_SVC_CHAR_TEMP_UUID_128,
                PERM_VAL(UUID_LEN, PERM_UUID_128) | PERM(RD, ENABLE) | PERM(NTF, ENABLE),
                PERM(RI, ENABLE) | DEF_IOT_SENSOR_SVC_CHAR_VALUE_MAX_LENGTH_TEMP
        },

        // custom characteristic Temp Configuration Descriptor (RI is Enforced)
        [IOT_SENSOR_SVC_TEMP_CHAR_CONF_DESC_IDX] = {
                {(uint8_t)ATT_DESC_CLIENT_CHAR_CFG, ATT_DESC_CLIENT_CHAR_CFG >> 8 },
                PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE),
                sizeof(ATT_DESC_CLIENT_CHAR_CFG)
        },

        // custom characteristic Temp User Description
        [IOT_SENSOR_SVC_TEMP_CHAR_USER_DESC_IDX] = {
                {(uint8_t)ATT_DESC_CHAR_USER_DESCRIPTION, ATT_DESC_CHAR_USER_DESCRIPTION >> 8 },
                PERM(RD, ENABLE),
                sizeof(iotsensor_svc_temp_desc)
        }
};
// @formatter:on


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void iot_sensor_svc_write_cfm(uint16_t handle, ke_task_id_t dest_id, ke_task_id_t src_id,
        uint8_t status)
{
        struct gattc_write_cfm *cfm = (struct gattc_write_cfm *)BleMsgAlloc(GATTC_WRITE_CFM, src_id, dest_id,
                sizeof(struct gattc_write_cfm));
        configASSERT(cfm);

        cfm->handle = handle;
        cfm->status = status;

        BleSendMsg((void*)cfm);
}

static void iot_sensor_svc_send_event(uint8_t value)
{
    struct gattc_send_evt_cmd *cmd = (struct gattc_send_evt_cmd *)BleMsgAlloc(GATTC_SEND_EVT_CMD, TASK_ID_GATTC,
            TASK_ID_GTL, sizeof(struct gattc_send_evt_cmd) + sizeof(uint8_t));
    configASSERT(cmd);

    cmd->operation = GATTC_NOTIFY;
    cmd->seq_num = (uint16_t)(iot_sensor_seq_num++ % 0xFFFFFFFF);
    cmd->handle = IOT_SENSOR_SVC_GET_HANDLE(IOT_SENSOR_SVC_TEMP_CHAR_VALUE_IDX);
    cmd->length = sizeof(uint8_t);
    cmd->value[0] = value;

    BleSendMsg(cmd);
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the IoT sensor service environment
 ****************************************************************************************
 */
void iot_sensor_svc_init(void)
{
    iot_sensor_env.svc_handle = 0xFFFF;
    iot_sensor_env.svc_value = 0;
    iot_sensor_env.svc_value_notify_enabled = false;
    iot_sensor_env.svc_callback = NULL;
}

/**
 ****************************************************************************************
 * @brief Get the IoT sensor service environment
 ****************************************************************************************
 */
struct iot_sensor_env_tag *iot_sensor_svc_get_env(void)
{
    return &iot_sensor_env;
}

/**
 ****************************************************************************************
 * @brief Create the database for the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_create_custom_db(uint16_t start_hdl)
{
        struct gattm_add_svc_req *svc_req = (struct gattm_add_svc_req *)BleMsgAlloc(GATTM_ADD_SVC_REQ, TASK_ID_GATTM,
                TASK_ID_GTL, sizeof(struct gattm_svc_desc) + sizeof(iot_sensor_svc_atts));
        configASSERT(svc_req);

        svc_req->svc_desc.start_hdl = start_hdl;
        svc_req->svc_desc.task_id = TASK_ID_GTL;
        svc_req->svc_desc.perm = PERM(SVC_PRIMARY,
                ENABLE) | PERM(SVC_AUTH, ENABLE) | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128);
        svc_req->svc_desc.nb_att = IOT_SENSOR_SVC_NB_ATTS;
        memcpy(svc_req->svc_desc.uuid, iot_sensor_svc_uuid, ATT_UUID_128_LEN);
        memcpy(svc_req->svc_desc.atts, iot_sensor_svc_atts, sizeof(iot_sensor_svc_atts));

        BleSendMsg(svc_req);
}

/**
 ****************************************************************************************
 * @brief Set the handle of the database for the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_set_handle(uint16_t handle)
{
    iot_sensor_env.svc_handle = handle;
}

/**
 ****************************************************************************************
 * @brief Get the handle of the database for the IoT sensor service
 ****************************************************************************************
 */
uint16_t iot_sensor_svc_get_handle(void)
{
        return iot_sensor_env.svc_handle;
}

/**
 ****************************************************************************************
 * @brief Enable the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_enable(uint16_t conhdl, struct iot_sensor_svc_cb *callback)
{
		DA16X_UNUSED_ARG(conhdl);

        configASSERT(iot_sensor_env.svc_handle != 0xFFFF);
        DBG_TMP("[%s] conhdl = %d \n", __func__, conhdl);

        iot_sensor_env.svc_callback = callback;

        gattm_att_set_value_req_send(
                IOT_SENSOR_SVC_GET_HANDLE(IOT_SENSOR_SVC_TEMP_CHAR_USER_DESC_IDX),
                (uint8_t*)iotsensor_svc_temp_desc,
                sizeof(iotsensor_svc_temp_desc));

        if (iot_sensor_env.svc_callback && iot_sensor_env.svc_callback->enabled) {
            iot_sensor_env.svc_callback->enabled(true);
        }
}

/**
 ****************************************************************************************
 * @brief Disable the IoT sensor service
 ****************************************************************************************
 */
void iot_sensor_svc_disable(uint16_t conhdl)
{
		DA16X_UNUSED_ARG(conhdl);

        configASSERT(iot_sensor_env.svc_handle != 0xFFFF);
        DBG_TMP("[%s] conhdl = %d \n", __func__, conhdl);

        if (iot_sensor_env.svc_callback && iot_sensor_env.svc_callback->enabled) {
            iot_sensor_env.svc_callback->enabled(false);
        }

        iot_sensor_env.svc_callback = NULL;
}

/**
 ****************************************************************************************
 * @brief Set the temperature value
 ****************************************************************************************
 */
void iot_sensor_svc_set_temp(uint8_t value)
{
        configASSERT(iot_sensor_env.svc_handle != 0xFFFF);

        iot_sensor_env.svc_value = value;

        DBG_TMP("[%s] value = %d [%d]\n", __func__, value, iot_sensor_env.svc_handle);

        if (iot_sensor_env.svc_value_notify_enabled) {
            iot_sensor_svc_send_event(value);
        }
}

/**
 ****************************************************************************************
 * @brief Handler for the read request of the IoT sensor service
 ****************************************************************************************
 */
uint8_t iot_sensor_svc_gattc_read_req_ind(ke_msg_id_t const msgid,
        void *req_param, ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
        struct gattc_read_req_ind *param = (struct gattc_read_req_ind *)req_param;

        uint8_t *value = NULL;
        uint16_t length = 0;
        uint8_t status = GAP_ERR_NO_ERROR;
        uint16_t handle = (param) ? param->handle : 0;

        (void)msgid;

        if ((handle <= iot_sensor_env.svc_handle)
                || (handle > iot_sensor_env.svc_handle + IOT_SENSOR_SVC_NB_ATTS)) {
                return KE_MSG_NO_FREE;
        }

        switch (IOT_SENSOR_SVC_GET_IDX(handle))
        {
                case IOT_SENSOR_SVC_TEMP_CHAR_VALUE_IDX: {
                        value = &iot_sensor_env.svc_value;
                        length = sizeof(iot_sensor_env.svc_value);
                        break;
                }

                default:
                        status = ATT_ERR_APP_ERROR;
                        break;
        }

        struct gattc_read_cfm *cfm = (struct gattc_read_cfm *)BleMsgAlloc(GATTC_READ_CFM, src_id, dest_id,
                (unsigned short)(sizeof(struct gattc_read_cfm) + length));
        configASSERT(cfm);

        // Set parameters
        cfm->handle = param->handle;
        cfm->length = length;
        if (length > 0 && value) {
                memcpy(cfm->value, value, length);
        }
        cfm->status = status;

        BleSendMsg((void*)cfm);

        return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handler for the write request of the IoT sensor service
 ****************************************************************************************
 */
uint8_t iot_sensor_svc_gattc_write_req_ind(ke_msg_id_t const msgid,
        void *req_param, ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
        struct gattc_write_req_ind *param = (struct gattc_write_req_ind *)req_param;
        uint16_t handle = (param) ? param->handle : 0;

        (void)msgid;

        if ((handle <= iot_sensor_env.svc_handle)
                || (handle > iot_sensor_env.svc_handle + IOT_SENSOR_SVC_NB_ATTS)) {
                return KE_MSG_NO_FREE;
        }

        iot_sensor_svc_write_cfm(handle, dest_id, src_id, ATT_ERR_NO_ERROR);

        switch (IOT_SENSOR_SVC_GET_IDX(handle))
        {
                case IOT_SENSOR_SVC_TEMP_CHAR_CONF_DESC_IDX: {
                        uint16_t value =
                                (param->length == sizeof(uint16_t)) ? co_read16(param->value) : 0;

                        iot_sensor_env.svc_value_notify_enabled = value ? pdTRUE : pdFALSE;
                        if (iot_sensor_env.svc_callback && iot_sensor_env.svc_callback->notify_enabled) {
                            iot_sensor_env.svc_callback->notify_enabled(iot_sensor_env.svc_value_notify_enabled);
                        }
                        break;
                }

                default:
                        break;
        }

        return KE_MSG_CONSUMED;
}
#endif // __IOT_SENSOR_SVC_ENABLE__

/* EOF */
