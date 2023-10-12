/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2018] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef MBEDTLS_CC_ECC_INTERNAL_H
#define MBEDTLS_CC_ECC_INTERNAL_H

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif


#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT) || defined(MBEDTLS_ECDSA_GENKEY_ALT)
/*
 * Generate a keypair with configurable base point
 */
int cc_ecp_gen_keypair_base( mbedtls_ecp_group *grp,
                             const mbedtls_ecp_point *G,
                             mbedtls_mpi *d, mbedtls_ecp_point *Q,
                             int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng );

/*
 * Generate key pair, wrapper for conventional base point
 */
int cc_ecp_gen_keypair( mbedtls_ecp_group *grp,
                             mbedtls_mpi *d, mbedtls_ecp_point *Q,
                             int (*f_rng)(void *, unsigned char *, size_t),
                             void *p_rng );

#endif /* MBEDTLS_ECDH_GEN_PUBLIC_ALT || MBEDTLS_ECDSA_GENKEY_ALT*/

#if defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
/*
 * Multiplication R = m * P
 */
int cc_ecp_mul( mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
             const mbedtls_mpi *m, const mbedtls_ecp_point *P,
             int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );

int mbedtls_ecdh_compute_shared( mbedtls_ecp_group *grp, mbedtls_mpi *z,
                         const mbedtls_ecp_point *Q, const mbedtls_mpi *d,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng );

#endif /* MBEDTLS_ECDH_COMPUTE_SHARED_ALT */

#endif //MBEDTLS_CC_ECC_INTERNAL_H


