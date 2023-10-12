/**
 ****************************************************************************************
 *
 * @file http_server_sample.c
 *
 * @brief Sample app of HTTP server.
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


#if defined (__HTTP_SERVER_SAMPLE__)

#include "sdk_type.h"
#include "sample_defs.h"
#include <ctype.h>
#include "da16x_system.h"
#include "application.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_network_common.h"
#include "util_api.h"
#include "da16x_network_main.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "lwip/apps/httpd.h"

#define HTTP_SERVER_SAMPLE_TASK_NAME	"http_server"
#define HTTPS_SERVER_SAMPLE_TASK_NAME	"https_server"
#define	HTTP_SERVER_SAMPLE_STACK_SIZE	(1024 * 4) / 4 //WORD

TaskHandle_t g_sample_http_server_xHandle = NULL;

static err_t http_server_cb_recv_fn(struct pbuf *p, err_t err)
{
    err_t error = ERR_OK;
    extern void hex_dump(UCHAR * data, UINT length);

    while (p != NULL) {
        if ((p->payload != NULL) && (p->len > 0)) {
            PRINTF("Received tot_len = %d, len = %d, err = %d \n", p->tot_len, p->len, err);
            hex_dump(p->payload, p->len);
        } else {
            PRINTF("\nReceive data is NULL !! \n");
            error = ERR_BUF;
            break;
        }
        p = p->next;
    }
    return error;
}


/////////////////////////////////////////////////////////////////////////
/// HTTPS Server
/// NOTICE - The sample code's certificate is just a self-signed certificate for testing.
/// Be sure to use a certificate verified by an official certification authority.
/////////////////////////////////////////////////////////////////////////
#undef ENABLE_HTTPS_SERVER
#if defined (ENABLE_HTTPS_SERVER)
const unsigned char  tls_srv_sample_cert[] =
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
const size_t tls_srv_sample_cert_len = sizeof(tls_srv_sample_cert);

const unsigned char tls_srv_sample_key[] =
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
const size_t tls_srv_sample_key_len = sizeof(tls_srv_sample_key);

struct altcp_tls_config *tls_srv_sample_config = NULL;
static void https_server_sample(void *params)
{
    DA16X_UNUSED_ARG(params);

    tls_srv_sample_config = altcp_tls_create_config_server_privkey_cert(tls_srv_sample_key,
                            tls_srv_sample_key_len,
                            NULL,
                            0,
                            tls_srv_sample_cert,
                            tls_srv_sample_cert_len);
    if (!tls_srv_sample_config) {
        PRINTF("[%s] Failed to create tls config\r\n", __func__);

        goto end_of_task;
    }

    httpd_inits(tls_srv_sample_config, (altcp_user_recv_fn)http_server_cb_recv_fn);

    PRINTF("[%s] HTTPS-Server Start!! \r\n", __func__);

end_of_task:

    while (1) {
        vTaskDelay(100);
    }

    return ;
}
#else

/////////////////////////////////////////////////////////////////////////
/// HTTP Server
/////////////////////////////////////////////////////////////////////////
static void http_server_sample(void *params)
{
    DA16X_UNUSED_ARG(params);

    httpd_init((altcp_user_recv_fn)http_server_cb_recv_fn);

    PRINTF("[%s] HTTP-Server Start!! \r\n", __func__);

    while (1) {
        vTaskDelay(100);
    }

    return ;
}

#endif //(ENABLE_HTTPS_SERVER)

void http_server_sample_entry(void *param)
{
    DA16X_UNUSED_ARG(param);

    BaseType_t	xReturned;

    PRINTF("\n\n>>> Start HTTP-Server sample\n");

    if (g_sample_http_server_xHandle) {
        vTaskDelete(g_sample_http_server_xHandle);
        g_sample_http_server_xHandle = NULL;
    }

#if !defined (ENABLE_HTTPS_SERVER)
    xReturned = xTaskCreate(http_server_sample,
                            HTTP_SERVER_SAMPLE_TASK_NAME,
                            HTTP_SERVER_SAMPLE_STACK_SIZE,
                            (void *)NULL,
                            tskIDLE_PRIORITY + 1,
                            &g_sample_http_server_xHandle);

#else
    xReturned = xTaskCreate(https_server_sample,
                            HTTPS_SERVER_SAMPLE_TASK_NAME,
                            HTTP_SERVER_SAMPLE_STACK_SIZE,
                            (void *)NULL,
                            tskIDLE_PRIORITY + 1,
                            &g_sample_http_server_xHandle);
#endif // (ENABLE_HTTPS_SERVER)

    if (xReturned != pdPASS) {
        PRINTF("[%s] Failed to create task(%s)\r\n",
               __func__, HTTP_SERVER_SAMPLE_TASK_NAME);
    }

    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }

}

#endif // (__HTTP_SERVER_SAMPLE__)

/* EOF */
