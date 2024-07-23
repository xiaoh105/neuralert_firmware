/**
 ****************************************************************************************
 *
 * @file dtls_client_sample.c
 *
 * @brief DTLS Client sample code
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


#if defined(__DTLS_CLIENT_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "sample_defs.h"
#include "da16x_crypto.h"
#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#undef	DTLS_CLIENT_SAMPLE_ENABLED_HEXDUMP

#define DTLS_CLIENT_SAMPLE_DEF_TIMER_NAME				"dtls_client_timer"
#define DTLS_CLIENT_SAMPLE_DEF_BUF_SIZE					(1024 * 1)
#define DTLS_CLIENT_SAMPLE_DEF_TIMEOUT					10000 // ms
#define DTLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR			"192.168.0.11"
#define DTLS_CLIENT_SAMPLE_DEF_SERVER_PORT				DTLS_SVR_TEST_PORT
#define DTLS_CLIENT_SAMPLE_DEF_HANDSHAKE_MIN_TIMEOUT	1000 // ms
#define DTLS_CLIENT_SAMPLE_DEF_HANDSHAKE_MAX_TIMEOUT	8000 // ms

typedef enum _dtls_client_sample_timer_state_t {
    CANCELLED = -1,
    NO_EXPIRY = 0,
    INT_EXPIRY = 1,
    FIN_EXPIRY = 2
} dtls_client_sample_timer_state_t;

typedef struct _dtls_client_sample_timer_t {
    unsigned int int_ms; // Intermediate delay in msec
    unsigned int fin_ms; // Final delay in msec
    TickType_t snapshot; // Tick
} dtls_client_sample_timer_t;

//Function properties
void dtls_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void dtls_client_sample_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms);
int dtls_client_sample_timer_get_state(void *ctx);
void dtls_client_sample(void *params);

void dtls_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (DTLS_CLIENT_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (DTLS_CLIENT_SAMPLE_ENABLED_HEXDUMP)
    return ;
}

void dtls_client_sample_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
    dtls_client_sample_timer_t *timer = NULL;

    if (!ctx) {
        PRINTF("[%s] Invaild paramter\n", __func__);
        return ;
    }

    timer = (dtls_client_sample_timer_t *)ctx;

    timer->int_ms = int_ms;
    timer->fin_ms = fin_ms;

    if (fin_ms != 0) {
        timer->snapshot = xTaskGetTickCount();
    }

    return ;
}

int dtls_client_sample_timer_get_state(void *ctx)
{
    dtls_client_sample_timer_t *timer = NULL;
    unsigned int elapsed = 0;

    if (!ctx) {
        PRINTF("[%s] Invaild paramter\n", __func__);
        return CANCELLED;
    }

    timer = (dtls_client_sample_timer_t *)ctx;

    if (timer->fin_ms == 0) {
        return CANCELLED;
    }

    elapsed = xTaskGetTickCount() - timer->snapshot;

    if (elapsed >= pdMS_TO_TICKS(timer->fin_ms)) {
        return FIN_EXPIRY;
    }

    if (elapsed >= pdMS_TO_TICKS(timer->int_ms)) {
        return INT_EXPIRY;
    }

    return NO_EXPIRY;
}

void dtls_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    const char *pers = "dtls_client";
    int ret = 0;
    char data_buffer[DTLS_CLIENT_SAMPLE_DEF_BUF_SIZE] = {0x00,};
    size_t len = sizeof(data_buffer);
    char str_port[8] = {0x00,};

    mbedtls_net_context server_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    dtls_client_sample_timer_t timer;

    PRINTF("[%s] Start of DTLS Client sample\n", __func__);

    //Init session
    mbedtls_net_init(&server_ctx);

    //Init SSL context
    mbedtls_ssl_init(&ssl_ctx);

    //Init SSL config
    mbedtls_ssl_config_init(&ssl_conf);

    //Init CTR-DRBG context
    mbedtls_ctr_drbg_init(&ctr_drbg);

    //Init Entropy context
    mbedtls_entropy_init(&entropy);

    memset(&timer, 0x00, sizeof(dtls_client_sample_timer_t));

    PRINTF("\nConnecting to udp/%s:%d...\n",
           DTLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR,
           DTLS_CLIENT_SAMPLE_DEF_SERVER_PORT);

    snprintf(str_port, sizeof(str_port), "%d", DTLS_CLIENT_SAMPLE_DEF_SERVER_PORT);

    ret = mbedtls_net_connect(&server_ctx,
                              DTLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR, str_port,
                              MBEDTLS_NET_PROTO_UDP);
    if (ret) {
        PRINTF("\n[%s] Failed to connect DTLS session(0x%x)\n", __func__, -ret);
        goto end_of_task;
    }

    PRINTF("ok\n");

    PRINTF("Setting up the DTLS structure...\n");

    ret = mbedtls_ssl_config_defaults(&ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF("\n[%s] Failed to init dtls config(0x%x)\n", __func__, -ret);
        goto end_of_task;
    }

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret) {
        PRINTF("\n[%s] Failed to set CTR-DRBG(0x%x)\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_conf_rng(&ssl_conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_dtls_anti_replay(&ssl_conf, MBEDTLS_SSL_ANTI_REPLAY_ENABLED);
    mbedtls_ssl_conf_read_timeout(&ssl_conf, DTLS_CLIENT_SAMPLE_DEF_TIMEOUT);
    mbedtls_ssl_conf_handshake_timeout(&ssl_conf,
                                       DTLS_CLIENT_SAMPLE_DEF_HANDSHAKE_MIN_TIMEOUT,
                                       DTLS_CLIENT_SAMPLE_DEF_HANDSHAKE_MAX_TIMEOUT);

    ret = mbedtls_ssl_setup(&ssl_ctx, &ssl_conf);
    if (ret) {
        PRINTF("\n[%s] Failed to setup dtls contex(0x%x)\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_set_bio(&ssl_ctx, &server_ctx,
                        mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

    mbedtls_ssl_set_timer_cb(&ssl_ctx, &timer,
                             dtls_client_sample_timer_start,
                             dtls_client_sample_timer_get_state);

    PRINTF("ok\n");

    PRINTF("Performing the DTLS handshake...\n");

    while ((ret = mbedtls_ssl_handshake(&ssl_ctx)) != 0) {
        if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
            PRINTF("\n[%s] Peer closed the connection(0x%x)\n", __func__, -ret);
            goto end_of_task;
        }

        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            PRINTF("\n[%s] Failed to do dtls handshake(0x%x)\n", __func__, -ret);
            goto end_of_task;
        }
    }

    PRINTF("ok\n");

    //Data transmit
    do {

        len = sizeof(data_buffer) - 1;
        memset(data_buffer, 0x00, sizeof(data_buffer));

        PRINTF("< Read from server: ");

        ret = mbedtls_ssl_read(&ssl_ctx, (unsigned char *)data_buffer, len);
        if (ret <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\nNeed more data - mbedtls_ssl_read(0x%x)\n", -ret);
                continue;
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                PRINTF("\nConnection was closed gracefully\n");
                goto end_of_task;
            case MBEDTLS_ERR_NET_CONN_RESET:
                PRINTF("\nConnection was reset by peer\n");
                goto end_of_task;
            default:
                PRINTF("\nFailed to read data(0x%x)\n", -ret);
                break;
            }
            goto end_of_task;
        }

        len = ret;

        PRINTF("%d bytes read\n", len);

        dtls_client_sample_hex_dump("Received data", (unsigned char *)data_buffer, len);

        PRINTF("> Write to server: ");

        while ((ret = mbedtls_ssl_write(&ssl_ctx, (unsigned char *)data_buffer, len)) <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\nNeed more data - mbedtls_ssl_write(0x%x)\n", -ret);
                continue;
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                PRINTF("\nConnection was closed gracefully\n");
                goto end_of_task;
            case MBEDTLS_ERR_NET_CONN_RESET:
                PRINTF("\nConnection was reset by peer\n");
                goto end_of_task;
            default:
                PRINTF("\nFailed to write data(0x%x)\n", -ret);
                break;
            }
            goto end_of_task;
        }

        PRINTF("%d bytes written\n", len);

        dtls_client_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
    } while (1);

end_of_task:

    PRINTF("[%s] End of DTLS Client sample\n", __func__);

    //Deinit session
    mbedtls_net_free(&server_ctx);

    //Deinit SSL context
    mbedtls_ssl_free(&ssl_ctx);

    //Deinit SSL config
    mbedtls_ssl_config_free(&ssl_conf);

    //Deinit CTR-DRBG context
    mbedtls_ctr_drbg_free(&ctr_drbg);

    //Deinit Entropy context
    mbedtls_entropy_free(&entropy);

    vTaskDelete(NULL);

    return ;
}
#endif // (__DTLS_CLIENT_SAMPLE__)

/* EOF */
