/**
 ****************************************************************************************
 *
 * @file sys_specific.c
 *
 * @brief 
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
#include <stdlib.h>

#include "hal.h"
#include "sys_specific.h"

#define SUPPORT_GPIO_RETENTION_LOW
//#undef SUPPORT_GPIO_RETENTION_LOW

#ifdef SUPPORT_GPIO_RETENTION_LOW
#define RETMEM_GPIO_RETENTION_LOW_INFO_BASE (RETMEM_GPIO_RETENTION_INFO_BASE + 0x10)
#define RETMEM_GPIO_RETENTION_LOW_INFO ((GPIO_RETENTION_INFO_TypeDef *)RETMEM_GPIO_RETENTION_LOW_INFO_BASE)
#endif

#define GPIO_TRACE_PRINT(...)
//#define GPIO_TRACE_PRINT(...)	DRV_PRINTF(__VA_ARGS__)

static INT32 gpio_set_high(INT32 gpio_port, INT32 gpio_num)
{
	HANDLE gpio;
	UINT32 ioctldata;
	UINT16 data;

	gpio = GPIO_CREATE((UINT32)gpio_port);
	if (gpio == NULL)
	{
		return FALSE;
	}

	GPIO_INIT(gpio);

	ioctldata = (UINT32)gpio_num;
	data = (UINT16)gpio_num;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &ioctldata);
	GPIO_WRITE(gpio, (UINT32)gpio_num, &data, sizeof(UINT16));

	GPIO_TRACE_PRINT("GPIO%d Created, 0x%08x Pin Output High.\n", gpio_port, gpio_num);

	return TRUE;
}

#define CLEAR_BITS(reg, area, loc)		((reg) &= ~((area) << loc))
#define SET_BIT(reg, loc)	((reg) |= ((0x1) << (loc)))
#define SET_BITS(reg, val, loc)		((reg) |= ((val) << (loc)))
#define GET_BIT(reg, loc)	((reg) & ((0x1) << (loc)))
#define EXTRACT_BITS(reg, area, loc) (((reg) >> (loc)) & (area))

#ifdef SUPPORT_GPIO_RETENTION_LOW

static INT32 gpio_set_low(INT32 gpio_port, INT32 gpio_num)
{
	HANDLE gpio;
	UINT32 ioctldata;
	UINT16 data;

	gpio = GPIO_CREATE((UINT32)gpio_port);
	if (gpio == NULL)
	{
		return FALSE;
	}

	GPIO_INIT(gpio);

	ioctldata = (UINT32)gpio_num;
	data = 0;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &ioctldata);
	GPIO_WRITE(gpio, (UINT32)gpio_num, &data, sizeof(UINT16));

	GPIO_TRACE_PRINT("GPIO%d Created, 0x%08x Pin Output Low.\n", gpio_port, gpio_num);

	return TRUE;
}

INT32 __GPIO_RETAIN_HIGH_RECOVERY()
{
	UINT32 port_info, val = 0;
	UINT32 pin_info[GPIO_UNIT_MAX];

	/* restore pin mux info */
	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO1, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio1;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO1, val, 0);

	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO2, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio2;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO2, val, 0);

	/* Initialize GPIO retention high info area for a wake-up source that does not use retention memory. */
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		memset((void *)RETMEM_GPIO_RETENTION_INFO_BASE, 0x0, sizeof(UINT32)*2);

	/* Initialize GPIO retention low info area for a wake-up source that does not use retention memory. */
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		memset((void *)(RETMEM_GPIO_RETENTION_INFO_BASE + 0x10), 0x0, sizeof(UINT32)*2);

#if !defined ( __BLE_COMBO_REF__ ) && defined ( __SUPPORT_BTCOEX__ )
	/*initialize GPIO internal pull_up*/
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		SAVE_INTERNAL_PULLUP_PINS_INFO(GPIO_UNIT_MAX, 0);

	/*initialize GPIO internal pull_disable*/
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		SAVE_PULLUP_PINS_INFO(GPIO_UNIT_MAX, 0);
#endif

	GPIO_TRACE_PRINT("restore pin mux info\n");
	GPIO_TRACE_PRINT("\tFC9K_DIOCFG->FSEL_GPIO1: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO1);
	GPIO_TRACE_PRINT("\tFC9K_DIOCFG->FSEL_GPIO2: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO2);

	GPIO_TRACE_PRINT("\tRETMEM_PINMUX_INFO->fsel_gpio1: 0x%08x\n", RETMEM_PINMUX_INFO->fsel_gpio1);
	GPIO_TRACE_PRINT("\tRETMEM_PINMUX_INFO->fsel_gpio2: 0x%08x\n", RETMEM_PINMUX_INFO->fsel_gpio2);


	/************************************************
	 *
	 * GPIO Retention High
	 *
	 ************************************************/
	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);

	/* extract gpio retention info */
	port_info = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0x7, 0);
	pin_info[GPIO_UNIT_A] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffff, 16);
	pin_info[GPIO_UNIT_B] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 0);
	pin_info[GPIO_UNIT_C] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 16);

	GPIO_TRACE_PRINT("extract gpio retention info\n");
	GPIO_TRACE_PRINT("\tgpio port: 0x%08x\n", port_info);
	GPIO_TRACE_PRINT("\tgpio_a pin: 0x%08x\n", pin_info[0]);
	GPIO_TRACE_PRINT("\tgpio_b pin: 0x%08x\n", pin_info[1]);
	GPIO_TRACE_PRINT("\tgpio_c pin: 0x%08x\n", pin_info[2]);

	/* GPIOA clock enable */
	DA16X_CLOCK_SCGATE->Off_DAPB_GPIO0 = 0;

	/* create gpio instance */
	for (INT32 i = 0; i < GPIO_UNIT_MAX; i++)
	{
		if (GET_BIT(port_info, i))
			gpio_set_high(i, (INT32)(pin_info[i]));
	}

	/* clear gpio retention info */
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffffffff, 0);
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffffffff, 0);

	GPIO_TRACE_PRINT("clear gpio retention info\n");
	GPIO_TRACE_PRINT("\tret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);


	/************************************************
	 *
	 * GPIO Retention Low
	 *
	 ************************************************/
	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_LOW_INFO->ret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_LOW_INFO->ret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2);

	/* extract gpio retention info */
	port_info = EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, 0x7, 0);
	pin_info[GPIO_UNIT_A] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, 0xffff, 16);
	pin_info[GPIO_UNIT_B] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, 0xffff, 0);
	pin_info[GPIO_UNIT_C] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, 0xffff, 16);

	GPIO_TRACE_PRINT("extract gpio retention info\n");
	GPIO_TRACE_PRINT("\tgpio port: 0x%08x\n", port_info);
	GPIO_TRACE_PRINT("\tgpio_a pin: 0x%08x\n", pin_info[0]);
	GPIO_TRACE_PRINT("\tgpio_b pin: 0x%08x\n", pin_info[1]);
	GPIO_TRACE_PRINT("\tgpio_c pin: 0x%08x\n", pin_info[2]);

	/* GPIOA clock enable */
	DA16X_CLOCK_SCGATE->Off_DAPB_GPIO0 = 0;

	/* create gpio instance */
	for (INT32 i = 0; i < GPIO_UNIT_MAX; i++)
	{
		if (GET_BIT(port_info, i))
			gpio_set_low(i, (INT32)(pin_info[i]));
	}

	/* clear gpio retention info */
	CLEAR_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, 0xffffffff, 0);
	CLEAR_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, 0xffffffff, 0);

	GPIO_TRACE_PRINT("clear gpio retention info\n");
	GPIO_TRACE_PRINT("\tret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2);

	/************************************************
	 *
	 * call RTC control
	 *
	 ************************************************/
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &val);
	val &= ~(IO_RETENTION_ENABLE((IO_RETENTION_DIO1 | IO_RETENTION_DIO2 | IO_RETENTION_SEN_VDD | IO_RETENTION_FDIO)));
	RTC_IOCTL(RTC_SET_RETENTION_CONTROL_REG, &val);
	GPIO_TRACE_PRINT("RTC RETENTION CONTROL DISABLED\n");

	return TRUE;
}

/* DA16200 can be set GPIOA[11:4], GPIOC[8:6] */
static UINT32 check_gpio_retention_available(UINT32 gpio_port, UINT32 gpio_num) {
	/* GPIOA, GPIOC can be set. */
	if (gpio_port != GPIO_UNIT_A && gpio_port != GPIO_UNIT_C)
		return FALSE;

	/* GPIOA[11:4] can be set. */
	if (gpio_port == GPIO_UNIT_A && (gpio_num & 0xf00f ))
		return FALSE;

	/* GPIOC[8:6] can be set. */
	if (gpio_port == GPIO_UNIT_C && (gpio_num & 0xfe3f ))
		return FALSE;

	return TRUE;
}

INT32 _GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num)
{
	/* Check that the gpio port and pin available gpio retention. */
	if (check_gpio_retention_available(gpio_port, gpio_num) == FALSE)
		return FALSE;

	/* set output high */
	gpio_set_high((INT32)gpio_port, (INT32)gpio_num);

	/* save gpio port and gpio pin to retention memory */
	SET_BIT(RETMEM_GPIO_RETENTION_INFO->ret_info1, gpio_port);
	if (gpio_port == GPIO_UNIT_A) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, gpio_num, 16);
	} else if (gpio_port == GPIO_UNIT_B) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, gpio_num, 0);
	} else if (gpio_port == GPIO_UNIT_C) {
		SET_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, gpio_num, 16);
	}

	return TRUE;
}


INT32 _GPIO_RETAIN_LOW(UINT32 gpio_port, UINT32 gpio_num)
{
	/* Check that the gpio port and pin available gpio retention. */
	if (check_gpio_retention_available(gpio_port, gpio_num) == FALSE)
		return FALSE;

	/* set output low */
	gpio_set_low((INT32)gpio_port, (INT32)gpio_num);

	/* save gpio port and gpio pin to retention memory */
	SET_BIT(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, gpio_port);
	if (gpio_port == GPIO_UNIT_A) {
		SET_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, gpio_num, 16);
	} else if (gpio_port == GPIO_UNIT_B) {
		SET_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, gpio_num, 0);
	} else if (gpio_port == GPIO_UNIT_C) {
		SET_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, gpio_num, 16);
	}

	return TRUE;
}

#else

INT32 __GPIO_RETAIN_HIGH_RECOVERY()
{
	UINT32 port_info, val = 0;
	UINT32 pin_info[GPIO_UNIT_MAX];

	/* restore pin mux info */
	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO1, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio1;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO1, val, 0);

	CLEAR_BITS(DA16200_DIOCFG->FSEL_GPIO2, 0xffffffff, 0);
	val = RETMEM_PINMUX_INFO->fsel_gpio2;
	SET_BITS(DA16200_DIOCFG->FSEL_GPIO2, val, 0);

	/* Initialize GPIO retention info area for a wake-up source that does not use retention memory. */
	if ((fc9k_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		memset((void *)RETMEM_GPIO_RETENTION_INFO_BASE, 0x0, sizeof(UINT32)*2);

#if !defined ( __BLE_COMBO_REF__ ) && defined ( __SUPPORT_BTCOEX__ )
	/*initialize GPIO internal pull_up*/
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		SAVE_INTERNAL_PULLUP_PINS_INFO(GPIO_UNIT_MAX, 0);

	/*initialize GPIO internal pull_disable*/
	if ((da16x_boot_get_wakeupmode() & WAKEUP_RESET_WITH_RETENTION) == 0)
		SAVE_PULLUP_PINS_INFO(GPIO_UNIT_MAX, 0);
#endif

	GPIO_TRACE_PRINT("restore pin mux info\n");
	GPIO_TRACE_PRINT("\tFC9K_DIOCFG->FSEL_GPIO1: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO1);
	GPIO_TRACE_PRINT("\tFC9K_DIOCFG->FSEL_GPIO2: 0x%08x\n", DA16200_DIOCFG->FSEL_GPIO2);

	GPIO_TRACE_PRINT("\tRETMEM_PINMUX_INFO->fsel_gpio1: 0x%08x\n", RETMEM_PINMUX_INFO->fsel_gpio1);
	GPIO_TRACE_PRINT("\tRETMEM_PINMUX_INFO->fsel_gpio2: 0x%08x\n", RETMEM_PINMUX_INFO->fsel_gpio2);

	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);


	/* extract gpio retention info */
	port_info = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0x7, 0);
	pin_info[GPIO_UNIT_A] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffff, 16);
	pin_info[GPIO_UNIT_B] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 0);
	pin_info[GPIO_UNIT_C] = EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 16);

	GPIO_TRACE_PRINT("extract gpio retention info\n");
	GPIO_TRACE_PRINT("\tgpio port: 0x%08x\n", port_info);
	GPIO_TRACE_PRINT("\tgpio_a pin: 0x%08x\n", pin_info[0]);
	GPIO_TRACE_PRINT("\tgpio_b pin: 0x%08x\n", pin_info[1]);
	GPIO_TRACE_PRINT("\tgpio_c pin: 0x%08x\n", pin_info[2]);

	/* create gpio instance */
	for (INT32 i = 0; i < GPIO_UNIT_MAX; i++)
	{
		if (GET_BIT(port_info, i))
			gpio_set_high(i, pin_info[i]);
	}

	/* clear gpio retention info */
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffffffff, 0);
	CLEAR_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffffffff, 0);

	GPIO_TRACE_PRINT("clear gpio retention info\n");
	GPIO_TRACE_PRINT("\tret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	GPIO_TRACE_PRINT("\tret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);

	/* call RTC control */
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &val);
	val &= ~(IO_RETENTION_ENABLE((IO_RETENTION_DIO1 | IO_RETENTION_DIO2 | IO_RETENTION_SEN_VDD | IO_RETENTION_FDIO)));
	RTC_IOCTL(RTC_SET_RETENTION_CONTROL_REG, &val);
	GPIO_TRACE_PRINT("RTC RETENTION CONTROL DISABLED\n");

	return TRUE;
}
#endif

static RTC_BOR_CALLBACK black_isr, brown_isr;

void low_black_isr(void)
{
	black_isr();
}
void low_brown_isr(void)
{
	brown_isr();
}
static void	rtc_black_interrupt(void)
{
	INTR_CNTXT_SAVE();
	INTR_CNTXT_CALL(low_black_isr);
	INTR_CNTXT_RESTORE();
}
static void	rtc_brown_interrupt(void)
{
	INTR_CNTXT_SAVE();
	INTR_CNTXT_CALL(low_brown_isr);
	INTR_CNTXT_RESTORE();
}

/* if the user call this function, the user should call disable function before sleep */
void RTC_ENABLE_BROWN_BLACK(RTC_BOR_CALLBACK brown_callback, RTC_BOR_CALLBACK black_callback)
{
	UINT32 ioctldata;

	brown_isr = brown_callback;
	black_isr = black_callback;

	_sys_nvic_write( RTC_BLACK_IRQn, (void *)rtc_black_interrupt, 0x07 );	// black out
	_sys_nvic_write( RTC_BROWN_IRQn, (void *)rtc_brown_interrupt, 0x07);	// brown out

	RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &ioctldata);
	ioctldata |= 0x300;
	RTC_IOCTL(RTC_SET_BOR_CIRCUIT, &ioctldata);

	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &ioctldata);
	ioctldata |= ((brown_callback!=NULL)?BROWN_OUT_INT_ENABLE(1):BROWN_OUT_INT_ENABLE(0))
				| ((black_callback!=NULL)?BLACK_OUT_INT_ENABLE(1):BLACK_OUT_INT_ENABLE(0));
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &ioctldata);
}

void RTC_DISABLE_BROWN_BLACK(void)
{
	UINT32 ioctldata;

	_sys_nvic_write( RTC_BLACK_IRQn, (void *)NULL, 0x07);	// black out
	_sys_nvic_write( RTC_BROWN_IRQn, (void *)NULL, 0x07);	// brown out

	RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &ioctldata);
	ioctldata &= ~(0x300);
	RTC_IOCTL(RTC_SET_BOR_CIRCUIT, &ioctldata);

	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &ioctldata);
	ioctldata &= ~(BROWN_OUT_INT_ENABLE(1) | BLACK_OUT_INT_ENABLE(1));
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &ioctldata);
}

