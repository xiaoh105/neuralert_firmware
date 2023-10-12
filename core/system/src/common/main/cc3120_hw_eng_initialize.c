/**
 ****************************************************************************************
 *
 * @file cc3120_hw_engine_initialize.c
 *
 * @brief initialize APIs for CC3120 HW Security Engine 
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


#include "cc3120_hw_eng_initialize.h"
#include "da16x_crypto.h"

static int da16x_secure_module_cnt = 0;
static const unsigned int da16x_secure_module_rflag = 0xffffffff;

void da16x_secure_module_init(void)
{
	da16x_secure_module_cnt++;

	if (da16x_secure_module_cnt == 1) {
		DA16X_Crypto_Init(da16x_secure_module_rflag);
	}

	/*
	PRINTF("[%s:%d]secure module cnt:%d\r\n",
		__func__, __LINE__, da16x_secure_module_cnt);
	*/

	return ;
}

void da16x_secure_module_deinit(void)
{
	if (da16x_secure_module_cnt == 1) {
		DA16X_Crypto_Finish();
	} else if (da16x_secure_module_cnt == 0) {
		return ;
	}

	da16x_secure_module_cnt--;

	/*
	PRINTF("[%s:%d]secure module cnt:%d\r\n",
		__func__, __LINE__, da16x_secure_module_cnt);
	*/

	return ;
}

/* EOF */
