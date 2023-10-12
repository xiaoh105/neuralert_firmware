/**
 ****************************************************************************************
 *
 * @file tls_client_sample.c
 *
 * @brief TLS Client sample code
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


#if defined(__TLS_CLIENT_SAMPLE__)

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

#undef TLS_CLIENT_SAMPLE_ENABLED_HEXDUMP

#define TLS_CLIENT_SAMPLE_DEF_BUF_SIZE			(1024 * 1)
#define TLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR	"192.168.0.11"
#define TLS_CLIENT_SAMPLE_DEF_SERVER_PORT		TLS_CLI_TEST_PORT

//function properties
void tls_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void tls_client_sample(void *param);

//bodies
void tls_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (TLS_CLIENT_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (TLS_CLIENT_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void tls_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    const char *pers = "tls_client";
    int ret = 0;
    char data_buffer[TLS_CLIENT_SAMPLE_DEF_BUF_SIZE] = {0x00,};
    size_t len = sizeof(data_buffer);
    char str_port[8] = {0x00,};

    mbedtls_net_context server_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    PRINTF("[%s] Start of TLS Client sample\r\n", __func__);

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

    PRINTF("\r\nConnecting to tcp/%s:%d...",
           TLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR,
           TLS_CLIENT_SAMPLE_DEF_SERVER_PORT);

    snprintf(str_port, sizeof(str_port), "%d", TLS_CLIENT_SAMPLE_DEF_SERVER_PORT);

    ret = mbedtls_net_connect(&server_ctx,
                              TLS_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR, str_port,
                              MBEDTLS_NET_PROTO_TCP);
    if (ret) {
        PRINTF("\r\n[%s] Failed to connect TLS session(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    PRINTF("ok\r\n");

    PRINTF("Setting up the TLS structure...");

    //Set default configuration
    ret = mbedtls_ssl_config_defaults(&ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF("\r\n[%s] Failed to init tls config(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret) {
        PRINTF("\r\n[%s] Failed to set CTR-DRBG(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_conf_rng(&ssl_conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    //Don't care verification in this sample.
    mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_NONE);

    //Setup an SSL context for use.
    ret = mbedtls_ssl_setup(&ssl_ctx, &ssl_conf);
    if (ret) {
        PRINTF("\r\n[%s] Failed to setup dtls contex(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_set_bio(&ssl_ctx, &server_ctx,
                        mbedtls_net_send, mbedtls_net_recv, NULL);

    PRINTF("ok\r\n");

    PRINTF("Performing the TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl_ctx)) != 0) {
        if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
            PRINTF("\r\n[%s] Peer closed the connection(0x%x)\r\n", __func__, -ret);
            goto end_of_task;
        }

        if ((ret != MBEDTLS_ERR_SSL_WANT_READ)
                && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            PRINTF("\r\n[%s] Failed to do tls handshake(0x%x)\r\n", __func__, -ret);
            goto end_of_task;
        }
    }

    PRINTF("ok\r\n");

    //data transmit
    do {
        len = sizeof(data_buffer) - 1;
        memset(data_buffer, 0x00, sizeof(data_buffer));

        PRINTF("< Read from server: ");

        //Read at most 'len' application data bytes.
        ret = mbedtls_ssl_read(&ssl_ctx, (unsigned char *)data_buffer, len);
        if (ret <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\r\nNeed more data - mbedtls_ssl_read(0x%x)\r\n", -ret);
                continue;
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                PRINTF("\r\nConnection was closed gracefully\r\n");
                goto end_of_task;
            case MBEDTLS_ERR_NET_CONN_RESET:
                PRINTF("\r\nConnection was reset by peer\r\n");
                goto end_of_task;
            default:
                PRINTF("\r\nFailed to read data(0x%x)\r\n", -ret);
                break;
            }
            goto end_of_task;
        }

        len = ret;

        PRINTF("%d bytes read\r\n", len);

        tls_client_sample_hex_dump("Received data", (unsigned char *)data_buffer, len);

        PRINTF("> Write to server: ");

        //Try to write exactly 'len' application data bytes.
        while ((ret = mbedtls_ssl_write(&ssl_ctx, (unsigned char *)data_buffer, len)) <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\r\nNeed more data - mbedtls_ssl_write(0x%x)\r\n", -ret);
                continue;
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                PRINTF("\r\nConnection was closed gracefully\r\n");
                break;
            case MBEDTLS_ERR_NET_CONN_RESET:
                PRINTF("\r\nConnection was reset by peer\r\n");
                break;
            default:
                PRINTF("\r\nFailed to write data(0x%x)\r\n", -ret);
                break;
            }
            goto end_of_task;
        }

        PRINTF("%d bytes written\r\n", len);

        tls_client_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
    } while (1);

end_of_task:

    PRINTF("[%s] End of TLS Client sample\r\n", __func__);

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
#endif // (__TLS_CLIENT_SAMPLE__)
