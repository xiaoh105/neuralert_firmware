/**
 * \file des.h
 *
 * The confidential and proprietary information contained in this file may    *
 * only be used by a person authorised under and to the extent permitted      *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.    *
 *       (C) COPYRIGHT [2017] ARM Limited or its affiliates.                  *
 *           ALL RIGHTS RESERVED                                              *
 * This entire notice must be reproduced on all copies of this file           *
 * and copies of this file may only be made by a person if such person is     *
 * permitted to do so under the terms of a subsisting license agreement       *
 * from ARM Limited or its affiliates.                                        *
 *****************************************************************************/
#ifndef MBEDTLS_DES_H
#define MBEDTLS_DES_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if !defined(SUPPORT_FREERTOS)

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_DES_ENCRYPT     1
#define MBEDTLS_DES_DECRYPT     0

#define MBEDTLS_ERR_DES_INVALID_INPUT_LENGTH              -0x0032  /**< The data input has an invalid length. */

#define MBEDTLS_DES_KEY_SIZE    8

#if !defined(MBEDTLS_DES_ALT)
// Regular implementation
//

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          DES context structure
 */
typedef struct
{
    uint32_t sk[32];            /*!<  DES subkeys       */
}
mbedtls_des_context;

/**
 * \brief          Triple-DES context structure
 */
typedef struct
{
    uint32_t sk[96];            /*!<  3DES subkeys      */
}
mbedtls_des3_context;

/**
 * \brief          Initialize DES context
 *
 * \param ctx      DES context to be initialized
 */
void mbedtls_des_init( mbedtls_des_context *ctx );

/**
 * \brief          Clear DES context
 *
 * \param ctx      DES context to be cleared
 */
void mbedtls_des_free( mbedtls_des_context *ctx );

/**
 * \brief          Initialize Triple-DES context
 *
 * \param ctx      DES3 context to be initialized
 */
void mbedtls_des3_init( mbedtls_des3_context *ctx );

/**
 * \brief          Clear Triple-DES context
 *
 * \param ctx      DES3 context to be cleared
 */
void mbedtls_des3_free( mbedtls_des3_context *ctx );

/**
 * \brief          Set key parity on the given key to odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * \param key      8-byte secret key
 */
void mbedtls_des_key_set_parity( unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Check that key parity on the given key is odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * \param key      8-byte secret key
 *
 * \return         0 is parity was ok, 1 if parity was not correct.
 */
int mbedtls_des_key_check_key_parity( const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Check that key is not a weak or semi-weak DES key
 *
 * \param key      8-byte secret key
 *
 * \return         0 if no weak key was found, 1 if a weak key was identified.
 */
int mbedtls_des_key_check_weak( const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          DES key schedule (56-bit, encryption)
 *
 * \param ctx      DES context to be initialized
 * \param key      8-byte secret key
 *
 * \return         0
 */
int mbedtls_des_setkey_enc( mbedtls_des_context *ctx, const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          DES key schedule (56-bit, decryption)
 *
 * \param ctx      DES context to be initialized
 * \param key      8-byte secret key
 *
 * \return         0
 */
int mbedtls_des_setkey_dec( mbedtls_des_context *ctx, const unsigned char key[MBEDTLS_DES_KEY_SIZE] );

/**
 * \brief          Triple-DES key schedule (112-bit, encryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      16-byte secret key
 *
 * \return         0
 */
int mbedtls_des3_set2key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] );

/**
 * \brief          Triple-DES key schedule (112-bit, decryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      16-byte secret key
 *
 * \return         0
 */
int mbedtls_des3_set2key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 2] );

/**
 * \brief          Triple-DES key schedule (168-bit, encryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      24-byte secret key
 *
 * \return         0
 */
int mbedtls_des3_set3key_enc( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] );

/**
 * \brief          Triple-DES key schedule (168-bit, decryption)
 *
 * \param ctx      3DES context to be initialized
 * \param key      24-byte secret key
 *
 * \return         0
 */
int mbedtls_des3_set3key_dec( mbedtls_des3_context *ctx,
                      const unsigned char key[MBEDTLS_DES_KEY_SIZE * 3] );

/**
 * \brief          DES-ECB block encryption/decryption
 *
 * \param ctx      DES context
 * \param input    64-bit input block
 * \param output   64-bit output block
 *
 * \return         0 if successful
 */
int mbedtls_des_crypt_ecb( mbedtls_des_context *ctx,
                    const unsigned char input[8],
                    unsigned char output[8] );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief          DES-CBC buffer encryption/decryption
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      DES context
 * \param mode     MBEDTLS_DES_ENCRYPT or MBEDTLS_DES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
int mbedtls_des_crypt_cbc( mbedtls_des_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[8],
                    const unsigned char *input,
                    unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/**
 * \brief          3DES-ECB block encryption/decryption
 *
 * \param ctx      3DES context
 * \param input    64-bit input block
 * \param output   64-bit output block
 *
 * \return         0 if successful
 */
int mbedtls_des3_crypt_ecb( mbedtls_des3_context *ctx,
                     const unsigned char input[8],
                     unsigned char output[8] );

#if defined(MBEDTLS_CIPHER_MODE_CBC)
/**
 * \brief          3DES-CBC buffer encryption/decryption
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      3DES context
 * \param mode     MBEDTLS_DES_ENCRYPT or MBEDTLS_DES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful, or MBEDTLS_ERR_DES_INVALID_INPUT_LENGTH
 */
int mbedtls_des3_crypt_cbc( mbedtls_des3_context *ctx,
                     int mode,
                     size_t length,
                     unsigned char iv[8],
                     const unsigned char *input,
                     unsigned char *output );
#endif /* MBEDTLS_CIPHER_MODE_CBC */

/**
 * \brief          Internal function for key expansion.
 *                 (Only exposed to allow overriding it,
 *                 see MBEDTLS_DES_SETKEY_ALT)
 *
 * \param SK       Round keys
 * \param key      Base key
 */
void mbedtls_des_setkey( uint32_t SK[32],
                         const unsigned char key[MBEDTLS_DES_KEY_SIZE] );
#ifdef __cplusplus
}
#endif

#define MBEDTLS_DES_KEY_SIZE    8
//#define MBEDTLS_DES_ENCRYPT     1
//#define MBEDTLS_DES_DECRYPT     0
#define DES_CONTROL				0x50020000
#define CBC_IV					0x50020004
#define DES_KEY1				0x5002000c
#define DES_KEY2				0x50020014
#define DES_KEY3				0x5002001C
#define DES_INPUT				0x50020024
#define DES_OUTPUT				0x5002002C
#define DES_INT					52
#define DES_INT_EVT				0x01
#include "oal.h"
typedef struct
{
    OAL_SEMAPHORE     des_sema;
	OAL_EVENT_GROUP   des_event;
}
alt_des3_context;

int alt_des3_init( alt_des3_context *des );
int alt_des3_free( alt_des3_context *des );

int alt_des3_setkey(alt_des3_context *des, const unsigned char key[8]);
int alt_des3_set2key(alt_des3_context *des, const unsigned char key[16]);
int alt_des3_set3key(alt_des3_context *des, const unsigned char key[24]);

int alt_des3_crypt_cbc(	alt_des3_context *des,
					    int mode,
                     	size_t length, // %8
                     	unsigned char iv[8],
                     	const unsigned char *input,
                     	unsigned char *output );

int alt_des_self_test( int verbose );

#else  /* MBEDTLS_DES_ALT */
#include "des_alt.h"
#endif /* MBEDTLS_DES_ALT */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mbedtls_des_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif // !(SUPPORT_FREERTOS)

#endif /* des.h */
