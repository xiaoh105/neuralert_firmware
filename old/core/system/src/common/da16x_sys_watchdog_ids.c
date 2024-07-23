/**
 ****************************************************************************************
 *
 * @file da16x_sys_watchdog_ids.c
 *
 * @brief 
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

#include "da16x_sys_watchdog_ids.h"

#undef  ENABLE_DA16X_SYS_WATCHDOG_IDS_DBG_INFO

#define DA16X_SYS_WDOG_IDS_DBG  Printf

#if defined (ENABLE_DA16X_SYS_WATCHDOG_IDS_DBG_INFO)
#define DA16X_SYS_WDOG_IDS_INFO(fmt, ...)   \
    do { \
        DA16X_SYS_WDOG_IDS_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__);   \
    } while (0)
#else
#define DA16X_SYS_WDOG_IDS_INFO(...)    do {} while (0)
#endif  // ENABLE_DA16X_SYS_WATCHDOG_DBG_INFO

#if defined(__SUPPORT_SYS_WATCHDOG__)
static const int da16x_sys_wdog_id_active = pdTRUE;
#else
static const int da16x_sys_wdog_id_active = pdFALSE;
#endif // __SUPPORT_SYS_WATCHDOG__

static int da16x_sys_wdog_id_system_launcher = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_lmacMain = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_UmacRx = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_LwIP_init = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_wifi_ev_mon = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_da16x_supp = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_Console_OUT = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_Console_IN = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_Host_drv = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_dpm_sleep_daemon = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_OTA_update = DA16X_SYS_WDOG_ID_DEF_ID;
static int da16x_sys_wdog_id_MQTT_Sub = DA16X_SYS_WDOG_ID_DEF_ID;

static int da16x_sys_wdog_id_is_active()
{
    return da16x_sys_wdog_id_active;
}

int da16x_sys_wdog_id_set_system_launcher(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_system_launcher = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_system_launcher(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_system_launcher;
}

int da16x_sys_wdog_id_set_lmacMain(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_lmacMain = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_lmacMain(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_lmacMain;
}

int da16x_sys_wdog_id_set_UmacRx(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_UmacRx = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_UmacRx(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_UmacRx;
}

int da16x_sys_wdog_id_set_LwIP_init(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_LwIP_init = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_LwIP_init(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_LwIP_init;
}

int da16x_sys_wdog_id_set_wifi_ev_mon(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_wifi_ev_mon = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_wifi_ev_mon(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_wifi_ev_mon;
}

int da16x_sys_wdog_id_set_da16x_supp(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_da16x_supp = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_da16x_supp(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_da16x_supp;
}

int da16x_sys_wdog_id_set_Console_OUT(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_Console_OUT = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_Console_OUT(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_Console_OUT;
}

int da16x_sys_wdog_id_set_Console_IN(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_Console_IN = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_Console_IN(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_Console_IN;
}

int da16x_sys_wdog_id_set_Host_drv(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_Host_drv = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_Host_drv(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_Host_drv;
}

int da16x_sys_wdog_id_set_dpm_sleep_daemon(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_dpm_sleep_daemon = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_dpm_sleep_daemon(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_dpm_sleep_daemon;
}

int da16x_sys_wdog_id_set_OTA_update(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_OTA_update = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_OTA_update(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_OTA_update;
}

int da16x_sys_wdog_id_set_MQTT_Sub(int id)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    da16x_sys_wdog_id_MQTT_Sub = id;

    DA16X_SYS_WDOG_IDS_INFO("ID: %d\n", id);

    return 0;
}

int da16x_sys_wdog_id_get_MQTT_Sub(void)
{
    if (!da16x_sys_wdog_id_is_active()) {
        return DA16X_SYS_WDOG_ID_DEF_ID;
    }

    return da16x_sys_wdog_id_MQTT_Sub;
}
