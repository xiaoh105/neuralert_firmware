/**
 ****************************************************************************************
 *
 * @file sensor_gw_console.c
 *
 * @brief Basic console user interface for Sensor Gateway host application.
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
#include "app_errno.h"

#if defined (__BLE_CENT_SENSOR_GW__)

// ble platform common include
#include "../inc/da14585_config.h"
#include "compiler.h"
#include "rwble_config.h"
#include "proxm.h"

// gtl platform common include
#include "../../include/queue.h"

// project specific include
#include "../inc/console.h"
#include "../inc/app.h"
#include "../inc/app_task.h"
#define __BOOL_DEFINED

#include "da16x_system.h"
#include "common_def.h"
#include "common_utils.h"
#include "user_dpm_api.h"
#include "command.h"

#include "da16_gtl_features.h"
#ifdef __FREERTOS__
#include "msg_queue.h"
#endif
#include "lwip/err.h"

#define	CMD_OPTION_S0(...)	__VA_ARGS__
#define CMD_OPTION_0(...)	__VA_ARGS__

bool console_mode = false;					//Display of selected connection info
bool giv_num_state = false;					//Print the "Give Device number!!" message
bool conn_num_flag = false;					//Print the "Max connections Reached" message

/* 
 *	For sensor connection only: false only if no connection has been established
 *		note: if a provisioning app is connected (provision mode), 
 *		this dose not become true when connected
 */
bool connected_to_at_least_one_peer = false; 

bool keyflag = false;						//For the up-down arrows


//-----------------------------------------------------------------------
// Command Function-List
//-----------------------------------------------------------------------

extern void cmd_proxm(int argc, char *argv[]);
#ifndef __FREERTOS__
extern SemaphoreHandle_t ConsoleQueueMut;
#endif
#if defined (__EXCEPTION_RST_EMUL__)
extern void cmd_wifi_force_exception(int argc, char *argv[]);
#endif

//-----------------------------------------------------------------------
// Command BLE-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_ble_list[] =
{
	{ "ble", CMD_MY_NODE, cmd_ble_list,	NULL, "ble app command "},  // Head
	CMD_OPTION_S0(	{ "-------", CMD_FUNC_NODE,	NULL, NULL,	"------------------------"}, )
#if defined (__BLE_CENT_SENSOR_GW__)
	CMD_OPTION_S0(	{ "proxm_sensor_gw", CMD_FUNC_NODE,	NULL, &cmd_proxm, "ble proximity monitor user command"}, )
	CMD_OPTION_S0(	{ "ble_sw_reset", CMD_FUNC_NODE, NULL, &cmd_proxm, "ble software reset user command"}, )
	CMD_OPTION_S0(  { "adv_start",     CMD_FUNC_NODE,  NULL,       &cmd_proxm,    "BLE ADV start"}, )	
	CMD_OPTION_S0(  { "adv_stop",     CMD_FUNC_NODE,  NULL,       &cmd_proxm,    "BLE ADV stop"}, )	
#endif
#if defined (__EXCEPTION_RST_EMUL__)	
	CMD_OPTION_S0(	{ "wifi_force_excep", CMD_FUNC_NODE,  NULL, &cmd_wifi_force_exception,"force undef_instr exception on wifi"}, )
#endif
	{NULL, CMD_NULL_NODE, NULL, NULL, NULL }						// Tail
};

#if defined (__EXCEPTION_RST_EMUL__)
void cmd_wifi_force_exception(int argc, char *argv[])
{
		asm("udf.w #0");
}
#endif

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

void ConsoleSendScan(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}		

	pmsg->type = CONSOLE_DEV_DISC_CMD;

	EnQueueToConsoleQ(pmsg);

}

void ConsoleReadRSSI(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_RD_RSSI_CMD;

	EnQueueToConsoleQ(pmsg);

}

void ConsoleIoTSensorRdTemp(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_IOT_SENSOR_TEMP_RD_CMD;

	EnQueueToConsoleQ(pmsg);

}


void ConsoleIoTSensorTempPostCtl(enum IOT_SENSOR_TEMP_POST_CTL iot_sensor_temp_post_ctl,
								 uint8_t peer_idx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_IOT_SENSOR_TEMP_POST_CTL_CMD;
	pmsg->idx = peer_idx;
	pmsg->val = iot_sensor_temp_post_ctl;

	EnQueueToConsoleQ(pmsg);

}


void ConsoleSendConnnect(int indx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_CONNECT_CMD;
	pmsg->idx = indx;

	EnQueueToConsoleQ(pmsg);

}


void ConsoleSendDisconnnect(int id)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_DISCONNECT_CMD;
	pmsg->val = id;

	EnQueueToConsoleQ(pmsg);


}

void ConsoleRead(unsigned char type, unsigned char idx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = type;
	pmsg->idx = idx;

	EnQueueToConsoleQ(pmsg);

}


void ConsoleWrite(unsigned char type, unsigned char val, unsigned char idx)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = type;
	pmsg->val = val;
	pmsg->idx = idx;

	EnQueueToConsoleQ(pmsg);

}


void ConsoleSendExit(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_EXIT_CMD;

	EnQueueToConsoleQ(pmsg);

}

/**
	@brief ble proximity profile (report role) test commands

	e.g.) 	proxr 					// display help
		proxr dev_discovery 			// start ble device scan
		proxr conn [1~9]			// start connection to a ble peer
		proxr disconn				// disconnect from a ble peer
		proxr wr_ias [high | mild | none] 	// immediate alert service
		proxr wr_llv [high | mild | none] 	// link loss service

	argc = 2
*/

#if defined (__WFSVC_ENABLE__)
void ConsoleSendProvMode(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		DBG_ERR("[%s] Failed to alloc ! \n", __func__);
		return;
	}			

	pmsg->type = CONSOLE_ENABLE_WFSVC_CMD;

	EnQueueToConsoleQ(pmsg);

}
#endif

void ConsoleClear(void)
{
	VT_CLEAR;			/* Clear screen */
	VT_CURPOS(0, 0); 	/* move cursor position : 0,0 */
}

void ConsoleTitle(void)
{
	PRINTF ("\t  ####################################################\n");
	PRINTF ("\t  #   DA14531 Proxm Sensor Gateway Demo Application  #\n");
	PRINTF ("\t  ####################################################\n\n");
}


void ConsoleScan(void)
{
	if (!connected_to_at_least_one_peer)
	{
		ConsoleClear();
		ConsoleTitle();
		ConsoleIdle();
	}
	else
	{
		ConsoleClear();
		ConsoleTitle();
		ConsolePrintScanList();
	}
}


void ConsoleIdle(void)
{
	if (!connected_to_at_least_one_peer)
	{
		ConsolePrintScanList();
		PRINTF ("Scanning... Please wait");
	}
}


void ConsolePrintScanList(void)
{
	int index, i;

	if (!connected_to_at_least_one_peer)
	{
		console_mode = false;
	}

	PRINTF ("#  No. \tbd_addr \t\tName \t\t\tRssi\t\t#\n");

	for (index = 0; index < MAX_SCAN_DEVICES; index++)
	{
		if (app_env.devices[index].free == false)
		{
			PRINTF("#  %d\t", index + 1);

			for (i = 0; i < BD_ADDR_LEN-1; i++)
			{
				PRINTF ("%02x:", app_env.devices[index].adv_addr.addr[BD_ADDR_LEN - 1 - i]);
			}

			PRINTF ("%02x", app_env.devices[index].adv_addr.addr[0]);

			if ( app_env.devices[index].data[ (strlen((char *) app_env.devices[index].data)) - 1 ] < 32 )
			{
				app_env.devices[index].data[ (strlen((char *) app_env.devices[index].data)) - 1 ] = '\0';
			}

			if ( strlen((char *) app_env.devices[index].data) == 0 )
			{
				PRINTF ("\t\t\t\t%d dB\t\t#\n", app_env.devices[index].rssi);
			}
			else if ( strlen((char *) app_env.devices[index].data) < 8 )
			{
				PRINTF ("\t%s\t\t\t%d dB\t\t#\n", app_env.devices[index].data,
						app_env.devices[index].rssi);
			}
			else if ( strlen((char *) app_env.devices[index].data) < 16 )
			{
				PRINTF ("\t%s\t\t%d dB\t\t#\n", app_env.devices[index].data, app_env.devices[index].rssi);
			}
			else if ( strlen((char *) app_env.devices[index].data) < 24 )
			{
				PRINTF ("\t%s\t%d dB\t\t#\n", app_env.devices[index].data, app_env.devices[index].rssi);
			}
			else
			{
				PRINTF ("\t%.23s\t%d dB\t\t#\n", app_env.devices[index].data,
						app_env.devices[index].rssi);
			}
		}
	}
}


void ConsoleDevInfo(void)

{
	USHORT i;

	PRINTF ("#																		 #\n");
	PRINTF ("#	Device %d Information												  #\n", app_env.cur_dev + 1);
	PRINTF ("#	------------------													 #\n");
	PRINTF ("#																		 #\n");

	PRINTF ("#	Manufacturer: %s",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MANUFACTURER_NAME_CHAR].val);
	printtabs (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MANUFACTURER_NAME_CHAR].val), 1);

	PRINTF ("#	Model Number: %s",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MODEL_NB_STR_CHAR].val);
	printtabs (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MODEL_NB_STR_CHAR].val), 1);

	PRINTF ("#	Serial Number: %s\t\t\t\t\t\t\t#\n",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SERIAL_NB_STR_CHAR].val);

	PRINTF ("#	Hardware Revision: %s",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_HARD_REV_STR_CHAR].val);
	printtabs (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_HARD_REV_STR_CHAR].val ), 2);

	PRINTF ("#	Firmware Revision: %s",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_FIRM_REV_STR_CHAR].val);
	printtabs (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_FIRM_REV_STR_CHAR].val ), 2);

	PRINTF ("#	Software Revision: %s",
			app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SW_REV_STR_CHAR].val);
	printtabs (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SW_REV_STR_CHAR].val ), 2);

	if (strlen((char *)
			   app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].val) == 0)
	{
		PRINTF("#  System ID:  \t\t\t\t\t\t\t\t#\n");
	}
	else
	{
		PRINTF ("#	System ID: %02x",
				(uint8_t) app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].val[0]);

		for (i = 1; i < app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].len; i++)
		{
			PRINTF (":%02x", (uint8_t)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].val[i]);
		}

		PRINTF("\t\t\t\t\t#\n");
	}

	PRINTF ("#	IEEE Certification: %02x",
			(uint8_t) app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].val[0]);

	for (i = 1; i < app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].len; i++)
	{
		PRINTF (":%02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].val[i]);
	}

	PRINTF("\t\t\t\t\t\t#\n");

	if (strlen((char *)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].val) == 0)
	{
		PRINTF("#  System ID:  \t\t\t\t\t\t\t\t#\n");
	}
	else
	{
		PRINTF ("#	PnP ID: %02x", (uint8_t)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].val[0]);

		for (i = 1; i < app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].len; i++)
		{
			PRINTF (":%02x", (uint8_t)app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].val[i]);
		}

		PRINTF ("\t\t\t\t\t\t#\n");
	}

}


void ConsoleConnected(int full)
{
	ConsoleTitle();

	if (full)
	{
		connected_to_at_least_one_peer = true;
	}

	PRINTF("#########################################################################");
	ConsoleQues(); // connected device header
}

void ConsoleQues(void)
{
	unsigned int cnt, i;

	PRINTF ("\n#########################   Connected Devices   #########################\n");
	PRINTF (  "#########################################################################\n");
	PRINTF (  "                                                                       \n");
	PRINTF (  "#  No. Model No.\tBDA\t\t   Bonded  RSSI   LLA  TX_PL  Temp\t\n");

	for (cnt = 0; cnt < MAX_CONN_NUMBER; cnt++)
	{
		if (cnt == app_env.cur_dev)
		{
			PRINTF ("#  * %d", cnt + 1); //Active Counter
		}
		else
		{
			PRINTF ("#    %d", cnt + 1); //Counter
		}

		if (app_env.proxr_device[cnt].isactive == true)
		{
			// model No.
			if (strlen((char *) app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val) == 0)
			{
				if (app_env.proxr_device[cnt].iot_sensor.svc_found)
				{
					PRINTF (" iot_sensor       ");
				}
				else
				{
					PRINTF (" \t\t\t");
				}

			}
			else if (strlen((char *) app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val) < 8)
			{
				PRINTF (" %s\t\t", app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val);
			}
			else
			{
				PRINTF (" %.16s\t", app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val);
			}

			// BDA
			for (i = 0; i < BD_ADDR_LEN-1; i++) //BDA
			{
				PRINTF ("%02x:", app_env.proxr_device[cnt].device.adv_addr.addr[BD_ADDR_LEN - 1 - i]);
			}

			PRINTF ("%02x  ", app_env.proxr_device[cnt].device.adv_addr.addr[0]);

			// Bonded
			if (app_env.proxr_device[cnt].bonded) //BONDED
			{
				PRINTF (" YES\t  ");
			}
			else
			{
				PRINTF (" NO\t  ");
			}

			// RSSI
			if (app_env.proxr_device[cnt].avg_rssi != -127)	  //RSSI
			{
				PRINTF ("%02d dB", app_env.proxr_device[cnt].avg_rssi);
			}
			else
			{
				PRINTF ("\t");
			}

			if (app_env.proxr_device[cnt].alert)
			{
				PRINTF ("\a  PLA\t\t"); //Path Loss Alert

				if (app_env.proxr_device[cnt].iot_sensor.temp_val[0] != (uint8_t)100)
				{
					PRINTF ("%4d\t\n", app_env.proxr_device[cnt].iot_sensor.temp_val[0]);
				}
				else
				{
					PRINTF ("   \t\t\n");
				}
			}
			else
			{
				// LLA
				if (app_env.proxr_device[cnt].llv != 0xFF) //Link Loss Alert Level
				{
					PRINTF ("   %02d\t", app_env.proxr_device[cnt].llv);
				}
				else
				{
					PRINTF ("    \t");
				}

				// TX_PL (Tx Power Level)
				if (app_env.proxr_device[cnt].txp != -127)
				{
					//PRINTF (" %02d\t", app_env.proxr_device[cnt].txp);
					PRINTF (" %02d", app_env.proxr_device[cnt].txp);
				}
				else
				{
					//PRINTF ("    \t");
					PRINTF ("   ");
				}

				if (app_env.proxr_device[cnt].iot_sensor.temp_val[0] != (uint8_t)100)
				{
					PRINTF ("   %4d\t\n", app_env.proxr_device[cnt].iot_sensor.temp_val[0]);
				}
				else
				{
					PRINTF ("   \t\t\n");
				}

			}

			PRINTF ("\n");
		}
		else
		{
			PRINTF ("                --   Empty Slot --                                \n");
		}
	}

	PRINTF ("#                                                                       \n");
	PRINTF ("#########################################################################\n");

	if (giv_num_state)
	{
		PRINTF ("#                                                                       #\n");
		PRINTF ("#---------------------------  Select Device  ---------------------------#\n");
		PRINTF ("#                                                                       #\n");
	}

}


void print_err_con_num(void)
{

	PRINTF ("#########################################################################\n");
	PRINTF ("##                    max no of connections reached                    ##\n");
	PRINTF ("#########################################################################\n");

}


void printtabs(int size, int mode)
{
	if (mode == 1)
	{
		if (size < 7)
		{
			PRINTF ("\t\t\t\t\t\t\t#\n");
		}
		else if (size < 15)
		{
			PRINTF ("\t\t\t\t\t\t#\n");
		}
		else
		{
			PRINTF ("\t\t\t\t\t#\n");
		}
	}
	else if (mode == 2)
	{
		if (size < 2)
		{
			PRINTF ("\t\t\t\t\t\t\t#\n");
		}
		else if (size < 10)
		{
			PRINTF ("\t\t\t\t\t\t#\n");
		}
		else if (size < 18)
		{
			PRINTF ("\t\t\t\t\t#\n");
		}
		else
		{
			PRINTF ("\t\t\t\t#\n");
		}
	}
}

void ConsoleSendBleSWReset(void)
{
	console_msg *pmsg = (console_msg *)MEMALLOC(sizeof(console_msg));

	if (pmsg == NULL)
	{
		PRINTF("[%s] Failed to alloc ! \n", __func__);
		return;
	}	

	pmsg->type = CONSOLE_BLE_SW_RESET_CMD;
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

void cmd_proxm(int argc, char *argv[])
{
	UINT	status = ERR_OK;
	int scan_dev_idx;
	short peer_idx;
	char *cmd_to_peer;
	USHORT i;

	char **cmd_argv = argv;
	char **cur_argv = ++argv;

	if (argc >= 2 && argc <= 4)
	{

		// "scan": Scan Advertising devices
		if (strcmp("scan", *cur_argv) == 0)
		{
			for (i = 0; i < MAX_SCAN_DEVICES; i++)
			{
				memset( app_env.devices[i].data, '\0', ADV_DATA_LEN + 1 );
			}

			ConsoleSendScan();
		}

		// "show_conn_dev" : show connected device
		else if (strcmp("show_conn_dev", *cur_argv) == 0)
		{
			ConsoleConnected(0);
		}

#if defined (__WFSVC_ENABLE__)
		// "provision_mode" : enable Provision Mode
		else if (strcmp("provision_mode", *cur_argv) == 0)
		{
			ConsoleSendProvMode();
		}

#endif
		// "rd_rssi_conn_dev" : read rssi for all connected devices
		else if (strcmp("rd_rssi_conn_dev", *cur_argv) == 0)
		{
			ConsoleReadRSSI();
		}

		// "exit" : all peers are disconnected
		else if (strcmp("exit", *cur_argv) == 0)
		{
			ConsoleSendExit();
		}

		// "read_temp" : read temperature from iot_sensors
		else if (strcmp("read_temp", *cur_argv) == 0)
		{
			ConsoleIoTSensorRdTemp();
		}

		// "conn [1~9]" : connect to a ble peer
		else if (strcmp("conn", *cur_argv) == 0)
		{
			cur_argv = ++argv;

			scan_dev_idx = (atoi(*cur_argv)) - 1;
			DBG_INFO("scan_dev_idx = %d \n", scan_dev_idx);

			if (scan_dev_idx < 0 || scan_dev_idx > MAX_SCAN_DEVICES - 1)
			{
				DBG_ERR("wrong scanned ble peer index \n");
				return;
			}

			int avail_conn_slot_idx = rtrn_fst_avail();
			DBG_INFO("avail_conn_slot_idx = %d \n", avail_conn_slot_idx);

			if ((app_env.devices[scan_dev_idx].free == false) &&
					(avail_conn_slot_idx < MAX_CONN_NUMBER) /*&&
				!connected_to_at_least_one_peer */ )
			{

				PRINTF ("\t\t\t\tTrying to connect to %d... Please wait \n\n", scan_dev_idx);
				ConsoleSendConnnect(scan_dev_idx);
				vTaskDelay(10);
			}
			else if (avail_conn_slot_idx == MAX_CONN_NUMBER)
			{
				print_err_con_num();
				return;
			}
			else
			{
				DBG_ERR("please try scan and connection again \n");
				return;
			}
		}

		// "peer [1~8] [A|B|C|D|E|F|G|H|I|Q]": cmd to a ble peer connected
		else if (strcmp("peer", *cur_argv) == 0)
		{
			cur_argv = ++argv;
			peer_idx = (atoi(*(cur_argv))) - 1; // [1-9]

			cur_argv = ++argv;
			cmd_to_peer = *cur_argv; // [cmd]

			if (peer_idx < 0 || peer_idx > MAX_CONN_NUMBER - 1)
			{
				DBG_ERR("wrong ble peer index \n");
				return;
			}

			if (app_env.proxr_device[peer_idx].isactive != true)
			{
				DBG_ERR("the ble peer specified is not connected \n");
				return;
			}
			else
			{
				app_env.cur_dev = peer_idx;
			}

			// Read Link Loss Alert Level
			if (strcmp("A", cmd_to_peer) == 0)
			{
				ConsoleRead(CONSOLE_RD_LLV_CMD, app_env.cur_dev);
				vTaskDelay(10);
			}
			// Read Tx Power Level
			else if (strcmp("B", cmd_to_peer) == 0)
			{
				ConsoleRead(CONSOLE_RD_TXP_CMD, app_env.cur_dev);
				vTaskDelay(10);
			}
			// Start High Level Immediate Alert
			else if (strcmp("C", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_HIGH, app_env.cur_dev);
			}
			// Start Mild Level Immediate Alert
			else if (strcmp("D", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_MILD, app_env.cur_dev);
			}
			// Stop Immediate Alert
			else if (strcmp("E", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_NONE, app_env.cur_dev);
			}
			// Set Link Loss Alert Level to None
			else if (strcmp("F", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_NONE, app_env.cur_dev);
				vTaskDelay(10);
				ConsoleRead(CONSOLE_RD_LLV_CMD, app_env.cur_dev);
			}
			// Set Link Loss Alert Level to Mild
			else if (strcmp("G", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_MILD, app_env.cur_dev);
				vTaskDelay(10);
				ConsoleRead(CONSOLE_RD_LLV_CMD, app_env.cur_dev);
			}
			// Set Link Loss Alert Level to High
			else if (strcmp("H", cmd_to_peer) == 0)
			{
				ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_HIGH, app_env.cur_dev);
				vTaskDelay(10);
				ConsoleRead(CONSOLE_RD_LLV_CMD, app_env.cur_dev);
			}
			// Enable iot sensor's temperature posting
			else if (strcmp("J", cmd_to_peer) == 0)
			{
				ConsoleIoTSensorTempPostCtl(IOT_SENSOR_TEMP_POST_ENABLE, app_env.cur_dev);
				vTaskDelay(10);
				//ConsoleConnected(0);
			}
			// Disable iot sensor's temperature posting
			else if (strcmp("K", cmd_to_peer) == 0)
			{
				ConsoleIoTSensorTempPostCtl(IOT_SENSOR_TEMP_POST_DISABLE, app_env.cur_dev);
				vTaskDelay(10);
			}
			// Disconnect from device
			else if (strcmp("Z", cmd_to_peer) == 0)
			{
				ConsoleSendDisconnnect(app_env.cur_dev);
			}
			// Show Device Information
			else if (strcmp("I", cmd_to_peer) == 0)
			{
				ConsoleDevInfo();
			}
			else
			{
				DBG_ERR("wrong ble peer command !!\n");
			}
		}

		//return;
	}
	else if (argc == 1 && (strcmp("ble_sw_reset", *cmd_argv) == 0))
	{
		ConsoleSendBleSWReset();
	}
	else if (argc == 1 && (strcmp("adv_start", *cmd_argv) == 0))
	{
		ConsoleSendstart_adv();
	}
	else if (argc == 1 && (strcmp("adv_stop", *cmd_argv) == 0))
	{
		ConsoleSendstop_adv();
	}
	else
	{
		status = DA_APP_INVALID_PARAMETERS;
	}

	if (status)
	{
		PRINTF("Usage: BLE PROXM & Sensor Gateway Sample Application command \n");

		PRINTF("\x1b[93mName\x1b[0m\n");
		PRINTF("\tproxm_sensor_gw - Proximity Monitor and Sensor GW cmd \n");

		PRINTF("\x1b[93mSYNOPSIS\x1b[0m\n");
		PRINTF("\tproxm_sensor_gw [OPTION]\n");

		PRINTF("\x1b[93mOPTION DESCRIPTION\x1b[0m\n");

		PRINTF("\t\x1b[93m scan \x1b[0m\n");
		PRINTF("\t\t Scan BLE peers around \n");

		PRINTF("\t\x1b[93m show_conn_dev \x1b[0m\n");
		PRINTF("\t\t shows connected BLE peers with status \n");
#if defined (__WFSVC_ENABLE__)
		PRINTF("\t\x1b[93m provision_mode \x1b[0m\n");
		PRINTF("\t\t start Wi-Fi GATT Service for Provisioning \n");
#endif
		PRINTF("\t\x1b[93m rd_rssi_conn_dev \x1b[0m\n");
		PRINTF("\t\t read rssi for all connected devices \n");

		PRINTF("\t\x1b[93m read_temp \x1b[0m\n");
		PRINTF("\t\t read temperature sensor values from all connected devices \n");

		PRINTF("\t\x1b[93m conn [1~9] \x1b[0m\n");
		PRINTF("\t\t connect to a ble peer from the scan list \n");
		PRINTF("\t\t please choose index from the scan list \n");

		PRINTF("\t\x1b[93m peer [1~9] [A|B|...|Z] \x1b[0m\n");
		PRINTF("\t\t take an action on a connected BLE peer \n");
		PRINTF("\t\t ------------proxm cmd ---------------- \n");
		PRINTF("\t\t A: Read Link Loss Alert Level \n");
		PRINTF("\t\t B: Read Tx Power Level \n");
		PRINTF("\t\t C: Start High Level Immediate Alert \n");
		PRINTF("\t\t D: Start Mild Level Immediate Alert \n");
		PRINTF("\t\t E: Stop Immediate Alert  \n");
		PRINTF("\t\t F: Set Link Loss Alert Level to None \n");
		PRINTF("\t\t G: Set Link Loss Alert Level to Mild \n");
		PRINTF("\t\t H: Set Link Loss Alert Level to High \n");
		PRINTF("\t\t I: Show device info \n");
		PRINTF("\t\t ------------custom cmd ----------------\n");
		PRINTF("\t\t J: Enable iot sensor's temperature posting \n");
		PRINTF("\t\t K: Disable iot sensor's temperature posting \n");
		PRINTF("\t\t ------------common cmd ----------------\n");
		PRINTF("\t\t Z: Disconnect from the device \n");

		PRINTF("\t\x1b[93m exit \x1b[0m\n");
		PRINTF("\t\t all peers are disconnected \n");
	}

	return;
}

#endif /* End of __BLE_CENT_SENSOR_GW__ */

/* EOF */
