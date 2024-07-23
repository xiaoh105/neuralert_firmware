/**
 ****************************************************************************************
 *
 * @file atcmd_websocket_client.c
 *
 * @brief AT-CMD Websocket Client Controller
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

#include "atcmd_websocket_client.h"

#ifdef __SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__

#include "atcmd.h"

websocket_client_handle_t client;

void websocket_event_handler (websocket_client_event_id_t event_id, websocket_client_event_data_t *event_data)
{
	websocket_client_event_data_t *data;

	if (event_data == NULL) {
		WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", "event data is NULL\n");
		return;
	}

	data = (websocket_client_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_CLIENT_EVENT_CONNECTED:
        WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Connected\n");
        PRINTF_ATCMD("\r\n+NWWSC:1\r\n");
        PRINTF_ATCMD("\r\nOK\r\n");
        break;

    case WEBSOCKET_CLIENT_EVENT_DISCONNECTED:
        WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Disconnected\n");
        PRINTF_ATCMD("\r\n+NWWSC:0\r\n");
        break;

    case WEBSOCKET_CLIENT_EVENT_DATA:
        WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Data\n");
        WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", "Received opcode=%d\n", data->op_code);

        if (data->op_code == WS_TRANSPORT_OPCODES_CLOSE && data->data_len == 2) {
            WS_LOGW("WEBSOCKET_CLIENT_FOR_ATCMD", "Received closed message with code=%d\n", 256*data->data_ptr[0] + data->data_ptr[1]);
        } else {
            WS_LOGW("WEBSOCKET_CLIENT_FOR_ATCMD", "Received=%.*s\n", data->data_len, (char *)data->data_ptr);
        }

        WS_LOGW("WEBSOCKET_CLIENT_FOR_ATCMD", "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        if (data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
        	PRINTF_ATCMD("\r\n+NWWSC:1,%d,%d,%.*s\r\n", data->op_code, data->data_len, data->data_len, (char *)data->data_ptr);
        }
        break;

    case WEBSOCKET_CLIENT_EVENT_CLOSED:
    	WS_LOGW("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Closed\n");
        break;

    case WEBSOCKET_CLIENT_EVENT_FAIL_TO_CONNECT:
    	WS_LOGE("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Failed to connect to server\n");
		PRINTF_ATCMD("\r\nERROR:%d\r\n", AT_CMD_ERR_NW_WSC_SESS_NOT_CONNECTED);
    	break;

    case WEBSOCKET_CLIENT_EVENT_ERROR:
        WS_LOGE("WEBSOCKET_CLIENT_FOR_ATCMD", ">> Websocket Error\n");
        PRINTF_ATCMD("\r\nERROR:%d\r\n", AT_CMD_ERR_UNKNOWN);
        break;

    default:
    	break;
    }
}

ws_err_t websocket_client_connect(char * uri)
{
	WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", "websocket_client_connect()\n");

	if (websocket_client_is_connected(client)) {
		WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", "Websocket Client is already connected.\n");
		return WS_ERR_CONN_ALREADY_EXIST;
	}

    websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = uri;
    websocket_cfg.disable_auto_reconnect=true;

    WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", "Connecting to %s...\n", websocket_cfg.uri);

    client = websocket_client_init(&websocket_cfg);

    return websocket_client_start(client, websocket_event_handler);
}

ws_err_t websocket_client_disconnect(void)
{
	WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", "websocket_client_disconnect()\n");

	if (websocket_client_is_connected(client) == false) {
		WS_LOGE("WEBSOCKET_CLIENT_FOR_ATCMD", "Websocket Client already disconnected.\n");
		return WS_ERR_INVALID_STATE;
	}

	if (websocket_client_close_with_code(client, WS_CLOSE_STATUS_NORMAL, NULL, 0, client->config->network_timeout_ms) != WS_OK) {
		WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", "Failed to send Close Frame.\n");
		return WS_FAIL;
	}

    return WS_OK;
}

ws_err_t websocket_client_send_msg(char *msg)
{
	WS_LOGD("WEBSOCKET_CLIENT_FOR_ATCMD", "websocket_client_send_msg()\n");

	if (websocket_client_is_connected(client)) {
		if (NULL == msg) {
			return WS_FAIL;
		}

		WS_LOGI("WEBSOCKET_CLIENT_FOR_ATCMD", "Sending %s, len=%d\n", msg, strlen(msg));
		return websocket_client_send_text(client, msg, strlen(msg), portMAX_DELAY);
	}

	WS_LOGE("WEBSOCKET_CLIENT_FOR_ATCMD", "websocket_client is not connected.\n");
	return WS_FAIL;
}
#endif /*__SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__*/

/* EOF */
