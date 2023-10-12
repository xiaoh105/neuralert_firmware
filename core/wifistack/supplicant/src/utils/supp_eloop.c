/**
 ****************************************************************************************
 *
 * @file supp_eloop.c
 *
 * @brief Driver interaction with DA16X fc80211
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


#include "includes.h"

#include "fc80211_copy.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"
#include "supp_eloop.h"
#include "supp_config.h"
#include "supp_scan.h"
#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"

#include "common_def.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wunused-variable"

extern UCHAR	dpm_slp_time_reduce_flag;

/* For Event Group */
EventGroupHandle_t    da16x_sp_event_group;

QueueHandle_t	da16x_drv_msg_tx_q;
QueueHandle_t	da16x_probe_msg_tx_q;
QueueHandle_t	da16x_cli_msg_tx_q;
QueueHandle_t	da16x_cli_msg_rx_q;

int da16x_sp_msg_q_created = 0;
da16x_cli_rsp_buf_t cli_rsp_buf;	/* Supplicant -> CLI */

UCHAR *da16x_drv_tx_msg_q_buf;
UCHAR *da16x_probe_msg_q_buf;

#ifdef CONFIG_SCAN_REPLY_OPTIMIZE
char	*cli_rx_ev_data = NULL;
#else
da16x_cli_rx_ev_data_t *cli_rx_ev_data;
#endif /* CONFIG_SCAN_REPLY_OPTIMIZE */

QueueHandle_t TO_SUPP_QUEUE;
QueueHandle_t TO_CLI_QUEUE;
ULONG	TO_SUPP_QUEUE_BUF[TO_SUPP_QUEUE_SIZE*sizeof(ULONG)*2];
ULONG	TO_CLI_QUEUE_BUF[TO_CLI_QUEUE_SIZE*sizeof(ULONG)*2];

SemaphoreHandle_t da16x_drv_ev_mutex;
UCHAR	*da16x_drv_ev_mutex_name = "drv_ev_mutex";

struct da16x_eloop_data {
	struct dl_list timeout;
};

static struct da16x_eloop_data da16x_eloop;

struct da16x_eloop_timeout {
	struct dl_list list;
	struct os_reltime time;
	void *eloop_data;
	void *user_data;
	da16x_eloop_timeout_handler handler;
};

#ifndef CONFIG_MONITOR_THREAD_EVENT_CHANGE
EventGroupHandle_t wifi_monitor_event_group;
#endif // CONFIG_MONITOR_THREAD_EVENT_CHANGE
QueueHandle_t wifi_monitor_event_q;

extern void da16x_drv_event_handler(da16x_drv_msg_buf_t *drv_msg_buf);
extern void driver_fc80211_process_global_ev(da16x_drv_msg_buf_t *drv_msg_buf);
extern void wpa_supp_global_ctrl_iface_receive(struct wpa_global *global, char *buf);
extern void do_dpm_autoarp_send(void);


int da16x_eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   da16x_eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
	struct da16x_eloop_timeout *timeout, *tmp;
	os_time_t now_sec;

	timeout = os_zalloc(sizeof(*timeout));
	if (timeout == NULL) {
		da16x_eloop_prt("[%s] os_zalloc fail\n", __func__);
		return -1;
	}

	if (os_get_reltime(&timeout->time) < 0) {
		os_free(timeout);

		da16x_eloop_prt("[%s] os_get_reltime fail\n", __func__);
		return -1;
	}
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) {
		/*
		 * Integer overflow - assume long enough timeout to be assumed
		 * to be infinite, i.e., the timeout would never happen.
		 */
		da16x_eloop_prt("[%s] Too long timeout "
				"(secs=%u) to ever happen - ignore it\n",
					__func__, secs);
		os_free(timeout);
		return -1;
	}

#if 0
	da16x_eloop_prt("[%s] time.sec=%lu, now_sec=%lu\n",
				__func__, timeout->time.sec, now_sec);
#endif /* 0 */

	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;

	/* Maintain timeouts in order of increasing time */
	dl_list_for_each(tmp, &da16x_eloop.timeout,
				struct da16x_eloop_timeout, list) {
		if (os_reltime_before(&timeout->time, &tmp->time)) {
			dl_list_add(tmp->list.prev, &timeout->list);
			return 0;
		}
	}

#if 0
	da16x_eloop_prt("[%s] Add timer to eloop.timeout\n", __func__);
#endif /* 0 */

	dl_list_add_tail(&da16x_eloop.timeout, &timeout->list);

	return 0;
}


static void da16x_eloop_remove_timeout(struct da16x_eloop_timeout *timeout)
{
	dl_list_del(&timeout->list);
	os_free(timeout);
}

int da16x_eloop_cancel_timeout(da16x_eloop_timeout_handler handler,
			 void *da16x_eloop_data, void *user_data)
{
	struct da16x_eloop_timeout *timeout, *prev;
	int removed = 0;

	TX_FUNC_START("");

	dl_list_for_each_safe(timeout, prev, &da16x_eloop.timeout,
			      struct da16x_eloop_timeout, list) {
		if (timeout->handler == handler) {
			if (   timeout->eloop_data == da16x_eloop_data
			    || da16x_eloop_data == ELOOP_ALL_CTX) {
				if (   timeout->user_data == user_data
				    || user_data == ELOOP_ALL_CTX) {
					da16x_eloop_remove_timeout(timeout);
					removed++;
				}

			}
		}
	}

	TX_FUNC_END("");

	return removed;
}


int da16x_eloop_cancel_timeout_one(da16x_eloop_timeout_handler handler,
			     void *eloop_data, void *user_data,
			     struct os_reltime *remaining)
{
	struct da16x_eloop_timeout *timeout, *prev;
	int removed = 0;
	struct os_reltime now;

	os_get_reltime(&now);
	remaining->sec = remaining->usec = 0;

	dl_list_for_each_safe(timeout, prev, &da16x_eloop.timeout,
			      struct da16x_eloop_timeout, list) {
		if (timeout->handler == handler &&
		    (timeout->eloop_data == eloop_data) &&
		    (timeout->user_data == user_data)) {
			removed = 1;
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, remaining);
			da16x_eloop_remove_timeout(timeout);
			break;
		}
	}
	return removed;
}


int da16x_eloop_is_timeout_registered(da16x_eloop_timeout_handler handler,
				void *da16x_eloop_data, void *user_data)
{
	struct da16x_eloop_timeout *tmp;

	dl_list_for_each(tmp, &da16x_eloop.timeout, struct da16x_eloop_timeout, list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == da16x_eloop_data &&
		    tmp->user_data == user_data)
			return 1;
	}

	return 0;
}

int da16x_eloop_deplete_timeout(unsigned int req_secs,
				unsigned int req_usecs,
			  	da16x_eloop_timeout_handler handler,
				void *da16x_eloop_data,
				void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct da16x_eloop_timeout *tmp;

	dl_list_for_each(tmp, &da16x_eloop.timeout, struct da16x_eloop_timeout,
									list) {
		if (tmp->handler == handler &&
		    tmp->eloop_data == da16x_eloop_data &&
		    tmp->user_data == user_data) {

			requested.sec = req_secs;
			requested.usec = req_usecs;

			os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);

			if (os_reltime_before(&requested, &remaining)) {
				da16x_eloop_cancel_timeout(handler,
							da16x_eloop_data,
							user_data);
				da16x_eloop_register_timeout(requested.sec,
							requested.usec,
							handler,
							da16x_eloop_data,
							user_data);
				return 1;
			}
			return 0;
		}
	}

	return -1;
}


/*
 * Function : da16x_drv_event_handler
 *
 * - arguments	: da16x_drv_msg_buf_t *drv_msg_buf
 * - return	: NONE
 *
 * - Discription :
 *	FC80211 driver event handler for Renesas Wi-Fi supplicant.
 */
void da16x_drv_event_handler(da16x_drv_msg_buf_t *drv_msg_buf)
{
	//RX_FUNC_START("");

	driver_fc80211_process_global_ev(drv_msg_buf);

	//RX_FUNC_END("");
}

void da16x_cli_cmd_handler(struct wpa_global *global,
				da16x_cli_tx_msg_t *cli_cmd_buf)
{
	extern void wpa_supp_global_ctrl_iface_recv(
				struct wpa_global *global, char *buf);

	wpa_supp_global_ctrl_iface_recv(global,
				cli_cmd_buf->cli_tx_buf->cmd_str);
}

#ifdef CONFIG_RECEIVE_DHCP_EVENT
void da16x_dhcp_event_handler(struct wpa_global *global,
						ULONG *data)
{	
	struct wpa_supplicant *wpa_s=NULL;

	RX_FUNC_START("");

	for (wpa_s = global->ifaces; wpa_s ;
							wpa_s = wpa_s->next) {
		if(strcmp("sta0", wpa_s->ifname) == 0) {
			if (*data == 0x00000001)
			{
				da16x_eloop_prt("[%s] DHCP event received 1\n", __func__);
				wpa_supplicant_event(wpa_s, EVENT_DHCP_NO_RESPONSE, NULL);
			}
			else if (*data == 0x00000002)
			{
				da16x_eloop_prt("[%s] DHCP event received 2\n", __func__);
				wpa_supplicant_event(wpa_s, EVENT_DHCP_ACK_OK, NULL);			
			}
		}
	}

	RX_FUNC_END("");
}
#endif /* CONFIG_RECEIVE_DHCP_EVENT */

void *init_supp_mempool(void *pointer)
{
	int size = 0;
#ifndef CONFIG_SCAN_REPLY_OPTIMIZE
	cli_rx_ev_data = os_malloc(sizeof(da16x_cli_rx_ev_data_t));
#endif	/* CONFIG_SCAN_REPLY_OPTIMIZE */
        pointer = (void *)((unsigned int)pointer + size);

        return pointer;
}

int create_supp_msg_queues () 
{
	int status;

	if (da16x_sp_msg_q_created == 0) {
		da16x_eloop_prt("[%s] Create messge queues to / from supplicant\n", __func__);
#if 0
		TO_SUPP_QUEUE = xQueueCreate(TO_SUPP_QUEUE_SIZE, sizeof(ULONG));
#else
		TO_SUPP_QUEUE = xQueueCreate(TO_SUPP_QUEUE_SIZE, sizeof(ULONG)*2);
#endif
		if (TO_SUPP_QUEUE == NULL) {
			da16x_eloop_prt("[%s] : TO_SUPP_QUEUE create fail "
				"(stauts=%d)\n", __func__, status);
			return -1;
		}

		TO_CLI_QUEUE = xQueueCreate(TO_CLI_QUEUE_SIZE, sizeof(ULONG)*2);

		if (TO_CLI_QUEUE == NULL) {
			da16x_eloop_prt("[%s] : TO_SUPP_QUEUE create fail "
				"(stauts=%d)\n", __func__, status);
			return -1;
		}

		da16x_sp_msg_q_created = 1;
	} else {
		da16x_eloop_prt("[%s] event flag group already created.\n", __func__);
	}

	return pdPASS;
}
/*
 * Function : da16x_eloop_init
 *
 * - arguments	: none
 * - return	: OK
 *
 * - Discription :
 *	Event loop init function for Renesas Wi-Fi supplicant.
 */
int da16x_eloop_init()
{
	UINT	status;

	TX_FUNC_START("");

	status = create_supp_msg_queues();

	if (status !=  pdPASS) {
		da16x_eloop_prt("[%s] : event flag group create failed "
				"(stauts=%d)\n",
				__func__, status);
		return -1;
	}

	os_memset(&da16x_eloop, 0, sizeof(da16x_eloop));
	dl_list_init(&da16x_eloop.timeout);

	TX_FUNC_END("");

	return 0;
}

/*
 * Function : da16x_eloop_run
 *
 * - arguments	: wpa_global
 * - return	: none
 *
 * - Discription :
 *	Event loop main function for Renesas Wi-Fi supplicant.
 */
extern void l2_packet_receive(struct l2_packet_data *l2);
extern void fc80211_enable_drv_event(void);

void da16x_eloop_run(struct wpa_global *global, struct wpa_supplicant *wpa_s)
{
	int sys_wdog_id = da16x_sys_wdog_id_get_da16x_supp();
	UINT	status_q;
#ifdef CONFIG_SUPP_EVENT_DEPRECATED		
	ULONG	temp_pack[2];
#else
	ULONG	*temp_pack=NULL;
#endif /* CONFIG_SUPP_EVENT_DEPRECATED */

	int	dpm_sts, dpm_retry_cnt = 0;
	struct os_reltime tv, now;
	tv.sec = tv.usec = 0;

	fc80211_enable_drv_event();

	/* For Message Queue */
	da16x_drv_msg_buf_t *da16x_drv_msg_buf_p=NULL;
	da16x_cli_tx_msg_t *da16x_cli_cmd_buf_p=NULL;

	while (1) {
		struct da16x_eloop_timeout *timeout=NULL;

		da16x_sys_watchdog_notify(sys_wdog_id);

#ifdef CHECK_SUPPLICANT_ERR
		status_q = NX_NOT_FOUND;
#endif	/*CHECK_SUPPLICANT_ERR*/

		timeout = dl_list_first(&da16x_eloop.timeout,
					struct da16x_eloop_timeout, list);
		if (timeout) {
			da16x_sys_watchdog_notify(sys_wdog_id);

			da16x_sys_watchdog_suspend(sys_wdog_id);

			os_get_reltime(&now);

			/* check if some registered timeouts have occurred */
			if (os_reltime_before(&now, &timeout->time)) {
				/* Not occurred yet. Calculate remaining time */
				os_reltime_sub(&timeout->time, &now, &tv);
			} else { /* Some timeout has occurred! Call handler */
				void *eloop_data = timeout->eloop_data;
				void *user_data = timeout->user_data;

				da16x_eloop_timeout_handler handler = timeout->handler;
				da16x_eloop_remove_timeout(timeout);
				handler(eloop_data, user_data);
				tv.sec = tv.usec = 0;
			}

			da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
		}
        else{  // if timeout list is NULL, timer reset 
            tv.sec = tv.usec = 0;
        }
		if (chk_dpm_pdown_start()) {
			da16x_sys_watchdog_notify(sys_wdog_id);

			da16x_sys_watchdog_suspend(sys_wdog_id);

			vTaskDelay(1);

			da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
			continue;
		}

		da16x_sys_watchdog_notify(sys_wdog_id);

		da16x_sys_watchdog_suspend(sys_wdog_id);

		/* by Bright :
		 *	Queue delay time have to got minimum wait_option value
		 *	because of short running time by dpm_sleep start...
		 */
#ifdef CONFIG_SUPP_EVENT_DEPRECATED
		status_q = xQueueReceive(TO_SUPP_QUEUE,	&temp_pack, timeout ? (tv.sec*100 + 1) : 10);
#else
		if (dpm_slp_time_reduce_flag == pdTRUE) {
			status_q = xQueueReceive(TO_SUPP_QUEUE,	&temp_pack, timeout ? (tv.sec*100 + 1) : 1);
		} else {
			status_q = xQueueReceive(TO_SUPP_QUEUE,	&temp_pack, timeout ? (tv.sec*100 + 1) : 10);
		}

#endif /* CONFIG_SUPP_EVENT_DEPRECATED */

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

		da16x_sys_watchdog_suspend(sys_wdog_id);

		if (status_q == pdTRUE) {
dpm_clr_retry :
			if (dpm_retry_cnt++ < 5) {
				dpm_sts = clr_dpm_sleep(REG_NAME_DPM_SUPPLICANT);

				switch (dpm_sts) {
					case DPM_SET_ERR :
						vTaskDelay(1);
						goto dpm_clr_retry;
						break;
					case DPM_SET_ERR_BLOCK :
						/* Don't need try continues */
						;
					case DPM_SET_OK :
						break;
				}
			}
			dpm_retry_cnt = 0;

			if (temp_pack[0] == DA16X_SP_DRV_FLAG) {
				da16x_drv_msg_buf_p = (da16x_drv_msg_buf_t *)temp_pack[1];
#ifndef CONFIG_SUPP_EVENT_DEPRECATED		
				os_free(temp_pack);
#endif /* CONFIG_SUPP_EVENT_DEPRECATED */
				da16x_drv_event_handler(da16x_drv_msg_buf_p);
#if 1 // ORG 1
				if (da16x_drv_msg_buf_p)
					os_free(da16x_drv_msg_buf_p);
#endif

			} else if (temp_pack[0] == DA16X_SP_CLI_TX_FLAG) {
				da16x_cli_cmd_buf_p = (da16x_cli_tx_msg_t *)temp_pack[1];
#ifndef CONFIG_SUPP_EVENT_DEPRECATED
				if (temp_pack)
				{
					os_free(temp_pack);
				}
#endif /* CONFIG_SUPP_EVENT_DEPRECATED */

				da16x_cli_cmd_handler(global,da16x_cli_cmd_buf_p);

				if (da16x_cli_cmd_buf_p->cli_tx_buf)
				{
					os_free(da16x_cli_cmd_buf_p->cli_tx_buf);
				}

			} else if (temp_pack[0] == DA16X_L2_PKT_RX_EV) {
#ifndef CONFIG_SUPP_EVENT_DEPRECATED		
				os_free(temp_pack);
#endif /* CONFIG_SUPP_EVENT_DEPRECATED */
				if (wpa_s->l2 != NULL) {
					l2_packet_receive(wpa_s->l2);

					if (chk_dpm_wakeup() && wpa_s->wpa_state == WPA_COMPLETED) {
						da16x_eap_prt("L2 Packet process completed, Set DPM Sleep !!! \n");
#if 1
				/* * DPM EAPOL Update Patch
				 * In case of Some AP, After EAPOL Update, updated as different HW KEY ID , 
				 * so need to send ARP REQ for DPM Communication
		 		 */
//						if(get_bit_dpm_sleep(REG_NAME_DPM_KEY)) 
						{
							//PRINTF("ARP Request Send \n");	
							do_dpm_autoarp_send();
							/* tcp check port is not affected */
							//set_dpm_tim_tcp_chkport_enable();
						}
#endif
						set_dpm_sleep("DPM_KEY");
					}
				}
#ifdef CONFIG_RECEIVE_DHCP_EVENT
			} else if (temp_pack[0] == DA16X_DHCP_EV) {
				da16x_dhcp_event_handler(global, &temp_pack[1]);
#ifndef CONFIG_SUPP_EVENT_DEPRECATED		
				os_free(temp_pack);
#endif /* CONFIG_SUPP_EVENT_DEPRECATED */

#endif /* CONFIG_RECEIVE_DHCP_EVENT */
			}

			/* Need to set dpm sleep flag at this time ... */
			if (wpa_s->wpa_state == WPA_COMPLETED) {
dpm_set_retry_1 :
				if (dpm_retry_cnt++ < 5) {
					dpm_sts = set_dpm_sleep(REG_NAME_DPM_SUPPLICANT);
					switch (dpm_sts) {
					case DPM_SET_ERR :
						vTaskDelay(1);
						goto dpm_set_retry_1;
						break;
					case DPM_SET_ERR_BLOCK :
						/* Don't need try continues */
						;
					case DPM_SET_OK :
						break;
					}
				}
				dpm_retry_cnt = 0;
			}
		} else if (status_q == pdFALSE) {
			if (wpa_s->wpa_state == WPA_COMPLETED) {
dpm_set_retry_2 :
				if (dpm_retry_cnt++ < 5) {
					dpm_sts = set_dpm_sleep(REG_NAME_DPM_SUPPLICANT);
					switch (dpm_sts) {
					case DPM_SET_ERR :
						vTaskDelay(1);
						goto dpm_set_retry_2;
						break;
					case DPM_SET_ERR_BLOCK :
						/* Don't need try continues */
						;
					case DPM_SET_OK :
						break;
					}
				}
				dpm_retry_cnt = 0;
			}
		}

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
	}
}

/* EOF */
