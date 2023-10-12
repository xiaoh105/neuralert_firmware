/**
 ****************************************************************************************
 *
 * @file coap_client_sample.c
 *
 * @brief Sample app of CoAP client.
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

#ifndef COAP_CLIENT_SAMPLE_C
#define COAP_CLIENT_SAMPLE_C

#include "sdk_type.h"
#include "da16x_system.h"
#include "coap_client.h"
#include "sample_defs.h"

#if defined (__COAP_CLIENT_SAMPLE__)

#define COAP_CLIENT_SAMPLE_ENABLED_HEXDUMP
#define COAP_CLIENT_SAMPLE_DEF_TASK_NAME        "coap_cli_task"
#define COAP_CLIENT_SAMPLE_DEF_TASK_SIZE        (128 * 10)
#define COAP_CLIENT_SAMPLE_DEF_QUEUE_SIZE       2

#define COAP_CLIENT_SAMPLE_REQUEST_PORT         COAP_CLI_REQ_TEST_PORT
#define COAP_CLIENT_SAMPLE_OBSERVE_PORT         COAP_CLI_OBS_TEST_PORT

#define COAP_CLIENT_SAMPLE_DEF_NAME             "coapc"

typedef enum {
    COAP_CLIENT_SAMPLE_STATE_READY             = 0,
    COAP_CLIENT_SAMPLE_STATE_SUSPEND           = 1,
    COAP_CLIENT_SAMPLE_STATE_IN_PROGRESS       = 2,
    COAP_CLIENT_SAMPLE_STATE_TERMINATED        = 3,
} coap_client_sample_state_t;

typedef enum {
    COAP_CLIENT_SAMPLE_OPCODE_NONE             = 0,
    COAP_CLIENT_SAMPLE_OPCODE_GET              = 1,
    COAP_CLIENT_SAMPLE_OPCODE_PUT              = 2,
    COAP_CLIENT_SAMPLE_OPCODE_POST             = 3,
    COAP_CLIENT_SAMPLE_OPCODE_DELETE           = 4,
    COAP_CLIENT_SAMPLE_OPCODE_REG_OBSERVE      = 5,
    COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE    = 6,
    COAP_CLIENT_SAMPLE_OPCODE_PING             = 7,
    COAP_CLIENT_SAMPLE_OPCODE_START            = 8,
    COAP_CLIENT_SAMPLE_OPCODE_STOP             = 9,
    COAP_CLIENT_SAMPLE_OPCODE_STATUS           = 10,
} coap_client_sample_opcode_t;

typedef struct _coap_client_sample_request_t {
    coap_client_sample_opcode_t    op_code;

    char *uri;
    size_t urilen;

    char *proxy_uri;
    size_t proxy_urilen;

    unsigned char *data;
    size_t datalen;
} coap_client_sample_request_t;

typedef struct _coap_client_sample_conf_t {
    TaskHandle_t task_handler;
    unsigned int state;

    coap_client_t coap_client;

    QueueHandle_t queue_handler;

    unsigned int req_port;
    unsigned int obs_port;
} coap_client_sample_conf_t;

//Global variable
coap_client_sample_conf_t g_coap_client_sample_conf;

//Function prototype
void coap_client_sample_hexdump(const char *title, unsigned char *buf, size_t len);
void coap_client_sample_display_help();
void coap_client_sample_display_request(coap_client_sample_request_t *request);

int coap_client_sample_copy_request(coap_client_sample_request_t *dst, coap_client_sample_request_t *src);
int coap_client_sample_clear_request(coap_client_sample_request_t *request);
int coap_client_sample_parse_request(int argc, char *argv[], coap_client_sample_request_t *request);
int coap_client_sample_execute_request(coap_client_sample_request_t *request);
int coap_client_sample_cmd(int argc, char *argv[]);
int coap_client_sample_init_config(coap_client_sample_conf_t *config);
int coap_client_sample_deinit_config(coap_client_sample_conf_t *config);
int coap_client_sample_request_get(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_request_put(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_request_post(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_request_delete(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_observe_notify(void *client_ptr, coap_rw_packet_t *resp_ptr);
void coap_client_sample_observe_close_notify(char *timer_name);
int coap_client_sample_register_observe(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_unregister_observe(coap_client_sample_conf_t *config);
int coap_client_sample_request_ping(coap_client_sample_conf_t *config, coap_client_sample_request_t *request);
int coap_client_sample_start(coap_client_sample_conf_t *config);
int coap_client_sample_stop(coap_client_sample_conf_t *config);

void coap_client_sample_processor(void *param);
void coap_client_sample(void *param);

//Function body
static void *coap_client_sample_calloc(size_t n, size_t size)
{
    void *buf = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf) {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

static void coap_client_sample_free(void *f)
{
    if (f == NULL) {
        return;
    }

    vPortFree(f);
}

void coap_client_sample_hexdump(const char *title, unsigned char *buf, size_t len)
{
#if defined (COAP_CLIENT_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#endif // (COAP_CLIENT_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void coap_client_sample_display_help(void)
{
    PRINTF("CoAP Client CMD\r\n");
    PRINTF("-get\r\n");
    PRINTF("-put\r\n");
    PRINTF("-post\r\n");
    PRINTF("-delete\r\n");
    PRINTF("-reg_observe\r\n");
    PRINTF("-unreg_observe\r\n");
    PRINTF("-status\r\n");
    PRINTF("-ping\r\n");
    PRINTF("-start\r\n");
    PRINTF("-stop\r\n");
    PRINTF("-proxy_uri\r\n");

    return ;
}

void coap_client_sample_display_request(coap_client_sample_request_t *request)
{
    if (!request) {
        PRINTF("[%s]Empty request\r\n", __func__);
        return ;
    }

    switch (request->op_code) {
    case COAP_CLIENT_SAMPLE_OPCODE_NONE:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "NONE", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_GET:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "GET", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_PUT:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "PUT", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_POST:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "POST", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_DELETE:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "DELETE", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_REG_OBSERVE:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "REG_OBSERVE",
               request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "UNREG_OBSERVE",
               request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_PING:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "PING", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_STOP:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "STOP", request->op_code);
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_STATUS:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "STATUS", request->op_code);
        break;
    default:
        PRINTF("%-20s: %-10s(%d)\r\n", "Operation code", "Unknown", request->op_code);
        break;
    }

    if (request->urilen) {
        PRINTF("%-20s: %s(%d)\r\n", "URI", request->uri, request->urilen);
    }

    if (request->proxy_urilen) {
        PRINTF("%-20s: %s(%d)\r\n", "PROXY-URI", request->proxy_uri,
               request->proxy_urilen);
    }

    if (request->datalen) {
        PRINTF("%-20s: %s(%d)\r\n", "PAYLOAD", request->data, request->datalen);
    }

    return ;
}

int coap_client_sample_copy_request(coap_client_sample_request_t *dst, coap_client_sample_request_t *src)
{
    int ret = DA_APP_SUCCESS;

    coap_client_sample_clear_request(dst);

    dst->op_code = src->op_code;

    if (src->urilen) {
        dst->uri = coap_client_sample_calloc(src->urilen, sizeof(char));
        if (!dst->uri) {
            PRINTF("[%s]Failed to allocate memory for uri(%ld)\r\n", __func__, src->urilen);
            ret = DA_APP_MALLOC_ERROR;
            goto end;
        }

        memcpy(dst->uri, src->uri, src->urilen);
        dst->urilen = src->urilen;
    }

    if (src->proxy_urilen) {
        dst->proxy_uri = coap_client_sample_calloc(src->proxy_urilen, sizeof(char));
        if (!dst->proxy_uri) {
            PRINTF("[%s]Failed to allocate memory for proxy-uri(%ld)\r\n",
                   __func__, src->proxy_urilen);
            ret = DA_APP_MALLOC_ERROR;
            goto end;
        }

        memcpy(dst->proxy_uri, src->proxy_uri, src->proxy_urilen);
        dst->proxy_urilen = src->proxy_urilen;
    }

    if (src->datalen) {
        dst->data = coap_client_sample_calloc(src->datalen, sizeof(unsigned char));
        if (!dst->data) {
            PRINTF("[%s]failed to allocate memory for data(%ld)\r\n",
                   __func__, src->datalen);
            ret = DA_APP_MALLOC_ERROR;
            goto end;
        }

        memcpy(dst->data, src->data, src->datalen);
        dst->datalen = src->datalen;
    }

end:

    if (ret) {
        coap_client_sample_clear_request(dst);
    }

    return ret;
}

int coap_client_sample_clear_request(coap_client_sample_request_t *request)
{
    if (!request) {
        return DA_APP_SUCCESS;
    }

    request->op_code = COAP_CLIENT_SAMPLE_OPCODE_NONE;

    if (request->uri) {
        coap_client_sample_free(request->uri);
    }

    request->uri = NULL;
    request->urilen = 0;

    if (request->proxy_uri) {
        coap_client_sample_free(request->proxy_uri);
    }

    request->proxy_uri = NULL;
    request->proxy_urilen = 0;

    if (request->data) {
        coap_client_sample_free(request->data);
    }

    request->data = NULL;
    request->datalen = 0;

    return DA_APP_SUCCESS;
}

int coap_client_sample_parse_request(int argc, char *argv[], coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;
    int idx = 0;
    char **cur_argv = ++argv;

    size_t urilen = 0;
    size_t datalen = 0;

    coap_client_sample_clear_request(request);

    for (idx = 1 ; idx < argc ; idx++, cur_argv++) {
        if (strcmp("-get", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_GET;
        } else if ((strcmp("-put", *cur_argv) == 0)
                   || (strcmp("-post", *cur_argv) == 0)) {
            if ((request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) || (argc - idx) < 2) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            if (strcmp("-put", *cur_argv) == 0) {
                request->op_code = COAP_CLIENT_SAMPLE_OPCODE_PUT;
            } else {
                request->op_code = COAP_CLIENT_SAMPLE_OPCODE_POST;
            }

            ++cur_argv;
            --argc;

            datalen = strlen(*cur_argv);

            request->data = coap_client_sample_calloc(datalen + 1, sizeof(unsigned char));
            if (!request->data) {
                PRINTF("[%s]Failed to allocate memory for data\r\n", __func__);
                ret = DA_APP_MALLOC_ERROR;
                goto end;
            }

            memcpy(request->data, *cur_argv, datalen);
            request->data[datalen] = '\0';
            request->datalen = datalen + 1;
        } else if (strcmp("-delete", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_DELETE;
        } else if (strcmp("-reg_observe", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_REG_OBSERVE;
        } else if (strcmp("-unreg_observe", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE;
        } else if (strcmp("-status", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_STATUS;
        } else if (strcmp("-ping", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_PING;
        } else if (strcmp("-start", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_START;
        } else if (strcmp("-stop", *cur_argv) == 0) {
            if (request->op_code != COAP_CLIENT_SAMPLE_OPCODE_NONE) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            request->op_code = COAP_CLIENT_SAMPLE_OPCODE_STOP;
        } else if (strcmp("-proxy_uri", *cur_argv) == 0) {
            if (request->proxy_uri || ((argc - idx) < 2)) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            ++cur_argv;
            --argc;

            request->proxy_urilen = strlen(*cur_argv);

            request->proxy_uri = coap_client_sample_calloc(request->proxy_urilen + 1,
                                 sizeof(unsigned char));
            if (!request->proxy_uri) {
                PRINTF("[%s]Failed to allocate memory for data\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            memcpy(request->proxy_uri, *cur_argv, request->proxy_urilen);
            request->proxy_uri[request->proxy_urilen] = '\0';
            request->proxy_urilen++;
        } else if (strcmp("-help", *cur_argv) == 0) {
            coap_client_sample_display_help();
        } else {
            //set uri
            if (request->uri) {
                PRINTF("[%s]Invalid parameters\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            urilen = strlen(*cur_argv);

            request->uri = coap_client_sample_calloc(urilen + 1, sizeof(unsigned char));
            if (!request->uri) {
                PRINTF("[%s]Failed to allocate memory for uri\r\n", __func__);
                ret = DA_APP_INVALID_PARAMETERS;
                goto end;
            }

            memcpy(request->uri, *cur_argv, urilen);
            request->uri[urilen] = '\0';
            request->urilen = urilen + 1;
        }
    }

end:

    if (ret) {
        coap_client_sample_clear_request(request);
    }

    return ret;
}

int coap_client_sample_execute_request(coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;
    coap_client_sample_conf_t *config = &g_coap_client_sample_conf;
    coap_client_sample_request_t *cp_request = NULL;

    coap_client_sample_opcode_t op_code = request->op_code;

    switch (op_code) {
    case COAP_CLIENT_SAMPLE_OPCODE_GET:
    case COAP_CLIENT_SAMPLE_OPCODE_PUT:
    case COAP_CLIENT_SAMPLE_OPCODE_POST:
    case COAP_CLIENT_SAMPLE_OPCODE_DELETE:
    case COAP_CLIENT_SAMPLE_OPCODE_REG_OBSERVE:
    case COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE:
    case COAP_CLIENT_SAMPLE_OPCODE_PING:
    case COAP_CLIENT_SAMPLE_OPCODE_STOP:
        if (config->state != COAP_CLIENT_SAMPLE_STATE_SUSPEND) {
            PRINTF("[%s]CoAP Client is in progress(%d)\r\n", __func__, config->state);
            ret = DA_APP_IN_PROGRESS;
            break;
        }

        cp_request = coap_client_sample_calloc(1, sizeof(coap_client_sample_request_t));
        if (!cp_request) {
            PRINTF("[%s]Failed to allocate memory to copy CoAP request\r\n", __func__);
            ret = DA_APP_IN_PROGRESS;
            break;
        }

        memset(cp_request, 0x00, sizeof(coap_client_sample_request_t));

        ret = coap_client_sample_copy_request(cp_request, request);
        if (ret != DA_APP_SUCCESS) {
            PRINTF("[%s]Failed to copy request(0x%x)\r\n", __func__, -ret);
            break;
        }

        ret = xQueueSend(config->queue_handler, &cp_request, portMAX_DELAY);
        if (ret != pdTRUE) {
            PRINTF("[%s]Failed to send request to queue(%d)\r\n", __func__, ret);
            ret = DA_APP_NOT_CREATED;
            break;
        }

        ret = DA_APP_SUCCESS;
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_START:
        ret = coap_client_sample_start(config);
        if (ret != DA_APP_SUCCESS) {
            PRINTF("[%s]Failed to start coap client(0x%x)\r\n", __func__, -ret);
            break;
        }
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_STATUS:
        break;
    case COAP_CLIENT_SAMPLE_OPCODE_NONE:
        break;
    default:
        PRINTF("[%s]Unknonw request(%d)\r\n", __func__, request->op_code);
        break;
    }

    if (op_code == COAP_CLIENT_SAMPLE_OPCODE_STOP) {
        ret = coap_client_sample_stop(config);
        if (ret != DA_APP_SUCCESS) {
            PRINTF("[%s]Failed to stop CoAP client sample(0x%x)\r\n", __func__, -ret);
        }
    }

    if (ret && cp_request) {
        coap_client_sample_clear_request(cp_request);
    }

    return ret;
}

int coap_client_sample_cmd(int argc, char *argv[])
{
    int ret = DA_APP_SUCCESS;

    coap_client_sample_request_t request;

    memset(&request, 0x00, sizeof(coap_client_sample_request_t));

    ret = coap_client_sample_parse_request(argc, argv, &request);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to parse request(0x%x)\r\n", __func__, -ret);
        coap_client_sample_display_help();
    } else {
        ret = coap_client_sample_execute_request(&request);
        if (ret != DA_APP_SUCCESS) {
            PRINTF("[%s]Failed to execute request(0x%x)\r\n", __func__, -ret);
        }
    }

    coap_client_sample_clear_request(&request);

    return ret;
}

int coap_client_sample_init_config(coap_client_sample_conf_t *config)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    config->state = COAP_CLIENT_SAMPLE_STATE_SUSPEND;

    //Init coap client
    ret = coap_client_init(coap_client_ptr, COAP_CLIENT_SAMPLE_DEF_NAME);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to init coap client(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    coaps_client_set_authmode(coap_client_ptr, 0);

    config->req_port = COAP_CLIENT_SAMPLE_REQUEST_PORT;
    config->obs_port = COAP_CLIENT_SAMPLE_OBSERVE_PORT;

end:

    return ret;
}

int coap_client_sample_deinit_config(coap_client_sample_conf_t *config)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    //Deinit coap client
    ret = coap_client_deinit(coap_client_ptr);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to deinit coap client(0x%x)\r\n", __func__, -ret);
    }

    return ret;
}

int coap_client_sample_request_get(coap_client_sample_conf_t *config,
                                   coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    coap_rw_packet_t resp_packet;

    memset(&resp_packet, 0x00, sizeof(coap_rw_packet_t));

    //set URI.
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //send coap request
    ret = coap_client_request_get_with_port(coap_client_ptr, config->req_port);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to progress GET request(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //receive coap response
    ret = coap_client_recv_response(coap_client_ptr, &resp_packet);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to response(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //display output
    if (resp_packet.payload.len) {
        coap_client_sample_hexdump("GET Request", resp_packet.payload.p, resp_packet.payload.len);
    }

end:
    //release coap response
    coap_clear_rw_packet(&resp_packet);

    return ret;
}

int coap_client_sample_request_put(coap_client_sample_conf_t *config, coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    coap_rw_packet_t resp_packet;

    memset(&resp_packet, 0x00, sizeof(coap_rw_packet_t));

    //set URI
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //send coap request
    ret = coap_client_request_put_with_port(coap_client_ptr, config->req_port, request->data, request->datalen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to progress PUT request(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //receive coap response
    ret = coap_client_recv_response(coap_client_ptr, &resp_packet);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to get response(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //display output
    if (resp_packet.payload.len) {
        coap_client_sample_hexdump("PUT Request", resp_packet.payload.p, resp_packet.payload.len);
    }

end:
    //release coap response
    coap_clear_rw_packet(&resp_packet);

    return ret;
}

int coap_client_sample_request_post(coap_client_sample_conf_t *config,
                                    coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    coap_rw_packet_t resp_packet;

    memset(&resp_packet, 0x00, sizeof(coap_rw_packet_t));

    //set URI
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //send coap request
    ret = coap_client_request_post_with_port(coap_client_ptr, config->req_port, request->data, request->datalen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to progress POST request(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //receive coap response
    ret = coap_client_recv_response(coap_client_ptr, &resp_packet);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to response(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //display output
    if (resp_packet.payload.len) {
        coap_client_sample_hexdump("POST Request", resp_packet.payload.p, resp_packet.payload.len);
    }

end:
    //release coap response
    coap_clear_rw_packet(&resp_packet);

    return ret;
}

int coap_client_sample_request_delete(coap_client_sample_conf_t *config, coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    coap_rw_packet_t resp_packet;

    memset(&resp_packet, 0x00, sizeof(coap_rw_packet_t));

    //set URI
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //send coap request
    ret = coap_client_request_delete_with_port(coap_client_ptr, config->req_port);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to progress DELETE request(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //receive coap response
    ret = coap_client_recv_response(coap_client_ptr, &resp_packet);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to response(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //display output
    if (resp_packet.payload.len) {
        coap_client_sample_hexdump("DELETE Request", resp_packet.payload.p, resp_packet.payload.len);
    }

end:
    //release coap response
    coap_clear_rw_packet(&resp_packet);

    return ret;
}

int coap_client_sample_observe_notify(void *client_ptr, coap_rw_packet_t *resp_ptr)
{
    DA16X_UNUSED_ARG(client_ptr);

    int ret = DA_APP_SUCCESS;

    PRINTF("[%s]Received Observe notification(%d)\r\n", __func__,
           resp_ptr->payload.len);

    if (resp_ptr->payload.len) {
        coap_client_sample_hexdump("Observe Notification", resp_ptr->payload.p, resp_ptr->payload.len);
    }

    return ret;
}

void coap_client_sample_observe_close_notify(char *timer_name)
{
    DA16X_UNUSED_ARG(timer_name);

    int ret = DA_APP_SUCCESS;
    coap_client_sample_request_t request;

    PRINTF("[%s]Close Observe\r\n", __func__);

    memset(&request, 0x00, sizeof(coap_client_sample_request_t));

    request.op_code = COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE;

    //process unreigster observe
    ret = coap_client_sample_execute_request(&request);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to unregister observe(0x%x)\r\n", __func__, -ret);
    }

    return ;
}

int coap_client_sample_register_observe(coap_client_sample_conf_t *config, coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    //set URI
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //register coap observe
    ret = coap_client_set_observe_notify_with_port(coap_client_ptr, config->obs_port,
                                                   coap_client_sample_observe_notify,
                                                   coap_client_sample_observe_close_notify);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to register observe(0x%x)\r\n", __func__, -ret);
    }

end:

    return ret;
}

int coap_client_sample_unregister_observe(coap_client_sample_conf_t *config)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    coap_client_clear_observe(coap_client_ptr);

    return ret;
}

int coap_client_sample_request_ping(coap_client_sample_conf_t *config, coap_client_sample_request_t *request)
{
    int ret = DA_APP_SUCCESS;

    coap_client_t *coap_client_ptr = &config->coap_client;

    //set URI
    ret = coap_client_set_uri(coap_client_ptr, (unsigned char *)request->uri, request->urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //set Proxy-URI. If null, previous proxy uri will be removed.
    ret = coap_client_set_proxy_uri(coap_client_ptr, (unsigned char *)request->proxy_uri, request->proxy_urilen);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to set Proxy-URI(0x%x)\r\n", __func__, -ret);
        goto end;
    }

    //progress ping request
    ret = coap_client_ping_with_port(coap_client_ptr, config->req_port);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Failed to progress PING request(0x%x)\r\n", __func__, -ret);
    }

end:

    return ret;
}

int coap_client_sample_start(coap_client_sample_conf_t *config)
{
    int ret = DA_APP_SUCCESS;

    if ((config->state != COAP_CLIENT_SAMPLE_STATE_READY) && (config->state != COAP_CLIENT_SAMPLE_STATE_TERMINATED)) {
        PRINTF("[%s]CoAP Client Sample is already running(%d)\r\n",
               __func__, config->state);
        return DA_APP_ALREADY_ENABLED;
    }

    config->queue_handler = xQueueCreate(COAP_CLIENT_SAMPLE_DEF_QUEUE_SIZE, sizeof(void *));
    if (config->queue_handler == NULL) {
        PRINTF("[%s]Failed to create queue\r\n", __func__);
        return DA_APP_NOT_CREATED;
    }

    ret = xTaskCreate(coap_client_sample_processor,
                      COAP_CLIENT_SAMPLE_DEF_TASK_NAME,
                      COAP_CLIENT_SAMPLE_DEF_TASK_SIZE,
                      (void *)config,
                      tskIDLE_PRIORITY + 1,
                      &config->task_handler);
    if (ret != pdPASS) {
        PRINTF("[%s]Failed to create coap client task(%s:%d)\r\n",
               __func__, COAP_CLIENT_SAMPLE_DEF_TASK_NAME, ret);

        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int coap_client_sample_stop(coap_client_sample_conf_t *config)
{
    int ret = DA_APP_SUCCESS;

    unsigned long wait_option = 100;    //100 msec

    const int max_retry_cnt = 100;
    int retry_cnt = 0;

    do {
        if (config->state == COAP_CLIENT_SAMPLE_STATE_TERMINATED) {
            break;
        }

        if (retry_cnt > max_retry_cnt) {
            PRINTF("[%s]Failed to stop CoAP client sample(%d)\r\n", __func__, retry_cnt);
            ret = DA_APP_DELETE_ERROR;
            break;
        }

        retry_cnt++;

        vTaskDelay(wait_option);
    } while (pdTRUE);

    if (ret == 0 && config->task_handler) {
        vTaskDelete(config->task_handler);
        config->task_handler = NULL;
    }

    return ret;
}

void coap_client_sample_processor(void *param)
{
    int ret = DA_APP_SUCCESS;
    unsigned int loop_endlessly = pdTRUE;

    coap_client_sample_conf_t *config = (coap_client_sample_conf_t *)param;
    coap_client_sample_opcode_t op_code = COAP_CLIENT_SAMPLE_OPCODE_NONE;
    coap_client_sample_request_t *request = NULL;

    PRINTF("Start of CoAP Client Sampler\n");

    // Init configuration & coap client
    ret = coap_client_sample_init_config(config);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Falied to init coap client sample(%d)\r\n", __func__, ret);
        return ;
    }

    dpm_app_wakeup_done(SAMPLE_COAP_CLI_DPM);

    dpm_app_data_rcv_ready_set(SAMPLE_COAP_CLI_DPM);

    while (loop_endlessly) {
        config->state = COAP_CLIENT_SAMPLE_STATE_SUSPEND;

        op_code = COAP_CLIENT_SAMPLE_OPCODE_NONE;

        ret = xQueueReceive(config->queue_handler, &request, 100);
        if (ret != pdTRUE) {
            //PRINTF("[%s]Failed to receive data from queue\r\n", __func__);
            dpm_app_sleep_ready_set(SAMPLE_COAP_CLI_DPM);
            continue;
        }

        dpm_app_sleep_ready_clear(SAMPLE_COAP_CLI_DPM);

        ret = DA_APP_SUCCESS;

        config->state = COAP_CLIENT_SAMPLE_STATE_IN_PROGRESS;

        coap_client_sample_display_request(request);

        op_code = request->op_code;

        switch (op_code) {
        case COAP_CLIENT_SAMPLE_OPCODE_GET:
            ret = coap_client_sample_request_get(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_PUT:
            ret = coap_client_sample_request_put(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_POST:
            ret = coap_client_sample_request_post(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_DELETE:
            ret = coap_client_sample_request_delete(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_REG_OBSERVE:
            ret = coap_client_sample_register_observe(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_UNREG_OBSERVE:
            ret = coap_client_sample_unregister_observe(config);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_PING:
            ret = coap_client_sample_request_ping(config, request);
            break;
        case COAP_CLIENT_SAMPLE_OPCODE_STOP:
            goto terminate;
        default:
            PRINTF("[%s]Unknown request(%d)\r\n", __func__, op_code);
            break;
        }

        if (ret != DA_APP_SUCCESS) {
            PRINTF("[%s]Failed to progress operation(%d, 0x%x)\r\n",
                   __func__, op_code, -ret);
        }

        coap_client_sample_clear_request(request);

        request = NULL;
    }

terminate:

    PRINTF("End of CoAP client Sample\r\n");

    if (request) {
        coap_client_sample_clear_request(request);
        request = NULL;
    }

    //Deinit configuration & coap client
    ret = coap_client_sample_deinit_config(config);
    if (ret != DA_APP_SUCCESS) {
        PRINTF("[%s]Falied to deinit coap client sample(0x%x)\r\n", __func__, -ret);
    }

    dpm_app_unregister(SAMPLE_COAP_CLI_DPM);

    config->state = COAP_CLIENT_SAMPLE_STATE_TERMINATED;

    vTaskDelete(NULL);

    return ;
}

void coap_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    int ret = DA_APP_SUCCESS;

    ret = coap_client_sample_start(&g_coap_client_sample_conf);
    if (ret) {
        PRINTF("[%s] Failed to start Coap client sample(0x%x)\r\n", __func__, -ret);
        dpm_app_unregister(SAMPLE_COAP_CLI_DPM);
    }

    vTaskDelete(NULL);

    return ;
}
#endif // (__COAP_CLIENT_SAMPLE__)
#endif // (COAP_CLIENT_SAMPLE_C)

/* EOF */
