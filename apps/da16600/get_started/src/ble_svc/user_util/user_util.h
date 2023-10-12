/**
 ****************************************************************************************
 *
 * @file user_util.h
 *
 * @brief Header file for BLE utility
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

#ifndef COMBO_USER_UTILITIES_H_
#define COMBO_USER_UTILITIES_H_

#include "sdk_type.h"

#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
#include "stdbool.h"
#include "timers.h"

#define IDLE_DURATION_1_SEC						(100)	/* 1 sec */
#define IDLE_DURATION_AFTER_CONNECTION			(30)
#define IDLE_DURATION_AFTER_SCAN_RESULT_READ	(90)

void ble_peer_inact_chk_expire_cb(TimerHandle_t xTimer);

uint8_t app_ble_peer_idle_timer_create(void);

uint8_t app_set_ble_peer_idle_timer(uint8_t sec);

void app_del_ble_peer_idle_timer(void);

#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */

#if defined (__DA14531_BOOT_FROM_UART__)
uint8_t ota_fw_update_combo(void);
#endif

extern EventGroupHandle_t  evt_grp_wifi_conn_notify;
extern short			wifi_conn_fail_reason;

#ifdef __SUPPORT_ATCMD__
extern void app_adv_start(void);
extern void app_cancel(void);
#endif // __SUPPORT_ATCMD__

#define	EVT_BLE_ADV_CANCELLED	0x01
#define	EVT_BLE_DISCONNECTED	0x02

void app_set_ble_adv_status(int new_status);
int app_is_ble_in_advertising(void);

void app_set_ble_evt(unsigned int evt);
int app_is_no_adv_forced(void);

int combo_set_sleep2(char* context, int rtm, int seconds);

#endif /* COMBO_USER_UTILITIES_H_ */
