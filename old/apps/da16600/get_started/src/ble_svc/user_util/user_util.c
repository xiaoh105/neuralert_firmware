/**
 ****************************************************************************************
 *
 * @file user_util.c
 *
 * @brief Basic user util functions used in the sample application.
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

#include <stdbool.h>
#include <stdlib.h>

#include "FreeRTOSConfig.h"
#include "common_def.h"
#include "limits.h"
#include "sdk_type.h"


#ifdef __BLE_COMBO_REF__

#include "da16x_types.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "rwble_hl_config.h"
#include "da16x_system.h"
#include "da16_gtl_features.h"
#include "user_util.h"
#include "util_api.h"
#include "user_dpm_api.h"
#include "../include/console.h"
#ifdef __SUPPORT_ATCMD__
#include "atcmd_ble.h"
#endif
#if defined (__BLE_PEER_INACTIVITY_CHK_TIMER__)
static TimerHandle_t peer_inactivity_chk_timer;
static bool chk_timer_enable = 0;
static uint8_t chk_timer_time = 0;

void ble_peer_inact_chk_expire_cb(TimerHandle_t xTimer)
{
	  DBG_INFO("<-- Inactive timer expired!\n");
	  ConsoleSendDisconnnect();
}

uint8_t app_ble_peer_idle_timer_create(void)
{
    peer_inactivity_chk_timer = xTimerCreate("Ble-Inactive timer",
		            IDLE_DURATION_AFTER_CONNECTION * IDLE_DURATION_1_SEC,
        		    pdFALSE,
		            (void*)0,
        		    ble_peer_inact_chk_expire_cb);
    configASSERT(peer_inactivity_chk_timer);
    chk_timer_time = IDLE_DURATION_AFTER_CONNECTION;

    return pdFREERTOS_ERRNO_NONE;
}

uint8_t app_set_ble_peer_idle_timer(uint8_t sec)
{
	if (peer_inactivity_chk_timer) {
		if (chk_timer_enable) {
			xTimerStop(peer_inactivity_chk_timer, 0);
		}

		if (chk_timer_time != sec) {
			xTimerChangePeriod(peer_inactivity_chk_timer, sec * IDLE_DURATION_1_SEC, 0);
		} else {
			xTimerStart(peer_inactivity_chk_timer, 0);
		}

		DBG_INFO("--> Inactive timer started(%ds)!\n", sec);
		chk_timer_enable = TRUE;
		chk_timer_time = sec;

		return pdFREERTOS_ERRNO_NONE;
	}

	return pdFREERTOS_ERRNO_EINVAL;
}

void app_del_ble_peer_idle_timer(void)
{
	if (peer_inactivity_chk_timer && chk_timer_enable) {
		xTimerStop(peer_inactivity_chk_timer, 0);
		chk_timer_enable = FALSE;

		DBG_INFO("peer_inactivity_chk_timer disabled!\r\n");
	}
}

#endif /* __BLE_PEER_INACTIVITY_CHK_TIMER__ */

#if defined (__SUPPORT_OTA__)
OTA_UPDATE_CONFIG _fw_conf = { 0, };
OTA_UPDATE_CONFIG *fw_conf = (OTA_UPDATE_CONFIG *) &_fw_conf;

void ota_sample_download_cb(ota_update_type update_type, UINT status, UINT progress)
{
	DA16X_UNUSED_ARG(progress);

	if (status == OTA_SUCCESS) {
		DBG_INFO("[%s] <%s> Succeeded to download. \n", __func__, ota_update_type_to_text(update_type));
	} else {
		DBG_ERR("[%s] <%s> Failed to download. (Err : 0x%02x)\n", __func__, ota_update_type_to_text(update_type), status);
	}
}
void ota_sample_renew_cb(UINT status)
{
	if (status == OTA_SUCCESS) {
		DBG_INFO("[%s] Succeeded to replace with new FW.  \n", __func__);
	} else {
		DBG_ERR("[%s] Failed to replace with new FW. (Err : 0x%02x)\n", __func__, status);
	}
}

/* ------------------------------------------------
 * Check RTOS/BLE image addresses,
 *	then start update both of them if there are all of them.
 *	if there is either of them, update only one, else do nothing.
 * ------------------------------------------------
 */
uint8_t ota_fw_update_combo(void)
{
	uint8_t status = OTA_SUCCESS;
	char *read_buf = NULL;
	int read_len = 0;
	bool rtos_image = false;
	bool ble_image = false;

	read_buf = read_nvram_string("URI_RTOS");
	read_len = strlen(read_buf);
	if ((read_len < 256) && (read_len > 0)) {
		memset(fw_conf->url, 0x00, OTA_HTTP_URL_LEN);
		memcpy(fw_conf->url, read_buf, read_len + 1);
		rtos_image = true;
		DBG_INFO("[%s] RTOS url = %s\n", __func__, fw_conf->url);
	}

	read_buf = read_nvram_string("URI_BLE");
	read_len = strlen(read_buf);
	if ((read_len < 256) && (read_len > 0)) {
		memset(fw_conf->url_ble_fw, 0x00, OTA_HTTP_URL_LEN);
		memcpy(fw_conf->url_ble_fw, read_buf, read_len + 1);
		ble_image = true;
		DBG_INFO("[%s] BLE url = %s\n", __func__, fw_conf->url_ble_fw);
	}

	if ((rtos_image == true) && (ble_image == true)) {
		/* RTOS and BLE */
		fw_conf->update_type = OTA_TYPE_BLE_COMBO;

	} else if ((rtos_image == true) && (ble_image != true)) {
		/* RTOS only */
		fw_conf->update_type = OTA_TYPE_RTOS;

	} else if ((rtos_image != true) && (ble_image == true)) {
		/* BLE only */
		fw_conf->update_type = OTA_TYPE_BLE_FW;

	} else {
		status = OTA_ERROR_URL;
		goto ret_status;
	}

	fw_conf->download_notify = ota_sample_download_cb;
	fw_conf->renew_notify = ota_sample_renew_cb;

	/* FW renew (reboot) after successful  download*/
	fw_conf->auto_renew = 1;

	status = ota_update_start_download(fw_conf);

ret_status:

	if (status) {
		DBG_ERR("[%s] RTOS OTA update failed (0x%02x)\n", __func__, status);
	}

	return status;
}
#else
uint8_t ota_fw_update_combo(void)
{
	return 0;
}
#endif // __SUPPORT_OTA__

#ifdef __SUPPORT_ATCMD__
int atcmd_bleperi_cmd_handler(uint8_t cmd)
{
#if defined (__BLE_FEATURE_ENABLED__)
	switch (cmd) {
	case ATCMD_BLE_ADV_STOP:
	    DBG_INFO( "Send CMD: stop Advertising\n");
	    app_cancel();
	    break;

	case ATCMD_BLE_ADV_START:
	    DBG_INFO( "Send CMD: start advertising\n");
	    app_adv_start();
	    break;
	default:
	    return AT_CMD_ERR_UNKNOWN_CMD;
	}

	return AT_CMD_ERR_CMD_OK;
#else
	DA16X_UNUSED_ARG(cmd);
	return AT_CMD_ERR_NOT_SUPPORTED;
#endif // __BLE_FEATURE_ENABLED__
}
#endif // __SUPPORT_ATCMD__

static int ble_in_advertising = pdFALSE;
static EventGroupHandle_t evt_grp_ble_app;
static int app_force_no_adv = false;

extern void app_cancel(void);
extern int app_is_ble_in_connected(void);

#if defined (__BLE_FEATURE_ENABLED__) && !defined (__BLE_CENT_SENSOR_GW__)
extern void cmd_ble_disconnect(int argc, char *argv[]);
#else
void cmd_ble_disconnect(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);
}
#endif // defined (__BLE_FEATURE_ENABLED__) && !defined (__BLE_CENT_SENSOR_GW__)

void app_set_ble_adv_status(int new_status)
{
	ble_in_advertising = new_status;
	return;
}

int app_is_ble_in_advertising(void)
{
	return ble_in_advertising;
}

void app_set_ble_evt(unsigned int evt) 
{
	if (evt_grp_ble_app == NULL) {
		 evt_grp_ble_app = xEventGroupCreate();
	}

	xEventGroupSetBits(evt_grp_ble_app, evt);
}

int app_is_no_adv_forced(void) 
{
	return app_force_no_adv;
}

#if defined (__BLE_FEATURE_ENABLED__) && !defined (__BLE_CENT_SENSOR_GW__)

static void app_set_adv_forced(int value)
{
	app_force_no_adv = value;
}

static int app_do_and_wait_until_ble_evt_complete(unsigned int evt)
{
	EventBits_t events = 0;

	if (evt_grp_ble_app == NULL) {
		 evt_grp_ble_app = xEventGroupCreate();
	}

	if (evt == EVT_BLE_ADV_CANCELLED) {
		app_cancel();
	} else if (evt == EVT_BLE_DISCONNECTED) {
		cmd_ble_disconnect(1, NULL);
	}
	
	events = xEventGroupWaitBits(evt_grp_ble_app, evt, pdTRUE, pdFALSE, portMAX_DELAY);
					
	if (events & evt) {
		vTaskDelay(30);		
		return 0;
	} else {
		PRINTF("[%s] Wait BLE EVT (0x%x) failed! \n", __func__, evt);
		vTaskDelay(10);
		return -1;
	}

}


/* ---------------------------------------------------------------------------------------
 * Set sleep2 on DA16600 board environment
 *     return == 1   // Success: caller should proceed to the remaining code
 *		      <  0   // Error  : caller should stop processing and return error
 *		      == 0   // Success: caller lets DPM Daemon handle sleep1/sleep2 
 *						   caller should return immediatly as success 
 * 						   without proceeding to the remaining code
 * ---------------------------------------------------------------------------------------
 */
int combo_set_sleep2(char* context, int rtm, int seconds)
{
	int is_dpm_mode, is_ble_advertising, is_ble_connected;
	int ret, is_prof_exist, is_prof_disabled;

	is_dpm_mode = dpm_mode_is_enabled();
	is_ble_connected = 	app_is_ble_in_connected();

	if (is_ble_connected) {
		app_set_adv_forced(true);
		ret = app_do_and_wait_until_ble_evt_complete(EVT_BLE_DISCONNECTED);
		if (ret != 0) { // error
			return ret;
		}
	} else  {
		is_ble_advertising = app_is_ble_in_advertising();
		if (is_ble_advertising) {
			ret = app_do_and_wait_until_ble_evt_complete(EVT_BLE_ADV_CANCELLED);
			if (ret != 0) { // error
				return ret;
			}
		}
	} 

	if (is_dpm_mode) {	
		if (rtm == 1) {
			ret = wifi_chk_prov_state(&is_prof_exist, &is_prof_disabled);
			if (ret == 0) {
				combo_make_sleep_rdy();
				if (is_prof_exist && !is_prof_disabled) {
					return 1;
				} else {
					/* AP info NOT provisioned, pass thru ... */
				}
			} else { // error
				return ret;
			}
		} else {
			return 1;
		}
	
		let_dpm_daemon_trigger_sleep2(context, seconds);
		return 0;
	} else {
		return 1;
	}
}
#else 
int combo_set_sleep2(char* context, int rtm, int seconds)
{
	DA16X_UNUSED_ARG(context);
	DA16X_UNUSED_ARG(rtm);
	DA16X_UNUSED_ARG(seconds);

	return 1;
}
#endif // defined (__BLE_FEATURE_ENABLED__) && !defined (__BLE_CENT_SENSOR_GW__)

#endif // __BLE_COMBO_REF__

/* EOF */
