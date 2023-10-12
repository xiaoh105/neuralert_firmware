/*
 * wpa_supplicant/hostapd / Internal implementation of OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file is an example of operating system specific  wrapper functions.
 * This version implements many of the functions internally, so it can be used
 * to fill in missing functions from the target system C libraries.
 *
 * Some of the functions are using standard C library calls in order to keep
 * this file in working condition to allow the functions to be tested on a
 * Linux target. Please note that OS_NO_C_LIB_DEFINES needs to be defined for
 * this file to work correctly. Note that these implementations are only
 * examples and are not optimized for speed.
 *
 *
 * Copyright (c) 2019-2022 Modified by Renesas Electronics.
 *
 */

#include "includes.h"

#include "oal.h"
#include "os.h"
#include "supp_eloop.h"
#include "time.h"

#ifdef	BUILD_OPT_FC9000_ROMNDK
#include "rom_environ.h"
#endif	/* BUILD_OPT_FC9000_ROMNDK */

extern unsigned long long RTC_GET_COUNTER(void);


void os_sleep(os_time_t sec, os_time_t usec)
{
	if (sec)
		vTaskDelay(sec * 100);

	if (usec > 1000)
		vTaskDelay(usec / 10000);
}

int os_get_time(struct os_time *t)
{
	ULONG now;

	now =  xTaskGetTickCount();

	t->sec = now / ONE_SEC_TICK;
	t->usec = (now % ONE_SEC_TICK) * 10000;

	return 1;
}

int os_get_reltime(struct os_reltime *t)
{
	return os_get_time((struct os_time *)t);
}

#ifdef	BUILD_OPT_FC9000_ROMNDK
int os_mktime(int year, int month, int day, int hour, int min, int sec, os_time_t *t)
{
	return da16x_os_mktime(year, month, day, hour, min, sec, (void *)t);
}

#else

int os_mktime(int year, int month, int day, int hour, int min, int sec, os_time_t *t)
{
	struct tm tm;

	if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
	    hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 ||
	    sec > 60)
		return -1;

	os_memset(&tm, 0, sizeof(tm));
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;

	*t = (os_time_t) mktime(&tm);
	*t += 0x83AA7E80 - 0x7E90;	/* Synchronize to ThreadX NTP */

	return 0;
}
#endif	/* BUILD_OPT_FC9000_ROMNDK */

#ifdef	BUILD_OPT_FC9000_ROMNDK
int os_gmtime(os_time_t t, struct os_tm *tm)
{
	return da16x_os_gmtime((void *)(&t), (void *)tm);
}
#else
int os_gmtime(os_time_t t, struct os_tm *tm)
{
	struct tm *tm2;
	time_t t2 = t;

	tm2 = gmtime(&t2);
	if (tm2 == NULL)
		return -1;
	tm->sec = tm2->tm_sec;
	tm->min = tm2->tm_min;
	tm->hour = tm2->tm_hour;
	tm->day = tm2->tm_mday;
	tm->month = tm2->tm_mon + 1;
	tm->year = tm2->tm_year + 1900;
	return 0;
}
#endif	/* BUILD_OPT_FC9000_ROMNDK */

int os_get_random(unsigned char *buf, size_t len)
{
	unsigned int i;
	ULONG now = (UINT)RTC_GET_COUNTER();
	u8 *tmp_nonce;

	tmp_nonce = os_zalloc(len);

	srand((unsigned int)now);
	for (i = 0; i < len; i++)
		tmp_nonce[i] = rand() % 256;

	memcpy(buf, tmp_nonce, len);

	os_free(tmp_nonce);
	return 0;
}


unsigned long os_random(void)
{
	return rand();
}

#ifdef DEBUG_SUPP_MALLOC
#define	os_zalloc(...) _os_zalloc(__VA_ARGS__, __func__)
#define	os_malloc(...) supp_malloc(__VA_ARGS__, __func__)
void * _os_zalloc(size_t size, const char* caller)
#else
void * os_zalloc(size_t size)
#endif /* DEBUG_SUPP_MALLOC */
{
#ifdef DEBUG_SUPP_MALLOC
	void *n = supp_malloc(size, caller);
#else
	void *n = os_malloc(size);
#endif /* DEBUG_SUPP_MALLOC */

	if (n)
		os_memset(n, 0, size);
	return n;
}

size_t os_strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}

/* EOF */
