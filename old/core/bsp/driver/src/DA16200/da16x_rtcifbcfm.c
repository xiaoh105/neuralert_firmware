/**
 ****************************************************************************************
 *
 * @file da16x_rtcifbcfm.c.c
 *
 * @brief RTCIFBCFM Controller
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
#include <stdarg.h>

#include "hal.h"
#include "driver.h"

#if 0//(dg_configSYSTEMVIEW == 1)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define traceISR_ENTER()
#  define traceISR_EXIT()
#endif /* (dg_configSYSTEMVIEW == 1) */

//---------------------------------------------------------
// RTCIFBCFM
//---------------------------------------------------------

typedef		struct	_da16200_rtcif_bcfm_	DA16X_RTCIFBCFM_TypeDef;

struct		_da16200_rtcif_bcfm_
{
	volatile UINT32	RTC_REQ;
	volatile UINT32	RTC_EN;
	volatile UINT32	RTC_IF;
	volatile UINT32	RTC_IRQ_EN;
	volatile UINT32	RTC_CLK_INV;
	volatile UINT32	RTC_CLK_GR_CYC;
	volatile UINT32	_reserved0[2];

	volatile UINT32	RTC_IRQ_STATUS;
	volatile UINT32	RTC_MR_8040;
	volatile UINT32	RTC_MR_FRC_low;
	volatile UINT32	RTC_MR_FRC_high;
	volatile UINT32	RTC_MR_FRC_stat;
	volatile UINT32	_reserved1[3];

	volatile UINT32	BCFM_REQ;
	volatile UINT32	BCFM_EN;
	volatile UINT32	BCFM_TOT_CYC;
	volatile UINT32	BCFM_IRQ_EN;
	volatile UINT32	BCFM_IRQ_STATUS;
	volatile UINT32	BCFM_OP_STATUS;
	volatile UINT32	_reserved2[2];

	volatile UINT32 RTC_EXP_REQ;
	volatile UINT32 RTC_EXP_IRQ_EN;
	volatile UINT64 RTC_EXP_TH;
	volatile UINT32 RTC_EXP_Status;
};

#define DA16X_RTCIFBCFM	((DA16X_RTCIFBCFM_TypeDef *)RTCIF_BASE_0)


#define	RTC_REQ_CLR		(1<<0)
#define	RTC_REQ_CLR_MR		(1<<1)
#define	RTC_REQ_CLR_IRQ		(1<<2)
#define	RTC_REQ_LOAD_MR		(1<<3)

#define	RTC_OP_EN		(1<<0)
#define	RTC_MR_EN		(1<<1)

#define	RTC_IF_0_DL(x)		(((x)&0x0f)<<0)
#define	RTC_IF_1_DL(x)		(((x)&0x0f)<<4)
#define	RTC_IF_0_GL(x)		(((x)&0x0f)<<8)
#define	RTC_IF_1_GL(x)		(((x)&0x3f)<<8)

#define	RTC_CLK_GR_CYC(x)	(((x)&0x0f)<<0)

#define	RTC_FRC_VALID		(1<<2)

// RTCIF

#define	RTC_IRQ_ENABLE		(1<<0)

#define	RTC_MR_SYNCDONE		0x01
#define	RTC_MR_SYNCENABLE	0x08

// BCFM

#define	BCFM_OP_EN		(1<<0)

#define	BCFM_REQ_START		(1<<0)
#define	BCFM_REQ_CLR_IRQ	(1<<1)

#define BCFM_IRQ_ENABLE		(1<<0)

// RTC_EXP

#define RTC_EXP_CLR_IRQ		(1<<1)
#define RTC_EXP_START		(1<<0)

#define RTC_EXP_IRQ_EXPIRE	(1<<0)
#define RTC_EXP_IRQ_TARND	(1<<1)
#define RTC_EXP_IRQ_ABNORMAL	(1<<2)

/******************************************************************************
 *  da16x_rtc_init_mirror ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void	da16x_rtcm_interrupt(void);

void da16x_rtc_init_mirror(int mode)
{
	// if mode == TRUE, then Re-Sync
	DA16X_RTCIFBCFM->RTC_REQ = RTC_REQ_CLR_IRQ;

	if ( mode == TRUE ) {
		_sys_nvic_write(RTC_LMR_IRQn, (void *)da16x_rtcm_interrupt, 0x7);
	}

	if( (DA16X_RTCIFBCFM->RTC_REQ & RTC_REQ_LOAD_MR) == 0 ){
		int i;

		DA16X_RTCIFBCFM->RTC_IRQ_EN = RTC_IRQ_ENABLE;

		DA16X_RTCIFBCFM->RTC_EN = (RTC_MR_EN|RTC_OP_EN);
		DA16X_RTCIFBCFM->RTC_REQ = RTC_REQ_LOAD_MR;

		i = ( mode == TRUE ) ? 0 : 32 ;

		for( ; i < 32; i++ ){
			WAIT_FOR_INTR();
			if( (DA16X_RTCIFBCFM->RTC_IRQ_STATUS & RTC_MR_SYNCDONE) == RTC_MR_SYNCDONE ){
				break;
			}
		}
	}
}

void _lowlevel_da16x_rtcm_interrupt(void)
{
//	traceISR_ENTER();
//	ASIC_DBG_TRIGGER(0xFC910C1F);
//	traceISR_EXIT();
}

static void	da16x_rtcm_interrupt(void)
{
	_lowlevel_da16x_rtcm_interrupt();
	//INTR_CNTXT_CALL(_lowlevel_da16x_rtcm_interrupt);
}

/******************************************************************************
 *  da16x_rtc_get_mirror ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT64 da16x_rtc_get_mirror(void)
{
	UINT64 time_temp = 0;
	UINT32 *ptemp = (UINT32*)&time_temp;
	UINT32 intrbkup = 0;

	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);
	if( (DA16X_RTCIFBCFM->RTC_IRQ_STATUS & RTC_MR_SYNCDONE) == RTC_MR_SYNCDONE ){
		ptemp[0] = DA16X_RTCIFBCFM->RTC_MR_FRC_low;
		ptemp[1] = DA16X_RTCIFBCFM->RTC_MR_FRC_high;
	}else{
		ptemp[0] = 0xFFFFFFFF;
		ptemp[1] = 0xFFFFFFFF;
	}
	PREVENT_INTERRUPTS(intrbkup);

	return time_temp;
}

UINT32 da16x_rtc_get_mirror32(int mode)
{
	if( (DA16X_RTCIFBCFM->RTC_IRQ_STATUS & RTC_MR_SYNCDONE) == RTC_MR_SYNCDONE ){
		if( mode == FALSE ){
			return DA16X_RTCIFBCFM->RTC_MR_FRC_low;
		}else{
			return DA16X_RTCIFBCFM->RTC_MR_FRC_high;
		}
	}
	return 0xFFFFFFFF;
}

/******************************************************************************
 *  da16x_bcf_measure_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static void	da16x_bcf_interrupt(void);

void da16x_bcf_measure_init(void)
{
	_sys_nvic_write(BCF_MSR_IRQn	, (void *)da16x_bcf_interrupt ,0x7);
}

/******************************************************************************
 *  da16x_bcf_measure ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT64 da16x_bcf_measure(UINT32 x32k_cycle)
{
	UINT32 retry, bkuprtcm;
	UINT64 time_temp;
	UINT32 *ptemp = (UINT32*)&time_temp;
	UINT32 intrbkup = 0;


	ptemp[0] = 0xFFFFFFFF;
	ptemp[1] = 0xFFFFFFFF;

	bkuprtcm = DA16X_RTCIFBCFM->RTC_EN;
	DA16X_RTCIFBCFM->RTC_EN &= ~RTC_MR_EN;
	DA16X_RTCIFBCFM->RTC_REQ = RTC_REQ_CLR_MR;

	DA16X_RTCIFBCFM->BCFM_IRQ_EN = BCFM_IRQ_ENABLE;
	DA16X_RTCIFBCFM->BCFM_TOT_CYC = x32k_cycle;

	DA16X_RTCIFBCFM->BCFM_EN = BCFM_OP_EN;
	DA16X_RTCIFBCFM->BCFM_REQ = BCFM_REQ_START | BCFM_REQ_CLR_IRQ;

	for(retry = 64; retry > 0 ; retry-- ){
		WAIT_FOR_INTR();
		if( DA16X_RTCIFBCFM->BCFM_IRQ_STATUS != 0 ){
			CHECK_INTERRUPTS(intrbkup);
			PREVENT_INTERRUPTS(1);
			ptemp[1] = DA16X_RTCIFBCFM->RTC_MR_FRC_high;
			ptemp[0] = DA16X_RTCIFBCFM->RTC_MR_FRC_low;
			PREVENT_INTERRUPTS(intrbkup);
			break;
		}
	}

	DA16X_RTCIFBCFM->RTC_EN = bkuprtcm ;
	DA16X_RTCIFBCFM->RTC_REQ = RTC_REQ_CLR_MR ;

	return time_temp;
}

void _lowlevel_da16x_bcf_interrupt(void)
{
//	traceISR_ENTER();
//	traceISR_EXIT();
}

static void	da16x_bcf_interrupt(void)
{
	_lowlevel_da16x_bcf_interrupt();
	//INTR_CNTXT_CALL(_lowlevel_da16x_bcf_interrupt);
}

/******************************************************************************
 *  rtc_exp_timer_init ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

static ISR_CALLBACK_TYPE *rtc_exp_callback;

void _lowlevel_da16x_rtcexp_interrupt(void)
{
	if( (rtc_exp_callback != NULL) && (rtc_exp_callback->func != NULL) ){
		rtc_exp_callback->func( rtc_exp_callback->param );
	}
}

static void	da16x_rtcexp_interrupt(void)
{
	traceISR_ENTER();
	INTR_CNTXT_CALL(_lowlevel_da16x_rtcexp_interrupt);
	traceISR_EXIT();
}

void rtc_exp_timer_init(void)
{
	_sys_nvic_write(RTC_EXP_Timer_IRQn, (void *)da16x_rtcexp_interrupt, 0x7);
}

/******************************************************************************
 *  registry_rtc_exp_timer ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	registry_rtc_exp_timer(UINT64 exptime, VOID *isrcallback)
{
	UINT32  intrbkup = 0;

	if( (rtc_exp_callback != NULL)
	   || ((DA16X_RTCIFBCFM->RTC_EXP_Status & RTC_EXP_STATUS_RUN) != 0 ) ){
		return FALSE; // running now !!
	}

	rtc_exp_callback = isrcallback;

	DA16X_RTCIFBCFM->RTC_EXP_IRQ_EN
		= RTC_EXP_IRQ_EXPIRE|RTC_EXP_IRQ_TARND|RTC_EXP_IRQ_ABNORMAL ;

	CHECK_INTERRUPTS(intrbkup);
	PREVENT_INTERRUPTS(1);
	DA16X_RTCIFBCFM->RTC_EXP_TH  = exptime;
	DA16X_RTCIFBCFM->RTC_EXP_REQ = RTC_EXP_CLR_IRQ|RTC_EXP_START;
	PREVENT_INTERRUPTS(intrbkup);

	return TRUE;
}

/******************************************************************************
 *  clear_rtc_exp_status ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

UINT32	clear_rtc_exp_status(UINT32 mode)
{
	UINT32 dumpreg;

	//dumpreg = DA16X_RTCIFBCFM->RTC_EXP_Status ;

	if( mode == TRUE ){
		DA16X_RTCIFBCFM->RTC_EXP_REQ = RTC_EXP_CLR_IRQ; // Clear & Disable
		rtc_exp_callback = NULL; // unregistry
	}

	dumpreg = DA16X_RTCIFBCFM->RTC_EXP_Status ;	// 위치 변경
	return dumpreg;
}

/******************************************************************************
 *  easy wrapper functions
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
HANDLE	MCNT_CREATE(void)
{
	RTC_MCNT_TYPE *mcnt = NULL;
	mcnt = (HANDLE)pvPortMalloc(sizeof(RTC_MCNT_TYPE));
	if (mcnt == NULL)
		return NULL;

	rtc_exp_timer_init();

	return mcnt;
}
int		MCNT_START(HANDLE handler, UINT64 count, ISR_CALLBACK_TYPE *callback)
{
	RTC_MCNT_TYPE *mcnt = (RTC_MCNT_TYPE*)handler;
	if (mcnt == NULL)
		return FALSE;
	mcnt->set_time = count;
	clear_rtc_exp_status(TRUE);
	return registry_rtc_exp_timer(count + RTC_GET_COUNTER(), callback);
}
int 	MCNT_STOP(HANDLE handler, UINT64 *remain)
{
	INT64 temp;
	RTC_MCNT_TYPE *mcnt = (RTC_MCNT_TYPE*)handler;
	*remain = 0;
	if (mcnt == NULL)
		return 2;			// clear_rtc_exp_status의 값은 0또는 1의 값을 가짐
	mcnt->set_time = 0;
	if (clear_rtc_exp_status(TRUE) == 0)
	{
		temp = (UINT64)DA16X_RTCIFBCFM->RTC_EXP_TH - (UINT64)RTC_GET_COUNTER();
		if ( temp > 0)
		{
			*remain = temp;
			return 1;	// TRUE
		}
	}
	return 0;	// FALSE

}
void		MCNT_CLOSE(HANDLE handler)
{
	RTC_MCNT_TYPE *mcnt = (RTC_MCNT_TYPE*)handler;
	if (mcnt != NULL)
		vPortFree(mcnt);
}
