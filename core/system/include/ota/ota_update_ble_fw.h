/**
 ****************************************************************************************
 *
 * @file ota_update_ble_fw.h
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

#if !defined (__OTA_UPDATE_BLE_FW_H__)
#define	__OTA_UPDATE_BLE_FW_H__

#if defined (__SUPPORT_OTA__)
#include <stdio.h>
#include "application.h"

#include "da16200_map.h"
#include "da16x_image.h"
#include "ota_update.h"
#include "ota_update_common.h"
#if defined (__BLE_COMBO_REF__)
#include "da16_gtl_features.h"

/*
* The PRODUCT_HEADER_OFFSET(now 0xFF20) must be same as the header_position in create_img.bat at:
*  (.\projects\target_apps\ble_examples\prox_reporter_sensor_ext_coex\Keil_5) or
*  (.\projects\target_apps\ble_examples\prox_monitor_aux_ext_coex\Keil_5)
*  when build the DA14531 SDK
*/
#define PRODUCT_HEADER_OFFSET       (__BLE_IMG_SIZE__ - 0xE0)

#define PRODUCT_HEADER_POSITION     (SFLASH_BLE_FW_BASE + PRODUCT_HEADER_OFFSET)
#define PRODUCT_HEADER_SIGNATURE1   0x70
#define PRODUCT_HEADER_SIGNATURE2   0x52
#define IMAGE_HEADER_SIGNATURE1     0x70
#define IMAGE_HEADER_SIGNATURE2     0x51
#define IMAGE_HEADER_VERSION_SIZE   16
#define STATUS_VALID_IMAGE          0xAA
#define STATUS_INVALID_IMAGE        0xDD
#define IMAGE_HEADER_OK             0
#define IMAGE_HEADER_INVALID        1
#define IMAGE_HEADER_SAME_VERSION   2
#define ANY_IMAGE_BANK              0
#define FIRST_IMAGE_BANK            1
#define SECOND_IMAGE_BANK           2
#define IMAGE_ID_0                  0
#define IMAGE_ID_1                  1
#define IMAGE_ID_2                  2

// SUOTA Status values
enum {
    SUOTAR_RESERVED         = 0x00,     // Value zero must not be used !! Notifications are sent when status changes.
    SUOTAR_SRV_STARTED      = 0x01,     // Valid memory device has been configured by initiator. No sleep state while in this mode
    SUOTAR_CMP_OK           = 0x02,     // SUOTA process completed successfully.
    SUOTAR_SRV_EXIT         = 0x03,     // Forced exit of SUOTAR service.
    SUOTAR_CRC_ERR          = 0x04,     // Overall Patch Data CRC failed
    SUOTAR_PATCH_LEN_ERR    = 0x05,     // Received patch Length not equal to PATCH_LEN characteristic value
    SUOTAR_EXT_MEM_WRITE_ERR = 0x06,    // External Mem Error (Writing to external device failed)
    SUOTAR_INT_MEM_ERR      = 0x07,     // Internal Mem Error (not enough space for Patch)
    SUOTAR_INVAL_MEM_TYPE   = 0x08,     // Invalid memory device
    SUOTAR_APP_ERROR        = 0x09,     // Application error

    // SUOTAR application specific error codes
    SUOTAR_IMG_STARTED      = 0x10,     // SUOTA started for downloading image (SUOTA application)
    SUOTAR_INVAL_IMG_BANK   = 0x11,     // Invalid image bank
    SUOTAR_INVAL_IMG_HDR    = 0x12,     // Invalid image header
    SUOTAR_INVAL_IMG_SIZE   = 0x13,     // Invalid image size
    SUOTAR_INVAL_PRODUCT_HDR = 0x14,    // Invalid product header
    SUOTAR_SAME_IMG_ERR     = 0x15,     // Same Image Error (in active slot)
    SUOTAR_EXT_MEM_READ_ERR = 0x16,     // Failed to read from external memory device
    SUOTAR_SAME_INACTIVE_IMG_ERR	= 0x17,     // Same Image in inactive slot
};



// BLE multi-part header (SUOTA format)
typedef struct an_b_001_spi_header {
    uint8_t preamble[2];
    uint8_t empty[4];
    uint8_t len[2];
    uint8_t reserved[24];
}	ble_multi_part_header_t;

// BLE single image header (SUOTA format)
typedef struct {
    uint8_t signature[2];
    uint8_t validflag;      // Set to STATUS_VALID_IMAGE at the end of the image update
    uint8_t imageid;        // it is used to determine which image is the newest
    uint32_t code_size;     // Image size
    uint32_t CRC ;          // Image CRC
    uint8_t version[16];    // Vertion of the image
    uint32_t timestamp;
    uint8_t encryption;
    uint8_t reserved[31];
}	ble_img_hdr_t;

// BLE product header (SUOTA format)
typedef struct {
    uint8_t signature[2];
    uint8_t version[2];
    uint32_t offset1; // Start address of first image header
    uint32_t offset2; // Start address of second image header
}	ble_product_header_t;

typedef struct {
    uint8_t img_bank; // slot for new img

    uint8_t is_first_packet;
    ble_img_hdr_t fw_hdr;
    uint8_t same_img_in_inactive_bank;
    uint32_t crc_calculated;
    uint8_t hdr_error;
    uint8_t is_hdr_checked;
}	ble_new_fw_info_t;

typedef struct __fwInfo {
    uint8_t active_img_bank;
    uint32_t img_pos;
    uint32_t code_size;
    uint32_t crc;
    uint8_t imageid;
    uint8_t version[16];
    uint8_t is_encrypted;
}	ble_act_fw_info_t;

typedef struct __multiImgHeader {
    int active_img_bank;

    ble_product_header_t pd_hdr;
    ble_multi_part_header_t mp_hdr;
    ble_img_hdr_t fw1_hdr;
    ble_img_hdr_t fw2_hdr;
}	ble_img_hdr_all_t;

typedef struct __updateInfo {
    uint8_t exist_url_ble;
    uint8_t exist_url_rtos;
    uint32_t progress_ble;
    uint32_t progress_rtos;

}	ble_update_info_t;

void ota_update_ble_set_exist_url(ota_update_type update_type, uint8_t enable);
void ota_update_ble_determine_url(OTA_UPDATE_CONFIG *ota_update_conf);
UINT ota_update_ble_first_packet_offset(void);
UINT ota_update_ble_crc_calcu(UCHAR *data, UINT len);
UINT ota_update_ble_crc(void);
void ota_update_ble_chg_new_fw_validflag(uint8_t make_valid);
UINT ota_update_ble_check_version(uint8_t *data, uint32_t data_len);
UINT ota_update_ble_check_all_downloaded(void);
void ota_update_ble_chg_new_fw_valid(void);
UINT ota_update_ble_get_fw_info(ble_img_hdr_t *bank1_info, ble_img_hdr_t *bank2_info, int8_t *active_bank_id);

#endif /* (__BLE_COMBO_REF__) */
#endif	/* (__SUPPORT_OTA__) */
#endif	/* (__OTA_UPDATE_BLE_FW_H__) */

/* EOF */
