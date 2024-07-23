/**
 ****************************************************************************************
 *
 * @file command_net_sys.c
 *
 * @brief Network system application
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

#include "stdbool.h"
#include "clib.h"

#include "supp_def.h"        // For feature __LIGHT_SUPPLICANT__

#include "da16x_types.h"
#include "sys_feature.h"
#include "command.h"
#include "command_net.h"
#include "common_def.h"
#include "iface_defs.h"
#include "nvedit.h"
#include "environ.h"
#include "da16x_dpm.h"
#include "user_dpm.h"
#include "da16x_network_common.h"
#include "da16x_ip_handler.h"
#include "da16x_dhcp_server.h"
#include "da16x_dhcp_client.h"
#include "da16x_sntp_client.h"
#include "da16x_time.h"

#include "da16x_image.h"
#ifdef    XIP_CACHE_BOOT
#include "supp_def.h"
#include "mqtt_client.h"
#endif // XIP_CACHE_BOOT
#include "common_config.h"
#include "da16x_network_common.h"
#include "project_config.h"

#ifdef    XIP_CACHE_BOOT
#include "lwipopts.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"

#if LWIP_DHCP
#include "dhcpserver.h"
#include "dhcpserver_options.h"
#endif    /*LWIP_DHCP*/
#if LWIP_ARP
#include "lwip/etharp.h"
#endif /*LWIP_ARP*/
#include "da16x_arp.h"
#endif    /*XIP_CACHE_BOOT*/

#if defined (XIP_CACHE_BOOT)
#include "os.h"
#endif

#if defined (__SUPPORT_ATCMD_TLS__)
#include "atcmd_cert_mng.h"
#endif // __SUPPORT_ATCMD_TLS__
#include "da16x_cert.h"

#ifdef __SUPPORT_ASD__
#include "asd.h"
#endif    // __SUPPORT_ASD__



#define DEFULT_MAC_MSW                    0xEC9F
#define DEFULT_MAC_LSW                    0x0D9FFFFE
#define CHECK_STEP_FACTORY_BTN            10          /* 100ms */
#define CHECK_STEP_WPS_BTN                10          /* 100ms */
#define MIN_TIME_FOR_CHECK_FACTORY_RST    5           /* 500 msec */
#define CLI_SCAN_RSP_BUF_SIZE             3584
#define NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    (3600*36)    /*131072*/

#define NET_SYS_DEBUG                   1

#if DHCPS_DEBUG
  #define NET_SYS_LOG PRINTF
#else
  #define NET_SYS_LOG
#endif
#define NUM_CHANNEL_PWR                 14

//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------

void    cmd_arp_table(int argc, char *argv[]);
void    cmd_getWLANMac(int argc, char *argv[]);
void    cmd_setWLANMac(int argc, char *argv[]);
void    cmd_da16x_cli(int argc, char *argv[]);
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
void    cmd_setSysMode(int argc, char *argv[]);
#endif // __SUPPORT_WIFI_CONCURRENT__
void    cmd_setup(int argc, char *argv[]);
void    cmd_network_dhcpd(int argc, char *argv[]);
void    cmd_dpm(int argc, char *argv[]);
int     cert_flash_delete_all(void);
void    set_wlaninit(UINT flag);
UINT    get_wlaninit(void);
int     getMACAddrStr(UINT separate, char *macstr);
UINT    writeMACaddress(char *macaddr, int dst);
#if defined ( __SUPPORT_EVK_LED__ )
UINT    check_factory_button(int sw_gpio_num, int led_gpio_num, int reset_time);
UINT    check_wps_button(int btn_gpio_num, int led_gpio_num, int check_time);
#else
UINT    check_factory_button(int sw_gpio_num, int reset_time);
UINT    check_wps_button(int btn_gpio_num, int check_time);
#endif    // __SUPPORT_EVK_LED__
int     getStr(char *get_data, int get_len);

//-----------------------------------------------------------------------
// External Functions
//-----------------------------------------------------------------------
extern int    SYS_OTP_MAC_ADDR_WRITE(char *mac_addr);
extern int    SYS_OTP_MAC_ADDR_READ(char *mac_addr);

extern int    ip_change(UINT iface, char *ipaddress, char *netmask, char *gateway, UCHAR save);
extern void   wifi_netif_control(int, int);
extern UINT   is_supplicant_done();
extern int    fc80211_get_sta_rssi_value(int ifindex);

extern long   iptolong(char *ip);
extern UINT   set_netmode(UCHAR iface, UCHAR mode, UCHAR save);
extern int    get_netmode(int iface);

extern void   usage_dhcpd(void);
extern int    da16x_cli_reply(char *cmdline, char *delimit, char *cli_reply);
extern int    getSysMode(void);
#if !defined ( __TINY_MEM_MODEL__ )
extern int    setSysMode(int mode);
#endif    //!( __TINY_MEM_MODEL__ )
extern void   da16x_net_check(int interface_flag, int simple);
extern void   set_dpm_keepalive_config(int duration);
extern int    set_dpm_keepalive_to_nvram(int time);
extern int    set_dpm_user_wu_time_to_nvram(int msec);
extern UINT   set_debug_ch_tx_level(char *ch_pwr_l);
extern void   print_tx_level(void);
extern uint16_t  get_product_info(void);

extern void   clr_dpm_conn_info(void);
extern void   dpm_user_rtm_print(void);
extern void   dpm_udpf_cntrl(unsigned char en_flag);
extern void   dpm_tcpf_cntrl(unsigned char en_flag);
extern void   set_dpm_tcp_port_filter(unsigned short d_port);
extern UINT   get_dpm_dbg_level(void);
extern void   set_dpm_dbg_level(UINT level);
extern int    get_run_mode(void);
extern int    fc80211_set_tx_power_table(int ifindex, unsigned int *one_regcode_start_addr, unsigned int *one_regcode_start_addr_dsss);
extern int    get_ent_cert_verify_flags(void);
extern void   set_ent_cert_verify_flags(char flag, int value);
#if defined (__BLE_COMBO_REF__)
extern void combo_ble_sw_reset(void);
#endif    // __BLE_COMBO_REF__


//-----------------------------------------------------------------------
// External Variables
//-----------------------------------------------------------------------
#if defined (__BLE_COMBO_REF__)
extern unsigned char ble_combo_ref_flag;
#endif    //(__BLE_COMBO_REF__)

extern struct netif   *dhcps_netif;
extern UCHAR    region_power_level[NUM_CHANNEL_PWR];

unsigned char dpm_cmd_autoarp_en = 1;

enum
{
    E_WRITE_OK,
    E_WRITE_ERROR,
    E_ERASE_OK,
    E_ERASE_ERROR,
    E_DIGIT_ERROR,
    E_SAME_MACADDR,
    E_CANCELED,
    E_INVALID_ERROR,
    E_UNKNOW
} ERROR_WRITE_MAC;

enum
{
    MAC_SPOOFING,
    NVRAM_MAC,
    OTP_MAC
} MACADDR_TYPE;

/* One-touch press operation for Factory-Reset */
int (*button1_one_touch_cb)(void) = NULL;

static int noinitwlan = 0; // 1: WLAN init X, 2: WLAN init O
UINT get_wlaninit(void)
{
    if (noinitwlan == 1) {
        return pdFALSE;
    } else if (noinitwlan == 2) {
        return pdTRUE;
    }

    read_nvram_int("NOINITWLAN", &noinitwlan);

    if (noinitwlan == 1) {    /* WLAN init X */
        // In case of RF-Test mode, cannot support DPM operation
        write_nvram_int("dpm_mode", 0);

        return pdFALSE;
    } else {
        noinitwlan = 2; /* WLAN init O */
        return pdTRUE;
    }
}

void set_wlaninit(UINT flag)
{
    switch (flag) {
    case 0:
        /* In case of RF-Test mode, cannot support DPM operation */
        delete_nvram_env("dpm_mode");

        /* Don't run WLAN init when boot */
        write_nvram_int("NOINITWLAN", 1);
 
        noinitwlan = 1;
        break;

    case 1:
        /* Run WLAN init when boot */
        delete_nvram_env("NOINITWLAN");
        noinitwlan = 2;
        break;
    }
}

void printSysMode(UINT mode)
{
    char temp_str[30];

    memset(temp_str, 0, sizeof(temp_str));

    switch (mode) {
    case SYSMODE_STA_ONLY    :
        strcpy(temp_str, "Station Only");
        break;

    case SYSMODE_AP_ONLY    :
        strcpy(temp_str, "Soft-AP");
        break;
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#ifdef __SUPPORT_P2P__
    case SYSMODE_P2P_ONLY    :
        strcpy(temp_str, "WiFi Direct");
        break;

    case SYSMODE_P2PGO    :
        strcpy(temp_str, "WiFi Direct P2P GO fixed");
        break;
#endif /* __SUPPORT_P2P__ */

    case SYSMODE_STA_N_AP    :
        strcpy(temp_str, "Station & Soft-AP");
        break;

#ifdef __SUPPORT_P2P__
    case SYSMODE_STA_N_P2P    :
        strcpy(temp_str, "Station & WiFi Direct");
        break;
#endif /* __SUPPORT_P2P__ */

    default:
        strcpy(temp_str, "Unknown Mode ...");
#endif // __SUPPORT_WIFI_CONCURRENT__
    }

    PRINTF("\nSystem Mode : %s (%d)\r\n", temp_str, mode);
}

UINT isVaildDomain(char *domain)
{
    if (strchr(domain, '.') != NULL) {
        return TRUE;
    }

    return FALSE;
}

int chk_ssid_chg_state(void)
{
    user_dpm_supp_conn_info_t    *user_dpm_supp_conn_info;
    char    *nvram_ssid;
    char    *nvram_psk;
    int    rtn;

    user_dpm_supp_conn_info = (user_dpm_supp_conn_info_t *)get_supp_conn_info_ptr();

    if (user_dpm_supp_conn_info == NULL) {
        return 0;
    }

    /* Compare the status of ssid changing */
    nvram_ssid = read_nvram_string(NVR_KEY_SSID_0);

    if (nvram_ssid == NULL && strlen((char const *)(user_dpm_supp_conn_info->ssid)) > 0) {
        rtn = -1;
    } else if (nvram_ssid != NULL && strcmp(nvram_ssid, (char const *)(user_dpm_supp_conn_info->ssid)) == 0) {
        rtn = 0;
    } else {
        rtn = -1;
    }

    /* Compare the status of psk changing */
    nvram_psk = read_nvram_string(NVR_KEY_ENCKEY_0);

    if (nvram_psk == NULL && strlen((char const *)(user_dpm_supp_conn_info->psk)) > 0) {
        rtn = -1;
    } else if (nvram_psk != NULL
             && strcmp(nvram_psk, (char const *)(user_dpm_supp_conn_info->psk)) == 0) {
        rtn = 0;
    } else {
        rtn = -1;
    }

    return rtn;
}

#undef __DPM_TIM_TEST_CMD__ /* Command for Testing */

static int dpm_cmd_running = 0;

/* DPM Hold Flag by Command */
extern bool     dpm_daemon_cmd_hold_flag;
void cmd_dpm(int argc, char *argv[])
{
    extern void show_dpm_sleep_info(void);
    extern void dpm_show_dpm_monitor_info(void);
    extern void dpm_show_dpm_monitor_count(void);
    extern void dpm_monitor_stop(void);
    extern void dpm_monitor_restart(void);
    extern int set_dpm_abnormal_wait_time(int time, int mode);

    extern void dpm_abnormal_chk_hold();
    extern void dpm_abnormal_chk_resume();

    // Check running mode : STA_ONLY
    if (0 != getSysMode()) {
        PRINTF("\nNot support DPM !!!\n\n");
        return;
    }

    // Check RF-Test mode : Cannot support DPM operation
    if (get_wlaninit() == pdFALSE) {
        PRINTF("\nRF Test mode - Not support DPM !!!\n\n");
        return;
    }

    if (argc == 1) {
set_usage :
        PRINTF("- DPM module < %sabled >\n\n", chk_dpm_mode() ? "En" : "Dis");
        PRINTF("  Usage : dpm [option]\n");
        PRINTF("     option\n");
        PRINTF("\t[on|off]\t\t: DPM enable or disable\n");
        PRINTF("\t[status]\t\t: Display DPM status\n");
        PRINTF("\t[rtm]\t\t\t: Display DPM backup data\n");
        PRINTF("\t[rtmmap]\t\t\t: Display retention memory map\n");
        PRINTF("\t[rtc]\t\t\t: Display DPM RTC timer\n");
        PRINTF("\t[user_pool]\t\t: Display User RTM pool usage\n");
        PRINTF("\t[debug] [level]\t\t: Turn DPM debug on and off\n");
        PRINTF("\t[apsync] [on|off]\t: Turn DPM AP Sync on and off\n");
        PRINTF("\t\t  level = 1(MSG_ERROR), 2(MSG_INFO), 3(MSG_DEBUG), 4(MSG_EXCESSIVE)\n");
        PRINTF("\t[udph] [ip][src][dst]\n");

        PRINTF("\n");
        PRINTF("\t[abn] [hold]\t\t: Hold abnormal check\n");
        PRINTF("\t[abn] [resume]\t\t: Resume abnormal check\n");
        PRINTF("\t[abn] [wifi_conn_wait]\t: wi-fi connection wait\n");
        PRINTF("\t[abn] [wifi_conn_retry_cnt]\t: wi-fi connection retry count at abnormal dpm\n");
#if 0    // Add this commands if needed ...
        PRINTF("\t[abn] [dhcp_rsp_wait]\t: DHCP response wait time\n");
        PRINTF("\t[abn] [arp_rsp_wait]\t: ARP response wait time\n");
        PRINTF("\t[abn] [dpm_fail_wait]\t: Unknown DPM fail wait time\n");
#endif    // 0

#ifdef __CHK_DPM_ABNORM_STS__
        PRINTF("\n");
        PRINTF("\t[mon] [count]\t\t: Show counts of DPM wakeup source and abnormal errors\n");
        PRINTF("\t[mon] [stop]\t\t: Stop DPM abnormal counting\n");
        PRINTF("\t[mon] [restart]\t\t: Restart DPM abnormal counting\n");
        PRINTF("\t[mon] [count][option]\t: Show DPM abnormal counting value\n");
        PRINTF("\t                          option(seconds)\n");
        PRINTF("\t[mon] [info]\t\t: Show DPM abnormal counting status and threshold value\n");
        PRINTF("\n");
#endif    // __CHK_DPM_ABNORM_STS__

#ifdef __DPM_TIM_TEST_CMD__
        PRINTF("\n");
        PRINTF("\t[autoarp] [on|off]\t\t: Turn AutoARP on and off\n");
        PRINTF("\t[deauthchk] [on|off]\t\t: Turn DeauthChk on and off\n");
#endif    // __DPM_TIM_TEST_CMD__

        dpm_cmd_running = 0;
        return;
    }

    if (dpm_cmd_running == 1) {
        return;
    }

    dpm_cmd_running = 1;

    if (strcasecmp(argv[1], "ON") == 0) {
        if (!chk_rtm_exist()) {
            PRINTF("FAIL\n - RTM doesn't exist.\n");
            return;
        }

        //OAL_DPM_FULL_BOOT. DPM Mode on. Chang
        vTaskDelay(5);        // for tunning RamLib...

        enable_dpm_mode();
        vTaskDelay(10);    // for tunning RamLib...

        /* If connection config was changed,
         * need to reboot to start as normal mode
         */
        if (!chk_dpm_wakeup() || chk_ssid_chg_state() == -1) {
            mqtt_client_termination();
#if defined (__BLE_COMBO_REF__)
            if (ble_combo_ref_flag == pdTRUE) {
                combo_ble_sw_reset();
            }
#endif    // __BLE_COMBO_REF__
            reboot_func(SYS_REBOOT);
        }
    } else if (strcasecmp(argv[1], "OFF") == 0) {

        hold_dpm_sleepd();

        //OAL_DPM_NORMAL_BOOT. DPM Mode off. Chang
        vTaskDelay(5);    // for tunning RamLib...

        /* Disable DPM running mode */
        disable_dpm_mode();
        vTaskDelay(10);    // for tunning RamLib...

        mqtt_client_termination();
#if defined (__BLE_COMBO_REF__)
        if (ble_combo_ref_flag == pdTRUE)
            combo_ble_sw_reset();
#endif    // __BLE_COMBO_REF__
        reboot_func(SYS_REBOOT);
    } else if (strcasecmp(argv[1], "STATUS") == 0) {
        show_dpm_sleep_info();
    } else if (strcasecmp(argv[1], "ABN") == 0) {
        if (argc == 3) {
            if (strcasecmp(argv[2], "HOLD") == 0) {
                dpm_abnormal_chk_hold();
            } else if (strcasecmp(argv[2], "RESUME") == 0) {
                dpm_abnormal_chk_resume();
            }
        } else if (argc == 4) {
            if (strcasecmp(argv[2], "WIFI_CONN_WAIT") == 0) {
                set_dpm_abnormal_wait_time((int)ctoi(argv[3]), DPM_ABNORM_WIFI_CONN_WAIT);
            }
#if 0    // Add this commands if needed ...
            else if (strcasecmp(argv[2], "DHCP_RSP_WAIT") == 0) {
                set_dpm_abnormal_wait_time(ctoi(argv[3]), DPM_ABNORM_DHCP_RSP_WAIT);
            } else if (strcasecmp(argv[2], "ARP_RSP_WAIT") == 0) {
                set_dpm_abnormal_wait_time(ctoi(argv[3]), DPM_ABNORM_ARP_RSP_WAIT);
            } else if (strcasecmp(argv[2], "DPM_FAIL_WAIT") == 0) {
                set_dpm_abnormal_wait_time(ctoi(argv[3]), DPM_ABNORM_DPM_FAIL_WAIT);
            }
#endif    // 0
            else if (strcasecmp(argv[2], "WIFI_CONN_RETRY_CNT") == 0) {
                set_dpm_abnormal_wait_time(atoi(argv[3]), DPM_ABNORM_DPM_WIFI_RETRY_CNT);    
            }
        }
    }
#ifdef __CHK_DPM_ABNORM_STS__
    else if (strcasecmp(argv[1], "MON") == 0) {
        if (argc == 3) {
            if (strcasecmp(argv[2], "COUNT") == 0) {
                dpm_show_dpm_monitor_count();
            } else if (strcasecmp(argv[2], "INFO") == 0) {
                dpm_show_dpm_monitor_info();
            } else if (strcasecmp(argv[2], "STOP") == 0) {
                dpm_monitor_stop();
            } else if (strcasecmp(argv[2], "RESTART") == 0) {
                dpm_monitor_restart();
            }
        }
    }
#endif    // __CHK_DPM_ABNORM_STS__
    else if (strcasecmp(argv[1], "HOLD") == 0) {
        if (chk_dpm_mode()) {
            PRINTF("\n- DPM Sleep Manager HOLD ...\n");
            dpm_daemon_cmd_hold_flag = pdTRUE;
            hold_dpm_sleepd();
        } else {
            PRINTF("FAIL \nNOT DPM MODE\n");
        }
    } else if (strcasecmp(argv[1], "RESUME") == 0) {
        if (chk_dpm_mode()) {
            PRINTF("\n- DPM Sleep Manager RESUME ...\n");
            resume_dpm_sleepd();
            dpm_daemon_cmd_hold_flag = pdFALSE;
        } else {
            PRINTF("FAIL \nNOT DPM MODE\n");
        }
    } else if (strcasecmp(argv[1], "RTM") == 0) {
        show_rtm_for_app();
    } else if (strcasecmp(argv[1], "RTMMAP") == 0) {
        extern void show_rtm_map(void);
        show_rtm_map();
    } else if (strcasecmp(argv[1], "TCP_PORT_LIST") == 0) {
        extern    void dpm_print_regi_tcp_port(void);
        dpm_print_regi_tcp_port();
    } else if (strcasecmp(argv[1], "TIMER") == 0) {
        dpm_user_timer_list_print();
    } else if (strcasecmp(argv[1], "TIMER_REMAIN") == 0) {
        PRINTF("Remain = %d\n", dpm_user_timer_get_remaining_msec(argv[2], argv[3]));
    } else if (strcasecmp(argv[1], "TIMER_DELETE") == 0) {
        dpm_user_timer_delete(argv[2], argv[3]);
    } else if (strcasecmp(argv[1], "TIMER_CHANGE") == 0) {
        dpm_user_timer_change(argv[2], argv[3], ctoi(argv[4]));
    } else if (strcasecmp(argv[1], "TIMER_DELETE_ALL") == 0) {
        dpm_user_timer_delete_all(2);
    } else if (strcasecmp(argv[1], "RTC") == 0) {
        if (argc == 2) {
            rtc_timer_list_info();
        } else {
            PRINTF("FAIL\n");
            goto set_usage;
        }
    } else if (strcasecmp(argv[1], "USER_POOL") == 0) {
        dpm_user_rtm_print();
    } else if (strcasecmp(argv[1], "EXTSET") == 0) {
        if (chk_dpm_mode()) {
            ext_wu_sleep_flag = pdTRUE;
        }
    } else if (strcasecmp(argv[1], "DEBUG") == 0) {
        if (chk_dpm_mode()) {
            if (argc == 3 && argv[2][0] >= '0' && argv[2][0] <= '4') {
                set_dpm_dbg_level(ctoi(argv[2]));
                PRINTF("DPM Debug Level : %d\n", get_dpm_dbg_level());
            } else {
                PRINTF("FAIL\n");
                goto set_usage;
            }
        } else {
            PRINTF("FAIL \nNOT DPM MODE\n");
        }
    } else if (strcasecmp(argv[1], "APSYNC") == 0) {
        extern void dpm_set_tim_ap_sync_cntrl(char);

        if (argc < 3) {
            goto set_usage;
        } else if (argc == 3) {
            if (strcasecmp(argv[2], "ON") == 0) {
                //AP Sync On Command
                PRINTF("AP Sync On\n");
                dpm_set_tim_ap_sync_cntrl(1);
            } else if (strcasecmp(argv[2], "OFF") == 0) {
                //AP Sync Off Command
                PRINTF("AP Sync Off\n");
                dpm_set_tim_ap_sync_cntrl(0);
            } else {
                goto set_usage;
            }
        } else {
            goto set_usage;
        }
    } else if (strcasecmp(argv[1], "PS") == 0) {
        extern void fc80211_set_rf_wifi_cntrl(int onoff);

        if (argc < 3) {
            goto set_usage;
        } else if (argc == 3) {
            if (strcasecmp(argv[2], "ON") == 0) {
                fc80211_set_rf_wifi_cntrl(1);
            } else if(strcasecmp(argv[2], "OFF") == 0) {
                fc80211_set_rf_wifi_cntrl(0);
            } else {
                goto set_usage;
            }
        } else {
            goto set_usage;
        }
    }
#ifdef __DPM_TIM_TEST_CMD__
    else if (strcasecmp(argv[1], "AutoARP") == 0) {
        if (argc < 3) {
            goto set_usage;
        } else if (argc == 3) {
            if (strcasecmp(argv[2], "OFF") == 0) {
                PRINTF("AutoARP Off\n");
                dpm_cmd_autoarp_en = 0;
            } else {
                goto set_usage;
            }
        } else {
            goto set_usage;
        }
    } else if (strcasecmp(argv[1], "DeauthChk") == 0) {
        extern void dpm_reset_env_deauth_en(char);

        if (argc < 3) {
            goto set_usage;
        } else if (argc == 3) {
            if (strcasecmp(argv[2], "OFF") == 0) {
                PRINTF("DeauthChk Off\n");
                dpm_reset_env_deauth_en(GET_DPMP());
                TENTRY()->sche_delete(GET_DPMP(), DPMSCHE_DEAUTH_CHK);
            } else {
                goto set_usage;
            }
        } else {
            goto set_usage;
        }

    }
#endif    // __DPM_TIM_TEST_CMD__
    else {
        PRINTF("FAIL\n");
        goto set_usage;
    }

    dpm_cmd_running = 0;

    return;
}

void cmd_set_dpm_tim_wu(int argc, char *argv[])
{
    int dpm_tim_wakeup_msec;

    if (argc < 2) {
        PRINTF("- Usage : dpm_tim_wu_time [1 .. 6000 (ex 3000ms --> 30.)]\n");
        return;
    }

    dpm_tim_wakeup_msec = atoi(argv[1]);

    set_dpm_tim_wakeup_dur((unsigned int)dpm_tim_wakeup_msec, 1);    //NVRAM SAVE
}

void cmd_set_dpm_tim_bcn_timeout(int argc, char *argv[])
{
    int dpm_tim_bcn_timout_nsec;

    if (argc < 2) {
        PRINTF("- Usage : dpm_bcn_timeout [1 .. 20(ex 6 --> 6000nsec.)]\n");
        return;
    }

    dpm_tim_bcn_timout_nsec = atoi(argv[1]);

    if (dpm_tim_bcn_timout_nsec < 1 ||   dpm_tim_bcn_timout_nsec > 60) {
        PRINTF("- Usage : dpm_bcn_timeout [1 .. 20(ex 6 --> 6000nsec.)]\n");
        return;
    }

    set_dpm_bcn_wait_timeout((unsigned int)dpm_tim_bcn_timout_nsec);
}

void cmd_set_dpm_tim_nobcn_step(int argc, char *argv[])
{
    int dpm_tim_nobcn_check_step;

    if (argc < 2) {
        PRINTF("- Usage : dpm_nobcn_step [1 .. 5]\n");
        return;
    }

    dpm_tim_nobcn_check_step = atoi(argv[1]);

    if (dpm_tim_nobcn_check_step < 1 || dpm_tim_nobcn_check_step > 5) {
        PRINTF("- Usage : dpm_nobcn_step [1 .. 5]\n");
        return;
    }

    set_dpm_nobcn_check_step(dpm_tim_nobcn_check_step);
}

/**
 * command for multicast address filter in TIM as white list
 */
void cmd_set_dpm_mc_filter(int argc, char *argv[])
{
    unsigned long     mc_filter_addr;

    if (argc < 2) {
        PRINTF("- Usage : dpm_mc_filter  [x.x.x.x(mc addr)]\n");
        return;
    }

    if (isvalidip(argv[1]) == 0) {
        PRINTF("- Usage : dpm_mc_filter  [x.x.x.x(mc addr)]\n");
        return;
    }

    mc_filter_addr = (ULONG)iptolong(argv[1]) & 0xffffffff;

    set_dpm_mc_filter( mc_filter_addr);
}

/**
 * command for UDP Dest port filter in TIM as white list
 */
void cmd_dpm_udp_tcp_filter_func(int argc, char *argv[])
{
    unsigned char usage_en = 0;
    int d_port;

    switch (argc) {
    case 2:
        usage_en = 1;
        break;

    case 3:
        if (strcmp(argv[1], "udp") == 0) {
            d_port = atoi(argv[2]);

            if (d_port == 0) {          /* udp filter port disable */
                PRINTF("%s:: UDP Port filter Disabled\n", argv[0]);
                dpm_udpf_cntrl(0);
            } else if (d_port == 1) {   /* udp filter port enable */
                PRINTF("%s:: UDP Port filter Enabled\n", argv[0]);
                dpm_udpf_cntrl(1);
            } else {
                if ((d_port < 0) || (d_port >= 0x10000)) {
                    PRINTF("%s::invalid d_port=%d\n", argv[0], d_port);
                    break;
                }

                set_dpm_udp_port_filter((unsigned short)d_port);
            }
        } else if (strcmp(argv[1], "tcp") == 0) {
            d_port = atoi(argv[2]);

            if (d_port == 0) {           /* udp filter port disable */
                PRINTF("%s:: TCP Port filter Disabled\n", argv[0]);
                dpm_tcpf_cntrl(0);
            } else if (d_port == 1) {      /* udp filter port enable */
                PRINTF("%s:: TCP Port filter Enabled\n", argv[0]);
                dpm_tcpf_cntrl(1);
            } else {
                if ((d_port < 0) || (d_port >= 0x10000)) {
                    PRINTF("%s::invalid d_port=%d\n", argv[0], d_port);
                    break;
                }

                set_dpm_tcp_port_filter((unsigned short)d_port);
            }
        } else {
            usage_en = 1;
        }

        break;

    default:
        usage_en = 1;
    }

    if (usage_en) {
        PRINTF("USAGE:\n");
        PRINTF("\ttcp/udp 1/0]: DPM UDP/TCP Filter Enable/Disable\n");
        PRINTF("\ttcp / udp xxxx(number) : DPM UDP / TDP Port filtering with TCP/UDP Filter enable \n");
    }
}

void cmd_set_dpm_user_wu_time(int argc, char *argv[])
{
    extern void user_wakeup_cb(char *timer_name);
    int    set_time, timer_id;

    if (argc < 2) {
set_err :
        PRINTF("- Usage : dpm_user_wu_time [%d .. %d(msec)]\n", MIN_DPM_USER_WAKEUP_TIME, MAX_DPM_USER_WAKEUP_TIME);
        return;
    }

    /* sec */
    if (argv[1][0] < '0' || argv[1][0] > '9') {
        goto set_err;
    }

    set_time = atoi(argv[1]);

    if (set_time < MIN_DPM_USER_WAKEUP_TIME || set_time > MAX_DPM_USER_WAKEUP_TIME) {
        goto set_err;
    }

    /* fixed ID = 2 */
    timer_id = rtc_register_timer((unsigned int)set_time, NULL, 2, 1, user_wakeup_cb);

    if (2 != timer_id) {
        PRINTF("ERR : Can't register user wake up time\n");
        goto set_err;
    }

    set_dpm_user_wu_time_to_nvram(set_time);

    PRINTF("- DPM User Wake up time : %d msec, Timer ID : %d\n", set_time, timer_id);
}

/* Keepalive configuration during DPM */
void cmd_set_dpm_ka_cfg(int argc, char *argv[])
{
    int    keep_alive_dur;

    if (argc < 2) {
set_err :
        PRINTF("- Usage : dpm_keep_conf [%d .. %d(ms, step=100)] \n",
               MIN_DPM_KEEPALIVE_TIME, MAX_DPM_KEEPALIVE_TIME);
        return;
    }

    if (argv[1][0] < '0' || argv[1][0] > '9') {
        goto set_err;
    }

    keep_alive_dur = atoi(argv[1]);

    if (   (keep_alive_dur < MIN_DPM_KEEPALIVE_TIME || keep_alive_dur > MAX_DPM_KEEPALIVE_TIME)
        || (keep_alive_dur % 100 != 0)) {
        goto set_err;
    }

    set_dpm_keepalive_config(keep_alive_dur );
    set_dpm_keepalive_to_nvram(keep_alive_dur);

    PRINTF("- DPM Sleep keepalive period time : %d.%d sec, Debug(%d)\n",
           keep_alive_dur / 1000, (keep_alive_dur / 100) % 10);
}

void cmd_getSysMode(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    printSysMode((UINT)(getSysMode()));
}

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#define ISDIGIT(c)( (c < '0' || c > '9') ? 0 : 1)
void cmd_setSysMode(int argc, char *argv[])
{
    if (argc == 1) {
        goto usage_msg;
    }

    if (   argc == 2
        && ISDIGIT(argv[1][0])
        && (strlen(argv[1]) == 1)
        && atoi(argv[1]) >= SYSMODE_STA_ONLY
#ifdef __SUPPORT_P2P__
        && atoi(argv[1]) <= SYSMODE_STA_N_P2P
#else
        && atoi(argv[1]) <= SYSMODE_STA_N_AP
#endif /* __SUPPORT_P2P__ */
        ) {

        if (setSysMode(atoi(argv[1])) == 0) {
            printSysMode(ctoi(argv[1]));
            return;
        }
    }

usage_msg :
    printSysMode(getSysMode());
#ifdef __SUPPORT_P2P__
    PRINTF("\nUsage : setsysmode [0|1|2|3|4|5]\n"
           "\t0 : Station\n"
           "\t1 : Soft-AP\n"
           "\t2 : WiFi Direct Mode\n"
           "\t3 : WiFi Direct P2P GO Fixed Mode\n"
           "\t4 : Station & Soft-AP\n"
           "\t5 : Station & WiFi Direct\n");
#else
    PRINTF("\nUsage : setsysmode [0|1|2]\n"
           "\t0 : Station\n"
           "\t1 : Soft-AP\n"
           "\t2 : Station & Soft-AP\n");
#endif /* __SUPPORT_P2P__ */
}

void cmd_set_switch_mode(int argc, char *argv[])
{
    if (   argc == 2
        && ISDIGIT(argv[1][0])
        && (strlen(argv[1]) == 1)
        && (atoi(argv[1]) >= SYSMODE_STA_ONLY)
        && (atoi(argv[1]) <= SYSMODE_AP_ONLY) ) {

        int switch_mode = atoi(argv[1]);

        // Save "SWITCH_SYSMODE" to NVRAM which will be used as return sysmode
        if (write_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)switch_mode) != 0) {
            PRINTF("\nFailed to write \"SWITCH_SYSMODE\" in NVRAM !!!\n");
        }
    } else {
        PRINTF("\nUsage : set_switch_mode [0|1]\n"
               "\t0 : Station\n"
               "\t1 : Soft-AP\n");
    }
}

void cmd_mode_switch(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argv);

    if (argc > 1) {
        PRINTF("\nUsage : mode_switch\n");
        return;
    }

    if (factory_reset_btn_onetouch() == pdTRUE) {
        vTaskDelay(1);

        // Reboot to change runinng mode
        reboot_func(SYS_REBOOT);
    }
}
#endif // __SUPPORT_WIFI_CONCURRENT__

UINT writeMACaddress(char *macaddr, int dst)
{
    UINT status = E_WRITE_OK;
    UINT  idx, len;
    char tmp_macstr[13];
    char tmpstr[3];

    memset(tmp_macstr, 0, 13);
    len = strlen(macaddr);

    if (len == 17 || len == 12) {
        if (len == 12) {
            sprintf(tmp_macstr, "%c%c%c%c%c%c%c%c%c%c%c%c",
                    tolower(macaddr[0]),  tolower(macaddr[1]),
                    tolower(macaddr[2]),  tolower(macaddr[3]),
                    tolower(macaddr[4]),  tolower(macaddr[5]),
                    tolower(macaddr[6]),  tolower(macaddr[7]),
                    tolower(macaddr[8]),  tolower(macaddr[9]),
                    tolower(macaddr[10]), tolower(macaddr[11]));
        } else {
            sprintf(tmp_macstr, "%c%c%c%c%c%c%c%c%c%c%c%c",
                    tolower(macaddr[0]),  tolower(macaddr[1]),
                    tolower(macaddr[3]),  tolower(macaddr[4]),
                    tolower(macaddr[6]),  tolower(macaddr[7]),
                    tolower(macaddr[9]),  tolower(macaddr[10]),
                    tolower(macaddr[12]), tolower(macaddr[13]),
                    tolower(macaddr[15]), tolower(macaddr[16]));
        }

        for (idx = 0; idx < 12; idx++) {
            if (!isxdigit((int)(tmp_macstr[idx]))) {
                status = E_DIGIT_ERROR;
                goto error;
            }
        }
        
        strncpy(tmpstr, tmp_macstr, 2);
        if ( !strncmp(tmp_macstr, "000000000000", 12) ||
             !strncmp(tmp_macstr, "ffffffffffff", 12) ||
              strtoul(tmpstr, NULL, 16) % 2 == 1 ) {
            status = E_INVALID_ERROR;
            goto error;
        }
    } else {
        if (   dst == MAC_SPOOFING
            && strncasecmp(macaddr, "erase", 5) == 0) {
            if (da16x_unsetenv(ENVIRON_DEVICE, "app", "MAC_SP") == FALSE) {
                status = E_ERASE_ERROR;
            } else {
                status = E_ERASE_OK;
            }
        } else if (dst == NVRAM_MAC && strncasecmp(macaddr, "erase", 5) == 0) {
            if (da16x_unsetenv(ENVIRON_DEVICE, "dev", "WLANMAC") == FALSE) {
                status = E_ERASE_ERROR;
            } else {
                status = E_ERASE_OK;
            }
        } else {
            status = E_DIGIT_ERROR; /* Error */
        }
    }

    if (status == E_WRITE_OK) {
        /*Check  last digit Even(Exception: Mac Spoofing) */
        if  (strtoul(tmp_macstr + 9, NULL, 16) % 2 == 0 || dst == MAC_SPOOFING) {
            status = E_WRITE_ERROR; /* Write Error */

            if (dst == MAC_SPOOFING) {
                if (da16x_setenv(ENVIRON_DEVICE, "app", "MAC_SP", tmp_macstr)) {
                    status = E_WRITE_OK; /* Write OK */
                } else {
                    status = E_WRITE_ERROR;
                }
            } else if (dst == NVRAM_MAC) {
                if (da16x_setenv(ENVIRON_DEVICE, "dev", "WLANMAC", tmp_macstr)) {
                    status = E_WRITE_OK; /* Write OK */
                } else {
                    status = E_WRITE_ERROR;
                }
            } else if (dst == OTP_MAC) {
                char    macaddr_otp_str[13] = {0,};
                SYS_OTP_MAC_ADDR_READ(macaddr_otp_str);

                if (strncasecmp(tmp_macstr, macaddr_otp_str, 12) == 0) {   /* Same Mac Address */
                    status = E_SAME_MACADDR;
                    goto error;
                }

                if (SYS_OTP_MAC_ADDR_WRITE(tmp_macstr) == 1) {    /* OTP MAC */
                    status = E_WRITE_OK; /* Write OK */
                } else {
                    status = E_WRITE_ERROR; /* Write ERROR */
                }
            }
        } else {
            status = E_DIGIT_ERROR; /* Error Odd */
        }
    }

error:
    return status;
}


int getMacAddrMswLsw(UINT iface, ULONG *macmsw, ULONG *maclsw)
{
    char   *macaddr_str = NULL;
    char   macaddr_otp_str[13] = {0,};
    int    idx;
    int    type = 0;

    /* MAC Spoofing for Station Only */
#ifdef    XIP_CACHE_BOOT
    if (get_run_mode() == SYSMODE_STA_ONLY) {
        macaddr_str = da16x_getenv(ENVIRON_DEVICE, "app", "MAC_SP");
        type = MAC_SPOOFING;
    }
#endif    /* XIP_CACHE_BOOT */

    /* NVRAM MAC Address */
    if (macaddr_str == NULL) {
        macaddr_str = da16x_getenv(ENVIRON_DEVICE, "dev", "WLANMAC");
        type = NVRAM_MAC;
    }

    /* OTP MAC Address*/
    if (macaddr_str == NULL) {
        if (SYS_OTP_MAC_ADDR_READ(macaddr_otp_str) < 0) {
            goto error;
        }

        macaddr_str = &macaddr_otp_str[0];
        type = OTP_MAC;
    }

    if (macaddr_str == NULL) {
        goto error;
    } else if (strlen(macaddr_str) == 12) {        /* New type -  EC9F0D900000 */
        char tmp_macstr[9];

        for (idx = 0; idx < 12; idx++) {
            if (!isxdigit((int)(macaddr_str[idx]))) {
                goto error;
            }
        }

        /* MSW */
        memset(tmp_macstr, 0, 9);
        memcpy(tmp_macstr, macaddr_str, 4);
        *macmsw  = strtoul(tmp_macstr, NULL, 16);

        /* LSW */
        memset(tmp_macstr, 0, 9);
        memcpy(tmp_macstr, macaddr_str + 4, 8);
        *maclsw  = strtoul(tmp_macstr, NULL, 16);

        if (iface == WLAN1_IFACE) {
            (*maclsw)++;
        }
    } else {
error:
        *macmsw = DEFULT_MAC_MSW;
        *maclsw = DEFULT_MAC_LSW;

        PRINTF("\33[41m<<Please check MAC address.>>\33[49m\n");
        return -1;
    }

    return type;
}

#define    NET_INFO_STR_LEN    18
int getMACAddrStr(UINT separate, char *macstr)
{
    int status = 0;
    ULONG macmsw, maclsw;

    /* Get MAC Address */
    status = getMacAddrMswLsw(0, &macmsw,  &maclsw);

    if (((int)status) >= 0) {
        if (separate) {
            snprintf(macstr, NET_INFO_STR_LEN, "%02lX:%02lX:%02lX:%02lX:%02lX:%02lX",
                     (macmsw >> 8),
                     (macmsw & 0xff),
                     (maclsw >> 24),
                     (maclsw >> 16 & 0xff),
                     (maclsw >> 8  & 0xff),
                     (maclsw & 0xff));
        } else {
            snprintf(macstr, 13, "%02lX%02lX%02lX%02lX%02lX%02lX",
                     (macmsw >> 8),
                     (macmsw & 0xff),
                     (maclsw >> 24),
                     (maclsw >> 16 & 0xff),
                     (maclsw >> 8  & 0xff),
                     (maclsw & 0xff));
        }
    }

    return status;
}


#if 1    /* F_F_S */
void cmd_getWLANMac(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    int type;
    char macstr[18];

    memset(macstr, 0, 18);

    type = getMACAddrStr(1, macstr);

    if (type < 0) {
        PRINTF("Read Error : WLAN MAC\n");
    } else {
        PRINTF("MAC TYPE: ");

        switch (type) {
        case MAC_SPOOFING:
            PRINTF("MAC Spoofing");
            break;

        case NVRAM_MAC:
            PRINTF("NVRAM MAC");
            break;

        case OTP_MAC:
            PRINTF("OTP MAC");
            break;
        }

        /* WLAN0 */
        PRINTF("\nWLAN0 - %s\n", macstr);

        /* WLAN1 */
        macstr[16] = (char)(macstr[16] + (char)1);
        PRINTF("WLAN1 - %s\n", macstr);
    }
}


/*
setwlanmac
setotpmac
macspoofing
*/
#define OTP_MAC_AVAILABLE 4
void cmd_setWLANMac(int argc, char *argv[])
{
    UINT status = E_WRITE_ERROR;

    if (argc == 1) {
        PRINTF("Usage: %s [xx:xx:xx:xx:xx:xx | xx-xx-xx-xx-xx-xx | xxxxxxxxxxxx | erase]\n\t(for Station Only)\n",
               argv[0]);
        return;
    }

    if (argc == 2) {
        size_t len = strlen(argv[1]);

        if (strcmp(argv[0], CMD_SETWLANMAC) == 0) {
            status = writeMACaddress(argv[1], NVRAM_MAC);

            if (status == E_WRITE_ERROR) {
                recovery_NVRAM();
                status = writeMACaddress(argv[1], NVRAM_MAC);
            }
        } else if (strcmp(argv[0], CMD_MACSPOOFING) == 0) {
            status = writeMACaddress(argv[1], MAC_SPOOFING);

            if (status == E_WRITE_ERROR) {
                recovery_NVRAM();
                status = writeMACaddress(argv[1], MAC_SPOOFING);
            }
        } else if (strcmp(argv[0], CMD_SETOTPMAC) == 0) {
            char    input_str[2];
            int        otp_mac_index = -1;
            char    macaddr_otp_str[13];
            char    tmp_macstr[17];

            memset(macaddr_otp_str, 0, 13);
            memset(tmp_macstr, 0, 17);

            if (len != 17 && len != 12) {
                status = E_UNKNOW;
                goto msg_error;
            }

            otp_mac_index = SYS_OTP_MAC_ADDR_READ(macaddr_otp_str);

            /* Read current OTP MAC */
            if (otp_mac_index != -1 && otp_mac_index < OTP_MAC_AVAILABLE) {
                PRINTF("\n[Current OTP MAC]\n\tOTP MAC Index: %d of 5\n", otp_mac_index + 1,
                       OTP_MAC_AVAILABLE);
                PRINTF("\tMAC Addr: %c%c:%c%c:%c%c:%c%c:%c%c:%c%c\n",
                       macaddr_otp_str[0], macaddr_otp_str[1],
                       macaddr_otp_str[2], macaddr_otp_str[3],
                       macaddr_otp_str[4], macaddr_otp_str[5],
                       macaddr_otp_str[6], macaddr_otp_str[7],
                       macaddr_otp_str[8], macaddr_otp_str[9],
                       macaddr_otp_str[10], macaddr_otp_str[11]);
            } else if (otp_mac_index == 3) {
                PRINTF("Could not change OTP MAC address.(%d of %)\n", otp_mac_index + 1,
                       OTP_MAC_AVAILABLE);
                return;
            }

            PRINTF(ANSI_BOLD ANSI_COLOR_GREEN "Input MAC Addres: ");

            if (len == 12) {
                sprintf(tmp_macstr, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
                        tolower(argv[1][0]), tolower(argv[1][1]),
                        tolower(argv[1][2]), tolower(argv[1][3]),
                        tolower(argv[1][4]), tolower(argv[1][5]),
                        tolower(argv[1][6]), tolower(argv[1][7]),
                        tolower(argv[1][8]), tolower(argv[1][9]),
                        tolower(argv[1][10]), tolower(argv[1][11]));
            } else if (len == 17) {
                sprintf(tmp_macstr, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
                        tolower(argv[1][0]), tolower(argv[1][1]),
                        tolower(argv[1][3]), tolower(argv[1][4]),
                        tolower(argv[1][6]), tolower(argv[1][7]),
                        tolower(argv[1][9]), tolower(argv[1][10]),
                        tolower(argv[1][12]), tolower(argv[1][13]),
                        tolower(argv[1][15]), tolower(argv[1][16]));
            }

            PRINTF("%s" ANSI_NORMAL "\n\n", tmp_macstr);

            if (compare_macaddr(macaddr_otp_str, argv[1])) {
                status = E_SAME_MACADDR;
                goto msg_error;
            }

            PRINTF(" Are you sure ? [" ANSI_BOLD "N" ANSI_NORMAL "o/"  ANSI_BOLD "Y" ANSI_NORMAL
                   "es] : ");

            getStr(input_str, 1);

            if (toupper(input_str[0]) == 'Y') {
                status = writeMACaddress(argv[1], OTP_MAC);
            } else {
                status = E_CANCELED;
            }
        }
    }

msg_error:

    switch (status) {
    case E_WRITE_OK:
        PRINTF("Write OK\n");
        break;

    case E_WRITE_ERROR:
        PRINTF("Write Error\n");
        break;

    case E_ERASE_OK:
        PRINTF("Erase OK\n");
        break;

    case E_ERASE_ERROR:
        PRINTF("Erase ERROR\n");
        break;

    case E_DIGIT_ERROR:
        PRINTF("ERR: Last Digit - Need to Even-number ( 0, 2, 4, 6 , 8, A, C, or E )\n");
        break;

    case E_SAME_MACADDR:
        PRINTF("Same MAC Address!!\n");
        PRINTF("Canceled\n");
        break;

    case E_CANCELED:
        PRINTF("Canceled\n");
        break;
        
    case E_INVALID_ERROR:
        PRINTF("ERR: Invalid MAC Address\n");
        break;

    default:
        PRINTF("ERR: option %s\nex) %s [xx:xx:xx:xx:xx:xx |"
               " xx-xx-xx-xx-xx-xx | xxxxxxxxxxxx%s]\n",
               argc > 1 ? argv[1] : "", argv[0],
               strcmp(argv[0], CMD_SETOTPMAC) == 0 ? "" :
               " | erase");
        break;
    }
}


/* Validation check the local subnet of defined interface */
int isvalidIPsubnetInterface(long ip, int iface)
{
    int    status;
    char    ipstr[16];
    char    smstr[16];

    /* IP Address */
    get_ip_info(iface, GET_IPADDR, ipstr);

    /* Subnet */
    get_ip_info(iface, GET_SUBNET, smstr);

    status = isvalidIPsubnetRange(ip, iptolong(ipstr), iptolong(smstr));

    return status;
}
#endif    /* F_F_S */

extern void easy_setup(void);
void cmd_setup(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    easy_setup();
    return;
}


#ifdef    XIP_CACHE_BOOT
#ifdef    __SUPPORT_SIGMA_TEST__    /* by Shingu 20170215 (TTA Certified) */
static int check_sigma_cli(int argc, char *argv[])
{
    char input_str[3];
    extern int getStr(char *, int);

    if (argc == 5 && argv[2][0] == '1' &&
            strcasecmp(argv[3], "key_mgmt") == 0 && strcasecmp(argv[4], "NONE") == 0) {
        PRINTF("[Certified] AP is using open networking. (no security)\n");
    } else if (argc == 5 && argv[2][0] == '1' &&
             strcasecmp(argv[3], "ignore_broadcast_ssid") == 0 && argv[4][0] == '1') {
        PRINTF("[Certified] AP disables broadcating of the SSID.\n");
    } else {
        return 1;
    }

    PRINTF("Are you sure ? [Y/N] ");

    memset(input_str, 0, 3);
    getStr(input_str, 2);
    input_str[0] = toupper(input_str[0]);

    if (input_str[0] == 'N') {
        PRINTF("\nSetting canceled.\n");
        return -1;
    }

    return 0;
}
#endif    /* __SUPPORT_SIGMA_TEST__ */

extern void da16x_cli(int argc, char *argv[]);
void cmd_da16x_cli(int argc, char *argv[])
{
    if (argc < 2) {
        PRINTF("\tUnknown command options '%s'\n\n", argv[0]);
        return;
    }

#ifdef    __SUPPORT_SIGMA_TEST__    /* by Shingu 20170215 (TTA Certified) */

    if (check_sigma_cli(argc, argv) < 0) {
        return;
    }

#endif    /* __SUPPORT_SIGMA_TEST__ */

    da16x_cli(argc, argv);
}

#if LWIP_ARP
void cmd_arp_table(int argc, char *argv[])
{

    UINT status;
    UINT iface = 0;
    UCHAR tmp_str[ARP_TABLE_SIZE];

    memset(tmp_str, 0, 10);

    if (argc == 1 || strcasecmp(argv[1], "HELP") == 0) {
arp_help:
        PRINTF("    Usage : arp [option] [interface]\n");
        PRINTF("     option\n");
        PRINTF("\t [-a]\t : Print all of ARP table\n");
        PRINTF("\t [-d]\t : Delete all of ARP table\n");
        PRINTF("     interface\n");
        PRINTF("\t [WLAN0 or WLAN1]\n");
        PRINTF("\n");
        return;
    }

    if (strcasecmp(argv[1], "WLAN0") == 0 && argc == 2) {     /* WLAN0 */
        iface = WLAN0_IFACE;
        status = print_arp_table(iface);
    } else if (strcasecmp(argv[1], "WLAN1") == 0 && argc == 2) {    /* WLAN1 */
        iface = WLAN1_IFACE;
        status = print_arp_table(iface);
    } else if (strcasecmp(argv[1], "-A") == 0) {
        for (int i = 0; i < 2; i++) {
            print_arp_table((UINT)i);
        }

        status = pdPASS;
    } else if (strcasecmp(argv[1], "-D") == 0) {
        for (int i = 0; i < 2; i++) {
            arp_entry_delete(i);
        }

        status = pdPASS;
    } else {
        goto arp_help;
    }

    if (status != pdPASS) {
        goto arp_help;
    }

}
#else /*LWIP_ARP*/
void cmd_arp_table(int argc, char *argv[])
{
    UINT status;
    UINT iface = 0;
    UCHAR tmp_str[10];

    extern int arp_entry_delete(int iface);

    memset(tmp_str, 0, 10);

    if (argc == 1 || strcasecmp(argv[1], "HELP") == 0) {
arp_help:
        PRINTF("    Usage : arp [option] [interface]\n");
        PRINTF("     option\n");
        PRINTF("\t [-a]\t : Print all of ARP table\n");
        PRINTF("\t [-d]\t : Delete all of ARP table\n");
        PRINTF("     interface\n");
        PRINTF("\t [WLAN0 or WLAN1]\n");
        PRINTF("\n");
        return;
    }

    if (strcasecmp(argv[1], "WLAN0") == 0 && argc == 2) {     /* WLAN0 */
        iface = WLAN0_IFACE;
        status = arp_table(iface);
    } else if (strcasecmp(argv[1], "WLAN1") == 0 && argc == 2) {    /* WLAN1 */
        iface = WLAN1_IFACE;
        status = arp_table(iface);
    } else if (strcasecmp(argv[1], "-A") == 0) {
        for (int i = 0; i < 2; i++) {
            arp_table(i);
        }

        status = ERR_OK;
    } else if (strcasecmp(argv[1], "-D") == 0) {
        for (int i = 0; i < 2; i++) {
            arp_entry_delete(i);
        }

        status = ERR_OK;
    } else {
        goto arp_help;
    }

    if (status != ERR_OK) {
        goto arp_help;
    }
}
#endif    /*LWIP_ARP*/

void cmd_network_dhcpd(int argc, char *argv[])
{
    int status = ERR_OK;

    extern UINT is_supplicant_done();

    dhcps_cmd_param *param = NULL;
    ULONG start_ip, end_ip, ip_addr, net_mask, gw_addr = 0;

    switch (get_run_mode()) {
        case SYSMODE_STA_ONLY:
            goto not_support;

#ifdef __SUPPORT_P2P__
        case SYSMODE_P2P_ONLY:
        case SYSMODE_P2PGO:
        case SYSMODE_STA_N_P2P:
            if (get_netmode(WLAN1_IFACE) == DHCPCLIENT){
                goto not_support;
            }
            break;
#endif /* __SUPPORT_P2P__ */

        default:
            /* Don't support "dhcpd command" when no connection state */
            if (is_supplicant_done() == 0){
not_support :
                PRINTF("Notice : Doesn't support 'dhcpd' command ...\n");
                return;
            }
    }

    if (argc == 2 && strcasecmp(argv[1], "HELP") == 0) {
        usage_dhcpd();
        return;
    } else if (dhcpd_support_flag == pdFALSE) {
        PRINTF(">> Doesn't support DHCP server !!!\n");
        return;
    }

    if (argc < 2) {
        status = ERR_ARG;
    }

#ifdef    __SUPPORT_IPV6__
    if (ipv6_support_flag == pdTRUE) {
        for (int n = 0; n < argc; n++) {
            if (strcmp(argv[n], "-6") == 0) {
                if (argc < 3) {
                    usage_dhcpd();
                    return;
                }

                ver = 6;
            }
        }
    }

#endif    /* __SUPPORT_IPV6__ */


    param = pvPortMalloc(sizeof(dhcps_cmd_param));
    memset(param, 0, sizeof(dhcps_cmd_param));

    for (int n = 1; n < argc; n++) {
        //Parse & execute dhcp server
        if (strcasecmp(argv[n], "STATUS") == 0) {
            dhcps_get_info();
            break;
        } else if (strcasecmp(argv[n], "RANGE") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            if ( argc < 4 ) {
                status = ERR_ARG;
            } else {
                start_ip = (ULONG)iptolong(argv[n+1]);
                end_ip = (ULONG)iptolong(argv[n+2]);

                if (is_dhcp_server_running() == 1 /* DHCPD is in running state */) {
                    ip_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->ip_addr));
                    net_mask = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->netmask));
                    gw_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->gw));

                } else {
                    /* DHCPD is in stopped state */
                    struct netif *netif = netif_get_by_index(WLAN1_IFACE + 2); // assume Softap mode
                    ip_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->ip_addr));
                    net_mask = (ULONG)iptolong(ipaddr_ntoa(&netif->netmask));
                    gw_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->gw));
                }
                
                if (is_in_valid_ip_class(argv[n+1]) && is_in_valid_ip_class(argv[n+2])) {
                    if( ((ip_addr >> 8) != (start_ip >> 8)) || ((ip_addr >> 8) != (end_ip >> 8)) ) {
                        PRINTF("ERR: Failed to set range of IP_addr list.\n");
                        status = ERR_ARG;
                        break;
                    }

                    if (isvalidIPsubnetRange(start_ip,  gw_addr, net_mask) == pdFALSE) {
                        PRINTF("ERR: Start IP_addr is out of range. \n");
                        status = ERR_ARG;
                        break;
                    }

                    if (isvalidIPsubnetRange(end_ip,  gw_addr, net_mask) == pdFALSE) {
                        PRINTF("ERR: End IP_addr is out of range. \n");
                        status = ERR_ARG;
                        break;
                    }
                    if (isvalidIPrange(ip_addr, start_ip, end_ip)) {
                        PRINTF("ERR: IP_addr is out of DHCP range. \n");
                        status = ERR_ARG;
                        break;
                    }

                    if((end_ip - start_ip + 1) > DHCPS_MAX_LEASE) {
                        PRINTF("ERR: Failed to set range of IP_addr list(Max:%d).\n", DHCPS_MAX_LEASE);
                        status = ERR_ARG;
                        break;
                    }

                    /* Lease range */
                    da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_START_IP, argv[n+1]);
                    da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_END_IP, argv[n+2]);
                    da16x_nvcache2flash();
                    dhcps_set_ip_range(argv[n+1], argv[n+2]);
                } else {
                    PRINTF(">> DHCP IP RANGE invalid.\n");
                    status = ERR_ARG;
                }
            }
            break;
        } else if (strcasecmp(argv[n], "LEASE_TIME") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            if ((atoi(argv[n + 1]) < MIN_DHCP_SERVER_LEASE_TIME) || 
                (atoi(argv[n + 1]) > MAX_DHCP_SERVER_LEASE_TIME)) {
                goto dhcpd_help;
            }

            dhcps_time_t lease_time = (u32_t)atoi(argv[n + 1]);

            if (lease_time > 0) {
                /* Lease time */
                da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_LEASE_TIME, (int)lease_time);
                da16x_nvcache2flash();
                dhcps_set_lease_time(lease_time);
            } else {
                PRINTF(">> DHCP Lease Time invalid.\n");
                status = ERR_ARG;
            }
            break;
        } else if (strcasecmp(argv[n], "LEASE") == 0) {
            /* Lease List */
            u8_t is_assigned = (u8_t)atoi(argv[n+1]);
            dhcps_print_lease_pool(is_assigned);
            break;
        }
#if defined ( __SUPPORT_MESH_PORTAL__ ) || defined (__SUPPORT_MESH__)
        else if (strcasecmp(argv[n], "DNS") == 0)
        {
            if (argc == 3
                && (get_run_mode() == SYSMODE_STA_N_MESH_PORTAL || get_run_mode() == SYSMODE_MESH_POINT))
            {
                if (is_in_valid_ip_class(argv[n+1])) {
                    ip4_addr_t dns_addr;
                    ip4addr_aton(argv[n+1], &dns_addr);
                    da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_DNS, argv[n+1]);
                    dhcps_dns_setserver(&dns_addr);
                } else {
                    PRINTF("DHCP DNS address invalid.\n");
                    status = ERR_ARG;
                }
                break;
            }
            goto dhcpd_help;
        }
#endif // __SUPPORT_MESH_PORTAL__ || __SUPPORT_MESH__
        else if (strcasecmp(argv[n], "RESPONSE_DELAY") == 0) {
//            status = set_response_delay(atoi(argv[n + 1]));
            continue;
        } else if (strcasecmp(argv[n + 1], "WLAN0") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            param->dhcps_interface = WLAN0_IFACE;
            continue;
        } else if (strcasecmp(argv[n + 1], "WLAN1") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            param->dhcps_interface = WLAN1_IFACE;
            continue;
        } else if (strcasecmp(argv[n], "BOOT") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            if (strcasecmp(argv[n + 1], "ON") == 0) {
                da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_SERVER, CC_VAL_ENABLE);    /* DHCP Server boot Start */
            } else {
                delete_tmp_nvram_env("USEDHCPD");
            }
            
            da16x_nvcache2flash();
            break;
        } else if (strcasecmp(argv[n], "START") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            param->cmd = DHCP_SERVER_STATE_START;
            param->dhcps_interface = WLAN1_IFACE;
            dhcps_run(param);
            return;
        } else if (strcasecmp(argv[n], "STOP") == 0) {
#ifdef __SUPPORT_P2P__
            switch (get_run_mode()) {
                case SYSMODE_P2P_ONLY:
                case SYSMODE_P2PGO:
                case SYSMODE_STA_N_P2P:
                    goto dhcpd_help;
            }
#endif /* __SUPPORT_P2P__ */

            param->cmd = DHCP_SERVER_STATE_STOP;
            dhcps_run(param);
            return;
        }
        else
        {
            goto dhcpd_help;
        }
    }

    if (status != ERR_OK) {
dhcpd_help:
        usage_dhcpd();
    }

    if (param != NULL) {
        vPortFree(param);
    }
}

void cmd_network_config(int argc, char *argv[])
{
    int iface = WLAN0_IFACE;
    struct netif *netif;
    char     tmp_str[10];
    err_t status;

    if (argv[1][0] == '?') {
help:
        PRINTF( "\nUsage:\n"
                "\tifconfig [interface] [ipaddress] [subnet] [gateway]\t- set Netmode: Static IP\n"
                "\tifconfig [interface] [dhcp]\t\t- set Netmode: DHCP Client\n");
        PRINTF( "\tifconfig [interface] [up|down]\t\t- Network interface up/down\n"
                "\tifconfig [interface] [start|stop|renew|release] \t- DHCP Clinet cmd\n");
        PRINTF( "\tifconfig [interface] [dns] [ipaddress]\t- set DNS IP\n"
                "\tifconfig [interface] [dns2] [ipaddress]\t- set 2nd DNS IP\n"
                "\tifconfig [interface]    \t\t- Display config [interface]\n");

#ifdef    __SUPPORT_IPV6__
        if (ipv6_support_flag == pdTRUE) {
            PRINTF( "\tifconfig [interface] -6 [ipv6address]\t- set Net6mode: Static Global IPv6\n");
        }

#endif    /* __SUPPORT_IPV6__ */

        PRINTF( "\tifconfig -a\t\t\t\t- Display full configuration information.\n"
                "\tifconfig help or ?\t\t\t- This message\n\n");
        return;
    }

    if (argc >= 2) {
        /* ifconfig [wlan0|wlan1|eth0|all|-a]    */
        if (strcasecmp(argv[1], "WLAN0") == 0) {        /* WLAN0 */
            iface = WLAN0_IFACE;
        } else if (strcasecmp(argv[1], "WLAN1") == 0) {    /* WLAN1 */
            iface = WLAN1_IFACE;
        } else if (strcasecmp(argv[1], "-A") == 0 || strcasecmp(argv[1], "ALL") == 0) {
            for (int i = WLAN0_IFACE; i <= WLAN1_IFACE; i++) {
                da16x_net_check(i, pdFALSE);
            }

            return;
        } else {
            goto help;
        }
    }

    netif = netif_get_by_index((u8_t)(iface+2));

    switch (argc) {
        case 1:
        {
            /* ifconfig (All interfaces simplied) */
            for (int i = WLAN0_IFACE; i <= WLAN1_IFACE; i++) {
                da16x_net_check(i, pdTRUE);
            }
    
            break;
        }
    
        case 2:
        {
            da16x_net_check((int)iface, 0);
        }    break;
    
        case 3:
        {
            /* ifconfig [wlan0|wlan1|eth0] [up|down]    */
            /* ifconfig [wlan0|wlan1|eth0] [dhcp]    */
            /* ifconfig [wlan0|wlan1|eth0] [renew]    */
    
            if (strcasecmp(argv[2], "UP") == 0) {            /* UP */
                wifi_netif_control(iface, IFACE_UP);
                iface_updown((UINT)iface, IFACE_UP);
            } else if (strcasecmp(argv[2], "DOWN") == 0) {    /* DOWN */
                wifi_netif_control(iface, IFACE_DOWN);
            } else if (get_netmode(iface) == DHCPCLIENT) {
                /* DHCP Client  Renew */
                if (strcasecmp(argv[2], "RENEW") == 0) {
                    if (chk_dpm_wakeup() == pdTRUE)
                        status = dhcp_start(netif);
                    else
                        status = dhcp_renew(netif);
                } else if (strcasecmp(argv[2], "RELEASE") == 0) {
                    status = dhcp_release(netif);
                    PRINTF("\nRelease DHCP Client :");
    
                    if (status != ERR_OK) {
                        PRINTF(" Error WLAN%u.\n", iface);
                    } else {
                        PRINTF(" Success WLAN%d.\n", iface);
                    }
                } else  if (strcasecmp(argv[2], "START") == 0) {
                    status = dhcp_start(netif);
    
                    if (status == ERR_OK) {
                        PRINTF("\nDHCP Client Started WLAN%u.\n", iface);
                    } else {
                        PRINTF("\nDHCP Client Start Error(%d) WLAN%u.\n", status, iface);
                    }
                } else if (strcasecmp(argv[2], "STOP") == 0) {
                    dhcp_stop(netif);
                } else if (strcasecmp(argv[2], "DHCP") == 0) {
                    PRINTF("\nAlready DHCP Client Mode WLAN%u.\n", iface);
                } else {
                    goto help;
                }
            } else if (strncasecmp(argv[2], "DHCP", 4) == 0 && (iface == WLAN0_IFACE)) {
                /* Set DHCP Client */
                set_netmode((UCHAR)iface, DHCPCLIENT, TRUE);
                PRINTF("\nDHCP Client Mode(WLAN0)\n");
    
                if (strcasecmp(argv[2], "DHCP_OFF") != 0) {
                    status = dhcp_start(netif);/* DHCP Client : START */
                }
            } else {
                goto help;
            }
    
            break;
        }
    
        case 4:
        {
            /* ifconfig [wlan0|wlan1|eth0] [dns] [DNS Server IP]     */
            ip4_addr_t ipaddr;
    
            if (strcasecmp(argv[2], "DNS") == 0) {
                ipaddr_aton(argv[3], &ipaddr);
                dns_setserver(0, &ipaddr);
                memset(tmp_str, 0, 10);
                snprintf(tmp_str, 9, "%d:%s", iface, STATIC_IP_DNS_SERVER);

                write_nvram_string(tmp_str, argv[3]); /* SET DNS */
    
                PRINTF("DNS:%s \n", argv[3]);
            } else if (strcasecmp(argv[2], "DNS2") == 0) {
                ipaddr_aton(argv[3], &ipaddr);
                dns_setserver(1, &ipaddr);
                memset(tmp_str, 0, 10);
                snprintf(tmp_str, 10, "%d:%s2", iface, STATIC_IP_DNS_SERVER);

                write_nvram_string(tmp_str, argv[3]); /* SET DNS */

                PRINTF("DNS2:%s \n", argv[3]);
            } else {
                goto help;
            }
    
            break;
        }
    
        case 5:
        {
#ifdef    __SUPPORT_IPV6__
    
            /* ifconfig [wlan0|wlan1|eth0] [-6] dhcp [on|off] */
            if (ipv6_support_flag == pdTRUE) {
                if (strcasecmp(argv[3], "DHCP") == 0) {
                    if (strcasecmp(argv[4], "ON") == 0) {
                        write_nvram_int("DHCPv6", 1);
                    } else if (strcasecmp(argv[4], "OFF") == 0) {
                        write_nvram_int("DHCPv6", 0);
                    }
    
                    return;
                }
            }
    
#endif    /* __SUPPORT_IPV6__ */
            if (!is_in_valid_ip_class(argv[2]) || !isvalidip(argv[3]) || !is_in_valid_ip_class(argv[4])) {
                PRINTF("\nInvalid Address!!\n");
                goto help;
                break;
            }

            if (ip_change(iface, argv[2], argv[3], argv[4], pdTRUE) == pdFAIL) {
                PRINTF("\nInvalid Network Address!!\n");
                goto help;
                break;
            }
 
            PRINTF("\n[WLAN%d]\n" \
                   "NetMode\t\t:Static IP\n" \
                   "IP Address\t:%s\n" \
                   "Mask\t\t:%s\n" \
                   "Gateway\t\t:%s\n",
                   iface, argv[2], argv[3], argv[4]);

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
            delete_nvram_env(ENV_KEY_TEMP_STATIC_IP);
#endif /* __SUPPORT_DHCPC_IP_TO_STATIC_IP__ */
            break;
        }
    
        default:
        {
            goto help;
        }
    }
}
#endif    /*XIP_CACHE_BOOT*/

UINT factory_reset(int reboot_flag)
{
    UINT status;

#if defined (__SUPPORT_MQTT__)
    mqtt_client_termination();
#endif

    if (reboot_flag) {
        PRINTF("\nRebooting....");
    }

    PRINTF(ANSI_NORMAL "\n\n");
    vTaskDelay(2);

#ifdef    XIP_CACHE_BOOT
    if (get_run_mode() == SYSMODE_STA_ONLY) {
        /* Stop if in running dpm_sleep_daemon ... */
#if defined (__BLE_COMBO_REF__)
        if (ble_combo_ref_flag == pdTRUE) {
            if (chk_dpm_mode()) {
                disable_dpm_mode();
            }
        }
#endif // __BLE_COMBO_REF__
    }
#endif    /*XIP_CACHE_BOOT*/

    cert_flash_delete_all();

#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
    extern int da16x_dns_2nd_cache_erase_sflash(void);
    da16x_dns_2nd_cache_erase_sflash();
#endif


    status = da16x_clearenv(ENVIRON_DEVICE, "app");    /* Factory reset */
    if (status == FALSE) {
        status = (UINT)(recovery_NVRAM());
    }

    if (get_run_mode() == SYSMODE_STA_ONLY) {
        /* Clear Only retention memory data */
        if (chk_retention_mem_exist() == pdTRUE) {
            clr_dpm_conn_info();
        }
    }

    if (reboot_flag) {
#if defined (__BLE_COMBO_REF__)
        if (ble_combo_ref_flag == pdTRUE)
            combo_ble_sw_reset();
#endif    // __BLE_COMBO_REF__
        reboot_func(SYS_REBOOT);    /* Reset */
    }

    if (status == TRUE) {
        return ERR_OK;
    }

    return ER_DELETE_ERROR;
}

void cmd_factory_reset(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    char input_str[2];

    PRINTF("FACTORY RESET [N/y/?] ");

    getStr(input_str, 1);

    if (toupper(input_str[0]) == 'Y') {
        PRINTF("\n" ANSI_COLOR_RED "Start Factory-Reset ...\n" ANSI_COLOR_DEFULT);
         factory_reset(1);
    }
}

int get_gpio(UINT num)
{
    HANDLE    hgpio;
    UINT32 ioctldata = 0;
    UINT16    data = 0;
    UINT32 gpio_pin;

    hgpio = GPIO_CREATE(GPIO_UNIT_A);

    gpio_pin = (UINT32)(1 << num);

    if (hgpio != NULL) {

        if (gpio_pin & 0x0001) {    /* GPIO 0 */
            _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
        }

        if (gpio_pin & 0x0004) {    /* GPIO 2 */
            _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
        }

        if (gpio_pin & 0x0010) {    /* GPIO 4 */
            _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
        }

        if (gpio_pin & 0x0040) {    /* GPIO 6 */
            _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
        }

        if (gpio_pin & 0x0100) {    /* GPIO 8 */
            _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
        }

        if (gpio_pin & 0x0400) {    /* GPIO 10 */
            _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
        }

        GPIO_INIT(hgpio);
        ioctldata = 0x00;

        // in
        ioctldata = gpio_pin;
        GPIO_IOCTL(hgpio, GPIO_SET_INPUT, &ioctldata);
        GPIO_READ(hgpio, gpio_pin, &data, sizeof(UINT16));
        //PRINTF("\nGPIO in - Port %x/Pin %d(%x) : %x \n", port, num, gpio_pin, data);

        GPIO_CLOSE(hgpio);

        if (data == gpio_pin) {
            return 0;
        } else {
            return 1;
        }
    }

    return 0;
}


/* macaddr xx:xx:xx:xx:xx:xx */
UINT wps_setup(char *macaddr)
{
    char cmd_str[30];
    char reply[10];

    memset(cmd_str, 0, 30);
    memset(reply, 0, 10);

    sprintf(cmd_str, "wps_pbc %s", strlen(macaddr) == 17 ? macaddr : "any");

    da16x_cli_reply(cmd_str, NULL, reply);

    if (strcasecmp(reply, "OK") == 0) {
        return 1;
    }

    return 0;
}

#if defined ( __SUPPORT_EVK_LED__ )
int LED_OnOff(int num, int flag)
{
    UINT32    ioctldata;
    UINT16    gpiodata = 0;
    HANDLE    hgpio;

    if (num < 0) {
        return -1;
    }

    hgpio = GPIO_CREATE(GPIO_UNIT_A);

    ioctldata = 1 << num;

    GPIO_IOCTL(hgpio, GPIO_SET_OUTPUT, &ioctldata);
    GPIO_INIT(hgpio);

    if (flag) {
        gpiodata = (1 << num);
    } else {
        gpiodata = (0 << num);
    }

    GPIO_WRITE(hgpio, (UINT32)(ioctldata), (UINT16 *) & (gpiodata), sizeof(UINT16));
    GPIO_CLOSE(hgpio);

    return 1;
}
#endif    // __SUPPORT_EVK_LED__

#if defined ( __SUPPORT_EVK_LED__ )
UINT check_factory_button(int btn_gpio_num, int led_gpio_num, int check_time)
#else
UINT check_factory_button(int btn_gpio_num, int check_time)
#endif    // __SUPPORT_EVK_LED__
{
    if (factory_rst_btn_flag == pdFALSE) {
        return pdFALSE;
    }

    int gpio = 0;
    int check_time_cnt = 0;
    int ret = 0;
#if defined(__SUPPORT_SYS_WATCHDOG__)
    unsigned int wdog_status = pdFALSE;

    da16x_sys_watchdog_get_status(&wdog_status);

    if (wdog_status) {
        da16x_sys_watchdog_disable();
    }
#endif // __SUPPORT_SYS_WATCHDOG__

    PRINTF("\n");

    /* SETP #1 : check button pushed */
    do {
        gpio = get_gpio((UINT)btn_gpio_num);

        if (gpio == 1) {
#if defined ( __SUPPORT_EVK_LED__ )
            if (check_time_cnt == 2) {
                LED_OnOff(led_gpio_num, LED_ON); // LED On
            }
#endif    // __SUPPORT_EVK_LED__

            check_time_cnt++;
        } else {
            if ((check_time_cnt > 10) && (check_time_cnt < (10 * check_time))) {  /* 1 ~ reset_time Sec. */

                PRINTF("\nRebooting\n");
#if defined ( __SUPPORT_EVK_LED__ )
                LED_OnOff(led_gpio_num, LED_OFF); // LED Off
#endif    // __SUPPORT_EVK_LED__

                ret = 2; /* Reboot */
                goto end;
            } else if ((check_time_cnt > 0) && (check_time_cnt < 10)) {
                // In case of one-touch button push ...
                //
                /* Registered in system_start.c by application operation */
                if (button1_one_touch_cb != NULL) {
                    if (button1_one_touch_cb() == pdTRUE) {
                        // Reboot to change runinng mode
                        reboot_func(SYS_REBOOT);
                    }
                }
            }

#if defined ( __SUPPORT_EVK_LED__ )
            LED_OnOff(led_gpio_num, LED_OFF); // LED Off
#endif    // __SUPPORT_EVK_LED__

            ret = 0; /* false */
            goto end;
        }

        vTaskDelay(CHECK_STEP_FACTORY_BTN);

        if (check_time_cnt >= MIN_TIME_FOR_CHECK_FACTORY_RST) {
            PRINTF("\33[2K" "Factory reset after %d seconds\r", check_time - (check_time_cnt / CHECK_STEP_FACTORY_BTN));
        }
    } while (check_time_cnt < (CHECK_STEP_FACTORY_BTN * check_time));

    PRINTF("\33[2K" "Factory reset ready.\n");

    /* SETP #2 : check button released */
    do {
        gpio = get_gpio((UINT)btn_gpio_num);

        if (gpio == 0) {
            ret = 1; /* factory reset */
            goto end;
        }
#if defined ( __SUPPORT_EVK_LED__ )
        else {   /* sw release */
            /* Factory Status LED Blink */
            LED_OnOff(led_gpio_num, LED_ON);
            vTaskDelay(CHECK_STEP_FACTORY_BTN); /* 100ms */
            LED_OnOff(led_gpio_num, LED_OFF);
            vTaskDelay(CHECK_STEP_FACTORY_BTN); /* 100ms */
        }
#endif    // __SUPPORT_EVK_LED__
    } while (1);

end:

#if defined(__SUPPORT_SYS_WATCHDOG__)
    if (wdog_status) {
        da16x_sys_watchdog_enable();
    }
#endif // __SUPPORT_SYS_WATCHDOG__

    return ret;
}

#if defined ( __SUPPORT_EVK_LED__ )
UINT check_wps_button(int btn_gpio_num, int led_gpio_num, int check_time)
#else
UINT check_wps_button(int btn_gpio_num, int check_time)
#endif    // __SUPPORT_EVK_LED__
{
    UINT    result = pdFALSE;

    if (wps_btn_flag_flag == pdFALSE) {
        return result;
    }

    int gpio = 0;
    int check_time_cnt = 0;

    /* SETP #1 : check button pushed */
    do {
        gpio = get_gpio((UINT)btn_gpio_num);

        if (gpio == 1) {
#if defined ( __SUPPORT_EVK_LED__ )
            if (check_time_cnt == 0) {
                LED_OnOff(led_gpio_num, LED_ON); // LED On
            }
#endif    // __SUPPORT_EVK_LED__

            check_time_cnt++;
        } else {
#if defined ( __SUPPORT_EVK_LED__ )
            if (check_time_cnt != 0) {
                LED_OnOff(led_gpio_num, LED_OFF);
            }
#endif    // __SUPPORT_EVK_LED__

            return 0; /* false */
        }

        vTaskDelay(CHECK_STEP_WPS_BTN);

        PRINTF("\33[2K" "WPS after %d seconds\r", check_time - (check_time_cnt / CHECK_STEP_WPS_BTN));

    } while (check_time_cnt < (CHECK_STEP_WPS_BTN * check_time));

    PRINTF("\33[2K" "Ready to use WPS.\r");

    check_time_cnt = 0;

    /* SETP #2 : check button released */
    do {
        gpio = get_gpio((UINT)btn_gpio_num);

        if (gpio == 0) {
#if defined ( __SUPPORT_EVK_LED__ )
            LED_OnOff(led_gpio_num, LED_OFF);
#endif    // __SUPPORT_EVK_LED__
            PRINTF("\33[2K"  "Start WPS.\n");
            result = pdTRUE;
            break;

        } else {   /* sw release */
#if defined ( __SUPPORT_EVK_LED__ )
            /* WPS Status LED Blink */
            if (check_time_cnt % 2 == 0) {
                LED_OnOff(led_gpio_num, LED_OFF);
            } else {
                LED_OnOff(led_gpio_num, LED_ON);
            }
#endif    // __SUPPORT_EVK_LED__
            vTaskDelay(CHECK_TIME_WPS_BTN);

            check_time_cnt++;
        }
    } while (1);

    return result;
}


#ifdef __SUPPORT_ASD__
typedef enum 
{
    ASD_DIABLE,
    ASD_AUTO_ENABLE,
    ASD_ANTENNA_PORT_1,
    ASD_ANTENNA_PORT_2,
    ASD_UNKNOW
} ASD_SELECT;

extern int asd_enable(int ant_type);
extern int asd_set_ant_port(int port);
extern int get_asd_enabled(void);

void cmd_asd(int argc, char *argv[])
{

    if (argc == 1 || strcasecmp(argv[1], "HELP") == 0)
    {
        PRINTF("    Usage : asd [option]\n");
        PRINTF("     option\n");
        PRINTF("\t info : Print ASD info\n");
        PRINTF("\t disable : Disable ASD\n");
        PRINTF("\t enable : Enable Auto ASD\n");
        PRINTF("\t set_ant_p # : Set antenna port 1 or 2 \n");
        return;
    }

    if (strcasecmp(argv[1], "INFO") == 0) {
        switch(get_asd_enabled()) {

            case ASD_DIABLE:
                PRINTF("ASD Disabled \n");  
                break;

            case ASD_AUTO_ENABLE:
                PRINTF("ASD Auto Enabled \n");  
                break;
            case ASD_ANTENNA_PORT_1:
                PRINTF("ASD antenna port 1 \n");  
                break;
            case ASD_ANTENNA_PORT_2:
                PRINTF("ASD antenna port 2 \n");  
                break;
            default:
                PRINTF("[%s] Faild to read ASD flag\n", __func__);
                break;
        }
    } else if (strcasecmp(argv[1], "ENABLE") == 0) {
        if (asd_enable(ASD_AUTO_ENABLE) == 0) { //success
            PRINTF("Successed to ASD auto enable \n");
        } else {
            PRINTF("Fail to ASD auto enable\n");
        }
    } else if (strcasecmp(argv[1], "SET_ANT_P") == 0) {
        if (argc == 2) {
            PRINTF("ASD set_ant_p #\n");
        } else if (argc == 3) {
            int i = atoi(argv[2]);

            if ( i > 2 || i < 1) {
                PRINTF("Invalid argument (1 .. 2)");
                return;
            } else {
                if (asd_set_ant_port(i) == 0) {
                    PRINTF(" Success to ASD set_ant_p %d \n",i);
                }
            }
        }
    } else if (strcasecmp(argv[1], "DISABLE") == 0) {
        if (asd_disable() == 0) {  //success
            PRINTF("ASD disabled.\n");
        } else {
            PRINTF("ASD disable failed.\n");
        }
    }
}
#endif /* __SUPPORT_ASD__ */

int get_current_rssi(int iface)
{
    int rssi = fc80211_get_sta_rssi_value(iface);

    if (chk_supp_connected() == pdPASS) {
        return rssi;
    } else {
        return -1;
    }
}

int get_current_rssi_waiting(int iface)
{
    int    rssi;
    int    retryCount = 0;

read_rssi:
    rssi = get_current_rssi(iface);

    if (rssi == 0) {
        vTaskDelay(3);

        if (++retryCount < 20) {
            goto read_rssi;
        } else {
            PRINTF(RED_COLOR "[%s] ERR: current RSSI is %d(retry:%d) \n" CLEAR_COLOR,
                   __func__, rssi, retryCount);
        }
    } else if (rssi == -1) {
        PRINTF(RED_COLOR "[%s] Not connected! \n" CLEAR_COLOR, __func__);
        retryCount = 0;
    } else {
        PRINTF(YELLOW_COLOR "[%s] current RSSI is %d(retry:%d) \n" CLEAR_COLOR,
               __func__, rssi, retryCount);
        retryCount = 0;
    }

    return rssi;
}

void cmd_rssi(int argc, char *argv[])
{
    int rssi  = -1;

    if (argc == 1 || strcasecmp(argv[1], "HELP") == 0) {
rssi_help:
        PRINTF("    Usage : rssi [option]\n");
        PRINTF("     option\n");
        PRINTF("\t wlan0 : Get RSSI of wlan0 interface\n");
        PRINTF("\t wlan1 : Get RSSI of wlan1 interface\n");
        return;
    }

    if (strcasecmp(argv[1], "WLAN0") == 0) {
        rssi = get_current_rssi(0);
    } else if (strcasecmp(argv[1], "WLAN1") == 0) {
        rssi = get_current_rssi(1);
    } else {
        goto rssi_help;
    }

    if (rssi == -1) {
        PRINTF("Not connected. \n");
    } else {
        PRINTF("Current RSSI for %s : %d\n", argv[1], rssi);
    }
}

#define    CHECK_RSSI_STK_SZ    512
TaskHandle_t    check_rssi_thd = NULL;
char        check_rssi_stk[CHECK_RSSI_STK_SZ];
UINT    check_rssi_period = 1;
UINT    check_rssi_count = 1000;
UINT    check_rssi_run = 0;

static void check_rssi_entry(void *pvParameters)
{
    int sys_wdog_id = -1;
    UINT count = 0;
    int rssi = 0;
    int iface = (int)pvParameters;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    vTaskDelay(check_rssi_period * 100);

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    PRINTF("\n Start check rssi --- period:%d sec, count:%d \n", check_rssi_period, check_rssi_count);

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        ++count;
        rssi = get_current_rssi(iface);
        PRINTF(" %d) RSSI = %d\n", count, rssi);

        vTaskDelay(check_rssi_period * 100);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (count >= check_rssi_count) {
            break;
        }

        if (check_rssi_run != 1) {
            break;
        }
    }

    check_rssi_run = 0;

    da16x_sys_watchdog_unregister(sys_wdog_id);

    PRINTF(" Stop check rssi \n");
    vTaskDelete(check_rssi_thd);
}


static void start_check_rssi(int wlan)
{
    TaskStatus_t xTaskDetails;

    if (check_rssi_run) {
        vTaskGetInfo(check_rssi_thd, &xTaskDetails, pdTRUE, eInvalid);

        if (xTaskDetails.eCurrentState < eDeleted) {
            PRINTF("[%s] check_rssi task is running. \n", __func__);
            return;
        }
    }

    check_rssi_run = 1;

    xTaskCreate(check_rssi_entry,
                "check_rssi",
                CHECK_RSSI_STK_SZ,
                (void *)wlan,
                OS_TASK_PRIORITY_USER+6,
                &check_rssi_thd);
}

void cmd_chk_rssi(int argc, char *argv[])
{
    int arg;

    if (argc == 1 || strcasecmp(argv[1], "HELP") == 0) {
rssi_help:
        PRINTF("    Usage : chk_rssi [option] [period] [count] \n");
        PRINTF("     option\n");
        PRINTF("\t wlan0 : Get RSSI of wlan0 interface\n");
        PRINTF("\t wlan1 : Get RSSI of wlan1 interface\n");
        PRINTF("\t stop  : Stop get RSSI\n");
        return;
    }

    if (argc == 2) {
        if (strcasecmp(argv[1], "WLAN0") == 0) {
            arg = 0;
        } else if (strcasecmp(argv[1], "WLAN1") == 0) {
            arg = 1;
        } else if (strcasecmp(argv[1], "stop") == 0) {
            check_rssi_run = 0;
            return;
        } else {
            goto rssi_help;
        }

        check_rssi_period = 1;
        check_rssi_count = 1000;

        start_check_rssi(arg);
    } else if (argc == 4) {
        if (strcasecmp(argv[1], "WLAN0") == 0) {
            arg = 0;
        } else if (strcasecmp(argv[1], "WLAN1") == 0) {
            arg = 1;
        } else {
            goto rssi_help;
        }

        check_rssi_period = (UINT)atoi(argv[2]);
        check_rssi_count = (UINT)atoi(argv[3]);

        start_check_rssi(arg);
    } else {
        goto rssi_help;
    }
}

void del_thread(TaskHandle_t thread_ptr, char *del_thread_name)
{
    DA16X_UNUSED_ARG(thread_ptr);

    TaskHandle_t task_handle;
    task_handle = xTaskGetHandle(del_thread_name);

    if (task_handle != NULL && eTaskGetState(task_handle) != eDeleted){
        vTaskDelete(task_handle);
    }
}

enum
{
    CA_CERT1 = 0,
    CLIENT_CERT1,
    CLIENT_KEY1,
    DH_PARAM1,
    CA_CERT2,
    CLIENT_CERT2,
    CLIENT_KEY2,
    DH_PARAM2,
    CA_CERT3,
    CLIENT_CERT3,    // For Enterprise(802.1x)
    CLIENT_KEY3,
    DH_PARAM3,
    CA_CERT4,        // For Reserved
    CLIENT_CERT4,
    CLIENT_KEY4,
    DH_PARAM4,
    TLS_CERT_01 = 100,
    TLS_CERT_02,
    TLS_CERT_03,
    TLS_CERT_04,
    TLS_CERT_05,    
    TLS_CERT_06,
    TLS_CERT_07,
    TLS_CERT_08,
    TLS_CERT_09,
    TLS_CERT_10,
    CERT_ALL,
    CERT_END,
};

enum
{
    ACT_NONE = 0,
    ACT_WRITE,
    ACT_READ,
    ACT_DELETE,
    ACT_STATUS
};


/*
certificate read/write/delete/status
*/
int cert_rwds(char action, char dest)
{
    extern int make_message(char *title, char *buf, size_t buflen);

    int status = 0;
    char *buffer = NULL;

    // Status
    if (action == ACT_STATUS || action == ACT_READ) {
#if defined (XIP_CACHE_BOOT)
        buffer = os_calloc(CERT_MAX_LENGTH, sizeof(char));
#else
        buffer = calloc(CERT_MAX_LENGTH, sizeof(char));
#endif
        if (buffer == NULL) {
            PRINTF("[%s] Failed to allocate memory to read certificate\n", __func__);
            return pdFALSE;
        }

        switch (dest) {
            // Cert 1
            case CA_CERT1:
                status = cert_flash_read(SFLASH_ROOT_CA_ADDR1, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_CERT1:
                status = cert_flash_read(SFLASH_CERTIFICATE_ADDR1, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_KEY1:
                status = cert_flash_read(SFLASH_PRIVATE_KEY_ADDR1, buffer, CERT_MAX_LENGTH);
                break;

            case DH_PARAM1:
                status = cert_flash_read(SFLASH_DH_PARAMETER1, buffer, CERT_MAX_LENGTH);
                break;

            // Cert 2
            case CA_CERT2:
                status = cert_flash_read(SFLASH_ROOT_CA_ADDR2, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_CERT2:
                status = cert_flash_read(SFLASH_CERTIFICATE_ADDR2, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_KEY2:
                status = cert_flash_read(SFLASH_PRIVATE_KEY_ADDR2, buffer, CERT_MAX_LENGTH);
                break;

            case DH_PARAM2:
                status = cert_flash_read(SFLASH_DH_PARAMETER2, buffer, CERT_MAX_LENGTH);
                break;

            /* Cert 3 TLS Certificate WPA Enterprise */
            case CA_CERT3:
                status = cert_flash_read(SFLASH_ENTERPRISE_ROOT_CA, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_CERT3:
                status = cert_flash_read(SFLASH_ENTERPRISE_CERTIFICATE, buffer, CERT_MAX_LENGTH);
                break;

            case CLIENT_KEY3:
                status = cert_flash_read(SFLASH_ENTERPRISE_PRIVATE_KEY, buffer, CERT_MAX_LENGTH);
                break;

            case DH_PARAM3:
                status = cert_flash_read(SFLASH_ENTERPRISE_DH_PARAMETER, buffer, CERT_MAX_LENGTH);
                break;
        }

        if ((UCHAR)(buffer[0]) != 0xFF && status == 0) {
            if (action == ACT_READ) {
                PRINTF("\n%s\n", buffer);
            }

            memset(buffer, 0, CERT_MAX_LENGTH);
            vPortFree(buffer);
            buffer = NULL;
            return pdPASS;
        } else {
            if (action == ACT_READ) {
                PRINTF("Empty\n");
            }

            memset(buffer, 0, CERT_MAX_LENGTH);
            vPortFree(buffer);
            buffer = NULL;
            return pdFAIL;
        }
    } else if (action == ACT_WRITE) {
        int cert_len = 0;
        
#if defined (XIP_CACHE_BOOT)
        buffer = os_calloc(FLASH_WRITE_LENGTH, sizeof(char));
#else
        buffer = calloc(FLASH_WRITE_LENGTH, sizeof(char));
#endif // XIP_CACHE_BOOT

        if (buffer == NULL) {
            PRINTF("[%s] Failed to allocate memory to write certificate\n", __func__);
            return pdFALSE;
        }

        cert_len = make_message("Certificate value", buffer, FLASH_WRITE_LENGTH);

        if (cert_len > 0) {
            switch (dest) {
                // Cert 1
                case CA_CERT1:
                    status = cert_flash_write(SFLASH_ROOT_CA_ADDR1, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_CERT1:
                    status = cert_flash_write(SFLASH_CERTIFICATE_ADDR1, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_KEY1:
                    status = cert_flash_write(SFLASH_PRIVATE_KEY_ADDR1, buffer, FLASH_WRITE_LENGTH);
                    break;
                case DH_PARAM1:
                    status = cert_flash_write(SFLASH_DH_PARAMETER1, buffer, FLASH_WRITE_LENGTH);
                    break;

                // Cert 2
                case CA_CERT2:
                    status = cert_flash_write(SFLASH_ROOT_CA_ADDR2, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_CERT2:
                    status = cert_flash_write(SFLASH_CERTIFICATE_ADDR2, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_KEY2:
                    status = cert_flash_write(SFLASH_PRIVATE_KEY_ADDR2, buffer, FLASH_WRITE_LENGTH);
                    break;
                case DH_PARAM2:
                    status = cert_flash_write(SFLASH_DH_PARAMETER2, buffer, FLASH_WRITE_LENGTH);
                    break;

                /* Cert 3 TLS Certificate WPA Enterprise */
                case CA_CERT3:
                    status = cert_flash_write(SFLASH_ENTERPRISE_ROOT_CA, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_CERT3:
                    status = cert_flash_write(SFLASH_ENTERPRISE_CERTIFICATE, buffer, FLASH_WRITE_LENGTH);
                    break;
                case CLIENT_KEY3:
                    status = cert_flash_write(SFLASH_ENTERPRISE_PRIVATE_KEY, buffer, FLASH_WRITE_LENGTH);
                    break;
                case DH_PARAM3:
                    status = cert_flash_write(SFLASH_ENTERPRISE_DH_PARAMETER, buffer, FLASH_WRITE_LENGTH);
                    break;
            }
        } else {
            PRINTF("\n\nCanceled\n");
            status = -1;
        }

        if (buffer) {
            vPortFree(buffer);
        }
        
        buffer = NULL;

        if (status == 0) {
            return pdPASS;
        }

        return pdFAIL;
    } else if (action == ACT_DELETE) {

        switch (dest) {
            // Cert 1
            case CA_CERT1:
                status = cert_flash_delete(SFLASH_ROOT_CA_ADDR1);
                break;

            case CLIENT_CERT1:
                status = cert_flash_delete(SFLASH_CERTIFICATE_ADDR1);
                break;

            case CLIENT_KEY1:
                status = cert_flash_delete(SFLASH_PRIVATE_KEY_ADDR1);
                break;

            case DH_PARAM1:
                status = cert_flash_delete(SFLASH_DH_PARAMETER1);
                break;

            // Cert 2
            case CA_CERT2:
                status = cert_flash_delete(SFLASH_ROOT_CA_ADDR2);
                break;

            case CLIENT_CERT2:
                status = cert_flash_delete(SFLASH_CERTIFICATE_ADDR2);
                break;

            case CLIENT_KEY2:
                status = cert_flash_delete(SFLASH_PRIVATE_KEY_ADDR2);
                break;

            case DH_PARAM2:
                status = cert_flash_delete(SFLASH_DH_PARAMETER2);
                break;

            // Cert 3
            case CA_CERT3:
                status = cert_flash_delete(SFLASH_ENTERPRISE_ROOT_CA);
                break;
                
            case CLIENT_CERT3:
                status = cert_flash_delete(SFLASH_ENTERPRISE_CERTIFICATE);
                /* for EAP-FAST */
                da16x_unsetenv(ENVIRON_DEVICE, "app", "fast_pac");
                da16x_unsetenv(ENVIRON_DEVICE, "app", "fast_pac_len");
                break;

            case CLIENT_KEY3:
                status = cert_flash_delete(SFLASH_ENTERPRISE_PRIVATE_KEY);
                break;

            case DH_PARAM3:
                status = cert_flash_delete(SFLASH_ENTERPRISE_DH_PARAMETER);
                break;

            // Delete All
            case CERT_ALL:
                /* for EAP-FAST */
                da16x_unsetenv(ENVIRON_DEVICE, "app", "fast_pac");
                da16x_unsetenv(ENVIRON_DEVICE, "app", "fast_pac_len");

                status = cert_flash_delete_all();
                break;
        }

        if (status == 0) {
            return pdPASS;
        } else {
            return pdFAIL;
        }
    }

    return pdFAIL;
}

void cmd_certificate(int argc, char *argv[])
{
    int dest = CERT_END;
    char action = ACT_NONE;
    int status = 0;

#if defined (__SUPPORT_ATCMD_TLS__)
    atcmd_cm_cert_t *atcmd_cert = NULL;
    int atcmd_cert_idx = 0;
#endif // __SUPPORT_ATCMD_TLS__

    // Help
    if (argc == 1 || argc > 3 || strcasecmp(argv[1], "HELP") == 0) {
help:
        PRINTF("\ncert [action] [dest]\n\n");

        PRINTF("  action=[status|read|write|del|enable_verify_ent|disable_verify_ent|status_verify_ent|help]\n");
        PRINTF("  dest  =[ca#|cert#|key#|dh#|all]\n");
        
        PRINTF("\n\t[action]\n" \
                "\tstatus\tCertificate Status for <dest>\n" \
                "\tread\tCertificate Read\n" \
                "\twrite\tCertificate Write\n" \
                "\tdel\tCertificate Delete\n");
        PRINTF("\tenable_verify_ent\tEnable enterprise validation flag (stateless)\n");
        PRINTF("\tdisable_verify_ent\tDisable enterprise validation flag (stateless)\n");
        PRINTF("\tstatus_verify_ent\tEnterprise validation flag status\n");

        PRINTF("\thelp\tThis Message\n\n");

        PRINTF("\t[dest]\n" \
                "\tca#\tRoot CA (#1~3)\n" \
                "\tcert#\tServer/Client Certificate (#1~3)\n" \
                "\tkey#\tPrivate Key (#1~3)\n" \
                "\tdh#\tDH Parameter (#1~3)\n");

        PRINTF("\tall\tAll(Certificate #1,2,3); status/del only\n");
        PRINTF("\t#\t1: MQTT, CoAPs Client / 2: HTTPs, OTA / 3: Enterprise (802.1x)\n\n");

        PRINTF("\tverify value (For Enterprise certificate Only)\n" \
              "\t  0x1   Validation expiration time check\n" \
              "\t  0x200 Validation start time check\n\n");

        return;
    }

    if (argc == 2 && strcasecmp(argv[1], "STATUS_VERIFY_ENT") == 0) {
        PRINTF("0x%0x\n" \
          "  0x1   Validation Expiration Time Check\n" \
          "  0x200 Validation start time check item\n", get_ent_cert_verify_flags());
        return;
    } else if (argc == 3 && (strstr(argv[1], "VERIFY_ENT") != 0 || strstr(argv[1], "verify_ent") != 0)) {
        // ENABLE_VERIFY_ENT, DISABLE_VERIFY_ENT
        char flag = pdTRUE;

        dest = (int)htoi(argv[2]);

        if (!(dest == 0x1 || dest == 0x200 || dest == 0x201)) {
            goto help;
        }

        if (strcasecmp(argv[1], "DISABLE_VERIFY_ENT") == 0) {
            flag = pdFALSE;        
        }

        set_ent_cert_verify_flags(flag, dest);
        PRINTF("0x%0x\n", get_ent_cert_verify_flags());
        return;
    }

    // Action
    if (strcasecmp(argv[1], "DEL") == 0) {
        action = ACT_DELETE;
    } else if (strcasecmp(argv[1], "STATUS") == 0) {
        action = ACT_STATUS;
        dest = CERT_ALL;
    } else if (strcasecmp(argv[1], "WRITE") == 0) {
        action = ACT_WRITE;
    } else if (strcasecmp(argv[1], "READ") == 0) {
        action = ACT_READ;
    }

    // Dest
    // cert1
    if (strcasecmp(argv[2], "CA1") == 0) {
        dest = CA_CERT1;
    } else if (strcasecmp(argv[2], "CERT1") == 0) {
        dest = CLIENT_CERT1;
    } else if (strcasecmp(argv[2], "KEY1") == 0) {
        dest = CLIENT_KEY1;
    } else if (strcasecmp(argv[2], "DH1") == 0) {
        dest = DH_PARAM1;
    }
    // cert2
    else if (strcasecmp(argv[2], "CA2") == 0) {
        dest = CA_CERT2;
    } else if (strcasecmp(argv[2], "CERT2") == 0) {
        dest = CLIENT_CERT2;
    } else if (strcasecmp(argv[2], "KEY2") == 0) {
        dest = CLIENT_KEY2;
    } else if (strcasecmp(argv[2], "DH2") == 0) {
        dest = DH_PARAM2;
    }
    // cert3 Enterprise(802.1x)
    else if (strcasecmp(argv[2], "CA3") == 0) {
        dest = CA_CERT3;
    } else if (strcasecmp(argv[2], "CERT3") == 0) {
        dest = CLIENT_CERT3;
    } else if (strcasecmp(argv[2], "KEY3") == 0) {
        dest = CLIENT_KEY3;
    } else if (strcasecmp(argv[2], "DH3") == 0) {
        dest = DH_PARAM3;
    } else if (strcasecmp(argv[2], "ALL") == 0) {
        dest = CERT_ALL;
    } else if (action != ACT_STATUS && dest != CERT_ALL) {
        goto help;
    }

    // status
    if (dest == CERT_ALL && action == ACT_STATUS) {
        da16x_cert_display_certs();

        PRINTF("\n#1:\n  For MQTT, CoAPs Client\n");
        PRINTF("  - Root CA     : %s\n", cert_rwds(ACT_STATUS, CA_CERT1) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Certificate : %s\n", cert_rwds(ACT_STATUS, CLIENT_CERT1) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Private Key : %s\n", cert_rwds(ACT_STATUS, CLIENT_KEY1) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - DH Parameter: %s\n", cert_rwds(ACT_STATUS, DH_PARAM1) == pdTRUE ? "Found" : "Empty");

        PRINTF("\n#2:\n  For HTTPs, OTA\n");
        PRINTF("  - Root CA     : %s\n", cert_rwds(ACT_STATUS, CA_CERT2) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Certificate : %s\n", cert_rwds(ACT_STATUS, CLIENT_CERT2) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Private Key : %s\n", cert_rwds(ACT_STATUS, CLIENT_KEY2) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - DH Parameter: %s\n", cert_rwds(ACT_STATUS, DH_PARAM2) == pdTRUE ? "Found" : "Empty");

        PRINTF("\n#3:\n  For Enterprise (802.1x)\n");
        PRINTF("  - Root CA     : %s\n", cert_rwds(ACT_STATUS, CA_CERT3) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Certificate : %s\n", cert_rwds(ACT_STATUS, CLIENT_CERT3) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - Private Key : %s\n", cert_rwds(ACT_STATUS, CLIENT_KEY3) == pdTRUE ? "Found" : "Empty");
        PRINTF("  - DH Parameter: %s\n", cert_rwds(ACT_STATUS, DH_PARAM3) == pdTRUE ? "Found" : "Empty");

#if 0 // Reserved - For User requirement
        PRINTF("\n#4:\n  For Reserved\n");
        PRINTF("  - Root CA     : %sound\n", status = cert_rwds(ACT_STATUS, CA_CERT4) == pdTRUE ? "F" : "Not f");
        PRINTF("  - Certificate : %sound\n", status = cert_rwds(ACT_STATUS, CLIENT_CERT4) == pdTRUE ? "F" : "Not f");
        PRINTF("  - Private Key : %sound\n", status = cert_rwds(ACT_STATUS, CLIENT_KEY4) == pdTRUE ? "F" : "Not f");
        PRINTF("  - DH Parameter: %sound\n", status = cert_rwds(ACT_STATUS, DH_PARAM4) == pdTRUE ? "F" : "Not f");
#endif // Reserved

#if defined (__SUPPORT_ATCMD_TLS__)
        PRINTF("\nTLS_CERT for ATCMD\n");

        {
            atcmd_cert = pvPortMalloc(sizeof(atcmd_cm_cert_t));
            if (atcmd_cert == NULL) {
                PRINTF("Failed to allocate memory to get certificates(%ld)\n",
                       sizeof(atcmd_cm_cert_t));
            } else {
                for (atcmd_cert_idx = 0 ; atcmd_cert_idx < ATCMD_CM_MAX_CERT_NUM ; atcmd_cert_idx++) {
                    memset(atcmd_cert, 0x00, sizeof(atcmd_cm_cert_t));

                    status = atcmd_cm_read_cert_by_idx((unsigned int)atcmd_cert_idx, atcmd_cert);
                    if (status) {
                        PRINTF("Failed to read certificate(%d,0x%x)\n", atcmd_cert_idx, status);
                        continue;
                    }

                    if ((atcmd_cert->info.flag == ATCMD_CM_INIT_FLAG)
                            && (strlen(atcmd_cert->info.name) > 0)
                            && (atcmd_cert->info.cert_len > 0)) {
                        PRINTF("  - TLS_CERT_%02d : %s(%s/%d/%d/%d/%d)\n",
                               atcmd_cert_idx + 1, "Found",
                               atcmd_cert->info.name, atcmd_cert->info.type,
                               atcmd_cert->info.seq, atcmd_cert->info.format,
                               atcmd_cert->info.cert_len);
                    } else {
                        PRINTF("  - TLS_CERT_%02d : %s\n", atcmd_cert_idx + 1, "Empty");
                    }
                }

                vPortFree(atcmd_cert);
                atcmd_cert = NULL;
            }
        }
        PRINTF("\n");
#endif // __SUPPORT_ATCMD_TLS__

        return;
    } else if (dest == CERT_ALL && action != ACT_STATUS && action != ACT_DELETE) {
        goto help;
    }

    if (action == ACT_READ) {
        PRINTF("  - %-10s : ", argv[2]);
    }
    
    status = cert_rwds(action, (char)dest);

    switch (action) {
        case ACT_STATUS:
            PRINTF("  - %-10s : %s\n\n", argv[2], status == pdTRUE ? "Found" : "Empty");
            break;

        case ACT_WRITE:
            PRINTF("\n%s Write %s.\n", argv[2], (status == pdPASS) ? "success":"failed");
            break;

        case ACT_DELETE:
            PRINTF("\n%s Delete %s.\n", argv[2], (status == pdPASS) ? "success":"failed");

            break;
    }

    return;
}

void cmd_debug_ch_tx_level(int argc, char *argv[])
{
    UINT    status = 0;

    if (argc == 15) {
        char tmp_str[15];

        memset(tmp_str, 0, 15);

        for (int idx = 1; idx <= 14; idx++) {
            if (!isxdigit((int)(argv[idx][0])) || atoi(argv[idx]) > 0xF) {
                goto error;
            }

            strcat(tmp_str, argv[idx]);
        }

        status = set_debug_ch_tx_level(tmp_str);

        if (status == E_WRITE_OK) {
            PRINTF("set %s\n", tmp_str);
        }
    } else if (argc == 2 && strncasecmp(argv[1], "ERASE", 5) == 0) {   /* Erase NVRAM */
        set_debug_ch_tx_level("ERASE");
    } else if (argc == 2 && strncasecmp(argv[1], "PRINT", 5) == 0) {   /* Print tx Level */
        print_tx_level();
    } else {
        goto error;
    }

    return;

error:
    {
        PRINTF("\n\tUsage : txpwr_l [print]|[erase]|[CH1 CH2 ... CH13 CH14]\n");
        PRINTF("\t\terase : default\n");
        PRINTF("\t\tCH  0~f\n");
        PRINTF("\t\tex) txpwr_l erase\n");
        PRINTF("\t\tex) txpwr_l 2 3 3 3 3 3 3 3 3 3 1 f f f\n");
    }
}

#define PRODUCT_INFO_RL_RF9050        0x9050
#define PRODUCT_INFO_RL_RF9060        0x9060
void cmd_txpwr_ctl_same_idx_table(int argc, char *argv[])
{
    int pwr_idx, i;
    int if_idx;
    uint16_t product_info = get_product_info();

    if (argc < 2 || argc > 3) {
        goto tpc_print_help;
    } else if (argc == 2 && (strcasecmp(argv[1], "HELP") == 0)) {
        goto tpc_print_help;
    }

    if (getSysMode() == 0) {
        if_idx = 0;    //STA mode
    } else {
        if_idx = 1;    //Soft AP/P2P/Concurrent/P2P GO fixed
    }

    if ( product_info == PRODUCT_INFO_RL_RF9050) {
        PRINTF("Currently this command is only for FC9060 chip.\n", __func__);
        return;

    } else if ( product_info == PRODUCT_INFO_RL_RF9060) {
        if (argc == 2 && strcasecmp(argv[1], "get") == 0 ) {
            pwr_idx = (int) region_power_level[0];
            PRINTF("Current Tx Power Grade Index is (%d).\n", pwr_idx);
            return;

        } else if (   ( argc == 3 && strcasecmp(argv[1], "set") == 0 )
                   && ( (int)ctoi(argv[2]) >= 0 && (int)ctoi(argv[2]) < 14) ) {
            write_nvram_int((const char *)NVR_KEY_PWR_CTRL_SAME_IDX_TBL, (int)ctoi(argv[2]));

            for (i = 0; i < NUM_CHANNEL_PWR; i++) {
                region_power_level[i] = (UCHAR)ctoi(argv[2]);
            }

            //set requested power grade idx.
            fc80211_set_tx_power_table(if_idx, (unsigned int *)&region_power_level, (unsigned int *)&region_power_level);
            PRINTF("Configured Tx power grade index value is (%d)\n", ctoi(argv[2]));
            return;
        }

    }

tpc_print_help:
    PRINTF("\n"
           "Usage:"
           "\ttxpwr-ctl [set|get] [0|1|2]\n"
           "\n");
    return;
}

void wifi_service_stop_flush(void)
{
    set_debug_dhcpc(0); // dhcp client debug level

    /* stop services */
    if (is_supplicant_done()) {
        PRINTF("<<Stop WiFi Service>>\n");
        da16x_cli_reply("flush", NULL, NULL);
    } else {
        PRINTF("<<Already WiFi stopped>>\n");
    }
}

void cmd_defap(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    da16x_set_default_ap_config();
    reboot_func(SYS_REBOOT);
}


#if defined (__SUPPORT_SIGMA_TEST__)
#include "mac80211.h"

enum ieee80211_conf_flags {
    IEEE80211_CONF_MONITOR      = (1<<0),
    IEEE80211_CONF_PS           = (1<<1),
    IEEE80211_CONF_IDLE         = (1<<2),
    IEEE80211_CONF_OFFCHANNEL   = (1<<3),
};

enum ieee80211_conf_changed {
    IEEE80211_CONF_CHANGE_SMPS              = BIT(1),
    IEEE80211_CONF_CHANGE_LISTEN_INTERVAL   = BIT(2),
    IEEE80211_CONF_CHANGE_MONITOR           = BIT(3),
    IEEE80211_CONF_CHANGE_PS                = BIT(4),
    IEEE80211_CONF_CHANGE_POWER             = BIT(5),
    IEEE80211_CONF_CHANGE_CHANNEL           = BIT(6),
    IEEE80211_CONF_CHANGE_RETRY_LIMITS      = BIT(7),
    IEEE80211_CONF_CHANGE_IDLE              = BIT(8),
    IEEE80211_CONF_CHANGE_DPM_BOOT          = BIT(9),
    IEEE80211_CONF_CHANGE_P2P_GO_PS         = BIT(10),
};

/* U-APSD queues for WMM IEs sent by STA */
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VO  (1<<0)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_VI  (1<<1)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BK  (1<<2)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_BE  (1<<3)
#define IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK    0x0f

extern struct i3ed11_hw *hw;  /*  move to rwnx_main.c */
extern struct i3ed11_ops rwnx_ops;

extern void set_fci_dpsm_ps_mode(bool mode, bool use_ps_poll);
extern void ps_mode_from_supplicant(int mode);

void cmd_lmac_ps_mode(int argc, char *argv[])
{
    if (argc == 2) {
        int mode = atoi(argv[1]);

        set_fci_dpsm_ps_mode(false, 0);

        if (mode == 0) {    /* MM_PS_MODE_OFF */
            ps_mode_from_supplicant(0);
            PRINTF("PS MODE OFF\n");
        } else if (mode == 1) { /* MM_PS_MODE_ON */
            ps_mode_from_supplicant(1);
            PRINTF("PS MODE ON\n");
        } else if (mode == 2) { /* MM_PS_MODE_ON_DYN */
            ps_mode_from_supplicant(2);
            PRINTF("DYN PS MODE ON\n");
        }
        rwnx_ops.config(hw, IEEE80211_CONF_CHANGE_PS);
    }
}

void cmd_lmac_wmm_queue_mode(int argc, char *argv[])
{
    struct rwnx_hw *rwnx_hw = hw->priv;

    if (rwnx_hw != NULL) {
        if (argc == 2) {
            int mode = atoi(argv[1]);

            if (mode == 0) {
                hw->uapsd_queues = IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK;
                PRINTF("WMM-PS MODE ALL\n");
            } else if (mode == 1) {
                hw->uapsd_queues = (IEEE80211_WMM_IE_STA_QOSINFO_AC_VO&IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK);
                PRINTF("WMM-PS MODE VO\n");
            } else if (mode == 2) {
                hw->uapsd_queues = (IEEE80211_WMM_IE_STA_QOSINFO_AC_VI&IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK);
                PRINTF("WMM-PS MODE VI\n");
            } else if (mode == 3) {
                hw->uapsd_queues = (IEEE80211_WMM_IE_STA_QOSINFO_AC_BK&IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK);
                PRINTF("WMM-PS MODE BK\n");
            } else if (mode == 4) {
                hw->uapsd_queues = (IEEE80211_WMM_IE_STA_QOSINFO_AC_BE&IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK);
                PRINTF("WMM-PS MODE BE\n");
            } else if (mode == 5) {
                hw->uapsd_queues = (0x03&IEEE80211_WMM_IE_STA_QOSINFO_AC_MASK);
                PRINTF("WMM-PS MODE VO/VI\n");
                        } else if (mode == 6) {
                hw->uapsd_queues = (0x00);
                PRINTF("WMM-PS MODE NO\n");
            }
            rwnx_ops.config(hw, IEEE80211_CONF_CHANGE_PS);
        } else {

            PRINTF("\nUsage:\n"
                "\twmm_ps [wmm_ps mode]\n"
                "\t0: WMM-PS MODE All Queue.\n"
                "\t1: WMM-PS MODE VO Queue.\n"
                "\t2: WMM-PS MODE VI Queue.\n"
                "\t3: WMM-PS MODE BK Queue.\n"
                "\t4: WMM-PS MODE BE Queue.\n"
                "\t5: WMM-PS MODE VO/VI Queue.\n"
                "\t6: WMM-PS MODE No Queue.\n");
            return;
        }
    } else {
        PRINTF("rwnx_hw is null\n");
    }
}

u8 temp_queue;
void cmd_lmac_wmm_queue_mode2(int argc, char *argv[])
{
    struct rwnx_hw *rwnx_hw = hw->priv;

    if (rwnx_hw != NULL) {
        if (argc == 5) {
            int mode = atoi(argv[1]);

            if (mode == 0) {
                hw->uapsd_queues &= ~IEEE80211_WMM_IE_STA_QOSINFO_AC_BE;
                PRINTF("WMM-PS MODE BE OFF\n");
            } else{
                hw->uapsd_queues |= IEEE80211_WMM_IE_STA_QOSINFO_AC_BE;
                PRINTF("WMM-PS MODE BE ON\n");
            }

            mode = atoi(argv[2]);

            if (mode == 0) {
                hw->uapsd_queues &= ~IEEE80211_WMM_IE_STA_QOSINFO_AC_BK;
                PRINTF("WMM-PS MODE BK OFF\n");
            } else{
                hw->uapsd_queues |= IEEE80211_WMM_IE_STA_QOSINFO_AC_BK;
                PRINTF("WMM-PS MODE BK ON\n");
            }

            mode = atoi(argv[3]);

            if (mode == 0) {
                hw->uapsd_queues &= ~IEEE80211_WMM_IE_STA_QOSINFO_AC_VO;
                PRINTF("WMM-PS MODE VO OFF\n");
            } else{
                hw->uapsd_queues |= IEEE80211_WMM_IE_STA_QOSINFO_AC_VO;
                PRINTF("WMM-PS MODE VO ON\n");
            }

            mode = atoi(argv[4]);

            if (mode == 0) {
                hw->uapsd_queues &= ~IEEE80211_WMM_IE_STA_QOSINFO_AC_VI;
                PRINTF("WMM-PS MODE VI OFF\n");
            } else {
                hw->uapsd_queues |= IEEE80211_WMM_IE_STA_QOSINFO_AC_VI;
                PRINTF("WMM-PS MODE VI ON\n");
            }

            temp_queue =     hw->uapsd_queues;
            rwnx_ops.config(hw, IEEE80211_CONF_CHANGE_PS);
        } else {

            PRINTF("\nUsage:\n"
                "\twmm_ps_mode [BE] [BK] [VO] [VI]\n"
                "\t0: OFF, 1:ON\n"
                "\tex) wmm_ps 0 0 1 1 \n"
                "\tex)   - BE:OFF, BK:OFF, VO:ON, VI:ON\n");
            return;
        }
    } else {
        PRINTF("rwnx_hw is null\n");
    }
}
#endif // (__SUPPORT_SIGMA_TEST__)

/* EOF */
