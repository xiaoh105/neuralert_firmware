/**
 ****************************************************************************************
 *
 * @file tcp_server_dpm_sample.c
 *
 * @brief Sample app of TCP Server in DPM Mode.
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
#include "sample_defs.h"

#if defined (__TCP_SERVER_DPM_SAMPLE__)			\
		&& defined (__SUPPORT_DPM_MANAGER__)	\
		&& !defined (__LIGHT_DPM_MANAGER__)

#include "da16x_system.h"
#include "common_utils.h"
#include "common_def.h"

#include "user_dpm_manager.h"

//Defalut TCP Server configuration
#define TCP_SERVER_DPM_SAMPLE_DEF_SERVER_PORT			TCP_SVR_TEST_PORT

//NVRAM
#define TCP_SERVER_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME	"TCP_SVR_PORT"

//Hex Dump
#undef	TCP_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP

//TCP Server inforamtion
typedef struct {
    int port;
} tcp_server_dpm_sample_svr_info_t;

//Function prototype
void tcp_server_dpm_sample_init_callback();
void tcp_server_dpm_sample_wakeup_callback();
void tcp_server_dpm_sample_error_callback(UINT error_code, char *comment);
void tcp_server_dpm_sample_connect_callback(void *sock, UINT conn_status);
void tcp_server_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port);
void tcp_server_dpm_sample_init_user_config(dpm_user_config_t *user_config);
unsigned char tcp_server_dpm_sample_load_server_info();
void tcp_server_dpm_sample(void *param);

//Global variable
tcp_server_dpm_sample_svr_info_t srv_info = {0x00,};

//Function
static void tcp_server_dpm_sample_hex_dump(const char *title, UCHAR *buf, size_t len)
{
#if defined (TCP_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(UCHAR * data, UINT length);

    PRINTF("%s(%ld)\n", title, len);

    hex_dump(buf, len);
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (TCP_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void tcp_server_dpm_sample_init_callback()
{
    PRINTF(GREEN_COLOR " [%s] Boot initialization\n" CLEAR_COLOR,
           __func__);

    dpm_mng_job_done(); //Done opertaion
}

void tcp_server_dpm_sample_wakeup_callback()
{
    PRINTF(GREEN_COLOR " [%s] DPM Wakeup\n" CLEAR_COLOR, __func__);

    dpm_mng_job_done(); //Done opertaion
}

void tcp_server_dpm_sample_error_callback(UINT error_code, char *comment)
{
    PRINTF(RED_COLOR " [%s] Error Callback(%s - 0x%0x) \n" CLEAR_COLOR,
           __func__, comment, error_code);
}

void tcp_server_dpm_sample_connect_callback(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] TCP Connection Result(0x%x) \n" CLEAR_COLOR,
           __func__, conn_status);

    dpm_mng_job_done(); //Done opertaion
}

void tcp_server_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);

    unsigned char status = pdPASS;

    //Display received packet
    PRINTF(" =====> Received Packet(%ld) \n", rx_len);

    tcp_server_dpm_sample_hex_dump("Recevied Packet", rx_buf, rx_len);

    //Echo message
    status = dpm_mng_send_to_session(SESSION1, rx_ip, rx_port, (char *)rx_buf, rx_len);
    if (status) {
        PRINTF(RED_COLOR " [%s] Fail send data(session%d,0x%x) \n" CLEAR_COLOR,
               __func__, SESSION1, status);
    } else {
        //Display sent packet
        PRINTF(" <===== Sent Packet(%ld) \n", rx_len);

        tcp_server_dpm_sample_hex_dump("Sent Packet", rx_buf, rx_len);
    }

    dpm_mng_job_done(); //Done opertaion
}

void tcp_server_dpm_sample_init_user_config(dpm_user_config_t *user_config)
{
    const int session_idx = 0;

    if (!user_config) {
        PRINTF(" [%s] Invalid parameter\n", __func__);
        return ;
    }

    //Set Boot init callback
    user_config->bootInitCallback = tcp_server_dpm_sample_init_callback;

    //Set DPM wakkup init callback
    user_config->wakeupInitCallback = tcp_server_dpm_sample_wakeup_callback;

    //Set Error callback
    user_config->errorCallback = tcp_server_dpm_sample_error_callback;

    //Set session type(TCP Server)
    user_config->sessionConfig[session_idx].sessionType = REG_TYPE_TCP_SERVER;

    //Set local port
    user_config->sessionConfig[session_idx].sessionMyPort =
        TCP_SERVER_DPM_SAMPLE_DEF_SERVER_PORT;

    //Set Connection callback
    user_config->sessionConfig[session_idx].sessionConnectCallback =
        tcp_server_dpm_sample_connect_callback;

    //Set Recv callback
    user_config->sessionConfig[session_idx].sessionRecvCallback =
        tcp_server_dpm_sample_recv_callback;

    //Set user configuration
    user_config->ptrDataFromRetentionMemory = (UCHAR *)&srv_info;
    user_config->sizeOfRetentionMemory = sizeof(tcp_server_dpm_sample_svr_info_t);

    return ;
}

unsigned char tcp_server_dpm_sample_load_server_info()
{
    int srv_port = 0;

    tcp_server_dpm_sample_svr_info_t *srv_info_ptr = &srv_info;

    memset(srv_info_ptr, 0x00, sizeof(tcp_server_dpm_sample_svr_info_t));

    //Read server port in nvram
    if (read_nvram_int(TCP_SERVER_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME, &srv_port)) {
        srv_port = TCP_SERVER_DPM_SAMPLE_DEF_SERVER_PORT;
    }

    //Set server port
    srv_info_ptr->port = srv_port;

    PRINTF(" [%s] TCP Server Information(%d)\n",
           __func__, srv_info_ptr->port);

    return pdPASS;
}

void tcp_server_dpm_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    unsigned char status = pdPASS;

    PRINTF(" [%s] Start TCP Server Sample\n", __func__);

    //Load server information
    status = tcp_server_dpm_sample_load_server_info();
    if (status == pdFAIL) {
        PRINTF(" [%s] Failed to load TCP Server information(0x%02x)\n",
               __func__, status);
        vTaskDelete(NULL);
        return ;
    }

    //Register user configuration
    dpm_mng_regist_config_cb(tcp_server_dpm_sample_init_user_config);

    //Start TCP Server Sample
    dpm_mng_start();

    vTaskDelete(NULL);

    return ;
}

#endif // (__TCP_SERVER_DPM_SAMPLE__) && (__SUPPORT_DPM_MANAGER__) && !(__LIGHT_DPM_MANAGER) 

/* EOF */
