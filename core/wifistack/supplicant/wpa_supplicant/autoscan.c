/*
 * WPA Supplicant - auto scan
 * Copyright (c) 2012, Intel Corporation. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "supp_common.h"
#include "supp_config.h"
#include "wpa_supplicant_i.h"
#include "bss.h"
#include "supp_scan.h"
#include "autoscan.h"

#ifdef CONFIG_AUTOSCAN_EXPONENTIAL
extern const struct autoscan_ops autoscan_exponential_ops;
#endif /* CONFIG_AUTOSCAN_EXPONENTIAL */

#ifdef CONFIG_AUTOSCAN_PERIODIC
extern const struct autoscan_ops autoscan_periodic_ops;
#endif /* CONFIG_AUTOSCAN_PERIODIC */

#ifdef CONFIG_AUTOSCAN

static const struct autoscan_ops * autoscan_modules[] = {
#ifdef CONFIG_AUTOSCAN_EXPONENTIAL
	&autoscan_exponential_ops,
#endif /* CONFIG_AUTOSCAN_EXPONENTIAL */
#ifdef CONFIG_AUTOSCAN_PERIODIC
	&autoscan_periodic_ops,
#endif /* CONFIG_AUTOSCAN_PERIODIC */
	NULL
};

static void request_scan(struct wpa_supplicant *wpa_s)
{
	wpa_s->scan_req = MANUAL_SCAN_REQ;

	wpa_supplicant_req_scan(wpa_s, wpa_s->scan_interval, 0);
}


int autoscan_init(struct wpa_supplicant *wpa_s, int req_scan)
{
	const char *name = wpa_s->conf->autoscan;
	const char *params;
	size_t nlen;
	int i;
	const struct autoscan_ops *ops = NULL;

	da16x_scan_prt("[%s] START : req_scan=%d\n", __func__, req_scan);

	if (wpa_s->autoscan && wpa_s->autoscan_priv) {
		da16x_scan_prt("[%s] FINISH #1\n", __func__);
		return 0;
	}

	if (name == NULL) {
		da16x_scan_prt("[%s] FINISH #2\n", __func__);
		return 0;
	}

	params = os_strchr(name, ':');
	if (params == NULL) {
		params = "";
		nlen = os_strlen(name);
	} else {
		nlen = params - name;
		params++;
	}

	for (i = 0; autoscan_modules[i]; i++) {
		if (os_strncmp(name, autoscan_modules[i]->name, nlen) == 0) {
			ops = autoscan_modules[i];
			break;
		}
	}

	if (ops == NULL) {
		da16x_prt("[%s] Could not find module matching the "
			"parameter '%s'\n", __func__, name);
		return -1;
	}

	wpa_s->autoscan_params = NULL;

	wpa_s->autoscan_priv = ops->init(wpa_s, params);
	if (wpa_s->autoscan_priv == NULL)
		return -1;
	wpa_s->autoscan = ops;

	da16x_prt("[%s] Initialized module '%s' with "
		   "parameters '%s'\n", __func__, ops->name, params);
	if (!req_scan)
		return 0;

	/* Cancelling existing scan requests, if any. */
	wpa_supplicant_cancel_scan(wpa_s);

	/*
	 * Firing first scan, which will lead to call autoscan_notify_scan.
	 */
	request_scan(wpa_s);

	da16x_scan_prt("[%s] FINISH\n", __func__);

	return 0;
}


void autoscan_deinit(struct wpa_supplicant *wpa_s)
{
	if (wpa_s->autoscan && wpa_s->autoscan_priv) {
		da16x_scan_prt("[%s] Deinitializing module '%s'\n",
			   __func__, wpa_s->autoscan->name);

		wpa_s->autoscan->deinit(wpa_s->autoscan_priv);
		wpa_s->autoscan = NULL;
		wpa_s->autoscan_priv = NULL;

		wpa_s->scan_interval = 5;
	}
}


int autoscan_notify_scan(struct wpa_supplicant *wpa_s,
			 struct wpa_scan_results *scan_res)
{
	int interval;

	if (wpa_s->autoscan && wpa_s->autoscan_priv) {
		interval = wpa_s->autoscan->notify_scan(wpa_s->autoscan_priv,
							scan_res);

		if (interval <= 0)
			return -1;

		wpa_s->scan_interval = interval;

		request_scan(wpa_s);
	}

	return 0;
}
#endif

/* EOF */
