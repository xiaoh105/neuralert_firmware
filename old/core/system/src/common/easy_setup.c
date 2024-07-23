/**
 ****************************************************************************************
 *
 * @file esay_setup.c
 *
 * @brief Network system application
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
#include "project_config.h"

#include "sdk_type.h"

#ifdef	XIP_CACHE_BOOT

#include "da16x_types.h"
#include "sys_feature.h"
#include "command.h"
#include "command_net.h"
#include "common_def.h"
#include "common_utils.h"

#include "iface_defs.h"
#include "nvedit.h"
#include "environ.h"

#include "da16x_time.h"
#include "da16x_image.h"

#include "supp_def.h"
#include "common_config.h"
#include "wifi_common_features.h"
#include "da16x_network_common.h"

#include "lwipopts.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "dhcpserver.h"
#include "dhcpserver_options.h"
#include "lwip/etharp.h"
#include "da16x_arp.h"
#include "da16x_cert.h"

#include "mqtt_client.h"

#if defined __SUPPORT_WPA3_PERSONAL__
#if !defined __SUPPORT_WPA3_PERSONAL_CORE__
	#error "This SDK does not support WPA3-Personal."
#endif // __SUPPORT_WPA3_PERSONAL_CORE__
#endif // __SUPPORT_WPA3_PERSONAL__

#if defined __SUPPORT_WPA_ENTERPRISE__
#if !defined __SUPPORT_WPA_ENTERPRISE_CORE__
	#error "This SDK does not support WPA-Enterprise."
#else
	#if !defined __SUPPORT_WPA3_ENTERPRISE_CORE__
		#error "This SDK does not support WPA3-Enterprise."
	#else
		#if defined __SUPPORT_WPA3_ENTERPRISE_192B__
			#if !defined __SUPPORT_WPA3_ENTERPRISE_192B_CORE__
				#error "This SDK does not support WPA3-Enterprise 192Bits."
			#endif // __SUPPORT_WPA3_ENTERPRISE_192B_CORE__
		#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
	#endif // __SUPPORT_WPA3_ENTERPRISE_CORE__
#endif /* __SUPPORT_WPA_ENTERPRISE_CORE__ */
#endif // __SUPPORT_WPA_ENTERPRISE__


enum
{
	CA_CERT1 = 0,
	CLIENT_CERT1,
	CLIENT_KEY1,
	DH_PARAM1,
	CA_CERT2,
	CLIENT_CERT2,
	CLIENT_KEY2,
	DH_PARAM2,
	CA_CERT3,
	CLIENT_CERT3,	// For Enterprise(802.1x)
	CLIENT_KEY3,
	DH_PARAM3,
	CA_CERT4,		// For Reserved
	CLIENT_CERT4,
	CLIENT_KEY4,
	DH_PARAM4,
	TLS_CERT_01 = 100,
	TLS_CERT_02,
	TLS_CERT_03,
	TLS_CERT_04,
	TLS_CERT_05,	
	TLS_CERT_06,
	TLS_CERT_07,
	TLS_CERT_08,
	TLS_CERT_09,
	TLS_CERT_10,
	CERT_ALL,
	CERT_END,
};

enum
{
	ACT_NONE = 0,
	ACT_WRITE,
	ACT_READ,
	ACT_DELETE,
	ACT_STATUS
};


//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------
int		getStr(char *get_data, int get_len);

//-----------------------------------------------------------------------
// External Functions
//-----------------------------------------------------------------------
extern long	iptolong(char *ip);

extern int	wlaninit_cmd_parser (char *cmdline, char *delimit);
extern int	da16x_cli_reply(char *cmdline, char *delimit, char *cli_reply);
extern int	country_channel_range (char *country, int *startCH, int *endCH);

extern void	set_dpm_keepalive_config(int duration);
extern int	chk_channel_by_country(char *country, int set_channel, int mode,
								   int *rec_channel);

extern UINT	cc_power_table_size(void);
extern char *country_code(int index);
extern void	printf_encode(char *txt, size_t maxlen, const u8 *data, size_t len);

extern void	disable_all_dpm_timer(void);
extern void	do_set_all_timer_kill(void);
extern void	terminate_sys_apps(void);

extern int	get_run_mode(void);
extern void	set_run_mode(int mode);

extern int	_sys_clock_read(unsigned int *clock, int len);
extern void do_write_clock_change(unsigned int data32, int disp_flag);

extern int backup_NVRAM(void);

extern void hold_dpm_sleepd(void);
extern void set_dpm_tim_wakeup_dur(unsigned int dtim_period, int saveflag);

extern void sntp_stop(void);
extern int i3ed11_freq_to_ch(int freq);
extern int da16x_set_nvcache_int(int name, int value);

extern UCHAR	dpm_wu_tm_msec_flag;
extern UCHAR	sntp_support_flag;
extern unsigned char	ble_combo_ref_flag;
extern unsigned char	fast_conn_sleep_12_flag;

extern void delete_user_wifi_disconn(void);

#ifdef __SUPPORT_WPA_ENTERPRISE__
extern void * os_zalloc(size_t size);

static inline void * os_calloc(size_t nmemb, size_t size)
{
	if (size && nmemb > (~(size_t) 0) / size)
		return NULL;
	return os_zalloc(nmemb * size);
}

extern int cert_rwds(char action, char dest);
#endif	//__SUPPORT_WPA_ENTERPRISE__

//-----------------------------------------------------------------------
// External Variables
//-----------------------------------------------------------------------

const char *wifi_mode_str[] = {"11b/g/n", "11g/n", "11b/g", "11n only", "11g only", "11b only"};


#define RET_QUIT	-99
#define RET_DEFAULT	-88
#define RET_NODIGIT	-1
#define RET_MANUAL	-2

int getStr(char *get_data, int get_len)
{
	int i = 0;
	char ch = 0;

#if defined ( __SUPPORT_CONSOLE_PWD__ )
	extern UINT	password_svc;
	extern int	checkPassword();
#endif // __SUPPORT_CONSOLE_PWD__

	do {
		ch = GETC();

#if defined ( __SUPPORT_CONSOLE_PWD__ )
		if (password_svc) {
			if (checkPassword()) {
				continue;
			}
		}
#endif // __SUPPORT_CONSOLE_PWD__

		if (ch == 0x03) {	/* CTRL + C */
			/* Move the cursor left & Erase from the cursor to the end of the line */
			PRINTF("\33[D\33[0K\n");

			return RET_QUIT;
		}

		if ((ch & 0xFF) < ' ' && ch != '\b' && ch != '\n' && ch != '\r') {
			continue;
		}

		if (ch == '\b') {	/* backspace */
			if (i > 0) {
				get_data[--i] = '\0';
				PRINTF("\33[0K");
			} else {
				PRINTF("\33[C"); /* Move the cursor right */
			}

			continue;
		}

		get_data[i++] = (char)(ch & 0xFF);

	} while (i <= get_len && get_data[i - 1] != '\n' && get_data[i - 1] != '\r');


	if (get_len <= i) {
		PRINTF("\n");
	}

	if (get_data[i - 1] != '\n' || get_data[i - 1] != '\r') {
		get_data[i - 1] = '\0';
		return i - 1;
	} else {
		get_data[i] = '\0';
		return i;
	}
}

#define RUN_DELAY 100

/*
	wpas_mode WPAS_MODE_INFRA = 0,
	WPAS_MODE_IBSS = 1,
	WPAS_MODE_AP = 2,
	WPAS_MODE_P2P_GO = 3,
	WPAS_MODE_P2P_GROUP_FORMATION = 4,
*/
int getNum(void)
{
	char get_data[7];
	int i = 0;
	int get_len = 0;

	memset(get_data, 0, 7);

	get_len = getStr(get_data, 6);

	if (get_len > 0) {
		if ((get_data[0] == 'Q' || get_data[0] == 'q') && strlen(get_data) == 1) {
			return RET_QUIT; /* quit */
		} else if ((get_data[0] == 'M' || get_data[0] == 'm') && strlen(get_data) == 1) {
			return RET_MANUAL;
		}

		while (get_data[i] != 0) {
			if (isdigit((int)(get_data[i])) == 0) {
				return RET_NODIGIT;
			}

			i++;
		}

		return atoi(get_data);
	} else if (get_len == RET_QUIT) {
		return RET_QUIT;
	}

	return RET_DEFAULT;
}

#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
int  check_sae_groupid(int id)
{
	switch (id)   /* Check Support Group ID */
	{
		case 19:
		case 20:
#ifdef NOT_SUPPORT
		case 21:
		case 25:
		case 26:
#endif /* NOT_SUPPORT */
			return 1; /* support */

		default:
			return 0;
	}
}
#endif /* __SUPPORT_WPA3_SAE__ || defined __SUPPORT_MESH__ */


#define SWAP(a, b) { long t = *(long *)(a); *(long *)(a) = *(long *)(b); *(long *)(b) = t; }

#define E_SSID_LEN 128 /* 33 ==> 128 */
#define E_CMDLINE_LEN 232 /* 144 ==> 160 */

/* Easy Setup */

#define CMD_DELIMIT 			"\t"

enum {
	E_WLAN_SETUP = 200,
	E_INPUT_SSID,
#ifdef __SUPPORT_WPA_ENTERPRISE__
	E_INPUT_EAP_METHOD,
#endif // __SUPPORT_WPA_ENTERPRISE__
	E_INPUT_PSK_KEY,
	E_INPUT_WEP,
	E_AVD_CFG,
	E_SETUP_WIFI_END,
	E_NETWORK_SETUP,
	E_SETUP_APPLY_START,
	E_INPUT_IPADDRESS,
	E_SETUP_APPLY_SYSMODE,
	E_INPUT_SKIP_ALL_1,
	E_INPUT_SKIP_ALL_2,
	E_CONTINUE,
	E_ERROR = 254,
	E_QUIT = 255
};

static void setup_display_title(unsigned char sysmode, unsigned char cur_iface)
{
	/* Print Title */
	if (cur_iface == WLAN0_IFACE && (sysmode == E_SYSMODE_STA || sysmode == E_SYSMODE_STA_AP)) {
		PRINTF("\n" ANSI_COLOR_BLACK ANSI_BCOLOR_YELLOW
			   "[ STATION CONFIGURATION ]"
			   ANSI_BCOLOR_DEFULT ANSI_COLOR_DEFULT "\n");
	} else if (sysmode == E_SYSMODE_AP || (cur_iface == WLAN1_IFACE && sysmode == E_SYSMODE_STA_AP)) {
		PRINTF("\n" ANSI_COLOR_BLACK ANSI_BCOLOR_YELLOW
			   "[ SOFT-AP CONFIGURATION ]"
			   ANSI_BCOLOR_DEFULT ANSI_COLOR_DEFULT "\n");
	}
#if defined ( __SUPPORT_P2P__ )
	else if (sysmode == E_SYSMODE_P2P || (cur_iface == WLAN1_IFACE && sysmode == E_SYSMODE_STA_P2P)) {
		PRINTF("\n" ANSI_COLOR_BLACK ANSI_BCOLOR_YELLOW
			   "[ WiFi Direct CONFIGURATION ]"
			   ANSI_BCOLOR_DEFULT ANSI_COLOR_DEFULT "\n");
	}
#endif	// __SUPPORT_P2P__
}


#define DEFAULT_BSS_MAX_COUNT	40
#define	CLI_SCAN_RSP_BUF_SIZE	3584

static unsigned char setup_display_scan_list(
										char *p_ssid,
										unsigned char *p_connect_hidden_ssid,
										unsigned char *p_auth_mode,
										unsigned char *p_encryp_mode,
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
										char *p_sae_groups,
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__
										unsigned char *p_psk_key_type,
										char *p_psk_key,
#if defined __SUPPORT_IEEE80211W__										
										unsigned char *p_pmf,
#endif // __SUPPORT_IEEE80211W__
										unsigned char *p_wifi_mode,

										int* select_ssid, 
										int* channel_selected)
{
	DA16X_UNUSED_ARG(p_psk_key_type);
	DA16X_UNUSED_ARG(p_psk_key);
	DA16X_UNUSED_ARG(p_wifi_mode);
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
	DA16X_UNUSED_ARG(p_sae_groups);
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__

	char	*params[DEFAULT_BSS_MAX_COUNT][5]; // BSSID, SIGNAL
	int		param_cnt = 0;
	int		sel_ssid = 0;
	int		getStr_len = 0;
	char	*scan_result;

#if defined ( __SUPPORT_P2P__ )
	if (get_run_mode() == SYSMODE_P2P_ONLY || get_run_mode() == SYSMODE_P2PGO) {
		/* Set STA mode temporary to use SCAN operation */
		set_run_mode(SYSMODE_STA_ONLY);
	}
#endif	// __SUPPORT_P2P__

	scan_result= pvPortMalloc(CLI_SCAN_RSP_BUF_SIZE);

	if (scan_result == NULL)
	{
		PRINTF("[%s] Failed to malloc\n", __func__);
		//goto INPUT_SSID;
		return E_INPUT_SSID;
	}

SCAN_SSID:

	memset(scan_result, 0, CLI_SCAN_RSP_BUF_SIZE);
	memset(params, 0, sizeof(params));
	param_cnt = 0;
	da16x_cli_reply("scan", NULL, scan_result);
	
	/*	[START] ======= Separate  param ======= */
	/* parse arguments */
	params[param_cnt++][0] = strtok(scan_result, "\n"); /* Title */
	
	if (params[0][0] == NULL) {
		vPortFree(scan_result);
		PRINTF(ANSI_COLOR_RED "No Scan Resutls" ANSI_COLOR_DEFULT "\n");
	} else {
		print_separate_bar('=', 76, 1);
		PRINTF("[NO] %-45s %9s [CH] [SECURITY]\n", "[SSID]", "[SIGNAL]");
		print_separate_bar('-', 76, 1);

		while ((params[param_cnt][SCAN_BSSID_IDX] = strtok(NULL, "\t")) != NULL) {		/* BSSID */
			if ((params[param_cnt][SCAN_FREQ_IDX] = strtok(NULL, "\t")) != NULL) {		/* Freq. */
				if ((params[param_cnt][SCAN_SIGNAL_IDX] = strtok(NULL, "\t")) != NULL) {	/* Signal*/
					if ((params[param_cnt][SCAN_FLGA_IDX] = strtok(NULL, "\t")) != NULL) {	/* Flag */
						params[param_cnt][SCAN_SSID_IDX] = strtok(NULL, "\n");		/* SSID */
					}
				}
			}

			param_cnt++;

			if (param_cnt > (DEFAULT_BSS_MAX_COUNT - 1)) {
				break;
			}
		}

		/* Scan Result Sort */
		for  (int idx = 1; idx < param_cnt; idx++) {
			char ssid_txt[32 * 4 + 1];
			vTaskDelay(1);

			if (idx % 2 == 1) {
				PRINTF(ANSI_COLOR_BLACK ANSI_BCOLOR_WHITE);
			}

			PRINTF("[%2d]", idx);	/* No */

			/* SSID */
			if (params[idx][SCAN_SSID_IDX][0] == HIDDEN_SSID_DETECTION_CHAR) {	/* Hidden SSID */
				PRINTF(" " ANSI_COLOR_RED ANSI_BOLD "[Hidden] BSSID-%-35s" ANSI_NORMAL "%s%s",
					   params[idx][SCAN_BSSID_IDX],
					   (idx % 2 == 1) ? ANSI_COLOR_BLACK : ANSI_COLOR_DEFULT,
					   (idx % 2 == 1) ? ANSI_BCOLOR_WHITE : "");
			} else {
				printf_encode(ssid_txt, sizeof(ssid_txt),
							  (unsigned char const *)params[idx][SCAN_SSID_IDX],
							  strlen((char *)params[idx][SCAN_SSID_IDX]));

				if (strlen(ssid_txt) <= 50) {
					PRINTF(" %-50s", ssid_txt);
				} else {
					char tmp_ssid[50];
					memset(tmp_ssid, 0, 50);
					strncpy(tmp_ssid, ssid_txt, 45);
					strcat(tmp_ssid, " ...");
					PRINTF(" %-50s", tmp_ssid);
				}
			}

			PRINTF(" %3s %3d ", 		/* SIGNAL,	CH */
				   params[idx][SCAN_SIGNAL_IDX],
				   atoi(params[idx][SCAN_FREQ_IDX]) == 2484 \
				   ? 14 : (atoi(params[idx][SCAN_FREQ_IDX]) - 2407) / 5);

			/* Security */
			if (strstr(params[idx][SCAN_FLGA_IDX], "WPA") != 0) {
				if (strstr(params[idx][SCAN_FLGA_IDX], "EAP") != 0) {
					char wpa_ent_str[20];
					unsigned char supp_flg = pdTRUE; // support flag

					//  WPA2-EAP-SUITE-B-192-GCMP-256 : WPA3-Enterprise 192bits
					if (strstr(params[idx][SCAN_FLGA_IDX], "WPA2-EAP-SUITE-B-192-GCMP-256") != 0) { // WPA3-Enterprise 192Bits Mode
						strcpy(wpa_ent_str, "WPA3-ENT192B");
#if !defined __SUPPORT_WPA3_ENTERPRISE_192B__
						supp_flg = pdFALSE;
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
					} else {
						strcpy(wpa_ent_str, "WPA");

						//<WPA2/WPA3-ENT> PMF 1	: WPA2-EAP-CCMP, EAP-SHA256-CCMP
						if (strstr(params[idx][SCAN_FLGA_IDX], "WPA2-EAP-CCMP") != 0
							&& strstr(params[idx][SCAN_FLGA_IDX], "EAP-SHA256-CCMP") != 0) {
							strcat(wpa_ent_str, "2/WPA3");
#if !defined __SUPPORT_WPA3_ENTERPRISE__
							supp_flg = pdFALSE;
#endif // __SUPPORT_WPA3_ENTERPRISE__							
						}
						//<WPA3-ENT> PMF 2	: EAP-SHA256-CCMP : WPA3-Enterprise 128bits Only
						else if (strstr(params[idx][SCAN_FLGA_IDX], "EAP-SHA256-CCMP") != 0
							&& strstr(params[idx][SCAN_FLGA_IDX], "EAP-CCMP") == 0) {
							strcat(wpa_ent_str, "3"); // WPA3-ENT
#if !defined __SUPPORT_WPA3_ENTERPRISE__
							supp_flg = pdFALSE;
#endif // __SUPPORT_WPA3_ENTERPRISE__
						}
						//<WPA2-ENT> PMF 1 or 0: WPA2-EAP-CCMP
						else if (strstr(params[idx][SCAN_FLGA_IDX], "WPA2-EAP-CCMP") != 0
							&& strstr(params[idx][SCAN_FLGA_IDX], "EAP-SHA256") == 0) {
							strcat(wpa_ent_str, "2"); // WPA2-ENT
#if !defined __SUPPORT_WPA_ENTERPRISE__
							supp_flg = pdFALSE;
#endif // __SUPPORT_WPA_ENTERPRISE__
						}
						//<WPA1/2-ENT> PMF 0 	: WPA-EAP-, WPA2-EAP-
						else if (strstr(params[idx][SCAN_FLGA_IDX], "WPA2-EAP") != 0
							&& strstr(params[idx][SCAN_FLGA_IDX], "WPA-EAP") != 0) {
							strcat(wpa_ent_str, "/WPA2");
#if !defined __SUPPORT_WPA_ENTERPRISE__
							supp_flg = pdFALSE;
#endif // __SUPPORT_WPA_ENTERPRISE__
						}						
						//<WPA1-ENT> PMF 0	: WPA-EAP- : WPA1 Only
						else if (strstr(params[idx][SCAN_FLGA_IDX], "WPA-EAP") != 0
							&& strstr(params[idx][SCAN_FLGA_IDX], "WPA2-EAP") == 0) {
#if !defined __SUPPORT_WPA_ENTERPRISE__
							supp_flg = pdFALSE;
#endif // __SUPPORT_WPA_ENTERPRISE__
						}
						else
						{
							strcpy(wpa_ent_str, "UNKNOW");
							supp_flg = pdFALSE;
						}
						strcat(wpa_ent_str, "-ENT");
					}

					if (supp_flg == pdFALSE) {
						PRINTF(ANSI_COLOR_LIGHT_RED);
					}

					PRINTF("%12s" ANSI_NORMAL "%s" ANSI_NORMAL, wpa_ent_str,
						   (idx % 2 == 1) ? ANSI_COLOR_BLACK : ANSI_COLOR_DEFULT);
				} else if (strstr(params[idx][SCAN_FLGA_IDX], "WPA2-") != 0) {
					// Start :__SUPPORT_WPA3_SAE__
                    char wpa_wpa3_str[20];
                    unsigned char supp_flg = pdTRUE; // support flag

                    memset(wpa_wpa3_str, 0, 20);

					if (strstr(params[idx][SCAN_FLGA_IDX], "+SAE") != 0) {
						strcat(wpa_wpa3_str, "WPA2 / SAE");
					} else if (strstr(params[idx][SCAN_FLGA_IDX], "-SAE") != 0) {
						if (strstr(params[idx][SCAN_FLGA_IDX], "-SHA256") != 0) {
							strcat(wpa_wpa3_str, "WPA2PMF/SAE"); /* PMF Required */
						} else {
							strcat(wpa_wpa3_str, "SAE");
#if !defined __SUPPORT_WPA3_PERSONAL__
                            supp_flg = pdFALSE;
#endif /* __SUPPORT_WPA3_PERSONAL__ */
						}
					}
					else
					// End : __SUPPORT_WPA3_SAE__
					if (strstr(params[idx][SCAN_FLGA_IDX], "+OWE") != 0) {
						strcat(wpa_wpa3_str, "WPA2 / OWE");
					} else if (strstr(params[idx][SCAN_FLGA_IDX], "-OWE") != 0) {
#if !defined __SUPPORT_WPA3_PERSONAL__
                        supp_flg = pdFALSE;
#endif /* __SUPPORT_WPA3_PERSONAL__ */
						strcat(wpa_wpa3_str, "OWE");

					} else if (strstr(params[idx][SCAN_FLGA_IDX], "WPA-") != 0) {
						if (strstr(params[idx][SCAN_FLGA_IDX], "-SHA256") != 0) {
							strcat(wpa_wpa3_str, "WPA WPA2PMF"); /* PMF Required */
						} else {
							strcat(wpa_wpa3_str, "WPA / WPA2");
						}
					} else {
						if (strstr(params[idx][SCAN_FLGA_IDX], "-SHA256") != 0) {
							strcat(wpa_wpa3_str, "WPA2PMF"); /* PMF Required */
						} else {
							strcat(wpa_wpa3_str, "WPA2");
						}
					}
                    
					if (supp_flg == pdFALSE) {
						PRINTF(ANSI_COLOR_LIGHT_RED);
					}
					PRINTF("%12s" ANSI_NORMAL "%s" ANSI_NORMAL, wpa_wpa3_str,
						   (idx % 2 == 1) ? ANSI_COLOR_BLACK : ANSI_COLOR_DEFULT);
				} else {
					PRINTF("%12s", "WPA");
				}
			} else if (strstr(params[idx][SCAN_FLGA_IDX], "WEP") != 0) {
				PRINTF("%12s", "WEP");
			} else if (strstr(params[idx][SCAN_FLGA_IDX], "MESH") != 0) {
				if (strstr(params[idx][SCAN_FLGA_IDX], "RSN-SAE-CCMP") != 0) {
					PRINTF("%12s", "MESH SAE");
				} else {
					PRINTF("%12s", "MESH OPEN");
				}
			} else {
				PRINTF("%12s", " "); /*  OPEN */
			}

			PRINTF(ANSI_BCOLOR_DEFULT ANSI_COLOR_DEFULT "\n");

			if (	(params[idx][SCAN_SSID_IDX][0] != HIDDEN_SSID_DETECTION_CHAR)
				&& strstr(ssid_txt, "\\x") != 0) {

				PRINTF("%5s%s\n", " ", params[idx][SCAN_SSID_IDX]);
			}
		}

		/*	[END] ======= Separate param ======= */
		print_separate_bar('-', 76, 1);
		PRINTF(ANSI_COLOR_CYAN "[M] Manual Input" ANSI_COLOR_DEFULT "\n");
		PRINTF(ANSI_COLOR_CYAN "[Enter] Rescan" ANSI_COLOR_DEFULT "\n");
		print_separate_bar('=', 76, 1);

SELECT_SSID:
		sel_ssid = 0;
		PRINTF("\n" ANSI_REVERSE " Select SSID ?" ANSI_NORMAL " (1~%02d/" ANSI_BOLD "M"
			   ANSI_NORMAL "anual/" ANSI_BOLD "Q" ANSI_NORMAL "uit) : ", param_cnt - 1);

		sel_ssid = getNum();

		if (sel_ssid == RET_QUIT) {
			vPortFree(scan_result);

			return E_QUIT;
		} else if (sel_ssid >= param_cnt) {
			goto SELECT_SSID;
		} else if (sel_ssid == RET_MANUAL) {	/* Manual Input */
			vPortFree(scan_result);
			*select_ssid = RET_MANUAL;

			return E_INPUT_SSID;
		} else if (sel_ssid == RET_DEFAULT) {	/* Rescan */
			goto SCAN_SSID;
		} else if (sel_ssid < 0) {
			goto SELECT_SSID;
		} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "MESH") != 0) {
			/* MESH */
			PRINTF(" " ANSI_COLOR_LIGHT_RED "%-50s" ANSI_COLOR_DEFULT, "MESH Netowrk !!!\n");
			goto SELECT_SSID;
		} else if (params[sel_ssid][SCAN_SSID_IDX][0] == HIDDEN_SSID_DETECTION_CHAR) {	/* Hidden SSID */
			PRINTF(" " ANSI_COLOR_LIGHT_RED "%-50s" ANSI_COLOR_DEFULT, "Hidden SSID !!!\n");
			*channel_selected = atoi(params[sel_ssid][SCAN_FREQ_IDX]);
			*p_connect_hidden_ssid = 1;

INPUT_HIDDEN_SSID:

			/* hidden ssid input */
			do {
				PRINTF("\n" ANSI_REVERSE " SSID(NETWORK NAME) ?" ANSI_NORMAL " : ");
				getStr_len = getStr(p_ssid, 32);  /* Max 32 */

				if (getStr_len == 0) {
					continue;
				} else if (getStr_len == RET_QUIT) {
					vPortFree(scan_result);
					return E_QUIT;
				}

				/* Check Control character */
				for (int ii = 0; ii < getStr_len; ii++) {
					if (iscntrl((int)(p_ssid[ii])) != 0) {
						goto INPUT_HIDDEN_SSID;
					}
				}

				break;
			} while (1);

		} else if (sel_ssid > 0 && sel_ssid < param_cnt) {
			strncpy(p_ssid, params[sel_ssid][SCAN_SSID_IDX], 127);
			*channel_selected = atoi(params[sel_ssid][SCAN_FREQ_IDX]);
			*p_connect_hidden_ssid = 0;
		} else {	/* RET_NODIGIT */
			goto SELECT_SSID;
		}

		/* Auto Security */
		if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA") != 0) {	/* WPA */
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__)
			/* Proto & Key_mgmt */
			if (   strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA2-SAE-CCMP]") != 0
				|| strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA2-SAE+PSK-SHA256-CCMP]") != 0) {
				*p_auth_mode = E_AUTH_MODE_SAE;
				*p_encryp_mode = E_ENCRYP_MODE_CCMP;
				*p_pmf = E_PMF_MANDATORY; /* PMF Required */
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA2-PSK+SAE-CCMP]") != 0) {
				*p_auth_mode = E_AUTH_MODE_RSN_SAE;
				*p_encryp_mode = E_ENCRYP_MODE_CCMP;
				*p_pmf = E_PMF_OPTIONAL; /* PMF Capable(Optional) */
			}
#else //  __SUPPORT_WPA3_SAE__ && __SUPPORT_WPA3_PERSONAL__
			if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA2-PSK+SAE-CCMP]") != 0) {
				*p_auth_mode = E_AUTH_MODE_WPA_RSN;
				*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
#if defined __SUPPORT_IEEE80211W__
				*p_pmf = E_PMF_OPTIONAL; /* PMF Capable(Optional) */
#endif // __SUPPORT_IEEE80211W__
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "PSK-SHA256-CCMP") != 0) {
				/* [WPA2-SAE+PSK-SHA256-CCMP] */
#if defined __SUPPORT_IEEE80211W__
				*p_auth_mode = E_AUTH_MODE_RSN;
				*p_encryp_mode = E_ENCRYP_MODE_CCMP;
				*p_pmf = E_PMF_MANDATORY; /* PMF Required */
#else
				PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD "This SDK does not support PMF"
					   ANSI_NORMAL ANSI_COLOR_DEFULT "\n");

				goto SELECT_SSID;
#endif // __SUPPORT_IEEE80211W__
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "-SAE") != 0) {
				/* [WPA2-SAE-CCMP] */
				PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD "This SDK does not support WPA3-SAE."
					   ANSI_NORMAL ANSI_COLOR_DEFULT "\n");

				goto SELECT_SSID;
			}
#endif /* __SUPPORT_WPA3_SAE__ */
			else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "OWE") != 0) {
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
				*p_auth_mode = E_AUTH_MODE_OWE;
				*p_pmf = E_PMF_MANDATORY; /* PMF Required */
				vPortFree(scan_result);
				return E_AVD_CFG;
#else
				PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD "This SDK does not support WPA3-OWE."
					   ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
				goto SELECT_SSID;
#endif /* __SUPPORT_WPA3_OWE__ */
			// =============== Enterprise =================
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP") != 0) {
#ifdef __SUPPORT_WPA_ENTERPRISE__
				//  WPA2-EAP-SUITE-B-192-GCMP-256 : WPA3-Enterprise 192bits
				if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP-SUITE-B-192") != 0) { // WPA3-Enterprise 192Bits Mode
#if defined __SUPPORT_WPA3_ENTERPRISE_192B__
					*p_auth_mode = E_AUTH_MODE_WPA3_ENT_192B;
					*p_encryp_mode = E_ENCRYP_MODE_GCMP_256;
					*p_pmf = E_PMF_MANDATORY; /* PMF Required */
#else
					PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
					   "This SDK does not support WPA3-Enterprise 192Bits." ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
					goto SELECT_SSID;
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
				}
				else
				{
					//<WPA3-ENT> PMF 2			: EAP-SHA256-CCMP' : WPA3-Enterprise 128bits Only
					if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP-SHA256-CCMP") != 0
						&& strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP-CCMP") == 0) {
#ifdef __SUPPORT_WPA3_ENTERPRISE__
						*p_auth_mode = E_AUTH_MODE_WPA3_ENT; /* WPA3_ENT(PMF+WPA2) */
						*p_encryp_mode = E_ENCRYP_MODE_CCMP;
						*p_pmf = E_PMF_MANDATORY; /* PMF Required */
#else
						PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
						   "This SDK does not support WPA3-Enterprise." ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
						goto SELECT_SSID;
#endif // __SUPPORT_WPA3_ENTERPRISE__
					}
					//<WPA2/WPA3-ENT> PMF 1	: WPA2-EAP-CCMP, EAP-SHA256-CCMP
					else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA2-EAP-CCMP") != 0
						&& strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP-SHA256-CCMP") != 0) {
#ifdef __SUPPORT_WPA3_ENTERPRISE__
						*p_auth_mode = E_AUTH_MODE_WPA2_WPA3_ENT; /* WPA2_ENT or WPA3_ENT(PMF+WPA2) */
						*p_encryp_mode = E_ENCRYP_MODE_CCMP;
						*p_pmf = E_PMF_OPTIONAL; /* PMF Capable(Optional) */
#else
						PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
						   "This SDK does not support WPA3-Enterprise." ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
						goto SELECT_SSID;
#endif // __SUPPORT_WPA3_ENTERPRISE__
					}
					//<WPA1/WPA2-ENT> PMF 1 or 0 : WPA-EAP WPA2-EAP
					else if ((strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA2-EAP") != 0
							|| strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA-EAP") != 0)
						&& strstr(params[sel_ssid][SCAN_FLGA_IDX], "EAP-SHA256") == 0) {
#ifdef __SUPPORT_WPA_ENTERPRISE__
						*p_auth_mode = E_AUTH_MODE_ENT; /* WPA3_ENT(PMF+WPA2) */
						*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
						*p_pmf = E_PMF_OPTIONAL; /* PMF Capable(Optional) */
#else
						PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
							   "This SDK does not support WPA-Enterprise." ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
						goto SELECT_SSID;
#endif // __SUPPORT_WPA_ENTERPRISE__
					}
					else
					{
						PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
							   "This SDK does not support Unknown Enterprise." ANSI_NORMAL ANSI_COLOR_DEFULT "\n(%s)\n",
							   params[sel_ssid][SCAN_FLGA_IDX]);
						goto SELECT_SSID;
					}
				}

				vPortFree(scan_result);
				return E_INPUT_EAP_METHOD;
#else
				PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD
					   "This SDK does not support WPA-Enterprise." ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
				goto SELECT_SSID;
#endif // __SUPPORT_WPA_ENTERPRISE__
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA-PSK-") != 0
					 && strstr(params[sel_ssid][SCAN_FLGA_IDX], "WPA2-PSK-") != 0) {
				*p_auth_mode = E_AUTH_MODE_WPA_RSN; /* WPA1 / 2 */

#if defined __SUPPORT_IEEE80211W__
				if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "PSK-SHA256-") != 0) {
					/* [WPA-PSK-CCMP][WPA2-PSK-SHA256-CCMP] */
					*p_pmf = E_PMF_OPTIONAL; /* PMF Capable(Optional) */
				}
				
#endif // __SUPPORT_IEEE80211W__
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "PSK-SHA256-") != 0) {
				/* WPA2-PSK-SHA256-CCMP] */
#if defined __SUPPORT_IEEE80211W__
				*p_auth_mode = E_AUTH_MODE_RSN; /* WPA2-PSK-SHA256- */
				*p_pmf = E_PMF_MANDATORY; /* PMF Required */
#else
				PRINTF(ANSI_COLOR_LIGHT_RED ANSI_BOLD "This SDK does not support PMF."
					   ANSI_NORMAL ANSI_COLOR_DEFULT "\n");
				goto SELECT_SSID;
#endif // __SUPPORT_IEEE80211W__
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA-PSK") != 0) {
				/* [WPA-PSK- */
				// *p_auth_mode = E_AUTH_MODE_WPA;
				*p_auth_mode = E_AUTH_MODE_WPA_RSN; /* WPA-PSK WPA2-PSK */
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "[WPA2-PSK") != 0) {
				/* [WPA2-PSK- */
				*p_auth_mode = E_AUTH_MODE_WPA_RSN; /* WPA-PSK WPA2-PSK */
			}

			/* Pairwise : CCMP / TKIP */
			if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "CCMP+TKIP") != 0) {
				/* WPAx-xxx-CCMP+TKIP */
				*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "CCMP]") != 0) {
				if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "SHA256") == 0	//  WPA2 PMF x
					&& strstr(params[sel_ssid][SCAN_FLGA_IDX], "SAE") == 0) // && SAE x
				{
				    if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "SHA256") == 0) {	//  WPA2 PMF x
    					/* WPA2-xxx-CCMP*/
    					*p_encryp_mode = E_ENCRYP_MODE_CCMP;
				    } else {
    					/* WPAx-xxx-CCMP+TKIP */
    					*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
					}
					/* WPAx-xxx-CCMP+TKIP */
					*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;

				} else {
#if !defined __SUPPORT_WPA3_SAE__ || !defined __SUPPORT_WPA3_PERSONAL__
				    if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "SHA256") != 0) {	//  WPA2 PMF x
    					/* WPA2-xxx-CCMP*/
    					*p_encryp_mode = E_ENCRYP_MODE_CCMP;
				    } else {
    					/* WPAx-xxx-CCMP+TKIP */
    					*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
					}
#else
					/* WPAx-xxx-CCMP */
					*p_encryp_mode = E_ENCRYP_MODE_CCMP;
#endif /* __SUPPORT_WPA3_SAE__ */
				}
			} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "TKIP]") != 0) {
				if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "SHA256") == 0		// WPA2 PMF x
					&& strstr(params[sel_ssid][SCAN_FLGA_IDX], "SAE") == 0) {	// && SAE x
					/* WPAx-xxx-CCMP+TKIP */
					*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
				} else {
#if !defined __SUPPORT_WPA3_SAE__ && !defined __SUPPORT_WPA3_PERSONAL__
					/* WPAx-xxx-CCMP+TKIP */
					*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
#else
					/* WPAx-xxx-TKIP */
					*p_encryp_mode = E_ENCRYP_MODE_TKIP;
#endif /* __SUPPORT_WPA3_SAE__ */
				}
			}

			vPortFree(scan_result);
			return E_INPUT_PSK_KEY;
		} else if (strstr(params[sel_ssid][SCAN_FLGA_IDX], "WEP") != 0) {	/* WEP */
			*p_auth_mode = E_AUTH_MODE_WEP;
			vPortFree(scan_result);
			return E_INPUT_WEP;
		} else {	/* OPEN */
			*p_auth_mode = E_AUTH_MODE_NONE;
			vPortFree(scan_result);
			return E_AVD_CFG;
		}
	}
	
	vPortFree(scan_result);
	return E_CONTINUE;
}

static unsigned char setup_stop_services(void)
{
	int		getStr_len = 0;
	char	input_str[3];
	char	*reply = NULL;

	do {
		PRINTF("\n" ANSI_BOLD ANSI_COLOR_GREEN "Stop all services for the setting." ANSI_NORMAL
			   "\n" ANSI_REVERSE " Are you sure ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD
			   "N"  ANSI_NORMAL "o] : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'N' || input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				PRINTF("\nSetting canceled.\n");
				return E_QUIT;
			}
		}
	} while (!(input_str[0] == 'Y' && getStr_len == 1));

	if (chk_dpm_mode() == pdTRUE) {
		/* Pause DPM Sleep. */
		hold_dpm_sleepd();
	}

	terminate_sys_apps();

#if defined (__SUPPORT_MQTT__)
	mqtt_client_termination();
#endif // __SUPPORT_MQTT__

#if defined ( __SUPPORT_WIFI_CONN_CB__ )
	delete_user_wifi_disconn();
#endif  // __SUPPORT_WIFI_CONN_CB__

	reply = pvPortMalloc(16);

	if (reply == NULL) {
		PRINTF("Malloc Error - reply\n");
		return E_QUIT;
	}

	memset(reply, 0, 16);

	if (get_run_mode() == SYSMODE_STA_ONLY) {
		/* Timer Clearing : for RamLib / LMAC */
		do_set_all_timer_kill();

		/* Deactivate all dpm related timer */
		disable_all_dpm_timer();
	}

	if (sntp_support_flag) {
		sntp_stop();
	}

	set_debug_dhcpc(0); // dhcp client debug level

	da16x_cli_reply("set_log all 0", NULL, reply);

	if (strcmp(reply, "OK") != 0) {
		return E_ERROR;
	}

	da16x_cli_reply("flush", NULL, reply);		/* stop services */
	da16x_cli_reply("set_log all 0", NULL, reply);

	vPortFree(reply);
	return E_CONTINUE;
}


#define COUNTRY_MAX cc_power_table_size()
#define COUNTRY_LINE_MAX	20
#define COUNTRY_LINE_CR_CNT	(COUNTRY_MAX / COUNTRY_LINE_MAX)

unsigned char setup_country_code(char *p_country, unsigned char *p_channel)
{
	int	str_len = 0;

	char *buff_str = NULL;
	char tmp_country[8];
	char *t_reply = NULL;
	char cmdline[16];

	buff_str = pvPortMalloc((4 * COUNTRY_MAX) + COUNTRY_LINE_CR_CNT + 1);

	if (buff_str == NULL) {
		PRINTF("[%s] Failed to malloc (buff_str)\n", __func__);
		return E_QUIT;
	}

	memset(buff_str, 0, (4 * COUNTRY_MAX) + COUNTRY_LINE_CR_CNT + 1);
	memset(cmdline, 0, 16);

	t_reply = pvPortMalloc(16);
	if (t_reply == NULL) {
		PRINTF("[%s] Failed to malloc (reply)\n", __func__);
		vPortFree(t_reply);
		return E_QUIT;
	}

	memset(t_reply, 0, 16);

	for (str_len = 0; (UINT)str_len < COUNTRY_MAX; str_len++) {
		memset(tmp_country, 0, 8);
		da16x_sprintf(tmp_country, "%-3s ", country_code(str_len));
		da16x_sprintf(buff_str, "%s%s", buff_str, tmp_country);

		if (str_len % COUNTRY_LINE_MAX == 19 && str_len > 0) {
			da16x_sprintf(buff_str, "%s\n", buff_str, tmp_country);
		}
	}

	do {
		PRINTF("\nCountry Code List:\n");
		PRINTF("%s\n", buff_str);

		PRINTF("\n" ANSI_REVERSE " COUNTRY CODE ?" ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL
			   "uit] (Default %s) : ", COUNTRY_CODE_DEFAULT);

		str_len = getStr(p_country, 3);  /* Max 3 */

		for (int i = 0; i < str_len; i++) {
			p_country[i] = (char)toupper(p_country[i]);
		}

		if ((str_len == 1 && p_country[0] == 'Q') || str_len == RET_QUIT) {
			vPortFree(t_reply);
			vPortFree(buff_str);
			return E_QUIT;
		} else if (!str_len) {
			strcpy(p_country, COUNTRY_CODE_DEFAULT);
			PRINTF("Default Country: %s\n", p_country);
		} else {
			int ch = 0;
			ch = chk_channel_by_country(p_country, -1, 0, NULL);

			if (ch < CHANNEL_AUTO && ch != -2) {
				PRINTF(ANSI_COLOR_LIGHT_RED "\nIncorrect Country Code!!\n" ANSI_NORMAL);
				continue;
			}

			*p_channel = 0;
		}

		sprintf(cmdline, "country %s", p_country);
		da16x_cli_reply(cmdline, NULL, t_reply);

		break;
	} while (1);

	vPortFree(t_reply);
	vPortFree(buff_str);
	return E_CONTINUE;
}


unsigned char setup_sysmode(unsigned char *p_sysmode)
{
	int input_num;
	
	/* Configuration Menu - START ************************************************/
	do {
		PRINTF("\nSYSMODE(WLAN MODE) ?\n");
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " Station\n", E_SYSMODE_STA);
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " Soft-AP\n", E_SYSMODE_AP);
#if defined ( __SUPPORT_P2P__ )
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " WiFi Direct\n", E_SYSMODE_P2P);
#endif	// __SUPPORT_P2P__

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __SUPPORT_P2P__ )
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " WiFi Direct P2P GO Fixed\n", E_SYSMODE_P2P_GO);
  #endif // __SUPPORT_P2P__
		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " Station & SOFT-AP\n", E_SYSMODE_STA_AP);
  #if defined ( __SUPPORT_P2P__ )

		PRINTF("\t" ANSI_BOLD "%d." ANSI_NORMAL " Station & WiFi Direct\n", E_SYSMODE_STA_P2P);
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__

		PRINTF(ANSI_REVERSE " MODE ? " ANSI_NORMAL " [1/2/");

#if defined ( __SUPPORT_P2P__ )
		PRINTF("%d/", E_SYSMODE_P2P);		//  WiFi Direct
#endif	// __SUPPORT_P2P__

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __SUPPORT_P2P__ )
		PRINTF("%d/", E_SYSMODE_P2P_GO);	// WiFi Direct P2P GO Fixed
  #endif // __SUPPORT_P2P__
		PRINTF("%d/",  E_SYSMODE_STA_AP);	// Station & SOFT-AP
  #if defined ( __SUPPORT_P2P__ )
		PRINTF("%d/", E_SYSMODE_STA_P2P);	// Station & WiFi Direct
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__
		
		PRINTF(ANSI_BOLD "Q" ANSI_NORMAL "uit] (Default Station) : ");

		input_num = getNum();

		*p_sysmode = (unsigned char)input_num;

		if (input_num == RET_DEFAULT) {  /* deafult mode */
			*p_sysmode = E_SYSMODE_STA;
		} else if (input_num == RET_QUIT) {
			return E_QUIT;
		}
	} while (*p_sysmode < E_SYSMODE_STA
#if defined (__SUPPORT_MESH_PORTAL__)
			|| *p_sysmode > E_SYSMODE_STA_MESH_PORTAL
#elif defined (__SUPPORT_MESH_POINT_ONLY__)
				|| *p_sysmode > E_SYSMODE_MESH_PONIT
#elif defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __SUPPORT_P2P__ )
			|| *p_sysmode > E_SYSMODE_STA_P2P
  #endif	// __SUPPORT_P2P__
#else
			|| *p_sysmode > E_SYSMODE_AP
#endif // __SUPPORT_MESH_PORTAL__
		  );

	/* Configuration Menu - END  **************************************************/
	return E_CONTINUE;
}


void setup_selcet_interface(unsigned char sysmode, unsigned char *p_iface_start, unsigned char *p_iface_end)
{

	if (   sysmode == E_SYSMODE_AP
#if defined ( __SUPPORT_P2P__ )
		|| sysmode == E_SYSMODE_P2P
		|| sysmode == E_SYSMODE_P2P_GO
#endif	// __SUPPORT_P2P__
	   )
	{
		/* AP only , P2P Device only , P2P GO only, MESH Point Only */
		*p_iface_start = WLAN1_IFACE;
		*p_iface_end   = WLAN1_IFACE;
	}
	else if (   sysmode == E_SYSMODE_STA_AP
#if defined ( CONFIG_P2P )
			|| sysmode == E_SYSMODE_STA_P2P
#endif	// CONFIG_P2P
			)
	{
		/* Station & AP, Station & P2P Device, Station & MESH Portal*/
		*p_iface_start = WLAN0_IFACE;
		*p_iface_end   = WLAN1_IFACE;
	} else {
		/* Station only */
		*p_iface_start = WLAN0_IFACE;
		*p_iface_end   = WLAN0_IFACE;
	}
}


unsigned char setup_ssid_input_ui(char *p_ssid, int p_cur_iface)
{
	char	ssid_str[35] = {0, };
	int		getStr_len = 0;

	memset(ssid_str, 0, 35);
	
	if (p_cur_iface == WLAN1_IFACE) {
		/* SSID+MACADDRESS */
		if (gen_ssid(CHIPSET_NAME, WLAN1_IFACE, 0, ssid_str, sizeof(ssid_str)) == -1) {
			PRINTF("SSID Error\n");
		}
	}

	do {
		if (p_cur_iface == WLAN1_IFACE) {
			/* Soft AP */
			PRINTF("\n" ANSI_REVERSE " SSID ? (Default %s)" ANSI_NORMAL " : ", ssid_str);
		} else {
			/* Station */
			PRINTF("\n" ANSI_REVERSE " SSID ? " ANSI_NORMAL " : ");
		}

		getStr_len = getStr(p_ssid, 32);  /* Max 32 */

		if (getStr_len == 0) {
			if (p_cur_iface == WLAN1_IFACE && strlen(ssid_str) > 0) {
				strcpy (p_ssid, ssid_str);
				break;
			} else {
				continue;
			}
		} else if (getStr_len == RET_QUIT) {
			return E_QUIT;
		}

		break;
	} while (1);

	return E_CONTINUE;
}


unsigned char setup_select_channel(unsigned char sysmode, unsigned char cur_iface, char *p_country, unsigned char *p_channel)
{
	int firstCH, finshCH;
	int input_num;

	/* AP Country Code && Channel */
	if (sysmode == E_SYSMODE_AP) {	 /* 1: SoftAP Only (wlan0)  */

		country_channel_range (p_country, &firstCH, &finshCH);

		/* INPUT_AP_CHANNEL */
INPUT_AP_CHANNEL:

		do {
			PRINTF("\n" ANSI_REVERSE " CHANNEL ?" ANSI_NORMAL " [%d~%d, %s/" ANSI_BOLD "Q"
				   ANSI_NORMAL "uit] (Default %s) : ", firstCH, finshCH
				   , "Auto:0", "Auto"
				  );

			input_num = getNum();
			*p_channel = (unsigned char)input_num;

			if (input_num == RET_DEFAULT) {	/* deafult  */
				*p_channel = CHANNEL_DEFAULT;
			} else if (input_num == RET_QUIT) {
				return E_QUIT;
			} else if ((input_num < 0) && (input_num != RET_QUIT)) {
				continue;
			}

			if (input_num < CHANNEL_AUTO) {
				*p_channel = CHANNEL_DEFAULT; //set default 0(Auto)
			} else {
				input_num = chk_channel_by_country(p_country, input_num, 0, NULL);

				*p_channel = (unsigned char)input_num;

				if (input_num < CHANNEL_AUTO) {
					goto INPUT_AP_CHANNEL;
				}
			}

			break;
		} while (1);

	} else if (sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE) {	/* 3: STA(wlan0) & SoftAP (wlan1) */
		*p_channel = CHANNEL_DEFAULT;
	}

	return E_CONTINUE;
}


unsigned char setup_input_auth(unsigned char sysmode, unsigned char cur_iface, unsigned char *p_auth_mode)
{

	DA16X_UNUSED_ARG(sysmode);

	int input_num;

	/* INPUT AUTH */
	{
		do {
			PRINTF("\n\nAUTHENTICATION ?\n" \
				   "\t%2d. OPEN\n", E_AUTH_MODE_NONE);

			PRINTF("\t%2d. WEP%s\n", E_AUTH_MODE_WEP,
				   cur_iface == WLAN1_IFACE ? "(Unsupported)" : "");

			PRINTF("\t%2d. WPA-PSK\n", E_AUTH_MODE_WPA);
#if defined ( __SUPPORT_WPA3_SAE__ ) && defined ( __SUPPORT_WPA3_PERSONAL__ )
			PRINTF("\t%2d. WPA2-PSK\n", E_AUTH_MODE_RSN);
#else
			PRINTF("\t%2d. WPA2-PSK (Recommend)\n", E_AUTH_MODE_RSN);
#endif /* __SUPPORT_WPA3_SAE__ && __SUPPORT_WPA3_PERSONAL__ */
			PRINTF("\t%2d. WPA/WPA2-PSK\n", E_AUTH_MODE_WPA_RSN);

#if defined ( __SUPPORT_WPA3_SAE__ ) && defined ( __SUPPORT_WPA3_PERSONAL__ )
			PRINTF("\t%2d. WPA3-SAE\n", E_AUTH_MODE_SAE);
			PRINTF("\t%2d. WPA2-PSK+WPA3-SAE (Recommend)\n", E_AUTH_MODE_RSN_SAE);
#endif /* __SUPPORT_WPA3_SAE__ && __SUPPORT_WPA3_PERSONAL__*/
#if defined ( __SUPPORT_WPA3_OWE__ ) && defined ( __SUPPORT_WPA3_PERSONAL__ )
			PRINTF("\t%2d. WPA3-OWE\n", E_AUTH_MODE_OWE);
#endif /* __SUPPORT_WPA3_OWE__ && __SUPPORT_WPA3_PERSONAL__*/

#ifdef __SUPPORT_WPA_ENTERPRISE__
			if (cur_iface == WLAN0_IFACE) {
				PRINTF("\t%2d. WPA/WPA2-Enterprise\n", E_AUTH_MODE_ENT);
  #ifdef __SUPPORT_WPA3_ENTERPRISE__
				PRINTF("\t%2d. WPA2/WPA3-Enterprise (128bits)\n", E_AUTH_MODE_WPA2_WPA3_ENT);
				PRINTF("\t%2d. WPA3-Enterprise (128bits)\n", E_AUTH_MODE_WPA3_ENT);
    #if defined __SUPPORT_WPA3_ENTERPRISE_192B__
				PRINTF("\t%2d. WPA3-Enterprise (192Bits)\n", E_AUTH_MODE_WPA3_ENT_192B);
    #endif // __SUPPORT_WPA3_ENTERPRISE_192B__
  #endif // __SUPPORT_WPA3_ENTERPRISE__
			}
#endif // __SUPPORT_WPA_ENTERPRISE__

			PRINTF(ANSI_REVERSE " AUTHENTICATION ?" ANSI_NORMAL " [%d/", E_AUTH_MODE_NONE);

			if (cur_iface == WLAN0_IFACE) {
				PRINTF("%d/", E_AUTH_MODE_WEP);
			}

			PRINTF("%d/", E_AUTH_MODE_WPA);
			PRINTF("%d/", E_AUTH_MODE_RSN);
			PRINTF("%d/", E_AUTH_MODE_WPA_RSN);
#if defined ( __SUPPORT_WPA3_SAE__ ) && defined ( __SUPPORT_WPA3_PERSONAL__ )
			PRINTF("%d/", E_AUTH_MODE_SAE);
			PRINTF("%d/", E_AUTH_MODE_RSN_SAE);
#endif /* __SUPPORT_WPA3_SAE__ && __SUPPORT_WPA3_PERSONAL__ */
#if defined ( __SUPPORT_WPA3_OWE__ ) && defined ( __SUPPORT_WPA3_PERSONAL__ )
			PRINTF("%d/", E_AUTH_MODE_OWE);
#endif /* __SUPPORT_WPA3_OWE__ && __SUPPORT_WPA3_PERSONAL__*/

#ifdef __SUPPORT_WPA_ENTERPRISE__
			if (cur_iface == WLAN0_IFACE) {
				PRINTF("%d/", E_AUTH_MODE_ENT);
  #ifdef __SUPPORT_WPA3_ENTERPRISE__
				PRINTF("%d/", E_AUTH_MODE_WPA2_WPA3_ENT);
				PRINTF("%d/", E_AUTH_MODE_WPA3_ENT);
    #if defined __SUPPORT_WPA3_ENTERPRISE_192B__
				PRINTF("%d/", E_AUTH_MODE_WPA3_ENT_192B);
    #endif // __SUPPORT_WPA3_ENTERPRISE_192B__
  #endif // __SUPPORT_WPA3_ENTERPRISE__
			}
#endif // __SUPPORT_WPA_ENTERPRISE__

			PRINTF(ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

			input_num = getNum();
			*p_auth_mode = (unsigned char)input_num;

			if (input_num == RET_DEFAULT) {	/* deafult  */
				*p_auth_mode = E_AUTH_MODE_NONE;
			} else if (input_num == RET_QUIT) {
				return E_QUIT;
			}
		} while (input_num < E_AUTH_MODE_NONE
#if ((defined __SUPPORT_WPA3_SAE__) || (defined __SUPPORT_WPA3_OWE__)) && defined __SUPPORT_WPA3_PERSONAL__
				|| input_num >= E_AUTH_MODE_MAX
#else
				|| input_num >= E_AUTH_MODE_MAX
#endif /*  __SUPPORT_WPA3_SAE__ || __SUPPORT_WPA3_OWE__ */

				|| (cur_iface == WLAN1_IFACE && input_num == E_AUTH_MODE_WEP)
#ifdef __SUPPORT_WPA_ENTERPRISE__
  #ifdef __SUPPORT_WPA3_ENTERPRISE__
    #ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
				|| (cur_iface == WLAN1_IFACE && input_num == E_AUTH_MODE_WPA3_ENT_192B)
    #else
				|| (cur_iface == WLAN1_IFACE && input_num == E_AUTH_MODE_WPA3_ENT)
    #endif // __SUPPORT_WPA3_ENTERPRISE_192B__
  #else
				|| (cur_iface == WLAN1_IFACE && input_num == E_AUTH_MODE_ENT)
  #endif // __SUPPORT_WPA3_ENTERPRISE__
#endif // __SUPPORT_WPA_ENTERPRISE__
				);

		PRINTF("\n");
	}

	return E_CONTINUE;
}


unsigned char setup_select_encryption(unsigned char auth_mode,
											  unsigned char *p_encryp_mode
#if defined __SUPPORT_IEEE80211W__
											  , unsigned char *p_pmf
#endif // __SUPPORT_IEEE80211W__
											  )
{
#if defined __SUPPORT_IEEE80211W__
	DA16X_UNUSED_ARG(p_pmf);
#endif // __SUPPORT_IEEE80211W__

	int input_num;

#ifdef __SUPPORT_WPA_ENTERPRISE__
	if (auth_mode == E_AUTH_MODE_ENT)
	{
		*p_pmf = E_PMF_OPTIONAL; /* PMF Capable */
		*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
		return E_CONTINUE;
	}
#ifdef __SUPPORT_WPA3_ENTERPRISE__
	else if (auth_mode == E_AUTH_MODE_WPA3_ENT)
	{
		*p_pmf = E_PMF_MANDATORY; /* PMF Required */
		*p_encryp_mode = E_ENCRYP_MODE_CCMP;
		return E_CONTINUE;
	}
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
	else if (auth_mode == E_AUTH_MODE_WPA3_ENT_192B)
	{
		*p_pmf = E_PMF_MANDATORY; /* PMF Required */
		*p_encryp_mode = E_ENCRYP_MODE_GCMP_256;
		return E_CONTINUE;
	}
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
#endif // __SUPPORT_WPA3_ENTERPRISE__
#endif // __SUPPORT_WPA_ENTERPRISE__
	
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
	if (auth_mode == E_AUTH_MODE_OWE) {
		*p_pmf = E_PMF_MANDATORY; /* PMF Required */

		return E_CONTINUE;
	} else
#endif /* __SUPPORT_WPA3_OWE__ */
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
	if (auth_mode == E_AUTH_MODE_SAE || auth_mode == E_AUTH_MODE_RSN_SAE) {
		*p_encryp_mode = E_ENCRYP_MODE_CCMP;

		if (auth_mode == E_AUTH_MODE_RSN_SAE) {
			*p_pmf = E_PMF_OPTIONAL; /* PMF Capable */
		} else if (auth_mode == E_AUTH_MODE_SAE) {
			*p_pmf = E_PMF_MANDATORY; /* PMF Required */
		}
		
		return E_CONTINUE;
	} else
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
#if defined __SUPPORT_IEEE80211W__
    if (auth_mode == E_AUTH_MODE_RSN && *p_pmf == E_PMF_MANDATORY) {
        return E_CONTINUE;
    } 
#endif // __SUPPORT_IEEE80211W__

	if (auth_mode >= E_AUTH_MODE_WPA && auth_mode <= E_AUTH_MODE_WPA_RSN) {

		/* WPA-PSK */
		do {
			PRINTF("\nENCRYPTION ?\n" \
				   "\t%d. TKIP",
				   E_ENCRYP_MODE_TKIP);
			PRINTF(ANSI_COLOR_RED " (CAUTION: Unsupported 802.11N Mode)" ANSI_NORMAL "\n");
			PRINTF("\t%d. AES(CCMP)\n" \
				   "\t%d. TKIP/AES(CCMP)\n" \
				   ANSI_REVERSE " ENCRYPTION ?" ANSI_NORMAL " [%d/%d/%d/" ANSI_BOLD "Q" ANSI_NORMAL
				   "uit] : ",
				   E_ENCRYP_MODE_CCMP,
				   E_ENCRYP_MODE_TKIP_CCMP,
				   E_ENCRYP_MODE_TKIP,
				   E_ENCRYP_MODE_CCMP,
				   E_ENCRYP_MODE_TKIP_CCMP);

			input_num = getNum();
			*p_encryp_mode = (unsigned char)input_num;

			if (input_num == RET_DEFAULT) {	/* deafult  */
				*p_encryp_mode = E_ENCRYP_MODE_TKIP_CCMP;
			} else if (input_num == RET_QUIT) {
				return E_QUIT;
			}
		} while (input_num < E_ENCRYP_MODE_TKIP || input_num > E_ENCRYP_MODE_TKIP_CCMP);

	}

	return E_CONTINUE;
}


#ifdef __SUPPORT_WPA_ENTERPRISE__
unsigned char setup_input_eap_method(unsigned char auth_mode, unsigned char *p_eap_auth_mode, unsigned char *p_phase2)
{
	int input_num;

	if (auth_mode < E_AUTH_MODE_ENT) {
		return E_CONTINUE;
	}

	PRINTF("\n<WPA-Enterprise Security>\n");

	/* INPUT AUTH */
	do {
		PRINTF("\nAuthentication Type(802.1x) ?\n");
		PRINTF("\t%2d. PEAP or TTLS or FAST (Recommend)\n", E_EAP_AUTH_MODE_PEAP_TTLS_FAST);
		PRINTF("\t%2d. PEAP\n", E_EAP_AUTH_MODE_PEAP);
		PRINTF("\t%2d. TTLS\n", E_EAP_AUTH_MODE_TTLS);
		PRINTF("\t%2d. FAST\n", E_EAP_AUTH_MODE_FAST);
		PRINTF("\t%2d. TLS \n", E_EAP_AUTH_MODE_TLS);

		PRINTF(ANSI_REVERSE " Type ?" ANSI_NORMAL " [%d/", E_EAP_AUTH_MODE_PEAP_TTLS_FAST);
		PRINTF("%d/", E_EAP_AUTH_MODE_PEAP);
		PRINTF("%d/", E_EAP_AUTH_MODE_TTLS);
		PRINTF("%d/", E_EAP_AUTH_MODE_FAST);
		PRINTF("%d/", E_EAP_AUTH_MODE_TLS);
		PRINTF(ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

		*p_eap_auth_mode = input_num = getNum();

		if (input_num == RET_DEFAULT) {	/* deafult  */
			*p_eap_auth_mode = E_EAP_AUTH_MODE_PEAP_TTLS_FAST;
			break;
		} else if (input_num == RET_QUIT) {
			return E_QUIT;
		}
	} while (input_num <= E_EAP_AUTH_MODE_NONE || input_num >= E_EAP_AUTH_MODE_MAX);

	if (E_EAP_AUTH_MODE_FAST > input_num)
	{

		/* INPUT Phase2 */
		do {
			PRINTF("\n\nAuthentication Protocols (802.1x) ?\n");
			PRINTF("\t%2d. MSCHAPv2 or GTC (Recommend)\n", E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC);
			PRINTF("\t%2d. MSCHAPv2\n", E_EAP_PHASE2_MODE_MSCHAPV2);
			PRINTF("\t%2d. GTC\n", E_EAP_PHASE2_MODE_GTC);

			PRINTF(ANSI_REVERSE " Protocol ?" ANSI_NORMAL " [%d/", E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC);
			PRINTF("%d/", E_EAP_PHASE2_MODE_MSCHAPV2);
			PRINTF("%d/", E_EAP_PHASE2_MODE_GTC);

			PRINTF(ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

			*p_phase2 = input_num = getNum();

			if (input_num == RET_DEFAULT) {	/* deafult */
				*p_phase2 = E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC;
				break;
			} else if (input_num == RET_QUIT) {
				return E_QUIT;
			}
		} while (input_num < E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC || input_num >= E_EAP_PHASE2_MODE_MAX);
	}

	PRINTF("\n");

	return E_CONTINUE;
}

unsigned char setup_input_enterp_id_cert(unsigned char auth_mode, unsigned char eap_auth_type, char *p_id, char *p_password)
{
	int getStr_len = 0;

	if (auth_mode != E_AUTH_MODE_ENT
#ifdef __SUPPORT_WPA3_ENTERPRISE__
		&& auth_mode != E_AUTH_MODE_WPA3_ENT
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
		&& auth_mode != E_AUTH_MODE_WPA3_ENT_192B
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
#endif // __SUPPORT_WPA3_ENTERPRISE__
	) {
		return E_CONTINUE;
	}

	/* Enterprise ID */
	do {
		PRINTF("\n" ANSI_REVERSE " ID ?" ANSI_NORMAL " [" ANSI_BOLD "Q"
				   ANSI_NORMAL "uit] : ");

		getStr_len = getStr(p_id, 64);  /* Max ASCII 64 */

		if ((getStr_len == 1 && toupper(p_id[0]) == 'Q') || getStr_len == RET_QUIT) {
			return E_QUIT;
		}

		if (getStr_len == 1) {
			p_id[0] = toupper(p_id[0]);
		}

		if (getStr_len > 0) {
			break;
		}
	} while (1);

	switch (eap_auth_type)
	{
		case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
		case E_EAP_AUTH_MODE_PEAP:
		case E_EAP_AUTH_MODE_TTLS:
		case E_EAP_AUTH_MODE_FAST:
		{
			/* Enterprise Password */
			do {
				PRINTF("\n" ANSI_REVERSE " Password ?" ANSI_NORMAL " [" ANSI_BOLD "Q"
						   ANSI_NORMAL "uit] : ");

				getStr_len = getStr(p_password, 64);  /* Max ASCII 64 */

				if ((getStr_len == 1 && toupper(p_password[0]) == 'Q') || getStr_len == RET_QUIT) {
					return E_QUIT;
				}

				if (getStr_len == 1) {
					p_password[0] = toupper(p_password[0]);
				}

				if (getStr_len > 0) {
					break;
				}
			} while (1);

			// Delete Enterprise Certificate
			cert_rwds(ACT_DELETE, CLIENT_CERT3);
			cert_rwds(ACT_DELETE, CLIENT_KEY3);
			break;
		}

		case E_EAP_AUTH_MODE_TLS:
		{
			char input_str[3];

			PRINTF(ANSI_REVERSE "\n<Requires a certificate when using EAP-TLS>\n" ANSI_NORMAL);

			do {
				PRINTF("\n" ANSI_REVERSE "Do you want to save the certificate ?" ANSI_NORMAL
					   " [" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q"
					   ANSI_NORMAL "uit] (Default Yes) : ");

				memset(input_str, 0, 3);
				getStr_len = getStr(input_str, 2);
				input_str[0] = toupper(input_str[0]);

				if (getStr_len <= 1) {
					if (input_str[0] == 'Y' || getStr_len == 0) {
						PRINTF(ANSI_REVERSE "\n\n<Certificate>\n\n" ANSI_NORMAL);
						cert_rwds(ACT_WRITE, CLIENT_CERT3);

						PRINTF(ANSI_REVERSE "\n\n<Private Key>\n\n" ANSI_NORMAL);
						cert_rwds(ACT_WRITE, CLIENT_KEY3);

						break;
					} else if (input_str[0] == 'N') {
						break;
					} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
						return E_QUIT;
					}
				}
			} while (1);

			PRINTF("\n\n");
			break;
		}

	}

	return E_CONTINUE;
}

#endif // __SUPPORT_WPA_ENTERPRISE__


unsigned char setup_input_psk(unsigned char auth_mode, unsigned char *p_psk_key_type, char *p_psk_key)
{
	int getStr_len = 0;

	if (   auth_mode == E_AUTH_MODE_NONE
		|| auth_mode == E_AUTH_MODE_WEP
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
		|| auth_mode == E_AUTH_MODE_OWE
#endif /* __SUPPORT_WPA3_OWE__ */
#ifdef __SUPPORT_WPA_ENTERPRISE__
		|| auth_mode >= E_AUTH_MODE_ENT
#endif // __SUPPORT_WPA_ENTERPRISE__
		)
	{
		return E_CONTINUE;
	}

	/* PSK-KEY or SAE-PASSWORD */
	do {
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
		if (auth_mode == E_AUTH_MODE_SAE) {
			PRINTF("\n" ANSI_REVERSE " SAE-PASSWORD(ASCII) ?" ANSI_NORMAL " [" ANSI_BOLD "Q"
				   ANSI_NORMAL "uit]\n");
		} else if (auth_mode == E_AUTH_MODE_RSN_SAE) {
			PRINTF("\n" ANSI_REVERSE " PSK-KEY(ASCII characters 8~63) ?" ANSI_NORMAL " [" ANSI_BOLD
				   "Q" ANSI_NORMAL "uit]\n");
		} else
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
		{
			PRINTF("\n" ANSI_REVERSE " PSK-KEY(ASCII characters 8~63 or Hexadecimal characters 64) ?"
				   ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL "uit]\n");
		}

		PRINTF("[1234567" ANSI_BOLD "8" ANSI_NORMAL
			   "9|123456789|123456789|123456789|123456789|123456789|12" ANSI_BOLD "34" ANSI_NORMAL
			   "]\n:");

		getStr_len = getStr(p_psk_key, 64);  /* Max ASCII 63, Hexa 64 */

		if (getStr_len == 64) {
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
			if (auth_mode == E_AUTH_MODE_SAE) {
				*p_psk_key_type = E_PSK_KEY_ASCII;
			} else {
				*p_psk_key_type = E_PSK_KEY_HEXA;
			}
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
		} else {
			*p_psk_key_type = E_PSK_KEY_ASCII;
		}

		if ((getStr_len == 1 && toupper(p_psk_key[0]) == 'Q') || getStr_len == RET_QUIT) {
			return E_QUIT;
		}

		/* Check length */
		if ((
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
			auth_mode != E_AUTH_MODE_SAE && 
#endif // __SUPPORT_WPA3_SAE__
				getStr_len < 8)
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
			|| (auth_mode == E_AUTH_MODE_SAE && getStr_len < 1)
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__
		)
		{
			continue;
		}

		/* Check Hexa */
		if (*p_psk_key_type == E_PSK_KEY_HEXA) {
			int keyindex = 0;

			for (keyindex = 0; keyindex < getStr_len; keyindex++) {
				if (isxdigit((int)(p_psk_key[keyindex])) == 0) {
					break;
				}
			}

			if (keyindex < getStr_len) {
				continue;
			}
		}

		if (getStr_len == 1) {
			p_psk_key[0] = (char)toupper(p_psk_key[0]);
		}
		break;
	} while (1);

	return E_CONTINUE;
}


unsigned char setup_input_wep(unsigned char auth_mode, unsigned char cur_iface, unsigned char *p_wep_key_type,
									unsigned char *p_wep_bit, unsigned char *p_wep_key_idx, char *p_wep_key)
{
	int input_num = 0;

	if (auth_mode != E_AUTH_MODE_WEP || cur_iface != WLAN0_IFACE) /* WEP */ {
		return E_CONTINUE;
	}

	/* WEP-KEY */
	do {
		PRINTF("\n" ANSI_REVERSE
			   " WEP KEY (ASCII characters 5 or 13, Hexadecimal characters 10 or 26) ?" ANSI_NORMAL " ["
			   ANSI_BOLD "Q" ANSI_NORMAL "uit]\n");

		PRINTF("[1234" ANSI_BOLD "5" ANSI_NORMAL "6789" ANSI_BOLD "|" ANSI_NORMAL "12" ANSI_BOLD
			   "3" ANSI_NORMAL "456789|12345" ANSI_BOLD "6" ANSI_NORMAL "]\n:");

		input_num = getStr(p_wep_key, 26);

		if (input_num == 5 || input_num == 13) {			/* ASCII */
			*p_wep_key_type = WEP_KEY_TYPE_ASCII;
		} else if (input_num == 10 || input_num == 26) {	/* HEXA */
			*p_wep_key_type = WEP_KEY_TYPE_HEXA;
		} else if (input_num > 1) {
			continue;
		}

		if (input_num == 5 || input_num == 10) {			/* 64bit */
			*p_wep_bit = WEP_KEY_64BIT;
		} else if (input_num == 13 || input_num == 26) {    /* 128bit */
			*p_wep_bit = WEP_KEY_128BIT;
		}

		if ((input_num == 1 && toupper(p_wep_key[0]) == 'Q') || input_num == RET_QUIT) {
			return E_QUIT;
		}

		/* Check Hexa */
		if (*p_wep_key_type == WEP_KEY_TYPE_HEXA) {

			int keyindex = 0;

			for (keyindex = 0; keyindex < input_num; keyindex++) {
				if (isxdigit((int)(p_wep_key[keyindex])) == 0) {
					break;
				}
			}

			if (keyindex < input_num) {
				continue;
			}
		}

		break;
	} while (1);

	/* KEY INDEX */
	do {
		PRINTF("\nWEP KEY INDEX ?\n" \
			   ANSI_REVERSE " INDEX ?" ANSI_NORMAL " [%d/%d/%d/%d/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ",
			   E_WEP_KEY_IDX_1,
			   E_WEP_KEY_IDX_2,
			   E_WEP_KEY_IDX_3,
			   E_WEP_KEY_IDX_4);

		input_num = getNum();
		*p_wep_key_idx = (unsigned char)input_num;

		if (input_num == RET_QUIT) {
			return E_QUIT;
		}
	} while (input_num < E_WEP_KEY_IDX_1 || input_num > E_WEP_KEY_IDX_4);

	return E_CONTINUE;
}


#if defined ( __SUPPORT_P2P__ )
unsigned char setup_p2p_config(unsigned char sysmode,
								unsigned char cur_iface,
								unsigned char *p_oper_chan,
								unsigned char *p_listen_chan,
								unsigned char *p_go_intent)
{
	int input = 0;
	
	if (   sysmode == E_SYSMODE_P2P
		|| sysmode == E_SYSMODE_P2P_GO
		|| (cur_iface == WLAN1_IFACE && sysmode == E_SYSMODE_STA_P2P)) {

		if (sysmode != E_SYSMODE_STA_P2P) {

			/* Operation Channel */
			do {
				PRINTF("\n" ANSI_REVERSE " OPERATION CHANNEL ?" ANSI_NORMAL " [1/6/11/" ANSI_BOLD "Q"
					   ANSI_NORMAL "uit] (Default : Auto) : ");

				input = getNum();
				
				if (input == RET_QUIT) {
					return E_QUIT;
				} else if (input == RET_DEFAULT) {
					*p_oper_chan = WIFI_DIR_OPERATION_CH_DFLT;
					break;
				} else {
					*p_oper_chan = input;
				}
			} while (input != CHANNEL_AUTO && input != 1 && input != 6 && input != 11);

			if (sysmode == E_SYSMODE_P2P_GO) {
				return E_SETUP_WIFI_END;
			}

			do {
				/* Listen Channel */
				PRINTF("\n" ANSI_REVERSE " LISTEN CHANNEL ?" ANSI_NORMAL " [1/6/11/" ANSI_BOLD "Q"
					   ANSI_NORMAL "uit] (Default : Auto) : ");

				input = getNum();

				if (input == RET_QUIT) {
					return E_QUIT;
				} else if (input == RET_DEFAULT) {
					*p_listen_chan = WIFI_DIR_LISTEN_CH_DFLT;
					break;
				} else {
					*p_listen_chan = input;
				}
			} while (input != CHANNEL_AUTO && input != 1 && input != 6 && input != 11);
		}

		do {
			/* GO Intent */
			PRINTF("\n" ANSI_REVERSE " GO INTENT ?" ANSI_NORMAL " [0~15/" ANSI_BOLD "Q" ANSI_NORMAL
				   "uit] (Default : 3) : ");

			input = getNum();

			if (input == RET_QUIT) {
				return E_QUIT;
			} else if (input == RET_DEFAULT) {
				*p_go_intent = WIFI_DIR_GO_INTENT_DFLT;
				break;
			} else {
				*p_go_intent = input;
			}
		} while (input < 0 || input  > 15);

		return E_SETUP_WIFI_END;
	}

	return E_CONTINUE;
}
#endif	// __SUPPORT_P2P__


unsigned char setup_adv(unsigned char encryp_mode, unsigned char *p_wifi_mode)
{
	int		getStr_len = 0;
	char	input_str[3];

	do {
		PRINTF("\n" ANSI_REVERSE " Do you want to set advanced WiFi configuration ?" ANSI_NORMAL
			   " [" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD "Q"
			   ANSI_NORMAL "uit] (Default No) : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1) {
			if (input_str[0] == 'Y') {
				break;
			} else if (input_str[0] == 'N' || getStr_len == 0) {
				if (encryp_mode == E_ENCRYP_MODE_TKIP) {
					*p_wifi_mode = E_WIFI_MODE_BG;
				}

				return E_SETUP_WIFI_END;
			} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
				return E_QUIT;
			}
		}
	} while (1);

	return E_CONTINUE;
}


#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || (defined __SUPPORT_MESH__)
unsigned char setup_input_sae_group(char *p_sae_groups)
{
	/* key_mgmt SAE & unset groups */

#define MAX_SAE_GROUPS 3 // 6
#define DEAFULT_SAE_GROUPS "19" // "19 20"

	int getStr_len = 0;
	const char *sae_groups_str = DEAFULT_SAE_GROUPS;
	int group_id = 0;
	char params[20];

SAE_GROUPS:

	group_id = 0;
	memset(params, 0, 20);

	do {
		PRINTF("\n" ANSI_REVERSE "SAE_Groups ? [19/20](Default %s)" ANSI_NORMAL " : ",
			   sae_groups_str);

		memset(p_sae_groups, 0x0, 20);
		memset(params, 0x0, 20);

		getStr_len = getStr(p_sae_groups, 20);

		if (getStr_len == 0) {
			strcpy (p_sae_groups, sae_groups_str); /* default sae_groups */
		} else if (getStr_len == RET_QUIT) {
			return E_QUIT;
		}

		strcpy(params, p_sae_groups);

		for (size_t idx = 0; idx < strlen(params); idx++) {
			if (!(isdigit((int)(params[idx])) != 0 || isblank((int)(params[idx])) != 0)) {
				/* check : num, space or tab */
				goto SAE_GROUPS;
			}
		}

		group_id = atoi(strtok(params, " "));

		if (group_id != 0) {
			if (check_sae_groupid(group_id) == 0) {
				goto SAE_GROUPS;
			}

			for (int i = 1; i <= MAX_SAE_GROUPS; i++) {
				group_id = atoi(strtok(NULL, " "));

				if (group_id == 0) {
					break;
				}

				if (check_sae_groupid(group_id) == 0) {
					goto SAE_GROUPS;
				}
			}
		} else {
			goto SAE_GROUPS;
		}

		break;
	} while (1);

	return E_CONTINUE;
}
#endif //  __SUPPORT_WPA3_SAE__) || ( __SUPPORT_MESH__)


unsigned char setup_pmf(unsigned char auth_mode, unsigned char encryp_mode, unsigned char *p_pmf)
{
#if defined __SUPPORT_IEEE80211W__
	if (encryp_mode == E_ENCRYP_MODE_TKIP
		|| encryp_mode == E_ENCRYP_MODE_TKIP_CCMP
#if defined  __SUPPORT_WPA3_PERSONAL__
		|| auth_mode == E_AUTH_MODE_SAE
		|| auth_mode == E_AUTH_MODE_RSN_SAE
#ifdef __SUPPORT_WPA3_OWE__
		|| auth_mode == E_AUTH_MODE_OWE
#endif /* __SUPPORT_WPA3_OWE__ */
#endif // __SUPPORT_WPA3_PERSONAL__
#if defined __SUPPORT_WPA_ENTERPRISE__
		|| auth_mode == E_AUTH_MODE_ENT
#if defined __SUPPORT_WPA3_ENTERPRISE__
		|| auth_mode == E_AUTH_MODE_WPA2_WPA3_ENT
		|| auth_mode == E_AUTH_MODE_WPA3_ENT
#if defined __SUPPORT_WPA3_ENTERPRISE_192B__
		|| auth_mode == E_AUTH_MODE_WPA3_ENT_192B
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
#endif // __SUPPORT_WPA3_ENTERPRISE__
#endif // __SUPPORT_WPA_ENTERPRISE__
		) {
		return E_CONTINUE;
	}

	/* Check 11N & (WPA2) RSN */
	if (auth_mode == E_AUTH_MODE_RSN || auth_mode == E_AUTH_MODE_WPA_RSN)
	{
		int input_num;

		do {
			PRINTF("\nProtected Management Frame(PMF, MFP, IEEE80211W) ?\n" \
				   ANSI_REVERSE " Mode ?" ANSI_NORMAL " [0:Disable/1:Optional/%s" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ",
				   auth_mode == E_AUTH_MODE_RSN ? "2:Mantatory/":"");

			*p_pmf = input_num = getNum();

			if (input_num == RET_QUIT) {
				return E_QUIT;
			}

		} while (input_num < E_PMF_NONE
				|| (input_num > E_PMF_MANDATORY && auth_mode == E_AUTH_MODE_RSN && encryp_mode == E_ENCRYP_MODE_CCMP)
				|| (input_num > E_PMF_OPTIONAL && encryp_mode == E_ENCRYP_MODE_TKIP_CCMP));
	}
#else
	DA16X_UNUSED_ARG(auth_mode);
	DA16X_UNUSED_ARG(encryp_mode);
	DA16X_UNUSED_ARG(p_pmf);
#endif // __SUPPORT_IEEE80211W__

	return E_CONTINUE;
}


unsigned char setup_wifimode(unsigned char cur_iface, unsigned char e_auth_mode,
								   unsigned char encryp_mode, unsigned char *p_wifi_mode)
{
	DA16X_UNUSED_ARG(e_auth_mode);

	int input_num;

	if (cur_iface != WLAN1_IFACE
#if defined __SUPPORT_WPA3__ && defined __SUPPORT_WPA3_PERSONAL__
		|| e_auth_mode == E_AUTH_MODE_SAE
		|| e_auth_mode == E_AUTH_MODE_RSN_SAE
#endif // __SUPPORT_WPA3__
	) {
		return E_CONTINUE;
	}
	
	/* SoftAP */
	/* WiFi Mode */
	do {
		PRINTF("\nWiFi MODE ?\n" \
			   "\t%d. %s%s\n"	/* 11bgn */
			   "\t%d. %s%s\n"	/* 11gn */
			   "\t%d. %s\n" 	/* 11bg */
			   "\t%d. %s%s\n"	/* 11n */
			   "\t%d. %s\n" 	/* 11g */
			   "\t%d. %s\n" 	/* 11b */
			   ANSI_REVERSE " MODE ?" ANSI_NORMAL " [%d~%d/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ",
			   E_WIFI_MODE_BGN, wifi_mode_str[E_WIFI_MODE_BGN - 1],
			   cur_iface == WLAN1_IFACE && encryp_mode == E_ENCRYP_MODE_TKIP ? ANSI_COLOR_RED "(Unsupported)"
			   ANSI_NORMAL : "",
			   E_WIFI_MODE_GN,	 wifi_mode_str[E_WIFI_MODE_GN - 1],
			   cur_iface == WLAN1_IFACE && encryp_mode == E_ENCRYP_MODE_TKIP ? ANSI_COLOR_RED "(Unsupported)"
			   ANSI_NORMAL : "",
			   E_WIFI_MODE_BG,  wifi_mode_str[E_WIFI_MODE_BG - 1],
			   E_WIFI_MODE_N,   wifi_mode_str[E_WIFI_MODE_N - 1],
			   cur_iface == WLAN1_IFACE && encryp_mode == E_ENCRYP_MODE_TKIP ? ANSI_COLOR_RED "(Unsupported)"
			   ANSI_NORMAL : "",
			   E_WIFI_MODE_G,   wifi_mode_str[E_WIFI_MODE_G - 1],
			   E_WIFI_MODE_B,   wifi_mode_str[E_WIFI_MODE_B - 1],
			   E_WIFI_MODE_BGN,
			   E_WIFI_MODE_B);

		input_num = getNum();
		*p_wifi_mode = (unsigned char)input_num;

		if (input_num == RET_QUIT) {
			return E_QUIT;
		} else if (input_num == RET_DEFAULT) {
			*p_wifi_mode = E_WIFI_MODE_BGN;
		}
	} while (
		   (encryp_mode == E_ENCRYP_MODE_TKIP && input_num != E_WIFI_MODE_BG && input_num != E_WIFI_MODE_G && input_num != E_WIFI_MODE_B)
		|| (encryp_mode != E_ENCRYP_MODE_TKIP && (input_num < E_WIFI_MODE_BGN || input_num > E_WIFI_MODE_B))
	);

	return E_CONTINUE;
}


unsigned char setup_connect_hidden_ssid(unsigned char sysmode, unsigned char cur_iface, unsigned char *p_connect_hidden_ssid)
{
	int 	getStr_len = 0;
	char	input_str[3];

	if (   sysmode == E_SYSMODE_STA
		|| (   cur_iface == WLAN0_IFACE
			&& (   sysmode == E_SYSMODE_STA_AP
#if defined ( CONFIG_P2P )
				|| sysmode == E_SYSMODE_STA_P2P
#endif	// CONFIG_P2P
			   )
		   ))
	{	
		do {
			PRINTF("\n" ANSI_REVERSE " Connect to hidden SSID ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
				   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1) {
				if (input_str[0] == 'Y') {
					*p_connect_hidden_ssid = 1;
					break;
				} else if (input_str[0] == 'N') {
					*p_connect_hidden_ssid = 0;
					break;
				} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
					return E_QUIT;
				}
			}
		} while (1);
	}

	return E_CONTINUE;
}

#ifdef UNUSED
unsigned char setup_fixed_connection_channel(unsigned char sysmode, unsigned char cur_iface,
														char *p_country, int *p_assoc_ch,
														int select_ssid, int channel_selected)
{
	// For fast_connection with sleep 1/2 mode
	if (fast_conn_sleep_12_flag == pdTRUE) {
		int 	getStr_len = 0;
		int 	firstCH, finshCH;
		char	input_str[3];

		if (   sysmode == E_SYSMODE_STA
			|| (   cur_iface == WLAN0_IFACE
				&& (   sysmode == E_SYSMODE_STA_AP
#if defined ( CONFIG_P2P )
					|| sysmode == E_SYSMODE_STA_P2P
#endif	// CONFIG_P2P
				   )
			   )
		   )
		{
			do {
				PRINTF("\n" ANSI_REVERSE " Do you want to configure the association channel ?" ANSI_NORMAL
					   " [" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q"
					   ANSI_NORMAL "uit] : ");
				memset(input_str, 0, 3);
				getStr_len = getStr(input_str, 2);
				input_str[0] = (char)toupper(input_str[0]);
	
				if (getStr_len <= 1) {
					if (input_str[0] == 'Y') {
						if (select_ssid == RET_MANUAL) {
							country_channel_range (p_country, &firstCH, &finshCH);
	
/* INPUT_ASSOC_CHANNEL */
INPUT_ASSOC_CHANNEL:
	
							do {
								PRINTF("\n" ANSI_REVERSE " Assoc. channel ?" ANSI_NORMAL " [%d~%d, Auto:0/" ANSI_BOLD "Q"
									   ANSI_NORMAL "uit] (Default Auto) : ", firstCH, finshCH);
	
								*p_assoc_ch = getNum();
	
								if (*p_assoc_ch == RET_DEFAULT) {	/* deafult  */
									*p_assoc_ch = CHANNEL_DEFAULT; /* AUTO */
								} else if (*p_assoc_ch == RET_QUIT) {
									return E_QUIT;
								} else if ((*p_assoc_ch < 0) && (*p_assoc_ch != RET_QUIT)) {
									continue;
								}
	
								if (*p_assoc_ch < CHANNEL_AUTO) {
									*p_assoc_ch = CHANNEL_DEFAULT; //set default 0(Auto)
								} else {
									*p_assoc_ch = chk_channel_by_country(p_country, *p_assoc_ch, 0, NULL);
	
									if (*p_assoc_ch < CHANNEL_AUTO) {
										goto INPUT_ASSOC_CHANNEL;
									}
								}
	
								break;
							} while (1);

						} else {
							*p_assoc_ch = i3ed11_freq_to_ch(channel_selected);
						}
	
						break;
					} else if (input_str[0] == 'N') {
						*p_assoc_ch = 0;
						break;
					} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
						return E_QUIT;
					}
				}
			} while (1);
		}
	}

	return E_CONTINUE;
}
#endif // UNUSED


void setup_display_wlan_preset(unsigned char sysmode,
								unsigned char cur_iface,
								char *p_ssid,
								char channel,
								char listen_chan,
								char go_intent,
								unsigned char auth_mode,
								unsigned char encryp_mode,
#ifdef __SUPPORT_WPA_ENTERPRISE__
								unsigned char eap_auth,
								unsigned char eap_phase2_auth,
								char *eap_id,
								char *eap_pw,
#endif // __SUPPORT_WPA_ENTERPRISE__
								char *e_psk_key, int psk_key_type,
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
								char *e_sae_groups,
#endif // __SUPPORT_WPA3_SAE__ || defined __SUPPORT_MESH__
								char *p_wep_key,
								unsigned char wep_bit,
								unsigned char wep_key_idx,
								unsigned char wep_key_type,
								unsigned char assoc_ch,
								unsigned char wifi_mode,
								unsigned char connect_hidden_ssid
#if defined __SUPPORT_IEEE80211W__
						  ,
								unsigned char pmf
#endif // __SUPPORT_IEEE80211W__
						)
{
	DA16X_UNUSED_ARG(listen_chan);
	DA16X_UNUSED_ARG(go_intent);

	char	display_str[32];

	memset(display_str, 0, 32);

	VT_COLORGREEN;
	/* PRINT CONFIGURATION */
	/*============================================\n*/
	print_separate_bar('=', 44, 1);

#if defined ( __SUPPORT_P2P__ )
	// WiFi Direct
	if (   sysmode == E_SYSMODE_P2P
		|| sysmode == E_SYSMODE_P2P_GO
		|| (sysmode == E_SYSMODE_STA_P2P && cur_iface == WLAN1_IFACE)) {

		PRINTF("[ WiFi Direct ]\n");

		if (sysmode != E_SYSMODE_STA_P2P) {
			PRINTF("Operation Ch. : ");

			if (channel == CHANNEL_AUTO) {
				PRINTF("Auto\n");
			} else {
				PRINTF("%2d\n",  channel);
			}

			if (sysmode == E_SYSMODE_P2P) {
				PRINTF("Listen Ch.    : ");
				if (listen_chan == CHANNEL_AUTO) {
					PRINTF("Auto\n");
				} else {
					PRINTF("%2d\n",  listen_chan);
				}
			}
		}

		if (sysmode != E_SYSMODE_P2P_GO) {
			PRINTF("GO Intent     : %2d\n", go_intent);
		}

		goto DISPLAY_END;
	}
#endif	// __SUPPORT_P2P__

	PRINTF("SSID        : %s\n", p_ssid);

	if (sysmode == E_SYSMODE_AP) {
		if (channel == CHANNEL_AUTO) {
			PRINTF("CHANNEL     : AUTO(ACS)\n");
		} else {
			PRINTF("CHANNEL     : %d\n", channel);
		}
	}

	if (assoc_ch > 0) {
		PRINTF("ASSOC. CH.  : %d\n", assoc_ch);
	}

	memset(display_str, 0, 32);

#ifdef __SUPPORT_WPA_ENTERPRISE__
	switch (eap_auth)
	{
		case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
			sprintf(display_str, "PEAP or TTLS or FAST");
 			break;

		case E_EAP_AUTH_MODE_PEAP:
			sprintf(display_str, "PEAP");
			break;

		case E_EAP_AUTH_MODE_TTLS:
			sprintf(display_str, "TTLS");
			break;
 
		case E_EAP_AUTH_MODE_FAST:
			sprintf(display_str, "FAST");
			break;
 
		case E_EAP_AUTH_MODE_TLS:
			sprintf(display_str, "TLS");
			break;		

		case E_EAP_AUTH_MODE_NONE:
		default:
			break;
	}

#endif // __SUPPORT_WPA_ENTERPRISE__

	switch (auth_mode)
	{
#ifdef __SUPPORT_WPA_ENTERPRISE__
#ifdef __SUPPORT_WPA3_ENTERPRISE__
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
		case E_AUTH_MODE_WPA3_ENT_192B:
			sprintf(display_str, "WPA3-Enterprise 192Bits");
			break;
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__

		case E_AUTH_MODE_WPA3_ENT:
			sprintf(display_str, "WPA3-Enterprise(128Bits)");
			break;

		case E_AUTH_MODE_WPA2_WPA3_ENT:
			sprintf(display_str, "WPA2/3-Enterprise(128Bits)");
			break;
#endif // __SUPPORT_WPA3_ENTERPRISE__

		case E_AUTH_MODE_ENT:
			sprintf(display_str, "WPA1/2-Enterprise");
			break;
#endif // __SUPPORT_WPA_ENTERPRISE__

		case E_AUTH_MODE_NONE:
			sprintf(display_str, "OPEN");
			break;

		case E_AUTH_MODE_WEP:
			sprintf(display_str, "WEP");
			break;

		case E_AUTH_MODE_WPA:
			sprintf(display_str, "WPA-PSK");
			break;

		case E_AUTH_MODE_RSN:
			sprintf(display_str, "WPA2-PSK");
			break;

		case E_AUTH_MODE_WPA_RSN:
#if !defined __SUPPORT_WPA3_PERSONAL__ && !defined __SUPPORT_WPA3_SAE__ && !defined __SUPPORT_WPA3_OWE__ && !defined __SUPPORT_MESH__
		default:
#endif /* !defined __SUPPORT_WPA3_SAE__ && !defined __SUPPORT_WPA3_OWE__*/
			sprintf(display_str, "WPA/WAP2-PSK");
			break;

#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
		case E_AUTH_MODE_OWE:
			sprintf(display_str, "OWE");
			break;
#endif /* __SUPPORT_WPA3_OWE__ */

#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
		case E_AUTH_MODE_SAE:
			sprintf(display_str, "SAE");
			break;

		case E_AUTH_MODE_RSN_SAE:
		default:
			sprintf(display_str, "WPA2-PSK+SAE");
			break;
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
	}

	PRINTF("AUTH        : %s\n", display_str);

#ifdef __SUPPORT_WPA_ENTERPRISE__
	if (E_AUTH_MODE_ENT <= auth_mode)
	{
		memset(display_str, 0, 32);

		switch (eap_auth)
		{
			case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
 				sprintf(display_str, "PEAP or TTLS or FAST");
 				break;

			case E_EAP_AUTH_MODE_PEAP:
				sprintf(display_str, "PEAP");
				break;

			case E_EAP_AUTH_MODE_TTLS:
				sprintf(display_str, "TTLS");
				break;

			case E_EAP_AUTH_MODE_FAST:
				sprintf(display_str, "FAST");
				break;

			case E_EAP_AUTH_MODE_TLS:
				sprintf(display_str, "TLS");
				break;		

			case E_EAP_AUTH_MODE_NONE:
			default:
				break;
		}

		PRINTF("Auth. Type  : %s\n", display_str);

		memset(display_str, 0, 32);

		if (eap_auth != E_EAP_AUTH_MODE_TLS) {
			if (eap_auth == E_EAP_AUTH_MODE_PEAP_TTLS_FAST
			|| eap_auth == E_EAP_AUTH_MODE_PEAP
			|| eap_auth == E_EAP_AUTH_MODE_TTLS
 			|| eap_auth != E_EAP_AUTH_MODE_FAST
			) {
				switch (eap_phase2_auth)
				{
					case E_EAP_PHASE2_MODE_MSCHAPV2:
						sprintf(display_str, "MSCHAPV2");
						break;

					case E_EAP_PHASE2_MODE_GTC:
						sprintf(display_str, "GTC");
						break;

					case E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC:
					default:
						sprintf(display_str, "MSCHAPV2 or GTC");
						break;
				}

				PRINTF("Auth. Proto : %s\n", display_str);
			}
		}

		PRINTF("ID          : %s\n", eap_id);

		if (eap_auth == E_EAP_AUTH_MODE_TLS) {
			char *buffer = NULL;
#if defined (XIP_CACHE_BOOT)
			buffer = os_calloc(CERT_MAX_LENGTH, sizeof(char));
#else
			buffer = calloc(CERT_MAX_LENGTH, sizeof(char));
#endif
			if (buffer == NULL) {
				PRINTF("[%s] Failed to allocate memory to read certificate\n",
					   __func__);
				return ;
			}

			PRINTF("Certificate : %s\n", cert_rwds(ACT_STATUS, CLIENT_CERT3) == pdTRUE ? "Found" : "Empty");
			PRINTF("Private Key : %s\n", cert_rwds(ACT_STATUS, CLIENT_KEY3) == pdTRUE ? "Found" : "Empty");
			vPortFree(buffer);
			buffer = NULL;
		} else {
			PRINTF("Password    : %s\n", eap_pw);		
		}
	} else
#endif // __SUPPORT_WPA_ENTERPRISE__

	if (auth_mode >= E_AUTH_MODE_WPA
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
			&& auth_mode != E_AUTH_MODE_OWE
#endif /* __SUPPORT_WPA3_OWE__ */
			&& auth_mode < E_AUTH_MODE_MAX
	   ) {

		memset(display_str, 0, 32);

		switch (encryp_mode)
		{
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
			case E_ENCRYP_MODE_GCMP_256:
				sprintf(display_str, "GCMP_256");
				break;
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__

			case E_ENCRYP_MODE_TKIP:
				sprintf(display_str, "TKIP");
				break;

			case E_ENCRYP_MODE_CCMP:
				sprintf(display_str, "AES(CCMP)");
				break;

			case E_ENCRYP_MODE_TKIP_CCMP:
				sprintf(display_str, "TKIP/AES(CCMP)");
				break;

			default:
				sprintf(display_str, "Unknown(%d)", encryp_mode);
				break;
		}

		PRINTF("ENCRYPTION  : %s\n", display_str);

#ifdef __SUPPORT_WPA_ENTERPRISE__
		if (auth_mode < E_AUTH_MODE_ENT)
#endif // __SUPPORT_WPA_ENTERPRISE__
		{
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__

			if (auth_mode == E_AUTH_MODE_RSN_SAE) {
				PRINTF("PSK/SAE KEY : %s\n", e_psk_key);
			} else if (auth_mode == E_AUTH_MODE_SAE) {
				PRINTF("SAE KEY     : %s\n", e_psk_key);
			} else
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
			{
				PRINTF("PSK KEY     : %s\n", e_psk_key);
			}

#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__

			if (strlen(e_sae_groups) > 1) {
				PRINTF("SAE_Groups  : %s\n", e_sae_groups);
			}

			if (auth_mode != E_AUTH_MODE_SAE
					&& auth_mode != E_AUTH_MODE_RSN_SAE)
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */
			{
				PRINTF("KEY TYPE    : %s\n", psk_key_type == E_PSK_KEY_ASCII ? "ASCII" : "HEXA");
			}
		}

	} else if (auth_mode == E_AUTH_MODE_WEP) {
		PRINTF("WEP KEY     : %s\n", p_wep_key);
		PRINTF("KEY BIT     : %d\n", wep_bit == WEP_KEY_64BIT ? 64 : 128);
		PRINTF("KEY INDEX   : %d\n", wep_key_idx);
		PRINTF("KEY TYPE    : %s\n", wep_key_type == WEP_KEY_TYPE_ASCII ? "ASCII" : "HEXA");
	}

#ifdef __SUPPORT_IEEE80211W__
	if (auth_mode >= E_AUTH_MODE_RSN
#ifdef __SUPPORT_WPA_ENTERPRISE__
		&& auth_mode < E_AUTH_MODE_ENT
#endif // __SUPPORT_WPA_ENTERPRISE__
		) {
		PRINTF("PMF MODE    : ");

		switch (pmf)
		{
			case E_PMF_NONE:
				PRINTF("Disable\n");
				break;

			case E_PMF_OPTIONAL:
				PRINTF("Optional\n");
				break;

			case E_PMF_MANDATORY:
				PRINTF("Mandatory\n");
				break;
		}
	}
#endif /* __SUPPORT_IEEE80211W__ */

	if ((sysmode == E_SYSMODE_STA_AP || sysmode == E_SYSMODE_AP) && cur_iface == WLAN1_IFACE) {
		PRINTF("WIFI MODE   : %s\n", channel == 14 ? wifi_mode_str[(int)E_WIFI_MODE_B - 1] :
			   wifi_mode_str[wifi_mode - 1]);
	}

	if (cur_iface == WLAN0_IFACE) {	/* Station only */
		PRINTF("Hidden AP   : %sonnect\n", connect_hidden_ssid == 0 ? "Not c" : "C");
	}

#if defined ( CONFIG_P2P )
DISPLAY_END:
#endif	// CONFIG_P2P

	/*============================================\n*/
	print_separate_bar('=', 44, 1);
	VT_NORMAL;
}


unsigned char setup_wifi_config_confirm(unsigned char sysmode, unsigned char cur_iface)
{
	DA16X_UNUSED_ARG(sysmode);
	DA16X_UNUSED_ARG(cur_iface);

	int getStr_len = 0;
	char input_str[3];

	do {
		PRINTF(ANSI_REVERSE " WIFI CONFIGURATION CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
			   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1) {
			if (input_str[0] == 'Y') {
				break;
			} else if (input_str[0] == 'N') {
				return E_WLAN_SETUP;
			} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
				return E_QUIT;
			}
		}
	} while (1);

#if defined ( __SUPPORT_P2P__ )
	if (sysmode == E_SYSMODE_P2P || sysmode == E_SYSMODE_P2P_GO) {
		return E_SETUP_APPLY_START;
	} else if (sysmode == E_SYSMODE_STA_P2P && cur_iface == WLAN1_IFACE) {
		return E_SETUP_APPLY_START;
	} else
#endif	// __SUPPORT_P2P__
	{
		return E_CONTINUE;
	}
}


static unsigned char setup_netmode(unsigned char sysmode, unsigned char cur_iface, unsigned char *p_netmode)
{
	int		getStr_len = 0;
	char	input_str[3];
	

	if (    sysmode == E_SYSMODE_STA
		|| (   cur_iface == WLAN0_IFACE
			&& (   sysmode == E_SYSMODE_STA_AP
#if defined ( CONFIG_P2P )
				|| sysmode == E_SYSMODE_STA_P2P
#endif	// CONFIG_P2P
			   )
		   )
	   )
	{
		/* Station mode || concurrent Station */
		/* NETMODE */
		do {
			PRINTF(ANSI_REVERSE " IP Connection Type ?" ANSI_NORMAL " [" ANSI_BOLD "A" ANSI_NORMAL
				   "utomatic IP/" ANSI_BOLD "S" ANSI_NORMAL "tatic IP/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);

			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1) {
				if (input_str[0] == 'A' || getStr_len == 0) {
					*p_netmode = E_NETMODE_DYNAMIC_IP;
					break;
				} else if (input_str[0] == 'S') {
					*p_netmode = E_NETMODE_STATIC_IP;
					break;
				} else if (input_str[0] == 'Q' || getStr_len == RET_QUIT) {
					return E_QUIT;
				}
			}
		} while (1);

		VT_COLORGREEN;
		PRINTF("\nIP Connection Type: %s" ANSI_NORMAL "\n\n",
			   *p_netmode == E_NETMODE_DYNAMIC_IP ? "Automatic IP" : "Static IP");
	}
	else
	{
		*p_netmode = E_NETMODE_STATIC_IP; /* Static IP */
	}

	return E_CONTINUE;
}


unsigned char setup_nat(unsigned char sysmode, unsigned char cur_iface, unsigned char *p_nat_flag)
{
#ifdef __SUPPORT_NAT__
	int		getStr_len = 0;
	char	input_str[3];

	/* NAT */
	if (sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE)
	{
		{
			do
			{
				PRINTF(ANSI_REVERSE " NAT Eanble ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL "es/"
					   ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
				memset(input_str, 0, 3);
				getStr_len = getStr(input_str, 2);
			
				input_str[0] = toupper(input_str[0]);
			
				if (getStr_len <= 1)
				{
					if (input_str[0] == 'Y' || getStr_len == 0)
					{
						*p_nat_flag = E_ENABLE;
						break;
					}
					else if (input_str[0] == 'N')
					{
						*p_nat_flag = E_DISABLE;
						break;
					}
					else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
					{
						return E_QUIT;
					}
				}
			}
			while (1);
			
			VT_COLORGREEN;
			PRINTF("\nNAT: %s" ANSI_NORMAL "\n\n",
				   *p_nat_flag == E_ENABLE ? "Enable" : "Disable"); 
		}
	}
#else
	DA16X_UNUSED_ARG(sysmode);
	DA16X_UNUSED_ARG(cur_iface);
	DA16X_UNUSED_ARG(p_nat_flag);
#endif /* __SUPPORT_NAT__ */

	return E_CONTINUE;
}


static unsigned char setup_static_ip(unsigned char cur_iface, char *p_ipaddress)
{
	int getStr_len = 0;
	
	do
	{
		PRINTF("\n" ANSI_REVERSE " IP ADDRESS ?" ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL
			   "uit] (Default %s) : ",
			   cur_iface == WLAN0_IFACE ? DEFAULT_IPADDR_WLAN0 : DEFAULT_IPADDR_WLAN1);

		getStr_len = getStr(p_ipaddress, 15);

		p_ipaddress[0] = (char)toupper(p_ipaddress[0]);

		if (   (getStr_len == 1 && p_ipaddress[0] == 'Q')
			|| getStr_len == RET_QUIT)
		{
			return E_QUIT;
		}

		if (getStr_len == 0)
		{
			strcpy(p_ipaddress,
				   cur_iface == WLAN0_IFACE ? DEFAULT_IPADDR_WLAN0 : DEFAULT_IPADDR_WLAN1);
		}
	} while (!is_in_valid_ip_class(p_ipaddress));

	return E_CONTINUE;
}


static unsigned char setup_static_subnet(unsigned char cur_iface, char *p_subnetmask)
{
	int getStr_len = 0;

	do
	{
		PRINTF("\n" ANSI_REVERSE " SUBNET ?" ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL
			   "uit] (Default %s) : ",
			   cur_iface == WLAN0_IFACE ? DEFAULT_SUBNET_WLAN0 : DEFAULT_SUBNET_WLAN1);

		getStr_len = getStr(p_subnetmask, 15);

		p_subnetmask[0] = (char)toupper(p_subnetmask[0]);

		if (  (getStr_len == 1 && p_subnetmask[0] == 'Q')
			|| getStr_len == RET_QUIT)
		{
			return E_QUIT;
		}

		if (getStr_len == 0)
		{
			strcpy(p_subnetmask,
				   cur_iface == WLAN0_IFACE ? DEFAULT_SUBNET_WLAN0 : DEFAULT_SUBNET_WLAN1);
		}
	} while (!isvalidmask(p_subnetmask));

	return E_CONTINUE;
}


static unsigned char setup_static_gateway(unsigned char sysmode,
										unsigned char cur_iface,
										char *p_ipaddress,
										char *p_subnetmask,
										char *p_gateway)
{
	int getStr_len = 0;

#if defined ( __SUPPORT_P2P__ )
	if (
#if !defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
		   sysmode != E_SYSMODE_AP && 
#endif	// !__SUPPORT_SOFTAP_DFLT_GW__
		sysmode != E_SYSMODE_P2P_GO
	   )
#else
	DA16X_UNUSED_ARG(sysmode);

	if (
#if !defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
		   sysmode != E_SYSMODE_AP && 
#endif	// !__SUPPORT_SOFTAP_DFLT_GW__
		pdTRUE
	   )
#endif	// __SUPPORT_P2P__
	{
		/* Gateway */
		do
		{
			PRINTF("\n" ANSI_REVERSE " GATEWAY ?" ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL
				   "uit] (Default %s) : ",
				   cur_iface == WLAN0_IFACE ? DEFAULT_GATEWAY_WLAN0 : DEFAULT_GATEWAY_WLAN1);

			getStr_len = getStr(p_gateway, 15);
			p_gateway[0] = (char)toupper(p_gateway[0]);

			if ((getStr_len == 1 && p_gateway[0] == 'Q') || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}

			if (getStr_len == 0)
			{
				strcpy(p_gateway,
					   cur_iface == WLAN0_IFACE ? DEFAULT_GATEWAY_WLAN0 : DEFAULT_GATEWAY_WLAN1);
			}
		}
		while (    (!is_in_valid_ip_class(p_gateway) && strcmp(p_gateway, "0.0.0.0") != 0)
				|| (cur_iface == WLAN0_IFACE && strcmp(p_ipaddress, p_gateway) == 0)
				|| (   !isvalidIPsubnetRange(iptolong(p_gateway), \
										  iptolong(p_ipaddress), \
										  iptolong(p_subnetmask))
					&& strcmp(p_gateway, "0.0.0.0") != 0))
				;
	}

	return E_CONTINUE;
}


static unsigned char setup_static_dns(unsigned char sysmode, unsigned char cur_iface, char *p_dns)
{
	int getStr_len = 0;

	if (   sysmode != E_SYSMODE_AP
#if defined ( __SUPPORT_P2P__ )
		&& sysmode != E_SYSMODE_P2P_GO
#endif	// __SUPPORT_P2P__
        && !(sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE)
	   )
	{
		/* DNS */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DNS ?" ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL
				   "uit] (Default %s) : ",
				   cur_iface == WLAN0_IFACE ? DEFAULT_DNS_WLAN0 : DEFAULT_DNS_WLAN1);

			getStr_len = getStr(p_dns, 15);
			p_dns[0] = (char)toupper(p_dns[0]);

			if ((getStr_len == 1 && p_dns[0] == 'Q') || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}

			if (getStr_len == 0)
			{
				strcpy(p_dns,
					   cur_iface == WLAN0_IFACE ? DEFAULT_DNS_WLAN0 : DEFAULT_DNS_WLAN1);
			}
		}
		while (!is_in_valid_ip_class(p_dns) && strcmp(p_dns, "0.0.0.0") != 0);
	}

	return E_CONTINUE;
}


static void setup_display_network_preset(unsigned char cur_iface,
									unsigned char sysmode,
									char *p_ipaddress,
									char *p_subnetmask,
									char *p_gateway,
									char *p_dns)

{
	VT_COLORGREEN;
	/*============================================\n*/
	print_separate_bar('=', 44, 1);

	PRINTF("[WLAN%d]\n", cur_iface);
	PRINTF("IP ADDRESS: %s\n", p_ipaddress);
	PRINTF("SUBNET    : %s\n", p_subnetmask);

#if defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
	if (sysmode == E_SYSMODE_AP || 
       (sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE))
	{
		PRINTF("GATEWAY   : %s\n", p_gateway);
	}
	else
#endif	// __SUPPORT_SOFTAP_DFLT_GW__
	if (
#if !defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
		sysmode != E_SYSMODE_AP &&
#endif	// !__SUPPORT_SOFTAP_DFLT_GW__
#if defined ( __SUPPORT_P2P__ )
		sysmode != E_SYSMODE_P2P_GO
#else
		pdTRUE
#endif	// __SUPPORT_P2P__
	   )
	{
		PRINTF("GATEWAY   : %s\n", p_gateway);
		PRINTF("DNS       : %s\n", p_dns);
	}

	/*============================================\n*/
	print_separate_bar('=', 44, 1);

	VT_NORMAL;
}


unsigned char setup_check_network(unsigned char cur_iface,
									unsigned char sysmode,
									char *p_ipaddress,
									char *p_subnetmask,
									char *p_gateway,
									char *p_dns)
{
	DA16X_UNUSED_ARG(p_dns);
#if !defined ( __SUPPORT_P2P__ )
	DA16X_UNUSED_ARG(sysmode);
#endif	// __SUPPORT_P2P__

	if (
#if !defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
		   sysmode != E_SYSMODE_AP &&
#endif	// !__SUPPORT_SOFTAP_DFLT_GW__
#if defined ( __SUPPORT_P2P__ )
		sysmode != E_SYSMODE_P2P_GO
#else
		pdTRUE
#endif	// __SUPPORT_P2P__
	   )
	{
		/* Check Validity */
		if (   (is_in_valid_ip_class(p_gateway) == pdFALSE && strcmp(p_gateway, "0.0.0.0") != 0) /* check gateway */
			|| (cur_iface == WLAN0_IFACE && strcmp(p_ipaddress, p_gateway) == 0))
		{
			VT_COLORRED;
			PRINTF("\nInvalid GATEWAY Address !!!\n");
			VT_NORMAL;

			//goto NETWORK_SETUP;
			return E_NETWORK_SETUP;
			
		}
		else if (   isvalidmask(p_subnetmask) == pdFALSE
				 || is_in_valid_ip_class(p_ipaddress) == pdFALSE
				 || (   isvalidIPsubnetRange(iptolong(p_ipaddress),
											iptolong(p_gateway),
											iptolong(p_subnetmask)) == pdFALSE
					 && strcmp(p_gateway, "0.0.0.0") != 0))
		{
			VT_COLORRED;
			PRINTF("\nInvalid IP Address !!!\n");
			VT_NORMAL;

			//goto INPUT_IPADDRESS;
			return E_INPUT_IPADDRESS;
		}
	}
	return E_CONTINUE;
}


unsigned char setup_network_config_confirm(unsigned char sysmode)
{
	int getStr_len = 0;
	char input_str[3];

	do
	{
		PRINTF(ANSI_REVERSE " IP CONFIGURATION CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
			   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'Y')
			{
				break;
			}
			else if (input_str[0] == 'N')
			{
				if (sysmode == E_SYSMODE_AP)   /* AP */
				{
					return E_INPUT_IPADDRESS;
				}
				else
				{
					return  E_NETWORK_SETUP;
				}
			}
			else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
		}
	}
	while (1);

	return E_CONTINUE;
}


/* ?????? */
#define NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    3600*36	/*131072*/

unsigned char setup_sntp_client(unsigned char *p_sntp_client, int *p_sntp_client_period_time,
				char *p_sntp_gmt_timezone, int *p_sntp_timezone_int,
				char *p_sntp_svr_addr, char *p_sntp_svr_addr1,
				char *p_sntp_svr_addr2)
{
	int	sntp_svr_index = 0;
	int 	getStr_len = 0;
	char	input_str[3];


	/* Station mode || concurrent Station */
	/* NETMODE */
SNTP_CLIENT_START:

	if (sntp_support_flag == pdFALSE)
	{
		return E_CONTINUE;
	}

	do
	{
		PRINTF(ANSI_REVERSE " SNTP Client enable ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL
			   "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);

		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'Y' || getStr_len == 0)
			{
				*p_sntp_client = E_SNTP_CLIENT_START;
				break;
			}
			else if (input_str[0] == 'N')
			{
				*p_sntp_client = E_SNTP_CLIENT_STOP;
				//goto SNTP_CLIENT_END;
				return E_CONTINUE;
			}
			else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
		}
	}
	while (1);

	/* Set SNTP period */
	do
	{
		PRINTF("\n" ANSI_REVERSE " SNTP Period time (%d ~ %d hours) ? (default : %d hours)" \
			   ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL "uit] ",
			   1, NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600,
			   NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600);
		*p_sntp_client_period_time = getNum();

		if (*p_sntp_client_period_time  == RET_QUIT)
		{
			return E_QUIT;
		}
		else if (*p_sntp_client_period_time  == RET_DEFAULT)
		{
			*p_sntp_client_period_time = 3600 * (NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600);
			break;
		}
		else
		{
			*p_sntp_client_period_time *= 3600;
		}
	}
	while (*p_sntp_client_period_time < 3600
			|| *p_sntp_client_period_time > NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL);

	/* Set GMT Timezone */
	do
	{
		PRINTF("\n" ANSI_REVERSE
			   " GMT Timezone +xx:xx|-xx:xx (-12:00 ~ +12:00) ? (default : 00:00)" \
			   ANSI_NORMAL " [" ANSI_BOLD "Q" ANSI_NORMAL "uit] ");
		memset(p_sntp_gmt_timezone, 0, 8);
		getStr_len = getStr(p_sntp_gmt_timezone, 8);

		if ((getStr_len == 1 && toupper(p_sntp_gmt_timezone[0]) == 'Q')
				|| getStr_len == RET_QUIT)
		{
			return E_QUIT;
		}
		else if (getStr_len == 0 || strncmp(p_sntp_gmt_timezone, "00:00", 5) == 0)
		{
			strcpy(p_sntp_gmt_timezone, "00:00");
			*p_sntp_timezone_int = 0;
			break;
		}
		else if (getStr_len < 0)
		{
			continue;
		}
		else if (getStr_len == 6 && p_sntp_gmt_timezone[3] == ':')
		{
			char timezone_temp[8] = {0, 0};
			int timezone_offset[2] = {0, 0};

			strcpy(timezone_temp, p_sntp_gmt_timezone);

			timezone_offset[0] = atoi(strtok(timezone_temp, ":"));
			timezone_offset[1] = atoi(strtok(NULL, "\0"));

			if (timezone_offset[0] == 0 && timezone_offset[1] == 0)
			{
				*p_sntp_timezone_int = 0;
			}
			else if (timezone_offset[0] > 12
					 || timezone_offset[0] < -12
					 || timezone_offset[1] >= 60
					 || (timezone_offset[0] == 0 && p_sntp_gmt_timezone[1] != '0'))
			{
				continue;
			}
			else
			{
				*p_sntp_timezone_int = ((3600 * (long)timezone_offset[0])
									   + ((timezone_offset[0] < 0) ? (-1 * (long)timezone_offset[1] * 60) : ((
												   long)timezone_offset[1] * 60)));
			}

			break;
		}
	}
	while (1);

	/* Set SNTP Server address */
	do
	{
		if (sntp_svr_index == 0)
		{
			PRINTF("\n" ANSI_REVERSE " SNTP Server %d addr ? (default : %s)" ANSI_NORMAL " [" ANSI_BOLD
			   "Q" ANSI_NORMAL "uit]\n", sntp_svr_index, DFLT_SNTP_SERVER_DOMAIN);
			PRINTF("Input : ");
			memset(p_sntp_svr_addr, 0, 256);
			getStr_len = getStr(p_sntp_svr_addr, 256);
			if ((getStr_len == 1 && toupper(p_sntp_svr_addr[0]) == 'Q') || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
			else if (getStr_len == 0)
			{
				strcpy(p_sntp_svr_addr, DFLT_SNTP_SERVER_DOMAIN);
			}
			else if (isvalid_domain(p_sntp_svr_addr) == 0)
			{
				continue;
			}
		}
		else if (sntp_svr_index == 1)
		{
			PRINTF("\n" ANSI_REVERSE " SNTP Server %d addr ? (default : %s)" ANSI_NORMAL " [" ANSI_BOLD
			   "Q" ANSI_NORMAL "uit]\n", sntp_svr_index, DFLT_SNTP_SERVER_DOMAIN_1);
			PRINTF("Input : ");
			memset(p_sntp_svr_addr1, 0, 256);
			getStr_len = getStr(p_sntp_svr_addr1, 256);
			if ((getStr_len == 1 && toupper(p_sntp_svr_addr1[0]) == 'Q') || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
			else if (getStr_len == 0)
			{
				strcpy(p_sntp_svr_addr1, DFLT_SNTP_SERVER_DOMAIN_1);
			}
			else if (isvalid_domain(p_sntp_svr_addr1) == 0)
			{
				continue;
			}
		}
		else if (sntp_svr_index == 2)
		{
			PRINTF("\n" ANSI_REVERSE " SNTP Server %d addr ? (default : %s)" ANSI_NORMAL " [" ANSI_BOLD
			   "Q" ANSI_NORMAL "uit]\n", sntp_svr_index, DFLT_SNTP_SERVER_DOMAIN_2);
			PRINTF("Input : ");
			memset(p_sntp_svr_addr2, 0, 256);
			getStr_len = getStr(p_sntp_svr_addr2, 256);
			if ((getStr_len == 1 && toupper(p_sntp_svr_addr2[0]) == 'Q') || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
			else if (getStr_len == 0)
			{
				strcpy(p_sntp_svr_addr2, DFLT_SNTP_SERVER_DOMAIN_2);
			}
			else if (isvalid_domain(p_sntp_svr_addr2) == 0)
			{
				continue;
			}
		}
		else
		{
			sntp_svr_index = 0;
			break;
		}

		if (sntp_svr_index > 2) {
			sntp_svr_index = 0;
			break;
		}
		sntp_svr_index++;
	}
	while (1);

	VT_COLORGREEN;
	/*============================================\n*/
	print_separate_bar('=', 44, 1);

	PRINTF("SNTP Client      : ");
	if (*p_sntp_client == E_SNTP_CLIENT_START)
	{
		PRINTF("Enable\n");
		PRINTF("SNTP Period time : %d hours\n", *p_sntp_client_period_time / 3600);
		PRINTF("SNTP GMT Timezone: %s\n", p_sntp_gmt_timezone);
		PRINTF("SNTP Server addr : %s\n", p_sntp_svr_addr);
		PRINTF("SNTP Server addr1: %s\n", p_sntp_svr_addr1);
		PRINTF("SNTP Server addr2: %s\n", p_sntp_svr_addr2);
	}
	else
	{
		PRINTF("Disable\n");
	}

	/*============================================\n*/
	print_separate_bar('=', 44, 1);
	VT_NORMAL;

	do
	{
		PRINTF(ANSI_REVERSE " SNTP Client CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD "Y" ANSI_NORMAL
			   "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'Y')
			{
				break;
			}
			else if (input_str[0] == 'N')
			{
				goto SNTP_CLIENT_START;
			}
			else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
		}
	}
	while (1);

	return E_CONTINUE;
}


unsigned char setup_dpm(unsigned char sysmode, unsigned char cur_iface, unsigned char *p_dpm_mode,
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
						unsigned char *p_dpm_Dynamic_Period_Set,
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
						int *p_dpm_KeepAlive_time, int *p_dpm_User_Wakeup_time,
						int *p_dpm_TIM_wakeup_count
						)
{
	int		getStr_len = 0;
	char	input_str[3];

	/* DPM [enable/disable] */
	if ((cur_iface == WLAN0_IFACE) && sysmode == E_SYSMODE_STA)
	{
INPUT_DPM_MODE:

		do
		{
			PRINTF(ANSI_REVERSE " Dialog DPM (Dynamic Power Management) ?" ANSI_NORMAL " [" ANSI_BOLD
				   "Y" ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'Y')
				{
					*p_dpm_mode = 1;
					break;
				}
				else if (input_str[0] == 'N')
				{
					*p_dpm_mode = 0;
					break;
				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
			}
		}
		while (1);
	}
	else
	{
		return E_CONTINUE;
	}	

	if (*p_dpm_mode == 1)
	{
		/* Detail Config */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DPM factors : Defaults ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
				   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			*p_dpm_KeepAlive_time = DFLT_DPM_KEEPALIVE_TIME;
			*p_dpm_User_Wakeup_time = DFLT_DPM_USER_WAKEUP_TIME;
			*p_dpm_TIM_wakeup_count = DFLT_DPM_TIM_WAKEUP_COUNT;

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'Y')
				{
					goto DEFULT_DPM_VALUE;
				}
				else if (input_str[0] == 'N')
				{
					break;
				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
			}
		}
		while (1);

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
		/* DPM Dynamic Period Set */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DDPS Enable ?" ANSI_NORMAL " [" ANSI_BOLD "N"
				   ANSI_NORMAL "o/" ANSI_BOLD "Y" ANSI_NORMAL "es/" ANSI_BOLD "Q" ANSI_NORMAL "uit](Default: No) : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'N')
				{
					*p_dpm_Dynamic_Period_Set = 0;
					break;
				}
				else if (input_str[0] == 'Y')
				{
					*p_dpm_Dynamic_Period_Set = 1;
					break;
				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
				else
				{
					*p_dpm_Dynamic_Period_Set = DFLT_DPM_DYNAMIC_PERIOD_SET;
					break;
				}
			}
		}
		while (1);
#endif /* __SUPPORT_DPM_DYNAMIC_PERIOD_SET__ */

		/* DPM Keep Alive Time */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DPM Keep Alive Time(%d~%d ms) ?" ANSI_NORMAL " [" ANSI_BOLD "Q"
				   ANSI_NORMAL "uit] (Default %d ms) : ",
				   MIN_DPM_KEEPALIVE_TIME, MAX_DPM_KEEPALIVE_TIME, DFLT_DPM_KEEPALIVE_TIME);

			*p_dpm_KeepAlive_time = getNum();

			if (*p_dpm_KeepAlive_time  == RET_QUIT)
			{
				return E_QUIT;
			}
			else if (*p_dpm_KeepAlive_time  == RET_DEFAULT)
			{
				*p_dpm_KeepAlive_time = DFLT_DPM_KEEPALIVE_TIME;
			}
		}
		while (*p_dpm_KeepAlive_time < MIN_DPM_KEEPALIVE_TIME
				|| *p_dpm_KeepAlive_time > MAX_DPM_KEEPALIVE_TIME);

		/* DPM User Wakeup Time */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DPM User Wakeup Time(%d~%d ms) ?" ANSI_NORMAL " [" ANSI_BOLD
				   "Q" ANSI_NORMAL "uit] (Default %d ms) : ",
				   MIN_DPM_USER_WAKEUP_TIME, MAX_DPM_USER_WAKEUP_TIME, DFLT_DPM_USER_WAKEUP_TIME);

			*p_dpm_User_Wakeup_time = getNum();

			if (*p_dpm_User_Wakeup_time  == RET_QUIT)
			{
				return E_QUIT;
			}
			else if (*p_dpm_User_Wakeup_time  == RET_DEFAULT)
			{
				*p_dpm_User_Wakeup_time = DFLT_DPM_USER_WAKEUP_TIME;
			}
		}
		while (*p_dpm_User_Wakeup_time < MIN_DPM_USER_WAKEUP_TIME
				|| *p_dpm_User_Wakeup_time > MAX_DPM_USER_WAKEUP_TIME);
		{
			/* DPM TIM Wakeup Time */
			do
			{
				PRINTF("\n" ANSI_REVERSE " DPM TIM Wakeup Count(%d~%d dtim) ?" ANSI_NORMAL " [" ANSI_BOLD
					   "Q" ANSI_NORMAL "uit] (Default %d) : ",
					   MIN_DPM_TIM_WAKEUP_COUNT,
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
					   (*p_dpm_Dynamic_Period_Set ? MAX_DPM_TIM_WAKEUP_COUNT_DDPS : MAX_DPM_TIM_WAKEUP_COUNT),
#else
						MAX_DPM_TIM_WAKEUP_COUNT,
#endif /* __SUPPORT_DPM_DYNAMIC_PERIOD_SET__ */
					   DFLT_DPM_TIM_WAKEUP_COUNT);

				*p_dpm_TIM_wakeup_count = getNum();

				if (*p_dpm_TIM_wakeup_count  == RET_QUIT)
				{
					return E_QUIT;
				}
				else if (*p_dpm_TIM_wakeup_count  == RET_DEFAULT)
				{
					*p_dpm_TIM_wakeup_count = DFLT_DPM_TIM_WAKEUP_COUNT;
				}
			}
			while (*p_dpm_TIM_wakeup_count < MIN_DPM_TIM_WAKEUP_COUNT
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
					|| *p_dpm_TIM_wakeup_count > (*p_dpm_Dynamic_Period_Set ? MAX_DPM_TIM_WAKEUP_COUNT_DDPS : MAX_DPM_TIM_WAKEUP_COUNT)
#else
					|| *p_dpm_TIM_wakeup_count > MAX_DPM_TIM_WAKEUP_COUNT
#endif /* __SUPPORT_DPM_DYNAMIC_PERIOD_SET__ */
			);
		}

DEFULT_DPM_VALUE:

		VT_COLORGREEN;
		/*============================================\n*/
		print_separate_bar('=', 44, 1);

		PRINTF("DPM MODE           : Enable\n");
		
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
		PRINTF("Dynamic Period Set : %s\n", *p_dpm_Dynamic_Period_Set == 1 ? "Enable" : "Disable");
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__

		PRINTF("Keep Alive Time    : %d ms\n", *p_dpm_KeepAlive_time);
		PRINTF("User Wakeup Time   : %d ms\n", *p_dpm_User_Wakeup_time);

		if (dpm_wu_tm_msec_flag == pdTRUE)
		{
			PRINTF("TIM Wakeup Time    : %d ms (%d dtim)\n", *p_dpm_TIM_wakeup_count,
				   (int)coilToInt(*p_dpm_TIM_wakeup_count / 102.4));
		}
		else
		{
			PRINTF("TIM Wakeup Count   : %d dtim\n", *p_dpm_TIM_wakeup_count);
		}

		/*============================================\n*/
		print_separate_bar('=', 44, 1);
		VT_NORMAL;

		do
		{
			PRINTF(ANSI_REVERSE " DPM CONFIGURATION CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
				   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'Y')
				{
					break;
				}
				else if (input_str[0] == 'N')
				{
					goto INPUT_DPM_MODE;

				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
			}
		}
		while (1);
	}
	return E_CONTINUE;
}


unsigned char setup_fast_connection(unsigned char sysmode, unsigned char dpm_mode, unsigned char *p_mode, int *p_assoc_ch, int channel_selected)
{
	int getStr_len = 0;
	char input_str[3];

	if (sysmode != E_SYSMODE_STA || dpm_mode)
	{
		*p_mode = pdFALSE;
		return E_CONTINUE;
	}

	do
	{
		PRINTF(ANSI_REVERSE " Fast Connection Sleep 2 Mode ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
			   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

		memset(input_str, 0, 3);
		getStr_len = getStr(input_str, 2);
		input_str[0] = (char)toupper(input_str[0]);

		if (getStr_len <= 1)
		{
			if (input_str[0] == 'Y')
			{
				*p_mode = pdTRUE;
				if (*p_assoc_ch == 0)
				{
					*p_assoc_ch = i3ed11_freq_to_ch(channel_selected);
				}
				break;
			}
			else if (input_str[0] == 'N')
			{
				*p_mode = pdFALSE;
				break;
			}
			else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
			{
				return E_QUIT;
			}
		}
	}
	while (1);

	return E_CONTINUE;
}



unsigned char setup_dhcp_server(unsigned char sysmode, unsigned char cur_iface, unsigned char netmode,
								char *p_ipaddress, char *p_subnetmask, char *p_gateway, char *p_dns,
								unsigned char *p_use_dhcps,
								char *p_dhcp_lease_start, char *p_dhcp_lease_end,
								int *p_dhcp_lease_time, int *p_dhcp_lease_count)
{
	DA16X_UNUSED_ARG(sysmode);
	DA16X_UNUSED_ARG(cur_iface);
	DA16X_UNUSED_ARG(netmode);
	DA16X_UNUSED_ARG(p_ipaddress);
	DA16X_UNUSED_ARG(p_subnetmask);
	DA16X_UNUSED_ARG(p_gateway);
	DA16X_UNUSED_ARG(p_dns);

	int		getStr_len = 0;
	char	input_str[3];

	/* Setup DHCP Server */
	if ((sysmode == E_SYSMODE_AP
			|| sysmode == E_SYSMODE_STA_AP
		) && cur_iface == WLAN1_IFACE)
	{
		/* AP Mode */

		do
		{
			PRINTF("\n" ANSI_REVERSE " DHCP SERVER CONFIGURATION ?" ANSI_NORMAL " [" ANSI_BOLD "Y"
				   ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");
			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'Y' || getStr_len == 0)
				{
					*p_use_dhcps = 1;
					break;
				}
				else if (input_str[0] == 'N')
				{
					return E_CONTINUE;
				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
			}
		}
		while (1);

		/* Lease ip count */
INPUT_LEASE_IP_COUNT:

		do
		{
			PRINTF("\n" ANSI_REVERSE " DHCP SERVER LEASE IP Count(MAX %d) ?" ANSI_NORMAL " ["
				   ANSI_BOLD "Q" ANSI_NORMAL "uit] (Default %d) : ",
				   MAX_DHCP_LEASE_COUNT, DEFAULT_DHCP_LEASE_COUNT);
			*p_dhcp_lease_count = getNum();

			if (*p_dhcp_lease_count == RET_QUIT)
			{
				return E_QUIT;
			}

			if (*p_dhcp_lease_count == RET_DEFAULT)
			{
				*p_dhcp_lease_count = DEFAULT_DHCP_LEASE_COUNT;
			}
			else if (*p_dhcp_lease_count < 1 || *p_dhcp_lease_count > MAX_DHCP_LEASE_COUNT)
			{
				continue;
			}

			/*
				How to specify a default IP range :
				1. Assign as many as specified from interface IP +1 IP.
				2. If the specified number exceeds ServerNet, allocate from the ServerNet starting IP.
				3. Do not include the gateway IP in the specified range..
			*/
			if (    isvalidIPsubnetRange((iptolong(p_ipaddress) + *p_dhcp_lease_count),
											iptolong(p_ipaddress),
											iptolong(p_subnetmask))
				&& (   (!isvalidIPrange(iptolong(p_gateway),
										 iptolong(p_ipaddress) + 1,
										 iptolong(p_ipaddress) + *p_dhcp_lease_count))
					|| (   sysmode != E_SYSMODE_AP
#if defined ( __SUPPORT_P2P__ )
						&& sysmode != E_SYSMODE_P2P_GO
#endif	// __SUPPORT_P2P__
					   )
				   )
			   )
			{
				/* Lease First ip */
				longtoip((iptolong(p_ipaddress) + 1), p_dhcp_lease_start);
				/* Lease Last ip */
				longtoip((iptolong(p_ipaddress) + *p_dhcp_lease_count), p_dhcp_lease_end);
			}
			else
			{
				long firstIP;

				firstIP = subnetRangeFirstIP(iptolong(p_ipaddress),
											 iptolong(p_subnetmask));

				if (firstIP == iptolong(
							p_gateway))   /* If firstIP and Gateway IP are the same.*/
				{
					firstIP++;
				}

				if (isvalidIPrange(iptolong(p_ipaddress), firstIP,
								   firstIP + (*p_dhcp_lease_count - 1))
						|| isvalidIPrange(iptolong(p_gateway), firstIP,
										  firstIP + (*p_dhcp_lease_count - 1)))
				{
					VT_COLORRED;
					PRINTF("\nERR : DHCP Server Lease Range\n");
					VT_NORMAL;
					continue;
				}

				/* Lease First IP */
				longtoip(firstIP, p_dhcp_lease_start);
				/* Lease List IP */
				longtoip(firstIP + (*p_dhcp_lease_count - 1), p_dhcp_lease_end);
			}

			break;
		}
		while (1);

		/* Lease Time */
		do
		{
			PRINTF("\n" ANSI_REVERSE " DHCP SERVER LEASE TIME(%d ~ %d SEC) ?" ANSI_NORMAL " ["
				   ANSI_BOLD "Q" ANSI_NORMAL "uit] (Default %d) : ",
				   MIN_DHCP_SERVER_LEASE_TIME,
				   MAX_DHCP_SERVER_LEASE_TIME,
				   DEFAULT_DHCP_SERVER_LEASE_TIME);

			*p_dhcp_lease_time = getNum();

			if (*p_dhcp_lease_time == RET_QUIT)
			{
				return E_QUIT;
			}

			if (*p_dhcp_lease_time == RET_DEFAULT)
			{
				*p_dhcp_lease_time = DEFAULT_DHCP_SERVER_LEASE_TIME;
			}
		}
		while (*p_dhcp_lease_time < MIN_DHCP_SERVER_LEASE_TIME
				|| *p_dhcp_lease_time > MAX_DHCP_SERVER_LEASE_TIME);

		VT_COLORGREEN;
		/*============================================\n*/
		print_separate_bar('=', 44, 1);
		PRINTF("[DHCP SERVER]\n");
		PRINTF("Start IP  : %s\n", p_dhcp_lease_start);
		PRINTF("END IP    : %s\n", p_dhcp_lease_end);
#if defined __SUPPORT_NAT__
		if (sysmode == E_SYSMODE_STA_AP
		{
			PRINTF("DNS       : %s\n", p_dns);
		}
#endif // __SUPPORT_NAT__
		PRINTF("LEASE TIME: %d\n", *p_dhcp_lease_time);
		/*============================================\n*/
		print_separate_bar('=', 44, 1);
		VT_NORMAL;

		do
		{
			PRINTF("\n" ANSI_REVERSE " DHCP SERVER CONFIGURATION CONFIRM ?" ANSI_NORMAL " [" ANSI_BOLD
				   "Y" ANSI_NORMAL "es/" ANSI_BOLD "N" ANSI_NORMAL "o/" ANSI_BOLD "Q" ANSI_NORMAL "uit] : ");

			memset(input_str, 0, 3);
			getStr_len = getStr(input_str, 2);
			input_str[0] = (char)toupper(input_str[0]);

			if (getStr_len <= 1)
			{
				if (input_str[0] == 'Y')
				{
					break;
				}
				else if (input_str[0] == 'N')
				{
					goto INPUT_LEASE_IP_COUNT;
				}
				else if (input_str[0] == 'Q' || getStr_len == RET_QUIT)
				{
					return E_QUIT;
				}
			}
		}
		while (1);
	}
	return E_CONTINUE;
}


unsigned char setup_apply_preset(char *p_country, char *p_cmdline)
{
	char *reply = NULL;
	int ret = 0;

	reply = pvPortMalloc(64);

	if (reply == NULL)
	{
		PRINTF("Malloc Error - reply\n");
		return E_ERROR;
	}
	
	memset(reply, 0, 64);

	/* suplicant configuration flush */
	ret = da16x_cli_reply("flush", NULL, reply); /* cli flush*/

	if (ret != 0 || !strcmp(reply, REPLY_FAIL))
	{
		vPortFree(reply);
		return E_ERROR;
	}

	/* supplicant log off */
	da16x_cli_reply("set_log all 0", NULL, reply);

	/* country config */
	sprintf(p_cmdline, "country %s", p_country);
	da16x_cli_reply(p_cmdline, NULL, reply);

	vPortFree(reply);
	return E_CONTINUE;
}


#if defined ( __SUPPORT_P2P__ )
unsigned char setup_apply_p2p(unsigned char sysmode,
								unsigned char cur_iface,
								char* cmdline,
								unsigned char oper_chan,
								unsigned char listen_chan,
								unsigned char go_intent)
{
	char	*reply = NULL;

	if (   cur_iface == WLAN1_IFACE
		&& (   sysmode == E_SYSMODE_P2P
			|| sysmode == E_SYSMODE_P2P_GO
			|| sysmode == E_SYSMODE_STA_P2P)
	   )
	{
		reply = pvPortMalloc(64);

		if (reply == NULL)
		{
			PRINTF("Malloc Error - reply\n");
			return E_ERROR;
		}

		if (sysmode != E_SYSMODE_STA_P2P)
		{
			factory_reset(0);

			sprintf(cmdline, "p2p_set oper_channel %d", oper_chan);
			da16x_cli_reply(cmdline, NULL, reply);

			if (sysmode == E_SYSMODE_P2P_GO)
			{
				vPortFree(reply);
				return E_CONTINUE;
			}

			sprintf(cmdline, "p2p_set listen_channel %d", listen_chan);
			da16x_cli_reply(cmdline, NULL, reply);
		}

		sprintf(cmdline, "p2p_set go_intent %d", go_intent);
		da16x_cli_reply(cmdline, NULL, reply);

		vPortFree(reply);
	}

	return E_CONTINUE;
}
#endif	// __SUPPORT_P2P__


unsigned char setup_apply_wlan(unsigned char sysmode, unsigned char cur_iface, unsigned char iface_start, unsigned char iface_end,
									char *p_cmdline,
									char *p_ssid, unsigned char auth_mode, unsigned char encryp_mode,
#ifdef __SUPPORT_WPA_ENTERPRISE__
									unsigned char eap_auth,
									unsigned char eap_phase2_auth,
									char *eap_id,
									char *eap_pw,
#endif // __SUPPORT_WPA_ENTERPRISE__
									unsigned char psk_key_type, char *p_psk_key,
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
									char *p_sae_groups,
#endif // __SUPPORT_WPA3_SAE__									
									char *p_wep_key, int wep_key_idx,
									unsigned char channel,
									unsigned char connect_hidden_ssid,
									unsigned char wifi_mode
#if defined __SUPPORT_IEEE80211W__
									,
									unsigned char pmf
#endif // __SUPPORT_IEEE80211W__									
									,
									unsigned char assoc_ch
									)
{
	unsigned char ret = 0;
	char	tmp_number[5];
	char	*reply = NULL;

#if defined ( __SUPPORT_P2P__ )
	if (   (sysmode == E_SYSMODE_P2P)
		|| (sysmode == E_SYSMODE_P2P_GO)
		|| (sysmode == E_SYSMODE_STA_P2P && cur_iface == WLAN1_IFACE))
	{
		return E_CONTINUE;
	}
#endif	// __SUPPORT_P2P__

	reply = pvPortMalloc(64);

	if (reply == NULL)
	{
		PRINTF("Malloc Error - reply\n");
		return E_ERROR;
	}

	memset(reply, 0, 64);

	{
		/* PROFILE ID */
		sprintf(p_cmdline, "%d"CMD_DELIMIT, cur_iface);

		/* MODE */
		if (sysmode == E_SYSMODE_AP
				|| (sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE))
		{
			strcat(p_cmdline, "2"CMD_DELIMIT"\'"); /* MODE: AP */
		}
		else
		{
			strcat(p_cmdline, "1"CMD_DELIMIT"\'"); /* MODE: Station */
		}
	}

	/* SSID */
	strcat(p_cmdline, p_ssid);
	strcat(p_cmdline, "\'");

#ifdef __SUPPORT_WPA_ENTERPRISE__
	if (auth_mode >= E_AUTH_MODE_ENT)
	{
#ifdef __SUPPORT_WPA3_ENTERPRISE__
#ifdef __SUPPORT_WPA3_ENTERPRISE_192B__
		if (auth_mode == E_AUTH_MODE_WPA3_ENT_192B)
		{
			strcat(p_cmdline, CMD_DELIMIT"EAP-WPA3-192B_"); // WPA3-ENT 192Bits
		}
		else
#endif // __SUPPORT_WPA3_ENTERPRISE_192B__
		if (auth_mode == E_AUTH_MODE_WPA3_ENT && encryp_mode == E_ENCRYP_MODE_CCMP && pmf == E_PMF_MANDATORY ) // WPA3-ENT 128Bits
		{
			strcat(p_cmdline, CMD_DELIMIT"EAP-WPA3_"); // WPA3-ENT (WPA-EAP-SHA256, ieee80211w 2)
		}
		else if (auth_mode == E_AUTH_MODE_WPA2_WPA3_ENT && pmf == E_PMF_OPTIONAL) // WPA2/WPA3-ENT 128Bits
		{
			strcat(p_cmdline, CMD_DELIMIT"EAP-WPA23_"); // WPA2/3-ENT (WPA-EAP-SHA256 WPA-EAP, ieee80211w 1)
		}
		else // if (auth_mode == E_AUTH_MODE_ENT && pmf == E_PMF_NONE) // WPA1/WPA2-ENT 
#endif // __SUPPORT_WPA3_ENTERPRISE__
		{
			strcat(p_cmdline, CMD_DELIMIT"EAP-WPA12_"); // WPA1/2-ENT (WPA-EAP ieee80211w 0)
		}
	
		// Enterprise
		switch (eap_auth)
		{
			case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
				strcat(p_cmdline, "PEAP_TTLS_FAST");
				break;

			case E_EAP_AUTH_MODE_PEAP:
				strcat(p_cmdline, "PEAP");
				break;

			case E_EAP_AUTH_MODE_TTLS:
				strcat(p_cmdline, "TTLS"CMD_DELIMIT);
				break;

 			case E_EAP_AUTH_MODE_FAST:
				strcat(p_cmdline, "FAST"CMD_DELIMIT);
				break;

			case E_EAP_AUTH_MODE_TLS:
				strcat(p_cmdline, "TLS");
				break;
		}

		switch (eap_phase2_auth)
		{
			case E_EAP_PHASE2_MODE_MSCHAPV2_N_GTC:
				strcat(p_cmdline, CMD_DELIMIT"MSCHAPV2_GTC");
				break;

			case E_EAP_PHASE2_MODE_MSCHAPV2:
				strcat(p_cmdline, CMD_DELIMIT"MSCHAPV2");
				break;

			case E_EAP_PHASE2_MODE_GTC:
				strcat(p_cmdline, CMD_DELIMIT"GTC");
				break;			

			case E_EAP_PHASE2_MODE_NONE:
			default:
				strcat(p_cmdline, CMD_DELIMIT"NONE");
				break;
		}

		/* eap id */
		switch (eap_auth)
		{
			case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
			case E_EAP_AUTH_MODE_PEAP:
			case E_EAP_AUTH_MODE_TTLS:
 			case E_EAP_AUTH_MODE_FAST:
			case E_EAP_AUTH_MODE_TLS:
				// ID
				strcat(p_cmdline, CMD_DELIMIT"\'");
				strcat(p_cmdline, eap_id);
				strcat(p_cmdline, "\'");
		}

		/* eap password */
		switch (eap_auth)
		{
			case E_EAP_AUTH_MODE_PEAP_TTLS_FAST:
			case E_EAP_AUTH_MODE_PEAP:
			case E_EAP_AUTH_MODE_TTLS:
 			case E_EAP_AUTH_MODE_FAST:
				// PW
				strcat(p_cmdline, CMD_DELIMIT"\'");
				strcat(p_cmdline, eap_pw);
				strcat(p_cmdline, "\'");
		}
	}
	else
#endif // __SUPPORT_WPA_ENTERPRISE__

	/* WEP MODE */
	if (auth_mode == E_AUTH_MODE_WEP)
	{
		strcat(p_cmdline, CMD_DELIMIT"WEP"CMD_DELIMIT);	/* WEP KEY */
		snprintf(tmp_number, 2, "%d"CMD_DELIMIT, wep_key_idx - 1);
		strcat(p_cmdline, tmp_number);	/* WEP KEY INDEX */

		/* KEY TYPE */
		if (strlen(p_wep_key) == 10
				|| strlen(p_wep_key) == 26)
		{
			strcat(p_cmdline, CMD_DELIMIT"0x");		/* TYPE: HEXA KEY */
			/* KEY VALUE */
			strcat(p_cmdline, p_wep_key);
		}
		else
		{
			strcat(p_cmdline, CMD_DELIMIT"\'");		/* TYPE: ASCII KEY */
			/* KEY VALUE */
			strcat(p_cmdline, p_wep_key);
			strcat(p_cmdline, "\'");
		}

		/* WPA / WPA2 MODE */
	}
	else if (auth_mode > E_AUTH_MODE_WEP
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
			 && (auth_mode != E_AUTH_MODE_OWE)
#endif /* __SUPPORT_WPA3_OWE__ */
			)
	{
		/* AUTHTICATION */
		switch (auth_mode)
		{
		
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
			case E_AUTH_MODE_SAE:
				strcat(p_cmdline, CMD_DELIMIT"SAE"CMD_DELIMIT); /* SAE */
				break;

			case E_AUTH_MODE_RSN_SAE:
				strcat(p_cmdline, CMD_DELIMIT"RSN_SAE"CMD_DELIMIT); /* WPA2, SAE */
				break;
#endif /* __SUPPORT_WPA3_SAE__ */

			case E_AUTH_MODE_WPA:
				strcat(p_cmdline, CMD_DELIMIT"WPA"CMD_DELIMIT); /* WPA1 */
				break;

			case E_AUTH_MODE_RSN:
#ifdef __SUPPORT_IEEE80211W__
				if (pmf == E_PMF_MANDATORY)
				{
					strcat(p_cmdline, CMD_DELIMIT"RSN_PMF2"CMD_DELIMIT); /* WPA2 PMF 2 */
				}
				else if (pmf == E_PMF_OPTIONAL)
				{
					strcat(p_cmdline, CMD_DELIMIT"RSN_PMF1"CMD_DELIMIT); /* WPA2 PMF 1 */
				}
#endif /* __SUPPORT_IEEE80211W__ */
				else {
					strcat(p_cmdline, CMD_DELIMIT"RSN"CMD_DELIMIT); /* WPA2 */
				}
				break;

			case E_AUTH_MODE_WPA_RSN:
			default: /* case 4: */
#ifdef __SUPPORT_IEEE80211W__
				if (pmf == E_PMF_MANDATORY)
				{
					strcat(p_cmdline, CMD_DELIMIT"WPA_RSN_PMF"CMD_DELIMIT); /* AUTO(WPA1, WAP2) */
				}
				else
#endif /* __SUPPORT_IEEE80211W__ */
				{
					strcat(p_cmdline, CMD_DELIMIT"WPA_RSN"CMD_DELIMIT); /* AUTO(WPA1, WAP2) */
				}
				break;
		}

		/* ENCRYPTION */
		switch (encryp_mode)
		{
			case E_ENCRYP_MODE_TKIP:
				strcat(p_cmdline, CMD_DELIMIT"TKIP"CMD_DELIMIT);
				break;

			case E_ENCRYP_MODE_CCMP:
				strcat(p_cmdline, CMD_DELIMIT"CCMP"CMD_DELIMIT); /* AES */
				break;

			case E_ENCRYP_MODE_TKIP_CCMP:
			default:  /* case 2: */
				strcat(p_cmdline, CMD_DELIMIT"TKIP_CCMP"CMD_DELIMIT); /* AUTO(TKIP, CCMP) */
				break;
		}

		/* PSK_KEY */
		if (psk_key_type == E_PSK_KEY_ASCII)
		{
			strcat(p_cmdline, "\'");
		}

		strcat(p_cmdline, p_psk_key);

		if (psk_key_type == E_PSK_KEY_ASCII)
		{
			strcat(p_cmdline, "\'");
		}
	}
#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
	else if (auth_mode == E_AUTH_MODE_OWE)
	{
		/* OWE */
		strcat(p_cmdline, CMD_DELIMIT"OWE");
	}
#endif /* __SUPPORT_WPA3_OWE__ */
	else
	{
		/* OPEN MODE */
		strcat(p_cmdline, CMD_DELIMIT"OPEN");
	}

	if (   sysmode == E_SYSMODE_AP
		|| (sysmode == E_SYSMODE_STA_AP && cur_iface == WLAN1_IFACE))
	{
		/* Channel */
		snprintf(tmp_number, 5, CMD_DELIMIT"%d"CMD_DELIMIT, channel);
		strcat(p_cmdline, tmp_number);

		/* Country Code */
		//strcat(p_cmdline, e_country);
	}

	if (iface_start == iface_end
			|| (iface_start == WLAN0_IFACE && cur_iface == WLAN0_IFACE))
	{
		clear_tmp_nvram_env();
	}

	ret = (unsigned char)wlaninit_cmd_parser(p_cmdline, CMD_DELIMIT);

	if (ret != 0)
	{
		vPortFree(reply);
		return E_ERROR;
	}

	/* Advenced configuration */
	if (cur_iface == WLAN0_IFACE)
	{
		/* Connect to Hidden SSID */
		if (connect_hidden_ssid != 0)
		{
			memset(p_cmdline, 0, E_CMDLINE_LEN);
			sprintf(p_cmdline, "set_network 0 scan_ssid %d", connect_hidden_ssid);
			da16x_cli_reply(p_cmdline, NULL, reply);
		}
	}

	if (cur_iface == WLAN1_IFACE )
	{
		if (sysmode == E_SYSMODE_STA_AP || sysmode == E_SYSMODE_AP)
		{
			/* WiFi Mode */
			if (wifi_mode != E_WIFI_MODE_BGN)
			{
				memset(p_cmdline, 0, E_CMDLINE_LEN);
				sprintf(p_cmdline, "set_network 1 wifi_mode %d", wifi_mode - 1);
				da16x_cli_reply(p_cmdline, NULL, reply);
			}
		}
	}

#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
	if (strlen(p_sae_groups) > 1)
	{
		/* SAE_Groups */
		memset(p_cmdline, 0, E_CMDLINE_LEN);
		sprintf(p_cmdline, "sae_groups %s", p_sae_groups);
		da16x_cli_reply(p_cmdline, NULL, reply);
		memset(p_sae_groups, 0, 20);
	}

#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */

	if (assoc_ch > 0)
	{
		write_tmp_nvram_int(NVR_KEY_ASSOC_CH, assoc_ch);
	}

	vPortFree(reply);
	return E_CONTINUE;
}


unsigned char setup_supp_cfg_to_nvcache(void)
{
	char	*reply = NULL;

	reply = pvPortMalloc(64);

	if (reply == NULL)
	{
		PRINTF("Malloc Error - reply\n");
		return E_ERROR;
	}

	da16x_cli_reply("set_log default", NULL, reply);

	if (strcmp(reply,"OK") != 0)
	{
		vPortFree(reply);
		return E_ERROR;
	}
	
	da16x_cli_reply("save_config", NULL, reply);

	if (strcmp(reply,"OK") != 0)
	{
		vPortFree(reply);
		return E_ERROR;
	}

	vPortFree(reply);

	return E_CONTINUE;
}


unsigned char setup_apply_dpm(unsigned char dpm_mode,
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
									unsigned char dpm_Dynamic_Period_Set,
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
									int dpm_KeepAlive_time,
									int dpm_User_Wakeup_time, int dpm_TIM_wakeup_count)
{
	unsigned char ret = 0;

	if (dpm_mode == 1)
	{
		extern int dpm_write_tmp_nvram_int(const char *name, int val, int def);

		ret = (unsigned char)write_tmp_nvram_int("dpm_mode", 1);

		if (ret != 0)
		{
			PRINTF("ERR: dpm_mode ret =%d\n", ret);
			return E_ERROR;
		}

#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
		extern int set_dpm_DynamicPeriodSet_config(int flag);

		/* DPM Dynamic Period Set */
		set_dpm_DynamicPeriodSet_config((int)dpm_Dynamic_Period_Set);
#endif /* __SUPPORT_DPM_DYNAMIC_PERIOD_SET__ */

		/* DPM Keepalive time */
		set_dpm_keepalive_config(dpm_KeepAlive_time);
		dpm_write_tmp_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME,
								dpm_KeepAlive_time,
								DFLT_DPM_KEEPALIVE_TIME);

		/* DPM User wakeup time */
		dpm_write_tmp_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME,
								dpm_User_Wakeup_time,
								DFLT_DPM_USER_WAKEUP_TIME);

		/* DPM TIM Wakeup Time */
		if (dpm_wu_tm_msec_flag == pdTRUE)
		{
			set_dpm_tim_wakeup_dur((unsigned int)coilToInt(dpm_TIM_wakeup_count / 102.4), 1);
			dpm_write_tmp_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME,
									(int)coilToInt(dpm_TIM_wakeup_count / 102.4),
									(int)coilToInt(DFLT_DPM_TIM_WAKEUP_COUNT / 102.4));
		}
		else
		{
			set_dpm_tim_wakeup_dur((unsigned int)dpm_TIM_wakeup_count, 0);
			dpm_write_tmp_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME,
									dpm_TIM_wakeup_count,
									DFLT_DPM_TIM_WAKEUP_COUNT);
		}
	}
	else
	{
		delete_tmp_nvram_env("dpm_mode");
	}

	return E_CONTINUE;
}


unsigned char setup_apply_fast_connect(unsigned char sysmode, unsigned char e_dpm_mode, unsigned char mode)
{
	unsigned char ret = 0;

	if (sysmode != E_SYSMODE_STA || e_dpm_mode)
	{
		return E_CONTINUE;
	}

	ret = (unsigned char)da16x_set_nvcache_int(DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2, mode);

	if (ret != 0)
	{
		PRINTF("ERR: fast_connect_mode ret =%d\n", ret);
		return E_ERROR;
	}
	return ret;
}


void easy_setup(void)
{
	unsigned int	sysclock;
	unsigned char	ret;
	unsigned char	e_pre_run_mode = (unsigned char)get_run_mode();
	unsigned char	e_ap_iface;
	unsigned char	e_cur_iface, e_iface_start, e_iface_end;

	unsigned char	e_sysmode = 0;
	unsigned char	e_channel = 0;
	unsigned char	e_listen_chan = 0;	// WiFi Direct
	unsigned char	e_go_intent = 0;	// WiFi Direct

	char			e_country[3 + 1];

	char			e_ssid[2][128]; /* 33 ==> 128  for hexa unicode */
	unsigned char	e_connect_hidden_ssid = 0;
	unsigned char	e_auth_mode[2] = { 0, 0 };
#ifdef __SUPPORT_WPA_ENTERPRISE__
	unsigned char	e_eap_auth = 0;
	unsigned char	e_eap_phase2 = 0;
#endif // __SUPPORT_WPA_ENTERPRISE__
	unsigned char	e_encryp_mode[2] = { 0, 0 };
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
	char			e_sae_groups[20];
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__
	unsigned char	e_psk_key_type[2] = { 0, 0 };
#ifdef __SUPPORT_WPA_ENTERPRISE__
	char			e_eap_id[64 + 1];
	char			e_eap_pw[64 + 1];
#endif // __SUPPORT_WPA_ENTERPRISE__
	char			e_psk_key[2][64 + 1];
	unsigned char	e_wifi_mode = E_WIFI_MODE_BGN;

	unsigned char	e_wep_bit[2] = { 0, 0 };
	unsigned char	e_wep_key_idx[2] = { 1, 1 };
	unsigned char	e_wep_key_type[2] = { 0, 0 };
	char			e_wep_key[2][26 + 1];

#if defined __SUPPORT_IEEE80211W__ || defined __SUPPORT_WPA3_PERSONAL__
	unsigned char	e_pmf[2] = { E_PMF_NONE, E_PMF_NONE };
#endif // __SUPPORT_IEEE80211W__ || __SUPPORT_WPA3_PERSONAL__

	int e_assoc_ch = 0;

	unsigned char	e_netmode[2] = {0, 0};
	char	e_ipaddress[2][16];
	char	e_subnetmask[2][16];
	char	e_gateway[2][16];
	char	e_dns[2][16];
#ifdef __SUPPORT_NAT__
	unsigned char 	e_nat_flag = 0;
#endif // __SUPPORT_NAT__

	unsigned char	e_use_dhcps = 0;
	char	e_dhcp_lease_start[16];
	char	e_dhcp_lease_end[16];
	int		e_dhcp_lease_time = 0;
	int		e_dhcp_lease_count = 0;

	unsigned char 	e_dpm_mode = 0;
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
	unsigned char	e_dpm_Dynamic_Period_Set = DFLT_DPM_DYNAMIC_PERIOD_SET;
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
	int		e_dpm_KeepAlive_time = DFLT_DPM_KEEPALIVE_TIME;
	int		e_dpm_User_Wakeup_time = DFLT_DPM_USER_WAKEUP_TIME;
	int		e_dpm_TIM_wakeup_count = DFLT_DPM_TIM_WAKEUP_COUNT;

	unsigned char	e_fst_connect = 0;

	char	cmdline[E_CMDLINE_LEN];
	char	tmp_number[5];

	unsigned char	e_sntp_client = 0;
	int		e_sntp_client_period_time = NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL;
	char	e_sntp_gmt_timezone[8];
	int		e_sntp_timezone_int = 0;
	char	e_sntp_svr_addr[256];
	char	e_sntp_svr_1_addr[256];
	char	e_sntp_svr_2_addr[256];
	int		select_ssid = 0;
	int		e_channel_selected = 0;

	memset(cmdline, 0, E_CMDLINE_LEN);
	memset(e_country, 0, 4);

	memset(e_ipaddress, 0, 32);
	memset(e_subnetmask, 0, 32);
	memset(e_gateway, 0, 32);
	memset(e_dns, 0, 32);

	memset(e_sntp_gmt_timezone, 0, 8);
	memset(e_sntp_svr_addr, 0, 256);
	memset(e_sntp_svr_1_addr, 0, 256);
	memset(e_sntp_svr_2_addr, 0, 256);

	ret = setup_stop_services();

	switch (ret)
	{
		case E_ERROR:
			goto CMD_ERROR;
	
		case E_QUIT:
			goto CMD_QUIT;
	
		case E_CONTINUE:
		default:
			break;
	}

	/* Get current system CPU clock */
	_sys_clock_read(&sysclock, sizeof(unsigned int));
	sysclock = sysclock / MHz;

	VT_CLEAR;		/* Clear screen */
	VT_CURPOS(0, 0);		/* move cursor position : 0,0 */
	PRINTF("\n" ANSI_COLOR_BLACK ANSI_BCOLOR_YELLOW "[ %s EASY SETUP ]" ANSI_BCOLOR_DEFULT
		   ANSI_COLOR_DEFULT "\n", CHIPSET_NAME);

	ret = setup_country_code(e_country, &e_channel);

	if (ret == E_QUIT)
	{
		goto CMD_QUIT;
	}

WLAN_SETUP:

	// Reset wifi settings
	memset(e_ssid, 0, 128*2);
	e_connect_hidden_ssid = 0;
	memset(e_auth_mode, 0, 2);
	memset(e_encryp_mode, 0, 2);

	memset(e_psk_key_type, 0, 2);
	memset(e_psk_key, 0, 65*2);
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
	memset(e_sae_groups, 0, 20);
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__

	memset(e_wep_bit, 0, 2);
	memset(e_wep_key_idx, 0, 2);
	memset(e_wep_key_type, 0, 2);
	memset(e_wep_key, 0, 27*2);

	e_wifi_mode = E_WIFI_MODE_BGN;
	
#if defined __SUPPORT_IEEE80211W__ || defined __SUPPORT_WPA3_PERSONAL__
	memset(e_pmf, E_PMF_NONE, 2);
#endif // __SUPPORT_IEEE80211W__ || __SUPPORT_WPA3_PERSONAL__
	
	e_assoc_ch = 0;

	ret = setup_sysmode(&e_sysmode);
	if (ret == E_QUIT)
	{
		goto CMD_QUIT;
	}

	/* select configuration Interface */
	setup_selcet_interface(e_sysmode, &e_iface_start, &e_iface_end);

	if (dhcpd_support_flag == pdTRUE)
	{
		/* fix interface value as 1 for AP mode */
		e_ap_iface = WLAN1_IFACE;
	}

	// input configuration
	for (e_cur_iface = e_iface_start; e_cur_iface <= e_iface_end; e_cur_iface++)
	{
		/* Print Title */
		setup_display_title(e_sysmode, e_cur_iface);

		/* SCAN SSID */
		if (    e_sysmode == E_SYSMODE_STA
			|| (e_cur_iface == WLAN0_IFACE && e_sysmode == E_SYSMODE_STA_AP)
#if defined ( CONFIG_P2P )
			|| (e_cur_iface == WLAN0_IFACE && e_sysmode == E_SYSMODE_STA_P2P)
#endif	// CONFIG_P2P
		   )
		{
			ret = setup_display_scan_list(e_ssid[e_cur_iface],
										  &e_connect_hidden_ssid,
										  &e_auth_mode[e_cur_iface],
										  &e_encryp_mode[e_cur_iface],
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
										  e_sae_groups,
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__
										  &e_psk_key_type[e_cur_iface],
										  e_psk_key[e_cur_iface],
#if defined __SUPPORT_IEEE80211W__
										  &e_pmf[e_cur_iface],
#endif // __SUPPORT_IEEE80211W__
										  &e_wifi_mode, &select_ssid, &e_channel_selected);

			switch (ret)
			{
				case E_INPUT_SSID:
					goto INPUT_SSID;
#ifdef __SUPPORT_WPA_ENTERPRISE__
				case E_INPUT_EAP_METHOD:
					goto INPUT_EAP_METHOD;
#endif // __SUPPORT_WPA_ENTERPRISE__
				case E_INPUT_PSK_KEY:
					goto INPUT_PSK_KEY;

				case E_INPUT_WEP:
					goto INPUT_WEP;

				case E_AVD_CFG:
					goto AVD_CFG;

				case E_QUIT:
					goto CMD_QUIT;

				case E_CONTINUE:
				default:
					break;
			}
		}
		else if (    e_sysmode == E_SYSMODE_STA
				 || (e_cur_iface == WLAN0_IFACE	&& e_sysmode == E_SYSMODE_STA_AP)
#if defined ( CONFIG_P2P )
				 || (e_cur_iface == WLAN0_IFACE && e_sysmode == E_SYSMODE_STA_P2P)
#endif	// CONFIG_P2P
				)
		{
			PRINTF("\n" ANSI_COLOR_RED ANSI_BOLD
				   "The current system mode is not supported site survey." ANSI_NORMAL ANSI_COLOR_DEFULT
				   "\n");
		}

#if defined ( CONFIG_P2P )
		ret = setup_p2p_config(e_sysmode, e_cur_iface, &e_channel, &e_listen_chan, &e_go_intent);

		switch (ret)
		{
			case E_SETUP_WIFI_END:
				goto SETUP_WIFI_END;
		
			case E_QUIT:
				goto CMD_QUIT;
		
			case E_CONTINUE:
			default:
				break;
		}
#endif	// CONFIG_P2P

		/* INPUT SSID */
INPUT_SSID:
		ret = setup_ssid_input_ui(e_ssid[e_cur_iface], e_cur_iface);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		ret = setup_select_channel(e_sysmode, e_cur_iface, e_country, &e_channel);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		ret = setup_input_auth(e_sysmode, e_cur_iface, &e_auth_mode[e_cur_iface]);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		ret = setup_select_encryption(e_auth_mode[e_cur_iface],
									  &e_encryp_mode[e_cur_iface]
#if defined __SUPPORT_IEEE80211W__
									  , &e_pmf[e_cur_iface]
#endif // __SUPPORT_IEEE80211W__
									  );
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

#ifdef __SUPPORT_WPA_ENTERPRISE__
INPUT_EAP_METHOD:
		ret = setup_input_eap_method(e_auth_mode[e_cur_iface], &e_eap_auth, &e_eap_phase2);

		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		ret = setup_input_enterp_id_cert(e_auth_mode[e_cur_iface], e_eap_auth, e_eap_id, e_eap_pw);

		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}
#endif // __SUPPORT_WPA_ENTERPRISE__

INPUT_PSK_KEY:
		ret = setup_input_psk(e_auth_mode[e_cur_iface], &e_psk_key_type[e_cur_iface],
							  e_psk_key[e_cur_iface]);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

INPUT_WEP:
		ret = setup_input_wep(e_auth_mode[e_cur_iface], e_cur_iface,
							  &e_wep_key_type[e_cur_iface], &e_wep_bit[e_cur_iface],
							  &e_wep_key_idx[e_cur_iface], e_wep_key[e_cur_iface]);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		if (e_cur_iface == WLAN1_IFACE && e_channel == 14)   /* softap channel 14 */
		{
			goto SETUP_WIFI_END; /* 11b only */
		}

AVD_CFG:
		// --- ADVANCED CONFIG ---

#if (defined __SUPPORT_WPA3_OWE__ && defined __SUPPORT_WPA3_PERSONAL__)
		if (e_auth_mode[e_cur_iface] != E_AUTH_MODE_OWE)
#endif // __SUPPORT_WPA3_OWE__
		{
			ret = setup_adv(e_encryp_mode[e_cur_iface], &e_wifi_mode);

			switch (ret)
			{
				case E_SETUP_WIFI_END:
					goto SETUP_WIFI_END;

				case E_QUIT:

					goto CMD_QUIT;

				case E_CONTINUE:
				default:
					break;
			}

#ifdef __SUPPORT_WPA_ENTERPRISE__
			if (e_auth_mode[e_cur_iface] < E_AUTH_MODE_ENT)
#endif // __SUPPORT_WPA_ENTERPRISE__
			{
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || (defined __SUPPORT_MESH__)
				/* SAE Groups */
				if (  (   e_auth_mode[e_cur_iface] == E_AUTH_MODE_SAE
					   || e_auth_mode[e_cur_iface] == E_AUTH_MODE_RSN_SAE)
					&& strlen(e_sae_groups) == 0)
				{
					ret = setup_input_sae_group(e_sae_groups);

					if (ret == E_QUIT)
					{
						goto CMD_QUIT;
					}

				}
#endif /* __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__ */

#if defined __SUPPORT_IEEE80211W__
				ret = setup_pmf(e_auth_mode[e_cur_iface], e_encryp_mode[e_cur_iface], &e_pmf[e_cur_iface]);
				if (ret == E_QUIT)
				{
					goto CMD_QUIT;
				}
#endif // __SUPPORT_IEEE80211W__

				ret = setup_wifimode(e_cur_iface, e_auth_mode[e_cur_iface], e_encryp_mode[e_cur_iface], &e_wifi_mode);
				if (ret == E_QUIT)
				{
					goto CMD_QUIT;
				}
			}

			ret = setup_connect_hidden_ssid(e_sysmode, e_cur_iface, &e_connect_hidden_ssid);
			if (ret == E_QUIT)
			{
				goto CMD_QUIT;
			}
		}

SETUP_WIFI_END:
		setup_display_wlan_preset(e_sysmode, e_cur_iface, e_ssid[e_cur_iface], (char)e_channel,
							 (char)e_listen_chan, (char)e_go_intent,
							 e_auth_mode[e_cur_iface], e_encryp_mode[e_cur_iface],
#ifdef __SUPPORT_WPA_ENTERPRISE__
								e_eap_auth,
								e_eap_phase2,
								e_eap_id,
								e_eap_pw,
#endif // __SUPPORT_WPA_ENTERPRISE__
							 e_psk_key[e_cur_iface], e_psk_key_type[e_cur_iface],
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
							 e_sae_groups,
#endif // __SUPPORT_WPA3_SAE__ || defined __SUPPORT_MESH__

							 e_wep_key[e_cur_iface], e_wep_bit[e_cur_iface],
							 e_wep_key_idx[e_cur_iface], e_wep_key_type[e_cur_iface],
							 (unsigned char)e_assoc_ch,
							 e_wifi_mode, e_connect_hidden_ssid
#if defined __SUPPORT_IEEE80211W__
							 , e_pmf[e_cur_iface]
#endif // __SUPPORT_IEEE80211W__
							 );



		ret = setup_wifi_config_confirm(e_sysmode, e_cur_iface);
		switch (ret)
		{
			case E_WLAN_SETUP:
				goto WLAN_SETUP;

			case E_SETUP_APPLY_START:
				goto SETUP_APPLY_START;

			case E_QUIT:
				goto CMD_QUIT;

			case E_CONTINUE:
			default:
				break;
		}


		// --- NETWORK ---
NETWORK_SETUP:

		ret = setup_netmode(e_sysmode, e_cur_iface, &e_netmode[e_cur_iface]);
		if (ret == E_QUIT)
		{
			goto CMD_QUIT;
		}

		// Static IP
		if (e_netmode[e_cur_iface] == E_NETMODE_STATIC_IP)
		{
INPUT_IPADDRESS:
			/*  IP address*/
			ret = setup_static_ip(e_cur_iface, e_ipaddress[e_cur_iface]);
			if (ret == E_QUIT)
			{
				goto CMD_QUIT;
			}

			// Subnet mask
			ret = setup_static_subnet(e_cur_iface, e_subnetmask[e_cur_iface]);
			if (ret == E_QUIT)
			{
				goto CMD_QUIT;
			}

			// Gateway
			ret = setup_static_gateway(e_sysmode, e_cur_iface, e_ipaddress[e_cur_iface],
									  e_subnetmask[e_cur_iface], e_gateway[e_cur_iface]);
			if (ret == E_QUIT)
			{
				goto CMD_QUIT;
			}

			// DNS
			ret = setup_static_dns(e_sysmode, e_cur_iface, e_dns[e_cur_iface]);

			if (ret == E_QUIT)
			{
				goto CMD_QUIT;
			}

			// Display config stats
			setup_display_network_preset(e_cur_iface, e_sysmode, e_ipaddress[e_cur_iface],
									  e_subnetmask[e_cur_iface], e_gateway[e_cur_iface],
									  e_dns[e_cur_iface]);

			ret = setup_check_network(e_cur_iface, e_sysmode, e_ipaddress[e_cur_iface],
									  e_subnetmask[e_cur_iface], e_gateway[e_cur_iface],
									  e_dns[e_cur_iface]);

			switch (ret)
			{
				case E_NETWORK_SETUP:
					goto NETWORK_SETUP;
			
				case E_INPUT_IPADDRESS:
					goto INPUT_IPADDRESS;
						
				case E_QUIT:
					goto CMD_QUIT;
			
				case E_CONTINUE:
				default:
					break;
			}
		}
		else
		{
			e_use_dhcps = 0;
		}

		ret = setup_network_config_confirm(e_sysmode);

		switch (ret)
		{
			case E_NETWORK_SETUP:
				goto NETWORK_SETUP;

			case E_INPUT_IPADDRESS:
				goto INPUT_IPADDRESS;
					
			case E_QUIT:
				goto CMD_QUIT;

			case E_CONTINUE:
			default:
				break;
		}

		/* SNTP */
		if (   e_sysmode != E_SYSMODE_AP
#if defined ( __SUPPORT_P2P__ )
			&& e_sysmode != E_SYSMODE_P2P_GO
#endif	// __SUPPORT_P2P__
		   )
		{
			if (   e_sysmode == E_SYSMODE_STA
				|| (   e_cur_iface == WLAN0_IFACE
					&& (   e_sysmode == E_SYSMODE_STA_AP
#if defined ( __SUPPORT_P2P__ )
						|| e_sysmode == E_SYSMODE_STA_P2P
#endif	// __SUPPORT_P2P__
					   )
				   )
			   )
			{
				ret = setup_sntp_client(&e_sntp_client, &e_sntp_client_period_time,
							  e_sntp_gmt_timezone, &e_sntp_timezone_int,
							  e_sntp_svr_addr, e_sntp_svr_1_addr,
							  e_sntp_svr_2_addr);

				switch (ret)
				{
					case E_QUIT:
						goto CMD_QUIT;
				
					case E_CONTINUE:
					default:
						break;
				}
			}
			else
			{
				/* In case of if SNTP client doesn't configured yet ...,
				 *  change the value as E_SNTP_CLIENT_STOP */
				if (e_sntp_client != E_SNTP_CLIENT_START)
				{
					e_sntp_client = E_SNTP_CLIENT_STOP;
				}
			}
		}

		// For fast_connection with sleep 1/2 mode
		if (fast_conn_sleep_12_flag == pdTRUE) {
			ret = setup_fast_connection(e_sysmode, e_dpm_mode, &e_fst_connect, &e_assoc_ch, e_channel_selected);

			switch (ret)
			{
				case E_QUIT:
					goto CMD_QUIT;
			
				case E_CONTINUE:
				default:
					break;
			}
		}

		if (e_fst_connect == pdFALSE) {
			// Setup DPM mode
			ret = setup_dpm(e_sysmode, e_cur_iface, &e_dpm_mode,
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
							&e_dpm_Dynamic_Period_Set,
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
							&e_dpm_KeepAlive_time, &e_dpm_User_Wakeup_time,
							&e_dpm_TIM_wakeup_count);

			if (ret == E_QUIT) {
				goto CMD_QUIT;
			}
		}

		if (dhcpd_support_flag == pdTRUE)
		{
			setup_dhcp_server(e_sysmode, e_cur_iface, e_netmode[e_cur_iface],
							e_ipaddress[e_cur_iface], e_subnetmask[e_cur_iface],
							e_gateway[e_cur_iface], e_dns[e_cur_iface],
							&e_use_dhcps,
							e_dhcp_lease_start, e_dhcp_lease_end,
							&e_dhcp_lease_time, &e_dhcp_lease_count);
		}
	}

//=============================================================================
SETUP_APPLY_START:

	// Start Apply
	ret = setup_apply_preset(e_country, cmdline);

	switch (ret)
	{
		case E_QUIT:
			goto CMD_QUIT;
	
		case E_ERROR:
			goto CMD_ERROR;
	
		case E_CONTINUE:
		default:
			break;
	}

	// apply configuration
	for (e_cur_iface = e_iface_start; e_cur_iface <= e_iface_end; e_cur_iface++)
	{
		memset(cmdline, 0, E_CMDLINE_LEN);
		memset(tmp_number, 0, 5);

#ifdef	CONFIG_P2P
		ret = setup_apply_p2p(e_sysmode, e_cur_iface, cmdline, e_channel, e_listen_chan, e_go_intent);
		
		switch (ret)
		{
			case E_SETUP_APPLY_SYSMODE:
				goto SETUP_APPLY_SYSMODE;

			case E_QUIT:
				goto CMD_QUIT;
		
			case E_ERROR:
				goto CMD_ERROR;
		
			case E_CONTINUE:
			default:
				break;
		}
#endif	/* CONFIG_P2P */

		ret = setup_apply_wlan(e_sysmode, e_cur_iface, e_iface_start, e_iface_end,
							cmdline,
							e_ssid[e_cur_iface], e_auth_mode[e_cur_iface], e_encryp_mode[e_cur_iface],
#ifdef __SUPPORT_WPA_ENTERPRISE__
							e_eap_auth, e_eap_phase2,
							e_eap_id, e_eap_pw,
#endif // __SUPPORT_WPA_ENTERPRISE__
							e_psk_key_type[e_cur_iface], e_psk_key[e_cur_iface],
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
							e_sae_groups,
#endif // __SUPPORT_WPA3_SAE__
							e_wep_key[e_cur_iface] , e_wep_key_idx[e_cur_iface],
							e_channel, e_connect_hidden_ssid, e_wifi_mode
#if defined __SUPPORT_IEEE80211W__
							,
							e_pmf[e_cur_iface]
#endif // __SUPPORT_IEEE80211W__
							,
							(unsigned char)e_assoc_ch
				);

		switch (ret)
		{
			case E_INPUT_SKIP_ALL_2:
				goto INPUT_SKIP_ALL_2;
					
			case E_QUIT:
				goto CMD_QUIT;
		
			case E_ERROR:
				goto CMD_ERROR;
		
			case E_CONTINUE:
			default:
				break;
		}

		vTaskDelay(RUN_DELAY);
	}

	ret = setup_supp_cfg_to_nvcache();

	switch (ret)
	{
		
		case E_QUIT:
			goto CMD_QUIT;
	
		case E_ERROR:
			goto CMD_ERROR;
	
	
		case E_CONTINUE:
		default:
			break;
	}

#if defined ( CONFIG_P2P )
SETUP_APPLY_SYSMODE :
#endif	// CONFIG_P2P

	/* SysMode */
	ret = (unsigned char)da16x_set_nvcache_int(DA16X_CONF_INT_MODE, (e_sysmode - 1));

	if (ret != 0)
	{
		PRINTF("ERR: sysmode ret =%d\n", ret);
		goto CMD_ERROR;
	}

	vTaskDelay(RUN_DELAY);

	/* DPM MODE */
	ret = setup_apply_dpm(e_dpm_mode,
#ifdef __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
			e_dpm_Dynamic_Period_Set,
#endif // __SUPPORT_DPM_DYNAMIC_PERIOD_SET__
			e_dpm_KeepAlive_time, e_dpm_User_Wakeup_time, e_dpm_TIM_wakeup_count);

	switch (ret)
	{
		case E_QUIT:
			goto CMD_QUIT;
	
		case E_ERROR:
			goto CMD_ERROR;
	
		case E_CONTINUE:
		default:
			break;
	}	

	// For fast_connection with sleep 1/2 mode
	if (fast_conn_sleep_12_flag == pdTRUE) {
		ret = setup_apply_fast_connect(e_sysmode, e_dpm_mode, e_fst_connect);

		switch (ret)
		{
			case E_QUIT:
				goto CMD_QUIT;
	
			case E_ERROR:
				goto CMD_ERROR;
	
			case E_CONTINUE:
			default:
				break;
		}
	}

	switch (e_sysmode)
	{
		case E_SYSMODE_AP:	/* AP only */
			e_iface_start = WLAN1_IFACE;
			e_iface_end   = WLAN1_IFACE;
			break;

#if defined ( __SUPPORT_P2P__ )
		case E_SYSMODE_P2P:
		case E_SYSMODE_P2P_GO:	/* P2P (GO) only */
			goto INPUT_SKIP_ALL_2;
#endif	// __SUPPORT_P2P__

		case E_SYSMODE_STA_AP:
#if defined ( __SUPPORT_P2P__ )
		case E_SYSMODE_STA_P2P:	/* CONCURRENT */
#endif	// __SUPPORT_P2P__
			e_iface_start = WLAN0_IFACE;
			e_iface_end   = WLAN1_IFACE;
			break;

		case E_SYSMODE_STA:	/* STATION ONLY */
		default:
			e_iface_start = WLAN0_IFACE;
			e_iface_end   = WLAN0_IFACE;
			break;
	}

	/* network apply */
	for (e_cur_iface = e_iface_start; e_cur_iface <= e_iface_end; e_cur_iface++)
	{
		int iface = WLAN0_IFACE;

#if defined ( CONFIG_P2P )
		if (e_sysmode == E_SYSMODE_STA_P2P && e_cur_iface == WLAN1_IFACE)
		{
			if (e_sntp_client == E_SNTP_CLIENT_START)
			{
				goto INPUT_SKIP_ALL_1;
			}
			else
			{
				goto INPUT_SKIP_ALL_2;
			}
		}
#endif	// CONFIG_P2P

		if (   (   e_sysmode != E_SYSMODE_STA
				&& !(e_sysmode == E_SYSMODE_STA_AP && e_cur_iface == WLAN0_IFACE)
#if defined ( CONFIG_P2P )
				&& !(e_sysmode == E_SYSMODE_STA_P2P && e_cur_iface == WLAN0_IFACE)
#endif	// CONFIG_P2P
			   )
		   )
		{
			iface = WLAN1_IFACE; /* AP */
		}

		if (iface == WLAN0_IFACE)
		{
			if (e_netmode[e_cur_iface] == E_NETMODE_STATIC_IP)
			{
				da16x_set_nvcache_str(DA16X_CONF_STR_IP_0, e_ipaddress[e_cur_iface]);
				da16x_set_nvcache_str(DA16X_CONF_STR_NETMASK_0, e_subnetmask[e_cur_iface]);
				da16x_set_nvcache_str(DA16X_CONF_STR_GATEWAY_0, e_gateway[e_cur_iface]);
				da16x_set_nvcache_str(DA16X_CONF_STR_DNS_0, e_dns[e_cur_iface]);
			}
			else // DHCP_Client
			{
				if (e_fst_connect)
				{
					// Temporarily use a static IPaddress.
					da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP, pdTRUE);
				}
				else
				{
					da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_CLIENT_CONFIG_ONLY, DHCPCLIENT);
				}
			}
		}
		else
		{
			da16x_set_nvcache_str(DA16X_CONF_STR_IP_1, e_ipaddress[e_cur_iface]);
			da16x_set_nvcache_str(DA16X_CONF_STR_NETMASK_1, e_subnetmask[e_cur_iface]);

			if (
#if !defined ( __SUPPORT_SOFTAP_DFLT_GW__ )
				   e_sysmode != E_SYSMODE_AP &&
#endif	// !__SUPPORT_SOFTAP_DFLT_GW__
#if defined ( __SUPPORT_P2P__ )
				   e_sysmode != E_SYSMODE_P2P_GO
#else
				pdTRUE
#endif	// __SUPPORT_P2P__
			   )
			{
				da16x_set_nvcache_str(DA16X_CONF_STR_GATEWAY_1, e_gateway[e_cur_iface]);
			}

			da16x_set_nvcache_str(DA16X_CONF_STR_DNS_1, e_dns[e_cur_iface]);
		}
	}

	if (dhcpd_support_flag == pdTRUE)
	{
		/* DHCP Server CMD */
		if (e_use_dhcps && (e_netmode[WLAN1_IFACE] == E_NETMODE_STATIC_IP))   /* Check Static IP */
		{
			/* DNS */
			da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_DNS, e_dns[e_ap_iface]);

			ret = 0;
			
			/* Lease range */
			ret = (unsigned char)(ret + da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_START_IP, e_dhcp_lease_start));

			ret = (unsigned char)(ret + da16x_set_nvcache_str(DA16X_CONF_STR_DHCP_END_IP, e_dhcp_lease_end));


			/* Lease time */
			ret = (unsigned char)da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_LEASE_TIME, e_dhcp_lease_time);

			if (ret != 0)
			{
				PRINTF("ERR: WLAN%d - Lease range\n", e_cur_iface);
				goto CMD_ERROR;
			}
		}

		/* DHCP Server boot Start */
		da16x_set_nvcache_int(DA16X_CONF_INT_DHCP_SERVER, e_use_dhcps);
	}

#if defined ( CONFIG_P2P )
INPUT_SKIP_ALL_1:
#endif	// CONFIG_P2P

	if (e_sntp_client == E_SNTP_CLIENT_START) {
		da16x_set_nvcache_int(DA16X_CONF_INT_SNTP_CLIENT, 1);
		da16x_set_nvcache_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, e_sntp_client_period_time);
		da16x_set_nvcache_str(DA16X_CONF_STR_SNTP_SERVER_IP, e_sntp_svr_addr);
		da16x_set_nvcache_str(DA16X_CONF_STR_SNTP_SERVER_1_IP, e_sntp_svr_1_addr);
		da16x_set_nvcache_str(DA16X_CONF_STR_SNTP_SERVER_2_IP, e_sntp_svr_2_addr);
		da16x_set_nvcache_int(DA16X_CONF_INT_GMT, e_sntp_timezone_int);
	} else {
		da16x_set_nvcache_int(DA16X_CONF_INT_SNTP_CLIENT, 0);
	}

INPUT_SKIP_ALL_2:

	da16x_nvcache2flash();

	// Backup NVRAM area
	if (backup_NVRAM() != 0) {
		PRINTF(RED_COLOR "\nSETUP : Error to backup NVRAM (easy-setup) !!!\n" CLEAR_COLOR);
	}

	PRINTF("\n" ANSI_COLOR_RED "Reboot..." ANSI_NORMAL "\n\n");

	if (ble_combo_ref_flag == pdTRUE) {
		extern void combo_ble_sw_reset(void);
		combo_ble_sw_reset();
	}

	/* Recover saved CPU clock */
	do_write_clock_change(sysclock * MHz, pdFALSE);

	/* reboot */
	vTaskDelay(RUN_DELAY);

	reboot_func(SYS_REBOOT);
	return;

CMD_ERROR:
	PRINTF("\nSETUP: ERR(ret=%d) Please try again after factory reset!\n", ret);

CMD_QUIT:

	PRINTF("\n" ANSI_BOLD ANSI_COLOR_RED
		   "All services have been stopped.\nPlease reboot or setup again."  ANSI_NORMAL
		   ANSI_COLOR_DEFULT "\n\n");

	set_run_mode(e_pre_run_mode);
}

#endif	/*XIP_CACHE_BOOT*/

/* EOF */
