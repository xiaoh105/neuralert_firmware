/**
 ****************************************************************************************
 *
 * @file app_provisioning_sample.c
 *
 * @brief Sample application how to get softap provisioning via TCP or TLS
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

#include "sample_defs.h"


#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sdk_type.h"
#include "json.h"
#include "da16x_system.h"
#include "common_def.h"
#include "da16x_network_main.h"

#include "../inc/app_common_util.h"
#include "../inc/app_provision.h"

#include "common_utils.h"
#include "da16x_ping.h"

#if !defined (__SUPPORT_WIFI_PROVISIONING__)
#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif


/*
* brief DPM mode setting 
*/
int sleepModeVal =0; 
/*! When DPM mode is 3, DPM */
int useDpmVal =0;
int dpmKeepAliveVal =0;
int userWakeupVal =0;
int timWakeupVal =0;
int rtcTime =0;


/**
 ****************************************************************************************
 * @brief Get DPM set value 
 * @param[in] _type - get  value type 
 * @return  success : set value, fail : -1
 ****************************************************************************************
 */
int app_get_DPM_set_value( GetValueType _type )
{
	if(_type ==TYPE_SLEEP_MODE)return sleepModeVal;
	if(_type ==TYPE_USE_DPM)return useDpmVal;
	if(_type ==TYPE_DPM_KEEP_ALIVE)return dpmKeepAliveVal;
	if(_type ==TYPE_USER_WAKE_UP)return userWakeupVal;
	if(_type ==TYPE_TIM_WAKE_UP)return timWakeupVal;
	if(_type ==TYPE_RTC_TIME)return rtcTime;

	APRINTF_E("getDPMSetValue error [Undfined type]\n");
	return -1;
}

/**
 ****************************************************************************************
 * @brief Set Sleep mode and DTIM value 
 * @param[in] _sleepMode - sleepMode setting value 
 * @param[in] _useDpmSet -dpm setting value
 * @param[in] _rtcTime -rtc wakeup setting value
 * @param[in] _dpmKeepAlive -dpm Keep Alive settging value
 * @param[in] _userWakeup -user wakeup setting value
 * @param[in] _timWakeup -tim wakeup setting value
 * @return  void
 ****************************************************************************************
 */
void setSleepModeDtim(
	int _sleepMode,
	int _useDpmSet,
	int _rtcTime,
	int _dpmKeepAlive,
	int _userWakeup,
	int _timWakeup)
{
	if(_sleepMode >=1 && _sleepMode <=3)
	{
		sleepModeVal=_sleepMode;
	}else
	{
		APRINTF_E("Unsupport Sleep mode error \n");
		return;
	}
	useDpmVal = _useDpmSet;
	dpmKeepAliveVal=_dpmKeepAlive;
	userWakeupVal = _userWakeup;
	timWakeupVal=_timWakeup;
	rtcTime = _rtcTime;

	APRINTF("[Set] SleepMode: %d ,DPM: %d,rtcTime : %d sec,  DPM_KeepAlive: %d, userWakeUp: %d, timWakeUp: %d \n",
		sleepModeVal,useDpmVal,rtcTime , dpmKeepAliveVal, userWakeupVal,timWakeupVal);

	
}
#endif // !defined (__SUPPORT_WIFI_PROVISIONING__)
extern UINT	da16x_ping_client(int iface,
							char *domain_str,
							unsigned long ipaddr,
							unsigned long *ipv6dst,
							unsigned long *ipv6src,
							int len,
							unsigned long count,
							int wait,
							int interval,
							int nodisplay,
							char *ping_result);

/**
 ****************************************************************************************
 * @brief checking internet or network by using ping server
 * @param[in] ipaddrstr ping server address
 * @return  1 : ok	0 : internet fail 
 ****************************************************************************************
 */

int32_t app_check_internet_with_ping(char *ipaddrstr)
{
	char result_str[128] = {0, };
	int32_t len = 0;
	int32_t wait = 0;
	int32_t interval = 0;
	int32_t ping_interface = 0; /* default wlan0 */
	int32_t transmitted, reply_count;
	uint32_t ipaddr = 0;
	uint32_t count = 0;
	uint32_t average, time_min, ping_max;
    

	// result string 
	transmitted = reply_count = 0;
	average = time_min = ping_max = 0;
	ipaddr = (uint32_t)iptolong(ipaddrstr);

	count = 4;         // only 2
	len = 32;          //default
	wait = 8000;      // default 4sec
	interval = 1000;  // interval default 1sec

	/* If station interface */
	ping_interface = 0;                    //WLAN0_IFACE;

	/* ping client api execution with nodisplay as 1 and getting the string of result */                  
	da16x_ping_client(ping_interface, NULL, ipaddr, NULL, NULL, len, count, wait,
						interval, 1, result_str);                             

	/* parsing the result string */
	sscanf(result_str, "%u,%u,%u,%u,%u", (unsigned int *)&transmitted , (unsigned int *)&reply_count , (unsigned int *)&average,
			(unsigned int *)&time_min, (unsigned int *)&ping_max);
     
	if(reply_count > 0) /* Success */
	{
		APRINTF("Ping reply is ok\n");
		return 1;
	}
     
	APRINTF("Ping reply is fail\n");

	return 0; 
}

#if defined (__BLE_PROVISIONING_SAMPLE__)  

/* wi-fi connection callback flag */
extern EventGroupHandle_t  evt_grp_wifi_conn_notify;

static int ENABLE_FACTORYSET = 0;
/**
 ****************************************************************************************
 * @brief checking Provision Running Status
 * @param[in] set  1 : Provision Enable,	0 : Provision Disable 
 * @return void 
 ****************************************************************************************
 */
void app_check_provision(int set)
{
    ENABLE_FACTORYSET = set;
    
#if defined (__PROVISION_ATCMD__)
    if(ENABLE_FACTORYSET == 1)
    {
        atcmd_provstat(ATCMD_PROVISION_START);
    }
#endif

};

 /**
 ****************************************************************************************
 * @brief SoftAP Provisioning Sample application thread calling function 
 * @To test softap provisioning change below three definitions
 * @ 1 : sys_common_feature.h : #define	__SUPPORT_BLE_PROVISIONING__	for BLE Provisioning working
 * @ 2 : sys_common_feature.h : #define __SUPPORT_WIFI_CONN_CB__ for connection check
 * @param[in] arg - transfer information
 * @return  void
 ****************************************************************************************
 */
void ble_provisioning_sample(ULONG arg)
{
#if defined ( __SUPPORT_WIFI_CONN_CB__ )
    EventBits_t wifi_conn_ev_bits;
	uint8_t cnt = 0;
		
	PRINTF("\n>>> Start BLE COMBO Provisioning Sample Through Mobile App. <<<\n");
  
	while(1) {
        wifi_conn_ev_bits = xEventGroupWaitBits(evt_grp_wifi_conn_notify,
                                                WIFI_CONN_SUCC_STA_4_BLE,
                                                pdTRUE,
                                                pdFALSE,
                                                (1000 / portTICK_PERIOD_MS));

       if (wifi_conn_ev_bits & WIFI_CONN_SUCC_STA_4_BLE) {
            xEventGroupClearBits(evt_grp_wifi_conn_notify, WIFI_CONN_SUCC_STA_4_BLE);

            PRINTF("\n\nSuccess to connect Wi-Fi ...\n\n");

			break;		
       } else {
            if (cnt%5 == 3) {		
                if (ENABLE_FACTORYSET == 0) {  
                    PRINTF("\n>>> PRESS THE FACTORY RESET BUTTON AND RUN Bluetooth® Low Energy CONNECTION <<<\n");			

#if defined (__PROVISION_ATCMD__)
                    atcmd_provstat(ATCMD_PROVISION_IDLE);
#endif    
                }   
			}

			cnt++;
			if (cnt >= 255) {
				cnt = 0;
			}
       }
	}
#endif  // __SUPPORT_WIFI_CONN_CB__
}
#endif 	// __BLE_PROVISIONING_SAMPLE__


/* EOF */
