/**
 ****************************************************************************************
 *
 * @file app_provision.h
 *
 * @brief BLE or soft AP Provision.
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

#if !defined(_APP_PROVISION_H_)
#define _APP_PROVISION_H_

/**
 * @brief DPM Set value for Provisioning.
 */
typedef enum
{
	TYPE_SLEEP_MODE= 0,
	TYPE_USE_DPM, 
	TYPE_RTC_TIME,
	TYPE_DPM_KEEP_ALIVE,
	TYPE_USER_WAKE_UP,
	TYPE_TIM_WAKE_UP,
} GetValueType;

/**
 ****************************************************************************************
 * @brief Get DPM set value 
 * @param[in] _type - get  value type 
 * @return  success : set value, fail : -1
 ****************************************************************************************
 */
int app_get_DPM_set_value( GetValueType _type );

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
    int _timWakeup);

#if defined (__BLE_PROVISIONING_SAMPLE__)

#define _NVRAM_SAVE_THING_FACORY_

#define MODEL_NAME 		"DA16600"

/**
 ****************************************************************************************
 * @brief checking Provision Running Status
 * @param[in] set  1 : Provision Enable,	0 : Provision Disable 
 * @return void 
 ****************************************************************************************
 */
void app_check_provision(int set);

#endif	//	__BLE_PROVISIONING_SAMPLE__

/**
 ****************************************************************************************
 * @brief checking internet or network by using ping server
 * @param[in] ipaddrstr ping server address
 * @return  1 : ok	0 : internet fail 
 ****************************************************************************************
 */
int32_t app_check_internet_with_ping(char  *ipaddrstr);

#endif	// _APP_PROVISION_H_

/* EOF */
