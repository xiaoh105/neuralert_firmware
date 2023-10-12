/**
 ****************************************************************************************
 *
 * @file rtc.c
 *
 * @brief RTC Driver
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

#include "hal.h"
#include "da16x_compile_opt.h"

//---------------------------------------------------------
//	this driver is used only for a evaluation.
//---------------------------------------------------------

// struct		_da16x_rtcif_bcfm_
// {
// 	volatile UINT32	RTC_REQ;
// 	volatile UINT32	RTC_EN;
// 	volatile UINT32	RTC_IF;
// 	volatile UINT32	RTC_IRQ_EN;
// 	volatile UINT32	RTC_CLK_INV;
// 	volatile UINT32	RTC_CLK_GR_CYC;
// 	volatile UINT32	_reserved0[2];
//
// 	volatile UINT32	RTC_IRQ_STATUS;
// 	volatile UINT32	RTC_MR_8040;
// 	volatile UINT32	RTC_MR_FRC_low;
// 	volatile UINT32	RTC_MR_FRC_high;
// 	volatile UINT32	RTC_MR_FRC_stat;
// 	volatile UINT32	_reserved1[3];
//
// 	volatile UINT32	BCFM_REQ;
// 	volatile UINT32	BCFM_EN;
// 	volatile UINT32	BCFM_TOT_CYC;
// 	volatile UINT32	BCFM_IRQ_EN;
// 	volatile UINT32	BCFM_IRQ_STATUS;
// 	volatile UINT32	BCFM_OP_STATUS;
// };
//
// #define DA16X_RTCIFBCFM	((DA16X_RTCIFBCFM_TypeDef *)RTCIF_BASE_0)

#define SYS_RTC_BASE		((RTC_REG_MAP*)RTC_BASE_0)

#define BB_POWER_MANAGE			    (DA16200_SYSPERI_BASE | 0x00001300)
#define BB_RTC_IF_MIRROR_EN		    (RTCIF_BASE_0 | 0x00000004)
#define BB_RTC_IF_MIRROR_SYNC		(RTCIF_BASE_0 | 0x00000000)
#define BB_RTC_IF_MIRROR_SYNC_DONE	(RTCIF_BASE_0 | 0x00000020)
#define BB_RTC_IF_MIRROR_COUNTER0	(RTCIF_BASE_0 | 0x00000028)
#define BB_RTC_IF_MIRROR_COUNTER1	(RTCIF_BASE_0 | 0x0000002c)


#undef	RTC_DEBUG
#ifdef RTC_DEBUG
#define RTC_PRINTF(...)		DRV_PRINTF(__VA_ARGS__)
#else
#define	RTC_PRINTF(...)
#endif

#define SYS_RTC_BASE_0		((RTC_REG_MAP*)RTC_BASE_0)

#ifndef HW_REG_AND_WRITE32
#define	HW_REG_AND_WRITE32(x, v)	( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define	HW_REG_OR_WRITE32(x, v)		( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v) 		( *((volatile unsigned int   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define	HW_REG_READ32(x)		( *((volatile unsigned int   *) &(x)) )
#endif

/*
*	static variable
*/
const RTC_HANDLER	rtc = {
	.dev_addr = (UINT32)SYS_RTC_BASE_0,
	.regmap = (RTC_REG_MAP *)(SYS_RTC_BASE_0),
};

static unsigned long long	wakeup_period;
static UINT32			    retention_bank;
static UINT32				wakeup_gpio;
static UINT8 				retention_slp_voltage;
static UINT8				xtal32k_gm;
static UINT8				osc32k_ictrl;
static UINT8				xtal32k_ictrl;
static UINT8				uldo_lictrl;
static UINT8				uldo_hictrl;

/*
*	extern functions
*/
UINT32	RTC_GET_EXT_WAKEUP_SOURCE(void)
{
	UINT32 regtemp = 0;
	regtemp = (HW_REG_READ32(rtc.regmap->gpio_wakeup_control) & 0x3c00) >> 10;
	return regtemp;
}

UINT32	RTC_GET_AUX_WAKEUP_SOURCE(void)
{
	UINT32 regtemp = 0;
	regtemp = HW_REG_READ32(rtc.regmap->wakeup_source) & 0x70;
	return regtemp;

}
UINT32	RTC_GET_GPIO_SOURCE(void)
{
	return wakeup_gpio;
}

WAKEUP_SOURCE RTC_GET_WAKEUP_SOURCE(void)
{
	UINT32 source;
	source = HW_REG_READ32(rtc.regmap->wakeup_source) & 0x7f;
	//source |= ((HW_REG_READ32(rtc.regmap->retention_control) & RETENTION_FLAG(1)) << 6);// retention flag위치가 0x02 그래서 6만 shift함

	if ( source & WAKEUP_SOURCE_EXT_SIGNAL)
		HW_REG_AND_WRITE32(rtc.regmap->dc_power_control, ~(SET_1_2_POWER_OFF(1)));

	if ( source & WAKEUP_SOURCE_WAKEUP_COUNTER)
		HW_REG_AND_WRITE32(rtc.regmap->rtc_control, ~(WAKEUP_COUNTER_ENABLE(1)));

	if ( source & WAKEUP_WATCHDOG)
		HW_REG_AND_WRITE32(rtc.regmap->rtc_control, ~(WATCHDOG_ENABLE(1)));

	if ( source & WAKEUP_GPIO )
	{
		// read interrupt status
		wakeup_gpio = HW_REG_READ32(rtc.regmap->gpio_wakeup_control) & 0x3ff;
	}

	if (source & 0x60)
	{
		// pluse 와 gpio는 sensor와 동일하게 취급한다.
		source = (source & ~(0x60)) | WAKEUP_SENSOR;
	}

	/* 시뮬레이션 상에서 por과 다른 시그널이 같이 발생되는 경우를 방어함 */
	if (source & 0x04)
		return WAKEUP_SOURCE_POR;

	source |= ((HW_REG_READ32(rtc.regmap->retention_control) & RETENTION_FLAG(1)) << 6);// retention flag위치가 0x02 그래서 6만 shift함

	return (WAKEUP_SOURCE)source;
}

unsigned long RTC_GET_WAKEUP_PERIOD(void)
{
	return wakeup_period;
}

int RTC_IOCTL(UINT32 cmd , VOID *data )
{
	switch(cmd)
	{
		case RTC_GET_DEVREG:
			*((UINT32 *)data) = rtc.dev_addr;
		break;
		case RTC_SET_RTC_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->rtc_control, *((UINT32 *)data));
		break;
		case RTC_GET_RTC_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->rtc_control);
		break;
		case RTC_SET_XTAL_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->xtal_control, *((UINT32 *)data));
		break;
		case RTC_GET_XTAL_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->xtal_control);
		break;
		case RTC_SET_RETENTION_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->retention_control, *((UINT32 *)data));
		break;
		case RTC_GET_RETENTION_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->retention_control);
		break;

		case RTC_SET_DC_PWR_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->dc_power_control, *((UINT32 *)data));
		break;
		case RTC_GET_DC_PWR_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->dc_power_control);
		break;

		case RTC_SET_LDO_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->ldo_control, *((UINT32 *)data));
		break;
		case RTC_GET_LDO_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->ldo_control);
		break;

		case RTC_GET_SIGNALS_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->select_signals);
		break;

		case RTC_SET_WAKEUP_SOURCE_REG:
			HW_REG_WRITE32(rtc.regmap->wakeup_source, *((UINT32 *)data));
		break;
		case RTC_GET_WAKEUP_SOURCE_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->wakeup_source);
		break;
#if 0
		case RTC_SET_TIME_SCALE_SEL_REG:
			HW_REG_WRITE32(rtc.regmap->time_scale_sel, *((UINT32 *)data));
		break;
		case RTC_GET_TIME_SCALE_SEL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->time_scale_sel);
		break;
#endif

		case RTC_SET_TIME_DELEY_EN_REG:
			HW_REG_WRITE32(rtc.regmap->time_deley_en, *((UINT32 *)data));
		break;
		case RTC_GET_TIME_DELEY_EN_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->time_deley_en);
		break;

		case RTC_SET_TIME_DELEY_CNT_REG:
			HW_REG_WRITE32(rtc.regmap->time_delay_value, *((UINT32 *)data));
		break;
		case RTC_GET_TIME_DELEY_CNT_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->time_delay_value);
		break;

		case RTC_GET_LDO_STATUS:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->ldo_status);
		break;
		case RTC_SET_LDO_LEVEL_REG:
			HW_REG_WRITE32(rtc.regmap->ldo_pwr_control, *((UINT32 *)data));
		break;
		case RTC_GET_LDO_LEVEL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->ldo_pwr_control);
		break;

		case RTC_SET_BOR_CIRCUIT:
			HW_REG_WRITE32(rtc.regmap->bor_circuit, *((UINT32 *)data));
		break;
		case RTC_GET_BOR_CIRCUIT:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->bor_circuit);
		break;

		case RTC_GET_WAKEUP_COUNTER_REG:
			{
				unsigned long long time_temp;
				UINT32 *ptemp = (UINT32*)&time_temp;
				UINT32 temp1, temp2;
				// read counter
				temp2 = rtc.regmap->wakeup_counter0;
				temp1 = rtc.regmap->wakeup_counter1;

				memcpy(ptemp, &temp2, 4);
				memcpy(ptemp+1, &temp1, 4);
				*((unsigned long long *)data) = time_temp;
			}
		break;
		case RTC_GET_COUNTER_REG:
			*((unsigned long long *)data) = RTC_GET_COUNTER();
		break;

		case RTC_GET_WAKEUP_SOURCE_SIG_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->wakeup_source_read);
		break;

		case RTC_SET_GPIO_WAKEUP_CONFIG_REG:
			HW_REG_WRITE32(rtc.regmap->gpio_wakeup_config, *((UINT32 *)data));
		break;

		case RTC_GET_GPIO_WAKEUP_CONFIG_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->gpio_wakeup_config);
		break;

		case RTC_SET_GPIO_WAKEUP_CONTROL_REG:
			HW_REG_WRITE32(rtc.regmap->gpio_wakeup_control, *((UINT32 *)data));
		break;

		case RTC_GET_GPIO_WAKEUP_CONTROL_REG:
			*((UINT32 *)data) = HW_REG_READ32(rtc.regmap->gpio_wakeup_control);
		break;

		default :
			return -1;
	}
	return 0;

}

void RTC_READY_POWER_DOWN(int clear)
{
	UINT32 regtemp = 0x3e;
	// external wakeup signal만 예외로 한다, external wakeup 이 발생하였을 때에는 수동으로 clear 한다.
	if (clear)
	{
		HW_REG_WRITE32(rtc.regmap->wakeup_source, regtemp);
	}
	HW_REG_AND_WRITE32(rtc.regmap->rtc_control, ~( WAKEUP_COUNTER_ENABLE(1) | WATCHDOG_ENABLE(1) ));
	HW_REG_AND_WRITE32(rtc.regmap->dc_power_control, ~(SET_1_2_POWER_OFF(1)));

	// AO restore clear
	HW_REG_AND_WRITE32(rtc.regmap->time_deley_en, ~(AO_REGISTER_RESTORE_ENABLE(1)));
}

/* external wakeup signal로 wakeup이 되었을 경우 반드시 아래 func를 호출하여 flag를 clear 한다.*/
void RTC_CLEAR_EXT_SIGNAL( void )
{
	UINT32 regtemp;
	// external wakeup signal만 예외로 한다, external wakeup 이 발생하였을 때에는 수동으로 clear 한다.
	regtemp = HW_REG_READ32(rtc.regmap->wakeup_source);
	regtemp = (regtemp) | 0x41; 	// gpio interrupt 도 같이 처리해 준다
	HW_REG_WRITE32(rtc.regmap->wakeup_source, regtemp);

	//gpio interrupt 도 같이 처리해 준다// fc9050 slr에서 0x28 clear로 모두 clear 되弱途?함
	//HW_REG_OR_WRITE32(rtc.regmap->gpio_wakeup_control, wakeup_gpio);
}

void RTC_INTO_POWER_DOWN( UINT32 flag, unsigned long long period )
{
	unsigned long long time;

	time = RTC_GET_COUNTER() + period;

	wakeup_period = period;

	HW_REG_WRITE32(rtc.regmap->wakeup_source, 0x00);
	HW_REG_AND_WRITE32(rtc.regmap->retention_control, 0xffffff0f);
	HW_REG_OR_WRITE32(rtc.regmap->retention_control, ((flag << 4) & 0xf0));

	/* whether retention open or off */
	if (RTC_GET_RETENTION_FLAG() == 1)
	{
		RTC_RETENTION_CLOSE();
		// AO restore set
		HW_REG_OR_WRITE32(rtc.regmap->time_deley_en, (AO_REGISTER_RESTORE_ENABLE(1)));
	}

	HW_REG_WRITE32(rtc.regmap->wakeup_counter1, (UINT32)(time >> 32));
	HW_REG_WRITE32(rtc.regmap->wakeup_counter0, (UINT32)time);
	HW_REG_OR_WRITE32(rtc.regmap->rtc_control, WAKEUP_COUNTER_ENABLE(1));
}

void RTC_SET_RETENTION_FLAG(void)
{
	HW_REG_OR_WRITE32(rtc.regmap->retention_control, RETENTION_FLAG(1));
}

int RTC_GET_RETENTION_FLAG(void)
{
	UINT32 temp;
	temp = HW_REG_READ32(rtc.regmap->retention_control);
	if (temp & RETENTION_FLAG(1))
		return 1;
	else
		return 0;
}
void RTC_CLEAR_RETENTION_FLAG(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->retention_control, ~(RETENTION_FLAG(1)));
}

void RTC_RETENTION_PWR_ON(void)
{
	unsigned long long time;
	int retry = 0;
	UINT32 reg;
	// rtc uldo vctrl은 0x01로 uldo hictrl은 0x03으로
	reg = HW_REG_READ32(rtc.regmap->ldo_pwr_control);
	reg &= ~(0xe0fffffc);
	reg |= (RTC_ULDO_VCTRL(0x01) | RTC_ULDO_HICTRL(0x03) | (0x01 << 10) | (0x01 <<12 ) | (0x03<<16) );
	HW_REG_WRITE32(rtc.regmap->ldo_pwr_control, reg );
	HW_REG_OR_WRITE32(rtc.regmap->ldo_control, LDO_ULDO_ENABLE(1));
	// 30us의 안정화 시간이 필요
	time = RTC_GET_COUNTER();
	while( (time + 2) > RTC_GET_COUNTER())
	{
		retry++;
		if(retry > 100)
			break;
	}
}

void RTC_RETENTION_PWR_OFF(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->retention_control,~(PDB_ISOLATION_ENABLE(1)));
	*((volatile unsigned int *)BB_POWER_MANAGE) &= ~(0x00000001);
	HW_REG_AND_WRITE32(rtc.regmap->ldo_control, ~(LDO_ULDO_ENABLE(1)));
}

void RTC_RETENTION_OPEN(void)
{
	HW_REG_OR_WRITE32(rtc.regmap->retention_control,(PDB_ISOLATION_ENABLE(1)));
	HW_REG_AND_WRITE32(rtc.regmap->retention_control,( ~PDB_RETENTION_ENABLE(RETENTION_MEM_BANK_ALL)
													& ~RETENTION_SLEEP(RETENTION_MEM_BANK_ALL) ));
	*((volatile unsigned int *)BB_POWER_MANAGE) |= (0x00000001);
}

void RTC_RETENTION_CLOSE(void)
{
	UINT32 reg;
	HW_REG_AND_WRITE32(rtc.regmap->retention_control,~(PDB_ISOLATION_ENABLE(1)));
	HW_REG_OR_WRITE32(rtc.regmap->retention_control, PDB_RETENTION_ENABLE((~retention_bank))
													| RETENTION_SLEEP(retention_bank)); 	//0x18*/
	*((volatile unsigned int *)BB_POWER_MANAGE) &= ~(0x00000001);
	// rtc uldo vctrl은 0x0c로 uldo hictrl은 0x00으로

	reg = HW_REG_READ32(rtc.regmap->ldo_pwr_control);
	reg &= ~(0xe0fffffc);

	if (retention_slp_voltage)
		reg |= RTC_ULDO_VCTRL(retention_slp_voltage);
	else
		reg |= RTC_ULDO_VCTRL(0x0c);

	if (xtal32k_gm)
		reg |= RTC_XTAL32K_GM(xtal32k_gm);
	else
		reg |= RTC_XTAL32K_GM(0x3);

	if (osc32k_ictrl)
		reg |= RTC_OSC32K_ICTRL(osc32k_ictrl);

	if (xtal32k_ictrl)
		reg |= RTC_XTAL32K_ICTRL(xtal32k_ictrl);
	else
		reg |= RTC_XTAL32K_ICTRL(0x01);

	if (uldo_lictrl)
		reg |= RTC_ULDO_LICTRL(uldo_lictrl);
	else
		reg |= RTC_ULDO_LICTRL(0x01);

	if (uldo_hictrl)
		reg |= RTC_ULDO_HICTRL(uldo_hictrl);

	HW_REG_WRITE32(rtc.regmap->ldo_pwr_control, reg );
}

void RTC_DC12_ON(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->dc_power_control, ~(SET_1_2_POWER_OFF(1)));
}

void RTC_DC12_OFF( void )
{
	HW_REG_WRITE32(rtc.regmap->wakeup_source, 0x00);
	HW_REG_OR_WRITE32(rtc.regmap->dc_power_control, SET_1_2_POWER_OFF(1));
}


unsigned long long RTC_GET_COUNTER(void)
{
#if 1
	unsigned long long time_temp;
	UINT32 *ptemp = (UINT32*)&time_temp;
	UINT32 temp1, temp2;
	// check sync done
	if ((*((volatile UINT32 *)(BB_RTC_IF_MIRROR_SYNC_DONE)))== 0x01)
	{
		temp2 = *((volatile UINT *)BB_RTC_IF_MIRROR_COUNTER0);
		temp1 = *((volatile UINT *)BB_RTC_IF_MIRROR_COUNTER1);

		memcpy(ptemp, &temp2, 4);
		memcpy(ptemp+1, &temp1, 4);
		RTC_PRINTF("Rn");
	}
	else
	{
		// read counter
		temp2 = rtc.regmap->counter0;
		temp1 = rtc.regmap->counter1;
#if 0
		if( (*((volatile UINT *)BB_RTC_IF_MIRROR_SYNC) & 0x08) == 0 ){
			// enable mirroring
			*((volatile UINT *)BB_RTC_IF_MIRROR_EN) = 0x03;
			*((volatile UINT *)BB_RTC_IF_MIRROR_SYNC) = 0x08;
			RTC_PRINTF("MIRRORING ENABLE!!!!!!!!!!!!!!!!!!\n");
		}
#endif

		memcpy(ptemp, &temp2, 4);
		memcpy(ptemp+1, &temp1, 4);
	}
	return  time_temp;
#else
	volatile unsigned long long time_temp;
	volatile UINT32 *ptemp = (UINT32*)&time_temp;

	ptemp[0] = rtc.regmap->counter0;
	ptemp[1] = rtc.regmap->counter1;

	return  time_temp;
#endif
}

UINT32 RTC_GET_XTAL_READY(void)
{
	return HW_REG_READ32(rtc.regmap->ldo_status) & XTAL_READY(1);
}

void RTC_XTAL_ON(void)
{
	HW_REG_OR_WRITE32(rtc.regmap->xtal_control, XTAL_ENABLE(1));
}

void RTC_XTAL_OFF(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->xtal_control, ~XTAL_ENABLE(1));
}

void RTC_OSC_ON(void)
{
	HW_REG_OR_WRITE32(rtc.regmap->xtal_control, OSC_ENABLE(1));
}
void RTC_OSC_OFF(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->xtal_control, ~OSC_ENABLE(1));
}

XTAL_TYPE RTC_GET_CUR_XTAL(void)
{
	UINT32 sel;
	sel = HW_REG_READ32(rtc.regmap->xtal_control);

	if ( (sel&0x0c) == 0x00)
		return INTERNAL_OSC;
	else if ( (sel&0x0c) == 0x04)
		return EXTERNAL_XTAL;
	else
		return XTAL_ERROR;
}

XTAL_TYPE RTC_SWITCH_XTAL(XTAL_TYPE type)
{
	if (RTC_GET_CUR_XTAL() == type)
		return type;

	if (type == INTERNAL_OSC)
	{
		HW_REG_AND_WRITE32(rtc.regmap->xtal_control, ~(0x0c) );
		return type;
	}
	else if (type == EXTERNAL_XTAL)
	{
		if (RTC_GET_XTAL_READY())
		{
			HW_REG_OR_WRITE32(rtc.regmap->xtal_control, 0x04);
			return type;
		}
		else
		{
			return INTERNAL_OSC;
		}
	}
	else
		return XTAL_ERROR;
}
#if 0 // removed
void RTC_SET_AUTOEN_TIME_SCALE(UINT32 scale)
{
	//HW_REG_OR_WRITE32(rtc.regmap->time_scale_sel, LDO_EN_TIME_SCALE(scale));
}

UINT32 RTC_GET_AUTOEN_TIME_SCALE(void)
{
	//return (HW_REG_READ32(rtc.regmap->time_scale_sel) & 0x03);
}
#endif

void RTC_SET_AUTO_LDO(RTC_AUTO_LDO_EN_PARAM param)
{
	HW_REG_WRITE32(rtc.regmap->time_deley_en, (MANUAL_BOOST_TIME_SEL_ENABLE(param.boost_en) | MANUAL_RESET_TIME_SEL_ENABLE(param.core_reset_en) | MANUAL_DCDC_TIME_SEL_ENABLE(param.dcdc_cntl_en)) );
	HW_REG_WRITE32(rtc.regmap->time_delay_value,( SET_BOOST_TIME(param.boost_on_cnt) | SET_1_2_POWER_TIME(param.core_reset_cnt) | SET_DCDC_CNTL_TIME(param.dcdc_cntl_cnt) ));
}

RTC_AUTO_LDO_EN_PARAM RTC_GET_AUTO_LDO(void)
{
	RTC_AUTO_LDO_EN_PARAM ret;
	UINT32 regtemp;

	regtemp = HW_REG_READ32(rtc.regmap->time_delay_value);

	ret.boost_on_cnt = (UINT8)(regtemp >> 8) & 0x1f;
	ret.core_reset_cnt = (UINT8)(regtemp >> 4) & 0x0f;
	ret.dcdc_cntl_cnt = regtemp & 0x0f;

	regtemp = HW_REG_READ32(rtc.regmap->time_deley_en);

	ret.boost_en = (UINT8)((regtemp>> 2)&0x01);
	ret.core_reset_en = (UINT8)((regtemp>> 1)&0x01);
	ret.dcdc_cntl_en = (UINT8)((regtemp & 0x01));

	return ret;
}

UINT32 RTC_SET_RETENTION_BANK(UINT32 bank)
{
	retention_bank = (~bank) & RETENTION_MEM_BANK_ALL;
	return (~retention_bank);
}

UINT32 RTC_GET_RETENTION_BANK(void)
{
	return ((~retention_bank) & RETENTION_MEM_BANK_ALL);
}

UINT32 RTC_SET_RETENTION_SLP_VOLTAGE(UINT32 voltage)
{
	if (voltage == 0)
		return 0x0c;
	retention_slp_voltage = voltage;
	return retention_slp_voltage;
}

UINT32 RTC_GET_RETENTION_SLP_VOLTAGE(void)
{
	if (retention_slp_voltage == 0)
		return 0x0c;
	else
		return retention_slp_voltage;
}

void RTC_SET_RETENTION_SLP_PARAM(RTC_RETENTION_PARAM param)
{
	xtal32k_gm = param.xtal32k_gm;
	osc32k_ictrl = param.osc32k_ictrl;
	xtal32k_ictrl = param.xtal32k_ictrl;
	uldo_lictrl = param.uldo_lictrl;
	uldo_hictrl = param.uldo_hictrl;
}

RTC_RETENTION_PARAM RTC_GET_RETENTION_SLP_PARAM(void)
{
	RTC_RETENTION_PARAM ret;
	ret.xtal32k_gm = xtal32k_gm;
	ret.osc32k_ictrl = osc32k_ictrl;
	ret.xtal32k_ictrl = xtal32k_ictrl;
	ret.uldo_lictrl = uldo_lictrl;
	ret.uldo_hictrl = uldo_hictrl;
	return ret;
}

void RTC_SET_RETENTION_PWR_PARAM(RTC_RETENTION_PARAM param)
{
	UINT32 reg;
	reg = HW_REG_READ32(rtc.regmap->ldo_pwr_control);
	reg &= ~(0xffffff00);

	reg |= RTC_XTAL32K_GM(param.xtal32k_gm);
	reg |= RTC_OSC32K_ICTRL(param.osc32k_ictrl);
	reg |= RTC_XTAL32K_ICTRL(param.xtal32k_ictrl);
	reg |= RTC_ULDO_LICTRL(param.uldo_lictrl);
	reg |= RTC_ULDO_HICTRL(param.uldo_hictrl);

	HW_REG_WRITE32(rtc.regmap->ldo_pwr_control, reg );
}
RTC_RETENTION_PARAM RTC_GET_RETENTION_PWR_PARAM(void)
{
	RTC_RETENTION_PARAM ret;
	UINT32 reg;
	reg = HW_REG_READ32(rtc.regmap->ldo_pwr_control);

	ret.xtal32k_gm = ((reg >> 16) & 0x03);
	ret.osc32k_ictrl = ((reg >> 14) & 0x03);
	ret.xtal32k_ictrl = ((reg >> 12) & 0x03);
	ret.uldo_lictrl = ((reg >> 10) & 0x03);
	ret.uldo_hictrl = ((reg >> 8) & 0x03);
	return ret;
}

UINT32 RTC_SET_WATCHDOG_PERIOD(UINT32 bitpos)
{
	HW_REG_WRITE32(rtc.regmap->watchdog_cnt, bitpos);
	return HW_REG_READ32(rtc.regmap->watchdog_cnt);
}

void RTC_WATCHDOG_EN(void)
{
	HW_REG_WRITE32(rtc.regmap->wakeup_source, 0x00);
	HW_REG_OR_WRITE32(rtc.regmap->rtc_control, WATCHDOG_ENABLE(1));
}

void RTC_WATCHDOG_DIS(void)
{
	HW_REG_AND_WRITE32(rtc.regmap->rtc_control, ~(WATCHDOG_ENABLE(1)));
}

void RTC_COUNTER_CLR(void)
{
	volatile int i = 0;
	HW_REG_OR_WRITE32(rtc.regmap->ao_control, COUNTER_CLEAR(1));
	for (i = 0; i < 100; i++)
	{
		if (HW_REG_READ32(rtc.regmap->counter0) == 0xffffffff)
			break;
	}
	HW_REG_AND_WRITE32(rtc.regmap->ao_control, ~(COUNTER_CLEAR(1)));
}
