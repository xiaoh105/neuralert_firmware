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
@brief This file contains the CryptoCell random-number generation APIs.

The random-number generation module implements <em>NIST Special Publication 800-90A:
Recommendation for Random Number Generation Using Deterministic Random Bit Generators.</em>

@defgroup cc_rnd CryptoCell random-number generation APIs.
@brief Contains the CryptoCell random-number generation APIs.

@{
@ingroup cryptocell_api
*/

#ifndef _CC_RND_COMMON_H
#define _CC_RND_COMMON_H

#include "cc_error.h"
#include "cc_aes_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/* RND seed and additional input sizes */
/*! The maximal size of the random seed in words. */
#define CC_RND_SEED_MAX_SIZE_WORDS                  12
#ifndef USE_MBEDTLS_CRYPTOCELL
#ifndef CC_RND_ADDITINAL_INPUT_MAX_SIZE_WORDS
/*! The maximal size of the additional input-data in words. */
#define CC_RND_ADDITINAL_INPUT_MAX_SIZE_WORDS   CC_RND_SEED_MAX_SIZE_WORDS
#endif
#endif
/* maximal requested size counter (12 bits active) - maximal count
of generated random 128 bit blocks allowed per one request of
Generate function according NIST 800-90 it is (2^12 - 1) = 0x3FFFF */
/* Max size for one RNG generation (in bits) =
  max_num_of_bits_per_request = 2^19 (FIPS 800-90 Tab.3) */
/*! The maximal size of the generated vector in bits. */
#define CC_RND_MAX_GEN_VECTOR_SIZE_BITS       0x7FFFF
/*! The maximal size of the generated random vector in Bytes. */
#define CC_RND_MAX_GEN_VECTOR_SIZE_BYTES    0xFFFF
/*! The maximal size of the generated vector in Bytes. */
#define CC_RND_REQUESTED_SIZE_COUNTER  0x3FFFF

/*   Definitions of temp buffer for RND_DMA  */
/*******************************************************************/
/*   Definitions of temp buffer for DMA  */
/*! The size of the temporary buffer in words. */
#define CC_RND_WORK_BUFFER_SIZE_WORDS 1528

/*! The definition of the RAM buffer, for internal use in instantiation or reseeding operations. */
typedef struct
{
    /*! The internal buffer. */
    uint32_t ccRndIntWorkBuff[CC_RND_WORK_BUFFER_SIZE_WORDS];
}CCRndWorkBuff_t;


/* RND source buffer inner (entrpopy) offset       */
/*! The definition of the internal offset in words. */
#define CC_RND_TRNG_SRC_INNER_OFFSET_WORDS    2
/*! The definition of the internal offset in Bytes. */
#define CC_RND_TRNG_SRC_INNER_OFFSET_BYTES    (CC_RND_TRNG_SRC_INNER_OFFSET_WORDS*sizeof(uint32_t))


/************************ Enumerators  ****************************/

/*! The definition of the random operation modes. */
typedef  enum
{
    CC_RND_FE  = 1,   /*!< HW entropy estimation: 80090B or full. */
    CC_RND_ModeLast = 0x7FFFFFFF, /*! Reserved. */
} CCRndMode_t;


/************************ Structs  *****************************/


/* The internal state of DRBG mechanism based on AES CTR and CBC-MAC
   algorithms. It is set as global data defined by the following
   structure  */
/*!
@brief The structure for the RND state.

This includes internal data that must be saved by the user between boots.
*/
typedef  struct
{
#ifndef USE_MBEDTLS_CRYPTOCELL
    /* Seed buffer, consists from concatenated Key||V: max size 12 words */
    /*! The random-seed buffer. */
    uint32_t  Seed[CC_RND_SEED_MAX_SIZE_WORDS];
    /* Previous value for continuous test */
    /*! The previous random data, used for continuous test. */
    uint32_t  PreviousRandValue[CC_AES_CRYPTO_BLOCK_SIZE_IN_WORDS];

    /* AdditionalInput buffer max size = seed max size words + 4w for padding*/
    /*! The previous additional-input buffer. */
    uint32_t  PreviousAdditionalInput[CC_RND_ADDITINAL_INPUT_MAX_SIZE_WORDS+3];
    /*! The additional-input buffer. */
    uint32_t  AdditionalInput[CC_RND_ADDITINAL_INPUT_MAX_SIZE_WORDS+4];
    /*! The size of the additional input in words. */
    uint32_t  AddInputSizeWords; /* size of additional data set by user, words   */

    /*! The size of the entropy source in words. */
    uint32_t  EntropySourceSizeWords;

    /*! The Reseed counter (32-bit active). Indicates the number of requests for entropy
    since instantiation or reseeding. */
    uint32_t  ReseedCounter;

    /*! The key size according to security strength:<ul><li>128 bits: 4 words.</li><li>256 bits: 8 words.</li></ul> */
    uint32_t KeySizeWords;
    /* State flag (see definition of StateFlag above), containing bit-fields, defining:
    - b'0: instantiation steps:   0 - not done, 1 - done;
    - 2b'9,8: working or testing mode: 0 - working, 1 - KAT DRBG test, 2 -
      KAT TRNG test;
    b'16: flag defining is Previous random valid or not:
            0 - not valid, 1 - valid */
    /*! The state flag used internally in the code. */
    uint32_t StateFlag;

    /* validation tag */
    /*! The validation tag used internally in the code. */
    uint32_t ValidTag;

    /*! The size of the RND source entropy in bits. */
    uint32_t  EntropySizeBits;



#endif

    /*! The TRNG process state used internally in the code */
    uint32_t TrngProcesState;

} CCRndState_t;


/*! The RND vector-generation function pointer. */
typedef int (*CCRndGenerateVectWorkFunc_t)(        \
                void              *rndState_ptr, /*!< A pointer to the RND-state context. */   \
                unsigned char     *out_ptr,         /*!< A pointer to the output buffer. */ \
                size_t            outSizeBytes   /*!< The size of the output in Bytes. */  );


/*! The definition of the RND context that includes the CryptoCell RND state structure, and a function pointer for the RND-generation function. */
typedef  struct
{


       void *   rndState; /*!< A pointer to the internal state of the RND.
                               \note This pointer should be allocated in a physical and
                               contiguous memory, accessible to the CryptoCell DMA.
                               This pointer should be allocated and assigned before calling CC_LibInit().
                               */
       void *   entropyCtx; /*!< A pointer to the entropy context.
                               \note This pointer should be allocated and assigned before calling CC_LibInit().
*/
       CCRndGenerateVectWorkFunc_t rndGenerateVectFunc; /*!< A pointer to the user-given function for generation of a random vector. */
} CCRndContext_t;





/*****************************************************************************/
/**********************        Public Functions      *************************/
/*****************************************************************************/


/****************************************************************************************/
/*!

@brief This function sets the RND vector-generation function into the RND context.

It is called as part of Arm TrustZone CryptoCell library initialization, to set
the RND vector generation function into the primary RND context.

\note It must be called before any other API that requires the RND context as parameter.

@return \c CC_OK on success.
@return A non-zero value from cc_rnd_error.h on failure.
*/
CCError_t CC_RndSetGenerateVectorFunc(
            CCRndContext_t *rndContext_ptr,                 /*!< [in/out] A pointer to the RND context buffer allocated by the user,
                                                                          which is used to maintain the RND state, as well as pointers
                                                                          to the functions used for random vector generation. */
            CCRndGenerateVectWorkFunc_t rndGenerateVectFunc /*!< [in] A pointer to the \c CC_RndGenerateVector random-vector-generation function. */
);




#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CC_RND_COMMON_H */

/**
@}
 */

