/**
 ****************************************************************************************
 *
 * @file user_dpm.c
 *
 * @brief User APIs for DPM operation
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

#if defined ( __SUPPORT_DPM_CMD__ )

#include "da16x_system.h"
#include "user_dpm.h"
#include "user_dpm_api.h"


//////////////////////////////////////////////////////////////////////////////
/// DPM related functions for USER ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void show_rtm_for_app(void)
{
    user_dpm_supp_net_info_t    *user_dpm_supp_net_info;
    user_dpm_supp_ip_info_t     *user_dpm_supp_ip_info;
    user_dpm_supp_conn_info_t   *user_dpm_supp_conn_info;
    user_dpm_supp_key_info_t    *user_dpm_supp_key_info;

    user_dpm_supp_net_info  = (user_dpm_supp_net_info_t *)get_supp_net_info_ptr();
    user_dpm_supp_ip_info   = (user_dpm_supp_ip_info_t *)get_supp_ip_info_ptr();
    user_dpm_supp_conn_info = (user_dpm_supp_conn_info_t *)get_supp_conn_info_ptr();
    user_dpm_supp_key_info  = (user_dpm_supp_key_info_t *)get_supp_key_info_ptr();

    if (   user_dpm_supp_net_info == NULL
        || user_dpm_supp_ip_info == NULL
        || user_dpm_supp_conn_info == NULL
        || user_dpm_supp_key_info == NULL)
    {
        PRINTF("[DPM] Retention memory address doesn't assigned it.\n");
        return;
    }

    PRINTF("\n=== Retention Memory informat for DPM ====\n");

    PRINTF(" --- Connected AP Informat ---------------\n");
    PRINTF("  ssid               = %s \n", user_dpm_supp_conn_info->ssid);
    PRINTF("  bssid              = %02x:%02x:%02x:%02x:%02x:%02x \n",
            user_dpm_supp_conn_info->bssid[0],
            user_dpm_supp_conn_info->bssid[1],
            user_dpm_supp_conn_info->bssid[2],
            user_dpm_supp_conn_info->bssid[3],
            user_dpm_supp_conn_info->bssid[4],
            user_dpm_supp_conn_info->bssid[5]);

    PRINTF("\n --- Config SSID Key informat -------------\n");
    PRINTF("  wep_tx_keyidx      = %d\n", user_dpm_supp_key_info->wep_tx_keyidx);
    PRINTF("  wep_key_len        = %d\n", user_dpm_supp_key_info->wep_key_len);
    PRINTF("  ptk.wpa_alg        = %d\n", user_dpm_supp_key_info->ptk.wpa_alg);
    PRINTF("  gtk.wpa_alg        = %d\n", user_dpm_supp_key_info->gtk.wpa_alg);
    PRINTF("  proto              = %d\n", user_dpm_supp_key_info->proto);
    PRINTF("  key_mgmt           = %d\n", user_dpm_supp_key_info->key_mgmt);
    PRINTF("  pairwise_cipher    = %d\n", user_dpm_supp_key_info->pairwise_cipher);
    PRINTF("  group_cipher       = %d\n", user_dpm_supp_key_info->group_cipher);

    PRINTF("\n --- DHCP Network Informat ---------------\n");
    PRINTF("  wifi_mode          = %d\n", user_dpm_supp_net_info->wifi_mode);
    PRINTF("  country_code       = %s\n", user_dpm_supp_net_info->country);
    PRINTF("  net_mode           = %s\n", user_dpm_supp_net_info->net_mode == 1 ? "DHCPC" : "STATIC");


    PRINTF("\n --- IP Informat --------------------------\n");
    PRINTF("  ip_addr            = %d.%d.%d.%d\n",
            (user_dpm_supp_ip_info->dpm_ip_addr      ) & 0xff,
            (user_dpm_supp_ip_info->dpm_ip_addr >>  8) & 0xff,
            (user_dpm_supp_ip_info->dpm_ip_addr >> 16) & 0xff,
            (user_dpm_supp_ip_info->dpm_ip_addr >> 24) & 0xff);

    PRINTF("  netmask            = %d.%d.%d.%d\n",
            (user_dpm_supp_ip_info->dpm_netmask      ) & 0xff,
            (user_dpm_supp_ip_info->dpm_netmask >>  8) & 0xff,
            (user_dpm_supp_ip_info->dpm_netmask >> 16) & 0xff,
            (user_dpm_supp_ip_info->dpm_netmask >> 24) & 0xff);

    PRINTF("  gateway            = %d.%d.%d.%d\n",
            (user_dpm_supp_ip_info->dpm_gateway      ) & 0xff,
            (user_dpm_supp_ip_info->dpm_gateway >>  8) & 0xff,
            (user_dpm_supp_ip_info->dpm_gateway >> 16) & 0xff,
            (user_dpm_supp_ip_info->dpm_gateway >> 24) & 0xff);

    PRINTF("  dns_addr #1        = %d.%d.%d.%d\n",
            (user_dpm_supp_ip_info->dpm_dns_addr[0]      ) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[0] >>  8) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[0] >> 16) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[0] >> 24) & 0xff);

    if (user_dpm_supp_ip_info->dpm_dns_addr[1] > 0) {
        PRINTF("  dns_addr #2        = %d.%d.%d.%d\n",
            (user_dpm_supp_ip_info->dpm_dns_addr[1]      ) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[1] >>  8) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[1] >> 16) & 0xff,
            (user_dpm_supp_ip_info->dpm_dns_addr[1] >> 24) & 0xff);
    }

    PRINTF("  dhcp_xid           = %x\n",        user_dpm_supp_ip_info->dpm_dhcp_xid);
    PRINTF("  lease              = %d secs\n",    user_dpm_supp_ip_info->dpm_lease / 100);
    PRINTF("  renewal            = %d secs\n",    user_dpm_supp_ip_info->dpm_renewal / 100);
    PRINTF("  dpm_rebind         = %d secs\n",    user_dpm_supp_ip_info->dpm_timeout / 100);
}

int chk_abnormal_wakeup(void)
{
    extern UCHAR get_last_abnormal_act(void);

    if (dpm_mode_is_wakeup() && get_last_abnormal_act() > 0) {
        return pdTRUE;
    }

    return pdFALSE;
}

#endif /* __SUPPORT_DPM_CMD__ */

/* EOF */
