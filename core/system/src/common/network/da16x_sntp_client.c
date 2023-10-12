/**
 ****************************************************************************************
 *
 * @file da16x_sntp_client.c
 *
 * @brief Simple Time Protocol Client module
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

#include "lwip/apps/sntp.h"

#include "da16x_sntp_client.h"
#include "da16x_dns_client.h"

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"


/******************************************************************************
 *
 *  Definitions
 *
 ******************************************************************************/
#undef __DEBUG_SNTP_CLIENT__

/* Configure the SNTP Client to use unicast SNTP. */
#define USE_UNICAST_SNTP

extern unsigned int sntp_sync_flag;

unsigned int stop_sntp(void)
{
    if (sntp_support_flag == pdFALSE) {
        PRINTF("Doesn't support SNTP client !!!\n");
        return 1;
    }

    sntp_cmd_param * sntp_param = pvPortMalloc(sizeof(sntp_cmd_param));
    sntp_param->cmd = SNTP_STOP;
    sntp_run(sntp_param);
    return 0;
}

unsigned int start_sntp(void)
{
    if (sntp_support_flag == pdFALSE) {
        PRINTF("Doesn't support SNTP client !!!\n");
        return 1;
    }

    sntp_cmd_param * sntp_param = pvPortMalloc(sizeof(sntp_cmd_param));
    sntp_param->cmd = SNTP_START;
    sntp_run(sntp_param);
    
    return 0;
}


unsigned int get_sntp_use(void)
{
    return sntp_get_use();
}

unsigned int set_sntp_use(int use)
{
    if (use <= 0) {
        if (delete_nvram_env(NVR_KEY_SNTP_C) <= 0) {
            return 1; /* error */
        }
        stop_sntp();
    } else {
        if (write_nvram_int(NVR_KEY_SNTP_C, MODE_ENABLE) == 0) {
            if (use == pdTRUE) {
                start_sntp();
            }
        } else {
            return 1; /* error */
        }
    }
    return 0; /* success */
}

void get_sntp_server(char *svraddr, unsigned int index)
{
    strncpy(svraddr, sntp_get_server((sntp_svr_t)index), 32);

}

unsigned char set_sntp_server(unsigned char *svraddr, unsigned int index)
{
    return sntp_set_server((char *)svraddr, index);
}


unsigned char set_sntp_period(int period)
{
    int status;

    status = (int)sntp_set_period(period);

    return status;
}

int get_sntp_period(void)
{
    if (sntp_support_flag == pdTRUE) {
        return sntp_get_period();
    } else {
        PRINTF("SNTP not supported ...\n");
        return DFLT_SNTP_SYNC_PERIOD;
    }
}

unsigned int is_sntp_sync(void)
{
    return sntp_sync_flag;
}

/* EOF */
