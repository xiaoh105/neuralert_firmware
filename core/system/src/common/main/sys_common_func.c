/**
 ****************************************************************************************
 *
 * @file sys_common_func.c
 *
 * @brief System common functions
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


#include "sdk_type.h"

#include "common_def.h"
#include "da16x_system.h"

#include "rtc.h"
#include "sys_feature.h"
#include "da16x_image.h"


//////////////////////////////////////////////////////////////////////////////
/// Wakeup Source Related API function      //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief    DPM Wakeup source
 *
 * typedef enum _WAKEUP_SOURCE_ {
 *    // internal reset add
 *    WAKEUP_RESET                                        = 0x00,
 *
 *    // boot from extern wake up signal
 *    WAKEUP_SOURCE_EXT_SIGNAL                            = 0x01,
 *
 *    // boot from wake up counter
 *    WAKEUP_SOURCE_WAKEUP_COUNTER                        = 0x02,
 *
 *    // boot from wakeup counter or external wakeup sig
 *    WAKEUP_EXT_SIG_WAKEUP_COUNTER                       = 0x03,
 *
 *    // boot from power on reset
 *    WAKEUP_SOURCE_POR                                   = 0x04,
 *
 *    // boot from watch dog add
 *    WAKEUP_WATCHDOG                                     = 0x08,
 *
 *    // boot from watch dog or external wakeup sig
 *    WAKEUP_WATCHDOG_EXT_SIGNAL                          = 0x09,
 *
 *    // boot from internal reset and have retention add
 *    WAKEUP_RESET_WITH_RETENTION                         = 0x10,
 *
 *    WAKEUP_EXT_SIG_WITH_RETENTION                       = 0x11,
 *    WAKEUP_COUNTER_WITH_RETENTION                       = 0x12,
 *
 *    // boot from external signal or wakeup counter with retention
 *    WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION        = 0x13,
 *
 *    // boot from watch dog with retention add
 *    WAKEUP_WATCHDOG_WITH_RETENTION                      = 0x18,
 *
 *    // boot from watch dog or external wakeup sig
 *    WAKEUP_WATCHDOG_EXT_SIGNAL_WITH_RETENTION           = 0x19,
 * } WAKEUP_SOURCE;
 *
 */


/* POR4 , TIM, KEPP, UMAC RTC up 0xa, reboot 8 */

/* Default Wakeup source is POR */
WAKEUP_SOURCE source = (WAKEUP_SOURCE )WAKEUP_SOURCE_POR;

void dpm_save_wakeup_source( int wakeupsource )
{
    source = (WAKEUP_SOURCE)wakeupsource;
}

int dpm_get_wakeup_source(void)
{
    return (int)source;
}

int chk_rtm_exist(void)
{
    int bootType;

    /* Booting Scenario and Checkin */
    bootType = dpm_get_wakeup_source();

    /*
    *  0x01 : boot from extern wake up signal   sleep mode 1
    *  0x02 : boot form wake up counter     sleep mode 2
    *  0x04 : boot from power on reset
    *  0x08 :
    *  0x09 :
    *  0x0a : sleep mode 3 & DPM
    */

    /* Sleep mode 1 or Sleep mode 2 or POR or REBOOT(8)
     *    --> Normal full booting */

    switch (bootType) {
    case WAKEUP_SOURCE_EXT_SIGNAL :
    case WAKEUP_SOURCE_WAKEUP_COUNTER :
        return pdFALSE;    // Not-exist

    case WAKEUP_SOURCE_POR :
    case WAKEUP_WATCHDOG :
    case WAKEUP_RESET :
    case WAKEUP_RESET_WITH_RETENTION :
    case WAKEUP_EXT_SIG_WITH_RETENTION :
    case WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION :
        return pdTRUE;    // Exist

    default:
        break;
    }

    return pdTRUE;    // Exist
}

int chk_ext_int_exist(void)
{
    if (dpm_get_wakeup_source() & WAKEUP_SOURCE_EXT_SIGNAL) {
        return pdTRUE;    // Exist
    }

    return pdFALSE;    // No Exist
}

#define BOOT_IMG_VER        0
#define SLIB_IMG_VER        1
#define RTOS_IMG_VER        2

#define VER_STR_LEN         0x20

#define RED_COL             "\33[1;31m"
#define YEL_COL             "\33[1;33m"
#define CLR_COL             "\33[0m"

void chk_boot_fw_version(void)
{
    UCHAR    get_ver[3][VER_STR_LEN] = { 0, };
    UCHAR    ver_error_flag = pdFALSE;

    if (version_chk_flag == pdFALSE) {
        return;
    }

    if (!chk_dpm_wakeup()) {
        get_firmware_version(BOOT_IMG_VER, (UCHAR *)&get_ver[BOOT_IMG_VER]);
        get_firmware_version(SLIB_IMG_VER, (UCHAR *)&get_ver[SLIB_IMG_VER]);
        get_firmware_version(RTOS_IMG_VER, (UCHAR *)&get_ver[RTOS_IMG_VER]);

        /* Check Secure Boot */
        if (strncmp((char *)(&get_ver[BOOT_IMG_VER][0]), "EncUE-BOOT", 10) == 0) { /* Secure Boot: EncUE-BOOT */
            /* Secure Bootloader */
            /* Check SLIB Secure  IMG */
            if (strncmp((char *)(&get_ver[SLIB_IMG_VER]), "SSLIB", 5) != 0) { /* Secure SLIB: SSLIB */
                if (ver_error_flag == pdFALSE) {
                    PRINTF(YEL_COL "\nF/W Version :\n");
                    PRINTF("  %s\n", &get_ver[BOOT_IMG_VER]);
                    PRINTF("  %s\n", &get_ver[SLIB_IMG_VER]);
                    PRINTF("  %s\n\n" CLR_COL, &get_ver[RTOS_IMG_VER]);
                }

                PRINTF(RED_COL "!!! Notice !!!\n" CLR_COL);
                //PRINTF("[%s] Secure BOOT IMG & SLIB Non-Secure IMG\n", __func__);
                ver_error_flag = pdTRUE;
            }
        } else {
            /* Non Secure Bootloader */
            /* Check SLIB Secure  IMG */
            if (strncmp((char *)(&get_ver[SLIB_IMG_VER]), "NSLIB", 5) != 0) {  /* Non-Secure SLIB: NSLIB */
                if (ver_error_flag == pdFALSE) {
                    PRINTF(YEL_COL "\nF/W Version :\n");
                    PRINTF("  %s\n", &get_ver[BOOT_IMG_VER]);
                    PRINTF("  %s\n", &get_ver[SLIB_IMG_VER]);
                    PRINTF("  %s\n\n" CLR_COL, &get_ver[RTOS_IMG_VER]);
                }

                PRINTF(RED_COL "!!! Notice !!!\n" CLR_COL);
                ver_error_flag = pdTRUE;
            }
        }

        if (ver_error_flag == pdTRUE) {
            PRINTF(" FW version - " RED_COL "Secure Boot" CLR_COL " mismatch !!!\n");
        }

        /* Compare Vendor ID
        DA16200_xRTOS_XXXX-01-4576-000000.img
                    ^^^^
        */
        if (strncmp((char *)(&get_ver[SLIB_IMG_VER][7]), (char *)(&get_ver[RTOS_IMG_VER][7]), 5) != 0) {
            if (ver_error_flag == pdFALSE) {
                PRINTF(YEL_COL "\nF/W Version :\n");
                PRINTF("  %s\n", &get_ver[BOOT_IMG_VER]);
                PRINTF("  %s\n" CLR_COL, &get_ver[RTOS_IMG_VER]);
                PRINTF("  %s\n\n", &get_ver[SLIB_IMG_VER]);
                PRINTF(RED_COL "!!! Notice !!!\n" CLR_COL);

                ver_error_flag = pdTRUE;
            }

            PRINTF(" FW version - " RED_COL "Vendor ID" CLR_COL " mismatch !!!\n");
        }

        /*
        Compare Major Number
        DA16200_xRTOS_GEN01-xx-4576-000000.img
                          ^^
        */
        if (strncmp((char *)(&get_ver[SLIB_IMG_VER][13]), (char *)(&get_ver[RTOS_IMG_VER][13]), 2) != 0) {
            if (ver_error_flag == pdFALSE) {
                PRINTF(YEL_COL "\nF/W Version :\n");
                PRINTF("  %s\n", &get_ver[BOOT_IMG_VER]);
                PRINTF("  %s\n", &get_ver[RTOS_IMG_VER]);
                PRINTF("  %s\n\n" CLR_COL, &get_ver[SLIB_IMG_VER]);
                PRINTF(RED_COL "!!! Notice !!!\n" CLR_COL);

                ver_error_flag = pdTRUE;
            }

            PRINTF(" FW version - " RED_COL "Major Number" CLR_COL " mismatch !!!\n");
        }

        if (ver_error_flag == pdTRUE) {
            PRINTF(" Check and update with correct F/W.\n\n");
            reboot_func(0); /* reset */

            while (1) {
                vTaskDelay(10);
            }
        }
    }
}

#ifdef INDEPENDENT_FLASH_SIZE
UCHAR set_boot_index(int index)
{
    if (index == IMAGE0_INDEX) {
        write_boot_offset(index, DA16X_FLASH_RTOS_MODEL0);
    } else if (index == IMAGE1_INDEX) {
        write_boot_offset(index, DA16X_FLASH_RTOS_MODEL1);
    } else {
        PRINTF(RED_COLOR " [%s] Wrong booting index ... (index: %d)\n" CLEAR_COLOR, __func__, index);
        return pdFALSE;
    }

    return pdTRUE;
}

int get_cur_boot_index(void)
{
    int boot_offset = 0xff;
    int offset_address = 0;

    boot_offset = read_boot_offset(&offset_address);

    return boot_offset;
}

#else    /* INDEPENDENT_FLASH_SIZE */

#define BOOT_IDX_0           "DA16200_IMAGE_ZERO"
#define BOOT_IDX_1           "DA16200_IMAGE_ONE"
#define BOOT_IDX_0_STRLEN    18
#define BOOT_IDX_1_STRLEN    17

static int boot_index = -1;

UCHAR set_boot_index(int index)
{
    HANDLE  handler;
    UCHAR    *data_buf = NULL;

    data_buf = (void *)pvPortMalloc(SF_SECTOR_SZ);
    if (data_buf == NULL) {
        PRINTF("[%s] Failed to alloc data_buf ...\n", __func__);
        return pdFALSE;
    }

    memset(data_buf, 0, SF_SECTOR_SZ);
    handler = flash_image_open((sizeof(UINT)*8), SFLASH_BOOT_INDEX_BASE, (VOID *)&da16x_environ_lock);

    flash_image_read(handler, SFLASH_BOOT_INDEX_BASE, (void *)data_buf, SF_SECTOR_SZ);

    /* Clear index string */
    memset(data_buf, 0, 32);

    switch (index) {
    case 0 :
        strncpy((char *)data_buf, BOOT_IDX_0, BOOT_IDX_0_STRLEN);
        break;

    case 1 :
        strncpy((char *)data_buf, BOOT_IDX_1, BOOT_IDX_1_STRLEN);
        break;

    default :
        PRINTF("[%s] Wrong booting index ... (index = %d)\n", __func__, index);
        goto func_fin;
    }

    boot_index = index;

    flash_image_write(handler, SFLASH_BOOT_INDEX_BASE, (VOID *)data_buf, SF_SECTOR_SZ);

func_fin :
    flash_image_close(handler);
    vPortFree(data_buf);

    return pdTRUE;
}

int get_cur_boot_index(void)
{

    if (boot_index == -1) {
        HANDLE  handler;
        UCHAR    *data_buf = NULL;

        data_buf = (void *)pvPortMalloc(SF_SECTOR_SZ);

        if (data_buf == NULL) {
            PRINTF("[%s] Failed to alloc data_buf ...\n", __func__);
            return pdFALSE;
        }

        memset(data_buf, 0, SF_SECTOR_SZ);
        handler = flash_image_open((sizeof(UINT) * 8), (UINT32)SFLASH_BOOT_INDEX_BASE, (VOID *)&da16x_environ_lock);

        flash_image_read(handler, SFLASH_BOOT_INDEX_BASE, (void *)data_buf, SF_SECTOR_SZ);

        flash_image_close(handler);

        if (strncmp((char *)data_buf, BOOT_IDX_0, BOOT_IDX_0_STRLEN) == 0 || data_buf[0] == 0xFF) {
            boot_index = 0;
        } else if (strncmp((char *)data_buf, BOOT_IDX_1, BOOT_IDX_1_STRLEN) == 0) {
            boot_index = 1;
        } else {
            boot_index = 0;
        }

        vPortFree(data_buf);
    }

    return boot_index;
}
#endif    /* INDEPENDENT_FLASH_SIZE */

void (*wifi_conn_notify_cb)(void) = NULL;
void (*wifi_conn_fail_notify_cb)(short reason_code) = NULL;
void (*wifi_disconn_notify_cb)(short reason_code) = NULL;
void (*ap_sta_disconnected_notify_cb)(const unsigned char mac[6]) = NULL;

int wifi_conn_fail_reason_code;
int wifi_disconn_reason_code;
    
void wifi_conn_notify_cb_regist(void (*user_cb)(void))
{
    wifi_conn_notify_cb = user_cb;
}

void wifi_conn_fail_notify_cb_regist(void (*user_cb)(short reason_code))
{
    wifi_conn_fail_notify_cb = user_cb;
}

void wifi_disconn_notify_cb_regist(void (*user_cb)(short reason_code))
{
    wifi_disconn_notify_cb = user_cb;
}

void ap_sta_disconnected_notify_cb_regist(void (*user_cb)(const unsigned char mac[6]))
{
	ap_sta_disconnected_notify_cb = user_cb;
}

/* EOF */
