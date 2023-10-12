/**
 ****************************************************************************************
 *
 * @file sample_preconfig.c
 *
 * @brief configure parameters for Sample start
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

#include "common_config.h"
#include "da16x_system.h"
#include "util_api.h"


/*
 * Sample for Customer's Wi-Fi configuration
 */
#define	SAMPLE_AP_SSID			"TEST_AP_SSID"
#define	SAMPLE_AP_PSK			"12345678"

// CC_VAL_AUTH_OPEN, CC_VAL_AUTH_WEP, CC_VAL_AUTH_WPA, CC_VAL_AUTH_WPA2, CC_VAL_AUTH_WPA_AUTO
#define	SAMPLE_AP_AUTH_TYPE		CC_VAL_AUTH_WPA_AUTO

/*
 * Required when WEP security mode
 */
#define	SAMPLE_AP_WEP_INDEX		0

// CC_VAL_ENC_TKIP, CC_VAL_ENC_CCMP, CC_VAL_ENC_AUTO
#define	SAMPLE_AP_ENCRPT_INDEX	CC_VAL_ENC_AUTO



void sample_preconfig(void)
{
    //
    // Need to change as Customer's profile information
    //

#if 0	// Example ...
    char reply[32];

    // Delete existed Wi-Fi profile
    da16x_cli_reply("remove_network 0", NULL, reply);

    // Set new Wi-Fi profile for sample test
    da16x_set_nvcache_int(DA16X_CONF_INT_MODE, 0);
    da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, SAMPLE_AP_SSID);
    da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, SAMPLE_AP_AUTH_TYPE);

    if (SAMPLE_AP_AUTH_TYPE == CC_VAL_AUTH_WEP) {
        da16x_set_nvcache_str(DA16X_CONF_STR_WEP_KEY0 + SAMPLE_AP_WEP_INDEX, SAMPLE_AP_PSK);
        da16x_set_nvcache_int(DA16X_CONF_INT_WEP_KEY_INDEX, SAMPLE_AP_WEP_INDEX);
    } else if (SAMPLE_AP_AUTH_TYPE > CC_VAL_AUTH_WEP) {
        da16x_set_nvcache_str(DA16X_CONF_STR_PSK_0, SAMPLE_AP_PSK);
        da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, SAMPLE_AP_ENCRPT_INDEX);
    }

    // Save new Wi-Fi profile to NVRAM area
    da16x_nvcache2flash();

    vTaskDelay(10);

    // Enable new sample Wi-Fi profile
    da16x_cli_reply("select_network 0", NULL, reply);

#endif	// 0
}


/* EOF */

