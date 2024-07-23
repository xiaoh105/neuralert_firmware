/**
 ****************************************************************************************
 *
 * @file crypto_sample_ecdsa.c
 *
 * @brief Sample app of ECDSA(Elliptic Curve Digital Signature Algorithm).
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
#include "mbedtls/sha256.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"


static void crypto_sample_ecdsa_pubkey_hex_dump(const char *title, mbedtls_ecdsa_context *key)
{
#if defined (CRYPTO_SAMPLE_HEX_DUMP)
    unsigned char buf[300] = {0x00,};
    size_t len;

    if (mbedtls_ecp_point_write_binary(&key->grp, &key->Q,
                                       MBEDTLS_ECP_PF_UNCOMPRESSED, &len,
                                       buf, sizeof(buf)) != 0) {
        return;
    }

    crypto_sample_hex_dump("Public Key", buf, len);
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(key);
#endif // (CRYPTO_SAMPLE_HEX_DUMP)

    return ;
}

int crypto_sample_ecdsa_test()
{
    int ret = -1;

    const char *pers = "crypto_sample_ecdsa";

    mbedtls_ecdsa_context ctx_sign;
    mbedtls_ecdsa_context ctx_verify;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_sha256_context sha256_ctx;

    unsigned char *message = NULL;
    unsigned char *hash = NULL;
    unsigned char *sig = NULL;
    size_t sig_len = 0;

    message = crypto_sample_calloc(100, sizeof(unsigned char));
    if (!message) {
        PRINTF("[%s] Failed to allocate message buffer\r\n", __func__);
        goto cleanup;
    }

    hash = crypto_sample_calloc(32, sizeof(unsigned char));
    if (!hash) {
        PRINTF("[%s] Failed to allocate hash buffer\r\n", __func__);
        goto cleanup;
    }

    sig = crypto_sample_calloc(MBEDTLS_ECDSA_MAX_LEN, sizeof(unsigned char));
    if (!sig) {
        PRINTF("[%s] Failed to allocate signature buffer\r\n", __func__);
        goto cleanup;
    }

    // Initialize an ECDSA context.
    mbedtls_ecdsa_init(&ctx_sign);
    mbedtls_ecdsa_init(&ctx_verify);

    // Initialize the CTR_DRBG context.
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Initialize the SHA-256 context.
    mbedtls_sha256_init(&sha256_ctx);

    // Initialize the entropy context.
    mbedtls_entropy_init(&entropy);

    memset(sig, 0x00, MBEDTLS_ECDSA_MAX_LEN);
    memset(message, 0x25, 100);

    // Generate a key pair for signing
    PRINTF("* Seeding the random number generator: ");

    // Seed and sets up the CTR_DRBG entropy source for future reseeds.
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret) {
        PRINTF("failed - [%s] Failed to seed CTR_DRBG entropy(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    } else {
        PRINTF("passed\r\n");
    }


    PRINTF("* Generating key pair: ");

    // Generate an ECDSA keypair on the given curve.
    ret = mbedtls_ecdsa_genkey(&ctx_sign, MBEDTLS_ECP_DP_SECP192R1,
                               mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate an ECDSA keypair(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    } else {
        PRINTF("passed - (key size: %d bits)\r\n", (int)ctx_sign.grp.pbits);
        crypto_sample_ecdsa_pubkey_hex_dump("\tPublic key: ", &ctx_sign);
    }

    // Compute message hash
    PRINTF("* Computing message hash: ");

    // Start a SHA-256 checksum calculation.
    mbedtls_sha256_starts_ret(&sha256_ctx, 0);

    // Feeds an input buffer into an ongoing SHA-256 checksum calculation.
    mbedtls_sha256_update_ret(&sha256_ctx, message, 100);

    // Finishe the SHA-256 operation, and writes the result to the output buffer.
    mbedtls_sha256_finish(&sha256_ctx, hash);

    PRINTF("passed\r\n");

    crypto_sample_hex_dump("\tHash: ", hash, 32);

    // Sign message hash
    PRINTF("* Signing message hash: ");

    // Compute the ECDSA signature and writes it to a buffer.
    ret = mbedtls_ecdsa_write_signature(&ctx_sign, MBEDTLS_MD_SHA256, hash, 32,
                                        sig, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret) {
        PRINTF("failed - [%s] Failed to compute the ECDSA signature(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    } else {
        PRINTF("passed - (signature length = %u)\r\n", (unsigned int) sig_len);
        crypto_sample_hex_dump("\tSignature: ", sig, sig_len);
    }

    /*
     * Transfer public information to verifying context
     *
     * We could use the same context for verification and signatures, but we
     * chose to use a new one in order to make it clear that the verifying
     * context only needs the public key (Q), and not the private key (d).
     */
    PRINTF("* Preparing verification context: ");

    // Copy the contents of group src into group dst.
    ret = mbedtls_ecp_group_copy(&(ctx_verify.grp), &(ctx_sign.grp));
    if (ret) {
        PRINTF("failed - [%s] Failed to copy the contents of group(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Copy the contents of point Q into point P.
    ret = mbedtls_ecp_copy(&(ctx_verify.Q), &(ctx_sign.Q));
    if (ret) {
        PRINTF("failed - [%s] Failed to copy the contents of point Q(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    PRINTF("passed\r\n");

    // Verify signature
    PRINTF("* Verifying signature: ");

    // Read and verify an ECDSA signature.
    ret = mbedtls_ecdsa_read_signature(&ctx_verify, hash, 32, sig, sig_len);
    if (ret) {
        PRINTF("failed - [%s] Failed to verify an ECDSA signature(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    } else {
        PRINTF("passed\r\n");
    }

cleanup:

    // Free an ECDSA context.
    mbedtls_ecdsa_free(&ctx_verify);
    mbedtls_ecdsa_free(&ctx_sign);

    // Clear CTR_CRBG context data.
    mbedtls_ctr_drbg_free(&ctr_drbg);

    // Free the data in the context.
    mbedtls_entropy_free(&entropy);

    // Clear s SHA-256 context.
    mbedtls_sha256_free(&sha256_ctx);

    if (message) {
        crypto_sample_free(message);
    }

    if (hash) {
        crypto_sample_free(hash);
    }

    if (sig) {
        crypto_sample_free(sig);
    }

    return ret;
}

void crypto_sample_ecdsa(void *param)
{
    DA16X_UNUSED_ARG(param);

    crypto_sample_ecdsa_test();

    return ;
}

/* EOF */
