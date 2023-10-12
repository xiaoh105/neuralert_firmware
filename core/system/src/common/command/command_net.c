/**
 ****************************************************************************************
 *
 * @file command_net.c
 *
 * @brief Network application console commands
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

#ifdef XIP_CACHE_BOOT

#include "sdk_type.h"

#include "da16x_system.h"
#include "command.h"
#include "command_net.h"
#include "common_def.h"

#include "application.h"
#include "iface_defs.h"
#include "nvedit.h"
#include "environ.h"

#include "user_dpm.h"
#include "da16x_network_common.h"
#include "da16x_sntp_client.h"

#include "da16x_time.h"
#include "clib.h"

#include "sys_feature.h"
#include "mqtt_client.h"

#include "lwipopts.h"

#ifdef LWIP_DNS
#include "lwip/dns.h"
#endif	/*LWIP_DNS*/
#include "lwiperf.h"

#ifdef  __SUPPORT_OTA__
#include "ota_update.h"
#endif /*  __SUPPORT_OTA__ */

#ifdef __SUPPORT_SNTP_CLIENT__
#include "sntp.h"
#endif	/*__SUPPORT_SNTP_CLIENT__*/

#include "da16x_arp.h"

#include "user_http_client.h"
#include "user_http_server.h"
#include "zero_config.h"
#include "da16x_dhcp_client.h"
#include "util_api.h"

extern UINT isVaildDomain(char *domain);
extern int isvalidIPsubnetInterface(long ip, int iface);
extern int is_date_time_valid(struct tm *t);

void cmd_network_http_svr(int argc, char *argv[])
{
	run_user_http_server(argv, argc);
}

void cmd_network_https_svr(int argc, char *argv[])
{
	run_user_https_server(argv, argc);
}

void cmd_network_http_client(int argc, char *argv[])
{
	run_user_http_client(argc, argv);
}

void cmd_network_http_client_sni(int argc, char *argv[])
{
	http_client_set_sni(argc, argv);
}

void cmd_network_http_client_alpn(int argc, char *argv[])
{
	http_client_set_alpn(argc, argv);
}

#if defined (__SUPPORT_OTA__)
void cmd_ota_update(int argc, char *argv[])
{
	if (ota_update_cmd_parse(argc, argv)) {
		PRINTF("FAIL\n");
	} else {
		PRINTF("OK\n");
	}
	return;
}
#endif /*__SUPPORT_OTA__*/


#ifdef	__SUPPORT_NSLOOKUP__
void	cmd_nslookup(int argc, char *argv[]);
#endif	/* __SUPPORT_NSLOOKUP__ */

#ifdef	__SUPPORT_SNTP_CLIENT__
void	cmd_network_sntp(int argc, char *argv[]);
#endif	/* __SUPPORT_SNTP_CLIENT__ */

void	cmd_arp_send(int argc, char *argv[]);

void nslookup_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg);

// mDNS CMD
void	cmd_mdns_start(int argc, char *argv[]);
void	cmd_mdns_stop(int argc, char *argv[]);
void	cmd_mdns_change_host(int argc, char *argv[]);
void	cmd_mdns_lookup(int argc, char *argv[]);
void	cmd_mdns_config(int argc, char *argv[]);
void	cmd_zeroconf_state(int argc, char *argv[]);
void	cmd_zeroconf_cache(int argc, char *argv[]);
// xmDNS CMD
void	cmd_xmdns_start(int argc, char *argv[]);
void	cmd_xmdns_stop(int argc, char *argv[]);
void	cmd_xmdns_change_host(int argc, char *argv[]);
void	cmd_xmdns_lookup(int argc, char *argv[]);
// DNS-SD CMD
void	cmd_dns_sd(int argc, char *argv[]);

void	cmd_mqtt_client(int argc, char *argv[]);
//void	cmd_debug(int argc, char *argv[]);

/* debug cmd */
void cmd_debug(int argc, char *argv[])
{
	if (argc == 1 || (strcasecmp(argv[1], "HELP") == 0)) {
		PRINTF("debug [option]\n");
		PRINTF("\tarp [on|off]\n");

		if (dhcpd_support_flag == pdTRUE) {
#ifdef __CMD_DHCPD_RX_MSG_DROP__
			PRINTF("\tdhcpd_drop [option]\n");
#endif /* __DHCPD_RX_MSG_DROP__ */
		}

		PRINTF("\tdhcpc [level] level=0~6 defaut 1\n");
		PRINTF("\tumac on|off mask\n"
			   "\t\te.g. debug umac 1 0x4\n");

		PRINTF("\twpa_cli [on|off]\n");
#ifdef DEBUG_SAMPLE
		PRINTF("\tsample [on|off]\n");
#endif /* DEBUG_SAMPLE */
	} else if (strcasecmp(argv[1], "WPA_CLI") == 0 && (argc == 3)) {
		extern char dbg_wpa_cli;

		if (strcasecmp(argv[2], "ON") == 0) {
			dbg_wpa_cli = 1;
		} else {
			dbg_wpa_cli = 0;
		}

		PRINTF("Debug WPA_CLI : %s\n", dbg_wpa_cli ? "On" : "Off");
	} else if (strcasecmp(argv[1], "ARP") == 0 && (argc == 3)) {
#ifdef DEBUG_ARP

		if (strcasecmp(argv[2], "ON") == 0) {
			set_debug_arp(1);
		} else {
			set_debug_arp(0);
		}

		PRINTF("Debug ARP : %s\n", get_debug_arp() ? "On" : "Off");
#endif	/* DEBUG_ARP */
#ifdef __CMD_DHCPD_RX_MSG_DROP__
	} else if (strcasecmp(argv[1], "DHCPD_DROP") == 0) {
		if ( argc == 3
				&& (argv[2][0] == '1'
					|| argv[2][0] == '3'
					|| argv[2][0] == '4'
					|| argv[2][0] == '7'
					|| argv[2][0] == '8'
					|| argv[2][0] == '0'))
		{
			set_debug_dhcpd_msg_drop((UCHAR)ctoi(argv[2]));
			PRINTF("DHCPD_DROP status = %u\n", get_debug_dhcpd_msg_drop());
		} else {
			PRINTF("DHCPD_DROP status = %u\n", get_debug_dhcpd_msg_drop());
			/* Define the DHCP Message Types.  */
			PRINTF("\nDHCPD_DROP [option]\n"
				   "\t[option]\n"
				   "\tDHCP_DISCOVER 1\n"
				   "\tDHCP_REQUEST	3\n"
				   "\tDHCP_DECLINE	4\n"
				   "\tDHCP_RELEASE	7\n"
				   "\tDHCP_INFORM	8\n"
				   "\tDisable		0\n");
		}

#endif /* __CMD_DHCPD_RX_MSG_DROP__ */
	} else if (strcasecmp(argv[1], "DHCPC") == 0
			 && argc == 3
			 && argv[2][0] >= '0'
			 && argv[2][0] <= '6')
	{
		set_debug_dhcpc((u8_t)ctoi(argv[2]));

		PRINTF("\nDHCP CLIENT Debug Level : %d\n", get_debug_dhcpc());
	} else if (strcasecmp(argv[1], "UMAC") == 0) {
		if (argc == 4) {
			umac_set_debug( (u8)ctoi(argv[2]),			// d_on
							(u16)htoi(argv[3]), (u8)1 );		// d_msk
			return;
		} else if ( argc == 3 && ctoi(argv[2]) ==  0 ) {
			umac_set_debug(0, 0, 1);
			return;
		} else {
			PRINTF(
				"\n"
				"Usage: debug umac is_on dbg_mask(Hexa)\n"
				"e.g.	debug umac 1 0x4 /* UM_RX */\n"
				"		debug umac 0	 /* Off   */\n"
				"dbg_mask info:\n"
				"  UM_TX	  0x01\n"
				"  UM_TX2LM   0x02\n"
				"  UM_RX	  0x04\n"
				"  UM_RX_NI   0x08\n"
				"  UM_SC	  0x10\n"
				"  UM_CN	  0x20\n"
				"  UM_MF	  0x40\n"
				"\n");
			return;
		}

#ifdef FEATURE_P2P_M_ABS_CTW_VAL
	} else if (strcasecmp(argv[1], "P2P_GO_PS") == 0) {

		if (strcasecmp(argv[2], "SET") == 0) {
			int r;
			//check absence value.
			r = umac_set_p2p_go_noa_ctw(ctoi(argv[3]));

			if (!r) {
				return;
			} else {
				//print error #
				PRINTF("Failure of set_p2p_go_noa_ctw :Error#%d\n", r);
				return;
			}
		} else if (strcasecmp(argv[2], "GET") == 0) {
			int a_noa_ctw[2];

			//read values from mode params and p2p_env.
			umac_get_p2p_go_noa_ctw(a_noa_ctw);
			PRINTF("Current absent period:%d ms & CT window:%d ms\n"
				   , a_noa_ctw[0], a_noa_ctw[1] );
			return;
		} else {
			//print usage and inform 3rd arg is wrong.
			PRINTF(
				"\n"
				"Usage: debug p2p_go_ps set|get #\n"
				"e.g.	debug p2p_go_ps set 40	<= Set absent period to 40ms\n"
				"		debug p2p_go_ps get \n"
				"\n");
			return;
		}

#endif //FEATURE_P2P_M_ABS_CTW_VAL

#ifdef DEBUG_SAMPLE
	} else if (strcasecmp(argv[1], "SAMPLE") == 0) {

		if (strcasecmp(argv[2], "ON") == 0) {
			; /* ON */
		} else {
			; /* OFF */
		}

		PRINTF("Debug SAMPLE : %s\n", get_debug_arp() ? "On" : "Off");
#endif /* DEBUG_SAMPLE */
	} else {
		PRINTF("Error: Debug option\n");
	}
}


#define BOOTTIME	"BOOT"
#define UPTIME		"UPTIME"
#define TIMEZONE	"ZONE"
#define SETTIME		"SET"
#define DAYLIGHT	"DST"

int cmd_set_time(char *date_format, char *time_format, int daylight)
{
	struct tm correction;
	char *pos = NULL;
	int tmp_int1 = 0;

	if (!date_format || !time_format) {
		return -1;
	}

    memset(&correction, 0x00, sizeof(struct tm));

	// Year
	pos = strtok(date_format, "-");	
	if (pos && (get_int_val_from_str(pos, &tmp_int1, 3) == 0) ) {
		correction.tm_year = tmp_int1 - 1900;
	} else {
		return -2;
	}

	// Month
	pos = strtok(NULL, "-");
	if (pos && (get_int_val_from_date_time_str(pos, &tmp_int1) == 0) ) {
		correction.tm_mon  = tmp_int1 - 1;
	} else {
		return -2;
	}

	// Day
	pos = strtok(NULL, "\0");
	if (pos && (get_int_val_from_date_time_str(pos, &tmp_int1) == 0) ) {
		correction.tm_mday = tmp_int1;
	} else {
		return -3;
	}

	// Hour
	pos = strtok(time_format, ":");
	if (pos && (get_int_val_from_date_time_str(pos, &tmp_int1) == 0) ) {
		correction.tm_hour = tmp_int1;
	} else {
		return -3;
	}

	// Min
	pos = strtok(NULL, ":");
	if (pos && (get_int_val_from_date_time_str(pos, &tmp_int1) == 0) ) {
		correction.tm_min  = tmp_int1;
	} else {
		return -3;
	}

	// Sec
	pos = strtok(NULL, "\0");
	if (pos && (get_int_val_from_date_time_str(pos, &tmp_int1) == 0) ) {
		correction.tm_sec  = tmp_int1;
	} else {
		return -3;
	}

	if ((tmp_int1 = is_date_time_valid(&correction)) != pdTRUE) {
        return tmp_int1;
    }
	
	/* Season flag, such as daylight saving time */
	if (daylight) {
		correction.tm_isdst = 1;
	} else {
		correction.tm_isdst = -1;
	}

#ifdef __TIME64__
	__time64_t  corrtime, now;

	da16x_mktime64(&correction, &corrtime);
	da16x_time64(&corrtime, &now);	/* set time UTC */
#else
	time_t  corrtime, now;

	corrtime = da16x_mktime32(&correction);;
	now = da16x_time32(&corrtime);	/* set time UTC */
#endif /* __TIME64__ */

	return 0;
}

		
void cmd_time(int argc, char *argv[])
{
	long timezone;
#ifdef __TIME64__
	__time64_t now;
#else
	time_t now;
#endif /* __TIME64__ */
	struct tm *ts;
	char buf[80];
	int timezone_offset[2] = {0, 0};
	int abs_minute, abs_hour;

	memset(buf, 0, 80);

	if (argc == 3) {
		if (strcasecmp(argv[1], TIMEZONE) == 0) {
			/* Set time zone */
			int multiplier = (argv[2][0] == '-')?(-1):(1);
			
			timezone_offset[0] = atoi(strtok(argv[2], ":"));
			if (timezone_offset[0] < 0) timezone_offset[0] = (-1) * timezone_offset[0];
			
			timezone_offset[1] = atoi(strtok(NULL, "\0"));

			if (timezone_offset[0] == 0 && timezone_offset[1] == 0) {
				timezone = 0;
			} else {
				timezone = (multiplier) * (3600 * (long)timezone_offset[0])
						  +(multiplier) * ((long)(timezone_offset[1] * 60));
			}

			set_timezone_to_rtm(timezone);	/* to rtm */
			da16x_SetTzoff(timezone);
			set_time_zone(timezone);		/* to nvram */
		} else {
			goto help;
		}
	} else if (argc == 2) {
		if (strcasecmp(argv[1], BOOTTIME) == 0) {
			/* print bootting time */
#ifdef __TIME64__
			__boottime(&now);
			ts = (struct tm *)da16x_localtime64(&now);
#else
			now = __boottime();
			ts = da16x_localtime32(&now);
#endif /* __TIME64__ */
			da16x_strftime(buf, sizeof (buf), "%Y.%m.%d - %H:%M:%S", ts);

			PRINTF("Boottime: %s\n", buf);
		} else if (strcasecmp(argv[1], UPTIME) == 0) {
#ifdef __TIME64__
			__time64_t uptime = 0;
			__uptime(&uptime);
#else
			time_t uptime;
			uptime = __uptime();
#endif /* __TIME64__ */

			PRINTF("Uptime: %u days %02u:%02u.%02u\n",
				   (unsigned long)uptime / (24 * 3600),
				   (unsigned long)uptime % (24 * 3600) / 3600,
				   (unsigned long)uptime % (3600) / 60,
				   (unsigned long)uptime % 60);
		} else if (strcasecmp(argv[1], "ZONE") == 0) {
			/* print time zone */
			abs_hour = timezone = da16x_Tzoff();
			if (timezone < 0 ) {
				abs_minute = -((da16x_Tzoff() % 3600) / 60);
				abs_hour *= (-1);
			} else {
				abs_minute = ((da16x_Tzoff() % 3600) / 60);
			}
			PRINTF("Time Zone %s%02d:%02d\n",
				   (timezone < 0)?"-":"+",
				   abs_hour / 3600,
				   abs_minute);
		} else {
			goto help;
		}
	} else if (argc == 4 || argc == 5) {
		/* time set [YYYY-MM-DD] [hh:mm:ss] dst*/
		if (strcasecmp(argv[1], SETTIME) == 0) {
			int daylight = 0;

			/* Season flag, such as daylight saving time */
			if (argc == 5 && strcasecmp(argv[4], DAYLIGHT) == 0) {
				daylight = 1;
			}

			if (cmd_set_time(argv[2], argv[3], daylight)) {
				goto help;
			}

#ifdef __TIME64__
			da16x_time64(NULL, &now);
			ts = (struct tm *)da16x_localtime64(&now);
#else
			now = da16x_time32(NULL);
			ts = da16x_localtime32(&now);
#endif /* __TIME64__ */

			da16x_strftime (buf, sizeof (buf), "%Y.%m.%d %H:%M:%S", ts);
			abs_hour = timezone = da16x_Tzoff();
			if (timezone < 0 ) {
				abs_minute = -((da16x_Tzoff() % 3600) / 60);
				abs_hour *= (-1);
			} else {
				abs_minute = ((da16x_Tzoff() % 3600) / 60);
			}

			PRINTF("SetTime: %s (GMT %s%02d:%02d)\n",
				   buf,
				   (timezone < 0)?"-":"+",
				   abs_hour / 3600,
				   abs_minute);
		} else {
			goto help;
		}
	} else {
		/* current time */
#ifdef __TIME64__
		da16x_time64(NULL, &now);
		ts = (struct tm *)da16x_localtime64(&now);
#else
		now = da16x_time32(NULL);
		ts = da16x_localtime32(&now);
#endif /* __TIME64__ */

		da16x_strftime(buf, sizeof (buf), "%Y.%m.%d %H:%M:%S", ts);
		abs_hour = timezone = da16x_Tzoff();

		if (timezone < 0 ) {
			abs_minute = -((da16x_Tzoff() % 3600) / 60);
			abs_hour *= (-1);
		} else {
			abs_minute = ((da16x_Tzoff() % 3600) / 60);
		}
		PRINTF("- Current Time : %s (GMT %s%02d:%02d)\n",
			   buf,
			   (timezone < 0)?"-":"+",
			   abs_hour / 3600,
			   abs_minute);
	}

	return;
help:
	PRINTF("\nUsage: time [option]\n"
		   "\t\t: set [YYYY-MM-DD] [hh:mm:ss]\n"
		   "\t\t: zone [-hh:mm|+hh:mm]\n"
		   "\t\t: boot\n"
		   "\t\t: uptime\n"
		   "\t\t: help\n"
		  );

}


#ifdef __SUPPORT_SIGMA_TEST__	//////////////////////////////////////////////
/*##############################################
	WFA certificate SIGMA test features
  ##############################################*/

#include "da16x_sigma_test.h"

extern void sigma_throughput_test(char *input_str, int idx, int test_type);
extern SIGMA_TEST_PROF sigma_profile[MAX_PROF];

extern int sid_table[3];

void cmd_sigma_set_profile(int argc, char *argv[])
{
	int curr_stream;

	if (argc != 15) {
		PRINTF("Profile argument error !!!\n");
		return;
	}

	if (sigma_profile[0].run_flag == SIGMA_TEST_STOP) {
		curr_stream = 0;
	} else if (sigma_profile[1].run_flag == SIGMA_TEST_STOP) {
		curr_stream = 1;
	} else if (sigma_profile[2].run_flag == SIGMA_TEST_STOP) {
		curr_stream = 2;
	} else {
		PRINTF("Unexpected profile setting (check stream sequence) !!!\n");
	}

	sigma_profile[curr_stream].run_flag		= SIGMA_TEST_RUN;
	sigma_profile[curr_stream].func_name	= atoi(argv[1]);
	sigma_profile[curr_stream].stream_id	= atoi(argv[2]);
	sigma_profile[curr_stream].profile		= atoi(argv[3]);
	sigma_profile[curr_stream].direction	= atoi(argv[4]);
	//sigma_prof->d_ipaddr = htoi(argv[4);
	strncpy(sigma_profile[curr_stream].d_ipaddr, argv[5], IPV4_ADDRESS_STRING_LEN);
	sigma_profile[curr_stream].d_port		= atoi(argv[6]);

	//sigma_prof->s_ipaddr = htoi(argv[6];
	strncpy(sigma_profile[curr_stream].s_ipaddr, argv[7], IPV4_ADDRESS_STRING_LEN);
	sigma_profile[curr_stream].s_port		= atoi(argv[8]);
	sigma_profile[curr_stream].rate			= atoi(argv[9]);
	sigma_profile[curr_stream].duration		= atoi(argv[10]);
	sigma_profile[curr_stream].pkt_sz		= atoi(argv[11]);
	sigma_profile[curr_stream].traffic_class = atoi(argv[12]);
	sigma_profile[curr_stream].start_delay	= atoi(argv[13]);
	sigma_profile[curr_stream].max_cnt		= atoi(argv[14]);

	sid_table[curr_stream] = sigma_profile[curr_stream].stream_id;
}

void cmd_sigma_test(int argc, char *argv[])
{
	int i;
	int p_idx1, p_idx2, p_idx3;

#ifdef	__TEST_WITH_LOCAL_SVR__
	if (argc < 4) {
		PRINTF("sigma [start|stop] pid test_pattern\n");
		return;
	}

	if (argc == 4) {
		sigma_throughput_test(argv[1], atoi(argv[2]), atoi(argv[3]));
	}

#else

	if (argc < 3) {
		PRINTF("sigma [start|stop] pid\n");
		return;
	}

	if (argc == 3) {
		for (i = 0; i < MAX_PROF; i++) {
			if (sid_table[i] == atoi(argv[2])) {
				p_idx1 = i;
			}
		}

		sigma_throughput_test(argv[1], p_idx1,  0);
	} else if (argc == 4) {
		for (i = 0; i < MAX_PROF; i++) {
			if (sid_table[i] == atoi(argv[2])) {
				p_idx1 = i;
			}

			if (sid_table[i] == atoi(argv[3])) {
				p_idx2 = i;
			}
		}

		sigma_throughput_test(argv[1], p_idx1, 0);
		sigma_throughput_test(argv[1], p_idx2, 0);
	} else if (argc == 5) {
		for (i = 0; i < MAX_PROF; i++) {
			if (sid_table[i] == atoi(argv[2])) {
				p_idx1 = i;
			}

			if (sid_table[i] == atoi(argv[3])) {
				p_idx2 = i;
			}

			if (sid_table[i] == atoi(argv[4])) {
				p_idx3 = i;
			}
		}

		sigma_throughput_test(argv[1], p_idx1,  0);
		sigma_throughput_test(argv[1], p_idx2,  0);
		sigma_throughput_test(argv[1], p_idx3,  0);
	}

#endif	/* __TEST_WITH_LOCAL_SVR__ */
}
#endif	/* __SUPPORT_SIGMA_TEST__ */	////////////////////////////////////

/*##############################################
	mdns command methods
  ##############################################*/

/******************************************************************************
 *  cmd_mdns_start( )
 *
 *  Purpose :   start the mdns service
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_mdns_start(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_start_mdns_service_cmd(argc, argv);
	if (ret != 0) {
		PRINTF("[%s] ERR - Not able to start mdns service(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_mdns_stop( )
 *
 *  Purpose :   start the mdns service
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_mdns_stop(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	int ret = 0;

	ret = zeroconf_stop_mdns_service();
	if (ret) {
		PRINTF("[%s] ERR - Not able to stop mdns service(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_mdns_change_host( )
 *
 *  Purpose :   change the mdns host name
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_mdns_change_host(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_change_mdns_hostname_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to change host name of mdns server(0x%x)\r\n",
			   __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_mdns_lookup( )
 *
 *  Purpose :   mdns lookup
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_mdns_lookup(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_lookup_mdns_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to lookup mdns server(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_da16x_zeroconf_print_state( )
 *
 *  Purpose :   print the mdns thread states
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_zeroconf_state(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_print_status_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to print status of zeroconf(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_zeroconf_cache( )
 *
 *  Purpose :   print the full mdns cache
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_zeroconf_cache(int argc, char *argv[])
{
	zeroconf_print_cache_cmd(argc, argv);
}

/*##############################################
  xmdns command methods
##############################################*/

/******************************************************************************
 *  cmd_xmdns_init( )
 *
 *  Purpose :   start the xmdns service
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_xmdns_start(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_start_xmdns_service_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to start xmdns service(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/******************************************************************************
 *  cmd_xmdns_stop( )
 *
 *  Purpose :   stop the xmdns service
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_xmdns_stop(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	int ret = 0;

	ret = zeroconf_stop_xmdns_service();
	if (ret) {
		PRINTF("[%s] ERR - Not able to stop xmdns service(0x%x)\n", __func__, -ret);
	}

	return ;
}


/******************************************************************************
 *  cmd_xmdns_change_host( )
 *
 *  Purpose :   change the xmdns host name
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_xmdns_change_host(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_change_xmdns_hostname_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to change host name of xmdns service(0x%x)\r\n",
			   __func__, -ret);
	}

	return ;
}


/******************************************************************************
 *  cmd_xmdns_lookup( )
 *
 *  Purpose :   xmdns lookup
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_xmdns_lookup(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_lookup_xmdns_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to lookup xmdns service(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

/*##############################################
  dns-sd command methods
##############################################*/

/******************************************************************************
 *  cmd_dns_sd ()
 *
 *  Purpose :   handles different dns-sd commands
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void cmd_dns_sd(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_exec_dns_sd_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to execute dns-sd service(0x%x)\n", __func__, -ret);
	}

	return ;
}

void cmd_mdns_config(int argc, char *argv[])
{
	int ret = 0;

	ret = zeroconf_set_mdns_config_cmd(argc, argv);
	if (ret) {
		PRINTF("[%s] ERR - Not able to setup mDNS configuration(0x%x)\r\n", __func__, -ret);
	}

	return ;
}

void cmd_nslookup(int argc, char *argv[])
{
	LWIP_UNUSED_ARG(argv);

	if (argc == 1) {
		if (nslookup_support_flag == pdTRUE) {
			PRINTF("\r\nUsage: nslookup <domain>\n");
		}

		return;
	}

#ifdef LWIP_DNS
	ip_addr_t dns_ipaddr;
	char * domain_url = argv[1];

	err_t ret = dns_gethostbyname(domain_url, &dns_ipaddr, nslookup_dns_found, NULL);

	if (ERR_OK == ret) {
		if (dns_ipaddr.addr == 0) {
			PRINTF("DNS query failed. Domain url is invalid.\n");
		} else {
			PRINTF("Domain: %s\n\r", domain_url);
			PRINTF("IP: %s\n\r", ipaddr_ntoa(&dns_ipaddr));
		}
	} else if(ERR_INPROGRESS == ret) {
		;
	} else {
		PRINTF("DNS query fail: %d\n", ret);
	}
#endif	/*LWIP_DNS*/
}

void nslookup_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
	LWIP_UNUSED_ARG(arg);

	if (ipaddr != NULL && is_in_valid_ip_class(ipaddr_ntoa((ip4_addr_t *)&ipaddr))) {
		PRINTF("Domain: %s\n\r", hostname);
		PRINTF("IP: %s\n\r", ipaddr_ntoa(ipaddr));
	} else {
		PRINTF("DNS query failed.\n");
		return;
	}
}

/* arprequset, arpresponse, garpsend */
void cmd_arp_send(int argc, char *argv[])
{
	int status = pdPASS;

#ifdef __SUPPORT_MULTI_IP_IF__

	if (((argv[0][0] == 'a' && (argc == 2 || argc == 3)) || (argv[0][0] == 'g')))
#else
	if (   argv[1][0] == 'w'
			&& argv[1][1] == 'l'
			&& argv[1][2] == 'a'
			&& argv[1][3] == 'n'
			&& (argv[1][4] == '0' || argv[1][4] == '1')
			&& ((argv[0][0] == 'a' && (argc == 3 || argc == 4)) || (argv[0][0] == 'g') || (argv[0][0] == 'd')))
#endif /* __SUPPORT_MULTI_IP_IF__ */
	{
#ifndef __SUPPORT_MULTI_IP_IF__

#endif /* __SUPPORT_MULTI_IP_IF__ */

		if (argv[0][0] == 'g')
#ifdef __SUPPORT_MULTI_IP_IF__
		{
			int ch_ip = 0;

			if (argc == 2)
			{
				/* check ip confilt */
				ch_ip = ctoi(argv[1]);
			}

			/* GARP */
			status = garp_request(WLAN0_IFACE, ch_ip);
		}
		else if (strncmp(argv[0], "arpsend", 6) == 0 &&  argc == 2 && is_in_valid_ip_class(argv[1]))
		{
			/* ARP Request */
			ip_addr_t ipaddr;
			ipaddr_aton(argv[1], &ipaddr);
			status = arp_request(ipaddr, WLAN0_IFACE);
		}
		else if (strncmp(argv[0], "dhcp_arp", 8) == 0 && ( argc == 1 || (argc == 2 && is_in_valid_ip_class(argv[1]))))
		{
			/* DHCP ARP Request */
			status = dhcp_arp_request(WLAN0_IFACE, argc == 2 ? (ULONG)iptolong(argv[1]) : 0);
		}
		else if (strncmp(argv[0], "arpres", 6) == 0 && argc == 3 && is_in_valid_ip_class(argv[1]))
		{
			/* ARP Response*/
			status = arp_response(WLAN0_IFACE, argv[1],  argv[2]);
		}
#else
		{
			int ch_ip = 0;

			if (argc == 3)
			{
				/* check ip confilt */
				ch_ip = (int)ctoi(argv[2]);
			}

			/* GARP */
			status = garp_request((argv[1][4]) - 0x30, ch_ip);
		}
		else if (strncmp(argv[0], "arpsend", 6) == 0 &&  argc == 3 && is_in_valid_ip_class(argv[2]))
		{
			/* ARP Request */
			ip_addr_t ipaddr;
			ipaddr_aton(argv[2], &ipaddr);
			status = arp_request(ipaddr, (argv[1][4]) - 0x30);
		}
		else if (strncmp(argv[0], "dhcp_arp", 8) == 0 && (argc == 2 || (argc == 3 && is_in_valid_ip_class(argv[2]))))
		{
			/* DHCP ARP Request */
			status = dhcp_arp_request((argv[1][4]) - 0x30, argc == 3 ? (ULONG)iptolong(argv[2]) : 0);
		}
		else if (strncmp(argv[0], "arpres", 6) == 0 && argc == 4 && is_in_valid_ip_class(argv[2]))
		{
			/* ARP Response*/
			status = arp_response((argv[1][4]) - 0x30, argv[2],  argv[3]);
		}

#endif /* __SUPPORT_MULTI_IP_IF__ */
	}
	else     /* help */
	{
		PRINTF("\n%s "
#ifndef __SUPPORT_MULTI_IP_IF__
			   "[wlan0|wlan1]"
#endif /* __SUPPORT_MULTI_IP_IF__ */
			   " %s %s\n",
			   argv[0],							/* cmd */
			   argv[0][0] == 'g' ? "[Option]" : "[Dst IP Address]",		/* arp resquest */
			   strncmp(argv[0], "arpres", 6) == 0 ? "[Dst MAC Address]" : "" );	/* arp response */

		if (argv[0][0] == 'g')   /* garp */
		{
			PRINTF("\tOption = 0 : Normal garp\n"
				   "\tOption = 1 : Check IP conflict\n");
		}

		return;
	}

	if (status == pdPASS)
	{
		PRINTF("\n%s sent%s%s\n",
			   argv[0],
			   argv[0][0] == ('g') ? "" : " : ",
			   argv[0][0] == ('g') ? "" :
#ifdef __SUPPORT_MULTI_IP_IF__
			   argv[1]
#else
			   argv[2]
#endif /* __SUPPORT_MULTI_IP_IF__ */
			  );
	}
	else
	{
		PRINTF("\nERR : %s send\n", argv[0]);
	}
}

#ifdef __SUPPORT_SNTP_CLIENT__
void cmd_network_sntp(int argc, char *argv[])
{
	unsigned int status;

	if (sntp_support_flag == pdFALSE)
	{
		PRINTF("Doesn't support SNTP client !!!\n");
		return;
	}

	if (argc == 3)
	{
		unsigned int svr_addr_idx = 99;
	
		if (strcasecmp("ADDR", argv[1]) == 0) {
			svr_addr_idx = 0;
		}
		else if (strcasecmp("ADDR_2", argv[1]) == 0) {
			svr_addr_idx = 1;
		}
		else if (strcasecmp("ADDR_3", argv[1]) == 0) {
			svr_addr_idx = 2;
		}

		if (svr_addr_idx < 3)
		{
			if (strchr(argv[2], '.') != NULL)
			{		/* IPv4 */
				if (isVaildDomain(argv[2]) == 0 && is_in_valid_ip_class(argv[2]) == 0)
				{
					goto sntp_help;
				}
			}
#ifdef	__SUPPORT_IPV6__
			else if (strchr(argv[2], ':') != NULL)  	/* IPv6 */
			{
				ULONG   ipv6_dst[4];

				if (ipv6_support_flag == pdFALSE)
				{
					PRINTF("Doesn't support IPv6 !!!\n");
					return;
				}

				if (ParseIPv6ToLong(argv[2], ipv6_dst, NULL) == 0)
				{
					goto sntp_help;
				}
			}
#endif	/* __SUPPORT_IPV6__ */
			else
			{
					goto sntp_help;
			}

			status = (UINT)set_sntp_server((UCHAR *)argv[2], svr_addr_idx);
			PRINTF("%sSNTP Server %d: %s\n", status ? "":"Error: ", svr_addr_idx+1, (UCHAR *)argv[2]);
			return;
		}
		else if (strcasecmp("PERIOD", argv[1]) == 0)
		{
			unsigned int i = 0;

			for (i = 0 ; i < strlen(argv[2]) ; i++)
			{
				if (!ISDIGIT(argv[2][i]))
				{
					PRINTF("Error: SNTP period %s\n", argv[2]);
					return;
				}
			}

			i = ctoi(argv[2]);

			status = set_sntp_period((int)i);
			PRINTF("%sSNTP period %d\n", status ? "":"Error: ", i);
			return;
		}
		else // if (svr_addr_idx < 0)
		{
			goto sntp_help;
		}
	}
	else if (argc == 2)
	{
		if (strcasecmp("ENABLE", argv[1]) == 0)
		{
			if (sntp_get_use())
			{
				PRINTF("SNTP is Already Enabled\n");
			}
			else
			{
				sntp_set_use(pdTRUE);
				PRINTF("SNTP use: Enabled\n");
			}
			return;
		}
		else if (strcasecmp("SYNC", argv[1]) == 0)
		{
			if (sntp_get_use())
			{
				sntp_sync_now();
			}
			else
			{
				PRINTF("SNTP is not Enabled\n");
			}
			return;
		}
		else if (strcasecmp("DISABLE", argv[1]) == 0)
		{
			sntp_set_use(pdFALSE);
			PRINTF("SNTP use: Disabled\n");
			return;
		}
		else if (strcasecmp("STATUS", argv[1]) == 0)
		{
			char sntp_addr_str[32] = {0, };
			PRINTF("SNTP Status : %s\n", get_sntp_use() ? "Enabled" : "Disabled");
			get_sntp_server(sntp_addr_str, 0);
			PRINTF("\tServer_1 : %s\n", sntp_addr_str);
			get_sntp_server(sntp_addr_str, 1);
			PRINTF("\tServer_2 : %s\n", sntp_addr_str);
			get_sntp_server(sntp_addr_str, 2);
			PRINTF("\tServer_3 : %s\n", sntp_addr_str);
			PRINTF("\tPeriod   : %d sec.\n", get_sntp_period());
			return;
		}
		else if (strcasecmp("HELP", argv[1]) == 0)
		{
			goto sntp_help;
		}
		else
		{
			goto sntp_help;
		}
	}
	else
	{
		/* HELP */
sntp_help:
		PRINTF("Uasge: sntp [option]\n"
			   "\t\t: status\n"
			   "\t\t: enable | disable\n"
			   "\t\t: addr [server]\n"
			   "\t\t: addr_2 [server]\n"
			   "\t\t: addr_3 [server]\n"
			   "\t\t: period [second]\n"
			   "\t\t: sync\n"
			   "\t\t: help\n");
	}
}
#endif	/*__SUPPORT_SNTP_CLIENT__*/


extern UINT	iperf_cli(UCHAR iface, UCHAR iperf_mode, struct IPERF_CONFIG *config);

void  cmd_iperf_cli(int argc, char *argv[])
{
	UCHAR	protocol_mode = IPERF_TCP;
	UCHAR	iperf_mode = IPERF_SERVER;
	UCHAR	iperf_interface = NONE_IFACE;
	struct IPERF_CONFIG config = { 0, };

	config.ip_ver = 4;
	config.TestTime = 10 * configTICK_RATE_HZ;		/* 10 Sec. */
	config.RxTimeOut = 0;
	config.Interval = 0;
	config.PacketSize = 0;
	config.WMM_Tos = 255;
	config.bandwidth = 0;
	config.bandwidth_format = 'A';
	config.port = CLIENT_DEST_PORT;
	config.sendnum = 0;
	config.pair = 0;
	config.network_pool = 0;						/* 0: iperf_pool  1: main pool, */
	config.window_size = 0;							/* 0: use default */
#ifdef __IPERF_PRINT_MIB__
	config.mib_flag = 0;							/* 0: use default */
#endif /* __IPERF_PRINT_MIB__ */
	config.term = 0;
	config.tcp_api_mode = TCP_API_MODE_DEFAULT;		/* 0: tcp api mode */

	if (   argc == 1
		|| (argc == 2 && (strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "help", 4) == 0)))
	{
		PRINTF("Usage:\t%s "
#ifndef __SUPPORT_MULTI_IP_IF__
			"-I [WLAN0|WLAN1] "
#endif // __SUPPORT_MULTI_IP_IF__
			"[-s|-c host][options]\n"
			"\t%s [-h] [-v]\n"
			"\nClient/Server:\n"
#ifndef __SUPPORT_MULTI_IP_IF__
			"\t-I      Interface [WLAN0|WLAN1]\n"
#endif // __SUPPORT_MULTI_IP_IF__
			"\t-i      seconds between periodic bandwidth reports \n"
			"\t-u      use UDP rather than TCP\n"
			"\t-p, #   server port to listen on/connect to\n"
			"\t-f, [kmKM]   format to report: Kbits, Mbits, KBytes, MBytes\n"
			"\t-d      finsh service\n"
			"\t\tex) %s -I [wlan0|wlan1] -d -c -u : udp clinet\n"
			"\t\t    %s -I [wlan0|wlan1] -d -c    : tcp clinet\n"
			"\t\t    %s -I [wlan0|wlan1] -d -u    : udp server\n"
			"\t\t    %s -I [wlan0|wlan1] -d       : tcp server\n",
			argv[0], argv[0], argv[0],argv[0],argv[0],argv[0]);

#ifdef	__SUPPORT_IPV6__

		if (ipv6_support_flag == pdTRUE) {
			PRINTF("\t-V Set the domain to IPv6\n");
		}

#endif	/* __SUPPORT_IPV6__ */

		PRINTF( "\nServer specific:\n"
				"\t-s      run in server mode\n"
				"\t-T  #   Rx Time Out Min:1 sec. 'F' Forever\n");

		PRINTF( "\nClient specific:\n"
				"\t-c      <host>   run in client mode, connecting to <host>\n"
				"\t-t  #   time in seconds to transmit for (default 10 secs)\n"
 				"\t-x  #   tcp API mode default:basic tcp(API) 1:Altcp 2:Socket \n"		
				"\t-y  #   Transmit delay, tick 1 ~ 100\n"
				"\t-l  #   PacketSize option (UDP default %d, IPv6 %d TCP 1000)\n"
				"\t-n  #   UDP Tx packet number\n"
				"\t-P, #   Pair Index (0,1,2)\n"
#ifdef __IPERF_BANDWIDTH__
				"\t-b  #M  Bandwidth to send at in Mbits/sec (debug only)\n"
#endif /* __IPERF_BANDWIDTH__ */
				"\t        (default Max, Step 1~100 Mbps)\n"
				"\t-O      use Main Packet Pool\n"
				,
				DEFAULT_IPV4_PACKET_SIZE, DEFAULT_IPV6_PACKET_SIZE);

		PRINTF( "\nMiscellaneous:\n"
#ifdef __IPERF_PRINT_MIB__
				"\t-m #    Print MIB info(debug only)\n"
				"\t\t1 counter reset\n"
				"\t\t2 counter retention\n"

#endif /* __IPERF_PRINT_MIB__ */
				"\t-h      print this message\n"

				"\t-v      print version\n");
		return;
	}

	/*	    "\t -V     Set the domain to IPv6\n" \ */

	/* Parse the command.  */
	for (int n = 0; n < argc; n++) {

		/* -d Server/Client */
		if (argv[n][0] == '-' && argv[n][1] == 'd') {
			config.term = pdTRUE;
			n++;
		}

		if (argv[n][0] == '-' && argv[n][1] == 'v') {
			PRINTF("\niPerf Version %s\n\n", IPERF_VERSION);
			return;
		}

#ifdef	__SUPPORT_IPV6__

		if (ipv6_support_flag == pdTRUE) {
			if (argv[n][0] == '-' && argv[n][1] == 'V') {
				/* IPv6 */
				config.ip_ver = 6;
				continue;
			}
		}

#endif	/* __SUPPORT_IPV6__ */

		/* -u UDP */
		if (argv[n][0] == '-' && argv[n][1] == 'u') {
			protocol_mode = IPERF_UDP; /* UDP */
		}

		/* -s Server */
		if (argv[n][0] == '-' && argv[n][1] == 's') {
			iperf_mode = IPERF_SERVER;
		}

		/* -c Client */
		if (argv[n][0] == '-' && argv[n][1] == 'c') {
			iperf_mode = IPERF_CLIENT;

			if (config.term == pdTRUE) {
				continue;
			}

			/* DEST IP Address */
			if (is_in_valid_ip_class(argv[n + 1]) && config.ip_ver == 4) {	/* check ip */
				config.ipaddr = (ULONG)iptolong(argv[n + 1]);
				n++;
				continue;
#ifdef	__SUPPORT_IPV6__
			} else if (config.ip_ver == 6) {
				int nPort;
				int bSuccess;

				if (ipv6_support_flag == pdFALSE) {
					PRINTF("Doesn't support IPv6 !!!\n");
					return;
				}

				bSuccess = ParseIPv6ToLong(argv[n + 1], config.ipv6addr, &nPort);

				if (!bSuccess) {
					PRINTF("Invalid IPv6 Address\n");
					return;
				}

				n++;
				continue;
#endif	/* __SUPPORT_IPV6__ */
			} else {
				PRINTF("\tERR: -c IPAddress\n");
				return;
			}
		}

		/* -n Number */
		if (argv[n][0] == '-' && argv[n][1] == 'n') {
			config.sendnum = ctoi(argv[n+1]);

			if (config.sendnum >= 1) {
				n++;
				continue;
			} else {
				PRINTF("\tERR: -n Number\n");
				return;
			}
		}

		/* -b bandwidth */
		if (argv[n][0] == '-' && argv[n][1] == 'b') {
#ifdef __IPERF_BANDWIDTH__

			for (int tt = 0; tt < strlen(argv[n + 1]); tt++) {
				if (isdigit((int)(argv[n + 1][tt])) == 0) {
					if (toupper(argv[n + 1][tt]) == 'M') {
						argv[n + 1][tt] = '\0';
					} else {
						PRINTF("\tERR: -b Number\n");
						return;
					}
				}
			}

			config.bandwidth = ctoi(argv[n + 1]);
			n++;
			continue;
#else
			PRINTF("\t'-b' option is not supported.\n");
			return;
#endif /* __IPERF_BANDWIDTH__ */
		}

		/* -f bandwidth */
		if (argv[n][0] == '-' && argv[n][1] == 'f') {

			char bformat = argv[n + 1][0];

			if (   strlen(argv[n + 1]) == 1
				&& (bformat == 'M' || bformat == 'K' || bformat == 'm' || bformat == 'k')) {

				config.bandwidth_format = (UCHAR)bformat;
			} else {
				PRINTF("\tERR: -f [M|K|m|k]\n");
				return;
			}

			n++;
			continue;
		}

		/* -w Number : TCP window size */
		if (argv[n][0] == '-' && argv[n][1] == 'w') {
			config.window_size = ctoi(argv[n + 1]);

			if (config.window_size >= 4 && config.window_size <= 64) {
				n++;
				continue;
			} else {
				PRINTF("\tERR: -w Number\n");
				return;
			}
		}

		/* -P Pair Number */
		if (argv[n][0] == '-' && argv[n][1] == 'P') {
			config.pair = (UCHAR)ctoi(argv[n + 1]);

			if (config.pair < IPERF_TCP_TX_MAX_PAIR) {	/* 0, 1, 2 */
				n++;
				continue;
			} else {
				PRINTF("\tERR: -P Pair Number\n");
				return;
			}
		}

		/* -p Port Number */
		if (argv[n][0] == '-' && argv[n][1] == 'p') {
			config.port = ctoi(argv[n + 1]);

			if (config.port >= 5001 && config.port < 32768) {	/* Local Port Range 32768~61000 */
				n++;
				continue;
			} else {
				PRINTF("\tERR: -p Port (5001~32767)\n");
				return;
			}
		}

		/* -t Time */
		if (argv[n][0] == '-' && argv[n][1] == 't') {
			if (config.TestTime > 0) {
				config.TestTime = (ctoi(argv[n + 1]) * configTICK_RATE_HZ);
				n++;
				continue;
			} else {
				PRINTF("\tERR: -t time\n");
				return;
			}
		}

		/* -t Rx Time out */
		if (argv[n][0] == '-' && argv[n][1] == 'T') {
			if (argv[n + 1][0]  == 'f' || argv[n + 1][0]  == 'F') {
				config.RxTimeOut = portMAX_DELAY;
				n++;
				continue;
			} else {
				int check_num = 0;

				for (unsigned int tt = 0; tt < strlen(argv[n + 1]); tt++) {
					if (isdigit((int)(argv[n + 1][tt])) == 0) {
						check_num = 1;
						break;
					}
				}

				if (check_num == 0) {
					config.RxTimeOut = (ctoi(argv[n + 1]) * configTICK_RATE_HZ);
					n++;
					continue;
				} else {
					PRINTF("\tERR: -T time(sec.) or F\n");
					return;
				}
			}
		}

		/* -l PacketSize option */
		if (argv[n][0] == '-' && argv[n][1] == 'l') {
			config.PacketSize = ctoi(argv[n + 1]);

			if (config.PacketSize > 0) {
				n++;
				continue;
			} else {
				PRINTF("\tERR: -l size\n");
				return;
			}
		}

		/* -i Interval */
		if (argv[n][0] == '-' && argv[n][1] == 'i') {
			if (ctoi(argv[n + 1]) > 0) {
				config.Interval = ctoi(argv[n + 1]) * configTICK_RATE_HZ;
				n++;
				continue;
			} else {
				PRINTF("\tERR: -i interval\n");
				return;
			}
		}

		/* -I Interface */
		if (argv[n][0] == '-' && argv[n][1] == 'I') {
			if (strcasecmp(argv[n + 1], "WLAN0") == 0) {
				iperf_interface = WLAN0_IFACE;
			} else if (strcasecmp(argv[n + 1], "WLAN1") == 0) {
				iperf_interface = WLAN1_IFACE;
			} else {
				PRINTF("\tERR: -I interface\n");
				return;
			}

			n++;
			continue;
		}

		/* -S WMM TOS option */
		if (argv[n][0] == '-' && argv[n][1] == 'S') {
			config.WMM_Tos = (UCHAR)ctoi(argv[n + 1]);
			n++;
			continue;
		}

		/* -O iPerf network packet pool */
		if (argv[n][0] == '-' && argv[n][1] == 'O') {
			config.network_pool = 1;
		}

#ifdef __IPERF_PRINT_MIB__

		/* -m : mib */
		if (argv[n][0] == '-' && argv[n][1] == 'm') {
			for (int tt = 0; tt < strlen(argv[n + 1]); tt++) {
				if (isdigit((int)(argv[n + 1][tt])) == 0) {
					PRINTF("\tERR: -m %s\n", argv[n+1]);
					return;
				}
			}

			config.mib_flag = ctoi(argv[n + 1]);

			n++;
			continue;
		}

#endif /* __IPERF_PRINT_MIB__ */

		if (argv[n][0] == '-' && argv[n][1] == 'y') {
			config.transmit_rate = ctoi(argv[n + 1]);

			if (config.transmit_rate > 0) {
				n++;
				continue;
			} else {
				PRINTF("\tERR: -y size\n");
				return;
			}
		}

		if (argv[n][0] == '-' && argv[n][1] == 'x') {
			config.tcp_api_mode = ctoi(argv[n+1]);

			if (config.tcp_api_mode < TCP_API_MODE_MAX) {
				n++;
				continue;
			} else {
				PRINTF("\tERR: -x mode(0|1|2)\n");
				return;
			}
		}

	}

	if (iperf_interface == NONE_IFACE) {
		if (iperf_mode == IPERF_CLIENT) {
			if (isvalidIPsubnetInterface((long)config.ipaddr, WLAN1_IFACE)) {
				iperf_interface = WLAN1_IFACE;
			} else {
				iperf_interface = WLAN0_IFACE;
			}
		} else {	/* IPERF_SERVER Mode Inteface */
			PRINTF("\tERR: -I Interface\n");
			return;
		}
	}

	switch (protocol_mode)
	{
		case IPERF_TCP:
			if (iperf_mode == IPERF_CLIENT) {
				iperf_mode = IPERF_CLIENT_TCP;
			} else {
				iperf_mode = IPERF_SERVER_TCP;
			}

			break;

		case IPERF_UDP:
			if (iperf_mode == IPERF_CLIENT) {
				iperf_mode = IPERF_CLIENT_UDP;
			} else {
				iperf_mode = IPERF_SERVER_UDP;
			}

			break;
	}

	iperf_cli(iperf_interface, iperf_mode, &config);
}

void cmd_mqtt_client_help(void)
{
	PRINTF("- mqtt_client\n\n");
	PRINTF("  Usage : mqtt_client [option]\n");
	PRINTF("    option\n");
	PRINTF("    <start>\t\t    : start mqtt_client\n");
	PRINTF("    <stop>\t\t    : stop mqtt_client\n");
	PRINTF("    <check>\t\t    : shows mqtt_client connection status\n");
	PRINTF("    <unsub> <topic>\t    : unsubscribe from the topic \n");
	PRINTF("    <-m> <msg> [<topic>]    : publish <msg> w/ <topic> if specified \n");
	PRINTF("    <-l>\t\t    : publish large <msg>\n");
}

#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
static char* c0_cmd[] = {"mqtt_config", "clean_session", "0"};
static char* c1_cmd[] = {"mqtt_config", "clean_session", "1"};
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
void cmd_mqtt_client(int argc, char *argv[])
{
	int res_val;
	PRINTF("\r\n");
	
	if (strcmp(argv[0], "mqtt_client") == 0 || strcmp(argv[0], "mqtt_sub") == 0) {		
		if (strcmp(argv[1], "start") == 0) {
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
				if (argc == 3) { 
					if (strcmp(argv[2], "c0") == 0) {		
						mqtt_client_config(3, c0_cmd);
					} else if (strcmp(argv[2], "c1") == 0) {
						mqtt_client_config(3, c1_cmd);
					}
				}
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

			if (mqtt_client_is_running() == TRUE) {
				mqtt_client_force_stop();
				mqtt_client_stop_sub();
			}
			
			mqtt_client_start_sub();
		} else if (strcmp(argv[1], "stop") == 0) {
			if (mqtt_client_is_running() == TRUE) {
				mqtt_client_force_stop();
				mqtt_client_stop_sub();
			} else {
				MQTT_DBG_PRINT("no mqtt_client to stop\n");
			}
		} else if (strcmp(argv[1], "unsub") == 0) {
			res_val = mqtt_client_unsub_topic(argv[2]);

			if (res_val == MOSQ_ERR_SUCCESS) {
				MQTT_DBG_PRINT("Unsubscribing success !\n");
			} else if (res_val == MOSQ_ERR_NO_CONN) {
				MQTT_DBG_PRINT("mqtt_client should be running\n");
			} else {
				MQTT_DBG_ERR("err!, check ret=%d\n", res_val);
			}			
		} else if (strcmp(argv[1], "check") == 0) {
			PRINTF("%s\n", mqtt_client_check_sub_conn() ? "Connected" : "Not Connected");
		} else if (strcmp(argv[1], "-m") == 0) {
			if (argc == 4) {
				if (strlen(argv[3]) <= 0 || strlen(argv[3]) > MQTT_TOPIC_MAX_LEN) {
					PRINTF(RED_COLOR "Topic length error (max_len=%d)\r\n" CLEAR_COLOR, MQTT_TOPIC_MAX_LEN);
				} else {
					res_val = mqtt_client_send_message(argv[3], argv[2]);
					if (res_val != 0) PRINTF("mqtt msg send failed (%d)\n", res_val);
				}			
			} else if (argc == 3) {
				res_val = mqtt_client_send_message(NULL, argv[2]);
				if (res_val != 0) PRINTF("mqtt msg send failed (%d)\n", res_val);
			} else {
				PRINTF("Invalid message input\n");
			}
		} else if (strcmp(argv[1], "-l") == 0) {
			char *buffer = NULL;
			int ret;

			extern int make_message(char *title, char *buf, size_t buflen);
			extern void *_mosquitto_calloc(size_t nmemb, size_t size);

			buffer = _mosquitto_calloc(MQTT_MSG_MAX_LEN + 1, sizeof(char));
			if (buffer == NULL) {
				PRINTF("[%s] Failed to alloc memory to write certificate\n", __func__);
				return;
			}

			ret = make_message("MQTT Publisher message", buffer, MQTT_MSG_MAX_LEN + 1);
			PRINTF("\n");

			if (ret > 0) {
				res_val = mqtt_client_send_message(NULL, buffer);
				if (res_val != 0) PRINTF("mqtt msg send failed (%d)\n", res_val);
			} else {
				PRINTF("Invalid message input\n");
			}

			vPortFree(buffer);
		} else {
			cmd_mqtt_client_help();
		}
	} else if (strcmp(argv[0], "mqtt_config") == 0) {
		mqtt_client_config(argc, argv);
	} else {
		PRINTF("Invalid command\n");
	}
}

#if defined ( __USER_DHCP_HOSTNAME__ )
void cmd_dhcp_hostname(int argc, char *argv[])
{
	char	name_str[DHCP_HOSTNAME_MAX_LEN + 1];
	int status;

	if (argc == 1) {
dhcp_hostname_help :
		PRINTF("- dhcp_hostname\n\n");
		PRINTF("  Usage : dhcp_hostname [get|set|del] [hostname]\n");
		PRINTF("    option\n");
		PRINTF("    <get>\t\t    : get saved user DHCP-client hostname\n");
		PRINTF("    <set> <hostname>\t    : set new user DHCP-client hostname (len <= 32)\n");
		PRINTF("    <del>\t\t    : delete saved DHCP-client hostname\n");
	} else if (argc == 2) {
		if (strcmp(argv[1], "get") == 0) {
			memset(name_str, 0, (DHCP_HOSTNAME_MAX_LEN+1));

			if ((status = da16x_get_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, name_str)) != CC_SUCCESS) {
				PRINTF("No saved User DHCP-client hostname ...\n", status);
			} else {
				PRINTF(" > User DHCP-client hostname = [%s]\n", name_str);
			}
		} else if (strcmp(argv[1], "del") == 0) {

			if ((status = da16x_set_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, "")) != CC_SUCCESS) {
				PRINTF("Failed to delete User DHCP-client hostname (%d) !!!\n", status);
			}
		} else {
			goto dhcp_hostname_help;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "set") == 0) {
			if (strlen(argv[2]) > DHCP_HOSTNAME_MAX_LEN) {
				PRINTF("Too long user DHCP-client hostname <%s> !!!\n", argv[2]);
				goto dhcp_hostname_help;
			}

			if ((status = da16x_set_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, argv[2])) != CC_SUCCESS) {
				PRINTF("Failed to save user DHCP-client hostname (%d) !!!\n", status);
			}
		} else {
			goto dhcp_hostname_help;
		}
	} else {
		goto dhcp_hostname_help;
	}
}
#endif	// __USER_DHCP_HOSTNAME__

#endif	/* XIP_CACHE_BOOT */

/* EOF */
