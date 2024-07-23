/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#include <stddef.h>
#include <stdint.h>

#include "cc_hash_defs_proj.h"
#if defined (MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif


#define MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED                -0x0037  /**< SHA-256 hardware accelerator failed */

#if ( defined(__ARMCC_VERSION) || defined(_MSC_VER) ) && \
    !defined(inline) && !defined(__cplusplus)
#define inline __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          SHA-256 context structure
 */
typedef struct mbedtls_sha256_context {
        /*! Internal buffer */
        uint32_t buff[CC_HASH_USER_CTX_SIZE_IN_WORDS]; // defined in cc_hash_defs.h
} mbedtls_sha256_context;


/**
 * \brief          This function initializes a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to initialize.
 */
void mbedtls_sha256_init( mbedtls_sha256_context *ctx );

/**
 * \brief          This function clears a SHA-256 context.
 *
 * \param ctx      The SHA-256 context to clear.
 */
void mbedtls_sha256_free( mbedtls_sha256_context *ctx );

/**
 * \brief          This function clones the state of a SHA-256 context.
 *
 * \param dst      The destination context.
 * \param src      The context to clone.
 */
void mbedtls_sha256_clone( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src );

/**
 * \brief          This function starts a SHA-224 or SHA-256 checksum
 *                 calculation.
 *
 * \param ctx      The context to initialize.
 * \param is224    Determines which function to use.
 *                 <ul><li>0: Use SHA-256.</li>
 *                 <li>1: Use SHA-224.</li></ul>
 *
 * \return         \c 0 on success.
 */
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 );

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-256 checksum calculation.
 *
 * \param ctx      SHA-256 context
 * \param input    buffer holding the data
 * \param ilen     length of the input data
 *
 * \return         \c 0 on success.
 */
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input,
                    size_t ilen );

/**
 * \brief          This function finishes the SHA-256 operation, and writes
 *                 the result to the output buffer.
 *
 * \param ctx      The SHA-256 context.
 * \param output   The SHA-224 or SHA-256 checksum result.
 *
 * \return         \c 0 on success.
 */
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32] );

/* Internal use */
int mbedtls_internal_sha256_process( mbedtls_sha256_context *ctx, const unsigned char data[64] );


#if !defined(MBEDTLS_DEPRECATED_REMOVED)
#if defined(MBEDTLS_DEPRECATED_WARNING)
#define MBEDTLS_DEPRECATED      __attribute__((deprecated))
#else
#define MBEDTLS_DEPRECATED
#endif
/**
 * \brief          This function starts a SHA-256 checksum calculation.
 *
 * \deprecated     Superseded by mbedtls_sha256_starts_ret() in 2.7.0.
 *
 * \param ctx      The SHA-256 context to initialize.
 * \param is224    Determines which function to use.
 *                 <ul><li>0: Use SHA-256.</li>
 *                 <li>1: Use SHA-224.</li></ul>
 */
MBEDTLS_DEPRECATED static inline void mbedtls_sha256_starts(
                                                mbedtls_sha256_context *ctx,
                                                int is224 )
{
    mbedtls_sha256_starts_ret( ctx, is224 );
}

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-256 checksum calculation.
 *
 * \deprecated     Superseded by mbedtls_sha256_update_ret() in 2.7.0.
 *
 * \param ctx      The SHA-256 context to initialize.
 * \param input    The buffer holding the data.
 * \param ilen     The length of the input data.
 */
MBEDTLS_DEPRECATED static inline void mbedtls_sha256_update(
                                                mbedtls_sha256_context *ctx,
                                                const unsigned char *input,
                                                size_t ilen )
{
    mbedtls_sha256_update_ret( ctx, input, ilen );
}

/**
 * \brief          This function finishes the SHA-256 operation, and writes
 *                 the result to the output buffer.
 *
 * \deprecated     Superseded by mbedtls_sha256_finish_ret() in 2.7.0.
 *
 * \param ctx      The SHA-256 context.
 * \param output   The SHA-224or SHA-256 checksum result.
 */
MBEDTLS_DEPRECATED static inline void mbedtls_sha256_finish(
                                                mbedtls_sha256_context *ctx,
                                                unsigned char output[32] )
{
    mbedtls_sha256_finish_ret( ctx, output );
}

/**
 * \brief          This function processes a single data block within
 *                 the ongoing SHA-256 computation. This function is for
 *                 internal use only.
 *
 * \deprecated     Superseded by mbedtls_internal_sha256_process() in 2.7.0.
 *
 * \param ctx      The SHA-256 context.
 * \param data     The buffer holding one block of data.
 */
MBEDTLS_DEPRECATED static inline void mbedtls_sha256_process(
                                                mbedtls_sha256_context *ctx,
                                                const unsigned char data[64] )
{
    mbedtls_internal_sha256_process( ctx, data );
}

#undef MBEDTLS_DEPRECATED
#endif /* !MBEDTLS_DEPRECATED_REMOVED */


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mbedtls_sha256_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_SHA256_ALT_H */
