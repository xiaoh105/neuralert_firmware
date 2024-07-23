/**
 ****************************************************************************************
 *
 * @file atcmd_transfer_mng.c
 *
 * @brief Ring-buffer function over DA16200/DA16600 AT-CMD
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

#include "atcmd_transfer_mng.h"
#include "da16x_sys_watchdog.h"

#undef ENABLE_ATCMD_TR_MNG_DBG_INFO
#define ENABLE_ATCMD_TR_MNG_DBG_ERR

#define	ATCMD_TR_MNG_DBG	ATCMD_DBG

#if defined (ENABLE_ATCMD_TR_MNG_DBG_INFO)
#define	ATCMD_TR_MNG_INFO(fmt, ...)	\
	ATCMD_TR_MNG_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_TR_MNG_INFO(...)	do {} while (0)
#endif	// (ENABLE_ATCMD_TR_MNG_DBG_INFO)

#if defined (ENABLE_ATCMD_TR_MNG_DBG_ERR)
#define	ATCMD_TR_MNG_ERR(fmt, ...)	\
	ATCMD_TR_MNG_DBG("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define	ATCMD_TR_MNG_ERR(...)	do {} while (0)
#endif // (ENABLE_ATCMD_TR_MNG_DBG_ERR) 

#define	ATCMD_TR_MNG_START	ATCMD_TR_MNG_INFO("Start\n")
#define	ATCMD_TR_MNG_END	ATCMD_TR_MNG_INFO("End\n")

#if defined (__ENABLE_TRANSFER_MNG__)
char g_atcmd_tr_mng_print_buf[UART_PRINT_BUF_SIZE];
#endif // __ENABLE_TRANSFER_MNG__

#if defined (__ENABLE_TRANSFER_MNG_TEST_MODE__)
unsigned char g_atcmd_tr_mng_test_flag = pdFALSE;
#endif // __ENABLE_TRANSFER_MNG_TEST_MODE__

SemaphoreHandle_t atcmd_mutex = NULL;

extern int host_response(unsigned int buf_addr, unsigned int len, unsigned int resp,
						 unsigned int padding_bytes);
extern int host_response_esc(unsigned int buf_addr, unsigned int len, unsigned int resp,
						 unsigned int padding_bytes);
extern int atcmd_dpm_wait_ready(int timeout);

#if	defined(__ATCMD_IF_UART1__)
extern HANDLE	uart1;
#elif defined ( __ATCMD_IF_UART2__ )
extern HANDLE	uart2;
#endif

static void *atcmd_tr_mng_internal_calloc(size_t n, size_t size);
static void atcmd_tr_mng_internal_free(void *f);
static int atcmd_tr_mng_puts_atcmd(unsigned char *data, size_t data_len);
static void atcmd_tr_mng_task_entry(void *pvParameters);

void *(*atcmd_tr_mng_calloc)(size_t n, size_t size) = atcmd_tr_mng_internal_calloc;
void (*atcmd_tr_mng_free)(void *ptr) = atcmd_tr_mng_internal_free;

static void *atcmd_tr_mng_internal_calloc(size_t n, size_t size)
{
	void *buf = NULL;
	size_t buflen = (n * size);

	buf = pvPortMalloc(buflen);
	if (buf) {
		memset(buf, 0x00, buflen);
	}

	return buf;
}

static void atcmd_tr_mng_internal_free(void *f)
{
	if (f == NULL) {
		return;
	}

	vPortFree(f);
}

static int atcmd_tr_mng_puts_atcmd(unsigned char *data, size_t data_len)
{
	ATCMD_TR_MNG_START;

	if (data == NULL || data_len == 0) {
		ATCMD_TR_MNG_ERR("Invalid parameter(%d)\n",data_len);
		return -1;
	}

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
	int is_txfifo_empty = FALSE;
	extern int chk_dpm_mode(void);
#endif // defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

#if	defined(__ATCMD_IF_UART1__)
	UART_WRITE(uart1, data, data_len);
#elif defined(__ATCMD_IF_UART2__)
	UART_WRITE(uart2, data, data_len);
#elif defined(__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
	int real_data_len = data_len;
	data_len = (((data_len - 1) / 4) + 1 ) * 4;

	host_response((unsigned int)(data), data_len, 0x83, data_len - real_data_len);
#endif	//

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
	if (chk_dpm_mode()) {
		do {
#if	defined(__ATCMD_IF_UART1__)
			UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#elif defined(__ATCMD_IF_UART2__)
			UART_IOCTL(uart2, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#endif
		} while (is_txfifo_empty == FALSE);
	}
#endif // defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_create_atcmd_mutex()
{
	atcmd_mutex = xSemaphoreCreateMutex();
	if (atcmd_mutex == NULL) {
		PRINTF("[%s] Faild to creatre atcmd_mutex\n", __func__);
		return -1;
	}
	return 0;
}

int atcmd_tr_mng_take_atcmd_mutex(unsigned int timeout)
{
	int ret = 0;

	if (atcmd_mutex) {
		ret = xSemaphoreTake(atcmd_mutex, timeout);
		if (ret != pdTRUE) {
			return -1;
		}
	}

	return 0;
}

int atcmd_tr_mng_give_atcmd_mutex()
{
	int ret = 0;

	if (atcmd_mutex) {
		ret = xSemaphoreGive(atcmd_mutex);
		if (ret != pdTRUE) {
			return -1;
		}
	}

	return 0;
}

#if defined (__ENABLE_TRANSFER_MNG__)
void PRINTF_ATCMD(const char *fmt, ...)
{
	extern atcmd_tr_mng_context atcmd_tr_mng_ctx;

	va_list ap;
	size_t data_len;
	size_t buflen = 0;

	TickType_t timeout = ATCMD_TR_MNG_DEF_TIMEOUT;
	int max_cnt = ATCMD_TR_MNG_DEF_RETRY_CNT;
	int retry_cnt = 0;

	unsigned int mutex_timeout = portMAX_DELAY;
	unsigned int direct_write = pdFALSE;

	ATCMD_TR_MNG_START;

	if (atcmd_dpm_wait_ready(1)) {
		return;
	}

	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
	}

	memset(&g_atcmd_tr_mng_print_buf, 0x00, sizeof(g_atcmd_tr_mng_print_buf));

	va_start(ap, fmt);
	data_len = (size_t)da16x_vsnprintf(g_atcmd_tr_mng_print_buf,
									   sizeof(g_atcmd_tr_mng_print_buf),
									   0, (const char *)fmt, ap);
	va_end(ap);

	if (atcmd_tr_mng_ctx.task_handler == NULL) {
		direct_write = pdTRUE;
		goto end;
	}

	if (atcmd_tr_mng_ctx.conf->max_retry_cnt) {
		max_cnt = atcmd_tr_mng_ctx.conf->max_retry_cnt;
	}

	if (atcmd_tr_mng_ctx.conf->wtimeout) {
		timeout = atcmd_tr_mng_ctx.conf->wtimeout;
	}

	while (retry_cnt++ < max_cnt) {
		buflen = atcmd_tr_mng_get_free(&atcmd_tr_mng_ctx);
		if (buflen > data_len) {
			break;
		}

		vTaskDelay(timeout);

		ATCMD_TR_MNG_INFO("#%d. Buffer is full(free:%ld, data:%ld). waitting for %d\n",
				retry_cnt, buflen, data_len, timeout);
	}

	if (buflen < data_len) {
		ATCMD_TR_MNG_ERR("Not enough buffer space(buf:%d,data:%d).\n", buflen, data_len);
		direct_write = pdTRUE;
		goto end;
	}

	buflen = atcmd_tr_mng_write_buf(&atcmd_tr_mng_ctx,
			(unsigned char *)g_atcmd_tr_mng_print_buf,
			data_len,
			mutex_timeout);
	if (buflen == 0) {
		ATCMD_TR_MNG_ERR("Failed to write data to buffer(%ld)\n", buflen);
		direct_write = pdTRUE;
		goto end;
	}

	if (atcmd_tr_mng_set_event(&atcmd_tr_mng_ctx, ATCMD_TR_MNG_EVT_DATA)) {
		ATCMD_TR_MNG_ERR("Failed to set event(0x%x)\n", ATCMD_TR_MNG_EVT_DATA);
	}

end:

	if (direct_write) {
		atcmd_tr_mng_puts_atcmd((unsigned char *)g_atcmd_tr_mng_print_buf, data_len);
	}

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}

	ATCMD_TR_MNG_END;

	return ;
}

void PRINTF_ESCCMD(UINT32 result_code)
{
	ATCMD_TR_MNG_START;

	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
	}

	host_response_esc(0xffffffff, 0, result_code + 0x20, 0);

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}

	ATCMD_TR_MNG_END;
}

void PUTS_ATCMD(char *data, int data_len)
{
	extern atcmd_tr_mng_context atcmd_tr_mng_ctx;

	size_t buflen = 0;

	TickType_t timeout = ATCMD_TR_MNG_DEF_TIMEOUT;
	int max_cnt = ATCMD_TR_MNG_DEF_RETRY_CNT;
	int retry_cnt = 0;

	unsigned int mutex_timeout = portMAX_DELAY;
	unsigned int direct_write = pdFALSE;

	ATCMD_TR_MNG_START;

	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
	}

	if (atcmd_tr_mng_ctx.task_handler == NULL) {
		direct_write = pdTRUE;
		goto end;
	}

	if (atcmd_tr_mng_ctx.conf->max_retry_cnt) {
		max_cnt = atcmd_tr_mng_ctx.conf->max_retry_cnt;
	}

	if (atcmd_tr_mng_ctx.conf->wtimeout) {
		timeout = atcmd_tr_mng_ctx.conf->wtimeout;
	}

	while (retry_cnt++ < max_cnt) {
		buflen = atcmd_tr_mng_get_free(&atcmd_tr_mng_ctx);
		if (buflen > (size_t)data_len) {
			break;
		}

		vTaskDelay(timeout);

		ATCMD_TR_MNG_INFO("#%d. Buffer is full(free:%ld, data:%ld). waitting for %d\n",
				retry_cnt, buflen, data_len, timeout);
	}

	if (buflen < (size_t)data_len) {
		ATCMD_TR_MNG_ERR("Not enough buffer space(buf:%d,data:%d)\n", buflen, data_len);
		direct_write = pdTRUE;
		goto end;
	}

	buflen = atcmd_tr_mng_write_buf(&atcmd_tr_mng_ctx,
			(unsigned char *)data,
			(size_t)data_len,
			mutex_timeout);
	if (buflen == 0) {
		ATCMD_TR_MNG_ERR("Failed to write data to buffer(%ld)\n", buflen);
		direct_write = pdTRUE;
		goto end;
	}

	if (atcmd_tr_mng_set_event(&atcmd_tr_mng_ctx, ATCMD_TR_MNG_EVT_DATA)) {
		ATCMD_TR_MNG_ERR("Failed to set event(0x%x)\n", ATCMD_TR_MNG_EVT_DATA);
	}

end:

	if (direct_write) {
		atcmd_tr_mng_puts_atcmd((unsigned char *)data, (size_t)data_len);
	}

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}

	ATCMD_TR_MNG_END;

	return ;
}

#elif defined (__SUPPORT_ATCMD__)
void PRINTF_ATCMD(const char *fmt, ...)
{
#if	defined(__ATCMD_IF_UART1__)	|| defined(__ATCMD_IF_UART2__) || (defined(__ATCMD_IF_SPI__)||defined(__ATCMD_IF_SDIO__))
	int len;
	va_list ap;
#else
	DA16X_UNUSED_ARG(fmt);
#endif

	char	*atcmd_print_buf = NULL;

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
	int is_txfifo_empty = FALSE;
	extern int chk_dpm_mode(void);
#endif // defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

	if (atcmd_dpm_wait_ready(1)) {
		return;
	}

	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] Failed to get atcmd_mutex\n", __func__);
	}

	atcmd_print_buf = pvPortMalloc(UART_PRINT_BUF_SIZE);
	if (atcmd_print_buf == NULL) {
		PRINTF("- [%s] Failed to allocate the print buffer\n", __func__);

		if (atcmd_tr_mng_give_atcmd_mutex()) {
			PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
		}
		return;
	}

	memset(atcmd_print_buf, 0x00, UART_PRINT_BUF_SIZE);

#if	defined(__ATCMD_IF_UART1__)	|| defined(__ATCMD_IF_UART2__)	|| (defined(__ATCMD_IF_SPI__)||defined(__ATCMD_IF_SDIO__))

	va_start(ap, fmt);
	len = da16x_vsnprintf(atcmd_print_buf, UART_PRINT_BUF_SIZE, 0, (const char *)fmt, ap);
	va_end(ap);

#endif

#if	defined(__ATCMD_IF_UART1__)
	UART_WRITE(uart1, atcmd_print_buf, len);
#elif defined(__ATCMD_IF_UART2__)
	UART_WRITE(uart2, atcmd_print_buf, len);
#elif defined(__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
    int real_data_len = len;
    len = (((len - 1) / 4) + 1 ) * 4;

    host_response((unsigned int)(atcmd_print_buf), len, 0x83, len - real_data_len);
#endif

#if defined (__ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__)
#if	defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
	if (chk_dpm_mode()) {
		do {
#if	defined(__ATCMD_IF_UART1__)
			UART_IOCTL(uart1, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#elif defined(__ATCMD_IF_UART2__)
			UART_IOCTL(uart2, UART_CHECK_TXEMPTY, &is_txfifo_empty);
#endif
		} while (is_txfifo_empty == FALSE);
	}
#endif // defined(__ATCMD_IF_UART1__) || defined(__ATCMD_IF_UART2__)
#endif // __ENABLE_TXFIFO_CHK_IN_LOW_BAUDRATE__

	vPortFree(atcmd_print_buf);

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}
}

void PRINTF_ESCCMD(UINT32 result_code)
{
	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
	}

	host_response_esc(0xffffffff, 0, result_code + 0x20, 0);

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}
}

void PUTS_ATCMD(char *data, int data_len)
{
	if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
		PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
	}

#if defined(__ATCMD_IF_UART1__)
	UART_WRITE(uart1, data, data_len);
#elif defined(__ATCMD_IF_UART2__)
	UART_WRITE(uart2, data, data_len);
#elif defined(__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
    int real_data_len = data_len;
    data_len = (((data_len - 1) / 4) + 1 ) * 4;

    host_response((unsigned int)(data), data_len, 0x83, data_len - real_data_len);
#else
	DA16X_UNUSED_ARG(data);
	DA16X_UNUSED_ARG(data_len);
#endif  //

	if (atcmd_tr_mng_give_atcmd_mutex()) {
		PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
	}
}

#else

void PRINTF_ATCMD(const char *fmt, ...)
{
	DA16X_UNUSED_ARG(fmt);
}

void PRINTF_ESCCMD(UINT32 result_code)
{
	DA16X_UNUSED_ARG(result_code);
}

void PUTS_ATCMD(char *data, int data_len)
{
	DA16X_UNUSED_ARG(data);
	DA16X_UNUSED_ARG(data_len);
}

#endif // __ENABLE_TRANSFER_MNG__

int atcmd_tr_mng_init_context(atcmd_tr_mng_context *ctx)
{
	ATCMD_TR_MNG_START;

	if (ctx == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	memset(ctx, 0x00, sizeof(atcmd_tr_mng_context));

	ctx->task_size = ATCMD_TR_MNG_DEF_TASK_SIZE;
	ctx->task_priority = ATCMD_TR_MNG_DEF_TASK_PRIORITY;

	ctx->buflen = ATCMD_TR_MNG_DEF_BUFFER_SIZE;
	ctx->ir_buflen = ATCMD_TR_MNG_DEF_IF_READ_BUFFER_SIZE;

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_deinit_context(atcmd_tr_mng_context *ctx)
{
	ATCMD_TR_MNG_START;

	if (lwrb_is_ready(&ctx->lwrb_buf)) {
		lwrb_free(&ctx->lwrb_buf);
	}

	if (ctx->ir_buf) {
		atcmd_tr_mng_free(ctx->ir_buf);
	}

	if (ctx->buf) {
		atcmd_tr_mng_free(ctx->buf);
	}

	if (ctx->buf_mutex) {
		vSemaphoreDelete(ctx->buf_mutex);
	}

	if (ctx->event) {
		vEventGroupDelete(ctx->event);
	}

	atcmd_tr_mng_init_context(ctx);

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_init_config(atcmd_tr_mng_config *conf)
{
	ATCMD_TR_MNG_START;

	if (conf == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	memset(conf, 0x00, sizeof(atcmd_tr_mng_config));

	strcpy(conf->task_name, ATCMD_TR_MNG_DEF_TASK_NAME);
	conf->buflen = ATCMD_TR_MNG_DEF_BUFFER_SIZE;
	conf->wtimeout = ATCMD_TR_MNG_DEF_TIMEOUT;
	conf->max_retry_cnt = ATCMD_TR_MNG_DEF_RETRY_CNT;

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_set_task_name(atcmd_tr_mng_config *conf, const char *name)
{
	ATCMD_TR_MNG_START;

	if (conf == NULL || name == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	if (strlen(name) >= ATCMD_TR_MNG_TASK_NAME_LEN) {
		ATCMD_TR_MNG_ERR("long name length(%d/%d)\n",
				strlen(name), ATCMD_TR_MNG_TASK_NAME_LEN);
		return -1;
	}

	strcpy(conf->task_name, name);

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_set_buflen(atcmd_tr_mng_config *conf, const size_t len)
{
	ATCMD_TR_MNG_START;

	if (conf == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	conf->buflen = len;

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_set_config(atcmd_tr_mng_context *ctx, atcmd_tr_mng_config *conf)
{
	int ret = 0;

	ATCMD_TR_MNG_START;

	if (ctx == NULL || conf == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	if ((strlen(conf->task_name) == 0) || (conf->buflen == 0) || (ctx->ir_buflen == 0)) {
		ATCMD_TR_MNG_ERR("Invalid configuration\n");
		goto err;
	}

	ctx->ir_buf = atcmd_tr_mng_calloc(ctx->ir_buflen, sizeof(unsigned char));
	if (ctx->ir_buf == NULL) {
		ATCMD_TR_MNG_ERR("Failed to allocate memory for IF buffer(%d)\n", ctx->ir_buflen);
		goto err;
	}

	ctx->buflen = conf->buflen;

	ctx->buf = atcmd_tr_mng_calloc(ctx->buflen, sizeof(unsigned char));
	if (ctx->buf == NULL) {
		ATCMD_TR_MNG_ERR("Failed to allocate memory for buffer(%d)\n", ctx->buflen);
		goto err;
	}

	ret = (int)lwrb_init(&ctx->lwrb_buf, (void *)ctx->buf, ctx->buflen);
	if (ret == 0) {
		ATCMD_TR_MNG_ERR("Failed to init ring buffer(%d)\n", ctx->buflen);
		goto err;
	}

	ctx->buf_mutex = xSemaphoreCreateMutex();
	if (ctx->buf_mutex == NULL) {
		ATCMD_TR_MNG_ERR("Failed to create mutex for buffer\n");
		goto err;
	}

	ctx->event = xEventGroupCreate();
	if (ctx->event == NULL) {
		ATCMD_TR_MNG_ERR("Failed to create event\n");
		goto err;
	}

	ctx->conf = conf;

	ATCMD_TR_MNG_INFO("Completed configuration\n");
	ATCMD_TR_MNG_INFO("%-20s : %s(%ld)\n", "Task name",
			conf->task_name, strlen(conf->task_name));
	ATCMD_TR_MNG_INFO("%-20s : %ld bytes\n", "Task size", ctx->task_size * 4);
	ATCMD_TR_MNG_INFO("%-20s : %ld\n", "Task priority", ctx->task_size * 4);
	ATCMD_TR_MNG_INFO("%-20s : %ld\n", "Ring buffer size", ctx->buflen);
	ATCMD_TR_MNG_INFO("%-20s : %ld\n", "IF buffer size", ctx->buflen);
	ATCMD_TR_MNG_INFO("%-20s : %ld\n", "Writing Timeout", conf->wtimeout);
	ATCMD_TR_MNG_INFO("%-20s : %ld\n", "Writing Retry cont", conf->max_retry_cnt);


	ATCMD_TR_MNG_END;

	return 0;

err:

	if (lwrb_is_ready(&ctx->lwrb_buf)) {
		lwrb_free(&ctx->lwrb_buf);
	}

	if (ctx->ir_buf) {
		atcmd_tr_mng_free(ctx->ir_buf);
	}

	if (ctx->buf) {
		atcmd_tr_mng_free(ctx->buf);
	}

	if (ctx->buf_mutex) {
		vSemaphoreDelete(ctx->buf_mutex);
	}

	if (ctx->event) {
		vEventGroupDelete(ctx->event);
	}

	atcmd_tr_mng_init_context(ctx);

	ATCMD_TR_MNG_END;

	return -1;
}

int atcmd_tr_mng_create_task(atcmd_tr_mng_context *ctx)
{
	int ret = 0;

	ATCMD_TR_MNG_START;

	if (ctx == NULL || ctx->conf == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	ret = xTaskCreate(atcmd_tr_mng_task_entry,
				(const char *)(ctx->conf->task_name),
				ctx->task_size,
				(void *)ctx,
				ctx->task_priority,
				&ctx->task_handler);
	if (ret != pdPASS) {
		ATCMD_TR_MNG_ERR("Failed to create task(%d)\n", ret);
		return -1;
	}

	ATCMD_TR_MNG_END;

	return 0;
}

int atcmd_tr_mng_delete_task(atcmd_tr_mng_context *ctx)
{
	int ret = 0;
	TickType_t timeout = 100;
	const int max_cnt = 10;
	int retry_cnt = 0;

	ATCMD_TR_MNG_START;

	if (ctx == NULL) {
		ATCMD_TR_MNG_ERR("Invalid parameter\n");
		return -1;
	}

	if (ctx->task_handler == NULL) {
		ATCMD_TR_MNG_INFO("Already stopped\n");
		return 0;
	}

	ret = atcmd_tr_mng_set_event(ctx, ATCMD_TR_MNG_EVT_STOP);
	if (ret) {
		ATCMD_TR_MNG_ERR("Failed to set event(0x%x)\n", ATCMD_TR_MNG_EVT_STOP);
	}

	do {
		vTaskDelay(timeout);
		if (ctx->task_handler == NULL) {
			break;
		}
	} while (++retry_cnt < max_cnt);

	if (retry_cnt >= max_cnt) {
		ATCMD_TR_MNG_ERR("Failed to stop task(%d/%d/%d)\n", retry_cnt, max_cnt, timeout);
		return -1;
	}

	ATCMD_TR_MNG_END;

	return ret;
}

int atcmd_tr_mng_set_event(atcmd_tr_mng_context *ctx, unsigned int event)
{
	ATCMD_TR_MNG_START;

	if (ctx == NULL || ctx->event == NULL) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return -1;
	}

	xEventGroupSetBits(ctx->event, event);

	ATCMD_TR_MNG_END;

	return 0;
}

unsigned int atcmd_tr_mng_get_event(atcmd_tr_mng_context *ctx, unsigned int timeout)
{
	EventBits_t events = 0x00;

	ATCMD_TR_MNG_START;

	if (ctx == NULL || ctx->event == NULL) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return -1;
	}

	events = xEventGroupWaitBits(ctx->event, ATCMD_TR_MNG_EVT_ANY, pdTRUE, pdFALSE, timeout);

	ATCMD_TR_MNG_END;

	return events;
}

size_t atcmd_tr_mng_read_buf(atcmd_tr_mng_context *ctx, unsigned char *buf, size_t buflen, unsigned int timeout)
{
	size_t ret = 0;

	ATCMD_TR_MNG_START;

	if (ctx == NULL || buf == NULL || buflen == 0
			|| !lwrb_is_ready(&ctx->lwrb_buf) || ctx->buf_mutex == NULL) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return 0;
	}

	xSemaphoreTake(ctx->buf_mutex, timeout);

	ret = lwrb_read(&ctx->lwrb_buf, buf, buflen);

	ATCMD_TR_MNG_INFO("Read data from ring buffer(buflen:%ld, read:%ld)\n", buflen, ret);

	xSemaphoreGive(ctx->buf_mutex);

	ATCMD_TR_MNG_END;

	return ret;
}

size_t atcmd_tr_mng_write_buf(atcmd_tr_mng_context *ctx, unsigned char *buf, size_t buflen, unsigned int timeout)
{
	size_t ret = 0;

	ATCMD_TR_MNG_START;

	if (ctx == NULL || buf == NULL || buflen == 0
			|| !lwrb_is_ready(&ctx->lwrb_buf) || ctx->buf_mutex == NULL) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return 0;
	}

	xSemaphoreTake(ctx->buf_mutex, timeout);

	ret = lwrb_write(&ctx->lwrb_buf, buf, buflen);

	ATCMD_TR_MNG_INFO("Written data to ring buffer(%ld/%ld)\n", buflen, ret);

	xSemaphoreGive(ctx->buf_mutex);

	ATCMD_TR_MNG_END;

	return ret;
}

size_t atcmd_tr_mng_get_free(atcmd_tr_mng_context *ctx)
{
	ATCMD_TR_MNG_START;

	if (ctx == NULL || !lwrb_is_ready(&ctx->lwrb_buf)) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return 0;
	}

	return lwrb_get_free(&ctx->lwrb_buf);
}

size_t atcmd_tr_mng_get_full(atcmd_tr_mng_context *ctx)
{
	ATCMD_TR_MNG_START;

	if (ctx == NULL || !lwrb_is_ready(&ctx->lwrb_buf)) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		return 0;
	}

	return lwrb_get_full(&ctx->lwrb_buf);
}

static void atcmd_tr_mng_task_entry(void *pvParameters)
{
	int sys_wdog_id = -1;
	atcmd_tr_mng_context *ctx = NULL;
	size_t remaining_len = 0;
	size_t read_len = 0;

	unsigned int event_timeout = ATCMD_TR_MNG_DEF_TIMEOUT;
	unsigned int events = 0x00;

	unsigned int mutex_timeout = portMAX_DELAY;

	ATCMD_TR_MNG_START;

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	if (pvParameters == NULL) {
		ATCMD_TR_MNG_ERR("Invaild parameter\n");
		goto end;
	}

	ctx = (atcmd_tr_mng_context *)pvParameters;

	ATCMD_TR_MNG_INFO("Context information\n");
	ATCMD_TR_MNG_INFO("%-20s : %d\n", "TCP_MSS", ATCMD_TR_MNG_TCP_MSS);
	ATCMD_TR_MNG_INFO("%-20s : 0x%x\n", "Event timeout", event_timeout);
	ATCMD_TR_MNG_INFO("%-20s : 0x%x\n", "Mutext timeout", mutex_timeout);

	while (pdTRUE) {
		da16x_sys_watchdog_notify(sys_wdog_id);

		events = atcmd_tr_mng_get_event(ctx, event_timeout);

		ATCMD_TR_MNG_INFO("Get event(0x%x)\n", events);

		//check buffer even if there is no event
		do {
			read_len = atcmd_tr_mng_read_buf(ctx, ctx->ir_buf, ctx->ir_buflen, mutex_timeout);
			if (read_len == 0) {
#if defined (__ENABLE_TRANSFER_MNG_TEST_MODE__)
				if (g_atcmd_tr_mng_test_flag) {
					g_atcmd_tr_mng_test_flag = pdFALSE;
				}
#endif // __ENABLE_TRANSFER_MNG_TEST_MODE__

				break;
			}

			da16x_sys_watchdog_suspend(sys_wdog_id);

			atcmd_tr_mng_puts_atcmd(ctx->ir_buf, read_len);

			da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

			remaining_len = atcmd_tr_mng_get_full(ctx);

			ATCMD_TR_MNG_INFO("Sent data(%ld), remaining data(%ld)\n",
					read_len, remaining_len);
		} while (remaining_len > 0);

		if (events & ATCMD_TR_MNG_EVT_DATA) {
			events &= ~(ATCMD_TR_MNG_EVT_DATA);
		}

		if (events & ATCMD_TR_MNG_EVT_STOP) {
			events &= ~(ATCMD_TR_MNG_EVT_STOP);
			break;
		}
	}

end:

	da16x_sys_watchdog_unregister(sys_wdog_id);

	ctx->task_handler = NULL;

	ATCMD_TR_MNG_END;

	vTaskDelete(NULL);

	return ;
}

/* EOF */
