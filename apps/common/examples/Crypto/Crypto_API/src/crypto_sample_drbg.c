/**
 ****************************************************************************************
 *
 * @file crypto_sample_drbg.c
 *
 * @brief Sample app of DRBG(deterministic random bit generator).
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
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hmac_drbg.h"

static size_t drbg_offset;

static int drbg_test_entropy(void *data, unsigned char *buf, size_t len)
{
    const unsigned char *p = data;
    memcpy(buf, p + drbg_offset, len);
    drbg_offset += len;
    return 0;
}

static const unsigned char crypto_sample_ctr_drbg_entropy_src_pr[96] = {
    0xc1, 0x80, 0x81, 0xa6, 0x5d, 0x44, 0x02, 0x16,
    0x19, 0xb3, 0xf1, 0x80, 0xb1, 0xc9, 0x20, 0x02,
    0x6a, 0x54, 0x6f, 0x0c, 0x70, 0x81, 0x49, 0x8b,
    0x6e, 0xa6, 0x62, 0x52, 0x6d, 0x51, 0xb1, 0xcb,
    0x58, 0x3b, 0xfa, 0xd5, 0x37, 0x5f, 0xfb, 0xc9,
    0xff, 0x46, 0xd2, 0x19, 0xc7, 0x22, 0x3e, 0x95,
    0x45, 0x9d, 0x82, 0xe1, 0xe7, 0x22, 0x9f, 0x63,
    0x31, 0x69, 0xd2, 0x6b, 0x57, 0x47, 0x4f, 0xa3,
    0x37, 0xc9, 0x98, 0x1c, 0x0b, 0xfb, 0x91, 0x31,
    0x4d, 0x55, 0xb9, 0xe9, 0x1c, 0x5a, 0x5e, 0xe4,
    0x93, 0x92, 0xcf, 0xc5, 0x23, 0x12, 0xd5, 0x56,
    0x2c, 0x4a, 0x6e, 0xff, 0xdc, 0x10, 0xd0, 0x68
};

static const unsigned char crypto_sample_ctr_drbg_entropy_src_nopr[64] = {
    0x5a, 0x19, 0x4d, 0x5e, 0x2b, 0x31, 0x58, 0x14,
    0x54, 0xde, 0xf6, 0x75, 0xfb, 0x79, 0x58, 0xfe,
    0xc7, 0xdb, 0x87, 0x3e, 0x56, 0x89, 0xfc, 0x9d,
    0x03, 0x21, 0x7c, 0x68, 0xd8, 0x03, 0x38, 0x20,
    0xf9, 0xe6, 0x5e, 0x04, 0xd8, 0x56, 0xf3, 0xa9,
    0xc4, 0x4a, 0x4c, 0xbd, 0xc1, 0xd0, 0x08, 0x46,
    0xf5, 0x98, 0x3d, 0x77, 0x1c, 0x1b, 0x13, 0x7e,
    0x4e, 0x0f, 0x9d, 0x8e, 0xf4, 0x09, 0xf9, 0x2e
};

static const unsigned char crypto_sample_ctr_brdg_nonce_pers_pr[16] = {
    0xd2, 0x54, 0xfc, 0xff, 0x02, 0x1e, 0x69, 0xd2,
    0x29, 0xc9, 0xcf, 0xad, 0x85, 0xfa, 0x48, 0x6c
};

static const unsigned char crypto_sample_ctr_brdg_nonce_pers_nopr[16] = {
    0x1b, 0x54, 0xb8, 0xff, 0x06, 0x42, 0xbf, 0xf5,
    0x21, 0xf1, 0x5c, 0x1c, 0x0b, 0x66, 0x5f, 0x3f
};

static const unsigned char crypto_sample_ctr_drbg_result_pr[16] = {
    0x34, 0x01, 0x16, 0x56, 0xb4, 0x29, 0x00, 0x8f,
    0x35, 0x63, 0xec, 0xb5, 0xf2, 0x59, 0x07, 0x23
};

static const unsigned char crypto_sample_ctr_drbg_result_nopr[16] = {
    0xa0, 0x54, 0x30, 0x3d, 0x8a, 0x7e, 0xa9, 0x88,
    0x9d, 0x90, 0x3e, 0x07, 0x7c, 0x6f, 0x21, 0x8f
};

int crypto_sample_ctr_drbg_pr_on()
{
    int ret = -1;
    mbedtls_ctr_drbg_context ctx;   //The CTR_DRBG context structure.
    unsigned char *buf = NULL;

    buf = crypto_sample_calloc(16, sizeof(unsigned char));
    if (!buf) {
        PRINTF("[%s] Failed to allocate buffer\r\n", __func__);
        goto cleanup;
    }

    drbg_offset = 0;

    // Based on a NIST CTR_DRBG test vector (PR = True)
    PRINTF("* CTR_DRBG (PR = TRUE): ");

    // Initialize the CTR_DRBG context.
    mbedtls_ctr_drbg_init(&ctx);

    ret = mbedtls_ctr_drbg_seed_entropy_len(&ctx, drbg_test_entropy,
                                            (void *)crypto_sample_ctr_drbg_entropy_src_pr,
                                            crypto_sample_ctr_brdg_nonce_pers_pr, 16, 32);
    if (ret) {
        PRINTF("[%s] Failed to get seed entropy len(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Turn prediction resistance on
    mbedtls_ctr_drbg_set_prediction_resistance(&ctx, MBEDTLS_CTR_DRBG_PR_ON);

    // Generate random data using CTR_DRBG.
    ret = mbedtls_ctr_drbg_random(&ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random data using CTR_DRBG.
    ret = mbedtls_ctr_drbg_random(&ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Compare output with expected output.
    if ((ret = memcmp(buf, crypto_sample_ctr_drbg_result_pr,
                      MBEDTLS_CTR_DRBG_BLOCKSIZE)) != 0) {
        PRINTF("[%s] Not matched output\r\n", __func__);
        goto cleanup;
    }

cleanup:

    // Clear CTR_CRBG context data.
    mbedtls_ctr_drbg_free(&ctx);

    if (buf) {
        crypto_sample_free(buf);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

int crypto_sample_ctr_drbg_pr_off()
{
    int ret = -1;
    mbedtls_ctr_drbg_context ctx;   //The CTR_DRBG context structure.
    unsigned char *buf = NULL;

    buf = crypto_sample_calloc(16, sizeof(unsigned char));
    if (!buf) {
        PRINTF("[%s] Failed to allocate buffer\r\n", __func__);
        goto cleanup;
    }

    drbg_offset = 0;

    // Based on a NIST CTR_DRBG test vector (PR = FALSE)
    PRINTF("* CTR_DRBG (PR = FALSE): ");

    // Initialize the CTR_DRBG context.
    mbedtls_ctr_drbg_init(&ctx);

    ret = mbedtls_ctr_drbg_seed_entropy_len(&ctx, drbg_test_entropy,
                                            (void *) crypto_sample_ctr_drbg_entropy_src_nopr,
                                            crypto_sample_ctr_brdg_nonce_pers_nopr, 16, 32);
    if (ret) {
        PRINTF("[%s] Failed to get seed entropy len(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random data using CTR_DRBG.
    ret = mbedtls_ctr_drbg_random(&ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Reseed the CTR_DRBG context, that is extracts data from the entropy source.
    ret = mbedtls_ctr_drbg_reseed(&ctx, NULL, 0);
    if (ret) {
        PRINTF("[%s] Failed to reseed the CTR_DRBG context(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random data using CTR_DRBG.
    ret = mbedtls_ctr_drbg_random(&ctx, buf, MBEDTLS_CTR_DRBG_BLOCKSIZE);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Compare output with expected output.
    if ((ret = memcmp(buf, crypto_sample_ctr_drbg_result_nopr,
                      MBEDTLS_CTR_DRBG_BLOCKSIZE)) != 0) {
        PRINTF("[%s] Not matched output\r\n", __func__);
        goto cleanup;
    }

cleanup:

    // Clear CTR_CRBG context data.
    mbedtls_ctr_drbg_free(&ctx);

    if (buf) {
        crypto_sample_free(buf);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}


#define CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN  80

/* From a NIST PR=true test vector */
static const unsigned char crypto_sample_hmac_drbg_entropy_src_pr[] = {
    0xa0, 0xc9, 0xab, 0x58, 0xf1, 0xe2, 0xe5, 0xa4,
    0xde, 0x3e, 0xbd, 0x4f, 0xf7, 0x3e, 0x9c, 0x5b,
    0x64, 0xef, 0xd8, 0xca, 0x02, 0x8c, 0xf8, 0x11,
    0x48, 0xa5, 0x84, 0xfe, 0x69, 0xab, 0x5a, 0xee,
    0x42, 0xaa, 0x4d, 0x42, 0x17, 0x60, 0x99, 0xd4,
    0x5e, 0x13, 0x97, 0xdc, 0x40, 0x4d, 0x86, 0xa3,
    0x7b, 0xf5, 0x59, 0x54, 0x75, 0x69, 0x51, 0xe4
};

static const unsigned char
crypto_sample_hmac_drbg_result_pr[CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN] = {
    0x9a, 0x00, 0xa2, 0xd0, 0x0e, 0xd5, 0x9b, 0xfe,
    0x31, 0xec, 0xb1, 0x39, 0x9b, 0x60, 0x81, 0x48,
    0xd1, 0x96, 0x9d, 0x25, 0x0d, 0x3c, 0x1e, 0x94,
    0x10, 0x10, 0x98, 0x12, 0x93, 0x25, 0xca, 0xb8,
    0xfc, 0xcc, 0x2d, 0x54, 0x73, 0x19, 0x70, 0xc0,
    0x10, 0x7a, 0xa4, 0x89, 0x25, 0x19, 0x95, 0x5e,
    0x4b, 0xc6, 0x00, 0x1d, 0x7f, 0x4e, 0x6a, 0x2b,
    0xf8, 0xa3, 0x01, 0xab, 0x46, 0x05, 0x5c, 0x09,
    0xa6, 0x71, 0x88, 0xf1, 0xa7, 0x40, 0xee, 0xf3,
    0xe1, 0x5c, 0x02, 0x9b, 0x44, 0xaf, 0x03, 0x44
};

/* From a NIST PR=false test vector */
static const unsigned char crypto_sample_hmac_drbg_entropy_src_nopr[] = {
    0x79, 0x34, 0x9b, 0xbf, 0x7c, 0xdd, 0xa5, 0x79,
    0x95, 0x57, 0x86, 0x66, 0x21, 0xc9, 0x13, 0x83,
    0x11, 0x46, 0x73, 0x3a, 0xbf, 0x8c, 0x35, 0xc8,
    0xc7, 0x21, 0x5b, 0x5b, 0x96, 0xc4, 0x8e, 0x9b,
    0x33, 0x8c, 0x74, 0xe3, 0xe9, 0x9d, 0xfe, 0xdf
};

static const unsigned char
crypto_sample_hmac_drbg_result_nopr[CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN] = {
    0xc6, 0xa1, 0x6a, 0xb8, 0xd4, 0x20, 0x70, 0x6f,
    0x0f, 0x34, 0xab, 0x7f, 0xec, 0x5a, 0xdc, 0xa9,
    0xd8, 0xca, 0x3a, 0x13, 0x3e, 0x15, 0x9c, 0xa6,
    0xac, 0x43, 0xc6, 0xf8, 0xa2, 0xbe, 0x22, 0x83,
    0x4a, 0x4c, 0x0a, 0x0a, 0xff, 0xb1, 0x0d, 0x71,
    0x94, 0xf1, 0xc1, 0xa5, 0xcf, 0x73, 0x22, 0xec,
    0x1a, 0xe0, 0x96, 0x4e, 0xd4, 0xbf, 0x12, 0x27,
    0x46, 0xe0, 0x87, 0xfd, 0xb5, 0xb3, 0xe9, 0x1b,
    0x34, 0x93, 0xd5, 0xbb, 0x98, 0xfa, 0xed, 0x49,
    0xe8, 0x5f, 0x13, 0x0f, 0xc8, 0xa4, 0x59, 0xb7
};

int crypto_sample_hmac_drbg_pr_on()
{
    int ret = -1;
    mbedtls_hmac_drbg_context ctx;
    unsigned char *buf;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    buf = crypto_sample_calloc(CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN, sizeof(unsigned char));
    if (!buf) {
        PRINTF("[%s] Failed to allocate buffer for HMAC DRBG\r\n", __func__);
        goto cleanup;
    }

    drbg_offset = 0;

    PRINTF("* HMAC_DRBG (PR = True) : ");

    // Initialize HMAC DRBG context.
    mbedtls_hmac_drbg_init(&ctx);

    // HMAC_DRBG initial seeding Seed and setup entropy source for future reseeds.
    ret = mbedtls_hmac_drbg_seed(&ctx, md_info,
                                 drbg_test_entropy,
                                 (void *)crypto_sample_hmac_drbg_entropy_src_pr, NULL, 0);
    if (ret) {
        PRINTF("[%s] Failed to seed HMAC DRBG(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Enable prediction resistance.
    mbedtls_hmac_drbg_set_prediction_resistance(&ctx, MBEDTLS_HMAC_DRBG_PR_ON);

    // Generate random.
    ret = mbedtls_hmac_drbg_random(&ctx, buf, CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random.
    ret = mbedtls_hmac_drbg_random(&ctx, buf, CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Compare output with expected output.
    if ((ret = memcmp(buf,
                      crypto_sample_hmac_drbg_result_pr,
                      CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN)) != 0) {
        PRINTF("[%s] Not matched output\r\n", __func__);
        goto cleanup;
    }

cleanup:

    // Free an HMAC_DRBG context.
    mbedtls_hmac_drbg_free(&ctx);

    if (buf) {
        crypto_sample_free(buf);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

int crypto_sample_hmac_drbg_pr_off()
{
    int ret = -1;
    mbedtls_hmac_drbg_context ctx;
    unsigned char *buf = NULL;
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    buf = crypto_sample_calloc(CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN,
                               sizeof(unsigned char));
    if (!buf) {
        PRINTF("[%s] Failed to allocate buffer for HMAC DRBG\r\n", __func__);
        goto cleanup;
    }

    drbg_offset = 0;

    PRINTF("* HMAC_DRBG (PR = False) : ");

    // Initialize HMAC DRBG context.
    mbedtls_hmac_drbg_init(&ctx);

    // HMAC_DRBG initial seeding Seed and setup entropy source for future reseeds.
    ret = mbedtls_hmac_drbg_seed(&ctx, md_info,
                                 drbg_test_entropy,
                                 (void *)crypto_sample_hmac_drbg_entropy_src_nopr,
                                 NULL, 0);
    if (ret) {
        PRINTF("[%s] Failed to seed HMAC DRBG(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // HMAC_DRBG reseeding (extracts data from entropy source)
    ret = mbedtls_hmac_drbg_reseed(&ctx, NULL, 0);
    if (ret) {
        PRINTF("[%s] Failed to reseed HMAC DRBG(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random.
    ret = mbedtls_hmac_drbg_random(&ctx, buf, CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate random.
    ret = mbedtls_hmac_drbg_random(&ctx, buf, CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN);
    if (ret) {
        PRINTF("[%s] Failed to generate random data(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Compare output with expected output.
    if ((ret = memcmp(buf,
                      crypto_sample_hmac_drbg_result_nopr,
                      CRYPTO_SAMPLE_HMAC_DRBG_OUTPUT_LEN)) != 0) {
        PRINTF("[%s] Not matched output\r\n", __func__);
        goto cleanup;
    }

cleanup:

    // Free an HMAC_DRBG context.
    mbedtls_hmac_drbg_free(&ctx);

    if (buf) {
        crypto_sample_free(buf);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/*
 * Sample Functions for DRBG
 */
void crypto_sample_drbg(void *param)
{
    DA16X_UNUSED_ARG(param);

    crypto_sample_ctr_drbg_pr_on();

    crypto_sample_ctr_drbg_pr_off();

    crypto_sample_hmac_drbg_pr_on();

    crypto_sample_hmac_drbg_pr_off();

    return ;
}

/* EOF */
