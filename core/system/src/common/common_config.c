/**
 ****************************************************************************************
 *
 * @file common_config.c
 *
 * @brief Common APIs
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

#include "project_config.h"

#ifdef XIP_CACHE_BOOT  //rtos

#include "sdk_type.h"

#include "common_def.h"
#include "iface_defs.h"
#include "command_net.h"
#include "da16x_network_common.h"
#include "da16x_sntp_client.h"
#include "da16x_dhcp_server.h"
#include "da16x_dns_client.h"
#include "da16x_ip_handler.h"
#include "da16x_dhcp_client.h"

#include "nvedit.h"
#include "environ.h"
#include "sys_feature.h"

#include "supp_types.h"
#include "supp_common.h"
#include "supp_config.h"

#include "dhcpserver.h"

#include "da16x_dpm_regs.h"
#include "user_dpm.h"
#include "user_dpm_api.h"

#include "supp_def.h"

#include "common_config.h"

#include "sntp.h"
#include "user_nvram_cmd_table.h"

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

static int cc_set_network_str(char *name, int iface, char *val);
static int cc_set_network_int(char *name, int iface, int val);

char tmp_ipaddr[6][16] = {0, };

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
extern void da16x_set_temp_staticip_mode(int mode, int save);
extern int da16x_get_temp_staticip_mode(int iface_flag);
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

int da16x_set_config_str(int name, char *value)
{
    int status = CC_SUCCESS;

    char cmd[64] = {0, };
    char input[66] = {0, };
    char return_str[16] = {0, };
    int tmp;
    int save = 0;

    if (name > DA16X_CONF_STR_MAX)
        return user_set_str(name, value, 0);

    switch (name) {
    case DA16X_CONF_STR_MAC_NVRAM:
        status = writeMACaddress(value, 1);
        if (status) {
            return CC_FAILURE_INVALID;
        }
        break;

    case DA16X_CONF_STR_MAC_SPOOFING:
        status = writeMACaddress(value, 0);
        if (status) {
            return CC_FAILURE_INVALID;
        }
        break;

    case DA16X_CONF_STR_MAC_OTP:
        status = writeMACaddress(value, 2);
        if (status) {
            return CC_FAILURE_INVALID;
        }
        break;

    case DA16X_CONF_STR_SSID_0:
    case DA16X_CONF_STR_SSID_1:
        if (strlen(value) == 0 || strlen(value) > 32) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        status = cc_set_network_str("ssid", name - DA16X_CONF_STR_SSID_0, input);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_STR_WEP_KEY0:
    case DA16X_CONF_STR_WEP_KEY1:
    case DA16X_CONF_STR_WEP_KEY2:
    case DA16X_CONF_STR_WEP_KEY3:
        if (strlen(value) == 5 || strlen(value) == 13) {
            sprintf(input, "'%s'", value);
        } else if (strlen(value) == 10 || strlen(value) == 26) {
            strcpy(input, value);
        } else {
            return CC_FAILURE_STRING_LENGTH;
        }

        tmp = name - DA16X_CONF_STR_WEP_KEY0;
        sprintf(cmd, "wep_key%d", tmp);
        status = cc_set_network_str(cmd, 0, input);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_STR_PSK_0:
    case DA16X_CONF_STR_PSK_1:
        if (strlen(value) < 8 || strlen(value) > 63)
            return CC_FAILURE_STRING_LENGTH;

        sprintf(input, "'%s'", value);
        status = cc_set_network_str("psk", name - DA16X_CONF_STR_PSK_0, input);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;
        
    case DA16X_CONF_STR_EAP_IDENTITY:
        if (strlen(value) > 64) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        status = cc_set_network_str("identity", 0, input);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;
        
    case DA16X_CONF_STR_EAP_PASSWORD:
        if (strlen(value) > 64)
            return CC_FAILURE_STRING_LENGTH;

        sprintf(input, "'%s'", value);
        status = cc_set_network_str("password", 0, input);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_STR_COUNTRY:
    {
        int rec = 0;

        if (strlen(value) != 2 && strlen(value) != 3) {
            return CC_FAILURE_STRING_LENGTH;
        }

        if (getSysMode() == 1) {
            da16x_get_config_int(DA16X_CONF_INT_FREQUENCY, &tmp);
            if (chk_channel_by_country(value, tmp, 1, &rec) == -1)
                return CC_FAILURE_INVALID;
        }

        sprintf(cmd, "country %s", value);
        tmp = da16x_cli_reply(cmd, NULL, return_str);
        if (tmp < 0 || strcmp(return_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        }

        if (getSysMode() == 1 && rec) {
            memset(cmd, '\0', 64);
            sprintf(cmd, "set_network 1 frequency %d", rec);
            tmp = da16x_cli_reply(cmd, NULL, return_str);
            if (tmp < 0 || strcmp(return_str, "FAIL") == 0)
                return CC_FAILURE_INVALID;
        }

        save = 1;
    }
        break;

    case DA16X_CONF_STR_DEVICE_NAME:
        if (strlen(value) == 0 || strlen(value) > 64) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(cmd, "device_name %s", value);
        tmp = da16x_cli_reply(cmd, NULL, return_str);
        if (tmp < 0 || strcmp(return_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_STR_IP_0:
    case DA16X_CONF_STR_NETMASK_0:
    case DA16X_CONF_STR_GATEWAY_0:
    {
        char *tmp_ip;

        tmp_ip = read_nvram_string(NVR_KEY_IPADDR_0);    // IP address
        strcpy(tmp_ipaddr[0], tmp_ip);

        tmp_ip = read_nvram_string(NVR_KEY_NETMASK_0);    // Netmask
        strcpy(tmp_ipaddr[1], tmp_ip);

        tmp_ip = read_nvram_string(NVR_KEY_GATEWAY_0);    // GW address
        strcpy(tmp_ipaddr[2], tmp_ip);

        if (name == DA16X_CONF_STR_IP_0) {
            if (!is_in_valid_ip_class(value)) {
                return CC_FAILURE_INVALID;
            }

            write_nvram_int(NVR_KEY_NETMODE_0, 2);
            write_nvram_string(NVR_KEY_IPADDR_0, value);
            strcpy(tmp_ipaddr[0], value);
        } else if (name == DA16X_CONF_STR_NETMASK_0) {
            if (!isvalidmask(value)) {
                return CC_FAILURE_INVALID;
            }

            write_nvram_int(NVR_KEY_NETMODE_0, 2);
            write_nvram_string(NVR_KEY_NETMASK_0, value);
            strcpy(tmp_ipaddr[1], value);
        } else {
            if (!is_in_valid_ip_class(value)) {
                return CC_FAILURE_INVALID;
            }
            
            write_nvram_int(NVR_KEY_NETMODE_0, 2);
            write_nvram_string(NVR_KEY_GATEWAY_0, value);
            strcpy(tmp_ipaddr[2], value);
        }
        ip_change(0, tmp_ipaddr[0], tmp_ipaddr[1], tmp_ipaddr[2], 0);
    }
        break;
    
    case DA16X_CONF_STR_IP_1:
    case DA16X_CONF_STR_NETMASK_1:
    case DA16X_CONF_STR_GATEWAY_1:
    {
        char *tmp_ip;
 
        tmp_ip = read_nvram_string(NVR_KEY_IPADDR_1);
        strcpy(tmp_ipaddr[3], tmp_ip);
        tmp_ip = read_nvram_string(NVR_KEY_NETMASK_1);
        strcpy(tmp_ipaddr[4], tmp_ip);
        tmp_ip = read_nvram_string(NVR_KEY_GATEWAY_1);
        strcpy(tmp_ipaddr[5], tmp_ip);

        if (name == DA16X_CONF_STR_IP_1) {
            if (!is_in_valid_ip_class(value)) {
                return CC_FAILURE_INVALID;
            }

            write_nvram_int(NVR_KEY_NETMODE_1, 2);
            write_nvram_string(NVR_KEY_IPADDR_1, value);
            strcpy(tmp_ipaddr[3], value);
        } else if (name == DA16X_CONF_STR_NETMASK_1) {
            if (!isvalidmask(value)) {
                return CC_FAILURE_INVALID;
            }

            write_nvram_int(NVR_KEY_NETMODE_1, 2);
            write_nvram_string(NVR_KEY_NETMASK_1, value);
            strcpy(tmp_ipaddr[4], value);
        } else {
            if (!is_in_valid_ip_class(value)) {
                return CC_FAILURE_INVALID;
            }

            write_nvram_int(NVR_KEY_NETMODE_1, 2);
            write_nvram_string(NVR_KEY_GATEWAY_1, value);
            strcpy(tmp_ipaddr[5], value);
        }
        ip_change(1, tmp_ipaddr[3], tmp_ipaddr[4], tmp_ipaddr[5], 0);
    }
        break;

    case DA16X_CONF_STR_DNS_0:
    case DA16X_CONF_STR_DNS_1:
        if (is_in_valid_ip_class(value) == pdFALSE){
            return CC_FAILURE_INVALID;
        }

        if (get_netmode(0) == DHCPCLIENT) {
            // saving to nvram only (restart needed to take effect)
            write_nvram_string(NVR_KEY_DNSSVR_0, value);
            break;
        }
        
        set_dns_addr(name - DA16X_CONF_STR_DNS_0, value);
        break;


    case DA16X_CONF_STR_DNS_2ND:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        if (get_netmode(0) == DHCPCLIENT) {
            // saving to nvram only (restart needed to take effect)
            write_nvram_string(NVR_KEY_DNSSVR_0_2ND, value);
            break;
        }
        
        set_dns_addr_2nd(0, value);
        break;

    case DA16X_CONF_STR_DHCP_START_IP:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_nvram_string(DHCP_SERVER_START_IP, value);
        break;

    case DA16X_CONF_STR_DHCP_END_IP:
    {
        char *ret = NULL;

        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        ret = read_nvram_string(DHCP_SERVER_START_IP);
        if (ret == NULL){
            return CC_FAILURE_NOT_READY;
        }
        
        if (set_range_ip_address_list(ret, value, 4)) {
            return CC_FAILURE_INVALID;
        }

        dhcps_set_ip_range(ret, value);
    }
        break;
    
    case DA16X_CONF_STR_DHCP_DNS:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        set_dns_information(value, 4);

        break;

#if defined ( __USER_DHCP_HOSTNAME__ )
    case DA16X_CONF_STR_DHCP_HOSTNAME:
    {
        int    str_len = strlen(value);

        if (str_len > 0) {
            char    ch;

            // Check DHCP hostname validity : 0 .. 9, a .. z, A .. Z, -
            for (int i = 0; i < str_len; i++) {
                ch = value[i];
    
                if (   (ch == '-')
                    || (ch >= '0' && ch <= '9')
                    || (ch >= 'a' && ch <= 'z')
                    || (ch >= 'A' && ch <= 'Z') ) {
                    // Okay,,, next character...
                    continue;
                } else {
                    return CC_FAILURE_INVALID;
                }
            }

            // Save hostname to NVRAM
            status = write_nvram_string(NVR_DHCPC_HOSTNAME, value);                
        } else {
            // Delete hostname to NVRAM
            status = delete_nvram_env(NVR_DHCPC_HOSTNAME);
        }

        if (status == -1)
            return CC_FAILURE_INVALID;

        status = CC_SUCCESS;
    }

        break;
#endif    // __USER_DHCP_HOSTNAME__

    case DA16X_CONF_STR_SNTP_SERVER_IP:
        if (sntp_support_flag == pdTRUE) {
            set_sntp_server((unsigned char *)value, 0);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_1_IP:
        if (sntp_support_flag == pdTRUE) {
            set_sntp_server((UCHAR *)value, 1);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_2_IP:
        if (sntp_support_flag == pdTRUE) {
            set_sntp_server((UCHAR *)value, 2);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;
    case DA16X_CONF_STR_DBG_TXPWR:
        if (strlen(value)) {
            status = write_nvram_string(NVR_KEY_DBG_TXPWR, value);                
        } else {
            status = delete_nvram_env(NVR_KEY_DBG_TXPWR);
        }

        if (status == -1) {
            return CC_FAILURE_INVALID;
        }

        status = CC_SUCCESS;
        break;

    default:
        return CC_FAILURE_NOT_SUPPORTED;
    }
    
    if (save)
        da16x_cli_reply("save_config", NULL, NULL);

    return status;
}

int da16x_set_config_int(int name, int value)
{
    int status = CC_SUCCESS;

    char cmd[64] = {0, };
    char input[64] = {0, };
    char result_str[16] = {0, };
    int save = 0;

    if (name > DA16X_CONF_INT_MAX) {
        return user_set_int(name, value, 0);
    }

    switch (name) {
    case DA16X_CONF_INT_MODE:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
        if (   value < SYSMODE_STA_ONLY
  #ifdef __SUPPORT_P2P__
            || value > SYSMODE_STA_N_P2P
  #else
            || value > SYSMODE_STA_N_AP
  #endif /* __SUPPORT_P2P__ */
           ) {
            return CC_FAILURE_RANGE_OUT;
        }
#else
        if (value < SYSMODE_STA_ONLY || value > SYSMODE_AP_ONLY) {
            return CC_FAILURE_RANGE_OUT;
        }
#endif    // __SUPPORT_WIFI_CONCURRENT__

        write_nvram_int(ENV_SYS_MODE, value);
        break;

    case DA16X_CONF_INT_AUTH_MODE_0:
    case DA16X_CONF_INT_AUTH_MODE_1:
        switch (value) {
        case CC_VAL_AUTH_OPEN:
            strcpy(input, "NONE");
            status = cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, input);
            if (status == CC_SUCCESS) {
                if (name == DA16X_CONF_INT_AUTH_MODE_0)
                    delete_nvram_env(NVR_KEY_AUTH_TYPE_0);
                else
                    delete_nvram_env(NVR_KEY_AUTH_TYPE_1);
            }
            break;
        case CC_VAL_AUTH_WEP:
            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                strcpy(input, "NONE");
                status = cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, input);
                if (status == CC_SUCCESS)
                    delete_nvram_env(NVR_KEY_AUTH_TYPE_0);
            } else {
                return CC_FAILURE_NOT_SUPPORTED;
            }
            break;

        case CC_VAL_AUTH_WPA:
        case CC_VAL_AUTH_WPA2:
        case CC_VAL_AUTH_WPA_AUTO:
            strcpy(input, key_mgmt_WPA_PSK);
            status = cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, input);
            if (status)
                return status;

            if (value == CC_VAL_AUTH_WPA) {
                strcpy(input, proto_WPA);
                status = cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            } else if (value == CC_VAL_AUTH_WPA2) {
                strcpy(input, proto_RSN);
                status = cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            } else {
                strcpy(input, "WPA RSN");
                status = cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            }
            break;

        case CC_VAL_AUTH_WPA_EAP:
        case CC_VAL_AUTH_WPA2_EAP:
        case CC_VAL_AUTH_WPA_AUTO_EAP:
            if (name == DA16X_CONF_INT_AUTH_MODE_1)
                return CC_FAILURE_NOT_SUPPORTED;

            strcpy(input, key_mgmt_WPA_EAP);
            status = cc_set_network_str("key_mgmt", 0, input);
            if (status)
                return status;

            if (value == CC_VAL_AUTH_WPA_EAP) {
                strcpy(input, proto_WPA);
                status = cc_set_network_str("proto", 0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            } else if (value == CC_VAL_AUTH_WPA2_EAP) {
                strcpy(input, proto_RSN);
                status = cc_set_network_str("proto", 0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            } else {
                strcpy(input, "WPA RSN");
                status = cc_set_network_str("proto", 0, input);
                if (status == CC_SUCCESS)
                    save = 1;
            }
            break;
        default:
            return CC_FAILURE_RANGE_OUT;
        }
        break;

    case DA16X_CONF_INT_WEP_KEY_INDEX:
        if (value < 0 || value > 3) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("wep_tx_keyidx", 0, value);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_ENCRYPTION_0:
    case DA16X_CONF_INT_ENCRYPTION_1:
        switch (value) {
        case CC_VAL_ENC_TKIP:
            strcpy(input, "TKIP");
            status = cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);
            if (status == CC_SUCCESS) {
                save = 1;
            }
            break;
        case CC_VAL_ENC_CCMP:
            strcpy(input, "CCMP");
            status = cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);
            if (status == CC_SUCCESS) {
                save = 1;
            }
            break;
        case CC_VAL_ENC_AUTO:
            strcpy(input, "TKIP CCMP");
            status = cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);
            if (status == CC_SUCCESS) {
                save = 1;
            }
            break;
        default:
            return CC_FAILURE_RANGE_OUT;
        }
        break;

    case DA16X_CONF_INT_WIFI_MODE_0:
    case DA16X_CONF_INT_WIFI_MODE_1:
        if (value < 0 || value > CC_VAL_WFMODE_B) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("wifi_mode", name - DA16X_CONF_INT_WIFI_MODE_0, value);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_CHANNEL:
        da16x_get_config_str(DA16X_CONF_STR_COUNTRY, result_str);

        if (value < 0) {
            return CC_FAILURE_INVALID;
        }
    
        if (chk_channel_by_country(result_str, value, 0, NULL) < 0)
            return CC_FAILURE_RANGE_OUT;

        status = cc_set_network_int("channel", 1, value);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_FREQUENCY:
        da16x_get_config_str(DA16X_CONF_STR_COUNTRY, result_str);
        if (chk_channel_by_country(result_str, value, 1, NULL) < 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("frequency", 1, value);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_ROAM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            status = da16x_cli_reply("roam run", NULL, result_str);
        } else if (value == CC_VAL_DISABLE) {
            status = da16x_cli_reply("roam stop", NULL, result_str);
        }

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_ROAM_THRESHOLD:
        if (value < -95 || value > 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "roam_threshold %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_FIXED_ASSOC_CH:
        if (value < 0 || value >= 14) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == 0) {
            status = delete_nvram_env(NVR_KEY_ASSOC_CH);
            if (status == FALSE) {
                status = CC_FAILURE_UNKNOWN;
            } else {
                status = CC_SUCCESS;
            }
        } else {
            status = write_nvram_int(NVR_KEY_ASSOC_CH, value);
        }

        if (status < 0) {
            return CC_FAILURE_UNKNOWN;
        }


        break;

    case DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2:
        if (value < 0 || value > 1) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == 0) {
            status = delete_nvram_env(NVR_KEY_FST_CONNECT);
            if (status == FALSE) {
                status = CC_FAILURE_UNKNOWN;
            } else {
                delete_nvram_env(NVR_KEY_NETMODE_0);
                delete_nvram_env(NVR_KEY_IPADDR_0);
                delete_nvram_env(NVR_KEY_NETMASK_0);
                delete_nvram_env(NVR_KEY_GATEWAY_0);
                delete_nvram_env(NVR_KEY_DNSSVR_0);
                delete_nvram_env(NVR_KEY_PSK_RAW_0);
                delete_nvram_env(NVR_KEY_ASSOC_CH);
                status = CC_SUCCESS;
            }
        } else {
            status = write_nvram_int(NVR_KEY_FST_CONNECT, value);
        }

        if (status < 0) {
            return CC_FAILURE_UNKNOWN;
        }

        status = CC_SUCCESS;
        break;

    case DA16X_CONF_INT_STA_PROF_DISABLED:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("disabled", 0, value);
        if (status == CC_SUCCESS)
            save = 1;
        break;

    case DA16X_CONF_INT_EAP_PHASE1:
        if (value < 0 || value > CC_VAL_EAP_TLS) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_EAP_DEFAULT) {
            status = cc_set_network_str("eap", 0, "PEAP TTLS FAST");
        } else if (value == CC_VAL_EAP_PEAP0) {
            status = cc_set_network_str("eap", 0, "PEAP");
            da16x_cli_reply("peap_ver 0", NULL, NULL);
        } else if (value == CC_VAL_EAP_PEAP1) {
            status = cc_set_network_str("eap", 0, "PEAP");
            da16x_cli_reply("peap_ver 1", NULL, NULL);
        } else if (value == CC_VAL_EAP_TTLS) {
            status = cc_set_network_str("eap", 0, "TTLS");
        } else if (value == CC_VAL_EAP_FAST) {
            status = cc_set_network_str("eap", 0, "FAST");
            cc_set_network_str("phase1", 0, "'fast_provisioning=2'");
        } else {
            status = cc_set_network_str("eap", 0, "TLS");
        }

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_EAP_PHASE2:
        if (value < 0 || value > CC_VAL_EAP_GTC) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_EAP_PHASE2_MIX) {
            status = cc_set_network_str("phase2", 0, "'auth=MSCHAPV2 GTC'");
        } else if (value == CC_VAL_EAP_MSCHAPV2) {
            status = cc_set_network_str("phase2", 0, "'auth=MSCHAPV2'");
        } else {
            status = cc_set_network_str("phase2", 0, "'auth=GTC'");
        }

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

#ifdef    CONFIG_AP
    case DA16X_CONF_INT_BEACON_INTERVAL:
        if (value < 20 || value > 1000) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("beacon_int", 1, value);
        if (status == CC_SUCCESS) {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_INACTIVITY:
        if ((value != 0 && (value < 30 || value > 86400)) || (value % 10 != 0)) {
            PRINTF("Invalid Cmd\n"
                 "Timeout: 30 ~ 86400 secs (step=10)\n"
                 "(If 0, use default - 300 secs)\n");
            return CC_FAILURE_RANGE_OUT;
        }
        
        sprintf(cmd, "ap_max_inactivity %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_RTS_THRESHOLD:
        if (value <= 0 || value > 2347) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "ap_rts %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_WMM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "wmm_enabled %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;
        
    case DA16X_CONF_INT_WMM_PS:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "wmm_ps_enabled %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;
        
#endif    /* CONFIG_AP */

#if !defined ( __LIGHT_SUPPLICANT__ )
#ifdef __SUPPORT_P2P__
    case DA16X_CONF_INT_P2P_OPERATION_CHANNEL:
        if (value != 0 && value != 1 && value != 6 && value != 11) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "p2p_set oper_channel %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_P2P_LISTEN_CHANNEL:
        if (value != 0 && value != 1 && value != 6 && value != 11) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "p2p_set listen_channel %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_P2P_GO_INTENT:
        if (value < 0 || value > 15) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "p2p_set go_intent %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;

    case DA16X_CONF_INT_P2P_FIND_TIMEOUT:
        if (value <= 0 || value > 86400) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(cmd, "p2p_set find_timeout %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        } else {
            save = 1;
        }
        break;
#endif /* __SUPPORT_P2P__ */
#endif    // __LIGHT_SUPPLICANT__

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
    case DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP:
        da16x_set_temp_staticip_mode(value, pdTRUE);

        if (value == pdFALSE) {
            break;
        }
        // no break
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

    case DA16X_CONF_INT_DHCP_CLIENT:
    {
        UINT iface = WLAN0_IFACE;
        struct netif *netif = netif_get_by_index(iface+2);
        err_t err = ERR_OK;

        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            if (chk_supp_connected()) {
                err = dhcp_start(netif);

                if (err == ERR_OK) {
                    PRINTF("\nDHCP Client Started WLAN%u.\n", iface);
                    
                } else {
                    PRINTF("\nDHCP Client Start Error(%d) WLAN%u.\n", err, iface);
                    return CC_FAILURE_NOT_READY;
                }
            }    
            set_netmode(0, DHCPCLIENT, pdTRUE);
        } else {
            dhcp_stop(netif);
            set_netmode(0, STATIC_IP, pdTRUE);
        }
        break;
    }

    case DA16X_CONF_INT_DHCP_SERVER:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        {
#if LWIP_DHCPS
            dhcps_cmd_param *param = NULL;

            param = pvPortMalloc(sizeof(dhcps_cmd_param));
            memset(param, 0, sizeof(dhcps_cmd_param));

            if (value == CC_VAL_ENABLE) {
                param->cmd = DHCP_SERVER_STATE_STOP;
                dhcps_run(param);

                vTaskDelay(100);
                
                param = pvPortMalloc(sizeof(dhcps_cmd_param));
                memset(param, 0, sizeof(dhcps_cmd_param));

                param->cmd = DHCP_SERVER_STATE_START;
                param->dhcps_interface = WLAN1_IFACE;
                dhcps_run(param);

                write_nvram_int(NVR_KEY_DHCPD, pdTRUE);
            } else {
                param->cmd = DHCP_SERVER_STATE_STOP;
                dhcps_run(param);
                
                delete_nvram_env(NVR_KEY_DHCPD);
            }
#endif /*LWIP_DHCPS*/
        }
        break;


    case DA16X_CONF_INT_DHCP_LEASE_TIME:
        if (value <= 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        set_lease_time(value, 4);
        break;

    case DA16X_CONF_INT_SNTP_CLIENT:
        if (sntp_support_flag == pdTRUE) {
            
            if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
                return CC_FAILURE_RANGE_OUT;
            }
            
            if (value == CC_VAL_ENABLE && get_sntp_use() == 1) {
                // if sntp is in "running" state
                break;
            }

            set_sntp_use(value); // set run flag and start
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_INT_SNTP_SYNC_PERIOD:
        if (sntp_support_flag == pdTRUE) {
            if (value <= 0) {
                return CC_FAILURE_RANGE_OUT;
            }
            
            if (set_sntp_period(value) == pdFAIL) {
                return CC_FAILURE_RANGE_OUT;
            }
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_INT_TLS_VER:
        if (value < 0 || value > 2) {
            return CC_FAILURE_RANGE_OUT;
        }
        
        sprintf(cmd, "tls_ver %d", value);
        status = da16x_cli_reply(cmd, NULL, result_str);

        if (status < 0 || strcmp(result_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        }
        break;

    case DA16X_CONF_INT_TLS_ROOTCA_CHK:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            write_nvram_int("ROOTCA_CHK", CC_VAL_ENABLE);
        } else {
            delete_nvram_env("ROOTCA_CHK");
        }
        break;

    case DA16X_CONF_INT_GMT:
    {
        int temp;
        extern void da16x_SetTzoff(long offset);

        if (value < -43200 || value > 43200) {
            return CC_FAILURE_RANGE_OUT;
        }

        temp = (value/60)*60;

        set_timezone_to_rtm(temp);
        da16x_SetTzoff(temp);
        set_time_zone(temp);
    }
        break;

    case DA16X_CONF_INT_DPM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            enable_dpm_mode();
        } else {
            disable_dpm_mode();
        }

        break;

#if !defined ( __DISABLE_DPM_ABNORM__ )
    case DA16X_CONF_INT_DPM_ABN :
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            if (config_dpm_abnormal_timer(pdTRUE) == pdFAIL) {
                status = CC_FAILURE_NOT_SUPPORTED;
            } else {
                status = CC_SUCCESS;

                /* Delete flag from NVRAM */
                delete_nvram_env(NVR_KEY_DPM_ABNORM_STOP);
            }
        } else {
            if (config_dpm_abnormal_timer(pdFALSE) == pdFAIL) {
                status = CC_FAILURE_NOT_SUPPORTED;
            } else {
                status = CC_SUCCESS;

                /* Save flag into NVRAM */
                write_nvram_int(NVR_KEY_DPM_ABNORM_STOP, 1);
            }
        }

        break;
#endif // !__DISABLE_DPM_ABNORM__

    default:
        return CC_FAILURE_NOT_SUPPORTED;
    }

    if (save) {
        da16x_cli_reply("save_config", NULL, NULL);
    }

    return status;
}


int da16x_get_config_str(int name, char *value)
{
    int status = CC_SUCCESS;

    char result_str[16] = {0, };
    char *result_ptr = NULL;
#if LWIP_DHCPS
    dhcps_lease_t iprange;
#endif    // LWIP_DHCPS

    if (name > DA16X_CONF_STR_MAX) {
        return user_get_str(name, value);
    }

    if (value == NULL) {
        return CC_FAILURE_NO_ALLOCATION;
    }

    switch (name) {
    case DA16X_CONF_STR_MAC_0: {
        char mac_addr[18] = {0, };

        getMACAddrStr(1, mac_addr);
        memcpy(value, mac_addr, strlen(mac_addr));
    } break;

    case DA16X_CONF_STR_MAC_1: {
        char mac_addr[18] = {0, };

        getMACAddrStr(1, mac_addr);
        mac_addr[16] = mac_addr[16]+1;
        memcpy(value, mac_addr, strlen(mac_addr));
    } break;

    case DA16X_CONF_STR_SCAN:
        status = da16x_cli_reply("scan", NULL, value);
        if (status < 0 || strcmp(value, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        }

        break;

    case DA16X_CONF_STR_SSID_0:
    case DA16X_CONF_STR_SSID_1:
        if (name == DA16X_CONF_STR_SSID_0) {
            result_ptr = read_nvram_string(NVR_KEY_SSID_0);
        } else {
            result_ptr = read_nvram_string(NVR_KEY_SSID_1);
        }

        if (strlen(result_ptr)) {
            memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
        } else {
            return CC_FAILURE_NO_VALUE;
        }
        break;

    case DA16X_CONF_STR_WEP_KEY0:
    case DA16X_CONF_STR_WEP_KEY1:
    case DA16X_CONF_STR_WEP_KEY2:
    case DA16X_CONF_STR_WEP_KEY3:
        if (name == DA16X_CONF_STR_WEP_KEY0) {
            result_ptr = read_nvram_string(NVR_KEY_WEPKEY0_0);
        } else if (name == DA16X_CONF_STR_WEP_KEY1) {
            result_ptr = read_nvram_string(NVR_KEY_WEPKEY1_0);
        } else if (name == DA16X_CONF_STR_WEP_KEY2) {
            result_ptr = read_nvram_string(NVR_KEY_WEPKEY2_0);
        } else {
            result_ptr = read_nvram_string(NVR_KEY_WEPKEY3_0);
        }

        if (strlen(result_ptr)) {
            if (strlen(result_ptr) == 5 + 2 || strlen(result_ptr) == 13 + 2) {
                memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
            } else if (strlen(result_ptr) == 10 || strlen(result_ptr) == 26) {
                strcpy(value, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }
        } else {
            return CC_FAILURE_NO_VALUE;
        }

        break;

    case DA16X_CONF_STR_PSK_0:
    case DA16X_CONF_STR_PSK_1:
        if (name == DA16X_CONF_STR_PSK_0) {
            result_ptr = read_nvram_string(NVR_KEY_ENCKEY_0);
        } else {
            result_ptr = read_nvram_string(NVR_KEY_ENCKEY_1);
        }

        if (strlen(result_ptr)) {
            memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
        } else {
            return CC_FAILURE_NO_VALUE;
        }

        break;

#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
    case DA16X_CONF_STR_SAE_PASS_0:
    case DA16X_CONF_STR_SAE_PASS_1:
        if (name == DA16X_CONF_STR_SAE_PASS_0) {
            result_ptr = read_nvram_string(NVR_KEY_SAE_PASS_0);
        } else {
            result_ptr = read_nvram_string(NVR_KEY_SAE_PASS_1);
        }

        if (strlen(result_ptr)) {
            memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
        } else {
            return CC_FAILURE_NO_VALUE;
        }
        break;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__

    case DA16X_CONF_STR_EAP_IDENTITY:
        result_ptr = read_nvram_string("N0_identity");

        if (strlen(result_ptr)) {
            memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
        } else {
            return CC_FAILURE_NO_VALUE;
        }
        break;

    case DA16X_CONF_STR_EAP_PASSWORD:
        result_ptr = read_nvram_string("N0_password");

        if (strlen(result_ptr)) {
            memcpy(value, result_ptr + 1, strlen(result_ptr) - 2);
        } else {
            return CC_FAILURE_NO_VALUE;
        }
        break;

    case DA16X_CONF_STR_COUNTRY:
        result_ptr = read_nvram_string(NVR_KEY_COUNTRY_CODE);

        if (strlen(result_ptr)) {
            strcpy(value, result_ptr);
        } else {
            strcpy(value, DEFAULT_AP_COUNTRY);
        }
        break;

    case DA16X_CONF_STR_DEVICE_NAME:
        result_ptr = read_nvram_string("dev_name");

        if (strlen(result_ptr)) {
            strcpy(value, result_ptr);
        } else {
            return CC_FAILURE_NO_VALUE;
        }
        break;

    case DA16X_CONF_STR_IP_0:
    case DA16X_CONF_STR_IP_1:
        if (get_netmode(0) == DHCPCLIENT) {
            if (get_ip_info(name == DA16X_CONF_STR_IP_0 ? 0 : 1, GET_IPADDR, result_str) == pdPASS) {
                strcpy(value, result_str);
            } else {
                return CC_FAILURE_INVALID;
            }
        } else {
            if (name == DA16X_CONF_STR_IP_0) {
                result_ptr = read_nvram_string(NVR_KEY_IPADDR_0);
            } else {
                result_ptr = read_nvram_string(NVR_KEY_IPADDR_1);
            }
            
            if (strlen(result_ptr)) {
                strcpy(value, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }
        }
        break;

    case DA16X_CONF_STR_NETMASK_0:
    case DA16X_CONF_STR_NETMASK_1:
        if (get_netmode(0) == DHCPCLIENT) {
            if (get_ip_info(name == DA16X_CONF_STR_NETMASK_0 ? 0 : 1, GET_SUBNET, result_str) == pdPASS) {
                strcpy(value, result_str);
            } else {
                return CC_FAILURE_INVALID;
            }
        } else {
            if (name == DA16X_CONF_STR_NETMASK_0) {
                result_ptr = read_nvram_string(NVR_KEY_NETMASK_0);
            } else {
                result_ptr = read_nvram_string(NVR_KEY_NETMASK_1);
            }
            
            if (strlen(result_ptr)) {
                strcpy(value, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }
        }
        break;

    case DA16X_CONF_STR_GATEWAY_0:
    case DA16X_CONF_STR_GATEWAY_1:
        if (get_netmode(0) == DHCPCLIENT) {
            if (get_ip_info(name == DA16X_CONF_STR_GATEWAY_0 ? 0 : 1, GET_GATEWAY, result_str) == pdPASS) {
                strcpy(value, result_str);
            } else {
                return CC_FAILURE_INVALID;
            }
        } else {
            if (name == DA16X_CONF_STR_GATEWAY_0) {
                result_ptr = read_nvram_string(NVR_KEY_GATEWAY_0);
            } else {
                result_ptr = read_nvram_string(NVR_KEY_GATEWAY_1);
            }

            if (strlen(result_ptr)) {
                strcpy(value, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }
        }
        break;

    case DA16X_CONF_STR_DNS_0:
    case DA16X_CONF_STR_DNS_1:
        if (get_netmode(0) == DHCPCLIENT) {
            if (   (get_ip_info(name == DA16X_CONF_STR_DNS_0 ? 0 : 1, GET_DNS, result_str) == pdPASS)
                && strcmp(result_str, "0.0.0.0") != 0) {
                strcpy(value, result_str);
            } else {
                return CC_FAILURE_INVALID;
            }
        } else {
            if (name == DA16X_CONF_STR_DNS_0) {
                result_ptr = read_nvram_string(NVR_KEY_DNSSVR_0);
            } else {
                result_ptr = read_nvram_string(NVR_KEY_DNSSVR_1);
            }
            
            if (strlen(result_ptr)) {
                strcpy(value, result_ptr);
            } else {
                strcpy(value, DEFAULT_DNS_WLAN0);
            }
        }
        break;

    case DA16X_CONF_STR_DNS_2ND:
        if (get_netmode(0) == DHCPCLIENT) {
            if ((get_ip_info(0, GET_DNS_2ND, result_str) == pdPASS) && strcmp(result_str, "0.0.0.0") != 0) {
                strcpy(value, result_str);
            } else {
                return CC_FAILURE_INVALID;
            }
        } else {
            result_ptr = read_nvram_string(NVR_KEY_DNSSVR_0_2ND);
            if (strlen(result_ptr)) {
                strcpy(value, result_ptr);
            } else {
                strcpy(value, DEFAULT_DNS_2ND);
            }
        }

        break;

    case DA16X_CONF_STR_DHCP_START_IP:
#if LWIP_DHCPS
        iprange = dhcps_get_ip_range();
        sprintf(value, ipaddr_ntoa(&iprange.start_ip));
#endif /*LWIP_DHCPS*/
        break;

    case DA16X_CONF_STR_DHCP_END_IP:
#if LWIP_DHCPS
        iprange = dhcps_get_ip_range();
        sprintf(value, ipaddr_ntoa(&iprange.end_ip));
#endif /*LWIP_DHCPS*/
        break;

    case DA16X_CONF_STR_DHCP_DNS:
    {
        ULONG utmp;
    
        if (get_dns_information(&utmp, 1)) {
            return CC_FAILURE_INVALID;
        }

        sprintf(value, "%lu.%lu.%lu.%lu",
            (utmp>>24)&0x0ff, (utmp>>16)&0x0ff,
            (utmp>> 8)&0x0ff, (utmp>> 0)&0x0ff);
    }
        break;

#if defined ( __USER_DHCP_HOSTNAME__ )
    case DA16X_CONF_STR_DHCP_HOSTNAME:
        result_ptr = read_nvram_string(NVR_DHCPC_HOSTNAME);

        if (strlen(result_ptr) > 0) {
            strcpy(value, result_ptr);
        } else {
            return CC_FAILURE_NO_VALUE;
        }

        break;
#endif    // __USER_DHCP_HOSTNAME__

    case DA16X_CONF_STR_SNTP_SERVER_IP:
        if (sntp_support_flag == pdTRUE) {
            get_sntp_server(value, 0);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_1_IP:
        if (sntp_support_flag == pdTRUE) {
            get_sntp_server(value, 1);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_2_IP:
        if (sntp_support_flag == pdTRUE) {
            get_sntp_server(value, 2);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_DBG_TXPWR:
        result_ptr = read_nvram_string(NVR_KEY_DBG_TXPWR);

        if (strlen(result_ptr)) {
            strcpy(value, result_ptr);
        } else {
            return CC_FAILURE_NO_VALUE;
        }

        break;

    default:
        return CC_FAILURE_NOT_SUPPORTED;
    }

    return status;
}

int da16x_get_config_int(int name, int *value)
{
    int status = CC_SUCCESS;
    int tmp;

    char result_str[16] = {0, };
    char *result_ptr = NULL;

    if (name > DA16X_CONF_INT_MAX)
        return user_get_int(name, value);

    switch (name) {
    case DA16X_CONF_INT_MODE:
        *value = getSysMode();
        break;

    case DA16X_CONF_INT_AUTH_MODE_0:
        result_ptr = read_nvram_string(NVR_KEY_AUTH_TYPE_0);

        if (result_ptr == NULL) {
            if (read_nvram_int(NVR_KEY_WEPINDEX_0, &tmp) && read_nvram_string(NVR_KEY_WEPKEY0_0) == NULL) {
                *value = CC_VAL_AUTH_OPEN;
            } else {
                *value = CC_VAL_AUTH_WEP;
            }
        } else if (strcmp(result_ptr, "WPA-PSK") == 0) {
            result_ptr = read_nvram_string(NVR_KEY_PROTO_0);
            if (result_ptr == NULL) {
                *value = CC_VAL_AUTH_WPA_AUTO;
            } else if (strcmp(result_ptr, "WPA") == 0) {
                *value = CC_VAL_AUTH_WPA;
            } else {
                *value = CC_VAL_AUTH_WPA2;
            }
        } else if (strcmp(result_ptr, "WPA-EAP") == 0) {
            result_ptr = read_nvram_string(NVR_KEY_PROTO_0);
            if (result_ptr == NULL) {
                *value = CC_VAL_AUTH_WPA_AUTO_EAP;
            } else if (strcmp(result_ptr, "WPA") == 0) {
                *value = CC_VAL_AUTH_WPA_EAP;
            } else {
                *value = CC_VAL_AUTH_WPA2_EAP;
            }
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
        } else if (strcmp(result_ptr, "OWE") == 0) {
            *value = CC_VAL_AUTH_OWE;
        } else if (strcmp(result_ptr, "SAE") == 0) {
            *value = CC_VAL_AUTH_SAE;
        } else if (strcmp(result_ptr, "WPA-PSK SAE") == 0) {
            *value = CC_VAL_AUTH_RSN_SAE;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;
            
        case DA16X_CONF_INT_AUTH_MODE_1:
            result_ptr = read_nvram_string(NVR_KEY_AUTH_TYPE_1);
            if (result_ptr == NULL) {
                *value = CC_VAL_AUTH_OPEN;
            } else if (strcmp(result_ptr, "WPA-PSK") == 0) {
                result_ptr = read_nvram_string(NVR_KEY_PROTO_1);
                if (result_ptr == NULL) {
                    *value = CC_VAL_AUTH_WPA_AUTO;
                } else if (strcmp(result_ptr, "WPA") == 0) {
                    *value = CC_VAL_AUTH_WPA;
                } else {
                    *value = CC_VAL_AUTH_WPA2;
            }
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
            } else if (strcmp(result_ptr, "OWE") == 0) {
                *value = CC_VAL_AUTH_OWE;
            } else if (strcmp(result_ptr, "SAE") == 0) {
                *value = CC_VAL_AUTH_SAE;
            } else if (strcmp(result_ptr, "WPA-PSK SAE") == 0) {
                *value = CC_VAL_AUTH_RSN_SAE;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
            } else {
                return CC_FAILURE_NOT_SUPPORTED;
            }
            break;

        case DA16X_CONF_INT_WEP_KEY_INDEX:
            if (read_nvram_int(NVR_KEY_WEPINDEX_0, &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_ENCRYPTION_0:
            result_ptr = read_nvram_string(NVR_KEY_ENC_TYPE_0);
            if (result_ptr == NULL) {
                *value = CC_VAL_ENC_AUTO;
            } else if (strcmp(result_ptr, "TKIP") == 0) {
                *value = CC_VAL_ENC_TKIP;
            } else {
                *value = CC_VAL_ENC_CCMP;
            }
            break;

        case DA16X_CONF_INT_ENCRYPTION_1:
            result_ptr = read_nvram_string(NVR_KEY_ENC_TYPE_1);
            if (result_ptr == NULL) {
                *value = CC_VAL_ENC_AUTO;
            } else if (strcmp(result_ptr, "TKIP") == 0) {
                *value = CC_VAL_ENC_TKIP;
            } else {
                *value = CC_VAL_ENC_CCMP;
            }
            break;

        case DA16X_CONF_INT_WIFI_MODE_0:
            if (read_nvram_int(NVR_KEY_WLANMODE_0, &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_WIFI_MODE_1:
            if (read_nvram_int(NVR_KEY_WLANMODE_1, &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_CHANNEL:
        {
            extern int i3ed11_freq_to_ch(int);

            if (read_nvram_int(NVR_KEY_CHANNEL, &tmp)) {
                *value = i3ed11_freq_to_ch(FREQUENCE_DEFAULT);
            } else {
                if (tmp) {
                    *value = i3ed11_freq_to_ch(tmp);
                } else {
                    *value = 0;
                }
            }
        }
            break;
            
        case DA16X_CONF_INT_FREQUENCY:
            if (read_nvram_int(NVR_KEY_CHANNEL, &tmp)) {
                *value = FREQUENCE_DEFAULT;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_ROAM:
            if (read_nvram_int(ENV_ROAM, &tmp)) {
                *value = DEFAULT_ROAM_DISABLE;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_ROAM_THRESHOLD:
            if (read_nvram_int(ENV_ROAM_THRESHOLD, &tmp)) {
                *value = ROAM_RSSI_THRESHOLD;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_FIXED_ASSOC_CH:
            if (read_nvram_int(NVR_KEY_ASSOC_CH, &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2:
            if (read_nvram_int(NVR_KEY_FST_CONNECT, &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_STA_PROF_DISABLED:
            if (read_nvram_int("N0_disabled", &tmp)){
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_EAP_PHASE1:
            result_ptr = read_nvram_string("N0_eap");

            if (strlen(result_ptr)) {
                strcpy(result_str, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }

            if (strcmp(result_str, "PEAP TTLS FAST") == 0) {
                *value = CC_VAL_EAP_DEFAULT;
            } else if (strcmp(result_str, "PEAP") == 0) {
                char tmp_reply[2] = {0, };

                da16x_cli_reply("peap_ver", NULL, tmp_reply);

                if (atoi(tmp_reply) == 1) {
                    *value = CC_VAL_EAP_PEAP1;
                } else {
                    *value = CC_VAL_EAP_PEAP0;
                }
            } else if (strcmp(result_str, "TTLS") == 0) {
                *value = CC_VAL_EAP_TTLS;
            } else if (strcmp(result_str, "FAST") == 0) {
                *value = CC_VAL_EAP_FAST;
            } else if (strcmp(result_str, "TLS") == 0) {
                *value = CC_VAL_EAP_TLS;
            } else {
                return CC_FAILURE_UNKNOWN;
            }
            break;
            
        case DA16X_CONF_INT_EAP_PHASE2:
            result_ptr = read_nvram_string("N0_phase2");

            if (strlen(result_ptr)) {
                strcpy(result_str, result_ptr);
            } else {
                return CC_FAILURE_NO_VALUE;
            }
                
            if (strcmp(result_str + 5, "MSCHAPV2 GTC") == 0) {
                *value = CC_VAL_EAP_PHASE2_MIX;
            } else if (strcmp(result_str + 5, "MSCHAPV2") == 0) {
                *value = CC_VAL_EAP_MSCHAPV2;
            } else if (strcmp(result_str + 5, "GTC") == 0) {
                *value = CC_VAL_EAP_GTC;
            } else {
                return CC_FAILURE_UNKNOWN;
            }
            break;

#ifdef    CONFIG_AP
        case DA16X_CONF_INT_BEACON_INTERVAL:
            if (read_nvram_int("N1_beacon_int", &tmp)) {
                *value = 100;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_INACTIVITY:
            if (read_nvram_int(ENV_AP_MAX_INACTIVITY, &tmp)) {
                *value = DEFAULT_AP_MAX_INACTIVITY;
            } else {
                if (tmp == 0) {
                    *value = DEFAULT_AP_MAX_INACTIVITY;
                } else {
                    *value = tmp;
                }
            }
            break;

        case DA16X_CONF_INT_RTS_THRESHOLD:
            if (read_nvram_int(ENV_RTS_THRESHOLD, &tmp)) {
                *value = 2347;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_WMM:
            if (read_nvram_int(ENV_WMM_ENABLED, &tmp)) {
                *value = CC_VAL_ENABLE;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_WMM_PS:
            if (read_nvram_int(ENV_WMM_PS_ENABLED, &tmp)) {
                *value = CC_VAL_DISABLE;
            } else {
                *value = tmp;
            }
            break;
#endif    /* CONFIG_AP */

#if !defined ( __LIGHT_SUPPLICANT__ )
#ifdef __SUPPORT_P2P__
        case DA16X_CONF_INT_P2P_OPERATION_CHANNEL:
            if (read_nvram_int(ENV_P2P_OPER_CHANNEL, &tmp)) {
                *value = DEFAULT_P2P_OPER_CHANNEL;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_P2P_LISTEN_CHANNEL:
            if (read_nvram_int(ENV_P2P_LISTEN_CHANNEL, &tmp)) {
                *value = DEFAULT_P2P_LISTEN_CHANNEL;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_P2P_GO_INTENT:
            if (read_nvram_int(ENV_P2P_GO_INTENT, &tmp)) {
                *value = DEFAULT_P2P_GO_INTENT;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_P2P_FIND_TIMEOUT:
            if (read_nvram_int(ENV_P2P_FIND_TIMEOUT, &tmp)) {
                *value = DEFAULT_P2P_FIND_TIMEOUT;
            } else {
                *value = tmp;
            }
            break;
#endif /* __SUPPORT_P2P__ */
#endif    /* __LIGHT_SUPPLICANT__ */

#ifdef __SUPPORT_NAT__
        case DA16X_CONF_INT_NAT:
            if (read_nvram_int("NAT", &tmp)) {
                return CC_FAILURE_NO_VALUE;
            } else {
                if (tmp == 1) {
                    *value = CC_VAL_ENABLE;
                } else {
                    *value = CC_VAL_DISABLE;
                }
            }
            break;
#endif /* __SUPPORT_NAT__ */

        case DA16X_CONF_INT_DHCP_CLIENT:
            if (read_nvram_int(NVR_KEY_NETMODE_0, &tmp)) {
                *value = CC_VAL_ENABLE;
            } else {
                if (tmp == DHCPCLIENT) {
                    *value = CC_VAL_ENABLE;
                } else {
                    *value = CC_VAL_DISABLE;
                }
            }
            break;

        case DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP:
            if (da16x_get_temp_staticip_mode(WLAN0_IFACE)) {
                *value = CC_VAL_ENABLE;
            } else {
                *value = CC_VAL_DISABLE;
            }
            break;

        case DA16X_CONF_INT_DHCP_SERVER:
            if (read_nvram_int(NVR_KEY_DHCPD, &tmp)) {
                *value = CC_VAL_DISABLE;
            } else {
                if (tmp == 1) {
                    *value = CC_VAL_ENABLE;
                } else {
                    *value = CC_VAL_DISABLE;
                }
            }
            break;

        case DA16X_CONF_INT_DHCP_LEASE_TIME:
            if (read_nvram_int(DHCP_SERVER_LEASE_TIME, &tmp)) {
                *value = 10 * 50; /* NX_DHCP_SLOW_PERIODIC_TIME_INTERVAL == 50 */
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_SNTP_CLIENT:
            *value = get_sntp_use();
            break;

        case DA16X_CONF_INT_SNTP_SYNC_PERIOD:
            *value = get_sntp_period();
            break;

        case DA16X_CONF_INT_TLS_VER:
            if (!read_nvram_int("tls_ver", &tmp)) {
                if (tmp == 769) {
                    *value = 0;
                } else if (tmp == 770) {
                    *value = 1;
                } else {
                    *value = 2;
                }
            } else {
                *value = 2;
            }
            break;

        case DA16X_CONF_INT_TLS_ROOTCA_CHK:
            if (read_nvram_int("ROOTCA_CHK", &tmp)) {
                *value = 0;
            } else {
                *value = tmp;
            }
            break;

        case DA16X_CONF_INT_GMT:
        {
            extern long get_time_zone(void );

            *value = get_time_zone();
        }
            break;

        case DA16X_CONF_INT_DPM:
            if (read_nvram_int(NVR_KEY_DPM_MODE, &tmp)) {
                *value = CC_VAL_DISABLE;
            } else {
                *value = tmp;
            }
            break;

#if !defined ( __DISABLE_DPM_ABNORM__ )
        case DA16X_CONF_INT_DPM_ABN :
            if (read_nvram_int(NVR_KEY_DPM_ABNORM_STOP, &tmp)) {
                /* Not exist NVR_KEY_DPM_ABNORM value in NVRAM */
                *value = CC_VAL_DISABLE;
            } else {
                /* Exist NVR_KEY_DPM_ABNORM value in NVRAM */
                *value = tmp;
            }
        break;
#endif // !__DISABLE_DPM_ABNORM__

        default:
            return CC_FAILURE_NOT_SUPPORTED;
    }

    return status;
}

int da16x_set_nvcache_str(int name, char *value)
{
    int status = CC_SUCCESS;

    char cmd[64] = {0, };
    char input[66] = {0, };
    int tmp;

    if (name > DA16X_CONF_STR_MAX) {
        return user_set_str(name, value, 1);
    }

    switch (name) {
    case DA16X_CONF_STR_MAC_NVRAM:
    case DA16X_CONF_STR_MAC_SPOOFING:
    case DA16X_CONF_STR_MAC_OTP:
        status = CC_FAILURE_NOT_SUPPORTED;
        break;

    case DA16X_CONF_STR_SSID_0:
    case DA16X_CONF_STR_SSID_1:
        if (strlen(value) == 0 || strlen(value) > 32) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        cc_set_network_str("ssid", name - DA16X_CONF_STR_SSID_0, input);

        sprintf(input, "\"%s\"", value);
        if (name == DA16X_CONF_STR_SSID_0) {
            write_tmp_nvram_int(NVR_KEY_PROFILE_0, 1);
            write_tmp_nvram_string(NVR_KEY_SSID_0, input);
        } else {
            write_tmp_nvram_int(NVR_KEY_PROFILE_1, 1);
            write_tmp_nvram_int(NVR_KEY_MODE_1, 2);
            write_tmp_nvram_string(NVR_KEY_SSID_1, input);
        }
        break;

    case DA16X_CONF_STR_WEP_KEY0:
    case DA16X_CONF_STR_WEP_KEY1:
    case DA16X_CONF_STR_WEP_KEY2:
    case DA16X_CONF_STR_WEP_KEY3:
    {
        char tmp_str[16];

        if (strlen(value) == 5 || strlen(value) == 13) {
            sprintf(input, "'%s'", value);
        } else if (strlen(value) == 10 || strlen(value) == 26) {
            strcpy(input, value);
        } else {
            return CC_FAILURE_STRING_LENGTH;
        }

        tmp = name - DA16X_CONF_STR_WEP_KEY0;
        sprintf(cmd, "wep_key%d", tmp);
        cc_set_network_str(cmd, 0, input);

        if (strlen(value) == 5 || strlen(value) == 13) {
            sprintf(input, "\"%s\"", value);
        }
        sprintf(tmp_str, "N0_%s", cmd);
        write_tmp_nvram_string(tmp_str, input);
    }
        break;

    case DA16X_CONF_STR_PSK_0:
    case DA16X_CONF_STR_PSK_1:
        if (strlen(value) < 8 || strlen(value) > 63) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        cc_set_network_str("psk", name - DA16X_CONF_STR_PSK_0, input);

        sprintf(input, "\"%s\"", value);
        if (name == DA16X_CONF_STR_PSK_0) {
            write_tmp_nvram_string(NVR_KEY_ENCKEY_0, input);
        } else {
            write_tmp_nvram_string(NVR_KEY_ENCKEY_1, input);
        }
        break;

#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
    case DA16X_CONF_STR_SAE_PASS_0:
    case DA16X_CONF_STR_SAE_PASS_1:
        if (strlen(value) < 8 || strlen(value) > 63) {
            return CC_FAILURE_STRING_LENGTH;
        }
    
        sprintf(input, "'%s'", value);
        cc_set_network_str("sae_password", name - DA16X_CONF_STR_SAE_PASS_0, input);
    
        sprintf(input, "\"%s\"", value);
        if (name == DA16X_CONF_STR_SAE_PASS_0) {
            write_tmp_nvram_string(NVR_KEY_SAE_PASS_0, input);
        } else {
            write_tmp_nvram_string(NVR_KEY_SAE_PASS_1, input);
        }
        break;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__

    case DA16X_CONF_STR_EAP_IDENTITY:
        if (strlen(value) > 64) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        cc_set_network_str("identity", 0, input);

        sprintf(input, "\"%s\"", value);
        write_tmp_nvram_string("N0_identity", input);
        break;
        
    case DA16X_CONF_STR_EAP_PASSWORD:
        if (strlen(value) > 64) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(input, "'%s'", value);
        cc_set_network_str("password", 0, input);

        sprintf(input, "\"%s\"", value);
        write_tmp_nvram_string("N0_password", input);
        break;

    case DA16X_CONF_STR_COUNTRY:
        if (strlen(value) != 2 && strlen(value) != 3) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(cmd, "country %s", value);
        da16x_cli_reply(cmd, NULL, NULL);

        write_tmp_nvram_string(NVR_KEY_COUNTRY_CODE, value);
        break;

    case DA16X_CONF_STR_DEVICE_NAME:
        if (strlen(value) == 0 || strlen(value) > 64) {
            return CC_FAILURE_STRING_LENGTH;
        }

        sprintf(cmd, "device_name %s", value);
        da16x_cli_reply(cmd, NULL, NULL);

        write_tmp_nvram_string("dev_name", value);
        break;

    case DA16X_CONF_STR_IP_0:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_0, 2);
        write_tmp_nvram_string(NVR_KEY_IPADDR_0, value);
        break;

    case DA16X_CONF_STR_NETMASK_0:
        if (!isvalidmask(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_0, 2);
        write_tmp_nvram_string(NVR_KEY_NETMASK_0, value);
        break;
        
    case DA16X_CONF_STR_GATEWAY_0:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_0, 2);
        write_tmp_nvram_string(NVR_KEY_GATEWAY_0, value);
        break;
    
    case DA16X_CONF_STR_IP_1:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_1, 2);
        write_tmp_nvram_string(NVR_KEY_IPADDR_1, value);
        break;

    case DA16X_CONF_STR_NETMASK_1:
        if (!isvalidmask(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_1, 2);
        write_tmp_nvram_string(NVR_KEY_NETMASK_1, value);
        break;

    case DA16X_CONF_STR_GATEWAY_1:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_int(NVR_KEY_NETMODE_1, 2);
        write_tmp_nvram_string(NVR_KEY_GATEWAY_1, value);
        break;

    case DA16X_CONF_STR_DNS_0:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(NVR_KEY_DNSSVR_0, value);
        break;

    case DA16X_CONF_STR_DNS_1:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(NVR_KEY_DNSSVR_1, value);
        break;

    case DA16X_CONF_STR_DNS_2ND:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(NVR_KEY_DNSSVR_0_2ND, value);
        break;

    case DA16X_CONF_STR_DHCP_START_IP:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(DHCP_SERVER_START_IP, value);
        break;

    case DA16X_CONF_STR_DHCP_END_IP:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(DHCP_SERVER_END_IP, value);
        break;

    case DA16X_CONF_STR_DHCP_DNS:
        if (!is_in_valid_ip_class(value)) {
            return CC_FAILURE_INVALID;
        }

        write_tmp_nvram_string(DHCP_SERVER_DNS, value);
        break;

    case DA16X_CONF_STR_SNTP_SERVER_IP:
        if (sntp_support_flag == pdTRUE) {
            write_tmp_nvram_string(NVR_KEY_SNTP_SERVER_DOMAIN, value);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_1_IP:
        if (sntp_support_flag == pdTRUE) {
            write_tmp_nvram_string(NVR_KEY_SNTP_SERVER_DOMAIN_1, value);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_SNTP_SERVER_2_IP:
        if (sntp_support_flag == pdTRUE) {
            write_tmp_nvram_string(NVR_KEY_SNTP_SERVER_DOMAIN_2, value);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_STR_DBG_TXPWR:
        if (strlen(value)) {
            status = write_tmp_nvram_string(NVR_KEY_DBG_TXPWR, value);                
        } else {
            status = delete_tmp_nvram_env(NVR_KEY_DBG_TXPWR);
        }

        if (status == -1) {
            return CC_FAILURE_INVALID;
        }

        status = CC_SUCCESS;
        break;

    default:
        return CC_FAILURE_NOT_SUPPORTED;
    }

    return status;
}

int da16x_set_nvcache_int(int name, int value)
{
    int status = CC_SUCCESS;

    char input[64] = {0, };

    if (name > DA16X_CONF_INT_MAX) {
        return user_set_int(name, value, 1);
    }

    switch (name) {
    case DA16X_CONF_INT_MODE:
        if (value < SYSMODE_STA_ONLY
#ifdef __SUPPORT_MESH__
            || value > SYSMODE_STA_N_MESH_PORTAL //SYSMODE_MESH_POINT
#endif /* __SUPPORT_MESH__ */
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #ifdef __SUPPORT_P2P__
                        || value > SYSMODE_STA_N_P2P
  #else
                        || value > SYSMODE_STA_N_AP
  #endif /* __SUPPORT_P2P__ */
#endif /* __SUPPORT_WIFI_CONCURRENT__ */
            ) {
            return CC_FAILURE_RANGE_OUT;
        }

        write_tmp_nvram_int(ENV_SYS_MODE, value);
        break;

    case DA16X_CONF_INT_AUTH_MODE_0:
    case DA16X_CONF_INT_AUTH_MODE_1:
        switch (value) {
        case CC_VAL_AUTH_OPEN:
            cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, "NONE");

            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                delete_tmp_nvram_env(NVR_KEY_AUTH_TYPE_0);
                delete_tmp_nvram_env("N0_wep_key0");
                delete_tmp_nvram_env("N0_wep_key1");
                delete_tmp_nvram_env("N0_wep_key2");
                delete_tmp_nvram_env("N0_wep_key3");
                delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
                delete_tmp_nvram_env(NVR_KEY_PROTO_0);
                delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0);
                delete_tmp_nvram_env(NVR_KEY_ENCKEY_0);
            } else {
                delete_tmp_nvram_env(NVR_KEY_AUTH_TYPE_1);
                delete_tmp_nvram_env(NVR_KEY_PROTO_1);
                delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_1);
                delete_tmp_nvram_env(NVR_KEY_ENCKEY_1);
            }
            break;

        case CC_VAL_AUTH_WEP:
            cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, "NONE");

            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                delete_tmp_nvram_env(NVR_KEY_AUTH_TYPE_0);
                delete_tmp_nvram_env(NVR_KEY_PROTO_0);
                delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0);
                delete_tmp_nvram_env(NVR_KEY_ENCKEY_0);
            } else {
                return CC_FAILURE_NOT_SUPPORTED;
            }
            break;

        case CC_VAL_AUTH_WPA:
        case CC_VAL_AUTH_WPA2:
        case CC_VAL_AUTH_WPA_AUTO:
            cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, key_mgmt_WPA_PSK);

            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA_PSK);
                delete_tmp_nvram_env("N0_wep_key0");
                delete_tmp_nvram_env("N0_wep_key1");
                delete_tmp_nvram_env("N0_wep_key2");
                delete_tmp_nvram_env("N0_wep_key3");
                delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
            } else {
                write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_1, key_mgmt_WPA_PSK);
            }

            if (value == CC_VAL_AUTH_WPA) {
                cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, proto_WPA);

                if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                    write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_WPA);
                } else {
                    write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_WPA);
                }
            } else if (value == CC_VAL_AUTH_WPA2) {
                cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, proto_RSN);

                if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                    write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
                } else {
                    write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_RSN);
                }
            } else {
                cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, "WPA RSN");

                if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                    delete_tmp_nvram_env(NVR_KEY_PROTO_0);
                } else {
                    delete_tmp_nvram_env(NVR_KEY_PROTO_1);
                }
            }
            break;

#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
        case CC_VAL_AUTH_OWE:
            cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, key_mgmt_WPA3_OWE);

            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA3_OWE);
                delete_tmp_nvram_env("N0_wep_key0");
                delete_tmp_nvram_env("N0_wep_key1");
                delete_tmp_nvram_env("N0_wep_key2");
                delete_tmp_nvram_env("N0_wep_key3");
                delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
            } else {
                write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_1, key_mgmt_WPA3_OWE);
            }
                
            break;
        case CC_VAL_AUTH_SAE:
        case CC_VAL_AUTH_RSN_SAE:
            // run set_network 0/1 key_mgmt
            if (value == CC_VAL_AUTH_SAE) {
                // cli set_network 0/1 key_mgmt SAE
                cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, key_mgmt_WPA3_SAE);
            }
            else 
            {
                // CC_VAL_AUTH_RSN_SAE
                // cli set_network 0/1 key_mgmt WPA-PSK SAE
                cc_set_network_str("key_mgmt", name - DA16X_CONF_INT_AUTH_MODE_0, key_mgmt_WPA_PSK_WPA3_SAE);                
            }

            // save key_mgmt to nvram
            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                // STA mode
                    
                if (value == CC_VAL_AUTH_SAE) {
                    write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA3_SAE);
                } else {
                    // CC_VAL_AUTH_RSN_SAE
                    write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA_PSK_WPA3_SAE);        
                }                    
                    
                delete_tmp_nvram_env("N0_wep_key0");
                delete_tmp_nvram_env("N0_wep_key1");
                delete_tmp_nvram_env("N0_wep_key2");
                delete_tmp_nvram_env("N0_wep_key3");
                delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
            } else {
                // SoftAP mode
                if (value == CC_VAL_AUTH_SAE) {
                    write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_1, key_mgmt_WPA3_SAE);
                } else {
                    // CC_VAL_AUTH_RSN_SAE
                    write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_1, key_mgmt_WPA_PSK_WPA3_SAE);
                }        
            }

            // cli set_network 0/1 proto RSN
            cc_set_network_str("proto", name - DA16X_CONF_INT_AUTH_MODE_0, proto_RSN);

            // save proto to nvram
            if (name == DA16X_CONF_INT_AUTH_MODE_0) {
                write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
            } else {
                write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_RSN);
            }
            break;

#endif // __SUPPORT_WPA3_PERSONAL_CORE__
        case CC_VAL_AUTH_WPA_EAP:
        case CC_VAL_AUTH_WPA2_EAP:
        case CC_VAL_AUTH_WPA_AUTO_EAP:
            if (name == DA16X_CONF_INT_AUTH_MODE_1) {
                return CC_FAILURE_NOT_SUPPORTED;
            }

            cc_set_network_str("key_mgmt", 0, key_mgmt_WPA_EAP);
            write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA_EAP);
            
            if (value == CC_VAL_AUTH_WPA_EAP) {
                cc_set_network_str("proto", 0, proto_WPA);
                write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_WPA);
            } else if (value == CC_VAL_AUTH_WPA2_EAP) {
                cc_set_network_str("proto", 0, proto_RSN);
                write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
            } else {
                cc_set_network_str("proto", 0, "WPA RSN");
                delete_tmp_nvram_env(NVR_KEY_PROTO_0);
            }
            break;

        default:
            return CC_FAILURE_RANGE_OUT;
        }
        break;

    case DA16X_CONF_INT_WEP_KEY_INDEX:
        if (value < 0 || value > 3) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("wep_tx_keyidx", 0, value);
        write_tmp_nvram_int(NVR_KEY_WEPINDEX_0, value);
        break;

    case DA16X_CONF_INT_ENCRYPTION_0:
    case DA16X_CONF_INT_ENCRYPTION_1:
        switch (value) {
        case CC_VAL_ENC_TKIP:
            strcpy(input, "TKIP");
            cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);

            if (name == DA16X_CONF_INT_ENCRYPTION_0) {
                write_tmp_nvram_string(NVR_KEY_ENC_TYPE_0, input);
            } else {
                write_tmp_nvram_string(NVR_KEY_ENC_TYPE_1, input);
            }
            break;

        case CC_VAL_ENC_CCMP:
            strcpy(input, "CCMP");
            cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);

            if (name == DA16X_CONF_INT_ENCRYPTION_0) {
                write_tmp_nvram_string(NVR_KEY_ENC_TYPE_0, input);
            } else {
                write_tmp_nvram_string(NVR_KEY_ENC_TYPE_1, input);
            }
            break;

        case CC_VAL_ENC_AUTO:
            strcpy(input, "TKIP CCMP");
            cc_set_network_str("pairwise", name - DA16X_CONF_INT_ENCRYPTION_0, input);

            if (name == DA16X_CONF_INT_ENCRYPTION_0) {
                delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0);
            } else {
                delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_1);
            }
            break;

        default:
            return CC_FAILURE_RANGE_OUT;
        }
        break;

    case DA16X_CONF_INT_WIFI_MODE_0:
        if (value < 0 || value > CC_VAL_WFMODE_B) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("wifi_mode", 0, value);
        write_tmp_nvram_int(NVR_KEY_WLANMODE_0, value);
        break;

    case DA16X_CONF_INT_WIFI_MODE_1:
        if (value < 0 || value > CC_VAL_WFMODE_B) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("wifi_mode", 1, value);
        write_tmp_nvram_int(NVR_KEY_WLANMODE_1, value);
        break;

    case DA16X_CONF_INT_CHANNEL:
        da16x_get_config_str(DA16X_CONF_STR_COUNTRY, input);

        if (value < 0) {
            return CC_FAILURE_INVALID;
        }
    
        if (chk_channel_by_country(input, value, 0, NULL) < 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("channel", 1, value);
        if (value != 0) {
            write_tmp_nvram_int(NVR_KEY_CHANNEL, value == 14 ? 2484 : 2407 + 5 * value);
        } else {
            write_tmp_nvram_int(NVR_KEY_CHANNEL, 0);
        }
        break;

    case DA16X_CONF_INT_FREQUENCY:
        da16x_get_config_str(DA16X_CONF_STR_COUNTRY, input);
        if (chk_channel_by_country(input, value, 1, NULL) < 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("frequency", 1, value);
        write_tmp_nvram_int(NVR_KEY_CHANNEL, value);
        break;

    case DA16X_CONF_INT_ROAM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            da16x_cli_reply("roam run", NULL, NULL);
        } else if (value == CC_VAL_DISABLE) {
            da16x_cli_reply("roam stop", NULL, NULL);
        }

        write_tmp_nvram_int(ENV_ROAM, value);
        break;

    case DA16X_CONF_INT_ROAM_THRESHOLD:
        if (value < -95 || value > 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "roam_threshold %d", value);
        da16x_cli_reply(input, NULL, NULL);

        write_tmp_nvram_int(ENV_ROAM_THRESHOLD, value);
        break;

    case DA16X_CONF_INT_FIXED_ASSOC_CH:
        if (value < 0 || value >= 14) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == 0) {
            status = delete_tmp_nvram_env(NVR_KEY_ASSOC_CH);
            if (status == FALSE) {
                status = CC_FAILURE_UNKNOWN;
            } else {
                status = CC_SUCCESS;
            }
        } else {
            status = write_tmp_nvram_int(NVR_KEY_ASSOC_CH, value);
        }

        if (status < 0) {
            return CC_FAILURE_UNKNOWN;
        }

        break;

    case DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2:
        if (value < 0 || value > 1) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == 0) {
            status = delete_tmp_nvram_env(NVR_KEY_FST_CONNECT);
            if (status == FALSE) {
                status = CC_FAILURE_UNKNOWN;
            } else {
                delete_nvram_env(NVR_KEY_NETMODE_0);
                delete_nvram_env(NVR_KEY_IPADDR_0);
                delete_nvram_env(NVR_KEY_NETMASK_0);
                delete_nvram_env(NVR_KEY_GATEWAY_0);
                delete_nvram_env(NVR_KEY_DNSSVR_0);
                delete_nvram_env(NVR_KEY_PSK_RAW_0);
                delete_nvram_env(NVR_KEY_ASSOC_CH);
                status = CC_SUCCESS;
            }
        } else {
            status = write_tmp_nvram_int(NVR_KEY_FST_CONNECT, value);
        }

        if (status < 0) {
            return CC_FAILURE_UNKNOWN;
        }

        status = CC_SUCCESS;
        break;

    case DA16X_CONF_INT_STA_PROF_DISABLED:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        status = cc_set_network_int("N0_disabled", 0, value);
        write_tmp_nvram_int("N0_disabled", value);
        break;

    case DA16X_CONF_INT_EAP_PHASE1:
        if (value < 0 || value > CC_VAL_EAP_TLS) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_EAP_DEFAULT) {
            cc_set_network_str("eap", 0, "PEAP TTLS FAST");
            write_tmp_nvram_string("N0_eap", "PEAP TTLS FAST");
        } else if (value == CC_VAL_EAP_PEAP0) {
            cc_set_network_str("eap", 0, "PEAP");
            da16x_cli_reply("peap_ver 0", NULL, NULL);
            write_tmp_nvram_string("N0_eap", "PEAP");
            write_tmp_nvram_string(ENV_PEAP_VER, "0");
        } else if (value == CC_VAL_EAP_PEAP1) {
            cc_set_network_str("eap", 0, "PEAP");
            da16x_cli_reply("peap_ver 1", NULL, NULL);
            write_tmp_nvram_string("N0_eap", "PEAP");
            write_tmp_nvram_string(ENV_PEAP_VER, "1");
        } else if (value == CC_VAL_EAP_TTLS) {
            cc_set_network_str("eap", 0, "TTLS");
            write_tmp_nvram_string("N0_eap", "TTLS");
        } else if (value == CC_VAL_EAP_FAST) {
            cc_set_network_str("eap", 0, "FAST");
            cc_set_network_str("phase1", 0, "'fast_provisioning=2'");
            write_tmp_nvram_string("N0_eap", "FAST");
            write_tmp_nvram_string("N0_phase1", "'fast_provisioning=2'");
        } else {
            cc_set_network_str("eap", 0, "TLS");
            write_tmp_nvram_string("N0_eap", "TLS");
        }

        // Erase old fast_pac, fast_pac_len for EAP-FAST
        delete_tmp_nvram_env(ENV_FAST_PAC);           
        delete_tmp_nvram_env(ENV_FAST_PAC_LEN);
        break;

    case DA16X_CONF_INT_EAP_PHASE2:
        if (value < 0 || value > CC_VAL_EAP_GTC) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_EAP_PHASE2_MIX) {
            cc_set_network_str("phase2", 0, "'auth=MSCHAPV2 GTC'");
            write_tmp_nvram_string("N0_phase2", "auth=MSCHAPV2 GTC");
        } else if (value == CC_VAL_EAP_MSCHAPV2) {
            cc_set_network_str("phase2", 0, "'auth=MSCHAPV2'");
            write_tmp_nvram_string("N0_phase2", "auth=MSCHAPV2");
        } else {
            cc_set_network_str("phase2", 0, "'auth=GTC'");
            write_tmp_nvram_string("N0_phase2", "auth=GTC");
        }
        break;

#ifdef    CONFIG_AP
    case DA16X_CONF_INT_BEACON_INTERVAL:
        if (value < 20 || value > 1000) {
            return CC_FAILURE_RANGE_OUT;
        }

        cc_set_network_int("beacon_int", 1, value);
        write_tmp_nvram_int("N1_beacon_int", value);
        break;

    case DA16X_CONF_INT_INACTIVITY:
        if (value <= 0 || value > 86400) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "ap_max_inactivity %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_AP_MAX_INACTIVITY, value);
        break;

    case DA16X_CONF_INT_RTS_THRESHOLD:
        if (value <= 0 || value > 2347) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "ap_rts %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_RTS_THRESHOLD, value);
        break;

    case DA16X_CONF_INT_WMM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE)
            return CC_FAILURE_RANGE_OUT;

        sprintf(input, "wmm_enabled %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_WMM_ENABLED, value);
        break;

    case DA16X_CONF_INT_WMM_PS:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "wmm_ps_enabled %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_WMM_PS_ENABLED, value);
        break;
#endif    /* CONFIG_AP */

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __SUPPORT_P2P__ )
    case DA16X_CONF_INT_P2P_OPERATION_CHANNEL:
        if (value != 0 && value != 1 && value != 6 && value != 11) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "p2p_set oper_channel %d", value);
        status = da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_P2P_OPER_CHANNEL, value);
        break;

    case DA16X_CONF_INT_P2P_LISTEN_CHANNEL:
        if (value != 0 && value != 1 && value != 6 && value != 11) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "p2p_set listen_channel %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_P2P_LISTEN_CHANNEL, value);
        break;

    case DA16X_CONF_INT_P2P_GO_INTENT:
        if (value < 0 || value > 15) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "p2p_set go_intent %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_P2P_GO_INTENT, value);
        break;

    case DA16X_CONF_INT_P2P_FIND_TIMEOUT:
        if (value <= 0 || value > 86400) {
            return CC_FAILURE_RANGE_OUT;
        }

        sprintf(input, "p2p_set find_timeout %d", value);
        da16x_cli_reply(input, NULL, NULL);
        write_tmp_nvram_int(ENV_P2P_FIND_TIMEOUT, value);
        break;
  #endif /* __SUPPORT_P2P__ */
#endif /* __SUPPORT_WIFI_CONCURRENT__*/

#ifdef __SUPPORT_NAT__
    case DA16X_CONF_INT_NAT:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE)
            return CC_FAILURE_RANGE_OUT;

        if (value == CC_VAL_ENABLE) {
            write_tmp_nvram_int("NAT", pdTRUE);
        } else {
            delete_tmp_nvram_env("NAT");
        }
        break;
#endif /* __SUPPORT_NAT__ */

#ifdef __SUPPORT_DHCPC_IP_TO_STATIC_IP__
    case DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP:
        da16x_set_temp_staticip_mode(value, pdFALSE);
        if (value == pdFALSE) {
            break;
        }
        // no break
#endif // __SUPPORT_DHCPC_IP_TO_STATIC_IP__

    case DA16X_CONF_INT_DHCP_CLIENT:
    case DA16X_CONF_INT_DHCP_CLIENT_CONFIG_ONLY:
    {
        UINT iface = WLAN0_IFACE;
        struct netif *netif = netif_get_by_index(iface+2);
        err_t err = ERR_OK;

        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            if (name == DA16X_CONF_INT_DHCP_CLIENT) {
                err = dhcp_start(netif);

                if (err == ERR_OK) {
                    PRINTF("\nDHCP Client Started WLAN%u.\n", iface);
                } else {
                    PRINTF("\nDHCP Client Start Error(%d) WLAN%u.\n", err, iface);
                    return CC_FAILURE_NOT_READY;
                }
            }
            set_netmode(0, DHCPCLIENT, pdTRUE);
        } else {
            if (name == DA16X_CONF_INT_DHCP_CLIENT) {
                dhcp_stop(netif);
            }
            set_netmode(0, STATIC_IP, pdTRUE);
        }
    }
        break;

    case DA16X_CONF_INT_DHCP_SERVER:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }
        
        if (value == CC_VAL_ENABLE) {
            write_tmp_nvram_int(NVR_KEY_DHCPD, pdTRUE);
        } else {
            delete_tmp_nvram_env(NVR_KEY_DHCPD);
        }
        break;

    case DA16X_CONF_INT_DHCP_LEASE_TIME:
        if (value <= 0) {
            return CC_FAILURE_RANGE_OUT;
        }

        write_tmp_nvram_int(DHCP_SERVER_LEASE_TIME, value);
        break;

    case DA16X_CONF_INT_SNTP_CLIENT:
        if (sntp_support_flag == pdTRUE) {
            if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
                return CC_FAILURE_RANGE_OUT;
            }
            if (value == CC_VAL_ENABLE && get_sntp_use() == 1) {
                break;
            }

            set_sntp_use(value); // set run flag and start
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_INT_SNTP_SYNC_PERIOD:
        if (sntp_support_flag == pdTRUE) {
            if (value <= 0) {
                return CC_FAILURE_RANGE_OUT;
            }

            write_tmp_nvram_int(NVR_KEY_SNTP_SYNC_PERIOD, value);
        } else {
            return CC_FAILURE_NOT_SUPPORTED;
        }
        break;

    case DA16X_CONF_INT_TLS_VER:
        if (value < 0 || value > 2) {
            return CC_FAILURE_RANGE_OUT;
        }
        
        if (value == 0) {
            write_tmp_nvram_int("tls_ver", 769);
        } else if (value == 1) {
            write_tmp_nvram_int("tls_ver", 770);
        } else {
            delete_tmp_nvram_env("tls_ver");
        }
        break;

    case DA16X_CONF_INT_TLS_ROOTCA_CHK:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            write_tmp_nvram_int("ROOTCA_CHK", CC_VAL_ENABLE);
        } else {
            delete_tmp_nvram_env("ROOTCA_CHK");
        }
        break;
        
    case DA16X_CONF_INT_GMT:
        if (value < -43200 || value > 43200) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value != 0) {
            write_tmp_nvram_int(NVR_KEY_TIMEZONE, value);
        } else {
            delete_tmp_nvram_env(NVR_KEY_TIMEZONE);
        }
        break;

    case DA16X_CONF_INT_DPM:
        if (value != CC_VAL_ENABLE && value != CC_VAL_DISABLE) {
            return CC_FAILURE_RANGE_OUT;
        }

        if (value == CC_VAL_ENABLE) {
            write_tmp_nvram_int(NVR_KEY_DPM_MODE, 1);
        } else {
            delete_tmp_nvram_env(NVR_KEY_DPM_MODE);
        }
        break;

#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
    case DA16X_CONF_INT_PMF_80211W_0:
    case DA16X_CONF_INT_PMF_80211W_1:
        if (value != CC_VAL_MAND && value != CC_VAL_OPT) {
            return CC_FAILURE_RANGE_OUT;
        }
            
        if (value == CC_VAL_MAND) {
            cc_set_network_int("ieee80211w", name - DA16X_CONF_INT_PMF_80211W_0, 2);
            if (name == DA16X_CONF_INT_PMF_80211W_0) {
                write_tmp_nvram_int(NVR_KEY_PMF_0, 2);
            } else {
                write_tmp_nvram_int(NVR_KEY_PMF_1, 2);
            }
        } else if (value == CC_VAL_OPT) {
            cc_set_network_int("ieee80211w", name - DA16X_CONF_INT_PMF_80211W_0, 1);
            if (name == DA16X_CONF_INT_PMF_80211W_0) {
                write_tmp_nvram_int(NVR_KEY_PMF_0, 1);
            } else {
                write_tmp_nvram_int(NVR_KEY_PMF_1, 1);
            }
        } else {
            if (name == DA16X_CONF_INT_PMF_80211W_0) {
                delete_tmp_nvram_env(NVR_KEY_PMF_0);
            } else {
                delete_tmp_nvram_env(NVR_KEY_PMF_1);
            }
        }
        break;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
	case DA16X_CONF_INT_HIDDEN_0:
		cc_set_network_int("scan_ssid", name - DA16X_CONF_INT_HIDDEN_0, value);
		write_tmp_nvram_int("N0_scan_ssid", value);
		break;

    default:
        return CC_FAILURE_NOT_SUPPORTED;
    }

    return status;
}

void da16x_nvcache2flash(void)
{
    save_tmp_nvram();
}

static int cc_set_network(char *cmd, int iface)
{
    char return_str[16] = {0, };
    int ret = da16x_cli_reply(cmd, CLI_DELIMIT_TAB, return_str);

    if (ret < 0) {
        return CC_FAILURE_UNKNOWN;
    } else if (strcmp(return_str, "FAIL") == 0) {
        char cmd_tmp[16] = {0, };

        sprintf(cmd_tmp, "add_network"CLI_DELIMIT_TAB"%d", iface);
        da16x_cli_reply(cmd_tmp, CLI_DELIMIT_TAB, NULL);

        if (iface == 1) {
            da16x_cli_reply("set_network 1 mode 2", NULL, NULL);
        }

        ret = da16x_cli_reply(cmd, CLI_DELIMIT_TAB, return_str);
        if (ret < 0 || strcmp(return_str, "FAIL") == 0) {
            return CC_FAILURE_UNKNOWN;
        }
    }
    
    return CC_SUCCESS;
}

static int cc_set_network_str(char *name, int iface, char *val)
{
    char cmd[256] = {0, };

    sprintf(cmd, "set_network"CLI_DELIMIT_TAB"%d"CLI_DELIMIT_TAB"%s"CLI_DELIMIT_TAB"%s", iface, name, val);
    return cc_set_network(cmd, iface);
}

static int cc_set_network_int(char *name, int iface, int val)
{
    char cmd[256] = {0, };

    sprintf(cmd, "set_network"CLI_DELIMIT_TAB"%d"CLI_DELIMIT_TAB"%s"CLI_DELIMIT_TAB"%d", iface, name, val);
    return cc_set_network(cmd, iface);
}

int da16x_set_default_ap_config(void)
{
	char ssid_str[35] = {0, };
	char psk_str[65] = {0, };
	char dhcp_lease_start[16] = {0, };
	char dhcp_lease_end[16] = {0, };
	char *result_ptr = NULL;

	extern int gen_default_psk(char *default_psk);
	result_ptr = read_nvram_string(NVR_KEY_SSID_0);
	if (clear_tmp_nvram_env() == -1) {
        return CC_FAILURE_UNKNOWN;
    }   
	
	if ( result_ptr && strlen(result_ptr) != 0) {
		da16x_cli_reply("remove_network 0", NULL, NULL);
	}

	da16x_set_nvcache_int(DA16X_CONF_INT_MODE, WLAN1_IFACE);
	gen_ssid(CHIPSET_NAME, WLAN1_IFACE, 0, ssid_str, sizeof(ssid_str));
	da16x_set_nvcache_str(DA16X_CONF_STR_SSID_1, ssid_str);
#if defined (__SUPPORT_WPA3_PERSONAL__)
	da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_1, CC_VAL_AUTH_RSN_SAE);
#else
	da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_1, CC_VAL_AUTH_WPA2);
#endif /* __SUPPORT_WPA3_PERSONAL__ */

	gen_default_psk(psk_str);
	da16x_set_nvcache_str(DA16X_CONF_STR_PSK_1, psk_str);

	da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_1, CC_VAL_ENC_CCMP);
	da16x_set_nvcache_int(DA16X_CONF_INT_CHANNEL, 0);

	da16x_set_nvcache_str(DA16X_CONF_STR_IP_1, DEFAULT_IPADDR_WLAN1);
	da16x_set_nvcache_str(DA16X_CONF_STR_NETMASK_1, DEFAULT_SUBNET_WLAN1);
	da16x_set_nvcache_str(DA16X_CONF_STR_GATEWAY_1, DEFAULT_GATEWAY_WLAN1);

	longtoip((iptolong(DEFAULT_IPADDR_WLAN1) + 1), dhcp_lease_start);
	da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_START_IP, dhcp_lease_start);
	longtoip((iptolong(DEFAULT_IPADDR_WLAN1) + 10), dhcp_lease_end);
	da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_END_IP, dhcp_lease_end);
	da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_DNS, DEFAULT_DNS_WLAN1);
	da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_LEASE_TIME, 1800);
	da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_SERVER, CC_VAL_ENABLE);

    // Write NVRAM Magic Code ...
    write_tmp_nvram_int((const char *)NVR_KEY_MAGIC_CODE, (int)NVR_MAGIC_CODE);

	da16x_nvcache2flash();
    return CC_SUCCESS;
}

int da16x_set_default_ap(int reboot_flag)
{
    int op_mode = getSysMode();
    
    if (da16x_set_default_ap_config() != CC_SUCCESS) {
        return CC_FAILURE_UNKNOWN;
    }

    if (op_mode == WLAN1_IFACE) {
        da16x_cli_reply("ap restart", NULL, NULL);
    } else {
        if (reboot_flag == pdTRUE) {
            reboot_func(SYS_REBOOT);
        }
    }
    
    return CC_SUCCESS;
}

int da16x_set_default_sta(void)
{
    int op_mode = getSysMode();

    clear_tmp_nvram_env();

    da16x_set_nvcache_int(DA16X_CONF_INT_MODE, WLAN0_IFACE);
    da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, CHIPSET_NAME);
    da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_OPEN);

    da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_CLIENT, CC_VAL_ENABLE);

    da16x_nvcache2flash();

    if (op_mode == WLAN0_IFACE) {
        da16x_cli_reply("reassociate", NULL, NULL);
    } else {
        reboot_func(SYS_REBOOT);
    }
    
    return CC_SUCCESS;
}

#endif    /*  XIP_CACHE_BOOT */

/* EOF */
