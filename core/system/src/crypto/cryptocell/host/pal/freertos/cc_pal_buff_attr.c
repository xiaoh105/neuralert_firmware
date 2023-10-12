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



/************* Include Files ****************/
#include "cc_pal_buff_attr.h"

/************************ Defines ******************************/

/************************ Enums ******************************/

/************************ Typedefs ******************************/

/************************ Global Data ******************************/

/************************ Private Functions ******************************/

/************************ Public Functions ******************************/

CCError_t CC_PalDataBufferAttrGet(const unsigned char *pDataBuffer,       /*!< [in] Address of the buffer to map. */
                                  size_t              buffSize,           /*!< [in] Buffer size in bytes. */
                                  uint8_t             buffType,         /* ! [in] Input for read / output for write */
                                  uint8_t             *pBuffNs           /*!< [out] HNONSEC buffer attribute (0 for secure, 1 for non-secure) */
                                  )
{
    CC_UNUSED_PARAM(pDataBuffer);
    CC_UNUSED_PARAM(buffSize);
    CC_UNUSED_PARAM(buffType);

    *pBuffNs = DATA_BUFFER_IS_SECURE;

    return CC_OK;
}
