/*
 * Wi-Fi Direct - P2P Group Owner Negotiation
 * Copyright (c) 2009-2010, Atheros Communications
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	CONFIG_P2P

#include "common/ieee802_11_defs.h"
#include "wps/wps_defs.h"
#include "p2p_i.h"
#include "supp_p2p.h"
#include "supp_config.h"
#include "hostapd.h"
#include "wpa_supplicant_i.h"

static int p2p_go_det(u8 own_intent, u8 peer_value)
{
	u8 peer_intent = peer_value >> 1;
	if (own_intent == peer_intent) {
		if (own_intent == P2P_MAX_GO_INTENT)
			return -1; /* both devices want to become GO */

		/* Use tie breaker bit to determine GO */
		return (peer_value & 0x01) ? 0 : 1;
	}

	return own_intent > peer_intent;
}


int p2p_peer_channels_check(struct p2p_data *p2p, struct p2p_channels *own,
			    struct p2p_device *dev,
			    const u8 *channel_list, size_t channel_list_len)
{
	const u8 *pos, *end;
	struct p2p_channels *ch;
	size_t channels;
	struct p2p_channels intersection;

	ch = &dev->channels;
	os_memset(ch, 0, sizeof(*ch));
	pos = channel_list;
	end = channel_list + channel_list_len;

	if (end - pos < 3)
		return -1;
	os_memcpy(dev->country, pos, 3);

	da16x_ascii_dump("P2P: Peer country", pos, 3);

	if (pos[2] != 0x04 && os_memcmp(pos, p2p->cfg->country, 2) != 0) {
		da16x_p2p_prt("[%s] Mismatching country (ours=%c%c "
			"peer's=%c%c)\n",
			__func__,
			p2p->cfg->country[0], p2p->cfg->country[1],
			pos[0], pos[1]);
		return -1;
	}
	pos += 3;

	while (pos + 2 < end) {
		struct p2p_reg_class *cl = &ch->reg_class[ch->reg_classes];
		cl->reg_class = *pos++;
		if (pos + 1 + pos[0] > end) {
			da16x_p2p_prt("[%s] Invalid peer Channel List\n",
					__func__);
			return -1;
		}
		channels = *pos++;
		cl->channels = channels > P2P_MAX_REG_CLASS_CHANNELS ?
			P2P_MAX_REG_CLASS_CHANNELS : channels;
		os_memcpy(cl->channel, pos, cl->channels);
		pos += channels;
		ch->reg_classes++;
		if (ch->reg_classes == P2P_MAX_REG_CLASSES)
			break;
	}

	p2p_channels_intersect(own, &dev->channels, &intersection);
	da16x_p2p_prt("[%s] Own reg_classes %d peer reg_classes %d "
			"intersection reg_classes %d\n", __func__,
		(int) own->reg_classes,
		(int) dev->channels.reg_classes,
		(int) intersection.reg_classes);

	if (intersection.reg_classes == 0) {
		da16x_p2p_prt("[%s] No common channels found\n", __func__);
		return -1;
	}
	return 0;
}


static int p2p_peer_channels(struct p2p_data *p2p, struct p2p_device *dev,
			     const u8 *channel_list, size_t channel_list_len)
{
	return p2p_peer_channels_check(p2p, &p2p->channels, dev,
				       channel_list, channel_list_len);
}


u16 p2p_wps_method_pw_id(enum p2p_wps_method wps_method)
{
	switch (wps_method) {
	case WPS_PIN_DISPLAY:
		return DEV_PW_REGISTRAR_SPECIFIED;
	case WPS_PIN_KEYPAD:
		return DEV_PW_USER_SPECIFIED;
	case WPS_PBC:
		return DEV_PW_PUSHBUTTON;
#if 0
	case WPS_NFC:
		return DEV_PW_NFC_CONNECTION_HANDOVER;
#endif	/* 0 */
	default:
		return DEV_PW_DEFAULT;
	}
}

#ifdef	ENABLE_P2P_DBG
static const char * p2p_wps_method_str(enum p2p_wps_method wps_method)
{
	switch (wps_method) {
	case WPS_PIN_DISPLAY:
		return "Display";
	case WPS_PIN_KEYPAD:
		return "Keypad";
	case WPS_PBC:
		return "PBC";
#if 0
	case WPS_NFC:
		return "NFC";
#endif	/* 0 */
	default:
		return "??";
	}
}
#endif	/* ENABLE_P2P_DBG */

static struct wpabuf * p2p_build_go_neg_req(struct p2p_data *p2p,
					    struct p2p_device *peer)
{
	struct wpabuf *buf;
	u8 *len;
	u8 group_capab;
	size_t extra = 0;
	u16 pw_id;

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		extra = wpabuf_len(p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */

	buf = wpabuf_alloc(1000 + extra);
	if (buf == NULL)
		return NULL;

	p2p_buf_add_public_action_hdr(buf, P2P_GO_NEG_REQ, peer->dialog_token);

	len = p2p_buf_add_ie_hdr(buf);
	group_capab = 0;
	if (peer->flags & P2P_DEV_PREFER_PERSISTENT_GROUP) {
		group_capab |= P2P_GROUP_CAPAB_PERSISTENT_GROUP;
		if (peer->flags & P2P_DEV_PREFER_PERSISTENT_RECONN)
			group_capab |= P2P_GROUP_CAPAB_PERSISTENT_RECONN;
	}
#ifdef	CONFIG_P2P_UNUSED_CMD
	if (p2p->cross_connect)
		group_capab |= P2P_GROUP_CAPAB_CROSS_CONN;
#endif	/* CONFIG_P2P_UNUSED_CMD */
	if (p2p->cfg->p2p_intra_bss)
		group_capab |= P2P_GROUP_CAPAB_INTRA_BSS_DIST;
	p2p_buf_add_capability(buf, p2p->dev_capab &
			       ~P2P_DEV_CAPAB_CLIENT_DISCOVERABILITY,
			       group_capab);
	p2p_buf_add_go_intent(buf, (p2p->go_intent << 1) | peer->tie_breaker);
	p2p_buf_add_config_timeout(buf, p2p->go_timeout, p2p->client_timeout);
	p2p_buf_add_listen_channel(buf, p2p->cfg->country, p2p->cfg->reg_class,
				   p2p->cfg->channel);
#ifdef	CONFIG_P2P_UNUSED_CMD
	if (p2p->ext_listen_interval)
		p2p_buf_add_ext_listen_timing(buf, p2p->ext_listen_period,
					      p2p->ext_listen_interval);
#endif	/* CONFIG_P2P_UNUSED_CMD */
	p2p_buf_add_intended_addr(buf, p2p->intended_addr);
	p2p_buf_add_channel_list(buf, p2p->cfg->country, &p2p->channels);
	p2p_buf_add_device_info(buf, p2p, peer);
	p2p_buf_add_operating_channel(buf, p2p->cfg->country,
				      p2p->op_reg_class, p2p->op_channel);
	p2p_buf_update_ie_hdr(buf, len);

	/* WPS IE with Device Password ID attribute */
	pw_id = p2p_wps_method_pw_id(peer->wps_method);
	if (peer->oob_pw_id)
		pw_id = peer->oob_pw_id;
	if (p2p_build_wps_ie(p2p, buf, pw_id, 0) < 0) {
		da16x_p2p_prt("[%s] Failed to build WPS IE for GO Negotiation "
			"Request\n", __func__);

		wpabuf_free(buf);
		return NULL;
	}

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		wpabuf_put_buf(buf, p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */

	return buf;
}


int p2p_connect_send(struct p2p_data *p2p, struct p2p_device *dev)
{
	struct wpabuf *req;
	int freq;

	if (dev->flags & P2P_DEV_PD_BEFORE_GO_NEG) {
		u16 config_method;

		da16x_p2p_prt("[%s] Use PD-before-GO-Neg workaround for "
			MACSTR "\n",
			__func__, MAC2STR(dev->info.p2p_device_addr));

		if (dev->wps_method == WPS_PIN_DISPLAY)
			config_method = WPS_CONFIG_KEYPAD;
		else if (dev->wps_method == WPS_PIN_KEYPAD)
			config_method = WPS_CONFIG_DISPLAY;
		else if (dev->wps_method == WPS_PBC)
			config_method = WPS_CONFIG_PUSHBUTTON;
		else
			return -1;

		return p2p_prov_disc_req(p2p, dev->info.p2p_device_addr,
					 config_method, 0, 0, 1);
	}

	freq = dev->listen_freq > 0 ? dev->listen_freq : dev->oper_freq;
	if (dev->oob_go_neg_freq > 0)
		freq = dev->oob_go_neg_freq;
	if (freq <= 0) {
		da16x_p2p_prt("[%s] No Listen/Operating frequency known "
			"for the peer " MACSTR
			" to send GO Negotiation Request\n",
			__func__,
			MAC2STR(dev->info.p2p_device_addr));

		return -1;
	}

	req = p2p_build_go_neg_req(p2p, dev);
	if (req == NULL)
		return -1;

	da16x_p2p_prt("[%s] Sending GO Negotiation Request\n", __func__);

	p2p_set_state(p2p, P2P_CONNECT);
	p2p->pending_action_state = P2P_PENDING_GO_NEG_REQUEST;
	p2p->go_neg_peer = dev;
	dev->flags |= P2P_DEV_WAIT_GO_NEG_RESPONSE;
	dev->connect_reqs++;

	if (p2p_send_action(p2p, freq, dev->info.p2p_device_addr,
			    p2p->cfg->dev_addr, dev->info.p2p_device_addr,
			    wpabuf_head(req), wpabuf_len(req),
			    DEFAULT_P2P_NEGO_WAIT_TIME) < 0) {
		da16x_p2p_prt("[%s] Failed to send Action frame\n",
				__func__);

		/* Use P2P find to recover and retry */
		p2p_set_timeout(p2p, 0, 0);
	} else {
		dev->go_neg_req_sent++;
	}

	wpabuf_free(req);

	return 0;
}


static struct wpabuf * p2p_build_go_neg_resp(struct p2p_data *p2p,
					     struct p2p_device *peer,
					     u8 dialog_token, u8 status,
					     u8 tie_breaker)
{
	struct wpabuf *buf;
	u8 *len;
	u8 group_capab;
	size_t extra = 0;
	u16 pw_id;

	da16x_p2p_prt("[%s] Building GO Negotiation Response\n", __func__);

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		extra = wpabuf_len(p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */

	buf = wpabuf_alloc(1000 + extra);
	if (buf == NULL)
		return NULL;

	p2p_buf_add_public_action_hdr(buf, P2P_GO_NEG_RESP, dialog_token);

	len = p2p_buf_add_ie_hdr(buf);
	p2p_buf_add_status(buf, status);
	group_capab = 0;
	if (peer && peer->go_state == LOCAL_GO) {
		if (peer->flags & P2P_DEV_PREFER_PERSISTENT_GROUP) {
			group_capab |= P2P_GROUP_CAPAB_PERSISTENT_GROUP;
			if (peer->flags & P2P_DEV_PREFER_PERSISTENT_RECONN)
				group_capab |=
					P2P_GROUP_CAPAB_PERSISTENT_RECONN;
		}
#ifdef	CONFIG_P2P_UNUSED_CMD
		if (p2p->cross_connect)
			group_capab |= P2P_GROUP_CAPAB_CROSS_CONN;
#endif	/* CONFIG_P2P_UNUSED_CMD */
		if (p2p->cfg->p2p_intra_bss)
			group_capab |= P2P_GROUP_CAPAB_INTRA_BSS_DIST;
	}
	p2p_buf_add_capability(buf, p2p->dev_capab &
			       ~P2P_DEV_CAPAB_CLIENT_DISCOVERABILITY,
			       group_capab);
	p2p_buf_add_go_intent(buf, (p2p->go_intent << 1) | tie_breaker);
	p2p_buf_add_config_timeout(buf, p2p->go_timeout, p2p->client_timeout);

	if (peer && peer->go_state == REMOTE_GO) {
		da16x_p2p_prt("[%s] Omit Operating Channel attribute\n",
				__func__);
	} else {
		p2p_buf_add_operating_channel(buf, p2p->cfg->country,
					      p2p->op_reg_class,
					      p2p->op_channel);
	}
	p2p_buf_add_intended_addr(buf, p2p->intended_addr);
	if (status || peer == NULL) {
		p2p_buf_add_channel_list(buf, p2p->cfg->country,
					 &p2p->channels);
	} else if (peer->go_state == REMOTE_GO) {
		p2p_buf_add_channel_list(buf, p2p->cfg->country,
					 &p2p->channels);
	} else {
		struct p2p_channels res;
		p2p_channels_intersect(&p2p->channels, &peer->channels,
				       &res);
		p2p_buf_add_channel_list(buf, p2p->cfg->country, &res);
	}
	p2p_buf_add_device_info(buf, p2p, peer);
	if (peer && peer->go_state == LOCAL_GO) {
		p2p_buf_add_group_id(buf, p2p->cfg->dev_addr, p2p->ssid,
				     p2p->ssid_len);
	}
	p2p_buf_update_ie_hdr(buf, len);

	/* WPS IE with Device Password ID attribute */
	pw_id = p2p_wps_method_pw_id(peer ? peer->wps_method : WPS_NOT_READY);
	if (peer && peer->oob_pw_id)
		pw_id = peer->oob_pw_id;

	if (p2p_build_wps_ie(p2p, buf, pw_id, 0) < 0) {
		da16x_p2p_prt("[%s] Failed to build WPS IE for GO "
			"Negotiation Response\n", __func__);

		wpabuf_free(buf);
		return NULL;
	}

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		wpabuf_put_buf(buf, p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */


	return buf;
}


/**
 * p2p_reselect_channel - Re-select operating channel based on peer information
 * @p2p: P2P module context from p2p_init()
 * @intersection: Support channel list intersection from local and peer
 *
 * This function is used to re-select the best channel after having received
 * information from the peer to allow supported channel lists to be intersected.
 * This can be used to improve initial channel selection done in
 * p2p_prepare_channel() prior to the start of GO Negotiation. In addition, this
 * can be used for Invitation case.
 */
void p2p_reselect_channel(struct p2p_data *p2p,
			  struct p2p_channels *intersection)
{
	struct p2p_reg_class *cl;
#ifdef	CONFIG_P2P_UNUSED_CMD
	int freq;
	u8 op_reg_class, op_channel;
	unsigned int i;
	const int op_classes_5ghz[] = { 124, 115, 0 };
	const int op_classes_ht40[] = { 126, 127, 116, 117, 0 };
	const int op_classes_vht[] = { 128, 0 };

	if (p2p->own_freq_preference > 0 &&
	    p2p_freq_to_channel(p2p->own_freq_preference,
				&op_reg_class, &op_channel) == 0 &&
	    p2p_channels_includes(intersection, op_reg_class, op_channel)) {
		da16x_p2p_prt("[%s] Pick own channel preference (reg_class %u "
			"channel %u) from intersection\n",
			__func__,
			op_reg_class, op_channel);

		p2p->op_reg_class = op_reg_class;
		p2p->op_channel = op_channel;
		return;
	}

	if (p2p->best_freq_overall > 0 &&
	    p2p_freq_to_channel(p2p->best_freq_overall,
				&op_reg_class, &op_channel) == 0 &&
	    p2p_channels_includes(intersection, op_reg_class, op_channel)) {
		da16x_p2p_prt("[%s] Pick best overall channel (reg_class %u "
			"channel %u) from intersection\n",
			__func__,
			op_reg_class, op_channel);

		p2p->op_reg_class = op_reg_class;
		p2p->op_channel = op_channel;

		return;
	}

	/* First, try to pick the best channel from another band */
	freq = p2p_channel_to_freq(p2p->op_reg_class, p2p->op_channel);
	if (freq >= 2400 && freq < 2500 && p2p->best_freq_5 > 0 &&
	    !p2p_channels_includes(intersection, p2p->op_reg_class,
				   p2p->op_channel) &&
	    p2p_freq_to_channel(p2p->best_freq_5,
				&op_reg_class, &op_channel) == 0 &&
	    p2p_channels_includes(intersection, op_reg_class, op_channel)) {
		da16x_p2p_prt("[%s] Pick best 5 GHz channel (reg_class %u "
			"channel %u) from intersection\n",
			__func__,
			op_reg_class, op_channel);

		p2p->op_reg_class = op_reg_class;
		p2p->op_channel = op_channel;

		return;
	}

	if (freq >= 4900 && freq < 6000 && p2p->best_freq_24 > 0 &&
	    !p2p_channels_includes(intersection, p2p->op_reg_class,
				   p2p->op_channel) &&
	    p2p_freq_to_channel(p2p->best_freq_24,
				&op_reg_class, &op_channel) == 0 &&
	    p2p_channels_includes(intersection, op_reg_class, op_channel)) {
		da16x_p2p_prt("[%s] Pick best 2.4 GHz channel (reg_class %u "
			"channel %u) from intersection\n",
			__func__,
			op_reg_class, op_channel);

		p2p->op_reg_class = op_reg_class;
		p2p->op_channel = op_channel;

		return;
	}

	/* Select channel with highest preference if the peer supports it */
	for (i = 0; p2p->cfg->pref_chan && i < p2p->cfg->num_pref_chan; i++) {
		if (p2p_channels_includes(intersection,
					  p2p->cfg->pref_chan[i].op_class,
					  p2p->cfg->pref_chan[i].chan)) {
			p2p->op_reg_class = p2p->cfg->pref_chan[i].op_class;
			p2p->op_channel = p2p->cfg->pref_chan[i].chan;

			da16x_p2p_prt("[%s] Pick highest preferred channel "
				"(op_class %u channel %u) from intersection\n",
				__func__,
				p2p->op_reg_class, p2p->op_channel);

			return;
		}
	}

	/* Try a channel where we might be able to use VHT */
	if (p2p_channel_select(intersection, op_classes_vht,
			       &p2p->op_reg_class, &p2p->op_channel) == 0) {
		da16x_p2p_prt("[%s] Pick possible VHT channel (op_class %u "
			"channel %u) from intersection\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);
		return;
	}

	/* Try a channel where we might be able to use HT40 */
	if (p2p_channel_select(intersection, op_classes_ht40,
			       &p2p->op_reg_class, &p2p->op_channel) == 0) {
		da16x_p2p_prt("[%s] Pick possible HT40 channel (op_class %u "
			"channel %u) from intersection\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);
		return;
	}

	/* Prefer a 5 GHz channel */
	if (p2p_channel_select(intersection, op_classes_5ghz,
			       &p2p->op_reg_class, &p2p->op_channel) == 0) {
		da16x_p2p_prt("[%s] Pick possible 5 GHz channel (op_class %u "
			"channel %u) from intersection\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);
		return;
	}
#endif	/* CONFIG_P2P_UNUSED_CMD */

	/*
	 * Try to see if the original channel is in the intersection. If
	 * so, no need to change anything, as it already contains some
	 * randomness.
	 */
	if (p2p_channels_includes(intersection, p2p->op_reg_class,
				  p2p->op_channel)) {
		da16x_p2p_prt("[%s] Using original operating class and channel "
			"(op_class %u channel %u) from intersection\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);
		return;
	}

	/*
	 * Fall back to whatever is included in the channel intersection since
	 * no better options seems to be available.
	 */
	cl = &intersection->reg_class[0];
	da16x_p2p_prt("[%s] Pick another channel (reg_class %u channel %u) "
		"from intersection\n",
		__func__,
		cl->reg_class, cl->channel[0]);

	p2p->op_reg_class = cl->reg_class;
	p2p->op_channel = cl->channel[0];
}

			     
#if 1	/* by Shingu 20160726 (P2P Config) */
static void p2p_set_ssid_passphrase(struct p2p_data *p2p)
{
	struct wpa_supplicant *wpa_s = p2p->cfg->cb_ctx;
	if (wpa_s->conf->p2p_ssid_len) {
		os_memcpy(p2p->ssid, wpa_s->conf->p2p_ssid, wpa_s->conf->p2p_ssid_len);
		p2p->ssid_len = wpa_s->conf->p2p_ssid_len;
	} else {
		p2p_build_ssid(p2p, p2p->ssid, &p2p->ssid_len);
	}
}
#endif	/* 1 */

static int p2p_go_select_channel(struct p2p_data *p2p, struct p2p_device *dev,
				 u8 *status)
{
	struct p2p_channels tmp, intersection;

	p2p_channels_dump(p2p, "own channels", &p2p->channels);
	p2p_channels_dump(p2p, "peer channels", &dev->channels);
	p2p_channels_intersect(&p2p->channels, &dev->channels, &tmp);
	p2p_channels_dump(p2p, "intersection", &tmp);
	p2p_channels_remove_freqs(&tmp, &p2p->no_go_freq);
	p2p_channels_dump(p2p, "intersection after no-GO removal", &tmp);
	p2p_channels_intersect(&tmp, &p2p->cfg->channels, &intersection);
	p2p_channels_dump(p2p, "intersection with local channel list",
			  &intersection);
	if (intersection.reg_classes == 0 ||
	    intersection.reg_class[0].channels == 0) {
		*status = P2P_SC_FAIL_NO_COMMON_CHANNELS;
		da16x_p2p_prt("[%s] No common channels found\n", __func__);
		return -1;
	}

	if (!p2p_channels_includes(&intersection, p2p->op_reg_class,
				   p2p->op_channel)) {
		if (dev->flags & P2P_DEV_FORCE_FREQ) {
			*status = P2P_SC_FAIL_NO_COMMON_CHANNELS;
			da16x_p2p_prt("[%s] Peer does not support the forced "
				"channel\n", __func__);
			return -1;
		}

		da16x_p2p_prt("[%s] Selected operating channel (op_class %u "
			"channel %u) not acceptable to the peer\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);

		p2p_reselect_channel(p2p, &intersection);
	} else if (!(dev->flags & P2P_DEV_FORCE_FREQ) &&
		   !p2p->cfg->cfg_op_channel) {
		da16x_p2p_prt("[%s] Try to optimize channel selection with "
			"peer information received; previously selected "
			"op_class %u channel %u\n",
			__func__,
			p2p->op_reg_class, p2p->op_channel);

		p2p_reselect_channel(p2p, &intersection);
	}

#if 1   /* by Shingu 20160726 (P2P Config) */
	p2p_set_ssid_passphrase(p2p);
#else
	if (!p2p->ssid_set) {
		p2p_build_ssid(p2p, p2p->ssid, &p2p->ssid_len);
		p2p->ssid_set = 1;
	}
#endif	/* 1 */

	return 0;
}


void p2p_process_go_neg_req(struct p2p_data *p2p, const u8 *sa,
			    const u8 *data, size_t len, int rx_freq)
{
	struct p2p_device *dev = NULL;
	struct wpabuf *resp;
	struct p2p_message msg;
	u8 status = P2P_SC_FAIL_INVALID_PARAMS;
	int tie_breaker = 0;
	int freq;
	extern int get_run_mode(void);

	da16x_p2p_prt("[%s] Received GO Negotiation Request from " MACSTR
		"(freq=%d)\n",
		__func__, MAC2STR(sa), rx_freq);

	p2p_stop_listen(p2p);

	if (p2p_parse(data, len, &msg))
		return;

	if (!msg.capability) {
		da16x_p2p_prt("[%s] Mandatory Capability attribute missing "
			"from GO Negotiation Request\n", __func__);
#ifdef CONFIG_P2P_STRICT
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	}

	if (msg.go_intent) {
		tie_breaker = *msg.go_intent & 0x01;
	} else {
		da16x_p2p_prt("[%s] Mandatory GO Intent attribute missing "
			"from GO Negotiation Request\n", __func__);
#ifdef CONFIG_P2P_STRICT
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	}

	if (!msg.config_timeout) {
		da16x_p2p_prt("[%s] Mandatory Configuration Timeout attribute "
			"missing from GO Negotiation Request\n",
			__func__);
#ifdef CONFIG_P2P_STRICT
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	}

	if (!msg.listen_channel) {
		da16x_p2p_prt("[%s] No Listen Channel attribute received\n",
				__func__);
		goto fail;
	}
	if (!msg.operating_channel) {
		da16x_p2p_prt("[%s] No Operating Channel attribute received\n",
				__func__);
		goto fail;
	}
	if (!msg.channel_list) {
		da16x_p2p_prt("[%s] No Channel List attribute received\n",
				__func__);
		goto fail;
	}
	if (!msg.intended_addr) {
		da16x_p2p_prt("[%s] No Intended P2P Interface Address attribute "
				"received\n", __func__);
		goto fail;
	}
	if (!msg.p2p_device_info) {
		da16x_p2p_prt("[%s] No P2P Device Info attribute received\n",
				__func__);
		goto fail;
	}

	if (os_memcmp(msg.p2p_device_addr, sa, ETH_ALEN) != 0) {
		da16x_p2p_prt("[%s] Unexpected GO Negotiation Request SA="
			MACSTR " != dev_addr=" MACSTR "\n",
			__func__,
			MAC2STR(sa), MAC2STR(msg.p2p_device_addr));
		goto fail;
	}

	dev = p2p_get_device(p2p, sa);

	if (msg.status && *msg.status) {
		da16x_p2p_prt("[%s] Unexpected Status attribute (%d) in GO "
			"Negotiation Request\n",
			__func__, *msg.status);

		if (dev && p2p->go_neg_peer == dev &&
		    *msg.status == P2P_SC_FAIL_REJECTED_BY_USER) {
			/*
			 * This mechanism for using Status attribute in GO
			 * Negotiation Request is not compliant with the P2P
			 * specification, but some deployed devices use it to
			 * indicate rejection of GO Negotiation in a case where
			 * they have sent out GO Negotiation Response with
			 * status 1. The P2P specification explicitly disallows
			 * this. To avoid unnecessary interoperability issues
			 * and extra frames, mark the pending negotiation as
			 * failed and do not reply to this GO Negotiation
			 * Request frame.
			 */
			p2p->cfg->send_action_done(p2p->cfg->cb_ctx);
			p2p_go_neg_failed(p2p, dev, *msg.status);
			p2p_parse_free(&msg);
			return;
		}
		goto fail;
	}

	if (get_run_mode() == P2P_GO_FIXED_MODE) {
		da16x_p2p_prt("[%s] P2P GO Fixed mode do not mgmt peers\n");
	} else if (dev == NULL) {
		dev = p2p_add_dev_from_go_neg_req(p2p, sa, &msg);
	} else if ((dev->flags & P2P_DEV_PROBE_REQ_ONLY) ||
		   !(dev->flags & P2P_DEV_REPORTED)) {
		p2p_add_dev_info(p2p, sa, dev, &msg);
	} else if (!dev->listen_freq && !dev->oper_freq) {
		/*
		 * This may happen if the peer entry was added based on PD
		 * Request and no Probe Request/Response frame has been received
		 * from this peer (or that information has timed out).
		 */
		da16x_p2p_prt("[%s] Update peer " MACSTR " based on GO Neg Req "
			"since listen/oper freq not known\n",
			__func__,
			MAC2STR(dev->info.p2p_device_addr));

		p2p_add_dev_info(p2p, sa, dev, &msg);
	}

	if (dev && dev->flags & P2P_DEV_USER_REJECTED) {
		da16x_p2p_prt("[%s] User has rejected this peer\n", __func__);
		status = P2P_SC_FAIL_REJECTED_BY_USER;
	} else if (dev == NULL ||
		   (dev->wps_method == WPS_NOT_READY &&
		    (p2p->authorized_oob_dev_pw_id == 0 ||
		     p2p->authorized_oob_dev_pw_id !=
		     msg.dev_password_id))) {

		da16x_p2p_prt("[%s] Not ready for GO negotiation with "
			MACSTR "\n",
			__func__, MAC2STR(sa));

		status = P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE;
		p2p->cfg->go_neg_req_rx(p2p->cfg->cb_ctx, sa,
					msg.dev_password_id);
	} else if (p2p->go_neg_peer && p2p->go_neg_peer != dev) {
		da16x_p2p_prt("[%s] Already in Group Formation with "
			"another peer\n", __func__);

		status = P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE;
	} else {
		int go;

		if (!p2p->go_neg_peer) {
			da16x_p2p_prt("[%s] Starting GO Negotiation with "
				"previously authorized peer\n",
				__func__);

			if (!(dev->flags & P2P_DEV_FORCE_FREQ)) {
				da16x_p2p_prt("[%s] Use default channel "
					"settings\n", __func__);

				p2p->op_reg_class = p2p->cfg->op_reg_class;
				p2p->op_channel = p2p->cfg->op_channel;
				os_memcpy(&p2p->channels, &p2p->cfg->channels,
					  sizeof(struct p2p_channels));
			} else {
				da16x_p2p_prt("[%s] Use previously configured "
					"forced channel settings\n", __func__);
			}
		}

		dev->flags &= ~P2P_DEV_NOT_YET_READY;

		if (!msg.go_intent) {
			da16x_p2p_prt("[%s] No GO Intent attribute received\n",
					__func__);
			goto fail;
		}
		if ((*msg.go_intent >> 1) > P2P_MAX_GO_INTENT) {
			da16x_p2p_prt("[%s] Invalid GO Intent value (%u) "
				"received\n",
				__func__, *msg.go_intent >> 1);

			goto fail;
		}

		if (dev->go_neg_req_sent &&
		    os_memcmp(sa, p2p->cfg->dev_addr, ETH_ALEN) > 0) {
			da16x_p2p_prt("[%s] Do not reply since peer has higher "
				"address and GO Neg Request already sent\n",
				__func__);

			p2p_parse_free(&msg);
			return;
		}

		go = p2p_go_det(p2p->go_intent, *msg.go_intent);
		if (go < 0) {
			da16x_p2p_prt("[%s] Incompatible GO Intent\n", __func__);

			status = P2P_SC_FAIL_BOTH_GO_INTENT_15;
			goto fail;
		}

		if (p2p_peer_channels(p2p, dev, msg.channel_list,
				      msg.channel_list_len) < 0) {
			da16x_p2p_prt("[%s] No common channels found\n",
					__func__);

			status = P2P_SC_FAIL_NO_COMMON_CHANNELS;
			goto fail;
		}

		switch (msg.dev_password_id) {
		case DEV_PW_REGISTRAR_SPECIFIED:
			da16x_p2p_prt("[%s] PIN from peer Display\n", __func__);

			if (dev->wps_method != WPS_PIN_KEYPAD) {
				da16x_p2p_prt("[%s] We have wps_method=%s -> "
					"incompatible\n",
					__func__,
					p2p_wps_method_str(dev->wps_method));

				status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
				goto fail;
			}
			break;
		case DEV_PW_USER_SPECIFIED:
			da16x_p2p_prt("[%s] Peer entered PIN on Keypad\n",
					__func__);

			if (dev->wps_method != WPS_PIN_DISPLAY) {
				da16x_p2p_prt("[%s] We have wps_method=%s -> "
					"incompatible\n",
					__func__,
					p2p_wps_method_str(dev->wps_method));

				status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
				goto fail;
			}
			break;
		case DEV_PW_PUSHBUTTON:
			da16x_p2p_prt("[%s] Peer using pushbutton\n", __func__);

			if (dev->wps_method != WPS_PBC) {
				da16x_p2p_prt("[%s] We have wps_method=%s -> "
					"incompatible\n",
					__func__,
					p2p_wps_method_str(dev->wps_method));

				status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
				goto fail;
			}
			break;
		default:
#if 0
			if (msg.dev_password_id &&
			    msg.dev_password_id == dev->oob_pw_id) {
				da16x_p2p_prt("[%s] Peer using NFC\n", __func__);
				if (dev->wps_method != WPS_NFC) {
					da16x_p2p_prt("[%s] We have wps_method="
						"%s -> incompatible\n",
						__func__,
						p2p_wps_method_str(
							dev->wps_method));

					status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
					goto fail;
				}
				break;
			}
#endif	/* 0 */
			da16x_p2p_prt("[%s] Unsupported Device Password ID %d\n",
				__func__,
				msg.dev_password_id);

			status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
			goto fail;
		}

		if (go && p2p_go_select_channel(p2p, dev, &status) < 0)
			goto fail;

		dev->go_state = go ? LOCAL_GO : REMOTE_GO;
		dev->oper_freq = p2p_channel_to_freq(msg.operating_channel[3],
						     msg.operating_channel[4]);
		da16x_p2p_prt("[%s] Peer operating channel preference: %d MHz\n",
			__func__, dev->oper_freq);

		if (msg.config_timeout) {
			dev->go_timeout = msg.config_timeout[0];
			dev->client_timeout = msg.config_timeout[1];
		}

		da16x_p2p_prt("[%s] GO Negotiation with " MACSTR "\n",
			__func__, MAC2STR(sa));

		if (p2p->state != P2P_IDLE)
			p2p_stop_find_for_freq(p2p, rx_freq);

		p2p_set_state(p2p, P2P_GO_NEG);
		p2p_clear_timeout(p2p);
		dev->dialog_token = msg.dialog_token;
		os_memcpy(dev->intended_addr, msg.intended_addr, ETH_ALEN);
		p2p->go_neg_peer = dev;
		status = P2P_SC_SUCCESS;
	}

fail:
	if (dev)
		dev->status = status;
	resp = p2p_build_go_neg_resp(p2p, dev, msg.dialog_token, status,
				     !tie_breaker);
	p2p_parse_free(&msg);
	if (resp == NULL)
		return;

	da16x_p2p_prt("[%s] Sending GO Negotiation Response\n", __func__);
	if (rx_freq > 0)
		freq = rx_freq;
	else
		freq = p2p_channel_to_freq(p2p->cfg->reg_class,
					   p2p->cfg->channel);
	if (freq < 0) {
		da16x_p2p_prt("[%s] Unknown regulatory class/channel\n",
				__func__);

		wpabuf_free(resp);
		return;
	}
	if (status == P2P_SC_SUCCESS) {
		p2p->pending_action_state = P2P_PENDING_GO_NEG_RESPONSE;
		dev->flags |= P2P_DEV_WAIT_GO_NEG_CONFIRM;
		if (os_memcmp(sa, p2p->cfg->dev_addr, ETH_ALEN) < 0) {
			/*
			 * Peer has smaller address, so the GO Negotiation
			 * Response from us is expected to complete
			 * negotiation. Ignore a GO Negotiation Response from
			 * the peer if it happens to be received after this
			 * point due to a race condition in GO Negotiation
			 * Request transmission and processing.
			 */
			dev->flags &= ~P2P_DEV_WAIT_GO_NEG_RESPONSE;
		}
	} else {
		extern char p2p_accept_mac[18];
		sprintf(p2p_accept_mac, MACSTR, MAC2STR(sa));
		p2p->pending_action_state =
			P2P_PENDING_GO_NEG_RESPONSE_FAILURE;
#if 1	/* by Shingu 20161025 (PRINT) */
		da16x_notice_prt(">>> P2P Connection Request from " MACSTR "\n",
				MAC2STR(sa));
#endif	/* 1 */
	}

	if (p2p_send_action(p2p, freq, sa, p2p->cfg->dev_addr,
			    p2p->cfg->dev_addr,
			    wpabuf_head(resp), wpabuf_len(resp), 500) < 0) {
		da16x_p2p_prt("[%s] Failed to send Action frame\n", __func__);
	}

	wpabuf_free(resp);
}


static struct wpabuf * p2p_build_go_neg_conf(struct p2p_data *p2p,
					     struct p2p_device *peer,
					     u8 dialog_token, u8 status,
					     const u8 *resp_chan, int go)
{
	struct wpabuf *buf;
	u8 *len;
	struct p2p_channels res;
	u8 group_capab;
	size_t extra = 0;

	da16x_p2p_prt("[%s] Building GO Negotiation Confirm\n", __func__);

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		extra = wpabuf_len(p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */

	buf = wpabuf_alloc(1000 + extra);
	if (buf == NULL)
		return NULL;

	p2p_buf_add_public_action_hdr(buf, P2P_GO_NEG_CONF, dialog_token);

	len = p2p_buf_add_ie_hdr(buf);
	p2p_buf_add_status(buf, status);
	group_capab = 0;
	if (peer->go_state == LOCAL_GO) {
		if (peer->flags & P2P_DEV_PREFER_PERSISTENT_GROUP) {
			group_capab |= P2P_GROUP_CAPAB_PERSISTENT_GROUP;
			if (peer->flags & P2P_DEV_PREFER_PERSISTENT_RECONN)
				group_capab |=
					P2P_GROUP_CAPAB_PERSISTENT_RECONN;
		}
#ifdef	CONFIG_P2P_UNUSED_CMD
		if (p2p->cross_connect)
			group_capab |= P2P_GROUP_CAPAB_CROSS_CONN;
#endif	/* CONFIG_P2P_UNUSED_CMD */
		if (p2p->cfg->p2p_intra_bss)
			group_capab |= P2P_GROUP_CAPAB_INTRA_BSS_DIST;
	}
	p2p_buf_add_capability(buf, p2p->dev_capab &
			       ~P2P_DEV_CAPAB_CLIENT_DISCOVERABILITY,
			       group_capab);
	if (go || resp_chan == NULL)
		p2p_buf_add_operating_channel(buf, p2p->cfg->country,
					      p2p->op_reg_class,
					      p2p->op_channel);
	else
		p2p_buf_add_operating_channel(buf, (const char *) resp_chan,
					      resp_chan[3], resp_chan[4]);
	p2p_channels_intersect(&p2p->channels, &peer->channels, &res);
	p2p_buf_add_channel_list(buf, p2p->cfg->country, &res);
	if (go) {
		p2p_buf_add_group_id(buf, p2p->cfg->dev_addr, p2p->ssid,
				     p2p->ssid_len);
	}
	p2p_buf_update_ie_hdr(buf, len);

#ifdef CONFIG_WIFI_DISPLAY
	if (p2p->wfd_ie_go_neg)
		wpabuf_put_buf(buf, p2p->wfd_ie_go_neg);
#endif /* CONFIG_WIFI_DISPLAY */

	return buf;
}


void p2p_process_go_neg_resp(struct p2p_data *p2p, const u8 *sa,
			     const u8 *data, size_t len, int rx_freq)
{
	struct p2p_device *dev;
	int go = -1;
	struct p2p_message msg;
	u8 status = P2P_SC_SUCCESS;
	int freq;

	da16x_p2p_prt("[%s] Received GO Negotiation Response from " MACSTR
		" (freq=%d)\n", __func__, MAC2STR(sa), rx_freq);

	p2p_stop_listen(p2p);

	dev = p2p_get_device(p2p, sa);
	if (dev == NULL || dev->wps_method == WPS_NOT_READY ||
	    dev != p2p->go_neg_peer) {
		da16x_p2p_prt("[%s] Not ready for GO negotiation with "
			MACSTR "\n",
			__func__, MAC2STR(sa));
		return;
	}

	if (p2p_parse(data, len, &msg))
		return;

	if (!(dev->flags & P2P_DEV_WAIT_GO_NEG_RESPONSE)) {
		da16x_p2p_prt("[%s] Was not expecting GO Negotiation Response - "
				"ignore\n", __func__);

		p2p_parse_free(&msg);
		return;
	}
	dev->flags &= ~P2P_DEV_WAIT_GO_NEG_RESPONSE;

	if (msg.dialog_token != dev->dialog_token) {
		da16x_p2p_prt("[5s] Unexpected Dialog Token %u (expected %u)\n",
			__func__, msg.dialog_token, dev->dialog_token);

		p2p_parse_free(&msg);
		return;
	}

	if (!msg.status) {
		da16x_p2p_prt("[%s] No Status attribute received\n", __func__);

		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	if (*msg.status) {
		da16x_p2p_prt("[%s] GO Negotiation rejected: status %d\n",
				__func__, *msg.status);

		dev->go_neg_req_sent = 0;
		if (*msg.status == P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE) {
			da16x_p2p_prt("[%s] Wait for the peer to become ready "
				"for GO Negotiation\n", __func__);

			dev->flags |= P2P_DEV_NOT_YET_READY;
			os_get_reltime(&dev->go_neg_wait_started);

			p2p_set_state(p2p, P2P_WAIT_PEER_IDLE);
			p2p_set_timeout(p2p, 0, 0);
		} else {
			da16x_p2p_prt("[%s] Stop GO Negotiation attempt\n",
					__func__);

			p2p_go_neg_failed(p2p, dev, *msg.status);
		}
		p2p->cfg->send_action_done(p2p->cfg->cb_ctx);
		p2p_parse_free(&msg);
		return;
	}

	if (!msg.capability) {
		da16x_p2p_prt("[%s] Mandatory Capability attribute missing "
			"from GO Negotiation Response\n", __func__);

#ifdef CONFIG_P2P_STRICT
		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	}

	if (!msg.p2p_device_info) {
		da16x_p2p_prt("[%s] Mandatory P2P Device Info attribute "
			"missing from GO Negotiation Response\n", __func__);

#ifdef CONFIG_P2P_STRICT
		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	}

	if (!msg.intended_addr) {
		da16x_p2p_prt("[%s] No Intended P2P Interface Address "
			"attribute received\n", __func__);

		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	if (!msg.go_intent) {
		da16x_p2p_prt("[%s] No GO Intent attribute received\n",
				__func__);

		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	if ((*msg.go_intent >> 1) > P2P_MAX_GO_INTENT) {
		da16x_p2p_prt("[%s] Invalid GO Intent value (%u) received\n",
			__func__, *msg.go_intent >> 1);

		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	go = p2p_go_det(p2p->go_intent, *msg.go_intent);
	if (go < 0) {
		da16x_p2p_prt("[%s] Incompatible GO Intent\n", __func__);

		status = P2P_SC_FAIL_INCOMPATIBLE_PARAMS;
		goto fail;
	}

	if (!go && msg.group_id) {
		/* Store SSID for Provisioning step */
		p2p->ssid_len = msg.group_id_len - ETH_ALEN;
		os_memcpy(p2p->ssid, msg.group_id + ETH_ALEN, p2p->ssid_len);
	} else if (!go) {
		da16x_p2p_prt("[%s] Mandatory P2P Group ID attribute missing "
			"from GO Negotiation Response\n", __func__);
		p2p->ssid_len = 0;
		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	if (!msg.config_timeout) {
		da16x_p2p_prt("[%s] Mandatory Configuration Timeout attribute "
			"missing from GO Negotiation Response\n", __func__);
#ifdef CONFIG_P2P_STRICT
		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
#endif /* CONFIG_P2P_STRICT */
	} else {
		dev->go_timeout = msg.config_timeout[0];
		dev->client_timeout = msg.config_timeout[1];
	}

	if (!msg.operating_channel && !go) {
		/*
		 * Note: P2P Client may omit Operating Channel attribute to
		 * indicate it does not have a preference.
		 */
		da16x_p2p_prt("[%s] No Operating Channel attribute received\n",
				__func__);

		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}
	if (!msg.channel_list) {
		da16x_p2p_prt("[%s] No Channel List attribute received\n",
				__func__);
		status = P2P_SC_FAIL_INVALID_PARAMS;
		goto fail;
	}

	if (p2p_peer_channels(p2p, dev, msg.channel_list,
			      msg.channel_list_len) < 0) {
		da16x_p2p_prt("[%s] No common channels found\n", __func__);
		status = P2P_SC_FAIL_NO_COMMON_CHANNELS;
		goto fail;
	}

	if (msg.operating_channel) {
		dev->oper_freq = p2p_channel_to_freq(msg.operating_channel[3],
						     msg.operating_channel[4]);
		da16x_p2p_prt("[%s] Peer operating channel preference: %d MHz\n",
			__func__, dev->oper_freq);
	} else {
		dev->oper_freq = 0;
	}

	switch (msg.dev_password_id) {
	case DEV_PW_REGISTRAR_SPECIFIED:
		da16x_p2p_prt("[%s] PIN from peer Display\n", __func__);

		if (dev->wps_method != WPS_PIN_KEYPAD) {
			da16x_p2p_prt("[%s] We have wps_method=%s -> "
				"incompatible\n",
				__func__,
				p2p_wps_method_str(dev->wps_method));

			status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
			goto fail;
		}
		break;
	case DEV_PW_USER_SPECIFIED:
		da16x_p2p_prt("[%s] Peer entered PIN on Keypad\n", __func__);

		if (dev->wps_method != WPS_PIN_DISPLAY) {
			da16x_p2p_prt("[%s] We have wps_method=%s -> "
				"incompatible\n",
				__func__,
				p2p_wps_method_str(dev->wps_method));

			status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
			goto fail;
		}
		break;
	case DEV_PW_PUSHBUTTON:
		da16x_p2p_prt("[%s] Peer using pushbutton\n", __func__);

		if (dev->wps_method != WPS_PBC) {
			da16x_p2p_prt("[%s] We have wps_method=%s -> "
				"incompatible\n",
				__func__,
				p2p_wps_method_str(dev->wps_method));

			status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
			goto fail;
		}
		break;
	default:
#if 0
		if (msg.dev_password_id &&
		    msg.dev_password_id == dev->oob_pw_id) {
			da16x_p2p_prt("[%s] Peer using NFC\n", __func__);
			if (dev->wps_method != WPS_NFC) {
				da16x_p2p_prt("[%s] We have wps_method=%s -> "
					"incompatible\n",
					__func__,
					p2p_wps_method_str(dev->wps_method));

				status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
				goto fail;
			}
			break;
		}
#endif	/* 0 */
		da16x_p2p_prt("[%s] Unsupported Device Password ID %d\n",
			__func__, msg.dev_password_id);

		status = P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD;
		goto fail;
	}

	if (go && p2p_go_select_channel(p2p, dev, &status) < 0)
		goto fail;

	p2p_set_state(p2p, P2P_GO_NEG);
	p2p_clear_timeout(p2p);

	da16x_p2p_prt("[%s] GO Negotiation with " MACSTR "\n",
			__func__, MAC2STR(sa));

	os_memcpy(dev->intended_addr, msg.intended_addr, ETH_ALEN);

fail:
	/* Store GO Negotiation Confirmation to allow retransmission */
	wpabuf_free(dev->go_neg_conf);
	dev->go_neg_conf = p2p_build_go_neg_conf(p2p, dev, msg.dialog_token,
						 status, msg.operating_channel,
						 go);
	p2p_parse_free(&msg);
	if (dev->go_neg_conf == NULL)
		return;
	da16x_p2p_prt("[%s] Sending GO Negotiation Confirm\n", __func__);
	if (status == P2P_SC_SUCCESS) {
		p2p->pending_action_state = P2P_PENDING_GO_NEG_CONFIRM;
		dev->go_state = go ? LOCAL_GO : REMOTE_GO;
	} else
		p2p->pending_action_state = P2P_NO_PENDING_ACTION;
	if (rx_freq > 0)
		freq = rx_freq;
	else
		freq = dev->listen_freq;

	dev->go_neg_conf_freq = freq;
	dev->go_neg_conf_sent = 0;

	if (p2p_send_action(p2p, freq, sa, p2p->cfg->dev_addr, sa,
			    wpabuf_head(dev->go_neg_conf),
			    wpabuf_len(dev->go_neg_conf), 200) < 0) {
		da16x_p2p_prt("[%s] Failed to send Action frame\n", __func__);

		p2p_go_neg_failed(p2p, dev, -1);
		p2p->cfg->send_action_done(p2p->cfg->cb_ctx);
	} else {
		dev->go_neg_conf_sent++;
	}

	if (status != P2P_SC_SUCCESS) {
		da16x_p2p_prt("[%s] GO Negotiation failed\n", __func__);

		p2p_go_neg_failed(p2p, dev, status);
	}
}


void p2p_process_go_neg_conf(struct p2p_data *p2p, const u8 *sa,
			     const u8 *data, size_t len)
{
	struct p2p_device *dev;
	struct p2p_message msg;

	da16x_p2p_prt("[%s] Received GO Negotiation Confirm from " MACSTR "\n",
		__func__, MAC2STR(sa));

	dev = p2p_get_device(p2p, sa);
	if (dev == NULL || dev->wps_method == WPS_NOT_READY ||
	    dev != p2p->go_neg_peer) {
		da16x_p2p_prt("[%s] Not ready for GO negotiation with "
			MACSTR "\n", __func__, MAC2STR(sa));
		return;
	}

	if (p2p->pending_action_state == P2P_PENDING_GO_NEG_RESPONSE) {
		da16x_p2p_prt("[%s] Stopped waiting for TX status on GO "
			"Negotiation Response since we already received "
			"Confirmation\n", __func__);

		p2p->pending_action_state = P2P_NO_PENDING_ACTION;
	}

	if (p2p_parse(data, len, &msg))
		return;

	if (!(dev->flags & P2P_DEV_WAIT_GO_NEG_CONFIRM)) {
		da16x_p2p_prt("[%s] Was not expecting GO Negotiation Confirm - "
			"ignore\n", __func__);

		p2p_parse_free(&msg);
		return;
	}
	dev->flags &= ~P2P_DEV_WAIT_GO_NEG_CONFIRM;
	p2p->cfg->send_action_done(p2p->cfg->cb_ctx);

	if (msg.dialog_token != dev->dialog_token) {
		da16x_p2p_prt("[%s] Unexpected Dialog Token %u (expected %u)\n",
			__func__, msg.dialog_token, dev->dialog_token);

		p2p_parse_free(&msg);
		return;
	}

	if (!msg.status) {
		da16x_p2p_prt("[%s] No Status attribute received\n", __func__);
		p2p_parse_free(&msg);
		return;
	}
	if (*msg.status) {
		da16x_p2p_prt("[%s] GO Negotiation rejected: status %d\n",
			__func__, *msg.status);
		p2p_go_neg_failed(p2p, dev, *msg.status);
		p2p_parse_free(&msg);
		return;
	}

	if (dev->go_state == REMOTE_GO && msg.group_id) {
		/* Store SSID for Provisioning step */
		p2p->ssid_len = msg.group_id_len - ETH_ALEN;
		os_memcpy(p2p->ssid, msg.group_id + ETH_ALEN, p2p->ssid_len);
	} else if (dev->go_state == REMOTE_GO) {
		da16x_p2p_prt("[%s] Mandatory P2P Group ID attribute missing "
			"from GO Negotiation Confirmation\n", __func__);
		p2p->ssid_len = 0;
		p2p_go_neg_failed(p2p, dev, P2P_SC_FAIL_INVALID_PARAMS);
		p2p_parse_free(&msg);
		return;
	}

	if (!msg.operating_channel) {
		da16x_p2p_prt("[%s] Mandatory Operating Channel attribute "
			"missing from GO Negotiation Confirmation\n",
			__func__);
#ifdef CONFIG_P2P_STRICT
		p2p_parse_free(&msg);
		return;
#endif /* CONFIG_P2P_STRICT */
	} else if (dev->go_state == REMOTE_GO) {
		int oper_freq = p2p_channel_to_freq(msg.operating_channel[3],
						    msg.operating_channel[4]);
		if (oper_freq != dev->oper_freq) {
			da16x_p2p_prt("[%s] Updated peer (GO) operating "
				"channel preference from %d MHz to %d MHz\n",
				__func__, dev->oper_freq, oper_freq);

			dev->oper_freq = oper_freq;
		}
	}

	if (!msg.channel_list) {
		da16x_p2p_prt("[%s] Mandatory Operating Channel attribute "
			"missing from GO Negotiation Confirmation\n", __func__);

#ifdef CONFIG_P2P_STRICT
		p2p_parse_free(&msg);
		return;
#endif /* CONFIG_P2P_STRICT */
	}

	p2p_parse_free(&msg);

	if (dev->go_state == UNKNOWN_GO) {
		/*
		 * This should not happen since GO negotiation has already
		 * been completed.
		 */
		da16x_p2p_prt("[%s] Unexpected GO Neg state - do not know "
			"which end becomes GO\n", __func__);
		return;
	}

	/*
	 * The peer could have missed our ctrl::ack frame for GO Negotiation
	 * Confirm and continue retransmitting the frame. To reduce the
	 * likelihood of the peer not getting successful TX status for the
	 * GO Negotiation Confirm frame, wait a short time here before starting
	 * the group so that we will remain on the current channel to
	 * acknowledge any possible retransmission from the peer.
	 */
	da16x_p2p_prt("[%s] 20 ms wait on current channel before "
			"starting group\n", __func__);
	os_sleep(0, 20000);

	p2p_go_complete(p2p, dev);
}

#endif	/* CONFIG_P2P */

/* EOF */
