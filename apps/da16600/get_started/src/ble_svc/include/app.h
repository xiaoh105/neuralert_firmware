/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Header file Main application.
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
#include "../../../../../../core/ble_interface/ble_inc/platform/core_modules/common/api/co_bt.h"
#include "sdk_version.h"

#ifdef __BLE_PERI_WIFI_SVC_PERIPHERAL__
#include "timer0.h"
#include "timer0_2.h"
#include "timer2.h"
#endif

#if defined (__MULTI_BONDING_SUPPORT__)
#include "user_bonding.h"
#endif /* __MULTI_BONDING_SUPPORT__ */

#if defined (__WFSVC_ENABLE__)
#include "wifi_svc_user_custom_profile.h"
#endif /* __WFSVC_ENABLE__ */

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update_ble_fw.h"
#endif /* __BLE_FW_VER_CHK_IN_OTA__ */

#if defined (__IOT_SENSOR_SVC_ENABLE__)
#include "iot_sensor_user_custom_profile.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

#define MAX_SCAN_DEVICES 9

#define MS_TO_BLESLOTS(x)       ((int)((x)/0.625))
#define MS_TO_DOUBLESLOTS(x)    ((int)((x)/1.25))
#define MS_TO_TIMERUNITS(x)     ((int)((x)/10))

/* Advertising and Connection related parameters */
#define USER_CFG_DEFAULT_ADV_INTERVAL_MS                (687.5)

/// Local address type
#define APP_ADDR_TYPE                   0
/// Advertising channel map
#define APP_ADV_CHMAP                   0x07
/// Advertising filter policy
#define APP_ADV_POL                     0
/// Advertising minimum interval
#define APP_ADV_INT_MIN                 MS_TO_BLESLOTS(USER_CFG_DEFAULT_ADV_INTERVAL_MS)
/// Advertising maximum interval
#define APP_ADV_INT_MAX                 MS_TO_BLESLOTS(USER_CFG_DEFAULT_ADV_INTERVAL_MS)
/// Advertising data maximal length
#define APP_ADV_DATA_MAX_SIZE           (ADV_DATA_LEN - 3)
/// Scan Response data maximal length
#define APP_SCAN_RESP_DATA_MAX_SIZE     (SCAN_RSP_DATA_LEN)

#define APP_DFLT_ADV_DATA               "\x07\x03\x03\x18\x02\x18\x04\x18"
#define APP_DFLT_ADV_DATA_LEN           (8)

#define APP_SCNRSP_DATA                 "\x09\xFF\x00\x60\x52\x57\x2D\x42\x4C\x45"
#define APP_SCNRSP_DATA_LENGTH          (10)

/*
 ****************************************************************************************
 * DISS application profile configuration
 ****************************************************************************************
 */

#define APP_DIS_FEATURES                (DIS_MANUFACTURER_NAME_CHAR_SUP | \
                                        DIS_MODEL_NB_STR_CHAR_SUP | \
                                        DIS_SYSTEM_ID_CHAR_SUP | \
                                        DIS_SW_REV_STR_CHAR_SUP | \
                                        DIS_FIRM_REV_STR_CHAR_SUP | \
                                        DIS_PNP_ID_CHAR_SUP)

/// Manufacturer Name (up to 18 chars)
#define APP_DIS_MANUFACTURER_NAME       ("Renesas")
#define APP_DIS_MANUFACTURER_NAME_LEN   (7)

/// Model Number String (up to 18 chars)
#define APP_DIS_MODEL_NB_STR            ("DA14531")

#define APP_DIS_MODEL_NB_STR_LEN        (7)

/// System ID - LSB -> MSB
#define APP_DIS_SYSTEM_ID               ("\x12\x34\x56\xFF\xFE\x9A\xBC\xDE")
#define APP_DIS_SYSTEM_ID_LEN           (8)

/// Serial Number
#define APP_DIS_SERIAL_NB_STR           ("1.0.0.0-LE")
#define APP_DIS_SERIAL_NB_STR_LEN       (10)

/// Hardware Revision String
#define APP_DIS_HARD_REV_STR            ("x.y.z")
#define APP_DIS_HARD_REV_STR_LEN        (5)

/// Firmware Revision
#define APP_DIS_FIRM_REV_STR            SDK_VERSION
#define APP_DIS_FIRM_REV_STR_LEN        (sizeof(APP_DIS_FIRM_REV_STR) - 1)

/// Software Revision String
#define APP_DIS_SW_REV_STR              SDK_VERSION
#define APP_DIS_SW_REV_STR_LEN          (sizeof(APP_DIS_FIRM_REV_STR) - 1)

/// IEEE
#define APP_DIS_IEEE                    ("\xFF\xEE\xDD\xCC\xBB\xAA")
#define APP_DIS_IEEE_LEN                (6)

/**
 * PNP ID Value - LSB -> MSB
 *      Vendor ID Source : 0x02 (USB Implementers Forum assigned Vendor ID value)
 *      Vendor ID : 0x045E      (Microsoft Corp)
 *      Product ID : 0x0040
 *      Product Version : 0x0300
 * e.g. #define APP_DIS_PNP_ID          ("\x02\x5E\x04\x40\x00\x00\x03")
 */
#define APP_DIS_PNP_ID                  ("\x01\xD2\x00\x80\x05\x00\x01")
#define APP_DIS_PNP_ID_LEN              (7)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

#define DEMO_PEER_MAX_MTU (251)
#define ATT_HEADER_SIZE (7)
#define MY_MAX_MTU (251)
#define MY_MAX_MTU_TIME (2120)

#define KE_FIRST_MSG_BLE(task) ((ke_msg_id_t)((task) << 8))

// the exact same enum should exist on DA14xxx SDK
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
    APP_BLE_FORCE_EXCEPTION,

    APP_WIFI_READY,
    APP_WIFI_SLEEP,

    APP_GAS_LEAK_SENSOR_START,
    APP_GAS_LEAK_SENSOR_START_CFM,
    APP_GAS_LEAK_SENSOR_STOP,
    APP_GAS_LEAK_SENSOR_STOP_CFM,
    APP_GAS_LEAK_EVT_IND,
    APP_GAS_LEAK_SENSOR_RD_TIMER_ID,

    APP_PERI_BLINKY_START,
    APP_PERI_BLINKY_START_IND,
    APP_PERI_BLINKY_RUN_TIMER_ID,
    
    APP_PERI_SYSTICK_START,
    APP_PERI_SYSTICK_START_IND,
    APP_PERI_SYSTICK_RUN_TIMER_ID,
    APP_PERI_SYSTICK_STOP,
    APP_PERI_SYSTICK_STOP_IND,
    
    APP_PERI_TIMER0_GEN_START,
    APP_PERI_TIMER0_GEN_START_IND,
    APP_PERI_TIMER0_GEN_RUN_TIMER_ID,
    APP_PERI_TIMER0_GEN_STOP_TIMER_ID,

    APP_PERI_TIMER0_BUZ_START,
    APP_PERI_TIMER0_BUZ_START_IND,
    APP_PERI_TIMER0_BUZ_RUN_TIMER_ID,
    APP_PERI_TIMER0_BUZ_STOP_TIMER_ID,

    APP_PERI_TIMER2_PWM_START,
    APP_PERI_TIMER2_PWM_START_IND,
    APP_PERI_TIMER2_PWM_RUN_TIMER_ID,

    APP_PERI_GET_BATT_LVL,
    APP_PERI_BATT_LVL_IND,

    APP_PERI_I2C_EEPROM_START,
    APP_PERI_I2C_EEPROM_START_IND,
    APP_PERI_I2C_EEPROM_RUN_TIMER_ID,

    APP_PERI_SPI_FLASH_START,
    APP_PERI_SPI_FLASH_START_IND,
    APP_PERI_SPI_FLASH_RUN_TIMER_ID,

    APP_PERI_QUAD_DEC_START,
    APP_PERI_QUAD_DEC_START_IND,
    APP_PERI_QUAD_DEC_STOP,
    APP_PERI_QUAD_DEC_STOP_IND,
    APP_PERI_QUAD_DEC_RUN_TIMER_ID,

    APP_PERI_GPIO_SET_CMD,
    APP_PERI_GPIO_SET_IND,

    APP_PERI_GPIO_GET_CMD,
    APP_PERI_GPIO_GET_IND,

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

#if defined (__LOW_POWER_IOT_SENSOR__)
typedef struct _iot_sensor_data
{
    int    is_provisioned;
    int    is_sensor_started;
    int is_gas_leak_happened;
    char svr_ip_addr[16];
    int svr_port;
} iot_sensor_data_t;

#define SENSOR_DATA_NAME "SENSOR_DATA"
#endif /* __LOW_POWER_IOT_SENSOR__ */

/*
 ****************************************************************************************
 *
 * Security related configuration
 *
 ****************************************************************************************
 */
struct security_configuration
{
    /*************************************************************************************
     * IO capabilities (@see gap_io_cap)
     *
     * - GAP_IO_CAP_DISPLAY_ONLY          Display Only
     * - GAP_IO_CAP_DISPLAY_YES_NO        Display Yes No
     * - GAP_IO_CAP_KB_ONLY               Keyboard Only
     * - GAP_IO_CAP_NO_INPUT_NO_OUTPUT    No Input No Output
     * - GAP_IO_CAP_KB_DISPLAY            Keyboard Display
     *
     *************************************************************************************
     */
    enum gap_io_cap iocap;

    /*************************************************************************************
     * OOB information (@see gap_oob)
     *
     * - GAP_OOB_AUTH_DATA_NOT_PRESENT    OOB Data not present
     * - GAP_OOB_AUTH_DATA_PRESENT        OOB data present
     *
     *************************************************************************************
     */
    enum gap_oob oob;

    /*************************************************************************************
     * Authentication (@see gap_auth / gap_auth_mask)
     *
     * - GAP_AUTH_NONE      None
     * - GAP_AUTH_BOND      Bond
     * - GAP_AUTH_MITM      MITM
     * - GAP_AUTH_SEC       Secure Connection
     * - GAP_AUTH_KEY       Keypress Notification
     *
     *************************************************************************************
     */
    uint8_t auth;

    /*************************************************************************************
     * Encryption key size (7 to 16) - LTK Key Size

     *************************************************************************************
     */
    uint8_t key_size;

    /*************************************************************************************
     * Initiator key distribution (@see gap_kdist)
     *
     * - GAP_KDIST_NONE                   No Keys to distribute
     * - GAP_KDIST_ENCKEY                 LTK (Encryption key) in distribution
     * - GAP_KDIST_IDKEY                  IRK (ID key)in distribution
     * - GAP_KDIST_SIGNKEY                CSRK (Signature key) in distribution
     * - Any combination of the above
     *
     *************************************************************************************
     */
    uint8_t ikey_dist;

    /*************************************************************************************
     * Responder key distribution (@see gap_kdist)
     *
     * - GAP_KDIST_NONE                   No Keys to distribute
     * - GAP_KDIST_ENCKEY                 LTK (Encryption key) in distribution
     * - GAP_KDIST_IDKEY                  IRK (ID key)in distribution
     * - GAP_KDIST_SIGNKEY                CSRK (Signature key) in distribution
     * - Any combination of the above
     *
     *************************************************************************************
     */
    uint8_t rkey_dist;

    /*************************************************************************************
    * Device security requirements (minimum security level). (@see gap_sec_req)
    *
    * - GAP_NO_SEC                 No security (no authentication and encryption)
    * - GAP_SEC1_NOAUTH_PAIR_ENC   Unauthenticated pairing with encryption
    * - GAP_SEC1_AUTH_PAIR_ENC     Authenticated pairing with encryption
    * - GAP_SEC1_SEC_PAIR_ENC      Authenticated LE Secure Connections pairing with encryption
    * - GAP_SEC2_NOAUTH_DATA_SGN   Unauthenticated pairing with data signing
    * - GAP_SEC2_AUTH_DATA_SGN     Authentication pairing with data signing
    *
    **************************************************************************************
    */
    enum gap_sec_req sec_req;
};


typedef struct
{
    unsigned char  free;
    struct bd_addr adv_addr;
    unsigned short conidx;
    unsigned short conhdl;
    unsigned char idx;
    unsigned char  rssi;
    unsigned char  data_len;
    unsigned char  data[ADV_DATA_LEN + 1];

} ble_dev;

//Proximity Reporter connected device
typedef struct
{
    ble_dev device; // BLE peer device
    unsigned char bonded;

    struct gapc_ltk ltk;
    struct gapc_irk irk;
    struct gap_sec_key csrk;

    uint8_t peer_addr_type; // paired peer addr type
    uint8_t auth_lvl; // authentication level

    unsigned char llv;
    CHAR txp;
    unsigned char rssi;
    unsigned char alert;

} proxr_dev;

/// application environment structure
struct app_env_tag
{
    unsigned char state;
#if defined (__LOW_POWER_IOT_SENSOR__)
    unsigned char iot_sensor_state; // for iot_sensor demo purpose
#endif
    unsigned char num_of_devices;
    ble_dev devices[MAX_SCAN_DEVICES];
    proxr_dev proxr_device;
};

struct app_sec_env_tag
{
    // LTK
    struct gap_sec_key ltk;
    // Random Number
    struct rand_nb rand_nb;
    // EDIV
    uint16_t ediv;
    // LTK key size
    uint8_t key_size;
    
    // Last paired peer address type 
    uint8_t peer_addr_type;
    // Last paired peer address 
    struct bd_addr peer_addr;
    
    // authentication level
    uint8_t auth;

    // Current Peer Device NVDS Tag
    uint8_t nvds_tag;
    
    struct gap_sec_key irk;
    
    uint16_t client_config_cached;
};

struct app_env_tag_adv
{
    /// Connection handle
    uint16_t conhdl;

    ///  Advertising data
    uint8_t app_adv_data[32];
    ///  Advertising data length
    uint8_t app_adv_data_length;
    /// Scan response data
    uint8_t app_scanrsp_data[32];
    /// Scan response data length
    uint8_t app_scanrsp_data_length;
};
extern struct app_env_tag_adv app_env_adv;

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

typedef struct
{
    uint32_t blink_timeout;
    uint8_t blink_toggle;
    uint8_t lvl;
    uint8_t ll_alert_lvl;
    int8_t  txp_lvl;
    uint8_t adv_toggle;

} app_alert_state;

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
typedef struct _wf_svc_app_data_
{
    struct app_env_tag app_env;
    struct app_device_info device_info;
    app_alert_state alert_state;
    uint16_t    my_custom_service_wfsvc_start_handle;    
    user_custom_profile_wfsvc_env_t user_custom_profile_wfsvc_env;
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
    ble_img_hdr_all_t ble_fw_hdrs_all;
#endif /* __BLE_FW_VER_CHK_IN_OTA__ */
#if defined (__MULTI_BONDING_SUPPORT__)
    struct app_sec_env_tag bond_info[MAX_BOND_PEER];
    uint8_t bond_info_current_index;
    uint8_t bonding_in_progress;
#endif /* __MULTI_BONDING_SUPPORT__ */
#if defined (__LOW_POWER_IOT_SENSOR__)
    iot_sensor_data_t iot_sensor_data_info;
#endif /* __LOW_POWER_IOT_SENSOR__ */
#if defined (__IOT_SENSOR_SVC_ENABLE__)
    struct iot_sensor_env_tag iot_sensor_env;
#endif
    int is_ble_in_advertising;
} wf_svc_app_data_t;
#endif /* __ENABLE_DPM_FOR_GTL_BLE_APP__ */


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
#ifdef __BLE_PERI_WIFI_SVC_PERIPHERAL__
// blinky
typedef struct app_peri_blinky_start
{
    uint8_t gpio_port;
    uint8_t gpio_pin;
    uint8_t blink_cnt;
} app_peri_blinky_start_t;

// systick
typedef struct app_peri_systick_start
{
    uint8_t gpio_port;
    uint8_t gpio_pin;
    uint32_t period_us;
} app_peri_systick_start_t;

// timer0_gen
typedef struct app_peri_timer0_gen_start
{
    uint8_t gpio_port;
    uint8_t gpio_pin;
    uint16_t test_dur;
    tim0_2_clk_div_t in_clk_div_factor;
    TIM0_CLK_SEL_t timer0_clk_src;
    PWM_MODE_t pwm_mode;
    TIM0_CLK_DIV_t timer0_clk_div;
    int reload_val;
} app_peri_timer0_gen_start_t;


// timer0_buz
typedef struct app_peri_timer0_buz_start
{
    uint8_t t0_pwm0_port;
    uint8_t t0_pwm0_pin;
    uint8_t t0_pwm1_port;
    uint8_t t0_pwm1_pin;
    
    uint16_t buz_tst_counter;
    
    tim0_2_clk_div_t t0_t2_in_clk_div_factor;
    TIM0_CLK_SEL_t   t0_clk_src;
    PWM_MODE_t       t0_pwm_mod;
    TIM0_CLK_DIV_t   t0_clk_div;
    
    int t0_on_reld_val;
    int t0_high_reld_val;
    int t0_low_reld_val;
} app_peri_timer0_buz_start_t;

typedef struct
{
    uint8_t gpio_pin;
    uint8_t is_used;
} t2_pwm_pin_t;

// timer2_pwm
#define NUM_T2_PWM_PIN 3
typedef struct app_peri_timer2_pwm_start
{
    uint8_t          t2_pwm_port;
    t2_pwm_pin_t      t2_pwm_pin[NUM_T2_PWM_PIN];    
    tim0_2_clk_div_t t0_t2_in_clk_div_factor;
    tim2_hw_pause_t  t2_hw_pause;
    uint32_t          t2_pwm_freq;
    
} app_peri_timer2_pwm_start_t;

// batt_lvl : not needed

// i2c_eeprom : not needed

// spi_flash : not needed

// quad_dec
typedef struct app_peri_quad_dec_start
{
    uint8_t evt_cnt_bef_int;
    uint8_t clk_div;
} app_peri_quad_dec_start_t;

#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
/// GPIO mode
typedef enum {
    /// Input
    INPUT = 0,

    /// Input pullup
    INPUT_PULLUP = 0x100,

    /// Input pulldown
    INPUT_PULLDOWN = 0x200,

    /// Output
    OUTPUT = 0x300,
} GPIO_PUPD;

// gpio set/get
typedef struct app_peri_gpio_cmd
{
    uint8_t gpio_port;
    uint8_t gpio_pin;
    uint8_t active;
    GPIO_PUPD mode;
} app_peri_gpio_cmd_t;
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
#endif

/*
 * EXTERNAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;
extern app_alert_state alert_state;

/// Device info
extern struct app_device_info device_info;

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
 * @brief Set Bondable mode.
 *
 * @return void.
 ****************************************************************************************
 */
void app_set_mode(void);
/**
 ****************************************************************************************
 * @brief Send Reset request to GAPM task.
 *
 * @return void.
 ****************************************************************************************
 */
void app_rst_gap(void);

/**
 ****************************************************************************************
 * @brief Send Inquiry (devices discovery) request to GAPM task.
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
 * @brief Cancel the ongoing air operation.
 *
 * @return void
 ****************************************************************************************
 */
void app_cancel(void);

/**
 ****************************************************************************************
 * @brief Send Connect request to GAPM task.
 *
 * @param[in] indx  Peer device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect(unsigned char indx);

/**
 ****************************************************************************************
 * @brief Send the GAPC_DISCONNECT_IND message to a task.
 *
 * @param[in] dst     Task id of the destination task.
 * @param[in] conhdl  The conhdl parameter of the GAPC_DISCONNECT_IND message.
 * @param[in] reason  The reason parameter of the GAPC_DISCONNECT_IND message.
 *
 * @return void.
 ****************************************************************************************
 */
void app_send_disconnect(uint16_t dst, uint16_t conhdl, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Send Read rssi request to GAPC task instance for the current connection.
 *
 * @return void.
 ****************************************************************************************
 */
void app_read_rssi(void);
/**
 ****************************************************************************************
 * @brief Send disconnect request to GAPC task instance for the current connection.
 *
 * @return void.
 ****************************************************************************************
 */
void app_disconnect(void);
/**
 ****************************************************************************************
 * @brief Send pairing request.
 *
 * @return void.
 ****************************************************************************************
 */
void app_security_enable(void);
/**
 ****************************************************************************************
 * @brief Send connection confirmation.
 *
 * @param[in] auth  Authentication requirements.
 * @param[in] svc_changed_ind_enable    Service changed indication enable/disable.
 *
 * @return void.
 ****************************************************************************************
 */
void app_connect_confirm(enum gap_auth auth, uint8_t svc_changed_ind_enable);
/**
 ****************************************************************************************
 * @brief generate csrk key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_gen_csrk(void);
/**
 ****************************************************************************************
 * @brief generate ltk key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_sec_gen_ltk(uint8_t key_size);

/**
 ****************************************************************************************
 * @brief Generate Temporary Key.
 *
 * @return temporary key value
 ****************************************************************************************
 */
uint32_t app_gen_tk(void);
/**
 ****************************************************************************************
 * @brief Confirm bonding.
 *
 *  param[in] param  The parameters of GAPC_BOND_REQ_IND to be confirmed.
 *.
 * @return void.
 ****************************************************************************************
 */
void app_gap_bond_cfm(struct gapc_bond_req_ind *param);

void app_from_ext_host_security_start(void);

/**
 ****************************************************************************************
 * @brief Start Encryption with pre-agreed key.
 *
 * @return void.
 ****************************************************************************************
 */
void app_start_encryption(void);
/**
 ****************************************************************************************
 * @brief bd address compare.
 *
 *  @param[in] bd_address1  Pointer to bd_address 1.
 *  @param[in] bd_address2  Pointer to bd_address 2.
 *
 * @return true if addresses are equal / false if not.
 ****************************************************************************************
 */
bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2);
/**
 ****************************************************************************************
 * @brief Check if device is in application's discovered device list.
 *
 * @param[in] padv_addr  Pointer to devices bd_addr.
 *
 * @return Index in list. if return value equals MAX_SCAN_DEVICES device is not listed.
 ****************************************************************************************
 */
unsigned char app_device_recorded(struct bd_addr *padv_addr);

/**
 ****************************************************************************************
 * @brief Add Proximity Reporter Service instance in the DB.
 *
 * @return void.
 ****************************************************************************************
 */
void app_proxr_create_db(void);
/**
 ****************************************************************************************
 * @brief Add a Device Information Service instance in the DB.
 *
 * @return void.
 ****************************************************************************************
 */
void app_diss_create_db(void);
/**
 ****************************************************************************************
 * @brief Handles commands sent from console interface.
 *
 *
 * @return void.
 ****************************************************************************************
 */
void ConsoleEvent(void);
/**
 ****************************************************************************************
 * @brief Initialization of application environment
 *
 ****************************************************************************************
 */
void app_env_init(void);

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
void app_user_data_init_pre(void);
void app_user_data_dpm_mem_alloc(void);
#endif /* __ENABLE_DPM_FOR_GTL_BLE_APP__ */

#if defined (__LOW_POWER_IOT_SENSOR__)
void app_sensor_start(void);
void app_sensor_stop(void);
void app_wifi_ready_ind(void);
void app_todo_before_sleep(void);
#endif /* __LOW_POWER_IOT_SENSOR__ */

void app_ble_sw_reset(void);
void app_update_conn_params(void);
int app_is_ble_in_connected(void);


// others
void app_get_local_dev_info(enum gapm_operation ops);

#ifdef __BLE_PERI_WIFI_SVC_PERIPHERAL__
// blinky
void app_peri_blinky_start_send(uint8_t gpio_port, 
                                uint8_t gpio_pin, 
                                uint8_t blink_count);

// systick
void app_peri_systick_start_send(uint8_t gpio_port, 
                                uint8_t gpio_pin, 
                                uint32_t period_us);


void app_peri_systick_stop_send(void);

// timer0_gen
void app_peri_timer0_gen_start_send(uint8_t gpio_port,
                                    uint8_t gpio_pin,
                                    uint8_t test_dur,
                                    tim0_2_clk_div_t in_clk_div_factor,
                                    TIM0_CLK_SEL_t timer0_clk_src,
                                    PWM_MODE_t pwm_mode,
                                    TIM0_CLK_DIV_t timer0_clk_div,
                                    int reload_val);

// timer0_buzzer
void app_peri_timer0_buz_start_send(app_peri_timer0_buz_start_t* config_param);


// timer2_pwm
void app_peri_timer2_pwm_start_send(app_peri_timer2_pwm_start_t* config_param);

// batt_lvl
void app_peri_get_batt_lvl(void);

// i2c_eeprom
void app_peri_i2c_eeprom_start_send(void);

// spi_flash
void app_peri_spi_flash_start_send(void);

// quad_dec
void app_peri_quad_dec_start_send(app_peri_quad_dec_start_t* config_param);

void app_peri_quad_dec_stop_send();
#endif
/// @} APP

//
// ===================================
//

/*
 * External global functions
 */
extern void enable_wakeup_by_ble(void);

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

#if defined (__WFSVC_ENABLE__)
extern void set_wifi_sta_scan_result_str_idx(uint32_t idx);
#endif // __WFSVC_ENABLE__
#if defined ( __ENABLE_RTC_WAKE_UP2_INT__ )
extern void enable_wakeup_intr(UINT8 en);
#endif // __ENABLE_RTC_WAKE_UP2_INT__

#if defined (__GTL_IFC_UART__)
extern void boot_ble_from_uart(void);
#endif /* __GTL_IFC_UART__ */


extern dpm_boot_type_t dpm_boot_type;
extern EventGroupHandle_t  evt_grp_wifi_conn_notify;

extern short wifi_conn_fail_reason;
extern uint16_t my_custom_service_wfsvc_start_handle;
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
extern ble_img_hdr_all_t ble_fw_hdrs_all;
#endif /* __BLE_FW_VER_CHK_IN_OTA__ */

#endif // APP_H_
