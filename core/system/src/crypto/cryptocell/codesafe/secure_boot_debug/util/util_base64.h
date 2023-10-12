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



#ifndef UTIL_BASE64_H
#define UTIL_BASE64_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief This function converts pem base64 encoded string to char string
 *
 *
 * @param[in] pInStr - PEM base64 string
 * @param[in] inSize - size of given string
 * @param[out] pOutStr - output string decoded
 * @param[in/out] outSize - the output buffer size (in MAX size out actual size)
 *
 * @return CCError_t - On success the value CC_OK is returned,
 *         on failure - a value from bootimagesverifierx509_error.h
 */
CCError_t UTIL_ConvertPemStrToCharStr(uint8_t *pInStr, uint32_t inSize,
                      uint8_t *pOutStr, uint32_t *outSize);


#ifdef __cplusplus
}
#endif

#endif



