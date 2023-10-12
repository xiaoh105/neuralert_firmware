/****************************************************************************
 * The confidential and proprietary information contained in this file may   *
 * only be used by a person authorised under and to the extent permitted     *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.   *
 *  (C) COPYRIGHT [2018] ARM Limited or its affiliates.      *
 *      ALL RIGHTS RESERVED                          *
 * This entire notice must be reproduced on all copies of this file          *
 * and copies of this file may only be made by a person if such person is    *
 * permitted to do so under the terms of a subsisting license agreement      *
 * from ARM Limited or its affiliates.                       *
 *****************************************************************************/

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif


#include "mbedtls/ecdh.h"

#include "cc_ecc_internal.h"

#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
/*
 * Generate key pair, wrapper for conventional base point
 */
int cc_ecp_gen_keypair( mbedtls_ecp_group *grp,
                             mbedtls_mpi *d, mbedtls_ecp_point *Q,
                             int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng )
{
    return( cc_ecp_gen_keypair_base( grp, &grp->G, d, Q, f_rng, p_rng ) );
}

/*
 * Generate public key: simple wrapper around mbedtls_ecp_gen_keypair
 */
int mbedtls_ecdh_gen_public( mbedtls_ecp_group *grp, mbedtls_mpi *d, mbedtls_ecp_point *Q,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng )
{
    return cc_ecp_gen_keypair( grp, d, Q, f_rng, p_rng );
}
#endif /* MBEDTLS_ECDH_GEN_PUBLIC_ALT */

#if defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
/*
 * Compute shared secret (SEC1 3.3.1)
 */
int mbedtls_ecdh_compute_shared( mbedtls_ecp_group *grp, mbedtls_mpi *z,
                         const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng )
{
    int ret;
    mbedtls_ecp_point P;

    mbedtls_ecp_point_init( &P );

    if ( z == NULL || Q == NULL || grp == NULL || d == NULL)
    {
        ret = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }
    /*
     * Make sure Q is a valid pubkey before using it
     */
    MBEDTLS_MPI_CHK( mbedtls_ecp_check_pubkey( grp, Q ) );

    MBEDTLS_MPI_CHK( cc_ecp_mul( grp, &P, d, Q, f_rng, p_rng ) );

    if( mbedtls_ecp_is_zero( &P ) )
    {
        ret = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    MBEDTLS_MPI_CHK( mbedtls_mpi_copy( z, &P.X ) );

cleanup:
    mbedtls_ecp_point_free( &P );

    return( ret );
}
#endif /* MBEDTLS_ECDH_COMPUTE_SHARED_ALT */
