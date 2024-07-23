/**
 ****************************************************************************************
 *
 * @file da16x_dpm_filter.h
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

#ifndef	__DA16X_DPM_TLS_H__
#define	__DA16X_DPM_TLS_H__

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/x509.h"


#define	DA16X_DPM_TLS_NOT_SAVE_PERR_CERT

#define	DA16X_DPM_TLS_DEF_TIMEOUT	100

/**
 * set_tls_session - Save TLS session for DPM
 * @name: Name of rtm memory.
 * @ssl_ctx: Pointer of ssl context. TLS session has to be establihsed.
 * Returns: 0 on success
 */
int set_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx);

/**
 * get_tls_session - Recorver TLS session for DPM
 * @name: Name of rtm memory.
 * @ssl_ctx: Pointer of ssl context. ssl_ctx has to be initialized.
 * Returns: 0 on success
 */
int get_tls_session(const char *name, mbedtls_ssl_context *ssl_ctx);

/**
 * clr_tls_session - Release saved TLS session for DPM
 * @name: Name of rtm memory.
 * Returns: 0 on success, -1 on failure
 */
int clr_tls_session(const char *name);

#endif	/* __DA16X_DPM_TLS_H__ */

/* EOF */
