/**
 ****************************************************************************************
 *
 * @file ota_mcu_update.c
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
#include "da16x_types.h"
#include "sys_cfg.h"
#include "common_def.h"
#include "common_config.h"
#include "common_utils.h"
#include "command.h"
#include "command_net.h"
#include "environ.h"
#include "util_api.h"
#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif // (__SUPPORT_ATCMD__)
#include "ota_update.h"
#include "ota_update_common.h"
#include "ota_update_mcu_fw.h"
#include "common_uart.h"
#if defined (__BLE_COMBO_REF__)
#include "ota_update_ble_fw.h"
#endif // (__BLE_COMBO_REF__)

#undef DEBUG_OTA_MCU_DUMP

#if defined (__OTA_UPDATE_MCU_FW__)
#if defined (__SUPPORT_UART1__) || defined (__SUPPORT_UART2__)
extern HANDLE		uart1;
extern HANDLE		uart2;
#else //SPI || SPIO
extern int host_response(unsigned int buf_addr, unsigned int len, unsigned int resp,
						 unsigned int padding_bytes);
#endif

#define OTA_MCU_BUF_SIZE	(2048)

static UCHAR g_mcu_fw_name[OTA_MCU_FW_NAME_LEN] = OTA_MCU_FW_NAME;
static ota_update_download_t by_mcu;
static UINT MCU_TX_SIZE = 0;

static const unsigned int ota_crc_table[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

/* update the CRC on the data block one byte at a time */
static unsigned int update_crc (unsigned int init, const unsigned char *buf, int len)
{
    unsigned int crc = init;

    while (len--) {
        crc = ota_crc_table[(crc ^ * (buf++)) & 0xFF] ^ (crc >> 8);
    }

    return ~crc;
}

UINT ota_update_calcu_mcu_fw_crc(int sflash_addr, int size)
{
    HANDLE	handler;
    unsigned char	*data = NULL;
    unsigned int	addr, length, numBlocks, i;
    unsigned int 	crc_flash = 0;

    addr = sflash_addr;
    length = size;

    data = (unsigned char *)OTA_MALLOC(4096);
    if (data == NULL) {
        return 0;
    }

    handler = flash_image_open((sizeof(UINT) * 8), addr, (VOID *)&da16x_environ_lock);

    numBlocks = length / 4096;

    for (i = 0; i < numBlocks; i++) {
        flash_image_read(handler, addr, (void *)data, 4096);
        length -= 4096;
        addr += 4096;

        crc_flash = update_crc(~crc_flash, (unsigned char *)data, 4096);
    }

    if (length > 0) {
        flash_image_read(handler, addr, (void *)data, length );

        crc_flash = update_crc(~crc_flash, (unsigned char *)data, length);
    }

    flash_image_close(handler);
    OTA_FREE(data);

    return crc_flash;
}

void ota_update_iface_printf(const char *fmt, ...)
{
    va_list ap;
    int len;
    char *atcmd_print_buf = NULL;

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__SUPPORT_UART1__) || defined(__SUPPORT_UART2__)
    int is_txfifo_empty = FALSE;
    extern int chk_dpm_mode(void);
#endif // defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

    atcmd_print_buf = pvPortMalloc(UART_PRINT_BUF_SIZE);
    if (atcmd_print_buf == NULL) {
    	PRINTF("- [%s] Failed to allocate the print buffer\n", __func__);
    	return;
    }

    memset(atcmd_print_buf, 0x00, UART_PRINT_BUF_SIZE);

    va_start(ap, fmt);
    len = da16x_vsnprintf(atcmd_print_buf, UART_PRINT_BUF_SIZE, 0, (const char *)fmt, ap);
    va_end(ap);

#if	defined(__SUPPORT_UART1__)
    UART_WRITE(uart1, atcmd_print_buf, len);
#elif defined(__SUPPORT_UART2__)
    UART_WRITE(uart2, atcmd_print_buf, len);
#else //SPI || SPIO
    int real_data_len = len;
    len = (((len - 1) / 4) + 1 ) * 4;
    host_response((unsigned int)(atcmd_print_buf), len, 0x83, len - real_data_len);
#endif

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__SUPPORT_UART1__) || defined(__SUPPORT_UART2__)
    if (chk_dpm_mode()) {
        do {
#if	defined(__SUPPORT_UART1__)
            UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#elif defined(__SUPPORT_UART2__)
            UART_IOCTL(uart2, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#endif
        } while (is_txfifo_empty == FALSE);
    }
#endif // (__SUPPORT_UART1__) ||(__SUPPORT_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

    vPortFree(atcmd_print_buf);
    atcmd_print_buf = NULL;

}

void ota_update_iface_puts(char *data, int data_len)
{
#if defined(__SUPPORT_UART1__)
    UART_WRITE(uart1, data, data_len);
#elif defined(__SUPPORT_UART2__)
    UART_WRITE(uart2, data, data_len);
#else //SPI || SPIO
    int real_data_len = data_len;
    data_len = (((data_len - 1) / 4) + 1 ) * 4;
    host_response((unsigned int)(data), data_len, 0x83, data_len - real_data_len);
#endif
}

UINT ota_update_gen_mcu_header(UINT size)
{
    UINT mcu_fw_sflash_addr = 0;
    ota_mcu_fw_info_t ota_mcu_fw_info;
    UCHAR *temp_buf = NULL;


    mcu_fw_sflash_addr = ota_update_get_curr_sflash_addr(OTA_TYPE_MCU_FW);
    ota_mcu_fw_info.size = size;
    ota_mcu_fw_info.crc = ota_update_calcu_mcu_fw_crc(
                              (int)(mcu_fw_sflash_addr + OTA_MCU_FW_HEADER_SIZE),
                              (int)(ota_mcu_fw_info.size));


    memset(&ota_mcu_fw_info.name[0], 0x00, sizeof(ota_mcu_fw_info.name));
    ota_update_get_mcu_fw_name(&ota_mcu_fw_info.name[0]);
    if (strlen(ota_mcu_fw_info.name) == 0) {
        strncpy(&ota_mcu_fw_info.name[0], OTA_MCU_FW_NAME, strlen(OTA_MCU_FW_NAME));
    }

    temp_buf = OTA_MALLOC(OTA_SFLASH_BUF_SZ);
    if (temp_buf != NULL) {
        memset(temp_buf, 0x00, OTA_SFLASH_BUF_SZ);
        ota_update_read_flash(mcu_fw_sflash_addr,
                              temp_buf,
                              OTA_SFLASH_BUF_SZ);

        /* Inserting MCU information in front of MCU FW */
        memcpy(temp_buf, &ota_mcu_fw_info, sizeof(ota_mcu_fw_info_t));
        OTA_INFO("Insert MCU FW header info\n");
        OTA_INFO(" Version-------- %s \n", ota_mcu_fw_info.name);
        OTA_INFO(" Data Size------ %d \n", ota_mcu_fw_info.size);
        OTA_INFO(" HCRC----------- 0x%x \n", ota_mcu_fw_info.crc);
        if (writeDataToFlash(mcu_fw_sflash_addr,
                             temp_buf,
                             (UINT)OTA_SFLASH_BUF_SZ) != TRUE) {
            OTA_ERR("[%s] Failed to write MCU information\n", __func__);
            return OTA_FAILED;
        }
    } else {
        return OTA_FAILED;
    }

    return OTA_SUCCESS;
}

UINT ota_update_set_mcu_fw_name(char *name)
{
    if (name == NULL) {
        return OTA_FAILED;
    }

    if ((strlen(name) > 0) && (strlen(name) < OTA_MCU_FW_NAME_LEN)) {
        memset(&g_mcu_fw_name[0], 0x00, OTA_MCU_FW_NAME_LEN);
        memcpy(&g_mcu_fw_name[0], name, OTA_MCU_FW_NAME_LEN);
    } else {
        return OTA_FAILED;
    }

    return OTA_SUCCESS;
}

UINT ota_update_get_mcu_fw_name(char *name)
{
    if (name == NULL) {
        return OTA_FAILED;
    }

    memset(name, 0x00, OTA_MCU_FW_NAME_LEN);
    memcpy(name, &g_mcu_fw_name[0], OTA_MCU_FW_NAME_LEN);
    return OTA_SUCCESS;
}

UINT ota_update_get_mcu_fw_info(char *name, UINT *size, UINT *crc)
{
    UINT addr, name_len;
    ota_mcu_fw_info_t ota_mcu_fw_info = {0x00, };

    addr = ota_update_get_curr_sflash_addr(OTA_TYPE_MCU_FW);
    memset(ota_mcu_fw_info.name, 0x00, OTA_MCU_FW_NAME_LEN);
    if (ota_update_read_flash(addr, &ota_mcu_fw_info, sizeof(ota_mcu_fw_info_t)) == OTA_SUCCESS) {
        /* Check Size */
        if ((ota_mcu_fw_info.size <= 0) || (ota_mcu_fw_info.size >= 0xffffffff)) {
            ota_mcu_fw_info.size = 0;
        }

        /* Check CRC */
        if ((ota_mcu_fw_info.crc <= 0) || (ota_mcu_fw_info.crc >= 0xffffffff)) {
            ota_mcu_fw_info.crc = 0;
        }

        /* Check Name */
        name_len = strlen(ota_mcu_fw_info.name);
        if ((name_len <= 0) || (ota_mcu_fw_info.size <= 0)) {
            strcpy(ota_mcu_fw_info.name, "NULL\0");
        }

        /* Copy Size */
        if (size != NULL) {
            memcpy(size, &ota_mcu_fw_info.size, sizeof(ota_mcu_fw_info.size));
        }

        /* Copy CRC */
        if (crc != NULL) {
            memcpy(crc, &ota_mcu_fw_info.crc, sizeof(ota_mcu_fw_info.crc));
        }

        /* Copy Name */
        if (name != NULL) {
            if (name_len > OTA_MCU_FW_NAME_LEN) {
                name_len = OTA_MCU_FW_NAME_LEN;
            }
            memcpy(name, ota_mcu_fw_info.name, name_len);
        }
        /*
        		OTA_INFO("\n- MCU FW (addr = 0x%x)\n", addr);
        		OTA_INFO(" Name-------------%s \n", ota_mcu_fw_info.name);
        		OTA_INFO(" Size-------------%d \n", ota_mcu_fw_info.size);
        		OTA_INFO(" CRC--------------0x%x \n\n", ota_mcu_fw_info.crc);
        */
        return OTA_SUCCESS;
    }

    return OTA_FAILED;
}

UINT ota_update_erase_mcu_fw(void)
{
    UINT sflash_addr = 0, fw_size = 0;
    UINT status = OTA_SUCCESS;

    OTA_INFO("\n---------FW erase start--------- \n", __func__);
    if (ota_update_get_mcu_fw_info(NULL, &fw_size, NULL) == OTA_SUCCESS) {
        fw_size += OTA_MCU_FW_HEADER_SIZE;
        sflash_addr = ota_update_get_new_sflash_addr(OTA_TYPE_MCU_FW);

        if (ota_update_erase_flash(sflash_addr, fw_size) <= 0) {
            OTA_ERR("[%s] Failed to erase (addr=0x%x, size=%d).\n", __func__, sflash_addr, fw_size);
            status = OTA_FAILED;
        }
    }
    OTA_INFO("\n---------FW erase finish (0x%02x)--------- \n", status);
    return status;
}

UINT ota_update_read_mcu_fw(UINT sflash_addr, UINT size)
{
    char *fw_buf = NULL;
    UINT i, send_cnt = 0;
    UINT fw_size = 0, blk_size = 0, last_blk_size = 0;
    UINT fw_addr = 0;
    UINT status = OTA_SUCCESS;

    OTA_INFO("\n---------FW read start--------- \n", __func__);

    if ((sflash_addr == 0)
            || (sflash_addr < FLASH_ADDR_USER_DW_START)
            || (sflash_addr > FLASH_ADDR_USER_DW_END)
            || (size == 0))	{
        OTA_ERR("[%s] Please check the input parameter.\n", __func__);
        status =  OTA_FAILED;
        goto finish;
    }
    OTA_INFO("\nRead sflash (addr = 0x%x, size = %d) \n", sflash_addr, size);

    fw_addr = sflash_addr;
    fw_size = size;

    fw_buf = (char *)OTA_MALLOC(OTA_MCU_BUF_SIZE);
    if (fw_buf == NULL) {
        OTA_ERR("[%s] Failed to alloc memory (size=%d).\n", __func__, OTA_MCU_BUF_SIZE);
        status =  OTA_MEM_ALLOC_FAILED;
        goto finish;
    }
    memset(fw_buf, 0x00, OTA_MCU_BUF_SIZE);

    send_cnt = fw_size / OTA_MCU_BUF_SIZE;
    if (fw_size >= OTA_MCU_BUF_SIZE) {
        blk_size = OTA_MCU_BUF_SIZE;
        last_blk_size = fw_size % OTA_MCU_BUF_SIZE;
    } else {
        last_blk_size = fw_size;
    }

    /* Send FW Binary */
    OTA_INFO("Send binary (size=%d) \n", fw_size);
    for (i = 0; i < send_cnt; i++) {
        ota_update_read_flash(fw_addr, (VOID *)fw_buf, blk_size);
        fw_addr += OTA_MCU_BUF_SIZE;

#if defined (DEBUG_OTA_MCU_DUMP)
        hex_dump((UCHAR *)fw_buf, blk_size);
#endif //(DEBUG_OTA_MCU_DUMP)

#if defined (__SUPPORT_ATCMD__)
        PUTS_ATCMD(fw_buf, blk_size);
#else
        ota_update_iface_puts(fw_buf, blk_size);
#endif // (__SUPPORT_ATCMD__)
    }

    if (last_blk_size > 0) {
        ota_update_read_flash(fw_addr, (VOID *)fw_buf, last_blk_size);

#if defined (DEBUG_OTA_MCU_DUMP)
        hex_dump((UCHAR *)fw_buf, last_blk_size);
#endif //(DEBUG_OTA_MCU_DUMP)

#if defined (__SUPPORT_ATCMD__)
        PUTS_ATCMD(fw_buf, last_blk_size);
#else
        ota_update_iface_puts(fw_buf, last_blk_size);
#endif // (__SUPPORT_ATCMD__)
    }

    /* Flash Close */
    if (fw_buf != NULL) {
        OTA_FREE(fw_buf);
        fw_buf = NULL;
    }

finish :
    OTA_INFO("\n---------FW read finish (0x%02x)--------- \n", status);
    return status;
}

UINT ota_update_trans_mcu_fw(void)
{
    UINT i, send_cnt = 0;
    UINT fw_size = 0, blk_size = 0, last_blk_size = 0;
    UINT fw_addr = 0, cal_crc = 0;
    char *fw_buf = NULL;
    char fw_name[OTA_MCU_FW_NAME_LEN + 1] = {0x00, };
    UINT status = OTA_SUCCESS;

    OTA_INFO("\n---------FW transfer start--------- \n", __func__);

    /* Check exists - MCU FW */
    if (ota_update_get_mcu_fw_info(&fw_name[0], &fw_size, &cal_crc) == OTA_SUCCESS) {
        if ((strlen(fw_name) <= 0)
                || (fw_size == 0) || (cal_crc == 0)) {
            OTA_ERR("[%s] MCU FW does not exist.\n", __func__);
            status = OTA_FAILED;
            goto finish;
        }
    } else {
        OTA_ERR("[%s] Failed to get FW info.\n", __func__);
        status = OTA_FAILED;
        goto finish;
    }

    fw_addr = ota_update_get_new_sflash_addr(OTA_TYPE_MCU_FW);
    if (fw_addr != SFLASH_UNKNOWN_ADDRESS) {
        /* Header information offset */
        fw_addr += OTA_MCU_FW_HEADER_SIZE;
    } else	{
        OTA_ERR("[%s] Failed to get addr\n", __func__);
        status = OTA_FAILED;
        goto finish;
    }

    OTA_INFO("\nTransfer %s(len=%d), ReadAddr = 0x%x, FW_SIZE = %d, FW_CRC = 0x%x) \n",
                fw_name, strlen(fw_name), fw_addr, fw_size, cal_crc);

    /* Send FW Name & Size*/
#if defined (__SUPPORT_ATCMD__)
    PRINTF_ATCMD("%s,%d,0x%x\r\n", &fw_name[0], fw_size, cal_crc);
#else
    ota_update_iface_printf("%s,%d,0x%x\r\n", &fw_name[0], fw_size, cal_crc);
#endif //(__SUPPORT_ATCMD__)

    fw_buf = (char *)OTA_MALLOC(OTA_MCU_BUF_SIZE);
    if (fw_buf == NULL)	{
        OTA_ERR("[%s] Failed to alloc memory (size=%d).\n", __func__, OTA_MCU_BUF_SIZE);
        status = OTA_MEM_ALLOC_FAILED;
        goto finish;
    }

    send_cnt = fw_size / OTA_MCU_BUF_SIZE;
    if (fw_size >= OTA_MCU_BUF_SIZE) {
        blk_size = OTA_MCU_BUF_SIZE;
        last_blk_size = fw_size % OTA_MCU_BUF_SIZE;
    } else	{
        last_blk_size = fw_size;
    }

    /* Send FW Binary */
    OTA_INFO("Send binary (size=%d, crc=0x%x) \n", fw_size, cal_crc);
    for (i = 0; i < send_cnt; i++) {
        memset(fw_buf, 0x00, OTA_MCU_BUF_SIZE);
        ota_update_read_flash(fw_addr, (VOID *)fw_buf, blk_size);
        fw_addr += OTA_MCU_BUF_SIZE;

#if defined (DEBUG_OTA_MCU_DUMP)
        hex_dump((UCHAR *)fw_buf, blk_size);
#endif //(DEBUG_OTA_MCU_DUMP)

#if defined (__SUPPORT_ATCMD__)
        PUTS_ATCMD(fw_buf, blk_size);
#else
        ota_update_iface_puts(fw_buf, blk_size);
#endif //(__SUPPORT_ATCMD__)
    }

    if (last_blk_size > 0) {
        memset(fw_buf, 0x00, OTA_MCU_BUF_SIZE);
        ota_update_read_flash(fw_addr, (VOID *)fw_buf, last_blk_size);

#if defined (DEBUG_OTA_MCU_DUMP)
        hex_dump((UCHAR *)fw_buf, last_blk_size);
#endif //(DEBUG_OTA_MCU_DUMP)

#if defined (__SUPPORT_ATCMD__)
        PUTS_ATCMD(fw_buf, last_blk_size);
#else
        ota_update_iface_puts(fw_buf, last_blk_size);
#endif //(__SUPPORT_ATCMD__)
    }

    if (fw_buf != NULL) {
        OTA_FREE(fw_buf);
        fw_buf = NULL;
    }

finish :
    OTA_INFO("\n---------FW transfer finish (0x%02x)--------- \n", status);
    return status;
}

UINT ota_update_by_mcu_init(UINT fw_type, UINT len)
{
    if (ota_update_check_accept_size(fw_type, len) != OTA_SUCCESS) {
        return OTA_ERROR_SIZE;
    }

    by_mcu.write.sflash_addr = ota_update_get_new_sflash_addr(fw_type);
    if ((by_mcu.write.sflash_addr != SFLASH_RTOS_0_BASE)
            && (by_mcu.write.sflash_addr != SFLASH_RTOS_1_BASE)) {
        return OTA_ERROR_SFLASH_ADDR;
    }

    by_mcu.update_type = fw_type;
    by_mcu.received_length = 0;
    by_mcu.write.total_length = len;
    by_mcu.write.offset = 0;
    by_mcu.download_status = OTA_SUCCESS;
    by_mcu.version_check = OTA_NOT_FOUND;
    by_mcu.content_length = len;
    by_mcu.received_length = 0;
    MCU_TX_SIZE = 0;

#if defined (__BLE_COMBO_REF__)
    ota_update_ble_set_exist_url(OTA_TYPE_RTOS, 1);
#endif	// (__BLE_COMBO_REF__)

    return OTA_SUCCESS;
}

UINT ota_update_by_mcu_get_total_len(void)
{
     return by_mcu.content_length;
}

UINT ota_update_by_mcu_download(UCHAR *rev_data, UINT rev_data_len)
{
    UINT status = OTA_SUCCESS;
    UINT progress = 0;

    if (by_mcu.update_type != OTA_TYPE_RTOS) {
        status = OTA_ERROR_TYPE;
        goto finish;
    }

    if (rev_data == NULL) {
        OTA_ERR("[%s] rev_data is null\n", __func__);
        status = OTA_FAILED;
        goto finish;

    } else {

#if defined (DISABLE_OTA_VER_CHK)
        by_mcu.version_check = OTA_SUCCESS;
#else
        if (by_mcu.version_check == OTA_NOT_FOUND) {
            if (ota_update_check_version(by_mcu.update_type,
                                         rev_data, rev_data_len) == OTA_SUCCESS) {
                by_mcu.version_check = OTA_SUCCESS;
            } else {
                /* Version mismatch */
                by_mcu.version_check = OTA_VERSION_INCOMPATI;
                status = OTA_VERSION_INCOMPATI;
                goto finish;
            }

            if (by_mcu.version_check == OTA_SUCCESS) {
                if (ota_update_check_accept_size(by_mcu.update_type,
                                                 by_mcu.content_length) != OTA_SUCCESS) {
                    by_mcu.version_check = OTA_VERSION_INCOMPATI;
                    status = OTA_VERSION_INCOMPATI;
                    goto finish;
                }

                if (by_mcu.write.sflash_addr != ota_update_get_new_sflash_addr(OTA_TYPE_RTOS)) {
                    status = OTA_ERROR_SFLASH_ADDR;
                    goto finish;
                }
            }
        }
#endif // (DISABLE_OTA_VER_CHK)

        by_mcu.received_length += rev_data_len;
        status = ota_update_write_sflash(&by_mcu.write, rev_data, rev_data_len);

        if (status != OTA_SUCCESS) {
            OTA_ERR("[%s] Failed to write data to sflash(0x%02x)\n", __func__, status);
            goto finish;
        }

        if ((by_mcu.received_length > 0) && (by_mcu.content_length > 0)) {
            progress = (by_mcu.received_length * 100) / by_mcu.content_length;

            OTA_INFO("\r   >> By MCU Downloading... %d % (%d/%d Bytes)%s",
                     progress,
                     by_mcu.received_length,
                     by_mcu.content_length, progress == 100 ? "\n" : " ");
        } else {
            progress = 0;
        }
    }

finish:

    if ((status == OTA_SUCCESS) && (progress != 100)) {
        PRINTF_ATCMD("\r\nOK\r\n");
    }

    if ((status != OTA_SUCCESS) || progress == 100) {
        if (by_mcu.version_check == OTA_SUCCESS) {
            ota_update_set_download_progress(by_mcu.update_type, progress);
        }

        ota_update_print_status(by_mcu.update_type, status);
        by_mcu.update_type = OTA_TYPE_INIT;
        by_mcu.received_length = 0;
        by_mcu.write.sflash_addr = 0;
        by_mcu.write.total_length = 0;
        by_mcu.write.offset = 0;
        by_mcu.download_status = OTA_SUCCESS;
        by_mcu.version_check = OTA_NOT_FOUND;
        by_mcu.content_length = 0;
        by_mcu.received_length = 0;
        MCU_TX_SIZE = 0;

        PRINTF_ATCMD("+NWOTABYMCU:0x%02x\r\n", status);
    }

    return status;

}

void ota_update_atcmd_set_tx_size(UINT tx_size)
{
    MCU_TX_SIZE = tx_size;
    return;
}

UINT ota_update_atcmd_parser(char *in_buf)
{
    char *ptr = NULL;
    char *ptr_size = NULL;
    UINT tx_size = 0;

    if (ota_update_by_mcu_get_total_len() > 0) {
#if defined (__ATCMD_IF_UART1__) || defined (__ATCMD_IF_UART2__)
        if (MCU_TX_SIZE > 0) {
            ptr = in_buf;
            tx_size = MCU_TX_SIZE;

        } else {
            ptr = strstr(in_buf, "tx_size=");
            if (ptr != NULL) {
                ptr += 8;
                tx_size = strtol(ptr, &ptr_size, 10);
                ptr = ptr_size;

                if ((tx_size > 0) && (strlen(ptr) > 0)) {
                    if (strncmp(ptr,",", 1) == 0) {
                        ptr += 1;
                    }
                }
            }
        }
#else
        /* __ATCMD_IF_SPI__ || __ATCMD_IF_SDIO__ */
        ptr = in_buf;
        if (strncasecmp(ptr, "tx_size=", 8) == 0) {
            ptr += 8;
            tx_size = strtol(ptr, &ptr_size, 10);
            ptr = ptr_size;

            if ((tx_size > 0) && (strlen(ptr) > 0)) {
                if (strncmp(ptr,",", 1) == 0) {
                    ptr += 1;
                } else {
                    ptr = NULL;
                }
            } else {
                ptr = NULL;
            }
        } else {
            ptr = NULL;
        }
#endif

        /* Write downloaded firmware to flash */
        if (ota_update_by_mcu_download((UCHAR *)ptr, tx_size) == OTA_SUCCESS) {
            return OTA_SUCCESS;
        }
    }

    return OTA_FAILED;
}

#else
UINT ota_update_set_mcu_fw_name(char *name)
{
    DA16X_UNUSED_ARG(name);

    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /* OTA_FAILED */
}
UINT ota_update_get_mcu_fw_name(char *name)
{
    DA16X_UNUSED_ARG(name);

    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}
UINT ota_update_get_mcu_fw_info(char *name, UINT *size, UINT *crc)
{
    DA16X_UNUSED_ARG(name);
    DA16X_UNUSED_ARG(size);
    DA16X_UNUSED_ARG(crc);

    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}
UINT ota_update_read_mcu_fw(UINT sflash_addr, UINT size)
{
    DA16X_UNUSED_ARG(sflash_addr);
    DA16X_UNUSED_ARG(size);

    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}
UINT ota_update_trans_mcu_fw(void)
{
    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}
UINT ota_update_erase_mcu_fw(void)
{
    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}
UINT ota_update_calcu_mcu_fw_crc(int sflash_addr, int size)
{
    DA16X_UNUSED_ARG(sflash_addr);
    DA16X_UNUSED_ARG(size);

    PRINTF("[%s] Not supported this function!!\n", __func__);
    return 0x01; /*OTA_FAILED */
}

#endif // (__OTA_UPDATE_MCU_FW__)
#endif	// (__SUPPORT_OTA__)

/* EOF */
