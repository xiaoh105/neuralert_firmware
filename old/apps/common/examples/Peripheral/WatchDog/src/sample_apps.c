/**
 ****************************************************************************************
 *
 * @file sample_apps.c
 *
 * @brief Config table to start sample applications.
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

#include "sdk_type.h"
#include "sample_defs.h"

#if defined (__ENABLE_SAMPLE_APP__)

#include "da16x_system.h"
#include "da16x_network_common.h"
#include "application.h"
#include "user_dpm.h"
#include "user_dpm_api.h"


extern run_app_task_info_t *create_new_app(run_app_task_info_t *prevthread,
        const app_task_info_t *cur_info,
        int sys_mode);

extern int get_run_mode(void);

/* Sample Apps Call-back functions */
extern void (*sample_app_start_cb)(unsigned char);
extern void (*sample_app_dpm_reg_cb)(void);
extern void (*sample_app_dpm_unreg_cb)(void);

/******************************************************************************
 * Sample Application Threads
 *****************************************************************************/

// WatchDog Samples
#if defined (__WATCHDOG_SAMPLE__)
extern void	WatchDog_sample(void *param);
#endif // (__WATCHDOG_SAMPLE__)

/* name, entry_func, stack_size, priority, net_chk_flag, dpm_flag, port_no, run_sys_mode */

static const app_task_info_t	sample_apps_table[] = {
    /******* Add a custom thread of tasks ****************************************/
    ////// For testing sample code ... ////////////////////////////////////////

    // WatchDog Samples
#if defined (__WATCHDOG_SAMPLE__)
    { WATCHDOG_SAMPLE,			WatchDog_sample,					512, (OS_TASK_PRIORITY_USER + 6), FALSE, FALSE,	UNDEF_PORT,				RUN_ALL_MODE },
#endif // (__WATCHDOG_SAMPLE__)

    /******* End of List *****************************************************/
    { NULL,	NULL,	0, 0, FALSE, FALSE, UNDEF_PORT, 0	}
};


void dpm_reg_sample_app(void)
{
    int	i;
    const app_task_info_t *cur_info;

    for (i = 0 ; sample_apps_table[i].name != NULL ; i++) {
        cur_info = &(sample_apps_table[i]);

        if ((cur_info->run_sys_mode & RUN_STA_MODE) && (cur_info->dpm_flag == TRUE)) {
            reg_dpm_sleep(cur_info->name, cur_info->port_no);
        }
    }
}

void dpm_unreg_sample_app(void)
{
    int	i;
    const app_task_info_t *cur_info;

    for (i = 0 ; sample_apps_table[i].name != NULL ; i++) {
        cur_info = &(sample_apps_table[i]);

        if ((cur_info->run_sys_mode & RUN_STA_MODE) && (cur_info->dpm_flag == TRUE)) {
            unreg_dpm_sleep(cur_info->name);
        }
    }
}

void create_sample_apps(unsigned char net_chk_flag)
{
    int i;
    int	sysmode;
    run_app_task_info_t  *cur_task = NULL;

    sysmode = get_run_mode();

    for (i = 0 ; sample_apps_table[i].name != NULL ; i++) {
        /* Run matched apps with net_chk_flag */
        if (sample_apps_table[i].net_chk_flag == net_chk_flag) {
            cur_task = create_new_app(cur_task, &(sample_apps_table[i]), sysmode);
        }
    }
}

void register_sample_cb(void)
{
    sample_app_start_cb = create_sample_apps;
    sample_app_dpm_reg_cb = dpm_reg_sample_app;
    sample_app_dpm_unreg_cb = dpm_unreg_sample_app;
}
#endif // (__ENABLE_SAMPLE_APP__)

/* EOF */
