/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2018] Arm Limited (or its affiliates).                 *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
@file
@brief This file contains definitions of the CryptoCell hash APIs.

@defgroup cc_hash_defs CryptoCell hash API definitions
@brief Contains CryptoCell hash-API definitions.

@{
@ingroup cc_hash

*/

#ifndef CC_HASH_DEFS_H
#define CC_HASH_DEFS_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_types.h"
#include "cc_error.h"
#include "cc_hash_defs_proj.h"

/************************ Defines ******************************/

/*! The size of the hash result in words. The maximal size for SHA-512 is 512 bits. */
#define CC_HASH_RESULT_SIZE_IN_WORDS    16

/*! The size of the MD5 digest result in Bytes. */
#define CC_HASH_MD5_DIGEST_SIZE_IN_BYTES 16

/*! The size of the MD5 digest result in words. */
#define CC_HASH_MD5_DIGEST_SIZE_IN_WORDS 4

/*! The size of the SHA-1 digest result in Bytes. */
#define CC_HASH_SHA1_DIGEST_SIZE_IN_BYTES 20

/*! The size of the SHA-1 digest result in words. */
#define CC_HASH_SHA1_DIGEST_SIZE_IN_WORDS 5

/*! The size of the SHA-224 digest result in words. */
#define CC_HASH_SHA224_DIGEST_SIZE_IN_WORDS 7

/*! The size of the SHA-256 digest result in words. */
#define CC_HASH_SHA256_DIGEST_SIZE_IN_WORDS 8

/*! The size of the SHA-384 digest result in words. */
#define CC_HASH_SHA384_DIGEST_SIZE_IN_WORDS 12

/*! The size of the SHA-512 digest result in words. */
#define CC_HASH_SHA512_DIGEST_SIZE_IN_WORDS 16

/*! The size of the SHA-256 digest result in Bytes. */
#define CC_HASH_SHA224_DIGEST_SIZE_IN_BYTES 28

/*! The size of the SHA-256 digest result in Bytes. */
#define CC_HASH_SHA256_DIGEST_SIZE_IN_BYTES 32

/*! The size of the SHA-384 digest result in Bytes. */
#define CC_HASH_SHA384_DIGEST_SIZE_IN_BYTES 48

/*! The size of the SHA-512 digest result in Bytes. */
#define CC_HASH_SHA512_DIGEST_SIZE_IN_BYTES 64

/*! The size of the SHA-1 hash block in words. */
#define CC_HASH_BLOCK_SIZE_IN_WORDS 16

/*! The size of the SHA-1 hash block in Bytes. */
#define CC_HASH_BLOCK_SIZE_IN_BYTES 64

/*! The size of the SHA-2 hash block in words. */
#define CC_HASH_SHA512_BLOCK_SIZE_IN_WORDS  32

/*! The size of the SHA-2 hash block in Bytes. */
#define CC_HASH_SHA512_BLOCK_SIZE_IN_BYTES  128

/*! The maximal data size for the update operation. */
#define CC_HASH_UPDATE_DATA_MAX_SIZE_IN_BYTES (1 << 29)


/************************ Enums ********************************/

/*! The hash operation mode. */
typedef enum {
    CC_HASH_SHA1_mode          = 0,    /*!< SHA-1. */
    CC_HASH_SHA224_mode        = 1,    /*!< SHA-224. */
    CC_HASH_SHA256_mode        = 2,    /*!< SHA-256. */
    CC_HASH_SHA384_mode        = 3,    /*!< SHA-384. */
    CC_HASH_SHA512_mode        = 4,    /*!< SHA-512. */
    CC_HASH_MD5_mode           = 5,    /*!< MD5. */
    CC_HASH_NONE_mode          = 6,    /*!< NONE. */
    /*! The number of hash modes. */
    CC_HASH_NumOfModes,
    /*! Reserved. */
    CC_HASH_OperationModeLast= 0x7FFFFFFF,

}CCHashOperationMode_t;

/************************ Typedefs  *****************************/

/*! The hash result buffer. */
typedef uint32_t CCHashResultBuf_t[CC_HASH_RESULT_SIZE_IN_WORDS];

/************************ Structs  ******************************/
/*! @brief The context prototype of the user.

The argument type that is passed by the user to the hash APIs.
The context saves the state of the operation, and must be saved by the user
until the end of the API flow.
*/
typedef struct CCHashUserContext_t {
    /*! The internal buffer */
    uint32_t buff[CC_HASH_USER_CTX_SIZE_IN_WORDS];
}CCHashUserContext_t;


#ifdef __cplusplus
}
#endif

#endif /* #ifndef CC_HASH_DEFS_H */

/**
@}
 */

