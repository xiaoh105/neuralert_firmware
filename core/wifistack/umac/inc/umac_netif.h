/**
 ****************************************************************************************
 *
 * @file umac_netif.h
 *
 * @brief Header file for WiFi MAC Protocol Network Stack & Driver Interface
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

#ifndef __UMAC_NETIF_H
#define __UMAC_NETIF_H

#define MAX_WIFI_DRIVER_INTERFACES	2	//3
#define WLAN0_IFACE			0
#define WLAN1_IFACE			1


typedef struct MAC_ADDRESS_STRUCT
{
	ULONG mac_addr_msw;
	ULONG mac_addr_lsw;
} WIFI_MAC_ADDRESS;

typedef struct _wifi_network_driver_instance_type
{
	UINT                wifi_network_driver_in_use;

	UINT                wifi_network_driver_id;

	struct netif        *wifi_driver_interface_ptr;

	struct ip_globals   *wifi_driver_ip_ptr;

	WIFI_MAC_ADDRESS    wifi_driver_mac_address;

	UINT				wifi_network_driver_in_tx;

	/* Network stack can not receive the Data */
	bool 				wifi_network_stop_rx;
} _wifi_network_driver_instance_type;

void wifi_network_driver_thread();
void *get_net_stack_poolid(int intf);
bool get_wifi_driver_rxing(int intf);

#ifdef FEATURE_NEW_QUEUE


typedef struct LWIP_IPI_STRUCT
{
	struct pbuf	*nx_ip_driver_deferred_packet_head;
	struct pbuf	*nx_ip_driver_deferred_packet_tail;
} IP_TEMP;

extern IP_TEMP temp_ip_temp;
extern IP_TEMP temp_ip_temp_rx;

extern EventGroupHandle_t    ip_temp_queue;
extern SemaphoreHandle_t ip_temp_queue_sem;

extern EventGroupHandle_t    ip_temp_queue_rx;
extern SemaphoreHandle_t ip_temp_queue_sem_rx;

#endif /* FEATURE_NEW_QUEUE */

#endif /* __UMAC_NETIF_H */
