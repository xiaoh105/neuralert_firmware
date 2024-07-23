/**
 ****************************************************************************************
 *
 * @file if_ether.h
 *
 * @brief Header file for Global Definitions for the Ethernet IEEE 802.3 Interface
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

#ifndef _UAPI_DIW_IF_ETHER_H
#define _UAPI_DIW_IF_ETHER_H

#include "os_con.h"

#define DIW_ETH_ALEN	6
#define DIW_ETH_HLEN	14
//#define DIW_ETH_ZLEN	60
//#define DIW_ETH_DATA_LEN	1500
//#define DIW_ETH_FRAME_LEN	1514
//#define DIW_ETH_FCS_LEN	4

/* Ethernet Protocol ID's. */
#define DIW_ETH_P_LOOP		0x0060
#define DIW_ETH_P_PUP		0x0200
#define DIW_ETH_P_PUPAT		0x0201
#define DIW_ETH_P_IP		0x0800
#define DIW_ETH_P_X25		0x0805
#define DIW_ETH_P_ARP		0x0806
#define DIW_ETH_P_BPQ		0x08FF
#define DIW_ETH_P_IEEEPUP	0x0a00
#define DIW_ETH_P_IEEEPUPAT	0x0a01
#define DIW_ETH_P_BATMAN	0x4305
#define DIW_ETH_P_DEC       0x6000
#define DIW_ETH_P_DNA_DL    0x6001
#define DIW_ETH_P_DNA_RC    0x6002
#define DIW_ETH_P_DNA_RT    0x6003
#define DIW_ETH_P_LAT       0x6004
#define DIW_ETH_P_DIAG      0x6005
#define DIW_ETH_P_CUST      0x6006
#define DIW_ETH_P_SCA       0x6007
#define DIW_ETH_P_TEB		0x6558
#define DIW_ETH_P_RARP      0x8035
#define DIW_ETH_P_ATALK		0x809B
#define DIW_ETH_P_AARP		0x80F3
#define DIW_ETH_P_8021Q		0x8100
#define DIW_ETH_P_IPX		0x8137
#define DIW_ETH_P_IPV6		0x86DD
#define DIW_ETH_P_PAUSE		0x8808
#define DIW_ETH_P_SLOW		0x8809
#define DIW_ETH_P_WCCP		0x883E
#define DIW_ETH_P_PPP_DISC	0x8863
#define DIW_ETH_P_PPP_SES	0x8864
#define DIW_ETH_P_MPLS_UC	0x8847
#define DIW_ETH_P_MPLS_MC	0x8848
#define DIW_ETH_P_ATMMPOA	0x884c
#define DIW_ETH_P_LINK_CTL	0x886c
#define DIW_ETH_P_ATMFATE	0x8884
#define DIW_ETH_P_PAE		0x888E
#define DIW_ETH_P_AOE		0x88A2
#define DIW_ETH_P_8021AD	0x88A8
#define DIW_ETH_P_802_EX1	0x88B5
#define DIW_ETH_P_TIPC		0x88CA
#define DIW_ETH_P_8021AH	0x88E7
#define DIW_ETH_P_MVRP		0x88F5
#define DIW_ETH_P_1588		0x88F7
#define DIW_ETH_P_FCOE		0x8906
#define DIW_ETH_P_TDLS		0x890D
#define DIW_ETH_P_FIP		0x8914
#define DIW_ETH_P_QINQ1		0x9100
#define DIW_ETH_P_QINQ2		0x9200
#define DIW_ETH_P_QINQ3		0x9300
#define DIW_ETH_P_EDSA		0xDADA
#define DIW_ETH_P_AF_IUCV   0xFBFB

#define DIW_ETH_P_802_3		0x0001
#define DIW_ETH_P_AX25		0x0002
#define DIW_ETH_P_ALL		0x0003
#define DIW_ETH_P_802_2		0x0004
#define DIW_ETH_P_SNAP		0x0005
#define DIW_ETH_P_DDCMP     0x0006
#define DIW_ETH_P_WAN_PPP   0x0007
#define DIW_ETH_P_PPP_MP    0x0008
#define DIW_ETH_P_LOCALTALK 0x0009
#define DIW_ETH_P_CAN		0x000C
#define DIW_ETH_P_CANFD		0x000D
#define DIW_ETH_P_PPPTALK	0x0010
#define DIW_ETH_P_TR_802_2	0x0011
#define DIW_ETH_P_MOBITEX	0x0015
#define DIW_ETH_P_CONTROL	0x0016
#define DIW_ETH_P_IRDA		0x0017
#define DIW_ETH_P_ECONET	0x0018
#define DIW_ETH_P_HDLC		0x0019
#define DIW_ETH_P_ARCNET	0x001A
#define DIW_ETH_P_DSA		0x001B
#define DIW_ETH_P_TRAILER	0x001C
#define DIW_ETH_P_PHONET	0x00F5
#define DIW_ETH_P_IEEE802154 0x00F6
#define DIW_ETH_P_CAIF		0x00F7

/// Ethernet frame header.
struct ethhdr
{
	unsigned char	h_dest[DIW_ETH_ALEN];
	unsigned char	h_source[DIW_ETH_ALEN];
	__be16		h_proto;
};

#endif /* _UAPI_DIW_IF_ETHER_H */
