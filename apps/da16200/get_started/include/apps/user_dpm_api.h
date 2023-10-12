/**
 ****************************************************************************************
 *
 * @file user_dpm_api.h
 *
 * @brief Defined User DPM APIs.
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


#ifndef __USER_DPM_API_H__
#define    __USER_DPM_API_H__

#include <stdbool.h>
#include "sdk_type.h"
#include "da16x_system.h"
#include "user_dpm.h"
#include "da16x_types.h"
#if defined (XIP_CACHE_BOOT)
#include "da16x_dpm.h"
#endif // XIP_CACHE_BOOT

/* Defines */
#define DPM_ENABLED            1
#define DPM_DISABLED           0

#define DPM_WAKEUP             1
#define NORMAL_BOOT            0

#if !defined (DEF_DPM_WAKEUP_TYPE)
#define DEF_DPM_WAKEUP_TYPE
/// DPM Wakeup type
enum DPM_WAKEUP_TYPE {
    DPM_UNKNOWN_WAKEUP     = 0,
    DPM_RTCTIME_WAKEUP     = 1,
    DPM_PACKET_WAKEUP      = 2,
    DPM_USER_WAKEUP        = 3,
    DPM_NOACK_WAKEUP       = 4,
    DPM_DEAUTH_WAKEUP      = 5,
    DPM_TIM_ERR_WAKEUP     = 6,
    DPM_DDPS_BUFP_WAKEUP    = 7
};
#endif // DEF_DPM_WAKEUP_TYPE

/**
 ****************************************************************************************
 * @brief Enable DPM mode.
 ****************************************************************************************
 */
void dpm_mode_enable(void);

/**
 ****************************************************************************************
 * @brief Disable DPM mode.
 ****************************************************************************************
 */
void dpm_mode_disable(void);

/**
 ****************************************************************************************
 * @brief Return current DPM mode is on or not.
 *
 * @return pdTURE if DPM mode is on.
 ****************************************************************************************
 */
int dpm_mode_is_enabled(void);

/**
 ****************************************************************************************
 * @brief Get whether DA16200(or DA16600) wakes up or not.
 *
 * @return pdTRUE if DA16200(or DA16600) wakes up in DPM mode.
 ****************************************************************************************
 */
int dpm_mode_is_wakeup(void);

/**
 ****************************************************************************************
 * @brief Get DPM wake up source.
 *
 * @return DPM wakeup source. It's defined in WAKEUP_SOURCE.
 ****************************************************************************************
 */
int dpm_mode_get_wakeup_source(void);

/**
 ****************************************************************************************
 * @brief Get DPM wakeup type.
 *
 * @return DPM wakeup type. It's defined in DPM_WAKEUP_TYPE.
 ****************************************************************************************
 */
int dpm_mode_get_wakeup_type(void);

/**
 ****************************************************************************************
 * @brief Get state of what DA16200(or DA16600) goes to DPM sleep.
 *
 * @return State of what DA16200(or DA16600) goes to DPM sleep.
 ****************************************************************************************
 */
int dpm_sleep_is_started(void);

/**
 ****************************************************************************************
 * @brief Goto sleep mode 2. DA16200 wakes up only by external wake-up signal or RTC timer.
 * @param[in] usec Wake-up time, how long DA16200 is in DPM sleep mode 2.
 *                 If 0, DA16200 wakes up only by external wake-up signal.
 * @param[in] retention Power on RTM or not during DPM sleep mode 2.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
int dpm_sleep_start_mode_2(unsigned long long usec, unsigned char retention);

/**
 ****************************************************************************************
 * @brief Goto sleep mode 3. DA16200 wakes up only by external wake-up signal or RTC timer.
 * @param[in] usec Wake-up time, how long DA16200 is in sleep mode 3.
 *                 If 0, DA16200 wakes up only by external wake-up signal.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
int dpm_sleep_start_mode_3(unsigned long long usec);

/**
 ****************************************************************************************
 * @brief Register name of DPM module.
 * @param[in] mod_name Name of DPM module.
 * @param[in] port_number Port number of DPM module. If not required, the value can be 0.
 *
 * @return 0(DPM_REG_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_register(char *mod_name, unsigned int port_number);

/**
 ****************************************************************************************
 * @brief Deregister name of DPM module.
 * @param[in] mod_name Name of DPM module.
 ****************************************************************************************
 */
void dpm_app_unregister(char *mod_name);

/**
 ****************************************************************************************
 * @brief Set the DPM module to goto DPM sleep.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_sleep_ready_set(char *mod_name);

/**
 ****************************************************************************************
 * @brief Name of DPM module.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_sleep_ready_clear(char *mod_name);

/**
 ****************************************************************************************
 * @brief Set the DPM module to ready to get RTC timer callback function.
 *        If not set, RTC timer callback function won't be called.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_wakeup_done(char *mod_name);

/**
 ****************************************************************************************
 * @brief Set the DPM module to ready data transmission using registered name of DPM module.
 *        If not set, received data will be dropped.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_data_rcv_ready_set(char *mod_name);

/**
 ****************************************************************************************
 * @brief Set the DPM module to ready data transmission using registered port number.
 * @param[in] port Port number of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int dpm_app_rcv_ready_set_by_port(unsigned int port);

/**
 ****************************************************************************************
 * @brief Get status of user retention memory can be accessed.
 *
 * @return 1(pdTURE) if user retention memory is ready to access.
 ****************************************************************************************
 */
int dpm_user_mem_init_check(void);

/**
 ****************************************************************************************
 * @brief Allocate bytes from user retention memory area.
 * @param[in] name Specified name of allocated memory.
 * @param[out] memory_ptr Pointer to place allocated bytes pointer.
 * @param[in] memory_size Number of bytes to allocate.
 * @param[in] wait_option Suspension option. It's not used in v2.3.x.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
unsigned int dpm_user_mem_alloc(char *name, void **memory_ptr,
                                unsigned long memory_size, unsigned long wait_option);

/**
 ****************************************************************************************
 * @brief Release previously allocated memory to user retention memory.
 * @param[in] name Specified name of allocated memory.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
unsigned int dpm_user_mem_free(char *name);

/**
 ****************************************************************************************
 * @brief Get allocated memory from user retention memory area.
 * @param[in] name Specified name of allocated memory.
 * @param[out] data Pointer of allocated bytes pointer.
 *
 * @return Length of allocated bytes.
 ****************************************************************************************
 */
unsigned int dpm_user_mem_get(char *name, unsigned char **data);

/**
 ****************************************************************************************
 * @brief In DPM sleep mode, the timer count is also activated,
 *        and when the timeout event occurs,
 *        the DA16200 wakes up and calls the registered callbacke function.
 * @param[in] task_name Name of the process that wants to use the DPM timer.
 *                      It must be the same as the name registered
 *                      using the dpm_app_register() function.
 * @param[in] timer_name A string of 7 characters or less as a unique character
 *                       to distinguish timer.
 * @param[in] callback_func Function pointer to be called when timerout occurs.
 *                          If there is no function you want to call, enter NULL.
 * @param[in] msec Timeout occurrence time.
 * @param reschedule_msec Periodic timeout occurrence time.
 *                        When set to 0, timeout occurs only once with the time set as the value of msec.
 *
 * @return Internally set Timer ID 5 ~ 15. If Timer ID > 16, it is a failure.
 ****************************************************************************************
 */
int dpm_timer_create(char *task_name, char *timer_name,
                     void (* callback_func)(char *timer_name),
                     unsigned int msec, unsigned int reschedule_msec);

/**
 ****************************************************************************************
 * @brief Delete the registered timer.
 * @param[in] task_name Name used to register the timer using dpm_timer_create() function.
 * @param[in] timer_name Name of the timer to be deleted.
 *                       It is the same name used when registering timer using dpm_timer_create() function.
 *
 * @return Success: Timer IDs 5 to 15 are returned.
 *         Failed: -1 or -2 is returned.
 ****************************************************************************************
 */
int dpm_timer_delete(char *task_name, char *timer_name);

/**
 ****************************************************************************************
 * @brief Change the timeout occurrence time of an already running timer.
 * @param[in] task_name Name used to register the timer using dpm_timer_create() function.
 * @param[in] timer_name Timer name to change the time.  This is the same name used
 *                       when registering the timer using the dpm_timer_create() function.
 * @param[in] msec Time value to change.
 *
 * @return Success: Timer IDs 5 to 15 are returned.
           Failed: -1 is returned.
 ****************************************************************************************
 */
int dpm_timer_change(char *task_name, char *timer_name, unsigned int msec);

/**
 ****************************************************************************************
 * @brief Enable UDP filter functionality. If enabled, DA16200(or DA16600) allowes
 *        to receive UDP packet of registered port in DPM sleep.
 * @param[in] en_flag Flag to enable UDP filter functionality.
 ****************************************************************************************
 */
void dpm_udp_filter_enable(unsigned char en_flag);

/**
 ****************************************************************************************
 * @brief Set port number of UDP to allow to receive UDP packet in DPM sleep.
 *        The maximum is DPM_MAX_UDP_FILTER(8).
 * @param[in] d_port Port number of UDP.
 ****************************************************************************************
 */
void dpm_udp_port_filter_set(unsigned short d_port);

/**
 ****************************************************************************************
 * @brief Delete port number of UDP. It's set by dpm_udp_port_filter_set() function.
 * @param[in] d_port Port number of UDP.
 ****************************************************************************************
 */
void dpm_udp_port_filter_delete(unsigned short d_port);

/**
 ****************************************************************************************
 * @brief Set UDP hole puncing functionality.
 * @param[in] period Period of UDP hole punching functionality.
 * @param[in] dest_ip Destination IP address.
 * @param[in] src_port Source port number.
 * @param[in] dest_port Destination port number.
 ****************************************************************************************
 */
void dpm_udp_hole_punch_set(int period, unsigned long dest_ip,
                            unsigned short src_port, unsigned short dest_port);

/**
 ****************************************************************************************
 * @brief Enable TCP filter functionality.
 *        If enabled, DA16200(or DA16600) allowes to receive TCP packet of registered port in DPM sleep.
 * @param[in] en_flag Flag to enable TCP filter functionality.
 ****************************************************************************************
 */
void dpm_tcp_filter_enable(unsigned char en_flag);

/**
 ****************************************************************************************
 * @brief Set port number of UDP to allow to receive TCP packet in DPM sleep.
 *        The maximum is DPM_MAX_TCP_FILTER(8).
 * @param[in] d_port Port number of TCP.
 ****************************************************************************************
 */
void dpm_tcp_port_filter_set(unsigned short d_port);

/**
 ****************************************************************************************
 * @brief Delete port number of TCP. It's set by dpm_tcp_port_filter_set() function.
 * @param[in] d_port Port number of TCP.
 ****************************************************************************************
 */
void dpm_tcp_port_filter_delete(unsigned short d_port);

/**
 ****************************************************************************************
 * @brief Enable ARP in DPM mode..
 * @param[in] period Period.
 * @param[in] mode Enable to ARP.
 ****************************************************************************************
 */
void dpm_arp_enable(int period, int mode);

/**
 ****************************************************************************************
 * @brief Get whether DPM mode is on or not.
 *
 * @return 1 if DPM mode is on.
 ****************************************************************************************
 */
int dpm_mode_get(void);

/**
 ****************************************************************************************
 * @brief Check whether the DPM module is registered or not.
 * @param[in] mod_name Name of DPM module.
 *
 * @return DPM_REG_DUP_NAME(9999) if DPM module is registered.
 ****************************************************************************************
 */
int dpm_app_is_register(char *mod_name);

/**
 ****************************************************************************************
 * @brief Check whether the DPM module is able to goto DPM sleep or not.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 1 if the DPM module is able to goto DPM sleep.
 ****************************************************************************************
 */
int dpm_app_is_sleep_ready_set(char *mod_name);

/**
 ****************************************************************************************
 * @brief Check what the registered name of DPM module is with the port number.
 * @param[in] port Port number of DPM module.
 *
 * @return Pointer of Name of DPM module if the port number is registered.
 ****************************************************************************************
 */
char *dpm_app_is_register_port(unsigned int port);

/**
 ****************************************************************************************
 * @brief Set period of DTIM. If saving the period of DTIM to NVRAM is required,
 *        the period of DTIM will be applied after system reboot.
 * @param[in] dtim_period Period of DTIM in 100 msec. If 0, default period(100 msec) sets up.
 *            The range is from 1 to 65535.
 * @param[in] flag Flag to save period of DTIM to NVRAM.
 ****************************************************************************************
 */
void dpm_tim_wakeup_duration_set(unsigned int dtim_period, int flag);

/**
 ****************************************************************************************
 * @brief Set IP muticast address to allow receving packet in DPM sleep.
 * @param[in] mc_addr IP multicast address.
 ****************************************************************************************
 */
void dpm_mc_filter_set(unsigned long mc_addr);

/**
 ****************************************************************************************
 * @brief Check the remaining timeout occurrence time of an already running timer.
 * @param[in] thread_name Name used to register the timer using dpm_timer_create() function.
 * @param[in] timer_name Timer name to check the remaining time.
 *                       This is the same name used
 *                       when registering the timer using the dpm_timer_create() function.
 *
 * @return Success: Number of millisecond remaining until timeout is returned.
 *         Failed: -1 returned.
 ****************************************************************************************
 */
int dpm_timer_remaining_msec_get(char *thread_name, char *timer_name);

/**
 ****************************************************************************************
 * @brief Check whether the DPM module is ready to get RTC timer callback function or not.
 *        It will be internally waiting for 1 sec if it's not ready.
 * @param[in] mod_name Nmae of DPM module.
 *
 * @return 1, if the DPM module is ready to get RTC timer callback function.
 ****************************************************************************************
 */
bool dpm_app_is_wakeup_done(char *mod_name);

/**
 ****************************************************************************************
 * @brief Check if the system has been woken up from Abnormal DPM Sleep
 *
 * @return pdTRUE if DA16200(or DA16600) is woken up from Abnormal DPM Sleep
 ****************************************************************************************
 */
unsigned char dpm_mode_is_abnormal_wakeup(void);

/**
 ****************************************************************************************
 * @brief Start/Stop DPM-Abnormal status check timer
 *
 * @param[in] flag 1 is start DPM-Abnormal status check timer, 0 is stop it.
 * @return pdTRUE: Success to config DPM Abnormal timer operation
 *         pdFALSE: Not supported mode
 ****************************************************************************************
 */
char config_dpm_abnormal_timer(char flag);

#endif    /* __USER_DPM_API_H__ */

/* EOF */
