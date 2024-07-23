/**
 * Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright (c) 2021-2022 Modified by Renesas Electronics.
 *
**/
#ifndef _WS_LOG_H_
#define _WS_LOG_H_

#include "da16x_system.h"


#define	LIGHT_RED_COL		"\33[0;31m"
#define	LIGHT_GRN_COL		"\33[0;32m"
#define	LIGHT_YEL_COL		"\33[0;33m"
#define	LIGHT_BLU_COL		"\33[0;34m"
#define	LIGHT_MAG_COL		"\33[0;35m"
#define	LIGHT_CYN_COL		"\33[0;36m"
#define	LIGHT_WHT_COL		"\33[0;37m"

#define	RED_COL			"\33[1;31m"
#define	GRN_COL			"\33[1;32m"
#define	YEL_COL			"\33[1;33m"
#define	BLU_COL			"\33[1;34m"
#define	MAG_COL			"\33[1;35m"
#define	CYN_COL			"\33[1;36m"
#define	WHT_COL			"\33[1;37m"

#define	CLR_COL			"\33[0m"


#define WS_DEBUG		0

#if WS_DEBUG
#define WS_LOGD(TAG, ...) PRINTF(CLR_COL __VA_ARGS__)
#define WS_LOGV(TAG, ...) PRINTF(CLR_COL __VA_ARGS__)
#define WS_LOGE(TAG, ...) PRINTF(LIGHT_RED_COL __VA_ARGS__); PRINTF(CLR_COL);
#define WS_LOGI(TAG, ...) PRINTF(CLR_COL __VA_ARGS__)
#define WS_LOGW(TAG, ...) PRINTF(LIGHT_YEL_COL __VA_ARGS__); PRINTF(CLR_COL);
#else
#define WS_LOGD(TAG, ...)
#define WS_LOGV(TAG, ...)
#define WS_LOGE(TAG, ...) PRINTF(LIGHT_RED_COL __VA_ARGS__); PRINTF(CLR_COL);
#define WS_LOGI(TAG, ...) PRINTF(CLR_COL __VA_ARGS__)
#define WS_LOGW(TAG, ...) PRINTF(LIGHT_YEL_COL __VA_ARGS__); PRINTF(CLR_COL);
#endif

#endif	/*_WS_ERR_H_*/
