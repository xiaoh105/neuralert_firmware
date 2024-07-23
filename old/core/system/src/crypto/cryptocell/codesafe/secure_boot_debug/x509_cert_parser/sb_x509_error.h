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

#ifndef _SB_X509_ERROR_H
#define _SB_X509_ERROR_H

#include "secureboot_error.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define CC_SB_X509_CERT_INV_PARAM                       CC_SB_X509_CERT_BASE_ERROR + 0x00000001
#define CC_SB_X509_CERT_ILLEGAL_TOKEN                   CC_SB_X509_CERT_BASE_ERROR + 0x00000002
#define CC_SB_X509_CERT_PARSE_ILLEGAL_VAL               CC_SB_X509_CERT_BASE_ERROR + 0x00000003
#define CC_SB_X509_CERT_ILLEGAL_SWVER_ID                CC_SB_X509_CERT_BASE_ERROR + 0x00000005
#define CC_SB_X509_CERT_ILLEGAL_VERSION             CC_SB_X509_CERT_BASE_ERROR + 0x00000006
#define CC_SB_X509_CERT_ILLEGAL_LCS                 CC_SB_X509_CERT_BASE_ERROR + 0x00000007
#define CC_SB_X509_CERT_ILLEGAL_PKG_ADD                 CC_SB_X509_CERT_BASE_ERROR + 0x00000008
#define CC_SB_X509_CERT_ILLEGAL_SOC_ID                  CC_SB_X509_CERT_BASE_ERROR + 0x00000009
#define CC_SB_X509_CERT_ILLEGAL_CERT_TYPE           CC_SB_X509_CERT_BASE_ERROR + 0x0000000A
#define CC_SB_X509_CERT_SIG_ALIGN_INCORRECT                 CC_SB_X509_CERT_BASE_ERROR + 0x0000000C


#ifdef __cplusplus
}
#endif

#endif


