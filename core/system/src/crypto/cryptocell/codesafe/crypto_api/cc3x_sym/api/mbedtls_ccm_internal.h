/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorized under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2017] ARM Limited or its affiliates.      *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef _MBEDTLS_CCM_INTERNAL_H
#define _MBEDTLS_CCM_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccm.h"

int mbedtls_ccm_get_security_level(uint8_t sizeOfT, uint8_t *pSecurityLevel);

void mbedtls_ccm_init_int(mbedtls_ccm_context *ctx);

int mbedtls_ccm_setkey_int(mbedtls_ccm_context *ctx, mbedtls_cipher_id_t cipher, const unsigned char *key, unsigned int keybits);

void mbedtls_ccm_free_int(mbedtls_ccm_context *ctx);

int mbedtls_ccm_encrypt_and_tag_int(mbedtls_ccm_context *ctx,
                                    size_t length,
                                    const unsigned char *iv,
                                    size_t iv_len,
                                    const unsigned char *add,
                                    size_t add_len,
                                    const unsigned char *input,
                                    unsigned char *output,
                                    unsigned char *tag,
                                    size_t tag_len,
                                    uint32_t ccmMode);

int mbedtls_ccm_auth_decrypt_int(mbedtls_ccm_context *ctx,
                                 size_t length,
                                 const unsigned char *iv,
                                 size_t iv_len,
                                 const unsigned char *add,
                                 size_t add_len,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 const unsigned char *tag,
                                 size_t tag_len,
                                 uint32_t ccmMode);
#ifdef __cplusplus
}
#endif

#endif /* _MBEDTLS_CCM_INTERNAL_H */
