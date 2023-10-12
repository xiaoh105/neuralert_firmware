/**
 * Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright (c) 2021-2022 Modified by Renesas Electronics
 *
 */

#include <stdio.h>

#include "websocket_client.h"

#if defined ( __SUPPORT_WEBSOCKET_CLIENT__ )

#include "da16x_network_main.h"

websocket_client_event_callback ws_cli_event_cb;

static uint64_t _tick_get_ms(void)
{
    return xTaskGetTickCount()*portTICK_PERIOD_MS;
}

static void websocket_client_dispatch_event(websocket_client_handle_t client,
                                            websocket_client_event_id_t event,
                                            const char *data,
                                            int data_len)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_dispatch_event():%d\n", event);

    if (client == NULL) {
        WS_LOGD("WEBSOCKET_CLIENT", "client is Null\n");
        return;
    }

    websocket_client_event_data_t event_data;

    event_data.client = client;
    event_data.user_context = client->config->user_context;
    event_data.data_ptr = data;
    event_data.data_len = data_len;
    event_data.op_code = client->last_opcode;
    event_data.payload_len = client->payload_len;
    event_data.payload_offset = client->payload_offset;

    if (ws_cli_event_cb != NULL) {
        ws_cli_event_cb(event, &event_data);
    } else {
        WS_LOGD("WEBSOCKET_CLIENT", "ws_cli_event_cb is Null\n");
    }

    return;
}

ws_err_t websocket_client_abort_connection(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_abort_connection()\n");
    ws_err_t ret;

    WS_CLIENT_STATE_CHECK("WEBSOCKET_CLIENT", client, return WS_FAIL);
    ret = ws_transport_close(client->transport);

    if (client->config->auto_reconnect) {
        client->wait_timeout_ms = WEBSOCKET_RECONNECT_TIMEOUT_MS;
        client->reconnect_tick_ms = _tick_get_ms();
        WS_LOGI("WEBSOCKET_CLIENT", "Reconnect after %d ms\n", client->wait_timeout_ms);
    }

    client->state = WEBSOCKET_STATE_WAIT_TIMEOUT;

    if (ret==WS_OK){
        WS_LOGD("WEBSOCKET_CLIENT", "websocket transport closed()\n");
    }

    return WS_OK;
}

static ws_err_t ws_websocket_client_set_config(websocket_client_handle_t client, const websocket_client_config_t *config)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_set_config()\n");

    websocket_config_storage_t *cfg = client->config;
    cfg->task_prio = config->task_prio;
    if (cfg->task_prio <= 0) {
        cfg->task_prio = WEBSOCKET_TASK_PRIORITY;
    }

    cfg->task_stack = config->task_stack;
    if (cfg->task_stack == 0) {
        cfg->task_stack = WEBSOCKET_TASK_STACK;
    }

    if (config->host) {
        cfg->host = _ws_strdup(config->host);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->host, return WS_ERR_NO_MEM);
    }

    if (config->port) {
        cfg->port = config->port;
    }

    if (config->username) {
        _ws_free(cfg->username);
        cfg->username = _ws_strdup(config->username);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->username, return WS_ERR_NO_MEM);
    }

    if (config->password) {
        _ws_free(cfg->password);
        cfg->password = _ws_strdup(config->password);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->password, return WS_ERR_NO_MEM);
    }

    if (config->uri) {
        _ws_free(cfg->uri);
        cfg->uri = _ws_strdup(config->uri);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->uri, return WS_ERR_NO_MEM);
    }
    if (config->path) {
        _ws_free(cfg->path);
        cfg->path = _ws_strdup(config->path);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->path, return WS_ERR_NO_MEM);
    }
    if (config->subprotocol) {
        _ws_free(cfg->subprotocol);
        cfg->subprotocol = _ws_strdup(config->subprotocol);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->subprotocol, return WS_ERR_NO_MEM);
    }
    if (config->user_agent) {
        _ws_free(cfg->user_agent);
        cfg->user_agent = _ws_strdup(config->user_agent);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->user_agent, return WS_ERR_NO_MEM);
    }
    if (config->headers) {
        _ws_free(cfg->headers);
        cfg->headers = _ws_strdup(config->headers);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", cfg->headers, return WS_ERR_NO_MEM);
    }

    cfg->network_timeout_ms = WEBSOCKET_NETWORK_TIMEOUT_MS;
    cfg->user_context = config->user_context;
    cfg->auto_reconnect = true;
    if (config->disable_auto_reconnect) {
        cfg->auto_reconnect = false;
    }

    if (config->disable_pingpong_discon){
        cfg->pingpong_timeout_sec = 0;
    } else if (config->pingpong_timeout_sec) {
        cfg->pingpong_timeout_sec = config->pingpong_timeout_sec;
    } else {
        cfg->pingpong_timeout_sec = WEBSOCKET_PINGPONG_TIMEOUT_SEC;
    }

    if (config->ping_interval_sec == 0) {
        cfg->ping_interval_sec = WEBSOCKET_PING_INTERVAL_SEC;
    } else {
        cfg->ping_interval_sec = config->ping_interval_sec;
    }

    return WS_OK;
}

static ws_err_t ws_websocket_client_destroy_config(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_destroy_config()\n");

    if (client == NULL) {
        return WS_ERR_INVALID_ARG;
    }

    websocket_config_storage_t *cfg = client->config;
    if (client->config == NULL) {
        return WS_ERR_INVALID_ARG;
    }

    _ws_free(cfg->host);
    _ws_free(cfg->uri);
    _ws_free(cfg->path);
    _ws_free(cfg->scheme);
    _ws_free(cfg->username);
    _ws_free(cfg->password);
    _ws_free(cfg->subprotocol);
    _ws_free(cfg->user_agent);
    _ws_free(cfg->headers);

    memset(cfg, 0, sizeof(websocket_config_storage_t));
    _ws_free(client->config);
    client->config = NULL;

    return WS_OK;
}

static void set_websocket_transport_optional_settings(websocket_client_handle_t client, ws_transport_handle_t trans)
{
    WS_LOGD("WEBSOCKET_CLIENT", "set_websocket_transport_optional_settings()\n");

    if (trans && client->config->path) {
        ws_transport_ws_set_path(trans, client->config->path);
    }

    if (trans && client->config->subprotocol) {
        ws_transport_ws_set_subprotocol(trans, client->config->subprotocol);
    }

    if (trans && client->config->user_agent) {
        ws_transport_ws_set_user_agent(trans, client->config->user_agent);
    }

    if (trans && client->config->headers) {
        ws_transport_ws_set_headers(trans, client->config->headers);
    }
}

websocket_client_handle_t websocket_client_init(const websocket_client_config_t *config)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_init()\n");

    websocket_client_handle_t client = _ws_calloc(1, sizeof(struct websocket_client));
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client, return NULL);

    client->err = WS_OK;

    if (config->keep_alive_enable == true) {
        client->keep_alive_cfg.keep_alive_enable = true;
        client->keep_alive_cfg.keep_alive_idle = (config->keep_alive_idle == 0) ? WEBSOCKET_KEEP_ALIVE_IDLE : config->keep_alive_idle;
        client->keep_alive_cfg.keep_alive_interval = (config->keep_alive_interval == 0) ? WEBSOCKET_KEEP_ALIVE_INTERVAL : config->keep_alive_interval;
        client->keep_alive_cfg.keep_alive_count =  (config->keep_alive_count == 0) ? WEBSOCKET_KEEP_ALIVE_COUNT : config->keep_alive_count;
    }

    if (config->if_name) {
        client->if_name = _ws_calloc(1, sizeof(struct ifreq) + 1);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->if_name, {
                client->err = WS_ERR_NO_MEM;
                goto _websocket_init_fail;
        });
        memcpy(client->if_name, config->if_name, sizeof(struct ifreq));
    }

    client->lock = xSemaphoreCreateRecursiveMutex();
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->lock, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    client->config = _ws_calloc(1, sizeof(websocket_config_storage_t));
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    client->transport_list = ws_transport_list_init();
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->transport_list, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    ws_transport_handle_t tcp = ws_transport_tcp_init();
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", tcp, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    ws_transport_set_default_port(tcp, WEBSOCKET_TCP_DEFAULT_PORT);
    ws_transport_list_add(client->transport_list, tcp, "_tcp"); // need to save to transport list, for cleanup
    ws_transport_tcp_set_keep_alive(tcp, &client->keep_alive_cfg);
    ws_transport_tcp_set_interface_name(tcp, client->if_name);


    ws_transport_handle_t ws = ws_transport_ws_init(tcp);
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", ws, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    ws_transport_set_default_port(ws, WEBSOCKET_TCP_DEFAULT_PORT);
    ws_transport_list_add(client->transport_list, ws, "ws");
    if (config->transport == WEBSOCKET_CLIENT_TRANSPORT_OVER_TCP) {
        da16x_asprintf(&client->config->scheme, "ws");
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->scheme, {
                client->err = WS_ERR_NO_MEM;
                goto _websocket_init_fail;
        });
    }

    ws_transport_handle_t ssl = ws_transport_ssl_init();
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", ssl, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    ws_transport_set_default_port(ssl, WEBSOCKET_SSL_DEFAULT_PORT);
    if (config->cert_pem) {
        ws_transport_ssl_set_cert_data(ssl, config->cert_pem, strlen(config->cert_pem));
    }
    ws_transport_ssl_set_keep_alive(ssl, &client->keep_alive_cfg);
    ws_transport_list_add(client->transport_list, ssl, "_ssl"); // need to save to transport list, for cleanup

    ws_transport_handle_t wss = ws_transport_ws_init(ssl);
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", wss, {
            client->err = WS_ERR_NO_MEM;
            goto _websocket_init_fail;
    });

    ws_transport_set_default_port(wss, WEBSOCKET_SSL_DEFAULT_PORT);

    ws_transport_list_add(client->transport_list, wss, "wss");
    if (config->transport == WEBSOCKET_CLIENT_TRANSPORT_OVER_SSL) {
        da16x_asprintf(&client->config->scheme, "wss");
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->scheme, {
                client->err = WS_ERR_NO_MEM;
                goto _websocket_init_fail;
        });
    }

    if (config->uri) {
        if (websocket_client_set_uri(client, config->uri) != WS_OK) {
            WS_LOGE("WEBSOCKET_CLIENT", "Invalid uri\n");
            client->err = WS_ERR_INVALID_ARG;
            goto _websocket_init_fail;
        }
    }

    if (ws_websocket_client_set_config(client, config) != WS_OK) {
        WS_LOGE("WEBSOCKET_CLIENT", "Failed to set the configuration\n");
        client->err = WS_ERR_INVALID_ARG;
        goto _websocket_init_fail;
    }

    if (client->config->scheme == NULL) {
        da16x_asprintf(&client->config->scheme, "ws");
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->scheme, {
                client->err = WS_ERR_NO_MEM;
                goto _websocket_init_fail;
        });
    }

    set_websocket_transport_optional_settings(client, ws_transport_list_get_transport(client->transport_list, "ws"));
    set_websocket_transport_optional_settings(client, ws_transport_list_get_transport(client->transport_list, "wss"));

    client->keepalive_tick_ms = _tick_get_ms();
    client->reconnect_tick_ms = _tick_get_ms();
    client->ping_tick_ms = _tick_get_ms();
    client->wait_for_pong_resp = false;

    int buffer_size = config->buffer_size;
    if (buffer_size <= 0) {
        buffer_size = WEBSOCKET_BUFFER_SIZE_BYTE;
    }
    client->rx_buffer = pvPortMalloc(buffer_size);
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->rx_buffer, {
        client->err = WS_ERR_NO_MEM;
        goto _websocket_init_fail;
    });
    client->tx_buffer = pvPortMalloc(buffer_size);
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->tx_buffer, {
        client->err = WS_ERR_NO_MEM;
        goto _websocket_init_fail;
    });
    client->status_bits = xEventGroupCreate();
    WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->status_bits, {
        client->err = WS_ERR_NO_MEM;
        goto _websocket_init_fail;
    });

    client->buffer_size = buffer_size;
    return client;

_websocket_init_fail:
    WS_LOGD("WEBSOCKET_CLIENT", "_websocket_init_fail()\n");
    return client;
}

ws_err_t websocket_client_destroy(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_destroy()\n");

    if (client == NULL) {
        return WS_ERR_NO_MEM;
    }
    if (client->run) {
        websocket_client_stop(client);
    }
    if (client->if_name) {
        vPortFree(client->if_name);
    }

    ws_websocket_client_destroy_config(client);
    ws_transport_list_destroy(client->transport_list);
    vQueueDelete(client->lock);
    _ws_free(client->tx_buffer);
    _ws_free(client->rx_buffer);
    if (client->status_bits) {
        vEventGroupDelete(client->status_bits);
    }
    _ws_free(client);
    client = NULL;
    ws_cli_event_cb = NULL;
    return WS_OK;
}

ws_err_t websocket_client_set_uri(websocket_client_handle_t client, const char *uri)
{
    WS_LOGD("WEBSOCKET_CLIENT", "ws_websocket_client_set_uri()\n");

    if (client == NULL || uri == NULL) {
        return WS_ERR_INVALID_ARG;
    }
    struct http_parser_url puri;
    http_parser_url_init(&puri);
    int parser_status = http_parser_parse_url(uri, strlen(uri), 0, &puri);
    if (parser_status != 0) {
        WS_LOGE("WEBSOCKET_CLIENT", "Error parse uri = %s\n", uri);
        return WS_FAIL;
    }
    if (puri.field_data[UF_SCHEMA].len) {
        _ws_free(client->config->scheme);
        da16x_asprintf(&client->config->scheme, "%.*s", puri.field_data[UF_SCHEMA].len, uri + puri.field_data[UF_SCHEMA].off);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->scheme, return WS_ERR_NO_MEM);
    }

    if (puri.field_data[UF_HOST].len) {
        _ws_free(client->config->host);
        da16x_asprintf(&client->config->host, "%.*s", puri.field_data[UF_HOST].len, uri + puri.field_data[UF_HOST].off);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->host, return WS_ERR_NO_MEM);
    }


    if (puri.field_data[UF_PATH].len || puri.field_data[UF_QUERY].len) {
        _ws_free(client->config->path);
        if (puri.field_data[UF_QUERY].len == 0) {
            da16x_asprintf(&client->config->path, "%.*s", puri.field_data[UF_PATH].len, uri + puri.field_data[UF_PATH].off);
        } else if (puri.field_data[UF_PATH].len == 0)  {
            da16x_asprintf(&client->config->path, "/?%.*s", puri.field_data[UF_QUERY].len, uri + puri.field_data[UF_QUERY].off);
        } else {
            da16x_asprintf(&client->config->path, "%.*s?%.*s", puri.field_data[UF_PATH].len, uri + puri.field_data[UF_PATH].off,
                    puri.field_data[UF_QUERY].len, uri + puri.field_data[UF_QUERY].off);
        }
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->path, return WS_ERR_NO_MEM);
    }
    if (puri.field_data[UF_PORT].off) {
        client->config->port = strtol((const char*)(uri + puri.field_data[UF_PORT].off), NULL, 10);
    }

    if (puri.field_data[UF_USERINFO].len) {
        char *user_info = NULL;
        da16x_asprintf(&user_info, "%.*s", puri.field_data[UF_USERINFO].len, uri + puri.field_data[UF_USERINFO].off);
        if (user_info) {
            char *pass = strchr(user_info, ':');
            if (pass) {
                pass[0] = 0; //terminal username
                pass ++;
                _ws_free(client->config->password);
                client->config->password = _ws_strdup(pass);
                WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->password, return WS_ERR_NO_MEM);
            }
            _ws_free(client->config->username);
            client->config->username = _ws_strdup(user_info);
            WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", client->config->username, return WS_ERR_NO_MEM);
            _ws_free(user_info);
        } else {
            return WS_ERR_NO_MEM;
        }
    }
    return WS_OK;
}

static ws_err_t websocket_client_recv(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_recv()\n");

    int rlen;
    client->payload_offset = 0;

    do {
        rlen = ws_transport_read(client->transport, client->rx_buffer, client->buffer_size, client->config->network_timeout_ms);
        if (rlen < 0) {
            WS_LOGE("WEBSOCKET_CLIENT", "Error read data.\n");
               return WS_FAIL;
        }

        client->payload_len = ws_transport_ws_get_read_payload_len(client->transport);
        client->last_opcode = ws_transport_ws_get_read_opcode(client->transport);

        if (rlen == 0 && client->last_opcode == WS_TRANSPORT_OPCODES_NONE ) {
            WS_LOGV("WEBSOCKET_CLIENT", "ws_transport_read timeouts\n");
            return WS_OK;
        }

        websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_DATA, client->rx_buffer, rlen);
        client->payload_offset += rlen;
    } while (client->payload_offset < client->payload_len);

    //Print Receive Message
    if (client->payload_len >0)
    {
        void* recv_data = _ws_calloc(1, client->payload_len+1);    //for null string
        memcpy(recv_data, client->rx_buffer, client->payload_len);
        WS_LOGD("WEBSOCKET_CLIENT", LIGHT_CYN_COL"Recv Message: %s\n"CLR_COL, recv_data);
        WS_LOGD("WEBSOCKET_CLIENT", "Payload len:%d, offset:%d\n", client->payload_len, client->payload_offset);
        _ws_free(recv_data);
    }

    // if a PING message received -> send out the PONG, this will not work for PING messages with payload longer than buffer len
    if (client->last_opcode == WS_TRANSPORT_OPCODES_PING)
    {
        WS_LOGD("WEBSOCKET_CLIENT", "Recv PING\n");
        const char *data = (client->payload_len == 0) ? NULL : client->rx_buffer;
        WS_LOGD("WEBSOCKET_CLIENT", "Sending PONG with payload len=%d\n", client->payload_len);
        ws_transport_ws_send_raw(client->transport, WS_TRANSPORT_OPCODES_PONG | WS_TRANSPORT_OPCODES_FIN, data, client->payload_len,
                                  client->config->network_timeout_ms);
    } else if (client->last_opcode == WS_TRANSPORT_OPCODES_PONG) {
        client->wait_for_pong_resp = false;
    } else if (client->last_opcode == WS_TRANSPORT_OPCODES_CLOSE) {
        WS_LOGD("WEBSOCKET_CLIENT", "Received close frame\n");
        client->state = WEBSOCKET_STATE_CLOSING;
    }

    return WS_OK;
}

static int websocket_client_send_with_opcode(websocket_client_handle_t client, ws_transport_opcodes_t opcode, const uint8_t *data, int len, TickType_t timeout);

static int websocket_client_send_close(websocket_client_handle_t client, int code, const char *additional_data, int total_len, TickType_t timeout);

static void websocket_client_task(void *pv)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_task()\n");

    bool has_error = pdFALSE;
    const int lock_timeout = portMAX_DELAY;
    websocket_client_handle_t client = (websocket_client_handle_t) pv;
    client->run = true;

    //get transport by scheme
    client->transport = ws_transport_list_get_transport(client->transport_list, client->config->scheme);

    if (client->transport == NULL) {
        WS_LOGE("WEBSOCKET_CLIENT", "There are no transports valid, stop websocket client\n");
        client->run = false;
    }
    //default port
    if (client->config->port == 0) {
        client->config->port = ws_transport_get_default_port(client->transport);
    }

    client->state = WEBSOCKET_STATE_INIT;
    xEventGroupClearBits(client->status_bits, STOPPED_BIT);
    int read_select = 0;
    while (client->run) {
        if (xSemaphoreTakeRecursive(client->lock, lock_timeout) != pdPASS) {
            WS_LOGE("WEBSOCKET_CLIENT", "Failed to lock ws-client tasks, exitting the task...\n");
            break;
        }
        switch ((int)client->state) {
            case WEBSOCKET_STATE_INIT:
                WS_LOGD("WEBSOCKET_CLIENT", "WEBSOCKET_STATE_INIT\n");
                if (client->transport == NULL) {
                    WS_LOGE("WEBSOCKET_CLIENT", "There are no transport\n");
                    client->run = false;
                    break;
                }
                if (ws_transport_connect(client->transport,
                                          client->config->host,
                                          client->config->port,
                                          client->config->network_timeout_ms) < 0) {
                    WS_LOGE("WEBSOCKET_CLIENT", "Error transport connect\n");
                    websocket_client_abort_connection(client);
                    websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_FAIL_TO_CONNECT, NULL, 0);
                    break;
                }
                WS_LOGD("WEBSOCKET_CLIENT", "Transport connected to %s://%s:%d\n", client->config->scheme, client->config->host, client->config->port);

                client->state = WEBSOCKET_STATE_CONNECTED;
                client->wait_for_pong_resp = false;
                websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_CONNECTED, NULL, 0);

                break;
            case WEBSOCKET_STATE_CONNECTED:
                WS_LOGD("WEBSOCKET_CLIENT", "WEBSOCKET_STATE_CONNECTED\n");
                if ((CLOSE_FRAME_SENT_BIT & xEventGroupGetBits(client->status_bits)) == 0) { // only send and check for PING
                                                                                                          // if closing hasn't been initiated
                    if (_tick_get_ms() - client->ping_tick_ms > client->config->ping_interval_sec*1000) {
                        client->ping_tick_ms = _tick_get_ms();
                        WS_LOGD("WEBSOCKET_CLIENT", "Sending PING...\n");
                        ws_transport_ws_send_raw(client->transport, WS_TRANSPORT_OPCODES_PING | WS_TRANSPORT_OPCODES_FIN, NULL, 0, client->config->network_timeout_ms);
                        if (!client->wait_for_pong_resp && client->config->pingpong_timeout_sec) {
                            client->pingpong_tick_ms = _tick_get_ms();
                            client->wait_for_pong_resp = true;
                        }
                    }

                    if ( _tick_get_ms() - client->pingpong_tick_ms > client->config->pingpong_timeout_sec*1000 ) {
                        if (client->wait_for_pong_resp) {
                            WS_LOGE("WEBSOCKET_CLIENT", "Error, no PONG received for more than %d seconds after PING\n", client->config->pingpong_timeout_sec);
                            websocket_client_abort_connection(client);
                            has_error = pdTRUE;
                            break;
                        }
                    }
                }
                if (read_select == 0) {
                    WS_LOGV("WEBSOCKET_CLIENT", "Read poll timeout: skipping ws_transport_read()...\n");
                    break;
                }
                client->ping_tick_ms = _tick_get_ms();

                if (websocket_client_recv(client) == WS_FAIL) {
                    WS_LOGE("WEBSOCKET_CLIENT", "Error receive data\n");
                    websocket_client_abort_connection(client);
                    has_error = pdTRUE;
                    break;
                }
                break;
            case WEBSOCKET_STATE_WAIT_TIMEOUT:
                WS_LOGD("WEBSOCKET_CLIENT", "WEBSOCKET_STATE_WAIT_TIMEOUT\n");
                if (!client->config->auto_reconnect) {
                    client->run = false;
                    break;
                }

                if (_tick_get_ms() - client->reconnect_tick_ms > client->wait_timeout_ms) {
                    client->state = WEBSOCKET_STATE_INIT;
                    client->reconnect_tick_ms = _tick_get_ms();
                    WS_LOGI("WEBSOCKET_CLIENT", "Reconnecting...\n");
                }
                break;
            case WEBSOCKET_STATE_CLOSING:
                WS_LOGD("WEBSOCKET_CLIENT", "WEBSOCKET_STATE_CLOSING\n");
                // if closing not initiated by the client echo the close message back
                if ((CLOSE_FRAME_SENT_BIT & xEventGroupGetBits(client->status_bits)) == 0) {
                    WS_LOGD("WEBSOCKET_CLIENT", "Closing initiated by the server, sending close frame\n");
                    websocket_client_send_close(client, WS_CLOSE_STATUS_NORMAL, NULL, 2, client->config->network_timeout_ms); // len + 2 -> always sending the code
                    xEventGroupSetBits(client->status_bits, CLOSE_FRAME_SENT_BIT);
                }
                break;
            default:
                WS_LOGD("WEBSOCKET_CLIENT", "Client run iteration in a default state: %d\n", client->state);
                break;
        }
        xSemaphoreGiveRecursive(client->lock);
        if (WEBSOCKET_STATE_CONNECTED == client->state) {
            read_select = ws_transport_poll_read(client->transport, 1000); //Poll every 1000ms
            if (read_select < 0) {
                WS_LOGE("WEBSOCKET_CLIENT", "Network error: ws_transport_poll_read() returned %d, errno=%d\n", read_select, errno);
                websocket_client_abort_connection(client);
                has_error = pdTRUE;
            }
        } else if (WEBSOCKET_STATE_WAIT_TIMEOUT == client->state) {
            // waiting for reconnecting...
            WS_LOGD("WEBSOCKET_CLIENT", "Waiting for reconnecting\n");
            vTaskDelay(client->wait_timeout_ms / 2 / portTICK_RATE_MS);
        } else if (WEBSOCKET_STATE_CLOSING == client->state &&
                  (CLOSE_FRAME_SENT_BIT & xEventGroupGetBits(client->status_bits))) {
            WS_LOGD("WEBSOCKET_CLIENT", "Waiting for TCP connection to be closed by the server\n");
            int ret = ws_transport_ws_poll_connection_closed(client->transport, 1000);
            if (ret == 0) {
                // still waiting
                break;
            }
            if (ret < 0) {
                WS_LOGW("WEBSOCKET_CLIENT", "Connection terminated while waiting for clean TCP close\n");
            }
            client->run = false;
            client->state = WEBSOCKET_STATE_UNKNOW;
            websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_CLOSED, NULL, 0);
            break;
        }
    }

    if (has_error) {
        websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_ERROR, NULL, 0);
    }

    if (ws_transport_close(client->transport)==WS_OK){
        WS_LOGD("WEBSOCKET_CLIENT", "transport closed.\n");
        websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_DISCONNECTED, NULL, 0);
        client->run = false;
    }

    xEventGroupSetBits(client->status_bits, STOPPED_BIT);
    client->state = WEBSOCKET_STATE_UNKNOW;
    websocket_client_destroy(client);

    vTaskDelete(NULL);
}

ws_err_t websocket_client_start(websocket_client_handle_t client, websocket_client_event_callback callback)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_start()\n");

    int status = WS_OK;

    if (client == NULL) {
        WS_LOGE("WEBSOCKET_CLIENT", "Error: Client does not created.\n");
        return WS_ERR_NO_MEM;
    }
    if (callback == NULL) {
        WS_LOGE("WEBSOCKET_CLIENT", "Error: callback-function is null.\n");
        return WS_ERR_CB_FUNC_NOT_EXIST;
    }

    status = client->err;

    if (status != WS_OK)
    {
        websocket_client_destroy(client);
        return status;
    }

    if (!da16x_network_main_check_network_ready(WLAN0_IFACE)) {
        WS_LOGD("WEBSOCKET_CLIENT", "Websocket Client is only running in station mode.\n");

        websocket_client_destroy(client);
        return WS_ERR_NOT_SUPPORTED;
    }

    if (client->state >= WEBSOCKET_STATE_INIT) {
        WS_LOGE("WEBSOCKET_CLIENT", "Error: The client has started\n");
        return WS_ERR_INVALID_STATE;
    }

    ws_cli_event_cb = callback;

    if (xTaskCreate(websocket_client_task, "websocket_client", client->config->task_stack, client, (UBaseType_t)(client->config->task_prio), &client->task_handle) != pdTRUE) {
        WS_LOGE("WEBSOCKET_CLIENT", "Error: Fail to create websocket task.\n");
        websocket_client_destroy(client);
        return WS_FAIL;
    }

    xEventGroupClearBits(client->status_bits, STOPPED_BIT | CLOSE_FRAME_SENT_BIT);
    return WS_OK;
}

ws_err_t websocket_client_stop(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_stop()\n");

    if (client == NULL) {
        return WS_ERR_INVALID_ARG;
    }

    if (!client->run) {
        WS_LOGW("WEBSOCKET_CLIENT", "Client was not started\n");
        return WS_FAIL;
    }

    /* A running client cannot be stopped from the websocket task/event handler */
    TaskHandle_t running_task = xTaskGetCurrentTaskHandle();
    if (running_task == client->task_handle) {
        WS_LOGE("WEBSOCKET_CLIENT", "Client cannot be stopped from websocket task\n");
        return WS_FAIL;
    }

    client->run = false;
    xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, portMAX_DELAY);
    client->state = WEBSOCKET_STATE_UNKNOW;
    return WS_OK;
}

static int websocket_client_send_close(websocket_client_handle_t client, int code, const char *additional_data, int total_len, TickType_t timeout)
{
    uint8_t *close_status_data = NULL;
    // RFC6455#section-5.5.1: The Close frame MAY contain a body (indicated by total_len >= 2)
    if (total_len >= 2) {
        close_status_data = _ws_calloc(1, (size_t)total_len);
        WS_CLIENT_MEM_CHECK("WEBSOCKET_CLIENT", close_status_data, return -1);
        // RFC6455#section-5.5.1: The first two bytes of the body MUST be a 2-byte representing a status
        uint16_t *code_network_order = (uint16_t *) close_status_data;
        *code_network_order = (uint16_t)htons(code);
        memcpy(close_status_data + 2, additional_data, (size_t)(total_len - 2));
    }
    int ret = websocket_client_send_with_opcode(client, WS_TRANSPORT_OPCODES_CLOSE, close_status_data, total_len, timeout);
    vPortFree(close_status_data);
    return ret;
}

static ws_err_t websocket_client_close_with_optional_body(websocket_client_handle_t client, bool send_body, int code, const char *data, int len, TickType_t timeout)
{
    if (client == NULL) {
        return WS_ERR_INVALID_ARG;
    }
    if (!client->run) {
        WS_LOGW("WEBSOCKET_CLIENT", "Client was not started\n");
        return WS_FAIL;
    }

    /* A running client cannot be stopped from the websocket task/event handler */
    TaskHandle_t running_task = xTaskGetCurrentTaskHandle();
    if (running_task == client->task_handle) {
        WS_LOGE("WEBSOCKET_CLIENT", "Client cannot be stopped from websocket task\n");
        return WS_FAIL;
    }

    if (send_body) {
        websocket_client_send_close(client, code, data, len + 2, portMAX_DELAY); // len + 2 -> always sending the code
    } else {
        websocket_client_send_close(client, 0, NULL, 0, portMAX_DELAY); // only opcode frame
    }

    // Set closing bit to prevent from sending PING frames while connected
    xEventGroupSetBits(client->status_bits, CLOSE_FRAME_SENT_BIT);

    if (STOPPED_BIT & xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, timeout)) {
        return WS_OK;
    }

    // If could not close gracefully within timeout, stop the client and disconnect
    client->run = false;
    xEventGroupWaitBits(client->status_bits, STOPPED_BIT, false, true, portMAX_DELAY);
    client->state = WEBSOCKET_STATE_UNKNOW;
    return WS_OK;
}

ws_err_t websocket_client_close_with_code(websocket_client_handle_t client, int code, const char *data, int len, TickType_t timeout)
{
    return websocket_client_close_with_optional_body(client, true, code, data, len, timeout);
}

ws_err_t websocket_client_close(websocket_client_handle_t client, TickType_t timeout)
{
    return websocket_client_close_with_optional_body(client, false, 0, NULL, 0, timeout);
}


int websocket_client_send_text(websocket_client_handle_t client, const char *data, int len, TickType_t timeout)
{
    return websocket_client_send_with_opcode(client, WS_TRANSPORT_OPCODES_TEXT, (const uint8_t *)data, len, timeout);
}

int websocket_client_send(websocket_client_handle_t client, const char *data, int len, TickType_t timeout)
{
    return websocket_client_send_with_opcode(client, WS_TRANSPORT_OPCODES_BINARY, (const uint8_t *)data, len, timeout);
}

int websocket_client_send_bin(websocket_client_handle_t client, const char *data, int len, TickType_t timeout)
{
    return websocket_client_send_with_opcode(client, WS_TRANSPORT_OPCODES_BINARY, (const uint8_t *)data, len, timeout);
}

static int websocket_client_send_with_opcode(websocket_client_handle_t client, ws_transport_opcodes_t opcode, const uint8_t *data, int len, TickType_t timeout)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_send_with_opcode()\n");

    bool has_error = pdFALSE;
    int need_write = len;
    int wlen = 0, widx = 0;
    int ret = WS_FAIL;

    if (client == NULL || len < 0 ||
        (opcode != WS_TRANSPORT_OPCODES_CLOSE && (data == NULL || len <= 0))) {
        WS_LOGE("WEBSOCKET_CLIENT", "Invalid arguments\n");
        return WS_FAIL;
    }

    if (xSemaphoreTakeRecursive(client->lock, timeout) != pdPASS) {
        WS_LOGE("WEBSOCKET_CLIENT", "Could not lock ws-client within %d timeout\n", timeout);
        return WS_FAIL;
    }

    if (!websocket_client_is_connected(client) && (WEBSOCKET_STATE_CLOSING!=client->state)) {
        WS_LOGE("WEBSOCKET_CLIENT", "Websocket client is not connected: %d\n", client->state);
        goto unlock_and_return;
    }

    if (client->transport == NULL) {
        WS_LOGE("WEBSOCKET_CLIENT", "Invalid transport\n");
        goto unlock_and_return;
    }
    uint32_t current_opcode = opcode;
    while (widx < len || current_opcode) {  // allow for sending "current_opcode" only message with len==0
        if (need_write > client->buffer_size) {
            need_write = client->buffer_size;
        } else {
            current_opcode |= WS_TRANSPORT_OPCODES_FIN;
        }
        memcpy(client->tx_buffer, data + widx, need_write);
        // send with ws specific way and specific opcode
        wlen = ws_transport_ws_send_raw(client->transport, current_opcode, (char *)client->tx_buffer, need_write,
                                        (timeout==portMAX_DELAY)? -1 : (int)(timeout * portTICK_PERIOD_MS));
        if (wlen < 0 || (wlen == 0 && need_write != 0)) {
            ret = wlen;
            WS_LOGE("WEBSOCKET_CLIENT", "Network error: ws_transport_write() returned %d, errno=%d\n", ret, errno);
            websocket_client_abort_connection(client);
            has_error = pdTRUE;
            goto unlock_and_return;
        }
        current_opcode = 0;
        widx += wlen;
        need_write = len - widx;

    }
    ret = widx;
unlock_and_return:
    xSemaphoreGiveRecursive(client->lock);
    if (has_error) {
        websocket_client_dispatch_event(client, WEBSOCKET_CLIENT_EVENT_ERROR, NULL, 0);
    }

    return ret;
}

bool websocket_client_is_connected(websocket_client_handle_t client)
{
    WS_LOGD("WEBSOCKET_CLIENT", "websocket_client_is_connected()\n");

    if (client == NULL) {
        return false;
    }

    return client->state == WEBSOCKET_STATE_CONNECTED;
}

#endif // __SUPPORT_WEBSOCKET_CLIENT__

/* EOF */
