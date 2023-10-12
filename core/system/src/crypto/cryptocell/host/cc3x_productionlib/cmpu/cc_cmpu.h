/****************************************************************************
* The confidential and proprietary information contained in this file may   *
* only be used by a person authorised under and to the extent permitted     *
* by a subsisting licensing agreement from Arm Limited (or its affiliates). *
*     (C) COPYRIGHT [2001-2018] Arm Limited (or its affiliates).            *
*         ALL RIGHTS RESERVED                                               *
* This entire notice must be reproduced on all copies of this file          *
* and copies of this file may only be made by a person if such person is    *
* permitted to do so under the terms of a subsisting license agreement      *
* from Arm Limited (or its affiliates).                                     *
*****************************************************************************/

/*!
@file
@brief This file contains all of the ICV production library APIs, their enums and definitions.
@defgroup CMPU CryptoCell ICV production library APIs
@brief Contains CryptoCell ICV production library APIs.

@{
@ingroup prod
*/

#ifndef _CMPU_H
#define _CMPU_H

#include "cc_pal_types_plat.h"
#include "cc_prod.h"

/************************ Defines ******************************/

/*! The size of the ICV production library workspace in Bytes, needed by the library for internal use. */
#define CMPU_WORKSPACE_MINIMUM_SIZE  4096

/*! The size of the ICV production library unique buffer in Bytes: Hbk0 or user data. */
#define PROD_UNIQUE_BUFF_SIZE   16
/************************ Enums ********************************/

/*! The unique data type. */
typedef enum {
    /*! The device uses the unique data as Hbk0. */
        CMPU_UNIQUE_IS_HBK0 = 1,
    /*! The device uses the unique data as a random value.
        Hbk0 is not used for the device. */
        CMPU_UNIQUE_IS_USER_DATA = 2,
    /*! Reserved. */
        CMPU_UNIQUE_RESERVED    = 0x7FFFFFFF,
} CCCmpuUniqueDataType_t;

/************************ Typedefs  ****************************/


/************************ Structs  ******************************/

/*! \brief The device use of the unique buffer.

  If the device uses Hbk0, then the \p hbk0 field is used.
  Otherwise, a random buffer for the \p userData field is used.
  */
typedef union {
    /*! The Hbk0 buffer, if used by the device. */
        uint8_t hbk0[PROD_UNIQUE_BUFF_SIZE];
    /*! Any random value, if Hbk0 is not used by the device. */
        uint8_t userData[PROD_UNIQUE_BUFF_SIZE];
} CCCmpuUniqueBuff_t;


/*! The ICV production library input options. */
typedef struct {
    /*! The unique data type: Hbk0 or a random user-defined data. */
        CCCmpuUniqueDataType_t  uniqueDataType;
    /*! The unique data buffer. */
        CCCmpuUniqueBuff_t        uniqueBuff;
    /*! The asset type of the Kpicv. Allowed values are:<ul><li>Not used.</li>
        <li>Plain-asset.</li><li>Package.</li></ul> */
        CCAssetType_t                  kpicvDataType;
    /*! The buffer of the Kpicv, if its type is plain-asset or package. */
        CCAssetBuff_t               kpicv;
    /*! The asset type of the Kceicv. Allowed values are:<ul><li>Not used.</li>
        <li>Plain-asset.</li><li>Package.</li></ul> */
        CCAssetType_t                  kceicvDataType;
    /*! The buffer of the Kceicv, if its type is plain asset or package. */
        CCAssetBuff_t                kceicv;
    /*! The minimal SW version of the ICV. Valid only if Hbk0 is used. */
        uint32_t                     icvMinVersion;
    /*! The ICV configuration word. */
        uint32_t                     icvConfigWord;
    /*! The default DCU lock bits of the ICV. Valid only if Hbk0 is used. */
        uint32_t                     icvDcuDefaultLock[PROD_DCU_LOCK_WORD_SIZE];
}CCCmpuData_t;



/************************ Functions *****************************/

/*!
 @brief This function burns all ICV assets into the OTP of the device.

 The user must perform a power-on-reset (PoR) to trigger LCS change to DM LCS.

 @return \c CC_OK on success.
 @return A non-zero value from cc_prod_error.h on failure.
*/
CIMPORT_C CCError_t  CCProd_Cmpu(
        unsigned long    ccHwRegBaseAddr,   /*!< [in] The base address of
                                                 CrytoCell HW register. */
        CCCmpuData_t     *pCmpuData,        /*!< [in] A pointer to the ICV
                                                 defines structure. */
        unsigned long    workspaceBaseAddr, /*!< [in] The base address of the
                                                 workspace for internal use. */
        uint32_t         workspaceSize      /*!< [in] The size of the provided
                                                 workspace. Must be at least
                                                 #CMPU_WORKSPACE_MINIMUM_SIZE. */);


#endif  //_CMPU_H
/**
@}
*/

