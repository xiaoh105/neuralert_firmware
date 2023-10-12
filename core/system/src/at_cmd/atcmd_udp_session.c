/**
 ****************************************************************************************
 *
 * @file atcmd_udp_session.c
 *
 * @brief AT-CMD UDP Socket Controller
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

#include "atcmd_udp_session.h"
#include "da16x_sys_watchdog.h"

#if defined	(__SUPPORT_ATCMD__)

#undef ENABLE_ATCMD_UDPS_DBG_INFO
#undef ENABLE_ATCMD_UDPS_DBG_ERR

#define	ATCMD_UDPS_DBG	ATCMD_DBG

#if defined (ENABLE_ATCMD_UDPS_DBG_INFO)
#define	ATCMD_UDPS_INFO(fmt, ...)	\
	ATCMD_UDPS_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_UDPS_INFO(...)	do {} while (0)
#endif	// (ENABLE_ATCMD_TCCS_DBG_INFO)

#if defined (ENABLE_ATCMD_UDPS_DBG_ERR)
#define	ATCMD_UDPS_ERR(fmt, ...)	\
	ATCMD_UDPS_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_UDPS_ERR(...)	do {} while (0)
#endif // (ENABLE_ATCMD_UDPS_DBG_ERR) 

static void atcmd_udps_task_entry(void *param);

int atcmd_udps_init_context(atcmd_udps_context *ctx)
{
	ATCMD_UDPS_INFO("Start\r\n");

	memset(ctx, 0x00, sizeof(atcmd_udps_context));

	ctx->socket = -1;
	ctx->state = ATCMD_UDPS_STATE_TERMINATED;

	return 0;
}

int atcmd_udps_deinit_context(atcmd_udps_context *ctx)
{
	ATCMD_UDPS_INFO("Start\r\n");

	if (ctx->state != ATCMD_UDPS_STATE_TERMINATED) {
		ATCMD_UDPS_ERR("udp session is not terminated(%d)\r\n", ctx->state);
		return -1;
	}

	if (ctx->buffer) {
		ATCMD_UDPS_INFO("To free udp session's recv buffer\r\n");
		ATCMD_FREE(ctx->buffer);
		ctx->buffer = NULL;
	}

	if (ctx->socket > -1) {
		ATCMD_UDPS_INFO("To close udp session socket\r\n");
		close(ctx->socket);
		ctx->socket = -1;
	}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	if (ctx->event) {
		ATCMD_UDPS_INFO("To delete event\n");
		vEventGroupDelete(ctx->event);
		ctx->event = NULL;
	}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	atcmd_udps_init_context(ctx);

	return 0;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_udps_init_config(const int cid, atcmd_udps_config *conf, atcmd_udps_sess_info *sess_info)
{
	ATCMD_UDPS_INFO("Start\r\n");

	memset(conf, 0x00, sizeof(atcmd_udps_config));

	conf->cid = cid;	

	snprintf((char *)conf->task_name, (ATCMD_UDPS_MAX_TASK_NAME - 1), "%s_%d",
			ATCMD_UDPS_TASK_NAME, cid);

	conf->task_priority = ATCMD_UDPS_TASK_PRIORITY;

	conf->task_size = (ATCMD_UDPS_TASK_SIZE/4);

	snprintf(conf->sock_name, (ATCMD_UDPS_MAX_SOCK_NAME - 1), "%s_%d",
			ATCMD_UDPS_SOCK_NAME, cid);

	conf->rx_buflen = ATCMD_UDPS_RECV_BUF_SIZE;

	conf->sess_info = sess_info;

	return 0;
}
#else
int atcmd_udps_init_config(const int cid, atcmd_udps_config *conf)
{
	ATCMD_UDPS_INFO("Start\r\n");

	memset(conf, 0x00, sizeof(atcmd_udps_config));

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	conf->cid = cid;	
#else
	DA16X_UNUSED_ARG(cid);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	snprintf((char *)conf->task_name, (ATCMD_UDPS_MAX_TASK_NAME - 1), "%s_%d",
			ATCMD_UDPS_TASK_NAME, cid);

	conf->task_priority = ATCMD_UDPS_TASK_PRIORITY;

	conf->task_size = (ATCMD_UDPS_TASK_SIZE/4);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	snprintf(conf->sock_name, (ATCMD_UDPS_MAX_SOCK_NAME - 1), "%s_%d",
			ATCMD_UDPS_SOCK_NAME, cid);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	conf->rx_buflen = ATCMD_UDPS_RECV_BUF_SIZE;

	return 0;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_udps_deinit_config(atcmd_udps_config *conf)
{
	ATCMD_UDPS_INFO("Start\r\n");

	memset(conf, 0x00, sizeof(atcmd_udps_config));

	return 0;
}

int atcmd_udps_set_local_addr(atcmd_udps_config *conf, char *ip, int port)
{
	ATCMD_UDPS_INFO("Start\r\n");

	if (ip) {
		//not implemented yet.
		ATCMD_UDPS_ERR("Not allowed to set local ip address\r\n");
		return -1;
	}

	//check range
	if (port < ATCMD_UDPS_MIN_PORT || port > ATCMD_UDPS_MAX_PORT) {
		ATCMD_UDPS_ERR("Invalid port(%d)\n", port);
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

int atcmd_udps_set_peer_addr(atcmd_udps_config *conf, char *ip, int port)
{
	int ret = 0;
	struct addrinfo hints, *addr_list = NULL;
	char str_port[16] = {0x00, };

	ATCMD_UDPS_INFO("Start\r\n");

	if (!ip) {
		ATCMD_UDPS_ERR("Invalid parameters\r\n");
		return -1;
	}

	//check range
	if (port <= 0 || port > 0xFFFF) {
		ATCMD_UDPS_ERR("Invalid port(%d)\n", port);
		return -1;
	}

	memset(&hints, 0x00, sizeof(struct addrinfo));

	if (!is_in_valid_ip_class(ip)) {
		hints.ai_family = AF_INET;	//IPv4 only
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		snprintf(str_port, sizeof(str_port), "%d", port);

		ret = getaddrinfo(ip, str_port, &hints, &addr_list);
		if ((ret != 0) || !addr_list) {
			ATCMD_DBG("Failed to get address info(%d)\r\n", ret);
			return -1;
		}

		memcpy((struct sockaddr *)&conf->peer_addr, addr_list->ai_addr, sizeof(struct sockaddr));

		freeaddrinfo(addr_list);
	} else {
		conf->peer_addr.sin_addr.s_addr = inet_addr(ip);
	}

	conf->peer_addr.sin_family = AF_INET;
	conf->peer_addr.sin_port = htons(port);

	ATCMD_UDPS_INFO("Peer address(%d.%d.%d.%d:%d)\r\n",
			(ntohl(conf->peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr)      ) & 0xFF,
			(ntohs(conf->peer_addr.sin_port)));

	return 0; 
}

int atcmd_udps_set_config(atcmd_udps_context *ctx, atcmd_udps_config *conf)
{
	ATCMD_UDPS_INFO("Start\r\n");

	if (strlen((const char *)(conf->task_name)) == 0) {
		ATCMD_UDPS_ERR("Invalid task name\r\n");
		return -1;
	}

	if (conf->task_priority == 0) {
		ATCMD_UDPS_ERR("Invalid task priority\r\n");
		return -1;
	}

	if (conf->task_size == 0) {
		ATCMD_UDPS_ERR("Invalid task size\r\n");
		return -1;
	}

	if (conf->local_addr.sin_family != AF_INET) {
		ATCMD_UDPS_ERR("Invalid local address\r\n");
		return -1;
	}

	if (conf->rx_buflen == 0) {
		ATCMD_UDPS_ERR("Invalid recv buffer size\r\n");
		return -1;
	}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	if (strlen((const char *)(conf->sock_name)) == 0) {
		ATCMD_UDPS_ERR("Invalid socket name\n");
		return -1;
	}

	ctx->event = xEventGroupCreate();
	if (ctx->event == NULL) {
		ATCMD_UDPS_INFO("Failed to create event\n");
		return -1;
	}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	ATCMD_UDPS_INFO("* %-20s: %s(%ld)\r\n" // task name
			"* %-20s: %d\r\n" // task priority
			"* %-20s: %d\r\n" // task size 
			"* %-20s: %d\r\n" // rx buflen
			"* %-20s: %d.%d.%d.%d:%d\r\n" // local ip address
			"* %-20s: %d.%d.%d.%d:%d\r\n", // peer ip address
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
			"Peer IP address", 
			(ntohl(conf->peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
			(ntohl(conf->peer_addr.sin_addr.s_addr)      ) & 0xFF,
			(ntohs(conf->peer_addr.sin_port)));

	ctx->conf = conf;

	return 0;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
int atcmd_udps_wait_for_ready(atcmd_udps_context *ctx)
{
	const int wait_time = 10;
	const int max_wait_cnt = 10;
	int wait_cnt = 0;

	int ret = 0;
	unsigned int events = 0x00;

	if (!ctx) {
		ATCMD_UDPS_ERR("Invalid parameter\n");
		return -1;
	}

	for (wait_cnt = 0 ; wait_cnt < max_wait_cnt ; wait_cnt++) {
		if (ctx->event) {
			events = xEventGroupWaitBits(ctx->event, ATCMD_UDPS_EVT_ANY,
										pdTRUE, pdFALSE, wait_time);
			if (events & ATCMD_UDPS_EVT_ACTIVE) {
				ATCMD_UDPS_INFO("Got ready event\n");
				break;
			} else if (events & ATCMD_UDPS_EVT_CLOSED) {
				ATCMD_UDPS_INFO("Got close event\n");
				return -1;
			}
		} else {
			if (ctx->state == ATCMD_UDPS_STATE_ACTIVE) {
				break;
			} else if (ctx->state == ATCMD_UDPS_STATE_TERMINATED) {
				ret = -1;
				break;
			}

			vTaskDelay(wait_time);
		}
	}

	return ret;

}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

int atcmd_udps_start(atcmd_udps_context *ctx)
{
	int ret = 0;

	ATCMD_UDPS_INFO("Start\r\n");

	if (!ctx || !ctx->conf){
		ATCMD_UDPS_ERR("Invalid parameters\r\n");
		return -1;
	}

	if (ctx->state != ATCMD_UDPS_STATE_TERMINATED) {
		ATCMD_UDPS_ERR("UDP session is not terminated(%d)\r\n", ctx->state);
		return -1;
	}

	ctx->state = ATCMD_UDPS_STATE_READY;

	ret = xTaskCreate(atcmd_udps_task_entry,
			(const char *)(ctx->conf->task_name),
			ctx->conf->task_size,
			(void *)ctx,
			ctx->conf->task_priority,
			&ctx->task_handler);
	if (ret != pdPASS) {
		ATCMD_DBG("Failed to create udp session task(%d)\r\n", ret);
		ctx->state = ATCMD_UDPS_STATE_TERMINATED;
		return -1;
	}

	return 0;
}

int atcmd_udps_stop(atcmd_udps_context *ctx)
{
	const int wait_time = 100;
	const int max_cnt = 10;
	int cnt = 0; 
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	unsigned int events = 0x00;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	ATCMD_UDPS_INFO("Start\n");

	if (ctx->state == ATCMD_UDPS_STATE_ACTIVE) {
		ATCMD_UDPS_INFO("Change udp session state from %d to %d\r\n",
				ctx->state, ATCMD_UDPS_STATE_REQ_TERMINATE);

		ctx->state = ATCMD_UDPS_STATE_REQ_TERMINATE;

		for (cnt = 0 ; cnt < max_cnt ; cnt++) {
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
			if (ctx->event) {
				events = xEventGroupWaitBits(ctx->event, ATCMD_UDPS_EVT_CLOSED,
						pdTRUE, pdFALSE, wait_time);
				if (events & ATCMD_UDPS_EVT_CLOSED) {
					ATCMD_UDPS_INFO("Closed udp session task\n");
					break;
				}
			} else {
				if (ctx->state == ATCMD_UDPS_STATE_TERMINATED) {
					ATCMD_UDPS_INFO("Closed udp session task\n");
					break;
				}

				vTaskDelay(wait_time);
			}
#else
			if (ctx->state == ATCMD_UDPS_STATE_TERMINATED) {
				break;
			}

			vTaskDelay(wait_time);
#endif // __SUPPORT_ATCMD_MULTI_SESSION__
		}
	}

	return ((ctx->state == ATCMD_UDPS_STATE_TERMINATED) ? 0 : -1);
}

int atcmd_udps_tx(atcmd_udps_context *ctx, char *data, unsigned int data_len, char *ip, unsigned int port)
{
	int ret = 0;

	struct addrinfo hints, *addr_list = NULL;
	struct sockaddr_in peer_addr;
	char str_port[16] = {0x00, };

	ATCMD_UDPS_INFO("Start\r\n");

	memset(&hints, 0x00, sizeof(hints));
	memset(&peer_addr, 0x00, sizeof(peer_addr));

	if ((data_len > TX_PAYLOAD_MAX_SIZE)
			|| (ctx->state != ATCMD_UDPS_STATE_ACTIVE)
			|| !(ctx->conf)) {
		ATCMD_UDPS_ERR("Invalid parameter(data_len:%ld, state:%ld, ctx->conf:%p)\r\n",
				data_len, ctx->state, ctx->conf);
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}

	if (ip && (strcmp(ip, "0") == 0)) {
		ip = NULL;
	}

	if (ip && port) {
		ATCMD_UDPS_INFO("To find peer ip address(%s:%d)\r\n", ip, port);

		peer_addr.sin_family = AF_INET;
		peer_addr.sin_port = htons(port);

		if (!is_in_valid_ip_class(ip)) {
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;

			snprintf(str_port, sizeof(str_port), "%d", port);

			ret = getaddrinfo(ip, str_port, &hints, &addr_list);
			if ((ret != 0) || !addr_list) {
				ATCMD_DBG("Failed to get peer ip address(%d)\r\n", ret);
				return AT_CMD_ERR_IP_ADDRESS;;
			}

			//pick 1st address
			memcpy((struct sockaddr *)&peer_addr, addr_list->ai_addr, sizeof(struct sockaddr));

			freeaddrinfo(addr_list);
		} else {
			peer_addr.sin_addr.s_addr = inet_addr(ip);
		}
	} else  {
		ATCMD_UDPS_INFO("To use peer ip address of conf\r\n");

		memcpy(&peer_addr, &ctx->conf->peer_addr, sizeof(struct sockaddr_in));
	}

	ATCMD_UDPS_INFO("Peer ip address(%d.%d.%d.%d:%d), len(%ld)\r\n",
			(ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
			(ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
			(ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
			(ntohl(peer_addr.sin_addr.s_addr)      ) & 0xFF,
			(ntohs(peer_addr.sin_port)), data_len);

	if ((peer_addr.sin_family == AF_UNSPEC)
			|| (peer_addr.sin_addr.s_addr == 0)
			|| (peer_addr.sin_port == 0)) {
		ATCMD_DBG("Invalid peer ip address\r\n");
		return AT_CMD_ERR_IP_ADDRESS;;
	}

	ret = sendto(ctx->socket, data, data_len, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
	if (ret <= 0) {
		ATCMD_DBG("[%s] udp socket send fail (%d)\n", __func__, ret);
		return AT_CMD_ERR_DATA_TX;
	}

	return AT_CMD_ERR_CMD_OK;
}

static void atcmd_udps_task_entry(void *param)
{
	int sys_wdog_id = -1;
	int ret = 0;
	atcmd_udps_context *ctx = (atcmd_udps_context *)param;
	const atcmd_udps_config *conf = ctx->conf;

	unsigned int local_port = 0;

	struct sockaddr_in peer_addr;
	int addr_len = 0;

	struct timeval sockopt_timeout;

	//releated with recv data
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	const int cid = conf->cid;
#else
	const int cid = ID_US;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	unsigned char *hdr = NULL;
	size_t hdr_len = 0;
	size_t tot_len = 0;
	size_t act_hdr_len = 0;
	size_t act_payload_len = 0;

	unsigned char *payload = NULL;
	size_t payload_len = 0;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	const char *dpm_name = (const char *)conf->task_name;
	const char *sock_name = (const char *)conf->sock_name;
#else
	const char *dpm_name = (const char *)ATCMD_UDPS_DPM_NAME;
	const char *sock_name = (const char *)ATCMD_UDPS_DPM_NAME;
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	sockopt_timeout.tv_sec = 0;
	sockopt_timeout.tv_usec = ATCMD_UDPS_RECV_TIMEOUT * 1000;

	if (!ctx) {
		ATCMD_DBG("[%s] Invalid param\r\n", __func__);
		goto atcmd_udps_term;
	}

	ctx->buffer = ATCMD_MALLOC(conf->rx_buflen);
	if (!ctx->buffer) {
		ATCMD_DBG("[%s] No FREE memory space for rx buffer(%ld)\r\n", __func__, conf->rx_buflen);
		goto atcmd_udps_term;
	}

	local_port = ntohs(conf->local_addr.sin_port);

	dpm_app_register((char *)dpm_name, local_port);

	dpm_udp_port_filter_set(local_port);

	ATCMD_UDPS_INFO("Reg - DPM Name:%s(%ld), Local port(%d)\n",
			dpm_name, strlen(dpm_name), local_port);

	ctx->socket = socket_dpm((char *)sock_name, AF_INET, SOCK_DGRAM, 0);
	if (ctx->socket < 0) {
		ATCMD_DBG("Failed to create udp socket(%d:%d)\r\n", ctx->socket, errno);
		goto atcmd_udps_term;
	}

	ATCMD_UDPS_INFO("UDP Session: socket descriptor(%d)\r\n", ctx->socket);

	ret = setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO,
			(const void *)&sockopt_timeout, sizeof(sockopt_timeout));
	if (ret != 0) {
		ATCMD_DBG("Failed to set socket option - SO_RCVTIMEOUT(%d:%d)\r\n", ret, errno);
	}

	ret = bind(ctx->socket, (struct sockaddr *)&conf->local_addr, sizeof(struct sockaddr_in));
	if (ret != 0) {
		ATCMD_DBG("Failed to bind udp socket(%d:%d)\r\n", ret, errno);
		goto atcmd_udps_term;
	}

	ctx->state = ATCMD_UDPS_STATE_ACTIVE;

	dpm_app_wakeup_done((char *)dpm_name);

	dpm_app_data_rcv_ready_set((char *)dpm_name);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	if (ctx->event) {
		xEventGroupSetBits(ctx->event, ATCMD_UDPS_EVT_ACTIVE);
	}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	while (ctx->state == ATCMD_UDPS_STATE_ACTIVE) {
		da16x_sys_watchdog_notify(sys_wdog_id);

		memset(&peer_addr, 0x00, sizeof(struct sockaddr_in));
		memset(ctx->buffer, 0x00, conf->rx_buflen);

		tot_len = 0;
		act_hdr_len = 0;
		act_payload_len = 0;

		hdr = ctx->buffer;
		hdr_len = ATCMD_UDPS_RECV_HDR_SIZE;

		payload = ctx->buffer + hdr_len;
		payload_len = conf->rx_buflen - ATCMD_UDPS_RECV_HDR_SIZE;

		addr_len = sizeof(struct sockaddr_in);

		//ATCMD_UDPS_INFO("Waiting for udp data\r\n");

		da16x_sys_watchdog_suspend(sys_wdog_id);

		ret = recvfrom(ctx->socket, payload, payload_len, 0,
				(struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);

		da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

		dpm_app_sleep_ready_clear((char *)dpm_name);

		if (ret > 0) {
			act_payload_len = ret;

			ATCMD_UDPS_INFO("Recv(ip:%d.%d.%d.%d:%d/len:%ld)\r\n",
					(ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
					(ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
					(ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
					(ntohl(peer_addr.sin_addr.s_addr)      ) & 0xFF,
					(ntohs(peer_addr.sin_port)), act_payload_len);

			act_hdr_len = snprintf((char *)hdr, hdr_len,
								"\r\n" ATCMD_UDP_DATA_RX_PREFIX ":%d,%d.%d.%d.%d,%u,%d,", cid,
								(int)((ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xFF),
								(int)((ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xFF),
								(int)((ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xFF),
								(int)((ntohl(peer_addr.sin_addr.s_addr)      ) & 0xFF),
								ntohs(peer_addr.sin_port), act_payload_len);

			tot_len = act_hdr_len;

            /* For data integrity, memmove() can't be replaced to any other function */
            if (!memmove(hdr + tot_len, payload, act_payload_len)) {
				ATCMD_UDPS_ERR("Failed to copy received data(%ld)\n", act_payload_len);
			}

			tot_len += act_payload_len;
			memcpy(hdr + tot_len, "\r\n", 2);

			tot_len += 2;
			hdr[tot_len] = '\0';

			PUTS_ATCMD((char *)hdr, tot_len);
		}

		if (!da16x_dpm_sock_is_remaining_rx_data(ctx->socket)) {
			dpm_app_sleep_ready_set((char *)dpm_name);
		}
	}

atcmd_udps_term:

	da16x_sys_watchdog_unregister(sys_wdog_id);

	close(ctx->socket);
	ctx->socket = -1;

	if (ctx->buffer) {
		ATCMD_FREE(ctx->buffer);
		ctx->buffer = NULL;
	}
	ctx->buffer_len = 0;

	dpm_udp_port_filter_delete(local_port);

	dpm_app_unregister((char *)dpm_name);

	ATCMD_UDPS_INFO("Unreg - DPM Name:%s(%ld), Local port(%d)\n",
			dpm_name, strlen(dpm_name), local_port);

	ctx->state = ATCMD_UDPS_STATE_TERMINATED;

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
	if (ctx->event) {
		xEventGroupSetBits(ctx->event, ATCMD_UDPS_EVT_CLOSED);
	}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

	ATCMD_UDPS_INFO("End of task(state:%d)\r\n", ctx->state);

	ctx->task_handler = NULL;
    vTaskDelete(NULL);

	return ;
}
#endif	// __SUPPORT_ATCMD__

/* EOF */
