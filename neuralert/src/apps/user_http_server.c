/**
 ****************************************************************************************
 *
 * @file user_http_server.c
 *
 * @brief HTTP Server task
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

#include "FreeRTOSConfig.h"

#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "lwip/apps/httpd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "command_net.h"
#include "common_def.h"
#include "common_config.h"

#include "sdk_type.h"
#include "da16x_system.h"
#include "da16x_network_common.h"

#include "application.h"
#include "iface_defs.h"
#include "nvedit.h"
#include "environ.h"

#include "user_dpm.h"
#include "user_http_server.h"
#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif // (__SUPPORT_ATCMD__)
#include "da16x_sys_watchdog.h"

#define HTTP_SERVER_TASK_NAME            "http_server"
#define HTTPS_SERVER_TASK_NAME           "https_server"
#define HTTP_SERVER_TASK_SIZE            1024


const unsigned char  tls_srv_cert[] =
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
const size_t tls_srv_cert_len = sizeof(tls_srv_cert);

const unsigned char tls_srv_key[] =
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
const size_t tls_srv_key_len = sizeof(tls_srv_key);

struct altcp_tls_config *tls_srv_config = NULL;
TaskHandle_t g_http_server_xHandle = NULL;
TaskHandle_t g_https_server_xHandle = NULL;

err_t http_server_parse_cmd(char *argv[], int argc, DA16X_HTTP_SERVER_CMD *cmd);

static err_t http_server_cb_recv_fn(struct pbuf *p, err_t err)
{
    DA16X_UNUSED_ARG(err);
    err_t error = ERR_OK;
#if defined (__SUPPORT_ATCMD__)
    char *buffer;
    unsigned int  buffer_len;
    char prefix[20];
    char *pbuffer;
#endif /* __SUPPORT_ATCMD__ */

    while (p != NULL) {
        if ((p->payload != NULL) && (p->len > 0)) {
            PRINTF("Received tot_len = %d, len = %d \n", p->tot_len, p->len);
#if defined (__SUPPORT_ATCMD__)
            memset(prefix, 0, sizeof(prefix));
            sprintf(prefix, "+NWHTSDATA:%d,", p->len);
            buffer_len = strlen(prefix);
            buffer_len += p->len;
            buffer = ATCMD_MALLOC(buffer_len);
            if (buffer != NULL) {

                memset(buffer, 0, buffer_len);
                pbuffer = buffer;

                memcpy(pbuffer, prefix, strlen(prefix));
                pbuffer += strlen(prefix);

                memcpy(pbuffer, p->payload, p->len);
                PUTS_ATCMD(buffer, (int)buffer_len);

                buffer_len = 0;
                ATCMD_FREE(buffer);
            } else {
                PRINTF("\nReceive buffer allocation failed !! \n");
                error = ERR_BUF;
                break;
            }
#endif /* __SUPPORT_ATCMD__ */
        } else {
            PRINTF("\nReceive data is NULL !! \n");
            error = ERR_BUF;
            break;
        }
        p = p->next;
    }
    return error;
}

void http_server_task(void *params)
{
    int sys_wdog_id = -1;

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    httpd_init((altcp_user_recv_fn)http_server_cb_recv_fn);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_http_server_xHandle = NULL;
    vTaskDelete(NULL);

    return;
}

void https_server_task(void *params)
{
    int sys_wdog_id = -1;

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    tls_srv_config = altcp_tls_create_config_server_privkey_cert(tls_srv_key,
                                                                   tls_srv_key_len,
                                                                   NULL,
                                                                   0,
                                                                   tls_srv_cert,
                                                                   tls_srv_cert_len);
    if (!tls_srv_config) {
        PRINTF("[%s] Failed to create tls config\n", __func__);
        PRINTF("[%s] Server initialization failed\n", __func__);

        goto end_of_task;
    }

    httpd_inits(tls_srv_config, (altcp_user_recv_fn)http_server_cb_recv_fn);

end_of_task:

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_https_server_xHandle = NULL;
    vTaskDelete(NULL);

    return;
}

err_t run_user_http_server(char *argv[], int argc)
{
    err_t error = ERR_OK;
    BaseType_t    xReturned;
    DA16X_HTTP_SERVER_CMD cmd;

    if (argc <= 1) {
        PRINTF("[%s] Invailed argument. (argc = %d) \n", __func__, argc);
        return ERR_ARG;
    }

    http_server_parse_cmd(argv, argc, &cmd);

    if (cmd.op_code == HTTP_SERVER_OPCODE_START) {
        if (g_http_server_xHandle) {
            vTaskDelete(g_http_server_xHandle);
            g_http_server_xHandle = NULL;
        }

        xReturned = xTaskCreate(http_server_task,
                                  HTTP_SERVER_TASK_NAME,
                                  HTTP_SERVER_TASK_SIZE,
                                  (void *)NULL,
                                  OS_TASK_PRIORITY_USER,
                                  &g_http_server_xHandle);

        if (xReturned != pdPASS) {
            PRINTF("[%s] Failed to create task(%s)\n", __func__, HTTP_SERVER_TASK_NAME);
        } else {
            PRINTF("[%s] HTTP-Server Start!! \n", __func__);
        }
    } else if (cmd.op_code == HTTP_SERVER_OPCODE_STOP) {
        PRINTF("[%s] HTTP-Server Stop!! \n", __func__);

        http_server_stop();

        if (g_http_server_xHandle) {
            vTaskDelete(g_http_server_xHandle);
            g_http_server_xHandle = NULL;
        }
    } else {
        PRINTF("[%s] Invailed argument\n", __func__);
    }

    return error;
}

err_t run_user_https_server(char *argv[], int argc)
{
    err_t error = ERR_OK;
    BaseType_t    xReturned;
    DA16X_HTTP_SERVER_CMD cmd;

    if (argc <= 1) {
        PRINTF("[%s] Invailed argument\n", __func__);
        return ERR_ARG;
    }

    http_server_parse_cmd(argv, argc, &cmd);

    if (cmd.op_code == HTTP_SERVER_OPCODE_START) {

        if (g_https_server_xHandle) {
            vTaskDelete(g_https_server_xHandle);
            g_https_server_xHandle = NULL;
        }

        xReturned = xTaskCreate(https_server_task,
                                  HTTPS_SERVER_TASK_NAME,
                                  HTTP_SERVER_TASK_SIZE,
                                  (void *)NULL,
                                  OS_TASK_PRIORITY_USER,
                                  &g_https_server_xHandle);

        if (xReturned != pdPASS) {
            PRINTF("[%s] Failed to create task(%s)\n", __func__, HTTPS_SERVER_TASK_NAME);
        } else {
            PRINTF("[%s] HTTPS-Server Start!! \n", __func__);
        }
    } else if (cmd.op_code == HTTP_SERVER_OPCODE_STOP) {
        PRINTF("[%s] HTTPS-Server Stop!! \n", __func__);

        http_server_stop();

        if (g_https_server_xHandle) {
            vTaskDelete(g_https_server_xHandle);
            g_https_server_xHandle = NULL;
        }
    } else {
        PRINTF("[%s] Invailed argument\n", __func__);
    }

    return error;
}

void auto_run_http_svr(void *pvParameters)
{
    int sys_wdog_id = -1;
    char *_cmd[4];
    int result_int;

    DA16X_UNUSED_ARG(pvParameters);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    _cmd[1] = "-i";
    da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
    _cmd[2] = result_int == SYSMODE_STA_ONLY ? "wlan0" : "wlan1";
    _cmd[3] = "start";

    da16x_sys_watchdog_notify(sys_wdog_id);

    run_user_http_server(_cmd, 4);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;
}

void auto_run_https_svr(void *pvParameters)
{
    int sys_wdog_id = -1;
    char *_cmd[4];
    int result_int;

    DA16X_UNUSED_ARG(pvParameters);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    _cmd[1] = "-i";
    da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
    _cmd[2] = result_int == SYSMODE_STA_ONLY ? "wlan0" : "wlan1";
    _cmd[3] = "start";

    da16x_sys_watchdog_notify(sys_wdog_id);

    run_user_https_server(_cmd, 4);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;
}

err_t http_server_parse_cmd(char *argv[], int argc, DA16X_HTTP_SERVER_CMD *cmd)
{
    int index = 0;
    char **cur_argv = ++argv;

    DA16_HTTP_SERVER_OPCODE    op_code = HTTP_SERVER_OPCODE_READY;
    int iface = WLAN0_IFACE;

    if (argc < 1) {
        PRINTF("Invaild parameters\n");
        return ERR_ARG;
    }

    for (index = 1 ; index < argc ; index++, cur_argv++) {
        if (cur_argv[0][0] == '-') {
            //parse options
            if (cur_argv[0][1] == 'i') {
                if (--argc < 1) {
                    PRINTF("Invaild interface\n");
                    return ERR_ARG;
                }

                ++cur_argv;

                if (strcasecmp("WLAN0", *cur_argv) == 0) {
                    iface = WLAN0_IFACE;
                } else if (strcasecmp("WLAN1", *cur_argv) == 0) {
                    iface = WLAN1_IFACE;
                } else {
                    PRINTF("failed to set interface(%s)\n", *cur_argv);
                    return ERR_ARG;
                }
            } else {
                PRINTF("Invaild option(%s)\n", *cur_argv);
                return ERR_ARG;
            }
        } else {
            //parse operation
            if (strcmp("start", *cur_argv) == 0) {
                if (op_code != HTTP_SERVER_OPCODE_READY) {
                    PRINTF("Invaild operation(not ready)\n");
                    return ERR_ARG;
                }
                op_code = HTTP_SERVER_OPCODE_START;

            } else if (strcmp("stop", *cur_argv) == 0) {
                op_code = HTTP_SERVER_OPCODE_STOP;

            } else {
                PRINTF("Invaild paramters(%s)\n", *cur_argv);
                return ERR_ARG;
            }
        }
    }

    cmd->op_code = op_code;
    cmd->iface = iface;

    return ERR_OK;
}

/* EOF */
