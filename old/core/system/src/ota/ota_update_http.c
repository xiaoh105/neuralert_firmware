/**
 ****************************************************************************************
 *
 * @file ota_update_http.c
 *
 * @brief Over the air firmware update by http protocol
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

#if defined(__SUPPORT_OTA__)
#include <ctype.h>
#include "FreeRTOSConfig.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "mbedtls/ssl.h"
#include "util_api.h"
#include "command_net.h"
#include "da16x_network_common.h"
#include "da16x_dns_client.h"
#include "da16x_cert.h"
#include "ota_update_common.h"
#include "ota_update_http.h"
#include "ota_update_mcu_fw.h"
#include "ota_update_ble_fw.h"
#include "user_nvram_cmd_table.h"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"


static ota_update_download_t dw_info;
static ip_addr_t server_addr;
static httpc_connection_t ota_conn_settings = {0, };
static DA16_HTTP_CLIENT_REQUEST http_request;
#if defined (__BLE_COMBO_REF__)
extern unsigned int new_img_position;
#endif // __BLE_COMBO_REF__

static void http_client_clear_alpn(httpc_secure_connection_t *conf)
{
    int idx = 0;

    if (conf->alpn)	{
        for (idx = 0 ; idx < conf->alpn_cnt ; idx++) {
            if (conf->alpn[idx]) {
                OTA_FREE(conf->alpn[idx]);
                conf->alpn[idx] = NULL;
            }
        }
        OTA_FREE(conf->alpn);
    }

    conf->alpn = NULL;
    conf->alpn_cnt = 0;

    return ;
}
static void http_client_clear_https_conf(httpc_secure_connection_t *conf)
{
    int auth_mode = 0;

    if (conf) {
        if (conf->ca) {
            OTA_FREE(conf->ca);
            conf->ca = NULL;
        }
        conf->ca_len = 0;

        if (conf->cert)	{
            OTA_FREE(conf->cert);
            conf->cert = NULL;
        }
        conf->cert_len = 0;

        if (conf->privkey) {
            OTA_FREE(conf->privkey);
            conf->privkey = NULL;
        }
        conf->privkey_len = 0;

        if (conf->dh_param) {
            OTA_FREE(conf->dh_param);
            conf->dh_param = NULL;
        }
        conf->dh_param_len = 0;

        if (conf->sni) {
            OTA_FREE(conf->sni);
            conf->sni = NULL;
        }
        conf->sni_len = 0;

        http_client_clear_alpn(conf);

        memset(conf, 0x00, sizeof(httpc_secure_connection_t));

        if (da16x_get_config_int(DA16X_CONF_INT_OTA_TLS_AUTHMODE, &auth_mode) != 0) {
            auth_mode = OTA_HTTP_TLS_AUTHMODE_DEF;
        }
        conf->auth_mode = auth_mode;
        conf->incoming_len = HTTPC_DEF_INCOMING_LEN;
        conf->outgoing_len = HTTPC_DEF_OUTGOING_LEN;
    }

    return ;
}

static int http_client_read_cert(int module, int type, unsigned char **out, size_t *outlen)
{
    int ret = 0;
    unsigned char *buf = NULL;
    size_t buflen = CERT_MAX_LENGTH;
    int format = DA16X_CERT_FORMAT_NONE;

    buf = OTA_MALLOC(buflen);
    if (!buf) {
        OTA_ERR("Failed to allocate memory(module:%d, type:%d, len:%lu)\r\n", module, type, buflen);
        return OTA_MEM_ALLOC_FAILED;
    }

    memset(buf, 0x00, buflen);

    ret = da16x_cert_read(module, type, &format, buf, &buflen);
    if (ret == DA16X_CERT_ERR_OK) {
        *out = buf;
        *outlen = buflen;
        return OTA_SUCCESS;
    } else if (ret == DA16X_CERT_ERR_EMPTY_CERTIFICATE) {
        if (buf) {
            OTA_FREE(buf);
            buf = NULL;
        }
        return OTA_SUCCESS;
    }

    if (buf) {
        OTA_FREE(buf);
        buf = NULL;
    }

    return OTA_FAILED;
}


static void http_client_read_certs(httpc_secure_connection_t *settings)
{
    int ret = 0;

    /* to read ca certificate */
    ret = http_client_read_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_CA_CERT, &settings->ca, &settings->ca_len);
    if (ret) {
        OTA_ERR("failed to read CA cert\r\n");
        goto err;
    }

    /* to read certificate */
    ret = http_client_read_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_CERT, &settings->cert, &settings->cert_len);
    if (ret) {
        OTA_ERR("failed to read certificate\r\n");
        goto err;
    }

    /* to read private key */
    ret = http_client_read_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_PRIVATE_KEY, &settings->privkey, &settings->privkey_len);
    if (ret) {
        OTA_ERR("failed to read private key\r\n");
        goto err;
    }

    //to read dh param
    ret = http_client_read_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_DH_PARAMS, &settings->dh_param, &settings->dh_param_len);
    if (ret) {
        OTA_ERR("failed to read dh param\r\n");
        goto err;
    }

    return ;

err:

    if (settings->ca) {
        OTA_FREE(settings->ca);
    }

    if (settings->cert) {
        OTA_FREE(settings->cert);
    }

    if (settings->privkey) {
        OTA_FREE(settings->privkey);
    }

    if (settings->dh_param) {
        OTA_FREE(settings->dh_param);
    }

    settings->ca = NULL;
    settings->ca_len = 0;
    settings->cert = NULL;
    settings->cert_len = 0;
    settings->privkey = NULL;
    settings->privkey_len = 0;
    settings->dh_param = NULL;
    settings->dh_param_len = 0;

    return ;
}

UINT ota_update_httpc_set_tls_auth_mode_nvram(int tls_auth_mode)
{
    if (da16x_set_config_int(DA16X_CONF_INT_OTA_TLS_AUTHMODE, tls_auth_mode) != 0) {
        OTA_ERR("\n[%s] Failed to set TLS auth_mode\n", __func__);
        return OTA_FAILED;
    }
    OTA_INFO("[%s]WriteNVRAM tls_auth_mode = %d\n", __func__, tls_auth_mode);
    return OTA_SUCCESS;
}

err_t ota_update_httpc_cb_headers_done_fn(
    httpc_state_t *connection, void *arg, struct pbuf *hdr, 	u16_t hdr_len, u32_t content_len)
{

    DA16X_UNUSED_ARG(connection);
    DA16X_UNUSED_ARG(arg);

    OTA_DBG("\n[%s:%d] hdr_len : %d, content_len : %d\n%s\n", __func__, __LINE__,
            hdr_len, content_len, hdr->payload);

    if ((hdr == NULL) || (hdr_len <= 0)) {
        return ERR_UNKNOWN;
    }

    if (content_len == HTTPC_CONTENT_LEN_INVALID) {
        return ERR_NOT_FOUND;
    }

    if (strstr(hdr->payload, "200 OK") == NULL) {
        return ERR_NOT_FOUND;
    }

    dw_info.content_length = content_len;
    dw_info.write.total_length = content_len;

    return ERR_OK;
}

void ota_update_httpc_cb_result_fn(void *arg,
                                   httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    UINT sflash_addr;

    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(rx_content_len);
    DA16X_UNUSED_ARG(srv_res);

    OTA_DBG("[%s]httpc_result = %d\n", __func__, httpc_result);
    if ((httpc_result != HTTPC_RESULT_OK) && (httpc_result != HTTPC_RESULT_LOCAL_ABORT)) {
        OTA_ERR("\n[%s] Failed to get response(%d)\n", __func__, httpc_result);
    }

    if (httpc_result == HTTPC_RESULT_OK) {
        if (dw_info.update_type == OTA_TYPE_RTOS) 	{
            sflash_addr = ota_update_get_new_sflash_addr(dw_info.update_type);
            if ((sflash_addr != SFLASH_UNKNOWN_ADDRESS)
                    && (ota_update_sflash_crc(sflash_addr) > 0)) {
                dw_info.download_status = OTA_SUCCESS;
            } else {
                dw_info.download_status = OTA_ERROR_CRC;
            }
        }
#if defined (__BLE_COMBO_REF__)
        else if ( dw_info.update_type == OTA_TYPE_BLE_FW) {
            dw_info.download_status = ota_update_ble_crc();
        }
#endif //(__BLE_COMBO_REF__)

    } else if (httpc_result == HTTPC_RESULT_ERR_UNKNOWN) {
        dw_info.download_status = OTA_FAILED;
    } else if (httpc_result == HTTPC_RESULT_ERR_CONNECT) {
        dw_info.download_status = OTA_NOT_CONNECTED;
    } else if (httpc_result == HTTPC_RESULT_ERR_HOSTNAME) {
        dw_info.download_status = OTA_ERROR_URL;
    } else if (httpc_result == HTTPC_RESULT_ERR_CLOSED) {
        dw_info.download_status = OTA_NOT_CONNECTED;
    } else if (httpc_result == HTTPC_RESULT_ERR_TIMEOUT) {
        if (ota_update_get_download_progress(dw_info.update_type)) {
            dw_info.download_status = OTA_FAILED;
        } else if (dw_info.content_length == 0) {
            dw_info.download_status = OTA_NOT_CONNECTED;
        } else {
            dw_info.download_status = OTA_NOT_FOUND;
        }
    } else if (httpc_result == HTTPC_RESULT_ERR_SVR_RESP) {
        dw_info.download_status = OTA_NOT_FOUND;
    } else if (httpc_result == HTTPC_RESULT_ERR_MEM) {
        dw_info.download_status = OTA_MEM_ALLOC_FAILED;
    } else if (httpc_result == HTTPC_RESULT_LOCAL_ABORT) {
        if (err == ERR_NOT_FOUND) {
            dw_info.download_status = OTA_NOT_FOUND;
        } else {
            if (dw_info.version_check != OTA_SUCCESS) {
                dw_info.download_status = OTA_VERSION_UNKNOWN;
            } else {
                dw_info.download_status = OTA_FAILED;
            }
        }
    } else if (httpc_result == HTTPC_RESULT_ERR_CONTENT_LEN) {
        dw_info.download_status = OTA_ERROR_SIZE;
    } else {
        dw_info.download_status = OTA_FAILED;
    }

#if defined (__OTA_UPDATE_MCU_FW__)
    if (dw_info.update_type == OTA_TYPE_MCU_FW) {
        if (dw_info.download_status == OTA_SUCCESS)	{
            /* MCU FW header generation */
            ota_update_gen_mcu_header(dw_info.content_length);
        } else {
            /* In case of failure, the downloaded MCU FW is deleted */
            ota_update_erase_mcu_fw();
        }
    }
#endif // (__OTA_UPDATE_MCU_FW__)

    dw_info.version_check = OTA_NOT_FOUND;

    if (dw_info.download_status == OTA_SUCCESS) {
        /* Replace OTA_SUCCESS(0x00) with OTA_SUCCESS_EVT(0xff) because it does not trigger an event */
        ota_update_send_status(OTA_SUCCESS_EVT); /* OTA_SUCCESS */
    } else {
        ota_update_send_status(dw_info.download_status);
    }

    return;
}

static err_t ota_update_httpc_cb_recv_fn(void *arg,
        struct altcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    UINT status;
    UINT progress = 0;
    UCHAR *p_payload = NULL;
    UINT p_len = 0;
#if defined (__BLE_COMBO_REF__)
    UINT offset_t = 0;
#endif // __BLE_COMBO_REF__

    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(tpcb);

    if (p == NULL) {
        OTA_ERR("[%s] pbuf is null\n", __func__);
        return ERR_BUF;
    } else if (err != ERR_OK) {
        OTA_ERR("[%s] Received failed(err=%d)\n", __func__, err);
        return err;
    } else {

        if (ota_update_get_proc_state() != OTA_STATE_PROGRESS) {
            return ERR_VAL;
        }

#if defined (DISABLE_OTA_VER_CHK)
        dw_info.version_check = OTA_SUCCESS;
#else
        if (dw_info.version_check == OTA_NOT_FOUND) {

            if (ota_update_check_version(dw_info.update_type,
                                         (UCHAR *)p->payload, (UINT)p->len) == OTA_SUCCESS) {
                dw_info.version_check = OTA_SUCCESS;

#if defined (__BLE_COMBO_REF__)
                if (dw_info.update_type == OTA_TYPE_BLE_FW) {
                    dw_info.write.sflash_addr = new_img_position;
                }
#endif // __BLE_COMBO_REF__
            } else {
                /* Version mismatch */
                dw_info.version_check = OTA_VERSION_INCOMPATI;
                return ERR_VAL;
            }

            if (dw_info.version_check == OTA_SUCCESS) {
                if (ota_update_check_accept_size(dw_info.update_type,
                                                 dw_info.content_length) != OTA_SUCCESS) {
                    dw_info.version_check = OTA_VERSION_INCOMPATI;
                    return ERR_VAL;
                }
            }
        }
#endif // (DISABLE_OTA_VER_CHK)

        while (p != NULL) {

            p_payload = (UCHAR *)p->payload;
            p_len = p->len;
            dw_info.received_length += p_len;
#if defined (__BLE_COMBO_REF__)
            if (dw_info.update_type == OTA_TYPE_BLE_FW) {
                ota_update_ble_crc_calcu(p_payload, p_len);
                offset_t = ota_update_ble_first_packet_offset();
                p_payload += offset_t;
                p_len -= offset_t;
                dw_info.write.total_length -= offset_t;
            }
#endif //(__BLE_COMBO_REF__)
            status = ota_update_write_sflash(&dw_info.write, p_payload, p_len);
            if (status != OTA_SUCCESS) {
                OTA_ERR("[%s] Failed to write data to sflash(0x%02x)\n", __func__, status);
                return ERR_VAL;
            }

            if ((dw_info.received_length > 0) && (dw_info.content_length > 0)) {
                progress = (dw_info.received_length * 100) / dw_info.content_length;

                OTA_INFO("\r   >> HTTP(s) Client Downloading... %d % (%d/%d Bytes)%s",
                         progress,
                         dw_info.received_length,
                         dw_info.content_length, progress == 100 ? "\n" : " ");
            } else {
                progress = 0;
            }

            p = p->next;

            /* Don't remove until lwIP optimize ... */
            vTaskDelay( 10 / portTICK_PERIOD_MS );
        }

        if (dw_info.version_check == OTA_SUCCESS) {
            ota_update_set_download_progress(dw_info.update_type, progress);
        }
    }

    return ERR_OK;
}

UINT ota_update_http_client_request(ota_update_proc_t *ota_proc)
{
    UINT status = OTA_SUCCESS;
    err_t error = ERR_OK;
    unsigned int sni_len = 0;
    char *sni_str;
    int index = 0;
    int alpn_cnt = 0;

    if (ota_proc == NULL) {
        OTA_ERR("[%s] Parameter is null\n", __func__);
        return OTA_FAILED;
    }

    memset(&server_addr, 0, sizeof(ip_addr_t));
    http_client_clear_https_conf(&(ota_conn_settings.tls_settings));
    memset(&ota_conn_settings, 0, sizeof(httpc_connection_t));
    memset(&http_request, 0, sizeof(DA16_HTTP_CLIENT_REQUEST));
#if defined (__BLE_COMBO_REF__)
    if (ota_proc->update_type == OTA_TYPE_BLE_FW) {
        if ((ota_proc->url_ble_fw == NULL)
                || (strlen(ota_proc->url_ble_fw) > OTA_HTTP_URL_LEN)) {
            OTA_ERR("[%s] Invalid url_ble_fw\n", __func__);
            return OTA_ERROR_URL;
        }
        OTA_DBG("[%s]parse url_ble_fw(len=%d) = %s\n", __func__, strlen(ota_proc->url_ble_fw), ota_proc->url_ble_fw);
        error = http_client_parse_uri((unsigned char *)ota_proc->url_ble_fw,
                                      strlen(ota_proc->url_ble_fw), &http_request);

    } else {

        if ((ota_proc->url == NULL)
                || (strlen(ota_proc->url) > OTA_HTTP_URL_LEN)) {
            OTA_ERR("[%s] Invalid url\n", __func__);
            return OTA_ERROR_URL;
        }
        OTA_DBG("[%s]parse url(len=%d) = %s\n", __func__, strlen(ota_proc->url), ota_proc->url);
        error = http_client_parse_uri((unsigned char *)ota_proc->url,
                                      strlen(ota_proc->url), &http_request);
    }
#else
    if ((ota_proc->url == NULL)
            || (strlen(ota_proc->url) > OTA_HTTP_URL_LEN)) {
        OTA_ERR("[%s] Invalid url\n", __func__);
        return OTA_ERROR_URL;
    }
    OTA_DBG("[%s]parse url(len=%d) = %s\n", __func__, strlen(ota_proc->url), ota_proc->url);
    error = http_client_parse_uri((unsigned char *)ota_proc->url,
                                  strlen(ota_proc->url), &http_request);
#endif // (__BLE_COMBO_REF__)
    if (error != ERR_OK) {
        OTA_ERR("[%s]Failed to parse url(err=%d)\n", __func__, error);
        return OTA_ERROR_URL;
    }

    /****************************************************/
    /* Initialize ota_update download information */
    /****************************************************/
    dw_info.update_type = ota_proc->update_type;
    dw_info.received_length = 0;
    dw_info.write.total_length = 0;
    dw_info.write.length = 0;
    dw_info.write.offset = 0;
    dw_info.download_status = OTA_SUCCESS;
    dw_info.version_check = OTA_NOT_FOUND;
    dw_info.content_length = 0;
    dw_info.received_length = 0;
    if (dw_info.update_type == OTA_TYPE_MCU_FW)	{
        dw_info.write.sflash_addr = ota_update_get_new_sflash_addr(dw_info.update_type) + OTA_MCU_FW_HEADER_SIZE;
    } else	{
        dw_info.write.sflash_addr = ota_update_get_new_sflash_addr(dw_info.update_type);
    }

    /******************************************/
    /* Initialize http connection settings */
    /******************************************/
    ota_conn_settings.use_proxy = 0;
    ota_conn_settings.altcp_allocator = NULL;
    ota_conn_settings.headers_done_fn = ota_update_httpc_cb_headers_done_fn;
    ota_conn_settings.result_fn = ota_update_httpc_cb_result_fn;
    ota_conn_settings.insecure = http_request.insecure;

    if (ota_conn_settings.insecure == pdTRUE) {
        http_client_clear_https_conf(&ota_conn_settings.tls_settings);
        ota_conn_settings.tls_settings.incoming_len = HTTPC_MAX_INCOMING_LEN;
        ota_conn_settings.tls_settings.outgoing_len = HTTPC_DEF_OUTGOING_LEN;

        http_client_read_certs(&ota_conn_settings.tls_settings);
        sni_str = read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI);
        if (sni_str != NULL) {
            sni_len = strlen(sni_str);

            if ((sni_len > 0) && (sni_len < HTTPC_MAX_SNI_LEN)) {
                if (ota_conn_settings.tls_settings.sni != NULL)	{
                    OTA_FREE(ota_conn_settings.tls_settings.sni);
                }

                ota_conn_settings.tls_settings.sni = OTA_MALLOC(sni_len + 1);
                if (ota_conn_settings.tls_settings.sni == NULL)	{
                    OTA_ERR("[%s]Failed to allocate SNI(%ld)\n", __func__, sni_len);
                    return OTA_MEM_ALLOC_FAILED;
                }
                strcpy(ota_conn_settings.tls_settings.sni, sni_str);
                ota_conn_settings.tls_settings.sni_len = (int)(sni_len + 1);
                OTA_INFO("[%s]ReadNVRAM SNI = %s\n", __func__, ota_conn_settings.tls_settings.sni);
            }
        }

        if (read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &alpn_cnt) == 0) {
            if (alpn_cnt > 0) {
                http_client_clear_alpn(&ota_conn_settings.tls_settings);

                ota_conn_settings.tls_settings.alpn = OTA_MALLOC((alpn_cnt + 1) * sizeof(char *));
                if (!ota_conn_settings.tls_settings.alpn) {
                    OTA_ERR("[%s]Failed to allocate ALPN\n", __func__);
                    return OTA_MEM_ALLOC_FAILED;
                }

                for (index = 0 ; index < alpn_cnt ; index++) {
                    char nvrName[15] = {0, };
                    char *alpn_str;

                    if (index >= HTTPC_MAX_ALPN_CNT) {
                        break;
                    }

                    sprintf(nvrName, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, index);
                    alpn_str = read_nvram_string(nvrName);

                    ota_conn_settings.tls_settings.alpn[index] = OTA_MALLOC(strlen(alpn_str) + 1);
                    if (!ota_conn_settings.tls_settings.alpn[index]) {
                        OTA_ERR("[%s]Failed to allocate ALPN#%d(len=%d)\n", __func__, index + 1, strlen(alpn_str));
                        http_client_clear_alpn(&ota_conn_settings.tls_settings);
                        return OTA_MEM_ALLOC_FAILED;
                    }

                    ota_conn_settings.tls_settings.alpn_cnt = index + 1;
                    strcpy(ota_conn_settings.tls_settings.alpn[index], alpn_str);
                    OTA_INFO("[%s]ReadNVRAM ALPN#%d = %s\n", __func__,
                             ota_conn_settings.tls_settings.alpn_cnt,
                             ota_conn_settings.tls_settings.alpn[index]);
                }
                ota_conn_settings.tls_settings.alpn[index] = NULL;
            }
        }

    }

    da16x_sys_watchdog_notify(da16x_sys_wdog_id_get_OTA_update());

    da16x_sys_watchdog_suspend(da16x_sys_wdog_id_get_OTA_update());

    if (is_in_valid_ip_class((char *)http_request.hostname)) {
        ip4addr_aton((const char *)(http_request.hostname), &server_addr);
#if defined (__BLE_COMBO_REF__)
        if (ota_proc->update_type == OTA_TYPE_BLE_FW) {
            error = httpc_get_file(&server_addr,
                                   (u16_t)(http_request.port),
                                   ota_proc->url_ble_fw,
                                   &ota_conn_settings,
                                   ota_update_httpc_cb_recv_fn,
                                   NULL,
                                   NULL);
        } else {
            error = httpc_get_file(&server_addr,
                                   (u16_t)(http_request.port),
                                   ota_proc->url,
                                   &ota_conn_settings,
                                   ota_update_httpc_cb_recv_fn,
                                   NULL,
                                   NULL);
        }
#else
        error = httpc_get_file(&server_addr,
                               (u16_t)(http_request.port),
                               ota_proc->url,
                               &ota_conn_settings,
                               ota_update_httpc_cb_recv_fn,
                               NULL,
                               NULL);
#endif //(__BLE_COMBO_REF__)
    } else {
        error = httpc_get_file_dns((const char *) & (http_request.hostname[0]),
                                   (u16_t)(http_request.port),
                                   (const char *) & (http_request.path[0]),
                                   &ota_conn_settings,
                                   ota_update_httpc_cb_recv_fn,
                                   NULL,
                                   NULL);
    }

    da16x_sys_watchdog_notify_and_resume(da16x_sys_wdog_id_get_OTA_update());

    if (error != ERR_OK) {
        OTA_ERR("[%s]http-client error = %d\n", __func__, error);
        status = OTA_FAILED;
    }

    return status;
}
#endif //( __SUPPORT_OTA__)

/* EOF */
