/**
 ****************************************************************************************
 *
 * @file da16x_arp.c
 *
 * @brief ARP handling module
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


#include "da16x_arp.h"
#include "da16x_ip_handler.h"
#include "lwip/prot/iana.h"
#include "netif/ethernet.h"
#include "clib.h"
#include "da16x_dpm_regs.h"


struct cmd_arping_param * etharping_arg;

UINT print_arp_table(UINT iface)
{
    etharp_print_arp_table(iface);

    return pdPASS;
}

int arp_entry_delete(int iface)
{
    struct netif *netif;

    netif = netif_get_by_index(iface+2);

    if (netif != NULL) {
        ARP_DEBUG_PRINT("WLAN[%d] ARP Entry deleted\n", iface);
        etharp_cleanup_netif (netif);
    } else {
        ARP_DEBUG_PRINT("\nWLAN[%d] Network Interface is NULL\n", iface);
        return pdFAIL;
    }

    return pdPASS;
}


extern err_t dhcp_etharp_request(struct netif *netif, const ip4_addr_t *ipaddr);
int dhcp_arp_request(int iface, ULONG ipaddr)
{
    struct netif *netif;

    if (iface != WLAN0_IFACE && iface != WLAN1_IFACE) {
        goto error;
    }

    netif = netif_get_by_index(iface+2);

    if (netif == NULL) {
        goto error;
    }

    if (ipaddr == 0) {
        dhcp_etharp_request(netif, netif_ip4_addr(netif));
    } else {
        struct ip4_addr dst_ip_addr;
        dst_ip_addr.addr=PP_HTONL(ipaddr);

        PRINTF("[%s] %d.%d.%d.%d\n", __func__,
                ipaddr >> 24 & 0xff,
                ipaddr >> 16 & 0xff,
                ipaddr >>  8 & 0xff,
                ipaddr       & 0xff);

        dhcp_etharp_request(netif, &dst_ip_addr);
    }

    return pdPASS;

error:
    PRINTF("\n[WLAN%d] Network Interface is NULL\n", iface);
    return pdFAIL;
}


int arp_request(ip_addr_t ipaddr, int iface)
{
    err_t status;

    if (iface != WLAN0_IFACE && iface != WLAN1_IFACE) {
        goto error;
    }

    struct netif *netif;

    netif = netif_get_by_index(iface+2);

    if (netif == NULL) {
        goto error;
    }

    /* ARP */
    status = etharp_request(netif, &ipaddr);

    if (status != ERR_OK) {
        return pdFAIL;
    }

    return pdPASS;

error:
    PRINTF("\n[WLAN%d] Network Interface is NULL\n", iface);
    return pdFAIL;
}

int arp_response(int iface, char *dst_ipaddr,  char *dst_macaddr)
{
    ARP_DEBUG_PRINT("arp_response()\n");

    struct netif *netif;
    ip4_addr_t sipaddr;
    struct eth_addr shwaddr;
    err_t status;

    netif = netif_get_by_index(iface+2);

    ipaddr_aton(dst_ipaddr, &sipaddr);

    UINT  idx, len;
    len = strlen(dst_macaddr);

    char *tmp_addr = pvPortMalloc(3);
    memset(tmp_addr, 0, sizeof(tmp_addr));

    if (len == 12 || len == 17) {
        for (idx = 0; idx < len; idx++) {
            if (len == 17 && ((idx+ 1)%3 == 0)) {
                idx++;
            }
        
            if (!isxdigit((int)(dst_macaddr[idx]))) {
                PRINTF("MAC Addr is invalid.\n");
                vPortFree(tmp_addr);
                return pdFAIL;
            }
        }

        for(idx = 0; idx < 6; idx++) {
            if (len == 12) {
                strncpy(tmp_addr, dst_macaddr+(idx*2), 2);
            } else if (len == 17) {
                strncpy(tmp_addr, dst_macaddr+(idx*3), 2);
            }

            shwaddr.addr[idx] = strtoul(tmp_addr, NULL, 16);
        }

        ARP_DEBUG_PRINT("Dst mac addr-%02x:%02x:%02x:%02x:%02x:%02x\n",
                shwaddr.addr[0], shwaddr.addr[1],
                shwaddr.addr[2], shwaddr.addr[3],
                shwaddr.addr[4], shwaddr.addr[5]);
    } else {
        PRINTF("MAC Addr is invalid.\n");

        vPortFree(tmp_addr);

        return pdFAIL;
    }

    vPortFree(tmp_addr);

    status = etharp_send(netif,
                       (struct eth_addr *)netif->hwaddr, &shwaddr,
                       (struct eth_addr *)netif->hwaddr, netif_ip4_addr(netif),
                       &shwaddr, &sipaddr,
                       ARP_REPLY);

    if (status != ERR_OK) {
        return pdFAIL;
    }

    return pdPASS;
}

int garp_request(int iface, int check_ipconflict)
{
    struct netif *netif;

    netif = netif_get_by_index(iface+2);

    if (netif == NULL) {
        PRINTF("Network Interface is NULL\n");
        return pdFAIL;
    }

    /* GARP */
    err_t status = etharp_gratuitous(netif);

    if (status == ERR_OK) {
        ARP_DEBUG_PRINT("\n[WLAN%d] Send GARP\n", iface);
    } else if (status == ERR_USE && check_ipconflict == pdTRUE) {
        PRINTF("\n[WLAN%d] Address already in use.\n", iface);
    } else {
        PRINTF("\n[WLAN%d] Send GARP Error: %d\n", status);
        return pdFAIL;
    }

    return pdPASS;
}

UINT arp_send_for_ip_collision_avoid(int t_static_ip)
{
    UINT    status = pdFALSE;

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
    if (t_static_ip) {    // Temporarily use a static IPaddress.
        PRINTF(">>> Send DHCP_ARP to check IP collision...\n");
        status = dhcp_arp_request(WLAN0_IFACE, 0);
    } else
#else
    DA16X_UNUSED_ARG(t_static_ip);
#endif /* __SUPPORT_DHCPC_IP_TO_STATIC_IP__ */
    {
        // Default Static IP Address
        PRINTF(">>> Send GARP to check IP collision...\n");
        status = garp_request(WLAN0_IFACE, pdTRUE);
    }

    return status;
}


void cmd_arping(int argc, char *argv[])
{
    etharping_arg = pvPortMalloc(sizeof(struct cmd_arping_param));
    memset(etharping_arg, 0, sizeof(struct cmd_arping_param));

    /* parsing */
    for (int n = 0; n < argc; n++) {
        /* IP Address / Domain */
        if ((strlen(argv[n]) > 4) && (strchr(argv[n], '.') != NULL)) {
            if (is_in_valid_ip_class(argv[n])) {    /* check ip */
                ip4addr_aton(argv[n], &(etharping_arg->ipaddr));
            } else {
                PRINTF("IP Address failed.\n");
                return;
            }
        }

        /* -n */
        if (argv[n][0] == '-' && (argv[n][1] == 'n' || argv[n][1] == 'c')) {
            etharping_arg->count = atol(argv[n + 1]);
        }

        /* -t */
        if (argv[n][0] == '-' && argv[n][1] == 't') {
            etharping_arg->count = 0xFFFFFFFF;
        }

        /* -w */
        if (argv[n][0] == '-' && argv[n][1] == 'w') {
            etharping_arg->wait = ctoi(argv[n + 1]);
        }

        /* -i */
        if (argv[n][0] == '-' && argv[n][1] == 'i') {
            etharping_arg->interval = ctoi(argv[n + 1]);
        }

        /* -I Interface */
        if (argv[n][0] == '-' && argv[n][1] == 'I') {
            if (strcasecmp(argv[n + 1], "WLAN0") == 0) {
                etharping_arg->ping_interface = WLAN0_IFACE;
            } else if (strcasecmp(argv[n + 1], "WLAN1") == 0) {
                etharping_arg->ping_interface = WLAN1_IFACE;
            } else {
                goto arping_help;
            }
        }
    }

    if ((argc < 2) || (strcmp(argv[1], "help") == 0) || (etharping_arg->ipaddr.addr == 0)) {
arping_help:
        PRINTF("Usage: arping -I [interface] [ip] -n [count]"
               "-w [timeout] -i [interval]\n"
               "\t-I [wlan0|wlan1] Interface name\n"
               "\t-n or -c count   Stop after sending count ARP REQUEST.\n"
               "\t-w timeout       Specify a timeout, in milliseconds,\n"
               "\t                 before arping exists regardless of how many packets\n"
               "\t                 have been sent or received. (Min:10ms)\n"
               "\t-i interval      Interval in msec to wait for each reply.(MIN:10ms)\n\n"
               "\tex) arping 172.16.0.1 -n 10 -w 1000  -i 1000\n");

        vPortFree(etharping_arg);
        return;
    }

    etharping_init(etharping_arg);
}


/** 
 * In DPM Abnormal routine, check the default gw field in the arp table
 * Success(Exist) Return 0  , Fail(Not exist) Return 1;
 */
extern int etharp_exist_addr(struct netif *netif, const ip4_addr_t *ipaddr , struct eth_addr **eth_ret);
extern unsigned int da16x_network_main_check_ip_addr(unsigned char iface);

UINT etharp_exist_defgw_addr(void)
{
    const ip4_addr_t *gw_addr;
    struct eth_addr *ethaddr;
    struct netif *netif;
    unsigned long gw_ulongaddr;

    /* Find the netif by interface number */
    /* Default Station address for DPM */
    netif = netif_get_by_index(0 + 2);

    if (netif == NULL) {
        return pdFALSE;
    }

    /* check the default gw address */
    gw_addr = netif_ip4_gw(netif);
    gw_ulongaddr = ip4_addr_get_u32(gw_addr);

    /* check the ip address */
    if (da16x_network_main_check_ip_addr(0) == pdFALSE) {
        return pdFALSE;
    }

    if (gw_ulongaddr == IPADDR_ANY || gw_ulongaddr == IPADDR_NONE) {
        return pdFALSE;
    }

    if (etharp_exist_addr(netif , gw_addr , &ethaddr)) {
        dpm_accept_tim_arp_resp(gw_ulongaddr, ethaddr->addr);
        return pdTRUE;
    }
    return pdFALSE;

}

int do_autoarp_check(void)
{
    const ip4_addr_t *net_addr;
    unsigned long ulongaddr;

    struct netif *netif;

    if (etharp_exist_defgw_addr() == 0) {
        netif = netif_get_by_index(0 + 2);

        if (netif == NULL) {
            PRINTF("[%s] Netif is wrong\n", __func__);
            return -1;
        }

        /* check interface ip address */
        net_addr = netif_ip4_addr(netif);
        ulongaddr = ip4_addr_get_u32(net_addr);                    

        if (ulongaddr == IPADDR_ANY || ulongaddr == IPADDR_NONE) {
            return -1;
        }

        /* check the default gw address */
        net_addr = netif_ip4_gw(netif);
        ulongaddr = ip4_addr_get_u32(net_addr);                    

        if (ulongaddr == IPADDR_ANY || ulongaddr == IPADDR_NONE) {
            PRINTF("[%s] gw ipaddr(%x) is wrong\n", __func__ , ulongaddr);
            return -1;
        }

        if (etharp_request(netif, net_addr) == ERR_OK) {
            if (!chk_dpm_wakeup()) {
                PRINTF("[%s] GW ARP Request\n", __func__);
            }

            return 1; //ARP Request OK  (ARP response status is unknown.)
        }
    }
#ifdef DEBUG_ARP_CHECK
    else if (!chk_dpm_wakeup()) {
        PRINTF("[%s] GW ARP Found\n", __func__);
    }
#endif // DEBUG_ARP_CHECK
    return 0;
}

/* If we need to send ARP REQ BC for DPM  */ 
void do_dpm_autoarp_send(void)
{
    const ip4_addr_t *gw_addr;
    unsigned long gw_ulongaddr;
    struct netif *netif;

    netif = netif_get_by_index(0 + 2);
        
    if (netif == NULL) {
        PRINTF("[%s] Netif is wrong\n", __func__);
        return;
    }

    /* check the default gw address */
    gw_addr = netif_ip4_gw(netif);
    gw_ulongaddr = ip4_addr_get_u32(gw_addr);                    
    if (gw_ulongaddr == IPADDR_ANY || gw_ulongaddr == IPADDR_NONE) {
        PRINTF("[%s] gw ipaddr(%x) is wrong\n", __func__ , gw_ulongaddr);
        return ;
    }
    
    etharp_request(netif , gw_addr);

    return;
}

void do_set_dpm_ip_net(void)
{
    const ip4_addr_t *ip_addr , *net_mask;
    unsigned long ip_ulongaddr;
    unsigned long netmask_ulong;
    struct netif *netif;

    netif = netif_get_by_index(0 + 2);
        
    if (netif == NULL) {
        PRINTF("[%s] Netif is wrong\n", __func__);
        return;
    }

    /* check the default gw address */
    ip_addr = netif_ip4_addr(netif);
    ip_ulongaddr = ip4_addr_get_u32(ip_addr);                    

    net_mask = netif_ip4_netmask(netif);
    netmask_ulong = ip4_addr_get_u32(net_mask);    
    if (ip_ulongaddr == IPADDR_ANY || ip_ulongaddr == IPADDR_NONE) {
        PRINTF("[%s] gw ipaddr(%x) is wrong\n", __func__ , ip_ulongaddr);
        return ;
    }

    dpm_accept_arp_broadcast(ip_ulongaddr, netmask_ulong);
}

//manage arp table for dpm
extern struct etharp_entry arp_table[ARP_TABLE_SIZE];
void save_arp_table()
{
    struct etharp_entry    *da16x_dpm_arp;
    struct netif *netif;
    
    if (!chk_dpm_mode()) {
        return;
    }

    netif = netif_get_by_index(0 + 2);
        
    if (netif == NULL) {
        PRINTF("[%s] Netif is wrong\n", __func__);
        return;
    }

    da16x_dpm_arp = (struct etharp_entry *)get_da16x_dpm_arp_ptr();

    if (da16x_dpm_arp == NULL) {
        return;
    }

    if (ARP_ALLOC_SZ >= sizeof(arp_table)) {
        memcpy(&da16x_dpm_arp[0], &arp_table, sizeof(arp_table));
    } else {
        memcpy(&da16x_dpm_arp[0], &arp_table,
               ((ARP_ALLOC_SZ / sizeof(struct etharp_entry)) * sizeof(struct etharp_entry)));
    }
    
    return;
}

int restore_arp_table(void)
{
    struct etharp_entry    *da16x_dpm_arp;
    int i;
    struct netif *netif;
    
    netif = netif_get_by_index(0 + 2);
        
    if (netif == NULL) {
        PRINTF("[%s] Netif is wrong\n", __func__);
        return -1;
    }

    if (chk_dpm_mode() == pdTRUE) {
        da16x_dpm_arp = (struct etharp_entry *)get_da16x_dpm_arp_ptr();

        if (da16x_dpm_arp == NULL) {
            return -1;
        }

        if (chk_dpm_wakeup() == 1 && chk_supp_connected() == 1) {
            memcpy(&arp_table, &da16x_dpm_arp[0], sizeof(arp_table));
            
            for (i = 0; i < ARP_TABLE_SIZE; i++) {
                arp_table[i].q = NULL;
                if (netif)
                    arp_table[i].netif = netif;
                else
                    arp_table[i].netif = NULL;
            }            
        } else {
            // ARP Table of Retention is clearing
            memset(da16x_dpm_arp, 0, ARP_ALLOC_SZ);
        }
    }

    return 0;
}


void cleanup_arp_table(void)
{
    struct etharp_entry    *da16x_dpm_arp;
    
    if (chk_dpm_mode() == pdFALSE) {
        return;
    }

    da16x_dpm_arp = (struct etharp_entry *)get_da16x_dpm_arp_ptr();

    if (da16x_dpm_arp == NULL) {
        return;
    }

    // ARP Table of Retention is clearing
    memset(da16x_dpm_arp, 0, ARP_ALLOC_SZ);    
    return;
}

// Polling Task in DPM
TaskHandle_t poll_state_chk_thd = NULL;
extern void polling_state_check(void *pvParameters);
UINT da16x_arp_create_polling_state_check(UINT iface)
{
    BaseType_t    xRtn;

#if !defined ( __SUPPORT_SIGMA_TEST__ )

    if (iface == WLAN0_IFACE) {
        /* Network Interface Initialize */
        xRtn = xTaskCreate(polling_state_check,
                                "NetPollSts",
                                128 * 4,
                                (void *) NULL,
                                OS_TASK_PRIORITY_USER +9,
                                &poll_state_chk_thd);
        if (xRtn != pdPASS) {
            PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "NetPollSts");
            return pdFALSE;
        }
    }
#endif // !__SUPPORT_SIGMA_TEST__

    return xRtn;
}

/* EOF */
