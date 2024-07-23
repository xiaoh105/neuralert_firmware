/**
 ****************************************************************************************
 *
 * @file atcmd_transfer_mng.h
 *
 * @brief Header file for Ring-buffer function over DA16200/DA16600 AT-CMD
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

#ifndef __ATCMD_TRANSFER_MNG_H__
#define __ATCMD_TRANSFER_MNG_H__

#include "sdk_type.h"
#include "da16x_types.h"
#include "console.h"
#include "uart.h"
#include "lwrb.h"

#include "atcmd.h"
#include "common_uart.h"

#define	ATCMD_TR_MNG_TASK_NAME_LEN				20
#define	ATCMD_TR_MNG_DEF_TASK_NAME				"atcmd_tr_mng"
#define	ATCMD_TR_MNG_DEF_TASK_PRIORITY			OS_TASK_PRIORITY_USER_MAX
#define	ATCMD_TR_MNG_DEF_TASK_SIZE				256

#define	ATCMD_TR_MNG_DEF_TIMEOUT				1
#define	ATCMD_TR_MNG_DEF_RETRY_CNT				100

#if defined (TCP_MSS)
#define	ATCMD_TR_MNG_TCP_MSS					TCP_MSS
#else
#define	ATCMD_TR_MNG_TCP_MSS					1460
#endif // TCP_MSS

#define	ATCMD_TR_MNG_DEF_BUFFER_SIZE			(ATCMD_TR_MNG_TCP_MSS * 16)
#define	ATCMD_TR_MNG_DEF_IF_READ_BUFFER_SIZE	(ATCMD_TR_MNG_TCP_MSS * 5)

#define ATCMD_TR_MNG_EVT_DATA					0x01
#define ATCMD_TR_MNG_EVT_STOP					0x02
#define ATCMD_TR_MNG_EVT_ANY					0xFF

typedef struct _atcmd_tr_mng_config {
	char task_name[ATCMD_TR_MNG_TASK_NAME_LEN];
	size_t buflen;

	unsigned int wtimeout;
	unsigned int max_retry_cnt;
} atcmd_tr_mng_config;

typedef struct _atcmd_tr_mng_context {
	atcmd_tr_mng_config *conf;

	lwrb_t lwrb_buf;

	unsigned char *buf;
	size_t buflen;

	unsigned char *ir_buf;
	size_t ir_buflen;

	SemaphoreHandle_t buf_mutex;
	EventGroupHandle_t event;

	TaskHandle_t task_handler;
	size_t task_size;
	unsigned long task_priority;
} atcmd_tr_mng_context;

/**
 ****************************************************************************************
 * @brief Print-out to MCU
 * @param[in]  *fmt, ...
 * @return     None
 ****************************************************************************************
 */
void PRINTF_ATCMD(const char *fmt, ...);

/**
 ****************************************************************************************
 * @brief Print-out for ESCCMD to MCU
 * @param[in]  result_code  Result code for ESC-CMD
 * @return     None
 ****************************************************************************************
 */
void PRINTF_ESCCMD(UINT32 result_code);

/**
 ****************************************************************************************
 * @brief Send data which is defeined length to MCU
 * @param[in]  *data    Send data buffer
 * @param[in]  data_len Data length to send
 * @return     None
 ****************************************************************************************
 */
void PUTS_ATCMD(char *data, int data_len);

int atcmd_tr_mng_create_atcmd_mutex();
int atcmd_tr_mng_take_atcmd_mutex(unsigned int timeout);
int atcmd_tr_mng_give_atcmd_mutex();
int atcmd_tr_mng_init_context(atcmd_tr_mng_context *ctx);
int atcmd_tr_mng_deinit_context(atcmd_tr_mng_context *ctx);
int atcmd_tr_mng_init_config(atcmd_tr_mng_config *conf);
int atcmd_tr_mng_set_task_name(atcmd_tr_mng_config *conf, const char *name);
int atcmd_tr_mng_set_buflen(atcmd_tr_mng_config *conf, const size_t len);
int atcmd_tr_mng_set_config(atcmd_tr_mng_context *ctx, atcmd_tr_mng_config *conf);
int atcmd_tr_mng_create_task(atcmd_tr_mng_context *ctx);
int atcmd_tr_mng_delete_task(atcmd_tr_mng_context *ctx);
int atcmd_tr_mng_set_event(atcmd_tr_mng_context *ctx, unsigned int event);
size_t atcmd_tr_mng_read_buf(atcmd_tr_mng_context *ctx, unsigned char *buf, size_t buflen, unsigned int timeout);
size_t atcmd_tr_mng_write_buf(atcmd_tr_mng_context *ctx, unsigned char *buf, size_t buflen, unsigned int timeout);
unsigned int atcmd_tr_mng_get_event(atcmd_tr_mng_context *ctx, unsigned int timeout);
size_t atcmd_tr_mng_get_free(atcmd_tr_mng_context *ctx);
size_t atcmd_tr_mng_get_full(atcmd_tr_mng_context *ctx);

#endif // __ATCMD_TRANSFER_MNG_H__

/* EOF */
