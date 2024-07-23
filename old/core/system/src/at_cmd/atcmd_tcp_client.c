/**
 ****************************************************************************************
 *
 * @file atcmd_tcp_client.c
 *
 * @brief AT-CMD TCP Client Socket Controller
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

#include "atcmd_tcp_client.h"
#include "da16x_sys_watchdog.h"

#if defined (__SUPPORT_ATCMD__)

#undef ENABLE_ATCMD_TCPC_DBG_INFO
#undef ENABLE_ATCMD_TCPC_DBG_ERR

#define ATCMD_TCPC_DBG  ATCMD_DBG

#if defined (ENABLE_ATCMD_TCPC_DBG_INFO)
#define ATCMD_TCPC_INFO(fmt, ...)   ATCMD_TCPC_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_TCPC_INFO(...)        do {} while (0)
#endif  // (ENABLE_ATCMD_TCPC_DBG_INFO)

#if defined (ENABLE_ATCMD_TCPC_DBG_ERR)
#define ATCMD_TCPC_ERR(fmt, ...)    ATCMD_TCPC_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ATCMD_TCPC_ERR(...)         do {} while (0)
#endif // (ENABLE_ATCMD_TCPC_DBG_ERR) 


static void atcmd_tcpc_task_entry(void *param);

#if defined(__SUPPORT_TCP_RECVDATA_HEX_MODE__)
static int tcpc_data_mode = 0;

static int is_tcpc_data_mode_hexstring(void)
{
    return tcpc_data_mode == 1 ? 1 : 0;
}

void set_tcpc_data_mode(int mode)
{
    /*
     * TCP_DATA_MODE_ASCII 	= 0
     * TCP_DATA_MODE_HEXSTRING = 1
     */
    tcpc_data_mode = mode;
}

static void Convert_Str2HexStr(unsigned char *in, char *out, u32_t in_size)
{
    u32_t cnt = 0;
    u32_t i = 0;

    while (cnt < in_size) {
        sprintf((char *)(out + i), "%02X", in[cnt]);
        cnt += 1;
        i += 2;
    }
}
#endif // __SUPPORT_TCP_RECVDATA_HEX_MODE__

int atcmd_tcpc_init_context(atcmd_tcpc_context *ctx)
{
    ATCMD_TCPC_INFO("Start\r\n");

    memset(ctx, 0x00, sizeof(atcmd_tcpc_context));

    ctx->socket = -1;
    ctx->state = ATCMD_TCPC_STATE_TERMINATED;

    return 0;
}

int atcmd_tcpc_deinit_context(atcmd_tcpc_context *ctx)
{
    ATCMD_TCPC_INFO("Start\r\n");

    if (ctx->state != ATCMD_TCPC_STATE_TERMINATED) {
        ATCMD_TCPC_ERR("tcp client is not terminated(%d)\r\n", ctx->state);
        return -1;
    }

    if (ctx->buffer) {
        ATCMD_TCPC_INFO("To free tcp client's recv buffer\r\n");
        ATCMD_FREE(ctx->buffer);
        ctx->buffer = NULL;
    }

    if (ctx->socket > -1) {
        ATCMD_TCPC_INFO("To close tcp client socket\r\n");
        close(ctx->socket);
        ctx->socket = -1;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (ctx->event) {
        ATCMD_TCPC_INFO("To delete event\n");
        vEventGroupDelete(ctx->event);
        ctx->event = NULL;
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    atcmd_tcpc_init_context(ctx);

    return 0;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcpc_init_config(const int cid, atcmd_tcpc_config *conf, atcmd_tcpc_sess_info *sess_info)
{
    ATCMD_TCPC_INFO("Start\r\n");

    if (!conf || !sess_info) {
        ATCMD_TCPC_ERR("Invalid parameter\n");
        return -1;
    }

    conf->cid = cid;

    snprintf((char *)conf->task_name, (ATCMD_TCPC_MAX_TASK_NAME - 1), "%s_%d",
             ATCMD_TCPC_TASK_NAME, cid);

    conf->task_priority = ATCMD_TCPC_TASK_PRIORITY;

    conf->task_size = (ATCMD_TCPC_TASK_SIZE / 4);

    snprintf(conf->sock_name, (ATCMD_TCPC_MAX_SOCK_NAME - 1), "%s_%d", ATCMD_TCPC_SOCK_NAME, cid);

    conf->rx_buflen = ATCMD_TCPC_RECV_BUF_SIZE;

    conf->sess_info = sess_info;

    return 0;
}
#else
int atcmd_tcpc_init_config(const int cid, atcmd_tcpc_config *conf)
{
    ATCMD_TCPC_INFO("Start\r\n");

    DA16X_UNUSED_ARG(cid);

    snprintf((char *)conf->task_name, (ATCMD_TCPC_MAX_TASK_NAME - 1), "%s_%d", ATCMD_TCPC_TASK_NAME, cid);

    conf->task_priority = ATCMD_TCPC_TASK_PRIORITY;

    conf->task_size = (ATCMD_TCPC_TASK_SIZE / 4);

    conf->rx_buflen = ATCMD_TCPC_RECV_BUF_SIZE;

    return 0;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcpc_deinit_config(atcmd_tcpc_config *conf)
{
    ATCMD_TCPC_INFO("Start\r\n");

    memset(conf, 0x00, sizeof(atcmd_tcpc_config));

    return 0;
}

int atcmd_tcpc_set_local_addr(atcmd_tcpc_config *conf, char *ip, int port)
{
    ATCMD_TCPC_INFO("Start\r\n");

    if (ip) {
        //not implemented yet.
        ATCMD_TCPC_ERR("Not allowed to set local ip address\r\n");
        return -1;
    }

    //check range
    if (port < ATCMD_TCPC_MIN_PORT || port > ATCMD_TCPC_MAX_PORT) {
        ATCMD_TCPC_ERR("Invalid port(%d)\n", port);
        return -1;
    }

    conf->local_addr.sin_family = AF_INET;
    conf->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (port == 0) {
        port = (int)get_random_value_ushort_range(ATCMD_TCPC_DEF_PORT, ATCMD_TCPC_DEF_PORT * 2);
    }

    conf->local_addr.sin_port = htons(port);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    conf->sess_info->local_port = port;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    return 0;
}

int atcmd_tcpc_set_svr_addr(atcmd_tcpc_config *conf, char *ip, int port)
{
    int ret = 0;
    struct addrinfo hints, *addr_list = NULL;
    char str_port[16] = {0x00, };

    ATCMD_TCPC_INFO("Start\r\n");

    if (!ip) {
        ATCMD_TCPC_ERR("Invalid parameters\r\n");
        return -1;
    }

    //check range
    if (port <= ATCMD_TCPC_MIN_PORT || port > ATCMD_TCPC_MAX_PORT) {
        ATCMD_TCPC_ERR("Invalid port(%d)\n", port);
        return -1;
    }

    memset(&hints, 0x00, sizeof(struct addrinfo));

    if (!is_in_valid_ip_class(ip)) {
        hints.ai_family = AF_INET;	//IPv4 only
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        snprintf(str_port, sizeof(str_port), "%d", port);

        ret = getaddrinfo(ip, str_port, &hints, &addr_list);
        if ((ret != 0) || !addr_list) {
            ATCMD_DBG("Failed to get address info(%d)\r\n", ret);
            return -1;
        }

        //pick 1st address
        memcpy((struct sockaddr *)&conf->svr_addr, addr_list->ai_addr, sizeof(struct sockaddr));

        freeaddrinfo(addr_list);
    } else {
        conf->svr_addr.sin_addr.s_addr = inet_addr(ip);
    }

    conf->svr_addr.sin_family = AF_INET;
    conf->svr_addr.sin_port = htons(port);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    conf->sess_info->peer_port = port;
    strncpy(conf->sess_info->peer_ipaddr, ip, sizeof(conf->sess_info->peer_ipaddr));
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPC_INFO("TCP server(%d.%d.%d.%d:%d)\r\n",
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(conf->svr_addr.sin_port)));

    return 0;
}

int atcmd_tcpc_set_config(atcmd_tcpc_context *ctx, atcmd_tcpc_config *conf)
{
    ATCMD_TCPC_INFO("Start\r\n");

    if (strlen((const char *)(conf->task_name)) == 0) {
        ATCMD_TCPC_ERR("Invalid task name\r\n");
        return -1;
    }

    if (conf->task_priority == 0) {
        ATCMD_TCPC_ERR("Invalid task priority\r\n");
        return -1;
    }

    if (conf->task_size == 0) {
        ATCMD_TCPC_ERR("Invalid task size\r\n");
        return -1;
    }

    if (conf->local_addr.sin_family != AF_INET) {
        ATCMD_TCPC_ERR("Invalid local address\r\n");
        return -1;
    }

    if ((conf->svr_addr.sin_family != AF_INET)
            || (conf->svr_addr.sin_port == 0)
            || (conf->svr_addr.sin_addr.s_addr == 0)) {
        ATCMD_TCPC_ERR("Invalid tcp server address\r\n");
        return -1;
    }

    if (conf->rx_buflen == 0) {
        ATCMD_TCPC_ERR("Invalid recv buffer size\r\n");
        return -1;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (strlen((const char *)(conf->sock_name)) == 0) {
        ATCMD_TCPC_ERR("Invalid socket name\n");
        return -1;
    }

    ctx->event = xEventGroupCreate();
    if (ctx->event == NULL) {
        ATCMD_TCPC_INFO("Failed to create event\n");
        return -1;
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPC_INFO("*%-20s: %s(%ld)\r\n" // task name
                    "*%-20s: %d\r\n" // task priority
                    "*%-20s: %d\r\n" // task size
                    "*%-20s: %d\r\n" // rx buflen
                    "*%-20s: %d.%d.%d.%d:%d\r\n" // local ip address
                    "*%-20s: %d.%d.%d.%d:%d\r\n", // tcp server ip address
                    "Task Name", (char *)conf->task_name, strlen((const char *)conf->task_name),
                    "Task Priority", conf->task_priority,
                    "Task Size", conf->task_size,
                    "RX buffer size", conf->rx_buflen,
                    "Local IP address",
                    (ntohl(conf->local_addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(conf->local_addr.sin_port)),
                    "TCP Server IP address",
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(conf->svr_addr.sin_port)));

    ctx->conf = conf;

    return 0;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcpc_wait_for_ready(atcmd_tcpc_context *ctx)
{
    const int wait_time = 100;

    int cnt = 0;
    unsigned int events = 0x00;

    if (!ctx) {
        ATCMD_TCPC_ERR("Invalid parameter\n");
        return -1;
    }

    //Check tcp client connects
    for (cnt = 0 ; cnt < (ATCMD_TCPC_RECONN_COUNT + 1) ; cnt++) {
        if (ctx->event) {
            events = xEventGroupWaitBits(ctx->event, ATCMD_TCPC_EVT_ANY,
                                         pdTRUE, pdFALSE, wait_time);
            if (events & ATCMD_TCPC_EVT_CONNECTION) {
                ATCMD_TCPC_INFO("Got connection event\n");
                return 0;
            } else if (events & ATCMD_TCPC_EVT_CLOSED) {
                ATCMD_TCPC_INFO("Got close event\n");
                return -1;
            }
        } else {
            if (ctx->state == ATCMD_TCPC_STATE_CONNECTED) {
                return 0;
            } else if (ctx->state == ATCMD_TCPC_STATE_TERMINATED) {
                return -1;
            }

            vTaskDelay(wait_time);
        }
    }

    return -1;

}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcpc_start(atcmd_tcpc_context *ctx)
{
    int ret = 0;

    ATCMD_TCPC_INFO("Start\r\n");

    if (!ctx->conf) {
        ATCMD_TCPC_ERR("Invalid parameters\r\n");
        return -1;
    }

    if (ctx->state != ATCMD_TCPC_STATE_TERMINATED) {
        ATCMD_TCPC_ERR("TCP client is not terminated(%d)\r\n", ctx->state);
        return -1;
    }

    ctx->state = ATCMD_TCPC_STATE_DISCONNECTED;

    ret = xTaskCreate(atcmd_tcpc_task_entry,
                      (const char *)(ctx->conf->task_name),
                      ctx->conf->task_size,
                      (void *)ctx,
                      ctx->conf->task_priority,
                      &ctx->task_handler);
    if (ret != pdPASS) {
        ATCMD_DBG("Failed to create tcp client task(%d)\r\n", ret);
        ctx->state = ATCMD_TCPC_STATE_TERMINATED;
        return -1;
    }

    return 0;
}

int atcmd_tcpc_stop(atcmd_tcpc_context *ctx)
{
    const int wait_time = 100;
    const int max_cnt = 10;
    int cnt = 0;
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    unsigned int events = 0x00;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    if (ctx->state == ATCMD_TCPC_STATE_CONNECTED) {
        ATCMD_TCPC_INFO("Change tcp client state from %d to %d\r\n",
                        ctx->state, ATCMD_TCPC_STATE_REQ_TERMINATE);

        ctx->state = ATCMD_TCPC_STATE_REQ_TERMINATE;

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
            if (ctx->event) {
                events = xEventGroupWaitBits(ctx->event,
                                             ATCMD_TCPC_EVT_CLOSED,
                                             pdTRUE,
                                             pdFALSE,
                                             wait_time);
                if (events & ATCMD_TCPC_EVT_CLOSED) {
                    ATCMD_TCPC_INFO("Closed tcp client task\n");
                    break;
                }
            } else {
                if (ctx->state == ATCMD_TCPC_STATE_TERMINATED) {
                    ATCMD_TCPC_INFO("Closed tcp client task\n");
                    break;
                }

                vTaskDelay(wait_time);
            }
#else
            if (ctx->state == ATCMD_TCPC_STATE_TERMINATED) {
                break;
            }

            vTaskDelay(wait_time);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

            ATCMD_TCPC_INFO("Waiting for closing task of tcp client(%d,%d,%d)\n", cnt, max_cnt, wait_time);
        }
    }

    return ((ctx->state == ATCMD_TCPC_STATE_TERMINATED) ? 0 : -1);
}

int atcmd_tcpc_tx(atcmd_tcpc_context *ctx, char *data, unsigned int data_len)
{
    int ret = 0;

    ATCMD_TCPC_INFO("Start\r\n");

    if ((data_len > TX_PAYLOAD_MAX_SIZE) || (ctx->state != ATCMD_TCPC_STATE_CONNECTED)) {
        ATCMD_TCPC_ERR("Invalid parameter(data_len:%ld, state:%ld)\r\n", data_len, ctx->state);
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    ATCMD_TCPC_INFO("To send data(%ld)\r\n", data_len);

    ret = send(ctx->socket, data, data_len, 0);
    if (ret <= 0) {
        ATCMD_DBG("[%s] Failed to send TCP (%d)\r\n", __func__, ret);
        return AT_CMD_ERR_DATA_TX;
    }

    return AT_CMD_ERR_CMD_OK;
}

int atcmd_tcpc_tx_with_peer_info(atcmd_tcpc_context *ctx, char *ip, int port,
                                 char *data, unsigned int data_len)
{
    ATCMD_TCPC_INFO("Start\r\n");

    if (ctx == NULL || ctx->conf == NULL) {
        ATCMD_TCPC_ERR("Invalid context\n");
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    //check ip address
    if (ip == NULL) {
        ATCMD_TCPC_ERR("Invalid IP address\n");
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    } else if (strcmp("0.0.0.0", ip) == 0 || strcmp("0", ip) == 0) {
        ;
    } else if (!is_in_valid_ip_class(ip)) {
        ATCMD_TCPC_INFO("Not supported domain yet\n");
        return AT_CMD_ERR_NOT_SUPPORTED;
    } else if (ctx->conf->svr_addr.sin_addr.s_addr != inet_addr(ip)) {
        ATCMD_TCPC_ERR("Not matched IP address(%d.%d.%d.%d vs %s)\n",
                       (ntohl(ctx->conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                       (ntohl(ctx->conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                       (ntohl(ctx->conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                       (ntohl(ctx->conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                       ip);
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    //check port
    if (port < ATCMD_TCPC_MIN_PORT || port > ATCMD_TCPC_MAX_PORT) {
        ATCMD_TCPC_ERR("Invalid port(%d)\n", port);
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    } else if (((strcmp("0.0.0.0", ip) == 0) || (strcmp("0", ip) == 0)) && (port == 0)) {
        ;
    } else if (ctx->conf->svr_addr.sin_port != htons(port)) {
        ATCMD_TCPC_ERR("Not matched port(%d vs %d)\n",
                       ntohs(ctx->conf->svr_addr.sin_port), port);
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    return atcmd_tcpc_tx(ctx, data, data_len);
}

static void atcmd_tcpc_task_entry(void *param)
{
    int sys_wdog_id = -1;
    int ret = 0;
    atcmd_tcpc_context *ctx = (atcmd_tcpc_context *)param;
    const atcmd_tcpc_config *conf = ctx->conf;

    unsigned int local_port = 0;

    fd_set writablefds;
    int sockfd_flags_before;
    struct timeval conn_timeout;

    //socket option
    int sockopt_reuse = 1;
    struct timeval sockopt_timeout;

    //releated with recv data
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    const int cid = conf->cid;
#else
    const int cid = ID_TC;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    unsigned char *hdr = NULL;
    size_t hdr_len = 0;
    size_t tot_len = 0;
    size_t act_hdr_len = 0;
    size_t act_payload_len = 0;

    unsigned char *payload = NULL;
    size_t payload_len = 0;

    //name
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    const char *dpm_name = (const char *)conf->task_name;
    const char *sock_name = (const char *)conf->sock_name;
#else
    const char *dpm_name = (const char *)ATCMD_TCPC_DPM_NAME;
    const char *sock_name = (const char *)ATCMD_TCPC_DPM_NAME;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPC_INFO("Start\r\n");

    ctx->state = ATCMD_TCPC_STATE_DISCONNECTED;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    ctx->buffer = ATCMD_MALLOC(conf->rx_buflen);
    if (!ctx->buffer) {
        ATCMD_DBG("[%s] No FREE memory space for rx buffer(%ld)\r\n", __func__, conf->rx_buflen);
        goto atcmd_tcpc_term;
    }

    local_port = ntohs(conf->local_addr.sin_port);

    dpm_app_register((char *)dpm_name, local_port);

    dpm_tcp_port_filter_set(local_port);

    ATCMD_TCPC_INFO("Reg - DPM Name:%s(%ld), Local port(%d)\n",
                    dpm_name, strlen(dpm_name), local_port);

    ctx->socket = socket_dpm((char *)sock_name, PF_INET, SOCK_STREAM, 0);
    if (ctx->socket < 0) {
        ATCMD_DBG("Failed to create socket of tcp client(%d:%d)\r\n", ctx->socket, errno);
        goto atcmd_tcpc_term;
    }

    ATCMD_TCPC_INFO("TCP Client: socket descriptor(%d)\r\n", ctx->socket);

    ret = setsockopt(ctx->socket, SOL_SOCKET, SO_REUSEADDR, &sockopt_reuse, sizeof(sockopt_reuse));
    if (ret != 0) {
        ATCMD_DBG("Failed to set socket option - SO_REUSEADDR(%d)\r\n", ret);
    }

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = ATCMD_TCPC_RECV_TIMEOUT * 1000;

    ret = setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (ret != 0) {
        ATCMD_DBG("Failed to set socket option - SO_RCVTIMEOUT(%d:%d)\r\n", ret, errno);
    }

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = ATCMD_TCPC_SEND_TIMEOUT * 1000;

    if (setsockopt(ctx->socket, SOL_SOCKET, SO_SNDTIMEO, &sockopt_timeout, sizeof(sockopt_timeout))) {
        ATCMD_DBG("Failed to set socket option - SO_SNDTIMEO(%d)\n");
    }

    ret = bind(ctx->socket, (struct sockaddr *)&conf->local_addr, sizeof(struct sockaddr_in));
    if (ret != 0) {
        ATCMD_DBG("Failed to bind socket of tcp client(%d:%d)\r\n", ret, errno);
        goto atcmd_tcpc_term;
    }

    sockfd_flags_before = fcntl(ctx->socket, F_GETFL, 0);
    fcntl(ctx->socket, F_SETFL, sockfd_flags_before | O_NONBLOCK);

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    ret = connect(ctx->socket, (struct sockaddr *)&conf->svr_addr, sizeof(struct sockaddr_in));

    if (ret == 0) {
        ATCMD_TCPC_INFO("Connected, ret=0\r\n");
        fcntl(ctx->socket, F_SETFL, sockfd_flags_before);
    } else if ( ret == -1 && errno == EINPROGRESS) {
        ATCMD_TCPC_INFO("connection in progress, ret=%d, errno=%d\r\n", ret, errno);

        FD_ZERO(&writablefds);
        FD_SET(ctx->socket, &writablefds);
        conn_timeout.tv_sec = ATCMD_TCPC_RECONNECT_TIMEOUT;
        conn_timeout.tv_usec = 0;

        ret = select(ctx->socket + 1, NULL, &writablefds, NULL, &conn_timeout);

        if (ret == -1) {
            ATCMD_TCPC_INFO("Select failed !\r\n");
            goto atcmd_tcpc_term;
        } else if (ret == 0) {
            ATCMD_TCPC_INFO("Select timeout !\r\n");
            goto atcmd_tcpc_term;
        }

        // Socket selected for write ...

        int sock_error;
        socklen_t len = sizeof(sock_error);

        ret = getsockopt(ctx->socket, SOL_SOCKET, SO_ERROR, &sock_error, &len);

        if (ret < 0) {
            ATCMD_DBG("Error getsockopt (%d:%d)\r\n", ret, errno);
            goto atcmd_tcpc_term;
        }

        // getsockopt success !!

        if (sock_error == 0) {
            ATCMD_TCPC_INFO("Connected\r\n");
            fcntl(ctx->socket, F_SETFL, sockfd_flags_before);
        } else {
            ATCMD_DBG("Error in non-blocking connection (sock_error=%d:%d)\r\n",
                      sock_error, errno);
            goto atcmd_tcpc_term;
        }
    } else {
        // Non EINPROGRESS error!
        ATCMD_DBG("Error in connection (%d:%d)\r\n", ret, errno);
        goto atcmd_tcpc_term;
    }

    ctx->state = ATCMD_TCPC_STATE_CONNECTED;

    dpm_app_wakeup_done((char *)dpm_name);

    dpm_app_data_rcv_ready_set((char *)dpm_name);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (ctx->event) {
        xEventGroupSetBits(ctx->event, ATCMD_TCPC_EVT_CONNECTION);
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    ATCMD_TCPC_INFO("Ready to receive packet(cid:%d)\n", cid);

    while (ctx->state == ATCMD_TCPC_STATE_CONNECTED) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        memset(ctx->buffer, 0x00, conf->rx_buflen);

        tot_len = 0;
        act_hdr_len = 0;
        act_payload_len = 0;

        hdr = ctx->buffer;
        hdr_len = ATCMD_TCPC_RECV_HDR_SIZE;

        payload = ctx->buffer + hdr_len;
        payload_len = conf->rx_buflen - ATCMD_TCPC_RECV_HDR_SIZE;

        //ATCMD_TCPC_INFO("Waiting to receive packet(cid:%d)\n", cid);
        
        da16x_sys_watchdog_suspend(sys_wdog_id);

        ret = recv(ctx->socket, payload, payload_len, 0);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        dpm_app_sleep_ready_clear((char *)dpm_name);

        if (ret > 0) {
            act_payload_len = ret;

            ATCMD_TCPC_INFO("Recv(ip:%d.%d.%d.%d:%d/len:%ld)\r\n",
                            (ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(conf->svr_addr.sin_port)), act_payload_len);

#if defined(__SUPPORT_TCP_RECVDATA_HEX_MODE__)
            if (is_tcpc_data_mode_hexstring()) {
                char *hex_buf = ATCMD_MALLOC(ATCMD_TCPC_RECV_HDR_SIZE + (act_payload_len * 2));
                if (!hex_buf) {
                    ATCMD_TCPC_ERR("Failed to allocate buffer to pass recv data(%ld)\n",
                                   ATCMD_TCPC_RECV_HDR_SIZE + (act_payload_len * 2));
                    goto atcmd_tcpc_term;
                }

                act_hdr_len = snprintf((char *)hex_buf, ATCMD_TCPC_RECV_HDR_SIZE,
                                       "\r\n" ATCMD_TCPC_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", cid,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                                       (ntohs(conf->svr_addr.sin_port)), (act_payload_len * 2));

                tot_len = act_hdr_len;
                Convert_Str2HexStr((unsigned char *)payload, hex_buf + tot_len, (u32_t)(act_payload_len));

                tot_len += act_payload_len * 2;                
                memcpy(hex_buf + tot_len, "\r\n", 2);

                tot_len += 2;
                hex_buf[tot_len] = '\0';

                PUTS_ATCMD(hex_buf, tot_len);

                ATCMD_FREE(hex_buf);
                hex_buf = NULL;
            } else
#endif // __SUPPORT_TCP_RECVDATA_HEX_MODE__
            {
                act_hdr_len = snprintf((char *)hdr, hdr_len,
                                       "\r\n" ATCMD_TCPC_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", cid,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                                       (int)(ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                                       (ntohs(conf->svr_addr.sin_port)), act_payload_len);

                tot_len = act_hdr_len;

                /* For data integrity, memmove() can't be replaced to any other function */
                if (!memmove(hdr + tot_len, payload, act_payload_len)) {
                    ATCMD_TCPC_ERR("Failed to copy received data(%ld)\n", act_payload_len);
                }

                tot_len += act_payload_len;
                memcpy(hdr + tot_len, "\r\n", 2);

                tot_len += 2;
                hdr[tot_len] = '\0';

                PUTS_ATCMD((char *)hdr, tot_len);
            }
        } else {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
                ATCMD_TCPC_INFO("Disconnected(ip:%d.%d.%d.%d:%d/errno:%d)\r\n",
                                (ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                                (ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                                (ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                                (ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                                (ntohs(conf->svr_addr.sin_port)), errno);

                ATCMD_DBG("[%s] TCP Client disconnected from Server (%d)\n", __func__, ret);

                PRINTF_ATCMD("\r\n" ATCMD_TCPC_DISCONN_RX_PREFIX ":%d,%d.%d.%d.%d,%u\r\n", cid,
                             (ntohl(conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                             (ntohl(conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                             (ntohl(conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                             (ntohl(conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                             (ntohs(conf->svr_addr.sin_port)));

                goto atcmd_tcpc_term;
            }
        }

        if (!da16x_dpm_sock_is_remaining_rx_data(ctx->socket)) {
            dpm_app_sleep_ready_set((char *)dpm_name);
        }
    }

atcmd_tcpc_term:

    da16x_sys_watchdog_unregister(sys_wdog_id);

    close(ctx->socket);
    ctx->socket = -1;

    if (ctx->buffer) {
        ATCMD_FREE(ctx->buffer);
        ctx->buffer = NULL;
    }

    dpm_tcp_port_filter_delete(local_port);

    dpm_app_unregister((char *)dpm_name);

    ATCMD_TCPC_INFO("Unreg - DPM Name:%s(%ld), Local port(%d)\n", dpm_name, strlen(dpm_name), local_port);

    ctx->state = ATCMD_TCPC_STATE_TERMINATED;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (ctx->event) {
        xEventGroupSetBits(ctx->event, ATCMD_TCPC_EVT_CLOSED);
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPC_INFO("End of task(state:%d)\r\n", ctx->state);

    ctx->task_handler = NULL;
    vTaskDelete(NULL);

    return ;
}
#endif  // __SUPPORT_ATCMD__

/* EOF */
