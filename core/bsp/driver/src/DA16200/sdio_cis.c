/**
 ****************************************************************************************
 *
 * @file sdio_cis.c
 *
 * @brief SDIO CIS
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

#include "sdio_cis.h"
#include "emmc.h"
#include "hal.h"

static int version_1_for_cistpl(HANDLE handler, UINT8 *buf, UINT32 size)
{
	EMMC_HANDLER_TYPE *emmc = NULL;
	UINT32 i, nr_strings;
	UINT8 **buffer, *string;
	emmc = (EMMC_HANDLER_TYPE*)handler;

	buf += 2;
	size -= 2;

	nr_strings = 0;
	for (i = 0; i < size; i++) {
		if (buf[i] == 0xff)
			break;
		if (buf[i] == 0)
			nr_strings++;
	}
	if (nr_strings == 0)
		return ERR_CIS_NONE;

	size = i;

	buffer = (UINT8 **)HAL_MALLOC(sizeof(char*) * nr_strings + size);
	if (!buffer)
		return -ERR_CIS_NOMEM;

	string = (UINT8*)(buffer + nr_strings);

	for (i = 0; i < nr_strings; i++) {
		buffer[i] = string;
		strcpy((char *)string, (char const *)buf);
		string += strlen((char const *)string) + 1;
		buf += strlen((char const *)buf) + 1;
	}
	emmc->sdio_num_info = nr_strings;
	emmc->psdio_info = buffer;

	return ERR_CIS_NONE;
}

static int man_fid_for_cistpl(HANDLE handler, UINT8 *buf, UINT32 size)
{
	DA16X_UNUSED_ARG(size);

	UINT16 vendor, device;
	EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE*)handler;

	/* TPLMID_MANF */
	vendor = (UINT16)(buf[0] | (buf[1] << 8));

	/* TPLMID_CARD */
	device = (UINT16)(buf[2] | (buf[3] << 8));

	emmc->cis.vendor = vendor;
	emmc->cis.device = device;

	return ERR_CIS_NONE;
}

static const unsigned char speed_val[16] =
	{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
static const unsigned int speed_unit[8] =
	{ 10000, 100000, 1000000, 10000000, 0, 0, 0, 0 };


typedef int (tpl_parse_t)(HANDLE handler, UINT8 *buf, UINT32 size);

struct tuples_for_cis {
	UINT8 code;
	UINT8 min_size;
	tpl_parse_t *parse;
};

static int parse_for_cis_tuple(HANDLE handler, UINT8 *tpl_descr, const struct tuples_for_cis *tpl,
						 int tpl_count, UINT8 code, UINT8 *buf, UINT32 size)
{
	DA16X_UNUSED_ARG(tpl_descr);

	int i, ret;

	/* look for a matching code in the table */
	for (i = 0; i < tpl_count; i++, tpl++)
	{
		if (tpl->code == code)
			break;
	}
	if (i < tpl_count)
	{
		if (size >= tpl->min_size)
		{
			if (tpl->parse)
				ret = tpl->parse(handler, buf, size);
			else
				ret = ERR_NONE;	/* known tuple, not parsed */
		}
		else
		{
			/* invalid tuple */
			ret = ERR_CIS_UNKNOWN;
		}
	}
	else
	{
		/* unknown tuple */
		ret = ERR_CIS_UNKNOWN;
	}

	return ret;
}

static int common_fne_for_cistpl(HANDLE handler, UINT8 *buf, UINT32 size)
{
	DA16X_UNUSED_ARG(size);

	EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE*)handler;
	/* TPLFE_FN0_BLK_SIZE */
	emmc->cis.blksize = (unsigned short)(buf[1] | (buf[2] << 8));

	/* TPLFE_MAX_TRAN_SPEED */
	emmc->cis.max_dtr = speed_val[(buf[3] >> 3) & 15] *
			    speed_unit[buf[3] & 7];

	return ERR_CIS_NONE;
}

static int fn_fne_for_cistpl(HANDLE handler, UINT8 *buf, UINT32 size)
{
	unsigned vsn;
	unsigned min_size;
	EMMC_HANDLER_TYPE *emmc = (EMMC_HANDLER_TYPE*)handler;

	/*
	 * This tuple has a different length depending on the SDIO spec
	 * version.
	 */
	vsn = emmc->cccr.sdio_vsn;
	min_size = (vsn == DIALOG_SDIO_SDIO_R_1_00) ? 28 : 42;

	if (size < min_size)
		return ERR_CIS_INVAL;

	/* TPLFE_MAX_BLK_SIZE */
	emmc->max_blk_size = (UINT32)(buf[12] | (buf[13] << 8));

	/* TPLFE_ENABLE_TIMEOUT_VAL, present in ver 1.1 and above */
	if (vsn > DIALOG_SDIO_SDIO_R_1_00)
		emmc->cis.enable_timeout = (UINT32)((buf[28] | (buf[29] << 8)) * 10);
	//else
	//	emmc->cis.enable_timeout  = jiffies_to_msecs(HZ);

	return ERR_CIS_NONE;
}

static const struct tuples_for_cis func_list_for_cis_tpl[] = {
	{	0,	4,	common_fne_for_cistpl	},
	{	1,	0,	fn_fne_for_cistpl		},
	{	4,	8,	NULL					},
};

#define	CISTPL_FUNC_NAME	"CISTPL_FUNCE"

static int local_cistpl_fn(HANDLE handler, UINT8 *buffer, UINT32 sz)
{
	if (sz < 1)
		return -ERR_CIS_INVAL;

	return parse_for_cis_tuple(handler, (UINT8 *)CISTPL_FUNC_NAME, func_list_for_cis_tpl, ARRAY_SIZE(func_list_for_cis_tpl), buffer[0], buffer, sz);
}

/* Known TPL_CODEs table for CIS tuples */
static const struct tuples_for_cis list_cis_tpl[] = {
	{	21,	3,	version_1_for_cistpl	},
	{	32,	4,	man_fid_for_cistpl		},
	{	33,	2,	NULL					},
	{	34,	0,	local_cistpl_fn			},
};

#define	CIS_NAME	"CIS"
int sdio_cis_tpl_parse(HANDLE handler, UINT8 *data, UINT8 tpl_code, UINT8 tpl_link)
{
	int ret;
	ret = parse_for_cis_tuple(handler, (UINT8 *)CIS_NAME, list_cis_tpl, ARRAY_SIZE(list_cis_tpl), tpl_code , data, (UINT32)tpl_link);
	return ret;
}
