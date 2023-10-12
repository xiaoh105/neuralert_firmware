/**
 ****************************************************************************************
 *
 * @file atcmd_tcp_server.c
 *
 * @brief AT-CMD TCP Server Socket Controller
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

#include "atcmd_tcp_server.h"
#include "da16x_sys_watchdog.h"

#if defined	(__SUPPORT_ATCMD__)

#undef ENABLE_ATCMD_TCPS_DBG_INFO
#undef ENABLE_ATCMD_TCPS_DBG_ERR

#define	ATCMD_TCPS_DBG	ATCMD_DBG

#if defined (ENABLE_ATCMD_TCPS_DBG_INFO)
#define	ATCMD_TCPS_INFO(fmt, ...)	\
    ATCMD_TCPS_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_TCPS_INFO(...)	do {} while (0)
#endif	// (ENABLE_ATCMD_TCPS_DBG_INFO)

#if defined (ENABLE_ATCMD_TCPS_DBG_ERR)
#define	ATCMD_TCPS_ERR(fmt, ...)	\
    ATCMD_TCPS_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_TCPS_ERR(...)	do {} while (0)
#endif // (ENABLE_ATCMD_TCPS_DBG_ERR) 

#define ATCMD_TCPS_MIN_BACKLOG  3

static int atcmd_tcps_ready_to_accept_socket(atcmd_tcps_context *ctx);

static void atcmd_tcps_close_socket(int *sock);

static void atcmd_tcps_task_entry(void *param);

static atcmd_tcps_cli_context *atcmd_tcps_cli_create_context(atcmd_tcps_context *svr_ctx, int cli_sock,
        struct sockaddr_in *cli_addr);

static int atcmd_tcps_cli_delete_context(atcmd_tcps_cli_context **ctx);

static int atcmd_tcps_cli_start(atcmd_tcps_cli_context *ctx);

static int atcmd_tcps_cli_stop(atcmd_tcps_cli_context *ctx);

static int atcmd_tcps_cli_add(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx);

#if defined (__ENABLE_UNUSED__)
static atcmd_tcps_cli_context *atcmd_tcps_cli_remove_by_ip(atcmd_tcps_context *svr_ctx, struct sockaddr_in *ip_addr);
#endif // __ENABLE_UNUSED__

static atcmd_tcps_cli_context *atcmd_tcps_cli_remove_by_ctx(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx);

static atcmd_tcps_cli_context *atcmd_tcps_cli_find_by_ip(atcmd_tcps_context *svr_ctx, struct sockaddr_in *ip_addr);

static int atcmd_tcps_cli_release(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx);

static void atcmd_tcps_cli_task_entry(void *param);

static int atcmd_tcps_compare_ip_addr(struct sockaddr_in *a, struct sockaddr_in *b);

#if defined(__SUPPORT_TCP_RECVDATA_HEX_MODE__)
static int tcps_data_mode = 0;

static int is_tcps_data_mode_hexstring(void)
{
    return tcps_data_mode == 1 ? 1 : 0;
}

void set_tcps_data_mode(int mode)
{
    /*
    	TCP_DATA_MODE_ASCII 	= 0
    	TCP_DATA_MODE_HEXSTRING = 1
    */
    tcps_data_mode = mode;
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
#endif

int atcmd_tcps_init_context(atcmd_tcps_context *ctx)
{
    ATCMD_TCPS_INFO("Start\n");

    memset(ctx, 0x00, sizeof(atcmd_tcps_context));

    ctx->socket = ATCMD_TCPS_INIT_SOCKET_FD;

    ctx->state = ATCMD_TCPS_STATE_TERMINATED;

    return 0;
}

int atcmd_tcps_deinit_context(atcmd_tcps_context *ctx)
{
    ATCMD_TCPS_INFO("Start\n");

    if (ctx->state != ATCMD_TCPS_STATE_TERMINATED) {
        ATCMD_TCPS_ERR("TCP server is not terminated(%d)\n", ctx->state);
        return -1;
    }

    atcmd_tcps_close_socket(&ctx->socket);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (ctx->event) {
        vEventGroupDelete(ctx->event);
        ctx->event = NULL;
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    atcmd_tcps_init_context(ctx);

    return 0;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcps_init_config(int cid, atcmd_tcps_config *conf, atcmd_tcps_sess_info *sess_info)
{
    ATCMD_TCPS_INFO("Start\n");

    if (!conf || !sess_info) {
        ATCMD_TCPS_ERR("Invalid parameter\n");
        return -1;
    }

    conf->cid = cid;

    conf->task_priority = ATCMD_TCPS_TASK_PRIORITY;

    conf->task_size = (ATCMD_TCPS_TASK_SIZE / 4);

    snprintf((char *)conf->task_name, (ATCMD_TCPS_MAX_TASK_NAME - 1), "%s_%d",
             ATCMD_TCPS_TASK_NAME, cid);

    snprintf(conf->sock_name, (ATCMD_TCPS_MAX_SOCK_NAME - 1), "%s_%d",
             ATCMD_TCPS_SOCK_NAME, cid);

    conf->sess_info = sess_info;

    conf->sess_info->max_allow_client = ATCMD_TCPS_MAX_SESS;

    conf->rx_buflen = ATCMD_TCPS_RECV_BUF_SIZE;

    return 0;
}
#else
int atcmd_tcps_init_config(int cid, atcmd_tcps_config *conf)
{
    ATCMD_TCPS_INFO("Start\n");

    DA16X_UNUSED_ARG(cid);

    conf->task_priority = ATCMD_TCPS_TASK_PRIORITY;

    conf->task_size = (ATCMD_TCPS_TASK_SIZE / 4);

    strcpy((char *)(conf->task_name), ATCMD_TCPS_TASK_NAME);

    conf->max_allow_client = ATCMD_TCPS_MAX_SESS;

    conf->rx_buflen = ATCMD_TCPS_RECV_BUF_SIZE;

    return 0;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcps_deinit_config(atcmd_tcps_config *conf)
{
    ATCMD_TCPS_INFO("Start\n");

    memset(conf, 0x00, sizeof(atcmd_tcps_config));

    return 0;
}

int atcmd_tcps_set_local_addr(atcmd_tcps_config *conf, char *ip, int port)
{
    ATCMD_TCPS_INFO("Start\n");

    if (ip) {
        //not implemented yet.
        ATCMD_TCPS_ERR("Not allowed to set local ip address\n");
        return -1;
    }

    //check range
    if (port < ATCMD_TCPS_MIN_PORT || port > ATCMD_TCPS_MAX_PORT) {
        ATCMD_TCPS_ERR("Invalid port(%d)\n", port);
        return -1;
    }

    conf->local_addr.sin_family = AF_INET;
    conf->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    conf->local_addr.sin_port = htons(port);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    conf->sess_info->local_port = port;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    return 0;
}

int atcmd_tcps_set_max_allowed_client(atcmd_tcps_config *conf, int max_allowed_client)
{
    ATCMD_TCPS_INFO("Start\n");

    if (max_allowed_client < 1 || max_allowed_client > ATCMD_TCPS_MAX_SESS) {
        ATCMD_TCPS_ERR("Invalid max_allowed_client(%d)\n", max_allowed_client);
        return -1;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    conf->sess_info->max_allow_client = max_allowed_client;
#else
    conf->max_allow_client = max_allowed_client;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("Set max_allow_client(%d)\n", max_allowed_client);

    return 0;
}

int atcmd_tcps_set_config(atcmd_tcps_context *ctx, atcmd_tcps_config *conf)
{
    ATCMD_TCPS_INFO("Start\n");

    if (strlen((const char *)(conf->task_name)) == 0) {
        ATCMD_TCPS_ERR("Invalid task name\n");
        return -1;
    }

    if (conf->task_priority == 0) {
        ATCMD_TCPS_ERR("Invalid task priority\n");
        return -1;
    }

    if (conf->task_size == 0) {
        ATCMD_TCPS_ERR("Invalid task size\n");
        return -1;
    }

    if (conf->rx_buflen == 0) {
        ATCMD_TCPS_ERR("Invalid recv buffer size\n");
        return -1;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (conf->sess_info->max_allow_client == 0) {
        ATCMD_TCPS_ERR("Invalid max client\n");
        return -1;
    }
#else
    if (conf->max_allow_client == 0) {
        ATCMD_TCPS_ERR("Invalid max client\n");
        return -1;
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    if ((conf->local_addr.sin_family != AF_INET) || (conf->local_addr.sin_port == 0)) {
        ATCMD_TCPS_ERR("Invali local address\n");
        return -1;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (strlen((const char *)(conf->sock_name)) == 0) {
        ATCMD_TCPS_ERR("Invalid socket name\n");
        return -1;
    }

    ctx->event = xEventGroupCreate();
    if (ctx->event == NULL) {
        ATCMD_TCPS_INFO("Failed to create event\n");
        return -1;
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("*%-20s: %s(%ld)\n" // task name
                    "*%-20s: %d\n" // task priority
                    "*%-20s: %d\n" // task size
                    "*%-20s: %d\n" // rx buflen
                    "*%-20s: %d\n" // max allow clients
                    "*%-20s: %d.%d.%d.%d:%d\n", // local ip address
                    "Task Name", (char *)conf->task_name, strlen((const char *)conf->task_name),
                    "Task Priority", conf->task_priority,
                    "Task Size", conf->task_size,
                    "RX buffer size", conf->rx_buflen,
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
                    "Max allow connection", conf->sess_info->max_allow_client,
#else
                    "Max allow connection", conf->max_allow_client,
#endif // __SUPPORT_ATCMD_MULTI_SESSION__
                    "IP Address",
                    (ntohl(conf->local_addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(conf->local_addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(conf->local_addr.sin_port)));

    ctx->conf = conf;

    return 0;
}

int atcmd_tcps_start(atcmd_tcps_context *ctx)
{
    int ret = 0;

    ATCMD_TCPS_INFO("Start\n");

    if (!ctx || !ctx->conf) {
        ATCMD_TCPS_ERR("Invalid parameters\n");
        return -1;
    }

    if (ctx->state != ATCMD_TCPS_STATE_TERMINATED) {
        ATCMD_TCPS_ERR("TCP server is not terminated(%d)\n", ctx->state);
        return -1;
    }

    ctx->state = ATCMD_TCPS_STATE_READY;

    ret = xTaskCreate(atcmd_tcps_task_entry,
                      (const char *)(ctx->conf->task_name),
                      ctx->conf->task_size,
                      (void *)ctx,
                      ctx->conf->task_priority,
                      &ctx->task_handler);
    if (ret != pdPASS) {
        ATCMD_DBG("Failed to create tcp server task(%d)\n", ret);
        ctx->state = ATCMD_TCPS_STATE_TERMINATED;
        return -1;
    }

    return 0;
}

int atcmd_tcps_stop(atcmd_tcps_context *ctx)
{
    int ret = 0;

    const int wait_time = 100;
    const int max_cnt = 10;
    int cnt = 0;
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    unsigned int events = 0x00;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    atcmd_tcps_cli_context *cli_ctx = NULL;

    ATCMD_TCPS_INFO("Start\n");

    if (!ctx) {
        ATCMD_TCPS_ERR("Invalid parameters\n");
        return -1;
    }

    //Stop tcp server task
    if (ctx->state == ATCMD_TCPS_STATE_ACCEPT) {
        ATCMD_TCPS_INFO("Change tcp server state from %d to %d\n",
                        ctx->state, ATCMD_TCPS_STATE_REQ_TERMINATE);

        ctx->state = ATCMD_TCPS_STATE_REQ_TERMINATE;

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
            if (ctx->event) {
                events = xEventGroupWaitBits(ctx->event, ATCMD_TCPS_EVT_CLOSED,
                                             pdTRUE, pdFALSE, wait_time);
                if (events & ATCMD_TCPS_EVT_CLOSED) {
                    ATCMD_TCPS_INFO("Closed tcp server task\n");
                    break;
                }
            } else {
                if (ctx->state == ATCMD_TCPS_STATE_TERMINATED) {
                    ATCMD_TCPS_INFO("Closed tcp server task\n");
                    break;
                }

                vTaskDelay(wait_time);
            }
#else
            if (ctx->state == ATCMD_TCPS_STATE_TERMINATED) {
                ATCMD_TCPS_INFO("Closed tcp server task\n");
                break;
            }

            vTaskDelay(wait_time);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

            ATCMD_TCPS_INFO("Waiting for closing task of tcp server(%d,%d,%d)\n",
                            cnt, max_cnt, wait_time);
        }
    }

    if (ctx->state != ATCMD_TCPS_STATE_TERMINATED) {
        ATCMD_DBG("Failed to stop tcp server task(%s,%d)\n",
                  ctx->conf->task_name, ctx->state);
        return -1;
    }

    //Stop tcp client task
    cli_ctx = ctx->cli_ctx;

    while (cli_ctx) {
        ret = atcmd_tcps_cli_release(ctx, cli_ctx);
        if (ret) {
            ATCMD_DBG("Failed to release TCP client(%d)\n", ret);
            break;
        }

        cli_ctx = ctx->cli_ctx;
    }

    return 0;
}

int atcmd_tcps_stop_cli(atcmd_tcps_context *ctx, const char *ip, const int port)
{
    int ret = AT_CMD_ERR_WRONG_ARGUMENTS;
    atcmd_tcps_cli_context *cli_ctx = NULL;
    struct sockaddr_in peer_addr = {0x00,};

    ATCMD_TCPS_INFO("Close peer session(%s:%d)\n", ip, port);

    //Convert ip address & port
    //inet_pton(AF_INET, ip_addr, &(peer.sin_addr));
    peer_addr.sin_addr.s_addr = inet_addr(ip);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    //Find client context
    cli_ctx = atcmd_tcps_cli_find_by_ip(ctx, &peer_addr);
    if (cli_ctx) {
        ret = atcmd_tcps_cli_release(ctx, cli_ctx);
        if (ret) {
            ATCMD_DBG("Failed to release TCP client(%d)\n", ret);
            return ret;
        }
    }

    return ret;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_tcps_wait_for_ready(atcmd_tcps_context *ctx)
{
    const int max_wait_cnt = 10;
    int wait_time = 10;
    int wait_cnt = 0;

    unsigned int events = 0x00;

    if (!ctx) {
        ATCMD_TCPS_ERR("Invalid parameter\n");
        return -1;
    }

    for (wait_cnt = 0 ; wait_cnt < max_wait_cnt ; wait_cnt++) {
        if (ctx->event) {
            events = xEventGroupWaitBits(ctx->event, ATCMD_TCPS_EVT_ANY,
                                         pdTRUE, pdFALSE, wait_time);
            if (events & ATCMD_TCPS_EVT_ACCEPT) {
                ATCMD_TCPS_INFO("Got accept event\n");
                break;
            } else if (events & ATCMD_TCPS_EVT_CLOSED) {
                ATCMD_TCPS_INFO("Got close event\n");
                return -1;
            }
        } else {
            if (ctx->state == ATCMD_TCPS_STATE_ACCEPT) {
                break;
            } else if (ctx->state == ATCMD_TCPS_STATE_TERMINATED) {
                return -1;
            }

            vTaskDelay(wait_time);
        }
    }

    return 0;

}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_tcps_tx(atcmd_tcps_context *ctx, char *data, unsigned int data_len, char *ip, unsigned int port)
{
    int ret = 0;
    atcmd_tcps_cli_context *cli_ctx = NULL;
    struct addrinfo hints, *addr_list = NULL;
    char str_port[16] = {0x00, };

    struct sockaddr_in cli_addr;

    ATCMD_TCPS_INFO("Start\n");

    if ((data_len > TX_PAYLOAD_MAX_SIZE)
            || (ctx->state != ATCMD_TCPS_STATE_ACCEPT)
            || (ctx->cli_cnt == 0)) {
        ATCMD_TCPS_ERR("Invalid parameters(data_len:%ld, state:%ld, cli_cnt:%d)\n",
                       data_len, ctx->state, ctx->cli_cnt);
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    memset(&hints, 0x00, sizeof(struct addrinfo));
    memset(&cli_addr, 0x00, sizeof(struct sockaddr_in));

    if (!is_in_valid_ip_class(ip)) {
        hints.ai_family = AF_INET;	//IPv4 only
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        snprintf(str_port, sizeof(str_port), "%d", port);

        ret = getaddrinfo(ip, str_port, &hints, &addr_list);
        if ((ret != 0) || !addr_list) {
            ATCMD_DBG("Failed to get address info(%d)\n", ret);
            return AT_CMD_ERR_IP_ADDRESS;
        }

        //pick 1st address
        memcpy((struct sockaddr *)&cli_addr, addr_list->ai_addr, sizeof(struct sockaddr));

        freeaddrinfo(addr_list);
    } else {
        cli_addr.sin_addr.s_addr = inet_addr(ip);
    }

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(port);

    ATCMD_TCPS_INFO("Client ip address: %d.%d.%d.%d:%d\n",
                    (ntohl(cli_addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(cli_addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(cli_addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(cli_addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(cli_addr.sin_port)));

    //find client context
    cli_ctx = atcmd_tcps_cli_find_by_ip(ctx, &cli_addr);
    if (!cli_ctx) {
        ATCMD_TCPS_ERR("Failed to find client ip address in list\n");
        return AT_CMD_ERR_UNKNOWN;
    }

    if (cli_ctx->state != ATCMD_TCPS_CLI_STATE_CONNECTED) {
        ATCMD_TCPS_ERR("Client is not connected(%d)\n", cli_ctx->state);
        return AT_CMD_ERR_NOT_CONNECTED;
    }

    ATCMD_TCPS_INFO("Send data(%ld)\n", data_len);

    ret = send(cli_ctx->socket, data, data_len, 0);
    if (ret <= 0) {
        ATCMD_DBG("Failed to send tcp data to tcp client(%d,%d.%d.%d.%d:%d)\n", ret,
                  (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                  (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                  (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                  (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                  (ntohs(cli_ctx->addr.sin_port)));

        return AT_CMD_ERR_DATA_TX;
    }

    return AT_CMD_ERR_CMD_OK;
}

static int atcmd_tcps_get_backlog(const atcmd_tcps_config *conf, int connected)
{
    int max_backlog = 0;
    int backlog = 0;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    max_backlog = conf->sess_info->max_allow_client;
#else
    max_backlog = conf->max_allow_client;
#endif

    if ((max_backlog > ATCMD_TCPS_MIN_BACKLOG) && ((max_backlog / 2) > connected)) {
        backlog = max_backlog / 2;
    }

    ATCMD_TCPS_INFO("Max:%d, Min:%d, Conn:%d, backlog:%d\n",
                    max_backlog, ATCMD_TCPS_MIN_BACKLOG, connected, backlog);

    return backlog;
}

static int atcmd_tcps_ready_to_accept_socket(atcmd_tcps_context *ctx)
{
    int ret = 0;
    const atcmd_tcps_config *conf = ctx->conf;

    //socket option
    int sockopt_reuse = 1;
    struct timeval sockopt_timeout = {0x00,};

    //socket information
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    const char *svr_sock_name = (const char *)conf->sock_name;
#else
    const char *svr_sock_name = (const char *)ATCMD_TCPS_SOCK_NAME;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("Start\n");

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = ATCMD_TCPS_RECV_TIMEOUT * 1000;

    if (ctx->socket >= 0) {
        ATCMD_TCPS_ERR("Already assigned socket fd(%d)\n", ctx->socket);
        return -1;
    }

    //create socket
    ret = socket_dpm((char *)svr_sock_name, PF_INET, SOCK_STREAM, 0);
    if (ret < 0) {
        ATCMD_TCPS_ERR("Failed to create socket of tcp server(%d)\n", ret);
        goto err;
    }

    ctx->socket = ret;

    ATCMD_TCPS_INFO("Created socket descriptor(%d)\n", ctx->socket);

    //set socket option
    ret = setsockopt(ctx->socket, SOL_SOCKET, SO_REUSEADDR, &sockopt_reuse, sizeof(sockopt_reuse));
    if (ret != 0) {
        ATCMD_DBG("Failed to set socket option - SO_REUSEADDR(%d)\n", ret);
    }

    ret = setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (ret != 0) {
        ATCMD_DBG("Failed to set socket option - SO_RCVTIMEOUT(%d)\n", ret);
    }

    ATCMD_TCPS_INFO("bind socket descriptor(%d)\n", ctx->socket);

    //bind socket
    ret = bind(ctx->socket, (struct sockaddr *)&conf->local_addr, sizeof(struct sockaddr_in));
    if (ret != 0) {
        ATCMD_DBG("Failed to bind socket of tcp server(%d)\n", ret);
        goto err;
    }

    ATCMD_TCPS_INFO("listen socket descriptor(%d)\n", ctx->socket);

    //listen socket
    ret = listen(ctx->socket, atcmd_tcps_get_backlog(conf, 0));
    if (ret != 0) {
        ATCMD_DBG("Failed to listen socket of tcp server(%d)\n", ret);
        goto err;
    }

    return 0;

err:

    atcmd_tcps_close_socket(&ctx->socket);

    return ret;
}

static void atcmd_tcps_close_socket(int *sock)
{
    if (sock) {
        if (*sock != ATCMD_TCPS_INIT_SOCKET_FD) {
            ATCMD_TCPS_INFO("To close socket(%d)\n", *sock);
            close(*sock);
        }
        *sock = ATCMD_TCPS_INIT_SOCKET_FD;
    }

    return;
}

static void atcmd_tcps_task_entry(void *param)
{
    int sys_wdog_id = -1;
    int ret = 0;
    atcmd_tcps_context *svr_ctx = (atcmd_tcps_context *)param;
    const atcmd_tcps_config *svr_conf = svr_ctx->conf;

    unsigned int local_port = 0;

    //tcp client(sub)
    int cli_sock = ATCMD_TCPS_INIT_SOCKET_FD;
    struct sockaddr_in cli_addr;
    int cli_addrlen = sizeof(struct sockaddr_in);
    atcmd_tcps_cli_context *cli_ctx = NULL;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    const int max_allow_client = svr_conf->sess_info->max_allow_client;
    const char *svr_dpm_name = (const char *)svr_conf->task_name;
#else
    const int max_allow_client = svr_conf->max_allow_client;
    const char *svr_dpm_name = (const char *)ATCMD_TCPS_DPM_NAME;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("Start\n");

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    if (!svr_ctx || !svr_conf) {
        ATCMD_DBG("[%s] Invalid param\n", __func__);
        goto end;
    }

    svr_ctx->mutex = xSemaphoreCreateMutex();
    if (!svr_ctx->mutex) {
        ATCMD_DBG("Failed to create mutex to manage tcp client task\n");
        goto end;
    }

    local_port = ntohs(svr_conf->local_addr.sin_port);

    dpm_app_register((char *)svr_dpm_name, local_port);

    dpm_tcp_port_filter_set(local_port);

    ATCMD_TCPS_INFO("Reg - DPM Name:%s(%ld), Local port(%d)\n",
                    svr_dpm_name, strlen(svr_dpm_name), local_port);

    ret = atcmd_tcps_ready_to_accept_socket(svr_ctx);
    if (ret) {
        ATCMD_DBG("Failed to create socket of tcp server(%d)\n", ret);
        goto end;
    }

    svr_ctx->state = ATCMD_TCPS_STATE_ACCEPT;

    dpm_app_wakeup_done((char *)svr_dpm_name);

    dpm_app_data_rcv_ready_set((char *)svr_dpm_name);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (svr_ctx->event) {
        xEventGroupSetBits(svr_ctx->event, ATCMD_TCPS_EVT_ACCEPT);
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    while (svr_ctx->state == ATCMD_TCPS_STATE_ACCEPT) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        cli_sock = ATCMD_TCPS_INIT_SOCKET_FD;
        memset(&cli_addr, 0x00, sizeof(struct sockaddr_in));
        cli_addrlen = sizeof(struct sockaddr_in);

        cli_ctx = NULL;

        //set backlog again.
        listen(svr_ctx->socket, atcmd_tcps_get_backlog(svr_conf, svr_ctx->cli_cnt));

        da16x_sys_watchdog_suspend(sys_wdog_id);

        //ATCMD_TCPS_INFO("Waiting for client connects(connected clients:%ld)\n", svr_ctx->cli_cnt);
        cli_sock = accept(svr_ctx->socket, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        dpm_app_sleep_ready_clear((char *)svr_dpm_name);

        if (cli_sock < 0) {
            //check cli state
            cli_ctx = svr_ctx->cli_ctx;

            while (cli_ctx) {
                if ((cli_ctx->state != ATCMD_TCPS_CLI_STATE_READY)
                        && (cli_ctx->state != ATCMD_TCPS_CLI_STATE_CONNECTED)) {
                    ATCMD_TCPS_INFO("To remove tcp client(%d.%d.%d.%d:%d), "
                                    "state(%d), connected client(%d)\n",
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                                    ntohs(cli_ctx->addr.sin_port), cli_ctx->state,
                                    svr_ctx->cli_cnt);

                    ret = atcmd_tcps_cli_release(svr_ctx, cli_ctx);
                    if (ret) {
                        ATCMD_TCPS_ERR("Failed to release TCP client(%d)\n", ret);
                        break;
                    }

                    cli_ctx = svr_ctx->cli_ctx;
                } else {
                    cli_ctx = cli_ctx->next;
                }
            }

#if 0 //def ENABLE_ATCMD_TCPS_DBG_INFO
            if (svr_ctx->cli_ctx) {
                for (cli_ctx = svr_ctx->cli_ctx ; cli_ctx != NULL ; cli_ctx = cli_ctx->next) {
                    ATCMD_TCPS_INFO("connecte tcp client(%d.%d.%d.%d:%d), state(%d)\n",
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                                    (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                                    ntohs(cli_ctx->addr.sin_port), cli_ctx->state);
                }
            } else {
                ATCMD_TCPS_INFO("svr_ctx->cli_ctx is null\n");
            }

            ATCMD_TCPS_INFO("svr_ctx->cli_cnt: %ld\n", svr_ctx->cli_cnt);
#endif // ENABLE_ATCMD_TCPS_DBG_INFO

            dpm_app_sleep_ready_set((char *)svr_dpm_name);

            continue;
        }

        ATCMD_TCPS_INFO("Connected %d.client(%d.%d.%d.%d:%d), current connected(%d + 1)\n",
                        cli_sock,
                        (ntohl(cli_addr.sin_addr.s_addr) >> 24) & 0xFF,
                        (ntohl(cli_addr.sin_addr.s_addr) >> 16) & 0xFF,
                        (ntohl(cli_addr.sin_addr.s_addr) >>  8) & 0xFF,
                        (ntohl(cli_addr.sin_addr.s_addr)      ) & 0xFF,
                        (ntohs(cli_addr.sin_port)),
                        svr_ctx->cli_cnt);

        if (max_allow_client <= svr_ctx->cli_cnt) {
            ATCMD_DBG("Client accept full(max client count:%d)\n", max_allow_client);

            atcmd_tcps_close_socket(&cli_sock);
            continue;
        }

        //Check duplication
        for (cli_ctx = svr_ctx->cli_ctx ; cli_ctx != NULL ; cli_ctx = cli_ctx->next) {
            if (atcmd_tcps_compare_ip_addr(&cli_ctx->addr, &cli_addr) == 0) {
                ATCMD_DBG("Duplicated client(%d.%d.%d.%d:%d)\n",
                          (ntohl(cli_addr.sin_addr.s_addr) >> 24) & 0x0ff,
                          (ntohl(cli_addr.sin_addr.s_addr) >> 16) & 0x0ff,
                          (ntohl(cli_addr.sin_addr.s_addr) >>  8) & 0x0ff,
                          (ntohl(cli_addr.sin_addr.s_addr)      ) & 0x0ff,
                          ntohs(cli_addr.sin_port));

                atcmd_tcps_close_socket(&cli_sock);
                break;
            }
        }

        if (cli_sock < 0) {
            continue;
        }

        cli_ctx = atcmd_tcps_cli_create_context(svr_ctx, cli_sock, &cli_addr);
        if (!cli_ctx) {
            ATCMD_DBG("Failed to create client context(%d.%d.%d.%d:%d)\n",
                      (ntohl(cli_addr.sin_addr.s_addr) >> 24) & 0x0ff,
                      (ntohl(cli_addr.sin_addr.s_addr) >> 16) & 0x0ff,
                      (ntohl(cli_addr.sin_addr.s_addr) >>  8) & 0x0ff,
                      (ntohl(cli_addr.sin_addr.s_addr)      ) & 0x0ff,
                      ntohs(cli_addr.sin_port));

            atcmd_tcps_close_socket(&cli_sock);
            continue;
        }

        ret = atcmd_tcps_cli_add(svr_ctx, cli_ctx);
        if (ret != 0) {
            ATCMD_DBG("Failed to add client context(%d)\n", ret);

            atcmd_tcps_cli_delete_context(&cli_ctx);
            atcmd_tcps_close_socket(&cli_sock);
            continue;
        }

        ret = atcmd_tcps_cli_start(cli_ctx);
        if (ret != 0) {
            ATCMD_DBG("Failed to start tcp client(%d)\n", ret);

            atcmd_tcps_cli_remove_by_ctx(svr_ctx, cli_ctx);

            atcmd_tcps_cli_delete_context(&cli_ctx);

            atcmd_tcps_close_socket(&cli_sock);

            continue;
        }
    }

end:

    da16x_sys_watchdog_unregister(sys_wdog_id);

    atcmd_tcps_close_socket(&svr_ctx->socket);

    dpm_tcp_port_filter_delete(local_port);

    dpm_app_unregister((char *)svr_dpm_name);

    ATCMD_TCPS_INFO("Unreg - DPM Name:%s(%ld), Local port(%d)\n",
                    svr_dpm_name, strlen(svr_dpm_name), local_port);

    if (svr_ctx->mutex) {
        vSemaphoreDelete(svr_ctx->mutex);
        svr_ctx->mutex = NULL;
    }

    svr_ctx->state = ATCMD_TCPS_STATE_TERMINATED;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (svr_ctx->event) {
        xEventGroupSetBits(svr_ctx->event, ATCMD_TCPS_EVT_CLOSED);
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("End of task(%d)\n", svr_ctx->state);

    svr_ctx->task_handler = NULL;
    vTaskDelete(NULL);

    return ;
}

static atcmd_tcps_cli_context *atcmd_tcps_cli_create_context(atcmd_tcps_context *svr_ctx, int cli_sock,
        struct sockaddr_in *cli_addr)
{
    static int cli_idx = 0;
    atcmd_tcps_cli_context *cli_ctx = NULL;

    struct timeval sockopt_timeout;

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = ATCMD_TCPS_RECV_TIMEOUT * 1000;

    ATCMD_TCPS_INFO("Start\n");

    if (setsockopt(cli_sock, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout))) {
        ATCMD_DBG("Failed to set socket option - SO_RCVTIMEOUT(%d)\n");
        return NULL;
    }

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = ATCMD_TCPS_SEND_TIMEOUT * 1000;

    if (setsockopt(cli_sock, SOL_SOCKET, SO_SNDTIMEO, &sockopt_timeout, sizeof(sockopt_timeout))) {
        ATCMD_DBG("Failed to set socket option - SO_SNDTIMEO(%d)\n");
        return NULL;
    }

    cli_ctx = ATCMD_MALLOC(sizeof(atcmd_tcps_cli_context));
    if (!cli_ctx) {
        ATCMD_DBG("Failed to allocate memory for tcp client(%ld)\n",
                  sizeof(atcmd_tcps_cli_context));
        return NULL;
    }

    memset(cli_ctx, 0x00, sizeof(atcmd_tcps_cli_context));

    cli_ctx->buffer_len = svr_ctx->conf->rx_buflen;

    cli_ctx->buffer = ATCMD_MALLOC(cli_ctx->buffer_len);
    if (!cli_ctx->buffer) {
        ATCMD_DBG("Failed to allocate memory for tcp client's rx buffer(%ld)\n",
                  cli_ctx->buffer_len);
        goto err;
    }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    cli_ctx->event = xEventGroupCreate();
    if (cli_ctx->event == NULL) {
        ATCMD_TCPS_ERR("Failed to create event group to close tcp client's task\n");
        goto err;
    }

    //cid
    cli_ctx->cid = svr_ctx->conf->cid;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    //task name
    snprintf((char *)(cli_ctx->task_name), (ATCMD_TCPS_MAX_TASK_NAME - 1), "%s_%d",
             ATCMD_TCPS_CLI_TASK_NAME, cli_idx++);

    //task priority
    cli_ctx->task_priority = ATCMD_TCPS_CLI_TASK_PRIORITY;

    //task size
    cli_ctx->task_size = ATCMD_TCPS_CLI_TASK_SIZE;

    //state
    cli_ctx->state = ATCMD_TCPS_CLI_STATE_TERMINATED;

    //socket
    cli_ctx->socket = cli_sock;

    //address
    memcpy(&cli_ctx->addr, cli_addr, sizeof(struct sockaddr_in));

    cli_ctx->svr_ptr = (void *)svr_ctx;

    ATCMD_TCPS_INFO("*%-20s: %s(%ld)\n" // task name
                    "*%-20s: %d\n" // task priority
                    "*%-20s: %d\n" // task size
                    "*%-20s: %d\n" // rx buflen
                    "*%-20s: %d.%d.%d.%d:%d\n", // local ip address
                    "Task Name", cli_ctx->task_name, strlen((const char *)cli_ctx->task_name),
                    "Task Priority", cli_ctx->task_priority,
                    "Task Size", cli_ctx->task_size,
                    "RX buffer size", cli_ctx->buffer_len,
                    "IP Address",
                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(cli_ctx->addr.sin_port)));

    return cli_ctx;

err:

    if (cli_ctx) {
        atcmd_tcps_cli_delete_context(&cli_ctx);
    }

    return NULL;
}

static int atcmd_tcps_cli_delete_context(atcmd_tcps_cli_context **ctx)
{
    ATCMD_TCPS_INFO("Start\n");

    atcmd_tcps_cli_context *ptr = (atcmd_tcps_cli_context *)*ctx;

    if (ptr) {
        if (ptr->buffer) {
            ATCMD_FREE(ptr->buffer);
        }

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
        if (ptr->event) {
            vEventGroupDelete(ptr->event);
        }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

        ATCMD_FREE(ptr);
    }

    *ctx = NULL;

    return 0;
}

static int atcmd_tcps_cli_start(atcmd_tcps_cli_context *ctx)
{
    int ret = 0;

    ATCMD_TCPS_INFO("Start\n");

    if (!ctx || (ctx->state != ATCMD_TCPS_CLI_STATE_TERMINATED)) {
        ATCMD_TCPS_ERR("Invalid parameter\n");
        return -1;
    }

    ctx->state = ATCMD_TCPS_CLI_STATE_READY;

    ret = xTaskCreate(atcmd_tcps_cli_task_entry,
                      (const char *)(ctx->task_name),
                      ctx->task_size,
                      (void *)ctx,
                      ctx->task_priority,
                      &ctx->task_handler);
    if (ret != pdPASS) {
        ATCMD_DBG("Failed to create tcp client taks(%d)\n", ret);
        return -1;
    }

    return 0;
}

static int atcmd_tcps_cli_stop(atcmd_tcps_cli_context *ctx)
{
    const int wait_time = 100;
    const int max_cnt = 10;
    int cnt = 0;
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    unsigned int events = 0x00;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("Start\n");

    if (!ctx) {
        ATCMD_TCPS_ERR("Invalid parameter\n");
        return -1;
    }

    if ((ctx->state == ATCMD_TCPS_CLI_STATE_CONNECTED)
            || (ctx->state == ATCMD_TCPS_CLI_STATE_REQ_TERMINATE)) {
        ATCMD_TCPS_INFO("Change tcp client state from %d to %d\n",
                        ctx->state, ATCMD_TCPS_CLI_STATE_REQ_TERMINATE);

        ctx->state = ATCMD_TCPS_CLI_STATE_REQ_TERMINATE;

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
            if (ctx->event) {
                events = xEventGroupWaitBits(ctx->event, ATCMD_TCPS_EVT_CLOSED,
                                             pdTRUE, pdFALSE, wait_time);
                if (events & ATCMD_TCPS_EVT_CLOSED) {
                    ATCMD_TCPS_INFO("Closed tcp client task\n");
                    break;
                }
            } else {
                if (ctx->state == ATCMD_TCPS_CLI_STATE_TERMINATED) {
                    ATCMD_TCPS_INFO("Closed tcp client task\n");
                    break;
                }

                vTaskDelay(wait_time);
            }
#else
            if (ctx->state == ATCMD_TCPS_CLI_STATE_TERMINATED) {
                ATCMD_TCPS_INFO("Closed tcp client task\n");
                break;
            }

            vTaskDelay(wait_time);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__
        }
    }

    if (ctx->state != ATCMD_TCPS_CLI_STATE_TERMINATED) {
        ATCMD_TCPS_ERR("Failed to stop tcp client task(%d)\n", ctx->state);
        return -1;
    }

    return 0;
}

static int atcmd_tcps_cli_add(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx)
{
    atcmd_tcps_cli_context *last_ptr = NULL;
    const atcmd_tcps_config *svr_conf = svr_ctx->conf;
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    const int max_allow_client = svr_conf->sess_info->max_allow_client;
#else
    const int max_allow_client = svr_conf->max_allow_client;
#endif //__SUPPORT_ATCMD_MULTI_SESSION__

    if (svr_ctx->cli_cnt >= max_allow_client) {
        ATCMD_TCPS_ERR("Not allowed tcp client's connection(%d>=%d)\n",
                       svr_ctx->cli_cnt, max_allow_client);
        return -1;
    }

    //increase client count
    svr_ctx->cli_cnt++;

    //link cli_ctx
    if (svr_ctx->cli_ctx) {
        //find last pointer
        for (last_ptr = svr_ctx->cli_ctx ; last_ptr->next != NULL ; last_ptr = last_ptr->next);

        last_ptr->next = cli_ctx;
    } else {
        svr_ctx->cli_ctx = cli_ctx;
    }

    return 0;
}

#if defined (__ENABLE_UNUSED__)
static atcmd_tcps_cli_context *atcmd_tcps_cli_remove_by_ip(atcmd_tcps_context *svr_ctx, struct sockaddr_in *ip_addr)
{
    atcmd_tcps_cli_context *prv_ptr = NULL, *cli_ptr = NULL;

    ATCMD_TCPS_INFO("Start\n");

    for (cli_ptr = svr_ctx->cli_ctx ; cli_ptr != NULL ; cli_ptr = cli_ptr->next) {
        if (atcmd_tcps_compare_ip_addr(&cli_ptr->addr, ip_addr) == 0) {
            break;
        }

        prv_ptr = cli_ptr;
    }

    if (cli_ptr) {
        ATCMD_TCPS_INFO("Found client information(%d.%d.%d.%d:%d)\n",
                        (ntohl(ip_addr->sin_addr.s_addr) >> 24) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr) >> 16) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr) >>  8) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr)      ) & 0xFF,
                        (ntohs(ip_addr->sin_port)));

        if (cli_ptr == svr_ctx->cli_ctx) {
            svr_ctx->cli_ctx = svr_ctx->cli_ctx->next;
        } else {
            prv_ptr->next = cli_ptr->next;
        }

        cli_ptr->next = NULL;

        //decrease client count
        svr_ctx->cli_cnt--;
    } else {
        ATCMD_TCPS_INFO("Not found client information(%d.%d.%d.%d:%d)\n",
                        (ntohl(ip_addr->sin_addr.s_addr) >> 24) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr) >> 16) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr) >>  8) & 0xFF,
                        (ntohl(ip_addr->sin_addr.s_addr)      ) & 0xFF,
                        (ntohs(ip_addr->sin_port)));
    }

    return cli_ptr;
}
#endif // __ENABLE_UNUSED__

static atcmd_tcps_cli_context *atcmd_tcps_cli_remove_by_ctx(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx)
{
    atcmd_tcps_cli_context *prv_ptr = NULL, *cur_ptr = NULL;

    ATCMD_TCPS_INFO("Start\n");

    for (cur_ptr = svr_ctx->cli_ctx ; cur_ptr != NULL ; cur_ptr = cur_ptr->next) {
        if (cur_ptr == cli_ctx) {
            break;
        }

        prv_ptr = cur_ptr;
    }

    if (cur_ptr) {
        ATCMD_TCPS_INFO("Found client information(%p)\n", cli_ctx);

        if (cur_ptr == svr_ctx->cli_ctx) {
            svr_ctx->cli_ctx = svr_ctx->cli_ctx->next;
        } else {
            prv_ptr->next = cur_ptr->next;
        }

        cur_ptr->next = NULL;

        //decrease client count
        svr_ctx->cli_cnt--;
    } else {
        ATCMD_TCPS_INFO("Not found client information\n");
    }

    return cur_ptr;
}

static atcmd_tcps_cli_context *atcmd_tcps_cli_find_by_ip(atcmd_tcps_context *svr_ctx, struct sockaddr_in *ip_addr)
{
    atcmd_tcps_cli_context *cli_ptr = NULL;

    for (cli_ptr = svr_ctx->cli_ctx ; cli_ptr != NULL ; cli_ptr = cli_ptr->next) {
        if (atcmd_tcps_compare_ip_addr(&cli_ptr->addr, ip_addr) == 0) {
            ATCMD_TCPS_INFO("Found client information(%d.%d.%d.%d:%d)\n",
                            (ntohl(ip_addr->sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(ip_addr->sin_port)));

            return cli_ptr;
#ifdef ENABLE_ATCMD_TCPS_DBG_INFO
        } else {
            ATCMD_TCPS_INFO("connected client(%d.%d.%d.%d:%d) vs %d.%d.%d.%d:%d\n",
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(cli_ptr->addr.sin_port)),
                            (ntohl(ip_addr->sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(ip_addr->sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(ip_addr->sin_port)));
#endif // ENABLE_ATCMD_TCPS_DBG_INFO
        }
    }

    ATCMD_TCPS_INFO("Not found client information(%d.%d.%d.%d:%d)\n",
                    (ntohl(ip_addr->sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(ip_addr->sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(ip_addr->sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(ip_addr->sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(ip_addr->sin_port)));

    return NULL;
}

static int atcmd_tcps_cli_release(atcmd_tcps_context *svr_ctx, atcmd_tcps_cli_context *cli_ctx)
{
    int ret = 0;
    TickType_t wait_time = portMAX_DELAY;
    atcmd_tcps_cli_context *tmp_ctx = NULL;

    if (svr_ctx == NULL || cli_ctx == NULL) {
        ATCMD_TCPS_ERR("Invalid parameter\n");
        return -1;
    }

    if (svr_ctx->mutex) {
        xSemaphoreTakeRecursive(svr_ctx->mutex, wait_time);
        ATCMD_TCPS_INFO("Takes semaphore to stop TCP client\n");
    }

    //Check pointer
    for (tmp_ctx = svr_ctx->cli_ctx ; tmp_ctx != NULL ; tmp_ctx = tmp_ctx->next) {
        if (tmp_ctx == cli_ctx) {
            break;
        }
    }

    //Not found.
    if (tmp_ctx == NULL) {
        ATCMD_TCPS_ERR("Not found tcp client task\n");
        ret = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    ret = atcmd_tcps_cli_stop(cli_ctx);
    if (ret) {
        ATCMD_TCPS_ERR("Failed to stop tcp client task(%s,%d)\n",
                       cli_ctx->task_name, cli_ctx->state);
        ret = AT_CMD_ERR_TIMEOUT;
        goto end;
    }

    cli_ctx = atcmd_tcps_cli_remove_by_ctx(svr_ctx, cli_ctx);
    if (!cli_ctx) {
        ATCMD_TCPS_ERR("Failed to remove tcp client context(%s,%p)\n",
                       cli_ctx, cli_ctx->task_name);
        ret = AT_CMD_ERR_UNKNOWN;
        goto end;
    }

    ret = atcmd_tcps_cli_delete_context(&cli_ctx);
    if (ret) {
        ATCMD_TCPS_ERR("Failed to delete tcp client context(%d)\n", ret);
        ret = AT_CMD_ERR_UNKNOWN;
        goto end;
    }

end:

    if (svr_ctx->mutex) {
        ATCMD_TCPS_INFO("Gives semaphore after stoping TCP client\n");
        xSemaphoreGiveRecursive(svr_ctx->mutex);
    }

    return ret;
}

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
static int atcmd_tcps_add_cli_addr(atcmd_tcps_context *ctx, struct sockaddr_in addr)
{
    int idx = 0;
    int empty_idx = -1;
    struct sockaddr_in empty_addr = {0x00,};

    const atcmd_tcps_config *conf;

    ATCMD_TCPS_INFO("Start\n");

    if (ctx == NULL) {
        ATCMD_TCPS_ERR("Invaild parameter\n");
        return -1;
    }

    conf = ctx->conf;

#if defined (ENABLE_ATCMD_TCPS_DBG_INFO)
    for (idx = 0 ; idx < ATCMD_TCPS_MAX_SESS ; idx++) {
        ATCMD_TCPS_INFO("Saved client ip address(%d - %d.%d.%d.%d:%d)\n", idx,
                        (ntohl(conf->sess_info->cli_addr[idx].sin_addr.s_addr) >> 24) & 0xFF,
                        (ntohl(conf->sess_info->cli_addr[idx].sin_addr.s_addr) >> 16) & 0xFF,
                        (ntohl(conf->sess_info->cli_addr[idx].sin_addr.s_addr) >>  8) & 0xFF,
                        (ntohl(conf->sess_info->cli_addr[idx].sin_addr.s_addr)      ) & 0xFF,
                        (ntohs(conf->sess_info->cli_addr[idx].sin_port)));
    }
#endif // ENABLE_ATCMD_TCPS_DBG_INFO

    for (idx = 0 ; idx < ATCMD_TCPS_MAX_SESS ; idx++) {
        //find empty idx
        if ((empty_idx < 0)
                && (memcmp(&conf->sess_info->cli_addr[idx],
                           &empty_addr,
                           sizeof(struct sockaddr_in)) == 0)) {
            ATCMD_TCPS_INFO("Found Empty idx(%d)\n", idx);
            empty_idx = idx;
        }

        //compare ip address
        if (memcmp(&conf->sess_info->cli_addr[idx], &addr, sizeof(struct sockaddr_in)) == 0) {
            ATCMD_TCPS_INFO("Found client ip address(%d - %d.%d.%d.%d:%d)\n", idx,
                            (ntohl(addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(addr.sin_port)));
            return -1;
        }
    }

    if (empty_idx < 0 || ATCMD_TCPS_MAX_SESS < empty_idx) {
        ATCMD_TCPS_ERR("Not found empty idx(%d)\n", empty_idx);
        return -1;
    }

    ATCMD_TCPS_INFO("Added client ip address(%d - %d.%d.%d.%d:%d)\n", empty_idx,
                    (ntohl(addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(addr.sin_port)));

    memcpy((struct sockaddr_in *)&conf->sess_info->cli_addr[empty_idx],
           &addr,
           sizeof(struct sockaddr_in));

    return 0;
}

static int atcmd_tcps_del_cli_addr(atcmd_tcps_context *ctx, struct sockaddr_in addr)
{
    int idx = 0;

    const atcmd_tcps_config *conf;

    if (ctx == NULL) {
        ATCMD_TCPS_ERR("Invaild parameter\n");
        return -1;
    }

    conf = ctx->conf;

    for (idx = 0 ; idx < ATCMD_TCPS_MAX_SESS ; idx++) {
        //find & delete ip address
        if (memcmp(&conf->sess_info->cli_addr[idx], &addr, sizeof(struct sockaddr_in)) == 0) {
            ATCMD_TCPS_INFO("Found client ip address(%d - %d.%d.%d.%d:%d)\n", idx,
                            (ntohl(addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(addr.sin_port)));
            memset((struct socketaddr_in *)&conf->sess_info->cli_addr[idx],
                   0x00,
                   sizeof(struct sockaddr_in));
            return 0;
        }
    }

    ATCMD_TCPS_INFO("Not found client ip address(%d.%d.%d.%d:%d)\n",
                    (ntohl(addr.sin_addr.s_addr) >> 24) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr) >> 16) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr) >>  8) & 0xFF,
                    (ntohl(addr.sin_addr.s_addr)      ) & 0xFF,
                    (ntohs(addr.sin_port)));

    return -1;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

static void atcmd_tcps_cli_task_entry(void *param)
{
    int sys_wdog_id = -1;
    int ret = 0;
    unsigned int local_port = 0;

    atcmd_tcps_cli_context *cli_ctx = (atcmd_tcps_cli_context *)param;

    unsigned char *hdr = NULL;
    size_t hdr_len = 0;
    unsigned char *payload = NULL;
    size_t payload_len = 0;
    size_t tot_len = 0;
    size_t act_hdr_len = 0;
    size_t act_payload_len = 0;

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
    const int cid = cli_ctx->cid;
#else
    const int cid = ID_TS;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("Start\n");

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    if (!cli_ctx) {
        ATCMD_DBG("Invalid parameter\n");
        goto end;
    }

    local_port = ntohs(cli_ctx->addr.sin_port);

    dpm_app_register((char *)(cli_ctx->task_name), local_port);

    dpm_tcp_port_filter_set(local_port);

    ATCMD_TCPS_INFO("Reg - DPM Name:%s(%ld), Local port(%d)\n",
                    cli_ctx->task_name, strlen((char *)cli_ctx->task_name), local_port);

    dpm_app_wakeup_done((char *)(cli_ctx->task_name));

    dpm_app_data_rcv_ready_set((char *)(cli_ctx->task_name));

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
    if (atcmd_tcps_add_cli_addr((atcmd_tcps_context *)cli_ctx->svr_ptr, cli_ctx->addr) == 0) {
        PRINTF_ATCMD("\r\n" ATCMD_TCPS_CONN_RX_PREFIX ":%d,%d.%d.%d.%d,%u\r\n", cid,
                     (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                     (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                     (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                     (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                     (ntohs(cli_ctx->addr.sin_port)));
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    cli_ctx->state = ATCMD_TCPS_CLI_STATE_CONNECTED;

    while (cli_ctx->state == ATCMD_TCPS_CLI_STATE_CONNECTED) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        memset(cli_ctx->buffer, 0x00, cli_ctx->buffer_len);

        tot_len = 0;
        act_hdr_len = 0;
        act_payload_len = 0;

        hdr = cli_ctx->buffer;
        hdr_len = ATCMD_TCPS_RECV_HDR_SIZE;

        payload = cli_ctx->buffer + hdr_len;
        payload_len = cli_ctx->buffer_len - ATCMD_TCPS_RECV_HDR_SIZE;

        da16x_sys_watchdog_suspend(sys_wdog_id);

        ret = recv(cli_ctx->socket, payload, payload_len, 0);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        dpm_app_sleep_ready_clear((char *)(cli_ctx->task_name));

        if (ret > 0) {
            act_payload_len = ret;

            ATCMD_TCPS_INFO("Recv(ip:%d.%d.%d.%d:%d/len:%ld)\n",
                            (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(cli_ctx->addr.sin_port)), act_payload_len);

#if defined(__SUPPORT_TCP_RECVDATA_HEX_MODE__)
            if (is_tcps_data_mode_hexstring()) {
                char *hex_buf = ATCMD_MALLOC(ATCMD_TCPS_RECV_HDR_SIZE + (act_payload_len * 2));
                if (!hex_buf) {
                    ATCMD_TCPS_ERR("Failed to allocate buffer to pass recv data(%ld)\n",
                                   ATCMD_TCPS_RECV_HDR_SIZE + (act_payload_len * 2));
                    goto end;
                }

                act_hdr_len = snprintf((char *)hex_buf, ATCMD_TCPS_RECV_HDR_SIZE,
                                       "\r\n" ATCMD_TCPS_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", cid,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                                       (ntohs(cli_ctx->addr.sin_port)), (act_payload_len * 2));

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
                                       "\r\n" ATCMD_TCPS_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", cid,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                                       (int)(ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                                       (ntohs(cli_ctx->addr.sin_port)), act_payload_len);

                tot_len = act_hdr_len;

                /* For data integrity, memmove() can't be replaced to any other function */
                if (!memmove(hdr + tot_len, payload, act_payload_len)) {
                    ATCMD_TCPS_ERR("Failed to copy received data(%ld)\n", act_payload_len);
                }

                tot_len += act_payload_len;
                memcpy(hdr + tot_len, "\r\n", 2);

                tot_len += 2;
                hdr[tot_len] = '\0';

                PUTS_ATCMD((char *)hdr, tot_len);
            }
        } else {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
                ATCMD_TCPS_INFO("Disconnected(ip:%d.%d.%d.%d:%d/%d)\n",
                                (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                                (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                                (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                                (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                                (ntohs(cli_ctx->addr.sin_port)), errno);

                ATCMD_DBG("[%s] TCP Client disconnected from Server (%d)\n", __func__, ret);

                PRINTF_ATCMD("\r\n" ATCMD_TCPS_DISCONN_RX_PREFIX ":%d,%d.%d.%d.%d,%u\r\n", cid,
                             (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                             (ntohl(cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                             (ntohl(cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                             (ntohl(cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                             (ntohs(cli_ctx->addr.sin_port)));
                goto end;
            }
        }

        if (!da16x_dpm_sock_is_remaining_rx_data(cli_ctx->socket)) {
            dpm_app_sleep_ready_set((char *)(cli_ctx->task_name));
        }
    }

end:

    da16x_sys_watchdog_unregister(sys_wdog_id);

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
    atcmd_tcps_del_cli_addr((atcmd_tcps_context *)cli_ctx->svr_ptr, cli_ctx->addr);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    atcmd_tcps_close_socket(&cli_ctx->socket);

    if (cli_ctx->buffer) {
        ATCMD_FREE(cli_ctx->buffer);
        cli_ctx->buffer = NULL;
    }

    dpm_tcp_port_filter_delete(local_port);

    dpm_app_unregister((char *)(cli_ctx->task_name));

    ATCMD_TCPS_INFO("Unreg - DPM Name:%s(%ld), Local port(%d)\n",
                    cli_ctx->task_name, strlen((char *)cli_ctx->task_name), local_port);

    cli_ctx->state = ATCMD_TCPS_CLI_STATE_TERMINATED;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
    if (cli_ctx->event) {
        xEventGroupSetBits(cli_ctx->event, ATCMD_TCPS_EVT_CLOSED);
    }
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

    ATCMD_TCPS_INFO("End\n");

    cli_ctx->task_handler = NULL;
    vTaskDelete(NULL);

    return ;
}

static int atcmd_tcps_compare_ip_addr(struct sockaddr_in *a, struct sockaddr_in *b)
{
    if (!a || !b) {
        ATCMD_TCPS_INFO("Invalid parameter\n");
        return -2;
    }

    //return memcmp(a, b, sizeof(struct sockaddr_in));
    if ((a->sin_family == b->sin_family)
            && (ntohl(a->sin_addr.s_addr) == ntohl(b->sin_addr.s_addr))
            && (ntohs(a->sin_port) == ntohs(b->sin_port))) {
        return 0;
    }

    return -1;
}
#endif // __SUPPORT_ATCMD__

/* EOF */
