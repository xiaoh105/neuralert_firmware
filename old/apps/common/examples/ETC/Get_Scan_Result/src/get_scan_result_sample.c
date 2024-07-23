/**
 ****************************************************************************************
 *
 * @file get_scan_result_sample.c
 *
 * @brief Sample application how to get Wi-Fi Scan result
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

#if defined	(__GET_SCAN_RESULT_SAMPLE__)

#include "da16x_system.h"
#include "util_api.h"

extern void get_scan_result(void *user_buf_ptr);

/*
 * SCAN Result in use_buffer_ptr
 *
 *
 01) SSID: N_A1004_OPEN, RSSI: -14, Security: 0
 02) SSID: N_A1004_WPAx-PSK, RSSI: -14, Security: 1
 03) SSID: N_A1004_WEP_ê³µìœ ëª¨ë“œ, RSSI: -15, Security: 1
 04) SSID: N_A1004_W1_AES_ìœ í‹°ì˜ˆí”„8, RSSI: -17, Security: 1
 05) SSID: ZIO-2509N, RSSI: -29, Security: 1
 06) SSID: Synology_RT2600AC_NPG, RSSI: -30, Security: 0
 07) SSID: TENDA_FH456, RSSI: -33, Security: 1
 08) SSID: N_A3004_W2_AES, RSSI: -33, Security: 1
 09) SSID: Google_NLS1304A_NPG, RSSI: -34, Security: 1
 10) SSID: JMC_SWR-1100, RSSI: -36, Security: 1
 11) SSID: JMC_SWR-1100_OPEN, RSSI: -36, Security: 0
 12) SSID: N_A3004_W1_TKIP_ìœ í‹°ì˜ˆí”„8, RSSI: -37, Security: 1
 13) SSID: N_A3004_WEP__ìœ í‹°ì˜ˆí”„8, RSSI: -37, Security: 1
 14) SSID: JMC_DIR-615_OPEN, RSSI: -37, Security: 0
 15) SSID: in-test, RSSI: -38, Security: 1
 16) SSID: Tenda_W311R, RSSI: -38, Security: 1
 17) SSID: N_A3004_OPEN, RSSI: -38, Security: 0
 18) SSID: [Hidden], RSSI: -39, Security: 1
 19) SSID: JMC_DIR-615_Wx_AUTO, RSSI: -39, Security: 1
 20) SSID: NETGEAR_R7000_SG, RSSI: -40, Security: 1
 21) SSID: TPLINK_ArcherC7_MIKE, RSSI: -40, Security: 0
 22) SSID: JMC_DIR-615_Wx_AUTO5, RSSI: -41, Security: 1
 23) SSID: JMC_DIR-615_Wx_AUTO6, RSSI: -41, Security: 1
 24) SSID: N_N804V_W2_AES_ENT-Linux, RSSI: -41, Security: 1
 25) SSID: DLINK_880L, RSSI: -41, Security: 1
 26) SSID: IPTIME_N8004_CCMP, RSSI: -41, Security: 1
 */

/*
 * Sample to get scan_result
 */
void get_scan_result_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    char	*user_buf = NULL;
    scan_result_t	*scan_result;
    int     i;

    /* Allocate buffer to get scan result */
    user_buf = pvPortMalloc(SCAN_RSP_BUF_SIZE);

    /* Get scan result */
    get_scan_result((void *) user_buf);

    /* Display result on console */
    scan_result = (scan_result_t *)user_buf;

    PRINTF("\n>>> Scanned AP List (Total : %d) \n", scan_result->ssid_cnt);

    for (i = 0; i < scan_result->ssid_cnt; i++) {
        PRINTF(" %02d) SSID: %s, RSSI: %d, Security: %d\n",
               i + 1,
               scan_result->scanned_ap_info[i].ssid,
               scan_result->scanned_ap_info[i].rssi,
               scan_result->scanned_ap_info[i].auth_mode) ;
    }

    /* Buffer free */
    vPortFree(user_buf);
    vTaskDelete(NULL);
}
#endif	// (__GET_SCAN_RESULT_SAMPLE__)

/* EOF */
