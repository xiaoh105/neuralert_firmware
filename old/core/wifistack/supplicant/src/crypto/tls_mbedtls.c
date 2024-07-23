/**
 ****************************************************************************************
 *
 * @file tls_mbedtls.c
 *
 * @brief TLS functions linking between Supplicant and mbed TLS
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

#include "includes.h"

#include "tls.h"
#include "sha1.h"

#include "mbedtls/config.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"

#include "common_config.h"
#include "da16x_time.h"

#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-parameter"

static int tls_mbedtls_ref_count = 0;

#if defined(MBEDTLS_SSL_PROTO_TLS1)
u16 da16x_tls_version = 0x301;
#elif defined(MBEDTLS_SSL_PROTO_TLS1_1)
u16 da16x_tls_version = 0x302;
#else
u16 da16x_tls_version = 0x303;
#endif


int tls_mbedtls_cert_verify_flags =
				(MBEDTLS_X509_BADCERT_EXPIRED)
#if 0 /* to check ceritifcate's vailidity only. */
				| (MBEDTLS_X509_BADCERT_OTHER)
				| (MBEDTLS_X509_BADCERT_CN_MISMATCH)
				| (MBEDTLS_X509_BADCERT_KEY_USAGE)
				| (MBEDTLS_X509_BADCERT_EXT_KEY_USAGE)
				| (MBEDTLS_X509_BADCERT_NS_CERT_TYPE)
				| (MBEDTLS_X509_BADCERT_BAD_PK)
				| (MBEDTLS_X509_BADCERT_BAD_KEY)
				| (MBEDTLS_X509_BADCERT_REVOKED)
				| (MBEDTLS_X509_BADCERT_NOT_TRUSTED)
#endif
				| (MBEDTLS_X509_BADCERT_FUTURE);


struct tls_global {

	int server;

	mbedtls_x509_crt *ca_cert;
	mbedtls_x509_crt *cert;
	mbedtls_pk_context *priv_key;	
	mbedtls_pk_context *priv_key_alt;
	mbedtls_dhm_context *dhm_ctx;

	int params_set;
};


int get_ent_cert_verify_flags(void)
{
	return tls_mbedtls_cert_verify_flags;
}

void set_ent_cert_verify_flags(char flag, int value)
{
	if (flag == pdFALSE)
	{
		if (tls_mbedtls_cert_verify_flags & value)
		{
			tls_mbedtls_cert_verify_flags &= ~(value);
		}
	}
	else
	{
		if (!(tls_mbedtls_cert_verify_flags & value))
		{
			tls_mbedtls_cert_verify_flags |= value;
		}
	}
}

struct tls_connection {
	struct tls_global *global;

	struct wpabuf *send_buf;
	struct wpabuf *recv_buf;
	const u8 *recv_buf_offset;

	mbedtls_ssl_context *ssl_ctx;
	mbedtls_ssl_config *ssl_conf;
	mbedtls_entropy_context *entropy_ctx;
	mbedtls_ctr_drbg_context *ctr_drbg_ctx;

	mbedtls_x509_crt *ca_cert;
	mbedtls_x509_crt *cert;
	mbedtls_pk_context *priv_key;
	mbedtls_pk_context *priv_key_alt;
	mbedtls_dhm_context *dhm_ctx;

#define TLS_RANDOM_LEN 32
#define TLS_MASTER_SECRET_LEN 48
	unsigned char master[TLS_MASTER_SECRET_LEN];
	unsigned char client_random[TLS_RANDOM_LEN];
	unsigned char server_random[TLS_RANDOM_LEN];
	int (*tls_prf)(const unsigned char *secret, size_t slen,
			const char *label,
			const unsigned char *random, size_t rlen,
			unsigned char *dstbuf, size_t dlen);

	size_t maclen;
	size_t keylen;
	size_t ivlen;

	int auth_mode;

	int params_set;
	int established;

	tls_session_ticket_cb session_ticket_cb;
	void *session_ticket_cb_ctx;

	u8 *session_ticket;
	size_t session_ticket_len;

#define	TLS_MAX_CIPHER_LISTS 10
	int cipher_list[TLS_MAX_CIPHER_LISTS];
	int cipher_count;
};

extern void da16x_secure_module_init();
extern void da16x_secure_module_deinit();

#define	CRYPTO_FUNC_START	da16x_crypto_prt("[%s]Start\n", __func__);
#define	CRYPTO_FUNC_END		da16x_crypto_prt("[%s]End\n", __func__);

static void tls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
#ifdef ENABLE_CRYPTO_DBG
	const char *p, *basename;

	/* Extract basename from file */
	for( p = basename = file; *p != '\0'; p++ )
		if( *p == '/' || *p == '\\' )
			basename = p + 1;

	da16x_crypto_prt("%s:%04d: |%d| %s\n", basename, line, level, str );
#endif

	return ;
}

void * tls_init(const struct tls_config *conf)
{
	struct tls_global *global;

	CRYPTO_FUNC_START;

	global = os_zalloc(sizeof(*global));
	if (global == NULL) {
		return NULL;
	}

	tls_mbedtls_ref_count++;

	return global;
}

void tls_deinit(void *ssl_ctx)
{
	struct tls_global *global = ssl_ctx;

	CRYPTO_FUNC_START;

	if (global) {
		if (global->params_set) {
			if (global->ca_cert) {
				mbedtls_x509_crt_free(global->ca_cert);
				os_free(global->ca_cert);
			}

			if (global->cert) {
				mbedtls_x509_crt_free(global->cert);
				os_free(global->cert);
			}

			if (global->priv_key) {
				mbedtls_pk_free(global->priv_key);
				os_free(global->priv_key);
			}

			if (global->priv_key_alt) {
				mbedtls_pk_free(global->priv_key_alt);
				os_free(global->priv_key_alt);
			}

			if (global->dhm_ctx) {
				mbedtls_dhm_free(global->dhm_ctx);
				os_free(global->dhm_ctx);
			}
		}

		os_free(global);
	}

	tls_mbedtls_ref_count--;

	return ;
}

int tls_get_errors(void *ssl_ctx)
{
	CRYPTO_FUNC_START;

	return 0;
}

static int tls_recv_func(void *ctx, unsigned char *buf, size_t len)
{
	struct tls_connection *conn = (struct tls_connection *)ctx;
	const u8 *end;

	CRYPTO_FUNC_START;

	if (conn->recv_buf == NULL) {
		da16x_crypto_prt("[%s]recv_buf is empty\n",
			__func__);
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}

	end = wpabuf_head_u8(conn->recv_buf) + wpabuf_len(conn->recv_buf);
	if ((size_t)(end - conn->recv_buf_offset) < len) {
		len = end - conn->recv_buf_offset;
	}
	os_memcpy(buf, conn->recv_buf_offset, len);
	conn->recv_buf_offset += len;
	if (conn->recv_buf_offset == end) {
		//recv_buf consumed
		da16x_crypto_prt("[%s]recv_buf consumed\n", __func__);
		wpabuf_free(conn->recv_buf);
		conn->recv_buf = NULL;
		conn->recv_buf_offset = NULL;
	} else {
		da16x_crypto_prt("[%s]%lu bytes remaining in recv_buf\n",
			__func__, (unsigned long)(end - conn->recv_buf_offset));
	}


	return len;
}

static int tls_send_func(void *ctx, const unsigned char *buf, size_t len)
{
	struct tls_connection *conn = (struct tls_connection *)ctx;
	
	CRYPTO_FUNC_START;

	if (conn == NULL) {
		da16x_crypto_prt("[%s]invalid ctx\n", __func__);
		return -1;
	}

	if (wpabuf_resize(&conn->send_buf, len) < 0) {
		da16x_crypto_prt("[%s]failed to resize send buf\n", __func__);
		return -1;
	}

	wpabuf_put_data(conn->send_buf, buf, len);

	return len;
}

static int tls_mbedtls_export_keys(void *p_expkey,
		const unsigned char *ms,
		const unsigned char *rb,
		size_t maclen,
		size_t keylen,
		size_t ivlen)
{
	struct tls_connection *conn = (struct tls_connection *)p_expkey;

	CRYPTO_FUNC_START;

	if (p_expkey == NULL) {
		return -1;
	}

	conn->maclen = maclen;
	conn->keylen = keylen;
	conn->ivlen = ivlen;

	da16x_crypto_prt("[%s]maclen(%ld), keylen(%ld), ivlen(%ld)\n",
		__func__, conn->maclen, conn->keylen, conn->ivlen);

	return 0;
}

static int tls_mbedtls_export_keys_for_eap_tls(void *p_expkey,
                unsigned char *ms,
                unsigned char *rb,
		unsigned char *session_ticket, size_t session_ticket_len,
                int (*tls_prf)(const unsigned char *secret, size_t slen,
                        const char *label,
                        const unsigned char *random, size_t rlen,
                        unsigned char *dstbuf, size_t dlen))
{
	struct tls_connection *conn = (struct tls_connection *)p_expkey;

	CRYPTO_FUNC_START;

	if (p_expkey == NULL) {
		return -1;
	}

	os_memcpy(conn->master, ms, TLS_MASTER_SECRET_LEN);
	os_memcpy(conn->client_random, rb, TLS_RANDOM_LEN);
	os_memcpy(conn->server_random, rb + TLS_RANDOM_LEN, TLS_RANDOM_LEN);
	conn->tls_prf = tls_prf;

	if (conn->session_ticket) {
		os_free(conn->session_ticket);
		conn->session_ticket = NULL;
		conn->session_ticket_len = 0;
	}

	if (session_ticket_len) {
		conn->session_ticket = os_malloc(session_ticket_len);
		if (!conn->session_ticket) {
			da16x_crypto_prt("[%s]failed to allocate memory for session ticket\n",
					__func__);
		} else {
			os_memcpy(conn->session_ticket,
					session_ticket, session_ticket_len);
			conn->session_ticket_len = session_ticket_len;
		}
	}

	da16x_dump("Master", conn->master, TLS_MASTER_SECRET_LEN);
	da16x_dump("Client Random", conn->client_random, TLS_RANDOM_LEN);
	da16x_dump("Server Random", conn->server_random, TLS_RANDOM_LEN);
	da16x_dump("Session Ticket", conn->session_ticket, conn->session_ticket_len);

	if (conn->session_ticket_cb) {
		conn->session_ticket_cb(conn->session_ticket_cb_ctx,
				conn->session_ticket,
				conn->session_ticket_len,
				conn->client_random,
				conn->server_random,
				conn->master);

		if (os_memcmp(conn->master, ms, TLS_MASTER_SECRET_LEN) != 0) {
			da16x_dump("Changed Master secret",
				conn->master, TLS_MASTER_SECRET_LEN);

			os_memcpy(ms, conn->master, TLS_MASTER_SECRET_LEN);
		}
	}

	return 0;
}

static int tls_mbedtls_init_session(struct tls_global *global,
		struct tls_connection *conn)

{
	const char *pers = "da16200_tls";
	int ret;
	int major = MBEDTLS_SSL_MAJOR_VERSION_3;
	int minor = MBEDTLS_SSL_MINOR_VERSION_3;

	CRYPTO_FUNC_START;

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(2);
#endif	/* MBEDTLS_DEBUG_C */

	ret = mbedtls_ctr_drbg_seed(conn->ctr_drbg_ctx,
			mbedtls_entropy_func,
			conn->entropy_ctx,
			(const unsigned char *)pers,
			os_strlen(pers));
	if (ret != 0) {
		da16x_crypto_prt("[%s]failed to set ctr drg seed(0x%x)\n",
			__func__, -ret);
		return -1;
	}

	ret = mbedtls_ssl_config_defaults(conn->ssl_conf,
			global->server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		da16x_crypto_prt("[%s]failed to set default ssl config(0x%x)\n",
			__func__, -ret);
		return -1;
	}

	mbedtls_ssl_conf_rng(conn->ssl_conf,
			mbedtls_ctr_drbg_random,
			conn->ctr_drbg_ctx);

	mbedtls_ssl_conf_dbg(conn->ssl_conf, tls_debug, NULL);

	if (da16x_tls_version == 0x301) {	//v1.0
#if defined(MBEDTLS_SSL_PROTO_TLS1)
		da16x_crypto_prt("[%s]TLS v1.0\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_1;
#else
		da16x_crypto_prt("[%s]Not support TLS v1.0. "
			"It will be running TLSv1.2\n",
			__func__);
#endif	/* MBEDTLS_SSL_PROTO_TLS1 */
	} else if (da16x_tls_version == 0x302) {	//v1.1
#if defined(MBEDTLS_SSL_PROTO_TLS1_1)
		da16x_crypto_prt("[%s]TLS v1.1\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_2;
#else
		da16x_crypto_prt("[%s]Not support TLS v1.1. "
			"It will be running TLSv1.2\n",
			__func__);
#endif	/* MBEDTLS_SSL_PROTO_TLS1_1 */
	} else { 				//(v1.2)
		da16x_crypto_prt("[%s]TLS v1.2\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_3;
	}

	mbedtls_ssl_conf_min_version(conn->ssl_conf, major, minor);

	mbedtls_ssl_conf_authmode(conn->ssl_conf, conn->auth_mode);

	if (global->server && global->params_set) {
		mbedtls_ssl_conf_ca_chain(conn->ssl_conf, global->ca_cert, NULL);


#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
		ret = mbedtls_ssl_conf_own_cert(conn->ssl_conf,
				global->cert,
				global->priv_key_alt);
#else
		ret = mbedtls_ssl_conf_own_cert(conn->ssl_conf,
				global->cert,
				global->priv_key);
#endif  /* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to set own cert(0x%x)\n",
				__func__, -ret);
			return -1;
		}
		ret = mbedtls_ssl_conf_dh_param_ctx(conn->ssl_conf, global->dhm_ctx);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to set dh param(0x%x)\n",
				__func__, -ret);
			return -1;
		}

		ret = mbedtls_ssl_setup(conn->ssl_ctx, conn->ssl_conf);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to set ssl config(0x%0x)\n",
				__func__, -ret);
			return -1;
		}

		mbedtls_ssl_set_bio(conn->ssl_ctx, NULL, tls_send_func, tls_recv_func, NULL);
	}

	return ret;
}

static int mbedtls_rsa_decrypt_func( void *ctx, int mode, size_t *olen,
		const unsigned char *input, unsigned char *output,
		size_t output_max_len )
{
	return( mbedtls_rsa_pkcs1_decrypt( (mbedtls_rsa_context *) ctx, NULL, NULL, mode, olen,
				input, output, output_max_len ) );
}

static int mbedtls_rsa_sign_func( void *ctx,
		int (*f_rng)(void *, unsigned char *, size_t), void *p_rng,
		int mode, mbedtls_md_type_t md_alg, unsigned int hashlen,
		const unsigned char *hash, unsigned char *sig )
{
	return( mbedtls_rsa_pkcs1_sign( (mbedtls_rsa_context *) ctx, f_rng, p_rng, mode,
				md_alg, hashlen, hash, sig ) );
}

static size_t mbedtls_rsa_key_len_func( void *ctx )
{
	return( ((const mbedtls_rsa_context *) ctx)->len );
}

struct tls_connection * tls_connection_init(void *tls_ctx)
{
	struct tls_global *global = tls_ctx;
	struct tls_connection *conn;
	int ret;

	CRYPTO_FUNC_START;

	da16x_secure_module_init();

	conn = os_zalloc(sizeof(*conn));
	if (conn == NULL) {

		return NULL;
	}
	conn->global = global;

	conn->ssl_ctx = os_zalloc(sizeof(mbedtls_ssl_context));
	if (conn->ssl_ctx == NULL) {
		da16x_crypto_prt("[%s]failed to allocate memory for ssl context\n",
			__func__);
		goto error;
	}

	mbedtls_ssl_init(conn->ssl_ctx);

	conn->ssl_conf = os_zalloc(sizeof(mbedtls_ssl_config));
	if (conn->ssl_conf == NULL) {
		da16x_crypto_prt("[%s]failed to allocate memory for ssl config\n",
			__func__);
		goto error;
	}

	mbedtls_ssl_config_init(conn->ssl_conf);

	conn->ctr_drbg_ctx = os_zalloc(sizeof(mbedtls_ctr_drbg_context));
	if (conn->ctr_drbg_ctx == NULL) {
		da16x_crypto_prt("[%s]failed to allocate memory for ctr drbg context\n",
			__func__);
		goto error;
	}

	mbedtls_ctr_drbg_init(conn->ctr_drbg_ctx);

	conn->entropy_ctx = os_zalloc(sizeof(mbedtls_entropy_context));
	if (conn->entropy_ctx == NULL) {
		da16x_crypto_prt("[%s]failed to allocate memory for entropy\n",
			__func__);
		goto error;
	}

	mbedtls_entropy_init(conn->entropy_ctx);

	//set default value
	int ret_num;
	if (!da16x_get_config_int(DA16X_CONF_INT_TLS_ROOTCA_CHK, &ret_num) && ret_num == 1)
		conn->auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;
        else
		conn->auth_mode = MBEDTLS_SSL_VERIFY_OPTIONAL;

	os_memset(&(conn->cipher_list[0]), 0x00, sizeof(conn->cipher_list));
	conn->cipher_count = 0;

	ret = tls_mbedtls_init_session(global, conn);
	if (ret != 0) {
		da16x_crypto_prt("[%s]failed to init tls session(%d)\n",
			__func__, ret);
		goto error;
	}

	return conn;

error:

	if (conn->ssl_ctx) {
		mbedtls_ssl_free(conn->ssl_ctx);
		os_free(conn->ssl_ctx);
	}

	if (conn->ssl_conf) {
		mbedtls_ssl_config_free(conn->ssl_conf);
		os_free(conn->ssl_conf);
	}

	if (conn->entropy_ctx) {
		mbedtls_entropy_free(conn->entropy_ctx);
		os_free(conn->entropy_ctx);
	}

	if (conn->ctr_drbg_ctx) {
		mbedtls_ctr_drbg_free(conn->ctr_drbg_ctx);
		os_free(conn->ctr_drbg_ctx);
	}

	if (conn) {
		os_free(conn);
	}

	return NULL;
}

void tls_connection_deinit(void *tls_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START;

	if (conn == NULL) {
		return ;
	}

	if (conn->ssl_ctx) {
		mbedtls_ssl_free(conn->ssl_ctx);
		os_free(conn->ssl_ctx);
	}

	if (conn->ssl_conf) {
		mbedtls_ssl_config_free(conn->ssl_conf);
		os_free(conn->ssl_conf);
	}

	if (conn->ctr_drbg_ctx) {
		mbedtls_ctr_drbg_free(conn->ctr_drbg_ctx);
		os_free(conn->ctr_drbg_ctx);
	}

	if (conn->entropy_ctx) {
		mbedtls_entropy_free(conn->entropy_ctx);
		os_free(conn->entropy_ctx);
	}

	if (conn->ca_cert) {
		mbedtls_x509_crt_free(conn->ca_cert);
		os_free(conn->ca_cert);
	}

	if (conn->cert) {
		mbedtls_x509_crt_free(conn->cert);
		os_free(conn->cert);
	}

	if (conn->priv_key) {
		mbedtls_pk_free(conn->priv_key);
		os_free(conn->priv_key);
	}

	if (conn->priv_key_alt) {
		mbedtls_pk_free(conn->priv_key_alt);
		os_free(conn->priv_key_alt);
	}

	if (conn->dhm_ctx) {
		mbedtls_dhm_free(conn->dhm_ctx);
		os_free(conn->dhm_ctx);
	}

	if (conn->session_ticket) {
		os_free(conn->session_ticket);
	}

	wpabuf_free(conn->recv_buf);
	wpabuf_free(conn->send_buf);

	os_free(conn);

	da16x_secure_module_deinit();

	return ;
}

int tls_connection_established(void *tls_ctx, struct tls_connection *conn)
{
	return conn ? conn->established : 0;
}

int tls_connection_shutdown(void *tls_ctx, struct tls_connection *conn)
{
	//struct tls_global *global = tls_ctx;
	int ret;

	CRYPTO_FUNC_START;

	if (conn == NULL) {
		return -1;
	}

	//mbedtls_ssl_close_notify(conn->ssl_ctx)

	ret = mbedtls_ssl_session_reset(conn->ssl_ctx);
	if (ret != 0) {
		da16x_crypto_prt("[%s]failed to reset tls session(0x%x)\n",
			__func__, -ret);
	}

	return ret;
}

int tls_connection_set_params(void *tls_ctx, struct tls_connection *conn,
		const struct tls_connection_params *params)
{
	int ret = 0;
	int major = MBEDTLS_SSL_MAJOR_VERSION_3;
	int minor = MBEDTLS_SSL_MINOR_VERSION_3;

	CRYPTO_FUNC_START;

	if (conn == NULL || params == NULL) {
		return -1;
	}

	if (conn->params_set) {
		da16x_crypto_prt("[%s]Already setup params\n", __func__);
		return -1;
	}

	if (params->ca_cert_blob) {
		conn->ca_cert = os_zalloc(sizeof(mbedtls_x509_crt));
		if (conn->ca_cert == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for ca cert\n",
				__func__);
			goto error;
		}

		mbedtls_x509_crt_init(conn->ca_cert);

		ret = mbedtls_x509_crt_parse(conn->ca_cert,
				params->ca_cert_blob,
				params->ca_cert_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse ca cer(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (params->client_cert_blob) {
		conn->cert = os_zalloc(sizeof(mbedtls_x509_crt));
		if (conn->cert == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for cert\n",
				__func__);
			goto error;
		}

		mbedtls_x509_crt_init(conn->cert);

		ret = mbedtls_x509_crt_parse(conn->cert,
				params->client_cert_blob,
				params->client_cert_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse cert(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (params->private_key_blob) {
		conn->priv_key = os_zalloc(sizeof(mbedtls_pk_context));
		if (conn->priv_key == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for private key\n",
				__func__);
			goto error;
		}

		mbedtls_pk_init(conn->priv_key);

		ret = mbedtls_pk_parse_key(conn->priv_key,
				(const unsigned char *)params->private_key_blob,
				params->private_key_blob_len + 1,
				(const unsigned char *)params->private_key_passwd,
				sizeof(params->private_key_passwd));
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse private key(0x%x)\n",
				__func__, -ret);
			goto error;
		}

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
		if(mbedtls_pk_get_type(conn->priv_key) == MBEDTLS_PK_RSA) {
			conn->priv_key_alt = os_zalloc(sizeof(mbedtls_pk_context));
			if (conn->priv_key_alt == NULL) {
				da16x_crypto_prt("[%s]failed to allocate memory for private key alt\n",
					__func__);
				goto error;
			}

			mbedtls_pk_init(conn->priv_key_alt);

			ret = mbedtls_pk_setup_rsa_alt(conn->priv_key_alt,
					(void *)mbedtls_pk_rsa(*conn->priv_key),
					mbedtls_rsa_decrypt_func,
					mbedtls_rsa_sign_func,
					mbedtls_rsa_key_len_func);
			if( ret != 0 ){
				da16x_crypto_prt("[%s]failed to set rsa alt(0x%x)\n",
					__func__, -ret);
				goto error;
			}
		}
#endif	/* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */
	}

	if (params->dh_blob) {
		conn->dhm_ctx = os_zalloc(sizeof(mbedtls_dhm_context));
		if (conn->dhm_ctx == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for dhm\n",
				__func__);
			goto error;
		}

		mbedtls_dhm_init(conn->dhm_ctx);

		ret = mbedtls_dhm_parse_dhm(conn->dhm_ctx,
				params->dh_blob,
				params->dh_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse dh params(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (conn->ca_cert) {
		mbedtls_ssl_conf_ca_chain(conn->ssl_conf, conn->ca_cert, NULL);
	}

	if (conn->cert && conn->priv_key) {
#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
		ret = mbedtls_ssl_conf_own_cert(conn->ssl_conf,
				conn->cert,
				conn->priv_key_alt);
#else
		ret = mbedtls_ssl_conf_own_cert(conn->ssl_conf,
				conn->cert,
				conn->priv_key);
#endif	/* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to set cert(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (conn->dhm_ctx) {
		ret = mbedtls_ssl_conf_dh_param_ctx(conn->ssl_conf, conn->dhm_ctx);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to set dh params(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

skip_cert:
	mbedtls_ssl_conf_export_keys_cb(conn->ssl_conf,
			tls_mbedtls_export_keys, conn);

	mbedtls_ssl_conf_export_keys_cb_for_eap_tls(conn->ssl_conf,
			tls_mbedtls_export_keys_for_eap_tls, conn);

	if (da16x_tls_version == 0x301) {	//v1.0
#if defined(MBEDTLS_SSL_PROTO_TLS1)
		da16x_crypto_prt("[%s]TLS v1.0\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_1;
#else
		da16x_crypto_prt("[%s]Not support TLS v1.0. "
			"It will be running TLSv1.2\n",
			__func__);
#endif	/* MBEDTLS_SSL_PROTO_TLS1 */
	} else if (da16x_tls_version == 0x302) {	//v1.1
#if defined(MBEDTLS_SSL_PROTO_TLS1_1)
		da16x_crypto_prt("[%s]TLS v1.1\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_2;
#else
		da16x_crypto_prt("[%s]Not support TLS v1.1. "
			"It will be running TLSv1.2\n",
			__func__);
#endif	/* MBEDTLS_SSL_PROTO_TLS1_1 */
	} else { 				//(v1.2)
		da16x_crypto_prt("[%s]TLS v1.2\n", __func__);
		major = MBEDTLS_SSL_MAJOR_VERSION_3;
		minor = MBEDTLS_SSL_MINOR_VERSION_3;
	}

	mbedtls_ssl_conf_min_version(conn->ssl_conf, major, minor);

	ret = mbedtls_ssl_setup(conn->ssl_ctx, conn->ssl_conf);
	if (ret != 0) {
		da16x_crypto_prt("[%s]failed to set ssl conf(0x%x)\n",
			__func__, -ret);
		goto error;
	}

	mbedtls_ssl_set_bio(conn->ssl_ctx, conn, tls_send_func, tls_recv_func, NULL);

	conn->params_set = 1;

	return 0;

error:

	if (conn->ca_cert) {
		mbedtls_x509_crt_free(conn->ca_cert);
		os_free(conn->ca_cert);
		conn->ca_cert = NULL;
	}

	if (conn->cert) {
		mbedtls_x509_crt_free(conn->cert);
		os_free(conn->cert);
		conn->cert = NULL;
	}

	if (conn->priv_key_alt) {
		mbedtls_pk_free(conn->priv_key_alt);
		os_free(conn->priv_key_alt);
		conn->priv_key_alt = NULL;
	}

	if (conn->priv_key) {
		mbedtls_pk_free(conn->priv_key);
		os_free(conn->priv_key);
		conn->priv_key = NULL;
	}

	if (conn->dhm_ctx) {
		mbedtls_dhm_free(conn->dhm_ctx);
		os_free(conn->dhm_ctx);
		conn->dhm_ctx = NULL;
	}


	return ret;
}

int tls_global_set_params(void *tls_ctx, const struct tls_connection_params *params)
{
	struct tls_global *global = tls_ctx;
	int ret = 0;

	CRYPTO_FUNC_START;

	global->server = 1;

	if (params->ca_cert_blob) {
		global->ca_cert = os_zalloc(sizeof(mbedtls_x509_crt));
		if (global->ca_cert == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for ca cert\n",
				__func__);
			goto error;
		}

		mbedtls_x509_crt_init(global->ca_cert);

		ret = mbedtls_x509_crt_parse(global->ca_cert,
				params->ca_cert_blob,
				params->ca_cert_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse ca cert(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (params->client_cert_blob) {
		global->cert = os_zalloc(sizeof(mbedtls_x509_crt));
		if (global->cert == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for cert\n",
				__func__);
			goto error;
		}

		mbedtls_x509_crt_init(global->cert);

		ret = mbedtls_x509_crt_parse(global->ca_cert,
				params->client_cert_blob,
				params->client_cert_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse cert(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	if (params->private_key_blob) {
		global->priv_key = os_zalloc(sizeof(mbedtls_pk_context));
		if (global->priv_key == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for private key\n",
				__func__);
			goto error;
		}

		mbedtls_pk_init(global->priv_key);

		ret = mbedtls_pk_parse_key(global->priv_key,
				(const unsigned char *)params->private_key_blob,
				params->private_key_blob_len + 1,
				(const unsigned char *)params->private_key_passwd,
				os_strlen(params->private_key_passwd));
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse private key(0x%x)\n",
				__func__,- ret);
			goto error;
		}

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
		if(mbedtls_pk_get_type(global->priv_key) == MBEDTLS_PK_RSA) {
			global->priv_key_alt = os_zalloc(sizeof(mbedtls_pk_context));
			if (global->priv_key_alt == NULL) {
				da16x_crypto_prt("[%s]failed to allocate memory for private key alt\n",
					__func__);
				goto error;
			}

			mbedtls_pk_init(global->priv_key_alt);

			ret = mbedtls_pk_setup_rsa_alt(global->priv_key_alt,
					(void *)mbedtls_pk_rsa(*global->priv_key),
					mbedtls_rsa_decrypt_func,
					mbedtls_rsa_sign_func,
					mbedtls_rsa_key_len_func);
			if( ret != 0 ){
				da16x_crypto_prt("[%s]failed to set rsa alt(0x%x)\n",
					__func__, -ret);
				goto error;
			}
		}
#endif	/* defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT) */
	}

	if (params->dh_blob) {
		global->dhm_ctx = os_zalloc(sizeof(mbedtls_dhm_context));
		if (global->dhm_ctx == NULL) {
			da16x_crypto_prt("[%s]failed to allocate memory for dhm\n",
				__func__);
			goto error;
		}

		mbedtls_dhm_init(global->dhm_ctx);

		ret = mbedtls_dhm_parse_dhm(global->dhm_ctx,
				params->dh_blob,
				params->dh_blob_len + 1);
		if (ret != 0) {
			da16x_crypto_prt("[%s]failed to parse dh params(0x%x)\n",
				__func__, -ret);
			goto error;
		}
	}

	global->params_set = 1;

	return 0;

error:

	if (global->ca_cert) {
		mbedtls_x509_crt_free(global->ca_cert);
		os_free(global->ca_cert);
		global->ca_cert = NULL;
	}

	if (global->cert) {
		mbedtls_x509_crt_free(global->cert);
		os_free(global->cert);
		global->cert = NULL;
	}

	if (global->priv_key) {
		mbedtls_pk_free(global->priv_key);
		os_free(global->priv_key);
		global->priv_key = NULL;
	}

	if (global->priv_key_alt) {
		mbedtls_pk_free(global->priv_key_alt);
		os_free(global->priv_key_alt);
		global->priv_key_alt = NULL;
	}

	if (global->dhm_ctx) {
		mbedtls_dhm_free(global->dhm_ctx);
		os_free(global->dhm_ctx);
		global->dhm_ctx = NULL;
	}

	return ret;
}

int tls_global_set_verify(void *tls_ctx, int check_crl)
{
	CRYPTO_FUNC_START;
	return -1;
}

int tls_connection_set_verify(void *tls_ctx, struct tls_connection *conn,
		int verify_peer,
		unsigned int flags,
		const u8 *session_ctx,
		size_t session_ctx_len)
{
	CRYPTO_FUNC_START;

	if (conn == NULL || conn->ssl_ctx == NULL) {
		return -1;
	}

	if (verify_peer) {
		conn->auth_mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
	} else {
		conn->auth_mode = MBEDTLS_SSL_VERIFY_NONE;
	}

	mbedtls_ssl_conf_authmode(conn->ssl_conf, conn->auth_mode);

#if 0 /* TODO: this function is called only for eap server. */
	conn->session_ticket = session_ctx;
	conn->session_ticket_len = session_ctx_len;
#endif

	return 0;
}

int tls_connection_get_random(void *tls_ctx, struct tls_connection *conn,
		struct tls_random *data)
{
	CRYPTO_FUNC_START;

	if ((tls_ctx == NULL) || (conn == NULL) || (data == NULL)) {
		return -1;
	}

	data->client_random = conn->client_random;
	data->client_random_len = TLS_RANDOM_LEN;
	data->server_random = conn->server_random;
	data->server_random_len = TLS_RANDOM_LEN;

	return 0;
}

static int tls_get_keyblock_size(struct tls_connection *conn)
{
	CRYPTO_FUNC_START;

	if (conn == NULL) {
		return -1;
	}

	da16x_crypto_prt("[%s]maclen(%ld), keylen(%ld), ivlen(%ld)\n",
			__func__, conn->maclen, conn->keylen, conn->ivlen);

	return 2 * (conn->maclen + conn->keylen + conn->ivlen);
}


static int tls_connection_prf(void *tls_ctx, struct tls_connection *conn,
		const char *label, int server_random_first,
		int skip_keyblock, u8 *out, size_t out_len)
{
	int ret = -1, skip = 0;
	u8 *tmp_out = NULL;
	u8 *_out = out;
	unsigned char random[TLS_RANDOM_LEN * 2] = {0x00,};

	CRYPTO_FUNC_START;

	if ((conn == NULL) || (conn->ssl_ctx == NULL) || (conn->tls_prf == NULL)) {
		return -1;
	}

	if (skip_keyblock) {
		skip = tls_get_keyblock_size(conn);
		if (skip < 0)
			return -1;
		tmp_out = os_malloc(skip + out_len);
		if (!tmp_out)
			return -1;
		_out = tmp_out;
	}

	wpa_hexdump(MSG_MSGDUMP, "Master", conn->master, TLS_MASTER_SECRET_LEN);
	wpa_hexdump(MSG_MSGDUMP, "Client Random", conn->client_random, TLS_RANDOM_LEN);
	wpa_hexdump(MSG_MSGDUMP, "Server Random", conn->server_random, TLS_RANDOM_LEN);

	if (server_random_first) {
		os_memcpy(random, conn->server_random, TLS_RANDOM_LEN);
		os_memcpy(random + TLS_RANDOM_LEN, conn->client_random, TLS_RANDOM_LEN);
	} else {
		os_memcpy(random, conn->client_random, TLS_RANDOM_LEN);
		os_memcpy(random + TLS_RANDOM_LEN, conn->server_random, TLS_RANDOM_LEN);
	}

#if 0 // It maybe required to get WiFi certificate for EAP-FAST.
	if (server_random_first) {
		da16x_crypto_prt("[%s]Workaround for wpa supplicant 2.4 and openssl\n",
				__func__);

		ret = tls_prf_sha1_md5(conn->master, TLS_MASTER_SECRET_LEN, label,
				random, (TLS_RANDOM_LEN * 2), _out, skip + out_len);
		if (ret) {
			da16x_crypto_prt("[%s]failed to calculate tls prf(0x%x)\n",
					__func__, -ret);
		}
	} else {
		ret = conn->tls_prf(conn->master, TLS_MASTER_SECRET_LEN, label,
				random, (TLS_RANDOM_LEN * 2), _out, skip + out_len);
		if (ret) {
			da16x_crypto_prt("[%s]failed to calculate tls prf(0x%x)\n",
					__func__,  -ret);
		}
	}
#else
	ret = conn->tls_prf(conn->master, TLS_MASTER_SECRET_LEN, label,
				random, (TLS_RANDOM_LEN * 2), _out, skip + out_len);
	if (ret) {
		da16x_crypto_prt("[%s]failed to calculate tls prf(0x%x)\n",
			__func__,  -ret);
	}
#endif

	if (ret == 0 && skip_keyblock) {
		os_memcpy(out, _out + skip, out_len);
	}

#ifdef	CONFIG_SUPP27_BIN_CLR_FREE
	bin_clear_free(tmp_out, skip);
#else
	os_free(tmp_out);
#endif	/* CONFIG_SUPP27_BIN_CLR_FREE */

	return ret;

}

int tls_connection_export_key(void *tls_ctx, struct tls_connection *conn, const char *label, u8 *out, size_t out_len)
{
	return tls_connection_prf(tls_ctx, conn, label, 0, 0, out, out_len);
}

int tls_connection_get_eap_fast_key(void *tls_ctx, struct tls_connection *conn, u8 *out, size_t out_len)
{
	return tls_connection_prf(tls_ctx, conn, "key expansion", 1, 1, out, out_len);
}

static struct wpabuf *tls_mbedtls_get_appl_data(struct tls_connection *conn)
{
	int res;
	struct wpabuf *ad;

	CRYPTO_FUNC_START;
	ad = wpabuf_alloc((wpabuf_len(conn->recv_buf) + 500) * 3);
	if (ad == NULL) {
		return NULL;
	}

	res = mbedtls_ssl_read(conn->ssl_ctx, wpabuf_mhead_u8(ad), wpabuf_size(ad));
	if (res < 0) {
		da16x_crypto_prt("[%s]failed to read data(0x%x)\n",
			__func__, -res);
		wpabuf_free(ad);
		return NULL;
	}

	wpabuf_put(ad, res);

	da16x_crypto_prt("[%s]Received %d bytes of Application data\n",
		__func__, res);

	return ad;
}

static int tls_mbedtls_verify_certificate(struct tls_connection *conn)
{
	int ret = 0;
	int verify_result = 0;

	CRYPTO_FUNC_START;

	if (conn == NULL || conn->ssl_ctx == NULL) {
		return -1;
	}

	//to check result of verification of the certificate
	verify_result = mbedtls_ssl_get_verify_result(conn->ssl_ctx);

	// If the time is not set, the certificate start validity time is not checked.
	if (!chk_time_set_status())
	{
		if (tls_mbedtls_cert_verify_flags & MBEDTLS_X509_BADCERT_FUTURE)
		{
			tls_mbedtls_cert_verify_flags &= ~(MBEDTLS_X509_BADCERT_FUTURE);
		}
	}
	else if (!(tls_mbedtls_cert_verify_flags & MBEDTLS_X509_BADCERT_FUTURE))
	{
		tls_mbedtls_cert_verify_flags |= MBEDTLS_X509_BADCERT_FUTURE;

	}

	if (verify_result & tls_mbedtls_cert_verify_flags) {
		da16x_crypto_prt("[%s]failed to verify cert(0x%x)\n",
			__func__, verify_result);
		return -1;
	} 

	return ret;
}

struct wpabuf * tls_connection_handshake(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data,
		struct wpabuf **appl_data)
{
	CRYPTO_FUNC_START;

	return tls_connection_handshake2(tls_ctx, conn, in_data, appl_data, NULL);
}

struct wpabuf * tls_connection_handshake2(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data,
		struct wpabuf **appl_data,
		int *need_more_data)
{
	//struct tls_global *global = tls_ctx;
	struct wpabuf *out_data = NULL;
	int ret;

	CRYPTO_FUNC_START;

	if (appl_data) {
		*appl_data = NULL;
	}

	if (in_data && wpabuf_len(in_data) > 0) {
		if (conn->recv_buf) {
			da16x_crypto_prt("[%s]%lu bytes remaining in recv_buf\n",
				__func__, (unsigned long)wpabuf_len(conn->recv_buf));
			wpabuf_free(conn->recv_buf);
		}

		conn->recv_buf = wpabuf_dup(in_data);
		if (conn->recv_buf == NULL) {
			da16x_crypto_prt("[%s]failed to duplicate recv buff\n",
				__func__);
			return NULL;
		}

		conn->recv_buf_offset = wpabuf_head(conn->recv_buf);
	}

	while ((ret = mbedtls_ssl_handshake(conn->ssl_ctx)) != 0) {
		if ((ret != MBEDTLS_ERR_SSL_WANT_READ)
			&& (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
			da16x_crypto_prt("[%s]failed to proceed tls handshake(0x%x)\n",
				__func__, -ret);
		}

		break;
	}

	ret = tls_mbedtls_verify_certificate(conn);
	if (ret) {
		mbedtls_ssl_close_notify(conn->ssl_ctx);
	} else {
		//TLS: Handshake completed successfully
		if (conn->ssl_ctx->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
			da16x_crypto_prt("[%s]Handshake completed successfully\n", __func__);

#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
			if ((conn->ssl_ctx->major_ver == MBEDTLS_SSL_MAJOR_VERSION_3)
				&& (conn->ssl_ctx->minor_ver <= MBEDTLS_SSL_MINOR_VERSION_1)) {
				mbedtls_ssl_conf_cbc_record_splitting(conn->ssl_conf,
						MBEDTLS_SSL_CBC_RECORD_SPLITTING_DISABLED);
			}
#endif
			conn->established = 1;

			if (conn->send_buf == NULL) {
				/* Need to return somthing to get final TLS ACK. */
				conn->send_buf = wpabuf_alloc(0);
			}

			if (conn->recv_buf && appl_data) {
				da16x_crypto_prt("[%s]conn->recv_buf & appl_data is not null\n",
					__func__);
				*appl_data = tls_mbedtls_get_appl_data(conn);
			}
		}
	}

	out_data = conn->send_buf;
	conn->send_buf = NULL;

	return out_data;
}

struct wpabuf * tls_connection_server_handshake(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data,
		struct wpabuf **appl_data)
{
	CRYPTO_FUNC_START;

	return tls_connection_handshake(tls_ctx, conn, in_data, appl_data);
}

struct wpabuf * tls_connection_encrypt(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data)
{
	int ret;
	struct wpabuf *buf;

	CRYPTO_FUNC_START;

	ret = mbedtls_ssl_write(conn->ssl_ctx, wpabuf_head(in_data), wpabuf_len(in_data));
	if (ret < 0) {
		da16x_crypto_prt("[%s]failed to encrypt data(0x%x)\n",
			__func__, -ret);
		return NULL;
	}

	buf = conn->send_buf;
	conn->send_buf = NULL;
	return buf;
}

struct wpabuf *tls_connection_decrypt(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data)
{
	CRYPTO_FUNC_START;

	return tls_connection_decrypt2(tls_ctx, conn, in_data, NULL);
}

struct wpabuf *tls_connection_decrypt2(void *tls_ctx,
		struct tls_connection *conn,
		const struct wpabuf *in_data,
		int *need_more_data)
{
	int ret;
	struct wpabuf *out;

	CRYPTO_FUNC_START;

	if (conn->recv_buf) {
		da16x_crypto_prt("[%s]%lu bytes remaining in recv_buf\n",
			__func__, (unsigned long)wpabuf_len(conn->recv_buf));
		wpabuf_free(conn->recv_buf);
	}
	conn->recv_buf = wpabuf_dup(in_data);
	if (conn->recv_buf == NULL) {
		da16x_crypto_prt("[%s]failed to duplicate recv buf\n", __func__);
		return NULL;
	}
	conn->recv_buf_offset = wpabuf_head(conn->recv_buf);

	/*
	 * Even though we try to disable TLS compression, it is possible that
	 * this cannot be done with all TLS libraries. Add extra buffer space
	 * to handle the possibility of the decrypted data being longer than
	 * input data.
	 */
	out = wpabuf_alloc((wpabuf_len(in_data) + 500) * 3);
	if (out == NULL) {
		da16x_crypto_prt("[%s]failed to allocate memory(%ld)\n",
			__func__, (wpabuf_len(in_data) + 500) * 3);
		return NULL;
	}

	ret = mbedtls_ssl_read(conn->ssl_ctx,
			wpabuf_mhead(out),
			wpabuf_size(out));
	if (ret <= 0) {
		da16x_crypto_prt("[%s]failed to decrypt data(0x%x)\n",
			__func__, -ret);
		wpabuf_free(out);
		return NULL;
	}

	wpabuf_put(out, ret);

	return out;
}

int tls_connection_resumed(void *tls_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START

	if (conn && conn->ssl_ctx->handshake) {
		return conn->ssl_ctx->handshake->resume;
	}

	return 0;
}

int tls_connection_set_cipher_list(void *tls_ctx, struct tls_connection *conn,
		u8 *ciphers)
{
	CRYPTO_FUNC_START;

	u8 *cipher = ciphers;
	int list[TLS_MAX_CIPHER_LISTS] = {0x00,};
	int count = 0;

	CRYPTO_FUNC_START;

	if (conn == NULL || conn->ssl_ctx == NULL || ciphers == NULL) {
		return -1;
	}

	while (*cipher != TLS_CIPHER_NONE) {
		switch (*cipher) {
		case TLS_CIPHER_RC4_SHA:
			list[count++] = MBEDTLS_TLS_RSA_WITH_RC4_128_SHA;
			break;
		case TLS_CIPHER_AES128_SHA:
			list[count++] = MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA;
			break;
		case TLS_CIPHER_RSA_DHE_AES128_SHA:
			list[count++] = MBEDTLS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA;
			break;
		case TLS_CIPHER_RSA_DHE_AES256_SHA:
			list[count++] = MBEDTLS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA;
			break;
		case TLS_CIPHER_AES256_SHA:
			list[count++] = MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA;
			break;
		case TLS_CIPHER_ANON_DH_AES128_SHA:
		default:
			da16x_crypto_prt("[%s]TLS: Unsupported cipher selection: %d\n",
				__func__, *cipher);
			return -1;
		}

		cipher++;
	}

	list[count] = 0x00;

	os_memcpy(conn->cipher_list, list, sizeof(list));
	conn->cipher_count = count;

	mbedtls_ssl_conf_ciphersuites(conn->ssl_conf, conn->cipher_list);

	return 0;
}

int tls_get_version(void *tls_ctx, struct tls_connection *conn,
		char *buf, size_t buflen)
{
	if (conn == NULL || conn->ssl_ctx == NULL) {
		return -1;
	}

	if (conn->ssl_ctx->major_ver != MBEDTLS_SSL_MAJOR_VERSION_3) {
		return -1;
	}

	if (conn->ssl_ctx->minor_ver == MBEDTLS_SSL_MINOR_VERSION_0) {
		snprintf(buf, buflen, "SSLv3.0");
	} else if (conn->ssl_ctx->minor_ver == MBEDTLS_SSL_MINOR_VERSION_1) {
		snprintf(buf, buflen, "TLSv1.0");
	} else if (conn->ssl_ctx->minor_ver == MBEDTLS_SSL_MINOR_VERSION_2) {
		snprintf(buf, buflen, "TLSv1.1");
	} else if (conn->ssl_ctx->minor_ver == MBEDTLS_SSL_MINOR_VERSION_3) {
		snprintf(buf, buflen, "TLSv1.2");
	} else {
		return -1;
	}

	da16x_crypto_prt("[%s]TLS Version: %s\n", __func__, buf);

	return 0;
}

int tls_get_cipher(void *tls_ctx, struct tls_connection *conn,
		char *buf, size_t buflen)
{
	const char *name;

	CRYPTO_FUNC_START;

	if (tls_ctx == NULL || conn == NULL) {
		return -1;
	}

	name = mbedtls_ssl_get_ciphersuite(conn->ssl_ctx);
	if (!name) {
		return -1;
	}

	if (os_strlcpy(buf, name, buflen) >= buflen) {
		return -1;
	}

	da16x_crypto_prt("[%s]cipher suite:%s\n", __func__, name);

	return 0;
}

int tls_connection_enable_workaround(void *tls_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
	return -1;
}

int tls_connection_client_hello_ext(void *tls_ctx, struct tls_connection *conn,
		int ext_type, const u8 *data, size_t data_len)
{
	int ret = 0;

	CRYPTO_FUNC_START;

	if (conn == NULL || conn->ssl_ctx == NULL) {
		da16x_crypto_prt("[%s]Not initialized tls session\n", __func__);
		return -1;
	}

	if (ext_type != MBEDTLS_TLS_EXT_SESSION_TICKET) {
		da16x_crypto_prt("[%s]Not supported clienthello ext(%d)\n",
			__func__, ext_type);
		return -1;
	}

	ret = mbedtls_ssl_set_pac_info(conn->ssl_ctx, data, data_len);
	if (ret) {
		da16x_crypto_prt("[%s]Failed to set clienthello ext(0x%x)\n",
			__func__, -ret);
		return -1;
	}

	return 0;
}

int tls_connection_get_failed(void *tls_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
	return 0;
}

int tls_connection_get_read_alerts(void *ssl_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
	return 0;
}

int tls_connection_get_write_alerts(void *ssl_ctx, struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
	return 0;
}

int tls_connection_set_session_ticket_cb(void *tls_ctx,
                                struct tls_connection *conn,
                                tls_session_ticket_cb cb, void *ctx)
{
	CRYPTO_FUNC_START;

	if (conn == NULL || conn->ssl_ctx == NULL) {
		return -1;
}

	conn->session_ticket_cb = cb;
	conn->session_ticket_cb_ctx = ctx;

	return 0;
}

int tls_get_library_version(char *buf, size_t buf_len)
{
	CRYPTO_FUNC_START;
	return -1;
}

void tls_connection_set_success_data(struct tls_connection *conn,
		struct wpabuf *data)
{
	CRYPTO_FUNC_START;
}

void tls_connection_set_success_data_resumed(struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
}

	const struct wpabuf *
tls_connection_get_success_data(struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
	return NULL;
}

void tls_connection_remove_session(struct tls_connection *conn)
{
	CRYPTO_FUNC_START;
}

