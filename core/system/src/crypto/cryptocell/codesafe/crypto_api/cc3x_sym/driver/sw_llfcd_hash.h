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

#ifndef LLFCD_HASH_H
#define LLFCD_HASH_H


#include "cc_pal_types.h"
#include "cc_common_math.h"


#ifdef __cplusplus
extern "C"
{
#endif

/************************ Defines ******************************/

/* Define  SHA512unrolled = 1 for more fast performance or define it as 0 for
   small code size with enough whell performance  */
#define SHA512unrolled  0

/* The SHA1 f()-functions used on the SHA1 function */
#define SHA1_f1(x,y,z)   ( ( x & y ) | ( ~x & z ) )      /* Rounds  0-19 */
#define SHA1_f2(x,y,z)   ( x ^ y ^ z )                   /* Rounds 20-39 */
#define SHA1_f3(x,y,z)   ( (x & ( y | z )) | ( y & z ) ) /* Rounds 40-59 */
#define SHA1_f4(x,y,z)   ( x ^ y ^ z )                   /* Rounds 60-79 */

/* ROTATE_RIGHT_32BIT_VAR rotates x left n bits on 32 bit variables */
#define ROTATE_RIGHT_32BIT_VAR(y, x, n) ( (y) = ((x) >> (n)) | ((x) << ( 32 - (n) )) )
#define ROTR64(x, n)  ( ((x) >> (n)) | ((x) << ( 64 - (n) )) )

/* The following macros rotate right n times 64 bit variable x and store it in 64 bit variable d.      */
/* Note: xs, xd - 32bit pointers to low half of source and destination 64 bit variables x and d;       */
/* ROTRS64(xd, xs, n)  works with  n = [0...32]; ROTRG64(xd, xs, n)  works with  n = [32...64]         */
/* These macros are more efficient than previouse in aspect of performance.                            */
#ifdef BIG__ENDIAN
#define ROTRS64(xd, xs, n)  *(xd)= ( (*(xs) >> (n)) | (*((xs) - 1) << ( 32 - (n) )) ); \
                            *((xd) - 1) = ( (*((xs) - 1) >> (n)) | (*(xs) << ( 32 - (n) )) )


#define ROTRG64(xd, xs, n)  *(xd)     = ( (*((xs) - 1) >> ((n) - 32)) | (*(xs) << ( 64 - (n) )) ); \
                            *((xd) - 1) = ( (*(xs) >> ((n) - 32)) | (*((xs) - 1) << ( 64 - (n) )) )
#else

#define ROTRS64(xd, xs, n)  *(xd)     = ( (*(xs) >> (n)) | (*((xs) + 1) << ( 32 - (n) )) ); \
                            *((xd) + 1) = ( (*((xs) + 1) >> (n)) | (*(xs) << ( 32 - (n) )) )


#define ROTRG64(xd, xs, n)  *(xd)     = ( (*((xs) + 1) >> ((n) - 32)) | (*(xs) << ( 64 - (n) )) ); \
                            *((xd) + 1) = ( (*(xs) >> ((n) - 32)) | (*((xs) + 1) << ( 64 - (n) )) )

#endif

/* REVERSE32(w,x) macros reverses order of bytes in 32 bit word x and store it in  */
#define REVERSE32(w,x)      { \
                                (w) = ((CC_COMMON_ROT32(x) & 0xff00ff00UL) >> 8) | \
                                ((CC_COMMON_ROT32(x) & 0x00ff00ffUL) << 8); \
                            }


/* REVERSE64_F(xd,xs) macros reverses 64 bit variable x and stores it in destination 64 bit variable w */
/* Note: xs, xd - 32bit pointers to low half of source and destination 64 bit variables;               */

#define REVERSE64_F(xd,xs)      REVERSE32((*xd), *((xs) + 1)) \
                                REVERSE32(*((xd) + 1), *(xs))

/* The  SHA-X common functions used on the LLF_HASH_SHA-X_blockOperation */
#define Ch(x,y,z)   ( ( (x) & (y) ) | ( ~(x) & (z) ) )
#define Parity(x,y,z)   ( (x) ^ (y) ^ (z) )
#define Maj(x,y,z)   ( ((x) & ( (y) | (z) )) | ( (y) & (z) ) )


/* F, G, H and I are basic MD5 functions.*/
#define MD5_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~z)))


/* The SHA256  functions used on the LLF_HASH_SHA256_blockOperation */
#define SIGMA256_0( x )    ( CC_COMMON_ROTR32((x),  2) ^ CC_COMMON_ROTR32((x), 13) ^ CC_COMMON_ROTR32((x), 22) )
#define SIGMA256_1( x )    ( CC_COMMON_ROTR32((x),  6) ^ CC_COMMON_ROTR32((x), 11) ^ CC_COMMON_ROTR32((x), 25) )
#define sigma256_0( x )    ( CC_COMMON_ROTR32((x),  7) ^ CC_COMMON_ROTR32((x), 18) ^ ((x) >>  3) )
#define sigma256_1( x )    ( CC_COMMON_ROTR32((x), 17) ^ CC_COMMON_ROTR32((x), 19) ^ ((x) >> 10) )

#define Schedule2(w,k) ( w[(k)] = ( sigma256_1(w[(k) - 2]) + w[(k) - 7] + \
                                    sigma256_0(w[(k) - 15]) + w[(k) - 16]) )

#define ExchangeVars()  (h) = (g); \
                        (g) = (f); \
                        (f) = (e); \
                        (e) = (d) + (T1); \
                        (d) = (c); \
                        (c) = (b); \
                        (b) = (a); \
                        (a) = (T1) + (T2)

#ifdef BIG__ENDIAN
    #define CalcRound0(a,b,c,d,e,f,g,h) \
        T1 = (h) + SIGMA256_1(e) + Ch((e),(f),(g)) + K256[(k)] + (w[(k)] = DataIn_ptr[(k)]); \
        (d) += T1; \
        (h) = T1 + SIGMA256_0((a)) + Maj((a),(b),(c)); \
        k++
#else
    #define CalcRound0(a,b,c,d,e,f,g,h) \
        T1 = (h) + SIGMA256_1(e) + Ch((e),(f),(g)) + K256[(k)] + (w[(k)] = CC_COMMON_REVERSE32(DataIn_ptr[(k)])); \
        (d) += T1; \
        (h) = T1 + SIGMA256_0((a)) + Maj((a),(b),(c)); \
        k++
#endif

#define CalcRound1(a,b,c,d,e,f,g,h) \
    T1 = (h) + SIGMA256_1(e) + Ch((e),(f),(g)) + K256[(k)] + Schedule2((w),(k)); \
    (d) += T1; \
    (h) = T1 + SIGMA256_0((a)) + Maj((a),(b),(c)); \
    k++



/* These macros perform logical operations on 64 bit variables x,y,z and stores result in destination    */
/* variable . Pointers xd,xs,ys,zs points to destination and sourse variables respectively.              */
#ifdef BIG__ENDIAN

    #define Ch64(xd,xs,ys,zs)   *(xd) = Ch(*(xs),*(ys),*(zs)); \
                                *((xd) - 1) = Ch(*((xs) - 1),*((ys) - 1),*((zs) - 1))

    #define Maj64(xd,xs,ys,zs)  *(xd) = Maj(*(xs),*(ys),*(zs)); \
                                *((xd) - 1) = Maj(*((xs) - 1),*((ys) - 1),*((zs) - 1))

#else
    #define Ch64(xd,xs,ys,zs)   *(xd) = Ch(*(xs),*(ys),*(zs)); \
                                *((xd) + 1) = Ch(*((xs) + 1),*((ys) + 1),*((zs) + 1))

    #define Maj64(xd,xs,ys,zs)  *(xd) = Maj(*(xs),*(ys),*(zs)); \
                                *((xd) + 1) = Maj(*((xs) + 1),*((ys) + 1),*((zs) + 1))
#endif

/**********        The SHA512  sigma functions   *************/

/*  The  sigma functions realised with 64 bit C logical operations: their  eficiency is not best       */
#define SIGMA512_0(x)    ( ROTR64((x), 28) ^ ROTR64((x), 34) ^ ROTR64((x), 39) )
#define SIGMA512_1(x)    ( ROTR64((x), 14) ^ ROTR64((x), 18) ^ ROTR64((x), 41) )
#define sigma512_0(x)    ( ROTR64((x),  1) ^ ROTR64((x),  8) ^ ((x) >>  7) )
#define sigma512_1(x)    ( ROTR64((x), 19) ^ ROTR64((x), 61) ^ ((x) >>  6) )


/******** Macros for BIG ENDIAN mashines ******************/

#ifdef BIG__ENDIAN

/* The  sigma-macros, realised with 32 bit logical operations.   */

#define SIGMA512_64_0(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; xd2++; \
                                    ROTRS64((xd),(xs), 28);  ROTRG64((xd2),(xs), 34); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                    ROTRG64((xd2),(xs), 39);  *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                }

#define SIGMA512_64_1(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; xd2++; \
                                    ROTRS64((xd),(xs), 14);  ROTRS64((xd2),(xs), 18); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                    ROTRG64((xd2),(xs), 41);  *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                }

#define sigma512_64_0(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; xd2++; \
                                    ROTRS64((xd),(xs), 1);  ROTRS64((xd2),(xs), 8); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                    *(xd2) = (*(xs) >> 7) | (*((xs) - 1) << 25);  \
                                    *((xd2) - 1) =  *((xs) - 1) >> 7; \
                                    *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                }


#define sigma512_64_1(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; xd2++; \
                                    ROTRS64((xd),(xs), 19);  ROTRG64((xd2),(xs), 61); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                    *(xd2) = (*(xs) >> 6) | (*((xs) - 1) << 26);  \
                                    *((xd2) - 1) =  *((xs) - 1) >> 6; \
                                    *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) - 1) = *((xd) - 1) ^ *((xd2) - 1); \
                                }

/* This macro performs scheduling of of working array by computing its next member w[k]  from its prviouse */
/* members. Note:  wp - 32 bit pointer to low part of 64 bit first member of w                             */
#define Schedule64(w,wp,k)      { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; tp++; \
                                    sigma512_64_1( (tp), ((wp) + 2*(k) - 4)) \
                                    w[(k)] = tmp + w[(k) - 7]; \
                                    sigma512_64_0( (tp), ((wp) + 2*(k) - 30)) \
                                    w[(k)] = w[k] + tmp + w[(k) - 16]; \
                                }

/* This macro perform all logic and arithmetic operations of rounds 0...15 for unrolled variant of           */
/*  SHA256, SHA384, SHA512    functions.                                                                     */
#define CalcRound64_0(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp) \
                                { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; tp++; \
                                    wp[2*k] = data_p[2*k+1]; \
                                    wp[2*k-1] = data_p[2*k]; \
                                    SIGMA512_64_1(tp,(ep)) \
                                    T1 = (h) + tmp; \
                                    Ch64(tp,(ep),(fp),(gp)); \
                                    T1 = T1 + tmp + K512[k] + w[k]; \
                                    (d) += T1; \
                                    SIGMA512_64_0(tp,(ap)) \
                                    Maj64((hp),(ap),(bp),(cp)); \
                                    (h) = (h) + T1 + tmp; \
                                    k++; \
                                }

/* This macro perform all logic and arithmetic operations of rounds 16...79 for unrolled variant of          */
/*  SHA256, SHA384, SHA512    functions.                                                                     */
#define CalcRound64_1(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp) \
                                { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; tp++; \
                                    wp[2*k] = data_p[2*k+1]; \
                                    wp[2*k-1] = data_p[2*k]; \
                                    SIGMA512_64_1(tp,(ep)) \
                                    T1 = (h) + tmp; \
                                    Ch64(tp,(ep),(fp),(gp)); \
                                    Schedule64(w,wp,k) \
                                    T1 = T1 + tmp + K512[k] + w[k]; \
                                    (d) += T1; \
                                    SIGMA512_64_0(tp,(ap)) \
                                    Maj64((hp),(ap),(bp),(cp)); \
                                    (h) = (h) + T1 + tmp; \
                                    k++; \
                                }
/********    Macros for LITTLE ENDIAN machines   ***********/
#else

/* The  sigma-macros, realised with 32 bit logical operations.   */

#define SIGMA512_64_0(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; \
                                    ROTRS64((xd),(xs), 28);  ROTRG64((xd2),(xs), 34); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                    ROTRG64((xd2),(xs), 39);  *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                }

#define SIGMA512_64_1(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; \
                                    ROTRS64((xd),(xs), 14);  ROTRS64((xd2),(xs), 18); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                    ROTRG64((xd2),(xs), 41);  *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                }

#define sigma512_64_0(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; \
                                    ROTRS64((xd),(xs), 1);  ROTRS64((xd2),(xs), 8); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                    *(xd2) = (*(xs) >> 7) | (*((xs) + 1) << 25);  \
                                    *((xd2) + 1) =  *((xs) + 1) >> 7; \
                                    *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                }


#define sigma512_64_1(xd,xs)    { \
                                    uint64_t tmp; \
                                    uint32_t   *xd2 = (uint32_t*)&tmp; \
                                    ROTRS64((xd),(xs), 19);  ROTRG64((xd2),(xs), 61); \
                                    *(xd) = *(xd) ^ *(xd2); *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                    *(xd2) = (*(xs) >> 6) | (*((xs) + 1) << 26);  \
                                    *((xd2) + 1) =  *((xs) + 1) >> 6; \
                                    *(xd) = *(xd) ^ *(xd2); \
                                    *((xd) + 1) = *((xd) + 1) ^ *((xd2) + 1); \
                                }


#define Schedule64(w,wp,k)      { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; \
                                    sigma512_64_1( (tp), ((wp) + 2*(k) - 4)) \
                                    w[(k)] = tmp + w[(k) - 7]; \
                                    sigma512_64_0( (tp), ((wp) + 2*(k) - 30)) \
                                    w[(k)] = w[k] + tmp + w[(k) - 16]; \
                                }



/* This macro perform all logic and arithmetic operations of rounds 0...15 for unrolled variant of           */
/*  SHA256, SHA384, SHA512    functions.                                                                     */
#define CalcRound64_0(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp) \
                                { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; \
                                    REVERSE64_F((wp + 2*k),(data_p + 2*k)); \
                                    SIGMA512_64_1(tp,(ep)) \
                                    T1 = (h) + tmp; \
                                    Ch64(tp,(ep),(fp),(gp)); \
                                    T1 = T1 + tmp + K512[k] + w[k]; \
                                    (d) += T1; \
                                    SIGMA512_64_0(tp,(ap)) \
                                    Maj64((hp),(ap),(bp),(cp)); \
                                    (h) = (h) + T1 + tmp; \
                                    k++; \
                                }

/* This macro perform all logic and arithmetic operations of rounds 16...79 for unrolled variant of          */
/*  SHA256, SHA384, SHA512    functions.                                                                     */
#define CalcRound64_1(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp) \
                                { \
                                    uint64_t  tmp; \
                                    uint32_t  *tp = (uint32_t*)&tmp; \
                                    REVERSE64_F((wp + 2*k),(data_p + 2*k)); \
                                    SIGMA512_64_1(tp,(ep)) \
                                    T1 = (h) + tmp; \
                                    Ch64(tp,(ep),(fp),(gp)); \
                                    Schedule64(w,wp,k) \
                                    T1 = T1 + tmp + K512[k] + w[k]; \
                                    (d) += T1; \
                                    SIGMA512_64_0(tp,(ap)) \
                                    Maj64((hp),(ap),(bp),(cp)); \
                                    (h) = (h) + T1 + tmp; \
                                    k++; \
                                }

#endif /* BIG__ENDIAN */


/************************ Enums ********************************/


/************************ Typedefs  ****************************/


/************************ Structs  ******************************/


/************************ Public Variables **********************/


/************************ Public Functions **********************/


/************************ Private Functions ******************************/


/*
 * ==================================================================
 * Function name: LLF_HASH_SHA512_SingleBlockUpdate
 *
 * Description: This module performs SHA512 hashing of 512-bit block.
 *
 * Author:
 *
 * Last Revision: 1.00.00
 *
 * Update History:
 * Rev 1.00.00,
 * ========================================================================
 */
void LLF_HASH_SHA512_SingleBlockUpdate(HashContext_t  *hashCtx_ptr, const uint32_t *DataIn_ptr);

#ifdef __cplusplus
}
#endif

#endif



