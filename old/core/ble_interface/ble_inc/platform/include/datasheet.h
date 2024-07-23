/**
 ****************************************************************************************
 *
 * @file datasheet.h
 *
 * @brief Register definitions header file.
 *
 * Copyright (C) 2018-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _DATASHEET_H_
#define _DATASHEET_H_

#if defined (__DA14531__)
    #include "../../platform/include/da14531.h"
    #include "core_cm0plus.h"
    #include "../../platform/include/system_DA14531.h"
#elif defined (__DA14585__) || defined(__DA14586__)
    #include "../../platform/include/da14585_586.h"
    #include "../../platform/include/CMSIS/5.6.0/Include/core_cm0.h"
    #include "../../platform/include/system_DA14585_586.h"
#endif

#endif
