/**
 ****************************************************************************************
 *
 * @file da16x_dhcp_client.c
 *
 * @brief DHCP Client handling module
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


#include "lwip/dhcp.h"
#include "da16x_dhcp_client.h"
#include "da16x_dpm.h"
#include "user_dpm_api.h"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"


extern u32_t dhcp_get_timeout_value(const struct netif *netif, int time_type);
extern void dpm_accept_arp_broadcast(unsigned long accept_addr, unsigned long subnet_addr);
extern int da16x_network_main_get_netmode(int iface);
extern unsigned int da16x_network_main_check_ip_addr(unsigned char iface);
extern void dns_setserver(u8_t numdns, const ip_addr_t *dnsserver);
extern const ip_addr_t * dns_getserver(u8_t numdns);
extern int set_dns_addr(int iface, char *ip_addr);


int dpm_clr_dhcpc_info(void)
{
    dpm_supp_ip_info_t     *dpm_ip_info;

    if (chk_retention_mem_exist() == pdFAIL) {
        return pdFAIL;
    }

    dpm_ip_info = (dpm_supp_ip_info_t *)get_supp_ip_info_ptr();

    memset(dpm_ip_info, 0, sizeof(dpm_supp_ip_info_t));
    return pdPASS;
}

int dpm_save_dhcpc_info(void)
{
    dpm_supp_ip_info_t     *dpm_ip_info;
    struct netif *netif;
    struct dhcp *dhcp_client;

    unsigned long    ip_addr, net_mask, gateway;
    unsigned long    dhcp_server_ip_addr, dns_server;
    int get_tid;

    // Network interface
    netif = netif_get_by_index(0 + 2);
    if (netif == NULL) {
        PRINTF("[%s] Wrong network_interface !!!\n", __func__);
        return -1;
    }

    // DHCP client
    dhcp_client = netif_dhcp_data(netif);
    if (dhcp_client == NULL) {
        PRINTF("[%s] Wrong DHCP data !!!\n", __func__);
        return -1;
    }

    if (chk_retention_mem_exist() == pdFAIL) {
        return -1;
    }

    dpm_ip_info = (dpm_supp_ip_info_t *)get_supp_ip_info_ptr();

    // IP address
    ip_addr = ip4_addr_get_u32(&dhcp_client->offered_ip_addr);
    if (ip_addr == IPADDR_ANY || ip_addr == IPADDR_NONE) {
        //PRINTF("[%s] IP addr(%x) is wrong\n", __func__ , ip_addr);
        return -1;
    }
    
    dpm_ip_info->dpm_ip_addr = ip_addr;

    // Netmask
    net_mask = ip4_addr_get_u32(&dhcp_client->offered_sn_mask);
    dpm_ip_info->dpm_netmask = net_mask;

    // Gateway
    gateway = ip4_addr_get_u32(&dhcp_client->offered_gw_addr);
    dpm_ip_info->dpm_gateway = gateway;
    
    // DNS Server address #0
    dns_server = ip4_addr_get_u32(&dhcp_client->dns_ip_addr);
    dpm_ip_info->dpm_dns_addr[0] = dns_server;

    // DNS Server address #1
    dpm_ip_info->dpm_dns_addr[1] = 0;    

    dpm_ip_info->dpm_lease = (unsigned long)dhcp_get_timeout_value(netif, OFFERED_T0_LEASE);    // Lease Time
    dpm_ip_info->dpm_renewal = (unsigned long)dhcp_get_timeout_value(netif, OFFERED_T1_RENEW);    // Renew Time
    dpm_ip_info->dpm_timeout = (unsigned long)dhcp_get_timeout_value(netif, OFFERED_T2_REBIND);    // Rebind Time

    // DHCP Server address
    dhcp_server_ip_addr = ip4_addr_get_u32(&dhcp_client->server_ip_addr);
    dpm_ip_info->dpm_dhcp_server_ip = dhcp_server_ip_addr;

    /* It is judged whether it is infinite lease time.  */
    if (dpm_ip_info->dpm_lease < DHCP_INFINITE_LEASE) {
#if defined ( __RECONNECT_NON_ACTION_DHCP_CLIENT__ )

        if (rtc_timer_info(TID_U_DHCP_CLIENT) > 0) {
            PRINTF("[%s] Cancel DHCP renew timer. Manual renew...\n", __func__);
            rtc_cancel_timer(TID_U_DHCP_CLIENT);
        }

#endif // __RECONNECT_NON_ACTION_DHCP_CLIENT__

        get_tid = rtc_register_timer((unsigned int)dpm_ip_info->dpm_renewal * 1000,
                                     NET_IFCONFIG,
                                     TID_U_DHCP_CLIENT,
                                     0,
                                     rtc_dhcp_renew_timeout);

    } else if (dpm_ip_info->dpm_lease == DHCP_INFINITE_LEASE) {
        get_tid = TID_U_DHCP_CLIENT;
    }

    if (dpm_ip_info->dpm_ip_addr == IPADDR_ANY || dpm_ip_info->dpm_ip_addr == IPADDR_NONE) {
        PRINTF("[%s] DPM IP addr(%x) is wrong\n", __func__ , dpm_ip_info->dpm_ip_addr);
        return -1;
    }

    dpm_accept_arp_broadcast(dpm_ip_info->dpm_ip_addr, dpm_ip_info->dpm_netmask);

    return get_tid;
}

int dpm_restore_dhcpc_info(void)
{
    dpm_supp_ip_info_t     *dpm_ip_info;
    struct netif *netif;
    struct dhcp *dhcp_client;

    // Network interface
    netif = netif_get_by_index(0 + 2);
    if (netif == NULL) {
        PRINTF("[%s] Wrong network_interface !!!\n", __func__);
        return pdFAIL;
    }

    // DHCP client
    dhcp_client = netif_dhcp_data(netif);
    if (dhcp_client == NULL) {
        PRINTF("[%s] Wrong DHCP data !!!\n", __func__);
        return pdFAIL;
    }

    if (chk_retention_mem_exist() == pdFAIL) {
        return pdFAIL;
    }

    dpm_ip_info = (dpm_supp_ip_info_t *)get_supp_ip_info_ptr();

    // IP address
    if (dpm_ip_info->dpm_ip_addr == IPADDR_ANY || dpm_ip_info->dpm_ip_addr == IPADDR_NONE) {
        //PRINTF("[%s] IP addr(%x) is wrong\n", __func__ , dpm_ip_info->dpm_ip_addr);
        return pdFAIL;
    }

    dhcp_client->offered_ip_addr.addr = dpm_ip_info->dpm_ip_addr;

    // DHCP Server address
    if (dpm_ip_info->dpm_dhcp_server_ip == IPADDR_ANY || dpm_ip_info->dpm_dhcp_server_ip == IPADDR_NONE) {
        PRINTF("[%s] DHCP Server IP addr(%x) is wrong\n", __func__ , dpm_ip_info->dpm_dhcp_server_ip);
        return pdFAIL;
    }

    dhcp_client->server_ip_addr.addr = dpm_ip_info->dpm_dhcp_server_ip;

    return pdTRUE;
}

void rtc_dhcp_renew_timeout(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    int dpm_sts, dpm_retry_cnt = 0;
    int iface = WLAN0_IFACE;    //iface_select
    struct netif *netif = NULL;

    netif = netif_get_by_index(iface+2);

    if (dhcp_get_state(netif) > 0) {
        return;
    }

    if (   (DHCPCLIENT != get_netmode(iface))
        || (chk_supp_connected() != 1)
        || (dpm_mode_is_wakeup() != 1)) {
        return;
    }

dpm_dhcp_clr_retry :
    if (dpm_retry_cnt++ < 5) {
        dpm_sts = dpm_app_sleep_ready_clear(NET_IFCONFIG);

        switch (dpm_sts) {
        case DPM_SET_ERR :
            vTaskDelay(1);
                
            goto dpm_dhcp_clr_retry;
            break;

        case DPM_SET_ERR_BLOCK :
            /* Don't need try continues */
            break;

        case DPM_SET_OK :
            break;
        }
    }

    dhcp_start(netif);
}

int dpm_get_dhcpc_ipinfo(void)
{
    struct netif *netif;
    struct dhcp *dhcp_client;

    unsigned long    ip_addr;

    // Network interface
    netif = netif_get_by_index(0 + 2);
    if (netif == NULL) {
        return 0;
    }

    // DHCP client
    dhcp_client = netif_dhcp_data(netif);
    if (dhcp_client == NULL) {
        return 0;
    }

    // IP address
    ip_addr = ip4_addr_get_u32(&dhcp_client->offered_ip_addr);
    if (ip_addr == IPADDR_ANY || ip_addr == IPADDR_NONE) {
        //PRINTF("[%s] IP addr(%x) is wrong\n", __func__ , ip_addr);
        return 0;
    }

    return 1;
}

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
extern int da16x_get_temp_staticip_mode(int iface_flag);

int set_dhcpCientIP_to_staticIP(void)
{
    struct netif *netif = netif_get_by_index(WLAN0_IFACE + 2);

    /* Check the network initialization */
    if (check_net_init(WLAN0_IFACE) != pdPASS || netif == NULL) {
        return pdFAIL;
    }
    
    if (   da16x_network_main_get_netmode(WLAN0_IFACE) == DHCPCLIENT
        && da16x_get_temp_staticip_mode(WLAN0_IFACE)) {

        if (da16x_network_main_check_ip_addr(WLAN0_IFACE) == pdTRUE) {
            int        status;
            char    ipaddr_str[IPADDR_LEN];
            char    subnet_str[IPADDR_LEN];
            char    gateway_str[IPADDR_LEN];
            unsigned char wait_cnt = 0;

            // DHCP client
            struct dhcp    *dhcp_client = netif_dhcp_data(netif);
            
            while (pdTRUE) {
                if (dhcp_client->state == DHCP_STATE_BOUND && dhcp_client->offered_gw_addr.addr > 0) {
                    strcpy(ipaddr_str, ipaddr_ntoa(&dhcp_client->offered_ip_addr));
                    strcpy(subnet_str, ipaddr_ntoa(&dhcp_client->offered_sn_mask));
                    strcpy(gateway_str, ipaddr_ntoa(&dhcp_client->offered_gw_addr));
                    break;
                } else {
                    if (chk_supp_connected() == pdPASS && wait_cnt % 300 == 0) {    // 3 sec.
                        PRINTF("[%s] Waiting for DHCP_Client's BOUND.\n", __func__);
                        wait_cnt = 0;
                    }

                    // DHCP_Dcline
                    if (   chk_supp_connected() == pdFAIL
                        || dhcp_client->state == DHCP_STATE_INIT
                        || dhcp_client->state == DHCP_STATE_BACKING_OFF) {
                        wait_cnt = 0;
                    }

                    vTaskDelay(1); // 10ms
                    wait_cnt++;
                }
            }

            status = ip_change(WLAN0_IFACE, ipaddr_str, subnet_str, gateway_str, pdTRUE);

            if (status == pdPASS) {
                if (dns_getserver(0) != (ip_addr_t *)NULL) {
                    dns_setserver(0, dns_getserver(0));
                }

                strcpy(ipaddr_str, ipaddr_ntoa(&dhcp_client->dns_ip_addr));
                set_dns_addr(WLAN0_IFACE, ipaddr_str);

                PRINTF(">>> Change to Static-IP mode with assigned IP address from DHCP mode.\n");
            } else {
                PRINTF("[%s] Error: Set Temporarily static IPaddress.:%s ", __func__, ipaddr_str);
            }

            return status;
        }
    }

    return pdFAIL;
}

#endif /* __SUPPORT_DHCPC_IP_TO_STATIC_IP__ */

/* EOF */
