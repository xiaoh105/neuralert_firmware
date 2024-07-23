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

/*!
@file
@brief This file contains all of the CryptoCell ChaCha APIs, their enums and definitions.

@defgroup cc_chacha CryptoCell ChaCha APIs
@brief Contains all CryptoCell ChaCha APIs.

@{
@ingroup cryptocell_api

*/

#ifndef _MBEDTLS_CC_CHACHA_H
#define _MBEDTLS_CC_CHACHA_H


#include "cc_pal_types.h"
#include "cc_error.h"


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/*! The size of the ChaCha user-context in words. */
#define CC_CHACHA_USER_CTX_SIZE_IN_WORDS 17

/*! The size of the ChaCha block in words. */
#define CC_CHACHA_BLOCK_SIZE_IN_WORDS 16
/*! The size of the ChaCha block in Bytes. */
#define CC_CHACHA_BLOCK_SIZE_IN_BYTES  (CC_CHACHA_BLOCK_SIZE_IN_WORDS * sizeof(uint32_t))

/*! The maximal size of the nonce buffer in words. */
#define CC_CHACHA_NONCE_MAX_SIZE_IN_WORDS   3
/*! The maximal size of the nonce buffer in Bytes. */
#define CC_CHACHA_NONCE_MAX_SIZE_IN_BYTES  (CC_CHACHA_NONCE_MAX_SIZE_IN_WORDS * sizeof(uint32_t))

/*! The maximal size of the ChaCha key in words. */
#define CC_CHACHA_KEY_MAX_SIZE_IN_WORDS 8
/*! The maximal size of the ChaCha key in Bytes. */
#define CC_CHACHA_KEY_MAX_SIZE_IN_BYTES (CC_CHACHA_KEY_MAX_SIZE_IN_WORDS * sizeof(uint32_t))

/************************ Enums ********************************/

/*! The ChaCha operation:<ul><li>Encrypt</li><li>Decrypt</li></ul>. */
typedef enum {
    /*! A ChaCha decrypt operation. */
    CC_CHACHA_Decrypt = 0,
    /*! A ChaCha encrypt operation. */
    CC_CHACHA_Encrypt = 1,
    /*! The maximal number of encrypt or decrypt operations for the ChaCha engine. */
    CC_CHACHA_EncryptNumOfOptions,
    /*! Reserved. */
    CC_CHACHA_EncryptModeLast = 0x7FFFFFFF,

}mbedtls_chacha_encrypt_mode_t;

/*! The allowed nonce-size values of the ChaCha engine in bits. */
typedef enum {
    /*! A 64-bit nonce size. */
        CC_CHACHA_Nonce64BitSize = 0,
    /*! A 96-bit nonce size. */
        CC_CHACHA_Nonce96BitSize = 1,
    /*! The maximal number of nonce sizes for the ChaCha engine. */
        CC_CHACHA_NonceSizeNumOfOptions,
    /*! Reserved. */
        CC_CHACHA_NonceSizeLast = 0x7FFFFFFF,

}mbedtls_chacha_nonce_size_t;

/************************ Typedefs  ****************************/

/*! The definition of the 12-Byte array of the nonce buffer. */
typedef uint8_t mbedtls_chacha_nonce[CC_CHACHA_NONCE_MAX_SIZE_IN_BYTES];

/*! The definition of the key buffer of the ChaCha engine. */
typedef uint8_t mbedtls_chacha_key[CC_CHACHA_KEY_MAX_SIZE_IN_BYTES];


/************************ context Structs  ******************************/

/*!
@brief The context prototype of the user.

The argument type that is passed by the user to the ChaCha API.

The context saves the state of the operation. It must be saved by the user
until the end of the API flow, for example, until ::mbedtls_chacha_free is called.
*/
typedef struct mbedtls_chacha_user_context {
    /* The allocated buffer must be double the size of the actual context
     * + 1 word for offset management */
    /*! The context buffer for internal use */
    uint32_t buff[CC_CHACHA_USER_CTX_SIZE_IN_WORDS];
}mbedtls_chacha_user_context;

/************************ Public Variables **********************/


/************************ Public Functions **********************/

/****************************************************************************************************/

/*!
@brief This function initializes the context for ChaCha-engine operations.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha_init(
        mbedtls_chacha_user_context    *pContextID,        /*!< [in]  A pointer to the ChaCha context buffer that is allocated by the user and used for the ChaCha operation. */
        mbedtls_chacha_nonce          pNonce,           /*!< [in]  A buffer containing a nonce. */
        mbedtls_chacha_nonce_size_t      nonceSize,         /*!< [in]  An enumerator defining the nonce size. Valid values are:<ul><li>64bit</li><li>96bit</li></ul>. */
        mbedtls_chacha_key            pKey,               /*!< [in]  A pointer to the key buffer of the user. */
        uint32_t                     initialCounter,     /*!< [in]  An initial counter. */
        mbedtls_chacha_encrypt_mode_t    EncryptDecryptFlag  /*!< [in]  A flag specifying whether the ChaCha engine should perform an Encrypt operation or a Decrypt operation. */
);


/*!
@brief This function processes aligned blocks of the ChaCha engine.

The data-in size should be a multiple of the ChaCha block size.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha_block(
        mbedtls_chacha_user_context    *pContextID,        /*!< [in] A pointer to the context buffer. */
                        uint8_t                     *pDataIn,           /*!< [in]  A pointer to the buffer of the input data to the ChaCha engine.
                                                                                   The pointer does not need to be aligned. Must not be null. */
                        size_t                      dataInSize,         /*!< [in]  The size of the input data.
                                                                                   Must be a multiple of ::CC_CHACHA_BLOCK_SIZE_IN_BYTES Bytes, and must not be zero. */
                        uint8_t                     *pDataOut           /*!< [out] A pointer to the buffer of the output data from the ChaCha engine.
                                                                                   The pointer does not need to be aligned. Must not be null. */
);


/*!
@brief This function processes the remaining ChaCha data.

The data-in size should be smaller than the ChaCha block size.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha_finish(
        mbedtls_chacha_user_context    *pContextID,     /*!< [in]  A pointer to the context buffer. */
        uint8_t                     *pDataIn,           /*!< [in]  A pointer to the buffer of the input data to the ChaCha engine.
                                                                   The pointer does not need to be aligned. If dataInSize = 0,
                                                                   an input buffer is not required. */
        size_t                      dataInSize,         /*!< [in]  The size of the input data. Valid values are:<ul><li>Zero</li>
                                                                   <li>Values that are not multiples of ::CC_CHACHA_BLOCK_SIZE_IN_BYTES</li></ul>. */
        uint8_t                     *pDataOut           /*!< [out] A pointer to the buffer of the output data from the ChaCha engine.
                                                                   The pointer does not need to be aligned. If dataInSize = 0,
                                                                   an output buffer is not required. */
);


/*!
@brief This function frees the context used for ChaCha operations.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha_free(
        mbedtls_chacha_user_context *pContextID    /*!< [in] A pointer to the context buffer. */
);


/*!
@brief This function performs the ChaCha operation in one integrated process.

@return \c CC_OK on success.
@return A non-zero value on failure as defined in mbedtls_cc_chacha_error.h.
*/
CIMPORT_C CCError_t  mbedtls_chacha(
        mbedtls_chacha_nonce           pNonce,              /*!< [in]  A buffer containing a nonce. */
        mbedtls_chacha_nonce_size_t    nonceSize,           /*!< [in]  An enumerator defining the size of the nonce.
                                                                       Valid values are:<ul><li>64bit</li><li>96bit</li></ul>. */
        mbedtls_chacha_key             pKey,                /*!< [in]  A pointer to the key buffer of the user. */
        uint32_t                       initialCounter,      /*!< [in]  An initial counter. */
        mbedtls_chacha_encrypt_mode_t  encryptDecryptFlag,  /*!< [in]  A flag specifying which operation the ChaCha engine should
                                                                       perform: encrypt or decrypt. */
        uint8_t                        *pDataIn,            /*!< [in]  A pointer to the buffer of the input-data to the ChaCha engine.
                                                                       The pointer does not need to be aligned. Must not be null. */
        size_t                         dataInSize,          /*!< [in]  The size of the input data. Must not be zero. */
        uint8_t                        *pDataOut            /*!< [out] A pointer to the buffer of the output data from the ChaCha.
                                                                       The pointer does not need to be aligned. Must not be null. */
);


/***********************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MBEDTLS_CC_CHACHA_H */

/**
@}
 */

