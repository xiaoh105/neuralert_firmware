/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorized under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2017] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef CC_AES_KEYWRAP_DEFS_H
#define CC_AES_KEYWRAP_DEFS_H



#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/
/*! AES Key Wrap minimum number of input semiblocks for KW mode */
#define CC_AES_KEYWRAP_KW_MIN_NUM_OF_SEMIBLOCKS     2
/*! AES Key Wrap minimum number of input semiblocks for KWP mode */
#define CC_AES_KEYWRAP_KWP_MIN_NUM_OF_SEMIBLOCKS    1
/*! AES Key Wrap minimum number of input semiblocks for KW/P modes. */
#define CC_AES_KEYWRAP_MIN_NUM_OF_SEMIBLOCKS_ARR    {CC_AES_KEYWRAP_KW_MIN_NUM_OF_SEMIBLOCKS, CC_AES_KEYWRAP_KWP_MIN_NUM_OF_SEMIBLOCKS}

/*! AES Key Wrap data in maximal size in bytes. */
#define CC_AES_KEYWRAP_DATA_IN_MAX_SIZE_BYTES       0xFFFF // (64KB - 1)

/*! AES Key Wrap outer loop limit. */
#define CC_AES_KEYWRAP_OUTER_LOOP_LIMIT         0x5
/*! AES Key Wrap inner loop limit. */
#define CC_AES_KEYWRAP_INNER_LOOP_LIMIT         0x1


/************************ Typedefs  ****************************/

/*! Handle uint64_t as 2 x uint32_t */
typedef union {
    uint64_t val;
    struct {
        uint32_t valLow:32;
        uint32_t valHigh:32;
    }twoWords;
}DWORD_t;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CC_AES_KEYWRAP_DEFS_H */
