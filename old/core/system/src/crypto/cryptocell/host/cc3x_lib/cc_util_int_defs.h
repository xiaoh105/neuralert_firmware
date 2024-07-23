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

#ifndef  _CC_UTIL_INT_DEFS_H
#define  _CC_UTIL_INT_DEFS_H

typedef enum  {
    UTIL_USER_KEY = 0,
    UTIL_ROOT_KEY = 1,
    UTIL_KCP_KEY = 2,
    UTIL_KCE_KEY = 3,
    UTIL_KPICV_KEY = 4,
    UTIL_KCEICV_KEY = 5,
    UTIL_END_OF_KEY_TYPE = 0x7FFFFFFF
}UtilKeyType_t;


#endif /*_CC_UTIL_INT_DEFS_H*/
