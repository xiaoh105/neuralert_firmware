/**
 ****************************************************************************************
 *
 * @file dns_query_sample.c
 *
 * @brief Sample app of OTA update.
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


#if defined	(__DNS_QUERY_SAMPLE__)

#include "da16x_system.h"
#include "util_api.h"
#include "common_utils.h"
#include "da16x_dns_client.h"
#include "da16x_dhcp_client.h"
#include "da16x_network_main.h"

#define	MAX_URL_LEN				256
#define	MAX_IP_LEN				16
#define	MAX_IP_LIST_CNT			16
#define	MAX_DNS_QUERY_TIMEOUT	400

/* Test URL */
char *TEST_URL = "www.daum.net";
extern int netmode[2];

void dns_A_query_sample(char *test_url_str)
{
    char	*ipaddr_str = NULL;

    PRINTF(">>> IPv4 address DNS query test ...\n");

    /* DNS query with test url string */
    ipaddr_str = dns_A_Query(test_url_str, MAX_DNS_QUERY_TIMEOUT);

    /* Fail checking ... */
    if (ipaddr_str == NULL) {
        PRINTF("\nFailed to dns-query with %s\n", test_url_str);
    } else {
        PRINTF("- Name : %s\n", test_url_str);

        PRINTF("- Addresses : %s\n", ipaddr_str);
    }
}

void dns_query_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    char	*test_url = NULL;

    if (netmode[WLAN0_IFACE] == DHCPCLIENT) {
        // wait until dhcp is done
        while (da16x_network_main_check_dhcp_state(WLAN0_IFACE) != DHCP_STATE_BOUND) {
            vTaskDelay(100);
        }
    }

    vTaskDelay(500);

    /* Check test url */
    test_url = read_nvram_string("TEST_DOMAIN_URL");

    if (test_url == NULL) {
        test_url = TEST_URL;
    }

    PRINTF("\n\n");

    dns_A_query_sample(test_url);

    vTaskDelete(NULL);
}

#endif	// (__DNS_QUERY_SAMPLE__)

/* EOF */
