/**
 ****************************************************************************************
 *
 * @file app_common_support.c
 *
 * @brief Define the common utility for apps module
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

#include "da16x_dns_client.h"
#include "da16x_sntp_client.h"
#include "da16x_dpm.h"
#include "da16x_network_common.h"
#include "da16x_time.h"

#include "command.h"

#include "common_utils.h"
#include "common_def.h"

#include "util_api.h"

#include "app_common_util.h"
#include "app_common_support.h"
#include "user_dpm_api.h"

#if defined ( __SUPPORT_UPSS__ )
#include "upss.h"
#endif // __SUPPORT_UPSS__

#define NV_SAVED_IP_ADDRESS         "NV_SAVED_IP_ADDRESS"
#define NVR_FIRST_SNTP_TIME         "NV_FIRST_SNTP_TIME"
#define NV_SNTP_SUCCESS_FLAG        "NV_SNTP_SUCCESS_FLAG"
#define MAX_CNT_TO_RESET_SNTP       8 //Maximum checking count to restart SNTP

extern int chk_dpm_mode(void);
extern int chk_dpm_wakeup(void);

static UINT32 app_nvram_set_server_ip(char *ip_str)
{
    int res;
    res = write_nvram_string(NV_SAVED_IP_ADDRESS, ip_str);

    if (res == 0) {
        return 1;
    } else {
        PRINTF("write ip address in nvram fail !!\n");
        return 0;
    }
}

static char* app_nvram_get_server_ip(void)
{
    char *nvRetIP = NULL;

    nvRetIP = read_nvram_string(NV_SAVED_IP_ADDRESS);

    return nvRetIP;
}

static UINT32 app_rtm_set_server_ip(char *data_name, char *ip_addr)
{
    UINT32 res = ER_SUCCESS;
    app_dpm_info_rtm *data = NULL;

    UINT32 status = 0;
    const ULONG wait_option = 100;
    UINT32 len = 0;

    if (data_name == NULL) {
        PRINTF("[set]rtm data name is null !!\n");
        return ER_NOT_SUCCESSFUL;
    }

    if (chk_dpm_mode()) {
        len = user_rtm_get(data_name, (unsigned char**)&data);

        if (len == 0) {
            status = user_rtm_pool_allocate(data_name, (void**)&data, sizeof(app_dpm_info_rtm), wait_option);

            if (status) {
                PRINTF("failed to allocate app_dpm info in rtm(0x%02x)\n", status);
                data = NULL;
            }
            memset(data, 0x00, sizeof(app_dpm_info_rtm));
        } else if (len != sizeof(app_dpm_info_rtm)) {
            PRINTF("invalid size(%ld)\n", len);
            data = NULL;
        }

        if (data) {
            memset(data->ServrIp, 0, sizeof(data->ServrIp));
            if (isvalidip(ip_addr)) {
                strcpy(data->ServrIp, ip_addr);
            } else {
                APRINTF_S("[%s:%d] passed ip (=\"%s\") invalid, and cleared \n", __func__,
                    __LINE__, ip_addr);
            }
        } else {
            res = ER_NO_MEMORY;
            PRINTF("[set] rtm data: null");
        }
    } else {
        res = ER_NOT_SUCCESSFUL;
    }

    return res;
}

static char* app_rtm_get_server_ip(char *data_name)
{
    UINT32 status = 0;
    const ULONG wait_option = 100;

    app_dpm_info_rtm *data = NULL;
    UINT32 len = 0;

    if (data_name == NULL) {
        PRINTF("[get]rtm data name is null !!\n");
        return NULL;
    }

    if (chk_dpm_mode()) {
        len = user_rtm_get(data_name, (unsigned char**)&data);

        if (len == 0) {
            status = user_rtm_pool_allocate(data_name, (void**)&data, sizeof(app_dpm_info_rtm), wait_option);

            if (status) {
                PRINTF("failed to allocate app_dpm info in rtm(0x%02x)\n", status);
                data = NULL;
            }
            memset(data, 0x00, sizeof(app_dpm_info_rtm));
        } else if (len != sizeof(app_dpm_info_rtm)) {
            PRINTF("invalid size(%ld)\n", len);
            data = NULL;
        }

        if (data) {
            return data->ServrIp;
        } else {
            PRINTF("[get] rtm data: null\n");
            return NULL;
        }
    }

    return NULL;
}

char* app_common_get_ip_from_noraml_dns(char *hostname)
{
    char *ip_str = NULL;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;
    PRINTF("hostName = \"%s\"\n", hostname);

retryQuery: ip_str = dns_A_Query((char*)hostname, waitOptQuery);
    if (ip_str != NULL && isvalidip(ip_str)) {
        PRINTF("host IP = \"%s\"\n", ip_str);
    } else {
        if (++retryCount <= 5) {
            waitOptQuery += 400;
            vTaskDelay(10);
            PRINTF("dns_A_Query() failed : retry (cnt=%d)...\n", retryCount);
            goto retryQuery;
        }
        ip_str = NULL;
    }
    retryCount = 0; //re-initialize for reentrance

    return ip_str;
}

char* app_common_get_ip_from_fast_dns_with_rtm(char *hostname, char *rtm_data_name, UINT8 is_real_query)
{
    char *ip_str = NULL;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;
    PRINTF("hostName = \"%s\", flag to re-query (=%d)\n", hostname, is_real_query);

    if (is_real_query) {
        goto retryQuery;
    }

    int wakeupSource = WAKEUP_SOURCE_FN();
    int wakeupType = WAKEUP_TYPE_FN();
    if (chk_dpm_wakeup() || (wakeupType == 0 && wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION)
        || (wakeupType == 0 && wakeupSource == WAKEUP_COUNTER_WITH_RETENTION)) {
        ip_str = app_rtm_get_server_ip(rtm_data_name);
        if (isvalidip(ip_str))
            PRINTF("host IP from RTM = \"%s\"\n", ip_str);
        else {
            PRINTF("host IP from RTM: NG\n");
            goto retryQuery;
        }
    } else {
retryQuery: ip_str = dns_A_Query((char*)hostname, waitOptQuery);
        if (ip_str != NULL && isvalidip(ip_str)) {
            PRINTF("host IP = \"%s\" \n", ip_str);
            app_rtm_set_server_ip(rtm_data_name, ip_str);
        } else {
            if (++retryCount <= 5) {
                waitOptQuery += 400;
                vTaskDelay(10);
                PRINTF("dns_A_Query() failed : retry (cnt=%d)...\n", retryCount);
                goto retryQuery;
            }
            ip_str = NULL;
        }
        retryCount = 0; //re-initialize for reentrance
    }

    return ip_str;
}

char* app_common_get_ip_from_fast_dns_with_nvram(char *hostname, UINT8 is_real_query)
{
    char *ip_str = NULL;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;

    PRINTF("hostName = \"%s\", flag to re-query (=%d)\n", hostname, is_real_query);

    if (is_real_query) {
        goto retryQuery;
    }

    ip_str = app_nvram_get_server_ip();
    if (isvalidip(ip_str)) {
        PRINTF("host IP from NVRAM: \"%s\"\n", ip_str);
        return ip_str;
    } else {
        PRINTF("host IP from NVRAM: NG\n");
    }

retryQuery: ip_str = dns_A_Query((char*)hostname, waitOptQuery);
    if (ip_str != NULL && isvalidip(ip_str)) {
        PRINTF("host IP = \"%s\"\n", ip_str);
        app_nvram_set_server_ip(ip_str);
    } else {
        if (++retryCount <= 5) {
            waitOptQuery += 400;
            vTaskDelay(10);
            PRINTF("dns_A_Query() failed : retry (cnt=%d)...\n", retryCount);
            goto retryQuery;
        }
        ip_str = NULL;
    }
    retryCount = 0; //re-initialize for reentrance

    return ip_str;
}

static int save_sntp_time_nvram(unsigned long _now)
{
    char str[256];
    int retval;

    sprintf(str, "%lu", _now);
    retval = write_nvram_string(NVR_FIRST_SNTP_TIME, str);

    return retval;
}

static int delete_sntp_time_nvram()
{
    int retval;
    char *sntp_time = NULL;

    sntp_time = read_nvram_string(NVR_FIRST_SNTP_TIME);
    if (sntp_time != NULL) {
        retval = delete_nvram_env(NVR_FIRST_SNTP_TIME);
    }

    return retval;
}

static unsigned long long read_sntp_time_from_nvram()
{
    char *sntp_time = NULL;
    unsigned long long ret_sntp_time;

    sntp_time = read_nvram_string(NVR_FIRST_SNTP_TIME);

    if (sntp_time == NULL) {
        PRINTF("NO SNTP value in NV\n");
        return 0;
    }

    ret_sntp_time = strtoul(sntp_time, NULL, 10);

    return ret_sntp_time;
}

static void net_check_prop(void)
{
    while (1) {
        if (check_net_init(WLAN0_IFACE) == pdPASS) {
            break;
        }
        vTaskDelay(5);
    }

    /* Waiting netif status */
    while (chk_network_ready(WLAN0_IFACE) != pdTRUE) {
        vTaskDelay(5);
    }
}

void app_common_set_time_sync_from_normal_sntp(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval)
{
    __time64_t now;
    struct tm *ts;
    char buf[80];
    UINT32 loop_cnt = 0;
    char flagSntp = 0;
    int wakeupSource;
    int wakeupType;
    int isSntpSuccess = 0;
    int isSntpNeed = 0;
    int rcode;
    int use = 0;

    /* wait for network initialization */
    net_check_prop();

    APRINTF("\n\nSNTP connection Try count = %d\n\n", _sntpCount);

    wakeupSource = WAKEUP_SOURCE_FN();
    wakeupType = WAKEUP_TYPE_FN();

    //get current date/time:: in case of wakeup from sleep mode3, sleep mode2 with RTM or sleep mode1 with RTM
    if (chk_dpm_wakeup() || (wakeupSource == WAKEUP_COUNTER_WITH_RETENTION && wakeupType == 0 /*DPM_UNKNOWN_WAKEUP*/)
        || (wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION && wakeupType == 0 /*DPM_UNKNOWN_WAKEUP*/)) {
        flagSntp = 1;
        rcode = read_nvram_int(NV_SNTP_SUCCESS_FLAG, &isSntpSuccess);
        if (rcode != 0 || isSntpSuccess != 1) {
            flagSntp = 0;
            isSntpNeed = 1;
        } else {
            return;
        }

    } else {
        flagSntp = 0;
        isSntpNeed = 1;
    }

    if (isSntpNeed == 1) {
        read_nvram_int((const char*)NVR_KEY_SNTP_C, (INT32*)&use);
        if (use == -1) {
            use = 0;
        }

        if (use == 0) {
            APRINTF("SNTP use=%d...call set_sntp_use(1)\n", use);
            set_sntp_use(0);
            vTaskDelay(5);
            set_sntp_use(1);
        }

    }

    while (1) {
        if (flagSntp || is_sntp_sync()) {
            da16x_time64(NULL, &now);
            ts = (struct tm*)da16x_localtime64(&now);

            da16x_strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", ts);
            APRINTF_Y("SNTP Sync: %s : checked\n", buf);

            isSntpSuccess = 1;
            if (write_nvram_int(NV_SNTP_SUCCESS_FLAG, isSntpSuccess))
                PRINTF("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

            break;
        } else {
            if (loop_cnt++ > _sntpCount) {
                APRINTF("SNTP failed (cnt=%d)\n", loop_cnt - 1);

                isSntpSuccess = 0;
                if (write_nvram_int(NV_SNTP_SUCCESS_FLAG, isSntpSuccess))
                    PRINTF("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

                if (_sleepMode == SLEEP_MODE_1)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, 0);
                else if (_sleepMode == SLEEP_MODE_2)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, 0);
                else
                    app_common_goto_sleep_1_or_2(SLEEP_MODE_2, 1, _sleepInterval, 0);
            }
            vTaskDelay(50);
        }
    }

}

void app_common_set_time_sync_from_fast_sntp_with_nvram(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval,
    unsigned long long added_sleep_dur)
{
    __time64_t now;
    struct tm *ts;
    char buf[80];
    UINT32 loop_cnt = 0;

    __time64_t setTime;
    unsigned long long readSntpTime = 0;
    int wakeupSource;
    int isSntpSuccess = 0;
    int isSntpNeed = 0;
    int rcode;

    /* wait for network initialization */
    net_check_prop();

    wakeupSource = WAKEUP_SOURCE_FN();
    if (wakeupSource == WAKEUP_SOURCE_WAKEUP_COUNTER || wakeupSource == WAKEUP_SOURCE_EXT_SIGNAL
        || wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION || wakeupSource == WAKEUP_SENSOR_EXT_SIGNAL) {
        readSntpTime = read_sntp_time_from_nvram();
        rcode = read_nvram_int(NV_SNTP_SUCCESS_FLAG, &isSntpSuccess);
        if (readSntpTime != 0 && rcode == 0 && isSntpSuccess == 1) {
            readSntpTime = readSntpTime + added_sleep_dur;
            setTime = (__time64_t )readSntpTime;
            da16x_time64(&setTime, &now); /* set UTC Time */
            PRINTF("[Set UTC Time(Saved Time + Sleep Duration)] : %u \n", (unsigned long long )now);

            return;
        } else {
            PRINTF("read sntp time fail from nvram!!\n");
            isSntpNeed = 1;
        }
    }
    // restart sntp client regardless of easy setup
    else if (wakeupSource == WAKEUP_RESET || wakeupSource == WAKEUP_SOURCE_POR) {
        isSntpNeed = 1;
    }

    if (isSntpNeed == 1) {
        set_sntp_use(0);
        vTaskDelay(5);
        set_sntp_use(1);
    }

    while (1) {
        if (is_sntp_sync()) {
            da16x_time64(NULL, &now);
            save_sntp_time_nvram((unsigned long)now);

            PRINTF("[Saved SNTP Time in NVRAM] : %u \n", now);

            ts = (struct tm*)da16x_localtime64(&now);

            da16x_strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", ts);
            PRINTF("SNTP Sync: %s : checked\n", buf);

            isSntpSuccess = 1;
            if (write_nvram_int(NV_SNTP_SUCCESS_FLAG, isSntpSuccess))
                PRINTF("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

            set_sntp_use(0);

            break;
        } else {
            if (loop_cnt++ > _sntpCount) {
                PRINTF("SNTP failed (cnt=%d)\n", loop_cnt - 1);

                delete_sntp_time_nvram();
                isSntpSuccess = 0;
                if (write_nvram_int(NV_SNTP_SUCCESS_FLAG, isSntpSuccess))
                    PRINTF("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

                if (_sleepMode == SLEEP_MODE_1)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, 0);
                else if (_sleepMode == SLEEP_MODE_2)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, 0);
                else
                    app_common_goto_sleep_1_or_2(SLEEP_MODE_2, 1, _sleepInterval, 0);
            }
            vTaskDelay(50);
        }
    }
}

int32_t app_common_save_time_sync_for_fast_sntp_with_nvram(void)
{
    __time64_t now;
    int res;

    da16x_time64(NULL, &now);
    res = save_sntp_time_nvram((unsigned long)now);

    if (res == 0) {
        PRINTF("saved time before sleep : %u \n", now);
        return 1;
    } else {
        PRINTF("save sntp time fail !!\n");
        return 0;
    }
}

int32_t app_common_goto_sleep_1_or_2(UINT8 _sleepMode, UINT8 _useRTM, INT32 _wakeupIntervalSec, UINT8 _timeSave)
{
    if (_sleepMode == SLEEP_MODE_NONE || _sleepMode > SLEEP_MODE_2) {
        PRINTF("[%s:%d] invalid sleep mode (=%d): failed to sleep\n", __func__, __LINE__, _sleepMode);
        return 1;
    } else if (_wakeupIntervalSec <= 0) {
        PRINTF("[%s:%d] invalid wakeup timer interval seconds (=%d): failed to sleep\n", __func__, __LINE__,
            _wakeupIntervalSec);
        return 1;
    } else if (_useRTM != 0 && _useRTM != 1) {
        PRINTF("[%s:%d] invalid RTM used flag (=%d): failed to sleep\n", __func__, __LINE__, _useRTM);
        return 1;
    }

    if (_sleepMode == SLEEP_MODE_1) {
        PRINTF("[%s:%d] go to sleep mode 1...\n", __func__, __LINE__);
        vTaskDelay(1);
        fc80211_da16x_pri_pwr_down(_useRTM);
    } else if (_sleepMode == SLEEP_MODE_2) {
        if (_useRTM == 0 && _timeSave == 1)
            app_common_save_time_sync_for_fast_sntp_with_nvram();

#if defined ( __SUPPORT_UPSS__ )
        //rf power save off
        APP_Power_Save_Stop();
        app_print_elapse_time_ms("[%s:%d] APP_Power_Save_Stop called", __func__, __LINE__);
#endif // __SUPPORT_UPSS__

        PRINTF("[%s:%d] go to sleep mode 2...\n", __func__, __LINE__);
        vTaskDelay(1);
        dpm_sleep_start_mode_2((unsigned long long)_wakeupIntervalSec * 1000 * 1000, _useRTM);
    }

    return 0;
}

/* EOF */
