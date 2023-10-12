/**
 ****************************************************************************************
 *
 * @file system_start.c
 *
 * @brief DA16200 Generic feature - User application entry point
 *
 * Copyright (c) 2016-2020 Dialog Semiconductor. All rights reserved.
 *
 * This software ("Software") is owned by Dialog Semiconductor.
 *
 * By using this Software you agree that Dialog Semiconductor retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Dialog Semiconductor is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Dialog Semiconductor products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * DIALOG SEMICONDUCTOR BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#include "sdk_type.h"

#include "user_main.h"
#include "common_def.h"
#include "da16x_crypto.h"
#include "net_bsd_sockets.h"
#include "system_start.h"
#include "user_dpm_api.h"
#include "sys_specific.h"
#include "da16x_time.h"

#ifdef __SET_BOR_CIRCUIT__
#include "ramsymbols.h"
#include "rtc.h"
#endif // __SET_BOR_CIRCUIT__

#if defined (CFG_USE_RETMEM_WITHOUT_DPM)
#include "user_retmem.h"
#endif

#if defined ( __BLE_COMBO_REF__ )
#if defined (__BLE_PERI_WIFI_SVC__) && !defined (__ENABLE_SAMPLE_APP__)
#include "sleep2_monitor.h"
#endif

#if defined (__CONNECTION_STATUS_LED__)
extern void LED_threadCreate(int flag);
#endif // (__CONNECTION_STATUS_LED__)

#ifdef __BLE_FEATURE_ENABLED__
extern void combo_init(void);
#endif
extern char	mqtt_conn_state(void);
#endif	//( __BLE_COMBO_REF__ )

#define CLK2US(clk)			((((unsigned long long )clk) * 15625ULL) >> 9ULL)
#define CLK2MS(clk)			((CLK2US(clk))/1000ULL)

extern void printf_with_run_time(char *string);
extern void wifi_cs_rf_cntrl(int flag);

/*
 * Config customer's console baud-rate.
 */
void config_customer_console_baudrate(void)
{
#define	BAUDRATE_1	9600
#define	BAUDRATE_2	19200
#define	BAUDRATE_3	38400
#define	BAUDRATE_4	57600
#define	BAUDRATE_5	115200
#define	BAUDRATE_6	230400
#define	BAUDRATE_7	380400
#define	BAUDRATE_8	460800
#define	BAUDRATE_9	921600

	/* 9600 bps ~ 921600 bps */
	customer_console_baudrate = UART0_BAUDRATE;
}

#if defined ( __SET_WAKEUP_HW_RESOURCE__)
#if defined(__TCP_CLIENT_SLEEP2_SAMPLE__)
extern void user_rtc_wakeup_key_event_triggered();
#endif

extern UCHAR	runtime_cal_flag;		// system flag that enables runtime timing analysis

extern long long user_accelerometer_interrupt_time; // Save the time of the interrupt for use in the timestamp calculation


/*****************************************************************************/
/*	The extenal_wakeup_pin can be used for interrupt pin.					 */
/*	DA16200 6*6 have two external_wakeup_pin And							 */
/*  DA16200 8*8 have four external_wakeup_pin.								 */
/*****************************************************************************/
void rtc_ext_cb(void *data)
{
	unsigned long long time_old;
	unsigned long time_old_msec;
	unsigned char mystr[100];

	UINT32 ioctl = 0;

	time_old = RTC_GET_COUNTER();
	time_old_msec = (ULONG)CLK2MS(time_old);

	/* Waits until the RTC register is updated*/
	while (RTC_GET_COUNTER() < (time_old + 10))
		;

	RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &ioctl);

	if (ioctl & RTC_WAKEUP_STATUS) {
#if 1
		sprintf(mystr, " RTC1 wakeup interrupt %lu", time_old_msec);
		//printf_with_run_time(mystr);
		Printf(mystr);
		Printf("\n>>> rtc1 wakeup interrupt ...\r\n");
#endif

#if defined(__TCP_CLIENT_SLEEP2_SAMPLE__)
		// Save the time of the interrupt for use in the timestamp calculation
		user_accelerometer_interrupt_time = time_old;  // RTC clock ticks

		// Call the user's RTC wakeup event (we are in an interrupt)
		user_rtc_wakeup_key_event_triggered();
#endif
#if defined ( __SUPPORT_NOTIFY_RTC_WAKEUP__ )
		rtc_wakeup_intr_chk_flag = pdTRUE;
#endif	// __SUPPORT_NOTIFY_RTC_WAKEUP__
	}

	if (ioctl & RTC_WAKEUP2_STATUS) {
		Printf(">>> rtc2 wakeup interrupt ...\r\n");
#if defined ( __SUPPORT_NOTIFY_RTC_WAKEUP__ )
		rtc_wakeup_intr_chk_flag = pdTRUE;
#endif	// __SUPPORT_NOTIFY_RTC_WAKEUP__
	}

	if (ioctl & RTC_WAKEUP3_STATUS) {
		Printf(">>> rtc3 wakeup interrupt ...\r\n");
	}

	if (ioctl & RTC_WAKEUP4_STATUS) {
		Printf(">>> rtc4 wakeup interrupt ...\r\n");
	}

	/* clear wakeup source */
	time_old = RTC_GET_COUNTER();
	RTC_CLEAR_EXT_SIGNAL();

	while (RTC_GET_COUNTER() < (time_old + 1))
		;

	time_old = RTC_GET_COUNTER();
	ioctl = 0;
	RTC_IOCTL(RTC_SET_WAKEUP_SOURCE_REG, &ioctl);

	while (RTC_GET_COUNTER() < (time_old + 1))
		;
}

static void rtc_ext_intr(void)
{
	INTR_CNTXT_CALL(rtc_ext_cb);
}

void config_ext_wakeup_resource(void)
{
#if defined ( __BLE_COMBO_REF__ )
#if defined (__ENABLE_RTC_WAKE_UP1_INT__) || defined (__ENABLE_RTC_WAKE_UP2_INT__)
	UINT32 intr_src;

#if defined (__ENABLE_RTC_WAKE_UP1_INT__)
#if defined (__BLE_COMBO_REF__)
	#error "Do not use __ENABLE_RTC_WAKE_UP1_INT__ at DA16600"
#endif
	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &intr_src);
	intr_src |= WAKEUP_INTERRUPT_ENABLE(1) | WAKEUP_POLARITY(1);
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &intr_src);
#else
	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &intr_src);
    intr_src &= ~(WAKEUP_INTERRUPT_ENABLE(1));
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &intr_src);
#endif

#if defined (__ENABLE_RTC_WAKE_UP2_INT__)
	// wakeup pin2 (RTC_WAKE_UP2)
	RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &intr_src);
	intr_src |= RTC_WAKEUP2_EN(1);
	RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &intr_src);
	
	RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONFIG_REG, &intr_src);
	intr_src |= RTC_WAKEUP2_SEL(1); // active low
	// intr_src &= ~RTC_WAKEUP2_SEL(1); // active high
	RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONFIG_REG, &intr_src);
#endif
	_sys_nvic_write(RTC_ExtWkInt_IRQn, (void *)rtc_ext_intr, 0x7);
#endif
#else	//( __BLE_COMBO_REF__ )
	UINT32 intr_src;

//	PRINTF("====>>>Configuring external wakeup but not enabling\n");
	RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &intr_src);
	// Following line changed for Neuralert stage4 to delay
	// enabling the RTC interrupt from the accelerometer
//	intr_src |= WAKEUP_INTERRUPT_ENABLE(0) | WAKEUP_POLARITY(0);  // NJ
// Corrected version:
	intr_src &= ~(WAKEUP_INTERRUPT_ENABLE(1));  // Clears the Wakeup interrupt enable bit
	intr_src &= ~(WAKEUP_POLARITY(1));          // Set polarity to rising edge
	//	intr_src |= WAKEUP_INTERRUPT_ENABLE(1) | WAKEUP_POLARITY(1);
	RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &intr_src);

	_sys_nvic_write(RTC_ExtWkInt_IRQn, (void *)rtc_ext_intr, 0x7);
#endif	// ( __BLE_COMBO_REF__ )
}
#endif	// __SET_WAKEUP_HW_RESOURCE__

#if defined ( __SET_BOR_CIRCUIT__ )
RAMLIB_SYMBOL_TYPE *ramsymbols = (RAMLIB_SYMBOL_TYPE *) DA16X_RAMSYM_BASE;
extern void da16x_environ_lock(UINT32 flag);

#define MEM_LONG_WRITE(addr, data)  *((volatile UINT *) addr) = data
#define	MEM_LONG_OR_WRITE32(addr, data) *((volatile UINT *) addr) = *((volatile UINT *) addr) | data
#define	MEM_LONG_AND_WRITE32(addr, data) *((volatile UINT *) addr) = *((volatile UINT *) addr) & data
extern unsigned char br_ctrl;
/* it never return */
ATTRIBUTE_RAM_FUNC void flash_pin_stuck(void)
{
	int cnt = 0;
	HANDLE wdog;
	vPortEnterCritical();
	// Disable WDOG
	wdog = WDOG_CREATE(WDOG_UNIT_0) ;
	WDOG_IOCTL(wdog , WDOG_SET_DISABLE, NULL );

    // XFC off
    MEM_LONG_WRITE(0x500B0000, 0x00);
    // QSPI CS to high
    MEM_LONG_WRITE(0x500B012C, 0x000FFFFF);
    // QSPI clk to low
    MEM_LONG_WRITE(0x500B0128, 0x00000003);
    // QSPI data to low
    MEM_LONG_WRITE(0x500B0124, 0x0000FFFF);

    MEM_LONG_AND_WRITE32(0x5009104c, ~(0xf0));
    MEM_LONG_OR_WRITE32(0x5009104c, 0x70);


    // infinity loop
    while(1)
    {
        if ( (*((volatile UINT * )0x5009104c) & 0x00004000 ) == 0)
            cnt++;
        else
            cnt = 0;

        if (cnt > 0x200000)    // POR 0x1000000  ~7s @120Mhz
        {
        	MEM_LONG_AND_WRITE32(0x5009104c, ~(0xf0));
        	MEM_LONG_OR_WRITE32(0x5009104c, br_ctrl<<4);
            MEM_LONG_WRITE(0x5009101C, 3);
			while(1);
        }
    }
    vPortExitCritical();
}


void rtc_brown_cb(void)
{
	Printf("BR\r\n" );
	// It can make the next bootup as POR
	RTC_CLEAR_RETENTION_FLAG();
	da16x_environ_lock(TRUE);

    // stuck the flash pins
	vTaskSuspendAll();
	flash_pin_stuck();
}

static void config_bor_circuit(void)
{
    unsigned long bor_circuit = 0;

    RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &bor_circuit);
    // BR CTRL
    bor_circuit = (bor_circuit & 0xffffff0f) | (br_ctrl << 4);

    RTC_IOCTL(RTC_SET_BOR_CIRCUIT, &bor_circuit);

    RTC_ENABLE_BROWN_BLACK(rtc_brown_cb, NULL);
    RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &bor_circuit);

    if (bor_circuit & 0x00002000) {
        // brown out call brown out cb
        Printf(">>> brown out \r\n" );
        rtc_brown_cb();
    }
}
#endif // __SET_BOR_CIRCUIT__
#if defined ( __BLE_COMBO_REF__ ) && defined (__WAKE_UP_BY_BLE__)
void enable_wakeup_by_ble(void)
{
	_da16x_io_pinmux(PIN_CMUX, CMUX_GPIO); // RTS: GPIO[4], CTS: GPIO[5]
	DA16X_DIOCFG->GPIOA_PE_PS |= (0x20);//Pull Enable
	DA16X_DIOCFG->GPIOA_PE_PS |= (0x20 << 16); //Pull Up Enable
	_GPIO_RETAIN_HIGH(GPIO_UNIT_A, GPIO_PIN4);

    PRINTF("UART-CTS: pull-down retained in sleep ... \n");
}
#endif // ( __BLE_COMBO_REF__ ) && (__WAKE_UP_BY_BLE__)
void config_user_wu_hw_resource(void)
{
#if defined ( __SET_WAKEUP_HW_RESOURCE__ )
	/* Config HW Wakeup resources */
		config_ext_wakeup_resource();

#if defined ( __BLE_COMBO_REF__ ) && defined (__WAKE_UP_BY_BLE__)
	if (ble_combo_ref_flag == pdTRUE) {
		UINT32 ioctldata;
		RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &ioctldata);
		ioctldata &= ~(0x03ff0000);
		ioctldata |= GPIOA5_OR_GPIOC1(1);
		RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &ioctldata);

		RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONFIG_REG, &ioctldata);
		ioctldata |= GPIOA5_OR_GPIOC1_EDGE_SEL(1); // active low
		RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONFIG_REG, &ioctldata);

		PRINTF("gpio wakeup enable %08x\n", ioctldata);
	}
#endif // ( __BLE_COMBO_REF__ ) && (__WAKE_UP_BY_BLE__)
#endif	// __SET_WAKEUP_HW_RESOURCE__

#if defined ( __SET_BOR_CIRCUIT__ )
	config_bor_circuit();
#endif  // __SET_BOR_CIRCUIT__
}

/*
 * Format of dpm abnormal wakeup interval
 *
 * DPM_MON_RETRY_CNT	: ~/application/inc/common_def.h
 *
 * 	unsigned long long dpm_abnorm_wakeup_interval[DPM_MON_RETRY_CNT];
 * 	{
 * 		-1,			// Initial value : -1
 * 		10, 10, 10, 10, 10,
 * 		60,
 * 		3600,
 * 		3600,
 * 		3600 * 24
 * 	}
 */
unsigned long long _user_defined_wakeup_interval[DPM_MON_RETRY_CNT] =
{
	-1,			// Initial value : -1
	60,			// 1st Wakeup
	60,			// 2nd Wakeup	: 0xdeadbeaf is no wakeup
	60,			// 3rd Wakeup	: 0xdeadbeaf is no wakeup
	60 * 30,	// 4th Wakeup	: 0xdeadbeaf is no wakeup
	60 * 30,	// 5th Wakeup	: 0xdeadbeaf is no wakeup
	60 * 30,	// 6th Wakeup	: 0xdeadbeaf is no wakeup
	60 * 60,	// 7th Wakeup	: 0xdeadbeaf is no wakeup
	60 * 60,	// 8th Wakeup	: 0xdeadbeaf is no wakeup
	0xDEADBEAF	// 9th Wakeup 	: 0xdeadbeaf is no wakeup
};

static void set_dpm_abnorm_user_wakeup_interval(void)
{
#if defined ( __USER_DPM_ABNORM_WU_INTERVAL__ )
	extern unsigned long long *dpm_abnorm_user_wakeup_interval;

	dpm_abnorm_user_wakeup_interval = (unsigned long long *)_user_defined_wakeup_interval;
#endif	// __USER_DPM_ABNORM_WU_INTERVAL__
}

/* Customer defined operation if needed ... */
int chk_factory_rst_button_press_status(void)
{
#if defined ( __SUPPORT_FACTORY_RESET_BTN__ )

	if (check_factory_button(factory_rst_btn, factory_rst_led,
							 factory_rst_btn_chk_time) == 1)
	{
		factory_reset_default(1);
		return FALSE;
	}

#endif // __SUPPORT_FACTORY_RESET_BTN__

	return TRUE;
}

void set_customer_softap_config(void)
{
#if defined ( __SUPPORT_FACTORY_RST_APMODE__ )
	/* Set to user costomer's configuration */
	ap_config_param->customer_cfg_flag = MODE_DISABLE;	// MODE_ENABLE, MODE_DISABLE

	/*
	 * Wi-Fi configuration
	 */

	/* SSID prefix */
	sprintf(ap_config_param->ssid_name, "%s", "DA16200");

	/* Default open mode : AP_OPEN_MODE, AP_SECURITY_MODE */
	ap_config_param->auth_type = AP_OPEN_MODE;

	if (ap_config_param->auth_type == AP_SECURITY_MODE)
		sprintf(ap_config_param->psk, "%s", "12345678");

	/* Country Code : Default country US */
	sprintf(ap_config_param->country_code, "%s", DFLT_AP_COUNTRY_CODE);

	/*
	 * Network IP address configuration
	 */
	ap_config_param->customer_ip_address = IPADDR_DEFAULT;

	if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER)
	{
		sprintf(ap_config_param->ip_addr,	"%s", "192.168.1.1");
		sprintf(ap_config_param->subnet_mask,	"%s", "255.255.255.0");
		sprintf(ap_config_param->default_gw,	"%s", "192.168.1.1");
		sprintf(ap_config_param->dns_ip_addr,	"%s", "8.8.8.8");
	}

	/*
	 * DHCP Server configuration
	 */
	ap_config_param->customer_dhcpd_flag = DHCPD_DEFAULT;

	if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER)
	{
		ap_config_param->dhcpd_lease_time	= 3600;

		sprintf(ap_config_param->dhcpd_start_ip, "%s", "192.168.1.101");
		sprintf(ap_config_param->dhcpd_end_ip, "%s", "192.168.1.108");
		sprintf(ap_config_param->dhcpd_dns_ip_addr, "%s", "8.8.8.8");
	}

#endif	// __SUPPORT_FACTORY_RST_APMODE__
}

void button1_one_touch_cb_fn(void)
{
	PRINTF("\n**Neuralert: system_start:button1_one_touch_cb_fn()\n"); // FRSDEBUG

	/* User application function if needed ... */
	button1_one_touch_cb = NULL;
}

void config_gpio_button(void)
{
	/* Set factory-reset button configuration
	 *	: button-gpio / led-gpio / time_seconds */
	config_factory_reset_button(FACTORY_BUTTON, FACTORY_LED, CHECK_TIME_FACTORY_BTN);

	/* Set wps button configuration
	 *	: button-gpio / led-gpio / time_seconds */
	config_wps_button(WPS_BUTTON, WPS_LED, CHECK_TIME_WPS_BTN);

	/* Set gpio interrupt for WPS and factory-reset button */
	set_gpio_interrupt();
	
#if defined ( __TRIGGER_DPM_MCU_WAKEUP__ )
	/* Retain GPIO for DPM sleep */
	if (dpm_mode_is_enabled() == TRUE &&  dpm_mode_is_wakeup() == FALSE)
	{
		_da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
		_GPIO_RETAIN_LOW(GPIO_UNIT_A, GPIO_PIN11);
	}
#endif  // __TRIGGER_MCU_WAKEUP__

	// Configure the LED GPIO pins to be held during sleep2
	// so LEDs don't flash on while sleeping
	_da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);
    //PRINTF("###GPIO_UNIT_C GPIO 6,7,8 retain-high\n");
    _GPIO_RETAIN_HIGH(GPIO_UNIT_C, GPIO_PIN6);
    _GPIO_RETAIN_HIGH(GPIO_UNIT_C, GPIO_PIN7);
    _GPIO_RETAIN_HIGH(GPIO_UNIT_C, GPIO_PIN8);

}

void trigger_mcu_wakeup_gpio(void)
{
#if defined ( __TRIGGER_DPM_MCU_WAKEUP__ )
	HANDLE gpio;
	UINT32 pin;
	UINT16 write_data;

	if (dpm_mode_is_enabled() == TRUE &&  dpm_mode_is_wakeup() == TRUE) {
		_da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
	
		gpio = GPIO_CREATE(GPIO_UNIT_A);
		GPIO_INIT(gpio);

		pin = GPIO_PIN11;
		GPIO_IOCTL(gpio, GPIO_SET_OUTPUT, &pin);

		PRINTF("Waking up MCU ...\n");
		write_data = GPIO_PIN11;
		GPIO_WRITE(gpio, GPIO_PIN11, &write_data, sizeof(UINT16));

		write_data = 0;
		GPIO_WRITE(gpio, GPIO_PIN11, &write_data, sizeof(UINT16));

		_GPIO_RETAIN_LOW(GPIO_UNIT_A, GPIO_PIN11);
	}
#endif	// __TRIGGER_DPM_MCU_WAKEUP__
}

void regist_user_apps_to_DPM_manager(void)
{
#if defined ( __SUPPORT_DPM_MANAGER__ )

//	PRINTF("**Neuralert: regist_user_apps_to_DPM_manager()\n"); // FRSDEBUG

	/*
	 * Need to add user operation to use DPM manager...
	 */

#endif	// __SUPPORT_DPM_MANAGER__
}

static const mbedtls_net_primitive_type da16x_crypto_net_apis = {
	&mbedtls_net_bsd_init,
	&mbedtls_net_bsd_connect,
	&mbedtls_net_bsd_bind,
	&mbedtls_net_bsd_accept,
	&mbedtls_net_bsd_set_block,
	&mbedtls_net_bsd_set_nonblock,
	&mbedtls_net_bsd_usleep,
	&mbedtls_net_bsd_recv,
	&mbedtls_net_bsd_send,
	&mbedtls_net_bsd_recv_timeout,
	&mbedtls_net_bsd_free
};

int regist_crypto_net_socket_apis()
{
	int ret = mbedtls_platform_bind(&da16x_crypto_net_apis);
	if (ret != 2) {
		PRINTF("Failed to register crypto net apis(%d)\r\n", ret);
		return -1;
	}
	return 0;
}

#if defined ( __SUPPORT_USER_DPM_RCV_READY_TO__ )
void set_user_dpm_data_rcv_ready_timeout(void)
{
	/* For dpm_app_register() in user application */
	dpm_app_register_timeout = 10;			// 100 msec, unit is 10msec

	/* For dpm_app_data_rcv_ready_set() in user application */
	dpm_app_data_rcv_ready_set_timeout = 50;	// 500 msec, unit is 10msec
}
#endif	// __SUPPORT_USER_DPM_RCV_READY_TO__

#ifdef CFG_USE_SYSTEM_CONTROL
#define USER_RTM_SYS_CTRL_TAG			"sys_ctrl"
#define SYSTEM_CONTROL_WLAN_DEFAULT		(1)	// 0: Off, 1: On

struct system_control_t {
	uint8_t wlan_enabled;
};
static struct system_control_t *system_control = NULL;

static uint8_t system_control_init(void)
{
#ifdef CFG_USE_RETMEM_WITHOUT_DPM
	/* Create the user retention memory and Initialize. */
	unsigned int result = user_retmmem_get(USER_RTM_SYS_CTRL_TAG,
			(UCHAR **)&system_control);
	if (result == 0) {
		// created a memory for the user data
		result = user_retmmem_allocate(USER_RTM_SYS_CTRL_TAG,
				(void**)&system_control, sizeof(struct system_control_t));
		if (result == 0) {
			memset(system_control, 0, sizeof(struct system_control_t));
			system_control->wlan_enabled = SYSTEM_CONTROL_WLAN_DEFAULT;
		} else {
			PRINTF("%s: Failed to allocate retention memory\n", __func__);
		}
	}

	return (system_control != NULL);
#else
	return pdTRUE;
#endif
}

static uint8_t system_control_wlan_enabled(void)
{
	if (system_control_init() && system_control) {
		return system_control->wlan_enabled;
	} else {
		return pdTRUE;
	}
}

void system_control_wlan_enable(uint8_t onoff)
{
	if (system_control) {
		system_control->wlan_enabled = onoff;
	} else {
		PRINTF("%s: System control is not available!\n", __func__);
	}
}
#endif // CFG_USE_SYSTEM_CONTROL

// **************************************************************************
// system_start()
// **************************************************************************
void system_start(void)
{

//	PRINTF(">>Neuralert: In system_start()\n"); // FRSDEBUG

#if defined ( __SUPPORT_USER_DPM_RCV_READY_TO__ )
	/* Set user DPM register timeout value */
    set_user_dpm_data_rcv_ready_timeout();
#endif	// __SUPPORT_USER_DPM_RCV_READY_TO__

#if defined ( __BLE_COMBO_REF__ ) && defined (__ENABLE_SAMPLE_APP__)
	ble_combo_ref_flag = pdFALSE;
#endif // ( __BLE_COMBO_REF__ ) && (__ENABLE_SAMPLE_APP__)

	/* Config HW wakeup resource */
	config_user_wu_hw_resource();

	/* Set configuration for H/W button */
	config_gpio_button();

	/* Set parameters for system running */
	set_sys_config();

#if defined (CFG_USE_RETMEM_WITHOUT_DPM)
	user_retmem_init();
#endif

#if defined ( __BLE_COMBO_REF__ ) && defined (__BLE_FEATURE_ENABLED__)
	if (ble_combo_ref_flag == pdTRUE) {
		/* combo: init 4-wire uart, dpm_boot_type, ble_boot_mode, download ble fw */
		combo_init();
	}
#endif

	/* Regist Wi-Fi connect/disconnect status notify call-back functions */
	regist_wifi_notify_cb();

	/* Initialize WLAN interface */
//	PRINTF(">>Neuralert: system_start() calling wlaninit\n"); // FRSDEBUG
#ifdef CFG_USE_SYSTEM_CONTROL
	wlaninit(system_control_wlan_enabled());
#else
	wlaninit();
#endif

	if (runtime_cal_flag == pdTRUE) {
		printf_with_run_time(" return from wlaninit");
	}

#if defined ( __SUPPORT_BTCOEX__ )
	if (ble_combo_ref_flag == pdTRUE) {
		/* Initialize Wi-Fi & BT/BLE Coexistance for Wi-Fi & BLE Combo module */
		initialize_bt_coex();
	}
#endif  // __SUPPORT_BTCOEX__

	/* Setup WPS button */
	if (check_wps_button(wps_btn, wps_led, wps_btn_chk_time) == pdTRUE) {
		wps_setup(NULL);
	}

	/* Start GPIO polling thread */
	start_gpio_thread();
	
	if (ble_combo_ref_flag == pdTRUE) {	
#if defined (__CONNECTION_STATUS_LED__)
		LED_threadCreate(1); // Thread for LED action to be used on Provision demo
#endif // (__CONNECTION_STATUS_LED__)
	}

	set_dpm_abnorm_user_wakeup_interval();

	/* Register mbedtls net apis */
	regist_crypto_net_socket_apis();

	if (ble_combo_ref_flag == pdTRUE) {
#if defined (__APP_SLEEP2_MONITOR__)	
#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__)
		extern void app_todo_before_sleep(void);

		sleep2_monitor_config(2, app_todo_before_sleep);
#endif
		sleep2_monitor_start();
#endif // __APP_SLEEP2_MONITOR__
	}
	
#if defined ( __SUPPORT_DPM_MANAGER__ )
	/* Regist User DPM application before start system application */
	regist_user_apps_to_DPM_manager();
#endif	// __SUPPORT_DPM_MANAGER__

	/* Copy NVRAM from #0 to #1 if POR and both NVRAM0 and NVRAM1 are different. */
	if ((dpm_get_wakeup_source() & WAKEUP_SOURCE_POR) && compare_NVRAM() == 1)
	{
		copy_nvram_flash(SFLASH_NVRAM_BACKUP, SFLASH_NVRAM_ADDR, SFLASH_NVRAM_BACKUP_SZ);
	}

	/* Start system applications for DA16XXX */
	start_sys_apps();

	/*
	 * Entry point of user's applications
	 * 	: defined in user_apps_table.c
	 */
	/* Start system applications for DA16XXX */
	start_user_apps();
}

#if defined ( __BLE_COMBO_REF__ ) && !defined (__ENABLE_SAMPLE_APP__)
void combo_make_sleep_rdy(void)
{
#if defined (__ENABLE_DPM_FOR_GTL_BLE_APP__) && defined (__APP_SLEEP2_MONITOR__)
    extern int8_t sleep2_monitor_is_running(void);
    extern void sleep2_monitor_set_sleep2_rdy(void);
    if (sleep2_monitor_is_running() == TRUE)
    {
        sleep2_monitor_set_sleep2_rdy();
    }
#elif defined (__ENABLE_DPM_FOR_GTL_BLE_APP__) && !defined (__APP_SLEEP2_MONITOR__)
    extern void app_todo_before_sleep(void);
    app_todo_before_sleep();
#endif
}
#endif	//( __BLE_COMBO_REF__ ) && !(__ENABLE_SAMPLE_APP__)
/* EOF */
