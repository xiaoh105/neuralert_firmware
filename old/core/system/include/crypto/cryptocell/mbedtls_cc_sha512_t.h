/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2017-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
@file
@brief This file contains all of the CryptoCell SHA-512 truncated APIs, their enums and definitions.

@defgroup cc_sha512_t_h CryptoCell SHA-512 truncated APIs
@brief Contains all CryptoCell SHA-512 truncated APIs.

@{
@ingroup cc_hash

*/


#ifndef _MBEDTLS_CC_SHA512_T_H
#define _MBEDTLS_CC_SHA512_T_H

#include <sha512.h>

/**
 * \brief          This function initializes the SHA-512_t context.
 *
 * \param ctx      The SHA-512_t context to initialize.
 */
void mbedtls_sha512_t_init( mbedtls_sha512_context *ctx );

/**
 * \brief          This function clears the SHA-512_t context.
 *
 * \param ctx      The SHA-512_t context to clear.
 */
void mbedtls_sha512_t_free( mbedtls_sha512_context *ctx );

/**
 * \brief          This function starts a SHA-512_t checksum
 *                 calculation.
 *
 * \param ctx      The context to initialize.
 * \param is224    Determines which function to use.
 *                 <ul><li>0: Use SHA-512/256.</li>
 *                 <li>1: Use SHA-512/224.</li></ul>
 */
void mbedtls_sha512_t_starts( mbedtls_sha512_context *ctx, int is224 );

/**
 * \brief          This function feeds an input buffer into an ongoing
 *                 SHA-512_t checksum calculation.
 *
 * \param ctx      The SHA-512_t context.
 * \param input    The buffer holding the input data.
 * \param ilen     The length of the input data.
 */
void mbedtls_sha512_t_update( mbedtls_sha512_context *ctx, const unsigned char *input,
                    size_t ilen );

/**
 * \brief          This function finishes the SHA-512_t operation, and writes
 *                 the result to the output buffer.
 *
 *                 <ul><li>For SHA512/224, the output buffer will include
 *                 the 28 leftmost Bytes of the SHA-512 digest.</li>
 *                 <li>For SHA512/256, the output buffer will include
 *                 the 32 leftmost bytes of the SHA-512 digest.</li></ul>
 *
 * \param ctx      The SHA-512_t context.
 * \param output   The SHA-512/256 or SHA-512/224 checksum result.
 * \param is224    Determines which function to use.
 *                 <ul><li>0: Use SHA-512/256.</li>
 *                 <li>1: Use SHA-512/224.</li></ul>
 */
void mbedtls_sha512_t_finish( mbedtls_sha512_context *ctx, unsigned char output[32], int is224 );

/**
 * \brief          This function calculates the SHA-512 checksum of a buffer.
 *
 *                 The function allocates the context, performs the
 *                 calculation, and frees the context.
 *
 *                 The SHA-512 result is calculated as
 *                 output = SHA-512(input buffer).
 *
 * \param input    The buffer holding the input data.
 * \param ilen     The length of the input data.
 * \param output   The SHA-512/256 or SHA-512/224 checksum result.
 * \param is224    Determines which function to use.
 *                 <ul><li>0: Use SHA-512/256.</li>
 *                 <li>1: Use SHA-512/224.</li></ul>
 */
void mbedtls_sha512_t( const unsigned char *input, size_t ilen,
             unsigned char output[32], int is224 );

#endif

/**
@}
*/

