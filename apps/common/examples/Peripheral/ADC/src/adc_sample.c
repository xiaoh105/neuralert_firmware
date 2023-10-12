/**
 ****************************************************************************************
 *
 * @file adc_sample.c
 *
 * @brief Sample app of ADC
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

#if defined (__ADC_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "da16x_dpm.h"
#include "task.h"

/*
 * Sample Functions for ADC
 */
#include "adc.h"

#undef ADC_SAMPLE_INTERRUPT
#define ADC_SAMPLE_READ
#undef ADC_SAMPLE_DPM

#define	DA16x_ADC_NO_TIMESTAMP		(0)
#define	DA16x_ADC_TIMEOUT_DMA		(2000)
#define DA16x_ADC_SEL_ADC_12		(12)
#define DA16x_ADC_DIVIDER_12		(4)
#define DA16x_ADC_NUM_READ			(16)

#define	GET_VALID_ADC_VALUE(x)	(((x)>>(4))&(0xfff))
#define	CONVERT_TO_THD_VALUE(x)	(((x)<<(4))&(0xffff))

#if defined(ADC_SAMPLE_INTERRUPT)
void adc_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    HANDLE hadc;
    int status, int_handling_mode;
    unsigned int data, type, thd;

    PRINTF("ADC_SAMPLE\n");
    DA16X_CLOCK_SCGATE->Off_DAPB_AuxA = 0;
    DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

    // Set PAD Mux. GPIO_0 (ADC_CH0), GPIO_1(ADC_CH1)
    _da16x_io_pinmux(PIN_AMUX, AMUX_AD12);

    // Create Handle
    hadc = DRV_ADC_CREATE(DA16200_ADC_DEVICE_ID);

    // Initialization
    status = DRV_ADC_INIT(hadc, DA16x_ADC_NO_TIMESTAMP);
    PRINTF("ADC-INIT: %d\n", status);


    // Start. Set Sampling Frequency. 12B ADC Set to 200KHz
    // Clock = 1MHZ / (value + 1)
    // Ex) If Value = 4, Clock = 1MHz / (4+1) = 200KHz
    status = DRV_ADC_START(hadc, DA16x_ADC_DIVIDER_12, 0);
    PRINTF("ADC start: %d\n", status);

    // Set ADC_0 to 12Bit ADC
    status = DRV_ADC_ENABLE_CHANNEL(hadc, DA16200_ADC_CH_0, DA16x_ADC_SEL_ADC_12, 0);
    PRINTF("ADC enable: %d\n", status);

    //Set Data type offset binary, 0 : 2's complement ,  1 : offset binary
    type = 1;
    DRV_ADC_IOCTL(hadc, ADC_SET_DATA_MODE, &type);

    // Read Current ADC_0 Value. Caution!! When read current adc value consequently, need delay at each read function bigger than Sampling Frequency
    DRV_ADC_READ(hadc, DA16200_ADC_CH_0, &data, 0);
    PRINTF("Current ADC Value = 0x%x\r\n", GET_VALID_ADC_VALUE(data));

    //set threshold
    //value 0x800(=2048, max=4095) means about 0.7Volt in EVK environment.
    thd = 0x800;
    //thd value must be shifted 4 bits right.
    status = DRV_ADC_SET_THD_VALUE(hadc, ADC_THRESHOLD_TYPE_12B_UNDER, CONVERT_TO_THD_VALUE(thd), 0);
    PRINTF("ADC-set threshold: %d\n", status);

    // set Interrupt
    status = DRV_ADC_SET_INTERRUPT(hadc, DA16200_ADC_CH_0, TRUE, ADC_INTERRUPT_THD_UNDER, 0);
    PRINTF("ADC-set interrupt: %d\n", status);


    //set Interrupt Handling Mode
    //ADC_INTERRUPT_MODE_EVENT mode : in interrupt handler, call set_event function.
    //ADC_INTERRUPT_MODE_MASK mode  : in interrupt handler, disable interrupt.
    int_handling_mode = ADC_INTERRUPT_MODE_EVENT | ADC_INTERRUPT_MODE_MASK;
    DRV_ADC_IOCTL(hadc, ADC_SET_INTERRUPT_MODE, (void *)(&int_handling_mode));


    //Wait Interrupt
    UNSIGNED interrupt_status ;
    DRV_ADC_WAIT_INTERRUPT(hadc, &interrupt_status);

    vTaskDelay(5);

    // disable Interrupt
    status = DRV_ADC_SET_INTERRUPT(hadc, DA16200_ADC_CH_0, FALSE, ADC_INTERRUPT_THD_UNDER, 0);
    PRINTF("ADC-reset interrupt: %d\n", status);


    // Read Current ADC_0 Value. Caution!! When read current adc value consequently, need delay at each read function bigger than Sampling Frequency
    DRV_ADC_READ(hadc, DA16200_ADC_CH_0, &data, 0);
    PRINTF("Interrupt Occured with Value = 0x%x\r\n", GET_VALID_ADC_VALUE(data));

    vTaskDelete(NULL);
}
#elif defined(ADC_SAMPLE_READ)
void adc_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    HANDLE hadc;
    unsigned short data0[DA16x_ADC_NUM_READ];
    unsigned short data1[DA16x_ADC_NUM_READ];
    unsigned int data;

    PRINTF("ADC_SAMPLE\n");
    DA16X_CLOCK_SCGATE->Off_DAPB_AuxA = 0;
    DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;


    // Set PAD Mux. GPIO_0 (ADC_CH0), GPIO_1(ADC_CH1)
    _da16x_io_pinmux(PIN_AMUX, AMUX_AD12);

    // Create Handle
    hadc = DRV_ADC_CREATE(DA16200_ADC_DEVICE_ID);

    // Initialization
    DRV_ADC_INIT(hadc, DA16x_ADC_NO_TIMESTAMP);

    // Start. Set Sampling Frequency. 12B ADC Set to 200KHz
    // Clock = 1MHZ / (value + 1)
    // Ex) If Value = 4, Clock = 1MHz / (4+1) = 200KHz
    DRV_ADC_START(hadc, DA16x_ADC_DIVIDER_12, 0);

    // Set ADC_0 to 12Bit ADC, ADC_1 to 12Bit ADC
    DRV_ADC_ENABLE_CHANNEL(hadc, DA16200_ADC_CH_0, DA16x_ADC_SEL_ADC_12, 0);
    DRV_ADC_ENABLE_CHANNEL(hadc, DA16200_ADC_CH_1, DA16x_ADC_SEL_ADC_12, 0);

    // Read 16ea ADC_0 Value. 12B ADC, Bit [15:4] is valid adc_data, [3:0] is zero
    DRV_ADC_READ_DMA(hadc, DA16200_ADC_CH_0, data0, DA16x_ADC_NUM_READ * 2,
                     DA16x_ADC_TIMEOUT_DMA, 0);

    // Read 16ea ADC_1 Value
    DRV_ADC_READ_DMA(hadc, DA16200_ADC_CH_1, data1, DA16x_ADC_NUM_READ * 2,
                     DA16x_ADC_TIMEOUT_DMA, 0);

    vTaskDelay(1);
    // Read Current ADC_0 Value. Caution!! When read current adc value consequently, need delay at each read function bigger than Sampling Frequency
    DRV_ADC_READ(hadc, DA16200_ADC_CH_0, &data, 0);

    // Close ADC
    DRV_ADC_CLOSE(hadc);
    // Print
    PRINTF("Current ADC_0 Value = %04d\r\n\r\n", GET_VALID_ADC_VALUE(data));
    PRINTF("ADC0  | ADC1\r\n\r\n");

    for (int i = 0; i < 16; i++) {
        PRINTF("%05d | %05d\r\n", GET_VALID_ADC_VALUE(data0[i]), GET_VALID_ADC_VALUE(data1[i]));
    }
    vTaskDelete(NULL);
}
#elif defined(ADC_SAMPLE_DPM)

#define	DA16200_ADC_NO_TIMESTAMP		(0)
#define	DA16200_ADC_TIMEOUT_DMA		(2000)
#define DA16200_ADC_SEL_ADC_12		(12)
#define DA16200_ADC_DIVIDER_12		(4)
#define DA16200_ADC_NUM_READ		(16)

void drv_adc_sensor_out_enable(HANDLE handler) {
	ADC_HANDLER_TYPE *adc;

	if (handler == NULL) {
		return;
	}
	adc = (ADC_HANDLER_TYPE*) handler;
	adc->regmap_rtc->ao_control |= 0x10;
}

void setup_adc_sample(void) {
	HANDLE hadc;
	int status, int_handling_mode;
	unsigned int data, type, val;
    unsigned long long  wakeup_time;
	UINT32 wakeup_src;

	DA16X_CLOCK_SCGATE->Off_DAPB_AuxA = 0;
	DA16X_CLOCK_SCGATE->Off_DAPB_APBS = 0;

	// Set PAD Mux. GPIO_0 (ADC_CH0), GPIO_1(ADC_CH1)
	_da16x_io_pinmux(PIN_AMUX, AMUX_AD12);

	// Create Handle
	hadc = DRV_ADC_CREATE(DA16200_ADC_DEVICE_ID);

	// Initialization
	status = DRV_ADC_INIT(hadc, DA16200_ADC_NO_TIMESTAMP);
	//PRINTF("ADC-INIT: %d\n",status);

	// Start. Set Sampling Frequency. 12B ADC Set to 200KHz
	// Clock = 1MHZ / (value + 1)
	// Ex) If Value = 4, Clock = 1MHz / (4+1) = 200KHz
	status = DRV_ADC_START(hadc, DA16200_ADC_DIVIDER_12, 0);
	//PRINTF("ADC start: %d\n",status);

	// Set ADC_0 to 12Bit ADC
	status = DRV_ADC_ENABLE_CHANNEL(hadc, DA16200_ADC_CH_0,
	DA16200_ADC_SEL_ADC_12, 0);
	//PRINTF("ADC enable: %d\n",status);

	//Set Data type offset binary, 0 : 2's complement ,  1 : offset binary
	type = 1;
	DRV_ADC_IOCTL(hadc, ADC_SET_DATA_MODE, &type);

	//Read Current ADC_0 Value.
	DRV_ADC_READ(hadc, DA16200_ADC_CH_0, &data, 0);
	PRINTF("Current ADC Value = 0x%x\r\n", data & 0xffff);

	//set threshold
	//value 0x8000 means about 0.7Volt in EVK environment.
	status = DRV_ADC_SET_THD_VALUE(hadc, ADC_THRESHOLD_TYPE_12B_OVER, 0x1B60,
			0);
	PRINTF("ADC-set threshold: %d\n", status);

	//set Interrupt
	status = DRV_ADC_SET_INTERRUPT(hadc, DA16200_ADC_CH_0, TRUE,
	ADC_INTERRUPT_THD_OVER, 0);
	PRINTF("ADC-set interrupt: %d\n", status);

	//set Interrupt Mode
	int_handling_mode = ADC_INTERRUPT_MODE_EVENT | ADC_INTERRUPT_MODE_MASK;
	DRV_ADC_IOCTL(hadc, ADC_SET_INTERRUPT_MODE, (void*) (&int_handling_mode));

	//Set ADC for Sleep 2 //
	DRV_ADC_SET_THRESHOLD(hadc, DA16200_ADC_CH_0, 0x1B6,
	ADC_RTC_THRESHOLD_TYPE_OVER);
	val = 1;
	DRV_ADC_IOCTL(hadc, ADC_SET_RTC_CYCLE_BEFORE_ON, &val);
	val = 1;
	DRV_ADC_IOCTL(hadc, ADC_SET_RTC_CYCLE_BEFORE_CAPTURE, &val);
	val = 0;
	DRV_ADC_IOCTL(hadc, ADC_SET_CAPTURE_STEP, &val);
	DRV_ADC_SET_SLEEP_MODE(hadc, 1, 0xf, 1);
	DRV_ADC_SET_RTC_ENABLE_CHANNEL(hadc, DA16200_ADC_CH_0, 1);

	drv_adc_sensor_out_enable(hadc);
	vTaskDelay(50);

	wakeup_src = da16x_boot_get_wakeupmode();
	PRINTF("\nWakeup source is 0x%x\n", wakeup_src);

    wakeup_time = 10000000000 * 1000000;
    start_dpm_sleep_mode_2(wakeup_time, TRUE);
    PRINTF("Sample: Go to sleep 2 ... \n");

}

void adc_sample(void *param) {
	DA16X_UNUSED_ARG(param);
	PRINTF("ADC sample start.\n");
	setup_adc_sample();
	vTaskDelete(NULL);
}
#endif

#endif // (__ADC_SAMPLE__)
