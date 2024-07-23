/**
 ****************************************************************************************
 *
 * @file crypto_sample_pk.c
 *
 * @brief Sample app of Public Key abstraction layer.
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
#include "mbedtls/entropy.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecp.h"

#define	CC312_RSA_MARGIN(x)	(((x)+0x7F) & (~0x7F))
//#define CC312_ENTROPY_MARGIN(x)	(((x)+0x1FF) & (~0x1FF))

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)								\
    {														\
        (b)[(i)    ] = (unsigned char) ( (n) >> 24 );		\
        (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );		\
        (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );		\
        (b)[(i) + 3] = (unsigned char) ( (n)       );		\
    }
#endif // (PUT_UINT32_BE)

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
        output[i] = (UINT8)da16x_random();
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
static int rnd_pseudo_rand( void *rng_state, unsigned char *output, size_t len )
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

typedef struct {
    mbedtls_pk_type_t type;
    int size;
    int len;
    char *name;
} crypto_sample_pk_utils_type;

const crypto_sample_pk_utils_type crypto_sample_pk_utils_list[] = {
    { MBEDTLS_PK_RSA, 512, 64, "RSA" },
    { MBEDTLS_PK_ECKEY, 192, 24, "EC" },
    { MBEDTLS_PK_ECKEY_DH, 192, 24, "EC_DH" },
    { MBEDTLS_PK_ECDSA, 192, 24, "ECDSA" },
    { MBEDTLS_PK_NONE, 0, 0, NULL }
};

#define RSA_KEY_SIZE 512
#define RSA_KEY_LEN   64

int crypto_sample_pk_genkey(mbedtls_pk_context *pk)
{
    int ret = -1;
    mbedtls_entropy_context *entropy = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg = NULL;

    entropy = (mbedtls_entropy_context *)crypto_sample_calloc(1, sizeof(mbedtls_entropy_context));
    if (!entropy) {
        PRINTF("[%s] Failed to allocate Entropy context\r\n", __func__);
        goto cleanup;
    }

    ctr_drbg = (mbedtls_ctr_drbg_context *)crypto_sample_calloc(1,
               sizeof(mbedtls_ctr_drbg_context));
    if (!ctr_drbg) {
        PRINTF("[%s] Failed to allocate CTR_DRBG context\r\n", __func__);
        goto cleanup;
    }

    // Initialize the entropy context.
    mbedtls_entropy_init(entropy);

    // Initialize the CTR_DRBG context.
    mbedtls_ctr_drbg_init(ctr_drbg);

    // Seed and sets up the CTR_DRBG entropy source for future reseeds.
    mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy, NULL, 0);

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
    if (mbedtls_pk_get_type(pk) == MBEDTLS_PK_RSA) {
        // Generate the RSA key pair.
        ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*pk), rnd_std_rand, ctr_drbg,
                                  RSA_KEY_SIZE, 3);

        if (ret) {
            PRINTF("[%s] Failed to generate the RSA key pair(0x%x)\r\n", __func__, -ret);
            goto cleanup;
        }
    }
#endif // (MBEDTLS_RSA_C) && (MBEDTLS_GENPRIME)

#if defined(MBEDTLS_ECP_C)
    if ((mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECKEY)
            || (mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECKEY_DH)
            || (mbedtls_pk_get_type(pk) == MBEDTLS_PK_ECDSA)) {

        // Set a group using well-known domain parameters.
        ret = mbedtls_ecp_group_load(&mbedtls_pk_ec(*pk)->grp,
                                     MBEDTLS_ECP_DP_SECP192R1);
        if (ret) {
            PRINTF("[%s] Failed to set ec group(0x%x)\r\n", __func__, -ret);
            goto cleanup;
        }

        // Generate key pair, wrapper for conventional base point
        ret = mbedtls_ecp_gen_keypair(&mbedtls_pk_ec(*pk)->grp, &mbedtls_pk_ec(*pk)->d,
                                      &mbedtls_pk_ec(*pk)->Q, rnd_std_rand, ctr_drbg);
        if (ret) {
            PRINTF("[%s] Failed to EC generate key pair(0x%x)\r\n", __func__, -ret);
            goto cleanup;
        }
    }
#endif // (MBEDTLS_ECP_C)

cleanup:

    mbedtls_ctr_drbg_free(ctr_drbg);
    mbedtls_entropy_free(entropy);

    if (entropy) {
        crypto_sample_free(entropy);
    }

    if (ctr_drbg) {
        crypto_sample_free(ctr_drbg);
    }

    return ret;
}

int crypto_sample_pk_utils(mbedtls_pk_type_t type, int size, int len,
                           char *name)
{
    int ret = -1;
    mbedtls_pk_context pk;

    PRINTF(">>> %s: ", name);

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pk);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(type));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate key pair by the type.
    ret = crypto_sample_pk_genkey(&pk);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Get the key type.
    if (mbedtls_pk_get_type(&pk) != type) {
        PRINTF("failed - [%s] Failed to get the key type\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Tell if a context can do the operation given by type.
    if (!mbedtls_pk_can_do(&pk, type)) {
        PRINTF("failed - [%s] Not able to do the operation\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Get the size in bits of the underlying key.
    if (mbedtls_pk_get_bitlen(&pk) != (unsigned)size) {
        PRINTF("failed - [%s] Failed to get size\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Get the length in bytes of the underlying key.
    if (mbedtls_pk_get_len(&pk) != (unsigned)len) {
        PRINTF("failed - [%s] Failed to get the length\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Access the type name.
    if ((ret = strcmp(mbedtls_pk_get_name(&pk), name)) != 0) {
        PRINTF("failed - [%s] Not matched the type name\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

cleanup:

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free(&pk);

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

typedef struct {
    char *title;
    char *message_hex_string;
    mbedtls_md_type_t digest;
    int mod;
    int radix_N;
    char *input_N;
    int radix_E;
    char *input_E;
    char *result_hex_str;
    int result;
} crypto_sample_pk_rsa_verify_test_vec_type;

const crypto_sample_pk_rsa_verify_test_vec_type
crypto_sample_pk_rsa_verify_test_vec_list[] = {
    {
        "RSA verify test vector #1 (good)",
        "206ef4bf396c6087f8229ef196fd35f37ccb8de5efcdb238f20d556668f114257a11fbe038464a67830378e62ae9791453953dac1dbd7921837ba98e84e856eb80ed9487e656d0b20c28c8ba5e35db1abbed83ed1c7720a97701f709e3547a4bfcabca9c89c57ad15c3996577a0ae36d7c7b699035242f37954646c1cd5c08ac",
        MBEDTLS_MD_SHA1,
        1024,
        16,
        "e28a13548525e5f36dccb24ecb7cc332cc689dfd64012604c9c7816d72a16c3f5fcdc0e86e7c03280b1c69b586ce0cd8aec722cc73a5d3b730310bf7dfebdc77ce5d94bbc369dc18a2f7b07bd505ab0f82224aef09fdc1e5063234255e0b3c40a52e9e8ae60898eb88a766bdd788fe9493d8fd86bcdd2884d5c06216c65469e5",
        16,
        "3",
        "5abc01f5de25b70867ff0c24e222c61f53c88daf42586fddcd56f3c4588f074be3c328056c063388688b6385a8167957c6e5355a510e005b8a851d69c96b36ec6036644078210e5d7d326f96365ee0648882921492bc7b753eb9c26cdbab37555f210df2ca6fec1b25b463d38b81c0dcea202022b04af5da58aa03d77be949b7",
        0
    },
    {
        "RSA verify test vector #2 (bad)",
        "d6248c3e96b1a7e5fea978870fcc4c9786b4e5156e16b7faef4557d667f730b8bc4c784ef00c624df5309513c3a5de8ca94c2152e0459618666d3148092562ebc256ffca45b27fd2d63c68bd5e0a0aefbe496e9e63838a361b1db6fc272464f191490bf9c029643c49d2d9cd08833b8a70b4b3431f56fb1eb55ccd39e77a9c92",
        MBEDTLS_MD_SHA1,
        1024,
        16,
        "e28a13548525e5f36dccb24ecb7cc332cc689dfd64012604c9c7816d72a16c3f5fcdc0e86e7c03280b1c69b586ce0cd8aec722cc73a5d3b730310bf7dfebdc77ce5d94bbc369dc18a2f7b07bd505ab0f82224aef09fdc1e5063234255e0b3c40a52e9e8ae60898eb88a766bdd788fe9493d8fd86bcdd2884d5c06216c65469e5",
        16,
        "3",
        "3203b7647fb7e345aa457681e5131777f1adc371f2fba8534928c4e52ef6206a856425d6269352ecbf64db2f6ad82397768cafdd8cd272e512d617ad67992226da6bc291c31404c17fd4b7e2beb20eff284a44f4d7af47fd6629e2c95809fa7f2241a04f70ac70d3271bb13258af1ed5c5988c95df7fa26603515791075feccd",
        MBEDTLS_ERR_RSA_VERIFY_FAILED
    },
    { NULL, NULL, MBEDTLS_MD_NONE, 0, 0, NULL, 0, NULL, NULL, 0 }
};


int crypto_sample_pk_rsa_verify_test_vec(char *title, char *message_hex_string,
        mbedtls_md_type_t digest, int mod,
        int radix_N, char *input_N,
        int radix_E, char *input_E,
        char *result_hex_str, int result)
{
    unsigned char *message_str = NULL;
    unsigned char *hash_result = NULL;
    unsigned char *result_str = NULL;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;
    int msg_len = 0;
    int ret = -1;

    PRINTF(">>> %s: ", title);

    message_str = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!message_str) {
        PRINTF("failed - [%s] Failed to allocate message buffer\r\n", __func__);
        goto cleanup;
    }

    hash_result = crypto_sample_calloc(1000, sizeof(unsigned char));
    if (!hash_result) {
        PRINTF("failed - [%s] Failed to allocate hash buffer\r\n", __func__);
        goto cleanup;
    }

    result_str = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!result_str) {
        PRINTF("failed - [%s] Failed to allocate result buffer\r\n", __func__);
        goto cleanup;
    }

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pk);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Quick access to an RSA context inside a PK context.
    rsa = mbedtls_pk_rsa(pk);
    if (!rsa) {
        PRINTF("failed - [%s] Failed to access PSA context inside a PK context\r\n",
               __func__);
        ret = -1;
        goto cleanup;
    }

    rsa->len = mod / 8;

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa->N, radix_N, input_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa->E, radix_E, input_E));

    msg_len = unhexify(message_str, message_hex_string);

    unhexify(result_str, result_hex_str);

    // Get the message-digest information associated with the given digest type.
    if (mbedtls_md_info_from_type(digest) != NULL) {
        /* Calculates the message-digest of a buffer,
         * with respect to a configurable message-digest algorithm in a single call.
         */
        ret = mbedtls_md(mbedtls_md_info_from_type(digest), message_str, msg_len,
                         hash_result);
        if (ret) {
            PRINTF("failed - [%s] Failed to calculate the message-digest(0x%x)\r\n",
                   __func__, -ret);
            goto cleanup;
        }
    } else {
        PRINTF("failed - [%s] Failed to get message-digest information\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Verify signature (including padding if relevant) & Check result with expected result.
    ret = mbedtls_pk_verify(&pk, digest, hash_result, 0, result_str,
                            mbedtls_pk_get_len(&pk));
    if (ret != result) {
        PRINTF("failed - [%s] Failed to verify signature(0x%x)\r\n", __func__, -ret);
        ret = -1;
        goto cleanup;
    }

    ret = 0;

cleanup:

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free(&pk);

    if (message_str) {
        crypto_sample_free(message_str);
    }

    if (hash_result) {
        crypto_sample_free(hash_result);
    }

    if (result_str) {
        crypto_sample_free(result_str);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

typedef struct {
    char *title;
    mbedtls_pk_type_t type;
    int sign_ret;
    int verify_ret;
} crypto_sample_pk_sign_verify_type;

const crypto_sample_pk_sign_verify_type crypto_sample_pk_sign_verify_list[] = {
    { "ECDSA", MBEDTLS_PK_ECDSA, 0, 0 },
    { "EC(DSA)", MBEDTLS_PK_ECKEY, 0, 0 },
    {
        "EC_DH (no)",
        MBEDTLS_PK_ECKEY_DH,
        MBEDTLS_ERR_PK_TYPE_MISMATCH,
        MBEDTLS_ERR_PK_TYPE_MISMATCH
    },
    { "RSA", MBEDTLS_PK_RSA, 0, 0 },
    { NULL, MBEDTLS_PK_NONE, 0, 0 }
};

int crypto_sample_pk_sign_verify(char *title, mbedtls_pk_type_t type,
                                 int sign_ret,
                                 int verify_ret)
{
    mbedtls_pk_context pk;
    unsigned char *hash = NULL;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    int ret = -1;

    PRINTF(">>> %s: ", title);

    hash = crypto_sample_calloc(64, sizeof(unsigned char));
    if (!hash) {
        PRINTF("failed - [%s] Failed to allocate hash buffer\r\n", __func__);
        goto cleanup;
    }

    sig = crypto_sample_calloc(5000, sizeof(unsigned char));
    if (!sig) {
        PRINTF("failed - [%s] Failed to allocate sig buffer\r\n", __func__);
        goto cleanup;
    }

    memset(hash, 0x2a, 64);

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pk);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(type));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Generate key pair by the type.
    ret = crypto_sample_pk_genkey(&pk);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Make signature, including padding if relevant and Check result with expected result.
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 64, sig, &sig_len,
                          rnd_std_rand,
                          NULL);
    if (ret != sign_ret) {
        PRINTF("failed - [%s] Faield to make signature(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    } else {
        ret = 0;
    }

    // Verify signature (including padding if relevant) and Check result with expected result.
    ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 64, sig, sig_len);
    if (ret != verify_ret) {
        PRINTF("failed - [%s] Failed to verify signature(0x%x)\r\n", __func__, -ret);
        ret = -1;
        goto cleanup;
    } else {
        ret = 0;
    }

cleanup:

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free(&pk);

    if (hash) {
        crypto_sample_free(hash);
    }

    if (sig) {
        crypto_sample_free(sig);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

typedef struct {
    char *title;
    char *cipher_hex;
    int mod;
    int radix_P;
    char *input_P;
    int radix_Q;
    char *input_Q;
    int radix_N;
    char *input_N;
    int radix_E;
    char *input_E;
    char *clear_hex;
    int result;
} crypto_sample_pk_rsa_decrypt_type;

const crypto_sample_pk_rsa_decrypt_type crypto_sample_pk_rsa_decrypt_list[] = {
    {
        "RSA decrypt test vector #1",
        "a42eda41e56235e666e7faaa77100197f657288a1bf183e4820f0c37ce2c456b960278d6003e0bbcd4be4a969f8e8fd9231e1f492414f00ed09844994c86ec32db7cde3bec7f0c3dbf6ae55baeb2712fa609f5fc3207a824eb3dace31849cd6a6084318523912bccb84cf42e3c6d6d1685131d69bb545acec827d2b0dfdd5568b7dcc4f5a11d6916583fefa689d367f8c9e1d95dcd2240895a9470b0c1730f97cd6e8546860bd254801769f54be96e16362ddcbf34d56035028890199e0f48db38642cb66a4181e028a6443a404fea284ce02b4614b683367d40874e505611d23142d49f06feea831d52d347b13610b413c4efc43a6de9f0b08d2a951dc503b6",
        2048,
        16,
        "e79a373182bfaa722eb035f772ad2a9464bd842de59432c18bbab3a7dfeae318c9b915ee487861ab665a40bd6cda560152578e8579016c929df99fea05b4d64efca1d543850bc8164b40d71ed7f3fa4105df0fb9b9ad2a18ce182c8a4f4f975bea9aa0b9a1438a27a28e97ac8330ef37383414d1bd64607d6979ac050424fd17",
        16,
        "c6749cbb0db8c5a177672d4728a8b22392b2fc4d3b8361d5c0d5055a1b4e46d821f757c24eef2a51c561941b93b3ace7340074c058c9bb48e7e7414f42c41da4cccb5c2ba91deb30c586b7fb18af12a52995592ad139d3be429add6547e044becedaf31fa3b39421e24ee034fbf367d11f6b8f88ee483d163b431e1654ad3e89",
        16,
        "b38ac65c8141f7f5c96e14470e851936a67bf94cc6821a39ac12c05f7c0b06d9e6ddba2224703b02e25f31452f9c4a8417b62675fdc6df46b94813bc7b9769a892c482b830bfe0ad42e46668ace68903617faf6681f4babf1cc8e4b0420d3c7f61dc45434c6b54e2c3ee0fc07908509d79c9826e673bf8363255adb0add2401039a7bcd1b4ecf0fbe6ec8369d2da486eec59559dd1d54c9b24190965eafbdab203b35255765261cd0909acf93c3b8b8428cbb448de4715d1b813d0c94829c229543d391ce0adab5351f97a3810c1f73d7b1458b97daed4209c50e16d064d2d5bfda8c23893d755222793146d0a78c3d64f35549141486c3b0961a7b4c1a2034f",
        16,
        "3",
        "4E636AF98E40F3ADCFCCB698F4E80B9F",
        0
    },
    {
        "RSA decrypt test vector #2",
        "a42eda41e56235e666e7faaa77100197f657288a1bf183e4820f0c37ce2c456b960278d6003e0bbcd4be4a969f8e8fd9231e1f492414f00ed09844994c86ec32db7cde3bec7f0c3dbf6ae55baeb2712fa609f5fc3207a824eb3dace31849cd6a6084318523912bccb84cf42e3c6d6d1685131d69bb545acec827d2b0dfdd5568b7dcc4f5a11d6916583fefa689d367f8c9e1d95dcd2240895a9470b0c1730f97cd6e8546860bd254801769f54be96e16362ddcbf34d56035028890199e0f48db38642cb66a4181e028a6443a404feb284ce02b4614b683367d40874e505611d23142d49f06feea831d52d347b13610b413c4efc43a6de9f0b08d2a951dc503b6",
        2048,
        16,
        "e79a373182bfaa722eb035f772ad2a9464bd842de59432c18bbab3a7dfeae318c9b915ee487861ab665a40bd6cda560152578e8579016c929df99fea05b4d64efca1d543850bc8164b40d71ed7f3fa4105df0fb9b9ad2a18ce182c8a4f4f975bea9aa0b9a1438a27a28e97ac8330ef37383414d1bd64607d6979ac050424fd17",
        16,
        "c6749cbb0db8c5a177672d4728a8b22392b2fc4d3b8361d5c0d5055a1b4e46d821f757c24eef2a51c561941b93b3ace7340074c058c9bb48e7e7414f42c41da4cccb5c2ba91deb30c586b7fb18af12a52995592ad139d3be429add6547e044becedaf31fa3b39421e24ee034fbf367d11f6b8f88ee483d163b431e1654ad3e89",
        16,
        "b38ac65c8141f7f5c96e14470e851936a67bf94cc6821a39ac12c05f7c0b06d9e6ddba2224703b02e25f31452f9c4a8417b62675fdc6df46b94813bc7b9769a892c482b830bfe0ad42e46668ace68903617faf6681f4babf1cc8e4b0420d3c7f61dc45434c6b54e2c3ee0fc07908509d79c9826e673bf8363255adb0add2401039a7bcd1b4ecf0fbe6ec8369d2da486eec59559dd1d54c9b24190965eafbdab203b35255765261cd0909acf93c3b8b8428cbb448de4715d1b813d0c94829c229543d391ce0adab5351f97a3810c1f73d7b1458b97daed4209c50e16d064d2d5bfda8c23893d755222793146d0a78c3d64f35549141486c3b0961a7b4c1a2034f",
        16,
        "3",
        "4E636AF98E40F3ADCFCCB698F4E80B9F",
        MBEDTLS_ERR_RSA_INVALID_PADDING
    },
    { NULL, NULL, 0, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, 0 }
};

int crypto_sample_pk_rsa_decrypt_test_vec(char *title, char *cipher_hex,
        int mod,
        int radix_P, char *input_P,
        int radix_Q, char *input_Q,
        int radix_N, char *input_N,
        int radix_E, char *input_E,
        char *clear_hex, int result)
{
    unsigned char *clear = NULL;
    unsigned char *output = NULL;
    unsigned char *cipher = NULL;
    size_t clear_len = 0;
    size_t olen = 0;
    size_t cipher_len = 0;
    rnd_pseudo_info *rnd_info = NULL;
    mbedtls_mpi N, P, Q, E;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;
    int ret = -1;

    PRINTF(">>> %s: ", title);

    rnd_info = crypto_sample_calloc(1, sizeof(rnd_pseudo_info));
    if (!rnd_info) {
        PRINTF("[%s] Failed to allocate random pseudo info buffer\r\n", __func__);
        goto cleanup;
    }

    clear = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!clear) {
        PRINTF("[%s] Failed to allocate clear buffer\r\n", __func__);
        goto cleanup;
    }

    output = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!output) {
        PRINTF("[%s] Failed to allocate output buffer\r\n", __func__);
        goto cleanup;
    }

    cipher = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!cipher) {
        PRINTF("[%s] Failed to allocate cipher buffer\r\n", __func__);
        goto cleanup;
    }

    clear_len = unhexify(clear, clear_hex);

    cipher_len = unhexify(cipher, cipher_hex);

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pk);

    // Initialize an MPI context
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&E);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Quick access to an RSA context inside a PK context.
    rsa = mbedtls_pk_rsa(pk);
    if (!rsa) {
        PRINTF("failed - [%s] Failed to access RSA context inside a PK context\r\n",
               __func__);
        ret = -1;
        goto cleanup;
    }

    // Load public key
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&N, radix_N, input_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&E, radix_E, input_E));

    // Load private key
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&P, radix_P, input_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&Q, radix_Q, input_Q));

    // Import a set of core parameters into an RSA context.
    ret = mbedtls_rsa_import(rsa, &N, &P, &Q, NULL, &E);
    if (ret) {
        PRINTF("failed - [%s] Failed to import a set of core parameters(0x%0x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Retrieve the length of RSA modulus in Bytes.
    if (mbedtls_rsa_get_len(rsa) != (size_t)(mod / 8)) {
        PRINTF("failed - [%s] Failed to get RSA length\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Complete an RSA context from a set of imported core parameters.
    ret = mbedtls_rsa_complete(rsa);
    if (ret) {
        PRINTF("failed - [%s] Failed to complete an RSA context(0x%0x)\r\n", __func__,
               -ret);
        goto cleanup;
    }

    // Decrypt message (including padding if relevant).
    ret = mbedtls_pk_decrypt(&pk, cipher, cipher_len, output, &olen,
                             (1000 * sizeof(unsigned char)), rnd_pseudo_rand, rnd_info);
    if (ret != result) {
        PRINTF("failed - [%s]Not matched result with expected result(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    } else {
        if (ret == 0) {
            if (olen != clear_len) {
                PRINTF("failed - [%s] Not matched output length\r\n", __func__);
                ret = -1;
                goto cleanup;
            }

            if ((ret = memcmp(output, clear, olen)) != 0) {
                PRINTF("failed - [%s] Not matched output\r\n", __func__);
                goto cleanup;
            }
        }

        ret = 0;
    }

cleanup:

    // Free the components of an MPI context.
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&E);

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free(&pk);

    if (rnd_info) {
        crypto_sample_free(rnd_info);
    }

    if (clear) {
        crypto_sample_free(clear);
    }

    if (output) {
        crypto_sample_free(output);
    }

    if (cipher) {
        crypto_sample_free(cipher);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

static int crypto_sample_rsa_decrypt_func(void *ctx,
        int mode,
        size_t *olen,
        const unsigned char *input,
        unsigned char *output,
        size_t output_max_len)
{
    return (mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *)ctx, NULL, NULL, mode,
                                      olen,
                                      input, output, output_max_len));
}

static int crypto_sample_rsa_sign_func(void *ctx,
                                       int (*f_rng)(void *, unsigned char *, size_t),
                                       void *p_rng,
                                       int mode,
                                       mbedtls_md_type_t md_alg,
                                       unsigned int hashlen,
                                       const unsigned char *hash,
                                       unsigned char *sig)
{
    return (mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *) ctx, f_rng, p_rng, mode,
                                   md_alg, hashlen, hash, sig));
}

static size_t crypto_sample_rsa_key_len_func(void *ctx)
{
    return (((const mbedtls_rsa_context *)ctx)->len);
}

int crypto_sample_pk_rsa_alt()
{
    /*
     * An rsa_alt context can only do private operations (decrypt, sign).
     * Test it against the public operations (encrypt, verify) of a
     * corresponding rsa context.
     */
    mbedtls_rsa_context *raw = NULL;
    mbedtls_pk_context rsa, alt;
    mbedtls_pk_debug_item *dbg_items = NULL;
    unsigned char *hash = NULL;
    unsigned char *sig = NULL;
    unsigned char *msg = NULL;
    unsigned char *cipher = NULL;
    unsigned char *test = NULL;
    size_t cipher_len, test_len;
    int ret = -1;

    PRINTF("* RSA Alt Test: ");

    raw = crypto_sample_calloc(1, sizeof(mbedtls_rsa_context));
    if (!raw) {
        PRINTF("failed - [%s] Failed to allocate RSA context\r\n", __func__);
        goto cleanup;
    }

    dbg_items = crypto_sample_calloc(10, sizeof(mbedtls_pk_debug_item));
    if (!dbg_items) {
        PRINTF("failed - [%s] Failed to allocate debug items\r\n", __func__);
        goto cleanup;
    }

    hash = crypto_sample_calloc(50, sizeof(unsigned char));
    if (!hash) {
        PRINTF("failed - [%s] Failed to allocate hash buffer\r\n", __func__);
        goto cleanup;
    }

    sig = crypto_sample_calloc(1000, sizeof(unsigned char));
    if (!sig) {
        PRINTF("failed - [%s] Failed to allocate signature buffer\r\n", __func__);
        goto cleanup;
    }

    msg = crypto_sample_calloc(CC312_RSA_MARGIN(50), sizeof(unsigned char));
    if (!msg) {
        PRINTF("failed - [%s] Failed to allocate msg buffer\r\n", __func__);
        goto cleanup;
    }

    cipher = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!cipher) {
        PRINTF("failed - [%s] Failed to allocate cipher buffer\r\n", __func__);
        goto cleanup;
    }

    test = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!test) {
        PRINTF("failed - [%s] Failed to allocate test buffer\r\n", __func__);
        goto cleanup;
    }

    memset(hash, 0x2a, 50);
    memset(msg, 0x2a, 50);

    // Initialize an RSA context.
    mbedtls_rsa_init(raw, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&rsa);
    mbedtls_pk_init(&alt);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&rsa, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Generate key pair by the type.
    ret = crypto_sample_pk_genkey(&rsa);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Copy the components of an RSA context.
    ret = mbedtls_rsa_copy(raw, mbedtls_pk_rsa(rsa));
    if (ret) {
        PRINTF("failed - [%s] Failed to copy RSA context(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Initialize PK RSA_ALT context
    ret = mbedtls_pk_setup_rsa_alt(&alt,
                                   (void *)raw,
                                   crypto_sample_rsa_decrypt_func,
                                   crypto_sample_rsa_sign_func,
                                   crypto_sample_rsa_key_len_func);
    if (ret) {
        PRINTF("failed - [%s] Failed to init RSA ALT context(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    /* Test administrative functions */

    // Tell if a context can do the operation given by type.
    if (!mbedtls_pk_can_do(&alt, MBEDTLS_PK_RSA)) {
        PRINTF("failed - [%s] Not able to do the operation\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Get the size in bits of the underlying key.
    if (mbedtls_pk_get_bitlen(&alt) != RSA_KEY_SIZE) {
        PRINTF("failed - [%s] Failed to get size\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Get the length in bytes of the underlying key.
    if (mbedtls_pk_get_len(&alt) != RSA_KEY_LEN) {
        PRINTF("failed - [%s] Failed to get the length\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Get the key type.
    if (mbedtls_pk_get_type(&alt) != MBEDTLS_PK_RSA_ALT) {
        PRINTF("failed - [%s] Failed to get the key type\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    // Access the type name.
    if ((ret = strcmp(mbedtls_pk_get_name(&alt), "RSA-alt")) != 0 ) {
        PRINTF("failed - [%s] Not matched the type name\r\n", __func__);
        goto cleanup;
    }

    // Encrypt message (including padding if relevant).
    ret = mbedtls_pk_encrypt(&rsa, msg, 50,
                             cipher, &cipher_len,
                             1000, rnd_std_rand, NULL);
    if (ret) {
        PRINTF("failed - [%s] Failed to encrypt message(0x%0x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Decrypt message (including padding if relevant).
    ret = mbedtls_pk_decrypt(&alt, cipher, cipher_len,
                             test, &test_len,
                             1000, rnd_std_rand, NULL);
    if (ret) {
        PRINTF("failed - [%s] Failed to decrypt message(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    if (test_len != 50) {
        PRINTF("failed - [%s] Failed output length\r\n", __func__);
        ret = -1;
        goto cleanup;
    }

    if ((ret = memcmp(test, msg, test_len)) != 0) {
        PRINTF("failed - [%s] Not matched output\r\n", __func__);
        goto cleanup;
    }

    /* Test forbidden operations */
    ret = mbedtls_pk_encrypt(&alt, msg, 50,
                             cipher, &cipher_len,
                             1000, rnd_std_rand, NULL);
    if (ret != MBEDTLS_ERR_PK_TYPE_MISMATCH) {
        PRINTF("failed - [%s] Not matched result(0x%x)\r\n", __func__, -ret);
        ret = -1;
        goto cleanup;
    } else {
        ret = 0;
    }

cleanup:

    // Free the components of an RSA key.
    mbedtls_rsa_free(raw);

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free(&rsa);
    mbedtls_pk_free(&alt);

    if (raw) {
        crypto_sample_free(raw);
    }

    if (dbg_items) {
        crypto_sample_free(dbg_items);
    }

    if (hash) {
        crypto_sample_free(hash);
    }

    if (sig) {
        crypto_sample_free(sig);
    }

    if (msg) {
        crypto_sample_free(msg);
    }

    if (cipher) {
        crypto_sample_free(cipher);
    }

    if (test) {
        crypto_sample_free(test);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

typedef struct {
    char *title;
    char *message_hex_string;
    mbedtls_md_type_t digest;
    int mod;
    int radix_N;
    char *input_N;
    int radix_E;
    char *input_E;
    char *result_hex_str;
    int pk_type;
    int mgf1_hash_id;
    int salt_len;
    int result;
} crypto_sample_pk_rsa_verify_ext_type;

const crypto_sample_pk_rsa_verify_ext_type
crypto_sample_pk_rsa_verify_ext_list[] = {
    {
        "Verify ext RSA #2 (PKCS1 v2.1, salt_len = ANY, wrong message)",
        "54657374206d657373616766",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA256,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_RSA_VERIFY_FAILED
    },
    {
        "Verify ext RSA #3 (PKCS1 v2.1, salt_len = 0, OK)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "7fc506d26ca3b22922a1ce39faaedd273161b82d9443c56f1a034f131ae4a18cae1474271cb4b66a17d9707ca58b0bdbd3c406b7e65bbcc9bbbce94dc45de807b4989b23b3e4db74ca29298137837eb90cc83d3219249bc7d480fceaf075203a86e54c4ecfa4e312e39f8f69d76534089a36ed9049ca9cfd5ab1db1fa75fe5c8",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA256,
        0,
        0
    },
    {
        "Verify ext RSA #4 (PKCS1 v2.1, salt_len = max, OK)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA256,
        94,
        0
    },
    {
        "Verify ext RSA #5 (PKCS1 v2.1, wrong salt_len)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA256,
        32,
        MBEDTLS_ERR_RSA_INVALID_PADDING
    },
    {
        "Verify ext RSA #6 (PKCS1 v2.1, MGF1 alg != MSG hash alg)",
        "c0719e9a8d5d838d861dc6f675c899d2b309a3a65bb9fe6b11e5afcbf9a2c0b1",
        MBEDTLS_MD_NONE,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA256,
        MBEDTLS_RSA_SALT_LEN_ANY,
        0
    },
    {
        "Verify ext RSA #7 (PKCS1 v2.1, wrong MGF1 alg != MSG hash alg)",
        "c0719e9a8d5d838d861dc6f675c899d2b309a3a65bb9fe6b11e5afcbf9a2c0b1",
        MBEDTLS_MD_NONE,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        MBEDTLS_MD_SHA1,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_RSA_INVALID_PADDING
    },
    {
        "Verify ext RSA #8 (PKCS1 v2.1, RSASSA-PSS without options)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSASSA_PSS,
        -1,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_PK_BAD_INPUT_DATA
    },
    {
        "Verify ext RSA #9 (PKCS1 v1.5, RSA with options)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSA,
        MBEDTLS_MD_SHA256,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_PK_BAD_INPUT_DATA
    },
    {
        "Verify ext RSA #10 (PKCS1 v1.5, RSA without options)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_RSA,
        -1,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_RSA_VERIFY_FAILED
    },
    {
        "Verify ext RSA #11 (PKCS1 v2.1, asking for ECDSA)",
        "54657374206d657373616765",
        MBEDTLS_MD_SHA256,
        1024,
        16,
        "00dd118a9f99bab068ca2aea3b6a6d5997ed4ec954e40deecea07da01eaae80ec2bb1340db8a128e891324a5c5f5fad8f590d7c8cacbc5fe931dafda1223735279461abaa0572b761631b3a8afe7389b088b63993a0a25ee45d21858bab9931aedd4589a631b37fcf714089f856549f359326dd1e0e86dde52ed66b4a90bda4095",
        16,
        "010001",
        "0d2bdb0456a3d651d5bd48a4204493898f72cf1aaddd71387cc058bc3f4c235ea6be4010fd61b28e1fbb275462b53775c04be9022d38b6a2e0387dddba86a3f8554d2858044a59fddbd594753fc056fe33c8daddb85dc70d164690b1182209ff84824e0be10e35c379f2f378bf176a9f7cb94d95e44d90276a298c8810f741c9",
        MBEDTLS_PK_ECDSA,
        -1,
        MBEDTLS_RSA_SALT_LEN_ANY,
        MBEDTLS_ERR_PK_TYPE_MISMATCH
    },
    {
        "Verify ext RSA #12 (PKCS1 v1.5, good)",
        "206ef4bf396c6087f8229ef196fd35f37ccb8de5efcdb238f20d556668f114257a11fbe038464a67830378e62ae9791453953dac1dbd7921837ba98e84e856eb80ed9487e656d0b20c28c8ba5e35db1abbed83ed1c7720a97701f709e3547a4bfcabca9c89c57ad15c3996577a0ae36d7c7b699035242f37954646c1cd5c08ac",
        MBEDTLS_MD_SHA1,
        1024,
        16,
        "e28a13548525e5f36dccb24ecb7cc332cc689dfd64012604c9c7816d72a16c3f5fcdc0e86e7c03280b1c69b586ce0cd8aec722cc73a5d3b730310bf7dfebdc77ce5d94bbc369dc18a2f7b07bd505ab0f82224aef09fdc1e5063234255e0b3c40a52e9e8ae60898eb88a766bdd788fe9493d8fd86bcdd2884d5c06216c65469e5",
        16,
        "3",
        "5abc01f5de25b70867ff0c24e222c61f53c88daf42586fddcd56f3c4588f074be3c328056c063388688b6385a8167957c6e5355a510e005b8a851d69c96b36ec6036644078210e5d7d326f96365ee0648882921492bc7b753eb9c26cdbab37555f210df2ca6fec1b25b463d38b81c0dcea202022b04af5da58aa03d77be949b7",
        MBEDTLS_PK_RSA,
        -1,
        MBEDTLS_RSA_SALT_LEN_ANY,
        0
    },
    { NULL, NULL, MBEDTLS_MD_NONE, 0, 0, NULL, 0, NULL, NULL, 0, 0, 0, 0 }
};

int crypto_sample_pk_rsa_verify_ext_test_vec(char *title,
        char *message_hex_string,
        mbedtls_md_type_t digest,
        int mod,
        int radix_N,
        char *input_N,
        int radix_E,
        char *input_E,
        char *result_hex_str,
        int pk_type,
        int mgf1_hash_id,
        int salt_len,
        int result)
{
    DA16X_UNUSED_ARG(pk_type);
    DA16X_UNUSED_ARG(mgf1_hash_id);
    DA16X_UNUSED_ARG(salt_len);
    DA16X_UNUSED_ARG(result);

    unsigned char *message_str = NULL;
    unsigned char *hash_result = NULL;
    unsigned char *result_str = NULL;
    mbedtls_rsa_context *rsa = NULL;
    mbedtls_pk_context pk;
    mbedtls_pk_rsassa_pss_options *pss_opts = NULL;
    int msg_len = 0;
    int ret = -1;

    PRINTF(">>> %s: ", title);

    // Options for RSASSA-PSS signature verification.
    pss_opts = crypto_sample_calloc(1, sizeof(mbedtls_pk_rsassa_pss_options));
    if (!pss_opts) {
        PRINTF("failed - [%s] Failed to allocate PSASSA-PSS option\r\n", __func__);
        goto cleanup;
    }

    message_str = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!message_str) {
        PRINTF("failed - [%s] Failed to allocate message buffer\r\n", __func__);
        goto cleanup;
    }

    hash_result = crypto_sample_calloc(1000, sizeof(unsigned char));
    if (!hash_result) {
        PRINTF("failed - [%s] Failed to allocate hash buffer\r\n", __func__);
        goto cleanup;
    }

    result_str = crypto_sample_calloc(CC312_RSA_MARGIN(1000), sizeof(unsigned char));
    if (!result_str) {
        PRINTF("failed - [%s] Failed to allocate result buffer\r\n", __func__);
        goto cleanup;
    }

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pk);

    /* Initialize a PK context with the information given
     * and allocates the type-specific PK subcontext.
     */
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret) {
        PRINTF("failed - [%s] Failed to setup public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Quick access to an RSA context inside a PK context.
    rsa = mbedtls_pk_rsa(pk);
    if (!rsa) {
        PRINTF("failed - [%s] Failed to access PSA context inside a PK context\r\n",
               __func__);
        ret = -1;
        goto cleanup;
    }

    rsa->len = mod / 8;

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa->N, radix_N, input_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa->E, radix_E, input_E));

    msg_len = unhexify( message_str, message_hex_string );

    unhexify(result_str, result_hex_str);

    // Get the message-digest information associated with the given digest type.
    if (mbedtls_md_info_from_type(digest) != NULL) {
        /* Calculates the message-digest of a buffer,
         * with respect to a configurable message-digest algorithm in a single call.
         */
        ret = mbedtls_md(mbedtls_md_info_from_type(digest), message_str, msg_len,
                         hash_result);
        if (ret) {
            PRINTF("failed - [%s] Failed to calculate the message-digest(0x%x)\r\n",
                   __func__, -ret);
            goto cleanup;
        }
    }

cleanup:

    // Free the components of a mbedtls_pk_context.
    mbedtls_pk_free( &pk );

    if (pss_opts) {
        crypto_sample_free(pss_opts);
    }

    if (message_str) {
        crypto_sample_free(message_str);
    }

    if (hash_result) {
        crypto_sample_free(hash_result);
    }

    if (result_str) {
        crypto_sample_free(result_str);
    }

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

typedef struct {
    char *title;
    char *pub_file;
    char *prv_file;
    int result;
} crypto_sample_pk_check_pair_type;

const crypto_sample_pk_check_pair_type crypto_sample_pk_check_pair_list[] = {
    {
        "Check pair #1 (EC, OK)",
        "-----BEGIN PUBLIC KEY-----\r\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEd3Jlb4FLOZJ51eHxeB+sbwmaPFyh\r\n"
        "sONTUYNLCLZeC1clkM2vj3aTYbzzSs/BHl4HToQmvd4Evm5lOUVElhfeRQ==\r\n"
        "-----END PUBLIC KEY-----\r\n",
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHcCAQEEIEnJqMGMS4hWOMQxzx3xyZQTFgm1gNT9Q6DKsX2y8T7uoAoGCCqGSM49\r\n"
        "AwEHoUQDQgAEd3Jlb4FLOZJ51eHxeB+sbwmaPFyhsONTUYNLCLZeC1clkM2vj3aT\r\n"
        "YbzzSs/BHl4HToQmvd4Evm5lOUVElhfeRQ==\r\n"
        "-----END EC PRIVATE KEY-----\r\n",
        0
    },
    {
        "Check pair #2 (EC, bad)",
        "-----BEGIN PUBLIC KEY-----\r\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEd3Jlb4FLOZJ51eHxeB+sbwmaPFyh\r\n"
        "sONTUYNLCLZeC1clkM2vj3aTYbzzSs/BHl4HToQmvd4Evm5lOUVElhfeRQ==\r\n"
        "-----END PUBLIC KEY-----\r\n",
        "-----BEGIN EC PRIVATE KEY-----\r\n"
        "MHcCAQEEIPEqEyB2AnCoPL/9U/YDHvdqXYbIogTywwyp6/UfDw6noAoGCCqGSM49\r\n"
        "AwEHoUQDQgAEN8xW2XYJHlpyPsdZLf8gbu58+QaRdNCtFLX3aCJZYpJO5QDYIxH/\r\n"
        "6i/SNF1dFr2KiMJrdw1VzYoqDvoByLTt/w==\r\n"
        "-----END EC PRIVATE KEY-----\r\n",
        MBEDTLS_ERR_ECP_BAD_INPUT_DATA
    },
    { NULL, NULL, NULL, 0 }
};

int crypto_sample_pk_check_pair(char *title, char *pub_file, char *prv_file, int result)
{
    mbedtls_pk_context pub, prv, alt;
    int ret = -1;

    PRINTF(">>> %s: ", title);

    // Initialize a mbedtls_pk_context.
    mbedtls_pk_init(&pub);
    mbedtls_pk_init(&prv);
    mbedtls_pk_init(&alt);

    // Parse a public key in PEM or DER format.
    ret = mbedtls_pk_parse_public_key(&pub,
                                      (const unsigned char *)pub_file,
                                      (strlen(pub_file) + 1));
    if (ret) {
        PRINTF("failed - [%s] Failed to parse a public key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Parse a private key in PEM or DER format.
    ret = mbedtls_pk_parse_key(&prv, (const unsigned char *)prv_file,
                               (strlen(prv_file) + 1), NULL, 0);

    if (ret) {
        PRINTF("failed - [%s] Failed to parse private key(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    // Check if a public-private pair of keys matches.
    ret = mbedtls_pk_check_pair(&pub, &prv);
    if (ret != result) {
        ret = -1;
        PRINTF("failed - [%s] Not matched the public-private pair of keys(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    ret = 0;

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
    if (mbedtls_pk_get_type(&prv) == MBEDTLS_PK_RSA) {
        // Initialize an RSA-alt context.
        ret = mbedtls_pk_setup_rsa_alt(&alt, mbedtls_pk_rsa(prv),
                                       crypto_sample_rsa_decrypt_func,
                                       crypto_sample_rsa_sign_func,
                                       crypto_sample_rsa_key_len_func);
        if (ret == 0) {
            ret = mbedtls_pk_check_pair(&pub, &alt);
            if (ret) {
                PRINTF("failed - [%s] Not matched the "
                       "public-private pair of keys(0x%x)\r\n",
                       __func__, -ret);
                goto cleanup;
            }
        } else {
            int ret_status = 0;

            ret_status = mbedtls_pk_check_pair(&pub, &alt);
            if (ret_status != result) {
                PRINTF("failed - [%s] Not matched the "
                       "public-private pair of keys(0x%x)\r\n",
                       __func__, -ret);
                goto cleanup;
            }
        }
    }
#endif // (MBEDTLS_RSA_C) && (MBEDTLS_PK_RSA_ALT_SUPPORT)

cleanup:

    mbedtls_pk_free(&pub);
    mbedtls_pk_free(&prv);
    mbedtls_pk_free(&alt);

    if (ret == 0) {
        PRINTF("passed\r\n");
    }

    return ret;
}

/*
 * Sample Functions for Public Key abstraction layer.
 */
void crypto_sample_pk(void *param)
{
    DA16X_UNUSED_ARG(param);

    int ret = 0;
    int i = 0;

    PRINTF("* PK Information\r\n");

    for (i = 0 ; crypto_sample_pk_utils_list[i].type != MBEDTLS_PK_NONE ; i++) {
        ret = crypto_sample_pk_utils(crypto_sample_pk_utils_list[i].type,
                                     crypto_sample_pk_utils_list[i].size,
                                     crypto_sample_pk_utils_list[i].len,
                                     crypto_sample_pk_utils_list[i].name);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* RSA Verification Test\r\n");

    for (i = 0 ; crypto_sample_pk_rsa_verify_test_vec_list[i].title != NULL ; i++) {
        ret = crypto_sample_pk_rsa_verify_test_vec(
                  crypto_sample_pk_rsa_verify_test_vec_list[i].title,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].message_hex_string,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].digest,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].mod,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].radix_N,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].input_N,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].radix_E,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].input_E,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].result_hex_str,
                  crypto_sample_pk_rsa_verify_test_vec_list[i].result);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Signuature Verification Test\r\n");

    for (i = 0 ; crypto_sample_pk_sign_verify_list[i].title != NULL ; i++) {
        ret = crypto_sample_pk_sign_verify(crypto_sample_pk_sign_verify_list[i].title,
                                           crypto_sample_pk_sign_verify_list[i].type,
                                           crypto_sample_pk_sign_verify_list[i].sign_ret,
                                           crypto_sample_pk_sign_verify_list[i].verify_ret);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* Decryption Test\r\n");

    for (i = 0 ; crypto_sample_pk_rsa_decrypt_list[i].title != NULL ; i++) {
        ret = crypto_sample_pk_rsa_decrypt_test_vec(
                  crypto_sample_pk_rsa_decrypt_list[i].title,
                  crypto_sample_pk_rsa_decrypt_list[i].cipher_hex,
                  crypto_sample_pk_rsa_decrypt_list[i].mod,
                  crypto_sample_pk_rsa_decrypt_list[i].radix_P,
                  crypto_sample_pk_rsa_decrypt_list[i].input_P,
                  crypto_sample_pk_rsa_decrypt_list[i].radix_Q,
                  crypto_sample_pk_rsa_decrypt_list[i].input_Q,
                  crypto_sample_pk_rsa_decrypt_list[i].radix_N,
                  crypto_sample_pk_rsa_decrypt_list[i].input_N,
                  crypto_sample_pk_rsa_decrypt_list[i].radix_E,
                  crypto_sample_pk_rsa_decrypt_list[i].input_E,
                  crypto_sample_pk_rsa_decrypt_list[i].clear_hex,
                  crypto_sample_pk_rsa_decrypt_list[i].result);
        if (ret) {
            goto cleanup;
        }
    }

    ret = crypto_sample_pk_rsa_alt();
    if (ret) {
        goto cleanup;
    }

    PRINTF("* RSA Verification with option Test\r\n");

    for (i = 0; crypto_sample_pk_rsa_verify_ext_list[i].title != NULL; i++) {
        ret = crypto_sample_pk_rsa_verify_ext_test_vec(
                  crypto_sample_pk_rsa_verify_ext_list[i].title,
                  crypto_sample_pk_rsa_verify_ext_list[i].message_hex_string,
                  crypto_sample_pk_rsa_verify_ext_list[i].digest,
                  crypto_sample_pk_rsa_verify_ext_list[i].mod,
                  crypto_sample_pk_rsa_verify_ext_list[i].radix_N,
                  crypto_sample_pk_rsa_verify_ext_list[i].input_N,
                  crypto_sample_pk_rsa_verify_ext_list[i].radix_E,
                  crypto_sample_pk_rsa_verify_ext_list[i].input_E,
                  crypto_sample_pk_rsa_verify_ext_list[i].result_hex_str,
                  crypto_sample_pk_rsa_verify_ext_list[i].pk_type,
                  crypto_sample_pk_rsa_verify_ext_list[i].mgf1_hash_id,
                  crypto_sample_pk_rsa_verify_ext_list[i].salt_len,
                  crypto_sample_pk_rsa_verify_ext_list[i].result);
        if (ret) {
            goto cleanup;
        }
    }

    PRINTF("* PK pair Test\r\n");

    for (i = 0; crypto_sample_pk_check_pair_list[i].title != NULL; i++) {
        ret = crypto_sample_pk_check_pair(crypto_sample_pk_check_pair_list[i].title,
                                          crypto_sample_pk_check_pair_list[i].pub_file,
                                          crypto_sample_pk_check_pair_list[i].prv_file,
                                          crypto_sample_pk_check_pair_list[i].result);
        if (ret) {
            goto cleanup;
        }
    }

cleanup:

    return ;
}

/* EOF */
