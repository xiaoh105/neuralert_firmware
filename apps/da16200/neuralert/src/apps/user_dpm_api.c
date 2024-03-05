/**
 ****************************************************************************************
 *
 * @file user_dpm_api.c
 *
 * @brief User APIs for DPM operation
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

#include "user_dpm_api.h"

#define __WAIT_ENTER_SLEEP_MODE__

void dpm_mode_enable(void)
{
    enable_dpm_mode();
    return ;
}

void dpm_mode_disable(void)
{
    disable_dpm_mode();
    return ;
}

int dpm_mode_is_enabled(void)
{
    return chk_dpm_mode();
}

int dpm_mode_is_wakeup(void)
{
    return chk_dpm_wakeup();
}

int dpm_mode_get_wakeup_source(void)
{
    extern int dpm_get_wakeup_source(void);

    return dpm_get_wakeup_source();
}

int dpm_mode_get_wakeup_type(void)
{
    extern int da16x_get_dpm_wakeup_type(void);

    return da16x_get_dpm_wakeup_type();
}

int dpm_sleep_is_started(void)
{
    return chk_dpm_pdown_start();
}

int dpm_sleep_start_mode_2(unsigned long long usec, unsigned char retention)
{
#ifdef __WAIT_ENTER_SLEEP_MODE__
    #define DELAY_TICKS 1

    if ( RTC_GET_CUR_XTAL() != EXTERNAL_XTAL ) {
        INT32 timeout_ticks = SCHD_SYS_RTCSW_MAX_NUM * SCHD_SYS_RTCSW_TICKS;

        while(timeout_ticks > 0) {
            if ( RTC_GET_CUR_XTAL() == EXTERNAL_XTAL ) {
              //PRINTF("exit to wait for switching xal\r\n");
              //vTaskDelay(1);
              break;
            }

            vTaskDelay(DELAY_TICKS);
            timeout_ticks -= DELAY_TICKS;
        }
    }
#endif

    return start_dpm_sleep_mode_2(usec, retention);
}

int dpm_sleep_start_mode_3(unsigned long long usec)
{
#ifdef __WAIT_ENTER_SLEEP_MODE__
    #define DELAY_TICKS 1

    if ( RTC_GET_CUR_XTAL() != EXTERNAL_XTAL ) {
        INT32 timeout_ticks = SCHD_SYS_RTCSW_MAX_NUM * SCHD_SYS_RTCSW_TICKS;

        while(timeout_ticks > 0) {
            if ( RTC_GET_CUR_XTAL() == EXTERNAL_XTAL ) {
              //PRINTF("exit to wait for switching xal\r\n");
              //vTaskDelay(1);
              break;
            }

            vTaskDelay(DELAY_TICKS);
            timeout_ticks -= DELAY_TICKS;
        }
    }
#endif

    return start_dpm_sleep_mode_2(usec, 1);
}

int dpm_app_register(char *mod_name, unsigned int port_number)
{    
    return reg_dpm_sleep(mod_name, port_number);
}

void dpm_app_unregister(char *mod_name)
{
    unreg_dpm_sleep(mod_name);
    return ;
}

int dpm_app_sleep_ready_set(char *mod_name)
{
    return set_dpm_sleep(mod_name);
}

int dpm_app_sleep_ready_clear(char *mod_name)
{
    return clr_dpm_sleep(mod_name);
}

int dpm_app_wakeup_done(char *mod_name)
{
    return set_dpm_init_done(mod_name);
}

int dpm_app_data_rcv_ready_set(char *mod_name)
{
    return set_dpm_rcv_ready(mod_name);
}

int dpm_app_rcv_ready_set_by_port(unsigned int port)
{
    return set_dpm_rcv_ready_by_port(port);
}

int dpm_user_mem_init_check(void)
{
    return dpm_user_rtm_pool_init_chk();
}

unsigned int dpm_user_mem_alloc(char *name, void **memory_ptr, unsigned long memory_size, unsigned long wait_option)
{
    return dpm_user_rtm_allocate(name, memory_ptr, memory_size, wait_option);
}

unsigned int dpm_user_mem_free(char *name)
{
    return dpm_user_rtm_release(name);
}

unsigned int dpm_user_mem_get(char *name, unsigned char **data)
{
    return dpm_user_rtm_get(name, data);
}

int dpm_timer_create(char *task_name, char *timer_name,
                void (* callback_func)(char *timer_name),
                unsigned int msec, unsigned int reschedule_msec)
{
    return dpm_user_timer_create(task_name, timer_name, callback_func, msec, reschedule_msec);
}

int dpm_timer_delete(char *task_name, char *timer_name)
{
    return dpm_user_timer_delete(task_name, timer_name);
}

int dpm_timer_change(char *task_name, char *timer_name, unsigned int msec)
{
    return dpm_user_timer_change(task_name, timer_name, msec);
}

void dpm_udp_filter_enable(unsigned char en_flag)
{
    dpm_udpf_cntrl(en_flag);
    return ;
}

void dpm_udp_port_filter_set(unsigned short d_port)
{
    set_dpm_udp_port_filter(d_port);
    return ;
}

void dpm_udp_port_filter_delete(unsigned short d_port)
{
    del_dpm_udp_port_filter(d_port);
    return ;
}

void dpm_udp_hole_punch_set(int period, unsigned long dest_ip, unsigned short src_port, unsigned short dest_port)
{
    dpm_udp_hole_punch(period, dest_ip, src_port, dest_port);
    return; 
}

void dpm_tcp_filter_enable(unsigned char en_flag)
{
    dpm_tcpf_cntrl(en_flag);
    return ;
}

void dpm_tcp_port_filter_set(unsigned short d_port)
{
    set_dpm_tcp_port_filter(d_port);
    return ;
}

void dpm_tcp_port_filter_delete(unsigned short d_port)
{
    del_dpm_tcp_port_filter(d_port);
    return ;
}

void dpm_arp_enable(int period, int mode)
{
    dpm_arp_en(period, mode);
    return ;
}

// New APIs
int dpm_mode_get(void)
{
    return get_dpm_mode();
}

int dpm_app_is_register(char *mod_name)
{
    return chk_dpm_reg_state(mod_name);
}

int dpm_app_is_sleep_ready_set(char *mod_name)
{
    return chk_dpm_set_state(mod_name);
}

char *dpm_app_is_register_port(unsigned int port)
{
    return chk_dpm_reg_port(port);
}

void dpm_tim_wakeup_duration_set(unsigned int dtim_period, int flag)
{
    set_dpm_tim_wakeup_dur(dtim_period, flag);
    return ;
}

void dpm_mc_filter_set(unsigned long mc_addr)
{
    set_dpm_mc_filter(mc_addr);
}

int dpm_timer_remaining_msec_get(char *thread_name, char *timer_name)
{
    return dpm_user_timer_get_remaining_msec(thread_name, timer_name);
}

bool dpm_app_is_wakeup_done(char *mod_name)
{
    return confirm_dpm_init_done(mod_name);
}

unsigned char dpm_mode_is_abnormal_wakeup(void)
{
    return get_last_dpm_sleep_type();
}

/* EOF */
