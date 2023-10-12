/**
 * @file
 * RTM API
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Copyright (c) 2019-2022 Modified by Renesas Electronics
 */

#ifndef DA16X_DPM_RTM_MEM_H 
#define DA16X_DPM_RTM_MEM_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
#undef DA16X_DPM_RTM_MEM_SANITY_CHECK

typedef u16_t da16x_dpm_rtm_mem_size_t;

#if DA16X_DPM_RTM_MEM_OVERFLOW_CHECK
void da16x_dpm_rtm_mem_overflow_init_raw(void *p, size_t size);
void da16x_dpm_rtm_mem_overflow_check_raw(void *p, size_t size, const char *descr1, const char *descr2);
#endif /* DA16X_DPM_RTM_MEM_OVERFLOW_CHECK */

void  da16x_dpm_rtm_mem_init(void *mem_ptr, size_t mem_len);
void  da16x_dpm_rtm_mem_recovery(void *mem_ptr, size_t mem_len, void *lfree_ptr);
void *da16x_dpm_rtm_mem_get_free_ptr();
void *da16x_dpm_rtm_mem_trim(void *mem, da16x_dpm_rtm_mem_size_t size);
void *da16x_dpm_rtm_mem_malloc(da16x_dpm_rtm_mem_size_t size);
void *da16x_dpm_rtm_mem_calloc(da16x_dpm_rtm_mem_size_t count, da16x_dpm_rtm_mem_size_t size);
void  da16x_dpm_rtm_mem_free(void *mem);
void  da16x_dpm_rtm_mem_status();

#ifdef __cplusplus
}
#endif

#endif /* DA16X_DPM_RTM_MEM_H */
