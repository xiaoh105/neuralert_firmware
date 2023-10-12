/**
 ****************************************************************************************
 *
 * @file lw_packet_supp.c
 *
 * @brief Renesas Wi-Fi Supplicant - Layer2 packet handling
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


#include "ieee80211_i.h"        /* Located in umac/inc */

#include "supp_def.h"
#include "supp_common.h"
#include "wpabuf.h"
#include "supp_debug.h"
#include "supp_eloop.h"
#include "l2_packet.h"
#include "os.h"
#include "supp_driver.h"
#include "wpa_supplicant_i.h"

#pragma GCC diagnostic ignored "-Wparentheses"

#ifdef	L2_PACKET_OLD_TYPE
/* L2_PACKET communication by event flags and message queue */
da16x_l2_pkt_msg_t l2_pkt_rx_buf;
da16x_l2_pkt_msg_t *l2_pkt_rx_buf_p = (da16x_l2_pkt_msg_t *)&l2_pkt_rx_buf;

TX_QUEUE da16x_l2_pkt_rx_q;
UCHAR	da16x_l2_pkt_rx_msg_q_pool[DA16X_L2_PKT_Q_SIZE];

/* Mutex resources */
TX_MUTEX l2_pkt_mutex;

int l2_pkt_msg_q_init(void)
{
	int	status;
	int	rtn = ERR_OK;

	TX_FUNC_START("            ");

	/* Create Message Queue for FC9000 Driver Event */
	memset(&l2_pkt_rx_buf, 0, sizeof(da16x_l2_pkt_msg_t));

	/* UMAC L2_Packet Message Queue */
	status = tx_queue_create(&da16x_l2_pkt_rx_q, "l2_pkt_rx_q",
			DA16X_L2_PKT_Q_SIZE / 4,
			(void *)da16x_l2_pkt_rx_msg_q_pool,
			DA16X_L2_PKT_Q_SIZE);
	if (status != TX_SUCCESS) {
		da16x_err_prt("[%s] da16x_l2_pkt_rx_q create fail (stauts=%d)\n",
				__func__, status);
		rtn = DA16X_L2_PKT_RX_Q_CREATE_FAIL;
	}

	/* Create mutex for l2_packet_rx */
	status = tx_mutex_create(&l2_pkt_mutex, "l2_pkt_mutex", TX_NO_INHERIT);
	if (status != ERR_OK) {
		da16x_err_prt("[%s] l2_pkt_mutex create fail (status=%d) !!!\n",
				__func__, status);
	}

	TX_FUNC_END("            ");

	return rtn;
}
#endif	/* L2_PACKET_OLD_TYPE */

struct l2_packet_data *l2_packet_init(
	const char *ifname,
	const u8 *own_addr,
	unsigned short protocol,
	void (*rx_callback)(void *ctx, const u8 *src_addr,
					const u8 *buf, size_t len),
	void *rx_callback_ctx,
	int l2_hdr)
{
	struct l2_packet_data *l2;
#ifdef	L2_PACKET_OLD_TYPE
	UINT	status;
#endif	/* L2_PACKET_OLD_TYPE */

	TX_FUNC_START("          ");

	l2 = os_zalloc(sizeof(struct l2_packet_data));
	if (l2 == NULL) {
		da16x_eapol_prt("          [%s] l2 memory alloc error\n", __func__);
		return NULL;
	}

	os_strlcpy(l2->ifname, ifname, sizeof(l2->ifname));
	l2->rx_callback = rx_callback;
	l2->rx_callback_ctx = rx_callback_ctx;
	l2->l2_hdr = l2_hdr;

	/* by Bright : for FC9000 */
	memcpy(l2->own_addr, own_addr, ETH_ALEN);

#ifdef	L2_PACKET_OLD_TYPE
	/* by Bright :
	 *	Have to get a rx_event and read message queues
	 */
	status = l2_pkt_msg_q_init();
	if (status != ERR_OK) {
		free(l2);

		da16x_notice_prt("[%s] L2 MsgQ create error\n", __func__);
		return NULL;
	}
#endif	/* L2_PACKET_OLD_TYPE */

	TX_FUNC_END("          ");

	return l2;
}

void l2_packet_deinit(struct l2_packet_data *l2)
{
	TX_FUNC_START("          ");

	if (l2 == NULL) {
		return;
	}

	os_free(l2);

	TX_FUNC_END("          ");
}

extern struct softmac *da16x_softmac;

void l2_packet_receive(struct l2_packet_data *l2)
{
#ifdef	L2_PACKET_OLD_TYPE
	UINT	status, status_q;
#endif	/* L2_PACKET_OLD_TYPE */
	extern void wpa_supplicant_rx_eapol(void *ctx, const u8 *src_addr,
					const u8 *buf, size_t len);
#ifdef	L2_PACKET_OLD_TYPE

	/* receive l2_packet rx message */
	status = xSemaphoreTake(l2_pkt_mutex, 3 * ONE_SEC_TICK);
	if (status != pdTRUE) {
		da16x_eapol_prt("<%s> xSemaphoreTake() fail (%d)\n",
						__func__, status);
		return;
	}

	status_q = tx_queue_receive(&da16x_l2_pkt_rx_q,
				l2_pkt_rx_buf_p, portNO_DELAY);

	/* for mutex free between CLI and Supplicant */
	status = xSemaphoreGive(l2_pkt_mutex);
	if (status != pdTRUE) {
		da16x_eapol_prt("<%s> xSemaphoreGive() fail (%d)\n",
						__func__, status);
		return;
	}

	if (status_q == ERR_OK) {
		/* handle l2_packet for eapol */
		wpa_supplicant_rx_eapol(l2->rx_callback_ctx,
				l2_pkt_rx_buf_p->src_mac_addr,
				l2_pkt_rx_buf_p->pkt_buf,
				l2_pkt_rx_buf_p->pkt_len);

		/* free l2_packet buffer :
		 *	this buffer was allocated in UMAC */
		vPortFree(l2_pkt_rx_buf_p->pkt_buf);
	} else {
		da16x_err_prt("<%s> l2_pkt_rx_msg_q error\n", __func__);
	}
#else
        struct wpktbuff *wpkt;
        struct i3ed11_macctrl *macctrl = da16x_softmac->macctrl; 
        u8 dst[ETH_ALEN];
	struct wpa_supplicant *wpa_s = l2->rx_callback_ctx;
        
        while (wpkt = wpkt_dequeue(&macctrl->wpkt_queue_to_sp)) {
                
                struct i3ed11_hdr *hdr = (struct i3ed11_hdr *) wpkt->data; 
                u16 hdrlen = i3ed11_hdrlen(hdr->frame_control);

		memcpy(dst, i3ed11_get_DA(hdr), ETH_ALEN);
		
		da16x_eapol_prt("<%s> dst = " MACSTR "\n", __func__, MAC2STR(dst));
		da16x_eapol_prt("<%s> wpa_s->own_addr = " MACSTR "\n",
			__func__, MAC2STR(wpa_s->own_addr));

		for (wpa_s = wpa_s->global->ifaces; wpa_s ; wpa_s = wpa_s->next) {
			if (os_memcmp(dst, wpa_s->own_addr, ETH_ALEN) == 0) {
				wpa_supplicant_rx_eapol(wpa_s,
			                i3ed11_get_SA(hdr),
			                wpkt->data+hdrlen+6+2,
			                wpkt->len-hdrlen-6-2);
				/*by Jin*/
				break;
			}
		}
			
		dev_kfree_wpkt(wpkt);
	}
#endif	/* L2_PACKET_OLD_TYPE */
}

int l2_packet_send(struct l2_packet_data *l2, const u8 *dst_addr, u16 proto,
		   const u8 *buf, size_t len, int is_hs_ack)
{
	TX_FUNC_START("");
	int ret = 0;

	if (l2 == NULL)
		return -1;

	if (l2->l2_hdr) {
		da16x_eapol_prt("[%s] What do I do for l2_hdr exist case ???\n",
						__func__);
	}

	/* direct function call to UMAC */
	ret = i3ed11_l2data_from_sp(l2->ifindex, (unsigned char *)buf, (unsigned int)len, FALSE,
				 (const char *)dst_addr, is_hs_ack);

	TX_FUNC_END("");

	return ret;
}

int l2_packet_get_own_addr(struct l2_packet_data *l2, u8 *addr)
{
	memcpy(addr, l2->own_addr, ETH_ALEN);

	return 0;
}

int l2_packet_get_ip_addr(struct l2_packet_data *l2, char *buf, size_t len)
{
	return 0;
}

void l2_packet_notify_auth_start(struct l2_packet_data *l2)
{
}

/* EOF */
