/**
 ****************************************************************************************
 *
 * @file da16x_dpm_tls.c
 *
 * @brief TLS APIs and threads related to DPM.
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

#include "sdk_type.h"

#include "da16x_system.h"
#include "da16x_dpm_regs.h"
#include "da16x_dpm_tls.h"
#include "da16x_dpm.h"
#include "lwip/err.h"

#pragma GCC diagnostic ignored "-Wsign-compare"

#undef	DA16X_DPM_TLS_DBG_INFO
#undef	DA16X_DPM_TLS_DBG_ERR

#ifdef	DA16X_DPM_TLS_DBG_INFO
	#define	DPM_TLS_DBG_INFO	PRINTF
#else
	#define	DPM_TLS_DBG_INFO(...)	do { } while(0);
#endif	/* DA16X_DPM_TLS_DBG_INFO */

#ifdef	DA16X_DPM_TLS_DBG_ERR
	#define	DPM_TLS_DBG_ERR		PRINTF
#else
	#define	DPM_TLS_DBG_ERR(...)	do {} while(0);
#endif	/* DA16X_DPM_TLS_DBG_ERR */

#ifdef	MBEDTLS_SSL_DPM_SUPPORT
//internal function
static int is_supported_tls_dpm()
{
	if (!chk_dpm_mode()) {
		DPM_TLS_DBG_ERR("[%s]Only for dpm mode\n", __func__);
		return pdFALSE;
	}

	return pdTRUE;
}

static size_t cal_tls_session_size(mbedtls_ssl_context *ssl_ctx)
{
	size_t total_size = 0;

	//calculate ssl context size
	total_size += sizeof(mbedtls_ssl_context);
	total_size += 13;
	total_size += 13;
#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
	if (ssl_ctx->cli_id_len) {
		total_size += ssl_ctx->cli_id_len;
	}
#endif	/* MBEDTLS_SSL_DTLS_HELLO_VERIFY */

	//calculate ssl session size
	total_size += sizeof(mbedtls_ssl_session);
#if !defined(DA16X_DPM_TLS_NOT_SAVE_PERR_CERT)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (ssl_ctx->session->peer_cert) {
		total_size += ssl_ctx->session->peer_cert->raw.len;
	}
	total_size += 4;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined(MBEDTLS_SSL_CLI_C)
	if (ssl_ctx->session->ticket_len) {
		total_size += ssl_ctx->session->ticket_len;
	}
#endif /* MBEDTLS_SSL_SESSION_TICKETS && MBEDTLS_SSL_CLI_C */
#endif	/* DA16X_DPM_TLS_NOT_SAVE_PERR_CERT */

	//calculate ssl transform size
	total_size += sizeof(int);
	if (ssl_ctx->transform) {
		total_size += sizeof(ssl_ctx->transform->keyblk);
	} else if (ssl_ctx->transform_negotiate) {
		total_size += sizeof(ssl_ctx->transform_negotiate->keyblk);
	}

	DPM_TLS_DBG_INFO("[%s:%d]expected tls session size(%ld)\n",
		__func__, __LINE__, total_size);

	return total_size;
}

/*
 * @a: rtm memory ptr.
 * @alen: size of parameter a.
 * @b: mbedtls_ssl_context to save.
 */
static int is_duplicated_session(const unsigned char *start, const unsigned char *end,
			const mbedtls_ssl_context *ssl_ctx)
{
	const unsigned char *pos = start;

	pos += sizeof(mbedtls_ssl_context);

	//check in contents buffer
	if ((pos + 13 < end) && (memcmp(pos, ssl_ctx->in_buf, 13) != 0)) {
		DPM_TLS_DBG_INFO("[%s]in comming buffer is changed\n", __func__);
		return 0;
	}
	pos += 13;

	//check out contents buffer
	if ((pos + 13 < end) && (memcmp(pos, ssl_ctx->out_buf, 13) != 0)) {
		DPM_TLS_DBG_INFO("[%s]out going buffer is changed\n", __func__);
		return 0;
	}

	return 1;
}

static int save_ssl_context(mbedtls_ssl_context *ssl_ctx, unsigned char **msgpos, unsigned char *end)
{
	unsigned char *pos = *msgpos;
	size_t ssl_ctx_len = sizeof(mbedtls_ssl_context);

	if (!ssl_ctx) {
		DPM_TLS_DBG_ERR("[%s]ssl context is null\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	if (ssl_ctx_len > (unsigned int)(end - pos)) {
		DPM_TLS_DBG_ERR("[%s]buffer is not enough(%ld:%ld)\n",
			__func__, end - pos, ssl_ctx_len);
		return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
	}

	memcpy(pos, ssl_ctx, ssl_ctx_len);
	pos += ssl_ctx_len;

	memcpy(pos, ssl_ctx->in_buf, 13);
	pos += 13;

	memcpy(pos, ssl_ctx->out_buf, 13);
	pos += 13;

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
	if (ssl_ctx->cli_id_len) {
		memcpy(pos, ssl_ctx->cli_id, ssl_ctx->cli_id_len);
		pos += ssl_ctx->cli_id_len;
	}
#endif	/* MBEDTLS_SSL_DTLS_HELLO_VERIFY */

	DPM_TLS_DBG_INFO("[%s:%d]ssl context size(%ld)\n",
		__func__, __LINE__, pos - *msgpos);

	*msgpos = pos;

	return 0;
}

static int save_ssl_session(mbedtls_ssl_session *session, unsigned char **msgpos, unsigned char *end)
{
	unsigned char *pos = *msgpos;
	size_t ssl_session_len = sizeof(mbedtls_ssl_session);
#if !defined(DA16X_DPM_TLS_NOT_SAVE_PERR_CERT)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	size_t cert_len = 0;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
#endif	/* DA16X_DPM_TLS_NOT_SAVE_PERR_CERT */

	if (!session) {
		DPM_TLS_DBG_ERR("[%s]session is null\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	if (ssl_session_len > (unsigned int)(end - pos)) {
		DPM_TLS_DBG_ERR("[%s]buffer is not enough(%ld:%ld)\n",
			__func__, end - pos, ssl_session_len);
		return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
	}

	memcpy(pos, session, ssl_session_len);
	pos += ssl_session_len;

#if !defined(DA16X_DPM_TLS_NOT_SAVE_PERR_CERT)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (session->peer_cert) {
		cert_len = session->peer_cert->raw.len;
	} else {
		cert_len = 0;
	}

	if (4 + cert_len > (unsigned int)(end - pos)) {
		DPM_TLS_DBG_ERR("[%s]buffer is not enough(%ld:%ld)\n",
			__func__, end - pos, 3 + cert_len);
		return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
	}

	*pos++ = (unsigned char)( cert_len >> 24 & 0xFF );
	*pos++ = (unsigned char)( cert_len >> 16 & 0xFF );
	*pos++ = (unsigned char)( cert_len >>  8 & 0xFF );
	*pos++ = (unsigned char)( cert_len       & 0xFF );

	if (session->peer_cert)  {
		memcpy(pos, session->peer_cert->raw.p, cert_len);
	}

	pos += cert_len;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */

	//TODO: maybe it's not required. it's setup by user call.
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined(MBEDTLS_SSL_CLI_C)
	if (session->ticket_len) {
		memcpy(pos, session->ticket, session->ticket_len);
		pos += session->ticket_len;
	}
#endif /* MBEDTLS_SSL_SESSION_TICKETS && MBEDTLS_SSL_CLI_C */
#endif	/* DA16X_DPM_TLS_NOT_SAVE_PERR_CERT */

	DPM_TLS_DBG_INFO("[%s:%d]ssl session size(%ld)\n",
		__func__, __LINE__, pos - *msgpos);

	*msgpos = pos;

	return 0;
}

static int save_ssl_transform(mbedtls_ssl_transform *transform, unsigned char **msgpos, unsigned char *end)
{
	unsigned char *pos = *msgpos;
	size_t keyblk_size = sizeof(transform->keyblk);

	if (!transform) {
		DPM_TLS_DBG_ERR("[%s]transform is null\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	if (keyblk_size > (unsigned int)(end - pos)) {
		DPM_TLS_DBG_ERR("[%s]buffer is not enough(%ld:%ld)\n",
			__func__, end - pos, keyblk_size);
		return MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL;
	}

	memcpy(pos, &transform->ciphersuite_info->id, sizeof(int));
	pos += sizeof(int);

	memcpy(pos, transform->keyblk, keyblk_size);
	pos += keyblk_size;

	DPM_TLS_DBG_INFO("[%s:%d]ssl transform size(%ld)\n",
		__func__, __LINE__, pos - *msgpos);

	*msgpos = pos;

	return 0;
}

static int restore_ssl_context(mbedtls_ssl_context *dst, unsigned char **msgpos, unsigned char *end)
{
	unsigned char *pos = *msgpos;
	mbedtls_ssl_context *src = NULL;

	if (sizeof(mbedtls_ssl_context) > end - pos) {
		DPM_TLS_DBG_ERR("[%s]failed to resotre ssl context\n",
			__func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	src = (mbedtls_ssl_context *)pos;
	pos += sizeof(mbedtls_ssl_context);

	if (src->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
		DPM_TLS_DBG_ERR("[%s]ssl state is not established(%d)\n",
			__func__, src->state);
		return -1;
	}

	dst->state = src->state;

#if defined(MBEDTLS_SSL_RENEGOTIATION)
	dst->renego_status = src->renego_status;
	dst->renego_records_seen = src->renego_records_seen;
#endif	/* MBEDTLS_SSL_RENEGOTIATION */

	dst->major_ver = src->major_ver;
	dst->minor_ver = src->minor_ver;

#if defined(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)
	dst->badmac_seen = src->badmac_seen;
#endif	/* MBEDTLS_SSL_DTLS_BADMAC_LIMIT */

	mbedtls_ssl_handshake_free(dst->handshake);
	mbedtls_free(dst->handshake);
	dst->handshake = NULL;

#if defined(MBEDTLS_SSL_PROTO_DTLS)
	dst->in_epoch = src->in_epoch;
	dst->next_record_offset = src->next_record_offset;
#endif	/* MBEDTLS_SSL_PROTO_DTLS */

#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
	dst->in_window_top = src->in_window_top;
	dst->in_window = src->in_window;
#endif	/* MBEDTLS_SSL_DTLS_ANTI_REPLAY */

#if defined(MBEDTLS_SSL_RENEGOTIATION)
	dst->verify_data_len = src->verify_data_len;
	memcpy(dst->own_verify_data, src->own_verify_data, MBEDTLS_SSL_VERIFY_DATA_MAX_LEN);
	memcpy(dst->peer_verify_data, src->peer_verify_data, MBEDTLS_SSL_VERIFY_DATA_MAX_LEN);
#endif	/* MBEDTLS_SSL_RENEGOTIATION */

	dst->in_msgtype = src->in_msgtype;
	dst->in_msglen = src->in_msglen;
	dst->in_left = src->in_left;
	dst->in_hslen = src->in_hslen;
	dst->nb_zero = src->nb_zero;
	dst->keep_current_message = src->keep_current_message;

	dst->out_msgtype = src->out_msgtype;
	dst->out_msglen = src->out_msglen;
	dst->out_left = src->out_left;

#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
	dst->split_done = src->split_done;
#endif

	dst->client_auth = src->client_auth;
	dst->secure_renegotiation = src->secure_renegotiation;

	memcpy(dst->prev_in_iv, src->prev_in_iv, sizeof(dst->prev_in_iv));

	memcpy(dst->in_buf, pos, 13);
	pos += 13;

	memcpy(dst->out_buf, pos, 13);
	pos += 13;

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
	if (src->cli_id_len) {
		dst->cli_id = mbedtls_calloc(1, src->cli_id_len);
		if (dst->cli_id) {
			memcpy(dst->cli_id, pos, src->cli_id_len);
			dst->cli_id_len = src->cli_id_len;
		} else {
			DPM_TLS_DBG_ERR("[%s]Failed to copy client id\n", __func__);
		}

		pos += src->cli_id_len;
	}
#endif	/* MBEDTLS_SSL_DTLS_HELLO_VERIFY */

	*msgpos = pos;

	return 0;
}

static int restore_ssl_session(mbedtls_ssl_session *dst, unsigned char **msgpos, unsigned char *end)
{
	unsigned char *pos = *msgpos;
#if !defined(DA16X_DPM_TLS_NOT_SAVE_PERR_CERT)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	size_t cert_len = 0;
	int ret = 0;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */
#endif	/* DA16X_DPM_TLS_NOT_SAVE_PERR_CERT */
	size_t ssl_session_len = sizeof(mbedtls_ssl_session);

	if (pos + ssl_session_len > end) {
		DPM_TLS_DBG_ERR("[%s]invalid session size\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	memcpy(dst, pos, ssl_session_len);
	pos += ssl_session_len;

#if !defined(DA16X_DPM_TLS_NOT_SAVE_PERR_CERT)
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (pos + 4 > end) {
		DPM_TLS_DBG_ERR("[%s]invalid cert size\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	cert_len = (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];
	pos += 4;

	if (cert_len == 0) {
		dst->peer_cert = NULL;
	} else {
		if (pos + cert_len > end) {
			DPM_TLS_DBG_ERR("[%s]invalid cert size\n", __func__);
			return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
		}

		dst->peer_cert = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
		if (!dst->peer_cert) {
			DPM_TLS_DBG_ERR("[%s]failed to allocate memory\n", __func__);
			return MBEDTLS_ERR_SSL_ALLOC_FAILED;
		}

		mbedtls_x509_crt_init(dst->peer_cert);

		ret = mbedtls_x509_crt_parse_der(dst->peer_cert, pos, cert_len);
		if (ret) {
			DPM_TLS_DBG_ERR("[%s]failed to parse peer cert(0x%x)\n",
				__func__, -ret);

			mbedtls_x509_crt_free(dst->peer_cert);
			mbedtls_free(dst->peer_cert);
			dst->peer_cert = NULL;

			return ret;
		}

		pos += cert_len;
	}
#endif	/* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined(MBEDTLS_SSL_CLI_C)
	if (dst->ticket_len) {
		dst->ticket = mbedtls_calloc(1, dst->ticket_len);
		if(dst->ticket == NULL) {
			DPM_TLS_DBG_ERR("[%s]failed to allocate session ticket\n",
				__func__);

			return MBEDTLS_ERR_SSL_ALLOC_FAILED;
		}

		memcpy(dst->ticket, pos, dst->ticket_len);
		pos+= dst->ticket_len;
	}
#endif /* MBEDTLS_SSL_SESSION_TICKETS && MBEDTLS_SSL_CLI_C */

#else	/* DA16X_DPM_TLS_NOT_SAVE_PEER_CERT */
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	dst->peer_cert = NULL;
#endif	/* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_SSL_SESSION_TICKETS) && defined(MBEDTLS_SSL_CLI_C)
	dst->ticket_len = 0;
	dst->ticket = NULL;
#endif /* MBEDTLS_SSL_SESSION_TICKETS && MBEDTLS_SSL_CLI_C */
#endif	/* DA16X_DPM_TLS_NOT_SAVE_PEER_CERT */

	*msgpos = pos;

	return 0;
}

static int restore_ssl_transform(mbedtls_ssl_context *dst, unsigned char **msgpos, unsigned char *end)
{
	int ret = 0;

	int cipher_id = 0;
	unsigned char *keyblk = NULL;
	unsigned char *key1 = NULL;
	unsigned char *key2 = NULL;
	unsigned char *mac_enc = NULL;
	unsigned char *mac_dec = NULL;
	size_t iv_copy_len = 0;
	const mbedtls_cipher_info_t *cipher_info;
	const mbedtls_md_info_t *md_info;

	unsigned char *pos = *msgpos;

	size_t keyblk_size = sizeof(dst->transform->keyblk);

	if ((unsigned int)(end - pos) < sizeof(int)) {
		DPM_TLS_DBG_ERR("[%s]failed to restore cipher id\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	memcpy(&cipher_id, pos, sizeof(int));
	pos += sizeof(int);

	DPM_TLS_DBG_INFO("[%s:%d]cipher_id: 0x%x\n",
		__func__, __LINE__, cipher_id);

	if ((unsigned int)(end - pos) < keyblk_size) {
		DPM_TLS_DBG_ERR("[%s]failed to resotre transform\n", __func__);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	keyblk = pos;
	pos += keyblk_size;

	dst->transform->ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(cipher_id);
	if (!dst->transform->ciphersuite_info) {
		DPM_TLS_DBG_ERR("[%s]cipher info for %04x not found\n",
			__func__, cipher_id);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	cipher_info = mbedtls_cipher_info_from_type(dst->transform->ciphersuite_info->cipher);
	if (!cipher_info) {
		DPM_TLS_DBG_ERR("[%s]cipher info for %d not found\n",
			__func__, dst->transform->ciphersuite_info->cipher);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	md_info = mbedtls_md_info_from_type(dst->transform->ciphersuite_info->mac);
	if (!md_info) {
		DPM_TLS_DBG_ERR("[%s]mbedtls md info for %d not found\n",
			__func__, dst->transform->ciphersuite_info->mac);
		return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	}

	dst->transform->keylen = cipher_info->key_bitlen / 8;

	if ((cipher_info->mode == MBEDTLS_MODE_GCM)
		|| (cipher_info->mode == MBEDTLS_MODE_CCM)) {
		dst->transform->maclen = 0;
		dst->transform->ivlen = 12;
		dst->transform->fixed_ivlen = 4;

		dst->transform->minlen = dst->transform->ivlen
						- dst->transform->fixed_ivlen
						+ (dst->transform->ciphersuite_info->flags
							& MBEDTLS_CIPHERSUITE_SHORT_TAG ? 8 : 16);
	} else {
		ret = mbedtls_md_setup(&dst->transform->md_ctx_enc, md_info, 1);
		if (ret) {
			DPM_TLS_DBG_ERR("[%s]failed to setup md enc(0x%0x)\n",
				__func__, -ret);
			return ret;
		}

		ret = mbedtls_md_setup(&dst->transform->md_ctx_dec, md_info, 1);
		if (ret) {
			DPM_TLS_DBG_ERR("[%s]failed to setup md dec(0x%0x)\n",
				__func__, -ret);
			return ret;
		}

		dst->transform->maclen = mbedtls_md_get_size(md_info);

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
		if (dst->session->trunc_hmac == MBEDTLS_SSL_TRUNC_HMAC_ENABLED) {
			dst->transform->maclen = MBEDTLS_SSL_TRUNCATED_HMAC_LEN;
		}
#endif	/* MBEDTLS_SSL_TRUNCATED_HMAC */

		dst->transform->ivlen = cipher_info->iv_size;

		if (cipher_info->mode == MBEDTLS_MODE_STREAM) {
			dst->transform->minlen = dst->transform->maclen;
		} else {
#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
			if (dst->session->encrypt_then_mac == MBEDTLS_SSL_ETM_ENABLED) {
				dst->transform->minlen = dst->transform->maclen + cipher_info->block_size;
			} else
#endif	/* MBEDTLS_SSL_ENCRYPT_THEN_MAC */
			{
				dst->transform->minlen = dst->transform->maclen
								+ cipher_info->block_size
								- dst->transform->maclen % cipher_info->block_size;
			}

#if defined(MBEDTLS_SSL_PROTO_SSL3) || defined(MBEDTLS_SSL_PROTO_TLS1)
			if ((dst->minor_ver == MBEDTLS_SSL_MINOR_VERSION_0)
				|| (dst->minor_ver == MBEDTLS_SSL_MINOR_VERSION_1)) {
				;
			} else
#endif	/* MBEDTLS_SSL_PROTO_SSL3 || MBEDTLS_SSL_PROTO_TLS1 */
#if defined(MBEDTLS_SSL_PROTO_TLS1_1) || defined(MBEDTLS_SSL_PROTO_TLS1_2)
			if ((dst->minor_ver == MBEDTLS_SSL_MINOR_VERSION_2)
				|| (dst->minor_ver == MBEDTLS_SSL_MINOR_VERSION_3)) {
				dst->transform->minlen += dst->transform->ivlen;
			} else
#endif	/* MBEDTLS_SSL_PROTO_TLS1_1 || MBEDTLS_SSL_PROTO_TLS1_2 */
			{
				DPM_TLS_DBG_ERR("[%s]failed to restore mac info\n",
					__func__);
				return -1;
			}
		}
	}

	DPM_TLS_DBG_INFO("[%s:%d]restored transform info"
		"(keylen:%d,maclen:%d,ivlen:%d,fixed_ivlen:%d,minlen:%d)\n",
		__func__, __LINE__,
		dst->transform->keylen,
		dst->transform->maclen,
		dst->transform->ivlen,
		dst->transform->fixed_ivlen,
		dst->transform->minlen);

#if defined(MBEDTLS_SSL_CLI_C)
	if (dst->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
		key1 = keyblk + dst->transform->maclen * 2;
		key2 = keyblk + dst->transform->maclen * 2 + dst->transform->keylen;

		mac_enc = keyblk;
		mac_dec = keyblk + dst->transform->maclen;

		iv_copy_len = (dst->transform->fixed_ivlen) ? dst->transform->fixed_ivlen : dst->transform->ivlen;

		memcpy(dst->transform->iv_enc, key2 + dst->transform->keylen, iv_copy_len);
		memcpy(dst->transform->iv_dec, key2 + dst->transform->keylen + iv_copy_len, iv_copy_len);
	} else
#endif	/* MBEDTLS_SSL_CLI_C */
#if defined(MBEDTLS_SSL_SRV_C)
	if(dst->conf->endpoint == MBEDTLS_SSL_IS_SERVER) {
		key1 = keyblk + dst->transform->maclen * 2 + dst->transform->keylen;
		key2 = keyblk + dst->transform->maclen * 2;

		mac_enc = keyblk + dst->transform->maclen;
		mac_dec = keyblk;

		iv_copy_len = (dst->transform->fixed_ivlen ) ? dst->transform->fixed_ivlen : dst->transform->ivlen;
		memcpy(dst->transform->iv_dec, key1 + dst->transform->keylen, iv_copy_len );
		memcpy(dst->transform->iv_enc, key1 + dst->transform->keylen + iv_copy_len, iv_copy_len);
	} else
#endif	/* MBEDTLS_SSL_SRV_C */
	{
		DPM_TLS_DBG_ERR("[%s]failed to restore transform - key\n", __func__);
		return -1;
	}

	memcpy(dst->transform->keyblk, keyblk, keyblk_size);

#if defined(MBEDTLS_SSL_PROTO_SSL3)
	if (dst->minor_ver == MBEDTLS_SSL_MINOR_VERSION_0) {
		if (dst->transform->maclen > sizeof(dst->transform->mac_enc)) {
			DPM_TLS_DBG_ERR("[%s]failed to restore transform\n",
				__func__);
			return -1;
		}

		memcpy(dst->transform->mac_enc, mac_enc, dst->transform->maclen);
		memcpy(dst->transform->mac_dec, mac_dec, dst->transform->maclen);
	} else
#endif	/* MBEDTLSS_SSL_PROTO_SSL3 */
#if defined(MBEDTLS_SSL_PROTO_TLS1) || defined(MBEDTLS_SSL_PROTO_TLS1_1) \
	|| defined(MBEDTLS_SSL_PROTO_TLS1_2)
	if (dst->minor_ver >= MBEDTLS_SSL_MINOR_VERSION_1) {
		mbedtls_md_hmac_starts(&dst->transform->md_ctx_enc, mac_enc, dst->transform->maclen);
		mbedtls_md_hmac_starts(&dst->transform->md_ctx_dec, mac_dec, dst->transform->maclen);
	} else
#endif
	{
		DPM_TLS_DBG_ERR("[%s]failed to restore transform - mac\n", __func__);
	}

	ret = mbedtls_cipher_setup(&dst->transform->cipher_ctx_enc, cipher_info);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_setup returned 0x%x\n",
			__func__, -ret);
		return ret;
	}

	ret = mbedtls_cipher_setup(&dst->transform->cipher_ctx_dec, cipher_info);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_setup returned 0x%x\n",
			__func__, -ret);
		return -1;
	}

	ret = mbedtls_cipher_setkey(&dst->transform->cipher_ctx_enc,
				key1, cipher_info->key_bitlen, MBEDTLS_ENCRYPT);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_setkey returned 0x%x\n",
				__func__, -ret);
		return ret;
	}

	ret = mbedtls_cipher_setkey(&dst->transform->cipher_ctx_dec,
				key2, cipher_info->key_bitlen, MBEDTLS_DECRYPT);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_setkey returned 0x%x\n",
				__func__, -ret);
		return ret;
	}

#if defined(MBEDTLS_CIPHER_MODE_CBC)
	if (cipher_info->mode == MBEDTLS_MODE_CBC) {
		ret = mbedtls_cipher_set_padding_mode(&dst->transform->cipher_ctx_enc,
					MBEDTLS_PADDING_NONE);
		if (ret) {
			DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_set_padding_mode returned 0x%x\n",
				__func__, -ret);
			return ret;
		}

		ret = mbedtls_cipher_set_padding_mode(&dst->transform->cipher_ctx_dec,
					MBEDTLS_PADDING_NONE);
		if (ret) {
			DPM_TLS_DBG_ERR("[%s]failed mbedtls_cipher_set_padding_mode returned 0x%x\n",
				__func__, -ret);
			return ret;
		}
	}
#endif  /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_ZLIB_SUPPORT)
	if (dst->session->compression == MBEDTLS_SSL_COMPRESS_DEFLATE) {
		if (dst->compress_buf == NULL) {
			dst->compress_buf = mbedtls_calloc(1, MBEDTLS_SSL_BUFFER_LEN);
			if (!dst->compress_buf) {
				DPM_TLS_DBG_ERR("[%s]failed to allocate memory\n",
					__func__, __LINE__);
				return -1;
			}
		}

		DPM_TLS_DBG_INFO("[%s:%d]Initializing zlib states\n",
			__func__, __LINE__);

		memset(&dst->transform->ctx_deflate, 0x00, sizeof(dst->transform->ctx_deflate));
		memset(&dst->transform->ctx_inflate, 0x00, sizeof(dst->transform->ctx_inflate));

		if ((deflateInit(&dst->transform->ctx_deflate, Z_DEFAULT_COMPRESSION) != Z_OK)
			|| (inflateInit(&dst->transform->ctx_inflate) != Z_OK)) {

			DPM_TLS_DBG_ERR("[%s]failed to initialize compression\n",
				__func__);
			return -1;
		}
	}
#endif  /* MBEDTLS_ZLIB_SUPPORT */

	return 0;
}

//external function
int set_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx)
{
	int ret = 0;
	size_t expected_size = 0;

	unsigned char *pos = NULL;
	unsigned char *end = NULL;

	unsigned int status = ERR_OK;
	unsigned int stored_size = 0;
	unsigned long wait_option = DA16X_DPM_TLS_DEF_TIMEOUT;

	if (!is_supported_tls_dpm()) {
		DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
		return (ER_NOT_SUPPORTED);
	}

	if ((ssl_ctx == NULL)
		|| (ssl_ctx->session == NULL)
		|| ((ssl_ctx->transform == NULL) && (ssl_ctx->transform_negotiate == NULL))) {
		DPM_TLS_DBG_ERR("[%s]Invalid tls session\n", __func__);
		return (ER_INVALID_PARAMETERS);
	}

	if (ssl_ctx->state != MBEDTLS_SSL_HANDSHAKE_OVER) {
		DPM_TLS_DBG_ERR("[%s]tls session is not established(%d)\n",
			__func__, ssl_ctx->state);
		return -1;
	}

	//Calculate expected tls session size
	expected_size = cal_tls_session_size(ssl_ctx);
	if (expected_size == 0) {
		DPM_TLS_DBG_ERR("[%s]failed to calculate tls session size\n",
			__func__, __LINE__);
		return -1;
	}

	//allocate rtm memory
	status = dpm_user_rtm_allocate((char *)name, (void **)&pos, expected_size, wait_option);
	if (status == ER_DUPLICATED_ENTRY) {
		stored_size = dpm_user_rtm_get((char *)name, &pos);
		if (stored_size != expected_size) {
			//release previous one & allocate new one
			status = dpm_user_rtm_release((char *)name);
			if (status) {
				DPM_TLS_DBG_ERR("[%s]failed to release rtm memory(0x%02x)\n",
					__func__, status);
				return -1;
			}

			status = dpm_user_rtm_allocate((char *)name, (VOID **)&pos,
						expected_size, wait_option);
			if (status) {
				DPM_TLS_DBG_ERR("[%s]failed to allocate rtm memory(0x%02x)\n",
					__func__, status);
				return -1;
			}
		} else {
			if (is_duplicated_session(pos, pos + expected_size, ssl_ctx)) {
				DPM_TLS_DBG_INFO("[%s]No need to save tls session\n", __func__);
				return 0;
			}
		}
	} else if (status == ERR_OK) {
		DPM_TLS_DBG_INFO("[%s]Succeed allocated rtm memory(%ld)\n",
			__func__, expected_size);
	} else {
		DPM_TLS_DBG_ERR("[%s]failed to allocate rtm memory(0x%02x)\n",
			__func__, status);
		return -1;
	}

	memset(pos, 0x00, expected_size);
	end = pos + expected_size;

	//save ssl context
	ret = save_ssl_context(ssl_ctx, &pos, end);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to save ssl context(0x%x)\n",
			__func__, ret);
		goto fail;
	}

	//save ssl session
	ret = save_ssl_session(ssl_ctx->session, &pos, end);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to save ssl session(0x%x)\n",
			__func__, ret);
		goto fail;
	}

	//save ssl transform
	if (ssl_ctx->transform) {
		ret = save_ssl_transform(ssl_ctx->transform, &pos, end);
	} else if (ssl_ctx->transform_negotiate) {
		ret = save_ssl_transform(ssl_ctx->transform_negotiate, &pos, end);
	}
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to save ssl transform(0x%x)\n",
			__func__, -ret);
		goto fail;
	}

	return 0;

fail:
	dpm_user_rtm_release((char *)name);

	return ret;
}

int get_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx)
{
	int ret = 0;

	unsigned char *pos = NULL;
	unsigned char *end = NULL;

	unsigned int stored_size = 0;

	if (!is_supported_tls_dpm()) {
		DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
		return (ER_NOT_SUPPORTED);
	}

	//check initialization
	if ((ssl_ctx == NULL)
		|| (ssl_ctx->state != 0)) {
		DPM_TLS_DBG_ERR("[%s]tls session has to be initialized\n",
			__func__);
		return (ER_INVALID_PARAMETERS);
	}

	//get allocated rtm memory
	stored_size = dpm_user_rtm_get((char *)name, &pos);
	if (stored_size == 0) {
		DPM_TLS_DBG_INFO("[%s]there is no saved data\n", __func__);
		return (ER_NOT_FOUND);
	}

	end = pos + stored_size;

	//restore ssl context
	ret = restore_ssl_context(ssl_ctx, &pos, end);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to restore ssl context(0x%x)\n",
			__func__, -ret);
		return ret;
	}

	//restore ssl session
	ssl_ctx->session = ssl_ctx->session_negotiate;
	ssl_ctx->session_in = ssl_ctx->session;
	ssl_ctx->session_out = ssl_ctx->session;
	ssl_ctx->session_negotiate = NULL;

	ret = restore_ssl_session(ssl_ctx->session, &pos, end);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to restore ssl session(0x%x)\n",
			__func__, -ret);
		return ret;
	}

	//restore ssl transform
	ssl_ctx->transform = ssl_ctx->transform_negotiate;
	ssl_ctx->transform_in = ssl_ctx->transform;
	ssl_ctx->transform_out = ssl_ctx->transform;
	ssl_ctx->transform_negotiate = NULL;

	ret = restore_ssl_transform(ssl_ctx, &pos, end);
	if (ret) {
		DPM_TLS_DBG_ERR("[%s]failed to restore ssl transform(0x%x)\n",
			__func__, -ret);
		return ret;
	}

	//Set the msg pointer to the correct location based on IV length
	if (ssl_ctx->minor_ver >= MBEDTLS_SSL_MINOR_VERSION_2) {
		ssl_ctx->in_msg = ssl_ctx->in_iv
					+ ssl_ctx->transform->ivlen
					- ssl_ctx->transform->fixed_ivlen;
	} else {
		ssl_ctx->in_msg = ssl_ctx->in_iv;
	}

	if (ssl_ctx->minor_ver >= MBEDTLS_SSL_MINOR_VERSION_2) {
		ssl_ctx->out_msg = ssl_ctx->out_iv
					+ ssl_ctx->transform->ivlen
					- ssl_ctx->transform->fixed_ivlen;
	} else {
		ssl_ctx->out_msg = ssl_ctx->out_iv;
	}

	return 0;
}

int clr_tls_session(const char *name)
{
	unsigned int status = 0;

	if (!is_supported_tls_dpm()) {
		DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
		return ER_NOT_SUPPORTED;
	}

	status = dpm_user_rtm_release((char *)name);
	if (status) {
		DPM_TLS_DBG_ERR("[%s]failed to release rtm memory(0x%02x)\n",
			__func__, status);
		return status;
	}

	return 0;
}
#else
int set_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx)
{
	DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
	return -1;
}

int get_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx)
{
	DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
	return -1;
}

int clr_tls_session(const char *name)
{
	DPM_TLS_DBG_ERR("[%s]Not supported\n", __func__);
	return -1;
}
#endif	/* MBEDTLS_SSL_DPM_SUPPORT */

/* EOF */
