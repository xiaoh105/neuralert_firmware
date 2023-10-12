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


/********************** Include Files ***********************************/

#include "cc_common_math.h"
#include "cc_hash.h"
#include "cc_pal_mem.h"
#include "hash_driver.h"
#include "sw_llfcd_hash.h"

/************************ Defines **********************************/

/************************ Enums **************************************/

/************************ Typedefs ***********************************/

/************************ Global Data ********************************/


/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ***********************/

/* Hash constant words K for SHA-384 and SHA-512: */
static const uint64_t K512[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

/************************ Private Functions ******************************/

/*
 * ========================================================================
 * Function name: LLF_HASH512_32_SingleBlockUpdate
 *
 * Description: This module performs SHA512 hashing of 512-bit block.
 *
 * Author: R.Levin
 *
 * Last Revision: 1.00.00
 *
 * Update History:
 * Rev :
 *
 * ========================================================================
 */


void LLF_HASH_SHA512_SingleBlockUpdate( HashContext_t *hashCtx_ptr, const uint32_t *DataIn_ptr )
{
    /* LOCAL DECLARATIONS */
    uint32_t k = 0;

    /* working array and working variables  */
    uint64_t w[80];
    uint64_t a,b,c,d,e,f,g,h;
    uint64_t T1;

    /* pointers to  arrays and working variables  */
    const uint32_t *data_p = (uint32_t const *)DataIn_ptr;

    uint32_t *wp = (uint32_t*)&w[0];

    uint64_t HASH_Result[SHA512_DIGEST_SIZE_IN_WORDS/2] = {0};

    //uint64_t *HASH_Result = ( uint64_t *)&hashCtx_ptr->digest[0];

    uint32_t *ap = (uint32_t*)&a;
    uint32_t *bp = (uint32_t*)&b;
    uint32_t *cp = (uint32_t*)&c;/*, *dp = (uint32_t*)&d;*/
    uint32_t *ep = (uint32_t*)&e;
    uint32_t *fp = (uint32_t*)&f;
    uint32_t *gp = (uint32_t*)&g;/*, *hp = (uint32_t*)&h;*/

#if SHA512unrolled
    uint32_t *dp = (uint32_t*)&d;
    uint32_t *hp = (uint32_t*)&h;

#ifdef BIG__ENDIAN
    /* due to Big Endian order increment the pointer */
    ap++; bp++; cp++; ep++; fp++; gp++; dp++; hp++; wp++;
#endif

#else

    uint64_t  T2, temp=0;
    uint32_t *t_p = (uint32_t*)&temp;
    uint32_t *T2p = (uint32_t*)&T2;

#ifdef BIG__ENDIAN

    /* due to Big Endian order increment the pointer */
    ap++; bp++; cp++; ep++; fp++; gp++; t_p++; T2p++; wp++;
#endif

#endif
    /* FUNCTION LOGIC */
    CC_PalMemCopy((uint8_t *)&HASH_Result[0], (uint8_t *)&hashCtx_ptr->digest[0], SHA512_DIGEST_SIZE_IN_WORDS*sizeof(uint32_t));

    /* executing the SHA Process on the block: */
    a = HASH_Result[0];
    b = HASH_Result[1];
    c = HASH_Result[2];
    d = HASH_Result[3];
    e = HASH_Result[4];
    f = HASH_Result[5];

    g = HASH_Result[6];
    h = HASH_Result[7];

    k=0;

#if SHA512unrolled
    /****** SHA calculating loops unrolled  **********************/

    /* Round 1-16 with transforming the block to Big-Endian  */
    CalcRound64_0(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp);
    CalcRound64_0(h,a,b,c,d,e,f,g,hp,ap,bp,cp,dp,ep,fp,gp);
    CalcRound64_0(g,h,a,b,c,d,e,f,gp,hp,ap,bp,cp,dp,ep,fp);
    CalcRound64_0(f,g,h,a,b,c,d,e,fp,gp,hp,ap,bp,cp,dp,ep);
    CalcRound64_0(e,f,g,h,a,b,c,d,ep,fp,gp,hp,ap,bp,cp,dp);
    CalcRound64_0(d,e,f,g,h,a,b,c,dp,ep,fp,gp,hp,ap,bp,cp);
    CalcRound64_0(c,d,e,f,g,h,a,b,cp,dp,ep,fp,gp,hp,ap,bp);
    CalcRound64_0(b,c,d,e,f,g,h,a,bp,cp,dp,ep,fp,gp,hp,ap);

    CalcRound64_0(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp);
    CalcRound64_0(h,a,b,c,d,e,f,g,hp,ap,bp,cp,dp,ep,fp,gp);
    CalcRound64_0(g,h,a,b,c,d,e,f,gp,hp,ap,bp,cp,dp,ep,fp);
    CalcRound64_0(f,g,h,a,b,c,d,e,fp,gp,hp,ap,bp,cp,dp,ep);
    CalcRound64_0(e,f,g,h,a,b,c,d,ep,fp,gp,hp,ap,bp,cp,dp);
    CalcRound64_0(d,e,f,g,h,a,b,c,dp,ep,fp,gp,hp,ap,bp,cp);
    CalcRound64_0(c,d,e,f,g,h,a,b,cp,dp,ep,fp,gp,hp,ap,bp);
    CalcRound64_0(b,c,d,e,f,g,h,a,bp,cp,dp,ep,fp,gp,hp,ap);

    /* Round 17-64 */
    do{

        CalcRound64_1(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp);
        CalcRound64_1(h,a,b,c,d,e,f,g,hp,ap,bp,cp,dp,ep,fp,gp);
        CalcRound64_1(g,h,a,b,c,d,e,f,gp,hp,ap,bp,cp,dp,ep,fp);
        CalcRound64_1(f,g,h,a,b,c,d,e,fp,gp,hp,ap,bp,cp,dp,ep);
        CalcRound64_1(e,f,g,h,a,b,c,d,ep,fp,gp,hp,ap,bp,cp,dp);
        CalcRound64_1(d,e,f,g,h,a,b,c,dp,ep,fp,gp,hp,ap,bp,cp);
        CalcRound64_1(c,d,e,f,g,h,a,b,cp,dp,ep,fp,gp,hp,ap,bp);
        CalcRound64_1(b,c,d,e,f,g,h,a,bp,cp,dp,ep,fp,gp,hp,ap);

        CalcRound64_1(a,b,c,d,e,f,g,h,ap,bp,cp,dp,ep,fp,gp,hp);
        CalcRound64_1(h,a,b,c,d,e,f,g,hp,ap,bp,cp,dp,ep,fp,gp);
        CalcRound64_1(g,h,a,b,c,d,e,f,gp,hp,ap,bp,cp,dp,ep,fp);
        CalcRound64_1(f,g,h,a,b,c,d,e,fp,gp,hp,ap,bp,cp,dp,ep);
        CalcRound64_1(e,f,g,h,a,b,c,d,ep,fp,gp,hp,ap,bp,cp,dp);
        CalcRound64_1(d,e,f,g,h,a,b,c,dp,ep,fp,gp,hp,ap,bp,cp);
        CalcRound64_1(c,d,e,f,g,h,a,b,cp,dp,ep,fp,gp,hp,ap,bp);
        CalcRound64_1(b,c,d,e,f,g,h,a,bp,cp,dp,ep,fp,gp,hp,ap);

    } while ( k < 80 );

    /* end SHAunrolled */

#else
    /*****************    SHA calculating loops not unrolled *************************/

    /* if we in Little Endian: Rounds 0-15 with transforming the block to Big-Endian  */
    do
    {

#ifdef BIG__ENDIAN
        (*(wp + 2*k))=(*(data_p + 2*k+1));
        (*(wp - 1 + 2*k))=(*(data_p + 2*k));
#else
        REVERSE64_F((wp + 2*k),(data_p + 2*k));
#endif

        SIGMA512_64_1(t_p,(ep));
        T1 = (h) + temp;

        Ch64(t_p,(ep),(fp),(gp));

        T1 = T1 + temp + K512[k] + w[k];

        SIGMA512_64_0(t_p,(ap))
        Maj64((T2p),(ap),(bp),(cp));
        T2 = T2 + temp;

        ExchangeVars();
        k++;

    } while(k<16);

    /* Rounds 16-79 */
    do
    {
        Schedule64(w,wp,k);

        SIGMA512_64_1(t_p,(ep));
        T1 = (h) + temp;

        Ch64(t_p,(ep),(fp),(gp));
        T1 = T1 + temp + K512[k] + w[k];

        SIGMA512_64_0(t_p,(ap));
        Maj64((T2p),(ap),(bp),(cp));

        T2 = T2 + temp;

        ExchangeVars();
        k++;

    } while(k<80);

#endif /* #ifndef SHAunrolled */

    /* updating the digest on the context */
    HASH_Result[0] += a;
    HASH_Result[1] += b;
    HASH_Result[2] += c;
    HASH_Result[3] += d;
    HASH_Result[4] += e;
    HASH_Result[5] += f;
    HASH_Result[6] += g;
    HASH_Result[7] += h;

    a=0;b=0;c=0;d=0;e=0;f=0;g=0;h=0;T1=0;
#if !SHA512unrolled
    T2=0;temp=0;
#endif
    CC_PalMemSet(w, 0, sizeof(w));

    CC_PalMemCopy((uint8_t *)&hashCtx_ptr->digest[0], (uint8_t *)&HASH_Result[0], SHA512_DIGEST_SIZE_IN_WORDS*sizeof(uint32_t));

    return;

}
 /* END OF LLF_HASH_SHA512_32_SingleBlockUpdate */
