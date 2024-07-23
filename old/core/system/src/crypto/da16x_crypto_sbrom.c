/**
 ****************************************************************************************
 *
 * @file da16x_crypto_sbrom.c
 *
 * @brief CC312 Integration
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oal.h"
#include "sal.h"
#include "hal.h"
#include "cal.h"
#include "driver.h"
#include "library.h"

#include "da16x_image.h"

/* cerberus pal */
#include "cc_hal.h"
#include "cc_pal_init.h"
#include "cc_pal_types.h"
#include "cc_pal_types_plat.h"
#include "cc_pal_mem.h"
#include "cc_pal_perf.h"
#include "cc_regs.h"
#include "cc_otp_defs.h"
#include "cc_lib.h"
//REMOVE: #include "cc_rnd.h"
#include "cc_pal_x509_defs.h"

/* mbedtls */
#include "mbedtls/config.h"
#include "mbedtls/compat-1.3.h"

#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/entropy.h"

#include "da16x_crypto.h"
#include "da16x_secureboot.h"

/* sbrom */
#include "cc_sec_defs.h"
#include "secureboot_stage_defs.h"
#include "cc_pal_sb_plat.h"
#include "bootimagesverifier_def.h"
#include "bootimagesverifier_api.h"
#include "bsv_api.h"
#include "bsv_otp_api.h"
#include "bsv_error.h"
#include "secdebug_api.h"

#include "cc_cmpu.h"
#include "cc_dmpu.h"

#include "dx_env.h"

#include "mbedtls_cc_util_asset_prov.h"

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define	BSVIT_PRINT_DBG(...)		//if(da16x_crypto_debug == 1){ CRYPTO_PRINTF( __VA_ARGS__ );vTaskDelay(10); }
#define	BSVIT_PRINT_FDBG(...)		//if(da16x_crypto_debug == 1){ CRYPTO_PRINTF( __VA_ARGS__ );vTaskDelay(10); }
#define	BSVIT_PRINT_MEASURE(...)	//if(da16x_crypto_debug == 1){ CRYPTO_PRINTF( __VA_ARGS__ );vTaskDelay(10); }
#define	BSVIT_PRINT_ERROR(...)		if(da16x_crypto_debug == 1){ CRYPTO_PRINTF( __VA_ARGS__ );vTaskDelay(10); }
#define	SBROM_DBG_TRIGGER(...)		//ASIC_DBG_TRIGGER(__VA_ARGS__)
#undef true
#define	true	pdTRUE

typedef unsigned int	size_t;
typedef	unsigned char	uint8_t;
typedef	unsigned long	uint32_t;

/******************************************************************************
 *
 ******************************************************************************/

/**
 * Error Codes
 */
typedef enum BsvItError_t
{
    BSVIT_ERROR__OK = 0,
    BSVIT_ERROR__FAIL = 0x0000FFFF,
}BsvItError_t;


typedef	 struct {
	CCSbCertInfo_t *sbCertInfoCtx;
	uint32_t *pWorkspaceAligned;
	uint32_t  CertHeaderInfoSize;
	uint32_t *pOutCertHeaderInfoAligned;
	uint8_t chainIndex;
	HANDLE  sbootflash;
	CCSbFlashReadFunc flashwrap;
	UINT32  loadaddr;
	UINT32  jmpaddr;
	USR_CALLBACK stopproc;
	UINT64	bcfmeasure;
	UINT64	bcfmeasureflash;
} DA16X_SBOOT_TYPE;

#define DA16X_MAX_CHAIN	5

#define DA16X_SBOOT_BCFM_START(f)	//if(da16x_crypto_debug == 1){f->bcfmeasure = da16x_rtc_get_mirror();}
#define DA16X_SBOOT_BCFM_FINISH(f)	//if(da16x_crypto_debug == 1){f->bcfmeasure = da16x_rtc_get_mirror() - (f->bcfmeasure);}
#define DA16X_SBOOT_BCFM_PRINT(f,str)	//if(da16x_crypto_debug == 1){BSVIT_PRINT_MEASURE("[BCF] %s - %lld, %lld\n", str, (f->bcfmeasure), (f->bcfmeasureflash) );}

/******************************************************************************
 *
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_Init(void);
static BsvItError_t DA16X_SBoot_Boot(UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc);
static BsvItError_t DA16X_SDEBUG_FAST(uint32_t lcs, HANDLE sflash, UINT32 faddress);
static BsvItError_t DA16X_DeviceCompleteDisable(void);

#ifdef	BUILD_OPT_DA16200_FPGA
static BsvItError_t test_prepareOtp(uint32_t mode);
#endif	//BUILD_OPT_DA16200_FPGA

static	UINT32	da16x_crypto_debug;

extern int	CC_PalInit_Abstract(void);
extern void CC_PalTerminate_Abstract(void);

/******************************************************************************
 *  DA16X_Debug_SecureBoot( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void DA16X_Debug_SecureBoot(UINT32 mode)
{
	da16x_crypto_debug = mode;
}

UINT32 DA16X_Debug_SecureBoot_Mode(void)
{
	return da16x_crypto_debug;
}

/******************************************************************************
 *  DA16X_SecureBoot( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 DA16X_SecureBoot(UINT32 taddress, UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;

	CC_PalInit_Abstract(); // for Mutex
	CC_HalInit();

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32000));

	rc = DA16X_SBoot_Init();
	if( rc != BSVIT_ERROR__OK ) goto end_step;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32001));

	rc = DA16X_SBoot_Boot(taddress, jaddress, stopproc);
	if( rc != BSVIT_ERROR__OK ) goto end_step;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32002));

end_step:
	CC_HalTerminate();
	CC_PalTerminate_Abstract();

	return (rc == BSVIT_ERROR__OK) ? TRUE : FALSE;
}


UINT32 DA16X_SecureDebug(HANDLE fhandler, UINT32 faddress)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t lcs;

	CC_PalInit_Abstract(); // for Mutex
	CC_HalInit();

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32100));

	rc = DA16X_SBoot_Init();
	if( rc != BSVIT_ERROR__OK ) goto end_dbgstep;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32101));

	ccRc = CC_BsvLcsGet(DA16X_ACRYPT_BASE, &lcs);

	switch(lcs){
	case CC_BSV_CHIP_MANUFACTURE_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC321A0));
		BSVIT_PRINT_DBG("SDebug-CM-Skip\n");
		break;

	case CC_BSV_DEVICE_MANUFACTURE_LCS:
	case CC_BSV_SECURE_LCS:
	case CC_BSV_RMA_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC321B0));
		BSVIT_PRINT_DBG("SDebug-%d\n", lcs);
		rc = DA16X_SDEBUG_FAST(lcs, fhandler, faddress);
		break;

	default:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC321F0));
		BSVIT_PRINT_DBG("SDebug-???\n");
		//TODO: DeviceCompleteDisable();
		rc = DA16X_DeviceCompleteDisable();
		break;
	}

	if( rc != BSVIT_ERROR__OK ) goto end_dbgstep;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32102));

end_dbgstep:
	CC_HalTerminate();
	CC_PalTerminate_Abstract();

	return (rc == BSVIT_ERROR__OK) ? TRUE : FALSE;
}


void	DA16X_TestOtp(UINT32 mode)
{
#ifdef	BUILD_OPT_DA16200_FPGA
	CC_PalInit_Abstract(); // for Mutex
	CC_HalInit();

	test_prepareOtp(mode);

	CC_HalTerminate();
	CC_PalTerminate_Abstract();
#endif	//BUILD_OPT_DA16200_FPGA
}

UINT32 DA16X_SecureSocID(void)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t lcs;

	CC_PalInit_Abstract(); // for Mutex
	CC_HalInit();

	rc = DA16X_SBoot_Init();
	if( rc != BSVIT_ERROR__OK ) goto sdebug_bail;

	ccRc = CC_BsvLcsGet(DA16X_ACRYPT_BASE, &lcs);

	if( (ccRc == CC_OK) && (lcs == CC_BSV_SECURE_LCS))
	{
		CCHashResult_t *SocID;

		SocID = (CCHashResult_t *)CRYPTO_MALLOC(sizeof(CCHashResult_t));
		if( SocID == NULL ){
			rc = BSVIT_ERROR__FAIL;
			goto sdebug_bail;
		}

		// 1. calculate SOC_ID
		// export the SOC_ID
		// this step can also be performed from the runtime software
		/* Compute the SOC_ID only for secure life cycle
		 * SOC_ID is a function of HBK and HUK which are fully present only in secure mode. */

		/* Call API */
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32003));
#ifdef BUILD_OPT_DA16200_FPGA
		CC_BSV_READ_OTP_WORD(DA16X_ACRYPT_BASE, (0x10<<2), ccRc);		// OTP enable
#else
		CC_BSV_READ_OTP_WORD(DA16X_ACRYPT_BASE, (0x10<<2), &ccRc);		// OTP enable
#endif
		ccRc = CC_BsvSocIDCompute(DA16X_ACRYPT_BASE, (*SocID));
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32004));

		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("Failed CC_BsvSocIDCompute - ccRc = 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
		}
		else
		{
		        CRYPTO_PRINTF("CC_BsvSocIDCompute return SocID");
			//CRYPTO_DBG_DUMP(0, (*SocID), sizeof(CCHashResult_t));
			{
			    size_t i;
			    UINT8 *SocIDBuff;

			    SocIDBuff = (UINT8 *)(*SocID);

			    for( i = 0; i < sizeof(CCHashResult_t); i++ ){
			    	if( (i % 16) == 0 ){ CRYPTO_PRINTF("\n\t"); }
			        CRYPTO_PRINTF("%c%c "
					, "0123456789ABCDEF" [SocIDBuff[i] / 16]
					, "0123456789ABCDEF" [SocIDBuff[i] % 16] );
			    }
			    CRYPTO_PRINTF( "\n" );
			}
			CRYPTO_MEMSET((*SocID), 0, sizeof(CCHashResult_t));
		}

		CRYPTO_FREE(SocID);
	}
	else if (ccRc == CC_OK) {
		char *state;
		switch(lcs){
		case CC_BSV_CHIP_MANUFACTURE_LCS:	state = "CM"; break;
		case CC_BSV_DEVICE_MANUFACTURE_LCS:	state = "DM"; break;
		case CC_BSV_SECURE_LCS:			state = "SECURE"; break;
		case CC_BSV_RMA_LCS:			state = "RMA"; break;
		default:				state = "FATAL"; break;
		}
		CRYPTO_PRINTF("LifeCycle: %s\n", state);
	}

sdebug_bail:
	CC_HalTerminate();
	CC_PalTerminate_Abstract();

	/* OTP standby mode */
	*((volatile UINT32 *)(0x40120000)) = 0x04000000;
	return (rc == BSVIT_ERROR__OK) ? TRUE : FALSE;
}

/******************************************************************************
 *  test_prepareOtp( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifdef	BUILD_OPT_DA16200_FPGA

extern BsvItError_t bsvIt_burnOtp(unsigned int *otpBuf, unsigned int nextLcs);

static BsvItError_t test_prepareOtp(uint32_t mode)
{
#define TEST_OTP_SIZE_IN_WORDS 		0x2C

    BsvItError_t rc = BSVIT_ERROR__OK;
    const uint32_t OTP_SIZE = TEST_OTP_SIZE_IN_WORDS * sizeof(uint32_t);

    uint32_t otpLcs;

    uint32_t * pOtpValuesAligned = NULL;

    BSVIT_PRINT_DBG("prepare Otp\n");

    if( mode != CC_BSV_CHIP_MANUFACTURE_LCS ){
    	return BSVIT_ERROR__FAIL;
    }

    otpLcs = CC_BSV_CHIP_MANUFACTURE_LCS;

    /* Allocate dma-able region */
    pOtpValuesAligned   = (uint32_t *)CRYPTO_MALLOC(OTP_SIZE);

    /* Copy OTP image to dma-able memory space */
    memset(pOtpValuesAligned, 0x0, OTP_SIZE);

    {
    	uint32_t i;
	uint32_t OtpWord;
	for( i = 0; i < 0x2C; i++ ){
		CC_BsvOTPWordRead(DA16X_ACRYPT_BASE, i, &OtpWord );
		CRYPTO_PRINTF("OTP[%02x] := %08x\n", i, OtpWord );
	}
    }

    /*  Burn the OTP for secure LCS and with the Kpicv, kceicv */
    if (bsvIt_burnOtp(pOtpValuesAligned, otpLcs) != 0)
    {
        BSVIT_PRINT_ERROR("Failed to bsvIt_burnOtp\n");
        rc = BSVIT_ERROR__FAIL;
        goto bail;
    }

bail:
    CRYPTO_FREE(pOtpValuesAligned);

    return rc;
}

#endif	//BUILD_OPT_DA16200_FPGA
/******************************************************************************
 *  DA16X_SBoot_Init( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_Init(void)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t lcs;
	uint32_t rcRmaFlag;
	UINT64 bcfmeasure;

	bcfmeasure = da16x_rtc_get_mirror();

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32008));

	ccRc = CC_BsvInit(DA16X_ACRYPT_BASE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32009));

	if(ccRc != CC_OK){
		//TODO: DeviceCompleteDisable();
		rc = DA16X_DeviceCompleteDisable();

		goto init_bail;
	}

	/* Init library */
	ccRc = CC_BsvLcsGetAndInit(DA16X_ACRYPT_BASE, &lcs);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200A));

	if (ccRc != CC_OK)
	{
		uint32_t* pWorkspaceAligned = NULL;

		pWorkspaceAligned = (uint32_t *)CRYPTO_MALLOC(CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

		BSVIT_PRINT_ERROR("Failed CC_BsvLcsGetAndInit - ccRc = 0x%x\n", ccRc);
		rc = BSVIT_ERROR__FAIL;

		CC_BsvFatalErrorSet(DA16X_ACRYPT_BASE);

		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200E));

		/* this is done in order to lock the DCU and not enable debug */
		ccRc = CC_BsvSecureDebugSet(DA16X_ACRYPT_BASE,
		                NULL,
		                0,
		                &rcRmaFlag,
		                pWorkspaceAligned,
		                CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200F));

		CRYPTO_FREE(pWorkspaceAligned);
		//TODO: Abort_bootCode(); /* doesn't return */
		goto init_bail;
	}

	/* 1. Enable core clock gating */
	ccRc = CC_BsvCoreClkGatingEnable(DA16X_ACRYPT_BASE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200D));

	if (ccRc != CC_OK)
	{
		BSVIT_PRINT_ERROR("Failed CC_BsvCoreClkGatingEnable - ccRc = 0x%x\n", ccRc);
		rc = BSVIT_ERROR__FAIL;
		goto init_bail;
	}

	/* 2. Set secure mode to unsecured */
	ccRc = CC_BsvSecModeSet(DA16X_ACRYPT_BASE, CC_FALSE, CC_FALSE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200B));

	if (ccRc != CC_OK)
	{
		BSVIT_PRINT_ERROR("Failed CC_BsvSecModeSet - ccRc = 0x%x\n", ccRc);
		rc = BSVIT_ERROR__FAIL;
		goto init_bail;
	}

	/* 3. Set privileged mode to unprivileged */
	ccRc = CC_BsvPrivModeSet(DA16X_ACRYPT_BASE, CC_FALSE, CC_FALSE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3200C));

	if (ccRc != CC_OK)
	{
		BSVIT_PRINT_ERROR("Failed CC_BsvPrivModeSet - ccRc = 0x%x\n", ccRc);
		rc = BSVIT_ERROR__FAIL;
		goto init_bail;
	}

init_bail:
	bcfmeasure = da16x_rtc_get_mirror() - bcfmeasure;
	BSVIT_PRINT_DBG("DA16X_SBoot_Init - %lld\n", bcfmeasure);

	return rc;
}

/******************************************************************************
 *  DA16X_SBoot_Boot( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_CM(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc);
static BsvItError_t DA16X_SBoot_DM(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc);
static BsvItError_t DA16X_SBoot_Secure(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc);
static BsvItError_t DA16X_SBoot_RMA(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc);

static BsvItError_t DA16X_SBoot_Boot(UINT32 faddress , UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t lcs;

	ccRc = CC_BsvLcsGet(DA16X_ACRYPT_BASE, &lcs);

	switch(lcs){
	case CC_BSV_CHIP_MANUFACTURE_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320C0));
		BSVIT_PRINT_DBG("SBoot-CM: %08x\n", faddress);
		rc = DA16X_SBoot_CM(lcs, faddress, jaddress, stopproc);
		break;

	case CC_BSV_DEVICE_MANUFACTURE_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320D0));
		BSVIT_PRINT_DBG("SBoot-DM: %08x\n", faddress);
		rc = DA16X_SBoot_DM(lcs, faddress, jaddress, stopproc);
		break;

	case CC_BSV_SECURE_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320A0));
		BSVIT_PRINT_DBG("SBoot-SECURE: %08x\n", faddress);
		rc = DA16X_SBoot_Secure(lcs, faddress, jaddress, stopproc);
		break;

	case CC_BSV_RMA_LCS:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320E0));
		BSVIT_PRINT_DBG("SBoot-RMA: %08x\n", faddress);
		rc = DA16X_SBoot_RMA(lcs, faddress, jaddress, stopproc);
		break;

	default:
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320F0));

		BSVIT_PRINT_DBG("SBoot-???\n");
		//TODO: DeviceCompleteDisable();
		rc = DA16X_DeviceCompleteDisable();
		break;
	}

	return rc;
}

/******************************************************************************
 *  FLASH Wrapper
 ******************************************************************************/
#undef	DA16X_RAMLIB_LOAD_SFLASH
#define	DA16X_FLASH_RALIB_OFFSET	(*(UINT32*)0x00F802D0) 	/* CM = RA + PTIM  0xF1000 */
#define RAMLIB_OFFSET_0_4MB			0x0018A000
#define RAMLIB_OFFSET_1_4MB			0x00380000
#define RAMLIB_CACHE_OFFSET_4MB		0x00280000		// RTOS 0x00100000 + physical RAMLIB offset(0x180000)
#define RAMLIB_OFFSET_0			0x000F1000
#define RAMLIB_OFFSET_1			0x001E5000
#define RAMLIB_CACHE_OFFSET		0x001E7000		// RTOS 0x00100000 + physical RAMLIB offset(0xE7000)

static	uint32_t DA16X_sboot_flashRead(CCAddr_t flashAddress, uint8_t *pMemDst, uint32_t sizeToRead, void* context)
{
	DA16X_SBOOT_TYPE *sboot;
	UINT64 bcfmeasure;
#ifdef DA16X_RAMLIB_LOAD_SFLASH
#else
	CCAddr_t tmpAddress;
#endif

	sboot = (DA16X_SBOOT_TYPE *)context;

	bcfmeasure = da16x_rtc_get_mirror();
#ifdef DA16X_RAMLIB_LOAD_SFLASH
	BSVIT_PRINT_FDBG( "FLASH(%p): %08x, %p, siz %d\n", context, flashAddress, pMemDst, sizeToRead);
	flash_image_read( (HANDLE)(sboot->sbootflash), flashAddress, pMemDst, sizeToRead);
	//CRYPTO_DBG_DUMP(0, pMemDst, sizeToRead);
#else	// UE boot�� ���� load �Ǿ��ٸ� cache load, jtag �����̸� sflash load
	if (DA16X_FLASH_RALIB_OFFSET == RAMLIB_OFFSET_0 ||
		DA16X_FLASH_RALIB_OFFSET == RAMLIB_OFFSET_0_4MB ||
		DA16X_FLASH_RALIB_OFFSET == RAMLIB_OFFSET_1 ||
		DA16X_FLASH_RALIB_OFFSET == RAMLIB_OFFSET_1_4MB )
	{
		BSVIT_PRINT_FDBG( "cache load\n", context, flashAddress, pMemDst, sizeToRead);
		if (flashAddress > RAMLIB_OFFSET_1_4MB)
			tmpAddress=flashAddress-RAMLIB_OFFSET_1_4MB + RAMLIB_CACHE_OFFSET_4MB;
		else if (flashAddress > RAMLIB_OFFSET_1)
			tmpAddress=flashAddress-RAMLIB_OFFSET_1 + RAMLIB_CACHE_OFFSET;
		else if (flashAddress > RAMLIB_OFFSET_0_4MB)
			tmpAddress=flashAddress-RAMLIB_OFFSET_0_4MB + RAMLIB_CACHE_OFFSET_4MB;
		else
			tmpAddress=flashAddress-RAMLIB_OFFSET_0 + RAMLIB_CACHE_OFFSET;
		BSVIT_PRINT_FDBG( "FLASH(%p): %08x, %p, siz %d\n", context, tmpAddress, pMemDst, sizeToRead);
		memcpy(pMemDst, (void const *)(tmpAddress), sizeToRead);
	}
	/* For JTAG debug */
	else if (DA16X_FLASH_RALIB_OFFSET == 0x0000002b ||
			 DA16X_FLASH_RALIB_OFFSET == 0x0000004b )
	{
	  	if (DA16X_FLASH_RALIB_OFFSET == 0x0000002b)
	  		tmpAddress=flashAddress-RAMLIB_OFFSET_0 + RAMLIB_CACHE_OFFSET + 0xa000;
		else
		  	tmpAddress=flashAddress-RAMLIB_OFFSET_0_4MB + RAMLIB_CACHE_OFFSET_4MB + 0xa000;
	 	memcpy(pMemDst, (void const *)(tmpAddress), sizeToRead);
	}
	else
	{
		BSVIT_PRINT_FDBG( "FLASH(%p): %08x, %p, siz %d\n", context, flashAddress, pMemDst, sizeToRead);
		flash_image_read( (HANDLE)(sboot->sbootflash), flashAddress, pMemDst, sizeToRead);
	}
#endif
	bcfmeasure = da16x_rtc_get_mirror() - bcfmeasure;
	sboot->bcfmeasureflash = sboot->bcfmeasureflash + bcfmeasure;

	return BSVIT_ERROR__OK;
}

#define CC_CERT_SEC_DEBUG_REDIRECT_MAGIC     0x52446B63

typedef struct {
        uint32_t magicNumber;           /* Magic number to validate the certificate. */
        uint32_t certVersion;           /* retmem address  or sflash offset (0x4 or 0x7) */
} DA16XRedirectCertHeader_t;

static uint32_t DA16X_sdebug_redirection(HANDLE sflash, uint32_t lcs, uint8_t *pCertPkgPtr, uint32_t pkgSize)
{
	DA16XRedirectCertHeader_t *CertHeader;

	CertHeader = (DA16XRedirectCertHeader_t *)pCertPkgPtr;

	if( (CertHeader != NULL) && (pkgSize != 0) && (CertHeader->magicNumber == CC_CERT_SEC_DEBUG_REDIRECT_MAGIC) ){
		if( (lcs == CC_BSV_SECURE_LCS)
			&& (CertHeader->certVersion > DA16X_RETMEM_BASE) && (CertHeader->certVersion < DA16X_RETMEM_END) ){

			void *offset;
			offset = (void *)(CertHeader->certVersion);
			da16x_hwcopycrc32( sizeof(UINT32), (UINT8 *)pCertPkgPtr, (void *)(offset), (UINT32)(pkgSize), (~0));

			CertHeader = (DA16XRedirectCertHeader_t *)pCertPkgPtr;

			if( (CertHeader->magicNumber == 0) || (CertHeader->magicNumber == 0xFFFFFFFF)){
				return BSVIT_ERROR__FAIL;
			}
		}else
		if((lcs == CC_BSV_SECURE_LCS)
			&& ((BOOT_MEM_GET(CertHeader->certVersion) == BOOT_SFLASH) || (BOOT_MEM_GET(CertHeader->certVersion) == BOOT_EFLASH)) ){

			UINT32 soffset;
			soffset = BOOT_OFFSET_GET(CertHeader->certVersion);

			flash_image_read( sflash, soffset, (VOID *)(pCertPkgPtr), (UINT32)(pkgSize) );

			CertHeader = (DA16XRedirectCertHeader_t *)pCertPkgPtr;

			if( (CertHeader->magicNumber == 0) || (CertHeader->magicNumber == 0xFFFFFFFF)){
				return BSVIT_ERROR__FAIL;
			}
		}else{
			return BSVIT_ERROR__FAIL;
		}
	}

	return BSVIT_ERROR__OK;
}

/******************************************************************************
 *  DA16X_SBoot_Setup( )
 ******************************************************************************/

static BsvItError_t DA16X_SBOOT_OPEN(DA16X_SBOOT_TYPE *da16xsboot, UINT32 faddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32010));

	if( da16xsboot == NULL ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32011));
		return BSVIT_ERROR__FAIL;
	}

	CRYPTO_MEMSET(da16xsboot, 0x00, sizeof(DA16X_SBOOT_TYPE));

	DA16X_SBOOT_BCFM_START(da16xsboot);

	da16xsboot->sbCertInfoCtx = (CCSbCertInfo_t *)CRYPTO_MALLOC(sizeof(CCSbCertInfo_t));
	da16xsboot->pOutCertHeaderInfoAligned = (uint32_t *)CRYPTO_MALLOC(sizeof(CCX509CertHeaderInfo_t));
	da16xsboot->CertHeaderInfoSize = sizeof(CCX509CertHeaderInfo_t);
	da16xsboot->pWorkspaceAligned = (uint32_t *)CRYPTO_MALLOC(CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	if( (da16xsboot->sbCertInfoCtx == NULL)
	    || (da16xsboot->pOutCertHeaderInfoAligned == NULL)
	    || (da16xsboot->pWorkspaceAligned == NULL) ){
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32012));
		goto setup_fin;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32013));

	da16xsboot->sbootflash = flash_image_open( (DA16X_IMG_BOOT|(sizeof(UINT32)*8)) , faddress, NULL);
	if( da16xsboot->sbootflash == NULL ){
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32014));
		goto setup_fin;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32015));

	flash_image_check(da16xsboot->sbootflash, 0, faddress);

	da16xsboot->flashwrap = (CCSbFlashReadFunc) DA16X_sboot_flashRead;
	da16xsboot->stopproc = (USR_CALLBACK) stopproc;

setup_fin:
	DA16X_SBOOT_BCFM_FINISH(da16xsboot);
	DA16X_SBOOT_BCFM_PRINT(da16xsboot, __func__ );

	return rc;
}

static void DA16X_SBOOT_CLOSE(DA16X_SBOOT_TYPE *da16xsboot, UINT32 *jmpaddr)
{
	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32020));

	if( da16xsboot == NULL ){
		if( jmpaddr != NULL ){
			*jmpaddr = 0;
		}
		return ;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32021));

	DA16X_SBOOT_BCFM_START(da16xsboot);

	if( da16xsboot->sbCertInfoCtx != NULL ) 	{
		CRYPTO_FREE(da16xsboot->sbCertInfoCtx);
	}
	if( da16xsboot->pOutCertHeaderInfoAligned != NULL ) {
		CRYPTO_FREE(da16xsboot->pOutCertHeaderInfoAligned);
	}
	if( da16xsboot->pWorkspaceAligned != NULL ) {
		CRYPTO_FREE(da16xsboot->pWorkspaceAligned);
	}
	if( da16xsboot->sbootflash != NULL ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32022));
		flash_image_close(da16xsboot->sbootflash);
	}

	DA16X_SBOOT_BCFM_FINISH(da16xsboot);
	DA16X_SBOOT_BCFM_PRINT(da16xsboot, __func__ );

	if( jmpaddr != NULL ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32023));

		*jmpaddr = da16xsboot->jmpaddr;

		if(  (*jmpaddr != 0) && (da16xsboot->stopproc != NULL) ){
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32024));
			da16xsboot->stopproc( (VOID *)(*jmpaddr) );
		}
	}

	CRYPTO_MEMSET(da16xsboot, 0x00, sizeof(DA16X_SBOOT_TYPE));

	return ;
}

static BsvItError_t DA16X_SBOOT_VERIFICATION(DA16X_SBOOT_TYPE *da16xsboot, UINT32 faddress)
{
	UINT32 certsize;
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32030));

	if( da16xsboot == NULL ){
		return BSVIT_ERROR__FAIL;
	}

	DA16X_SBOOT_BCFM_START(da16xsboot);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32031));

	ccRc = CC_SbCertChainVerificationInit(da16xsboot->sbCertInfoCtx);
	if (ccRc != CC_OK)
	{
		BSVIT_PRINT_ERROR("CC_SbCertChainVerificationInit failed with - 0x%X\n", ccRc);
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32032));
		goto verify_fin;
	}
	else
	{
		BSVIT_PRINT_DBG("CC_SbCertChainVerificationInit succeeded!!\n");
	}

	/* Verify chain certificates */
	for (da16xsboot->chainIndex = 0; da16xsboot->chainIndex < DA16X_MAX_CHAIN ; ++da16xsboot->chainIndex)
	{
		CCAddr_t storeFlashAddress;

		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32033));
		certsize = 0;
		storeFlashAddress = (CCAddr_t)flash_image_certificate(da16xsboot->sbootflash, faddress, (UINT32)(da16xsboot->chainIndex), &certsize);

		if( storeFlashAddress == 0 ){
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32034));
			if( da16xsboot->chainIndex == 0){
				rc = BSVIT_ERROR__OK; // for RMA
				goto verify_fin;
			}
			break;
		}

		BSVIT_PRINT_DBG("CC_SbCertVerifySingle %d = %08x, %d\n", da16xsboot->chainIndex, storeFlashAddress, certsize);

		/* verify key certificate */
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32035));

		ccRc = CC_SbCertVerifySingle(da16xsboot->flashwrap,
				da16xsboot,
				(unsigned long)DA16X_ACRYPT_BASE,
				storeFlashAddress,
				da16xsboot->sbCertInfoCtx,
				//pOutCertHeaderInfoAligned, CertHeaderInfoSize,
				NULL, 0,
				da16xsboot->pWorkspaceAligned,
				CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("CC_SbCertVerifySingle for cert[%d] failed with - 0x%X\n", da16xsboot->chainIndex, ccRc);
			rc = BSVIT_ERROR__FAIL;
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32036));
			goto verify_fin;
		}
		else
		{
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32037));
			BSVIT_PRINT_DBG("CC_SbCertVerifySingle for cert[%d] succeeded!!\n", da16xsboot->chainIndex);
		}
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32038));
	flash_image_extract(da16xsboot->sbootflash, faddress, &(da16xsboot->loadaddr), &(da16xsboot->jmpaddr));

verify_fin:
	DA16X_SBOOT_BCFM_FINISH(da16xsboot);
	DA16X_SBOOT_BCFM_PRINT(da16xsboot, __func__ );

	return rc;
}

static BsvItError_t DA16X_SBOOT_DEBUG(uint32_t lcs, DA16X_SBOOT_TYPE *da16xsboot, UINT32 faddress)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t *pWorkspaceAligned = NULL;
	uint32_t *pCertPkgPtr = NULL;
	uint32_t pkgSize = 0;
	uint32_t rcRmaFlag = 0;
	uint32_t rmaEntrySignalsSet = 0;
	UINT32	storeFlashAddress;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32040));

	if( da16xsboot == NULL ){
		return BSVIT_ERROR__FAIL;
	}

	pWorkspaceAligned = (uint32_t *)CRYPTO_MALLOC(CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);
	if( pWorkspaceAligned == NULL ){
		return BSVIT_ERROR__FAIL;
	}

	DA16X_SBOOT_BCFM_START(da16xsboot);

	CRYPTO_MEMSET(pWorkspaceAligned, 0, CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32041));

	storeFlashAddress = (UINT32)flash_image_certificate(da16xsboot->sbootflash, faddress, 0xFFFFFFFF, (UINT32 *)&pkgSize);

	if( storeFlashAddress == 0 ){
		rc = BSVIT_ERROR__OK; // Skip
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32042));
		pCertPkgPtr = NULL; 			// DCU Test
		goto /*dbg_fin*/ dbg_nul_cert;		// DCU Test
	}

	pCertPkgPtr = (uint32_t *)CRYPTO_MALLOC(pkgSize);
	if( pCertPkgPtr == NULL ){
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32043));
		goto /*dbg_fin*/ dbg_nul_cert;		// DCU Test
	}

	BSVIT_PRINT_DBG("DbgCert: %08x, %d\n", storeFlashAddress, pkgSize);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32044));

	//flash_image_read( da16xsboot->sbootflash, storeFlashAddress, pCertPkgPtr, pkgSize);
	DA16X_sboot_flashRead(storeFlashAddress, (uint8_t *)pCertPkgPtr, (uint32_t)pkgSize, da16xsboot);

	// DebugCert Redirection
	if( DA16X_sdebug_redirection((da16xsboot->sbootflash), lcs, (uint8_t *)pCertPkgPtr, (uint32_t)(pkgSize)) == BSVIT_ERROR__FAIL ){
		CRYPTO_FREE(pCertPkgPtr);
		pCertPkgPtr = NULL; 			// DCU Test
		pkgSize = 0;				// DCU Test
	}

	//////////////////////////////////////////////////////
dbg_nul_cert:						// DCU Test
	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32045));

	ccRc = CC_BsvSecureDebugSet(DA16X_ACRYPT_BASE,
	                pCertPkgPtr,
	                pkgSize,
	                &rcRmaFlag,
	                pWorkspaceAligned,
	                CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	if (ccRc != CC_OK)
	{
		if( ccRc == CC_BSV_AO_WRITE_FAILED_ERR ){
			BSVIT_PRINT_ERROR("Already locked in CC_BsvSecureDebugSet, ccRc 0x%x\n", ccRc);
			rc = BSVIT_ERROR__OK; // Reboot or DPM Boot case
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3204A));
		}else{
			BSVIT_PRINT_ERROR("Failed to CC_BsvSecureDebugSet, ccRc 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32046));
		}
		goto dbg_fin;
	}else{
		BSVIT_PRINT_DBG("CC_BsvSecureDebugSet for DbgCert succeeded!!\n");
	}

	/* is the provided certificate RMA certificate */
	if( (lcs != CC_BSV_RMA_LCS) && (rcRmaFlag == true) )
	{
		BSVIT_PRINT_DBG("RMA mode enable - %d\n", lcs);
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32047));

		ccRc = CC_BsvRMAModeEnable(DA16X_ACRYPT_BASE);
		if (ccRc != CC_OK)
		{
			rc = BSVIT_ERROR__FAIL;
			goto dbg_fin;
		}
	}

	//TODO: rmaEntrySignalsSet = ???;

	if( (lcs != CC_BSV_RMA_LCS) && (rcRmaFlag || rmaEntrySignalsSet ) )
	{
		//TODO: BootROM_RmaModeEntry();
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32048));
	}

	rmaEntrySignalsSet = DA16X_SecureBoot_GetLock2(); // ICV-Lock Test

	if( (lcs == CC_BSV_SECURE_LCS) && (rcRmaFlag != true) && (rmaEntrySignalsSet != 0) ){
		// CC_BsvICVKeyLock, 4-75
		// lock the ICV Keys to prevent the OEM.
		ccRc = CC_BsvICVKeyLock(DA16X_ACRYPT_BASE, true, true);
		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("Failed CC_BsvICVKeyLock - rc = 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			goto dbg_fin;
		}

		// CC_BsvICVRMAFlagBitLock
		// prevent the OEM from a one-sided transition to RMA.
		ccRc = CC_BsvICVRMAFlagBitLock(DA16X_ACRYPT_BASE);
		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("Failed CC_BsvICVRMAFlagBitLock - rc = 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			goto dbg_fin;
		}
	}

	BSVIT_PRINT_DBG("DA16X_SBoot_Debug:OK\n");

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32049));

dbg_fin:
	if(pWorkspaceAligned != NULL) CRYPTO_FREE(pWorkspaceAligned);
	if(pCertPkgPtr != NULL) CRYPTO_FREE(pCertPkgPtr);

	DA16X_SBOOT_BCFM_FINISH(da16xsboot);
	DA16X_SBOOT_BCFM_PRINT(da16xsboot, __func__ );

	return rc;
}


static BsvItError_t DA16X_SDEBUG_FAST(uint32_t lcs, HANDLE sflash, UINT32 faddress)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc = CC_OK;
	uint32_t *pWorkspaceAligned = NULL;
	uint32_t *pCertPkgPtr = NULL;
	uint32_t pkgSize = 0;
	uint32_t rcRmaFlag = 0;
	uint32_t rmaEntrySignalsSet = 0;
	UINT32	storeFlashAddress;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32140));

	pWorkspaceAligned = (uint32_t *)CRYPTO_MALLOC(CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);
	if( pWorkspaceAligned == NULL ){
		return BSVIT_ERROR__FAIL;
	}

	CRYPTO_MEMSET(pWorkspaceAligned, 0, CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	if( sflash != NULL ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32141));
		storeFlashAddress = (UINT32)flash_image_certificate(sflash, faddress, 0xFFFFFFFF, (UINT32 *)&pkgSize);
	}else{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3214B));
		storeFlashAddress = 0;
	}

	if( storeFlashAddress == 0 ){
		rc = BSVIT_ERROR__OK; // Skip
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32142));
		pCertPkgPtr = NULL; 			// DCU Test
		goto /*fstdbg_fin*/ fstdbg_nul_cert;	// DCU Test
	}

	pCertPkgPtr = (uint32_t *)CRYPTO_MALLOC(pkgSize);
	if( pCertPkgPtr == NULL ){
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32143));
		goto /*fstdbg_fin*/ fstdbg_nul_cert;	// DCU Test
	}

	BSVIT_PRINT_DBG("FstDbgCert: %08x, %d\n", storeFlashAddress, pkgSize);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32144));

	flash_image_read( sflash, storeFlashAddress, pCertPkgPtr, pkgSize);

	// DebugCert Redirection
	if( DA16X_sdebug_redirection(sflash, lcs, (uint8_t *)pCertPkgPtr, (uint32_t)(pkgSize)) == BSVIT_ERROR__FAIL ){
		CRYPTO_FREE(pCertPkgPtr);
		pCertPkgPtr = NULL; 			// DCU Test
		pkgSize = 0;				// DCU Test
	}

	//////////////////////////////////////////////////////
fstdbg_nul_cert:						// DCU Test
	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32145));

	ccRc = CC_BsvSecureDebugSet(DA16X_ACRYPT_BASE,
	                pCertPkgPtr,
	                pkgSize,
	                &rcRmaFlag,
	                pWorkspaceAligned,
	                CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	if (ccRc != CC_OK)
	{
		if( ccRc == CC_BSV_AO_WRITE_FAILED_ERR ){
			BSVIT_PRINT_ERROR("FstDebug: Already locked in CC_BsvSecureDebugSet, ccRc 0x%x\n", ccRc);
			rc = BSVIT_ERROR__OK; // Reboot or DPM Boot case
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC3214A));
		}else{
			BSVIT_PRINT_ERROR("FstDebug: Failed to CC_BsvSecureDebugSet, ccRc 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32146));
		}
		goto fstdbg_fin;
	}else{
		BSVIT_PRINT_DBG("FstDebug: CC_BsvSecureDebugSet succeeded!!\n");
	}

	/* is the provided certificate RMA certificate */
	if( (lcs != CC_BSV_RMA_LCS) && (rcRmaFlag == true) )
	{
		BSVIT_PRINT_DBG("FstDebug: RMA mode enable - %d\n", lcs);
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32147));

		ccRc = CC_BsvRMAModeEnable(DA16X_ACRYPT_BASE);
		if (ccRc != CC_OK)
		{
			rc = BSVIT_ERROR__FAIL;
			goto fstdbg_fin;
		}
	}

	//TODO: rmaEntrySignalsSet = ???;

	if( (lcs != CC_BSV_RMA_LCS) && (rcRmaFlag || rmaEntrySignalsSet ) )
	{
		//TODO: BootROM_RmaModeEntry();
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32148));
	}

	rmaEntrySignalsSet = DA16X_SecureBoot_GetLock2(); // ICV-Lock Test

	if( (lcs == CC_BSV_SECURE_LCS) && (rcRmaFlag != true) && (rmaEntrySignalsSet != 0) ){
		// CC_BsvICVKeyLock, 4-75
		// lock the ICV Keys to prevent the OEM.
		ccRc = CC_BsvICVKeyLock(DA16X_ACRYPT_BASE, true, true);
		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("FstDebug: Failed CC_BsvICVKeyLock - rc = 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			goto fstdbg_fin;
		}

		// CC_BsvICVRMAFlagBitLock
		// prevent the OEM from a one-sided transition to RMA.
		ccRc = CC_BsvICVRMAFlagBitLock(DA16X_ACRYPT_BASE);
		if (ccRc != CC_OK)
		{
			BSVIT_PRINT_ERROR("FstDebug: Failed CC_BsvICVRMAFlagBitLock - rc = 0x%x\n", ccRc);
			rc = BSVIT_ERROR__FAIL;
			goto fstdbg_fin;
		}
	}

	BSVIT_PRINT_DBG("DA16X_Fast_Debug:OK\n");

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32149));

fstdbg_fin:
	if(pWorkspaceAligned != NULL) CRYPTO_FREE(pWorkspaceAligned);
	if(pCertPkgPtr != NULL) CRYPTO_FREE(pCertPkgPtr);

	return rc;
}


/******************************************************************************
 *  DA16X_SBoot_CM( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_CM(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	DA16X_SBOOT_TYPE *da16xsboot, tda16xsboot;
	UINT32	jmpaddr;

	da16xsboot = (DA16X_SBOOT_TYPE *)&tda16xsboot;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320C1));

	rc = DA16X_SBOOT_OPEN(da16xsboot, faddress, stopproc);
	if( rc != BSVIT_ERROR__OK ){
		goto cm_bail;
	}

	// 1. test secure boot and secure debug process
	// DCU Test : Secure Boot after Secure Debug
	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320C3));

	rc = DA16X_SBOOT_DEBUG(lcs, da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto cm_bail;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320C2));

	rc = DA16X_SBOOT_VERIFICATION(da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto cm_bail;
	}

	// 2. check for the existence of an ICV signature
	//	run ICV tool

	// 3. Load 2nd stage boot loader and continue with cold boot sequece runtime software

cm_bail:
	if( rc == BSVIT_ERROR__OK )
	{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320C4));

		DA16X_SBOOT_CLOSE(da16xsboot, &jmpaddr );

		if( (jmpaddr != 0) && (jaddress == NULL) )
		{ /* NOTICE !! do not modify this clause !! */
			volatile UINT32  dst_addr;
			/* get Reset handler address     */
			dst_addr = *((UINT32 *)((jmpaddr)|0x04));
			/* Jump into App                 */
			dst_addr = (dst_addr|0x01);
			ASM_BRANCH(dst_addr);	/* Jump into App */
		}
		if( jaddress != NULL ){
			*jaddress = jmpaddr;
		}
	}else{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320CE));

		DA16X_SBOOT_CLOSE(da16xsboot, NULL );
	}

	return rc;
}

/******************************************************************************
 *  DA16X_SBoot_DM( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_DM(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	DA16X_SBOOT_TYPE *da16xsboot, tda16xsboot;
	UINT32	jmpaddr;

	da16xsboot = (DA16X_SBOOT_TYPE *)&tda16xsboot;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320D1));

	// DCU Test : Secure Boot after Secure Debug

	rc = DA16X_SBOOT_OPEN(da16xsboot, faddress, stopproc);
	if( rc != BSVIT_ERROR__OK ){
		// DCU Test
		UINT32	rcRmaFlag = 0;
		DA16X_SecureBoot_Fatal(&rcRmaFlag);
		goto dm_bail;
	}

	// 1. check secure debug certificate

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320D3));

	rc = DA16X_SBOOT_DEBUG(lcs, da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto dm_bail;
	}

	// 2. check secure boot certificate

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320D2));

	rc = DA16X_SBOOT_VERIFICATION(da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto dm_bail;
	}

	// 3. check for the existence of an OEM signature
	//	run OEM tool

	// 4. load 2nd stage boot loader and continue with cold boot sequence runtime software

dm_bail:
	if( rc == BSVIT_ERROR__OK )
	{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320D4));

		DA16X_SBOOT_CLOSE(da16xsboot, &jmpaddr );

		if( (jmpaddr != 0) && (jaddress == NULL) )
		{ /* NOTICE !! do not modify this clause !! */
			volatile UINT32  dst_addr;
			/* get Reset handler address     */
			dst_addr = *((UINT32 *)((jmpaddr)|0x04));
			/* Jump into App                 */
			dst_addr = (dst_addr|0x01);
			ASM_BRANCH(dst_addr);	/* Jump into App */
		}
		if( jaddress != NULL ){
			*jaddress = jmpaddr;
		}
	}else{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320DE));

		DA16X_SBOOT_CLOSE(da16xsboot, NULL );
	}


	return rc;
}

/******************************************************************************
 *  DA16X_SBoot_Secure( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_Secure(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;

	DA16X_SBOOT_TYPE *da16xsboot, tda16xsboot;
	UINT32	jmpaddr;

	da16xsboot = (DA16X_SBOOT_TYPE *)&tda16xsboot;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320A1));

	// DCU Test : Secure Boot after Secure Debug

	rc = DA16X_SBOOT_OPEN(da16xsboot, faddress, stopproc);
	if( rc != BSVIT_ERROR__OK ){
		// DCU Test
		UINT32	rcRmaFlag = 0;
		DA16X_SecureBoot_Fatal(&rcRmaFlag);
		goto secure_bail;
	}

	// 1. calculate SOC_ID
	// export the SOC_ID

	// 2. check secure debug certificate
	// RMA may enter here

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320A3));

	rc = DA16X_SBOOT_DEBUG(lcs, da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto secure_bail;
	}

	// 3. check secure boot certificate
	//	verify secure boot cert., load trusted code, 2nd stage boot loader or Secure OS, using Secure boot Sequence
	//	Two entities handling (ICV & OEM)

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320A2));

	rc = DA16X_SBOOT_VERIFICATION(da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto secure_bail;
	}

secure_bail:
	if( rc == BSVIT_ERROR__OK )
	{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320A4));

		DA16X_SBOOT_CLOSE(da16xsboot, &jmpaddr );

		if( (jmpaddr != 0) && (jaddress == NULL) )
		{ /* NOTICE !! do not modify this clause !! */
			volatile UINT32  dst_addr;
			/* get Reset handler address     */
			dst_addr = *((UINT32 *)((jmpaddr)|0x04));
			/* Jump into App                 */
			dst_addr = (dst_addr|0x01);
			ASM_BRANCH(dst_addr);	/* Jump into App */
		}
		if( jaddress != NULL ){
			*jaddress = jmpaddr;
		}
	}else{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320AE));

		DA16X_SBOOT_CLOSE(da16xsboot, NULL );
	}


	return rc;
}

/******************************************************************************
 *  DA16X_SBoot_RMA( )
 ******************************************************************************/

static BsvItError_t DA16X_SBoot_RMA(uint32_t lcs, UINT32 faddress, UINT32 *jaddress, USR_CALLBACK stopproc)
{
	BsvItError_t rc = BSVIT_ERROR__OK;

	DA16X_SBOOT_TYPE *da16xsboot, tda16xsboot;
	UINT32	jmpaddr;

	da16xsboot = (DA16X_SBOOT_TYPE *)&tda16xsboot;

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320E1));

	// 1. OEM secure debug cerificate issue RMA
	// 3. ICV secure debug cerificate issue RMA

	rc = DA16X_SBOOT_OPEN(da16xsboot, faddress, stopproc);
	if( rc != BSVIT_ERROR__OK ){
		goto rma_bail;
	}

	// DCU Test : Secure Boot after Secure Debug

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320E3));

	rc = DA16X_SBOOT_DEBUG(lcs, da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto rma_bail;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320E2));

	rc = DA16X_SBOOT_VERIFICATION(da16xsboot, faddress);
	if( rc != BSVIT_ERROR__OK ){
		goto rma_bail;
	}

rma_bail:
	if( rc == BSVIT_ERROR__OK )
	{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320E4));

		DA16X_SBOOT_CLOSE(da16xsboot, &jmpaddr );

		// 2. call CC_BsvRMAModeEnable() and POR
		// 4. call CC_BsvRMAModeEnable() and POR

		CC_BsvRMAModeEnable(DA16X_ACRYPT_BASE);

		// TODO: Unsecure ??

		if( (jmpaddr != 0) && (jaddress == NULL) )
		{ /* NOTICE !! do not modify this clause !! */
			volatile UINT32  dst_addr;
			/* get Reset handler address     */
			dst_addr = *((UINT32 *)((jmpaddr)|0x04));
			/* Jump into App                 */
			dst_addr = (dst_addr|0x01);
			ASM_BRANCH(dst_addr);	/* Jump into App */
		}else
		if( (jmpaddr != 0) && jaddress != NULL ){
			*jaddress = jmpaddr;
		}

	}else{
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320EE));

		DA16X_SBOOT_CLOSE(da16xsboot, NULL );
	}

	return rc;
}

/******************************************************************************
 *  DA16X_SecureBoot_Fatal( ) : Test only
 ******************************************************************************/

UINT32	DA16X_SecureBoot_Fatal(UINT32 *rcRmaFlag)
{
	uint32_t rc;
	uint32_t *pWorkspaceAligned;

	pWorkspaceAligned = (uint32_t *)CRYPTO_MALLOC(CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320F1));

	rc = CC_BsvFatalErrorSet(DA16X_ACRYPT_BASE);
	if( rc != 0 ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320F2));
		goto fatal_bail;
	}

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320F3));

	rc = CC_BsvSecureDebugSet(DA16X_ACRYPT_BASE,
	                NULL, 0, (uint32_t *)rcRmaFlag
	                , pWorkspaceAligned, CC_SB_MIN_WORKSPACE_SIZE_IN_BYTES);
	if( rc != 0 ){
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC320F4));
		goto fatal_bail;
	}

fatal_bail:
	CRYPTO_FREE(pWorkspaceAligned);

	return (UINT32)rc;
}

/******************************************************************************
 *  DA16X_SecureBoot_Fatal( ) : Test only
 ******************************************************************************/

static BsvItError_t DA16X_DeviceCompleteDisable(void)
{
	BsvItError_t rc = BSVIT_ERROR__OK;
	CCError_t ccRc;

	ccRc = CC_DeviceCompleteDisable(DA16X_ACRYPT_BASE);

	if( ccRc == CC_BSV_AO_WRITE_FAILED_ERR ){
		rc = BSVIT_ERROR__OK; // Reboot or DPM Boot case
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC321FA));
	}else{
		rc = BSVIT_ERROR__FAIL;
		SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC321F6));
	}

	return rc;
}

/******************************************************************************
 *  DA16X_SecureBoot_CMPU( )
 ******************************************************************************/

UINT32	DA16X_SecureBoot_CMPU(UINT8 *pCmpuData, UINT32 rflag)
{
	uint32_t *workspace;
	CCError_t status;

	workspace = (uint32_t *)CRYPTO_MALLOC(CMPU_WORKSPACE_MINIMUM_SIZE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32090));

	if( rflag != 0 ){
		workspace[0] = (uint32_t)0xFC9CC312; // hidden tag
		workspace[1] = (uint32_t)rflag;
	}

	status = CCProd_Cmpu(DA16X_ACRYPT_BASE
		, (CCCmpuData_t *)pCmpuData
		, (unsigned long)workspace
		, CMPU_WORKSPACE_MINIMUM_SIZE);

	CRYPTO_FREE(workspace);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32091));

	//TEST : Test_HalPerformPowerOnReset();

	return (UINT32)status;
}

/******************************************************************************
 *  DA16X_SecureBoot_DMPU( )
 ******************************************************************************/

UINT32	DA16X_SecureBoot_DMPU(UINT8 *pDmpuData)
{
	uint32_t *workspace;
	CCError_t status;

	workspace = (uint32_t *)CRYPTO_MALLOC(DMPU_WORKSPACE_MINIMUM_SIZE);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32092));

	status = CCProd_Dmpu(DA16X_ACRYPT_BASE
		, (CCDmpuData_t *)pDmpuData
		, (unsigned long)workspace
		, DMPU_WORKSPACE_MINIMUM_SIZE);

	CRYPTO_FREE(workspace);

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32093));

	//TEST : Test_HalPerformPowerOnReset();

	return (UINT32)status;
}

void	DA16X_SecureBoot_ColdReset(void)
{
	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32C10)|0x0D);
	//200918: Test_HalPerformPowerOnReset(); // FPGA Test
}

/******************************************************************************
 *  DA16X_SecureBoot_GetLock( )
 ******************************************************************************/

#define WRITE_ENV(offset, val) { \
	volatile uint32_t ii1; \
        (*(volatile uint32_t *)(DX_BASE_ENV_REGS + (offset))) = (uint32_t)(val); \
        for(ii1=0; ii1<500; ii1++);\
}

#define READ_ENV(offset) \
	*(volatile uint32_t *)(DX_BASE_ENV_REGS + (offset))


UINT32 DA16X_SecureBoot_GetLock(void)
{
	uint32_t OtpWord;

	CC_BSV_READ_OTP_WORD(DA16X_ACRYPT_BASE, (CC_OTP_SB_LOADER_CODE_OFFSET<<2), &OtpWord);
	//CC_BsvOTPWordRead(DA16X_ACRYPT_BASE, CC_OTP_SB_LOADER_CODE_OFFSET, &OtpWord );

	/* OTP standby mode */
	//*((volatile UINT32 *)(0x40120000)) = 0x04000000;

	return (((OtpWord & 0x01) == 0x01) ? TRUE : FALSE );
}

UINT32 DA16X_SecureBoot_SetLock(void)
{
	uint32_t error = 0;
	uint32_t OtpWord, envflag;

	OtpWord = 0x01;

	envflag = READ_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET);
	WRITE_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET , 0x1UL);

	error = CC_BsvOTPWordWrite(DA16X_ACRYPT_BASE, CC_OTP_SB_LOADER_CODE_OFFSET, OtpWord);

	WRITE_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET , envflag);

	return ((error == CC_OK) ? TRUE : FALSE );
}

void DA16X_SecureBoot_OTPLock(UINT32 mode)
{
	WRITE_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET , mode);
}

UINT32 DA16X_SecureBoot_SecureLCS(void)
{
	uint32_t lcs;

	CC_BsvLcsGet(DA16X_ACRYPT_BASE, &lcs);

	return ((lcs == CC_BSV_SECURE_LCS) ? TRUE : FALSE);
}

/******************************************************************************
 * New Secure Model : ICV-Lock Test
 ******************************************************************************/

UINT32 DA16X_SecureBoot_GetLock2(void)
{
	uint32_t OtpWord;

	CC_BsvOTPWordRead(DA16X_ACRYPT_BASE, CC_OTP_SB_LOADER_CODE_OFFSET, &OtpWord );

	return (((OtpWord & 0x03) == 0x03) ? TRUE : FALSE );
}

UINT32 DA16X_SecureBoot_SetLock2(void)
{
	uint32_t error = 0;
	uint32_t OtpWord, envflag;

	OtpWord = 0x02;

	envflag = READ_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET);
	WRITE_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET , 0x1UL);

	error = CC_BsvOTPWordWrite(DA16X_ACRYPT_BASE, CC_OTP_SB_LOADER_CODE_OFFSET, OtpWord);

	WRITE_ENV(DX_ENV_OTP_FILTER_OFF_REG_OFFSET , envflag);

	return ((error == CC_OK) ? TRUE : FALSE );
}

/******************************************************************************
 *  DA16X_Secure_Asset( )
 ******************************************************************************/

UINT32 DA16X_Secure_Asset(UINT32 Owner, UINT32 AssetID
		, UINT32 *InAssetData, UINT32 AssetSize, UINT8 *OutAssetData)
{
	CCError_t status;
	size_t	  assetSize;

	status = mbedtls_util_asset_pkg_unpack(
			(CCAssetProvKeyType_t)Owner,
			(uint32_t)AssetID,
			(uint32_t *)InAssetData,
			(size_t)AssetSize,
			(uint8_t *)OutAssetData,
			(size_t *)&assetSize );

	if (status != CC_OK){
		return 0; // zero size
	}

	return assetSize;
}

/******************************************************************************
 *  DA16X_Secure_Asset_RuntimePack( )
 ******************************************************************************/
#include "cc_hal_plat.h"
#include "cc_regs.h"
#include "cc_pal_mem.h"
#include "cc_pal_mutex.h"
#include "cc_pal_abort.h"
#include "cc_util_int_defs.h"
#include "mbedtls_cc_util_defs.h"
#include "cc_util_error.h"
#include "cc_aes_defs.h"
#include "ccm.h"
#include "aes_driver.h"
#include "driver_defs.h"
#include "cc_util_cmac.h"
#include "mbedtls_cc_util_asset_prov.h"
#include "cc_util_asset_prov_int.h"
#include "prod_util.h"
#include "cmpu_llf_rnd.h"
#include "prod_crypto_driver.h"
#include "cmpu_derivation.h"

typedef struct {
        uint32_t  token;
        uint32_t  version;
        uint32_t  assetSize;
        uint32_t  reserved[CC_ASSET_PROV_RESERVED_WORD_SIZE];
        uint8_t   nonce[CC_ASSET_PROV_NONCE_SIZE];
        uint8_t   enctag[CC_ASSET_PROV_TAG_SIZE];
} CCRunAssetProvPkg_t;

INT32 DA16X_Secure_Asset_RuntimePack(AssetKeyType_t KeyType, UINT32 noncetype
		, AssetUserKeyData_t *KeyData, UINT32 AssetID, char *title
		, UINT8 *InAssetData, UINT32 AssetSize, UINT8 *OutAssetPkgData)
{
	uint32_t  rc = CC_OK;
	uint32_t  i;
	CCUtilAesCmacResult_t         keyProv = { 0 };
	uint8_t         dataIn[CC_UTIL_MAX_KDF_SIZE_IN_BYTES] = { 0 };
	uint32_t    dataInSize = CC_UTIL_MAX_KDF_SIZE_IN_BYTES;
	uint8_t     provLabel = 'P';
	CCRunAssetProvPkg_t   *pAssetPackage = NULL;
	mbedtls_ccm_context ccmCtx;
	uint8_t	    *pencRunAsset;

	/* Validate Inputs */
	if ((InAssetData == NULL) ||
		(OutAssetPkgData == NULL) ||
		/*(AssetSize > CC_ASSET_PROV_MAX_ASSET_SIZE) ||*/
		(AssetSize == 0) ||
		((AssetSize % CC_ASSET_PROV_BLOCK_SIZE) != 0) ||
		(((UtilKeyType_t)KeyType) > UTIL_KCEICV_KEY) )
	{
		CC_PAL_LOG_ERR("Invalid params");
		return (INT32)CC_UTIL_ILLEGAL_PARAMS_ERROR;

	}

        rc = CCProd_Init();
        if (rc != CC_OK) {
                CC_PAL_LOG_ERR("Failed to CCProd_Init 0x%x\n", rc);
                goto DA16X_sa_pack_end;
        }

	pAssetPackage = (CCRunAssetProvPkg_t *)OutAssetPkgData;
	pencRunAsset  = (uint8_t *)(OutAssetPkgData + sizeof(CCRunAssetProvPkg_t));

	/* fill asset size, must be multiply of 16 bytes */
	pAssetPackage->assetSize = AssetSize;

	/* fill package token and version */
	pAssetPackage->token = CC_RUNASSET_PROV_TOKEN;
	pAssetPackage->version = CC_RUNASSET_PROV_VERSION;

	if( title != NULL ){
		CRYPTO_MEMCPY(pAssetPackage->reserved, title
			, (CC_ASSET_PROV_RESERVED_WORD_SIZE*sizeof(uint32_t)) );
	}

	/* Generate dataIn buffer for CMAC: iteration || 'P' || 0x00 || asset Id || 0x80
		since deruved key is 128 bits we have only 1 iteration */
	rc = UtilCmacBuildDataForDerivation(&provLabel,sizeof(provLabel),
	                              (uint8_t *)&AssetID, sizeof(AssetID),
	                             dataIn, (size_t *)&dataInSize,
	                             (size_t)CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed UtilCmacBuildDataForDerivation 0x%x", rc);
		CRYPTO_MEMSET(pAssetPackage, 0, sizeof(CCRunAssetProvPkg_t));
		goto DA16X_sa_pack_end;
	}
	dataIn[0] = 1;  // only 1 iteration
	rc = UtilCmacDeriveKey(((UtilKeyType_t)KeyType),
				((CCAesUserKeyData_t *)KeyData),
				dataIn, dataInSize,
				keyProv);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed UtilCmacDeriveKey 0x%x", rc);
		CRYPTO_MEMSET(pAssetPackage, 0, sizeof(CCRunAssetProvPkg_t));
		goto DA16X_sa_pack_end;
	}

	/* Decrypt and authenticate the BLOB */
	mbedtls_ccm_init(&ccmCtx);

	rc = mbedtls_ccm_setkey(&ccmCtx, MBEDTLS_CIPHER_ID_AES, keyProv, CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES * CC_BITS_IN_BYTE);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed to mbedtls_ccm_setkey 0x%x\n", rc);
		CRYPTO_MEMSET(pAssetPackage, 0, sizeof(CCRunAssetProvPkg_t));
		goto DA16X_sa_pack_end;
	}

	if( noncetype == 0xFFFFFFFF ){
		for(i = 0; i < CC_ASSET_PROV_NONCE_SIZE; i++ ){
			pAssetPackage->nonce[i] = da16x_random();
		}
	}else{
		uint8_t   *pKey;
		uint8_t   *pIv;
		uint32_t *pEntrSrc;
		uint32_t  sourceSize;
		uint32_t *pRndWorkBuff;

	        pRndWorkBuff = (uint32_t *)CRYPTO_MALLOC(CMPU_WORKSPACE_MINIMUM_SIZE);
	        pKey = (uint8_t *)CRYPTO_MALLOC(CC_PROD_AES_Key256Bits_SIZE_IN_BYTES);
	        pIv  = (uint8_t *)CRYPTO_MALLOC(CC_PROD_AES_IV_COUNTER_SIZE_IN_BYTES);

		if( (pRndWorkBuff == NULL) || (pKey == NULL) || (pIv == NULL) ){
			if( pRndWorkBuff != NULL ){
				CRYPTO_FREE(pRndWorkBuff);
			}
			if( pKey != NULL ){
				CRYPTO_FREE(pKey);
			}
			if( pIv != NULL ){
				CRYPTO_FREE(pIv);
			}
			rc = CC_UTIL_FATAL_ERROR;
			goto DA16X_sa_pack_end;
		}

		pRndWorkBuff[0] = (uint32_t)0xFC9CC312; // hidden tag
		pRndWorkBuff[1] = (uint32_t)noncetype;

	        rc = CC_PROD_LLF_RND_GetTrngSource((uint32_t **)&pEntrSrc, &sourceSize, pRndWorkBuff);
	        if (rc != CC_OK) {
	                CC_PAL_LOG_ERR("failed CC_PROD_LLF_RND_GetTrngSource, error is 0x%X\n", rc);
	                goto DA16X_sa_pack_end_free;
	        }
	        rc = CC_PROD_Derivation_Instantiate(pEntrSrc,
					sourceSize,
					pKey,
					pIv);
	        if (rc != CC_OK) {
	                CC_PAL_LOG_ERR("failed to CC_PROD_Derivation_Instantiate, error 0x%x\n", rc);
	                goto DA16X_sa_pack_end_free;
	        }
	        rc = CC_PROD_Derivation_Generate(pKey,
					pIv,
					(uint32_t *)(pRndWorkBuff),
					(CC_ASSET_PROV_NONCE_SIZE*2));
	        if (rc != CC_OK) {
	                CC_PAL_LOG_ERR("failed to CC_PROD_Derivation_Generate, error 0x%x\n", rc);
	                goto DA16X_sa_pack_end_free;
	        }

	        rc = CC_PROD_LLF_RND_VerifyGeneration((uint8_t *)(pRndWorkBuff));
	        if (rc != CC_OK) {
	                CC_PAL_LOG_ERR("failed to CC_PROD_LLF_RND_VerifyGeneration, error 0x%x\n", rc);
	                goto DA16X_sa_pack_end_free;
	        }

		CRYPTO_MEMCPY((pAssetPackage->nonce), pRndWorkBuff, CC_ASSET_PROV_NONCE_SIZE);

DA16X_sa_pack_end_free:
		CRYPTO_FREE(pRndWorkBuff);
		CRYPTO_FREE(pKey);
		CRYPTO_FREE(pIv);

		if( rc != CC_OK){
			goto DA16X_sa_pack_end;
		}
	}

	rc = mbedtls_ccm_encrypt_and_tag(&ccmCtx, pAssetPackage->assetSize,
			pAssetPackage->nonce, CC_ASSET_PROV_NONCE_SIZE,
			(uint8_t *)pAssetPackage, CC_ASSET_PROV_ADATA_SIZE,
			(uint8_t *)InAssetData, pencRunAsset,
			pAssetPackage->enctag, CC_ASSET_PROV_TAG_SIZE);


	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed to mbedtls_ccm_auth_decrypt 0x%x\n", rc);
		CRYPTO_MEMSET(pAssetPackage, 0, sizeof(CCRunAssetProvPkg_t));
		goto DA16X_sa_pack_end;
	}

	rc = (sizeof(CCRunAssetProvPkg_t) + pAssetPackage->assetSize);

DA16X_sa_pack_end:
	CCPROD_Fini();

	// Set output data
	return (INT32)rc;
}

/******************************************************************************
 *  DA16X_Secure_Asset_RuntimeUnpack( )
 ******************************************************************************/

INT32 DA16X_Secure_Asset_RuntimeUnpack(AssetKeyType_t KeyType
		, AssetUserKeyData_t *KeyData, UINT32 AssetID
		, UINT8 *InAssetPkgData, UINT32 AssetPkgSize, UINT8 *OutAssetData)
{
	uint32_t  rc = CC_OK;
	CCUtilAesCmacResult_t         keyProv = { 0 };
	uint8_t         dataIn[CC_UTIL_MAX_KDF_SIZE_IN_BYTES] = { 0 };
	uint32_t    dataInSize = CC_UTIL_MAX_KDF_SIZE_IN_BYTES;
	uint8_t     provLabel = 'P';
	CCRunAssetProvPkg_t   *pAssetPackage = NULL;
	mbedtls_ccm_context ccmCtx;
	uint8_t	   *pencRunAsset;

	/* Validate Inputs */
	if ((InAssetPkgData == NULL) ||
		(OutAssetData == NULL) ||
		/*(AssetPkgSize > CC_ASSET_PROV_MAX_ASSET_PKG_SIZE) ||*/
		(((UtilKeyType_t)KeyType) > UTIL_KCEICV_KEY) )
	{
		CC_PAL_LOG_ERR("Invalid params");
		return (INT32)CC_UTIL_ILLEGAL_PARAMS_ERROR;

	}

        rc = CCProd_Init();
        if (rc != CC_OK) {
                CC_PAL_LOG_ERR("Failed to CCProd_Init 0x%x\n", rc);
                return (INT32)rc;
        }

	pAssetPackage = (CCRunAssetProvPkg_t *)InAssetPkgData;
	pencRunAsset  = (uint8_t *)(InAssetPkgData + sizeof(CCRunAssetProvPkg_t));

	/* Validate asset size, must be multiply of 16 bytes */
	if ( /*(pAssetPackage->assetSize > CC_ASSET_PROV_MAX_ASSET_SIZE) ||*/
	(pAssetPackage->assetSize == 0) ||
	(pAssetPackage->assetSize % CC_ASSET_PROV_BLOCK_SIZE)) {
		CC_PAL_LOG_ERR("Invalid asset size 0x%x", pAssetPackage->assetSize);
		return (INT32)CC_UTIL_ILLEGAL_PARAMS_ERROR;
	}
	/* Verify package token and version */
	if ((pAssetPackage->token != CC_RUNASSET_PROV_TOKEN) ||
	(pAssetPackage->version != CC_RUNASSET_PROV_VERSION)) {
		CC_PAL_LOG_ERR("Invalid token or version");
		return (INT32)CC_UTIL_ILLEGAL_PARAMS_ERROR;
	}

	/* Generate dataIn buffer for CMAC: iteration || 'P' || 0x00 || asset Id || 0x80
		since deruved key is 128 bits we have only 1 iteration */
	rc = UtilCmacBuildDataForDerivation(&provLabel,sizeof(provLabel),
	                              (uint8_t *)&AssetID, sizeof(AssetID),
	                             dataIn, (size_t *)&dataInSize,
	                             (size_t)CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed UtilCmacBuildDataForDerivation 0x%x", rc);
		return rc;
	}
	dataIn[0] = 1;  // only 1 iteration
	rc = UtilCmacDeriveKey(((UtilKeyType_t)KeyType),
				((CCAesUserKeyData_t *)KeyData),
				dataIn, dataInSize,
				keyProv);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed UtilCmacDeriveKey 0x%x", rc);
		return rc;
	}

	/* Decrypt and authenticate the BLOB */
	mbedtls_ccm_init(&ccmCtx);

	rc = mbedtls_ccm_setkey(&ccmCtx, MBEDTLS_CIPHER_ID_AES, keyProv, CC_UTIL_AES_CMAC_RESULT_SIZE_IN_BYTES * CC_BITS_IN_BYTE);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed to mbedtls_ccm_setkey 0x%x\n", rc);
		return rc;
	}

	rc = mbedtls_ccm_auth_decrypt(&ccmCtx, pAssetPackage->assetSize,
	              pAssetPackage->nonce, CC_ASSET_PROV_NONCE_SIZE,
	              (uint8_t *)pAssetPackage, CC_ASSET_PROV_ADATA_SIZE,
	              pencRunAsset, OutAssetData,
	              pAssetPackage->enctag, CC_ASSET_PROV_TAG_SIZE);
	if (rc != 0) {
		CC_PAL_LOG_ERR("Failed to mbedtls_ccm_auth_decrypt 0x%x\n", rc);
		return rc;
	}

    // Set output data
    return (pAssetPackage->assetSize);

}

/******************************************************************************
 *  DA16X_Secure_Asset_RuntimeUnpack( )
 ******************************************************************************/

/**
 * @file TRNG_test.c
 *
 * This file defines the data collection function for characterization of the
 * Discretix TRNG.
 */
/*
* This confidential and proprietary software may be used only as authorised by a licensing agreement from ARM Israel.
* Copyright (C) 2010 ARM Limited or its affiliates. All rights reserved.
* The entire notice above must be reproduced on all authorized copies and copies may only be
* made to the extent permitted by a licensing agreement from ARM Israel.
 */
/*
 * Revision history:
 * 1.0 (16-Mar-2010): Initial revision
 * 1.1 (11-Jul-2010): Added extra metadata to output format
 * 1.2 (18-Jul-2010): Fixed reset sequence
 *                    Changed sampling rate metadata to 32-bit
 * 1.3 (09-Jun-2011): Added 8 bit processor support
 * 1.4 (07-Feb-2013): Only 192bit EHR support
 * 1.5 (03-Mar-2013): Replace isSlowMode with TRNGMode
 * 1.6 (06-Mar-2013): Support both DxTRNG and CryptoCell4.4
 * 1.7 (14-May-2015): Support for 800-90B
 * 1.8 (07-Oct-2015):  renamed DX to CC
 */

#define HW_RNG_ISR_REG_ADDR                 0x104UL
#define HW_RNG_ICR_REG_ADDR                 0x108UL
#define HW_TRNG_CONFIG_REG_ADDR             0x10CUL
#define HW_TRNG_VALID_REG_ADDR              0x110UL
#define HW_EHR_DATA_ADDR_0_REG_ADDR         0x114UL
#define HW_RND_SOURCE_ENABLE_REG_ADDR       0x12CUL
#define HW_SAMPLE_CNT1_REG_ADDR             0x130UL
#define HW_TRNG_DEBUG_CONTROL_REG_ADDR      0x138UL
#define HW_RNG_SW_RESET_REG_ADDR            0x140UL
#define HW_RNG_VERSION_REG_ADDR             0x1C0UL
#define HW_RNG_CLK_ENABLE_REG_ADDR          0x1C4UL

/* @@@ */
#define CryptoCell
/* @@@ */

typedef unsigned int DxUint32_t;
/* report any compile errors in the next two lines to Discretix */
typedef int __testCharSize[((unsigned char)~0)==255?1:-1];
typedef int __testDxUint32Size[(sizeof(DxUint32_t)==4)?1:-1];

#define CC_GEN_WriteRegister(base_addr, reg_addr, val) \
    do { ((volatile DxUint32_t*)(base_addr))[(reg_addr) / sizeof(DxUint32_t)] = (DxUint32_t)(val); } while(0)

#define CC_GEN_ReadRegister(base_addr, reg_addr)  ((volatile DxUint32_t*)(base_addr))[(reg_addr) / sizeof(DxUint32_t)]


/**
 * Collect TRNG output for characterization
 *
 * @regBaseAddress:     Base address in system memory map of TRNG registers
 * @TRNGMode:           0 - FTRNG; 1 ?MM TRNG driver; 2 ?NIST TRNG driver
 * @roscLength:         Ring oscillator length (0 to 3)
 * @sampleCount:        Ring oscillator sampling rate
 * @buffSize:           Size of buffer passed in dataBuff_ptr; must be between 52 and 2^24 bytes
 * @dataBuff_ptr:       Buffer for results
 *
 * The function's output is 0 if the collection succeeds, or a (non-zero) error code on failure.
 */
int CC_TST_TRNG(    unsigned long regBaseAddress,
                    unsigned int TRNGMode,
                    unsigned int roscLength,
                    unsigned int sampleCount,
                    unsigned int buffSize,
                    DxUint32_t *dataBuff_ptr)
{
    /* LOCAL DECLARATIONS */

    unsigned int Error;

    /* loop variable */
    unsigned int i,j;

    /* The number of full blocks needed */
    unsigned int NumOfBlocks;

    /* Hardware parameters */
    //DxUint32_t HwVersion;
    unsigned int EhrSizeInWords;
    unsigned int tmpSamplCnt = 0;

    /* FUNCTION LOGIC */

    /* ............... local initializations .............................. */
    /* -------------------------------------------------------------------- */

    //HwVersion = CC_GEN_ReadRegister( regBaseAddress, HW_RNG_VERSION_REG_ADDR );
    EhrSizeInWords = 6;

    /* ............... validate inputs .................................... */
    /* -------------------------------------------------------------------- */

    if (buffSize < (7 + EhrSizeInWords) * sizeof(DxUint32_t)) return -1;
    if (buffSize >= (1<<24)) return -1;

    /* ........... initializing the hardware .............................. */
    /* -------------------------------------------------------------------- */

	/* enable the HW RND clock   */
	CC_GEN_WriteRegister( regBaseAddress, HW_RNG_CLK_ENABLE_REG_ADDR, 0x1);


    /* reset the RNG block */
    CC_GEN_WriteRegister( regBaseAddress, HW_RNG_SW_RESET_REG_ADDR, 0x1 );

	do{
		/* enable the HW RND clock   */
		CC_GEN_WriteRegister( regBaseAddress, HW_RNG_CLK_ENABLE_REG_ADDR, 0x1);

		/* set sampling ratio (rng_clocks) between consequtive bits */
		CC_GEN_WriteRegister( regBaseAddress, HW_SAMPLE_CNT1_REG_ADDR, sampleCount);

		/* read the sampling ratio  */
		tmpSamplCnt = CC_GEN_ReadRegister( regBaseAddress, HW_SAMPLE_CNT1_REG_ADDR );
	}while (tmpSamplCnt != sampleCount);

    /* enable the RNG clock  */

    /* configure RNG */
    CC_GEN_WriteRegister( regBaseAddress, HW_TRNG_CONFIG_REG_ADDR, roscLength & 0x03 );

     if ( TRNGMode == 0 )
    {
        /* For fast TRNG: VNC_BYPASS + TRNG_CRNGT_BYPASS + AUTO_CORRELATE_BYPASS */
        CC_GEN_WriteRegister( regBaseAddress, HW_TRNG_DEBUG_CONTROL_REG_ADDR , 0x0000000E);
    }
	 else if(TRNGMode == 2)//800-90B
    {
        /* For 800-90B TRNG: VNC_BYPASS + TRNG_CRNGT_BYPASS + AUTO_CORRELATE_BYPASS */
        CC_GEN_WriteRegister( regBaseAddress, HW_TRNG_DEBUG_CONTROL_REG_ADDR , 0x0000000A);
    }else if(TRNGMode == 1)
    {
        /* For TRNGS: enable all hardware test blocks */
        CC_GEN_WriteRegister( regBaseAddress, HW_TRNG_DEBUG_CONTROL_REG_ADDR , 0x00000000);
    }

    /* enable the RNG source  */
    CC_GEN_WriteRegister( regBaseAddress, HW_RND_SOURCE_ENABLE_REG_ADDR, 0x1 );

    /* ........... executing the RND operation ............................ */
    /* -------------------------------------------------------------------- */

    /* Write header into buffer */
    *(dataBuff_ptr++) = 0xAABBCCDD;
    *(dataBuff_ptr++) = (TRNGMode << 31) |
                        (roscLength << 24) |
                        buffSize;
    *(dataBuff_ptr++) = sampleCount;
    *(dataBuff_ptr++) = 0xAABBCCDD;

    /* The number of full blocks needed */
    NumOfBlocks = (buffSize/sizeof(DxUint32_t) - 7) / EhrSizeInWords;

    Error = 0;

    /* fill the Output buffer with up to full blocks */
    /* BEGIN TIMING: start time measurement at this point */
    for( i = 0 ; i < NumOfBlocks ; i++ )
    {
        DxUint32_t valid_at_start, valid;
        /*wait for RND ready*/
        valid_at_start = CC_GEN_ReadRegister( regBaseAddress, HW_TRNG_VALID_REG_ADDR );
        valid = valid_at_start;
        while( (valid & 0x3) == 0x0 )
            valid = CC_GEN_ReadRegister( regBaseAddress, HW_RNG_ISR_REG_ADDR );
        if ( (valid_at_start != 0) && (i != 0) ) Error = 1;
        if ( (valid & ~1) != 0 ) Error |= (valid & ~1);
		if ( Error & 0x2 ) break; /* autocorrelation error is irrecoverable */
			CC_GEN_WriteRegister( regBaseAddress, HW_RNG_ICR_REG_ADDR, ~0 );

        for (j=0;j<EhrSizeInWords;j++)
        {
            /* load the current random data to the output buffer */
            *(dataBuff_ptr++) = CC_GEN_ReadRegister( regBaseAddress, HW_EHR_DATA_ADDR_0_REG_ADDR+(j*sizeof(DxUint32_t)) );
        }

        // another read to reset the bits-counter
        valid = CC_GEN_ReadRegister( regBaseAddress, HW_RNG_ISR_REG_ADDR );

    }
    /* END TIMING: end time measurement at this point */

    /* Write trailer into buffer */
    *(dataBuff_ptr++) = 0xDDCCBBAA;
    /* report about "no bit was skipped during collection" - meaning of mask 0x1 */
    *(dataBuff_ptr++) = Error & 0x1;
    *(dataBuff_ptr++) = 0xDDCCBBAA;

    /* disable the RND source  */
    CC_GEN_WriteRegister( regBaseAddress, HW_RND_SOURCE_ENABLE_REG_ADDR, 0x0 );

    /* close the Hardware clock */
    //CC_GEN_WriteRegister( regBaseAddress, HW_RNG_CLK_ENABLE_REG_ADDR , 0x0 );

    /* .............. end of function ..................................... */
    /* -------------------------------------------------------------------- */

    return Error;
}

#define	TRNG_WORKSPACE_SIZE	128

void DA16X_Trng_Test(int ssize, int mode)
{
	int rc, idx;
	unsigned int roclen, trngmode, sidx, lcnt;
	uint32_t *pRndWorkBuff[2] ;
	unsigned int scount[] = { 1000, 500, 200, 100, 5, 1, 0};
	const char *trngtitle[] = { "F-TRNG ", "800-90B", "NIST   " };

        pRndWorkBuff[0] = (uint32_t *)CRYPTO_MALLOC(TRNG_WORKSPACE_SIZE);
        pRndWorkBuff[1] = (uint32_t *)CRYPTO_MALLOC(TRNG_WORKSPACE_SIZE);
	CRYPTO_MEMSET(pRndWorkBuff[0], 0, TRNG_WORKSPACE_SIZE);
	CRYPTO_MEMSET(pRndWorkBuff[1], 0, TRNG_WORKSPACE_SIZE);

	//CCProd_Init();

	if( ssize > TRNG_WORKSPACE_SIZE ){
		ssize = TRNG_WORKSPACE_SIZE;
	}

	idx = 0;
	for( trngmode = 0; trngmode <= 2; trngmode ++ )
	for( roclen = 0; roclen <= 3; roclen ++ )
	for( sidx = 0; scount[sidx] != 0; sidx++ )
	{
		UINT32 errorcnt ;

		CRYPTO_PRINTF("[CC_TST_TRNG: %s (%d), %d, %4d, %d]=============================\n"
			, trngtitle[trngmode], trngmode, roclen, scount[sidx], ssize
			);

		errorcnt = 0;
		for( lcnt = 0; lcnt < 16; lcnt++ )
		{
			CRYPTO_MEMSET(pRndWorkBuff[idx], 0, ssize);

			rc = CC_TST_TRNG(DA16X_ACRYPT_BASE
				, trngmode
				, roclen /* 0 to 3 */
				, scount[sidx]
				, ssize
				, (DxUint32_t *)pRndWorkBuff[idx]);

			errorcnt = errorcnt + rc;

			if( mode == TRUE ){
				CRYPTO_PRINTF("%2d-th (%s, %d, %d, %d) = %d\n"
					, lcnt, trngtitle[trngmode], roclen, scount[sidx], ssize, rc );
			}

			if( CRYPTO_MEMCMP(pRndWorkBuff[idx], pRndWorkBuff[((idx+1) % 2)], ssize) != 0 ){
				int i;
				uint8_t *pRndWorkBuff8[2];

				pRndWorkBuff8[0] = (uint8_t *)pRndWorkBuff[idx];
				pRndWorkBuff8[1] = (uint8_t *)pRndWorkBuff[((idx+1) % 2)];

				if( mode == TRUE ){
					for( i = 0; i < ssize; i++ ){
						if( (i % 16) == 0 ){
							CRYPTO_PRINTF("\t");
						}

						if( pRndWorkBuff8[0][i] == pRndWorkBuff8[1][i] ){
							CRYPTO_PRINTF("-- ");
						}else{
							CRYPTO_PRINTF("%02x ", pRndWorkBuff8[0][i]);
						}

						if( (i % 16) == 15 ){
							CRYPTO_PRINTF("\n");
						}
					}
					CRYPTO_PRINTF("\n");
				}
			}

			idx = (idx+1) % 2;
		}

		CRYPTO_PRINTF("[CC_TST_TRNG: %s (%d), %d, %4d, %d] err: %3d \n"
				, trngtitle[trngmode], trngmode, roclen, scount[sidx], ssize
				, errorcnt);
	}

	//CCPROD_Fini();

	CRYPTO_FREE(pRndWorkBuff[0]);
	CRYPTO_FREE(pRndWorkBuff[1]);
}


UINT32 DA16X_CalcHuk_Test(UINT8 *HUKBuffer, UINT32 *HUKsize)
{
        uint32_t error = 0;
        uint32_t  zeroCount = 0;
        uint8_t   pKey[CC_PROD_AES_Key256Bits_SIZE_IN_BYTES] = { 0 };
        uint8_t   pIv[CC_PROD_AES_IV_COUNTER_SIZE_IN_BYTES] = { 0 };
        uint32_t *pEntrSrc;
        uint32_t  sourceSize;
        uint32_t *pRndWorkBuff;
	uint32_t *pBuffForOtp;

	*HUKsize = 0;
	pBuffForOtp = (uint32_t *)HUKBuffer;
        /*Call CC_PROD_LLF_RND_GetTrngSource to get entropy bits and to check entropy size*/
        pRndWorkBuff = (uint32_t *)CRYPTO_MALLOC(CMPU_WORKSPACE_MINIMUM_SIZE);

	CCProd_Init();

        error = CC_PROD_LLF_RND_GetTrngSource((uint32_t **)&pEntrSrc, &sourceSize, pRndWorkBuff);
        if (error != CC_OK) {
                CRYPTO_PRINTF("failed CC_PROD_LLF_RND_GetTrngSource, error is 0x%X\n", error);
                goto DA16X_CalcHuk_Test_done;
        }

        error = CC_PROD_Derivation_Instantiate(pEntrSrc,
                                               sourceSize,
                                               pKey,
                                               pIv);
        if (error != CC_OK) {
                CRYPTO_PRINTF("failed to CC_PROD_Derivation_Instantiate, error 0x%x\n", error);
                goto DA16X_CalcHuk_Test_done;
        }

        error = CC_PROD_Derivation_Generate(pKey,
                                            pIv,
                                            pBuffForOtp,
                                            CC_OTP_HUK_SIZE_IN_WORDS * sizeof(uint32_t));
        if (error != CC_OK) {
                CRYPTO_PRINTF("failed to CC_PROD_Derivation_Generate, error 0x%x\n", error);
                goto DA16X_CalcHuk_Test_done;
        }

        error = CC_PROD_LLF_RND_VerifyGeneration((uint8_t *)pBuffForOtp);
        if (error != CC_OK) {
                CRYPTO_PRINTF("failed to CC_PROD_LLF_RND_VerifyGeneration, error 0x%x\n", error);
                goto DA16X_CalcHuk_Test_done;
        }

        /*Count number of zero bits in HUK OTP fileds*/
        error  = CC_PROD_GetZeroCount(pBuffForOtp, CC_OTP_HUK_SIZE_IN_WORDS, &zeroCount);
        if (error != CC_OK) {
                CRYPTO_PRINTF("Invalid Huk zero count\n");
                goto DA16X_CalcHuk_Test_done;
        }

	*HUKsize = (CC_OTP_HUK_SIZE_IN_WORDS*sizeof(uint32_t));

	error = CC_OK;

DA16X_CalcHuk_Test_done:

	CCPROD_Fini();

	CRYPTO_FREE(pRndWorkBuff);

        return error;
}


UINT32 DA16X_SecureBoot_RMA(void)
{
	BsvItError_t rc = BSVIT_ERROR__OK;

	CC_PalInit_Abstract(); // for Mutex
	CC_HalInit();

	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32000));

	rc = DA16X_SBoot_Init();
	if( rc != BSVIT_ERROR__OK ) goto end_step;

//	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32001));
//
//	rc = DA16X_SBoot_Boot(taddress, jaddress, stopproc);
//	if( rc != BSVIT_ERROR__OK ) goto end_step;
//
//	SBROM_DBG_TRIGGER(MODE_CRY_STEP(0xC32002));

end_step:
	CC_HalTerminate();
	CC_PalTerminate_Abstract();

	return (rc == BSVIT_ERROR__OK) ? TRUE : FALSE;
}

/* EOF */
