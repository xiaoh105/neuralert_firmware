/**
 ****************************************************************************************
 *
 * @file sys_feature.c
 *
 * @brief System feature
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

#include <ctype.h>
#include "clib.h"

#include "da16x_system.h"
#include "sys_feature.h"
#include "common_def.h"
#include "iface_defs.h"
#include "da16x_types.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "environ.h"
#include "da16x_network_main.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


extern int	chk_dpm_wakeup(void);
extern void	fc80211_da16x_pri_pwr_down(int retention);
extern const char *dpm_restore_country_code(void);


///////////////////////////////////////////////////////////////
/// Thread / Timer control feature ////////////////////////////
///////////////////////////////////////////////////////////////
//
void kill_tx_thread(char *name)
{
	TaskHandle_t	check_thread;
	check_thread = xTaskGetHandle(name);

	if(check_thread != NULL){
		eTaskState state = eTaskGetState(check_thread);

		if(state != eDeleted){
			vTaskDelete(check_thread);
		}
	}
}

void kill_all_tx_thread(void)
{
}

void kill_all_tx_timer(void)
{
}

#include "da16x_timer.h"
void kill_all_rtc_timer(void)
{
	DA16X_TIMER_ID rtc_tid;
	DA16X_TIMER *gtimer = (DA16X_TIMER *)(DA16X_RETMEM_BASE + 0x100);

	UINT32 *CURIDX = (UINT32*)((DA16X_RETMEM_BASE + 0x100)
					+ (sizeof(DA16X_TIMER) * TIMER_ERR)
					+ sizeof(UINT32*)
					+ sizeof(UINT32*));

	PRINTF(">>> Stop all RTC timers ...\n\n");

	rtc_tid = *CURIDX;
	while (rtc_tid < TIMER_ERR) {
		rtc_cancel_timer(rtc_tid);

		rtc_tid	= (DA16X_TIMER_ID)gtimer[rtc_tid].type.content[DA16X_IDX_NEXT];
	}
}


void set_power_off(int flag)
{
	PRINTF(ANSI_COLOR_YELLOW "<< SLEEP >>\n" ANSI_COLOR_DEFULT);

	vTaskDelay(1); /* Sleep Console */

	check_environ();

	kill_all_tx_thread();
	kill_all_tx_timer();

	for (int i = 0; i < 0x200000; i++) {
		__NOP();
	}

	if (wifi_netif_status(WLAN0_IFACE) == pdTRUE
		|| wifi_netif_status(WLAN1_IFACE) == pdTRUE
		|| flag == 0) {
		/* Sleep */

		/* Case of External Interrupt,
		 * let's do clear external int flag of wakeup source
		 */
		RTC_CLEAR_EXT_SIGNAL();

		fc80211_da16x_pri_pwr_down(FALSE);
	}
}


///////////////////////////////////////////////////////////////
/// Tx Power Grade ////////////////////////////////////////////
///////////////////////////////////////////////////////////////

#include "common_config.h"
#define NUM_CHANNEL_PWR 14

UCHAR	region_power_level[NUM_CHANNEL_PWR] = { 0, };
UCHAR	region_power_level_dsss[NUM_CHANNEL_PWR] = { 0, };
extern country_ch_power_level_t cc_power_level_dsss[MAX_COUNTRY_CNT];
extern country_ch_power_level_t cc_power_level[MAX_COUNTRY_CNT];

ULONG cc_power_table(UINT index)
{
	return (ULONG)&cc_power_level[index];
}


UINT cc_power_table_size(void)
{
	return ARRAY_SIZE(cc_power_level);
}


char * country_code(int index)
{
	return cc_power_level[index].country;
}


enum {
	E_WRITE_OK,
	E_WRITE_ERROR,
	E_ERASE_OK,
	E_ERASE_ERROR,
	E_DIGIT_ERROR,
	E_CANCELED,
	E_UNKNOW
};


UINT set_debug_ch_tx_level(char* ch_pwr_l)
{
	UINT	status = E_WRITE_OK;
	UINT	idx, len;
	char	c_code[4];

	/* Check country code value if "ALL" or not */
	if (chk_dpm_wakeup()) {
		strcpy(c_code, (char*)dpm_restore_country_code());
	} else {
		if (da16x_get_config_str(DA16X_CONF_STR_COUNTRY, c_code) != CC_SUCCESS) {
			strncpy(c_code, COUNTRY_CODE_DEFAULT, 4);
		}
	}

	len = strlen(ch_pwr_l);

	if (len == 14) {
		if (strcmp(c_code, "ALL") != 0) { /* ALL Channel for Debug */
			PRINTF("Error : Current country code = [%s]\n", c_code);
			status = E_CANCELED;
			goto error;
		}

		for (idx = 0; idx < 14; idx++) {
			if (!isxdigit((int)(ch_pwr_l[idx]))) {

				status = E_DIGIT_ERROR;
				goto error;
			}
		}

		for (idx = 0; idx < 14; idx++) {
			region_power_level[idx] = toint(ch_pwr_l[idx]);
		}

		if (da16x_set_nvcache_str(DA16X_CONF_STR_DBG_TXPWR, ch_pwr_l) == CC_SUCCESS) {
			status = E_WRITE_OK;
			da16x_nvcache2flash();
		}
	} else {
		if (strncasecmp(ch_pwr_l, "ERASE", 5) == 0) { /* Erase NVRAM */
			if (da16x_set_nvcache_str(DA16X_CONF_STR_DBG_TXPWR, "") != CC_SUCCESS) {
				status = E_WRITE_ERROR;
			} else {
				da16x_nvcache2flash();
			}
		} else {
			status = E_DIGIT_ERROR;
		}
	}

error:

	return status;
}


UINT init_region_tx_level(void)
{
	UINT 	idx;
	UINT	status;
	char	ch_pwr_l[15] = { 0, };
	char	c_code[4]= { 0, };

	if (chk_dpm_wakeup()) {
		strcpy(c_code, (char*)dpm_restore_country_code());
	} else {
		if (da16x_get_config_str(DA16X_CONF_STR_COUNTRY, c_code) != CC_SUCCESS) {
			strncpy(c_code, COUNTRY_CODE_DEFAULT, 4);
		}
	}

	/* "ALL" country code for debugging */
	if (strcmp(c_code, "ALL") == 0) {
		status = da16x_get_config_str(DA16X_CONF_STR_DBG_TXPWR, ch_pwr_l);

		if (status == CC_SUCCESS) {
			if (strlen(ch_pwr_l) == 14) {
				for (idx = 0; idx < 14; idx++) {
					if (!isxdigit((int)(ch_pwr_l[idx]))) {
						break;
					}
				}

				for (idx = 0; idx < NUM_CHANNEL_PWR; idx++) {
					region_power_level[idx] = toint(ch_pwr_l[idx]);
				}
			} else {
				goto all_default;
			}
		} else {
all_default :
			memcpy(region_power_level, cc_power_level[MAX_COUNTRY_CNT-2].ch, sizeof(region_power_level));
			memcpy(region_power_level_dsss, cc_power_level_dsss[MAX_COUNTRY_CNT-2].ch, sizeof(region_power_level_dsss));
		}
	} else {
		/* Channel Tx PWR */
		for (idx = 0; cc_power_level[idx].country != NULL; idx++) {
			if (strcmp(cc_power_level[idx].country, c_code) == 0) {
				break;
			}
		}

		/* If Country is not existed, return */
		if (cc_power_level[idx].country == NULL)
			return 0;

		memcpy(region_power_level, cc_power_level[idx].ch, sizeof(region_power_level));
		memcpy(region_power_level_dsss, cc_power_level_dsss[idx].ch, sizeof(region_power_level_dsss));
	}

	return 0;
}


/* We use this primitive for country and power confirmation */
void print_tx_level(void)
{
	char 	c_code[4] ={0,};
	UINT 	idx;

	if (da16x_get_config_str(DA16X_CONF_STR_COUNTRY, c_code) != CC_SUCCESS) {
		strncpy(c_code, COUNTRY_CODE_DEFAULT, 4);
	}

	/* Channel Tx PWR */
	for (idx = 0; cc_power_level[idx].country != NULL; idx++) {
		if (strcmp(cc_power_level[idx].country, c_code) == 0) {
			break;
		}
	}

	if (cc_power_level[idx].country == NULL) {
		PRINTF("\nInvaild current Country Code(%s)\n", c_code);
		return;
	}

	if (strcmp(c_code, "ALL") != 0) {
		/* If Both Current Power table and Current Country Power table is different, do save */
		if (memcmp(region_power_level, cc_power_level[idx].ch, sizeof(region_power_level)) != 0) {
			memcpy(region_power_level, cc_power_level[idx].ch, sizeof(region_power_level));
		}

		if (memcmp(region_power_level_dsss, cc_power_level_dsss[idx].ch, sizeof(region_power_level_dsss)) != 0) {
			memcpy(region_power_level_dsss, cc_power_level_dsss[idx].ch, sizeof(region_power_level_dsss));
		}
	}

	PRINTF("[Current TX Level]\n\tC_CODE: %s\n\ttxpwr level per channel : ", c_code);
	for (int ii = 0; ii < NUM_CHANNEL_PWR; ii++) {
		PRINTF("%x,", region_power_level[ii]);
	}
	PRINTF("\n");

	PRINTF("[Current TX Level of DSSS]\n\tC_CODE: %s\n\ttxpwr level per channel : ", c_code);
	for (int ii = 0; ii < NUM_CHANNEL_PWR; ii++) {
		PRINTF("%x,", region_power_level_dsss[ii]);
	}
	PRINTF("\n");
}

/* EOF */
