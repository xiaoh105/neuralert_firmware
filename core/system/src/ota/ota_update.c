/**
 ****************************************************************************************
 *
 * @file ota_common.c
 *
 * @brief Over the air firmware update module
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

#if defined (__SUPPORT_OTA__)
#include "FreeRTOSConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "da16x_types.h"
#include "sys_cfg.h"
#include "common_def.h"
#include "common_config.h"
#include "command.h"
#include "command_net.h"
#include "environ.h"
#include "util_api.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "ota_update_http.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"


UINT ota_update_cmd_parse(int argc, char *argv[])
{
    return ota_update_cli_cmd_parse(argc, argv);
}

UINT ota_update_read_flash(UINT addr, VOID *buf, UINT len)
{
    HANDLE  sflash_handler;
    UINT	status = OTA_SUCCESS;

    /* Open SFLASH device */
    sflash_handler = flash_image_open( (sizeof(UINT32) * 8),
                                       addr,
                                       (VOID *)&da16x_environ_lock);

    if (sflash_handler == NULL) {
        PRINTF("- SFLASH access error\n");
        return	OTA_FAILED;
    }

    memset(buf, 0, len);

    /* Read SFLASH user area */
    status = flash_image_read(sflash_handler,
                              addr,
                              (VOID *)buf,
                              len);

    if (status == 0) {
        // Read Error
        PRINTF("- SFLASH read error : addr=0x%x, size=%d\n", addr, len);
        return OTA_FAILED;
    }

    /* Close SFLASH device */
    flash_image_close(sflash_handler);

    return OTA_SUCCESS;
}

UINT ota_update_erase_flash(UINT flash_addr, UINT size)
{
    HANDLE flash_handler;
    UINT erase_size = 0;

    flash_handler = flash_image_open((sizeof(UINT32) * 8),
                                     flash_addr,
                                     (VOID *)&da16x_environ_lock);

    if (flash_handler == NULL) {
        return 0;
    }

    erase_size = flash_image_erase(flash_handler,
                                   (UINT32)flash_addr,
                                   size); //4096 byte

    flash_image_close(flash_handler);

    return erase_size;
}

UINT ota_update_copy_flash(UINT dest_addr, UINT src_addr, UINT len)
{
    UINT  offset = 0, cnt = 0;
    UINT8   *imgbuffer;
    HANDLE  handler;
    UINT status = OTA_FAILED;

    if ((dest_addr % 0x1000) || (src_addr % 0x1000)) {
        PRINTF("[%s] sflash address unit must be 4Kbyte.\n", __func__);
        return OTA_ERROR_SFLASH_ADDR;
    }

    imgbuffer = (UINT8 *)OTA_MALLOC(4096);
    if (imgbuffer == NULL) {
        PRINTF("[%s] Fail to alloc memory\n", __func__);
        return OTA_MEM_ALLOC_FAILED;
    }

    handler = (HANDLE)flash_image_open((sizeof(UINT32) * 8), src_addr, (VOID *)&da16x_environ_lock);
    if (handler == NULL) {
        PRINTF("[%s] Fail to get Handler\n", __func__);
        goto finish;
    }

    cnt = len / 4096;
    if (len % 4096) {
        cnt = cnt + 1;
    }

    while (cnt > 0) {
        if (flash_image_read(handler, (src_addr + offset), imgbuffer, 4096) > 0) {
            if (flash_image_write(handler, (dest_addr + offset), imgbuffer, 4096) == 0) {
                PRINTF(ANSI_COLOR_RED " sflash write error \n" ANSI_COLOR_DEFULT);
                goto finish;
            }
        } else {
            PRINTF(ANSI_COLOR_RED " sflash read error \n" ANSI_COLOR_DEFULT);
            goto finish;
        }
        offset += 0x1000; //4096 byte
        cnt--;
    }
    status = OTA_SUCCESS;

finish:
    if (imgbuffer != NULL) {
        OTA_FREE(imgbuffer);
    }

    flash_image_close(handler);

    return status;
}


UINT ota_update_start_download(OTA_UPDATE_CONFIG *ota_update_conf)
{
    UINT status = OTA_SUCCESS;

    status = ota_update_process_create(ota_update_conf);

    return status;
}

UINT ota_update_stop_download(void)
{
    UINT status = OTA_SUCCESS;

    status = ota_update_process_stop();

    return status;
}

UINT ota_update_start_renew(OTA_UPDATE_CONFIG *ota_update_conf)
{
    UINT status = OTA_SUCCESS;
    UINT reboot_wait_cnt = 50; /* 5 secs */

    status = ota_update_check_state();
    if (status == OTA_SUCCESS) {
        status = ota_update_current_fw_renew();
    }

    ota_update_status_atcmd(7, status);

    if ((ota_update_conf != NULL) && (ota_update_conf->renew_notify != NULL)) {
        ota_update_conf->renew_notify(status);
    }

    if (status == OTA_SUCCESS) {
#if defined (__BLE_COMBO_REF__)
        if (ota_update_get_download_progress(OTA_TYPE_BLE_FW) == 100) {
            DBG_INFO("[BLE_OTA] BLE only or Both Wi-FI and BLE F/W to be renewed...\n");
        }
        combo_ble_sw_reset();
#endif	// (__BLE_COMBO_REF__)
        while (reboot_wait_cnt-- > 0) {
            vTaskDelay( 100 / portTICK_PERIOD_MS );

            if ((reboot_wait_cnt % 10) == 0) {
                PRINTF("\r- OTA: Reboot after %d secs ...", reboot_wait_cnt / 10);
            }
        }
        PRINTF("\n");

        reboot_func(SYS_REBOOT_POR);
    }

    return status;
}

UINT ota_update_get_progress(ota_update_type update_type)
{
    return ota_update_get_download_progress(update_type);
}

UINT ota_update_set_tls_auth_mode_nvram(int tls_auth_mode)
{
    return ota_update_httpc_set_tls_auth_mode_nvram(tls_auth_mode);
}

#endif	// ( __SUPPORT_OTA__)

/* EOF */
