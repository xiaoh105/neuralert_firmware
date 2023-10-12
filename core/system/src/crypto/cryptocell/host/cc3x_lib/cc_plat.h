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

#ifndef  CC_PLAT_H
#define  CC_PLAT_H

#define NULL_SRAM_ADDR ((CCSramAddr_t)0xFFFFFFFF)

#define _WriteWordsToSram(addr, data, size) \
do { \
    uint32_t ii; \
    volatile uint32_t dummy; \
    CC_HAL_WRITE_REGISTER( CC_REG_OFFSET (HOST_RGF,SRAM_ADDR), (addr)); \
    for( ii = 0 ; ii < size/sizeof(uint32_t) ; ii++ ) { \
           CC_HAL_WRITE_REGISTER( CC_REG_OFFSET (HOST_RGF,SRAM_DATA), SWAP_TO_LE(((uint32_t *)data)[ii])); \
           do { \
             dummy = CC_HAL_READ_REGISTER( CC_REG_OFFSET (HOST_RGF, SRAM_DATA_READY)); \
           }while(!(dummy & 0x1)); \
    } \
}while(0)

#define _ReadWordsFromSram( addr , data , size ) \
do { \
    uint32_t ii; \
    volatile uint32_t dummy; \
    CC_HAL_WRITE_REGISTER( CC_REG_OFFSET (HOST_RGF,SRAM_ADDR) ,(addr) ); \
    dummy = CC_HAL_READ_REGISTER( CC_REG_OFFSET (HOST_RGF,SRAM_DATA)); \
    for( ii = 0 ; ii < size/sizeof(uint32_t) ; ii++ ) { \
        do { \
            dummy = CC_HAL_READ_REGISTER( CC_REG_OFFSET (HOST_RGF, SRAM_DATA_READY)); \
        }while(!(dummy & 0x1)); \
        dummy = CC_HAL_READ_REGISTER( CC_REG_OFFSET (HOST_RGF,SRAM_DATA));\
        ((uint32_t*)data)[ii] = SWAP_TO_LE(dummy); \
    } \
    do { \
        dummy = CC_HAL_READ_REGISTER( CC_REG_OFFSET (HOST_RGF, SRAM_DATA_READY)); \
    }while(!(dummy & 0x1)); \
}while(0)

#define CLEAR_TRNG_SRC()

#endif
