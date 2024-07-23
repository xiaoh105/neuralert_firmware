/**
 * \addtogroup HALayer
 * \{
 * \addtogroup RTC_DRIVER
 * \{
 * \brief Real time clock driver.
 */
/**
 ****************************************************************************************
 *
 * @file rtc.h
 *
 * @brief RTC Driver
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


//---------------------------------------------------------
//    this driver is used only for a evaluation.
//---------------------------------------------------------

#ifndef __rtc_h__
#define __rtc_h__

#include "hal.h"

#ifndef HW_REG_AND_WRITE32
#define    HW_REG_AND_WRITE32(x, v)    ( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) & (v) )
#endif
#ifndef HW_REG_OR_WRITE32
#define    HW_REG_OR_WRITE32(x, v)        ( *((volatile unsigned int   *) &(x)) = *((volatile unsigned int   *) &(x)) | (v) )
#endif
#ifndef HW_REG_WRITE32
#define HW_REG_WRITE32(x, v)         ( *((volatile unsigned int   *) &(x)) = (v) )
#endif
#ifndef HW_REG_READ32
#define    HW_REG_READ32(x)            ( *((volatile unsigned int   *) &(x)) )
#endif

/* watch dog setting period */
/*
    value x -> 2^x < period < 2*2^x
    ex)
    value 3 -> 8s < period < 16s
*/
//---------------------------------------------------------
// Global Definition
//---------------------------------------------------------
/**
 * \brief Wakeup Source
 *
 *
 */
typedef enum _WAKEUP_SOURCE_ {
    WAKEUP_RESET                                    = 0x00,         /**< Internal reset */
    WAKEUP_SOURCE_EXT_SIGNAL                        = 0x01,         /**< Boot from external wake up signal */
    WAKEUP_SOURCE_WAKEUP_COUNTER                    = 0x02,         /**< Boot from wake up counter */
    WAKEUP_EXT_SIG_WAKEUP_COUNTER                   = 0x03,         /**< Boot from wakeup counter or external wakeup signal */
    WAKEUP_SOURCE_POR                               = 0x04,         /**< Boot from power on reset */
    WAKEUP_SOURCE_POR_EXT_SIGNAL                    = 0x05,         /**< FPGA only */
    WAKEUP_WATCHDOG                                 = 0x08,         /**< Boot from RTC_watch dog (not cpu watchdog) */
    WAKEUP_WATCHDOG_EXT_SIGNAL                      = 0x09,         /**< Boot from watch dog or external wakeup signal */
    WAKEUP_SENSOR                                   = 0x10,         /**< Boot from sensor (ADC) */
    WAKEUP_PULSE                                    = 0x20,         /**< Boot from pulse sensor */
    WAKEUP_GPIO                                     = 0x40,         /**< Boot from gpio */
    WAKEUP_SENSOR_EXT_SIGNAL                        = 0x11,         /**< Boot from sensor or external wakeup signal */
    WAKEUP_SENSOR_WAKEUP_COUNTER                    = 0x12,         /**< Boot from sensor or wakeup counter */
    WAKEUP_SENSOR_EXT_WAKEUP_COUNTER                = 0x13,         /**< Boot from sensor or external or wakeup counter */
    WAKEUP_SENSOR_WATCHDOG                          = 0x18,         /**< Boot from sensor or RTC watch dog */
    WAKEUP_SENSOR_EXT_WATCHDOG                      = 0x19,         /**< Boot from sensor or external or watch dog */
    WAKEUP_RESET_WITH_RETENTION                     = 0x80,         /**< Boot from internal reset and have retention info */
    WAKEUP_EXT_SIG_WITH_RETENTION                   = 0x81,         /**< Boot from external signal and have retention info */
    WAKEUP_COUNTER_WITH_RETENTION                   = 0x82,         /**< Boot from counter and have retention info (most common) */
    WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION    = 0x83,         /**< Boot from external signal or wakeup counter with retention */
    WAKEUP_WATCHDOG_WITH_RETENTION                  = 0x88,         /**< Boot from RTC watch dog with retention */
    WAKEUP_WATCHDOG_EXT_SIGNAL_WITH_RETENTION       = 0x89,         /**< Boot from watch dog or external wakeup sig */
    WAKEUP_SENSOR_WITH_RETENTION                    = 0x90,         /**< Boot from sensor with retention */
    WAKEUP_SENSOR_EXT_SIGNAL_WITH_RETENTION         = 0x91,         /**< Boot from sensor or external signal with retention */
    WAKEUP_SENSOR_WAKEUP_COUNTER_WITH_RETENTION     = 0x92,         /**< Boot from sensor or wakeup counter with retention */
    WAKEUP_SENSOR_EXT_WAKEUP_COUNTER_WITH_RETENTION = 0x93,         /**< Boot from sensor or external sig or wakeup counter with retention */
    WAKEUP_SENSOR_WATCHDOG_WITH_RETENTION           = 0x98,         /**< Boot from sensor or watchdog with retention */
    WAKEUP_SENSOR_EXT_WATCHDOG_WITH_RETENTION       = 0x99,         /**< Boot from sensor or external sig or watchdog with retention */

    WAKEUP_SOURCE_UNKNOWN                           = 0xff,
}WAKEUP_SOURCE;

/**
 * \brief RTC clock Source
 *
 *
 */
typedef enum _XTAL_ {
    INTERNAL_OSC,               /**< internal 32k OSC */
    EXTERNAL_XTAL,              /**< external 32k XTAL */
    XTAL_ERROR,
}XTAL_TYPE;

/* 0x50091008 */
/* GPIO wakeup */
#define GPIOA13_OR_GPIOA14(x)               ((x&0x01) << 25)
#define GPIOC5_OR_GPIOA12(x)                ((x&0x01) << 24)
#define GPIOA11_OR_GPIOC8(x)                ((x&0x01) << 23)
#define GPIOA10_OR_GPIOC7(x)                ((x&0x01) << 22)
#define GPIOA9_OR_GPIOC6(x)                 ((x&0x01) << 21)
#define GPIOA8_OR_GPIOC4(x)                 ((x&0x01) << 20)
#define GPIOA7_OR_GPIOC3(x)                 ((x&0x01) << 19)
#define GPIOA6_OR_GPIOC2(x)                 ((x&0x01) << 18)
#define GPIOA5_OR_GPIOC1(x)                 ((x&0x01) << 17)
#define GPIOA4_OR_GPIOC0(x)                 ((x&0x01) << 16)

/* RTC wakeup pin sel 2,3,4  0: active high, 1: active low */
#define RTC_WAKEUP4_SEL(x)                  ((x&0x01) << 12)
#define RTC_WAKEUP3_SEL(x)                  ((x&0x01) << 11)
#define RTC_WAKEUP2_SEL(x)                  ((x&0x01) << 10)

/* GPIO wakeup sel  0: active high, 1: active low */
#define GPIOA13_OR_GPIOA14_EDGE_SEL(x)      ((x&0x01) << 9)
#define GPIOC5_OR_GPIOA12_EDGE_SEL(x)       ((x&0x01) << 8)
#define GPIOA11_OR_GPIOC8_EDGE_SEL(x)       ((x&0x01) << 7)
#define GPIOA10_OR_GPIOC7_EDGE_SEL(x)       ((x&0x01) << 6)
#define GPIOA9_OR_GPIOC6_EDGE_SEL(x)        ((x&0x01) << 5)
#define GPIOA8_OR_GPIOC4_EDGE_SEL(x)        ((x&0x01) << 4)
#define GPIOA7_OR_GPIOC3_EDGE_SEL(x)        ((x&0x01) << 3)
#define GPIOA6_OR_GPIOC2_EDGE_SEL(x)        ((x&0x01) << 2)
#define GPIOA5_OR_GPIOC1_EDGE_SEL(x)        ((x&0x01) << 1)
#define GPIOA4_OR_GPIOC0_EDGE_SEL(x)        ((x&0x01) << 0)

/* 0x5009100c */
/* RTC wakeup enable 2,3,4 */
#define RTC_WAKEUP4_EN(x)                   ((x&0x01) << 28)
#define RTC_WAKEUP3_EN(x)                   ((x&0x01) << 27)
#define RTC_WAKEUP2_EN(x)                   ((x&0x01) << 26)

#define GPIOA13_OR_GPIOA14_INT_EN(x)        ((x&0x01) << 25)
#define GPIOC5_OR_GPIOA12_INT_EN(x)         ((x&0x01) << 24)
#define GPIOA11_OR_GPIOC8_INT_EN(x)         ((x&0x01) << 23)
#define GPIOA10_OR_GPIOC7_INT_EN(x)         ((x&0x01) << 22)
#define GPIOA9_OR_GPIOC6_INT_EN(x)          ((x&0x01) << 21)
#define GPIOA8_OR_GPIOC4_INT_EN(x)          ((x&0x01) << 20)
#define GPIOA7_OR_GPIOC3_INT_EN(x)          ((x&0x01) << 19)
#define GPIOA6_OR_GPIOC2_INT_EN(x)          ((x&0x01) << 18)
#define GPIOA5_OR_GPIOC1_INT_EN(x)          ((x&0x01) << 17)
#define GPIOA4_OR_GPIOC0_INT_EN(x)          ((x&0x01) << 16)

/* RTC wakeup source */
#define RTC_WAKEUP4_STATUS                  ((0x01) << 13)
#define RTC_WAKEUP3_STATUS                  ((0x01) << 12)
#define RTC_WAKEUP2_STATUS                  ((0x01) << 11)
#define RTC_WAKEUP_STATUS                   ((0x01) << 10)

#define GPIOA13_OR_GPIOA14_INT_STATUS       ((0x01) << 9)
#define GPIOC5_OR_GPIOA12_INT_STATUS        ((0x01) << 8)
#define GPIOA11_OR_GPIOC8_INT_STATUS        ((0x01) << 7)
#define GPIOA10_OR_GPIOC7_INT_STATUS        ((0x01) << 6)
#define GPIOA9_OR_GPIOC6_INT_STATUS         ((0x01) << 5)
#define GPIOA8_OR_GPIOC4_INT_STATUS         ((0x01) << 4)
#define GPIOA7_OR_GPIOC3_INT_STATUS         ((0x01) << 3)
#define GPIOA6_OR_GPIOC2_INT_STATUS         ((0x01) << 2)
#define GPIOA5_OR_GPIOC1_INT_STATUS         ((0x01) << 1)
#define GPIOA4_OR_GPIOC0_INT_STATUS         ((0x01) << 0)


/* rtc_control 0x50091010*/
#define WAKEUP_PIN_ENABLE(x)                ((x&0x01) << 6)
#define BROWN_OUT_INT_ENABLE(x)             ((x&0x01) << 5)
#define BLACK_OUT_INT_ENABLE(x)             ((x&0x01) << 4)
#define WATCHDOG_ENABLE(x)                  ((x&0x01) << 3)
#define WAKEUP_POLARITY(x)                  ((x&0x01) << 2)
#define WAKEUP_INTERRUPT_ENABLE(x)          ((x&0x01) << 1)
#define WAKEUP_COUNTER_ENABLE(x)            (x&0x01)

/* xtal_control 0x50001014 */
/* ext xtal ready */
#define VBAT_BIAS_I_CTRL(x)                 ((x&0x01) << 10)
#define XTAL_LDO_I_CTRL(x)                  ((x&0x03) << 8)
#define XTAL_LDO_CTRL(x)                    ((x&0x07) << 5)
#define EXT_RESISTOR_ENABLE(x)              ((x&0x01) << 4)
#define CLOCK_SOURCE_SEL(x)                 ((x&0x03) << 2)
#define XTAL_ENABLE(x)                      ((x&0x01) << 1)
#define OSC_ENABLE(x)                       (x&0x01)
#define XTAL_DISABLE(x)                     ((x&0x01) == 0x01 ? 0:XTAL_ENABLE(1))
#define OSC_DISABLE(x)                      ((x&0x01) == 0x01 ? 0:OSC_ENABLE(1))

/* retention memory 0x50091018*/
/* retention_bank_control */
#define IO_RETENTION_FDIO                   (0x08)
#define IO_RETENTION_DIO2                   (0x04)
#define IO_RETENTION_DIO1                   (0x02)
#define IO_RETENTION_SEN_VDD                (0x01)
#define IO_RETENTION_ENABLE(x)              ((x&0x0f) << 24)
#define PDB_RETENTION_ENABLE(x)             ((x&0x7f) << 16)
#define RETENTION_SLEEP(x)                  ((x&0x7f) << 8)
#define RETENTION_SLP_ID(x)                 ((x&0x0f) << 4)
#define RETENTION_ISO_SHARED(x)             ((x&0x01) << 2)
#define RETENTION_FLAG(x)                   ((x&0x01) << 1)
#define PDB_ISOLATION_ENABLE(x)             ((x&0x01) << 0)
//#define RETENTION_POWER(x)                ((x&0x01) << 1)
//#define PDB_CML(x)                        ((x&0x01) << 0)

/* DC power control 0x5009101c*/
#define SET_1_2_POWER_OFF(x)                (x&0x01)

/* ldo set 0x50091020*/
// [15] test only
#define DCDC_CNTL_OFF(x)                    ((x&0x01) << 14)
#define EXT_BOOST_PWR(x)                    ((x&0x01) << 13)
#define RESET_HOLD_ENABLE(x)                ((x&0x01) << 12)
#define LDO_FLASH_MANUAL_VAL(x)             ((x&0x01) << 11)
#define LDO_FLASH_MANUAL_ENABLE(x)          ((x&0x01) << 10)
#define LDO_IP1_ENABLE(x)                   ((x&0x01) << 9)
#define RF_LDO_ENABLE(x)                    ((x&0x01) << 8)
#define DIG_LDO_CNTL(x)                     ((x&0x01) << 7)
#define DCDC_CNTL_XTAL(x)                   ((x&0x01) << 6)
#define LDO_ULDO_ENABLE(x)                  ((x&0x01) << 5)
#define LDO_OTP_ENABLE(x)                   ((x&0x01) << 4)
#define OTP_PWRPRDY(x)                      ((x&0x01) << 3)
#define LDO_PLL1_ENABLE(x)                  ((x&0x01) << 2)
#define LDO_FEM_ENABLE(x)                   ((x&0x01) << 1)
#define LDO_XTAL_NOISE_REDUCTION(x)         ((x&0x01)     )

/* select signals */
// 0x50091024
#define F_DIO_ST_BYP_EN(x)                  ((x&0x01) << 14)
#define DCDC_ST_BTP(x)                      ((x&0x01) << 10)

// 0x28
#define EXT_WAKEUP_STATUS(x)                ((x&0x0f) << 8)
#define GPIO_WAKEUP_DETECT(x)               ((x&0x01) << 6)
#define PULSE_CNT_DETECT(x)                 ((x&0x01) << 5)
#define SENSOR_DETECT(x)                    ((x&0x01) << 4)


// 0x50091030
#define AO_REGISTER_RESTORE_ENABLE(x)       ((x&0x01) << 3)
#define MANUAL_BOOST_TIME_SEL_ENABLE(x)     ((x&0x01) << 2)
#define MANUAL_RESET_TIME_SEL_ENABLE(x)     ((x&0x01) << 1)
#define MANUAL_DCDC_TIME_SEL_ENABLE(x)      ((x&0x01) << 0)

// 0x34
#define SET_BOOST_TIME(x)                   ((x&0x1f) << 8)
#define SET_1_2_POWER_TIME(x)               ((x&0x0f) << 4)
#define SET_DCDC_CNTL_TIME(x)               ((x&0x0f))

// 0x50091040
#define XTAL_READY(x)                       ((x&0x01) << 3)

// 0x50091044
#define DCDC_ST_BYP(x)                      ((x&0x01) << 29)
#define DCDC_ST_CTRL(x)                     ((x&0x1f) << 24)
#define IP2_MON_PATH_CTRL(x)                ((x&0x01) << 21)
#define IP2_MON_CTRL(x)                     ((x&0x03) << 18)
#define RTC_XTAL32K_GM(x)                   ((x&0x03) << 16)
#define RTC_OSC32K_ICTRL(x)                 ((x&0x03) << 14)
#define RTC_XTAL32K_ICTRL(x)                ((x&0x03) << 12)
#define RTC_ULDO_LICTRL(x)                  ((x&0x03) << 10)
#define RTC_ULDO_HICTRL(x)                  ((x&0x03) << 8)
#define RTC_ULDO_VCTRL(x)                   ((x&0x0f) << 4)
#define RTC_TEST_BUF(x)                     ((x&0x01) << 1)
#define RTC_CLK_INVERSION(x)                ((x&0x01)     )

#define RF_LO_CAL_ENABLE(x)                 ((x&0x01) << 10)
#define BROWN_CIR_ENABLE(x)                 ((x&0x01) << 9)
#define BLACK_CIR_ENABLE(x)                 ((x&0x01) << 8)

// 0x50091050
#define COUNTER_CLEAR(x)                    ((x&0x01) << 0)

// 8k * 6ea = 48kbyte
#define RETENTION_MEM_BANK0                 0x01        // 0x00180000 ~ 0x00181fff
#define RETENTION_MEM_BANK1                 0x02        // 0x00182000 ~
#define RETENTION_MEM_BANK2                 0x04        // 0x00184000 ~
#define RETENTION_MEM_BANK3                 0x08        // 0x00186000 ~
#define RETENTION_MEM_BANK4                 0x10        // 0x00188000 ~
#define RETENTION_MEM_BANK5                 0x20        // 0x0018A000 ~
#define RETENTION_MEM_BANK6                 0x40        // AO AREA ~  /************/
#define RETENTION_MEM_BANK_ALL              0x7f        // all

//
//    Register MAP
//

typedef enum {
    RTC_GET_DEVREG =                1,
    RTC_SET_RTC_CONTROL_REG         ,
    RTC_GET_RTC_CONTROL_REG         ,
    RTC_SET_XTAL_CONTROL_REG        ,
    RTC_GET_XTAL_CONTROL_REG        ,

    RTC_SET_RETENTION_CONTROL_REG   ,
    RTC_GET_RETENTION_CONTROL_REG   ,

    RTC_SET_DC_PWR_CONTROL_REG      ,
    RTC_GET_DC_PWR_CONTROL_REG      ,

    RTC_SET_LDO_CONTROL_REG         ,
    RTC_GET_LDO_CONTROL_REG         ,

    RTC_GET_SIGNALS_REG             ,

    RTC_SET_WAKEUP_SOURCE_REG       ,
    RTC_GET_WAKEUP_SOURCE_REG       ,
    RTC_SET_TIME_DELEY_EN_REG       ,
    RTC_GET_TIME_DELEY_EN_REG       ,
    RTC_SET_TIME_DELEY_CNT_REG      ,
    RTC_GET_TIME_DELEY_CNT_REG      ,

    RTC_GET_LDO_STATUS              ,
    RTC_SET_LDO_LEVEL_REG           ,
    RTC_GET_LDO_LEVEL_REG           ,

    RTC_SET_BOR_CIRCUIT             ,
    RTC_GET_BOR_CIRCUIT             ,

    RTC_GET_WAKEUP_COUNTER_REG      ,
    RTC_GET_COUNTER_REG             ,
    RTC_GET_WAKEUP_SOURCE_SIG_REG   ,

    RTC_SET_GPIO_WAKEUP_CONFIG_REG  ,
    RTC_GET_GPIO_WAKEUP_CONFIG_REG  ,

    RTC_SET_GPIO_WAKEUP_CONTROL_REG ,
    RTC_GET_GPIO_WAKEUP_CONTROL_REG ,

    RTC_IOCTL_MAX
}RTC_DEVICE_IOCTL_LIST;

/**
 * \brief RTC register map
 *
 *
 */
typedef struct    __rtc_reg_map__
{
    volatile UINT32 wakeup_counter0;        /**<[31:0]                        0x00 */
    volatile UINT32 wakeup_counter1;        /**<[35:32]                       0x04 */
    volatile UINT32 gpio_wakeup_config;     /**<gpio interrupt setting        0x08 */
    volatile UINT32 gpio_wakeup_control;    /**<gpio interrupt status         0x0c */
    volatile UINT32 rtc_control;            /**<control register              0x10 */
    volatile UINT32 xtal_control;           /**<32k xtal control              0x14 */
    volatile UINT32 retention_control;      /**<retention control             0x18 */
    volatile UINT32 dc_power_control;       /**<DC power control              0x1c */
    volatile UINT32 ldo_control;            /**<ldo control register          0x20 */
    volatile UINT32 select_signals;         /**<signal monitor                0x24 */
    volatile UINT32 wakeup_source;          /**<wakeup source                 0x28 */
    volatile UINT32 odfe_cfg;               /**<odfe_cfg (FPGA Only)          0x2c */
    volatile UINT32 time_deley_en;          /**<ldo auto en time scale sel    0x30 */
    volatile UINT32 time_delay_value;       /**<xtal, pll1, pll2, dio1, aio1  0x34 */
    volatile UINT32 counter0;               /**<counter [31:0]                0x38 */
    volatile UINT32 counter1;               /**<counter [35:32]               0x3c */
    volatile UINT32 ldo_status;             /**<read register                 0x40 */
    volatile UINT32 ldo_pwr_control;        /**<ldo level control             0x44 */
    volatile UINT32 reserved3;              /**<reserved                      0x48 */
    volatile UINT32 bor_circuit;            /**<Brown & Black out circuit     0x4c */
    volatile UINT32 ao_control;             /**<ao control                    0x50 */
    volatile UINT32 toggle_cnt;             /**<[1:0] toggle counter          0x54 */
    volatile UINT32 wakeup_source_read;     /**<wakeup source read            0x58 */
    volatile UINT32 watchdog_cnt;           /**<watchdog period set           0x5c */
    volatile UINT32 auxadc_control;         /**<aux_adc_control               0x60 */
    volatile UINT32 auxadc_threshold_lev0_1;/**<aux_adc_threshold_level       0x64 */
    volatile UINT32 auxadc_threshold_lev2_3;/**<aux_adc_sample number         0x68 */
    volatile UINT32 auxadc_sample_num;      /**<aux_adc_sample number         0x6c */
    volatile UINT32 auxadc_timer;           /**<aux_adc_timer                 0x70 */
    volatile UINT32 pulse_control;          /**<pulse counter                 0x74 */
    volatile UINT32 pulse_int_thr;          /**<pulse counter                 0x78 */
    volatile UINT32 pulse_cnt_reg;          /**<pulse counter                 0x7c */
} RTC_REG_MAP;


//
//    Driver Structure
//
typedef struct    __rtc_device_handler__
{
    UINT32            dev_addr;                      // Unique Address
    // Device-dependant
    RTC_REG_MAP        *regmap;
} RTC_HANDLER;

/**
 * \brief RTC LDO control param
 *
 *
 */
typedef struct __RTC_AUTO_LDO_EN_PARAM__
{
    UINT8            boost_on_cnt;          /**<boost on count*/
    UINT8            core_reset_cnt;        /**<core reset count*/
    UINT8            dcdc_cntl_cnt;         /**<dcdc count*/
    UINT8            boost_en;              /**<boost control enable*/
    UINT8            core_reset_en;         /**<core reset enable */
    UINT8            dcdc_cntl_en;          /**<dcdc enable */
} RTC_AUTO_LDO_EN_PARAM;

/**
 * \brief RTC retention LDO control
 *
 *
 */
typedef struct __RTC_RETENTION_PARAM__
{
    UINT8                xtal32k_gm;        /**<xtal */
    UINT8                osc32k_ictrl;      /**<osc*/
    UINT8                xtal32k_ictrl;     /**<xtal32k ictrl*/
    UINT8                uldo_lictrl;       /**<uLDO low ctrl*/
    UINT8                uldo_hictrl;       /**<uLDO high ctrl*/
}RTC_RETENTION_PARAM;

//---------------------------------------------------------
//    DRIVER :: APP-Interface
//---------------------------------------------------------
/**
 ****************************************************************************************
 * @brief Get which wakeup pin occur.
 * @return wakeup pin.
 * @note The DA16200 have wakeup pins. (2 wakeup pins in 6*6, 4 wakeup pins in 8*8) <br>
 * This function call after RTC_GET_WAKEUP_SOURCE() <br>
 * example <br>
 * @code{.c}
 * wakeup = RTC_GET_WAKEUP_SOURCE();
 * if (wakeup == WAKEUP_SOURCE_EXT_SIGNAL)
 *	{
 *		UINT32 wakeup_pin;
 *		wakeup_pin = RTC_GET_EXT_WAKEUP_SOURCE();
 *		PRINTF("wakeup #%d pin\n", (wakeup_pin));
 *	}
 * @endcode
 * @sa   RTC_GET_WAKEUP_SOURCE
 ****************************************************************************************
 */
extern UINT32 RTC_GET_EXT_WAKEUP_SOURCE(void);

/**
 ****************************************************************************************
 * @brief Get aux wakeup source.
 * @return Aux wakeup source.
 * @note The aux wakeup source include GPIO, ADC sensor, and pulse wakeup <br>
 * This function call after RTC_GET_WAKEUP_SOURCE() <br>
 * example GPIO wakeup <br>
 * @code{.c}
     wakeup = RTC_GET_WAKEUP_SOURCE();
     if (wakeup == WAKEUP_SENSOR)
 	{
 		UINT32 source;
 		source = RTC_GET_AUX_WAKEUP_SOURCE();
        PRINTF("aux wakeup source %x\n", source);
        if ((source & WAKEUP_GPIO) == WAKEUP_GPIO)    // GPIO wakeup
        {
            gpio_source = RTC_GET_GPIO_SOURCE();
            PRINTF("aux gpio wakeup %x\n", gpio_source);
            RTC_CLEAR_EXT_SIGNAL();
            RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            gpio_source |= (0x3ff);
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            //check zero
            for (i = 0; i < 10; i++)
            {
                RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
                if ((gpio_source &0x3ff) == 0x00)
                    break;
                else
                    PRINTF("gpio wakeup clear %x\n", gpio_source);
            }
            gpio_source &= ~(0x3ff);
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
            for (i = 0; i < 10; i++)
            {
                RTC_IOCTL(RTC_GET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
                if ((gpio_source &0x3ff) == 0x00)
                    break;
                else
                    PRINTF("gpio wakeup clear %x\n", gpio_source);
            }
            RTC_IOCTL(RTC_SET_GPIO_WAKEUP_CONTROL_REG, &gpio_source);
        }
 	}
  @endcode
  @sa RTC_GET_WAKEUP_SOURCE
  @sa WAKEUP_SOURCE
 ****************************************************************************************
 */
extern UINT32 RTC_GET_AUX_WAKEUP_SOURCE(void);

/**
 ****************************************************************************************
 * @brief Get which GPIO pin occur.
 * @return wakeup GPIO pin.
 * @note The DA16200 can be wakeup by GPIO.<br>
 * This function call after RTC_GET_WAKEUP_SOURCE() <br>
 * @sa RTC_GET_AUX_WAKEUP_SOURCE()
 ****************************************************************************************
 */
extern UINT32 RTC_GET_GPIO_SOURCE(void);

/**
 ****************************************************************************************
 * @brief Get wakeup source
 * @return WAKEUP_SOURCE
 ****************************************************************************************
 */
extern WAKEUP_SOURCE RTC_GET_WAKEUP_SOURCE(void);

/**
 ****************************************************************************************
 * @brief Do power down ready.
 * @param[in] clear wakeup source to clear
 * @note This function must be called 35us before RTC_INTO_POWER_DOWN() <br>
 * Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_READY_POWER_DOWN( int clear );

/**
 ****************************************************************************************
 * @brief Clear wakeup pin status
 ****************************************************************************************
 */
extern void RTC_CLEAR_EXT_SIGNAL( void );

/**
 ****************************************************************************************
 * @brief Into power down status
 * @param[in] flag wakeup flag, it is used for DPM
 * @param[in] period sleep period unit(RTC clock at 32kHz)
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_INTO_POWER_DOWN( UINT32 flag, unsigned long long period );

/**
 ****************************************************************************************
 * @brief Set the retention flag <br>
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_SET_RETENTION_FLAG(void);

/**
 ****************************************************************************************
 * @brief Get the flag about retention memory.
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern int RTC_GET_RETENTION_FLAG(void);

/**
 ****************************************************************************************
 * @brief Clear the retention flag
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_CLEAR_RETENTION_FLAG(void);

/**
 ****************************************************************************************
 * @brief retention memory power on
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_RETENTION_PWR_ON(void);

/**
 ****************************************************************************************
 * @brief retention memory power off
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_RETENTION_PWR_OFF(void);

/**
 ****************************************************************************************
 * @brief enable access retention memory
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_RETENTION_OPEN(void);

/**
 ****************************************************************************************
 * @brief keep the data during main power down
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_RETENTION_CLOSE(void);

/**
 ****************************************************************************************
 * @brief Turn on the DC 1.2V
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_DC12_ON(void);

/**
 ****************************************************************************************
 * @brief Turn off the DC 1.2V
 * @note Normally DPM manager calls this function. no need to call this function manauly
 ****************************************************************************************
 */
extern void RTC_DC12_OFF(void);

/**
 ****************************************************************************************
 * @brief Get the real time counter 32kHz
 * @note bit field is [35:0]
 ****************************************************************************************
 */
extern unsigned long long RTC_GET_COUNTER(void);

/**
 ****************************************************************************************
 * @brief Power on the extenal XTAL
 ****************************************************************************************
 */
extern void RTC_XTAL_ON(void);

/**
 ****************************************************************************************
 * @brief Power off the extenal XTAL
 ****************************************************************************************
 */
extern void RTC_XTAL_OFF(void);

/**
 ****************************************************************************************
 * @brief Power on internal OSC
 ****************************************************************************************
 */
extern void RTC_OSC_ON(void);

/**
 ****************************************************************************************
 * @brief Power off internal OSC
 ****************************************************************************************
 */
extern void RTC_OSC_OFF(void);

/**
 ****************************************************************************************
 * @brief Check if external XTAL ready
 * @return XTAL_READY(1) if the external XTAL ready, else 0
 * @note The external XTAL need time to stabilize
 ****************************************************************************************
 */
extern UINT32 RTC_GET_XTAL_READY(void);

/**
 ****************************************************************************************
 * @brief Check the current RTC source.
 * @return XTAL_TYPE
 * @sa XTAL_TYPE
 ****************************************************************************************
 */
extern XTAL_TYPE RTC_GET_CUR_XTAL(void);

/**
 ****************************************************************************************
 * @brief Switch the source clock of the RTC
 * @param [in] type XTAL_TYPE between internal OSC or external XTAL
 * @return XTAL_TYPE the source clock of the RTC
 * @sa XTAL_TYPE
 ****************************************************************************************
 */
extern XTAL_TYPE RTC_SWITCH_XTAL(XTAL_TYPE type);

/**
 ****************************************************************************************
 * @brief Set the LDO enable time of each LDO
 * @param [in] param RTC_AUTO_LDO_EN_PARAM
 * @note
 * example <br>
 * @code{.c}
	{
		RTC_AUTO_LDO_EN_PARAM param_test, ret_param;
		param_test.core_reset_cnt= 3;
		param_test.dcdc_cntl_cnt = 4;

		param_test.core_reset_en = 1;
		param_test_dcdc_cntl_en = 1;

		RTC_SET_AUTO_LDO(param_test);
		ret_param = RTC_GET_AUTO_LDO();

		PRINTF("%x, %x\n", ret_param.core_reset_cnt, ret_param.dcdc_cntl_cnt);
	}
 * @endcode
 * @sa RTC_AUTO_LDO_EN_PARAM
 * @sa RTC_GET_AUTO_LDO
 ****************************************************************************************
 */
extern void RTC_SET_AUTO_LDO(RTC_AUTO_LDO_EN_PARAM param);

/**
 ****************************************************************************************
 * @brief Get the LDO enable time of each LDO
 * @return RTC_AUTO_LDO_EN_PARAM
 * @sa RTC_AUTO_LDO_EN_PARAM
 * @sa RTC_GET_AUTO_LDO
 ****************************************************************************************
 */
extern RTC_AUTO_LDO_EN_PARAM RTC_GET_AUTO_LDO(void);

extern UINT32 RTC_SET_RETENTION_BANK(UINT32 bank);
extern UINT32 RTC_GET_RETENTION_BANK(void);

extern UINT32 RTC_SET_RETENTION_SLP_VOLTAGE(UINT32 voltage);
extern UINT32 RTC_GET_RETENTION_SLP_VOLTAGE(void);

extern void RTC_SET_RETENTION_SLP_PARAM(RTC_RETENTION_PARAM param);
extern RTC_RETENTION_PARAM RTC_GET_RETENTION_SLP_PARAM(void);
extern void RTC_SET_RETENTION_PWR_PARAM(RTC_RETENTION_PARAM param);
extern RTC_RETENTION_PARAM RTC_GET_RETENTION_PWR_PARAM(void);

///*set the watchdog period  2^bitpos < period 2*2^bitpos*/
extern UINT32 RTC_SET_WATCHDOG_PERIOD(UINT32 bitpos);
extern void RTC_WATCHDOG_EN(void);
extern void RTC_WATCHDOG_DIS(void);

extern int RTC_IOCTL(UINT32 cmd , VOID *data );

/**
 ****************************************************************************************
 * @brief Clear the RTC counter
 ****************************************************************************************
 */
extern void RTC_COUNTER_CLR(void);
#endif
