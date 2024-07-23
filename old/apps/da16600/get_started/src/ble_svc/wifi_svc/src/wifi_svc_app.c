/**
 ****************************************************************************************
 *
 * @file wifi_svc_app.c
 *
 * @brief Proximity Reporter & Sensor Host demo application main
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

#if defined (__BLE_PERI_WIFI_SVC__)

#include "../../include/da14585_config.h"
#include "../../include/app.h"

#include "gap.h"
#include "proxr.h"
#include "proxr_task.h"
#include "diss_task.h"
#include "atts.h"
#include "gapc_task.h"
#include "gapm_task.h"

#if defined (__SUPPORT_BLE_PROVISIONING__)
#include "../../provision/inc/app_provision.h"
#endif

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update_ble_fw.h"
#endif /* __BLE_FW_VER_CHK_IN_OTA__ */

#include "da16_gtl_features.h"

#include "../../include/console.h"
#include "../../include/queue.h"
#include "../../include/ble_msg.h"
#include "../../include/app_task.h"
#include "../../include/user_config.h"
#include "../../include/wifi_svc_user_custom_profile.h"
#include "../../include/app.h"
#include "../../include/user_bonding.h"

#if defined (__GTL_IFC_UART__)
#include "../../include/uart.h"
#endif

#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"
#include "command.h"

#include "sleep2_monitor.h"
#include "lwip/err.h"
#include "command_net.h"

#ifdef __FREERTOS__
#include "msg_queue.h"
#endif

#include "../../user_util/user_util.h"

#if defined (__IOT_SENSOR_SVC_ENABLE__)
#include "../../include/iot_sensor_user_custom_profile.h"
#endif

#ifdef __SUPPORT_ATCMD__
#include "atcmd_ble.h"
#else
#define assert(x)    /* nothing */
#endif

extern void combo_ble_sw_reset(void);

#if defined (__MULTI_BONDING_SUPPORT__)
extern struct app_sec_env_tag temp_bond_info;
extern struct app_sec_env_tag bond_info[MAX_BOND_PEER]; 
extern uint8_t bond_info_current_index;
extern void user_bonding_init(void);
#endif /* __MULTI_BONDING_SUPPORT__ */

extern void host_uart_init_pre_usr_rtm(void);
#ifndef __FREERTOS__
extern SemaphoreHandle_t ConsoleQueueMut;
#endif
extern void InitTasksUserCmd(void);
extern void clear_wakeup_src_all(void);

extern int dpm_user_rtm_pool_init_chk(void);

#if defined    (__WFSVC_ENABLE__)
extern void set_wifi_sta_scan_result_str_idx(uint32_t idx);
#endif

extern uint16_t my_custom_service_wfsvc_start_handle;

user_custom_profile_wfsvc_env_t user_custom_profile_wfsvc_env;

ble_boot_mode_t ble_boot_mode;
dpm_boot_type_t dpm_boot_type = DPM_BOOT_TYPE_NO_DPM;

/* GTL main thread */
#define    GTL_MAIN_STACK_SZ        (2048 / sizeof(StackType_t))
static TaskHandle_t    tx_gtl_main = NULL;

#if defined (__GTL_IFC_UART__)
#define GTL_HOST_IFC_MON_STACK_SZ   (768 / sizeof(StackType_t))
static TaskHandle_t    tx_gtl_host_ifc_mon = NULL;
extern void boot_ble_from_uart();
#endif /* __GTL_IFC_UART__ */

/* BLE App user command handler Task */
#define    BLE_APP_USR_CMD_HDLER_STACK_SZ        (1024 / sizeof(StackType_t))
static TaskHandle_t    tx_ble_app_usr_cmd_hdler = NULL;

struct app_env_tag app_env;
struct app_device_info device_info;

#if defined (__LOW_POWER_IOT_SENSOR__)
iot_sensor_data_t iot_sensor_data_info;
#endif

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
wf_svc_app_data_t* wf_svc_app_data_in_rtm = NULL;
#endif

uint8_t addr_resolv_req_from_gapc_bond_ind;
uint8_t bonding_in_progress;

#if defined (__APP_SLEEP2_MONITOR__)
uint8_t is_first_gtl_msg = TRUE;
#endif

#if defined(__SUPPORT_ATCMD__)
static uint8_t app_user_device_name[GAP_MAX_NAME_SIZE];
static uint8_t app_user_device_name_len = 0;
#endif

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
extern ble_img_hdr_all_t ble_fw_hdrs_all;
#endif /* __BLE_FW_VER_CHK_IN_OTA__ */

// security user settings
const struct security_configuration user_security_configuration =
{
#if defined (__WIFI_SVC_SECURITY__)
    .oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT,
    .key_size       = KEY_LEN,
    .iocap          = GAP_IO_CAP_DISPLAY_YES_NO,
#ifdef __USE_LEGACY_PAIRING__
    .auth           = (enum gap_auth) (GAP_AUTH_BOND | GAP_AUTH_MITM),
#else /* Secure Connection Pairing */
    .auth           = (enum gap_auth) (GAP_AUTH_BOND | GAP_AUTH_MITM | GAP_AUTH_SEC),
#endif
    .sec_req        = GAP_SEC1_AUTH_PAIR_ENC,
    .ikey_dist      = GAP_KDIST_IDKEY,
    .rkey_dist      = GAP_KDIST_ENCKEY,
#else
    .oob            = GAP_OOB_AUTH_DATA_NOT_PRESENT,
    .key_size       = KEY_LEN,
    .iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
    .auth           = (enum gap_auth)GAP_AUTH_REQ_NO_MITM_BOND,
    .sec_req        = GAP_NO_SEC,
    .ikey_dist      = GAP_KDIST_SIGNKEY,
    .rkey_dist      = GAP_KDIST_ENCKEY,
#endif /* __WIFI_SVC_SECURITY__ */
};

#define LEGACY_PAIRING_USER_PIN_CODE (0x01E240)

#if defined (__SUPPORT_WIFI_CONN_FAIL_BY_WK_IND__)
int wifi_conn_fail_by_wrong_pass;
#endif

void app_ble_sw_reset(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_BLE_SW_RESET, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);
    // Send the message
    BleSendMsg(cmd);
}

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)

extern void reboot_func(UINT flag);

int app_user_data_get_from_rtm(void)
{
    return dpm_user_mem_get(APP_GTL_MAIN, (UCHAR **)&wf_svc_app_data_in_rtm);
}

void app_user_data_load(void)
{
    // if needed, load data from nvram or other place
#if defined (__LOW_POWER_IOT_SENSOR__)
    char    *server_ip = NULL;

    /* Get "provisioned" from NVRAM */
    if (read_nvram_int("provisioned", &(iot_sensor_data_info.is_provisioned))) {
        iot_sensor_data_info.is_provisioned = 0;
    }
    
    /* Get "is_sensor_started" from NVRAM */
    if (read_nvram_int("sensor_started", &(iot_sensor_data_info.is_sensor_started))) {
        iot_sensor_data_info.is_sensor_started = 0;
    }

    /* Get "svr_ip_addr" from NVRAM */
    if ((server_ip = read_nvram_string("UDP_SERVER_IP"))) {
        strcpy(iot_sensor_data_info.svr_ip_addr, server_ip);
    } else {
        strcpy(iot_sensor_data_info.svr_ip_addr, "172.16.0.1"); // default
    }

    /* Get "svr_port" from NVRAM */
    if (read_nvram_int("UDP_SERVER_PORT", &(iot_sensor_data_info.svr_port))) {
        iot_sensor_data_info.svr_port = 10195; // default
    }
    
#endif /* __LOW_POWER_IOT_SENSOR__ */
}

void app_user_data_save(void)
{
    if (wf_svc_app_data_in_rtm != NULL) {
        memcpy(&(wf_svc_app_data_in_rtm->app_env), &app_env, sizeof(struct app_env_tag));
        memcpy(&(wf_svc_app_data_in_rtm->device_info), &device_info, sizeof(struct app_device_info));
        memcpy(&(wf_svc_app_data_in_rtm->alert_state), &alert_state, sizeof(app_alert_state));
        wf_svc_app_data_in_rtm->my_custom_service_wfsvc_start_handle = my_custom_service_wfsvc_start_handle;
        memcpy(&(wf_svc_app_data_in_rtm->user_custom_profile_wfsvc_env), &user_custom_profile_wfsvc_env, sizeof(user_custom_profile_wfsvc_env_t));
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
        memcpy(&(wf_svc_app_data_in_rtm->ble_fw_hdrs_all), &ble_fw_hdrs_all, sizeof(ble_img_hdr_all_t));
#endif
#if defined (__MULTI_BONDING_SUPPORT__)        
        memcpy(&(wf_svc_app_data_in_rtm->bond_info), &bond_info, sizeof(bond_info));
        wf_svc_app_data_in_rtm->bond_info_current_index = bond_info_current_index;
        wf_svc_app_data_in_rtm->bonding_in_progress = bonding_in_progress;    
#endif
#if defined (__LOW_POWER_IOT_SENSOR__)
        memcpy(&(wf_svc_app_data_in_rtm->iot_sensor_data_info), &iot_sensor_data_info, sizeof(iot_sensor_data_t));
#endif
#if defined (__IOT_SENSOR_SVC_ENABLE__)
        memcpy(&(wf_svc_app_data_in_rtm->iot_sensor_env), (void *)iot_sensor_svc_get_env(), sizeof(struct iot_sensor_env_tag));
#endif
        wf_svc_app_data_in_rtm->is_ble_in_advertising = app_is_ble_in_advertising();
    }
}

void app_user_data_restore(void)
{
    if (wf_svc_app_data_in_rtm != NULL) {
        memcpy(&app_env, &(wf_svc_app_data_in_rtm->app_env), sizeof(struct app_env_tag));
        memcpy(&device_info, &(wf_svc_app_data_in_rtm->device_info), sizeof(struct app_device_info));
        memcpy(&alert_state, &(wf_svc_app_data_in_rtm->alert_state), sizeof(app_alert_state));
        my_custom_service_wfsvc_start_handle = wf_svc_app_data_in_rtm->my_custom_service_wfsvc_start_handle;
        memcpy(&user_custom_profile_wfsvc_env, &(wf_svc_app_data_in_rtm->user_custom_profile_wfsvc_env), sizeof(user_custom_profile_wfsvc_env_t));
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
        memcpy(&ble_fw_hdrs_all, &(wf_svc_app_data_in_rtm->ble_fw_hdrs_all), sizeof(ble_img_hdr_all_t));
#endif
#if defined (__MULTI_BONDING_SUPPORT__)        
        memcpy(&bond_info, &(wf_svc_app_data_in_rtm->bond_info), sizeof(bond_info));
        bond_info_current_index = wf_svc_app_data_in_rtm->bond_info_current_index;
        bonding_in_progress = wf_svc_app_data_in_rtm->bonding_in_progress;    
#endif
#if defined (__LOW_POWER_IOT_SENSOR__)
        memcpy(&iot_sensor_data_info, &(wf_svc_app_data_in_rtm->iot_sensor_data_info), sizeof(iot_sensor_data_t));
#endif
#if defined (__IOT_SENSOR_SVC_ENABLE__)
        memcpy((void *)iot_sensor_svc_get_env(), &(wf_svc_app_data_in_rtm->iot_sensor_env), sizeof(struct iot_sensor_env_tag));
#endif
        app_set_ble_adv_status(wf_svc_app_data_in_rtm->is_ble_in_advertising);
    }
}

void app_user_data_init_pre(void)
{
    int     rtm_len;
    UINT8    init_from_rtm_done = FALSE;

    // get user data from rtm if found
    if (dpm_mode_is_enabled() == TRUE) {
        if (dpm_mode_is_wakeup() == TRUE) {
            rtm_len = app_user_data_get_from_rtm();
            
            if (rtm_len == sizeof(wf_svc_app_data_t)) {
                // user rtm data is found and in-tact
                init_from_rtm_done = TRUE;

                // Restore user data from rtm
                app_user_data_restore();
                DBG_TMP("[combo] app data init from rtm ... \n");
            } else if (rtm_len > 0 && rtm_len != sizeof(wf_svc_app_data_t)) {
                // user data exists in rtm but corrupted
                DBG_ERR("[%s] Invalid RTM alloc size (%d), reboot...\n", __func__, rtm_len);
                combo_ble_sw_reset();
                reboot_func(SYS_REBOOT);
            } else {
                DBG_TMP("[combo] app data not found in rtm... : rtm_len = %d\n", rtm_len);
                init_from_rtm_done = FALSE;
            }
        }
    }

    // get data from nvram or other place if needed
    if (init_from_rtm_done == FALSE) {
        DBG_TMP("[combo] app data init from nvram ... \n");
        app_user_data_load();
    }
}

void app_user_data_dpm_mem_alloc(void)
{
    UINT    status = ERR_OK;
    
    if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM && wf_svc_app_data_in_rtm == NULL) {
        status = dpm_user_mem_alloc(APP_GTL_MAIN,
                                    (VOID **)&wf_svc_app_data_in_rtm,
                                    sizeof(wf_svc_app_data_t),
                                    100);

        if (status) {
            // user data exists in rtm but corrupted
            DBG_ERR("[%s] RTM alloc failed! (%d), reboot ....\n", __func__, status);
            combo_ble_sw_reset();
            reboot_func(SYS_REBOOT);
        }
    } else {
        DBG_TMP("[combo] no rtm allocation, wf_svc_app_data_in_rtm = %s \n", 
                    wf_svc_app_data_in_rtm? "NOT_NULL" : "NULL");
    }
}

void app_todo_before_sleep(void)
{
    clear_wakeup_src_all();

    DBG_INFO(YELLOW_COLOR"sleep (rtm ON) entered \n"CLEAR_COLOR);
#if defined (__WAKE_UP_BY_BLE__)
    enable_wakeup_by_ble();
#endif
#if defined ( __ENABLE_RTC_WAKE_UP2_INT__ )
    enable_wakeup_intr(1);
#endif // __ENABLE_RTC_WAKE_UP2_INT__
    app_user_data_save();

    // specific to-do for gas leak sample
    if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
        rtc_cancel_timer(TID_U_DHCP_CLIENT);
    }
}

uint8_t app_is_wakeup_by_ble_event(void)
{
    /* Check if it is waked up by BLE */
    if (dpm_mode_get_wakeup_source() == WAKEUP_SENSOR_WITH_RETENTION) {
        return pdTRUE;
    } else {
        return pdFALSE;
    }
}

#endif /* __ENABLE_DPM_FOR_GTL_BLE_APP__ */

#if defined (__LOW_POWER_IOT_SENSOR__)
void app_sensor_start(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_GAS_LEAK_SENSOR_START, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}

void app_sensor_stop(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_GAS_LEAK_SENSOR_STOP, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}

void app_wifi_ready_ind(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_WIFI_READY, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}
#endif /* __LOW_POWER_IOT_SENSOR__ */

void combo_init(void)
{
    UINT32    flow_ctrl, baud_rate;
    UINT    status = 0;
#ifdef __ENABLE_DPM_FOR_GTL_BLE_APP__
    app_user_data_init_pre();
#endif    
#if defined (__LOW_POWER_IOT_SENSOR__)
    DBG_INFO("[combo][iot_sensor]\n   is_provisioned = %d\n   is_sensor_started = %d \n", 
                iot_sensor_data_info.is_provisioned,
             iot_sensor_data_info.is_sensor_started);
#endif /* __LOW_POWER_IOT_SENSOR__ */

#if defined(__SUPPORT_ATCMD__)
    app_user_device_name_len = 0;
    memset(app_user_device_name, 0, GAP_MAX_NAME_SIZE);
#endif

    /* set dpm boot type */
    if (dpm_mode_is_enabled() == TRUE) {
        if (dpm_mode_is_wakeup() == TRUE) {
#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
            if (app_is_wakeup_by_ble_event() == TRUE) {    // wake-up by BLE event
                dpm_boot_type = DPM_BOOT_TYPE_WAKEUP_CARE;
            } else
#endif /* __ENABLE_DPM_FOR_GTL_BLE_APP__  */
            {
                dpm_boot_type = DPM_BOOT_TYPE_WAKEUP_DONT_CARE;
            }
        } else {
            // no dpm wakeup --> normal boot w/ DPM enabled
            dpm_boot_type = DPM_BOOT_TYPE_NORMAL;
        }
    }
    DBG_INFO("[combo] dpm_boot_type = %d\n", dpm_boot_type);

    /* Additional handling on dpm_boot_type */
    switch (dpm_boot_type) {
    case DPM_BOOT_TYPE_WAKEUP_CARE:
    case DPM_BOOT_TYPE_WAKEUP_DONT_CARE:
        // additional handling if any
        break;

    case DPM_BOOT_TYPE_NORMAL:
    default:
        break;
    }

    /* Init uart for GTL */
    if (dpm_boot_type <= DPM_BOOT_TYPE_NORMAL) {
        // H/W Flow control ON
        status = read_nvram_int((const char *)NVR_KEY_UART1_FLOWCTRL, (int *)&flow_ctrl);
        if (status != 0 || flow_ctrl != 1) {
            write_nvram_int((const char *)NVR_KEY_UART1_FLOWCTRL, 1);
        }

        // Read baud rate
        status = read_nvram_int((const char *)NVR_KEY_UART1_BAUDRATE, (int *)&baud_rate);
#if defined (__GTL_UART_115200__)
        if (status != 0 || baud_rate != 115200) {
            write_nvram_int((const char *)NVR_KEY_UART1_BAUDRATE, 115200);
        }
#else
        if (status != 0 || baud_rate != 921600) {
            write_nvram_int((const char *)NVR_KEY_UART1_BAUDRATE, 921600);
        }    
#endif
    }

    host_uart_init_pre_usr_rtm();

    /* Set ble boot mode */
    switch (dpm_boot_type) {
    case DPM_BOOT_TYPE_WAKEUP_CARE:
    case DPM_BOOT_TYPE_WAKEUP_DONT_CARE:
        ble_boot_mode = BLE_BOOT_MODE_1;
        break;

    default:
        ble_boot_mode = BLE_BOOT_MODE_0;
        break;
    }
    DBG_INFO("[combo] BLE_BOOT_MODE_%d\n", ble_boot_mode);

#if defined (__DA14531_BOOT_FROM_UART__)
    if (ble_boot_mode == BLE_BOOT_MODE_0) {    
        _da16x_io_pinmux(PIN_CMUX, CMUX_UART1c);
        boot_ble_from_uart();
    }
#endif /* __DA14531_BOOT_FROM_UART__ */
}

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
void app_force_ble_exception(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_BLE_FORCE_EXCEPTION, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}
#endif /* __TEST_FORCE_EXCEPTION_ON_BLE__ */

void app_ble_fw_ver(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_GET_FW_VERSION, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}

void app_exit(void)
{
    /* Place something if need */
}

void app_set_mode(void)
{
    struct gapm_set_dev_config_cmd *message = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD,
                                          TASK_ID_GAPM,
                                          TASK_ID_GTL,
                                          sizeof(struct gapm_set_dev_config_cmd ));

    // Operation
    message->operation = GAPM_SET_DEV_CONFIG;

    // Device Role
    message->role = GAP_ROLE_PERIPHERAL;

    // Maximal MTU
    message->max_mtu = DEMO_PEER_MAX_MTU - ATT_HEADER_SIZE;

    // Device Address Type
#ifdef USE_BLE_RANDOM_STATIC_ADDRESS
    message->addr_type = GAPM_CFG_ADDR_STATIC;
    for (int i = 0; i < BD_ADDR_LEN; i++) {
        message->addr.addr[i] = (uint8_t)(rand() & 0xff);
    }
#else
    message->addr_type = GAPM_CFG_ADDR_PUBLIC;
#endif

    // Duration before regenerate device address when privacy is enabled
    message->renew_dur = 0 ;
    memset(message->irk.key, 0, sizeof(struct gap_sec_key));

    //ATT database configuration
    message->att_cfg = GAPM_MASK_ATT_SVC_CHG_EN;

    // GAP service start handle
    message->gap_start_hdl = 0;

    // GATT service start handle
    message->gatt_start_hdl = 0;

    // Maximal MPS
    message->max_mps = 0;

    // Maximal Tx octets
    message->max_txoctets = 251;

    // Maximal Tx time
    message->max_txtime = 2120;

    BleSendMsg((void *)message);

    return;
}

#if defined(__SUPPORT_ATCMD__)
uint8_t *app_get_ble_dev_name(void)
{
    if (app_user_device_name_len > 0) {
        return app_user_device_name;
    } else if (device_info.dev_name.length > 0) {
        return device_info.dev_name.name;
    }

    return NULL;
}
#endif

void app_rst_gap(void)
{
    struct gapm_reset_cmd *message = BleMsgAlloc(GAPM_RESET_CMD,
                                 TASK_ID_GAPM,
                                 TASK_ID_GTL,
                                 sizeof(struct gapm_reset_cmd));
    if (message == NULL) {
        DBG_ERR("malloc error in BleMsgAlloc( ) \n");
        return;
    }

    int i;

    message->operation = GAPM_RESET;
    app_env.state = APP_IDLE;
#if defined (__LOW_POWER_IOT_SENSOR__)
    app_env.iot_sensor_state = IOT_SENSOR_INACTIVE;
#endif

#if defined    (__WFSVC_ENABLE__)
    set_wifi_sta_scan_result_str_idx(0);
#endif

    //init scan devices list
    app_env.num_of_devices = 0;

    // Initialize device info
    device_info.dev_name.length = (USER_DEVICE_NAME_LEN <= GAP_MAX_NAME_SIZE) ?
                                             USER_DEVICE_NAME_LEN : GAP_MAX_NAME_SIZE;

    memcpy(device_info.dev_name.name, USER_DEVICE_NAME, device_info.dev_name.length);
#ifdef __APPEND_EXTRA_INFO_AT_DEVICE_NAME
    {
        char macstr[18];

        memset(macstr, 0, 18);
        getMACAddrStr(1, macstr);
        device_info.dev_name.name[8] = macstr[12];
        device_info.dev_name.name[9] = macstr[13];
        device_info.dev_name.name[10] = macstr[15];
        device_info.dev_name.name[11] = macstr[16];
    }
#endif
#if defined(__SUPPORT_ATCMD__)
    if (app_user_device_name_len > 0) {
        // Override the device name if the app_user_device_name has been set.
        device_info.dev_name.length = (app_user_device_name_len <= GAP_MAX_NAME_SIZE) ?
                                                 app_user_device_name_len : GAP_MAX_NAME_SIZE;

        memset(device_info.dev_name.name, 0, GAP_MAX_NAME_SIZE);
        memcpy(device_info.dev_name.name, app_user_device_name, device_info.dev_name.length);
    }
#endif

    PRINTF("IoT dev_name=\"%s\", len=%d\n", device_info.dev_name.name, device_info.dev_name.length);
    device_info.appearance = 0x0000;

    for (i = 0; i < MAX_SCAN_DEVICES; i++) {
        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }

    BleSendMsg((void *)message);

    return;
}

void app_inq(void)
{
    struct gapm_start_scan_cmd *message = BleMsgAlloc(GAPM_START_SCAN_CMD,
                                                      TASK_ID_GAPM,
                                                      TASK_ID_GTL,
                                                      sizeof(struct gapm_start_scan_cmd));
    int i;

    //init scan devices list
    app_env.num_of_devices = 0;

    for (i = 0; i < MAX_SCAN_DEVICES; i++) {
        app_env.devices[i].free = true;
        app_env.devices[i].adv_addr.addr[0] = '\0';
        app_env.devices[i].data[0] = '\0';
        app_env.devices[i].data_len = 0;
        app_env.devices[i].rssi = 0;
    }

    message->mode = GAP_GEN_DISCOVERY;
    message->op.code = GAPM_SCAN_ACTIVE;
    message->interval = 10;
    message->window = 5;

    BleSendMsg((void *)message);

    return;
}

void app_adv_start(void)
{
    uint8_t device_name_length = 0;
    uint8_t device_name_avail_space = 0;
    uint8_t device_name_temp_buf[64];

    // Allocate a message for GAP
    struct gapm_start_advertise_cmd *cmd = BleMsgAlloc(GAPM_START_ADVERTISE_CMD,
                                                       TASK_ID_GAPM,
                                                       TASK_ID_GTL,
                                                       sizeof (struct gapm_start_advertise_cmd));

    cmd->op.code     = GAPM_ADV_UNDIRECT;
    cmd->op.addr_src = GAPM_STATIC_ADDR;
    cmd->intv_min    = APP_ADV_INT_MIN;
    cmd->intv_max    = APP_ADV_INT_MAX;
    cmd->channel_map = ADV_ALL_CHNLS_EN;

    cmd->info.host.mode = GAP_GEN_DISCOVERABLE;
    cmd->info.host.adv_filt_policy = ADV_ALLOW_SCAN_ANY_CON_ANY;

    /*-----------------------------------------------------------------------------------
     * Set the Advertising Data and the Scan Response Data
     *---------------------------------------------------------------------------------*/
    cmd->info.host.adv_data_len       = APP_ADV_DATA_MAX_SIZE;
    cmd->info.host.scan_rsp_data_len  = APP_SCAN_RESP_DATA_MAX_SIZE;

    // Advertising Data
#if (NVDS_SUPPORT)
    if (nvds_get(NVDS_TAG_APP_BLE_ADV_DATA, &cmd->info.host.adv_data_len, &cmd->info.host.adv_data[0]) != NVDS_OK)
#endif
    {
        cmd->info.host.adv_data_len = APP_DFLT_ADV_DATA_LEN;
        memcpy(&cmd->info.host.adv_data[0], APP_DFLT_ADV_DATA, 
                cmd->info.host.adv_data_len);
    }

    // Scan Response Data
#if (NVDS_SUPPORT)
    if (nvds_get(NVDS_TAG_APP_BLE_SCAN_RESP_DATA,
                 &cmd->info.host.scan_rsp_data_len,
                 &cmd->info.host.scan_rsp_data[0]) != NVDS_OK)
#endif
    {
        cmd->info.host.scan_rsp_data_len = APP_SCNRSP_DATA_LENGTH;
        memcpy(&cmd->info.host.scan_rsp_data[0], APP_SCNRSP_DATA,
               cmd->info.host.scan_rsp_data_len);
    }

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    device_name_avail_space = APP_ADV_DATA_MAX_SIZE - cmd->info.host.adv_data_len - 2;

    // Check if data can be added to the Advertising data
    if (device_name_avail_space > 0) {
        // Get the Device Name to add in the Advertising Data (Default one or NVDS one)
#if (NVDS_SUPPORT)
        device_name_length = NVDS_LEN_DEVICE_NAME;

        if (nvds_get(NVDS_TAG_DEVICE_NAME, &device_name_length, &device_name_temp_buf[0]) != NVDS_OK)
#endif /* NVDS_SUPPORT */
        {
            // Get default Device Name (No name if not enough space)
            device_name_length = device_info.dev_name.length;
            memcpy(&device_name_temp_buf[0], device_info.dev_name.name, device_name_length);
        }

        if (device_name_length > 0) {
            // Check available space
            device_name_length = min(device_name_length, device_name_avail_space);

            // Fill Length
            cmd->info.host.adv_data[cmd->info.host.adv_data_len] = device_name_length + 1;

            // Fill Device Name Flag
            cmd->info.host.adv_data[cmd->info.host.adv_data_len + 1] = '\x09';

            // Copy device name
            memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len + 2],
                   device_name_temp_buf,
                   device_name_length);

            // Update Advertising Data Length
            cmd->info.host.adv_data_len += (device_name_length + 2);
        }
    }

    // Send the message
    DBG_INFO(YELLOW_COLOR "[combo] Advertising...\n" CLEAR_COLOR);
    
    app_set_ble_adv_status(pdTRUE);
    
    BleSendMsg(cmd);

    return;
}

void app_cancel(void)
{
    struct gapm_cancel_cmd *message;

    message = (struct gapm_cancel_cmd *) BleMsgAlloc(GAPM_CANCEL_CMD,
                                                     TASK_ID_GAPM,
                                                     TASK_ID_GTL,
                                                     sizeof(struct gapm_cancel_cmd));

    message->operation = GAPM_CANCEL;

    BleSendMsg((void *) message);
}

void app_connect(unsigned char indx)
{
    struct gapm_start_connection_cmd *message;

    if (app_env.devices[indx].free == true) {
        return;
    }

    message = (struct gapm_start_connection_cmd *) BleMsgAlloc(GAPM_START_CONNECTION_CMD,
                                                               TASK_ID_GAPM,
                                                               TASK_ID_GTL,
                                                               sizeof(struct gapm_start_connection_cmd ));

    message->nb_peers = 1;
    memcpy((void *)&message->peers[0].addr, (void *)&app_env.devices[indx].adv_addr.addr, BD_ADDR_LEN);
    message->con_intv_min = 100;
    message->con_intv_max = 100;
    message->ce_len_min = 0x0;
    message->ce_len_max = 0x5;
    message->con_latency = 0;
    message->superv_to = 0x7D0;
    message->scan_interval = 0x180;
    message->scan_window = 0x160;
    message->op.code = GAPM_CONNECTION_DIRECT;

    BleSendMsg((void *) message);
}

void app_send_disconnect(uint16_t dst, uint16_t conhdl, uint8_t reason)
{
    struct gapc_disconnect_ind *disconnect_ind = BleMsgAlloc(GAPC_DISCONNECT_IND,
                                                             dst,
                                                             TASK_ID_GTL,
                                                             sizeof(struct gapc_disconnect_ind));

    // fill parameters
    disconnect_ind->conhdl   = conhdl;
    disconnect_ind->reason   = reason;

    // send indication
    BleSendMsg(disconnect_ind);
}

void app_read_rssi(void)
{
    struct gapc_get_info_cmd *req = BleMsgAlloc(GAPC_GET_INFO_CMD,
                                                KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                                TASK_ID_GTL, sizeof(struct gapc_get_info_cmd));

    req->operation = GAPC_GET_CON_RSSI;

    BleSendMsg((void *) req);
}

void app_disconnect(void)
{
    struct gapc_disconnect_cmd *req = BleMsgAlloc(GAPC_DISCONNECT_CMD,
                                                  KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                                  TASK_ID_GTL, sizeof(struct gapc_disconnect_cmd ));

    req->operation = GAPC_DISCONNECT;
    req->reason = CO_ERROR_REMOTE_USER_TERM_CON;

    BleSendMsg((void *) req);
}

void app_get_local_dev_info(enum gapm_operation ops)
{
    struct gapm_get_dev_info_cmd *req = BleMsgAlloc(GAPM_GET_DEV_INFO_CMD,
                                                    TASK_ID_GAPM,
                                                    TASK_ID_GTL, 
                                                    sizeof(struct gapm_get_dev_info_cmd ));

    req->operation = ops;

    BleSendMsg((void *) req);
}


void app_security_enable(void)
{
    // Allocate the message
    struct gapc_bond_cmd *req = BleMsgAlloc(GAPC_BOND_CMD,
                                            KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                            TASK_ID_GTL, sizeof(struct gapc_bond_cmd));


    req->operation = GAPC_BOND;
    //GAP_SEC1_NOAUTH_PAIR_ENC;
    req->pairing.sec_req = GAP_NO_SEC;

    // OOB information
    req->pairing.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;

    // IO capabilities
    req->pairing.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;

    // Authentication requirements
    req->pairing.auth           = GAP_AUTH_REQ_NO_MITM_BOND;

    // Encryption key size
    req->pairing.key_size       = 16;

    //Initiator key distribution
    req->pairing.ikey_dist      = 0x04;

    //Responder key distribution
    req->pairing.rkey_dist      = 0x03;

    // Send the message
    BleSendMsg(req);

}

void app_connect_confirm(enum gap_auth auth, uint8_t svc_changed_ind_enable)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = BleMsgAlloc(GAPC_CONNECTION_CFM,
                                                  KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                                  TASK_ID_GTL,
                                                  sizeof (struct gapc_connection_cfm));

    cfm->auth = auth;
    cfm->svc_changed_ind_enable = svc_changed_ind_enable;

    // Send the message
    BleSendMsg(cfm);
}

/**
 ****************************************************************************************
 * @brief Fills an array with random bytes (starting from the end of the array)
 *          Assuming an array of array_size and a dynamic key size, so that key_size = M*4+N:
 *          Calls rand() M times and appends the 4 bytes of each 32 bit return value to the output array
 *          Calls rand() once and appends the N most significant bytes of the 32 bit return value to the output array
 *          Fills the rest bytes of the array with zeroes
 *
 * @param[in] *dst              The address of the array
 * @param[in] key_size          Number of bytes that shall be randomized
 * @param[in] array_size        Total size of the array
 *
 * @return void
 ****************************************************************************************
 */
static void fill_random_byte_array(uint8_t *dst, uint8_t key_size, uint8_t array_size)
{
    uint32_t rand_val;
    uint8_t rand_bytes_cnt = 0;
    uint8_t remaining_bytes;
    uint8_t i;

    // key_size must not be greater than array_size
    assert(array_size >= key_size);

    // key_size = M*4+N
    // Randomize the M most significant bytes of the array
    for (i = 0; i < key_size - 3; i += 4) {
        // Calculate a random 32 bit value
        rand_val = rand();
        // Assign each of the 4 rand bytes to the byte array
        dst[array_size - (i + 0) - 1] = (rand_val >> 24) & 0xFF;
        dst[array_size - (i + 1) - 1] = (rand_val >> 16) & 0xFF;
        dst[array_size - (i + 2) - 1] = (rand_val >> 8) & 0xFF;
        dst[array_size - (i + 3) - 1] = (rand_val >> 0) & 0xFF;
        // Count randomized bytes
        rand_bytes_cnt += 4;
    }

    // Randomize the remaining N most significant bytes of the array. (Max N = 3)
    remaining_bytes = key_size - rand_bytes_cnt;

    if (remaining_bytes) {
        rand_val = rand();

        for (i = 0; i < remaining_bytes; i++) {
            dst[array_size - (rand_bytes_cnt + i) - 1] = (rand_val >> ((3 - i) << 3)) & 0xFF;
        }
    }

    // Fill the least significant bytes of the array with zeroes
    remaining_bytes = array_size - key_size;

    for (i = 0; i < remaining_bytes; i++) {
        dst[array_size - (key_size + i) - 1] = 0;
    }
}

void app_gen_csrk(void)
{
    // Randomly generate the CSRK
    fill_random_byte_array(app_env.proxr_device.csrk.key, KEY_LEN, KEY_LEN);
}

void app_sec_gen_ltk(uint8_t key_size)
{
    app_env.proxr_device.ltk.key_size = key_size;

    // Randomly generate the LTK and the Random Number
    fill_random_byte_array(app_env.proxr_device.ltk.randnb.nb, RAND_NB_LEN, RAND_NB_LEN);

    // Randomly generate the end of the LTK
    fill_random_byte_array(app_env.proxr_device.ltk.ltk.key, key_size, KEY_LEN);

    // Randomly generate the EDIV
    app_env.proxr_device.ltk.ediv = rand() % 65536;
}

uint32_t app_gen_tk(void)
{
    // Generate a PIN Code (Between 100000 and 999999)
    return (100000 + (rand() % 900000));
}

void app_resolve_random_addr(uint8_t *addr, uint8_t *irks, uint16_t counter)
{

    struct gapm_resolv_addr_cmd *cmd = BleMsgAlloc(GAPM_RESOLV_ADDR_CMD,
                                                  TASK_ID_GAPM, TASK_ID_GTL,
                                                  sizeof(struct gapm_resolv_addr_cmd) + (counter * sizeof(struct gap_sec_key)));

    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = counter;
    memcpy( &cmd->addr, addr, 6);
    memcpy(cmd->irk, irks, counter * sizeof(struct gap_sec_key));

    // Send the message
    BleSendMsg(cmd);
}


void app_gap_bond_cfm(struct gapc_bond_req_ind *param)
{
    struct gapc_bond_cfm *cfm = BleMsgAlloc(GAPC_BOND_CFM,
                                            KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                            TASK_ID_GTL, sizeof(struct gapc_bond_cfm));

    DBG_INFO("<<< GAPC_BOND_REQ_IND: req=%d, auth_req=%d, tk_type=%d \n",
             param->request,
             param->data.auth_req,
             param->data.tk_type);

    switch (param->request) {
        // Bond Pairing request
        case GAPC_PAIRING_REQ:
        {
            DBG_INFO("    req = GAPC_PAIRING_REQ \n");
            bonding_in_progress = true;
            cfm->request = GAPC_PAIRING_RSP;
            cfm->accept = true;

            // OOB information
            cfm->data.pairing_feat.oob            = user_security_configuration.oob;
            // Encryption key size
            cfm->data.pairing_feat.key_size       = user_security_configuration.key_size;
            // IO capabilities
            cfm->data.pairing_feat.iocap          = user_security_configuration.iocap;
            // Authentication requirements
            cfm->data.pairing_feat.auth           = user_security_configuration.auth;
            //Initiator key distribution
            cfm->data.pairing_feat.ikey_dist      = user_security_configuration.ikey_dist;
            //Responder key distribution
            cfm->data.pairing_feat.rkey_dist      = user_security_configuration.rkey_dist;
            //Security requirements
            cfm->data.pairing_feat.sec_req        = user_security_configuration.sec_req;

            DBG_INFO("    GAPC_BOND_CFM.req = GAPC_PAIRING_RSP\n");
        } break;

        // Used to retrieve pairing Temporary Key
        case GAPC_TK_EXCH:
        {
            DBG_INFO("    req = GAPC_TK_EXCH \n");

            if (param->data.tk_type == GAP_TK_DISPLAY) {
                DBG_INFO("    Passkey Type = GAP_TK_DISPLAY \n");

                // static passkey can also be used instead of randomly generated one
                uint32_t pin_code = app_gen_tk();

                cfm->request = GAPC_TK_EXCH;
                cfm->accept = true;

                memset(cfm->data.tk.key, 0, KEY_LEN);

                cfm->data.tk.key[3] = (uint8_t)((pin_code & 0xFF000000) >> 24);
                cfm->data.tk.key[2] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
                cfm->data.tk.key[1] = (uint8_t)((pin_code & 0x0000FF00) >> 8);
                cfm->data.tk.key[0] = (uint8_t)((pin_code & 0x000000FF) >> 0);

                DBG_INFO(YEL_COL"    A passkey generated, ENTER IT ON THE PEER SIDE : %d\n" CLR_COL, pin_code);
            } else if (param->data.tk_type == GAP_TK_KEY_CONFIRM) {
                DBG_INFO("    Passkey Type = GAP_TK_KEY_CONFIRM \n");

                /*
                BLE Application sends the Pincode which external host must display 
                in this message, so that users can confirm
                */

                uint32_t passkey;
                // Print the 6 Least Significant Digits of Passkey
                char buf[6];
                passkey = (param->tk.key[0] << 0 ) | (param->tk.key[1] << 8 ) |
                          (param->tk.key[2] << 16) | (param->tk.key[3] << 24);

                DBG_INFO(YEL_COL "\r\n Confirmation Value (PINCODE) : ");
                for (uint8_t i=0; i<6; i++) {
                    buf[5-i] = passkey%10;
                    passkey /= 10;
                }
                
                for (uint8_t i=0; i<6; i++) {
                    DBG_INFO("%u",buf[i]);
                }
                DBG_INFO("\n" CLR_COL);
                
                cfm->request = GAPC_TK_EXCH;
                cfm->accept = true;
                
                // in case of GAP_TK_KEY_CONFIRM, filled with 0
                memset(cfm->data.tk.key, 0, KEY_LEN);
            } else {
                ASSERT_ERR(0);
            }
        } break;

        // Used for Long Term Key exchange
        case GAPC_LTK_EXCH:
        {
#if defined (__MULTI_BONDING_SUPPORT__)        
            struct app_sec_env_tag *sec_env = &temp_bond_info;
#endif
            DBG_INFO("        GAPC_BOND_CFM.req = GAPC_LTK_EXCH, Legacy Pairing...\n");

            // generate ltk and store to app_env
            app_sec_gen_ltk(param->data.key_size);
            
#if defined (__MULTI_BONDING_SUPPORT__)    
            //copy LTK to temp_bond_info
            sec_env->key_size = param->data.key_size;
            memcpy(sec_env->ltk.key, app_env.proxr_device.ltk.ltk.key, sec_env->key_size);
            memcpy(sec_env->rand_nb.nb, app_env.proxr_device.ltk.randnb.nb, RAND_NB_LEN);
            sec_env->ediv = app_env.proxr_device.ltk.ediv;
#endif
            cfm->request = GAPC_LTK_EXCH;
            cfm->accept = true;
            cfm->data.ltk.key_size = app_env.proxr_device.ltk.key_size;
            cfm->data.ltk.ediv = app_env.proxr_device.ltk.ediv;

            memcpy(&(cfm->data.ltk.randnb), &(app_env.proxr_device.ltk.randnb), RAND_NB_LEN);
            memcpy(&(cfm->data.ltk.ltk), &(app_env.proxr_device.ltk.ltk), KEY_LEN);

            DBG_INFO("        LTK generated and stored in ext host, and also being sent to BLE Stack ... \n");
        } break;

        default:
        {
            DBG_INFO("No handler defined for param->request = %d\n", param->request);
            BleFreeMsg(cfm);
            return;
        } break;
    }

    // Send the message
    BleSendMsg(cfm);
    DBG_INFO(">>> GAPC_BOND_CFM sent \n\n");
}

void app_update_conn_params(void)
{
    // Connection Parameters update 
    struct gapc_param_update_cmd  *cmd = BleMsgAlloc(GAPC_PARAM_UPDATE_CMD, 
                                                     KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx), 
                                                     TASK_ID_GTL, sizeof(struct gapc_param_update_cmd));

    cmd->operation = GAPC_UPDATE_PARAMS;

    /* 
        Connection interval minimum measured in ble double slots (1.25ms)
        use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
    */
    cmd->intv_min   = MS_TO_DOUBLESLOTS((int)(12));

    /*  Connection interval maximum measured in ble double slots (1.25ms)
        use the macro MS_TO_DOUBLESLOTS to convert from milliseconds (ms) to double slots
        use different min/max for Apple devices
    */
    cmd->intv_max   = MS_TO_DOUBLESLOTS((int)(12));

    // Latency measured in connection events
    cmd->latency    = 0x0000; //user_connection_param_conf.latency;

    /*
        Supervision timeout measured in timer units (10 ms)
        use the macro MS_TO_TIMERUNITS to convert from milliseconds (ms) to timer units
    */
    cmd->time_out   = ((int)(10000/10));// 10 sec

    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    BleSendMsg(cmd);
    DBG_INFO(">>> GAPC_PARAM_UPDATE_CMD \n");
}


/**
 ****************************************************************************************
 */
void app_from_ext_host_security_start(void)
{
    // Send security request command
    struct gapc_security_cmd *cmd = BleMsgAlloc(GAPC_SECURITY_CMD, KE_BUILD_ID(TASK_ID_GAPC,
                                    app_env.proxr_device.device.conhdl), TASK_ID_GTL,
                                    sizeof (struct gapc_security_cmd));

    // Security request
    cmd->operation = GAPC_SECURITY_REQ;
    cmd->auth = user_security_configuration.auth;

    // Send the message
    BleSendMsg(cmd);
}


void app_start_encryption(void)
{
    // Allocate the message
    struct gapc_encrypt_cmd *req = BleMsgAlloc(GAPC_ENCRYPT_CMD,
                                               KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device.device.conidx),
                                               TASK_ID_GTL,
                                               sizeof(struct gapc_encrypt_cmd));

    req->operation = GAPC_ENCRYPT;
    memcpy(&req->ltk, &app_env.proxr_device.ltk, sizeof(struct gapc_ltk));

    // Send the message
    BleSendMsg(req);
}

bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2)
{
    unsigned char idx;

    for (idx = 0; idx < BD_ADDR_LEN; idx++) {
        // checks if the addresses are similar
        if (bd_address1->addr[idx] != bd_address2->addr[idx]) {
            return (false);
        }
    }

    return (true);
}

unsigned char app_device_recorded(struct bd_addr *padv_addr)
{
    int i;

    for (i = 0; i < MAX_SCAN_DEVICES; i++) {
        if (app_env.devices[i].free == false) {
            if (bdaddr_compare(&app_env.devices[i].adv_addr, padv_addr)) {
                break;
            }
        }
    }

    return i;
}

void app_proxr_create_db(void)
{
    struct proxr_db_cfg *db_cfg;
    // Add PROXR in the database
    struct gapm_profile_task_add_cmd *req = BleMsgDynAlloc(GAPM_PROFILE_TASK_ADD_CMD,
                                                           TASK_ID_GAPM,
                                                           TASK_ID_GTL,
                                                           sizeof(struct gapm_profile_task_add_cmd),
                                                           sizeof(struct proxr_db_cfg));

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 4;
    req->prf_task_id = TASK_ID_PROXR;
    req->app_task = TASK_ID_GTL;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct proxr_db_cfg * ) req->param;
    db_cfg->features = PROXR_IAS_TXPS_SUP;

    // Send the message
    BleSendMsg((void *) req);
}

void app_diss_create_db(void)
{
    struct diss_db_cfg *db_cfg;
    // Add DISS in the database
    struct gapm_profile_task_add_cmd *req = BleMsgDynAlloc(GAPM_PROFILE_TASK_ADD_CMD,
                                                           TASK_ID_GAPM,
                                                           TASK_ID_GTL,
                                                           sizeof(struct gapm_profile_task_add_cmd),
                                                           sizeof(struct diss_db_cfg));

    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = 4;
    req->prf_task_id = TASK_ID_DISS;
    req->app_task = TASK_ID_GTL;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct diss_db_cfg * ) req->param;
    db_cfg->features = APP_DIS_FEATURES;

    // Send the message
    BleSendMsg((void *) req);
}

void ConsoleEvent_handler(unsigned char type)
{
    switch (type) {
#if defined (__LOW_POWER_IOT_SENSOR__)
        case CONSOLE_IOT_SENSOR_START:
        {
            if (dpm_mode_is_enabled() == FALSE) {
                DBG_INFO("\n Please do provision w/ DPM enabled and try again!\n");
                break;
            }

            if (app_env.iot_sensor_state == IOT_SENSOR_INACTIVE) {
                app_sensor_start();
            }
        } break;

        case CONSOLE_IOT_SENSOR_STOP:
        {
            if (app_env.iot_sensor_state == IOT_SENSOR_ACTIVE) {
                app_sensor_stop();
            }
        } break;
#endif /* __LOW_POWER_IOT_SENSOR__ */

        case CONSOLE_GAPM_RESET:
        {
            app_rst_gap();
        } break;

        case CONSOLE_GET_BLE_FW_VER_CMD:
        {
            app_ble_fw_ver();
        } break;

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
        case CONSOLE_FORCE_EXCEPTION_ON_BLE:
        {
            app_force_ble_exception();
        } break;
#endif

        case CONSOLE_DISCONNECT_CMD:
        {
            if (app_env.state == APP_CONNECTED) {
                app_disconnect();
            } else {
                DBG_INFO("BLE is not in connected state. \n");
            }
        } break;

        case CONSOLE_ADV_START_CMD:
        {
            DBG_INFO("CONSOLE_ADV_START_CMD\n");

            if (app_env.state != APP_CONNECTED) {
                app_adv_start();
            }

        } break;

        case CONSOLE_ADV_STOP_CMD:
        {
            DBG_INFO("CONSOLE_ADV_STOP_CMD\n");
            app_cancel();

        } break;

        default:
            break;
    }
}

void ConsoleEvent(void)
{
#ifdef __FREERTOS__
    msg queue_msg;

    if (msg_queue_get(&ConsoleQueue, &queue_msg, portMAX_DELAY) == OS_QUEUE_EMPTY) {
        configASSERT(0);
    }

    console_msg *message = (console_msg *)queue_msg.data;

    if (message != NULL) {
        ConsoleEvent_handler(message->type);
    }

    msg_release(&queue_msg);

#else
    console_msg *message;

    xSemaphoreTake(ConsoleQueueMut, portMAX_DELAY);

    if (ConsoleQueue.First != NULL) {
        DBG_INFO("[%s] user command exists in queue \n", __func__);

        message = (console_msg *) DeQueue(&ConsoleQueue);

        ConsoleEvent_handler(message->type);

        MEMFREE(message);
    }

    xSemaphoreGive(ConsoleQueueMut);
#endif
}

void app_env_init(void)
{
    memset(app_env.proxr_device.device.adv_addr.addr, 0,
           sizeof(app_env.proxr_device.device.adv_addr.addr));
    app_env.proxr_device.bonded = 0;
    app_env.num_of_devices = 0;
}

void gtl_host_ifc_mon(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    UARTProc();
}

/*
 *    @brief GTL message main handler
*/
void gtl_main(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int status;
    
    InitTasks();

#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)
    dpm_app_register("test_sleep3", 0);
    dpm_app_wakeup_done("test_sleep3");
#endif

#if defined(__GTL_IFC_UART__)
    // create host interface monitoring thread (gtl_host_ifc_mon)

    if (tx_gtl_host_ifc_mon) {
        vTaskDelete(tx_gtl_host_ifc_mon);
        tx_gtl_host_ifc_mon = NULL;
    }

    status = xTaskCreate(gtl_host_ifc_mon,
                         "gtl_host_ifc_mon",
                         GTL_HOST_IFC_MON_STACK_SZ,
                         (void *)NULL,
                         tskIDLE_PRIORITY + 2,
                         &tx_gtl_host_ifc_mon);

    if (status != pdPASS) {
        DBG_ERR("[%s] ERROR(0x%0x)\n", __func__, status);
        if (tx_gtl_host_ifc_mon) {
            MEMFREE(tx_gtl_host_ifc_mon);
        }
    }

#endif /* __GTL_IFC_UART__ */

#if !defined(__DA14531_BOOT_FROM_UART__)
#if !defined(__TMP_DSPS_TEST__)
    app_rst_gap();
#endif
#endif

#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)
    if (ble_boot_mode == BLE_BOOT_MODE_3) {
        app_rst_gap();
    }
#endif
    _da16x_io_pinmux(PIN_CMUX, CMUX_UART1c);        // RTS: GPIO[4], CTS: GPIO[5]
    DA16X_DIOCFG->GPIOA_PE_PS &= (~(0x10));//Pull Disable jason200908

    _da16x_io_pinmux(PIN_CMUX, CMUX_UART1c);
    
    // GTL message loop
    while (1) {
#if defined(__GTL_IFC_UART__)
        BleReceiveMsg();
#endif
    }
}

void ble_app_usr_cmd_hdler(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    // init mutex and sema for message Qs
    InitTasksUserCmd();

    // message loop
    while (1) {
        ConsoleEvent();
    }
}

/*
 * @brief init ble interface and starts main GTL message handler
*/
void gtl_init(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    UINT    status = ERR_OK;

    if (ble_combo_ref_flag == pdFALSE) {
        vTaskDelete(NULL);
        return;
    }

    if (dpm_boot_type > DPM_BOOT_TYPE_NO_DPM) {
        extern void host_uart_init_post_usr_rtm(void);
        // Check and wait until User RTM initialize done...
        if (dpm_mode_is_enabled() && dpm_mode_is_wakeup() == pdFALSE) {
            while (dpm_user_mem_init_check() == pdFALSE) {
                vTaskDelay(1);
            }
        }
        DBG_INFO("\r\n[DPM]User RTM initialization done\r\n");

        // save uart1 info to rtm for dpm wakeup
        host_uart_init_post_usr_rtm();
#ifdef __ENABLE_DPM_FOR_GTL_BLE_APP__ 
        app_user_data_dpm_mem_alloc();
#endif
    }
    
#if defined (__APP_SLEEP2_MONITOR__)
    sleep2_monitor_regi(SLEEP2_MON_NAME_PROV, __func__, __LINE__);
    sleep2_monitor_regi(SLEEP2_MON_NAME_IOT_SENSOR, __func__, __LINE__);

    if (dpm_boot_type == DPM_BOOT_TYPE_WAKEUP_DONT_CARE) {
        sleep2_monitor_set_state(SLEEP2_MON_NAME_PROV, SLEEP2_SET, __func__, __LINE__);
    }
#endif /* __APP_SLEEP2_MONITOR__ */

#if defined (__MULTI_BONDING_SUPPORT__)
    user_bonding_init();
#endif

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
    app_ble_peer_idle_timer_create();
#endif

    /* Create GTL main Task */
    if (tx_gtl_main) {
        vTaskDelete(tx_gtl_main);
        tx_gtl_main = NULL;
    }

    status = xTaskCreate(gtl_main,
                         APP_GTL_MAIN,
                         GTL_MAIN_STACK_SZ,
                         (void *)NULL,
                         tskIDLE_PRIORITY + 1,
                         &tx_gtl_main);

    if (status != pdPASS) {
        DBG_ERR("[%s] ERROR(0x%0x)\n", __func__, status);
        if (tx_gtl_main) {
            MEMFREE(tx_gtl_main);
        }
    }


    /* BLE Application User Command handler Task */
    if (tx_ble_app_usr_cmd_hdler) {
        vTaskDelete(tx_ble_app_usr_cmd_hdler);
        tx_ble_app_usr_cmd_hdler = NULL;
    }

    status = xTaskCreate(ble_app_usr_cmd_hdler,
                         APP_GTL_BLE_USR_CMD,
                         BLE_APP_USR_CMD_HDLER_STACK_SZ,
                         (void *)NULL,
                         tskIDLE_PRIORITY + 1,
                         &tx_ble_app_usr_cmd_hdler);

    if (status != pdPASS) {
        DBG_ERR("[%s] ERROR(0x%0x)\n", __func__, status);

        if (tx_ble_app_usr_cmd_hdler) {
            MEMFREE(tx_ble_app_usr_cmd_hdler);
        }
    }

    vTaskDelete(NULL);
}


#if defined(__SUPPORT_ATCMD__)
int atcmd_ble_set_device_name_handler(uint8_t *name)
{
    uint8_t length = 0;
    if (name == NULL)    return AT_CMD_ERR_WRONG_ARGUMENTS;

    length = (uint8_t)strlen((char *)name);
    app_user_device_name_len = (length > GAP_MAX_NAME_SIZE) ? GAP_MAX_NAME_SIZE : length;
    memcpy(app_user_device_name, name, app_user_device_name_len);

    // BLE restart to update the device name.
    app_rst_gap();

    return AT_CMD_ERR_CMD_OK;
}
#endif // __SUPPORT_ATCMD__

int app_is_ble_in_connected(void)
{
    return (app_env.state == APP_CONNECTED? true:false);
}

#endif /* __BLE_PERI_WIFI_SVC__ */

/* EOF */
