/*
 * Wi-Fi Direct - P2P service discovery
 * Copyright (c) 2009, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_P2P

#include "common/ieee802_11_defs.h"
#ifdef CONFIG_GAS
#include "common/gas.h"
#endif /* CONFIG_GAS */
#include "p2p_i.h"
#include "supp_p2p.h"

#ifdef	CONFIG_P2P_OPTION

#ifdef CONFIG_WIFI_DISPLAY
static int wfd_wsd_supported(struct wpabuf *wfd)
{
	const u8 *pos, *end;
	u8 subelem;
	u16 len;

	if (wfd == NULL)
		return 0;

	pos = wpabuf_head(wfd);
	end = pos + wpabuf_len(wfd);

	while (pos + 3 <= end) {
		subelem = *pos++;
		len = WPA_GET_BE16(pos);
		pos += 2;
		if (pos + len > end)
			break;

		if (subelem == WFD_SUBELEM_DEVICE_INFO && len >= 6) {
			u16 info = WPA_GET_BE16(pos);
			return !!(info & 0x0040);
		}

		pos += len;
	}

	return 0;
}
#endif /* CONFIG_WIFI_DISPLAY */

struct p2p_sd_query * p2p_pending_sd_req(struct p2p_data *p2p,
					 struct p2p_device *dev)
{
	struct p2p_sd_query *q;
	int wsd = 0;
	int count = 0;

	if (!(dev->info.dev_capab & P2P_DEV_CAPAB_SERVICE_DISCOVERY))
		return NULL; /* peer does not support SD */
#ifdef CONFIG_WIFI_DISPLAY
	if (wfd_wsd_supported(dev->info.wfd_subelems))
		wsd = 1;
#endif /* CONFIG_WIFI_DISPLAY */

	for (q = p2p->sd_queries; q; q = q->next) {
		/* Use WSD only if the peer indicates support or it */
		if (q->wsd && !wsd)
			continue;
		/* if the query is a broadcast query */
		if (q->for_all_peers) {
			/*
			 * check if there are any broadcast queries pending for
			 * this device
			 */
			if (dev->sd_pending_bcast_queries <= 0)
				return NULL;
			/* query number that needs to be send to the device */
			if (count == dev->sd_pending_bcast_queries - 1)
				return q;
			count++;
		}
		if (!q->for_all_peers &&
		    os_memcmp(q->peer, dev->info.p2p_device_addr, ETH_ALEN) ==
		    0)
			return q;
	}

	return NULL;
}


static void p2p_decrease_sd_bc_queries(struct p2p_data *p2p, int query_number)
{
	struct p2p_device *dev;

	p2p->num_p2p_sd_queries--;
	dl_list_for_each(dev, &p2p->devices, struct p2p_device, list) {
		if (query_number <= dev->sd_pending_bcast_queries - 1) {
			/*
			 * Query not yet sent to the device and it is to be
			 * removed, so update the pending count.
			*/
			dev->sd_pending_bcast_queries--;
		}
	}
}


static int p2p_unlink_sd_query(struct p2p_data *p2p,
			       struct p2p_sd_query *query)
{
	struct p2p_sd_query *q, *prev;
	int query_number = 0;

	q = p2p->sd_queries;
	prev = NULL;
	while (q) {
		if (q == query) {
			/* If the query is a broadcast query, decrease one from
			 * all the devices */
			if (query->for_all_peers)
				p2p_decrease_sd_bc_queries(p2p, query_number);
			if (prev)
				prev->next = q->next;
			else
				p2p->sd_queries = q->next;
			if (p2p->sd_query == query)
				p2p->sd_query = NULL;
			return 1;
		}
		if (q->for_all_peers)
			query_number++;
		prev = q;
		q = q->next;
	}
	return 0;
}


static void p2p_free_sd_query(struct p2p_sd_query *q)
{
	if (q == NULL)
		return;
	wpabuf_free(q->tlvs);
	os_free(q);
}


void p2p_free_sd_queries(struct p2p_data *p2p)
{
	struct p2p_sd_query *q, *prev;
	q = p2p->sd_queries;
	p2p->sd_queries = NULL;
	while (q) {
		prev = q;
		q = q->next;
		p2p_free_sd_query(prev);
	}
	p2p->num_p2p_sd_queries = 0;
}


#ifdef CONFIG_GAS
static struct wpabuf *p2p_build_sd_query(u16 update_indic, struct wpabuf *tlvs)
{
	struct wpabuf *buf;
	u8 *len_pos;

	buf = gas_anqp_build_initial_req(0, 100 + wpabuf_len(tlvs));
	if (buf == NULL)
		return NULL;

	/* ANQP Query Request Frame */
	len_pos = gas_anqp_add_element(buf, ANQP_VENDOR_SPECIFIC);
	wpabuf_put_be32(buf, P2P_IE_VENDOR_TYPE);
	wpabuf_put_le16(buf, update_indic); /* Service Update Indicator */
	wpabuf_put_buf(buf, tlvs);
	gas_anqp_set_element_len(buf, len_pos);

	gas_anqp_set_len(buf);

	return buf;
}


static void p2p_send_gas_comeback_req(struct p2p_data *p2p, const u8 *dst,
				      u8 dialog_token, int freq)
{
	struct wpabuf *req;

	req = gas_build_comeback_req(dialog_token);
	if (req == NULL)
		return;

	p2p->pending_action_state = P2P_NO_PENDING_ACTION;
	if (p2p_send_action(p2p, freq, dst, p2p->cfg->dev_addr, dst,
			    wpabuf_head(req), wpabuf_len(req), 200) < 0)
		da16x_p2p_prt("[%s] Failed to send Action frame\n", __func__);

	wpabuf_free(req);
}


static struct wpabuf * p2p_build_sd_response(u8 dialog_token, u16 status_code,
					     u16 comeback_delay,
					     u16 update_indic,
					     const struct wpabuf *tlvs)
{
	struct wpabuf *buf;
	u8 *len_pos;

	buf = gas_anqp_build_initial_resp(dialog_token, status_code,
					  comeback_delay,
					  100 + (tlvs ? wpabuf_len(tlvs) : 0));
	if (buf == NULL)
		return NULL;

	if (tlvs) {
		/* ANQP Query Response Frame */
		len_pos = gas_anqp_add_element(buf, ANQP_VENDOR_SPECIFIC);
		wpabuf_put_be32(buf, P2P_IE_VENDOR_TYPE);
		 /* Service Update Indicator */
		wpabuf_put_le16(buf, update_indic);
		wpabuf_put_buf(buf, tlvs);
		gas_anqp_set_element_len(buf, len_pos);
	}

	gas_anqp_set_len(buf);

	return buf;
}


static struct wpabuf * p2p_build_gas_comeback_resp(u8 dialog_token,
						   u16 status_code,
						   u16 update_indic,
						   const u8 *data, size_t len,
						   u8 frag_id, u8 more,
						   u16 total_len)
{
	struct wpabuf *buf;

	buf = gas_anqp_build_comeback_resp(dialog_token, status_code, frag_id,
					   more, 0, 100 + len);
	if (buf == NULL)
		return NULL;

	if (frag_id == 0) {
		/* ANQP Query Response Frame */
		wpabuf_put_le16(buf, ANQP_VENDOR_SPECIFIC); /* Info ID */
		wpabuf_put_le16(buf, 3 + 1 + 2 + total_len);
		wpabuf_put_be32(buf, P2P_IE_VENDOR_TYPE);
		/* Service Update Indicator */
		wpabuf_put_le16(buf, update_indic);
	}

	wpabuf_put_data(buf, data, len);
	gas_anqp_set_len(buf);

	return buf;
}
#endif /* CONFIG_GAS */


int p2p_start_sd(struct p2p_data *p2p, struct p2p_device *dev)
{
	struct wpabuf *req = NULL;
	int ret = 0;
	struct p2p_sd_query *query;
	int freq;

	freq = dev->listen_freq > 0 ? dev->listen_freq : dev->oper_freq;
	if (freq <= 0) {
		da16x_p2p_prt("[%s] No Listen/Operating frequency known for "
			"the peer " MACSTR " to send SD Request\n",
			__func__,
			MAC2STR(dev->info.p2p_device_addr));
		return -1;
	}

	query = p2p_pending_sd_req(p2p, dev);
	if (query == NULL)
		return -1;

	da16x_p2p_prt("[%s] Start Service Discovery with " MACSTR "\n",
		__func__, MAC2STR(dev->info.p2p_device_addr));

#ifdef CONFIG_GAS
	req = p2p_build_sd_query(p2p->srv_update_indic, query->tlvs);
#endif /* CONFIG_GAS */
	if (req == NULL) {
		da16x_p2p_prt("[%s]  req is NULL\n", __func__);
		return -1;
	}

	p2p->sd_peer = dev;
	p2p->sd_query = query;
	p2p->pending_action_state = P2P_PENDING_SD;

	if (p2p_send_action(p2p, freq, dev->info.p2p_device_addr,
			    p2p->cfg->dev_addr, dev->info.p2p_device_addr,
			    wpabuf_head(req), wpabuf_len(req), 5000) < 0) {
		da16x_p2p_prt("[%s] Failed to send Action frame\n",
				__func__);
		ret = -1;
	}

	/* Update the pending broadcast SD query count for this device */
	dev->sd_pending_bcast_queries--;

	/*
	 * If there are no pending broadcast queries for this device, mark it as
	 * done (-1).
	 */
	if (dev->sd_pending_bcast_queries == 0)
		dev->sd_pending_bcast_queries = -1;

	wpabuf_free(req);

	return ret;
}


#ifdef CONFIG_GAS
void p2p_rx_gas_initial_req(struct p2p_data *p2p, const u8 *sa,
			    const u8 *data, size_t len, int rx_freq)
{
	const u8 *pos = data;
	const u8 *end = data + len;
	const u8 *next;
	u8 dialog_token;
	u16 slen;
	int freq;
	u16 update_indic;


	if (p2p->cfg->sd_request == NULL)
		return;

	if (rx_freq > 0)
		freq = rx_freq;
	else
		freq = p2p_channel_to_freq(p2p->cfg->reg_class,
					   p2p->cfg->channel);
	if (freq < 0)
		return;

	if (len < 1 + 2)
		return;

	dialog_token = *pos++;
	da16x_p2p_prt("[%s] GAS Initial Request from " MACSTR
		" (dialog token %u, freq %d)\n",
		__func__, MAC2STR(sa), dialog_token, rx_freq);

	if (*pos != WLAN_EID_ADV_PROTO) {
		da16x_p2p_prt("[%s] Unexpected IE in GAS Initial Request: %u\n",
			__func__, *pos);
		return;
	}
	pos++;

	slen = *pos++;
	next = pos + slen;
	if (next > end || slen < 2) {
		da16x_p2p_prt("[%s] Invalid IE in GAS Initial Request\n",
				__func__);
		return;
	}
	pos++; /* skip QueryRespLenLimit and PAME-BI */

	if (*pos != ACCESS_NETWORK_QUERY_PROTOCOL) {
		da16x_p2p_prt("[%s] Unsupported GAS advertisement "
			"protocol id %u\n", __func__, *pos);

		return;
	}

	pos = next;
	/* Query Request */
	if (pos + 2 > end)
		return;
	slen = WPA_GET_LE16(pos);
	pos += 2;
	if (pos + slen > end)
		return;
	end = pos + slen;

	/* ANQP Query Request */
	if (pos + 4 > end)
		return;
	if (WPA_GET_LE16(pos) != ANQP_VENDOR_SPECIFIC) {
		da16x_p2p_prt("[%s] Unsupported ANQP Info ID %u\n",
				__func__, WPA_GET_LE16(pos));
		return;
	}
	pos += 2;

	slen = WPA_GET_LE16(pos);
	pos += 2;
	if (pos + slen > end || slen < 3 + 1) {
		da16x_p2p_prt("[%s] Invalid ANQP Query Request length\n",
			__func__);
		return;
	}

	if (WPA_GET_BE32(pos) != P2P_IE_VENDOR_TYPE) {
		da16x_p2p_prt("[%s] Unsupported ANQP vendor OUI-type %08x\n",
			__func__, WPA_GET_BE32(pos));
		return;
	}
	pos += 4;

	if (pos + 2 > end)
		return;
	update_indic = WPA_GET_LE16(pos);
	da16x_p2p_prt("[%s] Service Update Indicator: %u\n",
			__func__, update_indic);
	pos += 2;

	p2p->cfg->sd_request(p2p->cfg->cb_ctx, freq, sa, dialog_token,
			     update_indic, pos, end - pos);
	/* the response will be indicated with a call to p2p_sd_response() */
}
#endif /* CONFIG_GAS */


void p2p_sd_response(struct p2p_data *p2p, int freq, const u8 *dst,
		     u8 dialog_token, const struct wpabuf *resp_tlvs)
{
	struct wpabuf *resp = NULL;
	unsigned int wait_time = 200;

	/* TODO: fix the length limit to match with the maximum frame length */
	if (wpabuf_len(resp_tlvs) > 1400) {
		da16x_p2p_prt("[%s] SD response long enough to require "
			"fragmentation\n", __func__);
		if (p2p->sd_resp) {
			/*
			 * TODO: Could consider storing the fragmented response
			 * separately for each peer to avoid having to drop old
			 * one if there is more than one pending SD query.
			 * Though, that would eat more memory, so there are
			 * also benefits to just using a single buffer.
			 */
			da16x_p2p_prt("[%s] Drop previous SD response\n",
					__func__);
			wpabuf_free(p2p->sd_resp);
		}
		p2p->sd_resp = wpabuf_dup(resp_tlvs);
		if (p2p->sd_resp == NULL) {
			da16x_p2p_prt("[%s] Failed to allocate SD response "
				"fragmentation area\n", __func__);
			return;
		}
		os_memcpy(p2p->sd_resp_addr, dst, ETH_ALEN);
		p2p->sd_resp_dialog_token = dialog_token;
		p2p->sd_resp_pos = 0;
		p2p->sd_frag_id = 0;

#ifdef CONFIG_GAS
		resp = p2p_build_sd_response(dialog_token, WLAN_STATUS_SUCCESS,
					     1, p2p->srv_update_indic, NULL);
#endif /* CONFIG_GAS */
	} else {
		da16x_p2p_prt("[%s] SD response fits in initial response\n",
				__func__);
		wait_time = 0; /* no more SD frames in the sequence */
#ifdef CONFIG_GAS
		resp = p2p_build_sd_response(dialog_token,
					     WLAN_STATUS_SUCCESS, 0,
					     p2p->srv_update_indic, resp_tlvs);
#endif /* CONFIG_GAS */
	}
	if (resp == NULL) {
		da16x_p2p_prt("[%s] resp is NULL\n", __func__);
		return;
	}

	p2p->pending_action_state = P2P_NO_PENDING_ACTION;
	if (p2p_send_action(p2p, freq, dst, p2p->cfg->dev_addr,
			    p2p->cfg->dev_addr,
			    wpabuf_head(resp), wpabuf_len(resp), wait_time) < 0)
		da16x_p2p_prt("[%s] Failed to send Action frame\n", __func__);

	wpabuf_free(resp);
}


#ifdef CONFIG_GAS
void p2p_rx_gas_initial_resp(struct p2p_data *p2p, const u8 *sa,
			     const u8 *data, size_t len, int rx_freq)
{
	const u8 *pos = data;
	const u8 *end = data + len;
	const u8 *next;
	u8 dialog_token;
	u16 status_code;
	u16 comeback_delay;
	u16 slen;
	u16 update_indic;

	if (p2p->state != P2P_SD_DURING_FIND || p2p->sd_peer == NULL ||
	    os_memcmp(sa, p2p->sd_peer->info.p2p_device_addr, ETH_ALEN) != 0) {
		da16x_p2p_prt("[%s] Ignore unexpected GAS Initial Response from "
			MACSTR "\n", __func__, MAC2STR(sa));

		return;
	}
	p2p->cfg->send_action_done(p2p->cfg->cb_ctx);
	p2p_clear_timeout(p2p);

	da16x_p2p_prt("[%s] Received GAS Initial Response from "
			MACSTR " (len=%d)\n",
			__func__, MAC2STR(sa), (int) len);

	if (len < 5 + 2) {
		da16x_p2p_prt("[%s] Too short GAS Initial Response frame\n",
				__func__);
		return;
	}

	dialog_token = *pos++;
	/* TODO: check dialog_token match */
	status_code = WPA_GET_LE16(pos);
	pos += 2;
	comeback_delay = WPA_GET_LE16(pos);
	pos += 2;

	da16x_p2p_prt("[%s] dialog_token=%u status_code=%u comeback_delay=%u\n",
		__func__, dialog_token, status_code, comeback_delay);

	if (status_code) {
		da16x_p2p_prt("[%s] Service Discovery failed: status code %u\n",
			__func__, status_code);

		return;
	}

	if (*pos != WLAN_EID_ADV_PROTO) {
		da16x_p2p_prt("[%s] Unexpected IE in GAS Initial Response: %u\n",
			     __func__,  *pos);
		return;
	}
	pos++;

	slen = *pos++;
	next = pos + slen;
	if (next > end || slen < 2) {
		da16x_p2p_prt("[%s] Invalid IE in GAS Initial Response\n",
				__func__);
		return;
	}
	pos++; /* skip QueryRespLenLimit and PAME-BI */

	if (*pos != ACCESS_NETWORK_QUERY_PROTOCOL) {
		da16x_p2p_prt("[%s] Unsupported GAS advertisement "
			"protocol id %u\n", __func__, *pos);

		return;
	}

	pos = next;
	/* Query Response */
	if (pos + 2 > end) {
		da16x_p2p_prt("[%s] Too short Query Response\n", __func__);
		return;
	}
	slen = WPA_GET_LE16(pos);
	pos += 2;
	da16x_p2p_prt("[%s] Query Response Length: %d\n", __func__, slen);

	if (pos + slen > end) {
		da16x_p2p_prt("[%s] Not enough Query Response data\n",
				__func__);
		return;
	}
	end = pos + slen;

	if (comeback_delay) {
		da16x_p2p_prt("[%s] Fragmented response - request fragments\n",
				__func__);
		if (p2p->sd_rx_resp) {
			da16x_p2p_prt("[%s] Drop old SD reassembly buffer\n",
					__func__);
			wpabuf_free(p2p->sd_rx_resp);
			p2p->sd_rx_resp = NULL;
		}
		p2p_send_gas_comeback_req(p2p, sa, dialog_token, rx_freq);
		return;
	}

	/* ANQP Query Response */
	if (pos + 4 > end)
		return;
	if (WPA_GET_LE16(pos) != ANQP_VENDOR_SPECIFIC) {
		da16x_p2p_prt("[%s] Unsupported ANQP Info ID %u\n",
				__func__, WPA_GET_LE16(pos));
		return;
	}
	pos += 2;

	slen = WPA_GET_LE16(pos);
	pos += 2;
	if (pos + slen > end || slen < 3 + 1) {
		da16x_p2p_prt("[%s] Invalid ANQP Query Response length\n",
				__func__);
		return;
	}

	if (WPA_GET_BE32(pos) != P2P_IE_VENDOR_TYPE) {
		da16x_p2p_prt("[%s] Unsupported ANQP vendor OUI-type %08x\n",
			__func__, WPA_GET_BE32(pos));
		return;
	}
	pos += 4;

	if (pos + 2 > end)
		return;
	update_indic = WPA_GET_LE16(pos);
	da16x_p2p_prt("[%s] Service Update Indicator: %u\n",
			__func__, update_indic);

	pos += 2;

	p2p->sd_peer = NULL;

	if (p2p->sd_query) {
		if (!p2p->sd_query->for_all_peers) {
			struct p2p_sd_query *q;
			da16x_p2p_prt("[%s] Remove completed SD query %p\n",
				__func__, p2p->sd_query);

			q = p2p->sd_query;
			p2p_unlink_sd_query(p2p, p2p->sd_query);
			p2p_free_sd_query(q);
		}
		p2p->sd_query = NULL;
	}

	if (p2p->cfg->sd_response)
		p2p->cfg->sd_response(p2p->cfg->cb_ctx, sa, update_indic,
				      pos, end - pos);
	p2p_continue_find(p2p);
}


void p2p_rx_gas_comeback_req(struct p2p_data *p2p, const u8 *sa,
			     const u8 *data, size_t len, int rx_freq)
{
	struct wpabuf *resp;
	u8 dialog_token;
	size_t frag_len;
	int more = 0;
	unsigned int wait_time = 200;

	wpa_hexdump(MSG_DEBUG, "P2P: RX GAS Comeback Request", data, len);
	if (len < 1)
		return;
	dialog_token = *data;

	da16x_p2p_prt("Dialog Token: %u\n", __func__, dialog_token);

	if (dialog_token != p2p->sd_resp_dialog_token) {
		da16x_p2p_prt("[%s] No pending SD response fragment for "
			"dialog token %u\n",
			__func__, dialog_token);

		return;
	}

	if (p2p->sd_resp == NULL) {
		da16x_p2p_prt("[%s] No pending SD response fragment available\n",
				__func__);
		return;
	}
	if (os_memcmp(sa, p2p->sd_resp_addr, ETH_ALEN) != 0) {
		da16x_p2p_prt("[%s] No pending SD response fragment for "
			MACSTR "\n", __func__, MAC2STR(sa));
		return;
	}

	frag_len = wpabuf_len(p2p->sd_resp) - p2p->sd_resp_pos;
	if (frag_len > 1400) {
		frag_len = 1400;
		more = 1;
	}
	resp = p2p_build_gas_comeback_resp(dialog_token, WLAN_STATUS_SUCCESS,
					   p2p->srv_update_indic,
					   wpabuf_head_u8(p2p->sd_resp) +
					   p2p->sd_resp_pos, frag_len,
					   p2p->sd_frag_id, more,
					   wpabuf_len(p2p->sd_resp));
	if (resp == NULL)
		return;

	da16x_p2p_prt("[%s] Send GAS Comeback Response (frag_id %d more=%d "
			"frag_len=%d)\n",
			__func__, p2p->sd_frag_id, more, (int) frag_len);

	p2p->sd_frag_id++;
	p2p->sd_resp_pos += frag_len;

	if (more) {
		da16x_p2p_prt("[%s] %d more bytes remain to be sent\n",
			(int) (wpabuf_len(p2p->sd_resp) - p2p->sd_resp_pos));
	} else {
		da16x_p2p_prt("[%s] All fragments of SD response sent\n",
				__func__);
		wpabuf_free(p2p->sd_resp);
		p2p->sd_resp = NULL;
		wait_time = 0; /* no more SD frames in the sequence */
	}

	p2p->pending_action_state = P2P_NO_PENDING_ACTION;
	if (p2p_send_action(p2p, rx_freq, sa, p2p->cfg->dev_addr,
			    p2p->cfg->dev_addr,
			    wpabuf_head(resp), wpabuf_len(resp), wait_time) < 0)
		da16x_p2p_prt("[%s] Failed to send Action frame\n", __func__);

	wpabuf_free(resp);
}


void p2p_rx_gas_comeback_resp(struct p2p_data *p2p, const u8 *sa,
			      const u8 *data, size_t len, int rx_freq)
{
	const u8 *pos = data;
	const u8 *end = data + len;
	const u8 *next;
	u8 dialog_token;
	u16 status_code;
	u8 frag_id;
	u8 more_frags;
	u16 comeback_delay;
	u16 slen;

	wpa_hexdump(MSG_DEBUG, "P2P: RX GAS Comeback Response", data, len);

	if (p2p->state != P2P_SD_DURING_FIND || p2p->sd_peer == NULL ||
	    os_memcmp(sa, p2p->sd_peer->info.p2p_device_addr, ETH_ALEN) != 0) {
		da16x_p2p_prt("[%s] Ignore unexpected GAS Comeback "
			"Response from " MACSTR "\n", __func__, MAC2STR(sa));
		return;
	}
	p2p->cfg->send_action_done(p2p->cfg->cb_ctx);
	p2p_clear_timeout(p2p);

	da16x_p2p_prt("[%s] Received GAS Comeback Response from "
			MACSTR " (len=%d)\n",
			__func__, MAC2STR(sa), (int) len);

	if (len < 6 + 2) {
		da16x_p2p_prt("[%s] Too short GAS Comeback Response frame\n",
				__func__);
		return;
	}

	dialog_token = *pos++;
	/* TODO: check dialog_token match */
	status_code = WPA_GET_LE16(pos);
	pos += 2;
	frag_id = *pos & 0x7f;
	more_frags = (*pos & 0x80) >> 7;
	pos++;
	comeback_delay = WPA_GET_LE16(pos);
	pos += 2;

	da16x_p2p_prt("[%s] dialog_token=%u status_code=%u frag_id=%d "
			"more_frags=%d comeback_delay=%u\n",
			__func__,
			dialog_token, status_code, frag_id, more_frags,
			comeback_delay);

	/* TODO: check frag_id match */
	if (status_code) {
		da16x_p2p_prt("[%s] Service Discovery failed: status code %u\n",
			__func__, status_code);
		return;
	}

	if (*pos != WLAN_EID_ADV_PROTO) {
		da16x_p2p_prt("[%s] Unexpected IE in GAS Comeback "
			"Response: %u\n", __func__, *pos);
		return;
	}
	pos++;

	slen = *pos++;
	next = pos + slen;
	if (next > end || slen < 2) {
		da16x_p2p_prt("[%s] Invalid IE in GAS Comeback Response\n",
				__func__);
		return;
	}
	pos++; /* skip QueryRespLenLimit and PAME-BI */

	if (*pos != ACCESS_NETWORK_QUERY_PROTOCOL) {
		da16x_p2p_prt("[%s] Unsupported GAS advertisement "
			"protocol id %u\n", __func__, *pos);
		return;
	}

	pos = next;
	/* Query Response */
	if (pos + 2 > end) {
		da16x_p2p_prt("[%s] Too short Query Response\n", __func__);
		return;
	}
	slen = WPA_GET_LE16(pos);
	pos += 2;

	da16x_p2p_prt("[%s] Query Response Length: %d\n", __func__, slen);

	if (pos + slen > end) {
		da16x_p2p_prt("[%s] Not enough Query Response data\n", __func__);
		return;
	}
	if (slen == 0) {
		da16x_p2p_prt("[%s] No Query Response data\n", __func__);
		return;
	}
	end = pos + slen;

	if (p2p->sd_rx_resp) {
		 /*
		  * ANQP header is only included in the first fragment; rest of
		  * the fragments start with continue TLVs.
		  */
		goto skip_nqp_header;
	}

	/* ANQP Query Response */
	if (pos + 4 > end)
		return;
	if (WPA_GET_LE16(pos) != ANQP_VENDOR_SPECIFIC) {
		da16x_p2p_prt("[%s] Unsupported ANQP Info ID %u\n",
				__func__, WPA_GET_LE16(pos));
		return;
	}
	pos += 2;

	slen = WPA_GET_LE16(pos);
	pos += 2;

	da16x_p2p_prt("[%s] ANQP Query Response length: %u\n",
			__func__, slen);
	if (slen < 3 + 1) {
		da16x_p2p_prt("[%s] Invalid ANQP Query Response length\n",
			__func__);
		return;
	}
	if (pos + 4 > end)
		return;

	if (WPA_GET_BE32(pos) != P2P_IE_VENDOR_TYPE) {
		da16x_p2p_prt("[%s] Unsupported ANQP vendor OUI-type %08x\n",
			__func__, WPA_GET_BE32(pos));
		return;
	}
	pos += 4;

	if (pos + 2 > end)
		return;
	p2p->sd_rx_update_indic = WPA_GET_LE16(pos);
	da16x_p2p_prt("[%s] Service Update Indicator: %u\n",
			__func__, p2p->sd_rx_update_indic);
	pos += 2;

skip_nqp_header:
	if (wpabuf_resize(&p2p->sd_rx_resp, end - pos) < 0)
		return;
	wpabuf_put_data(p2p->sd_rx_resp, pos, end - pos);
	da16x_p2p_prt("[%s] Current SD reassembly buffer length: %u\n",
		__func__, (unsigned int) wpabuf_len(p2p->sd_rx_resp));

	if (more_frags) {
		da16x_p2p_prt("[%s] More fragments remains\n", __func__);
		/* TODO: what would be a good size limit? */
		if (wpabuf_len(p2p->sd_rx_resp) > 64000) {
			wpabuf_free(p2p->sd_rx_resp);
			p2p->sd_rx_resp = NULL;
			da16x_p2p_prt("[%s] Too long SD response - drop it\n",
					__func__);
			return;
		}
		p2p_send_gas_comeback_req(p2p, sa, dialog_token, rx_freq);
		return;
	}

	p2p->sd_peer = NULL;

	if (p2p->sd_query) {
		if (!p2p->sd_query->for_all_peers) {
			struct p2p_sd_query *q;

			da16x_p2p_prt("[%s] Remove completed SD query %p\n",
				__func__, p2p->sd_query);

			q = p2p->sd_query;
			p2p_unlink_sd_query(p2p, p2p->sd_query);
			p2p_free_sd_query(q);
		}
		p2p->sd_query = NULL;
	}

	if (p2p->cfg->sd_response)
		p2p->cfg->sd_response(p2p->cfg->cb_ctx, sa,
				      p2p->sd_rx_update_indic,
				      wpabuf_head(p2p->sd_rx_resp),
				      wpabuf_len(p2p->sd_rx_resp));
	wpabuf_free(p2p->sd_rx_resp);
	p2p->sd_rx_resp = NULL;

	p2p_continue_find(p2p);
}
#endif /* CONFIG_GAS */


void * p2p_sd_request(struct p2p_data *p2p, const u8 *dst,
		      const struct wpabuf *tlvs)
{
	struct p2p_sd_query *q;

	q = os_zalloc(sizeof(*q));
	if (q == NULL)
		return NULL;

	if (dst)
		os_memcpy(q->peer, dst, ETH_ALEN);
	else
		q->for_all_peers = 1;

	q->tlvs = wpabuf_dup(tlvs);
	if (q->tlvs == NULL) {
		p2p_free_sd_query(q);
		return NULL;
	}

	q->next = p2p->sd_queries;
	p2p->sd_queries = q;

	da16x_p2p_prt("[%s] Added SD Query %p\n", __func__, q);

	if (dst == NULL) {
		struct p2p_device *dev;

		p2p->num_p2p_sd_queries++;

		/* Update all the devices for the newly added broadcast query */
		dl_list_for_each(dev, &p2p->devices, struct p2p_device, list) {
			if (dev->sd_pending_bcast_queries <= 0)
				dev->sd_pending_bcast_queries = 1;
			else
				dev->sd_pending_bcast_queries++;
		}
	}

	return q;
}


#ifdef CONFIG_WIFI_DISPLAY
void * p2p_sd_request_wfd(struct p2p_data *p2p, const u8 *dst,
			  const struct wpabuf *tlvs)
{
	struct p2p_sd_query *q;
	q = p2p_sd_request(p2p, dst, tlvs);
	if (q)
		q->wsd = 1;
	return q;
}
#endif /* CONFIG_WIFI_DISPLAY */


void p2p_sd_service_update(struct p2p_data *p2p)
{
	p2p->srv_update_indic++;
}


int p2p_sd_cancel_request(struct p2p_data *p2p, void *req)
{
	if (p2p_unlink_sd_query(p2p, req)) {
		da16x_p2p_prt("[%s] Cancel pending SD query %p\n",
				__func__, req);
		p2p_free_sd_query(req);
		return 0;
	}
	return -1;
}

#endif	/* CONFIG_P2P_OPTION */

#endif	/* CONFIG_P2P */

/* EOF */
