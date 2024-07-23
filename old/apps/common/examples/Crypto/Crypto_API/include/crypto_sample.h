/**
 ****************************************************************************************
 *
 * @file crypto_sample.h
 *
 * @brief Header of crypto samples.
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

#ifndef __CRYPTO_SAMPLE_H__
#define	__CRYPTO_SAMPLE_H__

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/error.h"

#pragma GCC diagnostic ignored "-Wunused-function"

// Definition of Crypto samples

// AES Algorithms
#define	__CRYPTO_SAMPLE_AES__

// Cipher API
#undef	__CRYPTO_SAMPLE_CIPHER__

// DES Algorithms
#undef	__CRYPTO_SAMPLE_DES__

// Diffie–Hellman key exchange
#undef	__CRYPTO_SAMPLE_DHM__

// DRBG
#undef	__CRYPTO_SAMPLE_DRBG__

// ECDH
#undef	__CRYPTO_SAMPLE_ECDH__

// ECDSA
#undef	__CRYPTO_SAMPLE_ECDSA__

// Hash & HMAC Algorithms
#undef	__CRYPTO_SAMPLE_HASH__

// Key Derivation Function
#undef	__CRYPTO_SAMPLE_KDF__

// Public Key abstraction layer.
#undef	__CRYPTO_SAMPLE_PK__

// RSA PKCS#1
#undef	__CRYPTO_SAMPLE_RSA__

// Print Hex dump for Crypto samples
#undef	CRYPTO_SAMPLE_HEX_DUMP

// Main function of Crypto samples
void crypto_sample_aes(void *param);

void crypto_sample_cipher(void *param);

void crypto_sample_des(void *param);

void crypto_sample_dhm(void *param);

void crypto_sample_drbg(void *param);

void crypto_sample_ecdh(void *param);

void crypto_sample_ecdsa(void *param);

void crypto_sample_hash(void *param);

void crypto_sample_kdf(void *param);

void crypto_sample_pk(void *param);

void crypto_sample_rsa(void *param);

#ifdef CRYPTO_SAMPLE_HEX_DUMP
    extern void hex_dump(UCHAR *data, UINT length);
#endif	//CRYPTO_SAMPLE_HEX_DUMP

static void *crypto_sample_calloc(size_t n, size_t size)
{
    void *buf = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf) {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

static void crypto_sample_free(void *f)
{
    if (f == NULL) {
        return;
    }

    vPortFree(f);
}

static void crypto_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (CRYPTO_SAMPLE_HEX_DUMP)
    if (len) {
        PRINTF("%s(%ld)\r\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (CRYPTO_SAMPLE_HEX_DUMP)

    return ;
}

#endif	/* __CRYPTO_SAMPLE_H__ */

/* EOF */
