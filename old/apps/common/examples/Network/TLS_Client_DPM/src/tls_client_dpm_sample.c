/**
 ****************************************************************************************
 *
 * @file tls_client_dpm_sample.c
 *
 * @brief Sample app of TLS Client in DPM mode.
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

#if defined (__TLS_CLIENT_DPM_SAMPLE__) && defined (__SUPPORT_DPM_MANAGER__)

#include "da16x_system.h"
#include "common_utils.h"
#include "common_def.h"

#include "user_dpm_manager.h"

//Defalut TLS Server configuration
#define TLS_CLIENT_DPM_SAMPLE_DEF_SERVER_IP					"192.168.0.11"
#define TLS_CLIENT_DPM_SAMPLE_DEF_SERVER_PORT				TLS_CLI_TEST_PORT

//Default TLS Client configuration
#define TLS_CLIENT_DPM_SAMPLE_DEF_CLIENT_PORT				TLS_CLI_TEST_PORT
#define TLS_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_RETRY		3
#define TLS_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_TIMEOUT	5
#define TLS_CLIENT_DPM_SAMPLE_DEF_AUTO_RECONNECTION			1

//NVRAM
#define TLS_CLIENT_DPM_SAMPLE_NVRAM_SERVER_IP_NAME			"TLSC_SERVER_IP"
#define TLS_CLIENT_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME		"TLSC_SERVER_PORT"

//Hex Dump
#undef	TLS_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP

//TLS Server inforamtion
typedef struct {
    char    ip_addr[32];
    int port;
} tls_client_dpm_sample_svr_info_t;

//Function prototype
void tls_client_dpm_sample_init_callback();
void tls_client_dpm_sample_wakeup_callback();
void tls_client_dpm_sample_external_callback();
void tls_client_dpm_sample_error_callback(UINT error_code, char *comment);
void tls_client_dpm_sample_connect_callback(void *sock, UINT conn_status);
void tls_client_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port);
void tls_client_dpm_sample_secure_callback(void *config);
void tls_client_dpm_sample_init_user_config(dpm_user_config_t *user_config);
unsigned char tls_client_dpm_sample_load_server_info();
void tls_client_dpm_sample(ULONG arg);

//Global variable
tls_client_dpm_sample_svr_info_t srv_info = {0x00,};

//Function
static void tls_client_dpm_sample_hex_dump(const char *title, UCHAR *buf, size_t len)
{
#if defined (TLS_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(UCHAR * data, UINT length);

    PRINTF("%s(%ld)\n", title, len);

    hex_dump(buf, len);
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (TLS_CLIENT_DPM_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void tls_client_dpm_sample_init_callback()
{
    PRINTF(GREEN_COLOR " [%s] Boot initialization\n" CLEAR_COLOR,
           __func__);

    dpm_mng_job_done(); //Done opertaion
}

void tls_client_dpm_sample_wakeup_callback()
{
    PRINTF(GREEN_COLOR " [%s] DPM Wakeup\n" CLEAR_COLOR, __func__);

    dpm_mng_job_done(); //Done opertaion
}

void tls_client_dpm_sample_external_callback()
{
    PRINTF(GREEN_COLOR " [%s] External DPM Wakeup\n" CLEAR_COLOR, __func__);

    dpm_mng_job_done(); //Done opertaion
}

void tls_client_dpm_sample_error_callback(UINT error_code, char *comment)
{
    PRINTF(RED_COLOR " [%s] Error Callback(%s - 0x%0x) \n" CLEAR_COLOR,
           __func__, comment, error_code);
}

void tls_client_dpm_sample_connect_callback(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] TLS Connection Result(0x%x) \n" CLEAR_COLOR,
           __func__, conn_status);

    dpm_mng_job_done(); //Done opertaion
}

void tls_client_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);

    unsigned char status = pdPASS;

    //Display received packet
    PRINTF(" =====> Received Packet(%ld) \n", rx_len);

    tls_client_dpm_sample_hex_dump("Recevied Packet", rx_buf, rx_len);

    //Echo message
    status = dpm_mng_send_to_session(SESSION1, rx_ip, rx_port, (char *)rx_buf, rx_len);
    if (status) {
        PRINTF(RED_COLOR " [%s] Fail send data(session%d,0x%x) \n" CLEAR_COLOR,
               __func__, SESSION1, status);
    } else {
        //Display sent packet
        PRINTF(" <===== Sent Packet(%ld) \n", rx_len);

        tls_client_dpm_sample_hex_dump("Sent Packet", rx_buf, rx_len);
    }

    dpm_mng_job_done(); //Done opertaion
}

void tls_client_dpm_sample_secure_callback(void *config)
{
    int ret = 0;

    SECURE_INFO_T *secure_config = (SECURE_INFO_T *)config;

    PRINTF(GREEN_COLOR " [%s] TLS Setup\n" CLEAR_COLOR,
           __func__);

    if (config == NULL) {
        PRINTF(" [%s] Invalid parameter\n", __func__);
        return ;
    }

    ret = mbedtls_ssl_config_defaults(secure_config->ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF(" [%s] Failed to set default ssl config(0x%x)\n",
               __func__, -ret);
        return ;
    }

    ret = dpm_mng_setup_rng(secure_config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    //don't care verification in this sample.
    mbedtls_ssl_conf_authmode(secure_config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);

    ret = mbedtls_ssl_setup(secure_config->ssl_ctx, secure_config->ssl_conf);
    if (ret) {
        PRINTF(" [%s] Failed to set ssl config(0x%x)\n",
               __func__, -ret);
        return ;
    }

    dpm_mng_job_done(); //Done opertaion

    return ;
}

void tls_client_dpm_sample_init_user_config(dpm_user_config_t *user_config)
{
    const int session_idx = 0;

    if (!user_config) {
        PRINTF(" [%s] Invalid parameter\n", __func__);
        return ;
    }

    //Set Boot init callback
    user_config->bootInitCallback = tls_client_dpm_sample_init_callback;

    //Set DPM wakeup init callback
    user_config->wakeupInitCallback = tls_client_dpm_sample_wakeup_callback;

    //Set External wakeup callback
    user_config->externWakeupCallback = tls_client_dpm_sample_external_callback;

    //Set Error callback
    user_config->errorCallback = tls_client_dpm_sample_error_callback;

    //Set session type(TLS Client)
    user_config->sessionConfig[session_idx].sessionType = REG_TYPE_TCP_CLIENT;

    //Set local port
    user_config->sessionConfig[session_idx].sessionMyPort =
        TLS_CLIENT_DPM_SAMPLE_DEF_CLIENT_PORT;

    //Set server IP address
    memcpy(user_config->sessionConfig[session_idx].sessionServerIp,
           srv_info.ip_addr, strlen(srv_info.ip_addr));

    //Set server port
    user_config->sessionConfig[session_idx].sessionServerPort = srv_info.port;

    //Set Connection callback
    user_config->sessionConfig[session_idx].sessionConnectCallback =
        tls_client_dpm_sample_connect_callback;

    //Set Recv callback
    user_config->sessionConfig[session_idx].sessionRecvCallback =
        tls_client_dpm_sample_recv_callback;

    //Set connection retry count
    user_config->sessionConfig[session_idx].sessionConnRetryCnt =
        TLS_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_RETRY;

    //Set connection timeout
    user_config->sessionConfig[session_idx].sessionConnWaitTime =
        TLS_CLIENT_DPM_SAMPLE_DEF_MAX_CONNECTION_TIMEOUT;

    //Set auto reconnection flag
    user_config->sessionConfig[session_idx].sessionAutoReconn = pdTRUE;

    //Set secure mode
    user_config->sessionConfig[session_idx].supportSecure = pdTRUE;

    //Set secure setup callback
    user_config->sessionConfig[session_idx].sessionSetupSecureCallback =
        tls_client_dpm_sample_secure_callback;

    //Set user configuration
    user_config->ptrDataFromRetentionMemory = (UCHAR *)&srv_info;
    user_config->sizeOfRetentionMemory = sizeof(tls_client_dpm_sample_svr_info_t);

    return ;
}

unsigned char tls_client_dpm_sample_load_server_info()
{
    char *srv_ip_str = NULL;
    int srv_port = 0;

    tls_client_dpm_sample_svr_info_t *srv_info_ptr = &srv_info;

    if (dpm_mode_is_wakeup()) {
        return pdPASS;
    }

    memset(srv_info_ptr, 0x00, sizeof(tls_client_dpm_sample_svr_info_t));

    //Read server IP address in nvram
    srv_ip_str = read_nvram_string(TLS_CLIENT_DPM_SAMPLE_NVRAM_SERVER_IP_NAME);
    if (srv_ip_str == NULL) {
        srv_ip_str = TLS_CLIENT_DPM_SAMPLE_DEF_SERVER_IP;
    }

    //Set server IP address
    strcpy(srv_info_ptr->ip_addr, srv_ip_str);

    //Read server port in nvram
    if (read_nvram_int(TLS_CLIENT_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME, &srv_port)) {
        srv_port = TLS_CLIENT_DPM_SAMPLE_DEF_SERVER_PORT;
    }

    //Set server port
    srv_info_ptr->port = srv_port;

    PRINTF(" [%s] TLS Server Information(%s:%d)\n",
           __func__, srv_info_ptr->ip_addr, srv_info_ptr->port);

    return pdPASS;
}

void tls_client_dpm_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    unsigned char status = pdPASS;

    PRINTF(" [%s] Start TLS Client DPM Sample\n", __func__);

    //Load server information
    status = tls_client_dpm_sample_load_server_info();
    if (status == pdFAIL) {
        PRINTF(" [%s] Failed to load TLS Server information(0x%02x)\n",
               __func__, status);
        vTaskDelete(NULL);
        return ;
    }

    //Register user configuration
    dpm_mng_regist_config_cb(tls_client_dpm_sample_init_user_config);

    //Start TLS Client Sample
    dpm_mng_start();

    vTaskDelete(NULL);

    return ;
}

#endif // (__TLS_CLIENT_DPM_SAMPLE__) && (__SUPPORT_DPM_MANAGER__)

/* EOF */
