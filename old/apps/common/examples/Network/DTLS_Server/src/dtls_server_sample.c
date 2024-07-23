/**
 ****************************************************************************************
 *
 * @file dtls_server_sample.c
 *
 * @brief DTLS Server sample code
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


#if defined(__DTLS_SERVER_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "da16x_crypto.h"
#include "task.h"
#include "sample_defs.h"
#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/pk.h"
#include "mbedtls/ssl_cookie.h"

#undef	DTLS_SERVER_SAMPLE_ENABLED_HEXDUMP

#define DTLS_SERVER_SAMPLE_DEF_TIMER_NAME				"dtls_server_timer"
#define DTLS_SERVER_SAMPLE_DEF_PORT						DTLS_SVR_TEST_PORT
#define DTLS_SERVER_SAMPLE_DEF_BUF_SIZE					(1024 * 1)
#define DTLS_SERVER_SAMPLE_DEF_TIMEOUT					10000 // ms
#define DTLS_SERVER_SAMPLE_DEF_HANDSHAKE_MIN_TIMEOUT	1000 // ms
#define DTLS_SERVER_SAMPLE_DEF_HANDSHAKE_MAX_TIMEOUT	8000 // ms

const unsigned char dtls_server_sample_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDBjCCAe4CCQCg5xtL5ap/CDANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\n"
    "VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\n"
    "cyBQdHkgTHRkMB4XDTE4MDgyMTA3Mzg0M1oXDTI4MDgxODA3Mzg0M1owRTELMAkG\n"
    "A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\n"
    "IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
    "ALMgpDjh0c0ZThAxE/B2mBcDAp2KUaeoXqY5+03bZWRQ7McE0og3DV9u14FGwhDQ\n"
    "J93itsHXnWGZ6Zh1xtPJzoMiaxxaAe+1WdognwlTD8LUr7qS/7VtFXXmvVaIQ7E5\n"
    "AKLxlFsgVc3soSLo4f06DWOGl7pSIC2EttYFKMvBmlWcksEA1+DXP/NJy8h3wqTF\n"
    "MC+iydE5cXd3lKJ3CQ+0zTJli4DOF5ZudbrkWOQXwfZ1ylB24umvbNMSfltlSkuC\n"
    "km0VhF8VCLFwek+WNcrqeDkAAyMyRyVlDi+9r1qTHPF+EYwcJHWh+zFegztGc1w6\n"
    "7mxBBh+JRBH7GzWLT9kBg7kCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAWJVjI3po\n"
    "zjCpsfrPw1u/K/0AJGRpLChvBPMYz11/Ay2I2vyT8akRM+7km0W/d+wiyL0YT9QI\n"
    "PHT4gLEV4KeiQSAPindtJgbg2Wi53EBGeNrDWHMwH3islDr9h293zhUqZMie2WDb\n"
    "kpEcSh79DAhZDZ43d/qHgWeyWWkyC8JLQIzYPsdTJYf8qq7yNZlr7OZCHXdq2sG8\n"
    "SjE4I1j4ANkkJpnX6yFDZZGDDrUzPcJRPVtZzmytJUIbzoTU9E6nSDIlvBTT273o\n"
    "9gJXvkU7L4cwNCAg0g3RRwyqIUq8zyWsIeNSdqz64EIH2wX4XvWkVzDzfxW76HL7\n"
    "NOds4a5SIHfFZw==\n"
    "-----END CERTIFICATE-----\n";
const size_t dtls_server_sample_cert_len = sizeof(dtls_server_sample_cert);

const unsigned char dtls_server_sample_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEAsyCkOOHRzRlOEDET8HaYFwMCnYpRp6hepjn7TdtlZFDsxwTS\n"
    "iDcNX27XgUbCENAn3eK2wdedYZnpmHXG08nOgyJrHFoB77VZ2iCfCVMPwtSvupL/\n"
    "tW0Vdea9VohDsTkAovGUWyBVzeyhIujh/ToNY4aXulIgLYS21gUoy8GaVZySwQDX\n"
    "4Nc/80nLyHfCpMUwL6LJ0Tlxd3eUoncJD7TNMmWLgM4Xlm51uuRY5BfB9nXKUHbi\n"
    "6a9s0xJ+W2VKS4KSbRWEXxUIsXB6T5Y1yup4OQADIzJHJWUOL72vWpMc8X4RjBwk\n"
    "daH7MV6DO0ZzXDrubEEGH4lEEfsbNYtP2QGDuQIDAQABAoIBADniYG8pOhznAnzk\n"
    "/yaDjF5TULMMEZr2I6/fqL/eGAO0yu79NfNipuWh8e4KqYe5XEitjJVTUb5KeFwW\n"
    "IywpWJyzsJ020M1fcyuzwvDGcJ9rD2ZhPlSobXjuGV0vJ4DLhNMi8egIqPGkd+XK\n"
    "D80+xzjUM4+4HkHXUyYSAL7nTzI+nh9mm3yCntFSJsNj1tRX7IVjnjh1RIS7MR5B\n"
    "f0u9xVGf3UbBveK/RyVGd/QWJacktBcr3WyCxHkwo64TETuj5D3NxqIrmiChBauB\n"
    "ediqoe3RuxPw6vgE3+z3mfDdD0Xb58CsvN27dW//euf1xZjjw6xLqAo3Zd8fAMME\n"
    "EFDURiECgYEA1mX/Z0OoNId3SoFhg5NkQlEBSb2Lna35JtqhuylPLarjapoGj3C/\n"
    "4z0hAIU6d1W3L12P175XaHrDuA51Tc9bnMp/JuSrV6r5CvmZJ81MftYlJzl41zJd\n"
    "JIf1nILxlXvQS9rVHSqIO/0dPwEAkGKJHqqhyWTM5sh3CcgpQy7JsL0CgYEA1eKZ\n"
    "pDdgkLRQgmtkW0FDIkR1aG6J6gE/TpmmSY64xqEYr5rnGRObvYrGQXHp9oD/QuCH\n"
    "fiVeudvuyQYXt0KsEv0NyhoLWmAlTij1eKEjUmvHrTV3ydMSdEIs5p5k6w3Ht8KU\n"
    "4YAjjFglL+0xDV9EC6nwB4swNwKfBpifuwkspK0CgYB1RFjMHJ92C9pdsCKsGwQt\n"
    "ma0ArmIdHrk2XUM04cVjDyNQfWq1LlBmdFsGs9hkyUdm6t/wezXH+c3vcEkNBCvx\n"
    "uHiPx2dIjkWlkRwKPypl/a9YowDLg8qaXpsiviRxRMWLl+gVCdx2I13JxjyOvLaP\n"
    "RXk0dKP2XxNtEEQxcPf0aQKBgQDHXzzcqIopGQvbJoQb1E/iB3Jx8Gg6awM6H1u0\n"
    "QYfYD57VQk2dQHvySQPZSXhPwZswGd/zJJ6SHYMOe9FrkIiaAqzx8SkYC3t6yg9X\n"
    "bM1iLPmqaabJySjwmicEqi1kNiovDwB821dHoXq4nB8XWfAx9yy5u3MsNBNMsMRk\n"
    "Mn8c2QKBgQC0OKboeUgzk9i06KE6CqGO43WpPDKNG+4ylnCnIxjeGIYLNKHJ+RCv\n"
    "4lt+FHWcwciM3JoNJWSfqXGcqp2B97I/YDW87L6He4ShUmzvdlAG1CecGqTYBrYB\n"
    "lcr/+RukWzIMUCA24KeAoheKsC+HJGg4U08nWz5n9K30kqNya+nz9g==\n"
    "-----END RSA PRIVATE KEY-----\n";
const size_t dtls_server_sample_key_len = sizeof(dtls_server_sample_key);

typedef enum _dtls_server_sample_timer_state_t {
    CANCELLED = -1,
    NO_EXPIRY = 0,
    INT_EXPIRY = 1,
    FIN_EXPIRY = 2
} dtls_server_sample_timer_state_t;

typedef struct _dtls_server_sample_timer_t {
    unsigned int int_ms; // Intermediate delay in msec
    unsigned int fin_ms; // Final delay in msec
    TickType_t snapshot; // Tick
} dtls_server_sample_timer_t;

//Function properties
void dtls_server_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void dtls_server_sample_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms);
int dtls_server_sample_timer_get_state(void *ctx);
int dtls_server_sample_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
                                        const unsigned char *input, unsigned char *output,
                                        size_t output_max_len);
int dtls_server_sample_rsa_sign_func(void *ctx,
                                     int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                                     int mode, mbedtls_md_type_t md_alg,
                                     unsigned int hashlen, const unsigned char *hash,
                                     unsigned char *sig);
size_t dtls_server_sample_rsa_key_len_func(void *ctx);
void dtls_server_sample(void *param);

void dtls_server_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (DTLS_SERVER_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (DTLS_SERVER_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void dtls_server_sample_timer_start(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
    dtls_server_sample_timer_t *timer = NULL;

    if (!ctx) {
        PRINTF("[%s] Invaild paramter\n", __func__);
        return ;
    }

    timer = (dtls_server_sample_timer_t *)ctx;

    timer->int_ms = int_ms;
    timer->fin_ms = fin_ms;

    if (fin_ms != 0) {
        timer->snapshot = xTaskGetTickCount();
    }

    return ;
}

int dtls_server_sample_timer_get_state(void *ctx)
{
    dtls_server_sample_timer_t *timer = NULL;
    unsigned int elapsed = 0;

    if (!ctx) {
        PRINTF("[%s] Invaild paramter\n", __func__);
        return CANCELLED;
    }

    timer = (dtls_server_sample_timer_t *)ctx;

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

int dtls_server_sample_rsa_decrypt_func(void *ctx, int mode, size_t *olen,
                                        const unsigned char *input, unsigned char *output,
                                        size_t output_max_len)
{
    return (mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *)ctx, NULL, NULL, mode,
                                      olen, input, output, output_max_len));
}

int dtls_server_sample_rsa_sign_func(void *ctx,
                                     int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
                                     int mode, mbedtls_md_type_t md_alg,
                                     unsigned int hashlen, const unsigned char *hash,
                                     unsigned char *sig)
{
    return (mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *)ctx, f_rng, p_rng, mode,
                                   md_alg, hashlen, hash, sig));
}

size_t dtls_server_sample_rsa_key_len_func(void *ctx)
{
    return (((const mbedtls_rsa_context *)ctx)->len);
}

void dtls_server_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    const char *pers = "dtls_server";
    int ret = 0;
    char data_buffer[DTLS_SERVER_SAMPLE_DEF_BUF_SIZE] = {0x00,};
    size_t len = sizeof(data_buffer);
    char str_port[8] = {0x00,};
    unsigned char client_ip[16] = {0x00,};
    size_t client_ip_len = 0;
    dtls_server_sample_timer_t timer;

    mbedtls_net_context listen_ctx, client_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
    mbedtls_pk_context pkey_alt;
    mbedtls_ssl_cookie_ctx cookies;

    PRINTF("[%s] Start of DTLS Server sample\r\n", __func__);

    //Init session
    mbedtls_net_init(&listen_ctx);
    mbedtls_net_init(&client_ctx);

    //Init SSL context
    mbedtls_ssl_init(&ssl_ctx);

    //Init SSL config
    mbedtls_ssl_config_init(&ssl_conf);

    //Init CTR-DRBG context
    mbedtls_ctr_drbg_init(&ctr_drbg);

    //Init Entropy context
    mbedtls_entropy_init(&entropy);

    //Init Certificate context
    mbedtls_x509_crt_init(&cert);

    //Init Private key context
    mbedtls_pk_init(&pkey);

    //Init Private key context for ALT
    mbedtls_pk_init(&pkey_alt);

    //Init Cookies
    mbedtls_ssl_cookie_init(&cookies);

    memset(&timer, 0x00, sizeof(dtls_server_sample_timer_t));

    PRINTF("\r\nLoading the server cert. and key...");

    //Parse certificate
    ret = mbedtls_x509_crt_parse(&cert, dtls_server_sample_cert,
                                 dtls_server_sample_cert_len);
    if (ret) {
        PRINTF("\r\n[%s] Failed to parse certificate(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    //Parse private key
    ret = mbedtls_pk_parse_key(&pkey, dtls_server_sample_key,
                               dtls_server_sample_key_len, NULL, 0);
    if (ret) {
        PRINTF("\r\n[%s] Failed to parse private key(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    PRINTF("ok\r\n");

    PRINTF("Bind on udp/*/%d...", DTLS_SERVER_SAMPLE_DEF_PORT);

    snprintf(str_port, sizeof(str_port), "%d", DTLS_SERVER_SAMPLE_DEF_PORT);

    ret = mbedtls_net_bind(&listen_ctx, NULL, str_port, MBEDTLS_NET_PROTO_UDP);
    if (ret) {
        PRINTF("\r\n[%s] Failed to bind socket(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    PRINTF("ok\r\n");

    PRINTF("Seeding the random number generator...");

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret) {
        PRINTF("\r\n[%s] Failed to set CTR-DRBG(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    PRINTF("ok\r\n");

    PRINTF("Setting up the DTLS structure...");

    //Set default configuration
    ret = mbedtls_ssl_config_defaults(&ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF("\r\n[%s] Failed to init dtls config(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_conf_rng(&ssl_conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    //Import certificate & private key
    if (mbedtls_pk_get_type(&pkey) == MBEDTLS_PK_RSA) {
        ret = mbedtls_pk_setup_rsa_alt(&pkey_alt,
                                       (void *)mbedtls_pk_rsa(pkey),
                                       dtls_server_sample_rsa_decrypt_func,
                                       dtls_server_sample_rsa_sign_func,
                                       dtls_server_sample_rsa_key_len_func);
        if (ret) {
            PRINTF("\r\n[%s] Failed to set RSA-ALT(0x%x)\r\n", __func__, -ret);
            goto end_of_task;
        }

        ret = mbedtls_ssl_conf_own_cert(&ssl_conf, &cert, &pkey_alt);
        if (ret) {
            PRINTF("\r\n[%s] Failed to set certificate(0x%x)\r\n", __func__, -ret);
            goto end_of_task;
        }
    } else {
        ret = mbedtls_ssl_conf_own_cert(&ssl_conf, &cert, &pkey);
        if (ret) {
            PRINTF("\r\n[%s] Failed to set certificate(0x%x)\r\n", __func__, -ret);
            goto end_of_task;
        }
    }

    //Setup cookies
    ret = mbedtls_ssl_cookie_setup(&cookies, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("\r\n[%s] Failed to set ssl cookie(0x%x)\r\n", __func__, -ret);

        goto end_of_task;
    }

    //Register callbacks for DTLS cookies.
    mbedtls_ssl_conf_dtls_cookies(&ssl_conf,
                                  mbedtls_ssl_cookie_write,
                                  mbedtls_ssl_cookie_check,
                                  &cookies);

    //Don't care verificate of peer certificate
    mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_NONE);

    //Enable or disable anti-replay protection for DTLS.
    mbedtls_ssl_conf_dtls_anti_replay(&ssl_conf, MBEDTLS_SSL_ANTI_REPLAY_ENABLED);

    mbedtls_ssl_conf_read_timeout(&ssl_conf, DTLS_SERVER_SAMPLE_DEF_TIMEOUT);

    //Set retransmit timeout values for the DTLS handshake.
    mbedtls_ssl_conf_handshake_timeout(&ssl_conf,
                                       DTLS_SERVER_SAMPLE_DEF_HANDSHAKE_MIN_TIMEOUT,
                                       DTLS_SERVER_SAMPLE_DEF_HANDSHAKE_MAX_TIMEOUT);

    //Set up an SSL context for use.
    ret = mbedtls_ssl_setup(&ssl_ctx, &ssl_conf);
    if (ret) {
        PRINTF("[%s] Failed to set ssl config(0x%x)\r\n", __func__, -ret);
        goto end_of_task;
    }

    mbedtls_ssl_set_timer_cb(&ssl_ctx, &timer, dtls_server_sample_timer_start,
                             dtls_server_sample_timer_get_state);

    PRINTF("ok\r\n");

reset:

    mbedtls_net_free(&client_ctx);

    mbedtls_ssl_session_reset(&ssl_ctx);

    //Wait until a client connects
    PRINTF("Waiting for a remote connection...");

    ret = mbedtls_net_accept(&listen_ctx, &client_ctx, client_ip, sizeof(client_ip),
                             &client_ip_len);
    if (ret) {
        PRINTF("\r\n[%s] Failed to accept socket(0x%x)\r\n", __func__, -ret);

        goto end_of_task;
    }

    //Set client ip address
    ret = mbedtls_ssl_set_client_transport_id(&ssl_ctx, client_ip, client_ip_len);
    if (ret) {
        PRINTF("[%s] Failed to set client trasport id(0x%x)\r\n", __func__, -ret);

        goto end_of_task;
    }

    mbedtls_ssl_set_bio(&ssl_ctx, &client_ctx, mbedtls_net_send, NULL,
                        mbedtls_net_recv_timeout);

    PRINTF("ok\r\n");

    PRINTF("Performing the DTLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl_ctx)) != 0) {
        if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
            continue;
        }

        if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
            PRINTF("hello verification requested\r\n");
            ret = 0;
            goto reset;
        } else {
            PRINTF("\r\n[%s] Failed to do handshake(0x%x)\r\n", __func__, -ret);
            goto reset;
        }
    }

    PRINTF("ok\r\n");

    //Data transmit
    do {

        len = sizeof(data_buffer) - 1;
        memset(data_buffer, 0x00, sizeof(data_buffer));

        PRINTF("< Read from client: ");

        ret = mbedtls_ssl_read(&ssl_ctx, (unsigned char *)data_buffer, len);
        if (ret <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\r\nNeed more data - mbedtls_ssl_write(0x%x)\r\n", -ret);
                continue;
            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                PRINTF("\r\nConnection was closed gracefully\r\n");
                ret = 0;
                goto close_notify;
            case MBEDTLS_ERR_SSL_TIMEOUT:
                PRINTF("\r\nTimeout\r\n");
                goto reset;
            default:
                PRINTF("\r\nFailed to read data(0x%x)\r\n", -ret);
                break;
            }
            goto reset;
        }

        len = ret;

        PRINTF("%d bytes read\r\n", len);

        dtls_server_sample_hex_dump("Received data", (unsigned char *)data_buffer, len);

        PRINTF("> Write to client: ");

        while ((ret = mbedtls_ssl_write(&ssl_ctx, (unsigned char *)data_buffer, len)) <= 0) {
            switch (ret) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                PRINTF("\r\nNeed more data - mbedtls_ssl_write(0x%x)\r\n", -ret);
                continue;
            }

            PRINTF("\r\n[%s] Failed to write data(0x%x)\r\n", __func__, -ret);

            goto end_of_task;
        }

        PRINTF("%d bytes written\r\n", len);

        dtls_server_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
    } while (1);

    // Done, cleanly close the connection
close_notify:

    PRINTF("Closing the connection..." );

    // No error checking, the connection might be closed already
    do {
        ret = mbedtls_ssl_close_notify(&ssl_ctx);
    } while ((ret == MBEDTLS_ERR_SSL_WANT_WRITE));

    ret = 0;

    PRINTF("done\r\n");

    goto reset;

end_of_task:

    PRINTF("[%s] End of DTLS Server sample\r\n", __func__);

    //Deinit session
    mbedtls_net_free(&listen_ctx);
    mbedtls_net_free(&client_ctx);

    //Deinit SSL context
    mbedtls_ssl_free(&ssl_ctx);

    //Deinit SSL config
    mbedtls_ssl_config_free(&ssl_conf);

    //Deinit CTR-DRBG context
    mbedtls_ctr_drbg_free(&ctr_drbg);

    //Deinit Entropy context
    mbedtls_entropy_free(&entropy);

    //Deinit Certificate context
    mbedtls_x509_crt_free(&cert);

    //Deinit Private key context
    mbedtls_pk_free(&pkey);

    //Deinit Private key context for ALT
    mbedtls_pk_free(&pkey_alt);

    //Deinit Cookies
    mbedtls_ssl_cookie_free(&cookies);

    vTaskDelete(NULL);

    return ;
}
#endif // (__DTLS_SERVER_SAMPLE__)
