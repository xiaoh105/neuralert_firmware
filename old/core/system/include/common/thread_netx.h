/**
 ****************************************************************************************
 *
 * @file thread_netx.h
 *
 * @brief NetX Scheduler
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

#ifndef	__THREAD_NETX_H__
#define	__THREAD_NETX_H__


#include "FreeRTOS.h"
#include "da16x_types.h"

#define NX_DNS_ENABLE_EXTENDED_RR_TYPES

#define DNS_WAIT		500
#define DNS_LOCAL_CACHE_SIZE	128
#define DNS_BUFFER_SIZE		200
#define DNS_RECORD_COUNT	10

#define DNS_QUERY_AAAA		"aaaa"	/* ipv6 network address */
#define DNS_QUERY_A		"a"	/* network address */
#define DNS_QUERY_SRV		"srv"	/* Location of services */
#define DNS_QUERY_ALL		"all"	/* all query */
#define DNS_QUERY_MX		"mx"	/* mail exchanger */
#define DNS_QUERY_NS		"ns"	/* name server */
#define DNS_QUERY_SOA		"soa"	/* Start of authority */
#define DNS_QUERY_HINFO		"hinfo"	/* host info */
#define DNS_QUERY_TXT		"txt"	/* txt �� */
#define DNS_QUERY_PTR		"ptr"	/* Reverse DNS query */
#define DNS_QUERY_CNAME		"cname"	/* Canonical name */

#define PING_DATA		"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-"
#define PING_DATA_LEN		27 /* (sizeof(PING_DATA)-1) */
#define PING_DOMAIN_LEN		64
//#undef	MAX_PING_SIZE
#ifdef	__SUPPORT_SIGMA_TEST__
#define MAX_PING_SIZE		10000
#else
#define MAX_PING_SIZE		2048
#endif	/* __SUPPORT_SIGMA_TEST__ */
#define NET_INFO_STR_LEN	18
#ifdef	__SUPPORT_IPV6__
#define NET_INFO_LONG_STR_LEN	60
#endif

#define DEFAULT_PING_SIZE	32
#define DEFAULT_PING_WAIT	4000 /* Default time-out is 4000 (4 seconds). */
#define DEFAULT_PING_COUNT	4
#define MIN_INTERVAL		10
#define DEFAULT_INTERVAL	1000 /* ms */
#define	MICROSECOND		1000000 /* us */

#define POOL_USE_SIZE		1480

/* Define transmit packet pool. */
#define	ICMP_PKT_PAYLOAD_SZ		1536
#define	ICMP_PKT_PAYLOAD_SZ_IPV6	1556
#define ICMP_PKT_POOL_SZ		((ICMP_PKT_PAYLOAD_SZ+sizeof(NX_PACKET)) * 8)		/* (1536+64)*8 = 12800 */
#define ICMP_PKT_POOL_SZ_IPV6		((ICMP_PKT_PAYLOAD_SZ_IPV6+sizeof(NX_PACKET)) * 8)	/* (1556+64)*8 = 12960 */

#define DPM_ARP_TABLE_SIZE	(sizeof(NX_ARP) * NX_ARP_TABLE_DPM_SIZE)
#define DHCPV6_IANA_ID		0xC0DEDBAD

#define	DA16X_PKT_SIZE		640

#if defined ( __SUPPORT_CON_EASY__ )
  #define	DA16X_MAX_PACKETS	54 /* 64 -->54	   7040byte reduce( 704 * 10) */
#elif defined ( __SMALL_SYSTEM__ )
  #define	DA16X_MAX_PACKETS	54 /* 64 -->54	   7040byte reduce( 704 * 10) */
#else
  #define	DA16X_MAX_PACKETS	64	
#endif /*...*/

#define IP_PACKET_POOL_SIZE	(DA16X_PKT_SIZE + sizeof(NX_PACKET)) * DA16X_MAX_PACKETS		/* (640 + 64) * 64 = 45056 */

/* 1K is enouph for IP layer stack and thread stack */
#define DA16X_IP_STACK		((1 * KBYTE) + 384 )	// (1 * KBYTE)
#define DA16X_ARP_STACK		(512) /* (1*KBYTE) */
#define DA16X_BSD_SOCK_STACK	(2 * KBYTE)

int	get_netmode(int iface);
void	iface_updown(unsigned int iface, unsigned int flag);

unsigned int	is_dhcp_server_running(void);
extern void	*calc_thread_netx(void *pointer);
extern unsigned int	wlaninit(void);

extern void	set_supp_ip_str(char *src_str);
extern int	check_net_init(int iface);
extern int	check_net_ip_status(int iface);
extern unsigned int	chk_network_ready(UCHAR iface);
extern int	isvalidIPsubnetInterface(long ip, int iface);

#ifdef __SUPPORT_ASD__
struct antenna_info {
	int port;
	int *rssi;
	int gather_index;
	int max_rssi;
	int min_rssi;
	int avg_rssi;
	int stddev;
	int num_stay; //number of gathering without switching (stddev high , similar result)
	struct antenna_info *next;
};

extern int asd_enable(void);
extern int asd_disable(void);
extern int get_asd_enabled(void);
extern int get_asd_threshold(void);
extern int set_asd_threshold(int value);
extern int set_asd_threshold_permanent(int value);
extern int set_asd_debug(int value);
extern int get_asd_avg_min_diff(void);
extern int set_asd_avg_min_diff_permanent(int value);
extern int get_asd_stddev_max_diff(void);
extern int set_asd_stddev_max_diff_permanent(int value);
extern int fc80211_asd_ctl_gethering_bcn_rssi(int ctl);
#endif /* __SUPPORT_ASD__ */


/* For Customer's Soft-AP configuration */
#define	MAX_SSID_LEN		32
#define	MAX_PASSKEY_LEN		64
#define	MAX_IP_ADDR_LEN		16

#define	AP_OPEN_MODE		0
#define	AP_SECURITY_MODE	1

#define	IPADDR_DEFAULT		0
#define	IPADDR_CUSTOMER		1

#define	DHCPD_DEFAULT		0
#define	DHCPD_CUSTOMER		1


typedef struct _softap_config {
	int	customer_cfg_flag;	// MODE_ENABLE, MODE_DISABLE

	char	ssid_name[MAX_SSID_LEN+1];
	char	psk[MAX_PASSKEY_LEN+1];
	char	auth_type;		// AP_OPEN_MODE, AP_SECURITY_MODE
	char	country_code[4];

	int	customer_ip_address;	// IPADDR_DEFAULT, IPADDR_CUSTOMER
	char	ip_addr[MAX_IP_ADDR_LEN];
	char	subnet_mask[MAX_IP_ADDR_LEN];
	char	default_gw[MAX_IP_ADDR_LEN];
	char	dns_ip_addr[MAX_IP_ADDR_LEN];

	int	customer_dhcpd_flag;	// DHCPD_DEFAULT, DHCPD_CUSTOMER
	//int	dhcpd_ip_cnt;
	int	dhcpd_lease_time;
	char	dhcpd_start_ip[MAX_IP_ADDR_LEN];
	char	dhcpd_end_ip[MAX_IP_ADDR_LEN];
	char	dhcpd_dns_ip_addr[MAX_IP_ADDR_LEN];
} softap_config_t;

/* Application Names ... */
#define	NET_IFCONFIG	"ifconfig"
#define	NET_SNTP	"sntp_client"


#endif	/* __THREAD_NETX_H__ */

/* EOF */
