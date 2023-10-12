/**
 ****************************************************************************************
 *
 * @file target.h
 *
 * @brief DA16200 Platform header
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


#ifndef __TARGET_H__
#define __TARGET_H__

#include "da16200_addr.h"

#define DA16X_SYSCLOCK           DA16200_SYSCLOCK
#define DA16X_SRAM32S_BASE       DA16200_SRAM32S_BASE
#define DA16X_DIOCFG             DA16200_DIOCFG
#define DA16X_WLAN_BBM_BASE      DA16200_WLAN_BBM_BASE

#define DA16X_FDAC14             DA16200_FDAC14
#define DA16X_DAC10C             DA16200_DAC10C
#define DA16X_RFMDMC             DA16200_RFMDMC
#define DA16X_COMMON             DA16200_COMMON

#define DA16X_BOOTCON            DA16200_BOOTCON
#define DA16X_RETMEM_BASE        DA16200_RETMEM_BASE
#define DA16X_RETMEM_SIZE        DA16200_RETMEM_SIZE
#define DA16X_SRAM_BASE          DA16200_SRAM_BASE
#define DA16X_AIPCFG             DA16200_AIPCFG
#define DA16X_SYS_SECURE_BASE    DA16200_SYS_SECURE_BASE

#define DA16X_FPGA_ZBT_BASE      DA16200_FPGA_ZBT_BASE
#define DA16X_APB_BASE_TOP       DA16200_APB_BASE_TOP

#endif /* __TARGET_H__ */

/* EOF */
