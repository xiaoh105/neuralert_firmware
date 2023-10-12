/**
 ****************************************************************************************
 *
 * @file sys_apps.c
 *
 * @brief System applications table
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

#include "da16x_system.h"
#include "monitor.h"
#include "da16x_network_common.h"
#include "da16x_sntp_client.h"
#include "application.h"
#include "nvedit.h"
#include "environ.h"
#include "iface_defs.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "common_def.h"
#include "mqtt_client.h"
#include "lwip/dhcp.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#ifdef __SUPPORT_ZERO_CONFIG__
#include "zero_config.h"
#endif // __SUPPORT_ZERO_CONFIG__

#if defined ( __HTTP_SVR_AUTO_START__ ) || defined ( __HTTPS_SVR_AUTO_START__ )
#include "user_http_server.h"
#endif // defined ( __HTTP_SVR_AUTO_START__ ) || defined ( __HTTPS_SVR_AUTO_START__ )

#include "sntp.h"

#undef    DBG_PRINT_INFO


/* External variables */
extern const app_task_info_t    user_apps_table[];
extern unsigned char fast_connection_sleep_flag;
extern UCHAR wifi_conn_flag;    /* After connection, set by callback api */


/* Local functions */
static void dpm_reg_customer_app(void);
static void dpm_unreg_customer_app(void);
static void create_sys_apps(int sysmode, UCHAR net_chk_flag);
static void create_user_apps(int sysmode, UCHAR net_chk_flag);


/* Global variables */
int  interface_select = 0;
int  sysmode_select = 0;
UINT dpm_reg_done = 0;

/* For Sample code */
void (*sample_app_start_cb)(UCHAR) = NULL;
void (*sample_app_dpm_reg_cb)(void) = NULL;
void (*sample_app_dpm_unreg_cb)(void) = NULL;

/* Local variables */
static run_app_task_info_t *sys_app_task_info = NULL;
static run_app_task_info_t *user_app_task_info = NULL;

static const app_task_info_t    sys_apps_table[];

TaskHandle_t user_task_handler = NULL;

#define    FAST_CON_ARP_CHECK_MAX_CNT    5    // During N * 100 = 1s checking
#undef    DEBUG_PRINT_CONNECT_TIME


static void fast_sleepmode_init(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
    int iface = interface_select;
#if defined ( __SUPPORT_DHCPC_IP_TO_STATIC_IP__ )
    int t_static_ip = 0;
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

    if (fast_connection_sleep_flag == pdFALSE || get_run_mode() != SYSMODE_STA_ONLY) {
        vTaskDelete(NULL);
        return;
    }

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

#if defined ( __SUPPORT_DHCPC_IP_TO_STATIC_IP__ )
    t_static_ip = da16x_get_temp_staticip_mode(WLAN0_IFACE);

    if (get_netmode(WLAN0_IFACE) == STATIC_IP) {
        UINT arp_req_wait_cnt = 0;

        arp_req_wait_cnt = 0;

        // Check communication status.
        while (pdTRUE) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            if (chk_supp_connected() == pdPASS) {
                if (do_autoarp_check() == 0) {
                    arp_req_wait_cnt = 0;
                    da16x_sys_watchdog_suspend(sys_wdog_id);
                    vTaskDelay(10); // 100ms
                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                    continue;
                } else if (arp_req_wait_cnt < FAST_CON_ARP_CHECK_MAX_CNT) {    /* ARP Checking for 500ms */
                    arp_req_wait_cnt++;
                    da16x_sys_watchdog_suspend(sys_wdog_id);
                    vTaskDelay(20); // 200ms
                    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                    continue;
                } else {    /* Communication error(Gateway IP ARP No response): DHCP is started  */
                    struct netif *netif = (struct netif *)netif_get_by_index(iface+2);
                    arp_req_wait_cnt = 0;

                    PRINTF(">>> Gateway is no ARP response, Start DHCP_Client\n");

                    if (t_static_ip) {
                        /* Set DHCP Client as netmode */
                        set_netmode(iface, DHCPCLIENT, pdTRUE);
                        dhcp_start(netif);

                        da16x_sys_watchdog_suspend(sys_wdog_id);
                        vTaskDelay(100);        //1000ms sleep    
                        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

                        break;
                    } else if (get_netmode(iface) == DHCPCLIENT) {
                        dhcp_renew(netif);

                        break;
                    }

                    continue;
                }

                break;
            } else {
                da16x_sys_watchdog_suspend(sys_wdog_id);
                vTaskDelay(1); // 10ms
                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
            }
        }
    }
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;
}


/* PORT_FOWARD */
void dpm_reg_appl(void)
{
    int    i;
    const app_task_info_t *cur_info;

    if (dpm_reg_done) return;

    for (i = 0 ; sys_apps_table[i].name != NULL ; i++) {
        cur_info = &(sys_apps_table[i]);

        if (   fast_connection_sleep_flag == pdFALSE
            && strcmp(cur_info->name, FAST_SLEEPMODE_MOD) == 0) {
            continue;
        }

        if ((cur_info->run_sys_mode & RUN_STA_MODE) && (cur_info->dpm_flag == TRUE)) {
            dpm_app_register(cur_info->name, cur_info->port_no);
        }
    }

    /* For customer applications ... */
    dpm_reg_customer_app();

#if defined ( __ENABLE_SAMPLE_APP__ )
    /*
     * Have to register sample callback for RTC timer before dpm_reg_appl()
     */

    // For sample pre-configuration ...
    sample_preconfig();

    // For sample codes ...
    register_sample_cb();
#endif  // __ENABLE_SAMPLE_APP__

    /* For test sample applications ... */
    if (sample_app_dpm_reg_cb != NULL) {
        sample_app_dpm_reg_cb();
    }

    dpm_reg_done = 1;
}

void dpm_unreg_appl(void)
{
    int    i;
    const app_task_info_t *cur_info;

    for (i = 0 ; sys_apps_table[i].name != NULL ; i++) {
        cur_info = &(sys_apps_table[i]);

        if (fast_connection_sleep_flag == pdFALSE && strcmp(cur_info->name, FAST_SLEEPMODE_MOD) == 0) {
            continue;
        }

        if ((cur_info->run_sys_mode & RUN_STA_MODE) && (cur_info->dpm_flag == TRUE)) {
            dpm_app_unregister(cur_info->name);
        }
    }

    dpm_unreg_customer_app();

    /* For test sample applications ... */
    if (sample_app_dpm_unreg_cb != NULL) {
        sample_app_dpm_unreg_cb();
    }
}


/******************************************************************************
 *  create_new_app( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
run_app_task_info_t    *create_new_app(run_app_task_info_t *prev_task,
                                        const app_task_info_t *cur_info,
                                        int sys_mode)
{
    run_app_task_info_t    *cur_task = NULL;

    switch (sys_mode) {
    case SYSMODE_STA_ONLY : {
        if (cur_info->run_sys_mode & RUN_STA_MODE) {
            break;
        } else {
            return NULL;
        }
    } break;

    case SYSMODE_AP_ONLY : {
        if (cur_info->run_sys_mode & RUN_AP_MODE) {
            break;
        } else {
            return NULL;
        }
    } break;

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )

#if defined ( __SUPPORT_P2P__ )
    case SYSMODE_P2P_ONLY : {
        if (cur_info->run_sys_mode & RUN_P2P_MODE) {
            break;
        } else {
            return NULL;
        }
    } break;

    case SYSMODE_P2PGO :
        return NULL;
#endif // __SUPPORT_P2P__

    case SYSMODE_STA_N_AP :
        break;

#if defined ( __SUPPORT_P2P__ )
    case SYSMODE_STA_N_P2P :
#endif // __SUPPORT_P2P__
        return NULL;

#endif    // __SUPPORT_WIFI_CONCURRENT__
    } // End of Switch

    cur_task = pvPortMalloc(sizeof(run_app_task_info_t));
    if (cur_task == NULL) {
        return NULL;
    }
    memset(cur_task, 0, sizeof(run_app_task_info_t));

    cur_task->prev = prev_task;

    // Create application task
    xTaskCreate(cur_info->entry_func,
                    (const char *)cur_info->name,
                    cur_info->stksize,
                    (void *)&cur_info->port_no,
                    cur_info->priority,
                    &(cur_task->task_handler));

    return cur_task;
}

void stop_completed_app(run_app_task_info_t **root)
{
    DA16X_UNUSED_ARG(root);

    // Add task stop precedure for FreeRTOS if needed.
    // Refer to V2.3 ThreadX version

    return ;
}

static void create_sys_apps(int sysmode, UCHAR net_chk_flag)
{
    run_app_task_info_t  *cur_task = sys_app_task_info;
    int i;

    for (i = 0 ; sys_apps_table[i].name != NULL ; i++) {
        /* Run matched apps with net_chk_flag */
        if (sys_apps_table[i].net_chk_flag == net_chk_flag) {
            if (   fast_connection_sleep_flag == pdFALSE
                && strcmp(sys_apps_table[i].name, FAST_SLEEPMODE_MOD) == 0) {
                // Thread is not created if it is not in Fast_Connection_Sleep1/2 mode.(FAST_SLEEPMODE_MOD)
                continue;
            }

            cur_task = create_new_app(cur_task, &(sys_apps_table[i]), sysmode);
        }
    }

    sys_app_task_info = cur_task;

    /* Create test samples apps */
    if (sample_app_start_cb != NULL) {
        sample_app_start_cb(net_chk_flag);
    }    
}

static void create_user_apps(int sysmode, UCHAR net_chk_flag)
{
    run_app_task_info_t  *cur_user_task = user_app_task_info;
    int    i;

    for (i = 0 ; user_apps_table[i].name != NULL ; i++) {
        if (user_apps_table[i].net_chk_flag == net_chk_flag) {
            cur_user_task = create_new_app(cur_user_task, &(user_apps_table[i]), sysmode);
            if ((strcmp(user_apps_table[i].name , "wifi_disconn") == 0))
            {
                user_task_handler = cur_user_task->task_handler;
            }
        }
    }

    user_app_task_info = cur_user_task;
}

void delete_user_wifi_disconn(void)
{
    if (user_task_handler != NULL) {
        if (strcmp(pcTaskGetName(user_task_handler), "wifi_discon") == 0)
        {
            vTaskDelete(user_task_handler);
            user_task_handler = NULL;
        }
    }
}

/**
 ****************************************************************************************
 * @brief       Start system applications to run DA16200 ( Registered in user_apps_tablep[] )
 * @return      None
 ****************************************************************************************
 */
void start_user_apps(void)
{
    int    sysmode;
    int sys_wdog_id = da16x_sys_wdog_id_get_system_launcher();

    /* Get System running mode ... */
    if (dpm_mode_is_wakeup() == DPM_WAKEUP) {
        /*
         * !!! Caution !!!
         *    Do not read operation from NVRAM when dpm_wakeup.
         */
        sysmode = SYSMODE_STA_ONLY;
    } else {
        sysmode = get_run_mode();
    }

    da16x_sys_watchdog_notify(sys_wdog_id);
    
    //Wait for network connection
    da16x_sys_watchdog_suspend(sys_wdog_id);

    /* Run user's network dependent apps */
    create_user_apps(sysmode, TRUE);

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
}

void stop_user_apps_completed(void)
{
    stop_completed_app(&user_app_task_info);
}


static void run_sys_apps(void)
{
    UINT32  i;
    int        sysmode;
    INT        tmpValue;
    INT        iface = WLAN0_IFACE;
    UINT    status;

    extern int check_net_init(int iface);
    extern int wifi_netif_status(int intf);

    sysmode = get_run_mode();

#if defined ( __RUNTIME_CALCULATION__ )
    printf_with_run_time("Run sys_apps");
#endif    // __RUNTIME_CALCULATION__

#if defined ( __ENABLE_SAMPLE_APP__ )
    // For sample pre-configuration ...
    sample_preconfig();

    // For sample codes ...
    register_sample_cb();
#endif  // __ENABLE_SAMPLE_APP__

    /* Create network independent apps */
    create_sys_apps(sysmode, FALSE);

    /* Create user's network independent apps */
    create_user_apps(sysmode, FALSE);

    if (sysmode == SYSMODE_STA_ONLY) {
        // In case of dpm wakeup, move to dpm wlaninit
        if (chk_dpm_mode() == pdTRUE) {
            dpm_reg_appl();        // Register DPM usage.
        }

        iface = WLAN0_IFACE;

        read_nvram_int((const char *)NVR_KEY_NETMODE_0, (int *)&tmpValue);

        if (tmpValue == ENABLE_DHCP_CLIENT || tmpValue == -1) {
            i = 0;

            while (1) {
                if (!check_net_ip_status(WLAN0_IFACE)) {
#if defined ( DBG_PRINT_INFO )

                    if (i) {
                        PRINTF("\n[%s] delay %d Time\n", __func__, i);
                    }

#endif    // DBG_PRINT_INFO

                    i = 0;
                    break;
                }

                i++;

                vTaskDelay(1);
            }
        }
    } else if (sysmode == SYSMODE_AP_ONLY) {
        iface = WLAN1_IFACE;

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
#ifdef __SUPPORT_P2P__
    } else if (sysmode == SYSMODE_P2P_ONLY) {
        iface = WLAN1_IFACE;
#endif /* __SUPPORT_P2P__ */
    } else if (sysmode == SYSMODE_STA_N_AP) {
        iface = WLAN1_IFACE;
#ifdef __SUPPORT_P2P__
    } else if (sysmode == SYSMODE_STA_N_P2P) {
        iface = WLAN1_IFACE;
    } else if (sysmode == SYSMODE_P2PGO) {
        iface = WLAN1_IFACE;
#endif /* __SUPPORT_P2P__ */
#endif    // __SUPPORT_WIFI_CONCURRENT__

    }
#ifdef __SUPPORT_MESH__
    else if (sysmode == SYSMODE_MESH_POINT || sysmode == SYSMODE_STA_N_MESH_PORTAL) {
        iface = WLAN1_IFACE;
    }
#endif
    else {
        PRINTF("[%s] Unknown sysmode ??? \n", __func__);
    }

    interface_select = iface;
    sysmode_select = sysmode;

    /* wait for network initialization */
    while (1) {
        if (check_net_init(iface) == pdPASS) {
            i = 0;
            break;
        }

        i++;

        vTaskDelay(1);
    }

    /* Waiting netif status */
    status = wifi_netif_status(iface);

    while (status == 0xFF || status != pdTRUE) {
        vTaskDelay(1);
        status = wifi_netif_status(iface);
    }

    /* Check IP address resolution status */
    while (check_net_ip_status(iface)) {
        vTaskDelay(1);
    }

#if defined ( __RUNTIME_CALCULATION__ )
    printf_with_run_time("START user_application");
#endif    // __RUNTIME_CALCULATION__

    /* Create network apps */
    create_sys_apps(sysmode, TRUE);
}

void start_sys_apps(void)
{
    int sys_wdog_id = da16x_sys_wdog_id_get_system_launcher();

#if defined ( __SUPPORT_SIGMA_TEST__ )
    sigma_host_init();
#endif    // __SUPPORT_SIGMA_TEST__

    if (get_run_mode() == SYSMODE_STA_ONLY) {
        if (chk_dpm_wakeup() == pdFALSE) {
            dpm_reg_appl();        // Early register of DPM appl.
        }
    }

    da16x_sys_watchdog_notify(sys_wdog_id);
    
    //Wait for network connection
    da16x_sys_watchdog_suspend(sys_wdog_id);

    /* Start AT-CMD module */
    start_atcmd();

    /* Start user application functions */
    run_sys_apps();

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
}

void stop_sys_apps_completed(void)
{
    stop_completed_app(&sys_app_task_info);
}


/*
 * For customer applications ....
 */

static void dpm_reg_customer_app(void)
{
    int    i;
    const app_task_info_t *cur_info;

    for (i = 0 ; user_apps_table[i].name != NULL ; i++) {
        cur_info = &(user_apps_table[i]);

        if (   (cur_info->run_sys_mode & RUN_STA_MODE)
            && (cur_info->dpm_flag == TRUE)) {
            dpm_app_register(cur_info->name, cur_info->port_no);
        }
    }
}

static void dpm_unreg_customer_app(void)
{
    int    i;
    const app_task_info_t *cur_info;

    for (i = 0 ; user_apps_table[i].name != NULL ; i++) {
        cur_info = &(user_apps_table[i]);

        if (   (cur_info->run_sys_mode & RUN_STA_MODE)
            && (cur_info->dpm_flag == TRUE)) {
            dpm_app_unregister(cur_info->name);
        }
    }
}

/////////////////////////////////////////////////////////////////

#if defined ( __SUPPORT_APMODE_SHUTDOWN_TIMER__ )

#if defined ( __SUPPORT_CONSOLE_PWD__ )
extern    void PASS_PRINTF(char *str);
#endif    // __SUPPORT_CONSOLE_PWD__

extern void fc80211_da16x_pri_pwr_down(unsigned char retention);

volatile UINT8 ap_shutdown_tm_running = 0;
static void ap_mode_shutdown_tm(void *pvParameters)
{
    UINT ap_living_time;
#if defined ( __SUPPORT_CONSOLE_PWD__ )
    char str[40];
#endif    // __SUPPORT_CONSOLE_PWD__

    if (read_nvram_int((const char *)NVR_KEY_AP_LIVING_TIME, (int *)&ap_living_time)) {
        ap_living_time = DFLT_AP_LIVING_TIME;
    }

#if defined ( __SUPPORT_CONSOLE_PWD__ )
    snprintf(str, 40, "\r Shut down in %d seconds...", ap_living_time / 1000);
    PASS_PRINTF(str);
#else
    PRINTF("[%s] Shut down in %d seconds... \n", __func__, ap_living_time / 1000);
#endif    // __SUPPORT_CONSOLE_PWD__

    ap_shutdown_tm_running = 1;

    if (!ap_shutdown_tm_running) {
        return;
    }

#if defined ( __SUPPORT_CONSOLE_PWD__ )
    PASS_PRINTF("\r Going Shut down ... \n");
#else
    PRINTF("[%s] Going Shut down ... \n", __func__);
#endif    // __SUPPORT_CONSOLE_PWD__

    vTaskDelay(10);

    fc80211_da16x_pri_pwr_down(0);
}

void cancel_ap_mode_shutdown_tm(void)
{
    PRINTF("[%s] Cancel AP Shut down timer \n", __func__);
    ap_shutdown_tm_running = 0;
}
#endif    // __SUPPORT_APMODE_SHUTDOWN_TIMER__

void terminate_sys_apps(void)
{
#if defined ( __SUPPORT_ZERO_CONFIG__ )
    zeroconf_stop_all_service();
#endif // __SUPPORT_ZERO_CONFIG__

    if (sntp_support_flag) {
        sntp_finsh();
    }

    return;
}

void sys_app_ip_change_notify(void)
{
#if defined ( __SUPPORT_ZERO_CONFIG__ )
    zeroconf_restart_all_service();
#endif    // __SUPPORT_ZERO_CONFIG__

    return;
}


/******************************************************************************
 * DA16200 system applications
 *****************************************************************************/

static const app_task_info_t sys_apps_table[] = {
  /* name, entry_func, stack_size, priority, timeslice, net_chk_flag, dpm_flag, port_no, run_sys_mode */

  /****** For fucntion features ***************************************/

#if defined ( __SUPPORT_DPM_EXT_WU_MON__ )
  { APP_EXT_WU_MON, dpm_ext_wu_mon,                  256, (OS_TASK_PRIORITY_USER),    FALSE,  TRUE,  UNDEF_PORT,      RUN_STA_MODE },
#endif    // __SUPPORT_DPM_EXT_WU_MON__

#if defined ( __SUPPORT_MQTT__ )
  { APP_MQTT_SUB,   mqtt_auto_start,                 512, (OS_TASK_PRIORITY_USER + 6), TRUE,  TRUE,  UNDEF_PORT,      RUN_STA_MODE },
#endif    // __SUPPORT_MQTT__

#if defined ( __HTTP_SVR_AUTO_START__ )
  { APP_HTTP_SVR,   auto_run_http_svr,               256, (OS_TASK_PRIORITY_USER + 6), TRUE,  FALSE, HTTP_SVR_PORT,   RUN_AP_MODE  },
#endif // __HTTP_SVR_AUTO_START__

#if defined ( __HTTPS_SVR_AUTO_START__ )
  { APP_HTTPS_SVR,  auto_run_https_svr,              512, (OS_TASK_PRIORITY_USER + 6), TRUE,  FALSE, HTTP_SVR_PORT,   RUN_AP_MODE  },
#endif // __HTTPS_SVR_AUTO_START__

#if defined ( __ENABLE_AUTO_START_ZEROCONF__ )
  { APP_ZEROCONF,  zeroconf_auto_start_reg_services, 384, (OS_TASK_PRIORITY_USER + 6), TRUE,  TRUE,  MULTICAST_PORT,  RUN_ALL_MODE },
#endif    // __ENABLE_AUTO_START_ZEROCONF__

#if defined (  __SUPPORT_APMODE_SHUTDOWN_TIMER__ )
  { APP_AP_TIMER,  ap_mode_shutdown_tm,              256, (OS_TASK_PRIORITY_USER + 6), FALSE, FALSE, UNDEF_PORT,      RUN_AP_MODE  },
#endif  // __SUPPORT_APMODE_SHUTDOWN_TIMER__

  { FAST_SLEEPMODE_MOD, fast_sleepmode_init,         256, (OS_TASK_PRIORITY_USER + 6), TRUE,  TRUE,  UNDEF_PORT,      RUN_STA_MODE },


  /******* End of List *****************************************************/
  { NULL,    NULL,    0, 0, FALSE, FALSE, UNDEF_PORT,    0    }

};

/* EOF */
