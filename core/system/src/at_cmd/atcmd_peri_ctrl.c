/**
 ****************************************************************************************
 *
 * @file atcmd_peri_ctrl.c
 *
 * @brief AT-Command Peripheral device control
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

#include "sdk_type.h"
#include "da16x_system.h"
#include "atcmd.h"
#include "driver.h"
#include "clib.h"
#include "pwm.h"
#include "sys_i2c.h"
#include "da16200_regs.h"
#include "da16200_ioconfig.h"
#include "adc.h"
#include "atcmd_peri_ctrl.h"

#if defined (__SUPPORT_PERI_CTRL__)	/////////////////////////////////

#define UNUSED_PARAM(x) (void)(x)

// LED -------------------------------------------------------------------------
#define LED_PINS 	(GPIO_PIN6 | GPIO_PIN7 | GPIO_PIN8)

enum {
	EVB_LED_OFF = 0,
	EVB_LED_ON
};

HANDLE gpio_led = NULL;

static int led_gpio_init(int default_status)
{
	UINT32 pin;
	UINT16 write_data;

	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	/* UMUX to GPIOC[8:6] */
	_da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);
	
	/* GPIOC port init */
	gpio_led = GPIO_CREATE(GPIO_UNIT_C);
	GPIO_INIT(gpio_led);

	/* GPIOC6/7/8 output configuration */
	pin = LED_PINS;
	GPIO_IOCTL(gpio_led, GPIO_SET_OUTPUT, &pin);

	if (default_status == EVB_LED_ON)
	{
		/* GPIOC6/7/8 to high */
		write_data = LED_PINS;
		GPIO_WRITE(gpio_led, write_data, &write_data, sizeof(UINT16));
	}
	else
	{
		/* GPIOC6/7/8 to low */
		write_data = 0;
		GPIO_WRITE(gpio_led, write_data, &write_data, sizeof(UINT16));
	}

	return result;
}

static int led_gpio_control(unsigned int led, int onoff)
{
	UINT16 pin;
	UINT16 write_data;

	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	if (gpio_led == NULL) 
	{
		PRINTF("gpio_led handle is NULL. Call AT+LEDINIT command first.\n");
		return AT_CMD_ERR_UNKNOWN;
	}
		
	/* GPIOC6/7/8 output configuration */
	if (led == 1)
		pin = GPIO_PIN6;
	else if (led == 2)
		pin = GPIO_PIN7;
	else if (led == 3)
		pin = GPIO_PIN8;
	else 
	{
		PRINTF("Error: invalid led number.\n");
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}
	
	if (onoff == EVB_LED_ON)
	{
		/* GPIOC6/7/8 to high */
		write_data = pin;
		GPIO_WRITE(gpio_led, write_data, &write_data, sizeof(UINT16));
	}
	else
	{
		/* GPIOC6/7/8 to low*/
		write_data = 0;
		GPIO_WRITE(gpio_led, pin, &write_data, sizeof(UINT16));
	}

	return result;
}

int atcmd_led_init(int argc, char *argv[])
{
	UNUSED_PARAM(argc);
	UNUSED_PARAM(argv);
	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	/* LED1/2/3 init - GPIOC6/7/8 */
	result = led_gpio_init(EVB_LED_ON);

	return result;
}

int atcmd_led_ctrl(int argc, char *argv[])
{
	UNUSED_PARAM(argc);
	unsigned int led = 0;
	int onoff = 0;
	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	led = ctoi(argv[1]);
	if ( led < 1 || led > 3)
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	
	if ( (strcasecmp(argv[2], "ON") == 0) )
		onoff = EVB_LED_ON;
	else if ( (strcasecmp(argv[2], "OFF") == 0) )
			onoff = EVB_LED_OFF;
	else
		return AT_CMD_ERR_WRONG_ARGUMENTS;

	result = led_gpio_control(led, onoff);

	return result;
}

// I2C -------------------------------------------------------------------------
static 	HANDLE i2c_handle;

static int i2c_read(UINT8 slv, UINT8 reg, UINT8 count, UINT8 *buf);
static int i2c_write(UINT8 slv, UINT8 reg, UINT8 count, UINT8 *buf);

#define	AT_I2C_LENGTH_FOR_BYTE_ADDRESS	(1)
#define	AT_I2C_LENGTH_FOR_WORD_ADDRESS	(2)
#define	AT_I2C_DATA_LENGTH				(32)
#define	AT_I2C_FIRST_WORD_ADDRESS		(0x00)
#define	AT_I2C_SECOND_WORD_ADDRESS		(0x00)
#define	AT_I2C_ERROR_DATA_CHECK			(0x8)

static int i2c_board_init(void)
{
	/* GPIOA_8: SDA, GPIOA_9: SCL */
	_da16x_io_pinmux(PIN_EMUX, EMUX_I2Cm);

	return TRUE;
}

static int i2c_read(UINT8 slv, UINT8 reg, UINT8 count, UINT8 *buf)
{
	UINT8 *i2c_data;
	int status;
	
	if (count < 1 || buf == NULL)
	{
		PRINTF("Error: Invalid parameters.\n");
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}

	i2c_data = APP_MALLOC((size_t)(AT_I2C_LENGTH_FOR_BYTE_ADDRESS + count));
	if (i2c_data == NULL)
	{
		PRINTF("Error: Failed to allocate memory\n");
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}

	memset(i2c_data, 0, (size_t)(AT_I2C_LENGTH_FOR_BYTE_ADDRESS + count));
	i2c_data[0] = (UINT8)(reg) & 0xFF;

	/* Set slave address */
	DRV_I2C_IOCTL(i2c_handle, I2C_SET_CHIPADDR, &slv);

	status = DRV_I2C_READ(i2c_handle, i2c_data, count, 
						  AT_I2C_LENGTH_FOR_BYTE_ADDRESS, 0);
	if (status == FALSE)
	{
		PRINTF("Error: Failed to read i2c data\n");
		APP_FREE(i2c_data);

		return AT_CMD_ERR_TIMEOUT;
	}

	memcpy(buf, i2c_data, (size_t)count);

	APP_FREE(i2c_data);

	return AT_CMD_ERR_CMD_OK;
}

static int i2c_write(UINT8 slv, UINT8 reg, UINT8 count, UINT8 *buf)
{
	UINT8 *i2c_data;
	int status = 0;
	
	if (count < 1 || buf == NULL)
	{
		PRINTF("Error: Invalid parameters.\n");
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}

	i2c_data = APP_MALLOC((size_t)(AT_I2C_LENGTH_FOR_BYTE_ADDRESS + count));
	if (i2c_data == NULL)
	{
		PRINTF("Error: Failed to allocate memory\n");
		return AT_CMD_ERR_WRONG_ARGUMENTS;
	}

	memset(i2c_data, 0, (size_t)(AT_I2C_LENGTH_FOR_BYTE_ADDRESS + count));

	i2c_data[0] = (UINT8)(reg) & 0xFF;
	memcpy(&i2c_data[1], buf, (size_t)count);

	/* Set slave address */
	DRV_I2C_IOCTL(i2c_handle, I2C_SET_CHIPADDR, &slv);

	status = DRV_I2C_WRITE(i2c_handle, i2c_data, 
			       		   (UINT32)(AT_I2C_LENGTH_FOR_BYTE_ADDRESS + count), 1, 0);
	if (status != TRUE)
	{
		PRINTF("Error: Failed to write i2c data\n");
		return AT_CMD_ERR_TIMEOUT;
	}

	APP_FREE(i2c_data);

	return AT_CMD_ERR_CMD_OK;
}

int i2c_init(void)
{
	int status;
	/* I2C Working Clock [KHz] */
	UINT32 i2c_clock = 400;
	/* PIN configuration as I2C master */
	i2c_board_init();

	/* Need to handle I2C clock gating */
	DA16X_CLOCK_SCGATE->Off_DAPB_I2CM = 0;
	DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

	i2c_handle = DRV_I2C_CREATE(i2c_0);

	/* Initialization I2C Device */
	status = DRV_I2C_INIT(i2c_handle);
	if (status == FALSE) 
	{
		PRINTF("Error: Failed I2C initialization\n");
		return -1;
	}

	/* Set I2C Working Clock. Unit = KHz */
	DRV_I2C_IOCTL(i2c_handle, I2C_SET_CLOCK, &i2c_clock);

	return 0;
}

#ifdef __NOT_USED__ 
void cmd_user_i2c_read(int argc, char *argv[])
{
	UINT8 slv_addr;
	UINT8 read_data;
	UINT8 reg_addr;
	int status;

	/* i2c_read slave_address(8-bit) read_register */
	if (argc < 3) {
		PRINTF("Error: Invalid parameter\n");
		return;
	}

	slv_addr = strtoul(argv[1], NULL, 16);
	reg_addr = strtoul(argv[2], NULL, 16);
	status = i2c_read(slv_addr, reg_addr, 1, &read_data);

	if (status)
	{
		PRINTF("\t Error: i2c_read\n");
	}
	else
	{
		PRINTF("\t READ >> Slave: 0x%x, Addr: 0x%x, Data: 0x%x\n", 
				slv_addr, reg_addr, read_data);
	}
}

int cmd_user_i2c_write(UINT8 slv_addr, UINT8 reg_addr, UINT8 value)
{
	int status;

	status = i2c_write(slv_addr, reg_addr, 1, &value);

	if (status != TRUE)
	{
		PRINTF("\t Error: i2c_write, status(%d)\n", status);
		return AT_CMD_ERR_UNKNOWN;
	}
	else
	{
		PRINTF("\t WRITE >> Slave: 0x%x, Addr: 0x%x, Data: 0x%x\n", 
				slv_addr, reg_addr, value);
		return AT_CMD_ERR_CMD_OK;
	}
}
#endif

int atcmd_i2c(int argc, char *argv[])
{
	UNUSED_PARAM(argc);
	UINT8 slv_addr;
	UINT8 reg_addr;
	UINT8 len;
	UINT8 *buf = NULL;
	char temp[2] = {0, }, *outbuf = NULL;
	unsigned int i;
	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	if (strcasecmp(argv[0] + 6, "INIT") == 0)
	{
		if (i2c_init())
		{
			result = AT_CMD_ERR_UNKNOWN;			
		}
	}
	else if ( (strcasecmp(argv[0] + 6, "READ") == 0))
	{
		slv_addr = (UINT8)htoi(argv[1]);
		reg_addr = (UINT8)htoi(argv[2]);
		len	= (UINT8)ctoi(argv[3]);

		buf	= (unsigned char *)ATCMD_MALLOC((size_t)len);
		if (buf == NULL)
		{
			return AT_CMD_ERR_UNKNOWN_CMD;
		}

		result = i2c_read(slv_addr, reg_addr, len, buf);
		if (result != AT_CMD_ERR_CMD_OK)
		{
			PRINTF("i2c read error ret : %d\r\n", result);
			PRINTF_ATCMD("\r\nERROR\r\n");
		}
		else
		{
			outbuf = (char *)ATCMD_MALLOC((size_t)(len*2));
			if (outbuf == NULL)
			{
				ATCMD_FREE(buf);
				return AT_CMD_ERR_UNKNOWN_CMD;
			}
			memset(outbuf, 0, len);

			for (i = 0; i < len ; i++)
				sprintf(outbuf + i * 2, "%02x", buf[i]);

			PRINTF_ATCMD("\r\n%s\r\n", outbuf);
			ATCMD_FREE(outbuf);
		}

		ATCMD_FREE(buf);
	}
	else if ( (strcasecmp(argv[0] + 6, "WRITE") == 0))
	{
	
		/* AT+I2CWRITE=<slave_addr>,<reg_addr>,<len>,<data>,...
		 * AT+I2CWRITE=D0,10,3,676869
		 */
		slv_addr = (UINT8)strtoul(argv[1], NULL, 16);
		reg_addr = (UINT8)strtoul(argv[2], NULL, 16);
		len	= (UINT8)ctoi(argv[3]);

		if (len > 10)
		{
			PRINTF("Error: length shuld be <10\n");
			return AT_CMD_ERR_WRONG_ARGUMENTS;
		}

		if (argv[4] == NULL || strlen(argv[4])>20)
		{
			PRINTF("Error: length shuld be <10\n");
			return AT_CMD_ERR_WRONG_ARGUMENTS;
		}

		buf = APP_MALLOC(len);
		if (buf == NULL)
		{
			PRINTF("Error: Failed to allocate memory\n");
			return AT_CMD_ERR_WRONG_ARGUMENTS;
		}
		
		memset(buf, 0, len);

		for(i=0;i<len;i++){
			memset(temp, 0, 2);
			memcpy(temp, argv[4] + (i*2), 2);
			buf[i]  = (UINT8)strtoul(temp, NULL, 16);
		}

		result = i2c_write(slv_addr, reg_addr, len, buf);
		
		if (result != AT_CMD_ERR_CMD_OK)
		{
			PRINTF("i2c write error, ret : %d\r\n", result);
			PRINTF_ATCMD("\r\nERROR\r\n");
		}
		
		APP_FREE(buf);
	}

	return result;
}

// PWM -------------------------------------------------------------------------
// GPIOA10
static void pwm_board_init(void)
{
	HANDLE	gpio_pwm = NULL;

	/* GPIOA[11:10] */
	_da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);

	DA16X_CLOCK_SCGATE->Off_CAPB_PWM = 0;
	
	gpio_pwm = GPIO_CREATE(GPIO_UNIT_A);
	GPIO_INIT(gpio_pwm);
	GPIO_SET_ALT_FUNC(gpio_pwm, GPIO_ALT_FUNC_PWM_OUT0, GPIO_ALT_FUNC_GPIO10);
}

int atcmd_pwm(int argc, char *argv[])
{
	static HANDLE	pwm[1];
	UINT32	dev, period, duty_percent, mode_cyc;
	
	atcmd_error_code result = AT_CMD_ERR_CMD_OK;

	if ( (strcasecmp(argv[0] + 6, "INIT") == 0)  )
	{
		pwm_board_init();
		pwm[0] = DRV_PWM_CREATE(pwm_0);
		DRV_PWM_INIT(pwm[0]);
	}
	else if ( (strcasecmp(argv[0] + 6, "START") == 0))
	{
		if (argc == 4)
		{
			dev = ctoi(argv[1]);
			period = ctoi(argv[2]);
			duty_percent = ctoi(argv[3]);
			DRV_PWM_START(pwm[dev], period, duty_percent, PWM_DRV_MODE_US);
		}
		else if (argc == 5)
		{
			dev = ctoi(argv[1]);
			period = ctoi(argv[2]);
			duty_percent = ctoi(argv[3]);
			mode_cyc = ctoi(argv[4]);
			DRV_PWM_START(pwm[dev], period, duty_percent, mode_cyc);
		}
	}
	else if ( (strcasecmp(argv[0] + 6, "STOP") == 0))
	{
		dev = ctoi(argv[1]);
		DRV_PWM_STOP(pwm[dev], 0);
	}

	return result;
}

// ADC -------------------------------------------------------------------------
#define	DA16200_ADC_NO_TIMESTAMP	(0)
#define	DA16200_ADC_TIMEOUT_DMA		(200)	// 200 -> 2 sec
#define DA16200_ADC_SEL_ADC_12		(12)
#define DA16200_ADC_DIVIDER_12		(4)
#define DA16200_ADC_NUM_READ		(16)

#ifdef __NOT_USED__ 
static unsigned short convert_adc_to_mv(unsigned short adc_value)
{
	unsigned long temp = 0;

	temp = (unsigned long)(1400 * (adc_value >> 4)) / 4095;
	return (unsigned short)temp;
}
#endif

int atcmd_adc(int argc, char *argv[])
{
	static HANDLE			adc[1] = {NULL};
	static unsigned int		prev_len = 0;
	unsigned int			channel, enable, i, len;
	atcmd_error_code 		status = AT_CMD_ERR_CMD_OK;
	static unsigned short	*data = NULL;
	unsigned int			div12;

	/* ex
			AT+ADCINIT
			
			AT+ADCCHEN=0,12
			AT+ADCCHEN=1,12
			AT+ADCCHEN=2,12
			AT+ADCCHEN=3,12

			AT+ADCSTART=1
			
			AT+ADCREAD=0,16		// 0~3 : channel(GPIOA0~3), 16 : sample count
			AT+ADCREAD=1,16
			AT+ADCREAD=2,16
			AT+ADCREAD=3,16
			...
			
			AT+ADCSTOP
	*/

	if (strcasecmp(argv[0] + 6, "READ") == 0)
	{
		if (argc == 3)
		{
			channel = ctoi(argv[1]);	// 0  -> channel
			len = ctoi(argv[2]);		// 16 -> sample count to read

			if (len > prev_len)
			{
				if (data != NULL)
				{
					ATCMD_FREE(data);
				}
				data = (unsigned short *)ATCMD_MALLOC(len * 2);
			}

			DRIVER_MEMSET(data, 0, len * 2);

			// 1MHz 16 sample rate => 1/1MHz = 1us x 16 sp => 16 us
			DRV_ADC_READ_DMA(adc[0], channel, data, len * 2, DA16200_ADC_TIMEOUT_DMA, 0);

			PRINTF_ATCMD("\r\n[ ");
			for (i=0; i<len; i++)
				PRINTF_ATCMD("%4d ", (unsigned short)data[i]>>4);	// 12bit adc
			//convert_adc_to_mv((unsigned short)data[i]>>4);
			PRINTF_ATCMD(" ]\r\n");
		}
		else
		{
			status = AT_CMD_ERR_INSUFFICENT_ARGS;
		}
	}
	else if (strcasecmp(argv[0] + 6, "START") == 0)
	{
		if (argc == 2)
		{
			div12 = htoi(argv[1]);

			/* divider adc sampling rate : 1MHz / (div12(1)+1) = 500khz */
			DRV_ADC_START(adc[0], div12, 0);
		}
		else
		{
			status = AT_CMD_ERR_INSUFFICENT_ARGS;
		}
	}
	else if ( (strcasecmp(argv[0] + 6, "CHEN") == 0)	)
	{
		if (argc == 3)
		{
			channel = ctoi(argv[1]);
			enable = ctoi(argv[2]);
			//enable = DA16200_ADC_SEL_ADC_12;	// always 12 in DA16200, 0 means channel disable
			DRV_ADC_ENABLE_CHANNEL(adc[0], channel, enable, 0);
		}
		else
		{
			status = AT_CMD_ERR_INSUFFICENT_ARGS;
		}
	}
	else if ( (strcasecmp(argv[0] + 6, "STOP") == 0)	)
	{
		if (argc == 1)
		{
			// AT+ADCSTOP
			if (data != NULL)
			{
				ATCMD_FREE(data);
			}

			DRV_ADC_STOP(adc[0], 0);
			DRV_ADC_CLOSE(adc[0]);
		}
		else
		{
			status = AT_CMD_ERR_INSUFFICENT_ARGS;
		}
	}
	else if ( (strcasecmp(argv[0] + 6, "INIT") == 0))
	{
		if (argc == 1)
		{
			// AT+ADCINIT
			adc[0] = DRV_ADC_CREATE(0);

			// board init - Analog Pad
			_da16x_io_pinmux(PIN_AMUX, AMUX_AD12);
			_da16x_io_pinmux(PIN_BMUX, BMUX_AD12);

			DRV_ADC_INIT(adc[0], DA16200_ADC_NO_TIMESTAMP);
		}
		else
		{
			status = AT_CMD_ERR_INSUFFICENT_ARGS;
		}
	}

	return status;
}

#endif	/* __SUPPORT_PERI_CTRL__ */ /////////////////////////////////

/* EOF */
