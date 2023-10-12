/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Type definitions for application
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

#ifndef APP_H_
#define APP_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>            // standard integer
#include "gapc_task.h"
#include "gapm_task.h"
#include "gap.h"
#include "disc.h"
#include "da16x_types.h"
#include "app_task.h"
#include "event_groups.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define MAX_SCAN_DEVICES                 9
#define RSSI_SAMPLES                     5
#define MAX_CONN_NUMBER                  3    //8

// borrowed from ble suota sample application; optimization point
#define MAX_LE_PKT_SIZE                  (251)
#define MAX_LE_TX_TIME                   (2120)
#define MAX_GATT_MTU_SIZE                (247)
#define ATT_HEADER_SIZE                  (7)
#define DEFAULT_DATA_CHUNK_SIZE          (20)

#define DEMO_PEER_MAX_MTU                (251)
#define ATT_HEADER_SIZE                  (7)
#define MY_MAX_MTU                       (251)
#define MY_MAX_MTU_TIME                  (2120)

#define MS_TO_BLESLOTS(x)                ((int)((x)/0.625))
#define MS_TO_DOUBLESLOTS(x)             ((int)((x)/1.25))
#define MS_TO_TIMERUNITS(x)              ((int)((x)/10))

/* Advertising and Connection related parameters */
#define USER_CFG_DEFAULT_ADV_INTERVAL_MS (687.5)

#define CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR_UUID 0x2902

/* advertising relevant defines */
/// Local address type
#define APP_ADDR_TYPE                    0
/// Advertising channel map
#define APP_ADV_CHMAP                    0x07
/// Advertising filter policy
#define APP_ADV_POL                      0
/// Advertising minimum interval
#define APP_ADV_INT_MIN                  MS_TO_BLESLOTS(USER_CFG_DEFAULT_ADV_INTERVAL_MS)
/// Advertising maximum interval
#define APP_ADV_INT_MAX                  MS_TO_BLESLOTS(USER_CFG_DEFAULT_ADV_INTERVAL_MS)
/// Advertising data maximal length
#define APP_ADV_DATA_MAX_SIZE            (ADV_DATA_LEN - 3)
/// Scan Response data maximal length
#define APP_SCAN_RESP_DATA_MAX_SIZE      (SCAN_RSP_DATA_LEN)

#define APP_DFLT_ADV_DATA                "\x07\x03\x03\x18\x02\x18\x04\x18"
#define APP_DFLT_ADV_DATA_LEN            (8)

#define APP_SCNRSP_DATA                  "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LENGTH           (10)


// 128-bit UUID (stored in LE order)
typedef uint8_t uuid_128_t[16];

#define UUID_128_LE(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0) \
    {b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}

/// Maximal length for Characteristic values - 18
#define DIS_VAL_MAX_LEN                  (0x12)

///System ID string length
#define DIS_SYS_ID_LEN                   (0x08)
///IEEE Certif length (min 6 bytes)
#define DIS_IEEE_CERTIF_MIN_LEN          (0x06)
///PnP ID length
#define DIS_PNP_ID_LEN                   (0x07)

#define KE_FIRST_MSG_BLE(task)           ((ke_msg_id_t)((task) << 8))

typedef enum
{
    APP_GEN_RAND_REQ = KE_FIRST_MSG_BLE(TASK_ID_EXT_HOST_BLE_AUX) + 1,  // 0xA000 + 1
    APP_GEN_RAND_RSP,       //2
    APP_STATUS_IND,         //3
    APP_SET_TXPOWER,        //4
    APP_SET_TXPOWER_CFM,    //5
    APP_SET_REG_VALUE,      //6
    APP_SET_REG_VALUE_CFM,  //7
    APP_GET_REG_VALUE,      //8
    APP_GET_REG_VALUE_RSP,  //9
    APP_STATS_IND,          //10 0x0A
    APP_DEV_RESET_IND,      //11 0x0B
    APP_MTU_CHANGED_IND,    //12 0x0C
    APP_DBG_CONFIG_CMD,     //13 0x0D
    //APP_STATS_CONFIG_CMD,
    //APP_CRC_ERROR_IND,    //missing
    APP_ECHO,               //14 0x0E
    APP_GET_FW_VERSION,     //15 0x0F
    APP_FW_VERSION_IND,     //16 0x10
    APP_GET_DEBUG_DUMP,     //17 0x11
    APP_GET_DEBUG_DUMP_RSP, //18 0x12
    APP_GTL_COM_ERR_IND,    //19 0x13

    APP_SVC_CHANGED_CFG_CMD = KE_FIRST_MSG_BLE(TASK_EXT_HOST_BLE_AUX) + 38,
    APP_SVC_CHANGED_CFG_CFM = KE_FIRST_MSG_BLE(TASK_EXT_HOST_BLE_AUX) + 39,

    APP_STATS_TIMER_ID = KE_FIRST_MSG_BLE(TASK_EXT_HOST_BLE_AUX) + 40,
    APP_HEARTBEAT_TIMER_ID,

    APP_BLE_SW_RESET,

    APP_GAS_LEAK_SENSOR_START,
    APP_GAS_LEAK_SENSOR_START_CFM,
    APP_GAS_LEAK_SENSOR_STOP,
    APP_GAS_LEAK_SENSOR_STOP_CFM,
    APP_GAS_LEAK_EVT_IND,
    APP_GAS_LEAK_SENSOR_RD_TIMER_ID,

    APP_CUSTOM_COMMANDS_LAST,
} ext_host_ble_aux_task_msg_t;

typedef enum
{
    BLE_BOOT_MODE_0, // normal boot: boot_ble_from_uart(O), app_rst_gap(O)
    BLE_BOOT_MODE_1, // dpm wakeup : boot_ble_from_uart(X), app_rst_gap(X)
    BLE_BOOT_MODE_3, // 
    BLE_BOOT_MODE_LAST
} ble_boot_mode_t;

typedef enum
{
    DPM_BOOT_TYPE_NO_DPM,
    DPM_BOOT_TYPE_NORMAL,
    DPM_BOOT_TYPE_WAKEUP_CARE,
    DPM_BOOT_TYPE_WAKEUP_DONT_CARE,

    DPM_BOOT_TYPE_LAST
} dpm_boot_type_t;

typedef struct
{
    unsigned char free;
    struct bd_addr adv_addr;
    unsigned short conidx;
    unsigned short conhdl;
    unsigned char idx;
    CHAR  rssi;
    unsigned char  data_len;
    unsigned char  data[ADV_DATA_LEN + 1];
    unsigned char  adv_addr_type;
} ble_dev;

typedef struct
{
    uint16_t    val_hdl;
    char        val[DIS_VAL_MAX_LEN + 1];
    uint16_t    len;
} dis_char;

//DIS information
typedef struct
{
    dis_char chars[DISC_CHAR_MAX];
} dis_env;


enum iot_sensor_act_status
{
    IOT_SENSOR_UNDEF,
    IOT_SENSOR_WR_EN_NOTI,
    IOT_SENSOR_WR_DIS_NOTI
};

typedef struct
{
    uint8_t svc_found;
    uint16_t svc_start_handle;
    uint16_t svc_end_handle;

    uint16_t temp_char_val_handle; // temperature char value handle

    uint16_t temp_cccd_hdl; // temperature cccd handle
    uint8_t temp_post_enabled; // indicates whether or not notify/indicate is enabled

    enum iot_sensor_act_status iot_sensor_act_sta; // see enum iot_sensor_act_status

    uint8_t temp_val[1]; // temperature value
} iot_sensor_svc_env;

//Proximity Reporter connected device
typedef struct
{
    ble_dev device;
    unsigned char bonded;
    unsigned short ediv;
    struct rand_nb rand_nb[RAND_NB_LEN];
    struct gapc_ltk ltk;
    struct gapc_irk irk;
    struct gap_sec_key csrk;
    unsigned char llv;
    CHAR txp;
    CHAR rssi[RSSI_SAMPLES];
    UCHAR rssi_indx;
    CHAR avg_rssi;
    unsigned char alert;
    dis_env dis;
    unsigned char isactive; // if connected to a monitor, it is set to true

    iot_sensor_svc_env iot_sensor;
} proxr_dev;

/// application environment structure
struct app_env_tag
{
    unsigned char state;
    unsigned char num_of_devices;
    unsigned int cur_dev;
    ble_dev devices[MAX_SCAN_DEVICES];
    proxr_dev proxr_device[MAX_CONN_NUMBER];
};

/// Device name structure
struct app_device_name
{
    /// Length for name
    uint8_t length;
    /// Array of bytes for name
    uint8_t name[GAP_MAX_NAME_SIZE];
};

/// Device info structure
struct app_device_info
{
    /// Device Name
    struct app_device_name dev_name;

    /// Device Appearance Icon
    uint16_t appearance;
};


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Device name write permissions requirements
enum device_name_write_perm
{
    NAME_WRITE_DISABLE  = (0x00 << GAPM_POS_ATT_NAME_PERM),
    NAME_WRITE_ENABLE   = (0x01 << GAPM_POS_ATT_NAME_PERM),
    NAME_WRITE_UNAUTH   = (0x02 << GAPM_POS_ATT_NAME_PERM),
    NAME_WRITE_AUTH     = (0x03 << GAPM_POS_ATT_NAME_PERM),
    /// NAME_WRITE_SECURE is not supported by SDK
};

/// Device appearance write permissions requirements
enum device_appearance_write_perm
{
    APPEARANCE_WRITE_DISABLE  = (0x00 << GAPM_POS_ATT_APPEARENCE_PERM),
    APPEARANCE_WRITE_ENABLE   = (0x01 << GAPM_POS_ATT_APPEARENCE_PERM),
    APPEARANCE_WRITE_UNAUTH   = (0x02 << GAPM_POS_ATT_APPEARENCE_PERM),
    APPEARANCE_WRITE_AUTH     = (0x03 << GAPM_POS_ATT_APPEARENCE_PERM),
    /// APPEARANCE_WRITE_SECURE is not supported by SDK
};

/*
 * EXTERNAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;

/// Device info
extern struct app_device_info device_info;

extern const uuid_128_t IOT_SENSOR_SVC_UUID;
extern const uuid_128_t IOT_SENSOR_SVC_TEMP_CHAR_UUID;
extern const uuid_128_t IOT_SENSOR_SVC_TEMP_CCCD_UUID;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 ****************************************************************************************
 * @brief Exit application.
 *
 * @return void.
 ****************************************************************************************
 */
void app_exit(void);
/**
 ****************************************************************************************
 * @brief DISC add profile.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disc_create(void);
/**
 ****************************************************************************************
 * @brief PROXM add profile.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_create(void);
/**
 ****************************************************************************************
 * @brief Configure device mode either to Central or Peripheral
 *
 * @return void.
 ****************************************************************************************
 */
void app_set_mode_cent(void);
void app_set_mode_peri(void);


/**
 ****************************************************************************************
 * @brief Send Reset request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_rst_gap(void);

/**
 ****************************************************************************************
 * @brief Send Inquiry (devices discovery) request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_inq(void);

/**
 ****************************************************************************************
 * @brief Send Start Advertising command to GAPM task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_adv_start(void);


/**
 ****************************************************************************************
 * @brief Send Connect request to GAP task.
 *
 * @param[in] indx  Peer device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect(unsigned char indx);

/**
 ****************************************************************************************
 * @brief Send Read rssi request to GAP task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_read_rssi(void);

/**
 ****************************************************************************************
 * @brief Send disconnect request to GAP task.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disconnect(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send pairing request.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void
 ****************************************************************************************
 */
void app_security_enable(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send connection confirmation.
 *
 * @param[in] auth  Authentication requirements.
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect_confirm(uint8_t auth, uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief generate key.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gen_csrk(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gap_bond_cfm(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Start Encryption with pre-agreed key.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_start_encryption(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send enable request to proximity monitor profile task.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_enable(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send read request for Link Loss Alert Level characteristic to proximity monitor profile task.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_read_llv(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send read request for Tx Power Level characteristic to proximity monitor profile task.
 *
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_read_txp(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send write request to proximity monitor profile task.
 *
 * @param[in] chr Characteristic to be written.
 * @param[in] val Characteristic's Value.
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxm_write(unsigned int chr, unsigned char val, uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send enable request to DIS client profile task.
 *
 * @param[in] con_idx    Index in connected devices list.

 * @return void.
 ****************************************************************************************
 */
void app_disc_enable(uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief Send read request to DIS Client profile task.
 *
 * @param[in] char_code Characteristic to be retrieved.
 * @param[in] con_idx    Index in connected devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disc_rd_char(uint8_t char_code, uint8_t con_idx);

/**
 ****************************************************************************************
 * @brief bd addresses compare.
 *
 * @param[in] bd_address1  Pointer to bd_address 1.
 * @param[in] bd_address2  Pointer to bd_address 2.
 *
 * @return true if addresses are equal / false if not.
 ****************************************************************************************
 */
bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2);

/**
 ****************************************************************************************
 * @brief Check if device is in application's discovered devices list.
 *
 * @param[in] padv_addr  Pointer to devices bd_addr.
 *
 * @return Index in list. if return value equals MAX_SCAN_DEVICES device is not listed.
 ****************************************************************************************
 */
unsigned char app_device_recorded(struct bd_addr *padv_addr);


/**
 ****************************************************************************************
 * @brief Discover service by 128-bit UUID
 *
 * @param[in] conidx    Connection index for the connected peer device.
 *
 * @return void.
 ****************************************************************************************
 */
void app_discover_svc_by_uuid_128(uint16_t conidx, const uint8_t *uuid_128);


/**
 ****************************************************************************************
 * @brief Discover all characteristics in specified handle range
 *
 * @param[in] conidx        Connection index for the connected peer device.
 * @param[in] start_handle  start of handle range to search
 * @param[in] end_handle    end of handle range to search
 *
 * @return void.
 ****************************************************************************************
 */
void app_discover_all_char_uuid_128(uint16_t conidx, uint16_t start_handle,
                                    uint16_t end_handle);


/**
 ****************************************************************************************
 * @brief Discover characteristic descriptors in a specified handle range
 *
 * @param[in] conidx        Connection index for the connected peer device.
 * @param[in] start_handle  start of handle range to search
 * @param[in] end_handle    end of handle range to search
 *
 * @return void.
 ****************************************************************************************
 */
void app_discover_char_desc_uuid_128(uint16_t conidx, uint16_t start_handle,
                                     uint16_t end_handle);


/**
 ****************************************************************************************
 * @brief Write peer device characteristic
 *
 * @param[in] conidx           Connection index for the connected peer device.
 * @param[in] char_val_handle  Characteristic value descriptor handle.
 * @param[in] value            pointer to value byte array
 * @param[in] value_length     length of value byte array
 *
 * @return void.
 ****************************************************************************************
 */
void app_characteristic_write(uint16_t conidx, uint16_t char_val_handle, uint8_t *value,
                              uint8_t value_length);

/**
 ****************************************************************************************
 * @brief Read peer device characteristic
 *
 * @param[in] conidx           Connection index for the connected peer device.
 * @param[in] char_val_handle  Characteristic value descriptor handle.
 *
 * @return void.
 ****************************************************************************************
 */
void app_characteristic_read(uint16_t conidx, uint16_t char_val_handle);


/**
 ****************************************************************************************
 * @brief  Starts GATT MTU size negotiation
 *
 ****************************************************************************************
 */
void app_start_gatt_mtu_negotiation();


/**
 ****************************************************************************************
 * @brief  helper : gets array idx from ble connection idx
 *
 ****************************************************************************************
 */
uint8_t app_get_slot_id_from_conidx(uint8_t conidx);


/**
 ****************************************************************************************
 * @brief Cancel the ongoing air operation.
 *
 * @return void
 ****************************************************************************************
 */
void app_cancel(void);

/**
 ****************************************************************************************
 * @brief Read COM port number from console
 *
 * @return the COM port number.
 ****************************************************************************************
 */
unsigned int ConsoleReadPort(void);

/**
 ****************************************************************************************
 * @brief Find first available slot in device list.
 *
 * @return device list index
 ****************************************************************************************
 */
unsigned int rtrn_fst_avail(void);

/**
 ****************************************************************************************
 * @brief Find previously available connection device and return it.
 *
 * @return valid connection index
 ****************************************************************************************
 **/
unsigned int rtrn_prev_avail_conn(void);

/**
 ****************************************************************************************
 * @brief Find the number of active connections
 *
 * @return the number of active connections
 ****************************************************************************************
 **/
unsigned int rtrn_con_avail(void);

/**
 ****************************************************************************************
 * @brief Cancel last GAPM command
 *
 * @return void
 ****************************************************************************************
 */
void app_cancel_gap(void);

/**
 ****************************************************************************************
 * @brief Resolve connection index (cidx).
 *
 * @param[in] cidx Connection index
 *
 * @return the index in connected devices list.
 ****************************************************************************************
 */
unsigned short res_conidx(unsigned short cidx);

/**
 ****************************************************************************************
 * @brief Returns the index that corresponds to the given connection handle
 *
 * @param[in] conhdl Connection handler
 *
 * @return the index in connected devices list.
 ****************************************************************************************
 */
unsigned short res_conhdl(uint16_t conhdl);

#ifdef __WFSVC_ENABLE__

/**
 ****************************************************************************************
 * @brief set a flag that indicates Wi-Fi GATT Service is enabled
 *
 * @return void
 ****************************************************************************************
 */
void app_wf_prov_mode_enable();

/**
 ****************************************************************************************
 * @brief check if Wi-Fi GATT Service is currently active
 *
 * @return void
 ****************************************************************************************
 */
bool app_wf_prov_mode_is_enabled(void);
#endif


/*
 * External global variables
 */
extern short  wifi_conn_fail_reason;
extern EventGroupHandle_t  evt_grp_wifi_conn_notify;

#endif // APP_H_

/* EOF */
