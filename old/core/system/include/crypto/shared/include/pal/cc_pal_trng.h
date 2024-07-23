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
@brief This file contains APIs for retrieving TRNG user parameters.

@defgroup cc_pal_trng CryptoCell PAL TRNG APIs
@brief Contains APIs for retrieving TRNG user parameters.

@{
@ingroup cc_pal
*/

#ifndef _CC_PAL_TRNG_H
#define _CC_PAL_TRNG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_types.h"

#if (CC_CONFIG_TRNG_MODE==1)
/*!
\brief The random-generator parameters of CryptoCell for TRNG mode.

This is as defined in <em>NIST SP 90B: DRAFT Recommendation for
the Entropy Sources Used for Random Bit Generation</em>.
*/
typedef struct  CC_PalTrngModeParams_t
{
    uint32_t  numOfBytes;   /*!< The amount of Bytes for the required entropy bits.
                                 It is calculated as ROUND_UP(ROUND_UP(((required entropy bits)/(entropy per bit)), 1024),
                                 (EHR width in Bytes)) / 8.
                                 The 1024 bits is the multiple of the window size. The multiple of the EHR width, which is 192 bits. */
    uint32_t  repetitionCounterCutoff;  /*!<  The repetition counter cutoff, as defined in <em>NIST SP 90B:
                                              DRAFT Recommendation for the Entropy Sources Used for Random Bit
                                              Generation</em>, section 4.4.1.
                                              This is calculated as C = ROUND_UP(1+(-log(W)/H)), W = 2^(-40),
                                              H=(entropy per bit). */
    uint32_t  adaptiveProportionCutOff; /*!<  The adaptive proportion cutoff, as defined in <em>NIST SP 90B:
                                              DRAFT Recommendation for the Entropy Sources Used for Random Bit
                                              Generation</em>, section 4.4.2.
                                              This is calculated as C =CRITBINOM(W, power(2,(-H)),1-a), W = 1024,
                                              a = 2^(-40), H=(entropy per bit). */

} CC_PalTrngModeParams_t;
#endif

/*! Definition for the structure of the random-generator parameters
    of CryptoCell, containing the user-given parameters. */
typedef struct  CC_PalTrngParams_t
{
    uint32_t  SubSamplingRatio1;  /*!< The sampling ratio of ROSC #1.*/
    uint32_t  SubSamplingRatio2;  /*!< The sampling ratio of ROSC #2.*/
    uint32_t  SubSamplingRatio3;  /*!< The sampling ratio of ROSC #3.*/
    uint32_t  SubSamplingRatio4;  /*!< The sampling ratio of ROSC #4.*/
#if (CC_CONFIG_TRNG_MODE==1)
    CC_PalTrngModeParams_t   trngModeParams;  /*!< Specific parameters of the TRNG mode.*/
#endif
} CC_PalTrngParams_t;

/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*!
  @brief This function returns the TRNG user parameters.


  @return \c 0 on success.
  @return A non-zero value on failure.
 */
CCError_t CC_PalTrngParamGet(CC_PalTrngParams_t *pTrngParams, /*!< [out] A pointer to the TRNG user parameters. */
                             size_t *pParamsSize     /*!< [in/out] A pointer to the size of the TRNG-user-parameters structure used.
                                                           <ul><li>Input: the function must verify its size is the same as #CC_PalTrngParams_t.</li>
                                                          <li>Output: the function returns the size of #CC_PalTrngParams_t for library-size verification.</li></ul>
                                                           */
                             );


#ifdef __cplusplus
}
#endif

#endif

/**
@}
 */

