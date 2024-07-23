/**
 ****************************************************************************************
 *
 * @file crypto_sample_des.c
 *
 * @brief Sample app of DES algorithms.
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
#include "mbedtls/des.h"

#define	CRYPTO_SAMPLE_DES_LOOP_COUNT	10000

/*
 * DES and 3DES test vectors from:
 *
 * http://csrc.nist.gov/groups/STM/cavp/documents/des/tripledes-vectors.zip
 */
static const unsigned char crypto_sample_des3_keys[24] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01,
    0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23
};

static const unsigned char crypto_sample_des3_buf[8] = {
    0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74
};

static const unsigned char crypto_sample_des3_iv[8] = {
    0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF,
};

#if defined(MBEDTLS_CIPHER_MODE_CBC)
static const unsigned char crypto_sample_des3_cbc_dec[3][8] = {
    {
        0x12, 0x9F, 0x40, 0xB9, 0xD2, 0x00, 0x56, 0xB3
    },
    {
        0x47, 0x0E, 0xFC, 0x9A, 0x6B, 0x8E, 0xE3, 0x93
    },
    {
        0xC5, 0xCE, 0xCF, 0x63, 0xEC, 0xEC, 0x51, 0x4C
    }
};

static const unsigned char crypto_sample_des3_cbc_enc[3][8] = {
    {
        0x54, 0xF1, 0x5A, 0xF6, 0xEB, 0xE3, 0xA4, 0xB4
    },
    {
        0x35, 0x76, 0x11, 0x56, 0x5F, 0xA1, 0x8E, 0x4D
    },
    {
        0xCB, 0x19, 0x1F, 0x85, 0xD1, 0xED, 0x84, 0x39
    }
};

int crypto_sample_des_cbc()
{
    int i, j, u, v, ret = -1;
    mbedtls_des_context *ctx = NULL;
    mbedtls_des3_context *ctx3 = NULL;
    unsigned char buf[8] = {0x00,};
    unsigned char prv[8] = {0x00,};
    unsigned char iv[8] = {0x00,};

    ctx = (mbedtls_des_context *)crypto_sample_calloc(1, sizeof(mbedtls_des_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate DES Context\r\n", __func__);
        return ret;
    }

    ctx3 = (mbedtls_des3_context *)crypto_sample_calloc(1, sizeof(mbedtls_des3_context));
    if (!ctx3) {
        PRINTF("[%s] Failed to allocate Triple-DES Context\r\n", __func__);
        goto cleanup;
    }

    // Initialize the DES context.
    mbedtls_des_init(ctx);

    // Initialize the Triple-DES context.
    mbedtls_des3_init(ctx3);

    // Test CBC
    for (i = 0; i < 6; i++) {
        u = i >> 1;
        v = i  & 1;

        PRINTF("* DES%c-CBC-%3d (%s): ",
               ( u == 0 ) ? ' ' : '3', 56 + u * 56,
               ( v == MBEDTLS_DES_DECRYPT ) ? "dec" : "enc" );

        memcpy(iv,  crypto_sample_des3_iv,  8);
        memcpy(prv, crypto_sample_des3_iv,  8);
        memcpy(buf, crypto_sample_des3_buf, 8);

        switch (i) {
        case 0: {
            // DES key schedule (56-bit, decryption).
            mbedtls_des_setkey_dec(ctx, crypto_sample_des3_keys);
        }
        break;
        case 1: {
            // DES key schedule (56-bit, encryption).
            mbedtls_des_setkey_enc(ctx, crypto_sample_des3_keys);
        }
        break;
        case 2: {
            // Triple-DES key schedule (112-bit, decryption).
            mbedtls_des3_set2key_dec(ctx3, crypto_sample_des3_keys);
        }
        break;
        case 3: {
            // Triple-DES key schedule (112-bit, encryption).
            mbedtls_des3_set2key_enc(ctx3, crypto_sample_des3_keys);
        }
        break;
        case 4: {
            // Triple-DES key schedule (168-bit, decryption).
            mbedtls_des3_set3key_dec(ctx3, crypto_sample_des3_keys);
        }
        break;
        case 5: {
            // Triple-DES key schedule (168-bit, encryption).
            mbedtls_des3_set3key_enc(ctx3, crypto_sample_des3_keys);
        }
        break;
        default:
            goto cleanup;
        }

        if (v == MBEDTLS_DES_DECRYPT) {
            for (j = 0 ; j < CRYPTO_SAMPLE_DES_LOOP_COUNT ; j++) {
                if (u == 0) {
                    // DES-CBC buffer decryption.
                    mbedtls_des_crypt_cbc(ctx, v, 8, iv, buf, buf);
                } else {
                    // 3DES-CBC buffer decryption.
                    mbedtls_des3_crypt_cbc(ctx3, v, 8, iv, buf, buf);
                }
            }
        } else {
            for (j = 0; j < CRYPTO_SAMPLE_DES_LOOP_COUNT; j++) {
                unsigned char tmp[8] = {0x00,};

                if (u == 0) {
                    // DES-CBC buffer encryption.
                    mbedtls_des_crypt_cbc(ctx, v, 8, iv, buf, buf);
                } else {
                    // 3DES-CBC buffer encryption.
                    mbedtls_des3_crypt_cbc(ctx3, v, 8, iv, buf, buf);
                }

                memcpy(tmp, prv, 8);
                memcpy(prv, buf, 8);
                memcpy(buf, tmp, 8);
            }

            memcpy(buf, prv, 8);
        }

        // Compare output with expected output.
        if ((v == MBEDTLS_DES_DECRYPT)
                && (ret = memcmp(buf, crypto_sample_des3_cbc_dec[u], 8)) != 0) {
            PRINTF("failed\r\n");
            goto cleanup;
        } else if ((v == MBEDTLS_DES_ENCRYPT)
                   && (ret = memcmp(buf, crypto_sample_des3_cbc_enc[u], 8)) != 0) {
            PRINTF("failed\r\n");
            goto cleanup;
        }

        PRINTF("passed\r\n");
    }

cleanup:

    // Clear the DES context.
    mbedtls_des_free(ctx);

    // Clear the Triple-DES context.
    mbedtls_des3_free(ctx3);

    if (ctx) {
        crypto_sample_free(ctx);
    }

    if (ctx3) {
        crypto_sample_free(ctx3);
    }

    return ret;
}
#endif // (MBEDTLS_CIPHER_MODE_CBC)

/*
 * Sample Functions for DES Algorithms
 */
void crypto_sample_des(void *param)
{
    DA16X_UNUSED_ARG(param);

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    crypto_sample_des_cbc();
#endif // (MBEDTLS_CIPHER_MODE_CBC)

    return ;
}

/* EOF */
