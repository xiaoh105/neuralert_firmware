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


#ifndef _BSV_INT_API_H
#define _BSV_INT_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "cc_pal_types.h"
#include "bsv_defs.h"
#include "cc_crypto_boot_defs.h"


/*----------------------------
      PUBLIC FUNCTIONS
-----------------------------------*/

/*! 
@brief This function retrieves the public key hash from OTP memory, according to the provided index.

@return CC_OK	On success.
@return A non-zero value from bsv_error.h on failure.
*/    
CCError_t CC_BsvPubKeyHashGet(
	unsigned long hwBaseAddress,		/*!< [in] HW registers base address. */ 
	CCSbPubKeyIndexType_t keyIndex,		/*!< [in] Enumeration defining the key hash to retrieve: 128-bit HBK0, 128-bit HBK1, or 256-bit HBK. */
	uint32_t *hashedPubKey,			/*!< [out] A buffer to contain the public key HASH. */
	uint32_t hashResultSizeWords		/*!< [in] The size of the hash in 32-bit words:
							- Must be 4 for 128-bit hash.
							- Must be 8 for 256bit hash. */
	);

/*! 
@brief This function reads software revocation counter from OTP memory, according to the provided key index.

@return CC_OK	On success.
@return A non-zero value from bsv_error.h on failure.
*/    
CCError_t CC_BsvSwVersionGet(
	unsigned long hwBaseAddress,		/*!< [in] HW registers base address. */ 
	CCSbPubKeyIndexType_t keyIndex,		/*!< [in] Enumeration defining the key hash to retrieve: 128-bit HBK0, 128-bit HBK1, or 256-bit HBK. */
	uint32_t *swVersion			/*!< [out] The value of the requested counter as read from OTP memory. */
	);

/*! 
@brief This function received the minimal allowed SW version and updates OTP memory to match.

@return CC_OK	On success.
@return A non-zero value from bsv_error.h on failure.
*/    
CCError_t CC_BsvSwVersionSet(
	unsigned long hwBaseAddress,		/*!< [in] HW registers base address. */ 
	CCSbPubKeyIndexType_t keyIndex,		/*!< [in] Enumeration defining the key hash to retrieve: 128-bit HBK0, 128-bit HBK1, or 256-bit HBK. */
	uint32_t swVersion			/*!< [in] New value of the counter to be programmed in OTP memory. */
	);


#ifdef __cplusplus
}
#endif

#endif



