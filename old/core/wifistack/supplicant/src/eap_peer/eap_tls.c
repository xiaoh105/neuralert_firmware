/*
 * EAP peer method: EAP-TLS (RFC 2716)
 * Copyright (c) 2004-2008, 2012-2015, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#ifdef	EAP_TLS

#include "supp_common.h"
#include "crypto/tls.h"
#include "eap_i.h"
#include "eap_tls_common.h"
#include "eap_config.h"


static void eap_tls_deinit(struct eap_sm *sm, void *priv);


struct eap_tls_data {
	struct eap_ssl_data ssl;
	u8 *key_data;
	u8 *session_id;
	size_t id_len;
	void *ssl_ctx;
	u8 eap_type;
	struct wpabuf *pending_resp;
};


static void * eap_tls_init(struct eap_sm *sm)
{
	struct eap_tls_data *data;
	struct eap_peer_config *config = eap_get_config(sm);
#if 0	/* by Shingu */
	if (config == NULL ||
	    ((sm->init_phase2 ? config->private_key2 : config->p_key)
	     == NULL &&
	     (sm->init_phase2 ? config->engine2 : config->engine) == 0)) {
		da16x_eap_prt("[%s] EAP-TLS: Private key not configured\n",
			     __func__);
		return NULL;
	}
#endif

	data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;

	data->ssl_ctx = sm->init_phase2 && sm->ssl_ctx2 ? sm->ssl_ctx2 :
		sm->ssl_ctx;

	if (eap_peer_tls_ssl_init(sm, &data->ssl, config, EAP_TYPE_TLS)) {
		da16x_eap_prt("[%s] EAP-TLS: Failed to initialize SSL.\n",
			     __func__);
		eap_tls_deinit(sm, data);
		if (config->engine) {
			da16x_eap_prt("[%s] EAP-TLS: Requesting Smartcard "
				     "PIN\n", __func__);
			eap_sm_request_pin(sm);
			sm->ignore = TRUE;
		} else if (config->private_key && !config->private_key_passwd)
		{
			da16x_eap_prt("[%s] EAP-TLS: Requesting private "
				     "key passphrase\n", __func__);
			eap_sm_request_passphrase(sm);
			sm->ignore = TRUE;
		}
		return NULL;
	}

	data->eap_type = EAP_TYPE_TLS;

	return data;
}


#ifdef EAP_UNAUTH_TLS
static void * eap_unauth_tls_init(struct eap_sm *sm)
{
	struct eap_tls_data *data;
	struct eap_peer_config *config = eap_get_config(sm);

	data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;

	data->ssl_ctx = sm->init_phase2 && sm->ssl_ctx2 ? sm->ssl_ctx2 :
		sm->ssl_ctx;

	if (eap_peer_tls_ssl_init(sm, &data->ssl, config,
				  EAP_UNAUTH_TLS_TYPE)) {
		wpa_printf(MSG_INFO, "EAP-TLS: Failed to initialize SSL.");
		eap_tls_deinit(sm, data);
		return NULL;
	}

	data->eap_type = EAP_UNAUTH_TLS_TYPE;

	return data;
}
#endif /* EAP_UNAUTH_TLS */


#ifdef CONFIG_HS20
static void * eap_wfa_unauth_tls_init(struct eap_sm *sm)
{
	struct eap_tls_data *data;
	struct eap_peer_config *config = eap_get_config(sm);

	data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;

	data->ssl_ctx = sm->init_phase2 && sm->ssl_ctx2 ? sm->ssl_ctx2 :
		sm->ssl_ctx;

	if (eap_peer_tls_ssl_init(sm, &data->ssl, config,
				  EAP_WFA_UNAUTH_TLS_TYPE)) {
		wpa_printf(MSG_INFO, "EAP-TLS: Failed to initialize SSL.");
		eap_tls_deinit(sm, data);
		return NULL;
	}

	data->eap_type = EAP_WFA_UNAUTH_TLS_TYPE;

	return data;
}
#endif /* CONFIG_HS20 */


static void eap_tls_free_key(struct eap_tls_data *data)
{
	if (data->key_data) {
#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
		bin_clear_free(data->key_data, EAP_TLS_KEY_LEN + EAP_EMSK_LEN);
#else
		os_free(data->key_data);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */
		data->key_data = NULL;
	}
}


static void eap_tls_deinit(struct eap_sm *sm, void *priv)
{
	struct eap_tls_data *data = priv;
	if (data == NULL)
		return;
	eap_peer_tls_ssl_deinit(sm, &data->ssl);
	eap_tls_free_key(data);
	os_free(data->session_id);
	wpabuf_free(data->pending_resp);
	os_free(data);
}


static struct wpabuf * eap_tls_failure(struct eap_sm *sm,
				       struct eap_tls_data *data,
				       struct eap_method_ret *ret, int res,
				       struct wpabuf *resp, u8 id)
{
	da16x_eap_prt("[%s] EAP-TLS: TLS processing failed\n", __func__);

	ret->methodState = METHOD_DONE;
	ret->decision = DECISION_FAIL;

	if (resp) {
		/*
		 * This is likely an alert message, so send it instead of just
		 * ACKing the error.
		 */
		return resp;
	}

	return eap_peer_tls_build_ack(id, data->eap_type, 0);
}


static void eap_tls_success(struct eap_sm *sm, struct eap_tls_data *data,
			    struct eap_method_ret *ret)
{
	const char *label;

	wpa_printf(MSG_DEBUG, "EAP-TLS: Done");

	if (data->ssl.tls_out) {
		wpa_printf(MSG_DEBUG, "EAP-TLS: Fragment(s) remaining");
		return;
	}

	if (data->ssl.tls_v13) {
		label = "EXPORTER_EAP_TLS_Key_Material";

		/* A possible NewSessionTicket may be received before
		 * EAP-Success, so need to allow it to be received. */
		ret->methodState = METHOD_MAY_CONT;
		ret->decision = DECISION_COND_SUCC;
	} else {
		label = "client EAP encryption";

		ret->methodState = METHOD_DONE;
		ret->decision = DECISION_UNCOND_SUCC;
	}

	eap_tls_free_key(data);
	data->key_data = eap_peer_tls_derive_key(sm, &data->ssl, label,
						 EAP_TLS_KEY_LEN +
						 EAP_EMSK_LEN);
	if (data->key_data) {
		wpa_hexdump_key(MSG_DEBUG, "EAP-TLS: Derived key",
				data->key_data, EAP_TLS_KEY_LEN);
		wpa_hexdump_key(MSG_DEBUG, "EAP-TLS: Derived EMSK",
				data->key_data + EAP_TLS_KEY_LEN,
				EAP_EMSK_LEN);
	} else {
		da16x_eap_prt("[%s] EAP-TLS: Failed to derive key\n", __func__);
	}

	os_free(data->session_id);
	data->session_id = eap_peer_tls_derive_session_id(sm, &data->ssl,
							  EAP_TYPE_TLS,
			                                  &data->id_len);
	if (data->session_id) {
		da16x_dump("EAP-TLS: Derived Session-Id",
			  data->session_id, data->id_len);
	} else {
		da16x_eap_prt("[%s] EAP-TLS: Failed to derive Session-Id\n",
			     __func__);
	}
}


static struct wpabuf * eap_tls_process(struct eap_sm *sm, void *priv,
				       struct eap_method_ret *ret,
				       const struct wpabuf *reqData)
{
	size_t left;
	int res;
	struct wpabuf *resp;
	u8 flags, id;
	const u8 *pos;
	struct eap_tls_data *data = priv;
	struct wpabuf msg;

	if (sm->waiting_ext_cert_check && data->pending_resp) {
		struct eap_peer_config *config = eap_get_config(sm);

		if (config->pending_ext_cert_check == EXT_CERT_CHECK_GOOD) {
			da16x_eap_prt("[%s]EAP-TLS: External certificate check succeeded - continue handshake\n", __func__);
			resp = data->pending_resp;
			data->pending_resp = NULL;
			sm->waiting_ext_cert_check = 0;
			return resp;
		}

		if (config->pending_ext_cert_check == EXT_CERT_CHECK_BAD) {
			da16x_eap_prt("[%s]EAP-TLS: External certificate check failed - force authentication failure\n", __func__);
			ret->methodState = METHOD_DONE;
			ret->decision = DECISION_FAIL;
			sm->waiting_ext_cert_check = 0;
			return NULL;
		}

		da16x_eap_prt("[%s]EAP-TLS: Continuing to wait external server certificate validation\n", __func__);
		return NULL;
	}

	pos = eap_peer_tls_process_init(sm, &data->ssl, data->eap_type, ret,
					reqData, &left, &flags);
	if (pos == NULL)
		return NULL;
	id = eap_get_id(reqData);

	if (flags & EAP_TLS_FLAGS_START) {
		da16x_eap_prt("[%s] EAP-TLS: Start\n", __func__);
		left = 0; /* make sure that this frame is empty, even though it
			   * should always be, anyway */
	}

	resp = NULL;
	wpabuf_set(&msg, pos, left);
	res = eap_peer_tls_process_helper(sm, &data->ssl, data->eap_type, 0,
					  id, &msg, &resp);

	if (res < 0) {
		return eap_tls_failure(sm, data, ret, res, resp, id);
	}

	if (sm->waiting_ext_cert_check) {
		da16x_eap_prt("[%s]EAP-TLS: Waiting external server certificate validation\n", __func__);
		wpabuf_free(data->pending_resp);
		data->pending_resp = resp;
		return NULL;
	}

	if (tls_connection_established(data->ssl_ctx, data->ssl.conn))
		eap_tls_success(sm, data, ret);

	if (res == 1) {
		wpabuf_free(resp);
		return eap_peer_tls_build_ack(id, data->eap_type, 0);
	}

	return resp;
}


static Boolean eap_tls_has_reauth_data(struct eap_sm *sm, void *priv)
{
	struct eap_tls_data *data = priv;
	return tls_connection_established(data->ssl_ctx, data->ssl.conn);
}


static void eap_tls_deinit_for_reauth(struct eap_sm *sm, void *priv)
{
	struct eap_tls_data *data = priv;

	wpabuf_free(data->pending_resp);
	data->pending_resp = NULL;
}


static void * eap_tls_init_for_reauth(struct eap_sm *sm, void *priv)
{
	struct eap_tls_data *data = priv;
	eap_tls_free_key(data);
	os_free(data->session_id);
	data->session_id = NULL;
	if (eap_peer_tls_reauth_init(sm, &data->ssl)) {
		os_free(data);
		return NULL;
	}
	return priv;
}


static int eap_tls_get_status(struct eap_sm *sm, void *priv, char *buf,
			      size_t buflen, int verbose)
{
	struct eap_tls_data *data = priv;
	return eap_peer_tls_status(sm, &data->ssl, buf, buflen, verbose);
}


static Boolean eap_tls_isKeyAvailable(struct eap_sm *sm, void *priv)
{
	struct eap_tls_data *data = priv;
	return data->key_data != NULL;
}


static u8 * eap_tls_getKey(struct eap_sm *sm, void *priv, size_t *len)
{
	struct eap_tls_data *data = priv;
	u8 *key;

	if (data->key_data == NULL)
		return NULL;

	key = os_memdup(data->key_data, EAP_TLS_KEY_LEN);
	if (key == NULL)
		return NULL;

	*len = EAP_TLS_KEY_LEN;

	return key;
}


static u8 * eap_tls_get_emsk(struct eap_sm *sm, void *priv, size_t *len)
{
	struct eap_tls_data *data = priv;
	u8 *key;

	if (data->key_data == NULL)
		return NULL;

	key = os_memdup(data->key_data + EAP_TLS_KEY_LEN, EAP_EMSK_LEN);
	if (key == NULL)
		return NULL;

	*len = EAP_EMSK_LEN;

	return key;
}


static u8 * eap_tls_get_session_id(struct eap_sm *sm, void *priv, size_t *len)
{
	struct eap_tls_data *data = priv;
	u8 *id;

	if (data->session_id == NULL)
		return NULL;

	id = os_memdup(data->session_id, data->id_len);
	if (id == NULL)
		return NULL;

	*len = data->id_len;

	return id;
}


int eap_peer_tls_register(void)
{
	struct eap_method *eap;

	eap = eap_peer_method_alloc(EAP_PEER_METHOD_INTERFACE_VERSION,
				    EAP_VENDOR_IETF, EAP_TYPE_TLS, "TLS");
	if (eap == NULL)
		return -1;

	eap->init = eap_tls_init;
	eap->deinit = eap_tls_deinit;
	eap->process = eap_tls_process;
	eap->isKeyAvailable = eap_tls_isKeyAvailable;
	eap->getKey = eap_tls_getKey;
	eap->getSessionId = eap_tls_get_session_id;
	eap->get_status = eap_tls_get_status;
	eap->has_reauth_data = eap_tls_has_reauth_data;
	eap->deinit_for_reauth = eap_tls_deinit_for_reauth;
	eap->init_for_reauth = eap_tls_init_for_reauth;
	eap->get_emsk = eap_tls_get_emsk;

	return eap_peer_method_register(eap);
}


#ifdef EAP_UNAUTH_TLS
int eap_peer_unauth_tls_register(void)
{
	struct eap_method *eap;

	eap = eap_peer_method_alloc(EAP_PEER_METHOD_INTERFACE_VERSION,
				    EAP_VENDOR_UNAUTH_TLS,
				    EAP_VENDOR_TYPE_UNAUTH_TLS, "UNAUTH-TLS");
	if (eap == NULL)
		return -1;

	eap->init = eap_unauth_tls_init;
	eap->deinit = eap_tls_deinit;
	eap->process = eap_tls_process;
	eap->isKeyAvailable = eap_tls_isKeyAvailable;
	eap->getKey = eap_tls_getKey;
	eap->get_status = eap_tls_get_status;
	eap->has_reauth_data = eap_tls_has_reauth_data;
	eap->deinit_for_reauth = eap_tls_deinit_for_reauth;
	eap->init_for_reauth = eap_tls_init_for_reauth;
	eap->get_emsk = eap_tls_get_emsk;

	return eap_peer_method_register(eap);
}
#endif /* EAP_UNAUTH_TLS */


#ifdef CONFIG_HS20
int eap_peer_wfa_unauth_tls_register(void)
{
	struct eap_method *eap;

	eap = eap_peer_method_alloc(EAP_PEER_METHOD_INTERFACE_VERSION,
				    EAP_VENDOR_WFA_NEW,
				    EAP_VENDOR_WFA_UNAUTH_TLS,
				    "WFA-UNAUTH-TLS");
	if (eap == NULL)
		return -1;

	eap->init = eap_wfa_unauth_tls_init;
	eap->deinit = eap_tls_deinit;
	eap->process = eap_tls_process;
	eap->isKeyAvailable = eap_tls_isKeyAvailable;
	eap->getKey = eap_tls_getKey;
	eap->get_status = eap_tls_get_status;
	eap->has_reauth_data = eap_tls_has_reauth_data;
	eap->deinit_for_reauth = eap_tls_deinit_for_reauth;
	eap->init_for_reauth = eap_tls_init_for_reauth;
	eap->get_emsk = eap_tls_get_emsk;

	return eap_peer_method_register(eap);
}
#endif /* CONFIG_HS20 */
#endif	/* EAP_TLS */
