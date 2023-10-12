/****************************************************************************
 * The confidential and proprietary information contained in this file may   *
 * only be used by a person authorized under and to the extent permitted     *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.   *
 *  (C) COPYRIGHT [2017] ARM Limited or its affiliates.      *
 *      ALL RIGHTS RESERVED                          *
 * This entire notice must be reproduced on all copies of this file          *
 * and copies of this file may only be made by a person if such person is    *
 * permitted to do so under the terms of a subsisting license agreement      *
 * from ARM Limited or its affiliates.                       *
 *****************************************************************************/

/*
 * Definition of CCM:
 * http://csrc.nist.gov/publications/nistpubs/800-38C/SP800-38C_updated-July20_2007.pdf
 * RFC 3610 "Counter with CBC-MAC (CCM)"
 *
 * Related:
 * RFC 5116 "An Interface and Algorithms for Authenticated Encryption"
 */
#include "ccm_alt.h"

#if defined(MBEDTLS_CCM_C) && defined (MBEDTLS_CCM_ALT)

#include "cc_pal_types.h"
#include "mbedtls_common.h"
#include "cc_pal_abort.h"

#include "cc3x_sym/api/mbedtls_ccm_internal.h"
#include "mbedtls_ccm_common.h"

#include <string.h>

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

/************************ Public Functions **********************/
/*
 * Initialize context
 */
void mbedtls_ccm_init( mbedtls_ccm_context *ctx )
{
    mbedtls_ccm_init_int(ctx);
}

int mbedtls_ccm_setkey( mbedtls_ccm_context *ctx,
                        mbedtls_cipher_id_t cipher,
                        const unsigned char *key,
                        unsigned int keybits)
{
#ifdef	CCM_FCI_OPTIMIZE
	if (ctx == NULL || key == NULL)
	{
		return MBEDTLS_ERR_CCM_BAD_INPUT;
	}

	ctx->cipher = cipher;

	if (cipher != MBEDTLS_CIPHER_ID_AES)
	{
		int ret;
		const mbedtls_cipher_info_t *cipher_info;
		mbedtls_cipher_context_t *cipher_ctx;

		cipher_info = mbedtls_cipher_info_from_values( cipher, keybits, MBEDTLS_MODE_ECB );
		if( cipher_info == NULL )
			return( MBEDTLS_ERR_CCM_BAD_INPUT );

		if( cipher_info->block_size != 16 )
			return( MBEDTLS_ERR_CCM_BAD_INPUT );

		cipher_ctx = (mbedtls_cipher_context_t *)(&(ctx->buf[0]));

		mbedtls_cipher_free( cipher_ctx );

		if( ( ret = mbedtls_cipher_setup( cipher_ctx, cipher_info ) ) != 0 )
			return( ret );

		if( ( ret = mbedtls_cipher_setkey( cipher_ctx, key, keybits,
			MBEDTLS_ENCRYPT ) ) != 0 )
		{
			return( ret );
		}

		return( 0 );
	}
#endif	//CCM_FCI_OPTIMIZE
    return mbedtls_ccm_setkey_int(ctx, cipher, key, keybits);
}

/*
 * Free context
 */
void mbedtls_ccm_free( mbedtls_ccm_context *ctx )
{
#ifdef	CCM_FCI_OPTIMIZE
	if (ctx == NULL)
	{
		/*TODO: (adikac01) we should actually use a better error code here.*/
		CC_PalAbort("!!!!CCM context is NULL!!!\n");
	}

	if(ctx->cipher != MBEDTLS_CIPHER_ID_AES)
	{
		mbedtls_cipher_context_t *cipher_ctx;

		cipher_ctx = (mbedtls_cipher_context_t *)(&(ctx->buf[0]));
		mbedtls_cipher_free( cipher_ctx );

		mbedtls_zeroize_internal( ctx, sizeof( mbedtls_ccm_context ));
		return ;
	}
#endif	//CCM_FCI_OPTIMIZE
    mbedtls_ccm_free_int(ctx);
}

#ifdef	CCM_FCI_OPTIMIZE
#define FCI_CCM_ENCRYPT 0
#define FCI_CCM_DECRYPT 1

static int da16x_ccm_auth_crypt( mbedtls_ccm_context *ctx, int mode, size_t length,
                           const unsigned char *iv, size_t iv_len,
                           const unsigned char *add, size_t add_len,
                           const unsigned char *input, unsigned char *output,
                           unsigned char *tag, size_t tag_len );
#endif	//CCM_FCI_OPTIMIZE
/*
 * Authenticated encryption
 */
int mbedtls_ccm_encrypt_and_tag( mbedtls_ccm_context *ctx,
                                 size_t length,
                                 const unsigned char *iv,
                                 size_t iv_len,
                                 const unsigned char *add,
                                 size_t add_len,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 unsigned char *tag,
                                 size_t tag_len )
{
#ifdef	CCM_FCI_OPTIMIZE
	if((ctx != NULL) && (ctx->cipher != MBEDTLS_CIPHER_ID_AES))
	{
		return( da16x_ccm_auth_crypt( ctx, FCI_CCM_ENCRYPT, length, iv, iv_len,
			add, add_len, input, output, tag, tag_len ) );
	}
#endif	//CCM_FCI_OPTIMIZE
    return mbedtls_ccm_encrypt_and_tag_int(ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len, MBEDTLS_AESCCM_MODE_CCM);
}

/*
 * Authenticated decryption
 */
int mbedtls_ccm_auth_decrypt( mbedtls_ccm_context *ctx,
                              size_t length,
                              const unsigned char *iv,
                              size_t iv_len,
                              const unsigned char *add,
                              size_t add_len,
                              const unsigned char *input,
                              unsigned char *output,
                              const unsigned char *tag,
                              size_t tag_len )
{
#ifdef	CCM_FCI_OPTIMIZE
	if((ctx != NULL) && (ctx->cipher != MBEDTLS_CIPHER_ID_AES))
	{
		int ret;
		unsigned char check_tag[16];
		unsigned char i;
		int diff;

		if( ( ret = da16x_ccm_auth_crypt( ctx, FCI_CCM_DECRYPT, length,
			iv, iv_len, add, add_len,
			input, output, check_tag, tag_len ) ) != 0 )
		{
			return( ret );
		}

		/* Check tag in "constant-time" */
		for( diff = 0, i = 0; i < tag_len; i++ )
			diff |= tag[i] ^ check_tag[i];

		if( diff != 0 )
		{
			mbedtls_zeroize_internal( output, length );
			return( MBEDTLS_ERR_CCM_AUTH_FAILED );
		}

		return( 0 );
	}
#endif	//CCM_FCI_OPTIMIZE
    return mbedtls_ccm_auth_decrypt_int( ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len, MBEDTLS_AESCCM_MODE_CCM);
}

#ifdef	CCM_FCI_OPTIMIZE
/******************************************************************************
 *  FCI SW model
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
/*
 * Macros for common operations.
 * Results in smaller compiled code than static inline functions.
 */

/*
 * Update the CBC-MAC state in y using a block in b
 * (Always using b as the source helps the compiler optimise a bit better.)
 */
#define UPDATE_CBC_MAC                                                      \
    for( i = 0; i < 16; i++ )                                               \
        y[i] ^= b[i];                                                       \
                                                                            \
    if( ( ret = mbedtls_cipher_update( cipher_ctx, y, 16, y, &olen ) ) != 0 ) \
        return( ret );

/*
 * Encrypt or decrypt a partial block with CTR
 * Warning: using b for temporary storage! src and dst must not be b!
 * This avoids allocating one more 16 bytes buffer while allowing src == dst.
 */
#define CTR_CRYPT( dst, src, len  )                                            \
    if( ( ret = mbedtls_cipher_update( cipher_ctx, ctr, 16, b, &olen ) ) != 0 )  \
        return( ret );                                                         \
                                                                               \
    for( i = 0; i < len; i++ )                                                 \
        dst[i] = src[i] ^ b[i];

/*
 * Authenticated encryption or decryption
 */
static int da16x_ccm_auth_crypt( mbedtls_ccm_context *ctx, int mode, size_t length,
                           const unsigned char *iv, size_t iv_len,
                           const unsigned char *add, size_t add_len,
                           const unsigned char *input, unsigned char *output,
                           unsigned char *tag, size_t tag_len )
{
    int ret;
    unsigned char i;
    unsigned char q;
    size_t len_left, olen;
    unsigned char b[16];
    unsigned char y[16];
    unsigned char ctr[16];
    const unsigned char *src;
    unsigned char *dst;
    mbedtls_cipher_context_t *cipher_ctx;

    /*
     * Check length requirements: SP800-38C A.1
     * Additional requirement: a < 2^16 - 2^8 to simplify the code.
     * 'length' checked later (when writing it to the first block)
     */
    if( tag_len < 4 || tag_len > 16 || tag_len % 2 != 0 )
        return( MBEDTLS_ERR_CCM_BAD_INPUT );

    /* Also implies q is within bounds */
    if( iv_len < 7 || iv_len > 13 )
        return( MBEDTLS_ERR_CCM_BAD_INPUT );

    if( add_len > 0xFF00 )
        return( MBEDTLS_ERR_CCM_BAD_INPUT );

    q = 16 - 1 - (unsigned char) iv_len;

    cipher_ctx = (mbedtls_cipher_context_t *)(&(ctx->buf[0]));

    /*
     * First block B_0:
     * 0        .. 0        flags
     * 1        .. iv_len   nonce (aka iv)
     * iv_len+1 .. 15       length
     *
     * With flags as (bits):
     * 7        0
     * 6        add present?
     * 5 .. 3   (t - 2) / 2
     * 2 .. 0   q - 1
     */
    b[0] = 0;
    b[0] |= ( add_len > 0 ) << 6;
    b[0] |= ( ( tag_len - 2 ) / 2 ) << 3;
    b[0] |= q - 1;

    memcpy( b + 1, iv, iv_len );

    for( i = 0, len_left = length; i < q; i++, len_left >>= 8 )
        b[15-i] = (unsigned char)( len_left & 0xFF );

    if( len_left > 0 )
        return( MBEDTLS_ERR_CCM_BAD_INPUT );


    /* Start CBC-MAC with first block */
    memset( y, 0, 16 );
    UPDATE_CBC_MAC;

    /*
     * If there is additional data, update CBC-MAC with
     * add_len, add, 0 (padding to a block boundary)
     */
    if( add_len > 0 )
    {
        size_t use_len;
        len_left = add_len;
        src = add;

        memset( b, 0, 16 );
        b[0] = (unsigned char)( ( add_len >> 8 ) & 0xFF );
        b[1] = (unsigned char)( ( add_len      ) & 0xFF );

        use_len = len_left < 16 - 2 ? len_left : 16 - 2;
        memcpy( b + 2, src, use_len );
        len_left -= use_len;
        src += use_len;

        UPDATE_CBC_MAC;

        while( len_left > 0 )
        {
            use_len = len_left > 16 ? 16 : len_left;

            memset( b, 0, 16 );
            memcpy( b, src, use_len );
            UPDATE_CBC_MAC;

            len_left -= use_len;
            src += use_len;
        }
    }

    /*
     * Prepare counter block for encryption:
     * 0        .. 0        flags
     * 1        .. iv_len   nonce (aka iv)
     * iv_len+1 .. 15       counter (initially 1)
     *
     * With flags as (bits):
     * 7 .. 3   0
     * 2 .. 0   q - 1
     */
    ctr[0] = q - 1;
    memcpy( ctr + 1, iv, iv_len );
    memset( ctr + 1 + iv_len, 0, q );
    ctr[15] = 1;

    /*
     * Authenticate and {en,de}crypt the message.
     *
     * The only difference between encryption and decryption is
     * the respective order of authentication and {en,de}cryption.
     */
    len_left = length;
    src = input;
    dst = output;

    while( len_left > 0 )
    {
        size_t use_len = len_left > 16 ? 16 : len_left;

        if( mode == FCI_CCM_ENCRYPT )
        {
            memset( b, 0, 16 );
            memcpy( b, src, use_len );
            UPDATE_CBC_MAC;
        }

        CTR_CRYPT( dst, src, use_len );

        if( mode == FCI_CCM_DECRYPT )
        {
            memset( b, 0, 16 );
            memcpy( b, dst, use_len );
            UPDATE_CBC_MAC;
        }

        dst += use_len;
        src += use_len;
        len_left -= use_len;

        /*
         * Increment counter.
         * No need to check for overflow thanks to the length check above.
         */
        for( i = 0; i < q; i++ )
            if( ++ctr[15-i] != 0 )
                break;
    }

    /*
     * Authentication: reset counter and crypt/mask internal tag
     */
    for( i = 0; i < q; i++ )
        ctr[15-i] = 0;

    CTR_CRYPT( y, y, 16 );
    memcpy( tag, y, tag_len );

    return( 0 );
}
#endif	//CCM_FCI_OPTIMIZE

#endif /* defined(MBEDTLS_CCM_C) && defined (MBEDTLS_CCM_ALT) */
