/**
 ****************************************************************************************
 *
 * @file supp_main.c
 *
 * @brief main entry point of DA16X supplicant
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


#include "sdk_type.h"  // After app_features.h

#include "includes.h"

#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "p2p_supplicant.h"

#include "supp_eloop.h"
#include "supp_config.h"
#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"
#include "oal.h"
#include "common_def.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

extern void wpas_p2p_group_add_cb(void *eloop_ctx, void *timeout_ctx);
extern void wpas_p2p_find_cb(void *eloop_ctx, void *timeout_ctx);

static int	da16x_run_mode = STA_ONLY_MODE;
#ifdef CONFIG_FAST_RECONNECT_V2
u8	fast_reconnect_tries;
u8	da16x_is_fast_reconnect = 0;
#endif
char *da16x_driver = "fc80211";
#ifdef	CONFIG_BRIDGE_IFACE
char *bridge_ifname = "br0";
#endif	/* CONFIG_BRIDGE_IFACE */


extern UCHAR	runtime_cal_flag;

int get_run_mode(void)
{
	return da16x_run_mode;
}

void set_run_mode(int mode)
{
	da16x_run_mode = mode;
}

#ifdef CONFIG_AP
	extern struct wpa_supplicant * wpas_add_softap_interface(struct wpa_supplicant *wpa_s);
#endif /* CONFIG_AP */


static void da16x_supplicant_thread_entry(void * pvPraram)
{
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;
	struct wpa_interface *ifaces;
	struct wpa_params params;
	struct wpa_global *global;
	struct wpa_supplicant *wpa_s;
#ifdef CONFIG_AP
	struct wpa_supplicant *wpa_s_new;
#endif /* CONFIG_AP */

	extern void printf_with_run_time(char * string);

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
	printf_with_run_time("Run SUPP thread");
#else
	if (runtime_cal_flag == pdTRUE)
		PRINTF(">>> Start DA16X Supplicant ...\n");
#endif

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
	if (sys_wdog_id >= 0) {
		da16x_sys_wdog_id_set_da16x_supp(sys_wdog_id);
	}

	da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_da16x_supp());

	/* Currently,,, DPM will be operated on STA mode */
	if (get_run_mode() != STA_ONLY_MODE) {
		disable_dpm_mode();
	}

	if (chk_dpm_mode() == pdTRUE) {
		int	bit_idx;

		/* Register DPM_SLEEP operation */
		bit_idx = reg_dpm_sleep(REG_NAME_DPM_SUPPLICANT, 0);
		if (bit_idx == DPM_REG_ERR) {
			da16x_err_prt("[%s] reg_dpm_sleep(REG_NAME_DPM_SUPPLICANT) "
					"error\n", __func__);
		}
	}

	os_memset(&params, 0, sizeof(params));

	ifaces = os_malloc(sizeof(struct wpa_interface));

	if (ifaces == NULL) {
		da16x_sys_watchdog_unregister(da16x_sys_wdog_id_get_da16x_supp());
		da16x_sys_wdog_id_set_da16x_supp(DA16X_SYS_WDOG_ID_DEF_ID);
		return;
	}

	memset(ifaces, 0, sizeof(struct wpa_interface));

	ifaces->ifname = STA_DEVICE_NAME;

	ifaces->driver = da16x_driver;
#ifdef	CONFIG_BRIDGE_IFACE
	memset((char *)ifaces->bridge_ifname, 0, sizeof(ifaces->bridge_ifname));
	sprintf(ifaces->bridge_ifname, "%s", bridge_ifname);
#endif	/* CONFIG_BRIDGE_IFACE */

	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		da16x_fatal_prt(" Init Fail...\n\n");
		goto out;
	}

	wpa_s = wpa_supplicant_add_iface(global, ifaces, NULL);
	if (wpa_s == NULL) {
		da16x_err_prt(">>> Failed to add SUPP interface ...\n");
		goto supp_init_fail;
	}

	/* Run DA16X supplicant as defined mode */
	switch (get_run_mode()) {

#ifdef CONFIG_MESH
		case MESH_POINT_MODE :
		case STA_MESH_PORTAL_MODE:
			/* create new interface for soft-ap */
			if (!wpas_mesh_add_interface(wpa_s)) {
				da16x_err_prt(">>> Failed to add MP interface ...\n");
				goto supp_init_fail;
			}

			if (get_run_mode() == MESH_POINT_MODE)
			{
				if (wpa_drv_if_remove(wpa_s, WPA_IF_STATION, "wlan0")){
					da16x_err_prt(">>> Supplicant Failed to remove "
							"STA Device interface...\n");
					goto supp_init_fail;
				}

				wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);

				wpa_s_new->parent = wpa_s_new;
			}

			da16x_notice_prt("\n>>> Start MESH Point Mode...\n");
			break;

#endif /* CONFIG_MESH */


#ifdef  CONFIG_AP
		case AP_ONLY_MODE :
			/* create new interface for soft-ap */
			wpa_s_new = wpas_add_softap_interface(wpa_s);
			if (!wpa_s_new) {
				da16x_err_prt(">>> Failed to add SUPP Soft-AP interface...\n");
				goto supp_init_fail;
			}
#ifdef CONFIG_OWE_TRANS
			if (wpa_s_new->conf->owe_transition_mode == 1)
				break;
#endif // CONFIG_OWE_TRANS
			if (wpa_drv_if_remove(wpa_s, WPA_IF_STATION, "wlan0")) {
				da16x_err_prt(">>> Failed to remove SUPP STA interface...\n");
				goto supp_init_fail;
			}

			wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
			wpa_s_new->parent = wpa_s_new;

			//da16x_notice_prt("\n>>> Start Soft-AP mode...\n");
			break;
#endif  /* CONFIG_AP */

#if defined(CONFIG_P2P) && defined(CONFIG_AP)
		case P2P_ONLY_MODE :
			wpa_s_new = wpas_p2p_add_p2pdev_interface(wpa_s);
			if (!wpa_s_new) {
				da16x_notice_prt(">>> Failed to add SUPP P2P interface ...\n");
				goto supp_init_fail;
			}

			if (wpa_drv_if_remove(wpa_s, WPA_IF_STATION, "wlan0")) {
				da16x_notice_prt(">>> Failed to remove SUPP interface...\n");
				goto supp_init_fail;
			}

			wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
			wpa_s_new->parent = wpa_s_new;

			da16x_notice_prt("\n>>> Start Wi-Fi Direct mode...\n");

#if 1
			da16x_eloop_register_timeout(0, 100000,
						wpas_p2p_find_cb, wpa_s_new, NULL);
#else
			p2p_ctrl_find(wpa_s);
#endif /* 1 */
			break;

		case P2P_GO_FIXED_MODE :
			wpa_s_new = wpas_p2p_add_p2pdev_interface(wpa_s);
			if (!wpa_s_new) {
				da16x_notice_prt(">>> Failed to add SUPP P2P interface...\n");
				goto supp_init_fail;
			}

			if (wpa_drv_if_remove(wpa_s, WPA_IF_STATION, "wlan0")){
				da16x_notice_prt(">>> Failed to remove SUPP STA interface...\n");
				goto supp_init_fail;
			}
			wpa_supplicant_remove_iface(wpa_s->global, wpa_s, 0);
			wpa_s_new->parent = wpa_s_new;

			da16x_notice_prt("\n>>> Start P2P GO fixed mode...\n");

#if 1
			da16x_eloop_register_timeout(0, 100000,
						wpas_p2p_group_add_cb, wpa_s_new, NULL);

#else
			wpas_p2p_group_add(wpa_s_new, 0, 0, 0, 0);

#endif /* 1 */

			break;
#endif	/* defined(CONFIG_P2P) && defined(CONFIG_AP) */

#ifdef	CONFIG_CONCURRENT
#if defined(CONFIG_STA) && defined(CONFIG_AP)
		case STA_SOFT_AP_CONCURRENT_MODE :
			/* create new interface for soft-ap */
			if (!wpas_add_softap_interface(wpa_s)) {
				da16x_notice_prt(">>> Supplicant fail to add "
						"Soft-AP interface...\n");
				goto supp_init_fail;
			}

			da16x_notice_prt("\n>>> Start STA & SOFT-AP Concurrent mode...\n");
			break;
#endif /* defined(CONFIG_STA) && defined(CONFIG_AP) */

#if defined(CONFIG_STA) && defined(CONFIG_P2P) && defined(CONFIG_AP)
		case STA_P2P_CONCURRENT_MODE :
			/* Insert proper code for sta:p2p concurrent mode if needed */
			wpa_s_new = wpas_p2p_add_p2pdev_interface(wpa_s);
			if (!wpa_s_new) {
				da16x_notice_prt(">>> Supplicant fail to add P2P interface...\n");
				goto supp_init_fail;
			}

			da16x_notice_prt("\n>>> Start STA & P2P Concurrent mode...\n");
			os_sleep(0, 100000);
#if 1
			da16x_eloop_register_timeout(5, 0,
						wpas_p2p_find_cb, wpa_s_new, NULL);
#else
			p2p_ctrl_find(wpa_s);
#endif /* 1 */
			break;
#endif /* defined(CONFIG_STA) && defined(CONFIG_P2P) && defined(CONFIG_AP) */
#endif	/* CONFIG_CONCURRENT */

#ifdef CONFIG_STA
		case STA_ONLY_MODE :
			if (chk_dpm_wakeup() == pdTRUE) {
				da16x_dpm_prt(">>> Start STA mode (with DPM) : Count=%d\n", current_usec_count());
			} else {
				da16x_notice_prt(">>> Start STA mode...\n");
			}

			break;
#endif /* CONFIG_STA */

		default :
			da16x_notice_prt(">>> Unsupported mode...\n");
			wpa_drv_if_remove(wpa_s, WPA_IF_STATION, "wlan0");
			goto supp_init_fail;
	}

	wpa_supplicant_run(global, global->ifaces);

supp_init_fail :
	wpa_supplicant_deinit(global);
out:
	os_free(ifaces);

	da16x_sys_watchdog_unregister(da16x_sys_wdog_id_get_da16x_supp());
	da16x_sys_wdog_id_set_da16x_supp(DA16X_SYS_WDOG_ID_DEF_ID);

	vTaskDelete(NULL);
}

/**************************************************/
/* Supp mem pool space */
#define DA16X_SP_POOL_SIZE	(128*8 + 1024)	//(6 * 1024)	// 4 -> 6. 20181017
#define DA16X_SP_POOL_MESH_PORTAL_SIZE (DA16X_SP_POOL_SIZE+1024)
#define DA16X_SP_PRIORITY	OS_TASK_PRIORITY_SYSTEM+7	//18//22

/* Define FreeRTOS thread control block */
TaskHandle_t *da16x_sp_thread = NULL;

int start_da16x_supp(void)
{
	BaseType_t status;

	PRINTF(">>> Start DA16X Supplicant ...\n");


	da16x_sp_thread = (TaskHandle_t *)pvPortMalloc(sizeof(TaskHandle_t));

	status = xTaskCreate(da16x_supplicant_thread_entry,
                                "da16x_supp",
                                DA16X_SP_POOL_SIZE,
                                (void *) NULL,
                                DA16X_SP_PRIORITY,
                                da16x_sp_thread);

	return status;
}

void get_supp_ver(char *buf)
{
	if (buf == NULL) {
		da16x_notice_prt("[%s] NULL pointer...\n", __func__);
		return;
	}

	sprintf(buf, "%s %s", VER_NUM, RELEASE_VER_STR);
}

/* EOF */
