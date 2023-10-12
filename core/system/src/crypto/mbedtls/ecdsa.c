/*
 *  Elliptic curve DSA
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/*
 * References:
 *
 * SEC1 http://www.secg.org/index.php?action=secg,docs_secg
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECDSA_C)

#include "mbedtls/ecdsa.h"
#include "mbedtls/asn1write.h"

#include <string.h>

#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/platform.h"
#endif

/*
 * Derive a suitable integer for group grp from a buffer of length len
 * SEC1 4.1.3 step 5 aka SEC1 4.1.4 step 3
 */
static int derive_mpi( const mbedtls_ecp_group *grp, mbedtls_mpi *x,
                       const unsigned char *buf, size_t blen )
{
    int ret;
    size_t n_size = ( grp->nbits + 7 ) / 8;
    size_t use_size = blen > n_size ? n_size : blen;

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary( x, buf, use_size ) );
    if( use_size * 8 > grp->nbits )
        MBEDTLS_MPI_CHK( mbedtls_mpi_shift_r( x, use_size * 8 - grp->nbits ) );

    /* While at it, reduce modulo N */
    if( mbedtls_mpi_cmp_mpi( x, &grp->N ) >= 0 )
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi( x, x, &grp->N ) );

cleanup:
    return( ret );
}

#if !defined(MBEDTLS_ECDSA_SIGN_ALT)
/*
 * Compute ECDSA signature of a hashed message (SEC1 4.1.3)
 * Obviously, compared to SEC1 4.1.3, we skip step 4 (hash message)
 */
typedef	   struct {
    mbedtls_ecp_point R;
    mbedtls_mpi k, e, t;
} ECDSA_SIGN_TYPE;

int mbedtls_ecdsa_sign( mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret, key_tries, sign_tries, blind_tries;
    //REMOVE: mbedtls_ecp_point R;
    //REMOVE: mbedtls_mpi k, e, t;
    ECDSA_SIGN_TYPE *ecdsas;

    /* Fail cleanly on curves such as Curve25519 that can't be used for ECDSA */
    if( grp->N.p == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    ecdsas = (ECDSA_SIGN_TYPE *)mbedtls_calloc(1, sizeof(ECDSA_SIGN_TYPE));
    if( ecdsas == NULL ){
    	return( MBEDTLS_ERR_ECP_ALLOC_FAILED );
    }

    mbedtls_ecp_point_init( &(ecdsas->R) );
    mbedtls_mpi_init( &(ecdsas->k) );
    mbedtls_mpi_init( &(ecdsas->e) );
    mbedtls_mpi_init( &(ecdsas->t) );

    sign_tries = 0;
    do
    {
        /*
         * Steps 1-3: generate a suitable ephemeral keypair
         * and set r = xR mod n
         */
        key_tries = 0;
        do
        {
            MBEDTLS_MPI_CHK( mbedtls_ecp_gen_keypair( grp, &(ecdsas->k), &(ecdsas->R), f_rng, p_rng ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( r, &(ecdsas->R.X), &grp->N ) );

            if( key_tries++ > 10 )
            {
                ret = MBEDTLS_ERR_ECP_RANDOM_FAILED;
                goto cleanup;
            }
        }
        while( mbedtls_mpi_cmp_int( r, 0 ) == 0 );

        /*
         * Step 5: derive MPI from hashed message
         */
        MBEDTLS_MPI_CHK( derive_mpi( grp, &(ecdsas->e), buf, blen ) );

        /*
         * Generate a random value to blind inv_mod in next step,
         * avoiding a potential timing leak.
         */
        blind_tries = 0;
        do
        {
            size_t n_size = ( grp->nbits + 7 ) / 8;
            MBEDTLS_MPI_CHK( mbedtls_mpi_fill_random( &(ecdsas->t), n_size, f_rng, p_rng ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_shift_r( &(ecdsas->t), 8 * n_size - grp->nbits ) );

            /* See mbedtls_ecp_gen_keypair() */
            if( ++blind_tries > 30 )
                return( MBEDTLS_ERR_ECP_RANDOM_FAILED );
        }
        while( mbedtls_mpi_cmp_int( &(ecdsas->t), 1 ) < 0 ||
               mbedtls_mpi_cmp_mpi( &(ecdsas->t), &grp->N ) >= 0 );

        /*
         * Step 6: compute s = (e + r * d) / k = t (e + rd) / (kt) mod n
         */
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( s, r, d ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( &(ecdsas->e), &(ecdsas->e), s ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &(ecdsas->e), &(ecdsas->e), &(ecdsas->t) ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &(ecdsas->k), &(ecdsas->k), &(ecdsas->t) ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod( s, &(ecdsas->k), &grp->N ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( s, s, &(ecdsas->e) ) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( s, s, &grp->N ) );

        if( sign_tries++ > 10 )
        {
            ret = MBEDTLS_ERR_ECP_RANDOM_FAILED;
            goto cleanup;
        }
    }
    while( mbedtls_mpi_cmp_int( s, 0 ) == 0 );

cleanup:
    mbedtls_ecp_point_free( &(ecdsas->R) );
    mbedtls_mpi_free( &(ecdsas->k) );
    mbedtls_mpi_free( &(ecdsas->e) );
    mbedtls_mpi_free( &(ecdsas->t) );
    mbedtls_free(ecdsas);

    return( ret );
}
#endif /* MBEDTLS_ECDSA_SIGN_ALT */

#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
/*
 * Deterministic signature wrapper
 */

typedef	   struct {
    mbedtls_hmac_drbg_context rng_ctx;
    unsigned char data[2 * MBEDTLS_ECP_MAX_BYTES];
} ECDSA_SIGN_DET_TYPE;

int mbedtls_ecdsa_sign_det( mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                    const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                    mbedtls_md_type_t md_alg )
{
    int ret;
    //REMOVE: mbedtls_hmac_drbg_context rng_ctx;
    //REMOVE: unsigned char data[2 * MBEDTLS_ECP_MAX_BYTES];
    ECDSA_SIGN_DET_TYPE *ecdsa_sd;
    size_t grp_len;
    const mbedtls_md_info_t *md_info;
    mbedtls_mpi h;

    if ( grp == NULL  || d == NULL || buf == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    if (blen == 0)
        return ( MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL );
    
    if( ( md_info = mbedtls_md_info_from_type( md_alg ) ) == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    ecdsa_sd = (ECDSA_SIGN_DET_TYPE *)mbedtls_calloc(1, sizeof(ECDSA_SIGN_DET_TYPE));
    if( ecdsa_sd == NULL ){
    	return ( MBEDTLS_ERR_ECP_ALLOC_FAILED );
    }

    grp_len = ( grp->nbits + 7 ) / 8;

    mbedtls_mpi_init( &h );
    mbedtls_hmac_drbg_init( &(ecdsa_sd->rng_ctx) );

    /* Use private key and message hash (reduced) to initialize HMAC_DRBG */
    MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary( d, ecdsa_sd->data, grp_len ) );
    MBEDTLS_MPI_CHK( derive_mpi( grp, &h, buf, blen ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary( &h, ecdsa_sd->data + grp_len, grp_len ) );
    mbedtls_hmac_drbg_seed_buf( &(ecdsa_sd->rng_ctx), md_info, ecdsa_sd->data, 2 * grp_len );

    ret = mbedtls_ecdsa_sign( grp, r, s, d, buf, blen,
                      mbedtls_hmac_drbg_random, &(ecdsa_sd->rng_ctx) );

cleanup:
    mbedtls_hmac_drbg_free( &(ecdsa_sd->rng_ctx) );
    mbedtls_mpi_free( &h );
    mbedtls_free(ecdsa_sd);

    return( ret );
}
#endif /* MBEDTLS_ECDSA_DETERMINISTIC */

#if !defined(MBEDTLS_ECDSA_VERIFY_ALT)
/*
 * Verify ECDSA signature of hashed message (SEC1 4.1.4)
 * Obviously, compared to SEC1 4.1.3, we skip step 2 (hash message)
 */
typedef	   struct {
    mbedtls_mpi e, s_inv, u1, u2;
    mbedtls_ecp_point R;
} ECDSA_VERIFY_TYPE;

int mbedtls_ecdsa_verify( mbedtls_ecp_group *grp,
                  const unsigned char *buf, size_t blen,
                  const mbedtls_ecp_point *Q, const mbedtls_mpi *r, const mbedtls_mpi *s)
{
    int ret;
    //REMOVE: mbedtls_mpi e, s_inv, u1, u2;
    //REMOVE: mbedtls_ecp_point R;
    ECDSA_VERIFY_TYPE *ecdsav;

    /* Fail cleanly on curves such as Curve25519 that can't be used for ECDSA */
    if( grp->N.p == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    ecdsav = (ECDSA_VERIFY_TYPE *)mbedtls_calloc(1, sizeof(ECDSA_VERIFY_TYPE));
    if( ecdsav == NULL ){
    	return( MBEDTLS_ERR_ECP_ALLOC_FAILED );
    }

    mbedtls_ecp_point_init( &(ecdsav->R) );
    mbedtls_mpi_init( &(ecdsav->e) );
    mbedtls_mpi_init( &(ecdsav->s_inv) );
    mbedtls_mpi_init( &(ecdsav->u1) );
    mbedtls_mpi_init( &(ecdsav->u2) );

    /*
     * Step 1: make sure r and s are in range 1..n-1
     */
    if( mbedtls_mpi_cmp_int( r, 1 ) < 0 || mbedtls_mpi_cmp_mpi( r, &grp->N ) >= 0 ||
        mbedtls_mpi_cmp_int( s, 1 ) < 0 || mbedtls_mpi_cmp_mpi( s, &grp->N ) >= 0 )
    {
        ret = MBEDTLS_ERR_ECP_VERIFY_FAILED;
        goto cleanup;
    }

    /*
     * Additional precaution: make sure Q is valid
     */
    MBEDTLS_MPI_CHK( mbedtls_ecp_check_pubkey( grp, Q ) );

    /*
     * Step 3: derive MPI from hashed message
     */
    MBEDTLS_MPI_CHK( derive_mpi( grp, &(ecdsav->e), buf, blen ) );

    /*
     * Step 4: u1 = e / s mod n, u2 = r / s mod n
     */
    MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod( &(ecdsav->s_inv), s, &grp->N ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &(ecdsav->u1), &(ecdsav->e), &(ecdsav->s_inv) ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &(ecdsav->u1), &(ecdsav->u1), &grp->N ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &(ecdsav->u2), r, &(ecdsav->s_inv) ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &(ecdsav->u2), &(ecdsav->u2), &grp->N ) );

    /*
     * Step 5: R = u1 G + u2 Q
     *
     * Since we're not using any secret data, no need to pass a RNG to
     * mbedtls_ecp_mul() for countermesures.
     */
    MBEDTLS_MPI_CHK( mbedtls_ecp_muladd( grp, &(ecdsav->R), &(ecdsav->u1), &grp->G, &(ecdsav->u2), Q ) );

    if( mbedtls_ecp_is_zero( &(ecdsav->R) ) )
    {
        ret = MBEDTLS_ERR_ECP_VERIFY_FAILED;
        goto cleanup;
    }

    /*
     * Step 6: convert xR to an integer (no-op)
     * Step 7: reduce xR mod n (gives v)
     */
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &(ecdsav->R.X), &(ecdsav->R.X), &grp->N ) );

    /*
     * Step 8: check if v (that is, R.X) is equal to r
     */
    if( mbedtls_mpi_cmp_mpi( &(ecdsav->R.X), r ) != 0 )
    {
        ret = MBEDTLS_ERR_ECP_VERIFY_FAILED;
        goto cleanup;
    }

cleanup:
    mbedtls_ecp_point_free( &(ecdsav->R) );
    mbedtls_mpi_free( &(ecdsav->e) );
    mbedtls_mpi_free( &(ecdsav->s_inv) );
    mbedtls_mpi_free( &(ecdsav->u1) );
    mbedtls_mpi_free( &(ecdsav->u2) );
    mbedtls_free(ecdsav);

    return( ret );
}
#endif /* MBEDTLS_ECDSA_VERIFY_ALT */

/*
 * Convert a signature (given by context) to ASN.1
 */

#define MBEDTLS_ECDSA_ASN1_CHK_ADD(g, f) 		\
	do { if( ( ret = f ) < 0 ) goto cleanup; else g += ret; } while( 0 )

static int ecdsa_signature_to_asn1( const mbedtls_mpi *r, const mbedtls_mpi *s,
                                    unsigned char *sig, size_t *slen )
{
    int ret;
    //REMOVE: unsigned char buf[MBEDTLS_ECDSA_MAX_LEN];
    //REMOVE: unsigned char *p = buf + sizeof( buf );
    unsigned char *buf, *p;
    size_t len = 0;

    buf = mbedtls_calloc( MBEDTLS_ECDSA_MAX_LEN, sizeof(unsigned char));
    if( buf == NULL ){
    	return (-1);
    }
    p = buf + (MBEDTLS_ECDSA_MAX_LEN * sizeof(unsigned char));

    MBEDTLS_ECDSA_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, s ) );
    MBEDTLS_ECDSA_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, r ) );

    MBEDTLS_ECDSA_ASN1_CHK_ADD( len, mbedtls_asn1_write_len( &p, buf, len ) );
    MBEDTLS_ECDSA_ASN1_CHK_ADD( len, mbedtls_asn1_write_tag( &p, buf,
                                       MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) );

    ret = 0;
    memcpy( sig, p, len );
    *slen = len;

cleanup:
    mbedtls_free(buf);

    return( ret );
}

/*
 * Compute and write signature
 */
int mbedtls_ecdsa_write_signature( mbedtls_ecdsa_context *ctx, mbedtls_md_type_t md_alg,
                           const unsigned char *hash, size_t hlen,
                           unsigned char *sig, size_t *slen,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng )
{
    int ret;
    mbedtls_mpi r, s;

    if ( ctx == NULL || sig == NULL || slen == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );

#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
    (void) f_rng;
    (void) p_rng;

    MBEDTLS_MPI_CHK( mbedtls_ecdsa_sign_det( &ctx->grp, &r, &s, &ctx->d,
                             hash, hlen, md_alg ) );
#else
    (void) md_alg;

    MBEDTLS_MPI_CHK( mbedtls_ecdsa_sign( &ctx->grp, &r, &s, &ctx->d,
                         hash, hlen, f_rng, p_rng ) );
#endif

    MBEDTLS_MPI_CHK( ecdsa_signature_to_asn1( &r, &s, sig, slen ) );

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );

    return( ret );
}

#if ! defined(MBEDTLS_DEPRECATED_REMOVED) && \
    defined(MBEDTLS_ECDSA_DETERMINISTIC)
int mbedtls_ecdsa_write_signature_det( mbedtls_ecdsa_context *ctx,
                               const unsigned char *hash, size_t hlen,
                               unsigned char *sig, size_t *slen,
                               mbedtls_md_type_t md_alg )
{
    return( mbedtls_ecdsa_write_signature( ctx, md_alg, hash, hlen, sig, slen,
                                   NULL, NULL ) );
}
#endif

/*
 * Read and check signature
 */
int mbedtls_ecdsa_read_signature( mbedtls_ecdsa_context *ctx,
                          const unsigned char *hash, size_t hlen,
                          const unsigned char *sig, size_t slen )
{
    int ret;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    size_t len;
    mbedtls_mpi r, s;

    if ( ctx == NULL || sig == NULL ) /* hash is validated in verify API*/
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    if (slen == 0 )
        return MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL;
    
    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( p + len != end )
    {
        ret = MBEDTLS_ERR_ECP_BAD_INPUT_DATA +
              MBEDTLS_ERR_ASN1_LENGTH_MISMATCH;
        goto cleanup;
    }

    if( ( ret = mbedtls_asn1_get_mpi( &p, end, &r ) ) != 0 ||
        ( ret = mbedtls_asn1_get_mpi( &p, end, &s ) ) != 0 )
    {
        ret += MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( ( ret = mbedtls_ecdsa_verify( &ctx->grp, hash, hlen,
                              &ctx->Q, &r, &s ) ) != 0 )
        goto cleanup;

    if( p != end )
        ret = MBEDTLS_ERR_ECP_SIG_LEN_MISMATCH;

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );

    return( ret );
}

#if !defined(MBEDTLS_ECDSA_GENKEY_ALT)
/*
 * Generate key pair
 */
int mbedtls_ecdsa_genkey( mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
                  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    if ( ctx == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    return( mbedtls_ecp_group_load( &ctx->grp, gid ) ||
            mbedtls_ecp_gen_keypair( &ctx->grp, &ctx->d, &ctx->Q, f_rng, p_rng ) );
}
#endif /* MBEDTLS_ECDSA_GENKEY_ALT */

/*
 * Set context from an mbedtls_ecp_keypair
 */
int mbedtls_ecdsa_from_keypair( mbedtls_ecdsa_context *ctx, const mbedtls_ecp_keypair *key )
{
    int ret;

    if ( ctx == NULL || key == NULL )
        return( MBEDTLS_ERR_ECP_BAD_INPUT_DATA );

    if( ( ret = mbedtls_ecp_group_copy( &ctx->grp, &key->grp ) ) != 0 ||
        ( ret = mbedtls_mpi_copy( &ctx->d, &key->d ) ) != 0 ||
        ( ret = mbedtls_ecp_copy( &ctx->Q, &key->Q ) ) != 0 )
    {
        mbedtls_ecdsa_free( ctx );
    }

    return( ret );
}

/*
 * Initialize context
 */
void mbedtls_ecdsa_init( mbedtls_ecdsa_context *ctx )
{
    mbedtls_ecp_keypair_init( ctx );
}

/*
 * Free context
 */
void mbedtls_ecdsa_free( mbedtls_ecdsa_context *ctx )
{
    mbedtls_ecp_keypair_free( ctx );
}

#endif /* MBEDTLS_ECDSA_C */
