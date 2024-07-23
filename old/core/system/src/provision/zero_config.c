/**
 ****************************************************************************************
 * @file zero_config.c
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
#include "zero_config.h"

#if defined (__SUPPORT_ZERO_CONFIG__)

#include "sys_feature.h"
#include "da16x_network_common.h"
#include "da16x_dns_client.h"
#include "da16x_arp.h"
#include "user_nvram_cmd_table.h"
#include "da16x_compile_opt.h"
#include "da16x_sys_watchdog.h"

#define ZEROCONF_PRINTF        PRINTF

#if defined (ZEROCONF_ENABLE_DEBUG_INFO)
#define ZEROCONF_DEBUG_INFO(fmt, ...)   ZEROCONF_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ZEROCONF_DEBUG_INFO(...)        do {} while(0)
#endif // (ZEROCONF_ENABLE_DEBUG_INFO)

#if defined (ZEROCONF_ENABLE_DEBUG_ERR)
#define ZEROCONF_DEBUG_ERR(fmt, ...)    ZEROCONF_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ZEROCONF_DEBUG_ERR(...)         do {} while(0)
#endif // (ZEROCONF_ENABLE_DEBUG_ERR)

#if defined (ZEROCONF_ENABLE_DEBUG_TEMP)
#define ZEROCONF_DEBUG_TEMP(fmt, ...)   ZEROCONF_PRINTF("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ZEROCONF_DEBUG_TEMP(...)        do {} while(0)
#endif    // (ZEROCONF_ENABLE_DEBUG_TEMP)

#define ZEROCONF_CONFLICT_NAME_RULE(x, y, z)    sprintf(x, "%s-%d", y, z)

#define HexVal(X) ( ((X) >= '0' && (X) <= '9') ? ((X) - '0'     ) :        \
                    ((X) >= 'A' && (X) <= 'F') ? ((X) - 'A' + 10) :        \
                    ((X) >= 'a' && (X) <= 'f') ? ((X) - 'a' + 10) : 0)

#define HexPair(P) ((HexVal((P)[0]) << 4) | HexVal((P)[1]))

//local structure
//DPM flags
typedef enum {
    MDNS_DPM_TYPE          = 0,
    DNS_SD_DPM_TYPE        = 1,
    XMDNS_DPM_TYPE         = 2,
    SENDER_DPM_TYPE        = 3,
    RESPONDER_DPM_TYPE     = 4,
} zeroconf_dpm_type_t;

//DPM flags
typedef struct _zeroconf_dpm_state_t {
    unsigned char mdns: 1;
    unsigned char dns_sd: 1;
    unsigned char xmdns: 1;
    unsigned char sender: 1;
    unsigned char responder: 1;
} zeroconf_dpm_state_t;

//DPM information for DNS-SD
typedef struct _zeroconf_local_dns_sd_dpm_t {
    char name[ZEROCONF_DNS_SD_SRV_NAME_LEN];
    size_t name_len;

    char type[ZEROCONF_DNS_SD_SRV_NAME_LEN];
    size_t type_len;

    char domain[8];            //temporary size(.local)
    size_t domain_len;

    int port;

    unsigned char txt_record[ZEROCONF_DNS_SD_SRV_TXT_LEN];
    size_t txt_len;
} zeroconf_local_dns_sd_dpm_t;

typedef struct _zeroconf_dpm_prob_info_t
{
    zeroconf_service_type_t service_key;
    unsigned int skip_prob;
} zeroconf_dpm_prob_info_t;

//Extern variables
extern int interface_select;

//static variables
/// Top domain name for mDNS service
static const char  *mdns_top_domain    = ".local";
/// Top domain name for xmDNS service
static const char  *xmdns_top_domain    = ".site";

//Local variables
zeroconf_status_t g_zeroconf_status;

zeroconf_config_t g_zeroconfig_config = { ZEROCONF_DEF_PROB_TIMEOUT,
                                          ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT,
                                          ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT };

zeroconf_dpm_state_t zero_dpm_state = { pdTRUE,        //mdns
                                        pdTRUE,        //dns-sd
                                        pdTRUE,        //xmdns
                                        pdTRUE,        //sender
                                        pdTRUE };    //responder

EventGroupHandle_t g_zeroconf_sender_event_handler = NULL;

zeroconf_mdns_header_t g_zeroconf_mdns_header;
zeroconf_mdns_response_t *g_zeroconf_mdns_response_root = NULL;
zeroconf_pck_send_t *g_zeroconf_pck_send_root = NULL;
zeroconf_query_string_t *g_zeroconf_query_string_root = NULL;

zeroconf_mdns_host_info_t g_zeroconf_mdns_host_info;
zeroconf_mdns_host_info_t g_zeroconf_xmdns_host_info;

#if defined (__SUPPORT_DNS_SD__)
zeroconf_local_dns_sd_info_t *g_zeroconf_local_dns_sd_service = NULL;
zeroconf_proxy_dns_sd_info_t *g_zeroconf_proxy_dns_sd_svc = NULL;
zeroconf_dns_sd_query_t g_zeroconf_dns_sd_query;
#endif    // (__SUPPORT_DNS_SD__)

zeroconf_dpm_prob_info_t g_zeroconf_prob_info = { UNKNOWN_SERVICE_TYPE, pdFALSE };

//local function
static ssize_t recvfromto(int s, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen,
                          struct sockaddr *to, socklen_t *tolen);

void init_zero_status(void);
void set_mdns_service_status(const zeroconf_status_e status);
void set_dns_sd_service_status(const zeroconf_status_e status);
void set_xmdns_service_status(const zeroconf_status_e status);

unsigned int is_done_init_processor(void);
unsigned int is_running_mdns_probing_processor(void);
unsigned int is_running_dns_sd_probing_processor(void);
unsigned int is_running_responder_processor(void);
unsigned int is_running_sender_processor(void);

int set_zero_ip_info(const int iface);
unsigned int is_supporting_ipv4(void);
struct sockaddr_in get_ipv4_address(void);
void get_ipv4_bytes(unsigned char *value);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
unsigned int is_supporting_ipv6(void);
void get_ipv6_address(unsigned long *addr);
void get_ipv6_bytes(unsigned char *value);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

void init_mdns_host_info(void);
void init_xmdns_host_info(void);

int init_zeroconf(const int iface);
int deinit_zeroconf(void);
int zeroconf_start_mdns_probing_task(zeroconf_dpm_prob_info_t *prob_info);
int zeroconf_start_xmdns_probing_task(zeroconf_dpm_prob_info_t *prob_info);
int zeroconf_stop_mdns_probing_task(void);
#if defined (__SUPPORT_DNS_SD__)
int zeroconf_start_dns_sd_local_probing_task(zeroconf_dpm_prob_info_t *prob_info);
int zeroconf_start_dns_sd_proxy_probing_task(zeroconf_dpm_prob_info_t *prob_info);
int zeroconf_stop_dns_sd_probing_task(void);
#endif // (__SUPPORT_DNS_SD__)
int zeroconf_start_restarter_task(unsigned int type);
int zeroconf_stop_restarter_task(void);
int zeroconf_start_responder_task(void);
int zeroconf_stop_responder_task(void);
int zeroconf_start_sender_task(void);
int zeroconf_stop_sender_task(void);
#if defined (__SUPPORT_DNS_SD__)
int zeroconf_start_exec_task(char *name_ptr, void (*entry_function)(void *params), void *entry_input);
int zeroconf_stop_exec_task(void);
#endif // (__SUPPORT_DNS_SD__)

int mdns_Query(const zeroconf_service_type_t service_key, char *domain_name, char *ipaddr);
void mdns_Lookup(const zeroconf_service_type_t service_key, int argc, char *argv[]);

int generate_response(zeroconf_response_type_t response_type,
                      zeroconf_service_type_t rprotocol,
                      unsigned char *rname_bytes, size_t rname_bytes_len,
                      unsigned short rtype,
                      zeroconf_resource_record_t *known_answer_rrs,
                      zeroconf_resource_record_t **out_answer_rrs,
                      unsigned short *out_answer_rrs_count,
                      zeroconf_resource_record_t **out_additional_rrs,
                      unsigned short *out_additional_rrs_count);
int send_multi_responses(zeroconf_service_type_t protocol, zeroconf_mdns_packet_t *payload);
int send_mdns_packet(const zeroconf_service_type_t service_key,
                     void *data_string, int len_data);
int send_mdns_packet_with_dest(const zeroconf_service_type_t service_key,
                               void *data_string, int len_data, struct sockaddr ip_addr);
void send_ttl_zero(const zeroconf_service_type_t service_key);
void doProb(void *params);
int doAnnounce(const zeroconf_service_type_t service_key, unsigned int skip_announcement);
int receive_prob_response(int timeout, zeroconf_mdns_packet_t *prob_packet);
int process_mdns_responder(zeroconf_service_type_t service_key, char *hostname, unsigned int skip_prob);
void restarter_process(void *params);
void responder_process(void *params);
void sender_process(void *params);
void cache_clear_process(void);
void wait_for_close(void);
int insertOwnHostInfo(const zeroconf_service_type_t service_key);
int get_offset(unsigned char msb, unsigned char lsb);
void set_offset(int val, unsigned char *offset_arr);
void to_lower(char *str);
int random_number(int min_num, int max_num);
void create_packet_header(int QDCOUNT, int ANCOUNT, int NSCOUNT, int ARCOUNT,
                          int isRequest, unsigned char *arr, int idx);
unsigned int create_query(unsigned char *name_bytes, size_t name_bytes_len,
                          unsigned short type, unsigned short qclass,
                          unsigned char *out, int start);
unsigned int create_general_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                unsigned short type, unsigned short rclass,
                                unsigned int ttl, unsigned short data_length,
                                unsigned char *data, unsigned char *out, int start);
unsigned int create_mdns_srv_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                 unsigned short rclass, unsigned int ttl, int port,
                                 unsigned char *host_bytes, size_t host_bytes_len,
                                 unsigned char *out, int start);
unsigned int create_mdns_txt_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                 unsigned short rclass, unsigned int ttl,
                                 size_t txtLen, unsigned char *txtRecord,
                                 unsigned char *out, int start);
unsigned int convert_questions_to_bytes(zeroconf_question_t *questions,
                                        unsigned char *out, size_t out_len);
unsigned int convert_resource_records_to_bytes(zeroconf_resource_record_t *rrecords,
                                               unsigned char *out, size_t out_len);
unsigned int convert_mdns_packet_to_bytes(zeroconf_mdns_packet_t *mdns_packet,
                                          unsigned char *out, size_t out_len);
unsigned int calculate_resource_record_delay(zeroconf_resource_record_t *rrecords,
                                             unsigned int is_truncated);
unsigned int is_known_answer_rr(zeroconf_resource_record_t *answer_rrs,
                                unsigned char *name_bytes,
                                size_t name_bytes_len,
                                unsigned int ttl);
int insert_question(zeroconf_question_t **questions, unsigned char *name_bytes,
                    size_t name_bytes_len, unsigned short type, unsigned short qclass);
zeroconf_question_t *create_question(unsigned char *name_bytes,
                                     size_t name_bytes_len,
                                     unsigned short type,
                                     unsigned short qclass);
int insert_resource_record(zeroconf_resource_record_t **rrecords,
                           unsigned char *name_bytes,
                           size_t name_bytes_len,
                           unsigned short type,
                           unsigned short rclass,
                           unsigned int ttl,
                           unsigned short data_length,
                           unsigned char *data);
zeroconf_resource_record_t *create_resource_record(unsigned char *name_bytes,
                                                   size_t name_bytes_len,
                                                   unsigned short type,
                                                   unsigned short rclass,
                                                   unsigned int ttl,
                                                   unsigned short data_length,
                                                   unsigned char *data);
int insert_srv_record(zeroconf_resource_record_t **rrecords,
                      unsigned char *name_bytes, size_t name_bytes_len,
                      unsigned short rclass, unsigned int ttl, unsigned short priority,
                      unsigned short weight, unsigned short port,
                      unsigned short data_length, unsigned char *data);
int set_host_info(const zeroconf_service_type_t service_key,
                  char *hostname, const char *domain);
unsigned int compare_domain_bytes(unsigned char *domain_bytes,
                                  size_t domain_bytes_len,
                                  unsigned char *rcvd_bytes,
                                  size_t rcvd_bytes_len);
int compare_data_bytes(unsigned char *data1, unsigned char *data2, int len);
unsigned int search_in_byte_array(unsigned char *source, size_t source_len,
                                  unsigned char *key, size_t key_len);
void reset_lists(void);
void clear_query_cache(void);
void clear_send_cache(int type);
void print_mdns_response(zeroconf_mdns_response_t *resp, int print_raw, int show_date);

#if defined (__SUPPORT_DNS_SD__)
zeroconf_local_dns_sd_info_t *createLocalDnsSd(char *name, char *type,
                                               char *domain, int port,
                                               size_t txtLen, unsigned char *txtRecord);
void deleteLocalDnsSd(zeroconf_local_dns_sd_info_t *local_dns_sd);
zeroconf_proxy_dns_sd_info_t *createProxyDnsSd(char *h_name, unsigned char *h_ip,
                                               char *name, char *type,
                                               char *domain, int port,
                                               size_t txtLen, unsigned char *txtRecord);
void deleteProxyDnsSd(zeroconf_proxy_dns_sd_info_t *proxy_dns_sd);
void printLocalDnsSd(void);
void printProxyDnsSd(void);
#endif    /* __SUPPORT_DNS_SD__ */
zeroconf_mdns_response_t *createMdnsResp(unsigned char *query, size_t query_len,
                                         int type, LONG ttl, unsigned long timestamp,
                                         size_t data_len, unsigned char *data);
void viewMdnsResponses(void);
void insertMdnsResp(unsigned char *query, size_t query_len,
                    int type, LONG ttl, unsigned long timestamp,
                    int data_len, unsigned char *data, int check);
void deleteMdnsPtrRec(unsigned char *query, size_t query_len);
void deleteMdnsResp(unsigned char *query, size_t query_len, int searchMode, int type);
void clear_mdns_cache(void);
void clear_mdns_invalid_ttls(void);
int check_ttl_expiration(unsigned long packet_time, int ttl_value);
void free_mdns_resp(zeroconf_mdns_response_t *tofree);
void insertPckSend(unsigned char *full_packet, int len, int max_count, int interval,
                   int last_sent_time, int mult_fact, int type,
                   zeroconf_peer_info_t ipv4_addr, zeroconf_peer_info_t ipv6_addr,
                   int packet_type);
void deletePckSend(int id);
void viewPckSends(void);
zeroconf_query_string_t *createQueryString(unsigned char *query, size_t query_len);
void deleteQueryString(unsigned char *query, size_t query_len);
void insertQueryString(unsigned char *query, size_t query_len);
void viewQueryStrings(void);
void viewStructHeader(void);
void display_mdns_usage(void);
#if defined (__SUPPORT_XMDNS__)
void display_xmdns_usage(void);
#endif // (__SUPPORT_XMDNS__)
void display_mdns_config_usage(void);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
ip_addr_t get_mdns_ipv6_multicast(void);
#if defined (__SUPPORT_XMDNS__)
ip_addr_t get_xmdns_ipv6_multicast(void);
#endif // (__SUPPORT_XMDNS__)
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#if defined (__SUPPORT_DNS_SD__)
void insertLocalDns(void);
void deleteLocalDns(void);
void insertProxyDns(void);
void deleteProxyDns(void);
void doDnsSdProb(void *params);
void doDnsSdProxyProb(void *params);
void doDnsSdAnnounce(const int announce_type, unsigned int skip_announcement);
void execute_dns_sd_E(void *params);
void execute_dns_sd_F(void *params);
void execute_dns_sd_B(void *params);
void execute_dns_sd_L(void *params);
int execute_dns_sd_P(int argc, char *argv[]);
void execute_dns_sd_Q(void *params);
void execute_dns_sd_Z(void *params);
void execute_dns_sd_G(void *params);
unsigned long get_type(char *type_code);
void print_dns_sd_command_error(void);
void print_dns_sd_command_help(void);
void stop_dns_sd_local_service(void);
void stop_dns_sd_proxy_service(void);
void print_dns_sd_old_cache(void);
void print_dns_sd_cache(zeroconf_mdns_response_t *temp, int isCreate);
int zeroconf_unregister_dns_sd_service(void);
#endif // (__SUPPORT_DNS_SD__)
void clear_mdns_packet(zeroconf_mdns_packet_t *mdns_packet);
unsigned int parse_mdns_packet_question(unsigned char *payload,
                                        zeroconf_mdns_packet_t *out, unsigned int start);
unsigned int parse_mdns_packet_resource_record(unsigned char *payload,
                                               zeroconf_mdns_packet_t *out,
                                               unsigned int start,
                                               unsigned int rr_section);
int parse_mdns_packet(unsigned char *payload, zeroconf_mdns_packet_t *out);
unsigned int encode_mdns_name_string(char *name, unsigned char *ptr, size_t size);
unsigned int unencode_mdns_name_string(unsigned char *payload, unsigned int start,
                                       char *buffer, size_t size);
unsigned int calculate_mdns_name_size(unsigned char *name);
int parse_mdns_command(int argc, char *argv[],
                       zeroconf_service_type_t service_key, int *iface, char *hostname);
int dpm_set_zeroconf_sleep(zeroconf_dpm_type_t type);
int dpm_clr_zeroconf_sleep(zeroconf_dpm_type_t type);
#if defined (__SUPPORT_DNS_SD__)
static int dpm_set_local_dns_sd_info(zeroconf_local_dns_sd_info_t *dns_sd_service);
zeroconf_local_dns_sd_info_t *dpm_get_local_dns_sd_info(void);
int dpm_clear_local_dns_sd_info(void);
#endif // (__SUPPORT_DNS_SD__)

static void *zeroconf_internal_calloc(size_t n, size_t size)
{
    void *buf = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf) {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

static void zeroconf_internal_free(void *f)
{
    if (f == NULL) {
        return;
    }

    vPortFree(f);
}

void *(*zeroconf_calloc)(size_t n, size_t size) = zeroconf_internal_calloc;
void (*zeroconf_free)(void *ptr) = zeroconf_internal_free;

void zeroconf_set_calloc_free(void *(*calloc_func)(size_t, size_t), void (*free_func)(void *))
{
    zeroconf_calloc = calloc_func;
    zeroconf_free = free_func;
}

//LWIP_NETBUF_RECVINFO
static ssize_t recvfromto(int s, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen,
                          struct sockaddr *to, socklen_t *tolen)
{
    struct msghdr msg;
    struct iovec iov[1];
    int n;
    struct cmsghdr *cmptr;
    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(struct in_addr)) + CMSG_SPACE(sizeof(struct in_pktinfo))];
    } control_un;

    struct in_pktinfo pktinfo;

    if (*tolen) {
        memset(to, 0x00, sizeof(struct sockaddr));
    }

#if !defined(IP_PKTINFO) && !defined(LWIP_NETBUF_RECVINFO)
    return recvfrom(s, mem, len, flags, from, fromlen);
#endif // !(IP_PKTINFO) && !(LWIP_NETBUF_RECVINFO)

    //Try to enable getting the return address
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    msg.msg_flags = 0;
    msg.msg_name = from;
    msg.msg_namelen = *fromlen;
    iov[0].iov_base = mem;
    iov[0].iov_len = len;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    if ((n = recvmsg(s, &msg, flags)) < 0) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            ZEROCONF_DEBUG_TEMP("n(%d), err(%d)\n", n, errno);
            return n; //Error
        }
        return 0;
    }
    *fromlen = msg.msg_namelen;

    if (to) {
        memset(to, 0x00, sizeof(struct sockaddr));
        ((struct sockaddr_in *)to)->sin_family = AF_INET;
        if (msg.msg_controllen < sizeof(struct cmsghdr) || (msg.msg_flags & MSG_CTRUNC)) {
            ZEROCONF_DEBUG_TEMP("No information available\n");
            return n;
        }

        for (cmptr = CMSG_FIRSTHDR(&msg) ; cmptr != NULL ; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
            if ((cmptr->cmsg_level == IPPROTO_IP) && (cmptr->cmsg_type == IP_PKTINFO)) {
                memcpy(&pktinfo, CMSG_DATA(cmptr), sizeof(struct in_pktinfo));
                memcpy(&((struct sockaddr_in *)to)->sin_addr, &pktinfo.ipi_addr,
                       sizeof(struct in_addr));
                *tolen = sizeof(struct sockaddr_in);

                ZEROCONF_DEBUG_INFO("from(%d.%d.%d.%d) to(%d.%d.%d.%d)\n",
                                    (ntohl(((struct sockaddr_in *)from)->sin_addr.s_addr) >> 24) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)from)->sin_addr.s_addr) >> 16) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)from)->sin_addr.s_addr) >>  8) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)from)->sin_addr.s_addr)      ) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)to)->sin_addr.s_addr) >> 24) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)to)->sin_addr.s_addr) >> 16) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)to)->sin_addr.s_addr) >>  8) & 0x0ff,
                                    (ntohl(((struct sockaddr_in *)to)->sin_addr.s_addr)      ) & 0x0ff);
            } // if
        } // for
    } // if (to)

    ZEROCONF_DEBUG_INFO("Received data(%ld)\n", n);

    return n;
}

//manage status of zero-config
void init_zero_status(void)
{
    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "Init Zero-config status\n" CLEAR_COLOR);

    memset(&g_zeroconf_status, 0x00, sizeof(zeroconf_status_t));

    g_zeroconf_status.status = ZEROCONF_INIT;

    g_zeroconf_status.mdns_service = ZEROCONF_INIT;
    g_zeroconf_status.xmdns_service = ZEROCONF_INIT;
    g_zeroconf_status.dns_sd_service = ZEROCONF_INIT;

    g_zeroconf_status.mdns_probing_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.mdns_probing_proc.task_handler = NULL;
    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.dns_sd_probing_proc.task_handler = NULL;
    g_zeroconf_status.responder_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.responder_proc.task_handler = NULL;
    g_zeroconf_status.sender_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.sender_proc.task_handler = NULL;
    g_zeroconf_status.restarter_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.restarter_proc.task_handler = NULL;
    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    g_zeroconf_status.iface = WLAN0_IFACE;
    g_zeroconf_status.is_supported_ipv4 = pdFALSE;

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    g_zeroconf_status.is_supported_ipv6 = pdFALSE;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    return;
}

void set_mdns_service_status(const zeroconf_status_e status)
{
    g_zeroconf_status.mdns_service = status;

    ZEROCONF_DEBUG_INFO("status(%ld)\n", status);

    if (dpm_mode_is_enabled()) {
        if (status == ZEROCONF_ACTIVATED) {
            const char *domain_ptr;
            size_t hostname_len = strlen(g_zeroconf_mdns_host_info.hostname);

            domain_ptr = strstr(g_zeroconf_mdns_host_info.hostname, mdns_top_domain);
            if (domain_ptr) {
                hostname_len = domain_ptr - g_zeroconf_mdns_host_info.hostname;
            }

            dpm_set_mdns_info(g_zeroconf_mdns_host_info.hostname, hostname_len);
        } else if (status == ZEROCONF_DEACTIVATED) {
            dpm_clear_mdns_info();
        }
    }
}

void set_dns_sd_service_status(const zeroconf_status_e status)
{
    g_zeroconf_status.dns_sd_service = status;

    ZEROCONF_DEBUG_INFO("status(%ld)\n", status);

#if defined (__SUPPORT_DNS_SD__)
    int ret = DA_APP_SUCCESS;

    if (dpm_mode_is_enabled()) {
        if (status == ZEROCONF_ACTIVATED) {
            ret = dpm_set_local_dns_sd_info(g_zeroconf_local_dns_sd_service);
            if (ret != DA_APP_SUCCESS) {
                ZEROCONF_DEBUG_ERR("Failed to set dns-sd info(0x%x)\n", ret);
                dpm_clear_local_dns_sd_info();
            }
        } else if (status == ZEROCONF_DEACTIVATED) {
            dpm_clear_local_dns_sd_info();
        }
    }
#endif // (__SUPPORT_DNS_SD__)
}

void set_xmdns_service_status(const zeroconf_status_e status)
{
    g_zeroconf_status.xmdns_service = status;
}

unsigned int zeroconf_is_running_mdns_service(void)
{
    return g_zeroconf_status.mdns_service == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int zeroconf_is_running_dns_sd_service(void)
{
    return g_zeroconf_status.dns_sd_service == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int zeroconf_is_running_xmdns_service(void)
{
    return g_zeroconf_status.xmdns_service == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int is_done_init_processor(void)
{
    return g_zeroconf_status.status == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int is_running_mdns_probing_processor(void)
{
    return g_zeroconf_status.mdns_probing_proc.status == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int is_running_dns_sd_probing_processor(void)
{
    return g_zeroconf_status.dns_sd_probing_proc.status == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int is_running_responder_processor(void)
{
    return g_zeroconf_status.responder_proc.status == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

unsigned int is_running_sender_processor(void)
{
    return g_zeroconf_status.sender_proc.status == ZEROCONF_ACTIVATED ? pdTRUE : pdFALSE;
}

int set_zero_ip_info(const int iface)
{
    struct netif *netif = netif_get_by_index(iface + 2);

    if (netif == NULL) {
        return DA_APP_INVALID_INTERFACE;
    }

    //interface
    g_zeroconf_status.iface = iface;

    //ipv4 address
    g_zeroconf_status.is_supported_ipv4 = pdTRUE;

    memset(&g_zeroconf_status.ipv4_addr, 0x00, sizeof(struct sockaddr_in));
    g_zeroconf_status.ipv4_addr.sin_family = AF_INET;
    memcpy(&g_zeroconf_status.ipv4_addr.sin_addr.s_addr, netif_ip_addr4(netif),
           sizeof(in_addr_t));

    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR
                        "Set Zero-config IP Address information\n"
                        "* Interface: %d\n"
                        "* IPv4 Address: %d.%d.%d.%d\n"
                        CLEAR_COLOR,
                        iface,
                        (ntohl(g_zeroconf_status.ipv4_addr.sin_addr.s_addr) >> 24) & 0x0ff,
                        (ntohl(g_zeroconf_status.ipv4_addr.sin_addr.s_addr) >> 16) & 0x0ff,
                        (ntohl(g_zeroconf_status.ipv4_addr.sin_addr.s_addr) >>  8) & 0x0ff,
                        (ntohl(g_zeroconf_status.ipv4_addr.sin_addr.s_addr)      ) & 0x0ff);

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    struct sockaddr_in6 ipv6_addr;

    g_zeroconf_status.is_supported_ipv6 = pdTRUE;

    for (int idx = 0 ; idx < LWIP_IPV6_NUM_ADDRESSES ; idx++) {
        memset(&g_zeroconf_status.ipv6_addr, 0x00, sizeof(struct sockaddr_in6));

        g_zeroconf_status.ipv6_addr.sin6_family = AF_INET6;
        //netif_ip_addr6(netif, idx);

#if 0 /* TODO. 20201008 */
        if (addr.nxd_ip_address.v6[0] >> 16 == 0xFE80) {
            memcpy(g_zeroconf_status.ipv6_addr,
                   addr.nxd_ip_address.v6,
                   sizeof(g_zeroconf_status.ipv6_addr));

            break;
        }
#endif
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF)

    ZEROCONF_DEBUG_TEMP("\n");

    return DA_APP_SUCCESS;
}

unsigned int is_supporting_ipv4(void)
{
    return g_zeroconf_status.is_supported_ipv4;
}

struct sockaddr_in get_ipv4_address(void)
{
    return g_zeroconf_status.ipv4_addr;
}

void get_ipv4_bytes(unsigned char *value)
{
    struct sockaddr_in ipv4_addr = get_ipv4_address();

    memset(value, 0x00, sizeof(unsigned long));

    value[0] = (ntohl(ipv4_addr.sin_addr.s_addr) >> 24) & 0x0ff;
    value[1] = (ntohl(ipv4_addr.sin_addr.s_addr) >> 16) & 0x0ff;
    value[2] = (ntohl(ipv4_addr.sin_addr.s_addr) >> 8) & 0x0ff;
    value[3] = ntohl(ipv4_addr.sin_addr.s_addr) & 0x0ff;

    return;
}

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
unsigned int is_supporting_ipv6(void)
{
    return g_zeroconf_status.is_supported_ipv6;
}

void get_ipv6_address(unsigned long *addr)
{
    memset(addr, 0x00, sizeof(unsigned long) * 4);

    if (is_supporting_ipv6()) {
        memcpy(addr, g_zeroconf_status.ipv6_addr, sizeof(unsigned long) * 4);
    }

    return;
}

void get_ipv6_bytes(unsigned char *value)
{
    unsigned long addr[4] = {0x00,};
    int index = 0;

    memset(value, 0x00, sizeof(unsigned long) * 4);

    get_ipv6_address(addr);

    if (addr) {
        for (index = 0 ; index < 4 ; index++) {
            value[(index * 4) + 0] = (addr[index] >> 24) & 0x000000ff;
            value[(index * 4) + 1] = (addr[index] >> 16) & 0x000000ff;
            value[(index * 4) + 2] = (addr[index] >> 8) & 0x000000ff;
            value[(index * 4) + 3] = (addr[index] >> 0) & 0x000000ff;
        }
    }

    return;
}
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

void init_mdns_host_info(void)
{
    memset(&g_zeroconf_mdns_host_info, 0x00, sizeof(zeroconf_mdns_host_info_t));
}

void init_xmdns_host_info(void)
{
    memset(&g_zeroconf_xmdns_host_info, 0x00, sizeof(zeroconf_mdns_host_info_t));
}

void zeroconf_init_mdns_prob_timeout(void)
{
    zeroconf_set_mdns_prob_timeout(ZEROCONF_DEF_PROB_TIMEOUT);
    zeroconf_set_mdns_prob_tie_breaker_timeout(ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT);
    zeroconf_set_mdns_prob_conflict_timeout(ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT);
}

void zeroconf_set_mdns_prob_timeout(unsigned int value)
{
    if (value < ZEROCONF_DEF_PROB_TIMEOUT) {
        return;
    }

    g_zeroconfig_config.prob_timeout = value;

    return;
}

void zeroconf_set_mdns_prob_tie_breaker_timeout(unsigned int value)
{
    if (value < ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT) {
        return;
    }

    g_zeroconfig_config.prob_tie_breaker_timeout = value;

    return;
}

void zeroconf_set_mdns_prob_conflict_timeout(unsigned int value)
{
    if (value < ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT) {
        return;
    }

    g_zeroconfig_config.prob_conflict_timeout = value;
}

unsigned int zeroconf_get_mdns_prob_timeout(void)
{
    if (g_zeroconfig_config.prob_timeout < ZEROCONF_DEF_PROB_TIMEOUT) {
        return ZEROCONF_DEF_PROB_TIMEOUT;
    }

    return g_zeroconfig_config.prob_timeout;
}

unsigned int zeroconf_get_mdns_prob_tie_breaker_timeout(void)
{
    if (g_zeroconfig_config.prob_tie_breaker_timeout < ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT) {
        return ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT;
    }

    return g_zeroconfig_config.prob_tie_breaker_timeout;
}

unsigned int zeroconf_get_mdns_prob_conflict_timeout(void)
{
    if (g_zeroconfig_config.prob_conflict_timeout < ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT) {
        return ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT;
    }

    return g_zeroconfig_config.prob_conflict_timeout;
}

unsigned int zeroconf_get_mdns_hostname(char *out, size_t len)
{
    if (zeroconf_is_running_mdns_service()) {
        unsigned int name_len = strlen(g_zeroconf_mdns_host_info.hostname);

        if (name_len > 0 && name_len < len) {
            strcpy(out, g_zeroconf_mdns_host_info.hostname);

            return name_len;
        }
    }

    return 0;
}

unsigned int zeroconf_get_dns_sd_service_name(char *out, size_t len)
{
#if defined (__SUPPORT_DNS_SD__)
    if (zeroconf_is_running_dns_sd_service()
        && g_zeroconf_local_dns_sd_service != NULL) {
        unsigned int service_len = strlen(g_zeroconf_local_dns_sd_service->name);
        if (service_len > 0 && service_len < len) {
            strcpy(out, g_zeroconf_local_dns_sd_service->name);

            return service_len;
        }
    }
#else
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(len);
#endif // (__SUPPORT_DNS_SD__)

    return 0;
}

int init_zeroconf(const int iface)
{
    int ret = DA_APP_SUCCESS;

    int sock_fd = 0;
    unsigned int reuse = 1;
    unsigned char loop_back = 0;
    unsigned char ttl = 0xff;
    struct ip_mreq mdns_mreq;
#if defined(__SUPPORT_XMDNS__)
    struct ip_mreq xmdns_mreq;
#endif // __SUPPORT_XMDNS__
    struct sockaddr_in dest_addr;
    struct netif *netif = NULL;

    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "Init zero-conf(interface %d)\n" CLEAR_COLOR, iface);

    if (is_done_init_processor()) {
        ZEROCONF_DEBUG_INFO("zeroconf already initialized\n");
        return DA_APP_SUCCESS;
    }

    //check netif
    netif = netif_get_by_index(iface + 2);
    if (!netif) {
        ZEROCONF_DEBUG_ERR("Failed to get network interface\n");
        return DA_APP_INVALID_INTERFACE;
    }

    if (!netif_is_up(netif)) {
        ZEROCONF_DEBUG_ERR("Network is not up\n");
        return DA_APP_NOT_ENABLED;
    }

    if (!(netif->flags & NETIF_FLAG_IGMP)) {
#if defined(LWIP_IGMP)
        ZEROCONF_DEBUG_INFO("Set IGMP & start\n");

        netif_set_flags(netif, NETIF_FLAG_IGMP);

        igmp_start(netif);
#endif // (LWIP_IGMP)
    }

    //init status of zero config
    init_zero_status();

    // Create a zeroconf socket.
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        ZEROCONF_DEBUG_ERR("Zero config Socket create error(%d)\n", sock_fd);
        return DA_APP_SOCKET_NOT_CREATE;
    }

    ret = bind_if(sock_fd, iface + 2);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to set interface(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }

    //Allow multiple sockets to use the same PORT number
    ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse,
                     sizeof(reuse));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set SO_REUSEADDR(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }

    //Disallow loop-back
    ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop_back,
                     sizeof(loop_back));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set IP_MULTICAST_LOOP(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }

    ret = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set IP_MULTICAST_TTL(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }

    //Use setsockopt() to request that kernel join a multicast group
    memset(&mdns_mreq, 0x00, sizeof(mdns_mreq));
    mdns_mreq.imr_multiaddr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);
    mdns_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mdns_mreq,
                     sizeof(mdns_mreq));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to join multicast group for mdns(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }

#if defined(__SUPPORT_XMDNS__)
    xmdns_mreq.imr_multiaddr.s_addr = inet_addr(ZEROCONF_XMDNS_ADDR);
    xmdns_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    ret = setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&xmdns_mreq,
                     sizeof(xmdns_mreq));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to join multicast group for xmdns(%d)\n", ret);
        close(sock_fd);

        return DA_APP_SOCKET_OPT_ERROR;
    }
#endif // (__SUPPORT_XMDNS__)

    int sockopt_pktinfo = 1;
    ret = setsockopt(sock_fd, IPPROTO_IP, IP_PKTINFO, &sockopt_pktinfo,
                     sizeof(sockopt_pktinfo));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set socket option - IP_PKTINFO\n");
    }

    //Setup destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(ZEROCONF_MDNS_PORT);

    //Bind to receive address
    ret = bind(sock_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to bind socket(%d)\n", ret);
        close(sock_fd);

        return DA_APP_NOT_BOUND;
    }

    //init ip information of zero configuration
    ret = set_zero_ip_info(iface);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to set ip address up(0x%x)\n", -ret);
        close(sock_fd);

        return ret;
    }

    g_zeroconf_status.status = ZEROCONF_ACTIVATED;

    g_zeroconf_status.socket_fd = sock_fd;

    return DA_APP_SUCCESS;
}

int deinit_zeroconf(void)
{
    int ret = 0;

    struct ip_mreq mdns_mreq;
#if defined(__SUPPORT_XMDNS__)
    struct ip_mreq xmdns_mreq;
#endif // __SUPPORT_XMDNS__

    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "Deinit zero-conf\n" CLEAR_COLOR);

    if (!is_done_init_processor()) {
        ZEROCONF_DEBUG_INFO("zeroconf is not initailized\n");
        return DA_APP_NOT_ENABLED;
    }

    mdns_mreq.imr_multiaddr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);
    mdns_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    ret = setsockopt(g_zeroconf_status.socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                     &mdns_mreq, sizeof(mdns_mreq));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set IP_DROP_MEMBERSHIP for mdns(%d)\n", ret);
    }

#if defined(__SUPPORT_XMDNS__)
    xmdns_mreq.imr_multiaddr.s_addr = inet_addr(ZEROCONF_XMDNS_ADDR);
    xmdns_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    ret = setsockopt(g_zeroconf_status.socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                     &xmdns_mreq, sizeof(xmdns_mreq));
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to set IP_DROP_MEMBERSHIP for xmdns(%d)\n", ret);
    }
#endif // (__SUPPORT_XMDNS__)

    ret = close(g_zeroconf_status.socket_fd);
    if (ret < 0) {
        ZEROCONF_DEBUG_ERR("Failed to close socket(%d)\n", ret);
    } else {
        g_zeroconf_status.socket_fd = 0;
    }

    zeroconf_stop_mdns_probing_task();

    zeroconf_stop_responder_task();

    zeroconf_stop_sender_task();

#if defined(__SUPPORT_DNS_SD__)
    zeroconf_stop_dns_sd_probing_task();

    zeroconf_stop_exec_task();
#endif    // (__SUPPORT_DNS_SD__)

    zeroconf_stop_restarter_task();

    g_zeroconf_status.status = ZEROCONF_INIT;

    return DA_APP_SUCCESS;
}

int zeroconf_start_mdns_probing_task(zeroconf_dpm_prob_info_t *prob_info)
{
    int ret = 0;

    ZEROCONF_DEBUG_INFO("create mdns-probing task(%s)\n",
                        ZEROCONF_MDNS_PROB_THREAD_NAME);

    ret = zeroconf_stop_mdns_probing_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop mdns-probing task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(doProb,
                      ZEROCONF_MDNS_PROB_THREAD_NAME,
                      ZEROCONF_PROB_STACK_SIZE,
                      (void *)prob_info,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.mdns_probing_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create mdns-probing task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_start_xmdns_probing_task(zeroconf_dpm_prob_info_t *prob_info)
{
    int ret = 0;

    ZEROCONF_DEBUG_INFO("create xmdns-probing task(%s)\n",
                        ZEROCONF_MDNS_PROB_THREAD_NAME);

    ret = zeroconf_stop_mdns_probing_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop xmdns-probing task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(doProb,
                      ZEROCONF_MDNS_PROB_THREAD_NAME,
                      ZEROCONF_PROB_STACK_SIZE,
                      (void *)prob_info,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.mdns_probing_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create xmdns-probing task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_stop_mdns_probing_task()
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy mdns-probing task\n");

    if (g_zeroconf_status.mdns_probing_proc.status != ZEROCONF_INIT) {
        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.mdns_probing_proc.status == ZEROCONF_DEACTIVATED) {
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.mdns_probing_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.mdns_probing_proc.task_handler);
        }
    }

    g_zeroconf_status.mdns_probing_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.mdns_probing_proc.task_handler = NULL;

    return DA_APP_SUCCESS;
}


#if defined (__SUPPORT_DNS_SD__)
int zeroconf_start_dns_sd_local_probing_task(zeroconf_dpm_prob_info_t *prob_info)
{
    int ret = 0;

    ZEROCONF_DEBUG_INFO("create dns-sd probing task(%s)\n",
                        ZEROCONF_DNS_SD_PROB_THREAD_NAME);

    ret = zeroconf_stop_dns_sd_probing_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop dns-sd probing task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(doDnsSdProb,
                      ZEROCONF_DNS_SD_PROB_THREAD_NAME,
                      ZEROCONF_PROB_STACK_SIZE,
                      (void *)prob_info,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.dns_sd_probing_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create dns-sd probing task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_start_dns_sd_proxy_probing_task(zeroconf_dpm_prob_info_t *prob_info)
{
    int ret = 0;

    ZEROCONF_DEBUG_INFO("create dns-sd probing task(%s)\n",
                        ZEROCONF_DNS_SD_PROB_THREAD_NAME);

    ret = zeroconf_stop_dns_sd_probing_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop dns-sd probing task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(doDnsSdProxyProb,
                      ZEROCONF_DNS_SD_PROB_THREAD_NAME,
                      ZEROCONF_PROB_STACK_SIZE,
                      (void *)prob_info,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.dns_sd_probing_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create dns-sd probing task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;

}

int zeroconf_stop_dns_sd_probing_task(void)
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy dns-sd probing task\n");

    if (g_zeroconf_status.dns_sd_probing_proc.status != ZEROCONF_INIT) {
        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.dns_sd_probing_proc.status == ZEROCONF_DEACTIVATED) {
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.dns_sd_probing_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.dns_sd_probing_proc.task_handler);
        }
    }

    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.dns_sd_probing_proc.task_handler = NULL;

    return DA_APP_SUCCESS;
}
#endif // (__SUPPORT_DNS_SD__)

int zeroconf_start_restarter_task(unsigned int type)
{
    int ret = DA_APP_SUCCESS;

    if (!is_running_responder_processor() && !is_running_sender_processor()) {
        ZEROCONF_DEBUG_INFO("zero config is not running\n");
        return DA_APP_NOT_ENABLED;
    }

    if (g_zeroconf_status.restarter_proc.status == ZEROCONF_ACTIVATED) {
        ZEROCONF_DEBUG_ERR("Restarter is running(%d)\n", g_zeroconf_status.restarter_proc.status);
        return DA_APP_IN_PROGRESS;
    }

    ret = zeroconf_stop_restarter_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("failed to stop restart thread(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(restarter_process,
                      ZEROCONF_RESTART_THREAD_NAME,
                      ZEROCONF_RESTART_STACK_SIZE,
                      (void *)type,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.restarter_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create restarter thread(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_stop_restarter_task(void)
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy restarter task\n");

    if (g_zeroconf_status.restarter_proc.status != ZEROCONF_INIT) {
        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.restarter_proc.status == ZEROCONF_DEACTIVATED) {
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.restarter_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.restarter_proc.task_handler);
        }
    }

    g_zeroconf_status.restarter_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.restarter_proc.task_handler = NULL;

    return DA_APP_SUCCESS;

}

int zeroconf_start_responder_task(void)
{
    int ret = DA_APP_SUCCESS;

    ret = zeroconf_stop_responder_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop responder task(0x%x)\n", ret);
        return ret;
    }

    ret = xTaskCreate(responder_process,
                      ZEROCONF_RESP_THREAD_NAME,
                      ZEROCONF_RESP_STACK_SIZE,
                      NULL,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.responder_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create responder task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }


    return DA_APP_SUCCESS;
}

int zeroconf_stop_responder_task(void)
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy responder task(%d)\n",
                        g_zeroconf_status.responder_proc.status);

    if (g_zeroconf_status.responder_proc.status != ZEROCONF_INIT) {
        if (g_zeroconf_status.responder_proc.status == ZEROCONF_ACTIVATED) {
            ZEROCONF_DEBUG_INFO("Change status from %d to %d\n",
                                g_zeroconf_status.responder_proc.status,
                                ZEROCONF_REQ_DEACTIVATED);
            g_zeroconf_status.responder_proc.status = ZEROCONF_REQ_DEACTIVATED;
        }

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.responder_proc.status == ZEROCONF_DEACTIVATED) {
                ZEROCONF_DEBUG_INFO("Changed status(%d)\n",
                                    g_zeroconf_status.responder_proc.status);
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            ZEROCONF_DEBUG_INFO("Failed to stop responder task(%d/%d)\n",
                                max_cnt, wait_option);
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.responder_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.responder_proc.task_handler);
        }
    }

    g_zeroconf_status.responder_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.responder_proc.task_handler = NULL;

    return DA_APP_SUCCESS;
}

int zeroconf_start_sender_task(void)
{
    int ret = DA_APP_SUCCESS;

    ret = zeroconf_stop_sender_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to stop sender task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(sender_process,
                      ZEROCONF_SEND_THREAD_NAME,
                      ZEROCONF_SEND_STACK_SIZE,
                      NULL,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.sender_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create sender task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_stop_sender_task(void)
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy sender task(%d)\n",
                        g_zeroconf_status.sender_proc.status);

    if (g_zeroconf_status.sender_proc.status != ZEROCONF_INIT) {
        if (g_zeroconf_status.sender_proc.status == ZEROCONF_ACTIVATED) {
            ZEROCONF_DEBUG_INFO("Change status from %d to %d\n",
                                g_zeroconf_status.sender_proc.status,
                                ZEROCONF_REQ_DEACTIVATED);
            g_zeroconf_status.sender_proc.status = ZEROCONF_REQ_DEACTIVATED;
        }

        if (g_zeroconf_sender_event_handler) {
            xEventGroupSetBits(g_zeroconf_sender_event_handler, ZEROCONF_EVT_EMPTY);
        }

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.sender_proc.status == ZEROCONF_DEACTIVATED) {
                ZEROCONF_DEBUG_INFO("Changed status(%d)\n",
                                    g_zeroconf_status.sender_proc.status);
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            ZEROCONF_DEBUG_INFO("Failed to stop sender task(%d/%d)\n",
                                max_cnt, wait_option);
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.sender_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.sender_proc.task_handler);
        }
    }

    g_zeroconf_status.sender_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.sender_proc.task_handler = NULL;

    return DA_APP_SUCCESS;
}

#if defined (__SUPPORT_DNS_SD__)
int zeroconf_start_exec_task(char *name_ptr, void (*entry_function)(void *params), void *entry_input)
{
    int ret = 0;

    ZEROCONF_DEBUG_INFO("create exec task(%s)\n", name_ptr);

    ret = zeroconf_stop_exec_task();
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("failed to stop exec task(0x%x)\n", -ret);
        return ret;
    }

    ret = xTaskCreate(entry_function,
                      name_ptr,
                      ZEROCONF_EXEC_STACK_SIZE,
                      entry_input,
                      OS_TASK_PRIORITY_USER,
                      &g_zeroconf_status.dns_sd_exec_proc.task_handler);
    if (ret != pdPASS) {
        ZEROCONF_DEBUG_ERR("Failed to create dns-sd exec task(%d)\n", ret);
        return DA_APP_NOT_CREATED;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_stop_exec_task(void)
{
    const int max_cnt = 10;
    int cnt = 0;
    int wait_option = 100;

    ZEROCONF_DEBUG_INFO("destroy dns-sd exec task(%d)\n",
                        g_zeroconf_status.dns_sd_exec_proc.status);

    if (g_zeroconf_sender_event_handler) {
        vEventGroupDelete(g_zeroconf_sender_event_handler);
        g_zeroconf_sender_event_handler = NULL;
    }

    if (g_zeroconf_status.dns_sd_exec_proc.status != ZEROCONF_INIT) {
        if (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
            ZEROCONF_DEBUG_INFO("Change status from %d to %d\n",
                                g_zeroconf_status.dns_sd_exec_proc.status,
                                ZEROCONF_REQ_DEACTIVATED);
            g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_REQ_DEACTIVATED;
        }

        for (cnt = 0 ; cnt < max_cnt ; cnt++) {
            if (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_DEACTIVATED) {
                ZEROCONF_DEBUG_INFO("Changed status(%d)\n",
                                    g_zeroconf_status.dns_sd_exec_proc.status);
                break;
            }
            vTaskDelay(wait_option);
        }

        if (cnt >= max_cnt) {
            ZEROCONF_DEBUG_INFO("Failed to stop sender task(%d/%d)\n",
                                max_cnt, wait_option);
            return DA_APP_NOT_SUCCESSFUL;
        }

        if (g_zeroconf_status.dns_sd_exec_proc.task_handler) {
            vTaskDelete(g_zeroconf_status.dns_sd_exec_proc.task_handler);
        }
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_INIT;
    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    return DA_APP_SUCCESS;

}
#endif // (__SUPPORT_DNS_SD__)

int zeroconf_set_mdns_config_cmd(int argc, char *argv[])
{
    int nvram_status = 0;
    int index = 0;

    char **cur_argv = ++argv;
    char hostname[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    int isEnabledMdns = pdFALSE;

    if (argc == 1) {
        display_mdns_config_usage();
        return DA_APP_SUCCESS;
    }

    for (index = 1 ; index < argc ; index++, cur_argv++) {
        if ((strncmp(*cur_argv, "-n", 2) == 0)
            || (strcmp(*cur_argv, "-hostname") == 0)) {
            if (--argc < 1) {
                ZEROCONF_DEBUG_ERR("Invaild parameter\n");
                display_mdns_config_usage();
                return DA_APP_INVALID_PARAMETERS;
            }

            ++cur_argv;

            strcpy(hostname, *cur_argv);
            nvram_status = zeroconf_set_mdns_hostname_to_nvram(hostname);
            if (nvram_status != 0) {
                ZEROCONF_DEBUG_ERR("Failed to set up hostname\n");
                return DA_APP_NOT_SUCCESSFUL;
            }
        } else if ((strncmp(*cur_argv, "-e", 2) == 0)
                   || (strcmp(*cur_argv, "-auto-enable") == 0)) {
            if (--argc < 1) {
                ZEROCONF_DEBUG_ERR("Invaild paramter\n");
                display_mdns_config_usage();
                return DA_APP_INVALID_PARAMETERS;
            }

            ++cur_argv;

            if (*cur_argv[0] == 'y') {
                isEnabledMdns = pdTRUE;
            } else if (*cur_argv[0] == 'n') {
                isEnabledMdns = pdFALSE;
            } else {
                ZEROCONF_DEBUG_ERR("Invalid parameter\n");
                display_mdns_config_usage();
                return DA_APP_INVALID_PARAMETERS;
            }

            nvram_status = zeroconf_set_mdns_reg_to_nvram(isEnabledMdns);
            if (nvram_status != 0) {
                ZEROCONF_DEBUG_ERR("Failed to set up auto-enable\n");
                return DA_APP_NOT_SUCCESSFUL;
            }
        } else if ((strncmp(*cur_argv, "-s", 2) == 0)
                   || (strcmp(*cur_argv, "-status") == 0)) {
            zeroconf_get_mdns_reg_from_nvram(&isEnabledMdns);
            zeroconf_get_mdns_hostname_from_nvram(hostname, sizeof(hostname));

            ZEROCONF_PRINTF("Configuration of mDNS:\n"
                            "\thost name:%s\n"
                            "\tauto-enable:%s\n",
                            hostname,
                            (isEnabledMdns == pdTRUE ? "Yes" : "No"));
            return DA_APP_SUCCESS;
        } else if ((*cur_argv[0] == '?')
                   || (strncmp(*cur_argv, "-h", 2) == 0)
                   || (strcmp(*cur_argv, "-help") == 0)) {
            display_mdns_config_usage();
            return DA_APP_SUCCESS;;
        } else {
            ZEROCONF_DEBUG_ERR("Invalid parameter\n");
            display_mdns_config_usage();
            return DA_APP_INVALID_PARAMETERS;
        }
    }

    return DA_APP_SUCCESS;
}

int zeroconf_start_mdns_service_cmd(int argc, char *argv[])
{
    int ret = 0;

    zeroconf_service_type_t service_key = MDNS_SERVICE_TYPE;
    int iface = WLAN0_IFACE;
    char hostname[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};

    ret = parse_mdns_command(argc, argv, service_key, &iface, hostname);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to parse commnad(0x%x)\n", -ret);
        display_mdns_usage();
        return ret;
    }

    if (strlen(hostname) == 0) {
        if ((ret = zeroconf_get_mdns_hostname_from_nvram(hostname, sizeof(hostname))) < 0) {
            ZEROCONF_DEBUG_ERR("Failed to set up hostname(0x%x)\n", -ret);
            return ret;
        }
    } else {
        if ((ret = zeroconf_set_mdns_hostname_to_nvram(hostname)) < 0) {
            ZEROCONF_DEBUG_ERR("Failed to set up hostname(0%x)\n", -ret);
            return ret;
        }
    }

    return zeroconf_start_mdns_service(iface, hostname, pdFALSE);
}

int zeroconf_start_mdns_service(int interface, char *hostname, unsigned int skip_prob)
{
    int ret = 0;

    zeroconf_service_type_t service_key = MDNS_SERVICE_TYPE;
    int iface = interface;

    // Check dpm register
    if (dpm_mode_is_enabled()) {
        ret = dpm_app_is_register(APP_ZEROCONF);
        if (ret == DPM_NOT_REGISTERED) {
            dpm_app_register(APP_ZEROCONF, ZEROCONF_MDNS_PORT);
        }
    }

    if (interface == NONE_IFACE) {
        iface = interface_select;
    }

    if (zeroconf_is_running_mdns_service() || is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("mDNS services are running. Execute mdns-stop to stop\n");
        return DA_APP_IN_PROGRESS;
    }

    if (!is_done_init_processor()) {
        ret = init_zeroconf(iface);
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to init zeroconf(0x%x)\n", -ret);
            return ret;
        }
    } else {
#if defined (__SUPPORT_DNS_SD__)
        if (is_running_dns_sd_probing_processor()) {
            zeroconf_stop_dns_sd_probing_task();
        }
#endif // (__SUPPORT_DNS_SD__)

        zeroconf_stop_sender_task();

        zeroconf_stop_responder_task();
    }

    ret = process_mdns_responder(service_key, hostname, skip_prob);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to start mdns service(0x%x)\n", -ret);
        return ret;
    }

    return ret;
}

int zeroconf_stop_mdns_service(void)
{
    int ret = DA_APP_SUCCESS;
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (zeroconf_is_running_mdns_service()) {
        if (is_running_mdns_probing_processor()) {
            ZEROCONF_DEBUG_INFO("system is probing.\n");
            return DA_APP_IN_PROGRESS;
#if defined (__SUPPORT_DNS_SD__)
        } else if (is_running_dns_sd_probing_processor()) {
            zeroconf_stop_dns_sd_probing_task();
#endif // (__SUPPORT_DNS_SD__)
        }

        send_ttl_zero(MDNS_SERVICE_TYPE);
        zeroconf_stop_sender_task();
        zeroconf_stop_responder_task();

        ZEROCONF_DEBUG_INFO("clear previous entry from mdns resp cache.\n");

        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                                                     hostname_bytes,
                                                     ZEROCONF_DOMAIN_NAME_SIZE);

        deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0, ZEROCONF_MDNS_RR_TYPE_ALL);

        ZEROCONF_DEBUG_INFO("clear mdns send cache.\n");

        clear_send_cache(MDNS_SERVICE_TYPE);

        if (!zeroconf_is_running_xmdns_service()) {
            ZEROCONF_DEBUG_INFO("clear query cache as xmdns not running.\n");
            clear_query_cache();

            ZEROCONF_DEBUG_INFO("clear response cache as xmdns not running.\n");
            clear_mdns_cache();
        } else {
            ZEROCONF_DEBUG_INFO("starting sender & responder thread for xmDNS.\n");

            zeroconf_start_responder_task();

            zeroconf_start_sender_task();
        }

        set_mdns_service_status(ZEROCONF_INIT);

        ZEROCONF_DEBUG_INFO("mDNS services stopped.\n");
    } else {
        ZEROCONF_DEBUG_INFO("mDNS services not running!\n");
        return DA_APP_NOT_ENABLED;
    }

    if (!zeroconf_is_running_xmdns_service()) {
        ret = deinit_zeroconf();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to deinit zeroconf(0x%x)\n", -ret);
            return ret;
        }
    }

    return ret;
}

int zeroconf_change_mdns_hostname_cmd(int argc, char *argv[])
{
    zeroconf_service_type_t service_key = MDNS_SERVICE_TYPE;
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (!zeroconf_is_running_mdns_service()) {
        ZEROCONF_DEBUG_INFO("mDNS services not running!\n");
        return DA_APP_NOT_ENABLED;
    }

    if (argc == 2) {
        to_lower(argv[1]);

        if (strstr(argv[1], mdns_top_domain) == NULL) {
            if (is_running_mdns_probing_processor()) {
                ZEROCONF_DEBUG_INFO("system is probing.\n");
                return DA_APP_IN_PROGRESS;
#if defined ( __SUPPORT_DNS_SD__ )
            } else if (is_running_dns_sd_probing_processor()) {
                zeroconf_stop_dns_sd_probing_task();
#endif // __SUPPORT_DNS_SD__
            }

            zeroconf_stop_sender_task();
            zeroconf_stop_responder_task();

            ZEROCONF_DEBUG_INFO("clear mdns send cache.\n");
            clear_send_cache(service_key);

            ZEROCONF_DEBUG_INFO("clear previous entry from mdns resp cache.\n");
            hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                                                         hostname_bytes,
                                                         ZEROCONF_DOMAIN_NAME_SIZE);


            deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0, ZEROCONF_MDNS_RR_TYPE_ALL);
            send_ttl_zero(service_key);
            set_mdns_service_status(ZEROCONF_INIT);
            process_mdns_responder(service_key, argv[1], pdFALSE);
        }
    } else {
        ZEROCONF_DEBUG_ERR("mdns-change-name command format : "
                           "mdns-change-name [new host-name] "
                           "(host-name should not contain .local)\n");

        return DA_APP_INVALID_PARAMETERS;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_lookup_mdns_cmd(int argc, char *argv[])
{
    if (is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("system is probing.\n");
        return DA_APP_IN_PROGRESS;
    }

    if (!zeroconf_is_running_mdns_service()) {
        ZEROCONF_DEBUG_ERR("mDNS Services not running\n");
        return DA_APP_NOT_ENABLED;
    }

    if (argc >= 2) {
        mdns_Lookup(MDNS_SERVICE_TYPE, argc, argv);
    } else {
        ZEROCONF_DEBUG_ERR("mdns-lookup command format : "
                           "mdns-lookup [domains seperated by space]\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_print_status_cmd(int argc, char *argv[])
{
    if (is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("system is probing.\n");
        return DA_APP_IN_PROGRESS;
    }

    ZEROCONF_PRINTF("mDNS Responder Service     :  ");

    if (zeroconf_is_running_mdns_service()) {
        ZEROCONF_PRINTF("Running ");

        if (strlen(g_zeroconf_mdns_host_info.hostname) == 0) {
            ZEROCONF_PRINTF(" <Host name not set>\n");
        } else {
            ZEROCONF_PRINTF("(%s)\n", g_zeroconf_mdns_host_info.hostname);
        }
    } else {
        ZEROCONF_PRINTF("Stopped\n");
    }

#if defined (__SUPPORT_XMDNS__)
    ZEROCONF_PRINTF("xmDNS Responder Service    :  ");

    if (zeroconf_is_running_xmdns_service()) {
        ZEROCONF_PRINTF("Running ");

        if (strlen(g_zeroconf_xmdns_host_info.hostname) == 0) {
            ZEROCONF_PRINTF(" <Host name not set>\n");
        } else {
            ZEROCONF_PRINTF("(%s)\n", g_zeroconf_xmdns_host_info.hostname);
        }
    } else {
        ZEROCONF_PRINTF("Stopped\n");
    }
#endif // (__SUPPORT_XMDNS__)

    ZEROCONF_PRINTF("zeroconf Responder Service :  ");

    if (is_running_responder_processor()) {
        ZEROCONF_PRINTF("Running\n");
    } else {
        ZEROCONF_PRINTF("Stopped\n");
    }

    ZEROCONF_PRINTF("zeroconf Sender Service    :  ");

    if (is_running_sender_processor()) {
        ZEROCONF_PRINTF("Running\n");
    } else {
        ZEROCONF_PRINTF("Stopped\n");
    }

    ZEROCONF_PRINTF("zeroconf Cache Details     :  ");

    viewStructHeader();

    //display_dpm_mdns_info();

    if ((argc == 2) && (strcmp(argv[1], "-debug") == 0)) {
        ZEROCONF_PRINTF("\n\n>>>> Zeroconf Status\n");

        ZEROCONF_PRINTF("%-30s: %d msec(def:%d)\n",
                        "Prob Timeout",
                        zeroconf_get_mdns_prob_timeout(),
                        ZEROCONF_DEF_PROB_TIMEOUT);

        ZEROCONF_PRINTF("%-30s: %d msec(def:%d)\n",
                        "Prob Tie-breaker Timeout",
                        zeroconf_get_mdns_prob_tie_breaker_timeout(),
                        ZEROCONF_DEF_PROB_TIE_BREAKER_TIMEOUT);

        ZEROCONF_PRINTF("%-30s: %d msec(def:%d)\n",
                        "Prob Conflict Timeout",
                        zeroconf_get_mdns_prob_conflict_timeout(),
                        ZEROCONF_DEF_PROB_CONFLICT_TIMEOUT);

        if (zeroconf_is_running_mdns_service()) {
            ZEROCONF_PRINTF("%-30s: ", "MDNS Hostname");

            if (strlen(g_zeroconf_mdns_host_info.hostname) == 0) {
                ZEROCONF_PRINTF("<Host name not set>\n");
            } else {
                ZEROCONF_PRINTF("%s\n", g_zeroconf_mdns_host_info.hostname);
            }
        }

#if defined (__SUPPORT_DNS_SD__)
        if (zeroconf_is_running_dns_sd_service()) {
            printLocalDnsSd();
        }
#endif // (__SUPPORT_DNS_SD__)
    }

    return DA_APP_SUCCESS;
}

void zeroconf_print_cache_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    ZEROCONF_PRINTF(">>>>>>CACHE DETAILS - \n\n");
    ZEROCONF_PRINTF(">>");
    viewStructHeader();
    ZEROCONF_PRINTF("\n");

    ZEROCONF_PRINTF(">>>>>>RESPONSE CACHE - \n\n");
    viewMdnsResponses();

    ZEROCONF_PRINTF(">>>>>>QUERY CACHE - \n\n");
    viewQueryStrings();

    ZEROCONF_PRINTF(">>>>>>SEND DATA BUFFER - \n\n");
    viewPckSends();
}

#if defined (__SUPPORT_XMDNS__)
int zeroconf_start_xmdns_service_cmd(int argc, char *argv[])
{
    int ret = 0;

    zeroconf_service_type_t service_key = XMDNS_SERVICE_TYPE;
    int iface = WLAN0_IFACE;
    char hostname[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};

    ret = parse_mdns_command(argc, argv, service_key, &iface, hostname);
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("Failed to parse commnad(0x%x)\n", -ret);
        display_xmdns_usage();
        return ret;
    }

    if (strlen(hostname)) {
        ZEROCONF_DEBUG_ERR("Please, input host name\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    return zeroconf_start_xmdns_service(iface, hostname, pdFALSE);
}

int zeroconf_start_xmdns_service(int interface, char *hostname, unsigned int skip_prob)
{
    int ret = DA_APP_SUCCESS;

    zeroconf_service_type_t service_key = XMDNS_SERVICE_TYPE;
    int iface = interface;

    if (interface == NONE_IFACE) {
        iface = interface_select;
    }

    if (zeroconf_is_running_xmdns_service()
        || is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("xmDNS services are running. Execute xmdns-stop to stop\n");
        return DA_APP_IN_PROGRESS;
    }

    if (!is_done_init_processor()) {
        ret = init_zeroconf(iface);
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to init zeroconf(0x%x)\n", -ret);
            return ret;
        }
    } else {
        zeroconf_stop_mdns_probing_task();
        zeroconf_stop_sender_task();
        zeroconf_stop_responder_task();
    }

    ret = process_mdns_responder(service_key, hostname, skip_prob);
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("failed to start xmdns service(0x%x)\n", -ret);
        return ret;
    }

    return ret;
}

int zeroconf_stop_xmdns_service(void)
{
    int ret = DA_APP_SUCCESS;
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (zeroconf_is_running_xmdns_service()) {
        if (is_running_mdns_probing_processor()) {
            ZEROCONF_DEBUG_INFO("system is probing.\n");
            return DA_APP_IN_PROGRESS;
#if defined (__SUPPORT_DNS_SD__)
        } else if (is_running_dns_sd_probing_processor()) {
            zeroconf_stop_dns_sd_probing_task();
#endif // (__SUPPORT_DNS_SD__)
        }

        send_ttl_zero(XMDNS_SERVICE_TYPE);
        zeroconf_stop_sender_task();
        zeroconf_stop_responder_task();

        ZEROCONF_DEBUG_INFO("clear previous entry from xmdns resp cache.\n");
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_xmdns_host_info.hostname,
                                                     hostname_bytes,
                                                     ZEROCONF_DOMAIN_NAME_SIZE);

        deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0, ZEROCONF_MDNS_RR_TYPE_ALL);

        ZEROCONF_DEBUG_INFO("clear xmdns send cache.\n");
        clear_send_cache(XMDNS_SERVICE_TYPE);

        if (!zeroconf_is_running_mdns_service()) {
            ZEROCONF_DEBUG_INFO("clear query cache as mdns not running.\n");
            clear_query_cache();

            ZEROCONF_DEBUG_INFO("clear response cache as mdns not running.\n");
            clear_mdns_cache();
        } else {
            ZEROCONF_DEBUG_INFO("starting sender & responder thread for mDNS.\n");

            zeroconf_start_responder_task();
            zeroconf_start_sender_task();
        }

        set_xmdns_service_status(ZEROCONF_INIT);

        ZEROCONF_DEBUG_INFO("xmDNS services stopped.\n");
    } else {
        ZEROCONF_DEBUG_INFO("xmDNS services not running!\n");
        return DA_APP_NOT_ENABLED;
    }

    if (!zeroconf_is_running_mdns_service()) {
        ret = deinit_zeroconf();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to deinit zeroconf(0x%x)\n", -ret);
            return ret;
        }
    }

    return ret;
}

int zeroconf_change_xmdns_hostname_cmd(int argc, char *argv[])
{
    zeroconf_service_type_t service_key = XMDNS_SERVICE_TYPE;
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (!zeroconf_is_running_xmdns_service()) {
        ZEROCONF_DEBUG_INFO("xmDNS services not running!\n");
        return DA_APP_NOT_ENABLED;
    }

    if (argc == 2) {
        to_lower(argv[1]);

        if (strstr(argv[1], xmdns_top_domain) == NULL) {
            if (is_running_mdns_probing_processor()) {
                ZEROCONF_DEBUG_INFO("system is probing.\n");
                return DA_APP_IN_PROGRESS;
#if defined (__SUPPORT_DNS_SD__)
            } else if (is_running_dns_sd_probing_processor()) {
                zeroconf_stop_dns_sd_probing_task();
#endif // (__SUPPORT_DNS_SD__)
            }

            zeroconf_stop_sender_task();

            zeroconf_stop_responder_task();

            ZEROCONF_DEBUG_INFO("clear xmdns send cache.\n");

            clear_send_cache(service_key);

            ZEROCONF_DEBUG_INFO("clear previous entry from xmdns resp cache.\n");

            hostname_bytes_len = encode_mdns_name_string(g_zeroconf_xmdns_host_info.hostname,
                                                         hostname_bytes,
                                                         ZEROCONF_DOMAIN_NAME_SIZE);

            deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0,
                           ZEROCONF_MDNS_RR_TYPE_ALL);

            send_ttl_zero(service_key);

            set_xmdns_service_status(ZEROCONF_INIT);

            process_mdns_responder(service_key, argv[1], pdFALSE);
        }
    } else {
        ZEROCONF_PRINTF("xmdns-change-name command format : "
                        "xmdns-change-name [new host-name] "
                        "(host-name should not contain .site)\n");
    }

    return DA_APP_SUCCESS;
}

int zeroconf_lookup_xmdns_cmd(int argc, char *argv[])
{
    if (is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("system is probing.\n");
        return DA_APP_IN_PROGRESS;
    }

    if (!zeroconf_is_running_xmdns_service() && !is_running_sender_processor()) {
        ZEROCONF_DEBUG_ERR("xmDNS Services not running\n");
        return DA_APP_NOT_ENABLED;
    }

    if (argc >= 2) {
        mdns_Lookup(XMDNS_SERVICE_TYPE, argc, argv);
    } else {
        ZEROCONF_PRINTF("xmdns-lookup command format : "
                        "xmdns-lookup [domains seperated by space]\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    return DA_APP_SUCCESS;
}
#endif // (__SUPPORT_XMDNS__)


/*##############################################
    mdns handler methods
  ##############################################*/

/******************************************************************************
 *  mdns_Query( )
 *
 *  Purpose :   send mdns query and return the IP of the domain name
 *  Input   :   mdns domain name e.g "da16x.local"
 *  Return  :   ip address is string e.g. "127.0.0.1"
 ******************************************************************************/
int mdns_Query(const zeroconf_service_type_t service_key, char *domain_name, char *ipaddr)
{
    char ipaddr_str[IPADDR_LEN] = {0x00,};
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    size_t req_packet_len = 0;
    unsigned char *domain_bytes = NULL;
    size_t domain_bytes_len = 0;
    int retry_count = 0, len = 0;
    int terminate = 0;
    zeroconf_mdns_response_t *temp = NULL;
    int flag = 0, ch_res = -1;

    DA16X_UNUSED_ARG(service_key);

    memset(ipaddr, 0x00, IPADDR_LEN);

    if (strlen(domain_name) >= ZEROCONF_DOMAIN_NAME_SIZE) {
        ZEROCONF_DEBUG_ERR("invalid domain name\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    to_lower((char *)domain_name);

    domain_bytes = zeroconf_calloc((strlen(domain_name) + 2), 1);
    if (domain_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for domain bytes\n");
        return DA_APP_MALLOC_ERROR;
    }

    domain_bytes_len = encode_mdns_name_string(domain_name,
                       domain_bytes,
                       ZEROCONF_DOMAIN_NAME_SIZE);

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);

    req_packet_len = ZEROCONF_MDNS_HEADER_SIZE;

    req_packet_len += create_query(domain_bytes,
                                   domain_bytes_len,
                                   ZEROCONF_MDNS_RR_TYPE_A,
                                   1,
                                   req_packet,
                                   req_packet_len);

    insertQueryString(domain_bytes, domain_bytes_len);

    while (pdTRUE) {
        flag = 0;

        if (g_zeroconf_mdns_response_root != NULL) {
            temp = g_zeroconf_mdns_response_root;

            while (temp != NULL) {
                ch_res = search_in_byte_array(temp->query, temp->query_len,
                                              domain_bytes, domain_bytes_len);
                if ((ch_res == pdTRUE) && (temp->type == ZEROCONF_MDNS_RR_TYPE_A)) {
                    flag = 1;
                    break;
                }

                temp = temp->next;
            }
        }

        if (flag == 1) {
            sprintf(ipaddr_str, "%d.%d.%d.%d",
                    temp->data[0], temp->data[1],
                    temp->data[2], temp->data[3]);

            terminate = 1;
        }

        if (terminate) {
            ZEROCONF_DEBUG_TEMP("Host found: %s\n", ipaddr_str);
            break;
        }

        if (retry_count == 3) {
            ZEROCONF_DEBUG_INFO("Maximum retry(%d) exceeded. Unable to find host\n",
                                retry_count);
            break;
        }

        if (retry_count > 0) {
            vTaskDelay(800);
        }

        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

        retry_count++;

        vTaskDelay(200);
    }

    deleteQueryString(domain_bytes, domain_bytes_len);

    zeroconf_free(domain_bytes);

    memcpy(ipaddr, ipaddr_str, IPADDR_LEN);

    return DA_APP_SUCCESS;
}

/******************************************************************************
*  mdns_Lookup( )
*
*  Purpose :   looks up for multiple mdns hosts
******************************************************************************/
void mdns_Lookup(const zeroconf_service_type_t service_key, int argc, char *argv[])
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    unsigned char domain_bytes_arr[5][ZEROCONF_DOMAIN_NAME_SIZE] = {{0x00,},};
    size_t domain_bytes_len_arr[5] = {0x00,};
    char *domain_name = NULL;
    char *to_print = NULL;
    int i = 0, k = 0;
    int len = 0;
    zeroconf_mdns_response_t *temp = NULL;
    int ch_res = -1, rec_c = 0, isOwn = 0;

    int domain_name_length = 0;

    const char *top_domain;
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (service_key == MDNS_SERVICE_TYPE) {
        top_domain = mdns_top_domain;
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                             hostname_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);
    }
#if defined (__SUPPORT_XMDNS__)
    else if (service_key == XMDNS_SERVICE_TYPE) {
        top_domain = xmdns_top_domain;
        hostname_bytes_len = encode_mdns_name_string(
                                 g_zeroconf_xmdns_host_info.hostname,
                                 hostname_bytes,
                                 ZEROCONF_DOMAIN_NAME_SIZE);
    }
#endif // (__SUPPORT_XMDNS__)
    else {
        return;
    }

    for (i = 1 ; i < argc ; i++) {
        if (k == 5) {
            break;
        }

        domain_name = argv[i];
        domain_name_length = strlen(domain_name);

        if (domain_name_length >= ZEROCONF_DOMAIN_NAME_SIZE) {
            continue;
        }

        to_lower(domain_name);

        if (service_key == MDNS_SERVICE_TYPE) {
            if (strcmp(domain_name + domain_name_length - 6, top_domain) != 0) {
                continue;
            }

            if ((strcmp(domain_name + domain_name_length - 11, "._tcp.local") == 0)
                || (strcmp(domain_name + domain_name_length - 11, "._udp.local") == 0)) {
                ZEROCONF_DEBUG_ERR("ERROR: Service Discovery "
                                   "not supported in this command."
                                   "Use dns-sd commands instead.\n");
                return;
            }
        } else if (service_key == XMDNS_SERVICE_TYPE) {
            if (strcmp(domain_name + domain_name_length - 5, top_domain) != 0) {
                continue;
            }

            if ((strcmp(domain_name + domain_name_length - 10, "._tcp.local") == 0)
                || (strcmp(domain_name + domain_name_length - 10, "._udp.local") == 0)) {
                ZEROCONF_DEBUG_ERR("ERROR: Service Discovery "
                                   "not supported in this command."
                                   "Use dns-sd commands instead.\n");
                return;
            }
        }

        domain_bytes_len_arr[k] = encode_mdns_name_string(domain_name,
                                  domain_bytes_arr[k],
                                  ZEROCONF_DOMAIN_NAME_SIZE);

        k++;
    }

    if (k == 0) {
        ZEROCONF_DEBUG_ERR("ERROR: No valid names entered.\n");
        return;
    }

    create_packet_header(k, 0, 0, 0, 1, req_packet, 0);

    len = ZEROCONF_MDNS_HEADER_SIZE;

    for (i = 0 ; i < 5 ; i++) {
        isOwn = 0;

        if (domain_bytes_len_arr[i] > 0) {
            if (compare_domain_bytes(hostname_bytes,
                                     hostname_bytes_len,
                                     domain_bytes_arr[i],
                                     domain_bytes_len_arr[i])) {
                isOwn = 1;
            }

            if (!isOwn) {
                len += create_query(domain_bytes_arr[i],
                                    domain_bytes_len_arr[i],
                                    ZEROCONF_MDNS_RR_TYPE_ALL,
                                    1,
                                    req_packet, len);

                insertQueryString(domain_bytes_arr[i],
                                  domain_bytes_len_arr[i]);
            }
        }
    }

    isOwn = 0;

    send_mdns_packet(service_key, req_packet, len);

    ZEROCONF_PRINTF("[mdns Lookup] Query Sent. Waiting for responses.. \n");

    vTaskDelay(2000);

    ZEROCONF_PRINTF("\nResponses : \n");

    for (i = 0 ; i < 5 ; i++) {
        isOwn = 0;
        rec_c = 0;

        if (domain_bytes_len_arr[i] > 0) {
            to_print = zeroconf_calloc(domain_bytes_len_arr[i], 1);
            if (to_print == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory for print\n");
                break;
            }

            if (unencode_mdns_name_string(domain_bytes_arr[i],
                                          0,
                                          to_print,
                                          domain_bytes_len_arr[i])) {
                ZEROCONF_PRINTF("\n*** Domain : %s ****\n", to_print);
            }

            zeroconf_free(to_print);
            to_print = NULL;

            if (g_zeroconf_mdns_response_root != NULL) {
                temp = g_zeroconf_mdns_response_root;

                while (temp != NULL) {
                    ch_res = search_in_byte_array(temp->query,
                                                  temp->query_len,
                                                  domain_bytes_arr[i],
                                                  domain_bytes_len_arr[i]);
                    if (ch_res) {
                        rec_c++;
                        ZEROCONF_PRINTF("\n#Record : %d ", rec_c);
                        print_mdns_response(temp, 0, 0);
                    }

                    temp = temp->next;
                }
            }

            if (rec_c == 0) {
                ZEROCONF_PRINTF("<No records for this domain>\n");
            }

            ZEROCONF_PRINTF("**** end of domain ****\n");
        }

        if (compare_domain_bytes(hostname_bytes,
                                 hostname_bytes_len,
                                 domain_bytes_arr[i],
                                 domain_bytes_len_arr[i])) {
            isOwn = 1;
        }

        if (!isOwn) {
            deleteQueryString(domain_bytes_arr[i],
                              domain_bytes_len_arr[i]);
        }
    }

    return;
}

int generate_response(zeroconf_response_type_t response_type,
                      zeroconf_service_type_t rprotocol,
                      unsigned char *rname_bytes, size_t rname_bytes_len,
                      unsigned short rtype,
                      zeroconf_resource_record_t *known_answer_rrs,
                      zeroconf_resource_record_t **out_answer_rrs,
                      unsigned short *out_answer_rrs_count,
                      zeroconf_resource_record_t **out_additional_rrs,
                      unsigned short *out_additional_rrs_count)
{
    int ret = 0;

    unsigned char str_ipv4[4] = {0x00,};
    get_ipv4_bytes(str_ipv4);

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[16] = {0x00,};
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned short class = 1;
    unsigned short cache_flush_bit = 0;
    unsigned int ttl = 0;

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    unsigned char arpa_v4_bytes[ZEROCONF_ARPA_NAME_SIZE] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char arpa_v6_bytes[ZEROCONF_ARPA_NAME_SIZE] = {0x00,};
#endif // (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF) 

    size_t hostname_bytes_len = 0;
    size_t arpa_v4_bytes_len = 0;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    size_t arpa_v6_bytes_len = 0;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    if (rprotocol == MDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                             hostname_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);

        if (is_supporting_ipv4()) {
            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_mdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_mdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#if defined (__SUPPORT_XMDNS__)
    } else if (rprotocol == XMDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(
                                 g_zeroconf_xmdns_host_info.hostname,
                                 hostname_bytes,
                                 ZEROCONF_DOMAIN_NAME_SIZE);

        if (is_supporting_ipv4()) {
            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#endif // (__SUPPORT_XMDNS__)
    } else {
        return DA_APP_INVALID_PARAMETERS;
    }

    if (compare_domain_bytes(hostname_bytes, hostname_bytes_len,
                             rname_bytes, rname_bytes_len)) {
        ttl = 120;
        class = 1;

        if (response_type == UNICAST_RESPONSE_TYPE) {
            cache_flush_bit = 0;
        } else {
            cache_flush_bit = 1;
        }

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_ALL)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_A)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_AAAA)) {
            if (is_supporting_ipv4()) {
                ret = insert_resource_record(out_answer_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_A,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv4),
                                             str_ipv4);
                if (ret == DA_APP_SUCCESS) {
                    *out_answer_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                    return ret;
                }
            }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                ret = insert_resource_record(out_answer_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_AAAA,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv6),
                                             str_ipv6);
                if (ret == DA_APP_SUCCESS) {
                    *out_answer_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                    return ret;
                }
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
        }
    } else if (compare_domain_bytes(arpa_v4_bytes, arpa_v4_bytes_len,
                                    rname_bytes, rname_bytes_len)) {

        ttl = 120;
        class = 1;
        cache_flush_bit = 0;

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        ret = insert_resource_record(out_answer_rrs,
                                     arpa_v4_bytes,
                                     arpa_v4_bytes_len,
                                     ZEROCONF_MDNS_RR_TYPE_PTR,
                                     ((cache_flush_bit << 15) | class),
                                     ttl,
                                     hostname_bytes_len,
                                     hostname_bytes);
        if (ret == DA_APP_SUCCESS) {
            *out_answer_rrs_count += 1;

            ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
        } else if (ret != DA_APP_DUPLICATED_ENTRY) {
            ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
            return ret;
        }
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    } else if (compare_domain_bytes(arpa_v6_bytes, arpa_v6_bytes_len,
                                    rname_bytes, rname_bytes_len)) {

        ttl = 120;
        class = 1;
        cache_flush_bit = 0;

        if (is_known_answer_rr(known_answer_rrs,
                               rname_bytes, rname_bytes_len,
                               ttl)) {
            return DA_APP_SUCCCESS;
        }

        ret = insert_resource_record(out_answer_rrs,
                                     arpa_v6_bytes,
                                     arpa_v6_bytes_len,
                                     ZEROCONF_MDNS_RR_TYPE_PTR,
                                     ((cache_flush_bit << 15) | class),
                                     ttl,
                                     hostname_bytes_len,
                                     hostname_bytes);
        if (ret == DA_APP_SUCCESS) {
            *out_answer_rrs_count += 1;

            ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
        } else if (ret != DA_APP_DUPLICATED_ENTRY) {
            ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
            return ret;
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#if defined (__SUPPORT_DNS_SD__)
    } else if (compare_domain_bytes(g_zeroconf_local_dns_sd_service->t_bytes,
                                    g_zeroconf_local_dns_sd_service->t_bytes_len,
                                    rname_bytes, rname_bytes_len)) {
        ttl = 120;
        class = 1;
        cache_flush_bit = 0;

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_PTR)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {

            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_local_dns_sd_service->t_bytes,
                                         g_zeroconf_local_dns_sd_service->t_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_PTR,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         g_zeroconf_local_dns_sd_service->q_bytes_len,
                                         g_zeroconf_local_dns_sd_service->q_bytes);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }

            //set data for svr type
            ttl = 4500;
            class = 1;

            if (response_type == UNICAST_RESPONSE_TYPE) {
                cache_flush_bit = 0;
            } else {
                cache_flush_bit = 1;
            }

            ret = insert_srv_record(out_answer_rrs,
                                    g_zeroconf_local_dns_sd_service->q_bytes,
                                    g_zeroconf_local_dns_sd_service->q_bytes_len,
                                    ((cache_flush_bit << 15) | class),
                                    ttl,
                                    0,
                                    0,
                                    g_zeroconf_local_dns_sd_service->port,
                                    hostname_bytes_len,
                                    hostname_bytes);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }

            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_local_dns_sd_service->q_bytes,
                                         g_zeroconf_local_dns_sd_service->q_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_TXT,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         g_zeroconf_local_dns_sd_service->txtLen,
                                         g_zeroconf_local_dns_sd_service->txtRecord);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }

            if (is_supporting_ipv4()) {
                ttl = 120;
                class = 1;

                if (response_type == UNICAST_RESPONSE_TYPE) {
                    cache_flush_bit = 0;
                } else {
                    cache_flush_bit = 1;
                }

                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_A,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv4),
                                             str_ipv4);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_AAAA,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv6),
                                             str_ipv6);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
        }
    } else if (compare_domain_bytes(g_zeroconf_local_dns_sd_service->q_bytes,
                                    g_zeroconf_local_dns_sd_service->q_bytes_len,
                                    rname_bytes, rname_bytes_len)) {
        ttl = 4500;
        class = 1;

        if (response_type == UNICAST_RESPONSE_TYPE) {
            cache_flush_bit = 0;
        } else {
            cache_flush_bit = 1;
        }

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_SRV)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ret = insert_srv_record(out_answer_rrs,
                                    g_zeroconf_local_dns_sd_service->q_bytes,
                                    g_zeroconf_local_dns_sd_service->q_bytes_len,
                                    ((cache_flush_bit << 15) | class),
                                    ttl,
                                    0,
                                    0,
                                    g_zeroconf_local_dns_sd_service->port,
                                    hostname_bytes_len,
                                    hostname_bytes);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(%d)\n", -ret);
                return ret;
            }
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_TXT)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {

            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_local_dns_sd_service->q_bytes,
                                         g_zeroconf_local_dns_sd_service->q_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_TXT,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         g_zeroconf_local_dns_sd_service->txtLen,
                                         g_zeroconf_local_dns_sd_service->txtRecord);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_SRV)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_TXT)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {

            ttl = 120;
            class = 1;
            cache_flush_bit = 0;

            if (is_supporting_ipv4()) {
                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_A,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv4),
                                             str_ipv4);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_AAAA,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv6),
                                             str_ipv6);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF)
        }
    } else if (compare_domain_bytes(g_zeroconf_proxy_dns_sd_svc->t_bytes,
                                    g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                                    rname_bytes, rname_bytes_len)) {

        ttl = 120;
        class = 1;
        cache_flush_bit = 0;

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_PTR)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_proxy_dns_sd_svc->t_bytes,
                                         g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_PTR,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                         g_zeroconf_proxy_dns_sd_svc->q_bytes);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }
        }
    } else if (compare_domain_bytes(g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                    g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                    rname_bytes, rname_bytes_len)) {
        ttl = 4500;
        class = 1;

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if (response_type == UNICAST_RESPONSE_TYPE) {
            cache_flush_bit = 0;
        } else {
            cache_flush_bit = 1;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_SRV)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ret = insert_srv_record(out_answer_rrs,
                                    g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                    g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                    ((cache_flush_bit << 15) | class),
                                    ttl,
                                    0,
                                    0,
                                    g_zeroconf_proxy_dns_sd_svc->port,
                                    g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                    g_zeroconf_proxy_dns_sd_svc->h_bytes);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_TXT)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                         g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_TXT,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         g_zeroconf_proxy_dns_sd_svc->txtLen,
                                         g_zeroconf_proxy_dns_sd_svc->txtRecord);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_SRV)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_TXT)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ttl = 120;
            class = 1;
            cache_flush_bit = 0;

            if (is_supporting_ipv4()) {
                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_A,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv4),
                                             str_ipv4);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                ret = insert_resource_record(out_additional_rrs,
                                             hostname_bytes,
                                             hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_AAAA,
                                             ((cache_flush_bit << 15) | class),
                                             ttl,
                                             sizeof(str_ipv6),
                                             str_ipv6);
                if (ret == DA_APP_SUCCESS) {
                    *out_additional_rrs_count += 1;

                    ZEROCONF_DEBUG_TEMP("add additional_rr:%d\n",
                                        *out_additional_rrs_count);
                } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                    ZEROCONF_DEBUG_ERR("Failed to add additional_rr(0x%x)\n", -ret);
                    return ret;
                }
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
        }
    } else if (compare_domain_bytes(g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                    g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                    rname_bytes, rname_bytes_len)) {
        ttl = 120;
        class = 1;

        if (is_known_answer_rr(known_answer_rrs, rname_bytes, rname_bytes_len, ttl)) {
            return DA_APP_SUCCESS;
        }

        if (response_type == UNICAST_RESPONSE_TYPE) {
            cache_flush_bit = 0;
        } else {
            cache_flush_bit = 1;
        }

        if ((rtype == ZEROCONF_MDNS_RR_TYPE_A)
            || (rtype == ZEROCONF_MDNS_RR_TYPE_ALL)) {
            ret = insert_resource_record(out_answer_rrs,
                                         g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                         g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_A,
                                         ((cache_flush_bit << 15) | class),
                                         ttl,
                                         sizeof(g_zeroconf_proxy_dns_sd_svc->h_ip),
                                         g_zeroconf_proxy_dns_sd_svc->h_ip);
            if (ret == DA_APP_SUCCESS) {
                *out_answer_rrs_count += 1;

                ZEROCONF_DEBUG_TEMP("add answer_rr:%d\n", *out_answer_rrs_count);
            } else if (ret != DA_APP_DUPLICATED_ENTRY) {
                ZEROCONF_DEBUG_ERR("Failed to add answer_rr(0x%x)\n", -ret);
                return ret;
            }
        }
#endif // (__SUPPORT_DNS_SD__)
    }

    DA16X_UNUSED_ARG(out_additional_rrs);
    DA16X_UNUSED_ARG(out_additional_rrs_count);

    return DA_APP_SUCCESS;
}

int send_multi_responses(zeroconf_service_type_t protocol, zeroconf_mdns_packet_t *payload)
{
    int ret = DA_APP_SUCCESS;
    unsigned char data[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    size_t data_length = 0;

    unsigned int delay = 0;
    zeroconf_mdns_packet_t send_mdns_packet;
    zeroconf_question_t *question = NULL;
    zeroconf_resource_record_t *rrecord = NULL;

    zeroconf_peer_info_t ipv4_addr;
    zeroconf_peer_info_t ipv6_addr;

    memset(&send_mdns_packet, 0x00, sizeof(zeroconf_mdns_packet_t));

    memset(&ipv4_addr, 0x00, sizeof(zeroconf_peer_info_t));
    memset(&ipv6_addr, 0x00, sizeof(zeroconf_peer_info_t));

#if 0    /* not used. */
    unsigned int is_qu_question = pdFALSE;
#endif

    if (payload->questions_count) {
        for (question = payload->questions ; question != NULL ;
             question = question->next) {
            ret = generate_response(payload->peer_ip.type,
                                    protocol,
                                    question->name_bytes,
                                    question->name_bytes_len,
                                    question->type,
                                    payload->answer_rrs,
                                    &send_mdns_packet.answer_rrs,
                                    &send_mdns_packet.answer_rrs_count,
                                    &send_mdns_packet.additional_rrs,
                                    &send_mdns_packet.additional_rrs_count);
            if (ret != DA_APP_SUCCESS) {
                ZEROCONF_DEBUG_ERR("Failed to generate response(0x%x)\n", -ret);
            }

#if 0    /* not used. */
            if (question->qclass & 0x8000) {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR
                                    "QU question is true\n"
                                    CLEAR_COLOR);

                is_qu_question = pdTRUE;
            }
#endif
        }
    } else if (payload->answer_rrs_count) {
        for (rrecord = payload->answer_rrs ; rrecord != NULL ;
             rrecord = rrecord->next) {

            ret = generate_response(payload->peer_ip.type,
                                    protocol,
                                    rrecord->name_bytes,
                                    rrecord->name_bytes_len,
                                    rrecord->type,
                                    payload->answer_rrs,
                                    &send_mdns_packet.answer_rrs,
                                    &send_mdns_packet.answer_rrs_count,
                                    &send_mdns_packet.additional_rrs,
                                    &send_mdns_packet.additional_rrs_count);

            if (ret != DA_APP_SUCCESS) {
                ZEROCONF_DEBUG_ERR("Failed to generate response(0x%x)\n", -ret);
            }
        }
    }

    //set response flags
    send_mdns_packet.flags = 0x8400;

    data_length = convert_mdns_packet_to_bytes(&send_mdns_packet,
                  data,
                  ZEROCONF_DEF_BUF_SIZE - 1);
    if (data_length == 0) {
        ZEROCONF_DEBUG_INFO("Empty response\n");
        goto error;
    } else {
        if (send_mdns_packet.answer_rrs_count) {
            delay = calculate_resource_record_delay(send_mdns_packet.answer_rrs,
                                                    (payload->flags & 0x0200));
        }

        ZEROCONF_DEBUG_INFO("Generated multi response\n"
                            ">>>>> answer_rr_count:%d\n"
                            ">>>>> additional_rr_count:%d\n"
                            ">>>>> send delay time:%d\n"
                            ">>>>> data length: %ld\n",
                            send_mdns_packet.answer_rrs_count,
                            send_mdns_packet.additional_rrs_count,
                            delay,
                            data_length);

        if (payload->peer_ip.ip_addr.sa_family == AF_INET) {
            memcpy(&ipv4_addr, &payload->peer_ip, sizeof(zeroconf_peer_info_t));
#if defined(LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        } else if (payload->peer_ip.ip_addr.sa_family == AF_INET6) {
            memcpy(&ipv6_addr, &payload->peer_ip, sizeof(zeroconf_peer_info_t));
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF)
        } else {
            ZEROCONF_DEBUG_ERR("invalid ip address\n");
            goto error;
        }

        insertPckSend(data,
                      data_length,
                      1, //max_count
                      delay,
                      delay ? xTaskGetTickCount() : 0,
                      1, //multi_fact
                      protocol,
                      ipv4_addr,
                      ipv6_addr,
                      MDNS_NONE_TYPE);
    }

error:

    clear_mdns_packet(&send_mdns_packet);

    return DA_APP_SUCCESS;
}

int send_mdns_packet(const zeroconf_service_type_t service_key,
                     void *data_string, int len_data)
{
    int ret = DA_APP_SUCCESS;

    struct sockaddr ip_addr;
    struct sockaddr_in *ipv4_addr;

    if (is_supporting_ipv4()) {
        ipv4_addr = (struct sockaddr_in *)&ip_addr;

        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port = htons(ZEROCONF_MDNS_PORT);

        if (service_key == MDNS_SERVICE_TYPE) {
            ipv4_addr->sin_addr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);
#if defined (__SUPPORT_XMDNS__)
        } else if (service_key == XMDNS_SERVICE_TYPE) {
            ipv4_addr->sin_addr.s_addr = inet_addr(ZEROCONF_XMDNS_ADDR);
#endif // (__SUPPORT_XMDNS__)
        } else {
            ZEROCONF_DEBUG_ERR("Invalid service key(%d)\n", service_key);
            return DA_APP_INVALID_PARAMETERS;
        }

        ret = send_mdns_packet_with_dest(service_key, data_string, len_data, ip_addr);
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    struct sockaddr_in6 ipv6_addr;

    if (is_supporting_ipv6()) {
        ipv6_addr.sin6_family = AF_INET6;
        ipv6_addr.sin6_port = htons(ZEROCONF_MDNS_PORT);
        ipv6_addr.sin6_flowinfo = 0;

        if (service_key == MDNS_SERVICE_TYPE) {
            //TODO
            //ipv6_addr = get_mdns_ipv6_multicast();
#if defined (__SUPPORT_XMDNS__)
        } else if (service_key == XMDNS_SERVICE_TYPE) {
            //TODO
            //ipv6_addr = get_xmdns_ipv6_multicast();
#endif // (__SUPPORT_XMDNS__)
        } else {
            ZEROCONF_DEBUG_ERR("Invalid service key(%d)\n", service_key);
            return DA_APP_INVALID_PARAMETERS;
        }

        ret += send_mdns_packet_with_dest(service_key, data_string, len_data,
                                          (struct sockaddr)ipv6_addr);
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    return ret;
}

int send_mdns_packet_with_dest(const zeroconf_service_type_t service_key,
                               void *data_string, int len_data, struct sockaddr ip_addr)
{
    int ret = 0;

    if (data_string == NULL || len_data == 0) {
        ZEROCONF_DEBUG_ERR("Empty data\n");
        return DA_APP_SUCCESS;
    }

#ifdef ZEROCONF_ENABLE_DEBUG_INFO
    if (ip_addr.sa_family == AF_INET) {
        struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&ip_addr;
        ZEROCONF_DEBUG_INFO("Dest(%d.%d.%d.%d:%d), service_key(%d), len(%d)\n",
                            (ntohl(ipv4_addr->sin_addr.s_addr) >> 24) & 0x0ff,
                            (ntohl(ipv4_addr->sin_addr.s_addr) >> 16) & 0x0ff,
                            (ntohl(ipv4_addr->sin_addr.s_addr) >>  8) & 0x0ff,
                            (ntohl(ipv4_addr->sin_addr.s_addr)      ) & 0x0ff,
                            ntohs(ipv4_addr->sin_port),
                            service_key, len_data);
    }
#else
    DA16X_UNUSED_ARG(service_key);
#endif // ZEROCONF_ENABLE_DEBUG_INFO

    ret = sendto(g_zeroconf_status.socket_fd, data_string, len_data, 0,
                 (const struct sockaddr *)&ip_addr, sizeof(ip_addr));
    if (ret <= 0) {
        ZEROCONF_DEBUG_ERR("Failed to send query(%d)\n", ret);
        return DA_APP_NOT_SUCCESSFUL;
    }

    return DA_APP_SUCCESS;
}

void send_ttl_zero(const zeroconf_service_type_t service_key)
{
    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    unsigned char arpa_v4_bytes[ZEROCONF_ARPA_NAME_SIZE] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char arpa_v6_bytes[ZEROCONF_ARPA_NAME_SIZE] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    size_t hostname_bytes_len = 0;
    size_t arpa_v4_bytes_len = 0;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    size_t arpa_v6_bytes_len = 0;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned int answer_rr_count = 0;

    unsigned char send_buffer[512] = {0x00,};
    size_t send_buffer_len = 0;

    ZEROCONF_DEBUG_INFO("Sending ttl zero.\n");

    if (service_key == MDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                             hostname_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);

        if (is_supporting_ipv4()) {
            get_ipv4_bytes(str_ipv4);

            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_mdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            get_ipv6_bytes(str_ipv6);

            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_mdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#if defined (__SUPPORT_XMDNS__)
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(
                                 g_zeroconf_xmdns_host_info.hostname,
                                 hostname_bytes,
                                 ZEROCONF_DOMAIN_NAME_SIZE);

        if (is_supporting_ipv4()) {
            get_ipv4_bytes(str_ipv4);

            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            get_ipv6_bytes(str_ipv6);

            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
        }
#endif    // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#endif    /* __SUPPORT_XMDNS__ */
    } else {
        return;
    }

    if (is_supporting_ipv4()) {
        send_buffer_len += create_general_rec(hostname_bytes,
                                              hostname_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_A,
                                              1, //class
                                              0, //ttl
                                              sizeof(str_ipv4),
                                              str_ipv4,
                                              send_buffer,
                                              send_buffer_len);

        answer_rr_count++;

        send_buffer_len += create_general_rec(arpa_v4_bytes,
                                              arpa_v4_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_PTR,
                                              1, //class
                                              0, //ttl
                                              hostname_bytes_len,
                                              hostname_bytes,
                                              send_buffer,
                                              send_buffer_len);
        answer_rr_count++;
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    if (is_supporting_ipv6()) {
        send_buffer_len += create_general_rec(hostname_bytes,
                                              hostname_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_AAAA,
                                              1, //class
                                              0, //ttl
                                              sizeof(str_ipv6),
                                              str_ipv6,
                                              send_buffer,
                                              send_buffer_len);

        answer_rr_count++;

        send_buffer_len += create_general_rec(arpa_v6_bytes,
                                              arpa_v6_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_PTR,
                                              1, //class
                                              0, //ttl
                                              hostname_bytes_len,
                                              hostname_bytes,
                                              send_buffer,
                                              send_buffer_len);
        answer_rr_count++;
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    create_packet_header(0, answer_rr_count, 0, 0, 0, send_buffer, 0);

    send_mdns_packet(service_key, send_buffer, send_buffer_len);

    return;
}

void doProb(void *params)
{
    int sys_wdog_id = -1;
    int ret = DA_APP_SUCCESS;
    int timeout = ZEROCONF_DEF_PROB_TIMEOUT;
    int conflict_count = 0;
    unsigned int prob_count = 0;

    const size_t send_buffer_size = 256;
    unsigned char *send_buffer = NULL;
    size_t send_buffer_len = 0;

    zeroconf_dpm_prob_info_t *prob_info = (zeroconf_dpm_prob_info_t *)params;
    zeroconf_service_type_t service_key = prob_info->service_key;
    unsigned int skip_prob = prob_info->skip_prob;

    const char *top_domain;
    char *hostname = NULL;

    char *prob_hostname = NULL;
    char *full_prob_hostname = NULL;
    unsigned char *full_prob_hostname_bytes = NULL;
    size_t full_prob_hostname_bytes_len = 0;

    zeroconf_mdns_packet_t prob_packet;

    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    dpm_clr_zeroconf_sleep(MDNS_DPM_TYPE);

    g_zeroconf_status.mdns_probing_proc.status = ZEROCONF_ACTIVATED;

    ZEROCONF_DEBUG_INFO("MDNS Probing(%d).\n", service_key);

    if (service_key == MDNS_SERVICE_TYPE) {
        hostname = g_zeroconf_mdns_host_info.hostname;
        top_domain = mdns_top_domain;
#if defined (__SUPPORT_XMDNS__)
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        hostname = g_zeroconf_xmdns_host_info.hostname;
        top_domain = xmdns_top_domain;
#endif // (__SUPPORT_XMDNS__)
    } else {
        ZEROCONF_DEBUG_ERR("Not supported service(%d)\n", service_key);
        goto end_of_probing;
    }

    get_ipv4_bytes(str_ipv4);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    memset(&prob_packet, 0x00, sizeof(zeroconf_mdns_packet_t));

    send_buffer = zeroconf_calloc(send_buffer_size, 1);
    if (send_buffer == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for send buffer\n");
        goto end_of_probing;
    }

    prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for hostname\n");
        goto end_of_probing;
    }

    full_prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for full hostname\n");
        goto end_of_probing;
    }

    full_prob_hostname_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for full hostname bytes\n");
        goto end_of_probing;
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_stop_responder_task();

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    strcpy(prob_hostname, hostname);

    timeout = zeroconf_get_mdns_prob_timeout();

    if (skip_prob) {
        ZEROCONF_DEBUG_INFO("To skip probing\n");
        goto skip_probing;
    }

    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        prob_count = 0;

        if (strlen(prob_hostname) + strlen(top_domain) + 2 > ZEROCONF_DOMAIN_NAME_SIZE) {
            ZEROCONF_DEBUG_ERR("host name is too long\n");
            goto end_of_probing;
        }

        memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
        strcpy(full_prob_hostname, prob_hostname);
        strcat(full_prob_hostname, top_domain);

        full_prob_hostname_bytes_len = encode_mdns_name_string(full_prob_hostname,
                                                               full_prob_hostname_bytes,
                                                               ZEROCONF_DOMAIN_NAME_SIZE);
        if (full_prob_hostname_bytes_len == 0) {
            ZEROCONF_DEBUG_ERR("Failed to encode mdns host name\n");
            goto end_of_probing;
        }

        while (prob_count < 3) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            clear_mdns_packet(&prob_packet);

            memset(send_buffer, 0x00, send_buffer_size);

            ZEROCONF_DEBUG_INFO("Probing with %s(#%02d)\n",
                                full_prob_hostname, prob_count + 1);

            ret = insert_question(&prob_packet.questions,
                                  full_prob_hostname_bytes,
                                  full_prob_hostname_bytes_len,
                                  ZEROCONF_MDNS_RR_TYPE_ALL,
                                  (((prob_count == 0 ? 1 : 0) << 15) | 1));
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("Failed to create question(0x%x)\n", -ret);
                goto end_of_probing;
            }

            prob_packet.questions_count++;

            if (is_supporting_ipv4()) {
                ret = insert_resource_record(&prob_packet.authority_rrs,
                                             full_prob_hostname_bytes,
                                             full_prob_hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_A,
                                             1,
                                             120,
                                             sizeof(str_ipv4),
                                             str_ipv4);
                if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                    ZEROCONF_DEBUG_ERR("Failed to create authority rr(0x%x)\n", -ret);
                    goto end_of_probing;
                }

                prob_packet.authority_rrs_count++;
            }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                ret = insert_resource_record(&prob_packet.authority_rrs,
                                             full_prob_hostname_bytes,
                                             full_prob_hostname_bytes_len,
                                             ZEROCONF_MDNS_RR_TYPE_AAAA,
                                             1,
                                             120,
                                             sizeof(str_ipv6),
                                             str_ipv6);
                if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                    ZEROCONF_DEBUG_ERR("Failed to create authority rr(0x%x)\n", -ret);
                    goto end_of_probing;
                }

                prob_packet.authority_rrs_count++;
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

            send_buffer_len = convert_mdns_packet_to_bytes(&prob_packet,
                                                           send_buffer,
                                                           send_buffer_size);
            if (send_buffer_len == 0) {
                ZEROCONF_DEBUG_ERR("Failed to generate prob packet\n");
                ret = DA_APP_NOT_SUCCESSFUL;
                goto end_of_probing;
            }

            da16x_sys_watchdog_suspend(sys_wdog_id);

            ret = send_mdns_packet(service_key, send_buffer, send_buffer_len);
            if (ret == DA_APP_SUCCESS) {
                ret = receive_prob_response(timeout, &prob_packet);
                if (ret != DA_APP_SUCCESS) {
                    ZEROCONF_DEBUG_ERR("Probing ERROR: Conflict detected! Assigning "
                                       "New Host Name..(%d)\n", conflict_count + 1);

                    conflict_count++;

                    if (conflict_count >= ZEROCONF_DEF_CONFLICT_COUNT) {
                        timeout = zeroconf_get_mdns_prob_conflict_timeout();
                    }

                    ZEROCONF_CONFLICT_NAME_RULE(prob_hostname, hostname, conflict_count);

                    ZEROCONF_DEBUG_INFO(YELLOW_COLOR "sleep : %d msec\n" CLEAR_COLOR,
                                        timeout);

                    vTaskDelay(timeout);

                    break;
                }

                prob_count++;

                ZEROCONF_DEBUG_TEMP("Increased probing count(%d)\n", prob_count);
            } else {
                ZEROCONF_DEBUG_ERR("failed to send prob(0x%x)\n", -ret);
                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                goto end_of_probing;
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }

        //if prob is completed
        if (prob_count >= ZEROCONF_DEF_PROB_COUNT) {
            break;
        }
    } // while

    ZEROCONF_DEBUG_INFO("Probing successful! Host name assigned : %s\n", prob_hostname);

skip_probing:

    ret = set_host_info(service_key, prob_hostname, top_domain);
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("failed to host information(0x%x)\n", -ret);
        goto end_of_probing;
    }

    ZEROCONF_DEBUG_INFO("Scheduling announcements.\n");

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_start_responder_task();

    zeroconf_start_sender_task();

    ret = doAnnounce(service_key, skip_prob);
    if (ret != DA_APP_SUCCESS) {
        ZEROCONF_DEBUG_ERR("failed to send announcement(0x%x)\n", -ret);
        zeroconf_stop_sender_task();

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        goto end_of_probing;
    }

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    if (service_key == MDNS_SERVICE_TYPE) {
        ZEROCONF_DEBUG_ERR("mDNS Service started >>\n"
                           "\tHost name\t: %s\n",
                           g_zeroconf_mdns_host_info.hostname);
        set_mdns_service_status(ZEROCONF_ACTIVATED);
#if defined (__SUPPORT_XMDNS__)
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        ZEROCONF_DEBUG_ERR("xmDNS Service started >>\n"
                           "\tHost name\t: %s\n",
                           g_zeroconf_xmdns_host_info.hostname);
        set_xmdns_service_status(ZEROCONF_ACTIVATED);
#endif // (__SUPPORT_XMDNS__)
    }

end_of_probing:

    if (send_buffer) {
        zeroconf_free(send_buffer);
    }

    if (prob_hostname) {
        zeroconf_free(prob_hostname);
    }

    if (full_prob_hostname) {
        zeroconf_free(full_prob_hostname);
    }

    if (full_prob_hostname_bytes) {
        zeroconf_free(full_prob_hostname_bytes);
    }

    clear_mdns_packet(&prob_packet);

    dpm_set_zeroconf_sleep(MDNS_DPM_TYPE);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.mdns_probing_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.mdns_probing_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

int doAnnounce(const zeroconf_service_type_t service_key, unsigned int skip_announcement)
{
    int ret = 0;
    zeroconf_peer_info_t ipv4_info;
    zeroconf_peer_info_t ipv6_info;

    struct sockaddr_in ipv4_addr;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    struct sockaddr_in6 ipv6_addr;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned char send_buffer[256] = {0x00,};
    size_t send_buffer_len = 0;

    unsigned char *hostname_bytes = NULL;
    unsigned char *arpa_v4_bytes = NULL;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char *arpa_v6_bytes = NULL;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    size_t hostname_bytes_len = 0;
    size_t arpa_v4_bytes_len = 0;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    size_t arpa_v6_bytes_len = 0;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned int answer_rr_count = 0;
    unsigned short class = 0;
    unsigned short cache_flush_bit = 0;

    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    memset(&ipv4_info, 0x00, sizeof(zeroconf_peer_info_t));
    memset(&ipv6_info, 0x00, sizeof(zeroconf_peer_info_t));

    hostname_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (hostname_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for hostname bytes\n");
        ret = DA_APP_MALLOC_ERROR;
        goto error;
    }

    arpa_v4_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (arpa_v4_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for arpa_v4\n");
        ret = DA_APP_MALLOC_ERROR;
        goto error;
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    if (is_supporting_ipv6()) {
        arpa_v6_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
        if (arpa_v6_bytes == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for arpa_v6\n");
            ret = DA_APP_MALLOC_ERROR;
            goto error;
        }
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    if (service_key == MDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                             hostname_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);
        if (hostname_bytes_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to encode hostname(%s)\n",
                               g_zeroconf_mdns_host_info.hostname);

            ret = DA_APP_NOT_SUCCESSFUL;
            goto error;
        }

        if (is_supporting_ipv4()) {
            get_ipv4_bytes(str_ipv4);

            ipv4_addr.sin_family = AF_INET;
            ipv4_addr.sin_port = htons(ZEROCONF_MDNS_PORT);
            ipv4_addr.sin_addr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);

            ipv4_info.type = MULTICAST_RESPONSE_TYPE;
            memcpy(&ipv4_info.ip_addr, &ipv4_addr, sizeof(struct sockaddr));

            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_mdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
            if (arpa_v4_bytes_len == 0) {
                ZEROCONF_DEBUG_ERR("failed to encode arpa.v4(%s)\n",
                                   g_zeroconf_mdns_host_info.arpa_v4_hostname);

                ret = DA_APP_NOT_SUCCESSFUL;
                goto error;
            }
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            get_ipv6_bytes(str_ipv6);

            ipv6_addr.sin6_family = AF_INET6;
            ipv6_addr.sin6_port = htons(ZEROCONF_MDNS_PORT);
            ipv6_addr.sin6_flowinfo = 0;
            //ipv6_addr.ip_addr = get_mdns_ipv6_multicast();

            ipv6_info.type = MULTICAST_RESPONSE_TYPE;
            memcpy(&ipv6_info.ip_addr, &ipv6_addr, sizeof(struct sockaddr));

            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeronconf_mdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);

            if (arpa_v6_bytes_len == 0) {
                ZEROCONF_DEBUG_ERR("Failed to encode arpa.v6(%s)\n",
                                   g_zeronconf_mdns_host_info.arpa_v6_hostname);

                ret = DA_APP_NOT_SUCCESSFUL;
                goto error;
            }
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#if defined (__SUPPORT_XMDNS__)
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_xmdns_host_info.hostname,
                                                     hostname_bytes,
                                                     ZEROCONF_DOMAIN_NAME_SIZE);
        if (hostname_bytes_len == 0) {
            ZEROCONF_DEBUG_ERR("Failed to encode hostname(%s)\n",
                               g_zeroconf_xmdns_host_info.hostname);
            ret = DA_APP_NOT_SUCCESSFUL;
            goto error;
        }

        if (is_supporting_ipv4()) {
            get_ipv4_bytes(str_ipv4);

            ipv4_addr.sin_family = AF_INET;
            ipv4_addr.sin_port = htons(ZEROCONF_MDNS_PORT);
            ipv4_addr.sin_addr.s_addr = inet_addr(ZEROCONF_XMDNS_ADDR);

            ipv4_info.type = MULTICAST_RESPONSE_TYPE;
            memcpy(&ipv4_info.ip_addr, &ipv4_addr, sizeof(struct sockaddr));

            arpa_v4_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v4_hostname,
                                    arpa_v4_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
            if (arpa_v4_bytes_len == 0) {
                ZEROCONF_DEBUG_ERR("Failed to encode arpa.v4(%s)\n",
                                   g_zeroconf_xmdns_host_info.arpa_v4_hostname);
                ret = DA_APP_NOT_SUCCESSFUL;
                goto error;
            }
        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
        if (is_supporting_ipv6()) {
            get_ipv6_bytes(str_ipv6);

            ipv6_addr.sin6_family = AF_INET6;
            ipv6_addr.sin6_port = htons(ZEROCONF_MDNS_PORT);
            ipv6_addr.sin6_flowinfo = 0;
            //ipv6_addr.ip_addr = get_mdns_ipv6_multicast();

            ipv6_info.type = MULTICAST_RESPONSE_TYPE;
            memcpy(&ipv6_info.ip_addr, &ipv6_addr, sizeof(struct sockaddr));

            arpa_v6_bytes_len = encode_mdns_name_string(
                                    g_zeroconf_xmdns_host_info.arpa_v6_hostname,
                                    arpa_v6_bytes,
                                    ZEROCONF_ARPA_NAME_SIZE);
            if (arpa_v6_bytes_len == 0) {
                ZEROCONF_DEBUG_ERR("Failed to encode arpa.v6(%s)\n",
                                   g_zeroconf_xmdns_host_info.arpa_v6_hostname);
                ret = DA_APP_NOT_SUCCESSFUL;
                goto error;
            }
        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
#endif // (__SUPPORT_XMDNS__)
    } else {
        ZEROCONF_DEBUG_ERR("Not supported service(%d)\n", service_key);
        ret = DA_APP_INVALID_PARAMETERS;
        goto error;
    }

    //generate announcement
    send_buffer_len = ZEROCONF_MDNS_HEADER_SIZE;

    if (is_supporting_ipv4()) {
        class = 1;
        cache_flush_bit = 1;

        send_buffer_len += create_general_rec(hostname_bytes,
                                              hostname_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_A,
                                              ((cache_flush_bit << 15) | class),
                                              120,
                                              sizeof(str_ipv4),
                                              str_ipv4,
                                              send_buffer,
                                              send_buffer_len);
        answer_rr_count++;

        class = 1;
        cache_flush_bit = 0;

        send_buffer_len += create_general_rec(arpa_v4_bytes,
                                              arpa_v4_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_PTR,
                                              ((cache_flush_bit << 15) | class),
                                              120,
                                              hostname_bytes_len,
                                              hostname_bytes,
                                              send_buffer,
                                              send_buffer_len);

        answer_rr_count++;
    }

#if defined (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF)
    if (is_supporting_ipv6()) {
        class = 1;
        cache_flush_bit = 1;

        send_buffer_len += create_general_rec(hostname_bytes,
                                              hostname_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_AAAA,
                                              ((cache_flush_bit << 15) | class),
                                              120,
                                              sizeof(str_ipv6),
                                              str_ipv6,
                                              send_buffer,
                                              send_buffer_len);
        answer_rr_count++;

        class = 1;
        cache_flush_bit = 0;

        send_buffer_len += create_general_rec(arpa_v6_bytes,
                                              arpa_v6_bytes_len,
                                              ZEROCONF_MDNS_RR_TYPE_PTR,
                                              ((cache_flush_bit << 15) | class),
                                              120,
                                              hostname_bytes_len,
                                              hostname_bytes,
                                              send_buffer,
                                              send_buffer_len);

        answer_rr_count++;
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    create_packet_header(0, answer_rr_count, 0, 0, 0, send_buffer, 0);

    if (!skip_announcement) {
        send_mdns_packet(service_key, send_buffer, send_buffer_len);

        insertPckSend(send_buffer,
                      send_buffer_len,
                      7,        //max count
                      100,        //first interval(1 sec)
                      xTaskGetTickCount(),
                      2,        //mult fact
                      service_key,    //type
                      ipv4_info,
                      ipv6_info,
                      MDNS_ANNOUNCE_TYPE);
    }

    insertOwnHostInfo(service_key);

    ret = DA_APP_SUCCESS;

error:

    if (hostname_bytes) {
        zeroconf_free(hostname_bytes);
    }

    if (arpa_v4_bytes) {
        zeroconf_free(arpa_v4_bytes);
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    if (arpa_v6_bytes) {
        zeroconf_free(arpa_v6_bytes);
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    return ret;
}

int receive_prob_response(int timeout, zeroconf_mdns_packet_t *prob_packet)
{
    int ret = -1;
    int ret_val = DA_APP_SUCCESS;
    unsigned char *payload = NULL;
    //size_t payload_len = 0;
    struct timeval sockopt_timeout = {0, timeout * 1000};

    zeroconf_mdns_packet_t parsed_payload;

    zeroconf_question_t *question = NULL;
    zeroconf_resource_record_t *authority_rr = NULL;
    zeroconf_resource_record_t *recv_rrecord = NULL;

    struct sockaddr_in peer_addr, dst_addr;
    int peer_addrlen = 0, dst_addrlen = 0;

    ZEROCONF_DEBUG_TEMP("Receving wait time(%d ms)\n", timeout);

    memset(&peer_addr, 0x00, sizeof(struct sockaddr_in));
    memset(&dst_addr, 0x00, sizeof(struct sockaddr_in));
    memset(&parsed_payload, 0x00, sizeof(zeroconf_mdns_packet_t));

    peer_addrlen = sizeof(struct sockaddr_in);
    dst_addrlen = sizeof(struct sockaddr_in);

    //TODO: check payload size
    payload = zeroconf_calloc(ZEROCONF_DEF_BUF_SIZE, sizeof(unsigned char));
    if (!payload) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory to receive packet(%ld)\n",
                           ZEROCONF_DEF_BUF_SIZE);
        return DA_APP_MALLOC_ERROR;
    }

    //set timeout
    ret = setsockopt(g_zeroconf_status.socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                     &sockopt_timeout, sizeof(sockopt_timeout));
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to set timeout of socket option(%d)\n", ret);
    }

    ret = recvfromto(g_zeroconf_status.socket_fd, payload, ZEROCONF_DEF_BUF_SIZE, 0,
                     (struct sockaddr *)&peer_addr, (socklen_t *)&peer_addrlen,
                     (struct sockaddr *)&dst_addr, (socklen_t *)&dst_addrlen);
    if (ret > 0) {
        //payload_len = ret;

        ret = parse_mdns_packet(payload, &parsed_payload);
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to parse received packet(0x%x)\n", -ret);
            ret_val = ret;
            goto cleanup;
        }

        if (parsed_payload.transaction_id == 0) {
            if (parsed_payload.flags & 0x8400) {
                for (question = prob_packet->questions
                        ; question != NULL
                        ; question = question->next) {
                    for (recv_rrecord = parsed_payload.answer_rrs
                            ; recv_rrecord != NULL
                            ; recv_rrecord = recv_rrecord->next) {
                        if (compare_domain_bytes(question->name_bytes,
                                                 question->name_bytes_len,
                                                 recv_rrecord->name_bytes,
                                                 recv_rrecord->name_bytes_len)) {
                            ZEROCONF_DEBUG_INFO(RED_COLOR
                                                "Duplicated host name in answer rr\n"
                                                CLEAR_COLOR);

                            ret_val = DA_APP_NOT_SUCCESSFUL;
                            goto cleanup;
                        }
                    }
                }
            } else if (parsed_payload.flags == 0) {
                for (authority_rr = prob_packet->authority_rrs
                        ; authority_rr != NULL
                        ; authority_rr  = authority_rr->next) {
                    for (recv_rrecord = parsed_payload.authority_rrs
                            ; recv_rrecord != NULL
                            ; recv_rrecord = recv_rrecord->next) {
                        if (compare_domain_bytes(authority_rr->name_bytes,
                                                 authority_rr->name_bytes_len,
                                                 recv_rrecord->name_bytes,
                                                 recv_rrecord->name_bytes_len)) {
                            if (authority_rr->type == recv_rrecord->type) {
                                for (int index = 0 ; index < authority_rr->data_length ; index++) {
                                    if (authority_rr->data[index] < recv_rrecord->data[index]) {
                                        ZEROCONF_DEBUG_INFO(RED_COLOR
                                                            "Tie-breaking lost..\n"
                                                            CLEAR_COLOR);

                                        ret_val = DA_APP_NOT_SUCCESSFUL;
                                        break;
                                    } else if (authority_rr->data[index] > recv_rrecord->data[index]) {
                                        ZEROCONF_DEBUG_INFO(GREEN_COLOR
                                                            "Tie-breaking won by comparing IP addresses..\n"
                                                            CLEAR_COLOR);

                                        vTaskDelay(zeroconf_get_mdns_prob_tie_breaker_timeout());

                                        ret_val = DA_APP_SUCCESS;
                                        break;
                                    }
                                }
                            } else {
                                ZEROCONF_DEBUG_INFO(RED_COLOR
                                                    "Type is not matched(recv:%d, prob:%d)\n"
                                                    CLEAR_COLOR,
                                                    authority_rr->type,
                                                    recv_rrecord->type);

                                ret_val = DA_APP_NOT_SUCCESSFUL;
                                continue;
                            }

                            goto cleanup;
                        }
                    }
                }
            }
        }

        clear_mdns_packet(&parsed_payload);
    } else if (ret == 0) {
        ZEROCONF_DEBUG_INFO("No received packet\n");
        ret = DA_APP_SUCCESS;
    } else {
        ZEROCONF_DEBUG_ERR("Failed to receive packet(%d)\n", ret);
        ret = DA_APP_SUCCESS;
    }

cleanup:

    clear_mdns_packet(&parsed_payload);

    if (payload) {
        zeroconf_free(payload);
        payload = NULL;
    }

    return ret_val;
}

int process_mdns_responder(zeroconf_service_type_t service_key, char *hostname, unsigned int skip_prob)
{
    ZEROCONF_DEBUG_INFO("Starting responder.\n");

    to_lower(hostname);

    if (service_key == MDNS_SERVICE_TYPE) {
        if (strstr(hostname, mdns_top_domain) != NULL) {
            ZEROCONF_DEBUG_ERR("hostname should be not contain .local(%s).\n", hostname);
            return DA_APP_INVALID_PARAMETERS;
        }

        if (zeroconf_is_running_mdns_service()) {
            ZEROCONF_DEBUG_ERR("mdns service is already running.\n");
            return DA_APP_ALREADY_ENABLED;
        }

        init_mdns_host_info();

        strcpy(g_zeroconf_mdns_host_info.hostname, hostname);

        g_zeroconf_prob_info.service_key = service_key;
        g_zeroconf_prob_info.skip_prob = skip_prob;

        zeroconf_start_mdns_probing_task(&g_zeroconf_prob_info);
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        if (strstr(hostname, xmdns_top_domain) != NULL) {
            ZEROCONF_DEBUG_ERR("hostname should be not contain .site(%s).\n", hostname);
            return DA_APP_INVALID_PARAMETERS;
        }

        if (zeroconf_is_running_xmdns_service()) {
            ZEROCONF_DEBUG_ERR("xmdns service is already running.\n");
            return DA_APP_ALREADY_ENABLED;
        }

        init_xmdns_host_info();

        strcpy(g_zeroconf_xmdns_host_info.hostname, hostname);

        g_zeroconf_prob_info.service_key = service_key;
        g_zeroconf_prob_info.skip_prob = skip_prob;

        zeroconf_start_xmdns_probing_task(&g_zeroconf_prob_info);
    } else {
        ZEROCONF_DEBUG_ERR("Not supported service(%d)\n", service_key);
        return DA_APP_INVALID_PARAMETERS;
    }

    return DA_APP_SUCCESS;
}

void restarter_process(void *params)
{
    int sys_wdog_id = -1;
    int ret = DA_APP_SUCCESS;
    int iface = g_zeroconf_status.iface;

#if defined (__SUPPORT_DNS_SD__) || defined (__SUPPORT_XMDNS__)
    const unsigned int wait_option = 100;
    const unsigned int max_retry_cnt = 200;
    unsigned int retry_cnt = 0;
#endif // (__SUPPORT_DNS_SD__) || (__SUPPORT_XMDNS__)

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;
    char *domain_ptr = NULL;
#if defined (__SUPPORT_DNS_SD__)
    zeroconf_local_dns_sd_info_t *dns_sd_service = NULL;
#endif // (__SUPPORT_DNS_SD__)

    unsigned int restart_service = (unsigned int)params;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    g_zeroconf_status.restarter_proc.status = ZEROCONF_ACTIVATED;

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    if (is_running_mdns_probing_processor()) {
        zeroconf_stop_mdns_probing_task();
#if defined (__SUPPORT_DNS_SD__)
    } else if (is_running_dns_sd_probing_processor()) {
        zeroconf_stop_dns_sd_probing_task();
#endif // (__SUPPORT_DNS_SD__)
    }

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    ZEROCONF_DEBUG_INFO("Restarting zeroconf services(0x%x).\n", restart_service);

    if ((restart_service & ZEROCONF_RESTART_MDNS_SERVICE)
        || (restart_service & ZEROCONF_RESTART_XMDNS_SERVICE)) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        zeroconf_stop_sender_task();

        zeroconf_stop_responder_task();

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        clear_send_cache(MDNS_SERVICE_TYPE);
        clear_send_cache(XMDNS_SERVICE_TYPE);

        ret = set_zero_ip_info(iface);
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to set up changed ip address(0x%x)\n", -ret);
            goto end;
        }

        if ((restart_service & ZEROCONF_RESTART_MDNS_SERVICE)
            && zeroconf_is_running_mdns_service()) {

            hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                                                         hostname_bytes,
                                                         ZEROCONF_DOMAIN_NAME_SIZE);

            deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0,
                           ZEROCONF_MDNS_RR_TYPE_ALL);

            set_mdns_service_status(ZEROCONF_INIT);

            ZEROCONF_DEBUG_INFO("Restarting mdns services.\n");

            send_ttl_zero(MDNS_SERVICE_TYPE);

            domain_ptr = strstr(g_zeroconf_mdns_host_info.hostname, mdns_top_domain);
            if (domain_ptr) {
                memset(domain_ptr, 0x00,
                       (g_zeroconf_mdns_host_info.hostname + ZEROCONF_DOMAIN_NAME_SIZE) - domain_ptr);
            }

            g_zeroconf_prob_info.service_key = MDNS_SERVICE_TYPE;
            g_zeroconf_prob_info.skip_prob = pdFALSE;

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            zeroconf_start_mdns_probing_task(&g_zeroconf_prob_info);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }

#if defined (__SUPPORT_XMDNS__)
        if ((restart_service & ZEROCONF_RESTART_XMDNS_SERVICE)
            && zeroconf_is_running_xmdns_service()) {

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            for (retry_cnt = 0 ; retry_cnt < max_retry_cnt ; retry_cnt++) {
                if (zeroconf_is_running_mdns_service()) {
                    break;
                }

                vTaskDelay(wait_option);
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            if (retry_cnt > max_retry_cnt) {
                ZEROCONF_DEBUG_ERR("failed to start mdns service\n");
                goto end;
            }

            hostname_bytes_len = encode_mdns_name_string(
                                     g_zeroconf_xmdns_host_info.hostname,
                                     hostname_bytes,
                                     ZEROCONF_DOMAIN_NAME_SIZE);

            deleteMdnsResp(hostname_bytes, hostname_bytes_len, 0,
                           ZEROCONF_MDNS_RR_TYPE_ALL);

            set_xmdns_service_status(ZEROCONF_INIT);

            ZEROCONF_DEBUG_INFO("Restarting xmdns services.\n");

            send_ttl_zero(XMDNS_SERVICE_TYPE);

            domain_ptr = strstr(g_zeroconf_xmdns_host_info.hostname, xmdns_top_domain);
            if (domain_ptr) {
                memset(domain_ptr, 0x00,
                       (g_zeroconf_mdns_host_info.hostname + ZEROCONF_DOMAIN_NAME_SIZE) - domain_ptr);
            }

            g_zeroconf_prob_info.service_key = XMDNS_SERVICE_TYPE;
            g_zeroconf_prob_info.skip_prob = pdFALSE;

            da16x_sys_watchdog_notify(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            zeroconf_start_xmdns_probing_task(&g_zeroconf_prob_info);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
#endif // (__SUPPORT_XMDNS__)
    }

#if defined (__SUPPORT_DNS_SD__)
    if ((restart_service & ZEROCONF_RESTART_MDNS_SERVICE)
        || (restart_service & ZEROCONF_RESTART_XMDNS_SERVICE)
        || (restart_service & ZEROCONF_RESTART_DNS_SD_SERVICE)) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        for (retry_cnt = 0 ; retry_cnt < max_retry_cnt ; retry_cnt++) {
            if (zeroconf_is_running_mdns_service()) {
                break;
            }

            vTaskDelay(wait_option);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (retry_cnt >= max_retry_cnt) {
            ZEROCONF_DEBUG_ERR("failed to start mdns service\n");
            goto end;
        }

        if (g_zeroconf_local_dns_sd_service) {
            ZEROCONF_DEBUG_TEMP("Before dns-sd information\n");
            printLocalDnsSd();

            dns_sd_service = createLocalDnsSd(g_zeroconf_local_dns_sd_service->name,
                                              g_zeroconf_local_dns_sd_service->type,
                                              g_zeroconf_local_dns_sd_service->domain,
                                              g_zeroconf_local_dns_sd_service->port,
                                              g_zeroconf_local_dns_sd_service->txtLen,
                                              g_zeroconf_local_dns_sd_service->txtRecord);
            if (dns_sd_service == NULL) {
                ZEROCONF_DEBUG_ERR("failed to backup dns-sd service's info\n");
                goto end;
            }
        } else {
            ZEROCONF_DEBUG_ERR("no dns-sd service's info\n");
            goto end;
        }

        ret = zeroconf_unregister_dns_sd_service();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("failed to unregister dns-sd service(0x%x)\n", -ret);
            goto end;
        }

        g_zeroconf_local_dns_sd_service = createLocalDnsSd(dns_sd_service->name,
                                          dns_sd_service->type,
                                          dns_sd_service->domain,
                                          dns_sd_service->port,
                                          dns_sd_service->txtLen,
                                          dns_sd_service->txtRecord);
        if (g_zeroconf_local_dns_sd_service == NULL) {
            ZEROCONF_DEBUG_ERR("failed to register dns-sd service(0x%x)\n", -ret);
            goto end;
        }

        g_zeroconf_prob_info.service_key = UNKNOWN_SERVICE_TYPE;
        g_zeroconf_prob_info.skip_prob = pdFALSE;

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        zeroconf_start_dns_sd_local_probing_task(&g_zeroconf_prob_info);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }
#endif // (__SUPPORT_DNS_SD__)

end:

#if defined (__SUPPORT_DNS_SD__)
    if (dns_sd_service) {
        deleteLocalDnsSd(dns_sd_service);
    }
#endif // (__SUPPORT_DNS_SD__)

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.restarter_proc.status = ZEROCONF_DEACTIVATED;

    ZEROCONF_DEBUG_INFO("End of task\n");

    g_zeroconf_status.restarter_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void zeroconf_restart_all_service()
{
    zeroconf_start_restarter_task(ZEROCONF_RESTART_ALL_SERVICE);
}

void zeroconf_stop_all_service()
{
    int ret = DA_APP_SUCCESS;

#if defined (__SUPPORT_DNS_SD__)
    if (zeroconf_is_running_dns_sd_service()) {
        ret = zeroconf_unregister_dns_sd_service();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to unregister dns-sd service(0x%x)\n", -ret);
        }
    }
#endif // (__SUPPORT_DNS_SD__)

#if defined (__SUPPORT_XMDNS__)
    if (zeroconf_is_running_xmdns_service()) {
        ret = zeroconf_stop_xmdns_service();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to stop xmdns service(0x%x)\n", -ret);
        }
    }
#endif // (__SUPPORT_XMDNS__)

    if (zeroconf_is_running_mdns_service()) {
        ret = zeroconf_stop_mdns_service();
        if (ret != DA_APP_SUCCESS) {
            ZEROCONF_DEBUG_ERR("Failed to stop mdns service(0x%x)\n", -ret);
        }
    }

    return;
}

/* responder process thread */
void responder_process(void *params)
{
    int sys_wdog_id = -1;
    int ret = 0;

    unsigned char *payload = NULL;
#if defined (ZEROCONF_ENABLE_DEBUG_INFO)
    int payload_len = 0;
#endif // (ZEROCONF_ENABLE_DEBUG_INFO)
    zeroconf_mdns_packet_t parsed_payload;
    zeroconf_resource_record_t *rrecord = NULL;

    zeroconf_service_type_t protocol = UNKNOWN_SERVICE_TYPE;
    unsigned int is_mdns_reset = pdFALSE;
#if defined (__SUPPORT_DNS_SD__)
    unsigned int is_dns_sd_reset = pdFALSE;
#endif // (__SUPPORT_DNS_SD__)

    struct sockaddr recv_addr;
    struct sockaddr_in dst_addr;
    int recv_addrlen, dst_addrlen;

    struct sockaddr_in *recv_ipv4_addr = NULL;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    struct sockaddr_in6 *recv_ipv6_addr = NULL;
#endif // (LWIP_IPV6)  && (SUPPORT_IPV6_ZEROCONF) 

    struct sockaddr_in *ipv4_addr = NULL;
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    struct sockaddr_in6 *ipv6_addr = NULL;
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    struct timeval sockopt_timeout = {0, 100 * 1000};

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    g_zeroconf_status.responder_proc.status = ZEROCONF_ACTIVATED;

    memset(&parsed_payload, 0x00, sizeof(zeroconf_mdns_packet_t));

    ZEROCONF_DEBUG_INFO("Responder started.\n");

    payload = zeroconf_calloc(ZEROCONF_DEF_BUF_SIZE, sizeof(unsigned char));
    if (!payload) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory to receive packet(%ld)\n",
                           ZEROCONF_DEF_BUF_SIZE);
        goto end_of_task;
    }

    while (g_zeroconf_status.responder_proc.status == ZEROCONF_ACTIVATED) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        dpm_set_zeroconf_sleep(RESPONDER_DPM_TYPE);

        memset(payload, 0x00, ZEROCONF_DEF_BUF_SIZE);

        clear_mdns_packet(&parsed_payload);

        memset(&recv_addr, 0x00, sizeof(struct sockaddr));
        memset(&dst_addr, 0x00, sizeof(struct sockaddr_in));

        recv_addrlen = sizeof(struct sockaddr);
        dst_addrlen = sizeof(struct sockaddr_in);

        //set timeout
        ret = setsockopt(g_zeroconf_status.socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                         &sockopt_timeout, sizeof(sockopt_timeout));
        if (ret) {
            ZEROCONF_DEBUG_ERR("Failed to set timeout of socket option(%d)\n", ret);
        }

        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        // receive the data, socket timeout 100 msec
        ret = recvfromto(g_zeroconf_status.socket_fd, payload, ZEROCONF_DEF_BUF_SIZE, 0,
                         (struct sockaddr *)&recv_addr, (socklen_t *)&recv_addrlen,
                         (struct sockaddr *)&dst_addr, (socklen_t *)&dst_addrlen);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (ret > 0) {
#if defined (ZEROCONF_ENABLE_DEBUG_INFO)
            payload_len = ret;
#endif // (ZEROCONF_ENABLE_DEBUG_INFO)

            dpm_clr_zeroconf_sleep(RESPONDER_DPM_TYPE);

            //check dest ip adderss
            if (is_supporting_ipv4() && recv_addr.sa_family == AF_INET) {
                recv_ipv4_addr = (struct sockaddr_in *)&recv_addr;

                ZEROCONF_DEBUG_INFO("src address(%d.%d.%d.%d:%d)\n"
                                    "dst address(%d.%d.%d.%d:%d)\n"
                                    "mdns(%d.%d.%d.%d:%d)\n"
                                    "xmdns(%d.%d.%d.%d:%d)\n",
                                    (ntohl(recv_ipv4_addr->sin_addr.s_addr) >> 24) & 0x0ff,
                                    (ntohl(recv_ipv4_addr->sin_addr.s_addr) >> 16) & 0x0ff,
                                    (ntohl(recv_ipv4_addr->sin_addr.s_addr) >>  8) & 0x0ff,
                                    (ntohl(recv_ipv4_addr->sin_addr.s_addr)      ) & 0x0ff,
                                    ntohs(recv_ipv4_addr->sin_port),
                                    (ntohl(dst_addr.sin_addr.s_addr) >> 24) & 0x0ff,
                                    (ntohl(dst_addr.sin_addr.s_addr) >> 16) & 0x0ff,
                                    (ntohl(dst_addr.sin_addr.s_addr) >>  8) & 0x0ff,
                                    (ntohl(dst_addr.sin_addr.s_addr)      ) & 0x0ff,
                                    ntohs(dst_addr.sin_port),
                                    (ntohl(inet_addr(ZEROCONF_MDNS_ADDR)) >> 24) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_MDNS_ADDR)) >> 16) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_MDNS_ADDR)) >>  8) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_MDNS_ADDR))      ) & 0x0ff,
                                    ZEROCONF_MDNS_PORT,
                                    (ntohl(inet_addr(ZEROCONF_XMDNS_ADDR)) >> 24) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_XMDNS_ADDR)) >> 16) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_XMDNS_ADDR)) >>  8) & 0x0ff,
                                    (ntohl(inet_addr(ZEROCONF_XMDNS_ADDR))      ) & 0x0ff,
                                    ZEROCONF_MDNS_PORT);

                if ((inet_addr(ZEROCONF_MDNS_ADDR) == dst_addr.sin_addr.s_addr)
                    && zeroconf_is_running_mdns_service()) {
                    protocol = MDNS_SERVICE_TYPE;
#if defined (__SUPPORT_XMDNS__)
                } else if ((inet_addr(ZEROCONF_XMDNS_ADDR) == dst_addr.sin_addr.s_addr)
                         && zeroconf_is_running_xmdns_service()) {
                    protocol = XMDNS_SERVICE_TYPE;
#endif // (__SUPPORT_XMDNS__)
                } else {
                    ZEROCONF_DEBUG_INFO(RED_COLOR
                                        "dropped received packet(%d.%d.%d.%d:%d)\n"
                                        CLEAR_COLOR,
                                        (ntohl(dst_addr.sin_addr.s_addr) >> 24) & 0x0ff,
                                        (ntohl(dst_addr.sin_addr.s_addr) >> 16) & 0x0ff,
                                        (ntohl(dst_addr.sin_addr.s_addr) >>  8) & 0x0ff,
                                        (ntohl(dst_addr.sin_addr.s_addr)      ) & 0x0ff,
                                        ntohs(recv_ipv4_addr->sin_port));
                    continue;
                }

                //set peer ip info
                if (ntohs(recv_ipv4_addr->sin_port) != ZEROCONF_MDNS_PORT) {
                    parsed_payload.peer_ip.type = UNICAST_RESPONSE_TYPE;
                    memcpy(&parsed_payload.peer_ip.ip_addr, &recv_addr, sizeof(recv_addr));
                } else {
                    parsed_payload.peer_ip.type = MULTICAST_RESPONSE_TYPE;

                    ipv4_addr = (struct sockaddr_in *)&parsed_payload.peer_ip.ip_addr;

                    ipv4_addr->sin_family = AF_INET;
                    ipv4_addr->sin_port = htons(ZEROCONF_MDNS_PORT);

                    if (protocol == MDNS_SERVICE_TYPE) {
                        ipv4_addr->sin_addr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);
#if defined (__SUPPORT_XMDNS__)
                    } else if (protocol == XMDNS_SERVICE_TYPE) {
                        ipv4_addr->sin_addr.s_addr = inet_addr(ZEROCONF_XMDNS_ADDR);
#endif // (__SUPPORT_XMDNS__)
                    }
                }
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            } else if (is_supporting_ipv6() recv_addr.sa_family == AF_INET6) {
                recv_ipv6_addr = (struct sockaddr_in6 *)&recv_addr;

                if (!memcmp(header->nx_ip_header_destination_ip,
                            mdns_ipv6_addr.nxd_ip_address.v6,
                            sizeof(header->nx_ip_header_destination_ip))
                    && zeroconf_is_running_mdns_service()) {

                    protocol = MDNS_SERVICE_TYPE;
#if defined ( __SUPPORT_XMDNS__)
                } else if (memcmp(header->nx_ip_header_destination_ip,
                                xmdns_ipv6_addr.nxd_ip_address.v6,
                                sizeof(header->nx_ip_header_destination_ip))
                         && zeroconf_is_running_xmdns_service()) {

                    protocol = XMDNS_SERVICE_TYPE;
#endif // (__SUPPORT_XMDNS__)
                } else {
                    unsigned char ipv6_str[40] = {0x00,};
                    ipv6long2str(rcv_ip_addr.nxd_ip_address.v6, ipv6_str);
                    ZEROCONF_DEBUG_TEMP(RED_COLOR
                                        "Dropped received packet(dest:%s)\n"
                                        CLEAR_COLOR,
                                        ipv6_str);

                    continue;
                }

                //set peer ip info
                parsed_payload.peer_ip.port = rcv_port;

                if (rcv_port != ZEROCONF_MDNS_PORT) {
                    parsed_payload.peer_ip.type = UNICAST_RESPONSE_TYPE;
                    memcpy(&parsed_payload.peer_ip.ip_addr,
                           &rcv_ip_addr,
                           sizeof(NXD_ADDRESS));
                } else {
                    parsed_payload.peer_ip.type = MULTICAST_RESPONSE_TYPE;

                    if (protocol == MDNS_SERVICE_TYPE) {
                        parsed_payload.peer_ip.ip_addr = get_mdns_ipv6_multicast();
#if defined (__SUPPORT_XMDNS__)
                    } else if (protocol == XMDNS_SERVICE_TYPE) {
                        parsed_payload.peer_ip.ip_addr = get_xmdns_ipv6_multicast();
#endif // (__SUPPORT_XMDNS__)
                    }
                }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 
            } else {
                ZEROCONF_DEBUG_INFO("Dropped received packet "
                                    "because of invaild ip address\n");
                continue;
            }

            if (protocol == MDNS_SERVICE_TYPE) {
                hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                                     hostname_bytes,
                                     ZEROCONF_DOMAIN_NAME_SIZE);
#if defined (__SUPPORT_XMDNS__)
            } else if (protocol == XMDNS_SERVICE_TYPE) {
                hostname_bytes_len = encode_mdns_name_string(
                                         g_zeroconf_xmdns_host_info.hostname,
                                         hostname_bytes,
                                         ZEROCONF_DOMAIN_NAME_SIZE);
#endif // (__SUPPORT_XMDNS__)
            }

            ret = parse_mdns_packet(payload, &parsed_payload);
            if (ret != DA_APP_SUCCESS) {
                ZEROCONF_DEBUG_ERR("failed to parse received packet(0x%x)\n", -ret);
                continue;
            }

            if (parsed_payload.transaction_id == 0) {
                if (parsed_payload.flags & 0x8400) {
                    ZEROCONF_DEBUG_INFO("Received mdns response packet(%ld)\n", payload_len);

                    for (rrecord = parsed_payload.answer_rrs ; rrecord != NULL ;
                         rrecord = rrecord->next) {
                        if (compare_domain_bytes(hostname_bytes,
                                                 hostname_bytes_len,
                                                 rrecord->name_bytes,
                                                 rrecord->name_bytes_len)) {

                            ZEROCONF_DEBUG_INFO(RED_COLOR
                                                "Hostname conflict. "
                                                "Reset to probing state\n"
                                                CLEAR_COLOR);

                            is_mdns_reset = pdTRUE;

                            break;
                        }

#if defined (__SUPPORT_DNS_SD__)
                        if ((g_zeroconf_local_dns_sd_service != NULL)
                            && (compare_data_bytes(g_zeroconf_local_dns_sd_service->q_bytes,
                                                   rrecord->name_bytes,
                                                   g_zeroconf_local_dns_sd_service->q_bytes_len))) {
                            ZEROCONF_DEBUG_INFO(RED_COLOR
                                                "dns-sd service conflict. "
                                                "Reset to probing state\n"
                                                CLEAR_COLOR);

                            is_dns_sd_reset = pdTRUE;

                            break;
                        }
#endif // (__SUPPORT_DNS_SD__)

                        da16x_sys_watchdog_notify(sys_wdog_id);
                        insertMdnsResp(rrecord->name_bytes,
                                       rrecord->name_bytes_len,
                                       rrecord->type,
                                       rrecord->ttl,
                                       xTaskGetTickCount(),
                                       rrecord->data_length,
                                       rrecord->data,
                                       0);
                    }
                } else if (parsed_payload.flags == 0) {
                    ZEROCONF_DEBUG_INFO("Received mdns request(%ld)\n", payload_len);

                    da16x_sys_watchdog_notify(sys_wdog_id);
                    send_multi_responses(protocol, &parsed_payload);
                } else {
                    ZEROCONF_DEBUG_INFO("Dropped received packet(flags:%x)\n",
                                        parsed_payload.flags);
                }

                if (dpm_mode_is_wakeup()) {
                    da16x_sys_watchdog_notify(sys_wdog_id);
                    garp_request(WLAN0_IFACE, pdFALSE);
                }

                if (is_mdns_reset) {
                    break;
                }

#if defined (__SUPPORT_DNS_SD__)
                if (is_dns_sd_reset) {
                    break;
                }
#endif // (__SUPPORT_DNS_SD__)
            }
        }
    }

end_of_task:

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    clear_mdns_packet(&parsed_payload);

    if (payload != NULL) {
        zeroconf_free(payload);
        payload = NULL;
    }

    if (is_mdns_reset) {
        zeroconf_start_restarter_task(ZEROCONF_RESTART_MDNS_SERVICE);
    }

#if defined (__SUPPORT_DNS_SD__)
    if (is_dns_sd_reset) {
        zeroconf_start_restarter_task(ZEROCONF_RESTART_DNS_SD_SERVICE);
    }
#endif // (__SUPPORT_DNS_SD__)

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.responder_proc.status = ZEROCONF_DEACTIVATED;

    ZEROCONF_DEBUG_TEMP("End of task\n");

    g_zeroconf_status.responder_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void sender_process(void *params)
{
    int sys_wdog_id = -1;
    zeroconf_pck_send_t *srch = NULL;
    int cur_time = 0;
    int send_index = 0;
    int remove_index = 0;
    int id_arr[100] = {0x00,};

    unsigned long sleep_time = portMAX_DELAY;
    unsigned long next_sleep_time = portMAX_DELAY;
    int time_diff = 0;
    EventBits_t events = 0;

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    g_zeroconf_status.sender_proc.status = ZEROCONF_ACTIVATED;

    ZEROCONF_DEBUG_TEMP("Start of task(%d)\n", g_zeroconf_status.sender_proc.status);

    if (!g_zeroconf_sender_event_handler) {
        g_zeroconf_sender_event_handler = xEventGroupCreate();
        if (g_zeroconf_sender_event_handler == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to create sender event\n");
            goto end;
        }
    } else {
        ZEROCONF_DEBUG_INFO("Event handler is already created\n");
    }

    while (g_zeroconf_status.sender_proc.status == ZEROCONF_ACTIVATED) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        ZEROCONF_DEBUG_TEMP("sleep_time(0x%x)\n", next_sleep_time);

        if (!g_zeroconf_sender_event_handler) {
            ZEROCONF_DEBUG_ERR("Event handler is released\n");
            break;
        }

        da16x_sys_watchdog_suspend(sys_wdog_id);

        events = xEventGroupWaitBits(g_zeroconf_sender_event_handler,
                                     ZEROCONF_EVT_ANY,
                                     pdTRUE,
                                     pdFALSE,
                                     next_sleep_time);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        ZEROCONF_DEBUG_INFO("events(0x%x)\n", events);

        if ((events & ZEROCONF_EVT_REMOVE_MDNS_ANNOUNCE)
            || (events & ZEROCONF_EVT_REMOVE_XMDNS_ANNOUNCE)
            || (events & ZEROCONF_EVT_REMOVE_DNS_SD_ANNOUNCE)) {
            sleep_time = portMAX_DELAY;
            next_sleep_time = portMAX_DELAY;

            send_index = 0;
            remove_index = 0;
            memset(id_arr, 0x00, sizeof(id_arr));

            srch = g_zeroconf_pck_send_root;

            if (g_zeroconf_pck_send_root) {
                ZEROCONF_DEBUG_TEMP("number of packets in send buffer:%d\n",
                                    g_zeroconf_pck_send_root->sent_count);
            }

            while (srch) {
                da16x_sys_watchdog_notify(sys_wdog_id);

                if ((events & ZEROCONF_EVT_REMOVE_MDNS_ANNOUNCE)
                    && srch->packet_type == MDNS_ANNOUNCE_TYPE) {
                    ZEROCONF_DEBUG_TEMP("Removed mdns announce(id:%d)\n", srch->id);

                    id_arr[send_index++] = srch->id;
                }

                if ((events & ZEROCONF_EVT_REMOVE_XMDNS_ANNOUNCE)
                    && srch->packet_type == XMDNS_ANNOUNCE_TYPE) {
                    ZEROCONF_DEBUG_TEMP("Removed xmdns announce(id:%d)\n", srch->id);

                    id_arr[send_index++] = srch->id;
                }

                if ((events & ZEROCONF_EVT_REMOVE_DNS_SD_ANNOUNCE)
                    && srch->packet_type == DNS_SD_ANNOUNCE_TYPE) {
                    ZEROCONF_DEBUG_TEMP("Removed dns-sd announce(id:%d)\n", srch->id);

                    id_arr[send_index++] = srch->id;
                }

                srch = srch->next;
            }

            for (remove_index = 0 ; remove_index < send_index ; remove_index++) {
                deletePckSend(id_arr[remove_index]);
            }
        }

        if (g_zeroconf_pck_send_root != NULL) {
            dpm_clr_zeroconf_sleep(SENDER_DPM_TYPE);

            ZEROCONF_DEBUG_TEMP("Number of packets in send buffer:%d\n",
                                g_zeroconf_pck_send_root->sent_count);

            sleep_time = portMAX_DELAY;
            next_sleep_time = portMAX_DELAY;
            time_diff = 0;
            cur_time = xTaskGetTickCount();

            send_index = 0;
            remove_index = 0;
            memset(id_arr, 0x00, sizeof(id_arr));

            srch = g_zeroconf_pck_send_root;

            do {
                da16x_sys_watchdog_notify(sys_wdog_id);

                time_diff = (cur_time - srch->last_sent_time);

                if (time_diff >= srch->interval) {
                    if ((srch->sent_count < srch->max_count)
                        || (srch->max_count == -1)) {
                        if (srch->peer_info[0].type != NONE_RESPONSE_TYPE) {
                            send_mdns_packet_with_dest((zeroconf_service_type_t)srch->type,
                                                       srch->full_packet,
                                                       srch->len,
                                                       srch->peer_info[0].ip_addr);
                        }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
                        if (srch->peer_info[1].type != NONE_RESPONSE_TYPE) {
                            send_mdns_packet_with_dest((zeroconf_service_type_t)srch->type,
                                                       srch->full_packet,
                                                       srch->len,
                                                       srch->peer_info[1].ip_addr);
                        }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

                        if ((srch->sent_count + 1 < srch->max_count)
                            || (srch->max_count == -1)) {
                            srch->sent_count++;
                            srch->last_sent_time = cur_time;
                            srch->interval = srch->interval * srch->mult_factor;
                            sleep_time = srch->interval;
                        } else {
                            id_arr[send_index++] = srch->id;
                        }
                    } else {
                        id_arr[send_index++] = srch->id;
                    }
                } else {
                    sleep_time = srch->interval - time_diff;
                }

                if (sleep_time < next_sleep_time) {
                    next_sleep_time = sleep_time;
                }

                srch = srch->next;
            } while (srch != NULL);

            for (remove_index = 0 ; remove_index < send_index ; remove_index++) {
                deletePckSend(id_arr[remove_index]);
            }

            dpm_set_zeroconf_sleep(SENDER_DPM_TYPE);
        }

        cache_clear_process();
    }

end:

    if (g_zeroconf_sender_event_handler) {
        vEventGroupDelete(g_zeroconf_sender_event_handler);
        g_zeroconf_sender_event_handler = NULL;
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.sender_proc.status = ZEROCONF_DEACTIVATED;

    ZEROCONF_DEBUG_TEMP("End of task\n");

    g_zeroconf_status.sender_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void cache_clear_process(void)
{
    clear_mdns_invalid_ttls();
}

void wait_for_close(void)
{
    char ch;

    while (pdTRUE) {
        ch = (char)getchar();
        if (ch == 'x' || ch == 'X') {
            break;
        }

        vTaskDelay(1);
    }

    ZEROCONF_PRINTF("\n");
    return;
}

unsigned long zeroconf_get_mdns_ipv4_address_by_name(char *addr, unsigned long wait_option)
{
    unsigned long ipaddr = 0 ;
    char ipaddr_str[IPADDR_LEN] = {0x00,};

    //check ip
    if (is_in_valid_ip_class(addr)) {
        ipaddr = (unsigned long)iptolong(addr);
    } else {
        to_lower(addr);

        //redirect all .local queries to mDNS
        if ((strlen(addr) > 6)
            && (strcmp(addr + strlen(addr) - 6, mdns_top_domain) == 0)) {
            if (mdns_Query(MDNS_SERVICE_TYPE, addr, ipaddr_str) == DA_APP_SUCCESS) {
                ipaddr = (unsigned long)iptolong(ipaddr_str);
            }
#if defined (__SUPPORT_XMDNS__)
        } else if ((strlen(addr) > 5)
                 && (strcmp(addr + strlen(addr) - 5, xmdns_top_domain) == 0)) {
            //redirect all .site queries to mDNS
            if (mdns_Query(XMDNS_SERVICE_TYPE, addr, ipaddr_str) == DA_APP_SUCCESS) {
                ipaddr = (unsigned long)iptolong(ipaddr_str);
            }
#endif // (__SUPPORT_XMDNS__)
        } else {
            ipaddr = (unsigned long)iptolong(dns_A_Query(addr, wait_option));
        }
    }

    return ipaddr;
}

int insertOwnHostInfo(const zeroconf_service_type_t service_key)
{
    unsigned char data[4] = {127, 0, 0, 1};

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    if (service_key == MDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                             hostname_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);
        if (hostname_bytes_len == 0) {
            ZEROCONF_DEBUG_ERR("Failed to encode hostname(%s)\n",
                               g_zeroconf_mdns_host_info.hostname);
            return DA_APP_NOT_SUCCESSFUL;
        }

        insertMdnsResp(hostname_bytes,
                       hostname_bytes_len,
                       ZEROCONF_MDNS_RR_TYPE_A,
                       -1,
                       xTaskGetTickCount(),
                       sizeof(data),
                       data,
                       0);
#if defined (__SUPPORT_XMDNS__)
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        hostname_bytes_len = encode_mdns_name_string(
                                 g_zeroconf_xmdns_host_info.hostname,
                                 hostname_bytes,
                                 ZEROCONF_DOMAIN_NAME_SIZE);
        if (hostname_bytes_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to encode hostname(%s)\n",
                               g_zeroconf_xmdns_host_info.hostname);


            return DA_APP_NOT_SUCCESSFUL;
        }

        insertMdnsResp(hostname_bytes,
                       hostname_bytes_len,
                       ZEROCONF_MDNS_RR_TYPE_A,
                       -1,
                       xTaskGetTickCount(),
                       sizeof(data),
                       data,
                       0);
#endif // (__SUPPORT_XMDNS__)
    } else {
        ZEROCONF_DEBUG_ERR("not supported service(%d)\n", service_key);
        return DA_APP_INVALID_PARAMETERS;
    }

    return DA_APP_SUCCESS;
}

int get_offset(unsigned char msb, unsigned char lsb)
{
    int arr[8] = {0x00,};
    int i = 0, a = 0;

    while (msb > 0) {
        arr[i] = (msb % 2);
        msb = msb / 2;
        i++;
    }

    for (i = 7 ; i >= 0 ; i--) {
        if (i == 7 || i == 6) {
            continue;
        } else {
            if (arr[i] != 0) {
                a = a + (int)pow_long(2, (i + 8));
            }
        }
    }

    a = a + lsb;

    return a;
}

void set_offset(int val, unsigned char *offset_arr)
{
    int arr[16] = {0x00,};
    int i = 0, a = 0;
    arr[15] = 1;
    arr[14] = 1;

    while (val > 0) {
        arr[i] = (val % 2);
        val = val / 2;
        i++;
    }

    for (i = 15 ; i >= 8 ; i--) {
        if (arr[i] != 0) {
            a = a + (int)pow_long(2, (i - 8));
        }
    }

    offset_arr[0] = a;
    a = 0;

    for (i = 7 ; i >= 0 ; i--) {
        if (arr[i] != 0) {
            a = a + (int)pow_long(2, (i));
        }
    }

    offset_arr[1] = a;
}

void to_lower(char *str)
{
    int i = 0;

    while (str[i]) {
        str[i] = (char)tolower(str[i]);
        i++;
    }
}

int random_number(int min_num, int max_num)
{
    int result = 0, low_num = 0, hi_num = 0;

    if (min_num < max_num) {
        low_num = min_num;
        // this is done to include max_num in output.
        hi_num = max_num + 1;
    } else {
        // this is done to include max_num in output.
        low_num = max_num + 1;

        hi_num = min_num;
    }

    srand(xTaskGetTickCount());
    result = (rand() % (hi_num - low_num)) + low_num;

    return result;
}

void create_packet_header(int QDCOUNT, int ANCOUNT, int NSCOUNT, int ARCOUNT,
                          int isRequest, unsigned char *arr, int idx)
{
    arr[idx + 0] = 0;
    arr[idx + 1] = 0;
    arr[idx + 2] = 132;

    if (isRequest) {
        arr[idx + 2] = 0;
    }

    arr[idx + 3] = 0;
    arr[idx + 4] = (int) QDCOUNT / 256;
    arr[idx + 5] = (int) QDCOUNT % 256;
    arr[idx + 6] = (int) ANCOUNT / 256;
    arr[idx + 7] = (int) ANCOUNT % 256;
    arr[idx + 8] = (int) NSCOUNT / 256;
    arr[idx + 9] = (int) NSCOUNT % 256;
    arr[idx + 10] = (int) ARCOUNT / 256;
    arr[idx + 11] = (int) ARCOUNT % 256;
}

unsigned int create_query(unsigned char *name_bytes, size_t name_bytes_len,
                          unsigned short type, unsigned short qclass,
                          unsigned char *out, int start)
{
    unsigned char *ptr = out + start;

    memcpy(ptr, name_bytes, name_bytes_len);
    ptr += name_bytes_len;

    //type
    *ptr++ = (unsigned char)((type & 0xff00) >> 8);
    *ptr++ = (unsigned char)(type & 0x00ff);

    //qu & class
    *ptr++ = (unsigned char)((qclass & 0xff00) >> 8);
    *ptr++ = (unsigned char)(qclass & 0x00ff);

    return (ptr - (out + start));
}

unsigned int create_general_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                unsigned short type, unsigned short rclass,
                                unsigned int ttl, unsigned short data_length,
                                unsigned char *data, unsigned char *out, int start)
{
    unsigned char *ptr = out + start;

    //name
    memcpy(ptr, name_bytes, name_bytes_len);
    ptr += name_bytes_len;

    //type
    *ptr++ = (unsigned char)((type & 0xff00) >> 8);
    *ptr++ = (unsigned char)(type & 0x00ff);

    //class
    *ptr++ = (unsigned char)((rclass & 0xff00) >> 8);
    *ptr++ = (unsigned char)(rclass & 0x00ff);

    //ttl
    *ptr++ = (unsigned char)((ttl & 0xff000000) >> 24);
    *ptr++ = (unsigned char)((ttl & 0x00ff0000) >> 16);
    *ptr++ = (unsigned char)((ttl & 0x0000ff00) >> 8);
    *ptr++ = (unsigned char)(ttl & 0x000000ff);

    //data length
    *ptr++ = (unsigned char)((data_length & 0xff00) >> 8);
    *ptr++ = (unsigned char)(data_length & 0x00ff);

    //data
    if (data_length) {
        memcpy(ptr, data, data_length);
        ptr += data_length;
    }

    return (ptr - (out + start));
}

unsigned int create_mdns_srv_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                 unsigned short rclass, unsigned int ttl, int port,
                                 unsigned char *host_bytes, size_t host_bytes_len,
                                 unsigned char *out, int start)
{
    unsigned int data_length = host_bytes_len + 6;
    unsigned char *ptr = out + start;

    //name
    memcpy(ptr, name_bytes, name_bytes_len);
    ptr += name_bytes_len;

    //type
    *ptr++ = (unsigned char)((ZEROCONF_MDNS_RR_TYPE_SRV & 0xff00) >> 8);
    *ptr++ = (unsigned char)(ZEROCONF_MDNS_RR_TYPE_SRV & 0x00ff);

    //class
    *ptr++ = (unsigned char)((rclass & 0xff00) >> 8);
    *ptr++ = (unsigned char)(rclass & 0x00ff);

    //ttl
    *ptr++ = (unsigned char)((ttl & 0xff000000) >> 24);
    *ptr++ = (unsigned char)((ttl & 0x00ff0000) >> 16);
    *ptr++ = (unsigned char)((ttl & 0x0000ff00) >> 8);
    *ptr++ = (unsigned char)(ttl & 0x000000ff);

    //len
    *ptr++ = (unsigned char)((data_length & 0xff00) >> 8);
    *ptr++ = (unsigned char)(data_length & 0x00ff);

    //priority
    *ptr++ = 0;
    *ptr++ = 0;

    //weight
    *ptr++ = 0;
    *ptr++ = 0;

    //port
    *ptr++ = (unsigned char)((port & 0xff00) >> 8);
    *ptr++ = (unsigned char)(port & 0x00ff);

    //host bytes
    if (host_bytes_len) {
        memcpy(ptr, host_bytes, host_bytes_len);
        ptr += host_bytes_len;
    }

    return (ptr - (out + start));
}

unsigned int create_mdns_txt_rec(unsigned char *name_bytes, size_t name_bytes_len,
                                 unsigned short rclass, unsigned int ttl,
                                 size_t txtLen, unsigned char *txtRecord,
                                 unsigned char *out, int start)
{
    unsigned char *ptr = out + start;

    //name
    memcpy(ptr, name_bytes, name_bytes_len);
    ptr += name_bytes_len;

    //type
    *ptr++ = (unsigned char)((ZEROCONF_MDNS_RR_TYPE_TXT & 0xff00) >> 8);
    *ptr++ = (unsigned char)(ZEROCONF_MDNS_RR_TYPE_TXT & 0x00ff);

    //class
    *ptr++ = (unsigned char)((rclass & 0xff00) >> 8);
    *ptr++ = (unsigned char)(rclass & 0x00ff);

    //ttl
    *ptr++ = (unsigned char)((ttl & 0xff000000) >> 24);
    *ptr++ = (unsigned char)((ttl & 0x00ff0000) >> 16);
    *ptr++ = (unsigned char)((ttl & 0x0000ff00) >> 8);
    *ptr++ = (unsigned char)(ttl & 0x000000ff);

    //data length
    *ptr++ = (unsigned char)(((txtLen) & 0xff00) >> 8);
    *ptr++ = (unsigned char)((txtLen) & 0x00ff);

    //txt length
    if (txtLen > 255) {
        *ptr++ = 0xff;
        memcpy(ptr, txtRecord, 255);
        ptr += 255;
    } else {
        memcpy(ptr, txtRecord, txtLen);
        ptr += txtLen;
    }

    return (ptr - (out + start));
}

unsigned int convert_questions_to_bytes(zeroconf_question_t *questions,
                                        unsigned char *out, size_t out_len)
{
    unsigned char *ptr = out;
    zeroconf_question_t *question = NULL;

    DA16X_UNUSED_ARG(out_len);

    for (question = questions ; question != NULL ; question = question->next) {
        ptr += create_query(question->name_bytes, question->name_bytes_len,
                            question->type, question->qclass, ptr, 0);
    }

    return (ptr - out);
}

unsigned int convert_resource_records_to_bytes(zeroconf_resource_record_t *rrecords,
                                               unsigned char *out, size_t out_len)
{
    unsigned char *ptr = out;
    zeroconf_resource_record_t *rrecord = NULL;

    DA16X_UNUSED_ARG(out_len);

    for (rrecord = rrecords ; rrecord != NULL ; rrecord = rrecord->next) {
        ptr += create_general_rec(rrecord->name_bytes, rrecord->name_bytes_len,
                                  rrecord->type, rrecord->rclass, rrecord->ttl,
                                  rrecord->data_length, rrecord->data, ptr, 0);
    }

    return (ptr - out);
}

unsigned int convert_mdns_packet_to_bytes(zeroconf_mdns_packet_t *mdns_packet,
                                          unsigned char *out, size_t out_len)
{
    unsigned int data_len = 0;
    unsigned char *ptr = out;
    unsigned char *end_ptr = out + out_len;

    memset(out, 0x00, out_len);

    if ((mdns_packet->questions_count == 0)
        && (mdns_packet->answer_rrs_count == 0)
        && (mdns_packet->authority_rrs_count == 0)
        && (mdns_packet->additional_rrs_count == 0)) {
        ZEROCONF_DEBUG_INFO("Invalid mdns pakcet\n");
        return 0;
    }

    *ptr++ = (unsigned char)((mdns_packet->transaction_id & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->transaction_id & 0x00ff);

    *ptr++ = (unsigned char)((mdns_packet->flags & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->flags & 0x00ff);

    *ptr++ = (unsigned char)((mdns_packet->questions_count & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->questions_count & 0x00ff);

    *ptr++ = (unsigned char)((mdns_packet->answer_rrs_count & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->answer_rrs_count & 0x00ff);

    *ptr++ = (unsigned char)((mdns_packet->authority_rrs_count & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->authority_rrs_count & 0x00ff);

    *ptr++ = (unsigned char)((mdns_packet->additional_rrs_count & 0xff00) >> 8);
    *ptr++ = (unsigned char)(mdns_packet->additional_rrs_count & 0x00ff);

    if (mdns_packet->questions_count) {
        data_len = convert_questions_to_bytes(mdns_packet->questions, ptr,
                                              (end_ptr - ptr));
        if (data_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to convert questions\n");
            return 0;
        }

        ZEROCONF_DEBUG_TEMP("questions count(%d), data_len(%d)\n",
                            mdns_packet->questions_count, data_len);

        ptr += data_len;
    }

    if (mdns_packet->answer_rrs_count) {
        data_len = convert_resource_records_to_bytes(mdns_packet->answer_rrs,
                                                     ptr, (end_ptr - ptr));
        if (data_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to convert answer_rrs\n");
            return 0;
        }

        ZEROCONF_DEBUG_TEMP("answer rr count(%d), data_len(%d)\n",
                            mdns_packet->answer_rrs_count, data_len);

        ptr += data_len;
    }

    if (mdns_packet->authority_rrs_count) {
        data_len = convert_resource_records_to_bytes(mdns_packet->authority_rrs,
                                                     ptr, (end_ptr - ptr));
        if (data_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to convert authority_rrs\n");
            return 0;
        }

        ZEROCONF_DEBUG_TEMP("authority rr count(%d), data_len(%d)\n",
                            mdns_packet->authority_rrs_count, data_len);

        ptr += data_len;
    }

    if (mdns_packet->additional_rrs_count) {
        data_len = convert_resource_records_to_bytes(mdns_packet->additional_rrs,
                                                     ptr, (end_ptr - ptr));
        if (data_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to convert additional_rrs\n");
            return 0;
        }

        ZEROCONF_DEBUG_TEMP("additional rr count(%d), data_len(%d)\n",
                            mdns_packet->additional_rrs_count, data_len);

        ptr += data_len;
    }


    return (ptr - out);
}

unsigned int calculate_resource_record_delay(zeroconf_resource_record_t *rrecords,
                                             unsigned int is_truncated)
{
    const unsigned int shared_record_delay_min = 2;
    const unsigned int shared_record_delay_max = 12;

    const unsigned int trucated_delay_min = 40;
    const unsigned int trucated_delay_max = 50;

    unsigned int delay = 0;
    unsigned int random_delay  = 0;
    zeroconf_resource_record_t *rrecord = NULL;

    for (rrecord = rrecords ; rrecord != NULL ; rrecord = rrecord->next) {
        //shared record
        if (rrecord->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
            if (is_truncated) {
                random_delay = random_number(trucated_delay_min, trucated_delay_max);
            } else {
                random_delay = random_number(shared_record_delay_min,
                                             shared_record_delay_max);
            }

            if (delay == 0 || random_delay < delay) {
                delay = random_delay;
            }
        }
    }

    return delay;
}

unsigned int is_known_answer_rr(zeroconf_resource_record_t *answer_rrs,
                                unsigned char *name_bytes,
                                size_t name_bytes_len,
                                unsigned int ttl)
{
    unsigned int is_known_answer_rr = pdFALSE;
    zeroconf_resource_record_t *answer_rr = NULL;

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

#if defined (__SUPPORT_DNS_SD__)
    unsigned char service_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t service_bytes_len = 0;
#endif // (__SUPPORT_DNS_SD__)

    hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                                                 hostname_bytes,
                                                 ZEROCONF_DOMAIN_NAME_SIZE);

#if defined (__SUPPORT_DNS_SD__)
    if (g_zeroconf_local_dns_sd_service != NULL) {
        memcpy(service_bytes,
               g_zeroconf_local_dns_sd_service->q_bytes,
               g_zeroconf_local_dns_sd_service->q_bytes_len);

        service_bytes_len = g_zeroconf_local_dns_sd_service->q_bytes_len;
    }
#endif // (__SUPPORT_DNS_SD__)

    for (answer_rr = answer_rrs ; answer_rr != NULL ; answer_rr = answer_rr->next) {
        if ((compare_domain_bytes(answer_rr->name_bytes, answer_rr->name_bytes_len,
                                  name_bytes, name_bytes_len))
            && answer_rr->ttl > ttl / 2) {
            if (answer_rr->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                if (hostname_bytes_len && answer_rr->data_length) {
                    if (compare_domain_bytes(answer_rr->data,
                                             answer_rr->data_length,
                                             hostname_bytes,
                                             hostname_bytes_len)) {

                        ZEROCONF_DEBUG_TEMP(YELLOW_COLOR
                                            "known answer with hostname"
                                            "(%s's ttl:%d)(ori ttl:%d)\n"
                                            CLEAR_COLOR,
                                            answer_rr->name_bytes, answer_rr->ttl, ttl);

                        is_known_answer_rr = pdTRUE;

                        break;
                    }
                }

#if defined (__SUPPORT_DNS_SD__)
                if (service_bytes_len && answer_rr->data_length) {
                    if (compare_domain_bytes(answer_rr->data,
                                             answer_rr->data_length,
                                             service_bytes,
                                             service_bytes_len)) {

                        ZEROCONF_DEBUG_TEMP(YELLOW_COLOR
                                            "known answer with service name"
                                            "(%s's ttl:%d)(ori ttl:%d)\n"
                                            CLEAR_COLOR,
                                            answer_rr->name_bytes, answer_rr->ttl, ttl);

                        is_known_answer_rr = pdTRUE;

                        break;
                    }
                }
#endif // (__SUPPORT_DNS_SD__)
            }
        }
    }

    return is_known_answer_rr;
}

int insert_question(zeroconf_question_t **questions, unsigned char *name_bytes,
                    size_t name_bytes_len, unsigned short type, unsigned short qclass)
{
    zeroconf_question_t *head = *questions;
    zeroconf_question_t *question = NULL;
    zeroconf_question_t *temp = NULL;

    unsigned int is_new_question = pdTRUE;

    if (head != NULL) {
        for (temp = head ; temp != NULL ; temp = temp->next) {
            if (compare_domain_bytes(name_bytes, name_bytes_len,
                                     temp->name_bytes, temp->name_bytes_len)
                && temp->type == type) {
                is_new_question = pdFALSE;
            }
        }
    }

    if (is_new_question) {
        question = create_question(name_bytes, name_bytes_len, type, qclass);
        if (question == NULL) {
            ZEROCONF_DEBUG_ERR("failed to create question\n");
            return DA_APP_NOT_CREATED;
        }

        if (head == NULL) {
            *questions = question;
        } else {
            for (temp = head ; temp->next != NULL ; temp = temp->next);

            temp->next = question;
        }
    } else {
        return DA_APP_DUPLICATED_ENTRY;
    }

    return DA_APP_SUCCESS;
}

zeroconf_question_t *create_question(unsigned char *name_bytes,
                                     size_t name_bytes_len,
                                     unsigned short type,
                                     unsigned short qclass)
{
    zeroconf_question_t *question = NULL;

    question = zeroconf_calloc(1, sizeof(zeroconf_question_t));
    if (question == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for question\n");
        return NULL;
    }

    question->name_bytes = zeroconf_calloc(name_bytes_len + 1, 1);
    if (question->name_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for name bytes\n");
        goto error;
    }

    memcpy(question->name_bytes, name_bytes, name_bytes_len);

    question->name_bytes_len = name_bytes_len;

    question->type = type;

    question->qclass = qclass;

    question->next = NULL;

    return question;

error:

    if (question) {
        if (question->name_bytes) {
            zeroconf_free(question->name_bytes);
        }

        zeroconf_free(question);
    }

    return NULL;
}

int insert_resource_record(zeroconf_resource_record_t **rrecords,
                           unsigned char *name_bytes,
                           size_t name_bytes_len,
                           unsigned short type,
                           unsigned short rclass,
                           unsigned int ttl,
                           unsigned short data_length,
                           unsigned char *data)
{
    zeroconf_resource_record_t *head = *rrecords;
    zeroconf_resource_record_t *rrecord = NULL;
    zeroconf_resource_record_t *temp = NULL;
    unsigned int is_new_rrecord = pdTRUE;

    if (head != NULL) {
        for (temp = head ; temp != NULL ; temp = temp->next) {
            if (compare_domain_bytes(name_bytes, name_bytes_len,
                                     temp->name_bytes, temp->name_bytes_len)
                && temp->type == type) {
                is_new_rrecord = pdFALSE;
                break;
            }
        }
    }

    if (is_new_rrecord) {
        rrecord = create_resource_record(name_bytes, name_bytes_len,
                                         type, rclass, ttl,
                                         data_length, data);
        if (rrecord == NULL) {
            ZEROCONF_DEBUG_ERR("failed to create resource record\n");
            return DA_APP_NOT_CREATED;
        }

        if (head == NULL) {
            *rrecords = rrecord;
        } else {
            for (temp = head ; temp->next != NULL ; temp = temp->next);

            temp->next = rrecord;
        }
    } else {
        return DA_APP_DUPLICATED_ENTRY;
    }

    return DA_APP_SUCCESS;
}


zeroconf_resource_record_t *create_resource_record(unsigned char *name_bytes,
                                                   size_t name_bytes_len,
                                                   unsigned short type,
                                                   unsigned short rclass,
                                                   unsigned int ttl,
                                                   unsigned short data_length,
                                                   unsigned char *data)
{
    zeroconf_resource_record_t *rrecord = NULL;

    rrecord = zeroconf_calloc(1, sizeof(zeroconf_resource_record_t));
    if (rrecord == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for resource record\n");
        return NULL;
    }

    rrecord->name_bytes = zeroconf_calloc(name_bytes_len + 1, 1);
    if (rrecord->name_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for name bytes\n");
        goto error;
    }

    memcpy(rrecord->name_bytes, name_bytes, name_bytes_len);

    rrecord->name_bytes_len = name_bytes_len;

    rrecord->type = type;

    rrecord->rclass = rclass;

    rrecord->ttl = ttl;

    rrecord->data_length = data_length;

    if (data_length) {
        rrecord->data = zeroconf_calloc(data_length, 1);
        if (rrecord->data == NULL) {
            ZEROCONF_DEBUG_ERR("failed to allocate memory "
                               "for data of resource record\n");
            goto error;
        }

        memcpy(rrecord->data, data, data_length);
    } else {
        rrecord->data = NULL;
    }

    rrecord->next = NULL;

    return rrecord;

error:

    if (rrecord) {
        if (rrecord->name_bytes) {
            zeroconf_free(rrecord->name_bytes);
        }

        if (rrecord->data) {
            zeroconf_free(rrecord->data);
        }

        zeroconf_free(rrecord);
    }

    return NULL;
}

int insert_srv_record(zeroconf_resource_record_t **rrecords,
                      unsigned char *name_bytes, size_t name_bytes_len,
                      unsigned short rclass, unsigned int ttl, unsigned short priority,
                      unsigned short weight, unsigned short port,
                      unsigned short data_length, unsigned char *data)
{
    zeroconf_resource_record_t *head = *rrecords;
    zeroconf_resource_record_t *rrecord = NULL;
    zeroconf_resource_record_t *temp = NULL;
    unsigned int is_new_rrecord = pdTRUE;

    unsigned char *srv_ptr = NULL;
    unsigned char *srv_data = NULL;
    size_t srv_data_len = 0;

    if (head != NULL) {
        for (temp = head ; temp != NULL ; temp = temp->next) {
            if (compare_domain_bytes(name_bytes, name_bytes_len,
                                     temp->name_bytes, temp->name_bytes_len)
                && temp->type == ZEROCONF_MDNS_RR_TYPE_SRV) {
                is_new_rrecord = pdFALSE;
                break;
            }
        }
    }

    if (is_new_rrecord) {
        srv_data_len = 6 + data_length;

        srv_data = zeroconf_calloc(srv_data_len, 1);
        if (srv_data == NULL) {
            ZEROCONF_DEBUG_ERR("failed to create srv data\n");
            return -1;
        }

        srv_ptr = srv_data;

        *srv_ptr++ = (unsigned char)((priority & 0xff00) >> 8);
        *srv_ptr++ = (unsigned char)(priority & 0x00ff);
        *srv_ptr++ = (unsigned char)((weight & 0xff00) >> 8);
        *srv_ptr++ = (unsigned char)(weight & 0x00ff);
        *srv_ptr++ = (unsigned char)((port & 0xff00) >> 8);
        *srv_ptr++ = (unsigned char)(port & 0x00ff);
        memcpy(srv_ptr, data, data_length);

        rrecord = create_resource_record(name_bytes,
                                         name_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_SRV,
                                         rclass,
                                         ttl,
                                         srv_data_len,
                                         srv_data);
        if (rrecord == NULL) {
            ZEROCONF_DEBUG_ERR("failed to create resource record\n");

            zeroconf_free(srv_data);
            return DA_APP_NOT_CREATED;
        }

        if (head == NULL) {
            *rrecords = rrecord;
        } else {
            for (temp = head ; temp->next != NULL ; temp = temp->next);

            temp->next = rrecord;
        }

        zeroconf_free(srv_data);
    } else {
        return DA_APP_DUPLICATED_ENTRY;
    }

    return DA_APP_SUCCESS;
}

int set_host_info(const zeroconf_service_type_t service_key,
                  char *hostname, const char *domain)
{
    struct sockaddr_in ipv4_addr = (struct sockaddr_in)get_ipv4_address();
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // ( LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    zeroconf_mdns_host_info_t *host_info = NULL;

    if (service_key == MDNS_SERVICE_TYPE) {
        host_info = &g_zeroconf_mdns_host_info;
    } else if (service_key == XMDNS_SERVICE_TYPE) {
        host_info = &g_zeroconf_xmdns_host_info;
    } else {
        ZEROCONF_DEBUG_ERR("Not supported service(%d)\n", service_key);
        return DA_APP_INVALID_PARAMETERS;
    }

    memset(host_info, 0x00, sizeof(zeroconf_mdns_host_info_t));

    sprintf(host_info->hostname, "%s%s", hostname, domain);

    if (is_supporting_ipv4()) {
        sprintf(host_info->arpa_v4_hostname,
                "%ld.%ld.%ld.%ld.in-addr.arpa",
                ((ntohl(ipv4_addr.sin_addr.s_addr) >> 0) & 0x0ff),
                ((ntohl(ipv4_addr.sin_addr.s_addr) >> 8) & 0x0ff),
                ((ntohl(ipv4_addr.sin_addr.s_addr) >> 16) & 0x0ff),
                ((ntohl(ipv4_addr.sin_addr.s_addr) >> 24) & 0x0ff));
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    if (is_supporting_ipv6()) {
        for (int index = 0 ; index < 16 ; index++) {
            const char hexValues[] = "0123456789ABCDEF";
            host_info->arpa_v6_hostname[index * 4 + 0] = hexValues[str_ipv6[15 - index] &
                    0x0F];
            host_info->arpa_v6_hostname[index * 4 + 1] = '.';
            host_info->arpa_v6_hostname[index * 4 + 2] = hexValues[str_ipv6[15 - index] >>
                    4];
            host_info->arpa_v6_hostname[index * 4 + 3] = '.';
        }

        strcat(host_info->arpa_v6_hostname, "ip6.arpa");
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    ZEROCONF_DEBUG_TEMP("hostname info\n"
                        ">>>>> service\t : %d\n"
                        ">>>>> hostname(%ld)\t: %s\n"
                        ">>>>> arpa(v4)(%ld)\t: %s\n"
                        ">>>>> arpa(v6)(%ld)\t: %s\n",
                        service_key,
                        strlen(host_info->hostname),
                        host_info->hostname,
                        strlen(host_info->arpa_v4_hostname),
                        host_info->arpa_v4_hostname,
                        strlen(host_info->arpa_v6_hostname),
                        host_info->arpa_v6_hostname);
#else
    ZEROCONF_DEBUG_TEMP("hostname info\n"
                        ">>>>> service\t : %d\n"
                        ">>>>> hostname(%ld)\t: %s\n"
                        ">>>>> arpa(v4)(%ld)\t: %s\n",
                        service_key,
                        strlen(host_info->hostname),
                        host_info->hostname,
                        strlen(host_info->arpa_v4_hostname),
                        host_info->arpa_v4_hostname);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    return DA_APP_SUCCESS;
}

unsigned int compare_domain_bytes(unsigned char *domain_bytes,
                                  size_t domain_bytes_len,
                                  unsigned char *rcvd_bytes,
                                  size_t rcvd_bytes_len)
{
    unsigned int i = 0, j = 0;

    if (domain_bytes_len != rcvd_bytes_len) {
        return pdFALSE;
    }

    for (i = 0, j = 0; i < domain_bytes_len; i++, j++) {
        if (domain_bytes[j] != rcvd_bytes[i]) {
            if ((domain_bytes[j] >= 65) && (domain_bytes[j] <= 90)) {
                unsigned char x = rcvd_bytes[i] - 32;

                if (domain_bytes[j] != x) {
                    return pdFALSE;
                }
            } else if ((domain_bytes[j] >= 97) && (domain_bytes[j] <= 122)) {
                unsigned char x = rcvd_bytes[i] + 32;

                if (domain_bytes[j] != x) {
                    return pdFALSE;
                }
            } else {
                return pdFALSE;
            }
        }
    }

    return pdTRUE;
}

int compare_data_bytes(unsigned char *data1, unsigned char *data2, int len)
{
    if (len == 0) {
        return pdFALSE;
    }

    for (int i = 0; i < (len); i++) {
        if (data1[i] != data2[i]) {
            return pdFALSE;
        }
    }

    return pdTRUE;
}

unsigned int search_in_byte_array(unsigned char *source, size_t source_len,
                                  unsigned char *key, size_t key_len)
{
    unsigned int i = 0, j = 0;

    to_lower((char *)key);
    to_lower((char *)source);

    if (source_len == key_len) {
        for (i = 0; i < source_len ; i++) {
            if (source[i] != key[i]) {
                return pdFALSE;
            }
        }

        return pdTRUE;

    } else if (source_len > key_len) {
        for (j = 0; j <= (source_len - key_len); ++j) {
            for (i = 0; i < key_len && key[i] == source[i + j]; ++i);

            if (i >= key_len) {
                return pdTRUE;
            }
        }
    }

    return pdFALSE;
}

void reset_lists(void)
{
    if (g_zeroconf_mdns_header.query_count <= 0) {
        g_zeroconf_query_string_root = NULL;
        g_zeroconf_mdns_header.query_count = 0;
    }

    if (g_zeroconf_mdns_header.sent_count <= 0) {
        g_zeroconf_pck_send_root = NULL;
        g_zeroconf_mdns_header.sent_count = 0;
    }

    if (g_zeroconf_mdns_header.resp_count <= 0) {
        g_zeroconf_mdns_response_root = NULL;
        g_zeroconf_mdns_header.resp_count = 0;
    }
}

void clear_query_cache(void)
{
    zeroconf_query_string_t *temp = NULL;

    while (g_zeroconf_query_string_root != NULL) {
        temp = g_zeroconf_query_string_root;
        g_zeroconf_query_string_root = g_zeroconf_query_string_root->next;
        g_zeroconf_mdns_header.query_count--;
        zeroconf_free(temp);
    }

    reset_lists();
}

void clear_send_cache(int type)
{
    zeroconf_pck_send_t *cur = g_zeroconf_pck_send_root;
    zeroconf_pck_send_t *prev = NULL;
    zeroconf_pck_send_t *del = NULL;

    while (cur) {
        if (cur->type == type) {
            if (cur == g_zeroconf_pck_send_root) {
                g_zeroconf_pck_send_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            zeroconf_free(del->full_packet);
            zeroconf_free(del);

            g_zeroconf_mdns_header.sent_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void print_mdns_response(zeroconf_mdns_response_t *resp, int print_raw, int show_date)
{
    char *to_print = NULL;
    char time_str[100] = {0x00,};
    unsigned int i = 0;
#if defined (__TIME64__)
    __time64_t now = resp->timestamp;
#else
    time_t now = (int)resp->timestamp;
#endif // (__TIME64__)
    int port = 0;

#if defined (__TIME64__)
    da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime64(&now));
#else
    da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime32(&now));
#endif // (__TIME64__)

    to_print = zeroconf_calloc(resp->query_len, 1);
    if (to_print == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for query\n");
        return;
    }

    if (unencode_mdns_name_string(resp->query, 0, to_print, resp->query_len)) {
        ZEROCONF_PRINTF("\n>> %s  ", to_print);
    }

    if (show_date) {
        ZEROCONF_PRINTF(" (Cached on %s )", time_str);
    }

    ZEROCONF_PRINTF("\n");

    zeroconf_free(to_print);
    to_print = NULL;

    switch (resp->type) {
        case ZEROCONF_MDNS_RR_TYPE_A:
            ZEROCONF_PRINTF ("\tA(IPv4)");
            ZEROCONF_PRINTF ("\t%d.%d.%d.%d",
                             resp->data[0],
                             resp->data[1],
                             resp->data[2],
                             resp->data[3]);
            break;
        case ZEROCONF_MDNS_RR_TYPE_AAAA:
            ZEROCONF_PRINTF ("\tAAAA(IPv6)");
            ZEROCONF_PRINTF ("\t%x%x.%x%x.%x%x.%x%x.%x%x.%x%x.%x%x.%x%x",
                             resp->data[0], resp->data[1],
                             resp->data[2], resp->data[3],
                             resp->data[4], resp->data[5],
                             resp->data[6], resp->data[7],
                             resp->data[8], resp->data[9],
                             resp->data[10], resp->data[11],
                             resp->data[12], resp->data[13],
                             resp->data[14], resp->data[15]);
            break;
        case ZEROCONF_MDNS_RR_TYPE_HINFO:
            ZEROCONF_PRINTF ("\tHOST_INFO");

            to_print = zeroconf_calloc(resp->data_len, 1);
            if (to_print == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory for hinfo\n");
                break;
            }

            if (unencode_mdns_name_string(resp->data, 0, to_print, resp->data_len)) {
                ZEROCONF_PRINTF ("\t%s", to_print);
            }

            zeroconf_free(to_print);
            to_print = NULL;
            break;
        case ZEROCONF_MDNS_RR_TYPE_PTR:
            ZEROCONF_PRINTF ("\tPTR");

            to_print = zeroconf_calloc(resp->data_len, 1);
            if (to_print == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory for ptr\n");
                break;
            }

            if (unencode_mdns_name_string(resp->data, 0, to_print, resp->data_len)) {
                ZEROCONF_PRINTF ("\t%s", to_print);
            }

            zeroconf_free(to_print);
            to_print = NULL;
            break;
        case ZEROCONF_MDNS_RR_TYPE_TXT:
            ZEROCONF_PRINTF ("\tTXT\n");

            unsigned char *ptr = resp->data;

            while (ptr - resp->data < (int)resp->data_len) {
                unsigned char sTxtLen = *ptr++; //length
                ZEROCONF_PRINTF("\tTXT(%d)\t", sTxtLen);

                for (i = 0 ; i < sTxtLen ; i++) {
                    ZEROCONF_PRINTF("%c", ptr[i]);
                }

                ZEROCONF_PRINTF("\n");
                ptr += (i);
            }
            break;
        case ZEROCONF_MDNS_RR_TYPE_NSEC:
            ZEROCONF_PRINTF ("\tNSEC");

            to_print = zeroconf_calloc(resp->data_len, 1);
            if (to_print == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory for nsec\n");
                break;
            }

            if (unencode_mdns_name_string(resp->data, 0, to_print, resp->data_len)) {
                ZEROCONF_PRINTF("\t");

                for (i = 0; i < resp->data_len ; i++) {
                    ZEROCONF_PRINTF(" %x ", resp->data[i]);
                }
            }

            zeroconf_free(to_print);
            to_print = NULL;
            break;
        case ZEROCONF_MDNS_RR_TYPE_SRV:
            ZEROCONF_PRINTF ("\tSRV");

            to_print = zeroconf_calloc(resp->data_len, 1);
            if (to_print == NULL) {
                ZEROCONF_DEBUG_ERR("failed to allocate memory for srv\n");
                break;
            }

            port = (resp->data[4] << 8) | resp->data[5];

            ZEROCONF_PRINTF("\tPort=%d\n", port);

            if (unencode_mdns_name_string(resp->data, 6, to_print, resp->data_len)) {
                ZEROCONF_PRINTF("\tTarget=%s", to_print);
            }

            zeroconf_free(to_print);
            to_print = NULL;
            break;
        default:
            ZEROCONF_PRINTF ("\tTYPE_CODE %d\n", resp->type);
            ZEROCONF_PRINTF ("\tData Len - %d\n", resp->len);

            if (!print_raw) {
                ZEROCONF_PRINTF ("\tData -\n");

                for (i = 0 ; i < (unsigned int)resp->len ; i++) {
                    ZEROCONF_PRINTF("0x%02x ", resp->data[i]);

                    if ((i + 1) % 16 == 0) {
                        ZEROCONF_PRINTF("\n");
                    }
                }
            }
            break;
    }

    if (print_raw) {
        ZEROCONF_PRINTF ("\n\tData Bytes -\n");

        for (i = 0 ; i < resp->data_len ; i++) {
            ZEROCONF_PRINTF("0x%02x ", resp->data[i]);

            if ((i + 1) % 16 == 0) {
                ZEROCONF_PRINTF("\n");
            }
        }
    }

    if (to_print != NULL) {
        zeroconf_free(to_print);
    }

    ZEROCONF_PRINTF ("\n\n");

    return;
}

#if defined (__SUPPORT_DNS_SD__)
zeroconf_local_dns_sd_info_t *createLocalDnsSd(char *name, char *type,
                                               char *domain, int port,
                                               size_t txtLen, unsigned char *txtRecord)
{
    zeroconf_local_dns_sd_info_t *temp;

    temp = zeroconf_calloc(1, sizeof(zeroconf_local_dns_sd_info_t));
    if (temp == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for dns-sd\n");
        return NULL;
    }

    temp->name = (char *)zeroconf_calloc(strlen(name) + 1, 1);
    if (temp->name == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for name\n");
        goto error;
    }

    strcpy(temp->name, name);

    temp->type = (char *)zeroconf_calloc(strlen(type) + 1, 1);
    if (temp->type == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for type\n");
        goto error;
    }

    strcpy(temp->type, type);

    temp->domain = (char *)zeroconf_calloc(strlen(domain) + 1, 1);
    if (temp->domain == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for domain\n");
        goto error;
    }

    strcpy(temp->domain, domain);

    temp->txtLen = txtLen;

    if (txtLen) {
        temp->txtRecord = zeroconf_calloc(txtLen, 1);
        if (temp->txtRecord == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for txt\n");
            goto error;
        }

        memcpy(temp->txtRecord, txtRecord, txtLen);
    } else {
        temp->txtRecord = NULL;
    }

    temp->port = port;

    return temp;

error:

    deleteLocalDnsSd(temp);

    return NULL;
}

void deleteLocalDnsSd(zeroconf_local_dns_sd_info_t *local_dns_sd)
{
    if (local_dns_sd) {
        if (local_dns_sd->name) {
            zeroconf_free(local_dns_sd->name);
            local_dns_sd->name = NULL;
        }

        if (local_dns_sd->type) {
            zeroconf_free(local_dns_sd->type);
            local_dns_sd->type = NULL;
        }

        if (local_dns_sd->domain) {
            zeroconf_free(local_dns_sd->domain);
            local_dns_sd->domain = NULL;
        }

        if (local_dns_sd->txtLen) {
            zeroconf_free(local_dns_sd->txtRecord);
            local_dns_sd->txtRecord = NULL;
            local_dns_sd->txtLen = 0;
        }

        zeroconf_free(local_dns_sd);
        local_dns_sd = NULL;
    }

    return;
}

zeroconf_proxy_dns_sd_info_t *createProxyDnsSd(char *h_name, unsigned char *h_ip,
                                               char *name, char *type,
                                               char *domain, int port,
                                               size_t txtLen, unsigned char *txtRecord)
{
    zeroconf_proxy_dns_sd_info_t *temp = NULL;

    temp = zeroconf_calloc(1, sizeof(zeroconf_proxy_dns_sd_info_t));
    if (temp == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for proxy dns-sd\n");
        return NULL;
    }

    temp->h_name = (char *)zeroconf_calloc(strlen(h_name) + 1, 1);
    if (temp->h_name == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for hname\n");
        goto error;
    }

    strcpy(temp->h_name, h_name);

    temp->name = (char *)zeroconf_calloc(strlen(name) + 1, 1);
    if (temp->name == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for name\n");
        goto error;
    }

    strcpy(temp->name, name);

    temp->type = (char *)zeroconf_calloc(strlen(type) + 1, 1);
    if (temp->type == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for type\n");
        goto error;
    }

    strcpy(temp->type, type);

    temp->domain = (char *)zeroconf_calloc(strlen(domain) + 1, 1);
    if (temp->domain == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for domain\n");
        goto error;
    }

    strcpy(temp->domain, domain);

    temp->txtLen = txtLen;

    if (temp->txtLen) {
        temp->txtRecord = zeroconf_calloc(txtLen, 1);
        if (temp->txtRecord == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for txt\n");
            goto error;
        }

        memcpy(temp->txtRecord, txtRecord, txtLen);
    } else {
        temp->txtRecord = NULL;
    }

    temp->port = port;

    temp->h_ip[0] = h_ip[0];
    temp->h_ip[1] = h_ip[1];
    temp->h_ip[2] = h_ip[2];
    temp->h_ip[3] = h_ip[3];

    return temp;

error:

    deleteProxyDnsSd(temp);

    return NULL;

}

void deleteProxyDnsSd(zeroconf_proxy_dns_sd_info_t *proxy_dns_sd)
{
    if (proxy_dns_sd) {
        if (proxy_dns_sd->h_name) {
            zeroconf_free(proxy_dns_sd->h_name);
            proxy_dns_sd->h_name = NULL;
        }

        if (proxy_dns_sd->name) {
            zeroconf_free(proxy_dns_sd->name);
            proxy_dns_sd->name = NULL;
        }

        if (proxy_dns_sd->type) {
            zeroconf_free(proxy_dns_sd->type);
            proxy_dns_sd->type = NULL;
        }

        if (proxy_dns_sd->domain) {
            zeroconf_free(proxy_dns_sd->domain);
            proxy_dns_sd->domain = NULL;
        }

        if (proxy_dns_sd->txtLen) {
            zeroconf_free(proxy_dns_sd->txtRecord);
            proxy_dns_sd->txtRecord = NULL;
            proxy_dns_sd->txtLen = 0;
        }

        zeroconf_free(proxy_dns_sd);
        proxy_dns_sd = NULL;
    }

    return;
}

void printLocalDnsSd(void)
{
    if (g_zeroconf_local_dns_sd_service) {
        ZEROCONF_PRINTF("Local Service Details >> \n");
        ZEROCONF_PRINTF("\tName\t: %s,\n", g_zeroconf_local_dns_sd_service->name);
        ZEROCONF_PRINTF("\tType\t: %s,\n", g_zeroconf_local_dns_sd_service->type);
        ZEROCONF_PRINTF("\tDomain\t: %s,\n", g_zeroconf_local_dns_sd_service->domain);
        ZEROCONF_PRINTF("\tPort\t: %d,\n", g_zeroconf_local_dns_sd_service->port);

        if (g_zeroconf_local_dns_sd_service->txtLen) {
            int index = 0;
            unsigned char *ptr = g_zeroconf_local_dns_sd_service->txtRecord;

            while (ptr - g_zeroconf_local_dns_sd_service->txtRecord <
                   (int)g_zeroconf_local_dns_sd_service->txtLen) {
                unsigned char sTxtLen = *ptr++; //length
                ZEROCONF_PRINTF("\tTxt(%d)\t: ", sTxtLen);

                for (index = 0 ; index < sTxtLen ; index++) {
                    ZEROCONF_PRINTF("%c", ptr[index]); //record
                }

                ZEROCONF_PRINTF("\n");

                ptr += (index);
            }
        }
    }
}

void printProxyDnsSd(void)
{
    if (g_zeroconf_proxy_dns_sd_svc) {
        ZEROCONF_PRINTF("Proxy Service Details >> \n");
        ZEROCONF_PRINTF("Host Name : %s,\n", g_zeroconf_proxy_dns_sd_svc->h_name);
        ZEROCONF_PRINTF("Host IP : %d.%d.%d.%d,\n",
                        g_zeroconf_proxy_dns_sd_svc->h_ip[0],
                        g_zeroconf_proxy_dns_sd_svc->h_ip[1],
                        g_zeroconf_proxy_dns_sd_svc->h_ip[2],
                        g_zeroconf_proxy_dns_sd_svc->h_ip[3]);
        ZEROCONF_PRINTF("Service Name : %s,\n", g_zeroconf_proxy_dns_sd_svc->name);
        ZEROCONF_PRINTF("Service Type : %s,\n", g_zeroconf_proxy_dns_sd_svc->type);
        ZEROCONF_PRINTF("Domain : %s,\n", g_zeroconf_proxy_dns_sd_svc->domain);
        ZEROCONF_PRINTF("Port : %d,\n", g_zeroconf_proxy_dns_sd_svc->port);

        if (g_zeroconf_proxy_dns_sd_svc->txtLen) {
            int index = 0;
            unsigned char *ptr = g_zeroconf_proxy_dns_sd_svc->txtRecord;

            while (ptr - g_zeroconf_proxy_dns_sd_svc->txtRecord <
                   (int) g_zeroconf_proxy_dns_sd_svc->txtLen) {
                unsigned char sTxtLen = *ptr++; //length
                ZEROCONF_PRINTF("\tTxt(%d)\t: ", sTxtLen);

                for (index = 0 ; index < sTxtLen ; index++) {
                    ZEROCONF_PRINTF("%c", ptr[index]); //record
                }

                ZEROCONF_PRINTF("\n");

                ptr += (index);
            }
        }
    }
}
#endif    /* __SUPPORT_DNS_SD__ */

zeroconf_mdns_response_t *createMdnsResp(unsigned char *query, size_t query_len,
                                         int type, LONG ttl, unsigned long timestamp,
                                         size_t data_len, unsigned char *data)
{
    int flag = 0, ch_res = -1;
    zeroconf_mdns_response_t *temp = NULL;
    zeroconf_mdns_response_t *srch = NULL;

    if (ttl == 0) {
        if (type == ZEROCONF_MDNS_RR_TYPE_PTR) {
            deleteMdnsPtrRec(data, data_len);
            deleteMdnsResp(data, data_len, 0, ZEROCONF_MDNS_RR_TYPE_ALL);
        } else {
            deleteMdnsResp(query, query_len, 1, ZEROCONF_MDNS_RR_TYPE_ALL);
        }

        return NULL;
    }

    if (g_zeroconf_mdns_response_root != NULL) {
        srch = g_zeroconf_mdns_response_root;

        do {
            ch_res = compare_domain_bytes(query, query_len, srch->query, srch->query_len);
            if (ch_res == 1  && srch->type == type && srch->len == (int)data_len) {
                ch_res = compare_data_bytes(data, srch->data, data_len);
                if (ch_res == 1) {
                    srch->timestamp = timestamp;
                    srch->ttl = ttl;
                    flag = 1;
                    break;
                }
            }

            srch = srch->next;
        } while (srch != NULL);
    }

    if (flag == 0) {
        temp = zeroconf_calloc(1, sizeof(zeroconf_mdns_response_t));
        if (temp == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for response\n");
            return NULL;
        }

        temp->query = zeroconf_calloc(query_len, 1);
        if (temp->query == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for query\n");
            zeroconf_free(temp);
            return NULL;
        }

        temp->data = zeroconf_calloc(data_len, 1);
        if (temp->data == NULL) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory for data\n");
            zeroconf_free(temp->query);
            zeroconf_free(temp);
            return NULL;
        }

        g_zeroconf_mdns_header.currrent_resp_id_ptr++;

        memcpy(temp->query, query, query_len);
        temp->query_len = query_len;

        memcpy(temp->data, data, data_len);
        temp->data_len = data_len;

        temp->id = g_zeroconf_mdns_header.currrent_resp_id_ptr;
        temp->type = type;
        temp->ttl = ttl;
        temp->timestamp = timestamp;

        temp->next = NULL;

#if defined (__SUPPORT_DNS_SD__)
        print_dns_sd_cache(temp, 1);
#endif // (__SUPPORT_DNS_SD__)

        return temp;
    }

    return NULL;
}

void viewMdnsResponses(void)
{
    zeroconf_mdns_response_t *temp = NULL;

    if (g_zeroconf_mdns_response_root == NULL) {
        ZEROCONF_DEBUG_INFO("Nothing to display\n");
    } else {
        temp = g_zeroconf_mdns_response_root;

        while (temp->next != NULL) {
            print_mdns_response(temp, 1, 1);
            temp = temp->next;
        }

        print_mdns_response(temp, 1, 1);
    }
}

void insertMdnsResp(unsigned char *query, size_t query_len,
                    int type, LONG ttl, unsigned long timestamp,
                    int data_len, unsigned char *data, int check)
{
    int flag = 0, ch_res = -1;
    zeroconf_query_string_t *srch = NULL;
    zeroconf_mdns_response_t *n = NULL;
    zeroconf_mdns_response_t *temp = NULL;

    if (ttl == 0) {
        check = 0;
    }

    if (check) {
        if (g_zeroconf_query_string_root != NULL) {
            srch = g_zeroconf_query_string_root;

            do {
                ch_res = search_in_byte_array(query, query_len,
                                              srch->query, srch->query_len);
                if (ch_res) {
                    flag = 1;
                    break;
                }

                srch = srch->next;
            } while (srch != NULL);
        }
    } else {
        flag = 1;
    }

    if (flag) {
        //removed duplicated query.
        deleteMdnsResp(query, query_len, 1, type);

        n = createMdnsResp(query,
                           query_len,
                           type,
                           ttl,
                           timestamp,
                           data_len,
                           data);
        if (n != NULL) {
            if (g_zeroconf_mdns_response_root == NULL) {
                g_zeroconf_mdns_response_root = n;
            } else {
                temp = g_zeroconf_mdns_response_root;

                while (temp->next != NULL) {
                    temp = temp->next;
                }

                temp->next = n;
            }

            g_zeroconf_mdns_header.resp_count++;
        }
    }
}

void deleteMdnsPtrRec(unsigned char *query, size_t query_len)
{
    zeroconf_mdns_response_t *cur = g_zeroconf_mdns_response_root;
    zeroconf_mdns_response_t *prev = NULL;
    zeroconf_mdns_response_t *del = NULL;

    while (cur) {
        if (compare_domain_bytes(cur->data, cur->data_len, query, query_len)) {
#if defined (__SUPPORT_DNS_SD__)
            print_dns_sd_cache(cur, 0);
#endif // __SUPPORT_DNS_SD__

            if (cur == g_zeroconf_mdns_response_root) {
                g_zeroconf_mdns_response_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            free_mdns_resp(del);

            g_zeroconf_mdns_header.resp_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

// searchMode 0 : compares bytes to byte,searchMode > 1 : searches key in source
void deleteMdnsResp(unsigned char *query, size_t query_len, int searchMode, int type)
{
    int flag = 0;

    zeroconf_mdns_response_t *cur = g_zeroconf_mdns_response_root;
    zeroconf_mdns_response_t *prev = NULL;
    zeroconf_mdns_response_t *del = NULL;

    while (cur) {
        flag = 0;

        if (searchMode == 0) {
            flag = compare_domain_bytes(cur->query, cur->query_len, query, query_len);
        } else {
            flag = search_in_byte_array(query, query_len, cur->query, cur->query_len);
        }

        if (flag && type != ZEROCONF_MDNS_RR_TYPE_ALL) {
            flag = (cur->type == type ? 1 : 0);
        }

        if (flag) {
#if defined (__SUPPORT_DNS_SD__)
            print_dns_sd_cache(cur, 0);
#endif // (__SUPPORT_DNS_SD__)

            if (cur == g_zeroconf_mdns_response_root) {
                g_zeroconf_mdns_response_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            free_mdns_resp(del);

            g_zeroconf_mdns_header.resp_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void clear_mdns_cache(void)
{
    zeroconf_mdns_response_t *temp = NULL;

    while (g_zeroconf_mdns_response_root != NULL) {
        ZEROCONF_DEBUG_INFO("clear mdns cache\n");

        temp = g_zeroconf_mdns_response_root;
        g_zeroconf_mdns_response_root = g_zeroconf_mdns_response_root->next;
        g_zeroconf_mdns_header.resp_count--;
        free_mdns_resp(temp);
    }

    reset_lists();
}

void clear_mdns_invalid_ttls(void)
{
    zeroconf_mdns_response_t *cur = g_zeroconf_mdns_response_root;
    zeroconf_mdns_response_t *prev = NULL;
    zeroconf_mdns_response_t *del = NULL;

    while (cur) {
        if (check_ttl_expiration(cur->timestamp, cur->ttl)) {
#if defined (__SUPPORT_DNS_SD__)
            print_dns_sd_cache(cur, 0);
#endif // (__SUPPORT_DNS_SD__)

            if (cur == g_zeroconf_mdns_response_root) {
                g_zeroconf_mdns_response_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            free_mdns_resp(del);

            g_zeroconf_mdns_header.resp_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

int check_ttl_expiration(unsigned long packet_time, int ttl_value)
{
    unsigned int time_cr = xTaskGetTickCount();

    if (ttl_value <= -1) {
        return pdFALSE;
    }

    if ((int)(time_cr - packet_time) >= ttl_value * 100) {
        ZEROCONF_DEBUG_TEMP("expired(time_cr:%ld, packet_time:%ld, ttl_value:%ld)\n",
                            time_cr, packet_time, ttl_value);
        return pdTRUE;
    }

    return pdFALSE;
}

void free_mdns_resp(zeroconf_mdns_response_t *tofree)
{
    tofree->next = NULL;

    zeroconf_free(tofree->query);
    zeroconf_free(tofree->data);
    zeroconf_free(tofree);
}

void insertPckSend(unsigned char *full_packet, int len, int max_count, int interval,
                   int last_sent_time, int mult_fact, int type,
                   zeroconf_peer_info_t ipv4_addr, zeroconf_peer_info_t ipv6_addr,
                   int packet_type)
{
    zeroconf_pck_send_t *temp = NULL;
    zeroconf_pck_send_t *new_packet = NULL;

    new_packet = zeroconf_calloc(1, sizeof(zeroconf_pck_send_t));
    if (new_packet == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for sending buffer\n");
        return;
    }

    new_packet->full_packet = zeroconf_calloc(len, 1);
    if (new_packet->full_packet == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for sending buffer\n");
        zeroconf_free(new_packet);
        return;
    }

    g_zeroconf_mdns_header.currrent_send_id_ptr++;
    new_packet->id = g_zeroconf_mdns_header.currrent_send_id_ptr;
    memcpy(new_packet->full_packet, full_packet, len);
    new_packet->len = len;
    new_packet->last_sent_time = last_sent_time;
    new_packet->sent_count = 0;
    new_packet->max_count = max_count;
    new_packet->interval = interval;
    new_packet->mult_factor = mult_fact;
    new_packet->type = type;
    memcpy(&new_packet->peer_info[0], &ipv4_addr, sizeof(zeroconf_peer_info_t));
    memcpy(&new_packet->peer_info[1], &ipv6_addr, sizeof(zeroconf_peer_info_t));
    new_packet->packet_type = packet_type;
    new_packet->next = NULL;

    if (new_packet != NULL) {
        if (g_zeroconf_pck_send_root == NULL) {
            g_zeroconf_pck_send_root = new_packet;
        } else {
            temp = g_zeroconf_pck_send_root;

            while (temp->next != NULL) {
                temp = temp->next;
            }

            temp->next = new_packet;
        }

        g_zeroconf_mdns_header.sent_count++;

#ifdef ZEROCONF_ENABLE_DEBUG_INFO
        struct sockaddr_in *tmp_ipv4_addr = (struct sockaddr_in *) &
                                            (new_packet->peer_info[0].ip_addr);
        ZEROCONF_DEBUG_INFO("Dest(%d.%d.%d.%d:%d)\n",
                            (ntohl(tmp_ipv4_addr->sin_addr.s_addr) >> 24) & 0x0ff,
                            (ntohl(tmp_ipv4_addr->sin_addr.s_addr) >> 16) & 0x0ff,
                            (ntohl(tmp_ipv4_addr->sin_addr.s_addr) >>  8) & 0x0ff,
                            (ntohl(tmp_ipv4_addr->sin_addr.s_addr)      ) & 0x0ff,
                            ntohs(tmp_ipv4_addr->sin_port));

        ZEROCONF_DEBUG_TEMP("Set bits(ZEROCONF_EVT_SEND)\n");
#endif // ZEROCONF_ENABLE_DEBUG_INFO

        if (g_zeroconf_sender_event_handler) {
            xEventGroupSetBits(g_zeroconf_sender_event_handler, ZEROCONF_EVT_SEND);
        } else {
            ZEROCONF_DEBUG_ERR("Event handler is NULL to send packet\n");
        }
    }

    return;
}

void deletePckSend(int id)
{
    zeroconf_pck_send_t *cur = g_zeroconf_pck_send_root;
    zeroconf_pck_send_t *prev = NULL;
    zeroconf_pck_send_t *del = NULL;

    while (cur) {
        if (cur->id == id) {
            if (cur == g_zeroconf_pck_send_root) {
                g_zeroconf_pck_send_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            zeroconf_free(del->full_packet);
            zeroconf_free(del);

            g_zeroconf_mdns_header.sent_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void viewPckSends(void)
{
    char time_str[100] = {0x00,};
#if defined (__TIME64__)
    __time64_t time_val;
#else
    time_t time_val;
#endif // (__TIME64__)

    zeroconf_pck_send_t *temp = NULL;

    if (g_zeroconf_pck_send_root == NULL) {
        ZEROCONF_DEBUG_INFO("Nothing to display\n\n");
    } else {
        temp = g_zeroconf_pck_send_root;

        while (temp->next != NULL) {
            time_val = (int) temp->last_sent_time;

#if defined (__TIME64__)
            da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime64(&time_val));
#else
            da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime32(&time_val));
#endif // (__TIME64__)

            ZEROCONF_PRINTF("Type %d. Last sent on %s. %d times sent, "
                            "maximum %d time will be sent. Current Interval is %d.\n",
                            temp->type, time_str, temp->sent_count,
                            temp->max_count, temp->interval);
            temp = temp->next;
        }

        time_val = (int) temp->last_sent_time;

#if defined (__TIME64__)
        da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime64 (&time_val));
#else
        da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime32 (&time_val));
#endif // (__TIME64__)

        ZEROCONF_PRINTF ("Type %d. Last sent on %s. %d times sent, "
                         "maximum %d time will be sent. Current Interval is %d.\n",
                         temp->type, time_str, temp->sent_count,
                         temp->max_count, temp->interval);
    }
}

zeroconf_query_string_t *createQueryString(unsigned char *query, size_t query_len)
{
    zeroconf_query_string_t *temp = NULL;

    temp = zeroconf_calloc(1, sizeof(zeroconf_query_string_t));
    if (temp == NULL) {
        return NULL;
    }

    temp->query = zeroconf_calloc(1, query_len);
    if (temp->query == NULL) {
        zeroconf_free(temp);
        return NULL;
    }

    memcpy(temp->query, query, query_len);

    temp->next = NULL;

    return temp;
}

void deleteQueryString(unsigned char *query, size_t query_len)
{
    zeroconf_query_string_t *cur = g_zeroconf_query_string_root;
    zeroconf_query_string_t *prev = NULL;
    zeroconf_query_string_t *del = NULL;

    while (cur) {
        if (compare_domain_bytes(cur->query, cur->query_len, query, query_len)) {
            if (cur == g_zeroconf_query_string_root) {
                g_zeroconf_query_string_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            zeroconf_free(del->query);
            zeroconf_free(del);

            g_zeroconf_mdns_header.query_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void insertQueryString(unsigned char *query, size_t query_len)
{
    zeroconf_query_string_t *n = createQueryString(query, query_len);

    if (n != NULL) {
        if (g_zeroconf_query_string_root == NULL) {
            g_zeroconf_query_string_root = n;
        } else {
            zeroconf_query_string_t *temp = NULL;
            temp = g_zeroconf_query_string_root;

            while (temp->next != NULL) {
                temp = temp->next;
            }

            temp->next = n;
        }

        g_zeroconf_mdns_header.query_count++;
    }
}

void viewQueryStrings(void)
{
    zeroconf_query_string_t *temp;

    if (g_zeroconf_query_string_root == NULL) {
        ZEROCONF_DEBUG_INFO("Nothing to display\n");
    } else {
        temp = g_zeroconf_query_string_root;

        while (temp->next != NULL) {
            ZEROCONF_PRINTF("%s\n", temp->query);
            temp = temp->next;
        }

        ZEROCONF_PRINTF("%s\n", temp->query);
    }
}

void viewStructHeader(void)
{
    ZEROCONF_PRINTF("Query Buffer - %d items, ",
                    g_zeroconf_mdns_header.query_count);
    ZEROCONF_PRINTF(" Response Cache - %d items, ",
                    g_zeroconf_mdns_header.resp_count);
    ZEROCONF_PRINTF(" Send Buffer - %d items\n",
                    g_zeroconf_mdns_header.sent_count);
}

void display_mdns_usage(void)
{
    ZEROCONF_PRINTF("mdns-start  Command Format :\n"
                    "\tmdns-start -i [wlan0|wlan1] [host-name] "
                    "(host-name should not contain .local)\n");
}

#if defined (__SUPPORT_XMDNS__)
void display_xmdns_usage(void)
{
    ZEROCONF_PRINTF("xmdns-start  Command Format :\n"
                    "\txmdns-start [host-name] "
                    "(host-name should not contain .site) \n");
}
#endif // (__SUPPORT_XMDNS__)

void display_mdns_config_usage(void)
{
    ZEROCONF_PRINTF("mdns-config Command Format :\n");
    ZEROCONF_PRINTF("\tmdns-config -hostname [host-name] "
                    "(host-name should not contain .local)\n");
    ZEROCONF_PRINTF("\tmdns-config -auto-enable [yes|no]\n");
    ZEROCONF_PRINTF("\tmdns-config -status\n");
    ZEROCONF_PRINTF("\tmdns-config -help\n");
}

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
ip_addr_t get_mdns_ipv6_multicast(void)
{
    return IPADDR6_INIT_HOST(0xFF020000, 0, 0, 0xFB);
}

#if defined (__SUPPORT_XMDNS__)
ip_addr_t get_xmdns_ipv6_multicast(void)
{
    return IPADDR6_INIT_HOST(0xFF050000, 0, 0, 0xFB);
}
#endif // (__SUPPORT_XMDNS__)
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

#if defined (__SUPPORT_DNS_SD__)
int zeroconf_exec_dns_sd_cmd(int argc, char *argv[])
{
    char *cmd_key;
    unsigned char buffer[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t buffer_len = 0;
    int status = 0;

    if (argc >= 2) {
        cmd_key = argv[1];

        if (strcmp(cmd_key, "-help") == 0) {
            print_dns_sd_command_help();
            return DA_APP_INVALID_PARAMETERS;
        }

        if (!zeroconf_is_running_mdns_service()) {
            ZEROCONF_DEBUG_ERR("DNS-SD cannot work while mDNS is not running!\n");
            return DA_APP_NOT_ENABLED;
        }

        if (is_running_mdns_probing_processor()) {
            ZEROCONF_DEBUG_ERR("DNS-SD cannot work while zeroconf is probing!\n");
            return DA_APP_IN_PROGRESS;
        }

        set_dns_sd_service_status(ZEROCONF_ACTIVATED);

        if (strcmp(cmd_key, "-E") == 0) {
            char *query = "r._dns-sd._udp.local";

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(query, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'E';

            insertQueryString(buffer, buffer_len);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_E, NULL);

            wait_for_close();

            deleteQueryString(buffer, buffer_len);

            zeroconf_stop_exec_task();
        } else if (strcmp(cmd_key, "-F") == 0) {
            char *query = "b._dns-sd._udp.local";

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(query, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'F';

            insertQueryString(buffer, buffer_len);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_F, NULL);

            wait_for_close();

            deleteQueryString(buffer, buffer_len);

            zeroconf_stop_exec_task();
        } else if (strcmp(cmd_key, "-R") == 0 && argc > 6) {
            status = zeroconf_register_dns_sd_service(argv[2],            //name
                                                      argv[3],            //type
                                                      argv[4],            //domain
                                                      atoi(argv[5]),    //port
                                                      argc - 6,            //txt_len
                                                      &argv[6],            //txt
                                                      pdFALSE);
            if (status == DA_APP_SUCCESS) {
                ZEROCONF_PRINTF("Advertising DNS-SD Local Service. Press 'x' to stop..\n");

                wait_for_close();
                zeroconf_unregister_dns_sd_service();
            }
        } else if (strcmp(cmd_key, "-P") == 0 && argc > 8) {
            ZEROCONF_PRINTF ("Advertising DNS-SD Proxy Service. Press 'x' to stop..\n");

            if (execute_dns_sd_P(argc, argv)) {
                wait_for_close();
                stop_dns_sd_proxy_service();
            } else {
                ZEROCONF_DEBUG_ERR("Error in Command!\n");
            }
        } else if (strcmp(cmd_key, "-B") == 0 && argc == 4) {
            to_lower(argv[2]);
            char *type = (argv[2]);
            to_lower(argv[3]);
            char *domain = (argv[3]);
            char *query = NULL;

            if ((strlen(type) == 0) || (strlen(domain) == 0)) {
                ZEROCONF_DEBUG_ERR("Type and Domain cannot be blank\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if ((strcmp(type + strlen(type) - 5, "._tcp") != 0)
                && (strcmp(type + strlen(type) - 5, "._udp") != 0)) {
                ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(domain, ".") == 0) {
                domain = "local";
            } else if (strcmp(domain, "local") != 0) {
                ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            query = zeroconf_calloc(strlen(type) + 1 + strlen(domain) + 1, 1);
            if (query == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to alloc memory for query\n");
                return DA_APP_MALLOC_ERROR;
            }

            strcpy(query, type);
            strcat(query, ".");
            strcat(query, domain);

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(query, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'B';

            insertQueryString(buffer, buffer_len);

            ZEROCONF_PRINTF("Looking for %s services. Press 'x' to stop..\n", query);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_B, NULL);

            wait_for_close();

            zeroconf_free(query);

            zeroconf_stop_exec_task();

            deleteQueryString(buffer, buffer_len);
        } else if (strcmp(cmd_key, "-L") == 0 && argc == 5) {
            to_lower(argv[2]);
            char *name = (argv[2]);
            to_lower(argv[3]);
            char *type = (argv[3]);
            to_lower(argv[4]);
            char *domain = (argv[4]);
            char *query = NULL;

            if ((strlen(type) == 0) || (strlen(name) == 0) || (strlen(domain) == 0)) {
                ZEROCONF_DEBUG_ERR("Name, Type and Domain can not be blank\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if ((strcmp(type + strlen(type) - 5, "._tcp") != 0)
                && (strcmp(type + strlen(type) - 5, "._udp") != 0)) {
                ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(domain, ".") == 0) {
                domain = "local";
            } else if (strcmp(domain, "local") != 0) {
                ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            query = zeroconf_calloc(strlen(name) + 1 + strlen(type) + 1 + strlen(
                                        domain) + 1, 1);
            if (query == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to alloc memory for query\n");
                return DA_APP_MALLOC_ERROR;
            }

            strcpy(query, name);
            strcat(query, ".");
            strcat(query, type);
            strcat(query, ".");
            strcat(query, domain);

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(query, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'L';

            insertQueryString(buffer, buffer_len);

            ZEROCONF_PRINTF("Looking for %s services. Press 'x' to stop..\n", query);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_L, NULL);

            wait_for_close();

            zeroconf_free(query);

            query = NULL;

            zeroconf_stop_exec_task();

            deleteQueryString(buffer, buffer_len);
        } else if (strcmp(cmd_key, "-Q") == 0 && argc == 5) {
            to_lower(argv[2]);
            char *fqdn = (argv[2]);
            to_lower(argv[3]);
            char *type = (argv[3]);
            to_lower(argv[4]);
            char *rr_class_str = (argv[4]);

            if (strcmp(rr_class_str, ".") == 0) {
                rr_class_str = "1";
            }

            int rr_class = atoi(rr_class_str);

            unsigned long type_code = get_type(type);
            if (type_code == 0) {
                ZEROCONF_DEBUG_ERR("Type not supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (rr_class != 1) {
                ZEROCONF_DEBUG_ERR("RR class not supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strlen(fqdn) == 0 || strlen(type) == 0 ) {
                ZEROCONF_DEBUG_ERR("FQDN and Type can not be blank\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(fqdn + strlen(fqdn) - 6, ".local") != 0) {
                ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(fqdn, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'Q';

            g_zeroconf_dns_sd_query.q_type = type_code;

            insertQueryString(buffer, buffer_len);

            ZEROCONF_PRINTF("Looking for %s records of %s services. "
                            "Press 'x' to stop..\n", type, fqdn);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_Q, (void *)type_code);

            wait_for_close();

            zeroconf_stop_exec_task();

            deleteQueryString(buffer, buffer_len);
        } else if (strcmp(cmd_key, "-Z") == 0 && argc == 4) {
            to_lower(argv[2]);
            char *type = (argv[2]);
            to_lower(argv[3]);
            char *domain = (argv[3]);
            char *query = NULL;

            if ((strlen(type) == 0) || (strlen(domain) == 0)) {
                ZEROCONF_DEBUG_ERR("Type and Domain can not be blank\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(type + strlen(type) - 5, "._tcp") != 0
                && strcmp(type + strlen(type) - 5, "._udp") != 0) {
                ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(domain, ".") == 0) {
                domain = "local";
            } else if (strcmp(domain, "local") != 0) {
                ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            query = zeroconf_calloc(strlen(type) + 1 + strlen(domain) + 1, 1);
            if (query == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to alloc memory for query\n");
                return DA_APP_MALLOC_ERROR;
            }

            strcpy(query, type);
            strcat(query, ".");
            strcat(query, domain);

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(query, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'Z';

            insertQueryString(buffer, buffer_len);

            ZEROCONF_PRINTF("Looking for all records of %s services. "
                            "Press 'x' to stop..\n", query);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_Z, NULL);

            wait_for_close();

            zeroconf_free(query);

            zeroconf_stop_exec_task();

            deleteQueryString(buffer, buffer_len);
        } else if (strcmp(cmd_key, "-G") == 0 && argc == 4) {
            char *fqdn = (argv[3]);
            to_lower(argv[2]);
            char *type = (argv[2]);
            unsigned long type_code = 0;

            if ((strlen(fqdn) == 0) || (strlen(type) == 0)) {
                ZEROCONF_DEBUG_ERR("FQDN and Type can not be blank\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(fqdn + strlen(fqdn) - 6, ".local") != 0) {
                ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            if (strcmp(type, "v4") == 0) {
                type_code = ZEROCONF_MDNS_RR_TYPE_A;
            } else if (strcmp(type, "v6") == 0) {
                type_code = ZEROCONF_MDNS_RR_TYPE_AAAA;
            } else if (strcmp(type, "v4v6") == 0) {
                type_code = ZEROCONF_MDNS_RR_TYPE_ALL;
            } else {
                ZEROCONF_DEBUG_ERR("Invalid Type\n");
                return DA_APP_INVALID_PARAMETERS;
            }

            memset(&g_zeroconf_dns_sd_query, 0x00, sizeof(zeroconf_dns_sd_query_t));

            buffer_len = encode_mdns_name_string(fqdn, buffer, sizeof(buffer));

            memcpy(g_zeroconf_dns_sd_query.query_bytes, buffer, buffer_len);

            g_zeroconf_dns_sd_query.query_bytes_len = buffer_len;

            g_zeroconf_dns_sd_query.query_key = 'G';

            g_zeroconf_dns_sd_query.g_type = type_code;

            insertQueryString(buffer, buffer_len);

            ZEROCONF_PRINTF("Looking for IP%s records of %s. "
                            "Press 'x' to stop..\n", type, fqdn);

            zeroconf_start_exec_task(ZEROCONF_DNS_SD_EXEC_THREAD_NAME,
                                     execute_dns_sd_G, (void *)type_code);

            wait_for_close();

            zeroconf_stop_exec_task();

            deleteQueryString(buffer, buffer_len);
        } else if (strcmp(cmd_key, "-UR") == 0) {
            status = zeroconf_unregister_dns_sd_service();
            if (status == 0) {
                ZEROCONF_PRINTF("Unregister dns-sd service\n");
            }
        } else {
            // print error if no return executed
            print_dns_sd_command_error();
        }
    } else {
        print_dns_sd_command_error();
    }

    set_dns_sd_service_status(ZEROCONF_INIT);

    return DA_APP_SUCCESS;
}

void insertLocalDns(void)
{
    if (g_zeroconf_local_dns_sd_service) {
        insertMdnsResp(g_zeroconf_local_dns_sd_service->t_bytes,
                       g_zeroconf_local_dns_sd_service->t_bytes_len,
                       12,
                       -2,
                       xTaskGetTickCount(),
                       g_zeroconf_local_dns_sd_service->q_bytes_len,
                       g_zeroconf_local_dns_sd_service->q_bytes,
                       0);
    }
}

void deleteLocalDns(void)
{
    zeroconf_mdns_response_t *cur = g_zeroconf_mdns_response_root;
    zeroconf_mdns_response_t *prev = NULL;
    zeroconf_mdns_response_t *del = NULL;

    while (cur) {
        if (cur->ttl == -2) {
            if (cur == g_zeroconf_mdns_response_root) {
                g_zeroconf_mdns_response_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            free_mdns_resp(del);

            g_zeroconf_mdns_header.resp_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void insertProxyDns(void)
{
    if (g_zeroconf_proxy_dns_sd_svc) {
        insertMdnsResp(g_zeroconf_proxy_dns_sd_svc->t_bytes,
                       g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                       12,
                       -3,
                       xTaskGetTickCount(),
                       g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                       g_zeroconf_proxy_dns_sd_svc->q_bytes,
                       0);

        insertMdnsResp(g_zeroconf_proxy_dns_sd_svc->h_bytes,
                       g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                       1,
                       -3,
                       xTaskGetTickCount(),
                       4,
                       g_zeroconf_proxy_dns_sd_svc->h_ip,
                       0);
    }
}

void deleteProxyDns(void)
{
    zeroconf_mdns_response_t *cur = g_zeroconf_mdns_response_root;
    zeroconf_mdns_response_t *prev = NULL;
    zeroconf_mdns_response_t *del = NULL;

    while (cur) {
        if (cur->ttl == -3) {
            if (cur == g_zeroconf_mdns_response_root) {
                g_zeroconf_mdns_response_root = cur->next;
            } else {
                prev->next = cur->next;
            }

            del = cur;
            cur = cur->next;
            free_mdns_resp(del);

            g_zeroconf_mdns_header.resp_count--;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }

    reset_lists();
}

void doDnsSdProb(void *params)
{
    int sys_wdog_id = -1;
    int ret = DA_APP_SUCCESS;
    int timeout = ZEROCONF_DEF_PROB_TIMEOUT;
    int conflict_count = 0;
    unsigned int prob_count = 0;

    const int send_buffer_size = ZEROCONF_DEF_BUF_SIZE;
    unsigned char *send_buffer = NULL;
    size_t send_buffer_len = 0;

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    char *prob_hostname = NULL;
    char *full_prob_hostname = NULL;
    unsigned char *full_prob_hostname_bytes = NULL;
    size_t full_prob_hostname_bytes_len = 0;

    zeroconf_dpm_prob_info_t *prob_info = (zeroconf_dpm_prob_info_t *)params;
    //zeroconf_service_type_t service_key = prob_info->service_key;
    unsigned int skip_prob = prob_info->skip_prob;

    zeroconf_mdns_packet_t prob_packet;

    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    dpm_clr_zeroconf_sleep(DNS_SD_DPM_TYPE);

    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_ACTIVATED;

    ZEROCONF_DEBUG_INFO("DNS-SD Probing.\n");

    get_ipv4_bytes(str_ipv4);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    memset(&prob_packet, 0x00, sizeof(zeroconf_mdns_packet_t));

    send_buffer = zeroconf_calloc(send_buffer_size, 1);
    if (send_buffer == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for send buffer\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_prob;
    }

    prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for hostname\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_prob;
    }

    full_prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for full hostname\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_prob;
    }

    full_prob_hostname_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for hostname bytes\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_prob;
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_stop_responder_task();

    strcpy(prob_hostname, g_zeroconf_local_dns_sd_service->name);

    vTaskDelay(random_number(1, 250));

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                         hostname_bytes,
                         ZEROCONF_DOMAIN_NAME_SIZE);

    timeout = zeroconf_get_mdns_prob_timeout();

    if (skip_prob) {
        ZEROCONF_DEBUG_INFO("To skip dns-sd probing\n");
        goto skip_probing;
    }

    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        prob_count = 0;

        if ((strlen(prob_hostname)
             + strlen(g_zeroconf_local_dns_sd_service->type)
             + strlen(g_zeroconf_local_dns_sd_service->domain) + 3) >
            ZEROCONF_DOMAIN_NAME_SIZE) {

            ZEROCONF_DEBUG_ERR("host name is too long\n");
            ret = DA_APP_NOT_SUCCESSFUL;
            goto end_of_dns_sd_prob;
        }

        memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
        sprintf(full_prob_hostname, "%s.%s.%s",
                prob_hostname,
                g_zeroconf_local_dns_sd_service->type,
                g_zeroconf_local_dns_sd_service->domain);

        full_prob_hostname_bytes_len = encode_mdns_name_string(full_prob_hostname,
                                       full_prob_hostname_bytes,
                                       ZEROCONF_DOMAIN_NAME_SIZE);

        ZEROCONF_DEBUG_INFO("Service Name: %s, Port : %d\n",
                            full_prob_hostname, g_zeroconf_local_dns_sd_service->port);

        while (prob_count < ZEROCONF_DEF_PROB_COUNT) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            clear_mdns_packet(&prob_packet);

            memset(send_buffer, 0x00, send_buffer_size);

            ZEROCONF_DEBUG_INFO("DNS-SD Probing with %s(#%02d)\n",
                                full_prob_hostname, prob_count + 1);

            ret = insert_question(&prob_packet.questions,
                                  full_prob_hostname_bytes,
                                  full_prob_hostname_bytes_len,
                                  ZEROCONF_MDNS_RR_TYPE_ALL,
                                  (((prob_count == 0 ? 1 : 0) << 15) | 1));
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create question(0x%x)\n", -ret);
                goto end_of_dns_sd_prob;
            }

            prob_packet.questions_count++;

            ret = insert_srv_record(&prob_packet.authority_rrs,
                                    full_prob_hostname_bytes,
                                    full_prob_hostname_bytes_len,
                                    1,
                                    120,
                                    0,
                                    0,
                                    g_zeroconf_local_dns_sd_service->port,
                                    hostname_bytes_len,
                                    hostname_bytes);
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create question(0x%x)\n", -ret);
                goto end_of_dns_sd_prob;
            }

            prob_packet.authority_rrs_count++;

            send_buffer_len = convert_mdns_packet_to_bytes(&prob_packet,
                                                           send_buffer,
                                                           send_buffer_size);
            if (send_buffer_len == 0) {
                ZEROCONF_DEBUG_ERR("failed to generate prob packet\n");

                ret = DA_APP_NOT_SUCCESSFUL;
                goto end_of_dns_sd_prob;
            }

            da16x_sys_watchdog_suspend(sys_wdog_id);

            ret = send_mdns_packet(MDNS_SERVICE_TYPE, send_buffer, send_buffer_len);
            if (ret == DA_APP_SUCCESS) {
                ret = receive_prob_response(timeout, &prob_packet);
                if (ret != DA_APP_SUCCESS) {
                    ZEROCONF_PRINTF("Probing ERROR: Conflict detected! Assigning "
                                    "New Host Name..(%d)\n", conflict_count + 1);

                    conflict_count++;

                    if (conflict_count >= ZEROCONF_DEF_CONFLICT_COUNT) {
                        timeout = zeroconf_get_mdns_prob_tie_breaker_timeout();
                    }

                    ZEROCONF_CONFLICT_NAME_RULE(prob_hostname,
                                                g_zeroconf_local_dns_sd_service->name,
                                                conflict_count);

                    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "sleep : %d msec\n" CLEAR_COLOR,
                                        timeout);

                    vTaskDelay(timeout);

                    break;
                }

                prob_count++;

                ZEROCONF_DEBUG_TEMP("Increased probing count(%d)\n", prob_count);
            } else {
                ZEROCONF_DEBUG_ERR("failed to send prob(0x%x)\n", -ret);

                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                goto end_of_dns_sd_prob;
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            ret = DA_APP_SUCCESS;
        }

        //if prob is completed
        if (prob_count >= ZEROCONF_DEF_PROB_COUNT) {
            break;
        }
    } //while

    ZEROCONF_DEBUG_INFO("Probing successfull! Service name assigned : %s\n",
                        prob_hostname);

    if (g_zeroconf_local_dns_sd_service->name) {
        zeroconf_free(g_zeroconf_local_dns_sd_service->name);

        g_zeroconf_local_dns_sd_service->name = zeroconf_calloc(strlen(prob_hostname) + 1, 1);
        if (g_zeroconf_local_dns_sd_service->name == NULL) {
            ZEROCONF_DEBUG_ERR("failed to allocate memory for name\n");
            goto end_of_dns_sd_prob;
        }

        memset(g_zeroconf_local_dns_sd_service->name, 0x00, strlen(prob_hostname) + 1);
    }

skip_probing:

    strcpy(g_zeroconf_local_dns_sd_service->name, prob_hostname);

    memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
    sprintf(full_prob_hostname, "%s.%s",
            g_zeroconf_local_dns_sd_service->type,
            g_zeroconf_local_dns_sd_service->domain);

    g_zeroconf_local_dns_sd_service->t_bytes_len = encode_mdns_name_string(
                full_prob_hostname,
                g_zeroconf_local_dns_sd_service->t_bytes,
                ZEROCONF_DOMAIN_NAME_SIZE);

    memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
    sprintf(full_prob_hostname, "%s.%s.%s",
            g_zeroconf_local_dns_sd_service->name,
            g_zeroconf_local_dns_sd_service->type,
            g_zeroconf_local_dns_sd_service->domain);

    g_zeroconf_local_dns_sd_service->q_bytes_len = encode_mdns_name_string(
                full_prob_hostname,
                g_zeroconf_local_dns_sd_service->q_bytes,
                ZEROCONF_DOMAIN_NAME_SIZE);

    ZEROCONF_DEBUG_INFO("Announcements scheduled. Service registered.\n");

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_start_responder_task();

    doDnsSdAnnounce(1, skip_prob);

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    ZEROCONF_DEBUG_INFO("Local Service Successfully registered for Advertising..\n");

end_of_dns_sd_prob:

    if (send_buffer) {
        zeroconf_free(send_buffer);
    }

    if (prob_hostname) {
        zeroconf_free(prob_hostname);
    }

    if (full_prob_hostname) {
        zeroconf_free(full_prob_hostname);
    }

    if (full_prob_hostname_bytes) {
        zeroconf_free(full_prob_hostname_bytes);
    }

    clear_mdns_packet(&prob_packet);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_DEACTIVATED;

    if (ret == DA_APP_SUCCESS) {
        set_dns_sd_service_status(ZEROCONF_ACTIVATED);
        printLocalDnsSd();
    }

    dpm_set_zeroconf_sleep(DNS_SD_DPM_TYPE);

    g_zeroconf_status.dns_sd_probing_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void doDnsSdProxyProb(void *params)
{
    //to do
    int sys_wdog_id = -1;
    int ret = DA_APP_SUCCESS;
    int check = 0;
    int timeout = ZEROCONF_DEF_PROB_TIMEOUT;
    int prob_resp = 0;
    int prob_count = 0;
    int conflict_count_service = 0;
    int conflict_count_host = 0;

    const int send_buffer_size = ZEROCONF_DEF_BUF_SIZE;
    unsigned char *send_buffer = NULL;
    size_t send_buffer_len = 0;

    char *prob_hostname = NULL;
    char *prob_servicename = NULL;

    char *full_prob_hostname = NULL;
    char *full_prob_servicename = NULL;

    unsigned char *full_prob_hostname_bytes = NULL;
    unsigned char *full_prob_servicename_bytes = NULL;

    size_t full_prob_hostname_bytes_len = 0;
    size_t full_prob_servicename_bytes_len = 0;

    zeroconf_dpm_prob_info_t *prob_info = (zeroconf_dpm_prob_info_t *)params;
    //zeroconf_service_type_t service_key = prob_info->service_key;
    unsigned int skip_prob = prob_info->skip_prob;

    zeroconf_mdns_packet_t prob_packet;

    unsigned short class = 1;
    unsigned short cache_flush_bit = 1;

    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    dpm_clr_zeroconf_sleep(DNS_SD_DPM_TYPE);

    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_ACTIVATED;

    ZEROCONF_DEBUG_INFO("DNS-SD Probing.\n");

    get_ipv4_bytes(str_ipv4);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    memset(&prob_packet, 0x00, sizeof(zeroconf_mdns_packet_t));

    send_buffer = zeroconf_calloc(send_buffer_size, 1);
    if (send_buffer == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for send buffer\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for hostname\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    full_prob_hostname = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for full hostname\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    full_prob_servicename = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_servicename == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for service name\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    full_prob_hostname_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_hostname_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for full hostname bytes\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    full_prob_servicename_bytes = zeroconf_calloc(ZEROCONF_DOMAIN_NAME_SIZE, 1);
    if (full_prob_servicename_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for full service name bytes\n");
        ret = DA_APP_MALLOC_ERROR;
        goto end_of_dns_sd_proxy_prob;
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_stop_responder_task();

    strcpy(prob_servicename, g_zeroconf_proxy_dns_sd_svc->name);

    strcpy(prob_hostname, g_zeroconf_proxy_dns_sd_svc->h_name);

    vTaskDelay(random_number(1, 250));

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    timeout = zeroconf_get_mdns_prob_timeout();

    if (skip_prob) {
        ZEROCONF_DEBUG_INFO("To skip dns-sd proxy probing\n");
        goto skip_probing;
    }

    while (check != 1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        prob_count = 0;

        //service name
        if ((strlen(prob_hostname)
             + strlen(g_zeroconf_proxy_dns_sd_svc->type)
             + strlen(g_zeroconf_proxy_dns_sd_svc->domain) + 3) >
            ZEROCONF_DOMAIN_NAME_SIZE) {

            ZEROCONF_DEBUG_ERR("service name is too long\n");
            ret = DA_APP_NOT_SUCCESSFUL;
            goto end_of_dns_sd_proxy_prob;
        }

        memset(full_prob_servicename, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
        sprintf(full_prob_servicename, "%s.%s.%s",
                prob_hostname,
                g_zeroconf_proxy_dns_sd_svc->type,
                g_zeroconf_proxy_dns_sd_svc->domain);

        //host name
        if ((strlen(prob_hostname)
             + strlen(g_zeroconf_proxy_dns_sd_svc->domain) + 2) >
            ZEROCONF_DOMAIN_NAME_SIZE) {

            ZEROCONF_DEBUG_ERR("host name is too long\n");
            ret = DA_APP_NOT_SUCCESSFUL;
            goto end_of_dns_sd_proxy_prob;
        }

        memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
        sprintf(full_prob_hostname, "%s.%s",
                prob_hostname, g_zeroconf_proxy_dns_sd_svc->domain);

        //service name in bytes
        full_prob_servicename_bytes_len = encode_mdns_name_string(full_prob_servicename,
                                          full_prob_servicename_bytes,
                                          ZEROCONF_DOMAIN_NAME_SIZE);

        //host name in bytes
        full_prob_hostname_bytes_len = encode_mdns_name_string(full_prob_hostname,
                                       full_prob_hostname_bytes,
                                       ZEROCONF_DOMAIN_NAME_SIZE);

        ZEROCONF_DEBUG_INFO("Service Name: %s , Host Name : %s, Port : %d\n",
                            prob_servicename,
                            prob_hostname,
                            g_zeroconf_proxy_dns_sd_svc->port);

        while (prob_count < ZEROCONF_DEF_PROB_COUNT) {
            da16x_sys_watchdog_notify(sys_wdog_id);

            clear_mdns_packet(&prob_packet);
            memset(send_buffer, 0x00, ZEROCONF_DEF_BUF_SIZE);

            ZEROCONF_DEBUG_INFO("DNS-SD Probing with %s(#%02d)\n",
                                full_prob_hostname, prob_count + 1);

            ret = insert_question(&prob_packet.questions,
                                  full_prob_servicename_bytes,
                                  full_prob_servicename_bytes_len,
                                  ZEROCONF_MDNS_RR_TYPE_ALL,
                                  (((prob_count == 0 ? 1 : 0) << 15) | 1));
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create question(0x%x)\n", -ret);
                goto end_of_dns_sd_proxy_prob;
            }

            prob_packet.questions_count++;

            ret = insert_question(&prob_packet.questions,
                                  full_prob_hostname_bytes,
                                  full_prob_hostname_bytes_len,
                                  ZEROCONF_MDNS_RR_TYPE_ALL,
                                  (((prob_count == 0 ? 1 : 0) << 15) | 1));
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create question(0x%x)\n", -ret);
                goto end_of_dns_sd_proxy_prob;
            }

            prob_packet.questions_count++;

            ret = insert_resource_record(&prob_packet.authority_rrs,
                                         full_prob_servicename_bytes,
                                         full_prob_servicename_bytes_len,
                                         ZEROCONF_MDNS_RR_TYPE_A,
                                         1,
                                         120,
                                         sizeof(g_zeroconf_proxy_dns_sd_svc->h_ip),
                                         g_zeroconf_proxy_dns_sd_svc->h_ip);
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create authority rr(0x%x)\n", -ret);
                goto end_of_dns_sd_proxy_prob;
            }

            prob_packet.authority_rrs_count++;

            class = 1;
            cache_flush_bit = 1;

            ret = insert_srv_record(&prob_packet.authority_rrs,
                                    full_prob_servicename_bytes,
                                    full_prob_servicename_bytes_len,
                                    ((cache_flush_bit << 15) | class),
                                    120,
                                    0,
                                    0,
                                    g_zeroconf_local_dns_sd_service->port,
                                    full_prob_hostname_bytes_len,
                                    full_prob_hostname_bytes);
            if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
                ZEROCONF_DEBUG_ERR("failed to create authority rr(0x%x)\n", -ret);
                goto end_of_dns_sd_proxy_prob;
            }

            prob_packet.authority_rrs_count++;

            da16x_sys_watchdog_suspend(sys_wdog_id);

            ret = send_mdns_packet(MDNS_SERVICE_TYPE, send_buffer, send_buffer_len);
            if (ret == DA_APP_SUCCESS) {
                //FIXME: Need to consider how to pass proxy's ip address
                prob_resp = receive_prob_response(timeout, &prob_packet);
                if (prob_resp != DA_APP_SUCCESS) {
                    conflict_count_service++;

                    if (prob_resp == 0) {
                        ZEROCONF_PRINTF("Probing ERROR: Conflict detected! "
                                        "Assigning New Service Name..\n");

                        ZEROCONF_CONFLICT_NAME_RULE(prob_servicename,
                                                    g_zeroconf_proxy_dns_sd_svc->name,
                                                    conflict_count_service);
                    } else if (prob_resp == 1) {
                        ZEROCONF_PRINTF("Probing ERROR: Conflict detected! "
                                        "Assigning New Host Name..\n");

                        ZEROCONF_CONFLICT_NAME_RULE(prob_hostname,
                                                    g_zeroconf_proxy_dns_sd_svc->h_name,
                                                    conflict_count_host);
                    }

                    if ((conflict_count_service >= ZEROCONF_DEF_CONFLICT_COUNT)
                        || (conflict_count_host >= ZEROCONF_DEF_CONFLICT_COUNT)) {
                        timeout = zeroconf_get_mdns_prob_tie_breaker_timeout();
                    }

                    ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "sleep : %d msec\n" CLEAR_COLOR,
                                        timeout);

                    vTaskDelay(timeout);

                    break;
                }

                prob_count++;
            } else {
                ZEROCONF_DEBUG_ERR("failed to send prob(0x%x)\n", -ret);

                da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
                goto end_of_dns_sd_proxy_prob;
            }
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        //if prob is completed
        if (prob_count >= ZEROCONF_DEF_PROB_COUNT) {
            ret = DA_APP_SUCCESS;
            break;
        }
    }

    ZEROCONF_DEBUG_INFO("Probing successfull!\n"
                        ">>>>> Host name : %s\n"
                        ">>>>> Service name : %s\n",
                        prob_hostname, prob_servicename);

    //for service name and related byte arrays
    if (g_zeroconf_proxy_dns_sd_svc->name) {
        zeroconf_free(g_zeroconf_proxy_dns_sd_svc->name);

        g_zeroconf_proxy_dns_sd_svc->name = zeroconf_calloc(strlen(
                                                prob_servicename) + 1, 1);
        if (g_zeroconf_proxy_dns_sd_svc->name == NULL) {
            ZEROCONF_DEBUG_ERR("failed to allocate memory for service name\n");
            ret = DA_APP_MALLOC_ERROR;
            goto end_of_dns_sd_proxy_prob;
        }

        memset(g_zeroconf_proxy_dns_sd_svc->name, 0x00, strlen(prob_servicename) + 1);
    }

skip_probing:

    strcpy(g_zeroconf_proxy_dns_sd_svc->name, prob_servicename);

    memset(full_prob_servicename, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
    sprintf(full_prob_servicename, "%s.%s",
            g_zeroconf_proxy_dns_sd_svc->type, g_zeroconf_proxy_dns_sd_svc->domain);

    g_zeroconf_proxy_dns_sd_svc->t_bytes_len = encode_mdns_name_string(
                full_prob_servicename,
                g_zeroconf_proxy_dns_sd_svc->t_bytes,
                ZEROCONF_DOMAIN_NAME_SIZE);

    memset(full_prob_servicename, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
    sprintf(full_prob_servicename, "%s.%s.%s",
            g_zeroconf_proxy_dns_sd_svc->name,
            g_zeroconf_proxy_dns_sd_svc->type,
            g_zeroconf_proxy_dns_sd_svc->domain);

    g_zeroconf_proxy_dns_sd_svc->q_bytes_len = encode_mdns_name_string(
                full_prob_servicename,
                g_zeroconf_proxy_dns_sd_svc->q_bytes,
                ZEROCONF_DOMAIN_NAME_SIZE);

    //for host name
    memset(full_prob_hostname, 0x00, ZEROCONF_DOMAIN_NAME_SIZE);
    sprintf(full_prob_hostname, "%s.%s", prob_hostname,
            g_zeroconf_proxy_dns_sd_svc->domain);

    g_zeroconf_proxy_dns_sd_svc->h_bytes_len = encode_mdns_name_string(
                full_prob_hostname,
                g_zeroconf_proxy_dns_sd_svc->h_bytes,
                ZEROCONF_DOMAIN_NAME_SIZE);

    ZEROCONF_DEBUG_INFO("Announcements scheduled. Service registered.\n");

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    zeroconf_start_responder_task();

    doDnsSdAnnounce(2, skip_prob);

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

end_of_dns_sd_proxy_prob:

    if (send_buffer) {
        zeroconf_free(send_buffer);
    }

    if (prob_hostname) {
        zeroconf_free(prob_hostname);
    }

    if (full_prob_hostname) {
        zeroconf_free(full_prob_hostname);
    }

    if (full_prob_servicename) {
        zeroconf_free(full_prob_servicename);
    }

    if (full_prob_hostname_bytes) {
        zeroconf_free(full_prob_hostname_bytes);
    }

    if (full_prob_servicename_bytes) {
        zeroconf_free(full_prob_servicename_bytes);
    }

    clear_mdns_packet(&prob_packet);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    g_zeroconf_status.dns_sd_probing_proc.status = ZEROCONF_DEACTIVATED;

    if (ret == DA_APP_SUCCESS) {
        printProxyDnsSd();
    }

    return;
}

void doDnsSdAnnounce(const int announce_type, unsigned int skip_announcement)
{
    unsigned char offset[3] = {192, 12, 0};
    unsigned char offset_host[3] = {0, 0, 0};
    unsigned char nsec_host[8] = {0x00,};
    unsigned char nsec_dns_sd[9] = {192, 12, 0, 5, 0, 0, 128, 0, 64};
    unsigned char str_ipv4[sizeof(unsigned long)] = {0x00,};
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    unsigned char str_ipv6[sizeof(unsigned long) * 4] = {0x00,};
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    unsigned int answer_rr_count = 0;
    unsigned int additional_rr_count = 0;

    unsigned short class = 0;
    unsigned short cache_flush_bit = 0;

    zeroconf_peer_info_t ipv4_info;
    zeroconf_peer_info_t ipv6_info;

    char *dns_sd_ptr_str = "_services._dns-sd._udp.local";

    unsigned char send_buffer[1024] = {0x00,};
    size_t send_buffer_len = 0;

    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    unsigned char *dns_sd_bytes = NULL;
    size_t dns_sd_bytes_len = 0;

    get_ipv4_bytes(str_ipv4);
#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    get_ipv6_bytes(str_ipv6);
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    memset(&ipv4_info, 0x00, sizeof(zeroconf_peer_info_t));
    memset(&ipv6_info, 0x00, sizeof(zeroconf_peer_info_t));

    if (is_supporting_ipv4()) {
        ipv4_info.type = MULTICAST_RESPONSE_TYPE;

        struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)&ipv4_info.ip_addr;

        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port = htons(ZEROCONF_MDNS_PORT);
        ipv4_addr->sin_addr.s_addr = inet_addr(ZEROCONF_MDNS_ADDR);
    }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
    if (is_supporting_ipv6()) {
        ipv6_info.type = MULTICAST_RESPONSE_TYPE;

        struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)&ipv6_info.ip_addr;

        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port = htons(ZEROCONF_MDNS_PORT);
        ipv6_addr->sin6_flowinfo = 0;
        //ipv6_addr = get_mdns_ipv6_multicast();
    }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

    dns_sd_bytes = zeroconf_calloc(strlen(dns_sd_ptr_str) + 2, 1);
    if (dns_sd_bytes == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory for dns-sd bytes\n");
        return;
    }

    dns_sd_bytes_len = encode_mdns_name_string(dns_sd_ptr_str,
                       dns_sd_bytes,
                       strlen(dns_sd_ptr_str) + 2);

    hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                         hostname_bytes,
                         ZEROCONF_DOMAIN_NAME_SIZE);

    switch (announce_type) {
        case 1: //for local service
            send_buffer_len = ZEROCONF_MDNS_HEADER_SIZE;

            class = 1;
            cache_flush_bit = 1;

            send_buffer_len += create_mdns_srv_rec(g_zeroconf_local_dns_sd_service->q_bytes,
                                                   g_zeroconf_local_dns_sd_service->q_bytes_len,
                                                   ((cache_flush_bit << 15) | class),
                                                   4500, //ttl
                                                   g_zeroconf_local_dns_sd_service->port,
                                                   hostname_bytes,
                                                   hostname_bytes_len,
                                                   send_buffer,
                                                   send_buffer_len);

            answer_rr_count++;

            send_buffer_len += create_mdns_txt_rec(offset,
                                                   2,
                                                   ((cache_flush_bit << 15) | class),
                                                   4500,
                                                   g_zeroconf_local_dns_sd_service->txtLen,
                                                   g_zeroconf_local_dns_sd_service->txtRecord,
                                                   send_buffer,
                                                   send_buffer_len);

            answer_rr_count++;

            class = 1;
            cache_flush_bit = 0;

            send_buffer_len += create_general_rec(dns_sd_bytes,
                                                  dns_sd_bytes_len,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  g_zeroconf_local_dns_sd_service->t_bytes_len,
                                                  g_zeroconf_local_dns_sd_service->t_bytes,
                                                  send_buffer,
                                                  send_buffer_len);

            answer_rr_count++;

            send_buffer_len += create_general_rec(g_zeroconf_local_dns_sd_service->t_bytes,
                                                  g_zeroconf_local_dns_sd_service->t_bytes_len,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  2,
                                                  offset,
                                                  send_buffer,
                                                  send_buffer_len);

            answer_rr_count++;

            set_offset(send_buffer_len, offset_host);
            nsec_host[0] = offset_host[0];
            nsec_host[1] = offset_host[1];

            //additional rrs
            //host's A record
            if (is_supporting_ipv4()) {
                class = 1;
                cache_flush_bit = 1;

                send_buffer_len += create_general_rec(hostname_bytes,
                                                      hostname_bytes_len,
                                                      ZEROCONF_MDNS_RR_TYPE_A,
                                                      ((cache_flush_bit << 15) | class),
                                                      120,
                                                      sizeof(str_ipv4),
                                                      str_ipv4,
                                                      send_buffer,
                                                      send_buffer_len);

                additional_rr_count++;

                nsec_host[3] = 1;
                nsec_host[4] |= 64;
            }

#if defined (LWIP_IPV6) && defined (SUPPORT_IPV6_ZEROCONF)
            if (is_supporting_ipv6()) {
                class = 1;
                cache_flush_bit = 1;

                send_buffer_len += create_general_rec(hostname_bytes,
                                                      hostname_bytes_len,
                                                      ZEROCONF_MDNS_RR_TYPE_AAAA,
                                                      ((cache_flush_bit << 15) | class),
                                                      120,
                                                      sizeof(str_ipv6),
                                                      str_ipv6,
                                                      send_buffer,
                                                      send_buffer_len);

                additional_rr_count++;

                nsec_host[3] = 4;
                nsec_host[7] |= 8;
            }
#endif // (LWIP_IPV6) && (SUPPORT_IPV6_ZEROCONF) 

            class = 1;
            cache_flush_bit = 1;

            //host's NSEC record
            send_buffer_len += create_general_rec(offset_host,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_NSEC,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  3 + 1 + nsec_host[3],
                                                  nsec_host,
                                                  send_buffer,
                                                  send_buffer_len);

            additional_rr_count++;

            //dns-sd NSEC record
            send_buffer_len += create_general_rec(offset,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_NSEC,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  9,
                                                  nsec_dns_sd,
                                                  send_buffer,
                                                  send_buffer_len);

            additional_rr_count++;

            create_packet_header(0,
                                 answer_rr_count,
                                 0,
                                 additional_rr_count,
                                 0,
                                 send_buffer, 0);

            if (!dpm_mode_is_wakeup()) {
                send_mdns_packet(MDNS_SERVICE_TYPE,
                                 send_buffer,
                                 send_buffer_len);

                insertPckSend(send_buffer,
                              send_buffer_len,
                              3,        //max count
                              100,        //first interval(1 sec)
                              xTaskGetTickCount(),
                              2,            //multi fact
                              MDNS_SERVICE_TYPE,    //type
                              ipv4_info,
                              ipv6_info,
                              DNS_SD_ANNOUNCE_TYPE);
            }

            insertLocalDns();
            break;
        case 2: //for proxy service
            send_buffer_len = ZEROCONF_MDNS_HEADER_SIZE;

            class = 1;
            cache_flush_bit = 1;

            //answer rrs
            send_buffer_len += create_mdns_srv_rec(g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                                   g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                                   ((cache_flush_bit << 15) | class),
                                                   4500,
                                                   g_zeroconf_proxy_dns_sd_svc->port,
                                                   g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                                   g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                                   send_buffer,
                                                   send_buffer_len);

            answer_rr_count++;

            send_buffer_len += create_mdns_txt_rec(offset,
                                                   2,
                                                   ((cache_flush_bit << 15) | class),
                                                   4500,
                                                   g_zeroconf_proxy_dns_sd_svc->txtLen,
                                                   g_zeroconf_proxy_dns_sd_svc->txtRecord,
                                                   send_buffer,
                                                   send_buffer_len);

            answer_rr_count++;

            class = 1;
            cache_flush_bit = 0;

            send_buffer_len += create_general_rec(dns_sd_bytes,
                                                  dns_sd_bytes_len,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                                                  g_zeroconf_proxy_dns_sd_svc->t_bytes,
                                                  send_buffer,
                                                  send_buffer_len);

            answer_rr_count++;

            send_buffer_len += create_general_rec(g_zeroconf_proxy_dns_sd_svc->t_bytes,
                                                  g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  2,
                                                  offset,
                                                  send_buffer,
                                                  send_buffer_len);

            answer_rr_count++;

            set_offset(send_buffer_len, offset_host);
            nsec_host[0] = offset_host[0];
            nsec_host[1] = offset_host[1];

            send_buffer_len += create_general_rec(g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                                  g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                                  ZEROCONF_MDNS_RR_TYPE_A,
                                                  1,
                                                  120,
                                                  4,
                                                  g_zeroconf_proxy_dns_sd_svc->h_ip,
                                                  send_buffer,
                                                  send_buffer_len);

            answer_rr_count++;

            //additional rrs
            //host's NSEC record

            //fixed value
            nsec_host[2] = 0;
            nsec_host[3] = 1;
            nsec_host[4] = 64;

            send_buffer_len += create_general_rec(offset_host,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_NSEC,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  5,
                                                  nsec_host,
                                                  send_buffer,
                                                  send_buffer_len);

            additional_rr_count++;

            //dns-sd NSEC record
            send_buffer_len += create_general_rec(offset,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_NSEC,
                                                  ((cache_flush_bit << 15) | class),
                                                  120,
                                                  9,
                                                  nsec_dns_sd,
                                                  send_buffer,
                                                  send_buffer_len);

            additional_rr_count++;

            create_packet_header(0,
                                 answer_rr_count,
                                 0,
                                 additional_rr_count,
                                 0,
                                 send_buffer, 0);

            if (!skip_announcement) {
                send_mdns_packet(MDNS_SERVICE_TYPE,
                                 send_buffer,
                                 send_buffer_len);

                insertPckSend(send_buffer, send_buffer_len,
                              3,                            //max count
                              100,                            //first interval(1 sec)
                              xTaskGetTickCount(),
                              2,                            //multi fact
                              MDNS_SERVICE_TYPE,            //type
                              ipv4_info,
                              ipv6_info,
                              DNS_SD_ANNOUNCE_TYPE);
            }

            insertProxyDns();
            break;
        default:
            ZEROCONF_DEBUG_ERR("Invaild key\n");
            break;
    }

    if (dns_sd_bytes != NULL) {
        zeroconf_free(dns_sd_bytes);
        dns_sd_bytes = NULL;
    }

    return;
}

void execute_dns_sd_E(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    int len = 0;
    int count = 1;
    int ch_res = 0;
    char *name = NULL;
    zeroconf_mdns_response_t *temp = NULL;

    DA16X_UNUSED_ARG(params);

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    ZEROCONF_PRINTF("Looking for recommended registration domains. Press 'x' to stop..\n");

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);
    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        ZEROCONF_MDNS_RR_TYPE_PTR,
                        1,
                        req_packet,
                        len);

    send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        vTaskDelay(100);

        if (g_zeroconf_mdns_response_root != NULL) {
            temp = g_zeroconf_mdns_response_root;

            while (temp != NULL) {
                ch_res = compare_domain_bytes(temp->query,
                                              temp->query_len,
                                              g_zeroconf_dns_sd_query.query_bytes,
                                              g_zeroconf_dns_sd_query.query_bytes_len);

                if (ch_res == 1 && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                    name = zeroconf_calloc(temp->data_len, 1);
                    if (name == NULL) {
                        ZEROCONF_DEBUG_ERR("Failed to allocate memory for name\n");
                        break;
                    }

                    if (unencode_mdns_name_string(temp->data, 0, name, temp->data_len)) {
                        ZEROCONF_PRINTF("Domain #%d : %s\n", count, name);
                    }

                    zeroconf_free(name);

                    name = NULL;

                    count++;
                }

                temp = temp->next;
            }
        }
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void execute_dns_sd_F(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    int len = 0;
    int count = 1;
    int ch_res = 0;
    char *name = NULL;
    zeroconf_mdns_response_t *temp = NULL;

    DA16X_UNUSED_ARG(params);

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);
    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        ZEROCONF_MDNS_RR_TYPE_PTR,
                        1,
                        req_packet,
                        len);

    send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

    ZEROCONF_PRINTF("Looking for recommended browsing domains. Press 'x' to stop..\n");

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        vTaskDelay(100);

        if (g_zeroconf_mdns_response_root != NULL) {
            temp = g_zeroconf_mdns_response_root;

            while (temp != NULL) {
                ch_res = compare_domain_bytes(temp->query,
                                              temp->query_len,
                                              g_zeroconf_dns_sd_query.query_bytes,
                                              g_zeroconf_dns_sd_query.query_bytes_len);

                if (ch_res == 1 && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                    name = zeroconf_calloc(temp->data_len, 1);
                    if (name == NULL) {
                        ZEROCONF_DEBUG_ERR("Failed to allocate memory\n");
                        break;
                    }

                    if (unencode_mdns_name_string(temp->data, 0, name, temp->data_len)) {
                        ZEROCONF_PRINTF("Domain #%d : %s\n", count, name);
                    }

                    zeroconf_free(name);

                    name = NULL;

                    count++;
                }

                temp = temp->next;
            }
        }
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void execute_dns_sd_B(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    unsigned char offset[3] = {192, 12, 0};
    int len = 0;
    int count = 0;
    int retry_count = 0;
    int ch_res = 0;
    int interval = 3000;
    int last_id = 0;
    zeroconf_mdns_response_t *temp = NULL;

    DA16X_UNUSED_ARG(params);

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);
    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        ZEROCONF_MDNS_RR_TYPE_PTR,
                        1,
                        req_packet,
                        len);

    print_dns_sd_old_cache();

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        if (g_zeroconf_mdns_response_root != NULL) {
            temp = g_zeroconf_mdns_response_root;

            while (temp != NULL) {
                if (temp->id > last_id) {
                    ch_res = compare_domain_bytes(temp->query,
                                                  temp->query_len,
                                                  g_zeroconf_dns_sd_query.query_bytes,
                                                  g_zeroconf_dns_sd_query.query_bytes_len);

                    if (ch_res == 1 && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                        count++;

                        create_packet_header(1, count, 0, 0, 1, req_packet, 0);

                        len += create_general_rec(offset,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  1,
                                                  temp->ttl,
                                                  strlen((const char *)temp->data) + 1,
                                                  temp->data,
                                                  req_packet,
                                                  len);

                        last_id = temp->id;
                    }
                }

                temp = temp->next;
            }
        }

        if (retry_count == 3) {
            interval = 5000;
        } else if (retry_count == 15) {
            interval = 10000;
        }

        if (retry_count > 0) {
            vTaskDelay(interval);
        }

        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

        retry_count++;
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void execute_dns_sd_L(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    unsigned char offset[3] = {192, 12, 0};
    int len = 0;
    int interval = 30000;

    DA16X_UNUSED_ARG(params);

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    create_packet_header(2, 0, 0, 0, 1, req_packet, 0);
    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        ZEROCONF_MDNS_RR_TYPE_SRV,
                        1,
                        req_packet,
                        len);

    len += create_query(offset,
                        2,
                        ZEROCONF_MDNS_RR_TYPE_PTR,
                        1,
                        req_packet,
                        len);

    print_dns_sd_old_cache();

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);
        vTaskDelay(interval);
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

int execute_dns_sd_P(int argc, char *argv[])
{
    g_zeroconf_dns_sd_query.query_key = 'P';

    //Check primary command execution condition
    if (g_zeroconf_proxy_dns_sd_svc) {
        ZEROCONF_DEBUG_ERR("Already registered. Execute 'dns-sd -UP' command!\n");
        return 0;
    }

    if (is_running_dns_sd_probing_processor()) {
        ZEROCONF_DEBUG_ERR("DNS-SD services are probing! "
                           "Please try again when Probing is over.\n");
        return 0;
    }

    if (argc > 8) {
        unsigned char h_ip[4];
        int isValid = 1;

        to_lower(argv[2]);
        char *name = (argv[2]);

        to_lower(argv[3]);
        char *type = (argv[3]);

        to_lower(argv[4]);
        char *domain = (argv[4]);

        int port = atoi(argv[5]);

        to_lower(argv[6]);
        char *h_name = (argv[6]);

        to_lower(argv[7]);
        char *h_ip_txt = (argv[7]);

        if ((strlen(h_name) == 0) || (strlen(h_ip_txt) == 0) || (strlen(name) == 0)
            || (strlen(type) == 0) || (strlen(domain) == 0)) {

            ZEROCONF_DEBUG_ERR("Host Name, Host IP, "
                               "Service Name, service Type and Domain can not be blank\n");
            isValid = 0;
        }

        volatile long ip_addr = (long)iptolong(h_ip_txt);

        if (ip_addr <= 0) {
            ZEROCONF_DEBUG_ERR("Host IP address format error.\n");
            isValid = 0;
        } else {
            h_ip[0] = (unsigned char)((ip_addr >> 24) & 0x0ff);
            h_ip[1] = (unsigned char)((ip_addr >> 16) & 0x0ff);
            h_ip[2] = (unsigned char)((ip_addr >> 8) & 0x0ff);
            h_ip[3] = (unsigned char)((ip_addr >> 0) & 0x0ff);
        }

        if ((strcmp(type + strlen(type) - 5, "._tcp") != 0)
            && (strcmp(type + strlen(type) - 5, "._udp") != 0)) {
            ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type\n");
            isValid = 0;
        }

        if (strcmp(h_name + strlen(h_name) - 6, ".local") == 0) {
            ZEROCONF_DEBUG_ERR("Do not include top level domain in Host name\n");
            isValid = 0;
        }

        if (port <= 0) {
            ZEROCONF_DEBUG_ERR("Invalid Port No.\n");
            isValid = 0;
        }

        if (strcmp(domain, ".") == 0) {
            domain = "local";
        } else if (strcmp(domain, "local") != 0) {
            ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
            isValid = 0;
        }

        unsigned char txt[256] = "";
        unsigned char *ptr = txt;

        if (argc) {
            for (int i = 8 ; i < (argc) ; i++) {
                const char *p = argv[i];
                *ptr = 0;

                while (*p && *ptr < 255 && ptr + 1 + *ptr < txt + sizeof(txt)) {
                    if (p[0] != '\\' || p[1] == 0) {
                        ptr[++*ptr] = *p;
                        p += 1;
                    } else if (p[1] == 'x' && isxdigit((int)p[2]) && isxdigit((int)p[3])) {
                        ptr[++*ptr] = HexPair(p + 2);
                        p += 4;
                    } else {
                        ptr[++*ptr] = p[1];
                        p += 2;
                    }
                }

                ptr += 1 + *ptr;
            }
        }

        if (isValid) {
            g_zeroconf_prob_info.service_key = UNKNOWN_SERVICE_TYPE;
            g_zeroconf_prob_info.skip_prob = pdFALSE;

            g_zeroconf_proxy_dns_sd_svc = createProxyDnsSd(h_name, h_ip, name, type,
                                                           domain, port,
                                                           (ptr - txt), txt);

            zeroconf_start_dns_sd_proxy_probing_task(&g_zeroconf_prob_info);

            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

void execute_dns_sd_Q(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    int len = 0;
    int interval = 30000;

    unsigned long type = (unsigned long)params;

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);

    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        type,
                        1,
                        req_packet,
                        len);

    print_dns_sd_old_cache();

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

        vTaskDelay(interval);
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void execute_dns_sd_Z(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    unsigned char offset[3] = {192, 12, 0};
    int len = 0;
    int count = 0;
    int retry_count = 0;
    int ch_res = 0;
    int interval = 3000;
    int last_id = 0;
    zeroconf_mdns_response_t *temp = NULL;

    DA16X_UNUSED_ARG(params);

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    create_packet_header(1, 0, 0, 0, 1, req_packet, 0);

    len = ZEROCONF_MDNS_HEADER_SIZE;

    len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                        g_zeroconf_dns_sd_query.query_bytes_len,
                        ZEROCONF_MDNS_RR_TYPE_ALL,
                        1,
                        req_packet,
                        len);

    print_dns_sd_old_cache();

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        if (g_zeroconf_mdns_response_root != NULL) {
            temp = g_zeroconf_mdns_response_root;

            while (temp != NULL) {
                if (temp->id > last_id) {
                    ch_res = compare_domain_bytes(temp->query,
                                                  temp->query_len,
                                                  g_zeroconf_dns_sd_query.query_bytes,
                                                  g_zeroconf_dns_sd_query.query_bytes_len);

                    if (ch_res == 1 && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                        count++;

                        create_packet_header(1, count, 0, 0, 1, req_packet, 0);

                        len += create_general_rec(offset,
                                                  2,
                                                  ZEROCONF_MDNS_RR_TYPE_PTR,
                                                  1,
                                                  temp->ttl,
                                                  temp->data_len,
                                                  temp->data,
                                                  req_packet,
                                                  len);

                        last_id = temp->id;
                    }
                }

                if (temp != NULL) {
                    temp = temp->next;
                }
            }
        }

        if (retry_count == 3) {
            interval = 5000;
        } else if (retry_count == 15) {
            interval = 10000;
        }

        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

        retry_count++;

        vTaskDelay(interval);
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

void execute_dns_sd_G(void *params)
{
    unsigned char req_packet[ZEROCONF_DEF_BUF_SIZE] = {0x00,};
    int len = 0;
    int interval = 30000;
    unsigned long type = (unsigned long)params;

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_ACTIVATED;

    len = ZEROCONF_MDNS_HEADER_SIZE;

    if (type == ZEROCONF_MDNS_RR_TYPE_A) {
        create_packet_header(1, 0, 0, 0, 1, req_packet, 0);

        len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                            g_zeroconf_dns_sd_query.query_bytes_len,
                            ZEROCONF_MDNS_RR_TYPE_A,
                            1,
                            req_packet,
                            len);

    } else if (type == ZEROCONF_MDNS_RR_TYPE_AAAA) {
        create_packet_header(1, 0, 0, 0, 1, req_packet, 0);

        len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                            g_zeroconf_dns_sd_query.query_bytes_len,
                            ZEROCONF_MDNS_RR_TYPE_AAAA,
                            1,
                            req_packet,
                            len);

    } else { //ZEROCONF_MDNS_RR_TYPE_ALL
        create_packet_header(2, 0, 0, 0, 1, req_packet, 0);

        len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                            g_zeroconf_dns_sd_query.query_bytes_len,
                            ZEROCONF_MDNS_RR_TYPE_A,
                            1,
                            req_packet,
                            len);

        len += create_query(g_zeroconf_dns_sd_query.query_bytes,
                            g_zeroconf_dns_sd_query.query_bytes_len,
                            ZEROCONF_MDNS_RR_TYPE_AAAA,
                            1,
                            req_packet,
                            len);
    }

    print_dns_sd_old_cache();

    while (g_zeroconf_status.dns_sd_exec_proc.status == ZEROCONF_ACTIVATED) {
        send_mdns_packet(MDNS_SERVICE_TYPE, req_packet, len);

        vTaskDelay(interval);
    }

    g_zeroconf_status.dns_sd_exec_proc.status = ZEROCONF_DEACTIVATED;

    g_zeroconf_status.dns_sd_exec_proc.task_handler = NULL;

    vTaskDelete(NULL);

    return;
}

unsigned long get_type(char *type_code)
{
    if (strcmp(type_code, "a") == 0) {
        return ZEROCONF_MDNS_RR_TYPE_A;
    } else if (strcmp(type_code, "aaaa") == 0) {
        return ZEROCONF_MDNS_RR_TYPE_AAAA;
    } else if (strcmp(type_code, "ptr" ) == 0) {
        return ZEROCONF_MDNS_RR_TYPE_PTR;
    } else if (strcmp(type_code, "txt" ) == 0) {
        return ZEROCONF_MDNS_RR_TYPE_TXT;
    } else if (strcmp(type_code, "srv" ) == 0) {
        return ZEROCONF_MDNS_RR_TYPE_SRV;
    } else if (strcmp(type_code, "nsec") == 0) {
        return ZEROCONF_MDNS_RR_TYPE_NSEC;
    } else if (strcmp(type_code, "any" ) == 0
               || strcmp(type_code, "." ) == 0) {
        return ZEROCONF_MDNS_RR_TYPE_ALL;
    } else {
        return 0;
    }
}

void print_dns_sd_command_error(void)
{
    ZEROCONF_DEBUG_ERR("Invalid dns-sd command format\n");
    print_dns_sd_command_help();
}

void print_dns_sd_command_help(void)
{
    ZEROCONF_PRINTF("DNS-SD command formats : \n");

    ZEROCONF_PRINTF("dns-sd -E                                        <Enumerates recommended registration domains>\n\n");

    ZEROCONF_PRINTF("dns-sd -F                                        <Enumerates recommended brwosing domains>\n\n");

    ZEROCONF_PRINTF("dns-sd -R name type domain port [key=value ...]  <Register a service>\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -R myservice _service._tcp local 9110 \"host=16x\" \n\n");

    ZEROCONF_PRINTF("dns-sd -P name type domain port host ip[key=value ...]  <Register a service>\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -R myservice _service._tcp local 9110 proxy 172.28.2.199 \"host=16x\"\n\n");

    ZEROCONF_PRINTF("dns-sd -B type domain                            <Browse for service instances>\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -B _service._tcp local \n\n");

    ZEROCONF_PRINTF("dns-sd -L name type domain                       <Look up for a service instance>\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -L myservice _service._tcp local \n\n");

    ZEROCONF_PRINTF("dns-sd -Q FQDN rrtype rrclass                    <Generic query for any Record type>\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -Q myservice._service._tcp.local txt 1\n\n");

    ZEROCONF_PRINTF("dns-sd -Z type domain                            <Output browsesd services in Zone file format\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -Z _service._tcp local \n\n");

    ZEROCONF_PRINTF("dns-sd -G v4/v6/v4v6 FQDN                        <Get address information for a host name\n");
    ZEROCONF_PRINTF("    e.g. dns-sd -G v4v6 fci.local \n\n");

}

void stop_dns_sd_local_service(void)
{
    unsigned char hostname_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t hostname_bytes_len = 0;

    const size_t send_buffer_size = ZEROCONF_DEF_BUF_SIZE;
    unsigned char *send_buffer = NULL;
    unsigned int send_buffer_len = 0;

    unsigned short class = 1;
    unsigned short cache_flush_bit = 0;

    ZEROCONF_DEBUG_INFO("StarT\n");

    if (g_zeroconf_sender_event_handler) {
        xEventGroupSetBits(g_zeroconf_sender_event_handler,
                           ZEROCONF_EVT_REMOVE_DNS_SD_ANNOUNCE);
    } else {
            ZEROCONF_DEBUG_ERR("Event handler is NULL to send packet\n");
    }

    send_buffer = zeroconf_calloc(send_buffer_size, 1);
    if (send_buffer == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for send buffer\n");
        return;
    }

    memset(send_buffer, 0x00, send_buffer_size);

    hostname_bytes_len = encode_mdns_name_string(g_zeroconf_mdns_host_info.hostname,
                         hostname_bytes,
                         ZEROCONF_DOMAIN_NAME_SIZE);

    create_packet_header(0, 2, 0, 0, 0, send_buffer, 0);

    send_buffer_len = ZEROCONF_MDNS_HEADER_SIZE;

    send_buffer_len += create_general_rec(g_zeroconf_local_dns_sd_service->t_bytes,
                                          g_zeroconf_local_dns_sd_service->t_bytes_len,
                                          ZEROCONF_MDNS_RR_TYPE_PTR,
                                          ((cache_flush_bit << 15) | class),
                                          0,
                                          g_zeroconf_local_dns_sd_service->q_bytes_len,
                                          g_zeroconf_local_dns_sd_service->q_bytes,
                                          send_buffer,
                                          send_buffer_len);

    class = 1;
    cache_flush_bit = 1;

    send_buffer_len += create_mdns_srv_rec(g_zeroconf_local_dns_sd_service->q_bytes,
                                           g_zeroconf_local_dns_sd_service->q_bytes_len,
                                           ((cache_flush_bit << 15) | class),
                                           0,
                                           g_zeroconf_local_dns_sd_service->port,
                                           hostname_bytes,
                                           hostname_bytes_len,
                                           send_buffer,
                                           send_buffer_len);

    send_mdns_packet(MDNS_SERVICE_TYPE, send_buffer, send_buffer_len);

    if (g_zeroconf_local_dns_sd_service != NULL) {
        deleteLocalDnsSd(g_zeroconf_local_dns_sd_service);
    }

    g_zeroconf_local_dns_sd_service = NULL;

    deleteLocalDns();

    set_dns_sd_service_status(ZEROCONF_DEACTIVATED);

    set_dns_sd_service_status(ZEROCONF_INIT);

    if (send_buffer) {
        zeroconf_free(send_buffer);
    }

    return;
}

void stop_dns_sd_proxy_service(void)
{
    unsigned short class = 1;
    unsigned short cache_flush_bit = 0;

    const size_t send_buffer_size = ZEROCONF_DEF_BUF_SIZE;
    unsigned char *send_buffer = NULL;
    unsigned int send_buffer_len = 0;

    send_buffer = zeroconf_calloc(send_buffer_size, 1);

    if (send_buffer == NULL) {
        ZEROCONF_DEBUG_ERR("failed to allocate memory for send buffer\n");
        return;
    }

    memset(send_buffer, 0x00, send_buffer_size);

    create_packet_header(0, 3, 0, 0, 0, send_buffer, 0);

    send_buffer_len = ZEROCONF_MDNS_HEADER_SIZE;

    send_buffer_len += create_general_rec(g_zeroconf_proxy_dns_sd_svc->t_bytes,
                                          g_zeroconf_proxy_dns_sd_svc->t_bytes_len,
                                          ZEROCONF_MDNS_RR_TYPE_PTR,
                                          ((cache_flush_bit << 15) | class),
                                          0,
                                          g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                          g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                          send_buffer,
                                          send_buffer_len);

    class = 1;
    cache_flush_bit = 1;

    send_buffer_len += create_general_rec(g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                          g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                          ZEROCONF_MDNS_RR_TYPE_A,
                                          1,
                                          0,
                                          sizeof(g_zeroconf_proxy_dns_sd_svc->h_ip),
                                          g_zeroconf_proxy_dns_sd_svc->h_ip,
                                          send_buffer,
                                          send_buffer_len);

    send_buffer_len += create_mdns_srv_rec(g_zeroconf_proxy_dns_sd_svc->q_bytes,
                                           g_zeroconf_proxy_dns_sd_svc->q_bytes_len,
                                           ((cache_flush_bit << 15) | class),
                                           0,
                                           g_zeroconf_proxy_dns_sd_svc->port,
                                           g_zeroconf_proxy_dns_sd_svc->h_bytes,
                                           g_zeroconf_proxy_dns_sd_svc->h_bytes_len,
                                           send_buffer,
                                           send_buffer_len);

    send_mdns_packet(MDNS_SERVICE_TYPE, send_buffer, send_buffer_len);

    if (g_zeroconf_proxy_dns_sd_svc != NULL) {
        deleteProxyDnsSd(g_zeroconf_proxy_dns_sd_svc);
    }

    g_zeroconf_proxy_dns_sd_svc = NULL;

    deleteProxyDns();

    if (send_buffer) {
        zeroconf_free(send_buffer);
    }

    return;
}

void print_dns_sd_old_cache(void)
{
    zeroconf_mdns_response_t *temp = g_zeroconf_mdns_response_root;

    while (temp != NULL) {
        print_dns_sd_cache(temp, 1);
        temp = temp->next;
    }
}

void print_dns_sd_cache(zeroconf_mdns_response_t *temp, int isCreate)
{
    char time_str[100] = {0x00,};
#if defined (__TIME64__)
    __time64_t now = temp->timestamp;
#else
    time_t now = (int)temp->timestamp;
#endif // (__TIME64__)
    int ch_res = 0;
    char *name = NULL;
    char *to_print = NULL;
    int index = 0;

#if defined (__TIME64__)
    da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime64(&now));
#else
    da16x_strftime(time_str, 100, "%H:%M:%S", da16x_localtime32(&now));
#endif // (__TIME64__)

    if (zeroconf_is_running_dns_sd_service()) {
        switch (g_zeroconf_dns_sd_query.query_key) {
            case 'B':
                if (temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                    ch_res = compare_domain_bytes(temp->query,
                                                  temp->query_len,
                                                  g_zeroconf_dns_sd_query.query_bytes,
                                                  g_zeroconf_dns_sd_query.query_bytes_len);

                    if ( ch_res == 1) {
                        to_print = zeroconf_calloc(temp->data_len, 1);
                        if (to_print == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for ptr\n");
                            break;
                        }

                        unencode_mdns_name_string(temp->data, 0, to_print, temp->data_len);

                        name = zeroconf_calloc(temp->query_len, 1);

                        if (name == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for query\n");
                            zeroconf_free(to_print);
                            to_print = NULL;
                            break;
                        }

                        unencode_mdns_name_string(temp->query, 0, name, temp->query_len);

                        if (isCreate) {
                            ZEROCONF_PRINTF("\n%s   Added    %s  %s\n",
                                            time_str, name, to_print);
                        } else {
                            ZEROCONF_PRINTF("\n%s   Removed  %s  %s\n",
                                            time_str, name, to_print);
                        }

                        zeroconf_free(to_print);
                        to_print = NULL;

                        zeroconf_free(name);
                        name = NULL;
                    }
                }
                break;
            case 'L':
                if (temp->type == ZEROCONF_MDNS_RR_TYPE_SRV
                    || temp->type == ZEROCONF_MDNS_RR_TYPE_TXT) {
                    ch_res = compare_domain_bytes(temp->query,
                                                  temp->query_len,
                                                  g_zeroconf_dns_sd_query.query_bytes,
                                                  g_zeroconf_dns_sd_query.query_bytes_len);

                    if (ch_res == 1) {
                        name = zeroconf_calloc(temp->query_len, 1);

                        if (name == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for query\n");
                            break;
                        }

                        unencode_mdns_name_string(temp->query, 0, name, temp->query_len);

                        if (isCreate) {
                            if (temp->type == ZEROCONF_MDNS_RR_TYPE_SRV) {
                                int port = 0;

                                to_print = zeroconf_calloc(temp->data_len, 1);

                                if (to_print == NULL) {
                                    ZEROCONF_DEBUG_ERR("Failed to allocate memory for srv\n");
                                    zeroconf_free(name);
                                    name = NULL;
                                    break;
                                }

                                port = (temp->data[4] << 8 | temp->data[5]);

                                unencode_mdns_name_string(temp->data, 6, to_print, temp->data_len);

                                ZEROCONF_PRINTF("\n>> %s can be reached on %s:%d\n",
                                                name, to_print, port);

                            } else if (temp->type == ZEROCONF_MDNS_RR_TYPE_TXT) {
                                to_print = zeroconf_calloc(temp->data_len, 1);

                                if (to_print == NULL) {
                                    ZEROCONF_DEBUG_ERR("Failed to allocate memory for txt\n");
                                    zeroconf_free(name);
                                    name = NULL;
                                    break;
                                }

                                ZEROCONF_PRINTF("\n>> %s\n", name);

                                unsigned char *ptr = temp->data;

                                while (ptr - temp->data < (int)temp->data_len) {
                                    unsigned char sTxtLen = *ptr++; //length
                                    ZEROCONF_PRINTF("\tTXT(%d)\t: ", sTxtLen);

                                    for (index = 0 ; index < sTxtLen ; index++) {
                                        ZEROCONF_PRINTF("%c", ptr[index]);
                                    }

                                    ZEROCONF_PRINTF("\n");
                                    ptr += (index);
                                }

                                break;
                            }
                        } else {
                            if (temp->type == ZEROCONF_MDNS_RR_TYPE_SRV) {
                                ZEROCONF_PRINTF("\n%s  %s  Removed SRV record\n",
                                                time_str, name);
                            } else {
                                ZEROCONF_PRINTF("\n%s  %s  Removed TXT record\n",
                                                time_str, name);
                            }
                        }

                        zeroconf_free(name);
                        name = NULL;
                    }
                }
                break;
            case 'Q':
                if (compare_domain_bytes(temp->query,
                                         temp->query_len,
                                         g_zeroconf_dns_sd_query.query_bytes,
                                         g_zeroconf_dns_sd_query.query_bytes_len)) {
                    if ((temp->type == (int)g_zeroconf_dns_sd_query.q_type)
                        || (g_zeroconf_dns_sd_query.q_type == ZEROCONF_MDNS_RR_TYPE_ALL)) {
                        if (isCreate) {
                            print_mdns_response(temp, 0, 0);
                        }
                    } else if (!isCreate && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                        name = zeroconf_calloc(temp->data_len, 1);

                        if (name == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for ptr\n");
                            break;
                        }

                        unencode_mdns_name_string(temp->data, 0, name, temp->data_len);

                        ZEROCONF_PRINTF("\n%s  %s  Service Stopped. Cache Cleared \n",
                                        time_str, name);

                        zeroconf_free(name);
                        name = NULL;
                    }
                }
                break;
            case 'Z':
                if (compare_domain_bytes(temp->query,
                                         temp->query_len,
                                         g_zeroconf_dns_sd_query.query_bytes,
                                         g_zeroconf_dns_sd_query.query_bytes_len)
                    || search_in_byte_array(temp->query,
                                            temp->query_len,
                                            g_zeroconf_dns_sd_query.query_bytes,
                                            g_zeroconf_dns_sd_query.query_bytes_len)) {
                    if (isCreate) {
                        print_mdns_response(temp, 0, 0);
                    } else if (!isCreate && temp->type == ZEROCONF_MDNS_RR_TYPE_PTR) {
                        name = zeroconf_calloc(temp->data_len, 1);

                        if (name == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for name\n");
                            break;
                        }

                        unencode_mdns_name_string(temp->data, 0, name, temp->data_len);

                        ZEROCONF_PRINTF("\n%s  %s  Service Stopped. Cache Cleared \n",
                                        time_str, name);

                        zeroconf_free(name);
                        name = NULL;
                    }
                }
                break;
            case 'G':
                if (temp->type == ZEROCONF_MDNS_RR_TYPE_A
                    || temp->type == ZEROCONF_MDNS_RR_TYPE_AAAA) {

                    ch_res = compare_domain_bytes(temp->query,
                                                  temp->query_len,
                                                  g_zeroconf_dns_sd_query.query_bytes,
                                                  g_zeroconf_dns_sd_query.query_bytes_len);

                    if (ch_res == 1) {
                        name = zeroconf_calloc(temp->query_len, 1);

                        if (name == NULL) {
                            ZEROCONF_DEBUG_ERR("Failed to allocate memory for query\n");
                            break;
                        }

                        unencode_mdns_name_string(temp->query, 0, name, temp->query_len);

                        if (isCreate) {
                            if (temp->type == ZEROCONF_MDNS_RR_TYPE_A) {
                                if (g_zeroconf_dns_sd_query.g_type == ZEROCONF_MDNS_RR_TYPE_A
                                    || g_zeroconf_dns_sd_query.g_type == ZEROCONF_MDNS_RR_TYPE_ALL) {
                                    ZEROCONF_PRINTF("\n>>%s Address Information  ", name);
                                    ZEROCONF_PRINTF("  IPv4");
                                    ZEROCONF_PRINTF("   %d.%d.%d.%d\n",
                                                    temp->data[0],
                                                    temp->data[1],
                                                    temp->data[2],
                                                    temp->data[3]);
                                }
                            } else {
                                if (g_zeroconf_dns_sd_query.g_type == ZEROCONF_MDNS_RR_TYPE_AAAA
                                    || g_zeroconf_dns_sd_query.g_type == ZEROCONF_MDNS_RR_TYPE_ALL) {
                                    ZEROCONF_PRINTF("\n>>%s Address Information  ", name);
                                    ZEROCONF_PRINTF("  IPv6");
                                    ZEROCONF_PRINTF("   %x%x.%x%x.%x%x.%x%x.%x%x.%x%x.%x%x.%x%x\n",
                                                    temp->data[0], temp->data[1],
                                                    temp->data[2], temp->data[3],
                                                    temp->data[4], temp->data[5],
                                                    temp->data[6], temp->data[7],
                                                    temp->data[8], temp->data[9],
                                                    temp->data[10], temp->data[11],
                                                    temp->data[12], temp->data[13],
                                                    temp->data[14], temp->data[15]);
                                }
                            }
                        } else {
                            if (temp->type == ZEROCONF_MDNS_RR_TYPE_A) {
                                ZEROCONF_PRINTF("\n%s  %s  IPv4 Information Removed\n",
                                                time_str, name);
                            } else {
                                ZEROCONF_PRINTF("\n%s  %s  IPv6 Information Removed\n",
                                                time_str, name);
                            }
                        }

                        zeroconf_free(name);
                        name = NULL;
                    }
                }
                break;
            case 'R':
                break;
            default:
                ZEROCONF_DEBUG_INFO("unknown dns-sd query key:%c\n",
                                    g_zeroconf_dns_sd_query.query_key);
                break;
        }

        if (name != NULL) {
            zeroconf_free(name);
            name = NULL;
        }

        if (to_print != NULL) {
            zeroconf_free(to_print);
            to_print = NULL;
        }
    }

    return;
}

int zeroconf_register_dns_sd_service(char *name, char *type, char *domain, int port,
                                     int txt_cnt, char **txt_rec, unsigned int skip_prob)
{
    unsigned char txt[ZEROCONF_DNS_SD_SRV_TXT_LEN + 1] = "";
    unsigned char *ptr = txt;
    int i = 0;

    if (!zeroconf_is_running_mdns_service()) {
        ZEROCONF_DEBUG_INFO("DNS-SD can not work while mDNS is not running!\n");
        return DA_APP_NOT_ENABLED;
    }

    if (is_running_mdns_probing_processor()) {
        ZEROCONF_DEBUG_INFO("DNS-SD can not work while zeroconf is probing!\n");
        return DA_APP_IN_PROGRESS;
    }

    if (is_running_dns_sd_probing_processor()) {
        ZEROCONF_DEBUG_INFO("DNS SD Probing is processing\n");
        return DA_APP_IN_PROGRESS;
    }

    if (g_zeroconf_local_dns_sd_service != NULL) {
        ZEROCONF_DEBUG_INFO("Already registered.\n");
        return DA_APP_ALREADY_ENABLED;
    }

    if (strlen(name) == 0 || strlen(type) == 0 || strlen(domain) == 0) {
        ZEROCONF_DEBUG_ERR("DNS-SD Name , Type and Domain cannot be blank\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (strcmp(type + strlen(type) - 5, "._tcp") != 0
        && strcmp(type + strlen(type) - 5, "._udp") != 0) {
        ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type(%s)\n", type);
        return DA_APP_INVALID_PARAMETERS;
    }

    if (port <= 0) {
        ZEROCONF_DEBUG_ERR("Invalid Port No.\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (strcmp(domain, ".") == 0) {
        domain = "local";
    } else if (strcmp(domain, "local") != 0) {
        ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (txt_cnt) {
        for (i = 0; i < txt_cnt ; i++) {
            const char *p = txt_rec[i];
            *ptr = 0;

            while (*p && *ptr < 255 && ptr + 1 + *ptr < txt + sizeof(txt)) {
                if (p[0] != '\\' || p[1] == 0) {
                    ptr[++*ptr] = *p;
                    p += 1;
                } else if (p[1] == 'x' && isxdigit((int)p[2]) && isxdigit((int)p[3])) {
                    ptr[++*ptr] = HexPair(p + 2);
                    p += 4;
                } else {
                    ptr[++*ptr] = p[1];
                    p += 2;
                }
            }

            ptr += 1 + *ptr;
        }
    }

    g_zeroconf_local_dns_sd_service = createLocalDnsSd(name, type, domain, port,
                                                       (size_t)(ptr - txt), txt);
    if (g_zeroconf_local_dns_sd_service == NULL) {
        ZEROCONF_DEBUG_ERR("Failed to create dns sd service\n");
        return DA_APP_NOT_CREATED;
    }

    g_zeroconf_dns_sd_query.query_key = 'R';

    g_zeroconf_prob_info.service_key = UNKNOWN_SERVICE_TYPE;
    g_zeroconf_prob_info.skip_prob = skip_prob;

    return zeroconf_start_dns_sd_local_probing_task(&g_zeroconf_prob_info);
}

int zeroconf_unregister_dns_sd_service(void)
{
    if (is_running_dns_sd_probing_processor()) {
        ZEROCONF_DEBUG_INFO("DNS-SD service Probing is processing\n");
        return DA_APP_IN_PROGRESS;
    }

    if (g_zeroconf_local_dns_sd_service) {
        ZEROCONF_DEBUG_INFO("DNS-SD service is stopped\n");
        stop_dns_sd_local_service();
        return DA_APP_SUCCESS;
    } else {
        ZEROCONF_DEBUG_INFO("Not registered dns-sd service.\n");
    }

    return DA_APP_SUCCESS;
}
#endif // (__SUPPORT_DNS_SD__)

void clear_mdns_packet(zeroconf_mdns_packet_t *mdns_packet)
{
    zeroconf_question_t *question = NULL;
    zeroconf_resource_record_t *rrecord = NULL;

    if (mdns_packet) {
        while (mdns_packet->questions) {
            question = mdns_packet->questions;
            mdns_packet->questions = mdns_packet->questions->next;

            if (question->name_bytes_len) {
                zeroconf_free(question->name_bytes);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no name in question\n" CLEAR_COLOR);
            }

            zeroconf_free(question);
        }

        while (mdns_packet->answer_rrs) {
            rrecord = mdns_packet->answer_rrs;
            mdns_packet->answer_rrs = mdns_packet->answer_rrs->next;

            if (rrecord->name_bytes_len) {
                zeroconf_free(rrecord->name_bytes);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no name in answer rr\n" CLEAR_COLOR);
            }

            if (rrecord->data_length) {
                zeroconf_free(rrecord->data);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no data in answer rr\n" CLEAR_COLOR);
            }

            zeroconf_free(rrecord);
        }

        while (mdns_packet->authority_rrs) {
            rrecord = mdns_packet->authority_rrs;
            mdns_packet->authority_rrs = mdns_packet->authority_rrs->next;

            if (rrecord->name_bytes_len) {
                zeroconf_free(rrecord->name_bytes);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no name in authority rr\n" CLEAR_COLOR);
            }

            if (rrecord->data_length) {
                zeroconf_free(rrecord->data);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no data in authority rr\n" CLEAR_COLOR);
            }

            zeroconf_free(rrecord);
        }

        while (mdns_packet->additional_rrs) {
            rrecord = mdns_packet->additional_rrs;
            mdns_packet->additional_rrs = mdns_packet->additional_rrs->next;

            if (rrecord->name_bytes_len) {
                zeroconf_free(rrecord->name_bytes);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no name in additional rr\n" CLEAR_COLOR);
            }

            if (rrecord->data_length) {
                zeroconf_free(rrecord->data);
            } else {
                ZEROCONF_DEBUG_TEMP(YELLOW_COLOR "no data in additional rr\n" CLEAR_COLOR);
            }

            zeroconf_free(rrecord);
        }

        memset(mdns_packet, 0x00, sizeof(zeroconf_mdns_packet_t));
    }

    return;
}

unsigned int parse_mdns_packet_question(unsigned char *payload,
                                        zeroconf_mdns_packet_t *out, unsigned int start)
{
    int count = 0;
    unsigned char *ptr = payload + start;

    unsigned char name_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00, };
    size_t name_bytes_len = 0;

    char name_str[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00, };
    unsigned int name_len = 0;
    unsigned int name_size = 0;

    unsigned short type = 0;
    unsigned short class = 0;

    int ret = DA_APP_SUCCESS;

    ZEROCONF_DEBUG_TEMP("Parsed qeustions(%d)\n", out->questions_count);

    for (count = 0 ; count < out->questions_count ; count++) {
        memset(name_str, 0x00, sizeof(name_str));
        memset(name_bytes, 0x00, sizeof(name_bytes));

        name_len = unencode_mdns_name_string(payload, (ptr - payload), name_str,
                                             ZEROCONF_DOMAIN_NAME_SIZE - 1);
        if (name_len == 0) {
            ZEROCONF_DEBUG_ERR("failed to unencode mdns name\n");
            goto error;
        }

        name_size = calculate_mdns_name_size(ptr);
        if (name_size) {
            ptr += name_size;
        } else {
            ZEROCONF_DEBUG_ERR("failed to get mdns name size\n");
            goto error;
        }

        name_bytes_len = encode_mdns_name_string(name_str, name_bytes,
                         ZEROCONF_DOMAIN_NAME_SIZE);

        type = (*ptr << 8) | *(ptr + 1);
        ptr += 2;

        class = (*ptr << 8) | *(ptr + 1);
        ptr += 2;

        ret = insert_question(&out->questions, name_bytes, name_bytes_len, type, class);
        if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
            ZEROCONF_DEBUG_ERR("failed to create question(0x%x)\n", -ret);
            goto error;
        }

        ZEROCONF_DEBUG_TEMP(">>>>> #%d. Question\n"
                            "\tname\t: %s\n"
                            "\ttype\t: %d\n"
                            "\tclass\t: %d\n",
                            count + 1,
                            name_str,
                            type,
                            class);
    }

    return (ptr - (payload + start));

error:

    return 0;
}

unsigned int parse_mdns_packet_resource_record(unsigned char *payload,
                                               zeroconf_mdns_packet_t *out,
                                               unsigned int start,
                                               unsigned int rr_section)
{
    int count = 0;
    unsigned char *ptr = payload + start;

    unsigned char name_bytes[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00, };
    size_t name_bytes_len = 0;

    char name_str[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00, };
    unsigned int name_len = 0;
    unsigned int name_size = 0;

    unsigned short type = 0;
    unsigned short class = 0;
    unsigned int ttl = 0;
    unsigned short data_length = 0;
    unsigned char *data = NULL;

    unsigned char *data_bytes = NULL;
    size_t data_bytes_len = 0;
    size_t decoded_data_len = 0;

    unsigned short rr_count = 0;
    zeroconf_resource_record_t **rrs = NULL;

    int ret = DA_APP_SUCCESS;

    if (rr_section == ZEROCONF_MDNS_RR_ANSWER_SECTION) {
        rr_count = out->answer_rrs_count;
        rrs = &out->answer_rrs;

        ZEROCONF_DEBUG_TEMP("Parsed answer rr(%d)\n", rr_count);
    } else if (rr_section == ZEROCONF_MDNS_RR_AUTHORITY_SECTION) {
        rr_count = out->authority_rrs_count;
        rrs = &out->authority_rrs;

        ZEROCONF_DEBUG_TEMP("Parsed authority rr(%d)\n", rr_count);
    } else if (rr_section == ZEROCONF_MDNS_RR_ADDITIONAL_SECTION) {
        rr_count = out->additional_rrs_count;
        rrs = &out->additional_rrs;

        ZEROCONF_DEBUG_TEMP("Parsed additional rr(%d)\n", rr_count);
    } else {
        ZEROCONF_DEBUG_ERR("Invalid section(%d)\n", rr_section);
        goto error;
    }

    for (count = 0 ; count < rr_count ; count++) {
        memset(name_str, 0x00, sizeof(name_str));
        memset(name_bytes, 0x00, sizeof(name_bytes));

        if (*ptr == 0) { //opt type
            name_size = 1;
            name_bytes[0] = *ptr;
            name_bytes_len = 1;
        } else {
            name_len = unencode_mdns_name_string(payload,
                                                 (ptr - payload),
                                                 name_str,
                                                 ZEROCONF_DOMAIN_NAME_SIZE - 1);
            if (name_len == 0) {
                ZEROCONF_DEBUG_ERR("failed to unencode mdns name\n");
                ret = DA_APP_INVALID_PACKET;
                goto error;
            }

            name_size = calculate_mdns_name_size(ptr);

            name_bytes_len = encode_mdns_name_string(name_str,
                             name_bytes,
                             ZEROCONF_DOMAIN_NAME_SIZE);
        }

        ptr += name_size;

        type = (*ptr << 8) | *(ptr + 1);
        ptr += 2;

        class = (*ptr << 8) | *(ptr + 1);
        ptr += 2;

        ttl = (*ptr << 24)
              | (*(ptr + 1) << 16)
              | (*(ptr + 2) << 8)
              | (*(ptr + 3));
        ptr += 4;

        data_length = (*ptr << 8) | *(ptr + 1);
        ptr += 2;

        if (type == ZEROCONF_MDNS_RR_TYPE_PTR) {
            //need to parse domain name
            data = zeroconf_calloc(data_length + ZEROCONF_DOMAIN_NAME_SIZE, 1);
            if (data == NULL) {
                ZEROCONF_DEBUG_ERR("[%s:%d]failed to allocate memory for data\n");

                goto error;
            }

            decoded_data_len = unencode_mdns_name_string(payload, (ptr - payload),
                               (char *)data,
                               data_length + ZEROCONF_DOMAIN_NAME_SIZE);
            if (decoded_data_len == 0) {
                ZEROCONF_DEBUG_ERR("failed to parse domain name in ptr\n");

                zeroconf_free(data);
                goto error;
            }

            data_bytes = zeroconf_calloc(decoded_data_len + 2, 1);
            if (data_bytes == NULL) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory for data bytes\n");

                zeroconf_free(data);
                goto error;
            }

            data_bytes_len = encode_mdns_name_string((char *)data, data_bytes,
                             decoded_data_len + 2);

            ret = insert_resource_record(rrs, name_bytes, name_bytes_len,
                                         type, class, ttl, data_bytes_len, data_bytes);

            zeroconf_free(data);
            zeroconf_free(data_bytes);

            data = NULL;
            data_bytes = NULL;
        } else {
            data = ptr;

            ret = insert_resource_record(rrs, name_bytes, name_bytes_len,
                                         type, class, ttl, data_length, data);
        }

        if ((ret != DA_APP_SUCCESS) && (ret != DA_APP_DUPLICATED_ENTRY)) {
            ZEROCONF_DEBUG_ERR("Failed to create rr(0x%x)\n", -ret);
            goto error;
        }

        ptr += data_length;

        ZEROCONF_DEBUG_TEMP(">>>>> #%d. Resource Record\n"
                            "\tname\t\t: %s\n"
                            "\ttype\t\t: %d\n"
                            "\tclass\t\t: %d\n"
                            "\tttl\t\t: %d\n"
                            "\tdata len\t: %d\n",
                            count + 1,
                            name_str,
                            type,
                            class,
                            ttl,
                            data_length);
    }

    return (ptr - (payload + start));

error:

    return 0;
}

int parse_mdns_packet(unsigned char *payload, zeroconf_mdns_packet_t *out)
{
    unsigned char *ptr = NULL;
    unsigned int parsed_data_length = 0;

    if (payload == NULL || out == NULL) {
        return DA_APP_INVALID_PARAMETERS;
    }

    ptr = payload;

    out->transaction_id = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    out->flags = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    out->questions_count = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    out->answer_rrs_count = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    out->authority_rrs_count = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    out->additional_rrs_count = (*ptr << 8) | *(ptr + 1);
    ptr += 2;

    ZEROCONF_DEBUG_INFO("Received mdns packet\n"
                        ">>>>> Transaction id\t: %d\n"
                        ">>>>> Flags\t\t: 0x%x\n"
                        ">>>>> Questions\t\t: %d\n"
                        ">>>>> Answer RRs\t: %d\n"
                        ">>>>> Authority RRs\t: %d\n"
                        ">>>>> Additional RRs\t: %d\n",
                        out->transaction_id,
                        out->flags,
                        out->questions_count,
                        out->answer_rrs_count,
                        out->authority_rrs_count,
                        out->additional_rrs_count);

    if ((out->questions_count == 0)
        && (out->answer_rrs_count == 0)
        && (out->authority_rrs_count == 0)
        && (out->additional_rrs_count == 0)) {
        ZEROCONF_DEBUG_INFO("Empty payload in mdns packet\n");
        goto error;
    }

    if (out->questions_count) {
        parsed_data_length = parse_mdns_packet_question(payload, out, (ptr - payload));
        if (parsed_data_length == 0) {
            ZEROCONF_DEBUG_ERR("Failed to parse question\n");
            goto error;
        }

        ptr += parsed_data_length;
    }

    if (out->answer_rrs_count) {
        parsed_data_length = parse_mdns_packet_resource_record(payload, out,
                             (ptr - payload),
                             ZEROCONF_MDNS_RR_ANSWER_SECTION);

        if (parsed_data_length == 0) {
            ZEROCONF_DEBUG_ERR("failed to parse answer rrs\n");
            goto error;
        }

        ptr += parsed_data_length;
    }

    if (out->authority_rrs_count) {
        parsed_data_length = parse_mdns_packet_resource_record(payload, out,
                             (ptr - payload),
                             ZEROCONF_MDNS_RR_AUTHORITY_SECTION);

        if (parsed_data_length == 0) {
            ZEROCONF_DEBUG_ERR("Failed to parse authority rrs\n");
            goto error;
        }

        ptr += parsed_data_length;
    }

    if (out->additional_rrs_count) {
        parsed_data_length = parse_mdns_packet_resource_record(payload, out,
                             (ptr - payload),
                             ZEROCONF_MDNS_RR_ADDITIONAL_SECTION);

        if (parsed_data_length == 0) {
            ZEROCONF_DEBUG_ERR("Failed to parse additional rrs\n");
            goto error;
        }

        ptr += parsed_data_length;
    }

    return DA_APP_SUCCESS;

error:

    clear_mdns_packet(out);

    return DA_APP_NOT_SUCCESSFUL;
}

unsigned int encode_mdns_name_string(char *name, unsigned char *ptr, size_t size)
{
    unsigned char *length;
    unsigned int count = 1;

    length = ptr++;

    *length = 0;

    while (*name) {
        if (*name == '.') {
            length = ptr++;
            *length = 0;
            name++;
        } else {
            *ptr++ = *name++;
            (*length)++;
        }

        count++;

        if (count >= size) {
            ZEROCONF_DEBUG_ERR("Not enough buffer size\n");
            return 0;
        }
    }

    *ptr = 0;

    count++;

    return count;
}

unsigned int unencode_mdns_name_string(unsigned char *payload, unsigned int start,
                                       char *buffer, size_t size)
{
    unsigned char *character = payload + start;
    unsigned int length = 0;

    while (*character != '\0') {
        if (size > length) {
            unsigned int  labelSize =  *character++;

            if (labelSize <= 63) {
                while ((size > length) && (labelSize > 0)) {

                    *buffer++ =  *character++;
                    length++;
                    labelSize--;
                }

                *buffer++ =  '.';
                length++;
            } else if ((labelSize & ZEROCONF_MDNS_COMPRESS_MASK) ==
                       ZEROCONF_MDNS_COMPRESS_VALUE) {
                character =  payload + ((labelSize & 63) << 8) + *character;
            } else {
                ZEROCONF_DEBUG_ERR("not defined(%d)\n", labelSize);
                return 0;
            }
        } else {
            ZEROCONF_DEBUG_ERR("Not enough buffer size(len:%d)\n", length);
            return 0;
        }
    }

    if (*(buffer - 1) == '.') {
        buffer--;
        length --;
    }

    *buffer = '\0';

    return length;
}

unsigned int calculate_mdns_name_size(unsigned char *name)
{
    unsigned int size = 0;

    while (*name != '\0') {
        unsigned int labelSize = *name++;

        if (labelSize <= 63) {
            size += labelSize + 1;
            name += labelSize;
        } else if ((labelSize & ZEROCONF_MDNS_COMPRESS_MASK) ==
                   ZEROCONF_MDNS_COMPRESS_VALUE) {
            return (size + 2);
        } else {
            return 0;
        }
    }

    return (size + 1);
}

int parse_mdns_command(int argc, char *argv[],
                       zeroconf_service_type_t service_key, int *iface, char *hostname)
{
    int index = 1;
    char **cur_argv = ++argv;

    const char *top_domain = ((service_key == MDNS_SERVICE_TYPE) ?
                              mdns_top_domain : xmdns_top_domain);

    for (index = 1 ; index < argc ; index++, cur_argv++) {
        if (strcmp(*cur_argv, "-i") == 0) {
            if (--argc < 1) {
                return DA_APP_INVALID_PARAMETERS;
            }

            ++cur_argv;

            if (strcasecmp(*cur_argv, "WLAN0") == 0) {
                *iface = WLAN0_IFACE;
            } else if (strcasecmp(*cur_argv, "WLAN1") == 0) {
                *iface = WLAN1_IFACE;
            } else {
                return DA_APP_INVALID_INTERFACE;
            }
        } else if ((strcmp(*cur_argv, "-help") == 0)
                   || (strcmp(*cur_argv, "-h") == 0)
                   || (strcmp(*cur_argv, "?") == 0)) {

            return DA_APP_NOT_IMPLEMENTED;
        } else {
            strcpy(hostname, *cur_argv);
            to_lower(hostname);

            if ((strlen(hostname) < 1) || (strstr(hostname, top_domain) != NULL)) {
                return DA_APP_INVALID_PARAMETERS;
            }
        }
    }

    return 0;
}

int dpm_set_zeroconf_sleep(zeroconf_dpm_type_t type)
{
    if (!dpm_mode_is_enabled()) {
        return 0;
    }

    switch (type) {
        case MDNS_DPM_TYPE:
            zero_dpm_state.mdns = pdTRUE;
            break;
        case DNS_SD_DPM_TYPE:
            zero_dpm_state.dns_sd = pdTRUE;
            break;
        case XMDNS_DPM_TYPE:
            zero_dpm_state.xmdns = pdTRUE;
            break;
        case SENDER_DPM_TYPE:
            zero_dpm_state.sender = pdTRUE;
            break;
        case RESPONDER_DPM_TYPE:
            zero_dpm_state.responder = pdTRUE;
            break;
        default:
            return -1;
    }

    ZEROCONF_DEBUG_TEMP("set dpm sleep for type(%d)\n"
                        ">>> 1. mdns(%d), 2. dns-sd(%d), 3. xmdns(%d), "
                        "4. sender(%d), 5. responder(%d)\n",
                        type,
                        zero_dpm_state.mdns,
                        zero_dpm_state.dns_sd,
                        zero_dpm_state.xmdns,
                        zero_dpm_state.sender,
                        zero_dpm_state.responder);

    if (zero_dpm_state.mdns
        && zero_dpm_state.dns_sd
        && zero_dpm_state.xmdns
        && zero_dpm_state.sender
        && zero_dpm_state.responder) {
        dpm_app_sleep_ready_set(APP_ZEROCONF);
    }

    return 0;
}

int dpm_clr_zeroconf_sleep(zeroconf_dpm_type_t type)
{
    if (!dpm_mode_is_enabled()) {
        return 0;
    }

    switch (type) {
        case MDNS_DPM_TYPE:
            zero_dpm_state.mdns = pdFALSE;
            break;
        case DNS_SD_DPM_TYPE:
            zero_dpm_state.dns_sd = pdFALSE;
            break;
        case XMDNS_DPM_TYPE:
            zero_dpm_state.xmdns = pdFALSE;
            break;
        case SENDER_DPM_TYPE:
            zero_dpm_state.sender = pdFALSE;
            break;
        case RESPONDER_DPM_TYPE:
            zero_dpm_state.responder = pdFALSE;
            break;
        default:
            return -1;
    }

    ZEROCONF_DEBUG_TEMP("clear dpm sleep for type(%d)\n"
                        ">>> 1. mdns(%d), 2. dns-sd(%d), 3. xmdns(%d), "
                        "4. sender(%d), 5. responder(%d)\n",
                        type,
                        zero_dpm_state.mdns,
                        zero_dpm_state.dns_sd,
                        zero_dpm_state.xmdns,
                        zero_dpm_state.sender,
                        zero_dpm_state.responder);

    dpm_app_sleep_ready_clear(APP_ZEROCONF);

    return 0;
}

#if defined (__SUPPORT_DNS_SD__)
static int dpm_set_local_dns_sd_info(zeroconf_local_dns_sd_info_t *dns_sd_service)
{
    unsigned int status = ERR_OK;
    const unsigned long wait_option = 100;

    unsigned int len = 0;
    zeroconf_local_dns_sd_dpm_t *data = NULL;

    ZEROCONF_DEBUG_INFO("Start\n");

    if (!dns_sd_service->name
            || !dns_sd_service->type
            || !dns_sd_service->domain
            || !dns_sd_service->txtLen
            || !dns_sd_service->txtRecord) {
        ZEROCONF_DEBUG_ERR("Invalid parameters\n");
        return DA_APP_NOT_SUCCESSFUL;
    }

    len = dpm_user_mem_get(ZEROCONF_DPM_DNS_SD_INFO, (unsigned char **)&data);
    if (len == 0) {
        ZEROCONF_DEBUG_INFO("Not found. New allocation for DNS-SD service(%s)\n",
                            ZEROCONF_DPM_DNS_SD_INFO);

        status = dpm_user_mem_alloc(ZEROCONF_DPM_DNS_SD_INFO,
                                    (void **)&data,
                                    sizeof(zeroconf_local_dns_sd_dpm_t),
                                    wait_option);
        if (status != ERR_OK) {
            ZEROCONF_DEBUG_ERR("failed to allocate memory in rtm(0x%02x)\n", status);
            return DA_APP_NOT_CREATED;
        }
    } else if (len != sizeof(zeroconf_local_dns_sd_dpm_t)) {
        ZEROCONF_DEBUG_ERR("Invalid size(%ld)\n", len);
        return DA_APP_NOT_SUCCESSFUL;
    }

    memset(data, 0x00, sizeof(zeroconf_local_dns_sd_dpm_t));

    strcpy(data->name, dns_sd_service->name);
    data->name_len = strlen(dns_sd_service->name);

    strcpy(data->type, dns_sd_service->type);
    data->type_len = strlen(dns_sd_service->type);

    strcpy(data->domain, dns_sd_service->domain);
    data->domain_len = strlen(dns_sd_service->domain);

    data->port = dns_sd_service->port;

    memcpy(data->txt_record, dns_sd_service->txtRecord, dns_sd_service->txtLen);
    data->txt_len = dns_sd_service->txtLen;

    return DA_APP_SUCCESS;
}

zeroconf_local_dns_sd_info_t *dpm_get_local_dns_sd_info(void)
{
    zeroconf_local_dns_sd_info_t *data = NULL;
    zeroconf_local_dns_sd_dpm_t *data_dpm = NULL;
    unsigned int len = 0;

    len = dpm_user_mem_get(ZEROCONF_DPM_DNS_SD_INFO, (unsigned char **)&data_dpm);
    if (len == 0 || len != sizeof(zeroconf_local_dns_sd_dpm_t)) {
        ZEROCONF_DEBUG_ERR("Invalid dns-sd info in rtm(%ld)\n", len);
        return NULL;
    }

    data = createLocalDnsSd(data_dpm->name, data_dpm->type, data_dpm->domain,
                            data_dpm->port, data_dpm->txt_len, data_dpm->txt_record);
    if (data == NULL) {
        ZEROCONF_DEBUG_ERR("failed to restore dns-sd info\n");
    }

    return data;
}

int dpm_clear_local_dns_sd_info(void)
{
    unsigned int status = 0;

    ZEROCONF_DEBUG_INFO("Start\n");

    status = dpm_user_mem_free(ZEROCONF_DPM_DNS_SD_INFO);
    if (status) {
        ZEROCONF_DEBUG_ERR("Failed to clear dns-sd info(0x%x)\n", status);
        return DA_APP_NOT_SUCCESSFUL;
    }

    return DA_APP_SUCCESS;
}

int zeroconf_lookup_dns_sd(char *name, char *type, char *domain,
                           unsigned long interval, unsigned long wait_option,
                           unsigned int *port, char *txt[], size_t *txt_len)
{
    int ret = DA_APP_SUCCESS;

    char raw_query[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    unsigned char enc_query[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t enc_query_len = 0;

    unsigned char req_offset[3] = {192, 12, 0};
    unsigned char req_query[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    size_t req_query_len = 0;

    zeroconf_mdns_response_t *resp = NULL;

    unsigned int is_found_srv = pdFALSE;
    unsigned int is_found_txt = pdFALSE;

    unsigned long cur_wait_option = 0;

    int txt_idx = 0;
    unsigned char *txt_pos = NULL;
    unsigned char a_txt_len = 0;

    if (!name || !type || !domain) {
        ZEROCONF_DEBUG_ERR("Name, Type and Domain cannot be NULL\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if ((strlen(type) == 0) || (strlen(name) == 0) || (strlen(domain) == 0)) {
        ZEROCONF_DEBUG_ERR("Name, Type and Domain cannot be blank\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if ((strcmp(type + strlen(type) - 5, "._tcp") != 0)
        && (strcmp(type + strlen(type) - 5, "._udp") != 0)) {
        ZEROCONF_DEBUG_ERR("Invalid Protocol in Service Type\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (strcmp(domain, "local") != 0) {
        ZEROCONF_DEBUG_ERR("Only '.local' domain is supported\n");
        return DA_APP_INVALID_PARAMETERS;
    }

    if (sizeof(raw_query) < strlen(name) + strlen(type) + strlen(domain) + 4) {
        ZEROCONF_DEBUG_ERR("Buffer is not enough(%ld:%ld)\n",
                           sizeof(enc_query), strlen(raw_query));
        return DA_APP_NOT_SUCCESSFUL;
    }

    sprintf(raw_query, "%s.%s.%s", name, type, domain);

    if (sizeof(enc_query) <= strlen(raw_query)) {
        ZEROCONF_DEBUG_ERR("Buffer is not enough(%ld:%ld)\n",
                           sizeof(enc_query), strlen(raw_query));
        return DA_APP_NOT_SUCCESSFUL;
    }

    enc_query_len = encode_mdns_name_string(raw_query, enc_query,
                                            sizeof(enc_query));

    if (enc_query_len == 0) {
        ZEROCONF_DEBUG_ERR("Failed to make Query\n");
        return DA_APP_NOT_SUCCESSFUL;
    }

    insertQueryString(enc_query, enc_query_len);

    // Generate Request Header
    create_packet_header(3, 0, 0, 0, 1, req_query, 0);
    req_query_len = ZEROCONF_MDNS_HEADER_SIZE;

    // SRV
    req_query_len += create_query(enc_query, enc_query_len, ZEROCONF_MDNS_RR_TYPE_SRV, 1, req_query, req_query_len);

    // PTR
    req_query_len += create_query(req_offset, 2, ZEROCONF_MDNS_RR_TYPE_PTR, 1, req_query, req_query_len);

    // TXT
    req_query_len += create_query(enc_query, enc_query_len, ZEROCONF_MDNS_RR_TYPE_TXT, 1, req_query, req_query_len);

    do {
        send_mdns_packet(MDNS_SERVICE_TYPE, req_query, req_query_len);

        for (resp = g_zeroconf_mdns_response_root ; resp != NULL ; resp = resp->next) {
            //Find SRV
            if (!is_found_srv && resp->type == ZEROCONF_MDNS_RR_TYPE_SRV) {
                is_found_srv = search_in_byte_array(resp->query, resp->query_len, enc_query, enc_query_len);

                if (is_found_srv) {
                    ZEROCONF_DEBUG_INFO("Found SRV\n");
                    *port = (resp->data[4] << 8) | resp->data[5];
                }
            }

            //Find TXT
            if (!is_found_txt && resp->type == ZEROCONF_MDNS_RR_TYPE_TXT) {
                is_found_txt = search_in_byte_array(resp->query, resp->query_len, enc_query, enc_query_len);

                if (is_found_txt) {
                    ZEROCONF_DEBUG_INFO("Found TXT\n");
                    txt_pos  = resp->data;

                    while (txt_pos - resp->data < (int)resp->data_len) {
                        if (txt_idx < (int)*txt_len) {
                            a_txt_len = *txt_pos++;
                            memcpy(txt[txt_idx], txt_pos, a_txt_len);
                            txt[txt_idx][a_txt_len] = '\0';
                            txt_pos += a_txt_len;
                            txt_idx++;
                        } else {
                            break;
                        }
                    }

                    *txt_len = txt_idx;
                }
            }
        }

        if (is_found_srv && is_found_txt) {
            ZEROCONF_DEBUG_INFO("\tPort: %d\n", *port);

            for (int idx = 0 ; idx < txt_idx ; idx++) {
                ZEROCONF_DEBUG_INFO("\t%d. TXT: %s\n", idx + 1, txt[idx]);
            }

            break;
        }

        if (wait_option < cur_wait_option) {
            if (!is_found_srv) {
                ZEROCONF_DEBUG_INFO("Not Found port\n");
                *port = 0;
            }

            if (!is_found_txt) {
                ZEROCONF_DEBUG_INFO("Not Found txt\n");
                *txt_len = 0;
            }

            if (!is_found_srv && !is_found_txt) {
                ret = DA_APP_NOT_SUCCESSFUL;
            } else {
                ret = DA_APP_SUCCESS;
            }

            break;
        }

        cur_wait_option += interval;

        vTaskDelay(interval);
    } while (pdTRUE);

    deleteQueryString(enc_query, enc_query_len);

    return ret;
}
#endif // (__SUPPORT_DNS_SD__)

int zeroconf_set_mdns_reg_to_nvram(int value)
{
    ZEROCONF_DEBUG_INFO("- mDNS service is%senabled.\n", value ? " " : " not ");

    if (da16x_set_config_int(DA16X_CONF_INT_ZEROCONF_MDNS_REG, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_mdns_reg_from_nvram(int *out)
{
    if (da16x_get_config_int(DA16X_CONF_INT_ZEROCONF_MDNS_REG, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_set_mdns_hostname_to_nvram(char *value)
{
    ZEROCONF_DEBUG_INFO("- mDNS hostname : %s(len:%ld)\n", value, strlen(value));

    if (da16x_set_nvcache_str(DA16X_CONF_STR_ZEROCONF_MDNS_HOSTNAME, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_mdns_hostname_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(outlen);

    if (da16x_get_config_str(DA16X_CONF_STR_ZEROCONF_MDNS_HOSTNAME, out)) {
        return -1;
    }

    return 0;
}

#if defined (__SUPPORT_DNS_SD__)
int zeroconf_set_dns_sd_srv_name_to_nvram(char *value)
{
    ZEROCONF_DEBUG_INFO("- DNS-SD Service Name : %s(len:%ld)\n", value, strlen(value));

    if (da16x_set_nvcache_str(DA16X_CONF_STR_ZEROCONF_SRV_NAME, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_dns_sd_srv_name_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(outlen);

    if (da16x_get_config_str(DA16X_CONF_STR_ZEROCONF_SRV_NAME, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_set_dns_sd_srv_protocol_to_nvram(char *value)
{
    ZEROCONF_DEBUG_INFO("- DNS-SD Service Protocoal: %s(len:%ld)\n", value, strlen(value));

    if (da16x_set_nvcache_str(DA16X_CONF_STR_ZEROCONF_SRV_PROT, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_dns_sd_srv_protocol_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(outlen);

    if (da16x_get_config_str(DA16X_CONF_STR_ZEROCONF_SRV_PROT, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_set_dns_sd_srv_txt_to_nvram(char *value)
{
    ZEROCONF_DEBUG_INFO("- DNS-SD Service TXT: %s(len:%ld)\n", value, strlen(value));

    if (da16x_set_nvcache_str(DA16X_CONF_STR_ZEROCONF_SRV_TXT, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_dns_sd_srv_txt_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(outlen);

    if (da16x_get_config_str(DA16X_CONF_STR_ZEROCONF_SRV_TXT, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_set_dns_sd_srv_port_to_nvram(int value)
{
    ZEROCONF_DEBUG_INFO("- DNS-SD Service Port: %d\n", value);

    if (da16x_set_config_int(DA16X_CONF_INT_ZEROCONF_SRV_PORT, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_dns_sd_srv_port_from_nvram(int *out)
{
    if (da16x_get_config_int(DA16X_CONF_INT_ZEROCONF_SRV_PORT, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_set_dns_sd_srv_reg_to_nvram(int value)
{
    ZEROCONF_DEBUG_INFO("- DNS-SD Service%senabled.\n", value ? " " : " not ");

    if (da16x_set_config_int(DA16X_CONF_INT_ZEROCONF_SRV_REG, value)) {
        return -1;
    }

    return 0;
}

int zeroconf_get_dns_sd_srv_reg_from_nvram(int *out)
{

    if (da16x_get_config_int(DA16X_CONF_INT_ZEROCONF_SRV_REG, out)) {
        return -1;
    }

    return 0;
}

int zeroconf_register_dns_sd_service_by_nvram(void)
{
    int status = DA_APP_SUCCESS;
    int ret = 0;

    char *name = NULL;
    char *protocol = NULL;
    char *domain = NULL;
    int port = 0;
    char *raw_txt = NULL;

    int idx = 0;
    int txt_cnt = 0;
    char **txt_rec = NULL;
    const int max_txt_cnt = 10;

    name = zeroconf_calloc(ZEROCONF_DNS_SD_SRV_NAME_LEN, 1);
    if (!name) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory(%ld)\n", ZEROCONF_DNS_SD_SRV_NAME_LEN);
        status = DA_APP_MALLOC_ERROR;
        goto end;
    }

    protocol = zeroconf_calloc(ZEROCONF_DNS_SD_SRV_PROT_LEN, 1);
    if (!protocol) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory(%ld)\n", ZEROCONF_DNS_SD_SRV_PROT_LEN);
        status = DA_APP_MALLOC_ERROR;
        goto end;
    }

    domain = zeroconf_calloc(7, 1);
    if (!domain) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory(%ld)\n", 6);
        status = DA_APP_MALLOC_ERROR;
        goto end;
    }

    raw_txt = zeroconf_calloc(ZEROCONF_DNS_SD_SRV_TXT_LEN, 1);
    if (!raw_txt) {
        ZEROCONF_DEBUG_ERR("Failed to allocate memory(%ld)\n", ZEROCONF_DNS_SD_SRV_TXT_LEN);
        status = DA_APP_MALLOC_ERROR;
        goto end;
    }

    // Set service name
    ret = zeroconf_get_dns_sd_srv_name_from_nvram(name, ZEROCONF_DNS_SD_SRV_NAME_LEN);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to get service name(%d)\n", ret);
        status = DA_APP_ENTRY_NOT_FOUND;
        goto end;
    }

    // Set protocol
    ret = zeroconf_get_dns_sd_srv_protocol_from_nvram(protocol, ZEROCONF_DNS_SD_SRV_PROT_LEN);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to get protocol(%d)\n", ret);
        status = DA_APP_ENTRY_NOT_FOUND;
        goto end;
    }

    // Set domain
    strcpy(domain, "local");

    // Set port
    ret = zeroconf_get_dns_sd_srv_port_from_nvram(&port);
    if (ret) {
        ZEROCONF_DEBUG_ERR("Failed to get port(%d)\n", ret);
        status = DA_APP_ENTRY_NOT_FOUND;
        goto end;
    }

    // Get txt
    ret = zeroconf_get_dns_sd_srv_txt_from_nvram(raw_txt, ZEROCONF_DNS_SD_SRV_TXT_LEN);
    if (ret == 0 && strlen(raw_txt) > 0) {
        char *txt = NULL;
        size_t txt_len = 0;
        const char *delimiter = ",";

        //register & set txt
        txt_rec = zeroconf_calloc(max_txt_cnt, sizeof(char *));
        if (!txt_rec) {
            ZEROCONF_DEBUG_ERR("Failed to allocate memory(%d)\n",
                               (max_txt_cnt * sizeof(char *)));
            status = DA_APP_MALLOC_ERROR;
            goto end;
        }

        idx = 0;
        txt = strtok(raw_txt, delimiter);
        while (txt) {
            txt_len = strlen(txt);

            txt_rec[idx] = zeroconf_calloc(txt_len + 1, sizeof(char));
            if (!txt_rec[idx]) {
                ZEROCONF_DEBUG_ERR("Failed to allocate memory(%d)\n", txt_len + 1);
                status = DA_APP_MALLOC_ERROR;
                goto end;
            }

            strcpy(txt_rec[idx], txt);
            txt_rec[idx][txt_len] = '\0';

            idx++;
            txt_cnt++;
            txt = strtok(NULL, delimiter);
        }
    }

    // Register service
    status = zeroconf_register_dns_sd_service(name, protocol, domain, port, txt_cnt, txt_rec, pdFALSE);
    if (status) {
        ZEROCONF_DEBUG_ERR("Failed to register service(0x%x)\r\n", status);
        goto end;
    }

end:

    if (name) {
        zeroconf_free(name);
    }

    if (protocol) {
        zeroconf_free(protocol);
    }

    if (domain) {
        zeroconf_free(domain);
    }

    if (raw_txt) {
        zeroconf_free(raw_txt);
    }

    if (txt_rec) {
        for (idx = 0 ; idx < txt_cnt ; idx++) {
            zeroconf_free(txt_rec[idx]);
        }

        zeroconf_free(txt_rec);
    }

    return status;

}
#endif // (__SUPPORT_DNS_SD__)

#if defined ( __ENABLE_AUTO_START_ZEROCONF__ )
void zeroconf_auto_start_reg_services(void *params)
{
    int sys_wdog_id = -1;
    int status = DA_APP_SUCCESS;

    int ret = 0;
    int mdns_reg = pdFALSE;
#if defined (__SUPPORT_DNS_SD__)
    int dnssd_reg = pdFALSE;
#endif // (__SUPPORT_DNS_SD__)

    int iface = interface_select;
    char hostname[ZEROCONF_DOMAIN_NAME_SIZE] = {0x00,};
    int hostname_len = 0;

    int retry = 0;
    const int max_retry = 100;
    int wait_time = 10;

    unsigned int skip_prob = pdFALSE;

    DA16X_UNUSED_ARG(params);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    zeroconf_get_mdns_reg_from_nvram(&mdns_reg);
    if (mdns_reg) {
        dpm_clr_zeroconf_sleep(MDNS_DPM_TYPE);
    }

#if defined (__SUPPORT_DNS_SD__)
    zeroconf_get_dns_sd_srv_reg_from_nvram(&dnssd_reg);
    if (dnssd_reg) {
        dpm_clr_zeroconf_sleep(DNS_SD_DPM_TYPE);
    }
#endif // (__SUPPORT_DNS_SD__)

    //is enabled mdns service?
    if (mdns_reg) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        if (dpm_mode_is_enabled()) {
            hostname_len = dpm_get_mdns_info(hostname);
            if (hostname_len <= 0) {
                ret = zeroconf_get_mdns_hostname_from_nvram(hostname, sizeof(hostname));
                if (ret) {
                    ZEROCONF_DEBUG_ERR("Failed to read mdns hostname\n");
                    status = DA_APP_NOT_SUCCESSFUL;
                    goto error;
                }
            } else {
                skip_prob = pdTRUE;
            }
        } else {
            ret = zeroconf_get_mdns_hostname_from_nvram(hostname, sizeof(hostname));
            if (ret) {
                ZEROCONF_DEBUG_ERR("Failed to read mdns hostname\n");
                status = DA_APP_NOT_SUCCESSFUL;
                goto error;
            }
        }

        ZEROCONF_DEBUG_INFO("mDNS host name:%s, interface:%d\n", hostname, iface);

        if (!skip_prob) {
            dpm_app_data_rcv_ready_set(APP_ZEROCONF);
        }

        da16x_sys_watchdog_suspend(sys_wdog_id);

        //Start mdns service
        status = zeroconf_start_mdns_service(iface, hostname, skip_prob);
        if (status) {
            ZEROCONF_DEBUG_ERR("Failed to start mdns(0x%x)\n", status);
            goto error;
        }

        //wait for prob & announcement
        while (!zeroconf_is_running_mdns_service() && (retry++ < max_retry)) {
            vTaskDelay(wait_time);
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (!zeroconf_is_running_mdns_service()) {
            goto error;
        }

#if defined (__SUPPORT_DNS_SD__)
        //is enabled dns-sd service register?
        if (dnssd_reg) {
            da16x_sys_watchdog_suspend(sys_wdog_id);

            //Start to register dns-sd service
            if (dpm_mode_is_enabled()) {
                g_zeroconf_local_dns_sd_service = dpm_get_local_dns_sd_info();
                if (g_zeroconf_local_dns_sd_service) {
                    g_zeroconf_dns_sd_query.query_key = 'R';

                    skip_prob = pdTRUE;

                    g_zeroconf_prob_info.service_key = UNKNOWN_SERVICE_TYPE;
                    g_zeroconf_prob_info.skip_prob = skip_prob;

                    zeroconf_start_dns_sd_local_probing_task(&g_zeroconf_prob_info);
                } else {
                    dpm_app_data_rcv_ready_set(APP_ZEROCONF);

                    status = zeroconf_register_dns_sd_service_by_nvram();
                    if (status) {
                        ZEROCONF_DEBUG_ERR("Failed to register dns-sd service(0x%x)\n", status);
                        goto error;
                    }
                }
            } else {
                dpm_app_data_rcv_ready_set(APP_ZEROCONF);

                status = zeroconf_register_dns_sd_service_by_nvram();
                if (status) {
                    ZEROCONF_DEBUG_ERR("Failed to register dns-sd service(0x%x)\n", status);
                    goto error;
                }
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
#endif // (__SUPPORT_DNS_SD__)
    } else {
        dpm_app_sleep_ready_set(APP_ZEROCONF);
    }

    dpm_app_data_rcv_ready_set(APP_ZEROCONF);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;

error:

    dpm_app_data_rcv_ready_set(APP_ZEROCONF);

    dpm_app_sleep_ready_set(APP_ZEROCONF);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;
}
#endif    // (__ENABLE_AUTO_START_ZEROCONF__)

#else
#include "da16x_types.h"

int zeroconf_start_mdns_service_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_start_mdns_service(int interface, char *hostname, unsigned int skip_prob)
{
    DA16X_UNUSED_ARG(interface);
    DA16X_UNUSED_ARG(hostname);
    DA16X_UNUSED_ARG(skip_prob);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_stop_mdns_service(void)
{
    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_set_mdns_config_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_print_status_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

void zeroconf_print_cache_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return;
}

int zeroconf_lookup_mdns_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_change_mdns_hostname_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

unsigned int zeroconf_get_mdns_hostname(char *out, size_t len)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(len);

    return 0;
}

unsigned int zeroconf_is_running_mdns_service(void)
{
    return pdFALSE;
}

unsigned int zeroconf_is_running_dns_sd_service(void)
{
    return pdFALSE;
}

unsigned int zeroconf_is_running_xmdns_service(void)
{
    return pdFALSE;
}

void zeroconf_init_mdns_prob_timeout()
{
    return;
}

void zeroconf_set_mdns_prob_timeout(unsigned int value)
{
    DA16X_UNUSED_ARG(value);

    return;
}

void zeroconf_set_mdns_prob_tie_breaker_timeout(unsigned int value)
{
    DA16X_UNUSED_ARG(value);

    return;
}

void zeroconf_set_mdns_prob_conflict_timeout(unsigned int value)
{
    DA16X_UNUSED_ARG(value);

    return;
}

unsigned int zeroconf_get_mdns_prob_timeout(void)
{
    return 0;
}

unsigned int zeroconf_get_mdns_prob_tie_breaker_timeout(void)
{
    return 0;
}

unsigned int zeroconf_get_mdns_prob_conflict_timeout(void)
{
    return 0;
}

int zeroconf_exec_dns_sd_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

#if defined (__SUPPORT_DNS_SD__)
int zeroconf_register_dns_sd_service(char *name, char *type,
                                     char *domain, int port,
                                     int txt_cnt, char **txt_rec,
                                     unsigned int skip_prob)
{
    DA16X_UNUSED_ARG(name);
    DA16X_UNUSED_ARG(type);
    DA16X_UNUSED_ARG(domain);
    DA16X_UNUSED_ARG(port);
    DA16X_UNUSED_ARG(txt_cnt);
    DA16X_UNUSED_ARG(txt_rec);
    DA16X_UNUSED_ARG(skip_prob);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_unregister_dns_sd_service(void)
{
    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_lookup_dns_sd(char *name, char *type, char *domain,
                            unsigned long interval, unsigned long wait_option,
                            unsigned int *port, char *txt[], size_t *txt_len)
{
    DA16X_UNUSED_ARG(name);
    DA16X_UNUSED_ARG(type);
    DA16X_UNUSED_ARG(domain);
    DA16X_UNUSED_ARG(interval);
    DA16X_UNUSED_ARG(wait_option);
    DA16X_UNUSED_ARG(port);
    DA16X_UNUSED_ARG(txt);
    DA16X_UNUSED_ARG(txt_len);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_get_dns_sd_service_name(char *out, size_t len)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(len);

    return DA_APP_NOT_SUPPORTED;
}
#endif // (__SUPPORT_DNS_SD__)

#if defined (__SUPPORT_XMDNS__)
int zeroconf_start_xmdns_service(int interface, char *hostname, unsigned int skip_prob)
{
    DA16X_UNUSED_ARG(interface);
    DA16X_UNUSED_ARG(hostname);
    DA16X_UNUSED_ARG(skip_prob);

    return DA_APP_NOT_SUPPORTED;
}
#endif // (__SUPPORT_XMDNS__)

int zeroconf_change_xmdns_hostname_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

int zeroconf_lookup_xmdns_cmd(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return DA_APP_NOT_SUPPORTED;
}

void zeroconf_restart_all_service(void)
{
    return;
}

void zeroconf_stop_all_service(void)
{
    return;
}

unsigned long zeroconf_get_mdns_ipv4_address_by_name(char *addr, unsigned long wait_option)
{
    DA16X_UNUSED_ARG(addr);
    DA16X_UNUSED_ARG(wait_option);

    return 0;
}

int zeroconf_set_mdns_reg_to_nvram(int value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_mdns_reg_from_nvram(int *out)
{
    DA16X_UNUSED_ARG(out);

    return -1;
}

int zeroconf_set_mdns_hostname_to_nvram(char *value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_mdns_hostname_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(outlen);

    return -1;
}

#if defined (__SUPPORT_DNS_SD__)
int zeroconf_set_dns_sd_srv_name_to_nvram(char *value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_dns_sd_srv_name_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(outlen);

    return -1;
}

int zeroconf_set_dns_sd_srv_protocol_to_nvram(char *value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_dns_sd_srv_protocol_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(outlen);

    return -1;
}

int zeroconf_set_dns_sd_srv_txt_to_nvram(char *value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_dns_sd_srv_txt_from_nvram(char *out, size_t outlen)
{
    DA16X_UNUSED_ARG(out);
    DA16X_UNUSED_ARG(outlen);

    return -1;
}

int zeroconf_set_dns_sd_srv_port_to_nvram(int value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_dns_sd_srv_port_from_nvram(int *out)
{
    DA16X_UNUSED_ARG(out);

    return -1;
}

int zeroconf_set_dns_sd_srv_reg_to_nvram(int value)
{
    DA16X_UNUSED_ARG(value);

    return -1;
}

int zeroconf_get_dns_sd_srv_reg_from_nvram(int *out)
{
    DA16X_UNUSED_ARG(out);

    return -1;
}
#endif // (__SUPPORT_DNS_SD__)

#if defined ( __ENABLE_AUTO_START_ZEROCONF__ )
void zeroconf_auto_start_reg_services(void *params)
{
    DA16X_UNUSED_ARG(params);

    vTaskDelete(NULL);

    return;
}
#endif    // (__ENABLE_AUTO_START_ZEROCONF__)

#endif // (__SUPPORT_ZERO_CONFIG__)

/* EOF */
