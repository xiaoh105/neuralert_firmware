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

#ifndef  _HMAC_DRIVER_H
#define  _HMAC_DRIVER_H

#include "driver_defs.h"

/******************************************************************************
*               TYPE DEFINITIONS
******************************************************************************/

/* The context data-base used by the Hmac functions on the low level */
typedef struct HmacContext {
    uint32_t valid_tag;
    /* Key XOR opad result */
    uint8_t KeyXorOpadBuff[CC_HMAC_SHA2_1024BIT_KEY_SIZE_IN_BYTES];
    /* The operation mode */
    CCHashOperationMode_t mode;
    /* The user HASH context - required for operating the HASH described below */
    CCHashUserContext_t HashUserContext;
} HmacContext_t;


#endif /* _HMAC_DRIVER_H */

