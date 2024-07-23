/**
 ****************************************************************************************
 *
 * @file zero_config.h
 *
 * @brief Defined Zeroconfig Service(mDNS, DNS-SD, and xmDNS)
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

#ifndef __ZERO_CONFIG_H__
#define __ZERO_CONFIG_H__

#include "sdk_type.h"
#include "app_errno.h"

#if defined (__SUPPORT_ZERO_CONFIG__)

#include "application.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/igmp.h"
#include "lwip/sockets.h"
#include "common_def.h"
#include "da16x_time.h"
#include "user_dpm.h"
#include "user_dpm_api.h"


#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"


/// Debug Log level(Information) for Zeroconfig service
#undef	ZEROCONF_ENABLE_DEBUG_INFO

/// Debug Log level(Error) for Zeroconfig service
#define	ZEROCONF_ENABLE_DEBUG_ERR

/// Debug Log level(Temporary) for Zeroconfig service
#undef	ZEROCONF_ENABLE_DEBUG_TEMP

/// mDNS IP address
#define ZEROCONF_MDNS_ADDR						"224.0.0.251"

/// xmDNS IP address
#define ZEROCONF_XMDNS_ADDR						"239.255.255.239"

/// mDNS port number
#define ZEROCONF_MDNS_PORT						5353

// Defined Resource record types
/// Resource record type - Host address
#define	ZEROCONF_MDNS_RR_TYPE_A					1
/// Resource record type - Domain name pointer
#define	ZEROCONF_MDNS_RR_TYPE_PTR				12
/// Resource record type - Host information
#define	ZEROCONF_MDNS_RR_TYPE_HINFO				13
/// Resource record type - Text strings
#define	ZEROCONF_MDNS_RR_TYPE_TXT				16
/// Resource record type - The location of services
#define	ZEROCONF_MDNS_RR_TYPE_SRV				33
/// Resource record type - IPv6 Host address
#define	ZEROCONF_MDNS_RR_TYPE_AAAA				28
/// Resource record type - NSEC
#define	ZEROCONF_MDNS_RR_TYPE_NSEC				47
/// Resource record type - Request for all records
#define	ZEROCONF_MDNS_RR_TYPE_ALL				255

// Defined name compression masks
/// Name compression mask - Compress mask
#define ZEROCONF_MDNS_COMPRESS_MASK				0xc0
/// Name compression mask - Compress value
#define ZEROCONF_MDNS_COMPRESS_VALUE			0xc0

// Defined resource record sections
/// Resource record section - Answer section
#define	ZEROCONF_MDNS_RR_ANSWER_SECTION			1
/// Resource record section - Authority section
#define	ZEROCONF_MDNS_RR_AUTHORITY_SECTION		2
/// Resource record section - Additional section
#define	ZEROCONF_MDNS_RR_ADDITIONAL_SECTION		3

// Defined name for Zeroconf service
/// Thread name for mDNS Probe
#define	ZEROCONF_MDNS_PROB_THREAD_NAME			"ZEROCONF_MDNS_PROB"
/// Thread name for DNS-SD Probe
#define	ZEROCONF_DNS_SD_PROB_THREAD_NAME		"ZEROCONF_DNS_SD_PROB"
/// Thread name for DNS-SD operation
#define	ZEROCONF_DNS_SD_EXEC_THREAD_NAME		"ZEROCONF_DNS_SD_EXEC"
/// Thread name for sending message of Zeroconfig service
#define	ZEROCONF_SEND_THREAD_NAME				"ZEROCONF_SEND"
/// Thread name for receiving message of Zeroconfig service
#define	ZEROCONF_RESP_THREAD_NAME				"ZEROCONF_RESP"
/// Thread name for restarting Zeroconfig service
#define	ZEROCONF_RESTART_THREAD_NAME			"ZEROCONF_RESTART"
/// Event name for Zeroconfig service
#define	ZEROCONF_EVT_NAME						"ZEROCONF_EVENT"
/// Name of DNS-SD information in DPM mode
#define	ZEROCONF_DPM_DNS_SD_INFO				"zc_dns_sd_info"

#define	ZEROCONF_WORD_SIZE						4
#define	ZEROCONF_CALC_WORDS_FROM_BYTES(sizeBytes)	\
	((sizeBytes) / ZEROCONF_WORD_SIZE + (((sizeBytes) & (ZEROCONF_WORD_SIZE - 1)) > 0))

/// Defined Stack size for Zeroconf service
/// Stack size to receive message of Zeroconfig service
#define	ZEROCONF_RESP_STACK_SIZE				ZEROCONF_CALC_WORDS_FROM_BYTES(1024 * 3)
/// Stack size to send message of Zeroconfig service
#define	ZEROCONF_SEND_STACK_SIZE				ZEROCONF_CALC_WORDS_FROM_BYTES(1024 * 2)
/// Stack size to process mDNS & DNS-SD probe
#define	ZEROCONF_PROB_STACK_SIZE				ZEROCONF_CALC_WORDS_FROM_BYTES(1024 * 3)
#if defined (__SUPPORT_DNS_SD__)
	/// Stack size to operate DNS-SD operation
	#define ZEROCONF_EXEC_STACK_SIZE			ZEROCONF_CALC_WORDS_FROM_BYTES(1024 * 2)
#endif // (__SUPPORT_DNS_SD__)
/// Stack size to restart Zeroconf service
#define	ZEROCONF_RESTART_STACK_SIZE				ZEROCONF_CALC_WORDS_FROM_BYTES(1024 * 2)

/// Defined event flag for Zeroconfig serivce
#define	ZEROCONF_EVT_ANY						0xFFF
/// Event flag to send message
#define	ZEROCONF_EVT_SEND						0x001
/// Event flag to remove packet
#define	ZEROCONF_EVT_REMOVE_PACKET				0x002
/// Event flag to remove mDNS probe message
#define	ZEROCONF_EVT_REMOVE_MDNS_PROB			0x004
/// Event flag to remove xmDNS probe message
#define	ZEROCONF_EVT_REMOVE_XMDNS_PROB			0x008
/// Event flag to remove DNS-SD probe message
#define	ZEROCONF_EVT_REMOVE_DNS_SD_PROB			0x010
/// Event flag to remove mDNS announcement message
#define	ZEROCONF_EVT_REMOVE_MDNS_ANNOUNCE		0x020
/// Event flag to remove xmDNS announcement message
#define	ZEROCONF_EVT_REMOVE_XMDNS_ANNOUNCE		0x080
/// Event flag to remove DNS-SD announcement message
#define	ZEROCONF_EVT_REMOVE_DNS_SD_ANNOUNCE		0x100

#define ZEROCONF_EVT_EMPTY                      0x200

/// Defined Zeroconfig service restart flag
/// Definition to restart Zeroconfig service
#define	ZEROCONF_RESTART_ALL_SERVICE			0xFF
/// Definition to restart mDNS service
#define	ZEROCONF_RESTART_MDNS_SERVICE			0x01
/// Definition to restart xmDNS service
#define	ZEROCONF_RESTART_XMDNS_SERVICE			0x02
/// Definition to restart DNS-SD service
#define	ZEROCONF_RESTART_DNS_SD_SERVICE			0x04

// Defined default variables
/// Prob timeout(unit of time: msec)
#define	ZEROCONF_DEF_PROB_TIMEOUT				250
/// Prob timeout(unit of time: msec)
#define	ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT	150
/// Prob timeout(unit of time: msec)
#define	ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT		1000
/// Domain name size for Zeroconfig service
#define	ZEROCONF_DOMAIN_NAME_SIZE				128
/// ARPA size for Zeroconfig service
#define ZEROCONF_ARPA_NAME_SIZE					128
/// mDNS header size
#define	ZEROCONF_MDNS_HEADER_SIZE				12
/// Default buffer size for Zeroconfig service
#define	ZEROCONF_DEF_BUF_SIZE					1024
/// Prob conflict count
#define	ZEROCONF_DEF_CONFLICT_COUNT				15
/// Prob count
#define	ZEROCONF_DEF_PROB_COUNT					3

// Related with NVRAM
/// mDNS hostname in NVRAM
#define ZEROCONF_MDNS_HOSTNAME					"MDNS_HOSTNAME"
/// mDNS hostname length in NVRAM
#define ZEROCONF_MDNS_HOSTNAME_LEN				32
/// mDNS service reg flag in NVRAM
#define ZEROCONF_MDNS_REG						"MDNS_REG"
/// DNS-SD service name in NVRAM
#define ZEROCONF_DNS_SD_SRV_NAME				"DNSSD_SRV_NAME"
/// DNS-SD service name length in NVRAM
#define ZEROCONF_DNS_SD_SRV_NAME_LEN			32
/// DNS-SD service protocol in NVRAM
#define ZEROCONF_DNS_SD_SRV_PROT				"DNSSD_SRV_PROT"
/// DNS-SD service protocol length in NVRAM
#define ZEROCONF_DNS_SD_SRV_PROT_LEN			32
/// DNS-SD service text record in NVRAM
#define ZEROCONF_DNS_SD_SRV_TXT					"DNSSD_SRV_TXT"
/// DNS-SD service text record length in NVRAM
#define ZEROCONF_DNS_SD_SRV_TXT_LEN				255
/// DNS-SD service port in NVRAM
#define ZEROCONF_DNS_SD_SRV_PORT				"DNSSD_SRV_PORT"
/// DNS-SD service reg flag in NVRAM
#define ZEROCONF_DNS_SD_SRV_REG					"DNSSD_SRV_REG"

/// Defined internal service types
typedef enum {
	UNKNOWN_SERVICE_TYPE		= 0,
	/// mDNS service
	MDNS_SERVICE_TYPE			= 1,
	/// xmDNS service
	XMDNS_SERVICE_TYPE			= 2
} zeroconf_service_type_t;

/// Defined response types
typedef enum {
	NONE_RESPONSE_TYPE			= 0,
	/// Multicast packet
	MULTICAST_RESPONSE_TYPE		= 1,
	/// Unicast packet
	UNICAST_RESPONSE_TYPE		= 2
} zeroconf_response_type_t;

/// Defined packet types
typedef enum {
	MDNS_NONE_TYPE				= 0,
	/// mDNS probe packet
	MDNS_PROB_TYPE				= 1,
	/// xmDNS probe packet
	XMDNS_PROB_TYPE				= 2,
	/// DNS-SD probe packet
	DNS_SD_PROB_TYPE			= 3,
	/// mDNS announcement packet
	MDNS_ANNOUNCE_TYPE			= 4,
	/// xmDNS announcement packet
	XMDNS_ANNOUNCE_TYPE			= 5,
	/// DNS-SD announcement packet
	DNS_SD_ANNOUNCE_TYPE		= 6,
} zeroconf_packet_type_t;

typedef enum {
	ZEROCONF_INIT = 0,
	ZEROCONF_ACTIVATED = 1,
	ZEROCONF_DEACTIVATED = 2,
	ZEROCONF_REQ_DEACTIVATED = 3,
} zeroconf_status_e;

typedef struct _zeroconf_proc_status_t {
	zeroconf_status_e status;
	TaskHandle_t task_handler;
} zeroconf_proc_status_t;

/// Defined status of Zeroconfig service
typedef struct _zeroconf_status_t {
	int socket_fd;

	zeroconf_status_e status;

	/// mDNS service status
	zeroconf_status_e mdns_service;
	/// DNS-SD service status
	zeroconf_status_e xmdns_service;
	/// xmDNS service status
	zeroconf_status_e dns_sd_service;

	/// mDNS prob task status
	zeroconf_proc_status_t mdns_probing_proc;
	/// DNS-SD prob task status
	zeroconf_proc_status_t dns_sd_probing_proc;
	/// Responder task status
	zeroconf_proc_status_t responder_proc;
	/// Sender task status
	zeroconf_proc_status_t sender_proc;
	/// Restarter task status
	zeroconf_proc_status_t restarter_proc;
	// DNS-SD task status
	zeroconf_proc_status_t dns_sd_exec_proc;

	/// Interface
	int iface;
	/// Flag to check IPv4
	unsigned int is_supported_ipv4;
	/// IPv4 address
	struct sockaddr_in ipv4_addr;

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
	/// Flag to check IPv6
	unsigned int is_supported_ipv6;
	/// IPv6 address
	struct sockaddr_in6 ipv6_addr;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
} zeroconf_status_t;

/// Defined configuration of delay
typedef struct _zeroconf_config_t {
	/// Prob delay
	unsigned int prob_timeout;
	/// Prob tie-breaker delay
	unsigned int prob_tie_breaker_timeout;
	/// Prob conflict delay
	unsigned int prob_conflict_timeout;
} zeroconf_config_t;

/// Defined host information
typedef struct _zeroconf_mdns_host_info_t {
	/// Hostname
	char hostname[ZEROCONF_DOMAIN_NAME_SIZE];
	/// ARPAv4 Hostname
	char arpa_v4_hostname[ZEROCONF_ARPA_NAME_SIZE];
#if defined (__SUPPORT_IPV6__)
	/// ARPAv6 Hostname
	char arpa_v6_hostname[ZEROCONF_ARPA_NAME_SIZE];
#endif // (__SUPPORT_IPV6__)
} zeroconf_mdns_host_info_t;

/// Defined DNS-SD Query
typedef struct _zeroconf_dns_sd_query_t {
	/// Query in bytes
	unsigned char query_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Length of query
	size_t query_bytes_len;
	/// Query key
	char query_key;
	/// Query type
	unsigned int q_type;
	/// G type to get address information for hostname
	unsigned int g_type;
} zeroconf_dns_sd_query_t;

/// Defined destination information
typedef struct _zeroconf_peer_info_t {
	/// Response type
	zeroconf_response_type_t type;
	/// Destination IP information
	struct sockaddr ip_addr;
} zeroconf_peer_info_t;

/// Defined local DNS-SD service to register
typedef struct _zeroconf_local_dns_sd_info_t {
	/// Name of service
	char *name;
	/// Service type
	char *type;
	/// Domain
	char *domain;
	/// Port number
	int port;
	/// Length of TXT record
	size_t txtLen;
	/// TXT record
	unsigned char *txtRecord;
	/// Query in bytes
	unsigned char q_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Data in bytes
	unsigned char t_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Length of query
	size_t q_bytes_len;
	/// Length of data
	size_t t_bytes_len;
} zeroconf_local_dns_sd_info_t;

/// Defined proxy DNS-SD service to register
typedef struct _zeroconf_proxy_dns_sd_info_t {
	/// Name of hostname
	char *h_name;
	/// IP address of hostname
	unsigned char h_ip[4];
	/// Name of service
	char *name;
	/// Service type
	char *type;
	/// Domain
	char *domain;
	/// Port number
	int port;
	/// Length of TXT record
	size_t txtLen;
	/// TXT record
	unsigned char *txtRecord;
	/// Query in bytes
	unsigned char q_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Data in bytes
	unsigned char t_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Host in bytes
	unsigned char h_bytes[ZEROCONF_DOMAIN_NAME_SIZE];
	/// Length of query
	size_t q_bytes_len;
	/// Length of data
	size_t t_bytes_len;
	/// Length of host
	size_t h_bytes_len;
} zeroconf_proxy_dns_sd_info_t;

/// Defined mDNS response
typedef struct _zeroconf_mdns_response_t {
	/// ID
	int id;
	/// Pointer of query
	unsigned char *query;
	/// Length of query
	size_t query_len;
	/// Service type
	int type;
	/// TTL
	LONG ttl;
	/// Timestamp
	unsigned long timestamp;
	int len;
	/// Pointer of data
	unsigned char *data;
	/// Length of data
	size_t data_len;
	/// Next mDNS response pointer
	struct _zeroconf_mdns_response_t *next;
} zeroconf_mdns_response_t;

/// Defined structure to send packet
typedef struct _zeroconf_pck_send_t {
	/// ID
	int id;
	/// Pointer of packet
	unsigned char *full_packet;
	/// Length of packet
	int len;
	/// Last sent time
	int last_sent_time;
	/// Sent count
	int sent_count;
	/// Max sent count
	int max_count;
	/// Interval
	int interval;
	/// Multi-factor
	int mult_factor;
	/// Service type
	int type;
	/// Destination information
	zeroconf_peer_info_t peer_info[2]; //0:ipv4, 1:ipv6
	/// Packet type
	int packet_type;
	/// Next packet pointer to send
	struct _zeroconf_pck_send_t *next;
} zeroconf_pck_send_t;

/// Defined query strings
typedef struct _zeroconf_query_string_t {
	/// Pointer of query
	unsigned char *query;
	/// Length of query
	size_t query_len;
	/// Next query pointer
	struct _zeroconf_query_string_t *next;
} zeroconf_query_string_t;

/// Defined Zeroconfig message cached
typedef struct _zeroconf_mdns_header_t {
	/// Count of received
	int	resp_count;
	/// Count of sent
	int	sent_count;
	/// Count of query
	int	query_count;
	/// Current ID pointer to send
	int	currrent_send_id_ptr;
	//// Current ID pointer of response
	int	currrent_resp_id_ptr;
} zeroconf_mdns_header_t;

/// Defined Question section structure
typedef struct _zeroconf_question_t {
	/// Name in byte
	unsigned char *name_bytes;
	/// Length of name
	size_t name_bytes_len;
	/// Type
	unsigned short type;
	/// Q-class
	unsigned short qclass;
	/// Next question section pointer
	struct _zeroconf_question_t *next;
} zeroconf_question_t;

/// Defined resource record section structure
typedef struct _zeroconf_resource_record_t {
	/// Name in byte
	unsigned char *name_bytes;
	/// Length of name
	size_t name_bytes_len;
	/// Type
	unsigned short type;
	/// R-class
	unsigned short rclass;
	/// TTL
	unsigned int ttl;
	/// Length of data
	unsigned short data_length;
	/// Pointer of data
	unsigned char *data;
	/// Next resource record section pointer
	struct _zeroconf_resource_record_t *next;
} zeroconf_resource_record_t;

/// Defined parsed mDNS packet
typedef struct _zeroconf_mdns_packet_t {
	/// Destination information
	zeroconf_peer_info_t peer_ip;
	/// Transaction ID
	unsigned short transaction_id;
	/// Flags
	unsigned short flags;
	/// Count of question section
	unsigned short questions_count;
	/// Count of resource record section
	unsigned short answer_rrs_count;
	/// Count of authority resource record section
	unsigned short authority_rrs_count;
	/// Count of addtional resource record section
	unsigned short additional_rrs_count;
	/// Pointer of question section
	zeroconf_question_t *questions;
	/// Pointer of resource record section
	zeroconf_resource_record_t *answer_rrs;
	/// Pointer of authority resource record section
	zeroconf_resource_record_t *authority_rrs;
	/// Pointer of addtional resource record section
	zeroconf_resource_record_t *additional_rrs;
} zeroconf_mdns_packet_t;

extern void *(*zeroconf_calloc)(size_t n, size_t size);
extern void (*zeroconf_free)(void *ptr);

/**
 ****************************************************************************************
 * @brief Set memory-management functions.
 * @param[in] calloc_func    The calloc function implementation
 * @param[in] free_func      The free function implementation
 ****************************************************************************************
 */
void zeroconf_set_calloc_free(void *(*calloc_func)(size_t, size_t),
							  void (*free_func)(void *));

#endif // (__SUPPORT_ZERO_CONFIG__)

/**
 ****************************************************************************************
 * @brief Start mDNS service by User CMD.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_start_mdns_service_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Start mDNS service.
 * @param[in] interface      Interface
 * @param[in] hostname       Hostname of mDNS service
 * @param[in] skip_prob      Skip probing if True.
 * @return 0(NX_SCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_start_mdns_service(int interface, char *hostname, unsigned int skip_prob);

/**
 ****************************************************************************************
 * @brief Stop mDNS service.
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_stop_mdns_service();

/**
 ****************************************************************************************
 * @brief Set configuration for mDNS service
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_set_mdns_config_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Display status of Zeroconfig service.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_print_status_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Display cached information of Zeroconfig service.
 * @param[in] argc           Not used
 * @param[in] argv[]         Not used
 ****************************************************************************************
 */
void zeroconf_print_cache_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Lookup mDNS service.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_lookup_mdns_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Change hostname of mDNS service.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_change_mdns_hostname_cmd(int argc, char *argv[]);

//zero-config status
/**
 ****************************************************************************************
 * @brief Returns current status of mDNS service.
 * @return Returns 1(pdTRUE) in mDNS service is running.
 ****************************************************************************************
 */
unsigned int zeroconf_is_running_mdns_service(void);

/**
 ****************************************************************************************
 * @brief Returns current status of DNS-SD service.
 * @return Returns 1(pdTRUE) in DNS-SD service is running.
 ****************************************************************************************
 */
unsigned int zeroconf_is_running_dns_sd_service(void);

/**
 ****************************************************************************************
 * @brief Returns current status of xmDNS service.
 * @return Returns 1(pdTRUE) in xmDNS service is running.
 ****************************************************************************************
 */
unsigned int zeroconf_is_running_xmdns_service(void);

//Zero-config configuration
/**
 ****************************************************************************************
 * @brief Initialize Prob delay for Zeroconfig service.
 *        Prob delay is 250 msec, tie-breaker delay is 150 msec,
 *        and conlict delay is 1000 msec.
 ****************************************************************************************
 */
void zeroconf_init_mdns_prob_timeout();

/**
 ****************************************************************************************
 * @brief Set prob delay. Default is 250 msec.
 * @param[in] value          Prob delay(unit of time: msec)
 ****************************************************************************************
 */
void zeroconf_set_mdns_prob_timeout(unsigned int value);

/**
 ****************************************************************************************
 * @brief Set prob tie-breaker delay. Default is 150 msec.
 * @param[in] value          Prob tie-breaker delay(unit of time: msec)
 ****************************************************************************************
 */
void zeroconf_set_mdns_prob_tie_breaker_timeout(unsigned int value);

/**
 ****************************************************************************************
 * @brief Set prob conflict delay. Default is 1000 msec.
 * @param[in] value          Prob conflict delay(unit of time: msec)
 ****************************************************************************************
 */
void zeroconf_set_mdns_prob_conflict_timeout(unsigned int value);

/**
 ****************************************************************************************
 * @brief Get prob delay.
 * @return Return current prob delay in msec.
 ****************************************************************************************
 */
unsigned int zeroconf_get_mdns_prob_timeout();

/**
 ****************************************************************************************
 * @brief Get prob tie-breaker delay.
 * @return Return current prob tie-breaker delay in msec.
 ****************************************************************************************
 */
unsigned int zeroconf_get_mdns_prob_tie_breaker_timeout();

/**
 ****************************************************************************************
 * @brief Get prob conflict delay.
 * @return Return current prob conflict delay in msec.
 ****************************************************************************************
 */
unsigned int zeroconf_get_mdns_prob_conflict_timeout();

/**
 ****************************************************************************************
 * @brief Support DNS-SD service by User CMD.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_exec_dns_sd_cmd(int argc, char *argv[]);

#if defined (__SUPPORT_DNS_SD__)
/**
 ****************************************************************************************
 * @brief Register a service in DNS-SD.
 * @param[in] name           Name of service
 * @param[in] type           Type of service
 * @param[in] domain         Domain
 * @param[in] port           Port number of service
 * @param[in] txt_cnt        TXT records count
 * @param[in] txt_rec        TXT records vector
 * @param[in] skip_prob      Skip probing if Ture
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_register_dns_sd_service(char *name, char *type,
									 char *domain, int port,
									 int txt_cnt, char **txt_rec,
									 unsigned int skip_prob);

/**
 ****************************************************************************************
 * @brief Deregister a service.
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_unregister_dns_sd_service(void);

/**
 ****************************************************************************************
 * @brief Lookup a service in DNS-SD.
 * @param[in] name           Name of service
 * @param[in] type           Name of service
 * @param[in] domain         Domain
 * @param[in] interval       Interval to retransmit
 * @param[in] wait_option    Wait timeout to get response
 * @param[out] port          Port number of service
 * @param[out] txt[]         TXT records
 * @param[out] txt_len       Number of TXT records
 * @return 0(DA_APP_SUCCESS) on success. Port, TXT records will be returns.
 ****************************************************************************************
 */
int zeroconf_lookup_dns_sd(char *name, char *type, char *domain,
						   unsigned long interval, unsigned long wait_option,
						   unsigned int *port, char *txt[], size_t *txt_len);

/**
 ****************************************************************************************
 * @brief Get registered service in DNS-SD.
 * @param[out] out           Output buffer
 * @param[in] len            Length of buffer
 * @return 0 on success.
 ****************************************************************************************
 */
unsigned int zeroconf_get_dns_sd_service_name(char *out, size_t len);
#endif // (__SUPPORT_DNS_SD__)

/**
 ****************************************************************************************
 * @brief Start xmDNS service by User CMD.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_start_xmdns_service_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Stop xmDNS service
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_stop_xmdns_service();

#if defined (__SUPPORT_XMDNS__)
/**
****************************************************************************************
* @brief Start xmDNS service.
* @param[in] interface      Interface
* @param[in] hostname       Hostname of xmDNS service
* @param[in] skip_prob      Skip probing if Ture
* @return 0(NX_SCCESS) on success.
****************************************************************************************
*/
int zeroconf_start_xmdns_service(int interface, char *hostname, unsigned int skip_prob);
#endif // (__SUPPORT_XMDNS__)

/**
 ****************************************************************************************
 * @brief Change hostname of xmDNS service.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_change_xmdns_hostname_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Lookup xmDNS service.
 * @param[in] argc           Arguments count of User CMD
 * @param[in] argv[]         Arguments vector of User CMD
 * @return 0(DA_APP_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_lookup_xmdns_cmd(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Restart Zeroconfig service in running status.
 ****************************************************************************************
 */
void zeroconf_restart_all_service();

/**
 ****************************************************************************************
 * @brief Stop Zeroconfig service in running status.
 ****************************************************************************************
 */
void zeroconf_stop_all_service();

/**
 ****************************************************************************************
 * @brief Resovle address over mDNS service.
 * @param[in] addr           Domain name
 * @param[in] wait_option    Timeout
 * @return Returns IP address for the domain name.
 ****************************************************************************************
 */
unsigned long zeroconf_get_mdns_ipv4_address_by_name(char *addr, unsigned long wait_option);

/**
 ****************************************************************************************
 * @brief Set register flag of mDNS service.
 *        If NX_TRUE, mDNS service will be running after reboot.
 * @param[in] value          Enabled(NX_TRUE) or not
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_mdns_reg_to_nvram(int value);

/**
 ****************************************************************************************
 * @brief Get register flag of mDNS service.
 * @param[in] out           Enabled(NX_TRUE) or not
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_mdns_reg_from_nvram(int *out);

/**
 ****************************************************************************************
 * @brief Set hostname of mDNS serivce.
 * @param[in] value        Hostname
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_mdns_hostname_to_nvram(char *value);

/**
 ****************************************************************************************
 * @brief Get hostname of mDNS service.
 * @param[out] out        Hostname
 * @param[out] outlen     Length of hostname
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_mdns_hostname_from_nvram(char *out, size_t outlen);

#if defined (__SUPPORT_DNS_SD__)
/**
 ****************************************************************************************
 * @brief Set service name to register a service.
 * @param[in] value      Service name
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_dns_sd_srv_name_to_nvram(char *value);

/**
 ****************************************************************************************
 * @brief Get service name to register a service.
 * @param[out] out       Service name
 * @param[out] outlen    Length of service name
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_dns_sd_srv_name_from_nvram(char *out, size_t outlen);

/**
 ****************************************************************************************
 * @brief Set protocol to register a service.
 * @param[in] value      Protocol
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_dns_sd_srv_protocol_to_nvram(char *value);

/**
 ****************************************************************************************
 * @brief Get protocol to register a service.
 * @param[out] out      Protocol
 * @param[out] outlen   Length of protocol
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_dns_sd_srv_protocol_from_nvram(char *out, size_t outlen);

/**
 ****************************************************************************************
 * @brief Set text record to register a service.
 * @param[in] value    Text record
 * @return REturn 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_dns_sd_srv_txt_to_nvram(char *value);

/**
 ****************************************************************************************
 * @brief Get text record to register a service.
 * @param[out] out     Text record
 * @param[out] outlen  Length of test record
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_dns_sd_srv_txt_from_nvram(char *out, size_t outlen);

/**
 ****************************************************************************************
 * @brief Set port to register a serivce.
 * @param[in] value    Port number
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_dns_sd_srv_port_to_nvram(int value);

/**
 ****************************************************************************************
 * @brief Get port to register a serivce.
 * @param[out] out    Port number
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_dns_sd_srv_port_from_nvram(int *out);

/**
 ****************************************************************************************
 * @brief Set register flag to register a service.
 * @param[in] value   Enabled(NX_TRUE) or not
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_set_dns_sd_srv_reg_to_nvram(int value);

/**
 ****************************************************************************************
 * @brief Get register flag to register a service.
 * @param[out] out    Enabled(NX_TRUE) or not
 * @return Return 0 if successful.
 ****************************************************************************************
 */
int zeroconf_get_dns_sd_srv_reg_from_nvram(int *out);

/**
 ****************************************************************************************
 * @brief Register a service.
 * @return 0(NX_SUCCESS) on success.
 ****************************************************************************************
 */
int zeroconf_register_dns_sd_service_by_nvram();
#endif // (__SUPPORT_DNS_SD__)

#if defined ( __ENABLE_AUTO_START_ZEROCONF__ )
/**
 ****************************************************************************************
 * @brief Start mDNS and registered a service.
 * @param[in] params  Not used
 ****************************************************************************************
 */
void zeroconf_auto_start_reg_services(void *params);
#endif	// (__ENABLE_AUTO_START_ZEROCONF__)

#endif // (__ZERO_CONFIG_H__)

/* EOF */
