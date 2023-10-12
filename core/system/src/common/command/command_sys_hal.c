 /**
 ****************************************************************************************
 *
 * @file command_sys_hal.c
 *
 * @brief Command Parser
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

#include <stdio.h>
#include <string.h>

#include "oal.h"
#include "hal.h"
#include "da16x_system.h"
#include "command.h"
#include "command_sys.h"
#include "target.h"
#include "sys_cfg.h"
#include "da16x_sys_watchdog.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define SUPPORT_GPIO_TEST
#define SUPPORT_UART_TEST

#ifdef XIP_CACHE_BOOT
#undef  SUPPORT_ALL_HALCMD
#undef	SUPPORT_CMD_PERITEST
#else
#undef  SUPPORT_ALL_HALCMD
#undef	SUPPORT_CMD_PERITEST
#endif

extern unsigned int ctoi(char *s);
extern unsigned int htoi(char *s);

//-----------------------------------------------------------------------
// Command Function-List
//-----------------------------------------------------------------------
#ifdef	SUPPORT_ALL_HALCMD
static void cmd_usleep_cmd(int argc, char *argv[]);
static void	cmd_irqtest_cmd(int argc, char *argv[]);
#endif // SUPPORT_ALL_HALCMD
#ifdef	SUPPORT_CMD_PERITEST
static void cmd_peritest_cmd(int argc, char *argv[]);
#endif // SUPPORT_CMD_PERITEST

//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------

static HANDLE uart;
static CHAR *RX_buffer;
static UINT32 buf_size;

static const uart_dma_primitive_type uart_dma_primitives = {
		&SYS_DMA_COPY,
		&SYS_DMA_REGISTRY_CALLBACK,
		&SYS_DMA_RELEASE_CHANNEL
};

TaskHandle_t uart_driver_test_task_handler = NULL;
void uart_driver_test_task(void *pvParameters)
{
	int sys_wdog_id = -1;

	DA16X_UNUSED_ARG(pvParameters);

	UINT32 echo_buf;

	sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

	PRINTF("UART: echo thread run\n");
	while(1)
	{
		da16x_sys_watchdog_notify(sys_wdog_id);

		UART_READ(uart, &echo_buf, 1);
		UART_WRITE(uart, &echo_buf, 1);
		PRINTF("UART1: 0x%02x\n", (char)echo_buf);
		vTaskDelay(10);
	}

	da16x_sys_watchdog_unregister(sys_wdog_id);
}


INT32 _uart_dma_init(HANDLE handler)
{
	UINT32 dma_control;
	UINT32 fifo_level;
	UINT32 dev_unit;
	UINT32 rw_word;
	UINT32 use_word_access;
	int ret;
	HANDLE dma_channel_tx;
	HANDLE dma_channel_rx;

#define GET_RX_DMA_CHANNEL(x)	( \
								 (x == UART_UNIT_0) ? CH_DMA_PERI_UART0_RX : 	    \
								 (x == UART_UNIT_1) ? CH_DMA_PERI_UART1_RX :        \
								 (x == UART_UNIT_2) ? CH_DMA_PERI_UART2_RX : 10     \
								)

#define GET_TX_DMA_CHANNEL(x)	( \
								 (x == UART_UNIT_0) ? CH_DMA_PERI_UART0_TX : 		\
								 (x == UART_UNIT_1) ? CH_DMA_PERI_UART1_TX : 		\
								 (x == UART_UNIT_2) ? CH_DMA_PERI_UART2_TX : 10     \
								)


	UART_IOCTL(handler, UART_GET_FIFO_INT_LEVEL, &fifo_level);
	UART_IOCTL(handler, UART_GET_PORT, &dev_unit);
	UART_IOCTL(handler, UART_GET_RW_WORD, &rw_word);
	UART_IOCTL(handler, UART_GET_WORD_ACCESS, &use_word_access);

	/* RX */
	if (use_word_access)
	{
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_WORD);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
	} else {
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_NO_INCREMENT) | DMA_CHCTRL_DINCR(DMA_TRANSFER_INC_BYTE);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
	}

	if (((fifo_level)>>3) > 3)
	{
		PRINTF("Error: Not support DMA burst fifo length.\n");
		return -1;
	}

	if (use_word_access)
	{
		dma_control |= DMA_CHCTRL_BURSTLENGTH(fifo_level>>3);
	} else {
		dma_control |= DMA_CHCTRL_BURSTLENGTH((fifo_level>>3) + 2);
	}

	dma_channel_rx = SYS_DMA_OBTAIN_CHANNEL(GET_RX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
	if (dma_channel_rx == NULL)
	{
		PRINTF("Error: dma rx null.\n");
		return -1;
	}

	/* TX */
	if (use_word_access)
	{
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_WORD) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_WORD) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_WORD);
	} else {
		dma_control  = DMA_CHCTRL_SINCR(DMA_TRANSFER_INC_BYTE) | DMA_CHCTRL_DINCR(DMA_TRANSFER_NO_INCREMENT);
		dma_control |= DMA_CHCTRL_DWIDTH(DMA_TRANSFER_WIDTH_BYTE) | DMA_CHCTRL_SWIDTH (DMA_TRANSFER_WIDTH_BYTE);
	}

	dma_channel_tx = SYS_DMA_OBTAIN_CHANNEL(GET_TX_DMA_CHANNEL(dev_unit), dma_control, 0, 0);
	if (dma_channel_tx == NULL)
	{
		PRINTF("Error: dma tx null.\n");
		return -1;
	}

	ret = UART_DMA_INIT(handler, dma_channel_tx, dma_channel_rx, &uart_dma_primitives);
	if (ret == FALSE) {
		PRINTF("Error: uart dma init failed.\n");
		return -1;
	}

	return 0;
}

void cmd_uart_cmd(int argc, char *argv[]) {
	volatile INT32 ret;
	static UINT32 flag = 0;
	UINT32 	clock = 40000000 * 2;
	UINT32 baud = 115200, fifo_en = 1,
		   parity = 0, parity_en = 0,
		   hwflow = 0, swflow = 0,
		   word_access = 0, DMA = 0, rw_word = 0;
	UINT32 temp;
	UINT32 port = 0, mode = 0;

	DA16X_CLOCK_SCGATE->Off_CAPB_UART2 = 0;

	if (argc == 1) {
		PRINTF("usages : %s [port] [baudrate] [mode] [parity] [flow control] [word_access_rw_word or word_access_rw_byte]\n", argv[0]);
		return;
	}

	/*************************************
	 * parse arguments
	 *************************************/
	if (argc > 1) {
		port = ctoi(argv[1]);
		if ((port < UART_UNIT_1) || (port > UART_UNIT_2)) {
			PRINTF("Error: Invalid port number\n");
			return;
		}
	}
	if (argc > 2) {
		baud = ctoi(argv[2]);
	}
	if (argc > 3) {
		if (DRIVER_STRCMP(argv[3], "echo") == 0) {
			// echo mode
			mode = 1;
			fifo_en = 1;
		} else if (DRIVER_STRCMP(argv[3], "data") == 0) {
			/* data transfer mode */
			fifo_en = 1;
		} else if (DRIVER_STRCMP(argv[3], "dma") == 0) {
			fifo_en = 1;
			DMA = 1;
		}
	}
	if (argc > 4) {
		if(DRIVER_STRCMP(argv[4], "odd") == 0)
		{
			/* odd parity */
			parity_en = 1;
			parity = 0;
		}
		else if (DRIVER_STRCMP(argv[4], "even") == 0)
		{
			/* even parity */
			parity_en = 1;
			parity = 1;
		}
		else if (DRIVER_STRCMP(argv[4], "off") == 0)
		{
			/* even parity */
			parity_en = 0;
		}
	}
	if (argc > 5) {
		if(DRIVER_STRCMP(argv[5], "hwflow") == 0)
		{
			/* hw flow control */
			hwflow = 1;
			swflow = 0;
		}
		else if(DRIVER_STRCMP(argv[5], "swflow") == 0)
		{
			/* sw flow control */
			swflow = 1;
			hwflow = 0;
		}
		else if(DRIVER_STRCMP(argv[5], "off") == 0)
		{
			hwflow = 0;
			swflow = 0;
		}
	}
	if (argc > 6) {
		if(DRIVER_STRCMP(argv[6], "word_access_rw_word") == 0)
		{
			/* word access enable */
			word_access = 1;
			rw_word = 1;
		}
		else if(DRIVER_STRCMP(argv[6], "word_access_rw_byte") == 0)
		{
			/* word access enable */
			word_access = 1;
			rw_word = 0;
		}
		else if(DRIVER_STRCMP(argv[6], "off") == 0)
		{
			/* word access enable */
			word_access = 0;
			rw_word = 0;
		}
	}
	if (argc > 7) {
		PRINTF("usages : %s [port] [baudrate] [mode] [parity] [flow control] [word_access_rw_word or word_access_rw_byte]\n", argv[0]);
		return;
	}

	PRINTF("port: %d, baud: %d, mode: %d, parity: %d, parity: %d, hwflow: %d, swflow: %d, word: %d\n",
			port, baud, mode, parity, parity_en, hwflow, swflow, word_access);
	PRINTF("DMA: %d, word_access: %d, rw_word: %d\n", DMA, word_access, rw_word);


	/*************************************
	 * initialize rx thread and buffer
	 *************************************/
	if (mode == 1) {
		BaseType_t	xReturned;
		if (uart_driver_test_task_handler == NULL) {
			PRINTF(" [%s] UART_DRIVER_TEST!!!\n\r", __func__);
			xReturned = xTaskCreate(uart_driver_test_task,
									"uart_driver_test_task",
									512,
									(void *)NULL,
									OS_TASK_PRIORITY_USER,
									&uart_driver_test_task_handler);
			if (xReturned != pdPASS) {
				Printf(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, "uart_driver_test_task");
			}
		}
	}

	RX_buffer = APP_MALLOC(1024 * 4);
	if (RX_buffer == NULL)
	{
		PRINTF("Error: buffer allocation failed.");
		return;
	}
	memset(RX_buffer, 0x00, 1024*4);
	{
		int i;
		char asic[] = "\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}\r\n";
		for (i = 0; i <36; i++)
		{
			strcat((char *)RX_buffer,asic);
		}
		buf_size = strlen((const char*)RX_buffer);
	}


	/*************************************
	 * UART configuration
	 *************************************/
	if (uart == NULL)
		uart = UART_CREATE(port);

	UART_IOCTL(uart, UART_SET_CLOCK, &clock);
	UART_IOCTL(uart, UART_SET_BAUDRATE, &baud);

	temp = UART_WORD_LENGTH(8) | UART_FIFO_ENABLE(fifo_en) | UART_PARITY_ENABLE(parity_en) | UART_EVEN_PARITY(parity) /*parity*/ ;
	UART_IOCTL(uart, UART_SET_LINECTRL, &temp);

	temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(hwflow);
	UART_IOCTL(uart, UART_SET_CONTROL, &temp);

	temp = UART_RX_INT_LEVEL(UART_SEVENEIGHTHS) |UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);

	/*
	 * UART DMA 를 사용 할 경우 아래 설정 필요
	 */
	if (DMA)
		temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) |UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);

	UART_IOCTL(uart, UART_SET_FIFO_INT_LEVEL, &temp);

	temp = DMA;
	UART_IOCTL(uart, UART_SET_USE_DMA, &temp);

	if (fifo_en)
		temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT | UART_INTBIT_TIMEOUT |UART_INTBIT_ERROR | UART_INTBIT_FRAME
				| UART_INTBIT_PARITY | UART_INTBIT_BREAK | UART_INTBIT_OVERRUN ;
	else
		temp = UART_INTBIT_RECEIVE;

	UART_IOCTL(uart, UART_SET_INT, &temp);

	temp = swflow;
	UART_IOCTL(uart, UART_SET_SW_FLOW_CONTROL, &temp);

	temp = word_access;
	UART_IOCTL(uart, UART_SET_WORD_ACCESS, &temp);

	temp = rw_word;
	UART_IOCTL(uart, UART_SET_RW_WORD, &temp);

	if (flag)
	{
		PRINTF("UART%d changed \n", port);
		return;
	}

	/*
	 * UART 에서 DMA 사용하기 위해서는 아래의 함수가 반드시 호출 되어야 함.
	 */
	if (DMA) {
		if (_uart_dma_init(uart) < 0) {
			PRINTF("Error: UART%d DMA init failed! \n", port);
			return;
		}
	}

	ret = UART_INIT(uart);

	if (ret == 0) {
		PRINTF("Error: UART%d init failed.\n", port);
		return;
	}

	PRINTF("UART%d enalbed \n", port);
	return;
}

void 	cmd_uart_rx_cmd(int argc, char *argv[])
{
  	if (argc != 2)
	{
		PRINTF("input data size\n");
		return ;
    }

	buf_size = ctoi(argv[1]);
	memset(RX_buffer, 0x00, 1024 * 4);
	UART_READ(uart, RX_buffer, buf_size);

	PRINTF("\n");
	{
		PRINTF("%s", RX_buffer);
	}
}

void 	cmd_uart_tx_cmd(int argc, char *argv[])
{
	if (argc > 1)
	{
		UART_WRITE(uart, argv[1], strlen(argv[1]));
		return ;
	}

	if(RX_buffer == NULL)
	{
		PRINTF("no data\n");
		return;
  	}
  	UART_WRITE(uart, RX_buffer, buf_size);

  	PRINTF("complate\n");
}

void 	cmd_uart_rx_dma_cmd(int argc, char *argv[])
{
  	if (argc != 2)
	{
		PRINTF("input buffer size\n");
		return ;
	}
	buf_size = ctoi(argv[1]);

	if (RX_buffer == NULL)
	{
		PRINTF("buffer none\n");
		return;
	}

	UART_DMA_READ(uart, RX_buffer, buf_size);
	PRINTF("\n");
	{
		PRINTF("%s", RX_buffer);
	}
}

void 	cmd_uart_tx_dma_cmd(int argc, char *argv[])
{
	if (argc > 1)
	{
		UART_WRITE(uart, argv[1], strlen(argv[1]));
		return ;
	}

	if(RX_buffer == NULL)
	{
		PRINTF("no data\n");
		return;
  	}

  	UART_DMA_WRITE(uart, RX_buffer, buf_size);
  	PRINTF("complate\n");
}

extern void print_uart_debug();

void cmd_uart_debug_cmd(int argc, char *argv[]) {
	DA16X_UNUSED_ARG(argc);
	DA16X_UNUSED_ARG(argv);

	print_uart_debug();

}

void cmd_gpio_cmd_help(void) {
	PRINTF("gpioout [port] [pin] [data]\n");
	PRINTF("gpioin [port] [pin]\n");
	PRINTF("Example:\n");
	PRINTF("\tgpioout 1 40 40 (GPIOB[6] output high)\n");
	PRINTF("\tgpioin 0 10 (GPIOA[4] input)\n");
}


/******************************************************************************
 *  cmd_gpio_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void cmd_gpio_cmd(int argc, char *argv[])
{
	HANDLE	gpio;
	UINT32 port, num, dir ;
	UINT32 ioctldata;
	UINT16	data;


	if (argc != 4) {
		cmd_gpio_cmd_help();
		return;
	}

	port = ctoi(argv[1]);

	num  = (*argv[2] == 'a') ? 0xFFFF : htoi(argv[2]);

	data = ( argc == 4 ) ? (UINT16)htoi(argv[3]) : 0x00   ;

	if (DRIVER_STRCMP(argv[0], "gpioin") == 0) {
		dir = FALSE;	// in
	} else {
		dir = TRUE;	// out
	}

	if (port >= GPIO_UNIT_MAX) {
		PRINTF("Error: invalid port number\n");
		cmd_gpio_cmd_help();
		return;
	}

	gpio = GPIO_CREATE(port);

	if (gpio != NULL) {
		if (num & 0x0001) {
			da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
		}

		if (num & 0x0004) {
			da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
		}

		if (num & 0x0010) {
			da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
		}

		if (num & 0x0040) {
			da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
		}

		if (num & 0x0100) {
			da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
		}

		if (num & 0x0400) {
			da16x_io_pinmux(PIN_GMUX, GMUX_GPIO);
		}

		if (num & 0x1000) {
			da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
		}

		if (num & 0x2000) {
			da16x_io_pinmux(PIN_JMUX, JMUX_GPIO);
		}

		GPIO_INIT(gpio);
		ioctldata = 0x00;

		if (dir == TRUE) {
			// out
			ioctldata = num ;
			GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &ioctldata);

			GPIO_WRITE(gpio, num, &data, sizeof(UINT16));
			PRINTF("GPIO out - Pin %x : %x\n", num, data);

			GPIO_READ(gpio, num, &data, sizeof(UINT16));
			PRINTF("GPIO in - Pin %x : %x \n",  num,  data);

		} else {
			// in

			ioctldata = num ;
			GPIO_IOCTL(gpio, GPIO_SET_INPUT, &ioctldata);

			GPIO_READ(gpio, num, &data, sizeof(UINT16));
			PRINTF("GPIO in - Pin %x : %x \n", num,  data);
		}

		GPIO_CLOSE(gpio);
	}
}

static HANDLE hgpio = NULL;
static UINT16 switch_to_bitpos(int dig)
{
	UINT16 ret;
	ret = (UINT16)(0x01 << dig);
	return ret;
}

extern int dbg_test_print(char *fmt, ...);

static void gpio_callback(void *param)
{
	UINT32 data;
	data = (UINT32)param;

	dbg_test_print("[ISR] GPIO interrupt- Pin %x \r\n", data);
}

void cmd_gpio_interrupt_help(void) {
	PRINTF("gpioint [port] [pin] [active low/high] [level/edge]\n");
	PRINTF("Example:\n");
	PRINTF("\tgpioint 0 4 1 1  (GPIOA[4] active high, edge detect)\n");
}

void cmd_gpio_interrupt(int argc, char *argv[])
{
	UINT16 io, i, type=0, pol=0;
	UINT32 port;
	UINT32 ioctldata[3];

	if (argc != 5) {
		cmd_gpio_interrupt_help();
		return;
	}

	port = ctoi(argv[1]);

	if (htoi(argv[2]) != 0xffff) {
		io = switch_to_bitpos(atoi(argv[2]));
	} else {
		io = 0x8fff;
    }

	if (htoi(argv[3]) == 0 ) {
		// active low
		pol = 0;
	} else {
		// active high
		pol = 0xffff;
	}

	if (htoi(argv[4]) == 0 ) {
		// level detect
		type = 0;
	} else {
		// edge detect
		type = 0xffff;
	}

	if (port >= GPIO_UNIT_MAX) {
		PRINTF("Error: invalid port number\n");
		cmd_gpio_interrupt_help();
		return;
	}

	hgpio = GPIO_CREATE(port);
	GPIO_INIT(hgpio);

	GPIO_IOCTL(hgpio, GPIO_SET_INPUT, &io);

	ioctldata[0] = type;	// 1 edge, 0 level
	ioctldata[1] = pol;	// low 1 high, 0 low
	GPIO_IOCTL(hgpio, GPIO_SET_INTR_MODE, &ioctldata[0]);

	for (i = 0; i < 16; i++) {
		ioctldata[0] = (UINT32)(0x01 << i);
		ioctldata[1] = (UINT32)gpio_callback;
		ioctldata[2] = (UINT32)i;
		GPIO_IOCTL(hgpio, GPIO_SET_CALLACK, ioctldata);
	}

	ioctldata[0] = 0xffff;
	GPIO_IOCTL(hgpio, GPIO_SET_INTR_ENABLE, &io);
	PRINTF("gpio intertup enable \n");
}

/******************************************************************************
 *  cmd_emmc_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
static int get_width(int digi)
{
	int x = 10, width = 1;
	while ( (digi / x) != 0)
	{
		x *=10;
		width ++;
	}
	return width;
}

#define MAX_EMMC_BLOCK_COUNT	10 // 0x14		//128 0x80
#define EMMC_BLOCK_LEN			512
static EMMC_HANDLER_TYPE *emmc = 0;
void cmd_emmc_cmd(int argc, char *argv[])
{
	//static EMMC_HANDLER_TYPE *emmc = 0;
	volatile int err, card_insert = 0;
	static UINT8 *write_buf = NULL, *read_buf = NULL;
	UINT32 addr, block_len, loop=0, failcount = 0;
#ifndef BUILD_OPT_DA16200_ASIC
	UINT32 wtemp;
#endif	//BUILD_OPT_DA16200_ASIC
	unsigned long long wtb, wta, rtb, rta, itb, ita;
	UINT32 wtt =0 , rtt = 0;

	if(DRIVER_STRCMP(argv[1], "init") == 0 )
	{
		DA16X_CLOCK_SCGATE->Off_SysC_HIF = 0;
#ifdef BUILD_OPT_DA16200_ASIC
#define BUS_CLOCK	1
#define MMC_PORT1	1

#if BUS_CLOCK
		DA16X_SYSCLOCK->CLK_DIV_EMMC = 0x0b;
		DA16X_SYSCLOCK->CLK_EN_SDeMMC = 0x01;
#endif

#if MMC_PORT1
		_da16x_io_pinmux(PIN_EMUX, EMUX_SDm);
		_da16x_io_pinmux(PIN_CMUX, CMUX_SDm);
		_da16x_io_pinmux(PIN_DMUX, DMUX_SDm);
#endif
#else	//  BUILD_OPT_DA16200_ASIC
		wtemp = 50000000;
		_sys_clock_write( &wtemp, sizeof(UINT32));
		*((volatile unsigned char*)(0x500030a1)) = 0x09;
#endif

		if (write_buf == NULL)
		{
			int i;
			write_buf = pvPortMalloc(EMMC_BLOCK_LEN * MAX_EMMC_BLOCK_COUNT);
			if (write_buf == NULL)
				Printf("write buffer malloc fail\n");
#if 1
			for ( i = 0; i < EMMC_BLOCK_LEN*MAX_EMMC_BLOCK_COUNT; i++)
			{
				write_buf[i] = (UINT8)i & 0xff;
			}
#else
			memset((void*)write_buf, 0x00, EMMC_BLOCK_LEN*MAX_EMMC_BLOCK_COUNT);
#endif
		}
		if (read_buf == NULL)
		{
			read_buf = pvPortMalloc(EMMC_BLOCK_LEN *MAX_EMMC_BLOCK_COUNT);
			if (read_buf == NULL)
				Printf("read buffer malloc fail\n");
			memset((void*)read_buf, 0xcc,EMMC_BLOCK_LEN *MAX_EMMC_BLOCK_COUNT);
			/*wtemp = (UINT32)read_buf;
			wtemp = wtemp & 0xfffffff0;
			read_buf = (char*)wtemp;*/
		}
#if 0
		EMMC_SET_DRV_TYPE(EMMC_POLLING);
		//EMMC_SET_DELAY(1000);
		//EMMC_SET_DEBUG(1);
		//EMMC_SET_CLK_ALWAYS_ON(EMMC_CLK_RW_ON);
#endif
		if (emmc == NULL)
		{
			emmc = EMMC_CREATE();
			Printf("emmc create %08x \n", emmc);
		}
		itb = RTC_GET_COUNTER();
		err = EMMC_INIT(emmc);
		ita = RTC_GET_COUNTER();
		Printf("emmc_init %d\n", ita - itb);

		if (err)
		{
			Printf("emmc_init fail\n");
		}
		else
		{
#if 1
			card_insert = 1;
			if (emmc->card_type)
			{
				//PRINTF("inserted card is %s\n", card_insert == 1?"SDCARD":"MMC_CARD");
				PRINTF("%s is inserted\n", ((emmc->card_type == SD_CARD)?"SD card":(emmc->card_type == SD_CARD_1_1)?"SD_CARD 1.1":(emmc->card_type == SDIO_CARD)?"SDIO_CARD":"MMC card"));
			}

			if (emmc->card_type == SDIO_CARD && emmc->sdio_num_info > 0)
			{
				UINT32 i = 0;
				for (i = 0; i < emmc->sdio_num_info; i++)
					PRINTF("%s\n", emmc->psdio_info[i]);
			}
#endif
#ifdef BUILD_OPT_DA16200_ASIC
#if BUS_CLOCK
			DA16X_SYSCLOCK->CLK_DIV_EMMC = 0x03;
#endif
#else
			*((volatile unsigned char*) (0x500030a1)) = 0;
			MEM_LONG_WRITE(0x50030000,0x3d);
#endif
		}
	}
	else if(DRIVER_STRCMP(argv[1], "write") == 0 )
	{
		if (argc != 4)
		{
			PRINTF("usage: %s [init|read|write] [addr] [block_count]\n",argv[0]);
		}
		addr = htoi(argv[2]);
		block_len = htoi(argv[3]);
		{
			EMMC_WRITE(emmc,addr,write_buf,block_len);
		}
	}
	else if(DRIVER_STRCMP(argv[1], "read") == 0 )
	{
		memset((void*)read_buf,0xcc, 512*4);
		if (argc != 4)
		{
			PRINTF("usage: %s [init|read|write] [addr] [block_count]\n",argv[0]);
		}
		addr = htoi(argv[2]);
		block_len = htoi(argv[3]);
		{
			err = EMMC_READ(emmc,addr,read_buf,block_len);
			if (read_buf[0] != 0)
			{
				addr = addr + 1;
			}
		}
	}
	else if(DRIVER_STRCMP(argv[1], "verify") == 0)
	{
		if (argc != 5)
		{
			PRINTF("usage: %s [init|read|write]verify] [addr] [block_count] [loop]\n",argv[0]);
			return;
		}
		addr = htoi(argv[2]);
		block_len = htoi(argv[3]);
		loop = htoi(argv[4]);
		failcount = 0;
		//MEM_LONG_WRITE(0x5000100c,(0x00 << 20));
		{
			UINT32 i, j;
			int k;
			for(i = 0; i < loop; i++)
			{
				for (j = 0; j < block_len * EMMC_BLOCK_LEN; j++)
				{
					write_buf[j] = (UINT8)rand();
					//write_buf[j] = (char)j;
				}
				wtb = RTC_GET_COUNTER();
				err = EMMC_WRITE(emmc,addr,(write_buf),block_len);
				wta = RTC_GET_COUNTER();
				wtt += (UINT32)(wta - wtb);
				if (err)
				{
					PRINTF("WE %x", err);
					failcount++;
					continue;
				}
				rtb = RTC_GET_COUNTER();
				err = EMMC_READ(emmc,addr,(void*)read_buf,block_len);
				rta = RTC_GET_COUNTER();
				rtt += (UINT32)(rta - rtb);

				if (err)
				{
					failcount++;
					PRINTF("E");
				}

				if (memcmp(write_buf, read_buf, 512 * block_len) == 0)
				{
					PRINTF("%d",i);
					for (k = 0; k <get_width((int)i); k++)
						PRINTF("\b");
				}
				else
				{
					failcount++;
					PRINTF("X");
				}
			}
		}
		PRINTF("fail / total %d / %d\n", failcount,loop );
		PRINTF("write time ave %x read time ave %x \n", wtt/loop,rtt/loop);
	}
	else if (DRIVER_STRCMP(argv[1], "close") == 0)
	{
		EMMC_CLOSE(emmc);
		emmc = NULL;
		vPortFree(write_buf);
		vPortFree(read_buf);
		DA16X_SYSCLOCK->CLK_EN_SDeMMC = 0x00;
		DA16X_CLOCK_SCGATE->Off_SysC_HIF = 1;

	}
}

/******************************************************************************
 *  cmd_sdio_slave_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
#ifdef	SUPPORT_ALL_HALCMD
void cmd_sdio_slave_cmd(int argc, char *argv[])
{
	static char *sdio_slave_buf = NULL;
	/* call sdio_init function*/
	if(DRIVER_STRCMP(argv[1], "init") == 0 )
	{
		_da16x_io_pinmux(PIN_CMUX, CMUX_SDs);
		_da16x_io_pinmux(PIN_EMUX, EMUX_SDs);
		_da16x_io_pinmux(PIN_DMUX, DMUX_SDs);
		SDIO_SLAVE_INIT();
		sdio_slave_buf = pvPortMalloc(1024);
		PRINTF("sdio_slave init buf %x\n", sdio_slave_buf);
	}
	else if (DRIVER_STRCMP(argv[1], "close") == 0 )
	{
		SDIO_SLAVE_DEINIT();
		if (sdio_slave_buf != NULL)
			vPortFree(sdio_slave_buf);
		PRINTF("sdio_slave deinit\n");
	}
}
#endif //SUPPORT_ALL_HALCMD

#define CLK_GATING_OTP				0x50006048
#define MEM_BYTE_WRITE(addr, data)	*((volatile UINT8 *)(addr)) = (data)
#define MEM_BYTE_READ(addr, data)	*(data) = *((volatile UINT8 *)(addr))
#define MEM_LONG_READ(addr, data)		*data = *((volatile UINT *)(addr))
#define MEM_LONG_WRITE(addr, data)		*((volatile UINT *)(addr)) = data

#define OTP_IOCTL_CMD_WRITE	0x00
#define OTP_IOCTL_CMD_READ 	0x01
#define OTP_CMD_WRITE		0x07
#define OTP_CMD_READ		0x0B
#define _OTP_ADDR_BASE_		0x40122000
#define	FC9100_OTPROM_ADDRESS(x) 	(_OTP_ADDR_BASE_|(x<<2))


static void cmd_otp_cmd(int argc, char* argv[])
{
	UINT32 rval;

	DA16X_SYSCLOCK->PLL_CLK_EN_4_PHY = 1;
    DA16X_SYSCLOCK->CLK_EN_PHYBUS = 1;
	extern void	DA16X_SecureBoot_OTPLock(UINT32 mode);

	// reload otp lock status
	MEM_BYTE_WRITE(CLK_GATING_OTP, 0x00);
    DA16X_SecureBoot_OTPLock(1); // unlock
	otp_mem_create();
	otp_mem_lock_read((0x3ffc>>2), &rval);
	PRINTF("otp lock status 0x%08x\n", rval);
	otp_mem_close();

    if( DRIVER_STRCMP(argv[1], "write") == 0)
    {
		UINT32 offset, value;
		if (argc < 4)
		{
			PRINTF("otp write [offset] [value]\n");
			return ;
		}
		offset = htoi(argv[2]);
		value = htoi(argv[3]);
		if (offset < 0x2D ) { // secure area
			PRINTF("Under 0x2D offset is the secure area\n");
			return ;
		}
		PRINTF("offset %08x value %08x\n", offset, value);
		DA16X_SecureBoot_OTPLock(1); // unlock
		otp_mem_create();
		otp_mem_write(offset, value);
		otp_mem_close();
		DA16X_SecureBoot_OTPLock(0); // unlock
    }
    else if( DRIVER_STRCMP(argv[1], "read") == 0)
    {
		UINT32 offset, value, cnt = 1, i;
		if (argc < 3)
		{
			PRINTF("otp read [offset] [cnt]\n");
			return ;
		}
		offset = htoi(argv[2]);

		if (argc == 4)
			cnt = htoi(argv[3]);

		otp_mem_create();
		for (i = 0; i < cnt; i++)
		{
			otp_mem_read(offset + i, &value);
			PRINTF("0x%08x : 0x%08x\n", offset+(i), value);
		}
		otp_mem_close();

    }
    else if( DRIVER_STRCMP(argv[1], "lock") == 0)
    {
		UINT32 lock_pos, value;
		if (argc < 3)
		{
			PRINTF("otp lock [lock_position]\n");
			return ;
		}
		lock_pos = (UINT32)atoi(argv[2]);
		PRINTF("OTP lock 0x%x to 0x%x\n",(lock_pos*0x40), (lock_pos*0x40+0x40 - 1));

		otp_mem_create();
		otp_mem_lock_write(0xfff, (0x01<<lock_pos));
		otp_mem_lock_read(0xfff, &value);
		PRINTF("OTP lock register 0x%08x\n",(value));
		otp_mem_close();
    }
	//MEM_BYTE_WRITE(CLK_GATING_OTP, 0x01);
    DA16X_SecureBoot_OTPLock(0); // unlock
}


#if defined (__ENABLE_UNUSED__)
static HANDLE hTimer = 0;
static QueueHandle_t test_queue = 0;
static void timer_hisr(void *handler)
{
       unsigned long long send_value;
       UINT32 queue_temp;
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       // send queue
       send_value = RTC_GET_COUNTER();
       queue_temp = (UINT32)send_value;
#if 1
       xQueueSendFromISR(test_queue, &queue_temp ,&xHigherPriorityTaskWoken );
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#else
       PRINTF("ISR %08x\n", queue_temp);
#endif
}
#endif // __ENABLE_UNUSED__

#define TIMER_500MS        500*1000

#if defined (__ENABLE_UNUSED__)
static void cmd_switching_test(int argc, char* argv[])
{
       UINT32 ioctldata[3], sysclock, loop_counter = 1000, get_value, backup_value = 0, isr_time;
       // make queue
       test_queue = xQueueCreate(1, 4/*sizeof(UINT32)*/);

       // make timer
       hTimer = STIMER_CREATE(STIMER_UNIT_0);

       if (hTimer == NULL)
       {
             PRINTF("simple timer error \n");
             return ;
       }
       ioctldata[0] = 0 ;
       ioctldata[1] = (UINT32)&timer_hisr;
       ioctldata[2] = (UINT32)hTimer;
       STIMER_IOCTL(hTimer, STIMER_SET_CALLACK, ioctldata);

       STIMER_INIT(hTimer);

       _sys_clock_read(&sysclock, sizeof(UINT32));

       if (argc > 1)
             isr_time = atoi(argv[1]) * 1000;
       else
             isr_time = TIMER_500MS;

       PRINTF("reload time %d \n", isr_time);
       ioctldata[0] = sysclock;         // Clock
       ioctldata[1] = isr_time;         // reload value
       STIMER_IOCTL(hTimer , STIMER_SET_UTIME, ioctldata );

       ioctldata[0] = STIMER_DEV_INTCLK_MODE
                    | STIMER_DEV_INTEN_MODE
                    | STIMER_DEV_INTR_ENABLE ;

       STIMER_IOCTL(hTimer, STIMER_SET_MODE, ioctldata );
       STIMER_IOCTL(hTimer, STIMER_SET_ACTIVE, NULL);
       // while loop
       if (argc>2)
             loop_counter = htoi(argv[2]);
       PRINTF("loop counter %d\n");
#if 1
       do
       {
             xQueueReceive((test_queue),(VOID *)(&(get_value)), 1000);
             PRINTF("time diff %08x\n", (UINT32)RTC_GET_COUNTER() - get_value);

             if(--loop_counter == 0)
                    break;
       }while(1);

       STIMER_IOCTL(hTimer, STIMER_SET_DEACTIVE, NULL);
       STIMER_CLOSE(hTimer);
       vQueueDelete(test_queue);
#endif
       PRINTF("switching test done\n");
}
#endif // __ENABLE_UNUSED__

void cmd_pd_cmd(int argc, char *argv[])
{
    UINT32 mode, time, ioctldata;
	if(argc !=3 )
	{
		PRINTF("usage : %s [mode] [time:(second)]\n", argv[0]);
		return;
	}
	//RTC_CLEAR_EXT_SIGNAL();
	vTaskDelay(20);
	mode = htoi(argv[1]);
	time = (UINT32)(atoi(argv[2]));

	/* disable brown out black out */
	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &ioctldata);
	ioctldata &= (UINT32)(~(BROWN_OUT_INT_ENABLE(1) | BLACK_OUT_INT_ENABLE(1)));
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &ioctldata);

	RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &ioctldata);
	ioctldata &= (UINT32)(~(0x300));
	RTC_IOCTL(RTC_SET_BOR_CIRCUIT, &ioctldata);

	if (mode == 2)
	{
		RTC_CLEAR_RETENTION_FLAG();
		RTC_RETENTION_CLOSE();
		RTC_RETENTION_PWR_OFF();
		RTC_INTO_POWER_DOWN(0, time<<15);
	}
	else if (mode == 3)
	{
		RTC_SET_RETENTION_FLAG();
		da16x_abmsecure->c_abm_req_start = 2;
		RTC_INTO_POWER_DOWN(5, time<<15);
	}
	else
	{
		PRINTF("usage : %s [mode(2 or 3)] [time:(second)]\n", argv[0]);
		return;
	}
}

/* RF power down command */
extern void wifi_cs_rf_cntrl(int flag);
void cmd_rf_power_cmd(int argc, char *argv[])
{
	if(argc !=2 )
	{
		PRINTF("usage : %s [on/off]\n", argv[0]);
		return;
	}

	if(DRIVER_STRCMP(argv[1], "off") == 0) {
		/* RF OFF */
		wifi_cs_rf_cntrl(1);
	} else if(DRIVER_STRCMP(argv[1], "on") == 0) {
		/* RF ON */
		wifi_cs_rf_cntrl(0);
	} else {
		PRINTF("usage : %s [on/off]\n", argv[0]);
	}
}

//-----------------------------------------------------------------------
// Command SYS-List
//-----------------------------------------------------------------------

const COMMAND_TREE_TYPE	cmd_sys_hal_list[] = {
  { "HAL",			CMD_MY_NODE,	cmd_sys_hal_list,	NULL,	    "HAL command "} , // Head

  { "-------",		CMD_FUNC_NODE,	NULL,	NULL,					"--------------------------------"},
  { "sleep",		CMD_FUNC_NODE,	NULL,	&cmd_pd_cmd, 			"test power down"},
  { "rf_power",		CMD_FUNC_NODE,	NULL,	&cmd_rf_power_cmd, 		"RF power down"},
  { "otp",			CMD_FUNC_NODE,	NULL,	&cmd_otp_cmd, 	"otp"},

#ifdef	SUPPORT_ALL_HALCMD
  { "usleep",		CMD_FUNC_NODE,	NULL,	&cmd_usleep_cmd, 		"usleep"},
#if 0
  { "fdma",			CMD_FUNC_NODE,	NULL,	&cmd_fdma_cmd, 			"FDMA test"},
#endif
  { "irqtest",		CMD_FUNC_NODE,	NULL,	&cmd_irqtest_cmd, 		"IRQ Test"},
  { "emmc",			CMD_FUNC_NODE,	NULL,	&cmd_emmc_cmd, 			"emmc test command"},
#if 0
  { "rtimer",		CMD_FUNC_NODE,	NULL,	&cmd_rtimer_cmd, 		"rtc timer test"},
  { "sha",			CMD_FUNC_NODE,	NULL,	&cmd_sha_cmd, 			"sha test"},
#endif
#ifdef SUPPORT_UART_TEST
  { "uart",			CMD_FUNC_NODE,	NULL,	&cmd_uart_cmd, 			"uart1 & uart2 test command"},
  { "uart_rx",		CMD_FUNC_NODE,	NULL,	&cmd_uart_rx_cmd, 		"uart_rx test"},
  { "uart_tx",		CMD_FUNC_NODE,	NULL,	&cmd_uart_tx_cmd, 		"uart_tx test"},
  { "uart_rx_dma",	CMD_FUNC_NODE,	NULL,	&cmd_uart_rx_dma_cmd, 	"uart_rx test"},
  { "uart_tx_dma",	CMD_FUNC_NODE,	NULL,	&cmd_uart_tx_dma_cmd, 	"uart_tx test"},
#if 0
  { "uart_end",		CMD_FUNC_NODE,	NULL,	&cmd_uart_end_cmd, 		"uart_end test"},
  { "uart_err",		CMD_FUNC_NODE,	NULL,	&cmd_uart_err_cmd, 		"uart error cal"},
  { "uart_int",		CMD_FUNC_NODE,	NULL,	&cmd_uart_int_cmd, 		"uart interrupt report"},
  { "uart_flush",	CMD_FUNC_NODE,	NULL,	&cmd_uart_flush_cmd, 	"uart flush"},
  { "rs485",		CMD_FUNC_NODE,	NULL,	&cmd_rs485_config, 		"rs485 config"},
#endif
#endif
  { "sdio_slave",	CMD_FUNC_NODE,	NULL,	&cmd_sdio_slave_cmd, 	"sdio slave control"},
#if 0
  { "tkip",			CMD_FUNC_NODE,	NULL,	&cmd_michael_cmd, 		"tkip"},
  { "rtc",			CMD_FUNC_NODE,	NULL,	&cmd_rtc_cmd, 			"rtc function test"},
  { "timer",		CMD_FUNC_NODE,	NULL,	&cmd_timer_cmd, 		"ramlib timer test"},
  { "list",			CMD_FUNC_NODE,	NULL,	&cmd_list_cmd, 			"timer list"},
  { "des",			CMD_FUNC_NODE,	NULL,	&cmd_des_cmd, 			"3des test code"},
#endif
#ifdef SUPPORT_GPIO_TEST
  { "gpioin",		CMD_FUNC_NODE,	NULL,	&cmd_gpio_cmd, 			"gpio in test"},
  { "gpioout",		CMD_FUNC_NODE,	NULL,	&cmd_gpio_cmd, 			"gpio out test"},
  { "gpioint",		CMD_FUNC_NODE,	NULL,	&cmd_gpio_interrupt, 	"gpio interrupt"},
#if 0
  { "gpioret",		CMD_FUNC_NODE,	NULL,	&cmd_gpio_retention_cmd,"gpio retention test"},
  { "gpiopd",		CMD_FUNC_NODE,	NULL,	&cmd_gpio_pulldisable_cmd,"gpio pull disable test"},
  { "gpio",			CMD_FUNC_NODE,	NULL,	&cmd_gpio_test, 			"gpio test"},
  { "pad",			CMD_FUNC_NODE,	NULL,	&cmd_pad_input_enable_cmd,"pad input enable/disable test"},
  { "sleep_current_reduction",	CMD_FUNC_NODE,	NULL,	&cmd_sleep_current_reduction_cmd,"sleep current reduction"},
#endif
#endif
#if 0
  { "dcache",		CMD_FUNC_NODE,	NULL,	&cmd_dcache_cmd, 	"test dcache"},
#endif
  
#ifdef SUPPORT_SPI_LOOPBACK_TEST
  { "spi_init",		CMD_FUNC_NODE,	NULL,	&cmd_spi_master_init, 	"spi master initialize"},
  { "spi_close",	CMD_FUNC_NODE,	NULL,	&cmd_spi_master_close, 	"spi master close"},
#if 0
  { "plrd",			CMD_FUNC_NODE,	NULL,	&cmd_plrd_cmd, 	"spi 4byte read"},
  { "blrd",			CMD_FUNC_NODE,	NULL,	&cmd_blrd_cmd, 	"spi bulk read"},
  { "plwr",			CMD_FUNC_NODE,	NULL,	&cmd_plwr_cmd, 	"spi 4byte write"},
  { "blwr",			CMD_FUNC_NODE,	NULL,	&cmd_blwr_cmd, 	"spi bulk write"},
#endif
  { "sro",			CMD_FUNC_NODE,	NULL,	&cmd_spi_read_cmd, "spi read only"},
  { "spi_dverify",	CMD_FUNC_NODE,	NULL,	&cmd_spi_dverify_cmd, 	"spi_dverify"},
  { "spi_dverify_stm",	CMD_FUNC_NODE,	NULL,	&cmd_spi_dverify_stm_cmd, 	"spi_dverify_stm"},
  { "spi_trx_32",	CMD_FUNC_NODE,	NULL,	&cmd_spi_read_trx_32_cmd, 	"spi_trx_32"},
  #endif

#ifdef SUPPORT_GPIO_ALT_FUNC_TEST
  { "gpioalt_init",	CMD_FUNC_NODE,	NULL,	&cmd_gpioalt_init, 		"gpio alt init"},
  { "gpioalt_pin",	CMD_FUNC_NODE,	NULL,	&cmd_set_gpio_alt, 		"set all gpio alt pin"},
  { "gpioalt_pin2",	CMD_FUNC_NODE,	NULL,	&cmd_set_gpio_alt2, 	"set gpio alt pin"},
#endif

#endif	//SUPPORT_ALL_HALCMD
#ifdef	SUPPORT_CMD_PERITEST
  { "peritest",		CMD_FUNC_NODE,	NULL,	&cmd_peritest_cmd, 	"Peri Test option"},
#endif	//SUPPORT_CMD_PERITEST
#if 0
  { "alt_rtc",		CMD_FUNC_NODE,	NULL,	&cmd_alt_rtc_cmd, 	"alt_rtc"},
  { "alt_get",		CMD_FUNC_NODE,	NULL,	&cmd_alt_get_cmd, 	"alt_rtc"},
#endif
#if 0
  { "switching_test",CMD_FUNC_NODE,	NULL,	&cmd_switching_test, 	"switching test"},
#endif
  { NULL, 		CMD_NULL_NODE,	NULL,	NULL,		NULL }			// Tail
};

#ifdef	SUPPORT_ALL_HALCMD
/******************************************************************************
 *  cmd_usleep_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

void	cmd_usleep_cmd(int argc, char *argv[])
{
#if 1	/* Original */
	long timestamp ;
	UINT32	iter;

	if( argc < 2 ){
		USLEEP_IOCTL(USLEEP_HANDLER(), USLEEP_GET_INFO, NULL);
		return;
	}else
	if( argc == 3 && *argv[1] == 'r' ){
		UINT32	ioctldata;

		ioctldata = ctoi(argv[2]);
		USLEEP_IOCTL(USLEEP_HANDLER(), USLEEP_SET_RESOLUTION, &ioctldata);
		return;
	}else
	if( argc == 3 && *argv[1] == 'a' ){
		UINT32	ioctldata;

		ioctldata = ctoi(argv[2]);
		USLEEP_IOCTL(USLEEP_HANDLER(), USLEEP_SET_RUNMODE, &ioctldata);
		return;
	}else{
		if( argc == 3 ){
			iter = ctoi(argv[2]);
		}else{
			iter = 1;
		}
	}

	timestamp = xTaskGetTickCount();

	while( iter-- > 0 ){
		if( USLEEP_SUSPEND(USLEEP_HANDLER(), (ctoi(argv[1]))) != TRUE ){
			PRINTF("USleep-Err (%d)\n", xTaskGetTickCount());
		}
	}

	timestamp = xTaskGetTickCount() - timestamp;
	PRINTF("After-USleep (%d, %d)\n", timestamp , xTaskGetTickCount());
#else	/* for another test */
	long timestamp ;
	UINT32	period;
	UINT32	times;
	UINT32	count = 0;

	if( argc != 3 ) {
		Printf(" command: help) usleep [utime] [count] \n");


        Printf("    Usage : usleep [period] [count] \n");
        Printf("     option\n");
        Printf("\t period : u sec \n");
        Printf("\t count  : check count \n");
		Printf("      ex) usleep 1000000 10 <=== 1 sec, 10 times \n");
	}
	else {
		period = ctoi(argv[1]);
		times = ctoi(argv[2]);

		Printf(" usleep test (period:%d count:%d)\n", period, times);

		timestamp = xTaskGetTickCount();
		Printf(" >>> Pre-usleep  (tick:%d)\n", xTaskGetTickCount());
		while ( times-- > 0 ) {
#ifdef TEST_USLEEP_SUSPEND
			if ( USLEEP_SUSPEND(USLEEP_HANDLER(), period) != TRUE ) {
				//fail
			}
			else {
				count++;
			}
#else
			usleep(period);
			count++;
#endif
		}
		Printf(">>> Post-usleep (tick:%d)\n", xTaskGetTickCount());
		timestamp = xTaskGetTickCount() - timestamp;
		Printf(CYAN_COLOR "Result-USleep (gap tick:%d, count:%d)\n" CLEAR_COLOR, timestamp , count);
	}
#endif
}
#endif // SUPPORT_ALL_HALCMD

static char *testirqtitle;

void da16x_irqtest_isr(void)
{
	Printf(testirqtitle);
}

#ifdef	SUPPORT_ALL_HALCMD
static  void	da16x_irqtest_interrupt(void)
{
  	INTR_CNTXT_SAVE();
	INTR_CNTXT_CALL(da16x_irqtest_isr);
	INTR_CNTXT_RESTORE();
}

/******************************************************************************
 *  cmd_irqtest_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/
void	cmd_irqtest_cmd(int argc, char *argv[])
{
	UINT32 irqnum ;
	char   title[32];

	if( argc !=2 ){
		return;
	}

	irqnum = ctoi(argv[1]);

	DRIVER_SPRINTF(title, "<SIRQ-%d detected>\r\n", irqnum);
	testirqtitle = title;

	_sys_nvic_write(irqnum, (void *)da16x_irqtest_interrupt, 0x6);

	DA16X_BOOTCON->IRQ_Test_SW = irqnum ;

	_sys_nvic_write(irqnum, (void *)NULL, 0);
}
#endif // SUPPORT_ALL_HALCMD

/******************************************************************************
 *  cmd_peritest_cmd ( )
 *
 *  Purpose :
 *  Input   :
 *  Output  :
 *  Return  :
 ******************************************************************************/

#ifdef	SUPPORT_CMD_PERITEST
extern void	view_peritest_info(void);
void	cmd_peritest_cmd(int argc, char *argv[])
{

	if( (argc == 2) && (DRIVER_STRCMP(argv[1], "huk") == 0) ){
		volatile UINT64 i, j, k;

		k = 0;
		for(i = 0; 0xFFFFFFFFFFF0UL; i++){
			for(i = 0; 0xFFFFFFFFFFF0UL; i++){
				k = k + 1;
			}
		}

	}else
	if( (argc == 2) && (DRIVER_STRCMP(argv[1], "run") == 0) ){
		extern void    thread_peritest(ULONG thread_input);
		thread_peritest(TRUE);
	}else
	if( (argc == 4) && (DRIVER_STRCMP(argv[1], "set") == 0) ){
		UINT32 itemid, regidx;

		if( DRIVER_STRCMP(argv[2], "all") == 0 ){
			itemid = 0xFFFFFFFF;
			if( DRIVER_STRCMP(argv[3], "on") == 0 ){
				DA16X_BOOTCON->test_feature_peri[0] |= itemid;
				DA16X_BOOTCON->test_feature_peri[1] |= itemid;
			}else{
				DA16X_BOOTCON->test_feature_peri[0] &= ~itemid;
				DA16X_BOOTCON->test_feature_peri[1] &= ~itemid;
			}
		}else{
			itemid = ctoi(argv[2]);
			if( itemid < 32 ){
				regidx = 0;
				itemid = (1<<itemid);
			}else{
				regidx = 1;
				itemid = (1<<(itemid-32));
			}
			if( DRIVER_STRCMP(argv[3], "on") == 0 ){
				DA16X_BOOTCON->test_feature_peri[regidx] |= itemid;
			}else{
				DA16X_BOOTCON->test_feature_peri[regidx] &= ~itemid;
			}
		}

	}else
	if( (argc == 3) && (DRIVER_STRCMP(argv[1], "opt") == 0) ){
		UINT32 optvalue ;

		optvalue = htoi(argv[2]);
		DA16X_BOOTCON->test_feature_peri[2] =
			(DA16X_BOOTCON->test_feature_peri[2] & ~(0x0FF))
			| ( optvalue & 0x0FF );
	}else
	if( (argc == 3) && (DRIVER_STRCMP(argv[1], "thd") == 0) ){
		UINT32 maxthread ;

		maxthread = ctoi(argv[2]);
		DA16X_BOOTCON->test_feature_peri[2] =
			(DA16X_BOOTCON->test_feature_peri[2] & ~(0x07<<20))
			| ((maxthread & 0x07) << 20);
	}else
	if( (argc == 4) && (DRIVER_STRCMP(argv[1], "ate") == 0) ){
		UINT32 atecmd , options;

		atecmd = htoi(argv[2]);
		options = htoi(argv[3]);

		DA16X_BOOTCON->test_feature_peri[2] =
			(DA16X_BOOTCON->test_feature_peri[2] & ~(0x000FFFFF))
			| ((atecmd  &  0x0F) << 16)
			| ((options & 0x0FFFF) << 0)
			;
	}else
	if( (argc == 3) && (DRIVER_STRCMP(argv[1], "mode") == 0) ){

		switch(*argv[2]){
		// Stack Option : Variable
		case 'V': DA16X_BOOTCON->test_feature_peri[2] |= (1<<23); break;
		case 'C': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<23); break;
		// Monitor
		case 'M': DA16X_BOOTCON->test_feature_peri[2] |= (1<<24); break;
		case 'S': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<24); break;
		// Test Option : Detail
		case 'D': DA16X_BOOTCON->test_feature_peri[2] |= (1<<25); break;
		case 'B': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<25); break;
		// Test Option : LongRun
		case 'L': DA16X_BOOTCON->test_feature_peri[2] |= (1<<26); break;
		case 'O': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<26); break;
		// Clock Option : PLL
		case 'P': DA16X_BOOTCON->test_feature_peri[2] |= (1<<27); break;
		case 'F': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<27); break;
		// Test Option : Iteration
		case 'I': DA16X_BOOTCON->test_feature_peri[2] |= (1<<28); break;
		case 'N': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<28); break;
		// Log Option
		case 'G': DA16X_BOOTCON->test_feature_peri[2] |= (1<<29); break;
		case 'Q': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<29); break;
		// Test Option : Boot
		case 'T': DA16X_BOOTCON->test_feature_peri[2] |= (1<<30); break;
		case 'E': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<30); break;
		// Test Option : Run/Undo
		case 'R': DA16X_BOOTCON->test_feature_peri[2] |= (1<<31); break;
		case 'U': DA16X_BOOTCON->test_feature_peri[2] &= ~(1<<31); break;

		default:
			DA16X_BOOTCON->test_feature_peri[2] = htoi(argv[2]);
			break;
		}
	}
	else
	{
		view_peritest_info();
	}
}
#endif	//SUPPORT_CMD_PERITEST

/* EOF */
