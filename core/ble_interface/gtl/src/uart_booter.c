/**
 ****************************************************************************************
 *
 * @file uart_booter.c
 *
 * @brief Functions to transfer BLE main image to BLE's RAM
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

#if defined (__BLE_COMBO_REF__)

#include "application.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"

#include "da16_gtl_features.h"
#include "sflash.h"
#include "sys_image.h"
#include "user_dpm_api.h"

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update_ble_fw.h"
#endif

#define BLE_FW_FLASH_ADDR   SFLASH_BLE_FW_BASE
#define BLE_FW_BUFFER_SIZE  (4096)

#define SOH   0x01
#define STX   0x02
#define ACK   0x06
#define NAK   0x15

/*************************************
 * To make POR of DA14531 by a GPIO. *
 *************************************/
#ifdef __CFG_ENABLE_BLE_HW_RESET__
	#define BLE_RESET_GPIO_UNIT		GPIO_UNIT_C
	#define BLE_RESET_GPIO_PIN		GPIO_PIN8
	#define BLE_RESET_GPIO_POL		1	// 1: High, 0: Low
	#define OS_DELAY_MS(_ms)		vTaskDelay(_ms / portTICK_PERIOD_MS);
#endif	// __CFG_ENABLE_BLE_HW_RESET__

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#define DA14531_IMG_HDR_SIZE    (sizeof(ble_img_hdr_t))
extern ble_img_hdr_all_t ble_fw_hdrs_all;

extern int loadActiveImage(ble_act_fw_info_t* selected_fw_info);
#else
#define DA14531_IMG_HDR_SIZE    (4)
#define DA14531_IMG_MAX_SIZE    (0x8000)
#endif

uint8_t g_is_ble_reset_by_ext_host;
HANDLE	updateFwSflash;

extern void host_uart_set_baudrate_control(UINT32 baudrate, UINT32 set);
extern void da16x_environ_lock(UINT32 flag);
extern int getchar_uart1(UINT32 mode);
extern void PUTS_UART1(char *data, int data_len);
extern void hold_dpm_sleepd(void);
extern void da16x_boot_reset_wakeupmode(void);

__attribute__((weak)) void app_ble_sw_reset(void);

void	readFlashData(UINT sflash_addr, UINT read_len, char *sflash_data)
{
	SFLASH_READ(updateFwSflash, sflash_addr, sflash_data, read_len);
}

HANDLE	sflashOpen(void)
{
	UINT32	ioctldata[8];
	UINT32	busmode;

	updateFwSflash = SFLASH_CREATE(SFLASH_UNIT_0);

	if (updateFwSflash == NULL)
	{
		DBG_ERR(RED_COLOR "sflash error\n" CLEAR_COLOR);
		return (HANDLE)0;
	}

	/* Setup bussel */
	ioctldata[0] = da16x_sflash_get_bussel();
	SFLASH_IOCTL(updateFwSflash, SFLASH_SET_BUSSEL, ioctldata);

	if (SFLASH_INIT(updateFwSflash) == TRUE )
	{
		/* to prevent a reinitialization */
		if (da16x_sflash_setup_parameter (ioctldata) == TRUE )
		{
			SFLASH_IOCTL(updateFwSflash, SFLASH_SET_INFO, ioctldata);
		}
	}

	SFLASH_IOCTL(updateFwSflash, SFLASH_CMD_WAKEUP, ioctldata);

	if (ioctldata[0] > 0)
	{
		ioctldata[0] = ioctldata[0] / 1000;	/* msec */
		if(ioctldata[0] > 100)
			vTaskDelay(ioctldata[0] / 100);
		else
			vTaskDelay(1);
	}

	busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
	SFLASH_IOCTL(updateFwSflash, SFLASH_BUS_CONTROL, &busmode);

	/* Check SFLASH Info */
	ioctldata[0] = 0;
	SFLASH_IOCTL(updateFwSflash, SFLASH_GET_INFO, ioctldata);

	return updateFwSflash;
}


void	sflashClose(HANDLE handleSflash)
{
	UINT32	busmode;

	busmode = SFLASH_BUS_3BADDR | SFLASH_BUS_111;
	SFLASH_IOCTL(handleSflash, SFLASH_BUS_CONTROL, &busmode);
	SFLASH_CLOSE(handleSflash);
	da16x_environ_lock(FALSE);
}

static void boot_usec_delay(void)
{
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
}

void DA14531_HW_Reset(void)
{
	HANDLE gpio;
	UINT16 num, gdata;

	// GPIO Initialization
	gpio = GPIO_CREATE(GPIO_UNIT_A);
	GPIO_INIT(gpio);

	// Set GPIO Output Mode
	gdata = num = GPIO_PIN1;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &gdata);

	// Make GPIO High
	GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));

    boot_usec_delay();

	// Make GPIO Low
	gdata = 0;
	GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));

	// Make GPIO Default State
	gdata = GPIO_PIN1;
	GPIO_IOCTL(gpio, GPIO_SET_INPUT, &gdata);

	//Close GPIO
	GPIO_CLOSE(gpio);
}

#if defined(__CFG_ENABLE_BLE_HW_RESET__)
void DA14531_HW_Reset_alt(uint8_t onoff)
{
	HANDLE gpio;
	UINT16 num, gdata;

	// GPIO Initialization
	gpio = GPIO_CREATE(BLE_RESET_GPIO_UNIT);
	GPIO_INIT(gpio);

	// Set GPIO Output Mode
	gdata = num = BLE_RESET_GPIO_PIN;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &gdata);

	if (BLE_RESET_GPIO_POL) {
		if (onoff == FALSE)
			gdata = 0;
		// Make GPIO High
		GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));
	} else {
		// Make GPIO Low
		if (onoff == TRUE)
			gdata = 0;
		GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));
	}

	//Close GPIO
	GPIO_CLOSE(gpio);
}
#endif // __CFG_ENABLE_BLE_HW_RESET__

#undef __DA14531_BOOT_FROM_UART_NORESET__

uint32_t crc_value_convt(uint32_t crc_code)
{
	/* Remove dummy values */
	if(0xff & (crc_code >> 24)) {
		crc_code &= 0xffffff;
		if(0xff & (crc_code >> 16)) {
			crc_code &= 0xffff;
			if(0xff & (crc_code >> 8)) {
				crc_code &= 0xff;
			}
		}
	}
	return crc_code;
}

int DA14531_Download_IMG(void)
{
	UINT8 *fw_buf, tx;
	uint32_t crc_code = 0;
	UINT16 fw_size = 0, blk_size = 0;
	HANDLE  hSFlash;
	int i, ret = pdPASS;
	UINT32  ch;
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
	ble_act_fw_info_t active_fw_info;
#endif

	fw_buf = (UINT8 *)APP_MALLOC(BLE_FW_BUFFER_SIZE);

	if (fw_buf == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return -4;
	}

	DBG_TMP("Buf Addr. = 0x%x\r\n",(unsigned int)fw_buf);

	// Flash Open
	hSFlash = (HANDLE)sflashOpen();

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
	// decide which image (among fw_1 and fw_2) to boot
	ret = loadActiveImage(&active_fw_info);
	if (ret != pdPASS)
	{
		return -5;
	}

	fw_size = active_fw_info.code_size;
	crc_code = crc_value_convt(active_fw_info.crc);

	PRINTF("[combo] BLE FW VER to transfer .... \n   >>> v_%s (id=%d) at bank_%d\n", 	
			active_fw_info.version, 
			active_fw_info.imageid,
			active_fw_info.active_img_bank);
#else
	// Get Default FW Size
	fw_size = DA14531_IMG_MAX_SIZE;
#endif

	blk_size = ((fw_size - 1) / BLE_FW_BUFFER_SIZE) + 1;

	DBG_TMP("fw_size = %d, crc = 0x%x\r\n",fw_size,crc_code);

	//Get First Block of Download Image
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
	readFlashData(active_fw_info.img_pos, BLE_FW_BUFFER_SIZE, (char *)fw_buf);
#else
	readFlashData(BLE_FW_FLASH_ADDR, BLE_FW_BUFFER_SIZE, fw_buf);
#endif

	//    PRINTF("0x%02x 0x%02x 0x%02x 0x%02x\r\n",*fw_buf,*(fw_buf+1),*(fw_buf+2),*(fw_buf+3) );

#if defined (__DA14531_BOOT_FROM_UART_NORESET__)

	//Flush UART FIFO
	for (int i = 0; i < 128; i++)
	{
		ch = getchar_uart1(0);
		ch += ch;
	}

	ch = 0;
#endif
	//Wait "STX" Packet
	ch = 0;
	for (i = 0; i < 50; i++)	// 500ms is enough to detect the STX after H/W reset.
	{
		ch = getchar_uart1(0);
		if(ch == STX)
			break;
		vTaskDelay(1);
	}

#if !defined(__CFG_ENABLE_BLE_HW_RESET__)
	if(ch != STX) {
        // Trying to do S/W reset if H/W reset does not work.
        // Because the H/W reset of BLE has been disabled during BLE is running.
        PRINTF("[combo] S/W Reset");
        do {
            int idx = 0;
            uint16_t len = 0;
            app_ble_sw_reset();
            // 30ms is enough to detect the STX after S/W reset.
            for (i = 0; i < 3 && (ch != STX && ch != 0x05); i++) {
                vTaskDelay(1);
                ch = getchar_uart1(0);
            }
            if (ch==0x05) {
                do {
                    ch = getchar_uart1(0);
                    if (idx == 6)   len = ch;
                    if (idx == 7)   len += ch << 8;
                    if (idx > 7 && idx >= 7 + len) {
                        break;
                    }
                    idx++;
                } while(1);
                PRINTF("-");
            } else {
                PRINTF(".");
            }
        } while(ch != STX);
        PRINTF("\r\n");
    }
#endif

	//Check "STX" Packet
	if (ch == STX)
	{
		//Send "SOH" Packet
		tx = SOH;
		PUTS_UART1((char *)&tx, 1);

		//Send FW Size
		PUTS_UART1(((char *)&fw_size), 2);

		//Wait "ACK" Packet
		ch = getchar_uart1(portMAX_DELAY);

		//Check "ACK" Packet
		if (ch == ACK)
		{

			//Send FW Binary
			for (i = 0; i < blk_size; i++)
			{
				if (i < (blk_size - 1))
				{
					PUTS_UART1((char *)fw_buf, BLE_FW_BUFFER_SIZE);
				}
				else
				{
					if ((fw_size % BLE_FW_BUFFER_SIZE) != 0)
					{
						PUTS_UART1((char *)fw_buf, fw_size % BLE_FW_BUFFER_SIZE);
					}
					else
					{
						PUTS_UART1((char *)fw_buf,  BLE_FW_BUFFER_SIZE);
					}

					break;
				}
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
				readFlashData(active_fw_info.img_pos + ((i + 1)*BLE_FW_BUFFER_SIZE),
							  BLE_FW_BUFFER_SIZE, (char *)fw_buf);
#else
				readFlashData(BLE_FW_FLASH_ADDR + ((i + 1)*BLE_FW_BUFFER_SIZE),
							  BLE_FW_BUFFER_SIZE, fw_buf);
#endif
			}

			//Wait CRC Code
			ch = getchar_uart1(portMAX_DELAY);
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
			DBG_TMP("CRC Debug, ch(%x), crc_code(%x) \r\n",ch, crc_code);
			//Check CRC Code
			if (ch == crc_code)
			{
				tx = ACK;
			}
			else
			{
				ret = -3;
				tx = NAK;
			}
#else
			tx = ACK;
#endif
			//Send ACK/NAK
			PUTS_UART1((char *)&tx, 1);
		}
		else
		{
			ret = -2;
		}

	}
	else
	{
		ret = -1;
	}

	// Flash Close
	APP_FREE(fw_buf);
	sflashClose(hSFlash);

	return ret;
}


void boot_ble_from_uart(void)
{
	INT32 ret = 0;

#if defined (__BLE_FW_VER_CHK_IN_OTA__)
	int tmp_ble_act_bank;
#endif

	//Change Baud Rate 115200, No Flow Control
#if defined (__DA14531_BOOT_FROM_UART_NORESET__)
	host_uart_set_baudrate_control(921600, 0);
#else
#if !defined (__GTL_UART_115200__)
	host_uart_set_baudrate_control(115200, 0);
#endif
#endif

#ifndef __DA14531_BOOT_FROM_UART_NORESET__
	// Check uart tx fifo empty
	uint32_t status = 0;  // Check FIFO empty
	extern HANDLE  uart1;
	do {
		UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &status);
	}while(!status);
	SYSUSLEEP(500);		// Minimum 100us delay time is required to secure that the last byte is sent out completely.

	//Change PAD Mux to GPIO
	_da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);

	DA14531_HW_Reset(); // reset DA14531: takes effect when DA14531 is in bootloader mode
	
	//Change PAD Mux to UART1
	_da16x_io_pinmux(PIN_AMUX, AMUX_UART1d); // TXD: GPIO[0], RXD: GPIO[1]
#endif

	//Download Image
	ret = DA14531_Download_IMG();
#if defined(__CFG_ENABLE_BLE_HW_RESET__)
	if (ret != pdPASS) {
		DA14531_HW_Reset_alt(TRUE);
		ret = DA14531_Download_IMG();
		DA14531_HW_Reset_alt(FALSE);
	}
#endif // __CFG_ENABLE_BLE_HW_RESET__

	//Error Check
	if (ret != pdPASS)
	{
		PRINTF(RED_COLOR "Error Initialization of DA14531, freeRTOS, err_code = %d\r\n" CLEAR_COLOR, ret);
	}
	else
	{
		PRINTF(CYAN_COLOR "[combo] BLE FW transfer done\r\n" CLEAR_COLOR);
		
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
		if (read_nvram_int("ble_fw_act_bank", &tmp_ble_act_bank) != 0)
		{
			// "ble_fw_act_bank" not exist in nvram
			write_nvram_int("ble_fw_act_bank", ble_fw_hdrs_all.active_img_bank);
		}
		else
		{
			// "ble_fw_act_bank" exists
			if (tmp_ble_act_bank != ble_fw_hdrs_all.active_img_bank)
			{
				write_nvram_int("ble_fw_act_bank", ble_fw_hdrs_all.active_img_bank);
			}
		}
#endif
	}

#if !defined (__GTL_UART_115200__)
	//Change Baud Rate 921600, Flow Control
	host_uart_set_baudrate_control(921600, 1);
#endif
}


void cmd_ble_reset(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	UINT32  ch;
	int ret;

	//Change Baud Rate 115200, No Flow Control
	host_uart_set_baudrate_control(115200, 0);

	//Change PAD Mux to GPIO
	_da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);

	//DA14531 Reset
	DA14531_HW_Reset();

	//Flush UART FIFO
	for (int i = 0; i < 128; i++)
	{
		ch = getchar_uart1(0);
		ch += ch;
	}

	ch = 0;

	//Change PAD Mux to UART1
	_da16x_io_pinmux(PIN_AMUX, AMUX_UART1d); // TXD: GPIO[0], RXD: GPIO[1]

	//Download Image
	ret = DA14531_Download_IMG();

	//Error Check
	if (ret != pdPASS)
	{
		PRINTF("Error in initialization of DA14531\r\n");
	}

	//Change Baud Rate 921600, Flow Control
	host_uart_set_baudrate_control(921600, 1);

}

void combo_ble_sw_reset(void)
{
	if (chk_dpm_mode()) 
		hold_dpm_sleepd();
	else
		da16x_boot_reset_wakeupmode();
	
#if defined (__DA14531_BOOT_FROM_UART__)
	g_is_ble_reset_by_ext_host = TRUE;
	app_ble_sw_reset();
	PRINTF("Reset BLE ... \n");
	vTaskDelay(2);
#endif
}

#if defined(__ENABLE_BLE_WKUP_BEFORE_SEND_MSG__)
void combo_ble_wkup(void)
{
	HANDLE gpio;
	UINT16 num, gdata;

	_da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
	// GPIO Initialization
	gpio = GPIO_CREATE(GPIO_UNIT_A);
	GPIO_INIT(gpio);

	// Set GPIO Output Mode
	gdata = num = GPIO_PIN4;
	GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &gdata);

	// Make GPIO High
	GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));

	//Dummy Sleep
	SYSUSLEEP(0);

	// Make GPIO Low
	gdata = 0;
	GPIO_WRITE(gpio, num, &gdata, sizeof(UINT16));

	// Make GPIO Default State
	gdata = GPIO_PIN4;
	GPIO_IOCTL(gpio, GPIO_SET_INPUT, &gdata);

	//Close GPIO
	GPIO_CLOSE(gpio);
	_da16x_io_pinmux(PIN_CMUX, CMUX_UART1c);
	
}
#endif
#elif defined (GENERIC_SDK)
void combo_ble_sw_reset(void)
{
	return ;
}

#endif

/* EOF */
