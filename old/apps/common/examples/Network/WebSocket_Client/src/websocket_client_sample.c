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
**/


#if defined (__WEBSOCKET_CLIENT_SAMPLE__)

#include "da16x_system.h"
#include "da16x_network_common.h"
#include "iface_defs.h"

#include "websocket_client.h"


#ifdef __SUPPORT_WEBSOCKET_CLIENT__

#define NO_DATA_TIMEOUT_SEC 5

//https://www.piesocket.com/websocket-tester api-key can be changed every week. please update api-key for test
#define WEBSOCKET_SERVER_URI "ws://demo.piesocket.com/v3/channel_1?api_key=VCXCEuvhGcBDP7XhiJJUDvR1e1D3eiVjgZ9VRiaV&notify_self"

static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;

static void shutdown_signaler(TimerHandle_t xTimer)
{
    DA16X_UNUSED_ARG(xTimer);

    WS_LOGI("WEBSOCKET CLIENT SAMPLE", "No data received for %d seconds, signaling shutdown\n", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

static void ws_event_handler (websocket_client_event_id_t event_id, websocket_client_event_data_t *event_data)
{
    websocket_client_event_data_t *data = (websocket_client_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_CLIENT_EVENT_CONNECTED:
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Websocket Connected\n");
        break;
    case WEBSOCKET_CLIENT_EVENT_DISCONNECTED:
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Websocket Disconnected\n");
        break;
    case WEBSOCKET_CLIENT_EVENT_DATA:
        WS_LOGD("WEBSOCKET CLIENT SAMPLE", "Websocket Data\n");
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Received opcode=%d\n", data->op_code);
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Received=%.*s\n", data->data_len, (char *)data->data_ptr);
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len,
                data->data_len, data->payload_offset);

        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_CLIENT_EVENT_CLOSED:
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Websocket Closed\n");
        break;
    case WEBSOCKET_CLIENT_EVENT_ERROR:
        WS_LOGE("WEBSOCKET CLIENT SAMPLE", "Websocket Error\n");
        break;
    default:
        break;
    }
}

static void websocket_app_start(void)
{
    websocket_client_config_t websocket_cfg = {};

    int ret = WS_OK;

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    websocket_cfg.uri = WEBSOCKET_SERVER_URI;

    WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Connecting to %s...\n", websocket_cfg.uri);

    websocket_client_handle_t client = websocket_client_init(&websocket_cfg);

    ret = websocket_client_start(client, ws_event_handler);

    if (ret != WS_OK) {
        WS_LOGE("WEBSOCKET CLIENT SAMPLE", "Fail to start websocket client.", websocket_cfg.uri);
        return;
    }

    xTimerStart(shutdown_signal_timer, portMAX_DELAY);
    char data[32];
    int i = 0;
    while (i < 10) {
        if (websocket_client_is_connected(client)) {
            int len = sprintf(data, "hello %04d", i++);
            WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Sending %s\n", data);
            websocket_client_send_text(client, data, len, portMAX_DELAY);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    xSemaphoreTake(shutdown_sema, portMAX_DELAY);

    if (websocket_client_close_with_code(client, WS_CLOSE_STATUS_NORMAL, NULL, 0, portMAX_DELAY) == WS_OK) {
        WS_LOGI("WEBSOCKET CLIENT SAMPLE", "Websocket Closed\n");
    }
}


void websocket_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    WS_LOGI("WEBSOCKET CLIENT SAMPLE", "WEBSOCKET_CLIENT_SAMPLE\n");

    while (1) {
        if (check_net_ip_status(WLAN0_IFACE) == pdFALSE) {
            WS_LOGI("WEBSOCKET CLIENT SAMPLE", "[APP] Startup..\n");
            websocket_app_start();
            break;
        } else {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }

    vTaskDelete(NULL);

    return ;
}
#endif // (__SUPPORT_WEBSOCKET_CLIENT__)
#endif // (__WEBSOCKET_CLIENT_SAMPLE__)

/* EOF */
