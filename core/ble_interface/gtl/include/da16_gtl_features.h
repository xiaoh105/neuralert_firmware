/**
 ****************************************************************************************
 *
 * @file da16_gtl_features.h
 *
 * @brief Feature definitions commonly used for samples
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

#ifndef __DA16_GTL_FEATURES_H__
#define __DA16_GTL_FEATURES_H__

/*
to avoid duplicate definition warning,
please import the value from ble SDK's rwip_config.h
whenever you migrate to a new SDK
*/
#define BLE_DEEP_SLEEP        1

/* the minimum and maximum macros */
#undef min
#define min( a , b ) ( ( (a) < (b) ) ? (a) : (b) )

#undef max
#define max( a , b ) ( ( (a) > (b) ) ? (a) : (b) )

// PRINTF font color
#define RED_COL		"\33[1;31m"
#define GRN_COL		"\33[1;32m"
#define YEL_COL		"\33[1;33m"
#define BLU_COL		"\33[1;34m"
#define MAG_COL		"\33[1;35m"
#define CYN_COL		"\33[1;36m"
#define WHT_COL		"\33[1;37m"
#define CLR_COL		"\33[0m"

#undef DBG_PRINT_TEMP

#if defined (__GTL_DBG_PRINT__)
#define DBG_PRINT_INFO
#define DBG_PRINT_ERR
#else
#undef DBG_PRINT_INFO
#define DBG_PRINT_ERR
#endif

#if defined (DBG_PRINT_INFO)
#define DBG_INFO	PRINTF
#else
#define DBG_INFO(...)	do { } while (0);
#endif

#if defined (DBG_PRINT_ERR)
#define DBG_ERR		PRINTF
#else
#define DBG_ERR(...)	do { } while (0);
#endif

#if defined (DBG_PRINT_TEMP)
#define DBG_TMP		PRINTF
#else
#define DBG_TMP(...)	do { } while (0);
#endif

#if defined (DBG_PRINT_DUMP)
#define DBG_DUMP		PRINTF
#else
#define DBG_DUMP(...)	do { } while (0);
#endif

#if defined (DBG_PRINT_LOW)
#define DBG_LOW		PRINTF
#else
#define DBG_LOW(...)	do { } while (0);
#endif

#if defined (__USE_APP_MALLOC__)
#define MEMALLOC APP_MALLOC
#define JMEMFREE APP_FREE
#define MEMFREE(p) if(p) {APP_FREE(p); p=NULL;}
#else
#define MEMALLOC malloc
#define MEMFREE free
#endif

#define APP_FW_VERSION_STRING_MAX_LENGTH (24)

#define DEMO_PEER_MAX_MTU (251)
#define ATT_HEADER_SIZE (7)

#endif // __DA16_GTL_FEATURES_H__

