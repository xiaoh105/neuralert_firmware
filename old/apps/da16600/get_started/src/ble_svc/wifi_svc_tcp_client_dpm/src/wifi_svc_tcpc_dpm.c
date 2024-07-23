/**
 ****************************************************************************************
 *
 * @file tcp_client_dpm_sample.c
 *
 * @brief Sample app of TCP Client in DPM mode.
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

#if defined (__BLE_PERI_WIFI_SVC_TCP_DPM__)

#include "da16x_system.h"
#include "common_utils.h"
#include "common_def.h"
#include "lwip/err.h"
#include "user_dpm_manager.h"
#include "da16_gtl_features.h"

//Defalut TCP Server configuration
#define TCP_CLIENT_DPM_SAMPLE_DEF_SERVER_IP					"192.168.0.11"
#define TCP_CLIENT_DPM_SAMPLE_DEF_SERVER_PORT				10192

//Default TCP Client configuration
#define TCP_CLIENT_DPM_SAMPLE_DEF_CLIENT_PORT				10192
#define TCP_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_RETRY		3
#define TCP_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_TIMEOUT	5
#define TCP_CLIENT_DPM_SAMPLE_DEF_AUTO_RECONNECTION			1

//NVRAM
#define TCP_CLIENT_DPM_SAMPLE_NVRAM_SERVER_IP_NAME			"TCPC_SERVER_IP"
#define TCP_CLIENT_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME		"TCPC_SERVER_PORT"

//Hex Dump
#undef	TCP_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP

//TCP Server inforamtion
typedef struct
{
	char    ip_addr[32];
	int port;
} tcp_client_dpm_sample_svr_info_t;

//Function prototype
void tcp_client_dpm_sample_init_callback();
void tcp_client_dpm_sample_wakeup_callback();
void tcp_client_dpm_sample_external_callback();
void tcp_client_dpm_sample_error_callback(UINT error_code, char *comment);
void tcp_client_dpm_sample_connect_callback(void *sock, UINT conn_status);
void tcp_client_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
										 ULONG rx_ip, ULONG rx_port);
void tcp_client_dpm_sample_init_user_config(dpm_user_config_t *user_config);
UINT tcp_client_dpm_sample_load_server_info();
void tcp_client_dpm_sample(void *arg);

//Global variable
tcp_client_dpm_sample_svr_info_t srv_info = {0x00,};

//Function
#if defined (TCP_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)
static void tcp_client_dpm_sample_hex_dump(const char *title, UCHAR *buf, size_t len)
{

	extern void hex_dump(UCHAR * data, UINT length);

	DBG_INFO("%s(%ld)\n", title, len);

	hex_dump(buf, len);

	return ;
}
#endif // (TCP_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)

void tcp_client_dpm_sample_init_callback()
{
	DBG_INFO(GREEN_COLOR " [%s] Boot initialization\n" CLEAR_COLOR, __func__);
	
#ifdef __SUPPORT_DPM_MANAGER__
	dpm_mng_job_done();
#endif
}

void tcp_client_dpm_sample_wakeup_callback()
{
	DBG_INFO(GREEN_COLOR " [%s] DPM Wakeup\n" CLEAR_COLOR, __func__);

#ifdef __SUPPORT_DPM_MANAGER__
	dpm_mng_job_done();
#endif
}

void tcp_client_dpm_sample_external_callback()
{
	DBG_INFO(GREEN_COLOR " [%s] External DPM Wakeup\n" CLEAR_COLOR, __func__);
	
#ifdef __SUPPORT_DPM_MANAGER__
	dpm_mng_job_done();
#endif
}

void tcp_client_dpm_sample_error_callback(UINT error_code, char *comment)
{
	DBG_INFO(RED_COLOR " [%s] Error Callback(%s - 0x%0x) \n" CLEAR_COLOR,
		   __func__, comment, error_code);
}

void tcp_client_dpm_sample_connect_callback(void *sock, UINT conn_status)
{
	DA16X_UNUSED_ARG(sock);

	DBG_INFO(GREEN_COLOR " [%s] TCP Connection Result(0x%x) \n" CLEAR_COLOR,
		   __func__, conn_status);

#ifdef __SUPPORT_DPM_MANAGER__
	dpm_mng_job_done();
#endif
}

void tcp_client_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
										 ULONG rx_ip, ULONG rx_port)
{
	DA16X_UNUSED_ARG(sock);

#ifdef __SUPPORT_DPM_MANAGER__
	int status = ERR_OK;

	//Display received packet
	DBG_INFO(" =====> "YELLOW_COLOR"Received"CLEAR_COLOR" Packet(%ld) \n", rx_len);

#if defined (TCP_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)
	tcp_client_dpm_sample_hex_dump("Recevied Packet", rx_buf, rx_len);
#endif

	//Echo message
	status = dpm_mng_send_to_session(SESSION1, rx_ip, rx_port, (char *)rx_buf, rx_len);
	if (status)
	{
		DBG_INFO(RED_COLOR " [%s] Fail send data(session%d,0x%x) \n" CLEAR_COLOR,
			   __func__, SESSION1, status);
	}
	else
	{
		//Display sent packet
		DBG_INFO(" <===== Sent Packet(%ld) \n", rx_len);
#if defined (TCP_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)
		tcp_client_dpm_sample_hex_dump("Sent Packet", rx_buf, rx_len);
#endif
	}
	

	dpm_mng_job_done();
#else
	DA16X_UNUSED_ARG(rx_buf);
	DA16X_UNUSED_ARG(rx_len);
	DA16X_UNUSED_ARG(rx_ip);
	DA16X_UNUSED_ARG(rx_port);

	return;
#endif
}

void tcp_client_dpm_sample_init_user_config(dpm_user_config_t *user_config)
{
	const int session_idx = 0;

	if (!user_config)
	{
		DBG_ERR(" [%s] Invalid parameter\n", __func__);
		return ;
	}

	//Set Boot init callback
	user_config->bootInitCallback = tcp_client_dpm_sample_init_callback;

	//Set DPM wakeup init callback
	user_config->wakeupInitCallback = tcp_client_dpm_sample_wakeup_callback;

	//Set External wakeup callback
	user_config->externWakeupCallback = tcp_client_dpm_sample_external_callback;

	//Set Error callback
	user_config->errorCallback = tcp_client_dpm_sample_error_callback;

	//Set session type(TCP Client)
	user_config->sessionConfig[session_idx].sessionType = REG_TYPE_TCP_CLIENT;

	//Set local port
	user_config->sessionConfig[session_idx].sessionMyPort =
		TCP_CLIENT_DPM_SAMPLE_DEF_CLIENT_PORT;

	//Set server IP address
	memcpy(user_config->sessionConfig[session_idx].sessionServerIp,
		   srv_info.ip_addr, strlen(srv_info.ip_addr));

	//Set server port
	user_config->sessionConfig[session_idx].sessionServerPort = srv_info.port;

	//Set Connection callback
	user_config->sessionConfig[session_idx].sessionConnectCallback =
		tcp_client_dpm_sample_connect_callback;

	//Set Recv callback
	user_config->sessionConfig[session_idx].sessionRecvCallback =
		tcp_client_dpm_sample_recv_callback;

	//Set connection retry count
	user_config->sessionConfig[session_idx].sessionConnRetryCnt =
		TCP_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_RETRY;

	//Set connection timeout
	user_config->sessionConfig[session_idx].sessionConnWaitTime =
		TCP_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_TIMEOUT;

	//Set auto reconnection flag
	user_config->sessionConfig[session_idx].sessionAutoReconn = TRUE;

	//Set user configuration
	user_config->ptrDataFromRetentionMemory = (UCHAR *)&srv_info;
	user_config->sizeOfRetentionMemory = sizeof(tcp_client_dpm_sample_svr_info_t);

	return ;
}

UINT tcp_client_dpm_sample_load_server_info()
{
	char *srv_ip_str = NULL;
	int srv_port = 0;
	UINT8 srv_ip_default = 0;

	tcp_client_dpm_sample_svr_info_t *srv_info_ptr = &srv_info;

#ifdef __SUPPORT_DPM_MANAGER__
	if (dpm_mode_is_wakeup())
	{
		return ERR_OK;
	}
#endif
	memset(srv_info_ptr, 0x00, sizeof(tcp_client_dpm_sample_svr_info_t));

	//Read server IP address in nvram
	srv_ip_str = read_nvram_string(TCP_CLIENT_DPM_SAMPLE_NVRAM_SERVER_IP_NAME);
	if (srv_ip_str == NULL)
	{
		srv_ip_str = TCP_CLIENT_DPM_SAMPLE_DEF_SERVER_IP;
		srv_ip_default = 1;
	}

	//Set server IP address
	strcpy(srv_info_ptr->ip_addr, srv_ip_str);

	//Read server port in nvram
	if (read_nvram_int(TCP_CLIENT_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME, &srv_port))
	{
		srv_port = TCP_CLIENT_DPM_SAMPLE_DEF_SERVER_PORT;
	}

	//Set server port
	srv_info_ptr->port = srv_port;

	if (!srv_ip_default) {
		DBG_INFO(" [%s] TCP Server Information(%s:%d)\n",
			 __func__, srv_info_ptr->ip_addr, srv_info_ptr->port);
	}

	return ERR_OK;
}

void tcp_client_dpm_sample(void *arg)
{
	DA16X_UNUSED_ARG(arg);

#ifdef __SUPPORT_DPM_MANAGER__
	UINT status = ERR_OK;

	DBG_INFO(" [%s] Start TCP Client Sample\n", __func__);

	//Load server information
	status = tcp_client_dpm_sample_load_server_info();
	if (status)
	{
		DBG_INFO(" [%s] Failed to load TCP Server information(0x%02x)\n",
			   __func__, status);
		return ;
	}

	//Register user configuration
	dpm_mng_regist_config_cb(tcp_client_dpm_sample_init_user_config);

	//Start TCP Client Sample
	dpm_mng_start();
#endif

	vTaskDelete(NULL);
}

#endif /* __BLE_PERI_WIFI_SVC_TCP_DPM__ */

/* EOF */
