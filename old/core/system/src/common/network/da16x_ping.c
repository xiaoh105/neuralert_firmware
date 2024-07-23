/**
 ****************************************************************************************
 *
 * @file da16x_ping.c
 *
 * @brief PING Client module
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

#include "sys_feature.h"
#include "da16x_ping.h"
#include "ping.h"

#if defined ( LWIP_DNS )
#include "lwip/dns.h"
#endif // LWIP_DNS

#include "clib.h"
#include "common_def.h"

#if defined ( __SUPPORT_ZERO_CONFIG__ )
#include "zero_config.h"
#endif // __SUPPORT_ZERO_CONFIG__


/* Global external variables for PING */
extern bool ping_display_on;

struct cmd_ping_param * ping_arg = NULL;

void ping_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(hostname);

    if (ping_arg != NULL) {
        if (ipaddr != NULL && is_in_valid_ip_class(ipaddr_ntoa(ipaddr))) {
#if defined ( DA16X_PING_DEBUG )
            PING_DEBUG_PRINT(">> Domain address Found: %s : %s\n", hostname, ipaddr_ntoa(ipaddr));
#endif // DA16X_PING_DEBUG

            ip_addr_copy(ping_arg->ipaddr, *ipaddr);
            ping_init_for_console(ping_arg, pdFALSE);
        } else {
            PRINTF("DNS query failed.\n");
            vPortFree(ping_arg);
            return;
        }
    } else {
        PRINTF("Ping parameter error!\n");
    }
}

void cmd_ping_client(int argc, char *argv[])
{
    extern int isvalidIPsubnetInterface(long ip, int iface);

    bool is_domain_addr = pdFALSE;
    char * domain_url = NULL;
    int n = 0;

    ping_arg = pvPortMalloc(sizeof(struct cmd_ping_param));
    memset(ping_arg, 0, sizeof(struct cmd_ping_param));

    ping_arg->count = DEFAULT_PING_COUNT;        /* default count */
    ping_arg->ping_interface = NONE_IFACE;
    
    memset(ping_arg->domain_str, 0, PING_DOMAIN_LEN);
    ping_arg->dns_query_wait_option = 400;

#if defined ( __SUPPORT_IPV6__ )
    memset(ping_arg->ipv6_dst, 0, sizeof(ping_arg->ipv6_dst));
    memset(ping_arg->ipv6_src, 0, sizeof(ping_arg->ipv6_src));

    /* IP version check */
    for (int n = 0; n < argc; n++) {
        /* -6 */
        if (argv[n][0] == '-' && argv[n][1] == '6') {
            ping_arg->is_v6 = 1;
        }
    }
#endif // __SUPPORT_IPV6__

    /* parsing */
    for (n = 0; n < argc; n++) {
        /* IP Address / Domain */
        if ((strlen(argv[n]) > 4) && (strchr(argv[n], '.') != NULL)) {
            if (is_in_valid_ip_class(argv[n])) {    /* check ip */
                ip4addr_aton(argv[n], &(ping_arg->ipaddr));
            } else {
                is_domain_addr = pdTRUE;
                domain_url = argv[n];
            }
        }

#if defined ( __SUPPORT_IPV6__ )
        if (ping_arg->ipv6_support_flag == pdTRUE) {
            /* IPv6 Address */
            if ((strlen(argv[n]) > 2) && (strchr(argv[n], ':') != NULL)) {
                if (!ping_arg->is_v6) {
                    PRINTF("Not used in IPv4!");
                    vPortFree(ping_arg);
                    return;
                }

                ping_arg->is_ipv6 = 1;

                if (ParseIPv6ToLong(argv[n], ping_arg->ipv6_dst, NULL) == 0) {
                    PRINTF("Invalid IPv6 Address\n");
                    vPortFree(ping_arg);
                    return;
                }
            }

            /* -S */
            if (argv[n][0] == '-' && argv[n][1] == 'S') {
                if (!ping_arg->is_v6) {
                    PRINTF("Not used in IPv4!");
                    vPortFree(ping_arg);
                    return;
                }

                if (ParseIPv6ToLong(argv[++n], ping_arg->ipv6_src, NULL) == 0) {
                    PRINTF("Invalid IPv6 Address (Source)\n");
                    vPortFree(ping_arg);
                    return;
                }
            }
        }
#endif // __SUPPORT_IPV6__

        /* -n */
        if (argv[n][0] == '-' && (argv[n][1] == 'n' || argv[n][1] == 'c')) {
            ping_arg->count = atol(argv[n + 1]);
        }

        /* -t */
        if (argv[n][0] == '-' && argv[n][1] == 't') {
            ping_arg->count = 0xFFFFFFFF;
        }

        /* -l */
        if (argv[n][0] == '-' && argv[n][1] == 'l') {
            ping_arg->len = ctoi(argv[n + 1]);
        }

        /* -w */
        if (argv[n][0] == '-' && argv[n][1] == 'w') {
            ping_arg->wait = ctoi(argv[n + 1]);
        }

        /* -i */
        if (argv[n][0] == '-' && argv[n][1] == 'i') {
            ping_arg->interval = ctoi(argv[n + 1]);
        }

        /* -I Interface */
        if (argv[n][0] == '-' && argv[n][1] == 'I') {
            if (strcasecmp(argv[n + 1], "WLAN0") == 0) {
                ping_arg->ping_interface = WLAN0_IFACE;
            } else if (strcasecmp(argv[n + 1], "WLAN1") == 0) {
                ping_arg->ping_interface = WLAN1_IFACE;
            } else {
                goto ping_help;
            }
        }
    }

    if (ping_arg->ping_interface == NONE_IFACE) {
        if (isvalidIPsubnetInterface(PP_HTONL(ping_arg->ipaddr.addr), WLAN1_IFACE)) {
            ping_arg->ping_interface = WLAN1_IFACE;
        } else {
            ping_arg->ping_interface = WLAN0_IFACE;
        }
    }

    if ((ping_arg->ping_interface == WLAN0_IFACE) && (get_run_mode() >= SYSMODE_AP_ONLY)) {
        switch (get_run_mode()) {
        case SYSMODE_AP_ONLY :
#if defined ( __SUPPORT_P2P__ )
        case SYSMODE_P2P_ONLY :
        case SYSMODE_P2PGO :
#endif    // __SUPPORT_P2P__
             ping_arg->ping_interface = WLAN1_IFACE;
             break;
        }
    }

    if (   (argc < 2)
        || (strcmp(argv[1], "help") == 0)
        || (ping_arg->len > MAX_PING_SIZE)
#if defined ( __SUPPORT_IPV6__ )
        || (ping_arg->ipaddr == 0 && is_v6 == 0)
        || (ping_arg->is_v6 != is_ipv6)
#else
        || (is_domain_addr == pdFALSE && (ping_arg->ipaddr.addr == 0))
#endif    // __SUPPORT_IPV6__
       ) {
ping_help:
        PRINTF("Usage: ping -I [interface] [domain|ip] -n [count] -l [size] "
               "-w [timeout] -i [interval]\n"
               "\t-I [wlan0|wlan1]\tInterface name\n"
               "\t-n or -c count\tNumber of echo requests to send.\n"
               "\t-l size\t\tSend buffer size.(MAX:%d)\n"
               "\t-w timeout\tTime to wait for a respone in milliseconds (Min:10ms)\n"
#if defined ( __SUPPORT_IPV6__ )
               "\t-6 IPv6\n"
#endif    // __SUPPORT_IPV6__
               "\t-i interval\tWait interval seconds between sending each packet.(MIN:10ms)\n\n"
               "\tex) ping -I wlan0 172.16.0.1 -l 1024 -n 10 -w 1000"
               " -i 1000\n"
#if defined ( __SUPPORT_IPV6__ )
               "\t    ping -6 fe80::1:2 -I wlan0"
#endif    // __SUPPORT_IPV6__
               "\n\n", MAX_PING_SIZE);

        vPortFree(ping_arg);
        return;
    }

    if (da16x_network_main_check_network_ready(ping_arg->ping_interface) == pdFALSE) {
        PRINTF("connect: Network is unreachable\n");
        vPortFree(ping_arg);
        return;
    }

    if (is_domain_addr == pdTRUE) {
#if defined ( __SUPPORT_ZERO_CONFIG__ )
        unsigned long ip_addr = 0;
        char ip_addr_str[18] = {0x00,};

  #ifndef __SUPPORT_IPV6__
        strncpy((char *)ping_arg->domain_str, domain_url, PING_DOMAIN_LEN);

        ip_addr = zeroconf_get_mdns_ipv4_address_by_name(domain_url, ping_arg->dns_query_wait_option);
        if (ip_addr == 0) {
            PRINTF("DNS Query failed.\n");
            vPortFree(ping_arg);
            return ;
        }

        longtoip(ip_addr, ip_addr_str);

        ipaddr_aton(ip_addr_str, &ping_arg->ipaddr);

        ping_init_for_console(ping_arg, pdFALSE);
  #else
        PRINTF("DNS Query failed.\n");
        vPortFree(ping_arg);    // Not supported yet.
  #endif // !__SUPPORT_IPV6__

#else

  #if defined ( __SUPPORT_IPV6__ )
        if (ping_arg->ipv6_support_flag == pdTRUE) {
            if (ping_arg->is_v6) {
                ParseIPv6ToLong(dns_AAAA_Query(argv[n]), ping_arg->ipv6_dst, NULL);
                is_ipv6 = 1;
            } else {
                ping_arg->ipaddr = (unsigned long)iptolong(dns_A_Query(argv[n], ping_arg->dns_query_wait_option));

                if (ping_arg->ipaddr == 0) {
                    PRINTF("DNS query failed.\n");
                    vPortFree(ping_arg);
                    return;
                }
            }
        } else
  #endif    // __SUPPORT_IPV6__
        {
  #if defined ( LWIP_DNS )
            ip_addr_t dns_ipaddr;

            PING_DEBUG_PRINT("DNS Server Addr:%s\n", ip4addr_ntoa(dns_getserver(0)));
            PING_DEBUG_PRINT("DNS Query Domain:%s\n",domain_url);

            strncpy((char *)ping_arg->domain_str, domain_url, PING_DOMAIN_LEN);

            err_t ret = dns_gethostbyname(domain_url, &dns_ipaddr, NULL, NULL);

            if (ERR_OK == ret) {
                PING_DEBUG_PRINT("DNS query success.:%s\n", ipaddr_ntoa(&dns_ipaddr));
                ip_addr_copy_from_ip4(ping_arg->ipaddr, dns_ipaddr);

                if (ping_arg->ipaddr.addr == 0) {
                    PRINTF("DNS query failed.\n");
                    vPortFree(ping_arg);
                    return;
                } else {
                    ping_init_for_console(ping_arg, pdFALSE);
                }
            } else if (ERR_INPROGRESS == ret) {
                PING_DEBUG_PRINT("DNS query in progress.\n");
                // Wait during in_progress state...
                while (ERR_INPROGRESS == ret) {
                    vTaskDelay(1);
                    ret = dns_gethostbyname(domain_url, &dns_ipaddr, NULL, NULL);
                    vTaskDelay(100);
                    ping_arg->dns_query_wait_option -= 100;
                    if ( ping_arg->dns_query_wait_option <= 0 ) {
                        PRINTF("Ping request could not find host %s. Please check the name and try again. \n", domain_url);
                        vPortFree(ping_arg);
                        return;
                    }
                }

                // Run PING request ...
                if (ERR_OK == ret) {
                    PING_DEBUG_PRINT("DNS query success.:%s\n", ipaddr_ntoa(&dns_ipaddr));
                    ip_addr_copy_from_ip4(ping_arg->ipaddr, dns_ipaddr);

                    if (ping_arg->ipaddr.addr == 0) {
                        PRINTF("DNS query failed.\n");
                        vPortFree(ping_arg);
                        return;
                    } else {
                        ping_init_for_console(ping_arg, pdFALSE);
                    }
                } else {
                    PRINTF("DNS query fail");
                    vPortFree(ping_arg);
                }
            } else {
                PRINTF("DNS query fail");
                vPortFree(ping_arg);
            }
  #endif //LWIP_DNS
        }
#endif // __SUPPORT_ZERO_CONFIG__
    } else {
        ping_init_for_console(ping_arg, pdFALSE);
    }
}

unsigned int da16x_ping_client(int iface,
                            char *domain_str,
                            unsigned long ipaddr,
                            unsigned long *ipv6dst,
                            unsigned long *ipv6src,
                            int len,
                            unsigned long count,
                            int wait,
                            int interval,
                            int nodisplay,
                            char *ping_result)
{
    DA16X_UNUSED_ARG(ipv6dst);
    DA16X_UNUSED_ARG(ipv6src);

    char ip_str[40] = { 0x00, };
    char my_ip_str[40] = { 0x00, };
#if defined ( __SUPPORT_IPV6__ )
    int  ipv6_src_index = -1;
    int  ip_version = IP_VERSION_V4;
    int  i, j;
#endif    // __SUPPORT_IPV6__

    bool is_domain_addr = pdFALSE;
    char *domain_url;
    struct cmd_ping_param *atcmd_ping_arg = NULL;
#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
    unsigned long reply_count = 0;
    unsigned int  status;
#endif    //( __DNS_2ND_CACHE_SUPPORT__ )

    if (len <= 0 || len > MAX_PING_SIZE) {
        len = DEFAULT_PING_SIZE;        /* default size */
    }

    if (count <= 0) {
        count = DEFAULT_PING_COUNT;        /* default count */
    }

    if (wait <= 0) {
        wait = DEFAULT_PING_WAIT;        /* default wait */
    }

    if (interval < MIN_INTERVAL) {
        interval = DEFAULT_INTERVAL;    /* default interval */
    }

#if defined ( __SUPPORT_IPV6__ )
    if (ipaddr == 0x0) {
        ip_version = IP_VERSION_V6;
    }
#endif    // __SUPPORT_IPV6__

    memset(ip_str, 0, sizeof(ip_str));
    memset(my_ip_str, 0, sizeof(my_ip_str));

    atcmd_ping_arg = pvPortMalloc(sizeof(struct cmd_ping_param));

    memset(atcmd_ping_arg, 0, sizeof(struct cmd_ping_param));
    memset(atcmd_ping_arg->domain_str, 0, PING_DOMAIN_LEN);
    atcmd_ping_arg->dns_query_wait_option = 400;

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdTRUE) {
        if (ip_version == IP_VERSION_V6) {
            PRINTF("=== [%s] #1 Need to write FreeRTOS code for this function ...\n", __func__);
        }
    } else
#endif    // __SUPPORT_IPV6__
    {
        /* Conver IP address to IP */
        longtoip((long)ipaddr, ip_str);

        if (is_in_valid_ip_class(ip_str) == pdTRUE) {   /* check ip */
            ip4addr_aton(ip_str, &(atcmd_ping_arg->ipaddr));
        } else {
            is_domain_addr = pdTRUE;
            domain_url = domain_str;
        }
    }

    atcmd_ping_arg->count = count;
    atcmd_ping_arg->len = len;
    atcmd_ping_arg->wait = wait;
    atcmd_ping_arg->interval = interval;
    atcmd_ping_arg->ping_interface = iface;

    if (is_domain_addr == pdTRUE) {
#if defined ( __SUPPORT_IPV6__ )
        if (atcmd_ping_arg->ipv6_support_flag == pdTRUE) {
            if (atcmd_ping_arg->is_v6) {
                ParseIPv6ToLong(dns_AAAA_Query(argv[n]), atcmd_ping_arg->ipv6_dst, NULL);
                is_ipv6 = 1;
            } else {
                atcmd_ping_arg->ipaddr = (unsigned long)iptolong(dns_A_Query(argv[n], ping_arg->dns_query_wait_option));

                if (atcmd_ping_arg->ipaddr == 0) {
                    PRINTF("[%s] Failed to query DNS ...\n", __func__);
                    return pdFAIL;
                }
            }
        } else
#endif    // __SUPPORT_IPV6__
        {
            ip_addr_t dns_ipaddr;

            strncpy((char *)atcmd_ping_arg->domain_str, domain_url, PING_DOMAIN_LEN);

            err_t ret = dns_gethostbyname(domain_url, &dns_ipaddr, ping_dns_found, NULL);

            if (ret == ERR_OK) {
                ip_addr_copy_from_ip4(atcmd_ping_arg->ipaddr, dns_ipaddr);

                if (atcmd_ping_arg->ipaddr.addr == 0) {
                    return pdFAIL;
                }
            } else {
                PRINTF("[%s] DNS query fail ...\n", __func__);

                return pdFAIL;
            }
        }
    }

    // Disable console display for PING
    ping_display_on = pdFALSE;

    ping_init_for_console(atcmd_ping_arg, pdTRUE);

    /* Wait until ping tx finished ... */
    while ((unsigned long)get_ping_send_cnt() < count) {
        vTaskDelay(1);
    }

    vTaskDelay(100);

    // Enable console display for PING
    ping_display_on = pdTRUE;

    /* Result ... */
    if (nodisplay == pdFALSE) {
        PRINTF("\n--- %s ping statistics ---\n%d packets transmitted, " \
               " %d received, %d%%(%d) packet loss, time %dms\n",
                        ip_str,
                        get_ping_send_cnt(),
                        get_ping_recv_cnt(),
                        ((get_ping_send_cnt() - get_ping_recv_cnt()) * 100) / get_ping_send_cnt(),
                        get_ping_send_cnt() - get_ping_recv_cnt(),
                        get_ping_total_time_ms() );

#if defined ( __SUPPORT_SIGMA_TEST__ )
            snprintf(uart_buffer, sizeof(uart_buffer),
                     "\n--- %s ping statistics ---\n%d packets transmitted, " \
                     " %d received, %d%%(%d) packet loss, time %dms\n",
                        ip_str,
                        get_ping_send_cnt(),
                        get_ping_recv_cnt(),
                        ((get_ping_send_cnt() - get_ping_recv_cnt()) * 100) / get_ping_send_cnt(),
                        get_ping_send_cnt() - get_ping_recv_cnt(),
                        get_ping_total_time_ms() );
            UART_WRITE(uart1, uart_buffer, strlen(uart_buffer));
#endif // __SUPPORT_SIGMA_TEST__

            PRINTF("rtt min/avg/max = %u/%u/%u ms\n\n",
                    get_ping_min_time_ms(),
                    get_ping_avg_time_ms(),
                    get_ping_max_time_ms());
    } else if (nodisplay == pdTRUE) {    /* for AT-Command code */
        sprintf(ping_result, "%u,%u,%u,%u,%u",
                            get_ping_send_cnt(),
                            get_ping_recv_cnt(),
                            get_ping_avg_time_ms(),
                            get_ping_min_time_ms(),
                            get_ping_max_time_ms());
    }

#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
    else if (nodisplay == 2) {
        if (reply_count == 0) {
            status = ERR_INPROGRESS;
        } else {
            status = ERR_OK;
        }
    }
#endif    // __DNS_2ND_CACHE_SUPPORT__

#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
    return status;
#else
    return pdPASS;
#endif    // __DNS_2ND_CACHE_SUPPORT__
}

/* EOF */
