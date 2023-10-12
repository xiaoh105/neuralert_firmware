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


#ifndef HASH_COMMON_H
#define HASH_COMMON_H

/*
 * All the includes that are needed for code using this module to
 * compile correctly should be #included here.
 */
#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************
 * @brief This function adds a value to a large counter presented in a buffer.
 *        The MSB of the counter is stored in the first cell in the array.
 *
 *        for example:
 *
 *        a counter of 64 bit : the value is :
 *
 *        byte[0] << 56 | byte[1] << 48 ............ byte[6] << 8 | byte[7]
 *
 * @param[in] CounterBuff_ptr - The buffer containing the counter.
 * @param[in] Val             - this value to add.
 * @param[in] CounterSize      - the counter size in 32bit words.
 *
 * @return CCError_t - On success CC_OK is returned, on failure a
 *                        value MODULE_* as defined in ...
 */

void HASH_COMMON_IncMsbUnsignedCounter( uint32_t *CounterBuff_ptr ,
                    uint32_t  Val ,
                    uint32_t   CounterSize );

/********************************************************************************
 * @brief This function adds a value to a large counter presented in a buffer.
 *        The LSB of the counter is stored in the first cell in the array.
 *
 *        for example:
 *
 *        a counter of 64 bit : the value is :
 *
 *        byte[7] << 56 | byte[6] << 48 ............ byte[1] << 8 | byte[0]
 *
 * @param[in] CounterBuff_ptr - The buffer containing the counter.
 * @param[in] Val             - this value to add.
 * @param[in] CounterSize      - the counter size in 32bit words.
 *
 * @return carry bit from MS word if carry occur
 *
 */

uint32_t HASH_COMMON_IncLsbUnsignedCounter(
                      uint32_t     *CounterBuff_ptr ,
                      uint32_t      Val,
                      uint32_t       CounterSize );


#ifdef __cplusplus
}
#endif

#endif



