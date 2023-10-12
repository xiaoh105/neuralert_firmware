/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/
#ifndef MBEDTLS_HASH_COMMON_H
#define MBEDTLS_HASH_COMMON_H
#include "mbedtls_common.h"
#ifdef __cplusplus
extern "C"
{
#endif


uint32_t   mbedtls_hashUpdate(void   *pHashUserCtx,
                        uint8_t     *pDataIn,
                        size_t      dataInSize);

void mbedtls_sha_init_internal( void *ctx );

int mbedtls_sha_process_internal( void *ctx, const unsigned char *data );

int mbedtls_sha_finish_internal( void *ctx );

int mbedtls_sha_update_internal( void *ctx, const unsigned char *input, size_t ilen );

int mbedtls_sha_starts_internal( void *ctx, hashMode_t mode);




#ifdef __cplusplus
}
#endif

#endif  /* MBEDTLS_HASH_COMMON_H */
