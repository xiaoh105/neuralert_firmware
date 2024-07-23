/**
 ****************************************************************************************
 *
 * @file ota_update_sample.c
 *
 * @brief Sample app of OTA update.
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


#if defined	(__OTA_UPDATE_SAMPLE__)
/**
*****************************************************************************************
* In case of DPM mode enabled :
* If OTA update is in progress (firmware download is in progress), it does not enter DPM Sleep because SFLASH write operation occurs.
* After downloading the firmware, the DA16200 enables state to enter DPM sleep mode.
*****************************************************************************************
*/

#include "sdk_type.h"
#include "sample_defs.h"
#include "iface_defs.h"
#include "da16x_system.h"
#include "ota_update.h"
#include "command.h"
#include "command_net.h"
#include "da16x_network_common.h"

#undef SAMPLE_UPDATE_DA16_FW
#undef SAMPLE_UPDATE_MCU_FW
#undef SAMPLE_UPDATE_CERT_KEY

#define UNKNOWN_STATUS	0xFF

#ifdef SAMPLE_UPDATE_DA16_FW
/* This is an example server address. Change the server address to suit your environment. */
static char ota_server_url_rtos[256] = "https://192.168.0.101/DA16200_RTOS-GEN01-01-12345-000001.img";
static UINT da16_rtos_download_notify;
static UINT da16_renew_notify;
#endif // SAMPLE_UPDATE_DA16_FW

#ifdef SAMPLE_UPDATE_MCU_FW
/* This is an example server address. Change the server address to suit your environment. */
static char ota_server_url_mcu[256] = "https://192.168.0.101/user_mcu_firmware.img";
static UINT mcu_fw_download_notify;
#endif // SAMPLE_UPDATE_MCU_FW

#ifdef SAMPLE_UPDATE_CERT_KEY
/* This is an example server address. Change the server address to suit your environment. */
static char ota_server_url_cert[256] = "https://192.168.0.101/certificate.pem";
static UINT cert_key_download_notify;
#endif //SAMPLE_UPDATE_CERT_KEY

#if defined(SAMPLE_UPDATE_DA16_FW) || defined(SAMPLE_UPDATE_MCU_FW) || defined(SAMPLE_UPDATE_CERT_KEY)
static OTA_UPDATE_CONFIG ota_update_conf = { 0, };
static OTA_UPDATE_CONFIG *g_ota_update_conf = (OTA_UPDATE_CONFIG *) &ota_update_conf;
#endif /*SAMPLE_UPDATE_DA16_FW || SAMPLE_UPDATE_MCU_FW || SAMPLE_UPDATE_CERT_KEY*/
//static OTA_UPDATE_CONFIG g_ota_update_conf;

#ifdef SAMPLE_UPDATE_DA16_FW
static void user_sample_da16_fw_download_notify
(ota_update_type update_type, UINT status, UINT progress)
{
    if (update_type == OTA_TYPE_RTOS) {
        da16_rtos_download_notify = status;
        PRINTF("[%s] status = 0x%02x, progress = %d\n", __func__, status, progress);
    }
}

static void user_sample_da16_fw_renew_notify(UINT status)
{
    da16_renew_notify = status;
    PRINTF("[%s] status = 0x%02x\n", __func__, status);
}

static UINT user_sample_da16_fw_update(void)
{
    UINT status = OTA_SUCCESS;

    PRINTF("\n\n>>> Start sample : DA16200 FW update \n\n\n");

    memset(g_ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));

    /* Setting the type to be updated */
    g_ota_update_conf->update_type = OTA_TYPE_RTOS;

    /* url setting example - Change it to suit your environment. */
    memcpy(g_ota_update_conf->url, ota_server_url_rtos, strlen(ota_server_url_rtos));

    /* If you downloaded the FW successfully, the boot_index will change automatically. */
    g_ota_update_conf->auto_renew = 0;

    /* Called when the download of each FW is finished. (Success or Fail)  */
    g_ota_update_conf->download_notify = user_sample_da16_fw_download_notify;

    /* Called when the renew(change boot index) is complete (If it succeeds, it will reboot automatically.)  */
    g_ota_update_conf->renew_notify = user_sample_da16_fw_renew_notify;

    /* Flag to confirm callback */
    da16_rtos_download_notify = UNKNOWN_STATUS;
    da16_renew_notify = UNKNOWN_STATUS;

    /* Start the fw update process */
    status = ota_update_start_download(g_ota_update_conf);
    if (status) {
        PRINTF("[%s] OTA update start fail (0x%02x)\n", __func__, status);
        return status;
    }

    /* Waiting for download notification */
    while (1) {
        vTaskDelay( 10 / portTICK_PERIOD_MS );
        if (da16_rtos_download_notify != UNKNOWN_STATUS) {
            PRINTF("[%s] Download is complete (RTOS status = 0x%02x)\n",
                   __func__, da16_rtos_download_notify);
            break;
        }
    }

    if (da16_rtos_download_notify != OTA_SUCCESS) {
        return OTA_FAILED;
    }

    /* Waiting for renew notification */
    if (g_ota_update_conf->auto_renew == 1) {
        while (1) {
            vTaskDelay( 10 / portTICK_PERIOD_MS );
            if (da16_renew_notify != UNKNOWN_STATUS) {
                break;
            }
        }

        if (da16_renew_notify != OTA_SUCCESS) {
            return OTA_FAILED;
        }
    }

    return OTA_SUCCESS;
}

/* If auto_renew is not set to true (g_ota_update_conf->auto_renew = 0), you can renew it using the API. */
/* If you set auto_renew to true(g_ota_update_conf->auto_renew = 1), you do not need to call ota_update_start_renew().*/
void user_sample_run_only_renew(void)
{
    UINT status;

    g_ota_update_conf->renew_notify = user_sample_da16_fw_renew_notify;

    status = ota_update_start_renew(g_ota_update_conf);
    if (status == OTA_SUCCESS) {
        /* SUCCESS => REBOOT*/
        return;
    }
    PRINTF("[%s] OTA update renew fail (0x%02x)\n", __func__, status);
}
#endif //SAMPLE_UPDATE_DA16_FW

#ifdef SAMPLE_UPDATE_MCU_FW
static void user_sample_mcu_fw_download_notify
(ota_update_type update_type, UINT status, UINT progress)
{
    DA16X_UNUSED_ARG(update_type);
    DA16X_UNUSED_ARG(progress);

    mcu_fw_download_notify = status;
}

static UINT user_sample_update_mcu_fw(void)
{
    UINT status = OTA_SUCCESS;

    PRINTF("\n\n>>> Start sample : MCU FW update \n\n\n");

    memset(g_ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));

    /* Setting the type to be updated */
    g_ota_update_conf->update_type = OTA_TYPE_MCU_FW;

    /* url setting example - Change it to suit your environment. */
    memcpy(g_ota_update_conf->url, ota_server_url_mcu, strlen(ota_server_url_mcu));

    /* Users can specify the download location. If not specified, it defaults to the User Area address.*/
    g_ota_update_conf->download_sflash_addr = SFLASH_USER_AREA_0_START;

    /* Called when the download of each FW is finished. (Success or Fail)  */
    g_ota_update_conf->download_notify = user_sample_mcu_fw_download_notify;

    /* Flag to confirm callback */
    mcu_fw_download_notify = UNKNOWN_STATUS;

    /* Start the fw update process */
    status = ota_update_start_download(g_ota_update_conf);
    if (status) {
        PRINTF("[%s] OTA update start fail (0x%02x)\n", __func__, status);
        return status;
    }

    /* Waiting for download notification */
    while (1) {
        vTaskDelay( 10 / portTICK_PERIOD_MS );
        if (mcu_fw_download_notify != UNKNOWN_STATUS) {
            PRINTF("[%s] Download is complete (MCU FW status = 0x%02x)\n",
                   __func__, mcu_fw_download_notify);
            break;
        }
    }

    if (mcu_fw_download_notify == OTA_SUCCESS) {
        status = ota_update_trans_mcu_fw();
    }

    return status;
}
#endif // SAMPLE_UPDATE_MCU_FW

#ifdef SAMPLE_UPDATE_CERT_KEY
static void user_sample_cert_key_download_notify
(ota_update_type update_type, UINT status, UINT progress)
{
    DA16X_UNUSED_ARG(update_type);
    DA16X_UNUSED_ARG(progress);

    cert_key_download_notify = status;
}

static UINT user_sample_cert_key(void)
{
    UINT status = OTA_SUCCESS;

    PRINTF("\n\n>>> Start sample : CERT/KEY update \n\n\n");

    memset(g_ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));

    /* Setting the type to be updated */
    g_ota_update_conf->update_type = OTA_TYPE_CERT_KEY;

    /* url setting example - Change it to suit your environment. */
    memcpy(g_ota_update_conf->url, ota_server_url_cert, strlen(ota_server_url_cert));

    /* Users can specify the download location. If not specified, it defaults to the User Area address.*/
    g_ota_update_conf->download_sflash_addr = SFLASH_USER_AREA_0_START;

    /* Called when the download of each FW is finished. (Success or Fail)  */
    g_ota_update_conf->download_notify = user_sample_cert_key_download_notify;

    /* Flag to confirm callback */
    cert_key_download_notify = UNKNOWN_STATUS;

    /* Start the fw update process */
    status = ota_update_start_download(g_ota_update_conf);
    if (status) {
        PRINTF("[%s] OTA update start fail (0x%02x)\n", __func__, status);
        return status;
    }

    /* Waiting for download notification */
    while (1) {
        vTaskDelay( 10 / portTICK_PERIOD_MS );
        if (cert_key_download_notify != UNKNOWN_STATUS) {
            PRINTF("[%s] Download is complete (CERT/KEY status = 0x%02x)\n",
                   __func__, cert_key_download_notify);
            break;
        }
    }

    if (cert_key_download_notify == OTA_SUCCESS) {
        /* Please check the SFLASH address. */
        status = ota_update_copy_flash(SFLASH_ROOT_CA_ADDR1, g_ota_update_conf->download_sflash_addr, 4096);
        /**
        status = ota_update_copy_flash(SFLASH_CERTIFICATE_ADDR1, g_ota_update_conf->download_sflash_addr, 4096);
        status = ota_update_copy_flash(SFLASH_PRIVATE_KEY_ADDR1, g_ota_update_conf->download_sflash_addr, 4096);
        status = ota_update_copy_flash(SFLASH_DH_PARAMETER1, g_ota_update_conf->download_sflash_addr, 4096);
        **/
    }

    return status;
}
#endif //SAMPLE_UPDATE_CERT_KEY

void ota_update_sample_entry(void *param)
{
    DA16X_UNUSED_ARG(param);

    int wait_cnt = 0;
    int iface = WLAN0_IFACE;

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            PRINTF("\r\n [%s] OTA update : No network connection\r\n", __func__);
            wait_cnt = 0;
            goto finish;
        }
    }

#ifdef SAMPLE_UPDATE_MCU_FW
    /* Update firmware of MCU other than da16200 */
    user_sample_update_mcu_fw();
#endif // SAMPLE_UPDATE_MCU_FW

#ifdef SAMPLE_UPDATE_CERT_KEY
    /* Update required certificate or key in TLS */
    user_sample_cert_key();
#endif	//SAMPLE_UPDATE_CERT_KEY

#ifdef SAMPLE_UPDATE_DA16_FW
    /* Update the RTOS and SLIB of da16200 */
    user_sample_da16_fw_update();
#endif // SAMPLE_UPDATE_DA16_FW

finish:
    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}
#endif	// (__OTA_UPDATE_SAMPLE__)

/* EOF */
