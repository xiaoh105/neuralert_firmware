/**
 ****************************************************************************************
 *
 * @file da16x_dpm_regs.h
 *
 * @brief Define/Macro for DPM operation
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


#ifndef	__DA16X_DPM_REGS_H__
#define	__DA16X_DPM_REGS_H__

#include "sdk_type.h"

#include "da16x_system.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
////////////////////////////////////////////////////////////////////////////


#define DPM_ETH_ALEN			6
#define	DPM_MAX_SSID_LEN		32
#define DPM_MAX_WEP_KEY_LEN		16
#define	DPM_PSK_LEN				32

#define REG_NAME_DPM_MAX_LEN	20
#define DPM_TIMER_NAME_MAX_LEN	8

typedef struct _dpm_manage_table {
	char	mod_name[REG_NAME_DPM_MAX_LEN];
	unsigned long	bit_index;
	unsigned int	port_number;
	unsigned int	rcv_ready;
	unsigned int    init_done;
} dpm_list_table;

typedef struct _dpm_flag_in_rtm {
	unsigned char	dpm_mode;
	unsigned char	dpm_wakeup;
	unsigned char	dpm_sleepd_stop;

	int 	dpm_rtc_timeout_flag;
	int 	dpm_rtc_timeout_tid;
	int 	dpm_keepalive_time_msec;	/* DPM Keepalive periodic time */
	int		dpm_supp_state;				/* Supplicant connection state */
	long	dpm_timezone;				/* timezone */

	unsigned long long systime_offset;	/* msec */
	unsigned long long rtc_oldtime;		/* msec */
	unsigned char	dpm_sntp_use;				/* for sntp use */
	unsigned long	dpm_sntp_period;			/* for sntp period */
	unsigned long	dpm_sntp_timeout;			/* for sntp timeout */

	int		dpm_mgr_sleep_delay;

	unsigned char 	dpm_dbg_level;
	unsigned char 	dpm_test_app;
	
#if defined	( __CONSOLE_CONTROL__ )
	unsigned char   console_ctrl_rtm;
	char 			reserve_1[1];
#else
	char			reserve_1[2];
#endif	/* __CONSOLE_CONTROL__	*/

} dpm_flag_in_rtm_t;					// Total : 64 Bytes

/* Data structure in Retention memory for DPM operation */
typedef  struct {
	/* for Network Instances */
	char	net_mode;

	/* for da16x Supplicant */
	char	wifi_mode;
	char	country[4];
	char 	reserve_1[2];
} dpm_supp_net_info_t;				// Total 8 Bytes

typedef  struct {
	/* for DHCP Client and Network Instance */
	long			dpm_dhcp_xid;
	unsigned long	dpm_ip_addr;
	unsigned long	dpm_netmask;
	unsigned long	dpm_gateway;
	unsigned long	dpm_dns_addr[2];	// 2 * 4
	unsigned long	dpm_lease;
	unsigned long	dpm_renewal;
	unsigned long	dpm_timeout; // dpm_rebind

	/* for dpm dhcp renew */
	unsigned long	dpm_dhcp_server_ip;
} dpm_supp_ip_info_t;				// Total : 40 Bytes

typedef	 struct {
	int		mode;
	int		disabled;

	int		id;
	int		ssid_len;
	int		scan_ssid;
	int		psk_set;
	int		auth_alg;
	unsigned char	bssid[DPM_ETH_ALEN];	// 6
	unsigned char	reserved[2]; // Padding 2bytes
	unsigned char	ssid[DPM_MAX_SSID_LEN];	// 32
	unsigned char	psk[DPM_PSK_LEN];		// 32

#ifdef DEF_SAVE_DPM_WIFI_MODE
 	int 	wifi_mode;
	int 	dpm_opt;

#ifdef __SUPPORT_IEEE80211W__
	unsigned char	pmf;
	unsigned char	reserved_2[3]; // Padding 3bytes
#else
	unsigned char	reserved_2[4]; // 4bytes
#endif // __SUPPORT_IEEE80211W__
#else
#ifdef __SUPPORT_IEEE80211W__
	unsigned char	pmf;
	unsigned char	reserved_2[11]; // Padding 3bytes + 9bytes
#else
	unsigned char	reserved_2[12]; // 12bytes
#endif // __SUPPORT_IEEE80211W__
#endif
} dpm_supp_conn_info_t;			// Total : 112 Bytes


#define WPA_KCK_MAX_LEN	32
#define WPA_KEK_MAX_LEN	64
#define WPA_TK_MAX_LEN	32
typedef	 struct{
	int		wpa_alg;
	int		key_idx;
	int		set_tx;
	unsigned char	seq[6];
	unsigned char	reserved[2];	// Padding 2bytes
	int				seq_len;
	unsigned char	ptk_kck[WPA_KCK_MAX_LEN];	/* EAPOL-Key Key Confirmation Key (KCK) */
	unsigned char	ptk_kek[WPA_KEK_MAX_LEN];	/* EAPOL-Key Key Encryption Key (KEK) */
	unsigned char	ptk_tk[WPA_TK_MAX_LEN];		/* Temporal Key (TK) */

	unsigned char	ptk_kck_len;
	unsigned char	ptk_kek_len;
	unsigned char	ptk_tk_len;
	unsigned char	reserved_3[1];	// Padding 1bytes
} cipher_ptk_t;					// Total : 156 Bytes

typedef	 struct{
	int		wpa_alg;
	int		key_idx;
	int		set_tx;
	unsigned char	seq[6];
	unsigned char	reserved[2];	// Padding 2bytes
	int				seq_len;
	unsigned char	gtk[32];
	unsigned char	gtk_len;
	unsigned char	reserved_2[3];	// Padding 3bytes
} cipher_gtk_t;					// Total : 60 Bytes

typedef	 struct {
	int		proto;
	int		key_mgmt;
	int		pairwise_cipher;
	int		group_cipher;

	unsigned char	pmk_len;
	unsigned char	reserved;	// Padding 1bytes
	unsigned char	wep_key_len;
	unsigned char	wep_tx_keyidx;
	unsigned char	wep_key[DPM_MAX_WEP_KEY_LEN];	// 16 bytes

	cipher_ptk_t ptk;						// 156 bytes
	cipher_gtk_t gtk;						// 60 bytes
	unsigned char	reserved_2[4]; // Padding 4bytes
} dpm_supp_key_info_t;			// Total : 256 Bytes

typedef void (*timeout_cb)(char *timer_name);
typedef struct {
	timeout_cb 	timeout_callback;
	unsigned int		msec;
	char		task_name[REG_NAME_DPM_MAX_LEN];	// 20 Bytes
	char		timer_name[DPM_TIMER_NAME_MAX_LEN];	//  8 Bytes
} dpm_timer_info_t;					// Total : 36 Bytes

typedef struct {
	dpm_timer_info_t timer_2;
	dpm_timer_info_t timer_3;
	dpm_timer_info_t timer_4;
	dpm_timer_info_t timer_5;
	dpm_timer_info_t timer_6;
	dpm_timer_info_t timer_7;
	dpm_timer_info_t timer_8;
	dpm_timer_info_t timer_9;
	dpm_timer_info_t timer_10;
	dpm_timer_info_t timer_11;
	dpm_timer_info_t timer_12;
	dpm_timer_info_t timer_13;
	dpm_timer_info_t timer_14;
	dpm_timer_info_t timer_15;
} dpm_timer_list_t;					// Total : 504 Bytes

// DA16X_MON_CLIENT - Start
#define NUM_TIM_STATUS 14
#define NUM_ERROR_CODE 10
typedef struct {
	unsigned long	tim_count[NUM_TIM_STATUS];
	unsigned long	error_count[NUM_ERROR_CODE];

	unsigned char	last_abnormal_type;
	unsigned char	last_abnormal_count;
	unsigned char	last_sleep_type; //0: by DPM sleep daemon, 1: by DPM Monitor
#ifdef __DPM_FINAL_ABNORMAL__
	unsigned char	final_abnormal_tid;
#else
	unsigned char	reserved_10;
#endif /* __DPM_FINAL_ABNORMAL__ */

	char	wifi_conn_wait_time;
	char	dhcp_rsp_wait_time;
	char	arp_rsp_wait_time;
	char	unknown_dpm_fail_wait_time;

#ifdef __CHECK_BADCONDITION_WAKEUP__
	unsigned long	firstBadConditionRTC;
	unsigned int	badConditionCount;
	unsigned int	badConditionStep;
#else
	unsigned int	wifi_conn_retry_cnt;
	unsigned int	reserved_21;
	unsigned int	reserved_22;
#endif  /* __CHECK_BADCONDITION_WAKEUP__ */

#if defined(__CHECK_CONTINUOUS_FAULT__) && defined(XIP_CACHE_BOOT)
	unsigned int	fault_PC;
	unsigned char	fault_CNT;
	unsigned char	reserved_25;
	unsigned char	reserved_26;
	unsigned char	reserved_27;
#else
    unsigned int    reserved_23;
    unsigned char   reserved_24;
    unsigned char   reserved_25;
    unsigned char   reserved_26;
    unsigned char   reserved_27;
#endif	// __CHECK_CONTINUOUS_FAULT__
} dpm_monitor_info_t;
// DA16X_MON_CLIENT - End

/*
 * DA16200 SDK V3.X Retention Memory Map (from show_rtm_map())

[NAME]                 [RTM Addr] [RTM Offset] [Struct Name]       [Struct Size]
================================================================================
UMAC START ADDR          0xf80300   160(0x0a0)
--------------------------------------------------------------------------------
APP START ADDR           0xf803a0
--------------------------------------------------------------------------------
RTM_FLAG_BASE            0xf803a0    64(0x040) dpm_flag_in_rtm_t       64(0x040)
RTM_SUPP_NET_INFO_BASE   0xf803e0     8(0x008) dpm_supp_net_info_t      8(0x008)
RTM_SUPP_IP_INFO_BASE    0xf803e8    40(0x028) dpm_supp_ip_info_t      40(0x028)
RTM_SUPP_CONN_INFO_BASE  0xf80410   112(0x070) dpm_supp_conn_info_t   112(0x070)
RTM_SUPP_KEY_INFO_BASE   0xf80480   256(0x100) dpm_supp_key_info_t    256(0x100)
RTM_SPACE_BASE           0xf80580   176(0x0b0) (Free Space)
RTM_ARP_BASE             0xf80630   240(0x0f0)
RTM_MDNS_BASE            0xf80720    68(0x044)
RTM_DNS_BASE             0xf80764   128(0x080)
RTM_OTP_BASE             0xf807e4     8(0x008)
RTM_RTC_TIMER_BASE       0xf807ec   504(0x1f8) dpm_timer_list_t       504(0x1f8)
RTM_DPM_MONITOR_BASE     0xf809e4   124(0x07c) dpm_monitor_info_t     124(0x07c)
--------------------------------------------------------------------------------
RTM_RLIB_BASE            0xf815c0
RTM_TIM_BASE             0xf835c0
--------------------------------------------------------------------------------
RTM_TCP_BASE             0xf88db8 4352(0x1100) // FIXME: Actual size is 1600 bytes in FreeRTOS
RTM_TCP_KA_BASE          0xf89eb8  328(0x0148)
--------------------------------------------------------------------------------
RTM_USER_POOL_BASE       0xf89fc0   64(0x0040)
RTM_USER_DATA_BASE       0xf8a000 8191(0x1fff)
END ADDR                 0xf8c000
================================================================================
*/

/* UMAC ALLOC SZ(UMAC Connection info(0x80) */
#define UMAC_ALLOC_SZ				0xa0			/*   160 bytes */
#define	DPM_UMAC_CONN_INFO_SZ		0x80			/*   128 bytes */

#define FLAG_ALLOC_SZ				0x40			/*    64 bytes */
#define NET_INFO_ALLOC_SZ			0x08			/*     8 bytes */
#define NET_IP_ALLOC_SZ				0x28			/*    40 bytes */
#define CONN_INFO_ALLOC_SZ			0x70			/*   112 bytes */
#define KEY_INFO_ALLOC_SZ			0x100			/*   256 bytes */

#define SPACE_ALLOC_SZ				0xB0			/*   176 bytes FreeRTOS Only */

#define ARP_ALLOC_SZ				0xF0			/*   240 bytes */
#define MDNS_ALLOC_SZ				0x44			/*    68 bytes */
#define DNS_ALLOC_SZ				0x80			/*   128 bytes */
#define OTP_ALLOC_SZ				0x08			/*     8 bytes */
#define RTC_TIMER_ALLOC_SZ			0x1F8			/*   504 bytes */
#define MONITOR_ALLOC_SZ 			0x7C 			/*   124 bytes */

#define TCP_ALLOC_SZ				0x1100			/*  4352 bytes */ // FIXME: Actual size is 1600 bytes in FreeRTOS
#define TCP_KA_INFO_SZ				0x108			/*   264 bytes */
#define	RTM_TCP_TOTAL_SZ			(TCP_ALLOC_SZ + TCP_KA_INFO_SZ)

#define	USER_POOL_ALLOC_SZ			0x40		/*    64 bytes */
#define	USER_DATA_ALLOC_SZ			DA16X_RTM_USER_SIZE	/*  8192 bytes */


#ifdef PRINT_RETMEM_INFO
#define	APP_ALLOC_SZ				(UMAC_ALLOC_SZ +		\
									FLAG_ALLOC_SZ +			\
									NET_INFO_ALLOC_SZ +		\
									NET_IP_ALLOC_SZ +		\
									CONN_INFO_ALLOC_SZ +	\
									KEY_INFO_ALLOC_SZ + 	\
									SPACE_ALLOC_SZ + 		\
									ARP_ALLOC_SZ + 			\
									MDNS_ALLOC_SZ +			\
									DNS_ALLOC_SZ + 			\
									OTP_ALLOC_SZ + 			\
									RTC_TIMER_ALLOC_SZ + 	\
									MONITOR_ALLOC_SZ)
#endif // PRINT_RETMEM_INFO

/*****************************************************************************/
#define	RETMEM_APP_BASE				DA16X_RTM_UMAC_BASE

/*****************************************************************************/
/* Start addr 0x00180300 */
#define	RETMEM_APP_UMAC_OFFSET			(RETMEM_APP_BASE)
#define	RETMEM_APP_SUPP_OFFSET			(RETMEM_APP_BASE + UMAC_ALLOC_SZ)

/* End addr 0x0018153f */
#define	RETMEM_APP_SUPP_UDATA_OFFSET	(DA16X_RTM_USER_BASE)
#define	RETMEM_APP_END_OFFSET			(DA16X_RETMEM_END)

/*****************************************************************************/

#define RETM_UMAC_BAR				(RETMEM_APP_UMAC_OFFSET)
#define RETMEM_APP_SIZE				(DA16X_RTM_UMAC_SIZE + DA16X_RTM_APP_SUPP_SIZE)

/*****************************************************************************/
#define	RTM_DPM_AUTO_CONFIG_BASE	(RETMEM_APP_UMAC_OFFSET  + DPM_UMAC_CONN_INFO_SZ)
#define RTM_FLAG_BASE				RETMEM_APP_SUPP_OFFSET
#define RTM_SUPP_NET_INFO_BASE		(RTM_FLAG_BASE			 + FLAG_ALLOC_SZ)
#define RTM_SUPP_IP_INFO_BASE		(RTM_SUPP_NET_INFO_BASE  + NET_INFO_ALLOC_SZ)
#define RTM_SUPP_CONN_INFO_BASE		(RTM_SUPP_IP_INFO_BASE	 + NET_IP_ALLOC_SZ)
#define RTM_SUPP_KEY_INFO_BASE		(RTM_SUPP_CONN_INFO_BASE + CONN_INFO_ALLOC_SZ)

#define RTM_SPACE_BASE				(RTM_SUPP_KEY_INFO_BASE + KEY_INFO_ALLOC_SZ)

#define RTM_ARP_BASE				((RTM_SUPP_KEY_INFO_BASE + KEY_INFO_ALLOC_SZ) + SPACE_ALLOC_SZ)
#define RTM_MDNS_BASE				(RTM_ARP_BASE		+ ARP_ALLOC_SZ)
#define RTM_DNS_BASE				(RTM_MDNS_BASE		+ MDNS_ALLOC_SZ)
#define RTM_OTP_BASE				(RTM_DNS_BASE		+ DNS_ALLOC_SZ)
#define	RTM_RTC_TIMER_BASE			(RTM_OTP_BASE		+ OTP_ALLOC_SZ)
#define	RTM_DPM_MONITOR_BASE		(RTM_RTC_TIMER_BASE	+ RTC_TIMER_ALLOC_SZ)

//DA16X_RTM_VAR_BASE

#define	RTM_RLIB_BASE				DA16X_RETM_RALIB_OFFSET
#define	RTM_TIM_BASE				DA16X_RETM_PTIM_OFFSET

/* RETMEM_USER_BASE */
#define RTM_USER_DATA_BASE			RETMEM_APP_SUPP_UDATA_OFFSET

#define	RTM_USER_POOL_BASE			(RTM_USER_DATA_BASE	- USER_POOL_ALLOC_SZ)
#define RTM_TCP_BASE				(RTM_USER_POOL_BASE	- RTM_TCP_TOTAL_SZ)
#define RTM_TCP_KA_BASE				(RTM_TCP_BASE		+ TCP_ALLOC_SZ)

/* TCP ACK Reserved Area in TIM */
#ifdef __SUPPORT_DPM_TIM_TCP_ACK__
  #define	RTM_TIM_TCP_ACK_SZ			0x40	/* 64 bytes */
#else
  #define	RTM_TIM_TCP_ACK_SZ			0x00
#endif	// __SUPPORT_DPM_TIM_TCP_ACK__

#define	RTM_TIM_TCP_ACK_BASE		(RTM_TCP_BASE - RTM_TIM_TCP_ACK_SZ)	

//****************************************************************************
#define TCP_SESS_INFO				(RTM_TCP_BASE)

//****************************************************************************

#define RTM_FLAG_PTR			((dpm_flag_in_rtm_t *)	 RTM_FLAG_BASE)
#define RTM_SUPP_NET_INFO_PTR 		((dpm_supp_net_info_t *) RTM_SUPP_NET_INFO_BASE)
#define RTM_SUPP_IP_INFO_PTR 		((dpm_supp_ip_info_t *)  RTM_SUPP_IP_INFO_BASE)

#define RTM_SUPP_CONN_INFO_PTR 		((dpm_supp_conn_info_t *)RTM_SUPP_CONN_INFO_BASE)
#define RTM_SUPP_KEY_INFO_PTR 		((dpm_supp_key_info_t *) RTM_SUPP_KEY_INFO_BASE)

#define RTM_ARP_PTR			((unsigned char *)RTM_ARP_BASE)
#define RTM_MDNS_PTR			((unsigned char *)RTM_MDNS_BASE)
#define RTM_DNS_PTR			((unsigned char *)RTM_DNS_BASE)
#define RTM_OTP_PTR			((unsigned char *)RTM_OTP_BASE)

#define RTM_RTC_TIMER_PTR		((dpm_timer_list_t *) RTM_RTC_TIMER_BASE)
#define RTM_DPM_MONITOR_PTR		((dpm_monitor_info_t *)  RTM_DPM_MONITOR_BASE)

#define	RTM_USER_POOL_PTR		((unsigned char *)RTM_USER_POOL_BASE)
#define	RTM_USER_DATA_PTR		((unsigned char *)RTM_USER_DATA_BASE)

//****************************************************************************

////////////////////////////////////////////////////////////////////////////


#endif	/* __DA16X_DPM_REGS_H__ */

/* EOF */
