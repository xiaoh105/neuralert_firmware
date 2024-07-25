/*!
 * \file globals.c
 *
 * \brief global variables and buffers
 *
 *  Created on: Sep 22, 2021
 *  \author: T. J. Mulrooney
 */
#include <stdint.h>
#include "common.h"
#include "da16x_time.h"

#define NUMBER_LEDS 4

HANDLE gpioa, gpioc, hadc;							//!< GPIO and ADC device handles
HANDLE I2C = NULL;
//HANDLE SPI = NULL;									// Serial flash SPI bus handle
//HANDLE MQTT_SPI_handle = NULL;						// SPI handle for the MQTT task to use

uint32_t duty_cycle_seconds;						//!< current remaining duty cycle seconds
//accelDataStruct accelData[NUMBER_SAMPLES_TOTAL];	//!< accelerometer data buffer
/*
 * Data structure where one FIFO full of data is stored by the Accelerometer
 * when a FIFO threshold interrupt occurs
 */
accelBufferStruct receivedFIFO;					//!< accelerometer data buffer for FIFO dumping
//accelBufferStruct checkFIFO;					//!< FIFO read back after writing

UINT8 i2c_data_read[AT_I2C_DATA_LENGTH];
UINT8 i2c_data[AT_I2C_DATA_LENGTH + AT_I2C_LENGTH_FOR_WORD_ADDRESS] = {0,};
UINT8 sensorTypePresentAll = 0;						//!< available sensors
int16_t lastXvalue;
int16_t lastYvalue;
int16_t lastZvalue;

//Added LED - NJ 06/14/2022
ledStruct ledControl[NUMBER_LEDS];					//!< led control as individual LEDS
ledColorStruct ledColor;							//!< led control as single RGB color

//TaskHandle_t handleAccelTask;						//!< accelerometer task handle
//uint8_t accelIntFlag =0;

// *************************************
// Timestamp management
// *************************************

// Timestamp when accelerometer interrupt occurred (RTC counter @ 32.768 kHz)
// Note some of these were experiments while working out which clock
// time to use.  They aren't all necessary
//long long user_rtc_wakeup_time;    // RTC clock when wakeup happened
//__time64_t user_rtc_wakeup_time_msec;  // da16xx msec time when wakeup happened
__time64_t user_current_wakeup_time_int;   // da16xx_time64() when wakeup happened

// This is the time relative to being wakened from sleep as close as possible
// (as of 9/1/22 done in user_init().
__time64_t user_raw_rtc_wakeup_time_msec;  // relative msec time when wakeup happened
int user_raw_rtc_wakeup_time_source;		// code for where this time came from
__time64_t user_raw_launch_time_msec;		// early timestamp

// When an accelerometer interrupt occurs and we're still awake,
// the interrupt takes this snapshot of the RTC clock
// so we know when it happened.  Processing will happen later,
// after the event works its way through the event processor
// and whatever other delays occur
//unsigned long long user_accelerometer_interrupt_time;	// RTC clock when wakeup happened


//uint8_t pubAckSeen = 0;								//!< =1 reply from AWS that publish was seen, =0 publish not seen

// Runflag that reflects the state of the NVRAM runflag set by
// a user command.  1 means go and 0 means just hang around near
// the start of the application to give the user time to
// do any necessary setup
uint8_t runFlag = 0;								//!< =1 start message processing, =0 do not start task

// MAC address of this particular DA16200 used for a unique
// device identifier when communicating with the cloud
char macstr[18];				//!< complete MAC address ASCII string
char MACaddr[7];				//!< last 6 characters of MAC address


