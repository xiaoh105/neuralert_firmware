/**
 ****************************************************************************************
 *
 * @file netdevice.h
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

#ifndef _DIW_NETDEVICE_H
#define _DIW_NETDEVICE_H

#ifdef CONFIG_DCB
#include <net/dcbnl.h>
#endif

//#include <os_con.h>
#include <net_con.h>

#include "features.h"

/// The device structure.
/*
 * Actually, this whole structure is a big mistake.
 * It mixes I/O data with strictly "high-level" data,
 * and it has to know about almost every data structure used in the INET module.
 *
 * FIXME: cleanup struct net_device such that network protocol info moves out.
 */
/* Chang. Optimized net_device structure. 141016 */
struct net_device
{
	/* Virtual interface Index & type We can differ Net device as ifType. */
	/// Virtual interface type.
	int ifType;     //enum nld11_iftype
	/// Virtual interface index.
	int ifindex;	//Concurent Mode Interface Index.

	/// Protocol specific pointers.
	/* i3ed11 specific data, assign before registering. */
	struct wifidev	*i3ed11_ptr;

	/* Interface address info used in eth_type_trans() */
	/* hw address, (before bcast  because most packets are unicast) */
	/// Hardware address.
	unsigned char	dev_addr[DIW_ETH_ALEN];

	/// Sub interface.
	/* Chang. Need Sub interface. 141008 */
	struct i3ed11_wifisubif *wsubif;
};

#define IFF_TX_SKB_SHARING               1<<16

#endif	/* _DIW_NETDEVICE_H */
