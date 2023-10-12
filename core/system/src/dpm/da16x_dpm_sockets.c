/**
 ****************************************************************************************
 *
 * @file da16x_dpm_sockets.c
 *
 * @brief TCP socket inforamtion for DPM
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

#include "da16x_dpm_sockets.h"

#include "lwip/priv/sockets_priv.h" //for struct lwip_sock
#include "lwip/api.h"               //for struct netconn

#include "da16x_dpm_regs.h"
#include "da16x_dpm.h"

#undef	ENABLE_DA16X_DPM_SOCK_DEBUG_INFO
#define	ENABLE_DA16X_DPM_SOCK_DEBUG_ERR

#define	DA16X_DPM_SOCK_PRINTF			PRINTF

#if defined (ENABLE_DA16X_DPM_SOCK_DEBUG_INFO)
#define	DA16X_DPM_SOCK_DEBUG_INFO(fmt, ...)	\
	DA16X_DPM_SOCK_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	DA16X_DPM_SOCK_DEBUG_INFO(...)		do {} while (0)
#endif	// (ENABLED_DA16X_DPM_SOCK_DEBUG_INFO)

#if defined (ENABLE_DA16X_DPM_SOCK_DEBUG_ERR)
#define	DA16X_DPM_SOCK_DEBUG_ERR(fmt, ...)	\
	DA16X_DPM_SOCK_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	DA16X_DPM_SOCK_DEBUG_ERR(...)		do {} while (0)
#endif	// (ENABLED_DA16X_DPM_SOCK_DEBUG_ERR) 

#define DPM_SOCK_MAX_LWIP_SOCK_NAME_CNT NUM_SOCKETS

typedef enum _dpm_tcp_sess_reason_code {
	DPM_TCP_SESS_NONE = 0,
	DPM_TCP_SESS_BSD_REMAINING_LASTDATA = 1,
	DPM_TCP_SESS_BSD_CLOSING = 2,
	DPM_TCP_SESS_BSD_USED = 3,
	DPM_TCP_SESS_BSD_STATE = 4,
	DPM_TCP_SESS_PCB_UNSENT = 5,
	DPM_TCP_SESS_PCB_UNACK = 6,
} dpm_tcp_sess_reason_code;

static void da16x_dpm_sock_create_mutex();
static int da16x_dpm_sock_take_mutex(TickType_t wait_option);
static int da16x_dpm_sock_give_mutex();

static dpm_tcp_sess_info *da16x_dpm_sock_get_sess_info_by_name(char *name);

static int da16x_dpm_sock_is_available_sleep_tcp_pcb(struct tcp_pcb *pcb,
													 dpm_tcp_sess_reason_code *reason);
static int da16x_dpm_sock_is_available_sleep_bsd(struct lwip_sock *sock,
												 dpm_tcp_sess_reason_code *reason);

static struct lwip_sock_name *dpm_lwip_sock_names = NULL;
static dpm_tcp_sess_info *dpm_tcp_sess_list = NULL;
static SemaphoreHandle_t tcp_sock_alloc_mutex = NULL;

/* Renamed from set_dpm_tcp_ptr() in da16200 v2.3 */
void da16x_dpm_socket_init(void)
{
	if (dpm_tcp_sess_list != NULL) {
		return ;
	}

	DA16X_DPM_SOCK_DEBUG_INFO("Start\n");

	dpm_lwip_sock_names = (struct lwip_sock_name *)TCP_SESS_INFO;

	dpm_tcp_sess_list = (dpm_tcp_sess_info *)(TCP_SESS_INFO + (sizeof(struct lwip_sock_name) * DPM_SOCK_MAX_LWIP_SOCK_NAME_CNT));

	DA16X_DPM_SOCK_DEBUG_INFO("dpm_lwip_sock_names(%p, %ld), "
							  "dpm_tcp_sess_list(%p, %ld)\n",
							  dpm_lwip_sock_names,
							  sizeof(struct lwip_sock_name) * DPM_SOCK_MAX_LWIP_SOCK_NAME_CNT,
							  dpm_tcp_sess_list,
							  sizeof(dpm_tcp_sess_info) * DPM_SOCK_MAX_TCP_SESS);

	return ;
}

/* Renamed from clr_all_dpm_tcp_sess_info() in da16200 v2.3 */
void da16x_dpm_socket_clear_all_tcp_sess_info(void)
{
	if (!chk_rtm_exist()) {
		/* Unsupport RTM */
		return ;
	}

	DA16X_DPM_SOCK_DEBUG_INFO("Start\n");

	da16x_dpm_socket_init();

	//TODO: require to calculate TCP_ALLOC_SZ
	if (dpm_lwip_sock_names != NULL) {
		memset(TCP_SESS_INFO, 0x00, TCP_ALLOC_SZ);
	}

	DA16X_DPM_SOCK_DEBUG_INFO("clear TCP_SESS_INFO(%p, %ld, actual size:%ld)\n",
							  TCP_SESS_INFO, TCP_ALLOC_SZ,
							  (sizeof(struct lwip_sock_name) * DPM_SOCK_MAX_LWIP_SOCK_NAME_CNT)
								+ (sizeof(dpm_tcp_sess_info) * DPM_SOCK_MAX_TCP_SESS));

	return ;
}

void *da16x_dpm_sock_get_lwip_sock_names(void)
{
	if (!dpm_lwip_sock_names) {
		da16x_dpm_socket_init();
	}

	return (void *)dpm_lwip_sock_names;
}

static void da16x_dpm_sock_create_mutex()
{
	if (tcp_sock_alloc_mutex == NULL) {
		tcp_sock_alloc_mutex = xSemaphoreCreateRecursiveMutex();
		if (tcp_sock_alloc_mutex == NULL) {
			DA16X_DPM_SOCK_DEBUG_INFO("failed to create mutex for tcp session\n");
		}
	}

	return ;
}

static int da16x_dpm_sock_take_mutex(TickType_t wait_option)
{
	if (tcp_sock_alloc_mutex) {
		return xSemaphoreTakeRecursive(tcp_sock_alloc_mutex, wait_option);
	}

	return -1;
}

static int da16x_dpm_sock_give_mutex()
{
	if (tcp_sock_alloc_mutex) {
		xSemaphoreGiveRecursive(tcp_sock_alloc_mutex);
	}

	return -1;
}

void da16x_dpm_sock_print_tcp_pcb(int index)	// FOR_DEBUG
{
	int idx = index;
	dpm_tcp_sess_info *sess_info = NULL;
	struct tcp_pcb *ret = NULL;

	if (dpm_tcp_sess_list == NULL) {
		return;
	}

	sess_info = &(dpm_tcp_sess_list[idx]);

	if ((sess_info->used == 1)) {
		ret = &sess_info->pcb;
		PRINTF(CYAN_COLOR " >>> Idx:%d Address:%p,%p flags: %p - 0x%x\n" CLEAR_COLOR,
							idx, sess_info, &sess_info->pcb, &(ret->flags), ret->flags);
	}
}

struct tcp_pcb *da16x_dpm_sock_create_tcp_pcb(char *name)
{
	DA16X_UNUSED_ARG(name);

	int idx = 0;
	dpm_tcp_sess_info *sess_info = NULL;
	struct tcp_pcb *ret = NULL;

	if (!dpm_tcp_sess_list) {
		da16x_dpm_socket_init();
	}

	da16x_dpm_sock_create_mutex();

	da16x_dpm_sock_take_mutex(portMAX_DELAY);

	for (idx = 0 ; idx < DPM_SOCK_MAX_TCP_SESS ; idx++) {
		sess_info = &(dpm_tcp_sess_list[idx]);

		if (sess_info->used == 0) {
			memset(sess_info, 0x00, sizeof(dpm_tcp_sess_info));
			sess_info->used++;
			ret = &sess_info->pcb;

			DA16X_DPM_SOCK_DEBUG_INFO("Allcated TCP session\n"
									  "* Idx: %d\n"
									  "* Name: %s\n"
									  "* Address: %p,%p\n"
									  "* flags: %p - 0x%x\n"
									  "* Size: %ld\n",
									  idx, name, sess_info, &sess_info->pcb, &(ret->flags), ret->flags,
									  sizeof(dpm_tcp_sess_info));
			break;
		}
	}

	da16x_dpm_sock_give_mutex();

	return ret;
}

static dpm_tcp_sess_info *da16x_dpm_sock_get_sess_info_by_name(char *name)
{
	int idx = 0;
	dpm_tcp_sess_info *sess_info = NULL;
	size_t name_len = 0;

	if (!chk_dpm_mode()) {
		return NULL;
	}

	if (!dpm_tcp_sess_list) {
		da16x_dpm_socket_init();
	}

	name_len = dpm_strlen(name);

	for (idx = 0 ; idx < DPM_SOCK_MAX_TCP_SESS ; idx++) {
		sess_info = &(dpm_tcp_sess_list[idx]);

		if ((sess_info->used > 0)
			&& (dpm_strlen(sess_info->pcb.name) == name_len)
			&& (dpm_strncmp(sess_info->pcb.name, name, name_len) == 0)) {
			DA16X_DPM_SOCK_DEBUG_INFO("Found session(%p(%d),%p(%s))\n",
									  sess_info, idx,
									  &(sess_info->pcb), sess_info->pcb.name);
			return sess_info;
		}
	}

	return NULL;
}

struct tcp_pcb *da16x_dpm_sock_get_tcp_pcb(char *name)
{
	dpm_tcp_sess_info *sess_info = NULL;

	sess_info = da16x_dpm_sock_get_sess_info_by_name(name);
	if (sess_info != NULL) {
		DA16X_DPM_SOCK_DEBUG_INFO("Found TCP session(%s)\n", name);
		return &(sess_info->pcb);
	}

	DA16X_DPM_SOCK_DEBUG_INFO("Not found TCP session(%s)\n", name);

	return NULL;
}

int da16x_dpm_sock_delete_tcp_pcb(char *name)
{
	dpm_tcp_sess_info *sess_info = NULL;

	sess_info = da16x_dpm_sock_get_sess_info_by_name(name);
	if (sess_info != NULL) {
		DA16X_DPM_SOCK_DEBUG_INFO("Clear TCP session(%s)\n", name);
		memset(sess_info, 0x00, sizeof(dpm_tcp_sess_info));
		return 0;
	}

	DA16X_DPM_SOCK_DEBUG_ERR("Not found TCP session(%s)\n", name);

	return -1;
}

static int da16x_dpm_sock_is_available_sleep_tcp_pcb(struct tcp_pcb *pcb,
													 dpm_tcp_sess_reason_code *reason)
{
	DA16X_DPM_SOCK_DEBUG_INFO("name(%s), pcb(%p)\n", pcb->name, pcb);

	if (pcb->unsent || pcb->unacked || pcb->snd_queuelen) {
		if (pcb->unsent) {
			DA16X_DPM_SOCK_DEBUG_INFO("There is unsent messages \n");
		}

		if (pcb->unacked) {
			DA16X_DPM_SOCK_DEBUG_INFO("There is unacked messages \n");
		}

		if (pcb->snd_queuelen) {
			DA16X_DPM_SOCK_DEBUG_INFO("There is snd_queuelen (qlen:%d) \n",
									  pcb->snd_queuelen);
		}

		if (reason) {
			*reason = DPM_TCP_SESS_PCB_UNSENT;
		}

		return pdFALSE;
	}


	/* tcp_rx case: if tcp_ack not been sent out yet, wait until tcp_ack is sent out  */
	if(((pcb)->state != LISTEN) && ((pcb)->flags & TF_ACK_DELAY)) {
		DA16X_DPM_SOCK_DEBUG_INFO("Waiting for ACK (%p - flags:0x%x) \n",
								  &((pcb)->flags), (pcb)->flags);
		if (reason) {
			*reason = DPM_TCP_SESS_PCB_UNACK;
		}
		return pdFALSE;
	}

	return pdTRUE;
}

static int da16x_dpm_sock_is_available_sleep_bsd(struct lwip_sock *sock,
												 dpm_tcp_sess_reason_code *reason)
{
	const int max_fd_used = 2;

	if (sock->lastdata.pbuf) {
		DA16X_DPM_SOCK_DEBUG_INFO("there is received data\n");
		if (reason) {
			*reason = DPM_TCP_SESS_BSD_REMAINING_LASTDATA;
		}
		return pdFALSE;
	}

#if LWIP_NETCONN_FULLDUPLEX
	if (sock->fd_free_pending) {
		DA16X_DPM_SOCK_DEBUG_INFO("socket is closing(%d)\n", sock->fd_free_pending);
		if (reason) {
			*reason = DPM_TCP_SESS_BSD_CLOSING;
		}
		return pdFALSE;
	}

	if (sock->fd_used > max_fd_used) {
		DA16X_DPM_SOCK_DEBUG_INFO("lwip_sock->fd_used(%d,%d)\n",
								  sock->fd_used, max_fd_used);
		if (reason) {
			*reason = DPM_TCP_SESS_BSD_USED;
		}
		return pdFALSE;
	}
#endif /* LWIP_NETCONN_FULLDUPLEX */

	if (NETCONNTYPE_GROUP(sock->conn->type) == NETCONN_TCP) {
		if ((sock->conn->state != NETCONN_NONE)
			&& (sock->conn->state != NETCONN_LISTEN)) {
			DA16X_DPM_SOCK_DEBUG_INFO("netconn is busy(%d)\n", sock->conn->state);
			if (reason) {
				*reason = DPM_TCP_SESS_BSD_STATE;
			}
			return pdFALSE;
		}
	} else {
		DA16X_DPM_SOCK_DEBUG_INFO("socket type(%x) is not TCP\n",
								  NETCONNTYPE_GROUP(sock->conn->type));
		return pdTRUE;
	}

	return pdTRUE;
}

int da16x_dpm_sock_is_available_set_sleep(void)
{
	extern struct lwip_sock *get_socket_dpm(int fd);
	extern void done_socket_dpm(struct lwip_sock *sock);

	int ret = pdTRUE;
	int idx = 0;

	dpm_tcp_sess_reason_code reason = DPM_TCP_SESS_NONE;

	//for bsd
	struct lwip_sock *sock = NULL;

	//for tcp_pcb
	dpm_tcp_sess_info *sess_info = NULL;

	//for bsd socket
	for (idx = 0 ; idx < NUM_SOCKETS ; idx++) {
		reason = DPM_TCP_SESS_NONE;

		sock = get_socket_dpm(idx);
		if ((sock == NULL)
			|| (sock->conn == NULL)
			|| (strlen(sock->conn->pcb.tcp->name) == 0)) {
			continue;
		}

		DA16X_DPM_SOCK_DEBUG_INFO("Check BSD Socket(%d)\n", idx);

		ret = da16x_dpm_sock_is_available_sleep_bsd(sock, &reason);
		if (!ret) {
			DA16X_DPM_SOCK_DEBUG_INFO("BSD Socket(%d) is progressing(%d)\n", idx, reason);
			done_socket_dpm(sock);

			sess_info = da16x_dpm_sock_get_sess_info_by_name(sock->conn->pcb.tcp->name);
			if (sess_info) {
				DA16X_DPM_SOCK_DEBUG_INFO("wait_cnt(%d)\n", sess_info->wait_cnt);
				sess_info->wait_cnt++;

				if (sess_info->wait_cnt > DPM_SOCK_MAX_TCP_WAIT_CNT) {
					sess_info->wait_cnt = 0;
					ret = pdTRUE;

					DA16X_DPM_SOCK_DEBUG_ERR("BSD Socket(%d) is progressing(%d)"
											 "- wait_cnt(%d/%d)\n",
											 idx, reason,
											 DPM_SOCK_MAX_TCP_WAIT_CNT,
											 sess_info->wait_cnt);
					break;
				}
			}

			return pdFALSE;
		}

		done_socket_dpm(sock);
	}

	//for tcp_pcb
	if (!dpm_tcp_sess_list) {
		da16x_dpm_socket_init();
	}

	for (idx = 0 ; idx < DPM_SOCK_MAX_TCP_SESS ; idx++) {
		reason = DPM_TCP_SESS_NONE;

		sess_info = &(dpm_tcp_sess_list[idx]);

		if (sess_info->used > 0) {
			DA16X_DPM_SOCK_DEBUG_INFO("Check TCP_PCB(%p(%d),%p(%s)) - wait_cnt(%d)\n",
									  sess_info, idx, &(sess_info->pcb),
									  sess_info->pcb.name, sess_info->wait_cnt);

			ret = da16x_dpm_sock_is_available_sleep_tcp_pcb(&(sess_info->pcb), &reason);
			if (!ret) {
				DA16X_DPM_SOCK_DEBUG_INFO("TCP_PCB(%p(%d),%p(%s)) is progressing(%d)."
										  "- wait_cnt(%d)\n",
										  sess_info, idx, &(sess_info->pcb),
										  sess_info->pcb.name, reason,
										  sess_info->wait_cnt);

				sess_info->wait_cnt++;
				if (sess_info->wait_cnt > DPM_SOCK_MAX_TCP_WAIT_CNT) {
					sess_info->wait_cnt = 0;
					ret = pdTRUE;

					DA16X_DPM_SOCK_DEBUG_ERR("TCP_PCB(%p(%d),%p(%s)) is progressing(%d)."
											 "- wait_cnt(%d/%d)\n",
											 sess_info, idx, &(sess_info->pcb),
											 sess_info->pcb.name, reason,
											 DPM_SOCK_MAX_TCP_WAIT_CNT,
											 sess_info->wait_cnt);
					break;
				}

				return pdFALSE;
			}
		}
	}

	//clear wait_cnt
	if (ret) {
		DA16X_DPM_SOCK_DEBUG_INFO("Clear wait_cnt\n");
		for (idx = 0 ; idx < DPM_SOCK_MAX_TCP_SESS ; idx++) {
			sess_info = &(dpm_tcp_sess_list[idx]);
			sess_info->wait_cnt = 0;
		}
	}

	return ret;
}

int da16x_dpm_sock_is_available_connected_tcp_sess(void)
{
	unsigned int idx = 0;
	dpm_tcp_sess_info *sess_info = NULL;

    //for tcp_pcb
    if (!dpm_tcp_sess_list) {
        da16x_dpm_socket_init();
    }

    for (idx = 0 ; idx < DPM_SOCK_MAX_TCP_SESS ; idx++) {
        sess_info = &(dpm_tcp_sess_list[idx]);
        if (sess_info->used > 0 && sess_info->pcb.state == ESTABLISHED) {
            return pdTRUE;
        }
    }

    return pdFALSE;
}

int da16x_dpm_sock_is_established(char *name)
{
	dpm_tcp_sess_info *sess_info = NULL;
	sess_info = da16x_dpm_sock_get_sess_info_by_name(name);
	
    if (sess_info != NULL) {
        return (sess_info->pcb.state == ESTABLISHED ? pdTRUE : pdFALSE);
    }

   return pdFALSE;
}

//Only for BSD socket
int da16x_dpm_sock_is_remaining_rx_data(int fd)
{
	extern struct lwip_sock *get_socket_dpm(int fd);
	extern void done_socket_dpm(struct lwip_sock *sock);

	struct lwip_sock *sock = NULL;

	int remaining = 0;

	sock = get_socket_dpm(fd);
	if (sock) {
		if (sock->lastdata.pbuf) {
			if (NETCONNTYPE_GROUP(netconn_type(sock->conn)) == NETCONN_TCP) {
				remaining = sock->lastdata.pbuf->tot_len;
			} else {
				remaining = sock->lastdata.netbuf->p->tot_len;
			}
		}

		done_socket_dpm(sock);
	}
	
	return remaining;
}
