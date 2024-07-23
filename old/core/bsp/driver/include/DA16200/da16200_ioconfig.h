/**
 ****************************************************************************************
 *
 * @file da16200_ioconfig.h
 *
 * @brief IO pin configuration of DA16200
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

#ifndef	__da16200_ioconfig_h__
#define	__da16200_ioconfig_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal.h"
#include "gpio.h"

/******************************************************************************
 *
 *  IO Mode
 *
 ******************************************************************************/
#define	PIN_AMUX	0
#define	PIN_BMUX	1
#define	PIN_CMUX	2
#define	PIN_DMUX	3
#define	PIN_EMUX	4
#define	PIN_FMUX	5
#define	PIN_IMUX	6
#define	PIN_JMUX	7
#define	PIN_KMUX	8
#define	PIN_HMUX	9
#define	PIN_LMUX	10
#define	PIN_MMUX	11
#define	PIN_NMUX	12
#define	PIN_PMUX	13
#define	PIN_QMUX	14
#define	PIN_RMUX	15
#define	PIN_SMUX	16
#define	PIN_TMUX	17
#define	PIN_UMUX	18
#define	PIN_ALLMUX	19

/******************************************************************************
 *
 *  IO Port
 *
 ******************************************************************************/
// AMUX
#define	AMUX_AD12			0	/* ADC (GPIOA[0]:ADC, GPIOA[1]:ADC) */
#define	AMUX_SPIs			1	/* SPIs (GPIOA[0]:MISO, GPIOA[1]:MOSI) */
#define	AMUX_I2S			2	/* I2S (GPIOA[0]:BCLK, GPIOA[1]:MCLK) */
#define	AMUX_I2Cs			3	/* I2C slave (GPIOA[0]:I2C_SDA, GPIOA[1]:I2C_CLK) */
#define	AMUX_UART1d			4	/* UART1 (GPIOA[0]:TXD, GPIOA[1]:RXD) */
#define	AMUX_I2Cm			5	/* I2C master (GPIOA[0]:I2C_SDA, GPIOA[1]:I2C_CLK)*/
#define	AMUX_5GC			6	/* 5G control[1:0] */
#define	AMUX_ADGPIO			7	/* GPIOA[0]:ADC, GPIOA[1]:GPIO */
#define	AMUX_ADWRP			8	/* GPIOA[0]:ADC, GPIOA[1]:mSDeMMC_WRP */
#define	AMUX_GPIO			9	/* GPIOA[1:0] */
#define	AMUX_GPIOALT		10	/* GPIOA[1:0] */

// BMUX
#define BMUX_AD12			0	/* ADC (GPIOA[2]:ADC, GPIOA[3]:ADC) */
#define	BMUX_SPIs			1	/* SPIs (GPIOA[2]:CSB, GPIOA[3]CLK)	*/
#define	BMUX_I2S			2	/* I2S (GPIOA[2]:I2S_SDO, GPIOA[3]:I2S_LRCK) */
#define	BMUX_I2Cs			3	/* I2C slave (GPIOA[2]:I2C_SDA, GPIOA[3]:I2C_CLK) */
#define	BMUX_UART1d			4	/* UART1 (GPIOA[2]:TXD, GPIOA[3]:RXD) */
#define	BMUX_ADCGPIO		6	/* GPIOA[2]: ADC, GPIOA[3]	  	*/
#define	BMUX_ADCI2S			7	/* GPIOA[2]: ADC, GPIOA[3]:I2S_CLK_in */
#define	BMUX_GPIO			8	/* GPIOA[3:2] */
#define	BMUX_GPIOALT   		9	/* GPIOA[3:2] */

// CMUX
#define	CMUX_5GC			0	/* 5G control[3:2]  */
#define	CMUX_I2Cs			1	/* I2C slave (GPIOA[4]:I2C_SDA, GPIOA[5]:I2C_CLK) */
#define	CMUX_SDm			2	/* mSDeMMC (GPIOA[4]:CMD, GPIOA[5]:CLK) */
#define	CMUX_SDs			3	/* sSDIO (GPIOA[4]:CMD, GPIOA[5]:CLK) */
#define	CMUX_UART1c			4	/* UART1 (GPIOA[4]:RTS, GPIOA[5]CTS) */
#define	CMUX_I2Cm			5	/* I2C master (GPIOA[4]:I2C_SDA, GPIO[5]:I2C_CLK) */
#define	CMUX_UART1d			6	/* UART1 (GPIOA[4]:TXD, GPIOA[5]:RXD) */
#define	CMUX_I2S			7	/* I2S (GPIOA[4]:I2S_BCLK, GPIOA[5]:I2S_MCLK) */
#define	CMUX_GPIO			8	/* GPIOA[5:4] */
#define	CMUX_GPIOALT   		9	/* GPIOA[5:4] */

// DMUX
#define	DMUX_SPIs			1	/* SPIs (GPIOA[6]:CSB, GPIOA[7]:CLK) */
#define	DMUX_SDm			2	/* mSDeMMC (GPIOA[6]:D3, GPIOA[7]:D2) */
#define	DMUX_SDs			3	/* sSDIO (GPIOA[6]:D3, GPIOA[7]:D2) */
#define	DMUX_UART1d			4	/* UART1 (GPIOA[6]:TXD, GPIOA[7]:RXD) */
#define	DMUX_I2Cs			5	/* I2C slave (GPIOA[6]:I2C_SDA, GPIOA[7]:I2C_CLK) */
#define	DMUX_SPIm			6	/* SPIm (GPIOA[6]:CSB, GPIOA[7]:CLK) */
#define	DMUX_I2S			7	/* I2S (GPIOA[6]:I2S_SDO, GPIOA[7]:I2S_LRCK) */
#define	DMUX_GPIO			8	/* GPIOA[7:6] */
#define	DMUX_GPIOALT   		9	/* GPIOA[7:6] */

// EMUX
#define	EMUX_5GC			0	/* 5G control[5:4]  */
#define	EMUX_SPIs			1	/* SPIs (GPIOA[8]:MISO, GPIOA[9]:MOSI) */
#define	EMUX_SDm			2	/* mSDeMMC (GPIOA[8]:D1, GPIOA[9]:D0) */
#define	EMUX_SDs			3	/* sSDIO (GPIOA[8]:D1, GPIOA[9]:D0) */
#define	EMUX_I2Cm			4	/* I2C master (GPIOA[8]:I2C_SDA, GPIOA[9]:I2C_CLK) */
#define	EMUX_BT				5	/* GPIOA[8]:BT_sig0, GPIOA[9]:BT_sig1 */
#define	EMUX_SPIm			6	/* SPIm (GPIOA[8]:IO0, GPIOA[9]:IO1) */
#define	EMUX_I2S			7	/* I2S (GPIOA[8]:I2S_BCLK, GPIOA[9]:I2S_MCLK) */
#define	EMUX_GPIO			8	/* GPIOA[9:8] */
#define	EMUX_GPIOALT   		9	/* GPIOA[9:8] */

// FMUX
#define	FMUX_GPIOBT			0	/* GPIOA[10]:BT_sig2, GPIOA[11] */
#define	FMUX_GPIOI2S   		1	/* GPIOA[10]:I2C_CLK_in, GPIOA[11] */
#define	FMUX_GPIOSDm		2	/* GPIOA[10]:mSDeMMC_WRP, GPIOA[11] */
#define	FMUX_SPIs			3	/* SPIs (GPIOA[10]:MISO, GPIOA[11]:MOSI) */
#define	FMUX_UART2			4	/* UART2 (GPIOA[10]:TXD, GPIOA[11]:RXD) */
#define	FMUX_SPIm			5	/* SPIm (GPIOA[10]:IO2, GPIOA[11]:IO3) */
#define	FMUX_GPIO			6	/* GPIOA[11:10] */
#define	FMUX_GPIOALT   		7	/* GPIOA[11:10] */

// IMUX
#define	IMUX_UART2I2S      	0	/* UART2 (GPIOA[12]:TXD, GPIOA[13]:RXD), GPIOA[14]:I2S_CLK */
#define	IMUX_BT            	1	/* BT_sig[2:0] */
#define	IMUX_DSYS   	   	2	/* D_SYS_CLK,D_SYS_OUT[3:2] */
#define	IMUX_DSYSCLKGPIO   	3	/* GPIOA[14:12] */

// JMUX
#define	JMUX_UART2I2S		0	/* UART2 (GPIOA[12]:TXD, GPIOA[13]:RXD), GPIOA[14]:I2S_CLK */
#define	JMUX_BT				1	/* BT_sig[2:0] */
#define	JMUX_DSYS			2	/* D_SYS_CLK,D_SYS_OUT[3:2] */
#define	JMUX_DSYSCLKGPIO	3	/* GPIOA[14:12] */

// KMUX
#define	KMUX_UART2I2S		0	/* UART2 (GPIOA[12]:TXD, GPIOA[13]:RXD), GPIOA[14]:I2S_CLK */
#define	KMUX_BT				1	/* BT_sig[2:0] */
#define	KMUX_DSYS			2	/* D_SYS_CLK,D_SYS_OUT[3:2] */
#define	KMUX_DSYSCLKGPIO	3	/* GPIOA[14:12] */

// HMUX
#define	HMUX_JTAG			0	/* TMS,TCLK	*/
#define	HMUX_D_SYS_OUT		1	/* D_SYS_OUT[1:0]	*/
#define	HMUX_ADGPIO			2	/* TMS:unusable TCLK:GPIO[15] */
#define	HMUX_ADGPIOALT		3	/* TMS:unusable TCLK:GPIO[15] 	*/




// LMUX
#define LMUX_QSPI			0	/* QSPI(3:0) */
#define LMUX_sSPI			1	/* sSPI(3:0) */
#define LMUX_I2S			2	/* I2S(0:3) */
#define LMUX_sSDIO			3	/* sSDIO(0:3) */
#define LMUX_GPIO			4	/* GPIOC[12:9] */
#define LMUX_GPIOALT		5	/* GPIOC[12:9] */

// MMUX
#define MMUX_QSPI			0    /* QSPI(4:5) F_IO[3:2] */
#define MMUX_UART2			1    /* UART2(RXD,TXD) */
#define MMUX_sSDIO			3    /* sSDIO(4:5) */
#define MMUX_GPIO			4	 /* GPIOC[14:13] */
#define MMUX_GPIOALT		5	 /* GPIOC[14:13] */

// NMUX
#define NMUX_mSPI			0    /* sSPI */
#define NMUX_GPIO			2    /* GPIOB[3:0] */
#define NMUX_GPIOALT		3    /* GPIOB[3:0] */


// PMUX
#define PMUX_mSPI			0    /* I2S */
#define PMUX_5GC			1    /* 5GC_Sig[5:2] */
#define PMUX_GPIO			2    /* GPIOB[7:4] */
#define PMUX_GPIOALT		3    /* GPIOB[7:4] */

// QMUX
#define QMUX_I2S			0    /* I2S */
#define QMUX_QSPI			1    /* H_SPI_DIO[7:4] */
#define QMUX_GPIO			2    /* GPIOB[11:8] */

// RMUX
#define RMUX_UART1			0    /* UART1(TXD,RXD) */
#define RMUX_mI2C			1    /* I2C master */
#define RMUX_GPIO			2    /* GPIOC[1:0] */

// SMUX
#define SMUX_UART1			0    /* UART1(RTS,CTS) */
#define SMUX_5GC			1    /* 5GC_Sig[1:0] */
#define SMUX_GPIO			2    /* GPIOC[3:2] */

// TMUX
#define TMUX_GPIOI2S	   	0    /* GPIOC[4],I2S_CLK */
#define TMUX_UART2			1    /* UART2(RTS,CTS) */
#define TMUX_QSPI			2    /* E_SPI_IO[3:2] */
#define TMUX_GPIO			3    /* GPIOC[5:4] */

// UMUX
#define UMUX_JTAG			0    /* TDI,TDO,nTRST */
#define UMUX_UART2GPIO	   	1    /* UART2(TXD,RXD),GPIOC[8] */
#define UMUX_GPIO			2    /* GPIOC[8:6] */



/******************************************************************************
 *
 *  functions
 *
 ******************************************************************************/

#define	da16x_io_volt33(...)
#define	da16x_io_pinmux(...)
#define	da16x_io_get_pinmux(...)		0

typedef struct
{
    UINT32 FSEL_GPIO1;
    UINT32 FSEL_GPIO2;
} DA16X_IO_ALL_PINMUX;

typedef enum {
	PULL_DOWN      = 0,
    PULL_UP,
    HIGH_Z
} PULL_TYPE;

/**
 ****************************************************************************************
 * @brief This function configures the pin pull up/down.
 * @param[in] gpio GPIO port (GPIO_UNIT_A or GPIO_UNIT_B or GPIO_UNIT_C)
 * @param[in] pin The pin want to set.
 * @param[in] type The pull condition to set.
 * @return void
 *
 ****************************************************************************************
 */
void _da16x_gpio_set_pull(GPIO_UNIT_TYPE gpio, UINT16 pin, PULL_TYPE type);

/**
 ****************************************************************************************
 * @brief This function configures the pin mux.
 * @param[in] mux the pin mux name
 * @param[in] config the function name
 * @return True
 *
 ****************************************************************************************
 */
UINT32 _da16x_io_pinmux(UINT32 mux, UINT32 config);

/**
 ****************************************************************************************
 * @brief This function gets the pin mux value.
 * @param[in] mux the pin mux name
 * @return True
 *
 ****************************************************************************************
 */
UINT32 _da16x_io_get_pinmux(UINT32 mux);

void _da16x_io_set_all_pinmux(DA16X_IO_ALL_PINMUX * pinmux);
void _da16x_io_get_all_pinmux(DA16X_IO_ALL_PINMUX * pinmux);

/**
 ****************************************************************************************
 * @brief To reduce the sleep current, this function must be used to register external pull-up register.
 * @param[in] port_num the GPIO port name
 * @param[in] pinnum the GPIO pin name
 *
 ****************************************************************************************
 */
void SAVE_PULLUP_PINS_INFO(UINT32 port_num, UINT32 pinnum);

/**
 ****************************************************************************************
 * @brief To reduce the sleep current, this function have to use internal pull-up register.
 * @param[in] port_num the GPIO port name
 * @param[in] pinnum the GPIO pin name
 *
 ****************************************************************************************
 */
void SAVE_INTERNAL_PULLUP_PINS_INFO(UINT32 port_num, UINT32 pinnum);

void CHANGE_DIGITAL_PAD_TO_CMOS_TYPE(UINT32 gpio_port, UINT32 gpio_num);

/**
 ****************************************************************************************
 * @brief This function initializes the RTC_GPO pin.
 * @param[in] mode 0: Auto mode (output high in power on state and output low in sleep state)
 * 			  mode 1: Manual mode
 *
 ****************************************************************************************
 */
void RTC_GPO_OUT_INIT(UINT8 mode);

/**
 ****************************************************************************************
 * @brief Set RTC_GPO output level.
 * @param[in] value of GPO 0: low, 1: high
  *
 ****************************************************************************************
 */
void RTC_GPO_OUT_CONTROL(UINT8 value);

#endif	/*__da16200_ioconfig_h__*/

/* EOF */
