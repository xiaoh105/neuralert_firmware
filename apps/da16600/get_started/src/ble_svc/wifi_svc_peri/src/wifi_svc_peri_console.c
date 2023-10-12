/**
 ****************************************************************************************
 *
 * @file wifi_svc_console.c
 *
 * @brief Basic console user interface of the host application.
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

#if defined (__BLE_PERI_WIFI_SVC_PERIPHERAL__)

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../../include/da14585_config.h"
#include "../../include/queue.h"
#include "../../include/console.h"

#include "da16_gtl_features.h"
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif
#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif
#include "../../include/app.h"

#include "proxr.h"
#include "timer0.h"
#include "timer0_2.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "command.h"
#include "user_dpm_api.h"
#include "ota_update.h"
#include "clib.h"

#ifdef __FREERTOS__
#include "msg_queue.h"
#endif

#define	CMD_OPTION_S0(...)	__VA_ARGS__
#define CMD_OPTION_0(...)	__VA_ARGS__

#ifndef __FREERTOS__
extern SemaphoreHandle_t ConsoleQueueMut;
#endif

//-----------------------------------------------------------------------
// Command Function-List
//-----------------------------------------------------------------------
//extern void cmd_ble_reset(int argc, char *argv[]);
#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
extern void cmd_ble_force_exception(int argc, char *argv[]);
extern void ConsoleSendBleForceException(void);
#endif // __TEST_FORCE_EXCEPTION_ON_BLE__
#if defined (__EXCEPTION_RST_EMUL__)
extern void cmd_wifi_force_exception(int argc, char *argv[]);
#endif
extern void cmd_gapm_reset(int argc, char *argv[]);
extern void cmd_ble_fw_ver(int argc, char *argv[]);
extern void ConsoleSendGapmReset(void);
extern void ConsoleSendGetBleFwVer(void);
#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)
extern void cmd_enter_sleep2(int argc, char *argv[]);
#endif
#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)
extern void cmd_enter_sleep3(int argc, char *argv[]);
#endif

extern void cmd_ble_disconnect(int argc, char *argv[]);
extern void cmd_ble_start_adv(int argc, char *argv[]);
extern void cmd_ble_stop_adv(int argc, char *argv[]);

extern void cmd_peri_sample(int argc, char *argv[]);

#if defined (__OTA_TEST_CMD__)
extern void cmd_ota_test(int argc, char *argv[]);
#endif

extern void cmd_get_local_dev_info(int argc, char *argv[]);

//-----------------------------------------------------------------------
// Command BLE-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_ble_list[] =
{
	{ "ble",		CMD_MY_NODE,	cmd_ble_list,	NULL,	    	"BLE application command "},  // Head
	CMD_OPTION_S0(	{ "-------",		CMD_FUNC_NODE,	NULL,		NULL,	"--------------------------------"}, )
#if defined (__EXCEPTION_RST_EMUL__)	
	CMD_OPTION_S0(  { "wifi_force_excep",     CMD_FUNC_NODE,  NULL,       &cmd_wifi_force_exception,	"force undef_instr exception on wifi"}, )
#endif // __EXCEPTION_RST_EMUL__
#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)	
	CMD_OPTION_S0(  { "ble_force_excep",     CMD_FUNC_NODE,  NULL,       &cmd_ble_force_exception,	"Force UNDEF_INSTR exception on ble"}, )	
#endif // __TEST_FORCE_EXCEPTION_ON_BLE__
#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)	
	CMD_OPTION_S0(	{ "enter_sleep2",	 CMD_FUNC_NODE,  NULL,		 &cmd_enter_sleep2,	"Enter sleep2"}, )
#endif // __TEST_FORCE_SLEEP2_WITH_TIMER__
#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)	
	CMD_OPTION_S0(	{ "enter_sleep3",	 CMD_FUNC_NODE,  NULL,		 &cmd_enter_sleep3, "Enter sleep3"}, )
#endif // __FOR_DPM_SLEEP_CURRENT_TEST__
	CMD_OPTION_S0(  { "ble_gapm_reset",     CMD_FUNC_NODE,  NULL,       &cmd_gapm_reset,    "Reset BLE GAPM"}, )
	CMD_OPTION_S0(  { "ble_fw_ver",     CMD_FUNC_NODE,  NULL,       &cmd_ble_fw_ver,    "Get BLE FW version"}, )	
#if defined (__OTA_TEST_CMD__)
	CMD_OPTION_S0(  { "ota_test",     CMD_FUNC_NODE,  NULL,       &cmd_ota_test,    "OTA test command"}, )
#endif // __OTA_TEST_CMD__
	CMD_OPTION_S0(  { "adv_start",     CMD_FUNC_NODE,  NULL,       &cmd_ble_start_adv,    "BLE ADV start"}, )	
	CMD_OPTION_S0(  { "adv_stop",     CMD_FUNC_NODE,  NULL,       &cmd_ble_stop_adv,    "BLE ADV stop"}, )	
	CMD_OPTION_S0(  { "disconnect",     CMD_FUNC_NODE,  NULL,       &cmd_ble_disconnect,    "Disconnect BLE connection"}, )	
	CMD_OPTION_S0(	{ "get_dev_info", 	CMD_FUNC_NODE,	NULL,		&cmd_get_local_dev_info,	"Get local device info command"}, ) 
	//CMD_OPTION_S0(  { "ble_reset",     CMD_FUNC_NODE,  NULL,       &cmd_ble_reset,    "reset DA14531"}, )
	CMD_OPTION_S0(  { "peri",     CMD_FUNC_NODE,  NULL,       		&cmd_peri_sample,    		"Run peripheral driver sample"}, )
	{ NULL, 		CMD_NULL_NODE,	NULL,		NULL,		NULL }			// Tail
};

void cmd_gapm_reset(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendGapmReset();
}

void cmd_ble_fw_ver(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendGetBleFwVer();
}

void cmd_ble_disconnect(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendDisconnnect();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	DA14531 Peripheral driver sample helper functions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define RET_QUIT	-99
#define RET_RESTART -98
#define RET_PROCEED -97

#define PERI_T0_GEN_TST_DUR_DEFAULT 10
#define PERI_T0_GEN_PWM_ON_COUNTER_DEF		20000
#define PERI_T0_GEN_PIN_DEFAULT 8

#define PERI_T0_BUZ_TST_DUR_DEFAULT 50
#define PERI_T0_BUZ_PWM0_PIN_DEFAULT 8
#define PERI_T0_BUZ_PWM1_PIN_DEFAULT 11
#define PERI_T0_BUZ_PWM_ON_COUNTER_DEF		1000
#define PERI_T0_BUZ_PWM_HIGH_COUNTER_DEF  	500
#define PERI_T0_BUZ_PWM_LOW_COUNTER_DEF 	200

#define PERI_T2_PWM2_PIN_DEFAULT 8
#define PERI_T2_PWM3_PIN_DEFAULT 11
#define PERI_T2_PWM4_PIN_DEFAULT 2
#define PERI_T2_PWM_FREQUENCY 500    // 500 Hz

#define STR_IN_CLK_DIV_FACTOR 	"TIM0_2_CLK_DIV_%d"
#define STR_TIM0_CLK_SRC_32K 	"TIM0_CLK_32K"
#define STR_TIM0_CLK_SRC_FAST 	"TIM0_CLK_FAST"
#define STR_TIM0_PWM_MODE_ONE 	"PWM_MODE_ONE"
#define STR_TIM0_PWM_MODE_TWO 	"PWM_MODE_CLOCK_DIV_BY_TWO"
#define STR_TIM0_CLK_DIV_BY_10 	"TIM0_CLK_DIV_BY_10"
#define STR_TIM0_CLK_DIV_NO    	"TIM0_CLK_NO_DIV"
#define STR_T2_HW_PAUSE_OFF		"TIM2_HW_PAUSE_OFF"
#define STR_T2_HW_PAUSE_ON  	"TIM2_HW_PAUSE_ON"
#define MAX_OPTION_STR_SIZE sizeof(STR_TIM0_PWM_MODE_TWO)
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#define STR_IMG_VER_NAME_SPI_FLASH	"6.0.14.1114.2.s"
#define STR_IMG_VER_NAME_QUAD_DEC	"6.0.14.1114.2.q"
#endif

#define RETURN_NORM_FONT() \
		do { \
            PRINTF(ANSI_NORMAL); \
			return; \
        } while(0)

extern int getNum(void);
extern int getStr(char *get_data, int get_len);
extern ble_img_hdr_all_t ble_fw_hdrs_all;

typedef enum 
{
	PERI_BLINKY,
	PERI_SYSTICK,
	PERI_TIMER0_GEN,
	PERI_TIMER0_BUZ,
	PERI_TIMER2_PWM,
	PERI_BATT_LVL,
	PERI_I2C_EEPROM,
	PERI_SPI_FLASH,
	PERI_QUAD_DEC,
#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
	PERI_GPIO_CTRL,
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
	PERI_NONE,
	PERI_ALL
} peri_sample_type_t;

typedef enum 
{
	ON_COUNTER,
	HIGH_COUNTER,
	LOW_COUNTER
} peri_pwm_conter_type_t;

typedef enum 
{
	PWM_0,
	PWM_1,
	PWM_2,
	PWM_3,
	PWM_4
} peri_gpio_pin_type_t;

char option_str[MAX_OPTION_STR_SIZE+1];

uint8_t avail_gpio_pins[] = {2, 8, 10, 11}; // available PINs in DA14531

void print_timer0_tst_dur(peri_sample_type_t sample_id);
void print_timer0_clk_div_def(peri_sample_type_t sample_id);
void print_pwm_reload(peri_sample_type_t sample_id,        peri_pwm_conter_type_t cnt_type);

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
uint8_t is_ble_img_compatible (peri_sample_type_t sample_id)
{
	uint8_t chk_result = FALSE;
	uint8_t cur_ble_ver[16] = {0,};

	// get the current ble img ver
	if (ble_fw_hdrs_all.active_img_bank == 1)
	{
		memcpy(cur_ble_ver, ble_fw_hdrs_all.fw1_hdr.version, sizeof(cur_ble_ver));
	}
	else
	{
		memcpy(cur_ble_ver, ble_fw_hdrs_all.fw2_hdr.version, sizeof(cur_ble_ver));
	}

	switch (sample_id)
	{
		case PERI_SPI_FLASH:
		{
			if (strcmp((char*)cur_ble_ver, STR_IMG_VER_NAME_SPI_FLASH) == 0)
			{
				chk_result = TRUE;
			}
			else
			{
				PRINTF("Please use a DA14531 img ver "ANSI_BOLD"6.0.14.1114.2.s"ANSI_NORMAL" \n");
			}
		} break;
		case PERI_QUAD_DEC:
		{
			if (strcmp((char*)cur_ble_ver, STR_IMG_VER_NAME_QUAD_DEC) == 0)
			{
				chk_result = TRUE;
			}
			else
			{
				PRINTF("Please use a DA14531 img ver "ANSI_BOLD"6.0.14.1114.2.q"ANSI_NORMAL" \n");
			}
		} break;
		default: // other samples
		{
			if ((strcmp((char*)cur_ble_ver, STR_IMG_VER_NAME_QUAD_DEC)  != 0) && 
				(strcmp((char*)cur_ble_ver, STR_IMG_VER_NAME_SPI_FLASH) != 0))
			{
				chk_result = TRUE;
			}
			else
			{
				PRINTF("Please use a DA14531 img ver "ANSI_BOLD"6.0.14.1114.2"ANSI_NORMAL" \n");
			}
		} break;
	}

	return chk_result;
}
#endif

uint8_t is_gpio_pin_valid(uint8_t gpio_pin)
{
	int i;
	for (i=0; i<(int)(sizeof(avail_gpio_pins)/sizeof(avail_gpio_pins[0])); i++)
	{
		DBG_LOW("i=%d\n", i);
		DBG_LOW("avail_gpio_pins[%d]=%d\n", i, avail_gpio_pins[i]);
		
		if(gpio_pin == avail_gpio_pins[i])
		{
			DBG_LOW("gpio pin avaliable!\n");
			return TRUE;
		}
	}
	return FALSE;
}

int16_t get_gpio_pin_def(peri_sample_type_t sample_id, 
								peri_gpio_pin_type_t pin_type)
{
	int input_num = PERI_T0_GEN_PIN_DEFAULT;
	
	switch(sample_id)
	{
		case PERI_TIMER0_BUZ:
		{
			input_num = (pin_type)? PERI_T0_BUZ_PWM1_PIN_DEFAULT:PERI_T0_BUZ_PWM0_PIN_DEFAULT;
		} break;
		
		case PERI_TIMER2_PWM:
		{
			switch(pin_type)
			{
				case PWM_2:
					input_num = PERI_T2_PWM2_PIN_DEFAULT;
					break;
				case PWM_3:
					input_num = PERI_T2_PWM3_PIN_DEFAULT;
					break;
				case PWM_4:
					input_num = PERI_T2_PWM4_PIN_DEFAULT;
					break;
				default:
					input_num = PERI_T2_PWM2_PIN_DEFAULT;
			}
		} break;
	
		default:
		{
			input_num = PERI_T0_GEN_PIN_DEFAULT;
		} break;
	}
	
	return input_num;
}

int16_t get_gpio_pin(peri_sample_type_t sample_id, peri_gpio_pin_type_t pin_type)
{
	int input_num;
	
	do
	{
		if ((sample_id == PERI_TIMER0_BUZ) || (sample_id == PERI_TIMER2_PWM))
		{
			PRINTF("\n"ANSI_BOLD"GPIO PIN number for PWM_%d () ?\n", pin_type);
		}
		else
		{
			PRINTF("\n"ANSI_BOLD"GPIO PIN number () ?\n");
		}

		PRINTF("\t" ANSI_BOLD "1." ANSI_NORMAL " p0_2\n");
		PRINTF("\t" ANSI_BOLD "2." ANSI_NORMAL " p0_8\n");
		PRINTF("\t" ANSI_BOLD "3." ANSI_NORMAL " p0_10\n");
		PRINTF("\t" ANSI_BOLD "4." ANSI_NORMAL " p0_11\n");

		input_num = get_gpio_pin_def(sample_id, pin_type);
		
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (p0_%d) : ", input_num);

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return get_gpio_pin_def(sample_id, pin_type);
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}		
		else if (input_num == 1)
		{
			return 2;
		}
		else if (input_num == 2)
		{
			return 8;
		}
		else if (input_num == 3)
		{
			return 10;
		}
		else if (input_num == 4)
		{
			return 11;
		}
		else
		{
			continue;
		}
	}
	while (1);
}

int16_t get_tst_dur_def(peri_sample_type_t sample_id)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		return PERI_T0_GEN_TST_DUR_DEFAULT;
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		return PERI_T0_BUZ_TST_DUR_DEFAULT;
	}
	else
	  	return PERI_T0_GEN_TST_DUR_DEFAULT;
}

int16_t get_test_duration(peri_sample_type_t sample_id)
{
	int input_num;
	
	do
	{
		print_timer0_tst_dur(sample_id);

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return get_tst_dur_def(sample_id);
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}		
		else
		{
			if (input_num > 30 && sample_id == PERI_TIMER0_GEN) {
				PRINTF("Invalid input, set to MAX(30)\n");
				input_num = 30;
			}
			if (input_num > 100 && sample_id == PERI_TIMER0_BUZ) {
				PRINTF("Invalid input, set to MAX(100)\n");
				input_num = 100;
			}
			return input_num;
		}
	}
	while (1);
}

int16_t get_in_clk_div_factor_def(peri_sample_type_t sample_id)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		return TIM0_2_CLK_DIV_8;
	}
	else
	{
		return TIM0_2_CLK_DIV_8;
	}
		
}

int16_t get_in_clk_div_factor(peri_sample_type_t sample_id)
{
	int input_num;
	
	do
	{
		PRINTF("\nTimer0/Timer2 input clock division factor ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_2_CLK_DIV_1\n", TIM0_2_CLK_DIV_1);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_2_CLK_DIV_2\n", TIM0_2_CLK_DIV_2);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_2_CLK_DIV_4\n", TIM0_2_CLK_DIV_4);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_2_CLK_DIV_8\n", TIM0_2_CLK_DIV_8);
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM0_2_CLK_DIV_8) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return get_in_clk_div_factor_def(sample_id);
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}		
		else if (input_num > TIM0_2_CLK_DIV_8 || input_num < TIM0_2_CLK_DIV_1)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int16_t get_timer0_clk_src()
{
	int input_num;
	
	do
	{
		PRINTF("\ntimer0 clock source ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_CLK_32K\n", TIM0_CLK_32K);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_CLK_FAST\n", TIM0_CLK_FAST);
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM0_CLK_FAST) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return TIM0_CLK_FAST;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else if (input_num > TIM0_CLK_FAST || input_num < TIM0_CLK_32K)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int16_t get_timer2_puase_by_hw()
{
	int input_num;
	
	do
	{
		PRINTF("\ntimer2 pause (by HW) ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM2_HW_PAUSE_OFF\n", TIM2_HW_PAUSE_OFF);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM2_HW_PAUSE_ON\n", TIM2_HW_PAUSE_ON);
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM2_HW_PAUSE_OFF) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return TIM2_HW_PAUSE_OFF;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else if (input_num > TIM2_HW_PAUSE_ON || input_num < TIM2_HW_PAUSE_OFF)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int32_t get_timer2_pwm_freq()
{
	int32_t input_num;
	
	do
	{
		PRINTF("\ntimer2 PWM Frequency ? (see range below)\n");
		PRINTF(ANSI_NORMAL"--------------------------------------------------------\n");
		PRINTF("- [2 Hz, 16 kHz], if Timer2 input clock frequency is 32 kHz,\n");
		PRINTF("- [123 Hz, 1 MHz], if Timer2 input clock frequency is 2 MHz,\n");
		PRINTF("- [245 Hz, 2 MHz], if Timer2 input clock frequency is 4 MHz,\n");
		PRINTF("- [489 Hz, 4 MHz], if Timer2 input clock frequency is 8 MHz,\n");
		PRINTF("- [977 Hz, 8 MHz], if Timer2 input clock frequency is 16 MHz\n");		
		PRINTF("--------------------------------------------------------\n");
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (500 Hz) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return PERI_T2_PWM_FREQUENCY;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}


int16_t get_timer2_dc_scale()
{
	int input_num;
	
	do
	{
		PRINTF("\ntimer2 duty cycle scale?\n");
		PRINTF("--------------------------------------------------------\n");
		PRINTF(" e.g.) \n");
		PRINTF("--------------------------------------------------------\n");
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM2_HW_PAUSE_OFF) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return TIM2_HW_PAUSE_OFF;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else if (input_num > TIM2_HW_PAUSE_ON || input_num < TIM2_HW_PAUSE_OFF)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}


int16_t get_timer0_pwm_mode()
{
	int input_num;
	
	do
	{
		PRINTF("\ntimer0 pwm mode ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " PWM_MODE_ONE\n", PWM_MODE_ONE);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " PWM_MODE_CLOCK_DIV_BY_TWO\n", 
													PWM_MODE_CLOCK_DIV_BY_TWO);
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (PWM_MODE_ONE) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return PWM_MODE_ONE;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else if (input_num > PWM_MODE_CLOCK_DIV_BY_TWO || input_num < PWM_MODE_ONE)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int16_t get_t0_clk_div_def(peri_sample_type_t sample_id)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		return TIM0_CLK_DIV_BY_10;
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		return TIM0_CLK_NO_DIV;
	}
	else
	{
	  	return TIM0_CLK_DIV_BY_10;
	}
}

int16_t get_timer0_clk_div(peri_sample_type_t sample_id)
{
	int input_num;
	
	do
	{
		PRINTF("\nTimer0 clock div ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_CLK_DIV_BY_10\n", 
													TIM0_CLK_DIV_BY_10);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " TIM0_CLK_NO_DIV\n", TIM0_CLK_NO_DIV);
		print_timer0_clk_div_def(sample_id);

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return get_t0_clk_div_def(sample_id);
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else if (input_num > TIM0_CLK_NO_DIV || input_num < TIM0_CLK_DIV_BY_10)
		{
			continue;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int16_t get_pwm_reload_val_def(peri_sample_type_t sample_id,
								      peri_pwm_conter_type_t cnt_type)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		return PERI_T0_GEN_PWM_ON_COUNTER_DEF;		
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		switch (cnt_type)
		{
			case ON_COUNTER:
			{
				return PERI_T0_BUZ_PWM_ON_COUNTER_DEF;
			}	break;
			case HIGH_COUNTER:
			{
				return PERI_T0_BUZ_PWM_HIGH_COUNTER_DEF;
			}	break;
			case LOW_COUNTER:
			{
				return PERI_T0_BUZ_PWM_LOW_COUNTER_DEF;
			}	break;
			default:
			  	return PERI_T0_BUZ_PWM_ON_COUNTER_DEF;
				break;
		}
	}
	else
	{
	  return PERI_T0_GEN_PWM_ON_COUNTER_DEF;
	}
}

int get_pwm_reload_val(peri_sample_type_t sample_id, 
							 peri_pwm_conter_type_t cnt_type)
{
	int input_num;
	
	do
	{
		print_pwm_reload(sample_id, cnt_type);

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return get_pwm_reload_val_def(sample_id, cnt_type);
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}


int get_quad_dec_evt_count(void)
{
	int input_num;
	
	do
	{
		PRINTF("\n"ANSI_NORMAL"Number of events before Quadrature Decoder interrupt ?\n");
		PRINTF(ANSI_BOLD "Type [ENTER] only for default (1) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return 1;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

int get_quad_dec_clk_divisor(void)
{
	int input_num;
	
	do
	{
		PRINTF("\n"ANSI_NORMAL"The clock divisor of the quadrature decoder ?\n");
		PRINTF(ANSI_BOLD "Type [ENTER] only for default (1) : ");

		input_num = getNum();

		if (input_num == -88)   // [ENTER] only
		{
			return 1;
		}
		else if (input_num == RET_QUIT) // [Q]uit
		{
			return RET_QUIT;
		}
		else
		{
			return input_num;
		}
	}
	while (1);
}

void show_all_conf_values_t0_buz(app_peri_timer0_buz_start_t* conf_data)
{
		PRINTF("\n--------------------------------------------------\n");
		PRINTF("\nConfiguration Summary\n");
		PRINTF("\n--------------------------------------------------\n"ANSI_NORMAL);
		PRINTF("pwm0_pin                                  = " ANSI_NORMAL "%d \n", conf_data->t0_pwm0_pin);
		PRINTF("pwm1_pin                                  = " ANSI_NORMAL "%d \n", conf_data->t0_pwm1_pin);
	    PRINTF("buzzer test counter                       = " ANSI_NORMAL "%d \n", conf_data->buz_tst_counter);
	
	sprintf(option_str, STR_IN_CLK_DIV_FACTOR, (1) << conf_data->t0_t2_in_clk_div_factor);
		PRINTF("Timer0/Timer2 input clock division factor = " ANSI_NORMAL "%s \n", option_str);
	
	if ((TIM0_CLK_SEL_t)conf_data->t0_clk_src == TIM0_CLK_32K)
		PRINTF("timer0 clock source                       = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_SRC_32K);
	else
		PRINTF("timer0 clock source                       = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_SRC_FAST);
	
	if ((PWM_MODE_t)conf_data->t0_pwm_mod == PWM_MODE_ONE)
		PRINTF("timer0 pwm mode                           = " ANSI_NORMAL "%s \n", STR_TIM0_PWM_MODE_ONE);
	else
		PRINTF("timer0 pwm mode                           = " ANSI_NORMAL "%s \n", STR_TIM0_PWM_MODE_TWO);
	
	if ((TIM0_CLK_DIV_t)conf_data->t0_clk_div == TIM0_CLK_DIV_BY_10)
		PRINTF("timer0 clock division                     = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_DIV_BY_10);
	else
		PRINTF("timer0 clock division                     = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_DIV_NO);

	
		PRINTF("timer0 ON COUTNER reload value            = " ANSI_NORMAL "%d \n", conf_data->t0_on_reld_val);
		PRINTF("timer0 M (HIGH) COUTNER reload value      = " ANSI_NORMAL "%d \n", conf_data->t0_high_reld_val);
		PRINTF("timer0 N (LOW) COUTNER reload value       = " ANSI_NORMAL "%d \n", conf_data->t0_low_reld_val);

		PRINTF("--------------------------------------------------\n");

}

void show_all_conf_values_t2_pwm(app_peri_timer2_pwm_start_t* conf_data)
{
		DA16X_UNUSED_ARG(conf_data);

		PRINTF("\n--------------------------------------------------\n");
		PRINTF("\nConfiguration Summary\n");
		PRINTF("\n--------------------------------------------------\n"ANSI_NORMAL);
		PRINTF("pwm2_pin                                  = " ANSI_NORMAL "%d \n", conf_data->t2_pwm_pin[0].gpio_pin);
		PRINTF("pwm3_pin                                  = " ANSI_NORMAL "%d \n", conf_data->t2_pwm_pin[1].gpio_pin);
		PRINTF("pwm4_pin                                  = " ANSI_NORMAL "%d \n", conf_data->t2_pwm_pin[2].gpio_pin);
	
	sprintf(option_str, STR_IN_CLK_DIV_FACTOR, (1) << conf_data->t0_t2_in_clk_div_factor);
		PRINTF("Timer0/Timer2 input clock division factor = " ANSI_NORMAL "%s \n", option_str);

		PRINTF("timer2 pause (by HW)                      = " ANSI_NORMAL "%s \n", 
				(conf_data->t2_hw_pause)? STR_T2_HW_PAUSE_ON: STR_T2_HW_PAUSE_OFF);
		
		PRINTF("timer2 PWM Frequency                      = " ANSI_NORMAL "%d Hz \n", conf_data->t2_pwm_freq);

		PRINTF("--------------------------------------------------\n");

}


void print_interactive_menu_header(peri_sample_type_t sample_id)
{
	PRINTF("Sample Configuration Menu\n");
	PRINTF("\n--------------------------------------------------\n");

	if (sample_id == PERI_TIMER0_BUZ)
	{
		PRINTF("Choose two GPIO PINs (on which LEDs connected) for PWM control\n");
	}
	else if (sample_id == PERI_TIMER2_PWM)
	{
		PRINTF("Choose 3 GPIO PINs (on which LEDs connected) for PWM control\n");
	}
}

void print_timer0_tst_dur(peri_sample_type_t sample_id)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		PRINTF("\nTest duration (in sec) ?\n");
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (10 sec) : ");
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		PRINTF("\n"ANSI_NORMAL"Buzzer Test Counter (BTC)?\n");
		PRINTF("\ne.g.) BTC = 50 -> 320ms * 50 = 16 sec. 320ms is based on sample t0 buzzer config \n");	
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (BTC=50) : ");
	}
}

void print_timer0_clk_div_def(peri_sample_type_t sample_id)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM0_CLK_DIV_BY_10) : ");
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM0_CLK_NO_DIV) : ");
	}
	else
	{
		PRINTF(ANSI_BOLD "Type [Q]uit or [ENTER] for default (TIM0_CLK_DIV_BY_10) : ");
	}
}

void print_pwm_reload(peri_sample_type_t sample_id, 
						   peri_pwm_conter_type_t cnt_type)
{
	if (sample_id == PERI_TIMER0_GEN)
	{
		switch (cnt_type)
		{
			case ON_COUNTER:
			{
				PRINTF("\n"ANSI_BOLD"ON COUNTER reload value ?\n");
				PRINTF(ANSI_NORMAL": e.g.) reload value for 100ms (T = 1/200kHz * RELOAD_100MS = 0,000005 * %d = 100ms)\n", 
						get_pwm_reload_val_def(sample_id, cnt_type) );
				PRINTF(ANSI_BOLD "Type [ENTER] only for default (RELOAD_100MS = %d) : ",
						get_pwm_reload_val_def(sample_id, cnt_type) );
			} 	break;
			default:
				break;
		}
	}
	else if (sample_id == PERI_TIMER0_BUZ)
	{
		switch (cnt_type)
		{
			case ON_COUNTER:
			{
				PRINTF("\nON COUNTER reload value ?\n");
				PRINTF(ANSI_BOLD "Type [ENTER] only for default (%d) : " 
						,get_pwm_reload_val_def(sample_id, cnt_type));
			} 	break;
			case HIGH_COUNTER:
			{
				PRINTF("\nM (High) COUNTER reload value ?\n");
				PRINTF(ANSI_BOLD "Type [ENTER] only for default (%d) : " 
					,get_pwm_reload_val_def(sample_id, cnt_type));
			}	break;
			case LOW_COUNTER:
			{
				PRINTF("\nN (Low) COUNTER reload value ?\n");
				PRINTF(ANSI_BOLD "Type [ENTER] only for default (%d) : "
					,get_pwm_reload_val_def(sample_id, cnt_type));
			}	break;
			default:
				break;
		}
	}
}

int16_t config_confirm(void)
{
	int getStr_len = 0;
	char input_str[3];

	do
	{
		PRINTF(ANSI_REVERSE " SAMPLE CONFIGURATION CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
			   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'Y')
			{
				return RET_PROCEED;
			}
			else if (input_str[0] == 'N')
			{
				return RET_RESTART;
			}
			else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				return RET_QUIT;
			}
		}
	}
	while (1);
}

void cmd_peri_sample(int argc, char *argv[])
{
	peri_sample_type_t	help_type = PERI_ALL;
	char **cur_argv;

	// common
	uint8_t gpio_port = 0; // always 0
	int16_t gpio_pin_1, in_clk_div_factor, temp_val_16;
	int     temp_val_32;

	// timer0_common (gen/buz)
	int16_t t0_test_dur;
	int16_t t0_clk_src, t0_pwm_mode, t0_clk_div;
	int t0_reload_val;

	// blinky
	uint8_t blink_count;

	// systick
	uint32_t period_us;

	// timer0_gen
	int conf_ret;

	// timer0_buz
	app_peri_timer0_buz_start_t buz_conf;

	// timer2_pwm
	app_peri_timer2_pwm_start_t t2_pwm_conf;

	// quad_dec
	app_peri_quad_dec_start_t quadec_conf;	

#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
	// gpio
	uint8_t gpio_active;
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */

	if (argc == 1) // "peri" only
	{
		help_type = PERI_ALL;
		goto PERI_HELP;
	}

	// "blinky" ======================================================================
	/*
		peri blinky start [gpio_pin] [blink_cnt]
		e.g.) peri blinky start def
		e.g.) peri blinky start 8 10
	*/
	cur_argv = ++argv; // cur_argv: [sub-cmd]
	if (strcmp("blinky", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_BLINKY;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) != 0)
		{
			help_type = PERI_BLINKY;
			goto PERI_HELP;
		}

		cur_argv = ++argv; // cur_argv: [gpio_pin]
		// parse params
		if (strcmp("def", *cur_argv) == 0)
		{
			// default sample config = LED on p0_8 with 5 blinks
			gpio_pin_1 = 8;
			blink_count = 5;
		}
		else
		{
			gpio_pin_1 = (atoi(*(cur_argv)));
			
			cur_argv = ++argv;
			blink_count = (atoi(*(cur_argv)));
		}

		//DBG_INFO("gpio_pin = %d\n", gpio_pin);

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
		if (is_ble_img_compatible(PERI_BLINKY) == FALSE)
		{
			return;
		}
#endif
		// check param validity
		if (is_gpio_pin_valid(gpio_pin_1) == FALSE)
		{
			PRINTF("The GPIO pin you specified is not free in DA14531, please check. \n");
			return;
		}

		// send command
		app_peri_blinky_start_send(gpio_port, gpio_pin_1, blink_count);
		help_type = PERI_NONE;
	}
	// "systick" ======================================================================
	/*
		peri systick start [period] [pin]
		e.g.) peri systick start def
		e.g.) peri systick start 1000000 8
	*/
	else if (strcmp("systick", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_SYSTICK;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start" or "stop"
		if (strcmp("stop", *cur_argv) == 0)
		{
			app_peri_systick_stop_send();
			return;
		}
		else if (strcmp("start", *cur_argv) == 0)
		{

			cur_argv = ++argv; // cur_argv: [period]
			// parse params
			if (strcmp("def", *cur_argv) == 0)
			{
				// default sample config = LED on p0_8 with 5 blinks
				gpio_pin_1 = 8;
				period_us = 1000000; // 1 sec
			}
			else
			{
				period_us = (atoi(*(cur_argv)));
				
				cur_argv = ++argv;
				gpio_pin_1 = (atoi(*(cur_argv)));
			}
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_SYSTICK) == FALSE)
			{
				return;
			}
#endif
			// check param validity
			if (is_gpio_pin_valid(gpio_pin_1) == FALSE)
			{
				PRINTF("The GPIO pin you specified is not free in DA14531, please check. \n");
				return;
			}

			// send command
			app_peri_systick_start_send(gpio_port, gpio_pin_1, period_us);
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_SYSTICK;
			goto PERI_HELP;
		}
	}
	// "timer0_gen" ======================================================================
	/*
		peri timer0_gen start
	*/
	else if (strcmp("timer0_gen", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_TIMER0_GEN;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_TIMER0_GEN) == FALSE)
			{
				return;
			}
#endif

			/* user input to cofigure timer0 */
PARAM_CONFIG:
			print_interactive_menu_header(PERI_TIMER0_GEN);

			// get gpio_pin			
			if ((gpio_pin_1 = get_gpio_pin(PERI_TIMER0_GEN, PWM_0)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// get test duration
			if ((t0_test_dur = get_test_duration(PERI_TIMER0_GEN)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// get timer0_2 input clock division factor
			if ((in_clk_div_factor = get_in_clk_div_factor(PERI_TIMER0_GEN)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}


			// get timer0 clock source
			if ((t0_clk_src = get_timer0_clk_src()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			
			if ((TIM0_CLK_SEL_t)t0_clk_src == TIM0_CLK_32K) {
				if (in_clk_div_factor != 0) {
					in_clk_div_factor = 0;
					PRINTF("\n32K clock source selected!\n");
					PRINTF("\nSet input clock division factor to TIM0_2_CLK_DIV_1\n");
				}
			}

			// get timer0 pwm mode
			if ((t0_pwm_mode = get_timer0_pwm_mode()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// get timer0 clock division
			if ((t0_clk_div = get_timer0_clk_div(PERI_TIMER0_GEN)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// get on counter reload value
			if ((t0_reload_val = get_pwm_reload_val(PERI_TIMER0_GEN, ON_COUNTER)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// show all values
			PRINTF("\n--------------------------------------------------\n");
			PRINTF("\nConfiguration Summary\n");
			PRINTF("\n--------------------------------------------------\n");
			PRINTF(ANSI_NORMAL"gpio pin                                  = " ANSI_NORMAL "%d \n", gpio_pin_1);
			PRINTF("test duration                             = " ANSI_NORMAL "%d sec \n", t0_test_dur);

			sprintf(option_str, STR_IN_CLK_DIV_FACTOR, (1) << in_clk_div_factor);
				PRINTF("Timer0/Timer2 input clock division factor = " ANSI_NORMAL "%s \n", option_str);

			if ((TIM0_CLK_SEL_t)t0_clk_src == TIM0_CLK_32K)
				PRINTF("timer0 clock source                       = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_SRC_32K);
			else
				PRINTF("timer0 clock source                       = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_SRC_FAST);

			if ((PWM_MODE_t)t0_pwm_mode == PWM_MODE_ONE)
				PRINTF("timer0 pwm mode                           = " ANSI_NORMAL "%s \n", STR_TIM0_PWM_MODE_ONE);
			else
				PRINTF("timer0 pwm mode                           = " ANSI_NORMAL "%s \n", STR_TIM0_PWM_MODE_TWO);

			if ((TIM0_CLK_DIV_t)t0_clk_div == TIM0_CLK_DIV_BY_10)
				PRINTF("timer0 clock division                     = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_DIV_BY_10);
			else
				PRINTF("timer0 clock division                     = " ANSI_NORMAL "%s \n", STR_TIM0_CLK_DIV_NO);
			
				PRINTF("timer0 ON COUTNER reload value            = " ANSI_NORMAL "%d \n", t0_reload_val);
				PRINTF("--------------------------------------------------\n");

			// ask user to confirm
			conf_ret = config_confirm();
			
			if (conf_ret == RET_RESTART)
			{
				goto PARAM_CONFIG;
			}
			else if (conf_ret == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// send command
			app_peri_timer0_gen_start_send(gpio_port, 
										   (uint8_t)gpio_pin_1, 
										   (uint8_t)t0_test_dur, 
										   (tim0_2_clk_div_t)in_clk_div_factor,
										   (TIM0_CLK_SEL_t)t0_clk_src, 
										   (PWM_MODE_t)t0_pwm_mode,
										   (TIM0_CLK_DIV_t)t0_clk_div, 
										   t0_reload_val);
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_TIMER0_GEN;
			goto PERI_HELP;
		}
	}
	// "timer0_buz" ======================================================================
	/*
		peri timer0_buz start
	*/
	else if (strcmp("timer0_buz", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_TIMER0_BUZ;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_TIMER0_BUZ) == FALSE)
			{
				return;
			}
#endif
	
			/* user input to cofigure timer0 */
PARAM_CONFIG_BUZ:
			print_interactive_menu_header(PERI_TIMER0_BUZ);

			// get gpio_pin
			if ((gpio_pin_1 = get_gpio_pin(PERI_TIMER0_BUZ, PWM_0)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.t0_pwm0_pin = gpio_pin_1;
			}
				

			if ((gpio_pin_1 = get_gpio_pin(PERI_TIMER0_BUZ, PWM_1)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.t0_pwm1_pin = gpio_pin_1;
			}

			// get test duration
			if ((t0_test_dur = get_test_duration(PERI_TIMER0_BUZ)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.buz_tst_counter = t0_test_dur;
			}

			// get timer0_2 input clock division factor
			if ((in_clk_div_factor = get_in_clk_div_factor(PERI_TIMER0_BUZ)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.t0_t2_in_clk_div_factor = (tim0_2_clk_div_t)in_clk_div_factor;
			}

			// Set the clock src to FAST default.
			buz_conf.t0_clk_src = TIM0_CLK_FAST;

			// get timer0 pwm mode
			if ((t0_pwm_mode = get_timer0_pwm_mode()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.t0_pwm_mod = (PWM_MODE_t)t0_pwm_mode;
			}

			// get timer0 clock division
			if ((t0_clk_div = get_timer0_clk_div(PERI_TIMER0_BUZ)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				buz_conf.t0_clk_div = (TIM0_CLK_DIV_t)t0_clk_div;
			}


			// get on counter reload value
			if ((buz_conf.t0_on_reld_val = get_pwm_reload_val(PERI_TIMER0_BUZ, ON_COUNTER)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// get high counter reload value
			if ((buz_conf.t0_high_reld_val = get_pwm_reload_val(PERI_TIMER0_BUZ, HIGH_COUNTER)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			
			// get low counter reload value
			if ((buz_conf.t0_low_reld_val = get_pwm_reload_val(PERI_TIMER0_BUZ, LOW_COUNTER)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			show_all_conf_values_t0_buz(&buz_conf);
			
			// ask user to confirm
			conf_ret = config_confirm();
			
			if (conf_ret == RET_RESTART)
			{
				goto PARAM_CONFIG_BUZ;
			}
			else if (conf_ret == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// send command
			app_peri_timer0_buz_start_send(&buz_conf);
			
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_TIMER0_BUZ;
			goto PERI_HELP;
		}
	}
	// "timer2_pwm" ======================================================================
	/*
		peri timer2_pwm start
	*/
	else if (strcmp("timer2_pwm", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_TIMER2_PWM;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
			int i;
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_TIMER2_PWM) == FALSE)
			{
				return;
			}
#endif

			/* user input to cofigure timer0 */
PARAM_CONFIG_PWM:
			print_interactive_menu_header(PERI_TIMER2_PWM);
			
			// get pwm pins: PWM2, PWM3, and PWM4
			for (i=PWM_2; i<(PWM_4+1); i++)
			{
				if ((gpio_pin_1 = get_gpio_pin(PERI_TIMER2_PWM, (peri_gpio_pin_type_t)i)) == RET_QUIT)
				{
					RETURN_NORM_FONT();
				}
				else
				{
					t2_pwm_conf.t2_pwm_pin[i-2].gpio_pin = gpio_pin_1;
					t2_pwm_conf.t2_pwm_pin[i-2].is_used = TRUE;
				}
			}
			
			// get timer0_2 input clock division factor
			if ((in_clk_div_factor = get_in_clk_div_factor(PERI_TIMER2_PWM)) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				t2_pwm_conf.t0_t2_in_clk_div_factor = (tim0_2_clk_div_t)in_clk_div_factor;
			}

			// get t2 hw pause
			if ((temp_val_16 = get_timer2_puase_by_hw()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				t2_pwm_conf.t2_hw_pause = (tim2_hw_pause_t)temp_val_16;
			}
			
			// get t2 pwm frequency
			if ((temp_val_32 = get_timer2_pwm_freq()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				t2_pwm_conf.t2_pwm_freq = (uint32_t)temp_val_32;
			}
			t2_pwm_conf.t2_pwm_port = gpio_port;
			show_all_conf_values_t2_pwm(&t2_pwm_conf);
			
			// ask user to confirm
			conf_ret = config_confirm();
			
			if (conf_ret == RET_RESTART)
			{
				goto PARAM_CONFIG_PWM;
			}
			else if (conf_ret == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}

			// send command
			app_peri_timer2_pwm_start_send(&t2_pwm_conf);
			
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_TIMER2_PWM;
			goto PERI_HELP;
		}
	}
	// "batt_lvl" ======================================================================
	/*
		peri batt_lvl get
	*/
	else if (strcmp("batt_lvl", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_BATT_LVL;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("get", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_BATT_LVL) == FALSE)
			{
				return;
			}
#endif

			// send command
			app_peri_get_batt_lvl();
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_BATT_LVL;
			goto PERI_HELP;
		}
	}
	// "i2c_eeprom" ======================================================================
	/*
		peri i2c_eeprom start
	*/
	else if (strcmp("i2c_eeprom", *cur_argv) == 0)
	{
		if (argc == 2)
		{
			help_type = PERI_I2C_EEPROM;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_I2C_EEPROM) == FALSE)
			{
				return;
			}
#endif
			// send command
			app_peri_i2c_eeprom_start_send();
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_I2C_EEPROM;
			goto PERI_HELP;
		}
	}
	// "spi_flash" ======================================================================
	/*
		peri spi_flash start
	*/
	else if (strcmp("spi_flash", *cur_argv) == 0)
	{		
		if (argc == 2)
		{
			help_type = PERI_SPI_FLASH;
			goto PERI_HELP;
		}
		
		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_SPI_FLASH) == FALSE)
			{
				return;
			}
#endif
			// send command
			app_peri_spi_flash_start_send();
			help_type = PERI_NONE;
		}
		else
		{
			help_type = PERI_SPI_FLASH;
			goto PERI_HELP;
		}
	}
	// "quad_dec" ======================================================================
	/*
		peri quad_dec start
	*/
	else if (strcmp("quad_dec", *cur_argv) == 0)
	{	
		if (argc == 2)
		{
			help_type = PERI_QUAD_DEC;
			goto PERI_HELP;
		}

		cur_argv = ++argv; // cur_argv: "start"
		if (strcmp("start", *cur_argv) == 0)
		{
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			if (is_ble_img_compatible(PERI_QUAD_DEC) == FALSE)
			{
				return;
			}
#endif

PARAM_CONFIG_QUADEC:
			print_interactive_menu_header(PERI_QUAD_DEC);

			// get number of events before quad_dec interrupt, def=1
			if ((temp_val_16 = get_quad_dec_evt_count()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				quadec_conf.evt_cnt_bef_int = temp_val_16;
			}

			// get the clock divisor of the quadrature decoder, def=1
			if ((temp_val_16 = get_quad_dec_clk_divisor()) == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
			else
			{
				quadec_conf.clk_div = temp_val_16;
			}

			PRINTF("\n--------------------------------------------------\n");
			PRINTF("\nConfiguration Summary\n");
			PRINTF("\n--------------------------------------------------\n"ANSI_NORMAL);
			PRINTF("event count before quadec interrupt            = " ANSI_NORMAL "%d \n", quadec_conf.evt_cnt_bef_int);
			PRINTF("clock divisor of quadec                        = " ANSI_NORMAL "%d \n", quadec_conf.clk_div);
			PRINTF("CHX_A/CHX_B Pins (fixed in DA14531 of DA16600) = " ANSI_NORMAL "p0_8 (ChXA), p0_9 (ChXB) \n");
			
			// ask user to confirm
			conf_ret = config_confirm();
			
			if (conf_ret == RET_RESTART)
			{
				goto PARAM_CONFIG_QUADEC;
			}
			else if (conf_ret == RET_QUIT)
			{
				RETURN_NORM_FONT();
			}
		
			// send command
			app_peri_quad_dec_start_send(&quadec_conf);
			help_type = PERI_NONE;
		}
		//2.
		else if (strcmp("stop", *cur_argv) == 0)
		{
			// send command
			app_peri_quad_dec_stop_send();
			help_type = PERI_NONE;
		}
		else // none in (start, stop, poll)
		{
			help_type = PERI_QUAD_DEC;
			goto PERI_HELP;
		}
	}
#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
	// "gpio set/get" ======================================================================
	/*
		peri quad_dec start
	*/
	else if (strcmp("gpio", *cur_argv) == 0)
	{	
		GPIO_PUPD gpio_func = OUTPUT;
		extern void app_peri_gpio_set_cmd_send(uint8_t gpio_port, uint8_t gpio_pin, uint8_t active, GPIO_PUPD mode);
		extern void app_peri_gpio_get_cmd_send(uint8_t gpio_port, uint8_t gpio_pin);
		
		help_type = PERI_GPIO_CTRL;
		if (argc == 2)
		{
			goto PERI_HELP;
		}

		cur_argv = ++argv;
		if (strcmp("set", *cur_argv) == 0) {
			if (argc < 6) {
				goto PERI_HELP;
			}

			cur_argv = ++argv;
			gpio_port = atoi(*(cur_argv));

			cur_argv = ++argv;
			gpio_pin_1 = atoi(*(cur_argv));

			cur_argv = ++argv;
			gpio_active = htoi(*(cur_argv));

			if (argc > 6) {
			    cur_argv = ++argv;
			    gpio_func = atoi(*(cur_argv));
			    gpio_func = (gpio_func == 0) ? INPUT : (gpio_func == 1) ? INPUT_PULLUP : (gpio_func == 2) ? INPUT_PULLDOWN : OUTPUT;
			}

			// Unavailable port
			if (gpio_port > 0) {
			    PRINTF("Please check the port number.");
			    goto PERI_HELP;
			}

			// GTL pins
			if (gpio_pin_1 == 0 || gpio_pin_1 == 1 || gpio_pin_1 == 3 || gpio_pin_1 == 4) {
			    PRINTF("This pin is used for internal.");
			    goto PERI_HELP;
			}

#ifdef __CFG_ENABLE_BLE_HW_RESET__
			// HW reset pin
			if (gpio_pin_1 == 11)
			    PRINTF("This pin is used for internal.");
			    goto PERI_HELP;
#endif

			PRINTF(" GPIO SET > port: %d, pin: %d, %s\n",
				gpio_port, gpio_pin_1,
				gpio_active == 0xFF ? "Release" : (gpio_active ? "High" : "Low"));
			app_peri_gpio_set_cmd_send(gpio_port, gpio_pin_1, 
						   gpio_active, gpio_func);
		} else if (strcmp("get", *cur_argv) == 0) {
			if (argc < 5) {
				goto PERI_HELP;
			}

			cur_argv = ++argv;
			gpio_port = atoi(*(cur_argv));

			cur_argv = ++argv;
			gpio_pin_1 = atoi(*(cur_argv));

			PRINTF(" GPIO GET > port: %d, pin: %d\n",
				gpio_port, gpio_pin_1);
			app_peri_gpio_get_cmd_send(gpio_port, gpio_pin_1);
		}

		help_type = PERI_NONE;
	}
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
	else
	{
		PRINTF("Unknown command \n");
		help_type = PERI_ALL;
	}

PERI_HELP:
	switch (help_type)
	{
		case PERI_ALL:
		{	
			PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri : Run DA14531 Peripheral Driver Sample \n");
			PRINTF("       type a command below \n");
			PRINTF("--------------------------------------------------\n");

			PRINTF("[01] peri blinky     : blinking LED sample \n");
			PRINTF("[02] peri systick    : systick timer sample \n");
			PRINTF("[03] peri timer0_gen : timer0 general sample \n");
			PRINTF("[04] peri timer0_buz : timer0 PWM buzzer sample \n");
			PRINTF("[05] peri timer2_pwm : timer2 PWM LED array sample\n");
			PRINTF("[06] peri batt_lvl   : battery level read sample\n");
			PRINTF("[07] peri i2c_eeprom : I2C EEPROM read/write sample\n");
			PRINTF("[08] peri spi_flash  : SPI_flash read/write sample\n");
#if defined (__SUPPORT_QUAD_DEC_CMD__)
			PRINTF("[09] peri quad_dec   : Quadrature Decoder sample\n");
#endif /* __SUPPORT_QUAD_DEC_CMD__ */
#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
#if defined (__SUPPORT_QUAD_DEC_CMD__)
			PRINTF("[10] peri gpio       : GPIO contorl(High/Low)\n");
#else
			PRINTF("[09] peri gpio       : GPIO contorl(High/Low)\n");
#endif /* __SUPPORT_QUAD_DEC_SAMPLE__ */
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
			PRINTF("--------------------------------------------------\n");

		} break;

		case PERI_BLINKY:
		{
			PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri blinky : run blinky (blinking LED) sample \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri blinky start [gpio_pin] [blink_count]\n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("peri blinky start def\n");
			PRINTF("  --> run blinky sample w/ default config (GPIO=p0_8, blink_count=5)\n");
			PRINTF("peri blinky start 8 10\n");
			PRINTF("  --> run blinky sample w/ LED (on p0_8), blink 10 times\n");

			PRINTF("--------------------------------------------------\n");
		} break;
		case PERI_SYSTICK:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri systick : run systick timer sample \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri systick start [period] [gpio_pin] \n");
			PRINTF("        peri systick stop \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("peri systick start def\n");
			PRINTF("  --> run systick sample w/ default config (period=1sec, GPIO=p0_8)\n");
			PRINTF("peri systick start 1000000 8\n");
			PRINTF("  --> run systick timer. systick expires in 1000000 us (1 sec) \n");
			PRINTF("      turn LED ON or OFF per 1 sec \n");
			PRINTF("peri systick stop\n");
			PRINTF("  --> stop systick timer\n");

			PRINTF("--------------------------------------------------\n");
		} break;
		case PERI_TIMER0_GEN:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri timer0_gen : run timer0 general sample \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri timer0_gen start\n");
			PRINTF("        (interactive config menu follows) \n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_TIMER0_BUZ:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri timer0_buz : run timer0 buzzer sample \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri timer0_buz start\n");
			PRINTF("        (interactive config menu follows) \n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_TIMER2_PWM:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri timer2_pwm : run timer0 pwm sample \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri timer2_pwm start\n");
			PRINTF("        (interactive config menu follows) \n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_BATT_LVL:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri batt_lvl :  read level of the battery \n");
			PRINTF("                 that is connected to DA14531  \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri batt_lvl get\n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_I2C_EEPROM:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri i2c_eeprom :  read/write test of i2c eeprom \n");
			PRINTF("                   that is connected to DA14531.  \n");
			PRINTF("                   >>> SCL (p0_8), SDA (p0_11)   \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri i2c_eeprom start\n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_SPI_FLASH:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri spi_flash :  read/write test of spi_flash \n");
			PRINTF("                   that is connected to DA14531.  \n");
			PRINTF("                   >>> SPI_EN (p0_8), SPI_CLK (p0_11)   \n");
			PRINTF("                   >>> SPI_DO (p0_2), SPI_DI  (p0_10)   \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri spi_flash start\n");
			PRINTF("--------------------------------------------------\n");

		} break;
		case PERI_QUAD_DEC:
		{
		  	PRINTF("\n--------------------------------------------------\n");
			PRINTF("peri quad_dec :   configure and read from the \n");
			PRINTF("                  Quadrature Decoder peripheral\n");
			PRINTF("                   that is connected to DA14531.  \n");
			PRINTF("--------------------------------------------------\n");
			PRINTF("SYNTAX: peri quad_dec start     : start quadec \n");
			PRINTF("        peri quad_dec stop      : stop  quadec \n");
			PRINTF("--------------------------------------------------\n");

		} break;
#if defined(__SUPPORT_DA14531_GPIO_CONTROL__)
		case PERI_GPIO_CTRL:
		{
		  	PRINTF("\n------------------------------------------------------\n");
			PRINTF("peri gpio : Set/Get GPIO of DA14531                     \n");
			PRINTF("             - To set GPIO, checking required           \n");
			PRINTF("                 whether the GPIO is free or not        \n");
			PRINTF("------------------------------------------------------  \n");
			PRINTF("SYNTAX: SET                                             \n");
			PRINTF(" peri gpio set port_no pin_no state [func]              \n");
			PRINTF("   state : 1(H), 0(L), FF(Release)                      \n");
			PRINTF("   func  : 0(INPUT), 1(INPUT_PULLUP), 2(INPUT_PULLDOWN), 3(OUTPUT) - Default OUTPUT\n");
			PRINTF("   ex) peri gpio set 0 8 1 3  ; Set P0_8 to High as OUTPUT \n");
			PRINTF("   ex) peri gpio set 0 8 0 0  ; Set P0_8 as INPUT \n");
			PRINTF("                                                        \n");
			PRINTF("SYNTAX: GET                                             \n");
			PRINTF(" peri gpio get port_no pin_no                           \n");
			PRINTF("   ex) peri gpio get 0 8     ; Get P0_8 status       \n");
			PRINTF("--------------------------------------------------------\n");
		} break;
#endif /* __SUPPORT_DA14531_GPIO_CONTROL__ */
		default:
			break;
	}
	return;
}

void cmd_ble_start_adv(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendstart_adv();
}

void cmd_ble_stop_adv(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendstop_adv();
}

#if defined (__OTA_TEST_CMD__)
extern UINT ota_fw_update_combo(void);
void cmd_ota_test(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ota_fw_update_combo();
}
#endif

void cmd_get_local_dev_info(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	app_get_local_dev_info(GAPM_GET_DEV_BDADDR);
}


#if defined (__EXCEPTION_RST_EMUL__)
void cmd_wifi_force_exception(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	asm("udf.w #0");
}
#endif

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
void cmd_ble_force_exception(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argv);

	if (argc >= 2)
	{
		return;
	}
	else
	{
		ConsoleSendBleForceException();
	}
}
#endif

#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)
void cmd_enter_sleep2(int argc, char *argv[])
{
	int	sec;
	unsigned long long usec;

	//extern UCHAR set_boot_index(int index);

	if (argc != 2) {
		PRINTF("Usage : enter_sleep2 [sec]\n");
		return;
	}

	sec = atoi(argv[1]);

	usec = (sec * 1000000);
				if (dpm_mode_is_enabled())
				{
					PRINTF("sleep2 is entered, and DUT will wake up in %d sec\n", sec);
				}
				else
				{
					PRINTF("enable DPM and try again ! \n");
					return;
				}
	
#if defined (__WAKE_UP_BY_BLE__)
				enable_wakeup_by_ble();
#endif // (__WAKE_UP_BY_BLE__)
				// just to print the debug message above fully. It is safe to remove it
				vTaskDelay(20);
				dpm_sleep_start_mode_2(usec, true);

}
#endif // (__TEST_FORCE_SLEEP2_WITH_TIMER__)


#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)
void cmd_enter_sleep3(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argv);

	if (argc != 1) {
		PRINTF("Usage : enter_sleep3\n");
		return;
	}
	
#if defined (__WAKE_UP_BY_BLE__)	
	enable_wakeup_by_ble();
#endif // (__WAKE_UP_BY_BLE__)

	RTC_CLEAR_EXT_SIGNAL(); // to remove wakeup src
	vTaskDelay(200);
	dpm_app_sleep_ready_set("test_sleep3");
}
#endif // (__FOR_DPM_SLEEP_CURRENT_TEST__)

void EnQueueToConsoleQ(console_msg *pmsg)
{
#ifdef __FREERTOS__
	if (msg_queue_send(&ConsoleQueue, 0, 0, (void *)pmsg, sizeof(console_msg), portMAX_DELAY) == OS_QUEUE_FULL) {
		configASSERT(0);
	}
	MEMFREE(pmsg);
#else
	xSemaphoreTake(ConsoleQueueMut, portMAX_DELAY);
	EnQueue(&ConsoleQueue, pmsg);
	xSemaphoreGive(ConsoleQueueMut);
#endif
}

void ConsoleSendGetBleFwVer(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_GET_BLE_FW_VER_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendGapmReset(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_GAPM_RESET;
	EnQueueToConsoleQ(pmsg);
}

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
void ConsoleSendBleForceException(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_FORCE_EXCEPTION_ON_BLE;
	EnQueueToConsoleQ(pmsg);
}
#endif


void ConsoleSendSensorStart(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_IOT_SENSOR_START;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendSensorStop(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}
	
	pmsg->type = CONSOLE_IOT_SENSOR_STOP;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendScan(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_DEV_DISC_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendConnnect(int indx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_CONNECT_CMD;
	pmsg->val = indx;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendDisconnnect(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_DISCONNECT_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendstart_adv(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_ADV_START_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendstop_adv(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_ADV_STOP_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleRead(unsigned char type)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = type;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleWrite(unsigned char type, unsigned char val)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = type;
	pmsg->val = val;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendExit(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_EXIT_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleTitle(void)
{
	PRINTF ("\n\n\t\t####################################################\n");
	PRINTF ("\t\t#    DA14531 Proxr IoT Sensor demo application   #\n");
	PRINTF ("\t\t####################################################\n\n");
}

void ConsoleScan(void)
{
	ConsoleTitle();

	PRINTF ("\t\t\t\tScanning... Please wait \n\n");

	ConsolePrintScanList();
}

void ConsoleIdle(void)
{
	ConsoleTitle();
}

void ConsolePrintScanList(void)
{
	int index, i;

	PRINTF ("No. \tbd_addr \t\tName \t\tRssi\n");

	for (index = 0; index < MAX_SCAN_DEVICES; index++)
	{
		if (app_env.devices[index].free == false)
		{
			PRINTF("%d\t", index + 1);

			for (i = 0; i < BD_ADDR_LEN-1; i++)
			{
				PRINTF ("%02x:", app_env.devices[index].adv_addr.addr[BD_ADDR_LEN - 1 - i]);
			}

			PRINTF ("%02x", app_env.devices[index].adv_addr.addr[0]);

			PRINTF ("\t%s\t%d\n", app_env.devices[index].data, app_env.devices[index].rssi);
		}
	}

}

void ConsoleConnected(int full)
{
	int i;

	ConsoleTitle();

	PRINTF ("\t\t\t Connected to Device \n\n");


	PRINTF ("BLE Peer BDA: ");

	for (i = 0; i < BD_ADDR_LEN-1; i++)
	{
		PRINTF ("%02x:", app_env.proxr_device.device.adv_addr.addr[BD_ADDR_LEN - 1 - i]);
	}

	PRINTF ("%02x\t", app_env.proxr_device.device.adv_addr.addr[0]);

	PRINTF ("Bonded: ");

	if (app_env.proxr_device.bonded)
	{
		PRINTF ("YES\n");
	}
	else
	{
		PRINTF ("NO\n");
	}

	/* For Future Use
	if (app_env.proxr_device.rssi != 0xFF)
	        printf ("RSSI: %02d\n\n", app_env.proxr_device.rssi);
	    else
	        printf ("RSSI: \n\n");
	*/

	if (full)
	{

		if (app_env.proxr_device.llv != 0xFF)
		{
			PRINTF ("Link Loss Alert Lvl: %02d\t\t", app_env.proxr_device.llv);
		}
		else
		{
			PRINTF ("Link Loss Alert Lvl: \t\t");
		}

		if (app_env.proxr_device.txp != 127)
		{
			PRINTF ("Tx Power Lvl: %02d\n\n", app_env.proxr_device.txp);
		}
		else
		{
			PRINTF ("Tx Power Lvl:  \n\n");
		}

	}
}

#endif /* __BLE_PERI_WIFI_SVC_PERIPHERAL__ */

/* EOF */
