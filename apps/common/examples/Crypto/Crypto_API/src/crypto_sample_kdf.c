/**
 ****************************************************************************************
 *
 * @file crypto_sample_kdf.c
 *
 * @brief Sample app of KDF(Key Derivation Function).
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

#include "crypto_sample.h"
#include "mbedtls/sha1.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"

static const unsigned char pkcs5_password[32] = "password";

static const size_t pkcs5_plen = 8;

static const unsigned char pkcs5_salt[40] = "salt";

static const size_t pkcs5_slen = 4;

static const uint32_t pkcs5_it_cnt = 1;

static const uint32_t pkcs5_key_len = 20;

static const unsigned char pkcs5_result_key[32] = {
    0x0c, 0x60, 0xc8, 0x0f, 0x96, 0x1f, 0x0e, 0x71,
    0xf3, 0xa9, 0xb5, 0x24, 0xaf, 0x60, 0x12, 0x06,
    0x2f, 0xe0, 0x37, 0xa6
};

int crypto_sample_pkcs5()
{
    mbedtls_md_context_t sha1_ctx;
    const mbedtls_md_info_t *info_sha1;
    unsigned char *key = NULL;
    int ret = -1;

    key = crypto_sample_calloc(64, sizeof(unsigned char));
    if (!key) {
        PRINTF("[%s] Failed to allocate key buffer\n", __func__);
        goto cleanup;
    }

    // Initialize a SHA-1 context.
    mbedtls_md_init(&sha1_ctx);

    // Get the message-digest information associated with the given digest type.
    info_sha1 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (info_sha1 == NULL) {
        PRINTF("[%s] Unknown Hash Type\n", __func__);
        goto cleanup;
    }

    // Select the message digest algorithm to use, and allocates internal structures.
    ret = mbedtls_md_setup(&sha1_ctx, info_sha1, 1);
    if (ret) {
        PRINTF("[%s] Failed to setup the message digest algorithm(0x%0x)\n",
               __func__, -ret);
        goto cleanup;
    }

    PRINTF("* PBKDF2 (SHA1): ");

    // Derive a key from a password using PBKDF2 function with HMAC
    ret = mbedtls_pkcs5_pbkdf2_hmac(&sha1_ctx,
                                    pkcs5_password, pkcs5_plen,
                                    pkcs5_salt, pkcs5_slen,
                                    pkcs5_it_cnt,
                                    pkcs5_key_len, key);
    if (ret) {
        PRINTF("failed - [%s] Failed to verify key(0x%x)\n", __func__, -ret);
        goto cleanup;
    }

    // Check result.
    if ((ret = memcmp(pkcs5_result_key, key, pkcs5_key_len)) != 0) {
        PRINTF("failed - [%s] Not matched result", __func__);
        goto cleanup;
    }

    PRINTF("passed\n");

cleanup:

    /* Clear the internal structure of ctx and frees any embedded internal structure,
     * but does not free ctx itself.
     */
    mbedtls_md_free(&sha1_ctx);

    if (key) {
        crypto_sample_free(key);
    }

    return ret;
}

/*
 * Sample Functions for Key Derivation Function(KDF)
 */
void crypto_sample_kdf(void *param)
{
    DA16X_UNUSED_ARG(param);

    crypto_sample_pkcs5();

    return ;
}

/* EOF */
