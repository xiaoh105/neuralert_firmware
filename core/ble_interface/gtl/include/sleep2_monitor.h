/**
 ****************************************************************************************
 *
 * @file sleep2_monitor.h
 *
 * @brief Header file for sleep2 monitor
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

#ifndef SLEEP2_MONITOR_H_
#define SLEEP2_MONITOR_H_

#include "sdk_type.h"
#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "da16_gtl_features.h"

#define __APP_SLEEP2_MONITOR__

#define SLEEP2_APP_NUM_DEF 8
#define SLEEP2_MON_NAME	"sleep2_mon"
#define	SLEEP2_MON_STK_SIZE	1024
#define SLEEP2_SET 1
#define SLEEP2_CLR 0
#define SLEEP2_APP_NAME_LEN 20
#define SLEEP2_MON_APP_LIST_MUTEX "slp2_app_lst_mutx"
#define SEC_TO_SLEEP 3600 // temp

#define SLEEP2_MON_NAME_IOT_SENSOR "gas_leak_sensor"
#define SLEEP2_MON_NAME_PROV "provisioning"

/// sleep2 application info
typedef struct __sleep2_app {
	char app_name[SLEEP2_APP_NAME_LEN];
	uint8_t state;
	uint8_t is_occupied;
} sleep2_app_t;

/**
 ****************************************************************************************
 * @brief     Configure Sleep2 Monitor
 * @param[in] sleep2_app_num            number of sleep2 applications Sleep2 Monitor
 *                                      manages
 * @param[in] app_todo_before_sleep2_fn a user function that is invoked before entering
 *                                      sleep2 
 *
 * @return void
 ****************************************************************************************
*/
void	sleep2_monitor_config(uint8_t sleep2_app_num, 
								void (*app_todo_before_sleep2_fn)(void));

/**
 ****************************************************************************************
 * @brief     Start Sleep2 Monitor
 *
 * @return void.
 ****************************************************************************************
*/
void	sleep2_monitor_start(void);

/**
 ****************************************************************************************
 * @brief     Register an application to Sleep2 Monitor
 * @param[in] sleep2_app_name  application name to register to Sleep2 Monitor
 * @param[in] fn_name          debug purpose. __func__ is passed
 * @param[in] line             debug purpose, __LINE__ is passed
 *
 * @return 0 if successful / -1 if not successful.
 ****************************************************************************************
*/
int8_t  sleep2_monitor_regi(char* sleep2_app_name, char const *fn_name, int line);

/**
 ****************************************************************************************
 * @brief     Set the state of an application for Sleep2 Monitor
 * @param[in] sleep2_app_name  application to set state
 * @param[in] state_to_set     SLEEP2_SET : tells Sleep2 Monitor that your job is done,
 *                                          thus ready to enter sleep2
 *                             SLEEP2_CLR : tells Sleep2 Monitor that you have started
 *                                          the job thus not ready to enter sleep2
 * @param[in] fn_name          debug purpose. __func__ is passed
 * @param[in] line             debug purpose, __LINE__ is passed
 *
 *
 * @return void
 ****************************************************************************************
*/
void	sleep2_monitor_set_state(char* sleep2_app_name, uint8_t state_to_set, 
									char const *fn_name, int line);

/**
 ****************************************************************************************
 * @brief     Get the state of an application in Sleep2 Monitor
 * @param[in] sleep2_app_name  application name to register to Sleep2 Monitor
 *
 * @return SLEEP2_SET / SLEEP2_CLR if successful, -1 if the application is not found
 ****************************************************************************************
*/
int8_t sleep2_monitor_get_state(char* sleep2_app_name);





/**
 ****************************************************************************************
 * @brief     Check if DPM Wake-up is done by DPM Abnormal Checker
 *
 * @return   TRUE if DPM Wake-up by DPM Abnormal Checker (e.g. Wi-Fi disconnection)
 *           FALSE if DPM Wake-up by normal DPM wake-up causes
 *
 * @note     DPM Abnormal Che
 *           deep sleep.
 *           If RTC and Timer1 are not going to be used as wake-up sources, it is
 *           recommended to close the PD_TIM prior to putting the system in deep sleep.
 *           The exit from the deep sleep state causes a system reboot.
 ****************************************************************************************
*/
int8_t  is_wakeup_by_dpm_abnormal_chk(void);


/**
 ****************************************************************************************
 * @brief     Hold DPM Abnormal Checker
 *
 * @details   DPM Abnormal Checker monitors something abnormal in DPM operations. e.g. 
 *            Wi-Fi disconnection (by router's power loss), failure in getting IP
 *            addresses from DHCP, etc. If these events (abnormal) occur, 
 *            DPM Abnormal Checker retries several times, and if not successful, make the 
 *            system enter sleep2 to save power. Before entering sleep2, DPM abnormal
 *            checker schedules wake-up to peridically check if the abnormal conditions 
 *            are resolved in the next wake-up.
 *
 *            If an application that has been registered to Sleep2 Monitor needs to keep 
 *            the system stay alive (although there's abnormal condition happens)
 *            until the job is done, it should invoke this function
 *
 * @return void.
 ****************************************************************************************
*/
extern void dpm_abnormal_chk_hold(void);

/**
 ****************************************************************************************
 * @brief     Resume DPM Abnormal Checker
 *
 * @details   DPM Abnormal Checker monitors something abnormal in DPM operations. e.g. 
 *            Wi-Fi disconnection (by router's power loss), failure in getting IP
 *            addresses from DHCP, etc. If these events (abnormal) occur, 
 *            DPM Abnormal Checker re-tries several times, and if not successful, make the 
 *            system enter sleep2 to save power. Before entering sleep2, DPM abnormal
 *            checker schedules wake-up to peridically check if the abnormal conditions 
 *            are resolved in the next wake-up
 *
 *            If an application that has been registered to Sleep2 Monitor needs live 
 *            network until the job is done, it should invoke this function
 *
 * @return void.
 ****************************************************************************************
*/
extern void dpm_abnormal_chk_resume(void);

#endif // SLEEP2_MONITOR_H_

/* EOF */
