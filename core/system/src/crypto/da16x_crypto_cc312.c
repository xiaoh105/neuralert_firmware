/**
 ****************************************************************************************
 *
 * @file da16x_crypto_cc312.c
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


//#include "sdk_type.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oal.h"
#include "sal.h"
#include "hal.h"
#include "cal.h"
#include "driver.h"
#include "library.h"

//#include "da16x_system.h"

/* cerberus pal */
#include "cc_pal_types.h"
#include "cc_pal_types_plat.h"
#include "cc_pal_mem.h"
#include "cc_pal_perf.h"
#include "cc_regs.h"
#include "cc_otp_defs.h"
#include "cc_lib.h"
//REMOVE: #include "cc_rnd.h"
#include "cc_hal_plat.h"

#include "dx_nvm.h"
#include "dx_id_registers.h"

/* mbedtls */
#include "mbedtls/config.h"
#include "mbedtls/compat-1.3.h"

#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"

#include "da16x_crypto.h"

#include "da16x_time.h"

#pragma GCC diagnostic ignored "-Wpointer-sign"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

/******************************************************************************
 *
 ******************************************************************************/


typedef		struct {
	CCRndContext_t *rndContext;
	CCRndWorkBuff_t *rndWorkBuff;
} DA16X_CRYPTO_TYPE;

static DA16X_CRYPTO_TYPE *da16x_crypto_driver;


#define DA16X_Crypto_ALLOC(_ptr, _type, _size) 				\
        { 								\
		_ptr = ( _type *)CRYPTO_MALLOC( _size * sizeof(_type));	\
		if( _ptr == NULL ) goto failed;				\
        }

#define DA16X_Crypto_FREE(_ptr) 						\
        if( _ptr != NULL ){ 						\
		CRYPTO_FREE( _ptr );					\
		_ptr = NULL;						\
        }

/******************************************************************************
 *  DA16X_Crypto_ClkMgmt( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#define	CC312_PLL_MARKER	0x000CC312

static CLOCK_LIST_TYPE		cc312_pll_info;

static void cc312_pll_callback(UINT32 clock, void *param)
{
	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC321CC));

	if( ((UINT32)param) == CC312_PLL_MARKER ){
		DA16X_SYSCLOCK->CLK_EN_CC312 = 0;

		DA16X_SYSCLOCK->PLL_CLK_EN_7_CC312 = 0;
		DA16X_SYSCLOCK->PLL_CLK_DIV_7_CC312 = DA16X_SYSCLOCK->PLL_CLK_DIV_0_CPU;
		DA16X_SYSCLOCK->PLL_CLK_EN_7_CC312 = 1;
		DA16X_SYSCLOCK->SRC_CLK_SEL_7_CC312 = 1; // PLL

		DA16X_SYSCLOCK->CLK_EN_CC312 = 1;

		ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC321CD));
	}

}

void	DA16X_Crypto_ClkMgmt(UINT32 mode)
{
	if( mode == TRUE ){
		cc312_pll_info.callback.func = cc312_pll_callback;
		cc312_pll_info.callback.param = (void *)CC312_PLL_MARKER;
		cc312_pll_info.next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, &cc312_pll_info);

		cc312_pll_callback(0xFF, (void *)CC312_PLL_MARKER); // force
	}else{
		_sys_clock_ioctl( SYSCLK_RESET_CALLACK, &cc312_pll_info);
	}
}

static void cc312_pllx2_callback(UINT32 clock, void *param)
{
	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC321CE));

	if( ((UINT32)param) == CC312_PLL_MARKER ){
		DA16X_SYSCLOCK->CLK_EN_CC312 = 0;

		DA16X_SYSCLOCK->PLL_CLK_EN_7_CC312 = 0;
		//DA16X_SYSCLOCK->PLL_CLK_DIV_7_CC312 = ((DA16X_SYSCLOCK->PLL_CLK_DIV_0_CPU+1)>>1)-1; // x2
		DA16X_SYSCLOCK->PLL_CLK_DIV_7_CC312 = da16x_pll_x2check();
		DA16X_SYSCLOCK->PLL_CLK_EN_7_CC312 = 1;
		DA16X_SYSCLOCK->SRC_CLK_SEL_7_CC312 = 1; // PLL

		DA16X_SYSCLOCK->CLK_EN_CC312 = 1;

		ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC321CF));
	}

}

void	DA16X_Crypto_x2ClkMgmt(UINT32 mode)
{
	if( mode == TRUE ){
		cc312_pll_info.callback.func = cc312_pllx2_callback;
		cc312_pll_info.callback.param = (void *)CC312_PLL_MARKER;
		cc312_pll_info.next = NULL;

		_sys_clock_ioctl( SYSCLK_SET_CALLACK, &cc312_pll_info);

		cc312_pllx2_callback(0xFF, (void *)CC312_PLL_MARKER); // force
	}else{
		_sys_clock_ioctl( SYSCLK_RESET_CALLACK, &cc312_pll_info);
	}
}

/******************************************************************************
 *  DA16X_Crypto_Init( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

extern void	mbedtls_hardware_poll_fci_customize(uint32_t rflag);
extern CCError_t CC_BsvCoreClkGatingEnable(unsigned long hwBaseAddress);

UINT32 DA16X_Crypto_Init(UINT32 rflag)
{
	uint32_t lcs;
	CCError_t ccRc;

	if( da16x_crypto_driver != NULL ){
		// Skip Alloc !!
	}else{

		/* initilaise driver */
		DA16X_Crypto_ALLOC( da16x_crypto_driver, DA16X_CRYPTO_TYPE, 1);

		/* use global ptr to be able to free them on exit */
		DA16X_Crypto_ALLOC( da16x_crypto_driver->rndContext, CCRndContext_t, 1);
		DA16X_Crypto_ALLOC( da16x_crypto_driver->rndWorkBuff, CCRndWorkBuff_t, 1);

		/* init Rnd context's inner member */
		if( sizeof(CCRndState_t) > sizeof(mbedtls_ctr_drbg_context) ){
			DA16X_Crypto_ALLOC( da16x_crypto_driver->rndContext->rndState, CCRndState_t, 1);
		}else{
			DA16X_Crypto_ALLOC( da16x_crypto_driver->rndContext->rndState, mbedtls_ctr_drbg_context, 1);
		}
		DA16X_Crypto_ALLOC( da16x_crypto_driver->rndContext->entropyCtx, mbedtls_entropy_context, 1);
	}

	// Unsecure Mode ???
	ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC321C1));
	CC_BsvCoreClkGatingEnable(DA16X_ACRYPT_BASE);

//	ccRc = CC_BsvLcsGetAndInit(DA16X_ACRYPT_BASE, &lcs);
//
//	if (ccRc != CC_OK)
//	{
//		ASIC_DBG_TRIGGER(MODE_CRY_STEP(0xC320FF));
//		goto failed;
//	}
	// FCI hidden parameter
	da16x_crypto_driver->rndWorkBuff->ccRndIntWorkBuff[0] = (uint32_t)0xFC9CC312; // hidden tag
	da16x_crypto_driver->rndWorkBuff->ccRndIntWorkBuff[1] = (uint32_t)rflag;
	mbedtls_hardware_poll_fci_customize((uint32_t)rflag); // hidden tag

	/* initialise CC library */
	if(CC_LibInit(da16x_crypto_driver->rndContext
			, da16x_crypto_driver->rndWorkBuff) == CC_LIB_RET_OK)
	{
		return TRUE;
	}

failed:
	DA16X_Crypto_Finish();

	return FALSE;

}

/******************************************************************************
 *  DA16X_Crypto_Finish( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void DA16X_Crypto_Finish(void)
{
	if( da16x_crypto_driver == NULL ){
		return;
	}

	CC_LibFini(da16x_crypto_driver->rndContext);

	DA16X_Crypto_FREE(da16x_crypto_driver->rndContext->rndState);
	DA16X_Crypto_FREE(da16x_crypto_driver->rndContext->entropyCtx);
	DA16X_Crypto_FREE(da16x_crypto_driver->rndContext);
	DA16X_Crypto_FREE(da16x_crypto_driver->rndWorkBuff);

	DA16X_Crypto_FREE(da16x_crypto_driver);

	da16x_crypto_driver = NULL;
}

/******************************************************************************
 *  mbedtls_cc312_pka_self_test( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#include "cal.h"

#include "cc_rsa_error.h"
#include "rsa_public.h"
#include "cc_rsa_types.h"
#include "pka.h"
#include "pki.h"
#include "cc_pal_mutex.h"
#include "pka_error.h"

#define CCSWAP(word)					\
		( (((word) & 0xF0000000) >> 28) |	\
		  (((word) & 0x0F000000) >> 20) |	\
		  (((word) & 0x00F00000) >> 12) |	\
		  (((word) & 0x000F0000) >>  4) |	\
		  (((word) & 0x0000F000) <<  4) |	\
		  (((word) & 0x00000F00) << 12) |	\
		  (((word) & 0x000000F0) << 20) |	\
		  (((word) & 0x0000000F) << 28)   )

static const uint32_t cctableA[][4] = {
	{ CCSWAP(0xFEDCBA98), CCSWAP(0x76543210), CCSWAP(0xFEDCBA98), CCSWAP(0x76543210) },
	{ CCSWAP(0xA6000000), CCSWAP(0x00000000), CCSWAP(0x00000000), CCSWAP(0x00000000) }, /* Mod_inv */
	//{ CCSWAP(0x30003000), CCSWAP(0x30003000), CCSWAP(0x30003000), CCSWAP(0x30003000) }, /* Mod_exp */
	{ CCSWAP(0x30000000), CCSWAP(0x30000000), CCSWAP(0x30000000), CCSWAP(0x30000000) }, /* Mod_exp */
};

static const uint32_t cctableB[][4] = {
	{ CCSWAP(0x89ABCDEF), CCSWAP(0x01234567), CCSWAP(0x89ABCDEF), CCSWAP(0x01234567) },
	{ CCSWAP(0xA6000000), CCSWAP(0x00000000), CCSWAP(0x00000000), CCSWAP(0x00000000) }, /* Mod_inv */
	{ CCSWAP(0x00050000), CCSWAP(0x00050000), CCSWAP(0x00050000), CCSWAP(0x00050000) }, /* Mod_exp */
};

static const uint32_t cctableC[][4] = {
	{ CCSWAP(0xFEDCBA98), CCSWAP(0x76543210), CCSWAP(0xFEDCBA98), CCSWAP(0x76543210) },
};


#ifdef BUILD_OPT_VSIM
#define	PKA_CRYPTO_PRINTF(...)	DBGT_Printf( __VA_ARGS__ )
#define	PKA_CRYPTO_DUMP(...)	cc312_rev_dump( __VA_ARGS__ )
#else	//BUILD_OPT_VSIM
#define	PKA_CRYPTO_PRINTF(...)	CRYPTO_PRINTF( __VA_ARGS__ )
#define	PKA_CRYPTO_DUMP(...)	cc312_rev_dump( __VA_ARGS__ )
#endif	//BUILD_OPT_VSIM

static void cc312_view_status(void)
{
	uint32_t status;
#if 0
	PKA_CRYPTO_PRINTF("RGF.SRAM_RDY : %08x\n", CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, SRAM_DATA_READY)) );
	PKA_CRYPTO_PRINTF("PKA.PIPE_RDY : %08x\n", CC_HAL_READ_REGISTER(CC_REG_OFFSET(CRY_KERNEL, PKA_PIPE_RDY)) );
	PKA_CRYPTO_PRINTF("PKA.DONE     : %08x\n", CC_HAL_READ_REGISTER(CC_REG_OFFSET(CRY_KERNEL, PKA_DONE)) );
	status = CC_HAL_READ_REGISTER(CC_REG_OFFSET(CRY_KERNEL, PKA_STATUS)) ;
	PKA_CRYPTO_PRINTF("PKA.STATUS   : %08x\n", status );

	PKA_CRYPTO_PRINTF("PKA.STATUS.alu_msb    : %08x\n",
		((status>>DX_PKA_STATUS_ALU_MSB_4BITS_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_MSB_4BITS_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.alu_lsb    : %08x\n",
		((status>>DX_PKA_STATUS_ALU_LSB_4BITS_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_LSB_4BITS_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.sign       : %08x\n",
		((status>>DX_PKA_STATUS_ALU_SIGN_OUT_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_SIGN_OUT_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.carry      : %08x\n",
		((status>>DX_PKA_STATUS_ALU_CARRY_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_CARRY_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.carry_mod  : %08x\n",
		((status>>DX_PKA_STATUS_ALU_CARRY_MOD_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_CARRY_MOD_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.sub_zero   : %08x\n",
		((status>>DX_PKA_STATUS_ALU_SUB_IS_ZERO_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_SUB_IS_ZERO_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.out_zero   : %08x\n",
		((status>>DX_PKA_STATUS_ALU_OUT_ZERO_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_OUT_ZERO_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.mod_ovf    : %08x\n",
		((status>>DX_PKA_STATUS_ALU_MODOVRFLW_BIT_SHIFT)&((1<<DX_PKA_STATUS_ALU_MODOVRFLW_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.divbyzero  : %08x\n",
		((status>>DX_PKA_STATUS_DIV_BY_ZERO_BIT_SHIFT)&((1<<DX_PKA_STATUS_DIV_BY_ZERO_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.modinv_zero: %08x\n",
		((status>>DX_PKA_STATUS_MODINV_OF_ZERO_BIT_SHIFT)&((1<<DX_PKA_STATUS_MODINV_OF_ZERO_BIT_SIZE)-1))
		);
	PKA_CRYPTO_PRINTF("PKA.STATUS.opcode     : %08x\n",
		((status>>DX_PKA_STATUS_OPCODE_BIT_SHIFT)&((1<<DX_PKA_STATUS_OPCODE_BIT_SIZE)-1))
		);
#endif
	PKA_2CLEAR(LEN_ID_MAX_BITS/*LenID*/, PKA_REG_T0/*dest*/);
	PKA_2CLEAR(LEN_ID_MAX_BITS/*LenID*/, PKA_REG_T1/*dest*/);
}

#if 0
static void cc312_dump( const char *title, uint32_t *buf, size_t len )
{
    size_t i;
    unsigned char *buf8 = (unsigned char *)buf;

    PKA_CRYPTO_PRINTF( "%s (%d)", title , len);
    for( i = 0; i < len; i++ ){
    	if( (i % 32) == 0 ){ PKA_CRYPTO_PRINTF("\n\t"); }
    	//if( i == 0 ){ PKA_CRYPTO_PRINTF("0x"); }
        PKA_CRYPTO_PRINTF("%c%c", "0123456789ABCDEF" [buf8[i] / 16],
                       "0123456789ABCDEF" [buf8[i] % 16] );
    }
    PKA_CRYPTO_PRINTF( "\n" );
}
#endif /*0*/

static void cc312_rev_dump( const char *title, uint32_t *buf, size_t len )
{
    size_t i, j;
    unsigned char *buf8 = (unsigned char *)buf;

    PKA_CRYPTO_PRINTF( "%s (%d)", title , len);
    for( j = 0, i = len ; i > 0; i-- ){
    	if( (j % 32) == 0 ){ PKA_CRYPTO_PRINTF("\n\t"); }
    	//if( j == 0 ){ PKA_CRYPTO_PRINTF("0x"); }
        PKA_CRYPTO_PRINTF("%c%c", "0123456789ABCDEF" [buf8[i-1] / 16],
                       "0123456789ABCDEF" [buf8[i-1] % 16] );
	j++;
    }
    PKA_CRYPTO_PRINTF( "\n" );
}

#include "mbedtls/bignum.h"

int mbedtls_mpi_reverse_binary( mbedtls_mpi *X, const unsigned char *buf, size_t buflen )
{
#define ciL    (sizeof(mbedtls_mpi_uint))         /* chars in limb  */
#define CHARS_TO_LIMBS(i) ( (i) / ciL + ( (i) % ciL != 0 ) )
    int ret;
    size_t i, j, n;

    for( n = 0; n < buflen; n++ )
        if( buf[n] != 0 )
            break;

    MBEDTLS_MPI_CHK( mbedtls_mpi_grow( X, CHARS_TO_LIMBS( buflen - n ) ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_lset( X, 0 ) );

    for( i = buflen, j = 0; i > n; i--, j++ )
        X->p[j / ciL] |= ((mbedtls_mpi_uint) buf[j]) << ((j % ciL) << 3);

cleanup:
    return( ret );
}

int mbedtls_cc312_pka_self_test(int verbose)
{
	int status, i, j;
	CCError_t error;
	uint32_t  nSizeInBits;
	uint32_t  pkaReqRegs;
	uint32_t  *N, *NP, *oprA, *oprB, *oprC, *Res;
	uint32_t  Nsiz, NPsiz, oprAsiz, oprBsiz, oprCsiz, Ressiz;


	status = TRUE;

	N = (uint32_t *)CRYPTO_MALLOC(512);
	NP = (uint32_t *)CRYPTO_MALLOC(512);
	oprA = (uint32_t *)CRYPTO_MALLOC(512);
	oprB = (uint32_t *)CRYPTO_MALLOC(512);
	oprC = (uint32_t *)CRYPTO_MALLOC(512);
	Res  = (uint32_t *)CRYPTO_MALLOC(512);

	nSizeInBits = 16*32;
	pkaReqRegs = 7;
	error = PkaInitAndMutexLock(nSizeInBits, &pkaReqRegs);
	if (error != CC_OK) {
		status = FALSE;
		goto Failed;
	}

	PKA_2CLEAR(LEN_ID_MAX_BITS, PKA_REG_N);
	PKA_2CLEAR(LEN_ID_MAX_BITS, PKA_REG_NP);
	PKA_2CLEAR(LEN_ID_MAX_BITS, 2);
	PKA_2CLEAR(LEN_ID_MAX_BITS, 3);
	PKA_2CLEAR(LEN_ID_MAX_BITS, 4);
	PKA_2CLEAR(LEN_ID_MAX_BITS, 5);
	PKA_2CLEAR(LEN_ID_MAX_BITS, 6);

	CRYPTO_MEMCPY(oprA, cctableA[0], 16);
	CRYPTO_MEMCPY(oprB, cctableB[0], 16);
	CRYPTO_MEMCPY(oprC, cctableC[0], 16);
	oprAsiz = 4;
	oprBsiz = 4;
	oprCsiz = 4;
	Ressiz  = 8;

	Nsiz  = 1;
	NPsiz = 0;

PKA_CRYPTO_PRINTF("[mbedtls_cc312_pka_self_test]\n");
	//CC_HAL_WRITE_REGISTER(CC_REG_OFFSET(CRY_KERNEL, PKA_SW_RESET), 0x01);

	PKA_WAIT_ON_PKA_PIPE_READY();
	PKA_WAIT_ON_PKA_DONE();

	{
		uint32_t pidReg[CC_BSV_PID_SIZE_WORDS] = {0};
		uint32_t cidReg[CC_BSV_CID_SIZE_WORDS] = {0};

		/* verify peripheral ID (PIDR) */
	        pidReg[0] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, PERIPHERAL_ID_0));
	        pidReg[1] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, PERIPHERAL_ID_1));
	        pidReg[2] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, PERIPHERAL_ID_2));
	        pidReg[3] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, PERIPHERAL_ID_3));
	        pidReg[4] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, PERIPHERAL_ID_4));

		/* verify component ID (CIDR) */
	        cidReg[0] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, COMPONENT_ID_0));
	        cidReg[1] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, COMPONENT_ID_1));
	        cidReg[2] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, COMPONENT_ID_2));
	        cidReg[3] = CC_HAL_READ_REGISTER(CC_REG_OFFSET(HOST_RGF, COMPONENT_ID_3));

		PKA_CRYPTO_PRINTF("[CC312] PID = %02x.%02x.%02x.%02x.%02x\n"
					, pidReg[0], pidReg[1], pidReg[2], pidReg[3], pidReg[4] );

		PKA_CRYPTO_PRINTF("[CC312] CID = %02x.%02x.%02x.%02x\n"
					, cidReg[0], cidReg[1], cidReg[2], cidReg[3] );
	}

	/* copy modulus N into r0 register */
	cc312_view_status();

PKA_CRYPTO_PRINTF("PKA.AND ==========\n");
	PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, oprAsiz);
	PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, oprBsiz);
	PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, oprCsiz);
	PKA_AND(LEN_ID_N_BITS, 5, 2, 3);

	/* copy result into output: r4 =>DataOut */
	PkaCopyDataFromPkaReg(Res, Ressiz, 5/*srcReg*/);

	PKA_CRYPTO_DUMP("oprA", oprA, oprAsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("oprB", oprB, oprBsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("Res" , Res ,  Ressiz*sizeof(uint32_t));
	cc312_view_status();

PKA_CRYPTO_PRINTF("PKA.XOR ==========\n");
	PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, oprAsiz);
	PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, oprBsiz);
	PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, oprCsiz);

	PKA_XOR(LEN_ID_N_BITS, 5, 2, 3);

	/* copy result into output: r4 =>DataOut */
	PkaCopyDataFromPkaReg(Res, Ressiz, 5/*srcReg*/);

	PKA_CRYPTO_DUMP("oprA", oprA, oprAsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("oprB", oprB, oprBsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("Res" , Res ,  Ressiz*sizeof(uint32_t));
	cc312_view_status();

PKA_CRYPTO_PRINTF("PKA.ADD ==========\n");
	PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, oprAsiz);
	PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, oprBsiz);
	PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, oprCsiz);

	PKA_ADD(LEN_ID_N_BITS, 5, 2, 3);

	/* copy result into output: r4 =>DataOut */
	PkaCopyDataFromPkaReg(Res, Ressiz, 5/*srcReg*/);

	PKA_CRYPTO_DUMP("oprA", oprA, oprAsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("oprB", oprB, oprBsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("Res" , Res ,  Ressiz*sizeof(uint32_t));
	cc312_view_status();

PKA_CRYPTO_PRINTF("PKA.SUB ==========\n");
	PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, oprAsiz);
	PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, oprBsiz);
	PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, oprCsiz);

	PKA_SUB(LEN_ID_N_BITS, 5, 2, 3);

	/* copy result into output: r4 =>DataOut */
	PkaCopyDataFromPkaReg(Res, Ressiz, 5/*srcReg*/);

	PKA_CRYPTO_DUMP("oprA", oprA, oprAsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("oprB", oprB, oprBsiz*sizeof(uint32_t));
	PKA_CRYPTO_DUMP("Res" , Res ,  Ressiz*sizeof(uint32_t));
	cc312_view_status();

	PkaFinishAndMutexUnlock(pkaReqRegs);

	for(j = 1; j <= 4; j++)
	for(i = 1; i <= 4; i++)
	{
		uint32_t HRessiz;
#define OPR_B_LENG	(j)
		CRYPTO_MEMSET(oprA, 0, 16);
		CRYPTO_MEMSET(oprB, 0, 16);
		CRYPTO_MEMSET(oprC, 0, 16);

		CRYPTO_MEMCPY(oprA, cctableA[2], i*sizeof(uint32_t));
		CRYPTO_MEMCPY(oprB, cctableB[2], OPR_B_LENG*sizeof(uint32_t));
		CRYPTO_MEMCPY(oprC, cctableC[0], 16);

		PKA_CRYPTO_PRINTF("\n\nMultiplication ########################\n");

		Ressiz  = (i+OPR_B_LENG);
		HRessiz = (i+OPR_B_LENG + 1);
		nSizeInBits = (i+OPR_B_LENG + 1)*32;
		pkaReqRegs = 5;

		error = PkaInitAndMutexLock(nSizeInBits, &pkaReqRegs);
		if (error != CC_OK) {
			status = FALSE;
			goto Failed;
		}

		PKA_CRYPTO_PRINTF("PKA.MUL.L [oprA %d B * oprB %d B] ==========\n", (i*4), (OPR_B_LENG*4));
		PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, i);
		PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, OPR_B_LENG);
		//PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, i);

		PKA_MUL_LOW(LEN_ID_N_BITS, 4, 2, 3);

		/* copy result into output: r4 =>DataOut */
		PkaCopyDataFromPkaReg(Res, Ressiz, 4/*srcReg*/);

		PKA_CRYPTO_DUMP("oprA", oprA, i*sizeof(uint32_t));
		PKA_CRYPTO_DUMP("oprB", oprB, OPR_B_LENG*sizeof(uint32_t));
		PKA_CRYPTO_DUMP("Res" , Res ,  (Ressiz)*sizeof(uint32_t));
		cc312_view_status();

		PKA_CRYPTO_PRINTF("PKA.MUL.H [oprA %d B * oprB %d B] ==========\n", (i*4), (OPR_B_LENG*4));
		PkaCopyDataIntoPkaReg(2/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprA/*srcPtr*/, i);
		PkaCopyDataIntoPkaReg(3/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprB/*srcPtr*/, OPR_B_LENG);
		//PkaCopyDataIntoPkaReg(4/*dstReg*/, LEN_ID_MAX_BITS/*LenID*/,  oprC/*srcPtr*/, i);

		PKA_MUL_HIGH(LEN_ID_N_BITS, 4, 2, 3);

		/* copy result into output: r4 =>DataOut */
		PkaCopyDataFromPkaReg(Res, HRessiz, 4/*srcReg*/);

		PKA_CRYPTO_DUMP("oprA", oprA, i*sizeof(uint32_t));
		PKA_CRYPTO_DUMP("oprB", oprB, OPR_B_LENG*sizeof(uint32_t));
		PKA_CRYPTO_DUMP("Res" , Res ,  (HRessiz)*sizeof(uint32_t));
		cc312_view_status();

		{
			mbedtls_mpi X, A, B;

			PKA_CRYPTO_PRINTF("\nSW.MUL [oprA %d B * oprB %d B] ==========\n", (i*4), (OPR_B_LENG*4));

			mbedtls_mpi_init( &X );
			mbedtls_mpi_init( &A );
			mbedtls_mpi_init( &B );

			mbedtls_mpi_reverse_binary( &A, (const unsigned char *)oprA, i*sizeof(uint32_t));
			mbedtls_mpi_reverse_binary( &B, (const unsigned char *)oprB, (OPR_B_LENG*sizeof(uint32_t)));

			mbedtls_mpi_mul_mpi( &X, &A, &B );

			PKA_CRYPTO_DUMP("oprA", A.p, A.n*sizeof(mbedtls_mpi_uint));
			PKA_CRYPTO_DUMP("oprB", B.p, B.n*sizeof(mbedtls_mpi_uint));
			PKA_CRYPTO_DUMP("Result X", X.p, X.n*sizeof(mbedtls_mpi_uint));

			mbedtls_mpi_free( &X );
			mbedtls_mpi_free( &A );
			mbedtls_mpi_free( &B );
		}

		PkaFinishAndMutexUnlock(pkaReqRegs);
	}

Failed:
	//CC_HAL_WRITE_REGISTER(CC_REG_OFFSET(CRY_KERNEL, PKA_SW_RESET), 0x01);
	PKA_WAIT_ON_PKA_PIPE_READY();
	PKA_WAIT_ON_PKA_DONE();

	CRYPTO_FREE(N);
	CRYPTO_FREE(NP);
	CRYPTO_FREE(oprA);
	CRYPTO_FREE(oprB);
	CRYPTO_FREE(oprC);
	CRYPTO_FREE(Res);

	return status;
}


/******************************************************************************
 *  mbedtls_platform_init( )
 *
 *  Purpose : sub-functions
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static UINT32 mbedtls_platform_flag ;

static int crypto_snprintf(char *buf, size_t n, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	n = da16x_vsnprintf(buf, n, 0, fmt, args);
	va_end(args);

	return n;
}

static int	crypto_print(const char *fmt,...)
{
#ifdef BUILD_OPT_VSIM
	int len;
	static char uart_lowprint_buf[CON_LBUF_SIZ];
	va_list ap;
	va_start(ap,fmt);

	len = da16x_vsnprintf(uart_lowprint_buf,CON_LBUF_SIZ, 0, fmt,ap);

	da16x_sysdbg_dbgt(uart_lowprint_buf, len);

	va_end(ap);
#else	//BUILD_OPT_VSIM
	va_list ap;
	va_start(ap,fmt);

	embcrypto_vprint(0, fmt, ap);

	va_end(ap);
#endif	//BUILD_OPT_VSIM

	return TRUE;
}

#define CRYPTO_ALGIN_SIZE	(sizeof(UINT32)-1)

static 	void 	*crypto_calloc(size_t n, size_t size)
{
	void *retbuf = NULL;
	int  allocsiz ;

	allocsiz = (( n * size ) + CRYPTO_ALGIN_SIZE) & (~CRYPTO_ALGIN_SIZE);

	retbuf = CRYPTO_MALLOC(allocsiz);

	return retbuf;
}

static 	void 	crypto_free(void *f)
{
	if( f == NULL ){
		return;
	}
	if(0){
		UINT32 fsize;
		VOID *work_ptr;
		/* TX specific info */
		work_ptr = (VOID *)f;
		/* Back off the memory pointer to pickup its header.  */
	        work_ptr = (VOID *)((UINT32)work_ptr - (sizeof(UCHAR *) + sizeof(ULONG)));

		fsize = (UINT32)(*((UCHAR **) ((VOID *) work_ptr)) - (UCHAR *)work_ptr);
		fsize -= 8;
		Printf("%p - free %d\r\n", f, fsize );
	}

	CRYPTO_FREE(f);
}

static void crypto_exit( int status )
{
	CRYPTO_PRINTF("<Crypto.exit %d>\n", status);
}

static mbedtls_time_t crypto_time( mbedtls_time_t* timer )
{
#ifdef  __TIME64__
	mbedtls_time_t t;
	da16x_time64(timer, &t);
	return(t);
#else
	mbedtls_time_t t = time(timer);
	return t;
#endif  /* __TIME64__ */
}

static mbedtls_tm_t *crypto_gmtime( mbedtls_time_t* timer )
{
	mbedtls_tm_t *tm2;
	tm2 = gmtime(timer);
	return tm2;
}

static int crypto_nv_seed_read( unsigned char *buf, size_t buf_len )
{
	int i , alen;

	alen = buf_len & (~0x03);
	for( i = 0; i < alen; i += sizeof(UINT32) ){
		*((UINT32 *)&(buf[i])) = da16x_random( );
	}

	return( buf_len );
}

static int crypto_nv_seed_write( unsigned char *buf, size_t buf_len )
{
	unsigned int i ;
	UINT32 seed;

	for( i = 0; i < buf_len; i += sizeof(UINT32) ){
		//da16x_random_seed( *((UINT32 *)&(buf[i])) );
		seed = *((UINT32 *)&(buf[i]));
		seed = seed ^ da16x_random( );
		da16x_random_seed( seed );
	}
	return( buf_len );
}

static void crypto_mutex_init( mbedtls_threading_mutex_t *mutex )
{
	if( mutex == NULL ){
		CRYPTO_PRINTF( "%s:null\n", __func__);
		return;
	}

	mutex->mutex = xSemaphoreCreateMutex();
	if (mutex->mutex == NULL) {
		return ;
	}
}

static void crypto_mutex_free( mbedtls_threading_mutex_t *mutex )
{
	if( mutex == NULL ){
		CRYPTO_PRINTF( "%s:null\n", __func__);
		return;
	}

	if (mutex->mutex) {
		vSemaphoreDelete(mutex->mutex);
	}
	mutex->mutex = NULL;
}

static int crypto_mutex_lock( mbedtls_threading_mutex_t *mutex )
{
	if( mutex == NULL ){
		CRYPTO_PRINTF( "%s:null\n", __func__);
		return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );
	}

	if (xSemaphoreTake(mutex->mutex, portMAX_DELAY) != pdTRUE) {
		return( MBEDTLS_ERR_THREADING_MUTEX_ERROR );
	}

	return( 0 );
}

static int crypto_mutex_unlock( mbedtls_threading_mutex_t *mutex )
{
	if( mutex == NULL ){
		CRYPTO_PRINTF( "%s:null\n", __func__);
		return( MBEDTLS_ERR_THREADING_BAD_INPUT_DATA );
	}

	if (xSemaphoreGive(mutex->mutex) != pdTRUE) {
		return( MBEDTLS_ERR_THREADING_MUTEX_ERROR );
	}

	return( 0 );
}

/******************************************************************************
 *  mbedtls_platform_init( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32 mbedtls_platform_init(const mbedtls_net_primitive_type *mbedtls_platform)
{
	UINT32 intrbkup;

	if( mbedtls_platform_flag  == 0){
		CHECK_INTERRUPTS(intrbkup);
		PREVENT_INTERRUPTS(1);

		mbedtls_platform_set_calloc_free( &crypto_calloc, &crypto_free );
		mbedtls_platform_set_snprintf( &crypto_snprintf );
		mbedtls_platform_set_printf( &crypto_print );

		mbedtls_platform_set_exit( &crypto_exit );
		mbedtls_platform_set_time( &crypto_time );
		mbedtls_platform_set_gmtime( &crypto_gmtime );
#ifdef	MBEDTLS_PLATFORM_NV_SEED_ALT
		mbedtls_platform_set_nv_seed( &crypto_nv_seed_read, &crypto_nv_seed_write );
#endif //MBEDTLS_PLATFORM_NV_SEED_ALT
		//mbedtls_platform_setup();
		//mbedtls_platform_teardown();
		mbedtls_threading_set_alt(
			&crypto_mutex_init, &crypto_mutex_free,
			&crypto_mutex_lock, &crypto_mutex_unlock
		);

		if( mbedtls_platform != NULL ){
			mbedtls_platform_net_sockets( mbedtls_platform );
			mbedtls_platform_flag = 2; // w/ socket
		}else{
			mbedtls_platform_flag = 1; // w/o socket
		}

		PREVENT_INTERRUPTS(intrbkup);
	}

	return mbedtls_platform_flag;
}

UINT32 mbedtls_platform_bind(const mbedtls_net_primitive_type *mbedtls_platform)
{
	if( mbedtls_platform_flag == 1 ){
		UINT32 intrbkup;

		CHECK_INTERRUPTS(intrbkup);
		PREVENT_INTERRUPTS(1);

		mbedtls_platform_net_sockets( mbedtls_platform );
		mbedtls_platform_flag = 2; // w/ socket

		PREVENT_INTERRUPTS(intrbkup);
	}

	return mbedtls_platform_flag;
}

/******************************************************************************
 *  DA16X_CryptoClkGatingEnable( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	DA16X_SetCryptoCoreClkGating(UINT32 mode)
{
	if( mode == TRUE ){
		CC_HAL_WRITE_REGISTER(CC_REG_OFFSET(CRY_KERNEL, HOST_CORE_CLK_GATING_ENABLE), 0x01);
	}else{
		CC_HAL_WRITE_REGISTER(CC_REG_OFFSET(CRY_KERNEL, HOST_CORE_CLK_GATING_ENABLE), 0x00);
	}
}

/* EOF */
