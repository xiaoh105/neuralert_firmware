/**
 ****************************************************************************************
 * @addtogroup Drivers
 * @{
 * @addtogroup Timers
 * @{
 * @addtogroup Timer0
 * @brief Timer0 driver API
 * @{
 *
 * @file timer0.h
 *
 * @brief DA14585/DA14586/DA14531 Timer0 driver header file.
 *
 * Copyright (C) 2016-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _TIMER0_H_
#define _TIMER0_H_

/// Timer0 PWM mode
typedef enum
{
    /// PWM signals are '1' during high time
    PWM_MODE_ONE              = 0,

    /// PWM signals send out the (fast) clock divided by 2 during high time
    PWM_MODE_CLOCK_DIV_BY_TWO = 1
} PWM_MODE_t;

/// Timer0 clock div (applies to the ON-counter only)
typedef enum
{
    /// Timer0 uses selected clock frequency as is
    TIM0_CLK_DIV_BY_10 = 0,

    /// Timer0 uses selected clock frequency divided by 10
    TIM0_CLK_NO_DIV    = 1
} TIM0_CLK_DIV_t;

/// Timer0 clock source
typedef enum
{
    /// Timer0 uses the LP clock
    TIM0_CLK_32K  = 0,

    /// Timer0 uses the system clock
    TIM0_CLK_FAST = 1
} TIM0_CLK_SEL_t;

/// Timer0 off/on
typedef enum
{
    /// Timer0 is off and in reset state
    TIM0_CTRL_OFF_RESET = 0,

    /// Timer0 is running
    TIM0_CTRL_RUNNING   = 1
} TIM0_CTRL_t;

#endif /* _TIMER0_H_ */

///@}
///@}
///@}
