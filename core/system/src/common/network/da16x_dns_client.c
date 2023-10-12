/**
 ****************************************************************************************
 *
 * @file da16x_dns_client.c
 *
 * @brief DNS Client module
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

#include "da16x_dns_client.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "util_api.h"

// Local variables
static char    ipaddr_str[16];
#ifdef    __SUPPORT_IPV6__
static char    ipv6addr_str[40];
#endif    /* __SUPPORT_IPV6__ */
static uint8_t is_name_resolution_done = pdFALSE;

#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
static url_to_ip_addr_t    _url_ipaddr_table = { 0 };
static url_to_ip_addr_t    *url_ipaddr_table_p = (url_to_ip_addr_t *)&_url_ipaddr_table;

static char    url_ipaddr_loading_flag = FALSE;
static char    ipaddr_str_temp[MAX_IPADDR_LEN];
#endif

// External global variables
extern UCHAR    dns_2nd_cache_flag;


int get_dns_addr(int iface, char *result_str)
{
    PRINTF("=== [%s] Need to FreeRTOS code for this fucntion ...\n", __func__);

    DA16X_UNUSED_ARG(iface);
    DA16X_UNUSED_ARG(result_str);

    return pdPASS;
}

int set_dns_addr(int iface, char *ip_addr)
{
    UINT status = pdFAIL;
    ip4_addr_t ipaddr;
    
    ipaddr.addr = ipaddr_addr(ip_addr);
    
    if (ipaddr.addr == IPADDR_NONE) {
        // Converting fails ...
        
        status = pdFAIL;
    } else {
        // Converting ok ...

        // Apply to dns immediately
        dns_setserver(0, &ipaddr);

        // Save to nvram
        char tmp_str[10] = { 0, };
        snprintf(tmp_str, 9, "%d:%s", iface, STATIC_IP_DNS_SERVER);
        write_nvram_string(tmp_str, ip_addr); /* SET DNS */
        
        status = pdPASS;
    }

    return status;
}

int get_dns_addr_2nd(int iface_flag, char *result_str)
{
    PRINTF("=== [%s] Need to write FreeRTOS code for this function ...\n", __func__);

    DA16X_UNUSED_ARG(iface_flag);
    DA16X_UNUSED_ARG(result_str);

    return pdTRUE;
}

int set_dns_addr_2nd(int iface_flag, char *ip_addr)
{
    UINT status = pdFAIL;
    ip4_addr_t ipaddr;
    
    ipaddr.addr = ipaddr_addr(ip_addr);

    if (ipaddr.addr == IPADDR_NONE) {
        // Converting fails ...
        
        status = pdFAIL;
    } else {
        // Converting ok ...

        // Apply to dns immediately
        dns_setserver(1, &ipaddr);

        // save to nvram
        char tmp_str[11] = { 0, };
        snprintf(tmp_str, 10, "%d:%s2", iface_flag, STATIC_IP_DNS_SERVER);
        write_nvram_string(tmp_str, ip_addr); /* SET DNS */
        
        status = pdPASS;
    }

    return status;
}

void dns_req_resolved(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
    DA16X_UNUSED_ARG(arg);
#if !defined ( __DNS_2ND_CACHE_SUPPORT__ )
    DA16X_UNUSED_ARG(hostname);
#endif    //__DNS_2ND_CACHE_SUPPORT__

    if (ipaddr->addr == 0) {
        PRINTF("[%s] Failed to query DNS. Invaild domain URL...\n", __func__);
        is_name_resolution_done = pdFALSE;
    } else {
        memset(ipaddr_str, 0, 16);

        strcpy(ipaddr_str, ipaddr_ntoa(ipaddr));
        
#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
        if (da16x_dns_2nd_cache_is_enabled() == pdTRUE) {
            da16x_dns_2nd_cache_add((char*)hostname, NULL, (unsigned long)iptolong(ipaddr_str));
        }
#endif
        is_name_resolution_done = pdTRUE;
    }
}

char *dns_A_Query(char *domain_name, unsigned long wait_option)
{
    unsigned long    time_to_wait = wait_option;

    memset(ipaddr_str, 0, IPADDR_LEN);

    /*************************************************************************/
    /*                    Type A                                                 */
    /*    Send A type DNS Query to its DNS server and get the IPv4 address.     */
    /*************************************************************************/
    /* Look up an IPv4 address over IPv4. */

    ip_addr_t dns_ipaddr;
    char * domain_url = domain_name;

    // Initialize this flag before starting query
    is_name_resolution_done = pdFALSE;

    err_t ret = dns_gethostbyname(domain_url, &dns_ipaddr, dns_req_resolved, NULL);

    if (ERR_OK == ret) {
        if (dns_ipaddr.addr == 0) {
            PRINTF("[%s] Failed to query DNS. Invaild domain URL...\n", __func__);
            return NULL;
        } else {
            strcpy(ipaddr_str, ipaddr_ntoa(&dns_ipaddr));
            return (char *)ipaddr_str;
        }
    } else if (ERR_INPROGRESS == ret) {
        ;
    } else {
        PRINTF("> DNS query fail (%s), ret=%d\n", domain_url, ret);
        return NULL;
    }

    while (time_to_wait > 0) {
        if (is_name_resolution_done == pdTRUE) {
            break;
        }

        vTaskDelay(100);
        time_to_wait -= 100;
    }

    if (is_name_resolution_done == FALSE) {
        // time-out
        return NULL;
    }
    if (isvalidip(ipaddr_str) != 1) {
        return NULL;
    }
    // dns name resolution successful
    return (char *)ipaddr_str;
}


unsigned int dns_ALL_Query(unsigned char *domain_name,
                        unsigned char *record_buffer,
                        unsigned int record_buffer_size,
                        unsigned int *record_count,
                        unsigned long wait_option)    /* GET_MULTI_IP */
{
    DA16X_UNUSED_ARG(wait_option);

    int status;
    UINT rec_cnt = 0;
    UINT32* rec_buf = NULL;
    struct addrinfo hints; 
    struct addrinfo *addr_list, *cur;

    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (record_buffer == NULL || record_count == NULL) {
           PRINTF("[%s] record_buffer(NULL) / record_count(NULL) \n", __func__);
        return 1; // error
    }

    if((status = getaddrinfo((const char *)domain_name, NULL, &hints, &addr_list)) != 0) {
           PRINTF("[%s] DNS query error(0x%0x). [%s], see netdb.h\n",
               __func__, (int)status, domain_name);
        return 1; // error
    }

    rec_buf = (UINT32*)record_buffer;

    for (cur = addr_list; (cur != NULL) && (rec_cnt <= record_buffer_size); cur = cur->ai_next) {
       *rec_buf = (UINT32)(((struct sockaddr_in*)(cur->ai_addr))->sin_addr.s_addr);
       rec_cnt++;
       rec_buf++;
    }
    
    *record_count = rec_cnt;

    freeaddrinfo( addr_list );

    return 0;
}


#if defined ( __DNS_2ND_CACHE_SUPPORT__ )
extern UINT da16x_ping_client(int iface, CHAR *domain_str, unsigned long ipaddr,
                      unsigned long *ipv6dst, unsigned long *ipv6src, int len, unsigned long count, int wait,
                      int interval, int nodisplay, char *ping_result);

// update sflash()
int _da16x_dns_2nd_cache_update_sflash(void) 
{
    return user_sflash_write(SFLASH_USER_AREA_START, 
                            (UCHAR *)url_ipaddr_table_p,
                            sizeof(url_to_ip_addr_t));
}

// idx find_empty_slot() : find empty slot
int _da16x_dns_2nd_cache_find_empty_slot(void) 
{
    int empty_idx = -1;
    int i;

    for (i = 0; i < MAX_URL_TABLE_CNT; i++) {
        if (url_ipaddr_table_p->table[i].ipaddr_str[0] == 0) {
            empty_idx = i;
            break;            
        }
    }
    
    return empty_idx;
}

// res update_row (idx[I], ip_addr[I]) : update row (+sflash update)
int _da16x_dns_2nd_cache_update(int idx, char* ip_addr) 
{
    int status;

    strncpy(url_ipaddr_table_p->table[idx].ipaddr_str, ip_addr, MAX_IPADDR_LEN);

    status = _da16x_dns_2nd_cache_update_sflash();
    if (status == FALSE) {
        // SFLASH write error.. but don't need to stop the operation...
        // Go ahead next operation ...
    }

    return TRUE;
}

// res delete_row (idx) : delete row (set ip_addr=0) (+sflash update)
int _da16x_dns_2nd_cache_delete(int idx) 
{
    int status;
    char* ip_str = url_ipaddr_table_p->table[idx].ipaddr_str;
    char* url_str = url_ipaddr_table_p->table[idx].url_str;

    if (url_ipaddr_table_p->table[idx].ipaddr_str[0] != 0) {
        ip_str[0] = 0;            
        url_str[0] = 0;
        url_ipaddr_table_p->saved_cnt--;
    } else {
        PRINTF("Failed to delete IP. It doesn't exist in table.\n");
    }
    
    
    if (url_ipaddr_table_p->saved_cnt == 65) {
        url_ipaddr_table_p->saved_cnt = 0;
    } else if (url_ipaddr_table_p->saved_cnt > MAX_URL_TABLE_CNT) {
        url_ipaddr_table_p->saved_cnt = MAX_URL_TABLE_CNT;
    }

    status = _da16x_dns_2nd_cache_update_sflash();
    if (status == FALSE) {
        // SFLASH write error.. but don't need to stop the operation...
        // Go ahead next operation ...
    }

    return TRUE;
}

// init_table() : memset(table,0,..)
int _da16x_dns_2nd_cache_init(void) 
{
    int status;
    
    if (url_ipaddr_loading_flag == TRUE) return TRUE;

    status = user_sflash_read(SFLASH_USER_AREA_START, url_ipaddr_table_p, sizeof(url_to_ip_addr_t));
    
    if (url_ipaddr_table_p->saved_cnt == 65 || url_ipaddr_table_p->saved_cnt <= 0) {
        // empty
        memset(url_ipaddr_table_p, 0, sizeof(url_to_ip_addr_t));
    }
                                
    url_ipaddr_loading_flag = TRUE;
    return status;
}

// idx find_row(str) : search and return idx
int _da16x_dns_2nd_cache_find(char* url) 
{
    int idx_found = -1;
    int i;

    for (i = 0; i < MAX_URL_TABLE_CNT; i++) {
        if (strcmp(url_ipaddr_table_p->table[i].url_str, url) == 0 && 
            url_ipaddr_table_p->table[i].ipaddr_str[0] != 0) {
            idx_found = i;
            break;            
        }
    }
    
    return idx_found;
}

// is_full()
int _da16x_dns_2nd_cache_is_full(void) 
{
    return (url_ipaddr_table_p->saved_cnt == MAX_URL_TABLE_CNT)? TRUE : FALSE;
}


// is_empty()
int _da16x_dns_2nd_cache_is_empty(void) 
{

    return (url_ipaddr_table_p->saved_cnt == 0)? TRUE : FALSE;
}

void _da16x_dns_2nd_cache_show_table(void)
{    
    int i, empty = 1;

    for (i = 0; i < MAX_URL_TABLE_CNT; i++) {
        if (url_ipaddr_table_p->table[i].ipaddr_str[0] != 0) {
            PRINTF("[%d]%s\n", i, url_ipaddr_table_p->table[i].url_str);
            PRINTF("[%d]%s\n", i, url_ipaddr_table_p->table[i].ipaddr_str);
            PRINTF("-----\n");
            empty = 0;
        }
    }

    if (empty == 1) PRINTF("dns 2nd cache table is empty !! \n");
}

void _da16x_dns_2nd_cache_modify_row(int idx, char* ip_addr_str)
{    
    if (url_ipaddr_table_p->table[idx].url_str[0] != 0) {
        strcpy(url_ipaddr_table_p->table[idx].ipaddr_str, ip_addr_str);
        _da16x_dns_2nd_cache_update_sflash();
    } else {
        PRINTF("Failed to modify IP. It doesn't exist in table.\n");
    }
}

int _da16x_dns_2nd_cache_get_saved_cnt(void)
{    
    return url_ipaddr_table_p->saved_cnt;
}

void dns_a_query_test(char *test_url_str)
{
    char    *test_ipaddr_str = pdFALSE;

    PRINTF(">>> Single IPv4 address DNS query test ...\n");

    /* DNS query with test url string */
    test_ipaddr_str = dns_A_Query(test_url_str, 400);

    /* Fail checking ... */
    if (test_ipaddr_str == pdFALSE) {
        PRINTF("\nFailed to dns-query with %s\n", test_url_str);
    } else {
        PRINTF("- Name : %s\n", test_url_str);
        PRINTF("- Addresses : %s\n", test_ipaddr_str);
    }
}

#if defined ( CONFIG_DNS2CACHE_TEST )
void cmd_dns2cache(int argc, char *argv[])
{
    if (argc < 2 || argc > 4) 
        PRINTF("dns2cache < print | is_empty | is_full | savedcnt | query [url] | setip [idx] [ip_addr | -1 (erase)] >\n");

    if (argc == 2) {
        _da16x_dns_2nd_cache_init();
        if (strcmp(argv[1], "print") == 0) {
            _da16x_dns_2nd_cache_show_table();        
        } else if (strcmp(argv[1], "is_empty") == 0) {
            PRINTF("dns_2nd_cache, is empty = %d\n", _da16x_dns_2nd_cache_is_empty());
        } else if (strcmp(argv[1], "is_full") == 0) {
            PRINTF("dns_2nd_cache, is full = %d\n", _da16x_dns_2nd_cache_is_full());
        } else if (strcmp(argv[1], "savedcnt") == 0) {
            PRINTF("dns_2nd_cache, saved_cnt = %d\n", _da16x_dns_2nd_cache_get_saved_cnt());
        }
        return;
    }

    if (argc == 3) {
        if (strcmp(argv[1], "query") == 0) {
            PRINTF("query address: %s \n", argv[2]);
            dns_a_query_test(argv[2]);
        }
        return;
    }

    if (argc == 4 && (strcmp(argv[1], "setip") == 0)) {
        _da16x_dns_2nd_cache_init();

        if ((strcmp(argv[3], "-1") == 0)) {
            _da16x_dns_2nd_cache_delete(atoi(argv[2]));        
        } else {
            _da16x_dns_2nd_cache_modify_row(atoi(argv[2]), argv[3]);    
        }
        return;
    }
}
#endif // CONFIG_DNS2CACHE_TEST
///////

// res add_row(url[I], ip_addr[I]): find empty slot and add (+sflash update)
int da16x_dns_2nd_cache_add(char* url, char* ip_addr, unsigned long ip_addr_long) 
{
    int idx;
    int status;

    // convert if ip_addr is in unsigned long format
    if (ip_addr == NULL) {
        longtoip(ip_addr_long, ipaddr_str_temp);
        ip_addr = ipaddr_str_temp;
    }

    // find an existing one
    idx = _da16x_dns_2nd_cache_find(url);
    if (idx != -1) {
        // found, update only ip_addr
        strcpy(url_ipaddr_table_p->table[idx].ipaddr_str, ip_addr);
    } else {
        // not found
        
        // find an empty slot to add
        idx = _da16x_dns_2nd_cache_find_empty_slot();
        
        if (idx == -1) {
            // if table is full, select index randomly
            idx = rand()%(MAX_URL_TABLE_CNT-1);
        }

        // add
        strcpy(url_ipaddr_table_p->table[idx].url_str, url);
        strcpy(url_ipaddr_table_p->table[idx].ipaddr_str, ip_addr);
    }

    url_ipaddr_table_p->saved_cnt++;

    if (url_ipaddr_table_p->saved_cnt > MAX_URL_TABLE_CNT) {
        url_ipaddr_table_p->saved_cnt = MAX_URL_TABLE_CNT;
    }

    /// sflash update
    status = _da16x_dns_2nd_cache_update_sflash();
    if (status == FALSE) {
        // SFLASH write error.. but don't need to stop the operation...
        // Go ahead next operation ...
    }

    return TRUE;
}

// update sflash()
int da16x_dns_2nd_cache_erase_sflash(void) 
{
    // erase 4K
    return user_sflash_erase(SFLASH_USER_AREA_START);
}

UINT da16x_dns_2nd_cache_is_enabled(void) 
{
    return dns_2nd_cache_flag;
}

UINT da16x_dns_2nd_cache_find_answer(UCHAR* host_name, unsigned long* ip_addr) 
{
    UINT    status = TRUE;

    // Read URL_to_IP_Addr table from SFLASH
    if (url_ipaddr_loading_flag == FALSE) {
        // sizeof(url_to_ip_addr_t) = 3601 bytes
        status = user_sflash_read(SFLASH_USER_AREA_START, url_ipaddr_table_p, 
                                    sizeof(url_to_ip_addr_t));
        url_ipaddr_loading_flag = TRUE;
    }

    if (status == FALSE) {
        // SFLASH read error ...
        DNS_DBG_ERR("[%s] SFLASH read Error ! \n", __func__);
        return ERR_VAL;
    } else {
        // ... DNS 2nd cache loading is ok
    
        if (   url_ipaddr_table_p->saved_cnt == 65 
            || url_ipaddr_table_p->saved_cnt == 0) {
            
            // ... cache is empty
            memset(url_ipaddr_table_p, 0, sizeof(url_to_ip_addr_t));
            return ERR_VAL;
        } else {

            // cache is not empty

            /*
            find ip_addr from dns_2nd_cache                                         
                case: found                                     
                    try ping                                    
                    case: ping success                                    
                        return ip_addr                                
                    case: ping fail                                 
                        remove the entry from dns_2nd_cache (and update sflash)                             
            */
            int idx;
            
            idx = _da16x_dns_2nd_cache_find((char *)host_name);
            if (idx != -1) {
                // cache hit !
                *ip_addr = (unsigned long)iptolong(url_ipaddr_table_p->table[idx].ipaddr_str);
                
                DNS_DBG_INFO("[%s] Matched : to_find=<%s>, ipaddr=<%s>\n", __func__, 
                (char *)host_name, url_ipaddr_table_p->table[idx].ipaddr_str);
            
                // Check IP address is available or not...
                status = da16x_ping_client(0, NULL, *ip_addr, NULL, NULL, 32, 3, 300, 200, 2, NULL);
                if (status != ERR_OK) {
                    // the ip addr is not valid any more, erase it
                    _da16x_dns_2nd_cache_delete(idx);
                    
                    //ping_fail_overwrite = TRUE;
                    DNS_DBG_INFO("[%s] 2nd cache hit but not valid ! \n", __func__);
                    return ERR_VAL;
                } else {
                    DNS_DBG_INFO("[%s] cache hit and verified OK as well ! \n", __func__);
                    return ERR_OK;
                }                
            } else {
                // cache NOT hit !
                return ERR_VAL;
            }
        }
    }
}


#else 

UINT da16x_dns_2nd_cache_is_enabled(void) 
{
    return dns_2nd_cache_flag;
}

UINT da16x_dns_2nd_cache_find_answer(UCHAR* host_name, unsigned long* ip_addr) 
{
    DA16X_UNUSED_ARG(host_name);
    DA16X_UNUSED_ARG(ip_addr);

    return ERR_VAL;
}

#endif  // __DNS_2ND_CACHE_SUPPORT__


/* EOF */
