/**
 ****************************************************************************************
 *
 * @file http_client_sample.c
 *
 * @brief Sample app for HTTP client in DPM mode (GET, POST).
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


#if defined (__HTTP_CLIENT_DPM_SAMPLE__)

#include "sdk_type.h"
#include "sample_defs.h"
#include <ctype.h>
#include "da16x_system.h"
#include "application.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_network_common.h"
#include "da16x_dns_client.h"
#include "util_api.h"
#include "user_http_client.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "lwip/err.h"
#include "mbedtls/ssl.h"
#include "command_net.h"
#include "user_dpm_api.h"

#undef ENABLE_METHOD_POST_TEST
#undef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

#define HTTP_CLIENT_SAMPLE_TASK_NAME	"http_c_dpm_sample"
#define	HTTP_CLIENT_SAMPLE_STACK_SIZE	(1024 * 6) / 4 //WORD

TaskHandle_t g_sample_http_client_xHandle = NULL;

static ip_addr_t g_server_addr;
static httpc_connection_t g_conn_settings = {0, };
static DA16_HTTP_CLIENT_REQUEST g_request;

#if defined (ENABLE_METHOD_POST_TEST)
char user_post_data[256] = {"HTTP-Client POST method sample test!!!"};
unsigned char g_http_url[256] = {"http://httpbin.org/post"};
#else
unsigned char g_http_url[256] = {"http://httpbin.org/get"};
#endif //(ENABLE_METHOD_POST_TEST)

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
#define	CERT_MAX_LENGTH		1024 * 4
#define	FLASH_WRITE_LENGTH	1024 * 4
static void http_client_clear_alpn(httpc_secure_connection_t *conf)
{
    int idx = 0;

    if (conf->alpn) {
        for (idx = 0 ; idx < conf->alpn_cnt ; idx++) {
            if (conf->alpn[idx]) {
                vPortFree(conf->alpn[idx]);
                conf->alpn[idx] = NULL;
            }
        }

        vPortFree(conf->alpn);
    }

    conf->alpn = NULL;
    conf->alpn_cnt = 0;

    return ;
}

static void http_client_clear_https_conf(httpc_secure_connection_t *conf)
{
    if (conf) {
        if (conf->ca) {
            vPortFree(conf->ca);
        }

        if (conf->cert) {
            vPortFree(conf->cert);
        }

        if (conf->privkey) {
            vPortFree(conf->privkey);
        }

        if (conf->sni) {
            vPortFree(conf->sni);
        }

        http_client_clear_alpn(conf);

        memset(conf, 0x00, sizeof(httpc_secure_connection_t));

        conf->auth_mode = MBEDTLS_SSL_VERIFY_NONE;
        conf->incoming_len = HTTPC_DEF_INCOMING_LEN;
        conf->outgoing_len = HTTPC_DEF_OUTGOING_LEN;
    }

    return ;
}

static int http_client_read_cert(unsigned int addr, unsigned char **out, size_t *outlen)
{
    int ret = 0;
    unsigned char *buf = NULL;
    size_t buflen = CERT_MAX_LENGTH;

    buf = pvPortMalloc(CERT_MAX_LENGTH);
    if (!buf) {
        PRINTF("Failed to allocate memory(%x)\r\n", addr);
        return -1;
    }

    memset(buf, 0x00, CERT_MAX_LENGTH);

    ret = cert_flash_read(addr, buf, CERT_MAX_LENGTH);
    if (ret == 0 && buf[0] != 0xFF) {
        *out = buf;
        *outlen = strlen(buf) + 1;
        return 0;
    }

    if (buf) {
        vPortFree(buf);
    }
    return 0;
}
static void http_client_read_certs(httpc_secure_connection_t *settings)
{
    int ret = 0;

    //to read ca certificate
    ret = http_client_read_cert(SFLASH_ROOT_CA_ADDR2, &settings->ca, &settings->ca_len);
    if (ret) {
        PRINTF("failed to read CA cert\r\n");
        goto err;
    }

    //to read certificate
    ret = http_client_read_cert(SFLASH_CERTIFICATE_ADDR2,
                                &settings->cert, &settings->cert_len);
    if (ret) {
        PRINTF("failed to read certificate\r\n");
        goto err;
    }

    //to read private key
    ret = http_client_read_cert(SFLASH_PRIVATE_KEY_ADDR2,
                                &settings->privkey, &settings->privkey_len);
    if (ret) {
        PRINTF("failed to read private key\r\n");
        goto err;
    }

    return ;

err:

    if (settings->ca) {
        vPortFree(settings->ca);
    }

    if (settings->cert) {
        vPortFree(settings->cert);
    }

    if (settings->privkey) {
        vPortFree(settings->privkey);
    }

    settings->ca = NULL;
    settings->ca_len = 0;
    settings->cert = NULL;
    settings->cert_len = 0;
    settings->privkey = NULL;
    settings->privkey_len = 0;

    return ;
}
#endif //ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

#if TCP_FINISHED_CALLBACK
static void httpc_cb_finish_fn(void)
{
    /* Be careful not to incur DPM SLEEP delays. */
    dpm_app_sleep_ready_set(HTTP_CLIENT_SAMPLE_TASK_NAME);
    return;
}
#endif
static err_t httpc_cb_headers_done_fn(httpc_state_t *connection,
                                      void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{

    DA16X_UNUSED_ARG(connection);
    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(hdr);

    PRINTF("\nhdr_len : %d, content_len : %d\n", hdr_len, content_len);
    return ERR_OK;
}

static void httpc_cb_result_fn(void *arg, httpc_result_t httpc_result,
                               u32_t rx_content_len, u32_t srv_res, err_t err)
{
    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(srv_res);

    PRINTF("\nhttpc_result: %d, received: %d byte, err: %d\n", httpc_result, rx_content_len, err);

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    http_client_clear_https_conf(&g_conn_settings.tls_settings);
#endif // ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    return;
}

static err_t httpc_cb_recv_fn(void *arg, struct tcp_pcb *tpcb,
                              struct pbuf *p, err_t err)
{

    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(tpcb);
    DA16X_UNUSED_ARG(err);

    if (p == NULL) {
        PRINTF("\n[%s:%d] Receive data is NULL !! \r\n", __func__, __LINE__);
        return ERR_BUF;
    } else {
        while (p != NULL) {
            PRINTF("\n[%s:%d] Received length = %d \r\n", __func__, __LINE__, p->len);
            hexa_dump_print((u8 *)"Received data \r\n", p->payload, p->len, 0, OUTPUT_HEXA_ASCII);
            p = p->next;
        }
    }
    return ERR_OK;
}

static void http_client_sample(void *arg)
{
    err_t error = ERR_OK;
#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    unsigned int sni_len = 0;
    char *sni_str;
    int index = 0;
    int alpn_cnt = 0;
#endif// ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

    if (arg == NULL) {
        PRINTF("[%s:%d] Input is NULL !! \r\n", __func__, __LINE__);
        return;
    }
    // Initialize ...
    memset(&g_server_addr, 0, sizeof(ip_addr_t));
    memset(&g_conn_settings, 0, sizeof(httpc_connection_t));
    memcpy(&g_request, (DA16_HTTP_CLIENT_REQUEST *)arg, sizeof(DA16_HTTP_CLIENT_REQUEST));

    g_conn_settings.use_proxy = 0;
    g_conn_settings.altcp_allocator = NULL;
    g_conn_settings.headers_done_fn = httpc_cb_headers_done_fn;
    g_conn_settings.result_fn = httpc_cb_result_fn;
#if TCP_FINISHED_CALLBACK
    g_conn_settings.finish_cb_fn = httpc_cb_finish_fn;
#endif

    g_conn_settings.insecure = g_request.insecure;

#if defined (ENABLE_METHOD_POST_TEST)
    error = httpc_insert_send_data("POST", user_post_data, strlen(user_post_data));
    if (error != ERR_OK) {
        PRINTF("Failed to insert data\n");
    }
#endif

    if (g_conn_settings.insecure) {
        memset(&g_conn_settings.tls_settings, 0x00, sizeof(httpc_secure_connection_t));
        g_conn_settings.tls_settings.incoming_len = HTTPC_MAX_INCOMING_LEN;
        g_conn_settings.tls_settings.outgoing_len = HTTPC_DEF_OUTGOING_LEN;

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
        http_client_read_certs(&g_conn_settings.tls_settings);

        sni_str = read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI);
        if (sni_str != NULL) {
            sni_len = strlen(sni_str);

            if ((sni_len > 0) && (sni_len < HTTPC_MAX_SNI_LEN)) {
                if (g_conn_settings.tls_settings.sni != NULL) {
                    vPortFree(g_conn_settings.tls_settings.sni);
                }
                g_conn_settings.tls_settings.sni = pvPortMalloc(sni_len + 1);
                if (g_conn_settings.tls_settings.sni == NULL) {
                    PRINTF("[%s]Failed to allocate SNI(%ld)\n", __func__, sni_len);
                    goto finish;
                }
                strcpy(g_conn_settings.tls_settings.sni, sni_str);
                g_conn_settings.tls_settings.sni_len = sni_len + 1;
                PRINTF("[%s]ReadNVRAM SNI = %s\n", __func__, g_conn_settings.tls_settings.sni);
            }
        }

        if (read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &alpn_cnt) == 0) {
            if (alpn_cnt > 0) {
                http_client_clear_alpn(&g_conn_settings.tls_settings);

                g_conn_settings.tls_settings.alpn = pvPortMalloc((alpn_cnt + 1) * sizeof(char *));
                if (!g_conn_settings.tls_settings.alpn) {
                    PRINTF("[%s]Failed to allocate ALPN\n", __func__);
                    goto finish;
                }

                for (index = 0 ; index < alpn_cnt ; index++) {
                    char nvrName[15] = {0, };
                    char *alpn_str;

                    if (index >= HTTPC_MAX_ALPN_CNT) {
                        break;
                    }

                    sprintf(nvrName, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, index);
                    alpn_str = read_nvram_string(nvrName);

                    g_conn_settings.tls_settings.alpn[index] = pvPortMalloc(strlen(alpn_str) + 1);
                    if (!g_conn_settings.tls_settings.alpn[index]) {
                        PRINTF("[%s]Failed to allocate ALPN#%d(len=%d)\n", __func__, index + 1, strlen(alpn_str));
                        http_client_clear_alpn(&g_conn_settings.tls_settings);
                        goto finish;
                    }

                    g_conn_settings.tls_settings.alpn_cnt = index + 1;
                    strcpy(g_conn_settings.tls_settings.alpn[index], alpn_str);
                    PRINTF("[%s]ReadNVRAM ALPN#%d = %s\n", __func__,
                           g_conn_settings.tls_settings.alpn_cnt,
                           g_conn_settings.tls_settings.alpn[index]);
                }
                g_conn_settings.tls_settings.alpn[index] = NULL;
            }
        }
#endif //ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    }

    dpm_app_wakeup_done(HTTP_CLIENT_SAMPLE_TASK_NAME);
    dpm_app_data_rcv_ready_set(HTTP_CLIENT_SAMPLE_TASK_NAME);

    if (is_in_valid_ip_class((char *)g_request.hostname)) {
        ip4addr_aton((char *)g_request.hostname, &g_server_addr);
        error = httpc_get_file(&g_server_addr,
                               g_request.port,
                               (char *)&g_request.path[0],
                               &g_conn_settings,
                               (altcp_recv_fn)httpc_cb_recv_fn,
                               NULL,
                               NULL);
    } else {
        error = httpc_get_file_dns((char *)&g_request.hostname[0],
                                   g_request.port,
                                   (char *)&g_request.path[0],
                                   &g_conn_settings,
                                   (altcp_recv_fn)httpc_cb_recv_fn,
                                   NULL,
                                   NULL);
    }

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
finish:
#endif	//ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

    if (error != ERR_OK) {
        PRINTF("Failed to request!! (error=%d)\n", error);
    }

    vTaskDelete(NULL);

    return;

}

void http_client_dpm_sample_entry(void *param)
{
    DA16X_UNUSED_ARG(param);

    err_t error = ERR_OK;
    BaseType_t	xReturned;
    DA16_HTTP_CLIENT_REQUEST request;

    int wait_cnt = 0;
    int iface = WLAN0_IFACE;

    PRINTF("\n\n>>> Start Http Client DPM sample : %s \n\n\n", g_http_url);

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            PRINTF("\r\n [%s] ERR : No network connection\r\n", __func__);
            wait_cnt = 0;
            goto finish;
        }
    }

    if (g_sample_http_client_xHandle) {
        vTaskDelete(g_sample_http_client_xHandle);
        g_sample_http_client_xHandle = NULL;
    }

    error = http_client_parse_uri(g_http_url, strlen((char *)g_http_url), &request);
    if (error != ERR_OK) {
        PRINTF("Failed to set URI(error=%d) \r\n", error);
        goto finish;
    }

    dpm_app_register(HTTP_CLIENT_SAMPLE_TASK_NAME, request.port);
    dpm_app_sleep_ready_clear(HTTP_CLIENT_SAMPLE_TASK_NAME);

    xReturned = xTaskCreate(http_client_sample,
                            HTTP_CLIENT_SAMPLE_TASK_NAME,
                            HTTP_CLIENT_SAMPLE_STACK_SIZE,
                            (void *)&request,
                            tskIDLE_PRIORITY + 1,
                            &g_sample_http_client_xHandle);
    if (xReturned != pdPASS) {
        PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, HTTP_CLIENT_SAMPLE_TASK_NAME);
    }

finish:
    vTaskDelete(NULL);

    return;
}
#endif // (__HTTP_CLIENT_DPM_SAMPLE__)

/* EOF */
