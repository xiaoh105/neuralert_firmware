/**
 ****************************************************************************************
 *
 * @file crypto_sample_hash.c
 *
 * @brief Sample app of Hash and HMAC algorithms.
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
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#if defined(MBEDTLS_MD5_C)
#include "mbedtls/md5.h"
#endif // (MBEDTLS_MD5_C)
#include "mbedtls/md.h"

/**
 * Convert len bytes from in into hexadecimal, writing the resulting
 */
static void hexify(unsigned char *out, const unsigned char *in, int len)
{
    unsigned char l, h;

    while (len != 0) {
        h = *in / 16;
        l = *in % 16;

        if (h < 10) {
            *out++ = '0' + h;
        } else {
            *out++ = 'a' + h - 10;
        }

        if (l < 10) {
            *out++ = '0' + l;
        } else {
            *out++ = 'a' + l - 10;
        }

        ++in;
        len--;
    }

    return ;
}

/**
 * Convert hexadecimal characters from in, and write them to out.
 */
static int unhexify(unsigned char *out, const char *in)
{
    unsigned char c, c2;
    int len = strlen(in) / 2;

    while (*in != 0) {
        c = *in++;

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'a' && c <= 'f') {
            c -= 'a' - 10;
        } else if (c >= 'A' && c <= 'F') {
            c -= 'A' - 10;
        }

        c2 = *in++;

        if (c2 >= '0' && c2 <= '9') {
            c2 -= '0';
        } else if (c2 >= 'a' && c2 <= 'f') {
            c2 -= 'a' - 10;
        } else if (c2 >= 'A' && c2 <= 'F') {
            c2 -= 'A' - 10;
        }

        *out++ = (c << 4) | c2;
    }

    return len;
}

/**
 * SHA-1 Hash
 * Test Vecotr: FIPS-180-1
 */
static const unsigned char crypto_sample_hash_sha1_buf[4] = { "abc" };

static const int crypto_sample_hash_sha1_buflen = 3;

static const unsigned char crypto_sample_hash_sha1_sum[20] = {
    0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
    0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D
};

/**
 * crypto_sample_hash_sha1 - Sample Function for SHA-1 Hash
 */
int crypto_sample_hash_sha1()
{
    int ret = -1;
    unsigned char *sha1sum = NULL;
    mbedtls_sha1_context *ctx = NULL;

    ctx = (mbedtls_sha1_context *)crypto_sample_calloc(1, sizeof(mbedtls_sha1_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate SHA-1 context\r\n", __func__);
        return ret;
    }

    sha1sum = crypto_sample_calloc(20, sizeof(unsigned char));
    if (!sha1sum) {
        PRINTF("[%s] Failed to allocate SHA-1 output buffer\r\n", __func__);
        goto cleanup;
    }

    PRINTF("* SHA-1: ");

    // Initialize a SHA-1 context.
    mbedtls_sha1_init(ctx);

    // Start a SHA-1 checksum calculation.
    mbedtls_sha1_starts_ret(ctx);

    // Feed an input buffer into an ongoing SHA-1 checksum calculation.
    mbedtls_sha1_update_ret(ctx, crypto_sample_hash_sha1_buf,
                            crypto_sample_hash_sha1_buflen);

    // Finishe the SHA-1 operation, and writes the result to the output buffer.
    mbedtls_sha1_finish(ctx, sha1sum);

    // Compare output with expected output.
    if ((ret = memcmp(sha1sum, crypto_sample_hash_sha1_sum, 20)) != 0) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

cleanup:

    // Clear a SHA-1 context.
    mbedtls_sha1_free(ctx);

    if (sha1sum) {
        crypto_sample_free(sha1sum);
    }

    if (ctx) {
        crypto_sample_free(ctx);
    }

    return ret;
}

/**
 * SHA-224 Hash
 * Test Vecotr: FIPS-180-2
 */
static const unsigned char crypto_sample_hash_sha224_buf[4] = { "abc" };

static const int crypto_sample_hash_sha224_buflen = 3;

static const unsigned char crypto_sample_hash_sha224_sum[32] = {
    0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
    0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
    0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
    0xE3, 0x6C, 0x9D, 0xA7
};

/**
 * crypto_sample_hash_sha224 - Sample Function for SHA-224 Hash
 */
int crypto_sample_hash_sha224()
{
    int ret = -1;
    unsigned char *sha224sum = NULL;
    mbedtls_sha256_context *ctx = NULL;

    ctx = (mbedtls_sha256_context *)crypto_sample_calloc(1, sizeof(mbedtls_sha256_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate SHA-224 Context\r\n", __func__);
        return ret;
    }

    sha224sum = crypto_sample_calloc(32, sizeof(unsigned char));
    if (!sha224sum) {
        PRINTF("[%s] Failed to allocate SHA-224 output buffer\r\n", __func__);
        goto cleanup;
    }

    PRINTF("* SHA-224: ");

    // Initialize the SHA-224 context.
    mbedtls_sha256_init(ctx);

    // Start a SHA-224 checksum calculation.
    mbedtls_sha256_starts_ret(ctx, 1);

    // Feeds an input buffer into an ongoing SHA-224 checksum calculation.
    mbedtls_sha256_update_ret(ctx,
                              crypto_sample_hash_sha224_buf, crypto_sample_hash_sha224_buflen);

    // Finishe the SHA-224 operation, and writes the result to the output buffer.
    mbedtls_sha256_finish_ret(ctx, sha224sum);

    if ((ret = memcmp(sha224sum, crypto_sample_hash_sha224_sum, 28)) != 0 ) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

cleanup:

    //Clear s SHA-224 context.
    mbedtls_sha256_free(ctx);

    if (sha224sum) {
        crypto_sample_free(sha224sum);
    }

    if (ctx) {
        crypto_sample_free(ctx);
    }

    return ( ret );
}

/**
 * SHA-256 Hash
 * Test Vecotr: FIPS-180-2
 */
static const unsigned char crypto_sample_hash_sha256_buf[4] = { "abc" };

static const int crypto_sample_hash_sha256_buflen = 3;

static const unsigned char crypto_sample_hash_sha256_sum[32] = {
    0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
    0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
    0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
    0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD
};

/**
 * crypto_sample_hash_sha256 - Sample Function for SHA-256 Hash
 */
int crypto_sample_hash_sha256()
{
    int ret = -1;
    unsigned char *sha256sum = NULL;
    mbedtls_sha256_context *ctx = NULL;

    ctx = (mbedtls_sha256_context *)crypto_sample_calloc(1, sizeof(mbedtls_sha256_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate SHA-256 Context\r\n", __func__);
        return ret;
    }

    sha256sum = crypto_sample_calloc(32, sizeof(unsigned char));
    if (!sha256sum) {
        PRINTF("[%s] Failed to allocate SHA-256 output buffer\r\n", __func__);
        goto cleanup;
    }

    PRINTF("* SHA-256: ");

    // Initialize the SHA-256 context.
    mbedtls_sha256_init(ctx);

    // Start a SHA-256 checksum calculation.
    mbedtls_sha256_starts_ret(ctx, 0);

    // Feeds an input buffer into an ongoing SHA-256 checksum calculation.
    mbedtls_sha256_update_ret(ctx,
                              crypto_sample_hash_sha256_buf, crypto_sample_hash_sha256_buflen);

    // Finishe the SHA-256 operation, and writes the result to the output buffer.
    mbedtls_sha256_finish_ret(ctx, sha256sum);

    if ((ret = memcmp(sha256sum, crypto_sample_hash_sha256_sum, 32)) != 0 ) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

cleanup:

    //Clear s SHA-256 context.
    mbedtls_sha256_free(ctx);

    if (sha256sum) {
        crypto_sample_free(sha256sum);
    }

    if (ctx) {
        crypto_sample_free(ctx);
    }

    return ret;
}

/**
 * SHA-384 Hash
 * Test Vecotr: FIPS-180-2
 */
static const unsigned char crypto_sample_hash_sha384_buf[4] = { "abc" };

static const int crypto_sample_hash_sha384_buflen = 3;

static const unsigned char crypto_sample_hash_sha384_sum[48] = {
    0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B,
    0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
    0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63,
    0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
    0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23,
    0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7
};

/**
 * crypto_sample_hash_sha384 - Sample Function for SHA-384 Hash
 */
int crypto_sample_hash_sha384()
{
    int ret = -1;
    unsigned char *sha384sum = NULL;
    mbedtls_sha512_context *ctx = NULL;

    ctx = (mbedtls_sha512_context *)crypto_sample_calloc(1, sizeof(mbedtls_sha512_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate SHA-384 Context\r\n", __func__);
        return ret;
    }

    sha384sum = crypto_sample_calloc(64, sizeof(unsigned char));
    if (!sha384sum) {
        PRINTF("[%s] Failed to allocate SHA-384 output buffer\r\n", __func__);
        goto cleanup;
    }

    PRINTF("* SHA-384: ");

    // Initialize a SHA-384 context.
    mbedtls_sha512_init(ctx);

    // Start a SHA-384 checksum calculation.
    mbedtls_sha512_starts_ret(ctx, 1);

    // Feed an input buffer into an ongoing SHA-384 checksum calculation.
    mbedtls_sha512_update(ctx,
                          crypto_sample_hash_sha384_buf, crypto_sample_hash_sha384_buflen);

    // Finishe the SHA-384 operation, and writes the result to the output buffer.
    mbedtls_sha512_finish(ctx, sha384sum);

    if ((ret = memcmp(sha384sum, crypto_sample_hash_sha384_sum, 48)) != 0) {
        PRINTF( "failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

cleanup:

    // Clear a SHA-384 context.
    mbedtls_sha512_free(ctx);

    if (sha384sum) {
        crypto_sample_free(sha384sum);
    }

    if (ctx) {
        crypto_sample_free(ctx);
    }

    return ret;
}

/**
 * SHA-512 Hash
 * Test Vecotr: FIPS-180-2
 */
static const unsigned char crypto_sample_hash_sha512_buf[4] = { "abc" };

static const int crypto_sample_hash_sha512_buflen = 3;

static const unsigned char crypto_sample_hash_sha512_sum[64] = {
    0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
    0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
    0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
    0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
    0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
    0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
    0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
    0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F
};

/**
 * crypto_sample_hash_sha512 - Sample Function for SHA-512 Hash
 */
int crypto_sample_hash_sha512()
{
    int ret = -1;
    unsigned char *sha512sum = NULL;
    mbedtls_sha512_context *ctx = NULL;

    ctx = (mbedtls_sha512_context *)crypto_sample_calloc(1, sizeof(mbedtls_sha512_context));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate SHA-512 Context\r\n", __func__);
        return ret;
    }

    sha512sum = crypto_sample_calloc(64, sizeof(unsigned char));
    if (!sha512sum) {
        PRINTF("[%s] Failed to allocate SHA-512 output buffer\r\n", __func__);
        goto cleanup;
    }

    PRINTF("* SHA-512: ");

    // Initialize a SHA-512 context.
    mbedtls_sha512_init(ctx);

    // Start a SHA-512 checksum calculation.
    mbedtls_sha512_starts_ret(ctx, 0);

    // Feed an input buffer into an ongoing SHA-512 checksum calculation.
    mbedtls_sha512_update_ret(ctx, crypto_sample_hash_sha512_buf,
                              crypto_sample_hash_sha512_buflen);

    // Finishe the SHA-512 operation, and writes the result to the output buffer.
    mbedtls_sha512_finish(ctx, sha512sum);

    if ((ret = memcmp(sha512sum, crypto_sample_hash_sha512_sum, 64)) != 0) {
        PRINTF( "failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

cleanup:

    // Clear a SHA-512 context.
    mbedtls_sha512_free(ctx);

    if (sha512sum) {
        crypto_sample_free(sha512sum);
    }

    if (ctx) {
        crypto_sample_free(ctx);
    }

    return ret;
}

#if defined(MBEDTLS_MD5_C)
/**
 * MD5 Hash
 * Test Vecotr: RFC 1321
 */
static const unsigned char crypto_sample_hash_md5_buf[4] = { "abc" };

static const int crypto_sample_hash_md5_buflen = 3;

static const unsigned char crypto_sample_hash_md5_sum[16] = {
    0x90, 0x01, 0x50, 0x98, 0x3C, 0xD2, 0x4F, 0xB0,
    0xD6, 0x96, 0x3F, 0x7D, 0x28, 0xE1, 0x7F, 0x72
};

/**
 * crypto_sample_hash_md5 - Sample Function for MD5 Hash
 */
int crypto_sample_hash_md5()
{
    int ret = -1;
    unsigned char md5sum[16] = {0x00,};

    PRINTF("* MD5: ");

    // Output = MD5(input buffer)
    mbedtls_md5_ret(crypto_sample_hash_md5_buf, crypto_sample_hash_md5_buflen,
                    md5sum);

    if ((ret = memcmp(md5sum, crypto_sample_hash_md5_sum, 16)) != 0) {
        PRINTF( "failed\r\n" );
        return ret;
    }

    PRINTF("passed\r\n");

    return ret;
}
#endif	/* MBEDTLS_MD5_C */

/**
 * Hash Algorithm
 * Mbed TLS supports the generic message-digest wrapper.
 * This sample function prints message-digest's information using them.
 */
typedef  struct {
    char			*md_name;
    mbedtls_md_type_t	md_type;
    int			md_size;
} crypto_sample_md_info_type;

static const crypto_sample_md_info_type crypto_sample_md_info_list[] = {
#if defined(MBEDTLS_MD2_C)
    { "MD2", MBEDTLS_MD_MD2, 16 },
#endif // (MBEDTLS_MD2_C)
#if defined(MBEDTLS_MD4_C)
    { "MD4", MBEDTLS_MD_MD4, 16 },
#endif // (MBEDTLS_MD4_C)
#if defined(MBEDTLS_MD5_C)
    { "MD5", MBEDTLS_MD_MD5, 16 },
#endif // (MBEDTLS_MD5_C)
#if defined(MBEDTLS_RIPEMD160_C)
    { "RIPEMD160", MBEDTLS_MD_RIPEMD160, 20 },
#endif // (MBEDTLS_RIPEMD160_C)
    { "SHA1", MBEDTLS_MD_SHA1, 20 },
    { "SHA224", MBEDTLS_MD_SHA224, 28 },
    { "SHA256", MBEDTLS_MD_SHA256, 32 },
    { "SHA384", MBEDTLS_MD_SHA384, 48},
    { "SHA512", MBEDTLS_MD_SHA512, 64 },
    { NULL, MBEDTLS_MD_NONE, 0}
};


/**
 * crypto_sample_hash_md_wrapper_info - Sample Function to print the message-digest information.
 * @md_name: the message-digest name
 * @md_type: type of the message-digest
 * @md_size: size of the message-digest
 */
int crypto_sample_hash_md_wrapper_info(char *md_name, mbedtls_md_type_t md_type,
                                       int md_size)
{
    int ret = -1;
    const mbedtls_md_info_t *md_info = NULL;
    const int *md_type_ptr = NULL;
    int found = 0;

    PRINTF(">>> %s: ", md_name);

    // Get the message-digest information associated with the given digest type.
    md_info = mbedtls_md_info_from_type(md_type);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Type(%d)\r\n", __func__, md_type);
        goto cleanup;
    }

    // Get the message-digest information associated with the given digest name.
    if (md_info != mbedtls_md_info_from_string(md_name)) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    // Extract the message-digest type from the message-digest information structure.
    if (mbedtls_md_get_type(md_info) != (mbedtls_md_type_t)md_type) {
        PRINTF("[%s] Not matched Hash Type\r\n", __func__);
        goto cleanup;
    }

    // Extract the message-digest size from the message-digest information structure.
    if (mbedtls_md_get_size(md_info) != (unsigned char)md_size) {
        PRINTF("[%s] Not matched Hash Size\r\n", __func__);
        goto cleanup;
    }

    // Extract the message-digest name from the message-digest information structure.
    if (strcmp(mbedtls_md_get_name(md_info), md_name) != 0) {
        PRINTF("[%s] Not matched Hash Name\r\n", __func__);
        goto cleanup;
    }

    // Find the list of digests supported by the generic digest module.
    for (md_type_ptr = mbedtls_md_list() ; *md_type_ptr != 0 ; md_type_ptr++) {
        if (*md_type_ptr == md_type) {
            found = 1;
            break;
        }
    }

    if (!found) {
        PRINTF("[%s] Not supported Hash Type(%d)\r\n", __func__, md_type);
        ret = -1;
        goto cleanup;
    } else {
        ret = 0;
    }

cleanup:

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * Hash Algorithm
 * Mbed TLS supports the generic message-digest wrapper.
 * This sample function calculates hash with text string using selected message-digest.
 */
typedef  struct {
    char	*md_name;
    char	*text_src_string;
    char	*hex_hash_string;
} crypto_sample_md_text_type;

static const crypto_sample_md_text_type crypto_sample_md_text_list[] = {
#if defined(MBEDTLS_MD2_C)
    //Test vector: RFC1319 #3
    {
        "MD2",
        "abc",
        "da853b0d3f88d99b30283a69e6ded6bb"
    },
#endif // (MBEDTLS_MD2_C)
#if defined(MBEDTLS_MD4_C)
    // Test vector: RFC1320 #3
    {
        "MD4",
        "abc",
        "a448017aaf21d8525fc10ae87aa6729d"
    },
#endif // (MBEDTLS_MD4_C)
#if defined(MBEDTLS_MD5_C)
    // Test vector RFC1321 #3
    {
        "MD5",
        "abc",
        "900150983cd24fb0d6963f7d28e17f72"
    },
#endif // (MBEDTLS_MD5_C)
#if defined(MBEDTLS_RIPEMD160_C)
    //Test vector: paper #3
    {
        "RIPEMD160",
        "abc",
        "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc"
    },
#endif // (MBEDTLS_RIPEMD160_C)
    { NULL, NULL, NULL }
};

/**
 * crypto_sample_hash_md_wrapper_text
 * Sample funtion calculates hash with text string using selected message-digest.
 * @md_name: the message-digest name.
 * @text_src_string: The buffer holding the input data.
 * @hex_hash_string: The expected hash result.
 */
int crypto_sample_hash_md_wrapper_text(char *md_name, char *text_src_string,
                                       char *hex_hash_string)
{
    int ret = -1;;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    const mbedtls_md_info_t *md_info = NULL;

    PRINTF(">>> %s: ", md_name);

    hash_str = crypto_sample_calloc(100, sizeof(char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    // Get the message-digest information associated with the given digest name.
    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    /* Calculates the message-digest of a buffer,
     * with respect to a configurable message-digest algorithm in a single call.
     */
    ret = mbedtls_md(md_info,
                     (const unsigned char *)text_src_string,
                     strlen(text_src_string),
                     output);
    if (ret) {
        PRINTF("[%s] Failed to calculate the message-digest(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    // Compare output with expected output.
    if ((ret = strcmp((char *)hash_str, hex_hash_string)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * HMAC Algorithm
 * Mbed TLS supports the generic message-digest wrapper.
 * This sample function calculates hash with hex data using selected message-digest.
 */
typedef  struct {
    char	*md_name;
    int		trunc_size;
    char	*hex_key_string;
    char	*hex_src_string;
    char	*hex_hash_string;
} crypto_sample_md_hmac_type;

static const crypto_sample_md_hmac_type crypto_sample_md_hmac_list[] = {
#if defined(MBEDTLS_MD2_C)
    // Test vecotr: OpenSSL test #1
    {
        "MD2",
        16,
        "61616161616161616161616161616161",
        "b91ce5ac77d33c234e61002ed6",
        "d5732582f494f5ddf35efd166c85af9c"
    },
#endif // (MBEDTLS_MD2_C)
#if defined(MBEDTLS_MD4_C)
    // Test vector: OpenSSL test #1
    {
        "MD4",
        16,
        "61616161616161616161616161616161",
        "b91ce5ac77d33c234e61002ed6",
        "eabd0fbefb82fb0063a25a6d7b8bdc0f"
    },
#endif // (MBEDTLS_MD4_C)
    // Test vector: OpenSSL test #1
    {
        "MD5",
        16,
        "61616161616161616161616161616161",
        "b91ce5ac77d33c234e61002ed6",
        "42552882f00bd4633ea81135a184b284"
    },
#if defined(MBEDTLS_RIPEMD160_C)
    // Test vector: RFC 2286 #1
    {
        "RIPEMD160",
        20,
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
        "4869205468657265",
        "24cb4bd67d20fc1a5d2ed7732dcc39377f0a5668"
    },
#endif // (MBEDTLS_RIPEMD160_C)
    // Test vector: FIPS-198a #1
    {
        "SHA1",
        20,
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f",
        "53616d706c65202331",
        "4f4ca3d5d68ba7cc0a1208c9c61e9c5da0403c0a"
    },
    // Test vector: NIST CAVS #1
    {
        "SHA224",
        14,
        "e055eb756697ee573fd3214811a9f7fa",
        "3875847012ee42fe54a0027bdf38cca7021b83a2ed0503af69ef6c37c637bc1114fba40096c5947d736e19b7af3c68d95a4e3b8b073adbbb80f47e9db8f2d4f0018ddd847fabfdf9dd9b52c93e40458977725f6b7ba15f0816bb895cdf50401268f5d702b7e6a5f9faef57b8768c8a3fc14f9a4b3182b41d940e337d219b29ff",
        "40a453133361cc48da11baf616ee"
    },
    // Test vector: NIST CAVS #1
    {
        "SHA256",
        16,
        "cdffd34e6b16fdc0",
        "d83e78b99ab61709608972b36e76a575603db742269cc5dd4e7d5ca7816e26b65151c92632550cb4c5253c885d5fce53bc47459a1dbd5652786c4aac0145a532f12c05138af04cbb558101a7af5df478834c2146594dd73690d01a4fe72545894335f427ac70204798068cb86c5a600b40b414ede23590b41e1192373df84fe3",
        "c6f0dde266cb4a26d41e8259d33499cc"
    },
    // Test vector: NIST CAVS #1
    {
        "SHA384",
        32,
        "91a7401817386948ca952f9a20ee55dc",
        "2fea5b91035d6d501f3a834fa178bff4e64b99a8450432dafd32e4466b0e1e7781166f8a73f7e036b3b0870920f559f47bd1400a1a906e85e0dcf00a6c26862e9148b23806680f285f1fe4f93cdaf924c181a965465739c14f2268c8be8b471847c74b222577a1310bcdc1a85ef1468aa1a3fd4031213c97324b7509c9050a3d",
        "6d7be9490058cf413cc09fd043c224c2ec4fa7859b13783000a9a593c9f75838"
    },
    // Test vector: NIST CAVS #1
    {
        "SHA512",
        32,
        "c95a17c09940a691ed2d621571b0eb844ede55a9",
        "99cd28262e81f34878cdcebf4128e05e2098a7009278a66f4c785784d0e5678f3f2b22f86e982d273b6273a222ec61750b4556d766f1550a7aedfe83faedbc4bdae83fa560d62df17eb914d05fdaa48940551bac81d700f5fca7147295e386e8120d66742ec65c6ee8d89a92217a0f6266d0ddc60bb20ef679ae8299c8502c2f",
        "6bc1379d156559ddee2ed420ea5d5c5ff3e454a1059b7ba72c350e77b6e9333c"
    },
    { NULL, 0, NULL, NULL, NULL }
};

/**
 * crypto_sample_hash_md_wrapper_hmac
 * Sample funtion calculates hmac with hex data using selected message-digest.
 * @md_name: the message-digest name.
 * @trunc_size: The length of result.
 * @hex_key_string: The HMAC secret key.
 * @hex_src_string: The buffer holding the input data.
 * @hex_hash_sring: The expected HMAC result.
 */
int crypto_sample_hash_md_wrapper_hmac(char *md_name,
                                       int trunc_size,
                                       char *hex_key_string,
                                       char *hex_src_string,
                                       char *hex_hash_string)
{
    int ret = -1;
    unsigned char *src_str = NULL;
    unsigned char *key_str = NULL;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    int key_len = 0;
    int src_len = 0;
    const mbedtls_md_info_t *md_info = NULL;

    PRINTF(">>> %s: ", md_name);

    src_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!src_str) {
        PRINTF("[%s] Failed to allocate src buffer\r\n", __func__);
        goto cleanup;
    }

    key_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!key_str) {
        PRINTF("[%s] Failed to allocate key str buffer\r\n", __func__);
        goto cleanup;
    }

    hash_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(unsigned char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    // Get the message-digest information associated with the given digest name.
    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    key_len = unhexify(key_str, hex_key_string);

    src_len = unhexify(src_str, hex_src_string);

    // Calculate the full generic HMAC on the input buffer with the provided key.
    ret = mbedtls_md_hmac(md_info, key_str, key_len, src_str, src_len, output);

    if (ret) {
        PRINTF("[%s] Failed to calcaulte the full generic HMAC(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    if ((ret = strncmp((char *)hash_str, hex_hash_string, trunc_size * 2)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    if (src_str) {
        crypto_sample_free(src_str);
    }

    if (key_str) {
        crypto_sample_free(key_str);
    }

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * crypto_sample_hash_md_wrapper_text_multi
 * Sample funtion calculates hash with text string using selected message-digest.
 * The input data(@text_src_string) will be divided to describe multiple input datas.
 * @md_name: the message-digest name.
 * @text_src_string: The buffer holding the input data.
 * @hex_hash_string: The expected hash result.
 */
int crypto_sample_hash_md_wrapper_text_multi(char *md_name,
        char *text_src_string,
        char *hex_hash_string)
{
    int ret = -1;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    int halfway = 0;
    int len = 0;

    const mbedtls_md_info_t *md_info = NULL;
    mbedtls_md_context_t *ctx = NULL;       //The generic message-digest context.

    PRINTF(">>> %s: ", md_name);

    ctx = (mbedtls_md_context_t *)crypto_sample_calloc(1, sizeof(mbedtls_md_context_t));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate MD context\r\n", __func__);
        goto cleanup;
    }

    hash_str = crypto_sample_calloc(1000, sizeof(char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    /* Initialize a message-digest context without binding it
     * to a particular message-digest algorithm.
     */
    mbedtls_md_init(ctx);

    len = strlen((char *)text_src_string);

    halfway = len / 2;

    // Get the message-digest information associated with the given digest name.
    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    // Select the message digest algorithm to use, and allocates internal structures.
    ret = mbedtls_md_setup(ctx, md_info, 0);
    if (ret) {
        PRINTF("[%s] Failed to setup the message digest algorithm(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Start a message-digest computation.
    ret = mbedtls_md_starts(ctx);
    if (ret) {
        PRINTF("[%s] Failed to start a message-digest computation(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    if (!ctx->md_ctx) {
        PRINTF("[%s] Failed to get MD context\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_update(ctx, (const unsigned char *)text_src_string, halfway);
    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_update(ctx, (const unsigned char *)(text_src_string + halfway),
                            len - halfway);

    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Finish the digest operation, and writes the result to the output buffer.
    ret = mbedtls_md_finish(ctx, output);
    if (ret) {
        PRINTF("[%s] Failed to finish the digest operation(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    // Compare output with expected output.
    if ((ret = strcmp((char *)hash_str, hex_hash_string)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    /* Clear the internal structure of ctx and frees any embedded internal structure,
     * but does not free ctx itself.
     */
    mbedtls_md_free(ctx);

    if (ctx) {
        crypto_sample_free(ctx);
    }

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * crypto_sample_hash_md_wrapper_hmac_multi
 * Sample funtion calculates hmac with hex data using selected message-digest.
 * The input data(@hex_src_string) will be divided to describe multiple input data.
 * @md_name: the message-digest name.
 * @trunc_size: The length of result.
 * @hex_key_string: The HMAC secret key.
 * @hex_src_string: The buffer holding the input data.
 * @hex_hash_sring: The expected HMAC result.
 */
int crypto_sample_hash_md_wrapper_hmac_multi(char *md_name,
        int trunc_size,
        char *hex_key_string,
        char *hex_src_string,
        char *hex_hash_string)
{
    unsigned char *src_str = NULL;
    unsigned char *key_str = NULL;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    int key_len = 0;
    int src_len = 0;
    int halfway = 0;

    const mbedtls_md_info_t *md_info = NULL;
    mbedtls_md_context_t *ctx = NULL;

    int ret = -1;

    PRINTF(">>> %s: ", md_name);

    ctx = (mbedtls_md_context_t *)crypto_sample_calloc(1, sizeof(mbedtls_md_context_t));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate MD context\r\n", __func__);
        goto cleanup;
    }

    src_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!src_str) {
        PRINTF("[%s] Failed to allocate src buffer\r\n", __func__);
        goto cleanup;
    }

    key_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!key_str) {
        PRINTF("[%s] Failed to allocate key str buffer\r\n", __func__);
        goto cleanup;
    }

    hash_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(unsigned char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    /* Initialize a message-digest context without binding it
     * to a particular message-digest algorithm.
     */
    mbedtls_md_init(ctx);

    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    // Select the message digest algorithm to use, and allocates internal structures.
    ret = mbedtls_md_setup(ctx, md_info, 1);
    if (ret) {
        PRINTF("[%s] Failed to setup the message digest algorithm(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    key_len = unhexify( key_str, hex_key_string );

    src_len = unhexify( src_str, hex_src_string );

    halfway = src_len / 2;

    // Start a message-digest computation.
    ret = mbedtls_md_hmac_starts(ctx, key_str, key_len);
    if (ret) {
        PRINTF("[%s] Failed to start a message-digest computation(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    if (!ctx->md_ctx) {
        PRINTF("[%s] Failed to get MD context\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_hmac_update(ctx, src_str, halfway);
    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_hmac_update(ctx, src_str + halfway, src_len - halfway);
    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Finish the digest operation, and writes the result to the output buffer.
    ret = mbedtls_md_hmac_finish(ctx, output);
    if (ret) {
        PRINTF("[%s] Failed to finish the digest operation(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    if ((ret = strncmp((char *)hash_str, hex_hash_string, trunc_size * 2)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    /* Clear the internal structure of ctx and frees any embedded internal structure,
     * but does not free ctx itself.
     */
    mbedtls_md_free(ctx);

    if (ctx) {
        crypto_sample_free(ctx);
    }

    if (src_str) {
        crypto_sample_free(src_str);
    }

    if (key_str) {
        crypto_sample_free(key_str);
    }

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * Hash Algorithm
 * Mbed TLS supports the generic message-digest wrapper.
 * This sample function calculates hash with hex data using selected message-digest.
 */
typedef  struct {
    char	*md_name;
    char	*hex_src_string;
    char	*hex_hash_string;
} crypto_sample_md_hex_type;

static const crypto_sample_md_hex_type crypto_sample_md_hex_list[] = {
    // Test vector: NIST CAVS #3
    {
        "SHA1",
        "3000",
        "f944dcd635f9801f7ac90a407fbc479964dec024"
    },
    // Test vector: NIST CAVS #3
    {
        "SHA224",
        "984c",
        "2fa9df9157d9e027cfbc4c6a9df32e1adc0cbe2328ec2a63c5ae934e"
    },
    // Test vector NIST CAVS #3
    {
        "SHA256",
        "5fd4",
        "7c4fbf484498d21b487b9d61de8914b2eadaf2698712936d47c3ada2558f6788"
    },
    // Test vector: NIST CAVS #3
    {
        "SHA384",
        "7c27",
        "3d80be467df86d63abb9ea1d3f9cb39cd19890e7f2c53a6200bedc5006842b35e820dc4e0ca90ca9b97ab23ef07080fc"
    },
    // Test vector: NIST CAVS #3
    {
        "SHA512",
        "e724",
        "7dbb520221a70287b23dbcf62bfc1b73136d858e86266732a7fffa875ecaa2c1b8f673b5c065d360c563a7b9539349f5f59bef8c0c593f9587e3cd50bb26a231"
    },
    { NULL, NULL, NULL }
};

/**
 * crypto_sample_hash_md_wrapper_hex
 * Sample funtion calculates hash with hex data using selected message-digest.
 * @md_name: the message-digest name.
 * @hex_src_string: The buffer holding the input data.
 * @hex_hash_string: The expected hash result.
 */
int crypto_sample_hash_md_wrapper_hex(char *md_name,
                                      char *hex_src_string,
                                      char *hex_hash_string)
{
    int ret = -1;
    unsigned char *src_str = NULL;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    int src_len = 0;
    const mbedtls_md_info_t *md_info = NULL;

    PRINTF(">>> %s: ", md_name);

    src_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!src_str) {
        PRINTF("[%s] Failed to allocate src buffer\r\n", __func__);
        goto cleanup;
    }

    hash_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(unsigned char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    // Get the message-digest information associated with the given digest name.
    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    src_len = unhexify(src_str, hex_src_string);

    /* Calculates the message-digest of a buffer,
     * with respect to a configurable message-digest algorithm in a single call.
     */
    ret = mbedtls_md(md_info, src_str, src_len, output);
    if (ret) {
        PRINTF("[%s] Failed to calculate the message-digest(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    if ((ret = strcmp((char *)hash_str, hex_hash_string)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    if (src_str) {
        crypto_sample_free(src_str);
    }

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * crypto_sample_hash_md_wrapper_hex_multi
 * Sample funtion calculates hash with hex data using selected message-digest.
 * The input data(@hex_src_string) will be divided to describe multiple input datas.
 * @md_name: the message-digest name.
 * @hex_src_string: The buffer holding the input data.
 * @hex_hash_string: The expected hash result.
 */
int crypto_sample_hash_md_wrapper_hex_multi(char *md_name,
        char *hex_src_string,
        char *hex_hash_string)
{
    int ret = -1;
    unsigned char *src_str = NULL;
    unsigned char *hash_str = NULL;
    unsigned char *output = NULL;
    int src_len = 0;
    int halfway = 0;
    const mbedtls_md_info_t *md_info = NULL;
    mbedtls_md_context_t *ctx = NULL;

    PRINTF(">>> %s: ", md_name);

    ctx = (mbedtls_md_context_t *)crypto_sample_calloc(1, sizeof(mbedtls_md_context_t));
    if (!ctx) {
        PRINTF("[%s] Failed to allocate MD context\r\n", __func__);
        goto cleanup;
    }

    src_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!src_str) {
        PRINTF("[%s] Failed to allocate src buffer\r\n", __func__);
        goto cleanup;
    }

    hash_str = crypto_sample_calloc(10000, sizeof(unsigned char));
    if (!hash_str) {
        PRINTF("[%s] Failed to allocate hash str buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(100, sizeof(unsigned char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    /* Initialize a message-digest context without binding it
     * to a particular message-digest algorithm.
     */
    mbedtls_md_init(ctx);

    // Get the message-digest information associated with the given digest name.
    md_info = mbedtls_md_info_from_string(md_name);
    if (!md_info) {
        PRINTF("[%s] Unknown Hash Name(%s)\r\n", md_name);
        goto cleanup;
    }

    // Select the message digest algorithm to use, and allocates internal structures.
    ret = mbedtls_md_setup(ctx, md_info, 0);
    if (ret) {
        PRINTF("[%s] Failed to setup the message digest algorithm(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    src_len = unhexify(src_str, hex_src_string);

    halfway = src_len / 2;

    // Start a message-digest computation.
    ret = mbedtls_md_starts(ctx);
    if (ret) {
        PRINTF("[%s] Failed to start a message-digest computation(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    if (!ctx->md_ctx) {
        PRINTF("[%s] Failed to get MD context\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_update(ctx, src_str, halfway);
    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Feed an input buffer into an ongoing message-digest computation.
    ret = mbedtls_md_update(ctx, src_str + halfway, src_len - halfway);
    if (ret) {
        PRINTF("[%s] Failed to update message-digest(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Finish the digest operation, and writes the result to the output buffer.
    ret = mbedtls_md_finish(ctx, output);
    if (ret) {
        PRINTF("[%s] Failed to finish the digest operation(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    hexify(hash_str, output, mbedtls_md_get_size(md_info));

    if ((ret = strcmp((char *)hash_str, hex_hash_string)) != 0) {
        PRINTF("[%s] Not matched result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    /* Clear the internal structure of ctx and frees any embedded internal structure,
     * but does not free ctx itself.
     */
    mbedtls_md_free(ctx);

    if (ctx) {
        crypto_sample_free(ctx);
    }

    if (src_str) {
        crypto_sample_free(src_str);
    }

    if (hash_str) {
        crypto_sample_free(hash_str);
    }

    if (output) {
        crypto_sample_free(output);
    }

    PRINTF("%s\r\n", (ret == 0) ? "passed" : "failed");

    return ret;
}

/**
 * Mbed TLS supports the generic message-digest wrapper.
 * This sample function describes how hash & hmac algorithm calculate,
 * using supported message-digest.
 */
int crypto_sample_hash_md_wrapper()
{
    int ret = 0;
    int idx = 0;

    PRINTF("* Message-digest Information\r\n");

    for (idx = 0 ; crypto_sample_md_info_list[idx].md_name != NULL ; idx++) {
        ret = crypto_sample_hash_md_wrapper_info(crypto_sample_md_info_list[idx].md_name,
                crypto_sample_md_info_list[idx].md_type,
                crypto_sample_md_info_list[idx].md_size);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Hash with text string\r\n");

    for (idx = 0 ; crypto_sample_md_text_list[idx].md_name != NULL ; idx++) {
        ret = crypto_sample_hash_md_wrapper_text(crypto_sample_md_text_list[idx].md_name,
                crypto_sample_md_text_list[idx].text_src_string,
                crypto_sample_md_text_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Hash with multiple text string\r\n");

    for (idx = 0 ; crypto_sample_md_text_list[idx].md_name != NULL ; idx++) {
        ret = crypto_sample_hash_md_wrapper_text_multi(crypto_sample_md_text_list[idx].md_name,
                crypto_sample_md_text_list[idx].text_src_string,
                crypto_sample_md_text_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* HMAC with hex data\r\n");

    for (idx = 0 ; crypto_sample_md_hmac_list[idx].md_name != NULL ; idx++) {
        ret = crypto_sample_hash_md_wrapper_hmac(crypto_sample_md_hmac_list[idx].md_name,
                crypto_sample_md_hmac_list[idx].trunc_size,
                crypto_sample_md_hmac_list[idx].hex_key_string,
                crypto_sample_md_hmac_list[idx].hex_src_string,
                crypto_sample_md_hmac_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* HMAC with multiple hex data\r\n");

    for (idx = 0 ; crypto_sample_md_hmac_list[idx].md_name != NULL ; idx++) {
        ret =  crypto_sample_hash_md_wrapper_hmac_multi(crypto_sample_md_hmac_list[idx].md_name,
                crypto_sample_md_hmac_list[idx].trunc_size,
                crypto_sample_md_hmac_list[idx].hex_key_string,
                crypto_sample_md_hmac_list[idx].hex_src_string,
                crypto_sample_md_hmac_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Hash with hex data\r\n");

    for (idx = 0; crypto_sample_md_hex_list[idx].md_name != NULL; idx++) {
        ret = crypto_sample_hash_md_wrapper_hex(crypto_sample_md_hex_list[idx].md_name,
                                                crypto_sample_md_hex_list[idx].hex_src_string,
                                                crypto_sample_md_hex_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Hash with multiple hex data\r\n");

    for (idx = 0 ; crypto_sample_md_hex_list[idx].md_name != NULL ; idx++) {
        ret = crypto_sample_hash_md_wrapper_hex_multi(crypto_sample_md_hex_list[idx].md_name,
                crypto_sample_md_hex_list[idx].hex_src_string,
                crypto_sample_md_hex_list[idx].hex_hash_string);
        if (ret) {
            goto cleanup;
        }
    }

cleanup:

    return ret;
}

/*
 * Sample Functions for Hash & HMAC Algorithms
 */
void crypto_sample_hash(void *param)
{
    DA16X_UNUSED_ARG(param);

    crypto_sample_hash_sha1();

    crypto_sample_hash_sha224();

    crypto_sample_hash_sha256();

    crypto_sample_hash_sha384();

    crypto_sample_hash_sha512();

#if defined(MBEDTLS_MD5_C)
    crypto_sample_hash_md5();
#endif // (MBEDTLS_MD5_C)

    crypto_sample_hash_md_wrapper();

    return ;
}

/* EOF */
