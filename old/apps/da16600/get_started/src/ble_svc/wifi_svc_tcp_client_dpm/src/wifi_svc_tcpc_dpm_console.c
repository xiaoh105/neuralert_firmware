/**
 ****************************************************************************************
 *
 * @file wifi_svc_console.c
 *
 * @brief Basic console user interface of the host application.
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

#if defined (__BLE_PERI_WIFI_SVC_TCP_DPM__)

#include "../../include/da14585_config.h"
#include "../../include/queue.h"
#include "../../include/console.h"

#include "da16_gtl_features.h"
#if defined (__BLE_FW_VER_CHK_IN_OTA__)
#include "ota_update.h"
#endif
#if defined (__WFSVC_ENABLE__)
#include "../../include/wifi_svc_user_custom_profile.h"
#endif
#include "../../include/app.h"

#include "proxr.h"
#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "command.h"
#include "user_dpm_api.h"
#include "ota_update.h"

#ifdef __FREERTOS__
#include "msg_queue.h"
#endif

#define	CMD_OPTION_S0(...)	__VA_ARGS__
#define CMD_OPTION_0(...)	__VA_ARGS__

#ifndef __FREERTOS__
extern SemaphoreHandle_t ConsoleQueueMut;
#endif

//-----------------------------------------------------------------------
// Command Function-List
//-----------------------------------------------------------------------
//extern void cmd_ble_reset(int argc, char *argv[]);
#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
extern void cmd_ble_force_exception(int argc, char *argv[]);
extern void ConsoleSendBleForceException(void);
#endif // __TEST_FORCE_EXCEPTION_ON_BLE__
#if defined (__EXCEPTION_RST_EMUL__)
extern void cmd_wifi_force_exception(int argc, char *argv[]);
#endif // __EXCEPTION_RST_EMUL__
extern void cmd_gapm_reset(int argc, char *argv[]);
extern void cmd_ble_fw_ver(int argc, char *argv[]);
extern void ConsoleSendGapmReset(void);
extern void ConsoleSendGetBleFwVer(void);
#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)
extern void cmd_enter_sleep2(int argc, char *argv[]);
#endif // __TEST_FORCE_SLEEP2_WITH_TIMER__
#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)
extern void cmd_enter_sleep3(int argc, char *argv[]);
#endif // __FOR_DPM_SLEEP_CURRENT_TEST__

extern void cmd_ble_disconnect(int argc, char *argv[]);
void cmd_ble_start_adv(int argc, char *argv[]);
void cmd_ble_stop_adv(int argc, char *argv[]);

#if defined (__OTA_TEST_CMD__)
extern void cmd_ota_test(int argc, char *argv[]);
#endif // __OTA_TEST_CMD__

extern void cmd_get_local_dev_info(int argc, char *argv[]);

//-----------------------------------------------------------------------
// Command BLE-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_ble_list[] =
{
	{ "ble",		CMD_MY_NODE,	cmd_ble_list,	NULL,	    	"BLE application command "},  // Head
	CMD_OPTION_S0(	{ "-------",		CMD_FUNC_NODE,	NULL,		NULL,	"--------------------------------"}, )
#if defined (__EXCEPTION_RST_EMUL__)	
	CMD_OPTION_S0(  { "wifi_force_excep",     CMD_FUNC_NODE,  NULL,       &cmd_wifi_force_exception,	"force undef_instr exception on wifi"}, )
#endif // __EXCEPTION_RST_EMUL__
#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)	
	CMD_OPTION_S0(  { "ble_force_excep",     CMD_FUNC_NODE,  NULL,       &cmd_ble_force_exception,	"Force UNDEF_INSTR exception on ble"}, )	
#endif // __TEST_FORCE_EXCEPTION_ON_BLE__
#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)	
	CMD_OPTION_S0(	{ "enter_sleep2",	 CMD_FUNC_NODE,  NULL,		 &cmd_enter_sleep2,	"Enter sleep2"}, )
#endif // __TEST_FORCE_SLEEP2_WITH_TIMER__
#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)	
	CMD_OPTION_S0(	{ "enter_sleep3",	 CMD_FUNC_NODE,  NULL,		 &cmd_enter_sleep3, "Enter sleep3"}, )
#endif // __FOR_DPM_SLEEP_CURRENT_TEST__
	CMD_OPTION_S0(  { "ble_gapm_reset",     CMD_FUNC_NODE,  NULL,       &cmd_gapm_reset,    "Reset BLE GAPM"}, )
	CMD_OPTION_S0(  { "ble_fw_ver",     CMD_FUNC_NODE,  NULL,       &cmd_ble_fw_ver,    "Get BLE FW version"}, )	
#if defined (__OTA_TEST_CMD__)
	CMD_OPTION_S0(  { "ota_test",     CMD_FUNC_NODE,  NULL,       &cmd_ota_test,    "OTA test command"}, )
#endif // __OTA_TEST_CMD__
	CMD_OPTION_S0(  { "adv_start",     CMD_FUNC_NODE,  NULL,       &cmd_ble_start_adv,    "BLE ADV start"}, )	
	CMD_OPTION_S0(  { "adv_stop",     CMD_FUNC_NODE,  NULL,       &cmd_ble_stop_adv,    "BLE ADV stop"}, )	
	CMD_OPTION_S0(  { "disconnect",     CMD_FUNC_NODE,  NULL,       &cmd_ble_disconnect,    "Disconnect BLE connection"}, )	
	CMD_OPTION_S0(	{ "get_dev_info", 	CMD_FUNC_NODE,	NULL,		&cmd_get_local_dev_info,	"Get local device info command"}, ) 
	//CMD_OPTION_S0(  { "ble_reset",     CMD_FUNC_NODE,  NULL,       &cmd_ble_reset,    "reset DA14531"}, )
	{ NULL, 		CMD_NULL_NODE,	NULL,		NULL,		NULL }			// Tail
};

void cmd_gapm_reset(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendGapmReset();
}

void cmd_ble_fw_ver(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendGetBleFwVer();
}

void cmd_ble_start_adv(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendstart_adv();
}

void cmd_ble_stop_adv(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendstop_adv();
}
void cmd_ble_disconnect(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ConsoleSendDisconnnect();
}

#if defined (__OTA_TEST_CMD__)
extern UINT ota_fw_update_combo(void);
void cmd_ota_test(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	ota_fw_update_combo();
}
#endif

void cmd_get_local_dev_info(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	app_get_local_dev_info(GAPM_GET_DEV_BDADDR);
}


#if defined (__EXCEPTION_RST_EMUL__)
void cmd_wifi_force_exception(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	asm("udf.w #0");
}
#endif

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
void cmd_ble_force_exception(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argv);

	if (argc >= 2)
	{
		return;
	}
	else
	{
		ConsoleSendBleForceException();
	}
}
#endif

#if defined (__TEST_FORCE_SLEEP2_WITH_TIMER__)
void cmd_enter_sleep2(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argv);

	int	sec;
	unsigned long long usec;

	if (argc != 2) {
		PRINTF("Usage : enter_sleep2 [sec]\n");
		return;
	}

	sec = atoi(argv[1]);

	usec = (sec * 1000000);
				if (dpm_mode_is_enabled())
				{
					PRINTF("sleep2 is entered, and DUT will wake up in %d sec\n", sec);
				}
				else
				{
					PRINTF("enable DPM and try again ! \n");
					return;
				}
	
#if defined (__WAKE_UP_BY_BLE__)
				enable_wakeup_by_ble();
#endif // (__WAKE_UP_BY_BLE__)
				// just to print the debug message above fully. It is safe to remove it
				vTaskDelay(20);
				dpm_sleep_start_mode_2(usec, true);

}
#endif // (__TEST_FORCE_SLEEP2_WITH_TIMER__)


#if defined (__FOR_DPM_SLEEP_CURRENT_TEST__)
void cmd_enter_sleep3(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argv);

	if (argc != 1) {
		PRINTF("Usage : enter_sleep3\n");
		return;
	}
	
#if defined (__WAKE_UP_BY_BLE__)
	enable_wakeup_by_ble();
#endif // (__WAKE_UP_BY_BLE__)

	RTC_CLEAR_EXT_SIGNAL(); // to remove wakeup src
	vTaskDelay(200);
	dpm_app_sleep_ready_set("test_sleep3");
}
#endif // (__FOR_DPM_SLEEP_CURRENT_TEST__)

void EnQueueToConsoleQ(console_msg *pmsg)
{
#ifdef __FREERTOS__
	if (msg_queue_send(&ConsoleQueue, 0, 0, (void *)pmsg, sizeof(console_msg), portMAX_DELAY) == OS_QUEUE_FULL) {
		configASSERT(0);
	}
	MEMFREE(pmsg);
#else
	xSemaphoreTake(ConsoleQueueMut, portMAX_DELAY);
	EnQueue(&ConsoleQueue, pmsg);
	xSemaphoreGive(ConsoleQueueMut);
#endif
}

void ConsoleSendGetBleFwVer(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_GET_BLE_FW_VER_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendGapmReset(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_GAPM_RESET;
	EnQueueToConsoleQ(pmsg);
}

#if defined (__TEST_FORCE_EXCEPTION_ON_BLE__)
void ConsoleSendBleForceException(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_FORCE_EXCEPTION_ON_BLE;
	EnQueueToConsoleQ(pmsg);
}
#endif


void ConsoleSendSensorStart(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_IOT_SENSOR_START;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendSensorStop(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}
	
	pmsg->type = CONSOLE_IOT_SENSOR_STOP;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendScan(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_DEV_DISC_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendConnnect(int indx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_CONNECT_CMD;
	pmsg->val = indx;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendDisconnnect(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_DISCONNECT_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendstart_adv(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_ADV_START_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendstop_adv(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_ADV_STOP_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleRead(unsigned char type)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = type;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleWrite(unsigned char type, unsigned char val)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = type;
	pmsg->val = val;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleSendExit(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}

	pmsg->type = CONSOLE_EXIT_CMD;
	EnQueueToConsoleQ(pmsg);
}

void ConsoleTitle(void)
{
	PRINTF ("\n\n\t\t####################################################\n");
	PRINTF ("\t\t#    DA14531 Proxr IoT Sensor demo application   #\n");
	PRINTF ("\t\t####################################################\n\n");
}

void ConsoleScan(void)
{
	ConsoleTitle();

	PRINTF ("\t\t\t\tScanning... Please wait \n\n");

	ConsolePrintScanList();
}

void ConsoleIdle(void)
{
	ConsoleTitle();
}

void ConsolePrintScanList(void)
{
	int index, i;

	PRINTF ("No. \tbd_addr \t\tName \t\tRssi\n");

	for (index = 0; index < MAX_SCAN_DEVICES; index++)
	{
		if (app_env.devices[index].free == false)
		{
			PRINTF("%d\t", index + 1);

			for (i = 0; i < BD_ADDR_LEN-1; i++)
			{
				PRINTF ("%02x:", app_env.devices[index].adv_addr.addr[BD_ADDR_LEN - 1 - i]);
			}

			PRINTF ("%02x", app_env.devices[index].adv_addr.addr[0]);

			PRINTF ("\t%s\t%d\n", app_env.devices[index].data, app_env.devices[index].rssi);
		}
	}

}

void ConsoleConnected(int full)
{
	int i;

	ConsoleTitle();

	PRINTF ("\t\t\t Connected to Device \n\n");


	PRINTF ("BLE Peer BDA: ");

	for (i = 0; i < BD_ADDR_LEN -1; i++)
	{
		PRINTF ("%02x:", app_env.proxr_device.device.adv_addr.addr[BD_ADDR_LEN - 1 - i]);
	}

	PRINTF ("%02x\t", app_env.proxr_device.device.adv_addr.addr[0]);

	PRINTF ("Bonded: ");

	if (app_env.proxr_device.bonded)
	{
		PRINTF ("YES\n");
	}
	else
	{
		PRINTF ("NO\n");
	}

	if (full)
	{

		if (app_env.proxr_device.llv != 0xFF)
		{
			PRINTF ("Link Loss Alert Lvl: %02d\t\t", app_env.proxr_device.llv);
		}
		else
		{
			PRINTF ("Link Loss Alert Lvl: \t\t");
		}

		if (app_env.proxr_device.txp != 127)
		{
			PRINTF ("Tx Power Lvl: %02d\n\n", app_env.proxr_device.txp);
		}
		else
		{
			PRINTF ("Tx Power Lvl:  \n\n");
		}

	}
}

#endif /* __BLE_PERI_WIFI_SVC_TCP_DPM__ */

/* EOF */
