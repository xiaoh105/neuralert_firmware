/**
 ****************************************************************************************
 *
 * @file rf_meas_api.h
 *
 * @brief APIs for RF measurement
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

#ifndef _RF_MEAS_API_H_
#define _RF_MEAS_API_H_


#include <string.h>
#include "target.h"
#include "da16200_ioconfig.h"

/* priority parameter in rf_meas_btcoex*/
#define BTCOEX_PRIO_0		0	/* BT priority is higher than WLAN */
#define BTCOEX_PRIO_1 		1	/* BT and WLAN priorities are equal */
#define BTCOEX_PRIO_2		2	/* WLAN priority is higher than BT */
#define BTCOEX_PRIO_3		3	/* Force Mode */

/* enable parameter in rf_meas_btcoex*/
#define DISABLE_COEX 0
#define ENABLE_COEX 1

#if defined ( __SUPPORT_BTCOEX_1PIN__ )
    /* This should not be changed */
    #define __DEFAULT_COEX_PRIORITY__ BTCOEX_PRIO_0
#else
	/* This could set default configuration for COEX priority*/
    #if !defined ( __DEFAULT_COEX_PRIORITY__ )
    	#define __DEFAULT_COEX_PRIORITY__ BTCOEX_PRIO_0
	#endif
#endif

/* BTCOEX - Bluetooth coexistence Test */
void rf_meas_btcoex(uint8_t enable,uint8_t priority, uint8_t gpio);

/* MonRxTaRssi -	Monitor RX Target Address RSSI */
struct rssi_mon
{
	int8_t avg;
	int8_t min;
	int8_t max;
	uint8_t cnt;
	uint8_t flag;
};

struct MonRxTaRssi
{
	struct rssi_mon b;
	struct rssi_mon g;
	struct rssi_mon n;
	int8_t cur_rssi;
};

void rf_meas_monRxTaRssi_set(uint8_t enable,
							uint8_t OnlyFcsOk,
							uint8_t IgnoreTa,
							uint8_t HoldReq,
							uint8_t IIR_Alpha,
							char *addr);
void rf_meas_monRxTaRssi_get(struct MonRxTaRssi *meas);

/* MonRxTaCfo  -	Monitor RX Target Address CFO */

struct cfo_mon {
	uint8_t	cnt;
	uint8_t flag;
	float cfo;
};

struct MonRxTaCfo{
	struct cfo_mon dsss;
	struct cfo_mon ofdm;
};

void rf_meas_monRxTaCfo_set(uint8_t enable,
							uint8_t OnlyFcsOk,
							uint8_t IgnoreTa,
							uint8_t HoldReq,
							char *addr);
void rf_meas_monRxTaCfo_get(struct MonRxTaCfo *meas);

void  cmd_rf_meas_btcoex(int argc, char* argv[]);
void  cmd_rf_meas_MonRxTaRssi(int argc, char* argv[]);
void  cmd_rf_meas_MonRxTaCfo(int argc, char* argv[]);

#endif	// _RF_MEAS_API_H_

/* EOF */
