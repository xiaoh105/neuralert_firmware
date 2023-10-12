/**
 ****************************************************************************************
 *
 * @file da16x_dhcp_server.c
 *
 * @brief DHCP Server module
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

#include "da16x_dhcp_server.h"
#include "sys_feature.h"
#include "dhcpserver_options.h"
#include "dhcpserver.h"
#include "lwip/err.h"

#define IP_ADDRESS(a, b, c, d)    ((((ULONG)a) << 24) | (((ULONG)b) << 16) | (((ULONG)c) << 8) | ((ULONG)d))
#define NX_DHCP_NO_ADDRESS             IP_ADDRESS(0, 0, 0, 0)

#define NX_DHCP_IP_ADDRESS_MAX_LIST_SIZE      10

// keywords of stored data in nvram
static const char *DHCP_SERVER_CONF_START_IP        = DHCP_SERVER_START_IP;
static const char *DHCP_SERVER_CONF_END_IP            = DHCP_SERVER_END_IP;
static const char *DHCP_SERVER_CONF_LEASE_TIME        = DHCP_SERVER_LEASE_TIME;
static const char *DHCP_SERVER_CONF_DNS                = DHCP_SERVER_DNS;

#if defined ( __SUPPORT_IPV6__ )
static const char *DHCPV6_SERVER_CONF_START_IP        = DHCPV6_SERVER_START_IP;
static const char *DHCPV6_SERVER_CONF_END_IP        = DHCPV6_SERVER_END_IP;
static const char *DHCPV6_SERVER_CONF_LEASE_TIME    = DHCPV6_SERVER_LEASE_TIME;
static const char *DHCPV6_SERVER_CONF_DNS            = DHCPV6_SERVER_DNS;
#endif  // __SUPPORT_IPV6__

extern u8_t dhcps_is_running(void);

unsigned int get_current_ip_info(const unsigned int iface)
{
    DA16X_UNUSED_ARG(iface);

    PRINTF("=== [%s] Need to write FreeRTOS code for this function ...\n", __func__);

    return pdPASS;
}

unsigned int get_current_dns_ipaddr(unsigned long *dns_ipaddr, const unsigned int iface)
{
    DA16X_UNUSED_ARG(iface);

    unsigned int status = ERR_OK;
    ip4_addr_t dns_addr;

    dns_addr = dhcps_dns_getserver();

    *dns_ipaddr = dns_addr.addr;

    return status;
}

unsigned int set_start_ip_address(char *value, int ver)
{
    DA16X_UNUSED_ARG(ver);

    unsigned int status = ERR_OK;
    int result = 0;

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdFALSE && ver == 6) {
        PRINTF("Doesn't support IPv6 !!!\n");
        return status;
    }

    if (ipv6_support_flag == pdTRUE) {
        if (ver == 4) {
            result = write_nvram_string(DHCP_SERVER_CONF_START_IP, value);
        } else if (ver == 6) {
            result = write_nvram_string(DHCPV6_SERVER_CONF_START_IP, value);
        }
    } else
#endif // __SUPPORT_IPV6__
    {
        result = write_nvram_string(DHCP_SERVER_CONF_START_IP, value);
    }

    if (result != 0) {
        status = ER_NOT_SUCCESSFUL;
    }

    return status;
}

unsigned int set_end_ip_address(char *value, int ver)
{
    DA16X_UNUSED_ARG(ver);

    unsigned int status = ERR_OK;
    int result = 0;

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdFALSE && ver == 6) {
        PRINTF("Doesn't support IPv6 !!!\n");
        return status;
    }

    if (ipv6_support_flag == pdTRUE) {
        if (ver == 4) {
            result = write_nvram_string(DHCP_SERVER_CONF_END_IP, value);
        } else if (ver == 6) {
            result = write_nvram_string(DHCPV6_SERVER_CONF_END_IP, value);
        }
    } else
#endif // __SUPPORT_IPV6__
    {
        result = write_nvram_string(DHCP_SERVER_CONF_END_IP, value);
    }

    if (result != 0) {
        status = ER_NOT_SUCCESSFUL;
    }

    return status;
}

unsigned int set_range_ip_address_list(char *start, char *end, int ver)
{
    unsigned int status = ERR_OK;

    unsigned long start_ip = 0;
    unsigned long end_ip = 0;

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdFALSE && ver == 6) {
        PRINTF("Doesn't support IPv6 !!!\n");
        return status;
    }
#endif // __SUPPORT_IPV6__

    // to convert ip address from string
    if (ver == 4) {
        start_ip = (unsigned long)iptolong(start);
        end_ip = (unsigned long)iptolong(end);

        //to check range between end and start
        if (((end_ip - start_ip) >= NX_DHCP_IP_ADDRESS_MAX_LIST_SIZE) || (start_ip == NX_DHCP_NO_ADDRESS)) {

            PRINTF("ERR: Failed to set range of IP_addr list(Max:%d).\n", NX_DHCP_IP_ADDRESS_MAX_LIST_SIZE);

            return 0x9B; // NX_DHCP_INVALID_IP_ADDRESS (todo: later to adapt to lwip dhcp error number for debug)
            
        }

#if defined ( __SUPPORT_IPV6__ )
    } else if (ver == 6) {
        if (ParseIPv6ToLong(start, NULL, NULL) == 0 || ParseIPv6ToLong(end, NULL, NULL) == 0) {
            return NX_DHCP_INVALID_IP_ADDRESS;
        }

#endif // __SUPPORT_IPV6__
    }

    //to write start/end ip address
    status = set_start_ip_address(start, ver);

    if (status == ERR_OK) {
        status = set_end_ip_address(end, ver);
    } else {
        PRINTF("ERR: Failed to set range of IP_addr list(0x%02X).\n", status);
    }

    return status;
}

unsigned int get_range_ip_address_list(unsigned long *start_ipaddr, unsigned long *end_ipaddr)
{
    unsigned int status = ERR_OK;
    unsigned long start_ip = 0;
    unsigned long end_ip = 0;
    char *nvram_string = NULL;

#ifdef __SUPPORT_P2P__
    if (get_run_mode() == SYSMODE_P2P_ONLY || get_run_mode() == SYSMODE_STA_N_P2P) {
        *start_ipaddr = DHCP_SERVER_P2P_ULONG_START_IP;
        *end_ipaddr = DHCP_SERVER_P2P_ULONG_END_IP;
        return status;
    }
#endif /* __SUPPORT_P2P__ */

    // To get start ip address for list
    nvram_string = read_nvram_string(DHCP_SERVER_CONF_START_IP);

    if (nvram_string == NULL) {
        return ER_NOT_FOUND;
    } else {
        start_ip = (unsigned long)iptolong(nvram_string);
        nvram_string = NULL;
    }

    // To get end ip address for list
    nvram_string = read_nvram_string(DHCP_SERVER_CONF_END_IP);

    if (nvram_string == NULL) {
        return ER_NOT_FOUND;
    } else {
        end_ip = (unsigned long)iptolong(nvram_string);
        nvram_string = NULL;
    }

    // To check validation
    if ( (end_ip - start_ip) > NX_DHCP_IP_ADDRESS_MAX_LIST_SIZE ) {
        status = ER_SIZE_ERROR;
    } else {
        *start_ipaddr = start_ip;
        *end_ipaddr = end_ip;
        status = ERR_OK;
    }

    return status;
}

unsigned int set_lease_time(int value, int ver)
{
#if defined ( __SUPPORT_IPV6__ )
    NX_DHCPV6_SERVER *dhcpv6_server = &da16x_dhcpv6_server;
#endif // __SUPPORT_IPV6__

    unsigned int status = ERR_OK;
    int result = 0;

#if defined ( __SUPPORT_IPV6__ )

    if (ipv6_support_flag == pdFALSE && ver == 6) {
        PRINTF("Doesn't support IPv6 !!!\n");
        return status;
    }

#endif // __SUPPORT_IPV6__

    /* write lease time into nvram & server. */
    if (ver == 4) {
        dhcps_set_option_info(IP_ADDRESS_LEASE_TIME, &value, sizeof(dhcps_time_t));
    }

#if defined ( __SUPPORT_IPV6__ )

    if (ver == 6) {
        dhcpv6_server->nx_dhcpv6_lifetime_config = value;
    }

#endif // __SUPPORT_IPV6__

    if (status == ERR_OK) {
        if (ver == 4) {
            result = write_nvram_int(DHCP_SERVER_CONF_LEASE_TIME, value);
        }

#if defined ( __SUPPORT_IPV6__ )
        else if (ver == 6) {
            result = write_nvram_int(DHCPV6_SERVER_CONF_LEASE_TIME, value);
        }
#endif // __SUPPORT_IPV6__

        if (result != 0) {
            status = ER_NOT_SUCCESSFUL;
        }
    }

    return status;
}

unsigned int get_lease_time(int *value)
{
    DA16X_UNUSED_ARG(value);

    PRINTF("=== [%s] Need to write FreeRTOS code for this function ...\n", __func__);

    return pdPASS;
}

unsigned int set_dns_information(char *value, int ver)
{
    DA16X_UNUSED_ARG(ver);

    unsigned int status = ERR_OK;
    int result = 0;

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdTRUE) {
        if (ver == 4) {
            result = write_nvram_string(DHCP_SERVER_CONF_DNS, value);
        } else if (ver == 6) {
            result = write_nvram_string(DHCPV6_SERVER_CONF_DNS, value);
        }
    } else
#endif // __SUPPORT_IPV6__
    {
        result = write_nvram_string(DHCP_SERVER_CONF_DNS, value);
    }

    if (result != 0) {
        status = ER_NOT_SUCCESSFUL;
    }

    return status;
}

unsigned int get_dns_information(unsigned long *value, const unsigned int iface)
{
    unsigned int status = ERR_OK;
    unsigned long dns_ip = 0;
    char *nvram_string = NULL;

#ifdef __SUPPORT_P2P__
    if (get_run_mode() == SYSMODE_P2P_ONLY || get_run_mode() == SYSMODE_STA_N_P2P) {
        *value = DHCP_SERVER_P2P_STRING_DNS_IP;
        return status;
    }
#endif /* __SUPPORT_P2P__ */

    nvram_string = read_nvram_string(DHCP_SERVER_CONF_DNS);

    if (!(nvram_string == NULL || strlen(nvram_string) == 0)) {
        dns_ip = (unsigned long)iptolong(nvram_string);
        nvram_string = NULL;
    } else {
        status = get_current_dns_ipaddr(&dns_ip,  iface);
    }

    *value = dns_ip;

    return status;
}

unsigned int is_dhcp_server_running(void)
{
    return dhcps_is_running();
}

void usage_dhcpd(void)
{
    PRINTF("\nUsage: DHCP Server\n");

    PRINTF("\x1b[93m" "Name" "\x1b[0m\n");
    PRINTF("\tdhcpd - DHCP Server\n");

    PRINTF("\x1b[93m" "SYNOPSIS" "\x1b[0m\n");
    PRINTF("\tdhcpd [OPTION]...\n");

    PRINTF("\x1b[93m" "DESCRIPTION" "\x1b[0m\n");
    PRINTF("\tDHCP Server\n");

#ifdef __SUPPORT_P2P__
    if (get_run_mode() != SYSMODE_P2P_ONLY && get_run_mode() != SYSMODE_STA_N_P2P && get_run_mode() != SYSMODE_P2PGO)
#endif /* __SUPPORT_P2P__ */
    {

        PRINTF("\t\x1b[93m" "boot [on|off]" "\x1b[0m\n");
        PRINTF("\t\tSelect a DHCP server running flag.\n");

        PRINTF("\t\x1b[93m" "start" "\x1b[0m\n");
        PRINTF("\t\tStart DHCP Server\n");

        PRINTF("\t\x1b[93m" "stop" "\x1b[0m\n");
        PRINTF("\t\tStop DHCP Server\n");

        PRINTF("\t\x1b[93m" "range <Start IP ADDRESS> <END IP Address>" "\x1b[0m\n");
        PRINTF("\t\tSet range of IP address list. The max IP Address list size is %d.\n",
                DHCP_IP_ADDRESS_MAX_LIST_SIZE);

        PRINTF("\t\x1b[93m" "lease_time <Integer>" "\x1b[0m\n");
        PRINTF("\t\tSet client lease time(min %d sec. ~ max %d sec.)\n", MIN_DHCP_SERVER_LEASE_TIME, MAX_DHCP_SERVER_LEASE_TIME);

#if defined ( __SUPPORT_MESH_PORTAL__ ) || defined (__SUPPORT_MESH__)
        PRINTF("\t\x1b[93m" "dns <IP Address>" "\x1b[0m\n");
        PRINTF("\t\tSet DNS information for DHCP Server (MESH Mode Only)\n");
#endif // __SUPPORT_MESH_PORTAL__ || __SUPPORT_MESH__
    }

    PRINTF("\t\x1b[93m" "status" "\x1b[0m\n");
    PRINTF("\t\tDisplay status of DHCP Server\n");

    PRINTF("\t\x1b[93m" "lease [1|0]=Print Unassign Flag" "\x1b[0m\n");
    PRINTF("\t\tDispaly IP Lease Table\n");

#if defined ( __SUPPORT_IPV6__ )
    if (ipv6_support_flag == pdTRUE) {
        PRINTF("\t\x1b[93m" "-6" "\x1b[0m\n");
        PRINTF("\t\tDHCPv6 Server Commands\n");
        PRINTF("\t\tex) dhcpd -6 start\n");
        PRINTF("\t\t    dhcpd -6 stop\n");
        PRINTF("\t\t    dhcpd -6 range  <Start IPv6 ADDRESS> <END IPv6 Address>\n");
#if defined ( __SUPPORT_MESH_PORTAL__ ) || defined (__SUPPORT_MESH__
        PRINTF("\t\t    dhcpd -6 dns <IPv6 Address>\n");
#endif // __SUPPORT_MESH_PORTAL__ || __SUPPORT_MESH__
        PRINTF("\t\t    dhcpd -6 status\n");
    }

#endif // __SUPPORT_IPV6__

    PRINTF("\t\x1b[93m" "help" "\x1b[0m\n");
    PRINTF("\t\tDisplay help\n");

    return ;
}

int setDHCPServerBootStart(int flag)
{
    int ret;

    if (flag) {
        /* DHCP Server ON */
        ret = write_nvram_int("USEDHCPD", pdTRUE);
    } else {
        /* DHCP Server OFF */
        ret = delete_nvram_env("USEDHCPD");
    }

    return ret;
}

int getDHCPServerBootStart(void)
{
    int flag = 0;
    read_nvram_int("USEDHCPD", &flag);

    if (flag == -1) {
        flag = 0;
    }

    return flag;
}

unsigned int get_dhcp_server_state(unsigned int iface, unsigned int ver)
{
    DA16X_UNUSED_ARG(iface);
    DA16X_UNUSED_ARG(ver);

    return is_dhcp_server_running();
}

/* EOF */
