/**
 ****************************************************************************************
 *
 * @file common_config.h
 *
 * @brief Provide System Configuration API
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

#ifndef	__COMMON_CONFIG_H__
#define	__COMMON_CONFIG_H__

#include "sdk_type.h"

#define	MAX_COUNTRY_CNT		178

/// Default Netmask (Invalid)
#define	DEF_INVALID_NETMASK	"255.255.255.255"
/// Default Netmask (Valid)
#define	DEF_VALID_NETMASK	"255.255.0.0"
/// Default Gateway IP Address (Invalid)
#define	DEF_INVALID_GATEWAY	"0.0.0.0"
/// Default Gateway IP Address (Valid)
#define	DEF_VALID_GATEWAY	"10.0.0.1"

/// Common Config - No Error
#define CC_SUCCESS					0
/// Common Config - Too long string value input
#define CC_FAILURE_STRING_LENGTH	1
/// Common Config - No value input
#define CC_FAILURE_NO_VALUE			2
/// Common Config - Range out
#define CC_FAILURE_RANGE_OUT		3
/// Common Config - Not Supported input
#define CC_FAILURE_NOT_SUPPORTED	4
/// Common Config - Invalid input
#define	CC_FAILURE_INVALID			5
/// Common Config - Memory Allocation Failure
#define	CC_FAILURE_NO_ALLOCATION	6
/// Common Config - Not Ready (Wi-Fi connection or Network setting)
#define CC_FAILURE_NOT_READY		7
/// Common Config - Unknown Reason
#define CC_FAILURE_UNKNOWN			9

/// DA16200 System Configuration [Name(string)]
typedef enum {
	/// DA16200 MAC Address in NVRAM (set only)
	DA16X_CONF_STR_MAC_NVRAM,
	/// DA16200 MAC Spoofing (set only)
	DA16X_CONF_STR_MAC_SPOOFING,
	/// DA16200 MAC in OTP (set only)
	DA16X_CONF_STR_MAC_OTP,
	/// DA16200 MAC of wlan0 (get only)
	DA16X_CONF_STR_MAC_0,
	/// DA16200 MAC of wlan1 (get only)
	DA16X_CONF_STR_MAC_1,
	/// DA16200 MAC of Passive scan
	DA16X_CONF_STR_MAC_PS,

	/// Wi-Fi Scan List
	DA16X_CONF_STR_SCAN,
	/// Wi-Fi wlan0 SSID
	DA16X_CONF_STR_SSID_0,
	/// Wi-Fi wlan1 SSID
	DA16X_CONF_STR_SSID_1,
	/// Wi-Fi wlan0 WEP Key value #0
	DA16X_CONF_STR_WEP_KEY0,
	/// Wi-Fi wlan0 WEP Key value #1
	DA16X_CONF_STR_WEP_KEY1,
	/// Wi-Fi wlan0 WEP Key value #2
	DA16X_CONF_STR_WEP_KEY2,
	/// Wi-Fi wlan0 WEP Key value #3
	DA16X_CONF_STR_WEP_KEY3,
	/// Wi-Fi wlan0 WPA-PSK key
	DA16X_CONF_STR_PSK_0,
	/// Wi-Fi wlan1 WPA-PSK key
	DA16X_CONF_STR_PSK_1,
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
	/// Wi-Fi wlan0 SAE password
	DA16X_CONF_STR_SAE_PASS_0,
	/// Wi-Fi wlan1 SAE password
	DA16X_CONF_STR_SAE_PASS_1,
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
	/// Wi-Fi wlan0 WPA-Enterprise ID
	DA16X_CONF_STR_EAP_IDENTITY,
	/// Wi-Fi wlan0 WPA-Enterprise Password
	DA16X_CONF_STR_EAP_PASSWORD,
	/// Country Code
	DA16X_CONF_STR_COUNTRY,
	/// Wi-Fi Device Name
	DA16X_CONF_STR_DEVICE_NAME,

	/// IP Address of wlan0
	DA16X_CONF_STR_IP_0,
	/// Netmask of wlan0
	DA16X_CONF_STR_NETMASK_0,
	/// Gateway Address of wlan0
	DA16X_CONF_STR_GATEWAY_0,
	/// IP Address of wlan1
	DA16X_CONF_STR_IP_1,
	/// Netmask of wlan1
	DA16X_CONF_STR_NETMASK_1,
	/// Gateway Address of wlan1
	DA16X_CONF_STR_GATEWAY_1,
	/// DNS Server Address of wlan0
	DA16X_CONF_STR_DNS_0,
	/// DNS Server Address of wlan1
	DA16X_CONF_STR_DNS_1,
	/// DNS 2nd Server Address of wlan0
	DA16X_CONF_STR_DNS_2ND,

	/// Alloating Start IP Address of DHCP Server
	DA16X_CONF_STR_DHCP_START_IP,
	/// Alloating End IP Address of DHCP Server
	DA16X_CONF_STR_DHCP_END_IP,
	/// Alloating DNS Server Address of DHCP Server
	DA16X_CONF_STR_DHCP_DNS,

#if defined ( __USER_DHCP_HOSTNAME__ )
	/// DHCP Client HostName
	DA16X_CONF_STR_DHCP_HOSTNAME,
#endif	// __USER_DHCP_HOSTNAME__

	/// SNTP Server Address
	DA16X_CONF_STR_SNTP_SERVER_IP,
	DA16X_CONF_STR_SNTP_SERVER_1_IP,
	DA16X_CONF_STR_SNTP_SERVER_2_IP,

	/// Debug Tx Power
	DA16X_CONF_STR_DBG_TXPWR,

	DA16X_CONF_STR_MAX,
} DA16X_CONF_STR;


typedef enum {
	/// Wi-Fi Mode (0: STA / 1: Soft-AP)
	DA16X_CONF_INT_MODE,
	/// Wi-Fi wlan0 Authentication Mode (cc_val_auth)
	DA16X_CONF_INT_AUTH_MODE_0,
	/// Wi-Fi wlan1 Authentication Mode (cc_val_auth)
	DA16X_CONF_INT_AUTH_MODE_1,
	/// Wi-Fi wlan0 WEP Key Index (0~3)
	DA16X_CONF_INT_WEP_KEY_INDEX,
	/// Wi-Fi wlan0 Encryption Mode (cc_val_enc)
	DA16X_CONF_INT_ENCRYPTION_0,
	/// Wi-Fi wlan1 Encryption Mode (cc_val_enc)
	DA16X_CONF_INT_ENCRYPTION_1,
	/// Wi-Fi wlan0 Wi-Fi Mode (cc_val_wfmode)
	DA16X_CONF_INT_WIFI_MODE_0,
	/// Wi-Fi wlan1 Wi-Fi Mode (cc_val_wfmode)
	DA16X_CONF_INT_WIFI_MODE_1,
	/// Wi-Fi wlan1 Channel
	DA16X_CONF_INT_CHANNEL,
	/// Wi-Fi wlan1 Frequency (MHz)
	DA16X_CONF_INT_FREQUENCY,
	/// Wi-Fi wlan0 Roaming Use
	DA16X_CONF_INT_ROAM,
	/// Wi-Fi wlan0 Roaming Threshold (dBm)
	DA16X_CONF_INT_ROAM_THRESHOLD,
	/// Wi-Fi wlan0 Connect only to the designated channel
	DA16X_CONF_INT_FIXED_ASSOC_CH,	
	/// Wi-Fi wlan0 Fast connection sleep1,2 mode (0: Enable / 1: Disabled)
	DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2,
	/// Wi-Fi wlan0 Profile Disabled (0: Enable / 1: Disabled)
	DA16X_CONF_INT_STA_PROF_DISABLED,
	/// Wi-Fi wlan0 EAP Phase #1 (cc_val_eap1)
	DA16X_CONF_INT_EAP_PHASE1,
	/// Wi-Fi wlan0 EAP Phase #1 (cc_val_eap2)
	DA16X_CONF_INT_EAP_PHASE2,

	/// Wi-Fi wlan1 Beacon Interval (ms)
	DA16X_CONF_INT_BEACON_INTERVAL,
	/// Wi-Fi wlan1 Inactivity Check with STA (seconds)
	DA16X_CONF_INT_INACTIVITY,
	/// Wi-Fi wlan1 RTS Threshold
	DA16X_CONF_INT_RTS_THRESHOLD,
	/// Wi-Fi wlan1 WMM use
	DA16X_CONF_INT_WMM,
	/// Wi-Fi wlan1 WMM-PS use
	DA16X_CONF_INT_WMM_PS,
	/// Wi-Fi wlan1 P2P Operation Channel
	DA16X_CONF_INT_P2P_OPERATION_CHANNEL,
	/// Wi-Fi wlan1 P2P Listen Channel
	DA16X_CONF_INT_P2P_LISTEN_CHANNEL,
	/// Wi-Fi wlan1 GO Intent (0~15)
	DA16X_CONF_INT_P2P_GO_INTENT,
	/// Wi-Fi wlan1 Find Timeout (seconds)
	DA16X_CONF_INT_P2P_FIND_TIMEOUT,

#ifdef __SUPPORT_NAT__
	DA16X_CONF_INT_NAT,
#endif /* __SUPPORT_NAT__ */

	/// DHCP Client use
	DA16X_CONF_INT_DHCP_CLIENT,
	DA16X_CONF_INT_DHCP_CLIENT_CONFIG_ONLY,
	/// DHCP Clinet and Temporary static IP
	DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP,
	
	/// DHCP Server use
	DA16X_CONF_INT_DHCP_SERVER,
	/// DHCP Server Lease Time
	DA16X_CONF_INT_DHCP_LEASE_TIME,

	/// SNTP Client use
	DA16X_CONF_INT_SNTP_CLIENT,
	/// SNTP Sync. Period
	DA16X_CONF_INT_SNTP_SYNC_PERIOD,

	/// TLS Version (0: TLSv1.0 / 1: TLSv1.1 / 2: TLSv1.2)
	DA16X_CONF_INT_TLS_VER,
	/// TLS RootCA Check use
	DA16X_CONF_INT_TLS_ROOTCA_CHK,

	/// GMT (seconds; -43200 ~ 43200)
	DA16X_CONF_INT_GMT,

	/// DPM use
	DA16X_CONF_INT_DPM,
	DA16X_CONF_INT_DPM_ABN,

#if defined ( __SUPPORT_WPA3_PERSONAL_CORE__ ) || defined ( __SUPPORT_WPA3_ENTERPRISE_CORE__ )
	/// PMF
	DA16X_CONF_INT_PMF_80211W_0,
	DA16X_CONF_INT_PMF_80211W_1,
#endif // __SUPPORT_WPA3_PERSONAL_CORE__ || __SUPPORT_WPA3_ENTERPRISE_CORE__
	DA16X_CONF_INT_HIDDEN_0,
	DA16X_CONF_INT_MAX,
} DA16X_CONF_INT;


/// Common Config - Enable/Disable values
typedef enum {
	/// Not used
	CC_VAL_DISABLE,
	/// Used
	CC_VAL_ENABLE,
} cc_val_bool;

#if defined __SUPPORT_WPA3_PERSONAL_CORE__ || defined __SUPPORT_WPA3_ENTERPRISE_CORE__
/// Common Config - Mandatory/Opional values
typedef enum {
	/// Mandatory
	CC_VAL_MAND,
	/// Opional
	CC_VAL_OPT,
} cc_val_mand;
#endif // __SUPPORT_WPA3_PERSONAL_CORE__ || __SUPPORT_WPA3_ENTERPRISE_CORE__

/// Common Config - Wi-Fi Authentication Mode values
typedef enum {
	/// Wi-Fi Open Security
	CC_VAL_AUTH_OPEN,
	/// Wi-Fi Security: WEP
	CC_VAL_AUTH_WEP,
	/// Wi-Fi Security: WPA
	CC_VAL_AUTH_WPA,
	/// Wi-Fi Security: WPA2 (RSN)
	CC_VAL_AUTH_WPA2,
	/// Wi-Fi Security: WPA & WPA2
	CC_VAL_AUTH_WPA_AUTO,
#if defined (__SUPPORT_WPA3_PERSONAL_CORE__)
	/// Wi-Fi Security: WPA3 OWE
	CC_VAL_AUTH_OWE,
	/// Wi-Fi Security: WPA3 SAE
	CC_VAL_AUTH_SAE,
	/// Wi-Fi Security: WPA2 (RSN) & WPA3 SAE
	CC_VAL_AUTH_RSN_SAE,
#endif //  __SUPPORT_WPA3_PERSONAL_CORE__
	/// Wi-Fi Security: WPA-Enterprise
	CC_VAL_AUTH_WPA_EAP       = 8,
	/// Wi-Fi Security: WPA2-Enterprise
	CC_VAL_AUTH_WPA2_EAP      = 9,
	/// Wi-Fi Security: WPA & WPA2-Enterprise
	CC_VAL_AUTH_WPA_AUTO_EAP  = 10,
} cc_val_auth;

/// Common Config - Wi-Fi WPA Encryption values
typedef enum {
	/// WPA Encryption: TKIP
	CC_VAL_ENC_TKIP,
	/// WPA Encryption: CCMP
	CC_VAL_ENC_CCMP,
	/// WPA Encryption: TKIP + CCMP
	CC_VAL_ENC_AUTO,
} cc_val_enc;

/// Common Config - Wi-Fi Mode values
typedef enum {
	/// Wi-Fi IEEE802.11 Mode: 11b/g/n
	CC_VAL_WFMODE_BGN,
	/// Wi-Fi IEEE802.11 Mode: 11g/n
	CC_VAL_WFMODE_GN,
	/// Wi-Fi IEEE802.11 Mode: 11b/g
	CC_VAL_WFMODE_BG,
	/// Wi-Fi IEEE802.11 Mode: 11n only
	CC_VAL_WFMODE_N,
	/// Wi-Fi IEEE802.11 Mode: 11g only
	CC_VAL_WFMODE_G,
	/// Wi-Fi IEEE802.11 Mode: 11b only
	CC_VAL_WFMODE_B,
} cc_val_wfmode;

/// Common Config - Wi-Fi Enterprise EAP Phase#1 values
typedef enum {
	/// WPA-Enterprise Phase1: Default ( PEAP / TTLS / FAST )
	CC_VAL_EAP_DEFAULT,

	/// WPA-Enterprise Phase1: PEAPv0
	CC_VAL_EAP_PEAP0,

	/// WPA-Enterprise Phase1: PEAPv1
	CC_VAL_EAP_PEAP1,

	/// WPA-Enterprise Phase1: EAP-FAST
	CC_VAL_EAP_FAST,

	/// WPA-Enterprise Phase1: EAP-TTLS
	CC_VAL_EAP_TTLS,

	/// WPA-Enterprise Phase1: EAP-TLS
	CC_VAL_EAP_TLS,
} cc_val_eap1;

/// Common Config - Wi-Fi Enterprise EAP Phase#2 values
typedef enum {
	/// WPA-Enterprise Phase2: MSCHAPV2 GTC
    CC_VAL_EAP_PHASE2_MIX,

	/// WPA-Enterprise Phase2: EAP-MSCHAPv2
	CC_VAL_EAP_MSCHAPV2,

	/// WPA-Enterprise Phase2: EAP-GTC
	CC_VAL_EAP_GTC,
} cc_val_eap2;

typedef struct _country_ch_power_level {
	char	*country;
	unsigned char   ch[14];
} country_ch_power_level_t;

/**
 ****************************************************************************************
 * @brief Set string value with NVRAM save. (There is some delay to save to NVRAM.)
 * @param[in] name parameter name (DA16X_CONF_STR)
 * @param[in] value string value input
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_set_config_str(int name, char *value);

/**
 ****************************************************************************************
 * @brief Set integer value with NVRAM save. (There is some delay to save to NVRAM.)
 * @param[in] name parameter name (DA16X_CONF_STR)
 * @param[in] value integer value input
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_set_config_int(int name, int value);

/**
 ****************************************************************************************
 * @brief Read string value.
 * @param[in] name parameter name (DA16X_CONF_STR)
 * @param[out] value string value output
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_get_config_str(int name, char *value);

/**
 ****************************************************************************************
 * @brief Read integer value.
 * @param[in] name Parameter name (DA16X_CONF_STR)
 * @param[out] value integer value output
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_get_config_int(int name, int *value);

/**
 ****************************************************************************************
 * @brief Set string value without NVRAM save. If you want to save to NVRAM, use the API 
 * da16x_nvcache2flash after all configuration.
 * @param[in] name parameter name (DA16X_CONF_STR)
 * @param[in] value integer value input
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_set_nvcache_str(int name, char *value);

/**
 ****************************************************************************************
 * @brief Set integer value without NVRAM save. If you want to save to NVRAM, use the API 
 * da16x_nvcache2flash after all configuration.
 * @param[in] name parameter name (DA16X_CONF_STR)
 * @param[in] value integer value input
 * @return CC_SUCCESS if succeed, error code else.
 ****************************************************************************************
 */
int da16x_set_nvcache_int(int name, int value);

/**
 ****************************************************************************************
 * @brief Save all configuration to NVRAM set by da16x_set_nvcache_str or
 * da16x_set_nvcache_int
 * @return
 ****************************************************************************************
 */
void da16x_nvcache2flash(void);

/**
 ****************************************************************************************
 * @brief Execute the DA16200 Soft-AP with default configuration \n
 * (SSID:DA16200_[The last 3 octets of WLAN1_MAC_ADDR], Auth: WPA2-CCMP or 
 * WPA2-PSK+WPA3-SAE Key:12345678, IP:10.0.0.1, Enable DHCP Server)
 * @param[in] reboot_flag to reboot immediately
 * @return CC_SUCCESS always
 ****************************************************************************************
 */
int da16x_set_default_ap(int reboot_flag);

/**
 ****************************************************************************************
 * @brief Execute the DA16200 STA with blank configuration \n
 * @return CC_SUCCESS always
 ****************************************************************************************
 */
int da16x_set_default_sta(void);

int da16x_set_default_ap_config(void);



extern unsigned int	writeMACaddress(char* macaddr, int dst);
extern void	set_boot_mode(int mode);
extern int	set_dns_addr_2nd(int iface_flag, char *ip_addr);
extern char config_dpm_abnormal_timer(char flag);

#endif	/* __COMMON_CONFIG_H__ */

/* EOF */
