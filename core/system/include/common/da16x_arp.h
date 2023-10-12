/**
 ****************************************************************************************
 *
 * @file da16x_arp.h
 *
 * @brief Define for ARP module
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


#ifndef __DA16X_ARP_H__
#define __DA16X_ARP_H__

#include "sdk_type.h"

#include <stdbool.h>
#include "da16x_network_common.h"
#include "lwipopts.h"

#if LWIP_ARP
  #include "lwip/etharp.h"
#endif // LWIP_ARP

#undef DA16X_ARP_DEBUG
#ifdef DA16X_ARP_DEBUG
  #define ARP_DEBUG_PRINT(...)    PRINTF(__VA_ARGS__)
#else
  #define ARP_DEBUG_PRINT(...)
#endif // ARP_DEBUG_PRINT

#define DPM_ARP_TABLE_SIZE        (sizeof(NX_ARP) * NX_ARP_TABLE_DPM_SIZE)
#define POLL_STATE_CHK_STK_SZ     1024


/*
 * External global functions
 */
#if defined ( IP_STACK_PORT )
extern NX_INTERFACE *get_wifi_driver_interface_ptr(int intf);
#endif    /*IP_STACK_PORT*/

extern void print_separate_bar(UCHAR text, UCHAR loop_count, UCHAR CR_loop_count);
extern int  isvalidIPsubnetRange(long ip, long subnetip, long subnet);
extern void stop_sys_apps_completed(void);
extern void stop_user_apps_completed(void);
extern bool is_dpm_supplicant_done(void);
extern int  chk_dpm_mode(void);
extern int  chk_dpm_wakeup(void);
extern int  chk_supp_connected(void);
extern void dpm_accept_arp_broadcast(unsigned long accept_addr, unsigned long subnet_addr);
extern void dpm_accept_tim_arp_resp(unsigned long gw_ip, unsigned char *gw_mac);
extern unsigned char *get_da16x_dpm_arp_ptr(void);


struct cmd_arping_param{
    UINT      status;
    ip_addr_t ipaddr;
    UINT      count;
    UINT      wait;
    UINT      interval;
    int       ping_interface;
};

/**
 *********************************************************************************
 * @brief       Register call-back function to save ARP table in RTM on DPM mode
 * @input[in]   None
 * @return      None
 *********************************************************************************
 */
void da16x_arp_setup_custom_nx_save_arp_table(void);

/**
 *********************************************************************************
 * @brief       Restore ARP table from RTM when DPM wakeup
 * @input[in]   ip_ptr        IP pointer to restore ARP table
 * @input[in]   intf_select    Current interface number
 * @return      ERR_OK on success
 *********************************************************************************
 */
#if defined ( IP_STACK_PORT )
UINT restore_arp_table(NX_IP *ip_ptr, UINT intf_select);
#endif // IP_STACK_PORT

/**
 *********************************************************************************
 * @brief   Get current ARP request status
 * @return  Current arp_req_send_flag
 *********************************************************************************
 */
UINT get_current_arp_req_status(void);


int arp_request(ip_addr_t ipaddr, int iface);
int arp_response(int iface, char *dst_ipaddr,  char *dst_macaddr);
int garp_request(int iface, int check_ipconflict);
unsigned int arp_send_for_ip_collision_avoid(int t_static_ip);
UINT print_arp_table(UINT iface);

// Called by reconfig_net
int arp_entry_delete(int iface);
int dhcp_arp_request(int iface, ULONG ipaddr);

UINT da16x_arp_create_polling_state_check(UINT iface);
int do_autoarp_check(void);

///// start --- move here from etharp.c //////
#if defined ( LWIP_HOOK_FILENAME )
  #include LWIP_HOOK_FILENAME
#endif // LWIP_HOOK_FILENAME

/** Re-request a used ARP entry 1 minute before it would expire to prevent
 *  breaking a steadily used connection because the ARP entry timed out. */
#define ARP_AGE_REREQUEST_USED_UNICAST        (ARP_MAXAGE - 30)
#define ARP_AGE_REREQUEST_USED_BROADCAST    (ARP_MAXAGE - 15)

/** the time an ARP entry stays pending after first request,
 *  for ARP_TMR_INTERVAL = 1000, this is
 *  10 seconds.
 *
 *  @internal Keep this number at least 2, otherwise it might
 *  run out instantly if the timeout occurs directly after a request.
 */
#define ARP_MAXPENDING 5

/** arping receive timeout - in milliseconds */
#ifndef ETHARPING_RCV_TIMEO
  #define ETHARPING_RCV_TIMEO  1000
#endif

/** arping delay - in milliseconds */
#ifndef ETHARPING_DELAY
#define ETHARPING_DELAY        1000
#endif

/** ARP states */
enum etharp_state {
    ETHARP_STATE_EMPTY = 0,
    ETHARP_STATE_PENDING,
    ETHARP_STATE_STABLE,
    ETHARP_STATE_STABLE_REREQUESTING_1,
    ETHARP_STATE_STABLE_REREQUESTING_2
#if ETHARP_SUPPORT_STATIC_ENTRIES
    , ETHARP_STATE_STATIC
#endif /* ETHARP_SUPPORT_STATIC_ENTRIES */
};

struct etharp_entry {
#if ARP_QUEUEING
    /** Pointer to queue of pending outgoing packets on this ARP entry. */
    struct etharp_q_entry *q;
#else /* ARP_QUEUEING */
    /** Pointer to a single pending outgoing packet on this ARP entry. */
    struct pbuf    *q;
#endif /* ARP_QUEUEING */

    ip4_addr_t      ipaddr;
    struct netif    *netif;
    struct eth_addr ethaddr;
    u16_t  ctime;
    u8_t   state;
};
/////// end --- move here from etharp.c //////

#endif // __DA16X_ARP_H__

/* EOF */
