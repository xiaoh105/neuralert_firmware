/*****************************************************************************
 * The confidential and proprietary information contained in this file may   *
 * only be used by a person authorised under and to the extent permitted     *
 * by a subsisting licensing agreement from ARM Limited or its affiliates.   *
 *  (C) COPYRIGHT [2017] ARM Limited or its affiliates.                      *
 *      ALL RIGHTS RESERVED                                                  *
 * This entire notice must be reproduced on all copies of this file          *
 * and copies of this file may only be made by a person if such person is    *
 * permitted to do so under the terms of a subsisting license agreement      *
 * from ARM Limited or its affiliates.                                       *
 *****************************************************************************/

#define CC_PAL_LOG_CUR_COMPONENT CC_LOG_MASK_CC_API

#include "cc_pal_types.h"
#include "cc_pal_mem.h"
#include "ccm.h"
#include "cc_aesccm_error.h"
#include "mbedtls_cc_ccm_star.h"
#include "mbedtls_ccm_internal.h"

#pragma GCC diagnostic ignored "-Wsign-conversion"

/************************ Defines ******************************/

/************************ Public Functions **********************/
void mbedtls_ccm_star_init(mbedtls_ccm_context *ctx)
{
    mbedtls_ccm_init_int(ctx);
}

int mbedtls_ccm_star_setkey(mbedtls_ccm_context *ctx, mbedtls_cipher_id_t cipher, const unsigned char *key, unsigned int keybits)
{
    return mbedtls_ccm_setkey_int(ctx, cipher, key, keybits);
}

void mbedtls_ccm_star_free(mbedtls_ccm_context *ctx)
{
    mbedtls_ccm_free_int(ctx);
}

int mbedtls_ccm_star_encrypt_and_tag(mbedtls_ccm_context *ctx,
                                     size_t length,
                                     const unsigned char *iv,
                                     size_t iv_len,
                                     const unsigned char *add,
                                     size_t add_len,
                                     const unsigned char *input,
                                     unsigned char *output,
                                     unsigned char *tag,
                                     size_t tag_len)
{
    return mbedtls_ccm_encrypt_and_tag_int(ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len, MBEDTLS_AESCCM_MODE_STAR);
}

int mbedtls_ccm_star_auth_decrypt(mbedtls_ccm_context *ctx,
                                  size_t length,
                                  const unsigned char *iv,
                                  size_t iv_len,
                                  const unsigned char *add,
                                  size_t add_len,
                                  const unsigned char *input,
                                  unsigned char *output,
                                  const unsigned char *tag,
                                  size_t tag_len)
{
    return mbedtls_ccm_auth_decrypt_int(ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len, MBEDTLS_AESCCM_MODE_STAR);

}

int mbedtls_ccm_star_nonce_generate(unsigned char * src_addr, uint32_t frame_counter, uint8_t size_of_t, unsigned char * nonce_buf)
{
    CCError_t rc = CC_OK;
    uint8_t securityLevelField = 0;

    if ((src_addr == NULL) || (nonce_buf == NULL))
    {
        return CC_AESCCM_ILLEGAL_PARAMETER_PTR_ERROR;
    }

    if (mbedtls_ccm_get_security_level(size_of_t, &securityLevelField) != CC_OK)
    {
        return rc;
    }

    /*
     The nonce structure for AES-CCM* is defined in ieee-802.15.4-2011, Figure 61:
     Source address (8) | Frame counter (4) | Security lvel (1)
     */

    CC_PalMemCopy(nonce_buf, src_addr, MBEDTLS_AESCCM_STAR_SOURCE_ADDRESS_SIZE_BYTES);
    CC_PalMemCopy(nonce_buf + MBEDTLS_AESCCM_STAR_SOURCE_ADDRESS_SIZE_BYTES, &frame_counter, sizeof(uint32_t));
    nonce_buf[MBEDTLS_AESCCM_STAR_NONCE_SIZE_BYTES - 1] = securityLevelField;

    return CC_OK;
}
