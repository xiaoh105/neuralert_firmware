/**
 ****************************************************************************************
 *
 * @file da16200_ioconfig.c
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


#include "da16200_ioconfig.h"
#include "da16200_regs.h"

#undef SUPPORT_IOC_DEBUG

#ifdef SUPPORT_IOC_DEBUG
#define IOC_PRINT(...)	DRV_PRINTF(__VA_ARGS__)
#else
#define IOC_PRINT(...)
#endif


#define SUPPORT_GPIO_RETENTION_LOW
//#undef SUPPORT_GPIO_RETENTION_LOW

#ifdef SUPPORT_GPIO_RETENTION_LOW
#define RETMEM_GPIO_RETENTION_LOW_INFO_BASE (RETMEM_GPIO_RETENTION_INFO_BASE + 0x10)
#define RETMEM_GPIO_RETENTION_LOW_INFO ((GPIO_RETENTION_INFO_TypeDef *)RETMEM_GPIO_RETENTION_LOW_INFO_BASE)
#endif

#define IOC_CLEAR_BITS(reg, area, loc)		((reg) &= ~((area) << loc))
#define IOC_SET_BIT(reg, loc)	((reg) |= ((0x1) << (loc)))
#define IOC_SET_BITS(reg, val, loc)		((reg) |= ((val) << (loc)))
#define IOC_GET_BIT(reg, loc)	((reg) & ((0x1) << (loc)))
#define IOC_EXTRACT_BITS(reg, area, loc) (((reg) >> (loc)) & (area))

const UINT8 pinmap_bitmap[19][2] = {
		{  0, 0x0F  }, 	// PIN_AMUX, FSEL_GPIO1[ 3: 0] 4
		{  4, 0x0F  },	// PIN_BMUX, FSEL_GPIO1[ 7: 4] 4
		{  8, 0x0F  },	// PIN_CMUX, FSEL_GPIO1[11: 8] 4
		{ 12, 0x0F },	// PIN_DMUX, FSEL_GPIO1[15:12] 4
		{ 16, 0x0F },	// PIN_EMUX, FSEL_GPIO1[19:16] 4
		{ 20, 0x0F },	// PIN_FMUX, FSEL_GPIO1[22:20] 4
		{ 24, 0x03 },	// PIN_IMUX, FSEL_GPIO1[25:24] 2
		{ 26, 0x03 },	// PIN_JMUX, FSEL_GPIO1[27:26] 2
		{ 28, 0x03 },	// PIN_KMUX, FSEL_GPIO1[29:28] 2
		{ 30, 0x03 },	// PIN_HMUX, FSEL_GPIO1[31:30] 2
		{  0, 0x07  },	// PIN_LMUX, FSEL_GPIO2[ 2: 0] 3
		{  4, 0x07  },	// PIN_MMUX, FSEL_GPIO2[ 6: 4] 3
		{  8, 0x03  },	// PIN_NMUX, FSEL_GPIO2[ 9: 8] 2
		{ 10, 0x03  },	// PIN_PMUX, FSEL_GPIO2[11:10] 2
		{ 12, 0x03  },	// PIN_QMUX, FSEL_GPIO2[13:12] 2
		{ 14, 0x03  },	// PIN_RMUX, FSEL_GPIO2[15:14] 2
		{ 16, 0x03  },	// PIN_SMUX, FSEL_GPIO2[17:16] 2
		{ 18, 0x03  },	// PIN_TMUX, FSEL_GPIO2[19:18] 2
		{ 20, 0x03  }	// PIN_UMUX, FSEL_GPIO2[21:20] 2
};

/*
 * For IO Current Reduction
 */
#define GPIO_IE_IS_CONFIG_MAX_LEN	29
const UINT16 gpio_ie_is_config[GPIO_IE_IS_CONFIG_MAX_LEN][3] = {
		// mux, config, reg_val
		{ PIN_AMUX, AMUX_SPIs, 0x2 },
		{ PIN_AMUX, AMUX_I2S, 0x0 },
		{ PIN_AMUX, AMUX_UART1d, 0x2 },
		{ PIN_AMUX, AMUX_I2Cm, 0x1 },
		{ PIN_BMUX, BMUX_I2S, 0x4 },
		{ PIN_BMUX, BMUX_UART1d, 0x8 },
		{ PIN_CMUX, CMUX_SDm, 0x10 },
		{ PIN_CMUX, CMUX_I2Cm, 0x10 },
		{ PIN_CMUX, CMUX_UART1d, 0x20 },
		{ PIN_CMUX, CMUX_I2S, 0x0 },
		{ PIN_DMUX, DMUX_UART1d, 0x80 },
		{ PIN_DMUX, DMUX_I2S, 0x40 },
		{ PIN_EMUX, EMUX_SPIs, 0x200 },
		{ PIN_EMUX, EMUX_I2Cm, 0x100 },
		{ PIN_EMUX, EMUX_I2S, 0x0 },
		{ PIN_FMUX, FMUX_SPIs, 0x800 },
		{ PIN_FMUX, FMUX_UART2, 0x800 },
		{ PIN_IMUX, IMUX_UART2I2S, 0x0 },
		{ PIN_LMUX, LMUX_QSPI, 0x1800 },
		{ PIN_LMUX, LMUX_sSPI, 0xe00 },
		{ PIN_LMUX, LMUX_I2S, 0x1000 },
		{ PIN_MMUX, MMUX_UART2, 0x2000 },
		{ PIN_NMUX, NMUX_mSPI, 0xe },
		{ PIN_PMUX, PMUX_mSPI, 0x80 },
		{ PIN_QMUX, QMUX_I2S, 0x400 },
		{ PIN_RMUX, RMUX_UART1, 0x2 },
		{ PIN_RMUX, RMUX_mI2C, 0x1 },
		{ PIN_SMUX, SMUX_UART1, 0x8 },
		{ PIN_UMUX, UMUX_UART2GPIO, 0x180 }
};

const UINT16 gpio_pinmux_info[19][2] = {
		// GPIO port num, pin number mask
		{ 0x0, 0x3 }, // PIN_AMUX
		{ 0x0, 0xc },  // PIN_BMUX
		{ 0x0, 0x30 },  // PIN_CMUX
		{ 0x0, 0xc0 },  // PIN_DMUX
		{ 0x0, 0x300 },  // PIN_EMUX
		{ 0x0, 0xc00 },  // PIN_FMUX
		{ 0x0, 0x1000 },  // PIN_IMUX
		{ 0x0, 0x2000 },  // PIN_JMUX
		{ 0x0, 0x4000 },  // PIN_KMUX
		{ 0x0, 0x8004 },  // PIN_HMUX
		{ 0x2, 0x1e00 },  // PIN_LMUX
		{ 0x2, 0x6000 },  // PIN_MMUX
		{ 0x1, 0xf },  // PIN_NMUX
		{ 0x1, 0xf0 },  // PIN_PMUX
		{ 0x1, 0xf00 },  // PIN_QMUX
		{ 0x2, 0x3 },  // PIN_RMUX
		{ 0x2, 0xc },  // PIN_SMUX
		{ 0x2, 0x30 },  // PIN_TMUX
		{ 0x2, 0x1c0 }   // PIN_UMUX
};

/*
 * Set output only pin to input disable for current reduction.
 */
void set_gpio_ie_is(UINT32 mux, UINT32 config) {
	UINT16 val, port, mask, i;

	/*
	 * Set all GPIOA port pins to input disable when in test mode.
	 */
	if (DA16200_DIOCFG->Test_Control != 0) {
		val = 0xffff;
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOA_IE_IS, val, 16);
		return;
	}

	port = gpio_pinmux_info[mux][0];
	mask = gpio_pinmux_info[mux][1];

	/*
	 * Set input enable.
	 */
	if (port == GPIO_UNIT_A) {
		IOC_SET_BITS(DA16200_DIOCFG->GPIOA_IE_IS, mask, 16);
	} else if (port == GPIO_UNIT_B) {
		IOC_SET_BITS(DA16200_DIOCFG->GPIOB_IE_IS, mask, 16);
	} else if (port == GPIO_UNIT_C) {
		IOC_SET_BITS(DA16200_DIOCFG->GPIOC_IE_IS, mask, 16);
	}


	for (i = 0; i < GPIO_IE_IS_CONFIG_MAX_LEN; i++) {
		if ((gpio_ie_is_config[i][0] == mux) && (gpio_ie_is_config[i][1] == config)) {
			val = gpio_ie_is_config[i][2];
			break;
		}
	}

	// not selected
	if (i == GPIO_IE_IS_CONFIG_MAX_LEN)
		return;

	port = gpio_pinmux_info[mux][0];
	mask = gpio_pinmux_info[mux][1];

	/*
	 * Set input disable.
	 */
	if (port == GPIO_UNIT_A) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOA_IE_IS, ((~val) & mask), 16);
	} else if (port == GPIO_UNIT_B) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOB_IE_IS, ((~val) & mask), 16);
	} else if (port == GPIO_UNIT_C) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOC_IE_IS, ((~val) & mask), 16);
	}
}

/*
 * pull up/down disable
 */
void set_gpio_pe_ps(UINT32 mux)
{
	UINT16 port, pins;

	port = gpio_pinmux_info[mux][0];
	pins = gpio_pinmux_info[mux][1];

	if (port == GPIO_UNIT_A) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOA_PE_PS, pins, 16);
	} else if (port == GPIO_UNIT_B) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOB_PE_PS, pins, 16);
	} else if (port == GPIO_UNIT_C) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOC_PE_PS, pins, 16);
	} else {
		return;
	}
}

static UINT32 da16x_io_get_fsel_gpio_reg(UINT32 mux)
{
	if (mux > PIN_HMUX) {
		return DA16200_DIOCFG->FSEL_GPIO2;
	}
	else {
		return DA16200_DIOCFG->FSEL_GPIO1;
	}
}

static void da16x_io_set_fsel_gpio_reg(UINT32 mux, UINT32 pinmap)
{
	if (mux > PIN_HMUX) {
		DA16200_DIOCFG->FSEL_GPIO2 = pinmap;
		RETMEM_PINMUX_INFO->fsel_gpio2 = pinmap;
	}
	else {
		DA16200_DIOCFG->FSEL_GPIO1  = pinmap;
		RETMEM_PINMUX_INFO->fsel_gpio1 = pinmap;
	}
}

UINT32 _da16x_io_pinmux(UINT32 mux, UINT32 config)
{
	UINT32 bitshift, bitmask, pinmap;

	bitshift = (UINT32) (pinmap_bitmap[mux][0]);
	bitmask = (UINT32) (pinmap_bitmap[mux][1]);

	pinmap = da16x_io_get_fsel_gpio_reg(mux) & (~(bitmask << bitshift));
	pinmap = pinmap | ((config & bitmask) << bitshift);

	da16x_io_set_fsel_gpio_reg(mux, pinmap);

	set_gpio_ie_is(mux, config);
	set_gpio_pe_ps(mux);
	return 0;
}

UINT32 _da16x_io_get_pinmux(UINT32 mux)
{
	UINT32 bitshift, bitmask;

	bitshift = (UINT32) (pinmap_bitmap[mux][0]);
	bitmask = (UINT32) (pinmap_bitmap[mux][1]);

	return ((da16x_io_get_fsel_gpio_reg(mux) >> bitshift) & bitmask);
}


void _da16x_io_set_all_pinmux(DA16X_IO_ALL_PINMUX * pinmux)
{

	DA16200_DIOCFG->FSEL_GPIO1 = pinmux->FSEL_GPIO1;
	DA16200_DIOCFG->FSEL_GPIO2 = pinmux->FSEL_GPIO2;

	// save to RETM
	RETMEM_PINMUX_INFO->fsel_gpio1 = pinmux->FSEL_GPIO1;
	RETMEM_PINMUX_INFO->fsel_gpio2 = pinmux->FSEL_GPIO2;
}

void _da16x_io_get_all_pinmux(DA16X_IO_ALL_PINMUX * pinmux) {
	pinmux->FSEL_GPIO1 = DA16200_DIOCFG->FSEL_GPIO1;
	pinmux->FSEL_GPIO2 = DA16200_DIOCFG->FSEL_GPIO2;
}


struct __gpio_pullup_info__
{
	volatile UINT16 gpioa_pullup_info;
	volatile UINT16 gpiob_pullup_info;
	volatile UINT16 gpioc_pullup_info;
};

struct __gpio_internal_pullup_info__
{
	volatile UINT16 gpioa_internal_pullup_info;
	volatile UINT16 gpiob_internal_pullup_info;
	volatile UINT16 gpioc_internal_pullup_info;
};

typedef struct __gpio_pullup_info__ GPIO_PULLUP_INFO_TypeDef;
typedef struct __gpio_internal_pullup_info__ GPIO_INTERNAL_PULLUP_INFO_TypeDef;

#define RETMEM_GPIO_PULLUP_INFO_BASE (RETMEM_RTCM_BASE|0xe0)
#define RETMEM_GPIO_PULLUP_INFO ((GPIO_PULLUP_INFO_TypeDef *) RETMEM_GPIO_PULLUP_INFO_BASE)

#define RETMEM_GPIO_INTERNAL_PULLUP_INFO_BASE (RETMEM_RTCM_BASE|0xf8)
#define RETMEM_GPIO_INTERNAL_PULLUP_INFO ((GPIO_INTERNAL_PULLUP_INFO_TypeDef *) RETMEM_GPIO_INTERNAL_PULLUP_INFO_BASE)

static int pullup_init = 0;
static int pullup_internal_init = 0;
/*
 *
 * Register a pin that keeps high outside the chip.
 * The configured pin is pull-down disabled and pad disconnected during sleep.
 *
 */
void SAVE_INTERNAL_PULLUP_PINS_INFO(UINT32 port_num, UINT32 pinnum)
{
	if (pullup_internal_init == 0) {
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpioa_internal_pullup_info = 0;
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpiob_internal_pullup_info = 0;
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpioc_internal_pullup_info = 0;
		pullup_internal_init = 1;
	}

	switch (port_num) {
	case GPIO_UNIT_A:
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpioa_internal_pullup_info |= pinnum & 0xffff;
		break;
	case GPIO_UNIT_B:
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpiob_internal_pullup_info |= pinnum & 0x0fff;
		break;
	case GPIO_UNIT_C:
		RETMEM_GPIO_INTERNAL_PULLUP_INFO->gpioc_internal_pullup_info |= pinnum & 0x01ff;
		break;
	}
}


/*
 *
 * Register a pin that keeps high outside the chip.
 * The configured pin is pull-down disabled and pad disconnected during sleep.
 *
 */
void SAVE_PULLUP_PINS_INFO(UINT32 port_num, UINT32 pinnum)
{
	if (pullup_init == 0) {
		RETMEM_GPIO_PULLUP_INFO->gpioa_pullup_info = 0;
		RETMEM_GPIO_PULLUP_INFO->gpiob_pullup_info = 0;
		RETMEM_GPIO_PULLUP_INFO->gpioc_pullup_info = 0;
		pullup_init = 1;
	}

	switch (port_num) {
	case GPIO_UNIT_A:
		RETMEM_GPIO_PULLUP_INFO->gpioa_pullup_info |= pinnum & 0xffff;
		break;
	case GPIO_UNIT_B:
		RETMEM_GPIO_PULLUP_INFO->gpiob_pullup_info |= pinnum & 0x0fff;
		break;
	case GPIO_UNIT_C:
		RETMEM_GPIO_PULLUP_INFO->gpioc_pullup_info |= pinnum & 0x01ff;
		break;
	}
}

/*
 * Pull up pins and gpio retention high pins need to pull down disable.
 * All pins except pull up pins and gpio retention high pins need to pull down enable.
 * This fuction is needed for current reduction.
 */
void APPLY_PULLDOWN_DISABLE(void)
{
	UINT32 pins_info[GPIO_UNIT_MAX];
	UINT32 ioctl;

    pins_info[GPIO_UNIT_A] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffff, 16);
    pins_info[GPIO_UNIT_A] |= RETMEM_GPIO_PULLUP_INFO->gpioa_pullup_info;

    pins_info[GPIO_UNIT_B] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 0);
    pins_info[GPIO_UNIT_B] |= RETMEM_GPIO_PULLUP_INFO->gpiob_pullup_info;

    pins_info[GPIO_UNIT_C] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 16);
    pins_info[GPIO_UNIT_C] |= RETMEM_GPIO_PULLUP_INFO->gpioc_pullup_info;

    if (pins_info[GPIO_UNIT_A] & 0xffff) {
    	IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOA_PE_PS, pins_info[GPIO_UNIT_A], 16);
    }
	IOC_SET_BITS(DA16200_DIOCFG->GPIOA_PE_PS, ~(pins_info[GPIO_UNIT_A]), 16);

    if (pins_info[GPIO_UNIT_B] & 0x0fff) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOB_PE_PS, pins_info[GPIO_UNIT_B], 16);
	}
    IOC_SET_BITS(DA16200_DIOCFG->GPIOB_PE_PS, ~(pins_info[GPIO_UNIT_B]), 16);

    if (pins_info[GPIO_UNIT_C] & 0x1ff) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOC_PE_PS, pins_info[GPIO_UNIT_C], 16);
	}
	IOC_SET_BITS(DA16200_DIOCFG->GPIOC_PE_PS, ~(pins_info[GPIO_UNIT_C]), 16);

	IOC_PRINT("\tDA16X_DIOCFG->GPIOA_PE_PS: 0x%08x\n", DA16X_DIOCFG->GPIOA_PE_PS);
	IOC_PRINT("\tDA16X_DIOCFG->GPIOB_PE_PS: 0x%08x\n", DA16X_DIOCFG->GPIOB_PE_PS);
	IOC_PRINT("\tDA16X_DIOCFG->GPIOC_PE_PS: 0x%08x\n", DA16X_DIOCFG->GPIOC_PE_PS);

	/*
	 * Pull up pins and gpio retention high pins need to PAD disconnection.
	 */
	RTC_IOCTL(RTC_GET_RETENTION_CONTROL_REG, &ioctl);

	if (pins_info[GPIO_UNIT_A] & 0xffff) {
		ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO1);
	}

	if (pins_info[GPIO_UNIT_B] & 0x0FFF) {
		if (pins_info[GPIO_UNIT_B] & 0x003F)
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO1);
		if (pins_info[GPIO_UNIT_B] & 0x0FC0)
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO2);
	}

	if (pins_info[GPIO_UNIT_C] & 0x1FF) {
		if (pins_info[GPIO_UNIT_C] & 0x0003)
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO1);
		if (pins_info[GPIO_UNIT_C] & 0x01FC)
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO2);
	}

	for (int i = 4; i < 12; i++) {
		if ((IOC_GET_BIT(DA16200_DIOCFG->GPIOA_PE_PS, i + 16)) && (IOC_GET_BIT(DA16200_DIOCFG->GPIOA_PE_PS, i))) {
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO1);
			IOC_PRINT("\tpullup-pin: GPIOA[%d]\n", i);
		}
	}

	for (int i = 6; i < 9; i++) {
		if ((IOC_GET_BIT(DA16200_DIOCFG->GPIOC_PE_PS, i + 16)) && (IOC_GET_BIT(DA16200_DIOCFG->GPIOC_PE_PS, i))) {
			ioctl |= IO_RETENTION_ENABLE(IO_RETENTION_DIO2);
			IOC_PRINT("\tpullup-pin: GPIOC[%d]\n", i);
		}
	}


#ifdef SUPPORT_IOC_DEBUG
	IOC_PRINT("\tioctl: 0x%08x\n", ioctl);
	OAL_MSLEEP(2000);
#endif

	/* call RTC control */
	RTC_IOCTL(RTC_SET_RETENTION_CONTROL_REG, &ioctl);
}

/**
 * @brief Initialize retention memory to store pull-up pin information.
 */
void clear_retmem_gpio_pullup_info(void)
{
	memset((void *)RETMEM_GPIO_PULLUP_INFO_BASE, 0x0, sizeof(UINT32)*2);
}

static void gpio_set_input_mode(INT32 gpio_port, INT32 gpio_num)
{
	HANDLE gpio;
	UINT32 ioctldata;

	gpio = GPIO_CREATE(gpio_port);
	GPIO_INIT(gpio);

	ioctldata = gpio_num;
	GPIO_IOCTL(gpio, GPIO_SET_INPUT, &ioctldata);
	GPIO_CLOSE(gpio);

	IOC_PRINT("GPIO%d 0x%08x Pin set to input mode.\n", gpio_port, gpio_num);
}

static void disable_all_gpio_alt_func() {
	GPIO_HANDLER_TYPE	*gpio;
	CMSDK_GPIO_TypeDef *gpioreg;

	gpio = GPIO_CREATE(GPIO_UNIT_A);
	GPIO_INIT(gpio);
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);
	gpioreg->ALT_FUNC_OUT_EN = 0;
	GPIO_CLOSE(gpio);

	gpio = GPIO_CREATE(GPIO_UNIT_C);
	GPIO_INIT(gpio);
	gpioreg = (CMSDK_GPIO_TypeDef *)(gpio->dev_addr);
	gpioreg->ALT_FUNC_OUT_EN = 0;
	GPIO_CLOSE(gpio);
}

static void set_all_pins_to_gpio(void) {
	DA16200_DIOCFG->FSEL_GPIO1 = 0xFF688889;
	DA16200_DIOCFG->FSEL_GPIO2 = 0x22EAA00;
}

static UINT32 get_gpio_retention_mask(INT32 gpio_port)
{
	UINT32 mask;

	if (gpio_port == GPIO_UNIT_A) {
		// GPIOA[11:4]
		mask = 0xff0;
	} else if (gpio_port == GPIO_UNIT_B) {
		// no pins in DA16200 6x6
	} else if (gpio_port == GPIO_UNIT_C) {
		// GPIOC[8:6]
		mask = 0x1c0;
	}

	return mask;
}

void GPIO_CHANGE_TO_INPUT(void)
{
	UINT32 pin_info[GPIO_UNIT_MAX];
	UINT32 pin_info2[GPIO_UNIT_MAX];
	UINT32 pin_info_sum[GPIO_UNIT_MAX];
	UINT32 mask, temp;

	/*
	 * check for GPIO retention pins
	 */
	pin_info[GPIO_UNIT_A] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info1, 0xffff, 16);
	pin_info[GPIO_UNIT_B] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 0);
	pin_info[GPIO_UNIT_C] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_INFO->ret_info2, 0xffff, 16);

	pin_info2[GPIO_UNIT_A] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info1, 0xffff, 16);
	pin_info2[GPIO_UNIT_B] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, 0xffff, 0);
	pin_info2[GPIO_UNIT_C] = IOC_EXTRACT_BITS(RETMEM_GPIO_RETENTION_LOW_INFO->ret_info2, 0xffff, 16);

	pin_info_sum[GPIO_UNIT_A] = pin_info[GPIO_UNIT_A] | pin_info2[GPIO_UNIT_A];
	pin_info_sum[GPIO_UNIT_B] = pin_info[GPIO_UNIT_B] | pin_info2[GPIO_UNIT_B];
	pin_info_sum[GPIO_UNIT_C] = pin_info[GPIO_UNIT_C] | pin_info2[GPIO_UNIT_C];

	IOC_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info1: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info1);
	IOC_PRINT("\tRETMEM_GPIO_RETENTION_INFO->ret_info2: 0x%08x\n", RETMEM_GPIO_RETENTION_INFO->ret_info2);

	IOC_PRINT("pin_info_sum[0]: 0x%08x\n", pin_info_sum[0]);
	IOC_PRINT("pin_info_sum[1]: 0x%08x\n", pin_info_sum[1]);
	IOC_PRINT("pin_info_sum[2]: 0x%08x\n", pin_info_sum[2]);

	set_all_pins_to_gpio();

	disable_all_gpio_alt_func();

	mask = get_gpio_retention_mask(GPIO_UNIT_A);
	// internal pull-up PIN should not change to gpio input mode.
	temp = pin_info_sum[GPIO_UNIT_A];
	temp |= (DA16200_DIOCFG->GPIOA_PE_PS & 0x0fff);
	gpio_set_input_mode(GPIO_UNIT_A, (~temp) & mask);


	mask = get_gpio_retention_mask(GPIO_UNIT_C);

	// internal pull-up PIN should not change to gpio input mode.
	temp = pin_info_sum[GPIO_UNIT_C];
	temp |= (DA16200_DIOCFG->GPIOC_PE_PS & 0x1c0);
	gpio_set_input_mode(GPIO_UNIT_C, (~temp) & mask);
}

/*
 * Change Digital pad to cmos type for current reduction.
 */
void CHANGE_DIGITAL_PAD_TO_CMOS_TYPE(UINT32 gpio_port, UINT32 gpio_num)
{
	if (gpio_port == GPIO_UNIT_A) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOA_IE_IS, gpio_num, 0);
	} else if (gpio_port == GPIO_UNIT_B) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOB_IE_IS, gpio_num, 0);
	} else if (gpio_port == GPIO_UNIT_C) {
		IOC_CLEAR_BITS(DA16200_DIOCFG->GPIOC_IE_IS, gpio_num, 0);
	} else {
		return;
	}
}

/*******************************************
 *
 * RTC GPO out API
 *
 *******************************************/
struct _rtc_gpo_out_
{
	volatile UINT32 manual_cntrl;
	volatile UINT32 reserved1;
	volatile UINT32 reserved2;
	volatile UINT32 reserved3;
	volatile UINT32 gpo_out_value;
};

typedef struct _rtc_gpo_out_ RTC_GPO_OUT_TypeDef;

#define RTC_GPO_OUT_CNTRL_BASE 0x50091050
#define RTC_GPO_OUT_CNTRL ((RTC_GPO_OUT_TypeDef *) RTC_GPO_OUT_CNTRL_BASE)

void RTC_GPO_OUT_INIT(UINT8 mode) {
	UINT32 reg;

	// mode:0 for auto
	// mode;1 for manual

	// RTC_GPO_OUT manual or auto setting
	reg = RTC_GPO_OUT_CNTRL->manual_cntrl;
	if (mode == 1) {
		reg |= 0x08; // bit[3] for manual
	} else {
		// manual mode
		reg &= ~0x08; // bit[3] for auto
	}
	RTC_GPO_OUT_CNTRL->manual_cntrl = reg;
}

void RTC_GPO_OUT_CONTROL(UINT8 value) {
	UINT32 reg;

	// set output value
	reg = RTC_GPO_OUT_CNTRL->gpo_out_value;
	if (value)
		reg |= 0x80; // bit[7] for value
	else
		reg &= ~0x80;
	RTC_GPO_OUT_CNTRL->gpo_out_value = reg;
}

void _da16x_gpio_set_pull(GPIO_UNIT_TYPE gpio, UINT16 pin, PULL_TYPE type)
{
    UINT32 set_value = 0, clear_value = 0xFFFFFFFF;

    if (type == PULL_UP) {
        set_value = pin << 16 | pin;
    }
    else if (type == PULL_DOWN ) {
        set_value = pin << 16;
        clear_value &= ~(pin);
    }
    else {
        clear_value &= ~((pin<<16) | pin);
    }

    if (gpio == GPIO_UNIT_A) {
    	DA16200_DIOCFG->GPIOA_PE_PS |= set_value;
    	DA16200_DIOCFG->GPIOA_PE_PS &= clear_value;
	} else if (gpio == GPIO_UNIT_B) {
		DA16200_DIOCFG->GPIOB_PE_PS |= set_value;
		DA16200_DIOCFG->GPIOB_PE_PS &= clear_value;
	} else if (gpio == GPIO_UNIT_C) {
		DA16200_DIOCFG->GPIOC_PE_PS |= set_value;
		DA16200_DIOCFG->GPIOC_PE_PS &= clear_value;
	} else {
		return;
	}
    return;
}
