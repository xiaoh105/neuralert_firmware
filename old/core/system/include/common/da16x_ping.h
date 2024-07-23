/**
 ****************************************************************************************
 *
 * @file da16x_ping.h
 *
 * @brief Define for PING Client
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


#ifndef	__DA16X_PING_H__
#define	__DA16X_PING_H__

#include "sdk_type.h"

#include "da16x_system.h"
#include "iface_defs.h"

#include "da16x_network_main.h"
#include "da16x_dns_client.h"
#include "da16x_arp.h"

#include "ip_addr.h"

#undef DA16X_PING_DEBUG

#ifdef DA16X_PING_DEBUG
  #define PING_DEBUG_PRINT(...)	PRINTF(__VA_ARGS__)
#else
  #define PING_DEBUG_PRINT(...)
#endif	/*DA16X_PING_DEBUG*/

#define PING_DATA		"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-"
#define PING_DATA_LEN		27 /* (sizeof(PING_DATA)-1) */
#define PING_DOMAIN_LEN		64

#ifdef	__SUPPORT_SIGMA_TEST__
  #define MAX_PING_SIZE		10000
#else
  #define MAX_PING_SIZE		2048
#endif	/* __SUPPORT_SIGMA_TEST__ */

#define DEFAULT_PING_SIZE	32
#define DEFAULT_PING_WAIT	4000 /* Default time-out is 4000 (4 seconds). */
#define DEFAULT_PING_COUNT	4

#define MIN_INTERVAL		10
#define DEFAULT_INTERVAL	1000 /* ms */

#define POOL_USE_SIZE		1480

/* Define transmit packet pool. */
#define	ICMP_PKT_PAYLOAD_SZ			1536
#define	ICMP_PKT_PAYLOAD_SZ_IPV6	1556

#define ICMP_PKT_POOL_SZ		((ICMP_PKT_PAYLOAD_SZ + sizeof(NX_PACKET)) * 8)
#define ICMP_PKT_POOL_SZ_IPV6	((ICMP_PKT_PAYLOAD_SZ_IPV6 + sizeof(NX_PACKET)) * 8)


void cmd_ping_client(int argc, char *argv[]);
void cmd_arping(int argc, char *argv[]);

struct cmd_ping_param{
	unsigned int	status;
	char	domain_str[PING_DOMAIN_LEN + 1];
	ip_addr_t ipaddr;
	unsigned int	count;
	unsigned int	len;
	unsigned int	wait;
	unsigned int	interval;
	int		ping_interface;
	unsigned int	dns_query_wait_option;

#ifdef	__SUPPORT_IPV6__
	int		is_v6;
	int		is_ipv6;
	unsigned long	ipv6_dst[4];
	unsigned long	ipv6_src[4];
#endif	/* __SUPPORT_IPV6__ */
};

#endif	/* __DA16X_PING_H__ */

/* EOF */
