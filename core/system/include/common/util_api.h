/**
 ****************************************************************************************
 *
 * @file util_api.h
 *
 * @brief Utility APIs for user function
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

#ifndef __UTIL_API_H__
#define    __UTIL_API_H__

#include "sdk_type.h"
#include "app_errno.h"

/* External global functions */
extern int get_run_mode(void);
extern int get_cur_boot_index(void);
extern void app_rf_wifi_cntrl(int onoff);
extern void PRINTF_ATCMD(const char *fmt,...);
extern UINT is_supplicant_done(void);
extern int  gen_default_psk(char *default_psk);

#if defined ( __SUPPORT_WIFI_CONN_CB__ )
extern void wifi_conn_notify_cb_regist(void (*user_cb)(void));
extern void wifi_conn_fail_notify_cb_regist(void (*user_cb)(short reason_code));
extern void wifi_disconn_notify_cb_regist(void (*user_cb)(short reason_code));
extern void ap_sta_disconnected_notify_cb_regist(void (*user_cb)(const unsigned char mac[6]));
#endif    // __SUPPORT_WIFI_CONN_CB__

#if defined ( __RUNTIME_CALCULATION__ )
extern unsigned long long get_fci_dpm_curtime(void);
#endif    // __RUNTIME_CALCULATION__

//// For get SCAN result API ////////////////////////////////////

#define    MAX_SCAN_AP_CNT        30
#define    SCAN_RSP_BUF_SIZE    (4 * 1024 )

#define    AP_OPEN_MODE        0
#define    AP_SECURE_MODE        1

typedef struct scanned_ap_info {
    int    auth_mode;
    int    rssi;
    char   ssid[128];
} scanned_ap_info_t;

typedef struct scan_result_to_app {
    int    ssid_cnt;
    scanned_ap_info_t    scanned_ap_info[MAX_SCAN_AP_CNT];
} scan_result_t;

typedef enum {
    // default leading "0" / "+" / "-0" are not allowed
    POL_1    = 1,
    // leading "+" / "-0" are not allowed
    POL_2    = 2,
} atoi_err_policy;

//////////////////////////////////////////////////////////////////


/* Don't need description */
void regist_wifi_notify_cb(void);
void set_start_time_for_printf(char inital);
void printf_with_run_time(char * string);
void save_with_run_time(char * string);
void let_dpm_daemon_trigger_sleep2(char* context, int seconds);
void combo_make_sleep_rdy(void);
int  wifi_chk_prov_state(int* is_prof_exist, int* is_prof_disabled);
int  sntp_wait_sync(int wait_sec);
int is_in_softap_acs_mode(void);


//////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//// For SFLASH device APIs ///////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

/**
 ****************************************************************************************
 * @brief      Read from SFLASH device
 * @param[in]  sflash_addr    Read address of SFLASH device
 * @param[in]  rd_buf        Buffer pointer to read
 * @param[in]  rd_size        Read Size
 * @return     TRUE on success, FALSE on fail
 ****************************************************************************************
 */
UINT user_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size);

/**
 ****************************************************************************************
 * @brief      Write to SFLASH device
 * @param[in]  sflash_addr   Write address of SFLASH device
 * @param[in]  wr_buf        Write data buffer pointer
 * @param[in]  wr_size       Write Size
 * @return     TRUE on success, FALSE on fail
 ****************************************************************************************
 */
UINT user_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size);

/**
 ****************************************************************************************
 * @brief      Erase data in specific address in SFLASH device
 * @param[in]  sflash_addr    Address of SFLASH device to erase data
 * @return     TRUE on success, FALSE on fail
 ****************************************************************************************
 */
UINT user_sflash_erase(UINT sflash_addr);

#if defined ( __BLE_COMBO_REF__ )
/**
 ****************************************************************************************
 * @brief      BLE Area Read / Write from SFLASH device
 * @param[in]  sflash_addr   Read/Write  address of SFLASH device
 * @param[in]  rd_buf        Buffer pointer to read/write
 * @param[in]  rd_size       Read/Write Size
 * @return     TRUE on success, FALSE on fail
 ****************************************************************************************
 */
UINT ble_sflash_read(UINT sflash_addr, VOID *rd_buf, UINT rd_size);
UINT ble_sflash_write(UINT sflash_addr, UCHAR *wr_buf, UINT wr_size);
#ifdef ENABLE_BLE_SFLASH_ERASE
UINT ble_sflash_erase(UINT sflash_addr);
#endif // ENABLE_BLE_SFLASH_ERASE

#endif // __BLE_COMBO_REF__



///////////////////////////////////////////////////////////////////////////////
//
//// For RTM User Pool APIS ///////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

/**
 ****************************************************************************************
 * @brief      Allocate memory in rtm user pool
 * @param[in]  name         string id for memory to be allocated
 * @param[out] memory_ptr   memory pointer
 * @param[in]  memory_size  size of memory to allocate
 * @param[in]  wait_option  wait option
 * @return     0: Success / others: See err.h (ER_XX...)
 ****************************************************************************************
 */
unsigned int user_rtm_pool_allocate(char *name,
                                   void **memory_ptr,
                                   unsigned long memory_size,
                                   unsigned long wait_option);

/**
 ****************************************************************************************
 * @brief      Release memory from rtm user pool
 * @param[in]  name   string id for memory to release
 * @return     0: Success / others: See err.h (ER_XX...)
 ****************************************************************************************
 */
unsigned int user_rtm_release(char *name);

/**
 ****************************************************************************************
 * @brief      Get memory pointer from rtm user pool
 * @param[in]  name   string id for memory to be allocated
 * @param[out] data   memory pointer
 * @return     size: size of memory / 0: error or not found
 ****************************************************************************************
 */
unsigned int user_rtm_get(char *name, unsigned char **data);

/**
 ****************************************************************************************
 * @brief      Check rtm user pool init state
 * @return     1: initialized / 0: not initialized
 ****************************************************************************************
 */
int user_rtm_pool_init_chk(void);


///////////////////////////////////////////////////////////////////////////////
//
//// For Additional global APIS ///////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

/**
 ****************************************************************************************
 * @brief      Get firmware version from image header (OTA Update version)
 * @param[in]  fw_type   0 BOOT_LOADER, 1 SLIB_IMG, 2 RTOS_IMG
 * @param[in]  get_ver   Buffer pointer to read version string
 * @return     Length of version string
 ****************************************************************************************
 */
UINT get_firmware_version(UINT fw_type, UCHAR *get_ver);

void fw_ver_display(int argc, char *argv[]);


#define OUTPUT_ASCII_ONLY    0
#define OUTPUT_HEXA_ONLY     1
#define OUTPUT_HEXA_ASCII    2

/**
 ****************************************************************************************
 * @brief      Hexa dump the data on console
 * @param[in]  data      Dump address
 * @param[in]  length    Dump length
 * @param[in]  endian    Endian type
 * @return     None
 ****************************************************************************************
 */
void hex_dump(UCHAR *data, UINT length);

void hex_dump_atcmd(UCHAR *data, UINT length);
void hexa_dump_print(u8 *title, const void *buf, size_t len, char direction, char output_fmt);
void http_dump(UCHAR *data, UINT length, UINT endian);


/**
 ****************************************************************************************
 * @brief      RF On/Off with Legacy PS mode (Power Saving mode )
 * @param[in]  on_off    RF On/Off flag ( 1:On, 0:Off )
 * @return     None
 ****************************************************************************************
 */
void fc80211_set_rf_wifi_cntrl(int on_off);

/**
 ****************************************************************************************
 * @brief      Only Legacy PS Control without RF On/Off
 * @param[in]  on_off    RF On/Off flag ( 1:On, 0:Off )
 * @return     None
 ****************************************************************************************
 */
void fc80211_set_psmode_ctrl(int on_off);

/**
 ****************************************************************************************
 * @brief      Only RF On/Off Control
 * @param[in]  flag    On/Off flag ( 0:On, 1:Off )
 * @return     None
 ****************************************************************************************
 */
void wifi_cs_rf_cntrl(int flag);

/**
 ****************************************************************************************
 * @brief      Get current connected Wi-Fi BSSID
 * @param[out] bssid    Buffer to read connected BSSID value
 * @return     NX_SUCCESS on success, Others on fail
 ****************************************************************************************
 */
UINT get_connected_bssid(unsigned char *bssid);


/**
 ****************************************************************************************
 * @brief      Cast ascii to integer
 * @param[in]  str     Buffer to read general digit value
 * @return     output value
 ****************************************************************************************
 */
int atoi_custom (char *str);

/**
 ****************************************************************************************
 * @brief      Cast Date, Time value of string to integer
 * @param[in]  param    Buffer to read date, time value
 * @param[out] int_val  output pointer
 * @return     0: Success / -1: Fail
 ****************************************************************************************
 */
int get_int_val_from_date_time_str(char *param, int *int_val);

/**
 ****************************************************************************************
 * @brief      Cast general digit value to integer
 * @param[in]  param    Buffer to read general digit value
 * @param[out] int_val  output pointer
 * @param[in]  policy   Policy for Special cases
 * @return     0: Success / -1: Fail
 ****************************************************************************************
 */
int get_int_val_from_str(char *param, int *int_val, atoi_err_policy policy);

/**
 ****************************************************************************************
 * @brief      Check validity for MAC address string
 * @param[in]  str      Buffer to read hexa-digit value
 * @return     TRUE: Success / FALSE: Fail
 ****************************************************************************************
 */
int chk_valid_macaddr(char *str);

/**
 ****************************************************************************************
 * @brief      Check query character
 * @param[in]  argc     Argument count
 * @param[in]  str      Input character string
 * @return     0: Success / Others: Fail
 ****************************************************************************************
 */
int is_correct_query_arg(int argc, char *str);

/**
 ****************************************************************************************
 * @brief      Check validity for integer value with min/max range
 * @param[in]  val      Check value
 * @param[in]  min      Range Minimum value
 * @param[in]  max      Range maximum value
 * @return     TRUE: Success / FALSE: Fail
 ****************************************************************************************
 */
int is_in_valid_range(int val, int min, int max);


/**
 ****************************************************************************************
 * @brief      Run wpa cli command operation
 * @param[in]  cmdline   wpa-cli command string
 * @param[in]  delimit   Delimiter character for each parameter
 * @param[in]  cli_reply Reply buffer pointer
 * @return     TRUE on success, FALSE on fail
 ****************************************************************************************
 */
int da16x_cli_reply(char *cmdline, char *delimit, char *cli_reply);



///////////////////////////////////////////////////////////////////////////////
//
//// For Factory-Reset APIs ///////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

/*
 ****************************************************************************************
 * @brief      Clear all profile and set Factory-reset profile as STA mode.
 * @param[in]  None
 * @return     None
 ****************************************************************************************
 */
void factory_reset_sta_mode(void);

/*
 ****************************************************************************************
 * @brief      Clear all profile and set Factory-reset profile as Soft-AP mode.
 * @param[in]  None
 * @return     None
 ****************************************************************************************
 */
void factory_reset_ap_mode(void);

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#if defined ( __SUPPORT_FACTORY_RST_CONCURR_MODE__ )
/*
 ****************************************************************************************
 * @brief      Clear all profile and set Factory-reset profile as Concurrent mode.
 * @param[in]  None
 * @return     None
 ****************************************************************************************
 */
void factory_reset_concurrent_mode(void);
#endif // __SUPPORT_FACTORY_RST_CONCURR_MODE__

/*
 ****************************************************************************************
 * @brief      Save "SWITCH_SYSMODE" to NVRAM which will be used as switching sysmode
 * @param[in]  switch_sysmode  Switching sysmode (0:STA, 1:Soft-AP)
 * @return     TRUE: Success / FALSE: Fail
 ****************************************************************************************
 */
int put_switch_sysmode_4_concurrent_mode(int switch_sysmode);

/*
 ****************************************************************************************
 * @brief      Switch sys-run mode when Factory-Reset button press during 1 second.
 * @param[in]  None
 * @return     TRUE: Success / FALSE: Fail
 ****************************************************************************************
 */
int factory_reset_btn_onetouch(void);

#endif // __SUPPORT_WIFI_CONCURRENT__

#endif // __UTIL_API_H__

/* EOF */
