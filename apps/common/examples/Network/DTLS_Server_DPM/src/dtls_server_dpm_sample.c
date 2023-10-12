/**
 ****************************************************************************************
 *
 * @file dtls_server_dpm_sample.c
 *
 * @brief Sample app of DTLS Server in DPM mode.
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

#if defined (__DTLS_SERVER_DPM_SAMPLE__)			\
		&& defined (__SUPPORT_DPM_MANAGER__)		\
		&& !defined (__LIGHT_DPM_MANAGER__)

#include "da16x_system.h"
#include "common_utils.h"
#include "common_def.h"

#include "user_dpm_manager.h"

//Defalut DTLS Server configuration
#define DTLS_SERVER_DPM_SAMPLE_DEF_SERVER_PORT			DTLS_SVR_TEST_PORT

#define	DTLS_SERVER_DPM_SAMPLE_RECEIVE_TIMEOUT			100	/* msec */
#define	DTLS_SERVER_DPM_SAMPLE_HANDSAHKE_MIN_TIMEOUT	100	/* msec */
#define	DTLS_SERVER_DPM_SAMPLE_HANDSAHKE_MAX_TIMEOUT	400	/* msec */

//NVRAM
#define DTLS_SERVER_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME	"DTLS_SERVER_PORT"

//Hex Dump
#undef	DTLS_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP

const unsigned char  dtls_server_dpm_sample_cert[] =
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
const size_t dtls_server_dpm_sample_cert_len = sizeof(dtls_server_dpm_sample_cert);

const unsigned char dtls_server_dpm_sample_key[] =
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
const size_t dtls_server_dpm_sample_key_len = sizeof(dtls_server_dpm_sample_key);

//DTLS Server inforamtion
typedef struct {
    int port;
} dtls_server_dpm_sample_svr_info_t;

//Function prototype
void dtls_server_dpm_sample_init_callback();
void dtls_server_dpm_sample_wakeup_callback();
void dtls_server_dpm_sample_error_callback(UINT error_code, char *comment);
void dtls_server_dpm_sample_connect_callback(void *sock, UINT conn_status);
void dtls_server_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port);
void dtls_server_dpm_sample_secure_callback(void *config);
void dtls_server_dpm_sample_init_user_config(dpm_user_config_t *user_config);
unsigned char dtls_server_dpm_sample_load_server_info();
void dtls_server_dpm_sample(void *param);

//Global variable
dtls_server_dpm_sample_svr_info_t srv_info = {0x00,};

//Function
static void dtls_server_dpm_sample_hex_dump(const char *title, UCHAR *buf, size_t len)
{
#if defined (DTLS_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(UCHAR * data, UINT length);

    PRINTF("%s(%ld)\n", title, len);

    hex_dump(buf, len);
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (DTLS_SERVER_DPM_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

static int dtls_server_dpm_sample_rsa_decrypt_func( void *ctx, int mode, size_t *olen,
        const unsigned char *input, unsigned char *output,
        size_t output_max_len )
{
    return ( mbedtls_rsa_pkcs1_decrypt( (mbedtls_rsa_context *) ctx, NULL, NULL, mode, olen,
                                        input, output, output_max_len ) );
}

static int dtls_server_dpm_sample_rsa_sign_func( void *ctx,
        int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
        int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
        const unsigned char *hash, unsigned char *sig )
{
    return ( mbedtls_rsa_pkcs1_sign( (mbedtls_rsa_context *) ctx, f_rng, p_rng, mode,
                                     md_alg, hashlen, hash, sig ) );
}

static size_t dtls_server_dpm_sample_rsa_key_len_func( void *ctx )
{
    return ( ((const mbedtls_rsa_context *) ctx)->len );
}

void dtls_server_dpm_sample_init_callback()
{
    PRINTF(GREEN_COLOR " [%s] Boot initialization\n" CLEAR_COLOR,
           __func__);

    dpm_mng_job_done(); //Done opertaion
}

void dtls_server_dpm_sample_wakeup_callback()
{
    PRINTF(GREEN_COLOR " [%s] DPM Wakeup\n" CLEAR_COLOR, __func__);

    dpm_mng_job_done(); //Done opertaion
}

void dtls_server_dpm_sample_error_callback(UINT error_code, char *comment)
{
    PRINTF(RED_COLOR " [%s] Error Callback(%s - 0x%0x) \n" CLEAR_COLOR,
           __func__, comment, error_code);
}

void dtls_server_dpm_sample_connect_callback(void *sock, UINT conn_status)
{
    DA16X_UNUSED_ARG(sock);

    PRINTF(GREEN_COLOR " [%s] DTLS Connection Result(0x%x) \n" CLEAR_COLOR,
           __func__, conn_status);

    dpm_mng_job_done(); //Done opertaion
}

void dtls_server_dpm_sample_recv_callback(void *sock, UCHAR *rx_buf, UINT rx_len,
        ULONG rx_ip, ULONG rx_port)
{
    DA16X_UNUSED_ARG(sock);

    unsigned char status = pdPASS;

    //Display received packet
    PRINTF(" =====> Received Packet(%ld) \n", rx_len);

    dtls_server_dpm_sample_hex_dump("Recevied Packet", rx_buf, rx_len);

    //Echo message
    status = dpm_mng_send_to_session(SESSION1, rx_ip, rx_port, (char *)rx_buf, rx_len);
    if (status) {
        PRINTF(RED_COLOR " [%s] Fail send data(session%d,0x%x) \n" CLEAR_COLOR,
               __func__, SESSION1, status);
    } else {
        //Display sent packet
        PRINTF(" <===== Sent Packet(%ld) \n", rx_len);

        dtls_server_dpm_sample_hex_dump("Sent Packet", rx_buf, rx_len);
    }

    dpm_mng_job_done(); //Done opertaion
}

void dtls_server_dpm_sample_secure_callback(void *config)
{
    int ret = 0;

    SECURE_INFO_T *secure_config = (SECURE_INFO_T *)config;

    ret = mbedtls_ssl_config_defaults(secure_config->ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        PRINTF(" [%s] Failed to set default ssl config(0x%x)\n", __func__, -ret);
        return ;
    }

    //import test certificate
    ret = mbedtls_x509_crt_parse(secure_config->cert_crt,
                                 dtls_server_dpm_sample_cert,
                                 dtls_server_dpm_sample_cert_len);
    if (ret) {
        PRINTF("[%s] Failed to parse cert(0x%x)\n", __func__, -ret);
        return ;
    }

    ret = mbedtls_pk_parse_key(secure_config->pkey_ctx,
                               dtls_server_dpm_sample_key,
                               dtls_server_dpm_sample_key_len,
                               NULL, 0);
    if (ret) {
        PRINTF("[%s] Failed to parse private key(0x%x)\n", __func__, -ret);
        return ;
    }

    if (mbedtls_pk_get_type(secure_config->pkey_ctx) == MBEDTLS_PK_RSA) {
        ret = mbedtls_pk_setup_rsa_alt(secure_config->pkey_alt_ctx,
                                       (void *)mbedtls_pk_rsa(*secure_config->pkey_ctx),
                                       dtls_server_dpm_sample_rsa_decrypt_func,
                                       dtls_server_dpm_sample_rsa_sign_func,
                                       dtls_server_dpm_sample_rsa_key_len_func);
        if (ret) {
            PRINTF("[%s] Failed to set rsa alt(0x%x)\n", __func__, -ret);
            return ;
        }

        ret = mbedtls_ssl_conf_own_cert(secure_config->ssl_conf,
                                        secure_config->cert_crt,
                                        secure_config->pkey_alt_ctx);
        if (ret) {
            PRINTF("[%s] Failed to set cert & private key(0x%x)\n", __func__, -ret);
            return ;
        }
    } else {
        ret = mbedtls_ssl_conf_own_cert(secure_config->ssl_conf,
                                        secure_config->cert_crt,
                                        secure_config->pkey_ctx);

        if (ret) {
            PRINTF("[%s] Failed to set cert & private key(0x%x)\n", __func__, -ret);
            return ;
        }
    }

    ret = dpm_mng_setup_rng(secure_config->ssl_conf);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set random generator(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    ret = dpm_mng_cookie_setup_rng(secure_config->cookie_ctx);
    if (ret) {
        PRINTF(" [%s:%d] Failed to set cookie(0x%x\n", __func__, __LINE__, -ret);
        return;
    }

    mbedtls_ssl_conf_dtls_cookies(secure_config->ssl_conf, mbedtls_ssl_cookie_write,
                                  mbedtls_ssl_cookie_check,
                                  secure_config->cookie_ctx);

    //Don't care verification in this sample.
    mbedtls_ssl_conf_authmode(secure_config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_max_frag_len(secure_config->ssl_conf, 0);	//use default value
    mbedtls_ssl_conf_dtls_anti_replay(secure_config->ssl_conf,
                                      MBEDTLS_SSL_ANTI_REPLAY_ENABLED);
    mbedtls_ssl_conf_read_timeout(secure_config->ssl_conf,
                                  DTLS_SERVER_DPM_SAMPLE_RECEIVE_TIMEOUT * 10);
    mbedtls_ssl_conf_handshake_timeout(secure_config->ssl_conf,
                                       DTLS_SERVER_DPM_SAMPLE_HANDSAHKE_MIN_TIMEOUT * 10,
                                       DTLS_SERVER_DPM_SAMPLE_HANDSAHKE_MAX_TIMEOUT * 10);

    ret = mbedtls_ssl_setup(secure_config->ssl_ctx, secure_config->ssl_conf);
    if (ret) {
        PRINTF("[%s] Failed to set ssl config(0x%x)\n", __func__, -ret);
        return ;
    }

    dpm_mng_job_done(); //Done opertaion

    return ;
}

void dtls_server_dpm_sample_init_user_config(dpm_user_config_t *user_config)
{
    const int session_idx = 0;

    if (!user_config) {
        PRINTF(" [%s] Invalid parameter\n", __func__);
        return ;
    }

    //Set Boot init callback
    user_config->bootInitCallback = dtls_server_dpm_sample_init_callback;

    //Set DPM wakkup init callback
    user_config->wakeupInitCallback = dtls_server_dpm_sample_wakeup_callback;

    //Set Error callback
    user_config->errorCallback = dtls_server_dpm_sample_error_callback;

    //Set session type(UDP Server)
    user_config->sessionConfig[session_idx].sessionType = REG_TYPE_UDP_SERVER;

    //Set local port
    user_config->sessionConfig[session_idx].sessionMyPort =
        DTLS_SERVER_DPM_SAMPLE_DEF_SERVER_PORT;

    //Set Connection callback
    user_config->sessionConfig[session_idx].sessionConnectCallback =
        dtls_server_dpm_sample_connect_callback;

    //Set Recv callback
    user_config->sessionConfig[session_idx].sessionRecvCallback =
        dtls_server_dpm_sample_recv_callback;

    //Set secure mode
    user_config->sessionConfig[session_idx].supportSecure = pdTRUE;

    //Set secure setup callback
    user_config->sessionConfig[session_idx].sessionSetupSecureCallback =
        dtls_server_dpm_sample_secure_callback;

    //Set user configuration
    user_config->ptrDataFromRetentionMemory = (UCHAR *)&srv_info;
    user_config->sizeOfRetentionMemory = sizeof(dtls_server_dpm_sample_svr_info_t);

    return ;
}

unsigned char dtls_server_dpm_sample_load_server_info()
{
    int srv_port = 0;

    dtls_server_dpm_sample_svr_info_t *srv_info_ptr = &srv_info;

    memset(srv_info_ptr, 0x00, sizeof(dtls_server_dpm_sample_svr_info_t));

    //Read server port in nvram
    if (read_nvram_int(DTLS_SERVER_DPM_SAMPLE_NVRAM_SERVER_PORT_NAME, &srv_port)) {
        srv_port = DTLS_SERVER_DPM_SAMPLE_DEF_SERVER_PORT;
    }

    //Set server port
    srv_info_ptr->port = srv_port;

    PRINTF(" [%s] DTLS Server Information(%d)\n",
           __func__, srv_info_ptr->port);

    return pdPASS;
}

void dtls_server_dpm_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    unsigned char status = pdPASS;

    PRINTF(" [%s] Start DTLS Server Sample\n", __func__);

    //Load server information
    status = dtls_server_dpm_sample_load_server_info();
    if (status == pdFAIL) {
        PRINTF(" [%s] Failed to load DTLS Server information(0x%02x)\n",
               __func__, status);
        vTaskDelete(NULL);
        return ;
    }

    //Register user configuration
    dpm_mng_regist_config_cb(dtls_server_dpm_sample_init_user_config);

    //Start DTLS Server Sample
    dpm_mng_start();

    vTaskDelete(NULL);
    return ;
}

#endif // (__DTLS_SERVER_DPM_SAMPLE__) && (__SUPPORT_DPM_MANAGER) && !(__LIGHT_DPM_MANAGER__)

/* EOF */
