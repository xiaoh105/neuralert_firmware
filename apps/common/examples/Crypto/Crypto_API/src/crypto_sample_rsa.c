/**
 ****************************************************************************************
 *
 * @file crypto_sample_rsa.c
 *
 * @brief Sample app of RSA PKCS#1.
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
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/bignum.h"

#if defined(MBEDTLS_PKCS1_V15)
#define	CRYPTO_SAMPLE_RSA_MARGIN(x)	(((x)+0x7F) & (~0x7F))

/*
 * Example RSA-1024 keypair, for test purposes
 */
#define KEY_LEN 128

#define RSA_N   "9292758453063D803DD603D5E777D788" \
    "8ED1D5BF35786190FA2F23EBC0848AEA" \
    "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
    "7130B9CED7ACDF54CFC7555AC14EEBAB" \
    "93A89813FBF3C4F8066D2D800F7C38A8" \
    "1AE31942917403FF4946B0A83D3D3E05" \
    "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
    "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
    "66CA472BC44D253102F8B4A9D3BFA750" \
    "91386C0077937FE33FA3252D28855837" \
    "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
    "DF79C5CE07EE72C7F123142198164234" \
    "CABB724CF78B8173B9F880FC86322407" \
    "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
    "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
    "2C01CAD19EA484A87EA4377637E75500" \
    "FCB2005C5C7DD6EC4AC023CDA285D796" \
    "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
    "E211C2B9E5DB1ED0BF61D0D9899620F4" \
    "910E4168387E3C30AA1E00C339A79508" \
    "8452DD96A9A5EA5D9DCA68DA636032AF"

#define RSA_DP  "C1ACF567564274FB07A0BBAD5D26E298" \
    "3C94D22288ACD763FD8E5600ED4A702D" \
    "F84198A5F06C2E72236AE490C93F07F8" \
    "3CC559CD27BC2D1CA488811730BB5725"

#define RSA_DQ  "4959CBF6F8FEF750AEE6977C155579C7" \
    "D8AAEA56749EA28623272E4F7D0592AF" \
    "7C1F1313CAC9471B5C523BFE592F517B" \
    "407A1BD76C164B93DA2D32A383E58357"

#define RSA_QP  "9AE7FBC99546432DF71896FC239EADAE" \
    "F38D18D2B2F0E2DD275AA977E2BF4411" \
    "F5A3B2A5D33605AEBBCCBA7FEB9F2D2F" \
    "A74206CEC169D74BF5A8C50D6F48EA08"

#define PT_LEN  24
#define RSA_PT  "\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF" \
    "\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD"

static int myrand(void *rng_state, unsigned char *output, size_t len)
{
    size_t i;

    if (rng_state != NULL) {
        rng_state  = NULL;
    }

    for (i = 0 ; i < len; ++i) {
        output[i] = (unsigned char)da16x_random();
    }

    return 0;
}

int crypto_sample_rsa_pkcs1()
{
    int ret = -1;
    size_t len = 0;
    mbedtls_rsa_context *rsa = NULL;	// The RSA context structure.
    unsigned char *rsa_plaintext = NULL;
    unsigned char *rsa_decrypted = NULL;
    unsigned char *rsa_ciphertext = NULL;
#if defined(MBEDTLS_SHA1_C)
    unsigned char *sha1sum = NULL;
#endif // (MBEDTLS_SHA1_C)

    rsa = (mbedtls_rsa_context *)crypto_sample_calloc(1, sizeof(mbedtls_rsa_context));
    if (!rsa) {
        PRINTF("[%s] Failed to allocate RSA context\r\n", __func__);
        goto cleanup;
    }

    rsa_plaintext = crypto_sample_calloc(CRYPTO_SAMPLE_RSA_MARGIN(PT_LEN),
                                         sizeof(unsigned char));
    if (!rsa_plaintext) {
        PRINTF("[%s] Failed to allocate plain text buffer\r\n", __func__);
        goto cleanup;
    }

    rsa_decrypted = crypto_sample_calloc(CRYPTO_SAMPLE_RSA_MARGIN(PT_LEN),
                                         sizeof(unsigned char));
    if (!rsa_decrypted) {
        PRINTF("[%s] Failed to allocate decryption buffer\r\n", __func__);
        goto cleanup;
    }

    rsa_ciphertext = crypto_sample_calloc(KEY_LEN, sizeof(unsigned char));
    if (!rsa_ciphertext) {
        PRINTF("[%s] Failed to allocate cipher text buffer\r\n", __func__);
        goto cleanup;
    }

#if defined(MBEDTLS_SHA1_C)
    sha1sum = crypto_sample_calloc(20, sizeof(unsigned char));
    if (!sha1sum) {
        PRINTF("[%s] Failed to allocate SHA1 buffer\r\n", __func__);
        goto cleanup;
    }
#endif // (MBEDTLS_SHA1_C)

    // Initializes an RSA context.
    mbedtls_rsa_init(rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    // Import pre-defined RSA information.
    rsa->len = KEY_LEN;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->N), 16, RSA_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->E), 16, RSA_E));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->D), 16, RSA_D));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->P), 16, RSA_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->Q), 16, RSA_Q));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->DP), 16, RSA_DP));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->DQ), 16, RSA_DQ));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&(rsa->QP), 16, RSA_QP));

    PRINTF("* RSA key validation: ");

    // Check if a context contains at least an RSA public key.
    ret = mbedtls_rsa_check_pubkey(rsa);
    if (ret) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    ret = mbedtls_rsa_check_privkey(rsa);
    if (ret) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");

    PRINTF("* PKCS#1 encryption : ");

    memcpy(rsa_plaintext, RSA_PT, PT_LEN);

    // Add the message padding, then performs an RSA operation.
    ret = mbedtls_rsa_pkcs1_encrypt(rsa, myrand,
                                    NULL, MBEDTLS_RSA_PUBLIC, PT_LEN,
                                    rsa_plaintext, rsa_ciphertext);
    if (ret) {
        PRINTF( "failed\r\n" );
        goto cleanup;
    }

    PRINTF("passed\r\n");

    crypto_sample_hex_dump("  + rsa_ciphertext: ", rsa_ciphertext, PT_LEN);

    PRINTF("* PKCS#1 decryption : ");

    // Perform an RSA operation, then removes the message padding.
    ret = mbedtls_rsa_pkcs1_decrypt(rsa, myrand,
                                    NULL, MBEDTLS_RSA_PRIVATE, &len,
                                    rsa_ciphertext, rsa_decrypted,
                                    (PT_LEN * sizeof(unsigned char)));

    if (ret) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    // Compare output with expected result.
    if ((ret = memcmp(rsa_decrypted, rsa_plaintext, len)) != 0) {
        PRINTF("failed\r\n");
        crypto_sample_hex_dump("  + rsa_decrypted: ", rsa_decrypted, len );
        crypto_sample_hex_dump("  + rsa_plaintext: ", rsa_plaintext, len );
        goto cleanup;
    }

    PRINTF("passed\r\n");

    crypto_sample_hex_dump("  + rsa_decrypted: ", rsa_decrypted, len );
    crypto_sample_hex_dump("  + rsa_plaintext: ", rsa_plaintext, len );

#if defined(MBEDTLS_SHA1_C)
    PRINTF("* PKCS#1 data sign  : ");

    mbedtls_sha1_ret(rsa_plaintext, PT_LEN, sha1sum);

    // Perform a private RSA operation to sign a message digest using PKCS#1.
    ret = mbedtls_rsa_pkcs1_sign(rsa, myrand,
                                 NULL, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA1,
                                 0, sha1sum, rsa_ciphertext);
    if (ret) {
        PRINTF( "failed\r\n" );
        goto cleanup;
    }

    PRINTF("passed\r\n");

    PRINTF("* PKCS#1 sig. verify: ");

    // Perform a public RSA operation and checks the message digest.
    ret = mbedtls_rsa_pkcs1_verify(rsa, NULL,
                                   NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA1,
                                   0, sha1sum, rsa_ciphertext);
    if (ret) {
        PRINTF("failed\r\n");
        goto cleanup;
    }

    PRINTF("passed\r\n");
#endif /* MBEDTLS_SHA1_C */

cleanup:

    // Free the components of an RSA key.
    mbedtls_rsa_free(rsa);

#if defined(MBEDTLS_SHA1_C)
    if (sha1sum) {
        crypto_sample_free(sha1sum);
    }
#endif // (MBEDTLS_SHA1_C)

    if (rsa_plaintext) {
        crypto_sample_free(rsa_plaintext);
    }

    if (rsa_decrypted) {
        crypto_sample_free(rsa_decrypted);
    }

    if (rsa_ciphertext) {
        crypto_sample_free(rsa_ciphertext);
    }

    if (rsa) {
        crypto_sample_free(rsa);
    }

    return ret;
}
#endif // (MBEDTLS_PKCS1_V15)

/*
 * Sample Functions for RSA PKCS#1
 */
void crypto_sample_rsa(void *param)
{
    DA16X_UNUSED_ARG(param);

#if defined(MBEDTLS_PKCS1_V15)
    crypto_sample_rsa_pkcs1();
#endif // (MBEDTLS_PKCS1_V15)

    return ;
}

/* EOF */
