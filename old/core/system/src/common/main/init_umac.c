/**
 ****************************************************************************************
 *
 * @file init_umac.c
 *
 * @brief UMAC Initializer
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

#include "da16x_types.h"
#include "command.h"
#include "common_def.h"
#include "command_umac.h"

#include "iface_defs.h"

#include "target.h"
#include "sys_feature.h"
#include "da16x_system.h"
#include "rtc.h"


#ifdef    XIP_CACHE_BOOT
#include "dhcpserver.h"
#include "lwip/netif.h"
#include "sntp.h"
#endif    /*XIP_CACHE_BOOT*/
#include "da16x_dpm.h"
#include "user_dpm_api.h"

#include "supp_def.h"        // For feature __LIGHT_SUPPLICANT__

#ifndef    __LIGHT_SUPPLICANT__
  #ifdef __SUPPORT_P2P__
    #define P2P_GROUP_ADD    "p2p_group_add"
    #define P2P_FIND        "p2p_find"
  #endif /* __SUPPORT_P2P__ */
#endif    /* __LIGHT_SUPPLICANT__ */
#define RUN_DELAY    100

#define NUM_CHANNEL_PWR  14

#undef DPM_DEBUG_INFO

extern unsigned char fast_connection_sleep_flag;
extern void fc80211_set_ampdu_flag(int mode, int val);
extern void dpm_lmac_start(void);
extern void init_dpm_sleep_daemon(void);
extern void dpm_reg_appl(void);
extern bool is_dpm_supplicant_done(void);
extern void fc80211_connection_loss(void);

/* Do not use cmd_lmac_tx_init !!! */
#ifdef    __SUPPORT_IPV6__
extern void ipv6_auto_init(UINT intf_select, int mode);
#endif    /* __SUPPORT_IPV6__ */

extern long get_time_zone(void);

extern void dpm_ops_bss_infos_changed();
extern void umac_dpm_func_init(void);
extern int  dpm_ops_tim_status_handler();
extern unsigned int dpm_ops_get_cur_bcn_cnt();
extern bool dpm_ops_tim_rx_handler();
extern int  dpm_get_wakeup_source();

extern UINT checkRTC_OverflowCreate(void);
extern UCHAR get_last_dpm_sleep_type(void);

#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )
extern int  chk_factory_rst_button_press_status(void);
#endif // __SUPPORT_FACTORY_RESET_BTN__
extern int  get_run_mode(void);
extern void fc80211_dpm_info_clear(void);
extern int  fc80211_set_tx_power_table(int ifindex, unsigned int *one_regcode_start_addr, unsigned int *one_regcode_start_addr_dsss);
extern uint16_t get_product_info();
extern void dpm_lmac_set_tim_wakeup_abnormal(void);

extern unsigned int netInit(void);

extern void init_dpm_environment(void);
extern UINT init_region_tx_level(void);

extern int  start_da16x_supp(void);
extern int  start_wifi_monitor(void);
extern int  umac_init_func(bool dpmbooting);

extern long __timezone;
extern struct i3ed11_hw *hw;
extern UCHAR sntp_support_flag;

extern EventGroupHandle_t    da16x_sp_event_group;
extern bool lmac_init_done;
extern UCHAR region_power_level[NUM_CHANNEL_PWR];
extern UCHAR    region_power_level_dsss[NUM_CHANNEL_PWR];
extern UCHAR    runtime_cal_flag;
extern unsigned char    ble_combo_ref_flag;

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
/* DDPS Related Primitive & variables */
extern void dpm_tim_bufp_chk_en();
extern char get_dpm_tim_ddps_macflag(void);
extern void set_dpm_tim_ddps_macflag(char);
extern int get_dpm_DynamicPeriodSet_config();
extern UCHAR    dpm_dynamic_period_setting_flag;
#endif

extern unsigned int dpm_timer_task_create(void);
extern void set_sntp_period_to_rtm(long period);
extern long get_timezone_from_rtm(void);
extern void init_dpm_environment(void);

extern void printf_with_run_time(char * string);
extern unsigned char umac_dpm_check_init(void);
extern void umac_dpm_init_done(void);

extern UINT get_wlaninit(void);

extern void set_timezone_to_rtm(long timezone);
extern void wifi_netif_control(int, int);

void set_tx_power_table(int inf);
void disconnect_from_peers(int flag);



/** During DPM Wakeup, Initialize processing is like below.
mac80211 Init() / UMAC LMAC Init --> ops start(dpm_start_req_handler) --> bss_info_changed(lmac connection status) -->
umac_dpm_init(umac connection status) --> Network stack create & start --> Supplicant create & start --> tim status_handler()
**/
static UINT dpm_full_wakeup_wlaninit(bool abnormal)
{
    enum DPM_WAKEUP_TYPE     wakeuptype = DPM_UNKNOWN_WAKEUP;
    bool reconnect_try = abnormal;

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("Start wakeup wlaninit");
#endif

    init_dpm_environment();

    /* In case of abnormal wakeup, set the tim status as undefined in tim status  */
    if (abnormal) {
        //Printf("Abnormal wakeup\n");
        dpm_lmac_set_tim_wakeup_abnormal();
    }

    extern void da16x_SetTzoff(long offset);
    da16x_SetTzoff(get_timezone_from_rtm());
    
    /* Init the resource of power daemon */
    init_dpm_sleep_daemon();

    if (umac_init_func(true) == pdFALSE) {
        PRINTF("\nUMAC init. failed\n");
        return pdFALSE;
    }

    set_tx_power_table(0);

    /* set handler function of LMAC */
    dpm_ops_bss_infos_changed();

    /* set wifi as connection */
    umac_dpm_func_init();

    /* Init the resource of power daemon */
    /* already created event and mutex in start_dpm_sleep_daemon() run */
#if 0
    init_dpm_sleep_daemon();
#endif

    /** 1. First of all, wait until LMAC Initial is done.
     * 2. MAC IDLE Mode --> Active Mode
     * 3. do start receiving packet
     */
    while( lmac_init_done == 0)
        vTaskDelay(1);

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("Start lmac");
#endif

    dpm_lmac_start();
    
    /* network stack(lwip) should be initialized with DPM */
    /* Init network resource */
    netInit();
    
    /* Start Wifi monitor thread to receive events from Supplicant */
    start_wifi_monitor();

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("Start supplicant");
#endif

    /* Start supplicant */
    start_da16x_supp();

    /* !!! Caution !!!
     *    Don't run this function before netInit().
     */
    wakeuptype = (enum DPM_WAKEUP_TYPE)dpm_ops_tim_status_handler();

#if 0    /* we can remove this in FREERTOS DPM */
    /* we have to change the rf adc status for transfering something */
    set_dpm_rf_cntrl_mode();
#endif

    /* Case of UC, MB/BC, NO-BCN, let's do have enough time to process by Application */
    /* Don't check the wakeup condition ... */
    {
        /* Network interface UP in DPM */
        wifi_netif_control(WLAN0_IFACE, 1);

        /* In case of DPM Full booting, Need link up connection as true */
        if (umac_dpm_check_init()) {
            umac_dpm_init_done();
        }

        /* User DPM module should be registered as ASAP */
#ifdef __CHECK_ABNORMAL_WAKEUP__
        if (!(wakeuptype == DPM_NOACK_WAKEUP))
#endif    /* __CHECK_ABNORMAL_WAKEUP__ */
            dpm_reg_appl();

        /* TIM Status is UC, BC, BCN_CHG, DPM_USER_0, DPM_USER_1 , let's wait supplicant ready(key set, eloop run.) */
        if (wakeuptype == DPM_PACKET_WAKEUP || wakeuptype == DPM_USER_WAKEUP) {
            while(1) {
                if (is_dpm_supplicant_done())
                    break;
                //OAL_MSLEEP(1);
                vTaskDelay(1);
            }

            // In case DPM_USER_0, DPM_USER_1, Beacon Packet of TIM Processing Start
            // Other(UC, BC/MC) case , DPM sleep daemon would be start the RX Packet
            //if (tim_status == 2)
            dpm_ops_tim_rx_handler();
        } else if (wakeuptype == DPM_NOACK_WAKEUP) {    /* In case of No ACK, No BCN */
            unsigned char rx_bcn_check_count = 0;
            #define MAX_NO_ACK_BCN_CNT_SLEEP    10
            /* We have to process the connection loss by Null keepavlie */
            /* So we should wait enoughly */
            /* Normally, maximum DTIM Count of APs  is 3 , for sync so le't do wait 300msec */
            while(1) {
                /* If currently bcn is received, break */
                if (dpm_ops_get_cur_bcn_cnt() > 0)
                    break;
                /* total 100ms sleep */
                else if (rx_bcn_check_count++ > MAX_NO_ACK_BCN_CNT_SLEEP )
                    break;
                else
                    vTaskDelay(10);
            }
        } else if (wakeuptype == DPM_DEAUTH_WAKEUP) {     /* TIM status is Deauth */
            /* It means, TIM received the Deauth packet */
            /* So need reconnect */
            //OAL_MSLEEP(10);    /* Sleep for Supplicant initialize */
            while(1) {
                if (is_dpm_supplicant_done()) break;
                vTaskDelay(1);
            }
            /* For Fast reconnect processing */
            if (dpm_ops_tim_rx_handler() == false) {
                /* If Deauth/Deassoc mgmt packet is not exist, run the connection lost event */
                reconnect_try = true;
            }
        } else if (wakeuptype == DPM_TIM_ERR_WAKEUP) {     /* TIM status is NO DPM */
            /* It means, TIM has something wrong and fault */
            /* So need reboot */
            reboot_func(SYS_REBOOT);
        }

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__        
        else if (wakeuptype == DPM_DDPS_BUFP_WAKEUP) {    /* TIM status is BUFP */
            extern void dpm_set_tim_bufp_chk_param();
            if (dpm_dynamic_period_setting_flag)
                dpm_set_tim_bufp_chk_param();
        }
#endif
    }

    /* abnormal dpm full booting --> connection lost --> reconnect */
    if (reconnect_try) {
        fc80211_connection_loss();
    }

    checkRTC_OverflowCreate();

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("Start sleep daemon");
#endif

    /* Sleep daemon start */
    start_dpm_sleep_daemon((int)wakeuptype);

    if (sntp_support_flag == pdTRUE) {
        sntp_cmd_param * sntp_param = pvPortMalloc(sizeof(sntp_cmd_param));
        sntp_param->cmd = SNTP_START;
        sntp_run(sntp_param);
    }

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("End wakeup wlaninit");
#endif

    return ERR_OK;
}


UINT wlaninit(void)
{
    int BootScenario;
    int abnormal_dpm_boot = 0;

    extern void printSysMode(UINT mode);

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("Start wlaninit");
#else
    if (runtime_cal_flag == pdTRUE) {
        printf_with_run_time("Start wlaninit");
    }
#endif

    /* Booting Scenario and Checkin */
    BootScenario = dpm_get_wakeup_source();

    if (!(   BootScenario == WAKEUP_SOURCE_EXT_SIGNAL    // External wakeup - None DPM
          || BootScenario == WAKEUP_SOURCE_WAKEUP_COUNTER    // Counter wakeup
          || BootScenario == WAKEUP_SOURCE_POR    // POR
          || BootScenario == WAKEUP_WATCHDOG    // Reboot with retention
          || BootScenario == WAKEUP_RESET    /* reboot without retention */)) {    // cmd "reboot"
        /* Check RF-Test mode */
        if (get_wlaninit() == pdFALSE) {
            // Cannot support DPM in RF-Test mode.
            write_nvram_int("dpm_mode", 0);

#ifdef BUILD_OPT_DA16200_ASIC
            extern void dpm_power_up_step4_patch(unsigned long prep_bit);
            dpm_power_up_step4_patch(0xf);
#endif
            PRINTF("Fail to initialize WLAN. (step 0)\n!!! TEST MODE !!!\n");
            return pdFALSE;
        }

        /* Case of Wakeup Reset, clear the umac connection info */
        if ((BootScenario == WAKEUP_RESET_WITH_RETENTION) && get_dpm_mode()) {
            fc80211_dpm_info_clear();
        }
    }

    // trinity_0170113 -- Initialize "da16x_sp_event_group"
    da16x_sp_event_group = xEventGroupCreate();

    extern bool umac_dpm_latest_status(void);
    if (get_last_dpm_sleep_type()) {
        /* WiFi connection status --> dpm booting & reconnecting */
        if (umac_dpm_latest_status()){    /* have the correct dpm information */
            abnormal_dpm_boot = 1;
        } else { /* WiFi disconnection status --> normal booting */
            abnormal_dpm_boot = 1;
            disable_dpm_wakeup();

            /* abnormal wakeup set to LMAC RX */
            dpm_lmac_set_tim_wakeup_abnormal();

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
            /* In abnormal wakeup, if ddps is set */
            /* For DPM TIM WAKEUP TIME is set automatically by TIM BUFP */ 
            if (dpm_dynamic_period_setting_flag)    
            {
                if (get_dpm_tim_ddps_macflag())                
                {
                    /* ddps bufp enable */
                    dpm_tim_bufp_chk_en();
                    /* dpm ddps retention flag clearing */
                    set_dpm_tim_ddps_macflag(0);
                }        
            }    
#endif
        }
    }
    
    if (chk_dpm_wakeup() == pdTRUE) {
#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )
        /*
         * Check factory-reset :
         *
         * Check button interrupt at here
         * because GPIO interrupt doesn't generate correctly
         * when DPM-Wakeup state with button push,
        */
        if (chk_factory_rst_button_press_status() == FALSE) {
            return FALSE;
        }
#endif // __SUPPORT_FACTORY_RESET_BTN__

        /* DPM Wakeup and not DPM Monitor abnormal booting */
        if (!abnormal_dpm_boot) {
            dpm_full_wakeup_wlaninit(false);
        } else {    /* DPM Wakeup and fast reconnect by abnormal daemon */
            dpm_full_wakeup_wlaninit(true);
        }
    } else {

        /********* Here is Normal boot *********/
#ifdef    __SUPPORT_IPV6__
        int use_ipv6 = 0;
        int use_dhcpv6 = 0;
#endif    /* __SUPPORT_IPV6__ */

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
        /* POR / RESET / REBOOT , For DPM TIM WAKEUP TIME is set automatically by TIM BUFP */ 
        if (dpm_dynamic_period_setting_flag) {
            if (BootScenario == WAKEUP_SOURCE_POR || BootScenario == WAKEUP_RESET) {
                if (get_run_mode() == SYSMODE_STA_ONLY && get_dpm_mode()) {
                    /* After setup and provisioning, ddps working is enabled */
                    if (get_dpm_DynamicPeriodSet_config())
                        dpm_tim_bufp_chk_en();
                }
            }
        }
#endif    // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__

        init_dpm_environment();

        /* Check factory button when booting time */
#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )
        if (chk_factory_rst_button_press_status() == FALSE) {
            return FALSE;
        }
#endif // __SUPPORT_FACTORY_RESET_BTN__

        /* get time zone form nvram */
        __timezone = get_time_zone();

        set_timezone_to_rtm(__timezone);

        if (sntp_support_flag == pdTRUE) {
            set_sntp_period_to_rtm(sntp_get_period());
        }

        /* do skip for sleep mode 1/2 fast connect , Factory Reset button*/
        if (fast_connection_sleep_flag == pdFALSE) {
            if (get_wlaninit() == pdFALSE) {
#ifdef BUILD_OPT_DA16200_ASIC
                extern void dpm_power_up_step4_patch(unsigned long prep_bit);
                dpm_power_up_step4_patch(0xf);
#endif // BUILD_OPT_DA16200_ASIC
                PRINTF("Fail to initialize WLAN. (step 1)\n!!! TEST MODE !!!\n");
                return pdFALSE;
            }
        }

        /* start the dpm sleep daemon , in case of STA Mode */
        if (get_run_mode() == SYSMODE_STA_ONLY) {
            start_dpm_sleep_daemon(0);
        }

        vTaskDelay(1);

        if (hw == NULL) {    // Check init status
            if (umac_init_func(false) == pdFALSE) {
                PRINTF("\nUMAC init. failed\n");
                return pdFALSE;
            }
          
            /* Station Only mode */
            if (get_run_mode() == SYSMODE_STA_ONLY) {
                set_tx_power_table(0);
            }
        } else {
            PRINTF("WLAN already initiated \r\n");
            return pdFALSE;
        }

#if defined ( __SUPPORT_CON_EASY__ )
        fc80211_set_ampdu_flag(0, 0); // TX 0
        fc80211_set_ampdu_flag(1, 1); // TR 1
#elif defined ( __SUPPORT_MESH__ )
  #if !defined (__SUPPORT_CON_EASY__)
        if (   get_run_mode() == SYSMODE_MESH_POINT
    #ifdef __SUPPORT_MESH_PORTAL__
            || get_run_mode() == SYSMODE_STA_N_MESH_PORTAL
    #endif /* __SUPPORT_MESH_PORTAL__ */
        )
  #endif /* ! __SUPPORT_CON_EASY__ */
        {
            fc80211_set_ampdu_flag(0, 0); // TX 0
            fc80211_set_ampdu_flag(1, 1); // TR 1
        }
#else    // Normal Mode
        if (fast_connection_sleep_flag == pdTRUE) {
            fc80211_set_ampdu_flag(0, 0); // TX 0
            fc80211_set_ampdu_flag(1, 1); // TR 1
        }
#endif    //

#ifdef    __SUPPORT_IPV6__
        if (ipv6_support_flag == pdTRUE) {
            if (read_nvram_int("IPv6", &use_ipv6) == -1) {
                use_ipv6 = pdFALSE;
            }

            if (use_ipv6) {
                if (read_nvram_int("DHCPv6", &use_dhcpv6) == -1)
                    use_dhcpv6 = 1;
            }
        }
#endif    /* __SUPPORT_IPV6__ */

        // Workaround POR reboot aging test ...
        vTaskDelay(1);

        printSysMode(get_run_mode());

        /* Network stack(LWIP) initialization, without DPM */
        netInit();

        /**** WLAN0_IFACE *****/
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
        switch (get_run_mode()) {
        case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
        case SYSMODE_STA_N_P2P:
  #endif /* __SUPPORT_P2P__ */
            /* DPM support only Station (SYSMODE_STA_ONLY)*/
            disable_dpm_mode();
        }
#endif /* __SUPPORT_WIFI_CONCURRENT__ */

        /***** WLAN1_IFACE *****/
#ifdef XIP_CACHE_BOOT
        switch (get_run_mode()) {
        case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #ifdef __SUPPORT_P2P__
        case SYSMODE_P2P_ONLY:
        case SYSMODE_P2PGO:
  #endif /* __SUPPORT_P2P__ */
        case SYSMODE_STA_N_AP:
  #ifdef __SUPPORT_P2P__
        case SYSMODE_STA_N_P2P:
  #endif /* __SUPPORT_P2P__ */
#endif /* __SUPPORT_WIFI_CONCURRENT__ */
        {
            int use_dhcpd = NONE_IFACE;

            if (dhcpd_support_flag == pdTRUE) {
                /* DHCP Server Start flag */
                if (read_nvram_int("USEDHCPD", &use_dhcpd) == -1) {
                    use_dhcpd = pdFALSE;
                }

                if (use_dhcpd == pdTRUE) {
#ifndef SUPPORT_FREERTOS
                    while (da16x_network_main_check_net_ip_status(WLAN1_IFACE) != pdTRUE) {
                        vTaskDelay(1);
                    }
#endif // SUPPORT_FREERTOS
                    dhcps_cmd_param * dhcp_param = pvPortMalloc(sizeof(dhcps_cmd_param));
                    memset(dhcp_param, 0, sizeof(dhcps_cmd_param));
                    dhcp_param->cmd = DHCP_SERVER_STATE_START;
                    dhcp_param->dhcps_interface = WLAN1_IFACE;
                    dhcps_run(dhcp_param);
                }
            }
            }
            break;
        }
#endif    /*XIP_CACHE_BOOT*/

        /* ######## START SUPPLICANT ######## */
        /* Start Wifi Monitor Thread to receive events from Supplicant */
        start_wifi_monitor();

        /* Start supplicant */
        if (start_da16x_supp() < 0) {
            PRINTF("Faild to start Supplicant\n");
        }

        /* For supplicant initializing... */
        if (fast_connection_sleep_flag == pdFALSE)
            vTaskDelay(7); // 70ms

        /** Other Mode(except STA Mode) Power Tabel Setting
        * AP Mode , P2P Mode Power table init
        */
        if (   get_run_mode() == SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_P2P_ONLY
            || get_run_mode() == SYSMODE_P2PGO
#endif /* __SUPPORT_P2P__ */
           ) {
            set_tx_power_table(1);
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
        } else if (get_run_mode() != SYSMODE_STA_ONLY) {    /* concurrent mode Power table init */
            set_tx_power_table(0);
#endif    /* __SUPPORT_WIFI_CONCURRENT__ */
        }


#ifdef    XIP_CACHE_BOOT
        if (sntp_support_flag == pdTRUE && sntp_get_use()) {
            sntp_cmd_param * sntp_param = pvPortMalloc(sizeof(sntp_cmd_param));
            sntp_param->cmd = SNTP_START;
            sntp_run(sntp_param);
        }
#endif    /*XIP_CACHE_BOOT*/

#ifdef __SUPPORT_IPV6__
        if (ipv6_support_flag == pdTRUE && use_ipv6) {
            switch (get_run_mode()) {
            case SYSMODE_STA_ONLY:
                ipv6_auto_init(WLAN0_IFACE, 0);
                break;
            case SYSMODE_AP_ONLY:
#ifndef __LIGHT_SUPPLICANT__
#ifdef __SUPPORT_P2P__
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif /* __SUPPORT_P2P__ */
                ipv6_auto_init(WLAN1_IFACE, use_dhcpv6 ? 1 : 0);
                break;
            case SYSMODE_STA_N_AP:
#ifdef __SUPPORT_P2P__
            case SYSMODE_STA_N_P2P:
#endif /* __SUPPORT_P2P__ */
                ipv6_auto_init(WLAN0_IFACE, 0);
                ipv6_auto_init(WLAN1_IFACE, use_dhcpv6 ? 1 : 0);
#endif /* __LIGHT_SUPPLICANT__ */
                break;
            }
        }
#endif /* __SUPPORT_IPV6__ */
    }

#ifdef FEATURE_TX_BUF_FULL_REC
    if (   get_run_mode() == SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
        || get_run_mode() == SYSMODE_P2PGO
#endif /* __SUPPORT_P2P__ */
       ) {
        if (um_rec_tmr_start() < 0) {
            PRINTF("\nFaild to start um_rec_tmr...\n");
        }
    }
#endif // FEATURE_TX_BUF_FULL_REC

    /* RTC timer create */
    dpm_timer_task_create();

#ifdef  __RUNTIME_CALCULATION__
    printf_with_run_time("End wlaninit");
#endif

    return pdTRUE;
}

#define PRODUCT_INFO_RL_RF9050        0x9050
#define PRODUCT_INFO_RL_RF9060        0x9060
void set_tx_power_table(int inf)
{
    uint16_t product_info = get_product_info();
    int    pwr_grade;

    /* Set the Power table of region Country Code Table */
    init_region_tx_level();

    if (product_info == PRODUCT_INFO_RL_RF9050) {
        fc80211_set_tx_power_table(inf, (unsigned int *)&region_power_level, (unsigned int *)&region_power_level_dsss);
    } else if (product_info == PRODUCT_INFO_RL_RF9060) {
        if (read_nvram_int(NVR_KEY_PWR_CTRL_SAME_IDX_TBL, &pwr_grade) == -1) {
            PRINTF("[%s] No available SAME_PWR_IDX_VAL in NVRAM: set tx pwr grade idx to 0\n", __func__);

            memset(region_power_level, 0, sizeof(region_power_level));
            memset(region_power_level_dsss, 0, sizeof(region_power_level_dsss));

            fc80211_set_tx_power_table(inf, (unsigned int *)&region_power_level, (unsigned int *)&region_power_level_dsss);
        } else {
            memset(region_power_level, (unsigned char)pwr_grade, sizeof(region_power_level));
            memset(region_power_level_dsss, (unsigned char)pwr_grade, sizeof(region_power_level_dsss));

            fc80211_set_tx_power_table(inf, (unsigned int *)&region_power_level, (unsigned int *)&region_power_level_dsss);
        }
    }
        
}


void disconnect_from_peers(int flag)
{
    switch (get_run_mode()) {
    case SYSMODE_AP_ONLY:
#ifndef    __LIGHT_SUPPLICANT__
#ifdef __SUPPORT_P2P__
    case SYSMODE_P2PGO:
#endif /* __SUPPORT_P2P__ */
    case SYSMODE_STA_N_AP:
        if (get_run_mode() > SYSMODE_STA_N_AP && get_run_mode() < SYSMODE_TEST) { /* concurrent */
            da16x_cli_reply("iface 1", NULL, NULL);
            vTaskDelay(3); // 30ms
        }
#endif    /* __LIGHT_SUPPLICANT__ */

        switch (flag) {
        case 1:
            da16x_cli_reply("deauthenticate all", NULL, NULL);
            break;
        case 2:
            da16x_cli_reply("deauthenticate FF:FF:FF:FF:FF:FF", NULL, NULL);
            break;
        case 3:
            da16x_cli_reply("disassociate all", NULL, NULL);
            break;
        case 4:
            da16x_cli_reply("disassociate FF:FF:FF:FF:FF:FF", NULL, NULL);
            break;
        }

        break;

    case SYSMODE_STA_ONLY:
        da16x_cli_reply("disconnect", NULL, NULL);
        break;

#ifdef __SUPPORT_P2P__
    case SYSMODE_P2P_ONLY:
        da16x_cli_reply("p2p_group_remove", NULL, NULL);
        break;
#endif /* __SUPPORT_P2P__ */
    }
}

int da16x_get_dpm_wakeup_type(void)
{
    int    wakeup_type = 0;

    extern ULONG da16x_get_wakeup_source();

    switch (da16x_get_wakeup_source()) {
    case DPM_UC:                /* UC */
    case DPM_UC_MORE:
        wakeup_type = DPM_PACKET_WAKEUP;
        break;
    case DPM_BC_MC:             /* MC/BC */
        wakeup_type = DPM_PACKET_WAKEUP;
        break;
    case DPM_BCN_CHANGED:       /* BCN CHANGED */
        wakeup_type = DPM_DEAUTH_WAKEUP;
        break;
    case DPM_NO_BCN:            /* NO BCN */
        wakeup_type = DPM_NOACK_WAKEUP;
        break;
    case DPM_FROM_FAST:         /* DPM FROM FAST */
        wakeup_type = DPM_RTCTIME_WAKEUP;
        break;
    case DPM_KEEP_ALIVE_NO_ACK: /* DPM_KEEP_ALIVE_NO_ACK  */
        wakeup_type = DPM_NOACK_WAKEUP;
        break;
    case DPM_FROM_KEEP_ALIVE:   /* DPM_FROM_KEEP_ALIVE    */
        break;
    case DPM_NO:                /* DPM_NO  */
         wakeup_type = DPM_TIM_ERR_WAKEUP;
        break;
    case DPM_AP_RESET:
         wakeup_type = DPM_DEAUTH_WAKEUP;
        break;
    case DPM_DEAUTH:
         wakeup_type = DPM_DEAUTH_WAKEUP;
        break;
    case DPM_DETECTED_STA:
        break;
    case DPM_USER0:             /* USER_0 */
        wakeup_type = DPM_USER_WAKEUP;
        break;
    case DPM_USER1:             /* USER_1 */
        wakeup_type = DPM_USER_WAKEUP;
        break;
    case DPM_FROM_FULL:         /* FROM_FULL */
        wakeup_type = DPM_RTCTIME_WAKEUP;
        break;
    default:
        wakeup_type = DPM_UNKNOWN_WAKEUP;
        break;
    }

    return wakeup_type;
}

/**
 * @fn get_boot_mode
 * @return NORMAL BOOT is 0 and FULL BOOT is 1
 */
int get_boot_mode(void)
{
    int BootScenario;

    /* Booting Scenario and Checkin */
    BootScenario = dpm_get_wakeup_source();

    /* Sleep mode 1 or Sleep mode 2 or POR or REBOOT(8) --> Normal full booting */
    
    if (ble_combo_ref_flag == pdTRUE) {
        if (    BootScenario == WAKEUP_SENSOR
             || BootScenario == WAKEUP_SENSOR_EXT_SIGNAL
             || BootScenario == WAKEUP_SENSOR_WAKEUP_COUNTER
             || BootScenario == WAKEUP_SENSOR_EXT_WAKEUP_COUNTER) {
            return 0;
        }
    }
    
    if (   BootScenario == WAKEUP_SOURCE_EXT_SIGNAL
        || BootScenario == WAKEUP_SOURCE_WAKEUP_COUNTER
        || BootScenario == WAKEUP_SOURCE_POR
        || BootScenario == WAKEUP_WATCHDOG
        || BootScenario == WAKEUP_RESET
        || BootScenario == WAKEUP_RESET_WITH_RETENTION) {
        return 0;
    }

    return 1;
}

/* EOF */
