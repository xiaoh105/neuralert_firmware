/**
 ****************************************************************************************
 *
 * @file da16x_network_main.c
 *
 * @brief Network initialize and handle
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

#include <stdbool.h>

#include "supp_def.h"        // For feature __LIGHT_SUPPLICANT__

#include "sys_feature.h"
#include "da16x_network_main.h"
#include "da16x_dns_client.h"
#include "da16x_dhcp_client.h"
#include "da16x_ip_handler.h"

#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#include "dhcpserver.h"

#include "da16x_dpm.h"
#include "da16x_dpm_regs.h"
#include "user_dpm_api.h"
#include "common_def.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

/* External functions */
extern UINT set_netInit_flag(UINT iface);
extern err_t ethernetif_init(struct netif *netif);
extern void dpm_accept_arp_broadcast(unsigned long accept_addr, unsigned long subnet_addr);
extern UINT etharp_exist_defgw_addr(void);
extern int restore_arp_table(void);

#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
extern unsigned char get_last_abnormal_cnt(void);
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */

extern unsigned char fast_connection_sleep_flag;

/* Global variables */
TaskHandle_t lwip_init_task_handler = NULL;

int netmode[] = { DHCPCLIENT, STATIC_IP };

/* network init flag  wlan0=1, wlan1=2, wlan0+wlan1=3, eth0=4 */
static UCHAR netinit_flag    = NO_INIT;
static char dpm_wakeup_dhcp_client_renew_flag = pdFALSE;

unsigned int set_netInit_flag(unsigned int iface)
{
    unsigned int status = pdPASS;

    if (dpm_mode_is_enabled() == pdTRUE) {
        if (WLAN0_IFACE != iface) {
            disable_dpm_mode();
        }
    }

    /* wlan0=1, wlan1=2, wlan0+wlan1=3, eth0=4 */
    if (iface == WLAN0_IFACE) {
        netinit_flag = netinit_flag + 1;
    } else if (iface == WLAN1_IFACE) {
        netinit_flag = netinit_flag + 2;
    }

    /* Check the network initialization */
    status = check_net_init(iface);

    if (status != pdPASS) {
        if (iface == WLAN0_IFACE) {
            netinit_flag = netinit_flag - 1;
        } else if (iface == WLAN1_IFACE) {
            netinit_flag = netinit_flag - 2;
        }
    }

    return status;
}

#define TID_U_USER_WAKEUP    2
void user_wakeup_cb(char *timer_name)
{
    PRINTF("\t < TIMEOUT : USER WAKE UP >\n");

    DA16X_UNUSED_ARG(timer_name);
}


#undef USE_WLAN1_DNS
void prvLwipInitTask(void *pvParameters)
{
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;
    struct netif wlan0_iface;        /* Staion interface : wlan0 */
    struct netif wlan1_iface;        /* AP/P2P interface : wlan1 */

    ip_addr_t iface_ipaddr, iface_netmask, iface_gateway;
    ip_addr_t dns_ip_addr;

    /* For FreeRTOS lwip Network interface and WIFI Mac interface mapping as boot */
    char *p_ipaddr_str;
    char *p_netmask_str;
    char *p_gateway_str;
    char *p_dns_str;

    user_dpm_supp_ip_info_t  *dpm_ip_info;
    user_dpm_supp_net_info_t *dpm_netinfo;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_LwIP_init(sys_wdog_id);
    }

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_LwIP_init());

    ip_addr_set_zero(&iface_ipaddr);
    ip_addr_set_zero(&iface_netmask);
    ip_addr_set_zero(&iface_gateway);

    (void)pvParameters;

    // Started Lwip Init Task ...

    // Init lwip stack
    tcpip_init(NULL, NULL);

    /* [WLAN0] DPM mode lwip network stack initalization as connection */
    if (dpm_mode_is_wakeup() == pdTRUE) {
        dpm_ip_info = (user_dpm_supp_ip_info_t *)get_supp_ip_info_ptr();
        dpm_netinfo = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();
        
        /* Recover Network information from Retention Memory */
        da16x_network_main_set_netmode(WLAN0_IFACE, dpm_netinfo->net_mode, 0);

        // Restore DNS server info.
        dns_ip_addr.addr = dpm_ip_info->dpm_dns_addr[0];
        dns_setserver(0, &dns_ip_addr);
    
        set_netInit_flag(WLAN0_IFACE);

        // Restore netowork interface
        iface_ipaddr.addr  = dpm_ip_info->dpm_ip_addr;
        iface_netmask.addr = dpm_ip_info->dpm_netmask;
        iface_gateway.addr = dpm_ip_info->dpm_gateway;

        netif_add(&wlan0_iface, &iface_ipaddr, &iface_netmask, &iface_gateway,
                  NULL,
                  ethernetif_init,
                  ethernet_input);
    
        netif_set_default(&wlan0_iface);
    
        // Make it active ... after WIFI Connection and Ready
        // First of all ... do netif as down,
        // After WIFI Connection and Ready, do as UP
        netif_set_down(&wlan0_iface);

        if (!chk_abnormal_wakeup()) {
            restore_arp_table();
        }
    } else {
        /* [WLAN0] Default Network interface add */
        if (   get_run_mode() == SYSMODE_STA_ONLY
            || get_run_mode() == SYSMODE_STA_N_AP
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
           ) {
            char tmp_str[20];

            snprintf(tmp_str, sizeof(tmp_str), "%d:%s", WLAN0_IFACE, NETMODE);
            read_nvram_int(tmp_str, &netmode[WLAN0_IFACE]);

            if (netmode[WLAN0_IFACE] < 1) {
                netmode[WLAN0_IFACE] = DEFAULT_NETMODE_WLAN0;
            }

            if (get_netmode(0) == STATIC_IP) {
                p_ipaddr_str  = read_nvram_string(NVR_KEY_IPADDR_0);
                p_netmask_str = read_nvram_string(NVR_KEY_NETMASK_0);
                p_gateway_str = read_nvram_string(NVR_KEY_GATEWAY_0);

                if (p_ipaddr_str == NULL || p_netmask_str == NULL) {
                    p_ipaddr_str = DEFAULT_IPADDR_WLAN0;
                    p_netmask_str = DEFAULT_SUBNET_WLAN0;

                    if (p_gateway_str == NULL) {
                        p_gateway_str =    DEFAULT_GATEWAY_WLAN0;
                    }
                }

                ip_addr_set_zero(&iface_ipaddr);
                ip_addr_set_zero(&iface_netmask);
                ip_addr_set_zero(&iface_gateway);
                
                ipaddr_aton(p_ipaddr_str,  &iface_ipaddr);
                ipaddr_aton(p_netmask_str, &iface_netmask);
                ipaddr_aton(p_gateway_str, &iface_gateway);

                // DNS 1st
                p_dns_str = read_nvram_string(NVR_KEY_DNSSVR_0);

                if (p_dns_str == NULL) {
                    p_dns_str = DEFAULT_DNS_WLAN0;
                }

                ip_addr_set_zero(&dns_ip_addr);
                ipaddr_aton(p_dns_str, &dns_ip_addr);
                dns_setserver(0, &dns_ip_addr);

                // DNS 2nd
                p_dns_str = read_nvram_string(NVR_KEY_DNSSVR_0_2ND);

                if (p_dns_str == NULL) {
                    p_dns_str = DEFAULT_DNS_2ND;
                }

                ip_addr_set_zero(&dns_ip_addr);
                ipaddr_aton(p_dns_str, &dns_ip_addr);
                dns_setserver(1, &dns_ip_addr);

                /* Case of DPM & Static IP, Save info */
                if (dpm_mode_is_enabled() == pdTRUE) {
                    dpm_ip_info = (user_dpm_supp_ip_info_t *)get_supp_ip_info_ptr();
                    dpm_netinfo = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();
                    
                    dpm_netinfo->net_mode = STATIC_IP;
                    dpm_ip_info->dpm_ip_addr = iface_ipaddr.addr;
                    dpm_ip_info->dpm_netmask = iface_netmask.addr;
                    dpm_ip_info->dpm_gateway = iface_gateway.addr;
                    dpm_ip_info->dpm_dns_addr[0] = dns_getserver(0)->addr;
                    dpm_ip_info->dpm_dns_addr[1] = dns_getserver(1)->addr;

                    dpm_accept_arp_broadcast(dpm_ip_info->dpm_ip_addr, dpm_ip_info->dpm_netmask);
                }    
            } else {
                if (dpm_mode_is_enabled() == pdTRUE) {
                    dpm_netinfo = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();
                    dpm_netinfo->net_mode = DHCPCLIENT;
                }
            }

            set_netInit_flag(WLAN0_IFACE);
        }

        // [WLAN0] Add a network interface to the list of lwIP netifs.
        netif_add(&wlan0_iface, &iface_ipaddr, &iface_netmask, &iface_gateway,
                    NULL,
                    ethernetif_init,
                    ethernet_input);

        /* [WLAN1] AP or P2P, Secondary Network interface add */
        if (   get_run_mode() == SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_P2P_ONLY
            || get_run_mode() == SYSMODE_P2PGO
#endif /* __SUPPORT_P2P__ */
            || get_run_mode() == SYSMODE_STA_N_AP
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
           ) {
            p_ipaddr_str  = read_nvram_string(NVR_KEY_IPADDR_1);
            p_netmask_str = read_nvram_string(NVR_KEY_NETMASK_1);
            p_gateway_str = read_nvram_string(NVR_KEY_GATEWAY_1);
#ifdef USE_WLAN1_DNS 
            p_dns_str      = read_nvram_string(NVR_KEY_DNSSVR_0);
#endif // USE_WLAN1_DNS

            if (p_ipaddr_str == NULL || p_netmask_str == NULL ) {
#ifdef __SUPPORT_P2P__
                if (   get_run_mode() == SYSMODE_P2PGO
                    || get_run_mode() == SYSMODE_P2P_ONLY
                    || get_run_mode() == SYSMODE_STA_N_P2P
                    ) {
                    p_ipaddr_str  = IPADDR_ANY_STR;
                    p_netmask_str = IPADDR_ANY_STR;
                } else
#endif /* __SUPPORT_P2P__ */
                {
                    p_ipaddr_str  = DEFAULT_IPADDR_WLAN1;
                    p_netmask_str = DEFAULT_SUBNET_WLAN1;

                    if (p_gateway_str == NULL) {
                        p_gateway_str = DEFAULT_GATEWAY_WLAN1;
                    }
                }            
            }

#ifdef USE_WLAN1_DNS
            if (p_dns_str == NULL && get_run_mode() != SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
                && get_run_mode() != SYSMODE_P2PGO
                && get_run_mode() != SYSMODE_P2P_ONLY
                && get_run_mode() != SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
               ) {
                p_dns_str = DEFAULT_DNS_WLAN1;
            }
#endif // USE_WLAN1_DNS

            ip_addr_set_zero(&iface_ipaddr);
            ip_addr_set_zero(&iface_netmask);
            ip_addr_set_zero(&iface_gateway);

            ipaddr_aton(p_ipaddr_str,  &iface_ipaddr);
            ipaddr_aton(p_netmask_str, &iface_netmask);
            ipaddr_aton(p_gateway_str, &iface_gateway);

#ifdef USE_WLAN1_DNS
            ipaddr_aton(p_dns_str,   &dns_ip_addr);

            if (   get_run_mode() != SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
                && get_run_mode() != SYSMODE_P2PGO
                && get_run_mode() != SYSMODE_P2P_ONLY
                && get_run_mode() != SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
               ) {
                dns_setserver(0, &dns_ip_addr);
            }
#endif // USE_WLAN1_DNS

            // [WLAN1] Add a network interface to the list of lwIP netifs.
            netif_add(&wlan1_iface, &iface_ipaddr, &iface_netmask,     &iface_gateway,
                        NULL,
                        ethernetif_init,
                        ethernet_input);    //There is no device driver

            set_netInit_flag(WLAN1_IFACE);
        }

        /* 3. Station mode Netif Setup as default */
        if (   get_run_mode() == SYSMODE_STA_ONLY || get_run_mode() == SYSMODE_STA_N_AP
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
           ) {
            //Set the network interface as the default network interface.
            netif_set_default(&wlan0_iface);

            // Make it active ... after WIFI Connection and Ready
            // First of all ... do netif as down,
            // After WIFI Connection and Ready, do as UP
            netif_set_down(&wlan0_iface);
        }

        /* 4. If working mode is AP/P2P mode, Seconday Network if as default */
        if (   get_run_mode() == SYSMODE_AP_ONLY
#ifdef __SUPPORT_P2P__
            || get_run_mode() == SYSMODE_P2P_ONLY
            || get_run_mode() == SYSMODE_P2PGO
#endif /* __SUPPORT_P2P__ */
            || get_run_mode() == SYSMODE_STA_N_AP)
        {
            if (get_run_mode() == SYSMODE_STA_N_AP
#ifdef __SUPPORT_P2P__
                || get_run_mode() == SYSMODE_STA_N_P2P
#endif /* __SUPPORT_P2P__ */
               ) {
                //Set the network interface as the default network interface.
                netif_set_default(&wlan1_iface);
            }

            // Make it active ...after WIFI Connection and Ready
            // First of all ... do netif as down,
            // After WIFI Connection and Ready, do as UP
            netif_set_down(&wlan1_iface);
        }
    }

    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_LwIP_init());

    // !!! Do not delete this task by call vTaskDelete(NULL).
    //     Use the task-suspend instead.
    //
    // Suspend any task.
    // When suspended a task will never get any microcontroller processing time, no matter what its priority.
    vTaskSuspend(NULL);
}


unsigned int netInit(void)
{
    BaseType_t    xRtn;
    int dpm_sts, dpm_retry_cnt = 0;
#ifndef    __SUPPORT_SIGMA_TEST__
    int user_wu_time = 0;
#endif    /* __SUPPORT_SIGMA_TEST__ */

    /* Network Interface Initialize */
    xRtn = xTaskCreate(prvLwipInitTask,
                        "lwIP_init",
                        128 * 4,    //128 * 2,    //During DPM Aging, stack is increased by watchdog fault 
                        (void *) NULL,
                        OS_TASK_PRIORITY_USER+9,
                        &lwip_init_task_handler);

    if (xRtn != pdPASS) {
        PRINTF(RED_COLOR "[%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "LwipInitTask");
        return pdFALSE;
    }

    if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
        // Register for DPM operation
        dpm_app_register(NET_IFCONFIG, 0);
        set_dpm_init_done(NET_IFCONFIG);

        if ((dpm_mode_is_wakeup() == 1) && (chk_supp_connected() == 1)) {
dpm_dhcpc_set_r :

            if (dpm_retry_cnt++ < 5) {
                dpm_sts = set_dpm_sleep(NET_IFCONFIG);

                switch (dpm_sts) {
                case DPM_SET_ERR :
                    vTaskDelay(1);
                    goto dpm_dhcpc_set_r;
                    break;

                case DPM_SET_ERR_BLOCK :
                    /* Don't need try continues */
                    break;

                case DPM_SET_OK :
                    break;
                }
            }
        }
    }

#ifndef    __SUPPORT_SIGMA_TEST__

    if ((get_run_mode() == SYSMODE_STA_ONLY) && dpm_mode_is_enabled()) {
        da16x_arp_create_polling_state_check(0);

/* User Wakeup Timer Registeration */
        if (dpm_mode_is_wakeup() != 1) {
            /* User wake up time (id=2) init */
            user_wu_time = get_dpm_user_wu_time_from_nvram();

            if (user_wu_time > 0) {
                rtc_register_timer(user_wu_time, NULL, TID_U_USER_WAKEUP, 1, user_wakeup_cb);
            }
        }

    }

#endif    /* __SUPPORT_SIGMA_TEST__ */

    return pdTRUE;
}



long da16x_network_main_get_timezone(void)
{
    int use = 0;
    read_nvram_int(NVR_KEY_TIMEZONE, &use);

    if (use == -1) {
        use = 0;
    }

    return use;
}

unsigned int da16x_network_main_set_timezone(long timezone)
{
    extern int delete_nvram_env(const char *name);

    if (timezone != 0) {
        write_nvram_int(NVR_KEY_TIMEZONE, timezone);
    } else {
        delete_nvram_env(NVR_KEY_TIMEZONE);
    }

    return 0;
}

int da16x_network_main_get_netmode(int iface)
{
    return netmode[iface];
}

unsigned int da16x_network_main_set_netmode(unsigned char iface, unsigned char mode, unsigned char save)
{
    char tmp_str[10];

    memset(tmp_str, 0, 10);

    /* CONFIG NETMODE */
    netmode[iface] = mode;

    if (save) {
        snprintf(tmp_str, sizeof(tmp_str), "%d:%s", (int)iface, NETMODE);
        return (int)write_nvram_int(tmp_str, mode);
    }

    return 0;
}

char get_enable_restart_dhcp_client(void)
{
    return dpm_wakeup_dhcp_client_renew_flag;
}

/* In DPM Wakeup state, it sets whether to restart DHCP Client after WiFi reconnection. */
char set_enable_restart_dhcp_client(char flag)
{
    return dpm_wakeup_dhcp_client_renew_flag = flag;
}


#define NETIF_REPORT_TYPE_IPV4  0x01
#define NETIF_REPORT_TYPE_IPV6  0x02
extern void netif_issue_reports(struct netif *netif, u8_t report_type);
void da16x_network_main_reconfig_net(int state)
{
    unsigned int    status = pdPASS;
    int dpm_sts, dpm_retry_cnt = 0;
    struct netif *netif = NULL;

    switch (state) {
        case INF_NET_STATION :
        {
            if (check_net_init(WLAN0_IFACE) == pdPASS
#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__
                && (get_last_abnormal_cnt() > 0 || dpm_mode_is_wakeup() == pdFALSE || get_enable_restart_dhcp_client() == pdTRUE
                || chk_ipaddress(WLAN0_IFACE) == pdFAIL)    /* For: Power-on boot, abnormal boot */
#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */
                ) {
                netif = netif_get_by_index(2);
                status = (UCHAR)wifi_netif_status(WLAN0_IFACE);

                if ((status == 0xFF) || (status != pdPASS) || chk_ipaddress(WLAN0_IFACE) == pdPASS) {
                    /* === WLAN0_IFACE is DOWN === */
                    if (dpm_mode_is_enabled() == pdTRUE) {
                        dpm_retry_cnt = 0;

                        do {
                            dpm_sts = clr_dpm_sleep(NET_IFCONFIG);

                            if (dpm_sts == DPM_SET_OK) {
                                break;
                            }

                            vTaskDelay(1);
                        } while (dpm_retry_cnt++ < 5);

                        if (rtc_timer_info(TID_U_DHCP_CLIENT) > 0) {
                            //PRINTF("[%s] %d DHCPC Cancel\n", __func__, __LINE__);
                            rtc_cancel_timer(TID_U_DHCP_CLIENT);
                        }
                    }

                    if (chk_supp_connected() && chk_ipaddress(WLAN0_IFACE) == pdPASS) {  /* For: cli reassoc */
                        if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
                            /* DHCPCLIENT */
                            dhcp_renew(netif);
                        } else {
                            netif_issue_reports(netif, NETIF_REPORT_TYPE_IPV4 | NETIF_REPORT_TYPE_IPV6);
                        }
                    } else {
                        arp_entry_delete(WLAN0_IFACE); /* ARP table delect all */

                        if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
                            /* DHCPCLIENT */
                            dhcp_stop(netif);
                            dhcp_start(netif);
                        }
                    }
                } else {
                    /* === WLAN0_IFACE is UP === */
#ifdef __RECONNECT_NON_ACTION_DHCP_CLIENT__

                    if (rtc_timer_info(TID_U_DHCP_CLIENT) > 0) {
                        //PRINTF("[%s] %d DHCPC Cancel\n", __func__, __LINE__);
                        rtc_cancel_timer(TID_U_DHCP_CLIENT);
                    }

#endif /* __RECONNECT_NON_ACTION_DHCP_CLIENT__ */

                     /* If you are connected after waking up with the "Wakeup Switch" */
                    rtc_cancel_timer(TID_U_ABNORMAL);
#ifdef __DPM_FINAL_ABNORMAL__

                    if (get_final_abnormal_rtc_tid() > 0) {
                        //PRINTF("[%s] %d Final Abnormal Cancel\n", __func__, __LINE__);
                        rtc_cancel_timer(get_final_abnormal_rtc_tid());
                    }

#endif /* __DPM_FINAL_ABNORMAL__ */

                    if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
                        /* DHCPCLIENT */
                        /* Send DHCP DISCOVER when reassoc. operation */
                        if (dhcp_get_state(netif) > DHCP_STATE_OFF || chk_abnormal_wakeup()) {
                            arp_entry_delete(WLAN0_IFACE); /* ARP table delect all */
                            dhcp_stop(netif);
                        }

                        dhcp_start(netif);
                    } else {
                        netif_issue_reports(netif, NETIF_REPORT_TYPE_IPV4 | NETIF_REPORT_TYPE_IPV6);
                    }
                }
            }
            break;
        }

#if !defined (__LIGHT_SUPPLICANT__)
        case P2P_NET_GO:
        {
            dhcps_cmd_param * dhcp_param = pvPortMalloc(sizeof(dhcps_cmd_param));

            ip_change(WLAN1_IFACE, P2P_GO_IP_ADDR, P2P_GO_NETMASK, IPADDR_ANY_STR, pdFALSE);
            memset(dhcp_param, 0, sizeof(dhcps_cmd_param));
            dhcp_param->cmd = DHCP_SERVER_STATE_START;
            dhcp_param->dhcps_interface = WLAN1_IFACE;

            dhcps_run(dhcp_param);

            break;
        }

        case P2P_NET_CLIENT:
            netif = netif_get_by_index(3);
            dhcp_start(netif);
            break;

        case P2P_NET_DEVICE:
        {
            unsigned char ret;

            ret = (unsigned char)wifi_netif_status(WLAN1_IFACE);
            if (((ret == 0xFF) || (ret != pdTRUE))
                && chk_ipaddress(WLAN1_IFACE) == pdTRUE) {
                arp_entry_delete(WLAN1_IFACE); /* ARP table delect all */
            }

            if (get_netmode(WLAN1_IFACE) == DHCPCLIENT) {
                netif = netif_get_by_index(3);
                dhcp_release_and_stop(netif);
            } else {
                dhcps_cmd_param * dhcp_param = pvPortMalloc(sizeof(dhcps_cmd_param));

                memset(dhcp_param, 0, sizeof(dhcps_cmd_param));
                dhcp_param->cmd = DHCP_SERVER_STATE_STOP;
                dhcp_param->dhcps_interface = WLAN1_IFACE;

                dhcps_run(dhcp_param);
            }

            ip_change(WLAN1_IFACE, IPADDR_ANY_STR, IPADDR_ANY_STR, IPADDR_ANY_STR, pdFALSE);

            break;            
        }
#endif    // ! __LIGHT_SUPPLICANT__
    }
}

int da16x_network_main_get_sysmode(void)
{
    int sysmode = 0;

    read_nvram_int(ENV_SYS_MODE, &sysmode);

    if (sysmode == -1) {
        sysmode = SYSMODE_STA_ONLY;
    }

    return sysmode;
}

int da16x_network_main_set_sysmode(int mode)
{
    if (mode >= 0 && mode <= 5) {
        return write_nvram_int(ENV_SYS_MODE, mode);
    }

    return -1;
}

int da16x_get_fast_connection_mode(void)
{
    int fast_connection = 0;

    read_nvram_int(NVR_KEY_FST_CONNECT, &fast_connection);
    
    if (fast_connection == -1) {
        fast_connection = 0;
    }

    return fast_connection;
}

int da16x_set_fast_connection_mode(int mode)
{
    if (mode >= 0 && mode <= 1) {
        if (mode == 0) {
            delete_nvram_env(NVR_KEY_FST_CONNECT);
            return 0;
        } else {
            return write_nvram_int(NVR_KEY_FST_CONNECT, mode);
        }
    }

    return -1;
}

void da16x_network_main_change_iface_updown(unsigned int iface, unsigned int flag)
{
    /* dhcp_client run on just "wlan0" interface on da16x */
    struct netif *netif = netif_get_by_index(iface+2);

    DA16X_UNUSED_ARG(flag);

    if (iface == WLAN0_IFACE && get_netmode(iface) == DHCPCLIENT && netif_is_up(netif)) {
        if (netif != NULL) {
            if (dhcp_renew(netif) != ERR_OK) {
                PRINTF("DHCP Client Renew Fail\n");
            }
        } else {
            PRINTF("Network Interface is NULL!!\n");
        }
    }
}

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
void da16x_set_temp_staticip_mode(int mode, int save)
{
    if (mode >= 0 && mode <= 1) {
        if (mode == pdTRUE) {
            if (save  == pdTRUE) {
                write_nvram_int(ENV_KEY_TEMP_STATIC_IP, pdTRUE); // Temporarily use a static IPaddress.
            } else {
                write_tmp_nvram_int(ENV_KEY_TEMP_STATIC_IP, pdTRUE); // Temporarily use a static IPaddress.
            }
        } else {
            if (save  == pdTRUE) {
                delete_nvram_env(ENV_KEY_TEMP_STATIC_IP); // Temporarily use a static IPaddress.
            } else {
                delete_tmp_nvram_env(ENV_KEY_TEMP_STATIC_IP); // Temporarily use a static IPaddress.
            }
        }
    }
}

int da16x_get_temp_staticip_mode(int iface_flag)
{
    int t_staticIP = pdFALSE;

    if (iface_flag == WLAN0_IFACE) {
        read_nvram_int(ENV_KEY_TEMP_STATIC_IP, &t_staticIP); // Temporarily use a static IPaddress.
        if (t_staticIP < pdTRUE) {
            t_staticIP = pdFALSE;
        }
    }
    return t_staticIP;
}
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

void da16x_network_main_check_network(int iface_flag, int simple)
{
    struct netif *netif = netif_get_by_index(iface_flag + 2);

    /* Check the network initialization */
    if (check_net_init(iface_flag) != pdPASS) {
        return;
    }

    if (netif == NULL) {
        if (!simple)
            PRINTF("Interface(%s%d) NULL \n",
                   iface_flag == ETH0_IFACE ? "eth" : "wlan",
                   iface_flag == WLAN1_IFACE ? WLAN1_IFACE : WLAN0_IFACE);

        return;
    }

    PRINTF("\nWLAN%d\n", iface_flag);

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
    {
        PRINTF("\tNetMode: %s%s\n",
               get_netmode(iface_flag) == STATIC_IP ? "Static IP" : "DHCP Client",
               da16x_get_temp_staticip_mode(iface_flag) == pdTRUE ? "(TEMP)" : "");
    }
#else
    PRINTF("\tNetMode: %s\n",
           get_netmode(iface_flag) == STATIC_IP ? "Static IP" : "DHCP Client");
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__
    /* MAC Address */
    PRINTF("\tMAC Address %02X:%02X:%02X:%02X:%02X:%02X\n",
            netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
            netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

    /* IP Address */
    PRINTF("\tIP Address:%s  ", ipaddr_ntoa(&netif->ip_addr));

    /* Subnet */
    PRINTF("Mask:%s  ", ipaddr_ntoa(&netif->netmask));

    /* Gateway */
    PRINTF("Gateway:%s  ", ipaddr_ntoa(&netif->gw));

    /* MTU */
    PRINTF("MTU:%d\n", netif->mtu);

    if (netif_is_up(netif)) {
        if (iface_flag == WLAN0_IFACE) {
            /* DNS Server */
            PRINTF("\tDNS:");

            /* DNS */
            for (int idx = 0; idx < 2; idx++) {
                char tmp_str[16];

                strcpy(tmp_str, (ipaddr_ntoa((ip_addr_t *)dns_getserver(idx))));
                if (is_in_valid_ip_class(tmp_str)) {
                    PRINTF(" %s ", tmp_str);
                }
            }

            PRINTF("\n");
        }

#ifdef    __SUPPORT_IPV6__
        if (ipv6_support_flag == pdTRUE) {
            /* IPv6 Information */
            PRINTF("\tIPv6 Address:%s\n", ipaddr_ntoa(&netif->ip6_addr));
        }

#endif    /* __SUPPORT_IPV6__ */

        if (get_netmode(iface_flag == 1 ? WLAN1_IFACE : WLAN0_IFACE) == DHCPCLIENT) {
            struct dhcp *dhcp = netif_dhcp_data(netif);

            if (dhcp->offered_t0_lease == 0xFFFFFFFFUL) {
                PRINTF("\tIP Lease Time      : Forever\n");
            } else if (dhcp->offered_t0_lease) {
                PRINTF("\tIP Lease Time      : %02uh %02um %02us\n",
                       dhcp->offered_t0_lease / 3600,        /* Hour */
                       dhcp->offered_t0_lease / 60 % 60,    /* Minute */
                       dhcp->offered_t0_lease % 60);        /* Second */

                PRINTF("\tIP Renewal Time    : %02uh %02um %02us\n",
                       dhcp->offered_t1_renew / 3600,        /* Hour */
                       dhcp->offered_t1_renew / 60 % 60,    /* Minute */
                       dhcp->offered_t1_renew % 60);        /* Second */

                PRINTF("\t   Timeout\t   : %02uh %02um %02us\n",
                       (dhcp->t1_renew_time * DHCP_COARSE_TIMER_SECS) / 3600,        /* Hour */
                       (dhcp->t1_renew_time * DHCP_COARSE_TIMER_SECS) / 60 % 60,    /* Minute */
                       (dhcp->t1_renew_time * DHCP_COARSE_TIMER_SECS) % 60);        /* Sec. */
#if 0 // Debug
                PRINTF("\n\t%-19s: %5d\n", "tries",         dhcp->tries);
                PRINTF("\t%-19s: %5d sec (About)\n", "request_timeout",dhcp->request_timeout * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n", "t0_timeout",        dhcp->t0_timeout * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n", "t1_timeout",        dhcp->t1_timeout * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n", "t2_timeout",        dhcp->t2_timeout * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n", "t1_renew_time",    dhcp->t1_renew_time * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n", "t2_rebind_time",    dhcp->t2_rebind_time * DHCP_COARSE_TIMER_SECS);
                PRINTF("\t%-19s: %5d sec (About)\n\n", "lease_used",    dhcp->lease_used * DHCP_COARSE_TIMER_SECS);
#endif // Debug
            }
        }

        //netif->mib2_counters->ifoutnucastpkts
    }

    PRINTF("\n");
    /* Restore preemption.  */
}

int da16x_network_main_check_net_init(int iface)
{
    if (iface == NONE_IFACE) {
        iface = WLAN0_IFACE;
    }

    if (   (iface == WLAN0_IFACE && (netinit_flag == WLAN0_INIT || netinit_flag == CONCURRENT_INIT))
        || (iface == WLAN1_IFACE && (netinit_flag == WLAN1_INIT || netinit_flag == CONCURRENT_INIT)) ) {
        return pdPASS;
    }

    return pdFAIL;
}

unsigned int da16x_network_main_check_ip_addr(unsigned char iface)
{
    struct netif *netif = netif_get_by_index(iface + 2);
    unsigned long ip_address;

    ip_address = ip4_addr_get_u32(&netif->ip_addr);

    if (ip_address != IPADDR_ANY && ip_address != IPADDR_NONE) {
        return pdTRUE;
    }

    return pdFALSE;
}

unsigned char da16x_network_main_check_dhcp_state(unsigned char iface)
{
    struct netif *netif = netif_get_by_index(iface+2);
    unsigned char    dhcp_state;

    dhcp_state = dhcp_get_state(netif);

    return dhcp_state;
}

unsigned int da16x_network_main_check_network_ready(unsigned char iface)
{
    if (iface == NONE_IFACE) {
        iface = WLAN0_IFACE;
    }

    if (da16x_network_main_check_net_init(iface) == pdPASS) {
        if (dpm_mode_is_wakeup() == pdTRUE) {
            if (is_dpm_supplicant_done() == 1) {
                if (chk_supp_connected() == pdPASS) {
                    return pdTRUE;
                }
            }
        } else {
            if (chk_supp_connected() == pdPASS || get_run_mode() != SYSMODE_STA_ONLY) {
                if (   (iface == WLAN0_IFACE && da16x_network_main_check_net_link_status(iface))
                    || (iface == WLAN1_IFACE && da16x_network_main_check_net_link_status(iface)))
                {
                    return chk_ipaddress(iface);
                }
            }
        }
    }

    return pdFALSE;
}

int da16x_network_main_check_net_ip_status(int iface)
{
    return !da16x_network_main_check_network_ready(iface);
}

int da16x_network_main_check_net_link_status(int iface)
{
    struct netif *wlan0_netif = netif_get_by_index(WLAN0_IFACE + 2);
    struct netif *wlan1_netif = netif_get_by_index(WLAN1_IFACE + 2);

    if (   (iface == WLAN0_IFACE && (netif_is_up(wlan0_netif)))
        || (iface == WLAN1_IFACE && (netif_is_up(wlan1_netif)))) {
        return pdTRUE;
    }

    return pdFALSE;
}


#if !defined ( __SUPPORT_SIGMA_TEST__ )
extern unsigned char dpm_slp_time_reduce_flag;
static unsigned int  arp_req_send_flag = 1;    // Need Initialize value
#define APP_POLL_STATE        "poll_state"
#define DPM_REG_ERR           -1
#define WAIT_DPM_SLEEP        0
unsigned int get_current_arp_req_status(void)
{
    return arp_req_send_flag;
}

void polling_state_check(void *pvParameters)
{
    int sys_wdog_id = -1;
#ifdef __ENABLE_UNUSED__
    unsigned int    status;
#endif // __ENABLE_UNUSED__
    int        exist_gw;
    unsigned int    arp_req_wait_cnt = 0;
    const ip4_addr_t *gw_addr;
    unsigned long gw_ulongaddr;
#ifdef __ENABLE_UNUSED__
    const ip4_addr_t *net_mask;
    unsigned long netmask_ulong;
#endif    //__ENABLE_UNUSED__
    unsigned long ip_ulongaddr = 0;

    struct netif *netif;

    DA16X_UNUSED_ARG(pvParameters);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    netif = netif_get_by_index(0 + 2);

    if (netif == NULL) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        while(1) {
            netif = netif_get_by_index(0 + 2);
            if (netif)
                break;

            vTaskDelay(2);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }

    if (reg_dpm_sleep(APP_POLL_STATE, 0) == DPM_REG_ERR) {
        PRINTF("[%s] Failed to register %s ...\n", __func__, APP_POLL_STATE);
    }

    /* Initialize the flags */
    arp_req_send_flag = 1;

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        /* Do nothing anymore duing DPM sleep operation ... */
        if (chk_dpm_pdown_start() != WAIT_DPM_SLEEP) {
            goto next_turn;
        }

        if (arp_req_send_flag == 1) {
            /* Prohibit to enter DPM sleep mode */
            clr_dpm_sleep(APP_POLL_STATE);
        } else {
            /* Permit to enter DPM sleep mode */
            set_dpm_sleep(APP_POLL_STATE);
        }

        stop_sys_apps_completed();
        stop_user_apps_completed();

        /* Check if Connection & ip address assignment */
        if ((chk_supp_connected() == 1) && (da16x_network_main_check_ip_addr(0) == pdTRUE)) {
            /* checking the default gw address */
            exist_gw = etharp_exist_defgw_addr();

            if (exist_gw == 0) {
                /* Send ARP reqeuset if arp table is empty. */
                if (arp_req_wait_cnt == 0) {

                    /* In case of DPM Wakeup, After supplicant is done(End of key seting), we have to send packet */
                    if (dpm_mode_is_wakeup() == pdTRUE) {
                        da16x_sys_watchdog_notify(sys_wdog_id);

                        da16x_sys_watchdog_suspend(sys_wdog_id);

                        do {
                            if (is_dpm_supplicant_done()) {
                                break;
                            }

                            vTaskDelay(1);
                        } while (1);

                        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                    }

                    /* check the default gw address */
                    gw_addr = netif_ip4_gw(netif);
                    gw_ulongaddr = ip4_addr_get_u32(gw_addr);                    
                    if (gw_ulongaddr == IPADDR_ANY || gw_ulongaddr == IPADDR_NONE) {
                        PRINTF("[%s] gw ipaddr(%x) is wrong\n", __func__ , ip_ulongaddr);
                        arp_req_wait_cnt = 0;
                        arp_req_send_flag = 1;
                        goto next_turn;
                    }

#ifdef __ENABLE_UNUSED__     /* Temporary Code for DPM PTIM IP and subnet */
                    /* check the default gw address */
                    ip_addr = netif_ip4_addr(netif);
                    ip_ulongaddr = ip4_addr_get_u32(ip_addr);                    
                    
                    net_mask = netif_ip4_netmask(netif);
                    netmask_ulong = ip4_addr_get_u32(net_mask); 
                    if (ip_ulongaddr == IPADDR_ANY || ip_ulongaddr == IPADDR_NONE) {
                        PRINTF("[%s] IP addr(%x) is wrong\n", __func__ , ip_ulongaddr);
                        goto next_turn;
                    }
                    
                    dpm_accept_arp_broadcast(ip_ulongaddr, netmask_ulong);
                    
#endif  //__ENABLE_UNUSED__

                    arp_req_send_flag = 1;
#ifdef __ENABLE_UNUSED__     /* ip config is ok, so just do ARP Req sending 1 times per 10, don't need it */
                    status = etharp_request(netif , gw_addr);
                    if (status != ERR_OK) {
                        arp_req_wait_cnt = 0;
                        arp_req_send_flag = 1;
                        goto next_turn;
                    }
#else
                    etharp_request(netif , gw_addr);
#endif  //__ENABLE_UNUSED__
                    //PRINTF("[%s] Send ARP IP addr(%x) , GW(%x)\n", __func__ , ip_ulongaddr, gw_ulongaddr);
                }

                /* Wait until to receive response during 100 msec */
                if (arp_req_wait_cnt++ >= 10) {
                    arp_req_wait_cnt = 0;
                }

                if (dpm_slp_time_reduce_flag == 1) {
                    if ((arp_req_wait_cnt > 1) && dpm_mode_is_wakeup()) {
                        arp_req_wait_cnt = 0;
                        arp_req_send_flag = 0;
                        set_dpm_sleep(APP_POLL_STATE);
                    }
                }

                if (fast_connection_sleep_flag == pdTRUE) {
                    /* In Sleep mode 1,2 Fast connection, we don't need to make default ARP Table */
                    if (arp_req_wait_cnt > 1) {
                        arp_req_wait_cnt = 0;
                        arp_req_send_flag = pdTRUE;
                    }
                }
            } else if (exist_gw == 1) {      // Have ARP table for default_gw
                //PRINTF("[%s] DPM Default gw is exist\n");
                arp_req_wait_cnt = 0;
                arp_req_send_flag = 0;
            }
#ifdef __ENABLE_UNUSED__  /* Not used */
            else if (exist_gw == -1) {      // Error case
                arp_req_wait_cnt = 0;
                arp_req_send_flag = 1;
            }
#endif  //__ENABLE_UNUSED__
        } else {      // In case of no connection ...
            arp_req_wait_cnt = 0;
            arp_req_send_flag = 1;
        }

next_turn :
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        vTaskDelay(1);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    } // while()

    da16x_sys_watchdog_unregister(sys_wdog_id);
}
#endif

/* EOF */
