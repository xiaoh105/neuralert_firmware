/**
 ****************************************************************************************
 *
 * @file spi_slave_sample.c
 *
 * @brief Sample code for SPI slave
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

#if defined (__PERIPHERAL_SAMPLE_SPI_SLAVE__)

#include <stdio.h>
#include <string.h>

#include "da16x_system.h"
#include "driver.h"
#include "spi_slave_sample.h"
#include "lwip/inet.h"

#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#define SRAM_PROTECT

#if defined (SRAM_PROTECT)
#define SRAM_EXCEPT_RANGE_START_0    (DA16X_SYS_SECURE_BASE | 0x0520)
#define SRAM_EXCEPT_RANGE_END_0      (DA16X_SYS_SECURE_BASE | 0x0524)

#define SRAM_EXCEPT_RANGE_START_1    (DA16X_SYS_SECURE_BASE | 0x0528)
#define SRAM_EXCEPT_RANGE_END_1      (DA16X_SYS_SECURE_BASE | 0x052c)

#define SRAM_EXCEPT_READ             (0x01 << 0)
#define SRAM_EXCEPT_WRITE            (0x01 << 1)
#define SRAM_EXCEPT_ENABLE           (0x01 << 2)
#define SRAM_EXCEPT_FUNC_0           (DA16X_SYS_SECURE_BASE | 0x0550)
#define SRAM_EXCEPT_FUNC_1           (DA16X_SYS_SECURE_BASE | 0x0554)

#define SRAM_EXCEPT_ENABLE_ADDR      (DA16X_SYS_SECURE_BASE | 0x0504)
#define MEM_LONG_WRITE(addr, data)   *((volatile UINT *)addr) = data

static void host_sram_protect(UINT32 *buffer, UINT32 buffer_length, UINT32 *control,
                              UINT32 control_length)
{
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_START_0, (UINT32)buffer);
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_END_0, (UINT32)buffer + buffer_length);

    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_START_1, (UINT32)control);
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_END_1, (UINT32)control + control_length);

    MEM_LONG_WRITE(SRAM_EXCEPT_FUNC_0,
                   SRAM_EXCEPT_READ | SRAM_EXCEPT_WRITE | SRAM_EXCEPT_ENABLE);
    MEM_LONG_WRITE(SRAM_EXCEPT_FUNC_1,
                   SRAM_EXCEPT_READ | SRAM_EXCEPT_WRITE | SRAM_EXCEPT_ENABLE);

    MEM_LONG_WRITE(SRAM_EXCEPT_ENABLE_ADDR, 0x03);
}
#endif    /* SRAM_PROTECT */

char sample_tx_buf[1024 * 8];

/* Global Variables */
static unsigned int *host_data_buffer = NULL;
static unsigned int *host_data_buffer_allocated = NULL;
static unsigned int host_data_len_to_write = 0;
static unsigned int state_slave_proc      = STATE_WAIT_G_WR_REQ;
static unsigned int state_slave_proc_next = STATE_WAIT_G_WR_RES;
static TaskHandle_t host_drv_thread_type;
static unsigned int host_drv_thread_stacksize = 512; //512 * sizeof(unsigned int)
static unsigned int hostif = HOSTIF_SPI;
static unsigned int slave_interrupt_acc = 0;
static EventGroupHandle_t slave_rx_int_evt;
static unsigned int slave_tx_flag = 0;
static unsigned int slave_int_mask = ( SLAVE_INT_STATUS_ATCMD | SLAVE_INT_STATUS_CMD | SLAVE_INT_STATUS_END);


void    (*_host_rx_callback)(unsigned char *buf, unsigned int len) = NULL;
void    sample_process_data_spi();

SemaphoreHandle_t   host_if_lock;


/* Function Definition */

static void host_spi_slave_init(void)
{

    DA16X_SLAVECTRL->SPI_Slave_CR = 0xFD0B; //8-byte mode
    DA16X_SLAVECTRL->Base_Addr = 0x00005008; // base 0x5000xxxx
    DA16X_DIOCFG->EXT_INTB_CTRL =  0xb;
}

void    host_response_sample_spi(unsigned int buf_addr, unsigned int len, unsigned int resp,
                                 unsigned int dummy)
{
    unsigned int mask_evt ;
    volatile st_host_response host_resp;

    host_resp.buf_address = (unsigned int)buf_addr;
    host_resp.host_length = len;
    host_resp.resp = resp;
    host_resp.dummy = dummy ;
    DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(buf_addr)) >>
                                 16; //Set Addr.High to Base Address Register
    //    memcpy((void *)DA16X_SLAVECTRL->Resp_Addr1, &host_resp, sizeof(st_host_response));
    DA16X_SLAVECTRL->Resp_Addr1 = host_resp.buf_address;
    DA16X_SLAVECTRL->Resp_Addr2 = *((UINT32 *)&host_resp.host_length);

    DA16X_DIOCFG->EXT_INTB_SET = 0x1; // Trigger Host Int (GPIO    )

    /*
        uxBits = xEventGroupWaitBits(
                xEventGroup,        : The event group being tested.
                BIT_0 | BIT_4,      : The bits within the event group to wait for.
                pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
                pdFALSE,            : Don't wait for both bits, either bit will do.
                xTicksToWait )
    */
    mask_evt = xEventGroupWaitBits(slave_rx_int_evt, slave_int_mask, pdTRUE, pdFALSE, 20);
    if (!(mask_evt & slave_int_mask) ) {
        PRINTF("[%s] Slave Int. Timeout to init #1\n", __func__);
    }

    if (mask_evt & SLAVE_INT_STATUS_END) {

    }

    /*
        uxBits = xEventGroupWaitBits(
                xEventGroup,        : The event group being tested.
                BIT_0 | BIT_4,      : The bits within the event group to wait for.
                pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
                pdFALSE,            : Don't wait for both bits, either bit will do.
                xTicksToWait )
    */
    mask_evt = xEventGroupWaitBits(slave_rx_int_evt, slave_int_mask, pdTRUE, pdFALSE, 20);
    if (!(mask_evt & slave_int_mask) ) {
        PRINTF("[%s] Slave Int. Timeout to init #2\n", __func__);
    }

    if (mask_evt & SLAVE_INT_STATUS_END) {

    }

    return;
}

int sample_register_write_callback_spi(void (* p_rx_callback_func)(unsigned char *buf, unsigned int len))
{
    _host_rx_callback = p_rx_callback_func;
    return 0;

}

int sample_set_buffer_spi(void *buffer)
{
    host_data_buffer_allocated = (unsigned int *)buffer;
    //    PRINTF("%s, 0x%06x\r\n",__func__,(unsigned int)buffer );
    return 0;

}

void sample_process_general_command_spi()
{
    unsigned int data;
    st_host_request *host_req;
    st_host_response host_resp;

    data = DA16X_SLAVECTRL->Cmd_Addr;;

    if (data == HOST_ILLEGAL_CMD) {
        PRINT_HOST("!!!!! Illegal Status!!!!\r\n");
        return;
    }

    host_req = (st_host_request *)(&data);
    DA16X_SLAVECTRL->Cmd_Addr = HOST_ILLEGAL_CMD; // Clear Command Register


    //        PRINT_HOST("SLAVE_INT_STATUS_CMD length = %d, cmd 0x%02x\r\n", host_req->host_write_length, host_req->host_cmd);
    switch (host_req->host_cmd) {
    case HOST_MEM_WRITE_REQ:
        host_data_buffer = host_data_buffer_allocated;
        state_slave_proc = STATE_WAIT_G_WR_RES;

        state_slave_proc_next = STATE_WAIT_G_WR_DATA;

        if (host_data_buffer != NULL) {
            host_data_len_to_write = htons(host_req->host_write_length);
            host_response_sample_spi((unsigned int)host_data_buffer,
                                     (unsigned int)host_req->host_write_length,
                                     host_req->host_cmd + 1,
                                     1);
        } else {
            host_response_sample_spi(0xffffffff,
                                     0,
                                     HOST_MEM_WRITE_RES,
                                     1);
        }

        break;

    case HOST_MEM_READ_REQ:
        xSemaphoreTake(host_if_lock, (TickType_t) portMAX_DELAY);

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = host_req->host_write_length;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.dummy = 0;
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >>
                                         16; //Set Addr.High to Base Address Register

        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.dummy = 0;
        }

        DA16X_SLAVECTRL->Resp_Addr1 = *((unsigned int *)(&host_resp));
        DA16X_SLAVECTRL->Resp_Addr2 = *(((unsigned int *)(&host_resp)) + 1);
        DA16X_DIOCFG->EXT_INTB_SET = 0x1;    // Trigger Host Int (GPIO    )

        state_slave_proc = STATE_WAIT_G_RD_RES;
        state_slave_proc_next = STATE_WAIT_G_RD_DATA;

        xSemaphoreGive(host_if_lock);
        break;

    }
}

void sample_process_data_spi()
{
    switch (state_slave_proc) {
    case STATE_WAIT_G_WR_RES:
        state_slave_proc = state_slave_proc_next;

        if (state_slave_proc_next == STATE_WAIT_G_WR_DATA) {
            // Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        }

        break;

    case STATE_WAIT_G_WR_DATA:
        if (_host_rx_callback != NULL) {
            _host_rx_callback((unsigned char *)host_data_buffer_allocated, host_data_len_to_write);
        }

        break;

    case STATE_WAIT_G_RD_RES:
        state_slave_proc = state_slave_proc_next;

        if (state_slave_proc_next == STATE_WAIT_G_RD_DATA) {
            /* Set Addr.High to Base Address Register */
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        }

        // PRINT_HOST("STATE_WAIT_G_RD_RES\r\n");
        break;

    case STATE_WAIT_G_RD_DATA:
#if 1 // for RW loop test
        if (host_data_buffer != NULL) {
            host_data_buffer = NULL;
            // PRINT_HOST("Free Allocated Buffer\r\n");
        }
#endif
        break;

    default:
        break;

    }
}

void sample_process_command_spi(void)
{
    ULONG    mask_evt ;

    //Wait SPI Slave Interrupt

    mask_evt = xEventGroupWaitBits(slave_rx_int_evt, slave_int_mask, pdTRUE, pdFALSE, 20);
    if (mask_evt & SLAVE_INT_STATUS_CMD ) {
        return;
    } else {
        Printf("process command time out\r\n");
    }

    // Clear slave_rx_int_evt  <= Driver Event Set
    xEventGroupClearBits(slave_rx_int_evt, mask_evt );

    //If Tx from Slave Stage , ignore interrupt
    if (slave_tx_flag) {
        return;
    }

    //Command Parser
    if (mask_evt & SLAVE_INT_STATUS_CMD) {
        sample_process_general_command_spi();
    }

    slave_interrupt_acc++;
}

void sample_interrupt_handler_spi(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;
    xResult = xEventGroupSetBitsFromISR(slave_rx_int_evt, (DA16X_SLAVECTRL->SPI_Intr_Status), &xHigherPriorityTaskWoken);

    if (xResult != pdFAIL ) {
        /*
         * If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         * switch should be requested.  The macro used is port specific and will
         * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         * the documentation page for the port being used.
         */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
    //    xEventGroupSetBits(adc->mutex, (ADC_DMA_HISR_DONE));

    FPGA_DBG_TIGGER(DA16X_SLAVECTRL->SPI_Intr_Status);
    DA16X_SLAVECTRL->SPI_Intr_Status |= ( SLAVE_INT_STATUS_ATCMD | SLAVE_INT_STATUS_CMD |
                                          SLAVE_INT_STATUS_END); //mask all
}

void sample_slave_interrupt_spi(void)
{
    INTR_CNTXT_SAVE();
    INTR_CNTXT_CALL(sample_interrupt_handler_spi);
    INTR_CNTXT_RESTORE();
}

void   run_spi_slave_sample(ULONG arg)
{
    DA16X_UNUSED_ARG(arg);

    //Clock Gating Off
    DA16X_CLOCK_SCGATE->Off_SSI_SPI = 0;

    //Initialize SPI Slave HW
    if (hostif == HOSTIF_SPI) {
        host_spi_slave_init();
    }

    vTaskDelay(10);
    _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);
    _da16x_io_pinmux(PIN_AMUX, AMUX_SPIs);
    _da16x_io_pinmux(PIN_BMUX, BMUX_SPIs);

    //Set GPIOC6 as Interrupt Pin
    static HANDLE gpio;
    gpio = GPIO_CREATE(GPIO_UNIT_C);
    GPIO_INIT(gpio);

    PRINT_HOST("gpio num: %x\n", GPIO_ALT_FUNC_GPIO6);
    GPIO_SET_ALT_FUNC(gpio, GPIO_ALT_FUNC_EXT_INTR,
                      (GPIO_ALT_GPIO_NUM_TYPE)(GPIO_ALT_FUNC_GPIO6));
    GPIO_CLOSE(gpio);

    host_data_buffer_allocated = (unsigned int *)
                                 sample_tx_buf; //(unsigned int*)DRIVER_MALLOC(4096);
    PRINTF("BUF Address = 0x%x\r\n", (unsigned int)host_data_buffer_allocated);

    //Interrupt Register
    _sys_nvic_write(SPI_Slave_IRQn, (void *)sample_slave_interrupt_spi, 0x7);

    //Enable Memory Protection except Buffer and Required Register
#ifdef SRAM_PROTECT
    host_sram_protect(host_data_buffer_allocated, 4096, (UINT32 *)0x50080200, 0x7c);
#endif

    //Create Semaphore for Lock SPI Protocol
    host_if_lock = xSemaphoreCreateMutex();
    if (host_if_lock == NULL) {
        PRINTF("host if lock semaphore NG!!!\n");
    }

    //Create event for Interrupt Service Routine
    slave_rx_int_evt = xEventGroupCreate();
    if (slave_rx_int_evt == NULL) {
        PRINTF("[%s] slave_rx_int_evt Event Flags Create Error!\n", __func__);
    }

    //Create SPI Slave Thread
    BaseType_t xResult;

    xResult = xTaskCreate( sample_thread_spi, "@Host_drv", host_drv_thread_stacksize, ( void * ) NULL, 30, &host_drv_thread_type );
    if (xResult != pdPASS) {
        PRINTF("[%s] Failed to created Host_drv create fail(%d) \r\n", __func__, xResult);
    }

    vTaskDelete(NULL);
}

void sample_thread_spi(void *param)
{
    DA16X_UNUSED_ARG(param);

    while ( 1 ) {
        sample_process_command_spi();
    }
}

#endif

/* EOF */
