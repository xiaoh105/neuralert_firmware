/**
 *****************************************************************************************
 * @file	wpa_ctrl.h
 * @brief	wpa_supplicant/hostapd control interface library from wpa_supplicant-2.4
 *****************************************************************************************
 */

/*
 * wpa_supplicant/hostapd control interface library
 * Copyright (c) 2004-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * Copyright (c) 2020-2022 Modified by Renesas Electronics.
 */

#include "includes.h"
#include "wpa_ctrl.h"
#include "supp_eloop.h"

#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

/* For Event Group */
extern EventGroupHandle_t da16x_sp_event_group;

/* For Message queue */
extern QueueHandle_t TO_SUPP_QUEUE;
extern QueueHandle_t da16x_cli_msg_tx_q;
extern QueueHandle_t da16x_cli_msg_rx_q;

#ifdef CONFIG_SCAN_REPLY_OPTIMIZE
extern UINT da16x_cli_eloop_run(char **reply);
#else
extern UINT da16x_cli_eloop_run(char *reply);
#endif /* CONFIG_SCAN_REPLY_OPTIMIZE */


#ifdef CONFIG_SCAN_REPLY_OPTIMIZE
int wpa_ctrl_request(const char *cmd, size_t cmd_len,
		     char  **reply, size_t *reply_len,
		     void (*msg_cb)(char *msg))
#else
int wpa_ctrl_request(const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len,
		     void (*msg_cb)(char *msg))
#endif /* CONFIG_SCAN_REPLY_OPTIMIZE */
{
	UINT	status;
	UINT	rsp_status;
	ULONG	da16x_events = 0;
#ifdef CONFIG_SUPP_EVENT_DEPRECATED
	ULONG	temp_pack[2];
#endif
	da16x_cli_tx_msg_t	cli_cmd_tx_buf;	/* CLI -> Supplicant */

	da16x_cli_tx_msg_t *cli_cmd_tx_msg_p = (da16x_cli_tx_msg_t *)&cli_cmd_tx_buf;
	void *cli_tx_cmd_buf;

	CHAR *name;
	ULONG enqueued;
	TaskHandle_t *first_suspended;
	ULONG available_storage, suspended_count;
	QueueHandle_t next_queue;

	if (cmd_len > DA16X_QUEUE_BUF_SIZE) {
		da16x_cli_prt("[%s] cmd_len=%d\n", __func__, cmd_len);
		return -1;
	}

	da16x_cli_prt("<%s> START : cmd=[%s]\n", __func__, cmd);

	/* Checking the status of supplicant */

#if 0
	status = tx_queue_info_get(&TO_SUPP_QUEUE,
								&name,
								&enqueued,
								&available_storage,
								&first_suspended,
								&suspended_count,
								&next_queue);

	if (status == ERR_OK) {
		if (name == NULL) {
#ifdef CONFIG_SCAN_REPLY_OPTIMIZE
			memset(*reply, 0, 6);
			memcpy(*reply, "ERROR", 5);
#else
			strcpy(reply, "ERROR");
#endif /* CONFIG_SCAN_REPLY_OPTIMIZE */
			rsp_status = ER_QUEUE_ERROR;
			goto REQ_END;
		}
	}
#endif	/*0 F_F_S*/


	cli_tx_cmd_buf = pvPortMalloc(sizeof(da16x_cli_cmd_buf_t));

	if (cli_tx_cmd_buf == NULL)
	{
		return -1;
	}

	memset((void *)cli_tx_cmd_buf, 0, sizeof(da16x_cli_cmd_buf_t));

	cli_cmd_tx_msg_p->cli_tx_buf = (da16x_cli_cmd_buf_t *)cli_tx_cmd_buf;
	strcpy(cli_cmd_tx_msg_p->cli_tx_buf->cmd_str, cmd);

	if (strncmp(cmd, "SCAN_RESULTS", 12) == 0) {
		da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
										DA16X_SCAN_RESULTS_RX_EV | DA16X_SCAN_RESULTS_FAIL_EV,
										pdFALSE,
										pdFALSE,
										500);

		if(da16x_events & DA16X_SCAN_RESULTS_RX_EV)
		{
			da16x_cli_prt("Scan succeeded. \n");
		} else if(da16x_events & DA16X_SCAN_RESULTS_FAIL_EV) {
			da16x_cli_prt("Scan Failed. \n");

			da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
											DA16X_SCAN_RESULTS_FAIL_EV,
											pdTRUE,
											pdFALSE,
											portNO_DELAY);

			da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
											DA16X_SCAN_RESULTS_TX_EV,
											pdTRUE,
											pdFALSE,
											portNO_DELAY);

			vPortFree(cli_tx_cmd_buf);

			return -1;
		} else {
			da16x_cli_prt("<%s> No scan result received (da16x_events=%x)\r\n", __func__, da16x_events);

			da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
											DA16X_SCAN_RESULTS_TX_EV,
											pdTRUE,
											pdFALSE,
											portNO_DELAY);

			vPortFree(cli_tx_cmd_buf);

			return -1;
		}
	}

	da16x_events = xEventGroupWaitBits(da16x_sp_event_group,
									DA16X_SCAN_RESULTS_RX_EV,
									pdTRUE,
									pdFALSE,
									portNO_DELAY);

#ifdef CONFIG_SUPP_EVENT_DEPRECATED
	temp_pack[0] = DA16X_SP_CLI_TX_FLAG;
	temp_pack[1] = (ULONG)cli_cmd_tx_msg_p;

	status = xQueueSend(TO_SUPP_QUEUE, &temp_pack, portMAX_DELAY/*portNO_DELAY*/);
#else
	ULONG *temp_pack;

	temp_pack = (void *) pvPortMalloc(sizeof(ULONG)*2);

	if (temp_pack == NULL)
	{
			return -1;
	}

	temp_pack[0] = DA16X_SP_CLI_TX_FLAG;
	temp_pack[1] = (ULONG)cli_cmd_tx_msg_p;

	status = xQueueSend(TO_SUPP_QUEUE, &temp_pack, portMAX_DELAY/*portNO_DELAY*/);
#endif // CONFIG_SUPP_EVENT_DEPRECATED

	if (status != pdTRUE)
	{
		da16x_err_prt("[%s] msg send error !!! (%d)\r\n", __func__, status);

#ifndef CONFIG_SUPP_EVENT_DEPRECATED
		if (temp_pack)
		{
			vPortFree(temp_pack);
		}
#endif // CONFIG_SUPP_EVENT_DEPRECATED
		if (cli_tx_cmd_buf)
		{
			vPortFree(cli_tx_cmd_buf);
		}
		return -1;
	}

	/* Receive Message */
	rsp_status = da16x_cli_eloop_run(reply);
REQ_END:

	if (*reply) {
#ifdef CONFIG_SCAN_REPLY_OPTIMIZE
	*reply_len = strlen(*reply); 
#else
	*reply_len = strlen(reply); 
#endif /* CONFIG_SCAN_REPLY_OPTIMIZE */
	} else {
		*reply_len = 0;
	}
 
	da16x_cli_prt("<%s> FINISH\n", __func__);

	if (rsp_status == pdTRUE) {
		rsp_status = 0; // Success
	}
	else
	{
		rsp_status = 1; // Fail
	}
	return rsp_status;
}

#if 0	/* by Shingu 20160926 (WPA_CLI Optimize) */
int wpa_ctrl_recv(char *reply, size_t *reply_len)
{
	da16x_cli_eloop_run(reply);

	return 0;
}
#endif	/* 0 */
/* EOF */
