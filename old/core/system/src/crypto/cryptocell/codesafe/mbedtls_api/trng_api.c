/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2018] ARM Limited or its affiliates.                 *
*       ALL RIGHTS RESERVED                                                  *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                                        *
*****************************************************************************/

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "llf_rnd_trng.h"
#include "cc_rng_plat.h"
#include "cc_rnd_common.h"
#include "cc_pal_log.h"
#include "mbedtls_common.h"
#include "cc_pal_mem.h"

#if defined(MBEDTLS_PLATFORM_C)
  #include "mbedtls/platform.h"
#else
  #include <stdio.h>
  #define mbedtls_fprintf fprintf
  #define mbedtls_calloc calloc
  #define mbedtls_free   free
#endif

#include "da16200_regs.h"

#ifdef DEBUG
#define GOTO_END(ERR) \
    do { \
        Error = ERR; \
        mbedtls_printf("%s:%d - %s: %d (0x%08x)\n", __func__, __LINE__, __func__, (int)Error, (int)Error); \
        goto End; \
    } while (0)

#define GOTO_CLEANUP(ERR) \
    do { \
        Error = ERR; \
        mbedtls_printf("%s:%d - %s: %d (0x%08x)\n", __func__, __LINE__, __func__, (int)Error, (int)Error); \
        goto Cleanup; \
    } while (0)
#else // DEBUG
#define GOTO_END(ERR) \
    do { \
        Error = ERR; \
        goto End; \
    } while (0)

#define GOTO_CLEANUP(ERR) \
    do { \
        Error = ERR; \
        goto Cleanup; \
    } while (0)
#endif // DEBUG

#define	TRNGAPI_FCI_CUSTOMIZE
#ifdef	TRNGAPI_FCI_CUSTOMIZE
#include "target.h"

static	uint32_t mbedtls_hardware_poll_fci_rflag;
void	mbedtls_hardware_poll_fci_customize(uint32_t rflag)
{
	mbedtls_hardware_poll_fci_rflag = rflag;
}
#else	//TRNGAPI_FCI_CUSTOMIZE
void	mbedtls_hardware_poll_fci_customize(uint32_t rflag)
{
}
#endif	//TRNGAPI_FCI_CUSTOMIZE

int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    CCRndWorkBuff_t  *rndWorkBuff_ptr;
    CCRndState_t rndState;
    CCRndParams_t trngParams;
    int ret, Error = 0;
    uint32_t  *entrSource_ptr;

    CC_UNUSED_PARAM(data);

    if ( NULL == output )
    {
        CC_PAL_LOG_ERR( "output cannot be NULL\n" );
        GOTO_END( -1 );
    }
    if ( NULL == olen )
    {
        CC_PAL_LOG_ERR( "olen cannot be NULL\n" );
        GOTO_END( -1 );

    }
    if ( 0 == len )
    {
        CC_PAL_LOG_ERR( "len cannot be zero\n" );
        GOTO_END( -1 );
    }

    rndWorkBuff_ptr = ( CCRndWorkBuff_t * )mbedtls_calloc( 1, sizeof ( CCRndWorkBuff_t ) );
    if ( NULL == rndWorkBuff_ptr )
    {
        CC_PAL_LOG_ERR( "Error: cannot allocate memory for rndWorkbuff\n" );
        GOTO_END ( -1 );
    }
    CC_PalMemSetZero( &rndState, sizeof( CCRndState_t ) );
    CC_PalMemSetZero( &trngParams, sizeof( CCRndParams_t ) );

    ret = RNG_PLAT_SetUserRngParameters(  &trngParams );
    if ( ret != 0 )
    {
        CC_PAL_LOG_ERR( "Error: RNG_PLAT_SetUserRngParameters() failed.\n" );
        GOTO_CLEANUP( -1 );
    }

#ifdef	TRNGAPI_FCI_CUSTOMIZE
	// Renesas Customized Parameter
	{
		// Renesas Hidden Scaler !!
		uint32_t hiddenscaler ;

		hiddenscaler = mbedtls_hardware_poll_fci_rflag;

		ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC31203));
		ASIC_DBG_TRIGGER(hiddenscaler);

		if( ((hiddenscaler>>24)&0x0ff) != 0 ){
			trngParams.userParams.SubSamplingRatio1 = trngParams.userParams.SubSamplingRatio1 / ((hiddenscaler>>24)&0x0ff);
		}
		if( ((hiddenscaler>>16)&0x0ff) != 0 ){
			trngParams.userParams.SubSamplingRatio2 = trngParams.userParams.SubSamplingRatio2 / ((hiddenscaler>>16)&0x0ff);
		}
		if( ((hiddenscaler>> 8)&0x0ff) != 0 ){
			trngParams.userParams.SubSamplingRatio3 = trngParams.userParams.SubSamplingRatio3 / ((hiddenscaler>> 8)&0x0ff);
		}
		if( ((hiddenscaler>> 0)&0x0ff) != 0 ){
			trngParams.userParams.SubSamplingRatio4 = trngParams.userParams.SubSamplingRatio4 / ((hiddenscaler>> 0)&0x0ff);
		}
	}
#endif	//TRNGAPI_FCI_CUSTOMIZE

    ret = LLF_RND_GetTrngSource(
                &rndState ,    /*in/out*/
                &trngParams,       /*in/out*/
                0,                 /*in  -  isContinued - false*/
                (uint32_t*)&len,  /*in/out*/
                &entrSource_ptr,   /*out*/
                (uint32_t*)olen,               /*out*/
                (uint32_t*)rndWorkBuff_ptr,   /*in*/
                0                  /*in - isFipsSupport false*/ );
    if ( ret != 0 )
    {
        CC_PAL_LOG_ERR( "Error: LLF_RND_GetTrngSource() failed.\n" );
        GOTO_CLEANUP( -1 );
    }

    if (*olen <= len ){
        CC_PalMemCopy ( output, entrSource_ptr + CC_RND_TRNG_SRC_INNER_OFFSET_WORDS , *olen );
    } else{
#ifdef	TRNGAPI_FCI_CUSTOMIZE
	*olen = len;
        CC_PalMemCopy ( output, entrSource_ptr + CC_RND_TRNG_SRC_INNER_OFFSET_WORDS , *olen );
#else	//TRNGAPI_FCI_CUSTOMIZE
        CC_PAL_LOG_ERR( "buffer length is smaller than LLF_RND_GetTrngSource output length\n" );
        GOTO_CLEANUP( -1 );
#endif	//TRNGAPI_FCI_CUSTOMIZE
    }

Cleanup:
    mbedtls_zeroize_internal( rndWorkBuff_ptr, sizeof( CCRndWorkBuff_t ) );
    mbedtls_free( rndWorkBuff_ptr );
    mbedtls_zeroize_internal( &rndState, sizeof( CCRndState_t ) );
End:
    return Error;
}
