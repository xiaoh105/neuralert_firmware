/**
 ****************************************************************************************
 *
 * @file crypto_sample_dhm.c
 *
 * @brief Sample app of Diffie-Hellman Key exchange.
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
#include "mbedtls/dhm.h"

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)								\
    {														\
        (b)[(i)    ] = (unsigned char) ( (n) >> 24 );		\
        (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );		\
        (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );		\
        (b)[(i) + 3] = (unsigned char) ( (n)       );		\
    }
#endif

/**
 * This function just returns data from rand().
 * Although predictable and often similar on multiple
 * runs, this does not result in identical random on
 * each run. So do not use this if the results of a
 * test depend on the random data that is generated.
 *
 * rng_state shall be NULL.
 */
static int rnd_std_rand(void *rng_state, unsigned char *output, size_t len)
{
    size_t i;

    if (rng_state != NULL) {
        rng_state  = NULL;
    }

    for (i = 0; i < len; i++) {
        output[i] = (unsigned char)da16x_random();
    }

    return 0;
}

/**
 * Info structure for the pseudo random function
 *
 * Key should be set at the start to a test-unique value.
 * Do not forget endianness!
 * State( v0, v1 ) should be set to zero.
 */
typedef struct {
    uint32_t key[16];
    uint32_t v0, v1;
} rnd_pseudo_info;

/**
 * This function returns random based on a pseudo random function.
 * This means the results should be identical on all systems.
 * Pseudo random is based on the XTEA encryption algorithm to
 * generate pseudorandom.
 *
 * rng_state shall be a pointer to a rnd_pseudo_info structure.
 */
static int rnd_pseudo_rand(void *rng_state, unsigned char *output, size_t len)
{
    rnd_pseudo_info *info = (rnd_pseudo_info *) rng_state;
    uint32_t i, *k, sum, delta = 0x9E3779B9;
    unsigned char result[4], *out = output;

    if (rng_state == NULL) {
        return (rnd_std_rand(NULL, output, len));
    }

    k = info->key;

    while (len > 0) {
        size_t use_len = ( len > 4 ) ? 4 : len;
        sum = 0;

        for (i = 0; i < 32; i++) {
            info->v0 += (((info->v1 << 4) ^ (info->v1 >> 5 ) ) + info->v1) ^
                        (sum + k[sum & 3]);
            sum += delta;
            info->v1 += (((info->v0 << 4) ^ (info->v0 >> 5)) + info->v0) ^
                        (sum + k[( sum >> 11 ) & 3]);
        }

        PUT_UINT32_BE(info->v0, result, 0);
        memcpy(out, result, use_len);
        len -= use_len;
        out += 4;
    }

    return ( 0 );
}

static const char crypto_sample_dhm_params[] =
    "-----BEGIN DH PARAMETERS-----\r\n"
    "MIGHAoGBAJ419DBEOgmQTzo5qXl5fQcN9TN455wkOL7052HzxxRVMyhYmwQcgJvh\r\n"
    "1sa18fyfR9OiVEMYglOpkqVoGLN7qd5aQNNi5W7/C+VBdHTBJcGZJyyP5B3qcz32\r\n"
    "9mLJKudlVudV0Qxk5qUJaPZ/xupz0NyoVpviuiBOI1gNi8ovSXWzAgEC\r\n"
    "-----END DH PARAMETERS-----\r\n";

static const size_t crypto_sample_dhm_params_len = sizeof(
            crypto_sample_dhm_params);

int crypto_sample_dhm_parse_dhm()
{
    int ret;
    mbedtls_dhm_context *dhm = NULL;	// The DHM context structure.

    PRINTF("* DHM parameter load: ");

    dhm = (mbedtls_dhm_context *)crypto_sample_calloc(1, sizeof(mbedtls_dhm_context));
    if (!dhm) {
        PRINTF("failed - [%s] Failed to allocate DHM context\r\n", __func__);
        return -1;
    }

    // Initialize the DHM context.
    mbedtls_dhm_init(dhm);

    // Parse DHM parameters in PEM or DER format.
    ret = mbedtls_dhm_parse_dhm(dhm, (const unsigned char *)crypto_sample_dhm_params,
                                crypto_sample_dhm_params_len);
    if (ret) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF( "passed\r\n");

cleanup:

    // Free and clear the components of a DHM context.
    mbedtls_dhm_free(dhm);

    if (dhm) {
        crypto_sample_free(dhm);
    }

    return ret;
}

#define	DHM_BUF_SIZE	(sizeof(unsigned char)*1000)

typedef struct {
    char *title;
    int radix_P;
    char *input_P;
    int radix_G;
    char *input_G;
} crypto_sample_dhm_do_dhm_type;

static const crypto_sample_dhm_do_dhm_type crypto_sample_dhm_do_dhm_list[] = {
    {
        "Diffie-Hellman full exchange",
        10,
        "93450983094850938450983409623982317398171298719873918739182739712938719287391879381271",
        10,
        "9345098309485093845098340962223981329819812792137312973297123912791271"
    },
    { NULL, 0, NULL, 0, NULL }
};

int crypto_sample_dhm_do_dhm(char *title, int radix_P, char *input_P,
                             int radix_G, char *input_G)
{
    mbedtls_dhm_context ctx_srv;
    mbedtls_dhm_context ctx_cli;
    unsigned char *ske = NULL;
    unsigned char *p = NULL;
    unsigned char *pub_cli = NULL;
    unsigned char *sec_srv = NULL;
    unsigned char *sec_cli = NULL;
    size_t ske_len = 0;
    size_t pub_cli_len = 0;
    size_t sec_srv_len;
    size_t sec_cli_len;
    int x_size, ret = -1;
    rnd_pseudo_info rnd_info;

    PRINTF("* %s: ", title);

    // Initialize the DHM context.
    mbedtls_dhm_init(&ctx_srv);
    mbedtls_dhm_init(&ctx_cli);

    p = ske = (unsigned char *)crypto_sample_calloc(1, DHM_BUF_SIZE);
    if (!ske) {
        PRINTF("failed - [%s] Failed to allocate ske buffer\r\n", __func__);
        goto cleanup;
    }

    pub_cli = (unsigned char *)crypto_sample_calloc(1, DHM_BUF_SIZE);
    if (!pub_cli) {
        PRINTF("failed - [%s] Failed to allocate pub client buffer\r\n", __func__);
        goto cleanup;
    }

    sec_srv = (unsigned char *)crypto_sample_calloc(1, DHM_BUF_SIZE);
    if (!sec_srv) {
        PRINTF("failed - [%s] Failed to allocate server sec buffer\r\n", __func__);
        goto cleanup;
    }

    sec_cli = (unsigned char *)crypto_sample_calloc(1, DHM_BUF_SIZE);
    if (!sec_cli) {
        PRINTF("failed - [%s] Failed to allocate client sec buffer\r\n", __func__);
        goto cleanup;
    }

    memset(&rnd_info, 0x00, sizeof(rnd_pseudo_info));

    // Set parameters
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&ctx_srv.P, radix_P, input_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&ctx_srv.G, radix_G, input_G));

    x_size = mbedtls_mpi_size(&ctx_srv.P);
    pub_cli_len = x_size;

    /* Generate a DHM key pair and export its public part together
     * with the DHM parameters in the format.
     */
    ret = mbedtls_dhm_make_params(&ctx_srv, x_size, ske, &ske_len,
                                  &rnd_pseudo_rand, &rnd_info);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate a DHM key pair(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    ske[ske_len++] = 0;
    ske[ske_len++] = 0;

    // Parse the DHM parameters (DHM modulus, generator, and public key)
    ret = mbedtls_dhm_read_params(&ctx_cli, &p, ske + ske_len);
    if (ret) {
        PRINTF("failed - [%s] Failed to parse the DHM parameters(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Create a DHM key pair and export the raw public key in big-endian format.
    ret = mbedtls_dhm_make_public(&ctx_cli, x_size, pub_cli, pub_cli_len,
                                  &rnd_pseudo_rand, &rnd_info);
    if (ret) {
        PRINTF("failed - [%s] Failed to create a DHM key pair(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Import the raw public value of the peer.
    ret = mbedtls_dhm_read_public(&ctx_srv, pub_cli, pub_cli_len);
    if (ret) {
        PRINTF("failed - [%s] Failed to import the raw public value(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Derive and export the shared secret (G^Y)^X mod P.
    ret = mbedtls_dhm_calc_secret(&ctx_srv, sec_srv, DHM_BUF_SIZE,
                                  &sec_srv_len, &rnd_pseudo_rand, &rnd_info);
    if (ret) {
        PRINTF("failed - [%s] Failed to derive the shared secret(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Derive and export the shared secret (G^Y)^X mod P.
    ret = mbedtls_dhm_calc_secret(&ctx_cli, sec_cli, DHM_BUF_SIZE, &sec_cli_len,
                                  NULL, NULL);
    if (ret) {
        PRINTF("failed - [%s] Failed to derive the shared secret(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Check result
    if (sec_srv_len == 0) {
        PRINTF("failed - [%s] sec srv length is 0\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    if (sec_srv_len != sec_cli_len) {
        PRINTF("failed - [%s] Not matched sec length(%d:%d)\r\n",
               __func__, sec_srv_len, sec_cli_len);
        ret = -1;
        goto cleanup;
    }


    if ((ret = memcmp(sec_srv, sec_cli, sec_srv_len)) != 0) {
        PRINTF("failed - [%s] Not matched sec result\r\n", __func__);
        goto cleanup;
    }

cleanup:

    // Free and clear the components of a DHM context.
    mbedtls_dhm_free(&ctx_srv);
    mbedtls_dhm_free(&ctx_cli);

    if (ske) {
        crypto_sample_free(ske);
    }

    if (pub_cli) {
        crypto_sample_free(pub_cli);
    }

    if (sec_srv) {
        crypto_sample_free(sec_srv);
    }

    if (sec_cli) {
        crypto_sample_free(sec_cli);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

void crypto_sample_dhm(void *param)
{
    DA16X_UNUSED_ARG(param);

    int ret = 0;
    int idx = 0;

    ret = crypto_sample_dhm_parse_dhm();
    if (ret == 0) {
        for (idx = 0 ; crypto_sample_dhm_do_dhm_list[idx].title != NULL ; idx++) {
            ret = crypto_sample_dhm_do_dhm(crypto_sample_dhm_do_dhm_list[idx].title,
                                           crypto_sample_dhm_do_dhm_list[idx].radix_P,
                                           crypto_sample_dhm_do_dhm_list[idx].input_P,
                                           crypto_sample_dhm_do_dhm_list[idx].radix_G,
                                           crypto_sample_dhm_do_dhm_list[idx].input_G);
            if (ret) {
                break;
            }
        }
    }

    return ;
}

/* EOF */
