/**
 ****************************************************************************************
 *
 * @file da16x_sys_watchdog_ids.h
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

#ifndef __DA16X_SYS_WATCHDOG_IDS_H__
#define __DA16X_SYS_WATCHDOG_IDS_H__

#include "sdk_type.h"
#include "common_def.h"
#include "da16x_system.h"
#include "da16x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DA16X_SYS_WDOG_ID_DEF_ID    (-1)

int da16x_sys_wdog_id_set_system_launcher(int id);
int da16x_sys_wdog_id_get_system_launcher(void);
int da16x_sys_wdog_id_set_lmacMain(int id);
int da16x_sys_wdog_id_get_lmacMain(void);
int da16x_sys_wdog_id_set_UmacRx(int id);
int da16x_sys_wdog_id_get_UmacRx(void);
int da16x_sys_wdog_id_set_LwIP_init(int id);
int da16x_sys_wdog_id_get_LwIP_init(void);
int da16x_sys_wdog_id_set_wifi_ev_mon(int id);
int da16x_sys_wdog_id_get_wifi_ev_mon(void);
int da16x_sys_wdog_id_set_da16x_supp(int id);
int da16x_sys_wdog_id_get_da16x_supp(void);
int da16x_sys_wdog_id_set_Console_OUT(int id);
int da16x_sys_wdog_id_get_Console_OUT(void);
int da16x_sys_wdog_id_set_Console_IN(int id);
int da16x_sys_wdog_id_get_Console_IN(void);
int da16x_sys_wdog_id_set_Host_drv(int id);
int da16x_sys_wdog_id_get_Host_drv(void);
int da16x_sys_wdog_id_set_dpm_sleep_daemon(int id);
int da16x_sys_wdog_id_get_dpm_sleep_daemon(void);
int da16x_sys_wdog_id_set_OTA_update(int id);
int da16x_sys_wdog_id_get_OTA_update(void);
int da16x_sys_wdog_id_set_MQTT_Sub(int id);
int da16x_sys_wdog_id_get_MQTT_Sub(void);

#ifdef __cplusplus
}
#endif

#endif // __DA16X_SYS_WATCHDOG_IDS_H__
