/*
 * EAP-WSC server for Wi-Fi Protected Setup
 * Copyright (c) 2007-2008, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	EAP_SERVER_WSC

#include "supp_common.h"
#include "supp_eloop.h"
#include "eap_i.h"
#include "eap_common/eap_wsc_common.h"
#include "p2p/supp_p2p.h"
#include "wps/wps.h"


struct eap_wsc_data {
	enum _eap_wsc_data { START, MESG, FRAG_ACK, WAIT_FRAG_ACK, DONE, FAIL } state;
	int registrar;
	struct wpabuf *in_buf;
	struct wpabuf *out_buf;
	enum wsc_op_code in_op_code, out_op_code;
	size_t out_used;
	size_t fragment_size;
	struct wps_data *wps;
	int ext_reg_timeout;
};


#ifndef CONFIG_NO_STDOUT_DEBUG
#ifdef	ENABLE_EAP_DBG
static const char * eap_wsc_state_txt(int state)
{
	switch (state) {
	case START:
		return "START";
	case MESG:
		return "MESG";
	case FRAG_ACK:
		return "FRAG_ACK";
	case WAIT_FRAG_ACK:
		return "WAIT_FRAG_ACK";
	case DONE:
		return "DONE";
	case FAIL:
		return "FAIL";
	default:
		return "?";
	}
}
#endif /* ENABLE_EAP_DBG */
#endif /* CONFIG_NO_STDOUT_DEBUG */


static void eap_wsc_state(struct eap_wsc_data *data, int state)
{
	da16x_eap_prt("[%s] EAP-WSC: %s -> %s\n", __func__,
		   eap_wsc_state_txt(data->state),
		   eap_wsc_state_txt(state));
	data->state = (enum _eap_wsc_data ) state;
}


static void eap_wsc_ext_reg_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct eap_sm *sm = eloop_ctx;
	struct eap_wsc_data *data = timeout_ctx;

	if (sm->method_pending != METHOD_PENDING_WAIT)
		return;

	da16x_eap_prt("     [%s] EAP-WSC: Timeout while waiting for an External "
		   "Registrar\n", __func__);
	data->ext_reg_timeout = 1;
	eap_sm_pending_cb(sm);
}


static void * eap_wsc_init(struct eap_sm *sm)
{
	struct eap_wsc_data *data;
	int registrar;
	struct wps_config cfg;

	if (sm->identity && sm->identity_len == WSC_ID_REGISTRAR_LEN &&
	    os_memcmp(sm->identity, WSC_ID_REGISTRAR, WSC_ID_REGISTRAR_LEN) ==
	    0)
		registrar = 0; /* Supplicant is Registrar */
	else if (sm->identity && sm->identity_len == WSC_ID_ENROLLEE_LEN &&
		 os_memcmp(sm->identity, WSC_ID_ENROLLEE, WSC_ID_ENROLLEE_LEN)
		 == 0)
		registrar = 1; /* Supplicant is Enrollee */
	else {
		da16x_dump("EAP-WSC: Unexpected identity",
				  sm->identity, sm->identity_len);
		return NULL;
	}

	data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;
	data->state = registrar ? START : MESG;
	data->registrar = registrar;

	os_memset(&cfg, 0, sizeof(cfg));
	cfg.wps = sm->wps;
	cfg.registrar = registrar;
	if (registrar) {
		if (sm->wps == NULL || sm->wps->registrar == NULL) {
			da16x_eap_prt("     [%s] EAP-WSC: WPS Registrar not "
				   "initialized\n", __func__);
			os_free(data);
			return NULL;
		}
	} else {
		if (sm->user == NULL || sm->user->password == NULL) {
			/*
			 * In theory, this should not really be needed, but
			 * Windows 7 uses Registrar mode to probe AP's WPS
			 * capabilities before trying to use Enrollee and fails
			 * if the AP does not allow that probing to happen..
			 */
			da16x_eap_prt("     [%s] EAP-WSC: No AP PIN (password) "
				   "configured for Enrollee functionality - "
				   "allow for probing capabilities (M1)\n", __func__);
		} else {
			cfg.pin = sm->user->password;
			cfg.pin_len = sm->user->password_len;
		}
	}
	cfg.assoc_wps_ie = sm->assoc_wps_ie;
	cfg.peer_addr = sm->peer_addr;
#ifdef CONFIG_P2P
	if (sm->assoc_p2p_ie) {
		da16x_eap_prt("     [%s] EAP-WSC: Prefer PSK format for P2P "
			   "client\n", __func__);
		cfg.use_psk_key = 1;
		cfg.p2p_dev_addr = p2p_get_go_dev_addr(sm->assoc_p2p_ie);
	}
#endif /* CONFIG_P2P */
	cfg.pbc_in_m1 = sm->pbc_in_m1;
	data->wps = wps_init(&cfg);
	if (data->wps == NULL) {
		os_free(data);
		return NULL;
	}
	data->fragment_size = sm->fragment_size > 0 ? sm->fragment_size :
		WSC_FRAGMENT_SIZE;

	return data;
}


static void eap_wsc_reset(struct eap_sm *sm, void *priv)
{
	struct eap_wsc_data *data = priv;
	da16x_eloop_cancel_timeout(eap_wsc_ext_reg_timeout, sm, data);
	wpabuf_free(data->in_buf);
	wpabuf_free(data->out_buf);
	wps_deinit(data->wps);
	os_free(data);
}


static struct wpabuf * eap_wsc_build_start(struct eap_sm *sm,
					   struct eap_wsc_data *data, u8 id)
{
	struct wpabuf *req;

	req = eap_msg_alloc(EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC, 2,
			    EAP_CODE_REQUEST, id);
	if (req == NULL) {
		da16x_eap_prt("     [%s] EAP-WSC: Failed to allocate memory for "
			   "request\n", __func__);
		return NULL;
	}

	da16x_eap_prt("     [%s] EAP-WSC: Send WSC/Start\n", __func__);
	wpabuf_put_u8(req, WSC_Start); /* Op-Code */
	wpabuf_put_u8(req, 0); /* Flags */

	return req;
}


static struct wpabuf * eap_wsc_build_msg(struct eap_wsc_data *data, u8 id)
{
	struct wpabuf *req;
	u8 flags;
	size_t send_len, plen;

	flags = 0;
	send_len = wpabuf_len(data->out_buf) - data->out_used;
	if (2 + send_len > data->fragment_size) {
		send_len = data->fragment_size - 2;
		flags |= WSC_FLAGS_MF;
		if (data->out_used == 0) {
			flags |= WSC_FLAGS_LF;
			send_len -= 2;
		}
	}
	plen = 2 + send_len;
	if (flags & WSC_FLAGS_LF)
		plen += 2;
	req = eap_msg_alloc(EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC, plen,
			    EAP_CODE_REQUEST, id);
	if (req == NULL) {
		da16x_eap_prt("     [%s] EAP-WSC: Failed to allocate memory for "
			   "request\n", __func__);
		return NULL;
	}

	wpabuf_put_u8(req, data->out_op_code); /* Op-Code */
	wpabuf_put_u8(req, flags); /* Flags */
	if (flags & WSC_FLAGS_LF)
		wpabuf_put_be16(req, wpabuf_len(data->out_buf));

	wpabuf_put_data(req, wpabuf_head_u8(data->out_buf) + data->out_used,
			send_len);
	data->out_used += send_len;

	if (data->out_used == wpabuf_len(data->out_buf)) {
		da16x_eap_prt("     [%s] EAP-WSC: Sending out %lu bytes "
			   "(message sent completely)\n", __func__,
			   (unsigned long) send_len);
		wpabuf_free(data->out_buf);
		data->out_buf = NULL;
		data->out_used = 0;
		eap_wsc_state(data, MESG);
	} else {
		da16x_eap_prt("     [%s] EAP-WSC: Sending out %lu bytes "
			   "(%lu more to send)\n", __func__, (unsigned long) send_len,
			   (unsigned long) wpabuf_len(data->out_buf) -
			   data->out_used);
		eap_wsc_state(data, WAIT_FRAG_ACK);
	}

	return req;
}


static struct wpabuf * eap_wsc_buildReq(struct eap_sm *sm, void *priv, u8 id)
{
	struct eap_wsc_data *data = priv;

	switch (data->state) {
	case START:
		return eap_wsc_build_start(sm, data, id);
	case MESG:
		if (data->out_buf == NULL) {
			data->out_buf = wps_get_msg(data->wps, &data->out_op_code);
			if (data->out_buf == NULL) {
				da16x_eap_prt("     [%s] EAP-WSC: Failed to "
					   "receive message from WPS\n", __func__);
				return NULL;
			}
			data->out_used = 0;
		}
		/* fall through */
	case WAIT_FRAG_ACK:
		return eap_wsc_build_msg(data, id);
	case FRAG_ACK:
		return eap_wsc_build_frag_ack(id, EAP_CODE_REQUEST);
	default:
		da16x_eap_prt("     [%s] EAP-WSC: Unexpected state %d in "
			   "buildReq\n", __func__, data->state);
		return NULL;
	}
}


static Boolean eap_wsc_check(struct eap_sm *sm, void *priv,
			     struct wpabuf *respData)
{
	const u8 *pos;
	size_t len;

	pos = eap_hdr_validate(EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC,
			       respData, &len);
	if (pos == NULL || len < 2) {
		da16x_eap_prt("     [%s] EAP-WSC: Invalid frame\n", __func__);
		return TRUE;
	}

	return FALSE;
}


static int eap_wsc_process_cont(struct eap_wsc_data *data,
				const u8 *buf, size_t len, u8 op_code)
{
	/* Process continuation of a pending message */
	if (op_code != data->in_op_code) {
		da16x_eap_prt("     [%s] EAP-WSC: Unexpected Op-Code %d in "
			   "fragment (expected %d)\n", __func__,
			   op_code, data->in_op_code);
		eap_wsc_state(data, FAIL);
		return -1;
	}

	if (len > wpabuf_tailroom(data->in_buf)) {
		da16x_eap_prt("     [%s] EAP-WSC: Fragment overflow\n", __func__);
		eap_wsc_state(data, FAIL);
		return -1;
	}

	wpabuf_put_data(data->in_buf, buf, len);
	da16x_eap_prt("     [%s] EAP-WSC: Received %lu bytes, waiting for %lu "
		   "bytes more\n", __func__, (unsigned long) len,
		   (unsigned long) wpabuf_tailroom(data->in_buf));

	return 0;
}


static int eap_wsc_process_fragment(struct eap_wsc_data *data,
				    u8 flags, u8 op_code, u16 message_length,
				    const u8 *buf, size_t len)
{
	/* Process a fragment that is not the last one of the message */
	if (data->in_buf == NULL && !(flags & WSC_FLAGS_LF)) {
		da16x_eap_prt("     [%s] EAP-WSC: No Message Length "
			   "field in a fragmented packet\n", __func__);
		return -1;
	}

	if (data->in_buf == NULL) {
		/* First fragment of the message */
		data->in_buf = wpabuf_alloc(message_length);
		if (data->in_buf == NULL) {
			da16x_eap_prt("     [%s] EAP-WSC: No memory for "
				   "message\n", __func__);
			return -1;
		}
		data->in_op_code = (enum wsc_op_code)op_code;
		wpabuf_put_data(data->in_buf, buf, len);
		da16x_eap_prt("     [%s] EAP-WSC: Received %lu bytes in "
			   "first fragment, waiting for %lu bytes more\n", __func__,
			   (unsigned long) len,
			   (unsigned long) wpabuf_tailroom(data->in_buf));
	}

	return 0;
}


static void eap_wsc_process(struct eap_sm *sm, void *priv,
			    struct wpabuf *respData)
{
	struct eap_wsc_data *data = priv;
	const u8 *start, *pos, *end;
	size_t len;
	u8 op_code, flags;
	u16 message_length = 0;
	enum wps_process_res res;
	struct wpabuf tmpbuf;

	da16x_eloop_cancel_timeout(eap_wsc_ext_reg_timeout, sm, data);
	if (data->ext_reg_timeout) {
		eap_wsc_state(data, FAIL);
		return;
	}

	pos = eap_hdr_validate(EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC,
			       respData, &len);
	if (pos == NULL || len < 2)
		return; /* Should not happen; message already verified */

	start = pos;
	end = start + len;

	op_code = *pos++;
	flags = *pos++;
	if (flags & WSC_FLAGS_LF) {
		if (end - pos < 2) {
			da16x_eap_prt("     [%s] EAP-WSC: Message underflow\n", __func__);
			return;
		}
		message_length = WPA_GET_BE16(pos);
		pos += 2;

		if (message_length < end - pos || message_length > 50000) {
			wpa_printf(MSG_DEBUG, "EAP-WSC: Invalid Message "
				   "Length");
			return;
		}
	}

	da16x_eap_prt("     [%s] EAP-WSC: Received packet: Op-Code %d "
		   "Flags 0x%x Message Length %d\n", __func__,
		   op_code, flags, message_length);

	if (data->state == WAIT_FRAG_ACK) {
		if (op_code != WSC_FRAG_ACK) {
			da16x_eap_prt("     [%s] EAP-WSC: Unexpected Op-Code %d "
				   "in WAIT_FRAG_ACK state\n", __func__, op_code);
			eap_wsc_state(data, FAIL);
			return;
		}
		da16x_eap_prt("     [%s] EAP-WSC: Fragment acknowledged\n", __func__);
		eap_wsc_state(data, MESG);
		return;
	}

	if (op_code != WSC_ACK && op_code != WSC_NACK && op_code != WSC_MSG &&
	    op_code != WSC_Done) {
		da16x_eap_prt("     [%s] EAP-WSC: Unexpected Op-Code %d\n", __func__,
			   op_code);
		eap_wsc_state(data, FAIL);
		return;
	}

	if (data->in_buf &&
	    eap_wsc_process_cont(data, pos, end - pos, op_code) < 0) {
		eap_wsc_state(data, FAIL);
		return;
	}

	if (flags & WSC_FLAGS_MF) {
		if (eap_wsc_process_fragment(data, flags, op_code,
					     message_length, pos, end - pos) <
		    0)
			eap_wsc_state(data, FAIL);
		else
			eap_wsc_state(data, FRAG_ACK);
		return;
	}

	if (data->in_buf == NULL) {
		/* Wrap unfragmented messages as wpabuf without extra copy */
		wpabuf_set(&tmpbuf, pos, end - pos);
		data->in_buf = &tmpbuf;
	}

	res = wps_process_msg(data->wps, (enum wsc_op_code)op_code, data->in_buf);
	switch (res) {
	case WPS_DONE:
		da16x_eap_prt("     [%s] EAP-WSC: WPS processing completed "
			   "successfully - report EAP failure\n", __func__);
		eap_wsc_state(data, FAIL);
		break;
	case WPS_CONTINUE:
		eap_wsc_state(data, MESG);
		break;
	case WPS_FAILURE:
		da16x_eap_prt("     [%s] EAP-WSC: WPS processing failed\n", __func__);
		eap_wsc_state(data, FAIL);
		break;
	case WPS_PENDING:
		eap_wsc_state(data, MESG);
		sm->method_pending = METHOD_PENDING_WAIT;
		da16x_eloop_cancel_timeout(eap_wsc_ext_reg_timeout, sm, data);
		da16x_eloop_register_timeout(5, 0, eap_wsc_ext_reg_timeout,
				       sm, data);
		break;
	}

	if (data->in_buf != &tmpbuf)
		wpabuf_free(data->in_buf);
	data->in_buf = NULL;
}


static Boolean eap_wsc_isDone(struct eap_sm *sm, void *priv)
{
	struct eap_wsc_data *data = priv;
	return (Boolean)(data->state == FAIL);
}


static Boolean eap_wsc_isSuccess(struct eap_sm *sm, void *priv)
{
	/* EAP-WSC will always result in EAP-Failure */
	return FALSE;
}


static int eap_wsc_getTimeout(struct eap_sm *sm, void *priv)
{
	/* Recommended retransmit times: retransmit timeout 5 seconds,
	 * per-message timeout 15 seconds, i.e., 3 tries. */
	sm->MaxRetrans = 2; /* total 3 attempts */
	return 5;
}


int eap_server_wsc_register(void)
{
	struct eap_method *eap;

	eap = eap_server_method_alloc(EAP_SERVER_METHOD_INTERFACE_VERSION,
				      EAP_VENDOR_WFA, (EapType)EAP_VENDOR_TYPE_WSC,
				      "WSC");
	if (eap == NULL)
		return -1;

	eap->init = eap_wsc_init;
	eap->reset = eap_wsc_reset;
	eap->buildReq = eap_wsc_buildReq;
	eap->check = eap_wsc_check;
	eap->process = eap_wsc_process;
	eap->isDone = eap_wsc_isDone;
	eap->isSuccess = eap_wsc_isSuccess;
	eap->getTimeout = eap_wsc_getTimeout;

	return eap_server_method_register(eap);
}

#endif	/* EAP_SERVER_WSC */

/* EOF */
