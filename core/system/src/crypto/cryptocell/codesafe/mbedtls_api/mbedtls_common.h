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
#ifndef MBEDTLS_COMMON_H
#define MBEDTLS_COMMON_H
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \brief           Implementation of memset to zero
 *
 * \param v         adrress to set
 * \param n         size
 */
void mbedtls_zeroize_internal( void *v, size_t n );

#ifdef __cplusplus
}
#endif

#endif  /* MBEDTLS_COMMON_H */
