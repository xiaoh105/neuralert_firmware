/**
 ****************************************************************************************
 *
 * @file crypto_sample_cipher.c
 *
 * @brief Sample app of AES wrapper.
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
#include "mbedtls/cipher.h"

#define	CRYPTO_SAMPLE_CIPHER_TEST_SIZE	5
#define	CRYPTO_SAMPLE_CIPHER_TEST_ADD	24
#define	CRYPTO_SAMPLE_CIPHER_TEST_ASIZ	(CRYPTO_SAMPLE_CIPHER_TEST_SIZE + CRYPTO_SAMPLE_CIPHER_TEST_ADD + 2)

#define	crypto_sample_cipher_error_print(fmt)								\
    {																		\
        mbedtls_strerror(cipher_status, cipher_error, cipher_errlen);		\
        PRINTF("\r\n" #fmt " (%x, %s)\r\n", cipher_status, cipher_error);	\
    }

int crypto_sample_cipher_wrapper()
{
    mbedtls_cipher_type_t cipher_type = MBEDTLS_CIPHER_NONE;
    mbedtls_cipher_context_t cipher_ctx;
    mbedtls_cipher_info_t *cipherinfo = NULL;
    mbedtls_cipher_mode_t cipher_mode = MBEDTLS_MODE_NONE;
    char *cipher_name = NULL;

    unsigned char *cipher_key = NULL;
    int cipher_keylen = 0;
    unsigned char *cipher_iv = NULL;
    int cipher_ivlen = 0;
    unsigned int cipher_blksiz = 0;

    unsigned char *cipher_ad = NULL;
    size_t cipher_adlen = 0;
    unsigned char *cipher_tag = NULL;
    size_t cipher_taglen = 0;

    unsigned char *plain_in = NULL;
    size_t plain_inlen = 0;
    unsigned char *ciphertext = NULL;
    size_t ciphertext_len = 0;
    size_t ciphertext_finlen = 0;
    unsigned char *plain_out = NULL;
    size_t plain_outlen = 0;
    size_t plain_finlen = 0;

    int cipher_status = 0;
    size_t i = 0;
    int j = 0;
    int flag_pass = 0;
    char *cipher_error = NULL;
    int cipher_errlen = 128;
    unsigned char rand8b = 0;

    cipher_error = crypto_sample_calloc(cipher_errlen, sizeof(char));
    if (!cipher_error) {
        PRINTF("[%s] Failed to allocate error buffer\r\n", __func__);
        return -1;
    }

    for (cipher_type = MBEDTLS_CIPHER_AES_128_ECB ;
            cipher_type <= MBEDTLS_CIPHER_CAMELLIA_256_CCM ;
            cipher_type++) {

        flag_pass = FALSE;

        // Initialize a cipher_context as NONE.
        mbedtls_cipher_init(&cipher_ctx);

        // Retrieve the cipher-information structure associated with the given cipher type.
        cipherinfo = (mbedtls_cipher_info_t *)mbedtls_cipher_info_from_type(cipher_type);
        if (cipherinfo == NULL) {
            mbedtls_cipher_free(&cipher_ctx);
            continue;
        }

        // Initialize and fills the cipher-context structure with the appropriate values.
        mbedtls_cipher_setup(&cipher_ctx, cipherinfo);

        cipher_keylen = 0;
        cipher_ivlen = 0;
        cipher_blksiz = 0;
        cipher_mode = MBEDTLS_MODE_NONE;
        cipher_name = NULL;

        // Return the key length of the cipher.
        cipher_keylen = mbedtls_cipher_get_key_bitlen(&cipher_ctx);

        // Return the mode of operation for the cipher.
        cipher_mode   = mbedtls_cipher_get_cipher_mode(&cipher_ctx);

        // Return the size of the IV or nonce of the cipher, in Bytes.
        cipher_ivlen  = mbedtls_cipher_get_iv_size(&cipher_ctx);

        // Return the block size of the given cipher.
        cipher_blksiz = mbedtls_cipher_get_block_size(&cipher_ctx);

        // Return the name of the given cipher as a string.
        cipher_name = (char *)mbedtls_cipher_get_name(&cipher_ctx);

        if ((cipher_type >= MBEDTLS_CIPHER_CAMELLIA_128_GCM)
                && (cipher_type <= MBEDTLS_CIPHER_CAMELLIA_256_GCM)) {
            // cc312 SW doesn't support yet.
            mbedtls_cipher_free(&cipher_ctx);
            continue;
        } else if ((cipher_type == MBEDTLS_CIPHER_DES_ECB)
                   || (cipher_type == MBEDTLS_CIPHER_DES_EDE_ECB)
                   || (cipher_type == MBEDTLS_CIPHER_DES_EDE3_ECB)) {
            // DA16X SW doesn't support it.
            mbedtls_cipher_free(&cipher_ctx);
            continue;
        }

        cipher_blksiz = (cipher_blksiz > 128) ? 128 : cipher_blksiz;
        cipher_adlen = 0;
        cipher_taglen = 0;

        switch (cipher_mode) {
        case MBEDTLS_MODE_ECB: {
            plain_inlen = cipher_blksiz;
        }
        break;
        case MBEDTLS_MODE_CCM: {
            plain_inlen = cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_SIZE
                          + (da16x_random() % (cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ADD));
            cipher_adlen = 16;
            cipher_taglen = 16;
        }
        break;
        case MBEDTLS_MODE_GCM: {
            plain_inlen = cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_SIZE
                          + (da16x_random() % (cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ADD));
            cipher_adlen = 16;
            cipher_taglen = 16;
        }
        break;
        default: {
            plain_inlen = cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_SIZE + (da16x_random() %
                          (cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ADD));
        }
        break;
        }

        cipher_key = crypto_sample_calloc((cipher_keylen / 8), sizeof(unsigned char));
        if (!cipher_key) {
            PRINTF("[%s] Failed to allocate key buffer\r\n", __func__);
            goto cleanup;
        }

        cipher_iv = crypto_sample_calloc(cipher_ivlen, sizeof(unsigned char));
        if (!cipher_iv) {
            PRINTF("[%s] Failed to allocate iv buffer\r\n", __func__);
            goto cleanup;
        }

        plain_in = crypto_sample_calloc(plain_inlen, sizeof(unsigned char));
        if (!plain_in) {
            PRINTF("[%s] Failed to allocate plain input buffer\r\n", __func__);
            goto cleanup;
        }

        if (plain_inlen > (cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ASIZ)) {
            ciphertext = crypto_sample_calloc(plain_inlen, sizeof(unsigned char));
            if (!ciphertext) {
                PRINTF("[%s] Failed to allocate cipher text buffer\r\n", __func__);
                goto cleanup;
            }

            plain_out = crypto_sample_calloc(plain_inlen, sizeof(unsigned char));
            if (!plain_out) {
                PRINTF("[%s] Failed to allocate plain out buffer\r\n", __func__);
                goto cleanup;
            }
        } else {
            ciphertext = crypto_sample_calloc(cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ASIZ,
                                              sizeof(unsigned char));
            if (!ciphertext) {
                PRINTF("[%s] Failed to allocate cipher text buffer\r\n", __func__);
                goto cleanup;
            }

            plain_out = crypto_sample_calloc(cipher_blksiz * CRYPTO_SAMPLE_CIPHER_TEST_ASIZ,
                                             sizeof(unsigned char));
            if (!plain_out) {
                PRINTF("[%s] Failed to allocate plain out buffer\r\n", __func__);
                goto cleanup;
            }
        }

        if (cipher_adlen) {
            cipher_ad = crypto_sample_calloc(cipher_adlen, sizeof(unsigned char));
            if (!cipher_ad) {
                PRINTF("[%s] Failed to allocate additional data buffer\r\n", __func__);
                goto cleanup;
            }
        }

        if (cipher_taglen) {
            cipher_tag = crypto_sample_calloc(cipher_taglen, sizeof(unsigned char));
            if (!cipher_tag) {
                PRINTF("[%s] Failed to allocate tag buffer\r\n", __func__);
                goto cleanup;
            }
        }

        rand8b = (unsigned char)da16x_random();

        for (i = 0 ; i < plain_inlen ; i++) {
            plain_in[i] = (rand8b + i) % 255;
        }

        rand8b = (unsigned char)da16x_random();

        for (j = 0 ; j < (cipher_keylen / 8) ; j++) {
            cipher_key[j] = (rand8b + j) % 255;
        }

        rand8b = (unsigned char)da16x_random();

        for (j = 0 ; j < cipher_ivlen ; j++) {
            cipher_iv[j] = (rand8b + j) % 255;
        }

        rand8b = (unsigned char)da16x_random();

        for (i = 0 ; i < cipher_adlen ; i++ ) {
            cipher_ad[i] = (rand8b + i) % 255;
        }

        rand8b = (unsigned char)da16x_random();

        for (i = 0 ; i < cipher_taglen ; i++) {
            cipher_tag[i] = (rand8b + i) % 255;
        }

        ciphertext_len = 0;
        ciphertext_finlen = 0;
        plain_outlen = 0;
        plain_finlen = 0;

        PRINTF("* %s", cipher_name);

        PRINTF("(enc, ");

        if (cipher_adlen == 0) { // No CCM or GCM
            // Set the key to use with the given context.
            cipher_status = mbedtls_cipher_setkey(&cipher_ctx,
                                                  cipher_key, cipher_keylen,
                                                  MBEDTLS_ENCRYPT);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_setkey);
            }

            // Set the initialization vector (IV) or nonce.
            cipher_status = mbedtls_cipher_set_iv(&cipher_ctx,
                                                  cipher_iv, cipher_ivlen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_set_iv);
            }

            // Reset the cipher state.
            cipher_status = mbedtls_cipher_reset(&cipher_ctx);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_reset);
            }

            // Encrypt or decrypt using the given cipher context.
            cipher_status = mbedtls_cipher_update(&cipher_ctx,
                                                  plain_in, plain_inlen,
                                                  ciphertext, &ciphertext_len);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_update);
            }

            // Finish the operation.
            cipher_status = mbedtls_cipher_finish(&cipher_ctx,
                                                  &(ciphertext[ciphertext_len]),
                                                  &ciphertext_finlen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_finish);
            }
        } else {
            // Set the key to use with the given context.
            cipher_status = mbedtls_cipher_setkey(&cipher_ctx,
                                                  cipher_key, cipher_keylen,
                                                  MBEDTLS_ENCRYPT);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_setkey);
            }

            // Perform autenticated encryption (AEAD).
            cipher_status = mbedtls_cipher_auth_encrypt(&cipher_ctx,
                            cipher_iv, cipher_ivlen,
                            cipher_ad, cipher_adlen,
                            plain_in, plain_inlen,
                            ciphertext, &ciphertext_len,
                            cipher_tag, cipher_taglen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_auth_encrypt);
            }
        }

        PRINTF("dec): ");

        if (cipher_adlen == 0) { // No CCM or GCM
            // Set the key to use with the given context.
            cipher_status = mbedtls_cipher_setkey(&cipher_ctx,
                                                  cipher_key, cipher_keylen,
                                                  MBEDTLS_DECRYPT);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_setkey);
            }

            // Set the initialization vector (IV) or nonce.
            cipher_status = mbedtls_cipher_set_iv(&cipher_ctx,
                                                  cipher_iv, cipher_ivlen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_set_iv);
            }

            // Reset the cipher state.
            cipher_status = mbedtls_cipher_reset(&cipher_ctx);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_reset);
            }

            // Encrypt or decrypt using the given cipher context.
            cipher_status = mbedtls_cipher_update(&cipher_ctx,
                                                  ciphertext, (ciphertext_len + ciphertext_finlen),
                                                  plain_out, &plain_outlen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_update);
            }

            // Finish the operation.
            cipher_status = mbedtls_cipher_finish(&cipher_ctx,
                                                  &(plain_out[plain_outlen]), &plain_finlen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_finish);
            }
        } else {
            // Set the key to use with the given context.
            cipher_status = mbedtls_cipher_setkey(&cipher_ctx,
                                                  cipher_key, cipher_keylen,
                                                  MBEDTLS_DECRYPT);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_setkey);
            }

            // Perform autenticated decryption (AEAD).
            cipher_status = mbedtls_cipher_auth_decrypt(&cipher_ctx,
                            cipher_iv, cipher_ivlen,
                            cipher_ad, cipher_adlen,
                            ciphertext, ciphertext_len,
                            plain_out, &plain_outlen,
                            cipher_tag, cipher_taglen);
            if (cipher_status) {
                crypto_sample_cipher_error_print(mbedtls_cipher_auth_decrypt);
            }
        }

        // Compare result.
        if ((plain_inlen != (plain_outlen + plain_finlen))
                || (memcmp(plain_in, plain_out, plain_inlen) != 0)) {
            flag_pass = FALSE;
            PRINTF("failed\r\n");
        } else {
            flag_pass = TRUE;
            PRINTF("passed\r\n");
        }

        crypto_sample_hex_dump("  + plain_in: ", plain_in, plain_inlen);

        if ((cipher_keylen / 8) != 0) {
            crypto_sample_hex_dump("  + cipher_key: ", cipher_key, (cipher_keylen / 8));
        }

        crypto_sample_hex_dump("  + cipher_iv: ", cipher_iv, cipher_ivlen);

        crypto_sample_hex_dump("  + cipher_ad: ", cipher_ad, cipher_adlen);

        crypto_sample_hex_dump("  + cipher_tag: ", cipher_tag, cipher_taglen);

        crypto_sample_hex_dump("  + ciphertext: ",
                               ciphertext, (ciphertext_len + ciphertext_finlen));

        crypto_sample_hex_dump("  + plain_out: ", plain_out, (plain_outlen + plain_finlen));

cleanup:

        // Free and clear the cipher-specific context of ctx.
        mbedtls_cipher_free(&cipher_ctx);

        if (cipher_key) {
            crypto_sample_free(cipher_key);
            cipher_key = NULL;
        }

        if (cipher_iv) {
            crypto_sample_free(cipher_iv);
            cipher_iv = NULL;
        }

        if (plain_in) {
            crypto_sample_free(plain_in);
            plain_in = NULL;
        }

        if (ciphertext) {
            crypto_sample_free(ciphertext);
            ciphertext = NULL;
        }

        if (plain_out) {
            crypto_sample_free(plain_out);
            plain_out = NULL;
        }

        if (cipher_ad) {
            crypto_sample_free(cipher_ad);
            cipher_ad = NULL;
        }

        if (cipher_tag) {
            crypto_sample_free(cipher_tag);
            cipher_tag = NULL;
        }

        if (flag_pass == FALSE) {
            break;
        }
    }

    if (cipher_error) {
        crypto_sample_free(cipher_error);
    }

    return (flag_pass == FALSE) ? 1 : 0;
}

/*
 * Sample Functions for cipher
 */
void crypto_sample_cipher(void *param)
{
    DA16X_UNUSED_ARG(param);

    crypto_sample_cipher_wrapper();

    return ;
}

/* EOF */
