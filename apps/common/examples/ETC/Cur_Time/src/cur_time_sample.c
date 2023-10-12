/**
 ****************************************************************************************
 *
 * @file cur_time_sample.c
 *
 * @brief Sample app of current time.
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


#if defined (__CUR_TIME_SAMPLE__)

#include "da16x_system.h"
#include "da16x_time.h"
#include "da16x_sntp_client.h"

/*
 * Defines for sample
 */
#define	TEST_SNTP_SERVER		"time.windows.com"
#define	TEST_SNTP_RENEW_PERIOD	600
#define	TEST_TIME_ZONE			(9 * 3600) 	// seconds
#define	SNTP_ENABLE				1

#define	ONE_SECONDS				100
#define	CUR_TIME_LOOP_DELAY		10	// seconds

/*
 * External global functions
 */
extern void	da16x_time64(__time64_t *p, __time64_t *now);
extern struct tm *da16x_localtime64(const __time64_t *tod);
extern size_t	da16x_strftime(char *s, size_t maxsize, const char *format,
                               const struct tm *t);

/* For SNTP Client */
static unsigned char set_n_start_SNTP(void)
{
    unsigned char	status = pdPASS;

    /* Check current SNTP running status */
    status = get_sntp_use();
    if (status == pdPASS) {
        /* Already SNTP module running ... */
        return pdPASS;
    }

    /* Config and save SNTP server URI */
    status = set_sntp_server((unsigned char *)TEST_SNTP_SERVER, 0);
    if (status != pdPASS) {
        PRINTF("[%s] Failed to write nvram operation (SNTP server domain)...\n", __func__);
        status = pdFAIL;
        goto _exit;
    }

    /* Config and save SNTP periodic renew time : seconds */
    status = set_sntp_period(TEST_SNTP_RENEW_PERIOD);

    if (status != pdPASS) {
        PRINTF("[%s] Failed to write nvram operation (SNTP renew period)...\n", __func__);
        status = pdFAIL;
        goto _exit;
    }

    /* Config and save SNTP time zone */
    set_time_zone(TEST_TIME_ZONE);
    da16x_SetTzoff(TEST_TIME_ZONE);

    /* Config, save, and run SNTP client */
    if (set_sntp_use(SNTP_ENABLE) != 0) {
        PRINTF("[%s] Failed to run SNTP...\n", __func__);
        status = pdFAIL;
        goto _exit;
    }

_exit :
    return status;
}

/*
 * Sample code for getting current time
 */
void cur_time_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    unsigned char 	status;
    __time64_t	now;
    struct	tm *ts;
    char	time_buf[80];

    /* Config SNTP client */
    status = set_n_start_SNTP();
    if (status == pdFAIL) {
        PRINTF("[%s] Faile to start SNTP client ...\n", __func__);
        vTaskDelete(NULL);
        return;
    }

    /* Get current time periodically */
    while (1) {
        /* delay */
        vTaskDelay(CUR_TIME_LOOP_DELAY * ONE_SECONDS);

        /* get current time */
        da16x_time64(NULL, &now);
        ts = (struct tm *)da16x_localtime64(&now);

        /* make time string */
        da16x_strftime(time_buf, sizeof(time_buf), "%Y.%m.%d %H:%M:%S", ts);

        /* display current time string */
        PRINTF("- Current Time : %s (GMT %+02ld:%02ld)\n",
               time_buf,
               da16x_Tzoff() / 3600,
               da16x_Tzoff() % 3600);
    }

}
#endif	// (__CUR_TIME_SAMPLE__)

/* EOF */
