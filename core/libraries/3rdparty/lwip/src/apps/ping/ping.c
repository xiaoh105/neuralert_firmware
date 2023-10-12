/** 
 * @file 
 * Ping sender module 
 * 
 */ 


/* 
 * Redistribution and use in source and binary forms, with or without modification,  
 * are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution. 
 * 3. The name of the author may not be used to endorse or promote products 
 *    derived from this software without specific prior written permission.  
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED  
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF  
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT  
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS  
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN  
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY  
 * OF SUCH DAMAGE. 
 * 
 * This file is part of the lwIP TCP/IP stack. 
 *  
 */ 


/**  
 * This is an example of a "ping" sender (with raw API and socket API). 
 * It can be used as a start point to maintain opened a network connection, or 
 * like a network "watchdog" for your device. 
 * 
 */ 


#include <stdbool.h>
#include "lwip/opt.h" 


#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */ 


#include "ping.h" 


#include "lwip/mem.h" 
#include "lwip/raw.h" 
#include "lwip/icmp.h" 
#include "lwip/netif.h" 
#include "lwip/sys.h" 
#include "lwip/timeouts.h" 
#include "lwip/inet_chksum.h" 


#if PING_USE_SOCKETS 
#include "lwip/sockets.h" 
#include "lwip/inet.h" 
#endif /* PING_USE_SOCKETS */ 

#include "da16x_ping.h"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"


#if PING_USE_SOCKETS
/** ping target - should be a "ip_addr_t" */ 
#ifndef PING_TARGET 
#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any) 
#endif 
#else
static 	ip_addr_t	target_ip;
#endif

/** ping receive timeout - in milliseconds */ 
#ifndef PING_RCV_TIMEO 
#define PING_RCV_TIMEO 1000 
#endif 


/** ping delay - in milliseconds */ 
#ifndef PING_DELAY 
#define PING_DELAY     1000 
#endif 


/** ping identifier - must fit on a u16_t */ 
#ifndef PING_ID 
#define PING_ID        0xAFAF 
#endif 


/** ping additional data size to include in the packet */ 
#ifndef PING_DATA_SIZE 
#define PING_DATA_SIZE 32 	//600 , testing
#endif 


/** ping result action - no default action */ 
#ifndef PING_RESULT 
#define PING_RESULT(ping_ok) 
#endif 


/* ping variables */ 
#if !PING_USE_SOCKETS 
static struct raw_pcb *ping_pcb; 
#endif /* PING_USE_SOCKETS */ 

static u16_t ping_seq_num;
static u32_t ping_time;
u16_t packet_send_cnt;
u16_t packet_recv_cnt;
u32_t ping_total_time;
u32_t ping_min_time;
u32_t ping_max_time;
struct cmd_ping_param *ping_param;
EventGroupHandle_t ping_recv_evt_handle = NULL;

bool is_ping_timeout = pdFALSE;
bool ping_display_on = pdTRUE;		// Default ON

static void ping_thread(void *arg, bool no_display);

/** Prepare a echo ICMP request */ 
static void 
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len) 
{ 
	size_t i; 
	//unsigned int data_len = len - sizeof(struct icmp_echo_hdr); 
	unsigned int data_len = len - 8; 


	ICMPH_TYPE_SET(iecho, ICMP_ECHO); 
	ICMPH_CODE_SET(iecho, 0); 
	iecho->chksum = 0; 
	iecho->id     = PING_ID; 
	iecho->seqno  = htons(++ping_seq_num); 

	/* fill the additional data buffer with some data */ 
	for(i = 0; i < data_len; i++) { 
	  ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i; 
	} 


	iecho->chksum = inet_chksum(iecho, len); 
} 


#if PING_USE_SOCKETS 


/* Ping using the socket ip */ 
static err_t 
ping_send(int s, ip_addr_t *addr) 
{ 
	int err; 
	struct icmp_echo_hdr *iecho; 
	struct sockaddr_in to; 
	//unsigned int ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE; 
	unsigned int ping_size = 8 + PING_DATA_SIZE; 
	LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff); 


	iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size); 
	if (!iecho) { 
	  return ERR_MEM; 
	} 


	ping_prepare_echo(iecho, (u16_t)ping_size); 


	to.sin_len = sizeof(to); 
	to.sin_family = AF_INET; 
#if 1
	//inet_addr_from_ipaddr(&to.sin_addr, addr); 
	inet_addr_from_ip4addr(&to.sin_addr, ip_2_ip4(addr)); 
#endif

	err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to)); 


	mem_free(iecho); 


	return (err ? ERR_OK : ERR_VAL); 
} 


static void 
ping_recv(int s) 
{ 
	char buf[64]; 
	int fromlen, len; 
	struct sockaddr_in from; 
	struct ip_hdr *iphdr; 
	struct icmp_echo_hdr *iecho; 

	PRINTF("%s +++++\n", __func__);
#if 1
	while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) { 
	  if (len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) { 
	    ip_addr_t fromaddr; 
	  
	    inet_addr_to_ip4addr(ip_2_ip4(&fromaddr), &from.sin_addr); 
	    LWIP_DEBUGF( PING_DEBUG, "ping: recv "); 
	    ip_addr_debug_print(PING_DEBUG, &fromaddr); 
	    LWIP_DEBUGF( PING_DEBUG, " %"U32_F" ms\n", (sys_now() - ping_time)); 


	    iphdr = (struct ip_hdr *)buf; 
	    iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4)); 
	    if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) { 
	      /* do some ping result processing */ 
	      PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER)); 
	      return; 
	    } else { 
	      LWIP_DEBUGF( PING_DEBUG, "ping: drop\n"); 
	    } 
	  } 
	} 


	if (len == 0) { 
	  LWIP_DEBUGF( PING_DEBUG, "ping: recv - %"U32_F" ms - timeout\n", (sys_now()-ping_time)); 
	} 

#endif
	/* do some ping result processing */ 
	PING_RESULT(0); 
} 


static void 
ping_thread(void *arg, bool no_display) 
{ 
	int s; 
	int timeout = PING_RCV_TIMEO; 
	ip_addr_t ping_target; 


	LWIP_UNUSED_ARG(arg); 


	if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) { 
	  return; 
	} 


	lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); 


	while (1) { 
	  ping_target = PING_TARGET; 


	  if (ping_send(s, &ping_target) == ERR_OK) { 
	    LWIP_DEBUGF( PING_DEBUG, "ping: send "); 
	    ip_addr_debug_print(PING_DEBUG, &ping_target); 
	    LWIP_DEBUGF( PING_DEBUG, "\n"); 


	    ping_time = sys_now(); 
	    ping_recv(s); 
	  } else { 
	    LWIP_DEBUGF( PING_DEBUG, "ping: send "); 
	    ip_addr_debug_print(PING_DEBUG, &ping_target); 
	    LWIP_DEBUGF( PING_DEBUG, " - error\n"); 
	  } 
	  sys_msleep(PING_DELAY); 
	} 
} 


#else /* PING_USE_SOCKETS */ 

/* Ping using the raw ip */ 
static u8_t 
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr) 
{ 
	struct icmp_echo_hdr *iecho;
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(pcb);
	LWIP_UNUSED_ARG(addr);
	LWIP_ASSERT("p != NULL", p != NULL);

	u32_t response_time = sys_now() - ping_time;

	/* PBUF_IP_HLEN should be set as 20, if IPv6 is enabled, set as 40 , Check it */
	if (pbuf_header_force( p, -PBUF_IP_HLEN) == 0) {
		iecho = (struct icmp_echo_hdr *)p->payload;

		if (iecho->type == ICMP_ER
			&& iecho->id == PING_ID
			&& iecho->seqno == htons(ping_seq_num)
			&& is_ping_timeout == pdFALSE)
		{
			ping_total_time += response_time;

			if (response_time > 0) {
				if (ping_min_time == 0)	{
					ping_min_time = response_time;
					ping_max_time = response_time;
				}
				else if ((int)ping_min_time > (int)response_time) {
					ping_min_time = response_time;
				}
				else if ((int)ping_max_time < (int)response_time) {
					ping_max_time = response_time;
				}
			}

			packet_recv_cnt++;

			if (ping_display_on == pdTRUE) {
                PRINTF("%u bytes from %s icmp_req=%u time%c%ums\n",
                        p->tot_len - sizeof(struct icmp_echo_hdr), ipaddr_ntoa(addr), ping_seq_num,
                        response_time < portTICK_PERIOD_MS ? '<':'=',
                        response_time < portTICK_PERIOD_MS ? portTICK_PERIOD_MS : response_time);
			}

			/* do some ping result processing */
			PING_RESULT(1);

			if (ping_recv_evt_handle != NULL) {
				xEventGroupSetBits(ping_recv_evt_handle, PING_RECV_EVT_BITS);
			}
		}
		/* Only seq number is mismatch */
		else if (iecho->type == ICMP_ER && iecho->id == PING_ID) { /* ICMP Type: Reply & ID: PING */
			LWIP_DEBUGF( PING_DEBUG, "ping: drop seq=%d\n", ntohs(iecho->seqno));
		} else {
    #if 0
			PRINTF("Ping Fail(id %x ,icmp_req:%d)\n" , iecho->id , ntohs(iecho->seqno));
    #else   /* During Ping Tx&Rx, Received ping packet will be handled in icmp_input() by this change*/
            pbuf_header_force( p, PBUF_IP_HLEN);
            return 0;        /* Don't eat the packet */
    #endif
		}

		pbuf_free(p);
		return 1; /* eat the packet */
	}

	return 0; /* don't eat the packet */ 
} 


static void 
ping_send(struct raw_pcb *raw, ip_addr_t *addr) 
{ 
	struct pbuf *p; 
	struct icmp_echo_hdr *iecho;
	size_t ping_size;

	if (ping_param != NULL && ping_param->len != 0)
	{
	  ping_size = sizeof(struct icmp_echo_hdr) + ping_param->len;
	}
	else
	{
	  ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
	}

	LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

	p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);

	if (!p)
	{
	  return; 
	}

	ping_time = sys_now();

	p->if_idx = ping_pcb->netif_idx;

	if ((p->len == p->tot_len) && (p->next == NULL))
	{
	  iecho = (struct icmp_echo_hdr *)p->payload; 

	  ping_prepare_echo(iecho, (u16_t)ping_size); 

	  raw_sendto(raw, p, addr); 
	}

	pbuf_free(p); 
} 


static void 
ping_timeout(void *arg) 
{ 
	struct raw_pcb *pcb = (struct raw_pcb*)arg; 
	ip_addr_t ping_target;

	ip_addr_copy_from_ip4(ping_target, target_ip);
	
	LWIP_ASSERT("ping_timeout: no pcb given!\r\n", pcb != NULL);

	ping_send(pcb, &ping_target);
	
	sys_timeout(PING_DELAY, ping_timeout, pcb); 
} 


static void 
ping_raw_init(int if_idx) 
{ 
	struct netif *netif;
	
	ping_pcb = raw_new(IP_PROTO_ICMP); 
	LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL); 

	ping_seq_num = 0;

	netif = netif_get_by_index((u8_t)(if_idx + 1));

	if (netif == NULL)
	{
		if (netif_default != NULL)
		{
			ip_addr_copy_from_ip4(target_ip, netif_default->gw);
			//ip4_addr_set(ip_2_ip4(target_ip), netif_default->gw);
		}
		else
		{			
			ip_addr_copy_from_ip4(target_ip, ip_addr_any);
		}
	}
	else
	{
		ip_addr_copy_from_ip4(target_ip, netif->gw);
		//ip4_addr_set(ip_2_ip4(target_ip), ip_2_ip4(&netif->gw));
	}

	raw_recv(ping_pcb, (raw_recv_fn) ping_recv, NULL);
	raw_bind(ping_pcb, IP_ADDR_ANY); 
	sys_timeout(PING_DELAY, ping_timeout, ping_pcb); 
} 


void 
ping_send_now(void)
{
	ip_addr_t ping_target;
	ping_target = target_ip;

	LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL); 
	ping_send(ping_pcb, &ping_target);
}


static void ping_thread(void *arg, bool no_display)
{
	LWIP_UNUSED_ARG(arg);

	ip_addr_t ping_target;
	struct netif *netif;
	UINT count = 1;
	ping_seq_num = 0;
	int ch;

	if (ping_param == NULL)
	{
		//INTF("\r\n>> Ping Param is Null!!\r\n");

		if (ping_recv_evt_handle != NULL)
		{
			vEventGroupDelete(ping_recv_evt_handle);
			ping_recv_evt_handle = NULL;
		}

		return;
	}
#if 0 //Debug
	else
	{
		if (ping_display_on == pdTRUE)
			PRINTF("Ping Network Interface:WLAN[%d]\n", ping_param->ping_interface);
	}
#endif // Debug

	ip_addr_copy_from_ip4(ping_target, target_ip);
	ping_pcb = raw_new(IP_PROTO_ICMP);
	LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

	if (ping_display_on == pdTRUE)
	{
		PRINTF("PING %s %d bytes of data\n",ipaddr_ntoa(&(ping_param->ipaddr)),  PING_DATA_SIZE);
	}

	vTaskDelay(10);

	netif = netif_get_by_index((u8_t)(ping_param->ping_interface+2));

	ping_pcb->netif_idx = (u8_t)(ping_param->ping_interface+2);

	if (netif == NULL)
	{
		PRINTF(">> Network Interface is NULL\n");
		
		if (ping_param != NULL)
		{
		  vPortFree(ping_param);
		  ping_param = NULL;
		}

		raw_remove(ping_pcb);

		if (ping_recv_evt_handle != NULL)
		{
			vEventGroupDelete(ping_recv_evt_handle);
			ping_recv_evt_handle = NULL;
		}

		return;
	}

	raw_recv(ping_pcb, (raw_recv_fn) ping_recv, NULL);
	raw_bind(ping_pcb, IP_ADDR_ANY);
	raw_bind_netif (ping_pcb, netif);

	while (1) {
		ch = GETC_NOWAIT();
		if (ch == 0x03) /* CTRL+C */
		{
			goto ping_end;
		}

		if ((ping_param->count == 0 && count > 4)
			|| (ping_param->count != 0 && count > ping_param->count))
		{
			if (ping_display_on == pdTRUE)
			{
ping_end :
				PRINTF("\n--- %s ping statistics ---\n", ipaddr_ntoa(&ping_param->ipaddr));
				PRINTF("%u packets transmitted, %u received, %u%%(%u) packet loss, total time %ums\n",
									packet_send_cnt,
									packet_recv_cnt,
									((packet_send_cnt - packet_recv_cnt)*100) / packet_send_cnt,
									packet_send_cnt - packet_recv_cnt,
									ping_total_time);

				PRINTF("rtt min/avg/max = %u/%u/%u ms\n\n",
									ping_min_time,
									ping_total_time/packet_recv_cnt,
									ping_max_time);
			}

			if (ping_param != NULL)
			{
				vPortFree(ping_param);
				ping_param = NULL;
			}

			raw_remove(ping_pcb);

			if (ping_recv_evt_handle != NULL)
			{
				vEventGroupDelete(ping_recv_evt_handle);
				ping_recv_evt_handle = NULL;
			}

			break;
		}
		else
		{
			count++;
		}

		packet_send_cnt++;
		is_ping_timeout = pdFALSE;

		//send packet
		ping_send(ping_pcb, &ping_target);

		//wait receive packet or timeout
		if (ping_recv_evt_handle != NULL)
		{
			EventBits_t ret;

			if (ping_param->wait != 0)
			{
				ret = xEventGroupWaitBits(ping_recv_evt_handle, PING_RECV_EVT_BITS, pdTRUE, pdFALSE, pdMS_TO_TICKS(ping_param->wait));
			}
			else
			{
				ret = xEventGroupWaitBits(ping_recv_evt_handle, PING_RECV_EVT_BITS, pdTRUE, pdFALSE, pdMS_TO_TICKS(PING_RCV_TIMEO));
			}

			if ((ret & PING_RECV_EVT_BITS) == pdFALSE && no_display == pdFALSE)
			{
				PRINTF("From %s icmp_req=%u : Destination Host Unreachable\n", ipaddr_ntoa(&target_ip),	packet_send_cnt);
				is_ping_timeout = pdTRUE;
			}
		}

		//set interval
		if (ping_param->interval != 0)
		{
			vTaskDelay(pdMS_TO_TICKS(ping_param->interval));
		}
		else
		{
			vTaskDelay(pdMS_TO_TICKS(PING_DELAY));
		}
	}
}

#endif /* PING_USE_SOCKETS */

void ping_init(int if_idx)
{ 
#if PING_USE_SOCKETS 
	sys_thread_new("ping_thread", ping_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO); 
#else /* PING_USE_SOCKETS */ 
	ping_raw_init(if_idx); 
#endif /* PING_USE_SOCKETS */ 
} 


void ping_init_for_console(void * param, bool no_display)
{
	packet_send_cnt = 0;
	packet_recv_cnt = 0;
	ping_total_time = 0;
	ping_min_time = 0;
	ping_max_time = 0;

	ping_param = (struct cmd_ping_param *)param;
	ip_addr_copy_from_ip4(target_ip, ping_param->ipaddr);

	LWIP_DEBUGF( PING_DEBUG, "Ping request IP: %s", ipaddr_ntoa(&target_ip));

	ping_recv_evt_handle = xEventGroupCreate();

	ping_thread(param, no_display);
}

int get_ping_send_cnt(void)
{
	return (int)packet_send_cnt;
}

int get_ping_recv_cnt(void)
{
	return (int)packet_recv_cnt;
}

int get_ping_total_time_ms(void)
{
	return (int)ping_total_time;
}

int get_ping_min_time_ms(void)
{
	return (int)ping_min_time;
}

int get_ping_max_time_ms(void)
{
	return (int)ping_max_time;
}

int get_ping_avg_time_ms(void)
{
	return (int)(ping_total_time / packet_recv_cnt);
}

#endif /* LWIP_RAW */ 

