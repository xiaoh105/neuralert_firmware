/**
 ****************************************************************************************
 *
 * @file net_con.h
 *
 * @brief Header file
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

#ifndef _NET_CON_H
#define _NET_CON_H

#include "if_ether.h"
#include "skbuff.h"

/// netdevice
enum netdev_tx
{
	/// Make sure enum is signed.
	__NETDEV_TX_MIN	 = INT_MIN,
	/// Driver took care of packet.
	NETDEV_TX_OK	 = 0x00,
	/// Driver tx path was busy.
	NETDEV_TX_BUSY	 = 0x10,
	/// Driver tx lock was already taken.
	NETDEV_TX_LOCKED = 0x20,
};
typedef enum netdev_tx netdev_tx_t;

struct netdev_hw_addr_list
{
	struct diw_list_head	list;
	int			count;
};

#define NETDEV_ALIGN	32

/* Chang. netif related function define. 140922 */
#define netif_carrier_on(x)		/* nothing : bit operation for link status configuration */
#define netif_carrier_off(x)		/* nothing : bit operation for link status configuration */
#define netif_start_subqueue(x, y)	/* nothing : allow transmit by bit operation */
#define netif_stop_subqueue(x, y)	/* nothing : stop transmitted packets by bit operation */
#define netif_wake_subqueue(x, y)		/* nothing : resume individual transmit queue of a device with multiple transmit queues by bit operation */
#define netif_tx_stop_all_queues(x)	/* related to AP, Defined to nothing */
#define netif_tx_start_all_queues(x)	/* related to AP, Defined to nothing */

/* Chang. should be implemented. 140922 */
//#define netif_receive_wpkt(x)		/* i think fill wpkt and then forward by ip receive(ip_rcv)*/
//#define netif_rx(x)			/* First of all, we consider only one interface, and so do as comment */

//#define netif_rx_ni(x)			/* same as netif_rx and is used in noninterrupt contexts */
#ifdef UMAC_MCAST_ADDRLIST
static inline void __hw_addr_init(struct netdev_hw_addr_list *list)
{
	INIT_LIST_HEAD(&list->list);
	list->count = 0;
}
#endif

#endif
