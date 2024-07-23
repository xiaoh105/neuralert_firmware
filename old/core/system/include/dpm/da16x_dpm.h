/**
 ****************************************************************************************
 *
 * @file da16x_dpm.h
 *
 * @brief Define/Macro for DPM daemon and operations
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


#ifndef	__DA16X_DPM_H__
#define	__DA16X_DPM_H__

#include <stdbool.h>
#include "da16x_dpm_regs.h"

#undef	DPM_TIMER_DEBUG
#define USE_SET_DPM_INIT_DONE

////////////////////////////////////////////////////////////////////////////
#define	LIGHT_RED_COL				"\33[0;31m"
#define	LIGHT_GRN_COL				"\33[0;32m"
#define	LIGHT_YEL_COL				"\33[0;33m"
#define	LIGHT_BLU_COL				"\33[0;34m"
#define	LIGHT_MAG_COL				"\33[0;35m"
#define	LIGHT_CYN_COL				"\33[0;36m"
#define	LIGHT_WHT_COL				"\33[0;37m"

#define	RED_COL						"\33[1;31m"
#define	GRN_COL						"\33[1;32m"
#define	YEL_COL						"\33[1;33m"
#define	BLU_COL						"\33[1;34m"
#define	MAG_COL						"\33[1;35m"
#define	CYN_COL						"\33[1;36m"
#define	WHT_COL						"\33[1;37m"

#define	CLR_COL						"\33[0m"
////////////////////////////////////////////////////////////////////////////

#define	MAX_DPM_REQ_CNT				32

/* NVRAM KEY */

#define	REG_NAME_DPM_KEY			"DPM_KEY"
#define	REG_NAME_DISCON_LOSS		"Disconnect_loss"
#define	REG_NAME_DPM_UMAC			"DPM_UMAC"
#define	REG_NAME_DPM_UMAC_TX		"DPM_UMAC_TX"
#define	REG_NAME_DPM_UMAC_RX		"DPM_UMAC_RX"
#define	REG_NAME_DPM_SUPPLICANT		"DPM_SUPP"
#define	REG_NAME_DPM_TIMER_PROC		"DPM_TIMER"

#define WAIT_DPM_SLEEP				0
#define RUN_DPM_SLEEP				1
#define DONE_DPM_SLEEP				2

#define	DPM_OK						0
#define	DPM_REG_OK					0
#define	DPM_SET_OK					0

#define	DPM_REG_ERR					-1
#define	DPM_SET_ERR					-2
#define	DPM_SET_MUTEX_ERR			-3
#define	DPM_SET_ERR_BLOCK			-9

#define DPM_NOT_DPM_MODE			7777
#define DPM_NOT_REGISTERED			8888
#define DPM_REG_DUP_NAME			9999

//__CHK_DPM_ABNORM_STS__ - Start
#define DPM_UNDEFINED				0x000000
#define DPM_UC						0x000001 /* UC */
#define DPM_BC_MC					0x000002 /* BC/MC */
#define DPM_BCN_CHANGED				0x000004
#define DPM_NO_BCN					0x000008
#define DPM_FROM_FAST				0x000010
#define DPM_KEEP_ALIVE_NO_ACK		0x000020
#define DPM_FROM_KEEP_ALIVE			0x000040
#define DPM_NO						0x000080
#define DPM_UC_MORE					0x000200 /* UC more */
#define DPM_AP_RESET				0x000400
#define DPM_DEAUTH					0x000800
#define DPM_DETECTED_STA			0x001000
#define DPM_FROM_FULL				0x002000
#define DPM_USER0					0x080000
#define DPM_USER1					0x100000

#define FUNC_ERROR					100
#define FUNC_STATUS					200

#define STATUS_WAKEUP				(FUNC_STATUS + 1)
#define STATUS_POR					(FUNC_STATUS + 2)
//__CHK_DPM_ABNORM_STS__ - End

#define REG_NAME_DPM_MAX_LEN		20
#define DPM_TIMER_NAME_MAX_LEN		8

enum DPM_TID {
	TID_USER_WAKEUP = 2,
	TID_DHCP_CLIENT,
	TID_ABNORMAL
};

//////////////////////////////////////////////////////////
#define USER_RTM_POOL_NAME	"user_rtm_pool"

typedef struct _dpm_user_rtm {
	char		name[REG_NAME_DPM_MAX_LEN];
	unsigned char	*start_user_addr;
	unsigned int	size;

	struct _dpm_user_rtm *next_user_addr;
} dpm_user_rtm;

typedef struct _dpm_user_rtm_pool {
	void		*free_ptr;
	dpm_user_rtm	*first_user_addr;
} dpm_user_rtm_pool;

typedef struct _dpm_mdns_info {
	char hostname[64];
	size_t len;
} dpm_mdns_info_t;

#undef	WORKING_FOR_DPM_TCP	// For DPM TCP Session

#if defined ( WORKING_FOR_DPM_TCP )
/* RTC Timer ID for TCP Session Keep-Alive */
typedef struct {
        char    tcp_sess_name[DPM_MAX_TCP_SESS][REG_NAME_DPM_MAX_LEN];
        unsigned char	rtc_tid[DPM_MAX_TCP_SESS];
} dpm_tcp_ka_info_t;
#endif /* WORKING_FOR_DPM_TCP */

//////////////////////////////////////////////////////////

extern dpm_supp_conn_info_t *dpm_supp_conn_info;
extern dpm_supp_key_info_t *dpm_supp_key_info;

extern int show_info_periodically;
extern int show_info_term_by_sec;

//////////////////////////////////////////////////////////////////////////////
///Inilne function ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static inline char *dpm_strcpy(char *s1, char *s2)
{
	char *s = s1;

	while ((*s++ = *s2++) != 0)
		;

	return (s1);
}

static inline size_t dpm_strlen(char *str)
{
	char *s;

	for (s = str; *s; ++s);

	return (size_t)(s - str);
}

static inline int dpm_strcmp(const char *s1, const char *s2)
{
	for ( ; *s1 == *s2; s1++, s2++) {
		if (*s1 == '\0')
			return 0;
	}

	return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
}

static inline int dpm_strncmp(const char *s1, const char *s2, size_t n)
{
	for ( ; n > 0; s1++, s2++, --n) {
		if (*s1 != *s2)
			return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
		else if (*s1 == '\0')
			return 0;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
/// Internal Functions    ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void start_dpm_sleep_daemon(int tim_status);

void show_dpm_sleep_info(void);

int	chk_rtm_exist(void);

bool chk_retention_mem_exist(void);

unsigned int current_usec_count(void);

int	chk_dpm_sleepd_hold_state(void);

void hold_dpm_sleepd(void);

void resume_dpm_sleepd(void);

void dpm_set_mdns_info(const char *hostname, size_t len);

int	dpm_get_mdns_info(char *hostname);

void dpm_clear_mdns_info(void);

void set_multicast_for_zeroconfig(void);

int	get_supp_conn_state(void);

int	chk_supp_connected(void);

void save_supp_conn_state(int state);

const char *dpm_restore_country_code(void);

void dpm_save_country_code(const char *country);

void clr_dpm_conn_info(void);

void init_dpm_environment(void);

void reset_dpm_info(void);

void set_dpm_keepalive_config(int duration);

int	get_dpm_keepalive_config(void);

void set_systimeoffset_to_rtm(unsigned long long offset);

unsigned long long get_systimeoffset_from_rtm(void);

void set_rtc_oldtime_to_rtm(unsigned long long time);

unsigned long long get_rtc_oldtime_from_rtm(void);

void set_rtc_offset_to_rtm(unsigned long long offset);

unsigned long long get_rtc_offset_from_rtm(void);

void set_timezone_to_rtm(long timezone);

long get_timezone_from_rtm(void);

void set_sntp_use_to_rtm(UINT use);

UINT get_sntp_use_from_rtm(void);

void set_sntp_period_to_rtm(long period);

ULONG get_sntp_period_from_rtm(void);

void set_sntp_timeout_to_rtm(long timezone);

ULONG get_sntp_timeout_from_rtm(void);

UCHAR *get_da16x_dpm_dns_cache(void);

unsigned char *get_da16x_dpm_mdns_ptr(void);

unsigned char *get_da16x_dpm_arp_ptr(void);

int	set_dpm_TIM_wakeup_time_to_nvram(int sec);

void set_dpm_bcn_wait_timeout(unsigned int msec);

void set_dpm_nobcn_check_step(int step);

void dpm_user_rtm_clear(void);

void dpm_user_rtm_pool_clear(void);

unsigned int dpm_user_rtm_pool_create(void);

unsigned int dpm_user_rtm_pool_delete(void);

void dpm_user_rtm_add(dpm_user_rtm *data);

dpm_user_rtm *dpm_user_rtm_remove(char *name);

dpm_user_rtm *dpm_user_rtm_search(char *name);

unsigned int dpm_user_rtm_pool_info_get(void);

void dpm_user_rtm_print(void);

void dpm_arp_en_timsch(int mode);

void dpm_arp_req_en(int period, int mode);

void *get_supp_net_info_ptr(void);

void *get_supp_ip_info_ptr(void);

void *get_supp_conn_info_ptr(void);

void *get_supp_key_info_ptr(void);

void *get_rtc_timer_ops_ptr(void);

unsigned int get_dpm_dbg_level(void);

void set_dpm_dbg_level(unsigned int level);

//////////////////////////////////////////////////////////////////////////////
/// User API Functions    ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/*
 ****************************************************************************************
 * @brief Enable DPM mode.
 ****************************************************************************************
 */
void enable_dpm_mode(void);

/*
 ****************************************************************************************
 * @brief Disable DPM mode.
 ****************************************************************************************
 */
void disable_dpm_mode(void);

/*
 ****************************************************************************************
 * @brief Return current DPM mode is on or not.
 *
 * @return pdTURE if DPM mode is on.
 ****************************************************************************************
 */
int	chk_dpm_mode(void);

/*
 ****************************************************************************************
 * @brief Get whether DA16200(or DA16600) wakes up or not.
 *
 * @return pdTRUE if DA16200(or DA16600) wakes up in DPM mode.
 ****************************************************************************************
 */
int	chk_dpm_wakeup(void);

/*
 ****************************************************************************************
 * @brief Get state of what DA16200(or DA16600) goes to DPM sleep.
 *
 * @return State of what DA16200(or DA16600) goes to DPM sleep.
 ****************************************************************************************
 */
int	chk_dpm_pdown_start(void);

/*
 ****************************************************************************************
 * @brief Goto sleep mode 2. DA16200 wakes up only by external wake-up signal or RTC timer.
 * @param[in] usec Wake-up time, how long DA16200 is in DPM sleep mode 2.
 *                 If 0, DA16200 wakes up only by external wake-up signal.
 * @param[in] retention Power on RTM or not during DPM sleep mode 2.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
int start_dpm_sleep_mode_2(unsigned long long usec, unsigned char retention);

/*
 ****************************************************************************************
 * @brief Register name of DPM module.
 * @param[in] mod_name Name of DPM module.
 * @param[in] port_number Port number of DPM module. If not required, the value can be 0.
 *
 * @return 0(DPM_REG_OK) if successful.
 ****************************************************************************************
 */
int	reg_dpm_sleep(char *mod_name, unsigned int port_number);

/*
 ****************************************************************************************
 * @brief Deregister name of DPM module.
 * @param[in] mod_name Name of DPM module.
 ****************************************************************************************
 */
void unreg_dpm_sleep(char *mod_name);

/*
 ****************************************************************************************
 * @brief Set the DPM module to goto DPM sleep.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int	set_dpm_sleep(char *mod_name);

/*
 ****************************************************************************************
 * @brief Name of DPM module.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int	clr_dpm_sleep(char *mod_name);

/*
 ****************************************************************************************
 * @brief Set the DPM module to ready to get RTC timer callback function.
 *        If not set, RTC timer callback function won't be called.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int set_dpm_init_done(char *mod_name);

/*
 ****************************************************************************************
 * @brief Set the DPM module to ready data transmission using registered name of DPM module.
 *        If not set, received data will be dropped.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int set_dpm_rcv_ready(char *mod_name);

/*
 ****************************************************************************************
 * @brief Set the DPM module to ready data transmission using registered port number.
 * @param[in] port Port number of DPM module.
 *
 * @return 0(DPM_SET_OK) if successful.
 ****************************************************************************************
 */
int set_dpm_rcv_ready_by_port(unsigned int port);

/*
 ****************************************************************************************
 * @brief Get status of user retention memory can be accessed.
 *
 * @return 1(pdTURE) if user retention memory is ready to access.
 ****************************************************************************************
 */
int dpm_user_rtm_pool_init_chk(void);

/*
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
unsigned int dpm_user_rtm_allocate(char *name, void **memory_ptr, unsigned long memory_size, unsigned long wait_option);

/*
 ****************************************************************************************
 * @brief Release previously allocated memory to user retention memory.
 * @param[in] name Specified name of allocated memory.
 *
 * @return 0 if successful.
 ****************************************************************************************
 */
unsigned int dpm_user_rtm_release(char *name);

/*
 ****************************************************************************************
 * @brief Get allocated memory from user retention memory area.
 * @param[in] name Specified name of allocated memory.
 * @param[out] data Pointer of allocated bytes pointer.
 *
 * @return Length of allocated bytes.
 ****************************************************************************************
 */
unsigned int dpm_user_rtm_get(char *name, unsigned char **data);

/*
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
int	dpm_user_timer_create(char *thread_name, char *timer_name, void (* callback_func)(char *name), unsigned int msec, unsigned int reschedule_msec);

/*
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
int	dpm_user_timer_delete(char *thread_name, char *timer_name);

/*
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
int	dpm_user_timer_change(char *thread_name, char *timer_name, unsigned int msec);

/*
 ****************************************************************************************
 * @brief Enable UDP filter functionality. If enabled, DA16200(or DA16600) allowes
 *        to receive UDP packet of registered port in DPM sleep.
 * @param[in] en_flag Flag to enable UDP filter functionality.
 ****************************************************************************************
 */
void dpm_udpf_cntrl(unsigned char en_flag);

/*
 ****************************************************************************************
 * @brief Set port number of UDP to allow to receive UDP packet in DPM sleep.
 *        The maximum is DPM_MAX_UDP_FILTER(8).
 * @param[in] d_port Port number of UDP.
 ****************************************************************************************
 */
void set_dpm_udp_port_filter(unsigned short d_port);

/*
 ****************************************************************************************
 * @brief Delete port number of UDP. It's set by dpm_udp_port_filter_set() function.
 * @param[in] d_port Port number of UDP.
 ****************************************************************************************
 */
void del_dpm_udp_port_filter(unsigned short d_port);

/*
 ****************************************************************************************
 * @brief Set UDP hole puncing functionality.
 * @param[in] period Period of UDP hole punching functionality.
 * @param[in] dest_ip Destination IP address.
 * @param[in] src_port Source port number.
 * @param[in] dest_port Destination port number.
 ****************************************************************************************
 */
void dpm_udp_hole_punch(int period /* keep period times */,
		unsigned long dest_ip, unsigned short src_port,
		unsigned short dest_port);

/*
 ****************************************************************************************
 * @brief Enable TCP filter functionality.
 *        If enabled, DA16200(or DA16600) allowes to receive TCP packet of registered port in DPM sleep.
 * @param[in] en_flag Flag to enable TCP filter functionality.
 ****************************************************************************************
 */
void dpm_tcpf_cntrl(unsigned char en_flag);

/*
 ****************************************************************************************
 * @brief Set port number of UDP to allow to receive TCP packet in DPM sleep.
 *        The maximum is DPM_MAX_TCP_FILTER(8).
 * @param[in] d_port Port number of TCP.
 ****************************************************************************************
 */
void set_dpm_tcp_port_filter(unsigned short d_port);

/*
 ****************************************************************************************
 * @brief Delete port number of TCP. It's set by dpm_tcp_port_filter_set() function.
 * @param[in] d_port Port number of TCP.
 ****************************************************************************************
 */
void del_dpm_tcp_port_filter(unsigned short d_port);

/*
 ****************************************************************************************
 * @brief Get whether DPM mode is on or not.1 if DPM mode is on.
 *
 * @return 1 if DPM mode is on.
 ****************************************************************************************
 */
int	get_dpm_mode(void);

/*
 ****************************************************************************************
 * @brief Check whether the DPM module is registered or not.
 * @param[in] mod_name Name of DPM module.
 *
 * @return DPM_REG_DUP_NAME(9999) if DPM module is registered.
 ****************************************************************************************
 */
int chk_dpm_reg_state(char *mod_name);

/*
 ****************************************************************************************
 * @brief Check whether the DPM module is able to goto DPM sleep or not.
 * @param[in] mod_name Name of DPM module.
 *
 * @return 1 if the DPM module is able to goto DPM sleep.
 ****************************************************************************************
 */
int	chk_dpm_set_state(char *mod_name);

/*
 ****************************************************************************************
 * @brief Check what the registered name of DPM module is with the port number.
 * @param[in] port_number Port number of DPM module.
 *
 * @return Pointer of Name of DPM module if the port number is registered.
 ****************************************************************************************
 */
char *chk_dpm_reg_port(unsigned int port_number);

/*
 ****************************************************************************************
 * @brief Set period of DTIM. If saving the period of DTIM to NVRAM is required,
 *        the period of DTIM will be applied after system reboot.
 * @param[in] dtim_period Period of DTIM in 100 msec. If 0, default period(100 msec) sets up.
 *            The range is from 1 to 65535.
 * @param[in] flag Flag to save period of DTIM to NVRAM.
 ****************************************************************************************
 */
void set_dpm_tim_wakeup_dur(UINT dtim_period , int flag);

/*
 ****************************************************************************************
 * @brief Set IP muticast address to allow receving packet in DPM sleep.
 * @param[in] mc_addr IP multicast address.
 ****************************************************************************************
 */
void set_dpm_mc_filter(ULONG mc_addr);

/*
 ****************************************************************************************
 * @brief Check the remaining timeout occurrence time of an already running timer.
 * @param[in] thread_name Name used to register the timer using dpm_timer_create() function.
 * @param[in] timer_name Timer name to check the remaining time.
 *                       This is the same name used
 *                       when registering the timer using the dpm_timer_create() function.
 *
 * @return Success: Number of seconds remaining until timeout is returned.
 *         Failed: -1 returned.
 ****************************************************************************************
 */
int	dpm_user_timer_get_remaining_msec(char *thread_name, char *timer_name);

/*
 ****************************************************************************************
 * @brief Check whether the DPM module is ready to get RTC timer callback function or not.
 *        It will be internally waiting for 1 sec if it's not ready.
 * @param[in] mod_name Nmae of DPM module.
 *
 * @return 1, if the DPM module is ready to get RTC timer callback function.
 ****************************************************************************************
 */
bool confirm_dpm_init_done(char *mod_name);

/*
 ****************************************************************************************
 * @brief Enable ARP in DPM mode..
 * @param[in] period Period.
 * @param[in] mode Enable to ARP.
 ****************************************************************************************
 */
void dpm_arp_en(int period, int mode);

void enable_dpm_wakeup(void);

void disable_dpm_wakeup(void);

void dpm_user_timer_list_print(void);

/*
 ****************************************************************************************
 * @brief Delete all registered timers.
 * @param[in] if 1, only the last remaining timeout of each timer is occurred and all are deleted.
 *            if 2, all timers are deleted immediately.
 *
 * @return 1(pdTRUE) if successful.
 ****************************************************************************************
 */
int	dpm_user_timer_delete_all(unsigned int level);

/*
 ****************************************************************************************
 * @brief In DPM sleep mode, the timer count is also activated,
 *        and when the timeout event occurs,
 *        the DA16200 wakes up and calls the registered callback function.
 * @param[in] msec Timeout setting in seconds.
 * @param[in] name NULL or a character string of 20 bytes or less.
 *                 If the user program that calls the RTC timer is registered
 *                 and operated in DPM Manager, the same string
 *                 as the name string registered in DPM Manager must be entered.
 *                 If it is a user program that is not related to DPM Manager, enter Null.
 * @param[in] timer_id If 0, an available idle timer index value is automatically assigned. (recommend)
 * @param[in] peri If 0, timeout occurs once and then ends. (Non Periodic)
 *                 If 1, it is automatically re-registered after timeout occurs. (Periodic)
 * @param[in] callback_func Function pointer of the callback function
 *                          to be executed when timeout occurs.
 *
 * @return Timer IDs 5 to 15 are returned if successful.  -1 if failure.
 ****************************************************************************************
 */
int rtc_register_timer(unsigned int msec, char *name, int timer_id, int peri, void (* callback_func)(char *timer_name));

/*
 ****************************************************************************************
 * @brief Delete the registered timer.
 * @param[in] timer_id RTC timer ID to delete.
 *
 * @return Timer IDs 5 to 15 if successful.
 ****************************************************************************************
 */
int rtc_cancel_timer(int timer_id);

/*
 ****************************************************************************************
 * @brief Print registered timer information to the terminal.
 ****************************************************************************************
 */
void rtc_timer_list_info(void);

/*
 ****************************************************************************************
 * @brief Get the remaining time of a registered timer.
 * @param[in] tid RTC timer ID to inquire the remaining time.
 *
 * @return Remaining timeout msec if successful.
 ****************************************************************************************
 */
int	rtc_timer_info(int tid);

int	dpm_daemon_regist_sleep_cb(void (*regConfigFunction)());

extern UCHAR get_last_dpm_sleep_type(void);

#endif	/* __DA16X_DPM_H__ */

/* EOF */
