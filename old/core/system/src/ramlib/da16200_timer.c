/**
 ****************************************************************************************
 *
 * @file da16200_timer.c
 *
 * @brief Timer for Ram Library
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
#include "rtc.h"
#include "simpletimer.h"
#ifdef THREADX
#include "ramsymbols.h"
#endif
#include "da16x_timer.h"
#include "dpmty_patch.h"
#include "target.h"

//#define   TIMER_DEBUG	1
//#define  RTC_WATCHDOG /* RTC_WATCHDOG causes Launcher fault. Patch me!!! */

//#define US2CLK(us)              ((((volatile unsigned long long ) us) << 9ULL) / 15625ULL)
//#define CLK2US(clk)             ((((volatile unsigned long long )clk) * 15625ULL) >> 9ULL)
/*(((us) << 15) / 1000000)*/
//extern  RAMLIB_SYMBOL_TYPE		*ramsymbols;
static void rtc_mirror_hisr(void *param);

// GPIO retention issue
#define FIXED_TIMER_ERR ((TIMER_ERR) - (2))

#ifdef THREADX
/* IMPORT functions ***********************************************************/
#define MCNT_CREATE(...)			(ramsymbols->MCNT_CREATE)( __VA_ARGS__ )
#define MCNT_START(...)				(ramsymbols->MCNT_START)( __VA_ARGS__ )
#define MCNT_STOP(...)				(ramsymbols->MCNT_STOP)( __VA_ARGS__ )
#define MCNT_CLOSE(...)				(ramsymbols->MCNT_CLOSE)( __VA_ARGS__ )

#define RTC_IOCTL(...)				(ramsymbols->RTC_IOCTL)( __VA_ARGS__ )
#define RTC_INTO_POWER_DOWN(...)	(ramsymbols->RTC_INTO_POWER_DOWN)( __VA_ARGS__ )
#define RTC_GET_WAKEUP_SOURCE(...)	(ramsymbols->RTC_GET_WAKEUP_SOURCE)( __VA_ARGS__ )
#define RTC_READY_POWER_DOWN(...)   (ramsymbols->RTC_READY_POWER_DOWN)( __VA_ARGS__ )
#else

#endif
/* IMPORT functions end********************************************************/


/* retention memory�� �ſ��� �� �����ͷ� ������ �־�� �Ѵ�. */
/* retentions momory value ***************************************************/
//static DA16X_TIMER gtimer[TIMER_ERR];
static volatile DA16X_TIMER *gtimer = (DA16X_TIMER *)(DA16X_RETMEM_BASE + 0x100);
//#define MINIMUM_TIME_SLICE		200
static volatile UINT32 *time_slice = (UINT32*)((DA16X_RETMEM_BASE + 0x100) + (sizeof(DA16X_TIMER) * TIMER_ERR) );
#define MINIMUM_TIME_SLICE		*time_slice
static volatile UINT32 *boot_condition = (UINT32*)((DA16X_RETMEM_BASE + 0x100) + (sizeof(DA16X_TIMER) * TIMER_ERR) + sizeof(UINT32*) );
#define BOOT_TIM		*boot_condition
static volatile UINT32 *CURIDX = (UINT32*)((DA16X_RETMEM_BASE + 0x100) + (sizeof(DA16X_TIMER) * TIMER_ERR) + sizeof(UINT32*) + sizeof(UINT32*));
#define gCurIdx		(*CURIDX)

static volatile unsigned long long *TIME_GAP = (unsigned long long *)TIMER_OFFSET;
#define time_gap  (*TIME_GAP)

static volatile UINT32 *ISR_BLOCK = (UINT32*)(ISR_FLAG);
#define isr_block (*ISR_BLOCK)

static volatile unsigned long long isr_gap;
/* retentions momory value end************************************************/

static volatile HANDLE hTimer;
static volatile ISR_CALLBACK_TYPE rtc_mirror_callback;


#ifndef MEM_LONG_READ
#define MEM_LONG_READ(addr, data)		*data = *((volatile UINT *)addr)
#endif

#ifndef MEM_LONG_WRITE
#define MEM_LONG_WRITE(addr, data)		*((volatile UINT *)addr) = data
#endif

#ifndef MEM_LONG_OR_WRITE32
#define	MEM_LONG_OR_WRITE32(addr, data)		*((volatile UINT *)addr) = *((volatile UINT *)addr) | data
#endif

#ifndef MEM_LONG_AND_WRITE32
#define	MEM_LONG_AND_WRITE32(addr, data)		*((volatile UINT *)addr) = *((volatile UINT *)addr) & data
#endif

#ifndef EXTRACT_BITS
#define EXTRACT_BITS(reg, area, loc) (((reg) >> (loc)) & (area))
#endif

static unsigned long long get_counter(void)
{
	unsigned long long ret;
	RTC_IOCTL(RTC_GET_COUNTER_REG, &ret);
	return ret;
}

static unsigned long long get_remain_counter(void)
{
	volatile unsigned long long remain;
	volatile UINT32 *ptemp = (UINT32*)&remain;

	MEM_LONG_READ(0x50091000, &(ptemp[0]));
	MEM_LONG_READ(0x50091004, &(ptemp[1]));

	if (remain > get_counter())
		return (remain - get_counter());
	else
		return (get_counter() - remain);
}

/* static functions */
/* ���� ���� id�� ������̸� TIMER_ERR�� ���� �ƴϸ� ���� ���� id�� ���� */
static DA16X_TIMER_ID chk_timer_id(DA16X_TIMER_ID id)
{
	if (id >= TIMER_ERR)
		return TIMER_ERR;

	// GPIO retention issue
	if (id == TIMER_9 || id == TIMER_10)
		return TIMER_ERR;

    if (gtimer[id].type.content[DA16X_IDX_USING])
        return TIMER_ERR;
    else
        return id;
}

//#pragma optimize=none
static DA16X_TIMER_ID DA16200_PROCESS_CALLBACK(DA16X_TIMER_ID id)
{
    DA16X_TIMER_ID next = TIMER_ERR, before;
    UINT32 idx = 0;
	volatile unsigned long long temp_gap;
    if (gtimer[id].type.content[DA16X_IDX_USING])
	{

		/* timer�� ������ ���� overflow �� ���̿��ٸ� �ش� ID�� time�� �ٿ��ְ�,
		next ���� ���� ID�� ������ */
		/*call back with param */
		if (gtimer[id].callback.func != NULL)
			gtimer[id].callback.func(gtimer[id].callback.param);
		gtimer[id].type.content[DA16X_IDX_USING] = 0;
		gtimer[id].time = 0;

		/* ���� Ÿ�̸��� ���� ���� 200us ���϶�� callback�� ó���� �ش�.*/
		next = (DA16X_TIMER_ID)gtimer[gCurIdx].type.content[DA16X_IDX_NEXT];

		/* power down ���� wakeup ���� �� �����ϰ� timer_scheduler�� ȣ��ɶ� ������ �ð��� ������ �ش�.*/
#if 0
		if ((next != TIMER_ERR) && (time_gap != 0) )
		{
			if (gtimer[next].time > CLK2US(time_gap))
				gtimer[next].time -= CLK2US(time_gap);
			else
			{
				gtimer[next].time = 0;
			}
		}
#else
		if (time_gap)
		{
		temp_gap = CLK2US(get_remain_counter() + 3); // 100us�� �ð� ������ ��
		before = next;
		while ((next != TIMER_ERR) && (temp_gap > 0))
		{
			if (gtimer[next].time > temp_gap)
			{
				//(ramsymbols->printf)("gtimer[next].time %d\n", gtimer[next].time);
				gtimer[next].time -= temp_gap;
				//(ramsymbols->printf)("time_gap %d\n", (UINT32)temp_gap);
				//(ramsymbols->printf)("0x50091000 %x 0x50091004 %x\n", *((volatile UINT *)0x50091000), *((volatile UINT *)0x50091004));
				//(ramsymbols->printf)("get counter %x\n", (UINT32)get_counter());
				//(ramsymbols->printf)("time_gap %x\n", (UINT32)get_remain_counter());
				//(ramsymbols->printf)("gtimer[next].time %d\n", (UINT32)gtimer[next].time);
				break;
			}
			else
			{
				temp_gap -= gtimer[next].time;
				gtimer[next].time = 0;
				next = (DA16X_TIMER_ID)gtimer[next].type.content[DA16X_IDX_NEXT];
				if(temp_gap < MINIMUM_TIME_SLICE)
					break;
			}
			idx++;

			// GPIO retention issue
			if (idx > FIXED_TIMER_ERR)
				break;
		}
		next = before;
		idx = 0;
		}
#endif
#if 0
		/* tim���� ������ ����� callback�� ó�� ���� �ʴ´�.*/
		if (BOOT_TIM)
			return next;
#endif
		while ((next != TIMER_ERR) && (gtimer[next].time <= MINIMUM_TIME_SLICE))
		{
			if (gtimer[next].callback.func != NULL)
				gtimer[next].callback.func(gtimer[next].callback.param);
			gtimer[next].type.content[DA16X_IDX_USING] = 0;

			next = (DA16X_TIMER_ID)gtimer[next].type.content[DA16X_IDX_NEXT];

			idx++;
			// GPIO retention issue
			if (idx > FIXED_TIMER_ERR)
				break;
		}
	}

    return next;
}

static DA16X_TIMER_ID DA16200_TIMER_ALLOC(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param)
{
    if (chk_timer_id(id) == TIMER_ERR)
        return TIMER_ERR;

    gtimer[id].type.content[DA16X_IDX_USING] = 1;
	gtimer[id].type.content[DA16X_IDX_NEXT] = TIMER_ERR;
	gtimer[id].type.content[DA16X_IDX_POWER_STATUS] = DA16X_POWER_ON;
	gtimer[id].callback.func = (void (*)(void*))(param.callback_func);
    gtimer[id].callback.param = param.callback_param;
    gtimer[id].time = time;

	/* booting offset ���� ���� �����Ǿ�� �ϴ� ���� �ٸ� */
	if (param.booting_offset == NULL)		/*default offset���� ������ */
	{
		if (id == TIMER_0)					// tim boot
		{
			gtimer[id].booting_offset = (void*)CONVERT_BOOT_OFFSET(BOOT_ROM, 0xE800);
		}
		else if (id == TIMER_1)
		{
			gtimer[id].booting_offset = (void*)CONVERT_BOOT_OFFSET(BOOT_ROM, 0xF400);
		}
		else								// full boot
		{
#ifdef BUILD_OPT_DA16200_ASIC
			gtimer[id].booting_offset = (void*)CONVERT_BOOT_OFFSET(BOOT_SFLASH, 0x0000);
#else
			gtimer[id].booting_offset = (void*)CONVERT_BOOT_OFFSET(BOOT_SFLASH, 0x0000);
#endif
		}

	}
	else
	{
		gtimer[id].booting_offset = param.booting_offset;
	}
    return id;
}


static void DA16200_TIMER_ACTIVE(DA16X_TIMER_ID id)
{
	rtc_mirror_callback.func = &rtc_mirror_hisr;
	rtc_mirror_callback.param = (void*)id;
	MCNT_START(hTimer, US2CLK(gtimer[id].time), (ISR_CALLBACK_TYPE *) &rtc_mirror_callback);
}


/* FC9050_TIMER_CALLBACK�� running �� callback ó���� ���� �Լ� */
static void DA16200_TIMER_CALLBACK(void)
{
	DA16X_TIMER_ID next = TIMER_ERR;

    next = DA16200_PROCESS_CALLBACK(gCurIdx);

	if (next != TIMER_ERR)
	{
		gCurIdx = next;
		/* Ÿ�̸� ���� �Ŀ� ����  �ð� ���� */
		gtimer[gCurIdx].time = gtimer[gCurIdx].time - CLK2US((get_counter() -  isr_gap)); // +1 ����
		DA16200_TIMER_ACTIVE(gCurIdx);
	}
	else
	{
		gCurIdx = TIMER_ERR;
	}
}

static void rtc_mirror_hisr(void *param)
{
	DA16X_UNUSED_ARG(param);

	// ISR�� ���� �ϰ� callback ������ ���� ������ ���ο� alloc�� ���� �ʰ� �Ѵ�.
	isr_block = 1;
	isr_gap = get_counter();
#if THREADX
	if (gCurIdx != (UINT32)param)
		(ramsymbols->printf)("mirror callback unknown\n");
#endif
	DA16200_TIMER_CALLBACK();
	isr_block = 0;
}

//#pragma optimize=none
static DA16X_TIMER_ID time_sorting(DA16X_TIMER_ID current_id, DA16X_TIMER_ID new_one)
{
#if 1
	UINT32 idx = 1;
	DA16X_TIMER_ID ret = current_id, before = current_id;
	/*  �� �Լ� �������� timer�� ���� �ִ� */
	/* ����ü�� time�� �� �������� �� �����ؾߵ� ���� ������ �ְ�, */
	/* �߰��� ��� �� ��� �� �ڿ� �ͺ��� ���̰��� ������ �ش�. */
	while(gtimer[current_id].type.content[DA16X_IDX_USING] == 1)
	{
		if (gtimer[current_id].time > gtimer[new_one].time )
		{
			gtimer[current_id].time -= gtimer[new_one].time;
			if (current_id == before) /* ����Ʈ�� �� ó�� */
			{
				gtimer[new_one].type.content[DA16X_IDX_NEXT] = current_id;
				return new_one;
			}
			else
			{
				gtimer[before].type.content[DA16X_IDX_NEXT] = new_one;
				gtimer[new_one].type.content[DA16X_IDX_NEXT] = current_id;
				return ret;
			}
		}
		else if (gtimer[current_id].time < gtimer[new_one].time )
		{
			gtimer[new_one].time -= gtimer[current_id].time;
			if (gtimer[current_id].type.content[DA16X_IDX_NEXT] != TIMER_ERR)
			{
				before = current_id;
				current_id = (DA16X_TIMER_ID)gtimer[current_id].type.content[DA16X_IDX_NEXT];
			}
			else
			{
				gtimer[current_id].type.content[DA16X_IDX_NEXT] = new_one;
				return ret;
			}
		}
		else
		{
			/* ���� ��� scheduler���� ó���� �ش�. -> 0���� �ϴ� ���� �������� �´�. */
			gtimer[new_one].type.content[DA16X_IDX_NEXT] = gtimer[current_id].type.content[DA16X_IDX_NEXT];
			gtimer[current_id].type.content[DA16X_IDX_NEXT] = new_one;
			gtimer[new_one].time = 0;
			return ret;
		}
		idx++;

		// GPIO retention issue
		if (idx > FIXED_TIMER_ERR)
			return TIMER_ERR;
	}

	if (gtimer[ret].time > gtimer[new_one].time)
		return new_one;
	else
		return ret;

#endif
}


/* grobal functions */
DA16X_TIMER_ID DA16X_GET_EMPTY_ID(void)
{
	DA16X_TIMER_ID i;
	for(i = TIMER_5; i < TIMER_ERR; i++)
	{
		if (chk_timer_id(i) == i)
			break;
	}
	return i;
}

DA16X_TIMER_ID DA16X_SET_TIMER(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param)
{
	if (isr_block)
		return TIMER_ERR;

    if (DA16200_TIMER_ALLOC(id, time, param) == TIMER_ERR)
    {
        return TIMER_ERR;
    }

	if (hTimer == NULL)
	{
		hTimer = MCNT_CREATE();

		if (hTimer == NULL)
			return TIMER_ERR;
	}

	/* ���� �ɷ��ִ� Ÿ�̸Ӱ� �ִ��� Ȯ�� */
	if (gCurIdx != TIMER_ERR)
	{
		UINT64 remain;
		if (MCNT_STOP(hTimer, &remain))
			gtimer[gCurIdx].time = CLK2US(remain);
		else
		{
#ifdef THREADX
			// unknown status
			(ramsymbols->printf)("set timer unknown status");
#endif
		}
		gCurIdx = time_sorting(gCurIdx, id);
    }
	else
	{
		gCurIdx = id;
	}

	DA16200_TIMER_ACTIVE(gCurIdx);

    return id;
}

//#pragma optimize=none
DA16X_TIMER_ID DA16X_SET_WAKEUP_TIMER(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param)
{
	DA16X_TIMER_ID next = TIMER_ERR, before;
	volatile unsigned long long temp_gap;
	UINT32 idx = 0;
#ifdef RTC_WATCHDOG
	volatile unsigned long long watchdog;
	UINT32 i;
#endif
	if (isr_block)
		return TIMER_ERR;

    if (DA16200_TIMER_ALLOC(id, time, param) == TIMER_ERR)
    {
        return TIMER_ERR;
    }

	/* ���� �ɷ��ִ� Ÿ�̸Ӱ� �ִ��� Ȯ�� */
	if (gCurIdx != TIMER_ERR)
	{
		if (hTimer != NULL)
		{
			UINT64 remain;
			if (MCNT_STOP(hTimer, &remain))
				gtimer[gCurIdx].time = CLK2US(remain);
			else
			{
				// unknown status
				//(ramsymbols->printf)("set wakeup timer unknown status");
			}
		}

		if (time_gap != 0) {
			temp_gap = CLK2US(get_remain_counter() + 2);	// 32us ���� ���� �ڵ� Ÿ�� �ð�-> �ð� ������ �����
			before = gCurIdx;
			next = gCurIdx;
			while ((next != TIMER_ERR) && (temp_gap > 0))
			{
				if (gtimer[next].time > temp_gap)
				{
					gtimer[next].time -= temp_gap;
					break;
				}
				else
				{
					temp_gap -= gtimer[next].time;
					gtimer[next].time = 0;
					next = (DA16X_TIMER_ID)gtimer[next].type.content[DA16X_IDX_NEXT];
					if(temp_gap < MINIMUM_TIME_SLICE)
						break;
				}
				idx++;

				// GPIO retention issue
				if (idx > FIXED_TIMER_ERR)
					break;
			}
			gCurIdx = before;
			idx = 0;
		}
		gCurIdx = time_sorting(gCurIdx, id);
	}
	else
	{
		gCurIdx = id;
	}

	if (id < 2)
		BOOT_TIM = 1;

    ((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_RTCM] = (UINT32)gtimer[gCurIdx].booting_offset;
	// sleep �ð��� 1000us ���� ���� ��� 1000us�� �����Ѵ�.
	if (gtimer[gCurIdx].time < 1000)
        gtimer[gCurIdx].time = 1000;

#ifdef RTC_WATCHDOG
	// sleep �Ϸ��� �ð����� ���� �ð��� �����Ͽ� watchdog enable
	watchdog = (US2CLK(gtimer[gCurIdx].time));
	for (i = 35; i > 14; i--)
	{
		if (watchdog & (0x01 << i))
			break;
	}
	//RTC_SET_WATCHDOG_PERIOD(i - 14);
	MEM_LONG_WRITE(0x5009105c, ((i>14)?(i-14):1));

	//RTC_WATCHDOG_EN();
	MEM_LONG_WRITE(0x50091028, 0x00);
	MEM_LONG_OR_WRITE32(0x50091010, WATCHDOG_ENABLE(1));

	{
		struct dpm_param *dpmp = GET_DPMP();
		MK_TRG_RA(DPM_TRG_END);
	}

	//MEM_LONG_OR_WRITE32(0x50091018, IO_RETENTION_ENABLE(IO_RETENTION_DIO1));
	//GPIO_CHANGE_TO_INPUT();
	//APPLY_PULLDOWN_DISABLE();

	RTC_INTO_POWER_DOWN(gCurIdx, watchdog);
#else
	{
		/*struct dpm_param *dpmp = GET_DPMP();
		MK_TRG_RA(DPM_TRG_END);*/
	}

	//MEM_LONG_OR_WRITE32(0x50091018, IO_RETENTION_ENABLE(IO_RETENTION_DIO1));
	//GPIO_CHANGE_TO_INPUT();
	//APPLY_PULLDOWN_DISABLE();

	RTC_INTO_POWER_DOWN(gCurIdx, (US2CLK(gtimer[gCurIdx].time)));
	//(ramsymbols->printf)("into sleep");
#endif

    return id;
}

#pragma GCC optimize ("O0")
DA16X_TIMER_ID DA16X_KILL_TIMER(DA16X_TIMER_ID id)
{
	UINT8 next;
	UINT32 idx = 0;

	if (isr_block)
		return TIMER_ERR;
	/* ���� ���� id�� ��������� �ʴٸ� kill ���� �� �ʿ� ���� */
	if (chk_timer_id(id) == id)
        return TIMER_ERR;

	// GPIO retention issue
	if (id == TIMER_9 || id == TIMER_10)
		return TIMER_ERR;

	gtimer[id].type.content[DA16X_IDX_USING] = 0;

	/* �ش� id�� ã�� id ������ offset�� ������ �ش�. */
	if (hTimer == NULL)
		return TIMER_ERR;

	if (gCurIdx == id)
	{
		UINT64 remain;

		if(MCNT_STOP(hTimer, &remain))
		{

		}
		else
		{
			//(ramsymbols->printf)("kill id stop unknown\n");
		}

		next = gtimer[id].type.content[DA16X_IDX_NEXT];
		if (next == TIMER_ERR)
		{
			gCurIdx = TIMER_ERR;
			return id;
		}
		//(ramsymbols->printf)("kill id %d, next %d, ioctldata %d,  time  %d\n", id,next, ioctldata ,(UINT32)gtimer[next].time );
		gtimer[next].time += CLK2US(remain);
		gCurIdx = (DA16X_TIMER_ID)next;
        	DA16200_TIMER_ACTIVE(gCurIdx);
	}
	else
	{
		/* ���� ���� id�� ����Ű�� index�� ã�´�. */
		next = gCurIdx;
		while(gtimer[next].type.content[DA16X_IDX_NEXT] != id)
		{
			next = gtimer[next].type.content[DA16X_IDX_NEXT];
			idx++;

			// GPIO retention issue
			if (idx >= FIXED_TIMER_ERR)
				return TIMER_ERR;
		}

		/*LOOP �Ŀ� next�� id ���� ���� ��� �ִ�.*/
		gtimer[next].type.content[DA16X_IDX_NEXT] = gtimer[id].type.content[DA16X_IDX_NEXT];

		if (gtimer[id].type.content[DA16X_IDX_NEXT] != TIMER_ERR)
		{
			//(ramsymbols->printf)("kill id %d, next id %d, time %d\n", id,gtimer[id].type.content[DA16X_IDX_NEXT], (UINT32)gtimer[id].time );
			gtimer[gtimer[id].type.content[DA16X_IDX_NEXT]].time += gtimer[id].time;
		}
	}
	return id;
}

//#pragma optimize=none
/* full booting �� da16200 timer�� �ʱ�ȭ ȭ�� ���ҵ� �Ѵ�. */
WAKEUP_SOURCE DA16X_TIMER_SCHEDULER(void)
{
	WAKEUP_SOURCE wakeup_source;
	UINT32 wakeup_id, ioctldata[3], idx = 0, next = TIMER_ERR;

	/* wake up source read */
	wakeup_source = RTC_GET_WAKEUP_SOURCE();
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &(ioctldata[0]));
	wakeup_id = (ioctldata[0] >> 4) & 0x0f;

	/* wake up source clear */


	gCurIdx = (DA16X_TIMER_ID)wakeup_id;
	//(ramsymbols->printf)("gCurIdx %d addr %x\n",(UINT32)gCurIdx, &gCurIdx);
	//(ramsymbols->printf)("wakeup source %x\n",(UINT32)wakeup_source);

    time_gap = 0;
#ifdef RTC_WATCHDOG
	//RTC_WATCHDOG_DIS();
	MEM_LONG_AND_WRITE32(0x50091010, ~(WATCHDOG_ENABLE(1)));
#endif
	switch(wakeup_source)
	{
		case WAKEUP_COUNTER_WITH_RETENTION:
		case WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION:
		case WAKEUP_SENSOR_WAKEUP_COUNTER_WITH_RETENTION:
		case WAKEUP_SENSOR_EXT_WAKEUP_COUNTER_WITH_RETENTION:
			/* 160218 cloud id 0���� ���� ���� full booting�� �� ���� ������ ������ �ʿ� ����  �Ʒ� �ڵ带 �ּ����� ������ */
			/* id 0�� tim���� ���� ���� ��� */
			//RTC_READY_POWER_DOWN(0);
			if (wakeup_id < 2)
			{
				if (BOOT_TIM)
				{
					gtimer[wakeup_id].type.content[DA16X_IDX_USING] = 0;
					BOOT_TIM = 0;
					gCurIdx = (DA16X_TIMER_ID)gtimer[wakeup_id].type.content[DA16X_IDX_NEXT];
          			// boot �ð� ���� �Ŀ� �ٽ� sleep ���� ������ �� ����� �ð��� ����
          			time_gap = get_counter();

					// SLR external wakeup ��ȣ�� ���� ���� ��� counter wakeup status�� clear�Ѵ�.
					if (wakeup_source == WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION)
					{
						RTC_IOCTL(RTC_GET_WAKEUP_SOURCE_REG, &(ioctldata[0]));
						ioctldata [0] = (ioctldata[0] & 0xf0) | 0x0e;
						RTC_IOCTL(RTC_SET_WAKEUP_SOURCE_REG, &(ioctldata[0]));
					}
					return wakeup_source;
				}
				else
				{
					time_gap = get_remain_counter();
					gtimer[wakeup_id].type.content[DA16X_IDX_USING] = 1;
				}
			}
			else
			{
				gCurIdx = (DA16X_TIMER_ID)wakeup_id;
				time_gap = get_remain_counter();
			}
			/* callback current timer id*/
			//(ramsymbols->printf)("run callback \n");
			next = DA16200_PROCESS_CALLBACK((DA16X_TIMER_ID)wakeup_id);
			//(ramsymbols->printf)("callback id %d\n",(UINT32)wakeup_id);
			time_gap = 0;

			// Wakeup source clearing
			RTC_READY_POWER_DOWN(1);

			/* enable next timer */
			time_gap = get_counter();

			if ((next != TIMER_ERR) && (gtimer[next].type.content[DA16X_IDX_USING] ))
			{
				if (hTimer == NULL)
				{
					hTimer = MCNT_CREATE();
					if (hTimer == NULL)
						return wakeup_source;
				}
				gCurIdx = (DA16X_TIMER_ID)next;
				/* Ÿ�̸� ���� �Ŀ� ���� */
				//(ramsymbols->printf)("gCurIdx %d\n", gCurIdx);
				DA16200_TIMER_ACTIVE(gCurIdx);
			}
			else
				gCurIdx = TIMER_ERR;
			//(ramsymbols->printf)("time diff %d\n", (UINT32)(get_counter() - time_gap));
			time_gap = 0;
			break;
		case WAKEUP_EXT_SIG_WITH_RETENTION:
		case WAKEUP_SENSOR_WITH_RETENTION:
		case WAKEUP_SENSOR_EXT_SIGNAL_WITH_RETENTION:
		// ���� �ɷ� �ִ� Ÿ�̸Ӱ� expired �� ������ Ȯ�� �ϰ� �ƴ� ��� ���� �ɷ� �ִ� Ÿ�̸Ӹ� simple time�� ��� ����
		// tim id�� �ö� ���ų� �ٸ� id�� �ö���� �� �ٵ�
		// tim id �ϰ�� tim ���� �� external wakeup signal ����
		// tim id�� �ƴ� ��� �ش� ���̵��� ��� �ð��� ����ؼ� �������־�� ��
			if (wakeup_id < 2)
			{
				gtimer[wakeup_id].type.content[DA16X_IDX_USING] = 0;
				gCurIdx = (DA16X_TIMER_ID)gtimer[wakeup_id].type.content[DA16X_IDX_NEXT];
				gtimer[gCurIdx].time += CLK2US(get_remain_counter());
			}
			else
			{
				gCurIdx = (DA16X_TIMER_ID)wakeup_id;
				gtimer[gCurIdx].time = CLK2US(get_remain_counter());
			}

			// Wakeup source clearing
			RTC_READY_POWER_DOWN(1);

          	// boot �ð� ���� �Ŀ� �ٽ� sleep ���� ������ �� ����� �ð��� ����
          	/* enable next timer */
			if ((gCurIdx != TIMER_ERR) && (gtimer[gCurIdx].type.content[DA16X_IDX_USING]))
			{
				if (hTimer == NULL)
				{
					hTimer = MCNT_CREATE();

					if (hTimer == NULL)
						return wakeup_source;
				}
				/* Ÿ�̸� ���� �Ŀ� ���� */
				DA16200_TIMER_ACTIVE(gCurIdx);
			}
			else
				gCurIdx = TIMER_ERR;
			break;
		case WAKEUP_SOURCE_POR:
		case WAKEUP_RESET:
		case WAKEUP_SOURCE_EXT_SIGNAL:
        case WAKEUP_SOURCE_WAKEUP_COUNTER:
		case WAKEUP_WATCHDOG:
		case WAKEUP_WATCHDOG_EXT_SIGNAL:
		case WAKEUP_SENSOR:
		case WAKEUP_PULSE:
		case WAKEUP_GPIO:
		case WAKEUP_SENSOR_EXT_SIGNAL:
		case WAKEUP_SENSOR_WAKEUP_COUNTER:
		case WAKEUP_SENSOR_EXT_WAKEUP_COUNTER:
		case WAKEUP_SENSOR_WATCHDOG:
		case WAKEUP_SENSOR_EXT_WATCHDOG:
			RTC_READY_POWER_DOWN(1);
			MINIMUM_TIME_SLICE = 200;
			BOOT_TIM = 0;

			memset((void *) &gtimer[0],0x00, sizeof(DA16X_TIMER) * 9);
			memset((void *) &gtimer[11], 0x00, sizeof(DA16X_TIMER) * 5);

			for(idx = 0; idx < TIMER_ERR; idx++)
			{
				// GPIO retention issue
				if (idx == TIMER_9 || idx == TIMER_10)
					continue;

				gtimer[idx].type.content[DA16X_IDX_NEXT] = TIMER_ERR;
			}
            gCurIdx = TIMER_ERR;
			isr_block = 0;
			break;
		default :
			gCurIdx = TIMER_ERR;
			//(ramsymbols->printf)("scheduler return\n");
			break;
	}
	MEM_LONG_WRITE(0x50091028, 0x00);
	return wakeup_source;
}

DA16X_TIMER_ID DA16X_SET_PERIOD(DA16X_TIMER_ID id, unsigned long long time, DA16X_TIMER_PARAM param)
{
	DA16X_UNUSED_ARG(id);
	DA16X_UNUSED_ARG(time);
	DA16X_UNUSED_ARG(param);
#if 0
	UINT32 ioctldata[3];

    if (DA16200_TIMER_ALLOC(id, time, param) == TIMER_ERR)
        return TIMER_ERR;

	/* ���� �ɷ��ִ� Ÿ�̸Ӱ� �ִ��� Ȯ�� */
	if (gCurIdx != TIMER_ERR)
	{
#ifndef TIMER_DEBUG
		if (hTimer != NULL)
		{
			STIMER_IOCTL(hTimer, STIMER_SET_DEACTIVE, NULL);
			STIMER_IOCTL(hTimer, STIMER_GET_UTIME, &ioctldata[0]);
			gtimer[gCurIdx].time = ioctldata[0];
		}
#endif
		if( time_gap != 0 )
		{
			/* non os ���¿��� �ɸ� �ð���ŭ gCurIdx�� ���� ���ش�.*/
			if (gtimer[gCurIdx].time > CLK2US((get_counter() - time_gap)))
				gtimer[gCurIdx].time -= CLK2US((get_counter() - time_gap));
			else
				gtimer[gCurIdx].time = 1;
        }
        gCurIdx = time_sorting(gCurIdx, id);
    }
    else
    {
        gCurIdx = id;
    }
	time_gap = get_counter();
	if (id < 2)
		BOOT_TIM = 1;

    return id;
#endif
	return TIMER_ERR;
}

void DA16X_SLEEP_EN()
{
#if 0
	volatile unsigned long long time_new;

	time_new = get_counter();
	if (time_gap != 0)
	{
		if (gtimer[gCurIdx].time > CLK2US((time_new - time_gap)))
			gtimer[gCurIdx].time -= CLK2US((time_new - time_gap));
		else
			gtimer[gCurIdx].time = 1;
	}

	if (gtimer[gCurIdx].time < 1000)
		gtimer[gCurIdx].time = 1000;
    ((UINT32 *)RETMEM_BOOTMODE_BASE)[BOOT_IDX_RTCM] = (UINT32)gtimer[gCurIdx].booting_offset;
    RTC_INTO_POWER_DOWN(gCurIdx, (US2CLK(gtimer[gCurIdx].time)));
#endif
}

/* EOF */
