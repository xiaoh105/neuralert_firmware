/**
 ****************************************************************************************
 *
 * @file iot_sensor_udp_client.c
 *
 * @brief UDP client for iot_sensor example
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

/*
 * INCLUDE FILES
 */
 
#include "sdk_type.h"

#if defined (__BLE_PERI_WIFI_SVC__)
#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "common_utils.h"
#include "common_def.h"
#include "iface_defs.h"
#include "user_dpm_api.h"
#include "da16x_dhcp_client.h"
#include "da16x_network_main.h"

/* BLE header files */
#include "../../include/da14585_config.h"
#include "da16_gtl_features.h"

/* Sample application header files */
#include "../../include/wifi_svc_user_custom_profile.h"
#include "../../include/app.h"

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif

#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
#include "sleep2_monitor.h"
#endif

/*
 * DEFINITIONS
 ****************************************************************************************
*/
#define UDP_TX_BUF_SZ			128
#define UDP_CLIENT_MAX_QUEUE	5
#define UDP_TX_PERIOD_SEC		5
#define IOT_SENSOR_UDP_CLI		"UDPC_IOT_SENSOR"

#define UDP_IOT_LOOP_MAX		40
#define UDP_IOT_LOOP_TIME		10
#define UDP_CONNECTED_MSG		"Gas leak sensor connected!"

#define UDP_CLI_TEST_PORT		10195
#define UDP_LOCAL_PORT			UDP_CLI_TEST_PORT

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/*
 * VARIABLE DEFINITIONS
*/

static char *str_connected = UDP_CONNECTED_MSG;
extern int	interface_select;
#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
extern uint8_t is_first_gtl_msg;
#endif

/*
 * FUNCTIONS DECLARATION
*/
extern int check_net_ip_status(int iface);

#if defined (__LOW_POWER_IOT_SENSOR__)
extern iot_sensor_data_t iot_sensor_data_info;
extern int wifi_netif_status(int intf);

#if defined (__WAKE_UP_BY_BLE__)
#endif /* __WAKE_UP_BY_BLE__ */
#endif /* __LOW_POWER_IOT_SENSOR__ */

// #define COMBO_USER_CUSTOM_DATA_RTM_SAMPLE

#if defined (COMBO_USER_CUSTOM_DATA_RTM_SAMPLE)
typedef struct _user_custom_data_t
{
	int data_1;
	int data_2;
} user_custom_data_t;

#define MY_APP_DATA "MY_APP_DATA"

user_custom_data_t* g_user_data = NULL;

int restore_user_custom_data(void) 
{
	int rtm_len;
	unsigned int status;
	rtm_len = user_rtm_get(MY_APP_DATA, (UCHAR **)&g_user_data);

	if (rtm_len == 0) {
		// no existing data found in rtm, then newly allocate one
		status = user_rtm_pool_allocate(MY_APP_DATA,
									(void **)&g_user_data,
									sizeof(user_custom_data_t),
									100);

		if (status != 0 /* NOT SUCCESS */) {
			PRINTF("[%s] Failed to allocate RTM area !!!\n", __func__);
			return -1;
		}

		memset(g_user_data, 0x00, sizeof(user_custom_data_t));

		// e.g. init from nvram if required ... 
		
		if (read_nvram_int("user_data_1", &(g_user_data->data_1)) != 0) {
			// data_1 does not exist in nvram, create one and fill in with the default value
			write_nvram_int("user_data_1", 1);
			g_user_data->data_1 = 1;
		}
		
		if (read_nvram_int("user_data_2", &(g_user_data->data_2)) != 0) {
			// data_2 does not exist in nvram, create one and fill in with the default value
			write_nvram_int("user_data_2", 1);
			g_user_data->data_2 = 1;
		}
		
		PRINTF("[combo] User data init complete ! \n");
		return 0;
	} else if (rtm_len != sizeof(user_custom_data_t)) {
		// user data exists in rtm but corrupted
		PRINTF("[%s] Invalid RTM alloc size (%d)\n", __func__, rtm_len);
		return -2;
	} else if (rtm_len == sizeof(user_custom_data_t)){
		// user data in rtm in-tact, proceed ...
		PRINTF("[combo] User data restore complete ! \n");
		return 0;
	} else {
		return -3;
	}
}

void backup_user_custom_data(void)
{
	// latest user data is already in RTM ...
	PRINTF("[combo] User data back-up complete ! \n");
	
	// if you want to back up your data additionally to nvram, the code below will do that.
	if (g_user_data) {
		write_nvram_int("user_data_1", g_user_data->data_1);
		write_nvram_int("user_data_2", g_user_data->data_2);
		
		PRINTF("[combo] User data back-up to nvram complete ! \n");
	}
}

#endif // COMBO_USER_CUSTOM_DATA_RTM_SAMPLE

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

extern void wifi_netif_control(int intf, int flag);
extern void iface_updown(UINT iface, UINT flag);

void mem_free_and_null (void** mem_ptr)
{
	void* m_ptr = (void*)(*(mem_ptr));
	if (m_ptr)
	{
		vPortFree(m_ptr);
		*mem_ptr = NULL;
	}
}

void udp_client(char *ip, int port)
{
	int ret;
	int sock;
	int loop_cnt = 0;
	char *data_buf = NULL;
	unsigned int status = 0;
	struct sockaddr_in local_addr;
	struct sockaddr_in dest_addr;

	// On wake-up from sleep2, make sure re-connect to AP
	if (dpm_boot_type > DPM_BOOT_TYPE_NORMAL && (get_last_abnormal_act() == 0))
	{
		// for dpm wakeup (w/o abnormal wakeup)
UDP_RETRY:		
		da16x_cli_reply("disconnect", NULL, NULL);
		vTaskDelay(50);
				
		da16x_cli_reply("reassociate", NULL, NULL);
		vTaskDelay(50);

		if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
			loop_cnt = 0;
			while (chk_supp_connected() != pdPASS)
			{	
				/* 
					in case of AP connection fail, app loops forever 
					and lets DPM Abnoraml Checker take care of ....
				*/
				vTaskDelay(UDP_IOT_LOOP_TIME);
			}

			// ... ap connection done

			wifi_netif_control(0, 1);
			iface_updown(0, 1);
		}
		vTaskDelay(50);

		/* Waiting netif status */
		status = wifi_netif_status(interface_select);

		while (status == 0xFF || status != TRUE)
		{
			vTaskDelay(UDP_IOT_LOOP_TIME);
			status = wifi_netif_status(interface_select);
		}

		/* Check IP address resolution status */
		while (check_net_ip_status(interface_select) != 0)
		{
			vTaskDelay(UDP_IOT_LOOP_TIME);
		}


		loop_cnt = 0;
		if (chk_dpm_wakeup() == pdTRUE) {
			if (get_netmode(WLAN0_IFACE) == DHCPCLIENT) {
				while (da16x_network_main_check_dhcp_state(WLAN0_IFACE) != DHCP_STATE_BOUND) {
					if(loop_cnt++ > 60) {
						goto UDP_RETRY;
					}
					vTaskDelay(50);
				}
			}
		}
	}

	DBG_INFO("Start of UDP Socket sample, gas leak event(%d)\r\n",
			 iot_sensor_data_info.is_gas_leak_happened);

norm_start:
	memset(&dest_addr, 0x00, sizeof(dest_addr));

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET) {
		DBG_ERR(RED_COL "[%s] Failed to create socket\r\n" CLR_COL, __func__);
		goto end_of_udp_client;
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(UDP_LOCAL_PORT);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(ip);
	dest_addr.sin_port = htons(port);

	/* Bind udp socket */
	ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		DBG_ERR("[%s] Failed to bind socket(%d), retry!\r\n", __func__, ret);
		close(sock);
		goto UDP_RETRY;
	}

	/* At first, send the "Connected msg" to the UDP server*/
	ret = sendto(sock, str_connected, strlen(str_connected), 0,
				 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (ret < 0) {
		DBG_ERR("[%s] Failed to send data (0x%02x), retry!\n", __func__, ret);
		close(sock);
		goto UDP_RETRY;
	} else {
		PRINTF("iot_sensor connected to server \n");
	}

	/* Clear wakeup source in normal boot */
	if (dpm_mode_is_wakeup() == FALSE)
	{
		RTC_CLEAR_EXT_SIGNAL();
	}

	while (TRUE)
	{
		/* When receive the gas leak signal from the sensor via BLE,
		** Then send the message through the UDP protocol
		*/
		if (iot_sensor_data_info.is_gas_leak_happened == FALSE)
		{
#if defined (__APP_SLEEP2_MONITOR__)
            if (dpm_mode_is_wakeup() == TRUE && 
                dpm_boot_type == DPM_BOOT_TYPE_WAKEUP_DONT_CARE &&
                sleep2_monitor_get_state(SLEEP2_MON_NAME_PROV) == SLEEP2_SET) {
    		        sleep2_monitor_set_state(SLEEP2_MON_NAME_IOT_SENSOR, 
    								        SLEEP2_SET, __func__, __LINE__);
            }
#endif // __APP_SLEEP2_MONITOR__
			vTaskDelay(10);
			continue;
		}

		/* In case of network down */
		status = wifi_netif_status(interface_select);
		if (status == 0xFF || status != TRUE)
		{
			DBG_INFO(RED_COL "[%s] Network down ...\n" CLR_COL, __func__);
			close(sock);
			while (status == 0xFF || status != TRUE)
			{
				vTaskDelay(10);
				status = wifi_netif_status(interface_select);
				if (status == TRUE)
					goto norm_start;
			}
		}
			
		/* Gas leak occurred! Send the warning Message now. */
		
		mem_free_and_null((void**)&data_buf);
		data_buf = (char *)MEMALLOC(UDP_TX_BUF_SZ);
		if (data_buf == NULL)
		{
			DBG_ERR(RED_COL "[%s] data_buf memory alloc fail ...\n" CLR_COL, __func__);
			goto end_of_udp_client;
		}

		memset(data_buf, 0, UDP_TX_BUF_SZ);

		sprintf(data_buf, "[msg_sent] : gas_leak occurred, plz fix it !!!");

		DBG_INFO(YELLOW_COLOR">>> %s\n"CLEAR_COLOR, data_buf);

		// Send the message to the UDP server.
		ret = sendto(sock, data_buf, strlen(data_buf), 0,
					 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (ret < 0) {
			DBG_ERR("[%s] Failed to send data (0x%02x), retry!\n", __func__, ret);
			close(sock);
			goto UDP_RETRY;
		} else {
			iot_sensor_data_info.is_gas_leak_happened = FALSE;
		}

#if defined (__APP_SLEEP2_MONITOR__)
		sleep2_monitor_set_state(SLEEP2_MON_NAME_IOT_SENSOR, 
								SLEEP2_SET, __func__, __LINE__);

		if (dpm_mode_is_enabled() == TRUE)
		{
			if (is_wakeup_by_dpm_abnormal_chk() == TRUE && is_first_gtl_msg == TRUE)
			{
				// DPM abnormal wakeup + no concerned event happened from either sleep2 APPs
				sleep2_monitor_set_state(SLEEP2_MON_NAME_PROV,
										 SLEEP2_SET, __func__, __LINE__);
			}
		}
#endif
		vTaskDelay(7);
	}

end_of_udp_client:
	if(data_buf) {
		MEMFREE(data_buf);
	}

	close(sock);

	DBG_INFO("[%s] End of udp_client\r\n", __func__);
}

void udp_client_main(ULONG arg)
{
	DA16X_UNUSED_ARG(arg);

#if defined (__LOW_POWER_IOT_SENSOR__)
	int tmpProvisioned;
#endif /*__LOW_POWER_IOT_SENSOR__*/

#if defined (__LOW_POWER_IOT_SENSOR__)
	// update iot_sensor info (rtm) with latest info
	iot_sensor_data_info.is_provisioned = 1;

	if (read_nvram_int("provisioned", &tmpProvisioned) != 0)
	{
		// "provisioned" not exist in nvram
		write_nvram_int("provisioned", 1);
	}
	else
	{
		// "provisioned" exists
		if (tmpProvisioned != iot_sensor_data_info.is_provisioned)
		{
			write_nvram_int("provisioned", 1);
		}
	}
#endif /*__LOW_POWER_IOT_SENSOR__*/

#if defined (COMBO_USER_CUSTOM_DATA_RTM_SAMPLE)
	int result;
	extern void combo_set_usr_todo_before_sleep(void (*user_func)(void));
	combo_set_usr_todo_before_sleep(backup_user_custom_data);
	
	if ((result = restore_user_custom_data()) != 0) {
		PRINTF("[combo] user custom data init failed (%d)\n", result);		
	}
#endif // COMBO_USER_CUSTOM_DATA_RTM_SAMPLE

	udp_client(iot_sensor_data_info.svr_ip_addr, 
				iot_sensor_data_info.svr_port);

	vTaskDelete(NULL);
}

#endif	/*__BLE_PERI_WIFI_SVC__*/

/* EOF */
