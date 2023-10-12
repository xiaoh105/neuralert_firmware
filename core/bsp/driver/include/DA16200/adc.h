/**
 * \addtogroup HALayer
 * \{
 * \addtogroup ADC
 * \{
 * \brief Analog Digital Converter controller
 */
 
/**
 ****************************************************************************************
 *
 * @file adc.h
 *
 * @brief
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


#ifndef __adc_h__
#define __adc_h__

#include <stdio.h>
#include <stdlib.h>

#include "hal.h"


//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
typedef 	struct __adc_handler__	ADC_HANDLER_TYPE; 	// deferred
typedef 	struct __adc_regmap__ 	ADC_REG_MAP;		// deferred

//---------------------------------------------------------
//	DRIVER :: Commands & Parameters
//---------------------------------------------------------
//
//	Equipment Type
//

/**
 * \brief ADC FIFO
 */
typedef enum	{
	ADC_FIFO0	= 0,
	ADC_FIFO1	,
	ADC_FIFO2	,
	ADC_FIFO3	,
	ADC_MAX
} ADC_LIST;

/**
 * \brief ADC Callback Type
 */
typedef	enum	{
	ADC_CB_TERM  = 0,
	ADC_CB_EMPTY ,
	ADC_CB_MAX

} ADC_CALLBACK_LIST;

/**
 * \brief ADC Interrupt Type
 */
typedef enum	{
	ADC_12B_OVER_TH	= 0,
	ADC_16B_OVER_TH	,
	ADC_12B_UNDER_TH	,
	ADC_16B_UNDER_TH	,
} ADC_INTR_TH;



/**
 * \brief ADC IO Control List
 */
typedef enum	{
	ADC_SET_DEBUG = 0,
	ADC_SET_CONFIG ,
	ADC_GET_CONFIG ,
	ADC_SET_DIVIDER ,
	ADC_GET_DIVIDER ,
	ADC_SET_DMA_RD,
	ADC_UNSET_DMA_RD,
    ADC_GET_DMA_RD,
	ADC_GET_STATUS,
	ADC_SET_RESET,
	ADC_SET_HISR_CALLACK,
    ADC_SET_CLOCK    ,
    ADC_SET_INTERRUPT_MODE,
    ADC_GET_INTERRUPT_MODE,
	ADC_ENABLE_INTERRUPT,
	ADC_SET_PORT,
	ADC_SET_CHANNEL,
	ADC_GET_CHANNEL,

	ADC_SET_STEP,
	ADC_GET_STEP,

	ADC_GET_MODE_TIMESTAMP,

	ADC_SET_DATA_MODE,

	ADC_GET_RESOLUTION,
	ADC_SET_RESOLUTION,

	ADC_GET_RTC_THD0,
	ADC_SET_RTC_THD0,

	ADC_GET_RTC_THD1,
	ADC_SET_RTC_THD1,

	ADC_GET_RTC_THD2,
	ADC_SET_RTC_THD2,

	ADC_GET_RTC_THD3,
	ADC_SET_RTC_THD3,

	ADC_GET_RTC_COMPARE_MODE,
	ADC_SET_RTC_COMPARE_MODE,

	ADC_GET_RTC_CYCLE_BEFORE_ON,
	ADC_SET_RTC_CYCLE_BEFORE_ON,

	ADC_GET_RTC_CYCLE_BEFORE_CAPTURE,
	ADC_SET_RTC_CYCLE_BEFORE_CAPTURE,

	ADC_ENABLE_RTC_SENSOR,
	ADC_DISABLE_RTC_SENSOR,

	ADC_SET_RTC_TIMER1,
	ADC_SET_RTC_TIMER2,

	ADC_SET_CAPTURE_STEP,

	ADC_SET_NUMBER_SAMPLES, //(2^(n+2))

	ADC_SET_MAX
} ADC_IOCTL_LIST;

//
//	IOCTL Commands
//


/**
 * \brief ADC Resolution
 */
typedef enum	{

	ADC_RESOLUTION_12B,
	ADC_RESOLUTION_10B,
	ADC_RESOLUTION_7B,
	ADC_RESOLUTION_4B,

}ADC_RESOLUTION_TYPE;

//---------------------------------------------------------
//	DRIVER :: Bit Field Definition
//---------------------------------------------------------


#define	ADC_DMA_HISR_DONE	0x00800000

//
//	ADC Control Register
//
#define 	ADC_CTRL_ENABLE		((0x1)<<0)


#define  DMA_MAX_LLI_NUM (10)

#define		ADC_STATUS_RD_DONE  ((0x1)<<0)


#define SYS_ADC_BASE  					(DA16200_ADC0_BASE)
//#define SYS_ADC_COMMON_CLOCK_DIVIDER	(FC9000_STIMER1_BASE+0x1c)

#define HW_REG_WRITE32_ADC(addr,value)  ((addr) = (value))
#define HW_REG_READ32_ADC(addr)   ((addr))
#define HW_REG_AND_WRITE32_ADC(addr,value)  (addr) = ((addr)&(value))
#define HW_REG_OR_WRITE32_ADC(addr,value) (addr) = ((addr)|(value))

#define DRIVER_DBG_NONE DRV_PRINTF
#define DRIVER_DBG_ERROR DRV_PRINTF



//typedef 	VOID	(*USR_CALLBACK )(VOID *);

//---------------------------------------------------------
//	DRIVER :: Structure
//---------------------------------------------------------

/**
 * \brief ADC Register MAP
 */
struct	__adc_regmap__
{
	volatile UINT32	axadc12B_ctrl;		// 0x00
	volatile UINT32	reserved_0;		// 0x04
	volatile UINT32	adc_en_ch;		// 0x08
	volatile UINT32	xadc_thr_intr_mask; //0x0c int_ctrl_over_th;			// 0x0C
	volatile UINT32	xadc_thr_intr_clr; //0x10 int_ctrl_under_th;		// 0x10
	volatile UINT32	xadc_intr_ctrl_fifo;		// 0x14
	volatile UINT32	xadc_intr_status;		// 0x18
	volatile UINT32	adc_dmaen;			// 0x1c
	volatile UINT32	fifo0_data;		// 0x20
	volatile UINT32	fifo1_data;		// 0x24
	volatile UINT32	fifo2_data;		// 0x28
	volatile UINT32	fifo3_data;			// 0x2c
	volatile UINT32	axadc_rm_sample;			// 0x30
	volatile UINT32	fxadc_step_sample;			// 0x34
	volatile UINT32	adc12B_intr_thr_over;			// 0x38
	volatile UINT32	reserved_1;			// 0x3c
	volatile UINT32	adc12B_intr_thr_under;			// 0x40
	volatile UINT32	reserved_2;			// 0x44
	volatile UINT32	adc12B_intr_thr_diff;			// 0x48
	volatile UINT32	reserved_3;					// 0x4c
	volatile UINT32	fifo0_data_curr;			// 0x50
	volatile UINT32	fifo1_data_curr;			// 0x54
	volatile UINT32	fifo2_data_curr;			// 0x58
	volatile UINT32	fifo3_data_curr;			// 0x5c
	volatile UINT32	time_stamp_ctrl;			// 0x60
	volatile UINT32	rsvd1[3];					// 0x64 ~ 6c
	volatile UINT32	reserved_4;			// 0x70
};


/**
 * \brief ADC Handle Structure
 */
struct	__adc_handler__
{
	UINT32				dev_addr;  // Unique Address
	// Device-dependant
	UINT32				dev_unit;
	UINT32				instance;
	UINT32				clock;
	UINT32				use_timestamp;
	UINT32				interrupt_mode;

	// Register Map
	ADC_REG_MAP			*regmap;
	RTC_REG_MAP			*regmap_rtc;
	UINT32				dmachan_id_ADC_FIFO0;
	UINT32				dmachan_id_ADC_FIFO1;
	UINT32				dmachan_id_ADC_FIFO2;
	UINT32				dmachan_id_ADC_FIFO3;
	HANDLE				dmachannel_ADC_FIFO0;
	HANDLE				dmachannel_ADC_FIFO1;
	HANDLE				dmachannel_ADC_FIFO2;
	HANDLE				dmachannel_ADC_FIFO3;
	UINT32				dma_maxlli;


	USR_CALLBACK		callback[ADC_CB_MAX];
	VOID				*cb_param[ADC_CB_MAX];

	EventGroupHandle_t				mutex;
};

#define	ADC_INTERRUPT_FIFO_HALF				(0)
#define	ADC_INTERRUPT_FIFO_FULL				(1)
#define	ADC_INTERRUPT_THD_OVER				(2)
#define	ADC_INTERRUPT_THD_UNDER				(3)
#define	ADC_INTERRUPT_THD_DIFF				(4)
#define	ADC_INTERRUPT_ALL					(0xf)

#define	ADC_THRESHOLD_TYPE_12B_OVER			(0)
#define	ADC_THRESHOLD_TYPE_12B_UNDER		(2)
#define	ADC_THRESHOLD_TYPE_12B_DIFF			(4)

#define	ADC_BIT_POS_THD_OVER_INTR_SET		(0)
#define	ADC_BIT_POS_THD_UNDER_INTR_SET		(4)
#define	ADC_BIT_POS_THD_DIFF_INTR_SET		(8)

#define	ADC_BIT_POS_FIFO_HALF_INTR_STATUS	(0)
#define	ADC_BIT_POS_FIFO_FULL_INTR_STATUS	(4)
#define	ADC_BIT_POS_THD_OVER_INTR_STATUS	(8)
#define	ADC_BIT_POS_THD_UNDER_INTR_STATUS	(12)
#define	ADC_BIT_POS_THD_DIFF_INTR_STATUS	(16)

#define	ADC_INTERRUPT_MODE_MASK				(0x1)
#define ADC_INTERRUPT_MODE_EVENT			(0x2)

#define	ADC_BIT_POS_CNTL_PDB				(1)
#define	ADC_BIT_POS_CNTL_RESET				(0)

#define	ADC_BIT_POS_CNTL_BITNUM				(14)

#define	ADC_BIT_POS_HW_THD_TYPE				(12)

#define	ADC_HW_THRESHOLD_TYPE_OVER			(0x0)
#define	ADC_HW_THRESHOLD_TYPE_DIFF			(0x1)
#define	ADC_HW_THRESHOLD_TYPE_UNDER			(0x2)

#define	ADC_BIT_POS_HW_TIME_EXT_ACTIVE		(16)
#define	ADC_BIT_POS_HW_TIME_IP3_ACTIVE		(8)
#define	ADC_BIT_POS_HW_TIME_SENS_DETECT		(7)
#define	ADC_BIT_POS_HW_TIMER_X12_CLOCK		(4)
#define	ADC_BIT_POS_HW_TIMER_SET			(0)

#define ADC_HW_TIMER_SENSOR_MODE_ENABLE		(0x80)
#define ADC_HW_TIMER_SENSOR_MODE_DISABLE	(~(0x80))

#define	ADC_RTC_AVERAGE_TYPE_NORMAL			(0x4000)
#define	ADC_RTC_AVERAGE_TYPE_ALPHA			(0x0000)

#define	ADC_RTC_THRESHOLD_TYPE_OVER			(0x0000)
#define	ADC_RTC_THRESHOLD_TYPE_UNDER		(0x1000)
#define	ADC_RTC_THRESHOLD_TYPE_DIFF			(0x2000)

#define	DA16200_ADC_DEVICE_ID		(0)/**< -*/

#define	DA16200_ADC_CH_0			(0)/**< -*/
#define	DA16200_ADC_CH_1			(1)/**< -*/
#define	DA16200_ADC_CH_2			(2)/**< -*/
#define	DA16200_ADC_CH_3			(3)/**< -*/

//---------------------------------------------------------
//	DRIVER :: APP-Interface
//---------------------------------------------------------

/**
 * \brief Create ADC Handle
 *
 * This function will create ADC handle
 *
 * \param [in] dev_id ADC controller ID. \parblock
 *      - ::DA16200_ADC_DEVICE_ID
 * \endparblock
 *
 * \return HANDLE for adc access
 *
 */
extern HANDLE	DRV_ADC_CREATE( UINT32 dev_id );

/**
 * \brief Initialize adc 
 *
 * This function will init adc 
 *
 * \param [in] handler handle for intialization ADC block
 * \param [in] use_timestamp 1 : use time stamp, 0:do not use time stamp
 *
 * \return TRUE in case success, FALSE otherwise
 *
 * \note when using time stamp, read data from adc h/w is 4 bytes aligned. 
 */
extern int	DRV_ADC_INIT (HANDLE handler, unsigned int use_timestamp);

/**
 * \brief misc. control for ADC block 
 *
 * This function will control miscellaneous ADC controller
 *
 * \param [in] handler handle for control ADC block
 * \param [in] cmd Command
 * \param [in,out] Buffer Pointer to Write or Read
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_ADC_IOCTL(HANDLE handler, UINT32 cmd , VOID *data );

/**
 * \brief Start clock for ADC 
 *
 * This function will start clock generator with divider value 
 *
 * \param [in] handler handle for Start ADC block
 * \param [in] divider12 clock divider. (1MHz / (divider12+1)) is active sampling frequency
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_START(HANDLE handler, UINT32 divider12, UINT32 dummy);

/**
 * \brief Stop clock for ADC 
 *
 * \param [in] handler handle for Start ADC block
 * This function will stop clock generator 
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_STOP(HANDLE handler,  UINT32 dummy);

/**
 * \brief  Close ADC Handle
 *
 * \param [in] handler handle for Close ADC block
 *
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int	DRV_ADC_CLOSE(HANDLE handler);

/**
 * \brief Read single ADC data 
 *
 * This function will read instant ADC value 
 *
 * \param [in] handler handle for intialization ADC block
 * \param [in] channel Possible values for channel is: \parblock
 *      - ::DA16200_ADC_CH_0<br>
 *      - ::DA16200_ADC_CH_1<br>
 *      - ::DA16200_ADC_CH_2<br>
 *      - ::DA16200_ADC_CH_3<br>
 * \endparblock
 *
 * \param [out] data buffer pointer to get read ptim_adc value
 * \param [in] dummy not used 
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_READ(HANDLE handler, UINT32 channel, UINT32 *data,  UINT32 dummy);

/**
 * \brief Read single ADC data in FIFO
 *
 * This function will read single ADC value inside FIFO
 *
 * \param [in] handler handle for intialization ADC block
 * \param [in] channel Possible values for channel is: \parblock
 *      - ::DA16200_ADC_CH_0<br>
 *      - ::DA16200_ADC_CH_1<br>
 *      - ::DA16200_ADC_CH_2<br>
 *      - ::DA16200_ADC_CH_3<br>
 * \endparblock
 *
 * \param [out] data buffer pointer to get single read ptim_adc value
 * \param [in] dummy not used 
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_READ_FIFO(HANDLE handler, UINT32 channel, UINT32 *data,  UINT32 dummy);

/**
 * \brief Read single ADC data with DMA
 *
 * This function will read multiple ADC value inside FIFO
 *
 * \param [in] handler handle for Read ADC block
 * \param [in] channel Possible values for channel is: \parblock
 *      - ::DA16200_ADC_CH_0<br>
 *      - ::DA16200_ADC_CH_1<br>
 *      - ::DA16200_ADC_CH_2<br>
 *      - ::DA16200_ADC_CH_3<br>
 * \endparblock
 *
 * \param [in/out] p_data Data pointe to fill read ADC data
 * \param [in] p_dlen length to read adc value
 * \param [in] wait 1 : wait till end of DMA operation, 0 : return immediately
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_READ_DMA(HANDLE handler,  UINT32 channel, UINT16 *p_data, UINT32 p_dlen, UINT32 wait, UINT32 dummy);

/**
 * \brief Enable ADC channel
 *
 * This function will enable ADC channel 
 *
 * \param [in] handler handle for Enable ADC Channel
 * \param [in] channel Possible values for channel is: \parblock
 *      - ::DA16200_ADC_CH_0<br>
 *      - ::DA16200_ADC_CH_1<br>
 *      - ::DA16200_ADC_CH_2<br>
 *      - ::DA16200_ADC_CH_3<br>
 * \endparblock
 *
 * \param [in] sel_adc Possible values for flags are:
 * \parblock
 *      ::DA16200_ADC_DIVIDER_12<br>
 * \endparblock
 * \return TRUE in case success, FALSE otherwise
 *
 */
extern int DRV_ADC_ENABLE_CHANNEL(HANDLE handler,  UINT32 channel, unsigned int sel_adc, UINT32 dummy);
extern int DRV_ADC_SET_INTERRUPT(HANDLE handler, UINT32 channel, UINT32 enable, UINT32 type, UINT32 dummy);
extern int DRV_ADC_SET_THD_VALUE(HANDLE handler, UINT32 type, UINT32 thd, UINT32 dummy);
extern int DRV_ADC_WAIT_INTERRUPT(HANDLE handler, UNSIGNED *mask_evt);
extern int	DRV_ADC_SET_THRESHOLD(HANDLE handler, UNSIGNED channel, UNSIGNED threshold,  UNSIGNED mode);
extern int	DRV_ADC_SET_DELAY_AFTER_WKUP(HANDLE handler, UNSIGNED delay1, UNSIGNED delay2);
extern int	DRV_ADC_SET_RTC_ENABLE_CHANNEL(HANDLE handler, UNSIGNED ch, UNSIGNED enable);
extern int	DRV_ADC_SET_SLEEP_MODE(HANDLE handler, UNSIGNED	x12_clk, UNSIGNED reg_ax12b_timer, UNSIGNED adc_step);

void	adc_drv_interrupt(void);
void adc_drv_interrupt_handler(void);

//---------------------------------------------------------
//	DRIVER :: DEVICE-Interface
//---------------------------------------------------------


//---------------------------------------------------------
//	DRIVER :: ISR
//---------------------------------------------------------



//---------------------------------------------------------
//	DRIVER :: Utility
//---------------------------------------------------------


#define ADC_DEBUG_ON

#ifdef ADC_DEBUG_ON
#define PRINT_ADC(...)  Printf(__VA_ARGS__)
#else
#define PRINT_ADC
#endif


#endif /* __adc_h__ */

/**
 * \}
 * \}
 */

/* EOF */
