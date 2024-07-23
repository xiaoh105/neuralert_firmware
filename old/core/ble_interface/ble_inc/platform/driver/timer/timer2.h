/**
 ****************************************************************************************
 * @addtogroup Drivers
 * @{
 * @addtogroup Timers
 * @{
 * @addtogroup Timer2
 * @brief Timer2 driver API
 * @{
 *
 * @file timer2.h
 *
 * @brief DA14585/DA14586/DA14531 Timer2 driver header file.
 *
 * @note The Timer0/Timer2 input clock shall have been enabled first.
 *
 * Copyright (C) 2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _TIMER2_H_
#define _TIMER2_H_

#define __DA14531__

#if defined (__DA14531__)
/// Timer2 clock source
typedef enum
{
    /// Timer2 uses the LP clock
    TIM2_CLK_LP  = 0,

    /// Timer2 uses the system clock
    TIM2_CLK_SYS = 1
} tim2_clk_src_t;
#endif

/// Timer2 pause (by HW)
typedef enum
{
    /// HW cannot pause Timer2
    TIM2_HW_PAUSE_OFF = 0,

    /// HW can pause Timer2
    TIM2_HW_PAUSE_ON  = 1
} tim2_hw_pause_t;

/// Timer2 pause (by SW)
typedef enum
{
    /// SW resumes Timer2 run
    TIM2_SW_PAUSE_OFF = 0,

    /// SW pauses Timer2 run
    TIM2_SW_PAUSE_ON  = 1
} tim2_sw_pause_t;

/// Timer2 disable/enable
typedef enum
{
    /// Timer2 disabled
    TIM2_OFF = 0,

    /// Timer2 enabled
    TIM2_ON  = 1
} tim2_t;

/// Timer2 PWM signals
typedef enum
{
    /// Timer2 PWM2 signal
    TIM2_PWM_2 = 1,

    /// Timer2 PWM3 signal
    TIM2_PWM_3 = 2,

    /// Timer2 PWM4 signal
    TIM2_PWM_4 = 3,

#if defined (__DA14531__)
    /// Timer2 PWM5 signal
    TIM2_PWM_5 = 4,

    /// Timer2 PWM6 signal
    TIM2_PWM_6 = 5,

    /// Timer2 PWM7 signal
    TIM2_PWM_7 = 6
#endif
} tim2_pwm_t;

/// Timer2 configuration
typedef struct
{
#if defined (__DA14531__)
    /// Timer2 input clock frequency
    tim2_clk_src_t clk_source;
#endif

    /// Timer2 pause (by HW)
    tim2_hw_pause_t hw_pause;
} tim2_config_t;

/// Timer2 PWM signal configuration
typedef struct
{
    /// PWM signal to be configured
    tim2_pwm_t pwm_signal;

    /// Duty cycle of the PWM signal (0% - 100%)
    uint8_t pwm_dc;

#if defined (__DA14531__)
    /// Offset of the PWM signal (0% - 100%)
    uint8_t pwm_offset;
#endif
} tim2_pwm_config_t;

#endif /* _TIMER2_H_ */

///@}
///@}
///@}
