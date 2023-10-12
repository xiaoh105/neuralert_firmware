/**
 ****************************************************************************************
 * @addtogroup Drivers
 * @{
 * @addtogroup Timers
 * @{
 * @addtogroup Timer0_2 Timer0/2
 * @brief Timer0/2 driver API - common part
 * @{
 *
 * @file timer0_2.h
 *
 * @brief DA14585/DA14586/DA14531 Timer0/Timer2 driver header file - common part.
 *
 * Copyright (C) 2016-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _TIMER0_2_H_
#define _TIMER0_2_H_

//#include "datasheet.h"

/// Timer0/Timer2 input clock division factor (provided that system clock is the input clock)
typedef enum
{
    /// System clock divided by 1
    TIM0_2_CLK_DIV_1 = 0,

    /// System clock divided by 2
    TIM0_2_CLK_DIV_2 = 1,

    /// System clock divided by 4
    TIM0_2_CLK_DIV_4 = 2,

    /// System clock divided by 8
    TIM0_2_CLK_DIV_8 = 3
} tim0_2_clk_div_t;

/// Timer0/Timer2 input clock disable/enable
typedef enum
{
    /// Timer0/Timer2 input clock disabled
    TIM0_2_CLK_OFF,

    /// Timer0/Timer2 input clock enabled
    TIM0_2_CLK_ON,
} tim0_2_clk_t;

/// Configuration bearing the Timer0/Timer2 input clock division factor
typedef struct
{
    /// Timer0/Timer2 input clock division factor
    tim0_2_clk_div_t clk_div;
} tim0_2_clk_div_config_t;

#endif /* _TIMER0_2_H_ */

///@}
///@}
///@}
