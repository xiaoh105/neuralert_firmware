/**
 ****************************************************************************************
 *
 * @file sensor_gw_app.c
 *
 * @brief Sensor Gateway main application
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
#include "gap.h"
#include "gapc_task.h"
#include "gapm_task.h"

#if defined (__SUPPORT_BLE_PROVISIONING__)
#include "../../provision/inc/app_provision.h"
#endif

#include "proxm.h"
#include "proxm_task.h"
#include "disc.h"
#include "disc_task.h"
#include "prf_types.h"
#include "app_prf_perm_types.h"

// gtl platform common include
#include "../../include/queue.h" // common include
#include "../../include/ble_msg.h" // common include
#include "../../include/uart.h" // common include

// project specific include
#include "../inc/console.h"
#include "../inc/app.h"
#include "../inc/app_task.h"
#include "../../include/user_config.h"

#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"
#include "da16_gtl_features.h"
#ifdef __FREERTOS__
#include "msg_queue.h"
#endif
#ifdef __APPEND_EXTRA_INFO_AT_DEVICE_NAME
#include "command_net.h"
#endif

#ifdef __SUPPORT_ATCMD__
#include "atcmd_ble.h"
#else
#define assert(x)    /* nothing */
#endif

struct app_env_tag app_env;
struct app_device_info device_info;
int poll_count;
unsigned int proxm_trans_in_prog = true;
uint8_t last_char[MAX_CONN_NUMBER];

extern void host_uart_init_pre_usr_rtm(void);
extern void InitTasksUserCmd(void);

extern int noti_cnt[MAX_CONN_NUMBER];


#ifndef __FREERTOS__
extern SemaphoreHandle_t ConsoleQueueMut;
#endif

ble_boot_mode_t ble_boot_mode;
dpm_boot_type_t dpm_boot_type = DPM_BOOT_TYPE_NO_DPM;

// GTL main thread
#define    GTL_MAIN_STACK_SZ        (2048 / sizeof(StackType_t))
static TaskHandle_t    tx_gtl_main = NULL;

#if defined (__GTL_IFC_UART__)
#define GTL_HOST_IFC_MON_STACK_SZ   (768 / sizeof(StackType_t))
static TaskHandle_t    tx_gtl_host_ifc_mon = NULL;
extern void boot_ble_from_uart();
#endif

#define    BLE_APP_USR_CMD_HDLER_STACK_SZ        (1024 / sizeof(StackType_t))
static TaskHandle_t    tx_ble_app_usr_cmd_hdler = NULL;

/*
    UUID info below is for this specific sensor gateway demo only; 
    dependent on a ble peer (GATT Server)
*/
// IOT_SENSOR_SVC_UUID =            '12345678-1234-5678-1234-56789abcdef0'
const uuid_128_t IOT_SENSOR_SVC_UUID            = UUID_128_LE(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

// IOT_SENSOR_SVC_TEMP_CHAR_UUID =    '12345678-1234-5678-1234-56789abcdef1'
const uuid_128_t IOT_SENSOR_SVC_TEMP_CHAR_UUID    = UUID_128_LE(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf1);

// IOT_SENSOR_SVC_TEMP_CCCD_UUID =    '12345678-1234-5678-1234-56789abcdef2'
const uuid_128_t IOT_SENSOR_SVC_TEMP_CCCD_UUID    = UUID_128_LE(0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf2);

#if defined(__SUPPORT_ATCMD__)
static uint8_t app_user_device_name[GAP_MAX_NAME_SIZE];
static uint8_t app_user_device_name_len = 0;
#endif

#if defined (__WFSVC_ENABLE__)
static bool wifi_svc_enabled;
#endif

void combo_init(void)
{
    UINT32    flow_ctrl, baud_rate;
    UINT    status = 0;

#if defined(__SUPPORT_ATCMD__)
    app_user_device_name_len = 0;
    memset(app_user_device_name, 0, GAP_MAX_NAME_SIZE);
#endif
    
    DBG_INFO("[combo] dpm_boot_type = %d\n", dpm_boot_type);

    /* init uart for gtl */
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

    /* set ble boot mode */
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

void app_exit(void)
{
    /* Place something if need */
}

void app_disc_create(void)
{
    struct gapm_profile_task_add_cmd *message = BleMsgAlloc(GAPM_PROFILE_TASK_ADD_CMD,
                                                            TASK_ID_GAPM,
                                                            TASK_ID_GTL,
                                                            sizeof(struct gapm_profile_task_add_cmd));

    message->operation = GAPM_PROFILE_TASK_ADD;
    message->sec_lvl = SRV_PERM_ENABLE;
    message->prf_task_id = TASK_ID_DISC;
    message->app_task = TASK_ID_GTL;
    message->start_hdl = 0;

    BleSendMsg((void *)message);

    return;
}


void app_proxm_create(void)
{
    struct gapm_profile_task_add_cmd *message = BleMsgAlloc(GAPM_PROFILE_TASK_ADD_CMD,
                                                            TASK_ID_GAPM,
                                                            TASK_ID_GTL,
                                                            sizeof(struct gapm_profile_task_add_cmd));

    message->operation = GAPM_PROFILE_TASK_ADD;
    message->sec_lvl = SRV_PERM_ENABLE;
    message->prf_task_id = TASK_ID_PROXM;
    message->app_task = TASK_ID_GTL;
    message->start_hdl = 0;

    BleSendMsg((void *)message);

    return;
}

void app_set_mode_cent(void)
{
    struct gapm_set_dev_config_cmd *message = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD,
                                                          TASK_ID_GAPM,
                                                          TASK_ID_GTL,
                                                          sizeof(struct gapm_set_dev_config_cmd));

    message->operation = GAPM_SET_DEV_CONFIG;
    message->role = GAP_ROLE_CENTRAL; // Device Role
    memset( message->irk.key, 0, sizeof(struct gap_sec_key));
    message->att_cfg = GAPM_MASK_ATT_SVC_CHG_EN;
    message->gap_start_hdl = 0;
    message->gatt_start_hdl = 0;
    message->renew_dur = 0 ;

    /*
     *    depends on application, please enable this code and 
     *    specify max tx payload size here
     */
#ifdef __ENABLE_MAX_TX_PAYLOAD_SIZE
    message->max_mtu = MAX_GATT_MTU_SIZE;
#endif

    BleSendMsg((void *)message);

    return;
}


void app_set_mode_peri(void)
{
    struct gapm_set_dev_config_cmd *message = BleMsgAlloc(GAPM_SET_DEV_CONFIG_CMD,
                                                          TASK_ID_GAPM,
                                                          TASK_ID_GTL,
                                                          sizeof(struct gapm_set_dev_config_cmd ));

    message->operation = GAPM_SET_DEV_CONFIG;

    // Device Role
    message->role = GAP_ROLE_PERIPHERAL;

    // Maximal MTU
    message->max_mtu = MAX_LE_PKT_SIZE - ATT_HEADER_SIZE;

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
    memset( message->irk.key, 0, sizeof(struct gap_sec_key));

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

    int i;

    message->operation = GAPM_RESET;
    app_env.state = APP_IDLE;

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
    DBG_INFO("IoT dev_name=\"%s\", len=%d\n", device_info.dev_name.name, device_info.dev_name.length);
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
    message->op.addr_src = GAPM_STATIC_ADDR;
    message->filter_duplic = SCAN_FILT_DUPLIC_EN;

    /*
    About .interval & .window
        N = 0xXXXX Size: 2 Octets
        Range: 0x0012 to 0x1000; only even values are valid
        Default: 0x1000 (4096)
        Mandatory Range: 0x0012 (18) to 0x1000 (4096)

        Time = N * 0.625 msec

        Time Range: 11.25 ms (=0.625 * 18) to 2560 ms (=0.625 * 4096)
        Time Default: 2.56 sec

        e.g.) .interval=10, .window=5
        interval : 10 * 0.625 = 6.25  ms
        window   :  5 * 0.625 = 3.125 ms
        >>> this means every 6.25 ms, scan for 3.125 ms
    */
    message->interval = 10;
    message->window = 5;

    BleSendMsg((void *)message);

    return;
}


void app_connect(unsigned char indx)
{
    struct gapm_start_connection_cmd *message;
    /*
    if (app_env.devices[indx].free == true)
    {
        return;
    }*/
    message = (struct gapm_start_connection_cmd *) BleMsgAlloc(GAPM_START_CONNECTION_CMD,
                                                               TASK_ID_GAPM,
                                                               TASK_ID_GTL,
                                                               sizeof(struct gapm_start_connection_cmd ));

    message->nb_peers = 1;
    memcpy((void *) &message->peers[0].addr, (void *)&app_env.devices[indx].adv_addr.addr, BD_ADDR_LEN);
    message->con_intv_min = 100;
    message->con_intv_max = 100;
    message->ce_len_min = 0x0;
    message->ce_len_max = 0x5;
    message->con_latency = 0;
    message->op.addr_src = GAPM_STATIC_ADDR;
    message->peers[0].addr_type = app_env.devices[indx].adv_addr_type;
    message->superv_to = 0x1F4; // 5000 ms ;
    message->scan_interval = 0x180;
    message->scan_window = 0x160;
    message->op.code = GAPM_CONNECTION_DIRECT;

    BleSendMsg((void *) message);
}


void app_read_rssi(void)
{
    int i;

    for (i = 0; i < MAX_CONN_NUMBER; i++) {
        if (app_env.proxr_device[i].isactive == true) {
            struct gapc_get_info_cmd *req = BleMsgAlloc(GAPC_GET_INFO_CMD,
                                                        KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[i].device.conidx),
                                                        TASK_ID_GTL,
                                                        sizeof(struct gapc_get_info_cmd));
            req->operation = GAPC_GET_CON_RSSI;
            BleSendMsg((void *) req);
        }
    }
}

void app_read_temp_value(void)
{
    int i;

    for (i = 0; i < MAX_CONN_NUMBER; i++) {
        if (app_env.proxr_device[i].isactive == true) {
            app_characteristic_read(app_env.proxr_device[i].device.conidx,
                                    app_env.proxr_device[i].iot_sensor.temp_char_val_handle);

        }
    }
}


void app_disconnect(uint8_t con_idx)
{

    struct gapc_disconnect_cmd *req = BleMsgAlloc(GAPC_DISCONNECT_CMD,
                                                  KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[con_idx].device.conidx),
                                                  TASK_ID_GTL,
                                                  sizeof(struct gapc_disconnect_cmd));

    req->operation = GAPC_DISCONNECT;
    req->reason = CO_ERROR_REMOTE_USER_TERM_CON;


    BleSendMsg((void *) req);
}


void app_security_enable(uint8_t con_idx)
{
    // Allocate the message
    struct gapc_bond_cmd *req = BleMsgAlloc(GAPC_BOND_CMD,
                                            KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[con_idx].device.conidx),
                                            TASK_ID_GTL,
                                            sizeof(struct gapc_bond_cmd));


    req->operation = GAPC_BOND;

    req->pairing.sec_req = GAP_NO_SEC;  //GAP_SEC1_NOAUTH_PAIR_ENC;

    // OOB information
    req->pairing.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;

    // IO capabilities
    req->pairing.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;

    // Authentication requirements
    req->pairing.auth           = GAP_AUTH_REQ_NO_MITM_BOND; //SMP_AUTH_REQ_NO_MITM_NO_BOND;

    // Encryption key size
    req->pairing.key_size       = 16;

    //Initiator key distribution
    req->pairing.ikey_dist      = 0x04; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    //Responder key distribution
    req->pairing.rkey_dist      = 0x03; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    // Send the message
    BleSendMsg(req);
}


void app_connect_confirm(uint8_t auth, uint8_t con_idx)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = BleMsgAlloc(GAPC_CONNECTION_CFM,
                                                  KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[con_idx].device.conidx),
                                                  TASK_ID_GTL,
                                                  sizeof (struct gapc_connection_cfm));

    cfm->auth = auth;

    // Send the message
    BleSendMsg(cfm);
}


void app_gen_csrk(uint8_t con_idx)
{
    // Counter
    uint8_t i;

    // Randomly generate the CSRK
    for (i = 0; i < KEY_LEN; i++) {
        app_env.proxr_device[con_idx].csrk.key[i] = (((KEY_LEN) < (16 - i)) ? 0 : rand() % 256);
    }

}

void app_gap_bond_cfm(uint8_t con_idx)
{
    struct gapc_bond_cfm *req = BleMsgAlloc(GAPC_BOND_CFM,
                                            KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[con_idx].device.conidx),
                                            TASK_ID_GTL,
                                            sizeof(struct gapc_bond_cfm));

    req->request = GAPC_CSRK_EXCH;
    req->accept = 0;
    req->data.pairing_feat.auth = GAP_AUTH_REQ_NO_MITM_BOND;
    req->data.pairing_feat.sec_req = GAP_SEC2_NOAUTH_DATA_SGN;

    app_gen_csrk(con_idx);

    memcpy(req->data.csrk.key, app_env.proxr_device[con_idx].csrk.key, KEY_LEN);

    // Send the message
    BleSendMsg(req);

}


void app_start_encryption(uint8_t con_idx)
{
    // Allocate the message
    struct gapc_encrypt_cmd *req = BleMsgAlloc(GAPC_ENCRYPT_CMD,
                                               KE_BUILD_ID(TASK_ID_GAPC, app_env.proxr_device[con_idx].device.conidx),
                                               TASK_ID_GTL,
                                               sizeof(struct gapc_encrypt_cmd));


    req->operation = GAPC_ENCRYPT;
    memcpy(&req->ltk.ltk, &app_env.proxr_device[con_idx].ltk, sizeof(struct gapc_ltk));

    // Send the message
    BleSendMsg(req);

}


void app_proxm_enable(uint8_t con_idx)
{

    // Allocate the message
    struct proxm_enable_req *req = BleMsgAlloc(PROXM_ENABLE_REQ,
                                               KE_BUILD_ID(TASK_ID_PROXM, app_env.proxr_device[con_idx].device.conidx),
                                               TASK_ID_GTL,
                                               sizeof(struct proxm_enable_req));

    // Fill in the parameter structure
    req->con_type = PRF_CON_DISCOVERY;

    // Send the message
    BleSendMsg((void *) req);
}


void app_proxm_read_llv(uint8_t con_idx)
{
    DBG_INFO("app_env.proxr_device[%d].device.conidx = %d \n", con_idx, app_env.proxr_device[con_idx].device.conidx);
    DBG_INFO("2nd param = %d \n", KE_BUILD_ID(TASK_ID_PROXM, app_env.proxr_device[con_idx].device.conidx));

    struct proxm_rd_req *req = BleMsgAlloc(PROXM_RD_REQ,
                                           KE_BUILD_ID(TASK_ID_PROXM, app_env.proxr_device[con_idx].device.conidx),
                                           TASK_ID_GTL,
                                           sizeof(struct proxm_rd_req));

    last_char[con_idx] = PROXM_RD_LL_ALERT_LVL;
    req->svc_code = PROXM_RD_LL_ALERT_LVL;

    BleSendMsg((void *) req);

}


void app_proxm_read_txp(uint8_t con_idx)
{
    struct proxm_rd_req *req = BleMsgAlloc(PROXM_RD_REQ,
                                           KE_BUILD_ID(TASK_ID_PROXM, app_env.proxr_device[con_idx].device.conidx),
                                           TASK_ID_GTL,
                                           sizeof(struct proxm_rd_req));

    last_char[con_idx] = PROXM_RD_TX_POWER_LVL;
    req->svc_code = PROXM_RD_TX_POWER_LVL;

    BleSendMsg((void *) req);
}


void app_proxm_write(unsigned int chr, unsigned char val, uint8_t con_idx)
{
    struct proxm_wr_alert_lvl_req *req = BleMsgAlloc(PROXM_WR_ALERT_LVL_REQ,
                                                     KE_BUILD_ID(TASK_ID_PROXM, app_env.proxr_device[con_idx].device.conidx),
                                                     TASK_ID_GTL,
                                                     sizeof(struct proxm_wr_alert_lvl_req));

    req->svc_code = chr;
    req->lvl = val;
    BleSendMsg((void *) req);

}


void app_disc_enable( uint8_t con_idx )
{
    // Allocate the message
    struct disc_enable_req *req = BleMsgAlloc(DISC_ENABLE_REQ,
                                              KE_BUILD_ID(TASK_ID_DISC, app_env.proxr_device[con_idx].device.conidx),
                                              TASK_ID_GTL,
                                              sizeof(struct disc_enable_req));

    // Fill in the parameter structure
    req->con_type = PRF_CON_DISCOVERY;

    // Send the message
    BleSendMsg((void *) req);

}


void app_disc_rd_char(uint8_t char_code, uint8_t con_idx)
{
    struct disc_rd_char_req *req = BleMsgAlloc(DISC_RD_CHAR_REQ,
                                               KE_BUILD_ID(TASK_ID_DISC, app_env.proxr_device[con_idx].device.conidx),
                                               TASK_ID_GTL,
                                               sizeof(struct disc_rd_char_req));

    req->char_code = char_code;
    BleSendMsg((void *) req);

}


bool bdaddr_compare(struct bd_addr *bd_address1, struct bd_addr *bd_address2)
{
    unsigned char idx;

    for (idx = 0; idx < BD_ADDR_LEN; idx++) {
        /// checks if the addresses are similar
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

void app_ble_sw_reset(void)
{
    // Allocate a message for BLE_AUX
    void *cmd = BleMsgAlloc(APP_BLE_SW_RESET, TASK_ID_EXT_HOST_BLE_AUX, TASK_ID_GTL, 0);

    // Send the message
    BleSendMsg(cmd);
}

void app_reset_app_data(void)
{
    int cnt;
    app_env.cur_dev = 0;

    for (cnt = 0; cnt < MAX_CONN_NUMBER; cnt++) {
        app_env.proxr_device[cnt].isactive = false; //Sets all slots as inactive

        //Sets default value for the TxP and LLA indicator of each conn
        last_char[cnt] = 0xFF;
    }
}


void ConsoleEvent_handler(console_msg *message)
{
    switch (message->type) {
        case CONSOLE_DEV_DISC_CMD:
        {
            DBG_INFO("CONSOLE_DEV_DISC_CMD\n");

            if (connected_to_at_least_one_peer) {
                app_cancel_gap();
                vTaskDelay(10);
                app_inq();
            }

            if (app_env.state == APP_IDLE) {
                app_env.state = APP_SCAN;
                ConsoleScan();
                app_inq();
            } else if (app_env.state == APP_SCAN) {
                app_rst_gap();
            }
        } break;

        case CONSOLE_CONNECT_CMD:
        {
            DBG_INFO("CONSOLE_CONNECT_CMD\n");
            DBG_INFO("app_env.state = %d, connected_to_at_least_one_peer = %d \n", app_env.state, connected_to_at_least_one_peer );
            DBG_INFO("msg->idx = %d \n", message->idx );

            if ((app_env.state == APP_IDLE) || connected_to_at_least_one_peer) {
                DBG_INFO("app_env.state = %d, connected_to_at_least_one_peer = %d \n", app_env.state, connected_to_at_least_one_peer);
                app_connect(message->idx);
            } else if (app_env.state == APP_SCAN) {
                DBG_INFO("app_env.state == APP_SCAN \n");
                app_cancel();
                ConsoleSendConnnect(message->idx);
            }

        } break;

#if defined (__WFSVC_ENABLE__)
        case CONSOLE_ENABLE_WFSVC_CMD:
        {
            DBG_INFO("CONSOLE_ENABLE_WFSVC_CMD\n");
            app_wf_prov_mode_enable();
            app_reset_app_data();
            app_rst_gap();
        } break;
#endif
        case CONSOLE_RD_LLV_CMD:
        {
            DBG_INFO("CONSOLE_RD_LLV_CMD\n");
            DBG_INFO("app_env.state = %d, proxm_trans_in_prog = %d \n", app_env.state, proxm_trans_in_prog);
            DBG_INFO("msg->idx = %d \n", message->idx );

            if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog) {
                DBG_INFO("app_proxm_read_llv(%d) \n", message->idx);
                proxm_trans_in_prog = 3;
                app_proxm_read_llv(message->idx);
            }
        } break;

        case CONSOLE_RD_TXP_CMD:
        {
            DBG_INFO("CONSOLE_RD_TXP_CMD\n");
            DBG_INFO("app_env.state = %d, proxm_trans_in_prog = %d \n", app_env.state, proxm_trans_in_prog);
            DBG_INFO("msg->idx = %d \n", message->idx );

            if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog) {
                proxm_trans_in_prog = 3;
                app_proxm_read_txp(message->idx);
            }
        } break;

        case CONSOLE_WR_LLV_CMD:
        {
            DBG_INFO("CONSOLE_WR_LLV_CMD\n");
            DBG_INFO("app_env.state = %d, proxm_trans_in_prog = %d \n", app_env.state, proxm_trans_in_prog);
            DBG_INFO("msg->idx = %d \n", message->idx );

            if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog) {
                proxm_trans_in_prog = 3;
                app_proxm_write(PROXM_SET_LK_LOSS_ALERT, message->val, message->idx);
            }

        } break;

        case CONSOLE_WR_IAS_CMD:
        {
            DBG_INFO("CONSOLE_WR_IAS_CMD\n");
            DBG_INFO("app_env.state = %d, proxm_trans_in_prog = %d \n", app_env.state, proxm_trans_in_prog);
            DBG_INFO("msg->idx = %d \n", message->idx );

            if (app_env.state == APP_CONNECTED && !proxm_trans_in_prog) {
                proxm_trans_in_prog = 3;
                app_proxm_write(PROXM_SET_IMMDT_ALERT, message->val, message->idx);
            }
        } break;

        case CONSOLE_DISCONNECT_CMD:
        {
            DBG_INFO("CONSOLE_DISCONNECT_CMD\n");
            DBG_INFO("msg->idx = %d \n", message->idx );

            if (app_env.state == APP_CONNECTED) {
                app_disconnect(message->val);
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

        case CONSOLE_RD_RSSI_CMD:
        {
            DBG_INFO("CONSOLE_RD_RSSI_CMD\n");

            if (app_env.state == APP_CONNECTED) {
                app_read_rssi();
            }
        } break;

        case CONSOLE_BLE_SW_RESET_CMD:
        {
            DBG_INFO("CONSOLE_BLE_SW_RESET_CMD\n");
            app_ble_sw_reset();
        } break;

        case CONSOLE_IOT_SENSOR_TEMP_RD_CMD:
        {
            DBG_INFO("CONSOLE_IOT_SENSOR_TEMP_RD_CMD\n");

            if (app_env.state == APP_CONNECTED) {
                app_read_temp_value();
            }

        } break;

        case CONSOLE_IOT_SENSOR_TEMP_POST_CTL_CMD:
        {
            DBG_INFO("CONSOLE_IOT_SENSOR_TEMP_POST_CTL_CMD\n");

            if (app_env.state == APP_CONNECTED) {
                uint8_t v[2];
                v[1] = 0x00;

                if (message->val == IOT_SENSOR_TEMP_POST_ENABLE) {
                    DBG_INFO("Enable Temperature posting ...\n");
                    v[0] = 0x01;
                    app_env.proxr_device[message->idx].iot_sensor.iot_sensor_act_sta = IOT_SENSOR_WR_EN_NOTI;
                } else if (message->val == IOT_SENSOR_TEMP_POST_DISABLE) {
                    DBG_INFO("Disable Temperature posting ...\n");
                    v[0] = 0x00;
                    app_env.proxr_device[message->idx].iot_sensor.iot_sensor_act_sta = IOT_SENSOR_WR_DIS_NOTI;
                }

                noti_cnt[message->idx] = 0; // reset noti count
                app_characteristic_write(app_env.proxr_device[message->idx].device.conidx,
                                         app_env.proxr_device[message->idx].iot_sensor.temp_cccd_hdl,
                                         v, 2);
                DBG_INFO(">>> GATTC_WRITE (on temperature value CCCD) \n");
            }

        } break;

        case CONSOLE_EXIT_CMD:
        {
            DBG_INFO("CONSOLE_EXIT_CMD\n");
            app_rst_gap();
            vTaskDelay(10);
            app_exit();
        } break;

        default:
        {
        } break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles commands sent from console interface.
 *
 * @return void.
 ****************************************************************************************
 */
void ConsoleEvent(void)
{
#ifdef __FREERTOS__
    msg queue_msg;

    if (msg_queue_get(&ConsoleQueue, &queue_msg, portMAX_DELAY) == OS_QUEUE_EMPTY) {
        configASSERT(0);
    }
    console_msg *message = (console_msg *)queue_msg.data;
    if (message != NULL){
        ConsoleEvent_handler(message);
    }
    msg_release(&queue_msg);

#else
    console_msg *message;
    if (ConsoleQueue.First != NULL) {
        xSemaphoreTake(ConsoleQueueMut, portMAX_DELAY);
        message = (console_msg *) DeQueue(&ConsoleQueue);
        xSemaphoreGive(ConsoleQueueMut);

        ConsoleEvent_handler(message);
        MEMFREE(message);
    }
#endif    
}

unsigned int rtrn_fst_avail(void)
{
    int i;

    for (i = 0; i < MAX_CONN_NUMBER; i++) {
        if (app_env.proxr_device[i].isactive == false) {
            return i;
        }
    }

    return MAX_CONN_NUMBER;
}


unsigned int rtrn_prev_avail_conn(void)
{
    unsigned int icnt = 0;

    for (icnt = app_env.cur_dev; icnt > 0; icnt--) {
        if (app_env.proxr_device[icnt].isactive == true) {
            return icnt;
        }
    }

    for (icnt = MAX_CONN_NUMBER - 1; icnt > app_env.cur_dev; icnt--) {
        if (app_env.proxr_device[icnt].isactive == true) {
            return icnt;
        }
    }

    return 0;
}


unsigned int rtrn_con_avail(void)
{
    int i, con_avail = 0;

    for (i = 0; i < MAX_CONN_NUMBER; i++) {
        if (app_env.proxr_device[i].isactive == true) {
            con_avail++;
        }
    }

    return con_avail;
}

// stop the non-connectable advertising so that an undirect advertising can start
void app_cancel_gap(void)
{
    struct gapm_cancel_cmd *message = BleMsgAlloc(GAPM_CANCEL_CMD,
                                                  TASK_ID_GAPM,
                                                  TASK_ID_GTL,
                                                  sizeof(struct gapm_cancel_cmd));


    message->operation = GAPM_CANCEL;

    BleSendMsg((void *) message);
}


unsigned short res_conidx(unsigned short cidx)
{
    int i;

    for (i = 0; i < MAX_SCAN_DEVICES; i++) {
        if (app_env.proxr_device[i].device.conidx == cidx) {
            return i;
        }
    }

    return MAX_SCAN_DEVICES;
}


unsigned short res_conhdl(uint16_t conhdl)
{
    int i;

    for (i = 0; i < MAX_SCAN_DEVICES; i++) {
        if (app_env.proxr_device[i].device.conhdl == conhdl) {
            return i;
        }
    }

    return MAX_SCAN_DEVICES;
}

void app_read_rssi_on_conn_dev(void)
{
    if (app_env.state == APP_CONNECTED) {
        poll_count++;

        if (poll_count == 200) {  // rssi check every 200 x 20ms = 4000ms (4s)
            poll_count = 0;
            app_read_rssi();
        }
    }

    vTaskDelay(2);
}

#if defined (__GTL_IFC_UART__)
void gtl_host_ifc_mon(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    UARTProc();
}
#endif

/**
 ****************************************************************************************
 * @brief Application's main function.
 *
 ****************************************************************************************
 */

void gtl_main(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int status;
    
    // init
    InitTasks();

#if defined(__GTL_IFC_UART__)
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
    app_reset_app_data();

    ConsoleTitle();

    app_rst_gap();

    // GTL message loop
    while (1) {
#if defined(__GTL_IFC_UART__)
        BleReceiveMsg();
#endif

        if (proxm_trans_in_prog > 0) {
            proxm_trans_in_prog--;
        }
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
 *    @brief init ble interface and starts main GTL message handler
 */
void gtl_init(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    unsigned int status = ERR_OK;

    extern unsigned char  ble_combo_ref_flag;
    if (ble_combo_ref_flag == pdFALSE) {
        vTaskDelete(NULL);
        return;
    }

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


/**
 ****************************************************************************************
 * @brief Discover a characteristic by 128-bit UUID
 *
 * @param[in] conidx    Connection index for the connected peer device.
 * @param[in] uuid_128    128 bit uuid of the characteristic.

 * @return void.
 ****************************************************************************************
 */
void app_discover_char_by_uuid_128(uint16_t conidx, const uint8_t *uuid_128)
{
    // discover a char by uuid_128
    struct gattc_disc_cmd *svc_req = BleMsgAlloc(GATTC_DISC_CMD,             // cmd id
                                                 KE_BUILD_ID(TASK_ID_GATTC, conidx),     // dst
                                                 TASK_ID_GTL,                             // src
                                                 sizeof(struct gattc_disc_cmd) + ATT_UUID_128_LEN);     // len

    //gatt request type: by UUID
    svc_req->operation = GATTC_DISC_BY_UUID_CHAR;

    //start handle;
    svc_req->start_hdl = ATT_1ST_REQ_START_HDL;

    //end handle
    svc_req->end_hdl   = ATT_1ST_REQ_END_HDL;

    // UUID search
    svc_req->uuid_len = ATT_UUID_128_LEN;

    // set 128 bit UUID for searching
    memcpy(&svc_req->uuid[0], uuid_128, (sizeof(uint8_t) * ATT_UUID_128_LEN));

    //send the message to GATT, which will send back the response when it gets it
    BleSendMsg((void *)svc_req);
}

void app_discover_svc_by_uuid_128(uint16_t conidx, const uint8_t *uuid_128)
{
    struct gattc_disc_cmd *req = BleMsgAlloc(GATTC_DISC_CMD, 
                                             KE_BUILD_ID(TASK_ID_GATTC,conidx), 
                                             TASK_ID_GTL,
                                             sizeof(struct gattc_disc_cmd) + ATT_UUID_128_LEN /*uuid length*/);

    req->operation = GATTC_DISC_BY_UUID_SVC;
    req->start_hdl = ATT_1ST_REQ_START_HDL;
    req->end_hdl = ATT_1ST_REQ_END_HDL;

    // UUID search
    req->uuid_len = ATT_UUID_128_LEN;

    req->seq_num = 0;

    // set 128 bit UUID for searching
    memcpy(&req->uuid[0], uuid_128, (sizeof(uint8_t) * ATT_UUID_128_LEN));

    BleSendMsg((void *) req);
}

void app_discover_all_char_uuid_128(uint16_t conidx, uint16_t start_handle,
                                    uint16_t end_handle)
{
    struct gattc_disc_cmd *req = BleMsgAlloc(GATTC_DISC_CMD, 
                                             KE_BUILD_ID(TASK_ID_GATTC, conidx), 
                                             TASK_ID_GTL,
                                             sizeof(struct gattc_disc_cmd) + ATT_UUID_128_LEN /*uuid length*/);

    req->operation = GATTC_DISC_ALL_CHAR;
    req->uuid_len = ATT_UUID_128_LEN; // 128 bit UUID
    req->start_hdl = start_handle;
    req->end_hdl = end_handle;

    memset(req->uuid, 0x00, ATT_UUID_128_LEN);

    BleSendMsg((void *) req);
}

void app_discover_char_desc_uuid_128(uint16_t conidx, uint16_t start_handle,
                                     uint16_t end_handle)
{
    struct gattc_disc_cmd *req = BleMsgAlloc(GATTC_DISC_CMD, 
                                             KE_BUILD_ID(TASK_ID_GATTC, conidx), 
                                             TASK_ID_GTL,
                                             sizeof(struct gattc_disc_cmd) + ATT_UUID_128_LEN /*uuid length*/);

    req->operation = GATTC_DISC_DESC_CHAR;
    req->uuid_len = ATT_UUID_128_LEN; // 128 bit UUID
    req->start_hdl = start_handle;
    req->end_hdl = end_handle;

    memset(req->uuid, 0x00, ATT_UUID_128_LEN);

    BleSendMsg((void *) req);
}

void app_characteristic_read(uint16_t conidx, uint16_t char_val_handle)
{
    struct gattc_read_cmd *req = BleMsgAlloc(GATTC_READ_CMD, 
                                             KE_BUILD_ID(TASK_ID_GATTC, conidx), 
                                             TASK_ID_GTL,
                                             sizeof(struct gattc_read_cmd) );

    req->operation = GATTC_READ;
    req->nb = 0;
    req->req.simple.handle = char_val_handle;
    req->req.simple.offset = 0;
    req->req.simple.length = 0;

    BleSendMsg((void *) req);
}

uint8_t app_get_slot_id_from_conidx(uint8_t conidx)
{

    int i;

    for (i = 0; i < MAX_CONN_NUMBER; i++) {
        if (app_env.proxr_device[i].isactive == true && app_env.proxr_device[i].device.conidx == conidx) {
            return i;
        }
    }

    return i;
}

/**
 ****************************************************************************************
 * @brief  Starts GATT MTU size negotiation
 *
 ****************************************************************************************
 */
void app_start_gatt_mtu_negotiation()
{
    struct gattc_exc_mtu_cmd *req = BleMsgAlloc(GATTC_EXC_MTU_CMD, TASK_ID_GATTC, 
                                                TASK_ID_GTL,
                                                sizeof(struct gattc_exc_mtu_cmd));

    req->operation = GATTC_MTU_EXCH;
    req->seq_num = 0;

    BleSendMsg((void *) req);
}


int gattc_mtu_changed_ind_handler (ke_msg_id_t msgid,
                                   struct gattc_mtu_changed_ind *param,
                                   ke_task_id_t dest_id,
                                   ke_task_id_t src_id)
{
    DA16X_UNUSED_ARG(msgid);
    DA16X_UNUSED_ARG(dest_id);
    DA16X_UNUSED_ARG(src_id);

    DBG_INFO("GATT MTU size set to : %d\n", param->mtu);

    // update tx payload size if needed

    return (KE_MSG_CONSUMED);
}

void app_characteristic_write(uint16_t conidx, uint16_t char_val_handle, uint8_t *value,
                              uint8_t value_length)
{
    int kk = 0;
    struct gattc_write_cmd *req = BleMsgAlloc(GATTC_WRITE_CMD,
                                              KE_BUILD_ID(TASK_ID_GATTC, conidx), TASK_ID_GTL,
                                              sizeof(struct gattc_write_cmd) + value_length);

    req->operation = GATTC_WRITE;
    req->auto_execute = 1;
    req->handle = char_val_handle;
    req->offset = 0;
    req->length = value_length;
    req->cursor = 0;

    for (kk = 0; kk < value_length; ++kk) {
        req->value[kk] = value[kk];
    }

    BleSendMsg((void *) req);
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
#endif //(NVDS_SUPPORT)
    {
        cmd->info.host.adv_data_len = APP_DFLT_ADV_DATA_LEN;
        memcpy(&cmd->info.host.adv_data[0], APP_DFLT_ADV_DATA, cmd->info.host.adv_data_len);
    }

    // Scan Response Data
#if (NVDS_SUPPORT)

    if (nvds_get(NVDS_TAG_APP_BLE_SCAN_RESP_DATA, &cmd->info.host.scan_rsp_data_len,
                 &cmd->info.host.scan_rsp_data[0]) != NVDS_OK)
#endif //(NVDS_SUPPORT)
    {
        cmd->info.host.scan_rsp_data_len = APP_SCNRSP_DATA_LENGTH;
        memcpy(&cmd->info.host.scan_rsp_data[0], APP_SCNRSP_DATA, cmd->info.host.scan_rsp_data_len);
    }

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    device_name_avail_space = APP_ADV_DATA_MAX_SIZE - cmd->info.host.adv_data_len - 2;

    // Check if data can be added to the Advertising data
    if (device_name_avail_space > 0) {
        // Get the Device Name to add in the Advertising Data (Default one or NVDS one)
#if (NVDS_SUPPORT)
        device_name_length = NVDS_LEN_DEVICE_NAME;

        if (nvds_get(NVDS_TAG_DEVICE_NAME, &device_name_length, &device_name_temp_buf[0]) != NVDS_OK)
#endif //(NVDS_SUPPORT)
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
    BleSendMsg(cmd);

    return;
}


#if defined (__WFSVC_ENABLE__)
void app_wf_prov_mode_enable()
{
    wifi_svc_enabled = true;
}

bool app_wf_prov_mode_is_enabled(void)
{
    return wifi_svc_enabled;
}
#endif

#if defined(__SUPPORT_ATCMD__)
int atcmd_ble_set_device_name_handler(uint8_t *name)
{
    uint8_t length = 0;

    if (name == NULL) {
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    length = strlen((char *)name);
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

#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */

