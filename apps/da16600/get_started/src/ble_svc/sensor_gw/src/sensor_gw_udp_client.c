/**
 ****************************************************************************************
 *
 * @file sensor_gw_udp_client.c
 *
 * @brief Sample UDP Client implementation for Sensor Gateway example
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

#if defined (__BLE_CENT_SENSOR_GW__)
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "common_utils.h"
#include "common_def.h"
#include "iface_defs.h"
#include "user_dpm_api.h"
#include "da16x_dhcp_client.h"
#include "da16x_network_main.h"
#ifdef __FREERTOS__
#include "msg_queue.h"
#endif


/* BLE header files */
#include "../inc/da14585_config.h"
#include "../inc/app.h"
#include "da16_gtl_features.h"

/*
 * DEFINITIONS
 ****************************************************************************************
*/

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

typedef enum {
        GW_EVT_UPD_NO_EVT,
        GW_EVT_UPD_DATA_SEND,
        GW_EVT_UPD_MAX,
} SENSOR_GW_EVT;

typedef struct {
        SENSOR_GW_EVT event;
        uint8_t slot;
        uint8_t param;
} sensor_gw_task_q_evt_t;

#define UDP_TX_BUF_SZ			128
#define UDP_CLIENT_MAX_QUEUE	5
#define UDP_TX_PERIOD_SEC		5
#define SENSOR_GW_UDP_CLI		"UDPC_SENSOR_GW"
#define DEF_UDP_RECEVER_IP		"192.168.1.2"
#define UDP_LOCAL_PORT			10195
#define GW_UDP_CONNECTED_MSG	"Sensor GW connected!"
#define SENSOR_GW_Q_SIZE		10
#define UDP_CLI_TEST_PORT		10195
#define OS_GET_CURRENT_TASK() 	xTaskGetCurrentTaskHandle()

/*
 * VARIABLE DEFINITIONS
*/

static msg_queue sensor_gw_task_q;
static char *udp_server_ip = DEF_UDP_RECEVER_IP;
static TaskHandle_t sensor_gw_task_handle = NULL;
static char *gw_str_connected = GW_UDP_CONNECTED_MSG;

/*
 * FUNCTIONS DECLARATION
*/

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

void sensor_gw_evt_push(uint8_t evt, uint8_t param, uint8_t slot)
{
	sensor_gw_task_q_evt_t sensor_evt;

	if (sensor_gw_task_handle == NULL)      return;

	sensor_evt.event = evt;
	sensor_evt.slot = slot;
	sensor_evt.param = param;

	/* Put the event into the task queue */
	if (msg_queue_send(&sensor_gw_task_q, 0, 0, (void *)&sensor_evt,
		sizeof(sensor_gw_task_q_evt_t), portMAX_DELAY) == OS_QUEUE_FULL) {

		configASSERT(0);
	}
}

void enqueue_sensor_gw_udp_client(uint8_t param, uint8_t slot)
{
	sensor_gw_evt_push(GW_EVT_UPD_DATA_SEND, param, slot);
}

/* Sample Code for UDP Rx on DPM */
void udp_client(char *ip, int port)
{
	int ret;
	int sock;
	char *data_buf = NULL;
	struct sockaddr_in local_addr;
	struct sockaddr_in dest_addr;

	DBG_INFO("[%s] Start of Sensor GW UDP Socket sample\r\n", __func__);

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

	/* Bind UDP socket */
	ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		DBG_ERR("[%s] Failed to bind socket(%d)\r\n", __func__, ret);
		goto end_of_udp_client;
	}

	/* Allocate TX data buffer */
	data_buf = (char *)MEMALLOC(UDP_TX_BUF_SZ);

	if (data_buf == NULL)
	{
		DBG_ERR("[%s] data_buf memory alloc fail ...\n", __func__);
		goto end_of_udp_client;
	}

    /* At first, send the "Connected msg" to the UDP server*/
	ret = sendto(sock, gw_str_connected, strlen(gw_str_connected), 0,
				 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (ret < 0) {
		DBG_ERR(RED_COL "[%s] Failed to send data\r\n" CLR_COL, __func__);
		goto end_of_udp_client;
	}

	while (TRUE)
	{
		sensor_gw_task_q_evt_t *_sensor_evt;
		msg queue_msg;

		memset(data_buf, 0x00, UDP_TX_BUF_SZ);

		if (msg_queue_get(&sensor_gw_task_q, &queue_msg, portMAX_DELAY) == OS_QUEUE_EMPTY) {
			configASSERT(0);
		}

		_sensor_evt = (sensor_gw_task_q_evt_t *)queue_msg.data;
		if (_sensor_evt == NULL)
			goto end_of_udp_client;

		switch (_sensor_evt->event) {
		case GW_EVT_UPD_DATA_SEND:
			sprintf(data_buf, "iot_sensor[%d], temperature value = %d",
					_sensor_evt->slot,
					_sensor_evt->param);

			ret = sendto(sock, data_buf, strlen(data_buf), 0,
						 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (ret < 0) {
				DBG_INFO("[%s] Failed to send data\r\n", __func__);
				break;
			}
			DBG_INFO(" Sensor GW iot_sensor[%d], temperature value = %d\r\n",
					_sensor_evt->slot,
					_sensor_evt->param);
		break;

		default:
		break;
		}
		msg_release(&queue_msg);

	}

end_of_udp_client:

	if (data_buf) {
		MEMFREE(data_buf);
	}

	close(sock);

	DBG_INFO("[%s] End of UDP Socket sample\r\n", __func__);

	return ;
}

void udp_client_main(void *arg)
{
	DA16X_UNUSED_ARG(arg);

	char *ip = NULL;
	char server_ip[32];
	int	server_port = 0;

	sensor_gw_task_handle = OS_GET_CURRENT_TASK();

	if ((ip = read_nvram_string("UDP_SERVER_IP")))
	{
		strcpy(server_ip, ip);
	}
	else
	{
		/* When the ip address is not saved in nvram */
		strcpy(server_ip, udp_server_ip);
	}

	if (read_nvram_int("UDP_SERVER_PORT", &server_port)) {
		server_port = UDP_CLI_TEST_PORT;
	}

	/* Create a queue for the containing incoming events */
	msg_queue_create(&sensor_gw_task_q, SENSOR_GW_Q_SIZE, NULL);

	udp_client(server_ip, server_port);

	msg_queue_delete(&sensor_gw_task_q);

	sensor_gw_task_handle = NULL;

	vTaskDelete(NULL);
}

#endif	/* __BLE_CENT_SENSOR_GW__ */

/* EOF */
