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
#include "ota_update_ble_fw.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#if defined (__BLE_COMBO_REF__)
extern int	get_cur_boot_index(void);
extern UCHAR	set_boot_index(int index);
extern void combo_ble_sw_reset(void);
#if defined (__SUPPORT_ATCMD__)
extern void atcmd_asynchony_event(int index, char *buf);
#endif // (__SUPPORT_ATCMD__)

/*
	Depending on application policy, a specific bank (among bank1 and bank2)
	selection can be forced. In this case, 'ble_img_bank' should be properly updated (e.g.
	via nvram/network/etc...) before OTA update starts.
*/
static uint8_t ble_img_bank = ANY_IMAGE_BANK; /* by default, a bank that contains older imageid is selected */
static ble_new_fw_info_t new_ble_fw_info;
static ble_update_info_t ble_update_info;
ble_img_hdr_all_t ble_fw_hdrs_all;

static ota_update_sflash_t write_info;
unsigned int new_img_position;

uint8_t ota_update_ble_find_old_img(uint8_t id1, uint8_t id2)
{
    if (id1 == 0xFF && id2 == 0xFF) {
        return IMAGE_ID_1;
    }
    if (id1 == 0xFF && id2 == 0) {
        return IMAGE_ID_1;
    }
    if (id2 == 0xFF && id1 == 0) {
        return IMAGE_ID_2;
    }
    if (id1 > id2) {
        return IMAGE_ID_2;
    } else {
        return 1;
    }
}

uint8_t ota_update_ble_get_img_id(uint8_t imgid)
{
    uint8_t new_imgid = IMAGE_ID_0;

    if (imgid == 0xff) {
        return new_imgid;
    } else {
        new_imgid = (uint8_t)(imgid + 1);
        return new_imgid;
    }
}

/*
	ble img format (from server) = multi-part hdr (32) +
								   image_header_t (64) +
								   actual img bin


	version check policy:
			if new_fw and the current_fw are same version string and timestamp:
					> don't update

			else (the other case):
					> do update
*/
UINT ota_update_ble_check_version(uint8_t *data, uint32_t data_len)
{
    ble_img_hdr_t *pNewfwHeader;
    uint32_t new_fw_codesize;
    uint32_t imageposition1; /* should be multiple of 4 x (1024)K */
    uint32_t imageposition2; /* should be multiple of 4 x (1024)K */
    uint8_t new_bank = ANY_IMAGE_BANK;
    uint8_t is_invalid_image1 = IMAGE_HEADER_OK;
    uint8_t is_invalid_image2 = IMAGE_HEADER_OK;
    uint8_t imageid1 = IMAGE_ID_0;
    uint8_t imageid2 = IMAGE_ID_0;
    UINT ret_ble_ver_chk = IMAGE_HEADER_OK;

    memset(&new_ble_fw_info, 0x00, sizeof(new_ble_fw_info));

    new_ble_fw_info.is_first_packet = TRUE;
    new_ble_fw_info.is_hdr_checked = TRUE;

    /* ################################################################## */
    /* check new fw header */
    if ( data_len < sizeof(ble_img_hdr_t) ) {
        PRINTF("[BLE_OTA] ERR: 1st packet not containing a header! \n");
        new_ble_fw_info.hdr_error = TRUE;
        /* block size should be bigger than at least image header size */
        ret_ble_ver_chk = SUOTAR_INVAL_IMG_HDR;
        goto finish;

    } else {
        /* read header from the first data block */
        pNewfwHeader = (ble_img_hdr_t *) (data + sizeof(ble_multi_part_header_t));
    }

    /* verify new firmware signature */
    if ( (pNewfwHeader->signature[0] != IMAGE_HEADER_SIGNATURE1) ||
            (pNewfwHeader->signature[1] != IMAGE_HEADER_SIGNATURE2) ) {
        PRINTF("[BLE_OTA] ERR: New FW's signature is invalid! \n");
        new_ble_fw_info.hdr_error = TRUE;
        ret_ble_ver_chk = SUOTAR_INVAL_IMG_HDR;
        goto finish;
    }

    /* get image size */
    new_fw_codesize = pNewfwHeader->code_size; /* code size without header */

    /* ################################################################## */
    /* read product header */

    /* store the sflash address */
    imageposition1 = SFLASH_BLE_FW_BASE;
    imageposition2 = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset2;

    /* verify the new fw's image size */
    if (!new_fw_codesize ||
            new_fw_codesize > (imageposition2 - imageposition1 - sizeof(ble_img_hdr_t) - sizeof(ble_multi_part_header_t))) {
        PRINTF("[BLE_OTA] ERR: Wrong img. New FW's code size exceeds the bank size (32K)!\n");
        new_ble_fw_info.hdr_error = TRUE;
        ret_ble_ver_chk = SUOTAR_INVAL_IMG_SIZE;
        goto finish;
    }

    /* ################################################################## */
    /* read bank1 image header */

    imageid1 = ble_fw_hdrs_all.fw1_hdr.imageid;
    OTA_INFO("[BLE_OTA] bank1_img_id = %d\n", imageid1);

    if ( (ble_fw_hdrs_all.fw1_hdr.validflag != STATUS_VALID_IMAGE)         ||
            (ble_fw_hdrs_all.fw1_hdr.signature[0] != IMAGE_HEADER_SIGNATURE1) ||
            (ble_fw_hdrs_all.fw1_hdr.signature[1] != IMAGE_HEADER_SIGNATURE2)    ) {
        is_invalid_image1 = IMAGE_HEADER_INVALID;
        imageid1 = IMAGE_ID_0;
        OTA_INFO("[BLE_OTA] bank_1: Invalid BLE FW \n");
    } else {
        /* vaild image */
        /* Compare version string and timestamp */
        if (!memcmp(pNewfwHeader->version, ble_fw_hdrs_all.fw1_hdr.version, IMAGE_HEADER_VERSION_SIZE)
                && (pNewfwHeader->timestamp == ble_fw_hdrs_all.fw1_hdr.timestamp)) {
            is_invalid_image1 = IMAGE_HEADER_SAME_VERSION;
            OTA_INFO("[BLE_OTA] bank_1 %s: same version as new FW \n",
                     (ble_fw_hdrs_all.active_img_bank == 1) ? "(act)" : "");
        } else {
            OTA_INFO("[BLE_OTA] bank_1 %s: ver = v_%s, timestamp = %d \n",
                     (ble_fw_hdrs_all.active_img_bank == 1) ? "(act)" : "",
                     ble_fw_hdrs_all.fw1_hdr.version,
                     ble_fw_hdrs_all.fw1_hdr.timestamp);
        }
    }


    /* ################################################################## */
    /* read bank2 image header */

    imageid2 = ble_fw_hdrs_all.fw2_hdr.imageid;
    OTA_INFO("[BLE_OTA] bank2_img_id = %d\n", imageid2);

    if ( (ble_fw_hdrs_all.fw2_hdr.validflag != STATUS_VALID_IMAGE)         ||
            (ble_fw_hdrs_all.fw2_hdr.signature[0] != IMAGE_HEADER_SIGNATURE1) ||
            (ble_fw_hdrs_all.fw2_hdr.signature[1] != IMAGE_HEADER_SIGNATURE2)    ) {
        is_invalid_image2 = IMAGE_HEADER_INVALID;
        imageid2 = IMAGE_ID_0;
        OTA_INFO("[BLE_OTA] bank_2: Invalid BLE FW \n");
    } else {
        /* vaild image */

        /* compare version string and timestamp */
        if (!memcmp(pNewfwHeader->version, ble_fw_hdrs_all.fw2_hdr.version, IMAGE_HEADER_VERSION_SIZE) &&
                (pNewfwHeader->timestamp == ble_fw_hdrs_all.fw2_hdr.timestamp)) {
            is_invalid_image2 = IMAGE_HEADER_SAME_VERSION;
            OTA_INFO("[BLE_OTA] bank_2 %s: same version as new FW \n",
                     (ble_fw_hdrs_all.active_img_bank == 2) ? "(act)" : "");
        } else {
            OTA_INFO("[BLE_OTA] bank_2 %s: ver = v_%s, timestamp = %d \n",
                     (ble_fw_hdrs_all.active_img_bank == 2) ? "(act)" : "",
                     ble_fw_hdrs_all.fw2_hdr.version,
                     ble_fw_hdrs_all.fw2_hdr.timestamp);
        }
    }

    /* ################################################################## */
    /* decide image bank */
    if (ble_img_bank == ANY_IMAGE_BANK ||  ble_img_bank > SECOND_IMAGE_BANK) {
        if (is_invalid_image1 == IMAGE_HEADER_INVALID) {
            new_bank = FIRST_IMAGE_BANK;
        } else if (is_invalid_image2 == IMAGE_HEADER_INVALID) {
            new_bank = SECOND_IMAGE_BANK;
        } else {
            new_bank = ota_update_ble_find_old_img(imageid1, imageid2);
        }
    } else {
        /*
        	not used; ble_img_bank is always ANY_IMAGE_BANK by default. see ble_img_bank
        	if forcing a specific bank should allowed, ble_img_bank should be controlled
        	by application
        */
        new_bank = ble_img_bank;
    }
    new_ble_fw_info.img_bank = new_bank;
    OTA_INFO("[BLE_OTA] bank for new FW = %d\n", new_ble_fw_info.img_bank);

    /* ################################################################## */
    /* decide imageid */
    memset(&(new_ble_fw_info.fw_hdr), 0xFF, sizeof(ble_img_hdr_t));

    new_ble_fw_info.fw_hdr.imageid = IMAGE_ID_0;
    if (new_bank == SECOND_IMAGE_BANK) {
        if (is_invalid_image1 == IMAGE_HEADER_OK || is_invalid_image1 == IMAGE_HEADER_SAME_VERSION) {
            new_ble_fw_info.fw_hdr.imageid = ota_update_ble_get_img_id(imageid1);
        } else {
            new_ble_fw_info.fw_hdr.imageid = IMAGE_ID_1;
        }
    } else {
        if (is_invalid_image2 == IMAGE_HEADER_OK || is_invalid_image2 == IMAGE_HEADER_SAME_VERSION) {
            new_ble_fw_info.fw_hdr.imageid = ota_update_ble_get_img_id(imageid2);
        } else {
            new_ble_fw_info.fw_hdr.imageid = IMAGE_ID_1;
        }
    }
    OTA_INFO("[BLE_OTA] New FW img_id = %d\n", new_ble_fw_info.fw_hdr.imageid);

    /* write header apart from validflag. set validflag when the entire image has been received */
    new_ble_fw_info.fw_hdr.code_size =	pNewfwHeader->code_size;
    new_ble_fw_info.fw_hdr.CRC		 = pNewfwHeader->CRC;
    memcpy(new_ble_fw_info.fw_hdr.version, pNewfwHeader->version, IMAGE_HEADER_VERSION_SIZE);
    new_ble_fw_info.fw_hdr.timestamp   = pNewfwHeader->timestamp;
    new_ble_fw_info.fw_hdr.signature[0] = pNewfwHeader->signature[0];
    new_ble_fw_info.fw_hdr.signature[1] = pNewfwHeader->signature[1];
    new_ble_fw_info.fw_hdr.encryption  = pNewfwHeader->encryption;

    /*
    	new_ble_fw_info.new_fw_hdr will have a new fw header info except validflag
    	which will be written after CRC is verified.
    */
    if (new_ble_fw_info.img_bank == FIRST_IMAGE_BANK) {
        write_info.sflash_addr = SFLASH_BLE_FW_BASE;
    } else	{
        write_info.sflash_addr = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset2;
    }

    new_img_position = write_info.sflash_addr;

finish:

    if (ret_ble_ver_chk == SUOTAR_SAME_IMG_ERR) {
        PRINTF("[BLE_OTA] New FW is same version as the active FW. \n");
    } else if (ret_ble_ver_chk == SUOTAR_SAME_INACTIVE_IMG_ERR) {
        PRINTF("[BLE_OTA] New FW is the same version as the FW in the inactive bank. \n");
    }

    if (ret_ble_ver_chk != IMAGE_HEADER_OK) {
        PRINTF("[BLE_OTA] New FW version check NOT successful. Err_code (see ota_update_ble_fw.h, SUOTAR_xx) = 0x%x \n",
               ret_ble_ver_chk);
        return OTA_BLE_VERSION_UNKNOWN;
    }

    return OTA_SUCCESS;
}

UINT ota_update_ble_fw_reboot(void)
{
    UINT status = OTA_SUCCESS;
    UINT reboot_wait_cnt = 50; /* 5 secs */
#if defined (__SUPPORT_ATCMD__)
    char atc_buf[15] = {0, };

    memset(atc_buf, 0x00, sizeof(atc_buf));
    sprintf(atc_buf, "0x%02x", status);
    atcmd_asynchony_event(7, atc_buf);
#endif // (__SUPPORT_ATCMD__)

    PRINTF("\r\n %s \r\n..", __func__);

    combo_ble_sw_reset();

    while (reboot_wait_cnt-- > 0) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );

        if ((reboot_wait_cnt % 10) == 0) {
            PRINTF("\r- OTA: Reboot after %d secs ...", reboot_wait_cnt / 10);
        }
    }

    PRINTF("\n");
    reboot_func(SYS_REBOOT_POR);

    return status;
}

void ota_update_ble_chg_new_fw_validflag(uint8_t make_valid)
{
#define HDR_SIZE_MUL (sizeof(ble_multi_part_header_t))
#define HDR_SIZE_SINGLE (sizeof(ble_img_hdr_t))

    uint8_t header_buffer[HDR_SIZE_MUL + HDR_SIZE_SINGLE];
    int sflash_addr;

    memset(header_buffer, 0xFF, HDR_SIZE_MUL + HDR_SIZE_SINGLE);

    if (make_valid == TRUE) {
        new_ble_fw_info.fw_hdr.validflag = STATUS_VALID_IMAGE;
    } else {
        new_ble_fw_info.fw_hdr.validflag = STATUS_INVALID_IMAGE;
    }

    /* copy new fw header */
    memcpy(header_buffer + HDR_SIZE_MUL,
           &(new_ble_fw_info.fw_hdr),
           HDR_SIZE_SINGLE);

    if (new_ble_fw_info.img_bank == FIRST_IMAGE_BANK) {
        sflash_addr = SFLASH_BLE_FW_BASE;

        /* update header in sflash to reflect validflag */
        ble_sflash_write(sflash_addr,
                         (UCHAR *)header_buffer,
                         HDR_SIZE_MUL + HDR_SIZE_SINGLE);
        OTA_INFO("[BLE_OTA] bank1 hdr update, valid=%d\n", make_valid);
    } else {
        sflash_addr = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset2;

        /* update header in sflash to reflect validflag */
        ble_sflash_write(sflash_addr,
                         (UCHAR *)(header_buffer + HDR_SIZE_MUL),
                         HDR_SIZE_SINGLE);
        OTA_INFO("[BLE_OTA] bank2 hdr update, valid=%d\n", make_valid);
    }
}

void ota_update_ble_chg_new_fw_valid(void)
{
    /*
    	ble new fw : hdr_checked, header_error, download_progress

    	case1 : hdr_checked (x) -> http start error -> new_ble_fw_info is not init at all
    			do nothing

    	case2 : hdr_checked (o) + header_error (o)
    			--> do nothing

    	case3 : hdr_checked (o) + header_error (x) + regardless of download_progress (0~100)
    			--> make the new fw's candidate slot invalid, so that make the current ble fw
    			    intact.
    */

    if (new_ble_fw_info.is_hdr_checked == TRUE)	{
        if (new_ble_fw_info.hdr_error == FALSE)	{
            PRINTF("[BLE_OTA] ERR: Keep using the current BLE FW\n");
            ota_update_ble_chg_new_fw_validflag(FALSE);
        }
    }
}
static uint8_t findlatest(uint8_t id1, uint8_t id2)
{
    if (id1 == 0xFF && id2 == 0) {
        return 2;
    }

    if (id2 == 0xFF && id1 == 0) {
        return 1;
    }

    if (id1 >= id2) {
        return 1;
    } else {
        return 2;
    }
}

UINT ota_update_ble_get_fw_info(ble_img_hdr_t *bank1_info,
                                ble_img_hdr_t *bank2_info, int8_t *active_bank_id)
{
    uint8_t status = FALSE;
    uint32_t image1, image2;
    uint8_t images_status = 0;

    memset(&ble_fw_hdrs_all, 0xFF, sizeof(ble_img_hdr_all_t));

    status = (uint8_t)ble_sflash_read(PRODUCT_HEADER_POSITION, &ble_fw_hdrs_all.pd_hdr, sizeof(ble_product_header_t));
    if (status) {
        if (ble_fw_hdrs_all.pd_hdr.signature[0] != PRODUCT_HEADER_SIGNATURE1 ||
                ble_fw_hdrs_all.pd_hdr.signature[1] != PRODUCT_HEADER_SIGNATURE2) {
            DBG_ERR("invalid Product Header, program multi-part image again !\n");
            return 0x01;
        }

        image1 = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset1;
        image2 = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset2;

        if (bank1_info) {
            status = (uint8_t)ble_sflash_read(image1, bank1_info, sizeof(ble_img_hdr_t));
            if (status && (bank1_info->validflag == STATUS_VALID_IMAGE
                           && bank1_info->signature[0] == IMAGE_HEADER_SIGNATURE1
                           && bank1_info->signature[1] == IMAGE_HEADER_SIGNATURE2)) {
                images_status = 1;
            }
        }

        if (bank2_info) {
            status = (uint8_t)ble_sflash_read(image2, bank2_info, sizeof(ble_img_hdr_t));
            if (status && (bank2_info->validflag == STATUS_VALID_IMAGE
                           && bank2_info->signature[0] == IMAGE_HEADER_SIGNATURE1
                           && bank2_info->signature[1] == IMAGE_HEADER_SIGNATURE2)) {
                images_status = (uint8_t)(images_status + 2);
            }
        }

        if (images_status == 0) {
            PRINTF("No valid image found, program multi-part image set again !\n");
            *active_bank_id = -1;
        } else if (images_status == 3) {
            *active_bank_id = (findlatest(bank1_info->imageid, bank2_info->imageid) == 1) ? bank1_info->imageid : bank2_info->imageid;
        } else {
            *active_bank_id = (images_status == 1) ? bank1_info->imageid : bank2_info->imageid;
        }
    }

    return status ? 0x00 : 0x01;
}

void ota_update_ble_set_exist_url(ota_update_type update_type, uint8_t enable)
{
    if (update_type == OTA_TYPE_RTOS) {
        ble_update_info.exist_url_rtos = enable;
    } else if (update_type == OTA_TYPE_BLE_FW) {
        ble_update_info.exist_url_ble = enable;
    } else {
        OTA_ERR("- OTA: Wrong FW type (%d)\n", update_type);
    }
}

void ota_update_ble_determine_url(OTA_UPDATE_CONFIG *ota_update_conf)
{
    if ((strlen(ota_update_conf->url) > 0)
            && ((ota_update_conf->update_type == OTA_TYPE_RTOS)
                || (ota_update_conf->update_type == OTA_TYPE_BLE_COMBO))) {
        ble_update_info.exist_url_rtos = 1;
    }
    if (strlen(ota_update_conf->url_ble_fw) > 0) {
        ble_update_info.exist_url_ble = 1;
    }
}

UINT ota_update_ble_first_packet_offset(void)
{
    UINT offset = 0;

    if (new_ble_fw_info.is_first_packet == TRUE) {
        /* for writing to 2nd bank, flash write should start without multipart header */
        if (new_ble_fw_info.img_bank == SECOND_IMAGE_BANK) {
            offset += sizeof (ble_multi_part_header_t);
        }
        new_ble_fw_info.is_first_packet = FALSE;
    }

    return offset;
}

UINT ota_update_ble_crc_calcu(UCHAR *data, UINT len)
{
    uint8_t bin_offset = 0;
    UINT i = 0;

    if ((data == NULL) || (len <= 0)) {
        return OTA_ERROR_CRC;
    }

    /* in case of the first block, skip multi-part header */
    if (new_ble_fw_info.is_first_packet == TRUE) {
        bin_offset = sizeof(ble_multi_part_header_t) + sizeof(ble_img_hdr_t);
    }

    /*
    	Update CRC (XOR Checksum) for every block comes

    	NOTE: CRC calc may be different depending on which booter (on DA14531) is used
    		case1: default booter
    			XOR Checksum CRC is used by default DA14531 booter

    		case2: secondary booter (e.g. customized one where other CRC algo is used
    			- CRC32)
    			other CRC32/... should be used
    */
    for (i = 0; i < (len - bin_offset); i++) {
        new_ble_fw_info.crc_calculated ^= data[bin_offset + i];
    }

    return OTA_SUCCESS;
}

UINT ota_update_ble_crc(void)
{
    OTA_INFO("[BLE_OTA] BLE FW download done, verifying crc ... :  crc_calc = %d, crc_in_hdr = %d \n",
             (uint8_t)(new_ble_fw_info.crc_calculated), (uint8_t)(new_ble_fw_info.fw_hdr.CRC));

    if (((uint8_t)(new_ble_fw_info.crc_calculated)) == ((uint8_t)(new_ble_fw_info.fw_hdr.CRC)))	{
        /* tag as valid */
        ota_update_ble_chg_new_fw_validflag(TRUE);
        /* new_ble_fw_info.fw_hdr.validflag = STATUS_VALID_IMAGE; */
        return OTA_SUCCESS;
    } else {
        OTA_ERR("[BLE_OTA] ERR: CRC Err !\n");
        /* tag as invalid */
        ota_update_ble_chg_new_fw_validflag(FALSE);
        /* new_ble_fw_info.fw_hdr.validflag = STATUS_INVALID_IMAGE; */
        return OTA_ERROR_CRC;
    }
}

UINT ota_update_ble_only_download_status(void)
{
    UINT progress_ble = ota_update_get_download_progress(OTA_TYPE_BLE_FW);
    
    if ((ble_update_info.exist_url_ble == 1 && progress_ble == 100)
               && ble_update_info.exist_url_rtos == 0) {
        /* ble only ok */
        if (new_ble_fw_info.is_hdr_checked == TRUE
                && new_ble_fw_info.hdr_error == FALSE) {
            return OTA_SUCCESS;
        } else {
            return OTA_FAILED;
        }
    } else {
        return OTA_FAILED;
    }
}

UINT ota_update_ble_check_all_downloaded(void)
{
    UINT progress_rtos = ota_update_get_download_progress(OTA_TYPE_RTOS);
    UINT progress_ble = ota_update_get_download_progress(OTA_TYPE_BLE_FW);

    if ((ble_update_info.exist_url_ble == 1 && progress_ble == 100)
            && (ble_update_info.exist_url_rtos == 1
                && progress_rtos == 100)) {
        /* ble && rtos */
        OTA_INFO("- OTA: BLE FW, RTOS update\n");
        if (new_ble_fw_info.is_hdr_checked == TRUE
                && new_ble_fw_info.hdr_error == FALSE) {
            return OTA_SUCCESS;
        }

    } else if ((ble_update_info.exist_url_ble == 1 && progress_ble == 100)
               && ble_update_info.exist_url_rtos == 0) {
        /* ble only ok */
        OTA_INFO("- OTA: BLE FW only update\n");
        if (new_ble_fw_info.is_hdr_checked == TRUE
                && new_ble_fw_info.hdr_error == FALSE) {
            return OTA_SUCCESS;
        }
    } else if (ble_update_info.exist_url_ble == 0
               && (ble_update_info.exist_url_rtos == 1 && progress_rtos == 100)) {
        /* rtos only ok */
        OTA_INFO("- OTA: RTOS only update\n");
        return OTA_SUCCESS;
    }

    ble_update_info.exist_url_rtos = 0;
    ble_update_info.exist_url_ble = 0;
    OTA_ERR("- OTA: Download failed - %s (%d %) \n", OTA_RTOS_NAME, progress_rtos);
    OTA_ERR("- OTA: Download failed - %s (%d %) \n", OTA_BLE_FW_NAME, progress_ble);

    return OTA_NOT_ALL_DOWNLOAD;
}

#endif	// (__BLE_COMBO_REF__)
#endif	//( __SUPPORT_OTA__)

/* EOF */
