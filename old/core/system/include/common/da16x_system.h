/**
 ****************************************************************************************
 *
 * @file da16x_system.h
 *
 * @brief Define for System utilities
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


#ifndef __DA16X_SYSTEM_H__
#define __DA16X_SYSTEM_H__

#include "project_config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "schd_trace.h"
#include "schd_system.h"
#include "schd_idle.h"
#include "da16200_map.h"
#include "da16200_retmem.h"

/******************************************************************************
 *
 *  FPGA Model
 *
 ******************************************************************************/

#define     PRINTF(...)        trc_que_proc_print(0, __VA_ARGS__ )
#define     VPRINTF(...)    trc_que_proc_vprint(0, __VA_ARGS__ )
#define     GETC()            trc_que_proc_getchar(portMAX_DELAY)
#define     GETC_NOWAIT()    trc_que_proc_getchar(portNO_DELAY)
#define     PUTC(ch)        trc_que_proc_print(0, "%c", ch )
#define     PUTS(s)            trc_que_proc_print(0, s )

#define     VSIM_PRINTF(...)

/******************************************************************************
 *
 *  Common Model
 *
 ******************************************************************************/

#ifdef    SUPPORT_MEM_PROFILE
  #define SYSTEM_MALLOC(...)    profile_oal_malloc( __func__, __LINE__, __VA_ARGS__ )
  #define SYSTEM_FREE(...)    profile_oal_free( __func__, __LINE__, __VA_ARGS__ )
  #define APP_MALLOC(...)        profile_app_malloc( __func__, __LINE__, __VA_ARGS__ )
  #define APP_FREE(...)        profile_app_free( __func__, __LINE__, __VA_ARGS__ )
#else    //SUPPORT_MEM_PROFILE
  #define   SYSTEM_MALLOC       pvPortMalloc
  #define   SYSTEM_FREE         vPortFree
  #define   APP_MALLOC          pvPortMalloc
  #define   APP_FREE            vPortFree
#endif    //SUPPORT_MEM_PROFILE

#define APP_STRDUP                app_strdup

/******************************************************************************
 *
 *  System Features
 *
 ******************************************************************************/

#define DA16X_CONSOLE_PORT                   UART_UNIT_0
#define DA16X_BAUDRATE                       230400
#define DA16X_BOOT_CLOCK                     DA16X_SYSTEM_CLOCK
#define DA16X_XTAL_CLOCK                     DA16X_SYSTEM_XTAL
#define DA16X_UART_CLOCK                     (DA16X_SYSTEM_XTAL*2)

#define DA16X_NVRAM_NOR                      0x300000
#define DA16X_NVRAM_SFLASH                   SFLASH_NVRAM_ADDR
#define DA16X_NVRAM_FOOT_PRINT               SFLASH_NVRAM_FOOTPRINT
#define DA16X_NVRAM_RAM                      (DA16X_FPGA_ZBT_BASE|0x000F0000)

/// FLASH Offset of 1st Boot (2nd Boot or RTOS)
#define DA16X_FLASH_BOOT_OFFSET              0x000000

/// FLASH Offset of RamLIB & PTIM
#define DA16X_FLASH_PTIM_OFFSET              0x18E000
#define DA16X_PHYSICAL_FLASH_RALIB0_OFFSET   0x18A000
#define DA16X_PHYSICAL_FLASH_RALIB1_OFFSET   0x38A000 /* where ? */

#define DA16X_FLASH_RALIB_OFFSET             (*(UINT32*)0x00F802D0)     /* CM = RA + PTIM  0xF1000 */

/******************************************************************************
 *
 *  RTM Base Address
 *
 ******************************************************************************/
#define DA16X_RTM_RAMLIB_SIZE           0x2000
#define DA16X_RTM_PTIM_SIZE             0x57F8
#define DA16X_RTM_UMAC_SIZE             0x00A0
#define DA16X_RTM_APP_SUPP_SIZE         0x0760
#define DA16X_RTM_VAR_SIZE              0x0AC0


#define DA16X_RETMEM_END                DA16200_RETMEM_END


/// The size of a user RTM used by full-boot. The maximum size is 8K. And the
/// size of (DA16X_RTM_USER_SIZE + DA16X_RTM_PTIM_USER_SIZE) should be less than
/// 8K.
#define DA16X_RTM_USER_SIZE             0x2000
/// The size of a user RTM used by PTIM. The maximum size is 8K. And the size
/// of (DA16X_RTM_USER_SIZE + DA16X_RTM_PTIM_USER_SIZE) should be less than 8K.
#define DA16X_RTM_PTIM_USER_SIZE        0x0000

/// UMAC RTM base      : 0x0300
#define DA16X_RTM_UMAC_BASE             RETMEM_LMAC_BASE

/// SUPP RTM base      : 0x03A0
#define DA16X_RTM_APP_SUPP_BASE         (DA16X_RTM_UMAC_BASE + DA16X_RTM_UMAC_SIZE)

/// DPM RTM base       : 0x0B00
#define DA16X_RTM_VAR_BASE              (DA16X_RTM_APP_SUPP_BASE + DA16X_RTM_APP_SUPP_SIZE)

/// RA image RTM base  : 0x15C0
#define DA16X_RETM_RALIB_OFFSET         (DA16X_RTM_VAR_BASE + DA16X_RTM_VAR_SIZE)

/// PTIM image RTM base: 0x35C0
#define DA16X_RETM_PTIM_OFFSET          (DA16X_RETM_RALIB_OFFSET + DA16X_RTM_RAMLIB_SIZE)

/// User RTM base      : 0xA000
#define DA16X_RTM_USER_BASE             (  DA16X_RETMEM_BASE                 \
                                         + DA16X_RETMEM_SIZE                 \
                                         - DA16X_RTM_USER_SIZE               \
                                         - DA16X_RTM_PTIM_USER_SIZE)
/// User RTM base      : 0xC000
#define DA16X_RTM_PTIM_USER_BASE        (DA16X_RTM_USER_BASE + DA16X_RTM_USER_SIZE)

/// OTP SFLASH Configuration Offset
#define DA16X_OTP_SFLASH_ADDR            0x0000        /* SLR model */
#define DA16X_OTP_IOVOLT_ADDR            0x0400        /* SLR model */

// SRAM Offset of ATE-TEST
#define DA16X_SRAM_BOOT_MODEL0           (DA16X_SRAM_BASE|0x0B800)  // 1st
#define DA16X_SRAM_BOOT_MODEL1           (DA16X_SRAM_BASE|0x10000)  // 2nd

// FLASH Offset for 2nd Boot (UEBoot)
// NOTICE: Do NOT modify this definition !!

#define DA16X_FLASH_RTOS_MODEL0          SFLASH_RTOS_0_BASE
#define DA16X_FLASH_RTOS_MODEL1          SFLASH_RTOS_1_BASE
#define DA16X_SFLASH_BOOT_OFFSET         SFLASH_BOOT_INDEX_BASE

/******************************************************************************
 *
 *  Boot Features
 *
 ******************************************************************************/

#define SYS_FALT_BOOT_MASK               0xFFFF0000

/******************************************************************************
 *
 *  Test Features
 *
 ******************************************************************************/


#undef  SUPPORT_PERI_TEST
#undef  SUPPORT_HEAP_TEST ///< Don't touch

#if defined(RAM_2ND_BOOT)
  #define SUPPORT_NVRAM_CLOCK_SPEED
#elif defined(XIP_CACHE_BOOT)
  #undef  SUPPORT_NVRAM_CLOCK_SPEED
#else
    #da16x_system.h Config error...
#endif

/******************************************************************************
 *
 *  DPM Features
 *
 ******************************************************************************/
#include "da16x_dpm_system.h"

#define GET_DPMP()                      ((struct dpm_param *) (DA16X_RTM_VAR_BASE))


#endif /* __DA16X_SYSTEM_H__ */

/* EOF */
