/**
 ****************************************************************************************
 *
 * @file crypto_sample_ecdh.c
 *
 * @brief Sample app of ECDH(Elliptic-curve Diffie–Hellman).
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
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"

#if defined(MBEDTLS_ECDH_C)					\
	&& defined(MBEDTLS_ENTROPY_C)			\
	&& defined(MBEDTLS_CTR_DRBG_C)

int crypto_sample_ecdh_key_exchange(mbedtls_ecp_group_id id)
{
    int ret = -1;
    const char pers[] = "crypto_sample_ecdh";

    mbedtls_ecdh_context ctx_cli;
    mbedtls_ecdh_context ctx_srv;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    unsigned char *cli_to_srv_x = NULL;
    unsigned char *cli_to_srv_y = NULL;
    unsigned char *srv_to_cli_x = NULL;
    unsigned char *srv_to_cli_y = NULL;

    size_t buflen = 66;

    cli_to_srv_x = crypto_sample_calloc(buflen, sizeof(unsigned char));
    if (!cli_to_srv_x) {
        PRINTF("[%s] Failed to allocate client buffer\r\n", __func__);
        goto cleanup;
    }

    cli_to_srv_y = crypto_sample_calloc(buflen, sizeof(unsigned char));
    if (!cli_to_srv_y) {
        PRINTF("[%s] Failed to allocate client buffer\r\n", __func__);
        goto cleanup;
    }

    srv_to_cli_x = crypto_sample_calloc(buflen, sizeof(unsigned char));
    if (!srv_to_cli_x) {
        PRINTF("[%s] Failed to allocate server buffer\r\n", __func__);
        goto cleanup;
    }

    srv_to_cli_y = crypto_sample_calloc(buflen, sizeof(unsigned char));
    if (!srv_to_cli_y) {
        PRINTF("[%s] Failed to allocate server buffer\r\n", __func__);
        goto cleanup;
    }

    // Initialize an ECDH context.
    mbedtls_ecdh_init(&ctx_cli);
    mbedtls_ecdh_init(&ctx_srv);

    // Initialize the CTR_DRBG context.
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Initialize the entropy context.
    mbedtls_entropy_init(&entropy);

    PRINTF( ">>> Using Elliptic Curve: ");

    switch (id) {
    case MBEDTLS_ECP_DP_SECP224R1: {
        PRINTF("SECP224R1\r\n");
    }
    break;
    case MBEDTLS_ECP_DP_SECP256R1: {
        PRINTF("SECP256R1\r\n");
    }
    break;
    case MBEDTLS_ECP_DP_SECP384R1: {
        PRINTF("SECP384R1\r\n");
    }
    break;
    case MBEDTLS_ECP_DP_SECP521R1: {
        PRINTF("SECP521R1\r\n");
    }
    break;
    case MBEDTLS_ECP_DP_CURVE25519: {
        PRINTF("Curve25519\r\n");
    }
    break;
    default: {
        PRINTF("failed - [%s] Invalid Curve selected!\r\n");
    }
    goto cleanup;
    }

    // Initialize random number generation
    PRINTF("* Seeding the random number generator: ");

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, sizeof(pers));
    if (ret) {
        PRINTF("failed - [%s] Failed to seed CTR_DRBG entropy(0x%x)\r\n", __func__, -ret);
        goto cleanup;
    }

    PRINTF("passed\r\n");

    // Client: inialize context and generate keypair
    PRINTF("* Setting up client context: ");

    // Sets up an ECP group context from a standardized set of domain parameters.
    ret = mbedtls_ecp_group_load(&(ctx_cli.grp), id);
    if (ret) {
        PRINTF("failed - [%s] Failed to set up an ECP group context(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Generate an ECDH keypair on an elliptic curve.
    ret = mbedtls_ecdh_gen_public(&(ctx_cli.grp), &(ctx_cli.d), &(ctx_cli.Q),
                                  mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate an ECDH keypair(0x%x)\r\n",
               __func__, ret);
        goto cleanup;
    }

    /* Export multi-precision integer (MPI) into unsigned binary data,
     * big endian (X coordinate of ECP point)
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&(ctx_cli.Q.X), cli_to_srv_x, buflen));

    /* Export multi-precision integer (MPI) into unsigned binary data,
     * big endian (Y coordinate of ECP point)
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&(ctx_cli.Q.Y), cli_to_srv_y, buflen));

    PRINTF("passed\r\n");

    // Server: initialize context and generate keypair
    PRINTF("* Setting up server context: ");

    // Sets up an ECP group context from a standardized set of domain parameters.
    ret = mbedtls_ecp_group_load(&(ctx_srv.grp), id);
    if (ret) {
        PRINTF("failed - [%s] Failed to set up an ECP group context(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    // Generate a public key
    ret = mbedtls_ecdh_gen_public(&(ctx_srv.grp), &(ctx_srv.d), &(ctx_srv.Q),
                                  mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("failed - [%s] Failed to generate an ECDH keypair(0x%x)\r\n",
               __func__, ret);
        goto cleanup;
    }

    /* Export multi-precision integer (MPI) into unsigned binary data,
     * big endian (X coordinate of ECP point).
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&(ctx_srv.Q.X), srv_to_cli_x, buflen));

    /* Export multi-precision integer (MPI) into unsigned binary data,
     * big endian (Y coordinate of ECP point).
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary(&(ctx_srv.Q.Y), srv_to_cli_y, buflen));

    PRINTF("passed\r\n");

    // Server: read peer's key and generate shared secret.
    PRINTF("* Server reading client key and computing secret: ");

    // Set the Z component of the peer's public value (public key) to 1
    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&(ctx_srv.Qp.Z), 1));

    /* Set the X component of the peer's public value based on
     * what was passed from client in the buffer.
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&(ctx_srv.Qp.X), cli_to_srv_x, buflen));

    /* Set the Y component of the peer's public value based on
     * what was passed from client in the buffer.
     */
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&(ctx_srv.Qp.Y), cli_to_srv_y, buflen));

    // Compute the shared secret.
    ret = mbedtls_ecdh_compute_shared(&(ctx_srv.grp),
                                      &(ctx_srv.z), &(ctx_srv.Qp), &(ctx_srv.d),
                                      mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("failed - [%s] Failed to compute the shared secret(0x%x)\r\n",
               __func__, -ret);
        goto cleanup;
    }

    PRINTF("passed\r\n");

    // Client: read peer's key and generate shared secret
    PRINTF("* Client reading server key and computing secret: ");

    MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&(ctx_cli.Qp.Z), 1));

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&(ctx_cli.Qp.X), srv_to_cli_x, buflen));

    MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary(&(ctx_cli.Qp.Y), srv_to_cli_y, buflen));

    // Compute the shared secret.
    ret = mbedtls_ecdh_compute_shared(&(ctx_cli.grp), &(ctx_cli.z),
                                      &(ctx_cli.Qp), &(ctx_cli.d),
                                      mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret) {
        PRINTF("failed - [%s] Failed to compute the shared secret(0x%x)", __func__, -ret);
        goto cleanup;
    }

    PRINTF("passed\r\n");

    // Verification: are the computed secrets equal?
    PRINTF("* Checking if both computed secrets are equal: ");

    MBEDTLS_MPI_CHK(mbedtls_mpi_cmp_mpi(&(ctx_cli.z), &(ctx_srv.z)));

    PRINTF("passed\r\n");

cleanup:

    // Free ECDH context.
    mbedtls_ecdh_free(&ctx_cli);

    mbedtls_ecdh_free(&ctx_srv);

    // Free the data in the context.
    mbedtls_entropy_free(&entropy);

    // Clear CTR_CRBG context data.
    mbedtls_ctr_drbg_free(&ctr_drbg);

    if (cli_to_srv_x) {
        crypto_sample_free(cli_to_srv_x);
    }

    if (cli_to_srv_y) {
        crypto_sample_free(cli_to_srv_y);
    }

    if (srv_to_cli_x) {
        crypto_sample_free(srv_to_cli_x);
    }

    if (srv_to_cli_y) {
        crypto_sample_free(srv_to_cli_y);
    }

    PRINTF("\r\n");

    return ret;
}
#endif // (MBEDTLS_ECDH_C) && (MBEDTLS_ENTROPY_C) && (MBEDTLS_CTR_DRBG_C)

/*
 * Sample Functions for ECDH
 */
void crypto_sample_ecdh(void *param)
{
    DA16X_UNUSED_ARG(param);

#if defined(MBEDTLS_ECDH_C)					\
	&& defined(MBEDTLS_ENTROPY_C)			\
	&& defined(MBEDTLS_CTR_DRBG_C)

    int ret = 0;
    int idx = 0;
    mbedtls_ecp_group_id id = MBEDTLS_ECP_DP_NONE;

    mbedtls_ecp_group_id ids[6] = {
#if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
        MBEDTLS_ECP_DP_SECP224R1,	/*!< 224-bits NIST curve  */
#endif // (MBEDTLS_ECP_DP_SECP224R1_ENABLED)
#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
        MBEDTLS_ECP_DP_SECP256R1,	/*!< 256-bits NIST curve  */
#endif // (MBEDTLS_ECP_DP_SECP256R1_ENABLED)
#if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
        MBEDTLS_ECP_DP_SECP384R1,	/*!< 384-bits NIST curve  */
#endif // (MBEDTLS_ECP_DP_SECP384R1_ENABLED)
#if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
        MBEDTLS_ECP_DP_SECP521R1,	/*!< 521-bits NIST curve  */
#endif // (MBEDTLS_ECP_DP_SECP521R1_ENABLED)
#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
        MBEDTLS_ECP_DP_CURVE25519,	/*!< Curve25519           */
#endif // (MBEDTLS_ECP_DP_CURVE25519_ENABLED)
        MBEDTLS_ECP_DP_NONE
    };

    for (idx = 0, id = ids[idx] ; idx < 6 && id != MBEDTLS_ECP_DP_NONE ; idx++, id = ids[idx]) {
        ret = crypto_sample_ecdh_key_exchange(id);
        if (ret) {
            break;
        }
    }
#endif // (MBEDTLS_ECDH_C) && (MBEDTLS_ENTROPY_C) && (MBEDTLS_CTR_DRBG_C)

    return ;
}

/* EOF */
