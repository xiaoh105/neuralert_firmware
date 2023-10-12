/**
 ****************************************************************************************
 *
 * @file console.h
 *
 * @brief Header file for basic console user interface of the host application.
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

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "rwble_config.h"

typedef struct
{
	unsigned char type;
	unsigned char val;
} console_msg;

enum
{
	CONSOLE_DEV_DISC_CMD,
	CONSOLE_CONNECT_CMD,
	CONSOLE_DISCONNECT_CMD,
	CONSOLE_ADV_START_CMD,
	CONSOLE_ADV_STOP_CMD,
	CONSOLE_RD_LLV_CMD,
	CONSOLE_RD_TXP_CMD,
	CONSOLE_WR_LLV_CMD,
	CONSOLE_WR_IAS_CMD,
	CONSOLE_EXIT_CMD,

	CONSOLE_IOT_SENSOR_START,
	CONSOLE_IOT_SENSOR_STOP,

	CONSOLE_GAPM_RESET,
	CONSOLE_GET_BLE_FW_VER_CMD,
	
#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
	CONSOLE_FORCE_EXCEPTION_ON_BLE,
#endif
	CONSOLE_LAST
};


/*
 ****************************************************************************************
 * @brief Start iot_sensing task in ble.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendSensorStart(void);


/*
 ****************************************************************************************
 * @brief Stop iot_sensing task in ble.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendSensorStop(void);


/*
 ****************************************************************************************
 * @brief Sends Discover devices message to application 's main thread.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendScan(void);
/*
 ****************************************************************************************
 * @brief Sends Connect to device message to application 's main thread.
 *
 *  @param[in] indx Device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendConnnect(int indx);
/*
 ****************************************************************************************
 * @brief Sends Read request message to application 's main thread.
 *

 * @return void.
 ****************************************************************************************
*/
void ConsoleSendDisconnnect(void);

/*
 ****************************************************************************************
 * @brief Sends ADV start request message to application 's main thread.
 *

 * @return void.
 ****************************************************************************************
*/
void ConsoleSendstart_adv(void);

/*
 ****************************************************************************************
 * @brief Sends ADV stop request message to application 's main thread.
 *

 * @return void.
 ****************************************************************************************
*/
void ConsoleSendstop_adv(void);

/*
 ****************************************************************************************
 * @brief Sends Read request message to application 's main thread.
 *
 *  @param[in] type  Attribute type to read.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleRead(unsigned char type);
/*
 ****************************************************************************************
 * @brief Sends write request message to application 's main thread.
 *
 *  @param[in] type  Attribute type to write.
 *  @param[in] val   Attribute value.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleWrite(unsigned char type, unsigned char val);
/*
 ****************************************************************************************
 * @brief Sends a message to application 's main thread to exit.
 *

 * @return void.
 ****************************************************************************************
*/
void ConsoleSendExit(void);

/*
 ****************************************************************************************
 * @brief Handles keypress events and sends corresponding message to application's main thread.
 *
 * @return void.
 ****************************************************************************************
*/
void HandleKeyEvent(int Key);

/*
 ****************************************************************************************
 * @brief Demo application Console Menu header
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleTitle(void);
/*
 ****************************************************************************************
 * @brief Prints Console menu in Scan state.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleScan(void);
/*
 ****************************************************************************************
 * @brief Prints Console menu in Idle state.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleIdle(void);
/*
 ****************************************************************************************
 * @brief Prints List of discovered devices.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsolePrintScanList(void);
/*
 ****************************************************************************************
 * @brief Prints Console Menu in connected state.
 *
 * @param[in] full   If true, prints peers attributes values.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleConnected(int full);
/*
 ****************************************************************************************
 * @brief Console Thread main loop.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
*/
void ConsoleProc(void);

#endif //CONSOLE_H_

/* EOF */
