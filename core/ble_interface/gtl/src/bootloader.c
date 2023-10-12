/**
 ****************************************************************************************
 *
 * @file bootloader.c
 *
 * @brief bootloader application
 *
 * Copyright (c) 2012-2022 Renesas Electronics. All rights reserved.
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

#if defined (__BLE_COMBO_REF__) && defined (__BLE_FW_VER_CHK_IN_OTA__)

#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"

#include "da16_gtl_features.h"
#include "ota_update_ble_fw.h"

extern ble_img_hdr_all_t ble_fw_hdrs_all;
extern void	readFlashData(UINT sflash_addr, UINT read_len, char *sflash_data);

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Return the bank index of the active (latest and valid) image
 * @param[in] destination_buffer: buffer to put the data
 * @param[in] source_addr: starting position to read the data from spi
 * @param[in] len: size of data to read
 ****************************************************************************************
 */
static uint8_t findlatest(uint8_t id1, uint8_t id2)
{
    if (id1 == 0xFF && id2 == 0)
    {
        return 2;
    }

    if (id2 == 0xFF && id1 == 0)
    {
        return 1;
    }

    if (id1 >= id2)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

/**
 ****************************************************************************************
 * @brief Load the active (latest and valid) image from a non-volatile memory
 * @return Success (0) or Error Code.
 *
 ****************************************************************************************
 */
int loadActiveImage(ble_act_fw_info_t* selected_fw_info)
{
    uint32_t imageposition1;
    uint32_t imageposition2;
    uint8_t activeImage = 0;
    uint8_t images_status = 0;

	memset(&ble_fw_hdrs_all, 0xFF, sizeof(ble_img_hdr_all_t));

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// read Product Header 
    readFlashData((UINT)PRODUCT_HEADER_POSITION, sizeof(ble_product_header_t), 
    				(char*)(&(ble_fw_hdrs_all.pd_hdr)));
	
    // verify product header
    if (ble_fw_hdrs_all.pd_hdr.signature[0] != PRODUCT_HEADER_SIGNATURE1 ||
        ble_fw_hdrs_all.pd_hdr.signature[1] != PRODUCT_HEADER_SIGNATURE2)
    {
		DBG_ERR("invalid Product Header, program multi-part image again !\n");
        return -1;
    }

    imageposition1 = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset1;
    imageposition2 = SFLASH_BLE_FW_BASE + ble_fw_hdrs_all.pd_hdr.offset2;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// read Multi-part header
	readFlashData((UINT)SFLASH_BLE_FW_BASE, sizeof(ble_multi_part_header_t), 
				  (char*)(&(ble_fw_hdrs_all.mp_hdr)));

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// read fw_1 header	
	readFlashData((UINT)imageposition1, sizeof(ble_img_hdr_t), 
				  (char*)(&(ble_fw_hdrs_all.fw1_hdr)));
	
    if (ble_fw_hdrs_all.fw1_hdr.validflag == STATUS_VALID_IMAGE &&
        ble_fw_hdrs_all.fw1_hdr.signature[0] == IMAGE_HEADER_SIGNATURE1 &&
        ble_fw_hdrs_all.fw1_hdr.signature[1] == IMAGE_HEADER_SIGNATURE2)
    {
        images_status = 1;
    }

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// read fw_2 header	
	readFlashData((UINT)imageposition2, sizeof(ble_img_hdr_t), 
				  (char*)(&(ble_fw_hdrs_all.fw2_hdr)));
		
    if (ble_fw_hdrs_all.fw2_hdr.validflag == STATUS_VALID_IMAGE &&
        ble_fw_hdrs_all.fw2_hdr.signature[0] == IMAGE_HEADER_SIGNATURE1 &&
        ble_fw_hdrs_all.fw2_hdr.signature[1] == IMAGE_HEADER_SIGNATURE2)
    {
        images_status += 2;
    }

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	// find the latest image to load
    if (images_status == 3)
    {
    	// both images valid
        activeImage = findlatest(ble_fw_hdrs_all.fw1_hdr.imageid, 
        						 ble_fw_hdrs_all.fw2_hdr.imageid);
    }
    else if (images_status == 0)
    {
    	// both images not valid
    	PRINTF("No valid image found, program multi-part image set again !\n");
    	return -1;
    }
	else
    {
    	// only one image (1 or 2) is valid
        activeImage = images_status;
    }

	ble_fw_hdrs_all.active_img_bank = activeImage;
	
	// return selected image info
	if (activeImage == 1)
	{
		selected_fw_info->active_img_bank = ble_fw_hdrs_all.active_img_bank;
		selected_fw_info->code_size = ble_fw_hdrs_all.fw1_hdr.code_size;
		memcpy(selected_fw_info->version, ble_fw_hdrs_all.fw1_hdr.version, IMAGE_HEADER_VERSION_SIZE);
		selected_fw_info->crc = ble_fw_hdrs_all.fw1_hdr.CRC;
		selected_fw_info->imageid = ble_fw_hdrs_all.fw1_hdr.imageid;
		selected_fw_info->is_encrypted = ble_fw_hdrs_all.fw1_hdr.encryption;
		selected_fw_info->img_pos = imageposition1 + sizeof(ble_img_hdr_t);
	}
	else if (activeImage == 2)
	{
		selected_fw_info->active_img_bank = ble_fw_hdrs_all.active_img_bank;
		selected_fw_info->code_size = ble_fw_hdrs_all.fw2_hdr.code_size;
		memcpy(selected_fw_info->version, ble_fw_hdrs_all.fw2_hdr.version, IMAGE_HEADER_VERSION_SIZE);
		selected_fw_info->crc = ble_fw_hdrs_all.fw2_hdr.CRC;
		selected_fw_info->imageid = ble_fw_hdrs_all.fw2_hdr.imageid;
		selected_fw_info->is_encrypted = ble_fw_hdrs_all.fw2_hdr.encryption;
		selected_fw_info->img_pos = imageposition2 + sizeof(ble_img_hdr_t);
	}
	
    return pdPASS; // success
}

#endif	/* (__BLE_COMBO_REF__) && defined (__BLE_FW_VER_CHK_IN_OTA__) */

/* EOF */
