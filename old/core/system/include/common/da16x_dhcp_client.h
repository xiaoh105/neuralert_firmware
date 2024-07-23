/**
 ****************************************************************************************
 *
 * @file da16x_dhcp_client.h
 *
 * @brief Define for DHCP Client Module
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


#ifndef    __DA16X_DHCP_CLIENT_H__
#define    __DA16X_DHCP_CLIENT_H__

#include "sdk_type.h"

#include "da16x_system.h"
#include "iface_defs.h"
#include "common_def.h"

#include "da16x_ip_handler.h"
#include "lwip/netif.h"
#include "lwip/prot/dhcp.h"
#include "lwip/dhcp.h"

#include "da16x_dpm_regs.h"
#include "user_dpm.h"
#include "da16x_network_common.h"

#define DHCP_INFINITE_LEASE  0xFFFFFFFFUL

void rtc_dhcp_renew_timeout(char *timer_name);


#endif /* __DA16X_DHCP_CLIENT_H__ */

/* EOF */
