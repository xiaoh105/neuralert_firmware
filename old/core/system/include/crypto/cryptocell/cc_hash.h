/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
* 	(C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.	     *
*	    ALL RIGHTS RESERVED						     *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.					     *
*****************************************************************************/

/*! 
@file 
@brief This file contains all the enums and definitions
that are used for the CryptoCell HASH APIs, as well as the APIs themselves.
@defgroup cc_hash CryptoCell HASH APIs
@{
@ingroup cryptocell_api


This product supports the following HASH algorithms (or modes, according to product):
<ul><li> CC_HASH_MD5_mode (producing 16 byte output).</li>
<li> CC_HASH_SHA1_mode (producing 20 byte output).</li>
<li> CC_HASH_SHA224_mode (producing 28 byte output).</li>
<li> CC_HASH_SHA256_mode (producing 32 byte output).</li>
<li> CC_HASH_SHA384_mode (producing 48 byte output).</li>
<li> CC_HASH_SHA512_mode (producing 64 byte output).</li></ul>

HASH calculation can be performed in either of the following two modes of operation:
<ul><li> Integrated operation - Processes all data in a single function call. This flow is applicable when all data is available prior to the 
	 cryptographic operation.</li>
<li> Block operation - Processes a subset of the data buffers, and is called multiple times in a sequence. This flow is applicable when the 
     next data buffer becomes available only during/after processing of the current data buffer.</li></ul>

The following is a typical HASH Block operation flow:
<ol><li> ::CC_HashInit - this function initializes the HASH machine on the CryptoCell level by setting the context pointer that is used on the entire 
	 HASH operation.</li>
<li> ::CC_HashUpdate - this function runs a HASH operation on a block of data allocated by the user. This function may be called as many times 
     as required.</li>
<li> ::CC_HashFinish - this function ends the HASH operation. It returns the digest result and clears the context.</li></ol>
*/

#ifndef _CC_HASH_H
#define _CC_HASH_H

#include "cc_pal_types.h"
#include "cc_error.h"
#include "cc_hash_defs.h"

#ifndef USE_MBEDTLS_CRYPTOCELL

#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/*! The maximal hash result is 512 bits for SHA512. */
#define CC_HASH_RESULT_SIZE_IN_WORDS	16

/*! MD5 digest result size in bytes */
#define CC_HASH_MD5_DIGEST_SIZE_IN_BYTES 16

/*! MD5 digest result size in words */
#define CC_HASH_MD5_DIGEST_SIZE_IN_WORDS 4

/*! SHA-1 digest result size in bytes */
#define CC_HASH_SHA1_DIGEST_SIZE_IN_BYTES 20

/*! SHA-1 digest result size in words */
#define CC_HASH_SHA1_DIGEST_SIZE_IN_WORDS 5

/*! SHA-224 digest result size in words */
#define CC_HASH_SHA224_DIGEST_SIZE_IN_WORDS 7

/*! SHA-256 digest result size in words */
#define CC_HASH_SHA256_DIGEST_SIZE_IN_WORDS 8

/*! SHA-384 digest result size in words */
#define CC_HASH_SHA384_DIGEST_SIZE_IN_WORDS 12

/*! SHA-512 digest result size in words */
#define CC_HASH_SHA512_DIGEST_SIZE_IN_WORDS 16

/*! SHA-256 digest result size in bytes */
#define CC_HASH_SHA224_DIGEST_SIZE_IN_BYTES 28

/*! SHA-256 digest result size in bytes */
#define CC_HASH_SHA256_DIGEST_SIZE_IN_BYTES 32

/*! SHA-384 digest result size in bytes */
#define CC_HASH_SHA384_DIGEST_SIZE_IN_BYTES 48

/*! SHA-512 digest result size in bytes */
#define CC_HASH_SHA512_DIGEST_SIZE_IN_BYTES 64 

/*! SHA1 hash block size in words */
#define CC_HASH_BLOCK_SIZE_IN_WORDS 16

/*! SHA1 hash block size in bytes */
#define CC_HASH_BLOCK_SIZE_IN_BYTES 64

/*! SHA2 hash block size in words */
#define CC_HASH_SHA512_BLOCK_SIZE_IN_WORDS	32

/*! SHA2 hash block size in bytes */
#define CC_HASH_SHA512_BLOCK_SIZE_IN_BYTES	128

/*! Maximal data size for update operation. */
#define CC_HASH_UPDATE_DATA_MAX_SIZE_IN_BYTES (1 << 29)


/************************ Enums ********************************/

/*! 
HASH operation mode 
*/

/************************ Typedefs  *****************************/

/*! HASH result buffer. */
typedef uint32_t CCHashResultBuf_t[CC_HASH_RESULT_SIZE_IN_WORDS];

/************************ Structs  ******************************/
/*! The user's context prototype - the argument type that is passed by the user 
   to the HASH APIs. The context saves the state of the operation and must be saved by the user 
   till the end of the APIs flow. */

/************************ Public Variables **********************/

/************************ Public Functions **********************/


/************************************************************************************************/
/*!
@brief This function initializes the HASH machine and the HASH Context. 

It receives as input a pointer to store the context handle to the HASH Context, 
and initializes the HASH Context with the cryptographic attributes that are needed for the HASH block operation (initializes H's value for the HASH algorithm).

@return CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
*/
CIMPORT_C CCError_t CC_HashInit(
                        CCHashUserContext_t     *ContextID_ptr,         /*!< [in]  Pointer to the HASH context buffer allocated by the user that is used 
										for the HASH machine operation. */
                        CCHashOperationMode_t  OperationMode           /*!< [in]  One of the supported HASH modes, as defined in CCHashOperationMode_t. */
);

/************************************************************************************************/
/*!
@brief This function processes a block of data to be HASHed. 

It updates a HASH Context that was previously initialized by CC_HashInit or updated by a previous call to CC_HashUpdate.

@return CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
*/

CIMPORT_C CCError_t CC_HashUpdate(
                        CCHashUserContext_t  *ContextID_ptr,         /*!< [in]  Pointer to the HASH context buffer allocated by the user, which is used for the 
										   HASH machine operation. */
                        uint8_t                 *DataIn_ptr,            /*!< [in]  Pointer to the input data to be HASHed.
                                                                                   The size of the scatter/gather list representing the data buffer is limited to 
										   128 entries, and the size of each entry is limited to 64KB 
										   (fragments larger than 64KB are broken into fragments <= 64KB). */
                        size_t                  DataInSize             /*!< [in]  Byte size of the input data. Must be > 0.
                                                                                    If not a multiple of the HASH block size (64 for MD5, SHA-1 and SHA-224/256, 
										    128 for SHA-384/512), no further calls 
                                                                                    to CC_HashUpdate are allowed in this context, and only CC_HashFinish 
										    can be called to complete the computation. */
);

/************************************************************************************************/
/*!
@brief This function finalizes the hashing process of data block. 

It receives a handle to the HASH Context, which was previously initialized by CC_HashInit or by CC_HashUpdate. 
It "adds" a header to the data block according to the relevant HASH standard, and computes the final message digest.

@return CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
*/

CIMPORT_C CCError_t CC_HashFinish( 
                        CCHashUserContext_t  *ContextID_ptr,         /*!< [in]  Pointer to the HASH context buffer allocated by the user that is used for 
										   the HASH machine operation. */
                        CCHashResultBuf_t       HashResultBuff         /*!< [in]  Pointer to the word-aligned 64 byte buffer. The actual size of the HASH 
										   result depends on CCHashOperationMode_t. */
);


/************************************************************************************************/
/*!
@brief This function is a utility function that frees the context if the operation has failed.

The function executes the following major steps:
<ol><li> Checks the validity of all of the inputs of the function. </li>
<li> Clears the user's context.</li>
<li> Exits the handler with the OK code.</li></ol>

@return CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
*/

CIMPORT_C CCError_t  CC_HashFree(
                        CCHashUserContext_t  *ContextID_ptr         /*!< [in]  Pointer to the HASH context buffer allocated by the user that is used for 
										 the HASH machine operation. */
);


/************************************************************************************************/
/*!
@brief This function processes a single buffer of data.

The function allocates an internal HASH Context, and initializes it with the cryptographic attributes 
that are needed for the HASH block operation (initialize H's value for the HASH algorithm).
Then it processes the data block, calculating the HASH. Finally, it returns the data buffer's message digest.

@return CC_OK on success.
@return A non-zero value from cc_hash_error.h on failure.
 */

CIMPORT_C CCError_t CC_Hash  ( 
                        CCHashOperationMode_t  OperationMode,       /*!< [in]  One of the supported HASH modes, as defined in CCHashOperationMode_t. */
                        uint8_t                   *DataIn_ptr,          /*!< [in]  Pointer to the input data to be HASHed.
                                                                                   The size of the scatter/gather list representing the data buffer is limited 
										   to 128 entries, and the size of each entry is limited to 64KB 
										   (fragments larger than 64KB are broken into fragments <= 64KB). */
                        size_t                     DataSize,            /*!< [in]  The size of the data to be hashed in bytes. */
                        CCHashResultBuf_t         HashResultBuff       /*!< [out] Pointer to a word-aligned 64 byte buffer. The actual size of the HASH 
										   result depends on CCHashOperationMode_t. */
);



#ifdef __cplusplus
}
#endif

#endif //USE_MBEDTLS_CRYPTOCELL
/** 
@}
 */
#endif
